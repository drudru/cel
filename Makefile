CFLAGS = -O -g

# DEPENDENCIES

SUBDIRS=	hash vm
SUBDIRS2=	kits
MAKE=		make

all:
	for i in $(SUBDIRS); do \
		(cd $$i; $(MAKE) ); done

allkits:
	for i in $(SUBDIRS2); do \
		(cd $$i; $(MAKE) ); done

profile:
	for i in $(SUBDIRS); do \
		(cd $$i; $(MAKE) PROFILE=1); done



clean:
	-for i in $(SUBDIRS); do \
		if test -d $$i; then \
			(echo cleaning subdirectory $$i; cd $$i; \
			if test -f Makefile; \
			then $(MAKE) clean; \
			fi); \
		else true; fi; \
		done
	-for i in $(SUBDIRS2); do \
		if test -d $$i; then \
			(echo cleaning subdirectory $$i; cd $$i; \
			if test -f Makefile; \
			then $(MAKE) clean; \
			fi); \
		else true; fi; \
		done

allclean:
	-for i in $(SUBDIRS); do \
		if test -d $$i; then \
			(echo cleaning subdirectory $$i; cd $$i; \
			if test -f Makefile; \
			then $(MAKE) clean; \
			fi); \
		else true; fi; \
		done
	rm -rf samples/Compiled
	rm -rf kits/Compiled
