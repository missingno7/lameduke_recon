"""Read-only WOW64 breakpoint transport, adapted from the semir worker's GPLv2 PoC."""
import ctypes as C
from ctypes import wintypes as W
import struct
import subprocess
import tempfile
import time


class Context(C.Structure):
    _fields_ = (
        [(name, W.DWORD) for name in ("flags", "dr0", "dr1", "dr2", "dr3", "dr6", "dr7")]
        + [("floating", C.c_byte * 112)]
        + [(name, W.DWORD) for name in ("gs", "fs", "es", "ds", "edi", "esi", "ebx", "edx",
                                         "ecx", "eax", "ebp", "eip", "cs", "eflags", "esp", "ss")]
        + [("extended", C.c_byte * 512)]
    )


CONTINUE = 0x10002
NOT_HANDLED = 0x80010001


def checked(result):
    if not result:
        raise C.WinError(C.get_last_error())


class Debugger:
    def __init__(self, timeout=35):
        if C.sizeof(C.c_void_p) != 8 or C.sizeof(Context) != 716:
            raise RuntimeError("requires 64-bit Python on Windows with WOW64")
        self.k = C.WinDLL("kernel32", use_last_error=True)
        self.k.WaitForDebugEvent.argtypes = [C.c_void_p, W.DWORD]
        self.k.ContinueDebugEvent.argtypes = [W.DWORD, W.DWORD, W.DWORD]
        self.k.Wow64GetThreadContext.argtypes = [W.HANDLE, C.POINTER(Context)]
        self.k.Wow64SetThreadContext.argtypes = [W.HANDLE, C.POINTER(Context)]
        self.k.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t,
                                            C.POINTER(C.c_size_t)]
        self.k.CloseHandle.argtypes = [W.HANDLE]
        self.timeout = timeout
        self.breakpoints = ()
        self.thread = None
        self.process = None
        self.image = None

    def read(self, address, size):
        buf = C.create_string_buffer(size)
        got = C.c_size_t()
        checked(self.k.ReadProcessMemory(self.process, address, buf, size, C.byref(got)))
        if got.value != size:
            raise RuntimeError(f"short debugger read at {address:#x}")
        return buf.raw

    def word(self, address):
        return struct.unpack("<I", self.read(address, 4))[0]

    def string(self, address, limit=256):
        chars = bytearray()
        while address and len(chars) < limit:
            size = min(32, limit - len(chars), 4096 - (address & 4095))
            data = self.read(address, size)
            chars.extend(data.split(b"\0", 1)[0])
            if b"\0" in data:
                return chars.decode("ascii", errors="replace")
            address += size
        return None

    def context(self):
        ctx = Context()
        ctx.flags = 0x10013
        checked(self.k.Wow64GetThreadContext(self.thread, C.byref(ctx)))
        return ctx

    def arm(self, ctx, *, resume=False):
        if len(self.breakpoints) > 4:
            raise RuntimeError("WOW64 provides four hardware breakpoint slots")
        ctx.dr6 &= ~15
        ctx.dr7 = 0
        for slot, rva in enumerate(self.breakpoints):
            setattr(ctx, f"dr{slot}", self.image + rva)
            ctx.dr7 |= 1 << (slot * 2)
        if resume:
            ctx.eflags |= 0x10000
        checked(self.k.Wow64SetThreadContext(self.thread, C.byref(ctx)))

    def exception(self, code, tid):
        if tid != self.tid:
            return NOT_HANDLED
        if code in (0x80000003, 0x4000001F) and not self.armed:
            self.arm(self.context())
            self.armed = True
            return CONTINUE
        if code in (0x80000004, 0x4000001E):
            ctx = self.context()
            rva = ctx.eip - self.image
            if ctx.dr6 & 15 and rva in self.breakpoints:
                self.hit(rva, ctx)
                self.arm(ctx, resume=True)
                return CONTINUE
        return NOT_HANDLED

    def run(self, cmd, env, cwd, **_kwargs):
        with tempfile.TemporaryFile(dir=cwd) as out, tempfile.TemporaryFile(dir=cwd) as err:
            proc = subprocess.Popen(cmd, cwd=cwd, env=env, stdout=out, stderr=err,
                                    creationflags=2 | subprocess.CREATE_NO_WINDOW)
            pending = None
            exited = False
            self.armed = False
            event = C.create_string_buffer(176)  # native x64 DEBUG_EVENT
            deadline = time.monotonic() + self.timeout
            try:
                while not exited:
                    if time.monotonic() >= deadline:
                        raise TimeoutError(f"debug compile exceeded {self.timeout} seconds")
                    if not self.k.WaitForDebugEvent(event, 200):
                        if C.get_last_error() not in (121, 258):
                            raise C.WinError(C.get_last_error())
                        continue
                    kind, pid, tid = struct.unpack_from("<III", event.raw)
                    pending = (pid, tid)
                    status = CONTINUE
                    if kind == 3:
                        handle, self.process, self.thread, self.image = struct.unpack_from("<4Q", event.raw, 16)
                        self.tid = tid
                        if handle:
                            self.k.CloseHandle(handle)
                    elif kind in (2, 6):
                        if kind == 2:
                            raise RuntimeError("expected a single-threaded compiler")
                        handle, = struct.unpack_from("<Q", event.raw, 16)
                        if handle:
                            self.k.CloseHandle(handle)
                    elif kind == 1:
                        status = self.exception(struct.unpack_from("<I", event.raw, 16)[0], tid)
                    elif kind == 5:
                        exited = True
                    checked(self.k.ContinueDebugEvent(pid, tid, status))
                    pending = None
                rc = proc.wait(timeout=5)
                out.seek(0)
                err.seek(0)
                return subprocess.CompletedProcess(cmd, rc, out.read().decode(errors="replace"),
                                                   err.read().decode(errors="replace"))
            finally:
                # Process/thread handles in debug events close on the continued exit event.
                if not exited:
                    proc.kill()
                    if pending:
                        self.k.ContinueDebugEvent(*pending, CONTINUE)
                    cleanup = time.monotonic() + 5
                    while time.monotonic() < cleanup:
                        if self.k.WaitForDebugEvent(event, 100):
                            kind, pid, tid = struct.unpack_from("<III", event.raw)
                            if kind in (3, 6):
                                handle, = struct.unpack_from("<Q", event.raw, 16)
                                if handle:
                                    self.k.CloseHandle(handle)
                            self.k.ContinueDebugEvent(pid, tid, CONTINUE)
                            if kind == 5:
                                break
                    proc.wait(timeout=5)
