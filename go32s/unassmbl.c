/* This is file UNASSMBL.C */
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

/* History:255,3 */
#include <stdio.h>
#include <string.h>

#include "build.h"
#include "types.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "utils.h"
#include "unassmbl.h"
#include "syms.h"
#include "mono.h"

#if 0
#endif

#if DEBUGGER

int seg_size=32;

static word8 buf[20];
static word32 vaddr;
static int bufp, bufe;
static char ubuf[100], *ubufp;
static col;

/* Percent tokens in strings:
   First char after '%':
	A - direct address
	C - reg of r/m picks control register
	D - reg of r/m picks debug register
	E - r/m picks operand
	F - flags register
	G - reg of r/m picks general register
	I - immediate data
	J - relative IP offset
	M - r/m picks memory
	O - no r/m, offset only
	R - mod of r/m picks register only
	S - reg of r/m picks segment register
	T - reg of r/m picks test register
	X - DS:ESI
	Y - ES:EDI
	2 - prefix of two-byte opcode
	e - put in 'e' if use32 (second char is part of reg name)
	    put in 'w' for use16 or 'd' for use32 (second char is 'w')
	f - floating point (second char is esc value)
	g - do r/m group 'n'
	p - prefix
	s - size override (second char is a,o)
   Second char after '%':
	a - two words in memory (BOUND)
	b - byte
	c - byte or word
	d - dword
	p - 32 or 48 bit pointer
	s - six byte pseudo-descriptor
	v - word or dword
	w - word
	F - use floating regs in mod/rm
	1-8 - group number, esc value, etc
*/

