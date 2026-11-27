"""Audit Watcom 10.x claims against 9.5 STRICT source, objects and DOS controls."""
from collections import Counter
from pathlib import Path
import re
import struct

from idioms import open_index, statement_hit
from neutrality import ROOT, WORK, dump_json, load_json, omf, sha256_file

FILES = {
    'ray': ('D:/Prog/rayman_recon/docs/COMPILER.md', '10.6'),
    'causes': ('D:/Prog/rayman_recon/docs/CAUSES.md', '10.6'),
    'cyber': ('D:/Prog/cyberstorm_recon/docs/codegen_rules.md', '10.6'),
    'levers': ('D:/Prog/cyberstorm_recon/docs/levers.md', '10.6'),
    'lanes': ('D:/Prog/cyberstorm_recon/docs/codegen_rules_lanes.md', '10.6'),
    'kegg': ('D:/Prog/kegg_recon/docs/compiler-notes.md', '10.0'),
    'grinding': ('D:/Prog/kegg_recon/docs/grinding.md', '10.0'),
    'skill': ('C:/Users/jiriv/.claude/skills/matching-decompilation/references/compilers/watcom.md', '10.x'),
}

TABLE_RANGES = {
    'ray': [(31,48), (72,76), (379,394)],
    'cyber': [(7,88)],
    'levers': [(18,27),(32,34),(39,44),(50,57),(62,64)],
    'lanes': [(10,162)],
    'kegg': [(265,267)],
    'skill': [(53,75)],
}
BULLET_RANGES = {
    'ray': [(11,28), (94,119), (121,221), (223,370), (396,400), (405,428), (431,438), (442,455), (459,479), (495,512)],
    'kegg': [(6,248), (306,327), (331,338), (340,350)],
    'grinding': [(12,29)],
    'skill': [(34,47), (79,111)],
}
EXTRA_RANGES = {
    'ray': [(489,494),(495,498)],
    'levers': [(67,69)],
    'kegg': [(41,42),(63,63),(260,262),(269,270),(276,276),(277,277),(278,278),(279,279),(280,280),
             (281,281),(283,284),(285,286),(290,290),(291,291),(292,292),(293,293),(294,294),(295,295),
             (296,297),(298,299),(300,300),(301,301),(303,304),(325,327),(337,338),(341,344)],
    'skill': [(28,32)],
}


def in_ranges(number, ranges):
    return any(lo <= number <= hi for lo, hi in ranges)


def extract(tag, lines):
    """One record per rule-bearing row, bullet or numbered causal hypothesis."""
    rows = {}
    for number, line in enumerate(lines, 1):
        if in_ranges(number, TABLE_RANGES.get(tag, ())) and line.startswith('|'):
            rows[number] = (number, 'table')
        if in_ranges(number, BULLET_RANGES.get(tag, ())) and re.match(r'\s*(?:[-*]|\d+\.)\s+\S', line):
            end = number
            while end < len(lines) and lines[end].strip() and not re.match(r'\s*(?:[#|]|[-*]\s|\d+\.\s)', lines[end]):
                end += 1
            rows[number] = (end, 'bullet')
        if tag == 'causes' and re.match(r'^## H\d+ - ', line):
            end = number
            while end < len(lines) and not lines[end].startswith('## '):
                end += 1
            while not lines[end - 1].strip():
                end -= 1
            rows[number] = (end, 'causal hypothesis')
    for start, end in EXTRA_RANGES.get(tag, ()):
        if start <= len(lines) and lines[start-1].strip() and start not in rows:
            rows[start] = (min(end, len(lines)), 'paragraph')
    for start, (end, kind) in sorted(rows.items()):
        yield start, end, kind, '\n'.join(lines[start-1:end]).strip()


def ev(name, source=None, asm=None):
    return (name, source, asm)


