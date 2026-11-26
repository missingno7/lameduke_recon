; "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
; Ken Silverman's official web site: "http://www.advsys.net/ken"
; See the included license file "BUILDLIC.TXT" for license info.
; December 1994 reconstruction. See NOTES.md for source provenance.

.386P

EXTRN _asm1 : dword
EXTRN _asm2 : dword
EXTRN _asm3 : dword
EXTRN _asm4 : dword

CODE SEGMENT BYTE PUBLIC USE32 'CODE'
ASSUME cs:CODE

PUBLIC sethlinesizes_
sethlinesizes_:
	neg al
	add al, 32
	mov byte ptr ds:[hxsiz7+2], al
	mov byte ptr ds:[hxsiz5+2], al
	mov byte ptr ds:[hxsiz6+2], al
	mov byte ptr ds:[hxsiz1+2], al
	mov byte ptr ds:[hxsiz2+2], al
	mov byte ptr ds:[hxsiz3+2], al
	mov byte ptr ds:[hxsiz4+2], al
	mov al, bl
	mov byte ptr ds:[hysiz7+3], al
	mov byte ptr ds:[hysiz5+3], al
	mov byte ptr ds:[hysiz6+3], al
	mov byte ptr ds:[hysiz1+3], al
	mov byte ptr ds:[hysiz2+3], al
	mov byte ptr ds:[hysiz3+3], al
	mov byte ptr ds:[hysiz4+3], al
	mov dword ptr ds:[hoffs1+2], ecx
	mov dword ptr ds:[hoffs2+2], ecx
	mov dword ptr ds:[hoffs3+2], ecx
	mov dword ptr ds:[hoffs4+2], ecx
	mov dword ptr ds:[hoffs5+2], ecx
	mov dword ptr ds:[hoffs6+2], ecx
	mov dword ptr ds:[hoffs7+2], ecx
	ret

PUBLIC sethlinechainsizes_
sethlinechainsizes_:
	neg al
	add al, 32
	mov byte ptr ds:[hxsiz14+2], al
	mov byte ptr ds:[hxsiz12+2], al
	mov byte ptr ds:[hxsiz13+2], al
	mov byte ptr ds:[hxsiz8+2], al
	mov byte ptr ds:[hxsiz9+2], al
	mov byte ptr ds:[hxsiz10+2], al
	mov byte ptr ds:[hxsiz11+2], al
	mov al, bl
	mov byte ptr ds:[hysiz14+3], al
	mov byte ptr ds:[hysiz12+3], al
	mov byte ptr ds:[hysiz13+3], al
	mov byte ptr ds:[hysiz8+3], al
	mov byte ptr ds:[hysiz9+3], al
	mov byte ptr ds:[hysiz10+3], al
	mov byte ptr ds:[hysiz11+3], al
	mov dword ptr ds:[hchainasm64_load1+2], ecx
	mov dword ptr ds:[hchainasm64_load3+2], ecx
	mov dword ptr ds:[hchainasm64_load5+2], ecx
	mov dword ptr ds:[hchainasm64_load7+2], ecx
	mov dword ptr ds:[hchainasm64_load9+2], ecx
	mov dword ptr ds:[hchainasm64_load11+2], ecx
	mov dword ptr ds:[hchainasm64_load13+2], ecx
	ret

PUBLIC setpalookupaddress_
setpalookupaddress_:
	mov dword ptr ds:[pal1+2], eax
	mov dword ptr ds:[pal2+2], eax
	mov dword ptr ds:[pal3+2], eax
	mov dword ptr ds:[pal4+2], eax
	mov dword ptr ds:[pal5+2], eax
	mov dword ptr ds:[pal6+2], eax
	mov dword ptr ds:[pal7+2], eax
	ret

PUBLIC chainsetpalookupaddress_
chainsetpalookupaddress_:
	mov dword ptr ds:[hchainasm64_load2+2], eax
	mov dword ptr ds:[hchainasm64_load4+2], eax
	mov dword ptr ds:[hchainasm64_load6+2], eax
	mov dword ptr ds:[hchainasm64_load8+2], eax
	mov dword ptr ds:[hchainasm64_load10+2], eax
	mov dword ptr ds:[hchainasm64_load12+2], eax
	mov dword ptr ds:[hchainasm64_load14+2], eax
	ret