char *opmap1[] = {
/* 0 */
  "add %Eb,%Gb", "add %Ev,%Gv", "add %Gb,%Eb", "add %Gv,%Ev",
  "add al,%Ib", "add %eax,%Iv", "push es", "pop es",
  "or %Eb,%Gb", "or %Ev,%Gv", "or %Gb,%Eb", "or %Gv,%Ev",
  "or al,%Ib", "or %eax,%Iv", "push cs", "%2 ",
/* 1 */
  "adc %Eb,%Gb", "adc %Ev,%Gv", "adc %Gb,%Eb", "adc %Gv,%Ev",
  "adc al,%Ib", "adc %eax,%Iv", "push ss", "pop ss",
  "sbb %Eb,%Gb", "sbb %Ev,%Gv", "sbb %Gb,%Eb", "sbb %Gv,%Ev",
  "sbb al,%Ib", "sbb %eax,%Iv", "push ds", "pop ds",
/* 2 */
  "and %Eb,%Gb", "and %Ev,%Gv", "and %Gb,%Eb", "and %Gv,%Ev",
  "and al,%Ib", "and %eax,%Iv", "%pe", "daa",
  "sub %Eb,%Gb", "sub %Ev,%Gv", "sub %Gb,%Eb", "sub %Gv,%Ev",
  "sub al,%Ib", "sub %eax,%Iv", "%pc", "das",
/* 3 */
  "xor %Eb,%Gb", "xor %Ev,%Gv", "xor %Gb,%Eb", "xor %Gv,%Ev",
  "xor al,%Ib", "xor %eax,%Iv", "%ps", "aaa",
  "cmp %Eb,%Gb", "cmp %Ev,%Gv", "cmp %Gb,%Eb", "cmp %Gv,%Ev",
  "cmp al,%Ib", "cmp %eax,%Iv", "%pd", "aas",
/* 4 */
  "inc %eax", "inc %ecx", "inc %edx", "inc %ebx",
  "inc %esp", "inc %ebp", "inc %esi", "inc %edi",
  "dec %eax", "dec %ecx", "dec %edx", "dec %ebx",
  "dec %esp", "dec %ebp", "dec %esi", "dec %edi",
/* 5 */
  "push %eax", "push %ecx", "push %edx", "push %ebx",
  "push %esp", "push %ebp", "push %esi", "push %edi",
  "pop %eax", "pop %ecx", "pop %edx", "pop %ebx",
  "pop %esp", "pop %ebp", "pop %esi", "pop %edi",
/* 6 */
  "pusha", "popa", "bound %Gv,%Ma", "arpl %Ew,%Rw",
  "%pf", "%pg", "%so", "%sa",
  "push %Iv", "imul %Gv=%Ev*%Iv", "push %Ib", "imul %Gv=%Ev*%Ib",
  "insb %Yb,dx", "ins%ew %Yv,dx", "outsb dx,%Xb", "outs%ew dx,%Xv",
/* 7 */
  "jo %Jb", "jno %Jb", "jnc %Jb", "jc %Jb",
  "jz %Jb", "jnz %Jb", "jbe %Jb", "jnbe %Jb",
  "js %Jb", "jns %Jb", "jpe %Jb", "jpo %Jb",
  "jl %Jb", "jge %Jb", "jle %Jb", "jg %Jb",
/* 8 */
  "%g1 %Eb,%Ib", "%g1 %Ev,%Iv", "mov al,%Ib", "%g1 %Ev,%Ib",
  "test %Eb,%Gb", "test %Ev,%Gv", "xchg %Eb,%Gb", "xchg %Ev,%Gv",
  "mov %Eb,%Gb", "mov %Ev,%Gv", "mov %Gb,%Eb", "mov %Gv,%Ev",
  "mov %Ew,%Sw", "lea %Gv,%M ", "mov %Sw,%Ew", "pop %Ev",
/* 9 */
  "nop", "xchg %eax,%ecx", "xchg %eax,%edx", "xchg %eax,%ebx",
  "xchg %eax,%esp", "xchg %eax,%ebp", "xchg %eax,%esi", "xchg %eax,%edi",
  "cbw", "cwd", "call %Ap", "fwait",
  "push %eflags", "pop %eflags", "sahf", "lahf",
/* a */
  "mov al,%Ob", "mov %eax,%Ov", "mov %Ob,al", "mov %Ov,%eax",
  "movsb %Xb,%Yb", "movs%ew %Xv,%Yv", "cmpsb %Xb,%Yb", "cmps%ew %Xv,%Yv",
  "test al,%Ib", "test %eax,%Iv", "stosb %Yb,al", "stos%ew %Yv,%eax",
  "lodsb al,%Xb", "lods%ew %eax,%Xv", "scasb al,%Xb", "scas%ew %eax,%Xv",
/* b */
  "mov al,%Ib", "mov cl,%Ib", "mov dl,%Ib", "mov bl,%Ib",
  "mov ah,%Ib", "mov ch,%Ib", "mov dh,%Ib", "mov bh,%Ib",
  "mov %eax,%Iv", "mov %ecx,%Iv", "mov %edx,%Iv", "mov %ebx,%Iv",
  "mov %esp,%Iv", "mov %ebp,%Iv", "mov %esi,%Iv", "mov %edi,%Iv",
/* c */
  "%g2 %Eb,%Ib", "%g2 %Ev,%Ib", "ret %Iw", "ret",
  "les %Gv,%Mp", "lds %Gv,%Mp", "mov %Eb,%Ib", "mov %Ev,%Iv",
  "enter %Iw,%Ib", "leave", "retf %Iw", "retf",
  "int 3", "int %Ib", "into", "iret",
/* d */
  "%g2 %Eb,1", "%g2 %Ev,1", "%g2 %Eb,cl", "%g2 %Ev,cl",
  "aam", "aad", 0, "xlat",
#if 0
  "esc 0,%Ib", "esc 1,%Ib", "esc 2,%Ib", "esc 3,%Ib",
  "esc 4,%Ib", "esc 5,%Ib", "esc 6,%Ib", "esc 7,%Ib",
#else
  "%f0", "%f1", "%f2", "%f3",
  "%f4", "%f5", "%f6", "%f7",
#endif
/* e */
  "loopne %Jb", "loope %Jb", "loop %Jb", "jcxz %Jb",
  "in al,%Ib", "in %eax,%Ib", "out %Ib,al", "out %Ib,%eax",
  "call %Jv", "jmp %Jv", "jmp %Ap", "jmp %Jb",
  "in al,dx", "in %eax,dx", "out dx,al", "out dx,%eax",
/* f */
  "lock %p ", 0, "repne %p ", "rep(e) %p ",
  "hlt", "cmc", "%g3", "%g0",
  "clc", "stc", "cli", "sti",
  "cld", "std", "%g4", "%g5"
  };

