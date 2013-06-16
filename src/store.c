/** \file store.c 
    \brief Store handling

 * Store comments, racial penalties, what an item will cost, store 
 * inventory management, what a store will buy (what stores will sell
 * is in init2.c, and store owners in tables.c), interacting with the
 * stores (entering, haggling, buying, selling, leaving, getting kicked
 * out, etc.).  Shuffle store owners.
 *
 * Copyright (c) 2009 Nick McConnell, Ben Harrison, James E. Wilson, 
 * Robert A. Koeneke
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
#include "cave.h"
#include "cmds.h"
#include "files.h"
#include "game-event.h"
#include "history.h"
#include "squelch.h"
#include "textui.h"
#include "ui-menu.h"


#define MAX_COMMENT_0	6

static const char *comment_0[MAX_COMMENT_0] = {
    "Okay.",
    "Fine.",
    "Accepted!",
    "Agreed!",
    "Done!",
    "Taken!"
};

/**
 * Successful haggle.
 */
static void say_comment_0(void)
{
    msg(comment_0[randint0(MAX_COMMENT_0)]);
}


/**
 * Messages for reacting to purchase prices.
 */

#define MAX_COMMENT_1 	4

static const char *comment_1[MAX_COMMENT_1] = {
    "Arrgghh!",
    "You bastard!",
    "You hear someone sobbing...",
    "The shopkeeper howls in agony!"
};

#define MAX_COMMENT_2 	4

static const char *comment_2[MAX_COMMENT_2] = {
    "Robbery!",
    "You fiend!",
    "The shopkeeper curses at you.",
    "The shopkeeper glares at you."
};

#define MAX_COMMENT_3	4

static const char *comment_3[MAX_COMMENT_3] = {
    "Cool!",
    "You've made my day!",
    "The shopkeeper giggles.",
    "The shopkeeper laughs loudly."
};

#define MAX_COMMENT_4 	4

static const char *comment_4[MAX_COMMENT_4] = {
    "Yipee!",
    "I think I'll retire!",
    "The shopkeeper jumps for joy.",
    "The shopkeeper smiles gleefully."
};


/**
 * Let a shop-keeper React to a purchase
 *
 * We paid "price", it was worth "value", and we thought it was worth "guess"
 */
static void purchase_analyze(s32b price, s32b value, s32b guess)
{
    /* Item was worthless, but we bought it */
    if ((value <= 0) && (price > value)) {
	/* Comment */
	msgt(MSG_STORE1, comment_1[randint0(MAX_COMMENT_1)]);

	/* Sound */
	sound(MSG_STORE1);
    }

    /* Item was cheaper than we thought, and we paid more than necessary */
    else if ((value < guess) && (price > value)) {
	/* Comment */
	msgt(MSG_STORE2, comment_2[randint0(MAX_COMMENT_2)]);

	/* Sound */
	sound(MSG_STORE2);
    }

    /* Item was a good bargain, and we got away with it */
    else if ((value > guess) && (value < (4 * guess)) && (price < value)) {
	/* Comment */
	msgt(MSG_STORE3, comment_3[randint0(MAX_COMMENT_3)]);

	/* Sound */
	sound(MSG_STORE3);
    }

    /* Item was a great bargain, and we got away with it */
    else if ((value > guess) && (price < value)) {
	/* Comment */
	msgt(MSG_STORE4, comment_4[randint0(MAX_COMMENT_4)]);

	/* Sound */
	sound(MSG_STORE4);
    }
}





/**
 * We store the current "store number" here so everyone can access it
 */
static int store_num = (MAX_STORES - 1);

/**
 * We store the current town index here so everyone can access it
 */
static int town = 0;

/**
 * We store the current "store page" here so everyone can access it
 */
static int store_top = 0;

/**
 * We store the number of items per page here so everyone can access it
 */
static int store_per = 12;

/**
 * We store the current "store pointer" here so everyone can access it
 */
static store_type *st_ptr = NULL;

/**
 * We store the current "owner type" here so everyone can access it
 */
static owner_type *ot_ptr = NULL;

/**
 * We store the array of current owners here so everyone can access it
 */
static byte owner[MAX_STORES] =
    { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

/**
 * Determine the price of an object (qty one) in a store.  Altered in 
 * Oangband.
 *
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 *
 * Charisma adjustments run from 80 to 150.
 * Racial adjustments run from 85 to 130.
 * Greed adjustments run from 102 to 210.
 */
s32b price_item(object_type * o_ptr, int greed, bool flip)
{
    int racial_factor, charisma_factor;
    s32b offer_adjustment, price, purse;

    /* Get the value of one of the items */
    price = object_value(o_ptr);

    /* Worthless items */
    if (price <= 0)
	return (0L);

    /* Compute the racial factor */
    racial_factor = g_info[(ot_ptr->owner_race * z_info->p_max) + p_ptr->prace];

    /* Paranoia */
    if (!racial_factor)
	racial_factor = 100;

    /* Add in the charisma factor */
    charisma_factor = adj_chr_gold[p_ptr->state.stat_ind[A_CHR]];

    /* Shop is buying */
    if (flip) {
	/* Calculate the offer adjustment (x100). */
	offer_adjustment =
	    100000000L / (racial_factor * charisma_factor * greed);

	/* Never get "silly" */
	if (offer_adjustment > 100L)
	    offer_adjustment = 100L;
    }

    /* Shop is selling */
    else {
	/* Calculate the offer adjustment (x100). */
	offer_adjustment =
	    1L * (long) racial_factor *charisma_factor * greed / 10000L;

	/* Never get "silly" */
	if (offer_adjustment < 100L)
	    offer_adjustment = 100L;
    }

    /* Compute the final price (with rounding) */
    price = (price * offer_adjustment + 50L) / 100L;

    /* Get the owner's payout limit */
    purse = (s32b) (ot_ptr->max_cost);

    /* I'm not paying that */
    if (flip && (price >= purse))
	price = purse;

    /* Note -- Never become "free" */
    if (price <= 0L)
	return (1L);

    /* Return the price */
    return (price);
}


/**
 * Special "mass production" computation
 */
static int mass_roll(int num, int max)
{
    int i, t = 0;
    for (i = 0; i < num; i++) {
	t += ((max > 1) ? randint0(max) : 1);
    }
    return (t);
}


/**
 * Certain "cheap" objects should be created in "piles"
 * Some objects can be sold at a "discount" (in small piles)
 */
static void mass_produce(object_type * o_ptr)
{
    int size = 1;
    int discount = 0;

    int discount_probability, max_purse, temp;

    s32b cost = object_value(o_ptr);


    /* Analyze the type */
    switch (o_ptr->tval) {
	/* Food, Flasks, and Lights */
    case TV_FOOD:
    case TV_FLASK:
    case TV_LIGHT:
	{
	    if (cost < 6L)
		size += mass_roll(3, 5);
	    if (cost < 21L)
		size += mass_roll(3, 5);
	    break;
	}

	/* The Black Market sells potions and scrolls in *quantity*.  This
	 * allows certain utility objects to be actually worth using. -LM- */
    case TV_POTION:
    case TV_SCROLL:
	{
	    if ((st_ptr->type == 6) && (randint1(2) == 1)) {
		if (cost < 400L)
		    size += mass_roll(15, 3);
	    } else {
		if (cost < 61L)
		    size += mass_roll(3, 5);
		if (cost < 241L)
		    size += mass_roll(1, 5);
	    }
	    break;
	}

    case TV_MAGIC_BOOK:
    case TV_PRAYER_BOOK:
    case TV_DRUID_BOOK:
    case TV_NECRO_BOOK:
	{
	    if (cost < 51L)
		size += mass_roll(2, 3);
	    if (cost < 501L)
		size += mass_roll(1, 3);
	    break;
	}

    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SHIELD:
    case TV_GLOVES:
    case TV_BOOTS:
    case TV_CLOAK:
    case TV_HELM:
    case TV_CROWN:
    case TV_SWORD:
    case TV_POLEARM:
    case TV_HAFTED:
    case TV_DIGGING:
    case TV_BOW:
	{
	    if (o_ptr->name2)
		break;
	    if (cost < 11L)
		size += mass_roll(3, 5);
	    if (cost < 101L)
		size += mass_roll(2, 5);
	    break;
	}

    case TV_SPIKE:
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
	{
	    if (cost < 6L)
		size += mass_roll(2, 5);
	    if (cost < 51L)
		size += mass_roll(2, 5);
	    if (cost < 501L)
		size += mass_roll(12, 5);
	    break;
	}

	/* Because many rods (and a few wands and staffs) are useful mainly in 
	 * quantity, the Black Market will occasionally have a bunch of one
	 * kind. -LM- */
    case TV_ROD:
    case TV_WAND:
    case TV_STAFF:
	{
	    if ((st_ptr->type == 6) && (randint1(3) == 1)) 
	    {
		if (cost < 1601L)
		    size += mass_roll(1, 5);
		else if (cost < 3201L)
		    size += mass_roll(1, 3);
	    }
	    /* Ensure that mass-produced devices get the correct pvals. */
	    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND) || 
		(o_ptr->tval == TV_STAFF)) {
		o_ptr->pval *= size;
	    }
	    break;
	}
    }


    /* Determine the discount probability modifier.  The purpose of this is to
     * make shopkeepers with low purses offer more sales, as one might expect.
     * -LM- */

    /* Hack - determine the maximum possible purse available at any one store. */
    if (st_ptr->type == 0)
	max_purse = 450;
    if (st_ptr->type == 4)
	max_purse = 15000;
    else
	max_purse = 30000;

    /* This to prevent integer math ruining accuracy. */
    temp = ot_ptr->max_cost / 10;

    /* Determine discount probability (inflated by 10) but stay reasonable. */
    discount_probability = max_purse / temp;
    if (discount_probability > 50)
	discount_probability = 50;


    /* Pick a discount.  Despite the changes in the formulas, average discount
     * frequency hasn't changed much. */
    if (cost < 5) {
	discount = 0;
    } else if (randint0(400 / discount_probability) == 0) {
	discount = 25;
    } else if (randint0(2000 / discount_probability) == 0) {
	discount = 50;
    } else if (randint0(4000 / discount_probability) == 0) {
	discount = 75;
    } else if (randint0(6000 / discount_probability) == 0) {
	discount = 90;
    }


    /* Save the discount */
    o_ptr->discount = discount;

    /* Save the total pile size */
    o_ptr->number = size - (size * discount / 100);

    /* Devices share the timeout/charges */
    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND) || 
	(o_ptr->tval == TV_STAFF)) 
    {
	o_ptr->pval = o_ptr->number * k_info[o_ptr->k_idx].pval.base;
    }
}



