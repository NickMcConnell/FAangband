/**
 * \file obj-properties.c
 * \brief functions to deal with object properties
 *
 * Copyright (c) 2014 Chris Carr
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
#include "init.h"
#include "obj-curse.h"
#include "obj-knowledge.h"
#include "obj-slays.h"
#include "obj-tval.h"
#include "object.h"

/**
 * ------------------------------------------------------------------------
 * Object property utilities
 * ------------------------------------------------------------------------ */
struct obj_property *obj_properties;
struct activation *activations;

struct obj_property *lookup_obj_property(int type, int index)
{
	struct obj_property *prop;
	int i;

	/* Find the right property */
	for (i = 1; i < z_info->property_max; i++) {
		prop = &obj_properties[i];
		if ((prop->type == type) && (prop->index == index)) {
			return prop;
		}

		/* Special case - stats count as mods */
		if ((type == OBJ_PROPERTY_MOD) && (prop->type == OBJ_PROPERTY_STAT)
			&& (prop->index == index)) {
			return prop;
		}
	}

	return NULL;
}

struct obj_property *lookup_obj_property_name(const char *prop_name)
{
	struct obj_property *prop;
	int i;

	/* Find the right property */
	for (i = 1; i < z_info->property_max; i++) {
		prop = &obj_properties[i];
		if (streq(prop->name, prop_name)) {
			return prop;
		}
	}

	return NULL;
}

struct activation *lookup_activation(const char *act_name)
{
	struct activation *act = &activations[1];
	while (act) {
		if (streq(act->name, act_name))
			break;
		act = act->next;
	}
	return act;
}

/**
 * Create a "mask" of object flags of a specific type or ID threshold.
 *
 * \param f is the flag array we're filling
 * \param id is whether we're masking by ID level
 * \param ... is the list of flags or ID types we're looking for
 *
 * N.B. OFT_MAX must be the last item in the ... list
 */
void create_obj_flag_mask(bitflag *f, int id, ...)
{
	int i, j;
	va_list args;

	of_wipe(f);

	va_start(args, id);

	/* Process each type in the va_args */
    for (i = va_arg(args, int); i != OFT_MAX; i = va_arg(args, int)) {
		for (j = 1; j < z_info->property_max; j++) {
			struct obj_property *prop = &obj_properties[j];
			if (prop->type != OBJ_PROPERTY_FLAG) continue;
			if ((id && prop->id_type == i) || (!id && prop->subtype == i)) {
				of_on(f, prop->index);
			}
		}
	}

	va_end(args);

	return;
}

/**
 * Print a message when an object flag is identified by use.
 *
 * \param flag is the flag being noticed
 * \param name is the object name 
 */
void flag_message(int flag, char *name)
{
	struct obj_property *prop = lookup_obj_property(OBJ_PROPERTY_FLAG, flag);
	char buf[1024] = "\0";
	const char *next;
	const char *s;
	const char *tag;
	const char *in_cursor;
	size_t end = 0;

	/* See if we have a message */
	if (!prop->msg) return;
	in_cursor = prop->msg;

	next = strchr(in_cursor, '{');
	while (next) {
		/* Copy the text leading up to this { */
		strnfcat(buf, 1024, &end, "%.*s", next - in_cursor, in_cursor); 

		s = next + 1;
		while (*s && isalpha((unsigned char) *s)) s++;

		/* Valid tag */
		if (*s == '}') {
			/* Start the tag after the { */
			tag = next + 1;
			in_cursor = s + 1;
			if (strncmp(tag, "name", 4) == 0) {
				strnfcat(buf, 1024, &end, "%s", name); 
			}

		} else {
			/* An invalid tag, skip it */
			in_cursor = next + 1;
		}

		next = strchr(in_cursor, '{');
	}
	strnfcat(buf, 1024, &end, in_cursor);

	msg("%s", buf);
}

/**
 * Return the sustain flag of a given stat.
 */
int sustain_flag(int stat)
{
	if (stat < 0 || stat >= STAT_MAX) return -1;

	return stat + 1;
}

/**
 * Object property corresponding to a brand
 */
struct obj_property *brand_property(int brand_idx)
{
	int i;

	for (i = 1; i < z_info->property_max; i++) {
		struct obj_property *prop =	&obj_properties[i];
		char *s = strstr(prop->name, brands[brand_idx].name);
		if (s) return prop;
	}
	return NULL;
}

/**
 * Object property corresponding to a slay
 */
struct obj_property *slay_property(int slay_idx)
{
	int i;

	for (i = 1; i < z_info->property_max; i++) {
		struct obj_property *prop =	&obj_properties[i];
		char *s = strstr(prop->name, slays[slay_idx].name);
		if (s) return prop;
	}
	return NULL;
}

