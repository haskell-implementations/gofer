;;;
;;; EVINTR.ASM
;;;  Interrupt handlers for an interrupt-driven mouse and keyboard event queue
;;;  mechanism for Turbo C and DJGPP (under MS DOS of course)
;;;
;;;  Copyright (C) 1992, Csaba Biegl
;;;    [820 Stirrup Dr, Nashville, TN, 37221]
;;;    [csaba@vuse.vanderbilt.edu]
;;;
;;;  This program is free software; you can redistribute it and/or modify
;;;  it under the terms of the GNU General Public License as published by
;;;  the Free Software Foundation.
;;;
;;;  This program is distributed in the hope that it will be useful,
;;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;  GNU General Public License for more details.
;;;
;;;  You should have received a copy of the GNU General Public License
;;;  along with this program; if not, write to the Free Software
;;;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;;;

_TEXT	segment byte public 'CODE'
_TEXT	ends

_TEXT	segment byte public 'CODE'
	assume  cs:_TEXT,ds:NOTHING

;;
;; mouse interrupt routine -- called by the mouse handler callback mechanism
;;
__ev_mouseint	proc	far
	pushf
	cli
	push	cx
	push	dx
	mov	cx,sp
	mov	dx,ss
	mov	ss,WORD PTR cs:__ev_interss
	mov	sp,WORD PTR cs:__ev_msintsp
	sti
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	push	ds
	mov	ds,WORD PTR cs:__ev_interds
	push	di
	push	si
	push	bx
	push	ax
	call	FAR PTR __ev_mousehandler
	add	sp,8
	pop	ds
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	cli
	mov	ss,dx
	mov	sp,cx
	pop	dx
	pop	cx
	popf
	ret
__ev_mouseint	endp

;;
;; keyboard interrupt handler -- replaces the int 9 vector
;;
__ev_keybdint	proc	far
	inc	WORD PTR cs:__ev_kbinter
	jz	kbint_proceed
	dec	WORD  PTR cs:__ev_kbinter
	jmp	DWORD PTR cs:__ev_oldkbint
kbint_proceed:
	cli
	push	cx
	push	dx
	mov	cx,sp
	mov	dx,ss
	mov	ss,WORD PTR cs:__ev_interss
	mov	sp,WORD PTR cs:__ev_kbintsp
	pushf
	call	DWORD PTR cs:__ev_oldkbint
	sti
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	push	ds
	mov	ds,WORD PTR cs:__ev_interds
	call	FAR PTR __ev_keybdhandler
	pop	ds
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	cli
	mov	ss,dx
	mov	sp,cx
	pop	dx
	pop	cx
	dec	WORD PTR cs:__ev_kbinter
	iret
__ev_keybdint	endp

__ev_oldkbint	label	dword
	db	4 dup (?)
__ev_kbinter	label	word
	db	2 dup (?)
__ev_interss	label	word
	db	2 dup (?)
__ev_kbintsp	label	word
	db	2 dup (?)
__ev_msintsp	label	word
	db	2 dup (?)
__ev_interds	label	word
	db	2 dup (?)
_TEXT	ends

	public  __ev_keybdint
	public  __ev_mouseint
	public  __ev_msintsp
	public  __ev_kbintsp
	public  __ev_interss
	public  __ev_interds
	public  __ev_kbinter
	public  __ev_oldkbint

	extrn	__ev_keybdhandler:far
	extrn	__ev_mousehandler:far

	end

