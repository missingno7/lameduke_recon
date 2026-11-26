; "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
; See BUILDLIC.TXT in the historical Build archive for license terms.
; December 1994 rasterizer reconstruction, using the April 1994 and May 1995
; sources and surviving historical objects as references.
; Code writes to instruction operands are part of the original rasterizers.
; Assemble from a worker scratch directory containing a copy named A.ASM:
;   C:/tools/nmlgcdos/msdos.exe C:/tools/masm-5.10/files/MASM.EXE "a.asm;"
; Run tools/match.py --obj on the result with this worker's overlay.json.
; See NOTES.md for the full invocation and the 20/20 STRICT verification.

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
	add al, 20h
	mov byte ptr ds:[hlineasm64_xshift1+2], al
	mov byte ptr ds:[hlineasm64_xshift2+2], al
	mov byte ptr ds:[hlineasm64_xshift3+2], al
	mov byte ptr ds:[hlineasm64_xshift4+2], al
	mov byte ptr ds:[hlineasm64_xshift5+2], al
	mov byte ptr ds:[hlineasm64_xshift6+2], al
	mov byte ptr ds:[hlineasm64_xshift7+2], al
	mov al, bl
	mov byte ptr ds:[hlineasm64_yshift1+3], al
	mov byte ptr ds:[hlineasm64_yshift2+3], al
	mov byte ptr ds:[hlineasm64_yshift3+3], al
	mov byte ptr ds:[hlineasm64_yshift4+3], al
	mov byte ptr ds:[hlineasm64_yshift5+3], al
	mov byte ptr ds:[hlineasm64_yshift6+3], al
	mov byte ptr ds:[hlineasm64_yshift7+3], al
	mov dword ptr ds:[hlineasm64_load1+2], ecx
	mov dword ptr ds:[hlineasm64_load3+2], ecx
	mov dword ptr ds:[hlineasm64_load5+2], ecx
	mov dword ptr ds:[hlineasm64_load7+2], ecx
	mov dword ptr ds:[hlineasm64_load9+2], ecx
	mov dword ptr ds:[hlineasm64_load11+2], ecx
	mov dword ptr ds:[hlineasm64_load13+2], ecx
	ret

PUBLIC sethlinechainsizes_
sethlinechainsizes_:
	neg al
	add al, 20h
	mov byte ptr ds:[hchainasm64_xshift1+2], al
	mov byte ptr ds:[hchainasm64_xshift2+2], al
	mov byte ptr ds:[hchainasm64_xshift3+2], al
	mov byte ptr ds:[hchainasm64_xshift4+2], al
	mov byte ptr ds:[hchainasm64_xshift5+2], al
	mov byte ptr ds:[hchainasm64_xshift6+2], al
	mov byte ptr ds:[hchainasm64_xshift7+2], al
	mov al, bl
	mov byte ptr ds:[hchainasm64_yshift1+3], al
	mov byte ptr ds:[hchainasm64_yshift2+3], al
	mov byte ptr ds:[hchainasm64_yshift3+3], al
	mov byte ptr ds:[hchainasm64_yshift4+3], al
	mov byte ptr ds:[hchainasm64_yshift5+3], al
	mov byte ptr ds:[hchainasm64_yshift6+3], al
	mov byte ptr ds:[hchainasm64_yshift7+3], al
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
	mov dword ptr ds:[hlineasm64_load2+2], eax
	mov dword ptr ds:[hlineasm64_load4+2], eax
	mov dword ptr ds:[hlineasm64_load6+2], eax
	mov dword ptr ds:[hlineasm64_load8+2], eax
	mov dword ptr ds:[hlineasm64_load10+2], eax
	mov dword ptr ds:[hlineasm64_load12+2], eax
	mov dword ptr ds:[hlineasm64_load14+2], eax
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

; Draw four pixels at a time. Setup entries patch shifts and texture/palette operands.
PUBLIC hlineasm64_
hlineasm64_:
	push ebp
	mov eax, dword ptr [_asm1]
	mov dword ptr ds:[hline_draw_four+2], eax
	mov dword ptr ds:[hlineasm64_advance1+2], eax
	mov dword ptr ds:[hlineasm64_advance2+2], eax
	mov dword ptr ds:[hlineasm64_advance3+2], eax
	and edx, 0ffffff00h
	and dword ptr [_asm2], 0ffffff00h
	mov eax, dword ptr [_asm2]
	mov dword ptr ds:[hlineasm64_step1+2], eax
	mov dword ptr ds:[hlineasm64_step2+2], eax
	mov dword ptr ds:[hlineasm64_step3+2], eax
	mov dword ptr ds:[hlineasm64_step4+2], eax
	xchg cl, ch
	shr dword ptr [_asm3], 1
	jae short hline_pair
	mov eax, esi
hlineasm64_xshift1:
	shr eax, 1ah
hlineasm64_yshift1:
	shld eax, edx, 6
hlineasm64_load1:
	mov cl, byte ptr [eax+88888888h]
hlineasm64_load2:
	mov bl, byte ptr [ecx+88888888h]
	add esi, dword ptr [_asm1]
	add edx, dword ptr [_asm2]
	mov byte ptr [edi], bl
	inc edi
