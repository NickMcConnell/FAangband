/*
 * File: obj-util.c
 * Purpose: Object list maintenance and other object utilities
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
#include "defines.h"
#include "effects.h"
#include "game-cmd.h"
#include "generate.h"
#include "history.h"
#include "prefs.h"
#include "randname.h"
#include "spells.h"
#include "squelch.h"
#include "tvalsval.h"

/*
 * Hold the titles of scrolls, 6 to 14 characters each, plus quotes.
 */
char scroll_adj[MAX_TITLES][18];

static void flavor_assign_fixed(void)
{
    int i, j;

    for (i = 0; i < z_info->flavor_max; i++) {
	flavor_type *flavor_ptr = &flavor_info[i];

	/* Skip random flavors */
	if (flavor_ptr->sval == SV_UNKNOWN)
	    continue;

	for (j = 0; j < z_info->k_max; j++) {
	    /* Skip other objects */
	    if ((k_info[j].tval == flavor_ptr->tval)
		&& (k_info[j].sval == flavor_ptr->sval)) {
		/* Store the flavor index */
		k_info[j].flavor = i;
	    }
	}
    }
}


static void flavor_assign_random(byte tval)
{
    int i, j;
    int flavor_count = 0;
    int choice;

    /* Count the random flavors for the given tval */
    for (i = 0; i < z_info->flavor_max; i++) {
	if ((flavor_info[i].tval == tval)
	    && (flavor_info[i].sval == SV_UNKNOWN)) {
	    flavor_count++;
	}
    }

    for (i = 0; i < z_info->k_max; i++) {
	/* Skip other object types */
	if (k_info[i].tval != tval)
	    continue;

	/* Skip objects that already are flavored */
	if (k_info[i].flavor != 0)
	    continue;

	/* HACK - Ordinary food is "boring" */
	if ((tval == TV_FOOD) && (k_info[i].sval >= SV_FOOD_MIN_FOOD))
	    continue;

	if (!flavor_count)
	    quit_fmt("Not enough flavors for tval %d.", tval);

	/* Select a flavor */
	choice = randint0(flavor_count);

	/* Find and store the flavor */
	for (j = 0; j < z_info->flavor_max; j++) {
	    /* Skip other tvals */
	    if (flavor_info[j].tval != tval)
		continue;

	    /* Skip assigned svals */
	    if (flavor_info[j].sval != SV_UNKNOWN)
		continue;

	    if (choice == 0) {
		/* Store the flavor index */
		k_info[i].flavor = j;

		/* Mark the flavor as used */
		flavor_info[j].sval = k_info[i].sval;

		/* Hack - set the scroll name if it's a scroll */
		if (tval == TV_SCROLL) {
		    flavor_info[j].text = scroll_adj[k_info[i].sval];
		}

		/* One less flavor to choose from */
		flavor_count--;

		break;
	    }

	    choice--;
	}
    }
}


/*
 * Prepare the "variable" part of the "k_info" array.
 *
 * The "color"/"metal"/"type" of an item is its "flavor".
 * For the most part, flavors are assigned randomly each game.
 *
 * Initialize descriptions for the "colored" objects, including:
 * Rings, Amulets, Staffs, Wands, Rods, Food, Potions, Scrolls.
 *
 * The first 4 entries for potions are fixed (Water, Apple Juice,
 * Slime Mold Juice, Unused Potion).
 *
 * Scroll titles are always between 6 and 14 letters long.  This is
 * ensured because every title is composed of whole words, where every
 * word is from 2 to 8 letters long, and that no scroll is finished
 * until it attempts to grow beyond 15 letters.  The first time this
 * can happen is when the current title has 6 letters and the new word
 * has 8 letters, which would result in a 6 letter scroll title.
 *
 * Hack -- make sure everything stays the same for each saved game
 * This is accomplished by the use of a saved "random seed", as in
 * "town_gen()".  Since no other functions are called while the special
 * seed is in effect, so this function is pretty "safe".
 */
void flavor_init(void)
{
    int i, j;

    /* Hack -- Use the "simple" RNG */
    Rand_quick = TRUE;

    /* Hack -- Induce consistant flavors */
    Rand_value = seed_flavor;

    flavor_assign_fixed();

    flavor_assign_random(TV_STAFF);
    flavor_assign_random(TV_WAND);
    flavor_assign_random(TV_ROD);
    flavor_assign_random(TV_FOOD);
    flavor_assign_random(TV_POTION);

    /* Scrolls (random titles, always white) */
    for (i = 0; i < MAX_TITLES; i++) {
	char buf[26] = "\"";
	char *end = buf + 1;
	int titlelen = 0;
	int wordlen;
	bool okay = TRUE;

	wordlen = randname_make(RANDNAME_SCROLL, 2, 8, end, 24, name_sections);
	while (titlelen + wordlen < (int) (sizeof(scroll_adj[0]) - 3)) {
	    end[wordlen] = ' ';
	    titlelen += wordlen + 1;
	    end += wordlen + 1;
	    wordlen =
		randname_make(RANDNAME_SCROLL, 2, 8, end, 24 - titlelen,
			      name_sections);
	}
	buf[titlelen] = '"';
	buf[titlelen + 1] = '\0';

	/* Check the scroll name hasn't already been generated */
	for (j = 0; j < i; j++) {
	    if (streq(buf, scroll_adj[j])) {
		okay = FALSE;
		break;
	    }
	}

	if (okay) {
	    my_strcpy(scroll_adj[i], buf, sizeof(scroll_adj[0]));
	} else {
	    /* Have another go at making a name */
	    i--;
	}
    }
    flavor_assign_random(TV_SCROLL);

    /* Hack -- Use the "complex" RNG */
    Rand_quick = FALSE;

    /* Analyze every object */
    for (i = 1; i < z_info->k_max; i++) {
	object_kind *k_ptr = &k_info[i];

	/* Skip "empty" objects */
	if (!k_ptr->name)
	    continue;

	/* No flavor yields aware */
	if (!k_ptr->flavor)
	    k_ptr->aware = TRUE;

    }
}



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
	return (tval_to_attr[o_ptr->tval]);
}


#ifdef ALLOW_BORG_GRAPHICS
extern void init_translate_visuals(void);
#endif				/* ALLOW_BORG_GRAPHICS */


/*
 * Reset the "visual" lists
 *
 * This involves resetting various things to their "default" state.
 *
 * If the "prefs" flag is TRUE, then we will also load the appropriate
 * "user pref file" based on the current setting of the "use_graphics"
 * flag.  This is useful for switching "graphics" on/off.
 */
/* XXX this does not belong here */
void reset_visuals(bool load_prefs)
{
    int i;


    /* Extract default attr/char code for features */
    for (i = 0; i < z_info->f_max; i++)
    {
	int j;
	feature_type *f_ptr = &f_info[i];
	
	/* Assume we will use the underlying values */
	for (j = 0; j < FEAT_LIGHTING_MAX; j++)
	{
	    f_ptr->x_attr[j] = f_ptr->d_attr;
	    f_ptr->x_char[j] = f_ptr->d_char;
	}
    }

    /* Extract default attr/char code for objects */
    for (i = 0; i < z_info->k_max; i++) {
	object_kind *k_ptr = &k_info[i];

	/* Default attr/char */
	k_ptr->x_attr = k_ptr->d_attr;
	k_ptr->x_char = k_ptr->d_char;
    }

    /* Extract default attr/char code for monsters */
    for (i = 0; i < z_info->r_max; i++) {
	monster_race *r_ptr = &r_info[i];

	/* Default attr/char */
	r_ptr->x_attr = r_ptr->d_attr;
	r_ptr->x_char = r_ptr->d_char;
    }

    /* Extract default attr/char code for flavors */
    for (i = 0; i < z_info->flavor_max; i++) {
	flavor_type *flavor_ptr = &flavor_info[i];

	/* Default attr/char */
	flavor_ptr->x_attr = flavor_ptr->d_attr;
	flavor_ptr->x_char = flavor_ptr->d_char;
    }

    /* Extract attr/chars for inventory objects (by tval) */
    for (i = 0; i < (int) N_ELEMENTS(tval_to_attr); i++) {
	/* Default to white */
	tval_to_attr[i] = TERM_WHITE;
    }

    if (!load_prefs)
	return;



    /* Graphic symbols */
    if (use_graphics) {
	/* Process "graf.prf" */
	process_pref_file("graf.prf", FALSE, FALSE);
    }

    /* Normal symbols */
    else {
	/* Process "font.prf" */
	process_pref_file("font.prf", FALSE, FALSE);
    }

#ifdef ALLOW_BORG_GRAPHICS
    /* Initialize the translation table for the borg */
    init_translate_visuals();
#endif				/* ALLOW_BORG_GRAPHICS */
}



/*
 * Convert an inventory index into a one character label.
 *
 * Note that the label does NOT distinguish inven/equip.
 */ 
char index_to_label(int i)
{
    /* Indexes for "inven" are easy */
    if (i < INVEN_WIELD)
	return (I2A(i));

    /* Indexes for "equip" are offset */
    return (I2A(i - INVEN_WIELD));
}

/*
 * Convert a label into the index of an item in the "inven".
 *
 * Return "-1" if the label does not indicate a real item.
 */ 
s16b label_to_inven(int c)
{
    int i;

    /* Convert */
    i = (islower((unsigned char) c) ? A2I(c) : -1);

    /* Verify the index */
    if ((i < 0) || (i > INVEN_PACK))
	return (-1);

    /* Empty slots can never be chosen */
    if (!p_ptr->inventory[i].k_idx)
	return (-1);

    /* Return the index */
    return (i);
}


/*
 * Convert a label into the index of a item in the "equip".
 *
 * Return "-1" if the label does not indicate a real item.
 */
s16b label_to_equip(int c)
{
    int i;

    /* Convert */
    i = (islower((unsigned char) c) ? A2I(c) : -1) + INVEN_WIELD;

    /* Verify the index */
    if ((i < INVEN_WIELD) || (i >= ALL_INVEN_TOTAL) || (i == INVEN_TOTAL))
	return (-1);

    /* Empty slots can never be chosen */
    if (!p_ptr->inventory[i].k_idx)
	return (-1);

    /* Return the index */
    return (i);
}


/*
 * Hack -- determine if an item is "wearable" (or a missile)
 */
bool wearable_p(const object_type *o_ptr)
{
    /* Valid "tval" codes */
    switch (o_ptr->tval)
    {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_LIGHT:
    case TV_AMULET:
    case TV_RING: return (TRUE);
    }

    /* Nope */
    return (FALSE);
}

int get_inscribed_ammo_slot(const object_type *o_ptr)
{
    char *s = NULL, *t = NULL;
    if (!o_ptr->note) return 0;
    s = strchr(quark_str(o_ptr->note), 'v');
    t = strchr(quark_str(o_ptr->note), 'f');

    /* Prefer throwing weapons */
    if (s && (s[1] >= '0') && (s[1] <= '9'))
	return QUIVER_START + (s[1] - '0');
    else if (t && (t[1] >= '0') && (t[1] <= '9'))
	return QUIVER_START + (t[1] - '0');

    return 0;
}

/**
 * Used by wield_slot() to find an appopriate slot for ammo. See wield_slot()
 * for information on what this returns.
 */
s16b wield_slot_ammo(const object_type *o_ptr)
{
    s16b i, open = 0;

    /* If the ammo is inscribed with a slot number, we'll try to put it in */
    /* that slot, if possible. */
    i = get_inscribed_ammo_slot(o_ptr);
    if (i && !p_ptr->inventory[i].k_idx) return i;

    for (i = QUIVER_START; i < QUIVER_END; i++)
    {
	if (!p_ptr->inventory[i].k_idx)
	{
	    /* Save the open slot if we haven't found one already */
	    if (!open) open = i;
	    continue;
	}

	/* If ammo is cursed we can't stack it */
	if (cf_has(p_ptr->inventory[i].flags_curse, CF_STICKY_WIELD))
	    continue;

	/* If they are stackable, we'll use this slot for sure */
	if (object_similar(&p_ptr->inventory[i], o_ptr,
			   OSTACK_QUIVER)) return i;
    }

    /* If not absorbed, return an open slot (or QUIVER_START if no room) */
    return open ? open : QUIVER_START;
}

/**
 * Determine which equipment slot (if any) an item likes. The slot might (or
 * might not) be open, but it is a slot which the object could be equipped in.
 *
 * For items where multiple slots could work (e.g. ammo or rings), the function
 * will try to a return a stackable slot first (only for ammo), then an open
 * slot if possible, and finally a used (but valid) slot if necessary.
 */
s16b wield_slot(const object_type * o_ptr)
{
    /* Slot for equipment */
    switch (o_ptr->tval) {
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
	return (INVEN_WIELD);

    case TV_BOW:
	return (INVEN_BOW);

    case TV_RING:
	return p_ptr->inventory[INVEN_RIGHT].k_idx ? INVEN_LEFT : INVEN_RIGHT;

    case TV_AMULET:
	return (INVEN_NECK);

    case TV_LIGHT:
	return (INVEN_LIGHT);

    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
	return (INVEN_BODY);

    case TV_CLOAK:
	return (INVEN_OUTER);

    case TV_SHIELD:
	return (INVEN_ARM);

    case TV_CROWN:
    case TV_HELM:
	return (INVEN_HEAD);

    case TV_GLOVES:
	return (INVEN_HANDS);

    case TV_BOOTS:
	return (INVEN_FEET);

    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
	    return  wield_slot_ammo(o_ptr);
    }

    /* No slot available */
    return (-1);
}


/*
 * \returns whether item o_ptr will fit in slot 'slot'
 */
bool slot_can_wield_item(int slot, const object_type * o_ptr)
{
    if (o_ptr->tval == TV_RING)
	return (slot == INVEN_LEFT || slot == INVEN_RIGHT) ? TRUE : FALSE;
    else if (obj_is_ammo(o_ptr))
	return (slot >= QUIVER_START && slot < QUIVER_END) ? TRUE : FALSE;
    else if (wield_slot(o_ptr) == slot)
	return TRUE;
    else if (of_has(o_ptr->flags_obj, OF_THROWING))
	return (slot >= QUIVER_START && slot < QUIVER_END) ? TRUE : FALSE;
    else
	return FALSE;
}


/*
 * Return a string mentioning how a given item is carried
 */
const char *mention_use(int slot)
{
    switch (slot) {
    case INVEN_WIELD:
    {
	if (adj_str_hold[p_ptr->state.stat_ind[A_STR]] <
	    p_ptr->inventory[slot].weight / 10)
	    return "Just lifting";
	else
	    return "Wielding";
    }
    case INVEN_BOW:{
	if (adj_str_hold[p_ptr->state.stat_ind[A_STR]] <
	    p_ptr->inventory[slot].weight / 10)
	    return "Just holding";
	else
	    return "Shooting";
    }

    case INVEN_LEFT:	return "On left hand";
    case INVEN_RIGHT:	return "On right hand";
    case INVEN_NECK:	return "Around neck";
    case INVEN_LIGHT:	return "Light source";
    case INVEN_BODY:	return "On body";
    case INVEN_OUTER:	return "About body";
    case INVEN_ARM:	return "On arm";
    case INVEN_HEAD:	return "On head";
    case INVEN_HANDS:	return "On hands";
    case INVEN_FEET:	return "On feet";


    case QUIVER_START + 0: return "In quiver [f0]";
    case QUIVER_START + 1: return "In quiver [f1]";
    case QUIVER_START + 2: return "In quiver [f2]";
    case QUIVER_START + 3: return "In quiver [f3]";
    case QUIVER_START + 4: return "In quiver [f4]";
    case QUIVER_START + 5: return "In quiver [f5]";
    case QUIVER_START + 6: return "In quiver [f6]";
    case QUIVER_START + 7: return "In quiver [f7]";
    case QUIVER_START + 8: return "In quiver [f8]";
    case QUIVER_START + 9: return "In quiver [f9]";
    }
    
    return "In pack";
}


