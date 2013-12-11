/** \file  obj-make.c 
    \brief Object generation 

 * Object stacks, compaction, generation, IDing, "trying", pricing, 
 * stacking.  Creating special artifacts, Adding charges to wands and 
 * staffs, bonuses to weapons and armour, powers and higher pvals to 
 * amulets and rings, fuel to lights, trap difficulty to chests, creating
 * ego items, what consititutes "cursed", "good", and "great" items,
 * generate objects (inc. Acquirement code) and treasures, object &
 * inventory routines, inventory sorting, equipment.
 *
 * Copyright (c) 2009 Nick McConnell, Si Griffin, Leon Marrick & Bahman Rabii, 
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
#include "squelch.h"
#include "types.h"


/**
 * Apply a "object restriction function" to the "object allocation table"
 */
errr get_obj_num_prep(void)
{
	int i;

	/* Get the entry */
	alloc_entry *table = alloc_kind_table;

	/* Scan the allocation table */
	for (i = 0; i < alloc_kind_size; i++) {
		/* Accept objects which pass the restriction, if any */
		if (!get_obj_num_hook || (*get_obj_num_hook) (table[i].index)) {
			/* Accept this object */
			table[i].prob2 = table[i].prob1;
		}

		/* Do not use this object */
		else {
			/* Decline this object */
			table[i].prob2 = 0;
		}
	}

	/* Success */
	return (0);
}



/**
 * Choose an object kind that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "object allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" object, in
 * a relatively efficient manner.
 *
 * It is (slightly) more likely to acquire an object of the given level
 * than one of a lower level.  This is done by choosing several objects
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no objects are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_obj_num(int level)
{
	int i, j, p;

	int k_idx;

	long value, total;

	object_kind *k_ptr;

	alloc_entry *table = alloc_kind_table;


	/* Boost level */
	if (level > 0) {
		/* Occasional boost to allowed level.  Less generous in Oangband. */
		if (randint0(GREAT_OBJ) == 0)
			level += randint1(20) + randint1(level / 2);
	}


	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_kind_size; i++) {
		/* Objects are sorted by depth */
		if (table[i].level > level)
			break;

		/* Default */
		table[i].prob3 = 0;

		/* Access the index */
		k_idx = table[i].index;

		/* Access the actual kind */
		k_ptr = &k_info[k_idx];

		/* Hack -- prevent embedded chests */
		if (opening_chest && (k_ptr->tval == TV_CHEST))
			continue;

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Total */
		total += table[i].prob3;
	}

	/* No legal objects */
	if (total <= 0)
		return (0);


	/* Pick an object */
	value = randint0(total);

	/* Find the object */
	for (i = 0; i < alloc_kind_size; i++) {
		/* Found the entry */
		if (value < table[i].prob3)
			break;

		/* Decrement */
		value = value - table[i].prob3;
	}


	/* Power boost */
	p = randint0(100);

	/* Hack -- chests should have decent stuff, and chests should themselves be 
	 * decent, so we guarantee two more tries for better objects. -LM- */
	if ((opening_chest) || (required_tval == TV_CHEST))
		p = 0;

	/* Try for a "better" object once (60%) or twice (10%) */
	if (p < 60) {
		/* Save old */
		j = i;

		/* Pick an object */
		value = randint0(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++) {
			/* Found the entry */
			if (value < table[i].prob3)
				break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level)
			i = j;
	}

	/* Try for a "better" object twice (10%) */
	if (p < 10) {
		/* Save old */
		j = i;

		/* Pick an object */
		value = randint0(total);

		/* Find the object */
		for (i = 0; i < alloc_kind_size; i++) {
			/* Found the entry */
			if (value < table[i].prob3)
				break;

			/* Decrement */
			value = value - table[i].prob3;
		}

		/* Keep the "best" one */
		if (table[i].level < table[j].level)
			i = j;
	}

	/* Hack - keep level for jewellery generation */
	j_level = table[i].level;

	/* Result */
	return (table[i].index);
}


/**
 * Prepare an object based on an object kind.
 */
void object_prep(object_type * o_ptr, int k_idx, aspect rand_aspect)
{
	int i;

	object_kind *k_ptr = &k_info[k_idx];

	/* Clear the record */
	(void) WIPE(o_ptr, object_type);

	/* Save the kind  */
	o_ptr->k_idx = k_idx;
	o_ptr->kind = k_ptr;

	/* Efficiency -- tval/sval */
	o_ptr->tval = k_ptr->tval;
	o_ptr->sval = k_ptr->sval;

	/* Default "pval" */
	o_ptr->pval = randcalc(k_ptr->pval, k_ptr->level, rand_aspect);

	/* Default number */
	o_ptr->number = 1;

	/* Default weight */
	o_ptr->weight = k_ptr->weight;

	/* Default magic */
	o_ptr->to_h = randcalc(k_ptr->to_h, k_ptr->level, rand_aspect);
	o_ptr->to_d = randcalc(k_ptr->to_d, k_ptr->level, rand_aspect);
	o_ptr->to_a = randcalc(k_ptr->to_a, k_ptr->level, rand_aspect);

	/* Default power */
	o_ptr->ac = k_ptr->ac;
	o_ptr->dd = k_ptr->dd;
	o_ptr->ds = k_ptr->ds;

	/* Default effect and time */
	o_ptr->effect = k_ptr->effect;
	o_ptr->time = k_ptr->time;

	/* Default flags */
	of_copy(o_ptr->flags_obj, k_ptr->flags_obj);
	cf_copy(o_ptr->flags_curse, k_ptr->flags_curse);

	/* Default resists, bonuses, multiples */
	for (i = 0; i < MAX_P_RES; i++)
		o_ptr->percent_res[i] = k_ptr->percent_res[i];
	for (i = 0; i < A_MAX; i++)
		o_ptr->bonus_stat[i] = k_ptr->bonus_stat[i];
	for (i = 0; i < MAX_P_BONUS; i++)
		o_ptr->bonus_other[i] = k_ptr->bonus_other[i];
	for (i = 0; i < MAX_P_SLAY; i++)
		o_ptr->multiple_slay[i] = k_ptr->multiple_slay[i];
	for (i = 0; i < MAX_P_BRAND; i++)
		o_ptr->multiple_brand[i] = k_ptr->multiple_brand[i];
}

/**
 * Cheat -- describe a created object for the user
 */
static void object_mention(object_type * o_ptr)
{
	char o_name[120];

	/* Describe */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_BASE | ODESC_SPOIL);

	/* Artifact */
	if (artifact_p(o_ptr)) {
		/* Silly message */
		msg("Artifact (%s)", o_name);
	}

	/* Ego-item */
	else if (ego_item_p(o_ptr)) {
		/* Silly message */
		msg("Ego-item (%s)", o_name);
	}

	/* Normal item */
	else {
		/* Silly message */
		msg("Object (%s)", o_name);
	}
}

