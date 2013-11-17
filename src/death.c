/*
 * File: death.c
 * Purpose: Handle the UI bits that happen after the character dies.
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
#include "button.h"
#include "cmds.h"
#include "files.h"
#include "game-event.h"
#include "history.h"
#include "savefile.h"
#include "ui-menu.h"

/**
 * Hack - save the time of death
 */
static time_t death_time = (time_t) 0;


/**
 * Encode the screen colors for the closing screen
 */
static char hack[17] = "dwsorgbuDWvyRGBU";


/**
 * Gets a personalized string for ghosts.  Code originally from get_name. -LM-
 */
static char *get_personalized_string(byte choice)
{
	static char tmp[80], info[80];
	byte n, i;

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

	sprintf(info, "(%d characters maximum.  Entry will be used as", 79);

	prt(info, 17, 0);
	prt("(a) sentence(s).)", 18, 0);

	/* Ask until happy */
	while (1) {
		/* Start at beginning of field. */
		Term_gotoxy(0, 14);

		/* Get an input */
		(void) askfor_aux(tmp, 79, NULL);

		/* All done */
		break;
	}

	/* Pad the string (to clear junk and allow room for a ending) */
	sprintf(tmp, "%-79.79s", tmp);

	/* Ensure that strings end like a sentence, and neatly clip the string. */
	for (n = 79;; n--) {
		if ((tmp[n] == ' ') || (tmp[n] == '\0'))
			continue;
		else {
			if ((tmp[n] == '!') || (tmp[n] == '.') || (tmp[n] == '?')) {
				tmp[n + 1] = '\0';
				for (i = n + 2; i < 80; i++)
					tmp[i] = '\0';
				break;
			} else {
				tmp[n + 1] = '.';
				tmp[n + 2] = '\0';
				for (i = n + 3; i < 80; i++)
					tmp[i] = '\0';
				break;
			}
		}
	}

	/* Start the sentence with a capital letter. */
	if (islower(tmp[0]))
		tmp[0] = toupper(tmp[0]);

	/* Return the string */
	return tmp;

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
	byte choice = 0;

	int i;

	/* Ignore wizards and borgs */
	if (!(p_ptr->noscore & 0x00FF)) {
		/* Ignore people who die in town */
		if (p_ptr->depth) {
			int level;
			char tmp[128];

			/* Slightly more tenacious saving routine. */
			for (i = 0; i < 5; i++) {
				/* Ghost hovers near level of death. */
				if (i == 0)
					level = p_ptr->depth;
				else
					level = p_ptr->depth + 5 - damroll(2, 4);
				if (level < 1)
					level = randint1(4);

				/* XXX XXX XXX "Bones" name */
				sprintf(tmp, "bone.%03d", level);

				/* Build the filename */
				path_build(str, 1024, ANGBAND_DIR_BONE, tmp);

				/* Attempt to open the bones file */
				fp = file_open(str, MODE_READ, FTYPE_TEXT);

				/* Close it right away */
				if (fp)
					file_close(fp);

				/* Do not over-write a previous ghost */
				if (fp)
					continue;

				/* If no file by that name exists, we can make a new one. */
				if (!(fp))
					break;
			}

			/* Failure */
			if (fp)
				return;

			/* Try to write a new "Bones File" */
			fp = file_open(str, MODE_WRITE, FTYPE_TEXT);

			/* Not allowed to write it? Weird. */
			if (!fp)
				return;

			/* Save the info */
			if (op_ptr->full_name[0] != '\0')
				file_putf(fp, "%s\n", op_ptr->full_name);
			else
				file_putf(fp, "Anonymous\n");



			file_putf(fp, "%d\n", p_ptr->psex);
			file_putf(fp, "%d\n", p_ptr->prace);
			file_putf(fp, "%d\n", p_ptr->pclass);

			/* Clear screen */
			Term_clear();

			while (1) {
				/* Ask the player if he wants to add a personalized string. */
				prt("Information about your character has been saved", 15,
					0);
				prt("in a bones file.  Would you like to give the", 16, 0);
				prt("ghost a special message or description? (yes/no)", 17,
					0);
				button_add("Yes", 'y');
				button_add("No", 'n');

				answer = inkey_ex();

				/* Clear last line */
				clear_from(15);
				clear_from(16);

				/* Determine what the personalized string will be used for.  */
				if ((answer.key.code == 'Y') || (answer.key.code == 'y')) {
					prt("Will you add something for your ghost to say,",
						15, 0);
					prt("or add to the monster description?", 16, 0);
					prt("((M)essage/(D)escription)", 17, 0);

					/* Buttons */
					button_kill('y');
					button_kill('n');
					button_add("M", 'M');
					button_add("D", 'D');
					button_add("ESC", ESCAPE);

					while (1) {
						answer = inkey_ex();

						clear_from(15);
						clear_from(16);

						if ((answer.key.code == 'M')
							|| (answer.key.code == 'm')) {
							choice = 1;
							break;
						} else if ((answer.key.code == 'D')
								   || (answer.key.code == 'd')) {
							choice = 2;
							break;
						} else {
							choice = 0;
							break;
						}
					}
				} else if ((answer.key.code == 'N')
						   || (answer.key.code == 'n')
						   || (answer.key.code == ESCAPE)) {
					choice = 0;
					break;
				}

				button_kill_all();

				/* If requested, get the personalized string, and write it and
				 * info on how it should be used in the bones file.  Otherwise,
				 * indicate the absence of such a string. */
				if (choice)
					file_putf(fp, "%d:%s\n", choice,
							  get_personalized_string(choice));
				else
					file_putf(fp, "0: \n");

				/* Close and save the Bones file */
				file_close(fp);

				return;
			}
		}
	}
}


