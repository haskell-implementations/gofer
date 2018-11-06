; This is file SEGDEFS.INC
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

;	History:76,14

DGROUP	group	_DATA,_BSS

;------------------------------------------------------------------------

start_code16	macro
_TEXT	segment	byte public 'code' use16
	assume	cs:_TEXT,ds:DGROUP,ss:DGROUP
	endm

end_code16	macro
_TEXT	ends
	endm

_TEXT	segment	byte public 'code' use16
	assume	cs:_TEXT,ds:DGROUP,ss:DGROUP
_TEXT	ends

;------------------------------------------------------------------------

start_altcode16	macro
_ALTTEXT	segment	byte public 'code' use16
	assume	cs:_ALTTEXT,ds:DGROUP,ss:DGROUP
	endm

end_altcode16	macro
_ALTTEXT	ends
	endm

_ALTTEXT	segment	byte public 'code' use16
	assume	cs:_ALTTEXT,ds:DGROUP,ss:DGROUP
_ALTTEXT	ends

;------------------------------------------------------------------------

start_code32	macro
code32	segment	public para 'code32' use32
	assume	cs:code32,ds:DGROUP,ss:DGROUP
	endm

end_code32	macro
code32	ends
	endm

code32	segment	public para 'code32' use32
	assume	cs:code32,ds:DGROUP,ss:DGROUP
code32	ends

;------------------------------------------------------------------------

start_data32	macro
data32	segment	public para 'data32' use32
	endm

end_data32	macro
data32	ends
	endm

data32	segment	public para 'data32' use32
data32	ends

;------------------------------------------------------------------------

start_data16	macro
_DATA	segment para public 'DATA' use16
	endm

end_data16	macro
_DATA	ends
	endm

_DATA	segment para public 'DATA' use16
_DATA	ends

;------------------------------------------------------------------------

	.286c

start_bss	macro
_BSS	segment	word public 'BSS'
	endm

end_bss	macro
_BSS	ends
	endm

_BSS	segment	word public 'BSS'
_BSS	ends

	.386p

;------------------------------------------------------------------------

call16	macro	s	; from a 32-bit segment
	db	09ah
	dd	offset s
	dw	seg s
	endm

call32	macro	s	; from a 16-bit segment
	db	09ah
	dw	offset s
	dw	seg s
	endm
