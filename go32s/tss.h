/* This is file TSS.H */
/*
** Copyright (C) 1991 DJ Delorie, 24 Kirsten Ave, Rochester NH 03867-2954
**
** This file is distributed under the terms listed in the document
** "copying.dj", available from DJ Delorie at the address above.
** A copy of "copying.dj" should accompany this file; if not, a copy
** should be available from where this file was obtained.  This file
** may not be distributed without a verbatim copy of "copying.dj".
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/* History:51,52 */

typedef struct TSS {
	unsigned short tss_back_link;
	unsigned short res0;
	unsigned long  tss_esp0;
	unsigned short tss_ss0;
	unsigned short res1;
	unsigned long  tss_esp1;
	unsigned short tss_ss1;
	unsigned short res2;
	unsigned long  tss_esp2;
	unsigned short tss_ss2;
	unsigned short res3;
	unsigned long  tss_cr3;
	unsigned long  tss_eip;
	unsigned long  tss_eflags;
	unsigned long  tss_eax;
	unsigned long  tss_ecx;
	unsigned long  tss_edx;
	unsigned long  tss_ebx;
	unsigned long  tss_esp;
	unsigned long  tss_ebp;
	unsigned long  tss_esi;
	unsigned long  tss_edi;
	unsigned short tss_es;
	unsigned short res4;
	unsigned short tss_cs;
	unsigned short res5;
	unsigned short tss_ss;
	unsigned short res6;
	unsigned short tss_ds;
	unsigned short res7;
	unsigned short tss_fs;
	unsigned short res8;
	unsigned short tss_gs;
	unsigned short res9;
	unsigned short tss_ldt;
	unsigned short res10;
	unsigned short tss_trap;
	unsigned short tss_iomap;
	unsigned long  tss_cr2;
	unsigned long  tss_error;
	unsigned char  tss_irqn;
	unsigned char  res11;
	unsigned short res12;
	unsigned short stack0[512];
	unsigned short tss_stack[1];
} TSS;

extern TSS c_tss, a_tss, o_tss, p_tss, i_tss, f_tss;
extern TSS v74_tss, v78_tss, v79_tss;
extern TSS *tss_ptr;
