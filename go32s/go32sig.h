#ifndef _GO32SIG_H__
#define _GO32SIG_H__

// define DBG_READ to trace read invocation

#ifdef DBG_READ
extern short cRead;
#endif
extern short fInRead;
extern short fHadInterrupt;

extern int fpe_func();

#endif

