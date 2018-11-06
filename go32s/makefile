# History:25,17

AFLAGS = /mx /zi /zd
#CFLAGS = -ms -M -DSUPPORT_UDI -Iudi
CFLAGS = -ms -M -I\borlandc\include -L\borlandc\lib
# -DDEBUG_PIC : debug interrrupt controller picture
# -DGO32_STAT="f:\\go32_stat" : generate a statistics file about interrupt calls
# -DDEBUG_SIGFPE -DDEBUG_SIG

.c.obj:
#	bcc $(CFLAGS) -DDEBUGGER=1 -DTOPLINEINFO=1 -DSOURCE_LIST -c $*.c
#	-mv $*.obj d$*.obj
	bcc $(CFLAGS) -DDEBUGGER=0 -DTOPLINEINFO=0 -c $*.c

.asm.obj:
#	tasm $(AFLAGS) /DDEBUGGER=1 /DTOPLINEINFO=1 $*;
#	-mv $*.obj d$*.obj
	tasm $(AFLAGS) /DDEBUGGER=0 /DTOPLINEINFO=0 $*;

OBJS =\
	control.obj\
	dalloc.obj\
	debug.obj\
	doutils.obj\
	event.obj\
	evintr.obj\
	exphdlr.obj\
	graphics.obj\
	grprot.obj\
	mono.obj\
	mswitch.obj\
	npx.obj\
	paging.obj\
	syms.obj\
	tables.obj\
	unassmbl.obj\
	ustat.obj\
	utils.obj\
	valloc.obj\
	vga.obj\
	wild.obj\
	xms.obj\
	vcpi.obj\
	go32sig.obj

all : go32t.exe stub.exe aout2exe.exe exe2aout.exe

stub.exe: stub.c
	bcc $(CFLAGS) stub.c
	tdstrip stub

# no debugging information
NOV:
	tlink /3    /c          @go32.lnk

# Note: some tlinks require /3 here, some barf if you supply it.
go32t.exe : $(OBJS) go32.lnk 
	tlink /3    /c /v       @go32.lnk
#	tlink /3 /l /c /v /s /m @go32.lnk
#	tlink /3 /l /c /v /s /m @debug32.lnk

bin2byte.exe : bin2byte.c
	bcc $(CFLAGS) bin2byte.c
	-tdstrip bin2byte

stubbyte.h : bin2byte.exe stub.exe
	bin2byte stub.exe stub_bytes stubbyte.h

aout2exe.exe : aout2exe.c stubbyte.h
	bcc $(CFLAGS) aout2exe.c
	-tdstrip aout2exe

exe2aout.exe : exe2aout.c
	bcc $(CFLAGS) exe2aout.c
	-tdstrip exe2aout

# DEPENDENCIES

debug.obj : build.h types.h gdt.h tss.h utils.h unassmbl.h syms.h paging.h npx.h mono.h eventque.h

exphdlr.obj : build.h types.h gdt.h idt.h tss.h utils.h paging.h npx.h mono.h vcpi.h eventque.h 

control.obj : build.h types.h gdt.h idt.h tss.h valloc.h utils.h syms.h graphics.h mono.h vcpi.h

graphics.obj : build.h types.h paging.h graphics.h tss.h gdt.h driver.h

paging.obj : build.h types.h paging.h graphics.h tss.h idt.h gdt.h valloc.h dalloc.h utils.h aout.h mono.h vcpi.h

syms.obj : build.h types.h syms.h tss.h stab.h stab.def aout.h utils.h

unassmbl.obj : build.h types.h gdt.h idt.h tss.h utils.h unassmbl.h syms.h mono.h

utils.obj : build.h types.h tss.h gdt.h utils.h npx.h

valloc.obj : build.h types.h valloc.h xms.h mono.h vcpi.h

xms.obj : xms.h

dalloc.obj : build.h types.h valloc.h dalloc.h mono.h

mono.obj : mono.h

aout2exe.obj : stubbytes.h

event.obj : eventque.h

doutils.obj : build.inc segdefs.inc tss.inc gdt.inc idt.inc

mswitch.obj : build.inc segdefs.inc tss.inc gdt.inc idt.inc vcpi.inc

tables.obj : build.inc segdefs.inc tss.inc gdt.inc idt.inc

vga.obj : segdefs.inc

grprot.obj : build.inc segdefs.inc tss.inc gdt.inc idt.inc

npx.obj : build.inc segdefs.inc tss.inc gdt.inc idt.inc

vcpi.obj : build.inc segdefs.inc vcpi.inc
