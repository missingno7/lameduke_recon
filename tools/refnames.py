"""Extract obj1 addresses -> function names from external/lameduke-raynorpat source comments (/* 0xADDR ... */ above a
function). Writes build/map/raynorpat_functions.json. Semantic/naming aid only, never matching evidence."""
import re, json, glob, sys
sys.path.insert(0,'tools')
fm=json.load(open('build/map/funcmap.json'))
starts={f['start'] for f in fm['functions']}
root='external/lameduke-raynorpat/source'
res={}
pat=re.compile(r'/\*\s*0x([0-9A-Fa-f]{5,6})\b[^*]*?\*/\s*\n((?:[^\n]*\n){0,3})', re.S)
for f in glob.glob(root+'/**/*.c', recursive=True):
    s=open(f,encoding='latin-1').read()
    for m in pat.finditer(s):
        a=int(m.group(1),16); body=m.group(2)
        fn=re.search(r'^\s*(?:static\s+)?[\w\s\*]+?\b(\w+)\s*\([^;]*$', body, re.M)
        if fn and not fn.group(1) in ('if','while','for','switch','return'):
            res.setdefault(a,set()).add((fn.group(1), f.replace(root+'/','')))
off={a-0x10000:v for a,v in res.items() if 0x10000<=a<0x80000}
hit=sum(1 for a in off if a in starts)
print('function-annotated addresses',len(res),'in obj1',len(off),'at funcmap starts',hit)
regions=[(0,0x33e5c,'game'),(0x33e5c,0x47abf,'engine'),(0x47abf,0x49c84,'multi'),(0x49c84,0x62d8d,'audio'),(0x62d8d,0x6c445,'rt/asm')]
for lo,hi,n in regions:
    print(n, sum(1 for a in off if lo<=a<hi), 'of', sum(1 for s in starts if lo<=s<hi),'starts')
out={f"1:{a:05x}":sorted(v) for a,v in sorted(off.items())}
json.dump(out,open('build/map/raynorpat_functions.json','w'),indent=0)
for a in list(sorted(off))[:8]: print(hex(a), off[a])