/**
 * Centers a string within a 31 character string
 */
static void center_string(char *buf, const char *str)
{
	int i, j;

	/* Total length */
	i = strlen(str);

	/* Necessary border */
	j = 15 - i / 2;

	/* Mega-Hack */
	sprintf(buf, "%*s%s%*s", j, "", str, 31 - i - j, "");
}

/**
 * Display a "tomb-stone"
 */
static void print_tomb(void)
{
	const char *p;

	int offset = 12;

	char tmp[160];

	char buf[1024];

	ang_file *fp;

#ifdef _WIN32_WCE
	time_t ct = fake_time((time_t) 0);
#else
	time_t ct = time((time_t) 0);
#endif

	bool boat = ((p_ptr->total_winner) && (player_has(PF_ELVEN)));
	bool tree = ((p_ptr->total_winner)
				 && (player_has(PF_WOODEN) || player_has(PF_DIVINE)));

	/* Clear screen */
	Term_clear();

	/* Build the filename */
	if (tree)
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "tree.txt");
	else if (boat)
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "boat.txt");
	else
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "dead.txt");

	/* Open the death file */
	fp = file_open(buf, MODE_READ, -1);

	/* Dump */
	if (fp) {
		int i, y, x;

		int a = 0;
		wchar_t c = L' ';

		bool okay = TRUE;

		int len;


		/* Load the screen */
		for (y = 0; okay; y++) {
			/* Get a line of data */
			if (!file_getl(fp, buf, 1024))
				okay = FALSE;

			/* Stop on blank line */
			if (!buf[0])
				break;

			/* Get the width */
			len = strlen(buf);

			/* XXX Restrict to current screen size */
			if (len >= Term->wid)
				len = Term->wid;

			/* Show each row */
			for (x = 0; x < len; x++) {
				/* Put the attr/char */
				Term_draw(x, y, TERM_WHITE, buf[x]);
			}
		}

		/* Get the blank line */
		/* if (my_fgets(fp, buf, 1024)) okay = FALSE; */


		/* Load the screen */
		for (y = 0; okay; y++) {
			/* Get a line of data */
			if (!file_getl(fp, buf, 1024))
				okay = FALSE;

			/* Stop on blank line */
			if (!buf[0])
				break;

			/* Get the width */
			len = strlen(buf);

			/* XXX Restrict to current screen size */
			if (len >= Term->wid)
				len = Term->wid;

			/* Show each row */
			for (x = 0; x < len; x++) {
				/* Get the attr/char */
				(void) (Term_what(x, y, &a, &c));

				/* Look up the attr */
				for (i = 0; i < 16; i++) {
					/* Use attr matches */
					if (hack[i] == buf[x])
						a = i;
				}

				/* Put the attr/char */
				Term_draw(x, y, a, c);
			}

			/* Place the cursor */
			Term_gotoxy(x, y);

		}


		/* Get the blank line */
		/* if (my_fgets(fp, buf, 1024)) okay = FALSE; */


		/* Close it */
		file_close(fp);
	}

	/* King or Queen */
	if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL)) {
		p = "Magnificent";
	}

	/* Normal */
	else {
		p = cp_ptr->title[(p_ptr->lev - 1) / 5];
	}

	/* Set offset */
	offset = 11;

	center_string(buf, op_ptr->full_name);
	put_str(buf, 6, offset);

	center_string(buf, "the");
	put_str(buf, 7, offset);

	center_string(buf, p);
	put_str(buf, 8, offset);


	center_string(buf, cp_ptr->name);
	put_str(buf, 10, offset);

	sprintf(tmp, "Level: %d", (int) p_ptr->lev);
	center_string(buf, tmp);
	put_str(buf, 11, offset);

	sprintf(tmp, "Exp: %ld", (long) p_ptr->exp);
	center_string(buf, tmp);
	put_str(buf, 12, offset);

	sprintf(tmp, "AU: %ld", (long) p_ptr->au);
	center_string(buf, tmp);
	put_str(buf, 13, offset);

	if (p_ptr->depth)
		sprintf(tmp, "Killed in %s level %d",
				locality_name[stage_map[p_ptr->stage][LOCALITY]],
				p_ptr->depth);
	else if (boat)
		sprintf(tmp, "Sailed victorious to Aman.");
	else if (tree)
		sprintf(tmp, "Retired to Fangorn Forest.");
	else
		sprintf(tmp, "Killed in %s town",
				locality_name[stage_map[p_ptr->stage][LOCALITY]]);
	center_string(buf, tmp);
	put_str(buf, 14, offset);

	if (!(boat || tree)) {
		sprintf(tmp, "by %s.", p_ptr->died_from);
		center_string(buf, tmp);
		put_str(buf, 15, offset);
	}
