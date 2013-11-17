/** \file randart.c 
    \brief Random artifact generation

 * Random artifacts.  Selling and providing qualities.  Choosing a object 
 * type and kind, determining the potential, depth and rarity of the 
 * artifact.  Selecting a theme, and the properties of all possible themes.  
 * Adding semi-random qualities until potential is all used up.  Cursing 
 * an artifact.  Removing contradictory flags.  Naming an artifact, using 
 * either of two methods.  Initializing one random artifact.  Adding new 
 * names to the a_name array.  Initializing all random artifacts.
 *
 * Copyright (c) 1998 Leon Marrick
 *
 * Thanks to Greg Wooledge for his support and string-handling code 
 * and to W. Sheldon Simms for his Tolkienesque random name generator.
 *
 * Copyright (c) 2009 Nick McConnell, Si Griffin
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
#include "files.h"

/** A global variable whose contents will be bartered to acquire powers. */
static int potential = 0;

/** The initial potential of the artifact. */
static int initial_potential = 0;

/** The maximum potential that that artifact could have possessed. */
static int max_potential = 0;

/** Global variable indicating that the random naming routine is unavailable. */
static bool find_all_names = FALSE;

/** Percentage chance for an artifact to be terrible. */
#define TERRIBLE_CHANCE		10

/* Globals used by the artifact naming code. */
#define BUFLEN 1024

#define NAMES_FILE "names.txt"
#define MIN_NAME_LEN 5
#define MAX_NAME_LEN 9
#define S_WORD 26
#define E_WORD S_WORD
long lprobs[S_WORD + 1][S_WORD + 1][S_WORD + 1];	/* global, hence init
													 * to 0 */
long ltotal[S_WORD + 1][S_WORD + 1];	/* global, hence init to 0 */

/* Definitions of most artifact flags. */

#define ADD_STR			1
#define ADD_INT			2
#define ADD_WIS			3
#define ADD_DEX			4
#define ADD_CON			5
#define ADD_CHR			6

#define MAGIC_MASTERY	9
#define STEALTH			10
#define SEARCH			11
#define INFRA			12
#define TUNNEL			13
#define SPEED			14
#define MIGHT			15
#define SHOTS			16
#define SLAY_ANIMAL		20
#define SLAY_EVIL		21
#define SLAY_UNDEAD		22
#define SLAY_DEMON		23
#define SLAY_ORC		24
#define SLAY_TROLL		25
#define SLAY_GIANT		26
#define SLAY_DRAGON		27
#define PERFECT_BALANCE	28
#define BRAND_POIS		29
#define BRAND_ACID		30
#define BRAND_ELEC		31
#define BRAND_FIRE		32
#define BRAND_COLD		33
/* SJGU */
#define SLAY_KILL		34

#define SUST_STR		40
#define SUST_INT		41
#define SUST_WIS		42
#define SUST_DEX		43
#define SUST_CON		44
#define SUST_CHR		45
#define IM_ACID			48
#define IM_ELEC			49
#define IM_FIRE			50
#define IM_COLD			51
#define RES_ACID		52
#define RES_ELEC		53
#define RES_FIRE		54
#define RES_COLD		55
#define RES_POIS		56
#define RES_FEAR		57
#define RES_LIGHT		58
#define RES_DARK		59
#define RES_BLIND		60
#define RES_CONFU		61
#define RES_SOUND		62
#define RES_SHARD		63
#define RES_NEXUS		64
#define RES_NETHR		65
#define RES_CHAOS		66
#define RES_DISEN		67

#define SLOW_DIGEST		70
#define FEATHER	 		71
#define LIGHT			72
#define REGEN			73
#define TELEPATHY		74
#define SEE_INVIS		75
#define FREE_ACT		76
#define HOLD_LIFE		77
#define IMPACT			78
#define BLESSED			80

#define ADD_AC			94
#define IMPROVE_BASE_AC	95
#define ENHANCE_DICE	96
#define ADD_SKILL		97
#define ADD_DEADLINESS	98

#define VUL_ACID		100
#define VUL_ELEC		101
#define VUL_FIRE		102
#define VUL_COLD		103
#define VUL_POIS		104
#define VUL_LIGHT		105
#define VUL_DARK		106
#define VUL_CONFU		107
#define VUL_SOUND		108
#define VUL_SHARD		109
#define VUL_NEXUS		110
#define VUL_NETHR		111
#define VUL_CHAOS		112
#define VUL_DISEN		113

/* Random artifact activations are defined in defines.h. */

/**
 * Debit an artifact's account.
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
 * Grant the quality asked for, if the artifact can afford it.
 */
