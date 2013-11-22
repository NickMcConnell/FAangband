/*
 * File: ui-options.c
 * Purpose: Text UI options handling code (everything accessible from '=')
 *
 * Copyright (c) 1997-2000 Robert A. Koeneke, James E. Wilson, Ben Harrison
 * Copyright (c) 2007 Pete Mack
 * Copyright (c) 2010 Andi Sidwell
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
#include "keymap.h"
#include "squelch.h"
#include "prefs.h"
#include "tvalsval.h"
#include "ui-menu.h"
#include "files.h"


/**
 * Prompt the user for a filename to save the pref file to.
 */
static bool get_pref_path(const char *what, int row, char *buf, size_t max)
{
	char ftmp[80];
	bool ok;

	screen_save();

	/* Prompt */
	prt(format("%s to a pref file", what), row, 0);
	prt("File: ", row + 2, 0);

	/* Default filename */
	strnfmt(ftmp, sizeof ftmp, "%s.prf", player_safe_name(p_ptr, TRUE));

	/* Get a filename */
	ok = askfor_aux(ftmp, sizeof ftmp, NULL);
	screen_load();

	/* Build the filename */
	if (ok)
		path_build(buf, max, ANGBAND_DIR_USER, ftmp);

	return ok;
}


static void dump_pref_file(void (*dump) (ang_file *), const char *title,
						   int row)
{
	char buf[1024];

	/* Get filename from user */
	if (!get_pref_path(title, row, buf, sizeof(buf)))
		return;

	/* Try to save */
	if (prefs_save(buf, dump, title))
		msg("Saved %s.", strstr(title, " ") + 1);
	else
		msg("Failed to save %s.", strstr(title, " ") + 1);

	message_flush();

	return;
}

static void do_cmd_pref_file_hack(long row);






/*** Options display and setting ***/



/*** Boolean option menu code ***/

/**
 * Displays an option entry.
 */
static void option_toggle_display(menu_type * m, int oid, bool cursor,
								  int row, int col, int width)
{
	byte attr = curs_attrs[CURS_KNOWN][cursor != 0];
	bool *options = menu_priv(m);

	c_prt(attr, format("%-45s: %s  (%s)", option_desc(oid),
					   options[oid] ? "yes" : "no ", option_name(oid)),
		  row, col);
}

/**
 * Handle keypresses for an option entry.
 */
