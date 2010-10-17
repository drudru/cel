#
#  MAKEFILE for Classes
#  Copyright 1995 Dru Nelson
#
#  History:  ??/??/??  Created, Genetic Algrorithm's Makefile
#	     01/16/95  Modified heavily for SCO Unix
#	     06/05/95  Made a platform capable make system
#	     09/27/97  Made big changes and started using rsmake
# 	     08/09/98  Made use of for bubl vm
#

CFLAGS= -m32 -g -O2 -fno-omit-frame-pointer -Wall -I../hash -L/usr/local/lib 


ifdef PROFILE
CFLAGS += -pg -DPROF
endif

ifdef PHASE2
include kitlibs.make
endif

#%.o: %.m
#	gcc $(GCCFLAGS) -c $<
#
#%.o: %.c
#	gcc $(GCCFLAGS) -c $<

FILES= alloc.o stack.o stackframe.o atom.o crc.o memstream.o runtime.o sd2cel.o cel2sd.o svm32.o assemble.o array.o compiler.o codegen.o while.o OS.o

cel: main.o $(FILES)
#	$(CC) $(CFLAGS) -export-dynamic -o cel $^ ../hash/libhash.a -ldl
	$(CC) $(CFLAGS) -o cel $^ ../hash/libhash.a $(CLIBS) 

runtime.o: runtime.c kitlibs.h
	$(CC) $(CFLAGS) -c runtime.c

libcel.a: main.o $(FILES)
	ar -ru libcel.a main.o $(FILES); ranlib libcel.a

celdump: celdump.o $(FILES)
	$(CC) $(CFLAGS) -o celdump  $^ ../hash/libhash.a

libcel.so.0.0: $(FILES)
	$(CC) -shared -Wl,-soname,libcel.so.0 -o libcel.so.0.0 $^ ../hash/libhash.a

clean:
	rm *.[oa]; rm -rf cel; rm -rf celdump

reallyclean:
	rm *.[oa] *.depend *~

#backup:
#	tar cvf /dev/fd0.1440 *.[mh] Makefile build