# Generalizations are intentionally scoped. The source row stays verbatim in the
# audit so a qualified confirmation cannot silently endorse its version-specific details.
TESTS = {
    'register': ('CONFIRMED', 'The ordinary integer register ABI uses EAX, EDX, EBX, ECX and spills overflow arguments. Callee saves and custom pragma conventions remain context-dependent.',
                 [ev('spawn_blood', r'^EGS\(', r'call C:EGS'), ev('getangle', None, r'push ebx')], ['args/engine']),
    'proto': ('CONFIRMED', 'The caller’s visible type controls narrowing and return tests. PREMAP declares strget as int and tests EAX. The caller-specific declaration rule transfers.',
              [ev('getscorename', r'return strget', r'test eax,eax'), ev('shoot', r'^j = EGS')], ['oldarg/game_oe','narrowarg/game_oe']),
    'inlining': ('CONFIRMED', 'With -oe, ordinary eligible same-TU calls expand while standalone publics can remain. This game uses a measured threshold of at least 2500 across GAME, and the 9.5 default is 20 expression-tree nodes.',
                 [ev('timerhandler', r'^keytimerstuff\(\);$', r'keystatus'), ev('wait_for_players', r'^getpackets\(\);$')], ['thin1/game_oe','thin3/game_oe']),
    'inlinecopy': ('CONFIRMED', 'The same source can emit different instructions standalone and at different inlined sites. The fixed 10.6 register or stack recipe is a separate hypothesis.',
                   [ev('timerhandler', r'^keytimerstuff\(\);$',r'keystatus'), ev('keytimerstuff', r'angvel -= 16')], []),
    'depth': ('REFUTED', 'The recorded 10.6 three-level expansion does not transfer. 9.5 builds the tested copies only one level deep, including recursive callers. In the probe thin3 retains a call to thin1 after expanding thin2.',
              [ev('wait_for_players', r'^getpackets\(\);$'), ev('parsecommand', None, r'call C:parsecommand')], ['thin1/game_oe','thin2/game_oe','thin3/game_oe']),
    'threshold': ('REFUTED', 'The numeric -oe default and thresholds are version and source dependent. 9.5 A/B controls give a plain -oe threshold of 20 expression-tree nodes, unlike the imported 10.6 default of 100.',
                  [ev('timerhandler', r'^keytimerstuff\(\);$')], []),
    'wide16': ('CONFIRMED', '9.5 -5r promotes a nonvolatile signed short with a dword read at -2 followed by SAR 16. Under -4r, the small control emits MOVSX. The formal OMF and LE target must still resolve to the correct base and addend.',
               [ev('netinitconnection', r'i < numplayers - 1', r'sar eax,0x10')], ['widensshort/engine','widensshort/multi','widenvolatile/multi']),
    'wide8': ('UNTESTED', 'The exact memory-to-signed-long byte promotion was not located in STRICT MULTI. The 9.5 fixture confirms the -5r [char-3] plus SAR 24 idiom. Other SAR 24 instructions in MULTI extract the high byte from a true long.',
              [], ['widenschar/engine','widenschar/multi']),
    'bss': ('REFUTED', 'The transfer of the exact 25-slot, modulo-241 formula fails on 9.5. Its calibrated rule uses 22 slots and modulo 211. Size buckets and name sensitivity survive as a family resemblance.',
            [ev('keyhandler'),ev('calcwalls')], []),
    'bss-general': ('CONFIRMED', 'Declaration context and identifier hash can change 9.5 BSS layout. The 9.5 size/page/hash model is independently calibrated in docs/BSS_LAYOUT.md. The exact 10.x constants differ.',
                    [ev('keyhandler')], []),
    'pool': ('CONFIRMED', 'Ordinary C string literals relocate into CONST and can be shared. The 9.5 verified pools in GAME and GAMEDEF and two equal synthetic literal publics establish the scoped rule. An extern char-array format string is a separate case.',
             [ev('check_file_id', r'open\("FILE_ID', r"S:'FILE_ID.DIZ'"), ev('loadefs', r'open\("game.con')], ['string_a/game_oe','string_b/game_oe']),
    'pool-limit': ('REFUTED', 'The 10.x 28-byte pooling limit does not transfer to unpatched 9.5 GA. Under both -or and -ot, 9.5 shares the controlled 29- and 30-byte duplicates and splits 31- and 32-byte copies. Watcom 10.0 splits 29 onward. The controls use distinct external callees to prevent whole-function folding.',
                   [ev('check_file_id',r'open\("FILE_ID')], []),
    'residue': ('REFUTED', 'The 10.0 main-file 4-KiB buffer formula does not transfer to the 9.5 DOS profiles. The 9.5 verified GAMEDEF pool follows reused OMF record positions plus initial seed bytes. The 10.0 Apogee profile retains its own source-read residue evidence.',
                [ev('loadefs', r'open\("game.con')], []),
    'internal-initializer': ('CONFIRMED', 'The 9.5 compiler can place an initialized local aggregate in CODE ahead of the first public. animateknuckles takes the address of its knuckle_frames image at the PLAYER CODE prefix 1:1f280.',
                             [ev('animateknuckles', r'^short knuckle_frames', r'&F@1:1f280')], []),
    'data-alignment': ('REFUTED', 'Natural file-scope alignment needs the optimization context. Under 9.5 engine and multi the 9.5 fixture places a short at offset 5, an int at 7 and a double at 11. Under game_oe the same definitions start at 6, 8 and 16.',
                       [ev('keyhandler')], ['packsize/engine','packsize/game_oe']),
    'packing': ('CONFIRMED', 'Default one-byte member packing holds in 9.5. The 9.5 fixture returns sizeof=7 for char/int/short with dword at +1. STRICT map accesses independently give stride 38 for sprite and 37 for sector.',
                [ev('operatemasterswitches', r'MASTERSWITCH', r'imul\b.*0x26'), ev('moveplayers', r'^if \(sprite\[i\]\.z < sector')], ['packsize/game_oe','packoffset/game_oe']),
    'bitfields': ('UNTESTED', 'The 9.5 STRICT source views packed map flags through masks, with no qualifying equivalent bitfield pair. The byte-container fixture confirms LSB-first layout. Its high-field truth test and an explicit mask both lower to TEST byte,0xfc, so the test shape cannot identify a bitfield alone.',
                  [ev('cameratext', r'totalclock & 16', r'test m1')], ['flagsread/game_oe','flagstest/game_oe','bytemask/game_oe','flagsstore/game_oe']),
    'mask': ('CONFIRMED', 'Word and dword storage can be tested at byte width when the mask fits. The disassembly alone cannot distinguish a plain mask, a cast, or a packed bitfield.',
             [ev('cameratext', r'totalclock & 16', r'test m1'), ev('move_actor', r'^if \(flags & SLOW_TO_STOP', r'test m1')], ['bitlong/stackd2','bitarg/stackd2','bitarg/game_oe']),
    'compare': ('CONFIRMED', 'Equivalent operand order can change the first load and invert the signed comparison direction. The probe ga<gb versus gb>ga distinguishes them. No left-first rule applies to all binary expressions.',
                [ev('moveplayers', r'^if \(sprite\[i\]\.z < sector',r'cmp.*\bjge'),ev('getangle',r'xvect == yvect',r'cmp')], ['cmpab/game_oe','cmpba/game_oe','sumdeeper/game_oe']),
    'compound-canonical': ('REFUTED', '9.5 does not always canonicalize commutative square sums. In the register-profile controls x*x+y*y and y*y+x*x retain opposite IMUL order.',
                           [ev('shoot', r'ksqrt\(\(sprite', r'imul')], ['sumxy/game_oe','sumyx/game_oe']),
    'testzero': ('CONFIRMED', 'The ordinary scalar zero-comparison form selects TEST on the live register in the surveyed 9.5 C sites. The four CMP AL,0 observations are inline strcpy loops in parsecommand, the same kind of exception stated by Rayman.',
                 [ev('getangle', r'if \(xvect == 0', r'test ebx,ebx'),ev('parsecommand',r'^for \(i = 9; i > j;',r'cmp al,0x0')], ['eqbyte/game_oe','truthbyte/game_oe']),
    'd1': ('CONFIRMED', 'Fresh DOS object pairs establish byte, coverage and decoded fixup neutrality for the registered 9.5 C profiles. This covers full segments and ordinary publics. Other debug modes were tested separately.',
           [ev('keyhandler'),ev('netinitconnection'),ev('gametext')], []),
    'd2abi': ('UNTESTED', 'There is no 9.5 STRICT C unit built under -3s -d2 -s in this corpus. Under those flags the 9.5 control confirms the full stack prologue, [EBP+0x14] first argument, return spill and caller cleanup.',
              [], ['oldstack/stackd2','postinc/stackd2','packoffset/stackd2']),
    'd2push': ('UNTESTED', 'No matching -3s -d2 versus -od occurrence in the 9.5 STRICT corpus. The 9.5 control with identical source and flags otherwise confirms MOV/PUSH versus memory PUSH.',
               [], ['stringcopy/stackd2','stringcopy/stackod']),
    'narrowpush': ('UNTESTED', 'The 9.5 STRICT ordinary calls use register profiles. The 9.5 -3s -d2 and -od controls both use MOV/PUSH for short -1 and PUSH -1 for int. The short is sign-extended under the tested signed short prototype.',
                   [], ['shortarg/stackd2','intarg/stackd2','shortarg/game_oe','intarg/game_oe']),
    'd2postfix': ('UNTESTED', 'The precise -3s -d2 dead-load trigger is absent from 9.5 STRICT. The 9.5 -3s -d2 pair reproduces it. Under game_oe these postfix/prefix controls coalesce to one body, so the rule cannot be applied unconditionally to this target.',
                   [], ['postinc/stackd2','preinc/stackd2','postinc/game_oe','preinc/game_oe']),
    'postfix-opt': ('REFUTED', 'Unused postfix and prefix are not a universal source discriminator under optimized 9.5. The game_oe probe with the same parameter update aliases both spellings to one entry. The -3s -d2 probe does distinguish them.',
                    [ev('closedemowrite', r'reccnt--;', r'sub m4')], ['postinc/game_oe','preinc/game_oe','postinc/stackd2']),
    'd2loop': ('UNTESTED', 'The exact unrotated -3s -d2 pattern has no 9.5 STRICT occurrence. The controlled -3s -d2 loop shows the same split test/step/body. In 9.5 game_oe both for and while have an early guard and the same conditional back edge.',
               [], ['loopfor/stackd2','loopwhile/stackd2','loopfor/game_oe','loopwhile/game_oe']),
    'top-loop': ('CONFIRMED', 'A source loop that may run zero times has a guard before the first traversal. 9.5 may duplicate the test at the back edge, so the exact borrowed jump skeleton still depends on flags.',
                 [ev('processreservedmessage', r'while \(j < length\)', r'cmp'),ev('movetransports', r'while \(k >= 0\)')], ['loopfor/game_oe','loopwhile/game_oe','loopdo/game_oe']),
    'd2logic': ('UNTESTED', 'There is no eligible STRICT -3s -d2 unit. The 9.5 debug control reproduces the && trampoline and the nested-if direct exits. Under game_oe it emits direct exits in both forms.',
                [ev('operatemasterswitches', r'MASTERSWITCH.*&&',r'jne')], ['nested/stackd2','conjunction/stackd2','nested/game_oe','conjunction/game_oe']),
    'logical': ('CONFIRMED', 'The 9.5 optimized profiles preserve source short-circuit effects while freely inverting or threading jumps. Three field tests in operatemasterswitches use direct branches with distinct loads.',
                 [ev('operatemasterswitches', r'MASTERSWITCH.*&&',r'cmp.*\bjne')], ['conjunction/game_oe','disjunction/game_oe']),
    'd2assign': ('UNTESTED', 'The exact dereference-assignment reload does not occur in the STRICT 9.5 register-profile corpus. The -3s -d2 fixture reloads the stored value; the game_oe version tests the existing register.',
                 [ev('clipmove',r'hitwall = raytrace',r'\btest ax,ax')], ['assigncond/stackd2','ptrcond/stackd2','assigncond/game_oe']),
    'd2flat': ('UNTESTED', 'The exact 4-wide flat versus nested array pair is absent in 9.5 STRICT. The comparable -3s -d2 fixture confirms two SHL 2 versus one SHL 4. Both become SHL 4 under game_oe.',
               [], ['arrflat/stackd2','arrnested/stackd2','arrflat/game_oe','arrnested/game_oe']),
    'depth-eval': ('UNTESTED', 'No equivalent pair with only conversion-node depth changed was located in STRICT. The 9.5 multiply-before-add fixture shows that deeper work can lead, while simple memory comparisons keep source operand order.',
                   [], ['sumdeeper/game_oe','sumdepthrev/game_oe','cmpab/game_oe','cmpba/game_oe']),
    'callee-first': ('CONFIRMED', 'When all arguments call an effectful helper, the tested 9.5 register convention evaluates from the right and emits stack overflow arguments first. Staging the argument as a separate statement can move the call before these pushes.',
                     [ev('spawn_blood',r'^EGS\(',r'push 0x5.*call C:krand')], ['args/game_oe','sideeffect/game_oe']),
    'caller-pop': ('CONFIRMED', 'A variadic call pushes arguments right to left and the caller restores ESP. This is useful for validating a suspected address constant or prototype in LameDuke. The 10.x debug stack-specific instruction form does not generally apply.',
                   [ev('check_file_id',r'open\("FILE_ID',r'add esp,0xc'),ev('comlinehelp',r'printf\(help_usage',r'add esp,0x8')], ['oldstack/stackd2']),
    'const-args': ('REFUTED', 'The imported address-MOV-then-PUSH sequence is not a general 9.5 rule. STRICT variadic open pushes a relocated string address directly, and game register calls pass addresses in a parameter register.',
                   [ev('check_file_id',r'open\("FILE_ID',r'push &S:'),ev('USRHOOKS_GetMem',r'Out a memory',r'mov eax,&S:')], ['stringsvoid/stackd2']),
    'tail': ('CONFIRMED', 'A later function can enter a shared earlier epilogue. Adjust the owner and its relative placement together. This applies independently of the exact 10.6 size threshold or destructor-specific pattern.',
             [ev('setverts',None,r'jmp '),ev('setredbluepalette',None,r'jmp ')], []),
    'identical': ('CONFIRMED', '9.5 can fold complete identical procedures at one OMF offset. The 9.5 short comparison and prefix/postfix controls demonstrate it, while STRICT ENGINE witnesses cross-public tail sharing. This does not claim the exact empty-function placement rule for every TU.',
                  [ev('setverts'),ev('level_load_hook')], ['eqbyte/game_oe','truthbyte/game_oe','postinc/game_oe','preinc/game_oe']),
    'fallthrough': ('UNTESTED', '9.5 public return-tail merging is confirmed separately. The imported exact trigger is natural fall-through into an immediately following empty definition. The surveyed 9.5 corpus has no verified witness with that exact source and physical fall-through.',
                    [ev('setverts',None,r'jmp ')], []),
    'tablebefore': ('CONFIRMED', '9.5 emits switch tables before the public in optimized game_oe as well as the tested non-ot contexts. Grouping and owner confirmation are required to interpret predecessor extents.',
                    [ev('movefx',r'^switch',r'jmp cs:m4'),ev('parsecommand',r'^switch \(tw',r'jmp cs:m4')], []),
    'tableafter': ('REFUTED', 'The borrowed -ot after-function placement prediction fails for 9.5 game_oe. STRICT movefx and parsecommand carry relocated leading tables with -ot enabled through -otexan.',
                   [ev('movefx',r'^switch',r'jmp cs:m4'),ev('parsecommand',r'^switch \(tw',r'jmp cs:m4')], []),
    'tree': ('CONFIRMED', 'Compare trees retain case ranges when contiguous labels share a target. A narrow selector can stay in AX or AL. The source labels and signedness control more than the count alone.',
             [ev('animatewalls',r'^switch \(wall\[i\]\.picnum',r'cmp ax.*\bjb\b.*\bjbe\b'),ev('getpackets',r'^switch \(tempbuf')], []),
    'dense4': ('CONFIRMED', 'For the controlled shape with distinct arms, four contiguous cases produce a table and three produce comparisons. The borrowed universal density cutoff fails in other 9.5 fixtures, including twelve stride-two cases.',
               [ev('movefx',r'^switch',r'jmp cs:m4'),ev('addweapon',r'^switch',r'cmp dx')], []),
    'scan-switch': ('CONFIRMED', 'The scan-table mechanism transfers to controlled 9.5 C. The engine and multi fixtures use REPNE SCASW followed by a CS-indexed jump for 22 or 24 short cases separated by 27. The same inputs under game_oe choose compare trees. No SCASW dispatcher occurs in the 210 STRICT 9.5 procedures, and case count alone cannot predict the form.',
                    [], ['sw_22_27/engine','sw_24_27/multi','sw_22_27/game_oe','sw_24_3/game_oe']),
    'alignment': ('REFUTED', 'The 10.6 aligned-entry claim does not transfer as stated. 9.5 -ot -od with -3s emits dword-aligned entries; game_oe pads to paragraphs, but a 9.5 -5r -otexan control uses dword-aligned entries while keeping paragraph-aligned _TEXT metadata.',
                  [ev('gametext'),ev('nethandler')], ['eqglobal/game_oe','eqglobal/g5','truthglobal/stackotod']),
    'align4': ('CONFIRMED', 'The dword-aligned entry pattern transfers to the 9.5 -3s -ot -od fixture. The 9.5 engine has byte-aligned publics and game_oe supplies a different alignment, so this remains a flag-specific relationship.',
               [ev('keyhandler'),ev('gametext')], ['truthglobal/stackotod']),
    'ledatasize': ('CONFIRMED', 'The 172-byte minimum chunk mechanism occurs in 9.5. A 9.5 CODE census gives both 172-byte and larger near-threshold chunks; short chunks also exist. This confirms the ordinary size boundary without asserting the exact 192-byte import accounting.',
                   [ev('keyhandler'),ev('parsecommand')], ['loopfor/stackd2']),
    'ledatamath': ('UNTESTED', 'The index contains final instruction ranges, not first-reference import buffer state for every debug flush. The live objects give a 172-byte chunk threshold witness. The precise 192-byte cut rule and per-page WLINK ordering need a dedicated OMF/link probe.',
                   [], []),
    'intrinsic': ('CONFIRMED', 'Using the Watcom headers under game_oe emits the string-copy template and retains memset as a call in the test. STRICT newscore uses the byte-tail copy. The engine/MULTI convention is different, so an intrinsic prediction needs the actual profile.',
                  [ev('newscore',r'\bmemcpy\(',r'repne movsb'),ev('simulateint',r'\bmemset\(',r'call C:memset')], ['stringcopy/game_oe','stringcopy4/game_oe','stringmemset/game_oe','stringcopy/engine']),
    'pragmas': ('CONFIRMED', 'The general pragma ABI technique transfers where documented original helpers supply the register or string-instruction form. The specific 10.6 Rayman helpers do not establish shared lineage.',
                [ev('keyhandler',r'^readkey\(',r'\bin al'),ev('movelava',r'^offs2 \+=',r'rep movsd')], []),
    'ports': ('REFUTED', 'The specific through-DX claim for all immediate I/O does not transfer. STRICT keyhandler uses IN AL,0x60 and OUT 0x61,AL through the known Ken pragma. 9.5 has both immediate and DX forms, depending on the actual C/pragma.',
              [ev('keyhandler',r'^readkey\(',r'in al,0x60')], []),
    'fp': ('UNTESTED', 'No floating arithmetic appeared in the STRICT 9.5 indexed units. The 9.5 -fpc control calls __FDD/__FDA while the default emits FLD/FDIV/FADD. No inference about the executable’s complete floating-point profile follows.',
           [], ['some_fp/soft','some_fp/game_oe']),
    'fpround': ('REFUTED', 'The exact 10.x 16-significant-digit grid fails for unpatched 9.5 GA. The same DOS source gives 0.10526315789473684 in 9.5 and 0.1052631578947368 in 10.0 under both -or and -ot. 9.5 is not always correctly rounded either. The corpus has no matching long-decimal literal; this is a controlled compiler comparison.',
                [], ['some_fp/game_oe']),
    'savings': ('CONFIRMED', 'The 9.5 tracer and STRICT cacheit control establish savings-sensitive placement. The 10.6 numeric loop weighting and exact register lists remain separate, unmeasured transfers.',
                [ev('cacheit'),ev('movetransports')], []),
    'allocator': ('CONFIRMED', '9.5 also uses an unstable gap sorter for descending conflict savings, established by 47 replayed orders in docs/CG95.md. The exact 10.6 prior-GivenRegisters policy is not inferred from machine code.',
                  [ev('calcwalls'),ev('hline'),ev('cacheit')], []),
    'homes': ('CONFIRMED', '9.5 home priority depends on flag, size and the sort input, with alias offsets for the same temp. Matched calcwalls and hline changed homes with declaration order. No fixed declaration-only or ascending-size-only layout follows.',
              [ev('hline'),ev('calcwalls'),ev('moveweapons',r'\(long \*\)&distance')], []),
    'byte-rover': ('UNTESTED', 'The corpus contains many AH/AL allocations but cannot expose LdStAlloc visit order or prove the same first eligible byte. The 9.5 tracer scopes savings ties; it does not expose this 10.6 rover sequence.',
                   [ev('parsecommand',r'while \(!isaltok\(',r'mov ah,al')], []),
    'score-ring': ('UNTESTED', 'An emitted MOV and TEST alone cannot identify the compiler’s equivalence ring. The exact 10.6 Score pass, hard-coded ring tie rule and BP exception need instrumentation in 9.5.',
                   [], []),
    'sched': ('UNTESTED', 'STRICT source anchors show movement across independent statements, but do not give post-allocation DAG edges, stall costs or FU tables. 9.5 -5r demonstrably changes selected instructions and alignment in controls; it does not identify the borrowed scheduler algorithm.',
              [ev('calcwalls',r'^hplc = hplc2;',r'0x8000000')], ['loopfor/game_oe','loopfor/g5']),
    'release': ('REFUTED', 'A generally interchangeable 10.x code generator or binary recipe does not transfer to this target. The accepted 9.5 unpatched engine control gave 88/88 historical object functions; comparative 10.x installs gave 11–13. This does not dispute equality within the other project’s selected TUs.',
                [ev('keyhandler'),ev('drawrooms')], []),
    'path': ('CONFIRMED', 'Keep historical compile staging short. On this repository’s 9.5 NT build the compiler can hang on long stage components, and CONST residue depends on the host. Canonical DOSBox-X removes the cited path sensitivity for the checked 9.5 pools.',
             [ev('loadefs'),ev('timerhandler')], []),
    'dostools': ('CONFIRMED', 'Pinned MS-DOS Player fails silently for the 9.5 extended C compiler on the recorded controls. The DOSBox-X compile succeeds while MASM continues to use MS-DOS Player. This is a host compatibility result.',
                 [ev('keyhandler'),ev('gametext')], []),
    'runtime': ('CONFIRMED', 'Release-unique runtime members discriminate independently of game object code. LameDuke’s 125 CLIB/MATH contributions and pre-patch members identify Watcom 9.5 GA. Apogee’s C profile is kept separate.',
                [ev('getangle'),ev('keyhandler')], []),
    'layout': ('CONFIRMED', 'Leading tables can explain a predecessor extent that looks wrong. 9.5 confirms the real owner by its fixup targets and bound public, rather than accepting a normalized match as proof.',
               [ev('draw_water_distortion'),ev('operateforcefields'),ev('parsecommand')], []),
    'stores': ('CONFIRMED', 'Store and scheduling order can affect exact bytes after values and addresses are established. Separating the three getinput stores or reordering equal stores in resetplayer is verified. Exact order for the imported unrelated structures remains untested.',
               [ev('getinput',r'^syncvel\[snum\] = 0;'),ev('resetplayer')], []),
    'scope': ('CONFIRMED', 'Temporary or local home changes can shift other allocation choices. STRICT cacheit, hline and movetransports establish the scoped family. The imported specific spelling and result register still need a local A/B pair.',
              [ev('cacheit'),ev('hline'),ev('movetransports')], []),
}