static bool option_toggle_handle(menu_type * m, const ui_event * event,
								 int oid)
{
	bool next = FALSE;

	if (event->type == EVT_SELECT) {
		option_set(option_name(oid), !op_ptr->opt[oid]);
	} else if (event->type == EVT_KBRD) {
		if (event->key.code == 'y' || event->key.code == 'Y') {
			option_set(option_name(oid), TRUE);
			next = TRUE;
		} else if (event->key.code == 'n' || event->key.code == 'N') {
			option_set(option_name(oid), FALSE);
			next = TRUE;
		} else if (event->key.code == '?') {
			screen_save();
			show_file(format("optdesc.txt#%s", option_name(oid)), NULL, 0,
					  0);
			screen_load();
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}

	if (next) {
		m->cursor++;
		m->cursor = (m->cursor + m->filter_count) % m->filter_count;
	}

	return TRUE;
}

/** Toggle option menu display and handling functions */
static const menu_iter option_toggle_iter = {
	NULL,
	NULL,
	option_toggle_display,
	option_toggle_handle,
	NULL
};


/**
 * Interact with some options
 */
static void option_toggle_menu(const char *name, int page)
{
	int i;

	menu_type *m = menu_new(MN_SKIN_SCROLL, &option_toggle_iter);

	/* for all menus */
	m->prompt = "Set option (y/n/t), '?' for information";
	m->cmd_keys = "?YyNnTt";
	m->selections = "abcdefghijklmopqrsuvwxz";
	m->flags = MN_DBL_TAP;

	/* for this particular menu */
	m->title = name;

	/* Find the number of valid entries */
	for (i = 0; i < OPT_PAGE_PER; i++) {
		if (option_page[page][i] == OPT_none)
			break;
	}

	/* Set the data to the player's options */
	menu_setpriv(m, OPT_MAX, &op_ptr->opt);
	menu_set_filter(m, option_page[page], i);
	menu_layout(m, &SCREEN_REGION);

	/* Run the menu */
	screen_save();

	clear_from(0);
	menu_select(m, 0, FALSE);

	screen_load();

	mem_free(m);
}


/*
 * Modify the "window" options
 */
static void do_cmd_options_win(const char *name, int row)
{
	int i, j, d;

	int y = 0;
	int x = 0;

	ui_event ke;

	u32b new_flags[ANGBAND_TERM_MAX];

	/* Set new flags to the old values */
	for (j = 0; j < ANGBAND_TERM_MAX; j++) {
		new_flags[j] = op_ptr->window_flag[j];
	}


	/* Clear screen */
	screen_save();
	clear_from(0);

	/* Interact */
	while (1) {
		/* Prompt */
		prt("Window flags (<dir> to move, 't'/Enter to toggle, or ESC)", 0,
			0);

		/* Display the windows */
		for (j = 0; j < ANGBAND_TERM_MAX; j++) {
			byte a = TERM_WHITE;

			const char *s = angband_term_name[j];

			/* Use color */
			if (j == x)
				a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(35 + j * 5 - strlen(s) / 2, 2 + j % 2, -1, a, s);
		}

		/* Display the options */
		for (i = 0; i < PW_MAX_FLAGS; i++) {
			byte a = TERM_WHITE;

			const char *str = window_flag_desc[i];

			/* Use color */
			if (i == y)
				a = TERM_L_BLUE;

			/* Unused option */
			if (!str)
				str = "(Unused option)";

			/* Flag name */
			Term_putstr(0, i + 5, -1, a, str);

			/* Display the windows */
			for (j = 0; j < ANGBAND_TERM_MAX; j++) {
				char c = '.';

				a = TERM_WHITE;

				/* Use color */
				if ((i == y) && (j == x))
					a = TERM_L_BLUE;

				/* Active flag */
				if (new_flags[j] & (1L << i))
					c = 'X';

				/* Flag value */
				Term_putch(35 + j * 5, i + 5, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(35 + x * 5, y + 5);

		/* Get key */
		ke = inkey_ex();

		/* Allow escape */
		if ((ke.key.code == ESCAPE) || (ke.key.code == 'q'))
			break;

		/* Mouse interaction */
		if (ke.type == EVT_MOUSE) {
			int choicey = ke.mouse.y - 5;
			int choicex = (ke.mouse.x - 35) / 5;

			if ((choicey >= 0) && (choicey < PW_MAX_FLAGS)
				&& (choicex > 0) && (choicex < ANGBAND_TERM_MAX)
				&& !(ke.mouse.x % 5)) {
				y = choicey;
				x = (ke.mouse.x - 35) / 5;
			}
		}

		/* Toggle */
		else if ((ke.key.code == '5') || (ke.key.code == 't') ||
				 (ke.key.code == KC_ENTER) || (ke.type == EVT_MOUSE)) {
			/* Hack -- ignore the main window */
			if (x == 0) {
				bell("Cannot set main window flags!");
			}

			/* Toggle flag (off) */
			else if (new_flags[x] & (1L << y)) {
				new_flags[x] &= ~(1L << y);
			}

			/* Toggle flag (on) */
			else {
				new_flags[x] |= (1L << y);
			}

			/* Continue */
			continue;
		}

		/* Extract direction */
		d = target_dir(ke.key);

		/* Move */
		if (d != 0) {
			x = (x + ddx[d] + 8) % ANGBAND_TERM_MAX;
			y = (y + ddy[d] + 16) % PW_MAX_FLAGS;
		}

		/* Oops */
		else {
			bell("Illegal command for window options!");
		}
	}

	/* Notice changes */
	subwindows_set_flags(new_flags, ANGBAND_TERM_MAX);

	screen_load();
}



/*** Interact with keymaps ***/

/*
 * Current (or recent) keymap action
 */
static struct keypress keymap_buffer[KEYMAP_ACTION_MAX];


/*
 * Ask for, and display, a keymap trigger.
 *
 * Returns the trigger input.
 *
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
 */
static struct keypress keymap_get_trigger(void)
{
	char tmp[80];
	struct keypress buf[2] = { {0}, {0} };

	/* Flush */
	flush();

	/* Get a key */
	buf[0] = inkey();

	/* Convert to ascii */
	keypress_to_text(tmp, sizeof(tmp), buf, FALSE);

	/* Hack -- display the trigger */
	Term_addstr(-1, TERM_WHITE, tmp);

	/* Flush */
	flush();

	/* Return trigger */
	return buf[0];
}


/*
 * Macro menu action functions
 */

static void ui_keymap_pref_load(const char *title, int row)
{
	do_cmd_pref_file_hack(16);
}

static void ui_keymap_pref_append(const char *title, int row)
{
	(void) dump_pref_file(keymap_dump, "Dump keymaps", 13);
}

static void ui_keymap_query(const char *title, int row)
{
	char tmp[1024];
	int mode =
		OPT(rogue_like_commands) ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG;
	struct keypress c;
	const struct keypress *act;

	prt(title, 13, 0);
	prt("Key: ", 14, 0);

	/* Get a keymap trigger & mapping */
	c = keymap_get_trigger();
	act = keymap_find(mode, c);

	/* Nothing found */
	if (!act) {
		/* Prompt */
		prt("No keymap with that trigger.  Press any key to continue.", 16,
			0);
		inkey();
	}

	/* Found one */
	else {
		/* Analyze the current action */
		keypress_to_text(tmp, sizeof(tmp), act, FALSE);

		/* Display the current action */
		prt("Found: ", 15, 0);
		Term_addstr(-1, TERM_WHITE, tmp);

		prt("Press any key to continue.", 17, 0);
		inkey();
	}
}

static void ui_keymap_create(const char *title, int row)
{
	bool done = FALSE;
	size_t n = 0;

	struct keypress c;
	char tmp[1024];
	int mode =
		OPT(rogue_like_commands) ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG;

	prt(title, 13, 0);
	prt("Key: ", 14, 0);

	c = keymap_get_trigger();
	if (c.code == '$') {
		c_prt(TERM_L_RED, "The '$' key is reserved.", 16, 2);
		prt("Press any key to continue.", 18, 0);
		inkey();
		return;
	}

	/* Get an encoded action, with a default response */
	while (!done) {
		struct keypress kp = { EVT_NONE, 0, 0 };

		int color = TERM_WHITE;
		if (n == 0)
			color = TERM_YELLOW;
		if (n == KEYMAP_ACTION_MAX)
			color = TERM_L_RED;

		keypress_to_text(tmp, sizeof(tmp), keymap_buffer, FALSE);
		c_prt(color, format("Action: %s", tmp), 15, 0);

		c_prt(TERM_L_BLUE, "  Press '$' when finished.", 17, 0);
		c_prt(TERM_L_BLUE, "  Use 'CTRL-U' to reset.", 18, 0);
		c_prt(TERM_L_BLUE,
			  format("(Maximum keymap length is %d keys.)",
					 KEYMAP_ACTION_MAX), 19, 0);

		kp = inkey();

		if (kp.code == '$') {
			done = TRUE;
			continue;
		}

		switch (kp.code) {
		case KC_DELETE:
		case KC_BACKSPACE:{
				if (n > 0) {
					n -= 1;
					keymap_buffer[n].type = 0;
					keymap_buffer[n].code = 0;
					keymap_buffer[n].mods = 0;
				}
				break;
			}

		case KTRL('U'):{
				memset(keymap_buffer, 0, sizeof keymap_buffer);
				n = 0;
				break;
			}

		default:{
				if (n == KEYMAP_ACTION_MAX)
					continue;

				if (n == 0) {
					memset(keymap_buffer, 0, sizeof keymap_buffer);
				}
				keymap_buffer[n++] = kp;
				break;
			}
		}
	}

	if (c.code && get_check("Save this keymap? ")) {
		keymap_add(mode, c, keymap_buffer, TRUE);
		prt("Keymap added.  Press any key to continue.", 17, 0);
		inkey();
	}
}

static void ui_keymap_remove(const char *title, int row)
{
	struct keypress c;
	int mode =
		OPT(rogue_like_commands) ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG;

	prt(title, 13, 0);
	prt("Key: ", 14, 0);

	c = keymap_get_trigger();

	if (keymap_remove(mode, c))
		prt("Removed.", 16, 0);
	else
		prt("No keymap to remove!", 16, 0);

	/* Prompt */
	prt("Press any key to continue.", 17, 0);
	inkey();
}

static void keymap_browse_hook(int oid, void *db, const region * loc)
{
	char tmp[1024];

	message_flush();

	clear_from(13);

	/* Show current action */
	prt("Current action (if any) shown below:", 13, 0);
	keypress_to_text(tmp, sizeof(tmp), keymap_buffer, FALSE);
	prt(tmp, 14, 0);
}

static menu_type *keymap_menu;
static menu_action keymap_actions[] = {
	{0, 0, "Load a user pref file", ui_keymap_pref_load},
	{0, 0, "Append keymaps to a file", ui_keymap_pref_append},
	{0, 0, "Query a keymap", ui_keymap_query},
	{0, 0, "Create a keymap", ui_keymap_create},
	{0, 0, "Remove a keymap", ui_keymap_remove},
};

static void do_cmd_keymaps(const char *title, int row)
{
	region loc = { 0, 0, 0, 12 };

	screen_save();
	clear_from(0);

	if (!keymap_menu) {
		keymap_menu = menu_new_action(keymap_actions,
									  N_ELEMENTS(keymap_actions));

		keymap_menu->title = title;
		keymap_menu->selections = lower_case;
		keymap_menu->browse_hook = keymap_browse_hook;
	}

	menu_layout(keymap_menu, &loc);
	menu_select(keymap_menu, 0, FALSE);

	screen_load();
}




/*** Interact with visuals ***/

static void visuals_pref_load(const char *title, int row)
{
	do_cmd_pref_file_hack(15);
}

#ifdef ALLOW_VISUALS

static void visuals_dump_monsters(const char *title, int row)
{
	dump_pref_file(dump_monsters, title, 15);
}

static void visuals_dump_objects(const char *title, int row)
{
	dump_pref_file(dump_objects, title, 15);
}

static void visuals_dump_features(const char *title, int row)
{
	dump_pref_file(dump_features, title, 15);
}

static void visuals_dump_flavors(const char *title, int row)
{
	dump_pref_file(dump_flavors, title, 15);
}

#endif							/* ALLOW_VISUALS */

static void visuals_reset(const char *title, int row)
{
	/* Reset */
	reset_visuals(TRUE);

	/* Message */
	prt("", 0, 0);
	msg("Visual attr/char tables reset.");
	message_flush();
}


static menu_type *visual_menu;
static menu_action visual_menu_items[] = {
	{0, 0, "Load a user pref file", visuals_pref_load},
#ifdef ALLOW_VISUALS
	{0, 0, "Dump monster attr/chars", visuals_dump_monsters},
	{0, 0, "Dump object attr/chars", visuals_dump_objects},
	{0, 0, "Dump feature attr/chars", visuals_dump_features},
	{0, 0, "Dump flavor attr/chars", visuals_dump_flavors},
#endif							/* ALLOW_VISUALS */
	{0, 0, "Reset visuals", visuals_reset},
};


static void visuals_browse_hook(int oid, void *db, const region * loc)
{
	message_flush();
	clear_from(1);
}


/*
 * Interact with "visuals"
 */
static void do_cmd_visuals(const char *title, int row)
{
	screen_save();
	clear_from(0);

	if (!visual_menu) {
		visual_menu = menu_new_action(visual_menu_items,
									  N_ELEMENTS(visual_menu_items));

		visual_menu->title = title;
		visual_menu->selections = lower_case;
		visual_menu->browse_hook = visuals_browse_hook;
		visual_menu->header = "To edit visuals, use the knowledge menu";
	}

	menu_layout(visual_menu, &SCREEN_REGION);
	menu_select(visual_menu, 0, FALSE);

	screen_load();
}


/*** Interact with colours ***/

#ifdef ALLOW_COLORS

static void colors_pref_load(const char *title, int row)
{
	/* Ask for and load a user pref file */
	do_cmd_pref_file_hack(8);

	/* XXX should probably be a cleaner way to tell UI about
	 * colour changes - how about doing this in the pref file
	 * loading code too? */
	Term_xtra(TERM_XTRA_REACT, 0);
	Term_redraw();
}

static void colors_pref_dump(const char *title, int row)
{
	dump_pref_file(dump_colors, title, 15);
}

static void colors_modify(const char *title, int row)
{
	int i;
	struct keypress cx;

	static byte a = 0;

	/* Prompt */
	prt("Command: Modify colors", 8, 0);

	/* Hack -- query until done */
	while (1) {
		const char *name;
		char index;

		/* Clear */
		clear_from(10);

		/* Exhibit the normal colors */
		for (i = 0; i < BASIC_COLORS; i++) {
			/* Exhibit this color */
			Term_putstr(i * 3, 20, -1, a, "##");

			/* Exhibit character letter */
			Term_putstr(i * 3, 21, -1, (byte) i,
						format(" %c", color_table[i].index_char));

			/* Exhibit all colors */
			Term_putstr(i * 3, 22, -1, (byte) i, format("%2d", i));
		}

		/* Describe the color */
		name = ((a < BASIC_COLORS) ? color_table[a].name : "undefined");
		index = ((a < BASIC_COLORS) ? color_table[a].index_char : '?');

		/* Describe the color */
		Term_putstr(5, 10, -1, TERM_WHITE,
					format("Color = %d, Name = %s, Index = %c", a, name,
						   index));

		/* Label the Current values */
		Term_putstr(5, 12, -1, TERM_WHITE,
					format("K = 0x%02x / R,G,B = 0x%02x,0x%02x,0x%02x",
						   angband_color_table[a][0],
						   angband_color_table[a][1],
						   angband_color_table[a][2],
						   angband_color_table[a][3]));

		/* Prompt */
		Term_putstr(0, 14, -1, TERM_WHITE,
					"Command (n/N/k/K/r/R/g/G/b/B): ");

		/* Get a command */
		cx = inkey();

		/* All done */
		if (cx.code == ESCAPE)
			break;

		/* Analyze */
		if (cx.code == 'n')
			a = (byte) (a + 1);
		if (cx.code == 'N')
			a = (byte) (a - 1);
		if (cx.code == 'k')
			angband_color_table[a][0] =
				(byte) (angband_color_table[a][0] + 1);
		if (cx.code == 'K')
			angband_color_table[a][0] =
				(byte) (angband_color_table[a][0] - 1);
		if (cx.code == 'r')
			angband_color_table[a][1] =
				(byte) (angband_color_table[a][1] + 1);
		if (cx.code == 'R')
			angband_color_table[a][1] =
				(byte) (angband_color_table[a][1] - 1);
		if (cx.code == 'g')
			angband_color_table[a][2] =
				(byte) (angband_color_table[a][2] + 1);
		if (cx.code == 'G')
			angband_color_table[a][2] =
				(byte) (angband_color_table[a][2] - 1);
		if (cx.code == 'b')
			angband_color_table[a][3] =
				(byte) (angband_color_table[a][3] + 1);
		if (cx.code == 'B')
			angband_color_table[a][3] =
				(byte) (angband_color_table[a][3] - 1);

		/* Hack -- react to changes */
		Term_xtra(TERM_XTRA_REACT, 0);

		/* Hack -- redraw */
		Term_redraw();
	}
}

static void colors_browse_hook(int oid, void *db, const region * loc)
{
	message_flush();
	clear_from(1);
}


static menu_type *color_menu;
static menu_action color_events[] = {
	{0, 0, "Load a user pref file", colors_pref_load},
	{0, 0, "Dump colors", colors_pref_dump},
	{0, 0, "Modify colors", colors_modify}
};

/*
 * Interact with "colors"
 */
void do_cmd_colors(const char *title, int row)
{
	screen_save();
	clear_from(0);

	if (!color_menu) {
		color_menu = menu_new_action(color_events,
									 N_ELEMENTS(color_events));

		color_menu->title = title;
		color_menu->selections = lower_case;
		color_menu->browse_hook = colors_browse_hook;
	}

	menu_layout(color_menu, &SCREEN_REGION);
	menu_select(color_menu, 0, FALSE);

	screen_load();
}

#endif


/*** Non-complex menu actions ***/

static bool askfor_aux_numbers(char *buf, size_t buflen, size_t * curs,
							   size_t * len, struct keypress keypress,
							   bool firsttime)
{
	switch (keypress.code) {
	case ESCAPE:
	case KC_ENTER:
	case '\r':
	case ARROW_LEFT:
	case ARROW_RIGHT:
	case KC_DELETE:
	case KC_BACKSPACE:
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return askfor_aux_keypress(buf, buflen, curs, len, keypress,
								   firsttime);
	}

	return FALSE;
}


/*
 * Set base delay factor
 */
static void do_cmd_delay(const char *name, int row)
{
	char tmp[4] = "";
	int msec = op_ptr->delay_factor * op_ptr->delay_factor;

	strnfmt(tmp, sizeof(tmp), "%i", op_ptr->delay_factor);

	screen_save();

	/* Prompt */
	prt("Command: Base Delay Factor", 20, 0);

	prt(format("Current base delay factor: %d (%d msec)",
			   op_ptr->delay_factor, msec), 22, 0);
	prt("New base delay factor (0-255): ", 21, 0);

	/* Ask for a numeric value */
	if (askfor_aux(tmp, sizeof(tmp), askfor_aux_numbers)) {
		u16b val = (u16b) strtoul(tmp, NULL, 0);
		op_ptr->delay_factor = MIN(val, 255);
	}

	screen_load();
}


/*
 * Set hitpoint warning level
 */
static void do_cmd_hp_warn(const char *name, int row)
{
	bool res;
	char tmp[4] = "";
	u16b warn;

	strnfmt(tmp, sizeof(tmp), "%i", op_ptr->hitpoint_warn);

	screen_save();

	/* Prompt */
	prt("Command: Hitpoint Warning", 20, 0);

	prt(format("Current hitpoint warning: %d (%d%%)",
			   op_ptr->hitpoint_warn, op_ptr->hitpoint_warn * 10), 22, 0);
	prt("New hitpoint warning (0-9): ", 21, 0);

	/* Ask the user for a string */
	res = askfor_aux(tmp, sizeof(tmp), askfor_aux_numbers);

	/* Process input */
	if (res) {
		warn = (u16b) strtoul(tmp, NULL, 0);

		/* Reset nonsensical warnings */
		if (warn > 9)
			warn = 0;

		op_ptr->hitpoint_warn = warn;
	}

	screen_load();
}


/** Hack -- hitpoint warning factor */
void do_cmd_panel_change(const char *name, int row)
{
	/* Prompt */
	prt("Command: Panel Change", 20, 0);
	button_add("+", '+');
	button_add("-", '-');

	/* Get a new value */
	while (1) {
		int pdist = (op_ptr->panel_change + 1) * 2;
		ui_event ke;
		prt(format("Current panel change: %d (%d / %d)",
				   op_ptr->panel_change, pdist, pdist * 2), 22, 0);
		prt("New panel change (0-4, +, - or ESC to accept): ", 21, 0);

		ke = inkey_ex();
		if (ke.key.code == ESCAPE)
			break;
		if (isdigit(ke.key.code))
			op_ptr->panel_change = D2I(ke.key.code);
		if (ke.key.code == '+')
			op_ptr->panel_change++;
		if (ke.key.code == '-')
			op_ptr->panel_change--;
		if (op_ptr->panel_change > 4)
			op_ptr->panel_change = 4;
		if (op_ptr->panel_change < 0)
			op_ptr->panel_change = 0;
	}

	button_kill('+');
	button_kill('-');
}



/**
 * Set "lazy-movement" delay
 */
static void do_cmd_lazymove_delay(const char *name, int row)
{
	bool res;
	char tmp[4] = "";

	strnfmt(tmp, sizeof(tmp), "%i", lazymove_delay);

	screen_save();

	/* Prompt */
	prt("Command: Movement Delay Factor", 20, 0);

	prt(format("Current movement delay: %d (%d msec)",
			   lazymove_delay, lazymove_delay * 10), 22, 0);
	prt("New movement delay: ", 21, 0);

	/* Ask the user for a string */
	res = askfor_aux(tmp, sizeof(tmp), askfor_aux_numbers);

	/* Process input */
	if (res) {
		lazymove_delay = (u16b) strtoul(tmp, NULL, 0);
	}

	screen_load();
}



/*
 * Ask for a "user pref file" and process it.
 *
 * This function should only be used by standard interaction commands,
 * in which a standard "Command:" prompt is present on the given row.
 *
 * Allow absolute file names?  XXX XXX XXX
 */
static void do_cmd_pref_file_hack(long row)
{
	char ftmp[80];

	screen_save();

	/* Prompt */
	prt("Command: Load a user pref file", row, 0);

	/* Prompt */
	prt("File: ", row + 2, 0);

	/* Default filename */
	strnfmt(ftmp, sizeof ftmp, "%s.prf", player_safe_name(p_ptr, TRUE));

	/* Ask for a file (or cancel) */
	if (askfor_aux(ftmp, sizeof ftmp, NULL)) {
		/* Process the given filename */
		if (process_pref_file(ftmp, FALSE, TRUE) == FALSE) {
			/* Mention failure */
			prt("", 0, 0);
			msg("Failed to load '%s'!", ftmp);
		} else {
			/* Mention success */
			prt("", 0, 0);
			msg("Loaded '%s'.", ftmp);
		}
	}

	screen_load();
}


/*
 * Write options to a file.
 */
static void do_dump_options(const char *title, int row)
{
	dump_pref_file(option_dump, "Dump options", 20);
}

/*
 * Load a pref file.
 */
static void options_load_pref_file(const char *n, int row)
{
	do_cmd_pref_file_hack(20);
}





/**
 * Autosave options -- textual names
 */
static const char *autosave_text[1] = {
	"autosave"
};

/**
 * Autosave options -- descriptions
 */
static const char *autosave_desc[1] = {
	"Timed autosave"
};

s16b toggle_frequency(s16b current)
{
	if (current == 0)
		return 50;
	if (current == 50)
		return 100;
	if (current == 100)
		return 250;
	if (current == 250)
		return 500;
	if (current == 500)
		return 1000;
	if (current == 1000)
		return 2500;
	if (current == 2500)
		return 5000;
	if (current == 5000)
		return 10000;
	if (current == 10000)
		return 25000;

	else
		return 0;
}


/**
 * Interact with autosave options.  From Zangband.
 */
static void do_cmd_options_autosave(const char *name, int row)
{
	ui_event ke;

	int i, k = 0, n = 1;

	char buf[80];


	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE) {
		/* Prompt - return taken out as there's only one option... -NRM- */
		sprintf(buf,
				"Autosave options (y/n to set, 'F' for frequency, ESC to accept) ");
		prt(buf, 0, 0);

		/* Display the options */
		for (i = 0; i < n; i++) {
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k)
				a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s  (%s)", autosave_desc[i],
					autosave ? "yes" : "no ", autosave_text[i]);
			c_prt(a, buf, i + 2, 0);

			prt(format("Timed autosave frequency: every %d turns",
					   autosave_freq), 5, 0);
		}


		/* Hilight current option */
		Term_gotoxy(50, k + 2);

		button_add("F", 'F');
		button_add("n", 'n');
		button_add("y", 'y');

		/* Get a key */
		ke = inkey_ex();

		/* Analyze */
		switch (ke.key.code) {
		case ESCAPE:
			{
				button_kill('F');
				button_kill('n');
				button_kill('y');
				return;
			}

		case '-':
		case '8':
			{
				k = (n + k - 1) % n;
				break;
			}

		case ' ':
		case '\n':
		case '\r':
		case '2':
			{
				k = (k + 1) % n;
				break;
			}

		case 'y':
		case 'Y':
		case '6':
			{

				autosave = TRUE;
				k = (k + 1) % n;
				break;
			}

		case 'n':
		case 'N':
		case '4':
			{
				autosave = FALSE;
				k = (k + 1) % n;
				break;
			}

		case 'f':
		case 'F':
			{
				autosave_freq = toggle_frequency(autosave_freq);
				prt(format("Timed autosave frequency: every %d turns",
						   autosave_freq), 5, 0);
				break;
			}

		default:
			{
				bell("Illegal command for Autosave options!");
				break;
			}
		}
	}
}

/*** Ego item squelch menu ***/
#define EGO_MENU_HELPTEXT \
"{light green}Movement keys{/} scroll the list\n{light red}ESC{/} returns to the previous menu\n{light blue}Enter{/} toggles the current setting."

/**
 * Skip common prefixes in ego-item names.
 */
static const char *strip_ego_name(const char *name)
{
	if (prefix(name, "of the "))
		return name + 7;
	if (prefix(name, "of "))
		return name + 3;
	return name;
}

/**
 * Utility function used to find/sort tval names.
 */
static int tval_comp_func(const void *a_ptr, const void *b_ptr)
{
	int a = ((tval_desc *) a_ptr)->tval;
	int b = ((tval_desc *) b_ptr)->tval;
	return a - b;
}

/**
 * Display an ego-item type on the screen.
 */
int ego_item_name(char *buf, size_t buf_size, ego_desc * d_ptr)
{
	byte tval_table[EGO_TVALS_MAX], i, n = 0;
	int end;
	size_t prefix_size;
	const char *long_name;

	ego_item_type *e_ptr = &e_info[d_ptr->e_idx];

	/* Copy the valid tvals of this ego-item type */
	for (i = 0; i < EGO_TVALS_MAX; i++) {
		int j;
		bool dub = FALSE;

		/* Ignore "empty" entries */
		if (e_ptr->tval[i] < 1)
			continue;

		/* check for doubles */
		for (j = 0; j < n; j++)
			if (e_ptr->tval[i] == tval_table[j])
				dub = TRUE;
		if (dub)
			continue;

		/* Append valid tvals */
		tval_table[n++] = e_ptr->tval[i];
	}

	/* Sort the tvals using bubble sort  */
	for (i = 0; i < n; i++) {
		int j;

		for (j = i + 1; j < n; j++) {
			if (tval_table[i] > tval_table[j]) {
				byte temp = tval_table[i];
				tval_table[i] = tval_table[j];
				tval_table[j] = temp;
			}
		}
	}

	/* Initialize the buffer */
	end = my_strcat(buf, "[ ] ", buf_size);

	/* Concatenate the tval' names */
	for (i = 0; i < n; i++) {
		const char *tval_name;
		int j;

		for (j = 0; j < Q_TV_MAX; j++)
			if (quality_choices[j].tval == tval_table[i])
				break;

		tval_name = j < Q_TV_MAX ? quality_choices[j].desc : "????";

		/* Append the proper separator first, if any */
		if (i > 0) {
			end += my_strcat(buf, (i < n - 1) ? ", " : " and ", buf_size);
		}

		/* Append the name */
		end += my_strcat(buf, tval_name, buf_size);
	}

	/* Append an extra space */
	end += my_strcat(buf, " ", buf_size);

	/* Get the full ego-item name */
	long_name = e_ptr->name;

	/* Get the length of the common prefix, if any */
	prefix_size = (d_ptr->short_name - long_name);

	/* Found a prefix? */
	if (prefix_size > 0) {
		char prefix[100];

		/* Get a copy of the prefix */
		my_strcpy(prefix, long_name, prefix_size + 1);

		/* Append the prefix */
		end += my_strcat(buf, prefix, buf_size);
	}
	/* Set the name to the right length */
	return end;
}

/**
 * Utility function used for sorting an array of ego-item indices by
 * ego-item name.
 */
static int ego_comp_func(const void *a_ptr, const void *b_ptr)
{
	const ego_desc *a = a_ptr;
	const ego_desc *b = b_ptr;

	/* Note the removal of common prefixes */
	return (strcmp(a->short_name, b->short_name));
}

/**
 * Display an entry on the sval menu
 */
static void ego_display(menu_type * menu, int oid, bool cursor, int row,
						int col, int width)
{
	char buf[80] = "";
	ego_desc *choice = (ego_desc *) menu->menu_data;
	ego_item_type *e_ptr = &e_info[choice[oid].e_idx];

	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
	byte sq_attr = (e_ptr->squelch ? TERM_L_RED : TERM_L_GREEN);

	/* Acquire the "name" of object "i" */
	(void) ego_item_name(buf, sizeof(buf), &choice[oid]);

	/* Print it */
	c_put_str(attr, format("%s", buf), row, col);

	/* Show squelch mark, if any */
	if (e_ptr->squelch)
		c_put_str(TERM_L_RED, "*", row, col + 1);

	/* Show the stripped ego-item name using another colour */
	c_put_str(sq_attr, choice[oid].short_name, row, col + strlen(buf));
}

/**
 * Deal with events on the sval menu
 */
static bool ego_action(menu_type * menu, const ui_event * event, int oid)
{
	ego_desc *choice = menu->menu_data;

	/* Toggle */
	if (event->type == EVT_SELECT) {
		ego_item_type *e_ptr = &e_info[choice[oid].e_idx];
		e_ptr->squelch = !e_ptr->squelch;

		return TRUE;
	}

	return FALSE;
}

/**
 * Display list of ego items to be squelched.
 */
static void ego_menu(const char *unused, int also_unused)
{
	int idx, max_num = 0;
	ego_item_type *e_ptr;
	ego_desc *choice;

	menu_type menu;
	menu_iter menu_f = { 0, 0, ego_display, ego_action, 0 };
	region area = { 1, 5, -1, -1 };
	int cursor = 0;

	int i;

	/* Hack - Used to sort the tval table for the first time */
	static bool sort_tvals = TRUE;

	/* Sort the tval table if needed */
	if (sort_tvals) {
		sort_tvals = FALSE;

		qsort(quality_choices, Q_TV_MAX, sizeof(quality_choices[0]),
			  tval_comp_func);
	}

	/* Create the array */
	choice = C_ZNEW(alloc_ego_size, ego_desc);

	/* Get the valid ego-items */
	for (i = 0; i < alloc_ego_size; i++) {
		idx = alloc_ego_table[i].index;

		e_ptr = &e_info[idx];

		/* Only valid known ego-items allowed */
		if (!e_ptr->name || !e_ptr->everseen)
			continue;

		/* Append the index */
		choice[max_num].e_idx = idx;
		choice[max_num].short_name = strip_ego_name(e_ptr->name);

		++max_num;
	}

	/* Quickly sort the array by ego-item name */
	qsort(choice, max_num, sizeof(choice[0]), ego_comp_func);

	/* Return here if there are no objects */
	if (!max_num) {
		FREE(choice);
		return;
	}


	/* Save the screen and clear it */
	screen_save();
	clear_from(0);

	/* Help text */
	prt("Ego item squelch menu", 0, 0);

	/* Buttons */
	button_add("Up", ARROW_UP);
	button_add("Down", ARROW_DOWN);
	button_add("Toggle", '\r');

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = 1;
	text_out_wrap = 79;
	Term_gotoxy(1, 1);

	/* Display some helpful information */
	text_out_e(EGO_MENU_HELPTEXT);

	text_out_indent = 0;

	/* Set up the menu */
	WIPE(&menu, menu);
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu_setpriv(&menu, max_num, choice);
	menu_layout(&menu, &area);

	/* Select an entry */
	(void) menu_select(&menu, cursor, FALSE);

	/* Free memory */
	FREE(choice);

	/* Load screen */
	screen_load();

	/* Buttons */
	button_kill(ARROW_UP);
	button_kill(ARROW_DOWN);
	button_kill('\r');

	return;
}

/*** Quality-squelch menu ***/

#define QUALITY_MENU_HELPTEXT \
"{light green}Movement keys{/} scroll the list\n{light red}ESC{/} returns to the previous menu\n{light blue}Enter{/} selects a type of item or toggles the current setting."
int current_tval;

/**
 * Display an entry in the menu.
 */
static void quality_display(menu_type * menu, int oid, bool cursor,
							int row, int col, int width)
{
	const char *name = quality_choices[oid].desc;
	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);

	c_put_str(attr, format("%-20s", name), row, col);
}


