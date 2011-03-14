/** \file object1.c 
    \brief Object display and description

 * Object flavors, colors, easy-know, display of modifiers (eg. "(+2 to 
 * strength)"), naming, puralizations, etc., what items 
 * go where in equipment, equipment-related strings, etc., and inventory 
 * management and display functions.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick & Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "object.h"
#include "ui-menu.h"


/**
 * Max sizes of the following arrays.  Changes for FAangband 0.2.2 from
 * NPPangband.
 */
#define MAX_SYLLABLES 164	/* Used with scrolls (see below) */

/**
 * Syllables for scrolls (must be 1-4 letters each)
 */

static cptr syllables[MAX_SYLLABLES] = {
    "a", "ab", "ag", "aks", "ala", "an", "ankh", "app", "arg", "arze",
    "ash", "aus", "ban", "bar", "bat", "bek", "bie", "bin", "bit", "bjor",
    "blu", "bot", "bu", "byt", "can", "comp", "con", "cos", "cre", "czra",
    "dalf", "dan", "den", "der", "doe", "dok", "dora", "eep", "el", "eng",
    "er", "ere", "erk", "esh", "evs", "fa", "fid", "fin", "flit", "for",
    "fri", "fu", "gan", "gar", "glen", "gop", "gre", "ha", "he", "hyd",
    "i", "ing", "ion", "ip", "ish", "it", "ite", "iv", "jo", "kho",
    "kli", "klis", "la", "lech", "man", "mar", "me", "mi", "mic", "mik",
    "mon", "mung", "mur", "nag", "nej", "nelg", "nep", "ner", "nes", "nis",
    "nih", "nin", "o", "od", "ood", "org", "orn", "ox", "oxy", "pay",
    "pet", "ple", "plu", "po", "pot", "prok", "qua", "qur", "re", "rea",
    "rhov", "ri", "ro", "rog", "rok", "rol", "sa", "san", "sat", "see",
    "sef", "seh", "shu", "ski", "sna", "sne", "snik", "sno", "so", "sol",
    "sri", "sta", "sun", "ta", "tab", "tem", "ther", "ti", "tox", "trol",
    "tue", "turs", "u", "ulk", "um", "un", "uni", "ur", "val", "viv",
    "vly", "vom", "wah", "wed", "werg", "wex", "whon", "wun", "x", "yerg",
    "yp", "zun", "tri", "blaa"
};





/**
 * Get the inventory color for an object.
 *
 * Hack - set the listing color of a spellbook with no useable spells 
 * to grey -LM-
 */
byte proc_list_color_hack(object_type * o_ptr)
{
    /* Hack -- Spellbooks with no useable spells are grey. */
    if ((mp_ptr->spell_book == o_ptr->tval)
	&& (mp_ptr->book_start_index[o_ptr->sval] ==
	    mp_ptr->book_start_index[o_ptr->sval + 1])) {
	return (TERM_SLATE);
    }

    /* Otherwise, get the color normally. */
    else
	return (tval_to_attr[o_ptr->tval & 0x7F]);
}






/**
 * Choice window "shadow" of the "show_inven()" function
 */
void display_inven(void)
{
    register int i, n, z = 0;

    object_type *o_ptr;

    byte attr = TERM_WHITE;

    char tmp_val[80];

    char o_name[120];


    /* Find the "final" slot */
    for (i = 0; i < INVEN_PACK; i++) {
	o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Track */
	z = i + 1;
    }

    /* Display the pack */
    for (i = 0; i < z; i++) {
	/* Examine the item */
	o_ptr = &p_ptr->inventory[i];

	/* Start with an empty "index" */
	tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

	/* Is this item "acceptable"? */
	if (item_tester_okay(o_ptr)) {
	    /* Prepare an "index" */
	    tmp_val[0] = index_to_label(i);

	    /* Bracket the "index" --(-- */
	    tmp_val[1] = ')';
	}

	/* Display the index (or blank space) */
	Term_putstr(0, i, 3, TERM_WHITE, tmp_val);

	/* Obtain an item description */
	object_desc(o_name, o_ptr, TRUE, 4);

	/* Obtain the length of the description */
	n = strlen(o_name);

	/* Acquire inventory color.  Apply spellbook hack. */
	attr = proc_list_color_hack(o_ptr);

	/* Display the entry itself */
	c_put_str(attr, o_name, i, 3);

	/* Erase the rest of the line */
	Term_erase(3 + n, i, 255);

	/* Display the weight if needed */
	if (OPT(show_weights) && o_ptr->weight) {
	    int wgt = o_ptr->weight * o_ptr->number;
	    if (OPT(use_metric))
		sprintf(tmp_val, "%3d.%1d kg", make_metric(wgt) / 10,
			make_metric(wgt) % 10);
	    else
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
	    Term_putstr(71, i, -1, TERM_WHITE, tmp_val);
	}
    }

    /* Erase the rest of the window */
    for (i = z; i < Term->hgt; i++) {
	/* Erase the line */
	Term_erase(0, i, 255);
    }
}



/**
 * Choice window "shadow" of the "show_equip()" function
 */
void display_equip(void)
{
    register int i, n;
    object_type *o_ptr;
    byte attr = TERM_WHITE;

    char tmp_val[80];

    char o_name[120];


    /* Display the equipment */
    for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
	/* Examine the item */
	o_ptr = &p_ptr->inventory[i];

	/* Hack -- never show empty quiver slots. */
	if ((!o_ptr->k_idx) && (i >= INVEN_Q0) && (i <= INVEN_Q9)) {
	    /* Clear the line, skip to next slot */
	    Term_erase(0, i - INVEN_WIELD, 255);
	    continue;
	}

	/* Hack -- never show the "blank" slot. */
	if (i == INVEN_BLANK) {
	    /* Clear the line, skip to next slot */
	    Term_erase(0, i - INVEN_WIELD, 255);
	    continue;
	}

	/* Start with an empty "index" */
	tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';

	/* Is this item "acceptable"? */
	if (item_tester_okay(o_ptr)) {
	    /* Prepare an "index" */
	    tmp_val[0] = index_to_label(i);

	    /* Bracket the "index" --(-- */
	    tmp_val[1] = ')';
	}

	/* Display the index (or blank space) */
	Term_putstr(0, i - INVEN_WIELD, 3, TERM_WHITE, tmp_val);

	/* Obtain an item description */
	object_desc(o_name, o_ptr, TRUE, 4);

	/* Obtain the length of the description */
	n = strlen(o_name);

	/* Acquire inventory color.  Apply spellbook hack. */
	attr = proc_list_color_hack(o_ptr);

	/* Display the entry itself */
	c_put_str(attr, o_name, i - INVEN_WIELD, 3);

	/* Erase the rest of the line */
	Term_erase(3 + n, i - INVEN_WIELD, 255);

	/* Display the slot description (if needed) */
	if (OPT(show_labels)) {
	    Term_putstr(61, i - INVEN_WIELD, -1, TERM_WHITE, "<--");
	    Term_putstr(65, i - INVEN_WIELD, -1, TERM_WHITE, mention_use(i));
	}

	/* Display the weight (if needed) */
	if (OPT(show_weights) && o_ptr->weight) {
	    int wgt = o_ptr->weight * o_ptr->number;
	    int col = (OPT(show_labels) ? 52 : 71);

	    if (OPT(use_metric))
		sprintf(tmp_val, "%3d.%1d kg", make_metric(wgt) / 10,
			make_metric(wgt) % 10);
	    else
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);

	    Term_putstr(col, i - INVEN_WIELD, -1, TERM_WHITE, tmp_val);
	}
    }

    /* Erase the rest of the window */
    for (i = INVEN_TOTAL - INVEN_WIELD; i < Term->hgt; i++) {
	/* Clear that line */
	Term_erase(0, i, 255);
    }
}