/**
 * Check if an object has a given object property
 */
static bool object_has_property(const struct object *obj, struct obj_property *prop)
{
	int i, idx = prop->index;
	switch (prop->type) {
		case OBJ_PROPERTY_STAT :
		case OBJ_PROPERTY_MOD :
		{
			return obj->modifiers[idx] ? true : false;
			break;
		}
		case OBJ_PROPERTY_FLAG :
		{
			return of_has(obj->flags, idx);
			break;
		}
		case OBJ_PROPERTY_ELEMENT :
		{
			return (RES_LEVEL_BASE != obj->el_info[idx].res_level);
			break;
		}
		case OBJ_PROPERTY_IGNORE :
		{
			return obj->el_info[idx].flags & EL_INFO_IGNORE ? true : false;
			break;
		}
		case OBJ_PROPERTY_BRAND :
		{
			if (obj->brands) {
				for (i = 1; i < z_info->brand_max; i++) {
					if (obj->brands[i] && (prop == brand_property(i))) {
						return true;
					}
				}
			}
			break;
		}
		case OBJ_PROPERTY_SLAY :
		{
			if (obj->slays) {
				for (i = 1; i < z_info->slay_max; i++) {
					if (prop == slay_property(i)) {
						return true;
					}
				}
			}
			break;
		}
		case OBJ_PROPERTY_COMBAT :
		{
			if (streq(prop->name, "enhanced dice")) {
				return ((obj->dd > obj->kind->dd) || (obj->ds > obj->kind->ds));
			} else if (streq(prop->name, "extra armor")) {
				return (obj->ac > obj->kind->ac);
			} else if (streq(prop->name, "armor bonus")) {
				return (obj->to_a != 0);
			} else if (streq(prop->name, "skill bonus")) {
				return (obj->to_h != 0);
			} else if (streq(prop->name, "deadliness bonus")) {
				return (obj->to_d != 0);
			}
			break;
		}
		default : break;
	}
	return false;
}

/**
 * ------------------------------------------------------------------------
 * Object property values for random artifacts and jewellery
 * ------------------------------------------------------------------------ */
/**
 * ------------------------------------------------------------------------
 * Object pricing
 * ------------------------------------------------------------------------ */
/**
 * Calculate cost of a property with a given value.
 *
 * Cost is typically a quadratic in the numeric value being added.
 * There are two types of cost: design cost (in potential) for design of
 * random artifacts and jewellery; and money cost for buying and selling.
 */
int property_cost(struct obj_property *prop, int value, bool price)
{
	if (price) {
		return prop->price_constant + prop->price_linear * value +
			prop->price_square * value * ((value > 0) ? value : (0 - value));
	}

	return prop->design_constant + prop->design_linear * value +
		prop->design_square * value * value;
}

/**
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static int object_value_base(const struct object *obj)
{
	/* Use template cost for aware objects */
	if (object_flavor_is_aware(obj))
		return obj->kind->cost;

	/* Analyze the type */
	switch (obj->tval)
	{
		case TV_FOOD:
		case TV_MUSHROOM:
			return 5;
		case TV_POTION:
		case TV_SCROLL:
			return 20;
		case TV_RING:
		case TV_AMULET:
			return 45;
		case TV_WAND:
			return 50;
		case TV_STAFF:
			return 70;
		case TV_ROD:
			return 90;
	}

	return 0;
}


/**
 * Return the real price of a known (or partly known) item.
 *
 * Wand and staffs get cost for each charge.
 *
 * Wearable items (weapons, launchers, jewelry, lights, armour) and ammo
 * are priced according to their properties.
 */
