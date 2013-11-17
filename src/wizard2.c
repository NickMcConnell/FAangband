/** \file wizard2.c 
    \brief Cheat modes

 * The wizard & debugging commands and their effects.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "cave.h"
#include "cmds.h"
#include "files.h"
#include "monster.h"
#include "spells.h"
#include "target.h"
#include "ui-menu.h"


#ifdef ALLOW_DEBUG


/**
 * Debug scent trails and noise bursts.
 */
static void do_cmd_wiz_hack_ben(void)
{

	struct keypress cmd;

	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, y, x, y2, x2;

	/* Get a "debug command" */
	if (!get_com("Press 'S' for scent, 'N' for noise info: ", &cmd))
		return;


	/* Analyze the command */
	switch (cmd.code) {
	case 'S':
	case 's':
		{
			/* Update map */
			for (y = Term->offset_y; y <= Term->offset_y + SCREEN_HGT; y++) {
				for (x = Term->offset_x; x <= Term->offset_x + SCREEN_WID;
					 x++) {
					byte a;

					int age = get_scent(y, x);

					/* Must have scent */
					if (age == -1)
						continue;

					/* Pretty colors by age */
					if (age > SMELL_STRENGTH)
						a = TERM_L_DARK;

					else if (age < 10)
						a = TERM_BLUE;
					else if (age < 20)
						a = TERM_L_BLUE;
					else if (age < 30)
						a = TERM_GREEN;
					else if (age < 40)
						a = TERM_L_GREEN;
					else if (age < 50)
						a = TERM_YELLOW;
					else if (age < 60)
						a = TERM_ORANGE;
					else if (age < 70)
						a = TERM_L_RED;
					else
						a = TERM_RED;


					/* Display player/floors/walls */
					if ((y == py) && (x == px)) {
						print_rel('@', a, y, x);
					} else {
						print_rel('0' + (age % 10), a, y, x);
					}
				}
			}

			/* Prompt */
			prt("Scent ages", 0, 0);

			/* Wait for a keypress */
			(void) inkey();

			/* Redraw map */
			prt_map();

			break;

		}

	case 'N':
	case 'n':
		{

			/* Get a "debug command" */
			if (!get_com
				("Press 'D' for direction of flow, 'C' for actual cost values: ",
				 &cmd))
				return;

			if ((cmd.code == 'D') || (cmd.code == 'd')) {
				/* Update map */
				for (y = Term->offset_y; y <= Term->offset_y + SCREEN_HGT;
					 y++) {
					for (x = Term->offset_x;
						 x <= Term->offset_x + SCREEN_WID; x++) {
						int lowest_cost = cave_cost[y][x];
						int dir = -1;
						int cost;

						if (lowest_cost == 0)
							continue;

						for (i = 0; i < 8; i++) {
							/* Get the location */
							y2 = y + ddy_ddd[i];
							x2 = x + ddx_ddd[i];

							cost = cave_cost[y2][x2];
							if (!cost)
								continue;

							/* If this grid's scent is younger, save it */
							if (lowest_cost > cost)
								lowest_cost = cost;

							/* If it isn't, look elsewhere */
							else
								continue;

							/* Save this direction */
							dir = i;
						}

						/* If we didn't find any younger scent, print a '5' */
						if (dir == -1)
							print_rel('5', TERM_YELLOW, y, x);

						/* Otherwise, convert to true direction and print */
						else {
							i = ddd[dir];
							print_rel('0' + i, TERM_L_BLUE, y, x);
						}
					}
				}

				/* Prompt */
				prt("Directions given to advancing monsters using noise info", 0, 0);

				/* Wait for a keypress */
				(void) inkey();

				/* Redraw map */
				prt_map();
			}

			/* Actual cost values */
			else {
				int j;
				struct keypress key;

				for (i = cost_at_center - 2; i <= 100 + NOISE_STRENGTH;
					 ++i) {
					/* First show grids with no scent */
					if (i == cost_at_center - 2)
						j = 0;

					/* Then show specially marked grids (bug-checking) */
					else if (i == cost_at_center - 1)
						j = 255;

					/* Then show standard grids */
					else
						j = i;

					/* Update map */
					for (y = Term->offset_y;
						 y <= Term->offset_y + SCREEN_HGT; y++) {
						for (x = Term->offset_x;
							 x <= Term->offset_x + SCREEN_WID; x++) {
							byte a = TERM_YELLOW;
							feature_type *f_ptr = &f_info[cave_feat[y][x]];

							/* Display proper cost */
							if (cave_cost[y][x] != j)
								continue;

							/* Display player/floors/walls */
							if ((y == py) && (x == px)) {
								print_rel('@', a, y, x);
							} else if (!tf_has(f_ptr->flags, TF_NO_SCENT)) {
								print_rel('*', a, y, x);
							} else {
								print_rel('#', a, y, x);
							}
						}
					}

					/* Prompt */
					if (j == 0) {
						prt("Grids with no scent", 0, 0);
					} else if (j == 255) {
						prt("Specially marked grids", 0, 0);
					} else {
						prt(format("Depth %d: ", j), 0, 0);
					}

					/* Get key */
					key = inkey();
					if (key.code == ESCAPE)
						break;

					/* Redraw map */
					prt_map();
				}
			}

			break;
		}

	default:
		{
			break;
		}
	}

	/* Done */
	prt("", 0, 0);

	/* Redraw map */
	prt_map();
}



/**
 * Output part of a bitflag set in binary format.
 */
static void prt_binary(const bitflag * flags, int offset, int row, int col,
					   char ch, int num)
{
	int flag;

	/* Scan the flags */
	for (flag = FLAG_START + offset; flag < FLAG_START + offset + num;
		 flag++) {
		if (of_has(flags, flag))
			Term_putch(col++, row, TERM_BLUE, ch);
		else
			Term_putch(col++, row, TERM_WHITE, '-');
	}
}


/**
 * Hack -- Teleport to the target.  Oangband asks for a target after 
 * the command.
 */