/**
 * Test an ego item for any negative qualities
 */
bool good_ego(const ego_item_type * e_ptr)
{
	int i, to_h, to_d;

	/* Resists, bonuses, multiples */
	for (i = 0; i < MAX_P_RES; i++)
		if (e_ptr->percent_res[i] > RES_LEVEL_BASE)
			return FALSE;
	for (i = 0; i < A_MAX; i++)
		if (e_ptr->bonus_stat[i] < BONUS_BASE)
			return FALSE;
	for (i = 0; i < MAX_P_BONUS; i++)
		if (e_ptr->bonus_other[i] < BONUS_BASE)
			return FALSE;
	for (i = 0; i < MAX_P_SLAY; i++)
		if (e_ptr->multiple_slay[i] < MULTIPLE_BASE)
			return FALSE;
	for (i = 0; i < MAX_P_BRAND; i++)
		if (e_ptr->multiple_brand[i] < MULTIPLE_BASE)
			return FALSE;

	/* To skill, to deadliness, to AC */
	if ((e_ptr->max_to_h > 0) && (e_ptr->max_to_h < 129))
		to_h = e_ptr->max_to_h;
	else
		to_h = 128 - e_ptr->max_to_h;
	if ((e_ptr->max_to_d > 0) && (e_ptr->max_to_d < 129))
		to_d = e_ptr->max_to_d;
	else
		to_d = 128 - e_ptr->max_to_d;
	if (to_h + to_d < 0)
		return FALSE;
	if (e_ptr->max_to_a > 128)
		return FALSE;

	/* Curses */
	if (!cf_is_empty(e_ptr->flags_curse) ||
		kf_has(e_ptr->flags_kind, KF_RAND_CURSE))
		return FALSE;


	/* Must be OK */
	return TRUE;
}

/**
 * Attempt to change an object into an ego-item -MWK-
 * Better only called by apply_magic().
 * The return value says if we picked a cursed item (if allowed) and is
 * passed on to a_m_aux1/2().
 * If no legal ego item is found, this routine returns 0, resulting in
 * an unenchanted item.
 *
 * Modified for Oangband to specifically create cursed/non-cursed egos.
 * This is to keep the probabilities separate between cursed and
 * non-cursed ego types.
 */
static int make_ego_item(object_type * o_ptr, int power)
{
	int i, j, level;

	int e_idx;

	long value, total;

	ego_item_type *e_ptr;

	alloc_entry *table = alloc_ego_table;


	/* Fail if object already is ego or artifact */
	if (o_ptr->name1)
		return (FALSE);
	if (o_ptr->name2)
		return (FALSE);

	level = object_level;

	/* Boost level (like with object base types) */
	if (level > 0) {
		/* Occasional "boost" */
		if (randint0(GREAT_EGO) == 0) {
			/* The bizarre calculation again */
			level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
		}
	}

	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_ego_size; i++) {
		/* Default */
		table[i].prob3 = 0;

		/* Objects are sorted by depth */
		if (table[i].level > level)
			continue;

		/* Get the index */
		e_idx = table[i].index;

		/* Get the actual kind */
		e_ptr = &e_info[e_idx];

		/* If we force good/great, don't create cursed */
		if ((power > 0) && (!good_ego(e_ptr)))
			continue;

		/* If we force cursed, don't create good */
		if ((power < 0) && good_ego(e_ptr))
			continue;

		/* Test if this is a legal ego-item type for this object */
		for (j = 0; j < EGO_TVALS_MAX; j++) {
			/* Require identical base type */
			if (o_ptr->tval == e_ptr->tval[j]) {
				/* Require sval in bounds, lower */
				if (o_ptr->sval >= e_ptr->min_sval[j]) {
					/* Require sval in bounds, upper */
					if (o_ptr->sval <= e_ptr->max_sval[j]) {
						/* Accept */
						table[i].prob3 = table[i].prob2;
					}
				}
			}
		}

		/* Total */
		total += table[i].prob3;
	}

	/* No legal ego-items -- create a normal unenchanted one */
	if (total == 0)
		return (0);


	/* Pick an ego-item */
	value = randint0(total);

	/* Find the object */
	for (i = 0; i < alloc_ego_size; i++) {
		/* Found the entry */
		if (value < table[i].prob3)
			break;

		/* Decrement */
		value = value - table[i].prob3;
	}

	/* We have one */
	e_idx = (byte) table[i].index;
	o_ptr->name2 = e_idx;

	return ((!cf_is_empty(e_info[e_idx].flags_curse)
			 || kf_has(e_info[e_idx].flags_kind, KF_RAND_CURSE)) ? -2 : 2);
}

/**
 * Mega-Hack -- Attempt to create one of the "Special Objects".
 *
 * This function now does not run down the list sequentially, which 
 * favored some artifacts at the expense of others, but starts at 
 * a random index, and then wraps around.
 *
 * We are only called from "make_object()", and we assume that
 * "apply_magic()" is called immediately after we return.
 *
 * Note -- see "make_artifact()" and "apply_magic()"
 */
static bool make_artifact_special(object_type * o_ptr)
{
	int i, first_pick;

	int k_idx = 0;

	/* No artifacts in the town */
	if (!p_ptr->depth || MODE(NO_ARTIFACTS))
		return (FALSE);

	first_pick = randint0(artifact_special_cnt);

	for (i = 0; i < artifact_special_cnt; i++) {
		int j = (first_pick + i) % artifact_special_cnt;
		int choice = artifact_special[j];
		artifact_type *a_ptr = &a_info[choice];

		/* Skip "empty" artifacts */
		if (!a_ptr->name)
			continue;

		/* Cannot make an artifact twice */
		if (a_ptr->created)
			continue;

		/* Enforce minimum "depth" (loosely) */
		if (a_ptr->level > p_ptr->depth) {
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - p_ptr->depth) * 2;

			/* Roll for out-of-depth creation */
			if (randint0(d) != 0)
				continue;
		}

		/* Artifact "rarity roll" */
		if (randint0(a_ptr->rarity) != 0)
			continue;

		/* Find the base object */
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Enforce minimum "object" level (loosely) */
		if (k_info[k_idx].level > object_level) {
			/* Acquire the "out-of-depth factor" */
			int d = (k_info[k_idx].level - object_level) * 5;

			/* Roll for out-of-depth creation */
			if (randint0(d) != 0)
				continue;
		}

		/* Assign the template */
		object_prep(o_ptr, k_idx, RANDOMISE);

		/* Mega-Hack -- mark the item as an artifact */
		o_ptr->name1 = choice;

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/**
 * Attempt to change an object into an artifact
 *
 * This routine should only be called by "apply_magic()"
 *
 * Note -- see "make_artifact_special()" and "apply_magic()"
 */
static bool make_artifact(object_type * o_ptr)
{
	int i;


	/* No artifacts in the town */
	if (!p_ptr->depth)
		return (FALSE);

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1)
		return (FALSE);

	/* Check the artifact list (skip the "specials") */
	for (i = 0; i < artifact_normal_cnt; i++) {
		int choice = artifact_normal[i];
		artifact_type *a_ptr = &a_info[choice];

		/* Skip "empty" items */
		if (!a_ptr->name)
			continue;

		/* Cannot make an artifact twice */
		if (a_ptr->created)
			continue;

		/* Must have the correct fields */
		if (a_ptr->tval != o_ptr->tval)
			continue;
		if (a_ptr->sval != o_ptr->sval)
			continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > p_ptr->depth) {
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - p_ptr->depth) * 2;

			/* Roll for out-of-depth creation */
			if (randint0(d) != 0)
				continue;
		}

		/* We must make the "rarity roll" */
		if (randint0(a_ptr->rarity) != 0)
			continue;

		/* Hack -- mark the item as an artifact */
		o_ptr->name1 = choice;

		/* Success */
		return (TRUE);
	}

	/* Failure */
	return (FALSE);
}