int object_value_real(const struct object *obj, int qty)
{
	int value, total_value = 0;

	/* Wearables and ammo have prices that vary by individual item properties */
	if (tval_has_variable_power(obj)) {
		int i, j;

		/* Base cost */
		value = obj->kind->cost;

		/* Add/subtract value for properties */
		for (i = 1; i < z_info->property_max; i++) {
			struct obj_property *prop =	&obj_properties[i];
			int idx = prop->index;
			if (!object_has_property(obj, prop)) continue;
			switch (prop->type) {
				case OBJ_PROPERTY_STAT :
				case OBJ_PROPERTY_MOD :
				{
					value += property_cost(prop, obj->modifiers[idx], true);
					break;
				}
				case OBJ_PROPERTY_FLAG :
				{
					value += property_cost(prop, 0, true);
					break;
				}
				case OBJ_PROPERTY_ELEMENT :
				{
					int amount = RES_LEVEL_BASE - obj->el_info[idx].res_level;
					value += property_cost(prop, amount, true);
					break;
				}
				case OBJ_PROPERTY_IGNORE :
				{
					value += property_cost(prop, 0, true);
					break;
				}
				case OBJ_PROPERTY_BRAND :
				{
					for (j = 0; j < z_info->brand_max; j++) {
						if (obj->brands[j] && (prop == brand_property(j))) {
							int mult = brands[j].multiplier;
							int div = tval_is_ammo(obj) ? 10 : 1;
							int32_t bonus = property_cost(prop, mult, true) / div;
							/* Weapons with big dice are just better, so
							 * anything that enhances attacks is worth more on
							 them */
							if ((tval_is_melee_weapon(obj)) || (div == 10))	{
								bonus *= ((obj->dd + 1) * (obj->ds));
								bonus /= 24;
							}
							value += bonus;
						}
					}
					break;
				}
				case OBJ_PROPERTY_SLAY :
				{
					for (j = 0; j < z_info->slay_max; j++) {
						if (obj->slays[j] && (prop == slay_property(j))) {
							int mult = slays[j].multiplier;
							int div = tval_is_ammo(obj) ? 10 : 1;
							int32_t bonus = property_cost(prop, mult, true) / div;
							/* Weapons with big dice are just better, so
							 * anything that enhances attacks is worth more on
							 them */
							if ((tval_is_melee_weapon(obj)) || (div == 10))
							{
								bonus *= ((obj->dd + 1) * (obj->ds));
								bonus /= 24;
							}
							/* Cheap check for brands with broader applicability
							 * that might reduce the slay's practical value */
							if ((obj->brands) &&
								(!streq(prop->name, "slay evil creatures"))) {
								int k;
								bool reduce_bonus = false;
								for (k = 1; k < z_info->brand_max; k++)
								{
									if (!obj->brands[k]) continue;
									if (strstr(brands[k].name, "poison brand"))
										continue;
									if ((strstr(brands[k].name, "cold brand"))
										&& (streq(prop->name, "slay undead")))
										continue;
									reduce_bonus = true;
									break;
								}
								if (reduce_bonus) bonus -= (bonus / 4);
							}
							value += bonus;
						}
					}
					break;
				}
				case OBJ_PROPERTY_COMBAT :
				{
					struct object_kind *kind = obj->kind;
					if (streq(prop->name, "enhanced dice")) {
						value += (obj->ds - kind->ds) * (obj->ds - kind->ds)
							* obj->dd * obj->dd * 16L;
					} else if (streq(prop->name, "extra armor")) {
						value += property_cost(prop, obj->ac - kind->ac, true);
					} else if (streq(prop->name, "armor bonus")) {
						value += property_cost(prop, obj->to_a, true);
					} else if (streq(prop->name, "skill bonus")) {
						value += property_cost(prop, obj->to_h- kind->to_h.base,
											   true) /
							(tval_is_ammo(obj) ? 20 : 1);
					} else if (streq(prop->name, "deadliness bonus")) {
						value += property_cost(prop, obj->to_d, true) /
							(tval_is_ammo(obj) ? 20 : 1);
					}
					break;
				}
				default : break;
			}
		}

		/* Amend cost for curses */
		if (obj->curses) {
			for (i = 0; i < z_info->curse_max; i++) {
				if (obj->curses[i].power) {
					value += object_value_real(curses[i].obj, 1);
				}
			}
		}

		total_value = value;
	} else {
		/* Base cost */
		if (obj->artifact) {
			value = obj->artifact->cost;
		} else {
			value = obj->kind->cost;
		}

		/* Worthless items */
		if (!value) return (0L);

		/* Account for quantity */
		total_value = value * qty;

		/* Charge-holding devices need special treatment */
		if (tval_can_have_charges(obj)) {
			int charges;

			/* Calculate number of charges, rounded up */
			charges = obj->pval * qty / obj->number;
			if ((obj->pval * qty) % obj->number != 0)
				charges++;

			/* Pay extra for charges, depending on standard number of charges */
			total_value += value * charges / 20;
		}

		/* No negative value */
		if (total_value < 0) total_value = 0;
	}

	/* Return the value */
	return total_value;
}


/**
 * Return the price of an item including plusses (and charges).
 *
 * This function returns the "value" of the given item (qty one).
 *
 * Never notice unknown bonuses or properties, including curses,
 * since that would give the player information they did not have.
 */
int object_value(const struct object *obj, int qty)
{
	int value;

	/* Variable power items are assessed by what is known about them */
	if (tval_has_variable_power(obj) && obj->known) {
		value = object_value_real(obj->known, qty);
	} else if (tval_can_have_flavor_k(obj->kind) &&
			   object_flavor_is_aware(obj)) {
		value = object_value_real(obj, qty);
	} else {
		/* Unknown constant-price items just get a base value */
		value = object_value_base(obj) * qty;
	}

	/* Return the final value */
	return (value);
}
