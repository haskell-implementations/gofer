/* This is file DEBUG.C */
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

/* History:175,26 */
#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include <dos.h>

#include "build.h"
#include "types.h"
#include "gdt.h"
#include "tss.h"
#include "utils.h"
#include "unassmbl.h"
#include "syms.h"
#include "paging.h"
#include "npx.h"
#include "mono.h"

extern int was_exception, have_80387;
jmp_buf back_to_debugger;
int can_longjmp=0;

#if DEBUGGER

#ifndef  NOEVENTS
#include "eventque.h"
extern   EventQueue *event_queue;
#endif

extern word32 dr[8];
extern word32 dr0, dr1, dr2, dr3, dr6, dr7;

typedef struct {
	char *cp;
	int t;
	} item;

my_getline(char *buf, char *lasttoken)
{
  int idx, i, ch;
#ifndef NOEVENTS
  unsigned char old_enable;

  if(event_queue != NULL) {
    old_enable = event_queue->evq_enable;
    event_queue->evq_enable = 0;
  }
#endif
  if (use_ansi)
    printf("\033[0;32m");
  mono_attr = MONO_NORMAL;
  printf(">> %s", lasttoken);
  for (i=0; lasttoken[i]; i++)
    mputchar(8);
  while (!bioskey(1));
  for (i=0; lasttoken[i]; i++)
    mputchar(' ');
  for (i=0; lasttoken[i]; i++)
    mputchar(8);
  idx = 0;
  if (use_ansi)
    printf("\033[1;33m");
  mono_attr = MONO_BOLD;
  while (1)
  {
    ch = bioskey(0) & 0xff;
    switch (ch)
    {
      case 10:
      case 13:
        buf[idx] = 0;
        if (!idx && lasttoken[0])
          printf("\r  \r");
        else
          mputchar('\n');
        if (use_ansi)
          printf("\033[0m");
        mono_attr = MONO_NORMAL;
#ifndef NOEVENTS
        if(event_queue != NULL)
          event_queue->evq_enable = old_enable;
#endif
        return;
      case 27:
        while (idx)
        {
          printf("\b \b");
          idx--;
        }
        break;
      case 8:
        if (idx)
        {
          printf("\b \b");
          idx--;
        }
        break;
      default:
        mputchar(ch);
        buf[idx++] = ch;
        break;
    }
  }
}

typedef enum { Zero, Unknown, CONT, STEP, NEXT, REGS, SET, HELP, LIST,
DUMP, QUIT, BREAK, STATUS, WHERE, DUMP_A, DUMP_B, DUMP_W, WHEREIS, XNPX,
CLS };

extern struct {
  char *name;
  int size;
  int ofs;
  } regs[];

item cmds[] = {
	"g", CONT,
	"go", CONT,
	"cont", CONT,
	"c", CONT,
	"step", STEP,
	"s", STEP,
	"next", NEXT,
	"n", NEXT,
	"regs", REGS,
	"r", REGS,
	"set", SET,
	"help", HELP,
	"h", HELP,
	"?", HELP,
	"list", LIST,
	"l", LIST,
	"u", LIST,
	"dump", DUMP,
	"d", DUMP,
	"da", DUMP_A,
	"db", DUMP_B,
	"dw", DUMP_W,
	"dd", DUMP,
	"quit", QUIT,
	"q", QUIT,
	"break", BREAK,
	"b", BREAK,
	"status", STATUS,
	"where", WHERE,
	"whereis", WHEREIS,
	"npx", XNPX,
	"cls", CLS,
	0, 0
	};

extern int debug_mode;

