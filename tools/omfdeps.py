"""Print Watcom OMF dependency comments (COMENT class 0xE9) of objects.
Two layouts seen: DOS time+date words (Capstone 1995 objects, headers dated 1994-09-01) and a 32-bit
time_t (Ken's own objects).  Output tags which layout decoded."""
import struct, sys, time
sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def dosok(tm, dt):
    return 1 <= (dt >> 5) & 15 <= 12 and 1 <= dt & 31 <= 31 and tm >> 11 <= 23 and (tm >> 5) & 63 <= 59 and (tm & 31) <= 29

def deps(p):
    d = open(p, 'rb').read()
    i = 0; out = []; th = None
    while i + 3 <= len(d):
        t = d[i]; ln = struct.unpack_from('<H', d, i + 1)[0]; b = d[i + 3:i + 2 + ln]
        if t == 0x80:
            th = b[1:1 + b[0]].decode('latin-1')
        if t == 0x88 and len(b) > 6 and b[1] == 0xE9:
            tm, dt = struct.unpack_from('<HH', b, 2)
            n = b[6]; name = b[7:7 + n].decode('latin-1')
            ts = struct.unpack_from('<I', b, 2)[0]
            if dosok(tm, dt) and 1990 <= (dt >> 9) + 1980 <= 1999:
                out.append(('%04d-%02d-%02d %02d:%02d:%02d' % ((dt >> 9) + 1980, (dt >> 5) & 15, dt & 31,
                            tm >> 11, (tm >> 5) & 63, (tm & 31) * 2), name, 'dos'))
            else:
                out.append((time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(ts)), name, 'time_t'))
        if t in (0x8a, 0x8b):
            break
        i += 3 + ln
    return th, out

if __name__ == '__main__':
    for p in sys.argv[1:]:
        th, out = deps(p)
        print('==', p, th)
        for o in out:
            print('   %s %s [%s]' % o)
