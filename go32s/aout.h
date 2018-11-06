/* This is file AOUT.H */
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

/* History:45,17 */
#ifndef _A_OUT_H_
#define	_A_OUT_H_

typedef struct filehdr {
	unsigned short f_magic;
	unsigned short f_nscns;
	long f_timdat;
	long f_symptr;
	long f_nsyms;
	unsigned short f_opthdr;
	unsigned short f_flags;
	} FILEHDR;

typedef struct aouthdr {
	int magic;
	int vstamp;
	long tsize;
	long dsize;
	long bsize;
	long entry;
	long text_start;
	long data_start;
	} AOUTHDR;

typedef struct scnhdr {
	char s_name[8];
	long s_paddr;
	long s_vaddr;
	long s_size;
	long s_scnptr;
	long s_relptr;
	long s_lnnoptr;
	unsigned short s_nreloc;
	unsigned short nlnno;
	long s_flags;
	} SCNHDR;


typedef struct gnu_aout {
	word32 info;
	word32 tsize;
	word32 dsize;
	word32 bsize;
	word32 symsize;
	word32 entry;
	word32 txrel;
	word32 dtrel;
	} GNU_AOUT;

#endif