hline_pair:
	shr dword ptr [_asm3], 1
	jae short hline_groups
	mov eax, esi
hlineasm64_xshift2:
	shr eax, 1ah
hlineasm64_yshift2:
	shld eax, edx, 6
hlineasm64_load3:
	mov cl, byte ptr [eax+88888888h]
hlineasm64_load4:
	mov bl, byte ptr [ecx+88888888h]
	add esi, dword ptr [_asm1]
	add edx, dword ptr [_asm2]
	mov eax, esi
hlineasm64_xshift3:
	shr eax, 1ah
hlineasm64_yshift3:
	shld eax, edx, 6
hlineasm64_load5:
	mov cl, byte ptr [eax+88888888h]
hlineasm64_load6:
	mov bh, byte ptr [ecx+88888888h]
	add esi, dword ptr [_asm1]
	add edx, dword ptr [_asm2]
	mov word ptr [edi], bx
	add edi, 2
hline_groups:
	mov dl, byte ptr [_asm3]
	test dl, dl
	je near ptr hline_done
hline_draw_four:
	lea eax, [esi+88888888h]
hlineasm64_xshift4:
	shr esi, 1ah
hlineasm64_yshift4:
	shld esi, edx, 6
hlineasm64_step1:
	add edx, 88888888h
hlineasm64_load7:
	mov cl, byte ptr [esi+88888888h]
hlineasm64_advance1:
	lea esi, [eax+88888888h]
hlineasm64_load8:
	mov bl, byte ptr [ecx+88888888h]
hlineasm64_xshift5:
	shr eax, 1ah
hlineasm64_yshift5:
	shld eax, edx, 6
hlineasm64_step2:
	add edx, 88888888h
hlineasm64_load9:
	mov cl, byte ptr [eax+88888888h]
hlineasm64_advance2:
	lea eax, [esi+88888888h]
hlineasm64_load10:
	mov bh, byte ptr [ecx+88888888h]
	shl ebx, 10h
hlineasm64_xshift6:
	shr esi, 1ah
hlineasm64_yshift6:
	shld esi, edx, 6
hlineasm64_step3:
	add edx, 88888888h
hlineasm64_load11:
	mov cl, byte ptr [esi+88888888h]
hlineasm64_advance3:
	lea esi, [eax+88888888h]
hlineasm64_load12:
	mov bl, byte ptr [ecx+88888888h]
hlineasm64_xshift7:
	shr eax, 1ah
hlineasm64_yshift7:
	shld eax, edx, 6
	add edi, 4
hlineasm64_load13:
	mov cl, byte ptr [eax+88888888h]
hlineasm64_step4:
	add edx, 88888888h
hlineasm64_load14:
	mov bh, byte ptr [ecx+88888888h]
	rol ebx, 10h
	dec dl
	mov dword ptr [edi-4], ebx
	jne near ptr hline_draw_four
hline_done:
	pop ebp
	ret

; Scratch state for planar VGA writes.
chain_x_start dd 0
chain_y_start dd 0
chain_dest dd 0
chain_x_plane_step dd 0
chain_y_plane_step dd 0
chain_plane_select db 0

; Unchained VGA variant. Rotate the sequencer mask across the four planes.
PUBLIC hchainasm64_
hchainasm64_:
	push ebp
	mov cl, al
	mov al, 11h
	rol al, cl
	mov byte ptr ds:[chain_plane_select], al
	cmp dword ptr [_asm3], 4
	jge short hchainasm64_loc_01b
	mov cl, ah
	mov al, 11h
	rol al, cl
hchainasm64_loc_01b:
	mov byte ptr ds:[hchainasm64_loc_1e8+1], al
	and ecx, 0ff00h
	mov dword ptr ds:[chain_x_start], esi
	mov dword ptr ds:[chain_y_start], ebx
	mov dword ptr ds:[chain_dest], edi
	mov eax, dword ptr [_asm1]
	shl eax, 2
	mov dword ptr ds:[chain_x_plane_step], eax
	mov dword ptr ds:[chain_draw_four+2], eax
	mov dword ptr ds:[hchainasm64_advance1+2], eax
	mov dword ptr ds:[hchainasm64_advance2+2], eax
	mov dword ptr ds:[hchainasm64_advance3+2], eax
	mov eax, dword ptr [_asm2]
	shl eax, 2
	and eax, 0ffffff00h
	mov dword ptr ds:[chain_y_plane_step], eax
	mov dword ptr ds:[hchainasm64_step1+2], eax
	mov dword ptr ds:[hchainasm64_step2+2], eax
	mov dword ptr ds:[hchainasm64_step3+2], eax
	mov dword ptr ds:[hchainasm64_step4+2], eax
	mov edx, 3c4h
	mov al, 2
	out dx, al
chain_next_plane:
	mov edx, 3c5h
	mov al, byte ptr ds:[chain_plane_select]
	out dx, al
	mov esi, dword ptr ds:[chain_x_start]
	mov edx, dword ptr ds:[chain_y_start]
	mov edi, dword ptr ds:[chain_dest]
	mov eax, dword ptr [_asm3]
	shr eax, 2
	inc al
	mov dl, al
	shr dl, 1
	jae short hchainasm64_loc_0d8
	mov eax, esi