/*
 * Return a string describing how a given item is being worn.
 * Currently, only used for items in the equipment, not inventory.
 */
const char *describe_use(int i)
{
    const char *p;

    switch (i) {
    case INVEN_WIELD:	p = "attacking monsters with";
	break;
    case INVEN_BOW:	p = "shooting missiles with";
	break;
    case INVEN_LEFT:	p = "wearing on your left hand";
	break;
    case INVEN_RIGHT:	p = "wearing on your right hand";
	break;
    case INVEN_NECK:	p = "wearing around your neck";
	break;
    case INVEN_LIGHT:	p = "using to light the way";
	break;
    case INVEN_BODY:	p = "wearing on your body";
	break;
    case INVEN_OUTER:	p = "wearing on your back";
	break;
    case INVEN_ARM:{
	if (p_ptr->state.shield_on_back)
	    p = "carrying on your back";
	else
	    p = "wearing on your arm";
	break;
    }
    case INVEN_HEAD:	p = "wearing on your head";
	break;
    case INVEN_HANDS:	p = "wearing on your hands";
	break;
    case INVEN_FEET:	p = "wearing on your feet";
	break;
    case QUIVER_START + 0:
    case QUIVER_START + 1:
    case QUIVER_START + 2:
    case QUIVER_START + 3:
    case QUIVER_START + 4:
    case QUIVER_START + 5:
    case QUIVER_START + 6:
    case QUIVER_START + 7:
    case QUIVER_START + 8:
    case QUIVER_START + 9:
	{
	    p = "have in your quiver";
	    break;
	}
    default:
	p = "carrying in your pack";
	break;
    }

    /* Hack -- Heavy weapon */
    if (i == INVEN_WIELD) {
	object_type *o_ptr;
	o_ptr = &p_ptr->inventory[i];
	if (adj_str_hold[p_ptr->state.stat_ind[A_STR]] < o_ptr->weight / 10) {
	    p = "just lifting";
	}
    }

    /* Hack -- Heavy bow */
    if (i == INVEN_BOW) {
	object_type *o_ptr;
	o_ptr = &p_ptr->inventory[i];
	if (adj_str_hold[p_ptr->state.stat_ind[A_STR]] < o_ptr->weight / 10) {
	    p = "just holding";
	}
    }

    /* Return the result */
    return p;
}





/*
 * Check an item against the item tester info
 */
bool item_tester_okay(const object_type * o_ptr)
{
    /* Hack -- allow listing empty slots */
    if (item_tester_full)
	return (TRUE);

    /* Require an item */
    if (!o_ptr->k_idx)
	return (FALSE);

    /* Hack -- ignore "gold" */
    if (o_ptr->tval == TV_GOLD)
	return (FALSE);

    /* Check the tval */
    if (item_tester_tval) {
	if (item_tester_tval != o_ptr->tval)
	    return (FALSE);
    }

    /* Check the hook */
    if (item_tester_hook) {
	if (!(*item_tester_hook) (o_ptr))
	    return (FALSE);
    }

    /* Assume okay */
    return (TRUE);
}



/*
 * Get the indexes of objects at a given floor location. -TNB-
 *
 * Return the number of object indexes acquired.
 *
 * Valid flags are any combination of the bits:
 *   0x01 -- Verify item tester
 *   0x02 -- Marked/visible items only
 *   0x04 -- Only the top item
 */
int scan_floor(int *items, int max_size, int y, int x, int mode)
{
    int this_o_idx, next_o_idx;

    int num = 0;

    /* Sanity */
    if (!in_bounds(y, x))
	return 0;

    /* Scan all objects in the grid */
    for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr;

	/* XXX Hack -- Enforce limit */
	if (num >= max_size)
	    break;


	/* Get the object */
	o_ptr = &o_list[this_o_idx];

	/* Get the next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Item tester */
	if ((mode & 0x01) && !item_tester_okay(o_ptr))
	    continue;

	/* Marked */
	if ((mode & 0x02) && (!o_ptr->marked || squelch_hide_item(o_ptr)))
	    continue;

	/* Accept this item */
	items[num++] = this_o_idx;

	/* Only one */
	if (mode & 0x04)
	    break;
    }
    return num;
}




/*
 * Excise a dungeon object from any stacks
 */
void excise_object_idx(int o_idx)
{
    object_type *j_ptr;

    s16b this_o_idx, next_o_idx = 0;

    s16b prev_o_idx = 0;


    /* Object */
    j_ptr = &o_list[o_idx];

    /* Monster */
    if (j_ptr->held_m_idx) {
	monster_type *m_ptr;

	/* Monster */
	m_ptr = &m_list[j_ptr->held_m_idx];

	/* Scan all objects in the grid */
	for (this_o_idx = m_ptr->hold_o_idx; this_o_idx;
	     this_o_idx = next_o_idx) {
	    object_type *o_ptr;

	    /* Get the object */
	    o_ptr = &o_list[this_o_idx];

	    /* Get the next object */
	    next_o_idx = o_ptr->next_o_idx;

	    /* Done */
	    if (this_o_idx == o_idx) {
		/* No previous */
		if (prev_o_idx == 0) {
		    /* Remove from list */
		    m_ptr->hold_o_idx = next_o_idx;
		}

		/* Real previous */
		else {
		    object_type *i_ptr;

		    /* Previous object */
		    i_ptr = &o_list[prev_o_idx];

		    /* Remove from list */
		    i_ptr->next_o_idx = next_o_idx;
		}

		/* Forget next pointer */
		o_ptr->next_o_idx = 0;

		/* Done */
		break;
	    }

	    /* Save prev_o_idx */
	    prev_o_idx = this_o_idx;
	}
    }

    /* Dungeon */
    else {
	int y = j_ptr->iy;
	int x = j_ptr->ix;

	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {
	    object_type *o_ptr;

	    /* Get the object */
	    o_ptr = &o_list[this_o_idx];

	    /* Get the next object */
	    next_o_idx = o_ptr->next_o_idx;

	    /* Done */
	    if (this_o_idx == o_idx) {
		/* No previous */
		if (prev_o_idx == 0) {
		    /* Remove from list */
		    cave_o_idx[y][x] = next_o_idx;
		}

		/* Real previous */
		else {
		    object_type *i_ptr;

		    /* Previous object */
		    i_ptr = &o_list[prev_o_idx];

		    /* Remove from list */
		    i_ptr->next_o_idx = next_o_idx;
		}

		/* Forget next pointer */
		o_ptr->next_o_idx = 0;

		/* Done */
		break;
	    }

	    /* Save prev_o_idx */
	    prev_o_idx = this_o_idx;
	}
    }
}


/*
 * Delete a dungeon object
 *
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(int o_idx)
{
    object_type *j_ptr;

    /* Excise */
    excise_object_idx(o_idx);

    /* Object */
    j_ptr = &o_list[o_idx];

    /* Dungeon floor */
    if (!(j_ptr->held_m_idx)) {
	int y, x;

	/* Location */
	y = j_ptr->iy;
	x = j_ptr->ix;

	/* Visual update */
	light_spot(y, x);
    }

    /* Wipe the object */
    object_wipe(j_ptr);

    /* Count objects */
    o_cnt--;
}


/*
 * Deletes all objects at given location
 */
void delete_object(int y, int x)
{
    s16b this_o_idx, next_o_idx = 0;


    /* Paranoia */
    if (!in_bounds(y, x))
	return;


    /* Scan all objects in the grid */
    for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr;

	/* Get the object */
	o_ptr = &o_list[this_o_idx];

	/* Get the next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Wipe the object */
	object_wipe(o_ptr);

	/* Count objects */
	o_cnt--;
    }
    /* Objects are gone */ cave_o_idx[y][x] = 0;

    /* Visual update */
    light_spot(y, x);
}



/*
 * Move an object from index i1 to index i2 in the object list
 */
static void compact_objects_aux(int i1, int i2)
{
    int i;

    object_type *o_ptr;


    /* Do nothing */
    if (i1 == i2)
	return;


    /* Repair objects */
    for (i = 1; i < o_max; i++) {
	/* Get the object */
	o_ptr = &o_list[i];

	/* Skip "dead" objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Repair "next" pointers */
	if (o_ptr->next_o_idx == i1) {
	    /* Repair */
	    o_ptr->next_o_idx = i2;
	}
    }
    /* Get the object */ o_ptr = &o_list[i1];


    /* Monster */
    if (o_ptr->held_m_idx) {
	monster_type *m_ptr;

	/* Get the monster */
	m_ptr = &m_list[o_ptr->held_m_idx];

	/* Repair monster */
	if (m_ptr->hold_o_idx == i1) {
	    /* Repair */
	    m_ptr->hold_o_idx = i2;
	}
    }

    /* Dungeon */
    else {
	int y, x;

	/* Get location */
	y = o_ptr->iy;
	x = o_ptr->ix;

	/* Repair grid */
	if (cave_o_idx[y][x] == i1) {
	    /* Repair */
	    cave_o_idx[y][x] = i2;
	}
    }


    /* Hack -- move object */
    COPY(&o_list[i2], &o_list[i1], object_type);

    /* Hack -- wipe hole */
    object_wipe(o_ptr);
}


/*
 * Compact and reorder the object list
 *
 * This function can be very dangerous, use with caution!
 *
 * When compacting objects, we first destroy gold, on the basis that by the
 * time item compaction becomes an issue, the player really won't care.
 * We also nuke items marked as squelch.
 *
 * When compacting other objects, we base the saving throw on a combination of
 * object level, distance from player, and current "desperation".
 *
 * After compacting, we "reorder" the objects into a more compact order, and we
 * reset the allocation info, and the "live" array.
 */
void compact_objects(int size)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int i, y, x, cnt;

    int cur_lev, cur_dis, chance;


    /* Reorder objects when not passed a size */
    if (!size) {
	/* Excise dead objects (backwards!) */
	for (i = o_max - 1; i >= 1; i--) {
	    object_type *o_ptr = &o_list[i];

	    /* Skip real objects */
	    if (o_ptr->k_idx)
		continue;

	    /* Move last object into open hole */
	    compact_objects_aux(o_max - 1, i);

	    /* Compress "o_max" */
	    o_max--;
	}
	return;
    }


    /* Message */
    msg("Compacting objects...");

	/*** Try destroying objects ***/

    /* First do gold */
    for (i = 1; (i < o_max) && (size); i++) {
	object_type *o_ptr = &o_list[i];

	/* Nuke gold or squelched items */
	if (o_ptr->tval == TV_GOLD || squelch_item_ok(o_ptr)) {
	    delete_object_idx(i);
	    size--;
	}
    }


    /* Compact at least 'size' objects */
    for (cnt = 1; size; cnt++) {
	/* Get more vicious each iteration */
	cur_lev = 5 * cnt;

	/* Get closer each iteration */
	cur_dis = 5 * (20 - cnt);

	/* Examine the objects */
	for (i = 1; (i < o_max) && (size); i++) {
	    object_type *o_ptr = &o_list[i];
	    object_kind *k_ptr = &k_info[o_ptr->k_idx];

	    /* Skip dead objects */
	    if (!o_ptr->k_idx)
		continue;

	    /* Hack -- High level objects start out "immune" */
	    if (k_ptr->level > cur_lev && !k_ptr->squelch)
		continue;

	    /* Monster */
	    if (o_ptr->held_m_idx) {
		monster_type *m_ptr;

		/* Get the monster */
		m_ptr = &m_list[o_ptr->held_m_idx];

		/* Get the location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Monsters protect their objects */
		if ((randint0(100) < 90) && !k_ptr->squelch)
		    continue;
	    }

	    /* Dungeon */
	    else {
		/* Get the location */
		y = o_ptr->iy;
		x = o_ptr->ix;
	    }

	    /* Nearby objects start out "immune" */
	    if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis)
		&& !k_ptr->squelch)
		continue;

	    /* Saving throw */
	    chance = 90;


	    /* Hack -- only compact artifacts in emergencies */
	    if (artifact_p(o_ptr) && (cnt < 1000))
		chance = 100;

	    /* Apply the saving throw */
	    if (randint0(100) < chance)
		continue;

	    /* Delete the object */
	    delete_object_idx(i);
	    size--;
	}
    }


    /* Reorder objects */
    compact_objects(0);
}

/*
 * Delete all the items when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 *
 * Hack -- we clear the "cave_o_idx[y][x]" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 */ 
void wipe_o_list(void)
{
    int i;

    /* Delete the existing objects */
    for (i = 1; i < o_max; i++) {
	object_type *o_ptr = &o_list[i];

	/* Skip dead objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Hack -- Preserve unknown artifacts */
	if (artifact_p(o_ptr) && !object_known_p(o_ptr)) {
	    /* Mega-Hack -- Preserve the artifact */
	    a_info[o_ptr->name1].created = FALSE;
	}

	/* Monster */
	if (o_ptr->held_m_idx) {
	    monster_type *m_ptr;

	    /* Monster */
	    m_ptr = &m_list[o_ptr->held_m_idx];

	    /* Hack -- see above */
	    m_ptr->hold_o_idx = 0;
	}

	/* Dungeon */
	else {
	    /* Get the location */
	    int y = o_ptr->iy;
	    int x = o_ptr->ix;

	    /* Hack -- see above */
	    cave_o_idx[y][x] = 0;
	}

	/* Wipe the object */
	(void) WIPE(o_ptr, object_type);
    }

    /* Reset "o_max" */
    o_max = 1;

    /* Reset "o_cnt" */
    o_cnt = 0;
}


/*
 * Get and return the index of a "free" object.
 *
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
s16b o_pop(void)
{
    int i;


    /* Initial allocation */
    if (o_max < z_info->o_max) {
	/* Get next space */
	i = o_max;

	/* Expand object array */
	o_max++;

	/* Count objects */
	o_cnt++;

	/* Use this object */
	return (i);
    }


    /* Recycle dead objects */
    for (i = 1; i < o_max; i++) {
	object_type *o_ptr;

	/* Get the object */
	o_ptr = &o_list[i];

	/* Skip live objects */
	if (o_ptr->k_idx)
	    continue;

	/* Count objects */
	o_cnt++;

	/* Use this object */
	return (i);
    }


    /* Warn the player (except during dungeon creation) */
    if (character_dungeon)
	msg("Too many objects!");

    /* Oops */
    return (0);
}


/*
 * Get the first object at a dungeon location
 * or NULL if there isn't one.
 */
object_type *get_first_object(int y, int x)
{
    s16b o_idx = cave_o_idx[y][x];

    if (o_idx)
	return (&o_list[o_idx]);

    /* No object */
    return (NULL);
}


/*
 * Get the next object in a stack or NULL if there isn't one.
 */
object_type *get_next_object(const object_type * o_ptr)
{
    if (o_ptr->next_o_idx)
	return (&o_list[o_ptr->next_o_idx]);

    /* No more objects */
    return (NULL);
}