PUBLIC hlineasm64_
hlineasm64_:
	push ebp
	mov eax, _asm1
	mov dword ptr ds:[mach1a+2], eax
	mov dword ptr ds:[mach1b+2], eax
	mov dword ptr ds:[mach1c+2], eax
	mov dword ptr ds:[mach1d+2], eax
	and edx, 0ffffff00h              ;clear dl permanently for counter
	and _asm2, 0ffffff00h            ;make sure increment doesn't change dl
	mov eax, _asm2
	mov dword ptr ds:[mach2a+2], eax
	mov dword ptr ds:[mach2b+2], eax
	mov dword ptr ds:[mach2c+2], eax
	mov dword ptr ds:[mach2d+2], eax
	xchg cl, ch

	shr _asm3, 1
	jnc short skipthe1byte
	mov eax, esi
hxsiz7: shr eax, 26
hysiz7: shld eax, edx, 6
hoffs1: mov cl, byte ptr [eax+88888888h]
pal1: mov bl, byte ptr [ecx+88888888h]
	add esi, _asm1
	add edx, _asm2
	mov byte ptr [edi], bl
	inc edi

skipthe1byte:
	shr _asm3, 1
	jnc short skipthe2byte
	mov eax, esi
hxsiz5: shr eax, 26
hysiz5: shld eax, edx, 6
hoffs2: mov cl, byte ptr [eax+88888888h]
pal2: mov bl, byte ptr [ecx+88888888h]
	add esi, _asm1
	add edx, _asm2

	mov eax, esi
hxsiz6: shr eax, 26
hysiz6: shld eax, edx, 6
hoffs3: mov cl, byte ptr [eax+88888888h]
pal3: mov bh, byte ptr [ecx+88888888h]
	add esi, _asm1
	add edx, _asm2
	mov word ptr [edi], bx
	add edi, 2

skipthe2byte:
	mov dl, byte ptr _asm3[0]
	test dl, dl
	jz near ptr skipthe4byte

beginhline64:
mach1a: lea eax, [esi+88888888h]
hxsiz1: shr esi, 26
hysiz1: shld esi, edx, 6
mach2a: add edx, 88888888h
hoffs4: mov cl, byte ptr [esi+88888888h]
mach1b: lea esi, [eax+88888888h]
pal4: mov bl, byte ptr [ecx+88888888h]

hxsiz2: shr eax, 26
hysiz2: shld eax, edx, 6
mach2b: add edx, 88888888h
hoffs5: mov cl, byte ptr [eax+88888888h]
mach1c: lea eax, [esi+88888888h]
pal5: mov bh, byte ptr [ecx+88888888h]

	shl ebx, 16
hxsiz3: shr esi, 26
hysiz3: shld esi, edx, 6
mach2c: add edx, 88888888h
hoffs6: mov cl, byte ptr [esi+88888888h]
mach1d: lea esi, [eax+88888888h]
pal6: mov bl, byte ptr [ecx+88888888h]

hxsiz4: shr eax, 26
hysiz4: shld eax, edx, 6
	add edi, 4
hoffs7: mov cl, byte ptr [eax+88888888h]
mach2d: add edx, 88888888h
pal7: mov bh, byte ptr [ecx+88888888h]

	rol ebx, 16
	dec dl
	mov dword ptr [edi-4], ebx
	jnz near ptr beginhline64

skipthe4byte:

	pop ebp
	ret

PUBLIC hchainasm64_
startx: dd 0
starty: dd 0
startp: dd 0
xc4: dd 0
yc4: dd 0
plane: db 0

hchainasm64_:
	push ebp
	mov cl, al
	mov al, 11h
	rol al, cl
	mov byte ptr ds:[plane], al
	cmp _asm3, 4
	jge short hchainskip1
	mov cl, ah
	mov al, 11h
	rol al, cl

hchainskip1:
	mov byte ptr ds:[hchainskip5+1], al   ;startplane
	and ecx, 0ff00h
	mov dword ptr ds:[startx], esi
	mov dword ptr ds:[starty], ebx
	mov dword ptr ds:[startp], edi
	mov eax, _asm1
	shl eax, 2
	mov dword ptr ds:[xc4], eax
	mov dword ptr ds:[hchainstart1+2], eax
	mov dword ptr ds:[hchainasm64_advance1+2], eax
	mov dword ptr ds:[hchainasm64_advance2+2], eax
	mov dword ptr ds:[hchainasm64_advance3+2], eax
	mov eax, _asm2
	shl eax, 2
	and eax, 0ffffff00h
	mov dword ptr ds:[yc4], eax
	mov dword ptr ds:[hchainasm64_step1+2], eax
	mov dword ptr ds:[hchainasm64_step2+2], eax
	mov dword ptr ds:[hchainasm64_step3+2], eax
	mov dword ptr ds:[hchainasm64_step4+2], eax
	mov edx, 3c4h
	mov al, 2
	out dx, al

