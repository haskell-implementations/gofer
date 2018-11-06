// define DBG_READ to trace read invocation

/***************************************************************************
*
*  Signal support.
*
*  Done :
*     - signal
*     - signal manager
*     - go32 interface int 0xfa
*     - SIGINT
*     - Move modifications to a separate file
*
*  To do:
*     - SIGINT modification (see below 1)
*     - SIGALRM
*     - SIGSEGV
*
***************************************************************************/

/* This is file Go32Sig.c */
/*
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*
** Signal handling by Rami EL CHARIF and Bill SCHELTER
** version 0.0 alpha  03/30/1992
** version 0.1 alpha  04/12/1992 Solved SIGINT read problem (temp solution)
** version 0.2 alpha  04/26/1992 Moved to a different file, port to go32 v1.06
** version 0.3 alpha  11/29/1992 SIGFPE support added by rjl@monu1.cc.monash.edu.au
**                               moved to go32 1.09
**
** Send your comments or bugs report to
** rcharif@math.utexas.edu or wfs@cs.utexas.edu
*/

// Comments :
// 1 - A problem arises when Ctrl-c is issued while we are in certain
//    dos function calls. Some functions are not allowed to be interrupted
//    and must be checked with int 0x21 function 0x33.
//    Another problem comes from the implementation of go32.
//    Only one area of 4k is allocated and used to communicate between
//    user program and go32.
//    Example : function 0x3f of int 0x21 - read from file
//       If user interrupt is issued from inside the read, dos will break
//       and call user SIGINT handler. If user issues a read from the SIGINT
//       handler, go32 will trash the original input.
//       I implemented a temporary solution for this function but it is
//       specific to the application I am using, akcl.
//    What needs to be done is allocating different memory area for each
//    function call so that no collision will arise. A new low memory alloc
//    function needs to be implemented.
//    Some library files need to be modified to implement this in an efficient
//    way instead of managing that from go32 (less modifications to be done)
//

#include <stdio.h>
#include <dos.h>

#include "types.h"
#include "tss.h"
#include "Go32Sig.h"

extern int ctrl_c_flag;

#ifdef DBG_READ
short cRead = 0;  // Level of nested read
#endif
short fInRead = 0;   // Flag, set if in function 0x3f of int 0x21
                     // (read from file).
                     // Value : == 1 if file read
                     //         == 2 if stdin read
short fHadInterrupt = 0;   // Set if had interrupt while in read

// SIGSEGV support will be tricky to implement, as there many places in the
// code where a verification is done. Extensive modifications need to be done.
// Not yet available
//
// int SigSegV(unsigned long pv, unsigned long at);
//	#define SEGFAULT(p) SigSegV(((unsigned long)p), tss_ptr->tss_eip);
//	/*****************************************************
//	*	#define SEGFAULT(p) { \
//	*	  printf("Segmentation violation in pointer 0x%08lx at %x:%lx\n", (p), tss_ptr->tss_cs, tss_ptr->tss_eip); \
//	*	  return 1; \
//	*	  }
//	*****************************************************/

//111111111111111111111111111111111111111111111111111111111111111111111111111
// UNIX Signals code (taken from sys/signum.h, plugged inline to avoid
// conflict with BORLANDs signal code if signal.h was included
#define SIG_DFL ((void (*)(int))(-1))
#define SIG_IGN ((void (*)(int))(-2))
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
#define _SIGNAL_H
#define	_NSIG		32	/* Biggest signal number + 1.  */

// Different than in sys/signal.h
typedef unsigned long fSignalHandler;

