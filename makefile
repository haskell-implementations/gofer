.SUFFIXES	: .y .c .h .o .gs .s
HFILES		= prelude.h storage.h connect.h errors.h command.h
CFILES		= gofer.c storage.c input.c static.c type.c \
		  output.c compiler.c machine.c builtin.c  \
		  gofc.c cmachine.c cbuiltin.c runtime.c
INCFILES	= parser.c preds.c prims.c kind.c subst.c \
		  machdep.c commonui.c
GC_SRC		= markscan.c twospace.c
YFILES		= parser.y
SOURCES		= $(HFILES) $(CFILES) $(INCFILES) $(YFILES) prelude
DJGPPOBJ	= signal.o sigman.o
OBJECTS		= storage.o input.o static.o type.o compiler.o $(DJGPPOBJ)
IOBJECTS	= gofer.o builtin.o  machine.o output.o $(OBJECTS)
COBJECTS	= gofc.o cbuiltin.o cmachine.o $(OBJECTS)
TESTS		= apr0 apr1 apr2 apr3 apr4 apr5 apr6 apr7 apr8 apr9 apr10

# Edit the following settings as required.
# There are two choices of command line editor that can be used with Gofer:
#
#  GNU readline:		usual GNU sources (e.g. bash distribution)
#  add -DUSE_READLINE=1 to CFLAGS and libreadline.a -ltermcap to LDFLAGS
#				      (or maybe -lreadline -ltermcap)
#
#  editline:			(comp.sources.misc, vol 31, issue 71)
#  add -DUSE_READLINE=1 to CFLAGS and libedit.a to LDFLAGS
#				      (or maybe -ledit)
#
# The best bet is to `touch prelude.h' after changing these settings to
# ensure that the whole collection of files is recompiled with the correct
# settings.

CC		= gcc
#CFLAGS		= -DLAMBDAVAR -DGOFC_INCLUDE=\"\\\"gofc.h\\\"\"
#CFLAGS		= -DLAMBDANU -DLAMBDAVAR -DGOFC_INCLUDE=\"\\\"gofc.h\\\"\" -DUSE_READLINE=1
CFLAGS		= -I. -I\djgpp\include
#LDFLAGS   	= -lm -ledit		# libedit.a is in my gcc lib directory
LDFLAGS    	= -lm
#LDFLAGS  	= -lm libedit.a
#LDFLAGS     	= -lm libreadline.a -ltermcap
OPTFLAGS	= -O

all		: gofer gofc runtime.o

tests		: $(TESTS)

gofer		: $(IOBJECTS)
		  $(CC) $(OPTFLAGS) $(IOBJECTS) -o gofer $(LDFLAGS)
		  strip gofer

gofc		: $(COBJECTS)
		  $(CC) $(OPTFLAGS) $(COBJECTS) -o gofc $(LDFLAGS)
		  strip gofc

.s.o		:
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) $<

.c.o		:
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) $<

clean		:
		  rm *.o $(TESTS)

install		:
		  mv gofer ..

.gs		:
		  ./gofc $*.gs
		  $(CC) $(OPTFLAGS) $*.c runtime.o -o $* $(LDFLAGS)
		  rm $*.c
		  strip $*

.gp		:
		  ./gofc + $*.gp
		  $(CC) $(OPTFLAGS) $*.c runtime.o -o $* $(LDFLAGS)
		  rm $*.c
		  strip $*

parser.c	: parser.y
		  -yacc parser.y
		  mv y.tab.c parser.c

gofer.o		: prelude.h storage.h connect.h errors.h \
		  command.h machdep.c commonui.c
gofc.o		: prelude.h storage.h connect.h errors.h \
		  command.h machdep.c commonui.c output.c
runtime.o	: prelude.h gofc.h machdep.c $(GC_SRC)
storage.o	: prelude.h storage.h connect.h errors.h
input.o		: prelude.h storage.h connect.h errors.h parser.c command.h
static.o	: prelude.h storage.h connect.h errors.h
type.o		: prelude.h storage.h connect.h errors.h preds.c kind.c subst.c
output.o	: prelude.h storage.h connect.h errors.h
compiler.o	: prelude.h storage.h connect.h errors.h
#		  $(CC) -c -O1 $(CFLAGS) compiler.c
machine.o	: prelude.h storage.h connect.h errors.h
cmachine.o	: prelude.h storage.h connect.h errors.h
builtin.o	: prelude.h storage.h connect.h errors.h prims.c
cbuiltin.o	: prelude.h storage.h connect.h errors.h prims.c

signal.o	: signal.c
sigman.o	: sigman.s
