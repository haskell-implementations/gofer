/* This is file NPX.H */
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

#ifndef _NPX_H_
#define _NPX_H_

typedef struct {
  word16 sig0;
  word16 sig1;
  word16 sig2;
  word16 sig3;
  word16 exponent:15;
  word16 sign:1;
} NPXREG;

typedef struct {
  word32 control;
  word32 status;
  word32 tag;
  word32 eip;
  word32 cs;
  word32 dataptr;
  word32 datasel;
  NPXREG reg[8];
} NPX;

extern NPX npx;

#endif