/**
 * Display the quality squelch subtypes.
 */
static void quality_subdisplay(menu_type * menu, int oid, bool cursor,
							   int row, int col, int width)
{
	bool heavy = (player_has(PF_PSEUDO_ID_HEAVY));
	const char *name = heavy ? quality_strings[oid + 1].heavy_desc :
		quality_strings[oid + 1].light_desc;
	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);

	/* Print it */
	c_put_str(attr, format("[ ] %s", name), row, col);
	if (squelch_profile[current_tval][oid + 1])
		c_put_str(TERM_L_RED, "*", row, col + 1);
}

/**
 * Handle "Enter".  :(
  */
static bool quality_subaction(menu_type * menu, const ui_event * e,
							  int oid)
{
	if (e->type == EVT_SELECT)
		squelch_profile[current_tval][oid + 1] =
			!squelch_profile[current_tval][oid + 1];

	return TRUE;
}


/**
 * Handle keypresses.
 */
static bool quality_action(menu_type * menu1, const ui_event * ev, int oid)
{
	menu_type menu;
	menu_iter menu_f = { 0, 0, quality_subdisplay, quality_subaction, 0 };
	region area = { 24, 5, 44, SQUELCH_MAX };
	ui_event evt = EVENT_EMPTY;
	int count;
	bool heavy = (player_has(PF_PSEUDO_ID_HEAVY));

	/* Display at the right point */
	area.row += oid;

	/* Save */
	screen_save();

	current_tval = oid;

	/* Run menu */
	count = heavy ? SQUELCH_MAX - 3 : SQUELCH_MAX - 1;

	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu_setpriv(&menu, count, quality_strings + 1);

	/* Stop menus from going off the bottom of the screen */
	if (area.row + menu.count > Term->hgt - 1)
		area.row += Term->hgt - 1 - area.row - menu.count;

	menu_layout(&menu, &area);

	window_make(area.col - 2, area.row - 1, area.col + area.width + 2,
				area.row + area.page_rows);

	while (evt.type != EVT_ESCAPE) {
		evt = menu_select(&menu, 0, TRUE);
	}

	/* Load and finish */
	screen_load();
	return TRUE;
}

