/* This is file SYMS.C */
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

/* History:250,1 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include "build.h"
#include "types.h"
#include "syms.h"
#include "tss.h"
#include "stab.h"
#include "aout.h"
#include "utils.h"

#ifdef SUPPORT_UDI
#undef DEBUGGER
#define DEBUGGER 0
void syms_init(char *fname){}
int undefined_symbol=1;
word32 syms_name2val(char *name){ return strtol(name, 0, 16); }
char *syms_val2name(word32 val, word32 *delta)
{
  static char b[20];
  sprintf(b, "%#lx", val);
  *delta = 0;
  return b;
}
syms_listwild(){}
char *syms_val2line(word32 v, int *lineret, int exact){ return 0; }
#endif

#define Ofs(n) ((int)&(((TSS *)0)->n))

struct {
  char *name;
  int size;
  int ofs;
  } regs[] = {
  "%eip", 4, Ofs(tss_eip),
  "%eflags", 4, Ofs(tss_eflags),
  "%eax", 4, Ofs(tss_eax),
  "%ebx", 4, Ofs(tss_ebx),
  "%ecx", 4, Ofs(tss_ecx),
  "%edx", 4, Ofs(tss_edx),
  "%esp", 4, Ofs(tss_esp),
  "%ebp", 4, Ofs(tss_ebp),
  "%esi", 4, Ofs(tss_esi),
  "%edi", 4, Ofs(tss_edi),
  "%ax", 2, Ofs(tss_eax),
  "%bx", 2, Ofs(tss_ebx),
  "%cx", 2, Ofs(tss_ecx),
  "%dx", 2, Ofs(tss_edx),
  "%ah", 1, Ofs(tss_eax)+1,
  "%bh", 1, Ofs(tss_ebx)+1,
  "%ch", 1, Ofs(tss_ecx)+1,
  "%dh", 1, Ofs(tss_edx)+1,
  "%al", 1, Ofs(tss_eax),
  "%bl", 1, Ofs(tss_ebx),
  "%cl", 1, Ofs(tss_ecx),
  "%dl", 1, Ofs(tss_edx),
  0, 0, 0
};

#if DEBUGGER

#define SYMADDR 0xa0000000

static word32 sym_brk=SYMADDR;

word32 salloc(word32 size)
{
  word32 r = sym_brk;
  size = (size+3)&~3;
  sym_brk +=size;
  return r;
}

symsput(word32 where, void *ptr, int size)
{
  memput(where, ptr, size);
}

symsget(word32 where, void *ptr, int size)
{
  memget(where, ptr, size);
}

typedef struct SYMTREE {
  word32 me;
  word32 nleft, nright, ndown;
  word32 vleft, vright, vdown;
  word32 val;
  word32 type;
  word32 name_len;
} SYMTREE;

typedef struct FILENODE {
  word32 me;
  word32 next;
  word32 line_list;
  word32 first_addr, last_addr;
  word32 name_len;
} FILENODE;

typedef struct LINENODE {
  word32 me;
  word32 next;
  word32 num;
  word32 addr;
} LINENODE;

static word32 symtree_root=0;
static word32 file_list=0;
static word32 nsyms=0;

static char tmps[256];
static char tmps2[256];

static word32 symtree_add(word32 val, word32 type, char *name)
{
  SYMTREE s, sv, temp;
  int cval;
  int32 cval32;

  memset(&temp, 0, sizeof(temp));
  temp.name_len = strlen(name) + 1;
  temp.me = salloc(sizeof(temp)+temp.name_len);
  temp.val = val;
  temp.type = type;
  symsput(temp.me, &temp, sizeof(temp));
  symsput(temp.me+sizeof(temp), name, temp.name_len);

  if (symtree_root == 0)
  {
    symtree_root = temp.me;
    return temp.me;
  }
  s.me = symtree_root;
  while (1)
  {
    symsget(s.me, &s, sizeof(s));
    symsget(s.me+sizeof(s), tmps, s.name_len);
    cval = strcmp(name, tmps);
    if ((cval == 0) &&
        ( (s.val == 0)
       || (val == 0)
       || (s.val == val)))
    {
      if (val)
      {
        s.val = val;
        s.type = type;
        symsput(s.me, &s, sizeof(s));
      }
      return temp.me;
    }
    else if (cval == 0)
    {
      if (s.ndown == 0)
      {
        s.ndown = temp.me;
        symsput(s.me, &s, sizeof(s));
        break;
      }
      s.me = s.ndown;
    }
    else if (cval < 0)
    {
      if (s.nleft == 0)
      {
        s.nleft = temp.me;
        symsput(s.me, &s, sizeof(s));
        break;
      }
      s.me = s.nleft;
    }
    else
    {
      if (s.nright == 0)
      {
        s.nright = temp.me;
        symsput(s.me, &s, sizeof(s));
        break;
      }
      s.me = s.nright;
    }
  }
  nsyms++;

  sv.me = symtree_root;
  while (1)
  {
    symsget(sv.me, &sv, sizeof(sv));
    if (sv.me == temp.me)
      return temp.me;
    cval32 = val - sv.val;
    if (cval32 == 0)
    {
      if (sv.vdown == 0)
      {
        sv.vdown = temp.me;
        symsput(sv.me, &sv, sizeof(sv));
        break;
      }
      sv.me = sv.vdown;
    }
    else if (cval32 < 0)
    {
      if (sv.vleft == 0)
      {
        sv.vleft = temp.me;
        symsput(sv.me, &sv, sizeof(sv));
        break;
      }
      sv.me = sv.vleft;
    }
    else
    {
      if (sv.vright == 0)
      {
        sv.vright = temp.me;
        symsput(sv.me, &sv, sizeof(sv));
        break;
      }
      sv.me = sv.vright;
    }
  }
  return temp.me;
}

static symtree_print(word32 s, int byval, int level)
{
  SYMTREE tmp;
  if (s == 0)
    return;
  symsget(s, &tmp, sizeof(tmp));
  if (byval)
    symtree_print(tmp.vleft, byval, level+1);
  else
    symtree_print(tmp.nleft, byval, level+1);

  symsget(tmp.me+sizeof(tmp), tmps, tmp.name_len);
  printf("0x%08lx 0x%08lx %*s%s\n", tmp.val, tmp.type, level, "", tmps);

  if (byval)
    symtree_print(tmp.vdown, byval, level);
  else
    symtree_print(tmp.ndown, byval, level);

  if (byval)
    symtree_print(tmp.vright, byval, level+1);
  else
    symtree_print(tmp.nright, byval, level+1);
}

void syms_list(v)
{
  symtree_print(symtree_root, v, 0);
}

typedef struct SYM_ENTRY {
  word32 string_off;
  word8 type;
  word8 other;
  word16 desc;
  word32 val;
} SYM_ENTRY;

get_string(FILE *f, word32 where)
{
  char *cp;
  if (where != ftell(f))
    fseek(f, where, 0);
  cp = tmps2;
  do {
    *cp++ = fgetc(f);
  } while (cp[-1]);
}

void syms_init(char *fname)
{
  GNU_AOUT header;
  int fd, j, per, oldper=-1;
  word32 nsyms, i;
  FILE *sfd;
  word32 sfd_base;
  SYM_ENTRY sym;
  char *strings, *cp;
  word32 string_size;
  word32 last_dot_o=0;
  FILENODE filen;
  LINENODE linen;

  filen.me = 0;
  linen.me = 0;

  fd = open(fname, O_RDONLY | O_BINARY);
  sfd = fopen(fname, "rb");

  read(fd, &header, sizeof(header));
  if ((header.info & 0xffff) == 0x14c)
  {
    lseek(fd, 0xa8L, 0);
    read(fd, &header, sizeof(header));
    lseek(fd, 0xa8+sizeof(header) + header.tsize + header.dsize, 0);
  }
  else if (((header.info & 0xffff) == 0x10b) ||
     ((header.info & 0xffff) == 0x0107))
  {
    lseek(fd, sizeof(header) + header.tsize + header.dsize
      + header.txrel + header.dtrel, 0);
  }
  else
  {
    nsyms = 0;
    return;
  }

  printf("Reading symbols...   0%%");
  fflush(stdout);

  nsyms = header.symsize / sizeof(SYM_ENTRY);

  fseek(sfd, tell(fd)+header.symsize, 0);
  sfd_base = ftell(sfd);

  for (i=0; i<nsyms; i++)
  {
    if (nsyms > 1)
      per = i*100L/(nsyms-1);
    else
      per = 100;
    if (per != oldper)
    {
      printf("\b\b\b\b%3d%%", per);
      fflush(stdout);
      oldper = per;
    }
    read(fd, &sym, sizeof(sym));
    if (sym.string_off)
      get_string(sfd, sfd_base+sym.string_off);
    switch (sym.type)
    {
      case N_TEXT:
      case N_TEXT | N_EXT:
        cp = tmps2;
        cp += strlen(cp) - 2;
        if (strcmp(cp, ".o") == 0)
        {
          last_dot_o = sym.val;
          if (filen.me && (filen.last_addr == 0))
          {
            filen.last_addr = last_dot_o - 1;
            symsput(filen.me, &filen, sizeof(filen));
          }
          break;
        }
        if (strcmp(cp, "d.") == 0) /* as in gcc_compiled. */
          break;
      case N_DATA:
      case N_DATA | N_EXT:
      case N_ABS:
      case N_ABS | N_EXT:
      case N_BSS:
      case N_BSS | N_EXT:
      case N_FN:
      case N_SETV:
      case N_SETV | N_EXT:
      case N_SETA:
      case N_SETA | N_EXT:
      case N_SETT:
      case N_SETT | N_EXT:
      case N_SETD:
      case N_SETD | N_EXT:
      case N_SETB:
      case N_SETB | N_EXT:
      case N_INDR:
      case N_INDR | N_EXT:
        if (sym.string_off)
          symtree_add(sym.val, sym.type, tmps2);
        break;
      case N_SO:
        memset(&filen, 0, sizeof(FILENODE));
        filen.name_len = strlen(tmps2)+1;
        filen.me = salloc(sizeof(FILENODE)+filen.name_len);
        symsput(filen.me+sizeof(filen), tmps2, filen.name_len);
        filen.next = file_list;
        file_list = filen.me;
        filen.first_addr = last_dot_o;
        symsput(filen.me, &filen, sizeof(filen));
        break;
      case N_SLINE:
        memset(&linen, 0, sizeof(linen));
        linen.me = salloc(sizeof(LINENODE));
        linen.next = filen.line_list;
        filen.line_list = linen.me;
        linen.num = sym.desc;
        linen.addr = sym.val;
        symsput(linen.me, &linen, sizeof(linen));
        symsput(filen.me, &filen, sizeof(filen));
        break;
    }
  }
  printf(" %ld symbol%s read\n", nsyms, nsyms==1 ? "" : "s");
  return;
}

