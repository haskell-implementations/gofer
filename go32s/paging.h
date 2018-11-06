/* This is file PAGING.H */
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

#ifndef _PAGING_H_
#define _PAGING_H_

/* active if set */
#define PT_P	0x001	/* present (else not) */
#define PT_W	0x002	/* writable (else read-only) */
#define	PT_U	0x004	/* user mode (else kernel mode) */
#define PT_A	0x020	/* accessed (else not) */
#define PT_D	0x040	/* dirty (else clean) */
#define PT_I	0x200	/* Initialized (else not read from a.out file yet) */
#define PT_S	0x400	/* Swappable (else not) */
#define	PT_C	0x800	/* Candidate for swapping */

#define EMU_TEXT 0xb0000000

/*  If not present and initialized, page is in swap file.
**  If not present and not initialized, page is in a.out file.
*/

word32 paging_brk(word32 b);
word32 paging_sbrk(int32 b);

int emu_install(char *filename); /* returns 1 if installed, 0 if not */

#endif
