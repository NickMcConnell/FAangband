/*
 * File: obj-ui.c
 * Purpose: Mainly object descriptions and generic UI functions
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
#include "keymap.h"
#include "target.h"
#include "tvalsval.h"
#include "ui-menu.h"

/* Variables for item display and selection */
struct object_menu_data {
	char label[80];
	char key;
	object_type *object;
	int index;
};

const char *prompt;
int i1, i2;
int e1, e2;
int f1, f2;
int floor_list[MAX_FLOOR_STACK];
struct object_menu_data items[50];
int num_obj;
int item_mode;
cmd_code item_cmd;
static int offset = 0;
static bool need_spacer;
static char selection;
static bool show_list;
static olist_detail_t olist_mode = 0;




/*
 * Display an object.  Each object may be prefixed with a label.
 * Used by show_inven(), show_equip(), and show_floor().  Mode flags are
 * documented in object.h
 */
static void show_obj(int onum, size_t max_len, char label[80],
					 const object_type * object, bool cursor,
					 olist_detail_t mode)
{
	int row = 0, col = 0;
	int ex_width = 0, ex_offset, ex_offset_ctr;

	object_type *o_ptr = (object_type *) object;
	char o_name[160];
	char tmp_val[80];

	bool in_term;
	byte attr = proc_list_color_hack(o_ptr);

	/* Highlight */
	if (cursor)
		attr = get_color(attr, ATTR_HIGH, 1);

	in_term = (mode & OLIST_WINDOW) ? TRUE : FALSE;

	/* Object name */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

	/* Width of extra fields */
	if (mode & OLIST_WEIGHT)
		ex_width += 9;
	if (mode & OLIST_PRICE)
		ex_width += 9;
	if (mode & OLIST_FAIL)
		ex_width += 10;

	/* Determine beginning row and column */
	if (in_term) {
		/* Term window */
		row = offset;
		col = 0;
	} else {
		/* Main window */
		row = 1;
		col = Term->wid - 3 - max_len - ex_width;
		col = MIN(col, COL_MAP + tile_width);

		if (col < 3)
			col = 0;
	}

	/* Column offset of the first extra field */
	ex_offset = MIN(max_len, (size_t) (Term->wid - 1 - ex_width - col));

	/* Clear the line */
	prt("", row + onum, MAX(col - 2, 0));

	/* Print the label */
	put_str(label, row + onum, col);

	/* Print the object */
	if (o_ptr != NULL) {
		/* Limit object name */
		if (strlen(label) + strlen(o_name) > (size_t) ex_offset) {
			int truncate = ex_offset - strlen(label);

			if (truncate < 0)
				truncate = 0;
			if ((size_t) truncate > sizeof(o_name) - 1)
				truncate = sizeof(o_name) - 1;

			o_name[truncate] = '\0';
		}

		/* Object name */
		c_put_str(attr, o_name, row + onum, col + strlen(label));

		/* Extra fields */
		ex_offset_ctr = ex_offset;

		if (mode & OLIST_PRICE) {
			int price = price_item(o_ptr, TRUE, o_ptr->number);
			strnfmt(tmp_val, sizeof(tmp_val), "%6d au", price);
			put_str(tmp_val, row + onum, col + ex_offset_ctr);
			ex_offset_ctr += 9;
		}

		if (mode & OLIST_FAIL) {
			int fail = (9 + get_use_device_chance(o_ptr)) / 10;
			if (object_aware_p(o_ptr))
				strnfmt(tmp_val, sizeof(tmp_val), "%4d%% fail", fail);
			else
				my_strcpy(tmp_val, "    ? fail", sizeof(tmp_val));
			put_str(tmp_val, row + onum, col + ex_offset_ctr);
			ex_offset_ctr += 10;
		}

		if (mode & OLIST_WEIGHT) {
			int weight = o_ptr->weight * o_ptr->number;
			strnfmt(tmp_val, sizeof(tmp_val), "%4d.%1d lb", weight / 10,
					weight % 10);
			put_str(tmp_val, row + onum, col + ex_offset_ctr);
			ex_offset_ctr += 9;
		}
	}
}

/* 
 * Build the object list.  Note that only the equipment has first non-zero. 
 */