debugger()
{
  char buf[140], token[10];
  char buf2[140], *name, lasttoken[140];
  int i, n, s, len, rem_cmd, cmd, vstep, found;
  word32 vaddr, v, rem_v, olddr7;
  int32 delta;

  dr0 = dr1 = dr2 = ARENA;
  dr3 = syms_name2val("_main") + ARENA;
  if (undefined_symbol)
    dr3 = tss_ptr->tss_eip + ARENA;
  rem_cmd = Zero;
  lasttoken[0] = 0;
  setjmp(back_to_debugger);
  can_longjmp = 1;
  while (1)
  {
    if (debug_mode)
    {
      int found;
      my_getline(buf, lasttoken);
      token[0] = 0;
      if (sscanf(buf, "%s %[^\n]", token, buf) < 2)
        buf[0] = 0;
      if (token[0])
        strcpy(lasttoken, token);
      cmd = rem_cmd;
      found = 0;
      for (i=0; cmds[i].cp; i++)
        if (strcmp(cmds[i].cp, token) == 0)
        {
          cmd = cmds[i].t;
          found = 1;
        }
      if (!found && token[0])
        cmd = Unknown;
      if (rem_cmd != cmd)
        vaddr = tss_ptr->tss_eip;
    }
    else
    {
      cmd = CONT;
      debug_mode = 1;
    }
    switch (cmd)
    {
      case HELP:
        printf("Commands:\n");
        printf("go <v>\tg\tgo, stop at <v>\n");
        printf("cont\tc\tcontinue execution\n");
        printf("step\ts\tstep through current instruction\n");
        printf("next\tn\tstep to next instruction\n");
        printf("list\tl u\tlist instructions (takes addr, count)\n");
        printf("dump\td\tdump memory (takes addr, count)\n");
        printf("break\tb\tset breakpoint (takes which, addr)\n");
        printf("status\t\tbreakpoint status\n");
        printf("regs\tr\tprint registers\n");
        printf("set\t\tset register/memory\n");
        printf("npx\t\tdisplay 80387 contents\n");
        printf("where\t\tdisplay list of active functions\n");
        printf("whereis\t\tfind a symbol/location (takes wildcard or value)\n");
        printf("cls\t\tclear screen\n");
        printf("help\th,?\tprint help\n");
        printf("quit\tq\tquit\n");
        break;
      case CONT:
        sscanf(buf, "%s", buf);
        if (buf[0])
        {
          v = syms_name2val(buf);
          if (undefined_symbol)
            break;
          dr3 = v+ARENA;
          dr7 |= 0xc0;
        }
        else
          dr7 &= ~0xc0;
        olddr7 = dr7;
        dr7 = 0;
        tss_ptr->tss_eflags |= 0x0100;
        go_til_stop();
        dr7 = olddr7;
        if (tss_ptr->tss_irqn == 1)
        {
          tss_ptr->tss_eflags &= ~0x0100;
          tss_ptr->tss_eflags |= 0x10000;
          go_til_stop();
          if (tss_ptr->tss_irqn == 1)
            tssprint(tss_ptr);
        }
        print_reason();
        dr3 = unassemble(tss_ptr->tss_eip, 1) + ARENA;
        break;
      case STEP:
        if (rem_cmd != cmd)
          n = 1;
        sscanf(buf, "%d", &n);
        tss_ptr->tss_eflags |= 0x0100;
        for (i=0; i<n; i++)
        {
          olddr7 = dr7;
          dr7 = 0;
          go_til_stop();
          dr7 = olddr7;
          print_reason();
          dr3 = unassemble(tss_ptr->tss_eip, 1) + ARENA;
          if (tss_ptr->tss_irqn != 1)
            break;
        }
        tss_ptr->tss_eflags &= ~0x0100;
        break;
      case NEXT:
        if (rem_cmd != cmd)
          n = 1;
        sscanf(buf, "%d", &n);
        for (i=0; i<n; i++)
        {
          olddr7 = dr7;
          dr7 &= ~0xc0;
          dr7 |= 0xc0;
          if (last_unassemble_unconditional ||
              last_unassemble_jump)
            tss_ptr->tss_eflags |= 0x0100; /* step */
          else
            tss_ptr->tss_eflags &= ~0x0100;
          go_til_stop();
          dr7 = olddr7;
          print_reason();
          dr3 = unassemble(tss_ptr->tss_eip, 1) + ARENA;
          if (tss_ptr->tss_irqn != 1)
            break;
        }
        tss_ptr->tss_eflags &= ~0x0100;
        break;
      case WHERE:
        v = tss_ptr->tss_ebp;
        vaddr = tss_ptr->tss_eip;
        printf("0x%08lx %s", vaddr, syms_val2name(vaddr, &delta));
        name = syms_val2line(vaddr, &i, 0);
        if (name)
          printf(", line %d in file %s", i, name);
        else if (delta)
          printf("%+ld", delta);
        mputchar('\n');
        do {
          if (v == 0)
            break;
          rem_v = peek32(v+ARENA);
          if (rem_v == 0)
            break;
          vaddr = peek32(v+ARENA+4);
          printf("0x%08lx %s", vaddr, syms_val2name(vaddr, &delta));
          name = syms_val2line(vaddr, &i, 0);
          if (name)
            printf(", line %d in file %s", i, name);
          else if (delta)
            printf("%+ld", delta);
          mputchar('\n');
          v = rem_v;
        } while ((v>=tss_ptr->tss_esp) && (v<0x90000000L));
        break;
      case WHEREIS:
        sscanf(buf, "%s", buf2);
        if (strpbrk(buf2, "*?"))
        {
          syms_listwild(buf2);
          break;
        }
        if (buf2[0])
          vaddr = syms_name2val(buf2);
        if (undefined_symbol)
          break;
        name = syms_val2name(vaddr, &delta);
        printf("0x%08lx %s", vaddr, name);
        if (delta)
          printf("+%lx", delta);
        name = syms_val2line(vaddr, &i, 0);
        if (name)
          printf(", line %d in file %s", i, name);
        mputchar('\n');
        break;
      case LIST:
        if (rem_cmd != cmd)
          n = 10;
        buf2[0] = 0;
        sscanf(buf, "%s %d", buf2, &n);
        if (buf2[0] && strcmp(buf2, "."))
          vaddr = syms_name2val(buf2);
        if (undefined_symbol)
          break;
        for (i=0; i<n; i++)
        {
          vaddr = unassemble(vaddr, 0);
          i += last_unassemble_extra_lines;
/*          if (last_unassemble_unconditional)
            break; */
        }
        break;
      case DUMP_A:
        buf2[0] = 0;
        sscanf(buf, "%s %d", buf2, &n);
        if (buf2[0])
          vaddr = syms_name2val(buf2);
        if (undefined_symbol)
          break;
        while (1)
        {
          word8 ch;
          if (!page_is_valid(vaddr+ARENA))
          {
            printf("<bad address>\n");
            break;
          }
          ch = peek8(vaddr+ARENA);
          if (ch == 0)
          {
            mputchar('\n');
            break;
          }
          if (ch < ' ')
            printf("^%c", ch+'@');
          else if ((ch >= ' ') && (ch < 0x7f))
            mputchar(ch);
          else if (ch == 0x7f)
            printf("^?");
          else if ((ch >= 0x80) && (ch < 0xa0))
            printf("M-^%c", ch-0x80+'@');
          else if (ch >= 0xa0)
            printf("M-%c", ch-0x80);
          vaddr++;
        }
        break;
      case DUMP:
      case DUMP_B:
      case DUMP_W:
        if (rem_cmd != cmd)
          n = 4;
        buf2[0] = 0;
        sscanf(buf, "%s %d", buf2, &n);
        if (buf2[0])
          vaddr = syms_name2val(buf2);
        if (undefined_symbol)
          break;
        s = 0;
        len = n + (~((vaddr&15)/4-1) & 3);
        for (i=-(vaddr&15)/4; i<len; i++)
        {
          if ((s&3) == 0)
            printf("0x%08lx:", vaddr+i*4);
          if ((i>=0) && (i<n))
            printf(" 0x%08lx", peek32(vaddr+i*4+ARENA));
          else
            printf("           ");
          if ((s & 3) == 3)
          {
            int j, c;
            printf("  ");
            for (j=0; j<16; j++)
              if ((j+i*4-12>=0) && (j+i*4-12 < n*4))
              {
                c = peek8(vaddr+j+i*4-12+ARENA);
                if (c<' ')
                  mputchar('.');
                else
                  mputchar(c);
              }
              else
                mputchar(' ');
            printf("\n");
          }
          s++;
        }
        if (s & 3)
          printf("\n");
        vaddr += n*4;
        break;
      case BREAK:
        vaddr = n = 0;
        buf2[0] = 0;
        sscanf(buf, "%d %s", &n, buf2);
        if (buf2[0])
          vaddr = syms_name2val(buf2);
        if (undefined_symbol)
          break;
        dr[n] = vaddr + ARENA;
        if (vaddr == 0)
          dr7 &= ~(2 << (n*2));
        else
          dr7 |= 2 << (n*2);
      case STATUS:
        s = 0;
        for (n=0; n<4; n++)
        {
          s = 1;
          name = syms_val2name(dr[n]-ARENA, &delta);
          printf("  dr%d  %s", n, name);
          if (delta)
            printf("+%#lx", delta);
          if (name[0] != '0')
            printf(" (0x%lx)", dr[n] - ARENA);
          if (!(dr7 & (3 << (n*2))))
            printf(" (disabled)");
          mputchar('\n');
        }
        if (s == 0)
          printf("  No breakpoints set\n");
        break;
      case REGS:
        tssprint(tss_ptr);
        unassemble(tss_ptr->tss_eip, 0);
        break;
      case SET:
        cmd = Zero;
        lasttoken[0] = 0;
        buf2[0] = 0;
        len = sscanf(buf, "%s %s", buf2, buf);
        if (buf2[0] == 0)
        {
          break;
        }
        if (len > 1)
        {
          v = syms_name2val(buf);
          if (undefined_symbol)
            break;
        }
        found = 0;
        for (i=0; regs[i].name; i++)
          if (strcmp(regs[i].name, buf2) == 0)
          {
            found = 1;
            if (len > 1)
            {
              switch (regs[i].size)
              {
                case 1:
                  *(word8 *)((word8 *)tss_ptr + regs[i].ofs) = v;
                  break;
                case 2:
                  *(word16 *)((word8 *)tss_ptr + regs[i].ofs) = v;
                  break;
                case 4:
                  *(word32 *)((word8 *)tss_ptr + regs[i].ofs) = v;
                  break;
              }
            }
            else
            {
              switch (regs[i].size)
              {
                case 1:
                  printf("%02x ", *(word8 *)((word8 *)tss_ptr + regs[i].ofs));
                  my_getline(buf, "");
                  if (buf[0])
                  {
                    v = syms_name2val(buf);
                    if (undefined_symbol)
                      break;
                    *(word8 *)((word8 *)tss_ptr + regs[i].ofs) = v;
                  }
                  break;
                case 2:
                  printf("%04x ", *(word16 *)((word16 *)tss_ptr + regs[i].ofs));
                  my_getline(buf, "");
                  if (buf[0])
                  {
                    v = syms_name2val(buf);
                    if (undefined_symbol)
                      break;
                    *(word16 *)((word16 *)tss_ptr + regs[i].ofs) = v;
                  }
                  break;
                case 4:
                  printf("%08lx ", *(word32 *)((word32 *)tss_ptr + regs[i].ofs));
                  my_getline(buf, "");
                  if (buf[0])
                  {
                    v = syms_name2val(buf);
                    if (undefined_symbol)
                      break;
                    *(word32 *)((word32 *)tss_ptr + regs[i].ofs) = v;
                  }
                  break;
              }
            }
            break;
          }
        if (found)
          break;
        vaddr = syms_name2val(buf2);
        if (undefined_symbol)
          break;
        if (len < 2)
        {
          v = syms_name2val(buf);
          if (undefined_symbol)
            break;
          poke32(vaddr, v);
        }
        while (1)
        {
          printf("0x%08lx 0x%08lx", vaddr, peek32(vaddr+ARENA));
          my_getline(buf, "");
          if (buf[0])
          {
            if (strcmp(buf, ".") == 0)
              break;
            poke32(vaddr+ARENA, syms_name2val(buf));
          }
          vaddr += 4;
        }
        break;
      case XNPX:
        if (!have_80387)
        {
          printf("No 80387 present\n");
          break;
        }
        save_npx();
        printf("Control: 0x%04lx  Status: 0x%04lx  Tag: 0x%04lx\n",
               npx.control, npx.status, npx.tag);
        for (i=0; i<8; i++)
        {
          double d;
          int tag;
          int tos = (npx.status >> 11) & 7;
          printf("st(%d)  ", i);
          if (npx.reg[i].sign)
            mputchar('-');
          else
            mputchar('+');
          printf(" %04x %04x %04x %04x e %04x    ",
                 npx.reg[i].sig3,
                 npx.reg[i].sig2,
                 npx.reg[i].sig1,
                 npx.reg[i].sig0,
                 npx.reg[i].exponent);
          tag = (npx.tag >> (((i+tos)%8)*2)) & 3;
          switch (tag)
          {
            case 0:
              printf("Valid");
              if (((int)npx.reg[i].exponent-16382 < 1000) &&
                  ((int)npx.reg[i].exponent-16382 > -1000))
              {
                d = npx.reg[i].sig3/65536.0 + npx.reg[i].sig2/65536.0/65536.0
                  + npx.reg[i].sig1/65536.0/65536.0/65536.0;
                d = ldexp(d,(int)npx.reg[i].exponent-16382);
                if (npx.reg[i].sign)
                  d = -d;
                printf("  %.16lg", d);
              }
              else
                printf("  (too big to display)");
              mputchar('\n');
              break;
            case 1:
              printf("Zero\n");
              break;
            case 2:
              printf("Special\n");
              break;
            case 3:
              printf("Empty\n");
              break;
          }
        }
        break;
      case CLS:
        if (use_mono)
          mputchar(12);
        else
        {
          _AH=15;
          geninterrupt(0x10);
          _AH = 0;
          geninterrupt(0x10);
          mputchar('\n');
        }
        break;
      case QUIT:
        return;
      case Zero:
        break;
      default:
        printf("Unknown command\n");
        lasttoken[0] = 0;
        cmd = Zero;
        break;
    }
    if (undefined_symbol)
    {
      lasttoken[0] = 0;
      cmd = Zero;
      undefined_symbol = 0;
    }
    rem_cmd = cmd;
    debug_mode = 1;
  }
}
#endif /* DEBUGGER */

