/*
 * File: ui-birth.c
 * Purpose: Text-based user interface for character creation
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
#include "files.h"
#include "game-cmd.h"
#include "game-event.h"
#include "mapmode.h"
#include "ui-birth.h"
#include "ui-menu.h"

/*
 * Overview
 * ========
 * This file implements the user interface side of the birth process
 * for the classic terminal-based UI of Angband.
 *
 * It models birth as a series of steps which must be carried out in 
 * a specified order, with the option of stepping backwards to revisit
 * past choices.
 *
 * It starts when we receive the EVENT_ENTER_BIRTH event from the game,
 * and ends when we receive the EVENT_LEAVE_BIRTH event.  In between,
 * we will repeatedly be asked to supply a game command, which change
 * the state of the character being rolled.  Once the player is happy
 * with their character, we send the CMD_ACCEPT_CHARACTER command.
 */


/* A local-to-this-file global to hold the most important bit of state
   between calls to the game proper.  Probably not strictly necessary,
   but reduces complexity a bit. */
enum birth_stage {
	BIRTH_BACK = -1,
	BIRTH_RESET = 0,
	BIRTH_QUICKSTART,
	BIRTH_MAP_CHOICE,
	BIRTH_MODE_CHOICE,
	BIRTH_SEX_CHOICE,
	BIRTH_RACE_CHOICE,
	BIRTH_CLASS_CHOICE,
	BIRTH_ROLLER_CHOICE,
	BIRTH_POINTBASED,
	BIRTH_ROLLER,
	BIRTH_NAME_CHOICE,
	BIRTH_FINAL_CONFIRM,
	BIRTH_COMPLETE
};


enum birth_questions {
	BQ_METHOD = 0,
	BQ_SEX,
	BQ_RACE,
	BQ_CLASS,
	BQ_ROLLER,
	MAX_BIRTH_QUESTIONS
};

enum birth_rollers {
	BR_POINTBASED = 0,
	BR_NORMAL,
	MAX_BIRTH_ROLLERS
};


static void point_based_start(void);
static bool quickstart_allowed = FALSE;

/* ------------------------------------------------------------------------
 * Quickstart? screen.
 * ------------------------------------------------------------------------ */
static enum birth_stage get_quickstart_command(void)
{
	const char *prompt =
		"['Y' to use this character, 'N' to start afresh, 'C' to change name]";
	ui_event ke;

	enum birth_stage next = BIRTH_QUICKSTART;

	/* Prompt for it */
	prt("New character based on previous one:", 0, 0);
	prt(prompt, Term->hgt - 1, Term->wid / 2 - strlen(prompt) / 2);

	do {
		/* Get a key */
		ke = inkey_ex();

		if (ke.key.code == 'N' || ke.key.code == 'n') {
			cmd_insert(CMD_BIRTH_RESET);
			next = BIRTH_MAP_CHOICE;
		} else if (ke.key.code == KTRL('X')) {
			cmd_insert(CMD_QUIT);
			next = BIRTH_COMPLETE;
		} else if (ke.key.code == 'C' || ke.key.code == 'c') {
			next = BIRTH_NAME_CHOICE;
		} else if (ke.key.code == 'Y' || ke.key.code == 'y') {
			cmd_insert(CMD_ACCEPT_CHARACTER);
			next = BIRTH_COMPLETE;
		}
	} while (next == BIRTH_QUICKSTART);

	/* Clear prompt */
	clear_from(23);

	return next;
}

/* ------------------------------------------------------------------------
 * Set the map
 * ------------------------------------------------------------------------ */
#define MAP_TEXT \
    "    *    \n" \
    "  #{light yellow}#{/} {light yellow}#{/}#  \n" \
    " #{light green}#{/}{light yellow}#{/} {light yellow}#{/}{light green}#{/}# \n" \
    "/{light yellow}##{/}{deep light blue}/{/}{yellow}@{/}{deep light blue}\\{/}{light yellow}##{/}\\\n" \
    "*  {yellow}@{/}{orange}@{/}{yellow}@{/}  *  Welcome to First Age Angband!\n" \
    "\\{light yellow}##{/}{deep light blue}\\{/}{yellow}@{/}{deep light blue}/{/}{light yellow}##{/}/\n" \
    " #{light green}#{/}{light yellow}#{/} {light yellow}#{/}{light green}#{/}# \n" \
    "  #{light yellow}#{/} {light yellow}#{/}#  \n" \
    "    *    \n\n"				     \
    "There are four different ways of structuring the world\n" \
    "of FAangband; '{light green}?{/}' gives more details."

/* Show the map instructions */
static void print_map_instructions(void)
{
	/* Clear screen */
	Term_clear();

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = 5;
	Term_gotoxy(5, 1);

	/* Display some helpful information */
	text_out_e(MAP_TEXT);

	/* Reset text_out() indentation */
	text_out_indent = 0;
}

/**
 * Map descriptions
 */
const char *map_name[] = {
	"Standard wilderness",
	"Extended wilderness",
	"Hybrid dungeon",
	"Angband dungeon",
	"Quit"
};

