/**
 ** This is file GRDRIVER.H
 **
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
 **/

#ifndef _GRDRIVER_H_
#define _GRDRIVER_H_

#if defined(__GNUC__) && !defined(near)
# define near
# define far
# define huge
#endif

/* ================================================================== */
/*		      DRIVER HEADER STRUCTURES			      */
/* ================================================================== */

typedef struct {
    unsigned short  modeset_routine;
    unsigned short  paging_routine;
    unsigned short  driver_flags;
    unsigned short  def_tw;
    unsigned short  def_th;
    unsigned short  def_gw;
    unsigned short  def_gh;
} OLD_GR_DRIVER;

typedef struct {
    unsigned short  modeset_routine;
    unsigned short  paging_routine;
    unsigned short  driver_flags;
    unsigned short  def_tw;
    unsigned short  def_th;
    unsigned short  def_gw;
    unsigned short  def_gh;
    unsigned short  def_numcolor;
    unsigned short  driver_init_routine;
    unsigned short  text_table;
    unsigned short  graphics_table;
    /* The following entry is valid only if the GRD_MULTPAGES bit is set */
    unsigned short  pageflip_routine;
} NEW_GR_DRIVER;

typedef union {
    OLD_GR_DRIVER   old;
    NEW_GR_DRIVER   new;
} GR_DRIVER;

typedef struct {
    unsigned short  width;
    unsigned short  height;
    unsigned short  number_of_colors;
    unsigned char   BIOS_mode;
    unsigned char   special;
} GR_DRIVER_MODE_ENTRY;


/* ================================================================== */
/*			 DRIVER FLAG BITS			      */
/* ================================================================== */

#define GRD_NEW_DRIVER  0x0008		/* NEW FORMAT DRIVER IF THIS IS SET */

#define GRD_PAGING_MASK 0x0007		/* mask for paging modes */
#define GRD_NO_RW	0x0000		/* one standard 64 RW paging  */
#define GRD_RW_64K	0x0001		/* one 64K R + one 64K W page */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* THE FOLLOWING THREE OPTIONS ARE NOT SUPPORTED AT THIS TIME	      */
#define GRD_RW_32K	0x0002		/* two separate 32Kb RW pages */
#define GRD_MAP_128K	0x0003		/* 128Kb memory map -- some Tridents
					   can do it (1024x768x16 without
					   paging!!!) */
#define GRD_MAP_EXTMEM  0x0004		/* Can be mapped extended, above 1M.
					   Some Tseng 4000-s can do it, NO
					   PAGING AT ALL!!!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#define GRD_TYPE_MASK	0xf000		/* adapter type mask */
#define GRD_VGA		0x0000		/* vga */
#define GRD_EGA		0x1000		/* ega */
#define GRD_HERC	0x2000		/* hercules */
#define GRD_8514A	0x3000		/* IBM 8514A or compatible */
#define GRD_S3		0x4000		/* S3 graphics accelerator */

#define GRD_PLANE_MASK  0x0f00		/* bitplane number mask */
#define GRD_8_PLANES	0x0000		/* 8 planes = 256 colors */
#define GRD_4_PLANES	0x0100		/* 4 planes = 16 colors */
#define GRD_1_PLANE	0x0200		/* 1 plane  = 2 colors */
#define GRD_16_PLANES	0x0300		/* VGA with 32K colors */
#define GRD_8_X_PLANES  0x0400		/* VGA in mode X w/ 256 colors */

#define GRD_MEM_MASK	0x00f0		/* memory size mask */
#define GRD_64K		0x0010		/* 64K display memory */
#define GRD_128K	0x0020		/* 128K display memory */
#define GRD_256K	0x0030		/* 256K display memory */
#define GRD_512K	0x0040		/* 512K display memory */
#define GRD_1024K	0x0050		/* 1MB display memory */
#define GRD_192K	0x0060		/* 192K -- some 640x480 EGA-s */
#define GRD_MULTPAGES	0x0080		/* OR-d for multiple page support */
#define GRD_M_NOTSPEC	0x0000		/* memory amount not specified */


/* ================================================================== */
/*		     COMPILER-DEPENDENT CONSTANTS		      */
/* ================================================================== */

#ifdef __GNUC__
# define VGA_FRAME	0xd0000000	/* vga video RAM */
# define HERC_FRAME	0xe00b0000	/* Hercules frame addr */
# define EGA_FRAME	0xe00a0000	/* EGA frame addr */
# define RDONLY_OFF	0x00100000	/* VGA dual page: Read only */
# define WRONLY_OFF	0x00200000	/* VGA dual page: Write only */
#endif

#ifdef __TURBOC__
# define VGA_FRAME	0xa0000000UL	/* VGA video RAM */
# define EGA_FRAME	0xa0000000UL	/* EGA video RAM */
# define HERC_FRAME	0xb0000000UL	/* Hercules frame addr */
# define RDONLY_OFF	0		/* VGA dual page: Read only */
# define WRONLY_OFF	0		/* VGA dual page: Write only */
#endif


/* ================================================================== */
/*		      DRIVER INTERFACE FUNCTIONS		      */
/* ================================================================== */

extern void GrGetDriverModes
    (GR_DRIVER_MODE_ENTRY far **t,GR_DRIVER_MODE_ENTRY far **g);
extern int  _GrLowSetMode(int mode,int width,int height,int colors);
#ifdef __TURBOC__
extern void _GrSetVideoPage(int page);
extern void _GrSetAsmPage(void);		/* takes page(s) in AL, AH */
#endif

#endif  /* whole file */

