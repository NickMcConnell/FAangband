/**
 * \file ui-recall.c
 * \brief Recall point selection menu
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2019 Nick McConnell
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
#include "game-world.h"
#include "init.h"
#include "ui-menu.h"
#include "ui-output.h"

/**
 * Choice of recall point (or nowhere)
 */
static int selection;

/**
 * Number of actual recall points
 */
static int num_recall_points;

static char recall_tag(struct menu *menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the recall menu
 */
void recall_display(struct menu *menu, int oid, bool cursor, int row,
					int col, int width)
{
	const u16b *choice = menu_priv(menu);
	int idx = choice[oid];
	char place[30];

	byte attr = (cursor ? COLOUR_L_BLUE : COLOUR_WHITE);

	if (idx < num_recall_points) {
		int region = world->levels[player->recall[idx]].locality;
		int level = world->levels[player->recall[idx]].depth;

		/* Get the recall point description */
		if (level) {
			my_strcpy(place, format("%s %d   ", locality_name(region), level),
					  sizeof(place));
		} else if (region != LOC_ARENA) {
			my_strcpy(place, format("%s Town   ", locality_name(region)),
					  sizeof(place));
		} else {
			my_strcpy(place, "Nowhere     ", sizeof(place));
		}
	} else {
		my_strcpy(place, "Don't replace     ", sizeof(place));
	}

	/* Print it */
	c_put_str(attr, format("%s", place), row, col);
}

/**
 * Deal with events on the recall menu
 */
bool recall_action(struct menu *menu, const ui_event *e, int oid)
{
	u16b *choice = menu_priv(menu);
	if (e->type == EVT_SELECT) {
		selection = choice[oid];
	}

	return false;
}


/**
 * Display list of recall points, pick one or decline.
 */
int textui_get_recall_point(bool inward, int num_points, int num_poss)
{
	menu_iter menu_f = { recall_tag, 0, recall_display, recall_action, 0 };
	struct menu *m = menu_new(MN_SKIN_SCROLL, &menu_f);
	region area = { 15, 1, 48, -1 };
	ui_event evt = { 0 };
	int cursor = 0;
	int num_entries;

	int i;

	u16b *choice;

	if (inward && (num_points < num_poss))
		num_points++;
	if (inward) {
		num_entries = num_points + 1;
	} else {
		num_entries = num_points;
	}
	num_recall_points = num_points;

	/* Create the array */
	choice = mem_zalloc(num_entries * sizeof(u16b));

	/* Obvious */
	for (i = 0; i < num_entries; i++) {
		choice[i] = i;
	}

	/* Clear space */
	area.page_rows = num_entries + 2;

	/* Return here if there is nowhere to recall to */
	if (!num_entries) {
		mem_free(choice);
		mem_free(m);
		return -1;
	}

	/* Set up the item list variables */
	selection = 0;

	/* Set up the menu */
	if (inward) {
		m->title = "Which recall point will you replace (or ESC):";
	} else {
		m->title = "Which recall point do you want to go to?";
	}
	menu_setpriv(m, num_entries, choice);
	menu_layout(m, &area);

	/* Select an entry */
	evt = menu_select(m, cursor, true);

	/* Free memory */
	mem_free(choice);
	mem_free(m);

	return (evt.type != EVT_ESCAPE) ? selection : -1;
}