const char *map_description[] = {
	"The standard FAangband wilderness.  You will start in a town, and need to traverse the map to find dungeons, fighting enemies as you go.  Each dungeon has a guardian at the bottom.",
	"A more extended wilderness, which was the standard up until version 1.2.",
	"Here you descend just through the dungeons of Beleriand, with portals at the bottom of each which will take you to the next one.  All guardians except the last are optional to fight.",
	"An experience more like regular Angband, with a single town, a single dungeon, and two quests - Sauron on level 99, and Morgoth on level 100.",
	"Quit FAangband."
};

/**
 * Map menu data struct
 */
struct map_menu_data {
	const char *maps[MAP_MAX + 1];
	const char *description[MAP_MAX + 1];

	int selected_map;
};


/**
 * Display a row of the map menu
 */
static void map_menu_display(menu_type * m, int oid, bool cursor,
							 int row, int col, int wid)
{
	struct map_menu_data *d = menu_priv(m);
	int attr_name = cursor ? TERM_L_BLUE : TERM_WHITE;

	/* Dump the name */
	c_put_str(attr_name, d->maps[oid], row, col);
}

/**
 * Handle an event on a menu row.
 */
static bool map_menu_handler(menu_type * m, const ui_event * e, int oid)
{
	struct map_menu_data *d = menu_priv(m);

	/* Select */
	if (e->type == EVT_SELECT) {
		d->selected_map = oid;
		return FALSE;
	} else if (e->type == EVT_ESCAPE) {
		d->selected_map = -1;
		return FALSE;
	} else if (e->type == EVT_KBRD) {
		/* Get help */
		if (e->key.code == '?') {
			screen_save();
			show_file("mapmode.txt", NULL, 0, 0);
			screen_load();
		}
		return FALSE;
	}
	return TRUE;
}

/**
 * Show map description when browsing
 */
static void map_menu_browser(int oid, void *data, const region * loc)
{
	struct map_menu_data *d = data;

	/* Redirect output to the screen */
	text_out_hook = text_out_to_screen;
	text_out_wrap = 60;
	text_out_indent = loc->col - 1;
	text_out_pad = 1;

	/* Write the description */
	Term_gotoxy(loc->col, loc->row + loc->page_rows);
	text_out_c(TERM_DEEP_L_BLUE, format("\n%s\n", d->description[oid]));

	/* Reset parameters */
	text_out_pad = 0;
	text_out_indent = 0;
	text_out_wrap = 0;
}

static const menu_iter map_menu_iter = {
	NULL,						/* get_tag = NULL, use lowercase selections */
	NULL,						/* no validity hook */
	map_menu_display,
	map_menu_handler,
	NULL						/* no resize hook */
};

/** Create and initialise the map menu */
static menu_type *map_menu_new(void)
{
	menu_type *m = menu_new(MN_SKIN_SCROLL, &map_menu_iter);
	struct map_menu_data *d = mem_alloc(sizeof *d);
	int i;

	region loc = { 5, 14, 70, -99 };

	/* Copy across private data */
	d->selected_map = -1;
	for (i = 0; i <= MAP_MAX; i++) {
		d->maps[i] = map_name[i];
		d->description[i] = map_description[i];
	}
	menu_setpriv(m, MAP_MAX + 1, d);

	/* Set flags */
	m->cmd_keys = "?";
	m->flags = MN_CASELESS_TAGS;
	m->selections = lower_case;
	m->browse_hook = map_menu_browser;

	/* Set size */
	loc.page_rows = MAP_MAX + 2;
	menu_layout(m, &loc);

	return m;
}

/** Clean up a map menu instance */
static void map_menu_destroy(menu_type * m)
{
	struct map_menu_data *d = menu_priv(m);
	mem_free(d);
	mem_free(m);
}

/**
 * Run the map menu to select a map.
 */
static int map_menu_select(menu_type * m)
{
	struct map_menu_data *d = menu_priv(m);

	screen_save();
	region_erase_bordered(&m->active);

	menu_select(m, 0, TRUE);

	screen_load();

	return d->selected_map;
}

/**
 * Choose which map to play on
 */
static enum birth_stage get_map_command(void)
{
	enum birth_stage next;
	menu_type *m;

	m = map_menu_new();
	if (m) {
		int map = map_menu_select(m);
		if (map == -1) {
			next = BIRTH_BACK;
		} else if (map == MAP_MAX) {
			cmd_insert(CMD_QUIT);
			next = BIRTH_COMPLETE;
		} else {
			cmd_insert(CMD_SET_MAP);
			cmd_set_arg_choice(cmd_get_top(), 0, m->cursor);
			next = BIRTH_MODE_CHOICE;
		}
	} else {
		next = BIRTH_BACK;
	}
	map_menu_destroy(m);

	return next;
}

/* ------------------------------------------------------------------------
 * Get the game mode (formerly birth options)
 * ------------------------------------------------------------------------ */
#define MODE_TEXT \
    "Toggle any of the permanent game modes, '{light green}?{/}'" \
    "for help,\naccept or quit; selected modes appear {red}red{/}:"


/* Show the mode instructions */
static void print_mode_instructions(void)
{
	/* Clear screen */
	clear_from(0);

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = 5;
	Term_gotoxy(5, 0);

	/* Display some helpful information */
	text_out_e(MODE_TEXT);

	/* Reset text_out() indentation */
	text_out_indent = 0;
}

/**
 * Mode descriptions
 *
 * Note that there are two more menu entries than modes, used for 
 * accepting and quitting
 */