/**
 * Display quality squelch menu.
 */
static void quality_menu(const char *unused, int also_unused)
{
	menu_type menu;
	menu_iter menu_f = { 0, 0, quality_display, quality_action, 0 };
	region area = { 3, 5, -1, -1 };

	/* Save screen */
	screen_save();
	clear_from(0);

	/* Help text */
	prt("Quality squelch menu", 0, 0);

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = 1;
	text_out_wrap = 70;
	Term_gotoxy(1, 1);
	text_out_e(QUALITY_MENU_HELPTEXT);

	/* Reset text_out() indentation */
	text_out_indent = 0;
	text_out_wrap = 0;

	/* Set up the menu */
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu_setpriv(&menu, Q_TV_MAX, quality_choices);
	menu_layout(&menu, &area);

	/* Select an entry */
	menu_select(&menu, 0, FALSE);

	/* Load screen */
	screen_load();
	return;
}



/*** Sval-dependent menu ***/

/**
 * Display an entry on the sval menu
 */
static void sval_display(menu_type * menu, int oid, bool cursor, int row,
						 int col, int width)
{
	char buf[80];
	const u16b *choice = menu->menu_data;
	int idx = choice[oid];

	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);


	/* Acquire the "name" of object "i" */
	object_kind_name(buf, sizeof(buf), idx, TRUE);

	/* Print it */
	c_put_str(attr, format("[ ] %s", buf), row, col);
	if (k_info[idx].squelch)
		c_put_str(TERM_L_RED, "*", row, col + 1);
}