static void build_obj_list(int first, int last, const int *floor_list,
						   olist_detail_t mode)
{
	int i;
	object_type *o_ptr;
	bool in_term = (mode & OLIST_WINDOW) ? TRUE : FALSE;

	need_spacer = FALSE;
	offset = 0;
	num_obj = 0;

	/* Clear the existing contents */
	for (i = 0; i < 50; i++) {
		items[i].object = NULL;
		items[i].index = 0;
		items[i].key = '\0';
		my_strcpy(items[i].label, "", sizeof(items[i].label));
	}

	/* Leave top line clear for inventory subwindow */
	if (!first && !floor_list && in_term)
		offset = 1;

	for (i = first; i <= last; i++) {
		if (floor_list)
			o_ptr = &o_list[floor_list[i]];
		else
			o_ptr = &p_ptr->inventory[i];

		/* May need a blank line to separate equipment and quiver */
		if ((i == INVEN_TOTAL) && (first)) {
			int j;

			/* Scan the rest of the items for acceptable entries */
			for (j = i; j <= last; j++) {
				o_ptr = &p_ptr->inventory[j];
				if (item_tester_okay(o_ptr) || in_term)
					need_spacer = TRUE;
			}

			continue;
		}

		/* Tester always skips gold. When gold should be displayed,
		 * only test items that are not gold.
		 */
		if (((o_ptr->tval == TV_GOLD) && (mode & OLIST_GOLD)) ||
			item_tester_okay(o_ptr))
			strnfmt(items[num_obj].label, sizeof(items[num_obj].label),
					"%c) ", index_to_label(i));

		/* Unacceptable carried items are still displayed in term windows */
		else if ((in_term) && (!floor_list))
			my_strcpy(items[num_obj].label, "   ",
					  sizeof(items[num_obj].label));

		/* Unacceptable items are skipped in the main window */
		else
			continue;

		/* Special labels for equipment */
		if (first) {
			char tmp_val[80];

			/* Show full slot labels */
			strnfmt(tmp_val, sizeof(tmp_val), "%-14s: ", mention_use(i));
			my_strcat(items[num_obj].label, tmp_val,
					  sizeof(items[num_obj].label));
		}

		/* Save the object */
		items[num_obj].object = o_ptr;
		items[num_obj].index = i;
		items[num_obj].key = (items[num_obj].label)[0];
		num_obj++;
	}
}

/* Get the maximum object name length.  Only makes sense after building the 
 * object list
 */
static void get_max_len(size_t * max_len)
{
	int i;
	size_t max = 0;
	char o_name[160];
	object_type *o_ptr;

	/* Calculate name offset and max name length */
	for (i = 0; i < num_obj; i++) {
		o_ptr = items[i].object;

		/* Null objects are used to skip lines, or display only a label */
		if (o_ptr == NULL)
			continue;

		/* Max length of label + object name */
		object_desc(o_name, sizeof(o_name), o_ptr,
					ODESC_PREFIX | ODESC_FULL);
		max = MAX(max, strlen(items[i].label) + strlen(o_name));
	}

	/* Enforce external maximum */
	*max_len = MIN(*max_len, max);
}

/*
 * Display a list of objects.  Each object may be prefixed with a label.
 * Used by show_inven(), show_equip(), and show_floor().  Mode flags are
 * documented in object.h
 */
static void show_obj_list(int num_obj, u32b display, olist_detail_t mode)
{
	int i, row = 0, col = 0, sp = 0;
	size_t max_len = Term->wid - 1;
	int ex_width = 0;

	object_type *o_ptr;
	char tmp_val[80];

	bool in_term;

	in_term = (mode & OLIST_WINDOW) ? TRUE : FALSE;

	get_max_len(&max_len);

	/* Check for window size restrictions */
	if (in_term) {
		/* Scan windows */
		for (i = 0; i < ANGBAND_TERM_MAX; i++) {
			/* Unused */
			if (!angband_term[i])
				continue;

			/* Count windows displaying inven */
			if (op_ptr->window_flag[i] & display)
				max_len = MIN((int) max_len, angband_term[i]->wid);
		}
	}

	/* Width of extra fields */
	if (mode & OLIST_WEIGHT)
		ex_width += 9;
	if (mode & OLIST_PRICE)
		ex_width += 9;
	if (mode & OLIST_FAIL)
		ex_width += 10;

	/* Determine beginning row and column */
	if (in_term) {
		/* Term window */
		row = 0;
		col = 0;
	} else {
		/* Main window */
		row = 1;
		col = Term->wid - 1 - max_len - ex_width;
		col = MIN(col, 20);

		if (col < 3)
			col = 0;
	}

	/* Output the list */
	for (i = 0; i < num_obj; i++) {
		o_ptr = items[i].object;

		/* Display each line */
		show_obj(i + sp, max_len, items[i].label, o_ptr, FALSE, mode);
		if ((i == (INVEN_FEET - INVEN_WIELD)) && need_spacer) {
			sp = 1;
			prt("", i + sp + 1, MAX(col - 2, 0));
		}
	}

	/* For the inventory: print the quiver count */
	if (mode & OLIST_QUIVER) {
		int count, j;

		/* Adjust for subwindow */
		int skip = (in_term) ? 1 : 0;

		/* Quiver may take multiple lines */
		for (j = 0; j < p_ptr->quiver_slots; j++, i++) {
			/* Number of missiles in this "slot" */
			if (j == p_ptr->quiver_slots - 1
				&& p_ptr->quiver_remainder > 0)
				count = p_ptr->quiver_remainder;
			else
				count = 99;

			/* Clear the line */
			prt("", row + i + skip, MAX(col - 2, 0));

			/* Print the (disabled) label */
			strnfmt(tmp_val, sizeof(tmp_val), "%c) ", index_to_label(i));
			c_put_str(TERM_SLATE, tmp_val, row + i + skip, col);

			/* Print the count */
			strnfmt(tmp_val, sizeof(tmp_val), "in Quiver: %d missile%s",
					count, count == 1 ? "" : "s");
			c_put_str(TERM_L_UMBER, tmp_val, row + i + skip, col + 3);
		}
	}

	/* Clear term windows */
	if (in_term) {
		for (; i < Term->hgt; i++) {
			prt("", row + i + offset + sp, MAX(col - 2, 0));
		}
	}

	/* Print a drop shadow for the main window if necessary */
	else if (i > 0 && row + i < 24) {
		prt("", row + i + sp, MAX(col - 2, 0));
	}
}