#ifdef _WIN32_WCE
	{
		char *fake_ctime(const unsigned long *fake_time_t);
		sprintf(tmp, "%-.24s", fake_ctime(&ct));
	}
#else
	sprintf(tmp, "%-.24s", ctime(&ct));
#endif
	center_string(buf, tmp);
	put_str(buf, 17, offset);
}


/**
 * Hack - Know inventory and home items upon death
 */
static void death_knowledge(void)
{
	int i, which = 0;

	object_type *o_ptr;

	store_type *st_ptr = NULL;

	/* Get the store number of the home */
	if ((p_ptr->map == MAP_DUNGEON) || (p_ptr->map == MAP_FANILLA))
		which = NUM_TOWNS_SMALL * 4 + STORE_HOME;
	else {
		for (i = 0; i < NUM_TOWNS; i++) {
			/* Found the town */
			if (p_ptr->home == towns[i]) {
				which += (i < NUM_TOWNS_SMALL ? 3 : STORE_HOME);
				break;
			}
			/* Next town */
			else
				which +=
					(i <
					 NUM_TOWNS_SMALL ? MAX_STORES_SMALL : MAX_STORES_BIG);
		}
	}

	/* Hack -- Know everything in the inven/equip */
	for (i = 0; i < ALL_INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx)
			continue;

		/* Aware and Known */
		object_aware(o_ptr);
		object_known(o_ptr);

		/* Fully known */
		cf_union(o_ptr->id_curse, o_ptr->flags_curse);
	}

	/* Thralls sometimes (often!) don't have a home */
	if (p_ptr->home) {
		/* Activate the store */
		st_ptr = &store[which];


		/* Hack -- Know everything in the home */
		for (i = 0; i < st_ptr->stock_num; i++) {
			o_ptr = &st_ptr->stock[i];

			/* Skip non-objects */
			if (!o_ptr->k_idx)
				continue;

			/* Aware and Known */
			object_aware(o_ptr);
			object_known(o_ptr);

			/* Fully known */
			cf_union(o_ptr->id_curse, o_ptr->flags_curse);
		}
	}

	history_unmask_unknown();

	/* Hack -- Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
}

/*
 * Menu command: dump character dump to file.
 */
