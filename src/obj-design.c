/**
 * \file obj-design.c 
 * \brief Random artifact and jewellery design
 *
 * Random artifacts, rings and amulets.  Selling and providing qualities.
 * Choosing object type and kind, determining the potential.
 * Depth and rarity for artifacts.
 * Selecting a theme, and the properties of all possible themes.  
 * Adding semi-random qualities until potential is all used up.
 * Cursing an artifact.
 * Removing contradictory flags.
 * Naming an artifact, using either of two methods.
 * Initializing random artifacts.
 *
 * Copyright (c) 1998 Leon Marrick
 *
 * Thanks to Greg Wooledge for his support and string-handling code 
 * and to W. Sheldon Simms for his Tolkienesque random name generator.
 *
 * Copyright (c) Si Griffin
 * Copyright (c) 2020 Nick McConnell
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
#include "effects.h"
#include "effects-info.h"
#include "init.h"
#include "obj-curse.h"
#include "obj-design.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-properties.h"
#include "obj-slays.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "object.h"
#include "randname.h"

/**
 * ------------------------------------------------------------------------
 * Potential variables
 * ------------------------------------------------------------------------ */
/**
 * A global variable whose contents will be bartered to acquire powers.
 */
static int potential = 0;

/**
 * The initial potential of the artifact or object.
 */
static int initial_potential = 0;

/**
 * The maximum potential that that artifact could have possessed.
 */
static int max_potential = 0;


/**
 * ------------------------------------------------------------------------
 * Utilities for allocation of object properties
 * ------------------------------------------------------------------------ */
/**
 * Debit an object's account.
 */
static bool take_money(int cost, bool on_credit)
{
	/* Take the money. */
	if (potential > cost) {
		potential -= cost;
		return true;
	} else if (on_credit && (potential != 0)) {
		/* <<mutter>> OK, I'll charge it to your account... */
		potential = 0;
		return true;
	}

	/* Otherwise, kick da bum out. */
	return false;
}

/**
 * Check if an artifact or object has a given property.
 */