/**
 * Apply magic to an item known to be a "weapon"
 *
 * Hack -- note special base damage dice boosting
 * Hack -- note special processing for weapon/digger
 * Hack -- note special rating boost for dragon scale mail
 */
static void a_m_aux_1(object_type * o_ptr, int level, int power)
{
	int tohit1 = randint1(5) + m_bonus(5, level);
	int todam1 = randint1(5) + m_bonus(5, level);

	int tohit2 = m_bonus(10, level);
	int todam2 = m_bonus(10, level);

	/* Good */
	if (power > 0) {
		/* Enchant */
		o_ptr->to_h += tohit1;
		o_ptr->to_d += todam1;

		/* Very good */
		if (power > 1) {
			/* Enchant again */
			o_ptr->to_h += tohit2;
			o_ptr->to_d += todam2;
		}
	}

	/* Bad */
	else if (power < 0) {
		/* Penalize */
		o_ptr->to_h -= tohit1;
		o_ptr->to_d -= todam1;

		/* Perilous - more random now */
		if (power < -1) {
			/* Shake it up */
			if (randint0(3) == 0)
				o_ptr->to_h = 0 - o_ptr->to_h;
			if (randint0(3) == 0)
				o_ptr->to_d = 0 - o_ptr->to_d;
			if (randint0(2) == 0)
				o_ptr->to_h -= tohit2;
			else
				o_ptr->to_h += tohit2;
			if (randint0(2) == 0)
				o_ptr->to_d -= todam2;
			else
				o_ptr->to_d += todam2;
		}
	}


	/* Analyze type */
	switch (o_ptr->tval) {
	case TV_DIGGING:
		{
			/* Very bad */
			if (power < -1) {
				/* Hack -- Horrible digging bonus */
				o_ptr->bonus_other[P_BONUS_TUNNEL] = 0 - (5 + randint1(5));
			}

			/* Bad */
			else if (power < 0) {
				/* Hack -- Reverse digging bonus */
				o_ptr->bonus_other[P_BONUS_TUNNEL] =
					0 - (o_ptr->bonus_other[P_BONUS_TUNNEL]);
			}

			break;
		}


	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
		{
			/* Very bad */
			if (power < -1) {
				if (o_ptr->to_d < 0)
					o_ptr->to_d *= 2;
			}

			break;
		}

	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
		{
			/* First, a little hack to keep certain values sane. */
			/* If at least good, */
			if (power > 0) {
				/* chop the bonuses in half, rounded up. */
				o_ptr->to_h -= o_ptr->to_h / 2;
				o_ptr->to_d -= o_ptr->to_d / 2;

			}
		}
	}
}


/**
 * Apply magic to an item known to be "armor"
 *
 * Hack -- note special processing for crown/helm
 * Hack -- note special processing for robe of permanence
 */
static void a_m_aux_2(object_type * o_ptr, int level, int power)
{
	int toac1, toac2;

	/* Boots and handgear aren't as important to total AC. */
	if ((o_ptr->tval == TV_BOOTS) || (o_ptr->tval == TV_GLOVES)) {
		toac1 = randint1(3) + m_bonus(4, level);
		toac2 = m_bonus(8, level);
	} else {
		toac1 = randint1(5) + m_bonus(5, level);
		toac2 = m_bonus(10, level);
	}


	/* Good */
	if (power > 0) {
		/* Enchant */
		o_ptr->to_a += toac1;

		/* Very good */
		if (power > 1) {
			/* Enchant again */
			o_ptr->to_a += toac2;
		}
	}

	/* Bad */
	else if (power < 0) {
		/* Penalize */
		o_ptr->to_a -= toac1;

		/* Very bad */
		if (power < -1) {
			/* Penalize again */
			o_ptr->to_a -= toac2;
		}
	}


	/* Analyze type */
	switch (o_ptr->tval) {
	case TV_DRAG_ARMOR:
		{
			/* Rating boost */
			rating += 20;

			/* Mention the item */
			if (OPT(cheat_peek))
				object_mention(o_ptr);

			break;
		}


	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_SHIELD:
	case TV_GLOVES:
	case TV_BOOTS:
	case TV_CROWN:
	case TV_HELM:
	case TV_CLOAK:
		{
			break;
		}
	}
}




/**
 * Apply magic to miscellanious items
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type * o_ptr, int level, int power)
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Apply magic (good or bad) according to type */
	switch (o_ptr->tval) {
	case TV_LIGHT:
		{
			/* Hack -- Torches -- random fuel */
			if (o_ptr->sval == SV_LIGHT_TORCH) {
				if (o_ptr->pval > 0)
					o_ptr->pval = randint1(o_ptr->pval);
			}

			/* Hack -- Lanterns -- random fuel */
			if (o_ptr->sval == SV_LIGHT_LANTERN) {
				if (o_ptr->pval > 0)
					o_ptr->pval = randint1(o_ptr->pval);
			}

			break;
		}

	case TV_WAND:
	case TV_STAFF:
		{
			int temp_pval = randcalc(k_ptr->pval, level, RANDOMISE);
			/* The wand or staff gets a number of initial charges equal to
			 * between 1/2 (+1) and the full object kind's pval. */
			o_ptr->pval = temp_pval / 2 + randint1((temp_pval + 1) / 2);
			break;
		}

	case TV_ROD:
		{
			/* Transfer the pval. */
			o_ptr->pval = randcalc(k_ptr->pval, level, RANDOMISE);
			break;
		}

	case TV_CHEST:				/* changed by LM */
		{
			/* Hack -- pick a "value/difficulty", capable of being as little as 
			 * chest level - 16... */
			o_ptr->pval =
				(k_info[o_ptr->k_idx].level) + 20 - damroll(4, 9);

			/* ...and never exceeding chest level + 4. */
			if (o_ptr->pval > k_info[o_ptr->k_idx].level)
				o_ptr->pval = k_info[o_ptr->k_idx].level + randint1(4);

			/* Value/difficulty cannot be less than 5. */
			if (o_ptr->pval < 5)
				o_ptr->pval = 5;

			/* Never exceed "value/difficulty" of 99. */
			if (o_ptr->pval > 99)
				o_ptr->pval = 99;

			break;
		}
	}
}