hchain1:
	mov edx, 3c5h
	mov al, byte ptr ds:[plane]
	out dx, al
	mov esi, dword ptr ds:[startx]
	mov edx, dword ptr ds:[starty]
	mov edi, dword ptr ds:[startp]
	mov eax, _asm3
	shr eax, 2
	inc al
	mov dl, al
	shr dl, 1
	jae short hchainasm64_loc_0d8
	mov eax, esi
hxsiz14: shr eax, 26
hysiz14: shld eax, edx, 6
	add esi, dword ptr ds:[xc4]
hchainasm64_load1: mov cl, byte ptr [eax+88888888h]
	add edx, dword ptr ds:[yc4]
hchainasm64_load2: mov bl, byte ptr [ecx+88888888h]
	mov byte ptr [edi], bl
	inc edi

hchainasm64_loc_0d8:
	shr dl, 1
	jae short hchainasm64_loc_124
	mov eax, esi
hxsiz12: shr eax, 26
hysiz12: shld eax, edx, 6
	add esi, dword ptr ds:[xc4]
hchainasm64_load3: mov cl, byte ptr [eax+88888888h]
	add edx, dword ptr ds:[yc4]
hchainasm64_load4: mov bl, byte ptr [ecx+88888888h]

	mov eax, esi
hxsiz13: shr eax, 26
hysiz13: shld eax, edx, 6
	add esi, dword ptr ds:[xc4]
hchainasm64_load5: mov cl, byte ptr [eax+88888888h]
	add edx, dword ptr ds:[yc4]
hchainasm64_load6: mov bh, byte ptr [ecx+88888888h]
	mov word ptr [edi], bx
	add edi, 2

hchainasm64_loc_124:
	test dl, dl
	je near ptr hchainskip4

hchainstart1:
	lea eax, [esi+88888888h]
hxsiz8: shr esi, 26
hysiz8: shld esi, edx, 6
hchainasm64_step1: add edx, 88888888h
hchainasm64_load7: mov cl, byte ptr [esi+88888888h]
hchainasm64_advance1: lea esi, [eax+88888888h]
hchainasm64_load8: mov bl, byte ptr [ecx+88888888h]

hxsiz9: shr eax, 26
hysiz9: shld eax, edx, 6
hchainasm64_step2: add edx, 88888888h
hchainasm64_load9: mov cl, byte ptr [eax+88888888h]
hchainasm64_advance2: lea eax, [esi+88888888h]
hchainasm64_load10: mov bh, byte ptr [ecx+88888888h]

	shl ebx, 16
hxsiz10: shr esi, 26
hysiz10: shld esi, edx, 6
hchainasm64_step3: add edx, 88888888h
hchainasm64_load11: mov cl, byte ptr [esi+88888888h]
hchainasm64_advance3: lea esi, [eax+88888888h]
hchainasm64_load12: mov bl, byte ptr [ecx+88888888h]

hxsiz11: shr eax, 26
hysiz11: shld eax, edx, 6
hchainasm64_load13: mov cl, byte ptr [eax+88888888h]
hchainasm64_step4: add edx, 88888888h
hchainasm64_load14: mov bh, byte ptr [ecx+88888888h]

	rol ebx, 16
	mov dword ptr [edi], ebx
	add edi, 4
	dec dl
	jne near ptr hchainstart1

hchainskip4:
	mov eax, _asm1
	add dword ptr ds:[startx], eax
	mov eax, _asm2
	add dword ptr ds:[starty], eax
	dec _asm3
	mov al, byte ptr ds:[plane]
	rol al, 1
	cmp al, 11h
	jne short hchainskip5
	inc dword ptr ds:[startp]

hchainskip5:
	cmp al, 88h
	mov byte ptr ds:[plane], al
	jne near ptr hchain1

	pop ebp
	ret