// Redundant, might be removed in final version and replaced by a bit
// array for  implemented / not impl.
// Calls are done from SignalManager function running in protected mode.
//
static fSignalHandler ahsigUser[_NSIG + 1] = {
   (fSignalHandler)SIG_DFL,       /* SIGHUP	   */
   (fSignalHandler)SIG_DFL,       /* SIGINT	  +*/
   (fSignalHandler)SIG_DFL,       /* SIGQUIT	   */
   (fSignalHandler)SIG_DFL,       /* SIGILL	   */
   (fSignalHandler)SIG_DFL,       /* SIGABRT	   */
   (fSignalHandler)SIG_DFL,       /* SIGTRAP	   */
   (fSignalHandler)SIG_DFL,       /* SIGIOT	   */
   (fSignalHandler)SIG_DFL,       /* SIGEMT	   */
   (fSignalHandler)SIG_DFL,       /* SIGFPE	   */
   (fSignalHandler)SIG_DFL,       /* SIGKILL	   */
   (fSignalHandler)SIG_DFL,       /* SIGBUS	   */
   (fSignalHandler)SIG_DFL,       /* SIGSEGV	   */
   (fSignalHandler)SIG_DFL,       /* SIGSYS	   */
   (fSignalHandler)SIG_DFL,       /* SIGPIPE	   */
   (fSignalHandler)SIG_DFL,       /* SIGALRM	  -*/
   (fSignalHandler)SIG_DFL,       /* SIGTERM	   */
   (fSignalHandler)SIG_DFL,       /* SIGURG	   */
   (fSignalHandler)SIG_DFL,       /* SIGSTOP	   */
   (fSignalHandler)SIG_DFL,       /* SIGTSTP	   */
   (fSignalHandler)SIG_DFL,       /* SIGCONT	   */
   (fSignalHandler)SIG_DFL,       /* SIGCHLD	   */
   (fSignalHandler)SIG_DFL,       /* SIGCLD	   */
   (fSignalHandler)SIG_DFL,       /* SIGTTIN	   */
   (fSignalHandler)SIG_DFL,       /* SIGTTOU	   */
   (fSignalHandler)SIG_DFL,       /* SIGIO	   */
   (fSignalHandler)SIG_DFL,       /* SIGPOLL	   */
   (fSignalHandler)SIG_DFL,       /* SIGXCPU	   */
   (fSignalHandler)SIG_DFL,       /* SIGXFSZ	   */
   (fSignalHandler)SIG_DFL,       /* SIGVTALRM	*/
   (fSignalHandler)SIG_DFL,       /* SIGPROF	   */
   (fSignalHandler)SIG_DFL,       /* SIGWINCH	*/
   (fSignalHandler)SIG_DFL,       /* SIGUSR1	   */
   (fSignalHandler)SIG_DFL        /* SIGUSR2	   */
   };
   /*********************************
   *   - ==> Alpha
   *   + ==> Defined
   *   ? ==> Coming soone (perhaps ?)
   **********************************/

// For printf when debugging
#ifdef DEBUG_SIG
static char *aszSig[_NSIG + 1] = {
   "SIGHUP"			,
   "SIGINT"			,
   "SIGQUIT"		,
   "SIGILL"			,
   "SIGABRT"		,
   "SIGTRAP"		,
   "SIGIOT"			,
   "SIGEMT"			,
   "SIGFPE"			,
   "SIGKILL"		,
   "SIGBUS"			,
   "SIGSEGV"		,
   "SIGSYS"			,
   "SIGPIPE"		,
   "SIGALRM"		,
   "SIGTERM"		,
   "SIGURG"			,
   "SIGSTOP"		,
   "SIGTSTP"		,
   "SIGCONT"		,
   "SIGCHLD"		,
   "SIGCLD"			,
   "SIGTTIN"		,
   "SIGTTOU"		,
   "SIGIO"			,
   "SIGPOLL"		,
   "SIGXCPU"		,
   "SIGXFSZ"		,
   "SIGVTALRM"		,
   "SIGPROF"		,
   "SIGWINCH"		,
   "SIGUSR1"		,
   "SIGUSR2"
   };
#endif

static unsigned long ulSignalManager = 0;
static unsigned long culAlarm = 0;     // Time remaining in 18.2 ms
static unsigned long fResetAlarm = 0;  // Reset alarm ?

