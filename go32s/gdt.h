/* This is file GDT.H */
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

/* Modified for VCPI Implement by Y.Shibata Aug 5th 1991 */
/* History:28,1 */

#include "types.h"

typedef struct GDT_S {
word16 lim0;
word16 base0;
word8 base1;
word8 stype;	/* type, DT, DPL, present */
word8 lim1;	/* limit, granularity */
word8 base2;
} GDT_S;

#define g_zero		0
#define g_gdt		1
#define g_idt		2
#define g_rcode		3
#define g_rdata		4
#define g_pcode		5
#define g_pdata		6
#define g_core		7
#define g_acode		8
#define g_adata		9
#define g_ctss		10
#define g_atss		11	/* set according to tss_ptr in go32() */
#define g_ptss		12
#define g_itss		13
#define g_rc32		14	/* for NPX utils */
#define g_grdr		15

#define	g_vcpicode	16	/* for VCPI Call Selector in Protect Mode */
#define	g_vcpireserve0	17
#define	g_vcpireserve1	18

#define g_v74		19
#define g_v78		20
#define g_v79		21
#define g_altc		22

#define g_num		23

extern GDT_S gdt[g_num];

#define ARENA 0x10000000L
