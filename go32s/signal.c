/* This is file signal.c
**
** Copyright (C) 1992 Rami EL CHARIF and William SCHELTER
** rcharif@ma.utexas.edu   wfs@cs.utexas.edu
**
** Signal package for djgpp versions 1.05, 1.06
** version 0.0 alpha  03/30/1992
**
** Send your comments or bugs report to
** rcharif@ma.utexas.edu or wfs@cs.utexas.edu
**
** This file is distributed WITHOUT ANY WARRANTY; without even the implied
** warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include <dos.h>
#include <signal.h>

unsigned long SignalTable[_NSIG + 1] = {
   (unsigned long)SIG_DFL,       /* SIGHUP	   */
   (unsigned long)SIG_DFL,       /* SIGINT	  +*/
   (unsigned long)SIG_DFL,       /* SIGQUIT	   */
   (unsigned long)SIG_DFL,       /* SIGILL	   */
   (unsigned long)SIG_DFL,       /* SIGABRT	   */
   (unsigned long)SIG_DFL,       /* SIGTRAP	   */
   (unsigned long)SIG_DFL,       /* SIGIOT	   */
   (unsigned long)SIG_DFL,       /* SIGEMT	   */
   (unsigned long)SIG_DFL,       /* SIGFPE	   */
   (unsigned long)SIG_DFL,       /* SIGKILL	   */
   (unsigned long)SIG_DFL,       /* SIGBUS	   */
   (unsigned long)SIG_DFL,       /* SIGSEGV	  +*/
   (unsigned long)SIG_DFL,       /* SIGSYS	   */
   (unsigned long)SIG_DFL,       /* SIGPIPE	   */
   (unsigned long)SIG_DFL,       /* SIGALRM	   */
   (unsigned long)SIG_DFL,       /* SIGTERM	   */
   (unsigned long)SIG_DFL,       /* SIGURG	   */
   (unsigned long)SIG_DFL,       /* SIGSTOP	   */
   (unsigned long)SIG_DFL,       /* SIGTSTP	   */
   (unsigned long)SIG_DFL,       /* SIGCONT	   */
   (unsigned long)SIG_DFL,       /* SIGCHLD	   */
   (unsigned long)SIG_DFL,       /* SIGCLD	   */
   (unsigned long)SIG_DFL,       /* SIGTTIN	   */
   (unsigned long)SIG_DFL,       /* SIGTTOU	   */
   (unsigned long)SIG_DFL,       /* SIGIO	   */
   (unsigned long)SIG_DFL,       /* SIGPOLL	   */
   (unsigned long)SIG_DFL,       /* SIGXCPU	   */
   (unsigned long)SIG_DFL,       /* SIGXFSZ	   */
   (unsigned long)SIG_DFL,       /* SIGVTALRM	*/
   (unsigned long)SIG_DFL,       /* SIGPROF	   */
   (unsigned long)SIG_DFL,       /* SIGWINCH	*/
   (unsigned long)SIG_DFL,       /* SIGUSR1	   */
   (unsigned long)SIG_DFL        /* SIGUSR2	   */
   };

SignalHandler signal(int sig, SignalHandler action)
{
 extern void SignalManager();
 union REGS in, out;
 SignalHandler hsigOld;

   in.h.ah = 1;
   in.h.al = sig;
   SignalTable[sig] = in.x.dx = (long)action;
   in.x.cx = (long)SignalManager;
   int86(0xfa, &in, &out);
   hsigOld = (SignalHandler)out.x.dx;
   return hsigOld;
}

void SigInst()
{
 union REGS in, out;
 extern void SignalManager();

   in.h.ah = 0;
   in.h.al = 0;
   in.x.dx = (long)SignalManager;

#ifdef DEBUG_SIG
   printf("\nSignal Manager = %ld, %lx", in.x.dx, in.x.dx);
#endif

   int86(0xfa, &in, &out);

}

#ifndef NO_SIG_ALARM
unsigned int alarm(int culSeconds)
{

 union REGS in, out;

   if (!culSeconds) {
      in.h.ah = 3;   /* Reset alarm */
      int86(0xfa, &in, &out);
      }
   else {
      in.h.ah = 2;
      in.x.dx = culSeconds;
      int86(0xfa, &in, &out);
      }
   return in.x.cx;
}
#else
unsigned int alarm(int n)
{  return 0; }
#endif

