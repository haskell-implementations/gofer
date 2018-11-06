/* This is file GRAPHICS.C */
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

#pragma inline

/* History:42,23 */
#include <dos.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "build.h"
#include "types.h"
#include "paging.h"
#include "graphics.h"
#include "tss.h"
#include "gdt.h"

int gr_def_tw = 0;
int gr_def_th = 0;
int gr_def_gw = 0;
int gr_def_gh = 0;

far (*gr_init_func)();
unsigned gr_paging_offset;
unsigned gr_paging_segment;
word32 gr_paging_func;

#ifdef NONEWDRIVER

typedef struct {
  word16 init_routine;
  word16 paging_routine;
  word16 split_rw;
  word16 def_tw;
  word16 def_th;
  word16 def_gw;
  word16 def_gh;
} GR_DRIVER;

#else

#include <stdio.h>
#include "driver.h"

int gr_def_numcolor = 0;
static char new_driver = 0;
static char mult_pages = 0;

#define  init_routine   old.modeset_routine
#define  paging_routine old.paging_routine
#define  split_rw       old.driver_flags
#define  def_tw         old.def_tw
#define  def_th         old.def_th
#define  def_gw         old.def_gw
#define  def_gh         old.def_gh

#endif

extern GR_DRIVER builtin_gr_driver;
GR_DRIVER *gr_driver = NULL;

#if !DEBUGGER
char *drv_name = NULL;

void setup_graphics_driver(char *name)
{
  gr_paging_segment = _DS;
  gr_paging_offset = builtin_gr_driver.paging_routine;
  gr_paging_func = ((word32)g_grdr << 19) + gr_paging_offset;
  if(name != NULL) {
    drv_name = (char *)malloc(strlen(name) + 1);
    if(drv_name != NULL) strcpy(drv_name,name);
  }
}

extern fillgdt(int sel, word32 limit, word32 base, word8 type, int G);

static void init_graphics_driver(void)
#else
void setup_graphics_driver(char *drv_name)
#endif
{
  int file;
  struct stat sbuf;
  if ((drv_name == NULL) || stat(drv_name, &sbuf))
  {
    gr_driver = &builtin_gr_driver;
  }
  else
  {
    gr_driver = (GR_DRIVER *)malloc(sbuf.st_size + 16);
    if (gr_driver == 0)
    {
      gr_driver = &builtin_gr_driver;
    }
    else
    {
      gr_driver = (GR_DRIVER *)(((unsigned)gr_driver + 15) & ~15);
      file = open(drv_name, O_RDONLY | O_BINARY);
      read(file, gr_driver, sbuf.st_size);
      close(file);
    }
  }
  if (gr_driver == &builtin_gr_driver)
  {
    gr_paging_segment = _DS;
  }
  else
  {
    gr_paging_segment = _DS + (unsigned)gr_driver/16;
  }
  gr_init_func = MK_FP(gr_paging_segment, gr_driver->init_routine);
  gr_paging_offset = gr_driver->paging_routine;
  gr_paging_func = ((word32)g_grdr << 19) + gr_paging_offset;
#if !DEBUGGER
  if(drv_name != NULL) free(drv_name);
  fillgdt(g_grdr, 0xffff, (word32)gr_paging_segment*16L, 0x9a, 0);
#endif

  if (gr_def_tw) gr_driver->def_tw = gr_def_tw;
  if (gr_def_th) gr_driver->def_th = gr_def_th;
  if (gr_def_gw) gr_driver->def_gw = gr_def_gw;
  if (gr_def_gh) gr_driver->def_gh = gr_def_gh;
#ifndef NONEWDRIVER
  if(gr_driver->new.driver_flags & GRD_NEW_DRIVER) {
    void far (*init_fun)(void);

    new_driver = 1;
    if(gr_driver->new.driver_flags & GRD_MULTPAGES)
      mult_pages = 1;
    if(gr_def_numcolor)
      gr_driver->new.def_numcolor = gr_def_numcolor;
    init_fun = MK_FP(gr_paging_segment,gr_driver->new.driver_init_routine);
    asm push  ds;
    asm mov   ds,word ptr gr_paging_segment;
    asm call  dword ptr ss:[init_fun];
    asm pop   ds;
    if(_AX == 0) {
      /* You may want to do something more appropriate here */
      fputs("Graphics initialization error -- probably incorrect driver\n",stderr);
      /* exit(1); */
    }
  }
#endif
}

void graphics_mode(int ax)
{
  int bx, cx, dx;
#if !DEBUGGER
  if(!gr_driver) init_graphics_driver();
#endif
  bx = tss_ptr->tss_ebx;
  cx = tss_ptr->tss_ecx;
  dx = tss_ptr->tss_edx;
  _AX = ax;
  _BX = bx;
  _CX = cx;
  _DX = dx;
  asm push ds
  asm push ds
  asm pop es
  asm push word ptr gr_init_func+2
  asm pop ds
  asm call dword ptr es:[gr_init_func]
  asm pop ds
  dx = _DX;
  cx = _CX;
#ifndef NONEWDRIVER
  bx = _BX;
  if(!new_driver)
#endif
  bx = gr_driver->split_rw;
  tss_ptr->tss_ebx = bx;
  tss_ptr->tss_ecx = cx;
  tss_ptr->tss_edx = dx;
}

#ifndef NONEWDRIVER
void graphics_inquiry(void)
{
#if !DEBUGGER
  if(!gr_driver) init_graphics_driver();
#endif
  tss_ptr->tss_ebx = gr_driver->old.driver_flags;
  if(new_driver) {
    word32 base = 0xe0000000L + ((word32)gr_paging_segment << 4);
    tss_ptr->tss_ecx = base + gr_driver->new.text_table;
    tss_ptr->tss_edx = base + gr_driver->new.graphics_table;
  }
  else {
    tss_ptr->tss_ecx = 0L;
    tss_ptr->tss_edx = 0L;
  }
}

void graphics_pageflip(void)
{
  int ax = (unsigned char)tss_ptr->tss_eax;
  int bx = (unsigned int)tss_ptr->tss_ebx;
  int cx;

  if(mult_pages) {
    void far (*pageflip_func)(void) = MK_FP(gr_paging_segment,gr_driver->new.pageflip_routine);
    _BX = bx;
    _AX = ax;
    asm push ds;
    asm mov  ds,word ptr gr_paging_segment;
    asm call dword ptr ss:[pageflip_func];
    asm pop  ds;
    ax = _AX;
    bx = _BX;
    cx = _CX;
  }
  else {
    ax = (ax == 0x00ff) ? 1 : 0;
    bx = 0;
    cx = 0;
  }
  tss_ptr->tss_eax = ax;
  tss_ptr->tss_ebx = (word32)(unsigned int)bx + (word32)cx << 16;
}
#endif