const char *mode_name[] = {
	"Thrall",
	"Ironman",
	"Disconnected stairs",
	"Small device",
	"No artifacts",
	"No selling",
	"Smart cheat",
	"Accept current modes",
	"Quit"
};

const char *mode_description[] = {
	"Start as a thrall on the steps of Angband",
	"Only ever go into greater danger until you win or die",
	"Taking a dungeon stair or wilderness path will not place you on a stair or path back the way you came",
	"Player and monster spell and view distances are halved.  This is good if you have a small screen or large tiles",
	"Artifacts are never generated",
	"You cannot sell items to shops for money, but you will get more money from the dungeon floor and monsters",
	"Monsters know your weaknesses without having to learn them",
	"Accept the currently selected game modes",
	"Quit FAangband"
};

/**
 * Mode menu data struct
 */
struct mode_menu_data {
	const char *modes[GAME_MODE_MAX + 2];
	const char *description[GAME_MODE_MAX + 2];

	bool mode_settings[GAME_MODE_MAX + 1];
};

/**
 * Is item oid valid?
 */
static int mode_menu_valid(menu_type * m, int oid)
{
	/* No thralls in FAnilla map */
	if (MAP(FANILLA) && (oid == GAME_MODE_THRALL))
		return 0;

	return 1;
}

/**
 * Display a row of the mode menu
 */
static void mode_menu_display(menu_type * m, int oid, bool cursor,
							  int row, int col, int wid)
{
	struct mode_menu_data *d = menu_priv(m);
	int attr_name = cursor ? TERM_SLATE : TERM_WHITE;

	/* Chosen modes show in red */
	if (d->mode_settings[oid])
		attr_name = cursor ? TERM_L_RED : TERM_RED;

	/* Dump the name */
	c_put_str(attr_name, d->modes[oid], row, col);
}

/**
 * Handle an event on a menu row.
 */
static bool mode_menu_handler(menu_type * m, const ui_event * e, int oid)
{
	struct mode_menu_data *d = menu_priv(m);

	if (e->type == EVT_SELECT) {
		/* We're done, return */
		if (oid == GAME_MODE_MAX)
			return FALSE;
		/* Quitting */
		else if (oid == GAME_MODE_MAX + 1) {
			d->mode_settings[GAME_MODE_MAX + 1] = TRUE;
			return FALSE;
		}
		/* Toggle */
		else {
			d->mode_settings[oid] = !d->mode_settings[oid];
		}
	} else if (e->type == EVT_KBRD) {
		if (e->key.code == ESCAPE) {
			d->mode_settings[GAME_MODE_MAX] = TRUE;
		} else if (e->key.code == '?') {
			/* Get help */
			const char *name = d->modes[oid];
			screen_save();
			show_file(format("mapmode.txt#%s", name), NULL, 0, 0);
			screen_load();
		}
		return FALSE;
	}

	return TRUE;
}

/**
 * Show mode description when browsing
 */
static void mode_menu_browser(int oid, void *data, const region * loc)
{
	struct mode_menu_data *d = data;

	/* Redirect output to the screen */
	text_out_hook = text_out_to_screen;
	text_out_wrap = 60;
	text_out_indent = loc->col - 1;
	text_out_pad = 1;

	/* Mode description */
	Term_gotoxy(loc->col, loc->row + loc->page_rows);
	text_out_c(TERM_DEEP_L_BLUE, format("\n%s\n", d->description[oid]));

	/* Reset parameters */
	text_out_pad = 0;
	text_out_indent = 0;
	text_out_wrap = 0;
}

static const menu_iter mode_menu_iter = {
	NULL,						/* get_tag = NULL, use lowercase selections */
	mode_menu_valid,
	mode_menu_display,
	mode_menu_handler,
	NULL						/* no resize hook */
};

/** Create and initialise the mode menu */
static menu_type *mode_menu_new(void)
{
	menu_type *m = menu_new(MN_SKIN_SCROLL, &mode_menu_iter);
	struct mode_menu_data *d = mem_alloc(sizeof *d);
	int i;
	const char cmd_keys[] = { '?', (char) ESCAPE, '\0' };

	region loc = { 5, 4, 70, -99 };

	/* Current game modes */
	for (i = 0; i < GAME_MODE_MAX; i++) {
		d->mode_settings[i] = p_ptr->game_mode[i];
	}
	/* Don't go back yet */
	d->mode_settings[GAME_MODE_MAX] = FALSE;
	/* Don't quit yet */
	d->mode_settings[GAME_MODE_MAX + 1] = FALSE;
	/* Actual mode names and descriptions, plus accept and quit */
	for (i = 0; i < GAME_MODE_MAX + 2; i++) {
		d->modes[i] = mode_name[i];
		d->description[i] = mode_description[i];
	}

	/* Copy across private data */
	menu_setpriv(m, GAME_MODE_MAX + 2, d);

	/* set flags */
	m->flags = MN_CASELESS_TAGS;
	m->cmd_keys = cmd_keys;
	m->selections = lower_case;
	m->browse_hook = mode_menu_browser;

	/* set size */
	loc.page_rows = GAME_MODE_MAX + 3;
	menu_layout(m, &loc);

	return m;
}

/** Clean up a mode menu instance */
static void mode_menu_destroy(menu_type * m)
{
	struct mode_menu_data *d = menu_priv(m);
	mem_free(d);
	mem_free(m);
}