char *second[] = {
/* 0 */
  "%g6", "%g7", "lar %Gv,%Ew", "lsl %Gv,%Ew", 0, 0, "clts", 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 1 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 2 */
  "mov %Rd,%Cd", "mov %Rd,%Dd", "mov %Cd,%Rd", "mov %Dd,%Rd",
  "mov %Rd,%Td", 0, "mov %Td,%Rd", 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 3 */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
/* 8 */
  "jo %Jv", "jno %Jv", "jnc %Jv", "jc %Jv",
  "jz %Jv", "jnz %Jv", "jbe %Jv", "jnbe %Jv",
  "js %Jv", "jns %Jv", "jpe %Jv", "jpo %Jv",
  "jl %Jv", "jge %Jv", "jle %Jv", "jg %Jv",
/* 9 */
  "seto %Eb", "setno %Eb", "setnc %Eb", "setc %Eb",
  "setz %Eb", "setnz %Eb", "setbe %Eb", "setnbe %Eb",
  "sets %Eb", "setns %Eb", "setp %Eb", "setnp %Eb",
  "setl %Eb", "setge %Eb", "setle %Eb", "setg %Eb",
/* a */
  "push fs", "pop fs", 0, "bt %Ev,%Gv",
  "shld %Ev,%Gv,%Ib", "shld %Ev,%Gv,cl", 0, 0,
  "push gs", "pop gs", 0, "bts %Ev,%Gv",
  "shrd %Ev,%Gv,%Ib", "shrd %Ev,%Gv,cl", 0, "imul %Gv,%Ev",
/* b */
  0, 0, "lss %Mp", "btr %Ev,%Gv",
  "lfs %Mp", "lgs %Mp", "movzx %Gv,%Eb", "movzx %Gv,%Ew",
  0, 0, "%g8 %Ev,%Ib", "btc %Ev,%Gv",
  "bsf %Gv,%Ev", "bsr %Gv,%Ev", "movsx %Gv,%Eb", "movsx %Gv,%Ew",
/* c */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  };