/**
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s32b object_value_base(const object_type *o_ptr)
{
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    /* For most items, the "aware" value is simply that of the object kind. */
    s32b aware_cost = k_ptr->cost;

    /* Because weapons and ammo may have enhanced damage dice, their aware
     * value may vary. -LM- */
    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
	{
	    if (o_ptr->ds > k_ptr->ds) {
		aware_cost +=
		    (1 + o_ptr->ds - k_ptr->ds) * (2 + o_ptr->ds -
						   k_ptr->ds) * 1L;
	    }
	    if (o_ptr->dd > k_ptr->dd) {
		aware_cost +=
		    (1 + o_ptr->dd - k_ptr->dd) * (2 + o_ptr->dd -
						   k_ptr->dd) * 1L;
	    }
	    break;
	}

    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
	{
	    if (o_ptr->ds > k_ptr->ds) {
		aware_cost +=
		    (o_ptr->ds - k_ptr->ds) * (o_ptr->ds -
					       k_ptr->ds) * o_ptr->dd *
		    o_ptr->dd * 16L;
	    }
	    if (o_ptr->dd > k_ptr->dd) {
		aware_cost +=
		    (o_ptr->dd - k_ptr->dd) * (o_ptr->ds -
					       k_ptr->dd) * o_ptr->ds *
		    o_ptr->ds * 16L;
	    }
	    break;
	}
    }

    /* Aware item -- use template cost, modified by enhanced dice if needed. */
    if (object_aware_p(o_ptr))
	return (aware_cost);

    /* Analyze the type */
    switch (o_ptr->tval) {
	/* Un-aware Food */
    case TV_FOOD:
	return (5L);

	/* Un-aware Potions */
    case TV_POTION:
	return (20L);

	/* Un-aware Scrolls */
    case TV_SCROLL:
	return (20L);

	/* Un-aware Staffs */
    case TV_STAFF:
	return (80L);

	/* Un-aware Wands */
    case TV_WAND:
	return (80L);

	/* Un-aware Rods */
    case TV_ROD:
	return (200L);

	/* Un-aware Rings (shouldn't happen) */
    case TV_RING:
	return (45L);

	/* Un-aware Amulets (shouldn't happen) */
    case TV_AMULET:
	return (45L);
    }

    /* Paranoia -- Oops */
    return (0L);
}


/**
 * Return the "real" price of a "known" item, not including discounts
 *
 * Wand and staffs get cost for each charge
 *
 * Armor is worth an extra 100 gold per bonus point to armor class.
 *
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).
 *
 * Missiles are only worth 5 gold per bonus point, since they
 * usually appear in groups of 20, and we want the player to get
 * the same amount of cash for any "equivalent" item.  Note that
 * missiles never have any of the "pval" flags, and in fact, they
 * only have a few of the available flags, primarily of the "slay"
 * and "brand" and "ignore" variety.
 *
 * Now reworked to handle all item properties -NRM-
 */
static s32b object_value_real(const object_type *o_ptr)
{
    int i;

    s32b value;

    object_kind *k_ptr = &k_info[o_ptr->k_idx];


    /* Hack -- "worthless" items */
    if (!k_ptr->cost)
	return (0L);

    /* Base cost */
    value = k_ptr->cost;


    /* Artifact */
    if (o_ptr->name1) {
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	/* Hack -- Use the artifact cost instead */
	return (a_ptr->cost);
    }

    /* Ego-Item */
    else if (has_ego_properties(o_ptr)) {
	ego_item_type *e_ptr = &e_info[o_ptr->name2];

	/* Hack -- Reward the ego-item with a bonus */
	value += e_ptr->cost;
    }

    /* Analyze resists, bonuses and multiples */
    switch (o_ptr->tval) {
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_LIGHT:
    case TV_AMULET:
    case TV_RING:
	{

	    /* Give credit for resists */
	    if (if_has(o_ptr->id_other, IF_RES_ACID))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_ACID]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_ELEC))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_ELEC]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_FIRE))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_FIRE]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_COLD))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_COLD]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_POIS))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_POIS]) * 120L;
	    if (if_has(o_ptr->id_other, IF_RES_LIGHT))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_LIGHT]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_DARK))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_DARK]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_CONFU))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_CONFU]) * 90L;
	    if (if_has(o_ptr->id_other, IF_RES_SOUND))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_SOUND]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_SHARD))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_SHARD]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_NEXUS))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_NEXUS]) * 60L;
	    if (if_has(o_ptr->id_other, IF_RES_NETHR))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_NETHR]) * 90L;
	    if (if_has(o_ptr->id_other, IF_RES_CHAOS))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_CHAOS]) * 90L;
	    if (if_has(o_ptr->id_other, IF_RES_DISEN))
		value +=
		    (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_DISEN]) * 120L;

	    /* Wearing shows all bonuses */
	    if (o_ptr->ident & IDENT_WORN) {
		/* Give credit for stat bonuses. */
		for (i = 0; i < A_MAX; i++) {
		    if (o_ptr->bonus_stat[i] > BONUS_BASE)
			value +=
			    (((o_ptr->bonus_stat[i] +
			       1) * (o_ptr->bonus_stat[i] + 1)) * 100L);
		    else if (o_ptr->bonus_stat[i] < BONUS_BASE)
			value -=
			    (o_ptr->bonus_stat[i] * o_ptr->bonus_stat[i] *
			     100L);
		}

		/* Give credit for magic items bonus. */
		value += (o_ptr->bonus_other[P_BONUS_M_MASTERY] * 600L);

		/* Give credit for stealth and searching */
		value += (o_ptr->bonus_other[P_BONUS_STEALTH] * 100L);
		value += (o_ptr->bonus_other[P_BONUS_SEARCH] * 40L);

		/* Give credit for infra-vision */
		value += (o_ptr->bonus_other[P_BONUS_INFRA] * 50L);

		/* Give credit for tunneling bonus above that which a digger
		 * possesses intrinsically. */
		value +=
		    ((o_ptr->bonus_other[P_BONUS_TUNNEL] -
		      k_ptr->bonus_other[P_BONUS_TUNNEL]) * 50L);


		/* Give credit for speed bonus.  Formula changed to avoid
		 * excessively valuable low-speed items. */
		if (o_ptr->bonus_other[P_BONUS_SPEED] > 0)
		    value +=
			(o_ptr->bonus_other[P_BONUS_SPEED] *
			 o_ptr->bonus_other[P_BONUS_SPEED] * 5000L);
		else if (o_ptr->bonus_other[P_BONUS_SPEED] < 0)
		    value -=
			(o_ptr->bonus_other[P_BONUS_SPEED] *
			 o_ptr->bonus_other[P_BONUS_SPEED] * 2000L);

		/* Give credit for extra shots */
		value += (o_ptr->bonus_other[P_BONUS_SHOTS] * 8000L);

		/* Give credit for extra might */
		value += (o_ptr->bonus_other[P_BONUS_MIGHT] * 6000L);
	    }

	    /* Give credit for slays */
	    if (if_has(o_ptr->id_other, IF_SLAY_ANIMAL))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_ANIMAL] -
		     MULTIPLE_BASE) * 150L;
	    if (if_has(o_ptr->id_other, IF_SLAY_EVIL))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_EVIL] - MULTIPLE_BASE) * 500L;
	    if (if_has(o_ptr->id_other, IF_SLAY_UNDEAD))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_UNDEAD] -
		     MULTIPLE_BASE) * 200L;
	    if (if_has(o_ptr->id_other, IF_SLAY_DEMON))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_DEMON] - MULTIPLE_BASE) * 200L;
	    if (if_has(o_ptr->id_other, IF_SLAY_ORC))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_ORC] - MULTIPLE_BASE) * 100L;
	    if (if_has(o_ptr->id_other, IF_SLAY_TROLL))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_TROLL] - MULTIPLE_BASE) * 150L;
	    if (if_has(o_ptr->id_other, IF_SLAY_GIANT))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_GIANT] - MULTIPLE_BASE) * 100L;
	    if (if_has(o_ptr->id_other, IF_SLAY_DRAGON))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_DRAGON] -
		     MULTIPLE_BASE) * 200L;

	    /* Give credit for brands */
	    if (if_has(o_ptr->id_other, IF_BRAND_ACID))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_ACID] -
		     MULTIPLE_BASE) * 300L;
	    if (if_has(o_ptr->id_other, IF_BRAND_ELEC))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_ELEC] -
		     MULTIPLE_BASE) * 300L;
	    if (if_has(o_ptr->id_other, IF_BRAND_FIRE))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_FIRE] -
		     MULTIPLE_BASE) * 200L;
	    if (if_has(o_ptr->id_other, IF_BRAND_COLD))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_COLD] -
		     MULTIPLE_BASE) * 200L;
	    if (if_has(o_ptr->id_other, IF_BRAND_POIS))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_POIS] -
		     MULTIPLE_BASE) * 150L;

	    /* Give credit for object flags */
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_STR))
		value += 500L;
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_INT))
		value += 300L;
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_WIS))
		value += 300L;
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_DEX))
		value += 400L;
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_CON))
		value += 400L;
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_CHR))
		value += 50L;
	    if (of_has(o_ptr->id_obj, OF_SLOW_DIGEST))
		value += 200L;
	    if (of_has(o_ptr->id_obj, OF_FEATHER))
		value += 200L;
	    if (of_has(o_ptr->id_obj, OF_REGEN))
		value += 400L;
	    if (of_has(o_ptr->id_obj, OF_TELEPATHY))
		value += 5000L;
	    if (of_has(o_ptr->id_obj, OF_SEE_INVIS))
		value += 700L;
	    if (of_has(o_ptr->id_obj, OF_FREE_ACT))
		value += 800L;
	    if (of_has(o_ptr->id_obj, OF_HOLD_LIFE))
		value += 700L;
	    if (of_has(o_ptr->id_obj, OF_SEEING))
		value += 700L;
	    if (of_has(o_ptr->id_obj, OF_FEARLESS))
		value += 500L;
	    if (of_has(o_ptr->id_obj, OF_LIGHT))
		value += 300L;
	    if (of_has(o_ptr->id_obj, OF_BLESSED))
		value += 1000L;
	    if (of_has(o_ptr->id_obj, OF_IMPACT))
		value += 50L;
	    if (of_has(o_ptr->id_obj, OF_ACID_PROOF))
		value += 100L;
	    if (of_has(o_ptr->id_obj, OF_ELEC_PROOF))
		value += 100L;
	    if (of_has(o_ptr->id_obj, OF_FIRE_PROOF))
		value += 100L;
	    if (of_has(o_ptr->id_obj, OF_COLD_PROOF))
		value += 100L;
	    if (of_has(o_ptr->id_obj, OF_DARKNESS))
		value += 1000L;

	    /* Give 'credit' for curse flags */
	    if (cf_has(o_ptr->id_curse, CF_TELEPORT))
		value -= 200L;
	    if (cf_has(o_ptr->id_curse, CF_NO_TELEPORT))
		value -= 1000L;
	    if (cf_has(o_ptr->id_curse, CF_AGGRO_PERM))
		value -= 2000L;
	    if (cf_has(o_ptr->id_curse, CF_AGGRO_RAND))
		value -= 700L;
	    if (cf_has(o_ptr->id_curse, CF_SLOW_REGEN))
		value -= 200L;
	    if (cf_has(o_ptr->id_curse, CF_AFRAID))
		value -= 500L;
	    if (cf_has(o_ptr->id_curse, CF_HUNGRY))
		value -= 200L;
	    if (cf_has(o_ptr->id_curse, CF_POIS_RAND))
		value -= 200L;
	    if (cf_has(o_ptr->id_curse, CF_POIS_RAND_BAD))
		value -= 500L;
	    if (cf_has(o_ptr->id_curse, CF_CUT_RAND))
		value -= 200L;
	    if (cf_has(o_ptr->id_curse, CF_CUT_RAND_BAD))
		value -= 400L;
	    if (cf_has(o_ptr->id_curse, CF_HALLU_RAND))
		value -= 800L;
	    if (cf_has(o_ptr->id_curse, CF_DROP_WEAPON))
		value -= 500L;
	    if (cf_has(o_ptr->id_curse, CF_ATTRACT_DEMON))
		value -= 800L;
	    if (cf_has(o_ptr->id_curse, CF_ATTRACT_UNDEAD))
		value -= 900L;
	    if (cf_has(o_ptr->id_curse, CF_STICKY_WIELD))
		value -= 2500L;
	    if (cf_has(o_ptr->id_curse, CF_STICKY_CARRY))
		value -= 1000L;
	    if (cf_has(o_ptr->id_curse, CF_PARALYZE))
		value -= 800L;
	    if (cf_has(o_ptr->id_curse, CF_PARALYZE_ALL))
		value -= 2000L;
	    if (cf_has(o_ptr->id_curse, CF_DRAIN_EXP))
		value -= 800L;
	    if (cf_has(o_ptr->id_curse, CF_DRAIN_MANA))
		value -= 1000L;
	    if (cf_has(o_ptr->id_curse, CF_DRAIN_STAT))
		value -= 1500L;
	    if (cf_has(o_ptr->id_curse, CF_DRAIN_CHARGE))
		value -= 1500L;

	    break;
	}
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
	{
	    /* Give credit for slays */
	    if (if_has(o_ptr->id_other, IF_SLAY_ANIMAL))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_ANIMAL] - MULTIPLE_BASE) * 15L;
	    if (if_has(o_ptr->id_other, IF_SLAY_EVIL))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_EVIL] - MULTIPLE_BASE) * 50L;
	    if (if_has(o_ptr->id_other, IF_SLAY_UNDEAD))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_UNDEAD] - MULTIPLE_BASE) * 20L;
	    if (if_has(o_ptr->id_other, IF_SLAY_DEMON))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_DEMON] - MULTIPLE_BASE) * 20L;
	    if (if_has(o_ptr->id_other, IF_SLAY_ORC))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_ORC] - MULTIPLE_BASE) * 10L;
	    if (if_has(o_ptr->id_other, IF_SLAY_TROLL))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_TROLL] - MULTIPLE_BASE) * 15L;
	    if (if_has(o_ptr->id_other, IF_SLAY_GIANT))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_GIANT] - MULTIPLE_BASE) * 10L;
	    if (if_has(o_ptr->id_other, IF_SLAY_DRAGON))
		value +=
		    (o_ptr->multiple_slay[P_SLAY_DRAGON] - MULTIPLE_BASE) * 20L;

	    /* Give credit for brands */
	    if (if_has(o_ptr->id_other, IF_BRAND_ACID))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_ACID] - MULTIPLE_BASE) * 30L;
	    if (if_has(o_ptr->id_other, IF_BRAND_ELEC))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_ELEC] - MULTIPLE_BASE) * 30L;
	    if (if_has(o_ptr->id_other, IF_BRAND_FIRE))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_FIRE] - MULTIPLE_BASE) * 20L;
	    if (if_has(o_ptr->id_other, IF_BRAND_COLD))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_COLD] - MULTIPLE_BASE) * 20L;
	    if (if_has(o_ptr->id_other, IF_BRAND_POIS))
		value +=
		    (o_ptr->multiple_brand[P_BRAND_POIS] - MULTIPLE_BASE) * 15L;

	    break;
	}
    }


    /* Analyze the item */
    switch (o_ptr->tval) {
	/* Wands/Staffs */
    case TV_WAND:
	{
	    int temp_pval = randcalc(k_ptr->pval, k_ptr->level, RANDOMISE);
	    /* Pay extra for charges, depending on standard number of charges.
	     * Handle new-style wands correctly. */
	    value += (value * o_ptr->pval / o_ptr->number / (temp_pval * 2));

	    /* Done */
	    break;
	}
    case TV_STAFF:
	{
	    /* Pay extra for charges, depending on standard number of charges */
	    int temp_pval = randcalc(k_ptr->pval, k_ptr->level, RANDOMISE);
	    value += (value * o_ptr->pval / o_ptr->number / (temp_pval * 2));

	    /* Done */
	    break;
	}

	/* Rings/Amulets */
    case TV_RING:
    case TV_AMULET:
	{
	    /* Give credit for bonuses */
	    if (if_has(o_ptr->id_other, IF_TO_H))
		value += o_ptr->to_h * 100L;
	    if (if_has(o_ptr->id_other, IF_TO_D))
		value += o_ptr->to_d * 100L;
	    if (if_has(o_ptr->id_other, IF_TO_A))
		value += o_ptr->to_a * 100L;

	    /* Done */
	    break;
	}

	/* Armor */
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
	{
	    /* If it differs from the object kind's base armour Skill penalty
	     * (this penalty must be in the range +0 to -12), give a debit or
	     * credit for any modification to Skill. -LM- */
	    if (((o_ptr->to_h != k_ptr->to_h.base) || (o_ptr->to_h < -12)
		 || (o_ptr->to_h > 0)) && (if_has(o_ptr->id_other, IF_TO_H)))
		value += (o_ptr->to_h * 100L);

	    /* Standard debit or credit for to_d and to_a bonuses. */
	    if (if_has(o_ptr->id_other, IF_TO_D))
		value += (o_ptr->to_d) * 100L;
	    if (if_has(o_ptr->id_other, IF_TO_A))
		value += (o_ptr->to_a) * 100L;

	    /* Done */
	    break;
	}

	/* Sharp weapons */
    case TV_DIGGING:
    case TV_SWORD:
    case TV_POLEARM:
	{
	    /* Blessed */
	    if (of_has(o_ptr->id_obj, OF_BLESSED))
		value += 400L;

	    /* Fall through */
	}

	/* Bows/Weapons */
    case TV_BOW:
    case TV_HAFTED:
	{
	    /* Because +to_hit and +to_dam work differently now, this formula
	     * also needed to change.  Although long, it now yields results
	     * that match the increase in the weapon's combat power.  -LM- */
	    int hit = (if_has(o_ptr->id_other, IF_TO_H)) ? o_ptr->to_h : 0;
	    int dam = (if_has(o_ptr->id_other, IF_TO_D)) ? o_ptr->to_d : 0;
	    int arm = (if_has(o_ptr->id_other, IF_TO_A)) ? o_ptr->to_a : 0;
	    value +=
		(((hit + 10) * (dam + 10) - 100) * (k_ptr->cost +
						    1000L) / 250) + (arm * 100);


	    /* Hack -- Factor in improved damage dice.  If you want to buy one
	     * of the truly dangerous weapons, be prepared to pay for it. */
	    if ((o_ptr->ds > k_ptr->ds) && (o_ptr->dd == k_ptr->dd)
		&& (if_has(o_ptr->id_other, IF_DD_DS))) {
		value += (o_ptr->ds - k_ptr->ds) * (o_ptr->ds - k_ptr->ds)
		    * o_ptr->dd * o_ptr->dd * 16L;

		/* Naturally, ego-items with high base dice should be very
		 * expensive. */
		if (has_ego_properties(o_ptr)) {
		    value += (5 * value / 2);
		}
	    }

	    /* Give credit for perfect balance. */
	    if (of_has(o_ptr->id_obj, OF_PERFECT_BALANCE))
		value += o_ptr->dd * 200L;

	    /* Done */
	    break;
	}

	/* Ammo */
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
	{
	    /* Factor in the bonuses */
	    if (if_has(o_ptr->id_other, IF_TO_H))
		value += o_ptr->to_h * 2L;
	    if (if_has(o_ptr->id_other, IF_TO_D))
		value += o_ptr->to_d * 2L;


	    /* Hack -- Factor in improved damage dice. -LM- */
	    if ((o_ptr->ds > k_ptr->ds) && (if_has(o_ptr->id_other, IF_DD_DS))) {
		value +=
		    (1 + o_ptr->ds - k_ptr->ds) * (2 + o_ptr->ds -
						   k_ptr->ds) * 1L;

		/* Naturally, ego-items with high base dice should be
		 * expensive. */
		if (has_ego_properties(o_ptr)) {
		    value += (5 * value / 2);
		}
	    }

	    /* Done */
	    break;
	}

	/* Who wants an empty chest? */
    case TV_CHEST:
	{
	    if (o_ptr->pval == 0)
		return (0L);
	    break;
	}
    }


    /* Return the value */
    return (value < 0 ? 0L : value);
}