# Each line points to a named, evaluated transfer test. Other rows are still
# inventoried, with explicit source searches and a reason for leaving them open.
DIRECT = {
    'ray': {
        11:'layout', 15:'proto', 31:'proto', 32:'mask', 34:'compare', 37:'stores', 38:'scope', 39:'homes',
        40:'bitfields', 41:'testzero', 42:'top-loop', 43:'tree', 45:'intrinsic', 46:'register', 47:'pragmas',
        48:'pool', 121:'register',123:'runtime',129:'wide16',134:'alignment',136:'packing',
        138:'data-alignment',141:'bitfields',143:'wide16',146:'bss',149:'intrinsic',154:'internal-initializer',
        157:'inlining',159:'release',162:'fp',167:'fallthrough',171:'pragmas',176:'tree',186:'bitfields',
        197:'postfix-opt',201:'score-ring',207:'savings',213:'homes',238:'proto',240:'proto',245:'bitfields',
        248:'scope',249:'compare',256:'fallthrough',260:'pool',264:'identical',287:'score-ring',295:'homes',
        301:'layout',308:'tree',318:'ports',325:'intrinsic',334:'byte-rover',354:'pool-limit',359:'homes',
        363:'allocator',364:'allocator',367:'byte-rover',405:'score-ring',408:'score-ring',410:'score-ring',
        415:'byte-rover',419:'byte-rover',431:'testzero',435:'testzero',442:'fpround',448:'fpround',454:'fpround',
        459:'sched',462:'sched',468:'sched',472:'sched',495:'identical',
    },
    'causes': {41:'allocator',81:'byte-rover',112:'sched',152:'layout'},
    'cyber': {
        7:'alignment',8:'alignment',10:'stores',15:'d1',18:'top-loop',22:'testzero',26:'inlining',27:'scan-switch',
        30:'tail',31:'homes',41:'tableafter',45:'fallthrough',48:'compound-canonical',51:'release',52:'threshold',
        55:'tail',68:'homes',72:'alignment',76:'inlinecopy',77:'depth',79:'tail',
    },
    'levers': {18:'inlining',19:'inlining',21:'logical',22:'tail',23:'layout',25:'proto',32:'tail',33:'tail',
               34:'layout',39:'compare',44:'compare',50:'scope',51:'scope',54:'stores',56:'homes',64:'layout'},
    'lanes': {11:'scope',13:'inlining',17:'tail',19:'tree',25:'compare',27:'inlinecopy',28:'savings',29:'tree',
              30:'depth',61:'inlinecopy',69:'logical',71:'bitfields',72:'bitfields',77:'homes',78:'inlinecopy',
              87:'inlinecopy',89:'top-loop',95:'fallthrough',101:'inlinecopy',108:'top-loop',
              116:'callee-first',117:'homes',126:'bitfields',128:'tree',129:'depth-eval',130:'depth',131:'threshold',
              135:'intrinsic',137:'proto',138:'tree',150:'inlinecopy'},
    'kegg': {
        7:'d2abi',9:'d2abi',12:'d2abi',14:'d2abi',16:'d2abi',20:'d2abi',24:'narrowpush',30:'d2loop',
        36:'align4',41:'d2push',43:'d2push',46:'d2postfix',48:'d2push',53:'d2postfix',63:'d2abi',64:'d2postfix',
        66:'d2postfix',68:'d2postfix',70:'d2assign',74:'depth-eval',81:'d2flat',83:'d2logic',85:'d2abi',87:'d2abi',
        90:'mask',92:'bitfields',97:'callee-first',99:'packing',101:'const-args',109:'homes',115:'depth-eval',
        131:'d2assign',132:'d2push',140:'proto',144:'bitfields',153:'ledatasize',157:'ledatamath',164:'ledatamath',
        167:'ledatamath',172:'ledatamath',176:'ledatamath',185:'bss',192:'bss',202:'bss-general',206:'ledatamath',
        218:'residue',226:'residue',232:'residue',236:'path',244:'ledatasize',246:'ledatamath',248:'ledatamath',
        260:'d2logic',269:'d2logic',276:'d2logic',277:'d2logic',278:'d2logic',279:'d2logic',280:'d2logic',281:'d2logic',
        283:'d2logic',285:'d2logic',290:'d2logic',291:'d2logic',292:'d2logic',293:'d2logic',294:'d2loop',295:'d2loop',
        296:'d2loop',297:'d2loop',298:'d2loop',300:'d2abi',303:'d2loop',308:'d2loop',310:'tree',315:'dense4',318:'tree',
        325:'homes',341:'residue',345:'residue',346:'residue',348:'residue',
    },
    'grinding': {17:'proto',21:'alignment'},
    'skill': {
        36:'runtime',40:'release',43:'register',53:'d2abi',54:'register',55:'d2push',56:'narrowpush',57:'d2postfix',
        58:'depth-eval',59:'wide16',60:'ledatamath',61:'bss',62:'residue',63:'inlining',64:'intrinsic',66:'wide8',
        67:'data-alignment',68:'intrinsic',69:'fp',70:'proto',71:'compare',72:'identical',73:'pool',74:'d1',75:'pragmas',
        102:'path',109:'dostools',
    },
}