static bool get_quality(bool on_credit, int purchase, int value, int a_idx)
{
	artifact_type *a_ptr = &a_info[a_idx];
	int temp, i, choice;
	switch (purchase) {
	case ADD_STR:
		{
			if (take_money(on_credit, (150 * value) + (value * value * 8))) {
				a_ptr->bonus_stat[A_STR] += value;
				return (TRUE);
			}
			break;
		}
	case ADD_WIS:
		{
			if (mp_ptr->spell_stat == A_WIS) {
				if (take_money
					(on_credit, (150 * value) + (value * value * 8))) {
					a_ptr->bonus_stat[A_WIS] += value;
					return (TRUE);
				}
			}

			else {
				if (take_money
					(on_credit, (100 * value) + (value * value * 5))) {
					a_ptr->bonus_stat[A_WIS] += value;
					return (TRUE);
				}
			}
			break;
		}
	case ADD_INT:
		{
			if (mp_ptr->spell_stat == A_INT) {
				if (take_money
					(on_credit, (150 * value) + (value * value * 8))) {
					a_ptr->bonus_stat[A_INT] += value;
					return (TRUE);
				}
			}

			else {
				if (take_money
					(on_credit, (100 * value) + (value * value * 5))) {
					a_ptr->bonus_stat[A_INT] += value;
					return (TRUE);
				}
			}
			break;
		}
	case ADD_DEX:
		{
			if (take_money(on_credit, (130 * value) + (value * value * 7))) {
				a_ptr->bonus_stat[A_DEX] += value;
				return (TRUE);
			}
			break;
		}
	case ADD_CON:
		{
			if (take_money(on_credit, (130 * value) + (value * value * 7))) {
				a_ptr->bonus_stat[A_CON] += value;
				return (TRUE);
			}
			break;
		}
	case ADD_CHR:
		{
			if (take_money(on_credit, (40 * value) + (value * value * 2))) {
				a_ptr->bonus_stat[A_CHR] += value;
				return (TRUE);
			}
			break;
		}
	case MAGIC_MASTERY:
		{
			if (take_money(on_credit, (20 * value) + (value * value * 1))) {
				a_ptr->bonus_other[P_BONUS_M_MASTERY] += value;
				return (TRUE);
			}
			break;
		}
	case STEALTH:
		{
			if (take_money(on_credit, (100 * value) + (value * value * 5))) {
				a_ptr->bonus_other[P_BONUS_STEALTH] += value;
				return (TRUE);
			}
			break;
		}
	case SEARCH:
		{
			if (take_money(on_credit, (20 * value) + (value * value * 1))) {
				a_ptr->bonus_other[P_BONUS_SEARCH] += value;
				return (TRUE);
			}
			break;
		}
	case INFRA:
		{
			if (take_money(on_credit, (15 * value) + (value * value * 1))) {
				a_ptr->bonus_other[P_BONUS_INFRA] += value;
				return (TRUE);
			}
			break;
		}
	case TUNNEL:
		{
			if (take_money(on_credit, (25 * value) + (value * value * 1))) {
				a_ptr->bonus_other[P_BONUS_TUNNEL] += value;
				return (TRUE);
			}
			break;
		}
	case SPEED:
		{
			if (take_money
				(on_credit, (360 * value) + (value * value * 18))) {
				a_ptr->bonus_other[P_BONUS_SPEED] += value;
				return (TRUE);
			}
			break;
		}
	case MIGHT:
		{
			if (take_money(on_credit, 1250 * value)) {
				a_ptr->bonus_other[P_BONUS_MIGHT] += value;
				return (TRUE);
			}
			break;
		}
	case SHOTS:
		{
			if (take_money(on_credit, 1500 * value)) {
				a_ptr->bonus_other[P_BONUS_SHOTS] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_ANIMAL:
		{
			if (take_money(on_credit, 60 * value)) {
				a_ptr->multiple_slay[P_SLAY_ANIMAL] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_EVIL:
		{
			if (take_money(on_credit, 100 * value)) {
				a_ptr->multiple_slay[P_SLAY_EVIL] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_UNDEAD:
		{
			if (take_money(on_credit, 60 * value)) {
				a_ptr->multiple_slay[P_SLAY_UNDEAD] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_DEMON:
		{
			if (take_money(on_credit, 50 * value)) {
				a_ptr->multiple_slay[P_SLAY_DEMON] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_ORC:
		{
			if (take_money(on_credit, 40 * value)) {
				a_ptr->multiple_slay[P_SLAY_ORC] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_TROLL:
		{
			if (take_money(on_credit, 50 * value)) {
				a_ptr->multiple_slay[P_SLAY_TROLL] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_GIANT:
		{
			if (take_money(on_credit, 40 * value)) {
				a_ptr->multiple_slay[P_SLAY_GIANT] += value;
				return (TRUE);
			}
			break;
		}
	case SLAY_DRAGON:
		{
			if (take_money(on_credit, 60 * value)) {
				a_ptr->multiple_slay[P_SLAY_DRAGON] += value;
				return (TRUE);
			}
			break;
		}
	case PERFECT_BALANCE:
		{
			if (take_money(on_credit, 500)) {
				of_on(a_ptr->flags_obj, OF_PERFECT_BALANCE);
				return (TRUE);
			}
			break;
		}
	case BRAND_POIS:
		{
			if (take_money(on_credit, 100 * value)) {
				a_ptr->multiple_brand[P_BRAND_POIS] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_ACID:
		{
			if (take_money(on_credit, 100 * value)) {
				a_ptr->multiple_brand[P_BRAND_ACID] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_ELEC:
		{
			if (take_money(on_credit, 100 * value)) {
				a_ptr->multiple_brand[P_BRAND_ELEC] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_FIRE:
		{
			if (take_money(on_credit, 100 * value)) {
				a_ptr->multiple_brand[P_BRAND_FIRE] += value;
				return (TRUE);
			}
			break;
		}
	case BRAND_COLD:
		{
			if (take_money(on_credit, 100 * value)) {
				a_ptr->multiple_brand[P_BRAND_COLD] += value;
				return (TRUE);
			}
			break;
		}
	case SUST_STR:
		{
			if (take_money(on_credit, 550)) {
				of_on(a_ptr->flags_obj, OF_SUSTAIN_STR);
				return (TRUE);
			}
			break;
		}
	case SUST_WIS:
		{
			if (take_money(on_credit, 350)) {
				of_on(a_ptr->flags_obj, OF_SUSTAIN_WIS);
				return (TRUE);
			}
			break;
		}
	case SUST_INT:
		{
			if (take_money(on_credit, 350)) {
				of_on(a_ptr->flags_obj, OF_SUSTAIN_INT);
				return (TRUE);
			}
			break;
		}
	case SUST_DEX:
		{
			if (take_money(on_credit, 350)) {
				of_on(a_ptr->flags_obj, OF_SUSTAIN_DEX);
				return (TRUE);
			}
			break;
		}
	case SUST_CON:
		{
			if (take_money(on_credit, 350)) {
				of_on(a_ptr->flags_obj, OF_SUSTAIN_CON);
				return (TRUE);
			}
			break;
		}
	case SUST_CHR:
		{
			if (take_money(on_credit, 50)) {
				of_on(a_ptr->flags_obj, OF_SUSTAIN_CHR);
				return (TRUE);
			}
			break;
		}
	case IM_ACID:
		{
			if (take_money(on_credit, 3500)) {
				a_ptr->percent_res[P_RES_ACID] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case IM_ELEC:
		{
			if (take_money(on_credit, 2750)) {
				a_ptr->percent_res[P_RES_ELEC] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case IM_FIRE:
		{
			if (take_money(on_credit, 3250)) {
				a_ptr->percent_res[P_RES_FIRE] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case IM_COLD:
		{
			if (take_money(on_credit, 2750)) {
				a_ptr->percent_res[P_RES_COLD] = RES_BOOST_IMMUNE;
				return (TRUE);
			}
			break;
		}
	case RES_ACID:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_ACID] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_ELEC:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_ELEC] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_FIRE:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_FIRE] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_COLD:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_COLD] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_POIS:
		{
			choice = randint0(6);
			if (take_money(on_credit, 1200 - 100 * choice)) {
				a_ptr->percent_res[P_RES_POIS] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_FEAR:
		{
			if (take_money(on_credit, 500)) {
				of_on(a_ptr->flags_obj, OF_FEARLESS);
				return (TRUE);
			}
			break;
		}
	case RES_LIGHT:
		{
			choice = randint0(6);
			if (take_money(on_credit, 750 - 60 * choice)) {
				a_ptr->percent_res[P_RES_LIGHT] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_DARK:
		{
			choice = randint0(6);
			if (take_money(on_credit, 750 - 60 * choice)) {
				a_ptr->percent_res[P_RES_DARK] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_BLIND:
		{
			if (take_money(on_credit, 650)) {
				of_on(a_ptr->flags_obj, OF_SEEING);
				return (TRUE);
			}
			break;
		}
	case RES_CONFU:
		{
			choice = randint0(6);
			if (take_money(on_credit, 900 - 75 * choice)) {
				a_ptr->percent_res[P_RES_CONFU] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_SOUND:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_SOUND] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_SHARD:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_SHARD] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_NEXUS:
		{
			choice = randint0(6);
			if (take_money(on_credit, 600 - 50 * choice)) {
				a_ptr->percent_res[P_RES_NEXUS] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_NETHR:
		{
			choice = randint0(6);
			if (take_money(on_credit, 1000 - 80 * choice)) {
				a_ptr->percent_res[P_RES_NETHR] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_CHAOS:
		{
			choice = randint0(6);
			if (take_money(on_credit, 1000 - 80 * choice)) {
				a_ptr->percent_res[P_RES_CHAOS] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case RES_DISEN:
		{
			choice = randint0(6);
			if (take_money(on_credit, 1200 - 100 * choice)) {
				a_ptr->percent_res[P_RES_DISEN] = 40 + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case SLOW_DIGEST:
		{
			if (take_money(on_credit, 300)) {
				of_on(a_ptr->flags_obj, OF_SLOW_DIGEST);
				return (TRUE);
			}
			break;
		}
	case FEATHER:
		{
			if (take_money(on_credit, 300)) {
				of_on(a_ptr->flags_obj, OF_FEATHER);
				return (TRUE);
			}
			break;
		}
	case LIGHT:
		{
			if (take_money(on_credit, 500)) {
				of_on(a_ptr->flags_obj, OF_LIGHT);
				return (TRUE);
			}
			break;
		}
	case REGEN:
		{
			if (take_money(on_credit, 500)) {
				of_on(a_ptr->flags_obj, OF_REGEN);
				return (TRUE);
			}
			break;
		}
	case TELEPATHY:
		{
			if (take_money(on_credit, 2500)) {
				of_on(a_ptr->flags_obj, OF_TELEPATHY);
				return (TRUE);
			}
			break;
		}
	case SEE_INVIS:
		{
			if (take_money(on_credit, 700)) {
				of_on(a_ptr->flags_obj, OF_SEE_INVIS);
				return (TRUE);
			}
			break;
		}
	case FREE_ACT:
		{
			if (take_money(on_credit, 700)) {
				of_on(a_ptr->flags_obj, OF_FREE_ACT);
				return (TRUE);
			}
			break;
		}
	case HOLD_LIFE:
		{
			if (take_money(on_credit, 1400)) {
				of_on(a_ptr->flags_obj, OF_HOLD_LIFE);
				return (TRUE);
			}
			break;
		}
	case IMPACT:
		{
			if (take_money(on_credit, 0)) {
				of_on(a_ptr->flags_obj, OF_IMPACT);
				return (TRUE);
			}
			break;
		}
	case BLESSED:
		{
			if ((player_has(PF_BLESS_WEAPON))
				& ((a_ptr->tval == TV_POLEARM)
				   || (a_ptr->tval == TV_SWORD))) {
				if (take_money(on_credit, 400)) {
					of_on(a_ptr->flags_obj, OF_BLESSED);
					return (TRUE);
				}
			}

			else {
				if (take_money(on_credit, 0)) {
					of_on(a_ptr->flags_obj, OF_BLESSED);
					return (TRUE);
				}
			}
			break;
		}
	case ADD_AC:
		{
			if (take_money(on_credit, value * 40)) {
				a_ptr->to_a += value;
				return (TRUE);
			}
			break;
		}
	case IMPROVE_BASE_AC:
		{
			if (take_money(on_credit, value * 40)) {
				a_ptr->ac += value;
				return (TRUE);
			}
			break;
		}
	case ENHANCE_DICE:
		{
			int oldds;

			/* save the current ds - to make sure the new one isn't too
			 * outlandish */
			oldds = a_ptr->ds;

			/* Allow a portion of the budget for improvements.  Use "value" to
			 * control fraction. */
			temp = potential / value;
			if ((value > 1) && (temp > 1000))
				temp = 1000;

			/* If credit is extended, guarantee something to work with. */
			if (on_credit) {
				if (temp < 500)
					temp = 500;
				if (potential > temp)
					potential -= temp;

				else
					potential = 0;
			}

			/* Otherwise, subtract whatever portion is alloted from the main
			 * budget. */
			else
				potential -= temp;

			/* Enhance the damage dice, depending on potential. */
			/* SJGU Hack. Prevent most single dice weapons getting massive ds */
			if ((randint1(4) != 1) && ((a_ptr->dd == 1) && (temp >= 400))) {
				while (temp <= 100 * a_ptr->ds) {
					a_ptr->ds--;
					temp += 200;
				}
				a_ptr->dd++;
				temp -= 100 * a_ptr->ds;
			}
			for (i = 0; i < 5; i++) {
				if ((randint1(3) != 1) && (temp >= 100 * a_ptr->ds)) {
					a_ptr->dd++;
					temp -= 100 * a_ptr->ds;
				}

				else if (temp >= 200 * a_ptr->dd) {
					a_ptr->ds = a_ptr->ds + 2;
					temp -= 200 * a_ptr->dd;
				}

				/* Either stop the loop or increment the dice sides. */
				else {
					if (temp < 100 * a_ptr->dd)
						break;
					a_ptr->ds++;
					temp -= 100 * a_ptr->dd;
				}

				/* SJGU Hack. Prevent any weapons getting crazy ds. Boo Hiss */
				/* Note that ENHANCE_DICE can potentially be called more than
				 * once though */
				/* In which case, Happy Birthday! */
				if (a_ptr->ds >= 2 * oldds)
					break;
			}

			/* Pour any remainder back into the main budget. */
			potential += temp;
			break;
		}
	case ADD_SKILL:
		{
			if (take_money(on_credit, value * 40)) {
				a_ptr->to_h += value;
				return (TRUE);
			}
			break;
		}
	case ADD_DEADLINESS:
		{
			if (take_money(on_credit, value * 40)) {
				a_ptr->to_d += value;
				return (TRUE);
			}
			break;
		}
	case VUL_ACID:
		{
			if (a_ptr->percent_res[P_RES_ACID] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_ACID] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_ELEC:
		{
			if (a_ptr->percent_res[P_RES_ELEC] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_ELEC] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_FIRE:
		{
			if (a_ptr->percent_res[P_RES_FIRE] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_FIRE] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_COLD:
		{
			if (a_ptr->percent_res[P_RES_COLD] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_COLD] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_POIS:
		{
			if (a_ptr->percent_res[P_RES_POIS] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 100 * choice)) {
				a_ptr->percent_res[P_RES_POIS] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_LIGHT:
		{
			if (a_ptr->percent_res[P_RES_LIGHT] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 60 * choice)) {
				a_ptr->percent_res[P_RES_LIGHT] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_DARK:
		{
			if (a_ptr->percent_res[P_RES_DARK] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 60 * choice)) {
				a_ptr->percent_res[P_RES_DARK] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_CONFU:
		{
			if (a_ptr->percent_res[P_RES_CONFU] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 75 * choice)) {
				a_ptr->percent_res[P_RES_CONFU] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_SOUND:
		{
			if (a_ptr->percent_res[P_RES_SOUND] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_SOUND] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_SHARD:
		{
			if (a_ptr->percent_res[P_RES_SHARD] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_SHARD] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_NEXUS:
		{
			if (a_ptr->percent_res[P_RES_NEXUS] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 50 * choice)) {
				a_ptr->percent_res[P_RES_NEXUS] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_NETHR:
		{
			if (a_ptr->percent_res[P_RES_NETHR] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 80 * choice)) {
				a_ptr->percent_res[P_RES_NETHR] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_CHAOS:
		{
			if (a_ptr->percent_res[P_RES_CHAOS] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 80 * choice)) {
				a_ptr->percent_res[P_RES_CHAOS] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	case VUL_DISEN:
		{
			if (a_ptr->percent_res[P_RES_DISEN] < RES_LEVEL_BASE)
				return (FALSE);
			choice = randint0(6);
			if (take_money(on_credit, 0 - 100 * choice)) {
				a_ptr->percent_res[P_RES_DISEN] =
					RES_LEVEL_BASE + 5 * choice;
				return (TRUE);
			}
			break;
		}
	}

	/* No stock with that description on the shelf, or price too high. */
	return (FALSE);
}


/**
 * Assign a tval and sval, grant a certain amount of potential to be used 
 * for acquiring powers, and determine rarity and native depth.
 */
static void initialize_artifact(int a_idx)
{
	int i;
	int index, freq, rarity;
	int base_object_depth, base_object_rarity;
	int artifact_depth, artifact_rarity;
	int power_of_base_object = 0;
	int base_object_activation = 0;
	object_kind *k_ptr;
	artifact_type *a_ptr = &a_info[a_idx];

	/* Wipe the artifact completely. */
	a_ptr->tval = 0;
	a_ptr->sval = 0;
	a_ptr->to_h = 0;
	a_ptr->to_d = 0;
	a_ptr->to_a = 0;
	a_ptr->ac = 0;
	a_ptr->dd = 0;
	a_ptr->ds = 0;
	a_ptr->weight = 0;
	a_ptr->cost = 0;
	of_wipe(a_ptr->flags_obj);
	cf_wipe(a_ptr->flags_curse);
	a_ptr->level = 0;
	a_ptr->rarity = 0;
	a_ptr->effect = 0;

	/* Default resists, bonuses, multiples */
	for (i = 0; i < MAX_P_RES; i++)
		a_ptr->percent_res[i] = RES_LEVEL_BASE;
	for (i = 0; i < A_MAX; i++)
		a_ptr->bonus_stat[i] = BONUS_BASE;
	for (i = 0; i < MAX_P_BONUS; i++)
		a_ptr->bonus_other[i] = BONUS_BASE;
	for (i = 0; i < MAX_P_SLAY; i++)
		a_ptr->multiple_slay[i] = MULTIPLE_BASE;
	for (i = 0; i < MAX_P_BRAND; i++)
		a_ptr->multiple_brand[i] = MULTIPLE_BASE;

	/* Assign a tval and sval.  If the base object is powerful, an amount to
	 * reduce artifact power by will be calculated.  If the base object has an
	 * activation, it will be preserved (at least initially). */
	while (TRUE) {

		/* Acquire an object at random */
		index = randint0(z_info->k_max);
		k_ptr = &k_info[index];

		/* Skip "empty" objects */
		if (!k_ptr->name)
			continue;

		/* Skip objects that are not weapons or armour. */
		if ((k_ptr->tval != TV_BOW) && (k_ptr->tval != TV_HAFTED)
			&& (k_ptr->tval != TV_POLEARM) && (k_ptr->tval != TV_SWORD)
			&& (k_ptr->tval != TV_BOOTS) && (k_ptr->tval != TV_GLOVES)
			&& (k_ptr->tval != TV_HELM) && (k_ptr->tval != TV_CROWN)
			&& (k_ptr->tval != TV_SHIELD) && (k_ptr->tval != TV_CLOAK)
			&& (k_ptr->tval != TV_SOFT_ARMOR)
			&& (k_ptr->tval != TV_HARD_ARMOR)
			&& (k_ptr->tval != TV_DRAG_ARMOR)) {
			continue;
		}

		/* Make melee weapons a bit less frequent -NRM- */
		if (((k_ptr->tval == TV_HAFTED) || (k_ptr->tval != TV_POLEARM)
			 || (k_ptr->tval != TV_SWORD)) && (randint1(3) == 1))
			continue;

	/*** Determine rarity.  Method:  adding fractions ***/
		freq = 0;

		/* Scan all four allocation chance values */
		for (i = 0; i < 4; i++) {

			/* Skip empty values. */
			if (k_ptr->chance[i] == 0)
				continue;

			/* Sum relative chances of object being made. */
			freq += (1000 / k_ptr->chance[i]);
		}
		if (!freq)
			continue;
		rarity = 1000 / freq;

		/* Accept object if it passes the rarity roll. */
		if (randint0(rarity) == 0)
			break;
	}

	/* Determine "power" and get activation of base object. */
	switch (k_ptr->tval) {
	case TV_BOW:
		{
			if (k_ptr->sval == SV_HEAVY_XBOW)
				power_of_base_object = 750;
			break;
		}
	case TV_HAFTED:
		{
			if (k_ptr->sval == SV_MACE_OF_DISRUPTION)
				power_of_base_object = 500;
			break;
		}
	case TV_POLEARM:
		{
			if (k_ptr->sval == SV_SCYTHE_OF_SLICING)
				power_of_base_object = 750;
			break;
		}
	case TV_SWORD:
		{
			if (k_ptr->sval == SV_EXECUTIONERS_SWORD)
				power_of_base_object = 500;
			if (k_ptr->sval == SV_VALINOREAN_SWORD)
				power_of_base_object = 800;
			break;
		}
	case TV_SHIELD:
		{
			if (k_ptr->sval == SV_SHIELD_OF_DEFLECTION)
				power_of_base_object = 500;
			break;
		}
	case TV_HARD_ARMOR:
		{
			if (k_ptr->sval == SV_MITHRIL_CHAIN_MAIL)
				power_of_base_object = 500;
			if (k_ptr->sval == SV_MITHRIL_PLATE_MAIL)
				power_of_base_object = 700;
			if (k_ptr->sval == SV_ADAMANTITE_PLATE_MAIL)
				power_of_base_object = 500;
			break;
		}
	case TV_DRAG_ARMOR:
		{
			if (k_ptr->sval == SV_DRAGON_BLACK) {
				power_of_base_object = 750;
				base_object_activation = EF_DRAGON_BLACK;
			}
			if (k_ptr->sval == SV_DRAGON_BLUE) {
				power_of_base_object = 750;
				base_object_activation = EF_DRAGON_BLUE;
			}
			if (k_ptr->sval == SV_DRAGON_WHITE) {
				power_of_base_object = 750;
				base_object_activation = EF_DRAGON_WHITE;
			}
			if (k_ptr->sval == SV_DRAGON_RED) {
				power_of_base_object = 750;
				base_object_activation = EF_DRAGON_RED;
			}
			if (k_ptr->sval == SV_DRAGON_GREEN) {
				power_of_base_object = 1000;
				base_object_activation = EF_DRAGON_GREEN;
			}
			if (k_ptr->sval == SV_DRAGON_MULTIHUED) {
				power_of_base_object = 1500;
				base_object_activation = EF_DRAGON_MULTIHUED;
			}
			if (k_ptr->sval == SV_DRAGON_SHINING) {
				power_of_base_object = 1000;
				base_object_activation = EF_DRAGON_SHINING;
			}
			if (k_ptr->sval == SV_DRAGON_LAW) {
				power_of_base_object = 1250;
				base_object_activation = EF_DRAGON_LAW;
			}
			if (k_ptr->sval == SV_DRAGON_BRONZE) {
				power_of_base_object = 750;
				base_object_activation = EF_DRAGON_BRONZE;
			}
			if (k_ptr->sval == SV_DRAGON_GOLD) {
				power_of_base_object = 750;
				base_object_activation = EF_DRAGON_GOLD;
			}
			if (k_ptr->sval == SV_DRAGON_CHAOS) {
				power_of_base_object = 1250;
				base_object_activation = EF_DRAGON_CHAOS;
			}
			if (k_ptr->sval == SV_DRAGON_BALANCE) {
				power_of_base_object = 1750;
				base_object_activation = EF_DRAGON_BALANCE;
			}
			if (k_ptr->sval == SV_DRAGON_POWER) {
				power_of_base_object = 2250;
				base_object_activation = EF_DRAGON_POWER;
			}
			break;
		}
	}

	/* Store the base values of a bunch of data.  To avoid unbalancing the
	 * game, bonuses to Skill, Deadliness, and Armour Class are cut in half.
	 * Dragon scale mail activations are preserved.  Base object cost is
	 * preserved if sufficiently high. Rings and amulets are started again from 
	 * * * * scratch, and assigned a special sval. -NRM- */
	a_ptr->tval = k_ptr->tval;
	a_ptr->sval = k_ptr->sval;
	a_ptr->to_h = k_ptr->to_h.base / 2;
	a_ptr->to_d = k_ptr->to_d.base / 2;
	a_ptr->to_a = k_ptr->to_a.base / 2;
	a_ptr->ac = k_ptr->ac;
	a_ptr->dd = k_ptr->dd;
	a_ptr->ds = k_ptr->ds;
	if (k_ptr->cost > 4999)
		a_ptr->cost = k_ptr->cost;
	a_ptr->weight = k_ptr->weight;
	of_copy(a_ptr->flags_obj, k_ptr->flags_obj);
	cf_copy(a_ptr->flags_curse, k_ptr->flags_curse);
	for (i = 0; i < MAX_P_RES; i++)
		a_ptr->percent_res[i] = k_ptr->percent_res[i];
	for (i = 0; i < A_MAX; i++)
		a_ptr->bonus_stat[i] = k_ptr->bonus_stat[i];
	for (i = 0; i < MAX_P_BONUS; i++)
		a_ptr->bonus_other[i] = k_ptr->bonus_other[i];
	for (i = 0; i < MAX_P_SLAY; i++)
		a_ptr->multiple_slay[i] = k_ptr->multiple_slay[i];
	for (i = 0; i < MAX_P_BRAND; i++)
		a_ptr->multiple_brand[i] = k_ptr->multiple_brand[i];

	/* Ignore everything */
	of_on(a_ptr->flags_obj, OF_ACID_PROOF);
	of_on(a_ptr->flags_obj, OF_ELEC_PROOF);
	of_on(a_ptr->flags_obj, OF_FIRE_PROOF);
	of_on(a_ptr->flags_obj, OF_COLD_PROOF);
	a_ptr->effect = base_object_activation;

	/* The total potential of an artifact ranges from 1750 to 8750, biased
	 * towards the lesser figure. */
	/* Now less biased -NRM- */
	potential = 1750;
	for (i = 0; i < 14; i++) {
		if (randint1(15) > 2)
			potential += 500;

		else
			break;
	}

	/* 8750 is normally the maximum potential for any artifact. */
	max_potential = 8750;

	/* Preserve balance by not allowing powerful base objects to get very many
	 * extra powers. */
	potential -= power_of_base_object;

	/* Many equipment types include no truly powerful artifacts.  Maximum and
	 * actual potential is limited. SJGU but less so now */
	if (a_ptr->tval == TV_CLOAK) {
		if (potential > 5500)
			potential = 5500;
		max_potential = 5500;
	}
	if (a_ptr->tval == TV_GLOVES) {
		if (potential > 5500)
			potential = 5500;
		max_potential = 5500;
	}
	if (a_ptr->tval == TV_BOOTS) {
		if (potential > 8000)
			potential = 8000;
		max_potential = 8000;
	}
	if (a_ptr->tval == TV_CROWN) {
		if (potential > 6500)
			potential = 6500;
		max_potential = 6500;
	}
	if (a_ptr->tval == TV_HELM) {
		if (potential > 6000)
			potential = 6000;
		max_potential = 6000;
	}
	if (a_ptr->tval == TV_SHIELD) {
		if (potential > 6500)
			potential = 6500;
		max_potential = 6500;
	}
	if (a_ptr->tval == TV_BOW) {
		if (potential > 6500)
			potential = 6500;
		max_potential = 6500;
	}

	/* Regardless of other considerations, grant at least some potential. */
	if (potential < 2000)
		potential = 2000;

	/* Remember how much potential was allowed. */
	initial_potential = potential;

	/* The cost of the artifact depends solely on its potential (allowing for
	 * valuable base object kinds). */
	a_ptr->cost += (potential * 10L);

	/* Determine the base object depth and rarity (use all indexes). */
	base_object_depth = k_ptr->locale[0];
	base_object_rarity = k_ptr->chance[0];

	/* Paranoia. */
	if (base_object_rarity == 0)
		base_object_rarity = 1;

	/* Calculate artifact depth.  This is fairly simple. SJGU and hacked */
	artifact_depth = (potential / 100) - 10;
	if (artifact_depth < base_object_depth - 10)
		artifact_depth = base_object_depth - 10;
	if (artifact_depth < 5)
		artifact_depth = 5;

	/* Calculate artifact rarity.  This requires handling some special cases. */
	artifact_rarity =
		(((potential + power_of_base_object) / 250) / base_object_rarity);

	/* Modify artifact rarity to handle some special cases. */
	if (a_ptr->tval == TV_SHIELD)
		artifact_rarity /= 2;
	if ((a_ptr->tval == TV_SOFT_ARMOR) || (a_ptr->tval == TV_HARD_ARMOR))
		artifact_rarity /= 2;
	if (a_ptr->tval == TV_CLOAK)
		artifact_rarity = 4 * artifact_rarity / 3;
	if ((a_ptr->tval == TV_HELM) || (a_ptr->tval == TV_CROWN))
		artifact_rarity = 2 * artifact_rarity / 3;
	if (a_ptr->tval == TV_BOOTS)
		artifact_rarity += 5;

	/* Boundary control. */
	if (artifact_rarity < 3)
		artifact_rarity = 3;

	/* Assign the values just calculated to the artifact. */
	a_ptr->level = artifact_depth;
	a_ptr->rarity = artifact_rarity;
}


/**
 * Pick an initial set of qualities, based on a theme.  Also add a bonus to 
 * armour class, Skill, and Deadliness.
 */
static void choose_basic_theme(int a_idx)
{
	int i, selection, temp, old_weight;
	artifact_type *a_ptr = &a_info[a_idx];
	object_kind *k_ptr = &k_info[lookup_kind(a_ptr->tval, a_ptr->sval)];

	/* Possibly make very powerful artifacts cursed. */
	if ((potential > 6000) && (potential > randint1(30000)))
		cf_on(a_ptr->flags_curse, FLAG_START + randint0(CF_MAX));

	/* Frequently (but not always) assign a basic theme to the artifact. */
	selection = randint0(100);
	switch (k_ptr->tval) {

		/* I'm a melee weapon... */
	case TV_SWORD:
	case TV_POLEARM:
	case TV_HAFTED:
		{

			/* ...with bonuses to Deadliness and Skill, and... */
			a_ptr->to_d += (3 + randint1(7) + potential / 650);
			a_ptr->to_h += (3 + randint1(7) + potential / 650);
			/* ...of fire. */
			if (selection < 6) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 6000)
						a_ptr->effect = EF_RAND_FIRE3;
					else if (potential >= 3000)
						a_ptr->effect = EF_RAND_FIRE2;
					else
						a_ptr->effect = EF_RAND_FIRE1;
				}

				/* Brand the weapon with fire. */
				get_quality(TRUE, BRAND_FIRE, 7, a_idx);
				/* Grant either a resist or immunity to fire. */
				if ((potential >= 4500) && (randint1(3) == 1))
					get_quality(FALSE, IM_FIRE, 0, a_idx);
				else
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				/* Sometimes vulnerable to cold */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_COLD, 0, a_idx);
				/* Demons like fire */
				if (randint1(4) == 1) {
					cf_on(a_ptr->flags_curse, CF_ATTRACT_DEMON);
					potential += 200;
				}
			}

			/* ...of frost. */
			else if (selection < 12) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 6000)
						a_ptr->effect = EF_RAND_COLD3;
					else if (potential >= 3000)
						a_ptr->effect = EF_RAND_COLD2;
					else
						a_ptr->effect = EF_RAND_COLD1;
				}

				/* Brand the weapon with frost. */
				get_quality(TRUE, BRAND_COLD, 7, a_idx);
				/* Grant either a resist or immunity to frost. */
				if ((potential >= 4500) && (randint1(3) == 1))
					get_quality(FALSE, IM_COLD, 0, a_idx);
				else
					get_quality(TRUE, RES_COLD, 0, a_idx);
				/* Sometimes vulnerable to fire */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_FIRE, 0, a_idx);
				/* Undead like cold */
				if (randint1(4) == 1) {
					cf_on(a_ptr->flags_curse, CF_ATTRACT_UNDEAD);
					potential += 200;
				}
			}

			/* ...of acid. */
			else if (selection < 18) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 6000)
						a_ptr->effect = EF_RAND_ACID3;
					else if (potential >= 3000)
						a_ptr->effect = EF_RAND_ACID2;
					else
						a_ptr->effect = EF_RAND_ACID1;
				}

				/* Brand the weapon with acid. */
				get_quality(TRUE, BRAND_ACID, 7, a_idx);
				/* Grant either a resist or immunity to acid. */
				if ((potential >= 4500) && (randint1(3) == 1))
					get_quality(FALSE, IM_ACID, 0, a_idx);
				else
					get_quality(TRUE, RES_ACID, 0, a_idx);
				/* Sometimes vulnerable to electricity */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_ELEC, 0, a_idx);
			}

			/* ...of electricity. */
			else if (selection < 24) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 6000)
						a_ptr->effect = EF_RAND_ELEC3;
					else if (potential >= 3000)
						a_ptr->effect = EF_RAND_ELEC2;
					else
						a_ptr->effect = EF_RAND_ELEC1;
				}

				/* Brand the weapon with electricity. */
				get_quality(TRUE, BRAND_ELEC, 7, a_idx);
				/* Grant either a resist or immunity to electricity. */
				if ((potential >= 4500) && (randint1(3) == 1))
					get_quality(FALSE, IM_ELEC, 0, a_idx);
				else
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				/* Sometimes vulnerable to acid */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_ACID, 0, a_idx);
			}

			/* ...of poison. */
			else if (selection < 28) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 4500)
						a_ptr->effect = EF_RAND_POIS2;
					else
						a_ptr->effect = EF_RAND_POIS1;
				}

				/* Brand the weapon with poison. */
				get_quality(TRUE, BRAND_POIS, 7, a_idx);
				/* Grant resistance to poison. */
				get_quality(TRUE, RES_POIS, 0, a_idx);
				/* Comes with the territory */
				if (randint1(3) == 1) {
					cf_on(a_ptr->flags_curse, CF_POIS_RAND);
					potential += 100;
				}

				else if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_POIS_RAND_BAD);
					potential += 300;
				}
			}

			/* ...of life-sustaining. */
			else if (selection < 30) {

				/* This is an exclusive club. */
				if (potential >= 4000) {

					/* Possibly assign an activation for free. */
					if (randint1(3) == 1) {
						a_ptr->effect = EF_RAND_REGAIN;
					}

					/* Grant hold life. */
					get_quality(FALSE, HOLD_LIFE, 0, a_idx);
					/* Probably slay evil. */
					if (randint1(3) != 1)
						get_quality(FALSE, SLAY_EVIL, 5, a_idx);
					/* Possibly resist nether. */
					if (randint1(3) == 1)
						get_quality(FALSE, RES_NETHR, 0, a_idx);
					/* Possibly see invisible. */
					if (randint1(2) == 1)
						get_quality(FALSE, SEE_INVIS, 0, a_idx);
					/* Possibly slay undead. */
					if (randint1(3) == 1)
						get_quality(FALSE, SLAY_UNDEAD, 10, a_idx);
					/* But can leave the user immobile */
					if (randint1(8) == 1) {
						cf_on(a_ptr->flags_curse, CF_PARALYZE);
						potential += 700;
					}

					else if (randint1(8) == 1) {
						cf_on(a_ptr->flags_curse, CF_PARALYZE_ALL);
						potential += 1000;
					}
				}
			}

			/* ...of retain stats. */
			else if (selection < 32) {

				/* This is an exclusive club. */
				if (potential < 3500)
					break;
				/* Possibly assign an activation for free. */
				if (randint1(3) == 1) {
					a_ptr->effect = EF_RAND_RESTORE;
				}

				/* Grant resist nexus. */
				get_quality(FALSE, RES_NEXUS, 0, a_idx);
				/* Possibly resist disenchantment. */
				if (randint1(3) == 1)
					get_quality(FALSE, RES_DISEN, 0, a_idx);
				/* And some sustains. */
				if ((mp_ptr->spell_stat == A_STR) || (randint1(2) == 1))
					get_quality(FALSE, SUST_STR, 0, a_idx);
				if ((mp_ptr->spell_stat == A_WIS) || (randint1(2) == 1))
					get_quality(FALSE, SUST_WIS, 0, a_idx);
				if ((mp_ptr->spell_stat == A_INT) || (randint1(2) == 1))
					get_quality(FALSE, SUST_INT, 0, a_idx);
				if ((mp_ptr->spell_stat == A_DEX) || (randint1(2) == 1))
					get_quality(FALSE, SUST_DEX, 0, a_idx);
				if ((mp_ptr->spell_stat == A_CON) || (randint1(2) == 1))
					get_quality(FALSE, SUST_CON, 0, a_idx);
				if ((mp_ptr->spell_stat == A_CHR) || (randint1(2) == 1))
					get_quality(FALSE, SUST_CHR, 0, a_idx);
				/* May need to be handled with care */
				if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_CUT_RAND);
					potential += 100;
				}

				else if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_CUT_RAND_BAD);
					potential += 200;
				}
			}

			/* ...that shines with holy light. */
			else if (selection < 36) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential < 4500)
						a_ptr->effect = EF_RAND_LIGHT1;
					else
						a_ptr->effect = EF_RAND_LIGHT2;
				}

				/* Grant resist light and dark. */
				get_quality(TRUE, RES_LIGHT, 0, a_idx);
				get_quality(TRUE, RES_DARK, 0, a_idx);
				/* Possibly resist blindness. */
				if (randint1(2) == 1)
					get_quality(FALSE, RES_BLIND, 0, a_idx);
			}

			/* ...with plenty of slays. */
			else if (selection < 41) {

				/* Load up on the slays. */
				if ((randint1(6) == 1) && (potential >= 2000)) {
					get_quality(TRUE, SLAY_EVIL,
								(randint1(3) == 1 ? 7 : 5), a_idx);
					get_quality(TRUE, SLAY_ORC,
								(randint1(3) == 1 ? 15 : 10), a_idx);
					get_quality(TRUE, SLAY_TROLL,
								(randint1(3) == 1 ? 15 : 10), a_idx);
					get_quality(TRUE, SLAY_GIANT,
								(randint1(3) == 1 ? 15 : 10), a_idx);
				}

				else if ((randint1(5) == 1) && (potential >= 2000)) {
					get_quality(TRUE, SLAY_EVIL,
								(randint1(3) == 1 ? 7 : 5), a_idx);
					get_quality(TRUE, SLAY_UNDEAD,
								(randint1(3) == 1 ? 15 : 10), a_idx);
					get_quality(TRUE, SLAY_DEMON,
								(randint1(3) == 1 ? 15 : 10), a_idx);
				}

				else {
					if (randint1(2) == 1)
						get_quality(FALSE, SLAY_ANIMAL,
									(randint1(3) == 1 ? 10 : 7), a_idx);
					if (randint1(2) == 1)
						get_quality(FALSE, SLAY_DRAGON,
									(randint1(3) == 1 ? 15 : 10), a_idx);
					if (randint1(4) == 1)
						get_quality(FALSE, SLAY_EVIL,
									(randint1(3) == 1 ? 7 : 5), a_idx);
					if (randint1(3) == 1)
						get_quality(FALSE, SLAY_ORC,
									(randint1(3) == 1 ? 15 : 10), a_idx);
					if (randint1(3) == 1)
						get_quality(FALSE, SLAY_TROLL,
									(randint1(3) == 1 ? 15 : 10), a_idx);
					if (randint1(3) == 1)
						get_quality(FALSE, SLAY_DEMON,
									(randint1(3) == 1 ? 15 : 10), a_idx);
					if (randint1(3) == 1)
						get_quality(FALSE, SLAY_GIANT,
									(randint1(3) == 1 ? 15 : 10), a_idx);
				}

				/* Sometimes vulnerable to confusion */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_CONFU, 0, a_idx);
				/* Sometimes aggravate - but with compensation */
				if (randint1(10) == 1) {
					cf_on(a_ptr->flags_curse, CF_AGGRO_PERM);
					potential += 1000;
				}

				else if (randint1(10) == 1) {
					cf_on(a_ptr->flags_curse, CF_AGGRO_RAND);
					potential += 1000;
				}
			}

			/* ...with enhanced damage dice. */
			else if (selection < 46) {

				/* Use up to a third of the budget for enhancing the damage
				 * dice. */
				get_quality(TRUE, ENHANCE_DICE, 3, a_idx);
				/* Not so good for spellcasters */
				if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_DRAIN_MANA);
					potential += 300;
				}
			}

			/* ...that a druid wants by his side. */
			else if (selection < 50) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential >= 2500)) {
					a_ptr->effect = EF_RAND_SLOW_FOE;
				}

				/* Provide regenerative powers. */
				get_quality(FALSE, REGEN, 0, a_idx);
				/* Mark the weapon for a later bonus to stealth. */
				get_quality(FALSE, STEALTH, randint1(4), a_idx);
				/* Sometimes vulnerable to sound */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_SOUND, 0, a_idx);
			}

			/* ...that is just the thing a mage is looking for. */
			else if (selection < 54) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential >= 2500)) {
					a_ptr->effect = EF_RAND_SLEEP_FOE;
				}

				/* Provide resistance to blindness. */
				get_quality(FALSE, RES_BLIND, 0, a_idx);
				/* Bonus to magic mastery. */
				if (randint1(2) == 1)
					get_quality(FALSE, MAGIC_MASTERY, randint0(4), a_idx);
				/* Sometimes vulnerable to light or dark */
				if (randint1(8) == 1)
					get_quality(FALSE, VUL_LIGHT, 0, a_idx);
				else if (randint1(8) == 1)
					get_quality(FALSE, VUL_DARK, 0, a_idx);
				/* ...but sight can bring fear */
				if (randint1(4) == 1) {
					cf_on(a_ptr->flags_curse, CF_AFRAID);
					potential += 100;
				}
			}

			/* ...that a priest prays for. */
			else if (selection < 58) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential >= 2500)) {
					a_ptr->effect = EF_RAND_TURN_FOE;
				}

				/* Provide permanant light. */
				get_quality(FALSE, LIGHT, 0, a_idx);
				/* Bless the weapon. */
				get_quality(TRUE, BLESSED, 0, a_idx);
				/* Bonus to wisdom. */
				get_quality(FALSE, ADD_WIS, randint1(4), a_idx);
				/* Sometimes vulnerable to dark */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_DARK, 0, a_idx);
			}

			/* ...that a necromancer would kill for. */
			else if (selection < 62) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential >= 2500)) {
					a_ptr->effect = EF_RAND_CONFU_FOE;
				}

				/* Provide resistance to confusion. */
				get_quality(FALSE, RES_CONFU, 0, a_idx);
				/* Bonus to intelligence. */
				get_quality(FALSE, ADD_INT, randint1(4), a_idx);
				/* Sometimes vulnerable to light */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_LIGHT, 0, a_idx);
				/* Comes at a cost */
				if (randint1(3) == 1) {
					cf_on(a_ptr->flags_curse, CF_DRAIN_EXP);
					potential += 600;
				}
			}

			/* ...twisted with chaos. */
			else if (selection < 65) {

				/* This is an exclusive club. */
				if (potential >= 4000) {

					/* Assign an activation for free. */
					a_ptr->effect = EF_RAND_CHAOS;
					/* Resist chaos and disenchantment. */
					get_quality(TRUE, RES_CHAOS, 0, a_idx);
					get_quality(TRUE, RES_DISEN, 0, a_idx);
					/* Sometimes vulnerable to shards */
					if (randint1(5) == 1)
						get_quality(FALSE, VUL_SHARD, 0, a_idx);
					/* Can be unreliable */
					if (randint1(4) == 1) {
						cf_on(a_ptr->flags_curse, CF_DROP_WEAPON);
						potential += 200;
					}
				}
			}

			/* ...that is a strong champion of order. */
			else if (selection < 68) {

				/* This is an exclusive club. */
				if (potential >= 4000) {

					/* Assign an activation for free. */
					a_ptr->effect = EF_RAND_SHARD_SOUND;
					/* Resist shards, sound, and confusion. */
					get_quality(TRUE, RES_SHARD, 0, a_idx);
					get_quality(TRUE, RES_SOUND, 0, a_idx);
					get_quality(TRUE, RES_CONFU, 0, a_idx);
					/* Sometimes vulnerable to chaos and/or disenchantment */
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_CHAOS, 0, a_idx);
					if (randint1(8) == 1)
						get_quality(FALSE, VUL_DISEN, 0, a_idx);
				}
			}

			/* ...that smashes foes and dungeon alike. */
			else if (selection < 72) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					a_ptr->effect = EF_RAND_EARTHQUAKE;
				}

				/* Enhance the damage dice. */
				get_quality(TRUE, ENHANCE_DICE, 3, a_idx);
				/* 75% of the time, an earthquake brand.  SJGU but only if the
				 * dice are big enough */
				if ((a_ptr->dd * a_ptr->ds > 15) && (randint1(4) != 1))
					get_quality(TRUE, IMPACT, 0, a_idx);
				/* Grant either resistance or immunity to acid. */
				if ((potential >= 3500) && (randint1(3) == 1))
					get_quality(FALSE, IM_ACID, 0, a_idx);
				else
					get_quality(TRUE, RES_ACID, 0, a_idx);
				/* Bonus to tunneling. */
				get_quality(FALSE, TUNNEL, randint0(4), a_idx);
				/* Sometimes bonus to strength. */
				if ((potential >= 750) && (randint1(2) == 1))
					get_quality(FALSE, ADD_STR, randint1(4), a_idx);
				/* Sometimes vulnerable to chaos or confusion */
				if (randint1(8) == 1)
					get_quality(FALSE, VUL_CONFU, 0, a_idx);
				else if (randint1(8) == 1)
					get_quality(FALSE, VUL_CHAOS, 0, a_idx);
				/* ...and can make you see what isn't there */
				if (randint1(4) == 1) {
					cf_on(a_ptr->flags_curse, CF_HALLU_RAND);
					potential += 400;
				}
			}

			/* ...that hunts down all the children of nature. */
			else if (selection < 76) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					a_ptr->effect = EF_RAND_DETECT_MONSTERS;
				}

				/* Naturally, the appropriate slay. */
				get_quality(TRUE, SLAY_ANIMAL, 7, a_idx);
				/* SJGU ...and can be a great bane */
				if (randint1(2) == 1)
					get_quality(FALSE, SLAY_ANIMAL, 3, a_idx);
				/* A pair of survival skills. */
				get_quality(FALSE, REGEN, 0, a_idx);
				get_quality(FALSE, FEATHER, 0, a_idx);
				/* Mark the weapon for a later bonus to charisma. */
				get_quality(FALSE, ADD_CHR, randint1(4), a_idx);
				/* Sometimes mark the weapon for a later bonus to infravision. */
				if (randint1(2) == 1)
					get_quality(FALSE, INFRA, randint1(4), a_idx);
			}

			/* ...that Orcs and Trolls must fear. */
			else if (selection < 79) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 3000)
						a_ptr->effect = EF_RAND_STARLIGHT;
					else
						a_ptr->effect = EF_RAND_LINE_LIGHT;
				}

				/* Naturally, the appropriate slays. */
				get_quality(TRUE, SLAY_ORC, (randint1(3) == 1 ? 15 : 10),
							a_idx);
				get_quality(TRUE, SLAY_TROLL, (randint1(3) == 1 ? 15 : 10),
							a_idx);
				/* Often, grant a bonus to ac. */
				if (randint1(2) == 1)
					get_quality(FALSE, ADD_AC,
								randint1(4) + potential / 1000, a_idx);
				/* Sometimes, slay giant. */
				if (randint1(3) == 1)
					get_quality(FALSE, SLAY_GIANT, 10, a_idx);
				/* Bonus to strength. */
				get_quality(FALSE, ADD_STR, randint1(4), a_idx);
				/* Sometimes vulnerable to confusion */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_CONFU, 0, a_idx);
			}

			/* ...that the undead cannot withstand. */
			else if (selection < 82) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential > 2500)) {
					if (potential < 5000)
						a_ptr->effect = EF_RAND_SMITE_UNDEAD;
					else
						a_ptr->effect = EF_RAND_DISPEL_UNDEAD;
				}

				/* Grant slay undead and see invisible. */
				get_quality(TRUE, SLAY_UNDEAD,
							(randint1(3) == 1 ? 15 : 10), a_idx);
				get_quality(TRUE, SEE_INVIS, 0, a_idx);
				/* Sometimes, hold life. */
				if (randint1(3) == 1)
					get_quality(FALSE, HOLD_LIFE, 0, a_idx);
				/* Mark the weapon for a later bonus to wisdom. */
				get_quality(FALSE, ADD_WIS, randint1(4), a_idx);
				/* Sometimes vulnerable to light */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_LIGHT, 0, a_idx);
			}

			/* ...that evil creatures everywhere flee from. */
			else if (selection < 90) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if ((potential >= 5000) && (randint1(2) == 1))
						a_ptr->effect = EF_RAND_DISPEL_EVIL;
					else if (potential >= 5000)
						a_ptr->effect = EF_RAND_BANISH_EVIL;
					else if ((potential >= 3500) && (randint1(3) != 1))
						a_ptr->effect = EF_RAND_HOLY_ORB;
					else if (potential >= 2000)
						a_ptr->effect = EF_RAND_PROT_FROM_EVIL;
					else
						a_ptr->effect = EF_RAND_DETECT_EVIL;
				}

				/* Naturally, the appropriate slay. */
				get_quality(TRUE, SLAY_EVIL, (randint1(3) == 1 ? 7 : 5),
							a_idx);
				/* Bless the weapon if necessary. */
				if ((a_ptr->tval == TV_POLEARM)
					|| (a_ptr->tval == TV_SWORD))
					get_quality(FALSE, BLESSED, 0, a_idx);
				/* Sometimes, resist nether. */
				if (randint1(6) == 1)
					get_quality(FALSE, RES_NETHR, 0, a_idx);
				/* Sometimes, resist dark. */
				if (randint1(6) == 1)
					get_quality(FALSE, RES_DARK, 0, a_idx);
				/* Possibly bonus to intelligence. */
				if (randint1(3) == 1)
					get_quality(FALSE, ADD_INT, randint1(4), a_idx);
			}

			/* ...that demons intensely hate. */
			else if (selection < 93) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential > 2500)) {
					a_ptr->effect = EF_RAND_SMITE_DEMON;
				}

				/* Naturally, the appropriate slay. */
				get_quality(TRUE, SLAY_DEMON, (randint1(3) == 1 ? 15 : 10),
							a_idx);
				/* Sometimes, nip the spawn of hell with cold as well. */
				if (randint1(2) == 1)
					get_quality(FALSE, BRAND_COLD, 0, a_idx);
				/* Grant resistance to fire. */
				get_quality(FALSE, RES_FIRE, 0, a_idx);
				/* Bonus to dexerity. */
				get_quality(FALSE, ADD_DEX, randint1(4), a_idx);
				/* Sometimes vulnerable to cold */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_COLD, 0, a_idx);
			}

			/* ...that dragons long to destroy. */
			else if (selection < 96) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential > 2500)) {
					a_ptr->effect = EF_RAND_SMITE_DRAGON;
				}

				/* Naturally, the appropriate slay. */
				get_quality(TRUE, SLAY_DRAGON,
							(randint1(3) == 1 ? 15 : 10), a_idx);
				/* And one of the five elemental brands. */
				temp = randint1(5);
				if (temp == 1)
					get_quality(FALSE, BRAND_FIRE, 7, a_idx);
				if (temp == 2)
					get_quality(FALSE, BRAND_COLD, 7, a_idx);
				if (temp == 3)
					get_quality(FALSE, BRAND_ACID, 7, a_idx);
				if (temp == 4)
					get_quality(FALSE, BRAND_ELEC, 7, a_idx);
				if (temp == 5)
					get_quality(FALSE, BRAND_POIS, 7, a_idx);
				/* Bonus to constitution. */
				get_quality(FALSE, ADD_CON, randint1(4), a_idx);
			}

			/* ...that protects the wielder. */
			else if (selection < 1000) {

				/* This is an exclusive club. */
				if (potential < 2500)
					break;
				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 5000)
						a_ptr->effect = EF_RAND_SHIELD;
					else
						a_ptr->effect = EF_RAND_BLESS;
				}

				/* Grant a bonus to armour class. */
				get_quality(TRUE, ADD_AC, randint1(6) + potential / 800,
							a_idx);
				/* And the four basic resists. */
				get_quality(TRUE, RES_ACID, 0, a_idx);
				get_quality(TRUE, RES_ELEC, 0, a_idx);
				get_quality(TRUE, RES_FIRE, 0, a_idx);
				get_quality(TRUE, RES_COLD, 0, a_idx);
				/* And some of the survival abilities. */
				if (randint1(4) != 1)
					get_quality(TRUE, SEE_INVIS, 0, a_idx);
				if (randint1(4) != 1)
					get_quality(TRUE, FEATHER, 0, a_idx);
				if (randint1(4) != 1)
					get_quality(TRUE, FREE_ACT, 0, a_idx);
				if (randint1(4) != 1)
					get_quality(TRUE, REGEN, 0, a_idx);
			}

			/* HACK SJGU Any small dice weapon with big potential should get at 
			 * least some dice upgrade */
			if (((a_ptr->dd * a_ptr->ds) < 9)
				&& (initial_potential >= 3500))
				get_quality(TRUE, ENHANCE_DICE, 3, a_idx);
			break;
		}

		/* I'm a missile weapon... */
	case TV_BOW:
		{

			/* ...with bonuses to Deadliness and Skill, and... */
			a_ptr->to_d += (3 + randint1(9) + potential / 650);
			a_ptr->to_h += (3 + randint1(9) + potential / 650);
			/* ...of extra shots. */
			if (selection < 50) {
				get_quality(TRUE, SHOTS, 1, a_idx);
				/* Sometimes also grant extra might. */
				if ((potential >= 2000) && (randint1(3) == 1))
					get_quality(FALSE, MIGHT, 1, a_idx);
			}

			/* ...of extra might. */
			else {
				if ((potential >= 4000) && (randint1(2) == 1))
					get_quality(TRUE, MIGHT, 2, a_idx);
				else
					get_quality(TRUE, MIGHT, 1, a_idx);
				/* Sometimes also grant extra shots. */
				if ((potential >= 2250) && (randint1(3) == 1))
					get_quality(FALSE, SHOTS, 1, a_idx);
			}

			/* Sometimes, assign an activation. */
			if (potential > randint1(5000)) {
				if (randint1(2) == 1)
					a_ptr->effect = EF_RAND_SUPER_SHOOTING;
				else
					a_ptr->effect = EF_RAND_BRAND_MISSILE;
			}

			/* Hack - avoid boring bows. */
			if (potential < 1000)
				potential = 1000;
			break;
		}

		/* I'm a piece of body armour... */
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		{

			/* ...with a bonus to armour class, and... */
			a_ptr->to_a += (10 + randint1(8) + potential / 1500);
			/* ...that resists most or all of the elements. */
			if (selection < 30) {

				/* Possibly assign an activation for free. */
				if ((randint1(2) == 1) && (potential >= 3000)) {
					if (potential >= 5500)
						a_ptr->effect = EF_RAND_RESIST_ALL;
					else
						a_ptr->effect = EF_RAND_RESIST_ELEMENTS;
				}
				if (randint1(5) != 1)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) != 1)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) != 1)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) != 1)
					get_quality(TRUE, RES_COLD, 0, a_idx);
				/* Sometimes, also poison. */
				if ((randint1(2) == 1) && (potential >= 2000))
					get_quality(FALSE, RES_POIS, 0, a_idx);
			}

			/* ...that protects the very soul. */
			else if (selection < 40) {

				/* This is an exclusive club. */
				if (potential >= 5500) {

					/* Possibly assign an activation for free. */
					if (randint1(3) != 1) {
						if (randint1(2) == 1)
							a_ptr->effect = EF_RAND_HEAL3;
						else
							a_ptr->effect = EF_RAND_REGAIN;
					}

					/* Resist nether and hold life. */
					get_quality(FALSE, RES_NETHR, 0, a_idx);
					get_quality(FALSE, HOLD_LIFE, 0, a_idx);
					/* Sometimes also resist chaos. */
					if (randint1(2) == 1)
						get_quality(FALSE, RES_CHAOS, 0, a_idx);
				}

				/* Collect a suite of basic resists. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_COLD, 0, a_idx);
				/* But may stick too closely */
				if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_STICKY_WIELD);
					potential += 500;
				}
			}

			/* ...resistant to sound and confusion. */
			else if (selection < 50) {

				/* Resist sound and confusion. */
				get_quality(FALSE, RES_SOUND, 0, a_idx);
				get_quality(FALSE, RES_CONFU, 0, a_idx);
				/* Collect a suite of basic resists. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_COLD, 0, a_idx);
			}

			/* ...with an amazing armour class. */
			else if (selection < 62) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) == 1) && (potential >= 4000)) {
					a_ptr->effect = EF_RAND_SHIELD;
				}

				/* Increase both base and magical ac. */
				get_quality(TRUE, ADD_AC, randint1(3) + potential / 1600,
							a_idx);
				get_quality(TRUE, IMPROVE_BASE_AC,
							randint1(3) + potential / 1600, a_idx);
				/* Collect a suite of basic resists. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_COLD, 0, a_idx);
				/* Sometimes cost regenarative power */
				if (randint1(6) == 1) {
					cf_on(a_ptr->flags_curse, CF_SLOW_REGEN);
					potential += 100;
				}
			}

			/* ...that grants power over the realms of shadow. */
			else if (selection < 74) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (randint1(2) == 1)
						a_ptr->effect = EF_RAND_TELEPORT1;
					else
						a_ptr->effect = EF_RAND_BLESS;
				}

				/* Resist dark. */
				get_quality(TRUE, RES_DARK, 0, a_idx);
				/* Bonus to stealth. */
				get_quality(TRUE, STEALTH, randint1(4), a_idx);
				/* Grant see invisible. */
				get_quality(TRUE, SEE_INVIS, 0, a_idx);
				/* Possibly resist nether. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_NETHR, 0, a_idx);
				/* Collect a suite of basic resists. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_COLD, 0, a_idx);
			}

			/* ...that protects against poison. */
			else if (selection < 82) {

				/* This is an exclusive club. */
				if (potential >= 2500) {

					/* Possibly assign an activation for free. */
					if (randint1(3) != 1) {
						a_ptr->effect = EF_RAND_CURE;
					}

					/* Resist poison. */
					get_quality(TRUE, RES_POIS, 0, a_idx);
				}

				/* Collect a suite of basic resists. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_COLD, 0, a_idx);
			}

			/* ...that shines very brightly. */
			else if (selection < 95) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential < 4500)
						a_ptr->effect = EF_RAND_LIGHT1;
					else
						a_ptr->effect = EF_RAND_LIGHT2;
				}

				/* Grant permanant light. */
				get_quality(TRUE, LIGHT, 1, a_idx);
				/* And resistance to light. */
				get_quality(TRUE, RES_LIGHT, 0, a_idx);
				/* Collect a suite of basic resists. */
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) < 3)
					get_quality(TRUE, RES_COLD, 0, a_idx);
			}

			/* ...that contains the very essence of... */
			else if (selection < 100) {

				/* This is an exclusive club. */
				if (potential >= 5500) {
					temp = randint1(4);
					/* ...fire. */
					if (temp == 1) {

						/* Assign an activation for free. */
						if (randint1(3) != 1) {
							a_ptr->effect = EF_RAND_FIRE3;
						}

						/* Immunity. */
						get_quality(TRUE, IM_FIRE, 0, a_idx);
						/* Demons like fire */
						if (randint1(4) == 1) {
							cf_on(a_ptr->flags_curse, CF_ATTRACT_DEMON);
							potential += 100;
						}
					}

					/* ...cold. */
					if (temp == 2) {

						/* Assign an activation for free. */
						if (randint1(3) != 1) {
							a_ptr->effect = EF_RAND_COLD3;
						}

						/* Immunity. */
						get_quality(TRUE, IM_COLD, 0, a_idx);
						/* Undead like cold */
						if (randint1(4) == 1) {
							cf_on(a_ptr->flags_curse, CF_ATTRACT_UNDEAD);
							potential += 100;
						}
					}

					/* ...acid. */
					if (temp == 3) {

						/* Assign an activation for free. */
						if (randint1(3) != 1) {
							a_ptr->effect = EF_RAND_ACID3;
						}

						/* Immunity. */
						get_quality(TRUE, IM_ACID, 0, a_idx);
					}

					/* ...electricity. */
					if (temp == 4) {

						/* Assign an activation for free. */
						if (randint1(3) != 1) {
							a_ptr->effect = EF_RAND_ELEC3;
						}

						/* Immunity. */
						get_quality(TRUE, IM_ELEC, 0, a_idx);
					}
				}

				/* Collect a suite of basic resists. */
				if (randint1(2) == 1)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(2) == 1)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(2) == 1)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(2) == 1)
					get_quality(TRUE, RES_COLD, 0, a_idx);
			}

			/* SJGU Hack -- Dragon armour has SPEED */
			if (k_ptr->tval == TV_DRAG_ARMOR) {

				/* Should this be for free? */
				get_quality(FALSE, SPEED, potential / 2500 + randint1(2),
							a_idx);
			}
			break;
		}

		/* I'm a shield... */
	case TV_SHIELD:
		{

			/* ...with a bonus to armour class, and... */
			a_ptr->to_a += (8 + randint1(8) + potential / 1500);
			/* ...that resists most or all of the elements. */
			if (selection < 18) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) == 1) && (potential >= 3000)) {
					if (potential >= 5500)
						a_ptr->effect = EF_RAND_RESIST_ALL;
					else
						a_ptr->effect = EF_RAND_RESIST_ELEMENTS;
				}
				if (randint1(5) != 1)
					get_quality(TRUE, RES_ACID, 0, a_idx);
				if (randint1(5) != 1)
					get_quality(TRUE, RES_ELEC, 0, a_idx);
				if (randint1(5) != 1)
					get_quality(TRUE, RES_FIRE, 0, a_idx);
				if (randint1(5) != 1)
					get_quality(TRUE, RES_COLD, 0, a_idx);
			}

			/* ...that increases strength and constitution. */
			else if (selection < 30) {

				/* Mark the shield for a later bonus to constitution. */
				get_quality(FALSE, ADD_CON, randint1(4), a_idx);
				/* Mark the shield for a later bonus to strength. */
				get_quality(FALSE, ADD_STR, randint1(4), a_idx);
			}

			/* ...that resists shards (+ to ac). */
			else if (selection < 45) {

				/* Resist shards and increase base ac. */
				get_quality(TRUE, RES_SHARD, 0, a_idx);
				get_quality(TRUE, IMPROVE_BASE_AC,
							randint1(3) + potential / 1200, a_idx);
			}

			/* ...that resists light and dark. */
			else if (selection < 55) {

				/* Grant resistance to light and dark. */
				get_quality(TRUE, RES_LIGHT, 0, a_idx);
				get_quality(TRUE, RES_DARK, 0, a_idx);
			}

			/* ...with runes of protection. */
			else if (selection < 85) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) == 1) && (potential >= 4000)) {
					a_ptr->effect = EF_RAND_SHIELD;
				}

				/* Increase magical ac. */
				get_quality(TRUE, ADD_AC, randint1(6) + potential / 2000,
							a_idx);
				/* Randomly add some survival-enhancing magics. */
				if (randint1(4) == 1)
					get_quality(FALSE, REGEN, 0, a_idx);
				if (randint1(4) == 1)
					get_quality(FALSE, FEATHER, 0, a_idx);
				if (randint1(4) == 1)
					get_quality(FALSE, HOLD_LIFE, 0, a_idx);
				if (randint1(4) == 1)
					get_quality(FALSE, SEE_INVIS, 0, a_idx);
				if (randint1(4) == 1)
					get_quality(FALSE, FREE_ACT, 0, a_idx);
				if (randint1(4) == 1)
					get_quality(FALSE, RES_BLIND, 0, a_idx);
				if (randint1(4) == 1)
					get_quality(FALSE, RES_CONFU, 0, a_idx);
			}

			/* ...with immunity. */
			else if (selection < 100) {

				/* This is an exclusive club. */
				if (potential >= 5000) {
					temp = randint1(4);
					/* ...fire. */
					if (temp == 1) {

						/* Immunity. */
						get_quality(TRUE, IM_FIRE, 0, a_idx);
					}

					/* ...cold. */
					if (temp == 2) {

						/* Immunity. */
						get_quality(TRUE, IM_COLD, 0, a_idx);
					}

					/* ...acid. */
					if (temp == 3) {

						/* Immunity. */
						get_quality(TRUE, IM_ACID, 0, a_idx);
					}

					/* ...electricity. */
					if (temp == 4) {

						/* Immunity. */
						get_quality(TRUE, IM_ELEC, 0, a_idx);
					}

					/* Can cost the unwary */
					if (randint1(6) == 1) {
						cf_on(a_ptr->flags_curse, CF_DRAIN_STAT);
						potential += 1000;
					}
				}
			}
			break;
		}

		/* I'm a pair of boots... */
	case TV_BOOTS:
		{

			/* ...with a bonus to armour class, and... */
			a_ptr->to_a += (6 + randint1(5) + potential / 1500);
			/* ...that makes he who wears me run like the wind. */
			if (selection < 25) {

				/* This is an exclusive club. */
				if (potential >= 4500) {

					/* Add a speed bonus immediately. */
					get_quality(TRUE, SPEED, 5 + damroll(3, 2), a_idx);
				}

				/* Speed can be tiring */
				if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_HUNGRY);
					potential += 100;
				}
			}

			/* ...that keeps a guy's feet on the ground. */
			else if (selection < 35) {

				/* Resist nexus and feather fall. */
				get_quality(TRUE, RES_NEXUS, 0, a_idx);
				get_quality(TRUE, FEATHER, 0, a_idx);
				/* Sometimes too much */
				if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_NO_TELEPORT);
					potential += 300;
				}
			}

			/* ...with unrivaled magical movement. */
			else if (selection < 50) {

				/* Assign an activation,... */
				if (potential >= 4000)
					a_ptr->effect = EF_RAND_RECALL;
				else if (potential >= 2000)
					a_ptr->effect = EF_RAND_TELEPORT2;
				else
					a_ptr->effect = EF_RAND_TELEPORT1;
				/* ...but not for free. */
				potential = 2 * potential / 3;
				/* Resist nexus. */
				get_quality(TRUE, RES_NEXUS, 0, a_idx);
				/* Possibly bonus to dexerity. */
				if (potential >= 1000)
					get_quality(FALSE, ADD_DEX, randint1(4), a_idx);
				/* Sometimes too powerful */
				if (randint1(5) == 1) {
					cf_on(a_ptr->flags_curse, CF_TELEPORT);
					potential += 100;
				}
			}

			/* ...that makes long marches easy. */
			else if (selection < 60) {

				/* Possibly assign an activation for free. */
				if (randint1(2) == 1) {
					if (potential >= 5500)
						a_ptr->effect = EF_RAND_HEAL3;
					else if (potential >= 3000)
						a_ptr->effect = EF_RAND_HEAL2;
					else
						a_ptr->effect = EF_RAND_HEAL1;
				}

				/* Grant regenerative powers. */
				get_quality(TRUE, REGEN, 0, a_idx);
				/* Bonus to constitution. */
				get_quality(FALSE, ADD_CON, randint1(4), a_idx);
			}

			/* ...that dance up a storm. */
			else if (selection < 70) {

				/* Possibly assign an activation for free. */
				if ((randint1(3) != 1) && (potential >= 2500))
					a_ptr->effect = EF_RAND_STORM_DANCE;
				/* Grant feather fall. */
				get_quality(TRUE, FEATHER, 0, a_idx);
				/* Bonus to dexterity. */
				get_quality(FALSE, ADD_DEX, randint1(4), a_idx);
			}

			/* ...worn by a famous ranger of old. */
			else if (selection < 80) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1)
					a_ptr->effect = EF_RAND_DETECT_MONSTERS;
				/* Grant regeneration and slow digest. */
				get_quality(TRUE, REGEN, 0, a_idx);
				get_quality(TRUE, SLOW_DIGEST, 0, a_idx);
				/* Possibly telepathy. */
				if (randint1(6) == 1)
					get_quality(FALSE, TELEPATHY, 0, a_idx);
				/* Bonus to stealth. */
				get_quality(FALSE, STEALTH, randint1(4), a_idx);
			}

			/* Possibly assign a speed activation for free. */
			if ((randint1(4) == 1) && (a_ptr->effect == 0)
				&& (potential >= 2000)) {
				a_ptr->effect = EF_RAND_SPEED;
			}
			break;
		}

		/* I'm a cloak... */
	case TV_CLOAK:
		{

			/* ...with a bonus to armour class, and... */
			a_ptr->to_a += (8 + randint1(6) + potential / 1500);
			/* ...that hides the wearer from hostile eyes. */
			if (selection < 20) {

				/* Possibly assign an activation for free. */
				if (randint1(2) == 1)
					a_ptr->effect = EF_RAND_SLEEP_FOES;
				/* Bonus to stealth. */
				get_quality(FALSE, STEALTH, randint1(4), a_idx);
			}

			/* ...that confuses and dismays foes. */
			else if (selection < 30) {

				/* Possibly assign an activation for free. */
				if (randint1(2) == 1) {
					if (randint1(2) == 1)
						a_ptr->effect = EF_RAND_CONFU_FOES;
					else
						a_ptr->effect = EF_RAND_TURN_FOES;
				}
			}

			/* ...with amazing protective powers. */
			else if (selection < 40) {

				/* Possibly assign an activation for free. */
				if ((randint1(4) == 1) && (potential >= 3000)) {
					a_ptr->effect = EF_RAND_SHIELD;
				}

				/* Increase both base and magical ac. */
				get_quality(TRUE, ADD_AC, randint1(3) + potential / 1600,
							a_idx);
				get_quality(TRUE, IMPROVE_BASE_AC,
							randint1(3) + potential / 1600, a_idx);
			}

			/* ...that unmasks the locations of foes. */
			else if (selection < 50) {

				/* Assign an activation for free. */
				if (randint1(2) == 1)
					a_ptr->effect = EF_RAND_DETECT_MONSTERS;
				else
					a_ptr->effect = EF_RAND_DETECT_EVIL;
			}

			/* ...that is aware of the world around. */
			else if (selection < 60) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if ((potential >= 4000) && (randint1(3) != 1))
						a_ptr->effect = EF_RAND_DETECT_ALL;
					else if (potential >= 3000)
						a_ptr->effect = EF_RAND_MAGIC_MAP;
					else
						a_ptr->effect = EF_RAND_DETECT_D_S_T;
				}

				/* Bonus to searching. */
				get_quality(FALSE, SEARCH, randint1(4), a_idx);
			}

			/* ...of necromantic powers. */
			else if (selection < 65) {

				/* Possibly assign an activation for free. */
				if (randint1(2) == 1)
					a_ptr->effect = EF_RAND_SLOW_FOES;
				/* Resist chaos, dark, or nether */
				temp = randint1(3);
				if (temp == 1)
					get_quality(FALSE, RES_CHAOS, 0, a_idx);
				else if (temp == 2)
					get_quality(FALSE, RES_NETHR, 0, a_idx);
				else if (temp == 3)
					get_quality(FALSE, RES_DARK, 0, a_idx);
				/* Bonus to infravision. */
				get_quality(FALSE, INFRA, randint1(4), a_idx);
				/* Costs the device user */
				if (randint1(6) == 1) {
					cf_on(a_ptr->flags_curse, CF_DRAIN_CHARGE);
					potential += 800;
				}
			}

			/* Hack -- Elven Cloaks have STEALTH */
			if (k_ptr->sval == SV_ELVEN_CLOAK) {

				/* Should this be for free? */
				a_ptr->bonus_other[P_BONUS_STEALTH] +=
					potential / 2500 + randint1(2);
			}
			break;
		}

		/* I'm a helm or crown... */
	case TV_HELM:
	case TV_CROWN:
		{

			/* ...with a bonus to armour class, and... */
			a_ptr->to_a += (8 + randint1(6) + potential / 1500);
			/* ...of telepathy. */
			if (selection < 24) {

				/* This is an exclusive club. */
				if (potential < 3500)
					break;
				/* Grant telepathy. */
				get_quality(FALSE, TELEPATHY, 0, a_idx);
			}

			/* ...that maintains serenity amid uncertainly. */
			else if (selection < 35) {

				/* Possibly assign an activation for free. */
				if ((randint1(2) == 1) && (potential >= 4500))
					a_ptr->effect = EF_RAND_CURE;
				/* Grant resistance to confusion and sound. */
				get_quality(FALSE, RES_CONFU, 0, a_idx);
				get_quality(FALSE, RES_SOUND, 0, a_idx);
			}

			/* ...preservative of sight and awareness. */
			else if (selection < 46) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 3750)
						a_ptr->effect = EF_RAND_DETECT_ALL;
					else
						a_ptr->effect = EF_RAND_DETECT_D_S_T;
				}

				/* Grant resistance to blindness and see invisible. */
				get_quality(FALSE, RES_BLIND, 0, a_idx);
				get_quality(FALSE, SEE_INVIS, 0, a_idx);
				/* Bonus to searching. */
				get_quality(FALSE, SEARCH, randint1(4), a_idx);
			}

			/* ...whose wearer stands taller in the eyes of men. */
			else if (selection < 57) {

				/* Bonus to wisdom. */
				get_quality(FALSE, ADD_WIS, randint1(4), a_idx);
				/* Bonus to charisma. */
				get_quality(FALSE, ADD_CHR, randint1(4), a_idx);
				/* Possibly add a bonus to base ac. */
				if (randint1(3) == 1)
					get_quality(FALSE, IMPROVE_BASE_AC, randint1(2) + 2,
								a_idx);
			}

			/* ...strong in battle. */
			else if (selection < 68) {

				/* Possibly assign an activation for free. */
				if ((potential >= 2500) && (randint1(3) != 1))
					a_ptr->effect = EF_RAND_FRIGHTEN_ALL;
				else
					a_ptr->effect = EF_RAND_HEROISM;
				/* No fear. */
				get_quality(TRUE, RES_FEAR, 0, a_idx);
				/* Bonus to strength. */
				get_quality(FALSE, ADD_STR, randint1(4), a_idx);
				/* Bonus to dexterity. */
				get_quality(FALSE, ADD_DEX, randint1(4), a_idx);
				/* Sustain dexterity and strength. */
				get_quality(TRUE, SUST_DEX, 0, a_idx);
				get_quality(TRUE, SUST_STR, 0, a_idx);
			}

			/* ...to whose wearer is revealed many secrets. */
			else if (selection < 79) {

				/* This is an exclusive club. */
				if (potential < 2500)
					break;
				/* Assign an activation for free. */
				a_ptr->effect = EF_RAND_IDENTIFY;
			}

			/* ...which can focus healing magics. */
			else if (selection < 90) {

				/* Possibly assign an activation for free. */
				if (randint1(3) != 1) {
					if (potential >= 4000)
						a_ptr->effect = EF_RAND_HEAL3;
					else if (potential >= 2500)
						a_ptr->effect = EF_RAND_HEAL2;
					else
						a_ptr->effect = EF_RAND_HEAL1;
				}

				/* Grant regeneration. */
				get_quality(TRUE, REGEN, 0, a_idx);
			}
			break;
		}

		/* I'm a pair of gloves... */
	case TV_GLOVES:
		{

			/* ...with a bonus to armour class, and... */
			a_ptr->to_a += (7 + randint1(5) + potential / 1500);
			/* ...that grant increased combat prowess. */
			if (selection < 30) {

				/* Grant equal bonuses to Skill and Deadliness. */
				temp = 6 + randint1(4);
				get_quality(TRUE, ADD_DEADLINESS, temp, a_idx);
				get_quality(TRUE, ADD_SKILL, temp, a_idx);
				/* Often, acquire free action. */
				if (randint1(3) != 1)
					get_quality(TRUE, FREE_ACT, 0, a_idx);
				/* Sometimes vulnerable to confusion */
				if (randint1(6) == 1)
					get_quality(FALSE, VUL_CONFU, 0, a_idx);
			}

			/* ...with the dauntless spirit of a mighty warrior. */
			else if (selection < 45) {

				/* No fear. */
				get_quality(TRUE, RES_FEAR, 0, a_idx);
				/* Often, grant regeneration. */
				if (randint1(3) != 1)
					get_quality(FALSE, REGEN, 0, a_idx);
				/* Sometimes, acquire free action. */
				if (randint1(2) == 1)
					get_quality(FALSE, FREE_ACT, 0, a_idx);
				/* Bonus to one of the combat stats. */
				if (randint1(3) == 1)
					get_quality(FALSE, ADD_STR, randint1(4), a_idx);
				else if (randint1(2) == 1)
					get_quality(FALSE, ADD_DEX, randint1(4), a_idx);
				else
					get_quality(FALSE, ADD_CON, randint1(4), a_idx);
				/* Possibly grant equal bonuses to Skill and Deadliness. */
				if ((potential >= 1500) && (randint1(4) != 1)) {
					temp = 4 + randint1(4);
					get_quality(FALSE, ADD_DEADLINESS, temp, a_idx);
					get_quality(FALSE, ADD_SKILL, temp, a_idx);
				}

				/* Sometimes vulnerable to poison */
				if (randint1(6) == 1)
					get_quality(FALSE, VUL_POIS, 0, a_idx);
			}

			/* ...able to protect the wearer. */
			else if (selection < 60) {

				/* Increase bonus to AC. */
				get_quality(FALSE, ADD_AC, 4 + randint1(4), a_idx);
				/* Grant (usually) two basic resists. */
				for (i = 0; i < 2; i++) {
					temp = randint1(4);
					if (temp == 1)
						get_quality(FALSE, RES_FIRE, 0, a_idx);
					if (temp == 2)
						get_quality(FALSE, RES_COLD, 0, a_idx);
					if (temp == 3)
						get_quality(FALSE, RES_ACID, 0, a_idx);
					if (temp == 4)
						get_quality(FALSE, RES_ELEC, 0, a_idx);
				}

				/* Sometimes add a bolt activation for free. */
				if ((potential >= 500) && (randint1(3) != 1)) {
					if (temp == 1)
						a_ptr->effect = EF_RAND_FIRE1;
					if (temp == 2)
						a_ptr->effect = EF_RAND_COLD1;
					if (temp == 3)
						a_ptr->effect = EF_RAND_ACID1;
					if (temp == 4)
						a_ptr->effect = EF_RAND_ELEC1;
				}
			}

			/* ...with the cunning of a rogue of legend. */
			else if (selection < 75) {

				/* Assign an activation for free. */
				a_ptr->effect = EF_RAND_DISARM;
				/* Bonus to stealth. */
				get_quality(FALSE, STEALTH, randint1(4), a_idx);
				/* Bonus to searching. */
				get_quality(FALSE, SEARCH, randint1(4), a_idx);
				/* Often, acquire free action. */
				if (randint1(3) != 1)
					get_quality(FALSE, FREE_ACT, 0, a_idx);
				/* Sometimes vulnerable to sound */
				if (randint1(5) == 1)
					get_quality(FALSE, VUL_SOUND, 0, a_idx);
			}

			/* ...that untangles magical conundrums. */
			else if (selection < 90) {

				/* Mark the gloves for a later bonus to magic item mastery. */
				get_quality(FALSE, MAGIC_MASTERY, randint1(4), a_idx);
			}

			/* ...with a deadly mastery of archery. */
			else if (selection < 100) {

				/* This is an exclusive club. */
				if (potential < 3500)
					break;
				/* Always grant an activation, but not for free. */
				a_ptr->effect = EF_RAND_SUPER_SHOOTING;
				potential -= 750;
				/* Add equal bonuses to Skill and to Deadliness. */
				temp = 5 + randint1(5);
				get_quality(FALSE, ADD_DEADLINESS, temp, a_idx);
				get_quality(FALSE, ADD_SKILL, temp, a_idx);
				/* Often, acquire free action. */
				if (randint1(3) != 1)
					get_quality(FALSE, FREE_ACT, 0, a_idx);
			}
			break;
		}
	default:
		{
			msg("error -- object kind not recognized.");
			break;
		}
	}

	/* It is possible for artifacts to be unusually heavy or light. */
	if (randint1(6) == 1) {
		old_weight = a_ptr->weight;
		/* Sometimes, they are unusually heavy. */
		if ((randint1(2) == 1) && (a_ptr->weight >= 30)) {
			a_ptr->weight += randint1(a_ptr->weight / 10) * 5 + 5;
			potential += (a_ptr->weight - old_weight) * 10;
		}

		/* Sometimes, they are unusually light. */
		else if (a_ptr->weight >= 50) {
			a_ptr->weight -= randint1(a_ptr->weight / 10) * 5 + 5;
			potential -= (old_weight - a_ptr->weight) * 10;
			if (potential < 0)
				potential = 0;
		}
	}
}


