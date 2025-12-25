/**
 * \file ui-death.c
 * \brief Handle the UI bits that happen after the character dies.
 *
 * Copyright (c) 1987 - 2007 Angband contributors
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "cmds.h"
#include "game-input.h"
#include "game-world.h"
#include "init.h"
#include "obj-desc.h"
#include "obj-info.h"
#include "savefile.h"
#include "store.h"
#include "ui-death.h"
#include "ui-history.h"
#include "ui-input.h"
#include "ui-knowledge.h"
#include "ui-menu.h"
#include "ui-object.h"
#include "ui-player.h"
#include "ui-score.h"
#include "ui-spoil.h"

/**
 * Write formatted string `fmt` on line `y`, centred between points x1 and x2.
 */
static void put_str_centred(int y, int x1, int x2, const char *fmt, ...)
{
	va_list vp;
	char *tmp;
	size_t len;
	int x;

	/* Format into the (growable) tmp */
	va_start(vp, fmt);
	tmp = vformat(fmt, vp);
	va_end(vp);

	/* Centre now; account for possible multibyte characters */
	len = utf8_strlen(tmp);
	x = x1 + ((x2-x1)/2 - len/2);

	put_str(tmp, y, x);
}


/**
 * Display the tombstone/retirement screen
 */
static void display_exit_screen(void)
{
	ang_file *fp;
	char buf[1024];
	int line = 7, start = 8;
	time_t death_time = (time_t)0;
	bool boat = player->total_winner && player_has(player, PF_ELVEN);
	bool tree = player->total_winner && player_has(player, PF_WOODEN);
	bool retired = streq(player->died_from, "Retiring");
	struct level *lev = &world->levels[player->place];


	Term_clear();
	(void)time(&death_time);

	/* Build the filename */
	if (tree) {
		path_build(buf, sizeof(buf), ANGBAND_DIR_SCREENS, "tree.txt");
		start = 12;
	} else if (boat) {
		path_build(buf, sizeof(buf), ANGBAND_DIR_SCREENS, "boat.txt");
		start = 12;
	} else if (retired) {
		path_build(buf, sizeof(buf), ANGBAND_DIR_SCREENS, "retire.txt");
	} else {
		path_build(buf, sizeof(buf), ANGBAND_DIR_SCREENS, "dead.txt");
	}

	/* Open the background picture */
	fp = file_open(buf, MODE_READ, FTYPE_TEXT);

	if (fp) {
		/* Dump the file to the screen */
		while (file_getl(fp, buf, sizeof(buf))) {
			text_out_e("%s", buf);
			text_out("\n");
		}

		file_close(fp);
	}

	put_str_centred(line++, start, start+31, "%s", player->full_name);
	put_str_centred(line++, start, start+31, "the");
	if (player->total_winner)
		put_str_centred(line++, start, start+31, "Magnificent");
	else
		put_str_centred(line++, start, start+31, "%s", player->class->title[(player->lev - 1) / 5]);

	line++;

	put_str_centred(line++, start, start+31, "%s", player->class->name);
	put_str_centred(line++, start, start+31, "Level: %d", (int)player->lev);
	put_str_centred(line++, start, start+31, "Exp: %d", (int)player->exp);
	put_str_centred(line++, start, start+31, "AU: %d", (int)player->au);
	if (boat) {
		put_str_centred(line++, start, start+31, "Sailed victorious to Aman.");
	} else if (tree) {
		put_str_centred(line++,start, start+31, "Retired to Fangorn Forest.");
	} else if (retired) {
		put_str_centred(line++, 8, 8+31, "Retired in %s %d.",
						locality_name(lev->locality), lev->depth);
	} else {
		if (lev->depth) {
			put_str_centred(line++, start, start+31, "Killed in %s %d",
							locality_name(lev->locality), lev->depth);
		} else {
			put_str_centred(line++, start, start+31, "Killed in %s Town",
							locality_name(lev->locality));
		}
		put_str_centred(line++, start, start+31, "by %s.", player->died_from);
	}

	line++;

	put_str_centred(line, start, start+31, "on %-.24s", ctime(&death_time));
}


/**
 * Gets a personalized string for ghosts.  Code originally from get_name. -LM-
 */
