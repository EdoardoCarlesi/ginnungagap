# Copyright (C) 2011, Steffen Knollmann
# Released under the terms of the GNU General Public License version 3.
# This file is part of `ginnungagap'.

LIBNAME = lare

include ../../Makefile.config

.PHONY: all clean dist-clean tests tests-clean

sources = lare.c \
          lareIO.c \
          lareReader.c \
          lareReaderLegacy.c \
          lareReaderLOI.c

sourcesTests = lib${LIBNAME}_tests.c \
               lare_tests.c

include ../../Makefile.rules

all:
	${MAKE} lib${LIBNAME}.a

clean:
	$(MAKE) tests-clean
	rm -f lib${LIBNAME}.a $(sources:.c=.o)

dist-clean:
	$(MAKE) clean
	rm -f $(sources:.c=.d) $(sourcesTests:.c=.d)

tests:
	$(MAKE) lib${LIBNAME}_tests
	./lib${LIBNAME}_tests

tests-clean:
	rm -f lib${LIBNAME}_tests $(sourcesTests:.c=.o) 

lib${LIBNAME}_tests: lib${LIBNAME}.a \
                     ../libutil/libutil.a \
                     $(sourcesTests:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o lib${LIBNAME}_tests \
	   $(sourcesTests:.c=.o) \
	   lib${LIBNAME}.a \
	   ../libutil/libutil.a \
	   $(LIBS)

lib${LIBNAME}.a: $(sources:.c=.o)
	$(AR) -rs lib${LIBNAME}.a $(sources:.c=.o)

../libutil/libutil.a:
	$(MAKE) -C ../libutil

-include $(sources:.c=.d)

-include $(sourcesTests:.c=.d)
