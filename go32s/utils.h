/* This is file UTILS.H */
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

#ifndef _UTILS_H_
#define _UTILS_H_

zero32(word32 vaddr);
memput(word32 vaddr, void far *ptr, word16 len);
memget(word32 vaddr, void far *ptr, word16 len);

word32 peek32(word32 vaddr);
word16 peek16(word32 vaddr);
word8 peek8(word32 vaddr);
poke32(word32 vaddr, word32 v);
poke16(word32 vaddr, word16 v);
poke8(word32 vaddr, word8 v);

_do_save_npx();
_do_load_npx();

#if DEBUGGER
extern int use_ansi;
#endif

#endif