static void do_cmd_wiz_bamf(void)
{
	feature_type *f_ptr;

	/* target starts at player. */
	s16b ny = 0;
	s16b nx = 0;

	/* Use the targeting function. */
	if (!target_set_interactive(TARGET_LOOK, -1, -1))
		return;

	/* grab the target coords. */
	target_get(&nx, &ny);

	/* Test for passable terrain. */
	f_ptr = &f_info[cave_feat[ny][nx]];
	if (!tf_has(f_ptr->flags, TF_PASSABLE)) {
		msg("The square you are aiming for is impassable.");
	}

	/* The simple act of controlled teleport. */
	else
		teleport_player_to(ny, nx, TRUE);
}



/**
 * Aux function for "do_cmd_wiz_change()"
 */
static void do_cmd_wiz_change_aux(void)
{
	int i;

	int tmp_int;

	long tmp_long;

	char tmp_val[160];

	char ppp[80];


	/* Query the stats */
	for (i = 0; i < A_MAX; i++) {
		/* Prompt */
		sprintf(ppp, "%s (3-118): ", stat_names[i]);

		/* Default */
		sprintf(tmp_val, "%d", p_ptr->stat_max[i]);

		/* Query */
		if (!get_string(ppp, tmp_val, 4))
			return;

		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Verify */
		if (tmp_int > 18 + 100)
			tmp_int = 18 + 100;
		else if (tmp_int < 3)
			tmp_int = 3;

		/* Save it */
		p_ptr->stat_cur[i] = p_ptr->stat_max[i] = tmp_int;
	}


	/* Default */
	sprintf(tmp_val, "%ld", (long) (p_ptr->au));

	/* Query */
	if (!get_string("Gold: ", tmp_val, 10))
		return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0)
		tmp_long = 0L;

	/* Save */
	p_ptr->au = tmp_long;

	/* Default */
	sprintf(tmp_val, "%ld", (long) (p_ptr->max_exp));

	/* Query */
	if (!get_string("Max Experience: ", tmp_val, 10))
		return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0)
		tmp_long = 0L;

	/* Save */
	p_ptr->max_exp = tmp_long;

	/* Set experience to new maximum. */
	p_ptr->exp = p_ptr->max_exp;

	/* Update */
	check_experience();

	/* Default */
	sprintf(tmp_val, "%ld", (long) (p_ptr->exp));

	/* Query */
	if (!get_string("Experience: ", tmp_val, 10))
		return;

	/* Extract */
	tmp_long = atol(tmp_val);

	/* Verify */
	if (tmp_long < 0)
		tmp_long = 0L;

	/* Save */
	p_ptr->exp = tmp_long;

	/* Update */
	check_experience();

}


/**
 * Change various "permanent" player variables.
 */
static void do_cmd_wiz_change(void)
{
	/* Interact */
	do_cmd_wiz_change_aux();

	/* Redraw everything */
	do_cmd_redraw();
}


/**
 * Wizard routines for creating objects and modifying them
 *
 * This has been rewritten to make the whole procedure
 * of debugging objects much easier and more comfortable.
 *
 * Here are the low-level functions
 *
 * - wiz_display_item()
 *     display an item's debug-info
 * - wiz_create_itemtype()
 *     specify tval and sval (type and subtype of object)
 * - wiz_tweak_item()
 *     specify pval, +AC, +tohit, +todam
 *     Note that the wizard can leave this function anytime,
 *     thus accepting the default-values for the remaining values.
 *     pval comes first now, since it is most important.
 * - wiz_reroll_item()
 *     apply some magic to the item or turn it into an artifact.
 * - wiz_roll_item()
 *     Get some statistics about the rarity of an item:
 *     We create a lot of fake items and see if they are of the
 *     same type (tval and sval), then we compare pval and +AC.
 *     If the fake-item is better or equal it is counted.
 *     Note that cursed items that are better or equal (absolute values)
 *     are counted, too.
 *     HINT: This is *very* useful for balancing the game!
 * - wiz_quantity_item()
 *     change the quantity of an item, but be sane about it.
 *
 * And now the high-level functions
 * - do_cmd_wiz_play()
 *     play with an existing object
 * - wiz_create_item()
 *     create a new object
 *
 * Note -- You do not have to specify "pval" and other item-properties
 * directly. Just apply magic until you are satisfied with the item.
 *
 * Note -- For some items (such as wands, staffs, some rings, etc), you
 * must apply magic, or you will get "broken" or "uncharged" objects.
 *
 * Note -- Redefining artifacts via "do_cmd_wiz_play()" may destroy
 * the artifact.  Be careful.
 *
 * Hack -- this function will allow you to create multiple artifacts.
 * This "feature" may induce crashes or other nasty effects.
 */


/**
 * Display an item's properties
 */
static void wiz_display_item(object_type * o_ptr)
{
	int j = 0;

	char buf[256];


	/* Clear screen */
	Term_clear();

	/* Describe fully */
	object_desc(buf, sizeof(buf), o_ptr,
				ODESC_PREFIX | ODESC_FULL | ODESC_SPOIL);
	prt(buf, 2, j);

	prt(format
		("kind = %-5d  level = %-4d  tval = %-5d  sval = %-5d",
		 o_ptr->k_idx, k_info[o_ptr->k_idx].level, o_ptr->tval,
		 o_ptr->sval), 4, j);

	prt(format
		("number = %-3d  wgt = %-6d  ac = %-5d    damage = %dd%d",
		 o_ptr->number, o_ptr->weight, o_ptr->ac, o_ptr->dd, o_ptr->ds), 5,
		j);

	prt(format
		("pval = %-5d  toac = %-5d  tohit = %-4d  todam = %-4d",
		 o_ptr->pval, o_ptr->to_a, o_ptr->to_h, o_ptr->to_d), 6, j);

	prt(format
		("name1 = %-4d  name2 = %-4d  cost = %ld", o_ptr->name1,
		 o_ptr->name2, (long) object_value(o_ptr)), 7, j);

	prt(format
		("ident = %04x  timeout = %-d", o_ptr->ident, o_ptr->timeout), 8,
		j);

	prt("+------------FLAGS_------------+", 10, j);
	prt("..SUST..POWERS....OTHER.........", 11, j);
	prt("                                ", 12, j);
	prt("tbsiwdcc  re   s  ttbiaefc   f..", 13, j);
	prt("hatnieohsfessfhefphhlmcliosspr..", 14, j);
	prt("rlrtsxnadfgpialerlrdspppppmcca..", 15, j);
	prt_binary(o_ptr->flags_obj, 0, 16, j, '*', 32);

	prt("+------------CURSES------------+", 17, j);
	prt("	n r    b b       b             ", 18, j);
	prt("ttaasahppcuhdaassppdddd.........", 19, j);
	prt("eeggrfuooutawduttaaxssc.........", 20, j);
	prt("llggerniitrlemncwrrppth.........", 21, j);
	prt_binary(o_ptr->flags_curse, 0, 22, j, '*', 32);
	prt("Resists, bonuses and slays coming soon...", 23, j);
}