/**
 * Deal with events on the sval menu
 */
static bool sval_action(menu_type * menu, const ui_event * e, int oid)
{
	u16b *choice = menu->menu_data;

	/* Toggle */
	if (e->type == EVT_SELECT) {
		int idx = choice[oid];
		k_info[idx].squelch = !k_info[idx].squelch;

		return TRUE;
	}

	return FALSE;
}

/**
 * Hack - special artifacts are not squelchable 
 */
#define LAST_NORMAL 730

/**
 * Display list of svals to be squelched.
 */
static bool sval_menu(int tval, const char *desc)
{
	menu_type *menu;
	menu_iter menu_f = { 0, 0, sval_display, sval_action, 0 };
	region area = { 1, 5, -1, -1 };

	int num = 0;
	size_t i;

	u16b *choice;


	/* Create the array */
	choice = C_ZNEW(LAST_NORMAL, u16b);

	/* Iterate over all possible object kinds, finding ones which 
	 * can be squelched */
	for (i = 1; i < LAST_NORMAL; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Skip empty objects, unseen objects, incorrect tvals and artifacts */
		if (!k_ptr->name)
			continue;
		if (!k_ptr->everseen)
			continue;
		if (k_ptr->tval != tval)
			continue;
		if (kf_has(k_ptr->flags_kind, KF_INSTA_ART))
			continue;

		/* Add this item to our possibles list */
		choice[num++] = i;
	}

	/* Return here if there are no objects */
	if (!num) {
		FREE(choice);
		return FALSE;
	}


	/* Save the screen and clear it */
	screen_save();
	clear_from(0);

	/* Help text */
	prt("Item type squelch menu", 0, 0);

	/* Buttons */
	button_add("Up", ARROW_UP);
	button_add("Down", ARROW_DOWN);
	button_add("Toggle", '\r');

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = 1;
	text_out_wrap = 79;
	Term_gotoxy(1, 1);

	/* Display some helpful information */
	text_out_e(EGO_MENU_HELPTEXT);

	text_out_indent = 0;

	/* Run menu */
	menu = menu_new(MN_SKIN_COLUMNS, &menu_f);
	menu_setpriv(menu, num, choice);
	menu_layout(menu, &area);
	menu_select(menu, 0, FALSE);

	/* Free memory */
	FREE(choice);

	/* Load screen */
	screen_load();

	/* Buttons */
	button_kill(ARROW_UP);
	button_kill(ARROW_DOWN);
	button_kill('\r');

	return TRUE;
}