/**
 * Convert a store item index into a one character label
 *
 * We use labels "a"-"l" for page 1, and labels "m"-"x" for page 2.
 */
static s16b store_to_label(int i)
{
    /* Assume legal */
    return (I2A(i));
}


/**
 * Convert a one character label into a store item index.
 *
 * Return "-1" if the label does not indicate a real store item.
 */
static s16b label_to_store(int c)
{
    int i;

    /* Convert */
    i = (islower(c) ? A2I(c) : -1);

    /* Verify the index */
    if ((i < 0) || (i >= st_ptr->stock_num))
	return (-1);

    /* Return the index */
    return (i);
}




/**
 * Allow a store object to absorb another object
 */
static void store_object_absorb(object_type * o_ptr, object_type * j_ptr)
{
    int total = o_ptr->number + j_ptr->number;

    /* Combine quantity, lose excess items */
    o_ptr->number = (total > 99) ? 99 : total;

    /* If rods are stacking, add the pvals (maximum timeouts) together */
    if (o_ptr->tval == TV_ROD)
	o_ptr->pval += j_ptr->pval;

    /* If wands/staffs are stacking, combine the charges. */
    if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF))
	o_ptr->pval += j_ptr->pval;

    if ((o_ptr->origin != j_ptr->origin) ||
	(o_ptr->origin_stage != j_ptr->origin_stage) ||
	(o_ptr->origin_xtra != j_ptr->origin_xtra))
    {
	int act = 2;

	if ((o_ptr->origin == ORIGIN_DROP) && (o_ptr->origin == j_ptr->origin))
	{
	    monster_race *r_ptr = &r_info[o_ptr->origin_xtra];
	    monster_race *s_ptr = &r_info[j_ptr->origin_xtra];

	    bool r_uniq = rf_has(r_ptr->flags, RF_UNIQUE) ? TRUE : FALSE;
	    bool s_uniq = rf_has(s_ptr->flags, RF_UNIQUE) ? TRUE : FALSE;

	    if (r_uniq && !s_uniq) act = 0;
	    else if (s_uniq && !r_uniq) act = 1;
	    else act = 2;
	}

	switch (act)
	{
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


/**
 * Check to see if the shop will be carrying too many objects
 *
 * Note that the shop, just like a player, will not accept things
 * it cannot hold.  Before, one could "nuke" objects this way, by
 * adding them to a pile which was already full.
 */
static bool store_check_num(object_type * o_ptr)
{
    int i;
    object_type *j_ptr;

    /* Free space is always usable */
    if (st_ptr->stock_num < st_ptr->stock_size)
	return TRUE;

    /* The "home" acts like the player */
    if (st_ptr->type == STORE_HOME) {
	/* Check all the objects */
	for (i = 0; i < st_ptr->stock_num; i++) {
	    /* Get the existing object */
	    j_ptr = &st_ptr->stock[i];

	    /* Can the new object be combined with the old one? */
	    if (object_similar(j_ptr, o_ptr, OSTACK_PACK))
		return (TRUE);
	}
    }

    /* Normal stores do special stuff */
    else {
	/* Check all the objects */
	for (i = 0; i < st_ptr->stock_num; i++) {
	    /* Get the existing object */
	    j_ptr = &st_ptr->stock[i];

	    /* Can the new object be combined with the old one? */
	    if (object_similar(j_ptr, o_ptr, OSTACK_STORE))
		return (TRUE);
	}
    }

    /* But there was no room at the inn... */
    return (FALSE);
}




/**
 * Determine if the current store will purchase the given object
 *
 * Note that a shop-keeper must refuse to buy "worthless" objects
 */
static bool store_will_buy(const object_type * o_ptr)
{
    /* Hack -- The Home is simple */
    if (st_ptr->type == STORE_HOME)
	return (TRUE);

    /* Switch on the store */
    switch (st_ptr->type) {
	/* General Store */
    case 0:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_FOOD:
	    case TV_LIGHT:
	    case TV_FLASK:
	    case TV_SPIKE:
	    case TV_SHOT:
	    case TV_ARROW:
	    case TV_BOLT:
	    case TV_DIGGING:
	    case TV_CLOAK:
		break;
	    default:
		return (FALSE);
	    }
	    break;
	}

	/* Armoury */
    case 1:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_BOOTS:
	    case TV_GLOVES:
	    case TV_CROWN:
	    case TV_HELM:
	    case TV_SHIELD:
	    case TV_CLOAK:
	    case TV_SOFT_ARMOR:
	    case TV_HARD_ARMOR:
	    case TV_DRAG_ARMOR:
		break;
	    default:
		return (FALSE);
	    }
	    break;
	}

	/* Weapon Shop */
    case 2:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_SHOT:
	    case TV_BOLT:
	    case TV_ARROW:
	    case TV_BOW:
	    case TV_DIGGING:
	    case TV_HAFTED:
	    case TV_POLEARM:
	    case TV_SWORD:
		break;
	    default:
		return (FALSE);
	    }
	    break;
	}

	/* Temple */
    case 3:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_SCROLL:
	    case TV_POTION:
	    case TV_HAFTED:
		break;

		/* Hack -- The temple will buy known blessed swords and
		 * polearms. */
	    case TV_SWORD:
	    case TV_POLEARM:
		{
		    if (of_has(o_ptr->flags_obj, OF_BLESSED)
			&& object_known_p(o_ptr))
			break;
		    else
			return (FALSE);
		}

	    default:
		return (FALSE);
	    }
	    break;
	}

	/* Alchemist */
    case 4:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_SCROLL:
	    case TV_POTION:
		break;
	    default:
		return (FALSE);
	    }
	    break;
	}

	/* Magic Shop */
    case 5:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_AMULET:
	    case TV_RING:
	    case TV_STAFF:
	    case TV_WAND:
	    case TV_ROD:
		break;
	    default:
		return (FALSE);
	    }
	    break;
	}

	/* The Bookseller */
    case 8:
	{
	    /* Analyze the type */
	    switch (o_ptr->tval) {
	    case TV_MAGIC_BOOK:
	    case TV_PRAYER_BOOK:
	    case TV_DRUID_BOOK:
	    case TV_NECRO_BOOK:
		break;
	    default:
		return (FALSE);
	    }
	    break;
	}
    }

    /* Ignore "worthless" items XXX XXX XXX */
    if (object_value(o_ptr) <= 0)
	return (FALSE);

    /* Assume okay */
    return (TRUE);
}



/**
 * Add an object to the inventory of the "Home"
 *
 * In all cases, return the slot (or -1) where the object was placed.
 *
 * Note that this is a hacked up version of "inven_carry()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" becoming
 * known, the player may have to pick stuff up and drop it again.
 */
static int home_carry(object_type * o_ptr)
{
    int i, slot;
    s32b value, j_value;
    object_type *j_ptr;


    /* Check each existing object (try to combine) */
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
	/* Get the existing object */
	j_ptr = &st_ptr->stock[slot];

	/* The home acts just like the player */
	if (object_similar(j_ptr, o_ptr, OSTACK_PACK)) {
	    /* Save the new number of items */
	    object_absorb(j_ptr, o_ptr);

	    /* All done */
	    return (slot);
	}
    }

    /* No space? */
    if (st_ptr->stock_num >= st_ptr->stock_size)
	return (-1);


    /* Determine the "value" of the object */
    value = object_value(o_ptr);

    /* Check existing slots to see if we must "slide" */
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
	/* Get that object */
	j_ptr = &st_ptr->stock[slot];

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

	/* Can happen in the home */
	if (!object_aware_p(o_ptr))
	    continue;
	if (!object_aware_p(j_ptr))
	    break;

	/* Objects sort by increasing sval */
	if (o_ptr->sval < j_ptr->sval)
	    break;
	if (o_ptr->sval > j_ptr->sval)
	    continue;

	/* Objects in the home can be unknown */
	if (!object_known_p(o_ptr))
	    continue;
	if (!object_known_p(j_ptr))
	    break;

	/* Objects sort by decreasing value */
	j_value = object_value(j_ptr);
	if (value > j_value)
	    break;
	if (value < j_value)
	    continue;
    }

    /* Slide the others up */
    for (i = st_ptr->stock_num; i > slot; i--) {
	/* Hack -- slide the objects */
	object_copy(&st_ptr->stock[i], &st_ptr->stock[i - 1]);
    }

    /* More stuff now */
    st_ptr->stock_num++;

    /* Hack -- Insert the new object */
    object_copy(&st_ptr->stock[slot], o_ptr);

    /* Return the location */
    return (slot);
}


/**
 * Add an object to a real stores inventory.
 *
 * If the object is "worthless", it is thrown away (except in the home).
 *
 * If the object cannot be combined with an object already in the inventory,
 * make a new slot for it, and calculate its "per item" price.  Note that
 * this price will be negative, since the price will not be "fixed" yet.
 * Adding an object to a "fixed" price stack will not change the fixed price.
 *
 * In all cases, return the slot (or -1) where the object was placed
 */