static void death_file(const char *title, int row)
{
	do_cmd_change_name();
}



/**
 * Display some character info
 */
static void death_info(const char *title, int row)
{
	int i, j, k, which = 0;

	object_type *o_ptr;

	store_type *st_ptr;

	ui_event ke;

	bool done = FALSE;

	/* Get the store number of the home */
	if (p_ptr->map == MAP_DUNGEON)
		which = NUM_TOWNS_SMALL * 4 + STORE_HOME;
	else {
		for (i = 0; i < NUM_TOWNS; i++) {
			/* Found the town */
			if (p_ptr->home == towns[i]) {
				which += (i < NUM_TOWNS_SMALL ? 3 : STORE_HOME);
				break;
			}
			/* Next town */
			else
				which +=
					(i <
					 NUM_TOWNS_SMALL ? MAX_STORES_SMALL : MAX_STORES_BIG);
		}
	}

	/* Activate the store */
	st_ptr = &store[which];

	screen_save();

	/* Display player */
	display_player(0);

	/* Prompt for inventory */
	prt("Hit any key to see more information (ESC to abort): ", 0, 0);

	/* Buttons */
	button_backup_all();
	button_kill_all();
	button_add("ESC", ESCAPE);
	button_add("Continue", 'q');

	/* Allow abort at this point */
	ke = inkey_ex();
	if (ke.key.code == ESCAPE)
		done = TRUE;

	/* Show equipment and inventory */

	/* Equipment -- if any */
	if ((p_ptr->equip_cnt) && !done) {
		Term_clear();
		item_tester_full = TRUE;
		show_equip(OLIST_WEIGHT);
		prt("You are using: -more-", 0, 0);
		ke = inkey_ex();
		if (ke.key.code == ESCAPE)
			done = TRUE;
	}

	/* Inventory -- if any */
	if ((p_ptr->inven_cnt) && !done) {
		Term_clear();
		item_tester_full = TRUE;
		show_inven(OLIST_WEIGHT);
		prt("You are carrying: -more-", 0, 0);
		ke = inkey_ex();
		if (ke.key.code == ESCAPE)
			done = TRUE;
	}



	/* Home -- if anything there */
	if ((st_ptr->stock_num) && !done) {
		/* Display contents of the home */
		for (k = 0, i = 0; i < st_ptr->stock_num; k++) {
			/* Clear screen */
			Term_clear();

			/* Show 12 items */
			for (j = 0; (j < 12) && (i < st_ptr->stock_num); j++, i++) {
				byte attr;

				char o_name[80];
				char tmp_val[80];

				/* Get the object */
				o_ptr = &st_ptr->stock[i];

				/* Print header, clear line */
				sprintf(tmp_val, "%c) ", I2A(j));
				prt(tmp_val, j + 2, 4);

				/* Get the object description */
				object_desc(o_name, sizeof(o_name), o_ptr,
							ODESC_PREFIX | ODESC_FULL);

				/* Get the inventory color */
				attr = tval_to_attr[o_ptr->tval & 0x7F];

				/* Display the object */
				c_put_str(attr, o_name, j + 2, 7);
			}

			/* Caption */
			prt(format("Your home contains (page %d): -more-", k + 1), 0,
				0);

			/* Wait for it */
			ke = inkey_ex();
			if (ke.key.code == ESCAPE)
				done = TRUE;
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
 * Special version of 'do_cmd_examine'
 */
static void death_examine(const char *title, int row)
{
	int item;
	const char *q, *s;


	/* Get an item */
	q = "Examine which item? ";
	s = "You have nothing to examine.";

	while (get_item(&item, q, s, 0, (USE_INVEN | USE_EQUIP | IS_HARMLESS))) {
		char header[120];

		textblock *tb;
		region area = { 0, 0, 0, 0 };

		object_type *o_ptr = &p_ptr->inventory[item];

		tb = object_info(o_ptr, OINFO_FULL);
		object_desc(header, sizeof(header), o_ptr,
					ODESC_PREFIX | ODESC_FULL | ODESC_CAPITAL);

		textui_textblock_show(tb, area, format("%s", header));
		textblock_free(tb);
	}
}

/**
 * Change the player into a Winner
 */
static void kingly(void)
{
	/* Hack -- retire in town */
	p_ptr->depth = 0;

	/* Fake death */
	strcpy(p_ptr->died_from, "Ripe Old Age");

	/* Restore the experience */
	p_ptr->exp = p_ptr->max_exp;

	/* Restore the level */
	p_ptr->lev = p_ptr->max_lev;

	/* Hack -- Instant Gold */
	p_ptr->au += 10000000L;

	/* Limit to avoid buffer overflow */
	if (p_ptr->au > PY_MAX_GOLD)
		p_ptr->au = PY_MAX_GOLD;

	/* Clear screen */
	Term_clear();

	/* Display a crown */
	put_str("#", 1, 34);
	put_str("#####", 2, 32);
	put_str("#", 3, 34);
	put_str(",,,  $$$  ,,,", 4, 28);
	put_str(",,=$   \"$$$$$\"   $=,,", 5, 24);
	put_str(",$$        $$$        $$,", 6, 22);
	put_str("*>         <*>         <*", 7, 22);
	put_str("$$         $$$         $$", 8, 22);
	put_str("\"$$        $$$        $$\"", 9, 22);
	put_str("\"$$       $$$       $$\"", 10, 23);
	put_str("*#########*#########*", 11, 24);
	put_str("*#########*#########*", 12, 24);

	/* Display a message */
	put_str("Veni, Vidi, Vici!", 15, 26);
	put_str("I came, I saw, I conquered!", 16, 21);
	put_str(format("All Hail the Mighty %s!", sp_ptr->winner), 17, 22);

	/* Flush input */
	flush();

	/* Wait for response */
	pause_line(Term);
}

/*
 * Menu command: view character history.
 */
static void death_history(const char *title, int row)
{
	history_display();
}

/*
 * Menu command: allow spoiler generation (mainly for randarts).
 */
static void death_spoilers(const char *title, int row)
{
	do_cmd_spoilers();
}

/*
 * Menu structures for the death menu. Note that Quit must always be the
 * last option, due to a hard-coded check in death_screen
 */
static menu_type *death_menu;
static menu_action death_actions[] = {
	{0, 'i', "Information", death_info},
	{0, 'm', "Messages", death_messages},
	{0, 'f', "File dump", death_file},
	{0, 'v', "View scores", death_scores},
	{0, 'x', "Examine items", death_examine},
	{0, 'h', "History", death_history},
	{0, 's', "Spoilers", death_spoilers},
	{0, 'q', "Quit", NULL},
};


/**
 * Handle character death
 */
void death_screen(void)
{
	const region area = { 51, 2, 0, N_ELEMENTS(death_actions) };

	/* Dump bones file */
	make_bones();

	/* Handle retirement */
	if (p_ptr->total_winner)
		kingly();

	/* Save dead player */
	if (!savefile_save(savefile)) {
		msg("death save failed!");
		message_flush();
	}

	/* Get time of death */
#ifdef _WIN32_WCE
	{
		unsigned long fake_time(unsigned long *fake_time_t);
		fake_time(&death_time);
	}
#else
	(void) time(&death_time);
#endif

	/* Hack - Know everything upon death */
	death_knowledge();

	event_signal(EVENT_INVENTORY);
	event_signal(EVENT_EQUIPMENT);

	/* Handle stuff */
	notice_stuff(p_ptr);
	handle_stuff(p_ptr);

	/* You are dead */
	print_tomb();

	/* Enter player in high score list */
	enter_score(&death_time);

	/* Flush all input keys */
	flush();

	/* Flush messages */
	message_flush();

	if (!death_menu) {
		death_menu = menu_new_action(death_actions,
									 N_ELEMENTS(death_actions));

		death_menu->flags = MN_CASELESS_TAGS;
	}

	menu_layout(death_menu, &area);

	do {
		menu_select(death_menu, 0, FALSE);
	} while (!get_check("Do you want to quit? "));
}