static char *get_personalized_string(uint8_t choice)
{
	static char tmp[80], info[80];
	uint8_t n, i;
	bool no_string = true;

	/* Clear last line */
	clear_from(15);

	/* Prompt and ask */
	if (choice == 1) {
		prt("Enter a message for your character's ghost", 15, 0);
		prt("above, or hit ESCAPE.", 16, 0);
	} else if (choice == 2) {
		prt("Enter an addition to your character ghost's", 15, 0);
		prt("description above, or hit ESCAPE.", 16, 0);
	} else
		return NULL;

	strnfmt(info, sizeof(info),"(%d characters maximum.  Entry will be used as",
			79);

	prt(info, 17, 0);
	prt("(a) sentence(s).)", 18, 0);

	/* Ask until happy */
	while (1) {
		/* Start at beginning of field. */
		Term_gotoxy(0, 14);

		/* Get an input */
		(void) askfor_aux(tmp, sizeof(tmp) - 1, NULL);

		/* All done */
		break;
	}

	/* Pad the string (to clear junk and allow room for a ending) */
	strnfmt(tmp, sizeof(tmp), "%-79.79s", tmp);

	/* Ensure that strings end like a sentence, and neatly clip the string. */
	for (n = 78; n > 0 ; n--) {
		if ((tmp[n] == ' ') || (tmp[n] == '\0')) continue;
		no_string = false;
		if ((tmp[n] == '!') || (tmp[n] == '.') || (tmp[n] == '?')) {
			tmp[n + 1] = '\0';
			for (i = n + 2; i < 80; i++) {
				tmp[i] = '\0';
			}
			break;
		} else {
			tmp[n + 1] = '.';
			tmp[n + 2] = '\0';
			for (i = n + 3; i < 80; i++) {
				tmp[i] = '\0';
			}
			break;
		}
	}

	/* Start the sentence with a capital letter. */
	if (islower((unsigned char)tmp[0]))
		tmp[0] = toupper((unsigned char)tmp[0]);

	/* Return the string */
	return no_string ? NULL : tmp;

}

/**
 * Save a "bones" file for a dead character.  Now activated and (slightly) 
 * altered.  Allows the inclusion of personalized strings. 
 */
static void make_bones(void)
{
	ang_file *fp;
	char str[1024];
	ui_event answer;
	uint8_t choice = 0;
	int i;
	bool path_written = false;
	bool no_answer = true;
	int level;
	char tmp[128];

	/* Ignore wizards and borgs */
	if (player->noscore & (NOSCORE_WIZARD | NOSCORE_DEBUG)) return;

	/* Ignore suicide */
	if (!player->total_winner && streq(player->died_from, "Quitting")) return;

	/* Ignore people who die in town */
	if (!player->depth) return;

	/* Slightly more tenacious saving routine. */
	for (i = 0; i < 5; i++) {
		/* Ghost hovers near level of death. */
		if (i == 0) {
			level = player->depth;
		} else {
			level = player->depth + 5 - damroll(2, 4);
		}
		if (level < 1)
			level = randint1(4);

		/* "Bones" name */
		strnfmt(tmp, sizeof(tmp), "bone.%03d", level);

		/* Build the filename */
		path_build(str, sizeof(str), ANGBAND_DIR_BONE, tmp);
		path_written = true;

		/* Attempt to open the bones file */
		fp = file_open(str, MODE_READ, FTYPE_TEXT);

		/* Close it right away */
		if (fp) file_close(fp);

		/* Do not over-write a previous ghost */
		if (fp) continue;

		/* If no file by that name exists, we can make a new one. */
		if (!(fp)) break;
	}

	/* Failure */
	if (fp) return;

	/* Try to write a new "Bones File" */
	fp = file_open(str, MODE_WRITE, FTYPE_TEXT);

	/* Not allowed to write it? Weird. */
	if (!fp) {
		if (path_written) file_delete(str);
		return;
	}

	/* Save the info */
	if (player->full_name[0] != '\0') {
		file_putf(fp, "%s\n", player->full_name);
	} else {
		file_putf(fp, "Anonymous\n");
	}

	file_putf(fp, "%d\n", player->race->ridx);
	file_putf(fp, "%d\n", player->class->cidx);

	/* Clear screen */
	Term_clear();

	/* Ask if the player wants to add a personalized string. */
	prt("Information about your character has been saved", 15, 0);
	prt("in a bones file.  Would you like to give the", 16, 0);
	prt("ghost a special message or description? (y/n)", 17, 0);

	while (no_answer) {
		answer = inkey_ex();

		/* Clear last line */
		clear_from(15);
		clear_from(16);

		/* Determine what the personalized string will be used for.  */
		if ((answer.key.code == 'Y') || (answer.key.code == 'y')) {
			prt("Will you add something for your ghost to say,", 15, 0);
			prt("or add to the monster description?", 16, 0);
			prt("((M)essage/(D)escription/ESC for neither)", 17, 0);

			while (1) {
				answer = inkey_ex();

				clear_from(15);
				clear_from(16);

				if ((answer.key.code == 'M') ||
					(answer.key.code == 'm')) {
					choice = 1;
					no_answer = false;
					break;
				} else if ((answer.key.code == 'D') ||
						   (answer.key.code == 'd')) {
					choice = 2;
					no_answer = false;
					break;
				} else if (answer.key.code == ESCAPE) {
					choice = 0;
					no_answer = false;
					break;
				}
			}
		} else if ((answer.key.code == 'N') ||
				   (answer.key.code == 'n') ||
				   (answer.key.code == ESCAPE)) {
			choice = 0;
			break;
		}
	}

	/* If requested, get the personalized string, and write it and
	 * info on how it should be used in the bones file.  Otherwise,
	 * indicate the absence of such a string. */
	if (choice) {
		char *string = get_personalized_string(choice);
		if (string) {
			file_putf(fp, "%d:%s\n", choice, string);
		} else {
			file_putf(fp, "0: \n");
		}
	} else {
		file_putf(fp, "0: \n");
	}

	/* Close and save the Bones file */
	file_close(fp);
}