/**
 * Apply slightly randomised percentage resistances -NRM-
 * Also add element proofing flags where appropriate
 */
void apply_resistances(object_type * o_ptr, int lev, bitflag * flags)
{
	int i, res = 0;

	/* Get the ego type resists */
	if (o_ptr->name2) {
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		for (i = 0; i < MAX_P_RES; i++) {
			/* Get the resistance value */
			if (e_ptr->percent_res[i] != RES_LEVEL_BASE) {
				res = o_ptr->percent_res[i] * e_ptr->percent_res[i] / 100;
				o_ptr->percent_res[i] = res;
			}

		}
	}

	/* Add extra resists */

	/* Vulnerability */
	if (kf_has(flags, KF_RAND_RES_NEG)) {
		int roll = 0;
		/* This gets done more often as we go deeper */
		while (roll < 2) {
			/* Low resists more likely at low levels */
			int k = randint0(128);
			bool low = (k < (128 - lev));

			k = (low ? randint0(4) : randint0(10) + 4);
			o_ptr->percent_res[k] += 10 + randint0(5) + m_bonus(30, lev);

			roll = randint0(10 - (lev / 30));
		}
	}

	/* Rings of Protection, etc */
	if (kf_has(flags, KF_RAND_RES_SML)) {
		int roll = 0;
		/* This gets done more often as we go deeper */
		while (roll < 2) {
			/* Low resists more likely at low levels */
			int k = randint0(128);
			bool low = (k < (128 - lev));

			k = (low ? randint0(4) : randint0(10) + 4);
			o_ptr->percent_res[k] -= 4 + randint0(8) + m_bonus(10, lev);

			/* Twice as many of these */
			k = randint0(128);
			low = (k < (128 - lev));
			k = (low ? randint0(4) : randint0(10) + 4);
			o_ptr->percent_res[k] -= 4 + randint0(8) + m_bonus(10, lev);

			/* Occasionally reverse one */
			if (randint0(3) == 0)
				o_ptr->percent_res[k] =
					RES_LEVEL_MAX - o_ptr->percent_res[k];

			roll = randint0(10 - (lev / 30));
		}
	}

	/* Armour of Resistance etc */
	if (kf_has(flags, KF_RAND_RES)) {
		int roll = 0;
		/* This gets done more often as we go deeper */
		while (roll < 2) {
			/* Low resists more likely at low levels */
			int k = randint0(128);
			bool low = (k < (128 - lev));

			k = (low ? randint0(4) : randint0(10) + 4);
			o_ptr->percent_res[k] -= 20 + randint0(5) + m_bonus(20, lev);

			roll = randint0(10 - (lev / 30));
		}
	}

	/* Former one high resist objects */
	if (kf_has(flags, KF_RAND_RES_XTRA)) {
		int roll = 0;
		/* This gets done more often as we go deeper */
		while (roll < 2) {
			/* Low resists less likely here */
			int k = ((randint0(10) == 0) ? randint0(4) : randint0(10) + 4);
			o_ptr->percent_res[k] -= 30 + randint0(5) + m_bonus(20, lev);

			roll = randint0(10 - (lev / 30));
		}
	}


	/* Randomise a bit */
	for (i = 0; i < MAX_P_RES; i++) {
		res = 100 - o_ptr->percent_res[i];

		/* Only randomise proper resistances */
		if ((res > 0) && (res < 100)) {
			if (randint0(2) == 0)
				o_ptr->percent_res[i] -= randint0(res >> 2);
			else
				o_ptr->percent_res[i] += randint0(res >> 2);
		}

		/* Enforce bounds - no item gets better than 80% resistance */
		if (o_ptr->percent_res[i] < RES_CAP_ITEM)
			o_ptr->percent_res[i] = RES_CAP_ITEM;
		if (o_ptr->percent_res[i] > RES_LEVEL_MAX)
			o_ptr->percent_res[i] = RES_LEVEL_MAX;
	}

	/* Low resist means object is proof against that element */
	if (o_ptr->percent_res[P_RES_ACID] < RES_LEVEL_BASE)
		of_on(o_ptr->flags_obj, OF_ACID_PROOF);
	if (o_ptr->percent_res[P_RES_ELEC] < RES_LEVEL_BASE)
		of_on(o_ptr->flags_obj, OF_ELEC_PROOF);
	if (o_ptr->percent_res[P_RES_FIRE] < RES_LEVEL_BASE)
		of_on(o_ptr->flags_obj, OF_FIRE_PROOF);
	if (o_ptr->percent_res[P_RES_COLD] < RES_LEVEL_BASE)
		of_on(o_ptr->flags_obj, OF_COLD_PROOF);
}

/**
 * Complete the "creation" of an object by applying "magic" to the item
 *
 * This includes not only rolling for random bonuses, but also putting the
 * finishing touches on ego-items and artifacts, giving charges to wands and
 * staffs, giving fuel to lights, and placing traps on chests.
 *
 * In particular, note that "Instant Artifacts", if "created" by an external
 * routine, must pass through this function to complete the actual creation.
 *
 * The base "chance" of the item being "good" increases with the "level"
 * parameter, which is usually derived from the dungeon level, being equal
 * to the level plus 10, up to a maximum of 75.   If "good" is true, then
 * the object is guaranteed to be "good".  If an object is "good", then
 * the chance that the object will be "great" (ego-item or artifact), also
 * increases with the "level", being equal to half the level, plus 5, up to
 * a maximum of 20.  If "great" is true, then the object is guaranteed to be
 * "great".  At dungeon level 65 and below, 15/100 objects are "great".
 *
 * If the object is not "good", there is a chance it will be "cursed", and
 * if it is "cursed", there is a chance it will be "broken".  These chances
 * are related to the "good" / "great" chances above.
 *
 * Otherwise "normal" rings and amulets will be "good" half the time and
 * "cursed" half the time, unless the ring/amulet is always good or cursed.
 *
 * If "okay" is true, and the object is going to be "great", then there is
 * a chance that an artifact will be created.  This is true even if both the
 * "good" and "great" arguments are false.  As a total hack, if "great" is
 * true, then the item gets 3 extra "attempts" to become an artifact.
 *
 */