/**
 * Run the mode menu to select game modes.
 */
static bool *mode_menu_select(menu_type * m)
{
	struct mode_menu_data *d = menu_priv(m);

	screen_save();
	region_erase_bordered(&m->active);

	menu_select(m, 0, TRUE);

	screen_load();

	return (bool *) d->mode_settings;
}

/**
 * This function gets game mode choices from the player, encodes them as a
 * string of Ys and Ns and passes that back to the game
 */
static enum birth_stage get_mode_command(void)
{
	enum birth_stage next;
	menu_type *m;

	m = mode_menu_new();
	if (m) {
		bool *selections = mode_menu_select(m);
		if (selections[GAME_MODE_MAX] == TRUE) {
			next = BIRTH_BACK;
		} else if (selections[GAME_MODE_MAX + 1] == TRUE) {
			cmd_insert(CMD_QUIT);
			next = BIRTH_COMPLETE;
		} else {
			char modes[GAME_MODE_MAX];
			int i;

			/* Write a Y/N string for game mode setting */
			for (i = 0; i < GAME_MODE_MAX; i++)
				modes[i] = selections[i] ? 'Y' : 'N';

			cmd_insert(CMD_SET_MODES);
			cmd_set_arg_string(cmd_get_top(), 0, (const char *) modes);
			next = BIRTH_SEX_CHOICE;
		}
	} else {
		next = BIRTH_BACK;
	}
	mode_menu_destroy(m);

	return next;
}

/* ------------------------------------------------------------------------
 * The various "menu" bits of the birth process - namely choice of sex,
 * race, class, and roller type.
 * ------------------------------------------------------------------------ */

/* The various menus */
static menu_type sex_menu, race_menu, class_menu, roller_menu;

/* Locations of the menus, etc. on the screen */
#define HEADER_ROW       1
#define QUESTION_ROW     7
#define TABLE_ROW       10

#define QUESTION_COL     2
#define SEX_COL          2
#define RACE_COL        14
#define RACE_AUX_COL    29
#define CLASS_COL       29
#define CLASS_AUX_COL   48

static region gender_region = { SEX_COL, TABLE_ROW, 15, -2 };
static region race_region = { RACE_COL, TABLE_ROW, 15, -2 };
static region class_region = { CLASS_COL, TABLE_ROW, 19, -2 };
static region roller_region = { 44, TABLE_ROW, 21, -2 };

/* We use different menu "browse functions" to display the help text
   sometimes supplied with the menu items - currently just the list
   of bonuses, etc, corresponding to each race and class. */
typedef void (*browse_f) (int oid, void *db, const region * l);

/* We have one of these structures for each menu we display - it holds
   the useful information for the menu - text of the menu items, "help"
   text, current (or default) selection, and whether random selection
   is allowed. */
struct birthmenu_data {
	const char **items;
	const char *hint;
	bool allow_random;
};

/* A custom "display" function for our menus that simply displays the
   text from our stored data in a different colour if it's currently
   selected. */
static void birthmenu_display(menu_type * menu, int oid, bool cursor,
							  int row, int col, int width)
{
	struct birthmenu_data *data = menu->menu_data;

	byte attr = curs_attrs[CURS_KNOWN][0 != cursor];
	c_put_str(attr, data->items[oid], row, col);
}

/* Our custom menu iterator, only really needed to allow us to override
   the default handling of "commands" in the standard iterators (hence
   only defining the display and handler parts). */
static const menu_iter birth_iter =
	{ NULL, NULL, birthmenu_display, NULL, NULL };

static void race_help(int i, void *db, const region * l)
{
	int j;
	byte color;
	struct player_race *race = &p_info[i];
	int locality = stage_map[towns[race->hometown]][LOCALITY];

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = RACE_AUX_COL;
	Term_gotoxy(RACE_AUX_COL, TABLE_ROW);

	for (j = 0; j < A_MAX; j++) {
		text_out_e("%s%+d\n", stat_names_reduced[j], race->r_adj[j]);
	}

	text_out_e("Hit die: %d\n", p_info[i].r_mhp);
	if (MAP(COMPRESSED) || MAP(EXTENDED)) {
		text_out_e("Difficulty: Level %d\n", race->difficulty);

		/* Color code difficulty factor */
		if (p_info[i].difficulty < 3)
			color = TERM_GREEN;
		else if (p_info[i].difficulty < 15)
			color = TERM_ORANGE;
		else
			color = TERM_RED;

		text_out_c(color, format("Home town: %-15s\n", 
								 locality_name[locality]));
	}
	text_out_e("Infravision: %d ft", race->infra * 10);

	/* Reset text_out() indentation */
	text_out_indent = 0;
}

static void class_help(int i, void *db, const region * l)
{
	int j;

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = CLASS_AUX_COL;
	Term_gotoxy(CLASS_AUX_COL, TABLE_ROW);

	for (j = 0; j < A_MAX; j++) {
		text_out_e("%s%+d\n", stat_names_reduced[j], c_info[i].c_adj[j]);
	}

	text_out_e("Hit die: %d\n\n\n", c_info[i].c_mhp);

	/* Reset text_out() indentation */
	text_out_indent = 0;
}

