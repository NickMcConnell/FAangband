/** \file jewel.c
    \brief Jewellery generation.

 * Random rings and amulets.  Selling and providing qualities.   Selecting a 
 * theme and corresponding name, and the properties of all possible themes.  
 * Adding semi-random qualities until potential is all used up.  Removing 
 * contradictory flags.  
 *
 * Much of this is borrowed from randart.c.
 *
 * Copyright (c) 2010 Nick McConnell
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
#include "jewel.h"
#include "effects.h"
#include "types.h"

/** A global variable whose contents will be bartered to acquire powers. */
static int potential = 0;

/** The initial potential of the object. */
static int initial_potential = 0;

/** Percentage chance for an object to be terrible. */
#define TERRIBLE_CHANCE		5

/** Multiplier to get activation cost from effect power */
#define EFFECT_MULT             80

/** Maximum and minimum effects for random activation from effects.c */
#define EF_RAND_BASE          EF_RAND_FIRE1
#define EF_RAND_MAX           EF_RAND_SLOW_FOES
#define EF_POWER_BASE         EF_ACID_BLAST
#define EF_POWER_MAX          EF_TELEPORT_ALL

/**
 * Debit an object's account.
 */
static bool take_money(bool on_credit, int cost)
{

	/* Take the money. */
	if (potential > cost - 1) {
		potential -= cost;
		return (TRUE);
	}

	/* <<mutter>> OK, I'll charge it to your account... */
	else if (on_credit) {
		potential = 0;
		return (TRUE);
	}

	/* Otherwise, kick da bum out. */
	else
		return (FALSE);
}


/**
 * Grant the quality asked for, if the object can afford it.
 */