/******************************************************************************
* i_Interface()   Interface between user program and go32
*
*  Defined as interrupt 0xfa handler.
*  On entry : ah = 0xfa
*
* Functions (ah) :
*
*  0 : Give the Signal Manager address, called before main.
*     Input :
*        edx = Signal Manager
*
*  1 : Set Signal handler
*     Input :
*        al = 0 --> _NSIG : Signal to modify (see signum.h).
*        edx = new Signal Handler.
*     Output :
*        edx = Old Signal Handler.
*
*  2, 3 : SIGALRM (not ready yet)
*
******************************************************************************/
int i_Interface()
{
 long eax;
 int ah, al;

   eax = tss_ptr->tss_eax;

   ah = (eax & 0xff00) >> 8;  // Get function code
   al = eax & 0xff;  // Get Sub Function code   */

#ifdef DEBUG_SIG
   printf("\n In i_Interface, eax = %lx, ah = %x, al = %x", eax, ah, al);
#endif

   switch (ah) {

      case 0:
         ulSignalManager = tss_ptr->tss_edx;
         break;

      case 1: { // Signal handling

         long edx;
         fSignalHandler hsigOld;

         ulSignalManager = tss_ptr->tss_ecx;

#ifdef DEBUG_SIG
         printf("\nSetting Signal %s, edx = %lx", aszSig[al], tss_ptr->tss_edx);
         printf("\nOld handler = %lx,\tNew = %lx"
               , (fSignalHandler)ahsigUser[al], tss_ptr->tss_ecx
            );
#endif
         switch (al) {

            case SIGINT :
            case SIGALRM:
            case SIGFPE	   :
               edx = tss_ptr->tss_edx;
               hsigOld = (fSignalHandler)ahsigUser[al];
               ahsigUser[al] = (fSignalHandler)edx;
               edx = (fSignalHandler)hsigOld;
#ifdef DEBUG_SIGFPE
               if (al == SIGFPE)
                  printf("\nInstalling sigfpe handler");
#endif
               break;
            case SIGHUP	   :
            case SIGQUIT	: 
            case SIGILL	   :
            // case SIGABRT	: SIGIOT
            case SIGTRAP	: 
            case SIGIOT	   :
            case SIGEMT	   :
            case SIGKILL	: 
            case SIGBUS	   :
            case SIGSEGV	: 
            case SIGSYS	   :
            case SIGPIPE	: 
            case SIGTERM	: 
            case SIGURG	   :
            case SIGSTOP	: 
            case SIGTSTP	: 
            case SIGCONT	: 
            case SIGCHLD	: 
            // case SIGCLD	   : SIGCHLD
            case SIGTTIN	: 
            case SIGTTOU	: 
            case SIGIO	   :
            // case SIGPOLL	: SIGIO
            case SIGXCPU	: 
            case SIGXFSZ	: 
            case SIGVTALRM :
            case SIGPROF	: 
            case SIGWINCH	:
            case SIGUSR1	: 
            case SIGUSR2	:
//               printf("\nSignal %d undefined yet");
               break;
            }
         }
         break;

#ifndef NO_SIG_ALARM
      case 2: {   // Alarm was called
         unsigned long culRemain = culAlarm;
#ifdef  DEBUG_SIGALRM
         printf("\nSetting alarm to %lu", culAlarm);
         fflush(stdout);
#endif
         culAlarm = tss_ptr->tss_edx * 18.2; // in pc's granularity is 18.2 ms
         tss_ptr->tss_ecx = culRemain; // return value by alarm
         }
         break;

      case 3: { // Reset alarm
         unsigned long culRemain = culAlarm;
         int ci;
#ifdef  DEBUG_SIGALRM
         printf("\nSetting alarm to %lu", culAlarm);
         fflush(stdout);
#endif
         fResetAlarm = 1; // Reset alarm
         sleep(1);
         tss_ptr->tss_ecx = culRemain; // return value by alarm

         }
         break;
#endif
      default : // Unknown, abort
         return 1;
      }

 return 0;
}

