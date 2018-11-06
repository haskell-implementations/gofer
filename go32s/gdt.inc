; This is file GDT.INC
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

; Modified for VCPI Implement by Y.Shibata Aug 5th 1991

;	History:39,1

gdt_s	struc
lim0	dw	?
base0	dw	?
base1	db	?
stype	db	?	; type, DT, DPL, present
lim1	db	?	; limit, granularity
base2	db	?
gdt_s	ends

g_zero	equ	0*8
g_gdt	equ	1*8
g_idt	equ	2*8
g_rcode	equ	3*8
g_rdata	equ	4*8
g_pcode	equ	5*8
g_pdata	equ	6*8
g_core	equ	7*8
g_acode	equ	8*8
g_adata	equ	9*8
g_ctss	equ	10*8
g_atss	equ	11*8
g_ptss	equ	12*8
g_itss	equ	13*8
g_rc32	equ	14*8
g_grdr	equ	15*8

g_vcpicode	equ	16*8	;for VCPI Call Selctor in Protect Mode
g_vcpireserve0	equ	17*8
g_vcpireserve1	equ	18*8

g_v74	equ	19*8
g_v78	equ	20*8
g_v79	equ	21*8
g_altc	equ	22*8

g_num	equ	23

jmpt	macro	sel
	db	0eah
	dw	0,sel
	endm

jmptl	macro	sel
	db	0eah
	dd	0
	dw	sel
	endm
