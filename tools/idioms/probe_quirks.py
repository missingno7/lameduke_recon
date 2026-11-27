"""Controlled DOS compiler fixtures for observations from the STRICT index."""
import os
from collections import defaultdict

from neutrality import ROOT, WORK, CONFIG, doshost, dump_json, load_json, omf, sha256_file, wcc
import blockdiff as bd


PROGRAM = '''
extern long side(long);
extern long sink(long, long, long, long, long, long);
extern long old_sink();
extern long typed_short(short);
extern signed char sc;
extern unsigned char uc;
extern signed short ss;
extern unsigned short us;
extern long plain;
extern volatile long vol;
extern volatile short vshort;

#define FIVE 5
enum { E_FIVE = 5 };
long cmpliteral(void) { return ss == FIVE; }
long cmpenum(void) { return ss == E_FIVE; }
long cmpuchar(void) { return uc == FIVE; }
long widenschar(void) { return sc; }
long widenuchar(void) { return uc; }
long widensshort(void) { return ss; }
long widenushort(void) { return us; }
long widenvolatile(void) { return vshort; }
long readplain(void) { return plain + plain; }
long readvolatile(void) { return vol + vol; }
void storeplain(void) { plain=1; plain=2; }
void storevolatile(void) { vol=1; vol=2; }
long andalso(void) { return plain && side(1); }
long orelse(void) { return plain || side(1); }
long ternfold(void) { return plain ? 1 : 0; }
long ternconst(void) { return plain ? 11 : 22; }
long signed16(long a) { return a / 16; }
unsigned long unsigned16(unsigned long a) { return a / 16; }
long signed9(long a) { return a / 9; }
unsigned long unsigned9(unsigned long a) { return a / 9; }
long folded(void) { return (4 * 9 + 2) / 2; }
long times31(long a) { return a * 31; }
long times65(long a) { return a * 65; }
long times38(long a) { return a * 38; }
long args(void) { return sink(side(1), side(2), side(3), side(4), side(5), side(6)); }
long oldarg(long a) { return old_sink(a); }
long narrowarg(long a) { return typed_short(a); }
long oldchar(void) { return old_sink(sc); }
long narrowchar(void) { return typed_short(sc); }
extern void string_sink(const char *);
void string_a(void) { string_sink("the same text"); }
void string_b(void) { string_sink("the same text"); }
void string_c(void) { string_sink("other text"); }
'''


def switches():
    text = [f'extern long arm{n}(long);' for n in range(17)]
    info = {}
    for step in (1, 2, 3, 4, 8, 16):
        for n in (2, 3, 4, 5, 6, 8, 10, 12, 16):
            name = f'sw_{n}_{step}'
            text.append(f'long {name}(long x) {{ switch(x) {{')
            text.extend(f'case {i * step + 100}: return arm{i}(x);' for i in range(n))
            text.append('default: return arm16(x); } }')
            info[name] = {'cases': n, 'step': step, 'span': 1 + (n - 1) * step}
    return '\n'.join(text), info


def examine(path, info):
    mod, = omf.load(path)
    seg = mod.segments[1]
    pubs = defaultdict(list)
    for name, si, off, _ in mod.publics:
        if si == 1:
            pubs[off].append(bd.match.demangle(name))
    starts = sorted(pubs)
    fixups = {f.off: f for f in mod.fixups if f.seg == 1}
    case_output, output = [], {}
    # Direct tail-call jump tables can contain extern fixups. Find the base
    # from the SIB jump and count consecutive relocated slots preceding entry.
    for off, end in zip(starts, [*starts[1:], seg.size]):
        fn = pubs[off][0]
        asa = []
        for ins in bd.MD.disasm(bytes(seg.data[off:end]), off):
            if ins.address in fixups:
                break
            fd = fixups.get(ins.address + (ins.disp_offset or ins.imm_offset))
            desc = ' '.join([ins.mnemonic, ins.op_str]).strip()
            if fd:
                desc += f' <{mod.target_name(fd)}:{fd.disp:#x}>'
            asa.append(desc)
            if fn in info and ins.mnemonic == 'jmp' and '[e' in ins.op_str and '*4' in ins.op_str and fd:
                field = int.from_bytes(seg.data[fd.off:fd.off+4], 'little')
                base = field + fd.disp
                count = 0
                while base + count * 4 in fixups:
                    count += 1
                case_output.append({'function': fn, **info[fn], 'style': 'jump_table', 'dispatch': desc,
                                    'entry': off, 'table_base': base, 'slots': count})
                break
        if fn in info:
            if not case_output or case_output[-1]['function'] != fn:
                case_output.append({'function': fn, **info[fn], 'style': 'comparisons', 'entry': off})
        else:
            for alias in pubs[off]:
                output[alias] = {'entry': off, 'instructions': asa, 'same_address': pubs[off]}
    return case_output, output


def main():
    os.environ['LAMEDUKE_HOST'] = 'dos'
    doshost.WORKER = WORK / 'tmp'
    fixture = WORK / 'fixtures'
    fixture.mkdir(exist_ok=True)
    src = fixture / 'shapes.c'
    generated, info = switches()
    src.write_text(PROGRAM + '\n' + generated + '\n', encoding='ascii')
    result = {}
    for profile in ('engine', 'game_oe', 'multi', 'audiolib'):
        p = wcc.profile(profile)
        out = fixture / f'{profile}.obj'
        rc, diagnostics = wcc.compile_c(src, out, p['flags'], p['install'])
        if rc:
            raise SystemExit(diagnostics)
        cases, output = examine(out, info)
        result[profile] = {'install': p['install'], 'flags': p['flags'], 'object_sha256': sha256_file(out),
                           'case_matrix': cases, 'functions': output}
        cutoffs = defaultdict(list)
        for row in cases:
            if row['style'] == 'jump_table':
                cutoffs[row['step']].append(row['cases'])
        print(profile, dict(cutoffs), flush=True)
    dump_json(WORK / 'probes.json', result)


if __name__ == '__main__':
    main()
