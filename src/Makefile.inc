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
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h monster.h target.h
./birth.o: birth.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h files.h game-event.h history.h squelch.h \
 ui-menu.h
./buildid.o: buildid.c buildid.h
./button.o: button.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h
./cave.o: cave.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h game-event.h squelch.h trap.h
./charattr.o: charattr.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h ui-menu.h
./cmd0.o: cmd0.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h files.h keymap.h monster.h ui-menu.h
./cmd1.o: cmd1.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h history.h monster.h squelch.h \
 trap.h
./cmd2.o: cmd2.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h files.h history.h monster.h squelch.h \
 trap.h
./cmd3.o: cmd3.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h monster.h squelch.h target.h \
 ui-menu.h
./cmd4.o: cmd4.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h button.h cave.h files.h history.h \
 prefs.h
./cmd-misc.o: cmd-misc.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h monster.h trap.h ui-menu.h
./cmd-obj.o: cmd-obj.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h effects.h list-effects.h game-event.h \
 monster.h target.h squelch.h
./death.o: death.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h files.h game-event.h history.h \
 savefile.h ui-menu.h
./dungeon.o: dungeon.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h files.h game-event.h generate.h \
 init.h monster.h prefs.h savefile.h target.h
./effects.o: effects.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h effects.h list-effects.h monster.h \
 target.h
./files.o: files.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h button.h cave.h files.h history.h \
 savefile.h ui-menu.h
./game-cmd.o: game-cmd.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h target.h trap.h
./game-event.o: game-event.c z-virt.h h-basic.h game-event.h
./generate.o: generate.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h monster.h trap.h
./gen-cave.o: gen-cave.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h monster.h
./gen-monster.o: gen-monster.c angband.h h-basic.h z-util.h z-virt.h \
 z-form.h z-rand.h defines.h tvalsval.h list-blow-methods.h \
 list-blow-effects.h list-square-flags.h list-object-flags.h \
 list-curse-flags.h list-kind-flags.h list-identify-flags.h \
 list-player-flags.h list-terrain-flags.h list-trap-flags.h \
 list-mon-flags.h list-mon-spells.h z-term.h ui-event.h z-file.h \
 z-bitflag.h z-quark.h z-msg.h config.h game-cmd.h types.h option.h \
 list-options.h cmds.h object.h z-textblock.h parser.h player.h ui.h \
 z-type.h externs.h spells.h list-gf-types.h generate.h monster.h
./gen-room.o: gen-room.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h monster.h trap.h
./gen-util.o: gen-util.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h monster.h trap.h
./gen-wilderness.o: gen-wilderness.c angband.h h-basic.h z-util.h z-virt.h \
 z-form.h z-rand.h defines.h tvalsval.h list-blow-methods.h \
 list-blow-effects.h list-square-flags.h list-object-flags.h \
 list-curse-flags.h list-kind-flags.h list-identify-flags.h \
 list-player-flags.h list-terrain-flags.h list-trap-flags.h \
 list-mon-flags.h list-mon-spells.h z-term.h ui-event.h z-file.h \
 z-bitflag.h z-quark.h z-msg.h config.h game-cmd.h types.h option.h \
 list-options.h cmds.h object.h z-textblock.h parser.h player.h ui.h \
 z-type.h externs.h spells.h list-gf-types.h cave.h generate.h monster.h
./grafmode.o: grafmode.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h grafmode.h
./history.o: history.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h history.h
./identify.o: identify.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h history.h
./init.o: init.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h button.h cave.h game-event.h \
 generate.h history.h init.h keymap.h monster.h prefs.h randname.h \
 squelch.h list-effects.h
./jewel.o: jewel.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h jewel.h effects.h list-effects.h
./keymap.o: keymap.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h keymap.h
./load.o: load.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h history.h monster.h savefile.h squelch.h
./load-old.o: load-old.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h
./monattk.o: monattk.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h monster.h
./monmove.o: monmove.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h files.h monster.h squelch.h trap.h
./monster1.o: monster1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./monster2.o: monster2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h history.h monster.h target.h trap.h
./obj-desc.o: obj-desc.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h squelch.h
./obj-info.o: obj-info.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h effects.h list-effects.h
./obj-make.o: obj-make.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h squelch.h
./obj-ui.o: obj-ui.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h keymap.h target.h ui-menu.h
./obj-util.o: obj-util.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h effects.h list-effects.h generate.h \
 history.h prefs.h randname.h squelch.h
./option.o: option.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./parser.o: parser.c parser.h h-basic.h z-bitflag.h z-form.h z-virt.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-rand.h z-file.h z-util.h z-term.h ui-event.h \
 z-quark.h z-msg.h z-textblock.h types.h option.h list-options.h \
 externs.h spells.h list-gf-types.h
./pathfind.o: pathfind.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h squelch.h trap.h
./play-calcs.o: play-calcs.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h game-event.h monster.h squelch.h trap.h
./play-spell.o: play-spell.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h trap.h
./play-timed.o: play-timed.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h
./play-util.o: play-util.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h
./prefs.o: prefs.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h keymap.h prefs.h squelch.h
./randart.o: randart.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h effects.h list-effects.h files.h
./randname.o: randname.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h randname.h
./save.o: save.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h squelch.h history.h monster.h savefile.h
./savefile.o: savefile.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h savefile.h
./score.o: score.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h
./signals.o: signals.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h files.h savefile.h
./specialty.o: specialty.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h history.h ui-menu.h
./spells1.o: spells1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h history.h monster.h trap.h
./spells2.o: spells2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h history.h generate.h monster.h squelch.h \
 target.h trap.h ui-menu.h
