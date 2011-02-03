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
./attack.o: attack.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./birth.o: birth.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cave.o: cave.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd0.o: cmd0.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd1.o: cmd1.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd2.o: cmd2.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd3.o: cmd3.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd4.o: cmd4.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd5.o: cmd5.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./cmd6.o: cmd6.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./dungeon.o: dungeon.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./files.o: files.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./generate.o: generate.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./identify.o: identify.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./info.o: info.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./init1.o: init1.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h init.h
./init2.o: init2.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h init.h
./jewel.o: jewel.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h jewel.h
./load.o: load.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./monattk.o: monattk.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./monmove.o: monmove.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./monster1.o: monster1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./monster2.o: monster2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./object1.o: object1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./object2.o: object2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./randart.o: randart.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./randname.o: randname.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h randname.h
./save.o: save.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./score.o: score.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./spells1.o: spells1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./spells2.o: spells2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./squelch.o: squelch.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./store.o: store.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./tables.o: tables.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./ui.o: ui.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./util.o: util.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h button.h game-event.h macro.h
./variable.o: variable.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./wizard1.o: wizard1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./wizard2.o: wizard2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./xtra1.o: xtra1.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./xtra2.o: xtra2.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h z-term.h ui-event.h z-file.h config.h types.h externs.h cmds.h \
 ui.h z-textblock.h z-type.h
./z-file.o: z-file.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./z-form.o: z-form.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./z-rand.o: z-rand.c z-rand.h h-basic.h defines.h
./z-term.o: z-term.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h z-term.h ui-event.h z-file.h config.h types.h \
 externs.h cmds.h ui.h z-textblock.h z-type.h
./z-type.o: z-type.c h-basic.h z-form.h z-term.h ui-event.h ui.h \
 z-textblock.h z-file.h defines.h z-type.h
./z-util.o: z-util.c z-util.h h-basic.h
./z-virt.o: z-virt.c z-virt.h h-basic.h z-util.h
