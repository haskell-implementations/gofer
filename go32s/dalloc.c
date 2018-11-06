/* This is file DALLOC.C */
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

/* History:22,22 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "build.h"
#include "types.h"
#include "valloc.h"
#include "dalloc.h"
#include "mono.h"

#define DA_FREE	0
#define DA_USED	1

#define MAX_DBLOCK 32760 /* 4095 * 8 -> 128 Mb */

static int dalloc_initted = 0;
static word8 map[4096];
static first_avail, last_avail;
static int dfile = -1;
static disk_used = 0;

extern int debug_mode;

static void dset(unsigned i, int b)
{
  unsigned o, m;
  o = i>>3;
  m = 1<<(i&7);
  if (b)
    map[o] |= m;
  else
    map[o] &= ~m;
}

static int dtest(unsigned i)
{
  unsigned o, m;
  o = i>>3;
  m = 1<<(i&7);
  return map[o] & m;
}

static char dfilename[80];

dalloc_init()
{
  int i;
  char *tmp;
  tmp = getenv("GO32TMP");
  if (!tmp) tmp = getenv("GCCTMP");
  if (!tmp) tmp = getenv("TMP");
  if (!tmp) tmp = getenv("TEMP");
  if (!tmp) tmp = "/";
  if ((tmp[strlen(tmp)-1] == '/') || (tmp[strlen(tmp)-1] == '\\'))
    sprintf(dfilename, "%spg%04xXXXXXX", tmp, _CS);
  else
    sprintf(dfilename, "%s/pg%04xXXXXXX", tmp, _CS);
  for (i=0; i<4096; i++)
    map[i] = 0;
  first_avail = last_avail = 0;
  dalloc_initted = 1;
}

dalloc_uninit()
{
  if (dfile == -1)
    return;
  close(dfile);
  unlink(dfilename);
}

unsigned dalloc()
{
  char buf[8];
  int i;
  unsigned pn;
  if (!dalloc_initted)
    dalloc_init();
  for (pn=first_avail; pn<=MAX_DBLOCK; pn++)
    if (dtest(pn) == DA_FREE)
    {
      dset(pn, DA_USED);
      first_avail = pn+1;
#if TOPLINEINFO
      if (pn >= last_avail)
        last_avail = pn+1;
      disk_used++;
      sprintf(buf, "%5dk", disk_used*4);
      for (i=0; i<6; i++)
        poke(screen_seg, (54+i)*2, buf[i]|0x0c00);
#endif
      return pn;
    }
  fprintf(stderr, "Fatal: out of swap space!\n");
  return 0;
}

void dfree(unsigned pn)
{
  char buf[8];
  int i;
  dset(pn, DA_FREE);
  if (pn < first_avail)
    first_avail = pn;
#if TOPLINEINFO
  disk_used--;
#if 0
  sprintf(buf, "%5dk", disk_used*4);
  for (i=0; i<6; i++)
    poke(screen_seg, (54+i)*2, buf[i]|0x0c00);
#endif
#endif
}

dwrite(word8 *buf, unsigned block)
{
  int c;
  if(dfile < 0) {
    mktemp(dfilename);
    dfile = open(dfilename, O_RDWR|O_BINARY|O_CREAT|O_TRUNC, S_IWRITE|S_IREAD);
    if (dfile < 0)
    {
fprintf(stderr, "Fatal! cannot open swap file %s\n", dfilename);
exit(1);
    }
  }
  lseek(dfile, (long)block*4096L, 0);
  c = write(dfile, buf, 4096);
  if (c < 4096)
  {
    fprintf(stderr, "Fatal! disk full writing to swap file\n");
    exit(1);
  }
}

dread(word8 *buf, unsigned block)
{
  lseek(dfile, (long)block*4096L, 0);
  read(dfile, buf, 4096);
}