void apply_magic(object_type * o_ptr, int lev, bool okay, bool good,
				 bool great)
{
	int i, rolls, good_percent, great_percent, power;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	bitflag flags_kind[KF_SIZE];

	kf_copy(flags_kind, k_ptr->flags_kind);

	/* Maximum "level" for various things */
	if (lev > MAX_DEPTH - 1)
		lev = MAX_DEPTH - 1;

	/* Base chance of being "good".  Reduced in Oangband. */
	good_percent = 4 * lev / 5 + 10;

	/* Maximal chance of being "good".  Reduced in Oangband. */
	if (good_percent > 65)
		good_percent = 65;

	/* Base chance of being "great".  Increased in Oangband. */
	great_percent = (good_percent / 2) + 5;

	/* Maximal chance of being "great".  Increased in Oangband. */
	if (great_percent > 35)
		great_percent = 35;


	/* Assume normal */
	power = 0;

	/* Roll for "good" */
	if (good || (randint0(100) < good_percent)) {
		/* Assume "good" */
		power = 1;

		/* Roll for "great" */
		if (great || (randint0(100) < great_percent))
			power = 2;
	}

	/* Roll for "dubious" */
	else if ((randint0(100) < good_percent) && (required_tval == 0)) {
		/* Assume "dubious" */
		power = -1;

		/* Roll for "perilous" */
		if (randint0(100) < great_percent)
			power = -2;
	}

	/* Assume no rolls */
	rolls = 0;

	/* Get one roll if excellent */
	if (power >= 2)
		rolls = 1;

	/* Hack -- Get four rolls if forced great */
	if (great)
		rolls = 4;

	/* Hack -- Get no rolls if not allowed */
	if (!okay || o_ptr->name1)
		rolls = 0;

	/* Roll for artifacts if allowed */
	for (i = 0; i < rolls; i++) {
		/* Roll for an artifact */
		if (make_artifact(o_ptr))
			break;
	}


	/* Hack -- analyze artifacts and transfer the activation index. */
	if (o_ptr->name1) {
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		/* Hack -- Mark the artifact as "created" */
		a_ptr->created = TRUE;

		/* Extract the other fields */
		o_ptr->pval = a_ptr->pval;
		o_ptr->ac = a_ptr->ac;
		o_ptr->dd = a_ptr->dd;
		o_ptr->ds = a_ptr->ds;
		o_ptr->to_a = a_ptr->to_a;
		o_ptr->to_h = a_ptr->to_h;
		o_ptr->to_d = a_ptr->to_d;
		o_ptr->weight = a_ptr->weight;

		for (i = 0; i < MAX_P_RES; i++)
			o_ptr->percent_res[i] = a_ptr->percent_res[i];
		for (i = 0; i < A_MAX; i++)
			o_ptr->bonus_stat[i] = a_ptr->bonus_stat[i];
		for (i = 0; i < MAX_P_BONUS; i++)
			o_ptr->bonus_other[i] = a_ptr->bonus_other[i];
		for (i = 0; i < MAX_P_SLAY; i++)
			o_ptr->multiple_slay[i] = a_ptr->multiple_slay[i];
		for (i = 0; i < MAX_P_BRAND; i++)
			o_ptr->multiple_brand[i] = a_ptr->multiple_brand[i];

		of_copy(o_ptr->flags_obj, a_ptr->flags_obj);
		cf_copy(o_ptr->flags_curse, a_ptr->flags_curse);

		/* Transfer the activation information. */
		if (a_ptr->effect)
			o_ptr->effect = a_ptr->effect;
		o_ptr->time = a_ptr->time;

		/* Mega-Hack -- increase the rating */
		rating += 15;

		/* Mega-Hack -- increase the rating again */
		rating += a_ptr->cost / 2000;

		/* Cheat -- peek at the item */
		if (OPT(cheat_peek))
			object_mention(o_ptr);

		/* Done */
		return;
	}


	switch (o_ptr->tval) {
	  /**** 	  damage dice of Melee Weapons. -LM- ****/
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
		{
			/* All melee weapons get a chance to improve their base damage
			 * dice.  Note the maximum value for dd*ds of 40. */
			int newdicesides = 0;
			int oldsides = o_ptr->ds;

			if (randint0
				((2 * (o_ptr->dd + 1) * (o_ptr->ds + 1)) /
				 ((lev / 20) + 1)) == 0) {
				if ((randint0((o_ptr->dd + 2) * (o_ptr->ds + 2) / 5) == 0)
					&& (((o_ptr->dd + 1) * o_ptr->ds) < 41)) {
					o_ptr->weight =
						(o_ptr->weight * (o_ptr->dd + 3)) / (o_ptr->dd +
															 2);
					o_ptr->dd += 1;
				} else {
					newdicesides = o_ptr->ds / 2;

					for (i = 0; i < newdicesides; i++) {
						if ((o_ptr->dd * (o_ptr->ds + 1) < 41)
							&& (randint1(5) != 1))
							o_ptr->ds++;
					}

					if (randint0((o_ptr->dd + 2) * (o_ptr->ds + 2) / 5) ==
						0) {
						/* If we get this far have another go at the dice
						 * missed earlier */
						newdicesides = (11 * oldsides / 6) - o_ptr->ds;

						for (i = 0; i < newdicesides; i++) {
							if ((o_ptr->dd * (o_ptr->ds + 1) < 41)
								&& (randint1(5) != 1))
								o_ptr->ds++;
						}
					}

					if (randint1(5) == 1) {
						o_ptr->weight = (o_ptr->weight * 4) / 5;
					} else if (randint1(3) == 1) {
						o_ptr->weight = (o_ptr->weight * 5) / 4;
					}
				}
			}
			break;
		}

	  /**** Enhance damage dice of missiles. -LM- ****/
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		{
			/* Up to two chances to enhance damage dice. SJGU was 3 and +2 to
			 * ds */
			if (randint1(6) == 1) {
				o_ptr->ds += 1;
				if (randint1(10) == 1) {
					o_ptr->ds += 1;
				}
			}
			break;
		}
	}
  /**** Apply Magic ****/

	switch (o_ptr->tval) {
	case TV_DIGGING:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOW:
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		{
			if ((power > 1) || (power < -1)) {
				int ego_power;

				ego_power = make_ego_item(o_ptr, power);

				if (ego_power)
					power = ego_power;
			}

			if (power)
				a_m_aux_1(o_ptr, lev, power);
			break;
		}

	case TV_DRAG_ARMOR:
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_SHIELD:
	case TV_HELM:
	case TV_CROWN:
	case TV_CLOAK:
	case TV_GLOVES:
	case TV_BOOTS:
		{
			/* Hack - extra chance for perilous armour */
			if (!power && (randint0(100) < great_percent))
				power = -2;

			if ((power > 1) || (power < -1)) {
				int ego_power;

				ego_power = make_ego_item(o_ptr, power);

				if (ego_power)
					power = ego_power;
			}

			if (power)
				a_m_aux_2(o_ptr, lev, power);
			break;
		}

	case TV_RING:
	case TV_AMULET:
		{
			/* Ignore power, generate, then we're done */
			design_ring_or_amulet(o_ptr, lev);
			return;
		}

	default:
		{
			a_m_aux_4(o_ptr, lev, power);
			break;
		}
	}

	/* 
	 * Throwing weapons, especially ego-items may sometimes be
	 * perfectly  balanced. 
	 */
	if (of_has(k_ptr->flags_obj, OF_THROWING)) {
		int j;

		/* 22% of standard items, 44% of ego items */
		j = (o_ptr->name2) ? 4 : 2;

		if (randint0(9) < j) {
			of_on(o_ptr->flags_obj, OF_PERFECT_BALANCE);
		}
	}

	/* Kind flags */
	if (o_ptr->name2)
		kf_union(flags_kind, e_info[o_ptr->name2].flags_kind);

	/* Random sustain */
	if (kf_has(flags_kind, KF_RAND_SUSTAIN)) {
		of_on(o_ptr->flags_obj,
			  OBJECT_RAND_BASE_SUSTAIN +
			  randint0(OBJECT_RAND_SIZE_SUSTAIN));
	}

	/* Random power */
	if (kf_has(flags_kind, KF_RAND_POWER)) {
		of_on(o_ptr->flags_obj,
			  OBJECT_RAND_BASE_POWER + randint0(OBJECT_RAND_SIZE_POWER));
	}

	/* Random curse */
	if (kf_has(flags_kind, KF_RAND_CURSE)) {
		bool another_curse = TRUE;

		/* Mostly only one curse */
		while (another_curse) {
			cf_on(o_ptr->flags_curse, FLAG_START + randint0(CF_MAX));
			if (randint0(10) != 0)
				another_curse = FALSE;
		}


		/* Hack - only drop weapons */
		while (cf_has(o_ptr->flags_curse, CF_DROP_WEAPON)
			   && (o_ptr->tval > TV_SWORD)) {
			cf_off(o_ptr->flags_curse, CF_DROP_WEAPON);
			cf_on(o_ptr->flags_curse, FLAG_START + randint0(CF_MAX));
		}
	}

	/* Extra damage dice */
	if (kf_has(flags_kind, KF_XTRA_DICE)) {
		if (((o_ptr->dd + 1) * o_ptr->ds) < 41) {
			o_ptr->dd += 1;
		} else {
			/* If we can't get an extra dice then weapon is lighter */
			o_ptr->weight = (5 * o_ptr->weight / 6);
		}
	}

	if (kf_has(flags_kind, KF_LIGHTWEIGHT)) {
		/* Weapons */
		if (o_ptr->tval <= TV_SWORD)
			o_ptr->weight = (4 * o_ptr->weight / 5);

		/* Armour */
		else
			o_ptr->weight = (2 * o_ptr->weight / 3);
	}

	if (kf_has(flags_kind, KF_HEAVY)) {
		o_ptr->weight = o_ptr->weight * 3;
	}

	/* Add resistances */
	apply_resistances(o_ptr, lev, flags_kind);


	/* Extra bonuses for ego-items */
	if (o_ptr->name2) {
		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		int av = 0, num = 0, chances;

		/* Get flags */
		of_union(o_ptr->flags_obj, e_ptr->flags_obj);
		cf_union(o_ptr->flags_curse, e_ptr->flags_curse);

		/* Get activation */
		if (e_ptr->effect)
			o_ptr->effect = e_ptr->effect;
		if ((e_ptr->time.base != 0) || (e_ptr->time.dice != 0)) {
			o_ptr->time.base = e_ptr->time.base;
			o_ptr->time.dice = e_ptr->time.dice;
			o_ptr->time.sides = e_ptr->time.sides;
			o_ptr->time.m_bonus = e_ptr->time.m_bonus;
		}

		/* Assign bonuses (random between base and ego) */
		for (i = 0; i < A_MAX; i++) {
			if (e_ptr->bonus_stat[i] > 0)
				o_ptr->bonus_stat[i] += randint1(e_ptr->bonus_stat[i]);
			else if (e_ptr->bonus_stat[i] < 0) {
				o_ptr->bonus_stat[i] -= randint1(-e_ptr->bonus_stat[i]);
				av += o_ptr->bonus_stat[i];
				num++;
			}
		}

		/* Calculate extra sides for, eg, Angband weapons */
		if (kf_has(flags_kind, KF_XTRA_SIDES)) {
			/* Extra sides to compensate for stat penalties */
			if (num) {
				av /= num;
				chances = (randint1(1 - av) + 1 - av) / 2;

				for (i = 0; i < chances; i++) {
					if (randint1(3 - av) < (1 - av))
						o_ptr->ds++;
				}
			}
			/* Mainly for bashing shields */
			else {
				o_ptr->ds += m_bonus(2, lev);
			}
		}
		for (i = 0; i < MAX_P_BONUS; i++) {
			if (e_ptr->bonus_other[i] > 0)
				o_ptr->bonus_other[i] += randint1(e_ptr->bonus_other[i]);
			else if (e_ptr->bonus_other[i] < 0)
				o_ptr->bonus_other[i] -= randint1(-e_ptr->bonus_other[i]);
		}

		/* Assign multiples */
		for (i = 0; i < MAX_P_SLAY; i++)
			o_ptr->multiple_slay[i] =
				MAX(e_ptr->multiple_slay[i], o_ptr->multiple_slay[i]);
		for (i = 0; i < MAX_P_BRAND; i++)
			o_ptr->multiple_brand[i] =
				MAX(e_ptr->multiple_brand[i], o_ptr->multiple_brand[i]);


		/* Apply extra bonuses or penalties. */
		if ((e_ptr->max_to_h > 0) && (e_ptr->max_to_h < 129))
			o_ptr->to_h += randint1(e_ptr->max_to_h);
		else if (e_ptr->max_to_h > 128)
			o_ptr->to_h -= randint1(e_ptr->max_to_h - 128);

		if ((e_ptr->max_to_d > 0) && (e_ptr->max_to_d < 129))
			o_ptr->to_d += randint1(e_ptr->max_to_d);
		else if (e_ptr->max_to_d > 128)
			o_ptr->to_d -= randint1(e_ptr->max_to_d - 128);

		if ((e_ptr->max_to_a > 0) && (e_ptr->max_to_a < 129))
			o_ptr->to_a += randint1(e_ptr->max_to_a);
		else if (e_ptr->max_to_a > 128)
			o_ptr->to_a -= randint1(e_ptr->max_to_a - 128);


		/* Extra base armour class */
		if (kf_has(flags_kind, KF_XTRA_AC)) {
			o_ptr->ac += 5;
		}

		/* Extra skill bonus */
		if (kf_has(flags_kind, KF_XTRA_TO_H)) {
			if (o_ptr->to_h < 0)
				o_ptr->to_h = 0 - o_ptr->to_h;
			o_ptr->to_h += m_bonus(10, lev);
		}

		/* Extra deadliness bonus */
		if (kf_has(flags_kind, KF_XTRA_TO_D)) {
			if (o_ptr->to_d < 0)
				o_ptr->to_d = 0 - o_ptr->to_d;
			o_ptr->to_d += m_bonus(10, lev);
		}

		/* Extra armour class bonus */
		if (kf_has(flags_kind, KF_XTRA_TO_A)) {
			if (o_ptr->to_a < 0)
				o_ptr->to_a = 0 - o_ptr->to_a;
			o_ptr->to_a += m_bonus(10, lev);
		}

		/* Hack -- Frequently neaten missile to_h and to_d, for improved
		 * stacking qualities. */
		{
			if ((o_ptr->to_h % 2 == 1) && (randint1(4) != 1))
				o_ptr->to_h++;
			if ((o_ptr->to_d % 2 == 1) && (randint1(4) != 1))
				o_ptr->to_d++;
		}


		/* Hack -- apply rating bonus */
		rating += e_ptr->rating;

		/* Cheat -- describe the item */
		if (OPT(cheat_peek))
			object_mention(o_ptr);

		/* Done */
		return;
	}
}


