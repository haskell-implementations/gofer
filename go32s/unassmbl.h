/* This is file UNASSMBL.H */
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

#ifndef _UNASSMBL_H_
#define _UNASSMBL_H_

word32 unassemble(word32 v, int showregs);
extern int last_unassemble_unconditional;
extern int last_unassemble_jump;
extern int last_unassemble_extra_lines;

#endif