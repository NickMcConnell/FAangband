/**
 * \file player-properties.c 
 * \brief Class and race abilities
 *
 * Copyright (c) 1997-2020 Ben Harrison, James E. Wilson, Robert A. Koeneke,
 * Leon Marrick, Bahman Rabii, Nick McConnell
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
#include "game-input.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-properties.h"
#include "ui-input.h"
#include "ui-menu.h"

/**
 * ------------------------------------------------------------------------
 * Ability data structures and utilities
 * ------------------------------------------------------------------------ */
enum {
    PLAYER_FLAG_NONE,
    PLAYER_FLAG_SPECIAL,
    PLAYER_FLAG_RACE,
    PLAYER_FLAG_CLASS
};

struct player_ability *lookup_ability(const char *type, int index, int value)
{
	struct player_ability *ability;
	for (ability = player_abilities; ability; ability = ability->next) {
		if (!streq(ability->type, type)) continue;
		if (ability->index != index) continue;
		if (streq(type, "element") && (ability->value != value)) continue;
		return ability;
	}
	return NULL;
}

bool class_has_ability(const struct player_class *class,
					   struct player_ability *ability)
{
	if (streq(ability->type, "player") &&
		pf_has(class->pflags, ability->index)) {
		return true;
	} else if (streq(ability->type, "object") &&
			   of_has(class->flags, ability->index)) {
		return true;
	}
	return false;
}

bool race_has_ability(const struct player_race *race,
					  struct player_ability *ability)
{
	if (streq(ability->type, "player") &&
		pf_has(race->pflags, ability->index)) {
		return true;
	} else if (streq(ability->type, "object") &&
			   of_has(race->flags, ability->index)) {
		return true;
	} else if (streq(ability->type, "element") &&
			   (race->el_info[ability->index].res_level == ability->value)) {
		return true;
	}

	return false;
}

/**
 * ------------------------------------------------------------------------
 * Code for gaining specialty abilities
 * ------------------------------------------------------------------------ */
/**
 * Specialty ability menu data struct
 */
struct spec_menu_data {
	int *specialties;
	int spec_known;
	int selected_spec;
};

/**
 * Check if we can gain a specialty ability -BR-
 */
static bool check_specialty_gain(int specialty)
{
	/* Can't learn it if we already know it */
	if (pf_has(player->specialties, specialty)) {
		return false;
	}

	/* Is it allowed for this class? */
	if (pf_has(player->class->specialties, specialty)) {
		return true;
	}

	return false;
}

/**
 * Gain specialty code 
 */

