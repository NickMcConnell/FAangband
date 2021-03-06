MKPATH=mk/
include $(MKPATH)buildsys.mk

#SUBDIRS = src lib doc
SUBDIRS = src lib
CLEAN = config.status config.log *.dll *.exe

.PHONY: tests manual clean-manual
tests:
	$(MAKE) -C src tests

manual:
	$(MAKE) -C doc manual.html manual.pdf

clean-manual:
	$(MAKE) -C doc clean