/** Object creation code **/
bool choose_artifact = FALSE;

/**
 * A structure to hold a tval, its description and its possibility 
 <  * for becoming an artifact.
*/
typedef struct tval_desc {
	int tval;
	const char *desc;
	bool can_be_artifact;

} tval_desc;

/**
 * A list of tvals, their textual names, and possibility for becoming an
 * artifact.
 */
static tval_desc tvals[] = {
	{TV_SWORD, "Sword", TRUE},
	{TV_POLEARM, "Polearm", TRUE},
	{TV_HAFTED, "Hafted Weapon", TRUE},
	{TV_BOW, "Missile Weapon", TRUE},
	{TV_ARROW, "Arrows", FALSE},
	{TV_BOLT, "Bolts", FALSE},
	{TV_SHOT, "Shots", FALSE},
	{TV_SHIELD, "Shield", TRUE},
	{TV_CROWN, "Crown", TRUE},
	{TV_HELM, "Helm", TRUE},
	{TV_GLOVES, "Gloves", TRUE},
	{TV_BOOTS, "Boots", TRUE},
	{TV_CLOAK, "Cloak", TRUE},
	{TV_DRAG_ARMOR, "Dragon Scale Mail", TRUE},
	{TV_HARD_ARMOR, "Hard Armor", TRUE},
	{TV_SOFT_ARMOR, "Soft Armor", TRUE},
	{TV_RING, "Ring", FALSE},
	{TV_AMULET, "Amulet", TRUE},
	{TV_LIGHT, "Light", TRUE},
	{TV_POTION, "Potion", FALSE},
	{TV_SCROLL, "Scroll", FALSE},
	{TV_WAND, "Wand", TRUE},
	{TV_STAFF, "Staff", TRUE},
	{TV_ROD, "Rod", TRUE},
	{TV_PRAYER_BOOK, "Priest Book", FALSE},
	{TV_MAGIC_BOOK, "Magic Book", FALSE},
	{TV_DRUID_BOOK, "Druid Stone", FALSE},
	{TV_NECRO_BOOK, "Necromantic Tome", FALSE},
	{TV_SPIKE, "Spikes", FALSE},
	{TV_DIGGING, "Digger", FALSE},
	{TV_CHEST, "Chest", FALSE},
	{TV_FOOD, "Food", FALSE},
	{TV_FLASK, "Flask", FALSE},
	{TV_GOLD, "treasure", FALSE}
};


/**
 * Build an "artifact name" and transfer it into a buffer.
 */
static void get_art_name(char *buf, int a_idx)
{
	int i;
	object_type localObject;
	object_type *o_ptr;
	artifact_type *a_ptr = &a_info[a_idx];


	/* Get local object */
	o_ptr = &localObject;

	/* Wipe the object */
	object_wipe(o_ptr);

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
	if (!i)
		return;

	/* Create the base object */
	object_prep(o_ptr, i, RANDOMISE);

	/* Mark it as an artifact */
	o_ptr->name1 = a_idx;

	/* Make it known to us */
	o_ptr->ident |= IDENT_KNOWN;

	/* Create the artifact description */
	object_desc(buf, 60, o_ptr, ODESC_SINGULAR | ODESC_SPOIL);
}

static const region wiz_create_item_area = { 0, 0, 0, 0 };

void wiz_create_item_subdisplay(menu_type * m, int oid, bool cursor,
								int row, int col, int width)
{
	int *choices = menu_priv(m);
	char buf[80];

	/* Artifacts */
	if (choose_artifact) {
		get_art_name(buf, choices[oid]);
		c_prt(curs_attrs[CURS_KNOWN][0 != cursor], buf, row, col);
	}
	/* Regular objects */
	else {
		object_kind_name(buf, sizeof buf, choices[oid], TRUE);
		c_prt(curs_attrs[CURS_KNOWN][0 != cursor], buf, row, col);
	}
}

bool wiz_create_item_subaction(menu_type * m, const ui_event * e, int oid)
{
	int *choices = menu_priv(m);

	object_type *i_ptr;
	object_type object_type_body;

	/* Artifacts */
	if (choose_artifact) {
		int i;
		int o_idx;
		artifact_type *a_ptr = &a_info[choices[oid]];

		/* Get the artifact info */
		//a_ptr = &a_info[choices[oid]];

		/* Ignore "empty" artifacts */
		if (!a_ptr->name)
			return TRUE;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Acquire the "kind" index */
		o_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Create the base object */
		object_prep(i_ptr, o_idx, RANDOMISE);

		/* Mark the object as an artifact. */
		i_ptr->name1 = choices[oid];

		/* Extract the fields */
		i_ptr->pval = a_ptr->pval;
		i_ptr->ac = a_ptr->ac;
		i_ptr->dd = a_ptr->dd;
		i_ptr->ds = a_ptr->ds;
		i_ptr->to_a = a_ptr->to_a;
		i_ptr->to_h = a_ptr->to_h;
		i_ptr->to_d = a_ptr->to_d;
		i_ptr->weight = a_ptr->weight;

		of_copy(i_ptr->flags_obj, a_ptr->flags_obj);
		cf_copy(i_ptr->flags_curse, a_ptr->flags_curse);

		for (i = 0; i < MAX_P_RES; i++)
			i_ptr->percent_res[i] = a_ptr->percent_res[i];
		for (i = 0; i < A_MAX; i++)
			i_ptr->bonus_stat[i] = a_ptr->bonus_stat[i];
		for (i = 0; i < MAX_P_BONUS; i++)
			i_ptr->bonus_other[i] = a_ptr->bonus_other[i];
		for (i = 0; i < MAX_P_SLAY; i++)
			i_ptr->multiple_slay[i] = a_ptr->multiple_slay[i];
		for (i = 0; i < MAX_P_BRAND; i++)
			i_ptr->multiple_brand[i] = a_ptr->multiple_brand[i];


		/* Transfer the activation information. */
		if (a_ptr->effect)
			i_ptr->effect = a_ptr->effect;
	}
	/* Regular objects */
	else {
		object_kind *kind = &k_info[choices[oid]];

		if (e->type != EVT_SELECT)
			return TRUE;

		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Create the item */
		object_prep(i_ptr, kind->kidx, RANDOMISE);

		/* Apply magic (no messages, no artifacts) */
		apply_magic(i_ptr, p_ptr->depth, FALSE, FALSE, FALSE);

		/* Hack -- Since treasure objects are not effected by apply_magic, they
		 * need special processing. */
		if (i_ptr->tval == TV_GOLD) {
			i_ptr->pval = kind->cost / 2 + randint1((kind->cost + 1) / 2);
		}

		/* Mark as cheat, and where created */
		i_ptr->origin = ORIGIN_CHEAT;
		i_ptr->origin_stage = p_ptr->stage;

	}

	/* Drop from heaven */
	drop_near(i_ptr, -1, p_ptr->py, p_ptr->px, TRUE);

	/* All done */
	msg("Allocated.");

	return FALSE;
}