PUBLIC setvlinebpl_
setvlinebpl_:
	mov dword ptr ds:[fixchain1b+2], eax
	mov dword ptr ds:[fixchain1m+2], eax
	mov dword ptr ds:[fixchain1t+2], eax
	mov dword ptr ds:[fixchain2a+2], eax
	mov dword ptr ds:[fixchain2m+2], eax
	neg eax
	mov dword ptr ds:[fixchain1a+2], eax
	mov dword ptr ds:[fixchain2b+2], eax
	cmp dword ptr ds:[fixchain2m1+2], 0fffffec0h
	jne near ptr stride_done

	add eax, 320                     ;delta from the assembled 320-byte stride
	add dword ptr ds:[fixchain2m1+2], eax    ;0-bpl
	add dword ptr ds:[fixchain2m2+2], eax    ;1-bpl
	add dword ptr ds:[fixchain2m3+3], eax    ;0-bpl
	add dword ptr ds:[fixchain2m4+2], eax    ;2-bpl
	add dword ptr ds:[fixchain2m5+2], eax    ;0-bpl
	add dword ptr ds:[fixchain2m6+2], eax    ;2-bpl
	add dword ptr ds:[fixchain2m7+3], eax    ;1-bpl
	add dword ptr ds:[fixchain2m8+3], eax    ;0-bpl
	add dword ptr ds:[fixchain2m9+2], eax    ;2-bpl
	add dword ptr ds:[fixchain2m10+2], eax    ;3-bpl
	add dword ptr ds:[fixchain2m11+2], eax    ;0-bpl
	add dword ptr ds:[fixchain2m12+2], eax    ;3-bpl
	add dword ptr ds:[fixchain2m13+2], eax    ;1-bpl
	add dword ptr ds:[fixchain2m14+2], eax    ;3-bpl
	add dword ptr ds:[fixchain2m15+3], eax    ;0-bpl
	add dword ptr ds:[fixchain2m16+2], eax    ;3-bpl
	add dword ptr ds:[fixchain2m17+3], eax    ;2-bpl
	add dword ptr ds:[fixchain2m18+2], eax    ;0-bpl
	add dword ptr ds:[fixchain2m19+3], eax    ;2-bpl
	add dword ptr ds:[fixchain2m20+2], eax    ;1-bpl
	add dword ptr ds:[fixchain2m21+3], eax    ;2-bpl
	add dword ptr ds:[fixchain2m22+2], eax    ;0-bpl

stride_done:
	ret

PUBLIC vlineasm1_
vlineasm1_:
	mov byte ptr ds:[mach3a+2], cl
	mov dword ptr ds:[mach3b+2], eax
	mov dword ptr ds:[mach3c+2], ebx
	shr ecx, 8
	inc ecx
	jmp short beginvline

beginvline:
	mov ebx, edx
mach3a: shr ebx, 32
mach3b: add edx, 88888888h
	mov bl, byte ptr [esi+ebx]
fixchain1b: add edi, 320
mach3c: mov al, byte ptr [ebx+88888888h]
	dec ecx
fixchain1a: mov byte ptr [edi-320], al
	jnz short beginvline
	mov eax, edx
	ret

PUBLIC mvlineasm1_      ;Masked vline
mvlineasm1_:
	push ebp
	mov ebp, eax
	mov byte ptr ds:[maskmach3a+2], cl
	shr ecx, 8
	inc ecx
	mov dword ptr ds:[maskmach3c+2], ebx
	jmp short beginmvline

beginmvline:
	mov ebx, edx
maskmach3a: shr ebx, 32
	mov bl, byte ptr [esi+ebx]
	cmp bl, 255
	je short skipmask1
maskmach3c: mov al, byte ptr [ebx+88888888h]
	mov byte ptr [edi], al
skipmask1:
	add edx, ebp
fixchain1m: add edi, 320
	dec ecx
	jnz short beginmvline

	pop ebp
	mov eax, edx
	ret

PUBLIC fixtransluscence_
fixtransluscence_:
	mov dword ptr ds:[transmach4+2], eax
	mov dword ptr ds:[tmach1+2], eax
	mov dword ptr ds:[tmach2+2], eax
	mov dword ptr ds:[tmach3+2], eax
	ret

PUBLIC tvlineasm1_        ;Masked & transluscent vline
tvlineasm1_:
	push ebp
	mov ebp, eax
	xor eax, eax
	mov byte ptr ds:[transmach3a+2], cl
	shr ecx, 8
	inc ecx
	mov dword ptr ds:[transmach3c+2], ebx
	jmp short begintvline

begintvline:
	mov ebx, edx
transmach3a: shr ebx, 32
	mov bl, byte ptr [esi+ebx]
	cmp bl, 255
	je short skiptrans1
transmach3c: mov al, byte ptr [ebx+88888888h]
	mov ah, byte ptr [edi]
transmach4: mov al, byte ptr [eax+88888888h]    ;_transluc[eax]
	mov byte ptr [edi], al
skiptrans1:
	add edx, ebp
fixchain1t: add edi, 320
	dec ecx
	jnz short begintvline

	pop ebp
	mov eax, edx
	ret

