; This is file DOUTILS.ASM
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

;	History:14,1
	title	du_utils
	.386p

	include build.inc
	include	segdefs.inc
	include tss.inc
	include gdt.inc
	include idt.inc

;------------------------------------------------------------------------

	start_data16

	end_data16

;------------------------------------------------------------------------

	start_code16

	extrn	_fclex:near

	public	__do_memset32
__do_memset32:
	push	cx
	shr	cx,2
	jcxz	nodset
	db	67h		; so EDI is used
	rep	stosd
nodset:
	pop	cx
	and	cx,3
	jcxz	nobset
	db	67h		; so EDI is used
	rep	stosb
nobset:
	jmpt	g_ctss

;------------------------------------------------------------------------

	public	__do_memmov32
__do_memmov32:
	push	cx
	shr	cx,2
	jcxz	nodmove
	db	67h		; so ESI,EDI is used
	rep	movsd
nodmove:
	pop	cx
	and	cx,3
	jcxz	nobmove
	db	67h		; so ESI,EDI is used
	rep	movsb
nobmove:
	jmpt	g_ctss


;------------------------------------------------------------------------

	end_code16

;------------------------------------------------------------------------

	start_code32

	end_code32

;------------------------------------------------------------------------

	end
