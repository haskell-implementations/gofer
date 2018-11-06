/* This is file VALLOC.C */
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
/* History:126,1 */

/*
Note: The functions here assume that memory is never really freed, just
reused through page_out, unless we're exiting or running another
program.  In either case, we don't need to keep track of what's freed
since it's all going anyway.  Thus, vfree() is empty and the rest of the
routines are relatively simple.  Note that VCPI may blow you up if you
try to be clever in here. 
*/

#include <stdio.h>
#include <dos.h>

#include "build.h"
#include "types.h"
#include "valloc.h"
#include "xms.h"
#include "mono.h"
#include "vcpi.h"

#define VA_FREE	0
#define VA_USED	1

#define	DOS_PAGE 256		/*  1MB / 4KB = 256 Pages  */

int valloc_initted = 0;
static word8 map[4096];		/* Expanded/Extended paged, allocated with VA_1M */

word32 mem_avail, mem_used;	/* Kbytes */

static unsigned pn_lo_first, pn_lo_last, pn_hi_first, pn_hi_last;
static unsigned pn_lo_next, pn_hi_next;
static unsigned vcpi_flush_ok = 0;

extern int debug_mode;
extern word16 vcpi_installed;	/* If VCPI is installed, set this not Zero  */
extern word32 far *vcpi_pt;

#if TOPLINEINFO
valloc_update_status()
{
  char buf[20];
  int i;
  if (!valloc_initted)
    return;
  sprintf(buf, "%6ldk", mem_avail);
  for (i=0; i<7; i++)
    poke(screen_seg, (i+70)*2, buf[i] | 0x0a00);
  sprintf(buf, "%6ldk", mem_used);
  for (i=0; i<7; i++)
    poke(screen_seg, (i+62)*2, buf[i] | 0x0a00);
}
#endif

emb_handle_t emb_handle=-1;

void
xms_free(void) {
	if(use_xms && emb_handle != -1) {
		xms_unlock_emb(emb_handle);
		xms_emb_free(emb_handle);
#if DEBUGGER
	printf("XMS memory freed\n");
#endif
	}
}

void
xms_alloc_init(void) {
	xms_extended_info *x = xms_query_extended_memory();
	emb_off_t linear_base;
	emb_size_K_t emb_size;
#if DEBUGGER
	printf("XMS driver detected\n");
#endif
	emb_size = x->max_free_block;
	emb_handle = xms_emb_allocate(emb_size);
	linear_base = xms_lock_emb(emb_handle);
	pn_hi_first = (linear_base + 4095)/4096;
	pn_hi_last = (linear_base + emb_size * 1024L)/4096 - 1;
}


static int valloc_lowmem_page;

