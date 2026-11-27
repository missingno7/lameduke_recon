"""Compare selected 9.5 GA and 10.0 GA data-emission rules under DOS."""
import os
import struct

from neutrality import WORK, doshost, dump_json, omf, sha256_file, wcc

DECIMALS = ('1.0000000000000002', '1.2345678901234567', '0.10526315789473684',
            '0.10526315789473685', '0.10526315789473695')
STRINGS = (14, 27, 28, 29, 30, 31, 32, 40)


def program():
    code = ['double values[] = {'+', '.join(DECIMALS)+'};', 'extern void observe1(const char *, int);',
            'extern void observe2(const char *, int);']
    for size in STRINGS:
        literal = chr(65 + STRINGS.index(size))*size
        for member in (1,2):
            code.append(f'void lit{size}_{member}(void) {{ observe{member}("{literal}", {member}); }}')
    return '\n'.join(code)+'\n'


def examine(path):
    mod, = omf.load(path)
    value = next((si,off) for name,si,off,_ in mod.publics if name=='_values')
    vals = struct.unpack_from('<'+'d'*len(DECIMALS),mod.segments[value[0]].data,value[1])
    pubs = sorted((off, name, si) for name,si,off,_ in mod.publics if mod.segments[si].cls=='CODE')
    literals = {}
    for i,(off,name,si) in enumerate(pubs):
        end = pubs[i+1][0] if i+1<len(pubs) else mod.segments[si].size
        fixes = [(fx.target[1],fx.disp+struct.unpack_from('<i',mod.segments[si].data,fx.off)[0]) for fx in mod.fixups
                 if fx.seg==si and off<=fx.off<end and not fx.self_rel and fx.size()==4 and fx.target[0]&3==0
                 and mod.segments[fx.target[1]].name=='CONST']
        (seg,loc), = fixes
        literal = bytes(mod.segments[seg].data[loc:]).split(b'\0',1)[0]
        size = int(name.removeprefix('lit').split('_')[0])
        assert len(literal) == size
        literals[name.removesuffix('_')] = loc
    return {'decoded_values':vals,'equal_to_correct_rounding':[val==float(s) for val,s in zip(vals,DECIMALS)],
            'pooled_lengths':[n for n in STRINGS if literals[f'lit{n}_1']==literals[f'lit{n}_2']],
            'literal_const_offsets':literals}


def main():
    os.environ['LAMEDUKE_HOST']='dos'
    doshost.WORKER=WORK/'tmp'
    path=WORK/'fixtures'/'version.c'
    path.write_text(program(),encoding='ascii')
    out={}
    for install in ('wc95b','wc100'):
        for opt in ('-or','-ot'):
            target=path.with_name(f'{install}_{opt[1:]}.obj')
            flags=['-4r','-s',opt]
            rc, diagnostics=wcc.compile_c(path,target,flags,install)
            if rc:
                raise SystemExit(diagnostics)
            out[f'{install}/{opt}']={'flags':flags,'object_sha256':sha256_file(target),**examine(target)}
    dump_json(WORK/'version_controls.json',{'decimals':DECIMALS,'results':out})
    for label,r in out.items():
        print(label, r['equal_to_correct_rounding'], 'pools',r['pooled_lengths'])


if __name__ == '__main__':
    main()