/**
 * Grant extra abilities, until object's units of exchange are all used up.  
 * This function can be quite random - indeed needs to be - because of all 
 * the possible themed random artifacts.
 */
static void haggle_till_done(int a_idx)
{
	artifact_type *a_ptr = &a_info[a_idx];
	object_kind *k_ptr = &k_info[lookup_kind(a_ptr->tval, a_ptr->sval)];
	int i, rounds;
	int pval = 0;
	int choice = 0;
	int limit = 20;
	/* Rarely, and only if the artifact has a lot of potential, add all the
	 * stats. */
	if ((randint1(6) == 1) && (potential >= 4500)) {
		get_quality(TRUE, ADD_STR, 1, a_idx);
		get_quality(TRUE, ADD_WIS, 1, a_idx);
		get_quality(TRUE, ADD_INT, 1, a_idx);
		get_quality(TRUE, ADD_DEX, 1, a_idx);
		get_quality(TRUE, ADD_CON, 1, a_idx);
		get_quality(TRUE, ADD_CHR, 1, a_idx);
	}

	/* Otherwise, if at least some potential remains, add a pval-dependant
	 * quality or two with 67% probability. */
	else if ((randint1(3) != 1) && (potential >= 750)) {

		/* Add the pval. */
		pval = potential / 2000 + randint1(2);
		/* Determine number of loops. */
		rounds = randint1(2);
		for (i = 0; i < rounds; i++) {

			/* Only melee weapons can get a bonus to tunnelling. */
			if ((a_ptr->tval == TV_SWORD) || (a_ptr->tval == TV_POLEARM)
				|| (a_ptr->tval == TV_HAFTED))
				choice = randint1(11);
			else
				choice = randint1(10);
			if (choice == 1)
				get_quality(TRUE, ADD_STR, pval, a_idx);
			if (choice == 2)
				get_quality(TRUE, ADD_WIS, pval, a_idx);
			if (choice == 3)
				get_quality(TRUE, ADD_INT, pval, a_idx);
			if (choice == 4)
				get_quality(TRUE, ADD_DEX, pval, a_idx);
			if (choice == 5)
				get_quality(TRUE, ADD_CON, pval, a_idx);
			if (choice == 6)
				get_quality(TRUE, ADD_CHR, pval, a_idx);
			if (choice == 7)
				get_quality(TRUE, INFRA, pval, a_idx);
			if (choice == 8)
				get_quality(TRUE, STEALTH, pval, a_idx);
			if (choice == 9)
				get_quality(TRUE, SEARCH, pval, a_idx);
			if (choice == 10)
				get_quality(TRUE, SPEED, pval, a_idx);
			if (choice == 11)
				get_quality(TRUE, TUNNEL, pval, a_idx);
			choice = 0;
		}
	}

	/* Sometimes, collect a vulnerability in exchange for more potential */
	if (randint1(8) == 1) {
		choice = randint1(14);
		if ((choice == 1)
			&& (a_ptr->percent_res[P_RES_FIRE] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_FIRE, 0, a_idx);
		if ((choice == 2)
			&& (a_ptr->percent_res[P_RES_COLD] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_COLD, 0, a_idx);
		if ((choice == 3)
			&& (a_ptr->percent_res[P_RES_ACID] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_ACID, 0, a_idx);
		if ((choice == 4)
			&& (a_ptr->percent_res[P_RES_ELEC] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_ELEC, 0, a_idx);
		if ((choice == 5)
			&& (a_ptr->percent_res[P_RES_POIS] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_POIS, 0, a_idx);
		if ((choice == 6)
			&& (a_ptr->percent_res[P_RES_LIGHT] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_LIGHT, 0, a_idx);
		if ((choice == 7)
			&& (a_ptr->percent_res[P_RES_DARK] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_DARK, 0, a_idx);
		if ((choice == 8)
			&& (a_ptr->percent_res[P_RES_CONFU] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_CONFU, 0, a_idx);
		if ((choice == 9)
			&& (a_ptr->percent_res[P_RES_SOUND] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_SOUND, 0, a_idx);
		if ((choice == 10)
			&& (a_ptr->percent_res[P_RES_SHARD] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_SHARD, 0, a_idx);
		if ((choice == 11)
			&& (a_ptr->percent_res[P_RES_NEXUS] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_NEXUS, 0, a_idx);
		if ((choice == 12)
			&& (a_ptr->percent_res[P_RES_NETHR] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_NETHR, 0, a_idx);
		if ((choice == 13)
			&& (a_ptr->percent_res[P_RES_CHAOS] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_CHAOS, 0, a_idx);
		if ((choice == 14)
			&& (a_ptr->percent_res[P_RES_DISEN] == RES_LEVEL_BASE))
			get_quality(TRUE, VUL_DISEN, 0, a_idx);
	}

	/* Artifacts that still have lots of money to spend deserve the best. */
	if ((potential > 2000) && (randint1(3) != 1)) {

		/* Make a choice... */
		choice = randint1(5);
		/* ...among some tasty options. */
		if ((choice == 1) && (!(of_has(a_ptr->flags_obj, OF_TELEPATHY)))) {
			get_quality(TRUE, TELEPATHY, 0, a_idx);
		}
		if ((choice == 2) && (!(of_has(a_ptr->flags_obj, OF_HOLD_LIFE)))) {
			get_quality(TRUE, HOLD_LIFE, 0, a_idx);
		}
		if (choice == 3) {
			if (a_ptr->percent_res[P_RES_CONFU] >= RES_LEVEL_BASE)
				get_quality(TRUE, RES_CONFU, 0, a_idx);
			if (!of_has(a_ptr->flags_obj, OF_SEEING))
				get_quality(TRUE, RES_BLIND, 0, a_idx);
		}
		if (choice == 4) {
			if ((randint1(4) == 1)
				&& (a_ptr->percent_res[P_RES_DISEN] >= RES_LEVEL_BASE))
				get_quality(TRUE, RES_DISEN, 0, a_idx);
			else if ((randint1(3) == 1)
					 && (a_ptr->percent_res[P_RES_NETHR] >=
						 RES_LEVEL_BASE))
				get_quality(TRUE, RES_NETHR, 0, a_idx);
			else if ((randint1(2) == 1)
					 && (a_ptr->percent_res[P_RES_CHAOS] >=
						 RES_LEVEL_BASE))
				get_quality(TRUE, RES_CHAOS, 0, a_idx);
			else if (a_ptr->percent_res[P_RES_POIS] >= RES_LEVEL_BASE)
				get_quality(TRUE, RES_POIS, 0, a_idx);
		}
		if ((choice == 5) && (potential > 4500)) {
			if ((randint1(2) == 1)
				&& (a_ptr->bonus_other[P_BONUS_SPEED] == 0)) {
				get_quality(TRUE, SPEED, randint1(10), a_idx);
			}

			else {
				get_quality(TRUE, SUST_STR, pval, a_idx);
				get_quality(TRUE, SUST_WIS, pval, a_idx);
				get_quality(TRUE, SUST_INT, pval, a_idx);
				get_quality(TRUE, SUST_DEX, pval, a_idx);
				get_quality(TRUE, SUST_CON, pval, a_idx);
				get_quality(TRUE, SUST_CHR, pval, a_idx);
			}
		}
	}

	/* Sometimes, also add a new pval-dependant quality. Infravision and
	 * magical item mastery are not on offer. Only melee weapons can get a
	 * bonus to tunnelling. */
	if (((randint1(5) < 3) || (potential >= 2750))
		&& ((pval) && (pval < 7))) {
		pval = randint1(3);
		if ((a_ptr->tval == TV_SWORD) || (a_ptr->tval == TV_POLEARM)
			|| (a_ptr->tval == TV_HAFTED))
			choice = randint1(10);
		else
			choice = randint1(9);
		if (choice == 1)
			get_quality(TRUE, ADD_STR, pval, a_idx);
		if (choice == 2)
			get_quality(TRUE, ADD_WIS, pval, a_idx);
		if (choice == 3)
			get_quality(TRUE, ADD_INT, pval, a_idx);
		if (choice == 4)
			get_quality(TRUE, ADD_DEX, pval, a_idx);
		if (choice == 5)
			get_quality(TRUE, ADD_CON, pval, a_idx);
		if (choice == 6)
			get_quality(TRUE, ADD_CHR, pval, a_idx);
		if (choice == 7)
			get_quality(TRUE, STEALTH, pval, a_idx);
		if (choice == 8)
			get_quality(TRUE, SEARCH, pval, a_idx);
		if (choice == 9)
			get_quality(TRUE, SPEED, pval, a_idx);
		if (choice == 10)
			get_quality(TRUE, TUNNEL, pval, a_idx);
		choice = 0;
	}

	/* Now, we enter Filene's Basement, and shop 'til we drop! (well, nearly) */
	while ((potential >= 300) && (limit-- > 0)) {

		/* I'm a melee weapon. */
		if ((a_ptr->tval == TV_SWORD) || (a_ptr->tval == TV_POLEARM)
			|| (a_ptr->tval == TV_HAFTED)) {

			/* Some throwing weapons can be thrown hard and fast. */
			if ((of_has(a_ptr->flags_obj, OF_THROWING))
				&& (randint1(2) == 1))
				get_quality(TRUE, PERFECT_BALANCE, 0, a_idx);
			/* Some weapons already will have superb base damage. */
			if ((a_ptr->dd > k_ptr->dd) || (a_ptr->ds > k_ptr->ds)) {

				/* Sometimes, such weapons are unusual. */
				if ((randint1(3) == 1) && (potential >= 1500)) {
					if (a_ptr->weight == k_ptr->weight) {

						/* Sometimes, such weapons are unusually heavy. */
						if (randint1(3) == 1) {
							a_ptr->weight +=
								randint1(a_ptr->weight / 10) * 5 + 5;
							potential +=
								(a_ptr->weight - k_ptr->weight) * 10;
						}

						/* Sometimes, such weapons are unusually light. */
						else if ((randint1(2) == 1)
								 && (a_ptr->weight >= 150)) {
							a_ptr->weight -=
								randint1(a_ptr->weight / 10) * 5 + 5;
							potential -=
								(k_ptr->weight - a_ptr->weight) * 10;
						}
					}

					/* Sometimes spend everything to enhance the damage dice.
					 * SJGU assuming there wasn't too much to start with... */
					if ((randint1(3) == 1) && (initial_potential <= 4000)) {

						/* Probably sacrifice the Skill bonus. */
						if (randint1(3) != 1) {
							a_ptr->to_h = 0;
							potential += 600;
							/* Possibly also sacrifice the Deadliness bonus. */
							if (randint1(4) == 1) {
								a_ptr->to_h = 0;
								potential += 600;
							}
						}
						get_quality(TRUE, ENHANCE_DICE, 1, a_idx);
						/* We're done */
						break;
					}
				}
			}

			/* Other weapons have enhanced damage dice in addition to other
			 * qualities.  - SJGU increased chance of this as small dice are
			 * useless */
			else if ((randint1(a_ptr->dd * a_ptr->ds) < 5)
					 || (potential >= 2100)) {
				get_quality(TRUE, ENHANCE_DICE, 3, a_idx);
			}

			/* Collect a slay or brand. */
			choice = randint1(13);
			if ((choice == 1)
				&& (a_ptr->multiple_slay[P_SLAY_ANIMAL] == MULTIPLE_BASE))
				get_quality(TRUE, SLAY_ANIMAL, 7, a_idx);
			if ((choice == 2)
				&& (a_ptr->multiple_slay[P_SLAY_EVIL]
					== MULTIPLE_BASE))
				get_quality(TRUE, SLAY_EVIL, 5, a_idx);
			if ((choice == 3)
				&& (a_ptr->multiple_slay[P_SLAY_UNDEAD]
					== MULTIPLE_BASE))
				get_quality(TRUE, SLAY_UNDEAD, 10, a_idx);
			if ((choice == 4)
				&& (a_ptr->multiple_slay[P_SLAY_DEMON]
					== MULTIPLE_BASE))
				get_quality(TRUE, SLAY_DEMON, 10, a_idx);
			if ((choice == 5)
				&& (a_ptr->multiple_slay[P_SLAY_ORC] == MULTIPLE_BASE))
				get_quality(TRUE, SLAY_ORC, 10, a_idx);
			if ((choice == 6)
				&& (a_ptr->multiple_slay[P_SLAY_TROLL] == MULTIPLE_BASE))
				get_quality(TRUE, SLAY_TROLL, 10, a_idx);
			if ((choice == 7)
				&& (a_ptr->multiple_slay[P_SLAY_GIANT]
					== MULTIPLE_BASE))
				get_quality(TRUE, SLAY_GIANT, 10, a_idx);
			if ((choice == 8)
				&& (a_ptr->multiple_slay[P_SLAY_DRAGON]
					== MULTIPLE_BASE))
				get_quality(TRUE, SLAY_DRAGON, 10, a_idx);
			if ((choice == 9)
				&& (a_ptr->multiple_brand[P_BRAND_FIRE]
					== MULTIPLE_BASE))
				get_quality(TRUE, BRAND_FIRE, 7, a_idx);
			if ((choice == 10)
				&& (a_ptr->multiple_brand[P_BRAND_COLD]
					== MULTIPLE_BASE))
				get_quality(TRUE, BRAND_COLD, 7, a_idx);
			if ((choice == 11)
				&& (a_ptr->multiple_brand[P_BRAND_ACID]
					== MULTIPLE_BASE))
				get_quality(TRUE, BRAND_ACID, 7, a_idx);
			if ((choice == 12)
				&& (a_ptr->multiple_brand[P_BRAND_ELEC]
					== MULTIPLE_BASE))
				get_quality(TRUE, BRAND_ELEC, 7, a_idx);
			if ((choice == 13)
				&& (a_ptr->multiple_brand[P_BRAND_POIS]
					== MULTIPLE_BASE))
				get_quality(TRUE, BRAND_POIS, 7, a_idx);
			/* Often, collect a miscellanious quality, if it is affordable. */
			if (randint1(2) == 1) {
				choice = randint1(8);
				if ((choice == 1)
					&& (!(of_has(a_ptr->flags_obj, OF_SLOW_DIGEST))))
					get_quality(FALSE, SLOW_DIGEST, 0, a_idx);
				if ((choice == 2)
					&& (!(of_has(a_ptr->flags_obj, OF_FEATHER))))
					get_quality(FALSE, FEATHER, 0, a_idx);
				if ((choice == 3)
					&& (!(of_has(a_ptr->flags_obj, OF_LIGHT))))
					get_quality(FALSE, LIGHT, 1, a_idx);
				if ((choice == 4)
					&& (!(of_has(a_ptr->flags_obj, OF_REGEN))))
					get_quality(FALSE, REGEN, 0, a_idx);
				if ((choice == 5)
					&& (!(of_has(a_ptr->flags_obj, OF_SEE_INVIS))))
					get_quality(FALSE, SEE_INVIS, 0, a_idx);
				if ((choice == 6)
					&& (!(of_has(a_ptr->flags_obj, OF_FREE_ACT))))
					get_quality(FALSE, FREE_ACT, 0, a_idx);
				if ((choice == 7)
					&& (!(of_has(a_ptr->flags_obj, OF_FEARLESS))))
					get_quality(FALSE, RES_FEAR, 0, a_idx);
				if ((choice == 8)
					&& (!(of_has(a_ptr->flags_obj, OF_SEEING))))
					get_quality(FALSE, RES_BLIND, 0, a_idx);
			}

			/* Sometimes, collect a resistance, if it is affordable. */
			if (randint1(3) == 1) {
				choice = randint1(11);
				if ((choice == 1)
					&&
					(!(a_ptr->percent_res[P_RES_FIRE] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_FIRE, 0, a_idx);
				if ((choice == 2)
					&&
					(!(a_ptr->percent_res[P_RES_COLD] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_COLD, 0, a_idx);
				if ((choice == 3)
					&&
					(!(a_ptr->percent_res[P_RES_ACID] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_ACID, 0, a_idx);
				if ((choice == 4)
					&&
					(!(a_ptr->percent_res[P_RES_ELEC] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_ELEC, 0, a_idx);
				if ((choice == 5)
					&&
					(!(a_ptr->percent_res[P_RES_LIGHT] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_LIGHT, 0, a_idx);
				if ((choice == 6)
					&&
					(!(a_ptr->percent_res[P_RES_DARK] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_DARK, 0, a_idx);
				if ((choice == 7)
					&&
					(!(a_ptr->percent_res[P_RES_CONFU] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_CONFU, 0, a_idx);
				if ((choice == 8)
					&& (!(a_ptr->percent_res[P_RES_SOUND]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_SOUND, 0, a_idx);
				if ((choice == 9)
					&& (!(a_ptr->percent_res[P_RES_SHARD]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_SHARD, 0, a_idx);
				if ((choice == 10)
					&& (!(a_ptr->percent_res[P_RES_NEXUS]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_NEXUS, 0, a_idx);
				if ((choice == 11)
					&& (!(a_ptr->percent_res[P_RES_DISEN]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_DISEN, 0, a_idx);
			}

			/* Clean out the wallet. */
			if ((potential < 500) && (randint1(5) == 1)) {
				if (a_ptr->to_d > 0)
					a_ptr->to_d += potential / 150;
				if (a_ptr->to_h > 0)
					a_ptr->to_h += potential / 150;
				potential = 0;
			}
		}

		/* I'm a missile weapon. */
		if (a_ptr->tval == TV_BOW) {
			choice = randint1(8);
			/* Collect a miscellanious quality. */
			if ((choice == 1)
				&& (!(of_has(a_ptr->flags_obj, OF_SLOW_DIGEST))))
				get_quality(FALSE, SLOW_DIGEST, 0, a_idx);
			if ((choice == 2) && (!(of_has(a_ptr->flags_obj, OF_FEATHER))))
				get_quality(FALSE, FEATHER, 0, a_idx);
			if ((choice == 3) && (!(of_has(a_ptr->flags_obj, OF_LIGHT))))
				get_quality(FALSE, LIGHT, 1, a_idx);
			if ((choice == 4) && (!(of_has(a_ptr->flags_obj, OF_REGEN))))
				get_quality(FALSE, REGEN, 0, a_idx);
			if ((choice == 5)
				&& (!(of_has(a_ptr->flags_obj, OF_SEE_INVIS))))
				get_quality(FALSE, SEE_INVIS, 0, a_idx);
			if ((choice == 6)
				&& (!(of_has(a_ptr->flags_obj, OF_FREE_ACT))))
				get_quality(FALSE, FREE_ACT, 0, a_idx);
			if ((choice == 7)
				&& (!(of_has(a_ptr->flags_obj, OF_FEARLESS))))
				get_quality(FALSE, RES_FEAR, 0, a_idx);
			if ((choice == 8) && (!(of_has(a_ptr->flags_obj, OF_SEEING))))
				get_quality(FALSE, RES_BLIND, 0, a_idx);
			/* Sometimes, collect a resistance, if it is affordable. */
			if (randint1(2) == 1) {
				choice = randint1(11);
				if ((choice == 1)
					&&
					(!(a_ptr->percent_res[P_RES_FIRE] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_FIRE, 0, a_idx);
				if ((choice == 2)
					&&
					(!(a_ptr->percent_res[P_RES_COLD] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_COLD, 0, a_idx);
				if ((choice == 3)
					&&
					(!(a_ptr->percent_res[P_RES_ACID] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_ACID, 0, a_idx);
				if ((choice == 4)
					&&
					(!(a_ptr->percent_res[P_RES_ELEC] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_ELEC, 0, a_idx);
				if ((choice == 5)
					&&
					(!(a_ptr->percent_res[P_RES_LIGHT] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_LIGHT, 0, a_idx);
				if ((choice == 6)
					&&
					(!(a_ptr->percent_res[P_RES_DARK] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_DARK, 0, a_idx);
				if ((choice == 7)
					&&
					(!(a_ptr->percent_res[P_RES_CONFU] == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_CONFU, 0, a_idx);
				if ((choice == 8)
					&& (!(a_ptr->percent_res[P_RES_SOUND]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_SOUND, 0, a_idx);
				if ((choice == 9)
					&& (!(a_ptr->percent_res[P_RES_SHARD]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_SHARD, 0, a_idx);
				if ((choice == 10)
					&& (!(a_ptr->percent_res[P_RES_NEXUS]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_NEXUS, 0, a_idx);
				if ((choice == 11)
					&& (!(a_ptr->percent_res[P_RES_DISEN]
						  == RES_LEVEL_BASE)))
					get_quality(FALSE, RES_DISEN, 0, a_idx);
			}

			/* Clean out the wallet. */
			if ((potential < 500) && (randint1(5) == 1)) {
				if (a_ptr->to_d > 0)
					a_ptr->to_d += potential / 150;
				if (a_ptr->to_h > 0)
					a_ptr->to_h += potential / 150;
				potential = 0;
			}
		}

		/* I'm any piece of armour or jewellery. */
		else {

			/* Collect a resistance. */
			choice = randint1(11);
			if ((choice == 1)
				&& (!(a_ptr->percent_res[P_RES_FIRE] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_FIRE, 0, a_idx);
			if ((choice == 2)
				&& (!(a_ptr->percent_res[P_RES_COLD] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_COLD, 0, a_idx);
			if ((choice == 3)
				&& (!(a_ptr->percent_res[P_RES_ACID] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_ACID, 0, a_idx);
			if ((choice == 4)
				&& (!(a_ptr->percent_res[P_RES_ELEC] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_ELEC, 0, a_idx);
			if ((choice == 5)
				&& (!(a_ptr->percent_res[P_RES_LIGHT] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_LIGHT, 0, a_idx);
			if ((choice == 6)
				&& (!(a_ptr->percent_res[P_RES_DARK] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_DARK, 0, a_idx);
			if ((choice == 7)
				&& (!(a_ptr->percent_res[P_RES_CONFU] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_CONFU, 0, a_idx);
			if ((choice == 8)
				&& (!(a_ptr->percent_res[P_RES_SOUND] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_SOUND, 0, a_idx);
			if ((choice == 9)
				&& (!(a_ptr->percent_res[P_RES_SHARD] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_SHARD, 0, a_idx);
			if ((choice == 10)
				&& (!(a_ptr->percent_res[P_RES_NEXUS] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_NEXUS, 0, a_idx);
			if ((choice == 11)
				&& (!(a_ptr->percent_res[P_RES_DISEN] == RES_LEVEL_BASE)))
				get_quality(TRUE, RES_DISEN, 0, a_idx);
			/* Sometimes, collect a miscellaneous quality, if it is affordable. 
			 */
			if (randint1(3) == 1) {
				choice = randint1(8);
				/* Collect a miscellanious quality. */
				if ((choice == 1)
					&& (!(of_has(a_ptr->flags_obj, OF_SLOW_DIGEST))))
					get_quality(FALSE, SLOW_DIGEST, 0, a_idx);
				if ((choice == 2)
					&& (!(of_has(a_ptr->flags_obj, OF_FEATHER))))
					get_quality(FALSE, FEATHER, 0, a_idx);
				if ((choice == 3)
					&& (!(of_has(a_ptr->flags_obj, OF_LIGHT))))
					get_quality(FALSE, LIGHT, 1, a_idx);
				if ((choice == 4)
					&& (!(of_has(a_ptr->flags_obj, OF_REGEN))))
					get_quality(FALSE, REGEN, 0, a_idx);
				if ((choice == 5)
					&& (!(of_has(a_ptr->flags_obj, OF_SEE_INVIS))))
					get_quality(FALSE, SEE_INVIS, 0, a_idx);
				if ((choice == 6)
					&& (!(of_has(a_ptr->flags_obj, OF_FREE_ACT))))
					get_quality(FALSE, FREE_ACT, 0, a_idx);
				if ((choice == 7)
					&& (!(of_has(a_ptr->flags_obj, OF_FEARLESS))))
					get_quality(FALSE, RES_FEAR, 0, a_idx);
				if ((choice == 8)
					&& (!(of_has(a_ptr->flags_obj, OF_SEEING))))
					get_quality(FALSE, RES_BLIND, 0, a_idx);
			}

			/* Clean out the wallet. */
			if ((potential < 500) && (randint1(5) == 1)) {
				if (a_ptr->to_a > 0)
					a_ptr->to_a += potential / 100;
				potential = 0;
			}
		}
	}

	/* On sale free! Last chance! */

	/* If an artifact affects a stat, it may also possibly sustain it. */
	choice = randint1(6);
	if ((choice == 1) && (a_ptr->bonus_stat[A_STR] > 0))
		of_on(a_ptr->flags_obj, OF_SUSTAIN_STR);
	else if ((choice == 2) && (a_ptr->bonus_stat[A_WIS] > 0))
		of_on(a_ptr->flags_obj, OF_SUSTAIN_WIS);
	else if ((choice == 3) && (a_ptr->bonus_stat[A_INT] > 0))
		of_on(a_ptr->flags_obj, OF_SUSTAIN_INT);
	else if ((choice == 4) && (a_ptr->bonus_stat[A_DEX] > 0))
		of_on(a_ptr->flags_obj, OF_SUSTAIN_DEX);
	else if ((choice == 5) && (a_ptr->bonus_stat[A_CON] > 0))
		of_on(a_ptr->flags_obj, OF_SUSTAIN_CON);
	else if ((choice == 6) && (a_ptr->bonus_stat[A_CHR] > 0))
		of_on(a_ptr->flags_obj, OF_SUSTAIN_CHR);
	/* Armour gets a basic resist or low-level ability with 50% probability,
	 * and weapons with 25% probability. */
	if (((a_ptr->tval >= TV_BOOTS) && (randint1(2) == 1))
		|| (randint1(4) == 1)) {
		choice = randint1(6);
		if (choice == 1)
			a_ptr->percent_res[P_RES_FIRE] /= 2;
		else if (choice == 2)
			a_ptr->percent_res[P_RES_COLD] /= 2;
		else if (choice == 3)
			a_ptr->percent_res[P_RES_ACID] /= 2;
		else if (choice == 4)
			a_ptr->percent_res[P_RES_ELEC] /= 2;
		else if (choice == 5)
			of_on(a_ptr->flags_obj, OF_SLOW_DIGEST);
		else if (choice == 6)
			of_on(a_ptr->flags_obj, OF_FEATHER);
	}

	/* Frequently neaten bonuses to Armour Class, Skill, and Deadliness. */
	{
		if ((a_ptr->to_a % 5 == 4) && (randint1(2) == 1))
			a_ptr->to_a++;
		else if ((a_ptr->to_a % 5 == 1) && (randint1(2) == 1))
			a_ptr->to_a--;
		else if ((a_ptr->to_a % 2 == 1) && (randint1(4) != 1))
			a_ptr->to_a++;
		if ((a_ptr->to_h % 5 == 4) && (randint1(2) == 1))
			a_ptr->to_h++;
		else if ((a_ptr->to_h % 5 == 1) && (randint1(2) == 1))
			a_ptr->to_h--;
		else if ((a_ptr->to_h % 2 == 1) && (randint1(4) != 1))
			a_ptr->to_h++;
		if ((a_ptr->to_d % 5 == 4) && (randint1(2) == 1))
			a_ptr->to_d++;
		else if ((a_ptr->to_d % 5 == 1) && (randint1(2) == 1))
			a_ptr->to_d--;
		else if ((a_ptr->to_d % 2 == 1) && (randint1(4) != 1))
			a_ptr->to_d++;
	}
}


/**
 * Envoke perilous magics, and curse the artifact beyound redemption!  I 
 * had such fun coding this...
 */
static void make_terrible(int a_idx)
{
	artifact_type *a_ptr = &a_info[a_idx];
	int i, j, gauntlet_runs, wheel_of_doom, penalty;
	int num_curses = 1;
	/* Determine whether the artifact's magics are perilous enough to warrant
	 * extra curses. */
	if (potential >= 3000)
		num_curses++;
	if ((potential >= 3500) && (randint1(3) != 1))
		num_curses++;
	/* Greatly decrease the chance for an activation. */
	if ((a_ptr->effect) && (randint1(3) != 1))
		a_ptr->effect = 0;
	/* Force the artifact though the gauntlet two or three times. */
	gauntlet_runs = 1 + randint1(2);
	for (i = 0; i < gauntlet_runs; i++) {

		/* Choose a curse, biased towards penalties to_a, to_d, and to_h. */
		if ((a_ptr->tval < TV_BOOTS) && (a_ptr->to_h > 0)
			&& (randint1(2) == 1))
			wheel_of_doom = 2;
		else if ((a_ptr->to_a > 0) && (randint1(2) == 1))
			wheel_of_doom = 1;
		else
			wheel_of_doom = 2 + randint1(2);
		penalty = 0;
		/* Blast base armour class or inflict a penalty to armour class. */
		if (wheel_of_doom == 1) {

			/* Blast armour and twist magics. */
			if ((a_ptr->ac) && (randint1(6) == 1)) {
				a_ptr->cost -= 500L * a_ptr->ac;
				a_ptr->ac = 0;
			}
			if ((a_ptr->tval >= TV_BOOTS) && (a_ptr->to_a >= 0)) {
				a_ptr->to_a = -(5 + randint1(7)) * 2;
			}

			/* Chance of a truly nasty effect for weapons. */
			else if ((randint1(4) == 1) && (a_ptr->tval < TV_BOOTS)) {
				penalty = randint1(3) + 2;
				penalty *= 5;
				a_ptr->to_a = -penalty;
				a_ptr->cost -= penalty * 200L;
			}

			/* Artifact might very well still have some value. */
			if (a_ptr->to_a < 0)
				a_ptr->cost += a_ptr->to_a * 400L;
		}

		/* Make either the Skill or Deadliness bonus negative, or both. */
		if (wheel_of_doom == 2) {

			/* Weapons. */
			if (a_ptr->tval < TV_BOOTS) {

				/* Blast items with bonuses, Deadliness more rarely. */
				a_ptr->to_h = -(5 + randint1(7)) * 2;
				if (randint1(3) != 1)
					a_ptr->to_d = -(5 + randint1(7)) * 2;
			}

			/* All armours. */
			else {

				/* Reverse any magics. */
				if (a_ptr->to_h > 0)
					a_ptr->to_h = -a_ptr->to_h;
				if (a_ptr->to_d > 0)
					a_ptr->to_d = -a_ptr->to_d;
				/* Sometimes, blast even items without bonuses. */
				else if ((a_ptr->to_d == 0) && (a_ptr->to_h == 0)
						 && (randint1(5) == 1)) {
					penalty = randint1(4) + 1;
					penalty *= 5;
					a_ptr->to_h -= penalty;
					a_ptr->to_d -= penalty;
				}
			}

			/* Artifact might very well still have some value. */
			if ((a_ptr->to_d < 0) && (a_ptr->to_h < 0)) {
				a_ptr->cost -= 10000L;
				if (a_ptr->cost < 0)
					a_ptr->cost = 0;
			}

			else if ((a_ptr->to_d < 0) || (a_ptr->to_h < 0)) {
				a_ptr->cost -= 5000L;
				if (a_ptr->cost < 0)
					a_ptr->cost = 0;
			}
		}

		/* Make any bonuses negative, or strip all bonuses, and add a random
		 * stat or three with large negative bonuses. */
		if (wheel_of_doom == 3) {
			if (randint1(3) != 1) {
				for (j = 0; j < A_MAX; j++) {
					penalty = -(a_ptr->bonus_stat[j]);
					a_ptr->bonus_stat[j] = penalty;
					penalty = -(a_ptr->bonus_other[j]);
					a_ptr->bonus_other[j] = penalty;
				}

				/* Artifact is highly unlikely to have any value. */
				a_ptr->cost -= 30000L;
				if (a_ptr->cost < 0)
					a_ptr->cost = 0;
			}

			else {

				/* Iron Crown of Beruthiel, here we come... */
				penalty = -(randint1(5)) - 3;
				if (randint1(3) == 1)
					penalty *= 5;
				for (j = 0; j < A_MAX; j++) {
					a_ptr->bonus_stat[j] = 0;
					a_ptr->bonus_other[j] = 0;
				}
				if (randint1(5) == 1) {
					a_ptr->bonus_stat[A_STR] = penalty;
					a_ptr->bonus_stat[A_DEX] = penalty;
					a_ptr->bonus_stat[A_CON] = penalty;
				}

				else if (randint1(4) == 1) {
					a_ptr->bonus_stat[A_WIS] = penalty;
					a_ptr->bonus_stat[A_INT] = penalty;
				}

				else if (randint1(6) == 1) {
					a_ptr->bonus_stat[P_BONUS_SPEED] = penalty;
				}

				else {
					for (j = 0; j < A_MAX; j++)
						if (randint1(4) == 1)
							a_ptr->bonus_stat[j] = penalty;
				}

				/* More curses for good measure... */
				num_curses += randint1(2);
				/* ...and hard to get rid of */
				cf_on(a_ptr->flags_curse, CF_STICKY_CARRY);
				/* Artifact is highly unlikely to have any value. */
				a_ptr->cost -= 60000L;
				if (a_ptr->cost < 0)
					a_ptr->cost = 0;
			}
		}

		/* Strip lots of other qualities. */
		if (wheel_of_doom == 4) {
			if (randint1(4) == 1)
				a_ptr->bonus_other[P_BONUS_SHOTS] = 0;
			if (randint1(4) == 1)
				a_ptr->bonus_other[P_BONUS_MIGHT] = 0;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_ANIMAL] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_EVIL] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_UNDEAD] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_DEMON] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_ORC] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_TROLL] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_GIANT] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_slay[P_SLAY_DRAGON] = 10;
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_PERFECT_BALANCE);
			if (randint1(3) == 1)
				a_ptr->multiple_brand[P_BRAND_POIS] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_brand[P_BRAND_ACID] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_brand[P_BRAND_ELEC] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_brand[P_BRAND_FIRE] = 10;
			if (randint1(3) == 1)
				a_ptr->multiple_brand[P_BRAND_COLD] = 10;
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_ACID] +=
					(2 * (100 - a_ptr->percent_res[P_RES_ACID]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_ELEC] +=
					(2 * (100 - a_ptr->percent_res[P_RES_ELEC]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_FIRE] +=
					(2 * (100 - a_ptr->percent_res[P_RES_FIRE]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_COLD] +=
					(2 * (100 - a_ptr->percent_res[P_RES_COLD]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_POIS] +=
					(2 * (100 - a_ptr->percent_res[P_RES_POIS]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_LIGHT] +=
					(2 * (100 - a_ptr->percent_res[P_RES_LIGHT]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_DARK] +=
					(2 * (100 - a_ptr->percent_res[P_RES_DARK]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_CONFU] +=
					(2 * (100 - a_ptr->percent_res[P_RES_CONFU]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_SOUND] +=
					(2 * (100 - a_ptr->percent_res[P_RES_SOUND]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_SHARD] +=
					(2 * (100 - a_ptr->percent_res[P_RES_SHARD]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_NEXUS] +=
					(2 * (100 - a_ptr->percent_res[P_RES_NEXUS]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_NETHR] +=
					(2 * (100 - a_ptr->percent_res[P_RES_NETHR]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_CHAOS] +=
					(2 * (100 - a_ptr->percent_res[P_RES_CHAOS]));
			if (randint1(3) == 1)
				a_ptr->percent_res[P_RES_DISEN] +=
					(2 * (100 - a_ptr->percent_res[P_RES_DISEN]));
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_SLOW_DIGEST);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_FEATHER);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_LIGHT);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_REGEN);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_TELEPATHY);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_SEE_INVIS);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_FREE_ACT);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_HOLD_LIFE);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_FEARLESS);
			if (randint1(3) == 1)
				of_off(a_ptr->flags_obj, OF_SEEING);
			/* Artifact will still have some value. */
			if (a_ptr->cost > 0)
				a_ptr->cost /= 3L;
		}
	}

	/* Boundary control. */
	if (a_ptr->cost < 0)
		a_ptr->cost = 0;
	/* Apply curses. */
	for (i = 0; i < num_curses; i++)
		cf_on(a_ptr->flags_curse, FLAG_START + randint0(CF_MAX));
}


/**
 * Clean up the artifact by removing illogical combinations of powers -  
 * curses win out every time.
 */
static void remove_contradictory(int a_idx)
{
	artifact_type *a_ptr = &a_info[a_idx];
	int i;
	if (cf_has(a_ptr->flags_curse, CF_AGGRO_PERM))
		a_ptr->bonus_other[P_BONUS_STEALTH] = 0;
	if (!cf_is_empty(a_ptr->flags_curse))
		of_off(a_ptr->flags_obj, OF_BLESSED);
	if (cf_has(a_ptr->flags_curse, CF_DRAIN_EXP))
		of_off(a_ptr->flags_obj, OF_HOLD_LIFE);
	if (cf_has(a_ptr->flags_curse, CF_SLOW_REGEN))
		of_off(a_ptr->flags_obj, OF_REGEN);
	if (cf_has(a_ptr->flags_curse, CF_AFRAID))
		of_off(a_ptr->flags_obj, OF_FEARLESS);
	if (cf_has(a_ptr->flags_curse, CF_HUNGRY))
		of_off(a_ptr->flags_obj, OF_SLOW_DIGEST);
	if (cf_has(a_ptr->flags_curse, CF_PARALYZE))
		of_off(a_ptr->flags_obj, OF_FREE_ACT);
	for (i = 0; i < A_MAX; i++) {
		if (a_ptr->bonus_stat[i] < 0)
			of_off(a_ptr->flags_obj, OF_SUSTAIN_STR + i);
		if (a_ptr->bonus_stat[i] > 6)
			a_ptr->bonus_stat[i] = 6;
	}
	for (i = 0; i < MAX_P_BONUS; i++) {
		if ((a_ptr->bonus_stat[i] > 6) && (i != P_BONUS_SPEED))
			a_ptr->bonus_stat[i] = 6;
	}
	if ((cf_has(a_ptr->flags_curse, CF_NO_TELEPORT))
		&& ((a_ptr->effect = EF_RAND_TELEPORT1)
			|| (a_ptr->effect = EF_RAND_TELEPORT2)))
		a_ptr->effect = 0;

	/* Add effect timeout */
	effect_time(a_ptr->effect, &a_ptr->time);
}


/**
 * Use W. Sheldon Simms' random name generator.  This function builds
 * probability tables which are used later on for letter selection.  It
 * relies on the ASCII character set.
 */
static void build_prob(void)
{
	byte next;
	int c_prev, c_cur, c_next;
	ang_file *f;
	char buf[BUFLEN];
	/* Open the file containing our lexicon, and read from it. Warn the rest of 
	 * the code that random names are unavailable on failure. */
	path_build(buf, BUFLEN, ANGBAND_DIR_FILE, NAMES_FILE);
	if ((f = file_open(buf, MODE_READ, FTYPE_TEXT)) == NULL) {
		find_all_names = TRUE;
		return;
	}

	/* Build raw frequencies */
	while (1) {
		bool not_eof;
		c_prev = c_cur = S_WORD;
		do {
			not_eof = file_readc(f, &next);
		} while (!isalpha(next)
				 && (not_eof));
		if (!not_eof)
			break;
		do {
			c_next = tolower(next) - 'a';	/* ASCII */
			lprobs[c_prev][c_cur][c_next]++;
			ltotal[c_prev][c_cur]++;
			c_prev = c_cur;
			c_cur = c_next;
			(void) file_readc(f, &next);
		} while (isalpha(next));
		lprobs[c_prev][c_cur][E_WORD]++;
		ltotal[c_prev][c_cur]++;
	}

	/* Close the file. */
	file_close(f);
}


/**
 * Use W. Sheldon Simms' random name generator.  Generate a random word 
 * using the probability tables we built earlier.  Relies on the ASCII 
 * character set.  Relies on European vowels (a, e, i, o, u).  The generated 
 * name should be copied/used before calling this function again.
 */
static char *make_word(void)
{
	static char word_buf[90];
	int r, totalfreq;
	int tries, lnum, vow;
	int c_prev, c_cur, c_next;
	char *cp;
  startover:vow = 0;
	lnum = 0;
	tries = 0;
	cp = word_buf;
	c_prev = c_cur = S_WORD;
	while (1) {
	  getletter:c_next = 0;
		r = randint0(ltotal[c_prev][c_cur]);
		totalfreq = lprobs[c_prev][c_cur][c_next];
		while (totalfreq <= r) {
			c_next++;
			totalfreq += lprobs[c_prev][c_cur][c_next];
		}
		if (c_next == E_WORD) {
			if ((lnum < MIN_NAME_LEN) || vow == 0) {
				tries++;
				if (tries < 10)
					goto getletter;
				goto startover;
			}
			*cp = '\0';
			break;
		}
		if (lnum >= MAX_NAME_LEN)
			goto startover;
		*cp = c_next + 'a';		/* ASCII */
		switch (*cp) {
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u':
			vow++;
		}
		cp++;
		lnum++;
		c_prev = c_cur;
		c_cur = c_next;
	}
	word_buf[0] = toupper(word_buf[0]);
	return word_buf;
}


/**
 * Find a name from any of various text files.
 */
static char *find_word(int a_idx)
{
	static char art_name[81];
	artifact_type *a_ptr = &a_info[a_idx];
	art_name[0] = '\0';
	/* Select a file, depending on whether the artifact is a weapon or armour,
	 * and whether or not it is cursed. Get a random line from that file.
	 * Decision now based on cost -NRM- */
	if (a_ptr->cost == 0L) {
		if ((a_ptr->tval == TV_BOW) || (a_ptr->tval == TV_SWORD)
			|| (a_ptr->tval == TV_HAFTED)
			|| (a_ptr->tval == TV_POLEARM))
			get_rnd_line("w_cursed.txt", art_name);
		else
			get_rnd_line("a_cursed.txt", art_name);
	}

	else {
		if ((a_ptr->tval == TV_BOW) || (a_ptr->tval == TV_SWORD)
			|| (a_ptr->tval == TV_HAFTED)
			|| (a_ptr->tval == TV_POLEARM))
			get_rnd_line("w_normal.txt", art_name);
		else
			get_rnd_line("a_normal.txt", art_name);
	}

	/* Secretly call the make_word function in case of failure. If it is
	 * unavailable, complain loudly. */
	if (!art_name[0]) {
		if (find_all_names) {
			msg("Cannot find any files with names or naming resources!");
			msg("Your 'files' folder lacks needed documents!");
		}

		/* Format the output of make_word. */
		else if (randint1(3) == 1)
			strcpy(art_name, format("'%s'", make_word()));
		else
			strcpy(art_name, format("of %s", make_word()));
	}

	/* Return the name. */
	return art_name;
}


/**
 * Name an artifact, using one of two methods.
 */
static void name_artifact(int a_idx)
{
	char *word;
	char buf[BUFLEN];
	/* Use W. Sheldon Simms' random name generator most of the time, generally
	 * for the less powerful artifacts, if not out of commision. Otherwise,
	 * find a name from a text file. */
	if (((randint1(max_potential) > initial_potential)
		 || (randint1(2) == 1))
		&& (find_all_names == FALSE)) {
		word = make_word();
		if (randint0(3) == 0)
			sprintf(buf, "'%s'", word);
		else
			sprintf(buf, "of %s", word);
	}

	else {
		word = find_word(a_idx);
		sprintf(buf, "%s", word);
	}

	/* Insert whatever name is created or found into the temporary array. */
	a_info[a_idx].name = string_make(word);
}


/**
 * Design a random artifact.
 */
static void design_random_artifact(int a_idx)
{
	/* Initialize the artifact, and assign it a potential. */
	initialize_artifact(a_idx);

	/* Assign a theme to the artifact. */
	choose_basic_theme(a_idx);

	/* Add extra qualities until done. */
	haggle_till_done(a_idx);

	/* Decide if the artifact is to be terrible. */
	if (TERRIBLE_CHANCE > randint0(100)) {

		/* If it is, add a few benefits and more drawbacks. */
		make_terrible(a_idx);
	}

	/* Remove contradictory powers. */
	remove_contradictory(a_idx);

	/* Find or make a name for the artifact, and place into a temporary array */
	name_artifact(a_idx);
}


/**
 * Initialize all the random artifacts in the artifact array.  This function 
 * is only called when a player is born.  Because various sub-functions use 
 * player information, it must be called after the player has been generated 
 * and player information has been loaded.
 */
void initialize_random_artifacts(void)
{
	/* Index of the artifact currently being initialized. */
	int a_idx;
	/* Initialize the W. Seldon Simms random name generator. */
	build_prob();
	/* Go from beginning to end of the random section of the artifact array,
	 * initializing and naming as we go. */
	for (a_idx = ART_MIN_RANDOM; a_idx < z_info->a_max; a_idx++) {

		/* Design the artifact, storing information as we go along. */
		design_random_artifact(a_idx);
	}
}