/*
 * Display the inventory.  Builds a list of objects and passes them
 * off to show_obj_list() for display.  Mode flags documented in
 * object.h
 */
void show_inven(olist_detail_t mode)
{
	int i, last_slot = 0;
	int diff = weight_remaining();

	char header[80];

	object_type *o_ptr;

	bool in_term = (mode & OLIST_WINDOW) ? TRUE : FALSE;

	/* Include burden for term windows */
	if (in_term) {
		strnfmt(header, sizeof(header),
				"Burden %d.%d lb (%d.%d lb %s) ", p_ptr->total_weight / 10,
				p_ptr->total_weight % 10, abs(diff) / 10, abs(diff) % 10,
				(diff < 0 ? "overweight" : "remaining"));
		put_str(header, 0, 0);
	}

	/* Find the last occupied inventory slot */
	for (i = 0; i < INVEN_PACK; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (o_ptr->k_idx)
			last_slot = i;
	}

	/* Build the object list */
	build_obj_list(0, last_slot, NULL, mode);

	/* Display the object list */
	show_obj_list(num_obj, PW_INVEN, mode);
}


/*
 * Display the equipment.  Builds a list of objects and passes them
 * off to show_obj_list() for display.  Mode flags documented in
 * object.h
 */
void show_equip(olist_detail_t mode)
{
	int i, last_slot = 0;

	object_type *o_ptr;

	/* Find the last equipment slot to display */
	for (i = INVEN_WIELD; i < ALL_INVEN_TOTAL; i++) {
		o_ptr = &p_ptr->inventory[i];
		if (i < ALL_INVEN_TOTAL && o_ptr->k_idx)
			last_slot = i;
	}

	/* Build the object list */
	build_obj_list(INVEN_WIELD, last_slot, NULL, mode);

	/* Display the object list */
	show_obj_list(num_obj, PW_EQUIP, mode);
}


/*
 * Display the floor.  Builds a list of objects and passes them
 * off to show_obj_list() for display.  Mode flags documented in
 * object.h
 */
void show_floor(const int *floor_list, int floor_num, olist_detail_t mode)
{
	if (floor_num > MAX_FLOOR_STACK)
		floor_num = MAX_FLOOR_STACK;

	/* Build the object list */
	build_obj_list(0, floor_num - 1, floor_list, mode);

	/* Display the object list */
	show_obj_list(num_obj, 0, mode);
}


/*
 * Verify the choice of an item.
 *
 * The item can be negative to mean "item on floor".
 */
bool verify_item(const char *prompt, int item)
{
	char o_name[80];

	char out_val[160];

	object_type *o_ptr;

	/* Inventory */
	if (item >= 0) {
		o_ptr = &p_ptr->inventory[item];
	}

	/* Floor */
	else {
		o_ptr = &o_list[0 - item];
	}

	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

	/* Prompt */
	strnfmt(out_val, sizeof(out_val), "%s %s? ", prompt, o_name);

	/* Query */
	return (get_check(out_val));
}


/*
 * Hack -- allow user to "prevent" certain choices.
 *
 * The item can be negative to mean "item on floor".
 */
static bool get_item_allow(int item, unsigned char ch, bool is_harmless)
{
	object_type *o_ptr;
	char verify_inscrip[] = "!*";

	unsigned n;

	/* Inventory or floor */
	if (item >= 0)
		o_ptr = &p_ptr->inventory[item];
	else
		o_ptr = &o_list[0 - item];

	/* Check for a "prevention" inscription */
	verify_inscrip[1] = ch;

	/* Find both sets of inscriptions, add together, and prompt that number of
	 * times */
	n = check_for_inscrip(o_ptr, verify_inscrip);

	if (!is_harmless)
		n += check_for_inscrip(o_ptr, "!*");

	while (n--) {
		if (!verify_item("Really try", item))
			return (FALSE);
	}

	/* Allow it */
	return (TRUE);
}