static int store_carry(object_type * o_ptr)
{
    int i, slot;
    s32b value, j_value;
    object_type *j_ptr;
    bool merch_valid = FALSE;


    /* Evaluate the object */
    value = object_value(o_ptr);

    /* Cursed/Worthless items "disappear" when sold */
    if (value <= 0)
	return (-1);

    /* The merchant puts everything in his travelling bag */
    if (st_ptr->type == STORE_MERCH) {
	for (i = 0; i < st_ptr->table_num; i++)
	    if (o_ptr->k_idx == st_ptr->table[i])
		merch_valid = TRUE;
	if (!merch_valid)
	    return (-1);
    }

    /* Erase the feeling */
    o_ptr->feel = FEEL_NONE;

    /* Erase the inscription */
    o_ptr->note = 0;

    /* Check each existing object (try to combine) */
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
	/* Get the existing object */
	j_ptr = &st_ptr->stock[slot];

	/* Can the existing items be incremented? */
	if (object_similar(j_ptr, o_ptr, OSTACK_STORE)) {
	    /* Hack -- extra items disappear */
	    store_object_absorb(j_ptr, o_ptr);

	    /* All done */
	    return (slot);
	}
    }

    /* No space? */
    if (st_ptr->stock_num >= st_ptr->stock_size)
	return (-1);


    /* Check existing slots to see if we must "slide" */
    for (slot = 0; slot < st_ptr->stock_num; slot++) {
	/* Get that object */
	j_ptr = &st_ptr->stock[slot];

	/* Objects sort by decreasing type */
	if (o_ptr->tval > j_ptr->tval)
	    break;
	if (o_ptr->tval < j_ptr->tval)
	    continue;

	/* Objects sort by increasing sval */
	if (o_ptr->sval < j_ptr->sval)
	    break;
	if (o_ptr->sval > j_ptr->sval)
	    continue;

	/* Evaluate that slot */
	j_value = object_value(j_ptr);

	/* Objects sort by decreasing value */
	if (value > j_value)
	    break;
	if (value < j_value)
	    continue;
    }

    /* Slide the others up */
    for (i = st_ptr->stock_num; i > slot; i--) {
	/* Hack -- slide the objects */
	object_copy(&st_ptr->stock[i], &st_ptr->stock[i - 1]);
    }

    /* More stuff now */
    st_ptr->stock_num++;

    /* Hack -- Insert the new object */
    object_copy(&st_ptr->stock[slot], o_ptr);

    /* Return the location */
    return (slot);
}


/**
 * Increase, by a given amount, the number of a certain item
 * in a certain store.  This can result in zero items.
 */
static void store_item_increase(int item, int num)
{
    int cnt;
    object_type *o_ptr;

    /* Get the object */
    o_ptr = &st_ptr->stock[item];

    /* Verify the number */
    cnt = o_ptr->number + num;
    if (cnt > 255)
	cnt = 255;
    else if (cnt < 0)
	cnt = 0;
    num = cnt - o_ptr->number;

    /* Save the new number */
    o_ptr->number += num;
}


/**
 * Remove a slot if it is empty
 */
static void store_item_optimize(int item)
{
    int j;
    object_type *o_ptr;

    /* Get the object */
    o_ptr = &st_ptr->stock[item];

    /* Must exist */
    if (!o_ptr->k_idx)
	return;

    /* Must have no items */
    if (o_ptr->number)
	return;

    /* One less object */
    st_ptr->stock_num--;

    /* Slide everyone */
    for (j = item; j < st_ptr->stock_num; j++) {
	st_ptr->stock[j] = st_ptr->stock[j + 1];
    }

    /* Nuke the final slot */
    object_wipe(&st_ptr->stock[j]);
}


/**
 * This function will keep 'crap' out of the black market.
 * Crap is defined as any object that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 */
static bool black_market_crap(object_type * o_ptr)
{
    int i, j, base = MAX_STORES - 1;

    /* Ego items are never crap */
    if (o_ptr->name2)
	return (FALSE);

    /* Good items are never crap */
    if (o_ptr->to_a > 0)
	return (FALSE);
    if (o_ptr->to_h > 0)
	return (FALSE);
    if (o_ptr->to_d > 0)
	return (FALSE);

    /* Get into the right town */
    while (store_num <= base)
	base -= MAX_STORES_BIG;

    /* Check the other "normal" stores */
    for (i = 0; i < MAX_STORES_BIG; i++) {
	/* Ignore the Home and the Black Market itself. */
	if (i == STORE_HOME)
	    continue;
	if (i == STORE_BLACKM)
	    continue;

	/* Check every object in the store */
	for (j = 0; j < store[base + i].stock_num; j++) {
	    object_type *j_ptr = &store[base + i].stock[j];

	    /* Duplicate object "type", assume crappy */
	    if (o_ptr->k_idx == j_ptr->k_idx)
		return (TRUE);
	}
    }

    /* Assume okay */
    return (FALSE);
}


/**
 * Attempt to delete (some of) a random object from the store
 * Hack -- we attempt to "maintain" piles of items when possible.
 */
static void store_delete(void)
{
    int what, num;
    object_type *o_ptr;

    /* Paranoia */
    if (st_ptr->stock_num <= 0)
	return;

    /* Pick a random slot */
    what = randint0(st_ptr->stock_num);

    /* Get the object */
    o_ptr = &st_ptr->stock[what];

    /* Determine how many objects are in the slot */
    num = o_ptr->number;

    /* Hack -- sometimes, only destroy half the objects */
    if (randint0(100) < 50)
	num = (num + 1) / 2;

    /* Hack -- sometimes, only destroy a single object, if not a missile. */
    if (randint0(100) < 50) {
	if ((o_ptr->tval != TV_BOLT)
	    && (o_ptr->tval != TV_ARROW)
	    && (o_ptr->tval != TV_SHOT))
	    num = 1;
    }

    /* Hack -- decrement the maximum timeouts and total charges of devices */
    if ((o_ptr->tval == TV_ROD)	|| (o_ptr->tval == TV_WAND) || 
	(o_ptr->tval == TV_STAFF)) 
    	o_ptr->pval -= num * o_ptr->pval / o_ptr->number;

    /* Is the item an artifact? Mark it as lost if the player has it in 
     * history list */
    if (artifact_p(o_ptr))
	history_lose_artifact(o_ptr->name1);

    /* Actually destroy (part of) the object */
    store_item_increase(what, -num);
    store_item_optimize(what);
}


/**
 * Creates a random object and gives it to a store
 * This algorithm needs to be rethought.  A lot.
 * Currently, "normal" stores use a pre-built array.
 *
 * Note -- the "level" given to "obj_get_num()" is a "favored"
 * level, that is, there is a much higher chance of getting
 * items with a level approaching that of the given level...
 *
 * Should we check for "permission" to have the given object?
 */
static void store_create(void)
{
    int k_idx, tries, level;

    object_type *i_ptr;
    object_type object_type_body;

    /* Paranoia -- no room left */
    if (st_ptr->stock_num >= st_ptr->stock_size)
	return;


    /* Hack -- consider up to four items */
    for (tries = 0; tries < 4; tries++) {
	/* Black Market */
	if (st_ptr->type == STORE_BLACKM) {
	    /* Pick a level for object/magic.  Now depends partly on player
	     * level. */
	    level = 5 + p_ptr->lev / 2 + randint0(30);

	    /* Random object kind (biased towards given level) */
	    k_idx = get_obj_num(level);

	    /* Handle failure */
	    if (!k_idx)
		continue;
	}

	/* Normal Store */
	else {
	    int num = st_ptr->table_num;

	    /* Hack -- Pick an object kind to sell */
	    int i = randint0(num);

	    while ((st_ptr->table[i] == 0)
		   || (st_ptr->table[i] > z_info->k_max))
		i = randint0(num);

	    k_idx = st_ptr->table[i];

	    /* Hack -- fake level for apply_magic() */
	    level = rand_range(1, STORE_OBJ_LEVEL);
	}

	/* Get local object */
	i_ptr = &object_type_body;

	/* Create a new object of the chosen kind */
	object_prep(i_ptr, k_idx, RANDOMISE);

	/* Apply some "low-level" magic (no artifacts) */
	apply_magic(i_ptr, level, FALSE, FALSE, FALSE);

	/* Hack -- Charge light's */
	if (i_ptr->tval == TV_LIGHT) {
	    if (i_ptr->sval == SV_LIGHT_TORCH)
		i_ptr->pval = FUEL_TORCH / 2;
	    if (i_ptr->sval == SV_LIGHT_LANTERN)
		i_ptr->pval = FUEL_LAMP / 2;
	}

	/* The object is "known" */
	object_known(i_ptr);

	/* Item belongs to a store */
	i_ptr->ident |= IDENT_STORE;

	/* No origin yet */ 
	i_ptr->origin = ORIGIN_NONE;

	/* Shown curses are shown */
	if (of_has(i_ptr->flags_obj, OF_SHOW_CURSE))
	    cf_copy(i_ptr->id_curse, i_ptr->flags_curse);

	/* Mega-Hack -- no chests in stores */
	if (i_ptr->tval == TV_CHEST)
	    continue;

	/* Prune the black market */
	if (st_ptr->type == STORE_BLACKM) {
	    /* Hack -- No "crappy" items */
	    if (black_market_crap(i_ptr))
		continue;

	    /* Hack -- No "cheap" items */
	    if (object_value(i_ptr) < 10)
		continue;

	    /* No "worthless" items */
	    /* if (object_value(i_ptr) <= 0) continue; */
	}

	/* Prune normal stores */
	else {
	    /* No "worthless" items */
	    if (object_value(i_ptr) <= 0)
		continue;
	}


	/* Mass produce and/or Apply discount */
	mass_produce(i_ptr);

	/* Attempt to carry the (known) object */
	(void) store_carry(i_ptr);

	/* Definitely done */
	break;
    }
}


/**
 * Redisplay a single store entry
 */
