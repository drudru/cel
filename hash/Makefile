
CFLAGS =  -g -O2 -Wall -fno-omit-frame-pointer

#.cc.o:
#	gcc $(CFLAGS) -c $<


ifdef PROFILE
CFLAGS += -pg
endif


O = recycle.o lookupa.o hashtab.o unique.o

libhash.a: recycle.o lookupa.o hashtab.o
	ar -ru libhash.a recycle.o lookupa.o hashtab.o; ranlib libhash.a

unique : $(O)
	$(CC) -o unique $(O) 

clean:
	rm libhash.a *.o
# DEPENDENCIES

recycle.o : recycle.c standard.h recycle.h

lookupa.o : lookupa.c standard.h lookupa.h

hashtab.o : hashtab.c standard.h recycle.h lookupa.h hashtab.h

unique.o  : unique.c standard.h hashtab.h