static bool has_property(struct artifact *art, struct object *obj,
						 const char *prop_name)
{
	struct obj_property *prop = lookup_obj_property_name(prop_name);
	assert(prop);
	assert(art || ((art == NULL) && obj));

	switch (prop->type) {
		case OBJ_PROPERTY_STAT:
		case OBJ_PROPERTY_MOD:
		{
			s16b *modifiers = art ? art->modifiers : obj->modifiers;
			if (modifiers[prop->index] != 0) {
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_FLAG:
		{
			bitflag *flags = art ? art->flags : obj->flags;
			if (of_has(flags, prop->index)) {
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_ELEMENT:
		{
			struct element_info *el_info = art ? art->el_info : obj->el_info;
			if (el_info[prop->index].res_level != RES_LEVEL_BASE) {
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_IGNORE:
		{
			struct element_info *el_info = art ? art->el_info : obj->el_info;
			if (el_info[prop->index].flags & EL_INFO_IGNORE) {
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_BRAND:
		{
			bool *b = art ? art->brands : obj->brands;
			int pick;

			/* See if there are any brands, if so get the correct one */
			if (!b) return false;
			for (pick = 1; pick < z_info->brand_max; pick++) {
				if (strstr(prop->name, brands[pick].name) && b[pick])
					break;
			}

			if (pick < z_info->brand_max) {
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_SLAY:
		{
			bool *s = art ? art->slays : obj->slays;
			int pick;

			/* See if there are any slays, if so get the correct one */
			if (!s) return false;
			for (pick = 1; pick < z_info->slay_max; pick++) {
				if (strstr(prop->name, slays[pick].name) && s[pick])
					break;
			}

			if (pick < z_info->slay_max) {
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_COMBAT:
		{
			/* Get the object kind to compare */
			struct object_kind *kind = art ? lookup_kind(art->tval, art->sval)
				: lookup_kind(obj->tval, obj->sval);

			/* Enhanced dice require special calculations */
			if (streq(prop->name, "enhanced dice")) {
				if (art) {
					if ((art->dd > kind->dd) || (art->ds > kind->ds)) {
						return true;
					}
				} else if ((obj->dd > kind->dd) || (obj->ds > kind->ds)) {
					return true;
				}
			} else {
				/* Various things get checked to here */
				s16b bonus = 0, standard = 0;
				if (streq(prop->name, "extra armor")) {
					bonus = art ? art->ac : obj->ac;
					standard = kind->ac;
				} else if (streq(prop->name, "armor bonus")) {
					bonus = art ? art->to_a : obj->to_a;
				} else if (streq(prop->name, "skill bonus")) {
					bonus = art ? art->to_h : obj->to_h;
				} else if (streq(prop->name, "deadliness bonus")) {
					bonus = art ? art->to_d : obj->to_d;
				}
				if (bonus != standard) {
					return true;
				}
			}
			break;
		}

		default: msg("error -- object property not recognized."); break;
	}

	return false;
}

/**
 * Grant the property asked for, if the artifact or object can afford it.
 */
static bool get_property(struct artifact *art, struct object *obj,
						 const char *prop_name, int value,
						 bool on_credit)
{
	int cost;
	struct obj_property *prop = lookup_obj_property_name(prop_name);
	assert(prop);

	/* Basic cost */
	cost = property_cost(prop, value, false);

	assert(art || ((art == NULL) && obj));
	switch (prop->type) {
		case OBJ_PROPERTY_STAT:
		case OBJ_PROPERTY_MOD:
		{
			s16b *modifiers = art ? art->modifiers : obj->modifiers;
			int cur_value = modifiers[prop->index];
			int cur_cost = art ? 0 : property_cost(prop, cur_value, false);
			if (take_money(cost - cur_cost, on_credit)) {
				modifiers[prop->index] += value;
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_FLAG:
		{
			bitflag *flags = art ? art->flags : obj->flags;
			if (take_money(cost, on_credit)) {
				of_on(flags, prop->index);
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_ELEMENT:
		{
			struct element_info *el_info = art ? art->el_info : obj->el_info;
			int cur_value = RES_LEVEL_BASE - el_info[prop->index].res_level;
			int cur_cost = art ? 0 : property_cost(prop, cur_value, false);
			if (take_money(cost - cur_cost, on_credit)) {
				el_info[prop->index].res_level -= value;
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_IGNORE:
		{
			struct element_info *el_info = art ? art->el_info : obj->el_info;
			if (take_money(cost, on_credit)) {
				el_info[prop->index].flags |= EL_INFO_IGNORE;
				return true;
			}
			break;
		}
		case OBJ_PROPERTY_BRAND:
		{
			/* Get the correct brand */
			int pick;
			for (pick = 1; pick < z_info->brand_max; pick++) {
				char *s = strstr(prop->name, brands[pick].name);
				if (s && (brands[pick].multiplier == value))
					break;
			}
			assert(pick < z_info->brand_max);

			/* Simple for artifacts */
			if (art) {
				if (take_money(cost, on_credit) &&
					append_brand(&art->brands, pick))
					return true;
			} else {
				/* Objects need to subtract off their current effective cost */
				int i, cur_cost, cur_value = MULTIPLE_BASE;

				/* Find any existing brand of the same element */
				if (obj->brands) {
					for (i = 1; i < z_info->brand_max; i++) {
						if (obj->brands[i] &&
							streq(prop->name, brands[i].name)) {
							cur_value = brands[i].multiplier;
						}
					}
				}

				/* Work out the current cost and subtract it */
				cur_cost = property_cost(prop, cur_value, false);
				if (take_money(cost - cur_cost, on_credit)) {
					append_brand(&obj->brands, pick);
					return true;
				}
			}
			break;
		}
		case OBJ_PROPERTY_SLAY:
		{
			/* Get the correct slay */
			int pick;
			for (pick = 1; pick < z_info->slay_max; pick++) {
				char *s = strstr(prop->name, slays[pick].name);
				if (s && (slays[pick].multiplier == value))
					break;
			}
			assert(pick < z_info->slay_max);

			/* Simple for artifacts */
			if (art) {
				if (take_money(cost, on_credit) &&
					append_slay(&art->slays, pick))
					return true;
			} else {
				/* Objects need to subtract off their current effective cost */
				int i, cur_cost, cur_value = MULTIPLE_BASE;

				/* Find any existing slay of the same type */
				if (obj->slays) {
					for (i = 1; i < z_info->slay_max; i++) {
						if (obj->slays[i] &&
							streq(prop->name, slays[i].name)) {
							cur_value = slays[i].multiplier;
						}
					}
				}

				/* Work out the current cost and subtract it */
				cur_cost = property_cost(prop, cur_value, false);
				if (take_money(cost - cur_cost, on_credit)) {
					append_slay(&obj->slays, pick);
					return true;
				}
			}
			break;
		}
		case OBJ_PROPERTY_COMBAT:
		{
			/* Enhanced dice require special calculations */
			if (streq(prop->name, "enhanced dice")) {
				int i, old_ds, cur_potential;

				/* Only artifacts, as jewellery doesn't get enhanced dice */
				assert(art);
				old_ds = art->ds;

				/* Allow a portion of the budget for improvements.
				 *  Use "value" to control fraction. */
				cur_potential = potential / value;
				if ((value > 1) && (cur_potential > 1000))
					cur_potential = 1000;

				/* If credit is extended, guarantee something to work with. */
				if (on_credit) {
					if (cur_potential < 500)
						cur_potential = 500;
					if (potential > cur_potential) {
						potential -= cur_potential;
					} else {
						potential = 0;
					}
				} else {
					/* Otherwise, subtract whatever portion is alloted from
					 * the main budget. */
					potential -= cur_potential;
				}

				/* Enhance the damage dice, depending on potential. */
				/* SJGU  Prevent most single dice weapons getting massive ds */
				if ((randint1(4) != 1) && ((art->dd == 1) &&
										   (cur_potential >= 400))) {
					while (cur_potential <= 100 * art->ds) {
						art->ds--;
						cur_potential += 200;
					}
					art->dd++;
					cur_potential -= 100 * art->ds;
				}
				for (i = 0; i < 5; i++) {
					if ((randint1(3) != 1) &&
						(cur_potential >= 100 * art->ds)) {
						art->dd++;
						cur_potential -= 100 * art->ds;
					} else if (cur_potential >= 200 * art->dd) {
						art->ds = art->ds + 2;
						cur_potential -= 200 * art->dd;
					} else {
						/* Either stop the loop or increment the dice sides. */
						if (cur_potential < 100 * art->dd)
							break;
						art->ds++;
						cur_potential -= 100 * art->dd;
					}

					/* SJGU Hack. Prevent any weapons getting crazy ds. Boo Hiss
					 * Note that ENHANCE_DICE can potentially be called more
					 * than once though
					 * In which case, Happy Birthday! */
					if (art->ds >= 2 * old_ds)
						break;
				}
				break;
			} else {
				/* Various things get added to here */
				int cur_value, cur_cost;
				s16b *bonus;
				if (streq(prop->name, "extra armor")) {
					bonus = art ? &art->ac : &obj->ac;
				} else if (streq(prop->name, "armor bonus")) {
					bonus = art ? &art->to_a : &obj->to_a;
				} else if (streq(prop->name, "skill bonus")) {
					bonus = art ? &art->to_h : &obj->to_h;
				} else if (streq(prop->name, "deadliness bonus")) {
					bonus = art ? &art->to_d : &obj->to_d;
				} else {
					assert(0);
				}
				cur_value = *bonus;
				cur_cost = art ? 0 : property_cost(prop, cur_value, false);
				if (take_money(cost - cur_cost, on_credit)) {
					*bonus += value;
					return true;
				}
				break;
			}
		}

		default: msg("error -- object property not recognized."); break;
	}

	return false;
}

/**
 * Select a property from a group, subject to affordability, and ranked by
 * various methods.  Properties which cost a negative amount don't need to be
 * considered for ranking methods, since they are always affordable.
 */
static int select_property(int temp_potential, const char **property_list,
						   const int choices, int *max_value,
						   int rank_method, struct object *obj)
{
	int i, j, optimal = 0;
	int selection = 0;
	int current_value = 0;

	int prices[choices][*max_value + 1];

	bool found_it = false;

	/* Run through choices, record costs */
	for (i = 0; i < choices; i++) {
		for (j = 0; j <= *max_value; j++) {
			struct object *test_obj = object_new();

			/* Copy the object, try buying the property */
			object_copy(test_obj, obj);
			if (get_property(NULL, test_obj, property_list[i], j, false)) {
				prices[i][j] = temp_potential - potential;
				potential = temp_potential;
			} else {
				prices[i][j] = TOO_MUCH;
			}

			/* If the copy hasn't changed, the property was already there */
			if (object_similar(obj, test_obj, OSTACK_NONE)) {
				prices[i][j] = 0;
			}
			object_delete(NULL, NULL, &test_obj);
		}
	}

	/* Initialise best */
	if (rank_method == RANK_CHEAPEST_FIRST) {
		optimal = TOO_MUCH;
	}

	/* Run through again, selecting best */
	for (i = 0; i < choices; i++) {
		for (j = 0; j <= *max_value; j++) {
			switch (rank_method) {
				case RANK_CHEAPEST_FIRST: {
					if ((prices[i][j] > 0) && (prices[i][j] < optimal)) {
						optimal = prices[i][j];
						selection = i;
						current_value = j;
					}
					break;
				}
				case RANK_DEAREST_FIRST: {
					if ((prices[i][j] < TOO_MUCH)
						&& (prices[i][j] > optimal)) {
						optimal = prices[i][j];
						selection = i;
						current_value = j;
					}
					break;
				}
				case RANK_RANDOM_CHOICE: {
					int paranoia = 0;
					while ((!optimal) && (paranoia < 20)) {
						selection = randint0(choices);
						current_value = randint0(*max_value + 1);
						if ((prices[selection][current_value] < TOO_MUCH)
							&& (prices[selection][current_value] != 0)) {
							optimal = prices[selection][current_value];
						}
						paranoia++;
					}
					if (optimal) {
						found_it = true;
					}
					break;
				}
				case RANK_FIRST_VALID: {
					if ((prices[i][j] < TOO_MUCH) && (prices[i][j] > 0)) {
						selection = i;
						current_value = j;
						found_it = true;
					}
					break;
				}
				case RANK_FIND_MAX_VALUE: {
					if ((prices[i][j] < TOO_MUCH) && (prices[i][j] > 0)) {
						if (current_value < j) {
							current_value = j;
							selection = i;
						}
					}
					break;
				}
			}
			if (found_it)
				break;
		}
		if (found_it)
			break;
	}

	/* Set the max value */
	*max_value = current_value;

	/* Have we found one? */
	switch (rank_method) {
		case RANK_CHEAPEST_FIRST: {
			if (optimal < TOO_MUCH) {
				found_it = true;
			}
			break;
		}
		case RANK_DEAREST_FIRST: {
			if (optimal > 0) {
				found_it = true;
			}
			break;
		}
		case RANK_FIND_MAX_VALUE: {
			if (current_value > 0)
				found_it = true;
			break;
		}
	}

	/* Return if found */
	if (found_it) {
		return (selection);
	} else {
		return 0;
	}
}

/**
 * Find an activation that matches an object's effect (if one exists)
 */
static struct activation *find_activation_for_effect(struct effect *effect)
{
	int i;
	for (i = 0; i < z_info->act_max; i++) {
		struct activation *act = &activations[i];
		if (!act->effect) continue;
		if (effect_equal(effect, act->effect)) continue;
		return act;
	}
	return NULL;
}

/**
 * Grant the activation asked for, if the object can afford it.
 */
static bool get_activation(const char *act_name, struct object *obj)
{
	struct activation *act = lookup_activation(act_name);

	/* Allocate the activation, if affordable */
	if (act && take_money(act->power * EFFECT_MULT, true)) {
		obj->activation = act;
		if (act->time.base) {
			obj->time = act->time;
		} else {
			obj->time.base = (act->power * 8);
			obj->time.dice = 1;
			obj->time.sides = (act->power * 8);
		}
		return true;
	}

	return false;
}

/**
 * Get a random activation of the required type.
 */
static bool get_random_activation(int type, struct object *obj) {
	int i, count = 0, pick;

	/* Count the appropriate activations */
	for (i = 0; i < z_info->act_max; i++) {
		struct activation *act = &activations[i];
		if (act->type == type) count++;
	}

	/* Pick an activation and add it */
	pick = randint0(count);
	for (i = 0; i < z_info->act_max; i++) {
		struct activation *act = &activations[i];
		if (act->type != type) continue;

		/* Found it? */
		if (!pick) {
			get_activation(act->name, obj);
			return true;
		} else {
			pick--;
		}
	}
	return false;
}

/**
 * ------------------------------------------------------------------------
 * Creation of random artifacts
 * ------------------------------------------------------------------------ */
/**
 * Assign a tval and sval, grant a certain amount of potential to be used 
 * for acquiring powers, and determine rarity and native depth.
 */
static void initialize_artifact(struct artifact *art)
{
	int i, index;
	struct object_kind *kind;

	/* Wipe the artifact completely. */
	memset(art, 0, sizeof(*art));

	/* Assign a tval and sval.  If the base object is powerful, an amount to
	 * reduce artifact power by will be calculated.  If the base object has an
	 * activation, it will be preserved (at least initially). */
	while (true) {
		/* Acquire an object at random */
		index = randint0(z_info->k_max);
		kind = &k_info[index];

		/* Skip "empty" objects */
		if (!kind->name) continue;

		/* Skip objects which can't be randomly allocated */
		if (!kind->alloc_prob) continue;

		/* Skip objects that are not weapons or armour. */
		if (!tval_can_be_artifact_k(kind)) continue;

		/* Determine rarity */
		if (one_in_(100 / kind->alloc_prob)) break;
	}

	/* Store the base values of a bunch of data.  Bonuses to Skill, Deadliness,
	 * and Armour Class are cut in half.  Dragon scale mail activations are
	 * preserved.  Base object cost is preserved if sufficiently high. */
	art->tval = kind->tval;
	art->sval = kind->sval;
	art->to_h = kind->to_h.base / 2;
	art->to_d = kind->to_d.base / 2;
	art->to_a = kind->to_a.base / 2;
	art->ac = kind->ac;
	art->dd = kind->dd;
	art->ds = kind->ds;
	if (kind->cost > 4999)
		art->cost = kind->cost;
	art->weight = kind->weight;
	of_copy(art->flags, kind->flags);
	for (i = 0; i < ELEM_MAX; i++)
		art->el_info[i].res_level = kind->el_info[i].res_level;
	for (i = 0; i < OBJ_MOD_MAX; i++)
		art->modifiers[i] = kind->modifiers[i].base;
	copy_slays(&art->slays, kind->slays);
	copy_brands(&art->brands, kind->brands);
	art->activation = find_activation_for_effect(kind->effect);
	art->time = kind->time;

	/* Ignore everything */
	for (i = 0; i < ELEM_BASE_MAX; i++) {
		art->el_info[i].flags |= EL_INFO_IGNORE;
	}

	/* The total potential of an artifact ranges from 1750 to 8750, biased
	 * towards the lesser figure. */
	/* Now less biased -NRM- */
	potential = 1750;
	for (i = 0; i < 14; i++) {
		if (randint1(15) > 2) {
			potential += 500;
		} else {
			break;
		}
	}
	
	/* Preserve balance by not allowing powerful base objects too many powers */
	potential = MAX(potential - kind->power, 1750);

	/* Many equipment types include no truly powerful artifacts.  Maximum and
	 * actual potential is limited. SJGU but less so now */
	max_potential = kind->base->potential;
	potential = MIN(potential, max_potential);

	/* Remember how much potential was allowed. */
	initial_potential = potential;

	/* The cost of the artifact depends solely on its potential (allowing for
	 * valuable base object kinds). */
	art->cost += (potential * 10L);

	/* Calculate artifact depths.  This is fairly simple. */
	art->alloc_min = MAX(MAX((potential / 100) - 10, kind->alloc_min - 10), 5);
	art->alloc_max = MIN(127,
						 art->alloc_min + kind->alloc_max - kind->alloc_min);

	/* Calculate artifact rarity (simplified from FA1.4, may need adjustment) */
	art->alloc_prob = 10000 / (potential + kind->power);
	art->alloc_prob *= 100;
	art->alloc_prob /= kind->alloc_prob;

	/* Boundary control. */
	art->alloc_prob = MIN(30, art->alloc_prob);
}

/**
 * Pick an initial set of qualities for a melee weapon, based on a theme.
 * Also add a bonus to Skill and Deadliness.
 */
static void choose_melee_weapon_theme(struct artifact *art)
{
	/* I'm a melee weapon... */
	int selection = randint0(100);

	/* ...with bonuses to Deadliness and Skill, and... */
	art->to_d += (3 + randint1(7) + potential / 650);
	art->to_h += (3 + randint1(7) + potential / 650);

	/* ...of fire. */
	if (selection < 6) {

		/* Possibly assign an activation for free. */
		if (one_in_(3)) {
			if (potential >= 6000) {
				art->activation = lookup_activation("RAND_FIRE3");
			} else if (potential >= 3000) {
				art->activation = lookup_activation("RAND_FIRE2");
			} else {
				art->activation = lookup_activation("RAND_FIRE1");
			}
		}

		/* Brand the weapon with fire. */
		get_property(art, NULL, "fire brand", 17, true);

		/* Grant either a resist or immunity to fire. */
		if ((potential >= 4500) && one_in_(3)) {
			get_property(art, NULL, "fire resistance", 100, false);
		} else {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						false);
		}

		/* Sometimes vulnerable to cold */
		if (one_in_(5)) {
			get_property(art, NULL, "cold resistance", -5 * randint1(6), false);
		}
		/* Demons like fire */
		if (one_in_(4)) {
			append_artifact_curse(art, lookup_curse("demon summon"),
								  40 + randint0(20));
			potential += 200;
		}
	}

	/* ...of frost. */
	else if (selection < 12) {

		/* Possibly assign an activation for free. */
		if (one_in_(3)) {
			if (potential >= 6000) {
				art->activation = lookup_activation("RAND_COLD3");
			} else if (potential >= 3000) {
				art->activation = lookup_activation("RAND_COLD2");
			} else {
				art->activation = lookup_activation("RAND_COLD1");
			}
		}

		/* Brand the weapon with frost. */
		get_property(art, NULL, "cold brand", 17, true);

		/* Grant either a resist or immunity to frost. */
		if ((potential >= 4500) && one_in_(3)) {
			get_property(art, NULL, "cold resistance", 100, false);
		} else {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}

		/* Sometimes vulnerable to fire */
		if (one_in_(5)) {
			get_property(art, NULL, "fire resistance", -5 * randint1(6), false);
		}
		/* Undead like cold */
		if (one_in_(4) == 1) {
			append_artifact_curse(art, lookup_curse("undead summon"),
								  40 + randint0(20));
			potential += 200;
		}
	}

	/* ...of acid. */
	else if (selection < 18) {

		/* Possibly assign an activation for free. */
		if (one_in_(3)) {
			if (potential >= 6000) {
				art->activation = lookup_activation("RAND_ACID3");
			} else if (potential >= 3000) {
				art->activation = lookup_activation("RAND_ACID2");
			} else {
				art->activation = lookup_activation("RAND_ACID1");
			}
		}

		/* Brand the weapon with acid. */
		get_property(art, NULL, "acid brand", 17, true);
		/* Grant either a resist or immunity to acid. */
		if ((potential >= 4500) && one_in_(3)) {
			get_property(art, NULL, "acid resistance", 100, false);
		} else {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...of electricity. */
	else if (selection < 24) {

		/* Possibly assign an activation for free. */
		if (one_in_(3)) {
			if (potential >= 6000) {
				art->activation = lookup_activation("RAND_ELEC3");
			} else if (potential >= 3000) {
				art->activation = lookup_activation("RAND_ELEC2");
			} else {
				art->activation = lookup_activation("RAND_ELEC1");
			}
		}

		/* Brand the weapon with lightning. */
		get_property(art, NULL, "lightning brand", 17, true);
		/* Grant either a resist or immunity to acid. */
		if ((potential >= 4500) && one_in_(3)) {
			get_property(art, NULL, "electricity resistance", 100, false);
		} else {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
	}

	/* ...of poison. */
	else if (selection < 28) {

		/* Possibly assign an activation for free. */
		if (one_in_(3)) {
			if (potential >= 4500) {
				art->activation = lookup_activation("RAND_POIS2");
			} else {
				art->activation = lookup_activation("RAND_POIS1");
			}
		}

		/* Brand the weapon with poison. */
		get_property(art, NULL, "poison brand", 17, true);
		/* Grant resistance to poison. */
		get_property(art, NULL, "poison resistance", 35 + 5 * randint0(5),
					 true);
		/* Comes with the territory */
		if (one_in_(3)) {
			append_artifact_curse(art, lookup_curse("poison"),
								  40 + randint0(20));
			potential += 300;
		}
	}

	/* ...of life-sustaining. */
	else if (selection < 30) {

		/* This is an exclusive club. */
		if (potential >= 4000) {

			/* Possibly assign an activation for free. */
			if (one_in_(3)) {
				art->activation = lookup_activation("RAND_REGAIN");
			}

			/* Grant hold life. */
			get_property(art, NULL, "hold life", 0, false);
			/* Probably slay evil. */
			if (one_in_(3)) {
				get_property(art, NULL, "slay evil creatures", 15, false);
			}
			/* Possibly resist nether. */
			if (one_in_(3)) {
				get_property(art, NULL, "nether resistance",
							 35 + 5 * randint0(5), true);
			}
			/* Possibly see invisible. */
			if (one_in_(2)) {
				get_property(art, NULL, "see invisible", 0, false);
			}
			/* Possibly slay undead. */
			if (one_in_(3)) {
				get_property(art, NULL, "slay undead", 20, false);
			}
			/* But can leave the user immobile */
			if (one_in_(4)) {
				append_artifact_curse(art, lookup_curse("paralysis"),
									  40 + randint0(20));
				potential += 700;
			}
		}
	}

	/* ...of retain stats. */
	else if (selection < 32) {

		/* This is an exclusive club. */
		if (potential < 3500) {

			/* Possibly assign an activation for free. */
			if (one_in_(3)) {
				art->activation = lookup_activation("RAND_RESTORE");
			}

			/* Grant resist nexus. */
			get_property(art, NULL, "nexus resistance", 35 + 5 * randint0(5),
						 false);
			/* Possibly resist disenchantment. */
			if (one_in_(3)) {
				get_property(art, NULL, "disenchantment resistance",
							 35 + 5 * randint0(5), false);
			}
			/* And some sustains. */
			if (one_in_(2)) {
				get_property(art, NULL, "sustain strength", 0, false);
			}
			if (one_in_(2)) {
				get_property(art, NULL, "sustain intelligence", 0, false);
			}
			if (one_in_(2)) {
				get_property(art, NULL, "sustain wisdom", 0, false);
			}
			if (one_in_(2)) {
				get_property(art, NULL, "sustain dexterity", 0, false);
			}
			if (one_in_(2)) {
				get_property(art, NULL, "sustain constitution", 0, false);
			}
			/* May need to be handled with care */
			if (one_in_(5)) {
				append_artifact_curse(art, lookup_curse("cuts"),
									  40 + randint0(20));
				potential += 100;
			} else if (one_in_(5)) {
				append_artifact_curse(art, lookup_curse("wounds"),
									  40 + randint0(20));
				potential += 200;
			}
		}
	}

	/* ...that shines with holy light. */
	else if (selection < 36) {

		/* Possibly assign an activation for free. */
		if (one_in_(3)) {
			if (potential < 4500) {
				art->activation = lookup_activation("RAND_LIGHT1");
			} else {
				art->activation = lookup_activation("RAND_LIGHT2");
			}
		}

		/* Grant resist light and dark. */
		get_property(art, NULL, "light resistance", 35 + 5 * randint0(5), true);
		get_property(art, NULL, "dark resistance", 35 + 5 * randint0(5), true);
		/* Possibly resist blindness. */
		if (one_in_(2)) {
			get_property(art, NULL, "protection from blindness", 0, false);
		}
	}

	/* ...with plenty of slays. */
	else if (selection < 41) {

		/* Load up on the slays. */
		if (one_in_(6) && (potential >= 2000)) {
			get_property(art, NULL, "slay evil creatures", one_in_(3) ? 17 : 15,
						 true);
			get_property(art, NULL, "slay orcs", one_in_(3) ? 25 : 20, true);
			get_property(art, NULL, "slay trolls", one_in_(3) ? 25 : 20, true);
			get_property(art, NULL, "slay giants", one_in_(3) ? 25 : 20, true);
		} else if (one_in_(5) && (potential >= 2000)) {
			get_property(art, NULL, "slay evil creatures", one_in_(3) ? 17 : 15,
						 true);
			get_property(art, NULL, "slay undead", one_in_(3) ? 25 : 20, true);
			get_property(art, NULL, "slay demons", one_in_(3) ? 25 : 20, true);
		} else {
			if (one_in_(2)) {
				get_property(art, NULL, "slay animals", one_in_(3) ? 20 : 17,
							 true);
			}
			if (one_in_(2)) {
				get_property(art, NULL, "slay demons", one_in_(3) ? 25 : 20,
							 true);
			}
			if (one_in_(4)) {
				get_property(art, NULL, "slay evil creatures",
							 one_in_(3) ? 17 : 15, true);
			}
			if (one_in_(3)) {
				get_property(art, NULL, "slay orcs", one_in_(3) ? 25 : 20, true);
			}
			if (one_in_(3)) {
				get_property(art, NULL, "slay trolls", one_in_(3) ? 25 : 20,
							 true);
			}
			if (one_in_(3)) {
				get_property(art, NULL, "slay demons", one_in_(3) ? 25 : 20,
							 true);
			}
			if (one_in_(3)) {
				get_property(art, NULL, "slay giants", one_in_(3) ? 25 : 20,
							 true);
			}
		}

		/* Occasionally an extra blow */
		if (one_in_(20)) {
			get_property(art, NULL, "extra blows", 1, false);
		}

		/* Sometimes warn the enemy - but with compensation */
		if (one_in_(10)) {
			append_artifact_curse(art, lookup_curse("siren"),
								  40 + randint0(20));
			potential += 200;
		}
	}

	/* ...with enhanced damage dice. */
	else if (selection < 46) {

		/* Use up to a third of the budget for enhancing the damage dice. */
		get_property(art, NULL, "enhanced dice", 3, true);
		/* Not so good for spellcasters */
		if (one_in_(5)) {
			append_artifact_curse(art, lookup_curse("mana drain"),
								  40 + randint0(20));
			potential += 300;
		}
	}

	/* ...that a druid wants by his side. */
	else if (selection < 50) {

		/* Possibly assign an activation for free. */
		if ((randint1(3) != 1) && (potential >= 2500)) {
			art->activation = lookup_activation("RAND_SLOW_FOE");
		}

		/* Provide regenerative powers. */
		get_property(art, NULL, "regeneration", 0, false);
		/* A bonus to stealth. */
		get_property(art, NULL, "stealth", randint1(4), false);
		/* Sometimes vulnerable to sound */
		if (one_in_(5)) {
			get_property(art, NULL, "sound resistance", -5 * randint1(6),
						 false);
		}
	}

	/* ...that is just the thing a mage is looking for. */
	else if (selection < 54) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential >= 2500)) {
			art->activation = lookup_activation("RAND_SLEEP_FOE");
		}

		/* Provide resistance to blindness. */
		get_property(art, NULL, "protection from blindness", 0, false);
		/* Bonus to magic mastery. */
		if (one_in_(2)) {
			get_property(art, NULL, "magic mastery", randint1(4), false);
		}
		/* Sometimes vulnerable to light or dark */
		if (one_in_(8)) {
			get_property(art, NULL, "light resistance", -5 * randint1(6),
						 false);
		} else if (one_in_(8))
			get_property(art, NULL, "dark resistance", -5 * randint1(6), false);
		/* ...but sight can bring fear */
		if (one_in_(4)) {
			append_artifact_curse(art, lookup_curse("cowardice"),
								  40 + randint0(20));
			potential += 100;
		}
	}

	/* ...that a priest prays for. */
	else if (selection < 58) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential >= 2500)) {
			art->activation = lookup_activation("RAND_TURN_FOE");
		}

		/* Provide permanant light. */
		get_property(art, NULL, "light", 1, false);
		/* Bless the weapon. */
		get_property(art, NULL, "blessed melee", 0, true);
		/* Bonus to wisdom. */
		get_property(art, NULL, "wisdom", randint1(4), true);
		/* Sometimes vulnerable to dark */
		if (one_in_(5)) {
			get_property(art, NULL, "dark resistance", -5 * randint1(6), false);
		}
	}

	/* ...that a necromancer would kill for. */
	else if (selection < 62) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential >= 2500)) {
			art->activation = lookup_activation("RAND_CONFU_FOE");
		}

		/* Provide resistance to confusion. */
		get_property(art, NULL, "protection from confusion", 0, false);
		/* Bonus to intelligence. */
		get_property(art, NULL, "intelligence", randint1(4), false);
		/* Sometimes vulnerable to light */
		if (one_in_(5)) {
			get_property(art, NULL, "light resistance", -5 * randint1(6),
						 false);
		}
		/* Comes at a cost */
		if (one_in_(3)) {
			append_artifact_curse(art, lookup_curse("experience drain"),
								  40 + randint0(20));
			potential += 600;
		}
	}

	/* ...twisted with chaos. */
	else if (selection < 65) {

		/* This is an exclusive club. */
		if (potential >= 4000) {

			/* Assign an activation for free. */
			art->activation = lookup_activation("RAND_CHAOS");
			/* Resist chaos and disenchantment. */
			get_property(art, NULL, "chaos resistance",  35 + 5 * randint0(5),
						 true);
			get_property(art, NULL, "disenchantment resistance",
						 35 + 5 * randint0(5), true);
			/* Sometimes vulnerable to shards */
			if (one_in_(5)) {
				get_property(art, NULL, "shards resistance", -5 * randint1(6),
						 false);
			}
			/* Can be unreliable */
			if (one_in_(4)) {
				append_artifact_curse(art, lookup_curse("treacherous weapon"),
								  40 + randint0(20));
				potential += 200;
			}
		}
	}

	/* ...that is a strong champion of order. */
	else if (selection < 68) {

		/* This is an exclusive club. */
		if (potential >= 4000) {

			/* Assign an activation for free. */
			art->activation = lookup_activation("RAND_SHARD_SOUND");
			/* Resist shards, sound, and confusion. */
			get_property(art, NULL, "shards resistance", 35 + 5 * randint0(5),
						 true);
			get_property(art, NULL, "sound resistance", 35 + 5 * randint0(5),
						 true);
			get_property(art, NULL, "protection from confusion", 0, false);
			/* Sometimes vulnerable to chaos and/or disenchantment */
			if (one_in_(8)) {
				get_property(art, NULL, "chaos resistance", -5 * randint1(6),
						 false);
			}
			if (one_in_(8)) {
				get_property(art, NULL, "disenchantment resistance",
							 -5 * randint1(6), false);
			}
		}
	}

	/* ...that smashes foes and dungeon alike. */
	else if (selection < 72) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			art->activation = lookup_activation("RAND_EARTHQUAKE");
		}

		/* Enhance the damage dice. */
		get_property(art, NULL, "enhanced dice", 3, true);
		/* 75% of the time, an earthquake brand.  SJGU but only if the
		 * dice are big enough */
		if ((art->dd * art->ds > 15) && !one_in_(4))
			get_property(art, NULL, "earthquakes", 0, true);
		/* Grant either resistance or immunity to acid. */
		if ((potential >= 3500) && one_in_(3)) {
			get_property(art, NULL, "acid resistance", 100, true);
		} else {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		/* Bonus to tunneling. */
		get_property(art, NULL, "tunneling", randint0(4), false);
		/* Sometimes bonus to strength. */
		if ((potential >= 750) && one_in_(2)) {
			get_property(art, NULL, "strength", randint1(4), false);
		}
		/* Sometimes vulnerable to chaos */
		if (one_in_(8)) {
			get_property(art, NULL, "chaos resistance", -5 * randint1(6),
						 false);
		}
		/* ...and can make you see what isn't there */
		if (one_in_(4)) {
				append_artifact_curse(art, lookup_curse("hallucination"),
									  40 + randint0(20));
				potential += 400;
		}
	}

	/* ...that hunts down all the children of nature. */
	else if (selection < 76) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			art->activation = lookup_activation("RAND_DETECT_MONSTERS");
		}

		/* Naturally, the appropriate slay. */
		get_property(art, NULL, "slay animals", one_in_(2) ? 20 : 17, true);
		/* A pair of survival skills. */
		get_property(art, NULL, "regeneration", 0, false);
		get_property(art, NULL, "feather falling", 0, false);
		/* Sometimes a bonus to infravision. */
		if (one_in_(2)) {
			get_property(art, NULL, "infravision", randint1(4), false);
		}
	}

	/* ...that Orcs and Trolls must fear. */
	else if (selection < 79) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if (potential >= 3000)
				art->activation = lookup_activation("RAND_STARLIGHT");
			else
				art->activation = lookup_activation("RAND_LINE_LIGHT");
		}

		/* Naturally, the appropriate slays. */
		get_property(art, NULL, "slay orcs", one_in_(3) ? 25 : 20, true);
		get_property(art, NULL, "slay trolls", one_in_(3) ? 25 : 20, true);
		/* Often, grant a bonus to ac. */
		if (one_in_(2)) {
			get_property(art, NULL, "armor bonus",
						 randint1(4) + potential / 1000, false);
		}
		/* Sometimes, slay giant. */
		if (one_in_(3)) {
			get_property(art, NULL, "slay giants", one_in_(3) ? 25 : 20, true);
		}
		/* Bonus to strength. */
		get_property(art, NULL, "strength", randint1(4), false);
	}

	/* ...that the undead cannot withstand. */
	else if (selection < 82) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential > 2500)) {
			if (potential < 5000) {
				art->activation = lookup_activation("RAND_SMITE_UNDEAD");
			} else {
				art->activation = lookup_activation("RAND_DISPEL_UNDEAD");
			}
		}

		/* Grant slay undead and see invisible. */
		get_property(art, NULL, "slay undead", one_in_(3) ? 25 : 20, true);
		get_property(art, NULL, "see invisible", 0, true);
		/* Sometimes, hold life. */
		if (one_in_(3)) {
			get_property(art, NULL, "hold life", 0, false);
		}
		/* Bonus to wisdom. */
		get_property(art, NULL, "wisdom", randint1(4), false);
		/* Sometimes vulnerable to light */
		if (one_in_(5)) {
			get_property(art, NULL, "light resistance", -5 * randint1(6),
						 false);
		}
	}

	/* ...that evil creatures everywhere flee from. */
	else if (selection < 90) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if ((potential >= 5000) && one_in_(2)) {
				art->activation = lookup_activation("RAND_DISPEL_EVIL");
			} else if (potential >= 5000) {
				art->activation = lookup_activation("RAND_BANISH_EVIL");
			} else if ((potential >= 3500) && !one_in_(3)) {
				art->activation = lookup_activation("RAND_HOLY_ORB");
			} else if (potential >= 2000) {
				art->activation = lookup_activation("RAND_PROT_FROM_EVIL");
			} else {
				art->activation = lookup_activation("RAND_DETECT_EVIL");
			}
		}

		/* Naturally, the appropriate slay. */
		get_property(art, NULL, "slay evil creatures", one_in_(3) ? 17 : 15,
					 true);
		/* Bless the weapon. */
		get_property(art, NULL, "blessed melee", 0, false);
		/* Sometimes, resist nether. */
		if (one_in_(6)) {
			get_property(art, NULL, "nether resistance", 35 + 5 * randint0(5),
						 false);
		}
		/* Sometimes, resist dark. */
		if (one_in_(6)) {
			get_property(art, NULL, "dark resistance", 35 + 5 * randint0(5),
						 false);
		}
		/* Possibly bonus to intelligence. */
		if (one_in_(3)) {
			get_property(art, NULL, "intelligence", randint1(4), false);
		}
	}

	/* ...that demons intensely hate. */
	else if (selection < 93) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential > 2500)) {
			art->activation = lookup_activation("RAND_SMITE_DEMON");
		}

		/* Naturally, the appropriate slay. */
		get_property(art, NULL, "slay demons", one_in_(3) ? 25 : 20,	true);
		/* Sometimes, nip the spawn of hell with cold as well. */
		if (one_in_(2)) {
			get_property(art, NULL, "cold brand", 17, false);
		}
		/* Grant resistance to fire. */
		get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5), false);
		/* Bonus to dexerity. */
		get_property(art, NULL, "dexterity", randint1(4), false);
		/* Sometimes vulnerable to cold */
		if (one_in_(5)) {
			get_property(art, NULL, "cold resistance", -5 * randint1(6),
						 false);
		}
	}

	/* ...that dragons long to destroy. */
	else if (selection < 96) {
		int temp = randint1(5);

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential > 2500)) {
			art->activation = lookup_activation("RAND_SMITE_DRAGON");
		}

		/* Naturally, the appropriate slay. */
		get_property(art, NULL, "slay dragons", one_in_(3) ? 25 : 20, true);
		/* And one of the five elemental brands. */
		if (temp == 1) {
			get_property(art, NULL, "acid brand", 17, false);
		}
		if (temp == 2) {
			get_property(art, NULL, "lightning brand", 17, false);
		}
		if (temp == 3) {
			get_property(art, NULL, "fire brand", 17, false);
		}
		if (temp == 4) {
			get_property(art, NULL, "cold brand", 17, false);
		}
		if (temp == 5) {
			get_property(art, NULL, "poison brand", 17, false);
		}
		/* Bonus to constitution. */
		get_property(art, NULL, "constitution", randint1(4), false);
	}

	/* ...that protects the wielder. */
	else {

		/* This is an exclusive club. */
		if (potential < 2500) {

			/* Possibly assign an activation for free. */
			if (!one_in_(3)) {
				if (potential >= 5000) {
					art->activation = lookup_activation("RAND_SHIELD");
				} else {
					art->activation = lookup_activation("RAND_BLESS");
				}
			}

			/* Grant a bonus to armour class. */
			get_property(art, NULL, "armor bonus",
						 randint1(6) + potential / 800, true);
			/* And the four basic resists. */
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
			/* And some of the survival abilities. */
			if (!one_in_(4)) {
				get_property(art, NULL, "see invisible", 0, true);
			}
			if (!one_in_(4)) {
				get_property(art, NULL, "feather falling", 0, true);
			}
			if (!one_in_(4)) {
				get_property(art, NULL, "free action", 0, true);
			}
			if (!one_in_(4)) {
				get_property(art, NULL, "regeneration", 0, true);
			}
		}
	}

	/* HACK SJGU Any small dice weapon with big potential should get at 
	 * least some dice upgrade */
	if (((art->dd * art->ds) < 9) && (initial_potential >= 3500)) {
		get_property(art, NULL, "enhanced dice", 3, true);
	}

	/* Any melee weapon has a chance at an extra blow */
	if (one_in_(40)) {
		get_property(art, NULL, "extra blows", 1, false);
	}
}

