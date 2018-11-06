/**
 ** EVENT.C
 **  Handlers and setup stuff for an interrupt-driven mouse and keyboard
 **  event queue mechanism for Turbo C and DJGPP (under MS DOS of course)
 **
 **  Copyright (C) 1992, Csaba Biegl
 **    [820 Stirrup Dr, Nashville, TN, 37221]
 **    [csaba@vuse.vanderbilt.edu]
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program; if not, write to the Free Software
 **  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#ifndef  __TURBOC__
#error	 Don't even try to compile it with this compiler
#endif

#define FOR_GO32

#pragma  inline;

#include <stdlib.h>
#include <string.h>
#include <alloc.h>
#include <stdio.h>
#include <time.h>
#include <dos.h>

#include "eventque.h"

extern int  far _ev_interss;	/* interrupt stack segment */
extern int  far _ev_interds;	/* interrupt data segment */
extern int  far _ev_kbintsp;	/* keyboard interrupt stack */
extern int  far _ev_msintsp;	/* mouse interrupt stack */
extern int  far _ev_kbinter;	/* keyboard interrupt flag */

extern void interrupt (* far _ev_oldkbint)(void);

extern void far	      _ev_mouseint(void);
extern void interrupt _ev_keybdint(void);

static void dummydraw(void) {}

static EventQueue *queue = NULL;
static void (*mousedraw)(void) = dummydraw;
static char *stack = NULL;
static char *qsave = NULL;

static int  ms_xpos;
static int  ms_ypos;
static int  ms_xmickey;
static int  ms_ymickey;
static int  first_call = 1;

#define MS_ENABLE   EVENT_ENABLE(EVENT_MOUSE)
#define KB_ENABLE   EVENT_ENABLE(EVENT_KEYBD)

#define KB_SSIZE    128		/* keyboard handler stack size */
#define MS_SSIZE    128		/* mouse handler MINIMAL stack size */

#define IABS(x)	    (((x) > 0) ? (x) : -(x))

void far _ev_mousehandler(int msk,int btn,int mx,int my)
{
	EventRecord *ep;
	int moved = 0;
	int diff;

	if((diff = mx - ms_xmickey) != 0) {
	    ms_xmickey += diff;
	    ms_xpos    += diff;
	    if((diff = ms_xpos / queue->evq_xspeed) != 0) {
		ms_xpos %= queue->evq_xspeed;
		if(IABS(diff) >= queue->evq_thresh) diff *= queue->evq_accel;
		diff += queue->evq_xpos;
		if(diff <= queue->evq_xmin) diff = queue->evq_xmin;
		if(diff >= queue->evq_xmax) diff = queue->evq_xmax;
		if(diff != queue->evq_xpos) {
		    queue->evq_xpos  = diff;
		    queue->evq_moved = moved = 1;
		}
	    }
	}
	if((diff = my - ms_ymickey) != 0) {
	    ms_ymickey += diff;
	    ms_ypos    += diff;
	    if((diff = ms_ypos / queue->evq_yspeed) != 0) {
		ms_ypos %= queue->evq_yspeed;
		if(IABS(diff) >= queue->evq_thresh) diff *= queue->evq_accel;
		diff += queue->evq_ypos;
		if(diff <= queue->evq_ymin) diff = queue->evq_ymin;
		if(diff >= queue->evq_ymax) diff = queue->evq_ymax;
		if(diff != queue->evq_ypos) {
		    queue->evq_ypos  = diff;
		    queue->evq_moved = moved = 1;
		}
	    }
	}
	if((msk & ~1) && (queue->evq_enable & MS_ENABLE)) {
	    disable();
	    ep = &queue->evq_events[queue->evq_wrptr];
	    if(++queue->evq_wrptr == queue->evq_maxsize)
		queue->evq_wrptr = 0;
	    if(queue->evq_cursize < queue->evq_maxsize)
		queue->evq_cursize++;
	    else if(++queue->evq_rdptr == queue->evq_maxsize)
		queue->evq_rdptr = 0;
	    enable();
	    _AX = 0x200;
	    geninterrupt(0x16);
	    ep->evt_kbstat = _AL;
	    ep->evt_type   = EVENT_MOUSE;
	    ep->evt_mask   = msk;
	    ep->evt_button = btn;
	    ep->evt_xpos   = queue->evq_xpos;
	    ep->evt_ypos   = queue->evq_ypos;
	    ep->evt_time   = clock();
	}
	if(moved && queue->evq_drawmouse) (*mousedraw)();
}