/**
 * Return the price of an item including plusses (and charges)
 *
 * This function returns the "value" of the given item (qty one)
 *
 * Never notice "unknown" bonuses or properties, including "curses",
 * since that would give the player information he did not have.
 *
 * Note that discounted items stay discounted forever, even if
 * the discount is "forgotten" by the player via memory loss.
 */
s32b object_value(const object_type * o_ptr)
{
    s32b value;


    /* Known items -- acquire the actual value */
    if (object_known_p(o_ptr)
	|| ((wield_slot(o_ptr) > 0) && !artifact_p(o_ptr))) {
	/* Real value (see above) */
	value = object_value_real(o_ptr);
    }

    /* Unknown items -- acquire a base value */
    else {
	/* Base value (see above) */
	value = object_value_base(o_ptr);
    }


    /* Apply discount (if any) */
    if (o_ptr->discount)
	value -= (value * o_ptr->discount / 100L);


    /* Return the final value */
    return (value);
}

/*
 * Determine if an item can "absorb" a second item
 *
 * See "object_absorb()" for the actual "absorption" code.
 *
 * If permitted, we allow weapons/armor to stack, if "known".
 *
 * Missiles will combine if both stacks have the same "known" status.
 * This is done to make unidentified stacks of missiles useful.
 *
 * Food, potions, scrolls, and "easy know" items always stack.
 *
 * Chests, and activatable items, except rods, never stack (for various
 * reasons).
 */
bool object_similar(const object_type * o_ptr, const object_type * j_ptr,
		    object_stack_t mode)
{
    int i;
    int total = o_ptr->number + j_ptr->number;

    /* Check against stacking limit - except in stores which absorb anyway */
    if (!(mode & OSTACK_STORE) && (total >= MAX_STACK_SIZE))
	return FALSE;

    /* Hack -- identical items cannot be stacked */
    if (o_ptr == j_ptr)
	return FALSE;

    /* Require identical object kinds */
    if (o_ptr->k_idx != j_ptr->k_idx)
	return FALSE;

    /* Require identical effects */
    if (o_ptr->effect != j_ptr->effect)
	return (FALSE);

    /* Different flags don't stack */
    if (!of_is_equal(o_ptr->flags_obj, j_ptr->flags_obj))
	return FALSE;
    if (!cf_is_equal(o_ptr->flags_curse, j_ptr->flags_curse))
	return FALSE;

    /* Require identical resists, etc */
    for (i = 0; i < MAX_P_RES; i++)
	if (o_ptr->percent_res[i] != j_ptr->percent_res[i])
	    return (FALSE);
    for (i = 0; i < A_MAX; i++)
	if (o_ptr->bonus_stat[i] != j_ptr->bonus_stat[i])
	    return (FALSE);
    for (i = 0; i < MAX_P_BONUS; i++)
	if (o_ptr->bonus_other[i] != j_ptr->bonus_other[i])
	    return (FALSE);
    for (i = 0; i < MAX_P_SLAY; i++)
	if (o_ptr->multiple_slay[i] != j_ptr->multiple_slay[i])
	    return (FALSE);
    for (i = 0; i < MAX_P_BRAND; i++)
	if (o_ptr->multiple_brand[i] != j_ptr->multiple_brand[i])
	    return (FALSE);

    /* Artifacts never stack */
    if (o_ptr->name1 || j_ptr->name1)
	return FALSE;

    /* Analyze the items */
    switch (o_ptr->tval) {
	/* Chests never stack */
    case TV_CHEST:
	{
	    /* Never okay */
	    return FALSE;
	}

	/* Food, potions, scrolls and rods all stack nicely */
    case TV_FOOD:
    case TV_POTION:
    case TV_SCROLL:
    case TV_ROD:
	{
	    /* Since the kinds are identical, either both will be aware or both 
	     * will be unaware */
	    break;
	}

	/* Gold, staves and wands stack most of the time */
    case TV_STAFF:
    case TV_WAND:
    case TV_GOLD:
	{
	    /* Too much gold or too many charges */
	    if (o_ptr->pval + j_ptr->pval > MAX_PVAL)
		return FALSE;

	    /* ... otherwise ok */
	    else
		break;
	}

	/* Weapons, ammo, armour, jewelry, lights */
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_RING:
    case TV_AMULET:
    case TV_LIGHT:
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
	{
	    /* Require identical values */
	    if (o_ptr->ac != j_ptr->ac)
		return FALSE;
	    if (o_ptr->dd != j_ptr->dd)
		return FALSE;
	    if (o_ptr->ds != j_ptr->ds)
		return FALSE;

	    /* Require identical bonuses */
	    if (o_ptr->to_h != j_ptr->to_h)
		return FALSE;
	    if (o_ptr->to_d != j_ptr->to_d)
		return FALSE;
	    if (o_ptr->to_a != j_ptr->to_a)
		return FALSE;

	    /* Require identical pval */
	    if (o_ptr->pval != j_ptr->pval)
		return (FALSE);

	    /* Require identical ego-item types */
	    if (o_ptr->name2 != j_ptr->name2)
		return (FALSE);

	    /* Hack - Never stack recharging wearables ... */
	    if ((o_ptr->timeout || j_ptr->timeout) && (o_ptr->tval != TV_LIGHT))
		return FALSE;

	    /* ... and lights must have same amount of fuel */
	    else if ((o_ptr->timeout != j_ptr->timeout)
		     && (o_ptr->tval == TV_LIGHT))
		return FALSE;

	    /* Probably okay */
	    break;
	}

	/* Anything else */
    default:
	{
	    /* Require knowledge */
	    //if (!object_known_p(o_ptr) || !object_known_p(j_ptr))
	    //return (FALSE);

	    /* Probably okay */
	    break;
	}
    }

    /* Hack -- Require compatible inscriptions */
    if (o_ptr->note != j_ptr->note) {
	/* Never combine different inscriptions */
	if (o_ptr->note && j_ptr->note)
	    return (FALSE);
    }

    /* They must be similar enough */
    return (TRUE);
}


/*
 * Allow one item to "absorb" another, assuming they are similar.
 *
 * The blending of the "note" field assumes that either (1) one has an
 * inscription and the other does not, or (2) neither has an inscription.
 * In both these cases, we can simply use the existing note, unless the
 * blending object has a note, in which case we use that note.
 *
 * The blending of the "discount" field assumes that either (1) one is a
 * special inscription and one is nothing, or (2) one is a discount and
 * one is a smaller discount, or (3) one is a discount and one is nothing,
 * or (4) both are nothing.  In all of these cases, we can simply use the
 * "maximum" of the two "discount" fields.
 *
 * These assumptions are enforced by the "object_similar()" code.
 */
void object_absorb(object_type * o_ptr, const object_type * j_ptr)
{
    int total = o_ptr->number + j_ptr->number;

    /* Add together the item counts */
    o_ptr->number = ((total < MAX_STACK_SIZE) ? total : (MAX_STACK_SIZE - 1));

    /* Blend all knowledge */
    o_ptr->ident |= (j_ptr->ident & ~IDENT_EMPTY);
    of_union(o_ptr->id_obj, j_ptr->id_obj);
    cf_union(o_ptr->id_curse, j_ptr->id_curse);
    if_union(o_ptr->id_other, j_ptr->id_other);

    /* Hack -- Blend "notes" */
    if (j_ptr->note != 0)
	o_ptr->note = j_ptr->note;

    /* Hack -- save largest discount XXX XXX XXX */
    if (o_ptr->discount < j_ptr->discount)
	o_ptr->discount = j_ptr->discount;

    /* Hack -- blend "feelings" */
    if (o_ptr->feel && j_ptr->feel)
	o_ptr->feel = j_ptr->feel;

    /* Hack -- if rods are stacking, re-calculate the timeouts */
    if (o_ptr->tval == TV_ROD) {
	o_ptr->timeout += j_ptr->timeout;
    }

    /* Hack -- if wands or staves are stacking, combine the charges */
    /* If gold is stacking combine the amount */
    if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF)
	|| (o_ptr->tval == TV_GOLD)) {
	int total = o_ptr->pval + j_ptr->pval;
	o_ptr->pval = total >= MAX_PVAL ? MAX_PVAL : total;
    }

    if ((o_ptr->origin != j_ptr->origin)
	|| (o_ptr->origin_stage != j_ptr->origin_stage)
	|| (o_ptr->origin_xtra != j_ptr->origin_xtra)) {
	int act = 2;

	if ((o_ptr->origin == ORIGIN_DROP) && (o_ptr->origin == j_ptr->origin))
	{
	    monster_race *r_ptr = &r_info[o_ptr->origin_xtra];
	    monster_race *s_ptr = &r_info[j_ptr->origin_xtra];
	    
	    bool r_uniq = rf_has(r_ptr->flags, RF_UNIQUE) ? TRUE : FALSE;
	    bool s_uniq = rf_has(s_ptr->flags, RF_UNIQUE) ? TRUE : FALSE;

	    if (r_uniq && !s_uniq)
		act = 0;
	    else if (s_uniq && !r_uniq)
		act = 1;
	    else
		act = 2;
	}

	switch (act) {
	    /* Overwrite with j_ptr */
	case 1:
	{
	    o_ptr->origin = j_ptr->origin;
	    o_ptr->origin_stage = j_ptr->origin_stage;
	    o_ptr->origin_xtra = j_ptr->origin_xtra;
	}
	
	/* Set as "mixed" */
	case 2:
	{
	    o_ptr->origin = ORIGIN_MIXED;
	}
	}
    }
}



/*
 * Wipe an object clean.
 */
void object_wipe(object_type * o_ptr)
{
    /* Wipe the structure */
    (void) WIPE(o_ptr, object_type);
}


/*
 * Prepare an object based on an existing object
 */
void object_copy(object_type * o_ptr, const object_type * j_ptr)
{
    /* Copy the structure */
    COPY(o_ptr, j_ptr, object_type);
}

/*
 * Prepare an object `dst` representing `amt` objects,  based on an existing 
 * object `src` representing at least `amt` objects.
 *
 * Takes care of the charge redistribution concerns of stacked items.
 */ 
void object_copy_amt(object_type * dst, object_type * src, int amt)
{
    const object_kind *k_ptr = &k_info[src->k_idx];
    int charge_time = randcalc(k_ptr->time, 0, AVERAGE), max_time;

    /* Get a copy of the object */
    object_copy(dst, src);

    /* Modify quantity */
    dst->number = amt;

    /* 
     * If the item has charges/timeouts, set them to the correct level 
     * too. We split off the same amount as distribute_charges.
     */
    if (src->tval == TV_WAND || src->tval == TV_STAFF) {
	dst->pval = src->pval * amt / src->number;
    }

    if (src->tval == TV_ROD) {
	max_time = charge_time * amt;

	if (src->timeout > max_time)
	    dst->timeout = max_time;
	else
	    dst->timeout = src->timeout;
    }
}


/**
 * Find and return the index to the oldest object on the given grid marked as
 * "squelch".
 */
static s16b floor_get_idx_oldest_squelched(int y, int x)
{
    s16b squelch_idx = 0;
    s16b this_o_idx;

    object_type *o_ptr = NULL;

    for (this_o_idx = cave_o_idx[y][x]; this_o_idx;
	 this_o_idx = o_ptr->next_o_idx) {
	o_ptr = &o_list[this_o_idx];

	if (squelch_hide_item(o_ptr))
	    squelch_idx = this_o_idx;
    }
    return squelch_idx;
}



/*
 * Let the floor carry an object, deleting old squelched items if necessary
 */
s16b floor_carry(int y, int x, object_type * j_ptr)
{
    int n = 0;

    s16b o_idx;

    s16b this_o_idx, next_o_idx = 0;


    /* Scan objects in that grid for combination */
    for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr = &o_list[this_o_idx];

	/* Get the next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Check for combination */
	if (object_similar(o_ptr, j_ptr, OSTACK_FLOOR)) {
	    /* Combine the items */
	    object_absorb(o_ptr, j_ptr);

	    /* Result */
	    return (this_o_idx);
	}

	/* Count objects */
	n++;
    }

    /* The stack is already too large */
    if (n >= MAX_FLOOR_STACK) {
	/* Squelch the oldest squelched object */
	s16b squelch_idx = floor_get_idx_oldest_squelched(y, x);

	if (squelch_idx)
	    delete_object_idx(squelch_idx);
	else
	    return 0;
    }


    /* Make an object */
    o_idx = o_pop();

    /* Success */
    if (o_idx) {
	object_type *o_ptr;

	/* Get the object */
	o_ptr = &o_list[o_idx];

	/* Structure Copy */
	object_copy(o_ptr, j_ptr);

	/* Location */
	o_ptr->iy = y;
	o_ptr->ix = x;

	/* Forget monster */
	o_ptr->held_m_idx = 0;

	/* Link the object to the pile */
	o_ptr->next_o_idx = cave_o_idx[y][x];

	/* Link the floor to the object */
	cave_o_idx[y][x] = o_idx;

	/* Notice */
	note_spot(y, x);

	/* Redraw */
	light_spot(y, x);
    }

    /* Result */
    return (o_idx);
}