/**
 * Display the inventory.
 *
 * Hack -- do not display "trailing" empty slots
 */
void show_inven(void)
{
    int i, j, k, l, z = 0;
    int col, len, lim, wgt_col;
    int tile_hgt;

    object_type *o_ptr;

    char o_name[120];

    char tmp_val[80];

    int out_index[24];
    byte out_color[24];
    char out_desc[24][80];

    /* Count number of missiles in the quiver slots. */
    int ammo_num = quiver_count();

    /* Default length */
    len = 79 - 50;

    /* Maximum space allowed for descriptions */
    lim = Term->wid - 3;

    /* Require space for weight (if needed) */
    if (OPT(show_weights))
	lim -= 6;

    /* Hack -- Description must fit on screen */
    if (lim > Term->wid - 1)
	lim = Term->wid - 1;

    /* Find the "final" slot */
    for (i = 0; i < INVEN_PACK; i++) {
	o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Track */
	z = i + 1;
    }

    /* Display the inventory */
    for (k = 0, i = 0; i < z; i++) {
	o_ptr = &p_ptr->inventory[i];

	/* Is this item acceptable? */
	if (!item_tester_okay(o_ptr))
	    continue;

	/* Describe the object */
	object_desc(o_name, o_ptr, TRUE, 4);

	/* Hack -- enforce max length */
	o_name[lim] = '\0';

	/* Save the index */
	out_index[k] = i;

	/* Acquire inventory color.  Apply spellbook hack. */
	out_color[k] = proc_list_color_hack(o_ptr);

	/* Save the object description */
	strcpy(out_desc[k], o_name);

	/* Find the predicted "line length" */
	l = strlen(out_desc[k]) + 5;

	/* Be sure to account for the weight */
	if (OPT(show_weights))
	    l += 9;

	/* Maintain the maximum length */
	if (l > len)
	    len = l;

	/* Advance to next "line" */
	k++;
    }

    /* Find the column to start in */
    if (len > Term->wid - 4)
	col = 0;
    else if ((Term->wid - len) / 2 < 12)
	col = (Term->wid - len) / 2;
    else
	col = 12;

    /* Output each entry */
    for (j = 0; j < k; j++) {
	/* Get the index */
	i = out_index[j];

	/* Get the item */
	o_ptr = &p_ptr->inventory[i];

	/* Clear the line */
	prt("", j + 1, col ? col - 2 : col);

	/* Prepare an index --(-- */
	sprintf(tmp_val, "%c)", index_to_label(i));

	/* Clear the line with the (possibly indented) index */
	if ((object_known_p(o_ptr)) || (object_aware_p(o_ptr))) {
	    put_str(tmp_val, j + 1, col);
	} else {
	    c_put_str(TERM_L_WHITE, tmp_val, j + 1, col);
	}

	/* Display the entry itself */
	c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

	/* Display the weight if needed */
	if (OPT(show_weights)) {
	    int wgt = o_ptr->weight * o_ptr->number;

	    if (OPT(use_metric))
		sprintf(tmp_val, "%3d.%1d kg", make_metric(wgt) / 10,
			make_metric(wgt) % 10);
	    else
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);

	    wgt_col = col + len + 1;
	    if (wgt_col + 5 > Term->wid)
		wgt_col = Term->wid - 5;
	    put_str(tmp_val, j + 1, wgt_col);
	}
    }

    /* 
     * Add notes about slots used by the quiver, if we have space, want 
     * to show all slots, and have items in the quiver.
     */
    if ((p_ptr->pack_size_reduce) && (item_tester_full)
	&& (j <= (INVEN_PACK - p_ptr->pack_size_reduce))) {
	/* Insert a blank dividing line, if we have the space. */
	if (j <= ((INVEN_PACK - 1) - p_ptr->pack_size_reduce)) {
	    j++;
	    prt("", j, col ? col - 2 : col);
	}

	for (i = 1; i <= p_ptr->pack_size_reduce; i++) {
	    /* Go to next line. */
	    j++;
	    prt("", j, col ? col - 2 : col);

	    /* Determine index, print it out. */
	    sprintf(tmp_val, "%c)", index_to_label(INVEN_PACK - i));
	    put_str(tmp_val, j, col);

	    /* Note amount of ammo */
	    k = (ammo_num > 99) ? 99 : ammo_num;

	    /* Hack -- use "(Ready Ammunition)" as a description. */
	    c_put_str(TERM_BLUE, format("(Ready Ammunition) [%2d]", k), j,
		      col + 3);

	    /* Reduce ammo count */
	    ammo_num -= k;
	}
    }

    /* Make a "shadow" below the list (only if needed) */
    if (j && (j < 23))
	prt("", ++j, col ? col - 2 : col);

    /* Hack - delete exact graphics rows */
    if (tile_height > 1) {
	while ((j % tile_height) && (j <= SCREEN_ROWS))
	    prt("", ++j, col ? col - 2 : col);
    }
}



/**
 * Display the equipment.
 */