menu_iter wiz_create_item_submenu = {
	NULL,
	NULL,
	wiz_create_item_subdisplay,
	wiz_create_item_subaction,
	NULL
};

void wiz_create_item_display(menu_type * m, int oid, bool cursor,
							 int row, int col, int width)
{
	struct tval_desc *tvals = menu_priv(m);
	c_prt(curs_attrs[CURS_KNOWN][0 != cursor], tvals[oid].desc, row, col);
}

bool wiz_create_item_action(menu_type * m, const ui_event * e, int oid)
{
	ui_event ret;
	menu_type *menu;

	int choice[60];
	int n_choices;

	int i;
	char buf[60];

	if (e->type != EVT_SELECT)
		return TRUE;

	/* Artifacts */
	if (choose_artifact) {
		struct tval_desc *a_tvals = menu_priv(m);
		/* ...We have to search the whole artifact list. */
		for (n_choices = 0, i = 1; (n_choices < 60) && (i < z_info->a_max);
			 i++) {
			artifact_type *a_ptr = &a_info[i];

			/* Analyze matching items */
			if (a_ptr->tval == a_tvals[oid].tval) {
				/* Remember the artifact index */
				choice[n_choices++] = i;
			}
		}
	}
	/* Regular objects */
	else {
		for (n_choices = 0, i = 1; (n_choices < 60) && (i < z_info->k_max);
			 i++) {
			object_kind *kind = &k_info[i];

			if (kind->tval != tvals[oid].tval ||
				kf_has(kind->flags_kind, KF_INSTA_ART))
				continue;

			choice[n_choices++] = i;
		}
	}

	screen_save();
	clear_from(0);

	menu = menu_new(MN_SKIN_COLUMNS, &wiz_create_item_submenu);
	menu->selections = all_letters;
	if (m->count == N_ELEMENTS(tvals))
		strnfmt(buf, sizeof(buf), "What kind of %s?", tvals[oid].desc);
	else
		strnfmt(buf, sizeof(buf), "Which artifact %s? ", tvals[oid].desc);

	menu->title = buf;
	menu_setpriv(menu, n_choices, choice);
	menu_layout(menu, &wiz_create_item_area);
	ret = menu_select(menu, 0, FALSE);

	screen_load();

	return (ret.type == EVT_ESCAPE);
}

menu_iter wiz_create_item_menu = {
	NULL,
	NULL,
	wiz_create_item_display,
	wiz_create_item_action,
	NULL
};


/*
 * Choose and create an instance of an object kind
 */
static void wiz_create_item(void)
{
	menu_type *menu = menu_new(MN_SKIN_COLUMNS, &wiz_create_item_menu);

	choose_artifact = FALSE;

	menu->selections = all_letters;
	menu->title = "What kind of object?";

	screen_save();
	clear_from(0);

	menu_setpriv(menu, N_ELEMENTS(tvals), tvals);
	menu_layout(menu, &wiz_create_item_area);
	menu_select(menu, 0, FALSE);

	screen_load();
}

/*
 * Choose and create an instance of an object kind
 */
static void wiz_create_artifact(void)
{
	size_t num, i;
	menu_type *menu = menu_new(MN_SKIN_COLUMNS, &wiz_create_item_menu);
	tval_desc *a_tvals;

	choose_artifact = TRUE;

	a_tvals = C_ZNEW(N_ELEMENTS(tvals), tval_desc);

	for (num = i = 0; i < N_ELEMENTS(tvals); i++) {
		/* Don't show tvals with no artifacts. */
		if (!(tvals[i].can_be_artifact))
			continue;

		/* Increment number of items in list. */
		a_tvals[num++] = tvals[i];
	}

	menu->selections = all_letters;
	menu->title = "What kind of artifact?";

	screen_save();
	clear_from(0);

	menu_setpriv(menu, num, a_tvals);
	menu_layout(menu, &wiz_create_item_area);
	menu_select(menu, 0, FALSE);

	screen_load();
	FREE(a_tvals);
}


/**
 * Tweak an item
 */
static void wiz_tweak_item(object_type * o_ptr)
{
	const char *p;
	char tmp_val[80];


	/* Hack -- leave artifacts alone */
	if (artifact_p(o_ptr))
		return;

	p = "Enter new 'pval' setting: ";
	sprintf(tmp_val, "%d", o_ptr->pval);
	if (!get_string(p, tmp_val, 6))
		return;
	o_ptr->pval = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_a' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_a);
	if (!get_string(p, tmp_val, 6))
		return;
	o_ptr->to_a = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_h' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_h);
	if (!get_string(p, tmp_val, 6))
		return;
	o_ptr->to_h = atoi(tmp_val);
	wiz_display_item(o_ptr);

	p = "Enter new 'to_d' setting: ";
	sprintf(tmp_val, "%d", o_ptr->to_d);
	if (!get_string(p, tmp_val, 6))
		return;
	o_ptr->to_d = atoi(tmp_val);
	wiz_display_item(o_ptr);
}


/**
 * Apply magic to an item or turn it into an artifact. -Bernd-
 */