/*
 * Let an object fall to the ground at or near a location.
 *
 * The initial location is assumed to be "in_bounds_fully()".
 *
 * This function takes a parameter "chance".  This is the percentage
 * chance that the item will "disappear" instead of drop.  If the object
 * has been thrown, then this is the chance of disappearance on contact.
 *
 * This function will produce a description of a drop event under the player
 * when "verbose" is true.
 *
 * We check several locations to see if we can find a location at which
 * the object can combine, stack, or be placed.  Artifacts will try very
 * hard to be placed, including "teleporting" to a useful grid if needed.
 */
void drop_near(object_type * j_ptr, int chance, int y, int x, bool verbose)
{
    int i, k, n, d, s;

    int bs, bn;
    int by, bx;
    int dy, dx;
    int ty, tx;

    object_type *o_ptr;
    feature_type *f_ptr;

    char o_name[80];

    bool flag = FALSE;

    bool plural = FALSE;


    /* Extract plural */
    if (j_ptr->number != 1)
	plural = TRUE;

    /* Describe object */
    object_desc(o_name, sizeof(o_name), j_ptr, ODESC_BASE);


    /* Handle normal "breakage" */
    if (!artifact_p(j_ptr) && (randint0(100) < chance)) {
	/* Message */
	msg("The %s break%s.", o_name, PLURAL(plural));

	/* Failure */
	return;
    }


    /* Score */
    bs = -1;

    /* Picker */
    bn = 0;

    /* Default */
    by = y;
    bx = x;

    /* Scan local grids */
    for (dy = -3; dy <= 3; dy++) {
	/* Scan local grids */
	for (dx = -3; dx <= 3; dx++) {
	    bool comb = FALSE;

	    /* Calculate actual distance */
	    d = (dy * dy) + (dx * dx);

	    /* Ignore distant grids */
	    if (d > 10)
		continue;

	    /* Location */
	    ty = y + dy;
	    tx = x + dx;
	    f_ptr = &f_info[cave_feat[ty][tx]];

	    /* Skip illegal grids */
	    if (!in_bounds_fully(ty, tx))
		continue;

	    /* Require line of sight */
	    if (!los(y, x, ty, tx))
		continue;

	    /* Require floor space */
	    if (!tf_has(f_ptr->flags, TF_OBJECT))
		continue;

	    /* No objects */
	    k = 0;
	    n = 0;

	    /* Scan objects in that grid */
	    for (o_ptr = get_first_object(ty, tx); o_ptr;
		 o_ptr = get_next_object(o_ptr)) {
		/* Check for possible combination */
		if (object_similar(o_ptr, j_ptr, OSTACK_FLOOR))
		    comb = TRUE;

		/* Count objects */
		if (!squelch_hide_item(o_ptr))
		    k++;
		else
		    n++;
	    }

	    /* Add new object */
	    if (!comb)
		k++;

	    /* Paranoia? */
	    if ((k + n) > MAX_FLOOR_STACK
		&& !floor_get_idx_oldest_squelched(ty, tx))
		continue;

	    /* Calculate goodness of location, given distance from source of
	     * drop and number of objects.  Hack - If the player is dropping
	     * the item, encourage it to pile with up to 19 other items. */
	    if (cave_m_idx[y][x] < 0) {
		s = 1000 - (d + (k > 20 ? k * 5 : 0));
	    } else {
		s = 1000 - (d + k * 5);
	    }

	    /* Skip bad values */
	    if (s < bs)
		continue;

	    /* New best value */
	    if (s > bs)
		bn = 0;

	    /* Apply the randomizer to equivalent values */
	    if ((++bn >= 2) && (randint0(bn) != 0))
		continue;

	    /* Keep score */
	    bs = s;

	    /* Track it */
	    by = ty;
	    bx = tx;

	    /* Okay */
	    flag = TRUE;
	}
    }


    /* Handle lack of space */
    if (!flag && !artifact_p(j_ptr)) {
	/* Message */
	msg("The %s disappear%s.", o_name, PLURAL(plural));

	/* Debug */
	if (p_ptr->wizard)
	    msg("Breakage (no floor space).");

	/* Failure */
	return;
    }


    /* Find a grid */
    for (i = 0; !flag; i++) 
    {
	/* Bounce around */
	if (i < 1000) 
	{
	    ty = rand_spread(by, 1);
	    tx = rand_spread(bx, 1);
	}

	/* Random locations */
	else 
	{
	    ty = randint0(level_hgt);
	    tx = randint0(level_wid);
	}
	f_ptr = &f_info[cave_feat[ty][tx]];

	/* Require floor space */
	if (!tf_has(f_ptr->flags, TF_OBJECT))
	    continue;

	/* Bounce to that location */
	by = ty;
	bx = tx;
	f_ptr = &f_info[cave_feat[by][bx]];

	/* Okay */
	flag = TRUE;
    }


    /* Give it to the floor */
    if (!floor_carry(by, bx, j_ptr)) {
	artifact_type *a_ptr = artifact_of(j_ptr);

	/* Message */
	msg("The %s disappear%s.", o_name, PLURAL(plural));

	/* Debug */
	if (p_ptr->wizard)
	    msg("Breakage (too many objects).");

	if (a_ptr)
	    a_ptr->created = FALSE;

	/* Failure */
	return;
    }


    /* Sound */
    sound(MSG_DROP);

    /* Confirm location */
    f_ptr = &f_info[cave_feat[by][bx]];

    /* Message when an object falls under the player */
    if (verbose && (cave_m_idx[by][bx] < 0) && !squelch_item_ok(j_ptr)) {
	msg("You feel something roll beneath your feet.");
    }

    /* Message when an object falls under the player or in trees or rubble */
    else if (tf_has(f_ptr->flags, TF_HIDE_OBJ) && !p_ptr->timed[TMD_BLIND])
	msg("The %s disappear%s from view.", o_name, PLURAL(plural));
}


/*
 * Scatter some "great" objects near the player
 */
void acquirement(int y1, int x1, int num, bool great)
{
    object_type *i_ptr;
    object_type object_type_body;

    /* Acquirement */
    while (num--) {
	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make a good (or great) object (if possible) */
	if (!make_object(i_ptr, TRUE, great, FALSE))
	    continue;
	i_ptr->origin = ORIGIN_ACQUIRE;
	i_ptr->origin_stage = p_ptr->stage;

	/* Drop the object */
	drop_near(i_ptr, 0, y1, x1, TRUE);
    }
}

/*
 * Describe the charges on an item in the inventory.
 */ void inven_item_charges(int item)
{
    object_type *o_ptr = &p_ptr->inventory[item];

    /* Require staff/wand */
    if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND))
	return;

    /* Require known item */
    if (!object_known_p(o_ptr))
	return;

    /* Print a message */
    msg("You have %d charge%s remaining.", o_ptr->pval,
	       (o_ptr->pval != 1) ? "s" : "");
}

/*
 * Describe an item in the inventory.
 */ void inven_item_describe(int item)
{
    object_type *o_ptr = &p_ptr->inventory[item];

    char o_name[80];

    if (artifact_p(o_ptr) && object_known_p(o_ptr)) {
	/* Get a description */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_FULL);

	/* Print a message */
	msg("You no longer have the %s (%c).", o_name,
		   index_to_label(item));
    } else {
	/* Get a description */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

	/* Print a message */
	msg("You have %s (%c).", o_name, index_to_label(item));
    }
}


/*
 * Increase the "number" of an item in the inventory
 */
void inven_item_increase(int item, int num)
{
    object_type *o_ptr = &p_ptr->inventory[item];

    /* Apply */
    num += o_ptr->number;

    /* Bounds check */
    if (num > 255)
	num = 255;
    else if (num < 0)
	num = 0;

    /* Un-apply */
    num -= o_ptr->number;

    /* Change the number and weight */
    if (num) {
	/* Add the number */
	o_ptr->number += num;

	/* Add the weight */
	p_ptr->total_weight += (num * o_ptr->weight);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate mana XXX */
	p_ptr->update |= (PU_MANA);

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
    }
}


/**
 * Save the size of the quiver.
 */
void save_quiver_size(struct player *p)
{
	int i, count = 0;
	for (i = QUIVER_START; i < QUIVER_END; i++)
	    if (p->inventory[i].k_idx) {
		if (is_missile(&p->inventory[i]))
		    count += p->inventory[i].number;
		else {
		    int equiv = p->inventory[i].number * THROWER_AMMO_FACTOR;
		    count += equiv;
		    if ((QUIVER_SLOT_SIZE - equiv) < THROWER_AMMO_FACTOR)
			count += QUIVER_SLOT_SIZE - equiv;
		}
	    }

	p->quiver_size = count;
	p->quiver_slots = (count + QUIVER_SLOT_SIZE - 1) / QUIVER_SLOT_SIZE;
	p->quiver_remainder = count % QUIVER_SLOT_SIZE;
}


/**
 * Compare ammunition from slots (0-9); used for sorting.
 *
 * \returns -1 if slot1 should come first, 1 if slot2 should come first, or 0.
 */
int compare_ammo(int slot1, int slot2)
{
    object_type *o1 = &p_ptr->inventory[slot1 + QUIVER_START];
    object_type *o2 = &p_ptr->inventory[slot2 + QUIVER_START];
    bool throw1 = !is_missile(o1) && o1->kind;
    bool throw2 = !is_missile(o2) && o2->kind;

    if (throw1 && !throw2) return (-1);
    else if (!throw1 && throw2) return (1);
    else return 0;
}

/**
 * Swap ammunition between quiver slots (0-9).
 */
void swap_quiver_slots(int slot1, int slot2)
{
	int i = slot1 + QUIVER_START;
	int j = slot2 + QUIVER_START;
	object_type o;

	object_copy(&o, &p_ptr->inventory[i]);
	object_copy(&p_ptr->inventory[i], &p_ptr->inventory[j]);
	object_copy(&p_ptr->inventory[j], &o);
}

/**
 * Sorts the quiver--ammunition inscribed with @fN prefers to end up in quiver
 * slot N.
 */
void sort_quiver(void)
{
    /* Ammo slots go from 0-9; these indices correspond to the range of
     * (QUIVER_START) - (QUIVER_END-1) in inventory[].
     */
    int locked[QUIVER_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int desired[QUIVER_SIZE] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    int i, j, k;
    object_type *o_ptr, *o1_ptr;

    /* Here we figure out which slots have inscribed ammo, and whether that
     * ammo is already in the slot it "wants" to be in or not.
     */
    for (i = 0; i < QUIVER_SIZE; i++)
    {
	bool throwing = FALSE;
	j = QUIVER_START + i;
	o_ptr = &p_ptr->inventory[j];
	throwing = of_has(o_ptr->flags_obj, OF_THROWING);

	/* Skip this slot if it doesn't have ammo */
	if (!o_ptr->k_idx) continue;

	/* Figure out which slot this ammo prefers, if any */
	k = get_inscribed_ammo_slot(o_ptr);
	if (!k) continue;

	k -= QUIVER_START;
	if (k == i) locked[i] = throwing ? 2 : 1;
	if (desired[i] < 0) desired[i] = k;
    }

    /* For items which had a preference that was not fulfilled, we will swap
     * them into the slot as long as it isn't already locked.
     */
    for (i = 0; i < QUIVER_SIZE; i++)
    {
	bool throwing = FALSE;
	int old_lock = -1;

	/* Doesn't want to move */
	if (desired[i] < 0) continue;

	/* Throwing weapons can replace ammo */
	if (locked[desired[i]])
	{
	    o_ptr = &p_ptr->inventory[QUIVER_START + i];
	    throwing = of_has(o_ptr->flags_obj, OF_THROWING);
	    o1_ptr = &p_ptr->inventory[QUIVER_START + desired[i]];

	    /* Break the lock */
	    if (!of_has(o1_ptr->flags_obj, OF_THROWING) && throwing)
	    {
		old_lock = locked[desired[i]];
		locked[desired[i]] = 0;
	    }
	}

	/* Still locked, can't move */
	if (locked[desired[i]]) continue;

	/* item in slot 'desired[i]' desires to be in slot 'i' */
	swap_quiver_slots(desired[i], i);
	locked[desired[i]] = throwing ? 2 : 1;
	locked[i] = old_lock;
    }

    /* Now we need to compact ammo which isn't in a preferrred slot towards the
     * "front" of the quiver */
    for (i = 0; i < QUIVER_SIZE; i++)
    {
	/* If the slot isn't empty, skip it */
	if (p_ptr->inventory[QUIVER_START + i].k_idx) continue;

	/* Start from the end and find an unlocked item to put here. */
	for (j=QUIVER_SIZE - 1; j > i; j--)
	{
	    if (!p_ptr->inventory[QUIVER_START + j].k_idx || locked[j]) continue;
	    swap_quiver_slots(i, j);
	    break;
	}
    }

    /* Now we will sort all other ammo using a simple insertion sort */
    for (i = 0; i < QUIVER_SIZE; i++)
    {
	k = i;
	if (!locked[k])
	    for (j = i + 1; j < QUIVER_SIZE; j++)
		if (!locked[j] && compare_ammo(k, j) > 0)
		    swap_quiver_slots(j, k);
    }
}

/*
 * Shifts ammo at or above the item slot towards the end of the quiver, making
 * room for a new piece of ammo.
 */
void open_quiver_slot(int slot)
{
    int i, pref;
    int dest = QUIVER_END - 1;

    /* This should only be used on ammunition */
    if (slot < QUIVER_START) return;

    /* Quiver is full */
    if (p_ptr->inventory[QUIVER_END - 1].k_idx) return;

    /* Find the first open quiver slot */
    while (p_ptr->inventory[dest].k_idx) dest++;

    /* Swap things with the space one higher (essentially moving the open space
     * towards our goal slot. */
    for (i = dest - 1; i >= slot; i--)
    {
	/* If we have an item with an inscribed location (and it's in */
	/* that location) then we won't move it. */
	pref = get_inscribed_ammo_slot(&p_ptr->inventory[i]);
	if (i != slot && pref && pref == i) continue;

	/* Copy the item up and wipe the old slot */
	COPY(&p_ptr->inventory[dest], &p_ptr->inventory[i], object_type);
	dest = i;
	object_wipe(&p_ptr->inventory[dest]);
    }
}


/**
 * Erase an inventory slot if it has no more items
 */
void inven_item_optimize(int item)
{
    object_type *o_ptr = &p_ptr->inventory[item];
    int i, j, slot, limit;

    /* Save a possibly new quiver size */
    if (item >= QUIVER_START) save_quiver_size(p_ptr);

    /* Only optimize real items which are empty */
    if (!o_ptr->k_idx || o_ptr->number) return;

    /* Items in the pack are treated differently from other items */
    if (item < INVEN_WIELD)
    {
	p_ptr->inven_cnt--;
	p_ptr->redraw |= PR_INVEN;
	limit = INVEN_MAX_PACK;
    }

    /* Items in the quiver and equipped items are (mostly) treated similarly */
    else
    {
	p_ptr->equip_cnt--;
	p_ptr->redraw |= PR_EQUIP;
	limit = item >= QUIVER_START ? QUIVER_END : 0;
    }

    /* If the item is equipped (but not in the quiver), there is no need to */
    /* slide other items. Bonuses and such will need to be recalculated */
    if (!limit)
    {
	/* Erase the empty slot */
	object_wipe(&p_ptr->inventory[item]);

	/* Recalculate stuff */
	p_ptr->update |= (PU_BONUS);
	p_ptr->update |= (PU_TORCH);
	p_ptr->update |= (PU_MANA);

	return;
    }

    /* Slide everything down */
    for (j = item, i = item + 1; i < limit; i++)
    {
	if (limit == QUIVER_END && p_ptr->inventory[i].kind)
	{
	    /* If we have an item with an inscribed location (and it's in */
	    /* that location) then we won't move it. */
	    slot = get_inscribed_ammo_slot(&p_ptr->inventory[i]);
	    if (slot && slot == i)
		continue;
	}
	COPY(&p_ptr->inventory[j], &p_ptr->inventory[i], object_type);

	j = i;
    }

    /* Reorder the quiver if necessary */
    if (item >= QUIVER_START) sort_quiver();

    /* Wipe the left-over object on the end */
    object_wipe(&p_ptr->inventory[j]);

    /* Inventory has changed, so disable repeat command */
    cmd_disable_repeat();
}


/*
 * Describe the charges on an item on the floor.
 */
void floor_item_charges(int item)
{
    object_type *o_ptr = &o_list[item];

    /* Require staff/wand */
    if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND))
	return;

    /* Require known item */
    if (!object_known_p(o_ptr))
	return;

    /* Print a message */
    msg("There are %d charge%s remaining.", o_ptr->pval,
	       (o_ptr->pval != 1) ? "s" : "");
}

