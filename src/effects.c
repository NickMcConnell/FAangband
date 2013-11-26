/** \file effects.c 
    \brief Command, part 6

 * Code for eating food, drinking potions, reading scrolls, aiming wands, 
 * using staffs, zapping rods, and activating anything that can be 
 * activated.  Also defines all effects of the items listed, and all 
 * activations.  Ending a druid shapechange.
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
#include "effects.h"
#include "mapmode.h"
#include "monster.h"
#include "object.h"
#include "spells.h"
#include "target.h"


/**
 * If a spell/wand/rod/etc calls fire_bolt() followed by fire_cloud()
 * and the targetted monster is killed by the first fire_bolt(), then
 * the target is cleared and the second fire_cloud() will start at
 * the character's location. For example:
 *
 *     fire_bolt(GF_POIS, dir, damroll(plev / 2, 11));
 *     fire_cloud(GF_POIS, dir, 30, 6);
 *
 * The solution is to remember the target, and if the monster is
 * killed by the fire_bolt() then the target is set to the location
 * the monster was at. The macros to do this are:
 *
 * TARGET_DECLARE  -- Declare some variables
 * TARGET_PRESERVE -- Remember the current target location
 * TARGET_RESTORE  -- Set the target to the saved location
 *
 * The above statements would now be written:
 *
 *     TARGET_DECLARE
 *     ...
 *     TARGET_PRESERVE
 *     fire_bolt(GF_POIS, dir, damroll(plev / 2, 11));
 *     TARGET_RESTORE
 *     fire_cloud(GF_POIS, dir, 30, 6);
 */

#define TARGET_DECLARE \
	s16b save_target_y = 0, save_target_x = 0; \
	bool save_target_set;

#define TARGET_PRESERVE \
    if ((dir == 5) && target_okay() && target_get_monster())	\
	{ \
	    target_get(&save_target_x, &save_target_y); \
		save_target_set = TRUE; \
	} \
	else save_target_set = FALSE;

#define TARGET_RESTORE \
    if (save_target_set && !target_is_set())				\
		target_set_location(save_target_y, save_target_x);

/*
 * This file includes code for eating food, drinking potions,
 * reading scrolls, aiming wands, using staffs, zapping rods,
 * and activating artifacts.
 *
 * In all cases, if the player becomes "aware" of the item's use
 * by testing it, mark it as "aware" and reward some experience
 * based on the object's level, always rounding up.  If the player
 * remains "unaware", mark that object "kind" as "tried".
 *
 * This code now correctly handles the unstacking of wands, staffs,
 * and rods.  Note the overly paranoid warning about potential pack
 * overflow, which allows the player to use and drop a stacked item.
 *
 * In all "unstacking" scenarios, the "used" object is "carried" as if
 * the player had just picked it up.  In particular, this means that if
 * the use of an item induces pack overflow, that item will be dropped.
 *
 * For simplicity, these routines induce a full "pack reorganization"
 * which not only combines similar items, but also reorganizes various
 * items to obey the current "sorting" method.  This may require about
 * 400 item comparisons, but only occasionally.
 *
 * There may be a BIG problem with any "effect" that can cause "changes"
 * to the inventory.  For example, a "scroll of recharging" can cause
 * a wand/staff to "disappear", moving the inventory up.  Luckily, the
 * scrolls all appear BEFORE the staffs/wands, so this is not a problem.
 * But, for example, a "staff of recharging" could cause MAJOR problems.
 * In such a case, it will be best to either (1) "postpone" the effect
 * until the end of the function, or (2) "change" the effect, say, into
 * giving a staff "negative" charges, or "turning a staff into a stick".
 * It seems as though a "rod of recharging" might in fact cause problems.
 * The basic problem is that the act of recharging (and destroying) an
 * item causes the inducer of that action to "move", causing "o_ptr" to
 * no longer point at the correct item, with horrifying results.
 *
 * Note that food/potions/scrolls no longer use bit-flags for effects,
 * but instead use the "sval" (which is also used to sort the objects).
 */


/*
 * Entries for spell/activation descriptions
 */
typedef struct {
	u16b index;					/* Effect index */
	bool aim;					/* Whether the effect requires aiming */
	u16b power;					/* Power rating for obj-power.c */
	const char *desc;			/* Effect description */
	int base;					/* Effect timeout base */
	int sides;					/* Effect timeout sides */
} info_entry;

/*
 * Useful things about effects.
 */
static const info_entry effects[] = {
#define EFFECT(x, y, r, z, w, v)    { EF_##x, y, r, z, w, v },
#include "list-effects.h"
#undef EFFECT
};


/*
 * Utility functions
 */
bool effect_aim(effect_type effect)
{
	if (effect < 1 || effect > EF_MAX)
		return FALSE;

	return effects[effect].aim;
}

int effect_power(effect_type effect)
{
	if (effect < 1 || effect > EF_MAX)
		return FALSE;

	return effects[effect].power;
}

const char *effect_desc(effect_type effect)
{
	if (effect < 1 || effect > EF_MAX)
		return FALSE;

	return effects[effect].desc;
}

bool effect_obvious(effect_type effect)
{
	if (effect == EF_IDENTIFY)
		return TRUE;

	return FALSE;
}

bool effect_time(effect_type effect, random_value * time)
{
	if (effect < 1 || effect > EF_MAX)
		return FALSE;

	if (effects[effect].base == 0)
		return FALSE;

	(*time).base = effects[effect].base;
	(*time).dice = (effects[effect].sides == 0 ? 0 : 1);
	(*time).sides = effects[effect].sides;
	(*time).m_bonus = 0;

	return TRUE;
}

/**
 * The "wonder" effect.
 *
 * Returns TRUE if the effect is evident.
 */
bool effect_wonder(int dir, int die)
{
/* This spell should become more useful (more
   controlled) as the player gains experience levels.
   Thus, add 1/5 of the player's level to the die roll.
   This eliminates the worst effects later on, while
   keeping the results quite random.  It also allows
   some potent effects only at high level. */

	bool visible = FALSE;
	int py = p_ptr->py;
	int px = p_ptr->px;
	int plev = p_ptr->lev;

	if (die > 100) {
		/* above 100 the effect is always visible */
		msg("You feel a surge of power!");
		visible = TRUE;
	}

	if (die < 8)
		visible = clone_monster(dir);
	else if (die < 14)
		visible = speed_monster(dir);
	else if (die < 26)
		visible = heal_monster(dir);
	else if (die < 31)
		visible = poly_monster(dir);
	else if (die < 36)
		visible =
			fire_bolt_or_beam(10, GF_MISSILE, dir,
							  damroll(3 + ((plev - 1) / 5), 4));
	else if (die < 41)
		visible = confuse_monster(dir, plev);
	else if (die < 46)
		visible = fire_ball(GF_POIS, dir, 20 + (plev / 2), 3, TRUE);
	else if (die < 51)
		visible = light_line(dir);
	else if (die < 56)
		visible =
			fire_beam(GF_ELEC, dir, damroll(3 + ((plev - 5) / 6), 6));
	else if (die < 61)
		visible =
			fire_bolt_or_beam(10, GF_COLD, dir,
							  damroll(5 + ((plev - 5) / 4), 8));
	else if (die < 66)
		visible =
			fire_bolt_or_beam(10, GF_ACID, dir,
							  damroll(6 + ((plev - 5) / 4), 8));
	else if (die < 71)
		visible =
			fire_bolt_or_beam(10, GF_FIRE, dir,
							  damroll(8 + ((plev - 5) / 4), 8));
	else if (die < 76)
		visible = drain_life(dir, 75);
	else if (die < 81)
		visible = fire_ball(GF_ELEC, dir, 30 + plev / 2, 2, TRUE);
	else if (die < 86)
		visible = fire_ball(GF_ACID, dir, 40 + plev, 2, TRUE);
	else if (die < 91)
		visible = fire_ball(GF_ICE, dir, 70 + plev, 3, TRUE);
	else if (die < 96)
		visible = fire_ball(GF_FIRE, dir, 80 + plev, 3, TRUE);
	/* above 100 'visible' is already true */
	else if (die < 101)
		drain_life(dir, 100 + plev);
	else if (die < 104)
		earthquake(py, px, 12, FALSE);
	else if (die < 106)
		destroy_area(py, px, 15, TRUE);
	else if (die < 108)
		genocide();
	else if (die < 110)
		dispel_monsters(120);
	else {						/* RARE */

		dispel_monsters(150);
		slow_monsters(50);
		sleep_monsters(TRUE);
		hp_player(300);
	}

	return visible;
}



/**
 * Do an effect, given an object.
 * Boost is the extent to which skill surpasses difficulty, used as % boost. It
 * ranges from 0 to 138.
 */