static void wiz_reroll_item(object_type * o_ptr)
{
	object_type *i_ptr;
	object_type object_type_body;

	struct keypress ch;

	bool changed = FALSE;


	/* Hack -- leave artifacts alone */
	if (artifact_p(o_ptr))
		return;


	/* Get local object */
	i_ptr = &object_type_body;

	/* Copy the object */
	object_copy(i_ptr, o_ptr);


	/* Main loop. Ask for magification and artifactification */
	while (TRUE) {
		/* Display full item debug information */
		wiz_display_item(i_ptr);

		/* Ask wizard what to do. */
		if (!get_com("[a]ccept, [n]ormal, [g]ood, [e]xcellent? ", &ch)) {
			changed = FALSE;
			break;
		}

		/* Create/change it! */
		if (ch.code == 'A' || ch.code == 'a') {
			changed = TRUE;
			break;
		}

		/* Apply normal magic, but first clear object */
		else if (ch.code == 'n' || ch.code == 'N') {
			object_prep(i_ptr, o_ptr->k_idx, RANDOMISE);
			apply_magic(i_ptr, p_ptr->depth, FALSE, FALSE, FALSE);
		}

		/* Apply good magic, but first clear object */
		else if (ch.code == 'g' || ch.code == 'g') {
			object_prep(i_ptr, o_ptr->k_idx, RANDOMISE);
			apply_magic(i_ptr, p_ptr->depth, FALSE, TRUE, FALSE);
		}

		/* Apply great magic, but first clear object */
		else if (ch.code == 'e' || ch.code == 'e') {
			object_prep(i_ptr, o_ptr->k_idx, RANDOMISE);
			apply_magic(i_ptr, p_ptr->depth, FALSE, TRUE, TRUE);
		}
	}


	/* Notice change */
	if (changed) {
		/* Apply changes */
		object_copy(o_ptr, i_ptr);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
	}
}



/**
 * Maximum number of rolls
 */
#define TEST_ROLL 100000


/**
 * Try to create an item again. Output some statistics. -Bernd-
 *
 * The statistics are correct now.  We acquire a clean grid, and then
 * repeatedly place an object in this grid, copying it into an item
 * holder, and then deleting the object.  We fiddle with the artifact
 * counter flags to prevent weirdness.  We use the items to collect
 * statistics on item creation relative to the initial item.
 */
static void wiz_statistics(object_type * o_ptr)
{
	long i, matches, better, worse, other;

	struct keypress ch;
	char *quality;

	bool good, great;

	object_type *i_ptr;
	object_type object_type_body;

	const char *q =
		"Rolls: %ld, Matches: %ld, Better: %ld, Worse: %ld, Other: %ld";


	/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
	if (artifact_p(o_ptr))
		a_info[o_ptr->name1].created = FALSE;


	/* Interact */
	while (TRUE) {
		const char *pmt =
			"Roll for [n]ormal, [g]ood, or [e]xcellent treasure? ";

		/* Display item */
		wiz_display_item(o_ptr);

		/* Get choices */
		if (!get_com(pmt, &ch))
			break;

		if (ch.code == 'n' || ch.code == 'N') {
			good = FALSE;
			great = FALSE;
			quality = "normal";
		} else if (ch.code == 'g' || ch.code == 'G') {
			good = TRUE;
			great = FALSE;
			quality = "good";
		} else if (ch.code == 'e' || ch.code == 'E') {
			good = TRUE;
			great = TRUE;
			quality = "excellent";
		} else {
			good = FALSE;
			great = FALSE;
			break;
		}

		/* Let us know what we are doing */
		msg("Creating a lot of %s items. Base level = %d.", quality,
			p_ptr->depth);
		message_flush();

		/* Set counters to zero */
		matches = better = worse = other = 0;

		/* Let's rock and roll */
		for (i = 0; i <= TEST_ROLL; i++) {
			/* Output every few rolls */
			if ((i < 100) || (i % 100 == 0)) {
				struct keypress kp;

				/* Do not wait */
				inkey_scan = SCAN_INSTANT;

				/* Allow interupt */
				kp = inkey();
				if (kp.type != EVT_NONE) {
					flush();
					break;
				}

				/* Dump the stats */
				prt(format(q, i, matches, better, worse, other), 0, 0);
				Term_fresh();
			}


			/* Get local object */
			i_ptr = &object_type_body;

			/* Wipe the object */
			object_wipe(i_ptr);

			/* Create an object */
			make_object(i_ptr, good, great, FALSE);


			/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
			if (artifact_p(i_ptr))
				a_info[i_ptr->name1].created = FALSE;


			/* Test for the same tval and sval. */
			if ((o_ptr->tval) != (i_ptr->tval))
				continue;
			if ((o_ptr->sval) != (i_ptr->sval))
				continue;

			/* Check for match */
			if ((i_ptr->pval == o_ptr->pval)
				&& (i_ptr->to_a == o_ptr->to_a)
				&& (i_ptr->to_h == o_ptr->to_h)
				&& (i_ptr->to_d == o_ptr->to_d)) {
				matches++;
			}

			/* Check for better */
			else if ((i_ptr->pval >= o_ptr->pval)
					 && (i_ptr->to_a >= o_ptr->to_a)
					 && (i_ptr->to_h >= o_ptr->to_h)
					 && (i_ptr->to_d >= o_ptr->to_d)) {
				better++;
			}

			/* Check for worse */
			else if ((i_ptr->pval <= o_ptr->pval)
					 && (i_ptr->to_a <= o_ptr->to_a)
					 && (i_ptr->to_h <= o_ptr->to_h)
					 && (i_ptr->to_d <= o_ptr->to_d)) {
				worse++;
			}

			/* Assume different */
			else {
				other++;
			}
		}

		/* Final dump */
		msg(q, i, matches, better, worse, other);
		message_flush();
	}


	/* Hack -- Normally only make a single artifact */
	if (artifact_p(o_ptr))
		a_info[o_ptr->name1].created = TRUE;
}


/**
 * Change the quantity of an item
 */