/* Set up one of our menus ready to display choices for a birth question.
   This is slightly involved. */
static void init_birth_menu(menu_type * menu, int n_choices,
							int initial_choice, const region * reg,
							bool allow_random, browse_f aux)
{
	struct birthmenu_data *menu_data;

	/* Initialise a basic menu */
	menu_init(menu, MN_SKIN_SCROLL, &birth_iter);

	/* A couple of behavioural flags - we want selections letters in
	   lower case and a double tap to act as a selection. */
	menu->selections = lower_case;
	menu->flags = MN_DBL_TAP;

	/* Copy across the game's suggested initial selection, etc. */
	menu->cursor = initial_choice;

	/* Allocate sufficient space for our own bits of menu information. */
	menu_data = mem_alloc(sizeof *menu_data);

	/* Allocate space for an array of menu item texts and help texts
	   (where applicable) */
	menu_data->items = mem_alloc(n_choices * sizeof *menu_data->items);
	menu_data->allow_random = allow_random;

	/* Set private data */
	menu_setpriv(menu, n_choices, menu_data);

	/* Set up the "browse" hook to display help text (where applicable). */
	menu->browse_hook = aux;

	/* Lay out the menu appropriately */
	menu_layout(menu, reg);
}



static void setup_menus()
{
	int i;

	const char *roller_choices[MAX_BIRTH_ROLLERS] = {
		"Point-based",
		"Standard roller"
	};

	struct birthmenu_data *mdata;

	/* Sex menu fairly straightforward */
	init_birth_menu(&sex_menu, MAX_SEXES, p_ptr->psex, &gender_region,
					TRUE, NULL);
	mdata = sex_menu.menu_data;
	for (i = 0; i < MAX_SEXES; i++) {
		mdata->items[i] = sex_info[i].title;
	}
	mdata->hint =
		"Your 'sex' does not have any significant gameplay effects.";

	/* Race menu more complicated. */
	init_birth_menu(&race_menu, z_info->p_max, p_ptr->prace, &race_region,
					TRUE, race_help);
	mdata = race_menu.menu_data;

	for (i = 0; i < z_info->p_max; i++) {
		mdata->items[i] = p_info[i].name;
	}
	mdata->hint =
		"Your 'race' determines various intrinsic factors and bonuses.";

	/* Class menu similar to race. */
	init_birth_menu(&class_menu, z_info->c_max, p_ptr->pclass,
					&class_region, TRUE, class_help);
	mdata = class_menu.menu_data;

	for (i = 0; i < z_info->c_max; i++) {
		mdata->items[i] = c_info[i].name;
	}
	mdata->hint =
		"Your 'class' determines various intrinsic abilities and bonuses";

	/* Roller menu straightforward again */
	init_birth_menu(&roller_menu, MAX_BIRTH_ROLLERS, 0, &roller_region,
					FALSE, NULL);
	mdata = roller_menu.menu_data;
	for (i = 0; i < MAX_BIRTH_ROLLERS; i++) {
		mdata->items[i] = roller_choices[i];
	}
	mdata->hint =
		"Your choice of character generation.  Point-based is recommended.";
}

/* Cleans up our stored menu info when we've finished with it. */
static void free_birth_menu(menu_type * menu)
{
	struct birthmenu_data *data = menu->menu_data;

	if (data) {
		mem_free(data->items);
		mem_free(data);
	}
}

static void free_birth_menus()
{
	/* We don't need these any more. */
	free_birth_menu(&sex_menu);
	free_birth_menu(&race_menu);
	free_birth_menu(&class_menu);
	free_birth_menu(&roller_menu);
}

/*
 * Clear the previous question
 */
static void clear_question(void)
{
	int i;

	for (i = QUESTION_ROW; i < TABLE_ROW; i++) {
		/* Clear line, position cursor */
		Term_erase(0, i, 255);
	}
}


#define BIRTH_MENU_HELPTEXT \
	"{light blue}Please select your character from the menu below:{/}\n\n" \
	"Use the {light green}movement keys{/} to scroll the menu, " \
	"{light green}Enter{/} to select the current menu item, '{light green}*{/}' " \
	"for a random menu item, '{light green}ESC{/}' to step back through the " \
	"birth process, '{light green}?{/} " \
	"for help, or '{light green}Ctrl-X{/}' to quit."

/* Show the birth instructions on an otherwise blank screen */
static void print_menu_instructions(void)
{
	/* Clear screen */
	Term_clear();

	/* Output to the screen */
	text_out_hook = text_out_to_screen;

	/* Indent output */
	text_out_indent = QUESTION_COL;
	text_out_wrap = 60;
	Term_gotoxy(QUESTION_COL, HEADER_ROW);

	/* Display some helpful information */
	text_out_e(BIRTH_MENU_HELPTEXT);

	/* Reset text_out() indentation */
	text_out_indent = 0;
	text_out_wrap = 0;
}

/* Allow the user to select from the current menu, and return the 
   corresponding command to the game.  Some actions are handled entirely
   by the UI (displaying help text, for instance). */