hchainasm64_xshift1:
	shr eax, 1ah
hchainasm64_yshift1:
	shld eax, edx, 6
	add esi, dword ptr ds:[chain_x_plane_step]
hchainasm64_load1:
	mov cl, byte ptr [eax+88888888h]
	add edx, dword ptr ds:[chain_y_plane_step]
hchainasm64_load2:
	mov bl, byte ptr [ecx+88888888h]
	mov byte ptr [edi], bl
	inc edi
hchainasm64_loc_0d8:
	shr dl, 1
	jae short hchainasm64_loc_124
	mov eax, esi
hchainasm64_xshift2:
	shr eax, 1ah
hchainasm64_yshift2:
	shld eax, edx, 6
	add esi, dword ptr ds:[chain_x_plane_step]
hchainasm64_load3:
	mov cl, byte ptr [eax+88888888h]
	add edx, dword ptr ds:[chain_y_plane_step]
hchainasm64_load4:
	mov bl, byte ptr [ecx+88888888h]
	mov eax, esi
hchainasm64_xshift3:
	shr eax, 1ah
hchainasm64_yshift3:
	shld eax, edx, 6
	add esi, dword ptr ds:[chain_x_plane_step]
hchainasm64_load5:
	mov cl, byte ptr [eax+88888888h]
	add edx, dword ptr ds:[chain_y_plane_step]
hchainasm64_load6:
	mov bh, byte ptr [ecx+88888888h]
	mov word ptr [edi], bx
	add edi, 2
hchainasm64_loc_124:
	test dl, dl
	je near ptr chain_finish_plane
chain_draw_four:
	lea eax, [esi+88888888h]
hchainasm64_xshift4:
	shr esi, 1ah
hchainasm64_yshift4:
	shld esi, edx, 6
hchainasm64_step1:
	add edx, 88888888h
hchainasm64_load7:
	mov cl, byte ptr [esi+88888888h]
hchainasm64_advance1:
	lea esi, [eax+88888888h]
hchainasm64_load8:
	mov bl, byte ptr [ecx+88888888h]
hchainasm64_xshift5:
	shr eax, 1ah
hchainasm64_yshift5:
	shld eax, edx, 6
hchainasm64_step2:
	add edx, 88888888h
hchainasm64_load9:
	mov cl, byte ptr [eax+88888888h]
hchainasm64_advance2:
	lea eax, [esi+88888888h]
hchainasm64_load10:
	mov bh, byte ptr [ecx+88888888h]
	shl ebx, 10h
hchainasm64_xshift6:
	shr esi, 1ah
hchainasm64_yshift6:
	shld esi, edx, 6
hchainasm64_step3:
	add edx, 88888888h
hchainasm64_load11:
	mov cl, byte ptr [esi+88888888h]
hchainasm64_advance3:
	lea esi, [eax+88888888h]
hchainasm64_load12:
	mov bl, byte ptr [ecx+88888888h]
hchainasm64_xshift7:
	shr eax, 1ah
hchainasm64_yshift7:
	shld eax, edx, 6
hchainasm64_load13:
	mov cl, byte ptr [eax+88888888h]
hchainasm64_step4:
	add edx, 88888888h
hchainasm64_load14:
	mov bh, byte ptr [ecx+88888888h]
	rol ebx, 10h
	mov dword ptr [edi], ebx
	add edi, 4
	dec dl
	jne near ptr chain_draw_four
chain_finish_plane:
	mov eax, dword ptr [_asm1]
	add dword ptr ds:[chain_x_start], eax
	mov eax, dword ptr [_asm2]
	add dword ptr ds:[chain_y_start], eax
	dec dword ptr [_asm3]
	mov al, byte ptr ds:[chain_plane_select]
	rol al, 1
	cmp al, 11h
	jne short hchainasm64_loc_1e8
	inc dword ptr ds:[chain_dest]
hchainasm64_loc_1e8:
	cmp al, 88h
	mov byte ptr ds:[chain_plane_select], al
	jne near ptr chain_next_plane
	pop ebp
	ret