bool effect_do(effect_type effect, bool * ident, bool aware, int dir)
{
	int py = p_ptr->py;
	int px = p_ptr->px;
	int plev = p_ptr->lev;
	int chance, k;

	if (effect < 1 || effect > EF_MAX) {
		msg("Bad effect passed to do_effect().  Please report this bug.");
		return FALSE;
	}

	/* Analyze the effect */
	switch (effect) {
	case EF_POISON1:
		{
			if (pois_hit(15))
				*ident = TRUE;
			return TRUE;
		}

	case EF_POISON2:
		{
			if (pois_hit(30))
				*ident = TRUE;
			return TRUE;
		}

	case EF_BLIND1:
		{
			if (!p_ptr->state.no_blind) {
				if (inc_timed(TMD_BLIND, randint0(200) + 200, TRUE)) {
					*ident = TRUE;
				}
			} else if (aware)
				notice_obj(OF_SEEING, 0);
			return TRUE;
		}

	case EF_BLIND2:
		{
			if (!p_ptr->state.no_blind) {
				if (inc_timed(TMD_BLIND, randint0(100) + 100, TRUE)) {
					*ident = TRUE;
				}
			} else if (aware)
				notice_obj(OF_SEEING, 0);
			return TRUE;
		}

	case EF_SCARE:
		{
			if (!p_ptr->state.no_fear) {
				if (inc_timed(TMD_AFRAID, randint0(10) + 10, TRUE)) {
					*ident = TRUE;
				}
			} else if (aware)
				notice_obj(OF_FEARLESS, 0);
			return TRUE;
		}

	case EF_CONFUSE1:
		{
			if (!p_resist_good(P_RES_CONFU)) {
				if (inc_timed(TMD_CONFUSED, randint0(10) + 10, TRUE)) {
					*ident = TRUE;
				}
			} else {
				notice_other(IF_RES_CONFU, 0);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_CONFUSE2:
		{
			if (!p_resist_good(P_RES_CONFU)) {
				if (inc_timed(TMD_CONFUSED, randint0(20) + 15, TRUE)) {
					*ident = TRUE;
				}
			} else {
				notice_other(IF_RES_CONFU, 0);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_HALLUC:
		{
			if (!p_resist_good(P_RES_CHAOS)) {
				if (inc_timed(TMD_IMAGE, randint0(250) + 250, TRUE)) {
					*ident = TRUE;
				}
			} else {
				notice_other(IF_RES_CHAOS, 0);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_PARALYSE:
		{
			if (!p_ptr->state.free_act) {
				if (inc_timed(TMD_PARALYZED, randint0(10) + 10, TRUE)) {
					*ident = TRUE;
				}
			} else {
				notice_obj(OF_FREE_ACT, 0);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_WEAKNESS:
		{
			take_hit(damroll(6, 6), "poisonous food.");
			(void) do_dec_stat(A_STR);
			*ident = TRUE;
			return TRUE;
		}

	case EF_SICKNESS:
		{
			take_hit(damroll(6, 6), "poisonous food.");
			(void) do_dec_stat(A_CON);
			*ident = TRUE;
			return TRUE;
		}

	case EF_STUPIDITY:
		{
			take_hit(damroll(8, 8), "poisonous food.");
			(void) do_dec_stat(A_INT);
			*ident = TRUE;
			return TRUE;
		}

	case EF_NAIVETY:
		{
			take_hit(damroll(8, 8), "poisonous food.");
			(void) do_dec_stat(A_WIS);
			*ident = TRUE;
			return TRUE;
		}

	case EF_UNHEALTH:
		{
			take_hit(damroll(10, 10), "poisonous food.");
			(void) do_dec_stat(A_CON);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DISEASE:
		{
			take_hit(damroll(10, 10), "poisonous food.");
			(void) do_dec_stat(A_STR);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DETONATIONS:
		{
			msg("Massive explosions rupture your body!");
			take_hit(damroll(50, 20), "a potion of Detonation");
			(void) inc_timed(TMD_STUN, 75, TRUE);
			(void) inc_timed(TMD_CUT, 5000, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DEATH:
		{
			msg("A feeling of Death flows through your body.");
			take_hit(p_ptr->chp, "a potion of Death");
			*ident = TRUE;
			return TRUE;
		}

	case EF_SLOWNESS1:
		{
			if (inc_timed(TMD_SLOW, randint1(25) + 15, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SALT_WATER:
		{
			msg("The potion makes you vomit!");
			(void) set_food(PY_FOOD_STARVE - 1);
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) inc_timed(TMD_PARALYZED, 4, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_SLEEP:
		{
			if (!p_ptr->state.free_act) {
				if (inc_timed(TMD_PARALYZED, randint0(4) + 4, TRUE)) {
					*ident = TRUE;
				}
			} else {
				notice_obj(OF_FREE_ACT, 0);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_LOSE_MEMORIES:
		{
			if (!p_ptr->state.hold_life && (p_ptr->exp > 0)) {
				msg("You feel your memories fade.");
				lose_exp(p_ptr->exp / 4);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_RUINATION:
		{
			msg("Your nerves and muscles feel weak and lifeless!");
			take_hit(damroll(5, 10), "a potion of Ruination");
			(void) dec_stat(A_DEX, 25, FALSE);
			(void) dec_stat(A_WIS, 25, FALSE);
			(void) dec_stat(A_CON, 25, FALSE);
			(void) dec_stat(A_STR, 25, FALSE);
			(void) dec_stat(A_CHR, 25, FALSE);
			(void) dec_stat(A_INT, 25, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DEC_STR:
	case EF_DEC_INT:
	case EF_DEC_WIS:
	case EF_DEC_DEX:
	case EF_DEC_CON:
	case EF_DEC_CHR:
		{
			int stat = effect - EF_DEC_STR;
			if (do_dec_stat(stat))
				*ident = TRUE;
			return TRUE;
		}

	case EF_AGGRAVATE:
		{
			msg("There is a high pitched humming noise.");
			(void) aggravate_monsters(1, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_CURSE_ARMOR:
		{
			if (curse_armor())
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURSE_WEAPON:
		{
			if (curse_weapon())
				*ident = TRUE;
			return TRUE;
		}

	case EF_SUMMON3:
		{
			sound(MSG_SUM_MONSTER);
			for (k = 0; k < randint1(3); k++) {
				if (summon_specific(py, px, FALSE, p_ptr->depth, 0)) {
					*ident = TRUE;
				}
			}
			return TRUE;
		}

	case EF_SUMMON_UNDEAD:
		{
			sound(MSG_SUM_UNDEAD);
			for (k = 0; k < randint1(3); k++) {
				if (summon_specific
					(py, px, FALSE, p_ptr->depth, SUMMON_UNDEAD)) {
					*ident = TRUE;
				}
			}
			return TRUE;
		}

	case EF_TRAP_CREATION:
		{
			if (trap_creation())
				*ident = TRUE;
			return TRUE;
		}

	case EF_DARKNESS:
		{
			if (!p_ptr->state.no_blind) {
				(void) inc_timed(TMD_BLIND, 3 + randint1(5), TRUE);
			} else if (aware)
				notice_obj(OF_SEEING, 0);
			if (unlight_area(10, 3))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SLOWNESS2:
		{
			if (inc_timed(TMD_SLOW, randint1(30) + 15, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HASTE_MONSTERS:
		{
			if (speed_monsters())
				*ident = TRUE;
			return TRUE;
		}

	case EF_SUMMON4:
		{
			sound(MSG_SUM_MONSTER);
			for (k = 0; k < randint1(4); k++) {
				if (summon_specific(py, px, FALSE, p_ptr->depth, 0)) {
					*ident = TRUE;
				}
			}
			return TRUE;
		}

	case EF_HEAL_MONSTER:
		{
			if (heal_monster(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HASTE_MONSTER:
		{
			if (speed_monster(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CLONE_MONSTER:
		{
			if (clone_monster(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_ROUSE_LEVEL:
		{
			msg("A mighty blast of horns shakes the air, and you hear stirring everwhere!");
			(void) aggravate_monsters(1, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_POISON:
		{
			if (clear_timed(TMD_POISONED, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_BLINDNESS:
		{
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_PARANOIA:
		{
			if (clear_timed(TMD_AFRAID, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_CONFUSION:
		{
			if (clear_timed(TMD_CONFUSED, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_SMALL:
		{
			if (hp_player(damroll(4, 8)))
				*ident = TRUE;
			return TRUE;
		}

	case EF_RES_STR:
	case EF_RES_INT:
	case EF_RES_WIS:
	case EF_RES_DEX:
	case EF_RES_CON:
	case EF_RES_CHR:
		{
			int stat = effect - EF_RES_STR;
			if (do_res_stat(stat))
				*ident = TRUE;
			return TRUE;
		}

	case EF_RESTORING:
		{
			if (do_res_stat(A_STR))
				*ident = TRUE;
			if (do_res_stat(A_INT))
				*ident = TRUE;
			if (do_res_stat(A_WIS))
				*ident = TRUE;
			if (do_res_stat(A_DEX))
				*ident = TRUE;
			if (do_res_stat(A_CON))
				*ident = TRUE;
			if (do_res_stat(A_CHR))
				*ident = TRUE;
			return TRUE;
		}

	case EF_FOOD_ATHELAS:
		{
			msg("A fresh, clean essence rises, driving away wounds and poison.");
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			if (p_ptr->black_breath) {
				msg("The hold of the Black Breath on you is broken!");
			}
			p_ptr->black_breath = FALSE;
			*ident = TRUE;
			return TRUE;
		}

	case EF_FOOD_BEORNING:
		{
			msg("The cakes of the Beornings are tasty.");
			(void) hp_player(damroll(5, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_FOOD_GOOD:
		{
			msg("That tastes good.");
			*ident = TRUE;
			return TRUE;
		}

		/* Waybread is always fully satisfying. */
	case EF_FOOD_WAYBREAD:
		{
			msg("That tastes good.");
			(void) set_food(PY_FOOD_MAX - 1);
			(void) dec_timed(TMD_POISONED, p_ptr->timed[TMD_POISONED] / 2,
							 TRUE);
			(void) hp_player(damroll(5, 10));
			*ident = TRUE;
			return TRUE;
		}




	case EF_DRINK_GOOD:
		{
			msg("You feel less thirsty.");
			*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_LIGHT:
		{
			if (hp_player(damroll(2, 10)))
				*ident = TRUE;
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			if (dec_timed(TMD_CUT, 10, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_SERIOUS:
		{
			if (hp_player(damroll(4, 10)))
				*ident = TRUE;
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CONFUSED, TRUE))
				*ident = TRUE;
			if (dec_timed(TMD_CUT, 30, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_CRITICAL:
		{
			if (hp_player(damroll(6, 10)))
				*ident = TRUE;
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CONFUSED, TRUE))
				*ident = TRUE;
			if (dec_timed
				(TMD_POISONED, p_ptr->timed[TMD_POISONED] + 10, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (dec_timed(TMD_CUT, 50, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HEALING1:
		{
			if (hp_player(300))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CUT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HEALING2:
		{
			if (hp_player(500))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CUT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HEALING3:
		{
			if (hp_player(300))
				*ident = TRUE;
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CONFUSED, TRUE))
				*ident = TRUE;
			if (dec_timed(TMD_POISONED, 200, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CUT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HEALING4:
		{
			if (hp_player(600))
				*ident = TRUE;
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CONFUSED, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_POISONED, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CUT, TRUE))
				*ident = TRUE;
			if (p_ptr->black_breath) {
				msg("The hold of the Black Breath on you is broken!");
				*ident = TRUE;
			}
			p_ptr->black_breath = FALSE;
			return TRUE;
		}

	case EF_LIFE:
		{
			msg("You feel life flow through your body!");
			restore_level();
			(void) clear_timed(TMD_BLIND, TRUE);
			(void) clear_timed(TMD_CONFUSED, TRUE);
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) clear_timed(TMD_IMAGE, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			(void) do_res_stat(A_STR);
			(void) do_res_stat(A_CON);
			(void) do_res_stat(A_DEX);
			(void) do_res_stat(A_WIS);
			(void) do_res_stat(A_INT);
			(void) do_res_stat(A_CHR);
			hp_player(2000);
			if (p_ptr->black_breath) {
				msg("The hold of the Black Breath on you is broken!");
			}
			p_ptr->black_breath = FALSE;
			*ident = TRUE;
			return TRUE;
		}

	case EF_INFRAVISION:
		{
			if (inc_timed(TMD_SINFRA, 100 + randint1(100), TRUE)) {
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_SEE_INVIS:
		{
			if (inc_timed(TMD_SINVIS, 12 + randint1(12), TRUE)) {
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_SLOW_POISON:
		{
			if (dec_timed
				(TMD_POISONED, p_ptr->timed[TMD_POISONED] / 2, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SPEED1:
		{
			if (!p_ptr->timed[TMD_FAST]) {
				if (set_timed(TMD_FAST, randint1(20) + 15, TRUE))
					*ident = TRUE;
			} else {
				(void) inc_timed(TMD_FAST, 5, TRUE);
			}
			return TRUE;
		}

	case EF_RES_HEAT_COLD:
		{
			if (inc_timed(TMD_OPP_FIRE, randint1(30) + 20, TRUE)) {
				*ident = TRUE;
			}
			if (inc_timed(TMD_OPP_COLD, randint1(30) + 20, TRUE)) {
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_RES_ACID_ELEC:
		{
			if (inc_timed(TMD_OPP_ACID, randint1(30) + 20, TRUE)) {
				*ident = TRUE;
			}
			if (inc_timed(TMD_OPP_ELEC, randint1(30) + 20, TRUE)) {
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_RESIST_ALL:
		{
			if (inc_timed(TMD_OPP_FIRE, randint1(25) + 15, TRUE)) {
				*ident = TRUE;
			}
			if (inc_timed(TMD_OPP_COLD, randint1(25) + 15, TRUE)) {
				*ident = TRUE;
			}
			if (inc_timed(TMD_OPP_ACID, randint1(25) + 15, TRUE)) {
				*ident = TRUE;
			}
			if (inc_timed(TMD_OPP_ELEC, randint1(25) + 15, TRUE)) {
				*ident = TRUE;
			}
			if (inc_timed(TMD_OPP_POIS, randint1(25) + 15, TRUE)) {
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_HEROISM:
		{
			if (hp_player(10))
				*ident = TRUE;
			if (clear_timed(TMD_AFRAID, TRUE))
				*ident = TRUE;
			if (inc_timed(TMD_HERO, randint1(25) + 25, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_BERSERK_STR:
		{
			if (hp_player(30))
				*ident = TRUE;
			if (clear_timed(TMD_AFRAID, TRUE))
				*ident = TRUE;
			if (inc_timed(TMD_SHERO, randint1(25) + 25, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_RESTORE_MANA:
		{
			if (p_ptr->csp < p_ptr->msp) {
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				msg("Your magical powers are completely restored!");
				p_ptr->redraw |= (PR_MANA);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_RESTORE_EXP:
		{
			if (restore_level())
				*ident = TRUE;
			return TRUE;
		}

	case EF_INC_STR:
	case EF_INC_INT:
	case EF_INC_WIS:
	case EF_INC_DEX:
	case EF_INC_CON:
	case EF_INC_CHR:
		{
			int stat = effect - EF_INC_STR;
			if (do_inc_stat(stat, FALSE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_STAR_INC_STR:
	case EF_STAR_INC_INT:
	case EF_STAR_INC_WIS:
	case EF_STAR_INC_DEX:
	case EF_STAR_INC_CON:
	case EF_STAR_INC_CHR:
		{
			int stat = effect - EF_STAR_INC_STR;
			if (do_inc_stat(stat, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_AUGMENTATION:
		{
			if (do_inc_stat(A_STR, TRUE))
				*ident = TRUE;
			if (do_inc_stat(A_INT, TRUE))
				*ident = TRUE;
			if (do_inc_stat(A_WIS, TRUE))
				*ident = TRUE;
			if (do_inc_stat(A_DEX, TRUE))
				*ident = TRUE;
			if (do_inc_stat(A_CON, TRUE))
				*ident = TRUE;
			if (do_inc_stat(A_CHR, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_ENLIGHTENMENT1:
		{
			msg("An image of your surroundings forms in your mind...");
			wiz_light(FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_ENLIGHTENMENT2:
		{
			/* Hack - 'show' effected region only with the first detect */
			msg("You begin to feel more enlightened...");
			message_flush();
			wiz_light(TRUE);
			(void) do_inc_stat(A_INT, TRUE);
			(void) do_inc_stat(A_WIS, TRUE);
			(void) detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_doors(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_treasure(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_objects_gold(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_objects_normal(DETECT_RAD_DEFAULT, FALSE);
			identify_pack();
			*ident = TRUE;
			return TRUE;
		}

	case EF_EXPERIENCE:
		{
			if (p_ptr->exp < PY_MAX_EXP) {
				s32b ee = (p_ptr->exp / 2) + 10;
				if (ee > 100000L)
					ee = 100000L;
				msg("You feel more experienced.");
				gain_exp(ee);
				*ident = TRUE;
			}
			return TRUE;
		}

	case EF_VAMPIRE:
		{

			/* Already a Vampire */
			if (p_ptr->schange == SHAPE_VAMPIRE)
				return TRUE;

			/* Priests/Paladins can't be Vampires */
			if (mp_ptr->spell_book == TV_PRAYER_BOOK) {
				msg("You reject the unholy serum.");
				take_hit(damroll(10, 6), "dark forces");
				return TRUE;
			}

			/* Druids/Rangers can't be Vampires */
			if (mp_ptr->spell_book == TV_DRUID_BOOK) {
				msg("You reject the unnatural serum.");
				take_hit(damroll(10, 6), "dark forces");
				return TRUE;
			}

			/* Others can */
			msg("You are infused with dark power.");

			/* But it hurts */
			take_hit(damroll(3, 6), "shapeshifting stress");
			shapechange(SHAPE_VAMPIRE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_PHASE_DOOR:
		{
			teleport_player(10, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_TELEPORT100:
		{
			teleport_player(100, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_TELEPORT_LEVEL:
		{
			(void) teleport_player_level(TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_RECALL:
		{
			*ident = TRUE;
			if (!word_recall(randint0(20) + 15))
				return FALSE;
			return TRUE;
		}

	case EF_IDENTIFY:
	case EF_RAND_IDENTIFY:
		{
			*ident = TRUE;
			if (!ident_spell())
				return FALSE;
			return TRUE;
		}

	case EF_REVEAL_CURSES:
		{
			*ident = TRUE;
			if (!identify_fully())
				return FALSE;
			return TRUE;
		}

	case EF_BRANDING:
	case EF_RAND_BRAND_MISSILE:
		{
			*ident = TRUE;
			if (!brand_missile(0, 0))
				return FALSE;
			return TRUE;
		}

	case EF_FRIGHTENING:
		{
			if (fear_monster(dir, p_ptr->lev + 5))
				*ident = TRUE;
			return TRUE;
		}

	case EF_REMOVE_CURSE:
		{
			bool used = FALSE;
			if (remove_curse()) {
				used = TRUE;
			}
			*ident = TRUE;
			return used;
		}

	case EF_REM_CURSE_GOOD:
		{
			bool used = FALSE;
			if (remove_curse_good()) {
				used = TRUE;
			}
			*ident = TRUE;
			return used;
		}

	case EF_ENCHANT_ARMOR1:
		{
			*ident = TRUE;
			if (!enchant_spell(0, 0, 1))
				return FALSE;
			return TRUE;
		}

	case EF_ENCHANT_ARMOR2:
		{
			*ident = TRUE;
			if (!enchant_spell(0, 0, randint1(3) + 2))
				return FALSE;
			return TRUE;
		}

	case EF_ENCHANT_TO_HIT:
		{
			*ident = TRUE;
			if (!enchant_spell(1, 0, 0))
				return FALSE;
			return TRUE;
		}

	case EF_ENCHANT_TO_DAM:
		{
			*ident = TRUE;
			if (!enchant_spell(0, 1, 0))
				return FALSE;
			return TRUE;
		}

	case EF_ENCHANT_WEAPON:
		{
			*ident = TRUE;
			if (!enchant_spell(randint1(3), randint1(3), 0))
				return FALSE;
			return TRUE;
		}

	case EF_RECHARGING1:
		{
			*ident = TRUE;
			if (!recharge(130))
				return FALSE;
			return TRUE;
		}

	case EF_RECHARGING2:
		{
			*ident = TRUE;
			if (!recharge(200))
				return FALSE;
			return TRUE;
		}

	case EF_LIGHT:
		{
			if (light_area(damroll(2, 8), 2))
				*ident = TRUE;
			return TRUE;
		}

	case EF_MAPPING:
		{
			map_area(0, 0, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DETECT_GOLD:
		{
			/* Hack - 'show' effected region only with the first detect */
			if (detect_treasure(DETECT_RAD_DEFAULT, TRUE))
				*ident = TRUE;
			if (detect_objects_gold(DETECT_RAD_DEFAULT, FALSE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DETECT_ITEM:
		{
			if (detect_objects_normal(DETECT_RAD_DEFAULT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DETECT_TRAP:
		{
			if (detect_traps(DETECT_RAD_DEFAULT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DETECT_DOOR:
		{
			/* Hack - 'show' effected region only with the first detect */
			if (detect_doors(DETECT_RAD_DEFAULT, TRUE))
				*ident = TRUE;
			if (detect_stairs(DETECT_RAD_DEFAULT, FALSE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DETECT_INVIS:
		{
			if (detect_monsters_invis(DETECT_RAD_DEFAULT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SATISFY_HUNGER:
		{
			if (set_food(PY_FOOD_MAX - 1))
				*ident = TRUE;
			return TRUE;
		}

	case EF_BLESSING1:
		{
			if (inc_timed(TMD_BLESSED, randint1(12) + 6, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_BLESSING2:
		{
			if (inc_timed(TMD_BLESSED, randint1(24) + 12, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_BLESSING3:
		{
			if (inc_timed(TMD_BLESSED, randint1(48) + 24, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_MONSTER_CONFU:
		{
			if (!(p_ptr->special_attack & (ATTACK_CONFUSE))) {
				msg("Your hands begin to glow.");
				p_ptr->special_attack |= (ATTACK_CONFUSE);
				*ident = TRUE;
				p_ptr->redraw |= PR_STATUS;
			}
			return TRUE;
		}

	case EF_PROT_FROM_EVIL:
		{
			k = 3 * p_ptr->lev;
			if (inc_timed(TMD_PROTEVIL, randint1(25) + k, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_RUNE_PROTECT:
		{
			/* Use up scroll only if warding_glyph is created. */
			*ident = TRUE;
			if (!lay_rune(RUNE_PROTECT))
				return FALSE;
			return TRUE;
		}

	case EF_DOOR_DESTRUCT:
		{
			if (destroy_doors_touch())
				*ident = TRUE;
			return TRUE;
		}

	case EF_DESTRUCTION:
		{
			destroy_area(py, px, 15, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DISPEL_UNDEAD:
		{
			if (dispel_undead(60))
				*ident = TRUE;
			return TRUE;
		}

	case EF_GENOCIDE:
		{
			*ident = TRUE;
			if (!genocide())
				return FALSE;
			return TRUE;
		}

	case EF_MASS_GENOCIDE:
		{
			*ident = TRUE;
			(void) mass_genocide();
			return TRUE;
		}

	case EF_ACQUIREMENT1:
		{
			acquirement(py, px, 1, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_ACQUIREMENT2:
		{
			acquirement(py, px, randint1(2) + 1, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_ELE_ATTACKS:		/* -LM- */
		{
			k = randint0(5);

			/* Give an elemental attack for 400 turns. */
			if (k == 0)
				set_ele_attack(ATTACK_ACID, 400);
			if (k == 1)
				set_ele_attack(ATTACK_ELEC, 400);
			if (k == 2)
				set_ele_attack(ATTACK_FIRE, 400);
			if (k == 3)
				set_ele_attack(ATTACK_COLD, 400);
			if (k == 4)
				set_ele_attack(ATTACK_POIS, 400);
			*ident = TRUE;

			return TRUE;
		}
	case EF_ACID_PROOF:
		{
			bitflag proof_flag[OF_SIZE];
			of_wipe(proof_flag);
			of_on(proof_flag, OF_ACID_PROOF);
			*ident = TRUE;
			if (!el_proof(proof_flag))
				return FALSE;
			return TRUE;
		}

	case EF_ELEC_PROOF:
		{
			bitflag proof_flag[OF_SIZE];
			of_wipe(proof_flag);
			of_on(proof_flag, OF_ELEC_PROOF);
			*ident = TRUE;
			if (!el_proof(proof_flag))
				return FALSE;
			return TRUE;
		}

	case EF_FIRE_PROOF:
		{
			bitflag proof_flag[OF_SIZE];
			of_wipe(proof_flag);
			of_on(proof_flag, OF_FIRE_PROOF);
			*ident = TRUE;
			if (!el_proof(proof_flag))
				return FALSE;
			return TRUE;
		}

	case EF_COLD_PROOF:
		{
			bitflag proof_flag[OF_SIZE];
			of_wipe(proof_flag);
			of_on(proof_flag, OF_COLD_PROOF);
			*ident = TRUE;
			if (!el_proof(proof_flag))
				return FALSE;
			return TRUE;
		}

	case EF_STARLIGHT:
		{
			/* Message. */
			if (!p_ptr->timed[TMD_BLIND]) {
				msg("The staff glitters with unearthly light.");
			}

			/* Starbursts everywhere. */
			do_starlight(randint0(8) + 7, 12, FALSE);

			/* Hard not to *identify. */
			*ident = TRUE;
			return TRUE;
		}

	case EF_DETECT_EVIL:
		{
			if (detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CURE_MEDIUM:
		{
			if (hp_player(randint1(20) + 10))
				*ident = TRUE;
			(void) dec_timed(TMD_CUT, 10, TRUE);
			return TRUE;
		}

	case EF_CURING:
		{
			if (clear_timed(TMD_BLIND, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_POISONED, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CONFUSED, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CUT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_BANISHMENT:
		{
			if (banish_evil(80)) {
				*ident = TRUE;
				msg("A mighty force drives away evil!");
			}
			return TRUE;
		}

	case EF_SLEEP_MONSTERS:
		{
			if (sleep_monsters(p_ptr->lev + 10))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SLOW_MONSTERS:
		{
			if (slow_monsters(p_ptr->lev + 10))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SPEED2:
		{
			if (!p_ptr->timed[TMD_FAST]) {
				if (set_timed(TMD_FAST, randint1(30) + 15, TRUE))
					*ident = TRUE;
			} else {
				(void) inc_timed(TMD_FAST, 5, TRUE);
			}
			return TRUE;
		}

	case EF_PROBING:
		{
			probing();
			*ident = TRUE;
			return TRUE;
		}

	case EF_DISPEL_EVIL:
		{
			if (dispel_evil(60))
				*ident = TRUE;
			return TRUE;
		}

	case EF_POWER:
		{
			if (dispel_monsters(100))
				*ident = TRUE;
			return TRUE;
		}

	case EF_HOLINESS:
		{
			if (dispel_evil(120))
				*ident = TRUE;
			k = 2 * p_ptr->lev;
			if (inc_timed(TMD_PROTEVIL, randint1(25) + k, TRUE))
				*ident = TRUE;
			if (dec_timed
				(TMD_POISONED, p_ptr->timed[TMD_POISONED] / 2 - 10, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_AFRAID, TRUE))
				*ident = TRUE;
			if (hp_player(50))
				*ident = TRUE;
			if (clear_timed(TMD_STUN, TRUE))
				*ident = TRUE;
			if (clear_timed(TMD_CUT, TRUE))
				*ident = TRUE;
			return TRUE;
		}

	case EF_EARTHQUAKES:
		{
			earthquake(py, px, 10, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DETECTION:
		{
			detect_all(DETECT_RAD_DEFAULT, TRUE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_MSTORM:
		{

			msg("Mighty magics rend your enemies!");
			fire_sphere(GF_MANA, 0, randint1(75) + 125, 5, 20);
			if (!(player_has(PF_DEVICE_EXPERT))) {
				(void) take_hit(20,
								"unleashing magics too mighty to control");
			}
			*ident = TRUE;
			return TRUE;
		}

	case EF_STARBURST:
		{
			msg("Light bright beyond enduring dazzles your foes!");
			fire_sphere(GF_LIGHT, 0, randint1(67) + 100, 5, 20);
			*ident = TRUE;
			return TRUE;
		}

	case EF_MASS_CONFU:
		{
			if (confu_monsters(p_ptr->lev + 30))
				*ident = TRUE;
			return TRUE;
		}


	case EF_STARFIRE:
		{
			/* Message. */
			if (!p_ptr->timed[TMD_BLIND]) {
				msg("The staff blazes with unearthly light.");
			}

			/* (large) Starbursts everywhere. */
			do_starlight(randint0(8) + 7, 30, TRUE);

			/* Hard not to *identify. */
			*ident = TRUE;
			return TRUE;
		}

	case EF_WINDS:
		{
			/* Raise a storm. */
			msg("A howling whirlwind rises in wrath around you.");
			fire_sphere(GF_FORCE, 0, randint1(100) + 100, 6, 20);

			/* Whisk around the player and nearby monsters.  This is actually
			 * kinda amusing to see... */
			fire_ball(GF_AWAY_ALL, 0, 12, 6, FALSE);
			teleport_player(6, TRUE);

			/* *Identify */
			*ident = TRUE;
			return TRUE;
		}

	case EF_HOLDING:
		{
			msg("You envoke binding magics upon the undead nearby!");

			/* Attempt to Hold all undead in LOS. */
			if (hold_undead())
				*ident = TRUE;

			return TRUE;
		}

	case EF_KELVAR:
		{
			/* Try to learn about the dungeon and traps in it from animals. */
			if (listen_to_natural_creatures())
				msg("You listen and learn from natural creatures.");
			else
				msg("You found no animals nearby to learn from.");

			/* *Identify */
			*ident = TRUE;
			return TRUE;
		}

	case EF_TELEPORT_AWAY2:
		{
			if (teleport_monster(dir, 55 + (plev / 2)))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DISARMING:
		{
			if (disarm_trap(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DOOR_DEST:
		{
			if (destroy_door(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_STONE_TO_MUD:
		{
			if (wall_to_mud(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_LIGHT_LINE:
		{
			msg("A line of blue shimmering light appears.");
			light_line(dir);
			*ident = TRUE;
			return TRUE;
		}

	case EF_SLEEP_MONSTER2:
		{
			if (sleep_monster(dir, plev + 15))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SLOW_MONSTER2:
		{
			if (slow_monster(dir, plev + 15))
				*ident = TRUE;
			return TRUE;
		}

	case EF_CONFUSE_MONSTER:
		{
			if (confuse_monster(dir, plev + 15))
				*ident = TRUE;
			return TRUE;
		}

	case EF_FEAR_MONSTER:
		{
			if (fear_monster(dir, plev + 20))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DRAIN_LIFE1:
		{
			if (drain_life(dir, 50 + plev))
				*ident = TRUE;
			return TRUE;
		}

	case EF_POLYMORPH:
		{
			if (poly_monster(dir))
				*ident = TRUE;
			return TRUE;
		}

	case EF_STINKING_CLOUD:
		{
			fire_ball(GF_POIS, dir, 12, 2, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_MAGIC_MISSILE:
		{
			fire_bolt_or_beam(20, GF_MANA, dir, damroll(2, 6));
			*ident = TRUE;
			return TRUE;
		}

	case EF_ACID_BOLT1:
		{
			fire_bolt_or_beam(plev, GF_ACID, dir,
							  damroll(5 + plev / 10, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_ELEC_BOLT1:
		{
			fire_bolt_or_beam(plev, GF_ELEC, dir,
							  damroll(3 + plev / 14, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_FIRE_BOLT1:
		{
			fire_bolt_or_beam(plev, GF_FIRE, dir,
							  damroll(6 + plev / 8, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_COLD_BOLT1:
		{
			fire_bolt_or_beam(plev, GF_COLD, dir,
							  damroll(4 + plev / 12, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_ACID_BALL1:
		{
			fire_ball(GF_ACID, dir, 60 + 3 * plev / 5, 3, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_ELEC_BALL1:
		{
			fire_ball(GF_ELEC, dir, 40 + 3 * plev / 5, 3, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_FIRE_BALL1:
		{
			fire_ball(GF_FIRE, dir, 70 + 3 * plev / 5, 3, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_COLD_BALL1:
		{
			fire_ball(GF_COLD, dir, 50 + 3 * plev / 5, 3, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_WONDER:
		{
			if (effect_wonder(dir, randint1(100) + p_ptr->lev / 5))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DRAGON_FIRE:
		{
			fire_arc(GF_FIRE, dir, 160, 7, 90);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DRAGON_COLD:
		{
			fire_arc(GF_COLD, dir, 160, 7, 90);
			*ident = TRUE;
			return TRUE;
		}

	case EF_DRAGON_BREATH:
		{
			int tmp = randint1(5);

			if (tmp == 1)
				fire_arc(GF_ACID, dir, 200, 9, 90);
			if (tmp == 2)
				fire_arc(GF_ELEC, dir, 180, 9, 90);
			if (tmp == 3)
				fire_arc(GF_COLD, dir, 190, 9, 90);
			if (tmp == 4)
				fire_arc(GF_FIRE, dir, 210, 9, 90);
			if (tmp == 5)
				fire_arc(GF_POIS, dir, 200, 7, 120);

			*ident = TRUE;
			return TRUE;
		}

	case EF_ANNIHILATION:
		{
			if (drain_life(dir, 100 + randint1(plev * 4)))
				*ident = TRUE;
			return TRUE;
		}

	case EF_STRIKING:
		{
			fire_bolt(GF_METEOR, dir, damroll(10 + plev / 3, 9));
			*ident = TRUE;
			return TRUE;
		}

	case EF_STORMS:
		{
			fire_bolt(GF_STORM, dir, damroll(25 + plev / 5, 4));
			*ident = TRUE;
			return TRUE;
		}


	case EF_SHARD_BOLT:
		{
			fire_bolt(GF_SHARD, dir, damroll(4 + plev / 14, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_ILKORIN:
		{
			TARGET_DECLARE
				msg("Deadly venom spurts and steams from your wand.");
			TARGET_PRESERVE fire_bolt(GF_POIS, dir, damroll(plev / 2, 11));
			TARGET_RESTORE fire_cloud(GF_POIS, dir, 30, 6);
			*ident = TRUE;
			return TRUE;
		}

	case EF_BEGUILING:
		{
			msg("You speak soft, beguiling words.");

			if (randint0(2) == 0) {
				if (slow_monster(dir, plev * 2))
					*ident = TRUE;
			}
			if (randint0(2) == 0) {
				if (confuse_monster(dir, plev * 2))
					*ident = TRUE;
			} else {
				if (sleep_monster(dir, plev * 2))
					*ident = TRUE;
			}

			*ident = TRUE;
			return TRUE;
		}

	case EF_UNMAKING:
		{
			msg("You envoke the powers of Unmaking!");
			unmake(dir);
			*ident = TRUE;
			return TRUE;
		}

	case EF_OSSE:
		{
			msg("You raise a foam-crested tidal wave.");
			fire_arc(GF_WATER, dir, 3 * plev + randint1(100), 14, 90);
			*ident = TRUE;
			return TRUE;
		}
	case EF_RESTORATION:
		{
			if (restore_level())
				*ident = TRUE;
			if (do_res_stat(A_STR))
				*ident = TRUE;
			if (do_res_stat(A_INT))
				*ident = TRUE;
			if (do_res_stat(A_WIS))
				*ident = TRUE;
			if (do_res_stat(A_DEX))
				*ident = TRUE;
			if (do_res_stat(A_CON))
				*ident = TRUE;
			if (do_res_stat(A_CHR))
				*ident = TRUE;
			return TRUE;
		}

	case EF_TELEPORT_AWAY1:
		{
			if (teleport_monster(dir, 45 + (plev / 3)))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SLEEP_MONSTER1:
		{
			if (sleep_monster(dir, plev + 10))
				*ident = TRUE;
			return TRUE;
		}

	case EF_SLOW_MONSTER1:
		{
			if (slow_monster(dir, plev + 10))
				*ident = TRUE;
			return TRUE;
		}

	case EF_DRAIN_LIFE2:
		{
			if (drain_life(dir, 45 + 3 * plev / 2))
				*ident = TRUE;
			return TRUE;
		}

	case EF_ACID_BOLT2:
		{
			fire_bolt(GF_ACID, dir, damroll(6 + plev / 10, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_ELEC_BOLT2:
		{
			fire_bolt(GF_ELEC, dir, damroll(4 + plev / 14, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_FIRE_BOLT2:
		{
			fire_bolt(GF_FIRE, dir, damroll(7 + plev / 8, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_COLD_BOLT2:
		{
			fire_bolt(GF_COLD, dir, damroll(5 + plev / 12, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_ACID_BALL2:
		{
			fire_ball(GF_ACID, dir, 60 + 4 * plev / 5, 1, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_ELEC_BALL2:
		{
			fire_ball(GF_ELEC, dir, 40 + 4 * plev / 5, 1, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_FIRE_BALL2:
		{
			fire_ball(GF_FIRE, dir, 70 + 4 * plev / 5, 1, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_COLD_BALL2:
		{
			fire_ball(GF_COLD, dir, 50 + 4 * plev / 5, 1, FALSE);
			*ident = TRUE;
			return TRUE;
		}

	case EF_LIGHTINGSTRIKE:
		{
			fire_bolt_or_beam(plev / 2 - 10, GF_ELEC, dir,
							  damroll(18 + plev / 3, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_NORTHWINDS:
		{
			fire_bolt_or_beam(plev / 2 - 10, GF_COLD, dir,
							  damroll(21 + plev / 3, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_DRAGONFIRE:
		{
			fire_bolt_or_beam(plev / 2 - 10, GF_FIRE, dir,
							  damroll(24 + plev / 3, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_GLAURUNGS:
		{
			fire_bolt_or_beam(plev / 2 - 10, GF_ACID, dir,
							  damroll(27 + plev / 3, 8));
			*ident = TRUE;
			return TRUE;
		}

	case EF_DELVING:
		{
			s16b ty, tx;
			target_get(&tx, &ty);

			/* Aimed at oneself, this rod creates a room. */
			if ((dir == 5) && (ty == p_ptr->py) && (tx == p_ptr->px)) {
				/* Lots of damage to creatures of stone. */
				fire_sphere(GF_KILL_WALL, 0, 300, 4, 20);
			}

			/* Otherwise, an extremely powerful destroy wall/stone. */
			else {
				extern bool wall_to_mud_hack(int dir, int dam);
				(void) wall_to_mud_hack(dir, 160 + randint1(240));
			}
			*ident = TRUE;
			return TRUE;
		}

	case EF_SHADOW:
		{
			/* Hack - Extra good for those who backstab. */
			if (player_has(PF_BACKSTAB)) {
				if (p_ptr->timed[TMD_SSTEALTH])
					(void) inc_timed(TMD_SSTEALTH, 30, FALSE);
				else
					(void) inc_timed(TMD_SSTEALTH, 75, TRUE);
			} else {
				if (p_ptr->timed[TMD_SSTEALTH])
					(void) inc_timed(TMD_SSTEALTH, 20, FALSE);
				else
					(void) inc_timed(TMD_SSTEALTH, 50, TRUE);
			}
			*ident = TRUE;
			return TRUE;
		}

	case EF_AIR:
		{
			msg("You raise your rod skyward and call upon the powers of Air.");
			ele_air_smite();
			*ident = TRUE;
			return TRUE;
		}

	case EF_PORTALS:
		{
			msg("Choose a location to teleport to.");
			message_flush();
			dimen_door();
			*ident = TRUE;
			return TRUE;
		}



	case EF_GWINDOR:
		{
			light_area(damroll(2, 15), 3);
			return TRUE;
		}
	case EF_DWARVES:
		{
			wiz_light(FALSE);
			(void) detect_all(DUNGEON_WID, TRUE);
			return TRUE;
		}
	case EF_ELESSAR:
		{
			msg("You feel a warm tingling inside...");
			(void) hp_player(500);
			(void) clear_timed(TMD_CUT, TRUE);
			restore_level();
			return TRUE;
		}
	case EF_RAZORBACK:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become Dragonking of Storms.");
				shapechange(SHAPE_WYRM);
			} else {
				msg("You are surrounded by lightning...");
				for (k = 0; k < 8; k++)
					fire_ball(GF_ELEC, ddd[k], 150, 3, FALSE);
			}
			return TRUE;
		}
	case EF_BLADETURNER:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become an Avatar of Dragonkind.");
				shapechange(SHAPE_WYRM);
			} else {
				msg("Your scales glow many colours...");
				(void) hp_player(30);
				(void) clear_timed(TMD_AFRAID, TRUE);
				(void) inc_timed(TMD_SHERO, randint1(50) + 50, TRUE);
				(void) inc_timed(TMD_BLESSED, randint1(50) + 50, TRUE);
				(void) inc_timed(TMD_OPP_ACID, randint1(50) + 50, TRUE);
				(void) inc_timed(TMD_OPP_ELEC, randint1(50) + 50, TRUE);
				(void) inc_timed(TMD_OPP_FIRE, randint1(50) + 50, TRUE);
				(void) inc_timed(TMD_OPP_COLD, randint1(50) + 50, TRUE);
				(void) inc_timed(TMD_OPP_POIS, randint1(50) + 50, TRUE);
			}
			return TRUE;
		}
	case EF_SOULKEEPER:
		{
			msg("You feel much better...");
			(void) hp_player(1000);
			(void) clear_timed(TMD_CUT, TRUE);
			return TRUE;
		}
	case EF_ELEMENTS:
		{
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_ELEC, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_FIRE, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			return TRUE;
		}

	case EF_GIL_GALAD:
		{
			fire_sphere(GF_LIGHT, 0, 75, 6, 20);
			confu_monsters(3 * plev / 2);
			return TRUE;
		}

	case EF_NARGOTHROND:
		{
			msg("You feel a warm tingling inside...");
			(void) hp_player(500);
			(void) clear_timed(TMD_CUT, TRUE);
			return TRUE;
		}

	case EF_VALINOR:
		{
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_ELEC, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_FIRE, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_POIS, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_HOLCOLLETH:
		{
			sleep_monsters_touch(3 * plev / 2 + 10);
			return TRUE;
		}
	case EF_THINGOL:
		{
			recharge(180);
			return TRUE;
		}
	case EF_MAEGLIN:
		{
			fire_bolt(GF_SPIRIT, dir, damroll(9, 8));
			take_hit(damroll(1, 6), "the dark arts");
			return TRUE;
		}
	case EF_PAURNIMMEN:
		{
			set_ele_attack(ATTACK_COLD, 50);
			return TRUE;
		}
	case EF_PAURNEN:
		{
			set_ele_attack(ATTACK_ACID, 30);
			return TRUE;
		}
	case EF_DAL:
		{
			(void) clear_timed(TMD_AFRAID, TRUE);
			(void) clear_timed(TMD_POISONED, TRUE);
			return TRUE;
		}

	case EF_NARTHANC:
		{
			fire_bolt(GF_FIRE, dir, damroll(6, 8));
			return TRUE;
		}
	case EF_NIMTHANC:
		{
			fire_bolt(GF_COLD, dir, damroll(5, 8));
			return TRUE;
		}
	case EF_DETHANC:
		{
			fire_bolt(GF_ELEC, dir, damroll(4, 8));
			return TRUE;
		}
	case EF_RILIA:
		{
			fire_ball(GF_POIS, dir, 12, 3, FALSE);
			return TRUE;
		}
	case EF_BELANGIL:
		{
			fire_ball(GF_COLD, dir, 3 * p_ptr->lev / 2, 2, FALSE);
			return TRUE;
		}
	case EF_ARANRUTH:
		{
			fire_bolt(GF_COLD, dir, damroll(12, 8));
			return TRUE;
		}
	case EF_RINGIL:
		{
			fire_arc(GF_ICE, dir, 250, 10, 40);
			return TRUE;
		}
	case EF_NARSIL:
		{
			fire_ball(GF_FIRE, dir, 150, 2, FALSE);
			return TRUE;
		}

	case EF_MANWE:
		{
			fire_arc(GF_FORCE, dir, 300, 10, 180);
			return TRUE;
		}

	case EF_AEGLOS:
		{
			fire_ball(GF_COLD, dir, 100, 2, FALSE);
			return TRUE;
		}
	case EF_LOTHARANG:
		{
			hp_player(damroll(4, 12));
			(void) dec_timed(TMD_CUT, p_ptr->timed[TMD_CUT] / 2 + 50,
							 TRUE);
			return TRUE;
		}
	case EF_ULMO:
		{
			teleport_monster(dir, 45 + (plev / 3));
			return TRUE;
		}
	case EF_AVAVIR:
		{
			if (!word_recall(randint0(20) + 15))
				return FALSE;
			return TRUE;
		}
	case EF_TOTILA:
		{
			confuse_monster(dir, 3 * plev / 2 + 5);
			return TRUE;
		}

	case EF_FIRESTAR:
		{
			fire_ball(GF_FIRE, dir, 125, 3, FALSE);
			return TRUE;
		}
	case EF_TURMIL:
		{
			drain_life(dir, 90);
			return TRUE;
		}

		/* Activations for dragon scale mails. */
	case EF_DRAGON_BLACK:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become an acidic dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_ACID);
				msg("You breathe acid.");
				fire_arc(GF_ACID, dir, (plev / 10 + 1) * 45, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_BLUE:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a storm dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_ELEC);
				msg("You breathe lightning.");
				fire_arc(GF_ELEC, dir, (plev / 10 + 1) * 40, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_WHITE:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become an icy dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				msg("You breathe frost.");
				fire_arc(GF_COLD, dir, (plev / 10 + 1) * 45, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_RED:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a fire dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_FIRE);
				msg("You breathe fire.");
				fire_arc(GF_FIRE, dir, (plev / 10 + 1) * 50, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_GREEN:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a poisonous dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_GAS);
				msg("You breathe poison gas.");
				fire_arc(GF_POIS, dir, (plev / 10 + 1) * 45, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_MULTIHUED:
		{
			static const struct {
				int sound;
				const char *msg;
				int typ;
			} mh[] = {
				{
				MSG_BR_ELEC, "lightning", GF_ELEC}, {
				MSG_BR_FROST, "frost", GF_COLD}, {
				MSG_BR_ACID, "acid", GF_ACID}, {
				MSG_BR_GAS, "poison gas", GF_POIS}, {
				MSG_BR_FIRE, "fire", GF_FIRE}
			};

			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a powerful dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				chance = randint0(5);
				sound(mh[chance].sound);
				msg("You breathe %s.",
					((chance == 1) ? "lightning" :
					 ((chance == 2) ? "frost" :
					  ((chance == 3) ? "acid" :
					   ((chance == 4) ? "poison gas" : "fire")))));
				fire_arc(((chance == 1) ? GF_ELEC :
						  ((chance == 2) ? GF_COLD :
						   ((chance == 3) ? GF_ACID :
							((chance == 4) ? GF_POIS : GF_FIRE)))),
						 dir, (plev / 10 + 1) * 60, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_SHINING:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a glowing dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				chance = randint0(2);
				sound(((chance == 0 ? MSG_BR_LIGHT : MSG_BR_DARK)));
				msg("You breathe %s.",
					((chance == 0 ? "light" : "darkness")));
				fire_arc((chance == 0 ? GF_LIGHT : GF_DARK), dir,
						 (plev / 10 + 1) * 50, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_LAW:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a dragon of Order.");
				shapechange(SHAPE_WYRM);
			} else {
				chance = randint0(2);
				sound(((chance == 1 ? MSG_BR_SOUND : MSG_BR_SHARDS)));
				msg("You breathe %s.",
					((chance == 1 ? "sound" : "shards")));
				fire_arc((chance == 1 ? GF_SOUND : GF_SHARD), dir,
						 (plev / 10 + 1) * 60, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_BRONZE:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a mystifying dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_CONF);
				msg("You breathe confusion.");
				fire_arc(GF_CONFU, dir, (plev / 10 + 1) * 40, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_GOLD:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a dragon with a deafening roar.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_SOUND);
				msg("You breathe sound.");
				fire_arc(GF_SOUND, dir, (plev / 10 + 1) * 40, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_CHAOS:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a dragon of Chaos.");
				shapechange(SHAPE_WYRM);
			} else {
				chance = randint0(2);
				sound(((chance == 1 ? MSG_BR_CHAOS : MSG_BR_DISEN)));
				msg("You breathe %s.",
					((chance == 1 ? "chaos" : "disenchantment")));
				fire_arc((chance == 1 ? GF_CHAOS : GF_DISEN), dir,
						 (plev / 10 + 1) * 55, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_BALANCE:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a dragon of Balance.");
				shapechange(SHAPE_WYRM);
			} else {
				chance = randint0(4);
				sound(((chance == 1) ? MSG_BR_CHAOS :
					   ((chance == 2) ? MSG_BR_DISEN :
						((chance == 3) ? MSG_BR_SOUND : MSG_BR_SHARDS))));
				msg("You breathe %s.",
					((chance == 1) ? "chaos" :
					 ((chance == 2) ? "disenchantment" :
					  ((chance == 3) ? "sound" : "shards"))));
				fire_arc(((chance == 1) ? GF_CHAOS :
						  ((chance == 2) ? GF_DISEN :
						   ((chance == 3) ? GF_SOUND : GF_SHARD))),
						 dir, (plev / 10 + 1) * 65, 10, 40);
			}
			return TRUE;
		}
	case EF_DRAGON_POWER:
		{
			if ((p_ptr->schange) != SHAPE_WYRM) {
				msg("You become a wonderous dragon.");
				shapechange(SHAPE_WYRM);
			} else {
				sound(MSG_BR_ELEMENTS);
				msg("You breathe the elements.");
				fire_arc(GF_ALL, dir, (plev / 10 + 1) * 75, 10, 40);
			}
			return TRUE;
		}

		/* Activations for rings. */
	case EF_RING_ACID:
		{
			fire_ball(GF_ACID, dir, 45 + 3 * plev / 2, 3, FALSE);
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_RING_ELEC:
		{
			fire_ball(GF_ELEC, dir, 45 + 3 * plev / 2, 3, FALSE);
			(void) inc_timed(TMD_OPP_ELEC, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_RING_FIRE:
		{
			fire_ball(GF_FIRE, dir, 45 + 3 * plev / 2, 3, FALSE);
			(void) inc_timed(TMD_OPP_FIRE, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_RING_COLD:
		{
			fire_ball(GF_COLD, dir, 45 + 3 * plev / 2, 3, FALSE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_RING_POIS:
		{
			fire_ball(GF_POIS, dir, 45 + 3 * plev / 2, 3, FALSE);
			(void) inc_timed(TMD_OPP_POIS, randint1(20) + 20, TRUE);
			return TRUE;
		}


		/* Activations for amulets. */
	case EF_AMULET_ESCAPING:
		{
			teleport_player(40, TRUE);
			return TRUE;
		}

	case EF_AMULET_LION:
		{
			/* Already a Lion */
			if (p_ptr->schange == SHAPE_LION)
				return TRUE;

			msg("You become a fierce Lion.");
			shapechange(SHAPE_LION);
			return TRUE;
		}

	case EF_AMULET_METAMORPH:
		{
			/* Already changed */
			if (p_ptr->schange)
				return TRUE;

			shapechange(randint1(MAX_SHAPE));
			return TRUE;
		}


		/* Activations for random artifacts, and available for use elsewhere. */
	case EF_RAND_FIRE1:
		{
			msg("You launch a bolt of fire.");
			fire_bolt(GF_FIRE, dir, damroll(3 + plev / 8, 8));
			return TRUE;
		}
	case EF_RAND_FIRE2:
		{
			msg("You feel a sphere of fire form between your hands.");
			fire_sphere(GF_FIRE, dir, 90, 1, 20);
			return TRUE;
		}
	case EF_RAND_FIRE3:
		{
			msg("The fires of Anor rise in wrath!");
			fire_sphere(GF_FIRE, 0, 150, 5, 20);
			return TRUE;
		}
	case EF_RAND_COLD1:
		{
			msg("You launch a bolt of frost.");
			fire_bolt(GF_COLD, dir, damroll(3 + plev / 8, 8));
			return TRUE;
		}
	case EF_RAND_COLD2:
		{
			msg("You hurl a sphere of killing frost.");
			fire_sphere(GF_COLD, dir, 90, 1, 20);
			return TRUE;
		}
	case EF_RAND_COLD3:
		{
			msg("A wild Northland frost storms uncontrollably!");
			fire_sphere(GF_COLD, 0, 150, 5, 20);
			return TRUE;
		}
	case EF_RAND_ACID1:
		{
			msg("You launch a bolt of acid.");
			fire_bolt(GF_ACID, dir, damroll(3 + plev / 8, 8));
			return TRUE;
		}
	case EF_RAND_ACID2:
		{
			msg("A sphere of deadly acid forms upon your hand.");
			fire_sphere(GF_ACID, dir, 90, 1, 20);
			return TRUE;
		}
	case EF_RAND_ACID3:
		{
			msg("A tornado of acid melts armour and flesh!");
			fire_sphere(GF_ACID, 0, 160, 3, 20);
			return TRUE;
		}
	case EF_RAND_ELEC1:
		{
			msg("You launch a bolt of electricity.");
			fire_bolt(GF_ELEC, dir, damroll(3 + plev / 8, 8));
			return TRUE;
		}
	case EF_RAND_ELEC2:
		{
			msg("You summon ball lightning to your aid.");
			fire_sphere(GF_ELEC, dir, 90, 1, 20);
			return TRUE;
		}
	case EF_RAND_ELEC3:
		{
			msg("A massive stroke of lightning smites the ground!");
			fire_sphere(GF_ELEC, 0, 130, 2, 20);
			msg("Boom!");
			fire_sphere(GF_SOUND, 0, 25, 9, 20);
			return TRUE;
		}
	case EF_RAND_POIS1:
		{
			msg("You launch a poison dart.");
			fire_bolt(GF_POIS, dir, damroll(3 + plev / 10, 8));
			return TRUE;
		}
	case EF_RAND_POIS2:
		{
			msg("Deadly gases blanket the area.");
			fire_sphere(GF_POIS, 0, 110, 9, 30);
			return TRUE;
		}
	case EF_RAND_LIGHT1:
		{
			TARGET_DECLARE msg("You throw a radiant sphere...");
			TARGET_PRESERVE fire_ball(GF_LIGHT, dir, 50, 0, FALSE);
			TARGET_RESTORE fire_ball(GF_CONFU, dir, 10, 0, FALSE);
			return TRUE;
		}
	case EF_RAND_LIGHT2:
		{
			msg("You bathe the area in radiant light!");
			dispel_light_hating(175);
			return TRUE;
		}
	case EF_RAND_DISPEL_UNDEAD:
		{
			msg("A tide of life surrounds you!");
			(void) dispel_undead(100);
			return TRUE;
		}
	case EF_RAND_DISPEL_EVIL:
		{
			msg("A wave of goodness washes over you...");
			(void) dispel_evil(100);

			if (player_has(PF_EVIL)) {
				msg("Your black soul is hit!");
				take_hit(25, "struck down by Good");
			}
			return TRUE;
		}
	case EF_RAND_SMITE_UNDEAD:
		{
			msg("Spells to dispel an undead antagonist surround you...");
			dispel_an_undead(dir, damroll(plev / 4, 33));
			return TRUE;
		}
	case EF_RAND_SMITE_DEMON:
		{
			msg("Spells to dispel a demonic adversary surround you...");
			dispel_a_demon(dir, damroll(plev / 4, 33));
			return TRUE;
		}
	case EF_RAND_SMITE_DRAGON:
		{
			msg("Spells to dispel a dragonic foe surround you...");
			dispel_a_dragon(dir, damroll(plev / 4, 33));
			return TRUE;
		}
	case EF_RAND_HOLY_ORB:
		{
			msg("A cleansing ball materializes on your fingertips.");
			fire_sphere(GF_HOLY_ORB, dir, 60, 1, 20);
			return TRUE;
		}
	case EF_RAND_BLESS:
		{
			msg("You feel blessed for battle.");
			if (!p_ptr->timed[TMD_BLESSED]) {
				(void) inc_timed(TMD_BLESSED, randint1(24) + 24, TRUE);
			} else {
				(void) inc_timed(TMD_BLESSED, randint1(12) + 12, TRUE);
			}
			return TRUE;
		}
	case EF_RAND_FRIGHTEN_ALL:
		{
			msg("You reveal yourself in wrath; your enemies tremble!");
			(void) fear_monsters((3 * plev / 2) + 5);
			return TRUE;
		}
	case EF_RAND_HEAL1:
		{
			(void) hp_player(damroll(5, 20));
			(void) dec_timed(TMD_CUT, 5, TRUE);
			(void) dec_timed(TMD_POISONED, 5, TRUE);
			return TRUE;
		}
	case EF_RAND_HEAL2:
		{
			(void) hp_player(damroll(7, 40));
			(void) dec_timed(TMD_CUT, p_ptr->timed[TMD_CUT] / 2 + 5, TRUE);
			(void) dec_timed(TMD_POISONED,
							 p_ptr->timed[TMD_POISONED] / 2 + 5, TRUE);
			return TRUE;
		}
	case EF_RAND_HEAL3:
		{
			(void) hp_player(damroll(10, 60));
			(void) clear_timed(TMD_CUT, TRUE);
			(void) clear_timed(TMD_POISONED, TRUE);
			return TRUE;
		}
	case EF_RAND_CURE:
		{
			msg("Tender hands massage your hurts away.");
			(void) clear_timed(TMD_BLIND, TRUE);
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) clear_timed(TMD_CONFUSED, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			(void) do_res_stat(A_CON);
			return TRUE;
		}
	case EF_RAND_PROT_FROM_EVIL:
		{
			msg("A shrill wail surrounds you.");

			if (!p_ptr->timed[TMD_PROTEVIL]) {
				(void) inc_timed(TMD_PROTEVIL, randint1(24) + 24, TRUE);
			} else {
				(void) inc_timed(TMD_PROTEVIL, randint1(30), TRUE);
			}
			msg("You feel somewhat safer.");
			return TRUE;
		}
	case EF_RAND_CHAOS:
		{
			msg("You unleash the powers of Unmaking!");
			fire_ball(GF_CHAOS, dir, randint1(320), 2, FALSE);
			return TRUE;
		}
	case EF_RAND_SHARD_SOUND:
		{
			msg("You invoke the powers of Law...");
			if (randint1(2) == 1) {
				msg("...and razor-sharp obsidian chips hail upon your foes!");
				fire_ball(GF_SHARD, dir, 150, 4, FALSE);
			} else {
				msg("...and an awful cacophony shakes %s!",
					locality_name[stage_map[p_ptr->stage][LOCALITY]]);
				fire_ball(GF_SOUND, dir, 150, 4, FALSE);
			}
			return TRUE;
		}
	case EF_RAND_NETHR:
		{
			msg("You cast a gleaming orb of midnight hue!");
			fire_sphere(GF_NETHER, dir, 100, 1, 20);
			return TRUE;
		}
	case EF_RAND_LINE_LIGHT:
		{
			msg("A line of shimmering yellow light appears.");
			light_line(dir);
			return TRUE;
		}
	case EF_RAND_STARLIGHT:
		{
			msg("Light radiates outward in all directions.");
			for (k = 0; k < 8; k++)
				light_line(ddd[k]);
			return TRUE;
		}
	case EF_RAND_EARTHQUAKE:
		{
			msg("You strike the floor, and the earth crumbles!");
			earthquake(p_ptr->py, p_ptr->px, 10, FALSE);
			return TRUE;
		}
	case EF_RAND_SPEED:
		{
			msg("All around you move with dreamlike slowness.");
			if (!p_ptr->timed[TMD_FAST]) {
				(void) set_timed(TMD_FAST, randint1(20) + 20, FALSE);
			} else {
				(void) set_timed(TMD_FAST, 5, FALSE);
			}
			return TRUE;
		}
	case EF_RAND_TELEPORT_AWAY:
		{
			msg("You weave a pattern of rejection and denial.");
			(void) teleport_monster(dir, 55 + (plev / 2));
			return TRUE;
		}
	case EF_RAND_HEROISM:
		{
			msg("A thrilling battle song awakes the warrior within you!");
			(void) hp_player(10);
			(void) clear_timed(TMD_AFRAID, TRUE);
			(void) inc_timed(TMD_HERO, randint1(25) + 25, TRUE);
			return TRUE;
		}
	case EF_RAND_STORM_DANCE:
		{
			msg("Wild music plays, and you dance up a storm...");
			fire_sphere(GF_SOUND, 0, 24, 8, 20);
			fire_sphere(GF_SHARD, 0, 32, 8, 20);
			fire_sphere(GF_CONFU, 0, 8, 8, 20);

			if (randint1(2) == 1) {
				msg("Your wild movements exhaust you!");
				take_hit(damroll(1, 12), "danced to death");
			}
			return TRUE;
		}
	case EF_RAND_RESIST_ELEMENTS:
		{
			msg("Quadricolored magics swirl around you protectingly.");
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_ELEC, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_FIRE, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_RAND_RESIST_ALL:
		{
			msg("Penticolored magics swirl around you protectingly.");
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_ELEC, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_FIRE, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_POIS, randint1(20) + 20, TRUE);
			return TRUE;
		}
	case EF_RAND_TELEPORT1:
		{
			msg("You pass through a transparent gateway...");
			teleport_player(30, TRUE);
			return TRUE;
		}
	case EF_RAND_TELEPORT2:
		{
			msg("Time and space twist about you...");
			teleport_player(200, TRUE);
			return TRUE;
		}
	case EF_RAND_RECALL:
		{
			if (!word_recall(randint0(20) + 15))
				return TRUE;
			return TRUE;
		}
	case EF_RAND_REGAIN:
		{
			msg("Surrounded by darkness, you envoke light and beauty.");
			msg("Your spirit regains its natural vitality.");

			(void) restore_level();
			return TRUE;
		}
	case EF_RAND_RESTORE:
		{
			msg("A multicolored mist surounds you, restoring body and mind.");
			(void) do_res_stat(A_STR);
			(void) do_res_stat(A_INT);
			(void) do_res_stat(A_WIS);
			(void) do_res_stat(A_DEX);
			(void) do_res_stat(A_CON);
			(void) do_res_stat(A_CHR);
			return TRUE;
		}
	case EF_RAND_SHIELD:
		{
			msg("Magics coalesce to form a shimmering barrier.");
			if (!p_ptr->timed[TMD_SHIELD]) {
				(void) inc_timed(TMD_SHIELD, randint1(25) + 25, TRUE);
			} else {
				(void) inc_timed(TMD_SHIELD, randint1(15) + 15, TRUE);
			}
			return TRUE;
		}

	case EF_RAND_SUPER_SHOOTING:
		{
			char *missile_name;
			object_type *o_ptr = &p_ptr->inventory[INVEN_BOW];

			/* Get the correct name for the missile, if possible. */
			missile_name = "missile";
			if ((o_ptr->sval == SV_LIGHT_XBOW)
				|| (o_ptr->sval == SV_HEAVY_XBOW))
				missile_name = "bolt";
			if ((o_ptr->sval == SV_LONG_BOW)
				|| (o_ptr->sval == SV_LONG_BOW))
				missile_name = "arrow";
			if (o_ptr->sval == SV_SLING)
				missile_name = "shot";

			msg("The %s you have ready to hand gleams with deadly power.",
				missile_name);
			p_ptr->special_attack |= (ATTACK_SUPERSHOT);

			/* Redraw the state */
			p_ptr->redraw |= (PR_STATUS);

			return TRUE;
		}
	case EF_RAND_DETECT_MONSTERS:
		{
			msg("You search for monsters.");
			(void) detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
			return TRUE;
		}
	case EF_RAND_DETECT_EVIL:
		{
			msg("You hunt for evil creatures...");
			(void) detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
			return TRUE;
		}
	case EF_RAND_DETECT_ALL:
		{
			msg("You sense the area around you.");
			detect_all(DETECT_RAD_DEFAULT, TRUE);
			return TRUE;
		}
	case EF_RAND_MAGIC_MAP:
		{
			msg("A mental image of your surroundings is fixed in your mind.");
			map_area(0, 0, FALSE);
			return TRUE;
		}
	case EF_RAND_DETECT_D_S_T:
		{
			/* Hack - 'show' effected region only with the first detect */
			msg("The secrets of traps and doors are revealed.");
			(void) detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_doors(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, FALSE);
			return TRUE;
		}
	case EF_RAND_CONFU_FOE:
		{
			msg("You chant runes of confusing...");
			if (confuse_monster(dir, 5 * plev / 3))
				msg("...which utterly baffle your foe!");
			return TRUE;
		}
	case EF_RAND_SLEEP_FOE:
		{
			msg("A fine dust appears in your hand, and you throw it...");
			if (sleep_monster(dir, 5 * plev / 3))
				msg("...sending a foe to the realm of dreams!");
			return TRUE;
		}
	case EF_RAND_TURN_FOE:
		{
			msg("You lock eyes with an enemy...");
			if (fear_monster(dir, 5 * plev / 3))
				msg("...and break his courage!");
			return TRUE;
		}
	case EF_RAND_SLOW_FOE:
		{
			msg("You focus on the mind of an opponent...");
			if (slow_monster(dir, 5 * plev / 3))
				msg("...and sap his strength!");
			return TRUE;
		}
	case EF_RAND_BANISH_EVIL:
		{
			msg("A mighty hand drives your foes from you!");
			(void) banish_evil(80);
			return TRUE;
		}
	case EF_RAND_DISARM:
		{
			msg("You feel skilled hands guiding your disarming.");
			(void) disarm_trap(dir);
			return TRUE;
		}
	case EF_RAND_CONFU_FOES:
		{
			msg("You intone a bewildering hex...");
			if (confu_monsters(3 * plev / 2))
				msg("...which utterly baffles your foes!");
			return TRUE;
		}
	case EF_RAND_SLEEP_FOES:
		{
			msg("Soft, soothing music washes over you..");
			if (sleep_monsters(3 * plev / 2))
				msg("...and sends your enemies to sleep!");
			return TRUE;
		}
	case EF_RAND_TURN_FOES:
		{
			msg("You reveal yourself in wrath; your enemies tremble!");
			(void) fear_monsters(3 * plev / 2);
			return TRUE;
		}
	case EF_RAND_SLOW_FOES:
		{
			msg("An opaque cloud blankets the area...");
			if (slow_monsters(3 * plev / 2))
				msg("...and dissipates, along with your opponents' strength!");
			else
				msg("...and dissipates without effect.");
			return TRUE;
		}

		/* Activations for rings of power */
	case EF_ACID_BLAST:
		{
			/* Acid, confusion, fear */
			msg("A blast of corrosion strikes your foes!");
			fire_sphere(GF_ACID, 0, randint1(100) + 100, 5, 20);
			(void) confu_monsters(100);
			(void) fear_monsters(100);

			return TRUE;
		}
	case EF_CHAIN_LIGHTNING:
		{
			int strikes = 2 + randint1(10);
			int y, x, target;
			int targ_y, targ_x;
			int cur_y = p_ptr->py, cur_x = p_ptr->px, cur_mon = -1;
			int flag = PROJECT_STOP | PROJECT_KILL;
			int avail_mon[100], avail_mon_num;

			msg("The lightning of Manwe leaps from your hands!");

			/* Initialise */
			for (k = 0; k < 100; k++)
				avail_mon[k] = 0;
			targ_y = cur_y;
			targ_x = cur_x;

			/* Start striking */
			for (k = 0; k < strikes; k++) {
				/* No targets yet */
				avail_mon_num = 0;

				/* Find something in range */
				for (y = cur_y - 5; y <= cur_y + 5; y++)
					for (x = cur_x - 5; x <= cur_x + 5; x++) {
						int dist = distance(cur_y, cur_x, y, x);

						/* Skip distant grids */
						if (dist > 5)
							continue;

						/* Skip grids that are out of bounds */
						if (!in_bounds_fully(y, x))
							continue;

						/* Skip grids that are not projectable */
						if (projectable(cur_y, cur_x, y, x, flag) !=
							PROJECT_CLEAR)
							continue;

						/* Skip grids with no monster (including player) */
						if (!cave_m_idx[y][x])
							continue;

						/* Record the monster */
						avail_mon[avail_mon_num++] = cave_m_idx[y][x];
					}

				/* Maybe we're at a dead end */
				if (!avail_mon_num)
					return TRUE;

				/* Pick a target... */
				target = randint0(avail_mon_num);
				if (avail_mon[target] == -1) {
					targ_y = p_ptr->py;
					targ_x = p_ptr->px;
				} else {
					targ_y = m_list[avail_mon[target]].fy;
					targ_x = m_list[avail_mon[target]].fx;
				}

				/* Paranoia */
				if (!cave_m_idx[targ_y][targ_x])
					return TRUE;

				/* ...and hit it */
				project(cur_mon, 1, targ_y, targ_x, damroll(30, 4),
						GF_ELEC, flag, 0, 0);

				/* Set current monster (may be dead) */
				cur_y = targ_y;
				cur_x = targ_x;
				cur_mon = avail_mon[target];
			}

			return TRUE;
		}
	case EF_LAVA_POOL:
		{
			int y, x;
			int py = p_ptr->py, px = p_ptr->px;

			msg("Lava wells around you!");

			/* Everything in range */
			for (y = py - 7; y <= py + 7; y++)
				for (x = px - 7; x <= px + 7; x++) {
					int dist = distance(py, px, y, x);
					feature_type *f_ptr = &f_info[cave_feat[y][x]];

					/* Skip distant grids */
					if (dist > 7)
						continue;

					/* Skip grids that are out of bounds */
					if (!in_bounds_fully(y, x))
						continue;

					/* Skip grids that are permanent */
					if (tf_has(f_ptr->flags, TF_PERMANENT))
						continue;

					/* Skip grids in vaults */
					if (sqinfo_has(cave_info[y][x], SQUARE_ICKY))
						continue;

					/* Lava now */
					cave_set_feat(y, x, FEAT_LAVA);
				}

			return TRUE;
		}
	case EF_ICE_WHIRLPOOL:
		{
			/* Unleash a whirlpool. */
			msg("A howling vortex of ice rises in wrath around you.");
			fire_sphere(GF_ICE, 0, randint1(100) + 100, 5, 10);

			/* Whisk around and slow the nearby monsters. */
			fire_ball(GF_AWAY_ALL, 0, 12, 6, FALSE);
			slow_monsters(50);

			return TRUE;
		}
	case EF_GROW_FOREST:
		{
			msg("A leafy forest surrounds you!");
			grow_trees_and_grass(TRUE);
			return TRUE;
		}
	case EF_RESTORE_AND_ENHANCE:
		{
			msg("You feel life flow through your body!");
			restore_level();
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) clear_timed(TMD_BLIND, TRUE);
			(void) clear_timed(TMD_CONFUSED, TRUE);
			(void) clear_timed(TMD_IMAGE, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			(void) do_res_stat(A_STR);
			(void) do_res_stat(A_CON);
			(void) do_res_stat(A_DEX);
			(void) do_res_stat(A_WIS);
			(void) do_res_stat(A_INT);
			(void) do_res_stat(A_CHR);
			hp_player(2000);
			if (p_ptr->black_breath) {
				msg("The hold of the Black Breath on you is broken!");
			}
			p_ptr->black_breath = FALSE;
			(void) clear_timed(TMD_AFRAID, TRUE);
			(void) inc_timed(TMD_HERO, randint1(25) + 25, TRUE);
			(void) inc_timed(TMD_SHERO, randint1(25) + 25, TRUE);
			(void) inc_timed(TMD_BLESSED, randint1(25) + 25, TRUE);
			(void) inc_timed(TMD_PROTEVIL, randint1(25) + 25, TRUE);
			return TRUE;
		}
	case EF_ZONE_OF_CHAOS:
		{
			int y, x;
			int py = p_ptr->py, px = p_ptr->px;

			msg("The forces of chaos surround you!");

			/* Everything in range */
			for (y = py - 10; y <= py + 10; y++)
				for (x = px - 10; x <= px + 10; x++) {
					int dist = distance(py, px, y, x);

					/* Skip distant grids */
					if (dist > 10)
						continue;

					/* 20% chance */
					if (randint1(5) == 1)
						continue;

					/* Hit it */
					(void) fire_meteor(-1, GF_CHAOS, y, x, 50, 0, FALSE);
				}

			return TRUE;
		}
	case EF_PRESSURE_WAVE:
		{
			msg("Your foes are thrown backward!");
			fire_ball(GF_FORCE, 0, 50, 20, FALSE);

			return TRUE;
		}
	case EF_ENERGY_DRAIN:
		{
			int y, x;
			int py = p_ptr->py, px = p_ptr->px;
			monster_type *m_ptr;

			msg("Your foes slow, and you seem to have an eternity to act...");

			/* Everything in range */
			for (y = py - 5; y <= py + 5; y++)
				for (x = px - 5; x <= px + 5; x++) {
					int dist = distance(py, px, y, x);

					/* Skip distant grids */
					if (dist > 5)
						continue;

					/* Skip grids with no monster */
					if (cave_m_idx[y][x] <= 0)
						continue;

					/* Skip grids without LOS */
					if (!player_has_los_bold(y, x))
						continue;

					/* Get the monster */
					m_ptr = &m_list[cave_m_idx[y][x]];

					/* Take the energy */
					p_ptr->energy += m_ptr->energy;
					m_ptr->energy = 0;
				}

			return TRUE;
		}
	case EF_MASS_STASIS:
		{
			msg("You command your foes to be still!");
			hold_all();
			return TRUE;
		}
	case EF_LIGHT_FROM_ABOVE:
		{
			int y, x;
			int py = p_ptr->py, px = p_ptr->px;

			msg("The light of the Valar shines from above!");

			/* Everything in range */
			for (y = py - 10; y <= py + 10; y++)
				for (x = px - 10; x <= px + 10; x++) {
					int dist = distance(py, px, y, x);

					/* Skip distant grids */
					if (dist > 10)
						continue;

					/* Skip grids with no monster */
					if (cave_m_idx[y][x] <= 0)
						continue;

					/* Hit it */
					(void) fire_meteor(-1, GF_LIGHT, y, x, 100, 1, FALSE);
				}

			return TRUE;
		}
	case EF_MASS_POLYMORPH:
		{
			msg("Reality warps...");
			poly_all(p_ptr->depth);

			return TRUE;
		}
	case EF_GRAVITY_WAVE:
		{
			msg("Gravity crushes, then releases, your foes!");
			fire_ball(GF_GRAVITY, 0, 100, 20, FALSE);

			return TRUE;
		}
	case EF_ENLIST_EARTH:
		{
			int m_idx, targ_y, targ_x;
			int targ = target_get_monster();

			msg("You call on the Earth to bring forth allies!");

			/* Must target a monster */
			if (!get_aim_dir(&dir))
				return FALSE;
			if (targ <= 0) {
				msg("You must target a monster.");
				return FALSE;
			}

			targ_y = m_list[targ].fy;
			targ_x = m_list[targ].fx;

			/* Summon golems */
			summon_specific(targ_y, targ_x, FALSE, p_ptr->depth,
							SUMMON_GOLEM);

			/* Hack - make all local golems hostile to the target */
			for (m_idx = 0; m_idx < z_info->m_max; m_idx++) {
				monster_type *m_ptr = &m_list[m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				if ((distance(targ_y, targ_x, m_ptr->fy, m_ptr->fx) < 7)
					&& (r_ptr->d_char = 'g')
					&& (!(rf_has(r_ptr->flags, RF_DRAGON))))
					m_ptr->hostile = targ;
			}

			return TRUE;
		}
	case EF_TELEPORT_ALL:
		{
			teleport_all(80);
			return TRUE;
		}



		/* Activations for ego-items. */
	case EF_BALROG_WHIP:
		{
			object_type *o_ptr = &p_ptr->inventory[INVEN_WIELD];

			/* A crude way to simulate a long-range melee blow... */
			msg("You lash out at a nearby foe.");
			fire_arc(GF_FIRE, dir, damroll(o_ptr->dd, o_ptr->ds), 2, 0);
			return TRUE;
		}

		/* Activation for magestaffs */
	case EF_MAGESTAFF:
		{
			if (p_ptr->csp < p_ptr->msp) {
				p_ptr->csp += 10;
				if (p_ptr->csp > p_ptr->msp) {
					p_ptr->csp = p_ptr->msp;
					p_ptr->csp_frac = 0;
				}
				msg("Magical power flows from your staff.");
				p_ptr->redraw |= (PR_MANA);
			}
			return TRUE;
		}



	default:
		{
			/* Mistake */
			msg("Oops.  Effect type unrecognized.");
			return FALSE;
		}
	}
}