//***************************************************************************
// ctrlbrk_func
// User keyboard interrupt management
//
// idea:
//    when called and there is a user SIGINT function,
//    push the current user program instruction location (a_tss.tss_eip)
//    on user stack, then let a_tss.tss_eip = user SIGINT handler function.
//    On return, go32 will perform a jmp to this handler function.
//    When finished, this function will issue a ret instruction loading
//    eip with the old location.
//    The keyboard buffer is flushed before calling the user routine.
//
//***************************************************************************
word32 push32(void *ptr, int len);
int ctrlbrk_func()
{
 unsigned long a_tss_eip;

   if ((ulSignalManager != 0) &&
       (ahsigUser[SIGINT] != (fSignalHandler)SIG_DFL)
      ) {
      long lSigCur = SIGINT;

#ifdef DEBUG_SIG
      printf("\n\tInvoking user SIGINT Handler");
      printf("\neip = %lx, User handler = %lx\n"
            , a_tss.tss_eip, ahsigUser[SIGINT]);
#endif


      if (fInRead == 0) {  // If we are not in read fn 0x3f of int 0x21
         push32(&(a_tss.tss_eip), 4);
         push32(&lSigCur, 4);
         a_tss.tss_eip = ulSignalManager;
         }
      else { // In read
         if (fInRead == 2) {  // Are we reading from stdin
#ifdef DBG_READ
         printf("\n\t\tUngetting 2 char to stdin");
#endif
            // ************** See case 0x3f in function i_21 ***********
            //
            // If reading from stdin, then on interrupt DOS restarts the
            // read operation from the beginning of the line
            // so to abort the read, a character followed by a CR must
            // be typed.
            // Functions 0x05 of interrupt 0x16, direct keyboard write
            // will simulate the typing of the character 255 which is
            // invisible followed by a CR
            // ------------------- DRAWBACK ------------------------
            // On old bios < 1986 this wasn't implemented, the 386 neither
            // so unless using an old BIOS, there would be no problems.
            {
            union REGS in, out;

               in.h.ah = 5;      // Keyboard Write

               // Relevant to akcl
               // or space char No doesn't work
               in.h.ch = 0x039;  // scan code for space
               in.h.cl = 0x20;   // space
               int86(0x16, &in, &out);

//	               in.h.ch = 0xa;   // scan code
//	               in.h.cl = '(';   //
//	               int86(0x16, &in, &out);
//	               in.h.ch = 0xb;   // scan code
//	               in.h.cl = ')';   //
//	               int86(0x16, &in, &out);

//	               in.h.ch = 0x1c;  // scan code
//	               in.h.cl = 0x0d;  //
//	               int86(0x16, &in, &out);
//	               in.h.ch = 0x0;   // scan code
//	               in.h.cl = 0x0a;  //
//	               int86(0x16, &in, &out);

               in.h.ch = 0x1c;   // Scan code for return
               in.h.cl = 0x0d;   // '\n'
               int86(0x16, &in, &out);
            }
            }
#ifdef DBG_READ
         printf("\n\t\tHad Interrupt while in read\n");
#endif
         fHadInterrupt = 1;   // Set interrupt flag
         }
      return 1;
      }
   else // Default signal
#if DEBUGGER
      ctrl_c_flag = 1;
#else
     { printf("Ctrl-C Hit...exitting\n"); exit(3);}
#endif
}

//----------------------------------------------------------------------------

#ifndef NO_SIG_ALARM
void interrupt (*oldhandler0x1C)(void);

