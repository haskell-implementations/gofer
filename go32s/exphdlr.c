/* This is file EXPHDLR.C */
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
/* History:66,55 */

#include <process.h>
#include <stdio.h>
#include <dos.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include "build.h"
#include "types.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "utils.h"
#include "paging.h"
#include "npx.h"
#include "mono.h"
#include "vcpi.h"

//111111111111111111111111111111111111111111111111111111111111111111111111111
#include "go32sig.h"
//111111111111111111111111111111111111111111111111111111111111111111111111111

#define SEGFAULT(p) { \
  fprintf(stderr, "Segmentation violation in pointer 0x%08lx at %x:%lx\n", (p), tss_ptr->tss_cs, tss_ptr->tss_eip); \
  return 1; \
  }

extern short _openfd[];
extern word32 far *graphics_pt;

extern int was_user_int;
extern word16 vcpi_installed;		/* VCPI Installed flag */
word16 new_pic;				/* current IRQ0 Vector */
char transfer_buffer[4096];	/* must be near ptr for small model */
word32 transfer_linear;

word8 old_master_lo=0x08;
word8 hard_master_lo=0x08, hard_master_hi=0x0f;
word8 hard_slave_lo=0x70,  hard_slave_hi=0x77;

word32 user_dta;
static struct REGPACK r;
static int in_graphics_mode=0;
static int ctrl_c_causes_break=1;

static word32 flmerge(word32 rf, word32 tf)
{
  return (rf & 0xcff) | (tf & 0xfffff300L);
}

static set_controller(v)
{
/*  disable();  */
  outportb(0x20, 0x11);
  outportb(0x21, v);
  outportb(0x21, 4);
  outportb(0x21, 1);
/*  enable();  */
}

static char _save_vectors[32];
extern char vector_78h, vector_79h;

int find_empty_pic()
{
  static word8 try[] = { 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xf8, 0x68, 0x78 };
  int i, j;
  for (i=0; i<sizeof(try); i++)
  {
    char far * far * vec = (char far * far *)(0L+try[i]*4L);
    for (j=1; j<8; j++)
    {
      if (vec[j] != vec[0])
        goto not_empty;
    }
#ifdef DEBUG_PIC
    printf("Empty = %x\n", try[i]);
#endif
    return try[i];
    not_empty:;
  }
  return 0x78;
}

init_controllers()
{
  disable();

  if (vcpi_installed)
  {
    old_master_lo = vcpi_get_pic();
    hard_slave_lo = vcpi_get_secpic();
#ifdef DEBUG_PIC
    printf("VCPI pics were m=%x s=%x\n", old_master_lo, hard_slave_lo);
#endif
    hard_slave_hi = hard_slave_lo + 7;
  }
  else
  {
    old_master_lo = 0x08;
    hard_slave_lo = 0x70;
    hard_slave_hi = 0x77;
  }

  if (old_master_lo == 0x08)
  {
    hard_master_lo = find_empty_pic();
    if (vcpi_installed)
      vcpi_set_pics(hard_master_lo, hard_slave_lo);
    set_controller(hard_master_lo);
    movedata(0, hard_master_lo*4, FP_SEG(_save_vectors), FP_OFF(_save_vectors), 0x08*4);
    movedata(0, 0x08*4, 0, hard_master_lo*4, 0x08*4);
  }
  else
  {
    hard_master_lo = old_master_lo;
  }
  hard_master_hi = hard_master_lo + 7;

  enable();
  vector_78h = hard_master_lo;
  vector_79h = hard_master_lo + 1;

#ifdef DEBUG_PIC
   printf("\nIn init controllers :"
          "\nhard_master_lo = %x"
          "\nhard_slave_lo = %x"
          "\nhard_slave_hi = %x"
          "\nold_master_lo = %x"
         , (int)hard_master_lo, (int)hard_slave_lo, (int)hard_slave_hi
         , (int)old_master_lo
      );
#endif
}

uninit_controllers()
{
  disable();
  if (old_master_lo == 0x08)
  {
    if (vcpi_installed)
      vcpi_set_pics(0x08, hard_slave_lo);
    set_controller(0x08);
    movedata(FP_SEG(_save_vectors), FP_OFF(_save_vectors), 0, hard_master_lo*4, 0x08*4);
  }
  enable();
}

