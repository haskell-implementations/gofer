/* This is file VALLOC.H */
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

#ifndef _VALLOC_H_
#define _VALLOC_H_

#define VA_640	0  /* always deal with logical page numbers 0..255 */
#define VA_1M	1  /* always deal with physical page numbers */

/*
** These functions deal with page *numbers*
**  pn << 8  = segment number
**  pn << 12 = physical address
**  pn << 24 = seg:ofs
*/

void valloc_uninit();

unsigned valloc(int where);
void vfree();

extern int use_xms;

#endif
