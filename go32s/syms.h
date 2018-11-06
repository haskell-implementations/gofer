/* This is file SYMS.H */
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

#ifndef _SYMS_H_
#define _SYMS_H_

void syms_init(char *fname);
void syms_list(int byval);
word32 syms_name2val(char *name);
char *syms_val2name(word32 val, word32 *delta);
char *syms_val2line(word32 val, int *lineret, int exact);

extern int undefined_symbol;

#define N_INDR  0x0a
#define	N_SETA	0x14		/* Absolute set element symbol */
#define	N_SETT	0x16		/* Text set element symbol */
#define	N_SETD	0x18		/* Data set element symbol */
#define	N_SETB	0x1A		/* Bss set element symbol */
#define N_SETV	0x1C		/* Pointer to set vector in data area.  */

#endif