void show_equip(void)
{
    int i, j, k, l;
    int col, len, lim, wgt_col;
    int tile_hgt;

    object_type *o_ptr;

    char tmp_val[80];

    char o_name[120];

    int out_index[24];
    byte out_color[24];
    char out_desc[24][80];


    /* Default length */
    len = 79 - 50;

    /* Maximum space allowed for descriptions */
    lim = Term->wid - 3;

    /* Require space for labels (if needed) */
    if (OPT(show_labels))
	lim -= (14 + 2);

    /* Require space for weight (if needed) */
    if (OPT(show_weights))
	lim -= 6;

    /* Hack -- Description must fit */
    if (lim > Term->wid)
	lim = Term->wid;

    /* Scan the equipment list */
    for (k = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
	o_ptr = &p_ptr->inventory[i];

	/* Hack -- never show empty quiver slots. */
	if ((!o_ptr->k_idx) && (i >= INVEN_Q0) && (i <= INVEN_Q9)) {
	    /* Skip to next slot */
	    continue;
	}

	/* Is this item acceptable? */
	if (!item_tester_okay(o_ptr))
	    continue;

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 4);

	/* Truncate the description */
	o_name[lim] = 0;

	/* Save the index */
	out_index[k] = i;

	/* Acquire inventory color */
	out_color[k] = proc_list_color_hack(o_ptr);

	/* Save the description */
	strcpy(out_desc[k], o_name);

	/* Extract the maximal length (see below) */
	l = strlen(out_desc[k]) + (2 + 3);

	/* Increase length for labels (if needed) */
	if (OPT(show_labels))
	    l += (14 + 2);

	/* Increase length for weight (if needed) */
	if (OPT(show_weights))
	    l += 6;

	/* Maintain the max-length */
	if (l > len)
	    len = l;

	/* Advance the entry */
	k++;
    }

    /* Hack -- Find a column to start in */
    if (len > Term->wid - 4)
	col = 0;
    else if ((Term->wid - len) / 2 < 12)
	col = (Term->wid - len) / 2;
    else
	col = 12;

    /* Output each entry */
    for (j = 0; j < k; j++) {
	/* Get the index */
	i = out_index[j];

	/* Get the item */
	o_ptr = &p_ptr->inventory[i];

	/* Clear the line */
	prt("", j + 1, col ? col - 2 : col);


	/* Leave an empty line for the blank slot. */
	if (i == INVEN_BLANK)
	    continue;


	/* Prepare an index --(-- */
	sprintf(tmp_val, "%c)", index_to_label(i));

	/* Clear the line with the (possibly indented) index */
	if ((!o_ptr) || (object_known_p(o_ptr)) || (object_aware_p(o_ptr))) {
	    put_str(tmp_val, j + 1, col);
	} else {
	    c_put_str(TERM_L_WHITE, tmp_val, j + 1, col);
	}

	/* Use labels */
	if (OPT(show_labels)) {
	    /* Mention the use */
	    sprintf(tmp_val, "%-14s: ", mention_use(i));
	    put_str(tmp_val, j + 1, col + 3);

	    /* Display the entry itself */
	    c_put_str(out_color[j], out_desc[j], j + 1, col + 3 + 14 + 2);
	}

	/* No labels */
	else {
	    /* Display the entry itself */
	    c_put_str(out_color[j], out_desc[j], j + 1, col + 3);
	}

	/* Display the weight if needed */
	if (OPT(show_weights)) {
	    int wgt = o_ptr->weight * o_ptr->number;

	    if (OPT(use_metric))
		sprintf(tmp_val, "%3d.%1d kg", make_metric(wgt) / 10,
			make_metric(wgt) % 10);
	    else
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);

	    wgt_col = col + len + 1;
	    if (wgt_col + 5 > Term->wid)
		wgt_col = Term->wid - 5;
	    put_str(tmp_val, j + 1, wgt_col);
	}
    }

    /* Make a "shadow" below the list (only if needed) */
    if (j && (j < 23))
	prt("", ++j, col ? col - 2 : col);

    /* Hack - delete exact graphics rows */
    if (tile_height > 1) {
	while ((j % tile_height) && (j <= SCREEN_ROWS))
	    prt("", ++j, col ? col - 2 : col);
    }
}




/**
 * Verify the choice of an item.
 *
 * The item can be negative to mean "item on floor".
 */
bool verify_item(cptr prompt, int item)
{
    char o_name[120];

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
    object_desc(o_name, o_ptr, TRUE, 3);

    /* Prompt */
    sprintf(out_val, "%s %s? ", prompt, o_name);

    /* Query */
    return (get_check(out_val));
}


/**
 * Hack -- allow user to "prevent" certain choices
 *
 * The item can be negative to mean "item on floor".
 */
extern bool get_item_allow(int item)
{
    cptr s;

    object_type *o_ptr;

    /* Inventory */
    if (item >= 0) {
	o_ptr = &p_ptr->inventory[item];
    }

    /* Floor */
    else {
	o_ptr = &o_list[0 - item];
    }

    /* No inscription */
    if (!o_ptr->note)
	return (TRUE);

    /* Find a '!' */
    s = strchr(quark_str(o_ptr->note), '!');

    /* Process preventions */
    while (s) {
	/* Check the "restriction" */
	if ((s[1] == p_ptr->command_cmd) || (s[1] == '*')) {
	    /* Verify the choice */
	    if (!verify_item("Really try", item))
		return (FALSE);
	}

	/* Find another '!' */
	s = strchr(s + 1, '!');
    }

    /* Allow it */
    return (TRUE);
}



/**
 * Verify the "okayness" of a given item.
 *
 * The item can be negative to mean "item on floor".
 */
static bool get_item_okay(int item)
{
    object_type *o_ptr;

    /* Inventory */
    if (item >= 0) {
	o_ptr = &p_ptr->inventory[item];
    }

    /* Floor */
    else {
	o_ptr = &o_list[0 - item];
    }

    /* Verify the item */
    return (item_tester_okay(o_ptr));
}

/**
 * Find the "first" inventory object with the given "tag".
 *
 * A "tag" is a char "n" appearing as "@n" anywhere in the
 * inscription of an object.
 *
 * Also, the tag "@xn" will work as well, where "n" is a tag-char,
 * and "x" is the "current" p_ptr->command_cmd code.
 */