static void wiz_quantity_item(object_type * o_ptr)
{
	int tmp_int, tmp_qnt;

	char tmp_val[100];


	/* Never duplicate artifacts */
	if (artifact_p(o_ptr))
		return;

	/* Store old quantity. */
	tmp_qnt = o_ptr->number;

	/* Default */
	sprintf(tmp_val, "%d", o_ptr->number);

	/* Query */
	if (get_string("Quantity: ", tmp_val, 3)) {
		/* Extract */
		tmp_int = atoi(tmp_val);

		/* Paranoia */
		if (tmp_int < 1)
			tmp_int = 1;
		if (tmp_int > 99)
			tmp_int = 99;

		/* Accept modifications */
		o_ptr->number = tmp_int;

		/* Hack -- Rod total timeouts increase with stack size. */
		if (o_ptr->tval == TV_ROD)
			o_ptr->pval = o_ptr->pval * o_ptr->number / tmp_qnt;
	}
}



/**
 * Play with an item. Options include:
 *   - Output statistics (via wiz_roll_item)
 *   - Reroll item (via wiz_reroll_item)
 *   - Change properties (via wiz_tweak_item)
 *   - Change the number of items (via wiz_quantity_item)
 */
static void do_cmd_wiz_play(void)
{
	int item;

	object_type *i_ptr;
	object_type object_type_body;

	object_type *o_ptr;

	struct keypress ch;

	const char *q, *s;

	bool changed;


	/* Get an item */
	q = "Play with which object? ";
	s = "You have nothing to play with.";
	if (!get_item(&item, q, s, 0, (USE_EQUIP | USE_INVEN | USE_FLOOR)))
		return;

	/* Get the item (in the pack) */
	if (item >= 0) {
		o_ptr = &p_ptr->inventory[item];
	}

	/* Get the item (on the floor) */
	else {
		o_ptr = &o_list[0 - item];
	}


	/* The item was not changed */
	changed = FALSE;


	/* Save screen */
	screen_save();


	/* Get local object */
	i_ptr = &object_type_body;

	/* Copy object */
	object_copy(i_ptr, o_ptr);


	/* The main loop */
	while (TRUE) {
		/* Display the item */
		wiz_display_item(i_ptr);

		/* Get choice */
		if (!get_com
			("[a]ccept [s]tatistics [r]eroll [t]weak [q]uantity? ", &ch)) {
			changed = FALSE;
			break;
		}

		if (ch.code == 'A' || ch.code == 'a') {
			changed = TRUE;
			break;
		}

		if (ch.code == 's' || ch.code == 'S') {
			wiz_statistics(i_ptr);
		}

		if (ch.code == 'r' || ch.code == 'R') {
			wiz_reroll_item(i_ptr);
		}

		if (ch.code == 't' || ch.code == 'T') {
			wiz_tweak_item(i_ptr);
		}

		if (ch.code == 'q' || ch.code == 'Q') {
			wiz_quantity_item(i_ptr);
		}
	}


	/* Load screen */
	screen_load();


	/* Accept change */
	if (changed) {
		/* Message */
		msg("Changes accepted.");

		/* Change */
		object_copy(o_ptr, i_ptr);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER | PN_SORT_QUIVER);

		/* Redraw stuff */
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
	}

	/* Ignore change */
	else {
		msg("Changes ignored.");
	}
}


/**
 * Cure everything instantly
 */
static void do_cmd_wiz_cure_all(void)
{
	/* Restore stats */
	(void) res_stat(A_STR);
	(void) res_stat(A_INT);
	(void) res_stat(A_WIS);
	(void) res_stat(A_CON);
	(void) res_stat(A_DEX);
	(void) res_stat(A_CHR);

	/* Restore the level */
	(void) restore_level();

	/* Update stuff (if needed) */
	if (p_ptr->update)
		update_stuff(p_ptr);

	/* Heal the player */
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;

	/* Restore mana */
	p_ptr->csp = p_ptr->msp;
	p_ptr->csp_frac = 0;

	/* Cure stuff */
	(void) clear_timed(TMD_BLIND, TRUE);
	(void) clear_timed(TMD_CONFUSED, TRUE);
	(void) clear_timed(TMD_POISONED, TRUE);
	(void) clear_timed(TMD_AFRAID, TRUE);
	(void) clear_timed(TMD_PARALYZED, TRUE);
	(void) clear_timed(TMD_IMAGE, TRUE);
	(void) clear_timed(TMD_STUN, TRUE);
	(void) clear_timed(TMD_CUT, TRUE);
	(void) clear_timed(TMD_SLOW, TRUE);
	p_ptr->black_breath = FALSE;

	/* No longer hungry */
	(void) set_food(PY_FOOD_MAX - 1);

	/* Redraw everything */
	do_cmd_redraw();
}


/* Jump menu code */

/**
 * Choice of location 
 */
static int place = 0;

/**
 * Label for location
 */
static char jump_tag(menu_type * menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the jump menu
 */
void jump_display(menu_type * menu, int oid, bool cursor, int row, int col,
				  int width)
{
	const u16b *choice = menu_priv(menu);

	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);

	c_prt(attr, locality_name[stage_map[choice[oid]][LOCALITY]], row, col);

}

/**
 * Deal with events on the jump menu
 */
bool jump_action(menu_type * menu, const ui_event * evt, int oid)
{
	u16b *choice = menu_priv(menu);

	int idx = choice[oid];

	if (evt->type == EVT_SELECT) {
		place = idx;
		/* Accept request */
		msg("You jump to %s level %d.",
			locality_name[stage_map[place][LOCALITY]],
			stage_map[place][DEPTH]);
	} else
		return TRUE;

	return FALSE;
}


/**
 * Display list of places to jump to.
 */
bool jump_menu(int level, int *location)
{
	menu_type menu;
	menu_iter menu_f = { jump_tag, 0, jump_display, jump_action, 0 };
	region area = { 15, 1, 48, -1 };
	ui_event evt = { 0 };
	int cursor = 0, j = 0;
	size_t i;
	u16b *choice;

	/* Dungeon only is easy */
	if ((p_ptr->map == MAP_DUNGEON) || (p_ptr->map == MAP_FANILLA)) {
		*location = level + 1;
		return TRUE;
	}

	/* Create the array */
	choice = C_ZNEW(15, u16b);

	/* Get the possible stages */
	for (i = 0; i < NUM_STAGES; i++)
		if ((stage_map[i][DEPTH] == level)
			&& (stage_map[i][LOCALITY] != 0))
			choice[j++] = i;

	/* Clear space */
	area.page_rows = j + 2;

	/* Save the screen and clear it */
	screen_save();

	/* Set up the menu */
	WIPE(&menu, menu);
	menu.title = "Which region do you want to be transported to?";
	menu.cmd_keys = " \n\r";
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu_setpriv(&menu, j, choice);
	menu_layout(&menu, &area);

	/* Select an entry */
	evt = menu_select(&menu, cursor, TRUE);

	/* Set it */
	if (evt.type == EVT_SELECT)
		*location = place;

	/* Free memory */
	FREE(choice);

	/* Load screen */
	screen_load();
	return (evt.type != EVT_ESCAPE);
}

