/* This is file STUB.C */
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

#include <process.h>
#include <dos.h>

unsigned _heaplen = 8192;
unsigned _stklen = 4096;

char hex[] = "0123456789abcdef";

x2s(int v, char *s)
{
  int i;
  for (i=0; i<4; i++)
  {
    s[3-i] = hex[v&15];
    v >>= 4;
  }
  s[4] = 0;
}

char emsg[] = "Cannot exec go32t.exe\r\n";

main(int argc, char **argv)
{
  char s_argc[5], s_seg[5], s_argv[5];
  int r;
  x2s(argc, s_argc);
  x2s(_DS, s_seg);
  x2s((int)argv, s_argv);
  /* must wait, as go32t reads argv/argc from our memory space */
  r = spawnlp(P_WAIT, "go32t", "go32t", "!proxy", s_argc, s_seg, s_argv, 0);
  if (r == -1) write(2, emsg, sizeof(emsg));
/*  if (r == -1) perror("Cannot exec go32t"); */
  return r;
}