; Patch the strides and displacements in the vertical rasterizers.
PUBLIC setvlinebpl_
setvlinebpl_:
	mov dword ptr ds:[vlineasm1_step2+2], eax
	mov dword ptr ds:[mvlineasm1_step1+2], eax
	mov dword ptr ds:[tvlineasm1_step1+2], eax
	mov dword ptr ds:[vlineasm4_step3+2], eax
	mov dword ptr ds:[mvlineasm4_step5+2], eax
	neg eax
	mov dword ptr ds:[vlineasm1_store1+2], eax
	mov dword ptr ds:[vlineasm4_store1+2], eax
	cmp dword ptr ds:[mvlineasm4_store1+2], 0fffffec0h
	jne near ptr stride_done
	add eax, 140h
	add dword ptr ds:[mvlineasm4_store1+2], eax
	add dword ptr ds:[mvlineasm4_store2+2], eax
	add dword ptr ds:[mvlineasm4_store3+3], eax
	add dword ptr ds:[mvlineasm4_store4+2], eax
	add dword ptr ds:[mvlineasm4_store5+2], eax
	add dword ptr ds:[mvlineasm4_store6+2], eax
	add dword ptr ds:[mvlineasm4_store7+3], eax
	add dword ptr ds:[mvlineasm4_store8+3], eax
	add dword ptr ds:[mvlineasm4_store9+2], eax
	add dword ptr ds:[mvlineasm4_store10+2], eax
	add dword ptr ds:[mvlineasm4_store11+2], eax
	add dword ptr ds:[mvlineasm4_store12+2], eax
	add dword ptr ds:[mvlineasm4_store13+2], eax
	add dword ptr ds:[mvlineasm4_store14+2], eax
	add dword ptr ds:[mvlineasm4_store15+3], eax
	add dword ptr ds:[mvlineasm4_store16+2], eax
	add dword ptr ds:[mvlineasm4_store17+3], eax
	add dword ptr ds:[mvlineasm4_store18+2], eax
	add dword ptr ds:[mvlineasm4_store19+3], eax
	add dword ptr ds:[mvlineasm4_store20+2], eax
	add dword ptr ds:[mvlineasm4_store21+3], eax
	add dword ptr ds:[mvlineasm4_store22+2], eax
stride_done:
	ret

PUBLIC vlineasm1_
vlineasm1_:
	mov byte ptr ds:[vlineasm1_xshift1+2], cl
	mov dword ptr ds:[vlineasm1_step1+2], eax
	mov dword ptr ds:[vlineasm1_load1+2], ebx
	shr ecx, 8
	inc ecx
	jmp short vline_loop
vline_loop:
	mov ebx, edx
vlineasm1_xshift1:
	shr ebx, 20h
vlineasm1_step1:
	add edx, 88888888h
	mov bl, byte ptr [esi+ebx]
vlineasm1_step2:
	add edi, 140h
vlineasm1_load1:
	mov al, byte ptr [ebx+88888888h]
	dec ecx
vlineasm1_store1:
	mov byte ptr [edi-140h], al
	jne short vline_loop
	mov eax, edx
	ret

PUBLIC mvlineasm1_
mvlineasm1_:
	push ebp
	mov ebp, eax
	mov byte ptr ds:[mvlineasm1_xshift1+2], cl
	shr ecx, 8
	inc ecx
	mov dword ptr ds:[mvlineasm1_load1+2], ebx
	jmp short mvline_loop
mvline_loop:
	mov ebx, edx
mvlineasm1_xshift1:
	shr ebx, 20h
	mov bl, byte ptr [esi+ebx]
	cmp bl, 0ffh
	je short mvlineasm1_loc_02a
mvlineasm1_load1:
	mov al, byte ptr [ebx+88888888h]
	mov byte ptr [edi], al
mvlineasm1_loc_02a:
	add edx, ebp
mvlineasm1_step1:
	add edi, 140h
	dec ecx
	jne short mvline_loop
	pop ebp
	mov eax, edx
	ret

PUBLIC fixtransluscence_
fixtransluscence_:
	mov dword ptr ds:[tvlineasm1_load2+2], eax
	mov dword ptr ds:[thlineskipmodify_load3+2], eax
	mov dword ptr ds:[thlineskipmodify_load7+2], eax
	mov dword ptr ds:[thlineskipmodify_load9+2], eax
	ret

PUBLIC tvlineasm1_
tvlineasm1_:
	push ebp
	mov ebp, eax
	xor eax, eax
	mov byte ptr ds:[tvlineasm1_xshift1+2], cl
	shr ecx, 8
	inc ecx
	mov dword ptr ds:[tvlineasm1_load1+2], ebx
	jmp short tvlineasm1_loc_017
tvlineasm1_loc_017:
	mov ebx, edx
tvlineasm1_xshift1:
	shr ebx, 20h
	mov bl, byte ptr [esi+ebx]
	cmp bl, 0ffh
	je short tvlineasm1_loc_034
tvlineasm1_load1:
	mov al, byte ptr [ebx+88888888h]
	mov ah, byte ptr [edi]
tvlineasm1_load2:
	mov al, byte ptr [eax+88888888h]
	mov byte ptr [edi], al
tvlineasm1_loc_034:
	add edx, ebp
tvlineasm1_step1:
	add edi, 140h
	dec ecx
	jne short tvlineasm1_loc_017
	pop ebp
	mov eax, edx
	ret