static valloc_init()
{
  word32 left_lo, left_hi;
  unsigned char far *vdisk;
  int has_vdisk=1;
  unsigned long vdisk_top;
  unsigned los, i, lol;
  struct REGPACK r;
  
  if (vcpi_installed)
  {
    pn_hi_first = 0;
    pn_hi_last  = vcpi_maxpage();
  }
  else if (use_xms)
  {
    xms_alloc_init();	/*  Try XMS allocation  */
  }
  else
  {
    /*
    ** int 15/vdisk memory allocation
    */
    r.r_ax = 0x8800;	/* get extended memory size */
    intr(0x15, &r);
    pn_hi_last = r.r_ax / 4 + 255;

    /* get ivec 19h, seg only */
    vdisk = (unsigned char far *)(*(long far *)0x64L & 0xFFFF0000L);
    for (i=0; i<5; i++)
    if (vdisk[i+18] != "VDISK"[i])
      has_vdisk = 0;
    if (has_vdisk)
    {
      pn_hi_first = ( (vdisk[46]<<4) | (vdisk[45]>>4) );
      if (vdisk[44] | (vdisk[45]&0xf))
        pn_hi_first ++;
    }
    else
      pn_hi_first = 256;
  }

  r.r_ax= 0x4800;	/* get real memory size */
  r.r_bx = 0xffff;
  intr(0x21, &r);	/* lol == size of largest free memory block */

#ifdef SUPPORT_UDI					/* UDIONLY */
  if (r.r_bx > 1024)					/* UDIONLY */
    r.r_bx -= 1024;	/* reserve space for montip */	/* UDIONLY */
#endif							/* UDIONLY */

  lol = r.r_bx;
  r.r_ax = 0x4800;
  intr(0x21, &r);	/* get the block */
  pn_lo_first = (r.r_ax+0xFF) >> 8;	/* lowest real mem 4K block */
  pn_lo_last = ((r.r_ax+lol-0x100)>>8);	/* highest real mem 4K block */

  valloc_lowmem_page = r.r_ax;

  pn_lo_next = pn_lo_first;
  pn_hi_next = pn_hi_first;

  memset(map, 0, 4096);
  vcpi_flush_ok = 1;

  mem_used = 0;
  left_lo  = (pn_lo_last - pn_lo_first + 1)*4;
  left_hi  = (vcpi_installed)? vcpi_capacity()*4:(pn_hi_last-pn_hi_first+1)*4;
  mem_avail = left_lo + left_hi;

#if DEBUGGER
  if (debug_mode)
    printf("%ld Kb conventional, %ld Kb extended - %ld Kb total RAM available\n",
      left_lo, left_hi, mem_avail);
#endif

#if TOPLINEINFO
  valloc_update_status();
#endif

  valloc_initted = 1;
}

void valloc_uninit()
{
  struct REGPACK r;

  r.r_es = valloc_lowmem_page;	/* free the block we allocated */
  r.r_ax = 0x4900;
  intr(0x21, &r);

  xms_free();
  valloc_initted = 0;
}

unsigned valloc(where)
{
  unsigned pn;
  if (!valloc_initted)
    valloc_init();
  switch (where)
  {
    case VA_640:
      if (pn_lo_next <= pn_lo_last)
      {
        mem_avail -= 4;
        mem_used += 4;
#if TOPLINEINFO
        valloc_update_status();
#endif
        return pn_lo_next++;
      }

      pn = page_out(VA_640);
      if (pn != 0xffff)
      {
        return pn;
      }
      fprintf(stderr, "Error: out of conventional memory\n");
      exit(1);
    case VA_1M:
      if (vcpi_installed)
      {
	if ((pn = vcpi_alloc()) != 0)
	{
          mem_avail -= 4;
          mem_used += 4;
#if TOPLINEINFO
          valloc_update_status();
#endif
	  map[pn>>3] |= 1 << (pn&7);
	  return pn;
	}
      }
      else
      {
        if (pn_hi_next <= pn_hi_last)
        {
          mem_avail -= 4;
          mem_used += 4;
#if TOPLINEINFO
          valloc_update_status();
#endif
	  return pn_hi_next++;
	}
      }
      if (pn_lo_last-pn_lo_next > 4) /* save last four for VA_640 */
      {
        mem_avail -= 4;
        mem_used += 4;
#if TOPLINEINFO
        valloc_update_status();
#endif
        return vcpi_pt[pn_lo_next++] >> 12;
      }

      pn = page_out(VA_1M);
      if (pn != 0xffff)
      {
        return pn;
      }
      fprintf(stderr, "Error: out of extended memory\n");
      exit(1);
  }
  return 0;
}

void vfree()
{
  mem_avail += 4;
  mem_used -= 4;
#if TOPLINEINFO
  valloc_update_status();
#endif
}

void vcpi_flush(void)		/* only called on exit */
{
  word16 pn;

  if (!vcpi_flush_ok || !vcpi_installed)
    return;			/*  Not Initaialized Map[]  */
  for(pn = pn_hi_first; pn <= pn_hi_last; pn++)
    if (map[pn>>3] & (1 << (pn&7)))
      vcpi_free(pn);
}