/**
 * Hack -- determine if a template is the right kind of object. -LM-
 */
static bool kind_is_right_kind(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* We are only looking for non-worthless items of the tval asked for. */
	if ((k_ptr->tval == required_tval) && (k_ptr->cost > 0))
		return (TRUE);
	else
		return (FALSE);
}

/**
 * Hack -- determine if a template is "good".  This concept has been consider-
 * ably expanded in Oangband. -LM-
 */
static bool kind_is_good(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval) {
		/* Armor -- Good unless damaged */
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
		{
			if (k_ptr->to_a.base < 0)
				return (FALSE);
			return (TRUE);
		}

		/* Weapons -- Good unless damaged.  Diggers are no longer good. */
	case TV_BOW:
	case TV_SWORD:
	case TV_HAFTED:
	case TV_POLEARM:
		{
			if (k_ptr->to_h.base < 0)
				return (FALSE);
			if (k_ptr->to_d.base < 0)
				return (FALSE);
			return (TRUE);
		}

		/* Ammo -- Shots/Arrows/Bolts are good. */
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
		{
			return (TRUE);
		}

		/* Books -- High level books are good */
	case TV_MAGIC_BOOK:
	case TV_PRAYER_BOOK:
	case TV_DRUID_BOOK:
	case TV_NECRO_BOOK:
		{
			if (k_ptr->sval >= SV_BOOK_MIN_GOOD)
				return (TRUE);
			return (FALSE);
		}

		/* Rings -- any with a high enough value. */
	case TV_RING:
		{

			if (k_ptr->cost >= 500 + p_ptr->depth * 90)
				return (TRUE);

			return (FALSE);
		}

		/* Amulets -- any with a high enough value. */
	case TV_AMULET:
		{
			if (k_ptr->cost >= 500 + p_ptr->depth * 90)
				return (TRUE);

			return (FALSE);
		}

		/* Chests are always good. */
	case TV_CHEST:
		{
			return (TRUE);
		}

		/* Wands of Dragon Fire, Cold, and Breath, and of Annihilation are
		 * good, as are any with a high enough value. */
	case TV_WAND:
		{
			if (k_ptr->sval == SV_WAND_DRAGON_FIRE)
				return (TRUE);
			if (k_ptr->sval == SV_WAND_DRAGON_COLD)
				return (TRUE);
			if (k_ptr->sval == SV_WAND_DRAGON_BREATH)
				return (TRUE);
			if (k_ptr->sval == SV_WAND_ANNIHILATION)
				return (TRUE);
			if (k_ptr->sval == SV_WAND_TELEPORT_AWAY)
				return (TRUE);

			if (k_ptr->cost >= 400 + p_ptr->depth * 60)
				return (TRUE);

			return (FALSE);
		}

		/* Staffs of Power, Holiness, Mana Storm, and Starburst are good, as
		 * are any with a high enough value. */
	case TV_STAFF:
		{
			if (k_ptr->sval == SV_STAFF_POWER)
				return (TRUE);
			if (k_ptr->sval == SV_STAFF_HOLINESS)
				return (TRUE);
			if (k_ptr->sval == SV_STAFF_MSTORM)
				return (TRUE);
			if (k_ptr->sval == SV_STAFF_STARBURST)
				return (TRUE);
			if (k_ptr->sval == SV_STAFF_BANISHMENT)
				return (TRUE);

			if (k_ptr->cost >= 500 + p_ptr->depth * 60)
				return (TRUE);

			return (FALSE);
		}

		/* Rods of ID, Curing, Restoration, Speed, Healing, Lighting Strike,
		 * Northwinds, Dragonfire, and Glaurung's Blood are all good, as are
		 * any with a high enough value. */
	case TV_ROD:
		{
			if (k_ptr->sval == SV_ROD_CURING)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_RESTORATION)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_SPEED)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_HEALING)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_LIGHTINGSTRIKE)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_NORTHWINDS)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_DRAGONFIRE)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_GLAURUNGS)
				return (TRUE);
			if (k_ptr->sval == SV_ROD_TELEPORT_AWAY)
				return (TRUE);

			if (k_ptr->cost >= 500 + p_ptr->depth * 100)
				return (TRUE);

			return (FALSE);
		}

		/* Any potion or scroll that costs a certain amount or more must be
		 * good. */
	case TV_POTION:
		{
			if (k_ptr->sval == SV_POTION_HEALING)
				return (TRUE);
			if (k_ptr->sval == SV_POTION_STAR_HEALING)
				return (TRUE);
			if (k_ptr->sval == SV_POTION_LIFE)
				return (TRUE);
			if (k_ptr->sval == SV_POTION_RESTORE_MANA)
				return (TRUE);
		}
	case TV_SCROLL:
		{
			if (k_ptr->cost >= 10000)
				return (TRUE);
			if (k_ptr->cost >= 500 + p_ptr->depth * 110)
				return (TRUE);
			return (FALSE);
		}
	}

	/* Assume not good */
	return (FALSE);
}