/**
 * Pick an initial set of qualities for a missile launcher, based on a theme.
 * Also add a bonus to Skill and Deadliness.
 */
static void choose_launcher_theme(struct artifact *art)
{
	/* I'm a missile weapon... */
	int selection = randint0(100);

	/* ...with bonuses to Deadliness and Skill, and... */
	art->to_d += (3 + randint1(9) + potential / 650);
	art->to_h += (3 + randint1(9) + potential / 650);

	/* ...of extra shots. */
	if (selection < 50) {
		get_property(art, NULL, "shooting speed", 1, true);
		/* Sometimes also grant extra might. */
		if ((potential >= 2000) && one_in_(3)) {
			get_property(art, NULL, "shooting power", 1, false);
		}
	}

	/* ...of extra might. */
	else {
		if ((potential >= 4000) && one_in_(2)) {
			get_property(art, NULL, "shooting power", 2, true);
		} else {
			get_property(art, NULL, "shooting power", 1, true);
		}
		/* Sometimes also grant extra shots. */
		if ((potential >= 2250) && one_in_(3)) {
			get_property(art, NULL, "shooting speed", 1, false);
		}
	}

	/* Sometimes, assign an activation. */
	if (potential > randint1(5000) && !one_in_(3)) {
		if (one_in_(2)) {
			art->activation = lookup_activation("RAND_SUPER_SHOOTING");
		} else {
			art->activation = lookup_activation("RAND_BRAND_MISSILE");
		}
	}

	/* Hack - avoid boring bows. */
	if (potential < 1000) {
		potential = 1000;
	}
}

/**
 * Pick an initial set of qualities for a suit of armor, based on a theme.
 * Also add a bonus to armour class.
 */
static void choose_armor_theme(struct artifact *art)
{
	/* I'm a piece of body armour... */
	int selection = randint0(100);

	/* ...with a bonus to armour class, and... */
	art->to_a += (10 + randint1(8) + potential / 1500);

	/* ...that resists most or all of the elements. */
	if (selection < 30) {

		/* Possibly assign an activation for free. */
		if (one_in_(2) && (potential >= 3000)) {
			if (potential >= 5500) {
				art->activation = lookup_activation("RAND_RESIST_ALL");
			} else {
				art->activation = lookup_activation("RAND_RESIST_ELEMENTS");
			}
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
		/* Sometimes, also poison. */
		if (one_in_(2) && (potential >= 2000))
			get_property(art, NULL, "poison resistance", 35 + 5 * randint0(5),
						 false);
	}

	/* ...that protects the very soul. */
	else if (selection < 40) {

		/* This is an exclusive club. */
		if (potential >= 5500) {

			/* Possibly assign an activation for free. */
			if (!one_in_(3)) {
				if (one_in_(2)) {
					art->activation = lookup_activation("RAND_HEAL3");
				} else {
					art->activation = lookup_activation("RAND_REGAIN");
				}
			}

			/* Resist nether and hold life. */
			get_property(art, NULL, "nether resistance", 35 + 5 * randint0(5),
						 false);
			get_property(art, NULL, "hold life", 0, false);
			/* Sometimes also resist chaos. */
			if (one_in_(2)) {
				get_property(art, NULL, "chaos resistance",
							 35 + 5 * randint0(5), false);
			}
		}

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...resistant to sound and confusion. */
	else if (selection < 50) {

		/* Resist sound and confusion. */
		get_property(art, NULL, "sound resistance", 35 + 5 * randint0(5),
					 false);
		get_property(art, NULL, "protection from confusion", 0, false);

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...with an amazing armour class. */
	else if (selection < 62) {

		/* Possibly assign an activation for free. */
		if (one_in_(3) && (potential >= 4000)) {
			art->activation = lookup_activation("RAND_SHIELD");
		}

		/* Increase both base and magical ac. */
		get_property(art, NULL, "armor bonus", randint1(3) + potential / 1600,
							true);
		get_property(art, NULL, "extra armor", randint1(3) + potential / 1600,
					 true);

		/* Probably damage reduction */
		if (!one_in_(3)) {
			get_property(art, NULL, "damage reduction", 5 + randint1(10), true);
		}

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
		/* Sometimes cost regenarative power */
		if (one_in_(6)) {
			append_artifact_curse(art, lookup_curse("impair hitpoint recovery"),
								  40 + randint0(20));
			potential += 100;
		}
	}

	/* ...that grants power over the realms of shadow. */
	else if (selection < 74) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if (one_in_(2)) {
				art->activation = lookup_activation("RAND_TELEPORT1");
			} else {
				art->activation = lookup_activation("RAND_BLESS");
			}
		}

		/* Resist dark. */
		get_property(art, NULL, "dark resistance", 35 + 5 * randint0(5), true);
		/* Bonus to stealth. */
		get_property(art, NULL, "stealth", randint1(4), true);
		/* Grant see invisible. */
		get_property(art, NULL, "see invisible", 0, true);
		/* Possibly resist nether. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "nether resistance", 35 + 5 * randint0(5),
						 true);
		}

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...that protects against poison. */
	else if (selection < 82) {

		/* This is an exclusive club. */
		if (potential >= 2500) {

			/* Possibly assign an activation for free. */
			if (!one_in_(3)) {
				art->activation = lookup_activation("RAND_CURE");
			}

			/* Resist poison. */
			get_property(art, NULL, "poison resistance", 35 + 5 * randint0(5),
						 true);
		}

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...that shines very brightly. */
	else if (selection < 95) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if (potential < 4500) {
				art->activation = lookup_activation("RAND_LIGHT1");
			} else {
				art->activation = lookup_activation("RAND_LIGHT2");
			}
		}

		/* Grant permanant light. */
		get_property(art, NULL, "light", 1, true);
		/* And resistance to light. */
		get_property(art, NULL, "light resistance", 35 + 5 * randint0(5),
					 true);

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...that contains the very essence of... */
	else if (selection < 100) {

		/* This is an exclusive club. */
		if (potential >= 5500) {
			int temp = randint1(4);
			/* ...fire. */
			if (temp == 1) {

				/* Assign an activation for free. */
				if (!one_in_(3)) {
					art->activation = lookup_activation("RAND_FIRE3");
				}

				/* Immunity. */
				get_property(art, NULL, "fire resistance", 100, true);
				/* Demons like fire */
				if (one_in_(4)) {
					append_artifact_curse(art, lookup_curse("demon summon"),
										  40 + randint0(20));
					potential += 100;
				}
			}

			/* ...cold. */
			if (temp == 2) {

				/* Assign an activation for free. */
				if (!one_in_(3)) {
					art->activation = lookup_activation("RAND_COLD3");
				}

				/* Immunity. */
				get_property(art, NULL, "cold resistance", 100, true);
				/* Undead like cold */
				if (one_in_(4)) {
					append_artifact_curse(art, lookup_curse("undead summon"),
										  40 + randint0(20));
					potential += 100;
				}
			}

			/* ...acid. */
			if (temp == 3) {

				/* Assign an activation for free. */
				if (!one_in_(3)) {
					art->activation = lookup_activation("RAND_ACID3");
				}

				/* Immunity. */
				get_property(art, NULL, "acid resistance", 100, true);
			}

			/* ...electricity. */
			if (temp == 4) {

				/* Assign an activation for free. */
				if (!one_in_(3)) {
					art->activation = lookup_activation("RAND_ELEC3");
				}

				/* Immunity. */
				get_property(art, NULL, "electricity resistance", 100, true);
			}
		}

		/* Collect a suite of basic resists. */
		if (randint1(5) < 3) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (randint1(5) < 3) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}

		/* SJGU Hack -- Dragon armour has SPEED */
		if (art->tval == TV_DRAG_ARMOR) {

			/* Should this be for free? */
			get_property(art, NULL, "speed", potential / 2500 + randint1(2),
							false);
		}
	}
}

/**
 * Pick an initial set of qualities for a shield, based on a theme.
 * Also add a bonus to armour class.
 */
static void choose_shield_theme(struct artifact *art)
{
	/* I'm a shield... */
	int selection = randint0(100);

	/* ...with a bonus to armour class, and... */
	art->to_a += (8 + randint1(8) + potential / 1500);

	/* ...that resists most or all of the elements. */
	if (selection < 18) {

		/* Possibly assign an activation for free. */
		if ((one_in_(3)) && (potential >= 3000)) {
			if (potential >= 5500) {
				art->activation = lookup_activation("RAND_RESIST_ALL");
			} else {
				art->activation = lookup_activation("RAND_RESIST_ELEMENTS");
			}
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "acid resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "fire resistance", 35 + 5 * randint0(5),
						 true);
		}
		if (!one_in_(5)) {
			get_property(art, NULL, "cold resistance", 35 + 5 * randint0(5),
						 true);
		}
	}

	/* ...that increases strength and constitution. */
	else if (selection < 30) {

		/* Bonus to constitution. */
		get_property(art, NULL, "constitution", randint1(4), false);
		/* Bonus to strength. */
		get_property(art, NULL, "strength", randint1(4), false);
	}

	/* ...that resists shards (+ to ac). */
	else if (selection < 45) {

		/* Resist shards and increase base ac. */
		get_property(art, NULL, "shards resistance", 35 + 5 * randint0(5),
					 true);
		get_property(art, NULL, "extra armor", randint1(3) + potential / 1200,
					 true);
	}

	/* ...that resists light and dark. */
	else if (selection < 55) {

		/* Grant resistance to light and dark. */
		get_property(art, NULL, "light resistance", 35 + 5 * randint0(5),
					 true);
		get_property(art, NULL, "dark resistance", 35 + 5 * randint0(5),
					 true);
	}

	/* ...with runes of protection. */
	else if (selection < 85) {

		/* Possibly assign an activation for free. */
		if (one_in_(3) && (potential >= 4000)) {
			art->activation = lookup_activation("RAND_SHIELD");
		}

		/* Increase magical ac. */
		get_property(art, NULL, "armor bonus", randint1(6) + potential / 2000,
							true);
		/* Randomly add some survival-enhancing magics. */
		if (one_in_(4)) {
			get_property(art, NULL, "regeneration", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "feather falling", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "hold life", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "see invisible", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "free action", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "protection from blindness", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "protection from confusion", 0, false);
		}
		if (one_in_(4)) {
			get_property(art, NULL, "protection from stunning", 0, false);
		}
	}

	/* ...with immunity. */
	else if (selection < 100) {

		/* This is an exclusive club. */
		if (potential >= 5000) {
			int temp = randint1(4);
			/* ...fire. */
			if (temp == 1) {

				/* Immunity. */
				get_property(art, NULL, "fire resistance", 100, true);
			}

			/* ...cold. */
			if (temp == 2) {

				/* Immunity. */
				get_property(art, NULL, "cold resistance", 100, true);
			}

			/* ...acid. */
			if (temp == 3) {

				/* Immunity. */
				get_property(art, NULL, "acid resistance", 100, true);
			}

			/* ...electricity. */
			if (temp == 4) {

				/* Immunity. */
				get_property(art, NULL, "electricity resistance", 100, true);
			}
		}
	}
}

/**
 * Pick an initial set of qualities for a pair of boots, based on a theme.
 * Also add a bonus to armour class.
 */