def witness(fns, spec):
    name, source, asm = spec
    found = []
    for f in fns:
        if f['function'] != name:
            continue
        if source:
            for s in f['statements']:
                if not s['instructions'] or not re.search(source, s['text']):
                    continue
                h = statement_hit(f, s)
                if asm and not re.search(asm, '; '.join(h['symbolic'])):
                    continue
                found.append({k:h[k] for k in ('unit','function','source','profile','address','line','text','symbolic')})
        else:
            ins = [i for i in f['instructions'] if i['symbolic'] and (not asm or re.search(asm,i['symbolic']))]
            if not ins:
                continue
            hit = ins[0]
            found.append({k:f[k] for k in ('unit','function','source','profile')} | {
                'address': f"1:{f['extent_start']+hit['offset']:05x}", 'line': hit['line'],
                'symbolic': [i['symbolic'] for i in (ins[:6] if asm else [])]})
    if not found:
        raise ValueError(f'no fresh 9.5 STRICT witness for {spec!r}')
    return found[0]


def control(probe_ref, dos, older):
    fn, profile = probe_ref.split('/')
    build = dos[profile] if fn in dos.get(profile, {}).get('functions', {}) else older[profile]
    f = build['functions'][fn]
    return {'function':fn, 'profile':profile, 'flags':build['flags'], 'segment_offset':f['entry'],
            'object_sha256':build['sha256'] if 'sha256' in build else build['object_sha256'],
            'instructions':f['instructions'][:40]}