/*
 * Find the "first" inventory object with the given "tag".
 *
 * A "tag" is a char "n" appearing as "@n" anywhere in the
 * inscription of an object.
 *
 * Also, the tag "@xn" will work as well, where "n" is a tag-char,
 * and "x" is the action that tag will work for.
 */
static int get_tag(int *cp, char tag, cmd_code cmd, bool quiver_tags)
{
	int i;
	const char *s;

	/* (f)ire is handled differently from all others, due to the quiver */
	if (quiver_tags) {
		i = QUIVER_START + tag - '0';
		if (p_ptr->inventory[i].k_idx) {
			*cp = i;
			return (TRUE);
		}
		return (FALSE);
	}

	/* Check every object */
	for (i = 0; i < ALL_INVEN_TOTAL; ++i) {
		object_type *o_ptr = &p_ptr->inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx)
			continue;

		/* Skip empty inscriptions */
		if (!o_ptr->note)
			continue;

		/* Find a '@' */
		s = strchr(quark_str(o_ptr->note), '@');

		/* Process all tags */
		while (s) {
			/* Check the normal tags */
			if (s[1] == tag) {
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Check the special tags */
			if ((cmd_lookup(s[1], KEYMAP_MODE_ORIG) == cmd)
				&& (s[2] == tag)) {
				/* Save the actual inventory ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}

/**
 * Make the correct prompt for items
 */
void item_prompt(int mode)
{
	char tmp_val[160];
	char out_val[160];

	bool use_inven = ((mode & USE_INVEN) ? TRUE : FALSE);
	bool use_equip = ((mode & USE_EQUIP) ? TRUE : FALSE);
	bool can_squelch = ((mode & CAN_SQUELCH) && !show_list ? TRUE : FALSE);
	bool allow_floor = (f1 <= f2);


	/* Viewing inventory */
	if (p_ptr->command_wrk == USE_INVEN) {
		/* Begin the prompt */
		strnfmt(out_val, sizeof(out_val), "Inven:");

		/* List choices */
		if (i1 <= i2) {
			/* Build the prompt */
			strnfmt(tmp_val, sizeof(tmp_val), " %c-%c,",
					index_to_label(i1), index_to_label(i2));

			/* Append */
			my_strcat(out_val, tmp_val, sizeof(out_val));
		}

		/* Indicate ability to "view" */
		if (!show_list)
			my_strcat(out_val, " * to see,", sizeof(out_val));

		/* Indicate legality of "toggle" */
		if (use_equip)
			my_strcat(out_val, " / for Equip,", sizeof(out_val));

		/* Indicate legality of the "floor" */
		if (allow_floor)
			my_strcat(out_val, " - for floor,", sizeof(out_val));

		/* Indicate that squelched items can be selected */
		if (can_squelch && !OPT(hide_squelchable))
			my_strcat(out_val, " ! for squelched,", sizeof(out_val));
	}

	/* Viewing equipment */
	else if (p_ptr->command_wrk == USE_EQUIP) {

		/* Begin the prompt */
		strnfmt(out_val, sizeof(out_val), "Equip:");

		/* List choices */
		if (e1 <= e2) {
			/* Build the prompt */
			strnfmt(tmp_val, sizeof(tmp_val), " %c-%c,",
					index_to_label(e1), index_to_label(e2));

			/* Append */
			my_strcat(out_val, tmp_val, sizeof(out_val));
		}

		/* Indicate ability to "view" */
		if (!show_list)
			my_strcat(out_val, " * to see,", sizeof(out_val));

		/* Indicate legality of "toggle" */
		if (use_inven)
			my_strcat(out_val, " / for Inven,", sizeof(out_val));

		/* Indicate legality of the "floor" */
		if (allow_floor)
			my_strcat(out_val, " - for floor,", sizeof(out_val));
	}

	/* Viewing floor */
	else {
		/* Begin the prompt */
		strnfmt(out_val, sizeof(out_val), "Floor:");

		/* List choices */
		if (f1 <= f2) {
			/* Build the prompt */
			strnfmt(tmp_val, sizeof(tmp_val), " %c-%c,", I2A(f1), I2A(f2));

			/* Append */
			my_strcat(out_val, tmp_val, sizeof(out_val));
		}

		/* Indicate ability to "view" */
		if (!show_list)
			my_strcat(out_val, " * to see,", sizeof(out_val));

		/* Append */
		if (use_inven)
			my_strcat(out_val, " / for Inven,", sizeof(out_val));

		/* Append */
		else if (use_equip)
			my_strcat(out_val, " / for Equip,", sizeof(out_val));

		/* Indicate that squelched items can be selected */
		if (can_squelch && !OPT(hide_squelchable))
			my_strcat(out_val, " ! for squelched,", sizeof(out_val));
	}

	/* Finish the prompt */
	my_strcat(out_val, " ESC", sizeof(out_val));

	/* Build the prompt */
	strnfmt(tmp_val, sizeof(tmp_val), "(%s) %s", out_val, prompt);

	/* Show the prompt */
	prt(tmp_val, 0, 0);
}

cmd_code menu_cmd;
int menu_mode;
static region area = { 20, 1, -1, -2 };

/**
 * Display an entry on the item menu
 */
void get_item_display(menu_type * menu, int oid, bool cursor, int row,
					  int col, int width)
{
	struct object_menu_data *choice = menu_priv(menu);
	size_t max_len = Term->wid - 1;

	const object_type *o_ptr = choice[oid].object;

	int ex_width = 0;

	/* Do we even have a menu? */
	if (!show_list)
		return;

	/* Width of extra fields */
	if (olist_mode & OLIST_WEIGHT)
		ex_width += 9;
	if (olist_mode & OLIST_PRICE)
		ex_width += 9;
	if (olist_mode & OLIST_FAIL)
		ex_width += 10;

	/* Get max length */
	get_max_len(&max_len);

	/* Print it */
	show_obj(oid, max_len, items[oid].label, o_ptr, cursor, olist_mode);
}

/**
 * Deal with events on the get_item menu
 */
bool get_item_action(menu_type * menu, const ui_event * event, int oid)
{
	bool refresh = FALSE;
	struct object_menu_data *choice = menu_priv(menu);
	char key = event->key.code;
	int i, k;
	char *selections = (char *) menu->selections;

	if (event->type == EVT_SELECT) {
		selection = choice[oid].key;
		return FALSE;
	}

	if (event->type == EVT_KBRD) {
		if (key == '/') {
			/* Toggle to inventory */
			if ((item_mode & USE_INVEN)
				&& (p_ptr->command_wrk != USE_INVEN)) {
				p_ptr->command_wrk = USE_INVEN;
				build_obj_list(0, i2, NULL, olist_mode);
				refresh = TRUE;
			}

			/* Toggle to equipment */
			else if ((item_mode & USE_EQUIP) &&
					 (p_ptr->command_wrk != USE_EQUIP)) {
				p_ptr->command_wrk = USE_EQUIP;
				build_obj_list(INVEN_WIELD, e2, NULL, olist_mode);
				refresh = TRUE;
			}

			/* No toggle allowed */
			else {
				bell("Cannot switch item selector!");
			}
		}

		else if (key == '-') {
			/* No toggle allowed */
			if (f1 > f2) {
				bell("Cannot select floor!");
			}
			/* Toggle to floor */
			else {
				p_ptr->command_wrk = (USE_FLOOR);
				build_obj_list(0, f2, floor_list, olist_mode);
				refresh = TRUE;
			}
		}

		else if ((key >= '0') && (key <= '9')) {
			/* Look up the tag */
			if (!get_tag(&k, key, item_cmd, item_mode & QUIVER_TAGS)) {
				bell("Illegal object choice (tag)!");
				return TRUE;
			}

			/* Match the item */
			for (i = 0; i < menu->count; i++) {
				if (choice[i].object == &p_ptr->inventory[k]) {
					Term_keypress(choice[i].key, 0);
					return TRUE;
				}
			}
		}


		if (refresh) {
			/* Load screen */
			screen_load();
			Term_fresh();

			/* Save screen */
			screen_save();

			/* Show the prompt */
			item_prompt(item_mode);

			menu_setpriv(menu, num_obj, items);
			for (i = 0; i < num_obj; i++)
				selections[i] = items[i].key;
			area.page_rows = menu->count + 1;
			menu_layout(menu, &area);
			menu_refresh(menu, TRUE);
			redraw_stuff(p_ptr);
		}

		return FALSE;

	}

	return TRUE;
}

/**
 * Display list items to choose from
 */
ui_event item_menu(cmd_code cmd, int mode)
{
	menu_type menu;
	menu_iter menu_f = { 0, 0, get_item_display, get_item_action, 0 };
	ui_event evt = { 0 };

	size_t max_len = Term->wid - 1;

	char selections[40];

	int i;

	/* Set up the menu */
	WIPE(&menu, menu);
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu_setpriv(&menu, num_obj, items);
	for (i = 0; i < num_obj; i++)
		selections[i] = items[i].key;
	menu.selections = selections;
	menu.cmd_keys = "/-0123456789";
	get_max_len(&max_len);
	area.page_rows = menu.count + 1;
	area.width = max_len;
	area.col = MIN(Term->wid - 1 - (int) max_len, COL_MAP + tile_width);
	menu_layout(&menu, &area);
	evt = menu_select(&menu, 0, TRUE);

	if (evt.type != EVT_ESCAPE) {
		evt.key.code = selection;
	}

	/* Result */
	return (evt);
}



/**
 * Let the user select an item, save its "index"
 *
 * Return TRUE only if an acceptable item was chosen by the user.
 *
 * The selected item must satisfy the "item_tester_hook()" function,
 * if that hook is set, and the "item_tester_tval", if that value is set.
 *
 * All "item_tester" restrictions are cleared before this function returns.
 *
 * The user is allowed to choose acceptable items from the equipment,
 * inventory, or floor, respectively, if the proper flag was given,
 * and there are any acceptable items in that location.
 *
 * The equipment or inventory are displayed (even if no acceptable
 * items are in that location) if the proper flag was given.
 *
 * If there are no acceptable items available anywhere, and "str" is
 * not NULL, then it will be used as the text of a warning message
 * before the function returns.
 *
 * Note that the user must press "-" to specify the item on the floor,
 * and there is no way to "examine" the item on the floor, while the
 * use of "capital" letters will "examine" an inventory/equipment item,
 * and prompt for its use.
 *
 * If a legal item is selected from the inventory, we save it in "cp"
 * directly (0 to 35), and return TRUE.
 *
 * If a legal item is selected from the floor, we save it in "cp" as
 * a negative (-1 to -511), and return TRUE.
 *
 * If no item is available, we do nothing to "cp", and we display a
 * warning message, using "str" if available, and return FALSE.
 *
 * If no item is selected, we do nothing to "cp", and return FALSE.
 *
 * Global "p_ptr->command_wrk" is used to choose between equip/inven/floor
 * listings.  It is equal to USE_INVEN or USE_EQUIP or USE_FLOOR, except
 * when this function is first called, when it is equal to zero, which will
 * cause it to be set to USE_INVEN.
 *
 * We always erase the prompt when we are done, leaving a blank line,
 * or a warning message, if appropriate, if no items are available.
 *
 * Note that only "acceptable" floor objects get indexes, so between two
 * commands, the indexes of floor objects may change.  XXX XXX XXX
 */
bool get_item(int *cp, const char *pmt, const char *str, cmd_code cmd,
			  int mode)
{
	s16b py = p_ptr->py;
	s16b px = p_ptr->px;
	unsigned char cmdkey = cmd_lookup_key(cmd, KEYMAP_MODE_ORIG);

	bool done, item;
	int j, k;

	bool oops = FALSE;
	bool toggle = FALSE;

	bool use_inven = ((mode & USE_INVEN) ? TRUE : FALSE);
	bool use_equip = ((mode & USE_EQUIP) ? TRUE : FALSE);
	bool use_floor = ((mode & (USE_FLOOR | USE_TARGET)) ? TRUE : FALSE);
	bool use_quiver = ((mode & QUIVER_TAGS) ? TRUE : FALSE);
	bool can_squelch = ((mode & CAN_SQUELCH) ? TRUE : FALSE);
	bool is_harmless = ((mode & IS_HARMLESS) ? TRUE : FALSE);
	bool quiver_tags = ((mode & QUIVER_TAGS) ? TRUE : FALSE);

	bool allow_inven = FALSE;
	bool allow_equip = FALSE;
	bool allow_floor = FALSE;

	int floor_num;
	ui_event which;

	prompt = pmt;
	olist_mode = 0;
	item_mode = mode;
	item_cmd = cmd;

	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;
	*cp = 0;

	/* Object list display modes */
	if (mode & SHOW_FAIL)
		olist_mode |= (OLIST_FAIL);
	else
		olist_mode |= (OLIST_WEIGHT);
	if (mode & SHOW_PRICES)
		olist_mode |= (OLIST_PRICE);

	show_list = OPT(show_lists) ? TRUE : FALSE;

	/* Set target for telekinesis */
	if (mode & (USE_TARGET)) {
		target_get(&px, &py);
		if (!(px && py))
			return FALSE;
	}

	/* Full inventory */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid inventory */
	if (!use_inven)
		i2 = -1;

	/* Restrict inventory indexes */
	while ((i1 <= i2) && (!get_item_okay(i1)))
		i1++;
	while ((i1 <= i2) && (!get_item_okay(i2)))
		i2--;

	/* Accept inventory */
	if (i1 <= i2)
		allow_inven = TRUE;


	/* Full equipment */
	e1 = INVEN_WIELD;
	e2 = ALL_INVEN_TOTAL - 1;

	/* Forbid equipment */
	if (!use_equip)
		e2 = -1;

	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1)))
		e1++;
	while ((e1 <= e2) && (!get_item_okay(e2)))
		e2--;

	/* Accept equipment */
	if (e1 <= e2)
		allow_equip = TRUE;

	/* Reject quiver */
	if ((e2 < QUIVER_START) || !allow_equip)
		use_quiver = FALSE;

	/* Scan all non-gold objects in the grid */
	floor_num =
		scan_floor(floor_list, N_ELEMENTS(floor_list), py, px, 0x03);

	/* Full floor */
	f1 = 0;
	f2 = floor_num - 1;

	/* Forbid floor */
	if (!use_floor)
		f2 = -1;

	/* Restrict floor indexes */
	while ((f1 <= f2) && (!get_item_okay(0 - floor_list[f1])))
		f1++;
	while ((f1 <= f2) && (!get_item_okay(0 - floor_list[f2])))
		f2--;

	/* Accept floor */
	if (f1 <= f2)
		allow_floor = TRUE;

	/* Require at least one legal choice */
	if (!allow_inven && !allow_equip && !allow_floor) {
		/* Oops */
		oops = TRUE;
		done = TRUE;
	}

	/* Analyze choices, prepare for initial menu */
	else {
		/* Start where requested */
		if ((p_ptr->command_wrk == USE_EQUIP) && allow_equip) {
			p_ptr->command_wrk = USE_EQUIP;
			build_obj_list(INVEN_WIELD, e2, NULL, olist_mode);
		} else if ((p_ptr->command_wrk == USE_INVEN) && allow_inven) {
			p_ptr->command_wrk = USE_INVEN;
			build_obj_list(0, i2, NULL, olist_mode);
		} else if ((p_ptr->command_wrk == USE_FLOOR) && allow_floor) {
			p_ptr->command_wrk = USE_FLOOR;
			build_obj_list(0, f2, floor_list, olist_mode);
		}

		/* If we are using the quiver then start on equipment */
		else if (use_quiver && allow_equip) {
			p_ptr->command_wrk = USE_EQUIP;
			build_obj_list(INVEN_WIELD, e2, NULL, olist_mode);
		}

		/* Use inventory if allowed */
		else if (use_inven && allow_inven) {
			p_ptr->command_wrk = USE_INVEN;
			build_obj_list(0, i2, NULL, olist_mode);
		}

		/* Use equipment if allowed */
		else if (use_equip && allow_equip) {
			p_ptr->command_wrk = USE_EQUIP;
			build_obj_list(INVEN_WIELD, e2, NULL, olist_mode);
		}

		/* Use floor if allowed */
		else if (use_floor && allow_floor) {
			p_ptr->command_wrk = USE_FLOOR;
			build_obj_list(0, f2, floor_list, olist_mode);
		}

		/* Hack -- Use (empty) inventory */
		else {
			p_ptr->command_wrk = USE_INVEN;
			build_obj_list(0, i2, NULL, olist_mode);
		}
	}

	/* Clear all current messages */
	msg_flag = FALSE;

	/* Start out in "display" mode */
	if (show_list) {
		/* Save screen */
		screen_save();
	}

	/* Repeat until done */
	while (!done) {
		static bool refresh;
		int ni = 0;
		int ne = 0;

		/* Scan windows */
		for (j = 0; j < ANGBAND_TERM_MAX; j++) {
			/* Unused */
			if (!angband_term[j])
				continue;

			/* Count windows displaying inven */
			if (op_ptr->window_flag[j] & (PW_INVEN))
				ni++;

			/* Count windows displaying equip */
			if (op_ptr->window_flag[j] & (PW_EQUIP))
				ne++;
		}

		/* Toggle if needed */
		if (((p_ptr->command_wrk == USE_EQUIP) && ni && !ne)
			|| ((p_ptr->command_wrk == USE_INVEN) && !ni && ne)) {
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		/* Redraw */
		p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

		/* Redraw windows */
		redraw_stuff(p_ptr);

		/* Change the list if needed */
		if (refresh) {
			/* Rebuild object list */
			if (p_ptr->command_wrk == USE_INVEN)
				build_obj_list(0, i2, NULL, olist_mode);
			else if (p_ptr->command_wrk == USE_EQUIP)
				build_obj_list(INVEN_WIELD, e2, NULL, olist_mode);
			else
				build_obj_list(0, f2, floor_list, mode);

			refresh = FALSE;
		}

		/* Show the prompt */
		item_prompt(mode);

		redraw_stuff(p_ptr);

		/* Menu if requested */
		if (show_list) {
			which = item_menu(cmd, mode);
			if (which.type == EVT_ESCAPE)
				which.key.code = ESCAPE;
		}

		/* Get a key */
		else
			which = inkey_ex();

		/* Parse it */
		switch (which.key.code) {
		case ESCAPE:
			{
				done = TRUE;
				break;
			}

		case '*':
		case '?':
		case ' ':
			{
				if (!OPT(show_lists)) {
					/* Save screen */
					screen_save();

					/* Flip flag */
					show_list = TRUE;
				}

				refresh = TRUE;
				break;
			}

		case '/':
			{
				/* Toggle to inventory */
				if (use_inven && (p_ptr->command_wrk != USE_INVEN)) {
					p_ptr->command_wrk = USE_INVEN;
					refresh = TRUE;
				}

				/* Toggle to equipment */
				else if (use_equip && (p_ptr->command_wrk != USE_EQUIP)) {
					p_ptr->command_wrk = USE_EQUIP;
					refresh = TRUE;
				}

				/* No toggle allowed */
				else {
					bell("Cannot switch item selector!");
					break;
				}

				/* Need to redraw */
				break;
			}

		case '-':
			{
				/* Paranoia */
				if (!allow_floor) {
					bell("Cannot select floor!");
					break;
				}

				/* There is only one item */
				if (floor_num == 1) {
					/* Fall through */
				} else {
					p_ptr->command_wrk = (USE_FLOOR);
					refresh = TRUE;
					break;
				}
			}
		case '.':
			{
				/* 
				 * If we are allow to use the floor, select
				 * the top item. -BR-
				 */
				if (allow_floor) {
					int k;

					/* Special index */
					k = 0 - floor_list[0];

					/* Allow player to "refuse" certain actions */
					if (!get_item_allow(k, cmdkey, is_harmless)) {
						done = TRUE;
						break;
					}

					/* Accept that choice */
					(*cp) = k;
					item = TRUE;
					done = TRUE;
				}

				break;
			}

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
			{
				/* Look up the tag */
				if (!get_tag(&k, which.key.code, cmd, quiver_tags)) {
					bell("Illegal object choice (tag)!");
					break;
				}

				/* Hack -- Validate the item */
				if ((k < INVEN_WIELD) ? !allow_inven : !allow_equip) {
					bell("Illegal object choice (tag)!");
					break;
				}

				/* Validate the item */
				if (!get_item_okay(k)) {
					bell("Illegal object choice (tag)!");
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k, cmdkey, is_harmless)) {
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}

		case KC_ENTER:
			{
				/* Choose "default" inventory item */
				if (p_ptr->command_wrk == USE_INVEN) {
					if (i1 != i2) {
						bell("Illegal object choice (default)!");
						break;
					}

					k = i1;
				}

				/* Choose the "default" slot (0) of the quiver */
				else if (quiver_tags)
					k = e1;

				/* Choose "default" equipment item */
				else if (p_ptr->command_wrk == USE_EQUIP) {
					if (e1 != e2) {
						bell("Illegal object choice (default)!");
						break;
					}

					k = e1;
				}

				/* Choose "default" floor item */
				else {
					if (f1 != f2) {
						bell("Illegal object choice (default)!");
						break;
					}

					k = 0 - floor_list[f1];
				}

				/* Validate the item */
				if (!get_item_okay(k)) {
					bell("Illegal object choice (default)!");
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k, cmdkey, is_harmless)) {
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}

		case '!':
			{
				/* Try squelched items */
				if (can_squelch) {
					(*cp) = ALL_SQUELCHED;
					item = TRUE;
					done = TRUE;
					break;
				}

				/* Just fall through */
			}

		default:
			{
				bool verify;

				/* Note verify */
				verify =
					(isupper((unsigned char) which.key.code) ? TRUE :
					 FALSE);

				/* Lowercase */
				which.key.code = tolower((unsigned char) which.key.code);

				/* Convert letter to inventory index */
				if (p_ptr->command_wrk == USE_INVEN) {
					k = label_to_inven(which.key.code);

					if (k < 0) {
						bell("Illegal object choice (inven)!");
						break;
					}
				}

				/* Convert letter to equipment index */
				else if (p_ptr->command_wrk == USE_EQUIP) {
					k = label_to_equip(which.key.code);

					if (k < 0) {
						bell("Illegal object choice (equip)!");
						break;
					}
				}

				/* Convert letter to floor index */
				else {
					k = (islower((unsigned char) which.key.code) ?
						 A2I((unsigned char) which.key.code) : -1);

					if (k < 0 || k >= floor_num) {
						bell("Illegal object choice (floor)!");
						break;
					}

					/* Special index */
					k = 0 - floor_list[k];
				}

				/* Validate the item */
				if (!get_item_okay(k)) {
					bell("Illegal object choice (normal)!");
					break;
				}

				/* Verify the item */
				if (verify && !verify_item("Try", k)) {
					done = TRUE;
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k, cmdkey, is_harmless)) {
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
		}
	}

	/* Fix the screen if necessary */
	if (show_list) {
		/* Load screen */
		screen_load();

		/* Hack -- Cancel "display" */
		show_list = FALSE;
	}

	/* Forget the item_tester_tval restriction */
	item_tester_tval = 0;

	/* Forget the item_tester_hook restriction */
	item_tester_hook = NULL;

	/* Toggle again if needed */
	if (toggle)
		toggle_inven_equip();

	/* Update */
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
	redraw_stuff(p_ptr);

	/* Clear the prompt line */
	prt("", 0, 0);

	/* Warning if needed */
	if (oops && str)
		msg(str);

	/* Result */
	return (item);
}