static char gain_spec_tag(struct menu *menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the gain specialty menu
 */
static void gain_spec_display(struct menu *menu, int oid, bool cursor, int row,
					   int col, int width)
{
	struct spec_menu_data *d = menu_priv(menu);
	int idx = d->specialties[oid];
	byte attr = (cursor ? COLOUR_L_GREEN : COLOUR_GREEN);
	struct player_ability *ability = lookup_ability("player", idx, 0);

	/* Print it */
	c_put_str(attr, ability->name, row, col);

}

/**
 * Deal with events on the gain specialty menu
 */
static bool gain_spec_action(struct menu *menu, const ui_event *e, int oid)
{
	struct spec_menu_data *d = menu_priv(menu);
	static int i;
	if (oid)
		i = oid;

	if (e->type == EVT_SELECT) {
		d->selected_spec = d->specialties[i];
		return false;
	}

	return true;
}


/**
 * Show spell long description when browsing
 */
static void gain_spec_menu_browser(int oid, void *data, const region *loc)
{
	struct spec_menu_data *d = data;
	int idx = d->specialties[oid];
	struct player_ability *ability = lookup_ability("player", idx, 0);

	/* Redirect output to the screen */
	text_out_hook = text_out_to_screen;
	text_out_wrap = 60;
	text_out_indent = loc->col - 1;
	text_out_pad = 1;


	clear_from(loc->row + loc->page_rows);
	Term_gotoxy(loc->col, loc->row + loc->page_rows + 1);
	text_out_to_screen(COLOUR_DEEP_L_BLUE, (char *) ability->desc);

	/* Reset */
	text_out_pad = 0;
	text_out_indent = 0;
	text_out_wrap = 0;
}

/**
 * Display list available specialties.
 */
static bool gain_spec_menu(int *pick)
{
	struct menu menu;
	menu_iter menu_f = { gain_spec_tag, 0, gain_spec_display,
		gain_spec_action, 0
	};
	ui_event evt = { 0 };
	struct spec_menu_data *d = mem_alloc(sizeof *d);
	region loc = { 0, 0, 70, -99 };
	bool done = false;

	size_t i;

	char buf[80];

	/* Count the learnable specialties */
	d->spec_known = 0;
	for (i = 0; i < PF_MAX; i++) {
		if (check_specialty_gain(i)) {
			d->spec_known++;
		}
	}

	/* We are out of specialties - should never happen */
	if (!d->spec_known) {
		msg("No specialties available.");
		screen_load();
		return false;
	}

	/* Find the learnable specialties */
	d->specialties = mem_zalloc(d->spec_known * sizeof(int));
	d->spec_known = 0;
	for (i = 0; i < PF_MAX; i++) {
		if (check_specialty_gain(i)) {
			d->specialties[d->spec_known] = i;
			d->spec_known++;
		}
	}

	/* Save the screen and clear it */
	screen_save();
	clear_from(0);

	/* Prompt choices */
	strnfmt(buf, sizeof(buf),
			"(Specialties: %c-%c, ESC=exit) Gain which specialty (%d available)? ",
			I2A(0), I2A(d->spec_known - 1), player->upkeep->new_specialties);

	/* Set up the menu */
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu.title = buf;
	menu_setpriv(&menu, d->spec_known, d);
	loc.page_rows = d->spec_known + 2;
	menu.browse_hook = gain_spec_menu_browser;
	menu.flags = MN_DBL_TAP;
	region_erase_bordered(&loc);
	menu_layout(&menu, &loc);

	while (!done) {
		evt = menu_select(&menu, EVT_SELECT, true);
		done = (evt.type == EVT_ESCAPE);
		if (!done && (d->selected_spec)) {
			int idx = d->selected_spec;
			struct player_ability *ability = lookup_ability("player", idx, 0);
			region_erase_bordered(&loc);
			menu_layout(&menu, &loc);
			done = get_check(format("Definitely choose %s? ",
									ability->name));
		}
	}

	if (evt.type != EVT_ESCAPE)
		*pick = d->selected_spec;

	/* Load screen */
	screen_load();

	return (evt.type != EVT_ESCAPE);
}

/**
 * Gain a new specialty ability
 * Adapted from birth.c get_player_choice -BR-
 */
static void gain_specialty(void)
{
	int pick;

	/* Make one choice */
	if (gain_spec_menu(&pick)) {
		char buf[120];
		struct player_ability *ability = lookup_ability("player", pick, 0);

		/* Add it to the specialties */
		pf_on(player->specialties, pick);

		/* Specialty taken */
		strnfmt(buf, sizeof(buf), "Gained the %s specialty.", ability->name);

		/* Write a note */
		history_add(player, buf, HIST_GAIN_SPECIALTY);

		/* Update some stuff */
		player->upkeep->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS |
								   PU_SPECIALTY | PU_TORCH);

		/* Redraw Study Status */
		player->upkeep->redraw |= (PR_STUDY);
	}

	/* exit */
	return;
}


/**
 * ------------------------------------------------------------------------
 * Code for viewing race and class abilities
 * ------------------------------------------------------------------------ */
int num_abilities;
struct player_ability ability_list[32];