/**
 * Returns TRUE if there's anything to display a menu of 
 */
extern bool seen_tval(int tval)
{
	int i;

	for (i = 1; i < z_info->k_max; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Skip empty objects, unseen objects, and incorrect tvals */
		if (!k_ptr->name)
			continue;
		if (!k_ptr->everseen)
			continue;
		if (k_ptr->tval != tval)
			continue;

		return TRUE;
	}


	return FALSE;
}


/** Extra options on the "item options" menu */
struct {
	char tag;
	char *name;
	void (*action) (const char *unused, int also_unused);
} extra_item_options[] = {
	{
	'Q', "Quality squelching options", quality_menu}, {
	'E', "Ego squelching options", ego_menu}, {
'{', "Autoinscription setup", textui_browse_object_knowledge},};

static char tag_options_item(menu_type * menu, int oid)
{
	size_t line = (size_t) oid;

	if (line < N_ELEMENTS(sval_dependent))
		return I2A(oid);

	/* Separator - blank line. */
	if (line == N_ELEMENTS(sval_dependent))
		return 0;

	line = line - N_ELEMENTS(sval_dependent) - 1;

	if (line < N_ELEMENTS(extra_item_options))
		return extra_item_options[line].tag;

	return 0;
}

static int valid_options_item(menu_type * menu, int oid)
{
	size_t line = (size_t) oid;

	if (line < N_ELEMENTS(sval_dependent))
		return 1;

	/* Separator - blank line. */
	if (line == N_ELEMENTS(sval_dependent))
		return 0;

	line = line - N_ELEMENTS(sval_dependent) - 1;

	if (line < N_ELEMENTS(extra_item_options))
		return 1;

	return 0;
}