/**
 * Go to any level
 */
static void do_cmd_wiz_jump(void)
{
	char ppp[80];
	char tmp_val[160];

	int new_place;

	/* Ask for level */
	if (p_ptr->command_arg <= 0) {
		/* Prompt */
		sprintf(ppp, "Jump to level (0-%d): ", MAX_DEPTH - 1);

		/* Default */
		sprintf(tmp_val, "%d", p_ptr->depth);

		/* Ask for a level */
		if (!get_string(ppp, tmp_val, 11))
			return;

		/* Extract request */
		p_ptr->command_arg = atoi(tmp_val);
	}

	/* Paranoia */
	if (p_ptr->command_arg < 0)
		p_ptr->command_arg = 0;

	/* Paranoia */
	if (p_ptr->command_arg > MAX_DEPTH - 1)
		p_ptr->command_arg = MAX_DEPTH - 1;


	/* Get choice */
	if (!jump_menu(p_ptr->command_arg, &new_place)) {
		return;
	}

	/* New stage */
	p_ptr->stage = new_place;

	/* New depth */
	p_ptr->depth = stage_map[new_place][DEPTH];

	/* Land properly */
	p_ptr->last_stage = NOWHERE;

	/* Leaving */
	p_ptr->leaving = TRUE;
}


/**
 * Become aware of a lot of objects
 */
static void do_cmd_wiz_learn(void)
{
	int i;

	object_type *i_ptr;
	object_type object_type_body;

	/* Scan every object */
	for (i = 1; i < z_info->k_max; i++) {
		object_kind *k_ptr = &k_info[i];

		/* Induce awareness */
		if (k_ptr->level <= p_ptr->command_arg) {
			/* Get local object */
			i_ptr = &object_type_body;

			/* Prepare object */
			object_prep(i_ptr, i, RANDOMISE);

			/* Awareness */
			object_aware(i_ptr);
		}
	}
}


/**
 * Hack -- Rerate Hitpoints
 */
static void do_cmd_rerate(void)
{
	int min_value, max_value, i, percent;

	min_value = (PY_MAX_LEVEL * 3 * (p_ptr->hitdie - 1)) / 8;
	min_value += PY_MAX_LEVEL;

	max_value = (PY_MAX_LEVEL * 5 * (p_ptr->hitdie - 1)) / 8;
	max_value += PY_MAX_LEVEL;

	p_ptr->player_hp[0] = p_ptr->hitdie;

	/* Rerate */
	while (1) {
		/* Collect values */
		for (i = 1; i < PY_MAX_LEVEL; i++) {
			p_ptr->player_hp[i] = randint1(p_ptr->hitdie);
			p_ptr->player_hp[i] += p_ptr->player_hp[i - 1];
		}

		/* Legal values */
		if ((p_ptr->player_hp[PY_MAX_LEVEL - 1] >= min_value)
			&& (p_ptr->player_hp[PY_MAX_LEVEL - 1] <= max_value))
			break;
	}

	percent =
		(int) (((long) p_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
			   (p_ptr->hitdie + ((PY_MAX_LEVEL - 1) * p_ptr->hitdie)));

	/* Update and redraw hitpoints */
	p_ptr->update |= (PU_HP);
	p_ptr->redraw |= (PR_HP);

	/* Handle stuff */
	handle_stuff(p_ptr);

	/* Message */
	msg("Current Life Rating is %d/100.", percent);
}


/**
 * Summon some creatures
 */
static void do_cmd_wiz_summon(int num)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int i;

	for (i = 0; i < num; i++) {
		(void) summon_specific(py, px, FALSE, p_ptr->depth, 0);
	}
}


/**
 * Summon a creature of the specified type
 *
 * This function is rather dangerous XXX XXX XXX
 */
static void do_cmd_wiz_named(int r_idx, bool slp)
{
	monster_race *r_ptr = &r_info[r_idx];

	int py = p_ptr->py;
	int px = p_ptr->px;

	int i, x, y;

	/* Paranoia */
	if (!r_idx)
		return;
	if (r_idx >= z_info->r_max)
		return;

	/* Try 10 times */
	for (i = 0; i < 10; i++) {
		int d = 1;

		/* Pick a location */
		scatter(&y, &x, py, px, d, 0);

		/* Require grids that the monster can exist in */
		if (!cave_exist_mon(r_ptr, y, x, FALSE))
			continue;

		/* Place it (allow groups) */
		if (place_monster_aux(y, x, r_idx, slp, TRUE))
			break;
	}
}

/**
 * Hack -- Delete all nearby monsters
 */
static void do_cmd_wiz_zap(int d)
{
	int i;

	/* Genocide everyone nearby */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx)
			continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > d)
			continue;

		/* Delete the monster */
		delete_monster_idx(i);
	}
}


/**
 * Un-hide all monsters
 */
static void do_cmd_wiz_unhide(int d)
{
	int i;

	/* Process monsters */
	for (i = 1; i < m_max; i++) {
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx)
			continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > d)
			continue;

		/* Optimize -- Repair flags */
		repair_mflag_mark = repair_mflag_show = TRUE;

		/* Detect the monster */
		m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

		/* Update the monster */
		update_mon(i, FALSE);
	}
}


/**
 * Query the dungeon
 */