static enum birth_stage menu_question(enum birth_stage current,
									  menu_type * current_menu,
									  cmd_code choice_command)
{
	struct birthmenu_data *menu_data = menu_priv(current_menu);
	ui_event cx;

	enum birth_stage next = BIRTH_RESET;

	/* Print the question currently being asked. */
	clear_question();
	Term_putstr(QUESTION_COL, QUESTION_ROW, -1, TERM_YELLOW,
				menu_data->hint);

	current_menu->cmd_keys = "?*\x18";	/* ?, *, <ctl-X> */

	while (next == BIRTH_RESET) {
		/* Display the menu, wait for a selection of some sort to be made. */
		cx = menu_select(current_menu, EVT_KBRD, FALSE);

		/* As all the menus are displayed in "hierarchical" style, we allow
		   use of "back" (left arrow key or equivalent) to step back in 
		   the proces as well as "escape". */
		if (cx.type == EVT_ESCAPE) {
			next = BIRTH_BACK;
		} else if (cx.type == EVT_SELECT) {
			if (current == BIRTH_ROLLER_CHOICE) {
				cmd_insert(CMD_FINALIZE_OPTIONS);

				if (current_menu->cursor) {
					/* Do a first roll of the stats */
					cmd_insert(CMD_ROLL_STATS);
					next = current + 2;
				} else {
					/* 
					 * Make sure we've got a point-based char to play with. 
					 * We call point_based_start here to make sure we get
					 * an update on the points totals before trying to
					 * display the screen.  The call to CMD_RESET_STATS
					 * forces a rebuying of the stats to give us up-to-date
					 * totals.  This is, it should go without saying, a hack.
					 */
					point_based_start();
					cmd_insert(CMD_RESET_STATS);
					cmd_set_arg_choice(cmd_get_top(), 0, TRUE);
					next = current + 1;
				}
			} else {
				cmd_insert(choice_command);
				cmd_set_arg_choice(cmd_get_top(), 0, current_menu->cursor);
				next = current + 1;
			}
		} else if (cx.type == EVT_KBRD) {
			/* '*' chooses an option at random from those the game provides. */
			if (cx.key.code == '*' && menu_data->allow_random) {
				current_menu->cursor = randint0(current_menu->count);
				cmd_insert(choice_command);
				cmd_set_arg_choice(cmd_get_top(), 0, current_menu->cursor);

				menu_refresh(current_menu, FALSE);
				next = current + 1;
			} else if (cx.key.code == KTRL('X')) {
				cmd_insert(CMD_QUIT);
				next = BIRTH_COMPLETE;
			} else if (cx.key.code == '?') {
				const char *name =
					(const char *) menu_data->items[current_menu->cursor];
				screen_save();
				show_file(format("raceclas.txt#%s", name), NULL, 0, 0);
				screen_load();
			}
		}
	}

	return next;
}

/* ------------------------------------------------------------------------
 * The rolling bit of the roller.
 * ------------------------------------------------------------------------ */
#define ROLLERCOL 42

static enum birth_stage roller_command(bool first_call)
{
	char prompt[80] = "";
	size_t promptlen = 0;

	ui_event ke;
	keycode_t ch;

	enum birth_stage next = BIRTH_ROLLER;

	/* Used to keep track of whether we've rolled a character before or not. */
	static bool prev_roll = FALSE;

	/* Display the player - a bit cheaty, but never mind. */
	display_player(0);

	if (first_call)
		prev_roll = FALSE;

	clear_from(Term->hgt - 2);
	redraw_stuff(p_ptr);

	/* Prepare a prompt (must squeeze everything in) */
	strnfcat(prompt, sizeof(prompt), &promptlen, "['r' to reroll");
	if (prev_roll)
		strnfcat(prompt, sizeof(prompt), &promptlen, ", 'p' for prev");
	strnfcat(prompt, sizeof(prompt), &promptlen, " or 'Enter' to accept]");

	/* Prompt for it */
	prt(prompt, Term->hgt - 1, Term->wid / 2 - promptlen / 2);

	/* Prompt and get a command */
	ke = inkey_ex();
	ch = ke.key.code;

	if (ch == ESCAPE) {
		next = BIRTH_BACK;
	}

	/* 'Enter' accepts the roll */
	if (ch == KC_ENTER) {
		next = BIRTH_NAME_CHOICE;
	}

	/* Reroll this character */
	else if ((ch == ' ') || (ch == 'r')) {
		cmd_insert(CMD_ROLL_STATS);
		prev_roll = TRUE;
	}

	/* Previous character */
	else if (prev_roll && (ch == 'p')) {
		cmd_insert(CMD_PREV_STATS);
	}

	/* Quit */
	else if (ch == KTRL('X')) {
		cmd_insert(CMD_QUIT);
		next = BIRTH_COMPLETE;
	}

	/* Help XXX */
	else if (ch == '?') {
		do_cmd_help();
	}

	/* Nothing handled directly here */
	else {
		bell("Illegal roller command!");
	}

	return next;
}

/* ------------------------------------------------------------------------
 * Point-based stat allocation.
 * ------------------------------------------------------------------------ */

/* The locations of the "costs" area on the birth screen. */
#define COSTS_ROW 1
#define COSTS_COL (42 + 32)
#define TOTAL_COL (42 + 19)

/* This is called whenever the points totals are changed (in birth.c), so
   that we can update our display of how many points have been spent and
   are available. */