void far _ev_keybdhandler(void)
{
	EventRecord *ep;
	int keycode,scancode;

	if(queue->evq_enable & KB_ENABLE) for( ; ; ) {
	    _AX = 0x100;
	    geninterrupt(0x16);
	    asm jnz  charpresent;
	    return;
	  charpresent:
	    scancode = _AX;
	    keycode = (_AL != 0) ? _AL : _AH + 0x100;
	    if(queue->evq_delchar) {
		_AX = 0;
		geninterrupt(0x16);
	    }
	    disable();
	    ep = &queue->evq_events[queue->evq_wrptr];
	    if(++queue->evq_wrptr == queue->evq_maxsize)
		queue->evq_wrptr = 0;
	    if(queue->evq_cursize < queue->evq_maxsize)
		queue->evq_cursize++;
	    else if(++queue->evq_rdptr == queue->evq_maxsize)
		queue->evq_rdptr = 0;
	    enable();
	    _AX = 0x200;
	    geninterrupt(0x16);
	    ep->evt_kbstat  = _AL;
	    ep->evt_keycode = keycode;
	    ep->evt_scancode = scancode;
	    ep->evt_type    = EVENT_KEYBD;
	    ep->evt_time    = clock();
	}
}

void EventQueueDeInit(void)
{
	if(stack != NULL) {
	    _AX = 0;
	    geninterrupt(0x33);
	    setvect(9,_ev_oldkbint);
	    free(stack);
	    free(qsave);
	    stack = NULL;
	}
}

EventQueue *EventQueueInit(int qsize,int ms_stksize,void (*msdraw)(void))
{
	if(stack != NULL) EventQueueDeInit();
	if(qsize < 20) qsize = 20;
	if(ms_stksize < MS_SSIZE) ms_stksize = MS_SSIZE;
	stack = malloc(KB_SSIZE + ms_stksize);
	qsave = malloc(sizeof(EventQueue)+(sizeof(EventRecord)*(qsize-1))+4);
	if((stack == NULL) || (qsave == NULL)) {
	    if(stack != NULL) { free(stack); stack = NULL; }
	    if(qsave != NULL) { free(qsave); qsave = NULL; }
	    return(NULL);
	}
	_ev_interds = FP_SEG(&ms_xpos);
	_ev_interss = FP_SEG(stack);
	_ev_kbintsp = FP_OFF(stack) + KB_SSIZE;
	_ev_msintsp = FP_OFF(stack) + KB_SSIZE + ms_stksize;
	_ev_kbinter = (-1);
	ms_xpos = ms_xmickey = 0;
	ms_ypos = ms_ymickey = 0;
	queue = (EventQueue *)(((long)qsave + 3L) & ~3L);
	memset(queue,0,sizeof(EventQueue));
	queue->evq_maxsize   = qsize;
	queue->evq_xmax	     = 79;
	queue->evq_ymax	     = 24;
	queue->evq_xspeed    = 8;
	queue->evq_yspeed    = 16;
	queue->evq_thresh    = 100;
	queue->evq_accel     = 1;
	queue->evq_delchar   = 1;
	queue->evq_enable    = MS_ENABLE | KB_ENABLE;
	_AX = 0;
	geninterrupt(0x33);
	if(_AX != 0) {
	    _AX = 11;
	    geninterrupt(0x33);
	    mousedraw = (msdraw != NULL) ? msdraw : dummydraw;
	    _ES = FP_SEG(_ev_mouseint);
	    _DX = FP_OFF(_ev_mouseint);
	    _CX = 0xff;
	    _AX = 0x0c;
	    geninterrupt(0x33);
	}
	_ev_oldkbint = getvect(9);
	setvect(9,_ev_keybdint);
	if(first_call) {
	    atexit(EventQueueDeInit);
	    first_call = 0;
	}
	return(queue);
}

