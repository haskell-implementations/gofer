/* This is file PAGING.C */
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
/* NUR paging algorithm by rcharif@math.utexas.edu */
/* History:112,12 */

#include <stdio.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include "build.h"
#include "types.h"
#include "paging.h"
#include "graphics.h"
#include "tss.h"
#include "idt.h"
#include "gdt.h"
#include "valloc.h"
#include "dalloc.h"
#include "utils.h"
#include "aout.h"
#include "mono.h"
#include "vcpi.h"

#define VERBOSE 0


#if DEBUGGER
#define MAX_PAGING_NUM 1
#else
#define MAX_PAGING_NUM 4
#endif


#define	DOS_PAGE 256		/*  1MB / 4KB = 256 Pages  */


extern word32 ptr2linear(void far *ptr);

struct {
  word16 limit;
  word32 base;
} gdt_phys, idt_phys;

CLIENT	client;		/*  VCPI Change Mode Structure  */
word32	abs_client;	/*  _DS * 16L + &client		*/
far32	vcpi_entry;
SYS_TBL	int_descriptor;
SYS_TBL gbl_descriptor;

extern word16 vcpi_installed;	/*  VCPI Installed Flag  */
extern near protect_entry();

extern TSS *utils_tss;
extern int debug_mode;
extern word16 mem_avail;
extern int self_contained;
extern long header_offset;

typedef struct AREAS {
  word32 first_addr;
  word32 last_addr;
  word32 foffset; /* corresponding to first_addr; -1 = zero fill only */
  } AREAS;

#define MAX_AREA	8
static AREAS areas[MAX_AREA];
static char *aname[MAX_AREA] = {
	"text ",
	"data ",
	"bss  ",
	"arena",
	"stack",
	"vga  ",
	"syms ",
	"emu"
};
static char achar[MAX_AREA] = "tdbmsg?e";
typedef enum {
  A_text,
  A_data,
  A_bss,
  A_arena,
  A_stack,
  A_vga,
  A_syms,
  A_emu
} AREA_TYPES;

static aout_f;
static emu_f;

word32 far *pd = 0;
word8 pd_seg[1024];
word32 far *graphics_pt;
word32 far *vcpi_pt = 0;
extern word32 graphics_pt_lin;
char paging_buffer[4096*MAX_PAGING_NUM];
static int paged_out_something = 0;

word32 screen_primary, screen_secondary;

static word32 far2pte(void far *ptr, word32 flags)
{
  return (vcpi_pt[((word32)ptr) >> 24] & 0xfffff000L) | flags;
}

static word32 pn2pte(unsigned pn, word32 flags)
{
  return (vcpi_pt[pn] & 0xfffff000L) | flags;
}

/*  VCPI Get Interface  */
void	link_vcpi(word32 far *dir, word32 far *table)
{

  vcpi_entry.selector = g_vcpicode * 8;
  vcpi_entry.offset32 = get_interface(table,&gdt[g_vcpicode]);
  client.page_table   = far2pte(dir, 0); /* (word32)dir>>12; */
  client.gdt_address  = ptr2linear(&gdt_phys);
  client.idt_address  = ptr2linear(&idt_phys);
  client.ldt_selector = 0;
  client.tss_selector = g_ctss * 8;
  client.entry_eip    = (word16)protect_entry;
  client.entry_cs     = g_rcode * 8;

  abs_client = ptr2linear(&client);
}