static bool get_quality(bool on_credit, int purchase, int value,
						object_type * o_ptr)
{
	int new, current, diff_cost;
	switch (purchase) {
	case ADD_STR:
		{

			/* Check existing bonuses */
			current = o_ptr->bonus_stat[A_STR];
			new = current + value;
			diff_cost = (150 * new) + (new * new * 8)
				- (150 * current) + (current * current * 8);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_stat[A_STR] += value;
				return (TRUE);
			}
			break;
		}
	case ADD_WIS:
		{
			current = o_ptr->bonus_stat[A_WIS];
			new = current + value;
			if (mp_ptr->spell_stat == A_WIS) {
				diff_cost = (150 * new) + (new * new * 8)
					- (150 * current) + (current * current * 8);
				if (take_money(on_credit, diff_cost)) {
					o_ptr->bonus_stat[A_WIS] += value;
					return (TRUE);
				}
			}

			else {
				diff_cost = (100 * new) + (new * new * 5)
					- (100 * current) + (current * current * 5);
				if (take_money(on_credit, diff_cost)) {
					o_ptr->bonus_stat[A_WIS] += value;
					return (TRUE);
				}
			}
			break;
		}
	case ADD_INT:
		{
			current = o_ptr->bonus_stat[A_INT];
			new = current + value;
			if (mp_ptr->spell_stat == A_INT) {
				diff_cost = (150 * new) + (new * new * 8)
					- (150 * current) + (current * current * 8);
				if (take_money(on_credit, diff_cost)) {
					o_ptr->bonus_stat[A_INT] += value;
					return (TRUE);
				}
			}

			else {
				diff_cost = (100 * new) + (new * new * 5)
					- (100 * current) + (current * current * 5);
				if (take_money(on_credit, diff_cost)) {
					o_ptr->bonus_stat[A_INT] += value;
					return (TRUE);
				}
			}
			break;
		}
	case ADD_DEX:
		{
			current = o_ptr->bonus_stat[A_DEX];
			new = current + value;
			diff_cost = (130 * new) + (new * new * 7)
				- (130 * current) + (current * current * 7);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_stat[A_DEX] += value;
				return (TRUE);
			}
			break;
		}
	case ADD_CON:
		{
			current = o_ptr->bonus_stat[A_CON];
			new = current + value;
			diff_cost = (130 * new) + (new * new * 7)
				- (130 * current) + (current * current * 7);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_stat[A_CON] += value;
				return (TRUE);
			}
			break;
		}
	case ADD_CHR:
		{
			current = o_ptr->bonus_stat[A_CHR];
			new = current + value;
			diff_cost = (40 * new) + (new * new * 2)
				- (40 * current) + (current * current * 2);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_stat[A_CHR] += value;
				return (TRUE);
			}
			break;
		}
	case MAGIC_MASTERY:
		{
			current = o_ptr->bonus_other[P_BONUS_M_MASTERY];
			new = current + value;
			diff_cost = (100 * new) + (new * new * 5)
				- (100 * current) + (current * current * 5);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_M_MASTERY] += value;
				return (TRUE);
			}
			break;
		}
	case STEALTH:
		{
			current = o_ptr->bonus_other[P_BONUS_STEALTH];
			new = current + value;
			diff_cost = (50 * new) + (new * new * 3)
				- (50 * current) + (current * current * 3);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_STEALTH] += value;
				return (TRUE);
			}
			break;
		}
	case SEARCH:
		{
			current = o_ptr->bonus_other[P_BONUS_SEARCH];
			new = current + value;
			diff_cost = (20 * new) + (new * new * 1)
				- (20 * current) + (current * current * 1);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_SEARCH] += value;
				return (TRUE);
			}
			break;
		}
	case INFRA:
		{
			current = o_ptr->bonus_other[P_BONUS_INFRA];
			new = current + value;
			diff_cost = (15 * new) + (new * new * 1)
				- (15 * current) + (current * current * 1);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_INFRA] += value;
				return (TRUE);
			}
			break;
		}
	case TUNNEL:
		{
			current = o_ptr->bonus_other[P_BONUS_TUNNEL];
			new = current + value;
			diff_cost = (25 * new) + (new * new * 1)
				- (25 * current) + (current * current * 1);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_TUNNEL] += value;
				return (TRUE);
			}
			break;
		}
	case SPEED:
		{
			current = o_ptr->bonus_other[P_BONUS_SPEED];
			new = current + value;
			diff_cost = (360 * new) + (new * new * 18)
				- (360 * current) + (current * current * 18);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_SPEED] += value;
				return (TRUE);
			}
			break;
		}
	case MIGHT:
		{
			current = o_ptr->bonus_other[P_BONUS_MIGHT];
			new = current + value;
			diff_cost = (1250 * new) - (1250 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_MIGHT] += value;
				return (TRUE);
			}
			break;
		}
	case SHOTS:
		{
			current = o_ptr->bonus_other[P_BONUS_SHOTS];
			new = current + value;
			diff_cost = (1500 * new) - (1500 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->bonus_other[P_BONUS_SHOTS] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_ANIMAL:
		{
			current = o_ptr->multiple_slay[P_SLAY_ANIMAL];
			new = current + value;
			diff_cost = (60 * new) - (60 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_ANIMAL] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_EVIL:
		{
			current = o_ptr->multiple_slay[P_SLAY_EVIL];
			new = current + value;
			diff_cost = (100 * new) - (100 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_EVIL] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_UNDEAD:
		{
			current = o_ptr->multiple_slay[P_SLAY_UNDEAD];
			new = current + value;
			diff_cost = (60 * new) - (60 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_UNDEAD] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_DEMON:
		{
			current = o_ptr->multiple_slay[P_SLAY_DEMON];
			new = current + value;
			diff_cost = (50 * new) - (50 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_DEMON] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_ORC:
		{
			current = o_ptr->multiple_slay[P_SLAY_ORC];
			new = current + value;
			diff_cost = (40 * new) - (40 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_ORC] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_TROLL:
		{
			current = o_ptr->multiple_slay[P_SLAY_TROLL];
			new = current + value;
			diff_cost = (50 * new) - (50 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_TROLL] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_GIANT:
		{
			current = o_ptr->multiple_slay[P_SLAY_GIANT];
			new = current + value;
			diff_cost = (40 * new) - (40 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_GIANT] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_DRAGON:
		{
			current = o_ptr->multiple_slay[P_SLAY_DRAGON];
			new = current + value;
			diff_cost = (60 * new) - (60 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_slay[P_SLAY_DRAGON] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_POIS:
		{
			current = o_ptr->multiple_brand[P_BRAND_POIS];
			new = current + value;
			diff_cost = (100 * new) - (100 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_brand[P_BRAND_POIS] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_ACID:
		{
			current = o_ptr->multiple_brand[P_BRAND_ACID];
			new = current + value;
			diff_cost = (100 * new) - (100 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_brand[P_BRAND_ACID] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_ELEC:
		{
			current = o_ptr->multiple_brand[P_BRAND_ELEC];
			new = current + value;
			diff_cost = (100 * new) - (100 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_brand[P_BRAND_ELEC] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_FIRE:
		{
			current = o_ptr->multiple_brand[P_BRAND_FIRE];
			new = current + value;
			diff_cost = (100 * new) - (100 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_brand[P_BRAND_FIRE] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_COLD:
		{
			current = o_ptr->multiple_brand[P_BRAND_COLD];
			new = current + value;
			diff_cost = (100 * new) - (100 * current);
			if (take_money(on_credit, diff_cost)) {
				o_ptr->multiple_brand[P_BRAND_COLD] += value;
				return (TRUE);
			}
			break;
		}
	case SUST_STR:
		{
			if (take_money(on_credit, 300)) {
				of_on(o_ptr->flags_obj, OF_SUSTAIN_STR);
				return (TRUE);
			}
			break;
		}
	case SUST_WIS:
		{
			if (take_money(on_credit, 200)) {
				of_on(o_ptr->flags_obj, OF_SUSTAIN_WIS);
				return (TRUE);
			}
			break;
		}
	case SUST_INT:
		{
			if (take_money(on_credit, 200)) {
				of_on(o_ptr->flags_obj, OF_SUSTAIN_INT);
				return (TRUE);
			}
			break;
		}
	case SUST_DEX:
		{
			if (take_money(on_credit, 200)) {
				of_on(o_ptr->flags_obj, OF_SUSTAIN_DEX);
				return (TRUE);
			}
			break;
		}
	case SUST_CON:
		{
			if (take_money(on_credit, 200)) {
				of_on(o_ptr->flags_obj, OF_SUSTAIN_CON);
				return (TRUE);
			}
			break;
		}
	case SUST_CHR:
		{
			if (take_money(on_credit, 20)) {
				of_on(o_ptr->flags_obj, OF_SUSTAIN_CHR);
				return (TRUE);
			}
			break;
		}
	case SLOW_DIGEST:
		{
			if (take_money(on_credit, 300)) {
				of_on(o_ptr->flags_obj, OF_SLOW_DIGEST);
				return (TRUE);
			}
			break;
		}
	case FEATHER:
		{
			if (take_money(on_credit, 300)) {
				of_on(o_ptr->flags_obj, OF_FEATHER);
				return (TRUE);
			}
			break;
		}
	case LIGHT:
		{
			if (take_money(on_credit, 300)) {
				of_on(o_ptr->flags_obj, OF_LIGHT);
				return (TRUE);
			}
			break;
		}
	case REGEN:
		{
			if (take_money(on_credit, 500)) {
				of_on(o_ptr->flags_obj, OF_REGEN);
				return (TRUE);
			}
			break;
		}
	case TELEPATHY:
		{
			if (take_money(on_credit, 2500)) {
				of_on(o_ptr->flags_obj, OF_TELEPATHY);
				return (TRUE);
			}
			break;
		}
	case SEE_INVIS:
		{
			if (take_money(on_credit, 700)) {
				of_on(o_ptr->flags_obj, OF_SEE_INVIS);
				return (TRUE);
			}
			break;
		}
	case FREE_ACT:
		{
			if (take_money(on_credit, 700)) {
				of_on(o_ptr->flags_obj, OF_FREE_ACT);
				return (TRUE);
			}
			break;
		}
	case HOLD_LIFE:
		{
			if (take_money(on_credit, 1000)) {
				of_on(o_ptr->flags_obj, OF_HOLD_LIFE);
				return (TRUE);
			}
			break;
		}
	case SEEING:
		{
			if (take_money(on_credit, 650)) {
				of_on(o_ptr->flags_obj, OF_SEEING);
				return (TRUE);
			}
			break;
		}
	case FEARLESS:
		{
			if (take_money(on_credit, 500)) {
				of_on(o_ptr->flags_obj, OF_FEARLESS);
				return (TRUE);
			}
			break;
		}
	case DARKNESS:
		{
			if (take_money(on_credit, 800)) {
				of_on(o_ptr->flags_obj, OF_DARKNESS);
				return (TRUE);
			}
			break;
		}
	case ELEC_PROOF:
		{
			if (take_money(on_credit, 100)) {
				of_on(o_ptr->flags_obj, OF_ELEC_PROOF);
				return (TRUE);
			}
			break;
		}
	case CHAOTIC:
		{
			if (take_money(on_credit, 1200)) {
				of_on(o_ptr->flags_obj, OF_CHAOTIC);
				return (TRUE);
			}
			break;
		}
	case IM_ACID:
		{
			if (take_money(on_credit, 3500)) {
				o_ptr->percent_res[P_RES_ACID] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case IM_ELEC:
		{
			if (take_money(on_credit, 2750)) {
				o_ptr->percent_res[P_RES_ELEC] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case IM_FIRE:
		{
			if (take_money(on_credit, 3250)) {
				o_ptr->percent_res[P_RES_FIRE] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case IM_COLD:
		{
			if (take_money(on_credit, 2750)) {
				o_ptr->percent_res[P_RES_COLD] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case RES_ACID:
		{
			current = (o_ptr->percent_res[P_RES_ACID] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_ACID] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_ELEC:
		{
			current = (o_ptr->percent_res[P_RES_ELEC] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_ELEC] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_FIRE:
		{
			current = (o_ptr->percent_res[P_RES_FIRE] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_FIRE] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_COLD:
		{
			current = (o_ptr->percent_res[P_RES_COLD] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_COLD] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_POIS:
		{
			current = (o_ptr->percent_res[P_RES_POIS] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 100 * (current - value))) {
				o_ptr->percent_res[P_RES_POIS] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_LIGHT:
		{
			current = (o_ptr->percent_res[P_RES_LIGHT] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 60 * (current - value))) {
				o_ptr->percent_res[P_RES_LIGHT] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_DARK:
		{
			current = (o_ptr->percent_res[P_RES_DARK] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 60 * (current - value))) {
				o_ptr->percent_res[P_RES_DARK] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_CONFU:
		{
			current = (o_ptr->percent_res[P_RES_CONFU] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 75 * (current - value))) {
				o_ptr->percent_res[P_RES_CONFU] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_SOUND:
		{
			current = (o_ptr->percent_res[P_RES_SOUND] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_SOUND] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_SHARD:
		{
			current = (o_ptr->percent_res[P_RES_SHARD] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_SHARD] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_NEXUS:
		{
			current = (o_ptr->percent_res[P_RES_NEXUS] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 50 * (current - value))) {
				o_ptr->percent_res[P_RES_NEXUS] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_NETHR:
		{
			current = (o_ptr->percent_res[P_RES_NETHR] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 80 * (current - value))) {
				o_ptr->percent_res[P_RES_NETHR] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_CHAOS:
		{
			current = (o_ptr->percent_res[P_RES_CHAOS] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 80 * (current - value))) {
				o_ptr->percent_res[P_RES_CHAOS] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case RES_DISEN:
		{
			current = (o_ptr->percent_res[P_RES_DISEN] / 5) - 8;
			if (current <= 0)
				return (FALSE);
			if (!value)
				value = randint0(MIN(current, 8));
			if (take_money(on_credit, 100 * (current - value))) {
				o_ptr->percent_res[P_RES_DISEN] = 40 + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_ACID:
		{
			if (o_ptr->percent_res[P_RES_ACID] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_ACID] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_ELEC:
		{
			if (o_ptr->percent_res[P_RES_ELEC] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_ELEC] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_FIRE:
		{
			if (o_ptr->percent_res[P_RES_FIRE] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_FIRE] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_COLD:
		{
			if (o_ptr->percent_res[P_RES_COLD] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_COLD] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_POIS:
		{
			if (o_ptr->percent_res[P_RES_POIS] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 100 * value)) {
				o_ptr->percent_res[P_RES_POIS] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_LIGHT:
		{
			if (o_ptr->percent_res[P_RES_LIGHT] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 60 * value)) {
				o_ptr->percent_res[P_RES_LIGHT] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_DARK:
		{
			if (o_ptr->percent_res[P_RES_DARK] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 60 * value)) {
				o_ptr->percent_res[P_RES_DARK] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_CONFU:
		{
			if (o_ptr->percent_res[P_RES_CONFU] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 75 * value)) {
				o_ptr->percent_res[P_RES_CONFU] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_SOUND:
		{
			if (o_ptr->percent_res[P_RES_SOUND] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_SOUND] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_SHARD:
		{
			if (o_ptr->percent_res[P_RES_SHARD] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_SHARD] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_NEXUS:
		{
			if (o_ptr->percent_res[P_RES_NEXUS] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 50 * value)) {
				o_ptr->percent_res[P_RES_NEXUS] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_NETHR:
		{
			if (o_ptr->percent_res[P_RES_NETHR] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 80 * value)) {
				o_ptr->percent_res[P_RES_NETHR] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_CHAOS:
		{
			if (o_ptr->percent_res[P_RES_CHAOS] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 80 * value)) {
				o_ptr->percent_res[P_RES_CHAOS] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case VUL_DISEN:
		{
			if (o_ptr->percent_res[P_RES_DISEN] < RES_LEVEL_BASE)
				return (FALSE);
			if (!value)
				value = randint1(6);
			if (take_money(on_credit, 0 - 100 * value)) {
				o_ptr->percent_res[P_RES_DISEN] =
					RES_LEVEL_BASE + 5 * value;
				return (TRUE);
			}
			break;
		}
	case ADD_AC:
		{
			if (take_money(on_credit, value * 40)) {
				o_ptr->to_a += value;
				return (TRUE);
			}
			break;
		}
	case ADD_SKILL:
		{
			if (take_money(on_credit, value * 70)) {
				o_ptr->to_h += value;
				return (TRUE);
			}
			break;
		}
	case ADD_DEADLINESS:
		{
			if (take_money(on_credit, value * 70)) {
				o_ptr->to_d += value;
				return (TRUE);
			}
			break;
		}
	case TELEPORT:
		{
			if (take_money(on_credit, -100)) {
				cf_on(o_ptr->flags_curse, CF_TELEPORT);
				return (TRUE);
			}
			break;
		}
	case NO_TELEPORT:
		{
			if (take_money(on_credit, -500)) {
				cf_on(o_ptr->flags_curse, CF_NO_TELEPORT);
				return (TRUE);
			}
			break;
		}
	case AGGRO_PERM:
		{
			if (take_money(on_credit, -1000)) {
				cf_on(o_ptr->flags_curse, CF_AGGRO_PERM);
				return (TRUE);
			}
			break;
		}
	case AGGRO_RAND:
		{
			if (take_money(on_credit, -400)) {
				cf_on(o_ptr->flags_curse, CF_AGGRO_RAND);
				return (TRUE);
			}
			break;
		}
	case SLOW_REGEN:
		{
			if (take_money(on_credit, -100)) {
				cf_on(o_ptr->flags_curse, CF_SLOW_REGEN);
				return (TRUE);
			}
			break;
		}
	case AFRAID:
		{
			if (take_money(on_credit, -300)) {
				cf_on(o_ptr->flags_curse, CF_AFRAID);
				return (TRUE);
			}
			break;
		}
	case HUNGRY:
		{
			if (take_money(on_credit, -100)) {
				cf_on(o_ptr->flags_curse, CF_HUNGRY);
				return (TRUE);
			}
			break;
		}
	case POIS_RAND:
		{
			if (take_money(on_credit, -100)) {
				cf_on(o_ptr->flags_curse, CF_POIS_RAND);
				return (TRUE);
			}
			break;
		}
	case POIS_RAND_BAD:
		{
			if (take_money(on_credit, -300)) {
				cf_on(o_ptr->flags_curse, CF_POIS_RAND_BAD);
				return (TRUE);
			}
			break;
		}
	case CUT_RAND:
		{
			if (take_money(on_credit, -100)) {
				cf_on(o_ptr->flags_curse, CF_CUT_RAND);
				return (TRUE);
			}
			break;
		}
	case CUT_RAND_BAD:
		{
			if (take_money(on_credit, -200)) {
				cf_on(o_ptr->flags_curse, CF_CUT_RAND_BAD);
				return (TRUE);
			}
			break;
		}
	case HALLU_RAND:
		{
			if (take_money(on_credit, -400)) {
				cf_on(o_ptr->flags_curse, CF_HALLU_RAND);
				return (TRUE);
			}
			break;
		}
	case ATTRACT_DEMON:
		{
			if (take_money(on_credit, -400)) {
				cf_on(o_ptr->flags_curse, CF_ATTRACT_DEMON);
				return (TRUE);
			}
			break;
		}
	case ATTRACT_UNDEAD:
		{
			if (take_money(on_credit, -500)) {
				cf_on(o_ptr->flags_curse, CF_ATTRACT_UNDEAD);
				return (TRUE);
			}
			break;
		}
	case STICKY_CARRY:
		{
			if (take_money(on_credit, -500)) {
				cf_on(o_ptr->flags_curse, CF_STICKY_CARRY);
				return (TRUE);
			}
			break;
		}
	case STICKY_WIELD:
		{
			if (take_money(on_credit, -1300)) {
				cf_on(o_ptr->flags_curse, CF_STICKY_WIELD);
				return (TRUE);
			}
			break;
		}
	case PARALYZE:
		{
			if (take_money(on_credit, -400)) {
				cf_on(o_ptr->flags_curse, CF_PARALYZE);
				return (TRUE);
			}
			break;
		}
	case PARALYZE_ALL:
		{
			if (take_money(on_credit, -1000)) {
				cf_on(o_ptr->flags_curse, CF_PARALYZE_ALL);
				return (TRUE);
			}
			break;
		}
	case DRAIN_EXP:
		{
			if (take_money(on_credit, -400)) {
				cf_on(o_ptr->flags_curse, CF_DRAIN_EXP);
				return (TRUE);
			}
			break;
		}
	case DRAIN_MANA:
		{
			if (take_money(on_credit, -500)) {
				cf_on(o_ptr->flags_curse, CF_DRAIN_MANA);
				return (TRUE);
			}
			break;
		}
	case DRAIN_STAT:
		{
			if (take_money(on_credit, -800)) {
				cf_on(o_ptr->flags_curse, CF_DRAIN_STAT);
				return (TRUE);
			}
			break;
		}
	case DRAIN_CHARGE:
		{
			if (take_money(on_credit, -800)) {
				cf_on(o_ptr->flags_curse, CF_DRAIN_CHARGE);
				return (TRUE);
			}
			break;
		}
	}

	/* No stock with that description on the shelf, or price too high. */
	return (FALSE);
}


/**
 * Grant the activation asked for, if the object can afford it.
 */
static bool get_activation(bool on_credit, int activation,
						   object_type * o_ptr)
{

	/* Allocate the activation, if affordable */
	if (take_money(on_credit, effect_power(activation) * EFFECT_MULT)) {
		o_ptr->effect = activation;
		return (TRUE);
	}

	else
		return (FALSE);
}


/* Ranking methods, etc */
/** Pick the cheapest property available */
#define CHEAPEST_FIRST       1
/** Pick the most expensive property available */
#define DEAREST_FIRST        2
/** Pick a property at random */
#define RANDOM_CHOICE        3
/** Pick the first property from the list that is affordable and new */
#define FIRST_VALID          4
/** Find the maximum value that can be associated with a property */
#define FIND_MAX_VALUE       5
/** More than you can afford... */
#define TOO_MUCH         10000

/**
 * Select a property from a group, subject to affordability, and ranked by
 * various methods.  Properties which cost a negative amount don't need to be
 * considered for ranking methods, since they are always affordable.
 */
static int select_property(int temp_potential, int *property_list,
						   const int choices, int *max_value,
						   int rank_method, object_type * o_ptr)
{
	int i, j, optimal = 0;
	int selection = 0;
	int current_value = 0;

	int prices[choices][*max_value + 1];

	bool found_it = FALSE;
	object_type *i_ptr;
	object_type object_type_body;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Run through choices, record costs */
	for (i = 0; i < choices; i++)
		for (j = 0; j <= *max_value; j++) {

			/* Copy the object, try buying the property */
			object_copy(i_ptr, o_ptr);
			if (get_quality(FALSE, property_list[i], j, i_ptr)) {
				prices[i][j] = temp_potential - potential;
				potential = temp_potential;
			}

			else
				prices[i][j] = TOO_MUCH;

			/* If the copy hasn't changed, the property was already there */
			if (object_similar(o_ptr, i_ptr, OSTACK_NONE))
				prices[i][j] = 0;
		}

	/* Initialise best */
	if (rank_method == CHEAPEST_FIRST)
		optimal = TOO_MUCH;

	/* Run through again, selecting best */
	for (i = 0; i < choices; i++) {
		for (j = 0; j <= *max_value; j++) {
			switch (rank_method) {
			case CHEAPEST_FIRST:
				{
					if ((prices[i][j] > 0) && (prices[i][j] < optimal)) {
						optimal = prices[i][j];
						selection = i;
						current_value = j;
					}
					break;
				}
			case DEAREST_FIRST:
				{
					if ((prices[i][j] < TOO_MUCH)
						&& (prices[i][j] > optimal)) {
						optimal = prices[i][j];
						selection = i;
						current_value = j;
					}
					break;
				}
			case RANDOM_CHOICE:
				{
					int paranoia = 0;
					while ((!optimal) && (paranoia < 20)) {
						selection = randint0(choices);
						current_value = randint0(*max_value + 1);
						if ((prices[selection][current_value] < TOO_MUCH)
							&& (prices[selection][current_value] != 0))
							optimal = prices[selection][current_value];
						paranoia++;
					}
					if (optimal)
						found_it = TRUE;
					break;
				}
			case FIRST_VALID:
				{
					if ((prices[i][j] < TOO_MUCH) && (prices[i][j] > 0)) {
						selection = i;
						current_value = j;
						found_it = TRUE;
					}
					break;
				}
			case FIND_MAX_VALUE:
				{
					if ((prices[i][j] < TOO_MUCH) && (prices[i][j] > 0))
						if (current_value < j) {
							current_value = j;
							selection = i;
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
	case CHEAPEST_FIRST:
		if (optimal < TOO_MUCH)
			found_it = TRUE;
		break;
	case DEAREST_FIRST:
		if (optimal > 0)
			found_it = TRUE;
		break;
	case FIND_MAX_VALUE:
		if (current_value > 0)
			found_it = TRUE;
		break;
	}

	/* Return if found */
	if (found_it)
		return (property_list[selection]);

	else
		return (0);
}


/** 
 * Assign a tval and sval, grant a certain amount of potential to be used 
 * for acquiring powers, and determine rarity and native depth.
 */
static void allocate_potential(object_type * o_ptr, int lev)
{
	int random;

	/* The total potential of a ring or amulet ranges from 0 to 9000, 
	 * depending on depth and type. */
	potential = (j_level * 40) + (lev * 10);

	/* Randomise 10% either way */
	random = randint0(potential / 5);
	potential += random - potential / 10;

	/* Remember how much potential was allowed. */
	initial_potential = potential;
}

/** 
 * Pick an initial set of qualities, based on a theme.  Also add a bonus to 
 * armour class, Skill, and Deadliness.
 */
static bool choose_type(object_type * o_ptr)
{
	int i, j, temp, bonus;
	int e_idx;
	long value, total;
	bool done = FALSE;
	ego_item_type *e_ptr;
	alloc_entry *table = alloc_ego_table;

	/* Reset total */
	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_ego_size; i++) {

		/* Default */
		table[i].prob3 = 0;

		/* Get the index */
		e_idx = table[i].index;

		/* Get the actual kind */
		e_ptr = &e_info[e_idx];

		/* Test if this is a legal type for this object */
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

	/* No legal items (unlikely) */
	if (total == 0)
		return (FALSE);

	/* Get a standard bonus */
	bonus = MAX((potential / 1000), 1);

	/* The main design loop */
	while (!done) {

		/* Pick a type */
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
		if (o_ptr->tval == TV_AMULET) {
			switch (e_idx) {
			case EGO_AMULET_MENTAL:

				/* min potential 350 (100 for just CHR) */
				{
					int paranoia = 20;

					/* Mostly mental properties */
					while (potential >
						   MIN(2000, (2 * initial_potential / 3))) {

						/* 40% deepen wisdom */
						if (randint1(5) < 3) {

							/* Some wisdom, sustained */
							get_quality(TRUE, ADD_WIS, bonus, o_ptr);
							get_quality(TRUE, SUST_WIS, 0, o_ptr);

							/* And maybe some charisma */
							if (randint1(3) == 1)
								get_quality(TRUE, ADD_CHR, randint1(4),
											o_ptr);

							/* Maybe an activation */
							if (randint1(5) == 1)
								get_activation(TRUE, EF_RAND_BLESS, o_ptr);

							else if (randint1(5) == 1)
								get_activation(TRUE, EF_RAND_HEROISM,
											   o_ptr);

							else if (randint1(5) == 1)
								get_activation(TRUE,
											   EF_RAND_PROT_FROM_EVIL,
											   o_ptr);

							/* Sometimes vulnerable to dark and/or cold */
							if (randint1(8) == 1)
								get_quality(FALSE, VUL_COLD, 0, o_ptr);
							if (randint1(8) == 1)
								get_quality(FALSE, VUL_DARK, 0, o_ptr);
						}

						/* 40% sharpen intelligence */
						else if (randint1(3) != 1) {

							/* Some intelligence, sustained */
							get_quality(TRUE, ADD_INT, bonus, o_ptr);
							get_quality(TRUE, SUST_INT, 0, o_ptr);

							/* Maybe an activation */
							if (randint1(6) == 1)
								get_activation(TRUE, EF_RAND_IDENTIFY,
											   o_ptr);

							else if (randint1(6) == 1)
								get_activation(TRUE,
											   EF_RAND_DETECT_MONSTERS,
											   o_ptr);

							else if (randint1(6) == 1)
								get_activation(TRUE, EF_RAND_DETECT_D_S_T,
											   o_ptr);

							/* Sometimes vulnerable to electricity and/or light */
							if (randint1(8) == 1)
								get_quality(FALSE, VUL_ELEC, 0, o_ptr);
							if (randint1(8) == 1)
								get_quality(FALSE, VUL_LIGHT, 0, o_ptr);
						}

						/* Remaining 20% just get charisma */
						else {
							get_quality(TRUE, ADD_CHR, randint1(bonus),
										o_ptr);
							get_quality(TRUE, SUST_CHR, 0, o_ptr);
						}
						if (paranoia-- <= 0)
							break;
					}
					done = TRUE;
					break;
				}
			case EGO_AMULET_DOOM:

				/* min potential 0 */
				{

					/* Chance to recover similar to damage done */
					potential = 0;

					/* Hurt all stats */
					temp = 0 - randint1(bonus);
					get_quality(TRUE, ADD_STR, temp, o_ptr);
					get_quality(TRUE, ADD_WIS, temp, o_ptr);
					get_quality(TRUE, ADD_INT, temp, o_ptr);
					get_quality(TRUE, ADD_DEX, temp, o_ptr);
					get_quality(TRUE, ADD_CON, temp, o_ptr);
					get_quality(TRUE, ADD_CHR, temp, o_ptr);

					/* Can't get it off */
					get_quality(TRUE, STICKY_WIELD, 0, o_ptr);

					/* Maybe more curses */
					while (randint1(10) != 10) {
						int curse = randint1(CURSE_MAX);
						if (curse > CURSE_BASE)
							get_quality(TRUE, curse, 0, o_ptr);

						else
							break;
					}
					done = TRUE;
					break;
				}
			case EGO_AMULET_BASIC_RES:

				/* min potential 300 */
				{

					/* Electricity or acid or both */
					if (randint1(2) == 1) {
						get_quality(TRUE, RES_ELEC, randint0(4) + 3,
									o_ptr);
						if (randint1(3) == 1)
							get_quality(TRUE, RES_ACID, randint0(4) + 3,
										o_ptr);
					}

					else {
						get_quality(TRUE, RES_ACID, randint0(4) + 3,
									o_ptr);
						if (randint1(3) == 1)
							get_quality(TRUE, RES_ELEC, randint0(4) + 3,
										o_ptr);
					}

					/* Small chance of another one */
					if (potential > 1000) {
						if (randint1(10) == 1)
							get_quality(TRUE, RES_COLD, randint0(4) + 5,
										o_ptr);
						if (randint1(10) == 1)
							get_quality(TRUE, RES_FIRE, randint0(4) + 5,
										o_ptr);
					}
					done = TRUE;
					break;
				}
			case EGO_AMULET_MAGIC_MAST:

				/* min potential 105 */
				{

					/* This is an exclusive club */
					if (potential < 1000)
						break;

					/* Bonus to magical item mastery */
					get_quality(TRUE, MAGIC_MASTERY, randint1(bonus),
								o_ptr);

					/* Bonus to DEX */
					if (randint1(4) != 1)
						get_quality(TRUE, ADD_DEX, randint1(bonus), o_ptr);

					/* Protection for the devices */
					if (randint1(5) == 1)
						get_quality(TRUE, RES_ELEC, randint0(4) + 3,
									o_ptr);
					if (randint1(5) == 1)
						get_quality(TRUE, RES_FIRE, randint0(4) + 3,
									o_ptr);
					if (randint1(5) == 1)
						get_quality(TRUE, RES_ACID, randint0(4) + 3,
									o_ptr);
					done = TRUE;
					break;
				}
			case EGO_AMULET_CLARITY:

				/* min potential 800 */
				{

					/* This is an exclusive club */
					if (potential < 2000)
						break;

					/* Freedom of action (weaker objects may get a subset) */
					if (randint1(3500) > initial_potential)
						get_quality(TRUE, SEEING, 0, o_ptr);
					if (randint1(3500) > initial_potential)
						get_quality(TRUE, RES_CONFU, 0, o_ptr);
					if (randint1(3500) > initial_potential)
						get_quality(TRUE, FREE_ACT, 0, o_ptr);
					if ((randint1(3500) > initial_potential) ||
						(potential == initial_potential))
						get_quality(TRUE, SEE_INVIS, 0, o_ptr);

					/* Sometimes the lock */
					if (randint1(4) == 1)
						get_quality(TRUE, RES_SOUND, 0, o_ptr);

					/* Can't be damaged */
					get_quality(TRUE, ELEC_PROOF, 0, o_ptr);

					/* Sometimes vulnerable to nether and/or disenchantment */
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_NETHR, 0, o_ptr);

					else if (randint1(8) == 1)
						get_quality(FALSE, VUL_DISEN, 0, o_ptr);
					done = TRUE;
					break;
				}
			case EGO_AMULET_SHADOWS:

				/* min potential 1200 */
				{

					/* This is an exclusive club */
					if (potential < 1500)
						break;

					/* Creature of the night  */
					get_quality(TRUE, DARKNESS, 0, o_ptr);
					get_quality(TRUE, STEALTH, randint0(bonus), o_ptr);
					get_quality(TRUE, RES_DARK, 6 - randint0(bonus),
								o_ptr);
					get_quality(TRUE, VUL_LIGHT, 5 - randint0(bonus),
								o_ptr);

					/* Maybe some infravision and perception */
					if (randint1(4) == 1)
						get_quality(TRUE, INFRA, 1 + randint0(3), o_ptr);
					if (randint1(4) == 1)
						get_quality(TRUE, SEARCH, 1 + randint0(3), o_ptr);
					done = TRUE;
					break;
				}
			case EGO_AMULET_METAMORPH:

				/* min potential 2000 */
				{

					/* This is an exclusive club */
					if (potential < 3000)
						break;
					o_ptr->effect = EF_AMULET_METAMORPH;
					o_ptr->time.base = 300;
					potential -= 2000;
					done = TRUE;
					break;
				}
			case EGO_AMULET_SUSTENANCE:

				/* min potential  50 */
				{
					int sustains[8] =
						{ SUST_STR, SUST_INT, SUST_WIS, SUST_DEX,
						SUST_CON, SUST_CHR, HOLD_LIFE, SLOW_DIGEST
					};
					int max = 0;
					int property = 1;
					while (property > 0) {
						property =
							select_property(potential, sustains, 8, &max,
											RANDOM_CHOICE, o_ptr);
						get_quality(TRUE, property, 0, o_ptr);
					}
					done = TRUE;
					break;
				}
			case EGO_AMULET_TRICKERY:

				/* min potential 1650 */
				{

					/* This is an exclusive club */
					if (potential < 2000)
						break;

					/* Some properties for everyone */
					get_quality(TRUE, RES_POIS, 0, o_ptr);
					get_quality(TRUE, ADD_DEX, 2 + randint0(bonus), o_ptr);
					get_quality(TRUE, SUST_DEX, 0, o_ptr);
					get_quality(TRUE, STEALTH, 2 + randint0(bonus), o_ptr);

					/* Better amulets get more */
					if (randint1(500) < potential)
						get_quality(TRUE, SEARCH, 2 + randint0(bonus),
									o_ptr);
					if (randint1(1000) < potential)
						get_quality(TRUE, RES_NEXUS, 0, o_ptr);
					if (randint1(1500) < potential)
						get_quality(TRUE, SPEED, randint1(bonus), o_ptr);
					done = TRUE;
					break;
				}
			case EGO_AMULET_WEAPONMAST:

				/* min potential 2650 */
				{

					/* This is an exclusive club */
					if (potential < 3000)
						break;

					/* Safe to get up close */
					get_quality(FALSE, FEARLESS, 0, o_ptr);
					get_quality(FALSE, RES_DISEN, 0, o_ptr);
					get_quality(FALSE, HOLD_LIFE, 0, o_ptr);

					/* Combat bonuses */
					temp = 6 + randint1(4);
					get_quality(TRUE, ADD_DEADLINESS, temp, o_ptr);
					get_quality(TRUE, ADD_SKILL, temp, o_ptr);

					/* Sometimes vulnerable to confusion or chaos */
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_CONFU, 0, o_ptr);

					else if (randint1(8) == 1)
						get_quality(FALSE, VUL_CHAOS, 0, o_ptr);
					done = TRUE;
					break;
				}
			case EGO_AMULET_VITALITY:

				/* min potential 3300 */
				{

					/* This is an exclusive club */
					if (potential < 4500)
						break;

					/* Nice resists */
					get_quality(FALSE, RES_POIS, 0, o_ptr);
					get_quality(FALSE, RES_NETHR, 0, o_ptr);
					get_quality(FALSE, RES_CHAOS, 0, o_ptr);
					get_quality(FALSE, HOLD_LIFE, 0, o_ptr);

					/* And an activation */
					temp = randint1(3);
					if (temp == 1)
						get_activation(TRUE, EF_RAND_REGAIN, o_ptr);

					else if (temp == 2)
						get_activation(TRUE, EF_RAND_RESTORE, o_ptr);

					else
						get_activation(TRUE, EF_RAND_RESIST_ALL, o_ptr);
					done = TRUE;
					break;
				}
			case EGO_AMULET_INSIGHT:

				/* min potential 16 */
				{
					int property = INFRA;
					int max_value = randint1(5);
					int insight[3] = { TELEPATHY, SEE_INVIS, INFRA };

					/* Telepathy, See Invisible, or at least infravision */
					property =
						select_property(potential, insight, 3, &max_value,
										DEAREST_FIRST, o_ptr);
					get_quality(TRUE, property, max_value, o_ptr);
					done = TRUE;
					break;
				}
			}
		}

		else if (o_ptr->tval == TV_RING) {
			switch (e_idx) {
			case EGO_RING_ELEMENTS:

				/* min potential 900 */
				{
					int element = randint0(5);

					/* This is an exclusive club */
					if (potential < 2000)
						break;

					/* Brand, resistance, activation */
					get_quality(TRUE, BRAND_ACID + element, 4, o_ptr);
					get_quality(TRUE, RES_ACID + element, 6 - bonus,
								o_ptr);
					o_ptr->effect = EF_RING_ACID + element;
					o_ptr->time.base = 50;
					o_ptr->time.dice = 1;
					o_ptr->time.sides = 100;
					done = TRUE;
					break;
				}
			case EGO_RING_PHYSICAL:

				/* min potential 350 */
				{
					int property = ADD_STR;
					int max_value = bonus + 1;
					int stats[3] = { ADD_STR, ADD_DEX, ADD_CON };
					int sustains[3] = { SUST_STR, SUST_DEX, SUST_CON };
					int tries = 5;

					/* Stat boost... */
					property =
						select_property(potential, stats, 3, &max_value,
										RANDOM_CHOICE, o_ptr);
					get_quality(TRUE, property, randint1(max_value),
								o_ptr);

					/* And corresponding sustain */
					get_quality(TRUE, property - ADD_STR + SUST_STR, 0,
								o_ptr);

					/* Sometimes get another sustain */
					if (randint1(3) == 1) {
						property =
							select_property(potential, sustains, 3,
											&max_value, RANDOM_CHOICE,
											o_ptr);
						get_quality(TRUE, property, 0, o_ptr);
					}

					/* Repeat while relatively good */
					while ((randint1(initial_potential) >
							initial_potential - potential)
						   && (tries > 0)) {
						max_value = bonus + 1;
						property =
							select_property(potential, stats, 3,
											&max_value, RANDOM_CHOICE,
											o_ptr);
						get_quality(TRUE, property, randint1(max_value),
									o_ptr);
						get_quality(TRUE, property - ADD_STR + SUST_STR,
									0, o_ptr);
						tries--;
					}
					done = TRUE;
					break;
				}
			case EGO_RING_COMBAT:

				/* min potential 580 */
				{

					/* Combat bonuses */
					temp = 1 + bonus;
					get_quality(TRUE, ADD_SKILL, temp + randint1(5),
								o_ptr);
					get_quality(TRUE, ADD_DEADLINESS, temp + randint1(5),
								o_ptr);

					/* Particularly low level items become skill OR deadliness */
					if (randint1(initial_potential) > potential) {
						if (randint1(2) == 1) {
							o_ptr->to_h += o_ptr->to_d;
							o_ptr->to_d = 0;
						}

						else {
							o_ptr->to_d += o_ptr->to_h;
							o_ptr->to_h = 0;
						}
					}

					/* Maybe some stat boosts */
					temp = potential;
					if (randint1(temp) > (initial_potential / 6))
						get_quality(TRUE, ADD_STR, randint1(bonus), o_ptr);
					if (randint1(temp) > (initial_potential / 6))
						get_quality(TRUE, ADD_DEX, randint1(bonus), o_ptr);

					/* And fearlessness */
					get_quality(TRUE, FEARLESS, 0, o_ptr);

					/* Sometimes confusion vulnerability or aggravation */
					if (randint1(5) == 1)
						get_quality(FALSE, VUL_CONFU, 0, o_ptr);

					else if (randint1(5) == 1)
						get_quality(FALSE, AGGRO_PERM, 0, o_ptr);

					else if (randint1(5) == 1)
						get_quality(FALSE, AGGRO_RAND, 0, o_ptr);
					done = TRUE;
					break;
				}
			case EGO_RING_MOBILITY:

				/* min potential 700 */
				{
					int property;
					int max_value = 0;
					int mobility[5] =
						{ SEE_INVIS, FEARLESS, SEEING, RES_CONFU,
						RES_NEXUS
					};

					/* This is an exclusive club */
					if (potential < 500)
						break;

					/* Free action... */
					get_quality(TRUE, FREE_ACT, 0, o_ptr);

					/* ... and more nice properties, if affordable */
					property =
						select_property(potential, mobility, 5, &max_value,
										RANDOM_CHOICE, o_ptr);
					get_quality(FALSE, property, 0, o_ptr);
					done = TRUE;
					break;
				}
			case EGO_RING_ARCANE_RES:

				/* min potential 440 */
				{

					/* This is an exclusive club */
					if (potential < 2000)
						break;

					/* Poison or nether resist */
					if (randint1(2) == 1)
						get_quality(TRUE, RES_POIS, 0, o_ptr);

					else
						get_quality(TRUE, RES_NETHR, 0, o_ptr);

					/* Rack 'em up while we're feeling lucky */
					while (randint1(potential) > 1000) {
						int k = randint0(10);
						if (o_ptr->percent_res[P_RES_POIS + k] !=
							RES_LEVEL_BASE)
							get_quality(TRUE, RES_POIS + k, 0, o_ptr);
					}
					done = TRUE;
					break;
				}
			case EGO_RING_UTILITY:

				/* min potential 21 */
				{
					int property;
					int max_value = 5;
					int utility[5] =
						{ SEARCH, SLOW_DIGEST, FEATHER, LIGHT, REGEN };
					int tries = randint1(bonus + 2);

					/* Pick a few properties */
					if (tries > 6)
						tries = 6;
					for (j = 0; j < tries; j++) {
						property =
							select_property(potential, utility, 5,
											&max_value, RANDOM_CHOICE,
											o_ptr);
						get_quality(TRUE, property, max_value, o_ptr);
					}

					/* And maybe a useful activation, if some potential left  */
					if (potential > 300) {
						temp = randint1(6);
						if (temp == 1)
							get_activation(TRUE, EF_RAND_IDENTIFY, o_ptr);
						if (temp == 2)
							get_activation(TRUE, EF_RAND_DISARM, o_ptr);
						if (temp == 3)
							get_activation(TRUE, EF_RAND_RECALL, o_ptr);
					}
					done = TRUE;
					break;
				}
			case EGO_RING_BASIC_RES:

				/* min potential  */
				{

					/* Fire or cold or both */
					if (randint1(2) == 1) {
						get_quality(TRUE, RES_FIRE, randint0(4) + 3,
									o_ptr);
						if (randint1(3) == 1)
							get_quality(TRUE, RES_COLD, randint0(4) + 3,
										o_ptr);
					}

					else {
						get_quality(TRUE, RES_COLD, randint0(4) + 3,
									o_ptr);
						if (randint1(3) == 1)
							get_quality(TRUE, RES_FIRE, randint0(4) + 3,
										o_ptr);
					}

					/* Small chance of another one */
					if (potential > 1000) {
						if (randint1(10) == 1)
							get_quality(TRUE, RES_ACID, randint0(4) + 5,
										o_ptr);
						if (randint1(10) == 1)
							get_quality(TRUE, RES_ELEC, randint0(4) + 5,
										o_ptr);
					}
					done = TRUE;
					break;
				}
			case EGO_RING_HINDRANCE:

				/* min potential 0 */
				{
					int tries = randint1(3);
					int max_value = 5;
					int property;
					int stats[6] =
						{ ADD_STR, ADD_INT, ADD_WIS, ADD_DEX, ADD_CON,
						ADD_CHR
					};

					/* Sticky */
					get_quality(TRUE, STICKY_WIELD, 0, o_ptr);

					/* Cripple some stats */
					for (j = 0; j < tries; j++) {
						property =
							select_property(potential, stats, 6,
											&max_value, RANDOM_CHOICE,
											o_ptr);
						get_quality(TRUE, property, -randint1(5), o_ptr);
					}

					/* Potential gained counts as lost */
					if (potential - initial_potential > initial_potential)
						potential = 0;

					else
						potential = (2 * initial_potential) - potential;
					done = TRUE;
					break;
				}
			case EGO_RING_DAWN:

				/* min potential 1300 */
				{

					/* This is an exclusive club */
					if (potential < 2000)
						break;

					/* Power of seeing */
					get_quality(TRUE, SEEING, 0, o_ptr);
					get_quality(TRUE, RES_LIGHT, 0, o_ptr);
					get_quality(TRUE, RES_DARK, 0, o_ptr);

					/* Maybe see invisible, if affordable */
					if (randint1(3) == 1)
						get_quality(FALSE, SEE_INVIS, 0, o_ptr);
					done = TRUE;
					break;
				}
			case EGO_RING_SPEED:

				/* min potential  */
				{
					int speed[1] = { SPEED };
					int max_value = 14;

					/* This is an exclusive club */
					if (potential < 3000)
						break;

					/* SPEED! */
					(void) select_property(potential, speed, 1,
										   &max_value, FIND_MAX_VALUE,
										   o_ptr);

					/* Either sacrifice for some other properties... */
					if (randint1(2) == 1) {
						get_quality(TRUE, SPEED, 1 + max_value / 2, o_ptr);

						/* Maybe get combat bonuses... */
						if (randint1(4) == 1) {
							temp = 2 + randint1(bonus);
							get_quality(TRUE, ADD_DEADLINESS, temp, o_ptr);
							get_quality(TRUE, ADD_SKILL, temp, o_ptr);
						}

						/* ...or a high resist... */
						else if (randint1(3) == 1) {
							get_quality(TRUE,
										rand_range(RES_POIS, RES_DISEN),
										0, o_ptr);
						}

						/* ...or nice power */
						else if (randint1(2) == 1) {
							get_quality(TRUE, rand_range(REGEN, DARKNESS),
										0, o_ptr);
						}
					}

					/* ...or go all out */
					else {
						while (randint1(2) == 1)
							max_value++;
						get_quality(TRUE, SPEED, max_value, o_ptr);
					}
					done = TRUE;
					break;
				}
			case EGO_RING_WOE:

				/* min potential 0 */
				{

					/* Don't find these too early */
					if (potential < 1500)
						break;

					/* Curses */
					get_quality(TRUE, TELEPORT, 0, o_ptr);
					get_quality(TRUE, STICKY_WIELD, 0, o_ptr);

					/* Hit stats */
					get_quality(TRUE, ADD_WIS, 0 - bonus, o_ptr);
					get_quality(TRUE, ADD_CHR, 0 - bonus, o_ptr);

					/* Some advantages */
					get_quality(TRUE, RES_NEXUS, 0, o_ptr);
					get_quality(TRUE, FREE_ACT, 0, o_ptr);

					/* Reduce chance to recover */
					potential /= 2;
					done = TRUE;
					break;
				}
			case EGO_RING_FICKLENESS:

				/* min potential 0 */
				{
					int curse;
					int max_value = 0;
					int fickle[7] =
						{ TELEPORT, NO_TELEPORT, CUT_RAND, HALLU_RAND,
						AGGRO_RAND, ATTRACT_DEMON, PARALYZE
					};
					while (1) {
						curse =
							select_property(potential, fickle, 7,
											&max_value, RANDOM_CHOICE,
											o_ptr);
						get_quality(TRUE, curse, 0, o_ptr);
						if (randint1(3) == 1)
							break;
					}
					done = TRUE;
					break;
				}
			case EGO_RING_POWER:

				/* min potential  */
				{
					int element = randint0(4);

					/* This is an exclusive club */
					if (potential < 4000)
						break;

					/* Free bonuses to Deadliness, Skill and Armour Class */
					temp = 3 + randint1(5) + potential / 2000;
					o_ptr->to_d += temp;
					o_ptr->to_h += temp;
					o_ptr->to_a += 3 + randint1(5) + potential / 1000;

					/* Power over an element */
					get_quality(FALSE, IM_ACID + element, 0, o_ptr);

					/* Sometimes vulnerable to high elements... */
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_NEXUS, 0, o_ptr);
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_NETHR, 0, o_ptr);
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_CHAOS, 0, o_ptr);
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_DISEN, 0, o_ptr);

					/* ...but likely to sustain stats */
					if (randint1(3) == 1)
						get_quality(FALSE, SUST_STR, 0, o_ptr);
					if (randint1(3) == 1)
						get_quality(FALSE, SUST_INT, 0, o_ptr);
					if (randint1(3) == 1)
						get_quality(FALSE, SUST_WIS, 0, o_ptr);
					if (randint1(3) == 1)
						get_quality(FALSE, SUST_DEX, 0, o_ptr);
					if (randint1(3) == 1)
						get_quality(FALSE, SUST_CON, 0, o_ptr);
					if (randint1(3) == 1)
						get_quality(FALSE, SUST_CHR, 0, o_ptr);

					/* And with a powerful activation at a bargain price... */
					temp = randint0(EF_POWER_MAX - EF_POWER_BASE + 1);

					/* Hack - summon friendlies needs more work */
					while (temp == EF_ENLIST_EARTH - EF_POWER_BASE)
						temp = randint0(EF_POWER_MAX - EF_POWER_BASE + 1);
					if (temp < 4) {
						get_activation(TRUE, EF_POWER_BASE + element,
									   o_ptr);
						potential +=
							effect_power(EF_POWER_BASE + element) / 2;
					}

					else {
						get_activation(TRUE, EF_POWER_BASE + temp, o_ptr);
						potential +=
							effect_power(EF_POWER_BASE + temp) / 2;
					}

					/* ...plus help to activate it */
					get_quality(TRUE, MAGIC_MASTERY, bonus, o_ptr);
					done = TRUE;
					break;
				}
			}
		}

		else
			break;
	}
	return (done);
}


/** 
 * Grant extra abilities, until object's units of exchange are all used up.  
 * This function can be quite random - indeed needs to be - because of all 
 * the possible themed random artifacts.
 */
static void add_properties(object_type * o_ptr)
{
	int i, rounds;
	int pval = 0;
	int choice = 0;
	int limit = 20;

	/* Rarely, and only if the object has a lot of potential, 
	 * add all the stats. */
	if ((randint1(6) == 1) && (potential >= 3000)) {
		get_quality(TRUE, ADD_STR, 1, o_ptr);
		get_quality(TRUE, ADD_WIS, 1, o_ptr);
		get_quality(TRUE, ADD_INT, 1, o_ptr);
		get_quality(TRUE, ADD_DEX, 1, o_ptr);
		get_quality(TRUE, ADD_CON, 1, o_ptr);
		get_quality(TRUE, ADD_CHR, 1, o_ptr);
	}

	/* Otherwise, if at least some potential remains, add a 
	 * pval-dependant quality or two with 67% probability. */
	else if ((randint1(3) != 1) && (potential >= 750)) {

		/* Add the pval. */
		pval = potential / 2000 + randint1(2);

		/* Determine number of loops. */
		rounds = randint1(2);
		for (i = 0; i < rounds; i++) {
			choice = randint1(10);
			if (choice == 1)
				get_quality(TRUE, ADD_STR, pval, o_ptr);
			if (choice == 2)
				get_quality(TRUE, ADD_WIS, pval, o_ptr);
			if (choice == 3)
				get_quality(TRUE, ADD_INT, pval, o_ptr);
			if (choice == 4)
				get_quality(TRUE, ADD_DEX, pval, o_ptr);
			if (choice == 5)
				get_quality(TRUE, ADD_CON, pval, o_ptr);
			if (choice == 6)
				get_quality(TRUE, ADD_CHR, pval, o_ptr);
			if (choice == 7)
				get_quality(TRUE, INFRA, pval, o_ptr);
			if (choice == 8)
				get_quality(TRUE, STEALTH, pval, o_ptr);
			if (choice == 9)
				get_quality(TRUE, SEARCH, pval, o_ptr);
			if (choice == 10)
				get_quality(TRUE, SPEED, pval, o_ptr);
			choice = 0;
		}
	}

	/* Sometimes, collect a vulnerability or two in exchange for more potential */
	if (randint1(8) == 1) {

		/* Determine number of loops. */
		rounds = randint1(2);
		for (i = 0; i < rounds; i++) {
			choice = randint0(VUL_MAX - VUL_BASE + 1);
			get_quality(TRUE, VUL_BASE + choice, 0, o_ptr);
		}
	}

	/* Sometimes, powerful items collect curses in exchange for more potential */
	if ((randint1(5000) < potential) && (potential >= 1000)) {

		/* Determine number of loops. */
		rounds = randint1(potential / 1000);
		for (i = 0; i < rounds; i++) {
			choice = randint0(CURSE_MAX - CURSE_BASE + 1);
			get_quality(TRUE, CURSE_BASE + choice, 0, o_ptr);
		}
	}

	/* Objects that still have lots of money to spend deserve the best. */
	if ((potential > 1500) && (randint1(3) != 1)) {

		/* Make a choice... */
		choice = randint1(5);

		/* ...among some tasty options. */
		if ((choice == 1)
			&& (!(of_has(o_ptr->flags_obj, OF_TELEPATHY)))) {
			get_quality(TRUE, TELEPATHY, 0, o_ptr);
		}
		if ((choice == 2) && (!(of_has(o_ptr->flags_obj, OF_HOLD_LIFE)))) {
			get_quality(TRUE, HOLD_LIFE, 0, o_ptr);
		}
		if (choice == 3) {
			if (o_ptr->percent_res[P_RES_CONFU] >= RES_LEVEL_BASE)
				get_quality(TRUE, RES_CONFU, 0, o_ptr);
			if (!(of_has(o_ptr->flags_obj, OF_SEEING)))
				get_quality(TRUE, SEEING, 0, o_ptr);
		}
		if (choice == 4) {
			if ((randint1(4) == 1) &&
				(o_ptr->percent_res[P_RES_DISEN] >= RES_LEVEL_BASE))
				get_quality(TRUE, RES_DISEN, 0, o_ptr);

			else if ((randint1(3) == 1) &&
					 (o_ptr->percent_res[P_RES_NETHR] >= RES_LEVEL_BASE))
				get_quality(TRUE, RES_NETHR, 0, o_ptr);

			else if ((randint1(2) == 1) &&
					 (o_ptr->percent_res[P_RES_CHAOS] >= RES_LEVEL_BASE))
				get_quality(TRUE, RES_CHAOS, 0, o_ptr);

			else if (o_ptr->percent_res[P_RES_POIS] >= RES_LEVEL_BASE)
				get_quality(TRUE, RES_POIS, 0, o_ptr);
		}
		if ((choice == 5) && (potential > 2500)) {
			if ((randint1(2) == 1)
				&& (o_ptr->bonus_other[P_BONUS_SPEED] == 0)) {
				get_quality(TRUE, SPEED, randint1(10), o_ptr);
			}

			else {
				get_quality(TRUE, SUST_STR, pval, o_ptr);
				get_quality(TRUE, SUST_WIS, pval, o_ptr);
				get_quality(TRUE, SUST_INT, pval, o_ptr);
				get_quality(TRUE, SUST_DEX, pval, o_ptr);
				get_quality(TRUE, SUST_CON, pval, o_ptr);
				get_quality(TRUE, SUST_CHR, pval, o_ptr);
			}
		}
	}

	/* Objects with combat bonuses may increase them now */
	if ((o_ptr->to_h > 0) && (randint1(3) == 1))
		get_quality(TRUE, ADD_SKILL, randint1(o_ptr->to_h), o_ptr);
	if ((o_ptr->to_d > 0) && (randint1(3) == 1))
		get_quality(TRUE, ADD_DEADLINESS, randint1(o_ptr->to_d), o_ptr);
	if ((o_ptr->to_a > 0) && (randint1(3) == 1))
		get_quality(TRUE, ADD_AC, randint1(o_ptr->to_a), o_ptr);

	/* Objects without an activation may get one now, if affordable */
	if ((randint1(3) == 1) && !o_ptr->effect)
		get_activation(TRUE,
					   randint0(EF_RAND_MAX - EF_RAND_BASE + 1) +
					   EF_RAND_BASE, o_ptr);

	/* Sometimes, also add a new pval-dependant quality.  Infravision 
	 * and magical item mastery are not on offer.  
	 */
	if (((randint1(5) < 3) || (potential >= 2000))
		&& ((!pval) || (pval < 7))) {
		pval = randint1(3);
		choice = randint1(9);
		if (choice == 1)
			get_quality(TRUE, ADD_STR, pval, o_ptr);
		if (choice == 2)
			get_quality(TRUE, ADD_WIS, pval, o_ptr);
		if (choice == 3)
			get_quality(TRUE, ADD_INT, pval, o_ptr);
		if (choice == 4)
			get_quality(TRUE, ADD_DEX, pval, o_ptr);
		if (choice == 5)
			get_quality(TRUE, ADD_CON, pval, o_ptr);
		if (choice == 6)
			get_quality(TRUE, ADD_CHR, pval, o_ptr);
		if (choice == 7)
			get_quality(TRUE, STEALTH, pval, o_ptr);
		if (choice == 8)
			get_quality(TRUE, SEARCH, pval, o_ptr);
		if (choice == 9)
			get_quality(TRUE, SPEED, pval, o_ptr);
		choice = 0;
	}

	/* Now, we enter Filene's Basement, and shop 'til we drop! (well, nearly) */
	while ((potential >= 300) && (limit-- > 0)) {

		/* Collect a resistance. */
		choice = randint1(11);
		if ((choice == 1) &&
			(o_ptr->percent_res[P_RES_FIRE] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_FIRE, 0, o_ptr);
		if ((choice == 2) &&
			(o_ptr->percent_res[P_RES_COLD] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_COLD, 0, o_ptr);
		if ((choice == 3) &&
			(o_ptr->percent_res[P_RES_ACID] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_ACID, 0, o_ptr);
		if ((choice == 4) &&
			(o_ptr->percent_res[P_RES_ELEC] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_ELEC, 0, o_ptr);
		if ((choice == 5) &&
			(o_ptr->percent_res[P_RES_LIGHT] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_LIGHT, 0, o_ptr);
		if ((choice == 6) &&
			(o_ptr->percent_res[P_RES_DARK] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_DARK, 0, o_ptr);
		if ((choice == 7) &&
			(o_ptr->percent_res[P_RES_CONFU] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_CONFU, 0, o_ptr);
		if ((choice == 8) &&
			(o_ptr->percent_res[P_RES_SOUND] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_SOUND, 0, o_ptr);
		if ((choice == 9) &&
			(o_ptr->percent_res[P_RES_SHARD] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_SHARD, 0, o_ptr);
		if ((choice == 10) &&
			(o_ptr->percent_res[P_RES_NEXUS] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_NEXUS, 0, o_ptr);
		if ((choice == 11) &&
			(o_ptr->percent_res[P_RES_DISEN] == RES_LEVEL_BASE))
			get_quality(TRUE, RES_DISEN, 0, o_ptr);

		/* Sometimes, collect a miscellaneous quality, if it is affordable. */
		if (randint1(3) == 1) {
			choice = randint1(8);

			/* Collect a miscellanious quality. */
			if ((choice == 1)
				&& (!(of_has(o_ptr->flags_obj, OF_SLOW_DIGEST))))
				get_quality(FALSE, SLOW_DIGEST, 0, o_ptr);
			if ((choice == 2)
				&& (!(of_has(o_ptr->flags_obj, OF_FEATHER))))
				get_quality(FALSE, FEATHER, 0, o_ptr);
			if ((choice == 3) && (!(of_has(o_ptr->flags_obj, OF_LIGHT))))
				get_quality(FALSE, LIGHT, 1, o_ptr);
			if ((choice == 4) && (!(of_has(o_ptr->flags_obj, OF_REGEN))))
				get_quality(FALSE, REGEN, 0, o_ptr);
			if ((choice == 5)
				&& (!(of_has(o_ptr->flags_obj, OF_SEE_INVIS))))
				get_quality(FALSE, SEE_INVIS, 0, o_ptr);
			if ((choice == 6)
				&& (!(of_has(o_ptr->flags_obj, OF_FREE_ACT))))
				get_quality(FALSE, FREE_ACT, 0, o_ptr);
			if ((choice == 7)
				&& (!(of_has(o_ptr->flags_obj, OF_FEARLESS))))
				get_quality(FALSE, FEARLESS, 0, o_ptr);
			if ((choice == 8) && (!(of_has(o_ptr->flags_obj, OF_SEEING))))
				get_quality(FALSE, SEEING, 0, o_ptr);
		}

		/* Clean out the wallet. */
		if ((potential < 500) && (randint1(5) == 1)) {
			if (o_ptr->to_a > 0)
				o_ptr->to_a += potential / 100;
			potential = 0;
		}
	}

	/* On sale free!  Last chance! */

	/* If an object affects a stat, it may also possibly sustain it. */
	choice = randint1(6);
	if ((choice == 1) && (o_ptr->bonus_stat[A_STR] > 0))
		of_on(o_ptr->flags_obj, OF_SUSTAIN_STR);

	else if ((choice == 2) && (o_ptr->bonus_stat[A_WIS] > 0))
		of_on(o_ptr->flags_obj, OF_SUSTAIN_WIS);

	else if ((choice == 3) && (o_ptr->bonus_stat[A_INT] > 0))
		of_on(o_ptr->flags_obj, OF_SUSTAIN_INT);

	else if ((choice == 4) && (o_ptr->bonus_stat[A_DEX] > 0))
		of_on(o_ptr->flags_obj, OF_SUSTAIN_DEX);

	else if ((choice == 5) && (o_ptr->bonus_stat[A_CON] > 0))
		of_on(o_ptr->flags_obj, OF_SUSTAIN_CON);

	else if ((choice == 6) && (o_ptr->bonus_stat[A_CHR] > 0))
		of_on(o_ptr->flags_obj, OF_SUSTAIN_CHR);

	/* Frequently neaten bonuses to Armour Class, Skill, and Deadliness. */
	{
		if ((o_ptr->to_a % 5 == 4) && (randint1(2) == 1))
			o_ptr->to_a++;

		else if ((o_ptr->to_a % 5 == 1) && (randint1(2) == 1))
			o_ptr->to_a--;

		else if ((o_ptr->to_a % 2 == 1) && (randint1(4) != 1))
			o_ptr->to_a++;
		if ((o_ptr->to_h % 5 == 4) && (randint1(2) == 1))
			o_ptr->to_h++;

		else if ((o_ptr->to_h % 5 == 1) && (randint1(2) == 1))
			o_ptr->to_h--;

		else if ((o_ptr->to_h % 2 == 1) && (randint1(4) != 1))
			o_ptr->to_h++;
		if ((o_ptr->to_d % 5 == 4) && (randint1(2) == 1))
			o_ptr->to_d++;

		else if ((o_ptr->to_d % 5 == 1) && (randint1(2) == 1))
			o_ptr->to_d--;

		else if ((o_ptr->to_d % 2 == 1) && (randint1(4) != 1))
			o_ptr->to_d++;
	}
}


/** 
 * Invoke perilous magics, and curse the object beyound redemption!   
 * Not really in Leon's league, though.
 */
static void j_make_terrible(object_type * o_ptr)
{
	int i, j, gauntlet_runs, wheel_of_doom, penalty;
	int num_curses = 1;

	/* Determine whether the artifact's magics are perilous enough to warrant 
	 * extra curses.
	 */
	if (potential >= 3000)
		num_curses++;
	if ((potential >= 3500) && (randint1(3) != 1))
		num_curses++;

	/* Greatly decrease the chance for an activation. */
	if ((o_ptr->effect) && (randint1(3) != 1))
		o_ptr->effect = 0;

	/* Force the artifact though the gauntlet two or three times. */
	gauntlet_runs = 1 + randint1(2);
	for (i = 0; i < gauntlet_runs; i++) {

		/* Choose a curse, biased towards penalties to_a, to_d, and to_h. */
		if ((o_ptr->to_h > 0) && (randint1(2) == 1))
			wheel_of_doom = 2;

		else if ((o_ptr->to_a > 0) && (randint1(2) == 1))
			wheel_of_doom = 1;

		else
			wheel_of_doom = 2 + randint1(2);
		penalty = 0;

		/* Blast base armour class or inflict a penalty to armour class. */
		if (wheel_of_doom == 1) {

			/* Invert armour class. */
			if ((o_ptr->ac) && (randint1(6) == 1)) {
				o_ptr->ac = 0;
			}

			/* Chance of a truly nasty effect. */
			else if (randint1(4) == 1) {
				penalty = randint1(3) + 2;
				penalty *= 5;
				o_ptr->to_a = -penalty;
			}
		}

		/* Make either the Skill or Deadliness bonus negative, or both. */
		if (wheel_of_doom == 2) {

			/* One type of badness... */
			if (randint1(2) == 1) {

				/* Blast items with bonuses, Deadliness more rarely. */
				o_ptr->to_h = -(5 + randint1(7)) * 2;
				if (randint1(3) != 1)
					o_ptr->to_d = -(5 + randint1(7)) * 2;
			}

			/* ...or another. */
			else {

				/* Reverse any magics. */
				if (o_ptr->to_h > 0)
					o_ptr->to_h = -o_ptr->to_h;
				if (o_ptr->to_d > 0)
					o_ptr->to_d = -o_ptr->to_d;

				/* Sometimes, blast even items without bonuses. */
				else if ((o_ptr->to_d == 0) && (o_ptr->to_h == 0) &&
						 (randint1(5) == 1)) {
					penalty = randint1(4) + 1;
					penalty *= 5;
					o_ptr->to_h -= penalty;
					o_ptr->to_d -= penalty;
				}
			}
		}

		/* Make any bonuses negative, or strip all bonuses, 
		 * and add a random stat or three with large negative bonuses. */
		if (wheel_of_doom == 3) {
			if (randint1(3) != 1) {
				for (j = 0; j < A_MAX; j++) {
					penalty = -(o_ptr->bonus_stat[j]);
					o_ptr->bonus_stat[j] = penalty;
					penalty = -(o_ptr->bonus_other[j]);
					o_ptr->bonus_other[j] = penalty;
				}
			}

			else {

				/* This is bad... */
				penalty = -(randint1(5)) - 3;
				if (randint1(3) == 1)
					penalty *= 5;
				for (j = 0; j < A_MAX; j++) {
					o_ptr->bonus_stat[j] = 0;
					o_ptr->bonus_other[j] = 0;
				}
				if (randint1(5) == 1) {
					o_ptr->bonus_stat[A_STR] = penalty;
					o_ptr->bonus_stat[A_DEX] = penalty;
					o_ptr->bonus_stat[A_CON] = penalty;
				}

				else if (randint1(4) == 1) {
					o_ptr->bonus_stat[A_WIS] = penalty;
					o_ptr->bonus_stat[A_INT] = penalty;
				}

				else if (randint1(6) == 1) {
					o_ptr->bonus_stat[P_BONUS_SPEED] = penalty;
				}

				else {
					for (j = 0; j < A_MAX; j++)
						if (randint1(4) == 1)
							o_ptr->bonus_stat[j] = penalty;
				}

				/* More curses for good measure... */
				num_curses += randint1(2);

				/* ...and hard to get rid of */
				cf_on(o_ptr->flags_curse, CF_STICKY_CARRY);
			}
		}

		/* Strip lots of other qualities. */
		if (wheel_of_doom == 4) {
			if (randint1(4) == 1)
				o_ptr->bonus_other[P_BONUS_SHOTS] = 0;
			if (randint1(4) == 1)
				o_ptr->bonus_other[P_BONUS_MIGHT] = 0;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_ANIMAL] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_EVIL] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_UNDEAD] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_DEMON] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_ORC] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_TROLL] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_GIANT] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_slay[P_SLAY_DRAGON] = 10;
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_PERFECT_BALANCE);
			if (randint1(3) == 1)
				o_ptr->multiple_brand[P_BRAND_POIS] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_brand[P_BRAND_ACID] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_brand[P_BRAND_ELEC] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_brand[P_BRAND_FIRE] = 10;
			if (randint1(3) == 1)
				o_ptr->multiple_brand[P_BRAND_COLD] = 10;
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_ACID] +=
					(2 * (100 - o_ptr->percent_res[P_RES_ACID]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_ELEC] +=
					(2 * (100 - o_ptr->percent_res[P_RES_ELEC]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_FIRE] +=
					(2 * (100 - o_ptr->percent_res[P_RES_FIRE]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_COLD] +=
					(2 * (100 - o_ptr->percent_res[P_RES_COLD]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_POIS] +=
					(2 * (100 - o_ptr->percent_res[P_RES_POIS]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_LIGHT] +=
					(2 * (100 - o_ptr->percent_res[P_RES_LIGHT]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_DARK] +=
					(2 * (100 - o_ptr->percent_res[P_RES_DARK]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_CONFU] +=
					(2 * (100 - o_ptr->percent_res[P_RES_CONFU]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_SOUND] +=
					(2 * (100 - o_ptr->percent_res[P_RES_SOUND]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_SHARD] +=
					(2 * (100 - o_ptr->percent_res[P_RES_SHARD]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_NEXUS] +=
					(2 * (100 - o_ptr->percent_res[P_RES_NEXUS]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_NETHR] +=
					(2 * (100 - o_ptr->percent_res[P_RES_NETHR]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_CHAOS] +=
					(2 * (100 - o_ptr->percent_res[P_RES_CHAOS]));
			if (randint1(3) == 1)
				o_ptr->percent_res[P_RES_DISEN] +=
					(2 * (100 - o_ptr->percent_res[P_RES_DISEN]));
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_SLOW_DIGEST);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_FEATHER);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_LIGHT);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_REGEN);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_TELEPATHY);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_SEE_INVIS);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_FREE_ACT);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_HOLD_LIFE);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_FEARLESS);
			if (randint1(3) == 1)
				of_off(o_ptr->flags_obj, OF_SEEING);
		}
	}

	/* Apply curses. */
	for (i = 0; i < num_curses; i++)
		cf_on(o_ptr->flags_curse, FLAG_START + randint0(CF_MAX));
}


/** 
 * Clean up the object by removing illogical combinations of powers -  
 * curses win out every time.
 */
static void j_remove_contradictory(object_type * o_ptr)
{
	int i;
	if (cf_has(o_ptr->flags_curse, CF_AGGRO_PERM))
		o_ptr->bonus_other[P_BONUS_STEALTH] = 0;
	if (cf_has(o_ptr->flags_curse, CF_DRAIN_EXP))
		of_off(o_ptr->flags_obj, OF_HOLD_LIFE);
	if (cf_has(o_ptr->flags_curse, CF_SLOW_REGEN))
		of_off(o_ptr->flags_obj, OF_REGEN);
	if (cf_has(o_ptr->flags_curse, CF_AFRAID))
		of_off(o_ptr->flags_obj, OF_FEARLESS);
	if (cf_has(o_ptr->flags_curse, CF_HUNGRY))
		of_off(o_ptr->flags_obj, OF_SLOW_DIGEST);
	if (cf_has(o_ptr->flags_curse, CF_PARALYZE))
		of_off(o_ptr->flags_obj, OF_FREE_ACT);
	for (i = 0; i < A_MAX; i++) {
		if (o_ptr->bonus_stat[i] < 0)
			of_off(o_ptr->flags_obj, OF_SUSTAIN_STR + i);
		if (o_ptr->bonus_stat[i] > 6)
			o_ptr->bonus_stat[i] = 6;
	}
	for (i = 0; i < MAX_P_BONUS; i++) {
		if ((o_ptr->bonus_other[i] > 6) && (i != P_BONUS_SPEED))
			o_ptr->bonus_other[i] = 6;
	}
	if ((cf_has(o_ptr->flags_curse, CF_NO_TELEPORT)) &&
		((o_ptr->effect = EF_RAND_TELEPORT1) ||
		 (o_ptr->effect = EF_RAND_TELEPORT2)))
		o_ptr->effect = 0;

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
 * Design a ring or amulet.
 */
bool design_ring_or_amulet(object_type * o_ptr, int lev)
{

	/* Assign it a potential. */
	allocate_potential(o_ptr, lev);

	/* Assign a type. */
	if (!choose_type(o_ptr))
		return FALSE;

	/* Add extra qualities until done. */
	add_properties(o_ptr);

	/* Decide if the object is to be terrible. */
	if (TERRIBLE_CHANCE > randint0(100)) {

		/* If it is, add a few benefits and more drawbacks. */
		j_make_terrible(o_ptr);
	}

	/* Remove contradictory powers. */
	j_remove_contradictory(o_ptr);

	/* Add effect timeout */
	effect_time(o_ptr->effect, &o_ptr->time);
	return TRUE;
}
