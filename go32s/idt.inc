; This is file IDT.INC
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

;	History:1,2

idt_s		struc
offset0		dw	?
selector	dw	?
itype		dw	?
offset1		dw	?
idt_s		ends