/**
 * Display the winner crown
 */
static void display_winner(void)
{
	char buf[1024];
	ang_file *fp;

	int wid, hgt;
	int i = 2;

	path_build(buf, sizeof(buf), ANGBAND_DIR_SCREENS, "crown.txt");
	fp = file_open(buf, MODE_READ, FTYPE_TEXT);

	Term_clear();
	Term_get_size(&wid, &hgt);

	if (fp) {
		char *pe;
		long lw;
		int width;

		/* Get us the first line of file, which tells us how long the */
		/* longest line is */
		file_getl(fp, buf, sizeof(buf));
		lw = strtol(buf, &pe, 10);
		width = (pe != buf && lw > 0 && lw < INT_MAX) ? (int)lw : 74;

		/* Dump the file to the screen */
		while (file_getl(fp, buf, sizeof(buf))) {
			put_str(buf, i++, (wid / 2) - (width / 2));
		}

		file_close(fp);
	}

	put_str_centred(i, 0, wid, "All Hail the Mighty Champion!");

	event_signal(EVENT_INPUT_FLUSH);
	pause_line(Term);
}


/**
 * Menu command: dump character dump to file.
 */
static void death_file(const char *title, int row)
{
	char buf[1024];
	char ftmp[80];

	/* Get the filesystem-safe name and append .txt */
	player_safe_name(ftmp, sizeof(ftmp), player->full_name, false);
	my_strcat(ftmp, ".txt", sizeof(ftmp));

	if (get_file(ftmp, buf, sizeof buf)) {
		bool success;

		/* Dump a character file */
		screen_save();
		success = dump_save(buf);
		screen_load();

		/* Check result */
		if (success)
			msg("Character dump successful.");
		else
			msg("Character dump failed!");

		/* Flush messages */
		event_signal(EVENT_MESSAGE_FLUSH);
	}
}

/**
 * Menu command: view character dump and inventory.
 */
static void death_info(const char *title, int row)
{
	struct store *home = store_home(player);

	screen_save();

	/* Display player */
	display_player(0);

	/* Prompt for inventory */
	prt("Hit any key to see more information: ", 0, 0);

	/* Allow abort at this point */
	(void)anykey();


	/* Show equipment and inventory */

	/* Equipment -- if any */
	if (player->upkeep->equip_cnt) {
		Term_clear();
		show_equip(OLIST_WEIGHT | OLIST_SEMPTY | OLIST_DEATH, NULL);
		prt("You are using: -more-", 0, 0);
		(void)anykey();
	}

	/* Inventory -- if any */
	if (player->upkeep->inven_cnt) {
		Term_clear();
		show_inven(OLIST_WEIGHT | OLIST_DEATH, NULL);
		prt("You are carrying: -more-", 0, 0);
		(void)anykey();
	}

	/* Quiver -- if any */
	if (player->upkeep->quiver_cnt) {
		Term_clear();
		show_quiver(OLIST_WEIGHT | OLIST_DEATH, NULL);
		prt("Your quiver holds: -more-", 0, 0);
		(void)anykey();
	}

	/* Home -- if anything there */
	if (home && home->stock) {
		int page;
		struct object *obj = home->stock;

		/* Display contents of the home */
		for (page = 1; obj; page++) {
			int line;

			/* Clear screen */
			Term_clear();

			/* Show 12 items */
			for (line = 0; obj && line < 12; obj = obj->next, line++) {
				uint8_t attr;

				char o_name[80];
				char tmp_val[80];

				/* Print header, clear line */
				strnfmt(tmp_val, sizeof(tmp_val), "%c) ", I2A(line));
				prt(tmp_val, line + 2, 4);

				/* Get the object description */
				object_desc(o_name, sizeof(o_name), obj,
					ODESC_PREFIX | ODESC_FULL, player);

				/* Get the inventory color */
				attr = obj->kind->base->attr;

				/* Display the object */
				c_put_str(attr, o_name, line + 2, 7);
			}

			/* Caption */
			prt(format("Your home contains (page %d): -more-", page), 0, 0);

			/* Wait for it */
			(void)anykey();
		}
	}

	screen_load();
}