static void point_based_points(game_event_type type,
							   game_event_data * data, void *user)
{
	int i;
	int sum = 0;
	int *stats = data->birthstats.stats;

	display_player(0);
	dump_line_hook = dump_line_screen;

	/* Display the costs header */
	dump_row = COSTS_ROW - 1;
	dump_ptr = (char_attr *) & pline0[COSTS_ROW - 1];
	dump_put_str(TERM_WHITE, "Cost", COSTS_COL);
	dump_line(dump_ptr);

	/* Display the costs */
	for (i = 0; i < A_MAX; i++) {
		/* Display cost */
		dump_ptr = (char_attr *) & pline0[i + COSTS_ROW];
		dump_put_str(TERM_WHITE, format("%4d", stats[i]), COSTS_COL);
		sum += stats[i];
		dump_line(dump_ptr);
	}

	dump_ptr = (char_attr *) & pline0[A_MAX + COSTS_ROW];
	dump_put_str(TERM_WHITE,
				 format("Total Cost: %2d/%2d", sum,
						data->birthstats.remaining + sum), TOTAL_COL);
	dump_line(dump_ptr);
}

/* This is called whenever a stat changes.  We take the easy road, and just
   redisplay them all using the standard function. */
static void point_based_stats(game_event_type type, game_event_data * data,
							  void *user)
{
	if (data)
		point_based_points(type, data, user);
}

/* This is called whenever any of the other miscellaneous stat-dependent things
   changed.  We are hooked into changes in the amount of gold in this case,
   but redisplay everything because it's easier. */
static void point_based_misc(game_event_type type, game_event_data * data,
							 void *user)
{
	if (data)
		point_based_points(type, data, user);
}


static void point_based_start(void)
{
	const char *prompt =
		"[up/down to move, left/right to modify, 'r' to reset, 'Enter' to accept]";

	/* Clear */
	Term_clear();

	/* Display the player */
	display_player(0);

	prt(prompt, Term->hgt - 1, Term->wid / 2 - strlen(prompt) / 2);

	/* Register handlers for various events - cheat a bit because we redraw
	   the lot at once rather than each bit at a time. */
	event_add_handler(EVENT_BIRTHPOINTS, point_based_points, NULL);
	event_add_handler(EVENT_STATS, point_based_stats, NULL);
	event_add_handler(EVENT_GOLD, point_based_misc, NULL);
}

static void point_based_stop(void)
{
	event_remove_handler(EVENT_BIRTHPOINTS, point_based_points, NULL);
	event_remove_handler(EVENT_STATS, point_based_stats, NULL);
	event_remove_handler(EVENT_GOLD, point_based_misc, NULL);
}

static enum birth_stage point_based_command(void)
{
	static int stat = 0;
	struct keypress ch;
	enum birth_stage next = BIRTH_POINTBASED;

/*	point_based_display();*/

	/* Place cursor just after cost of current stat */
	Term_gotoxy(COSTS_COL + 4, COSTS_ROW + stat);

	/* Get key */
	ch = inkey();

	if (ch.code == KTRL('X')) {
		cmd_insert(CMD_QUIT);
		next = BIRTH_COMPLETE;
	}

	/* Go back a step, or back to the start of this step */
	else if (ch.code == ESCAPE) {
		next = BIRTH_BACK;
	}

	else if (ch.code == 'r' || ch.code == 'R') {
		cmd_insert(CMD_RESET_STATS);
		cmd_set_arg_choice(cmd_get_top(), 0, FALSE);
	}

	/* Done */
	else if (ch.code == KC_ENTER) {
		next = BIRTH_NAME_CHOICE;
	} else {
		int dir = target_dir(ch);

		/* Prev stat, looping round to the bottom when going off the top */
		if (dir == 8)
			stat = (stat + A_MAX - 1) % A_MAX;

		/* Next stat, looping round to the top when going off the bottom */
		if (dir == 2)
			stat = (stat + 1) % A_MAX;

		/* Decrease stat (if possible) */
		if (dir == 4) {
			cmd_insert(CMD_SELL_STAT);
			cmd_set_arg_choice(cmd_get_top(), 0, stat);
		}

		/* Increase stat (if possible) */
		if (dir == 6) {
			cmd_insert(CMD_BUY_STAT);
			cmd_set_arg_choice(cmd_get_top(), 0, stat);
		}
	}

	return next;
}

/* ------------------------------------------------------------------------
 * Asking for the player's chosen name.
 * ------------------------------------------------------------------------ */
static enum birth_stage get_name_command(void)
{
	enum birth_stage next;
	char name[32];

	if (get_name(name, sizeof(name))) {
		cmd_insert(CMD_NAME_CHOICE);
		cmd_set_arg_string(cmd_get_top(), 0, name);
		next = BIRTH_FINAL_CONFIRM;
	} else {
		next = BIRTH_BACK;
	}

	return next;
}

/* ------------------------------------------------------------------------
 * Final confirmation of character.
 * ------------------------------------------------------------------------ */
static enum birth_stage get_confirm_command(void)
{
	const char *prompt =
		"['ESC' to step back, 'S' to start over, or any other key to continue]";
	ui_event ke;

	enum birth_stage next;

	/* Prompt for it */
	prt(prompt, Term->hgt - 1, Term->wid / 2 - strlen(prompt) / 2);

	/* Get a key */
	ke = inkey_ex();

