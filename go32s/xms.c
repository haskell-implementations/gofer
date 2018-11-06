/*
** xms.c -- C bindings for xms API - implemented
** Author: Kent Williams william@umaxc.weeg.uiowa.edu
*/
#pragma inline
#include <stdio.h>
#include <stdlib.h>
#include "xms.h"

static void far xms_spoof(void);
static void (far *xms_entry)(void) = xms_spoof;
char xms_error;

int
xms_installed(void) {
	asm	mov	ax,0x4300	/* get installation status */
	asm	int 0x2f		/* call driver */
	asm	mov	ah,1		/* return true if present */
	asm	cmp al,0x80		/* are you there? */
	asm	je __present
	asm	xor ax,ax		/* no, return false */
	__present:
	asm	xchg	ah,al	/* if present, set ax to 1 */
	asm	cbw
}

static void
xms_get_entry(void) {
	asm	mov	ax,0x4310	/* get driver entry */
	asm	int 0x2f		/* call multiplex */
	asm	mov word ptr xms_entry,bx
	asm	mov	word ptr (xms_entry+2),es
}

static void far
xms_spoof(void) {
	asm	push ax
	asm	push bx
	asm	push cx
	asm	push dx
	asm	push si
	asm	push di
	asm	push es
	asm	push ds
	if(!xms_installed()) {
		fputs("No XMS driver installed\n",stderr);
		exit(1);
	} else
		xms_get_entry();
	asm	pop ds
	asm	pop	es
	asm	pop	di
	asm	pop	si
	asm	pop	dx
	asm	pop cx
	asm	pop	bx
	asm	pop	ax
	xms_entry();
}

xms_version_info *
xms_get_version_info(void) {
	unsigned xms_ver,xmm_ver,hma_indicator;
	static xms_version_info x;
	asm	xor ah,ah
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	mov	xms_ver,ax
	asm	mov xmm_ver,bx
	asm	mov	hma_indicator,dx
	xms_ver = (xms_ver & 0xf) + (((xms_ver >> 4) & 0xf) * 10) +
			((xms_ver >> 8) & 0xf) * 100 + ((xms_ver >> 12) & 0xf) * 1000;

	xmm_ver = (xmm_ver & 0xf) + (((xmm_ver >> 4) & 0xf) * 10) +
			((xmm_ver >> 8) & 0xf) * 100 + ((xmm_ver >> 12) & 0xf) * 1000;
	x.xms_ver = xms_ver; x.xmm_ver = xmm_ver; x.hma_present = hma_indicator;
	return &x;
}

xms_extended_info *
xms_query_extended_memory(void) {
	unsigned max_free_block,total_extended_memory;
	static xms_extended_info x;

	asm	mov	ah,0x8
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	mov	max_free_block,ax
	asm	mov	total_extended_memory,dx
	x.max_free_block = max_free_block;
	x.total_extended_memory = total_extended_memory;
	return &x;
}

int
xms_hma_allocate(unsigned siz) {
	asm	mov	ah,0x01
	asm	mov	dx,word ptr siz
	asm	call [xms_entry]
	asm	mov	xms_error,bl
		/* xms returns 1 on success, 0 on failure.  We need to invert this
		** for the C idiom.
		** if ax = 1 then not ax = 0xfffe and add ax,2 = 0
		** if ax = 0 then not ax = 0xffff and add ax,2 = 1
		*/
	asm	not ax
	asm	inc ax
	asm	inc ax
}

int
xms_hma_free(void) {
	asm	mov	ah,0x02
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}


int
xms_global_enable_a20(void) {
	asm	mov	ah,0x03
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

int
xms_global_disable_a20(void) {
	asm	mov	ah,0x4
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

int
xms_local_enable_a20(void) {
	asm	mov	ah,0x5
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

int
xms_local_disable_a20(void) {
	asm	mov	ah,0x6
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}


int
xms_a20_status(void) {
	asm	mov	ah,0x7
	asm	call [xms_entry]
	asm	mov	xms_error,bl
}

int
xms_emb_allocate(emb_size_K_t siz) {
	asm	mov ah,0x9
	asm	mov	dx,siz
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	or	ax,ax			/* allocation succeed? */
	asm	jz alloc_failed		/* no, return 0		   */
	asm	xchg ax,dx			/* yes, return handle */
	asm	jmp done
alloc_failed:
	asm	mov	ax,-1
done:;
}

int
xms_emb_free(emb_handle_t handle) {
	asm	mov	ah,0x0a
	asm	mov dx,handle
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

emb_handle_info *
xms_get_emb_handle_info(emb_handle_t handle) {
	static emb_handle_info info;
	unsigned char locks,handles;
	emb_size_K_t siz;
	asm	mov	ah,0x0e
	asm	mov	dx,handle
	asm	call [xms_entry]
	asm	or ax,ax		/* did call fail? */
	asm	jz call_failed
	asm	mov	locks,bh
	asm	mov handles,bl
	asm	mov	siz,dx
	asm	jmp short done
	call_failed:
	asm	mov	xms_error,bl
	asm	mov ax,1
	asm	mov word ptr siz,-1
	done:
	info.lock_count = locks;
	info.handles_available = handles;
	info.block_size = siz;
	return siz == 0xffff ? NULL : &info;
}

int
xms_emb_resize(emb_handle_t handle,emb_size_K_t siz) {
	asm	mov	ah,0xf
	asm	mov	bx,siz
	asm	mov	dx,handle
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

int
xms_move_emb(xms_move_param *x) {
	xms_move_param far *x2 = (xms_move_param far *)x;
	asm	push	ds
	asm	mov	ah,0xb
	asm	lds	si,x2
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

emb_off_t
xms_lock_emb(emb_handle_t handle) {
	asm	mov	ah,0x0c
	asm	mov	dx,handle
	asm	call [xms_entry]
	asm	cmp	ax,0
	asm	je	lock_failed
	asm	mov	ax,bx		/* return value in dx:ax */
	asm	mov	xms_error,0
	asm	jmp	lock_done
lock_failed:
	asm	mov	dx,ax
	asm	mov	xms_error,bl
lock_done:
	;
}

int
xms_unlock_emb(emb_handle_t handle) {
	asm	mov	ah,0x0d
	asm	mov	dx,handle
	asm	call [xms_entry]
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

umb_info *
xms_umb_allocate(umb_size_t siz) {
	static umb_info x;
	unsigned segment,block_size,status;
	asm	mov	ah,0x10
	asm	mov	dx,siz
	asm	call [xms_entry]
	asm	mov xms_error,bl
	asm	mov segment,bx
	asm	mov	block_size,dx
	asm	or ax,ax
	asm	jnz done
	asm	mov	segment,ax
done:
	x.segment = segment;
	x.block_size = block_size;
	return segment != 0 ? &x : NULL;
}

int
xms_umb_free(umb_segment_t segment) {
	asm	mov	ah,0x11
	asm	mov	dx,segment
	asm	call [xms_entry];
	asm	mov	xms_error,bl
	asm	not ax
	asm	inc ax
	asm	inc ax
}

