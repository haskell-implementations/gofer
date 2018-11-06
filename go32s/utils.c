/* This is file UTILS.C */
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

/* History:130,1 */

#include <dos.h>
#include <setjmp.h>

#include "build.h"
#include "types.h"
#include "tss.h"
#include "gdt.h"
#include "utils.h"
#include "npx.h"

extern jmp_buf back_to_debugger;
extern int can_longjmp;

extern near _do_memset32();
extern near _do_memmov32();
extern near _do_save_npx();
extern near _do_load_npx();

NPX npx;

extern int was_exception;

TSS *utils_tss = &o_tss;

go_til_done()
{
  while(1)
  {
    go32();
    if (was_exception)
    {
      if (exception_handler())
        if (can_longjmp)
          longjmp(back_to_debugger, 1);
        else
          return;
    }
    else
      return;
  }
}

zero32(word32 vaddr)
{
  TSS *old_ptr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  utils_tss->tss_eip = (word16)_do_memset32;
  utils_tss->tss_eax = 0;
  utils_tss->tss_ecx = 4096;
  utils_tss->tss_es = g_core*8;
  utils_tss->tss_edi = vaddr;
  go_til_done();
  tss_ptr = old_ptr;
}

memput(word32 vaddr, void far *ptr, word16 len)
{
  TSS *old_ptr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  utils_tss->tss_eip = (word16)_do_memmov32;
  utils_tss->tss_ecx = len;
  utils_tss->tss_es = g_core*8;
  utils_tss->tss_edi = vaddr;
  utils_tss->tss_ds = g_core*8;
  utils_tss->tss_esi = (word32)FP_SEG(ptr)*16 + (word32)FP_OFF(ptr);
  go_til_done();
  tss_ptr = old_ptr;
}

memget(word32 vaddr, void far *ptr, word16 len)
{
  TSS *old_ptr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  utils_tss->tss_eip = (word16)_do_memmov32;
  utils_tss->tss_ecx = len;
  utils_tss->tss_es = g_core*8;
  utils_tss->tss_edi = (word32)FP_SEG(ptr)*16 + (word32)FP_OFF(ptr);
  utils_tss->tss_ds = g_core*8;
  utils_tss->tss_esi = vaddr;
  go_til_done();
  tss_ptr = old_ptr;
}

word32 peek32(word32 vaddr)
{
  word32 rv;
  memget(vaddr, &rv, 4);
  return rv;
}

word16 peek16(word32 vaddr)
{
  word16 rv=0;
  memget(vaddr, &rv, 2);
  return rv;
}

word8 peek8(word32 vaddr)
{
  word8 rv=0;
  memget(vaddr, &rv, 1);
  return rv;
}

poke32(word32 vaddr, word32 v)
{
  memput(vaddr, &v, 4);
}

poke16(word32 vaddr, word16 v)
{
  memput(vaddr, &v, 2);
}

poke8(word32 vaddr, word8 v)
{
  memput(vaddr, &v, 1);
}

save_npx()
{
  TSS *old_ptr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  utils_tss->tss_eip = (word16)_do_save_npx;
  utils_tss->tss_ds = g_rdata*8;
  utils_tss->tss_es = g_rdata*8;
  go_til_done();
  tss_ptr = old_ptr;
}

load_npx()
{
  TSS *old_ptr;
  old_ptr = tss_ptr;
  tss_ptr = utils_tss;
  utils_tss->tss_eip = (word16)_do_load_npx;
  utils_tss->tss_ds = g_rdata*8;
  utils_tss->tss_es = g_rdata*8;
  go_til_done();
  tss_ptr = old_ptr;
}