PUBLIC vlineasm4_
vlineasm4_:
	push ebp
	mov byte ptr ds:[vlineasm4_xshift2+2], cl
	mov byte ptr ds:[vlineasm4_xshift1+2], cl
	mov byte ptr ds:[vlineasm4_xshift4+2], cl
	mov byte ptr ds:[vlineasm4_xshift3+2], cl
	mov ebp, dword ptr [edx]
	mov dword ptr ds:[vlineasm4_load7+2], ebp
	mov ebp, dword ptr [edx+4]
	mov dword ptr ds:[vlineasm4_load5+2], ebp
	mov ebp, dword ptr [edx+8]
	mov dword ptr ds:[vlineasm4_load3+2], ebp
	mov ebp, dword ptr [edx+0ch]
	mov dword ptr ds:[vlineasm4_load1+2], ebp
	mov ebp, dword ptr [esi]
	mov dword ptr ds:[vlineasm4_load8+2], ebp
	mov ebp, dword ptr [esi+4]
	mov dword ptr ds:[vlineasm4_load6+2], ebp
	mov ebp, dword ptr [esi+8]
	mov dword ptr ds:[vlineasm4_load4+2], ebp
	mov ebp, dword ptr [esi+0ch]
	mov dword ptr ds:[vlineasm4_load2+2], ebp
	mov ebp, dword ptr [ebx]
	and ebp, 0ffffff00h
	mov dword ptr ds:[vlineasm4_step5+2], ebp
	mov ebp, dword ptr [ebx+4]
	mov dword ptr ds:[vlineasm4_step4+2], ebp
	mov ebp, dword ptr [ebx+8]
	mov dword ptr ds:[vlineasm4_step2+2], ebp
	mov ebp, dword ptr [ebx+0ch]
	and ebp, 0ffffff00h
	mov dword ptr ds:[vlineasm4_step1+2], ebp
	mov ebx, ecx
	shr ebx, 8
	mov dword ptr [_asm2], eax
	mov edx, dword ptr [eax]
	mov esi, dword ptr [eax+4]
	mov ebp, dword ptr [eax+8]
	mov ecx, dword ptr [eax+0ch]
	mov cl, bl
	mov dl, bh
	inc cl
	inc dl
	jmp short vlineasm4_loc_0ad
vlineasm4_loc_0ad:
	mov eax, ecx
vlineasm4_xshift1:
	shr eax, 20h
vlineasm4_step1:
	add ecx, 88888888h
vlineasm4_load1:
	mov al, byte ptr [eax+88888888h]
vlineasm4_load2:
	mov bh, byte ptr [eax+88888888h]
	mov eax, ebp
vlineasm4_xshift2:
	shr eax, 20h
vlineasm4_step2:
	add ebp, 88888888h
vlineasm4_load3:
	mov al, byte ptr [eax+88888888h]
vlineasm4_step3:
	add edi, 140h
vlineasm4_load4:
	mov bl, byte ptr [eax+88888888h]
	mov eax, esi
vlineasm4_xshift3:
	shr eax, 20h
	shl ebx, 10h
vlineasm4_load5:
	mov al, byte ptr [eax+88888888h]
vlineasm4_step4:
	add esi, 88888888h
vlineasm4_load6:
	mov bh, byte ptr [eax+88888888h]
	mov eax, edx
vlineasm4_xshift4:
	shr eax, 20h
vlineasm4_step5:
	add edx, 88888888h
vlineasm4_load7:
	mov al, byte ptr [eax+88888888h]
	dec cl
vlineasm4_load8:
	mov bl, byte ptr [eax+88888888h]
vlineasm4_store1:
	mov dword ptr [edi-140h], ebx
	jne short vlineasm4_loc_0ad
	dec dl
	jne short vlineasm4_loc_0ad
	mov eax, dword ptr [_asm2]
	mov dword ptr [eax], edx
	mov dword ptr [eax+4], esi
	mov dword ptr [eax+8], ebp
	mov dword ptr [eax+0ch], ecx
	mov eax, edi
	pop ebp
	ret

PUBLIC mvlineasm4_
mvlineasm4_:
	push ebp
	mov byte ptr ds:[mvlineasm4_xshift4+2], cl
	mov byte ptr ds:[mvlineasm4_xshift3+2], cl
	mov byte ptr ds:[mvlineasm4_xshift2+2], cl
	mov byte ptr ds:[mvlineasm4_xshift1+2], cl
	mov ebp, dword ptr [edx]
	mov dword ptr ds:[mvlineasm4_load7+2], ebp
	mov ebp, dword ptr [edx+4]
	mov dword ptr ds:[mvlineasm4_load5+2], ebp
	mov ebp, dword ptr [edx+8]
	mov dword ptr ds:[mvlineasm4_load3+2], ebp
	mov ebp, dword ptr [edx+0ch]
	mov dword ptr ds:[mvlineasm4_load1+2], ebp
	mov ebp, dword ptr [esi]
	mov dword ptr ds:[mvlineasm4_load8+2], ebp
	mov ebp, dword ptr [esi+4]
	mov dword ptr ds:[mvlineasm4_load6+2], ebp
	mov ebp, dword ptr [esi+8]
	mov dword ptr ds:[mvlineasm4_load4+2], ebp
	mov ebp, dword ptr [esi+0ch]
	mov dword ptr ds:[mvlineasm4_load2+2], ebp
	mov ebp, dword ptr [ebx]
	and ebp, 0ffffff00h
	mov dword ptr ds:[mvlineasm4_step4+2], ebp
	mov ebp, dword ptr [ebx+4]
	mov dword ptr ds:[mvlineasm4_step3+2], ebp
	mov ebp, dword ptr [ebx+8]
	mov dword ptr ds:[mvlineasm4_step2+2], ebp
	mov ebp, dword ptr [ebx+0ch]
	and ebp, 0ffffff00h
	mov dword ptr ds:[mvlineasm4_step1+2], ebp
	mov ebx, ecx
	shr ebx, 8
	mov dword ptr [_asm2], eax
	mov edx, dword ptr [eax]
	mov esi, dword ptr [eax+4]
	mov ebp, dword ptr [eax+8]
	mov ecx, dword ptr [eax+0ch]
	mov cl, bl
	inc cl
	inc bh
	mov byte ptr [_asm3], bh
	jmp short mvlineasm4_loc_0b1