static void choose_boots_theme(struct artifact *art)
{
	/* I'm a pair of boots... */
	int selection = randint0(100);

	/* ...with a bonus to armour class, and... */
	art->to_a += (6 + randint1(5) + potential / 1500);

	/* ...that makes he who wears me run like the wind. */
	if (selection < 25) {

		/* This is an exclusive club. */
		if (potential >= 4500) {

			/* Add a speed bonus immediately. */
			get_property(art, NULL, "speed", 5 + damroll(3, 2), true);
		}
	}

	/* ...that keeps a guy's feet on the ground. */
	else if (selection < 35) {

		/* Resist nexus and feather fall. */
		get_property(art, NULL, "nexus resistance", 35 + 5 * randint0(5), true);
		get_property(art, NULL, "feather falling", 0, true);

		/* Sometimes too much */
		if (one_in_(5)) {
			append_artifact_curse(art, lookup_curse("anti-teleportation"),
								  40 + randint0(20));
			potential += 300;
		}
	}

	/* ...with unrivaled magical movement. */
	else if (selection < 50) {

		/* Assign an activation,... */
		if (potential >= 4000) {
			art->activation = lookup_activation("RAND_RECALL");
		} else if (potential >= 2000) {
			art->activation = lookup_activation("RAND_TELEPORT2");
		} else {
			art->activation = lookup_activation("RAND_TELEPORT1");
		}

		/* ...but not for free. */
		potential = 2 * potential / 3;

		/* Resist nexus and immunity to traps */
		get_property(art, NULL, "nexus resistance", 35 + 5 * randint0(5), true);
		get_property(art, NULL, "trap immunity", 0, true);

		/* Possibly bonus to dexerity. */
		if (potential >= 1000)
			get_property(art, NULL, "dexterity", randint1(4), false);

		/* Sometimes too powerful */
		if (one_in_(5)) {
			append_artifact_curse(art, lookup_curse("teleportation"),
								  40 + randint0(20));
			potential += 100;
		}
	}

	/* ...that makes long marches easy. */
	else if (selection < 60) {

		/* Possibly assign an activation for free. */
		if (one_in_(2)) {
			if (potential >= 5500) {
				art->activation = lookup_activation("RAND_HEAL3");
			} else if (potential >= 3000) {
				art->activation = lookup_activation("RAND_HEAL2");
			} else {
				art->activation = lookup_activation("RAND_HEAL1");
			}
		}

		/* Grant regenerative powers. */
		get_property(art, NULL, "regeneration", 0, true);

		/* Extra moves */
		if (one_in_(4)) {
			get_property(art, NULL, "extra moves", 2, true);
		} else {
			get_property(art, NULL, "extra moves", 1, true);
		}

		/* Bonus to constitution. */
		get_property(art, NULL, "constitution", randint1(4), false);
	}

	/* ...that dance up a storm. */
	else if (selection < 70) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3) && (potential >= 2500)) {
			art->activation = lookup_activation("RAND_STORM_DANCE");
		}

		/* Grant feather fall. */
		get_property(art, NULL, "feather falling", 0, true);

		/* Bonus to dexterity. */
		get_property(art, NULL, "dexterity", randint1(4), false);
	}

	/* ...worn by a famous ranger of old. */
	else if (selection < 80) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			art->activation = lookup_activation("RAND_DETECT_MONSTERS");
		}

		/* Grant regeneration and slow digest. */
		get_property(art, NULL, "regeneration", 0, true);
		get_property(art, NULL, "slow digestion", 0, true);

		/* Possibly telepathy. */
		if (one_in_(6)) {
			get_property(art, NULL, "telepathy", 0, false);
		}

		/* Bonus to stealth. */
		get_property(art, NULL, "stealth", randint1(4), false);
	}

	/* Possibly assign a speed activation for free. */
	if (one_in_(4) && (!art->activation) && (potential >= 2000)) {
		art->activation = lookup_activation("RAND_SPEED");
	}
}

/**
 * Pick an initial set of qualities for a cloak, based on a theme.
 * Also add a bonus to armour class.
 */
static void choose_cloak_theme(struct artifact *art)
{
	/* I'm a cloak... */
	int selection = randint0(100);

	/* ...with a bonus to armour class, and... */
	art->to_a += (8 + randint1(6) + potential / 1500);

	/* ...that hides the wearer from hostile eyes. */
	if (selection < 20) {

		/* Possibly assign an activation for free. */
		if (one_in_(2)) {
			art->activation = lookup_activation("RAND_SLEEP_FOES");
		}

		/* Bonus to stealth. */
		get_property(art, NULL, "stealth", randint1(4), false);
	}

	/* ...that confuses and dismays foes. */
	else if (selection < 30) {

		/* Possibly assign an activation for free. */
		if (one_in_(2)) {
			if (one_in_(2)) {
				art->activation = lookup_activation("RAND_CONFU_FOES");
			} else {
				art->activation = lookup_activation("RAND_TURN_FOES");
			}
		}
	}

	/* ...with amazing protective powers. */
	else if (selection < 40) {

		/* Possibly assign an activation for free. */
		if ((one_in_(4)) && (potential >= 3000)) {
			art->activation = lookup_activation("RAND_SHIELD");
		}

		/* Increase both base and magical ac. */
		get_property(art, NULL, "armor bonus", randint1(3) + potential / 1600,
					 true);
		get_property(art, NULL, "extra armor", randint1(3) + potential / 1600,
					 true);

		/* Perhaps demage reduction */
		if (one_in_(3)) {
			get_property(art, NULL, "damage reduction", 3 + randint1(5), true);
		}
	}

	/* ...that unmasks the locations of foes. */
	else if (selection < 50) {

		/* Assign an activation for free. */
		if (one_in_(2)) {
			art->activation = lookup_activation("RAND_DETECT_MONSTERS");
		} else {
			art->activation = lookup_activation("RAND_DETECT_EVIL");
		}
	}

	/* ...that is aware of the world around. */
	else if (selection < 60) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if ((potential >= 4000) && !one_in_(3)) {
				art->activation = lookup_activation("RAND_DETECT_ALL");
			} else if (potential >= 3000) {
				art->activation = lookup_activation("RAND_MAGIC_MAP");
			} else {
				art->activation = lookup_activation("RAND_DETECT_D_S_T");
			}
		}

		/* Bonus to searching. */
		get_property(art, NULL, "searching skill", randint1(4), false);
	}

	/* ...of necromantic powers. */
	else if (selection < 65) {
		int temp = randint1(3);

		/* Possibly assign an activation for free. */
		if (one_in_(2)) {
			art->activation = lookup_activation("RAND_SLOW_FOES");
		}

		/* Resist chaos, dark, or nether */
		if (temp == 1) {
			get_property(art, NULL, "chaos resistance", 35 + 5 * randint0(5),
						 false);
		} else if (temp == 2) {
			get_property(art, NULL, "nether resistance", 35 + 5 * randint0(5),
						 false);
		} else if (temp == 3) {
			get_property(art, NULL, "dark resistance", 35 + 5 * randint0(5),
						 false);
		}

		/* Bonus to infravision. */
		get_property(art, NULL, "infravision", randint1(4), false);

		/* Costs the device user */
		if (one_in_(6)) {
			get_property(art, NULL, "magic mastery", 0 - randint1(2), false);
		}
	}

	/* Hack -- Elven Cloaks have STEALTH */
	if (art->sval == lookup_sval(TV_CLOAK, "Elven Cloak")) {
		/* Should this be for free? */
		art->modifiers[OBJ_MOD_STEALTH] += potential / 2500 + randint1(2);
	}
}

/**
 * Pick an initial set of qualities for a helm or crown, based on a theme.
 * Also add a bonus to armour class, Skill, and Deadliness.
 */
static void choose_hat_theme(struct artifact *art)
{
	/* I'm a helm or crown... */
	int selection = randint0(100);

	/* ...with a bonus to armour class, and... */
	art->to_a += (8 + randint1(6) + potential / 1500);

	/* ...of telepathy. */
	if (selection < 24) {

		/* This is an exclusive club. */
		if (potential < 3500) {
			/* Grant telepathy. */
			get_property(art, NULL, "telepathy", 0, false);
		}
	}

	/* ...that maintains serenity amid uncertainty. */
	else if (selection < 35) {

		/* Possibly assign an activation for free. */
		if (one_in_(2) && (potential >= 4500)) {
			art->activation = lookup_activation("RAND_CURE");
		}

		/* Grant resistance to confusion and sound. */
		get_property(art, NULL, "protection from confusion", 0, false);
		get_property(art, NULL, "sound resistance", 35 + 5 * randint0(5),
					 false);
	}

	/* ...preservative of sight and awareness. */
	else if (selection < 46) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if (potential >= 3750) {
				art->activation = lookup_activation("RAND_DETECT_ALL");
			} else {
				art->activation = lookup_activation("RAND_DETECT_D_S_T");
			}
		}

		/* Grant resistance to blindness, see invisible and maybe dark vision */
		get_property(art, NULL, "protection from blindness", 0, false);
		get_property(art, NULL, "see invisible", 0, false);
		if (one_in_(2)) {
			get_property(art, NULL, "darkness", 0, false);
		}

		/* Bonus to searching. */
		get_property(art, NULL, "searching skill", randint1(4), false);
	}

	/* ...whose wearer stands taller in the eyes of men. */
	else if (selection < 57) {

		/* Bonus to wisdom. */
		get_property(art, NULL, "wisdom", randint1(4), false);

		/* Possibly add a bonus to base ac. */
		if (one_in_(3)) {
			get_property(art, NULL, "extra armor", randint1(2) + 2,	false);
		}
	}

	/* ...strong in battle. */
	else if (selection < 68) {

		/* Possibly assign an activation for free. */
		if ((potential >= 2500) && !one_in_(3)) {
			art->activation = lookup_activation("RAND_FRIGHTEN_ALL");
		} else {
			art->activation = lookup_activation("RAND_HEROISM");
		}

		/* No fear. */
		get_property(art, NULL, "protection from fear", 0, false);
		/* Bonus to strength. */
		get_property(art, NULL, "strength", randint1(4), false);
		/* Bonus to dexterity. */
		get_property(art, NULL, "dexterity", randint1(4), false);
		/* Sustain dexterity and strength. */
		get_property(art, NULL, "sustain dexterity", 0, true);
		get_property(art, NULL, "sustain strength", 0, true);
	}

	/* ...to whose wearer is revealed many secrets. */
	else if (selection < 79) {

		/* This is an exclusive club. */
		if (potential < 2500) {

			/* Assign an activation for free. */
			art->activation = lookup_activation("RAND_IDENTIFY");
		}
	}

	/* ...which can focus healing magics. */
	else if (selection < 90) {

		/* Possibly assign an activation for free. */
		if (!one_in_(3)) {
			if (potential >= 4000) {
				art->activation = lookup_activation("RAND_HEAL3");
			} else if (potential >= 2500) {
				art->activation = lookup_activation("RAND_HEAL2");
			} else {
				art->activation = lookup_activation("RAND_HEAL1");
			}

			/* Grant regeneration. */
			get_property(art, NULL, "regeneration", 0, false);
		}
	}
}

/**
 * Pick an initial set of qualities for a pair of gloves, based on a theme.
 * Also add a bonus to armour class, Skill, and Deadliness.
 */
static void choose_gloves_theme(struct artifact *art)
{
	/* I'm a pair of gloves... */
	int i, selection = randint0(100);

	/* ...with a bonus to armour class, and... */
	art->to_a += (7 + randint1(5) + potential / 1500);

	/* ...that grant increased combat prowess. */
	if (selection < 30) {

		/* Grant equal bonuses to Skill and Deadliness. */
		int temp = 6 + randint1(4);
		get_property(art, NULL, "skill bonus", temp, true);
		get_property(art, NULL, "deadliness bonus", temp, true);

		/* Often, acquire free action. */
		if (!one_in_(3)) {
			get_property(art, NULL, "free action", 0, true);
		}

		/* Sometimes negative to stealth */
		if (one_in_(6)) {
			get_property(art, NULL, "stealth", 0 - randint1(3), true);
		}
	}

	/* ...with the dauntless spirit of a mighty warrior. */
	else if (selection < 45) {

		/* No fear. */
		get_property(art, NULL, "protection from fear", 0, true);

		/* Often, grant regeneration. */
		if (!one_in_(3)) {
			get_property(art, NULL, "regeneration", 0, false);
		}

		/* Sometimes, acquire free action. */
		if (one_in_(2)) {
			get_property(art, NULL, "free action", 0, false);
		}

		/* Bonus to one of the combat stats. */
		if (one_in_(3)) {
			get_property(art, NULL, "strength", randint1(4), false);
		} else if (one_in_(2)) {
			get_property(art, NULL, "dexterity", randint1(4), false);
		} else {
			get_property(art, NULL, "constitution", randint1(4), false);
		}

		/* Possibly grant equal bonuses to Skill and Deadliness. */
		if ((potential >= 1500) && !one_in_(4)) {
			int temp = 4 + randint1(4);
			get_property(art, NULL, "deadliness bonus", temp, false);
			get_property(art, NULL, "skill bonus", temp, false);
		}

		/* Sometimes vulnerable to poison */
		if (one_in_(6)) {
			get_property(art, NULL, "poison resistance", -5 * randint1(6),
						 false);
		}
	}

	/* ...able to protect the wearer. */
	else if (selection < 60) {

		/* Increase bonus to AC. */
		get_property(art, NULL, "armor bonus", 4 + randint1(4), false);

		/* Grant (usually) two basic resists. */
		for (i = 0; i < 2; i++) {
			int temp = randint1(4);
			if (temp == 1) {
				get_property(art, NULL, "fire resistance",
							 35 + 5 * randint0(5), true);
			}
			if (temp == 2) {
				get_property(art, NULL, "cold resistance",
							 35 + 5 * randint0(5), true);
			}
			if (temp == 3) {
				get_property(art, NULL, "acid resistance",
							 35 + 5 * randint0(5), true);
			}
			if (temp == 4) {
				get_property(art, NULL, "electricity resistance",
							 35 + 5 * randint0(5), true);
			}

			/* Sometimes add a bolt activation for free. */
			if ((potential >= 500) && !one_in_(3)) {
				if (temp == 1) {
					art->activation = lookup_activation("RAND_FIRE1");
				}
				if (temp == 2) {
					art->activation = lookup_activation("RAND_COLD1");
				}
				if (temp == 3) {
					art->activation = lookup_activation("RAND_ACID1");
				}
				if (temp == 4) {
					art->activation = lookup_activation("RAND_ELEC1");
				}
			}
		}
	}

	/* ...with the cunning of a rogue of legend. */
	else if (selection < 75) {

		/* Assign an activation for free. */
		art->activation = lookup_activation("RAND_DISARM");

		/* Bonus to stealth. */
		get_property(art, NULL, "stealth", randint1(4), false);

		/* Bonus to searching. */
		get_property(art, NULL, "searching skill", randint1(4), false);

		/* Often, acquire free action. */
		if (!one_in_(3)) {
			get_property(art, NULL, "free action", 0, false);
		}

		/* Sometimes vulnerable to sound */
		if (one_in_(5)) {
			get_property(art, NULL, "sound resistance", -5 * randint1(6),
						 false);
		}
	}

	/* ...that untangles magical conundrums. */
	else if (selection < 90) {

		/* Mark the gloves for a later bonus to magic item mastery. */
		get_property(art, NULL, "magic mastery", randint1(4), false);

		/* Trap immunity */
		get_property(art, NULL, "trap immunity", 0, false);
	}

	/* ...with a deadly mastery of archery. */
	else if (selection < 100) {
		int temp;

		/* This is an exclusive club. */
		if (potential < 3500) {

			/* Always grant an activation, but not for free. */
			art->activation = lookup_activation("RAND_SUPER_SHOOTING");
			potential -= 750;

			/* Add equal bonuses to Skill and to Deadliness. */
			temp = 5 + randint1(5);
			get_property(art, NULL, "deadliness bonus", temp, false);
			get_property(art, NULL, "skill bonus", temp, false);

			/* Often, acquire free action. */
			if (!one_in_(3)) {
				get_property(art, NULL, "free action", 0, false);
			}
		}
	}
}

/**
 * Pick an initial set of qualities, based on a theme.  Also add a bonus to 
 * armour class, Skill, and Deadliness.
 */
static void choose_artifact_theme(struct artifact *art)
{
	struct object_kind *kind = lookup_kind(art->tval, art->sval);

	/* Possibly make very powerful artifacts cursed. */
	if ((potential > 6000) && (potential > randint1(30000))) {
		int max_tries;
		for (max_tries = 3; max_tries; max_tries--) {
			int pick = randint1(z_info->curse_max - 1);
			int power = randint1(9) + 10 * m_bonus(9, art->alloc_min);
			if (!curses[pick].poss[art->tval]) continue;
			append_artifact_curse(art, pick, power);
		}
	}

	/* Frequently (but not always) assign a basic theme to the artifact. */
	switch (kind->tval) {
		case TV_SWORD:
		case TV_POLEARM:
		case TV_HAFTED: choose_melee_weapon_theme(art); break;
		case TV_BOW: choose_launcher_theme(art); break;
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR: choose_armor_theme(art); break;
		case TV_SHIELD: choose_shield_theme(art); break;
		case TV_BOOTS: choose_boots_theme(art); break;
		case TV_CLOAK: choose_cloak_theme(art); break;
		case TV_HELM:
		case TV_CROWN: choose_hat_theme(art); break;
		case TV_GLOVES: choose_gloves_theme(art); break;
		default: msg("error -- object kind not recognized."); break;
	}

	/* It is possible for artifacts to be unusually heavy or light. */
	if (one_in_(6)) {
		int old_weight = art->weight;
		if (one_in_(2) && (art->weight >= 30)) {
			/* Sometimes, they are unusually heavy. */
			art->weight += randint1(art->weight / 10) * 5 + 5;
			potential += (art->weight - old_weight) * 10;
		} else if (art->weight >= 50) {
			/* Sometimes, they are unusually light. */
			art->weight -= randint1(art->weight / 10) * 5 + 5;
			potential -= (old_weight - art->weight) * 10;
			potential = MAX(potential, 0);
		}
	}
}