static char view_ability_tag(struct menu *menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the gain ability menu
 */
static void view_ability_display(struct menu *menu, int oid, bool cursor,
	int row, int col, int width)
{
	char buf[80];
	byte color;
	struct player_ability *choices = menu->menu_data;

	switch (choices[oid].group) {
	case PLAYER_FLAG_SPECIAL:
		{
			strnfmt(buf, sizeof(buf), "Specialty Ability: %s",
				choices[oid].name);
			color = COLOUR_GREEN;
			break;
		}
	case PLAYER_FLAG_CLASS:
		{
			strnfmt(buf, sizeof(buf), "Class: %s",
				choices[oid].name);
			color = COLOUR_UMBER;
			break;
		}
	case PLAYER_FLAG_RACE:
		{
			strnfmt(buf, sizeof(buf), "Racial: %s",
				choices[oid].name);
			color = COLOUR_ORANGE;
			break;
		}
	default:
		{
			my_strcpy(buf, "Mysterious", sizeof(buf));
			color = COLOUR_PURPLE;
		}
	}

	/* Print it */
	c_put_str(cursor ? COLOUR_WHITE : color, buf, row, col);

}


/**
 * Show ability long description when browsing
 */
static void view_ability_menu_browser(int oid, void *data, const region *loc)
{
	struct player_ability *choices = data;

	/* Redirect output to the screen */
	text_out_hook = text_out_to_screen;
	text_out_wrap = 60;
	text_out_indent = loc->col - 1;
	text_out_pad = 1;

	clear_from(loc->row + loc->page_rows);
	Term_gotoxy(loc->col, loc->row + loc->page_rows);
	text_out_c(COLOUR_L_BLUE, "\n%s\n", (char *) choices[oid].desc);

	/* XXX */
	text_out_pad = 0;
	text_out_indent = 0;
	text_out_wrap = 0;
}

/**
 * Display list available specialties.
 */
static void view_ability_menu(void)
{
	struct menu menu;
	menu_iter menu_f = { view_ability_tag, 0, view_ability_display, 0, 0 };
	region loc = { 0, 0, 70, -99 };
	char buf[80];

	/* Save the screen and clear it */
	screen_save();

	/* Prompt choices */
	strnfmt(buf, sizeof(buf),
			"Race, class, and specialty abilities (%c-%c, ESC=exit): ",
			I2A(0), I2A(num_abilities - 1));

	/* Set up the menu */
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu.header = buf;
	menu_setpriv(&menu, num_abilities, ability_list);
	loc.page_rows = num_abilities + 1;
	menu.flags = MN_DBL_TAP;
	menu.browse_hook = view_ability_menu_browser;
	region_erase_bordered(&loc);
	menu_layout(&menu, &loc);

	menu_select(&menu, 0, false);

	/* Load screen */
	screen_load();

	return;
}

/**
 * Browse known abilities -BR-
 */
static void view_abilities(void)
{
	int i;
	struct player_ability *ability;

	/* Count the number of specialties we know */
	num_abilities = 0;
	for (i = 0; i < PF_MAX; i++) {
		if (!pf_has(player->specialties, i)) continue;
		memcpy(&ability_list[num_abilities], lookup_ability("player", i, 0),
			   sizeof(struct player_ability));
		ability_list[num_abilities++].group = PLAYER_FLAG_SPECIAL;
	}

	/* Count the number of class powers we have */
	for (ability = player_abilities; ability; ability = ability->next) {
		if (class_has_ability(player->class, ability)) {
			memcpy(&ability_list[num_abilities], ability,
				   sizeof(struct player_ability));
			ability_list[num_abilities++].group = PLAYER_FLAG_CLASS;
		}
	}

	/* Count the number of race powers we have */
	for (ability = player_abilities; ability; ability = ability->next) {
		if (race_has_ability(player->race, ability)) {
			memcpy(&ability_list[num_abilities], ability,
				   sizeof(struct player_ability));
			ability_list[num_abilities++].group = PLAYER_FLAG_RACE;
		}
	}

	/* View choices until user exits */
	view_ability_menu();

	/* exit */
	return;
}


/**
 * Interact with abilities -BR-
 */
void do_cmd_abilities(void)
{
	ui_event answer;

	/* Might want to gain a new ability or browse old ones */
	if (player->upkeep->new_specialties > 0) {
		/* Interact and choose. */
		while (get_com_ex
			   ("View abilities or Learn specialty (l/v/ESC)?", &answer)) {
			/* New ability */
			if ((answer.key.code == 'L') || (answer.key.code == 'l')) {
				gain_specialty();
				break;
			}

			/* View Current */
			if ((answer.key.code == 'V') || (answer.key.code == 'v')) {
				view_abilities();
				break;
			}

			/* Exit */
			else if (answer.key.code == ESCAPE)
				break;


		}
	} else {
		/* View existing specialties is the only option */
		view_abilities();
		return;
	}

	return;
}
