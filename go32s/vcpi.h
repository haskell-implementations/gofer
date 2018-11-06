/*  This is file VCPI.H  */
/*
**  Copyright (C) Aug 5th 1991 Y.Shibata
**  This file is distributed under the term of GNU GPL.
**
*/
#ifndef _VCPI_H_
#define _VCPI_H_

/*  Change Protect Mode Structure  */
typedef	struct
	{
	word32	page_table;	/*  Page Table Address  */
	word32	gdt_address;	/*  GDT Address         */
	word32	idt_address;	/*  IDT Address         */
	word16	ldt_selector;	/*  LDT Selector        */
	word16	tss_selector;	/*  TR  Selector        */
	word32	entry_eip;	/*  Protect Mode Entry Address  */
	word16	entry_cs;
	}	CLIENT;

typedef	struct
	{
	word32	offset32;
	word16	selector;
	}	far32;

typedef	struct
	{
	word16	limit_16;
	word32	base_32;
	}	SYS_TBL;

word16	vcpi_present(void);	/*  VCPI Installed Check         */
word16	vcpi_maxpage(void);	/*  VCPI Max Page Number	 */
word16	vcpi_capacity(void);	/*  VCPI Unallocated Page Count  */
word16	vcpi_alloc(void);	/*  VCPI Allocate Page           */
void	vcpi_free(word16);	/*  VCPI Deallocate Pgae         */
word16	vcpi_get_pic(void);	/*  VCPI Get 8259A INT Vector    */
void	vcpi_set_pic(word16);	/*  VCPI Set 8259A INT Vector    */

word32	get_interface(void far *table,void *g);

word16	emm_present(void);	/*  EMM Installed Check          */
void	emm_free(word16);	/*  Deallocated EMS Pages        */

#endif