/**
 * Attempt to make an object (normal, good/great, or with a specified tval)
 *
 * This routine plays nasty games to generate the "special artifacts".
 *
 * This routine uses "object_level" for the "generation level".
 *
 * We assume that the given object has been "wiped".
 *
 * Reject some poor items -LM-
 */
bool make_object(object_type * j_ptr, bool good, bool great,
				 bool exact_kind)
{
	int prob, base;

	object_kind *k_ptr;

	/* Chance of "special object" - now depth dependant */
	prob = (1000 - (6 * object_level));

	/* Limit base chance of "special object" */
	if (prob < 700)
		prob = 700;

	/* Increase chance of "special object" for good items */
	if (good)
		prob = 10;

	/* Base level for the object.  Only "great" objects get this bonus in
	 * Oangband. */
	base = (great ? (object_level + 10) : object_level);


	/* If an object is rejected because it is sufficiently low-level and cheap
	 * to be of little possible interest, try again. */
  try_again:
	;


	/* Generate a special object, or a normal object */
	if ((randint0(prob) != 0) || !make_artifact_special(j_ptr)) {
		int k_idx;

		object_kind *k_ptr;

		/* Good objects */
		if (good) {
			/* Activate restriction */
			get_obj_num_hook = kind_is_good;

			/* Prepare allocation table */
			get_obj_num_prep();
		}

		/* Objects with a particular tval.  Only activate if there is a valid
		 * tval to restrict to. */
		if (exact_kind && required_tval) {
			/* Activate restriction */
			get_obj_num_hook = kind_is_right_kind;

			/* Prepare allocation table */
			get_obj_num_prep();
		}

		/* get an object index */
		k_idx = get_obj_num(base);

		/* Handle failure */
		if (!k_idx) {
			/* Clear any special object restriction, and prepare the standard
			 * object allocation table. */
			if (get_obj_num_hook) {
				/* Clear restriction */
				get_obj_num_hook = NULL;

				/* Prepare allocation table */
				get_obj_num_prep();
			}

			/* No object was created. */
			return (FALSE);
		}

		/* Get the object kind. */
		k_ptr = &k_info[k_idx];

		/* Usually (67% chance) forbid very low-level objects that are
		 * intrinsically worthless. */
		if ((k_ptr->level < object_level / 4) && (k_ptr->cost < 1)
			&& (randint1(3) != 1))
			goto try_again;

		/* Sometimes (33% chance) forbid very low-level objects that aren't
		 * worth very much. */
		else if ((k_ptr->level < object_level / 4)
				 && (k_ptr->cost < object_level - 20)
				 && (randint1(3) == 1))
			goto try_again;

		/* Hack -- If a chest is specifically asked for, almost always winnow
		 * out any not sufficiently high-level. */
		if (required_tval == TV_CHEST) {
			/* Need to avoid asking for the impossible. */
			int tmp = object_level > 100 ? 100 : object_level;

			if ((k_ptr->level < 2 * (tmp + 3) / 4 + randint1(tmp / 4))
				&& (randint1(10) != 1))
				goto try_again;
		}

		/* Clear any special object restriction, and prepare the standard
		 * object allocation table. */
		if (get_obj_num_hook) {
			/* Clear restriction */
			get_obj_num_hook = NULL;

			/* Prepare allocation table */
			get_obj_num_prep();
		}

		/* Prepare the object */
		object_prep(j_ptr, k_idx, RANDOMISE);
	}

	/* Apply magic (allow artifacts) */
	apply_magic(j_ptr, object_level, !MODE(NO_ARTIFACTS), good, great);

	/* Get the object kind. */
	k_ptr = &k_info[j_ptr->k_idx];

	/* Hack - Certain object types appear in stacks. */
	switch (j_ptr->tval) {
	case TV_SPIKE:
		{
			j_ptr->number = damroll(6, 7);
			break;
		}
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		{
			j_ptr->number = damroll(10, 7);
			break;
		}
	case TV_FOOD:
		{
			if ((k_ptr->cost < 150) && (randint1(2) == 1))
				j_ptr->number = damroll(2, 3);
			break;
		}
	case TV_HAFTED:
	case TV_SWORD:
	case TV_POLEARM:
		{
			if (of_has(k_ptr->flags_obj, OF_THROWING) && (!j_ptr->name1)
				&& (randint1(2) == 1))
				j_ptr->number = damroll(2, 2);
			break;
		}
	default:
		{
			break;
		}
	}

	/* Notice "okay" out-of-depth objects */
	if (!cursed_p(j_ptr) && (k_info[j_ptr->k_idx].level > p_ptr->depth)) {
		/* Rating increase */
		rating += (k_info[j_ptr->k_idx].level - p_ptr->depth);

		/* Cheat -- peek at items */
		if (OPT(cheat_peek))
			object_mention(j_ptr);
	}

	/* Return */
	return (TRUE);
}