/*
 * Describe an item in the inventory.
 */ void floor_item_describe(int item)
{
    object_type *o_ptr = &o_list[item];

    char o_name[80];

    /* Get a description */
    object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

    /* Print a message */
    msg("You see %s.", o_name);
}

/*
 * Increase the "number" of an item on the floor
 */ void floor_item_increase(int item, int num)
{
    object_type *o_ptr = &o_list[item];

    /* Apply */
    num += o_ptr->number;

    /* Bounds check */
    if (num > 255)
	num = 255;
    else if (num < 0)
	num = 0;

    /* Un-apply */
    num -= o_ptr->number;

    /* Change the number */
    o_ptr->number += num;
}

/*
 * Optimize an item on the floor (destroy "empty" items)
 */ void floor_item_optimize(int item)
{
    object_type *o_ptr = &o_list[item];

    /* Paranoia -- be sure it exists */
    if (!o_ptr->k_idx)
	return;

    /* Only optimize empty items */
    if (o_ptr->number)
	return;

    /* Delete the object */
    delete_object_idx(item);
}

/**
 * Check if we have space for an item in the pack without overflow
 */
bool inven_carry_okay(const object_type * o_ptr)
{
    /* Empty slot? */
    if (p_ptr->inven_cnt < INVEN_MAX_PACK) return TRUE;

    /* Check if it can stack */
    if (inven_stack_okay(o_ptr)) return TRUE;

    /* Nope */
    return (FALSE);
}


/*
 * Check to see if an item is stackable in the inventory
 */
bool inven_stack_okay(const object_type *o_ptr)
{
    /* Similar slot? */
    int j;

    /* If our pack is full and we're adding too many missiles, there won't be
     * enough room in the quiver, so don't check it. */
    int limit;

    if (!pack_is_full())
	/* The pack has more room */
	limit = ALL_INVEN_TOTAL;
    else if (p_ptr->quiver_remainder == 0)
	/* Quiver already maxed out */
	limit = INVEN_PACK;
    else if (p_ptr->quiver_remainder + o_ptr->number > 99)
	/* Too much new ammo */
	limit = INVEN_PACK;
    else
	limit = ALL_INVEN_TOTAL;

    for (j = 0; j < limit; j++)
    {
	object_type *j_ptr = &p_ptr->inventory[j];

	/* Skip equipped items and non-objects */
	if (j >= INVEN_PACK && j < QUIVER_START) continue;
	if (!j_ptr->k_idx) continue;

	/* Check if the two items can be combined */
	if (object_similar(j_ptr, o_ptr, OSTACK_PACK)) return (TRUE);
    }
    return (FALSE);
}


/*
 * Add an item to the players inventory, and return the slot used.
 *
 * If the new item can combine with an existing item in the inventory,
 * it will do so, using "object_similar()" and "object_absorb()", else,
 * the item will be placed into the "proper" location in the inventory.
 *
 * This function can be used to "over-fill" the player's pack, but only
 * once, and such an action must trigger the "overflow" code immediately.
 * Note that when the pack is being "over-filled", the new item must be
 * placed into the "overflow" slot, and the "overflow" must take place
 * before the pack is reordered, but (optionally) after the pack is
 * combined.  This may be tricky.  See "dungeon.c" for info.
 *
 * Note that this code must remove any location/stack information
 * from the object once it is placed into the inventory.
 */
s16b inven_carry(struct player *p, struct object *o)
{
    int i, j, k;
    int n = -1;

    object_type *j_ptr;

    /* Apply an autoinscription */
    apply_autoinscription(o);

    /* Check for combining */
    for (j = 0; j < INVEN_PACK; j++) {
	j_ptr = &p->inventory[j];

	/* Skip non-objects */
	if (!j_ptr->k_idx)
	    continue;

	/* Hack -- track last item */
	n = j;

	/* Check if the two items can be combined */
	if (object_similar(j_ptr, o, OSTACK_PACK)) {
	    /* Combine the items */
	    object_absorb(j_ptr, o);

	    /* Increase the weight */
	    p->total_weight += (o->number * o->weight);

	    /* Recalculate bonuses */
	    p->update |= (PU_BONUS);

	    /* Redraw stuff */
	    p->redraw |= (PR_INVEN);

	    /* Save quiver size */
	    save_quiver_size(p);

	    /* Success */
	    return (j);
	}
    }
    /* Paranoia */ if (p->inven_cnt > INVEN_MAX_PACK)
	return (-1);


    /* Find an empty slot */
    for (j = 0; j <= INVEN_MAX_PACK; j++) {
	j_ptr = &p->inventory[j];

	/* Use it if found */
	if (!j_ptr->k_idx)
	    break;
    }

    /* Use that slot */
    i = j;

    /* Reorder the pack */
    if (i < INVEN_MAX_PACK) {
	s32b o_value, j_value;

	/* Get the "value" of the item */
	o_value = object_value(o);

	/* Scan every occupied slot */
	for (j = 0; j < INVEN_MAX_PACK; j++) {
	    j_ptr = &p->inventory[j];

	    /* Use empty slots */
	    if (!j_ptr->k_idx)
		break;

	    /* Hack -- readable books always come first */
	    if ((o->tval == mp_ptr->spell_book)
		&& (j_ptr->tval != mp_ptr->spell_book))
		break;
	    if ((j_ptr->tval == mp_ptr->spell_book)
		&& (o->tval != mp_ptr->spell_book))
		continue;

	    /* Objects sort by decreasing type */
	    if (o->tval > j_ptr->tval)
		break;
	    if (o->tval < j_ptr->tval)
		continue;

	    /* Non-aware (flavored) items always come last */
	    if (!object_aware_p(o))
		continue;
	    if (!object_aware_p(j_ptr))
		break;

	    /* Objects sort by increasing sval */
	    if (o->sval < j_ptr->sval)
		break;
	    if (o->sval > j_ptr->sval)
		continue;

	    /* Unidentified objects always come last */
	    if (!object_known_p(o))
		continue;
	    if (!object_known_p(j_ptr))
		break;

	    /* Lights sort by decreasing fuel */
	    if (o->tval == TV_LIGHT) {
		if (o->pval > j_ptr->pval)
		    break;
		if (o->pval < j_ptr->pval)
		    continue;
	    }

	    /* Determine the "value" of the pack item */
	    j_value = object_value(j_ptr);

	    /* Objects sort by decreasing value */
	    if (o_value > j_value)
		break;
	    if (o_value < j_value)
		continue;
	}

	/* Use that slot */
	i = j;

	/* Slide objects */
	for (k = n; k >= i; k--) {
	    /* Hack -- Slide the item */
	    object_copy(&p->inventory[k + 1], &p->inventory[k]);
	}

	/* Wipe the empty slot */
	object_wipe(&p->inventory[i]);
    }

    object_copy(&p->inventory[i], o);

    j_ptr = &p->inventory[i];
    j_ptr->next_o_idx = 0;
    j_ptr->held_m_idx = 0;
    j_ptr->iy = j_ptr->ix = 0;
    j_ptr->marked = FALSE;

    p->total_weight += (j_ptr->number * j_ptr->weight);
    p->inven_cnt++;
    p->update |= (PU_BONUS);
    p->notice |= (PN_COMBINE | PN_REORDER);
    p->redraw |= (PR_INVEN);

    /* Save quiver size */
    save_quiver_size(p);

    /* Return the slot */
    return (i);
}


/*
 * Take off (some of) a non-cursed equipment item
 *
 * Note that only one item at a time can be wielded per slot.
 *
 * Note that taking off an item when "full" may cause that item
 * to fall to the ground.
 *
 * Return the inventory slot into which the item is placed.
 */
s16b inven_takeoff(int item, int amt)
{
    int slot;

    object_type *o_ptr;

    object_type *i_ptr;
    object_type object_type_body;

    const char *act;

    char o_name[80];


    /* Get the item to take off */
    o_ptr = &p_ptr->inventory[item];

    /* Paranoia */
    if (amt <= 0)
	return (-1);

    /* Verify */
    if (amt > o_ptr->number)
	amt = o_ptr->number;

    /* Check to see if a Set Item is being removed */
    if (o_ptr->name1) {
	artifact_type *a_ptr = &a_info[o_ptr->name1];
	if (a_ptr->set_no != 0) {
	    /* The object is part of a set. Check to see if rest of set is
	     * equiped and if so remove bonuses -GS- */
	    if (check_set(a_ptr->set_no)) {
		remove_set(a_ptr->set_no);
	    }
	}
    }

    /* Get local object */
    i_ptr = &object_type_body;

    /* Obtain a local object */
    object_copy(i_ptr, o_ptr);

    /* Modify quantity */
    i_ptr->number = amt;

    /* Describe the object */
    object_desc(o_name, sizeof(o_name), i_ptr, ODESC_PREFIX | ODESC_FULL);

    /* Took off weapon */
    if (item == INVEN_WIELD) {
	act = "You were wielding";
    }

    /* Took off bow */
    else if (item == INVEN_BOW) {
	act = "You were holding";
    }

    /* Took off light */
    else if (item == INVEN_LIGHT) {
	act = "You were holding";
    }

    /* Took off something */
    else {
	act = "You were wearing";
    }

    /* Modify, Optimize */
    inven_item_increase(item, -amt);
    inven_item_optimize(item);

    /* Carry the object */
    slot = inven_carry(p_ptr, i_ptr);

    /* Message */
    msgt(MSG_WIELD, "%s %s (%c).", act, o_name,
		   index_to_label(slot));

    p_ptr->notice |= PN_SQUELCH;

    /* Return slot */
    return (slot);
}


/*
 * Drop (some of) a non-cursed inventory/equipment item
 *
 * The object will be dropped "near" the current location
 */
void inven_drop(int item, int amt)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    object_type *o_ptr;

    object_type *i_ptr;
    object_type object_type_body;

    char o_name[80];


    /* Get the original object */
    o_ptr = &p_ptr->inventory[item];

    /* Error check */
    if (amt <= 0)
	return;

    /* Not too many */
    if (amt > o_ptr->number)
	amt = o_ptr->number;


    /* Take off equipment */
    if (item >= INVEN_WIELD) {
	/* Take off first */
	item = inven_takeoff(item, amt);

	/* Get the original object */
	o_ptr = &p_ptr->inventory[item];
    }


    /* Get local object */
    i_ptr = &object_type_body;

    /* Obtain local object */
    object_copy(i_ptr, o_ptr);

    /* Distribute charges of wands, staves, or rods */
    distribute_charges(o_ptr, i_ptr, amt);

    /* Modify quantity */
    i_ptr->number = amt;

    /* Describe local object */
    object_desc(o_name, sizeof(o_name), i_ptr, ODESC_PREFIX | ODESC_FULL);

    /* Message */
    msg("You drop %s (%c).", o_name, index_to_label(item));

    /* Drop it near the player */
    drop_near(i_ptr, 0, py, px, FALSE);

    /* Modify, Describe, Optimize */
    inven_item_increase(item, -amt);
    inven_item_describe(item);
    inven_item_optimize(item);
}



/*
 * Combine items in the pack
 * Also "pick up" any gold in the inventory by accident
 *
 * Note special handling of the "overflow" slot
 */
void combine_pack(void)
{
    int i, j, k;

    object_type *o_ptr;
    object_type *j_ptr;

    bool flag = FALSE;


    /* Combine the pack (backwards) */
    for (i = INVEN_PACK; i > 0; i--) {
	bool slide = FALSE;

	/* Get the item */
	o_ptr = &p_ptr->inventory[i];

	/* Skip empty items */
	if (!o_ptr->k_idx)
	    continue;

	/* Absorb gold */
	if (o_ptr->tval == TV_GOLD) {
	    /* Count the gold */
	    slide = TRUE;
	    p_ptr->au += o_ptr->pval;
	}

	/* Scan the items above that item */
	else
	    for (j = 0; j < i; j++) {
		/* Get the item */
		j_ptr = &p_ptr->inventory[j];

		/* Skip empty items */
		if (!j_ptr->k_idx)
		    continue;

		/* Can we drop "o_ptr" onto "j_ptr"? */
		if (object_similar(j_ptr, o_ptr, OSTACK_PACK)) {
		    /* Take note */
		    flag = slide = TRUE;

		    /* Add together the item counts */
		    object_absorb(j_ptr, o_ptr);

		    break;
		}
	    }


	/* Compact the inventory */
	if (slide) {
	    /* One object is gone */
	    p_ptr->inven_cnt--;

	    /* Slide everything down */
	    for (k = i; k < INVEN_PACK; k++) {
		/* Hack -- slide object */
		COPY(&p_ptr->inventory[k], &p_ptr->inventory[k + 1],
		     object_type);
	    }

	    /* Hack -- wipe hole */
	    object_wipe(&p_ptr->inventory[k]);

	    /* Redraw stuff */
	    p_ptr->redraw |= (PR_INVEN);
	}
    }

    /* Message */
    if (flag) {
	msg("You combine some items in your pack.");

	/* Stop "repeat last command" from working. */
	cmd_disable_repeat();
    }
}