/**
 * Grant extra abilities, until object's units of exchange are all used up.  
 * This function can be quite random - indeed needs to be - because of all 
 * the possible themed random artifacts.
 */
static void haggle_till_done(struct artifact *art, struct object *obj)
{
	struct object_kind *kind = art ? lookup_kind(art->tval, art->sval) :
		lookup_kind(obj->tval, obj->sval);
	int i;
	int bonus = 0;
	int choice = 0;
	int limit = 20;
	bool weapon = tval_is_melee_weapon_k(kind);
	bool launcher = tval_is_launcher_k(kind);

	/* Initial bonuses */
	if ((one_in_(6)) && (potential >= 4500)) {
		/* Rarely, and only if the artifact has a lot of potential, add all the
		 * stats. */
		get_property(art, obj, "strength", 1, true);
		get_property(art, obj, "wisdom", 1, true);
		get_property(art, obj, "intelligence", 1, true);
		get_property(art, obj, "dexterity", 1, true);
		get_property(art, obj, "constitution", 1, true);
	} else if (!one_in_(3) && (potential >= 750)) {
		/* Otherwise, if at least some potential remains, add a numerical bonus
		 * quality or two with 67% probability. */
		int rounds = randint1(2);
		bonus = potential / 2000 + randint1(2);

		for (i = 0; i < rounds; i++) {
			/* Only melee weapons can get a bonus to tunnelling. */
			if (weapon) {
				choice = randint1(10);
			} else {
				choice = randint1(9);
			}
			switch (choice) {
				case 1: get_property(art, obj, "strength", bonus, true); break;
				case 2: get_property(art, obj, "wisdom", bonus, true); break;
				case 3: get_property(art, obj, "intelligence", bonus, true);
					break;
				case 4: get_property(art, obj, "dexterity", bonus, true);
					break;
				case 5: get_property(art, obj, "constitution", bonus, true);
					break;
				case 6: get_property(art, obj, "infravision", bonus, true);
					break;
				case 7: get_property(art, obj, "stealth", bonus, true); break;
				case 8: get_property(art, obj, "searching skill", bonus, true);
					break;
				case 9: get_property(art, obj, "speed", bonus, true); break;
				case 10: get_property(art, obj, "tunneling", bonus, true);
					break;
				default: break;
			}
		}
	}

	/* Sometimes, collect a vulnerability in exchange for more potential */
	if (one_in_(8)) {
		choice = randint1(13);
		if ((choice == 1) && !has_property(art, obj, "acid resistance")) {
			get_property(art, obj, "acid resistance", -5 * randint1(6), true);
		}
		if ((choice == 2) && !has_property(art, obj, "electricity resistance")){
			get_property(art, obj, "electricity resistance", -5 * randint1(6),
						 true);
		}
		if ((choice == 3) && !has_property(art, obj, "fire resistance")) {
			get_property(art, obj, "fire resistance", -5 * randint1(6), true);
		}
		if ((choice == 4) && !has_property(art, obj, "cold resistance")) {
			get_property(art, obj, "cold resistance", -5 * randint1(6), true);
		}
		if ((choice == 5) && !has_property(art, obj, "poison resistance")) {
			get_property(art, obj, "poison resistance", -5 * randint1(6), true);
		}
		if ((choice == 6) && !has_property(art, obj, "light resistance")) {
			get_property(art, obj, "light resistance", -5 * randint1(6), true);
		}
		if ((choice == 7) && !has_property(art, obj, "dark resistance")) {
			get_property(art, obj, "dark resistance", -5 * randint1(6), true);
		}
		if ((choice == 8) && !has_property(art, obj, "sound resistance")) {
			get_property(art, obj, "sound resistance", -5 * randint1(6), true);
		}
		if ((choice == 9) && !has_property(art, obj, "shards resistance")) {
			get_property(art, obj, "shards resistance", -5 * randint1(6), true);
		}
		if ((choice == 10) && !has_property(art, obj, "nexus resistance")) {
			get_property(art, obj, "nexus resistance", -5 * randint1(6), true);
		}
		if ((choice == 11) && !has_property(art, obj, "nether resistance")) {
			get_property(art, obj, "nether resistance", -5 * randint1(6), true);
		}
		if ((choice == 12) && !has_property(art, obj, "chaos resistance")) {
			get_property(art, obj, "chaos resistance", -5 * randint1(6), true);
		}
		if ((choice == 13) && !has_property(art, obj,
											"disenchantment resistance")) {
			get_property(art, obj, "disenchantment resistance",
						 -5 * randint1(6), true);
		}
	}

	/* Artifacts that still have lots of money to spend deserve the best. */
	if ((potential > 2000) && !one_in_(3)) {

		/* Make a choice... */
		choice = randint1(5);
		/* ...among some tasty options. */
		if ((choice == 1) && !has_property(art, obj, "telepathy")) {
			get_property(art, obj, "telepathy", 0, true);
		}
		if ((choice == 2) && !has_property(art, obj, "hold life")) {
			get_property(art, obj, "hold life", 0, true);
		}
		if (choice == 3) {
			if (!has_property(art, obj, "protection from confusion")) {
				get_property(art, obj, "protection from confusion", 0, true);
			}
			if (!has_property(art, obj, "protection from blindness")) {
				get_property(art, obj, "protection from blindness", 0, true);
			}
		}
		if (choice == 4) {
			if (one_in_(4) && !has_property(art, obj,
											"disenchantment resistance")) {
				get_property(art, obj, "disenchantment resistance", 0, true);
			} else if (one_in_(3) && !has_property(art, obj,
												   "nether resistance")) {
				get_property(art, obj, "nether resistance", 0, true);
			} else if (one_in_(2) && !has_property(art, obj,
												   "chaos resistance")) {
				get_property(art, obj, "chaos resistance", 0, true);
			} else if (!has_property(art, obj, "poison resistance")) {
				get_property(art, obj, "poison resistance", 0, true);
			}
		}
		if ((choice == 5) && (potential > 4500)) {
			if (one_in_(2) && !has_property(art, obj, "speed")) {
				get_property(art, obj, "speed", randint1(10), true);
			}

			else {
				get_property(art, obj, "sustain strength", 0, true);
				get_property(art, obj, "sustain wisdom", 0, true);
				get_property(art, obj, "sustain intelligence", 0, true);
				get_property(art, obj, "sustain dexterity", 0, true);
				get_property(art, obj, "sustain constitution", 0, true);
			}
		}
	}

	/* Sometimes, also add a new numerical bonus quality. Infravision and
	 * magical item mastery are not on offer. Only melee weapons can get a
	 * bonus to tunnelling. */
	if ((randint1(5) < 3) || (potential >= 2750)) {
		bonus = randint1(3);
		if (weapon) {
			choice = randint1(9);
		} else {
			choice = randint1(8);
		}
		if (choice == 1) {
			get_property(art, obj, "strength", bonus, true);
		}
		if (choice == 2) {
			get_property(art, obj, "wisdom", bonus, true);
		}
		if (choice == 3) {
			get_property(art, obj, "intelligence", bonus, true);
		}
		if (choice == 4) {
			get_property(art, obj, "dexterity", bonus, true);
		}
		if (choice == 5) {
			get_property(art, obj, "constitution", bonus, true);
		}
		if (choice == 6) {
			get_property(art, obj, "stealth", bonus, true);
		}
		if (choice == 7) {
			get_property(art, obj, "searching skill", bonus, true);
		}
		if (choice == 8) {
			get_property(art, obj, "speed", bonus, true);
		}
		if (choice == 9) {
			get_property(art, obj, "tunneling", bonus, true);
		}
	}

	/* Now, we enter Filene's Basement, and shop 'til we drop! (well, nearly) */
	while ((potential >= 300) && (limit-- > 0)) {

		/* I'm a melee weapon. */
		if (weapon) {
			/* Should be an artifact */
			assert(art);

			/* Some throwing weapons can be thrown hard and fast. */
			if (of_has(art->flags, OF_THROWING) && one_in_(2))
				get_property(art, obj, "perfect balance", 0, true);
			/* Some weapons already will have superb base damage. */
			if (has_property(art, obj, "enhanced dice")) {

				/* Sometimes, such weapons are unusual. */
				if (one_in_(3) && (potential >= 1500)) {
					if (art->weight == kind->weight) {
						if (one_in_(3)) {
							/* Sometimes, such weapons are unusually heavy. */
							art->weight += randint1(art->weight / 10) * 5 + 5;
							potential += (art->weight - kind->weight) * 10;
						} else if (one_in_(2) && (art->weight >= 150)) {
							/* Sometimes, such weapons are unusually light. */
							art->weight -= randint1(art->weight / 10) * 5 + 5;
							potential -= (kind->weight - art->weight) * 10;
						}
					}

					/* Sometimes spend everything to enhance the damage dice.
					 * SJGU assuming there wasn't too much to start with... */
					if (one_in_(3) && (initial_potential <= 4000)) {

						/* Probably sacrifice the Skill bonus. */
						if (!one_in_(3)) {
							art->to_h = 0;
							potential += 600;
							/* Possibly also sacrifice the Deadliness bonus. */
							if (one_in_(4)) {
								art->to_h = 0;
								potential += 600;
							}
						}
						get_property(art, obj, "enhanced dice", 1, true);
						/* We're done */
						break;
					}
				}
			} else if (randint1(art->dd * art->ds) < 5 || potential >= 2100) {
				/* Other weapons have enhanced damage dice in addition to other
				 * qualities.
				 * SJGU increased chance of this as small dice are useless */
				get_property(art, obj, "enhanced dice", 3, true);
			}

			/* Collect a slay or brand. */
			choice = randint1(13);
			if ((choice == 1) && !has_property(art, obj, "slay animals")) {
				get_property(art, obj, "slay animals", 17, true);
			}
			if ((choice == 2) && !has_property(art, obj,
											   "slay evil creatures")) {
				get_property(art, obj, "slay evil creatures", 15, true);
			}
			if ((choice == 3) && !has_property(art, obj, "slay undead")) {
				get_property(art, obj, "slay undead", 20, true);
			}
			if ((choice == 4) && !has_property(art, obj, "slay demons")) {
				get_property(art, obj, "slay demons", 20, true);
			}
			if ((choice == 5) && !has_property(art, obj, "slay orcs")) {
				get_property(art, obj, "slay orcs", 20, true);
			}
			if ((choice == 6) && !has_property(art, obj, "slay trolls")) {
				get_property(art, obj, "slay trolls", 20, true);
			}
			if ((choice == 7) && !has_property(art, obj, "slay giants")) {
				get_property(art, obj, "slay giants", 20, true);
			}
			if ((choice == 8) && !has_property(art, obj, "slay dragons")) {
				get_property(art, obj, "slay dragons", 20, true);
			}
			if ((choice == 9) && !has_property(art, obj, "acid brand")) {
				get_property(art, obj, "acid brand", 17, true);
			}
			if ((choice == 10) && !has_property(art, obj, "lightning brand")) {
				get_property(art, obj, "lightning brand", 17, true);
			}
			if ((choice == 11) && !has_property(art, obj, "fire brand")) {
				get_property(art, obj, "fire brand", 17, true);
			}
			if ((choice == 12) && !has_property(art, obj, "cold brand")) {
				get_property(art, obj, "cold brand", 17, true);
			}
			if ((choice == 13) && !has_property(art, obj, "poison brand")) {
				get_property(art, obj, "poison brand", 17, true);
			}
			/* Often, collect a miscellaneous quality, if it is affordable. */
			if (one_in_(2)) {
				choice = randint1(8);
				if ((choice == 1) && !has_property(art, obj, "slow digestion")){
					get_property(art, obj, "slow digestion", 0, false);
				}
				if ((choice == 2) &&
					!has_property(art, obj, "feather falling")) {
					get_property(art, obj, "feather falling", 0, false);
				}
				if ((choice == 3) && !has_property(art, obj, "light")) {
					get_property(art, obj, "light", 1, false);
				}
				if ((choice == 4) && !has_property(art, obj, "regeneration")) {
					get_property(art, obj, "regeneration", 0, false);
				}
				if ((choice == 5) && !has_property(art, obj, "see invisible")) {
					get_property(art, obj, "see invisible", 0, false);
				}
				if ((choice == 6) && !has_property(art, obj, "free action")) {
					get_property(art, obj, "free action", 0, false);
				}
				if ((choice == 7) &&
					!has_property(art, obj, "protection from fear")) {
					get_property(art, obj, "protection from fear", 0, false);
				}
				if ((choice == 8) &&
					!has_property(art, obj, "protection from blindness")) {
					get_property(art, obj, "protection from blindness", 0,
								 false);
				}
			}

			/* Sometimes, collect a resistance, if it is affordable. */
			if (one_in_(3)) {
				choice = randint1(11);
				if ((choice == 1) &&
					!has_property(art, obj, "acid resistance")) {
					get_property(art, obj, "acid resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 2) &&
					!has_property(art, obj, "electricity resistance")){
					get_property(art, obj, "electricity resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 3) &&
					!has_property(art, obj, "fire resistance")) {
					get_property(art, obj, "fire resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 4) &&
					!has_property(art, obj, "cold resistance")) {
					get_property(art, obj, "cold resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 5) &&
					!has_property(art, obj, "poison resistance")) {
					get_property(art, obj, "poison resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 6) &&
					!has_property(art, obj, "light resistance")) {
					get_property(art, obj, "light resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 7) &&
					!has_property(art, obj, "dark resistance")) {
					get_property(art, obj, "dark resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 8) &&
					!has_property(art, obj, "sound resistance")) {
					get_property(art, obj, "sound resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 9) &&
					!has_property(art, obj, "shards resistance")) {
					get_property(art, obj, "shards resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 10) &&
					!has_property(art, obj, "nexus resistance")) {
					get_property(art, obj, "nexus resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 11) &&
					!has_property(art, obj, "disenchantment resistance")) {
					get_property(art, obj, "disenchantment resistance",
								 35 + 5 * randint0(5), true);
				}
			}

			/* Clean out the wallet. */
			if ((potential < 500) && one_in_(5)) {
				if (has_property(art, obj, "deadliness bonus")) {
					get_property(art, obj, "deadliness bonus",
								 potential / 150, true);
				}
				if (has_property(art, obj, "skill bonus")) {
					get_property(art, obj, "skill bonus",
								 potential / 150, true);
				}
				potential = 0;
			}
		}

		/* I'm a missile weapon. */
		if (launcher) {
			/* Collect a miscellaneous quality. */
			choice = randint1(8);
			if ((choice == 1) && !has_property(art, obj, "slow digestion")){
				get_property(art, obj, "slow digestion", 0, false);
			}
			if ((choice == 2) &&
				!has_property(art, obj, "feather falling")) {
				get_property(art, obj, "feather falling", 0, false);
			}
			if ((choice == 3) && !has_property(art, obj, "light")) {
				get_property(art, obj, "light", 1, false);
			}
			if ((choice == 4) && !has_property(art, obj, "regeneration")) {
				get_property(art, obj, "regeneration", 0, false);
			}
			if ((choice == 5) && !has_property(art, obj, "see invisible")) {
				get_property(art, obj, "see invisible", 0, false);
			}
			if ((choice == 6) && !has_property(art, obj, "free action")) {
				get_property(art, obj, "free action", 0, false);
			}
			if ((choice == 7) &&
				!has_property(art, obj, "protection from fear")) {
				get_property(art, obj, "protection from fear", 0, false);
			}
			if ((choice == 8) &&
				!has_property(art, obj, "protection from blindness")) {
				get_property(art, obj, "protection from blindness", 0, false);
			}
			/* Sometimes, collect a resistance, if it is affordable. */
			if (one_in_(2)) {
				choice = randint1(11);
				if ((choice == 1) &&
					!has_property(art, obj, "acid resistance")) {
					get_property(art, obj, "acid resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 2) &&
					!has_property(art, obj, "electricity resistance")){
					get_property(art, obj, "electricity resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 3) &&
					!has_property(art, obj, "fire resistance")) {
					get_property(art, obj, "fire resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 4) &&
					!has_property(art, obj, "cold resistance")) {
					get_property(art, obj, "cold resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 5) &&
					!has_property(art, obj, "poison resistance")) {
					get_property(art, obj, "poison resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 6) &&
					!has_property(art, obj, "light resistance")) {
					get_property(art, obj, "light resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 7) &&
					!has_property(art, obj, "dark resistance")) {
					get_property(art, obj, "dark resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 8) &&
					!has_property(art, obj, "sound resistance")) {
					get_property(art, obj, "sound resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 9) &&
					!has_property(art, obj, "shards resistance")) {
					get_property(art, obj, "shards resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 10) &&
					!has_property(art, obj, "nexus resistance")) {
					get_property(art, obj, "nexus resistance",
								 35 + 5 * randint0(5), true);
				}
				if ((choice == 11) &&
					!has_property(art, obj, "disenchantment resistance")) {
					get_property(art, obj, "disenchantment resistance",
								 35 + 5 * randint0(5), true);
				}
			}

			/* Clean out the wallet. */
			if ((potential < 500) && one_in_(5)) {
				if (has_property(art, obj, "deadliness bonus")) {
					get_property(art, obj, "deadliness bonus",
								 potential / 150, true);
				}
				if (has_property(art, obj, "skill bonus")) {
					get_property(art, obj, "skill bonus",
								 potential / 150, true);
				}
				potential = 0;
			}
		}

		/* I'm any piece of armour or jewellery. */

		/* Collect a resistance. */
		choice = randint1(11);
		if ((choice == 1) && !has_property(art, obj, "acid resistance")) {
			get_property(art, obj, "acid resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 2) && !has_property(art, obj, "electricity resistance")){
			get_property(art, obj, "electricity resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 3) && !has_property(art, obj, "fire resistance")) {
			get_property(art, obj, "fire resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 4) && !has_property(art, obj, "cold resistance")) {
			get_property(art, obj, "cold resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 5) && !has_property(art, obj, "poison resistance")) {
			get_property(art, obj, "poison resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 6) && !has_property(art, obj, "light resistance")) {
			get_property(art, obj, "light resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 7) && !has_property(art, obj, "dark resistance")) {
			get_property(art, obj, "dark resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 8) && !has_property(art, obj, "sound resistance")) {
			get_property(art, obj, "sound resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 9) && !has_property(art, obj, "shards resistance")) {
			get_property(art, obj, "shards resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 10) && !has_property(art, obj, "nexus resistance")) {
			get_property(art, obj, "nexus resistance",
						 35 + 5 * randint0(5), true);
		}
		if ((choice == 11) &&
			!has_property(art, obj, "disenchantment resistance")) {
			get_property(art, obj, "disenchantment resistance",
						 35 + 5 * randint0(5), true);
		}
		/* Sometimes, collect a miscellaneous quality, if it is affordable. */
		if (one_in_(3)) {
			choice = randint1(8);
			if ((choice == 1) && !has_property(art, obj, "slow digestion")){
				get_property(art, obj, "slow digestion", 0, false);
			}
			if ((choice == 2) &&
				!has_property(art, obj, "feather falling")) {
				get_property(art, obj, "feather falling", 0, false);
			}
			if ((choice == 3) && !has_property(art, obj, "light")) {
				get_property(art, obj, "light", 1, false);
			}
			if ((choice == 4) && !has_property(art, obj, "regeneration")) {
				get_property(art, obj, "regeneration", 0, false);
			}
			if ((choice == 5) && !has_property(art, obj, "see invisible")) {
				get_property(art, obj, "see invisible", 0, false);
			}
			if ((choice == 6) && !has_property(art, obj, "free action")) {
				get_property(art, obj, "free action", 0, false);
			}
			if ((choice == 7) &&
				!has_property(art, obj, "protection from fear")) {
				get_property(art, obj, "protection from fear", 0, false);
			}
			if ((choice == 8) &&
				!has_property(art, obj, "protection from blindness")) {
				get_property(art, obj, "protection from blindness", 0, false);
			}

			/* Clean out the wallet. */
			if ((potential < 500) && one_in_(5)) {
				if (has_property(art, obj, "armor bonus")) {
					get_property(art, obj, "armor bonus", potential/ 100, true);
					potential = 0;
				}
			}
		}
	}

	/* On sale free! Last chance! */

	/* If an artifact affects a stat, it may also possibly sustain it. */
	choice = randint0(STAT_MAX);
	for (i = 0; i < STAT_MAX; i++) {
		const char *stat = lookup_obj_property(OBJ_PROPERTY_STAT, i)->name;
		const char *sust = lookup_obj_property(OBJ_PROPERTY_FLAG, i + 1)->name;
		if ((choice == i) && has_property(art, obj, stat)) {
			get_property(art, obj, sust, 0, true);
		}
	}

	/* Armour gets a basic resist or low-level ability with 50% probability,
	 * and weapons with 25% probability. */
	if ((weapon && one_in_(4)) || one_in_(2)) {
		s16b *res;
		choice = randint0(6);
		res = art ? &(art->el_info[choice].res_level) :
			&(obj->el_info[choice].res_level);
		if (choice < 4) {
			*res = ((*res) * 2) / 3;
		} else if (choice == 4) {
			get_property(art, obj, "slow digestion", 0, true);
		} else if (choice == 5) {
			get_property(art, obj, "feather falling", 0, true);
		}
	}

	/* Frequently neaten bonuses to Armour Class, Skill, and Deadliness. */
	for (i = 0; i < 3; i++) {
		s16b *bonus_chg = art ? &art->to_a : &obj->to_a;
		if (i == 1) { 
			bonus_chg = art ? &art->to_h : &obj->to_h;
		}
		if (i == 2) { 
			bonus_chg = art ? &art->to_d : &obj->to_d;
		}
		if (((*bonus_chg) % 5 == 4) && one_in_(2)) {
			(*bonus_chg)++;
		} else if (((*bonus_chg) % 5 == 1) && one_in_(2)) {
			(*bonus_chg)--;
		} else if (((*bonus_chg) % 2 == 1) && !one_in_(4)) {
			(*bonus_chg)++;
		}
	}
}

/**
 * Envoke perilous magics, and curse the artifact beyound redemption!  I 
 * had such fun coding this...
 */
static void make_terrible(struct artifact *art, struct object *obj)
{
	int i, gauntlet_runs;
	int num_curses = 1;
	struct object_kind *kind = art ? lookup_kind(art->tval, art->sval) :
		lookup_kind(obj->tval, obj->sval);
	bool weapon = tval_is_melee_weapon_k(kind);

	/* Determine whether the artifact's magics are perilous enough to warrant
	 * extra curses. */
	if (potential >= 3000) {
		num_curses++;
	}
	if ((potential >= 3500) && !one_in_(3)) {
		num_curses++;
	}

	/* Greatly decrease the chance for an activation. */
	if (art && art->activation && !one_in_(3)) {
		art->activation = NULL;
	} else if (obj && obj->activation && !one_in_(3)) {
		obj->activation = NULL;
	}

	/* Force the artifact though the gauntlet two or three times. */
	gauntlet_runs = 1 + randint1(2);
	for (i = 0; i < gauntlet_runs; i++) {
		int j, wheel_of_doom, penalty = 0;

		/* Choose a curse, biased towards penalties to_a, to_d, and to_h. */
		if (weapon && has_property(art, obj, "skill bonus") && one_in_(2)) {
			wheel_of_doom = 2;
		} else if (has_property(art, obj, "armor bonus") && one_in_(2)) {
			wheel_of_doom = 1;
		} else {
			wheel_of_doom = 2 + randint1(2);
		}
		/* Blast base armour class or inflict a penalty to armour class. */
		if (wheel_of_doom == 1) {

			if (art) {
				/* Blast armour and twist magics. */
				if ((art->ac) && (one_in_(6))) {
					art->cost -= 500L * art->ac;
					art->ac = 0;
				}
				if ((art->tval >= TV_BOOTS) && (art->to_a >= 0)) {
					art->to_a = -(5 + randint1(7)) * 2;
				} else if ((one_in_(4)) && (art->tval < TV_BOOTS)) {
					/* Chance of a truly nasty effect for weapons. */
					penalty = randint1(3) + 2;
					penalty *= 5;
					art->to_a = -penalty;
					art->cost -= penalty * 200L;
				}

				/* Artifact might very well still have some value. */
				if (art->to_a < 0) {
					art->cost += art->to_a * 400L;
				}
			} else {
				/* Invert armour class. */
				if ((obj->ac) && (randint1(6) == 1)) {
					obj->ac = 0;
				} else if (randint1(4) == 1) {
					/* Chance of a truly nasty effect. */
					penalty = randint1(3) + 2;
					penalty *= 5;
					obj->to_a = -penalty;
				}
			}
		}

		/* Make either the Skill or Deadliness bonus negative, or both. */
		if (wheel_of_doom == 2) {

			if (art) {
				/* Weapons. */
				if (tval_is_weapon_a(art)) {
					/* Blast items with bonuses, Deadliness more rarely. */
					art->to_h = -(5 + randint1(7)) * 2;
					if (!one_in_(3)) {
						art->to_d = -(5 + randint1(7)) * 2;
					}
				} else {
					/* All armours. */

					/* Reverse any magics. */
					if (art->to_h > 0) {
						art->to_h = -art->to_h;
					}
					if (art->to_d > 0) {
						art->to_d = -art->to_d;
					}

					/* Sometimes, blast even items without bonuses. */
					if (!art->to_d && !art->to_h && one_in_(5)) {
						penalty = randint1(4) + 1;
						penalty *= 5;
						art->to_h -= penalty;
						art->to_d -= penalty;
					}
				}

				/* Artifact might very well still have some value. */
				if ((art->to_d < 0) && (art->to_h < 0)) {
					art->cost -= 10000L;
				} else if ((art->to_d < 0) || (art->to_h < 0)) {
					art->cost -= 5000L;
				}
				if (art->cost < 0) {
					art->cost = 0;
				}
			} else {
				if (randint1(2) == 1) {
					/* One type of badness... */
					/* Blast items with bonuses, Deadliness more rarely. */
					obj->to_h = -(5 + randint1(7)) * 2;
					if (!one_in_(3)) {
						obj->to_d = -(5 + randint1(7)) * 2;
					}
				} else {
					/* ...or another. */
					/* Reverse any magics. */
					if (obj->to_h > 0) {
						obj->to_h = -obj->to_h;
					}
					if (obj->to_d > 0) {
						obj->to_d = -obj->to_d;
					}

					/* Sometimes, blast even items without bonuses. */
					if (!obj->to_d && !obj->to_h && one_in_(5)) {
						penalty = randint1(4) + 1;
						penalty *= 5;
						obj->to_h -= penalty;
						obj->to_d -= penalty;
					}
				}
			}
		}

		/* Make any bonuses negative, or strip all bonuses, and add a random
		 * stat or three with large negative bonuses. */
		if (wheel_of_doom == 3) {
			if (!one_in_(3)) {
				for (j = 0; j < STAT_MAX; j++) {
					s16b *bonus = art ? &art->modifiers[j] : &obj->modifiers[j];
					if ((*bonus) > 0) {
						*bonus = -(*bonus);
					}
				}

				/* Artifact is highly unlikely to have any value. */
				if (art) {
					art->cost = MAX(art->cost - 30000L, 0);
				}
			} else {
				/* Iron Crown of Beruthiel, here we come... */
				penalty = -(randint1(5)) - 3;
				if (one_in_(3)) {
					penalty *= 5;
				}
				for (j = 0; j < OBJ_MOD_MAX; j++) {
					s16b *bonus = art ? &art->modifiers[j] : &obj->modifiers[j];
					*bonus = 0;
				}
				if (one_in_(5)) {
					for (j = 0; j < STAT_MAX; j++) {
						s16b *bonus = art ? &art->modifiers[j] :
							&obj->modifiers[j];
						if (j == OBJ_MOD_STR) *bonus = penalty;
						if (j == OBJ_MOD_DEX) *bonus = penalty;
						if (j == OBJ_MOD_CON) *bonus = penalty;
					}
				} else if (one_in_(4)) {
					for (j = 0; j < STAT_MAX; j++) {
						s16b *bonus = art ? &art->modifiers[j] :
							&obj->modifiers[j];
						if (j == OBJ_MOD_WIS) *bonus = penalty;
						if (j == OBJ_MOD_INT) *bonus = penalty;
					}
				} else if (one_in_(6)) {
					s16b *bonus = art ? &art->modifiers[OBJ_MOD_SPEED] :
						&obj->modifiers[OBJ_MOD_SPEED];
					*bonus = penalty;
				} else {
					for (j = 0; j < STAT_MAX; j++) {
						s16b *bonus = art ? &art->modifiers[j] :
							&obj->modifiers[j];
						if (one_in_(4)) {
							*bonus = penalty;
						}
					}
				}

				/* More curses for good measure... */
				num_curses += randint1(2);

				/* Artifact is highly unlikely to have any value. */
				if (art) {
					art->cost = MAX(art->cost - 60000L, 0);
				}
			}
		}

		/* Strip lots of other qualities. */
		if (wheel_of_doom == 4) {
			bitflag *flags = art ? art->flags : obj->flags;

			/* Powerful modifiers */
			if (art) {
				art->modifiers[OBJ_MOD_BLOWS] = 0;
			} else {
				obj->modifiers[OBJ_MOD_BLOWS] = 0;
			}
			if (one_in_(4)) {
				if (art) {
					art->modifiers[OBJ_MOD_SHOTS] = 0;
				} else {
					obj->modifiers[OBJ_MOD_SHOTS] = 0;
				}
			}
			if (one_in_(4)) {
				if (art) {
					art->modifiers[OBJ_MOD_MIGHT] = 0;
				} else {
					obj->modifiers[OBJ_MOD_MIGHT] = 0;
				}
			}

			/* Slays */
			for (i = 0; i < z_info->slay_max; i++) {
				bool *slay = NULL;
				if (art && art->slays) slay = &art->slays[i];
				if (obj && obj->slays) slay = &obj->slays[i];
				if (one_in_(3) && slay) *slay = false;
			}
			for (i = 0; i < z_info->slay_max; i++) {
				if (art && art->slays && art->slays[i]) break;
				if (obj && obj->slays && obj->slays[i]) break;
			}
			if (art && (i == z_info->slay_max)) {
				mem_free(art->slays);
				art->slays = NULL;
			} else if (obj && (i == z_info->slay_max)) {
				mem_free(obj->slays);
				obj->slays = NULL;
			}
			if (one_in_(3)) {
				of_off(flags, OF_PERFECT_BALANCE);
			}

			/* Brands */
			for (i = 0; i < z_info->brand_max; i++) {
				bool *brand = NULL;
				if (art && art->brands) brand = &art->brands[i];
				if (obj && obj->brands) brand = &obj->brands[i];
				if (one_in_(3) && brand) *brand = false;
			}
			for (i = 0; i < z_info->brand_max; i++) {
				if (art && art->brands && art->brands[i]) break;
				if (obj && obj->brands && obj->brands[i]) break;
			}
			if (art && (i == z_info->brand_max)) {
				mem_free(art->brands);
				art->brands = NULL;
			} else if (obj && (i == z_info->brand_max)) {
				mem_free(obj->brands);
				obj->brands = NULL;
			}

			/* Resists */
			for (i = 0; i < ELEM_HIGH_MAX; i++) {
				s16b *level = art ? &art->el_info[i].res_level :
					&obj->el_info[i].res_level;
				if (one_in_(3) && (*level < RES_LEVEL_BASE)) {
					*level += 2 * (RES_LEVEL_BASE - *level);
				}
			}

			/* Flags */
			for (i = 0; i < z_info->property_max; i++) {
				struct obj_property *prop = &obj_properties[i];
				if (one_in_(3) && (prop->type == OBJ_PROPERTY_FLAG) &&
					(prop->subtype == OFT_PROT || prop->subtype == OFT_MISC)) {
					of_off(flags, prop->index);
				}
			}

			/* Artifact will still have some value. */
			if (art && art->cost > 0) {
				art->cost /= 3L;
			}
		}
	}

	/* Boundary control. */
	if (art && art->cost < 0) {
		art->cost = 0;
	}

	/* Apply curses. */
	while (num_curses) {
		int level = art ? art->alloc_min : obj->kind->alloc_min;
		int tval = art ? art->tval : obj->tval;
		int pick = randint1(z_info->curse_max - 1);
		int power = randint1(9) + 10 * m_bonus(9, level);
		if (!curses[pick].poss[tval]) {
			continue;
		}
		if (art) {
			append_artifact_curse(art, pick, power);
		} else {
			append_object_curse(obj, pick, power);
		}
		num_curses--;
	}
}

/**
 * Help final_check():  remove the activation if it conflicts or is
 * redundant with the other properties of the artifact.
 */
static void remove_contradictory_activation(struct artifact *art,
											struct object *obj)
{
	bitflag *flags = art ? art->flags : obj->flags;
	struct element_info *el_info = art ? art->el_info : obj->el_info;
	struct activation *act = art ? art->activation : obj->activation;
	bool *loc_brands = art ? art->brands : obj->brands;
	bool *loc_slays = art ? art->slays : obj->slays;
	bool redundant = true;
	int unsummarized_count;
	struct effect_object_property *props, *pcurr;

	if (!act) return;

	props = effect_summarize_properties(act->effect, &unsummarized_count);

	if (unsummarized_count > 0) {
		/*
		 * The activation does at least one thing that doesn't
		 * correspond to an object property.
		 */
		redundant = false;
	} else {
		for (pcurr = props; pcurr && redundant; pcurr = pcurr->next) {
			int i, maxmult;
           
			switch (pcurr->kind) {
				case EFPROP_BRAND:
					maxmult = 1;
					if (!loc_brands) break;
					for (i = 1; i < z_info->brand_max; ++i) {
						if (!loc_brands[i]) continue;
						if (brands[i].resist_flag !=
							brands[pcurr->idx].resist_flag) continue;
						maxmult = MAX(brands[i].multiplier, maxmult);
					}
					if (maxmult < brands[pcurr->idx].multiplier) {
						redundant = false;
					}
					break;

				case EFPROP_SLAY:
					maxmult = 1;
					if (!loc_slays) break;
					for (i = 1; i < z_info->slay_max; ++i) {
						if (!loc_slays[i]) continue;
						if (!same_monsters_slain(i, pcurr->idx)) continue;
						maxmult = MAX(slays[i].multiplier, maxmult);
					}
					if (maxmult < slays[pcurr->idx].multiplier) {
						redundant = false;
					}
					break;
                       
				case EFPROP_RESIST:
				case EFPROP_CONFLICT_RESIST:
				case EFPROP_CONFLICT_VULN:
					if (el_info[pcurr->idx].res_level >= pcurr->reslevel_min &&
						el_info[pcurr->idx].res_level <= pcurr->reslevel_max) {
						redundant = false;
					}
					break;

				case EFPROP_OBJECT_FLAG:
					/*
					 * It does something more than just the object
					 * flag so don't call it redundant.  To screen
					 * out HERO and SHERO activations when the
					 * object has OF_PROT_FEAR, use the same
					 * handling for this case as for
					 * EFPROP_OBJECT_FLAG_EXACT.
					 */
					redundant = false;
					break;

				case EFPROP_OBJECT_FLAG_EXACT:
				case EFPROP_CURE_FLAG:
				case EFPROP_CONFLICT_FLAG:
					/*
					 * If the object doesn't have the flag, it's
					 * not redundant.
					 */
					if (!of_has(flags, pcurr->idx)) {
						redundant = false;
					}
					break;

				default:
					/*
					 * effect_summarize_properties() gave use
					 * something unexpected.  Assume the effect is
					 * useful.
					 */
					redundant = false;
					break;
			}
		}
	}

	while (props) {
		pcurr = props;
		props = props->next;
		mem_free(pcurr);
	}

	if (redundant) {
		act = NULL;
	}
}

/**
 * Clean up the artifact by removing illogical combinations of powers -  
 * curses win out every time.
 */
static void final_check(struct artifact *art, struct object *obj)
{
	bitflag *flags = art ? art->flags : obj->flags;
	s16b *modifiers = art ? art->modifiers : obj->modifiers;
	struct element_info *el_info = art ? art->el_info : obj->el_info;
	int i;

	/* Limit modifiers, remove sustains if a stat modifier is negative */
	for (i = 0; i < OBJ_MOD_MAX; i++) {
		if ((i < STAT_MAX) && (modifiers[i] < 0)) {
			of_off(flags, i + 1);
		}
		if (i != OBJ_MOD_SPEED) {
			modifiers[i] = MIN(modifiers[i], 6);
		}
	}

	/* Low resist means object is proof against that element */
	for (i = 0; i < ELEM_BASE_MAX; i++) {
		if (el_info[i].res_level < RES_LEVEL_BASE) {
			el_info[i].flags |= EL_INFO_IGNORE;
		}
	}

	/* Remove contradictory properties */
	if ((modifiers[OBJ_MOD_STEALTH] > 0) && of_has(flags, OF_AGGRAVATE)) {
		modifiers[OBJ_MOD_STEALTH] = 0;
	}
	if ((modifiers[OBJ_MOD_LIGHT] > 0) && of_has(flags, OF_DARKNESS)) {
		modifiers[OBJ_MOD_LIGHT] = 0;
	}
	remove_contradictory_activation(art, obj);
}

/**
 * Find a name from any of various text files.
 *
 * Select a file, depending on whether the artifact is a weapon or armour,
 * and whether or not it is cursed. Get a random line from that file.
 * Decision now based on cost -NRM-
 */
static void find_word(struct artifact *art, char *word, size_t len)
{
	static char art_name[81];
	art_name[0] = '\0';
	if (art->cost == 0L) {
		if (tval_is_weapon_a(art)) {
			get_rnd_line("w_cursed.txt", art_name);
		} else {
			get_rnd_line("a_cursed.txt", art_name);
		}
	} else {
		if (tval_is_weapon_a(art)) {
			get_rnd_line("w_normal.txt", art_name);
		} else {
			get_rnd_line("a_normal.txt", art_name);
		}
	}

	my_strcpy(word, art_name, len);
}

/**
 * Name an artifact
 *
 * Use W. Sheldon Simms' random name generator most of the time, generally
 * for the less powerful artifacts. Otherwise, find a name from a text file.
 */
static void name_artifact(struct artifact *art, const char ***words)
{
	char word[1024];
	char buf[1024];
	if ((randint1(max_potential) > initial_potential) || one_in_(2)) {
		randname_make(RANDNAME_TOLKIEN, MIN_NAME_LEN, MAX_NAME_LEN, word,
				  sizeof(word), words);
		my_strcap(word);
		if (one_in_(3)) {
			my_strcpy(buf, format("'%s'", word), sizeof(buf));
		} else {
			my_strcpy(buf, format("of %s", word), sizeof(buf));
		}
	} else {
		find_word(art, word, sizeof(word));
		my_strcpy(buf, word, sizeof(buf));
	}

	/* Apply whatever name is created. */
	art->name = string_make(word);
}

/**
 * Give an artifact a (boring) description
 */
static void describe_artifact(struct artifact *art)
{
	char desc[128] = "Random ";
	my_strcat(desc, tval_find_name(art->tval), sizeof(desc));
	my_strcat(desc, format(" of power %d", initial_potential), sizeof(desc));
	string_free(art->text);
	art->text = string_make(desc);
}

/**
 * Design a random artifact.
 */
static void design_random_artifact(struct artifact *art)
{
	/* Initialize the artifact, and assign it a potential. */
	initialize_artifact(art);

	/* Assign a theme to the artifact. */
	choose_artifact_theme(art);

	/* Add extra qualities until done. */
	haggle_till_done(art, NULL);

	/* Decide if the artifact is to be terrible. */
	if (TERRIBLE_CHANCE > randint0(100)) {
		/* If it is, add a few benefits and more drawbacks. */
		make_terrible(art, NULL);
	}

	/* Check powers for validity and contradictions. */
	final_check(art, NULL);

	/* Give the artifact a rudimentary description */
	describe_artifact(art);

	/* Find or make a name for the artifact, and place into a temporary array */
	name_artifact(art, name_sections);
}

/**
 * Write an artifact data file entry
 */
static void write_randart_file_entry(ang_file *fff, const struct artifact *art)
{
	char name[120] = "";
	struct object_kind *kind = lookup_kind(art->tval, art->sval);
	int j;

	static const char *obj_flags[] = {
		"NONE",
		#define OF(a, b) #a,
		#include "list-object-flags.h"
		#undef OF
		NULL
	};

	/* Ignore non-existent artifacts */
	if (!art->name) return;

	/* Output description */
	file_putf(fff, "# %s\n", art->text);

	/* Output name */
	file_putf(fff, "name:%s\n", art->name);

	/* Output tval and sval */
	object_short_name(name, sizeof name, kind->name);
	file_putf(fff, "base-object:%s:%s\n", tval_find_name(art->tval), name);

	/* Output graphics if necessary */
	if (kind->kidx >= z_info->ordinary_kind_max) {
		const char *attr = attr_to_text(kind->d_attr);
		file_putf(fff, "graphics:%c:%s\n", kind->d_char, attr);
	}

	/* Output level, weight and cost */
	file_putf(fff, "level:%d\n", art->level);
	file_putf(fff, "weight:%d\n", art->weight);
	file_putf(fff, "cost:%d\n", art->cost);

	/* Output allocation info */
	file_putf(fff, "alloc:%d:%d to %d\n", art->alloc_prob, art->alloc_min,
			  art->alloc_max);

	/* Output combat power */
	file_putf(fff, "attack:%dd%d:%d:%d\n", art->dd, art->ds, art->to_h,
			  art->to_d);
	file_putf(fff, "armor:%d:%d\n", art->ac, art->to_a);

	/* Output flags */
	write_flags(fff, "flags:", art->flags, OF_SIZE, obj_flags);

	/* Output modifiers */
	write_mods(fff, art->modifiers);

	/* Output resists, immunities and vulnerabilities */
	write_elements(fff, art->el_info);

	/* Output slays */
	if (art->slays) {
		for (j = 1; j < z_info->slay_max; j++) {
			if (art->slays[j]) {
				file_putf(fff, "slay:%s\n", slays[j].code);
			}
		}
	}

	/* Output brands */
	if (art->brands) {
		for (j = 1; j < z_info->brand_max; j++) {
			if (art->brands[j]) {
				file_putf(fff, "brand:%s\n", brands[j].code);
			}
		}
	}

	/* Output curses */
	if (art->curses) {
		for (j = 1; j < z_info->curse_max; j++) {
			if (art->curses[j] != 0) {
				file_putf(fff, "curse:%s:%d\n", curses[j].name,
						  art->curses[j]);
			}
		}
	}

	/* Output activation details */
	if (art->activation) {
		file_putf(fff, "act:%s\n", art->activation->name);
		file_putf(fff, "time:%d+%dd%d\n", art->time.base, art->time.dice,
				  art->time.sides);
	} else if (kind->activation) {
		file_putf(fff, "act:%s\n", kind->activation->name);
		file_putf(fff, "time:%d+%dd%d\n", kind->time.base, kind->time.dice,
				  kind->time.sides);
	}

	/* Output description again */
	file_putf(fff, "desc:%s\n", art->text);

	file_putf(fff, "\n");
}

/**
 * Initialize all the random artifacts in the artifact array.  This function 
 * is only called when a player is born.
 */
void initialize_random_artifacts(u32b randart_seed)
{
	char fname[1024];
	ang_file *randart_file = NULL;
	int i;

	/* Prepare to use the Angband "simple" RNG. */
	Rand_value = randart_seed;
	Rand_quick = true;

	/* Open the file, write a header */
	path_build(fname, sizeof(fname), ANGBAND_DIR_USER, "randart.txt");
	randart_file = file_open(fname, MODE_WRITE, FTYPE_TEXT);
	if (!randart_file) {
		quit_fmt("Error - can't create %s.", fname);
	}
	file_putf(randart_file,
			  "# Artifact file for random artifacts with seed %08x\n\n\n",
			  randart_seed);

	/* Initialize, name and write artifacts to the file */
	for (i = 0; i < ART_NUM_RANDOM; i++) {
		/* Extend the artifact array with a zero entry */
		struct artifact *art;
		a_info = mem_realloc(a_info, (z_info->a_max + 1) * sizeof(*art));
		aup_info = mem_realloc(aup_info, (z_info->a_max + 1)
							   * sizeof(*aup_info));
		art = &a_info[z_info->a_max];
		memset(art, 0, sizeof(*art));
		memset(&aup_info[z_info->a_max], 0, sizeof(*aup_info));
		aup_info[z_info->a_max].aidx = z_info->a_max;
		z_info->a_max += 1;

		/* Design the artifact, storing information as we go along. */
		design_random_artifact(art);

		/* Write the entry to the randart file */
		write_randart_file_entry(randart_file, art);
	}

	/* Close the file */
	if (!file_close(randart_file)) {
		quit_fmt("Error - can't close %s.", fname);
	}
}

/**
 * ------------------------------------------------------------------------
 * Creation of rings and amulets
 * ------------------------------------------------------------------------ */
/**
 * Grant a certain amount of potential to be used for acquiring powers.
 */
static void allocate_potential(struct object *obj, int lev)
{
	int random;

	/* The total potential of a ring or amulet (found by level 100)
	 * ranges from 0 to 5280, depending on depth and type. */
	potential = (obj->kind->level * 40) + (lev * 10);

	/* Randomise 10% either way */
	random = randint0(potential / 5);
	potential += random - potential / 10;

	/* Remember how much potential was allowed. */
	initial_potential = potential;
}

/**
 * Assign an ego type to a ring or amulet, and then give it properties based
 * on that type.
 */
static bool choose_type(struct object *obj)
{
	int j, temp, bonus;
	bool done = false;
	struct ego_item *ego = NULL;

	/* Get a standard bonus */
	bonus = MAX((potential / 1000), 1);

	/* The main design loop */
	while (!done) {
		/* Pick a random ego type */
		int value = randint0(z_info->e_max);
		struct poss_item *poss;
		ego = &e_info[value];
		poss = ego->poss_items;

		/* Check whether it's a suitable ego type */
		while (poss) {
			if (poss->kidx == obj->kind->kidx) {
				break;
			}
			poss = poss->next;
		}
		if (!poss) continue;

		/* Now the rest of this function simply takes ring or amulet names
		 * and gives them properties appropriate to their theme.  Note that
		 * we may have to go back and pick a new ego type if the object
		 * doesn't have sufficient potential for the chosen ego type. */
		if (streq(ego->name, "of Mental Strength")) {
			/* Min potential 350 */
			int paranoia = 20;

			/* Mostly mental properties */
			while (potential > MIN(2000, (2 * initial_potential / 3))) {
				if (one_in_(2)) {
					/* Deepened wisdom, sustained */
					get_property(NULL, obj, "wisdom", bonus, true);
					get_property(NULL, obj, "sustain wisdom", 0, true);

					/* Maybe an activation */
					if (one_in_(5)) {
						get_activation("RAND_BLESS", obj);
					} else if (one_in_(5)) {
						get_activation("RAND_HEROISM", obj);
					} else if (one_in_(5)) {
						get_activation("RAND_PROT_FROM_EVIL", obj);
					}

					/* Sometimes vulnerable to dark and/or cold */
					if (one_in_(8)) {
						get_property(NULL, obj, "cold resistance",
									 -5 * randint1(6), false);
					}
					if (one_in_(8)) {
						get_property(NULL, obj, "dark resistance",
									 -5 * randint1(6), false);
					}
				} else {
					/* Sharpened intelligence, sustained */
					get_property(NULL, obj, "intelligence", bonus, true);
					get_property(NULL, obj, "sustain intelligence", 0, true);

					/* Maybe an activation */
					if (one_in_(5)) {
						get_activation("RAND_IDENTIFY", obj);
					} else if (one_in_(5)) {
						get_activation("RAND_DETECT_MONSTERS", obj);
					} else if (one_in_(6)) {
						get_activation("RAND_DETECT_D_S_T", obj);
					}

					/* Sometimes vulnerable to electricity and/or light */
					if (one_in_(8)) {
						get_property(NULL, obj, "electricity resistance",
									 -5 * randint1(6), false);
					}
					if (one_in_(8)) {
						get_property(NULL, obj, "light resistance",
									 -5 * randint1(6), false);
					}
				}
				if (paranoia-- <= 0) break;
			}
			done = true;
		} else if (streq(ego->name, "of Doom")) {
			/* Chance to recover similar to damage done */
			potential = 0;

			/* Hurt all stats */
			temp = 0 - randint1(bonus);
			get_property(NULL, obj, "strength", temp, true);
			get_property(NULL, obj, "wisdom", temp, true);
			get_property(NULL, obj, "intelligence", temp, true);
			get_property(NULL, obj, "dexterity", temp, true);
			get_property(NULL, obj, "constitution", temp, true);

			/* Maybe more curses */
			while (!one_in_(10)) {
				int pick = randint1(z_info->curse_max - 1);
				int power = randint1(9) + 10 * m_bonus(9, obj->kind->alloc_min);
				if (!curses[pick].poss[obj->tval]) continue;
				append_object_curse(obj, pick, power);
			}
			done = true;
		} else if (streq(ego->name, "of Basic Resistance")) {
			/* Amulet and ring of the same name, so we do them both here */
			if (tval_is_amulet(obj)) {
				/* Min potential 300 */
				if (one_in_(2)) {
					/* Electricity... */
					get_property(NULL, obj, "electricity resistance",
								 45 + 5 * bonus, true);
					if (one_in_(3)) {
						/* and maybe acid */
						get_property(NULL, obj, "acid resistance",
									 45 + 5 * bonus, true);
					}
				} else {
					/* Acid... */
					get_property(NULL, obj, "acid resistance",
								 45 + 5 * bonus, true);
					if (one_in_(3)) {
						/* and maybe electricity */
						get_property(NULL, obj, "electricity resistance",
									 45 + 5 * bonus, true);
					}
				}

				/* Small chance of another one */
				if (potential > 1000) {
					if (one_in_(10)) {
						get_property(NULL, obj, "cold resistance",
									 35 + 5 * bonus, true);
					}
					if (one_in_(10)) {
						get_property(NULL, obj, "fire resistance",
									 35 + 5 * bonus, true);
					}
				}
			} else if (tval_is_ring(obj)) {
				/* Min potential 300 */
				if (one_in_(2)) {
					/* Fire... */
					get_property(NULL, obj, "fire resistance",
								 45 + 5 * bonus, true);
					if (one_in_(3)) {
						/* and maybe cold */
						get_property(NULL, obj, "cold resistance",
									 45 + 5 * bonus, true);
					}
				} else {
					/* Cold... */
					get_property(NULL, obj, "cold resistance",
								 45 + 5 * bonus, true);
					if (one_in_(3)) {
						/* and maybe fire */
						get_property(NULL, obj, "fire resistance",
									 45 + 5 * bonus, true);
					}
				}

				/* Small chance of another one */
				if (potential > 1000) {
					if (one_in_(10)) {
						get_property(NULL, obj, "acid resistance",
									 35 + 5 * bonus, true);
					}
					if (one_in_(10)) {
						get_property(NULL, obj, "electricity resistance",
									 35 + 5 * bonus, true);
					}
				}
			}
			done = true;
		} else if (streq(ego->name, "of Magical Item Mastery")) {
			/* min potential 105 */

			/* This is an exclusive club */
			if (potential > 1000) {

				/* Bonus to magical item mastery */
				get_property(NULL, obj, "magic mastery", randint1(bonus), true);

				/* Bonus to DEX */
				if (!one_in_(4)) {
					get_property(NULL, obj, "dexterity", randint1(bonus), true);
				}

				/* Protection for the devices */
				if (one_in_(5)) {
					get_property(NULL, obj, "electricity resistance",
								 35 + 5 * bonus, true);
				}
				if (one_in_(5)) {
					get_property(NULL, obj, "fire resistance",
								 35 + 5 * bonus, true);
				}
				if (one_in_(5)) {
					get_property(NULL, obj, "acid resistance",
								 35 + 5 * bonus, true);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Clarity")) {
			/* min potential 800 */

			/* This is an exclusive club */
			if (potential > 2000) {

				/* Freedom of action (weaker objects may get a subset) */
				if (randint1(3500) < initial_potential) {
					get_property(NULL, obj, "protection from blindness", 0,
								 true);
				}
				if (randint1(3500) < initial_potential) {
					get_property(NULL, obj, "protection from confusion", 0,
								 true);
				}
				if (randint1(3500) < initial_potential) {
					get_property(NULL, obj, "free action", 0, true);
				}
				if (randint1(3500) < initial_potential) {
					get_property(NULL, obj, "see invisible", 0, true);
				}

				/* Sometimes the lock */
				if (one_in_(4)) {
					get_property(NULL, obj, "protection from stunning", 0,
								 true);
				}

				/* Can't be damaged */
				get_property(NULL, obj, "ignore electricity", 0, true);

				/* Sometimes vulnerable to nether and/or disenchantment */
				if (one_in_(8)) {
					get_property(NULL, obj, "nether resistance",
								 -5 * randint1(6), false);
				} else if (one_in_(8)) {
					get_property(NULL, obj, "disenchantment resistance",
								 -5 * randint1(6), false);
				}
				done = true;
			}
		} else if (streq(ego->name, "of the Shadows")) {
			/* min potential 1200 */

			/* This is an exclusive club */
			if (potential > 1500) {

				/* Creature of the night  */
				get_property(NULL, obj, "darkness", 0, true);
				get_property(NULL, obj, "stealth", randint0(bonus), true);
				get_property(NULL, obj, "dark resistance",
							 35 + 5 * randint0(bonus), true);
				get_property(NULL, obj, "light resistance",
							 -5 * randint0(bonus), true);

				/* Maybe some infravision and perception */
				if (one_in_(4)) {
					get_property(NULL, obj, "infravision", 1 + randint0(3),
								 true);
				}
				if (one_in_(4)) {
					get_property(NULL, obj, "searching skill", 1 + randint0(3),
								 true);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Metamorphosis")) {
			/* Min potential 2000 */

			/* This is an exclusive club */
			if (potential > 3000) {

				obj->activation = lookup_activation("AMULET_METAMORPH");
				obj->time.base = 300;
				potential -= 2000;
				done = true;
			}
		} else if (streq(ego->name, "of Sustenance")) {
			/* Min potential 50 */
			const char *sustains[7] =
				{ "sustain strength", "sustain intelligence", "sustain wisdom",
				  "sustain dexterity", "sustain constitution", "hold life",
				  "slow digestion"
				};
			int max = 0;
			int property = 1;
			while (property > 0) {
				property = select_property(potential, sustains, 7, &max,
										   RANK_RANDOM_CHOICE, obj);
				get_property(NULL, obj, sustains[property], 0, true);
			}
			done = true;
		} else if (streq(ego->name, "of Trickery")) {
			/* min potential 1650 */

			/* This is an exclusive club */
			if (potential > 2000) {
				/* Some properties for everyone */
				get_property(NULL, obj, "poison resistance",
							 35 + 5 * randint0(5), true);
				get_property(NULL, obj, "dexterity", 2 + randint0(bonus), true);
				get_property(NULL, obj, "sustain dexterity", 0, true);
				get_property(NULL, obj, "stealth", 2 + randint0(bonus), true);

				/* Better amulets get more */
				if (randint1(500) < potential) {
					get_property(NULL, obj, "searching skill",
								 2 + randint0(bonus), true);
				} else if (randint1(1500) < potential) {
					get_property(NULL, obj, "trap immunity", 0, true);
				}
				if (randint1(1000) < potential) {
					get_property(NULL, obj, "nexus resistance",
								 35 + 5 * randint0(5), true);
				}
				if (randint1(1500) < potential) {
					get_property(NULL, obj, "speed", randint1(bonus), true);
				} else if (randint1(1000) < potential) {
					get_property(NULL, obj, "extra moves", 1, true);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Weaponmastery")) {
			/* min potential 2650 */

			/* This is an exclusive club */
			if (potential > 3000) {
				/* Safe to get up close */
				get_property(NULL, obj, "protection from fear", 0, false);
				get_property(NULL, obj, "disenchantment resistance",
							 35 + 5 * bonus, false);
				get_property(NULL, obj, "hold life", 0, false);

				/* Combat bonuses */
				temp = 6 + randint1(4);
				get_property(NULL, obj, "deadliness bonus", temp, true);
				get_property(NULL, obj, "skill bonus", temp, true);

				/* Sometimes vulnerable to nexus or chaos */
				if (one_in_(8)) {
					get_property(NULL, obj, "nexus resistance",
								 -5 * randint0(bonus), false);
				}
				if (one_in_(8)) {
					get_property(NULL, obj, "chaos resistance",
								 -5 * randint0(bonus), false);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Vitality")) {
			/* min potential 3300 */

			/* This is an exclusive club */
			if (potential > 4500) {

				/* Nice resists */
				get_property(NULL, obj, "poison resistance",
							 35 + 5 * randint0(5), false);
				get_property(NULL, obj, "nether resistance",
							 35 + 5 * randint0(5), false);
				get_property(NULL, obj, "chaos resistance",
							 35 + 5 * randint0(5), false);
				get_property(NULL, obj, "hold life", 0, false);

				/* And an activation */
				temp = randint1(3);
				if (temp == 1) {
					get_activation("RAND_REGAIN", obj);
				} else if (temp == 2) {
					get_activation("RAND_RESTORE", obj);
				} else {
					get_activation("RAND_RESIST_ALL", obj);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Insight")) {
			/* min potential 16 */
			int property = 2;
			int max_value = randint1(5);
			const char *insight[3] = { "telepathy", "see invisible", "infravision" };

			/* Telepathy, See Invisible, or at least infravision */
			property = select_property(potential, insight, 3, &max_value,
									   RANK_DEAREST_FIRST, obj);
			get_property(NULL, obj, insight[property], max_value, true);
			done = true;
		} else if (streq(ego->name, "of the Elements")) {
			/* min potential 900 */

			/* This is an exclusive club */
			if (potential > 2000) {
				/* Brand, resistance, activation */
				temp = randint1(4);
				if (temp == 1) {
					get_property(NULL, obj, "acid brand", 14, true);
					get_property(NULL, obj, "acid resistance",
								 35 + 5 * bonus, true);
					obj->activation = lookup_activation("RING_ACID");
				} else if (temp == 2) {
					get_property(NULL, obj, "lightning brand", 14, true);
					get_property(NULL, obj, "electricity resistance",
								 35 + 5 * bonus, true);
					obj->activation = lookup_activation("RING_ELEC");
				} else if (temp == 3) {
					get_property(NULL, obj, "fire brand", 14, true);
					get_property(NULL, obj, "fire resistance",
								 35 + 5 * bonus, true);
					obj->activation = lookup_activation("RING_FIRE");
				} else {
					get_property(NULL, obj, "cold brand", 14, true);
					get_property(NULL, obj, "cold resistance",
								 35 + 5 * bonus, true);
					obj->activation = lookup_activation("RING_COLD");
				}
				obj->time.base = 50;
				obj->time.dice = 1;
				obj->time.sides = 100;
				done = true;
			}
		} else if (streq(ego->name, "of Physical Prowess")) {
			/* min potential 350 */
			int property = 0;
			int max_value = bonus + 1;
			const char *stats[3] = { "strength", "dexterity", "constitution" };
			const char *sustains[3] = { "sustain strength", "sustain dexterity",
								  "sustain constitution" };
			int tries = 5;

			/* Stat boost... */
			property = select_property(potential, stats, 3, &max_value,
									   RANK_RANDOM_CHOICE, obj);
			get_property(NULL, obj, stats[property], randint1(max_value), true);

			/* And corresponding sustain */
			get_property(NULL, obj, sustains[property], 0, true);

			/* Sometimes get another sustain */
			if (one_in_(3)) {
				property = select_property(potential, sustains, 3, &max_value,
										   RANK_RANDOM_CHOICE, obj);
				get_property(NULL, obj, sustains[property], 0, true);
			}

			/* Repeat while relatively good */
			while ((randint1(initial_potential) > initial_potential - potential)
				   && (tries > 0)) {
				max_value = bonus + 1;
				property = select_property(potential, stats, 3,	&max_value,
										   RANK_RANDOM_CHOICE, obj);
				get_property(NULL, obj, stats[property], randint1(max_value),
							 true);
				get_property(NULL, obj, sustains[property], 0, true);
				tries--;
			}
			done = true;
		} else if (streq(ego->name, "of Combat")) {
			/* min potential 580 */

			/* Combat bonuses */
			temp = 1 + bonus;
			get_property(NULL, obj, "skill bonus", temp + randint1(5), true);
			get_property(NULL, obj, "deadliness bonus", temp + randint1(5),
						 true);

			/* Particularly low level items become skill OR deadliness */
			if (randint1(initial_potential) > potential) {
				if (one_in_(2)) {
					obj->to_h += obj->to_d;
					obj->to_d = 0;
				} else {
					obj->to_d += obj->to_h;
					obj->to_h = 0;
				}
			}

			/* Maybe some stat boosts */
			temp = potential;
			if (randint1(temp) > (initial_potential / 6)) {
				get_property(NULL, obj, "strength", randint1(bonus), true);
			}
			if (randint1(temp) > (initial_potential / 6)) {
				get_property(NULL, obj, "dexterity", randint1(bonus), true);
			}

			/* And fearlessness */
			get_property(NULL, obj, "protection from fear", 0, true);

			/* Sometimes a form of aggravation */
			if (one_in_(5)) {
				append_object_curse(obj, lookup_curse("siren"),
									40 + randint0(20));
			} else if (one_in_(5)) {
				get_property(NULL, obj, "aggravation", 0, false);
			}
			done = true;
		} else if (streq(ego->name, "of Mobility")) {
			/* min potential 700 */
			int property;
			int max_value = 0;
			const char *mobility[6] =	{ "see invisible", "protection from fear",
								  "protection from blindness",
								  "protection from confusion",
								  "nexus resistance", "trap immunity" };

			/* This is an exclusive club */
			if (potential > 500) {
				/* Free action... */
				get_property(NULL, obj, "free action", 0, true);

				/* ... and more nice properties, if affordable */
				property = select_property(potential, mobility, 5, &max_value,
										   RANK_RANDOM_CHOICE, obj);
				get_property(NULL, obj, mobility[property], 35 + 5 * bonus,
							 false);
				done = true;
			}
		} else if (streq(ego->name, "of Arcane Resistance")) {
			/* min potential 440 */

			/* This is an exclusive club */
			if (potential > 2000) {
				/* Poison or nether resist */
				if (one_in_(2)) {
					get_property(NULL, obj, "poison resistance",
								 40 + 5 * randint0(bonus), true);
				} else {
					get_property(NULL, obj, "nether resistance",
								 40 + 5 * randint0(bonus), true);
				}

				/* Rack 'em up while we're feeling lucky */
				while (randint1(potential) > 1000) {
					int choice = randint0(9);
					const char *resists[9] = { "poison resistance",
										 "light resistance",
										 "dark resistance", "sound resistance",
										 "shards resistance",
										 "nexus resistance",
										 "nether resistance",
										 "chaos resistance",
										 "disenchantment resistance" };

					if (obj->el_info[ELEM_POIS + choice].res_level ==
						RES_LEVEL_BASE) {
						get_property(NULL, obj, resists[choice],
									 30 + 5 * randint0(bonus), true);
					}
				}
				done = true;
			}
		} else if (streq(ego->name, "of Utility")) {
			/* min potential 21 */
			int property;
			int max_value = 5;
			const char *utility[5] = { "searching skill", "slow digestion",
								 "feather falling", "light", "regeneration" };
			int tries = randint1(bonus + 2);

			/* Pick a few properties */
			if (tries > 6) {
				tries = 6;
			}
			for (j = 0; j < tries; j++) {
				property = select_property(potential, utility, 5, &max_value,
										   RANK_RANDOM_CHOICE, obj);
				get_property(NULL, obj, utility[property], max_value, true);
			}

			/* And maybe a useful activation, if some potential left  */
			if (potential > 300) {
				temp = randint1(6);
				if (temp == 1) {
					get_activation("RAND_DISARM", obj);
				}
				if (temp == 2) {
					get_activation("RAND_RECALL", obj);
				}
				if (temp == 3) {
					get_activation("RAND_IDENTIFY", obj);
				}
			}
			done = true;
		} else if (streq(ego->name, "of Hindrance")) {
			/* min potential 0 */
			int tries = randint1(3);
			int max_value = 5;
			int property;
			const char *stats[5] = { "strength", "wisdom", "intelligence",
							   "dexterity", "constitution" };

			/* Too much or too little movement */
			if (one_in_(4)) {
				append_object_curse(obj, lookup_curse("teleportation"),
									40 + randint0(20));
			} else if (one_in_(3)) {
				append_object_curse(obj, lookup_curse("anti-teleportation"),
									40 + randint0(20));
			}

			/* Cripple some stats */
			for (j = 0; j < tries; j++) {
				property = select_property(potential, stats, 5,	&max_value,
										   RANK_RANDOM_CHOICE, obj);
				get_property(NULL, obj, stats[property], 0 - randint1(5), true);
			}

			/* Potential gained counts as lost */
			if (potential - initial_potential > initial_potential) {
				potential = 0;
			} else {
				potential = (2 * initial_potential) - potential;
			}
			done = true;
		} else if (streq(ego->name, "of the Dawn")) {
			/* min potential 1300 */

			/* This is an exclusive club */
			if (potential > 2000) {

				/* Power of seeing */
				get_property(NULL, obj, "protection from blindness", 0, true);
				get_property(NULL, obj, "light resistance",
							 40 + 5 * randint0(6), true);
				get_property(NULL, obj, "dark resistance",
							 40 + 5 * randint0(6), true);

				/* Maybe see invisible, if affordable */
				if (one_in_(3)) {
					get_property(NULL, obj, "see invisible", 0, false);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Speed")) {
			const char *speed[1] = { "speed" };
			int max_value = 14;

			/* This is an exclusive club */
			if (potential > 3000) {
				/* SPEED! */
				(void) select_property(potential, speed, 1, &max_value,
									   RANK_FIND_MAX_VALUE, obj);

				/* Either sacrifice for some other properties... */
				if (one_in_(2)) {
					get_property(NULL, obj, "speed", 1 + max_value / 2, true);

					/* Maybe get combat bonuses... */
					if (one_in_(4)) {
						temp = 2 + randint1(bonus);
						get_property(NULL, obj, "deadliness bonus", temp, true);
						get_property(NULL, obj, "skill bonus", temp, true);
					} else if (one_in_(3)) {
						/* ...or a high resist... */
						int choice;
						struct obj_property *prop = NULL;
						random_high_resist(obj, &choice);
						prop = lookup_obj_property(OBJ_PROPERTY_ELEMENT,choice);
						if (obj->el_info[choice].res_level == RES_LEVEL_BASE) {
							get_property(NULL, obj, prop->name,
										 30 + 5 * randint0(5), true);
						}
					} else if (one_in_(2)) {
						/* ...or nice power */
						bitflag flags[OF_SIZE];
						struct obj_property *prop = NULL;
						create_obj_flag_mask(flags, false, OFT_PROT, OFT_MISC,
											 OFT_MAX);
						prop = lookup_obj_property(OBJ_PROPERTY_FLAG,
												   get_new_attr(obj->flags,
																flags));
						get_property(NULL, obj, prop->name, 0, true);
					}
				} else {
					/* ...or go all out */
					while (one_in_(2)) {
							max_value++;
					}
					get_property(NULL, obj, "speed", max_value, true);
				}
				done = true;
			}
		} else if (streq(ego->name, "of Woe")) {
			/* min potential 0 */

			/* Don't find these too early */
			if (potential > 1500) {

				/* Curses */
				append_object_curse(obj, lookup_curse("teleportation"),
									40 + randint0(20));
				if (one_in_(2)) {
					append_object_curse(obj, lookup_curse("impair mana recovery"),
										40 + randint0(20));
				} else {
					append_object_curse(obj, lookup_curse("impair hitpoint recovery"),
										40 + randint0(20));
				}

				/* Hit stats */
				get_property(NULL, obj, "wisdom", 0 - bonus, true);
				get_property(NULL, obj, "intelligence", 0 - bonus, true);

				/* Some advantages */
				get_property(NULL, obj, "nexus resistance",
							 5 * randint0(8), true);
				get_property(NULL, obj, "free action", 0, true);

				/* Reduce chance to recover */
				potential /= 2;
				done = true;
			}
		} else if (streq(ego->name, "of Fickleness")) {
			/* min potential 0 */
			const char *fickle[7] = { "teleportation", "anti-teleportation",
								"cuts", "hallucination", "siren",
								"demon summon", "paralysis" };
			while (1) {
				append_object_curse(obj, lookup_curse(fickle[randint0(7)]),
									40 + randint0(20));
				if (one_in_(3))
					break;
			}
			done = true;
		} else if (streq(ego->name, "of Power")) {
			/* min potential >> 4000 */
			int element = randint0(4);

			/* This is an exclusive club */
			if (potential > 4000) {

				/* Free bonuses to Deadliness, Skill and Armour Class */
				temp = 3 + randint1(5) + potential / 2000;
				obj->to_d += temp;
				obj->to_h += temp;
				obj->to_a += 3 + randint1(5) + potential / 1000;

				/* Power over an element */
				switch (element) {
					case 0: {
						get_property(NULL, obj, "acid resistance", 100, false);
						break;
					}
					case 1: {
						get_property(NULL, obj, "electricity resistance", 100,
									 false);
						break;
					}
					case 2: {
						get_property(NULL, obj, "fire resistance", 100, false);
						break;
					}
					case 3: {
						get_property(NULL, obj, "cold resistance", 100, false);
						break;
					}
				}

				/* Sometimes vulnerable to high elements... */
				if (one_in_(8)) {
					get_property(NULL, obj, "nexus resistance",
								 -5 * randint0(4), false);
				}
				if (one_in_(8)) {
					get_property(NULL, obj, "nether resistance",
								 -5 * randint0(4), false);
				}
				if (one_in_(8)) {
					get_property(NULL, obj, "chaos resistance",
								 -5 * randint0(4), false);
				}
				if (one_in_(8)) {
					get_property(NULL, obj, "disenchantment resistance",
								 -5 * randint0(4), false);
				}

				/* ...but likely to sustain stats */
				if (one_in_(3)) {
					get_property(NULL, obj, "sustain strength", 0, false);
				}
				if (one_in_(3)) {
					get_property(NULL, obj, "sustain wisdom", 0, false);
				}
				if (one_in_(3)) {
					get_property(NULL, obj, "sustain intelligence", 0, false);
				}
				if (one_in_(3)) {
					get_property(NULL, obj, "sustain dexterity", 0, false);
				}
				if (one_in_(3)) {
					get_property(NULL, obj, "sustain constitution", 0, false);
				}

				/* And with a powerful activation at a bargain price... */
				if (one_in_(2)) {
					/* ...either thematic for this element... */
					switch (element) {
						case 0: {
							get_activation("ACID_BLAST", obj);
							break;
						}
						case 1: {
							get_activation("CHAIN_LIGHTNING", obj);
							break;
						}
						case 2: {
							get_activation("LAVA_POOL", obj);
							break;
						}
						case 3: {
							get_activation("ICE_WHIRLPOOL", obj);
							break;
						}
					}
				} else {
					/* ... or a random powerful effect. */
					get_random_activation(ACTIVATION_POWER, obj);
				}

				/* ...plus help to activate it */
				get_property(NULL, obj, "magic mastery", bonus, true);
				done = true;
			}
		}
	}

	/* Limit off-type features */
	potential /= 2;

	obj->ego = ego;
	return (done);
}

/**
 * Design a ring or amulet.
 */
bool design_jewellery(struct object *obj, int lev)
{

	/* Assign it a potential. */
	allocate_potential(obj, lev);

	/* Assign a type. */
	if (!choose_type(obj))
		return false;

	/* Add extra qualities until done. */
	haggle_till_done(NULL, obj);

	/* Decide if the object is to be terrible. */
	if (TERRIBLE_CHANCE > randint0(100)) {

		/* If it is, add a few benefits and more drawbacks. */
		make_terrible(NULL, obj);
	}

	/* Remove contradictory powers. */
	final_check(NULL, obj);

	return true;
}
