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

#CFLAGS= -I../hash -I../vm -g -Wall -fPIC -O2 -fno-omit-frame-pointer
CFLAGS= -m32 -I../hash -I../vm -g -Wall -O2 -fno-omit-frame-pointer

ifdef PROFILE
CFLAGS += -pg
endif

#FILES=OS.so Random.so Socket.so DateTime.so SleepyCat.so Math.so GD.so
#FILES=OS.o Socket.o DateTime.o Math.o
include kitmake.make

allkits: $(FILES)
	ar -ru libkits.a $^ ; ranlib libkits.a

#OS.so: OS.o
#	$(CC) -shared -o OS.so $^
#
#Random.so: Random.o
##	$(CC) -shared -o Random.so $^
#
##Socket.o: Socket.c
#	$(CC) -shared -o Socket.so $^
#
#DateTime.so: DateTime.o
#	$(CC) -shared -o DateTime.so $^
#
#Math.o: Math.c
#	$(CC) $(CFLAGS) -c $^ 
#
#Math.so: Math.o
#	$(CC) -shared -o Math.so $^ -lm
#
#GD.so: GD.o
#	$(CC) -shared -o GD.so $^ /usr/local/lib/libgd.a -lpng
#
#SleepyCat.o: SleepyCat.c
#	$(CC) $(CFLAGS) -I/usr/local/BerkeleyDB.3.3/include -c $^ 
#
#SleepyCat.so: SleepyCat.o
#	$(CC) -shared -I/usr/local/BerkeleyDB.3.3/include -L/usr/local/BerkeleyDB.3.3/lib -o SleepyCat.so $^ -ldb-3
#
clean:
	rm *.[oa]; rm *.so; rm Compiled/*.celmod;

reallyclean:
	rm *.[oa] *.depend *~

#backup:
#	tar cvf /dev/fd0.1440 *.[mh] Makefile build