void interrupt handler0x1C(void)
{
   if (culAlarm) {
      if (fResetAlarm) {
         culAlarm = 0;
         fResetAlarm = 0;
         }
      else {
         culAlarm--;

         if (!culAlarm) {
            if (ahsigUser[SIGINT] != (fSignalHandler)SIG_DFL) {
               long lSigCur = SIGALRM;

#ifdef DEBUG_SIGALRM
               printf("\n\tCalling SIGALRM\n");
               fflush(stdout);
#endif
               push32(&(a_tss.tss_eip), 4);
               push32(&lSigCur, 4);
               a_tss.tss_eip = ulSignalManager;
               }
            else
               printf("\n\tHmm... Calling alarm without SIGALRM handler");
            }
         }
      }

   /* call the old routine */
   oldhandler0x1C();
}

void SetInterrupt0x1C(void)
{
   /* save the old interrupt vector */
   oldhandler0x1C = getvect(0x1c);

   /* install the new interrupt handler */
   setvect(0x1c, handler0x1C);

#ifdef DEBUG_SIGALRM
   printf("\nSIGALRM installed");
   fflush(stdout);
#endif
}

void ReSetInterrupt0x1C(void)
{
#ifdef DEBUG_SIGALRM
   printf("\nRemoving SIGALRM, press any key ");
   getche();
   fflush(stdout);
#endif
   /* reset the old interrupt handler */
   setvect(0x1c, oldhandler0x1C);

#ifdef DEBUG_SIGALRM
   printf("\tSIGALRM removed");
   fflush(stdout);
#endif
}
#endif

/******************************************************************************
// SigSegv : SIGSEGV management
// ******************* not finished yet *********************
// **************** experimental stage **********************
******************************************************************************/
int SigSegV(unsigned long pv, unsigned long at)
{

   if (ahsigUser[SIGSEGV] != (fSignalHandler)SIG_DFL) {
#ifdef DEBUG_SIG
      printf("\n\tInvoking user SIGSEGV Handler\n");
#endif
      if (at != NULL)
	      printf("Segmentation violation in pointer 0x%08lx at %x:%lx\n", pv, tss_ptr->tss_cs, at);
      else 
         printf("Segmentation violation in pointer 0x%08lx\n", pv);

/*-------------------------------------------------------------------------*/
      push32(&(a_tss.tss_eip), 4);
      a_tss.tss_eip = (fSignalHandler)ahsigUser[SIGSEGV];
      return 1;
/*-------------------------------------------------------------------------*/
      }      
   else {
      if (at != NULL)
	      printf("Segmentation violation in pointer 0x%08lx at %x:%lx\n", pv, tss_ptr->tss_cs, at);
      else 
         printf("Segmentation violation in pointer 0x%08lx\n", pv);
      return 1;
      }
}
/****************************************************************************/
// SIGFPE support routine. Done by rjl@monu1.cc.monash.edu.au (R Lang)
//
//***************************************************************************
// fpe_func        UNTESTED   UNTESTED   UNTESTED   UNTESTED   UNTESTED  
// Floating point exception management
//
// idea:
//    when called and there is a user SIGFPE function,
//    push the current user program instruction location (a_tss.tss_eip)
//    on user stack, then let a_tss.tss_eip = user SIGFPE handler function.
//    On return, go32 will perform a jmp to this handler function.
//    When finished, handler function will issue a ret instruction loading
//    eip with the old location.
//
//***************************************************************************
int fpe_func()
{
 unsigned long a_tss_eip;

#ifdef DEBUG_SIGFPE
   printf("\nIn fpe_func");
#endif
   if ((ulSignalManager != 0) &&
       (ahsigUser[SIGFPE] != (fSignalHandler)SIG_DFL)
      ) {
      long lSigCur = SIGFPE;

#if defined(DEBUG_SIG) || defined(DEBUG_SIGFPE)
      printf("\n\tInvoking user SIGFPE Handler");
      printf("\neip = %lx, User handler = %lx\n"
            , a_tss.tss_eip, ahsigUser[SIGFPE]);
#endif

      push32(&(a_tss.tss_eip), 4);
      push32(&lSigCur, 4);
      a_tss.tss_eip = ulSignalManager;

      /* should we reset the FPU here? */

      return 0;	 /* continue on */
      }
   else
      return 1;  /* no handler, so abort */
}