static void display_entry(int item)
{
    int y;
    object_type *o_ptr;
    s32b x;
	odesc_detail_t desc = ODESC_PREFIX;

    char o_name[120];
    char out_val[160];

    int maxwid = 75;
    int wgt;


    /* Must be on current "page" to get displayed */
    if (!((item >= store_top) && (item < store_top + store_per)))
	return;


    /* Get the object */
    o_ptr = &st_ptr->stock[item];

    /* Get the row */
    y = (item % store_per) + 5;

    /* Label it, clear the line --(-- */
    sprintf(out_val, "%c) ", store_to_label(item));
    prt(out_val, y, 0);

    /* Describe an object in the home */
    if (st_ptr->type == STORE_HOME) {
	byte attr;

	maxwid = 75;

	/* Leave room for weights, if necessary -DRS- */
	maxwid -= 10;

	/* Describe the object */
	desc = ODESC_FULL;
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | desc);
	o_name[maxwid] = '\0';

	/* Acquire inventory color.  Apply spellbook hack. */
	attr = proc_list_color_hack(o_ptr);

	/* Display the object */
	c_put_str(attr, o_name, y, 3);

	/* Only show the weight of a single object */
	wgt = o_ptr->weight;
	
	sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
	
	put_str(out_val, y, 67);
    }

    /* Describe an object (fully) in a store */
    else 
    {
	byte attr;

	/* Must leave room for the "price" */
	maxwid = 58;

	/* Describe the object (fully) */
	desc = ODESC_FULL | ODESC_STORE;
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | desc);
	o_name[maxwid] = '\0';

	/* Acquire inventory color.  Apply spellbook hack. */
	attr = proc_list_color_hack(o_ptr);

	/* Display the object */
	c_put_str(attr, o_name, y, 3);

	/* Show weights */
	wgt = o_ptr->weight;
	sprintf(out_val, "%3d.%d lb", wgt / 10, wgt % 10);
	put_str(out_val, y, 61);

	/* Extract the price */
	x = price_item(o_ptr, ot_ptr->inflate, FALSE);

	/* Actually draw the price */
	if (((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF)) && 
	    (o_ptr->number > 1))
	    sprintf(out_val, "%9ld avg", (long) x);
	else
	    sprintf(out_val, "%9ld  ", (long) x);

	put_str(out_val, y, 67);


    }
}


/**
 * Display a store's inventory
 *
 * All prices are listed as "per individual object"
 */
static void display_inventory(void)
{
    int i, k;

    /* Display the next store_per items */
    for (k = 0; k < store_per; k++) {
	/* Stop when we run out of items */
	if (store_top + k >= st_ptr->stock_num)
	    break;

	/* Display that line */
	display_entry(store_top + k);
    }

    /* Erase the extra lines and the "more" prompt */
    for (i = k; i < store_per + 1; i++)
	prt("", i + 5, 0);

    /* Assume "no current page" */
    put_str("        ", 4, 20);

    /* Visual reminder of "more items" */
    if (st_ptr->stock_num > store_per) {
	/* Show "more" reminder (after the last object ) */
	prt("-more-", 17, 3);

	/* Indicate the "current page" */
	put_str(format("(Page %d)", store_top / store_per + 1), 4, 20);
    }
}


/**
 * Display players gold
 */
static void store_prt_gold(void)
{
    char out_val[64];

    prt("Gold Remaining: ", 18, 53);

    sprintf(out_val, "%9ld", (long) p_ptr->au);
    prt(out_val, 18, 67);
}


/**
 * Display store (after clearing screen)
 */
static void display_store(void)
{
    char buf[80];

    /* Clear screen */
    Term_clear();

    /* The "Home" is special */
    if (st_ptr->type == STORE_HOME) 
    {
	/* Put the owner name */
	put_str("Your Home", 2, 30);

	/* Label the object descriptions */
	put_str("Item Description", 4, 3);

	/* If showing weights, show label */
	put_str("Weight", 4, 70);
    }

    /* Normal stores */
    else 
    {

	const char *store_name = f_info[FEAT_SHOP_HEAD + st_ptr->type].name;
	const char *owner_name = ot_ptr->owner_name;
	const char *race_name = p_info[ot_ptr->owner_race].name;


	/* Put the owner name and race */
	sprintf(buf, "%s (%s)", owner_name, race_name);
	put_str(buf, 2, 10);
	
	/* Show the max price in the store (above prices) */
	sprintf(buf, "%s (%ld)", store_name, (long) (ot_ptr->max_cost));
	prt(buf, 2, 50);
	
	/* Label the object descriptions */
	put_str("Item Description", 4, 3);

	/* If showing weights, show label */
	put_str("Weight", 4, 60);

	/* Label the asking price (in stores) */
	put_str("Price", 4, 71);

    }

    /* Display the current gold */
    store_prt_gold();

    /* Draw in the inventory */
    display_inventory();
}



/**
 * Get the index of a store object
 *
 * Return TRUE if an object was selected
 */
static bool get_stock(int *com_val, const char *pmt)
{
    int item;

    ui_event which;

    char buf[160];

    char o_name[120];

    char out_val[160];

    object_type *o_ptr;

    /* Assume failure */
    *com_val = (-1);

    /* Build the prompt */
    sprintf(buf, "(Items %c-%c, ESC to exit) %s", store_to_label(0),
	    store_to_label(st_ptr->stock_num - 1), pmt);

    /* Ask until done */
    while (TRUE) {
	int ver = 0;

	/* Escape */
	if (!get_com_ex(buf, &which))
	    return (FALSE);

	/* Extract "query" setting */
	if (isalpha(which.key.code)) {
	    ver = isupper(which.key.code);
	    which.key.code = tolower(which.key.code);
	}

	/* Convert response to item */
	if (which.mouse.button) {
	    if (!which.mouse.y)
		return (FALSE);
	    else if ((which.mouse.y > 4) && (which.mouse.y < 17))
		item = which.mouse.y + store_top - 5;
	    else
		item = -1;
	    if (item >= st_ptr->stock_num)
		item = -1;
	} else
	    item = label_to_store(which.key.code);

	/* Oops */
	if (item < 0) {
	    /* Oops */
	    bell("Illegal store object choice!");
	    continue;
	}

	/* No verification */
	if (!ver)
	    break;

	/* Object */
	o_ptr = &st_ptr->stock[item];

	/* Home */
	if (st_ptr->type == STORE_HOME) {
	    /* Describe */
	    object_desc(o_name, sizeof(o_name), o_ptr, 
			ODESC_PREFIX | ODESC_FULL);
	}

	/* Shop */
	else {
	    /* Describe */
	    object_desc(o_name, sizeof(o_name), o_ptr, 
			ODESC_PREFIX | ODESC_FULL);
	}

	/* Prompt */
	sprintf(out_val, "Try %s? ", o_name);

	/* Query */
	if (!get_check(out_val))
	    return (FALSE);

	/* Done */
	break;
    }

    /* Save item */
    (*com_val) = item;

    /* Success */
    return (TRUE);
}


/**
 * Return the quantity of a given item in the pack.
 */
static int find_inven(object_type * o_ptr)
{
    int j;

    /* Similar slot? */
    for (j = 0; j < INVEN_PACK; j++) {
	object_type *j_ptr = &p_ptr->inventory[j];

	/* Skip non-objects */
	if (!j_ptr->k_idx)
	    continue;

	/* Skip unknown */
	if (!object_aware_p(j_ptr))
	    continue;

	/* Check if the two items can be combined */
	if (object_similar(j_ptr, o_ptr, OSTACK_PACK))
	    return j_ptr->number;
    }

    return 0;
}


/**
 * Buy an object from a store
 */
