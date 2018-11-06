;This is file VCPI.ASM
;
;Copyright(C) Aug 5th 1991 Y.Shibata
;This file is distributed under the term of GNU GPL.
;
	.386P

	include build.inc
	include segdefs.inc
	include vcpi.inc

	start_data16
emm_name	db	"EMMXXXX0",0
	end_data16

	start_code16
;
;  EMM Installed Check
;
;word16	emm_present(void)
;
;result EMS Handle
;
	public	_emm_present
_emm_present	proc	near
	;
	push	bp
	mov	bp,sp
	;
	mov	dx,offset emm_name
	mov	ax,3d00H		;Open Handle
	int	DOS_REQ
	jc	short no_emm_driver
	mov	bx,ax
	mov	ax,4400H		;Get IOCTL Data	
	int	DOS_REQ
	jc	short no_emm_driver
	test	dx,80H			;1 = Device , 0 = File
	jz	short no_emm_driver
	mov	ax,4407H		;Get Output IOCTL Status
	int	DOS_REQ
	push	ax
	mov	ah,3EH			;Close Handle
	int	DOS_REQ
	pop	ax
	cmp	al,-1			;Ready?
	jne	short no_emm_driver
	;
	mov	ah,40H			;Get Status
	int	EMS_REQ
	cmp	ah,0
	jne	short no_emm_driver
	mov	ah,42H			;Get Unallocate Page Count
	int	EMS_REQ
	cmp	ah,0
	jne	short no_emm_driver
	mov	ax,-1			;Handle = -1(Invalid Handle No.)
	cmp	bx,dx			;Other Program EMS Page Used?
	jne	short emm_present_end	;Used!!
	mov	bx,1
	mov	ah,43H			;Allocate Pages(1 Page Only)
	int	EMS_REQ
	cmp	ah,0
	jne	short no_emm_driver
	mov	ax,dx			;Handle
	jmp	short emm_present_end

no_emm_driver:
	xor	ax,ax			;Not Installed = 0
emm_present_end:
	pop	bp
	ret
	;
_emm_present	endp

;
;  EMS Page Deallocated
;
;void ems_free(word16 ems_handle)
;
	public	_ems_free
_ems_free	proc	near
	;
	push	bp
	mov	bp,sp
	;
	mov	dx,4[bp]		;EMS_Handle
	mov	ah,45H			;Deallocate Pages
	int	EMS_REQ
	;
	pop	bp
	ret
	;
_ems_free	endp

;
;  VCPI Installed Check
;
;word16	vcpi_present(void)
;
;result	-1:VCPI Installed 0:VCPI Not Installed
;
	public	_vcpi_present
_vcpi_present	proc	near
	;
	push	bp
	mov	bp,sp
	;
	mov	ax,VCPI_PRESENT		;VCPI Present
	int	VCPI_REQ
	sub	ah,1
	sbb	ax,ax			;ah = 0 -> AX = -1
	;
	pop	bp
	ret
	;
_vcpi_present	endp

;
;  VCPI Maximum page number
;
;word16	vcpi_maxpage(void)
;
;result	max returnable page number
;
	public	_vcpi_maxpage
_vcpi_maxpage	proc	near
	push	bp
	mov	bp,sp

	mov	ax,VCPI_MAX_PHYMEMADR
	int	VCPI_REQ
	shr	edx,12
	mov	ax,dx

	pop	bp
	ret

_vcpi_maxpage	endp

;
;  VCPI Unallocated Page count
;
;word16	vcpi_capacity(void)
;
;result	Free VCPI Memory(Pages)
;
	public	_vcpi_capacity
_vcpi_capacity	proc	near
	;
	push	bp
	mov	bp,sp
	;
	mov	ax,VCPI_MEM_CAPACITY
	int	VCPI_REQ
	mov	ax,dx			;Cut Upper16Bit(CAUTION!!)
	;
	pop	bp
	ret
	;
_vcpi_capacity	endp

;
;  VCPI Memory Allocate
;
;word16	vcpi_alloc(void)
;
;result	Allocate Page No.
;
	public	_vcpi_alloc
_vcpi_alloc	proc	near
	;
	push	bp
	mov	bp,sp
	;
	mov	ax,VCPI_ALLOC_PAGE
	int	VCPI_REQ
	test	ah,ah
	je	vcpi_alloc_success
	xor	ax,ax			;Error result = 0
	jmp	short vcpi_alloc_end
	;
vcpi_alloc_success:
	shr	edx,12
	mov	ax,dx			;Cut Upper16Bit (CAUTION!!)
vcpi_alloc_end:
	pop	bp
	ret
	;
_vcpi_alloc	endp

;
;  VCPI Memory Deallocate
;
;void	vcpi_free(word16 page_number)
;
	public	_vcpi_free
_vcpi_free	proc	near
	;
	push	bp
	mov	bp,sp
	;
	movzx	edx,word ptr 4[bp]
	sal	edx,12			;Address = Page_number * 4KB
	mov	ax,VCPI_FREE_PAGE
	int	VCPI_REQ
	;
	pop	bp
	ret
	;
_vcpi_free	endp

;
;  VCPI Get Interface
;
;word32	get_interface(word32 far *page_table,GDT_S *gdt)
;
	public	_get_interface
_get_interface	proc	near
	;
	push	bp
	mov	bp,sp
	push	si
	push	di
	;
	push	es
	mov	si,[bp+8]		;DS:SI = &GDT[g_vcpicode]
	les	di,[bp+4]		;ES:DI = Page Table (DI = 0)
	mov	ax,VCPI_INTERFACE
	int	VCPI_REQ
	mov	eax,ebx
	shld	edx,eax,16		;DX:AX = EBX
	pop	es
   	;
	pop	di
	pop	si
	pop	bp
	ret	
	;
_get_interface	endp

;
;  VCPI Get PIC Vector
;
;word16	vcpi_get_pic(void)
;
;Result MASTER PIC Vector No.(IRQ0)
;
	public	_vcpi_get_pic
_vcpi_get_pic	proc	near
	;
	push	bp
	mov	bp,sp
	mov	ax,VCPI_GET_PIC_VECTOR
	int	VCPI_REQ
	mov	ax,bx			;MASTER PIC Vector
	pop	bp
	ret
	;
_vcpi_get_pic	endp

;
;  VCPI Get PIC Vector
;
;word16	vcpi_get_secpic(void)
;
;Result SLAVE PIC Vector No.(IRQ0)
;
	public	_vcpi_get_secpic
_vcpi_get_secpic	proc	near
	;
	push	bp
	mov	bp,sp
	mov	ax,VCPI_GET_PIC_VECTOR
	int	VCPI_REQ
	mov	ax,cx			;SLAVE PIC Vector
	pop	bp
	ret
	;
_vcpi_get_secpic	endp

;
;  VCPI Set PIC Vector
;
;void	vcpi_set_pics(word16 master_pic, word16 slave_pic)
;
	public	_vcpi_set_pics
_vcpi_set_pics	proc	near
	;
	push	bp
	mov	bp,sp
	mov	bx,4[bp]		;MASTER PIC Vector
	mov	cx,6[bp]		;SLAVE PIC Vector
	mov	ax,VCPI_SET_PIC_VECTOR
	int	VCPI_REQ
	pop	bp
	ret
	;
_vcpi_set_pics	endp


	end_code16

	end