mvlineasm4_loc_0b1:
	xor dl, dl
	mov eax, ecx
mvlineasm4_xshift1:
	shr eax, 20h
mvlineasm4_step1:
	add ecx, 88888888h
mvlineasm4_load1:
	mov al, byte ptr [eax+88888888h]
	cmp al, 0ffh
	adc dl, dl
mvlineasm4_load2:
	mov bh, byte ptr [eax+88888888h]
	mov eax, ebp
mvlineasm4_xshift2:
	shr eax, 20h
mvlineasm4_step2:
	add ebp, 88888888h
mvlineasm4_load3:
	mov al, byte ptr [eax+88888888h]
	cmp al, 0ffh
	adc dl, dl
mvlineasm4_load4:
	mov bl, byte ptr [eax+88888888h]
	mov eax, esi
mvlineasm4_xshift3:
	shr eax, 20h
	shl ebx, 10h
mvlineasm4_load5:
	mov al, byte ptr [eax+88888888h]
	cmp al, 0ffh
	adc dl, dl
mvlineasm4_step3:
	add esi, 88888888h
mvlineasm4_load6:
	mov bh, byte ptr [eax+88888888h]
	mov eax, edx
mvlineasm4_xshift4:
	shr eax, 20h
mvlineasm4_step4:
	add edx, 88888888h
mvlineasm4_load7:
	mov al, byte ptr [eax+88888888h]
	cmp al, 0ffh
	adc dl, dl
mvlineasm4_load8:
	mov bl, byte ptr [eax+88888888h]
mvlineasm4_step5:
	add edi, 140h
	mov al, dl
	shl eax, 5
	add eax, OFFSET mvlineasm4_dec1
	jmp eax
mvlineasm4_loc_134:
	dec byte ptr [_asm3]
	jne near ptr mvlineasm4_loc_0b1
	mov eax, dword ptr [_asm2]
	mov dword ptr [eax], edx
	mov dword ptr [eax+4], esi
	mov dword ptr [eax+8], ebp
	mov dword ptr [eax+0ch], ecx
	mov eax, edi
	pop ebp
	ret
mvlineasm4_dec1:
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp short mvlineasm4_loc_134
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
mvlineasm4_store1:
	mov byte ptr [edi-140h], bl
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp short mvlineasm4_loc_134
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
mvlineasm4_store2:
	mov byte ptr [edi-13fh], bh
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp short mvlineasm4_loc_134
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
mvlineasm4_store3:
	mov word ptr [edi-140h], bx
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
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
	shr ebx, 10h
mvlineasm4_store4:
	mov byte ptr [edi-13eh], bl
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
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
mvlineasm4_store5:
	mov byte ptr [edi-140h], bl
	shr ebx, 10h
mvlineasm4_store6:
	mov byte ptr [edi-13eh], bl
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	nop
	shr ebx, 8
mvlineasm4_store7:
	mov word ptr [edi-13fh], bx
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
mvlineasm4_store8:
	mov word ptr [edi-140h], bx
	shr ebx, 10h
mvlineasm4_store9:
	mov byte ptr [edi-13eh], bl
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	shr ebx, 10h
mvlineasm4_store10:
	mov byte ptr [edi-13dh], bh
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
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
mvlineasm4_store11:
	mov byte ptr [edi-140h], bl
	shr ebx, 10h
mvlineasm4_store12:
	mov byte ptr [edi-13dh], bh
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	nop
mvlineasm4_store13:
	mov byte ptr [edi-13fh], bh
	shr ebx, 10h
mvlineasm4_store14:
	mov byte ptr [edi-13dh], bh
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	nop
mvlineasm4_store15:
	mov word ptr [edi-140h], bx
	shr ebx, 10h
mvlineasm4_store16:
	mov byte ptr [edi-13dh], bh
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	shr ebx, 10h
mvlineasm4_store17:
	mov word ptr [edi-13eh], bx
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
mvlineasm4_store18:
	mov byte ptr [edi-140h], bl
	shr ebx, 10h
mvlineasm4_store19:
	mov word ptr [edi-13eh], bx
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
mvlineasm4_store20:
	mov byte ptr [edi-13fh], bh
	shr ebx, 10h
mvlineasm4_store21:
	mov word ptr [edi-13eh], bx
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134
	nop
	nop
	nop
mvlineasm4_store22:
	mov dword ptr [edi-140h], ebx
	dec cl
	jne near ptr mvlineasm4_loc_0b1
	jmp near ptr mvlineasm4_loc_134