static void store_purchase(void)
{
    int n;
    int amt, num;
    int item, item_new;

    s32b price;

    object_type *o_ptr;

    object_type *i_ptr;
    object_type object_type_body;

    char o_name[120];

    char out_val[160];


    /* Empty? */
    if (st_ptr->stock_num <= 0) {
	if (st_ptr->type == STORE_HOME) {
	    msg("Your home is empty.");
	} else {
	    msg("I am currently out of stock.");
	}
	return;
    }


    /* Prompt */
    if (st_ptr->type == STORE_HOME) {
	sprintf(out_val, "Which item do you want to take? ");
    } else {
	sprintf(out_val, "Which item are you interested in? ");
    }

    /* Get the object number to be bought */
    if (!get_stock(&item, out_val))
	return;

    /* Get the actual object */
    o_ptr = &st_ptr->stock[item];

    /* Clear all current messages */
    msg_flag = FALSE;
    prt("", 0, 0);
    msg_flag = TRUE;

    if (st_ptr->type == STORE_HOME) {
	amt = o_ptr->number;
    } else {
	/* Price of one */
	price = price_item(o_ptr, ot_ptr->inflate, FALSE);

	/* Check if the player can afford any at all */
	if ((u32b) p_ptr->au < (u32b) price) {
	    /* Tell the user */
	    msg("You do not have enough gold for this item.");

	    /* Abort now */
	    return;
	}

	/* Work out how many the player can afford */
	amt = p_ptr->au / price;
	if (amt > o_ptr->number)
	    amt = o_ptr->number;
    }

    /* Find the number of this item in the inventory */
    num = find_inven(o_ptr);

    /* Show number */
    prt(format("(you have %d)", num), 1, 0);

    /* Get a quantity */
    amt = get_quantity(NULL, amt);

    /* Allow user abort */
    if (amt <= 0)
	return;

    /* Get local object */
    i_ptr = &object_type_body;

    /* Get desired object */
    object_copy(i_ptr, o_ptr);

    /* Hack -- If a rod or wand, allocate total maximum timeouts or charges
     * between those purchased and left on the shelf. */
    reduce_charges(i_ptr, i_ptr->number - amt);

    /* Modify quantity */
    i_ptr->number = amt;

    /* Hack -- require room in pack */
    if (!inven_carry_okay(i_ptr)) {
	msg("You cannot carry that many items.");
	return;
    }

    /* Attempt to buy it */
    if (st_ptr->type != STORE_HOME) 
    {
	char answer;

	/* Describe the object (fully) */
	object_desc(o_name, sizeof(o_name), i_ptr, ODESC_PREFIX | ODESC_FULL);

	/* Haggle for a final price */
	price = price_item(i_ptr, ot_ptr->inflate, FALSE) * i_ptr->number;

	/* Confirm purchase */
	answer = get_char(format("Buy %s for %d gold?", o_name, price), "yn", 
			  2, 'y');

	if ((answer == (keycode_t) 'n') || !answer)
		return;

	/* Message */
	msg("Buying %s (%c).", o_name, store_to_label(item));
	message_flush();

	/* Player can afford it */
	if (p_ptr->au >= price) {
	    bool aware = FALSE;

	    /* Say "okay" */
	    say_comment_0();

	    /* Spend the money */
	    p_ptr->au -= price;

	    /* Update the display */
	    store_prt_gold();

	    /* Remember awareness */
	    if (object_aware_p(i_ptr))
		aware = TRUE;

	    /* Buying an object makes you aware of it */
	    object_aware(i_ptr);

	    /* Combine / Reorder the pack (later) */
	    p_ptr->notice |= (PN_COMBINE | PN_REORDER | PN_SORT_QUIVER);

	    /* The object no longer belongs to the store */
	    i_ptr->ident &= ~(IDENT_STORE);
	    i_ptr->origin = ORIGIN_STORE;
	    i_ptr->origin_stage = p_ptr->stage; 

	    /* Describe the transaction */
	    object_desc(o_name, sizeof(o_name), i_ptr, 
			ODESC_PREFIX | ODESC_FULL);

	    /* Message */
	    msg("You bought %s (%c) for %ld gold.", o_name,
		       store_to_label(item), (long) price);

	    /* Erase the feeling */
	    i_ptr->feel = FEEL_NONE;

	    /* Erase the inscription */
	    i_ptr->note = 0;

	    /* Give it to the player */
	    item_new = inven_carry(p_ptr, i_ptr);

	    /* Hack -- Apply autoinscriptions if we become aware of the object */
	    if (!aware)
		apply_autoinscription(&p_ptr->inventory[item_new]);

	    /* Describe the final result */
	    object_desc(o_name, sizeof(o_name), &p_ptr->inventory[item_new], 
			ODESC_PREFIX | ODESC_FULL);

	    /* Message */
	    msg("You have %s (%c).", o_name, index_to_label(item_new));

	    /* Now, reduce the original stack's pval. */
	    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND) || 
		(o_ptr->tval == TV_STAFF)) 
		o_ptr->pval -= i_ptr->pval;

	    /* Handle stuff */
	    handle_stuff(p_ptr);

	    /* Note how many slots the store used to have */
	    n = st_ptr->stock_num;

	    /* Remove the bought objects from the store */
	    store_item_increase(item, -amt);
	    store_item_optimize(item);

	    /* Store is empty */
	    if (st_ptr->stock_num == 0) {
		int i;

		/* Shuffle.  Made retiring more likely. */
		if (randint0(STORE_SHUFFLE) < 5) {
		    /* Message */
		    msg("The shopkeeper retires.");

		    /* Shuffle the store */
		    store_shuffle(store_num);
		}

		/* Maintain */
		else {
		    /* Message */
		    msg("The shopkeeper brings out some new stock.");
		}

		/* New inventory */
		for (i = 0; i < 10; ++i) {
		    /* Maintain the store */
		    store_maint(store_num);
		}

		/* Start over */
		store_top = 0;

		/* Redraw everything */
		display_inventory();
	    }

	    /* The object is gone */
	    else if (st_ptr->stock_num != n) {
		/* Only one screen left */
		if (st_ptr->stock_num <= store_per) {
		    store_top = 0;
		}

		/* Redraw everything */
		display_inventory();
	    }

	    /* The object is still here */
	    else {
		/* Redraw the object */
		display_entry(item);
	    }
	}

	/* Player cannot afford it */
	else {
	    /* Simple message (no insult) */
	    msg("You do not have enough gold.");
	}

    }

    /* Home is much easier */
    else {
	/* Distribute charges of wands or rods */
	distribute_charges(o_ptr, i_ptr, amt);

	/* Give it to the player */
	item_new = inven_carry(p_ptr, i_ptr);

	/* Describe just the result */
	object_desc(o_name, sizeof(o_name), &p_ptr->inventory[item_new], 
		    ODESC_PREFIX | ODESC_FULL);

	/* Message */
	msg("You have %s (%c).", o_name, index_to_label(item_new));

	/* Handle stuff */
	handle_stuff(p_ptr);

	/* Take note if we take the last one */
	n = st_ptr->stock_num;

	/* Remove the items from the home */
	store_item_increase(item, -amt);
	store_item_optimize(item);

	/* The object is gone */
	if (st_ptr->stock_num != n) {
	    /* Only one screen left */
	    if (st_ptr->stock_num <= store_per) {
		store_top = 0;
	    }

	    /* Redraw everything */
	    display_inventory();
	}

	/* The object is still here */
	else {
	    /* Redraw the object */
	    display_entry(item);
	}
    }

    /* Not kicked out */
    return;
}


/**
 * Sell an object to the store (or home)
 */
static void store_sell(void)
{
    int item, item_pos;
    int amt;

    s32b price = 0, value, dummy;

    object_type *o_ptr;

    object_type *i_ptr;
    object_type object_type_body;

    const char *q, *s;

    char o_name[120];


    /* Home */
    q = "Drop which item? ";

    /* Real store */
    if (st_ptr->type != STORE_HOME) {
	/* New prompt */
	q = "Sell which item? ";

	/* Only allow items the store will buy */
	item_tester_hook = store_will_buy;
    }

    /* Start off in equipment mode. */
    p_ptr->command_wrk = (USE_INVEN);

    /* Clear all current messages */
    msg_flag = FALSE;
    prt("", 0, 0);
    msg_flag = TRUE;

    /* Get an item */
    s = "You have nothing that I want.";
    if (!get_item(&item, q, s, CMD_DROP, (USE_EQUIP | USE_INVEN | USE_FLOOR)))
	return;

    /* Get the item (in the pack) */
    if (item >= 0) {
	o_ptr = &p_ptr->inventory[item];
    }

    /* Get the item (on the floor) */
    else {
	o_ptr = &o_list[0 - item];
    }


    /* Hack -- Cannot remove cursed items */
    if (cf_has(o_ptr->flags_curse, CF_STICKY_CARRY)) {
	/* Oops */
	msg("Hmmm, it seems to be cursed.");

	/* Notice */
	notice_curse(CF_STICKY_CARRY, item + 1);

	/* Nope */
	return;
    }

    if ((item >= INVEN_WIELD) && cf_has(o_ptr->flags_curse, CF_STICKY_WIELD)) {
	/* Oops */
	msg("Hmmm, it seems to be cursed.");

	/* Notice */
	notice_curse(CF_STICKY_WIELD, item + 1);

	/* Nope */
	return;
    }

    /* Get a quantity */
    amt = get_quantity(NULL, o_ptr->number);
    display_store();

    /* Allow user abort */
    if (amt <= 0)
	return;

    /* Get local object */
    i_ptr = &object_type_body;

    /* Get a copy of the object */
    object_copy(i_ptr, o_ptr);

    /* Modify quantity */
    i_ptr->number = amt;

    /* Hack -- If a rod or wand, allocate total maximum timeouts or charges to
     * those being sold. */
    if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND) || 
		(o_ptr->tval == TV_STAFF))
	i_ptr->pval = o_ptr->pval * amt / o_ptr->number;

    /* Get a full description */
    object_desc(o_name, sizeof(o_name), i_ptr, ODESC_PREFIX | ODESC_FULL);

    /* Is there room in the store (or the home?) */
    if ((!store_check_num(i_ptr)) && (st_ptr->type != STORE_MERCH)) 
    {
	if (st_ptr->type == STORE_HOME) 
	    msg("Your home is full.");
	else 
	    msg("I have not the room in my store to keep it.");
	return;
    }


    /* Real store */
    if (st_ptr->type != STORE_HOME) 
    {
	bool aware = FALSE;
	char answer;

	/* No selling */
	if (!MODE(NO_SELLING)) 
	{
	    /* Get the price */
	    price = price_item(i_ptr, ot_ptr->inflate, TRUE) * i_ptr->number;

	    /* Confirm sale */
	    msg("Selling %s (%c) for %d gold.", o_name,
		       index_to_label(item), price);

	    /* Confirm sale */
	    answer = get_char(format("Accept %d gold?", price), "yn", 2, 'y');
	    
	    if ((answer == (keycode_t) 'n') || !answer)
		return;

	    /* Say "okay" */
	    say_comment_0();

	    /* Get some money */
	    p_ptr->au += price;

	    /* Limit to avoid buffer overflow */
	    if (p_ptr->au > PY_MAX_GOLD)
		p_ptr->au = PY_MAX_GOLD;
	}

	else 
	{
	    answer = get_char(format("Donate %s?", o_name), "yn", 2, 'y');
	    
	    if ((answer == (keycode_t) 'n') || !answer)
		return;
	}

	/* Update the display */
	store_prt_gold();

	/* Get the "apparent" value */
	dummy = object_value(i_ptr) * i_ptr->number;

	/* Remember if we were aware of this object */
	if (object_aware_p(o_ptr))
	    aware = TRUE;

	/* Identify original object */
	object_aware(o_ptr);
	object_known(o_ptr);

	/* Erase the feeling */
	i_ptr->feel = FEEL_NONE;

	/* Remove any inscription */
	i_ptr->note = 0;

	/* Apply an autoiscription if any left */
	if ((amt < o_ptr->number) && (!aware))
	    apply_autoinscription(o_ptr);

	/* Update the auto-history if selling an artifact that was previously 
	 * un-IDed. (Ouch!) */
	if (artifact_p(o_ptr))
	    history_add_artifact(o_ptr->name1, TRUE, TRUE);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER | PN_SORT_QUIVER);

	/* Get local object */
	i_ptr = &object_type_body;

	/* Get a copy of the object */
	object_copy(i_ptr, o_ptr);

	/* Modify quantity */
	i_ptr->number = amt;

	/* The object belongs to the store now */
	i_ptr->ident |= IDENT_STORE;

	/* Shown curses are shown */
	if (of_has(i_ptr->flags_obj, OF_SHOW_CURSE))
	    cf_copy(i_ptr->id_curse, i_ptr->flags_curse);

	/* If a rod, staff or wand, let the shopkeeper know just how many
	 * charges he really paid for. */
	if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND) || 
	    (o_ptr->tval == TV_STAFF)) 
	    i_ptr->pval = o_ptr->pval * amt / o_ptr->number;
	
	/* Get the "actual" value */
	value = object_value(i_ptr) * i_ptr->number;

	/* Get the description all over again */
	object_desc(o_name, sizeof(o_name), i_ptr, ODESC_PREFIX | ODESC_FULL);

	/* No selling */
	if (MODE(NO_SELLING)) 
	{
	    /* Describe the result (in message buffer) */
	    msg("You gave over %s (%c).", o_name, index_to_label(item));
	} 
	else 
	{
	    /* Describe the result (in message buffer) */
	    msg("You sold %s (%c) for %ld gold.", o_name,
		       index_to_label(item), (long) price);

	    /* Analyze the prices (and comment verbally) */
	    purchase_analyze(price, value, dummy);
	}


	/* Hack -- Allocate charges between those wands or rods sold and
	 * retained, unless all are being sold. */
	distribute_charges(o_ptr, i_ptr, amt);

	/* Take the object from the player */
	inven_item_increase(item, -amt);
	inven_item_describe(item);
	inven_item_optimize(item);

	if (item == INVEN_WIELD)
	    p_ptr->state.shield_on_back = FALSE;


	/* Handle stuff */
	handle_stuff(p_ptr);

	/* The store gets that (known) object */
	item_pos = store_carry(i_ptr);

	/* Update the display */
	if (item_pos >= 0) 
	{
	    /* Redisplay wares */
	    display_inventory();
	}

    }

    /* Player is at home */
    else {
	/* Distribute charges of wands/rods */
	distribute_charges(o_ptr, i_ptr, amt);

	/* Describe */
	msg("You drop %s (%c).", o_name, index_to_label(item));


	/* Take it from the players inventory */
	inven_item_increase(item, -amt);
	inven_item_describe(item);
	inven_item_optimize(item);

	if (item == INVEN_WIELD)
	    p_ptr->state.shield_on_back = FALSE;


	/* Handle stuff */
	handle_stuff(p_ptr);

	/* Let the home carry it */
	item_pos = home_carry(i_ptr);

	/* Update store display */
	if (item_pos >= 0) {
	    /* Redisplay wares */
	    display_inventory();
	}
    }

    /* Redraw stuff */
    p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

    /* Reset hook */
    item_tester_hook = NULL;
}