extern int ctrl_c_flag;

#ifdef GO32_STAT
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
extern FILE *pstrmStat;
extern long alStat[256];
extern long alStat21[256];
extern long alStatTurboAssist[256];
#endif

exception_handler()
{
  int i;
#if TOPLINEINFO
  char buf[20];
  if (tss_ptr->tss_irqn == 14)
    sprintf(buf, "0x%08lx", tss_ptr->tss_cr2 - ARENA);
  else
    sprintf(buf, "0x%08lx", tss_ptr->tss_eip);
  for (i=0; buf[i]; i++)
    poke(screen_seg, i*2+80, buf[i] | 0x0600);
#endif
  i = tss_ptr->tss_irqn;
/*  printf("i=%#02x\n", i); */
  if (((i >= hard_slave_lo)  && (i <= hard_slave_hi)
       && (i != hard_slave_hi + 5) && (i != 0x75))
      || ((i >= hard_master_lo) && (i <= hard_master_hi)))
  {
    intr(i, &r);
    if (ctrl_c_causes_break)
      if (i == hard_master_lo + 1)
      {
        r.r_ax = 0x0100;
        intr(0x16, &r);
        if (!(r.r_flags & 0x40) && (r.r_ax == 0x2e03))
        {
          _AH = 0;
          geninterrupt(0x16);

         //111111111111111111111111111111111111111111111111111111111111111111
	      // here is where you land when you do an interrupt 
	      // during a tight loop
   	   ctrlbrk_func();
	      return 0;
         // ctrl_c_flag = 1;
         //111111111111111111111111111111111111111111111111111111111111111111
        }
      }
    if (ctrl_c_flag)
    {
      ctrl_c_flag = 0;
      if (ctrl_c_causes_break)
        return 1;
    }
    return 0;
  }

#ifdef GO32_STAT
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// fprintf(pstrmStat, "\ni = %3.3d - %2.2x - AX = %4.4x", i, i, (unsigned short)tss_ptr->tss_eax);
alStat[i]++;
#endif

  switch (i)
  {
    case 8:
      fprintf(stderr, "double fault!\n");
      exit(1);
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 15:
      return 1;
    case 0x75:
      //111111111111111111111111111111111111111111111111111111111111111111
#ifdef DEBUG_PIC
      printf("\nIn interrupt 0x75");
#endif
      return fpe_func();	/* floating point exception */
      /* return 1; */
      //111111111111111111111111111111111111111111111111111111111111111111
    case 7:
      fprintf(stderr, "Fatal!  Application attempted to use not-present 80387!\n");
      fprintf(stderr, "Floating point opcode at virtual address 0x%08lx\n", tss_ptr->tss_eip);
      return 1;
    case 14:
      return page_in();

    case 0x10:
      return i_10();
    case 0x11:
    case 0x12:
    case 0x14:
    case 0x16:
    case 0x17:
    case 0x1a:
    case 0x15:
      return generic_handler();
    case 0x21:
      return i_21();
    case 0x33:
      return i_33();

    //11111111111111111111111111111111111111111111111111111111111111111111111
   case 0xfa : /* New interface between user program and go32 (see i_Interface()) */
#ifdef DEBUG_SIG
      printf("\nCalling i_Interface now");
#endif
      return i_Interface();
    //11111111111111111111111111111111111111111111111111111111111111111111111

    default:
      return 1;
  }
}

#if DEBUGGER
static char flset[] = "VMRF  NT    OFDNIETFMIZR  AC  PE  CY";
static char floff[] = "              UPID  PLNZ      PO  NC";
static char fluse[] = {1,1,0,1,0,0,1,1,1,1,1,1,0,1,0,1,0,1};