; Draw a masked sprite column, advancing its two fixed-point texture coordinates.
PUBLIC spritevline_
spritevline_:
	push ebp
	mov ebp, dword ptr [_asm1]
	mov dword ptr ds:[spritevline_step3+2], ebp
	mov ebp, eax
	shl ebp, 10h
	mov dword ptr ds:[spritevline_step2+2], ebp
	mov ebp, eax
	sar ebp, 10h
	mov dword ptr ds:[spritevline_carry1+2], ebp
	mov ebp, dword ptr [_asm3]
	mov dword ptr ds:[spritevline_step1+2], ebp
	mov ebp, dword ptr [_asm4]
	mov dword ptr ds:[spritevline_mask1+2], ebp
	mov ebp, dword ptr [_asm2]
	mov dword ptr ds:[sprite_advance+3], ebp
	mov ebp, ebx
	sar ebp, 10h
	add esi, ebp
	shl ebx, 10h
	jmp short sprite_loop
	nop
	nop
	nop
sprite_loop:
	mov al, byte ptr [esi]
spritevline_step1:
	add edx, 88888888h
	sbb ebp, ebp
spritevline_step2:
	add ebx, 88888888h
spritevline_carry1:
	adc esi, 88888888h
spritevline_mask1:
	and ebp, 88888888h
	cmp al, 0ffh
	je near ptr sprite_advance
	mov byte ptr [edi], al
sprite_advance:
	lea esi, [esi+ebp+88888888h]
spritevline_step3:
	add edi, 88888888h
	sub ecx, 1
	jae short sprite_loop
	pop ebp
	ret

; Masked and translucent horizontal spans. Their alternate entries reuse patched operands.
PUBLIC msethlineshift_
msethlineshift_:
	neg al
	mov byte ptr ds:[mhlineskipmodify_xshift1+2], al
	mov byte ptr ds:[mhlineskipmodify_yshift1+3], bl
	mov byte ptr ds:[mhlineskipmodify_xshift2+2], al
	mov byte ptr ds:[mhlineskipmodify_yshift2+3], bl
	mov byte ptr ds:[mhlineskipmodify_xshift3+2], al
	mov byte ptr ds:[mhlineskipmodify_yshift3+3], bl
	ret

PUBLIC mhline_
mhline_:
	mov dword ptr ds:[mhlineskipmodify_load4+2], eax
	mov dword ptr ds:[mhlineskipmodify_load5+2], eax
	mov dword ptr ds:[mhlineskipmodify_load1+2], eax
	mov eax, dword ptr [_asm3]
	mov dword ptr ds:[mhlineskipmodify_load3+2], eax
	mov dword ptr ds:[mhlineskipmodify_load6+2], eax
	mov dword ptr ds:[mhlineskipmodify_load7+2], eax
	mov dword ptr ds:[mhlineskipmodify_load8+2], eax
	mov dword ptr ds:[mhlineskipmodify_load2+2], eax
	mov eax, dword ptr [_asm1]
	mov dword ptr ds:[masked_loop+2], eax
	mov dword ptr ds:[mhlineskipmodify_advance1+2], eax
	mov eax, dword ptr [_asm2]
	mov dword ptr ds:[mhlineskipmodify_step1+2], eax
	mov dword ptr ds:[mhlineskipmodify_step2+2], eax
	jmp short mhlineskipmodify_
	nop
	nop
	nop

PUBLIC mhlineskipmodify_
mhlineskipmodify_:
	push ebp
	xor eax, eax
	mov ebp, ebx
	test ecx, 10000h
	jne short masked_loop
mhlineskipmodify_xshift1:
	shr ebx, 1ah
mhlineskipmodify_yshift1:
	shld ebx, esi, 6
	add ebp, dword ptr [_asm1]
mhlineskipmodify_load1:
	mov al, byte ptr [ebx+88888888h]
	add esi, dword ptr [_asm2]
	cmp al, 0ffh
	je near ptr masked_skip_first
mhlineskipmodify_load2:
	mov cl, byte ptr [eax+88888888h]
	mov byte ptr [edi], cl
masked_skip_first:
	inc edi
	sub ecx, 10000h
	jb near ptr masked_done
	jmp short masked_loop
mhlineskipmodify_loc_045:
	mov al, cl
mhlineskipmodify_load3:
	mov al, byte ptr [eax+88888888h]
	mov byte ptr [edi], al
mhlineskipmodify_loc_04f:
	add edi, 2
	sub ecx, 20000h
	jb short masked_done
masked_loop:
	lea ebx, [ebp+88888888h]
mhlineskipmodify_xshift2:
	shr ebp, 1ah
mhlineskipmodify_yshift2:
	shld ebp, esi, 6
mhlineskipmodify_step1:
	add esi, 88888888h
mhlineskipmodify_load4:
	mov cl, byte ptr [ebp+88888888h]
mhlineskipmodify_advance1:
	lea ebp, [ebx+88888888h]
mhlineskipmodify_xshift3:
	shr ebx, 1ah
mhlineskipmodify_yshift3:
	shld ebx, esi, 6
mhlineskipmodify_step2:
	add esi, 88888888h
