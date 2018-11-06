; This is file GRPROT.ASM
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

;	History:270,22
	title	grprot
	.386p

	include	build.inc
	include	segdefs.inc
	include tss.inc
	include gdt.inc
	include idt.inc

;
;  Memory Map (relative to 0xe0000000) :
;	00000000 - 000fffff == read/write area
;	00100000 - 001fffff == read only area
;	00200000 - 002fffff == write only area
;
;  If your board can support separate read & write mappings,
;  like the TSENG chips, then either one of two cases applies:
;  1. The read and write mappings are the same, and the rw, r,
;     and w areas all have one present bank.
;  2. The read and write mappings are different, and only the
;     r and w areas have a mapped bank.  Accesses to the rw area
;     cause a page fault and the board goes back into case #1.
;
;  If your board can't support separate read & write mappings,
;  always map the rw, r, and w areas to the same page (case #1 above)
;
;  It is up to the programmer to ensure that the program doesn't
;  read from the write only area, or write to the read only area.
;
;  This method is used because you can't use the string move instructions
;  across pages.  The "movsx" instruction causes *two* memory references,
;  potentially to different banks.  The first causes one bank to be
;  enabled, and the instruction is restarted.  The second access causes
;  the second bank to be enabled, and the instruction is *RESTARTED*.
;  This means it will attempt the *first* access again, causing it to be
;  enabled, ad infinitum.
;
;  Thus, bcopy() and memcpy() shouldn't *ever* access the rw area!

;------------------------------------------------------------------------

	start_data16

	extrn	_gr_paging_func:dword
	extrn	_graphics_pt:dword
	public	_graphics_pt_lin
_graphics_pt_lin	dd	?	; filled in by paging.c

; pointer into _graphics_pt_lin
; 0, 64, 128, . . .
; 00h, 40h, 80h, . . .
; addresses 00000h, 10000h, 20000h, . . .
cur_rw		dd	0
cur_r		dd	0
cur_w		dd	0
r_bank		db	0
w_bank		db	0

mode		db	0
 mode_none	equ	0
 mode_rw	equ	1
 mode_r_w	equ	2

	end_data16

;------------------------------------------------------------------------

	start_code16

handlers	label	word
	dw	offset handler_rw
	dw	offset handler_r
	dw	offset handler_w

	extrn	_page_fault:near
	public	graphics_fault
graphics_fault:
	cli
	mov	ax,g_core
	mov	gs,ax

	mov	ebx,cr2
	and	ebx,00f00000h
	shr	ebx,19
	jmp	cs:handlers[bx]

;------------------------------------------------------------------------

handler_rw:
	cmp	mode,mode_none
	je	h_none_rw
	cmp	mode,mode_r_w
	je	h_r_w_rw

	mov	esi,_graphics_pt_lin
	add	esi,cur_rw
	mov	ecx,16
L0:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L0
	jmp	h_none_rw

h_r_w_rw:
	mov	esi,_graphics_pt_lin
	add	esi,cur_r
	mov	ecx,16
L1:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L1
	mov	esi,_graphics_pt_lin
	add	esi,cur_w
	mov	ecx,16
L2:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L2

h_none_rw:
	mov	mode,mode_rw

	mov	eax,cr2
	shr	eax,10
	and	eax,000003c0h
	mov	cur_rw,eax

	shr	ax,6			; al is now page number
	mov	ah,al
	call	[_gr_paging_func]

	mov	esi,_graphics_pt_lin
	add	esi,cur_rw
	mov	ecx,16
L3:
	or	byte ptr gs:[esi],01h	; "present"
	add	esi,4
	loop	L3

	mov	eax,cr3
	mov	cr3,eax

	pop	eax
	iretd				; back to running program
	jmp	_page_fault

;------------------------------------------------------------------------

handler_r:
	cmp	mode,mode_none
	je	h_none_r
	cmp	mode,mode_rw
	je	h_rw_r

	mov	esi,_graphics_pt_lin
	add	esi,cur_r
	mov	ecx,16
L10:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L10
	jmp	h_none_r

h_rw_r:
	mov	esi,_graphics_pt_lin
	add	esi,cur_rw
	mov	ecx,16
L11:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L11

h_none_r:
	mov	mode,mode_r_w

	mov	eax,cr2
	shr	eax,10
	and	eax,000003c0h
	mov	cur_r,eax
	add	cur_r,1024		; 1M

	shr	ax,6			; al is now page number
	mov	r_bank,al
	mov	ah,al
	mov	al,w_bank
	call	[_gr_paging_func]

	mov	esi,_graphics_pt_lin
	add	esi,cur_r
	mov	ecx,16
L12:
	or	byte ptr gs:[esi],01h	; "present"
	add	esi,4
	loop	L12

	mov	eax,cr3
	mov	cr3,eax

	pop	eax
	iretd				; back to running program
	jmp	_page_fault

;------------------------------------------------------------------------

handler_w:
	cmp	mode,mode_none
	je	h_none_w
	cmp	mode,mode_rw
	je	h_rw_w

	mov	esi,_graphics_pt_lin
	add	esi,cur_w
	mov	ecx,16
L20:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L20
	jmp	h_none_w

h_rw_w:
	mov	esi,_graphics_pt_lin
	add	esi,cur_rw
	mov	ecx,16
L21:
	and	byte ptr gs:[esi],0feh	; "not present"
	add	esi,4
	loop	L21

h_none_w:
	mov	mode,mode_r_w

	mov	eax,cr2
	shr	eax,10
	and	eax,000003c0h
	mov	cur_w,eax
	add	cur_w,2048		; 1M

	shr	ax,6			; al is now page number
	mov	w_bank,al
	mov	ah,r_bank
	call	[_gr_paging_func]

	mov	esi,_graphics_pt_lin
	add	esi,cur_w
	mov	ecx,16
L23:
	or	byte ptr gs:[esi],01h	; "present"
	add	esi,4
	loop	L23

	mov	eax,cr3
	mov	cr3,eax

	pop	eax
	iretd				; back to running program
	jmp	_page_fault

;------------------------------------------------------------------------

	end_code16

;------------------------------------------------------------------------

	end