handle_screen_swap(word32 far *pt)
{
  struct REGPACK r;
  int have_mono=0;
  int have_color=0;
  int have_graphics=0;
  int save, new, i;

  r.r_ax = 0x1200;
  r.r_bx = 0xff10;
  r.r_cx = 0xffff;
  intr(0x10, &r);
  if (r.r_cx == 0xffff)
    pokeb(0x40, 0x84, 24); /* the only size for CGA/MDA */

  if (!vcpi_installed || (pt[0xb8] & (PT_U|PT_W)) == (PT_W|PT_U))
  {
    save = peekb(screen_seg, 0);
    pokeb(screen_seg, 0, ~save);
    new = peekb(screen_seg, 0);
    pokeb(screen_seg, 0, save);
    if (new == ~save)
      have_color = 1;
  }

  if (!vcpi_installed || (pt[0xb0] & (PT_U|PT_W)) == (PT_W|PT_U))
  {
    save = peekb(0xb000, 0);
    pokeb(0xb000, 0, ~save);
    new = peekb(0xb000, 0);
    pokeb(0xb000, 0, save);
    if (new == ~save)
      have_mono = 1;
  }

  r.r_ax = 0x0f00;
  intr(0x10, &r);
  if ((r.r_ax & 0xff) > 0x07)
    have_graphics = 1;

  if (have_graphics && have_mono)
    have_color = 1;
  else if (have_graphics && have_color)
    have_mono = 1;

  screen_primary = 0xe00b8000L;
  screen_secondary = 0xe00b0000L;

  if (have_color && !have_mono)
  {
    screen_secondary = 0xe00b8000L;
    return;
  }
  if (have_mono & !have_color)
  {
    screen_primary = 0xe00b0000L;
    return;
  }

  if ((biosequip() & 0x0030) == 0x0030) /* mono mode, swap! */
  {
    screen_primary = 0xe00b0000L;
    screen_secondary = 0xe00b8000L;
    return;
  }
}