def verify_controls(dos, older, versions, fns):
    ins = lambda profile, fn: '\n'.join(dos[profile]['functions'][fn]['instructions'])
    pub = lambda profile, fn: dos[profile]['functions'][fn]['entry']
    assert pub('game_oe','postinc') == pub('game_oe','preinc')
    assert 'mov eax, dword ptr [ebp + 0x14]\ninc' in ins('stackd2','postinc')
    assert 'push dword ptr [ebp + 0x14]' in ins('stackod','stringcopy')
    assert 'mov eax, dword ptr [ebp + 0x14]\npush eax' in ins('stackd2','stringcopy')
    assert 'jmp' in ins('stackd2','conjunction') and 'jmp' not in ins('game_oe','conjunction')
    assert ins('stackd2','arrflat').count('shl eax, 2') == 2
    assert ins('stackd2','arrnested').count('shl eax, 4') == 1
    assert all('shl eax, 4' in ins('game_oe', fn) for fn in ('arrflat','arrnested'))
    assert 'imul eax, eax\nimul edx, edx' in ins('game_oe','sumxy')
    assert 'imul edx, edx\nimul eax, eax' in ins('game_oe','sumyx')
    assert 'call' in ins('game_oe','thin3') and '<thin1_:' in ins('game_oe','thin3')
    assert 'repne movsb' in ins('game_oe','stringcopy') and '<memset_:' in ins('game_oe','stringmemset')
    assert 'test byte ptr [eax], 0xfc' in ins('game_oe','flagstest') and 'test byte ptr [eax], 0xfc' in ins('game_oe','bytemask')
    assert '<__FDD:' in ins('soft','some_fp') and 'fdiv' in ins('game_oe','some_fp')
    assert {f['entry'] % 16 for f in dos['game_oe']['functions'].values()} == {0}
    assert {f['entry'] % 4 for f in dos['g5']['functions'].values()} == {0} and pub('g5','eqglobal') % 16 != 0
    assert dos['g5']['code_segments'] == dos['game_oe']['code_segments']
    assert all((i['symbolic'] or '').find('scasw') == -1 for f in fns for i in f['instructions'])
    assert all('repne scasw' in ins(profile,f'sw_{count}_27') for profile in ('engine','multi') for count in (22,24))
    assert not any('scasw' in ins('game_oe',fn) for fn in dos['game_oe']['functions'] if fn.startswith('sw_'))
    assert any(r['style']=='comparisons' and r['function']=='sw_12_2' for r in older['game_oe']['case_matrix'])
    vals = next(off for n,seg,off,_ in dos['game_oe']['publics'] if n=='_vals' and seg=='_DATA')
    data = bytes.fromhex(dos['game_oe']['data']['_DATA'])
    assert struct.unpack_from('<dd',data,vals) == (1.0000000000000002,1.2345678901234567)
    for opt in ('-or','-ot'):
        a, b = (versions['results'][f'{cc}/{opt}'] for cc in ('wc95b','wc100'))
        assert a['pooled_lengths'] == [14,27,28,29,30] and b['pooled_lengths'] == [14,27,28]
        assert a['decoded_values'][2] == float(versions['decimals'][2]) and b['decoded_values'][2] == 0.1052631578947368
    assert not any(re.search(r'^f(?:ild|ld|div|stp)|call C:__FD',i['symbolic'] or '') for f in fns for i in f['instructions'])


