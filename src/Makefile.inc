#
# File: Makefile.inc
#
# Includes Makefile.src and contains dependency info
#

include Makefile.src

BASEOBJS := $(ANGFILES) $(ZFILES)

depgen:
	head -n 19 Makefile.inc > Makefile.new
	@for i in $(BASEOBJS:.o=.c); do \
		DIR=`echo ./$$i | sed 's|/[^/]*$$||'`; \
			gcc -MM -I. $$i | \
			sed "s|^\\([^ ]\\)|$$DIR/\\1|" >> Makefile.new; \
	done;

# Dependencies
./attack.o: attack.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./birth.o: birth.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cave.o: cave.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd0.o: cmd0.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd1.o: cmd1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd2.o: cmd2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd3.o: cmd3.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd4.o: cmd4.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd5.o: cmd5.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./cmd6.o: cmd6.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./dungeon.o: dungeon.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./files.o: files.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./generate.o: generate.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./identify.o: identify.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./info.o: info.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./init1.o: init1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./init2.o: init2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./jewel.o: jewel.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./load.o: load.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./monattk.o: monattk.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./monmove.o: monmove.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./monster1.o: monster1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./monster2.o: monster2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./object1.o: object1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./object2.o: object2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./prefs.o: prefs.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./randart.o: randart.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./randname.o: randname.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./save.o: save.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./score.o: files.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./spells1.o: spells1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./spells2.o: spells2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./squelch.o: squelch.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./store.o: store.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./tables.o: tables.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./ui.o: ui.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./util.o : util.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./variable.o: variable.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./wizard1.o: wizard1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./wizard2.o: wizard2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./xtra1.o: xtra1.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./xtra2.o: xtra2.c angband.h cmds.h config.h defines.h externs.h \
  h-basic.h h-config.h h-define.h h-system.h h-type.h init.h \
  main.h randname.h types.h ui.h z-file.h z-form.h z-rand.h z-term.h \
  z-type.h z-util.h z-virt.h
./z-file.o: z-file.c z-file.h h-basic.h h-config.h h-define.h h-system.h \
  h-type.h z-virt.h z-util.h z-form.h
./z-form.o: z-form.c z-form.h h-basic.h h-config.h h-define.h h-system.h \
  h-type.h z-type.h z-util.h z-virt.h
./z-rand.o: z-rand.c z-rand.h h-basic.h h-config.h h-define.h h-system.h \
  h-type.h 
./z-term.o: z-term.c z-term.h h-basic.h h-config.h h-define.h h-system.h \
  h-type.h ui.h z-virt.h
./z-type.o: z-type.c h-basic.h h-config.h h-define.h h-system.h h-type.h \
  z-form.h z-term.h ui.h z-type.h
./z-util.o: z-util.c z-util.h h-basic.h h-config.h h-define.h h-system.h \
  h-type.h 
./z-virt.o: z-virt.c z-virt.h h-basic.h h-config.h h-define.h h-system.h \
  h-type.h z-util.h