PUBLIC vlineasm4_
vlineasm4_:
	push ebp

	mov byte ptr ds:[machv13+2], cl
	mov byte ptr ds:[machv14+2], cl
	mov byte ptr ds:[machv15+2], cl
	mov byte ptr ds:[machv16+2], cl

	mov ebp, dword ptr [edx+0]        ;palookupoffse
	mov dword ptr ds:[machv1+2], ebp
	mov ebp, dword ptr [edx+4]
	mov dword ptr ds:[machv4+2], ebp
	mov ebp, dword ptr [edx+8]
	mov dword ptr ds:[machv7+2], ebp
	mov ebp, dword ptr [edx+12]
	mov dword ptr ds:[machv10+2], ebp

	mov ebp, dword ptr [esi+0]        ;bufplce
	mov dword ptr ds:[machv2+2], ebp
	mov ebp, dword ptr [esi+4]
	mov dword ptr ds:[machv5+2], ebp
	mov ebp, dword ptr [esi+8]
	mov dword ptr ds:[machv8+2], ebp
	mov ebp, dword ptr [esi+12]
	mov dword ptr ds:[machv11+2], ebp

	mov ebp, dword ptr [ebx+0]        ;vince
	and ebp, 0ffffff00h           ;make sure dl (counter2) doesn't get changed
	mov dword ptr ds:[machv3+2], ebp
	mov ebp, dword ptr [ebx+4]
	mov dword ptr ds:[machv6+2], ebp
	mov ebp, dword ptr [ebx+8]
	mov dword ptr ds:[machv9+2], ebp
	mov ebp, dword ptr [ebx+12]
	and ebp, 0ffffff00h           ;make sure cl (counter) doesn't get changed
	mov dword ptr ds:[machv12+2], ebp

	mov ebx, ecx
	shr ebx, 8
	mov _asm2, eax
	mov edx, dword ptr [eax]
	mov esi, dword ptr [eax+4]
	mov ebp, dword ptr [eax+8]
	mov ecx, dword ptr [eax+12]
	mov cl, bl
	mov dl, bh
	inc cl
	inc dl

	jmp short beginvlineasm4
beginvlineasm4:

	mov eax, ecx
machv14: shr eax, 32
machv12: add ecx, 88888888h    ;vince[3]
machv10: mov al, byte ptr [eax+88888888h]    ;bufplce[3]
machv11: mov bh, byte ptr [eax+88888888h]    ;palookupoffs[3]

	mov eax, ebp
machv13: shr eax, 32
machv9: add ebp, 88888888h    ;vince[2]
machv7: mov al, byte ptr [eax+88888888h]    ;bufplce[2]
fixchain2a: add edi, 320
machv8: mov bl, byte ptr [eax+88888888h]    ;palookupoffs[2]

	mov eax, esi
machv16: shr eax, 32
	shl ebx, 16
machv4: mov al, byte ptr [eax+88888888h]    ;bufplce[1]
machv6: add esi, 88888888h    ;vince[1]
machv5: mov bh, byte ptr [eax+88888888h]    ;palookupoffs[1]

	mov eax, edx
machv15: shr eax, 32
machv3: add edx, 88888888h    ;vince[0]
machv1: mov al, byte ptr [eax+88888888h]    ;bufplce[0]
	dec cl
machv2: mov bl, byte ptr [eax+88888888h]    ;palookupoffs[0]
fixchain2b: mov dword ptr [edi-320], ebx
	jnz short beginvlineasm4

	dec dl
	jnz short beginvlineasm4

	mov eax, _asm2
	mov dword ptr [eax], edx
	mov dword ptr [eax+4], esi
	mov dword ptr [eax+8], ebp
	mov dword ptr [eax+12], ecx
	mov eax, edi

	pop ebp
	ret

PUBLIC mvlineasm4_
mvlineasm4_:
	push ebp

	mov byte ptr ds:[machmv13+2], cl
	mov byte ptr ds:[machmv14+2], cl
	mov byte ptr ds:[machmv15+2], cl
	mov byte ptr ds:[machmv16+2], cl

	mov ebp, dword ptr [edx+0]        ;palookupoffse
	mov dword ptr ds:[machmv1+2], ebp
	mov ebp, dword ptr [edx+4]
	mov dword ptr ds:[machmv4+2], ebp
	mov ebp, dword ptr [edx+8]
	mov dword ptr ds:[machmv7+2], ebp
	mov ebp, dword ptr [edx+12]
	mov dword ptr ds:[machmv10+2], ebp

	mov ebp, dword ptr [esi+0]        ;bufplce
	mov dword ptr ds:[machmv2+2], ebp
	mov ebp, dword ptr [esi+4]
	mov dword ptr ds:[machmv5+2], ebp
	mov ebp, dword ptr [esi+8]
	mov dword ptr ds:[machmv8+2], ebp
	mov ebp, dword ptr [esi+12]
	mov dword ptr ds:[machmv11+2], ebp

	mov ebp, dword ptr [ebx+0]        ;vince
	and ebp, 0ffffff00h           ;make sure dl (counter) doesn't get changed
	mov dword ptr ds:[machmv3+2], ebp
	mov ebp, dword ptr [ebx+4]
	mov dword ptr ds:[machmv6+2], ebp
	mov ebp, dword ptr [ebx+8]
	mov dword ptr ds:[machmv9+2], ebp
	mov ebp, dword ptr [ebx+12]
	and ebp, 0ffffff00h           ;make sure cl (counter) doesn't get changed
	mov dword ptr ds:[machmv12+2], ebp

	mov ebx, ecx
	shr ebx, 8
	mov _asm2, eax
	mov edx, dword ptr [eax]
	mov esi, dword ptr [eax+4]
	mov ebp, dword ptr [eax+8]
	mov ecx, dword ptr [eax+12]
	mov cl, bl
	inc cl
	inc bh
	mov byte ptr _asm3[0], bh

	jmp short beginmvlineasm4