/**
 * Menu command: peruse pre-death messages.
 */
static void death_messages(const char *title, int row)
{
	screen_save();
	do_cmd_messages();
	screen_load();
}

/**
 * Menu command: see top twenty scores.
 */
static void death_scores(const char *title, int row)
{
	screen_save();
	show_scores();
	screen_load();
}

/**
 * Menu command: examine items in the inventory.
 */
static void death_examine(const char *title, int row)
{
	struct object *obj;
	const char *q, *s;

	/* Get an item */
	q = "Examine which item? ";
	s = "You have nothing to examine.";

	while (get_item(&obj, q, s, 0, NULL, (USE_INVEN | USE_QUIVER | USE_EQUIP | IS_HARMLESS))) {
		char header[120];

		textblock *tb;
		region area = { 0, 0, 0, 0 };

		tb = object_info(obj, OINFO_NONE);
		object_desc(header, sizeof(header), obj,
			ODESC_PREFIX | ODESC_FULL | ODESC_CAPITAL, player);

		textui_textblock_show(tb, area, header);
		textblock_free(tb);
	}
}


/**
 * Menu command: view character history.
 */
static void death_history(const char *title, int row)
{
	history_display();
}

/**
 * Menu command: allow spoiler generation (mainly for randarts).
 */
static void death_spoilers(const char *title, int row)
{
	do_cmd_spoilers();
}

/***
 * Menu command: start a new game
 */
static void death_new_game(const char *title, int row)
{
    play_again = get_check("Start a new game? ");
}

/**
 * Menu structures for the death menu. Note that Quit must always be the
 * last option, due to a hard-coded check in death_screen
 */
static menu_action death_actions[] =
{
	{ 0, 'i', "Information",   death_info      },
	{ 0, 'm', "Messages",      death_messages  },
	{ 0, 'f', "File dump",     death_file      },
	{ 0, 'v', "View scores",   death_scores    },
	{ 0, 'x', "Examine items", death_examine   },
	{ 0, 'h', "History",       death_history   },
	{ 0, 's', "Spoilers",      death_spoilers  },
	{ 0, 'n', "New Game",      death_new_game  },
	{ 0, 'q', "Quit",          NULL            },
};



/**
 * Handle character death
 */
void death_screen(void)
{
	struct menu *death_menu;
	bool done = false;
	const region area = { 51, 2, 0, N_ELEMENTS(death_actions) };

	/* Dump bones file */
	make_bones();

	/* Winner */
	if (player->total_winner)
	{
		display_winner();
	}

	/* Tombstone/retiring */
	display_exit_screen();

	/* Flush all input and output */
	event_signal(EVENT_INPUT_FLUSH);
	event_signal(EVENT_MESSAGE_FLUSH);

	/* Display and use the death menu */
	death_menu = menu_new_action(death_actions,
			N_ELEMENTS(death_actions));

	death_menu->flags = MN_CASELESS_TAGS;

	menu_layout(death_menu, &area);

	while (!done && !play_again)
	{
		ui_event e = menu_select(death_menu, EVT_KBRD, false);
		if (e.type == EVT_KBRD)
		{
			if (e.key.code == KTRL('X')) break;
			if (e.key.code == KTRL('N')) play_again = true;
		}
		else if (e.type == EVT_SELECT)
		{
			done = get_check("Do you want to quit? ");
		}
	}

	menu_free(death_menu);
}