tssprint(TSS *t)
{
  int i;
  printf("eax=%08lx  ebx=%08lx  ecx=%08lx  edx=%08lx\n",
    t->tss_eax, t->tss_ebx, t->tss_ecx, t->tss_edx);
  printf("esi=%08lx  edi=%08lx  ebp=%08lx ",
    t->tss_esi, t->tss_edi, t->tss_ebp);
  for (i=0; i<18; i++)
    if (fluse[i])
      if (t->tss_eflags & (1<<(17-i)))
        printf(" %2.2s", flset+i*2);
      else
        printf(" %2.2s", floff+i*2);
  printf("\nds=%04x es=%04x fs=%04x gs=%04x ss:esp=%04x:%08lx cs=%04x\n",
    t->tss_ds, t->tss_es, t->tss_fs, t->tss_gs, t->tss_ss, t->tss_esp, t->tss_cs);
}
#endif /* DEBUGGER */

int retrieve_string(word32 v, char *transfer_buffer, char tchar)
{
  int i;
  char c;
  for (i=0; i<4096; i++)
  {
    c = peek8(v);
    v++;
    transfer_buffer[i] = c;
    if (c == tchar)
      break;
  }
  return i+1; /* number of characters placed in buffer */
}

static int old_text_mode = -1;

generic_handler()
{
  tss2reg(&r);
  intr(tss_ptr->tss_irqn, &r);
  reg2tss(&r);
  return 0;
}

