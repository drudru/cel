#
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
#

#GCC= /usr/local/bin/gcc
GCC= gcc
GCCFLAGS= -I../hash -I../celmalloc -I../vm -g -Wall  -L../hash `gtk-config --cflags` 
GGGCCFLAGS= -I../hash -I../aqmalloc -I../vm -g -Wall  -L../hash `gtk-config --cflags` `gtk-config --libs`

%.o: %.m
	gcc $(GCCFLAGS) -Wno-import -c $<

%.o: %.c
	gcc $(GCCFLAGS) -Wno-import -c $<

FILES= main.o ../vm/runtime.o 

aqdbg: $(FILES)
	gcc $(GGGCCFLAGS) -o aqdbg  $^ ../vm/atom.o ../vm/assemble.o ../vm/sd2cel.o ../vm/memstream.o ../vm/crc.o ../vm/cel2sd.o ../vm/svm32.o ../vm/array.o ../vm/codegen.o ../vm/compiler.o ../vm/while.o ../hash/libhash.a ../celmalloc/celmalloc.o

###  #include *.depend

clean:
	rm *.[oa]

reallyclean:
	rm *.[oa] *.depend *~

backup:
	tar cvf /dev/fd0.1440 *.[mh] Makefile build

