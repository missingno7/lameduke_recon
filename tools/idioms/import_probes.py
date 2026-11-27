"""Small DOS controls for cross-version Watcom hypotheses."""
from collections import Counter
import os

from neutrality import WORK, doshost, dump_json, omf, sha256_file, wcc
from probe_quirks import examine

PROGRAM = r'''
#include <string.h>
extern int effect(int);
extern int consume(int, int, int, int, int, int);
extern int take_short(short);
extern int take_int(int);
extern int ga, gb, gc, gd;
extern signed char cbyte;
extern unsigned char ubyte;
extern signed short sword;
extern unsigned short uword;
extern int *rptr;
extern int matrix[10][4];
extern int flat[40];

int eqbyte(int x) { return (signed char)x == 0; }
int truthbyte(int x) { return !(signed char)x; }
int eqglobal(void) { return cbyte == 0; }
int truthglobal(void) { return !cbyte; }
int nested(int a, int b) { if (a) if (b) effect(3); return 7; }
int conjunction(int a, int b) { if (a && b) effect(3); return 7; }
int disjunction(int a, int b) { if (a || b) effect(3); return 7; }
int postinc(int i) { i++; return effect(i); }
int preinc(int i) { ++i; return effect(i); }
int loopfor(int lo, int n) { int i; for (i=lo; i<n; i++) effect(i); return i; }
int loopwhile(int lo, int n) { int i=lo; while(i<n) {effect(i); i++;} return i; }
int loopdo(int lo, int n) { int i=lo; do {effect(i); i++;} while(i<n); return i; }
int oldstack(void) { return consume(1,2,3,4,5,6); }
int shortarg(void) { return take_short(-1); }
int intarg(void) { return take_int(-1); }
int cmpab(void) { return ga < gb; }
int cmpba(void) { return gb > ga; }
int sumdeeper(void) { return ga + gb*gc; }
int sumdepthrev(void) { return gb*gc + ga; }
int sumxy(int x,int y) { return x*x+y*y; }
int sumyx(int x,int y) { return y*y+x*x; }
int sumretain(void) { ga=ga+1; return ga; }
int sideeffect(int x) { return consume(effect(x),2,effect(x+1),4,5,6); }
int boolassign(void) { if (ga) gb=1; else gb=0; return gb; }
int boolcopy(void) { return gb=(ga!=0); }
int bitlong(void) { return (ga & 0x1000) != 0; }
int bitarg(int x) { return (x & 0x1000) != 0; }
int arrflat(int i) { return flat[i*4+2]; }
int arrnested(int i) { return matrix[i][2]; }
int assigncond(int x) { if ((*rptr=x)<0) return effect(1); return 0; }
int ptrcond(void) { if (*rptr<0) return effect(1); return 0; }
void writepost(char *p, char x) { *p++=x; effect(*p); }
void stringcopy(char *p, const char *q, unsigned n) { memcpy(p,q,n); }
void stringcopy4(char *p, const char *q) { memcpy(p,q,4); }
void stringsvoid(void) { memset(&ga,0,sizeof(ga)); }
void stringmemset(char *p, unsigned n) { memset(p,0,n); }
int leadd(int i) { return effect(__LINE__+i); }
struct packed { char b; int d; short s; };
int packsize(void) { return sizeof(struct packed); }
int packoffset(struct packed *p) { return p->d; }
struct flags { unsigned char lo:2, hi:6; };
int flagsread(struct flags *f) { return f->hi; }
int flagstest(struct flags *f) { return f->hi != 0; }
int bytemask(unsigned char *f) { return (*f & 0xfc) != 0; }
void flagsstore(struct flags *f,int x) { f->hi=x; }
enum Tiny { ZERO, ONE, TWO };
enum Tiny enumv=ONE;
char ca[]="A";
char cb[]="B";
short alshort=3;
int alignint=7;
double aligned_double=0.1052628002663471742;
double vals[] = {1.0000000000000002, 1.2345678901234567, 0.1, 3.141592653589793};
double some_fp(double a,double b) { return a/b+aligned_double; }
static int shiftx(int a) { return a+1; }
int thin1(int a) { return shiftx(a); }
int thin2(int a) { return thin1(a)+2; }
int thin3(int a) { return thin2(a)+3; }
'''


def fixture():
    arms = range(24)
    code = [PROGRAM, *(f'extern int alt_{i}(int);' for i in arms)]
    for count in (22, 24):
        for step in (3, 9, 27):
            code.append(f'int sw_{count}_{step}(unsigned short i) {{ switch(i) {{')
            code.extend(f'case {j*step+100}: return alt_{j}(i);' for j in range(count))
            code.append('default: return effect(i); }}')
    return '\n'.join(code) + '\n'


def main():
    os.environ['LAMEDUKE_HOST'] = 'dos'
    doshost.WORKER = WORK / 'tmp'
    tree = WORK / 'fixtures'
    tree.mkdir(exist_ok=True)
    src = tree / 'imports.c'
    src.write_text(fixture(), encoding='ascii')
    settings = {p: wcc.profile(p)['flags'] for p in ('engine', 'game_oe', 'multi')}
    settings.update({'stackd2': ['-3s', '-d2', '-s'], 'stackod': ['-3s', '-od', '-s'],
                     'stackotod': ['-3s', '-ot', '-od', '-s'], 'g5': ['-5r', '-s', '-otexan', '-oe=3000'],
                     'of': ['-4r', '-s', '-or', '-of'], 'soft': ['-5r', '-s', '-fpc']})
    result = {}
    for name, flags in settings.items():
        path = tree / f'imp_{name}.obj'
        rc, diagnostics = wcc.compile_c(src, path, flags, 'wc95b')
        if rc:
            raise SystemExit(diagnostics)
        _unused, functions = examine(path, {})
        mod, = omf.load(path)
        text_segments = [i for i,s in enumerate(mod.segments) if s and s.cls == 'CODE']
        pub = [(n,mod.segments[s].name,v,l) for n,s,v,l in mod.publics if s]
        data = {s.name: bytes(s.data).hex() for s in mod.segments if s and 0 < s.size < 300 and s.cls == 'DATA'}
        result[name] = {'flags': flags, 'sha256': sha256_file(path), 'functions': functions, 'publics': pub,
                        'data': data, 'code_segments': [(mod.segments[s].name,mod.segments[s].align) for s in text_segments],
                        'code_chunk_lengths': dict(Counter(n for s,o,n in mod.chunks if s in text_segments))}
        print(name, len(functions), flush=True)
    dump_json(WORK / 'import_probes.json', result)


if __name__ == '__main__':
    main()
