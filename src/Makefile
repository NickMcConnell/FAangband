MKPATH=../mk/
include $(MKPATH)buildsys.mk

# import source list
include Makefile.src
INCLUDE = $(HDRS) $(INCS)
OBJECTS = $(ANGFILES) $(ZFILES)
SRCS    = ${OBJECTS:.o=.c} ${MAINFILES:.o=.c}
VERSION := $(shell ../scripts/version.sh)
CFLAGS += -DBUILD_ID=${VERSION} -I. -std=c99 -Wdeclaration-after-statement -Wall -Wextra -O0

CLEAN = angband.o $(OBJECTS) win/angband.res
DISTCLEAN = autoconf.h

export CFLAGS LDFLAGS LIBS

$(PROG): $(PROG).o $(MAINFILES)
	$(CC) -o $(PROG) $(PROG).o $(MAINFILES) $(LDFLAGS) $(LDADD) $(LIBS)
	@printf "%10s %-20s\n" LINK $@

win/$(PROG).res: win/$(PROG).rc
	$(RC) $< -O coff -o $@

$(PROG).o: $(OBJECTS)
	$(LD) -nostdlib -r -o $@ $(OBJECTS)
	@printf "%10s %-20s\n" LINK $@

#clean:: test-clean
#	-rm $(PROG).o

clean-extra:
	-rm autoconf.h

tests:
	$(MAKE) -C tests all

test-clean:
	$(MAKE) -C tests clean

splint:
	splint -f .splintrc ${OBJECTS:.o=.c} main.c main-gcu.c

.PHONY : tests