static void do_cmd_wiz_query(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int y, x;

	struct keypress cmd;

	u16b mask = 0x00;


	/* Get a "debug command" */
	if (!get_com("Debug Command Query: ", &cmd))
		return;

	/* Extract a flag */
	switch (cmd.code) {
	case '0':
		mask = (1 << 0);
		break;
	case '1':
		mask = (1 << 1);
		break;
	case '2':
		mask = (1 << 2);
		break;
	case '3':
		mask = (1 << 3);
		break;
	case '4':
		mask = (1 << 4);
		break;
	case '5':
		mask = (1 << 5);
		break;
	case '6':
		mask = (1 << 6);
		break;
	case '7':
		mask = (1 << 7);
		break;

	case 'm':
		mask |= (SQUARE_MARK);
		break;
	case 'g':
		mask |= (SQUARE_GLOW);
		break;
	case 'r':
		mask |= (SQUARE_ROOM);
		break;
	case 'i':
		mask |= (SQUARE_ICKY);
		break;
	case 's':
		mask |= (SQUARE_SEEN);
		break;
	case 'v':
		mask |= (SQUARE_VIEW);
		break;
	case 't':
		mask |= (SQUARE_TEMP);
		break;
	case 'w':
		mask |= (SQUARE_WALL);
		break;
	}

	/* Scan map */
	for (y = Term->offset_y; y <= Term->offset_y + SCREEN_HGT; y++) {
		for (x = Term->offset_x; x <= Term->offset_x + SCREEN_WID; x++) {
			byte a = TERM_RED;
			feature_type *f_ptr = &f_info[cave_feat[y][x]];

			/* Given mask, show only those grids */
			if (mask && !(cave_info[y][x][0] & mask))
				continue;

			/* Given no mask, show unknown grids */
			if (!mask && (cave_info[y][x][0] & (SQUARE_MARK)))
				continue;

			/* Color */
			if (tf_has(f_ptr->flags, TF_FLOOR))
				a = TERM_YELLOW;

			/* Display player/floors/walls */
			if ((y == py) && (x == px)) {
				print_rel('@', a, y, x);
			} else if (tf_has(f_ptr->flags, TF_FLOOR)) {
				print_rel('*', a, y, x);
			} else {
				print_rel('#', a, y, x);
			}
		}
	}

	/* Get keypress */
	msg("Press any key.");
	message_flush();

	/* Redraw map */
	prt_map();
}



#ifdef ALLOW_SPOILERS

/**
 * External function
 */
extern void do_cmd_spoilers(void);

#endif





/**
 * Ask for and parse a "debug command"
 *
 * The "p_ptr->command_arg" may have been set.
 */
void do_cmd_debug(void)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	struct keypress cmd;


	/* Get a "debug command" */
	if (!get_com("Debug Command: ", &cmd))
		return;

	/* Analyze the command */
	switch (cmd.code) {
		/* Nothing */
	case ESCAPE:
	case ' ':
	case '\n':
	case '\r':
		{
			break;
		}

#ifdef ALLOW_SPOILERS

		/* Hack -- Generate Spoilers */
	case '"':
		{
			do_cmd_spoilers();
			break;
		}

#endif

		/* Hack -- Help */
	case '?':
		{
			do_cmd_help();
			break;
		}

		/* Cure all maladies */
	case 'a':
		{
			do_cmd_wiz_cure_all();
			break;
		}

		/* Select a target, and teleport to it. */
	case 'b':
		{
			do_cmd_wiz_bamf();
			break;
		}

		/* Create any object */
	case 'c':
		{
			wiz_create_item();
			break;
		}

		/* Create any artifact */
	case 'C':
		{
			wiz_create_artifact();
			break;
		}

		/* Detect everything */
	case 'd':
		{
			detect_all(DETECT_RAD_MAP, FALSE);
			break;
		}

		/* Edit character */
	case 'e':
		{
			do_cmd_wiz_change();
			break;
		}

		/* View item info */
	case 'f':
		{
			(void) identify_fully();
			break;
		}

		/* Good Objects */
	case 'g':
		{
			if (p_ptr->command_arg <= 0)
				p_ptr->command_arg = 1;
			acquirement(py, px, p_ptr->command_arg, FALSE);
			break;
		}

		/* Hitpoint rerating */
	case 'h':
		{
			do_cmd_rerate();
			break;
		}

		/* Identify */
	case 'i':
		{
			(void) ident_spell();
			break;
		}

		/* Go up or down in the dungeon */
	case 'j':
		{
			do_cmd_wiz_jump();
			break;
		}

		/* Learn about objects */
	case 'l':
		{
			do_cmd_wiz_learn();
			break;
		}

		/* Magic Mapping */
	case 'm':
		{
			map_area(0, 0, FALSE);
			break;
		}

		/* Summon Named Monster */
	case 'n':
		{
			do_cmd_wiz_named(p_ptr->command_arg, TRUE);
			break;
		}

		/* Object playing routines */
	case 'o':
		{
			do_cmd_wiz_play();
			break;
		}

		/* Phase Door */
	case 'p':
		{
			teleport_player(10, TRUE);
			break;
		}

		/* Query the dungeon */
	case 'q':
		{
			do_cmd_wiz_query();
			break;
		}

		/* Summon Random Monster(s) */
	case 's':
		{
			if (p_ptr->command_arg <= 0)
				p_ptr->command_arg = 1;
			do_cmd_wiz_summon(p_ptr->command_arg);
			break;
		}

		/* Teleport */
	case 't':
		{
			teleport_player(100, TRUE);
			break;
		}

		/* Un-hide all monsters */
	case 'u':
		{
			if (p_ptr->command_arg <= 0)
				p_ptr->command_arg = 255;
			do_cmd_wiz_unhide(p_ptr->command_arg);
			break;
		}

		/* Very Good Objects */
	case 'v':
		{
			if (p_ptr->command_arg <= 0)
				p_ptr->command_arg = 1;
			acquirement(py, px, p_ptr->command_arg, TRUE);
			break;
		}

		/* Fully wizard Light the Level */
	case 'w':
		{
			wiz_light(TRUE);
			break;
		}

		/* Increase Experience */
	case 'x':
		{
			if (p_ptr->command_arg) {
				gain_exp(p_ptr->command_arg);
			} else {
				gain_exp(p_ptr->exp + 1);
			}
			break;
		}

		/* Zap Monsters (Genocide) */
	case 'z':
		{
			if (p_ptr->command_arg <= 0)
				p_ptr->command_arg = MAX_SIGHT;
			do_cmd_wiz_zap(p_ptr->command_arg);
			break;
		}

		/* Hack */
	case '_':
		{
			do_cmd_wiz_hack_ben();
			break;
		}

		/* Oops */
	default:
		{
			msg("That is not a valid debug command.");
			break;
		}
	}
}


#else

#ifdef MACINTOSH
static int i = 0;
#endif

#endif
