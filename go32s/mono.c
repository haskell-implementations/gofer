#include <stdio.h>
#include <dos.h>
#include <stdarg.h>
#include "mono.h"

extern int use_mono;

int screen_seg = 0xb800;
int mono_attr = 0x07;

static int row=-1,col=-1;

mono_putc(char c)
{
  int i;
  switch (c)
  {
    case 13:
      col = 0;
      break;
    case 10:
      col = 0;
      row ++;
      if (row == 25)
      {
        for (i=160; i<24*160; i++)
          pokeb(0xb000, i, peekb(0xb000, i+160));
        for (; i<25*160; i+=2)
          poke(0xb000, i, 0x0720);
        row--;
      }
      break;
    case 9:
      do {
        col++;
      } while (col % 8);
      break;
    case 8:
      if (col > 0)
        col--;
      break;
    case 7:
      write(1, "\a", 1);
      break;
    case 12:
      for (i=0; i<25*160;)
      {
        pokeb(0xb000, i++, 0x20);
        pokeb(0xb000, i++, 0x07);
      }
      row = 1;
      col = 0;
      break;
    default:
      pokeb(0xb000, row*160+col*2, c);
      pokeb(0xb000, row*160+col*2+1, mono_attr);
      col++;
      if (col == 80)
      {
        mono_putc(10);
      }
      break;
  }
}

int printf(const char *fmt, ...)
{
  char buf[200];
  int i;
  int n = vsprintf(buf, fmt, (void _ss *) _va_ptr);
#if 0
  for (i=0; buf[i]; i++)
  {
    if (buf[i] == '\n')
    {
      _AL='\r';
      _AH=0;
      _DX=0;
      geninterrupt(0x17);
    }
    _AL=buf[i];
    _AH=0;
    _DX=0;
    geninterrupt(0x17);
  }
  return n;
#endif
  if (use_mono)
  {
    if (col == -1)
      mono_putc(12);

    for (i=0; buf[i]; i++)
      mono_putc(buf[i]);

    outportb(0x3b4, 15);
    outportb(0x3b5, row*80+col);
    outportb(0x3b4, 14);
    outportb(0x3b5, (row*80+col)/256);
  }
  else
    write(1, buf, strlen(buf));
  return n;
}

void mputchar(char c)
{
  if (use_mono)
  {
    mono_putc(c);
    outportb(0x3b4, 15);
    outportb(0x3b5, row*80+col);
    outportb(0x3b4, 14);
    outportb(0x3b5, (row*80+col)/256);
  }
  else
    write(1, &c, 1);
}

mono_write(char *buf, int len)
{
  int i;
  if (col == -1)
    mono_putc(12);

  for (i=0; i<len; i++)
    mono_putc(buf[i]);

  outportb(0x3b4, 15);
  outportb(0x3b5, row*80+col);
  outportb(0x3b4, 14);
  outportb(0x3b5, (row*80+col)/256);
  return len;
}