i_10()
{
#ifdef NONEWDRIVER
  if ((tss_ptr->tss_eax & 0xFF00) == 0xFF00)
  {
#else
  switch(tss_ptr->tss_eax & 0xFF00) {
    case 0xFE00:
      graphics_inquiry();
      return 0;
#endif
    case 0xFF00:
      graphics_mode(tss_ptr->tss_eax & 0xff);
      in_graphics_mode = (peekb(0x40, 0x49) > 7);
      return 0;
  }
  tss2reg(&r);
  intr(0x10, &r);
  reg2tss(&r);
  tss_ptr->tss_ebp = r.r_es * 16L + r.r_bp + 0xe0000000L;
  return 0;
}

#ifndef NOEVENTS

#define  FOR_GO32
#include "eventque.h"

#define  MSDRAW_STACK  128              /* stack size for mouse callback */

static word32  mousedraw_func32;        /* 32-bit mouse cursor drawing function */
static word32  mousedraw_contaddr;      /* jump to this address after mouse draw */
static char    mousedraw_active;        /* set while drawing mouse cursor */
EventQueue    *event_queue = NULL;      /* event queue */

static void mousedraw_hook(void)
{
  disable();
  if(!mousedraw_active) {
    mousedraw_active = 1;
    mousedraw_contaddr = a_tss.tss_eip;
    a_tss.tss_eip = mousedraw_func32;
  }
  enable();
}

i_33()
{
  void (*msdrawfun)(void);
  int  queuesize;

  if(tss_ptr->tss_eax == 0x00ff) {
    if(event_queue != NULL) {
      EventQueueDeInit();
      event_queue = NULL;
    }
    if((queuesize = (int)tss_ptr->tss_ebx) > 0) {
      mousedraw_func32 = tss_ptr->tss_ecx;
      mousedraw_active = 0;
      msdrawfun =
        (mousedraw_func32 != 0L) ? mousedraw_hook : NULL;
      event_queue =
        EventQueueInit(queuesize,MSDRAW_STACK,msdrawfun);
      if(event_queue != NULL) {
        tss_ptr->tss_ebx =
          (((word32)FP_SEG(event_queue)) << 4) +
          ((word32)FP_OFF(event_queue)) +
          0xe0000000L;
        tss_ptr->tss_ecx =
          (((word32)FP_SEG(&mousedraw_contaddr)) << 4) +
          ((word32)FP_OFF(&mousedraw_contaddr)) +
          0xe0000000L;
        tss_ptr->tss_edx =
          (((word32)FP_SEG(&mousedraw_active)) << 4) +
          ((word32)FP_OFF(&mousedraw_active)) +
          0xe0000000L;
      }
      else tss_ptr->tss_ebx = 0L;
    }
    tss_ptr->tss_eax = 0x0ff0;              /* acknowledge event handling */
    return(0);
  }
#else  /* NOEVENTS */
i_33()
{
#endif /* NOEVENTS */
  if (*((unsigned far *)0x000000CEL) == 0)
    return 0;
  r.r_ax = tss_ptr->tss_eax;
  r.r_bx = tss_ptr->tss_ebx;
  r.r_cx = tss_ptr->tss_ecx;
  r.r_dx = tss_ptr->tss_edx;
  intr(0x33, &r);
  tss_ptr->tss_eax = r.r_ax;
  tss_ptr->tss_ebx = r.r_bx;
  tss_ptr->tss_ecx = r.r_cx;
  tss_ptr->tss_edx = r.r_dx;
  return 0;
}

/*1.07 TSS last_tss; */

i_21()
{
  word32 v, trans_total, countleft;
  int i, c, ah, tchar, trans_count;
  char *cp;
/*1.07  memcpy(&last_tss, tss_ptr, sizeof(TSS)); */
  tss2reg(&r);
  ah = (tss_ptr->tss_eax >> 8) & 0xff;
#if 0
  printf("int 21h ax=0x%04x bx=0x%04x cx=0x%04x dx=0x%04x\n",
    (int)(tss_ptr->tss_eax),
    (int)(tss_ptr->tss_ebx),
    (int)(tss_ptr->tss_ecx),
    (int)(tss_ptr->tss_edx)
    );
#endif

#ifdef GO32_STAT
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
alStat21[ah]++;
#endif

  switch (ah)
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 0x0b:
    case 0x0e:
    case 0x19:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x30:
    case 0x36:
    case 0x42:
    case 0x57:
    case 0x68:
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 0x33: /* ^C checking */
      if ((r.r_ax & 0xff) == 0x01)
        ctrl_c_causes_break = r.r_dx & 0xff;
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 0x3e:
#if DEBUGGER
      if (r.r_bx <= 2)
        return 0;
#endif
      if (r.r_bx == 1)
        redir_1_mono = redir_1_2 = 0;
      if (r.r_bx == 2)
        redir_2_mono = redir_2_1 = 0;
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 9:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x41:
    case 0x43:
      if (ah == 9)
        tchar = '$';
      else
        tchar = 0;
      v = tss_ptr->tss_edx + ARENA;
      if (!page_is_valid(v))
      {
        fprintf(stderr, "Segmentation violation in pointer 0x%08lx\n", tss_ptr->tss_edx);
        return 1;
      }
      retrieve_string(v, transfer_buffer, tchar);
      r.r_dx = FP_OFF(transfer_buffer);
      r.r_ds = _DS;
      intr(0x21, &r);
      reg2tss(&r);
      return 0;
    case 0x3c:
      v = tss_ptr->tss_edx + ARENA;
      if (!page_is_valid(v))
      {
        fprintf(stderr, "Segmentation violation in pointer 0x%08lx\n", tss_ptr->tss_edx);
        return 1;
      }
      retrieve_string(v, transfer_buffer, 0);
      i = _creat(transfer_buffer, (int)(tss_ptr->tss_ecx));
      if (i < 0)
      {
        tss_ptr->tss_eax = errno;
        tss_ptr->tss_eflags |= 1;
      }
      else
      {
        tss_ptr->tss_eax = i;
        tss_ptr->tss_eflags &= ~1;
      }
      return 0;
    case 0x3d:
      v = tss_ptr->tss_edx + ARENA;
      if (!page_is_valid(v))
      {
        fprintf(stderr, "Segmentation violation in pointer 0x%08lx\n", tss_ptr->tss_edx);
        return 1;
      }
      retrieve_string(v, transfer_buffer, 0);
      i = tss_ptr->tss_eax & 0xf0;
      if (tss_ptr->tss_eax & O_WRONLY) i &= 1;
      if (tss_ptr->tss_eax & O_RDWR) i &= 2;
      i = _open(transfer_buffer, i);
      if (i < 0)
      {
        tss_ptr->tss_eax = errno;
        tss_ptr->tss_eflags |= 1;
      }
      else
      {
        tss_ptr->tss_eax = i;
        tss_ptr->tss_eflags &= ~1;
      }
      return 0;
    case 0x1a:
      user_dta = tss_ptr->tss_edx;
      setdta((char far *)transfer_buffer);
      return 0;
    case 0x2f:
      tss_ptr->tss_ebx = user_dta;
      return 0;
    case 0x56:
      v = tss_ptr->tss_edx + ARENA;
      if (!page_is_valid(v))
      {
        fprintf(stderr, "Segmentation violation in pointer 0x%08lx\n", tss_ptr->tss_edx);
        return 1;
      }
      i = retrieve_string(v, transfer_buffer, 0);
      r.r_dx = FP_OFF(transfer_buffer);
      r.r_ds = _DS;
      v = tss_ptr->tss_edi + ARENA;
      retrieve_string(v, transfer_buffer+i, 0);
      r.r_di = FP_OFF(transfer_buffer)+i;
      r.r_es = _DS;
      intr(0x21, &r);
      tss_ptr->tss_eax = r.r_ax;
      tss_ptr->tss_eflags = flmerge(r.r_flags, tss_ptr->tss_eflags);
      return 0;
    case 0x3f:
      //111111111111111111111111111111111111111111111111111111111111111111111
      // Are we reading from stdin
      if (r.r_bx == 0) // stdin
         fInRead = 2;
      else 
         fInRead = 1;

#ifdef DBG_READ
   {
   char buf[20];
   sprintf(buf, "  In Read %2.2d, #=%3.3ld  ", ++cRead, tss_ptr->tss_ecx);
   for (i=0; buf[i]; i++)
      poke(screen_seg, i*2+110+2400, buf[i] | 0x0700);
   }
#endif
      //111111111111111111111111111111111111111111111111111111111111111111111
      if (tss_ptr->tss_edx == transfer_linear)
      {
        i = read(r.r_bx, transfer_buffer, r.r_cx);
        if (i<0)
        {
          tss_ptr->tss_eflags |= 1; /* carry */
          tss_ptr->tss_eax = _doserrno;
        }
        else
        {
          tss_ptr->tss_eflags &= ~1;
          tss_ptr->tss_eax = i;
        }
      //111111111111111111111111111111111111111111111111111111111111111111111
         goto handle_interrupt;
      //111111111111111111111111111111111111111111111111111111111111111111111
        return 0;
      }
      trans_total = 0;
      countleft = tss_ptr->tss_ecx;
      v = tss_ptr->tss_edx;
      if (!page_is_valid(v+ARENA))
      {
        fprintf(stderr, "Segmentation violation in pointer 0x%08lx\n", tss_ptr->tss_edx);
        return 1;
      }
      while (countleft > 0)
      {
        trans_count = (countleft <= 4096) ? countleft : 4096;
        i = read(r.r_bx, transfer_buffer, trans_count);
        if (i < 0)
        {
          tss_ptr->tss_eflags |= 1; /* carry */
          tss_ptr->tss_eax = _doserrno;
          return 0;
        }
        memput(v+ARENA, transfer_buffer, i);
        trans_total += i;
        v += i;
        countleft -= i;
        if (isatty(r.r_bx) && (i<trans_count))
          break; /* they're line buffered */
        if (i == 0)
          break;
      }
      tss_ptr->tss_eax = trans_total;
      tss_ptr->tss_eflags &= ~1;

      //111111111111111111111111111111111111111111111111111111111111111111111
handle_interrupt:

#ifdef DBG_READ
      {
      char buf[20];
      int j;
      sprintf(buf, "  Read Done %2.2d, #=%3.3d  ", cRead--, trans_total);
      for (j=0; buf[j]; j++)
         poke(screen_seg, j*2+110+2560, buf[j] | 0x0800);
//    Extensive debugging. Reprints the last typed line.
//	   for (j=0;j<i;j++)
//	      poke(screen_seg, j*2+2720, transfer_buffer[j] | 0x0600);
      }
#endif
      fInRead = 0;   // Read finished so reset flag

      // If had keyboard interrupt during the read, then call
      // SIGINT handler
      if (fHadInterrupt) {
#ifdef DBG_READ
         printf("\n\t\tCalling Interrupt handler from read\n");
#endif
         fHadInterrupt = 0;
         ctrlbrk_func();
         }
      //111111111111111111111111111111111111111111111111111111111111111111111

      return 0;
    case 0x40:
      if (tss_ptr->tss_edx == transfer_linear)
      {
        if ((r.r_bx == 1) && redir_1_mono)
          i = mono_write(transfer_buffer, trans_count);
        else if ((r.r_bx == 2) && redir_2_mono)
          i = mono_write(transfer_buffer, trans_count);
        else
        {
          int fd = r.r_bx;
          if ((r.r_bx == 2) && redir_2_1)
            fd = 1;
          else if ((r.r_bx == 1) && redir_1_2)
            fd = 2;
          i = write(fd, transfer_buffer, r.r_cx);
        }
        if (i<0)
        {
          tss_ptr->tss_eflags |= 1; /* carry */
          tss_ptr->tss_eax = _doserrno;
        }
        else
        {
          tss_ptr->tss_eflags &= ~1;
          tss_ptr->tss_eax = i;
        }
        return 0;
      }
      trans_total = 0;
      countleft = tss_ptr->tss_ecx;
      if (countleft == 0)
      {
        r.r_ax = 0x4000;
        r.r_cx = 0;
        intr(0x21,&r);
        tss_ptr->tss_eax = 0;
        tss_ptr->tss_eflags &= ~1;
        return 0;
      }
      v = tss_ptr->tss_edx;
      if (!page_is_valid(v+ARENA))
        SEGFAULT(v);
      r.r_dx = (int)transfer_buffer;
      while (countleft > 0)
      {
        trans_count = (countleft <= 4096) ? countleft : 4096;
        memget(v+ARENA, transfer_buffer, trans_count);
        if ((r.r_bx == 1) && redir_1_mono)
          i = mono_write(transfer_buffer, trans_count);
        else if ((r.r_bx == 2) && redir_2_mono)
          i = mono_write(transfer_buffer, trans_count);
        else
        {
          int fd = r.r_bx;
          if ((r.r_bx == 2) && redir_2_1)
            fd = 1;
          else if ((r.r_bx == 1) && redir_1_2)
            fd = 2;
          i = write(fd, transfer_buffer, trans_count);
          if (in_graphics_mode && (fd < 3))
          {
            word32 far *p = graphics_pt;
            for (c = 0; c < 256; c++)
              *p++ &= ~PT_P;
          }
        }
        if (i<0) /* carry */
        {
          tss_ptr->tss_eflags |= 1; /* carry */
          tss_ptr->tss_eax = _doserrno;
          return 0;
        }
        trans_total += i;
        v += i;
        countleft -= i;
        if (i < trans_count)
          break;
      }
      tss_ptr->tss_eax = trans_total;
      tss_ptr->tss_eflags &= ~1;
      return 0;
    case 0x44:
      return i_21_44();
    case 0x45:
      i = r.r_bx;
      intr(0x21, &r);
      if (!(r.r_flags & 1))
        _openfd[r.r_ax] = i;
      reg2tss(&r);
      return 0;
    case 0x46:
      i = r.r_bx;
      c = r.r_cx;
      intr(0x21, &r);
      if (!(r.r_flags & 1))
        _openfd[c] = i;
      reg2tss(&r);
      return 0;
    case 0x4e:
      if (!page_is_valid(user_dta+ARENA))
        SEGFAULT(user_dta);
      v = tss_ptr->tss_edx + ARENA;
      if (!page_is_valid(v))
        SEGFAULT(v);
      retrieve_string(v, transfer_buffer+43, 0);
      r.r_dx = FP_OFF(transfer_buffer+43);
      r.r_ds = _DS;
      intr(0x21, &r);
      reg2tss(&r);
      for (i=20; i>=0; i--)
        transfer_buffer[i+28] = transfer_buffer[i+26];
      transfer_buffer[32+13] = 0; /* asciiz termination */
      memput(user_dta+ARENA, transfer_buffer, 48);
      return 0;
    case 0x4f:
      if (!page_is_valid(user_dta+ARENA))
        SEGFAULT(user_dta);
      memget(user_dta+ARENA, transfer_buffer, 48);
      for (i=0; i<=20; i++)
        transfer_buffer[i+26] = transfer_buffer[i+28];
      intr(0x21, &r);
      reg2tss(&r);
      for (i=20; i>=0; i--)
        transfer_buffer[i+28] = transfer_buffer[i+26];
      transfer_buffer[32+13] = 0; /* asciiz termination */
      memput(user_dta+ARENA, transfer_buffer, 48);
      return 0;
    case 0x47:
      getcurdir((int)(tss_ptr->tss_edx & 0xff), transfer_buffer);
      for (cp=transfer_buffer; *cp; cp++)
      {
        if (*cp == '\\') *cp = '/';
        *cp = tolower(*cp);
      }
      memput(tss_ptr->tss_esi+ARENA, transfer_buffer, strlen(transfer_buffer)+1);
      tss_ptr->tss_eax = (unsigned)r.r_ax;
      tss_ptr->tss_eflags &= ~1;
      return 0;
    case 0x4a:
      if (tss_ptr->tss_eax & 0xff)
        tss_ptr->tss_eax = paging_sbrk(tss_ptr->tss_ebx);
      else
        tss_ptr->tss_eax = paging_brk(tss_ptr->tss_ebx);
      return 0;
    case 0x4c:
#if DEBUGGER
      printf("Program exited normally, return code %d (0x%x)\n",
             (int)(tss_ptr->tss_eax & 0xff), (int)(tss_ptr->tss_eax & 0xff));
      return 1;
#else
#if 0
      {
        int i, ch;
        FILE *f = fopen("con", "w");
        fprintf(f, "zero: ");
        for (i=0; i<20; i++)
        {
          ch = *(char *)i;
          fprintf(f, "%02x ", ch);
        }
        fprintf(f, "\r\n");
      }
#endif
      exit(tss_ptr->tss_eax & 0xff);
#endif


    case 0xff:
      return turbo_assist();
    default:
      return 1;
  }
}

struct time32 {
  word32 secs;
  word32 usecs;
};

struct tz32 {
  word32 offset;
  word32 dst;
};

struct	stat32 {
	short st_dev;
	short st_ino;
	short st_mode;
	short st_nlink;
	short st_uid;
	short st_gid;
	short st_rdev;
	short st_align_for_word32;
	long  st_size;
	long  st_atime;
	long  st_mtime;
	long  st_ctime;
	long  st_blksize;
};

static int dev_count=1;

turbo_assist()
{
  word32 p1, p2, p3, r;
  struct time32 time32;
  struct tz32 tz32;
  struct stat32 statbuf32;
  struct stat statbuf;
  int i;

  char buf[128];
  p1 = tss_ptr->tss_ebx;
  p2 = tss_ptr->tss_ecx;
  p3 = tss_ptr->tss_edx;

#ifdef GO32_STAT
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
alStatTurboAssist[(tss_ptr->tss_eax & 0xff)]++;
#endif

  switch (tss_ptr->tss_eax & 0xff)
  {
    case 1:
      retrieve_string(p1+ARENA, buf, 0);
      r = creat(buf, S_IREAD | S_IWRITE);
      break;
    case 2:
      retrieve_string(p1+ARENA, buf, 0);
      r = open(buf, (int)p2, S_IREAD | S_IWRITE);
      break;
    case 3:
      memset(&statbuf, 0, sizeof(statbuf));
      r = fstat((int)p1, &statbuf);
      statbuf32.st_dev = dev_count++;
      statbuf32.st_ino = statbuf.st_ino;
      statbuf32.st_mode = statbuf.st_mode;
      statbuf32.st_nlink = statbuf.st_nlink;
      statbuf32.st_uid = 42;
      statbuf32.st_gid = 42;
      statbuf32.st_rdev = statbuf.st_rdev;
      statbuf32.st_size = statbuf.st_size;
      statbuf32.st_atime = statbuf.st_atime;
      statbuf32.st_mtime = statbuf.st_mtime;
      statbuf32.st_ctime = statbuf.st_ctime;
      statbuf32.st_blksize = 512;
      memput(p2+ARENA, &statbuf32, sizeof(statbuf32));
      break;
    case 4:
      if (p2)
      {
        if (!page_is_valid(p2+ARENA))
          SEGFAULT(p2);
        tz32.offset = timezone;
        tz32.dst = daylight;
        memput(p2+ARENA, &tz32, sizeof(tz32));
      }
      if (p1)
      {
        if (!page_is_valid(p1+ARENA))
          SEGFAULT(p1);
        time(&(time32.secs));
        _AH = 0x2c;
        geninterrupt(0x21);
        time32.usecs = _DL * 10000L;
        memput(p1+ARENA, &time32, sizeof(time32));
      }
      r = 0;
      break;
    case 5:
      if (p2)
      {
        if (!page_is_valid(p2+ARENA))
          SEGFAULT(p2);
        memget(p2+ARENA, &tz32, sizeof(tz32));
        timezone = tz32.offset;
        daylight = tz32.dst;
      }
      if (p1)
      {
        if (!page_is_valid(p1+ARENA))
          SEGFAULT(p1);
        memget(p1+ARENA, &time32, sizeof(time32));
        stime(&(time32.secs));
      }
      r = 0;
      break;
    case 6:
      memset(&statbuf, 0, sizeof(statbuf));
      retrieve_string(p1+ARENA, transfer_buffer, 0);
      r = unixlike_stat(transfer_buffer, &statbuf);
      statbuf32.st_dev = dev_count++;
      statbuf32.st_ino = statbuf.st_ino;
      statbuf32.st_mode = statbuf.st_mode;
      statbuf32.st_nlink = statbuf.st_nlink;
      statbuf32.st_uid = statbuf.st_uid;
      statbuf32.st_gid = statbuf.st_gid;
      statbuf32.st_rdev = statbuf.st_rdev;
      statbuf32.st_size = statbuf.st_size;
      statbuf32.st_atime = statbuf.st_atime;
      statbuf32.st_mtime = statbuf.st_mtime;
      statbuf32.st_ctime = statbuf.st_ctime;
      statbuf32.st_blksize = 512;
      memput(p2+ARENA, &statbuf32, sizeof(statbuf32));
      break;
    case 7:
      retrieve_string(p1+ARENA, transfer_buffer, 0);
      page_out_everything();
      uninit_controllers();
      sscanf(transfer_buffer, "%s%n", buf, &i);
      if (strpbrk(transfer_buffer, "<>|") == NULL)
        r = spawnlp(P_WAIT, buf, buf, transfer_buffer+i, 0);
      else
        r = -1;
      if (r & 0x80000000L)
        r = system(transfer_buffer);
      init_controllers();
      page_in_everything();
      break;
    case 8:
      _BX=(int)p1;
      _AX=0x4400;
      geninterrupt(0x21);
      i = _DX;
      if (p2 & O_BINARY)
        i |= 0x20;
      else
        i &= ~0x20;
      _BX=(int)p1;
      _DX = i;
      _AX=0x4401;
      geninterrupt(0x21);
      r = setmode((int)p1, (int)p2);
      break;
    case 9:
      retrieve_string(p1+ARENA, buf, 0);
      r = chmod(buf, (int)p2);
      break;
    default:
      return 1;
  }
  tss_ptr->tss_eflags &= ~1;
  if (r == -1)
  {
    tss_ptr->tss_eflags |= 1;
    tss_ptr->tss_eax = errno;
    return 0;
  }
  tss_ptr->tss_eax = r;
  return 0;
}

i_21_44()
{
  switch (tss_ptr->tss_eax & 0xff)
  {
    case 0x00:
    case 0x01:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0e:
    case 0x0f:
      intr(0x21, &r);
      tss_ptr->tss_edx = r.r_dx;
      tss_ptr->tss_eax = r.r_ax;
      tss_ptr->tss_eflags = flmerge(r.r_flags, tss_ptr->tss_eflags);
      return 0;
    default:
      return 1;
  }
}

tss2reg(struct REGPACK *r)
{
  r->r_ax = tss_ptr->tss_eax;
  r->r_bx = tss_ptr->tss_ebx;
  r->r_cx = tss_ptr->tss_ecx;
  r->r_dx = tss_ptr->tss_edx;
  r->r_si = tss_ptr->tss_esi;
  r->r_di = tss_ptr->tss_edi;
  r->r_flags = tss_ptr->tss_eflags;
  r->r_ds = r->r_es = _DS;
}

reg2tss(struct REGPACK *r)
{
  tss_ptr->tss_eax = r->r_ax;
  tss_ptr->tss_ebx = r->r_bx;
  tss_ptr->tss_ecx = r->r_cx;
  tss_ptr->tss_edx = r->r_dx;
  tss_ptr->tss_esi = r->r_si;
  tss_ptr->tss_edi = r->r_di;
  tss_ptr->tss_eflags = flmerge(r->r_flags, tss_ptr->tss_eflags);
}