/*
 * Reorder items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void reorder_pack(void)
{
    int i, j, k;

    s32b o_value;
    s32b j_value;

    object_type *o_ptr;
    object_type *j_ptr;

    object_type *i_ptr;
    object_type object_type_body;

    bool flag = FALSE;


    /* Re-order the pack (forwards) */
    for (i = 0; i < INVEN_PACK; i++) {
	/* Get the item */
	o_ptr = &p_ptr->inventory[i];

	/* Skip empty slots */
	if (!o_ptr->k_idx)
	    continue;

	/* Get the "value" of the item */
	o_value = object_value(o_ptr);

	/* Scan every occupied slot */
	for (j = 0; j < INVEN_PACK; j++) {
	    /* Get the item already there */
	    j_ptr = &p_ptr->inventory[j];

	    /* Use empty slots */
	    if (!j_ptr->k_idx)
		break;

	    /* Hack -- readable books always come first */
	    if ((o_ptr->tval == mp_ptr->spell_book)
		&& (j_ptr->tval != mp_ptr->spell_book))
		break;
	    if ((j_ptr->tval == mp_ptr->spell_book)
		&& (o_ptr->tval != mp_ptr->spell_book))
		continue;

	    /* Objects sort by decreasing type */
	    if (o_ptr->tval > j_ptr->tval)
		break;
	    if (o_ptr->tval < j_ptr->tval)
		continue;

	    /* Non-aware (flavored) items always come last */
	    if (!object_aware_p(o_ptr))
		continue;
	    if (!object_aware_p(j_ptr))
		break;

	    /* Objects sort by increasing sval */
	    if (o_ptr->sval < j_ptr->sval)
		break;
	    if (o_ptr->sval > j_ptr->sval)
		continue;

	    /* Unidentified objects always come last */
	    if (!object_known_p(o_ptr))
		continue;
	    if (!object_known_p(j_ptr))
		break;

	    /* Lights sort by decreasing fuel */
	    if (o_ptr->tval == TV_LIGHT) {
		if (o_ptr->pval > j_ptr->pval)
		    break;
		if (o_ptr->pval < j_ptr->pval)
		    continue;
	    }

	    /* Determine the "value" of the pack item */
	    j_value = object_value(j_ptr);

	    /* Objects sort by decreasing value */
	    if (o_value > j_value)
		break;
	    if (o_value < j_value)
		continue;
	}

	/* Never move down */
	if (j >= i)
	    continue;

	/* Take note */
	flag = TRUE;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Save a copy of the moving item */
	object_copy(i_ptr, &p_ptr->inventory[i]);

	/* Slide the objects */
	for (k = i; k > j; k--) {
	    /* Slide the item */
	    object_copy(&p_ptr->inventory[k], &p_ptr->inventory[k - 1]);
	}

	/* Insert the moving item */
	object_copy(&p_ptr->inventory[j], i_ptr);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_INVEN);
    }

    if (flag) {
	msg("You reorder some items in your pack.");

	/* Stop "repeat last command" from working. */
	cmd_disable_repeat();
    }
}



/**
 *Returns the number of times in 1000 that @ will FAIL
 */
int get_use_device_chance(const object_type * o_ptr)
{
    int lev;

    /* Base chance of success */
    int chance = p_ptr->state.skills[SKILL_DEVICE];

    /* Final "probability" */
    int prob = 10000;

    /* Extract the item level, which is the difficulty rating */
    if (artifact_p(o_ptr))
	lev = a_info[o_ptr->name1].level;
    else
	lev = k_info[o_ptr->k_idx].level;

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

/**
 * Distribute charges of rods, staves, or wands.
 *
 * o_ptr = source item
 * q_ptr = target item, must be of the same type as o_ptr
 * amt   = number of items that are transfered
 */ 
void distribute_charges(object_type * o_ptr, object_type * q_ptr, int amt)
{
    const object_kind *k_ptr = &k_info[o_ptr->k_idx];
    int charge_time = randcalc(k_ptr->time, 0, AVERAGE), max_time;

    /* 
     * Hack -- If rods, staves, or wands are dropped, the total maximum
     * timeout or charges need to be allocated between the two stacks.
     * If all the items are being dropped, it makes for a neater message
     * to leave the original stack's pval alone. -LM-
     */
    if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF)) {
	q_ptr->pval = o_ptr->pval * amt / o_ptr->number;

	if (amt < o_ptr->number)
	    o_ptr->pval -= q_ptr->pval;
    }

    /* 
     * Hack -- Rods also need to have their timeouts distributed.
     *
     * The dropped stack will accept all time remaining to charge up to
     * its maximum.
     */
    if (o_ptr->tval == TV_ROD) {
	max_time = charge_time * amt;

	if (o_ptr->timeout > max_time)
	    q_ptr->timeout = max_time;
	else
	    q_ptr->timeout = o_ptr->timeout;

	if (amt < o_ptr->number)
	    o_ptr->timeout -= q_ptr->timeout;
    }
}


void reduce_charges(object_type * o_ptr, int amt)
{
    /* 
     * Hack -- If rods or wand are destroyed, the total maximum timeout or
     * charges of the stack needs to be reduced, unless all the items are
     * being destroyed. -LM-
     */
    if (((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
	&& (amt < o_ptr->number)) {
	o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
    }

    if ((o_ptr->tval == TV_ROD) && (amt < o_ptr->number)) {
	o_ptr->timeout -= o_ptr->timeout * amt / o_ptr->number;
    }
}


int number_charging(const object_type * o_ptr)
{
    int charge_time, num_charging;

    charge_time = randcalc(o_ptr->time, 0, AVERAGE);

    /* Item has no timeout */
    if (charge_time <= 0)
	return 0;

    /* No items are charging */
    if (o_ptr->timeout <= 0)
	return 0;

    /* Calculate number charging based on timeout */
    num_charging = (o_ptr->timeout + charge_time - 1) / charge_time;

    /* Number charging cannot exceed stack size */
    if (num_charging > o_ptr->number)
	num_charging = o_ptr->number;

    return num_charging;
}

bool recharge_timeout(object_type * o_ptr)
{
    int charging_before, charging_after;

    /* Find the number of charging items */
    charging_before = number_charging(o_ptr);

    /* Nothing to charge */
    if (charging_before == 0)
	return FALSE;

    /* Decrease the timeout */
    o_ptr->timeout -= MIN(charging_before, o_ptr->timeout);

    /* Find the new number of charging items */
    charging_after = number_charging(o_ptr);

    /* Return true if at least 1 item obtained a charge */
    if (charging_after < charging_before)
	return TRUE;
    else
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
    } while (s);

    return i;
}

/*** Object kind lookup functions ***/

/**
 * Return the k_idx of the object kind with the given `tval` and `sval`, or 0.
 */
int lookup_kind(int tval, int sval)
{
    int k;

    /* Look for it */
    for (k = 1; k < z_info->k_max; k++) {
	object_kind *k_ptr = &k_info[k];

	/* Found a match */
	if ((k_ptr->tval == tval) && (k_ptr->sval == sval))
	    return (k);
    }
    /* Failure */ 
    msg("No object (%s,%d)", tval_find_name(tval), tval, sval);
    return 0;
}

struct object_kind *objkind_get(int tval, int sval)
{
    int k = lookup_kind(tval, sval);
    return &k_info[k];
}

/**
 * Find the tval and sval of object kind `k_idx`, and return via the pointers
 * `tval` and `sval`.
 */ bool lookup_reverse(s16b k_idx, int *tval, int *sval)
{
    object_kind *k_ptr;

    /* Validate k_idx */
    if ((k_idx < 1) || (k_idx > z_info->k_max))
	return FALSE;

    /* Get pointer */
    k_ptr = &k_info[k_idx];
    *tval = k_ptr->tval;
    *sval = k_ptr->sval;

    /* Done */
    return TRUE;
}


/*** Textual<->numeric conversion ***/

/**
 * List of { tval, name } pairs.
 */
static const grouper tval_names[] = {
    { TV_SKELETON, "skeleton"},
    { TV_BOTTLE, "bottle"},
    { TV_JUNK, "junk"},
    { TV_SPIKE,	"spike"},
    { TV_CHEST, "chest"},
    { TV_SHOT, "shot"},
    { TV_ARROW, "arrow"},
    { TV_BOLT,	"bolt"},
    { TV_BOW,	"bow"},
    { TV_DIGGING, "digger"},
    { TV_HAFTED, "hafted"},
    { TV_POLEARM, "polearm"},
    { TV_SWORD,	"sword"},
    { TV_BOOTS, "boots"},
    { TV_GLOVES, "gloves"},
    { TV_HELM, "helm"},
    { TV_CROWN,	"crown"},
    { TV_SHIELD,	"shield"},
    { TV_CLOAK, "cloak"},
    { TV_SOFT_ARMOR, "soft armor"},
    { TV_SOFT_ARMOR,	"soft armour"},
    { TV_HARD_ARMOR,	"hard armor"},
    { TV_HARD_ARMOR, "hard armour"},
    { TV_DRAG_ARMOR, "dragon armor"},
    { TV_DRAG_ARMOR,	"dragon armour"},
    { TV_LIGHT, "light"},
    { TV_AMULET, "amulet"},
    { TV_RING, "ring"},
    { TV_STAFF,	"staff"},
    { TV_WAND,	"wand"},
    { TV_ROD, "rod"},
    { TV_SCROLL, "scroll"},
    { TV_POTION, "potion"},
    { TV_FLASK,	"flask"},
    { TV_FOOD,	"food"},
    { TV_MAGIC_BOOK, "magic book"},
    { TV_PRAYER_BOOK, "prayer book"},
    { TV_DRUID_BOOK, "stone of lore"},
    { TV_NECRO_BOOK, "necromantic tome"},
    { TV_GOLD,	"gold"},

};

/**
 * Return the k_idx of the object kind with the given `tval` and name `name`.
 */
int lookup_name(int tval, const char *name)
{
    int k;

    /* Look for it */
    for (k = 1; k < z_info->k_max; k++) {
	object_kind *k_ptr = &k_info[k];
	const char *nm = k_ptr->name;

	if (!nm)
	    continue;

	if (*nm == '&' && *(nm + 1))
	    nm += 2;

	/* Found a match */
	if (k_ptr->tval == tval && !strcmp(name, nm))
	    return k;
    }
    msg("No object (\"%s\",\"%s\")", tval_find_name(tval), name);
    return -1;
}

/**
 * Return the a_idx of the artifact with the given name
 */
int lookup_artifact_name(const char *name)
{
    int i;

    /* Look for it */
    for (i = 1; i < z_info->a_max; i++) {
	artifact_type *a_ptr = &a_info[i];

	/* Found a match */
	if (a_ptr->name && streq(name, a_ptr->name))
	    return i;

    }
    return -1;
}


/**
 * Return the numeric sval of the object kind with the given `tval` and name `name`.
 */
int lookup_sval(int tval, const char *name)
{
    int k;
    unsigned int r;

    if (sscanf(name, "%u", &r) == 1)
	return r;

    /* Look for it */
    for (k = 1; k < z_info->k_max; k++) {
	object_kind *k_ptr = &k_info[k];
	const char *nm = k_ptr->name;

	if (!nm)
	    continue;

	if (*nm == '&' && *(nm + 1))
	    nm += 2;

	/* Found a match */
	if (k_ptr->tval == tval && !strcmp(name, nm))
	    return k_ptr->sval;
    }
    return -1;
}

/**
 * Returns the numeric equivalent tval of the textual tval `name`.
 */
int tval_find_idx(const char *name)
{
    size_t i = 0;
    unsigned int r;

    if (sscanf(name, "%u", &r) == 1)
	return r;

    for (i = 0; i < N_ELEMENTS(tval_names); i++) {
	if (!my_stricmp(name, tval_names[i].name))
	    return tval_names[i].tval;
    }
    return -1;
}

/**
 * Returns the textual equivalent tval of the numeric tval `name`.
 */
const char *tval_find_name(int tval)
{
    size_t i = 0;

    for (i = 0; i < N_ELEMENTS(tval_names); i++) {
	if (tval == tval_names[i].tval)
	    return tval_names[i].name;
    }
    return "unknown";
}


/**
 * Sort comparator for objects using only tval and sval.
 * -1 if o1 should be first
 *  1 if o2 should be first
 *  0 if it doesn't matter
 */
static int compare_types(const object_type * o1, const object_type * o2)
{
    if (o1->tval == o2->tval)
	return CMP(o1->sval, o2->sval);
    else
	return CMP(o1->tval, o2->tval);
}

/* some handy macros for sorting */
#define object_is_worthless(o) (object_value(o) == 0)
/**
 * Sort comparator for objects
 * -1 if o1 should be first
 *  1 if o2 should be first
 *  0 if it doesn't matter
 *
 * The sort order is designed with the "list items" command in mind.
 */ static int compare_items(const object_type * o1, const object_type * o2)
{
    /* known artifacts will sort first */
    if (object_known_p(o1) && object_known_p(o2) && artifact_p(o1) && 
	artifact_p(o2))
	return compare_types(o1, o2);
    if (object_known_p(o1) && artifact_p(o1))
	return -1;
    if (object_known_p(o2) && artifact_p(o2))
	return 1;

    /* unknown objects will sort next */
    if (!object_aware_p(o1) && !object_aware_p(o2))
	return compare_types(o1, o2);
    if (!object_aware_p(o1))
	return -1;
    if (!object_aware_p(o2))
	return 1;

    /* if only one of them is worthless, the other comes first */
    if (object_is_worthless(o1) && !object_is_worthless(o2))
	return 1;
    if (!object_is_worthless(o1) && object_is_worthless(o2))
	return -1;

    /* otherwise, just compare tvals and svals */
    /* NOTE: arguably there could be a better order than this */
    return compare_types(o1, o2);
}

/**
 * Helper to draw the Object Recall subwindow; this actually does the work.
 */ void display_object_recall(object_type * o_ptr)
{
    char header[120];

    textblock *tb = object_info(o_ptr, OINFO_NONE);
    object_desc(header, sizeof(header), o_ptr, ODESC_PREFIX | ODESC_FULL);

    clear_from(0);
    textui_textblock_place(tb, SCREEN_REGION, header);
    textblock_free(tb);
}


/**
 * This draws the Object Recall subwindow when displaying a particular object
 * (e.g. a helmet in the backpack, or a scroll on the ground)
 */
void display_object_idx_recall(s16b item)
{
    object_type *o_ptr = object_from_item_idx(item);
    display_object_recall(o_ptr);
}


/**
 * This draws the Object Recall subwindow when displaying a recalled item kind
 * (e.g. a generic ring of acid or a generic blade of chaos)
 */
void display_object_kind_recall(s16b k_idx)
{
    object_type object = { 0 };
    object_prep(&object, k_idx, EXTREMIFY);
    if (k_info[k_idx].aware)
	object.ident |= IDENT_STORE;

    display_object_recall(&object);
}

/*
 * Display visible items, similar to display_monlist
 */
void display_itemlist(void)
{
    int max;
    int mx, my;
    unsigned num;
    int line = 1, x = 0;
    int cur_x;
    unsigned i;
    unsigned disp_count = 0;
    byte a;
    wchar_t c;

    object_type *types[MAX_ITEMLIST];
    int counts[MAX_ITEMLIST];
    int dx[MAX_ITEMLIST], dy[MAX_ITEMLIST];
    unsigned counter = 0;

    int dungeon_hgt = DUNGEON_HGT;
    int dungeon_wid = DUNGEON_WID;

    byte attr;
    char buf[80];

    int floor_list[MAX_FLOOR_STACK];

    /* Adjust for town */
    if (p_ptr->depth == 0) town_adjust(&dungeon_hgt, &dungeon_wid);

    /* Clear the term if in a subwindow, set x otherwise */
    if (Term != angband_term[0]) {
	clear_from(0);
	max = Term->hgt - 1;
    } else {
	x = 13;
	max = Term->hgt - 2;
    }

    /* Look at each square of the dungeon for items */
    for (my = 0; my < dungeon_hgt; my++) {
	for (mx = 0; mx < dungeon_wid; mx++) {
	    num = scan_floor(floor_list, MAX_FLOOR_STACK, my, mx, 0x02);

	    /* Iterate over all the items found on this square */
	    for (i = 0; i < num; i++) {
		object_type *o_ptr = &o_list[floor_list[i]];
		unsigned j;

		/* Skip gold/squelched */
		if (o_ptr->tval == TV_GOLD || squelch_hide_item(o_ptr))
		    continue;

		/* See if we've already seen a similar item; if so, just add */
		/* to its count */
		for (j = 0; j < counter; j++) {
		    if (object_similar(o_ptr, types[j], OSTACK_LIST)) {
			counts[j] += o_ptr->number;
			if ((my - p_ptr->py) * (my - p_ptr->py) +
			    (mx - p_ptr->px) * (mx - p_ptr->px) <
			    dy[j] * dy[j] + dx[j] * dx[j]) {
			    dy[j] = my - p_ptr->py;
			    dx[j] = mx - p_ptr->px;
			}
			break;
		    }
		}

		/* We saw a new item. So insert it at the end of the list and */
		/* then sort it forward using compare_items(). The types list */
		/* is always kept sorted. */
		/* If we have too many items, replace the last (normally least important) */
		/* item in the list. */
		if (j == counter) {
			if (counter == MAX_ITEMLIST) {
				counter -= 1;
				j -= 1;	
			}
		    types[counter] = o_ptr;
		    counts[counter] = o_ptr->number;
		    dy[counter] = my - p_ptr->py;
		    dx[counter] = mx - p_ptr->px;

		    while (j > 0 && compare_items(types[j - 1], types[j]) > 0) {
			object_type *tmp_o = types[j - 1];
			int tmpcount;
			int tmpdx = dx[j - 1];
			int tmpdy = dy[j - 1];

			types[j - 1] = types[j];
			types[j] = tmp_o;
			dx[j - 1] = dx[j];
			dx[j] = tmpdx;
			dy[j - 1] = dy[j];
			dy[j] = tmpdy;
			tmpcount = counts[j - 1];
			counts[j - 1] = counts[j];
			counts[j] = tmpcount;
			j--;
		    }
		    counter++;
		}
	    }
	}
    }

    /* Note no visible items */
    if (!counter) {
	/* Clear display and print note */
	c_prt(TERM_SLATE, "You see no items.", 0, 0);
	if (Term == angband_term[0])
	    Term_addstr(-1, TERM_WHITE, "  (Press any key to continue.)");

	/* Done */
	return;
    } else if (counter == MAX_ITEMLIST) {
	    c_prt(TERM_SLATE, "You see many items.", 0, 0);
    } else {
	/* Reprint Message */
	prt(format("You can see %d item%s:", counter, (counter > 1 ? "s" : "")),
	    0, 0);
    }

    for (i = 0; i < counter; i++) {
	/* o_name will hold the object_desc() name for the object. */
	/* o_desc will also need to put a (x4) behind it. */
	/* can there be more than 999 stackable items on a level? */
	char o_name[80];
	char o_desc[86];

	object_type *o_ptr = types[i];

	/* We shouldn't list coins or squelched items */
	if (o_ptr->tval == TV_GOLD || squelch_hide_item(o_ptr))
	    continue;

	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_FULL);
	if (counts[i] > 1)
	    strnfmt(o_desc, sizeof(o_desc), "%s (x%d) %d %c, %d %c", o_name,
		    counts[i], (dy[i] > 0) ? dy[i] : -dy[i],
		    (dy[i] > 0) ? 'S' : 'N', (dx[i] > 0) ? dx[i] : -dx[i],
		    (dx[i] > 0) ? 'E' : 'W');
	else
	    strnfmt(o_desc, sizeof(o_desc), "%s  %d %c %d %c", o_name,
		    (dy[i] > 0) ? dy[i] : -dy[i], (dy[i] > 0) ? 'S' : 'N',
		    (dx[i] > 0) ? dx[i] : -dx[i], (dx[i] > 0) ? 'E' : 'W');

	/* Reset position */
	cur_x = x;

	/* See if we need to scroll or not */
	if (Term == angband_term[0] && (line == max) && disp_count != counter) {
	    prt("-- more --", line, x);
	    anykey();

	    /* Clear the screen */
	    for (line = 1; line <= max; line++)
		prt("", line, x);

	    /* Reprint Message */
	    if (counter == MAX_ITEMLIST) {
		    prt(format
			("You can see many items:"),
			0, 0);		
		} else {
		    prt(format
			("You can see %d item%s:", counter, (counter > 1 ? "s" : "")),
			0, 0);
		}
	    /* Reset */
	    line = 1;
	} else if (line == max) {
	    continue;
	}

	/* Note that the number of items actually displayed */
	disp_count++;

	if (artifact_p(o_ptr) && object_known_p(o_ptr))
	    /* known artifact */
	    attr = TERM_VIOLET;
	else if (!object_aware_p(o_ptr))
	    /* unaware of kind */
	    attr = TERM_RED;
	else if (object_is_worthless(o_ptr))
	    /* worthless */
	    attr = TERM_SLATE;
	else
	    /* default */
	    attr = TERM_WHITE;

	a = object_kind_attr(o_ptr->k_idx);
	c = object_kind_char(o_ptr->k_idx);

	/* Display the pict */
	if ((tile_width == 1) && (tile_height == 1)) {
	    Term_putch(cur_x++, line, a, c);
	    Term_putch(cur_x++, line, TERM_WHITE, ' ');
	}

	/* Print and bump line counter */
	c_prt(attr, o_desc, line, cur_x);
	line++;
    }

    if (disp_count != counter) {
		/* Print "and others" message if we've run out of space */
		if (counter == MAX_ITEMLIST) {
			strnfmt(buf, sizeof buf, "  ...and many others.");
		} else {
			strnfmt(buf, sizeof buf, "  ...and %d others.", counter - disp_count);
		}
		c_prt(TERM_WHITE, buf, line, x);
    } else {
	/* Otherwise clear a line at the end, for main-term display */
	prt("", line, x);
    }

    if (Term == angband_term[0])
	Term_addstr(-1, TERM_WHITE, "  (Press any key to continue.)");
}