/**
 * Get an object in a store to inspect. Original code by -JDL- (from Zangband).
 */
static void store_inspect(void)
{
    int item;

    object_type *o_ptr;

    char out_val[160];
    char header[120];

    textblock *tb;
    region area = { 0, 0, 0, 0 };

    /* Empty? */
    if (st_ptr->stock_num <= 0) {
	if (st_ptr->type == STORE_HOME)
	    msg("Your home is empty.");
	else
	    msg("I am currently out of stock.");
	return;
    }

    /* Prompt */
    sprintf(out_val, "Examine which item? ");

    /* Get the item number to be examined */
    if (!get_stock(&item, out_val))
	return;

    /* Get the actual item */
    o_ptr = &st_ptr->stock[item];

    /* Show full info in most stores, but normal info in player home */
    tb = object_info(o_ptr, (st_ptr->type != STORE_HOME) ? 
		     OINFO_FULL : OINFO_NONE);
    object_desc(header, sizeof(header), o_ptr, ODESC_PREFIX | ODESC_FULL);

    textui_textblock_show(tb, area, header);
    textblock_free(tb);
}

/**
 * First book entry in order_items 
 */
#define BOOK_START 2

/**
 * Categories for store orderiing 
 */
static tval_desc order_types[] = {
    {TV_SCROLL, "Scrolls"},
    {TV_POTION, "Potions"},
    {TV_MAGIC_BOOK, "Magic Books"},
    {TV_PRAYER_BOOK, "Prayer Books"},
    {TV_DRUID_BOOK, "Stones of Lore"},
    {TV_NECRO_BOOK, "Necromantic Tomes"}
};

/**
 * Display an entry on the sval menu
 */
static void order_item_display(menu_type * menu, int oid, bool cursor, int row,
			       int col, int width)
{
    char buf[80];
    const u16b *choice = menu->menu_data;
    int i, idx = choice[oid];

    byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);


    /* Acquire the "name" of object "i" */
    object_kind_name(buf, sizeof(buf), idx, TRUE);

    /* Check to see if ordered or not */
    for (i = 24; i < 32; i++) {
	/* Ordered */
	if (st_ptr->table[i] == idx)
	    break;
    }

    /* Print it */
    c_put_str(attr, format("[ ] %s", buf), row, col);
    if (i < 32)
	c_put_str(TERM_L_RED, "*", row, col + 1);
}

/**
 * Deal with events on the sval menu
 */
static bool order_item_action(menu_type *menu, const ui_event *ev, int oid)
{
    u16b *choice = menu->menu_data;
    int i, avail;

    /* Toggle */
    if (ev->type == EVT_SELECT) 
    {
	int idx = choice[oid];

	/* Check to see if ordered or not */
	avail = 0;
	for (i = 24; i < 32; i++) {
	    /* Ordered */
	    if (st_ptr->table[i] == idx)
		break;

	    /* First free slot */
	    if ((st_ptr->table[i] == 0) && (!avail))
		avail = i;
	}

	/* Cancel order */
	if (i < 32)
	    st_ptr->table[i] = 0;

	/* New order */
	else if (avail)
	    st_ptr->table[avail] = idx;

	/* No more orders */
	else
	    return FALSE;

	return TRUE;
    }

    return FALSE;
}

/**
 * Display list of orderable item types
 */