FEATURES = (
    (r'default argument|constructor|destructor|\bCOMDAT\b|\bC\+\+\b|\bW\?\$|\bthis\b|vtable|virtual|class |__imp|dllimport|Win32',
     r'\b(?:class|virtual|__declspec|__stdcall)\b|operator\s*[!=+]',
     'The exact C++/Win32 or object-lifetime trigger is absent from the indexed 9.5 DOS C translation units.'),
    (r'debug|\-od|\-d2|\-3s|\bEBP\b|\[ebp', r'\b(?:short|char|long|int)\s+\*?\w+\s*[;=]',
     'The external stack/debug/front-end context has no matching STRICT control; local declarations alone do not identify it.'),
    (r'inline|accessor|helper|expand', r'\b\w+\([^;\n]*\)',
     'The named source form and its A/B equivalent or backend state were not established in one STRICT 9.5 caller.'),
    (r'byte|bitfield|char|\bmask\b', r'\b(?:unsigned|signed)?\s*char\b|\&\s*(?:0x[0-9a-f]+|\d+)',
     'Byte-sized analogues exist, but the stated paired widths, view or allocation precondition has no verified 9.5 occurrence.'),
    (r'switch|case|jump|branch|break|return|scope|loop|for\s*\(|while\s*\(',
     r'\b(?:if|else|switch|case|return|break|for|while)\b',
     'The exact CFG/source-shape pair and its changed emitted control flow were not established by the 9.5 corpus.'),
    (r'allocat|spill|register|stack|home|temporar|compiler|\bTU\b|schedule|\bIR\b', r'\b(?:char|short|int|long)\s+\*?\w+',
     'The internal pass, exact flag comparison, or paired source intervention is outside what the existing 9.5 instruction/line index observes.'),
    (r'extern|reloc|oracle|binding|object|WLINK|assembler', r'\b(?:extern|static|const)\b',
     'The imported linker, library, assembler or build rule has no equivalent STRICT C trigger in the index.'),
)