/**
 * Make a treasure object.  This function has been reworked in Oangband, 
 * but is still a bit of a hack.
 */
bool make_gold(object_type * j_ptr)
{
	int i;
	int treasure = 0;
	int gold_depth, first_gold_idx;

	/* Hack -- Creeping Coins only generate "themselves" */
	if (coin_type)
		treasure = coin_type;


	else {
		/* Hack -- Start looking at location of copper. */
		first_gold_idx = lookup_kind(TV_GOLD, SV_COPPER);

		/* Normal treasure level is between 1/2 and the full object level. */
		gold_depth = (object_level / 2) + randint1((object_level + 1) / 2);

		/* Apply "extra" magic. */
		if (randint0(GREAT_OBJ) == 0)
			gold_depth += randint1(gold_depth);

		/* Find the highest-level legal gold object. */
		for (i = first_gold_idx; i < first_gold_idx + SV_GOLD_MAX; i++) {
			/* Paranoia -- Skip objects without value. */
			if (!k_info[i].cost)
				continue;

			/* Stop if we're going too deep. */
			if (k_info[i].level > gold_depth)
				break;

			/* Otherwise, keep this index. */
			treasure = i;
		}
	}

	/* Report failure.  This should not happen. */
	if (!treasure)
		return (FALSE);


	/* Prepare a gold object */
	object_prep(j_ptr, treasure, RANDOMISE);

	/* Treasure can be worth between 1/2 and the full maximal value. */
	j_ptr->pval =
		k_info[treasure].cost / 2 +
		randint1((k_info[treasure].cost + 1) / 2);

	/* Hack - double for no selling */
	if (MODE(NO_SELLING))
		j_ptr->pval *= 2;

	/* Success */
	return (TRUE);
}





/**
 * Attempt to place an object (normal or good/great) at the given location.
 */
void place_object(int y, int x, bool good, bool great, bool exact_kind,
				  byte origin)
{
	object_type *i_ptr;
	object_type object_type_body;

	/* Paranoia */
	if (!in_bounds(y, x))
		return;

	/* Require clean floor space */
	if (!cave_clean_bold(y, x))
		return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make an object */
	if (make_object(i_ptr, good, great, exact_kind)) {
		/* Set the origin */
		i_ptr->origin = origin;
		i_ptr->origin_stage = p_ptr->stage;

		/* Give it to the floor */
		if (!floor_carry(y, x, i_ptr)) {
			/* Preserve artifacts */
			a_info[i_ptr->name1].created = FALSE;
		}
	}

}


/**
 * Places a treasure (Gold or Gems) at given location
 */
void place_gold(int y, int x)
{
	object_type *i_ptr;
	object_type object_type_body;

	/* Paranoia */
	if (!in_bounds(y, x))
		return;

	/* Require clean floor space */
	if (!cave_clean_bold(y, x))
		return;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make some gold */
	if (make_gold(i_ptr)) {
		/* Give it to the floor */
		(void) floor_carry(y, x, i_ptr);
	}
}
