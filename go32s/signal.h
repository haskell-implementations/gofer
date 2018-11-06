/* This is file signal.h */
/* This file may have been modified by DJ Delorie (Jan 1991).  If so,
** these modifications are Coyright (C) 1991 DJ Delorie, 24 Kirsten Ave,
** Rochester NH, 03867-2954, USA.
*/

/* This may look like C code, but it is really -*- C++ -*- */
/* 
Copyright (C) 1989 Free Software Foundation
    written by Doug Lea (dl@rocky.oswego.edu)

This file is part of GNU CC.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU CC General Public
License for full details.

Everyone is granted permission to copy, modify and redistribute
GNU CC, but only under the conditions described in the
GNU CC General Public License.   A copy of this license is
supposed to have been given to you along with GNU CC so you
can know your rights and responsibilities.  It should be in a
file named COPYING.  Among other things, the copyright notice
and this notice must be preserved on all copies.  
*/

#ifndef _signal_h
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

/* This #define KERNEL hack gets around bad function prototypes on most */
/* systems. If not, you need to do some real work... */
/*******************
* #define KERNEL
* #include <sys/signal.h>
* #undef KERNEL
********************/

#ifndef _signal_h
#define _signal_h 1
#endif

/* The Interviews folks call this SignalHandler. Might as well conform. */
/* Beware: some systems think that SignalHandler returns int. */
typedef void (*SignalHandler) ();

extern SignalHandler signal(int sig, SignalHandler action);
extern SignalHandler sigset(int sig, SignalHandler action);
extern SignalHandler ssignal(int sig, SignalHandler action);
extern int           gsignal (int sig);
extern int           kill (int pid, int sig);

#ifndef hpux /* Interviews folks claim that hpux doesn't like these */
struct sigvec;
extern int           sigsetmask(int mask);
extern int           sigblock(int mask);
extern int           sigpause(int mask);
extern int           sigvec(int sig, struct sigvec* v, struct sigvec* prev);
#endif

/* The Interviews version also has these ... */

#define SignalBad ((SignalHandler)-1)
#define SignalDefault ((SignalHandler)0)
#define SignalIgnore ((SignalHandler)1)

#ifdef __cplusplus
}
#endif
#define _SIGNAL_H
/** #include <signum.h>	**/
#ifdef	_SIGNAL_H

/* This file defines the fake signal functions and signal
   number constants for 4.2 or 4.3 BSD-derived Unix system.  */

#define	SIG_DFL	0
#if 0
/*#ifndef SIG_DFL*/
/* Fake signal functions.
   These lines MUST be split!  m4 will not change them otherwise.  */
#define	SIG_ERR	/* Error return.  */	\
	((void EXFUN((*), (int sig))) -1)
#define	SIG_DFL	/* Default action.  */	\
	((void EXFUN((*), (int sig))) 0)
#define	SIG_IGN	/* Ignore signal.  */	\
	((void EXFUN((*), (int sig))) 1)

#endif
/* Signals.  */
#define	SIGHUP		1	/* Hangup (POSIX).  */
#define	SIGINT		2	/* Interrupt (ANSI).  */
#define	SIGQUIT		3	/* Quit (POSIX).  */
#define	SIGILL		4	/* Illegal instruction (ANSI).  */
#define	SIGABRT		SIGIOT	/* Abort (ANSI).  */
#define	SIGTRAP		5	/* Trace trap (POSIX).  */
#define	SIGIOT		6	/* IOT trap (4.2 BSD).  */
#define	SIGEMT		7	/* EMT trap (4.2 BSD).  */
#define	SIGFPE		8	/* Floating-point exception (ANSI).  */
#define	SIGKILL		9	/* Kill, unblockable (POSIX).  */
#define	SIGBUS		10	/* Bus error (4.2 BSD).  */
#define	SIGSEGV		11	/* Segmentation violation (ANSI).  */
#define	SIGSYS		12	/* Bad argument to system call (4.2 BSD)*/
#define	SIGPIPE		13	/* Broken pipe (POSIX).  */
#define	SIGALRM		14	/* Alarm clock (POSIX).  */
#define	SIGTERM		15	/* Termination (ANSI).  */
#define	SIGURG		16	/* Urgent condition on socket (4.2 BSD).*/
#define	SIGSTOP		17	/* Stop, unblockable (POSIX).  */
#define	SIGTSTP		18	/* Keyboard stop (POSIX).  */
#define	SIGCONT		19	/* Continue (POSIX).  */
#define	SIGCHLD		20	/* Child status has changed (POSIX).  */
#define	SIGCLD		SIGCHLD	/* Same as SIGCHLD (System V).  */
#define	SIGTTIN		21	/* Background read from tty (POSIX).  */
#define	SIGTTOU		22	/* Background write to tty (POSIX).  */
#define	SIGIO		23	/* I/O now possible (4.2 BSD).  */
#define	SIGPOLL		SIGIO	/* Same as SIGIO? (SVID).  */
#define	SIGXCPU		24	/* CPU limit exceeded (4.2 BSD).  */
#define	SIGXFSZ		25	/* File size limit exceeded (4.2 BSD).  */
#define	SIGVTALRM	26	/* Virtual alarm clock (4.2 BSD).  */
#define	SIGPROF		27	/* Profiling alarm clock (4.2 BSD).  */
#define	SIGWINCH	28	/* Window size change (4.3 BSD, Sun).  */
#define	SIGUSR1		30	/* User-defined signal 1 (POSIX).  */
#define	SIGUSR2		31	/* User-defined signal 2 (POSIX).  */

#endif	/* <signal.h> included.  */

#define	_NSIG		32	/* Biggest signal number + 1.  */

#endif