/* Accessor functions, for prettier code */

artifact_type *artifact_of(const object_type * o_ptr)
{
    if (o_ptr->name1)
	return &a_info[o_ptr->name1];

    return NULL;
}

object_kind *object_kind_of(const object_type * o_ptr)
{
    return &k_info[o_ptr->k_idx];
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
    return o_ptr->tval == TV_LIGHT;
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
    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
	return TRUE;
    default:
	return FALSE;
    }
}

/**
 * Determine whether an object goes in the quiver
 *
 * \param o_ptr is the object to check
 */
bool obj_is_quiver_obj(const object_type * o_ptr)
{
    if (of_has(o_ptr->flags_obj, OF_THROWING))
	return TRUE;
    if (obj_is_ammo(o_ptr))
	return TRUE;
    
    return FALSE;
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
    const object_type *j_ptr = &p_ptr->inventory[INVEN_LIGHT];

    /* Other lights of the same type are OK */
    if ((o_ptr->tval == TV_LIGHT) && (o_ptr->sval == j_ptr->sval))
	return (TRUE);
    
    if (j_ptr->sval == SV_LIGHT_LANTERN) {
	/* Flasks of oil are okay */
	if (o_ptr->tval == TV_FLASK)
	    return (TRUE);
    }

    /* Assume not okay */
    return (FALSE);
}


bool obj_can_browse(const object_type * o_ptr)
{
    return o_ptr->tval == mp_ptr->spell_book;
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
    if (cf_has(o_ptr->flags_curse, CF_STICKY_WIELD)) 
    {
	int i;

	for (i = INVEN_WIELD; i < ALL_INVEN_TOTAL; i++)
	    if (o_ptr == &p_ptr->inventory[i]) break;

	notice_curse(CF_STICKY_WIELD, i + 1);
	return FALSE;
    }
    return TRUE;
}

bool obj_think_can_takeoff(const object_type * o_ptr)
{
    if (cf_has(o_ptr->id_curse, CF_STICKY_WIELD)) 
	return FALSE;
    
    return TRUE;
}

/* Can only put on wieldable items */
bool obj_can_wear(const object_type * o_ptr)
{
    return (wield_slot(o_ptr) >= INVEN_WIELD);
}

/* Can only fire an item with the right tval */
bool obj_can_fire(const object_type * o_ptr)
{
    return o_ptr->tval == p_ptr->state.ammo_tval;
}

/* Can only fire an item with the right tval */
bool obj_can_throw(const object_type *o_ptr)
{
    int i;

    for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	if (o_ptr == &p_ptr->inventory[i])
	    return FALSE;

    return TRUE;
}

/* Can has inscrip pls */
bool obj_has_inscrip(const object_type * o_ptr)
{
    return (o_ptr->note ? TRUE : FALSE);
}

/*** Generic utility functions ***/

/*
 * Return an object's effect.
 */
u16b object_effect(const object_type * o_ptr)
{

	return o_ptr->effect;
}

/* Get an o_ptr from an item number */
object_type *object_from_item_idx(int item)
{
    if (item >= 0)
	return &p_ptr->inventory[item];
    else
	return &o_list[0 - item];
}


/*
 * Does the given object need to be aimed?
 */
bool obj_needs_aim(object_type * o_ptr)
{
    int effect = o_ptr->effect;

    /* If the effect needs aiming, or if the object type needs aiming, this
     * object needs aiming. */
    if (effect_aim(effect) || o_ptr->tval == TV_BOLT || o_ptr->tval == TV_SHOT
	|| o_ptr->tval == TV_ARROW || o_ptr->tval == TV_WAND
	|| (o_ptr->tval == TV_ROD && !object_aware_p(o_ptr)))
	return TRUE;
    else
	return FALSE;
}


/*
 * Verify the "okayness" of a given item.
 *
 * The item can be negative to mean "item on floor".
 */
bool get_item_okay(int item)
{
    /* Verify the item */
    return (item_tester_okay(object_from_item_idx(item)));
}


/*
 * Get a list of "valid" item indexes.
 *
 * Fills item_list[] with items that are "okay" as defined by the
 * current item_tester_hook, etc.  mode determines what combination of
 * inventory, equipment and player's floor location should be used
 * when drawing up the list.
 *
 * Returns the number of items placed into the list.
 *
 * Maximum space that can be used is [INVEN_TOTAL + MAX_FLOOR_STACK],
 * though practically speaking much smaller numbers are likely.
 */
int scan_items(int *item_list, size_t item_list_max, int mode)
{
    bool use_inven = ((mode & USE_INVEN) ? TRUE : FALSE);
    bool use_equip = ((mode & USE_EQUIP) ? TRUE : FALSE);
    bool use_floor = ((mode & USE_FLOOR) ? TRUE : FALSE);

    int floor_list[MAX_FLOOR_STACK];
    int floor_num;

    int i;
    size_t item_list_num = 0;

    if (use_inven) {
	for (i = 0; i < INVEN_PACK && item_list_num < item_list_max; i++) {
	    if (get_item_okay(i))
		item_list[item_list_num++] = i;
	}
    }

    if (use_equip) {
	for (i = INVEN_WIELD;
	     i < ALL_INVEN_TOTAL && item_list_num < item_list_max; i++) {
	    if (get_item_okay(i))
		item_list[item_list_num++] = i;
	}
    }

    /* Scan all non-gold objects in the grid */
    if (use_floor) {
	floor_num =
	    scan_floor(floor_list, N_ELEMENTS(floor_list), p_ptr->py, p_ptr->px,
		       0x03);

	for (i = 0; i < floor_num && item_list_num < item_list_max; i++) {
	    if (get_item_okay(-floor_list[i]))
		item_list[item_list_num++] = -floor_list[i];
	}
    }

    /* Forget the item_tester_tval and item_tester_hook restrictions */
    item_tester_tval = 0;
    item_tester_hook = NULL;

    return item_list_num;
}


/* 
 * Check if the given item is available for the player to use. 
 *
 * 'mode' defines which areas we should look at, a la scan_items().
 */
bool item_is_available(int item, bool(*tester) (const object_type *), int mode)
{
    int item_list[ALL_INVEN_TOTAL + MAX_FLOOR_STACK];
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
 * Returns whether the pack is holding the maximum number of items. The max
 * size is INVEN_MAX_PACK, which is a macro since quiver size affects slots
 * available.
 */
bool pack_is_full(void)
{
    return p_ptr->inventory[INVEN_MAX_PACK - 1].k_idx ? TRUE : FALSE;
}

/*
 * Returns whether the pack is holding the more than the maximum number of
 * items. The max size is INVEN_MAX_PACK, which is a macro since quiver size
 * affects slots available. If this is true, calling pack_overflow() will
 * trigger a pack overflow.
 */
bool pack_is_overfull(void)
{
    return p_ptr->inventory[INVEN_MAX_PACK].k_idx ? TRUE : FALSE;
}

/*
 * Overflow an item from the pack, if it is overfull.
 */
void pack_overflow(void)
{
    int item = INVEN_MAX_PACK;
    char o_name[80];
    object_type *o_ptr;

    if (!pack_is_overfull())
	return;

    /* Get the slot to be dropped */
    o_ptr = &p_ptr->inventory[item];

    /* Disturbing */
    disturb(0, 0);

    /* Warning */
    msg("Your pack overflows!");

    /* Describe */
    object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

    /* Message */
    msg("You drop %s (%c).", o_name, index_to_label(item));

    /* Drop it (carefully) near the player */
    drop_near(o_ptr, 0, p_ptr->py, p_ptr->px, FALSE);

    /* Modify, Describe, Optimize */
    inven_item_increase(item, -255);
    inven_item_describe(item);
    inven_item_optimize(item);

    /* Notice stuff (if needed) */
    if (p_ptr->notice)
	notice_stuff(p_ptr);

    /* Update stuff (if needed) */
    if (p_ptr->update)
	update_stuff(p_ptr);

    /* Redraw stuff (if needed) */
    if (p_ptr->redraw)
	redraw_stuff(p_ptr);
}

/* Set Item Code */

/**
 * Determin if a given Set of artifacts is being used by the player.
 */
bool check_set(byte set_idx)
{
    byte count = 0;
    byte i;
    set_type *set_ptr = &set_info[set_idx];;

    for (i = INVEN_WIELD; i <= INVEN_FEET; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];
	if (o_ptr->name1) {
	    artifact_type *a_ptr = &a_info[o_ptr->name1];
	    if (a_ptr->set_no == set_idx) {
		count++;
	    }
	}
    }

    return (count >= set_ptr->no_of_items);
}

/**
 * Apply bonuses for complete artifact sets.
 */
void apply_set(int set_idx)
{
    set_type *set_ptr = &set_info[set_idx];

    bool bonus_applied = FALSE;

    byte i, j, k;

    for (i = INVEN_WIELD; i <= INVEN_FEET; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Is it an artifact? */
	if (o_ptr->name1) {
	    artifact_type *a_ptr = &a_info[o_ptr->name1];

	    /* Is it in the correct set? */
	    if (a_ptr->set_no == set_idx) {

		/* Loop through set elements */
		for (j = 0; j < (set_ptr->no_of_items); j++) {

		    set_element *se_ptr = &set_ptr->set_items[j];

		    /* Correct Element? */
		    if (se_ptr->a_idx == o_ptr->name1) {

			/* Bonus already applied? */
			if (!(a_ptr->set_bonus)) {
			    of_union(o_ptr->flags_obj, se_ptr->flags_obj);
			    cf_union(o_ptr->flags_curse, se_ptr->flags_curse);
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
	msg("Item set completed!");
}

/**
 * Remove bonuses for no-longer-complete artifact sets.
 */
void remove_set(int set_idx)
{
    set_type *set_ptr = &set_info[set_idx];

    bool bonus_removed = FALSE;

    byte i, j, k;

    for (i = INVEN_WIELD; i <= INVEN_FEET; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Is it an artifact? */
	if (o_ptr->name1) {
	    artifact_type *a_ptr = &a_info[o_ptr->name1];

	    /* Is it in the correct set? */
	    if (a_ptr->set_no == set_idx) {

		/* Loop through set elements */
		for (j = 0; j < (set_ptr->no_of_items); j++) {

		    set_element *se_ptr = &set_ptr->set_items[j];

		    /* Correct Element? */
		    if (se_ptr->a_idx == o_ptr->name1) {

			/* Is the bonus really there? */
			if (a_ptr->set_bonus) {
			    of_diff(o_ptr->flags_obj, se_ptr->flags_obj);
			    cf_diff(o_ptr->flags_curse, se_ptr->flags_curse);
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
	msg("Item set no longer completed.");
}