static bool order_menu(int tval, const char *desc)
{
    menu_type menu;
    menu_iter menu_f = { 0, 0, order_item_display, order_item_action, 0 };
    region area = { 1, 5, -1, -1 };
    int cursor = 0;

    int num = 0;
    size_t i;

    u16b *choice;


    /* Create the array */
    choice = C_ZNEW(z_info->k_max, u16b);

    /* Iterate over all possible object kinds, finding ones which can be
     * ordered */
    for (i = 1; i < z_info->k_max; i++) {
	object_kind *k_ptr = &k_info[i];

	/* Skip empty objects, unaware objects, and incorrect tvals */
	if (!k_ptr->name)
	    continue;
	if (!k_ptr->aware)
	    continue;
	if (kf_has(k_ptr->flags_kind, KF_NO_ORDER))
	    continue;
	if (k_ptr->tval != tval)
	    continue;

	/* Books  */
	if (tval >= TV_MAGIC_BOOK) {
	    if (k_ptr->sval >= 4)
		continue;
	    if (mp_ptr->book_start_index[k_ptr->sval] == 
		mp_ptr->book_start_index[k_ptr->sval + 1])
		continue;
	}


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

    /* Buttons */
    button_add("Up", ARROW_UP);
    button_add("Down", ARROW_DOWN);
    button_add("Toggle", '\r');

    /* Output to the screen */
    text_out_hook = text_out_to_screen;

    /* Indent output */
    text_out_indent = 1;
    text_out_wrap = 79;
    Term_gotoxy(1, 0);

    /* Display some helpful information */
    text_out("Use the ");
    text_out_c(TERM_L_GREEN, "movement keys");
    text_out(" to scroll the list or ");
    text_out_c(TERM_L_GREEN, "ESC");
    text_out(" to return to the previous menu.  ");
    text_out_c(TERM_L_BLUE, "Enter");
    text_out(" toggles the current setting.  ");
    text_out("At most eight items can be ordered.");

    text_out_indent = 0;

    /* Set up the menu */
    WIPE(&menu, menu);
    //menu.cmd_keys = " \n\r";
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
    menu_setpriv(&menu, num, choice);
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

    return TRUE;
}

static char tag_order(menu_type * menu, int oid)
{
    return I2A(oid);
}

static void display_order(menu_type * menu, int oid, bool cursor, int row,
			  int col, int width)
{
    int *choice = (int *) menu->menu_data;
    bool known = seen_tval(order_types[choice[oid]].tval);
    byte attr = curs_attrs[known ? CURS_KNOWN : CURS_UNKNOWN][(int) cursor];

    c_prt(attr, order_types[choice[oid]].desc, row, col);
}

/**
 * Display and handle the main squelching menu.
 */
void store_order(void)
{
    size_t i;
    int j = 0, cursor = 0;
    ui_event c = EVENT_EMPTY;
    const char cmd_keys[] = { (char) ARROW_LEFT, (char) ARROW_RIGHT, '\0' };

    menu_iter menu_f = { tag_order, 0, display_order, NULL, 0 };
    menu_type menu;
    int choice[3];

    for (i = 0; i < N_ELEMENTS(order_types); i++)
	if ((i < BOOK_START) || (mp_ptr->spell_book == order_types[i].tval))
	    choice[j++] = i;

    WIPE(&menu, menu_type);
    menu.title = "Item ordering menu";
    menu.cmd_keys = cmd_keys;
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
    menu_setpriv(&menu, BOOK_START + (mp_ptr->spell_book ? 1 : 0), choice);

    menu_layout(&menu, &SCREEN_REGION);

    /* Save and clear screen */
    screen_save();
    clear_from(0);

    while (c.type != EVT_ESCAPE) {
	clear_from(0);
	c = menu_select(&menu, cursor, FALSE);

	if (c.type == EVT_SELECT) {
	    order_menu(order_types[choice[menu.cursor]].tval,
		       order_types[choice[menu.cursor]].desc);
	}
    }

    /* Load screen and finish */
    screen_load();
    return;
}

/**
 * Hack -- set this to leave the store
 */
static bool leave_store = FALSE;


/**
 * Process a command in a store
 *
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 *
 * Hack -- note the bizarre code to handle the "=" command,
 * which is needed to prevent the "redraw" from affecting
 * the display of the store.  XXX XXX XXX
 */
static void store_process_command(ui_event ke)
{
    /* Clear messages */
    msg_flag = FALSE;

    /* Parse the command */
    switch (ke.key.code) {
	/* Leave */
    case ESCAPE:
	{
	    leave_store = TRUE;
	    break;
	}

	/* Browse */
    case ' ':
	{
	    if (st_ptr->stock_num <= store_per) {
		/* Nothing to see */
		msg("Entire inventory is shown.");
	    }

	    else if (store_top == 0) {
		/* Page 2 */
		store_top = store_per;

		/* Redisplay wares */
		display_inventory();
	    }

	    else {
		/* Page 1 */
		store_top = 0;

		/* Redisplay wares */
		display_inventory();
	    }

	    break;
	}

	/* Redraw */
    case KTRL('R'):
	{
	    do_cmd_redraw();
	    display_store();
	    break;
	}

	/* Get (purchase) */
    case 'g':
    case 'p':
	{
	    store_purchase();
	    break;
	}

	/* Drop (Sell) */
    case 'd':
    case 's':
	{
	    store_sell();
	    display_inventory();
	    break;
	}

	/* Ignore return */
    case '\r':
	{
	    break;
	}



      /*** Inventory Commands ***/

    case 'k': 

    case 't':
    case 'w':
	//case 'k':
    case 'b':
    case 'I':
    case '{':
    case '}':
    case '~':
	{
	    Term_key_push(ke.key.code);
	    textui_process_command(TRUE);
	    break;
	}

	/* Equipment list */
    case 'e':
	{
	    do_cmd_equip();
	    break;
	}

	/* Inventory list */
    case 'i':
	{
	    do_cmd_inven();
	    break;
	}

      /*** Various commands ***/

	/* Look at an object */
    case 'x':
    case 'l':
	{
	    store_inspect();
	    break;
	}

	/* Hack -- toggle windows */
    case KTRL('E'):
	{
	    toggle_inven_equip();
	    break;
	}



      /*** Help and Such ***/

	/* Help */
    case '?':
	{
	    do_cmd_help();
	    break;
	}

	/* Identify symbol */
    case '/':
	{
	    do_cmd_query_symbol();
	    break;
	}

	/* Character description */
    case 'C':
	{
	    do_cmd_change_name();
	    break;
	}


      /*** System Commands ***/

	/* Single line from a pref file */
    case '"':
	{
	    do_cmd_pref();
	    break;
	}

	/* Interact with options */
    case '=':
	{
	    do_cmd_options();
	    do_cmd_redraw();
	    display_store();
	    break;
	}


      /*** Misc Commands ***/

	/* Take notes */
    case ':':
	{
	    do_cmd_note();
	    break;
	}

	/* Version info */
    case 'V':
	{
	    do_cmd_version();
	    break;
	}

	/* Repeat level feeling */
    case KTRL('F'):
	{
	    do_cmd_feeling();
	    break;
	}

	/* Show previous message */
    case KTRL('O'):
	{
	    do_cmd_message_one();
	    break;
	}

	/* Show previous messages */
    case KTRL('P'):
	{
	    do_cmd_messages();
	    break;
	}

	/* Load "screen dump" */
    case '(':
	{
	    do_cmd_load_screen();
	    break;
	}

	/* Save "screen dump" */
    case ')':
	{
	    do_cmd_save_screen();
	    break;
	}


	/* Order an item */
    case 'o':
	{
	    if (st_ptr->type == STORE_MERCH)
		store_order();
	    else
		msg("You cannot order from this store.");
	    break;
	}

	/* Hack -- Unknown command */
    default:
	{
	    msg("That command does not work in stores.");
	    break;
	}
    }

    /* Let the game handle any core commands (equipping, etc) */
    process_command(CMD_STORE, TRUE);

    event_signal(EVENT_INVENTORY);
    event_signal(EVENT_EQUIPMENT);
    
    /* Notice and handle stuff */
    notice_stuff(p_ptr);
    handle_stuff(p_ptr);

    /* Display the store */
    display_inventory();
}


/**
 * Activate the existing store owner, or pick a new one 
 */
void get_owner(bool pick)
{
    int i, j, out_of = 100, chance = 0;
    byte type = st_ptr->type;

    /* Check for home */
    if (type == STORE_HOME)
	return;

    /* Choose an owner */
    if (pick) {
	byte *possible = malloc(z_info->p_max * sizeof(*possible));

	for (i = 0; i < z_info->p_max; i++)
	    possible[i] = 1;

	/* See who's available */
	for (i = 0; i < MAX_STORES; i++) {
	    if (type_of_store[i] == type) {
		for (j = 0; j < z_info->p_max; j++) {
		    if (owner[i] == j) {
			possible[j] = 0;
			out_of -= race_town_prob[town][j];
		    }
		}
	    }
	}

	/* Travelling merchant or no legal owner available */
	if ((type == STORE_MERCH) || (out_of == 0)) {
	    /* Choose a random owner */
	    i = randint0(z_info->p_max);

	    /* Try again if necessary */
	    while (possible[i] == 0)
		i = randint0(z_info->p_max);
	}

	/* Choose a legal race */
	else {
	    /* Get a random value */
	    j = randint0(out_of);

	    /* Look up the race probability */
	    for (i = 0; i < z_info->p_max; i++) {
		/* Add the probability for the legal races */
		chance += race_town_prob[town][i] * possible[i];

		/* Got the right one */
		if (chance > j)
		    break;
	    }
	}

	/* Set the owner */
	st_ptr->owner = i;

	free(possible);
    }

    /* Activate the owner */
    ot_ptr = &b_info[(type * z_info->b_max) + st_ptr->owner];

}

/**
 * Enter a store, and interact with it.
 *
 * Note that we use the standard "request_command()" function
 * to get a command, allowing us to use "p_ptr->command_arg" and all
 * command macros and other nifty stuff, but we use the special
 * "shopping" argument, to force certain commands to be converted
 * into other commands, normally, we convert "p" (pray) and "m"
 * (cast magic) into "g" (get), and "s" (search) into "d" (drop).
 */
void do_cmd_store(cmd_code code, cmd_arg args[])
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int i, which = 0;

    int tmp_chr;

    bool found = FALSE;
    bool old_curs = smlcurs;

    ui_event ke;

    feature_type *f_ptr = &f_info[cave_feat[py][px]];

    /* Verify a store */
    if (!tf_has(f_ptr->flags, TF_SHOP)) 
    {
	msg("You see no store here.");
	return;
    }

    /* Check if we're in a small town */
    for (i = 0; i < NUM_TOWNS_SMALL; i++) 
    {
	/* Found the town, adjust the index */
	if (towns[i] == p_ptr->stage) 
	{
	    /* Set the town */
	    town = i;

	    switch (f_ptr->shopnum) 
	    {
	    case STORE_TEMPLE:
		/* Temple */
	    {
		which += 1;
		break;
	    }
	    case STORE_ALCH:
		/* Alchemist */
	    {
		which += 2;
		break;
	    }
	    case STORE_HOME:
		/* Home */
	    {
		which += 3;
		break;
	    }
	    }

	    /* Done */
	    found = TRUE;
	    break;
	}

	else
	    /* Try the next town */
	    which += MAX_STORES_SMALL;
    }

    if (!found) 
    {
	/* Find the (large) town */
	for (i = NUM_TOWNS_SMALL; i < NUM_TOWNS; i++) 
	{
	    /* Found the town, adjust the index */
	    if (towns[i] == p_ptr->stage) 
	    {
		/* Set the town */
		town = i;

		/* Extract the store code */
		which += (f_ptr->shopnum);

		/* Got it */
		break;
	    } 
	    else
		/* Try the next town */
		which += MAX_STORES_BIG;
	}
    }

    /* Oops */
    if (which == MAX_STORES) 
    {
	if ((p_ptr->map == MAP_DUNGEON) || (p_ptr->map == MAP_FANILLA)) 
	{
	    if (f_ptr->shopnum == STORE_MERCH)
		which = 0;
	    else
		which = f_ptr->shopnum + 4 * NUM_TOWNS_SMALL;
	} else {
	    msg("You see no store here.");
	    return;
	}
    }

    /* Hack -- Check the "locked doors" */
    if (store[which].store_open >= turn) {
	msg("The doors are locked.");
	return;
    }

    /* Forget the view */
    forget_view();

    /* Hack -- Increase "icky" depth */
    character_icky++;

    /* Fix cursor */
    smlcurs = TRUE;

    /* No command argument */
    p_ptr->command_arg = 0;

    /* Save the store number */
    store_num = which;

    /* Save the store and owner pointers */
    st_ptr = &store[store_num];
    get_owner(FALSE);

    /* Start at the beginning */
    store_top = 0;

    /* Display the store */
    display_store();

    /* Do not leave */
    leave_store = FALSE;

    /* Buttons */
    button_add("g", 'g');
    button_add("d", 'd');
    button_add("i", 'i');
    button_add("l", 'l');
    if ((st_ptr->type == STORE_MERCH)
	&& (which < MAX_STORES_SMALL * NUM_TOWNS_SMALL))
	button_add("o", 'o');

    /* Interact with player */
    while (!leave_store) {
	/* Hack -- Clear line 1 */
	prt("", 1, 0);

	/* Hack -- Check the charisma */
	tmp_chr = p_ptr->state.stat_use[A_CHR];

	/* Clear */
	display_inventory();
	clear_from(20);

	/* Basic commands */
	/* Prompt */
	prt("You may: ", 20, 0);
	
	/* Commands */
	prt(" RET) Continue", 20, 27);
	prt("   i) Player inventory", 20, 55);
	prt(" ESC) Exit from Building.", 21, 0);
	
	/* Browse if necessary */
	if (st_ptr->stock_num > 12) {
	    prt(" SPACE) Next page of stock", 22, 0);
	}
	
	/* More commands */
	prt(" g) Get/Purchase an item.", 21, 29);
	prt(" d) Drop/Sell an item.", 22, 29);
	if (OPT(rogue_like_commands))
	    prt("   x) Look at an item.", 21, 55);
	else
	    prt("   l) Look at an item.", 21, 55);
	if ((st_ptr->type == STORE_MERCH)
	    && (which < MAX_STORES_SMALL * NUM_TOWNS_SMALL))
	    prt("   o) Order an item.", 21, 55);
    
	/* Get a command */
	ke = inkey_ex();
	
	/* Hack -- erase the message line. */
	prt("", 0, 0);
  
	/* Process the command */
	store_process_command(ke);

	/* Notice stuff */
	notice_stuff(p_ptr);

	/* Handle stuff */
	handle_stuff(p_ptr);

	/* Pack Overflow XXX XXX XXX */
	if (p_ptr->inventory[INVEN_PACK].k_idx) {
	    int item = INVEN_PACK;

	    object_type *o_ptr = &p_ptr->inventory[item];

	    /* Hack -- Flee from the store */
	    if (st_ptr->type != STORE_HOME) {
		/* Message */
		msg("Your pack is so full that you flee the store...");

		/* Leave */
		leave_store = TRUE;
	    }

	    /* Hack -- Flee from the home */
	    else if (!store_check_num(o_ptr)) {
		/* Message */
		msg("Your pack is so full that you flee your home...");

		/* Leave */
		leave_store = TRUE;
	    }

	    /* Hack -- Drop items into the home */
	    else {
		int item_pos;

		object_type *i_ptr;
		object_type object_type_body;

		char o_name[120];


		/* Give a message */
		msg("Your pack overflows!");

		/* Get local object */
		i_ptr = &object_type_body;

		/* Grab a copy of the object */
		object_copy(i_ptr, o_ptr);

		/* Describe it */
		object_desc(o_name, sizeof(o_name), i_ptr,
			    ODESC_PREFIX | ODESC_FULL);

		/* Message */
		msg("You drop %s (%c).", o_name, index_to_label(item));

		/* Remove it from the players inventory */
		inven_item_increase(item, -255);
		inven_item_describe(item);
		inven_item_optimize(item);

		/* Handle stuff */
		handle_stuff(p_ptr);

		/* Let the home carry it */
		item_pos = home_carry(i_ptr);

		/* Redraw the home */
		if (item_pos >= 0) {
		    /* Redisplay wares */
		    display_inventory();
		}
	    }
	}

	/* Hack -- Handle charisma changes */
	if (tmp_chr != p_ptr->state.stat_use[A_CHR]) {
	    /* Redisplay wares */
	    display_inventory();
	}

	/* Hack -- get kicked out of the store */
	if (st_ptr->store_open >= turn)
	    leave_store = TRUE;
    }


    /* Buttons */
    button_kill('g');
    button_kill('d');
    button_kill('i');
    button_kill('l');
    button_kill('o');

    /* Free turn */
    p_ptr->energy_use = 0;

    /* Hack -- Decrease "icky" depth */
    character_icky--;

    /* Restore cursor */
    smlcurs = old_curs;

    /* Clear the screen */
    Term_clear();


    /* Update the visuals */
    p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

    /* Redraw entire screen */
    p_ptr->redraw |= (PR_BASIC | PR_EXTRA);

    /* Redraw map */
    p_ptr->redraw |= (PR_MAP);
}