def unmatched_test(text, fns, cache):
    for trigger, pattern, reason in FEATURES:
        if re.search(trigger, text, re.I):
            if pattern not in cache:
                cache[pattern] = sum(bool(re.search(pattern, s['text'])) for f in fns for s in f['statements'])
            return pattern, cache[pattern], reason
    identifiers = set(re.findall(r'`([A-Za-z_]\w{3,})\b', text)) - {'void','char','short','long','unsigned','signed','return','else'}
    matched = set(re.findall(r'\b(?:'+'|'.join(sorted(identifiers))+r')\b', '\n'.join(f['function'] for f in fns))) if identifiers else set()
    return sorted(identifiers), len(matched), 'The imported row is project-specific or lacks an isolatable source and emitted-code pair in the 9.5 index.'


def quoted_sources(text, normalized_source):
    """Report literal source snippets separately from the broad analogue search."""
    clauses = [s.replace(r'\|', '|') for s in re.findall(r'`([^`\n]+)`', text)]
    snippets = sorted({s for s in clauses if len(s) >= 8 and re.search(r'[=(\[{;]|\+\+|--', s)})
    return [{'fragment':s,'exact_source_hits':normalized_source.count(re.sub(r'\s+','',s))} for s in snippets[:16]]


def title(text, kind):
    if kind == 'table':
        return ' · '.join(part.strip().replace(r'\|','|') for part in re.split(r'(?<!\\)\s+\|\s+',text[1:])[:2])[:132]
    return re.sub(r'^\s*(?:##|[-*]|\d+\.)\s+', '', text.splitlines()[0]).replace('**','')[:132]


PINS = {
    'ray': '17f17fc4e7135c89fff14fcc8e4adf3dd2f19432fc8428fbf1dfe7b993e18b05',
    'causes': 'b5bbb7b4030f25f3f29ffbe4f4ca0ff033be05c2fc3bd9c4ff656056355dafc8',
    'cyber': '5caf286d1e3b68a35029804aa266d3d99530a0ab59a0eeb7a485c32f031b6a52',
    'levers': '32549c7ba8efcb2761b55f4ec8faeea23a6afc7d1d12c034e91391e3a343ed63',
    'lanes': 'dbac8a39ad6f31ca33d20e7f8598d4c85bfb31d1db641844de149e42396c791c',
    'kegg': '3032cdc71fa6f503ec03ffa4be11202833b25ef72749f161a9b2d008b287bea1',
    'grinding': 'ee0ed7dc095972165d6634756b01eeaa31b15cf594fae984c68b3efbd31ea25b',
    'skill': '5f0b878c08f83b84a1cd89e76f6015678f30a2b28c54ef4fc766f03f9d0a7a71',
}