mhlineskipmodify_load5:
	mov ch, byte ptr [ebx+88888888h]
	cmp cl, 0ffh
	je short mhlineskipmodify_loc_0b9
	cmp ch, 0ffh
	je short mhlineskipmodify_loc_045
	mov al, cl
mhlineskipmodify_load6:
	mov bl, byte ptr [eax+88888888h]
	mov al, ch
mhlineskipmodify_load7:
	mov bh, byte ptr [eax+88888888h]
	mov word ptr [edi], bx
	add edi, 2
	sub ecx, 20000h
	jae short masked_loop
	jmp short masked_done
	nop
	nop
	nop
mhlineskipmodify_loc_0b9:
	cmp ch, 0ffh
	je short mhlineskipmodify_loc_04f
	mov al, ch
mhlineskipmodify_load8:
	mov al, byte ptr [eax+88888888h]
	mov byte ptr [edi+1], al
	add edi, 2
	sub ecx, 20000h
	jae short masked_loop
masked_done:
	pop ebp
	ret

PUBLIC tsethlineshift_
tsethlineshift_:
	neg al
	mov byte ptr ds:[thlineskipmodify_xshift1+2], al
	mov byte ptr ds:[thlineskipmodify_yshift1+3], bl
	mov byte ptr ds:[thlineskipmodify_xshift2+2], al
	mov byte ptr ds:[thlineskipmodify_yshift2+3], bl
	mov byte ptr ds:[thlineskipmodify_xshift3+2], al
	mov byte ptr ds:[thlineskipmodify_yshift3+3], bl
	ret

PUBLIC thline_
thline_:
	mov dword ptr ds:[thlineskipmodify_load4+2], eax
	mov dword ptr ds:[thlineskipmodify_load5+2], eax
	mov dword ptr ds:[thlineskipmodify_load1+2], eax
	mov eax, dword ptr [_asm3]
	mov dword ptr ds:[thlineskipmodify_load6+2], eax
	mov dword ptr ds:[thlineskipmodify_load8+2], eax
	mov dword ptr ds:[thlineskipmodify_load2+2], eax
	mov eax, dword ptr [_asm1]
	mov dword ptr ds:[thlineskipmodify_loc_05a+2], eax
	mov dword ptr ds:[thlineskipmodify_advance1+2], eax
	mov eax, dword ptr [_asm2]
	mov dword ptr ds:[thlineskipmodify_step1+2], eax
	mov dword ptr ds:[thlineskipmodify_step2+2], eax
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
	test ecx, 10000h
	jne short thlineskipmodify_loc_05a
thlineskipmodify_xshift1:
	shr ebx, 1ah
thlineskipmodify_yshift1:
	shld ebx, esi, 6
	add ebp, dword ptr [_asm1]
thlineskipmodify_load1:
	mov al, byte ptr [ebx+88888888h]
	add esi, dword ptr [_asm2]
	cmp al, 0ffh
	je near ptr translucent_skip_first
thlineskipmodify_load2:
	mov dl, byte ptr [eax+88888888h]
	mov dh, byte ptr [edi]
thlineskipmodify_load3:
	mov al, byte ptr [edx+88888888h]
	mov byte ptr [edi], al
translucent_skip_first:
	inc edi
	sub ecx, 10000h
	jb near ptr translucent_done
	jmp short thlineskipmodify_loc_05a
thlineskipmodify_loc_04f:
	add edi, 2
	sub ecx, 20000h
	jb short translucent_done
thlineskipmodify_loc_05a:
	lea ebx, [ebp+88888888h]
thlineskipmodify_xshift2:
	shr ebp, 1ah
thlineskipmodify_yshift2:
	shld ebp, esi, 6
thlineskipmodify_step1:
	add esi, 88888888h
thlineskipmodify_load4:
	mov cl, byte ptr [ebp+88888888h]
thlineskipmodify_advance1:
	lea ebp, [ebx+88888888h]
thlineskipmodify_xshift3:
	shr ebx, 1ah
thlineskipmodify_yshift3:
	shld ebx, esi, 6
thlineskipmodify_step2:
	add esi, 88888888h
thlineskipmodify_load5:
	mov ch, byte ptr [ebx+88888888h]
	cmp cx, -1
	je short thlineskipmodify_loc_04f
	mov bx, word ptr [edi]
	cmp cl, 0ffh
	je short thlineskipmodify_loc_0b1
	mov al, cl
thlineskipmodify_load6:
	mov dl, byte ptr [eax+88888888h]
	mov dh, bl
thlineskipmodify_load7:
	mov al, byte ptr [edx+88888888h]
	mov byte ptr [edi], al
	cmp ch, 0ffh
	je short thlineskipmodify_loc_0c4
thlineskipmodify_loc_0b1:
	mov al, ch
thlineskipmodify_load8:
	mov dl, byte ptr [eax+88888888h]
	mov dh, bh
thlineskipmodify_load9:
	mov al, byte ptr [edx+88888888h]
	mov byte ptr [edi+1], al
thlineskipmodify_loc_0c4:
	add edi, 2
	sub ecx, 20000h
	jae short thlineskipmodify_loc_05a
translucent_done:
	pop ebp
	ret

CODE ENDS
END