./squelch.o: squelch.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h squelch.h ui-menu.h
./store.o: store.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h files.h game-event.h history.h \
 squelch.h textui.h ui-menu.h
./tables.o: tables.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./trap.o: trap.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h generate.h monster.h squelch.h trap.h \
 ui-menu.h
./target.o: target.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h monster.h squelch.h trap.h
./ui-birth.o: ui-birth.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h files.h game-event.h ui-birth.h \
 ui-menu.h
./ui.o: ui.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./ui-event.o: ui-event.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./ui-knowledge.o: ui-knowledge.c angband.h h-basic.h z-util.h z-virt.h \
 z-form.h z-rand.h defines.h tvalsval.h list-blow-methods.h \
 list-blow-effects.h list-square-flags.h list-object-flags.h \
 list-curse-flags.h list-kind-flags.h list-identify-flags.h \
 list-player-flags.h list-terrain-flags.h list-trap-flags.h \
 list-mon-flags.h list-mon-spells.h z-term.h ui-event.h z-file.h \
 z-bitflag.h z-quark.h z-msg.h config.h game-cmd.h types.h option.h \
 list-options.h cmds.h object.h z-textblock.h parser.h player.h ui.h \
 z-type.h externs.h spells.h list-gf-types.h cave.h grafmode.h history.h \
 monster.h squelch.h ui-menu.h
./ui-menu.o: ui-menu.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h ui-menu.h
./ui-options.o: ui-options.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h keymap.h squelch.h prefs.h ui-menu.h \
 files.h
./ui-spell.o: ui-spell.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h ui-menu.h
./util.o: util.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h game-event.h randname.h
./variable.o: variable.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h
./wizard1.o: wizard1.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h buildid.h monster.h
./wizard2.o: wizard2.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h files.h monster.h target.h ui-menu.h
./x-spell.o: x-spell.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h effects.h list-effects.h \
 target.h
./xtra2.o: xtra2.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h cave.h history.h keymap.h squelch.h target.h
./xtra3.o: xtra3.c angband.h h-basic.h z-util.h z-virt.h z-form.h z-rand.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h button.h cave.h files.h game-event.h monster.h \
 textui.h ui-birth.h
./z-bitflag.o: z-bitflag.c z-bitflag.h h-basic.h z-form.h z-virt.h \
 defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h
./z-file.o: z-file.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./z-form.o: z-form.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./z-msg.o: z-msg.c z-virt.h h-basic.h z-term.h ui-event.h z-msg.h
./z-quark.o: z-quark.c z-virt.h h-basic.h z-quark.h
./z-rand.o: z-rand.c z-rand.h h-basic.h defines.h tvalsval.h \
 list-blow-methods.h list-blow-effects.h list-square-flags.h \
 list-object-flags.h list-curse-flags.h list-kind-flags.h \
 list-identify-flags.h list-player-flags.h list-terrain-flags.h \
 list-trap-flags.h list-mon-flags.h list-mon-spells.h
./z-set.o: z-set.c z-set.h h-basic.h z-rand.h defines.h tvalsval.h \
 list-blow-methods.h list-blow-effects.h list-square-flags.h \
 list-object-flags.h list-curse-flags.h list-kind-flags.h \
 list-identify-flags.h list-player-flags.h list-terrain-flags.h \
 list-trap-flags.h list-mon-flags.h list-mon-spells.h z-virt.h
./z-term.o: z-term.c angband.h h-basic.h z-util.h z-virt.h z-form.h \
 z-rand.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-term.h ui-event.h z-file.h z-bitflag.h z-quark.h \
 z-msg.h config.h game-cmd.h types.h option.h list-options.h cmds.h \
 object.h z-textblock.h parser.h player.h ui.h z-type.h externs.h \
 spells.h list-gf-types.h
./z-type.o: z-type.c h-basic.h z-form.h z-term.h ui-event.h ui.h \
 z-textblock.h z-file.h defines.h tvalsval.h list-blow-methods.h \
 list-blow-effects.h list-square-flags.h list-object-flags.h \
 list-curse-flags.h list-kind-flags.h list-identify-flags.h \
 list-player-flags.h list-terrain-flags.h list-trap-flags.h \
 list-mon-flags.h list-mon-spells.h z-type.h z-virt.h
./z-util.o: z-util.c z-util.h h-basic.h
./z-virt.o: z-virt.c z-virt.h h-basic.h z-util.h
./z-textblock.o: z-textblock.c z-term.h h-basic.h ui-event.h z-textblock.h \
 z-file.h defines.h tvalsval.h list-blow-methods.h list-blow-effects.h \
 list-square-flags.h list-object-flags.h list-curse-flags.h \
 list-kind-flags.h list-identify-flags.h list-player-flags.h \
 list-terrain-flags.h list-trap-flags.h list-mon-flags.h \
 list-mon-spells.h z-virt.h z-form.h