paging_set_file(char *fname)
{
  word32 lin;
  word32 far *pt;
  FILEHDR filehdr;
  AOUTHDR aouthdr;
  SCNHDR scnhdr[3];
  GNU_AOUT gnu_aout;
  unsigned short *exe_hdr;
  int i;
  aout_f = open(fname, O_RDONLY|O_BINARY);
  if (aout_f < 0)
  {
    fprintf(stderr, "Can't open file <%s>\n", fname);
    exit(1);
  }

#if TOPLINEINFO
  for (i=0; fname[i]; i++)
    poke(screen_seg, i*2+10, fname[i] | 0x0700);
#endif

  lseek(aout_f, header_offset, 0);
  read(aout_f, &filehdr, sizeof(filehdr));

  if (filehdr.f_magic == 0x5a4d) /* .EXE */
  {
    exe_hdr = (unsigned short *)&filehdr;
    header_offset += (long)exe_hdr[2]*512L;
    if (exe_hdr[1])
      header_offset += (long)exe_hdr[1] - 512L;
    lseek(aout_f, header_offset, 0);
    read(aout_f, &filehdr, sizeof(filehdr));
  }

  if (filehdr.f_magic != 0x14c)
  {
    lseek(aout_f, header_offset, 0);
    read(aout_f, &gnu_aout, sizeof(gnu_aout));
    a_tss.tss_eip = gnu_aout.entry;
    aouthdr.tsize = gnu_aout.tsize;
    aouthdr.dsize = gnu_aout.dsize;
    aouthdr.bsize = gnu_aout.bsize;
  }
  else
  {
    read(aout_f, &aouthdr, sizeof(aouthdr));
    a_tss.tss_eip = aouthdr.entry;
    read(aout_f, scnhdr, sizeof(scnhdr));
  }
  a_tss.tss_cs = g_acode*8;
  a_tss.tss_ds = g_adata*8;
  a_tss.tss_es = g_adata*8;
  a_tss.tss_fs = g_adata*8;
  a_tss.tss_gs = g_adata*8;
  a_tss.tss_ss = g_adata*8;
  a_tss.tss_esp = 0x7ffffffc;

  if (filehdr.f_magic == 0x14c)
  {
    areas[0].first_addr = aouthdr.text_start + ARENA;
    areas[0].foffset = scnhdr[0].s_scnptr + header_offset;
    areas[0].last_addr = areas[0].first_addr + aouthdr.tsize;
  }
  else if (filehdr.f_magic == 0x10b)
  {
    areas[0].first_addr = ARENA;
    if (a_tss.tss_eip >= 0x1000)	/* leave space for null reference */
      areas[0].first_addr += 0x1000;	/* to cause seg fault */
    areas[0].foffset = header_offset;
    areas[0].last_addr = areas[0].first_addr + aouthdr.tsize + 0x20;
  }
#if DEBUGGER
  else if (filehdr.f_magic == 0x107)
  {
    struct stat sbuf;
    fstat(aout_f, &sbuf);
    areas[0].first_addr = ARENA;
    areas[0].foffset = 0x20 + header_offset;
    areas[0].last_addr = sbuf.st_size + ARENA - 0x20;
  }
  else
  {
    struct stat sbuf;
    fstat(aout_f, &sbuf);
    areas[0].first_addr = ARENA;
    areas[0].foffset = header_offset;
    areas[0].last_addr = sbuf.st_size + ARENA;
  }
#else
  else
  {
    fprintf(stderr, "Unknown file type 0x%x (0%o)\n", filehdr.f_magic, filehdr.f_magic);
    exit(-1);
  }
#endif
#if DEBUGGER
  if (debug_mode)
    printf("%ld+", aouthdr.tsize);
#endif

  if (filehdr.f_magic == 0x14c)
  {
    areas[1].first_addr = aouthdr.data_start + ARENA;
    areas[1].foffset = scnhdr[1].s_scnptr + header_offset;
  }
  else
  {
    areas[1].first_addr = (areas[0].last_addr+0x3fffffL)&~0x3fffffL;
    areas[1].foffset = ((aouthdr.tsize + 0x20 + 0xfffL) & ~0xfffL) + header_offset;
  }
  areas[1].last_addr = areas[1].first_addr + aouthdr.dsize - 1;
#if DEBUGGER
  if (debug_mode)
    printf("%ld+", aouthdr.dsize);
#endif

  areas[2].first_addr = areas[1].last_addr + 1;
  areas[2].foffset = -1;
  areas[2].last_addr = areas[2].first_addr + aouthdr.bsize - 1;
#if DEBUGGER
  if (debug_mode)
    printf("%ld = %ld\n", aouthdr.bsize,
      aouthdr.tsize+aouthdr.dsize+aouthdr.bsize);
#endif

  areas[3].first_addr = areas[2].last_addr;
  areas[3].last_addr = areas[3].first_addr;
  areas[3].foffset = -1;

  areas[4].first_addr = 0x50000000;
  areas[4].last_addr = 0x8fffffff;
  areas[4].foffset = -1;

  areas[5].first_addr = 0xe0000000;
  areas[5].last_addr = 0xe03fffff;
  areas[5].foffset = -1;

  areas[A_syms].first_addr = 0xa0000000;
  areas[A_syms].last_addr = 0xafffffff;
  areas[A_syms].foffset = -1;

  pd = (word32 far *)((long)valloc(VA_640) << 24);
  vcpi_pt = pt = (word32 far *)((long)valloc(VA_640) << 24);
  for (i=0; i<1024; i++)
  {
    pd[i] = 0;
    pd_seg[i] = 0;
  }

  if (vcpi_installed)
    {
    link_vcpi(pd,pt);		/*  Get VCPI Page Table  */
    for ( i=0; i<1024; i++)
      if (pt[i] & PT_P)
	pt[i] |= PT_I;
    }
  else
    {
    for (i=0; i < DOS_PAGE; i++)
      pt[i] = ((unsigned long)i<<12) | PT_P | PT_W | PT_I;
    for (; i<1024; i++)
      pt[i] = 0;
    }

  pd[0] = 
  pd[0x3c0] = far2pte(pt, PT_P | PT_W | PT_I);	/* map 1:1 1st Mb */
  pd_seg[0] = 
  pd_seg[0x3c0] = (word32)pt >> 24;

  gdt_phys.limit = gdt[g_gdt].lim0;
  gdt_phys.base = ptr2linear(&gdt);
  idt_phys.limit = gdt[g_idt].lim0;
  idt_phys.base = ptr2linear(&idt);

  handle_screen_swap(pt);
  a_tss.tss_ebx = screen_primary;
  a_tss.tss_ebp = screen_secondary;

  graphics_pt = (word32 far *)((long)valloc(VA_640) << 24);
  graphics_pt_lin = ptr2linear(graphics_pt);
  for (i=0; i<1024; i++)
    graphics_pt[i] = 0x000a0000L | ((i * 4096L) & 0xffffL) | PT_W | PT_U;
  pd[0x380] = far2pte(graphics_pt, PT_P | PT_W | PT_U);
  pd_seg[0x380] = (word32)graphics_pt >> 24;

  c_tss.tss_cr3 = 
  a_tss.tss_cr3 = 
  o_tss.tss_cr3 = 
  i_tss.tss_cr3 = 
  p_tss.tss_cr3 = 
  f_tss.tss_cr3 = 
  v74_tss.tss_cr3 = 
  v78_tss.tss_cr3 = 
  v79_tss.tss_cr3 = far2pte(pd, 0);

  a_tss.tss_esi = far2pte(pd,0) >> 12; /* PID */

#if VERBOSE
    for (i=0; i<5; i++)
      printf("%d %-10s %08lx-%08lx (offset 0x%08lx)\n", i, aname[i], areas[i].first_addr, areas[i].last_addr, areas[i].foffset);
#endif
}