int undefined_symbol=0;

word32 syms_name2val(char *name)
{
  SYMTREE s;
  int cval, idx, sign=1, i;
  word32 v;
  char *cp;

  undefined_symbol = 0;

  idx = 0;
  sscanf(name, "%s", name);

  if (name[0] == 0)
    return 0;

  if (name[0] == '-')
  {
    sign = -1;
    name++;
  }
  else if (name[0] == '+')
  {
    name++;
  }
  if (isdigit(name[0]))
  {
    if (sign == -1)
      return -strtol(name, 0, 0);
    return strtol(name, 0, 0);
  }

  cp = strpbrk(name, "+-");
  if (cp)
    idx = cp-name;
  else
    idx = strlen(name);

  if (name[0] == '%') /* register */
  {
    for (i=0; regs[i].name; i++)
      if (strncmp(name, regs[i].name, idx) == 0)
      {
        switch (regs[i].size)
        {
          case 1:
            v = *(word8 *)((word8 *)tss_ptr + regs[i].ofs);
            break;
          case 2:
            v = *(word16 *)((word8 *)tss_ptr + regs[i].ofs);
            break;
          case 4:
            v = *(word32 *)((word8 *)tss_ptr + regs[i].ofs);
            break;
        }
        return v + syms_name2val(name+idx);
      }
  }

  for (i=0; i<idx; i++)
    if (name[i] == '#')
    {
      FILENODE f;
      LINENODE l;
      int lnum;
      sscanf(name+i+1, "%d", &lnum);
      for (f.me=file_list; f.me; f.me=f.next)
      {
        symsget(f.me, &f, sizeof(f));
        symsget(f.me+sizeof(f), tmps, f.name_len);
        if ((strncmp(name, tmps, i) == 0) && (tmps[i] == 0))
        {
          for (l.me=f.line_list; l.me; l.me=l.next)
          {
            symsget(l.me, &l, sizeof(l));
            if (l.num == lnum)
              return l.addr + syms_name2val(name+idx);
          }
          printf("undefined line number %.*s\n", idx, name);
          undefined_symbol = 1;
          return 0;
        }
      }
      printf("Undefined file name %.*s\n", i, name);
      undefined_symbol = 1;
      return 0;
    }

  s.me = symtree_root;
  while (s.me)
  {
    symsget(s.me, &s, sizeof(s));
    symsget(s.me+sizeof(s), tmps, s.name_len);
    cval = strncmp(name, tmps, idx);
    if ((cval == 0) && tmps[idx])
      cval = -1;
    if (cval == 0)
      return s.val*sign + syms_name2val(name+idx);
    else if (cval < 0)
      s.me = s.nleft;
    else
      s.me = s.nright;
  }
  s.me = symtree_root;
  while (s.me)
  {
    symsget(s.me, &s, sizeof(s));
    symsget(s.me+sizeof(s), tmps, s.name_len);
    if (tmps[0] == '_')
      cval = strncmp(name, tmps+1, idx);
    else
      cval = '_' - tmps[0];
    if ((cval == 0) && tmps[idx+1])
      cval = -1;
    if (cval == 0)
      return s.val*sign + syms_name2val(name+idx);
    else if (cval < 0)
      s.me = s.nleft;
    else
      s.me = s.nright;
  }
  printf("Undefined symbol %.*s\n", idx, name);
  undefined_symbol = 1;
  return 0;
}

