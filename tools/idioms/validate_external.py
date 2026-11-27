"""Regression checks for the proposed CLI and external normalization."""
import json
import os
import subprocess
import sys

from build_external import ROOT, WORK
from external_corpus import loose_key

PROPOSED = ROOT / 'tools/idioms/idioms.py'
CANONICAL = ROOT / 'tools/idioms/idioms.py'


def cli(tool, *args):
    result = subprocess.run([sys.executable, '-B', str(tool), *args, '--json'], cwd=ROOT,
                            env=dict(os.environ, PYTHONIOENCODING='utf-8'), capture_output=True,
                            text=True, encoding='utf-8', timeout=90)
    if result.returncode:
        raise ValueError(result.stderr)
    return json.loads(result.stdout)


def main():
    # Address structure and scales, immediates, and known call identity survive.
    a = ('mov R,m4[D:sprite+R+R*2]', 'mov R,m2[R-0x1c]', 'mov R,&D:sprite',
         'mov R,m4[esp+S]', 'cmp R,0x80', 'call C:updatesector', 'jL', 'jmp cs:m4[D:table+R*4]')
    b = ('mov R,m4[D@2:04d00+R+R*2]', 'mov R,m2[R+0x28]', 'mov R,&D@2:abcdef',
         'mov R,m4[esp+S]', 'cmp R,0x80', 'call C:updatesector', 'jL', 'jmp cs:m4[D:table+R*4]')
    assert loose_key(a) == loose_key(b)
    expected = ('mov R,m4[G+R+R*2]', 'mov R,m2[R+F]', 'mov R,&G', *a[3:])
    assert loose_key(a) == expected
    assert loose_key(('mov R,m4[S:"a]b"+R*4]',)) == ('mov R,m4[G+R*4]',)
    for changed in ('cmp R,0x81', 'jE', 'call C:rotatesprite', 'jmp cs:m4[D:table+R*2]'):
        assert loose_key((changed,))[0] not in loose_key(a)
    assert loose_key(('call C:someone', 'call F@1:00123')) == ('call C:?', 'call C:?')

    for invocation in (('query', '--asm', 'mov ax, word ptr [totalclock]; sub ax, word ptr [lockclock]'),
                       ('c', '--pattern', '^reccnt--;'),
                       ('near', 'game_actors', 'moveexplosions', '--block', '1:2993a'),
                       ('reverse', 'game_game', 'gamenumber')):
        assert cli(CANONICAL, *invocation) == cli(PROPOSED, *invocation), invocation
        print('Unchanged:', ' '.join(invocation), flush=True)
    print('Normalization checks passed.')


if __name__ == '__main__':
    main()