beginmvlineasm4:
	xor dl, dl

	mov eax, ecx
machmv16: shr eax, 32
machmv12: add ecx, 88888888h    ;vince[3]
machmv10: mov al, byte ptr [eax+88888888h]    ;bufplce[3]
	cmp al, 255
	adc dl, dl
machmv11: mov bh, byte ptr [eax+88888888h]    ;palookupoffs[3]

	mov eax, ebp
machmv15: shr eax, 32
machmv9: add ebp, 88888888h    ;vince[2]
machmv7: mov al, byte ptr [eax+88888888h]    ;bufplce[2]
	cmp al, 255
	adc dl, dl
machmv8: mov bl, byte ptr [eax+88888888h]    ;palookupoffs[2]

	mov eax, esi
machmv14: shr eax, 32
	shl ebx, 16
machmv4: mov al, byte ptr [eax+88888888h]    ;bufplce[1]
	cmp al, 255
	adc dl, dl
machmv6: add esi, 88888888h    ;vince[1]
machmv5: mov bh, byte ptr [eax+88888888h]    ;palookupoffs[1]

	mov eax, edx
machmv13: shr eax, 32
machmv3: add edx, 88888888h    ;vince[0]
machmv1: mov al, byte ptr [eax+88888888h]    ;bufplce[0]
	cmp al, 255
	adc dl, dl
machmv2: mov bl, byte ptr [eax+88888888h]    ;palookupoffs[0]
fixchain2m: add edi, 320

	mov al, dl                 ;32 byte cases
	shl eax, 5
	add eax, offset mvcase0
	jmp eax

endmvlineasm4:
	dec byte ptr _asm3[0]
	jnz near ptr beginmvlineasm4

	mov eax, _asm2
	mov dword ptr [eax], edx
	mov dword ptr [eax+4], esi
	mov dword ptr [eax+8], ebp
	mov dword ptr [eax+12], ecx
	mov eax, edi

	pop ebp
	ret

mvcase0:
	dec cl
	jnz near ptr beginmvlineasm4
	jmp short endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase1:
fixchain2m1: mov byte ptr [edi-320], bl
	dec cl
	jnz near ptr beginmvlineasm4
	jmp short endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase2:
fixchain2m2: mov byte ptr [edi-319], bh
	dec cl
	jnz near ptr beginmvlineasm4
	jmp short endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase3:
fixchain2m3: mov word ptr [edi-320], bx
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase4:
	shr ebx, 16
fixchain2m4: mov byte ptr [edi-318], bl
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase5:
fixchain2m5: mov byte ptr [edi-320], bl
	shr ebx, 16
fixchain2m6: mov byte ptr [edi-318], bl
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop

mvcase6:
	shr ebx, 8
fixchain2m7: mov word ptr [edi-319], bx
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase7:
fixchain2m8: mov word ptr [edi-320], bx
	shr ebx, 16
fixchain2m9: mov byte ptr [edi-318], bl
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop

mvcase8:
	shr ebx, 16
fixchain2m10: mov byte ptr [edi-317], bh
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase9:
fixchain2m11: mov byte ptr [edi-320], bl
	shr ebx, 16
fixchain2m12: mov byte ptr [edi-317], bh
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop

mvcase10:
fixchain2m13: mov byte ptr [edi-319], bh
	shr ebx, 16
fixchain2m14: mov byte ptr [edi-317], bh
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop

mvcase11:
fixchain2m15: mov word ptr [edi-320], bx
	shr ebx, 16
fixchain2m16: mov byte ptr [edi-317], bh
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop

mvcase12:
	shr ebx, 16
fixchain2m17: mov word ptr [edi-318], bx
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

mvcase13:
fixchain2m18: mov byte ptr [edi-320], bl
	shr ebx, 16
fixchain2m19: mov word ptr [edi-318], bx
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop

mvcase14:
fixchain2m20: mov byte ptr [edi-319], bh
	shr ebx, 16
fixchain2m21: mov word ptr [edi-318], bx
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4
	nop
	nop
	nop