char *groups[][8] = {   /* group 0 is group 3 for %Ev set */
  { "test %Ev,%Iv", "test %Ev,%Iv,", "not %Ev", "neg %Ev",
    "mul %eax,%Ev", "imul %eax,%Ev", "div %eax,%Ev", "idiv %eax,%Ev" },
  { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" },
  { "rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar" },
  { "test %Eb,%Ib", "test %Eb,%Ib,", "not %Eb", "neg %Eb",
    "mul al,%Eb", "imul al,%Eb", "div al,%Eb", "idiv al,%Eb" },
  { "inc %Eb", "dec %Eb", 0, 0, 0, 0, 0, 0 },
  { "inc %Ev", "dec %Ev", "call %Ev", "call %Ep",
    "jmp %Ev", "jmp %Ep", "push %Ev", 0 },
  { "sldt %Ew", "str %Ew", "lldt %Ew", "ltr %Ew",
    "verr %Ew", "verw %Ew", 0, 0 },
  { "sgdt %Ms", "sidt %Ms", "lgdt %Ms", "lidt %Ms",
    "smsw %Ew", 0, "lmsw %Ew", 0 },
  { 0, 0, 0, 0, "bt", "bts", "btr", "btc" }
  };

/* zero here means invalid.  If first entry starts with '*', use st(i) */
/* no assumed %EFs here.  Indexed by rm(modrm()) */
char *f0[] = {0, 0, 0, 0, 0, 0, 0, 0};
char *fop_9[]  = { "*fxch st,%GF" };
char *fop_10[] = { "fnop", 0, 0, 0, 0, 0, 0, 0 };
char *fop_12[] = { "fchs", "fabs", 0, 0, "ftst", "fxam", 0, 0 };
char *fop_13[] = { "fld1", "fldl2t", "fldl2e", "fldpi",
                   "fldlg2", "fldln2", "fldz", 0 };
char *fop_14[] = { "f2xm1", "fyl2x", "fptan", "fpatan",
                   "fxtract", "fprem1", "fdecstp", "fincstp" };
char *fop_15[] = { "fprem", "fyl2xp1", "fsqrt", "fsincos",
                   "frndint", "fscale", "fsin", "fcos" };
char *fop_21[] = { 0, "fucompp", 0, 0, 0, 0, 0, 0 };
char *fop_28[] = { 0, 0, "fclex", "finit", 0, 0, 0, 0 };
char *fop_32[] = { "*fadd %GF,st" };
char *fop_33[] = { "*fmul %GF,st" };
char *fop_36[] = { "*fsubr %GF,st" };
char *fop_37[] = { "*fsub %GF,st" };
char *fop_38[] = { "*fdivr %GF,st" };
char *fop_39[] = { "*fdiv %GF,st" };
char *fop_40[] = { "*ffree %GF" };
char *fop_42[] = { "*fst %GF" };
char *fop_43[] = { "*fstp %GF" };
char *fop_44[] = { "*fucom %GF" };
char *fop_45[] = { "*fucomp %GF" };
char *fop_48[] = { "*faddp %GF,st" };
char *fop_49[] = { "*fmulp %GF,st" };
char *fop_51[] = { 0, "fcompp", 0, 0, 0, 0, 0, 0 };
char *fop_52[] = { "*fsubrp %GF,st" };
char *fop_53[] = { "*fsubp %GF,st" };
char *fop_54[] = { "*fdivrp %GF,st" };
char *fop_55[] = { "*fdivp %GF,st" };
char *fop_60[] = { "fstsw ax", 0, 0, 0, 0, 0, 0, 0 };

char **fspecial[] = { /* 0=use st(i), 1=undefined 0 in fop_* means undefined */
  0, 0, 0, 0, 0, 0, 0, 0,
  0, fop_9, fop_10, 0, fop_12, fop_13, fop_14, fop_15,
  f0, f0, f0, f0, f0, fop_21, f0, f0,
  f0, f0, f0, f0, fop_28, f0, f0, f0,
  fop_32, fop_33, f0, f0, fop_36, fop_37, fop_38, fop_39,
  fop_40, f0, fop_42, fop_43, fop_44, fop_45, f0, f0,
  fop_48, fop_49, f0, fop_51, fop_52, fop_53, fop_54, fop_55,
  f0, f0, f0, f0, fop_60, f0, f0, f0,
  };

char *floatops[] = { /* assumed " %EF" at end of each.  mod != 3 only */
/*00*/ "fadd", "fmul", "fcom", "fcomp",
       "fsub", "fsubr", "fdiv", "fdivr",
/*08*/ "fld", 0, "fst", "fstp",
       "fldenv", "fldcw", "fstenv", "fstcw",
/*16*/ "fiadd", "fimul", "ficomw", "ficompw",
       "fisub", "fisubr", "fidiv", "fidivr",
/*24*/ "fild", 0, "fist", "fistp",
       "frstor", "fldt", 0, "fstpt",
/*32*/ "faddq", "fmulq", "fcomq", "fcompq",
       "fsubq", "fsubrq", "fdivq", "fdivrq",
/*40*/ "fldq", 0, "fstq", "fstpq",
       0, 0, "fsave", "fstsww",
/*48*/ "fiaddw", "fimulw", "ficomw", "ficompw",
       "fisubw", "fisubrw", "fidivw", "fidivr",
/*56*/ "fildw", 0, "fistw", "fistpw",
       "fbldt", "fildq", "fbstpt", "fistpq"
  };

static word8 getbyte()
{
  int s;
  if (bufp >= bufe)
  {
    s = 20;
    if ((vaddr & 0xfff) + s > 0x1000)
      s = 0x1000 - (vaddr & 0xfff);
    memget(vaddr+ARENA, buf, s);
    bufe = s;
    bufp = 0;
  }
  vaddr++;
  printf("%02x", buf[bufp]);
  col+=2;
  return buf[bufp++];
}

static prefix;
static modrmv;
static sibv;
static opsize;
static addrsize;

static modrm()
{
  if (modrmv == -1)
    modrmv = getbyte();
  return modrmv;
}

static sib()
{
  if (sibv == -1)
    sibv = getbyte();
  return sibv;
}

#define mod(a)	(((a)>>6)&7)
#define reg(a)	(((a)>>3)&7)
#define rm(a)	((a)&7)
#define ss(a)	(((a)>>6)&7)
#define indx(a)	(((a)>>3)&7)
#define base(a)	((a)&7)

/*------------------------------------------------------------------------*/
uprintf(char *s, ...)
{
  vsprintf(ubufp, s, ...);
  while (*ubufp) ubufp++;
}

uputchar(char c)
{
  if (c == '\t')
  {
    do {
      *ubufp++ = ' ';
    } while ((ubufp-ubuf) % 8);
  }
  else
    *ubufp++ = c;
  *ubufp = 0;
}

/*------------------------------------------------------------------------*/
int bytes(char c)
{
  switch (c)
  {
    case 'b':
      return 1;
    case 'w':
      return 2;
    case 'd':
      return 4;
    case 'v':
      if (opsize == 32)
        return 4;
      else
        return 2;
  }
  return 0;
}

/*------------------------------------------------------------------------*/
static ohex(char c, int extend, int optional, int defsize, int sign)
{
  int n=0, s=0, i, j;
  int32 delta;
  unsigned char buf[6];
  char *name;

  switch (c)
  {
    case 'a':
      break;
    case 'b':
      n = 1;
      break;
    case 'w':
      n = 2;
      break;
    case 'd':
      n = 4;
      break;
    case 's':
      n = 6;
      break;
    case 'c':
    case 'v':
      if (defsize == 32)
        n = 4;
      else
        n = 2;
      break;
    case 'p':
      if (defsize == 32)
        n = 6;
      else
        n = 4;
      s = 1;
      break;
  }
  for (i=0; i<n; i++)
    buf[i] = getbyte();
  for (; i<extend; i++)
    buf[i] = (buf[i-1] & 0x80) ? 0xff : 0;
  if (s)
  {
    uprintf("0x%02x%02x:", buf[n-1], buf[n-2]);
    n -= 2;
  }
  switch (n)
  {
    case 1:
      delta = *(signed char *)buf;
      break;
   case 2:
      delta = *(signed int *)buf;
      break;
    case 4:
      delta = *(signed long *)buf;
      break;
  }
  if (extend > n)
  {
    if (delta || !optional)
      uprintf("%+ld", delta);
    return;
  }
  if ((n == 4) && !sign)
  {
    name = syms_val2name(delta, &delta);
    if (name)
    {
      uprintf("%s", name);
      if (delta)
        uprintf("+%lu", delta);
      return;
    }
  }
  switch (n)
  {
    case 1:
      if (sign)
        uprintf("%+d", (unsigned char)delta);
      else
        uprintf("%#x", (unsigned char)delta);
      break;
    case 2:
      if (sign)
        uprintf("%+d", (unsigned int)delta);
      else
        uprintf("%#x", (unsigned int)delta);
      break;
    case 4:
      if (sign)
        uprintf("%+ld", (unsigned long)delta);
      else
        uprintf("%#lx", (unsigned long)delta);
      break;
  }
}

/*------------------------------------------------------------------------*/

static char *reg_names[3][8]={
  "al","cl","dl","bl","ah","ch","dh","bh",
  "ax","cx","dx","bx","sp","bp","si","di",
  "eax","ecx","edx","ebx","esp","ebp","esi","edi" };

reg_name(int which, char size)
{
  if (size == 'F')
  {
    uprintf("st(%d)", which);
    return;
  }
  if (((size == 'v') && (opsize == 32)) || (size == 'd'))
  {
    uputchar('e');
  }
  if (size == 'b')
  {
    uputchar("acdbacdb"[which]);
    uputchar("llllhhhh"[which]);
  }
  else
  {
    uputchar("acdbsbsd"[which]);
    uputchar("xxxxppii"[which]);
  }
}

/*------------------------------------------------------------------------*/
do_sib(m)
{
  int s, i, b;
  s = ss(sib());
  i = indx(sib());
  b = base(sib());
  switch (b)
  {
    case 0: ua_str("%p:[eax"); break;
    case 1: ua_str("%p:[ecx"); break;
    case 2: ua_str("%p:[edx"); break;
    case 3: ua_str("%p:[ebx"); break;
    case 4: ua_str("%p:[esp"); break;
    case 5:
      if (m == 0)
      {
        ua_str("%p:[");
        ohex('d', 4, 0, addrsize, 0);
      }
      else
        ua_str("%p:[ebp");
      break;
    case 6: ua_str("%p:[esi"); break;
    case 7: ua_str("%p:[edi"); break;
  }
  switch (i)
  {
    case 0: uprintf("+eax"); break;
    case 1: uprintf("+ecx"); break;
    case 2: uprintf("+edx"); break;
    case 3: uprintf("+ebx"); break;
    case 4: break;
    case 5: uprintf("+ebp"); break;
    case 6: uprintf("+esi"); break;
    case 7: uprintf("+edi"); break;
  }
  if (i != 4)
    switch (s)
    {
      case 0: uprintf(""); break;
      case 1: uprintf("*2"); break;
      case 2: uprintf("*4"); break;
      case 3: uprintf("*8"); break;
    }
}
/*------------------------------------------------------------------------*/
do_modrm(char t)
{
  int m = mod(modrm());
  int r = rm(modrm());
  int extend = (addrsize == 32) ? 4 : 2;
  if (m == 3)
  {
    reg_name(r, t);
    return;
  }
  if ((m == 0) && (r == 5) && (addrsize == 32))
  {
    ua_str("%p:[");
    ohex('d', extend, 0, addrsize, 0);
    uputchar(']');
    return;
  }
  if ((m == 0) && (r == 6) && (addrsize == 16))
  {
    ua_str("%p:[");
    ohex('w', extend, 0, addrsize, 0);
    uputchar(']');
    return;
  }
  if ((addrsize != 32) || (r != 4))
    ua_str("%p:[");
  if (addrsize == 16)
  {
    switch (r)
    {
      case 0: uprintf("bx+si"); break;
      case 1: uprintf("bx+di"); break;
      case 2: uprintf("bp+si"); break;
      case 3: uprintf("bp+di"); break;
      case 4: uprintf("si"); break;
      case 5: uprintf("di"); break;
      case 6: uprintf("bp"); break;
      case 7: uprintf("bx"); break;
    }
  }
  else
  {
    switch (r)
    {
      case 0: uprintf("eax"); break;
      case 1: uprintf("ecx"); break;
      case 2: uprintf("edx"); break;
      case 3: uprintf("ebx"); break;
      case 4: do_sib(m); break;
      case 5: uprintf("ebp"); break;
      case 6: uprintf("esi"); break;
      case 7: uprintf("edi"); break;
    }
  }
  switch (m)
  {
    case 1:
      ohex('b', extend, 1, addrsize, 0);
      break;
    case 2:
      ohex('v', extend, 1, addrsize, 1);
      break;
  }
  uputchar(']');
}

/*------------------------------------------------------------------------*/
static floating_point(int e1)
{
  int esc = e1*8 + reg(modrm());
  if (mod(modrm()) == 3)
  {
    if (fspecial[esc])
    {
      if (fspecial[esc][0][0] == '*')
      {
        ua_str(fspecial[esc][0]+1);
      }
      else
      {
        ua_str(fspecial[esc][rm(modrm())]);
      }
    }
    else
    {
      ua_str(floatops[esc]);
      ua_str(" %EF");
    }
  }
  else
  {
    ua_str(floatops[esc]);
    ua_str(" %EF");
  }
}

/*------------------------------------------------------------------------*/
static percent(char c, char t)
{
  word32 vofs, v, delta;
  char *name;
  int extend = (addrsize == 32) ? 4 : 2;
  switch (c)
  {
    case 'A':
      ohex(t, extend, 0, addrsize, 0);
      break;
    case 'C':
      uprintf("cr%d", reg(modrm()));
      break;
    case 'D':
      uprintf("dr%d", reg(modrm()));
      break;
    case 'E':
      do_modrm(t);
      break;
    case 'G':
      if (t == 'F')
        reg_name(rm(modrm()), t);
      else
        reg_name(reg(modrm()), t);
      break;
    case 'I':
      ohex(t, 0, 0, opsize, 0);
      break;
    case 'J':
      switch (bytes(t))
      {
        case 1:
          vofs = (int8)getbyte();
          break;
        case 2:
          vofs = getbyte();
          vofs += getbyte()<<8;
          vofs = (int16)vofs;
          break;
        case 4:
          vofs = (word32)getbyte();
          vofs |= (word32)getbyte() << 8;
          vofs |= (word32)getbyte() << 16;
          vofs |= (word32)getbyte() << 24;
          break;
      }
      name = syms_val2name(vofs+vaddr, &delta);
      uprintf("%s", name);
      if (delta)
        uprintf("+%lu (0x%lx %c)", delta, vofs+vaddr,
                (vofs & 0x80000000L) ? 0x1e : 0x1f);
      break;
    case 'M':
      do_modrm(t);
      break;
    case 'O':
      ua_str("%p:[");
      ohex(t, extend, 0, addrsize, 0);
      uputchar(']');
      break;
    case 'R':
      do_modrm(t);
      break;
    case 'S':
      uputchar("ecsdfg"[reg(modrm())]);
      uputchar('s');
      break;
    case 'T':
      uprintf("tr%d", reg(modrm()));
      break;
    case 'X':
      uprintf("ds:[");
      if (addrsize == 32)
        uputchar('e');
      uprintf("si]");
      break;
    case 'Y':
      uprintf("es:[");
      if (addrsize == 32)
        uputchar('e');
      uprintf("di]");
      break;
    case '2':
      ua_str(second[getbyte()]);
      break;
    case 'e':
      if (opsize == 32)
      {
        if (t == 'w')
          uputchar('d');
        else
        {
          uputchar('e');
          uputchar(t);
        }
      }
      else
        uputchar(t);
      break;
    case 'f':
      floating_point(t-'0');
      break;
    case 'g':
      ua_str(groups[t-'0'][reg(modrm())]);
      break;
    case 'p':
      switch (t)
      {
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 's':
          prefix = t;
          ua_str(opmap1[getbyte()]);
          break;
        case ':':
          if (prefix)
            uprintf("%cs:", prefix);
          break;
        case ' ':
          ua_str(opmap1[getbyte()]);
          break;
      }
      break;
    case 's':
      switch (t)
      {
        case 'a':
          addrsize = 48 - addrsize;
          ua_str(opmap1[getbyte()]);
          break;
        case 'o':
          opsize = 48 - opsize;
          ua_str(opmap1[getbyte()]);
          break;
      }
      break;
  }
}

static ua_str(char *s)
{
  int i, c;
  if (s == 0)
  {
    uprintf("<invalid>");
    return;
  }
  while ((c = *s++) != 0)
  {
    if (c == '%')
    {
      c = *s++;
      percent(c, *s++);
    }
    else
      if (c == ' ')
        uputchar('\t');
      else
        uputchar(c);
  }
}
  
#ifdef SOURCE_LIST
/*
** A little brute force hacking and hey presto! A source debugger!
** Courtesy of Kent Williams williams@herky.cs.uiowa.edu
**
** KNOWN BUGS:
** The program will summarily terminate if you run out
** of memory while you're looking for all the line offsets.  Since
** a two thousand line source file only creats an 8K array, and the
** symbol table goes into virtual memory, this shouldn't happen too
** often.
**
** One file is left open for reading indefinitely.
*/
#include <stdlib.h>
#include <string.h>
/*
** keep the source line offsets in virtual memory, so you can
** debug big programs
*/
extern word32 salloc(word32 size);
extern void symsput(word32 where, void *ptr, int size);
extern void symsget(word32 where, void *ptr, int size);

/*
** for each file encountered, keep an array of line start offsets
** so you can seek into the file to display the current line.
*/
typedef struct {
	char *filename;
	word32 offsets;
} line_info;

static line_info files[32];
static last_file = 0;

/*
** these don't seem to be defined anywhere else ...
*/
static void *
xmalloc(unsigned size) {
	void *rval = malloc(size);
	if(!rval) {
		fprintf(stderr,"Out of memory\n");
		exit(1);
	}
	return rval;
}

static void *
xrealloc(void *rval,unsigned size) {
	rval = realloc(rval,size);
	if(!rval) {
		fprintf(stderr,"Out of memory\n");
		exit(1);
	}
	return rval;
}

static char *
xstrdup(char *s) { 
	char *rval = xmalloc(strlen(s)+1);
	strcpy(rval,s);
	return rval;
}

/*
** add_file -- add a file to the source line database
*/
static int
add_file(char *name) {
	FILE *f = fopen(name,"r");
	char c;
	long *lines,curpos;
	unsigned curline = 0;

	if(!f)
		return -1;

	files[last_file].filename = xstrdup(name);
	/*
	** build an array of line offsets in real memory.
	*/
	lines = xmalloc(sizeof(long));
	lines[curline++] = curpos = 0L;

	while((c = getc(f)) != EOF) {
		curpos++;
		if(c == '\n') {
			lines = xrealloc(lines,sizeof(long)*(curline+1));
			lines[curline++] = curpos;
		}
	}
	/*
	** now move the whole array into virtual memory
	*/
	files[last_file].offsets = salloc(curline*sizeof(long));
	symsput(files[last_file].offsets,lines,(curline*sizeof(long)));
	/*
	** don't need the real memory version any more
	*/
	free(lines);

	fclose(f);

	last_file++;
	return 0;
}

static line_info *
find_file(char *name) {
	int i;
	for(i = 0; i < last_file; i++)
		if(strcmp(name,files[i].filename) == 0)
			return &files[i];
	if(add_file(name) == -1)
			return NULL;
	return find_file(name);
}

/*
** myfopen -- cache the most recently accessed source file
** so you aren't constantly reopening a new file
*/
static FILE *
myfopen(char *name) {
	static char fname[80] = "";
	static FILE *current = NULL;
	if(current != NULL && strcmp(fname,name) == 0)
			return current;
	if(current != NULL) fclose(current);
	strcpy(fname,name);
	return (current = fopen(name,"r"));
}

/*
** put_source_line -- print the current source line, along with
** the line # and file name, if necessary.
*/
void
put_source_line(int fmt,char *name,int line) {
	line_info *current = find_file(name);
	FILE *cur;
	if(current == NULL) {
	regular:
	  if(fmt == 0)
		  printf(" (%s#%d):\n", name, line);
	  else
		  printf("#%d:\n", line);
	} else {
		char buf[70];
		long offset;
		if((cur = myfopen(name)) == NULL)
			goto regular;
		/*
		** get the symbol out of virtual memory
		*/
		symsget(current->offsets+((line - 1) * sizeof(long)),
			&offset,sizeof(long));
		fseek(cur,offset,SEEK_SET);
		/*
		** truncate line so it fits on screen.
		*/
		fgets(buf,sizeof(buf)-2,cur);
		if(strchr(buf,'\n') == NULL)
			strcat(buf,"\n");
		if(fmt == 0)
			printf(" (%s#%d): %s", name, line,buf);
		else
			printf("#%d: %s",line,buf);
	}
}

#endif


int last_unassemble_unconditional;
int last_unassemble_jump;
int last_unassemble_extra_lines;

word32 unassemble(word32 v, int showregs)
{
  int a,b,n,wi, linenum;
  char *cmp, *brp;
  word8 *wp;
  word32 delta;
  char *name, *lname;

  last_unassemble_unconditional = 0;
  last_unassemble_jump = 0;
  last_unassemble_extra_lines = 0;
  name = syms_val2name(v, &delta);
  if (use_ansi)
    printf("\033[1;36m");
  mono_attr = MONO_BOLD;
  if (!delta && (name[0] != '0'))
  {
    printf("%s()", name);
    lname = syms_val2line(v, &linenum, 1);
    if (lname)
#ifndef SOURCE_LIST
      printf(" (%s#%d):\n", lname, linenum);
#else
      put_source_line(0,lname,linenum);
#endif
    else
      printf(":\n");
    last_unassemble_extra_lines++;
  }
  else
  {
    lname = syms_val2line(v, &linenum, 1);
    if (lname)
    {
#ifndef SOURCE_LIST
      printf("#%d:\n", linenum);
#else
      put_source_line(1,lname,linenum);
#endif
      last_unassemble_extra_lines++;
    }
  }

  if (use_ansi)
    printf("\033[0m");
  mono_attr = MONO_NORMAL;
  printf("%08lx: ", v);
  if (!page_is_valid(v+ARENA) || !page_is_valid(v+5+ARENA))
  {
    printf("<bad address>\n");
    return v;
  }

  prefix = 0;
  modrmv = sibv = -1;
  opsize = addrsize = seg_size;
  vaddr = v;
  bufp = bufe = 0;
  col = 0;
  ubufp = ubuf;
  ua_str(opmap1[getbyte()]);
  do {
    mputchar(' ');
    col++;
  } while (col < 15);
  col += strlen(ubuf);
  do {
    uputchar(' ');
    col++;
  } while (col < 43);
  if (use_ansi)
    printf("\033[1m");
  mono_attr = MONO_BOLD;
  printf("%s", ubuf);
  if (use_ansi)
    printf("\033[0m");
  mono_attr = MONO_NORMAL;

  if ((strncmp(ubuf, "jmp ", 4) == 0)
     || (strncmp(ubuf, "ret", 3) == 0))
    last_unassemble_unconditional = 1;
  if (ubuf[0] == 'j')
    last_unassemble_jump = 1;

  if (!showregs)
  {
    mputchar('\n');
    return vaddr;
  }

  col -= 43; /* total 25 columns left */
  wp = (word8 *)&(tss_ptr->tss_eax);
  cmp = strchr(ubuf+8, ',');
  brp = strchr(ubuf+8, '[');
  if (!cmp) cmp = ubuf+8;
  if (!brp) brp = ubufp;
  if (brp < cmp) cmp = brp;
  if (strncmp(ubuf, "mov ", 4))
    cmp = ubuf+8;
  for (b=0; b<8; b++)
  {
    for (a=2; a>=0; a--)
    {
      n = (a==0) ? 1 : ((a==1) ? 2 : 4);
      if (strstr(cmp, reg_names[a][b]))
      {
        col += strlen(reg_names[a][b])+n*2+2;
        if (col > 29)
        {
          printf("\n%53s", "");
          col = 0;
        }
        printf("%s=", reg_names[a][b]);
        if (a == 0)
          wi = (b&3)*4 + (b>>2);
        else
          wi = b*4;
        while (n)
        {
          n--;
          printf("%02x", wp[wi+n]);
        }
        mputchar(' ');
        break;
      }
    }
  }
  mputchar('\n');
  return vaddr;
}

#endif /* DEBUGGER */