static void display_options_item(menu_type * menu, int oid, bool cursor,
								 int row, int col, int width)
{
	size_t line = (size_t) oid;

	/* First section of menu - the svals */
	if (line < N_ELEMENTS(sval_dependent)) {
		bool known = seen_tval(sval_dependent[line].tval);
		byte attr =
			curs_attrs[known ? CURS_KNOWN : CURS_UNKNOWN][(int) cursor];

		c_prt(attr, sval_dependent[line].desc, row, col);
	}
	/* Second section - the "extra options" */
	else {
		byte attr = curs_attrs[CURS_KNOWN][(int) cursor];

		line = line - N_ELEMENTS(sval_dependent) - 1;

		if (line < N_ELEMENTS(extra_item_options))
			c_prt(attr, extra_item_options[line].name, row, col);
	}
}


bool handle_options_item(menu_type * menu, const ui_event * event, int oid)
{
	if (event->type == EVT_SELECT) {
		if ((size_t) oid < N_ELEMENTS(sval_dependent)) {
			sval_menu(sval_dependent[oid].tval, sval_dependent[oid].desc);
		} else {
			oid = oid - (int) N_ELEMENTS(sval_dependent) - 1;
			assert((size_t) oid < N_ELEMENTS(extra_item_options));
			extra_item_options[oid].action(NULL, 0);
		}

		return TRUE;
	}

	return FALSE;
}