mvcase15:
fixchain2m22: mov dword ptr [edi-320], ebx
	dec cl
	jnz near ptr beginmvlineasm4
	jmp near ptr endmvlineasm4

; Draw a masked sprite column, advancing its two fixed-point texture coordinates.
PUBLIC spritevline_
spritevline_:
	push ebp
	mov ebp, _asm1
	mov dword ptr ds:[fixchain1s+2], ebp
	mov ebp, eax
	shl ebp, 16
	mov dword ptr ds:[smach1+2], ebp
	mov ebp, eax
	sar ebp, 16
	mov dword ptr ds:[smach2+2], ebp
	mov ebp, _asm3
	mov dword ptr ds:[smach3+2], ebp
	mov ebp, _asm4
	mov dword ptr ds:[spritevline_mask1+2], ebp
	mov ebp, _asm2
	mov dword ptr ds:[skipsvline+3], ebp
	mov ebp, ebx
	sar ebp, 16
	add esi, ebp

	shl ebx, 16
	jmp short sprite_loop
	nop
	nop
	nop
sprite_loop:
	mov al, byte ptr [esi]
smach3: add edx, 88888888h    ;dayinc
	sbb ebp, ebp
smach1: add ebx, 88888888h    ;xincshl16
smach2: adc esi, 88888888h    ;xincshr16
spritevline_mask1: and ebp, 88888888h

startsvline:
	cmp al, 255
	je near ptr skipsvline
	mov byte ptr [edi], al
skipsvline:
	lea esi, [esi+ebp+88888888h]
fixchain1s: add edi, 88888888h
	sub ecx, 1
	jae short sprite_loop

	pop ebp
	ret

PUBLIC msethlineshift_
msethlineshift_:
	neg al
	mov byte ptr ds:[msh1d+2], al
	mov byte ptr ds:[msh2d+3], bl
	mov byte ptr ds:[msh3d+2], al
	mov byte ptr ds:[msh4d+3], bl
	mov byte ptr ds:[msh5d+2], al
	mov byte ptr ds:[msh6d+3], bl
	ret

PUBLIC mhline_
mhline_:
	;_asm1 = bxinc
	;_asm2 = byinc
	;_asm3 = shadeoffs
	;eax = picoffs
	;ebx = bx
	;ecx = cnt
	;edx = ?
	;esi = by
	;edi = p

	mov dword ptr ds:[mmach1d+2], eax
	mov dword ptr ds:[mmach5d+2], eax
	mov dword ptr ds:[mmach9d+2], eax
	mov eax, _asm3
	mov dword ptr ds:[mmach2d+2], eax
	mov dword ptr ds:[mmach2da+2], eax
	mov dword ptr ds:[mmach2db+2], eax
	mov dword ptr ds:[mmach6d+2], eax
	mov dword ptr ds:[mmach10d+2], eax
	mov eax, _asm1
	mov dword ptr ds:[mmach3d+2], eax
	mov dword ptr ds:[mmach7d+2], eax
	mov eax, _asm2
	mov dword ptr ds:[mmach4d+2], eax
	mov dword ptr ds:[mmach8d+2], eax
	jmp short mhlineskipmodify_
	nop
	nop
	nop

PUBLIC mhlineskipmodify_
mhlineskipmodify_:
	push ebp
	xor eax, eax
	mov ebp, ebx

	test ecx, 00010000h
	jnz short mbeghline
msh1d: shr ebx, 26
msh2d: shld ebx, esi, 6
	add ebp, _asm1
mmach9d: mov al, byte ptr [ebx+88888888h]    ;picoffs
	add esi, _asm2
	cmp al, 255
	je near ptr mskip5

mmach10d: mov cl, byte ptr [eax+88888888h]    ;shadeoffs
	mov byte ptr [edi], cl
mskip5:
	inc edi
	sub ecx, 65536
	jc near ptr mendhline
	jmp short mbeghline

mpreprebeghline:    ;1st only
	mov al, cl
mmach2d: mov al, byte ptr [eax+88888888h]    ;shadeoffs
	mov byte ptr [edi], al

mprebeghline:
	add edi, 2
	sub ecx, 131072
	jc short mendhline

mbeghline:
mmach3d: lea ebx, [ebp+88888888h]    ;bxinc
msh3d: shr ebp, 26
msh4d: shld ebp, esi, 6
mmach4d: add esi, 88888888h    ;byinc
mmach1d: mov cl, byte ptr [ebp+88888888h]    ;picoffs
mmach7d: lea ebp, [ebx+88888888h]    ;bxinc