go_til_stop()
{
  static long farzero=0;
  tss_ptr = &a_tss;
  while (1)
  {
    int ehr;
    go32();
    if (!was_exception)
      return;
    if (exception_handler())
      return;
  }
}

#if DEBUGGER
print_reason()
{
  int n, i;
  i = tss_ptr->tss_irqn;
  if ((i == 0x21) && ((tss_ptr->tss_eax & 0xff00) == 0x4c00))
  {
    tss_ptr->tss_eip -= 2; /* point to int 21h */
    return;
  }
  if (use_ansi)
    printf("\033[1;34m");
  mono_attr = MONO_BOLD;
  if (i != 1)
  {
    tssprint(tss_ptr);
    if (tss_ptr != &a_tss)
    {
      printf("internal error: tss_ptr NOT a_tss\n");
    }
    if (i == 0x79)
      printf("Keyboard interrupt\n");
    else if (i == 0x75)
    {
      save_npx();
      printf("Numeric Exception (");
      if ((npx.status & 0x0241) == 0x0241)
        printf("stack overflow");
      else if ((npx.status & 0x0241) == 0x0041)
        printf("stack underflow");
      else if (npx.status & 1)
        printf("invalid operation");
      else if (npx.status & 2)
        printf("denormal operand");
      else if (npx.status & 4)
        printf("divide by zero");
      else if (npx.status & 8)
        printf("overflow");
      else if (npx.status & 16)
        printf("underflow");
      else if (npx.status & 32)
        printf("loss of precision");
      printf(") at eip=0x%08lx\n", npx.eip);
      unassemble(npx.eip, 0);
    }
    else
    {
      printf("exception %d (%#02x) occurred", i, i);
      if ((i == 8) || ((i>=10) && (i<=14)))
        printf(", error code=%#lx", tss_ptr->tss_error);
      mputchar('\n');
    }
  }
  for (n=0; n<3; n++)
    if ((dr6 & (1<<n)) && (dr7 & (3<<(n*2))))
      printf("breakpoint %d hit\n", n);
  if (use_ansi)
    printf("\033[0m");
  mono_attr = MONO_NORMAL;
}

#endif /* DEBUGGER */