#if TOPLINEINFO
static update_status(int c, int col)
{
  int r;
  r = peek(screen_seg, 2*79);
  poke(screen_seg, 2*col, c);
  return r;
}
#endif

word32 paging_brk(word32 b)
{
  word32 r = (areas[3].last_addr - ARENA + 7) & ~7;
  areas[3].last_addr = b + ARENA;
  return r;
}

word32 paging_sbrk(int32 b)
{
  word32 r = (areas[3].last_addr - ARENA + 7) & ~7;
  areas[3].last_addr = r + b + ARENA;
  return r;
}

page_is_valid(word32 vaddr)
{
  int a;
  for (a=0; a<MAX_AREA; a++)
    if ((vaddr <= areas[a].last_addr) && (vaddr >= areas[a].first_addr))
      return 1;
  if (vaddr >= 0xf000000L)
    return 1;
  return 0;
}

page_in()
{
  int old_status;
  TSS *old_util_tss;
  word32 far *pt;
  word32 far *p;
  word32 vaddr, foffset, cnt32;
  word32 eaddr, vtran, vcnt, zaddr;
  int pdi, pti, pn, a, cnt, count;
  unsigned dblock;

#if 0
  unsigned char buf[100];
  sprintf(buf, "0x%08lx", a_tss.tss_cr2 - ARENA);
  for (a=0; buf[a]; a++)
    poke(screen_seg, 80+a*2, 0x0600 | buf[a]);
#endif

  old_util_tss = utils_tss;
  utils_tss = &f_tss;
  vaddr = tss_ptr->tss_cr2;

  for (a=0; a<MAX_AREA; a++)
    if ((vaddr <= areas[a].last_addr) && (vaddr >= areas[a].first_addr))
      goto got_area;

  if (vaddr >= 0xf0000000L)
  {
    pdi = (vaddr >> 22) & 0x3ff;
    if (!(pd[pdi] & PT_P))	/* put in a mapped page table */
    {
      pn = valloc(VA_640);
      pt = (word32 far *)((word32)pn << 24);
      if (pd[pdi] & PT_S)
      {
        dread(paging_buffer, pd[pdi]>>12);
        movedata(_DS, paging_buffer, FP_SEG(pt), FP_OFF(pt), 4096);
        dfree(pd[pdi]>>12);
        pd[pdi] = pn2pte(pn, PT_P | PT_W | PT_I | PT_S);
        pd_seg[pdi] = pn;
      }
      else
      {
        pd[pdi] = pn2pte(pn, PT_P | PT_W | PT_I | PT_S);
        pd_seg[pdi] = pn;
        vaddr &= 0x0fc00000L;
        for (pti = 0; pti < 1024; pti++)
          pt[pti] = PT_P | PT_W | PT_I | vaddr | (((word32)pti)<<12);
      }
      return 0;
    }
    pt = (word32 far *)((word32)(pd_seg[pdi]) << 24);
    vaddr &= 0x0ffff000L;
    pti = (vaddr>>12) & 0x3ff;
    pt[pti] = vaddr | PT_P | PT_W | PT_I;
    return 0;
  }

  fprintf(stderr, "Segmentation violation in pointer 0x%08lx at %x:%lx\n",
   tss_ptr->tss_cr2-ARENA, tss_ptr->tss_cs, tss_ptr->tss_eip);
  return 1;

got_area:
  vaddr &= 0xFFFFF000;	/* points to beginning of page */
#if 0 /* handled in protected mode for speed */
  if (a == A_vga)
    return graphics_fault(vaddr, graphics_pt);
#endif

#if VERBOSE
    printf("area(%d) - ", a);
#endif

  if ((a == 2) & (vaddr < areas[a].first_addr)) /* bss, but data too */
  {
#if VERBOSE
      printf("split page (data/bss) detected - ");
#endif
    a = 1; /* set to page in data */
  }

#if TOPLINEINFO
  old_status = update_status(achar[a] | 0x0a00, 78);
#endif
#if VERBOSE
  printf("Paging in %s block for vaddr %#010lx -", aname[a], tss_ptr->tss_cr2-ARENA);
#endif
  pdi = (vaddr >> 22) & 0x3ff;
  if (!(pd[pdi] & PT_P))	/* put in an empty page table if required */
  {
    pn = valloc(VA_640);
    pt = (word32 far *)((word32)pn << 24);
    if (pd[pdi] & PT_I)
    {
      dread(paging_buffer, pd[pdi]>>12);
      movedata(_DS, paging_buffer, FP_SEG(pt), FP_OFF(pt), 4096);
      dfree(pd[pdi] >> 12);
      pd[pdi] = pn2pte(pn, PT_P | PT_W | PT_I | PT_S);
      pd_seg[pdi] = pn;
    }
    else
    {
      pd[pdi] = pn2pte(pn, PT_P | PT_W | PT_I | PT_S);
      pd_seg[pdi] = pn;
      for (pti=0; pti<1024; pti++)
        pt[pti] = PT_W | PT_S;
    }
  }
  else
    pt = (word32 far *)((word32)(pd_seg[pdi]) << 24);
  pti = (vaddr >> 12) & 0x3ff;
  if (pt[pti] & PT_P)
  {
    utils_tss = old_util_tss;
#if TOPLINEINFO
    update_status(old_status, 78);
#endif
    return 0;
  }
  count = MAX_PAGING_NUM;
  if ((count > mem_avail/4) || (paged_out_something))
    count = 1;
  if (pti + count > 1024)
    count = 1024 - pti;
  if (vaddr + count*4096L > areas[a].last_addr+4096L)
    count = (areas[a].last_addr - vaddr + 4095) / 4096;
  if (count < 1)
    count = 1;
  zaddr = eaddr = -1;
  vtran = vaddr;
  vcnt = 0;
  for (; count; count--, pti++, vaddr+=4096)
  {
    if (pt[pti] & PT_P)
      break;
    dblock = pt[pti] >> 12;
    pn = valloc(VA_1M);
    pt[pti] &= 0xfffL & ~(word32)(PT_A | PT_D);
    pt[pti] |= ((word32)pn << 12) | PT_P;

    if (pt[pti] & PT_I)
    {
#if VERBOSE
        printf(" swap");
#endif
      dread(paging_buffer, dblock);
      dfree(dblock);
      memput(vaddr, paging_buffer, 4096);
      pt[pti] &= ~(word32)(PT_A | PT_D);  /* clean dirty an accessed bits (set by memput) */
    }
    else
    {
      pt[pti] &= ~(word32)(PT_C);
      if (areas[a].foffset != -1)
      {
#if VERBOSE
        if (a == A_emu)
          printf(" emu");
        else
          printf(" exec");
#endif
        if (eaddr == -1)
        {
          eaddr = areas[a].foffset + (vaddr - areas[a].first_addr);
          vtran = vaddr;
        }
        cnt32 = areas[a].last_addr - vaddr + 1;
        if (cnt32 > 4096)
          cnt32 = 4096;
        else
          zaddr = vaddr;
        vcnt += cnt32;
      }
      else
      {
        zero32(vaddr);
#if VERBOSE
        printf(" zero");
#endif
      }
      pt[pti] |= PT_I;
    }
    if (paged_out_something)
      break;
  }
  if (eaddr != -1)
  {
    int cur_f, rsize, vc;
    if (a == A_emu)
      cur_f = emu_f;
    else
      cur_f = aout_f;
    lseek(cur_f, eaddr, 0);
    rsize = read(cur_f, paging_buffer, vcnt);
    if (rsize < vcnt)
      memset(paging_buffer+rsize, 0, vcnt-rsize);
    if (zaddr != -1)
      zero32(zaddr);
    memput(vtran, paging_buffer, vcnt);
    vc = vcnt / 4096; /* don't reset BSS parts */
    while (vc)
    {
      pdi = vtran >> 22;
      pt = (word32 far *)((word32)(pd_seg[pdi]) << 24);
      pti = (vtran >> 12) & 0x3ff;
      pt[pti] &= ~(word32)(PT_A | PT_D);  /* clean dirty an accessed bits (set by memput) */
      vc--;
      vtran += 4096;
    }
  }
#if VERBOSE
  printf("\n");
#endif
  utils_tss = old_util_tss;
#if TOPLINEINFO
  update_status(old_status, 78);
#endif
  return 0;
}