msh5d: shr ebx, 26
msh6d: shld ebx, esi, 6
mmach8d: add esi, 88888888h    ;byinc
mmach5d: mov ch, byte ptr [ebx+88888888h]    ;picoffs

	cmp cl, 255
	je short mskip1
	cmp ch, 255
	je short mpreprebeghline

	mov al, cl                      ;BOTH
mmach2da: mov bl, byte ptr [eax+88888888h]    ;shadeoffs
	mov al, ch
mmach2db: mov bh, byte ptr [eax+88888888h]    ;shadeoffs
	mov word ptr [edi], bx
	add edi, 2
	sub ecx, 131072
	jnc short mbeghline
	jmp short mendhline
	nop
	nop
	nop

mskip1:    ;2nd only
	cmp ch, 255
	je short mprebeghline
	mov al, ch
mmach6d: mov al, byte ptr [eax+88888888h]    ;shadeoffs
	mov byte ptr [edi+1], al
	add edi, 2
	sub ecx, 131072
	jnc short mbeghline

mendhline:

	pop ebp
	ret

PUBLIC tsethlineshift_
tsethlineshift_:
	neg al
	mov byte ptr ds:[tsh1d+2], al
	mov byte ptr ds:[tsh2d+3], bl
	mov byte ptr ds:[tsh3d+2], al
	mov byte ptr ds:[tsh4d+3], bl
	mov byte ptr ds:[tsh5d+2], al
	mov byte ptr ds:[tsh6d+3], bl
	ret

PUBLIC thline_
thline_:
	;_asm1 = bxinc
	;_asm2 = byinc
	;_asm3 = shadeoffs
	;eax = picoffs
	;ebx = bx
	;ecx = cnt
	;edx = ?
	;esi = by
	;edi = p

	mov dword ptr ds:[tmach1d+2], eax
	mov dword ptr ds:[tmach5d+2], eax
	mov dword ptr ds:[tmach9d+2], eax
	mov eax, _asm3
	mov dword ptr ds:[tmach2d+2], eax
	mov dword ptr ds:[tmach6d+2], eax
	mov dword ptr ds:[tmach10d+2], eax
	mov eax, _asm1
	mov dword ptr ds:[tmach3d+2], eax
	mov dword ptr ds:[tmach7d+2], eax
	mov eax, _asm2
	mov dword ptr ds:[tmach4d+2], eax
	mov dword ptr ds:[tmach8d+2], eax
	jmp short thlineskipmodify_
	nop
	nop
	nop

PUBLIC thlineskipmodify_
thlineskipmodify_:
	push ebp
	xor eax, eax
	xor edx, edx
	mov ebp, ebx

	test ecx, 00010000h
	jnz short tbeghline
tsh1d: shr ebx, 26
tsh2d: shld ebx, esi, 6
	add ebp, _asm1
tmach9d: mov al, byte ptr [ebx+88888888h]    ;picoffs
	add esi, _asm2
	cmp al, 255
	je near ptr tskip5

tmach10d: mov dl, byte ptr [eax+88888888h]    ;shadeoffs
	mov dh, byte ptr [edi]
tmach1: mov al, byte ptr [edx+88888888h]    ;transluc
	mov byte ptr [edi], al
tskip5:
	inc edi
	sub ecx, 65536
	jc near ptr tendhline
	jmp short tbeghline

tprebeghline:
	add edi, 2
	sub ecx, 131072
	jc short tendhline

tbeghline:
tmach3d: lea ebx, [ebp+88888888h]    ;bxinc
tsh3d: shr ebp, 26
tsh4d: shld ebp, esi, 6
tmach4d: add esi, 88888888h    ;byinc
tmach1d: mov cl, byte ptr [ebp+88888888h]    ;picoffs
tmach7d: lea ebp, [ebx+88888888h]    ;bxinc

tsh5d: shr ebx, 26
tsh6d: shld ebx, esi, 6
tmach8d: add esi, 88888888h    ;byinc
tmach5d: mov ch, byte ptr [ebx+88888888h]    ;picoffs

	cmp cx, -1
	je short tprebeghline

	mov bx, word ptr [edi]

	cmp cl, 255
	je short tskip1
	mov al, cl
tmach2d: mov dl, byte ptr [eax+88888888h]    ;shadeoffs
	mov dh, bl
tmach2: mov al, byte ptr [edx+88888888h]    ;transluc
	mov byte ptr [edi], al

	cmp ch, 255
	je short tskip2
tskip1:
	mov al, ch
tmach6d: mov dl, byte ptr [eax+88888888h]    ;shadeoffs
	mov dh, bh
tmach3: mov al, byte ptr [edx+88888888h]    ;transluc
	mov byte ptr [edi+1], al

tskip2:
	add edi, 2
	sub ecx, 131072
	jnc short tbeghline

tendhline:

	pop ebp
	ret

CODE ENDS
END
