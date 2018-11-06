; This is file NPX.ASM
;
; Copyright (C) 1991 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
;
; This file is distributed under the terms listed in the document
; "copying.dj", available from DJ Delorie at the address above.
; A copy of "copying.dj" should accompany this file; if not, a copy
; should be available from where this file was obtained.  This file
; may not be distributed without a verbatim copy of "copying.dj".
;
; This file is distributed WITHOUT ANY WARRANTY; without even the implied
; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;

;	History:69,29
	title	tables
	.386p

	include build.inc
	include	segdefs.inc
	include tss.inc
	include gdt.inc
	include idt.inc

son	macro
	mov	al,33h
	out	61h,al
	endm

soff	macro
	mov	al,30h
	out	61h,al
	endm

;------------------------------------------------------------------------

	start_data16

	extrn	_npx:byte
	extrn	ivec_number:byte

	public	_npx_stored
_npx_stored	db	0		; 0=in 387, 1=in _npx

temp87	dw	1 dup (?)

	end_data16

;------------------------------------------------------------------------

	start_code16

;------------------------------------------------------------------------

	public	_detect_80387		; direct from the Intel manual
_detect_80387:				; returns 1 if 387, else 0
	push	si
	fninit
	mov	si,offset dgroup:temp87
	mov	word ptr [si],5a5ah
	fnstsw	[si]
	cmp	byte ptr [si],0
	jne	no_387

	fnstcw	[si]
	mov	ax,[si]
	and	ax,103fh
	cmp	ax,3fh
	jne	no_387

	fld1
	fldz
	fdiv
	fld	st
	fchs
	fcompp
	fstsw	[si]
	mov	ax,[si]
	sahf
	je	no_387
	fninit			; 387 present, initialize.
	fnstcw	temp87
	wait
	and	temp87,0fffah	; enable invalid operation exception
	fldcw	temp87
	mov	ax,1
	pop	si
	ret

no_387:
	mov	ax,0
	pop	si
	ret

;------------------------------------------------------------------------

	public	_ivec7
_ivec7:
	push	eax
	mov	eax,cr0
	and	eax,0FFFFFFF3h
	mov	cr0,eax
	pop	eax
	iretd

	public	_fclex		; for either mode
_fclex:
	in	al,0a0h
	test	al,20h
	jz	fclex_ret
	or	ax,ax
	out	0f0h,al
	mov	al,20h
	out	0a0h,al
	out	20h,al
fclex_ret:
	ret

	public	_ivec75
_ivec75:
	push	ax
	push	ds

	push	g_core
	pop	ds
	inc	word ptr ds:[0b8004h]

	push	g_rdata
	pop	ds
	mov	_npx_stored,1
	or	ax,ax
	out	0f0h,al
	mov	al,20h
	out	0a0h,al
	out	20h,al
;	fnsave	ds:[_npx]
	db	66h,67h,0ddh,35h
	dd	offset _npx
	fwait

	mov	ivec_number,75h
	pop	ds
	pop	ax
	db	0eah
	dw	0, g_itss

;------------------------------------------------------------------------

	public	__do_save_npx
__do_save_npx:
	cmp	_npx_stored,0
	jne	fpsave_ret
	call	_fclex
	mov	_npx_stored,1
;	fnsave	ds:[_npx]
	db	66h,67h,0ddh,35h
	dd	offset _npx
	fwait
fpsave_ret:
	jmpt	g_ctss

	public	__do_load_npx		; from *protected* mode
__do_load_npx:
	cmp	_npx_stored,0
	je	no_load_npx
	call	_fclex
	mov	_npx_stored,0
	mov	_npx[4],0		; clear pending exceptions
;	frstor	[_npx]
	db	66h,67h,0ddh,25h
	dd	offset _npx
no_load_npx:
	ret

;------------------------------------------------------------------------

	end_code16

;------------------------------------------------------------------------

	start_code32

	end_code32

;------------------------------------------------------------------------

	end