static fInPageOutEverything = 0;
static last_po_pdi = 0;
static last_po_pti = 0;
static last_pti = 0;

unsigned page_out(int where) /* return >= 0 page which is paged out, 0xffff if not */
{
  int start_pdi, start_pti, old_status;
  word32 far *pt, v, pti, rv;
  unsigned dblock, pn, pn1;
#if TOPLINEINFO
  old_status = update_status('>' | 0x0a00, 79);
#endif
  if (!fInPageOutEverything)
    paged_out_something = 1;
  start_pdi = last_po_pdi;
  start_pti = last_po_pti;
  if (where == VA_640)
  {
    for (pti = last_pti+1; pti != last_pti; pti = (pti+1)%1024)
      if ((pd[pti] & (PT_P | PT_S)) == (PT_P | PT_S))
      {
        dblock = dalloc();
        movedata(pd_seg[pti]<<8, 0, _DS, paging_buffer, 4096);
        dwrite(paging_buffer, dblock);
#if VERBOSE
        printf ("out_640 %d\n", pti);
#endif
        pd[pti] &= 0xfff & ~(word32)(PT_P); /* no longer present */
        pd[pti] |= (long)dblock << 12;
#if TOPLINEINFO
        update_status(old_status, 79);
#endif
	last_pti = pti;
        return pd_seg[pti];
      }
    return -1;
  }
  pt = (word32 far *)((word32)(pd_seg[last_po_pdi]) << 24);
  do {
    if ((pd[last_po_pdi] & (PT_P | PT_S)) == (PT_P | PT_S))
    {
      if ((pt[last_po_pti] & (PT_P | PT_S)) == (PT_P | PT_S))
      {
        rv = pt[last_po_pti] >> 12;
        v = ((word32)last_po_pdi << 22) | ((word32)last_po_pti << 12);
        if (!fInPageOutEverything)
          if ((v & 0xfffff000L) == ((tss_ptr->tss_eip + ARENA) & 0xfffff000L) ||
              (v & 0xfffff000L) == ((tss_ptr->tss_esp + ARENA) & 0xfffff000L))
          {
#if VERBOSE
            printf("\nskip: v=%08lx - ", v);
#endif
            goto bad_choice;
          }
        if (pt[last_po_pti] & (PT_C | PT_D))
        {
          pt[last_po_pti] |= PT_C;
          dblock = dalloc();
          memget(v, paging_buffer, 4096);
#if VERBOSE
          printf ("dout %08lx", ((word32)last_po_pdi<<22) | ((word32)last_po_pti<<12));
#endif
          dwrite(paging_buffer, dblock);
          pt[last_po_pti] &= 0xfff & ~PT_P; /* no longer present */
          pt[last_po_pti] |= (long)dblock << 12;
        }
        else
        {
          pt[last_po_pti] = PT_W | PT_S;
#if VERBOSE
          printf ("dflush %08lx", ((word32)last_po_pdi<<22) | ((word32)last_po_pti<<12));
#endif
        }
#if TOPLINEINFO
        update_status(old_status, 79);
#endif
        return rv;
      }
    }
    else /* imagine we just checked the last entry */
      last_po_pti = 1023;

bad_choice:
    if (++last_po_pti == 1024)
    {
      last_po_pti = 0;
      if (++last_po_pdi == 1024)
        last_po_pdi = 0;
      pt = (word32 far *)((word32)(pd_seg[last_po_pdi]) << 24);
    }
  } while ((start_pdi != last_po_pdi) || (start_pti != last_po_pti));
#if TOPLINEINFO
  update_status(old_status, 79);
#endif
  return 0xffff;
}