static char noname_buf[11];

char *syms_val2name(word32 val, word32 *delta)
{
  SYMTREE s, lasts;

  if (delta)
    *delta = 0;
  lasts.me = 0;
  s.me = symtree_root;
  while (s.me)
  {
    symsget(s.me, &s, sizeof(s));
    if (s.val <= val)
      lasts.me = s.me;
    if (val == s.val)
    {
      while (s.vdown)
        symsget(s.vdown, &s, sizeof(s));
      symsget(s.me+sizeof(s), tmps2, s.name_len);
      return tmps2;
    }
    else if (val < s.val)
      s.me = s.vleft;
    else
      s.me = s.vright;
  }
  if (lasts.me)
  {
    symsget(lasts.me, &lasts, sizeof(lasts));
    while (lasts.vdown)
      symsget(lasts.vdown, &lasts, sizeof(lasts));
    symsget(lasts.me+sizeof(lasts), tmps2, lasts.name_len);
    if (strcmp(tmps2, "_etext") == 0)
      goto noname;
    if (strcmp(tmps2, "_end") == 0)
      goto noname;
    if (delta)
      *delta = val - lasts.val;
    return tmps2;
  }
noname:
  sprintf(noname_buf, "%#lx", val);
  return noname_buf;
}

char *syms_val2line(word32 val, int *lineret, int exact)
{
  FILENODE filen;
  LINENODE linen, closest;
  closest.me = 0;
  for (filen.me = file_list; filen.me; filen.me=filen.next)
  {
    symsget(filen.me, &filen, sizeof(filen));
    if ((val <= filen.last_addr) && (val >= filen.first_addr))
    {
      for (linen.me=filen.line_list; linen.me; linen.me = linen.next)
      {
        symsget(linen.me, &linen, sizeof(linen));
        if (val == linen.addr)
        {
          *lineret = linen.num;
          symsget(filen.me+sizeof(filen), tmps2, filen.name_len);
          return tmps2;
        }
        if (val > linen.addr)
        {
          if (!closest.me)
            closest.me = linen.me;
          else if (closest.addr < linen.addr)
            closest.me = linen.me;
        }
      }
      if (closest.me && !exact)
      {
        symsget(closest.me, &closest, sizeof(closest));
        *lineret = closest.num;
        symsget(filen.me+sizeof(filen), tmps2, filen.name_len);
        return tmps2;
      }
    }
  }
  return 0;
}