static const menu_iter options_item_iter = {
	tag_options_item,
	valid_options_item,
	display_options_item,
	handle_options_item,
	NULL
};


/**
 * Display and handle the main squelching menu.
 */
void do_cmd_options_item(const char *name, int row)
{
	menu_type menu;

	menu.count =
		N_ELEMENTS(sval_dependent) + N_ELEMENTS(extra_item_options) + 1;
	menu_init(&menu, MN_SKIN_SCROLL, &options_item_iter);
	menu_setpriv(&menu, N_ELEMENTS(sval_dependent) +
				 N_ELEMENTS(extra_item_options) + 1, NULL);

	menu.title = name;
	menu_layout(&menu, &SCREEN_REGION);

	/* Save and clear screen */
	screen_save();
	clear_from(0);

	menu_select(&menu, 0, FALSE);
	/* Load screen */
	screen_load();

	/* Notice changes */
	p_ptr->notice |= PN_SQUELCH;

	return;

}


/*** Main menu definitions and display ***/

static menu_type *option_menu;
static menu_action option_actions[] = {
	{0, 'a', "Interface options", option_toggle_menu},
	{0, 'b', "Gameplay options", option_toggle_menu},
	{0, 'c', "Cheat options", option_toggle_menu},
	{0, 0, 0, 0},				/* Load and append */
	{0, 'w', "Subwindow display settings", do_cmd_options_win},
	{0, 's', "Item squelch settings", do_cmd_options_item},
	{0, 'd', "Set base delay factor", do_cmd_delay},
	{0, 'h', "Set hitpoint warning", do_cmd_hp_warn},
	{0, 'p', "Set panel change factor", do_cmd_panel_change},
	{0, 'i', "Set movement delay", do_cmd_lazymove_delay},
	{0, 'l', "Load a user pref file", options_load_pref_file},
	{0, 'o', "Save options", do_dump_options},
	{0, 'x', "Autosave options", do_cmd_options_autosave},
	{0, 0, 0, 0},				/* Interact with */

	{0, 'm', "Interact with keymaps (advanced)", do_cmd_keymaps},
	{0, 'v', "Interact with visuals (advanced)", do_cmd_visuals},

#ifdef ALLOW_COLORS
	{0, 'c', "Interact with colours (advanced)", do_cmd_colors},
#endif							/* ALLOW_COLORS */
};


/*
 * Display the options main menu.
 */
void do_cmd_options(void)
{
	if (!option_menu) {
		/* Main option menu */
		option_menu = menu_new_action(option_actions,
									  N_ELEMENTS(option_actions));

		option_menu->title = "Options Menu";
		option_menu->flags = MN_CASELESS_TAGS;
	}


	screen_save();
	clear_from(0);

	menu_layout(option_menu, &SCREEN_REGION);
	menu_select(option_menu, 0, FALSE);

	screen_load();
}