/**
 * Shuffle one of the stores.
 */
void store_shuffle(int which)
{
    int i;

    /* Save the store index */
    store_num = which;

    /* Activate that store */
    st_ptr = &store[store_num];

    /* Ignore home */
    if (st_ptr->type == STORE_HOME)
	return;

    /* Pick a new owner */
    get_owner(TRUE);

    /* Record the new owner */
    owner[store_num] = st_ptr->owner;

    /* Reset the owner data */
    st_ptr->insult_cur = 0;
    st_ptr->store_open = 0;
    st_ptr->good_buy = 0;
    st_ptr->bad_buy = 0;


    /* Hack -- discount all the items */
    for (i = 0; i < st_ptr->stock_num; i++) {
	object_type *o_ptr;

	/* Get the object */
	o_ptr = &st_ptr->stock[i];

	/* Sell all old items for "half price" */
	o_ptr->discount = 50;

	/* Inscribe the object as "on sale" */
	o_ptr->note = quark_add("on sale");
    }
}


/**
 * Maintain the inventory at the stores.
 */
void store_maint(int which)
{
    int j;
    int giveup = 0;
    int old_rating = rating;

    /* Save the store index */
    store_num = which;

    /* Activate that store */
    st_ptr = &store[store_num];

    /* Ignore home */
    if (st_ptr->type == STORE_HOME)
	return;

    /* Activate the owner */
    get_owner(FALSE);

    /* Store keeper forgives the player */
    st_ptr->insult_cur = 0;

    /* Mega-Hack -- prune the black market */
    if (st_ptr->type == STORE_BLACKM) {
	/* Destroy crappy black market items */
	for (j = st_ptr->stock_num - 1; j >= 0; j--) {
	    object_type *o_ptr = &st_ptr->stock[j];

	    /* Destroy crappy items */
	    if (black_market_crap(o_ptr)) {
		/* Destroy the object */
		store_item_increase(j, 0 - o_ptr->number);
		store_item_optimize(j);
	    }
	}
    }


    /* Choose the number of slots to keep */
    j = st_ptr->stock_num;

    /* Sell a few items */
    j = j - randint1(STORE_TURNOVER);

    /* Never keep more than "STORE_MAX_KEEP" slots */
    if (j > STORE_MAX_KEEP)
	j = STORE_MAX_KEEP;

    /* Always "keep" at least "STORE_MIN_KEEP" items */
    if (j < STORE_MIN_KEEP)
	j = STORE_MIN_KEEP;

    /* Hack -- prevent "underflow" */
    if (j < 0)
	j = 0;

    /* Destroy objects until only "j" slots are left */
    while (st_ptr->stock_num > j)
	store_delete();


    /* Choose the number of slots to fill */
    j = st_ptr->stock_num;

    /* Buy some more items */
    j = j + randint1(STORE_TURNOVER);

    /* Never keep more than "STORE_MAX_KEEP" slots */
    if (j > STORE_MAX_KEEP)
	j = STORE_MAX_KEEP;

    /* Always "keep" at least "STORE_MIN_KEEP" items */
    if (j < STORE_MIN_KEEP)
	j = STORE_MIN_KEEP;

    /* Hack -- prevent "overflow" */
    if (j >= st_ptr->stock_size)
	j = st_ptr->stock_size - 1;

    /* Acquire some new items */
    while (st_ptr->stock_num < j)
    {
	store_create();
	giveup++;
	if (giveup > 100) break;
    }


    /* Hack -- Restore the rating */
    rating = old_rating;
}

/**
 * Maintain all the stores
 */

void stores_maint(int times)
{
    int i, t, m = 0, n, max_stores, home, base;
    bool big = FALSE;

    /* Message */
    if (OPT(cheat_xtra))
	msg("Updating Shops...");

    /* Do each town in turn */
    for (t = 0; t < NUM_TOWNS; t++) {
	/* Set the town */
	town = t;

	/* Get the town size, home index */
	if (t < NUM_TOWNS_SMALL) {
	    max_stores = MAX_STORES_SMALL;
	    home = max_stores - 1;
	} else {
	    max_stores = MAX_STORES_BIG;
	    big = TRUE;
	    home = max_stores - 2;
	}

	/* Hack - record the first store */
	base = m;

	/* Maintain each shop (except home) - do Black Market last. */
	for (n = 0; n < max_stores; n++, m++) {
	    for (i = 0; i < times; i++) {
		/* Ignore home */
		if (n == home)
		    continue;

		/* Save black market for last */
		if (big && (n == (STORE_BLACKM)))
		    continue;

		/* Maintain */
		store_maint(m);
	    }
	}

	/* Now the Black Market */
	for (i = 0; i < times; i++)
	    if (big)
		store_maint(base + STORE_BLACKM);


	/* Sometimes, shuffle the shop-keepers */
	if (randint0(STORE_SHUFFLE) == 0) {
	    /* Message */
	    if (OPT(cheat_xtra))
		msg("Shuffling a Shopkeeper...");

	    /* pick a store randomly. */
	    n = randint0(max_stores);

	    /* Shuffle the store, if not the home. */
	    if (n != home)
		store_shuffle(base + n);
	}

	/* Message */
	if (OPT(cheat_xtra))
	    msg("Done.");
    }
}


/**
 * Initialize the stores
 */
void store_init(void)
{
    int k, n;

    /* Start at the first town */
    town = -1;

    /* Each store in turn */
    for (n = 0; n < MAX_STORES; n++) {
	/* Activate that store */
	st_ptr = &store[n];

	/* Find the town */
	if (n < NUM_TOWNS_SMALL * MAX_STORES_SMALL) {
	    if (!(n % MAX_STORES_SMALL))
		town++;
	} else {
	    if (!((n - NUM_TOWNS_SMALL * MAX_STORES_SMALL) % MAX_STORES_BIG))
		town++;
	}

	/* Pick an owner */
	get_owner(TRUE);

	/* Record the owner */
	owner[n] = st_ptr->owner;

	/* Initialize the store */
	st_ptr->store_open = 0;
	st_ptr->insult_cur = 0;
	st_ptr->good_buy = 0;
	st_ptr->bad_buy = 0;

	/* Nothing in stock */
	st_ptr->stock_num = 0;

	/* Clear any old items */
	for (k = 0; k < st_ptr->stock_size; k++) {
	    object_wipe(&st_ptr->stock[k]);
	}

	/* Clear any old merchant orders */
	if (st_ptr->type == STORE_MERCH)
	    for (k = 24; k < STORE_CHOICES; k++) {
		st_ptr->table[k] = 0;
	    }
    }

    /* Maintain all stores 10 times */
    stores_maint(10);
}