static int get_tag(int *cp, char tag)
{
    int i, start;
    cptr s;


    if (p_ptr->command_wrk == USE_EQUIP)
	start = INVEN_WIELD;
    else
	start = 0;


    /* Check every object */
    for (i = start; i < INVEN_TOTAL; ++i) {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Skip items not of the required tval. */
	if ((item_tester_tval) && (o_ptr->tval != item_tester_tval))
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
	    if ((s[1] == p_ptr->command_cmd) && (s[2] == tag)) {
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
 * Display a list of the items on the floor at the given location. -TNB-
 */
void show_floor(int *floor_list, int floor_num)
{
    int i, j, k, l;
    int col, len, lim;
    int tile_hgt;

    object_type *o_ptr;

    char o_name[120];

    char tmp_val[80];

    int out_index[24];
    byte out_color[24];
    char out_desc[24][80];

    /* Default length */
    len = 79 - 50;

    /* Maximum space allowed for descriptions */
    lim = 79 - 3;

    /* Require space for weight (if needed) */
    if (OPT(show_weights))
	lim -= 9;

    /* Display the inventory */
    for (k = 0, i = 0; i < floor_num; i++) {
	o_ptr = &o_list[floor_list[i]];

	/* Is this item acceptable? */
	if (!item_tester_okay(o_ptr))
	    continue;

	/* Describe the object */
	object_desc(o_name, o_ptr, TRUE, 4);

	/* Hack -- enforce max length */
	o_name[lim] = '\0';

	/* Save the index */
	out_index[k] = i;

	/* Get inventory color */
	out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];

	/* Save the object description */
	strcpy(out_desc[k], o_name);

	/* Find the predicted "line length" */
	l = strlen(out_desc[k]) + 5;

	/* Be sure to account for the weight */
	if (OPT(show_weights))
	    l += 9;

	/* Maintain the maximum length */
	if (l > len)
	    len = l;

	/* Advance to next "line" */
	k++;
    }

    /* Find the column to start in */
    if (len > Term->wid - 4)
	col = 0;
    else if ((Term->wid - len) / 2 < 12)
	col = (Term->wid - len) / 2;
    else
	col = 12;

    /* Output each entry */
    for (j = 0; j < k; j++) {
	/* Get the index */
	i = floor_list[out_index[j]];

	/* Get the item */
	o_ptr = &o_list[i];

	/* Clear the line */
	prt("", j + 1, col ? col - 2 : col);

	/* Prepare an index --(-- */
	sprintf(tmp_val, "%c)", index_to_label(j));

	/* Clear the line with the (possibly indented) index */
	put_str(tmp_val, j + 1, col);

	/* Display the entry itself */
	c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

	/* Display the weight if needed */
	if (OPT(show_weights)) {
	    int wgt = o_ptr->weight * o_ptr->number;
	    if (OPT(use_metric))
		sprintf(tmp_val, "%3d.%1d kg", make_metric(wgt) / 10,
			make_metric(wgt) % 10);
	    else
		sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
	    put_str(tmp_val, j + 1, 71);
	}
    }

    /* Make a "shadow" below the list (only if needed) */
    if (j && (j < 23))
	prt("", ++j, col ? col - 2 : col);

    /* Hack - delete exact graphics rows */
    if (tile_height > 1) {
	while ((j % tile_height) && (j <= SCREEN_ROWS))
	    prt("", ++j, col ? col - 2 : col);
    }
}

/* Arrays for inventory, equipment and floor */
int i1, i2, e1, e2, f1, f2, q1, q2;
int inven_count = 0, equip_count = 0, floor_count = 0, quiv_count = 0;
int equ[24], inv[24], flo[24], qui[24];


/** 
 * Make the correct prompt for items, handle mouse buttons
 */
void item_prompt(int mode, cptr pmt)
{
    bool can_squelch = ((mode & (CAN_SQUELCH)) ? TRUE : FALSE);
    bool use_inven = ((mode & (USE_INVEN)) ? TRUE : FALSE);
    bool use_equip = ((mode & (USE_EQUIP)) ? TRUE : FALSE);

    char tmp_val[160];
    char out_val[160];

    /* Clear the line */
    prt("", 0, 0);

    /* Viewing inventory */
    if (p_ptr->command_wrk == (USE_INVEN)) {
	/* Begin the prompt */
	sprintf(out_val, "Inven:");

	/* Indicate lack of inventory choices. */
	if (i1 > i2) {
	    sprintf(tmp_val, " (none),");
	}

	/* List choices. */
	else
	    sprintf(tmp_val, " %c-%c,", index_to_label(i1), index_to_label(i2));

	/* Append choices. */
	strcat(out_val, tmp_val);

	/* Indicate ability to "view" */
	if (!p_ptr->command_see) {
	    strcat(out_val, " * to see,");
	    button_add("*", '*');
	}

	/* Indicate that equipment items are available */
	if (use_equip) {
	    strcat(out_val, " / equip,");
	    button_add("/", '/');
	}

	/* Indicate that floor items are available */
	if (f1 <= f2) {
	    strcat(out_val, " - floor, . floor top,");
	    button_add("-", '-');
	    button_add(".", '.');
	}

	/* Indicate that selecting all SQUELCHED items is an option */
	if (can_squelch) {
	    strcat(out_val, " ! for squelched,");
	    button_add("!", '!');
	}
    }

    /* Viewing equipment */
    else if (p_ptr->command_wrk == (USE_EQUIP)) {
	/* Begin the prompt */
	sprintf(out_val, "Equip:");

	/* Indicate lack of equipment choices. */
	if (e1 > e2) {
	    sprintf(tmp_val, " (none),");
	}

	/* List choices. */
	else
	    sprintf(tmp_val, " %c-%c,", index_to_label(e1), index_to_label(e2));

	/* Append choices. */
	strcat(out_val, tmp_val);

	/* Indicate ability to "view" */
	if (!p_ptr->command_see) {
	    strcat(out_val, " * to see,");
	    button_add("*", '*');
	}

	/* Append */
	if (use_inven) {
	    strcat(out_val, " / inven,");
	    button_add("/", '/');
	}

	/* Append */
	if (f1 <= f2) {
	    strcat(out_val, " - floor, . floor top,");
	    button_add("-", '-');
	    button_add(".", '.');
	}

	/* Indicate that selecting all SQUELCHED items is an option */
	if (can_squelch) {
	    strcat(out_val, " ! for squelched,");
	    button_add("!", '!');
	}
    }

    /* Viewing floor */
    else if (p_ptr->command_wrk == (USE_FLOOR)) {
	/* Begin the prompt */
	sprintf(out_val, "Floor:");

	/* Indicate lack of floor choices. */
	if (f1 > f2)
	    sprintf(tmp_val, " (none),");

	/* List choices. */
	else
	    sprintf(tmp_val, " %c-%c,", I2A(f1 - f1), I2A(f2 - f1));

	/* Append */
	strcat(out_val, tmp_val);

	/* Indicate ability to "view" */
	if (!p_ptr->command_see) {
	    strcat(out_val, " * to see,");
	    button_add("*", '*');
	}

	/* Append */
	if (use_inven) {
	    strcat(out_val, " / inven,");
	    button_add("/", '/');
	} else if (use_equip) {
	    strcat(out_val, " / equip,");
	    button_add("/", '/');
	}

	/* Indicate that selecting all SQUELCHED items is an option */
	if (can_squelch) {
	    strcat(out_val, " ! for squelched,");
	    button_add("!", '!');
	}
    }

    /* Do the buttons */
    update_statusline();

    /* Finish the prompt */
    strcat(out_val, " ESC");

    /* Build the prompt */
    sprintf(tmp_val, "(%s) %s", out_val, pmt);

    /* Show the prompt */
    prt(tmp_val, 0, 0);
}




/**
 * Item selection menus 
 */
static char get_item_tag(menu_type * menu, int oid)
{
    const int *choice = menu->menu_data;
    int idx = choice[oid];

    if (p_ptr->command_wrk == USE_FLOOR)
	return I2A(oid);
    else
	return index_to_label(idx);
}

/**
 * Display an entry on the item menu
 */
void get_item_display(menu_type * menu, int oid, bool cursor, int row, int col,
		      int width)
{
    const int *choice = menu->menu_data;
    int idx = choice[oid];
    char o_name[120];

    byte attr;

    object_type *o_ptr;

    /* Do we even have a menu? */
    if (!p_ptr->command_see)
	return;

    /* Get the object - hack to abort if invalid */
    if (p_ptr->command_wrk == (USE_INVEN)) {
	o_ptr = &p_ptr->inventory[idx];
	if (!get_item_okay(idx))
	    return;
    } else if (p_ptr->command_wrk == (USE_EQUIP)) {
	o_ptr = &p_ptr->inventory[idx];
	if (!get_item_okay(idx))
	    return;
    } else {
	o_ptr = &o_list[idx];
	if (!get_item_okay(0 - idx))
	    return;
    }

    /* Set the colour */
    attr = tval_to_attr[o_ptr->tval & 0x7F];

    /* Get the object description */
    object_desc(o_name, o_ptr, TRUE, 4);

    /* Hack -- enforce max length */
    o_name[width - 3] = '\0';


    /* Print it */
    if (cursor)
	c_put_str(TERM_L_BLUE, format(">>", o_name), row, col);

    c_put_str(attr, format("%s", o_name), row, col + 2);
}

/**
 * Deal with events on the get_item menu
 */
bool get_item_action(char cmd, void *db, int oid)
{
    return TRUE;
}


/**
 * Display list items to choose from
 */
bool item_menu(int *cp, cptr pmt, int mode, bool * oops)
{
    menu_type menu;
    menu_iter menu_f = { get_item_tag, 0, get_item_display,
	get_item_action, 0
    };
    region area = { 0, 1, -1, -1 };
    ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
    int num_entries;
    bool done;
    int cursor = 0;

    int py = p_ptr->py;
    int px = p_ptr->px;
    int ty, tx;

    int j, k = 0;
    bool item;

    bool refresh = FALSE;

    bool fall_through = FALSE;

    bool can_squelch = ((mode & (CAN_SQUELCH)) ? TRUE : FALSE);

    bool use_inven = ((mode & (USE_INVEN)) ? TRUE : FALSE);
    bool use_equip = ((mode & (USE_EQUIP)) ? TRUE : FALSE);
    bool use_floor = ((mode & (USE_FLOOR | USE_TARGET)) ? TRUE : FALSE);
    bool use_quiver = FALSE;

    bool allow_equip = FALSE;
    bool allow_inven = FALSE;
    bool allow_floor = FALSE;

    bool toggle = FALSE;

    int floor_list[24];
    int floor_num;

    /* FIX LATER */
    int len = 70;

    /* Paranoia XXX XXX XXX */
    msg_print(NULL);

    /* Set target for telekinesis */
    target_get(&tx, &ty);
    if (mode & (USE_TARGET)) {
	if (ty && tx) {
	    py = ty;
	    px = tx;
	} else {
	    return FALSE;
	}
    }

    /* Not done */
    done = FALSE;

    /* No item selected */
    item = FALSE;

    /* Mega-hack -- show lists */
    if (OPT(show_lists) || small_screen)
	p_ptr->command_see = TRUE;

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
    if (i1 <= i2) {
	allow_inven = TRUE;

	/* Record them */
	for (inven_count = 0, j = i1; j <= i2; j++)
	    if (get_item_okay(j))
		inv[inven_count++] = j;
    }

    /* Full equipment */
    e1 = INVEN_WIELD;
    e2 = INVEN_TOTAL - 1;

    /* Forbid equipment */
    if (!use_equip)
	e2 = -1;

    /* Restrict equipment indexes */
    while ((e1 <= e2) && (!get_item_okay(e1)))
	e1++;
    while ((e1 <= e2) && (!get_item_okay(e2)))
	e2--;

    /* Accept equipment */
    if (e1 <= e2) {
	allow_equip = TRUE;

	/* Record them */
	for (equip_count = 0, quiv_count = 0, j = e1; j <= e2; j++)
	    if (get_item_okay(j)) {
		equ[equip_count++] = j;
		if (j >= INVEN_Q0)
		    qui[quiv_count++] = j;
	    }
    }

    /* Check for anything in the quiver */
    use_quiver = (((p_ptr->command_cmd == 'f') || (p_ptr->command_cmd == 'v'))
		  && allow_equip && quiv_count);

    /* Count "okay" floor items */
    floor_num = 0;

    /* Restrict floor usage */
    if (mode & (USE_FLOOR | USE_TARGET)) {
	/* Scan all objects in the grid */
	(void) scan_floor(floor_list, &floor_num, py, px, 0x01);
    }

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
    if (f1 <= f2) {
	allow_floor = TRUE;

	/* Record them */
	for (floor_count = 0, j = f1; j <= f2; j++)
	    if (get_item_okay(0 - floor_list[j]))
		flo[floor_count++] = floor_list[j];
    }

    /* Require at least one legal choice */
    if (!allow_inven && !allow_equip && !allow_floor) {
	/* Cancel p_ptr->command_see */
	p_ptr->command_see = FALSE;

	/* Report failure */
	*oops = TRUE;

	/* Done here */
	return FALSE;
    }

    /* Assume we'll be looking at something */
    p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

    /* Hack -- Start on equipment if requested */
    if ((mode == (USE_EQUIP)) && use_equip) {
	p_ptr->command_wrk = (USE_EQUIP);
    }

    /* Hack -- Start on equipment if shooting or throwing */
    else if (use_quiver) {
	p_ptr->command_wrk = (USE_EQUIP);
    }

    /* Use inventory if allowed. */
    else if (allow_inven) {
	p_ptr->command_wrk = (USE_INVEN);
    }

    /* Use equipment if allowed */
    else if (allow_equip) {
	p_ptr->command_wrk = (USE_EQUIP);
    }

    /* Use floor if allowed */
    else if (allow_floor) {
	p_ptr->command_wrk = (USE_FLOOR);
    }
    /* Hack -- Use (empty) inventory if no other choices available. */
    else {
	p_ptr->command_wrk = (USE_INVEN);
    }


    /* Find the column to start in */
    if (len > Term->wid - 4)
	area.col = 0;
    else if ((Term->wid - len) / 2 < 12)
	area.col = (Term->wid - len) / 2;
    else
	area.col = 12;

    /* Save the screen */
    screen_save();

    /* Set up the menu */
    WIPE(&menu, menu);
    menu.cmd_keys = "\n\r";

    /* Set the prompt */
    item_prompt(mode, pmt);

    /* Choose an appropriate set of items to display */
    if (p_ptr->command_wrk == (USE_INVEN)) {
	menu.menu_data = inv;
	num_entries = inven_count;
    } else if ((p_ptr->command_wrk == (USE_EQUIP)) && use_quiver) {
	menu.menu_data = qui;
	num_entries = quiv_count;
    } else if (p_ptr->command_wrk == (USE_EQUIP)) {
	menu.menu_data = equ;
	num_entries = equip_count;
    } else if (p_ptr->command_wrk == (USE_FLOOR)) {
	menu.menu_data = flo;
	num_entries = floor_count;
    } else
	return FALSE;
    if (!p_ptr->command_see) {
	num_entries = 0;
    }

    /* Clear space */
    area.page_rows = num_entries + 1;
    area.width = len;
    menu.count = num_entries;
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);

    /* Play until item selected or refused */
    while (!done) {
	ui_event_data ke0;
	int ni = 0;
	int ne = 0;

	/* Scan windows */
	for (j = 0; j < TERM_WIN_MAX; j++) {
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
	if ((((p_ptr->command_wrk == (USE_EQUIP)) && ni && !ne)
	     || ((p_ptr->command_wrk == (USE_INVEN)) && !ni && ne))
	    && (p_ptr->command_see)) {
	    /* Toggle */
	    toggle_inven_equip();

	    /* Track toggles */
	    toggle = !toggle;
	}

	/* Update */
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

	/* Redraw windows */
	redraw_stuff();

	/* Change the display if needed */
	if (refresh) {
	    /* Hack - load the screen and re-save */
	    screen_load();
	    screen_save();


	    /* Set the prompt */
	    item_prompt(mode, pmt);

	    /* Pick the right menu */
	    if (p_ptr->command_wrk == (USE_INVEN)) {
		menu.menu_data = inv;
		num_entries = inven_count;
	    } else if (p_ptr->command_wrk == (USE_EQUIP)) {
		menu.menu_data = equ;
		num_entries = equip_count;
	    } else if (p_ptr->command_wrk == (USE_FLOOR)) {
		menu.menu_data = flo;
		num_entries = floor_count;
	    } else
		return FALSE;

	    if (!p_ptr->command_see) {
		num_entries = 0;
	    }

	    /* Clear space */
	    area.page_rows = num_entries + 1;
	    menu.count = num_entries;
	    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);

	    refresh = FALSE;
	}

	menu_refresh(&menu);

	evt = inkey_ex();

	/* Hack - check for arrow-like inscriptions */
	if (get_tag(&k, evt.key)
	    && ((evt.key == '2') || (evt.key == '4') || (evt.key == '6')
		|| (evt.key == '8')));
	else {
	    evt = menu_select(&menu, cursor);
	}
	switch (evt.type) {
	case EVT_KBRD:
	    {
		break;
	    }

	case ESCAPE:
	    {
		done = TRUE;
		continue;
	    }

	case EVT_SELECT:
	    {
		int *tmp = (int *) menu.menu_data;
		if (p_ptr->command_wrk == (USE_FLOOR))
		    k = 0 - tmp[evt.index];
		else
		    k = tmp[evt.index];

		/* Paranoia */
		if (!get_item_okay(k))
		    continue;

		(*cp) = k;
		done = TRUE;
		continue;
	    }

	case EVT_MOVE:
	    {
		continue;
	    }

	default:
	    {
		continue;
	    }
	}
	switch (evt.key) {
	case '*':
	case ' ':
	    {
		/* Hide the list */
		if (p_ptr->command_see) {
		    /* Flip flag */
		    p_ptr->command_see = FALSE;
		}

		/* Show the list */
		else {
		    /* Flip flag */
		    p_ptr->command_see = TRUE;
		}
		refresh = TRUE;
		break;
	    }
	case '-':
	    {
		if (!allow_floor) {
		    bell("Cannot select floor!");
		    break;
		}

		/* 
		 * If we aren't examining the floor and there is only
		 * one item, we will select it if floor_query_flag
		 * is FALSE.
		 */
		/* Hack -- Auto-Select */
		if (!OPT(floor_query_flag) && (floor_num == 1)) {
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
		    if (!get_item_allow(k)) {
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
	case '/':
	    {
		/* Toggle to inventory */
		if (allow_inven && (p_ptr->command_wrk != (USE_INVEN))) {
		    p_ptr->command_wrk = (USE_INVEN);
		    refresh = TRUE;
		}

		/* Toggle to equipment */
		else if (allow_equip && (p_ptr->command_wrk != (USE_EQUIP))) {
		    p_ptr->command_wrk = (USE_EQUIP);
		    refresh = TRUE;
		}

		/* No toggle allowed */
		else {
		    bell("Cannot switch item selector!");
		}

		break;
	    }
	case '!':
	    {
		/* Can we select all squelched items? */
		if (can_squelch) {
		    (*cp) = ALL_SQUELCHED;
		    done = TRUE;
		    break;
		} else
		    bell("No squelched items!");

		break;
	    }
	case ESCAPE:
	    {
		evt.type = EVT_ESCAPE;
		done = TRUE;
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
		if (!get_tag(&k, evt.key)) {
		    /* We are asking for something from the quiver */
		    if ((item_tester_tval == 0) || (item_tester_tval == TV_SHOT)
			|| (item_tester_tval == TV_ARROW)
			|| (item_tester_tval == TV_BOLT)) {
			/* If allowed, look at equipment and fall through. */
			if (use_equip) {
			    p_ptr->command_wrk = USE_EQUIP;

			    /* Fall through. */
			    fall_through = TRUE;
			}
		    }

		    /* 
		     * If not asking for ammo, or not allowed to look at 
		     * equipment, display error message.
		     */
		    if (!fall_through) {
			bell("Illegal object choice (tag)!");
			break;
		    }
		}

		/* Hack -- Validate the item */
		if ((!fall_through)
		    && ((k < INVEN_WIELD) ? !allow_inven : !allow_equip)) {
		    bell("Illegal object choice (tag1)!");
		    break;
		}

		/* Validate the item */
		if ((!fall_through) && (!get_item_okay(k))) {
		    bell("Illegal object choice (tag2)!");
		    break;
		}

		/* Accept that choice */
		if (!fall_through) {
		    (*cp) = k;
		    done = TRUE;
		    break;
		}

		/* Done with fall_through */
		fall_through = FALSE;

	    }

	default:
	    {
		bool verify;

		/* Note verify */
		verify = (isupper(evt.key) ? TRUE : FALSE);

		/* Lowercase */
		evt.key = tolower(evt.key);

		/* Convert letter to inventory index */
		if (p_ptr->command_wrk == (USE_INVEN)) {
		    k = label_to_inven(evt.key);
		    if (k < 0) {
			bell("Illegal object choice (inven)!");
			break;
		    }
		}

		/* Convert letter to equipment index */
		else if (p_ptr->command_wrk == (USE_EQUIP)) {
		    k = label_to_equip(evt.key);

		    if (k < 0) {
			bell("Illegal object choice (equip)!");
			break;
		    }
		}

		/* Convert letter to floor index */
		else {
		    k = (islower(evt.key) ? A2I(evt.key) : -1);

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
		    evt.type = EVT_ESCAPE;
		    break;
		}

		/* Accept that choice */
		(*cp) = k;
		done = TRUE;
		break;
	    }
	}
    }

    /* Kill buttons */
    button_kill('*');
    button_kill('/');
    button_kill('-');
    button_kill('.');
    button_kill('!');
    update_statusline();

    /* Load screen */
    screen_load();

    /* Clean up */
    if (OPT(show_choices)) {
	/* Toggle again if needed */
	if (toggle)
	    toggle_inven_equip();

	/* Update */
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

	/* Window stuff */
	window_stuff();
    }


    return (evt.type != EVT_ESCAPE);
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
 * Any of these are displayed (even if no acceptable items are in that 
 * location) if the proper flag was given.
 *
 * If there are no acceptable items available anywhere, and "str" is
 * not NULL, then it will be used as the text of a warning message
 * before the function returns.
 *
 * Note that the user must press "-" to specify the item on the floor.  The
 * use of "capital" letters will "examine" an inventory, equipment, or floor
 * item, and prompt for its use.
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
 * If 'all squelched items' are selected we set cp to ALL_SQUELCHED and return
 * TRUE.
 *
 * Global "p_ptr->command_new" is used when viewing the inventory or equipment
 * to allow the user to enter a command while viewing those screens, and
 * also to induce "auto-enter" of stores, and other such stuff.
 *
 * Global "p_ptr->command_see" may be set before calling this function to start
 * out in "browse" mode.  It is cleared before this function returns.
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
 *
 * This function has been revised using code from Tim Baker's Easy Patch 1.2
 *
 * This function has been largely rewritten for FAangband 0.3.2 using 
 * Pete Mack's menu code. 
 */
bool get_item(int *cp, cptr pmt, cptr str, int mode)
{
    bool done, item;

    bool oops = FALSE;

    /* Get the item index */
    if (repeat_pull(cp)) {
	/* Verify the item */
	if (get_item_okay(*cp)) {
	    /* Forget the item_tester_tval restriction */
	    item_tester_tval = 0;

	    /* Forget the item_tester_hook restriction */
	    item_tester_hook = NULL;

	    /* Success */
	    return (TRUE);
	}

	/* Invalid repeat - reset it */
	else
	    repeat_clear();
    }

    /* Paranoia XXX XXX XXX */
    msg_print(NULL);


    /* Not done */
    done = FALSE;

    /* No item selected */
    item = FALSE;
    *cp = 0;

    /* Go to menu */
    item = item_menu(cp, pmt, mode, &oops);

    /* Check validity */
    if (item)
	if (!get_item_allow(*cp)) {
	    item = FALSE;
	    msg_print(NULL);
	}

    /* Hack -- Cancel "display" */
    p_ptr->command_see = FALSE;

    /* Forget the item_tester_tval restriction */
    item_tester_tval = 0;

    /* Forget the item_tester_hook restriction */
    item_tester_hook = NULL;

    /* Clear the prompt line */
    prt("", 0, 0);

    /* Warning if needed */
    if (oops && str)
	msg_print(str);

    /* Save item if available */
    if (item)
	repeat_push(*cp);

    /* Result */
    return (item);
}


/**
 * Link to various object coloring functions from info.c. -LM-
 */
char *object_adj(int tval, int sval)
{
    object_kind *k_ptr = &k_info[lookup_kind(tval, sval)];

    switch (tval) {
    case TV_STAFF:
    case TV_WAND:
    case TV_ROD:
    case TV_POTION:
	return flavor_text + flavor_info[k_ptr->flavor].text;
    case TV_SCROLL:
	return scroll_adj[sval];
    default:
	return (NULL);
    }
}



/* Set Item Code */

/**
 * Determin if a given Set of artifacts is being used by the player.
 */
extern bool check_set(byte s_idx)
{
    byte count = 0;
    byte i;
    set_type *s_ptr = &s_info[s_idx];;

    for (i = INVEN_WIELD; i <= INVEN_FEET; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];
	if (o_ptr->name1) {
	    artifact_type *a_ptr = &a_info[o_ptr->name1];
	    if (a_ptr->set_no == s_idx) {
		count++;
	    }
	}
    }

    return (count >= s_ptr->no_of_items);
}

/**
 * Apply bonuses for complete artifact sets.
 */
extern void apply_set(int s_idx)
{
    set_type *s_ptr = &s_info[s_idx];

    bool bonus_applied = FALSE;

    byte i, j, k;

    for (i = INVEN_WIELD; i <= INVEN_FEET; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Is it an artifact? */
	if (o_ptr->name1) {
	    artifact_type *a_ptr = &a_info[o_ptr->name1];

	    /* Is it in the correct set? */
	    if (a_ptr->set_no == s_idx) {

		/* Loop through set elements */
		for (j = 0; j < (s_ptr->no_of_items); j++) {

		    set_element *se_ptr = &s_ptr->set_items[j];

		    /* Correct Element? */
		    if (se_ptr->a_idx == o_ptr->name1) {

			/* Bonus already applied? */
			if (!(a_ptr->set_bonus)) {
			    o_ptr->flags_obj |= se_ptr->flags_obj;
			    o_ptr->flags_curse |= se_ptr->flags_curse;
			    for (k = 0; k < MAX_P_RES; k++)
				if (se_ptr->percent_res[k] != RES_LEVEL_BASE)
				    o_ptr->percent_res[k] =
					se_ptr->percent_res[k];
			    for (k = 0; k < A_MAX; k++)
				if (se_ptr->bonus_stat[k] != BONUS_BASE)
				    o_ptr->bonus_stat[k] =
					se_ptr->bonus_stat[k];
			    for (k = 0; k < MAX_P_BONUS; k++)
				if (se_ptr->bonus_other[k] != BONUS_BASE)
				    o_ptr->bonus_other[k] =
					se_ptr->bonus_other[k];
			    for (k = 0; k < MAX_P_SLAY; k++)
				if (se_ptr->multiple_slay[k] != MULTIPLE_BASE)
				    o_ptr->multiple_slay[k] =
					se_ptr->multiple_slay[k];
			    for (k = 0; k < MAX_P_BRAND; k++)
				if (se_ptr->multiple_brand[k] != MULTIPLE_BASE)
				    o_ptr->multiple_brand[k] =
					se_ptr->multiple_brand[k];
			    a_ptr->set_bonus = TRUE;
			    bonus_applied = TRUE;
			}
		    }
		}
	    }
	}
    }

    /* Notify */
    if (bonus_applied)
	msg_print("Item set completed!");
}

/**
 * Remove bonuses for no-longer-complete artifact sets.
 */
extern void remove_set(int s_idx)
{
    set_type *s_ptr = &s_info[s_idx];

    bool bonus_removed = FALSE;

    byte i, j, k;

    for (i = INVEN_WIELD; i <= INVEN_FEET; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Is it an artifact? */
	if (o_ptr->name1) {
	    artifact_type *a_ptr = &a_info[o_ptr->name1];

	    /* Is it in the correct set? */
	    if (a_ptr->set_no == s_idx) {

		/* Loop through set elements */
		for (j = 0; j < (s_ptr->no_of_items); j++) {

		    set_element *se_ptr = &s_ptr->set_items[j];

		    /* Correct Element? */
		    if (se_ptr->a_idx == o_ptr->name1) {

			/* Is the bonus really there? */
			if (a_ptr->set_bonus) {
			    o_ptr->flags_obj &= ~se_ptr->flags_obj;
			    o_ptr->flags_curse &= ~se_ptr->flags_curse;
			    a_ptr->set_bonus = FALSE;
			    for (k = 0; k < MAX_P_RES; k++)
				o_ptr->percent_res[k] = a_ptr->percent_res[k];
			    for (k = 0; k < A_MAX; k++)
				o_ptr->bonus_stat[k] = a_ptr->bonus_stat[k];
			    for (k = 0; k < MAX_P_BONUS; k++)
				o_ptr->bonus_other[k] = a_ptr->bonus_other[k];
			    for (k = 0; k < MAX_P_SLAY; k++)
				o_ptr->multiple_slay[k] =
				    a_ptr->multiple_slay[k];
			    for (k = 0; k < MAX_P_BRAND; k++)
				o_ptr->multiple_brand[k] =
				    a_ptr->multiple_brand[k];
			    bonus_removed = TRUE;
			}
		    }
		}
	    }
	}
    }

    /* Notify */
    if (bonus_removed)
	msg_print("Item set no longer completed.");
}

/**
 *Returns the number of times in 1000 that @ will FAIL
 */
int get_use_device_chance(const object_type * o_ptr)
{
    /* Get the object kind */
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    /* Extract the item level */
    int lev = k_ptr->level;

    /* Base chance of success */
    int chance = p_ptr->state.skills[SKILL_DEVICE];

    /* Final "probability" */
    int prob = 10000;

    /* Confusion hurts skill */
    if (p_ptr->timed[TMD_CONFUSED])
	chance = chance / 2;

    /* High level objects are harder */
    chance = chance - ((lev > 50) ? 50 : lev);

    /* Calculate the chance */
    if (chance < USE_DEVICE) {
	prob *= 2;
	prob /= (USE_DEVICE * (USE_DEVICE + 1 - chance));
    } else {
	prob -= prob * (USE_DEVICE - 1) / chance;
    }
    prob /= 10;

    return 1000 - prob;
}

/* Basic tval testers */
bool obj_is_staff(const object_type * o_ptr)
{
    return o_ptr->tval == TV_STAFF;
}

bool obj_is_wand(const object_type * o_ptr)
{
    return o_ptr->tval == TV_WAND;
}

bool obj_is_rod(const object_type * o_ptr)
{
    return o_ptr->tval == TV_ROD;
}

bool obj_is_potion(const object_type * o_ptr)
{
    return o_ptr->tval == TV_POTION;
}

bool obj_is_scroll(const object_type * o_ptr)
{
    return o_ptr->tval == TV_SCROLL;
}

bool obj_is_food(const object_type * o_ptr)
{
    return o_ptr->tval == TV_FOOD;
}

bool obj_is_light(const object_type * o_ptr)
{
    return o_ptr->tval == TV_LITE;
}

bool obj_is_ring(const object_type * o_ptr)
{
    return o_ptr->tval == TV_RING;
}

/**
 * Determine whether an object is ammo
 *
 * \param o_ptr is the object to check
 */
bool obj_is_ammo(const object_type * o_ptr)
{
    return is_missile((object_type *) o_ptr);
}





/* Determine if an object has charges */
bool obj_has_charges(const object_type * o_ptr)
{
    if (o_ptr->tval != TV_WAND && o_ptr->tval != TV_STAFF)
	return FALSE;

    if (o_ptr->pval <= 0)
	return FALSE;

    return TRUE;
}

int number_charging(const object_type * o_ptr)
{
    int num_charging;
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    /* A single rod */
    if (o_ptr->number == 1) {
	if (o_ptr->timeout)
	    return 1;
	else
	    return 0;
    }

    /* A stack of rods */
    else if ((o_ptr->number > 1)
	     && (o_ptr->timeout > o_ptr->pval - k_ptr->pval)) {
	num_charging = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
	if (num_charging > o_ptr->number)
	    num_charging = o_ptr->number;
	return num_charging;
    }
}


/* Determine if an object is zappable */
bool obj_can_zap(const object_type * o_ptr)
{
    /* Any rods not charging? */
    if (o_ptr->tval == TV_ROD && number_charging(o_ptr) < o_ptr->number)
	return TRUE;

    return FALSE;
}

/* Determine if an object is activatable */
bool obj_is_activatable(const object_type * o_ptr)
{
    return object_effect(o_ptr) ? TRUE : FALSE;
}

/* Determine if an object can be activated now */
bool obj_can_activate(const object_type * o_ptr)
{
    if (obj_is_activatable(o_ptr)) {
	/* Check the recharge */
	if (!o_ptr->timeout)
	    return TRUE;
    }

    return FALSE;
}

bool obj_can_refill(const object_type * o_ptr)
{
    const object_type *j_ptr = &p_ptr->inventory[INVEN_LITE];

    if (j_ptr->sval == SV_LITE_LANTERN) {
	/* Flasks of oil are okay */
	if (o_ptr->tval == TV_FLASK)
	    return (TRUE);
    }

    /* Non-empty, non-everburning sources are okay */
    if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == j_ptr->sval)
	&& (o_ptr->pval > 0)) {
	return (TRUE);
    }

    /* Assume not okay */
    return (FALSE);
}


bool obj_can_browse(const object_type * o_ptr)
{
    /* Wrong tval */
    if (mp_ptr->spell_book != o_ptr->tval)
	return (FALSE);

    /* Book not usable by this class */
    if ((mp_ptr->book_start_index[o_ptr->sval] ==
	 mp_ptr->book_start_index[o_ptr->sval + 1]))
	return (FALSE);

    /* Okay then */
    return (TRUE);
}

bool obj_can_cast_from(const object_type * o_ptr)
{
    return obj_can_browse(o_ptr)
	&& spell_book_count_spells(o_ptr, spell_okay_to_cast) > 0;
}

bool obj_can_study(const object_type * o_ptr)
{
    return obj_can_browse(o_ptr)
	&& spell_book_count_spells(o_ptr, spell_okay_to_study) > 0;
}


/* Can only take off non-cursed items */
bool obj_can_takeoff(const object_type * o_ptr)
{
    return !cursed_p(o_ptr);
}

/* Can only put on wieldable items */
bool obj_can_wear(const object_type * o_ptr)
{
    return (wield_slot(o_ptr) >= INVEN_WIELD);
}

/* Can only fire an item with the right tval */
bool obj_can_fire(const object_type * o_ptr)
{
    return o_ptr->tval == p_ptr->ammo_tval;
}

/* Can has inscrip pls */
bool obj_has_inscrip(const object_type * o_ptr)
{
    return (o_ptr->note ? TRUE : FALSE);
}

/*** Generic utility functions ***/

/*
 * Return an object's effect.

u16b object_effect(const object_type *o_ptr)
{
	if (o_ptr->name1)
		return a_info[o_ptr->name1].effect;
	else
		return k_info[o_ptr->k_idx].effect;
} */

/* Get an o_ptr from an item number */
object_type *object_from_item_idx(int item)
{
    if (item >= 0)
	return &p_ptr->inventory[item];
    else
	return &o_list[0 - item];
}

#if 0
/*
 * Does the given object need to be aimed?
 */
bool obj_needs_aim(object_type * o_ptr)
{
    int effect;

    /* Figure out effect the object would use */
    if (o_ptr->name1)
	effect = a_info[o_ptr->name1].effect;
    else
	effect = k_info[o_ptr->k_idx].effect;

    /* If the effect needs aiming, or if the object type needs aiming, this
     * object needs aiming. */
    if (effect_aim(effect) || o_ptr->tval == TV_BOLT || o_ptr->tval == TV_SHOT
	|| o_ptr->tval == TV_ARROW || o_ptr->tval == TV_WAND
	|| (o_ptr->tval == TV_ROD && !object_flavor_is_aware(o_ptr)))
	return TRUE;
    else
	return FALSE;
}
#endif

/* 
 * Check if the given item is available for the player to use. 
 *
 * 'mode' defines which areas we should look at, a la scan_items().
 */
bool item_is_available(int item, bool(*tester) (const object_type *), int mode)
{
    int item_list[INVEN_TOTAL + MAX_FLOOR_STACK];
    int item_num;
    int i;

    item_tester_hook = tester;
    item_tester_tval = 0;
    item_num = scan_items(item_list, N_ELEMENTS(item_list), mode);

    for (i = 0; i < item_num; i++) {
	if (item_list[i] == item)
	    return TRUE;
    }

    return FALSE;
}

/*
 * Looks if "inscrip" is present on the given object.
 */
unsigned check_for_inscrip(const object_type * o_ptr, const char *inscrip)
{
    unsigned i = 0;
    const char *s;

    if (!o_ptr->note)
	return 0;

    s = quark_str(o_ptr->note);

    do {
	s = strstr(s, inscrip);
	if (!s)
	    break;

	i++;
	s++;
    }
    while (s);

    return i;
}