unsigned pd_dblock;

extern int valloc_initted;

page_out_everything()
{
  int pdi, i;
  word32 opde;
  unsigned ptb;
  void far *fp;

  fInPageOutEverything = 1;

  while (page_out(-1) != 0xffff)
  {
    vfree();
  }
  for (pdi=0; pdi<1024; pdi++)
    if (pd[pdi] & PT_P)
    {
      ptb = dalloc();
      opde = pd[pdi] & 0xfffff000L;
      fp = (word32 far *)((word32)pd_seg[pdi] << 24);
      movedata(FP_SEG(fp), FP_OFF(fp), _DS, paging_buffer, 4096);
      dwrite(paging_buffer, ptb);
      vfree();
      pd[pdi] = (pd[pdi] & (0xFFF&~PT_P)) | ((word32)ptb<<12);
      for (i=pdi+1; i<1024; i++)
        if ((pd[i] & 0xfffff000L) == opde)
          pd[i] = pd[pdi];
    }
  movedata(FP_SEG(pd), FP_OFF(pd), _DS, paging_buffer, 4096);
  pd_dblock = dalloc();
  dwrite(paging_buffer, pd_dblock);
  vfree();
  vcpi_flush();
  valloc_uninit();
}

page_in_everything()
{
  int pdi, i;
  word32 opde;
  unsigned ptb;
  word32 far *pt;
  unsigned pta;

  fInPageOutEverything = 0;

  valloc_initted = 0;
  pta = valloc(VA_640);
  pd = (word32 far *)((word32)pta << 24);
  dread(paging_buffer, pd_dblock);
  dfree(pd_dblock);
  movedata(_DS, paging_buffer, FP_SEG(pd), FP_OFF(pd), 4096);
  for (pdi=0; pdi<1024; pdi++)
    if (pd[pdi] && !(pd[pdi] & PT_P))
    {
      pta = valloc(VA_640);
      opde = pd[pdi] & 0xfffff000L;
      pt = (word32 far *)((word32)pta << 24);
      ptb = opde >> 12;
      dread(paging_buffer, ptb);
      dfree(ptb);
      movedata(_DS, paging_buffer, FP_SEG(pt), FP_OFF(pt), 4096);
      if (pdi == 0)
        vcpi_pt = (word32 far *)((word32)pta << 24);
      pd[pdi] = pn2pte(pta, (pd[pdi] & 0xFFF) | PT_P);
      pd_seg[pdi] = pta;
      for (i=pdi+1; i<1024; i++)
        if ((pd[i] & 0xfffff000L) == opde)
          pd[i] = pd[pdi];
    }
  graphics_pt = (word32 far *)((long)pd_seg[0x380] << 24);
  graphics_pt_lin = ptr2linear(graphics_pt);
}


int emu_install(char *filename)
{
  GNU_AOUT eh;
  areas[A_emu].first_addr = EMU_TEXT+ARENA;
  areas[A_emu].last_addr = EMU_TEXT-1+ARENA;
  areas[A_emu].foffset = 0;

  if (filename == 0)
    return 0;
  emu_f = open(filename, O_RDONLY|O_BINARY);
  if (emu_f < 0)
  {
    fprintf(stderr, "Can't open 80387 emulator file <%s>\n", filename);
    return 0;
  }
  read(emu_f, &eh, sizeof(eh));
  areas[A_emu].last_addr += eh.tsize + eh.dsize + eh.bsize + 0x20;
  return 1;
}