static char type2char(type)
{
  switch (type)
  {
    case N_TEXT:
      return 't';
    case N_TEXT | N_EXT:
      return 'T';
    case N_DATA:
      return 'd';
    case N_DATA | N_EXT:
      return 'D';
    case N_BSS:
      return 'b';
    case N_BSS | N_EXT:
      return 'B';
    default:
      return ' ';
  }
}

static int linecnt, quit_list;

static syms_listwild2(word32 s_pos, char *pattern)
{
  SYMTREE s;
  char *name;
  int lnum;
  s.me = s_pos;
  if ((s.me == 0) || quit_list)
    return;
  symsget(s.me, &s, sizeof(s));
  syms_listwild2(s.vleft, pattern);
  if (quit_list)
    return;
  symsget(s.me+sizeof(s), tmps, s.name_len);
  if (wild(pattern, tmps))
  {
    if (++linecnt > 20)
    {
      printf("--- More ---");
      switch (getch())
      {
        case ' ':
          linecnt = 0;
          break;
        case 13:
          linecnt--;
          break;
        case 'q':
          quit_list = 1;
          return;
      }
      printf("\r            \r");
    }
    printf("0x%08lx %c %s", s.val, type2char(s.type), tmps);
    name = syms_val2line(s.val, &lnum, 0);
    if (name)
      printf(", line %d of %s", lnum, name);
    mputchar('\n');
  }
  syms_listwild2(s.vdown, pattern);
  if (quit_list)
    return;
  syms_listwild2(s.vright, pattern);
}

void syms_listwild(char *pattern)
{
  quit_list = linecnt = 0;
  syms_listwild2(symtree_root, pattern);
}

#endif