	/* Start over */
	if (ke.key.code == 'S' || ke.key.code == 's') {
		next = BIRTH_RESET;
	} else if (ke.key.code == KTRL('X')) {
		cmd_insert(CMD_QUIT);
		next = BIRTH_COMPLETE;
	} else if (ke.key.code == ESCAPE) {
		next = BIRTH_BACK;
	} else {
		cmd_insert(CMD_ACCEPT_CHARACTER);
		next = BIRTH_COMPLETE;
	}

	/* Clear prompt */
	clear_from(23);

	return next;
}



/* ------------------------------------------------------------------------
 * Things that relate to the world outside this file: receiving game events
 * and being asked for game commands.
 * ------------------------------------------------------------------------ */

/*
 * This is called when we receive a request for a command in the birth 
 * process.

 * The birth process continues until we send a final character confirmation
 * command (or quit), so this is effectively called in a loop by the main
 * game.
 *
 * We're imposing a step-based system onto the main game here, so we need
 * to keep track of where we're up to, where each step moves on to, etc.
 */
errr get_birth_command(bool wait)
{
	static enum birth_stage current_stage = BIRTH_RESET;
	static enum birth_stage prev;
	static enum birth_stage roller = BIRTH_RESET;
	enum birth_stage next = current_stage;

	switch (current_stage) {
	case BIRTH_RESET:
		{
			cmd_insert(CMD_BIRTH_RESET);

			roller = BIRTH_RESET;

			if (quickstart_allowed)
				next = BIRTH_QUICKSTART;
			else
				next = BIRTH_MAP_CHOICE;

			break;
		}

	case BIRTH_QUICKSTART:
		{
			display_player(0);
			next = get_quickstart_command();
			break;
		}

	case BIRTH_MAP_CHOICE:
		{
			print_map_instructions();
			next = get_map_command();
			if (next == BIRTH_BACK)
				next = BIRTH_RESET;
			break;
		}

	case BIRTH_MODE_CHOICE:
		{
			//print_map_instructions();
			print_mode_instructions();
			next = get_mode_command();
			if (next == BIRTH_BACK)
				next = BIRTH_MAP_CHOICE;
			break;
		}

	case BIRTH_SEX_CHOICE:
	case BIRTH_CLASS_CHOICE:
	case BIRTH_RACE_CHOICE:
	case BIRTH_ROLLER_CHOICE:
		{
			menu_type *menu = &sex_menu;
			cmd_code command = CMD_CHOOSE_SEX;

			Term_clear();
			print_menu_instructions();

			if (current_stage > BIRTH_SEX_CHOICE) {
				menu_refresh(&sex_menu, FALSE);
				menu = &race_menu;
				command = CMD_CHOOSE_RACE;
			}

			if (current_stage > BIRTH_RACE_CHOICE) {
				menu_refresh(&race_menu, FALSE);
				menu = &class_menu;
				command = CMD_CHOOSE_CLASS;
			}

			if (current_stage > BIRTH_CLASS_CHOICE) {
				menu_refresh(&class_menu, FALSE);
				menu = &roller_menu;
				command = CMD_NULL;
			}

			next = menu_question(current_stage, menu, command);

			if (next == BIRTH_BACK)
				next = current_stage - 1;

			/* Make sure that the character gets reset before quickstarting */
			if (next == BIRTH_QUICKSTART)
				next = BIRTH_RESET;

			break;
		}

	case BIRTH_POINTBASED:
		{
			roller = BIRTH_POINTBASED;

			if (prev > BIRTH_POINTBASED)
				point_based_start();

			next = point_based_command();

			if (next == BIRTH_BACK)
				next = BIRTH_ROLLER_CHOICE;

			if (next != BIRTH_POINTBASED)
				point_based_stop();

			break;
		}

	case BIRTH_ROLLER:
		{
			roller = BIRTH_ROLLER;
			next = roller_command(prev < BIRTH_ROLLER);
			if (next == BIRTH_BACK)
				next = BIRTH_ROLLER_CHOICE;

			break;
		}

	case BIRTH_NAME_CHOICE:
		{
			if (prev < BIRTH_NAME_CHOICE)
				display_player(0);

			next = get_name_command();
			if (next == BIRTH_BACK)
				next = roller;

			break;
		}

	case BIRTH_FINAL_CONFIRM:
		{
			if (prev < BIRTH_FINAL_CONFIRM)
				display_player(0);

			next = get_confirm_command();
			if (next == BIRTH_BACK)
				next = BIRTH_NAME_CHOICE;

			break;
		}

	default:
		{
			/* Remove dodgy compiler warning, */
		}
	}

	prev = current_stage;
	current_stage = next;

	return 0;
}

/*
 * Called when we enter the birth mode - so we set up handlers, command hooks,
 * etc, here.
 */
static void ui_enter_birthscreen(game_event_type type,
								 game_event_data * data, void *user)
{
	/* Set the ugly static global that tells us if quickstart's available. */
	quickstart_allowed = data->flag;

	setup_menus();
}

static void ui_leave_birthscreen(game_event_type type,
								 game_event_data * data, void *user)
{
	free_birth_menus();
}


void ui_init_birthstate_handlers(void)
{
	event_add_handler(EVENT_ENTER_BIRTH, ui_enter_birthscreen, NULL);
	event_add_handler(EVENT_LEAVE_BIRTH, ui_leave_birthscreen, NULL);
}
