; This is file TABLES.ASM
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

;	History:20,1
	title	tables
	.386p

	include	build.inc
	include	segdefs.inc
	include tss.inc
	include gdt.inc
	include idt.inc

;------------------------------------------------------------------------

	start_data16

	extrn	_tss_ptr:word
	extrn	_was_exception:word
	extrn	_npx:byte

	public	_was_user_int
_was_user_int	dw	0

	public	ivec_number
ivec_number	db	?

has_error	db	0,0,0,0,0,0,0,0,1,0,1,1,1,1,1,0,0

	public	_vector_78h
_vector_78h	db	?
	public	_vector_79h
_vector_79h	db	?

	end_data16

;------------------------------------------------------------------------

	.286c
	start_bss

	public	_gdt
_gdt	label	gdt_s
	db	type gdt_s * g_num dup (?)

	public	_idt
_idt	label	idt_s
	db	type idt_s * 256 dup (?)

	public	_i_tss
_i_tss	label	tss_s
	db	type tss_s dup (?)

	public	_v74_tss
_v74_tss	label	tss_s
	db	type tss_s dup (?)

	public	_v78_tss
_v78_tss	label	tss_s
	db	type tss_s dup (?)

	public	_v79_tss
_v79_tss	label	tss_s
	db	type tss_s dup (?)

	end_bss
	.386p

;------------------------------------------------------------------------

	start_altcode16

sound	macro
	local	foo
	mov	al,033h
	out	061h,al
	mov	cx,0ffffh
foo:
	loop	foo
	endm

ivec	macro	n
	push	ds
	push	g_rdata
	pop	ds
	mov	ivec_number,n
	pop	ds
	db	0eah
	dw	0, g_itss
	endm

	public	_ivec0, _ivec1
_ivec0:
	ivec 0
_ivec1:
	x=1
	rept 255
	 ivec x
	 x=x+1
	endm

	end_altcode16
	start_code16

	public	_interrupt_common
_interrupt_common:
	cli
	mov	bx,g_rdata
	mov	ds,bx
	mov	bx,_tss_ptr
	mov	al,ivec_number
	mov	[bx].tss_irqn,al
	mov	esi,[bx].tss_esp
	mov	fs,[bx].tss_ss		; fs:esi -> stack
	cmp	al,16
	ja	has_no_error
	mov	ah,0
	mov	di,ax
	cmp	has_error[di],0
	je	has_no_error
	mov	eax,fs:[esi]
	mov	[bx].tss_error,eax
	add	esi,4
has_no_error:
	mov	eax,fs:[esi]		; eip
	mov	[bx].tss_eip,eax
	mov	eax,fs:[esi+4]
	mov	[bx].tss_cs,ax
	mov	eax,fs:[esi+8]
	mov	[bx].tss_eflags,eax
	add	esi,12
	mov	[bx].tss_esp,esi	; store corrected stack pointer
	mov	_was_exception,1
	jmpt	g_ctss			; pass control back to real mode
	cli
	jmp	_interrupt_common	; it's a task

	extrn	graphics_fault:near

	public	_page_fault
_page_fault:
	cli
	mov	bx,g_rdata
	mov	ds,bx
	mov	es,bx
	mov	eax,cr2
if 1
	cmp	eax,0e0000000h
	jb	regular_pf
	cmp	eax,0e0300000h
	ja	regular_pf
	jmp	graphics_fault
regular_pf:
endif
	mov	bx,_tss_ptr
	mov	[bx].tss_irqn,14	; page fault
	mov	[bx].tss_cr2,eax
	pop	eax
	mov	[bx].tss_error,eax
	mov	eax,0
	mov	cr2,eax			; so we can tell INT 0D from page fault
	mov	_was_exception,1
	jmpt	g_ctss
	jmp	_page_fault

v_handler	macro	label,vector
	public	label
label:
	cli
	mov	bx,g_rdata
	mov	ds,bx
	mov	es,bx
	mov	bx,_tss_ptr
	mov	al,vector
	mov	[bx].tss_irqn,al
	mov	_was_exception,1
	jmpt	g_ctss
	jmp	label
	endm

	v_handler	_v74_handler, 74h
	v_handler	_v78_handler, _vector_78h
	v_handler	_v79_handler, _vector_79h

	end_code16

;------------------------------------------------------------------------

	end
