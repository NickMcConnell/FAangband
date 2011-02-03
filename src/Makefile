MKPATH=../mk/
include $(MKPATH)buildsys.mk

# import source list
include Makefile.src
INCLUDE = $(HDRS) $(INCS)
OBJECTS = $(ANGFILES) $(ZFILES)
SRCS    = ${OBJECTS:.o=.c} ${MAINFILES:.o=.c}
VERSION = `../scripts/version.sh`
CFLAGS += -DBUILD_ID=${VERSION} -I. -std=c99 -Wdeclaration-after-statement -Wall -Wextra -O0

$(PROG): $(PROG).o $(MAINFILES)
	$(CC) -o $(PROG) $(PROG).o $(MAINFILES) $(LDFLAGS) $(LDADD)
	@printf "%10s %-20s\n" LINK $@

$(PROG).o: $(OBJECTS)
	$(LD) -r -o $@ $(OBJECTS)
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