def render_md(path, imports, definitions, logs):
    marker = '<!-- cross-version audit -->'
    text = path.read_text(encoding='utf-8').split(marker, 1)[0].rstrip() + '\n\n'
    totals = Counter(row['applies_to_9.5'] for row in imports)
    lines = [marker,'## Rules imported from Watcom 10.x','',
             f"The audit catalogs {len(imports)} source rows across eight documents. "
             f"{totals['CONFIRMED']} have a scoped confirmation, {totals['REFUTED']} have a counterexample, "
             f"and {totals['UNTESTED']} remain open as transfers to the matched corpus.", '',
             'Each row in [quirks.json](quirks.json) retains its source range, full imported text, 9.5 scope, '
             'and the matching examples or reason it remains open. Confirmations apply only to the claim shown below. '
             'A matching C analogue cannot establish a C++ compiler rule, exact allocator internals, or an A/B difference. '
             'The 9.5 examples exclude Apogee’s Watcom 10.0 units.', '',
             '### Confirmed transfers', '']
    for state in ('CONFIRMED','REFUTED','UNTESTED'):
        if state == 'REFUTED':
            lines += ['### Refuted transfers', '', 'These rows have a contrary 9.5 example or a controlled 9.5 counterexample. '
                      'Their original 10.x scope remains attached in the JSON.', '']
        elif state == 'UNTESTED':
            lines += ['### Untested transfers with targeted evidence', '',
                      'The rows below were checked with an explicit control or corpus selector. Remaining rows have an open '
                      'result and search context in the JSON. The standalone compiler controls carry no STRICT status.', '']
        for ident, d in definitions.items():
            if d['applies_to_9.5'] != state:
                continue
            readers = [row for row in imports if row.get('test') == ident]
            if not readers:
                continue
            lines += [f"#### {ident.replace('-', ' ').capitalize()}", '',d['differences_between_versions'],'']
            if d['examples']:
                locations = ', '.join(f"`{e['function']}` `{e['address']}` ({e['profile']})" for e in d['examples'])
                lines.append(f'9.5 STRICT examples include {locations}.')
            if d['probes']:
                lines.append('DOS controls include '+', '.join(f"`{p['function']}` under `{p['profile']}`" for p in d['probes'])+'.')
            sources = ', '.join(f"`{r['document']}:{r['imported_from']['line']}`" for r in readers[:9])
            suffix = f' and {len(readers)-9} additional rows' if len(readers)>9 else ''
            lines += [f'Imported rows include {sources}{suffix}.','']
    lines += ['### Reproduction', '',
              'Run `python -B tools/idioms/import_probes.py` and '
              '`python -B tools/idioms/version_controls.py` followed by '
              '`python -B tools/idioms/import_audit.py`. Source documents are read-only and pinned by hash '
              'so a line-specific judgment cannot silently attach to another claim.', '',
              f"The index contributes {logs['strict_95_functions']} STRICT 9.5 functions. The OMF chunk census observed "
              f"{logs['code_chunk_lengths'].get(172,0)} 172-byte records across the registered 9.5 units. "
              'See the [9.5 probe output](../import_probes.json) and [9.5 versus 10.0 controls](../version_controls.json).', '']
    path.write_text(text+'\n'.join(lines),encoding='utf-8')


def main():
    index = open_index()
    fns = [f for f in index['functions'] if f['profile'] in ('engine','game_oe','multi') and f['kind']=='function']
    dos = load_json(WORK/'import_probes.json')
    older = load_json(WORK/'probes.json')
    versions = load_json(WORK/'version_controls.json')
    verify_controls(dos, older, versions, fns)
    evidence = {}
    for name, (status, difference, examples, refs) in TESTS.items():
        evidence[name] = {'applies_to_9.5':status, 'differences_between_versions':difference,
                          'examples':[witness(fns,e) for e in examples], 'probes':[control(r,dos,older) for r in refs]}
        if name in ('pool-limit','fpround'):
            evidence[name]['control_artifact'] = 'version_controls.json'
    imports, cache, documents = [], {}, []
    normalized_source = '\0'.join(re.sub(r'\s+','',s['text']) for f in fns for s in f['statements'])
    for tag,(raw,version) in FILES.items():
        path = Path(raw)
        digest = sha256_file(path)
        if digest != PINS[tag]:
            raise ValueError(f'{path} changed; recheck line-specific judgments')
        chunks = list(extract(tag,path.read_text(encoding='utf-8').splitlines()))
        documents.append({'path':raw, 'sha256':digest, 'version':version, 'imported_rules':len(chunks)})
        for start,end,kind,text in chunks:
            identifier = DIRECT.get(tag,{}).get(start)
            r = {'id':f'import-{tag}-{start}', 'class':'HYPOTHESIS', 'document':f'{tag}/{path.name}',
                 'imported_from':{'path':raw,'line':start,'end_line':end,'watcom_version':version},
                 'rule':title(text,kind),'original_claim':text,'quoted_source_search':quoted_sources(text,normalized_source)}
            if identifier:
                r['test'] = identifier
                r.update(evidence[identifier])
            else:
                selector,candidates,why = unmatched_test(text,fns,cache)
                r.update({'applies_to_9.5':'UNTESTED','differences_between_versions':why,
                          'examples':[],'probes':[], 'corpus_search':{'selector':selector,'related_hits':candidates,
                                                                      'qualifying_matches':0}})
            if r['examples']:
                r['strict_corpus_basis'] = ('Related STRICT examples' if r['applies_to_9.5']=='UNTESTED' else
                                            'STRICT examples and scoped evidence in the verdict')
            else:
                r['strict_corpus_basis'] = 'No qualifying source/code witness in the 9.5 index'
            imports.append(r)
    lengths = Counter()
    for unit in sorted({f['unit'] for f in fns}):
        mod, = omf.load(WORK/'objects'/f'{unit}.lines.obj')
        code = {i for i,s in enumerate(mod.segments) if s and s.cls=='CODE'}
        lengths.update(n for i,_,n in mod.chunks if i in code)
    assert lengths[172] and lengths[173] and lengths[174]
    logs = {'strict_95_functions':len(fns), 'code_chunk_lengths':dict(sorted(lengths.items())), 'source_documents':documents}
    dest = ROOT/'docs'/'quirks.json'
    editorial = load_json(dest)
    editorial['imported_rules'] = imports
    editorial['import_audit'] = logs
    dump_json(dest,editorial)
    render_md(dest.with_name('WATCOM95_QUIRKS.md'),imports,evidence,logs)
    print(len(fns),'STRICT 9.5 functions;',len(imports),'imported rows;',dict(Counter(r['applies_to_9.5'] for r in imports)))


if __name__ == '__main__':
    main()
