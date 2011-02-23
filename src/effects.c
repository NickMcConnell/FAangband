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
#include "effects.h"


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
	int save_target_y = 0, save_target_x = 0; \
	bool save_target_set;

#define TARGET_PRESERVE \
	if ((dir == 5) && target_okay() && p_ptr->target_who) \
	{ \
		save_target_y = p_ptr->target_row; \
		save_target_x = p_ptr->target_col; \
		save_target_set = TRUE; \
	} \
	else save_target_set = FALSE;

#define TARGET_RESTORE \
	if (save_target_set && !p_ptr->target_set) \
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
typedef struct
{
	u16b index;          /* Effect index */
	bool aim;            /* Whether the effect requires aiming */
	u16b power;	     /* Power rating for obj-power.c */
	const char *desc;    /* Effect description */
} info_entry;

/*
 * Useful things about effects.
 */
static const info_entry effects[] =
{
	#define EFFECT(x, y, r, z)    { EF_##x, y, r, z },
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


/**
 * The "wonder" effect.
 *
 * Returns TRUE if the effect is evident.
 */
bool effect_wonder(int dir, int die, int beam)
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

	if (die > 100)
	{
		/* above 100 the effect is always visible */
		msg_print("You feel a surge of power!");
		visible = TRUE;
	}

	if (die < 8) visible = clone_monster(dir);
	else if (die < 14) visible = speed_monster(dir);
	else if (die < 26) visible = heal_monster(dir);
	else if (die < 31) visible = poly_monster(dir);
	else if (die < 36)
		visible = fire_bolt_or_beam(beam - 10, GF_MISSILE, dir,
		                            damroll(3 + ((plev - 1) / 5), 4));
	else if (die < 41) visible = confuse_monster(dir, plev, FALSE);
	else if (die < 46) visible = fire_ball(GF_POIS, dir, 20 + (plev / 2), 3);
	else if (die < 51) visible = light_line(dir);
	else if (die < 56)
		visible = fire_beam(GF_ELEC, dir, damroll(3+((plev-5)/6), 6));
	else if (die < 61)
		visible = fire_bolt_or_beam(beam-10, GF_COLD, dir,
		                            damroll(5+((plev-5)/4), 8));
	else if (die < 66)
		visible = fire_bolt_or_beam(beam, GF_ACID, dir,
		                            damroll(6+((plev-5)/4), 8));
	else if (die < 71)
		visible = fire_bolt_or_beam(beam, GF_FIRE, dir,
		                            damroll(8+((plev-5)/4), 8));
	else if (die < 76) visible = drain_life(dir, 75);
	else if (die < 81) visible = fire_ball(GF_ELEC, dir, 30 + plev / 2, 2);
	else if (die < 86) visible = fire_ball(GF_ACID, dir, 40 + plev, 2);
	else if (die < 91) visible = fire_ball(GF_ICE, dir, 70 + plev, 3);
	else if (die < 96) visible = fire_ball(GF_FIRE, dir, 80 + plev, 3);
	/* above 100 'visible' is already true */
	else if (die < 101) drain_life(dir, 100 + plev);
	else if (die < 104) earthquake(py, px, 12);
	else if (die < 106) destroy_area(py, px, 15, TRUE);
	else if (die < 108) banishment();
	else if (die < 110) dispel_monsters(120);
	else /* RARE */
	{
		dispel_monsters(150);
		slow_monsters();
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
bool effect_do(effect_type effect, bool *ident, bool aware, int dir, int beam,
	int boost)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  int dam, chance;
  
  /* ?? Object level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Analyze the food */
  switch (effect)
    {
    case EF_POISON1:
      {
	if (pois_hit(15)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_POISON2:
      {
	if (pois_hit(30)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_BLIND1:
      {
	if (!p_ptr->no_blind)
	  {
	    if (set_blind(p_ptr->blind + randint0(200) + 200))
	      {
		*ident = TRUE;
	      }
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	return TRUE;
      }
      
    case EF_BLIND2:
      {
	if (!p_ptr->no_blind)
	  {
	    if (set_blind(p_ptr->blind + randint0(100) + 100))
	      {
		*ident = TRUE;
	      }
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	return TRUE;
      }
      
    case EF_SCARE:
      {
	if (!p_ptr->no_fear)
	  {
	    if (set_afraid(p_ptr->afraid + randint0(10) + 10))
	      {
		*ident = TRUE;
	      }
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_FEARLESS, 0);
	return TRUE;
      }
      
    case EF_CONFUSE1:
      {
	if (!p_resist_good(P_RES_CONFU))
	  {
	    if (set_confused(p_ptr->confused + randint0(10) + 10))
	      {
		*ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_other(IF_RES_CONFU, 0);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_CONFUSE2:
      {
	if (!p_resist_good(P_RES_CONFU))
	  {
	    if (set_confused(p_ptr->confused + randint0(20) + 15))
	      {
		*ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_other(IF_RES_CONFU, 0);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_HALLUC:
      {
	if (!p_resist_good(P_RES_CHAOS))
	  {
	    if (set_image(p_ptr->image + randint0(250) + 250))
	      {
		*ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_other(IF_RES_CHAOS, 0);
	    *ident = TRUE;
	  }
	return TRUE;
      }
    
    case EF_PARALYSE:
      {
	if (!p_ptr->free_act)
	  {
	    if (set_paralyzed(p_ptr->paralyzed + randint0(10) + 10))
	      {
		*ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_obj(OF_FREE_ACT, 0);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_WEAKNESS:
      {
	take_hit(damroll(6, 6), "poisonous food.");
	(void)do_dec_stat(A_STR);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_SICKNESS:
      {
	take_hit(damroll(6, 6), "poisonous food.");
	(void)do_dec_stat(A_CON);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_STUPIDITY:
      {
	take_hit(damroll(8, 8), "poisonous food.");
	(void)do_dec_stat(A_INT);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_NAIVETY:
      {
	take_hit(damroll(8, 8), "poisonous food.");
	(void)do_dec_stat(A_WIS);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_UNHEALTH:
      {
	take_hit(damroll(10, 10), "poisonous food.");
	(void)do_dec_stat(A_CON);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_DISEASE:
      {
	take_hit(damroll(10, 10), "poisonous food.");
	(void)do_dec_stat(A_STR);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_DETONATIONS:
      {
	msg_print("Massive explosions rupture your body!");
	take_hit(damroll(50, 20), "a potion of Detonation");
	(void)set_stun(p_ptr->stun + 75);
	(void)set_cut(p_ptr->cut + 5000);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_DEATH:
      {
	msg_print("A feeling of Death flows through your body.");
	take_hit(p_ptr->chp, "a potion of Death");
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_SLOWNESS:
      {
	if (set_slow(p_ptr->slow + randint1(25) + 15)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SALT_WATER:
      {
	msg_print("The potion makes you vomit!");
	(void)set_food(PY_FOOD_STARVE - 1);
	(void)set_poisoned(0);
	(void)set_paralyzed(p_ptr->paralyzed + 4);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_SLEEP:
      {
	if (!p_ptr->free_act)
	  {
	    if (set_paralyzed(p_ptr->paralyzed + randint0(4) + 4))
	      {
		*ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_obj(OF_FREE_ACT, 0);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_LOSE_MEMORIES:
      {
	if (!p_ptr->hold_life && (p_ptr->exp > 0))
	  {
	    msg_print("You feel your memories fade.");
	    lose_exp(p_ptr->exp / 4);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_RUINATION:
      {
	msg_print("Your nerves and muscles feel weak and lifeless!");
	take_hit(damroll(5, 10), "a potion of Ruination");
	(void)dec_stat(A_DEX, 25, FALSE);
	(void)dec_stat(A_WIS, 25, FALSE);
	(void)dec_stat(A_CON, 25, FALSE);
	(void)dec_stat(A_STR, 25, FALSE);
	(void)dec_stat(A_CHR, 25, FALSE);
	(void)dec_stat(A_INT, 25, FALSE);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_DEC_STR:
      {
	if (do_dec_stat(A_STR)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DEC_INT:
      {
	if (do_dec_stat(A_INT)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DEC_WIS:
      {
	if (do_dec_stat(A_WIS)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DEC_DEX:
      {
	if (do_dec_stat(A_DEX)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DEC_CON:
      {
	if (do_dec_stat(A_CON)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DEC_CHR:
      {
	if (do_dec_stat(A_CHR)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_AGGRAVATE:
      {
	msg_print("There is a high pitched humming noise.");
	(void) aggravate_monsters(1, FALSE);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_CURSE_ARMOR:
      {
	if (curse_armor()) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURSE_WEAPON:
      {
	if (curse_weapon()) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SUMMON3:
      {
	sound(MSG_SUM_MONSTER);
	for (k = 0; k < randint1(3); k++)
	  {
	    if (summon_specific(py, px, FALSE, p_ptr->depth, 0))
	      {
		*ident = TRUE;
	      }
	  }
	return TRUE;
      }
      
    case EF_SUMMON_UNDEAD:
      {
	sound(MSG_SUM_UNDEAD);
	for (k = 0; k < randint1(3); k++)
	  {
	    if (summon_specific(py, px, FALSE, p_ptr->depth, 
				SUMMON_UNDEAD))
	      {
		*ident = TRUE;
	      }
		      }
	return TRUE;
      }
      
    case EF_TRAP_CREATION:
      {
	if (trap_creation()) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DARKNESS:
      {
	if (!p_ptr->no_blind)
	  {
	    (void)set_blind(p_ptr->blind + 3 + randint1(5));
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	if (unlite_area(10, 3)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SLOWNESS:
      {
	if (set_slow(p_ptr->slow + randint1(30) + 15)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HASTE_MONSTERS:
      {
	if (speed_monsters()) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SUMMON4:
      {
	sound(MSG_SUM_MONSTER);
	for (k = 0; k < randint1(4); k++)
	  {
	    if (summon_specific(py, px, FALSE, p_ptr->depth, 0))
	      {
		*ident = TRUE;
	      }
	  }
	return TRUE;
      }
      
    case EF_HEAL_MONSTER:
      {
	if (heal_monster(dir)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HASTE_MONSTER:
      {
	if (speed_monster(dir)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CLONE_MONSTER:
      {
	if (clone_monster(dir)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_ROUSE_LEVEL:
      {
	msg_print("A mighty blast of horns shakes the air, and you hear stirring everwhere!");
	(void) aggravate_monsters(1, TRUE);
	*ident = TRUE;
	return TRUE;
      }
      
     case EF_CURE_POISON:
      {
	if (set_poisoned(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_BLINDNESS:
      {
	if (set_blind(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_PARANOIA:
      {
	if (set_afraid(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_CONFUSION:
      {
	if (set_confused(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_SMALL:
      {
	if (hp_player(damroll(4, 8))) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RES_STR:
      {
	if (do_res_stat(A_STR)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RES_INT:
      {
	if (do_res_stat(A_INT)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RES_WIS:
      {
	if (do_res_stat(A_WIS)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RES_DEX:
      {
	if (do_res_stat(A_DEX)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RES_CON:
      {
	if (do_res_stat(A_CON)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RES_CHR:
      {
	if (do_res_stat(A_CHR)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RESTORING:
      {
	if (do_res_stat(A_STR)) *ident = TRUE;
	if (do_res_stat(A_INT)) *ident = TRUE;
	if (do_res_stat(A_WIS)) *ident = TRUE;
	if (do_res_stat(A_DEX)) *ident = TRUE;
	if (do_res_stat(A_CON)) *ident = TRUE;
	if (do_res_stat(A_CHR)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_ATHELAS:
      {
	msg_print("A fresh, clean essence rises, driving away wounds and poison.");
	(void)set_poisoned(0);
	(void)set_stun(0);
	(void)set_cut(0);
	if (p_ptr->black_breath)
	  {
	    msg_print("The hold of the Black Breath on you is broken!");
	  }
	p_ptr->black_breath = FALSE;
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_BEORNING:
      {
	msg_print("The cakes of the Beornings are tasty.");
	(void)hp_player(damroll(5, 8));
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_FOOD_GOOD:
      {
	msg_print("That tastes good.");
	*ident = TRUE;
	return TRUE;
      }
      
      /* Waybread is always fully satisfying. */
    case EF_WAYBREAD:
      {
	msg_print("That tastes good.");
	(void)set_food(PY_FOOD_MAX - 1);
	(void)set_poisoned(p_ptr->poisoned / 2);
	(void)hp_player(damroll(5, 10));
	*ident = TRUE;
	return TRUE;
      }
      



    case EF_DRINK_GOOD:
      {
	msg_print("You feel less thirsty.");
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_LIGHT:
      {
	if (hp_player(damroll(2, 10))) *ident = TRUE;
	if (set_blind(0)) *ident = TRUE;
	if (set_cut(p_ptr->cut - 10)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_SERIOUS:
      {
	if (hp_player(damroll(4, 10))) *ident = TRUE;
	if (set_blind(0)) *ident = TRUE;
	if (set_confused(0)) *ident = TRUE;
	if (set_cut(p_ptr->cut - 30)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_CRITICAL:
      {
	if (hp_player(damroll(6, 10))) *ident = TRUE;
	if (set_blind(0)) *ident = TRUE;
	if (set_confused(0)) *ident = TRUE;
	if (set_poisoned((p_ptr->poisoned / 2)-10)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(p_ptr->cut - 50)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HEALING1:
      {
	if (hp_player(300)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HEALING2:
      {
	if (hp_player(500)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HEALING3:
      {
	if (hp_player(300)) *ident = TRUE;
	if (set_blind(0)) *ident = TRUE;
	if (set_confused(0)) *ident = TRUE;
	if (set_poisoned(p_ptr->poisoned - 200)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HEALING4:
      {
	if (hp_player(600)) *ident = TRUE;
	if (set_blind(0)) *ident = TRUE;
	if (set_confused(0)) *ident = TRUE;
	if (set_poisoned(0)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(0)) *ident = TRUE;
	if (p_ptr->black_breath)
	  {
	    msg_print("The hold of the Black Breath on you is broken!");
	    *ident = TRUE;
	  }
	p_ptr->black_breath = FALSE;
	return TRUE;
      }
      
    case EF_LIFE:
      {
	msg_print("You feel life flow through your body!");
	restore_level();
	(void)set_poisoned(0);
	(void)set_blind(0);
	(void)set_confused(0);
	(void)set_image(0);
	(void)set_stun(0);
	(void)set_cut(0);
	(void)do_res_stat(A_STR);
	(void)do_res_stat(A_CON);
	(void)do_res_stat(A_DEX);
	(void)do_res_stat(A_WIS);
	(void)do_res_stat(A_INT);
	(void)do_res_stat(A_CHR);
	hp_player(2000);
	if (p_ptr->black_breath)
	  {
	    msg_print("The hold of the Black Breath on you is broken!");
	  }
	p_ptr->black_breath = FALSE;
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_INFRAVISION:
      {
	if (set_tim_infra(p_ptr->tim_infra + 100 + randint1(100)))
	  {
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_SEE_INVIS:
      {
	if (set_tim_invis(p_ptr->tim_invis + 12 + randint1(12)))
	  {
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_SLOW_POISON:
      {
	if (set_poisoned(p_ptr->poisoned / 2)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SPEED1:
      {
	if (!p_ptr->fast)
	  {
	    if (set_fast(randint1(20) + 15)) *ident = TRUE;
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	return TRUE;
      }
      
    case EF_RES_HEAT_COLD:
      {
	if (set_oppose_fire(p_ptr->oppose_fire + randint1(30) + 20))
	  {
	    *ident = TRUE;
	  }
	if (set_oppose_cold(p_ptr->oppose_cold + randint1(30) + 20))
	  {
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_RES_ACID_ELEC:
      {
	if (set_oppose_acid(p_ptr->oppose_acid + randint1(30) + 20))
	  {
	    *ident = TRUE;
	  }
	if (set_oppose_elec(p_ptr->oppose_elec + randint1(30) + 20))
	  {
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_RESIST_ALL:
      {
	if (set_oppose_fire(p_ptr->oppose_fire + randint1(25) + 15))
	  {
	    *ident = TRUE;
	  }
	if (set_oppose_cold(p_ptr->oppose_cold + randint1(25) + 15))
	  {
	    *ident = TRUE;
	  }
	if (set_oppose_acid(p_ptr->oppose_acid + randint1(25) + 15))
	  {
	    *ident = TRUE;
	  }
	if (set_oppose_elec(p_ptr->oppose_elec + randint1(25) + 15))
	  {
	    *ident = TRUE;
	  }
	if (set_oppose_pois(p_ptr->oppose_pois + randint1(25) + 15))
	  {
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_HEROISM:
      {
	if (hp_player(10)) *ident = TRUE;
	if (set_afraid(0)) *ident = TRUE;
	if (set_hero(p_ptr->hero + randint1(25) + 25)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_BERSERK_STR:
      {
	if (hp_player(30)) *ident = TRUE;
	if (set_afraid(0)) *ident = TRUE;
	if (set_shero(p_ptr->shero + randint1(25) + 25)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RESTORE_MANA:
      {
	if (p_ptr->csp < p_ptr->msp)
	  {
	    p_ptr->csp = p_ptr->msp;
	    p_ptr->csp_frac = 0;
	    msg_print("Your magical powers are completely restored!");
	    p_ptr->redraw |= (PR_MANA);
	    p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_RESTORE_EXP:
      {
	if (restore_level()) *ident = TRUE;
	return TRUE;
      }
      
    case EF_INC_STR:
    case EF_INC_INT:
    case EF_INC_WIS:
    case EF_INC_DEX:
    case EF_INC_CON:
    case EF_INC_CHR:
      {
	int stat = effect - EF_GAIN_STR;
	if (do_inc_stat(stat, FALSE)) *ident = TRUE;
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
	if (do_inc_stat(stat, TRUE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_AUGMENTATION:
      {
	if (do_inc_stat(A_STR, TRUE)) *ident = TRUE;
	if (do_inc_stat(A_INT, TRUE)) *ident = TRUE;
	if (do_inc_stat(A_WIS, TRUE)) *ident = TRUE;
	if (do_inc_stat(A_DEX, TRUE)) *ident = TRUE;
	if (do_inc_stat(A_CON, TRUE)) *ident = TRUE;
	if (do_inc_stat(A_CHR, TRUE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_ENLIGHTENMENT1:
      {
	msg_print("An image of your surroundings forms in your mind...");
	wiz_lite(FALSE);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_ENLIGHTENMENT2:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	msg_print("You begin to feel more enlightened...");
	msg_print(NULL);
	wiz_lite(TRUE);
	(void)do_inc_stat(A_INT, TRUE);
	(void)do_inc_stat(A_WIS, TRUE);
	(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	(void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_treasure(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_objects_gold(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_objects_normal(DETECT_RAD_DEFAULT, FALSE);
	*identify_pack();
	self_knowledge(TRUE);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_SELF_KNOWLEDGE:
      {
	msg_print("You begin to know yourself a little better...");
	msg_print(NULL);
	self_knowledge(TRUE);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_EXPERIENCE:
      {
	if (p_ptr->exp < PY_MAX_EXP)
	  {
	    s32b ee = (p_ptr->exp / 2) + 10;
	    if (ee > 100000L) ee = 100000L;
	    msg_print("You feel more experienced.");
	    gain_exp(ee);
	    *ident = TRUE;
	  }
	return TRUE;
      }
      
    case EF_VAMPIRE:
      {
	
	/* Already a Vampire */
	if (p_ptr->schange == SHAPE_VAMPIRE) return TRUE;
	
	/* Priests/Paladins can't be Vampires */
	if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	  {
	    msg_print("You reject the unholy serum.");
	    take_hit(damroll(10, 6), "dark forces");
	    return TRUE;
	  }
	
	/* Druids/Rangers can't be Vampires */
	if (mp_ptr->spell_book == TV_DRUID_BOOK)
	  {
	    msg_print("You reject the unnatural serum.");
	    take_hit(damroll(10, 6), "dark forces");
	    return TRUE;
	  }
	
	/* Others can */
	msg_print("You are infused with dark power.");
	
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
	(void)teleport_player_level(TRUE);
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
      {
	*ident = TRUE;
	if (!ident_spell()) return FALSE;
	return TRUE;
      }

    case EF_REVEAL_CURSES:
      {
	*ident = TRUE;
	if (!*identify_fully()) return FALSE;
	return TRUE;
      }
 
    case EF_BRANDING:
      {
	*ident = TRUE;
	if (!brand_missile(0, 0)) return FALSE;
	return TRUE;
      }
      
    case EF_FRIGHTENING:
      {
		if (fear_monster(dir, p_ptr->lev + 5)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_REMOVE_CURSE:
      {
	if (remove_curse())
	  {
	    msg_print("You feel as if someone is watching over you.");
	  }
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_REM_CURSE_GOOD:
      {
	if (remove_curse_good())
	  {
	    msg_print("You feel as if someone is watching over you.");
	  }
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_ENCHANT_ARMOR1:
      {
	*ident = TRUE;
	if (!enchant_spell(0, 0, 1)) return FALSE;
	return TRUE;
      }
      
    case EF_ENCHANT_ARMOR2:
      {
	*ident = TRUE;
	if (!enchant_spell(0, 0, randint1(3) + 2)) return FALSE;
	return TRUE;
      }
      
    case EF_ENCHANT_TO_HIT:
      {
	*ident = TRUE;
	if (!enchant_spell(1, 0, 0)) return FALSE;
	return TRUE;
      }
      
    case EF_ENCHANT_TO_DAM:
      {
	*ident = TRUE;
	if (!enchant_spell(0, 1, 0)) return FALSE;
	return TRUE;
      }
      
    case EF_ENCHANT_WEAPON:
      {
	*ident = TRUE;
	if (!enchant_spell(randint1(3), randint1(3), 0)) return FALSE;
	return TRUE;
      }
      
    case EF_RECHARGING1:
      {
	*ident = TRUE;
	if (!recharge(130)) return FALSE;
	return TRUE;
      }
      
    case EF_RECHARGING2:
      {
	*ident = TRUE;
	if (!recharge(200)) return FALSE;
	return TRUE;
      }
      
    case EF_LIGHT:
      {
	if (lite_area(damroll(2, 8), 2)) *ident = TRUE;
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
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_treasure(DETECT_RAD_DEFAULT, TRUE)) *ident = TRUE;
	if (detect_objects_gold(DETECT_RAD_DEFAULT, FALSE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DETECT_ITEM:
      {
	if (detect_objects_normal(DETECT_RAD_DEFAULT, TRUE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DETECT_TRAP:
      {
	if (detect_traps(DETECT_RAD_DEFAULT, TRUE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DETECT_DOOR:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_doors(DETECT_RAD_DEFAULT, TRUE)) *ident = TRUE;
	if (detect_stairs(DETECT_RAD_DEFAULT, FALSE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DETECT_INVIS:
      {
	if (detect_monsters_invis(DETECT_RAD_DEFAULT, TRUE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SATISFY_HUNGER:
      {
	if (set_food(PY_FOOD_MAX - 1)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_BLESSING1:
      {
	if (set_blessed(p_ptr->blessed + randint1(12) + 6)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_BLESSING2:
      {
	if (set_blessed(p_ptr->blessed + randint1(24) + 12)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_BLESSING3:
      {
	if (set_blessed(p_ptr->blessed + randint1(48) + 24)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_MONSTER_CONFU:
      {
	if (!(p_ptr->special_attack & (ATTACK_CONFUSE)))
	  {
	    msg_print("Your hands begin to glow.");
	    p_ptr->special_attack |= (ATTACK_CONFUSE);
	    *ident = TRUE;
	    p_ptr->redraw |= PR_STATUS;
	  }
	return TRUE;
      }
      
    case EF_PROT_FROM_EVIL:
      {
	k = 3 * p_ptr->lev;
	if (set_protevil(p_ptr->protevil + randint1(25) + k)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_RUNE_PROTECT:
      {
	/* Use up scroll only if warding_glyph is created. */
	*ident = TRUE;
	if (!lay_rune(RUNE_PROTECT)) return FALSE;
	return TRUE;
      }
      
    case EF_DOOR_DESTRUCT:
      {
	if (destroy_doors_touch()) *ident = TRUE;
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
	if (dispel_undead(60)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_GENOCIDE:
      {
	*ident = TRUE;
	if (!genocide()) return FALSE;
	return TRUE;
      }
      
    case EF_MASS_GENOCIDE:
      {
	*ident = TRUE;
	(void)mass_genocide();
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
	if (k == 0) set_ele_attack(ATTACK_ACID, 400);
	if (k == 1) set_ele_attack(ATTACK_ELEC, 400);
	if (k == 2) set_ele_attack(ATTACK_FIRE, 400);
	if (k == 3) set_ele_attack(ATTACK_COLD, 400);
	if (k == 4) set_ele_attack(ATTACK_POIS, 400);
	*ident = TRUE;
	
	return TRUE;
      }
    case EF_ACID_PROOF:
      {
	proof_flag = OF_ACID_PROOF;
	*ident = TRUE;
	if (!el_proof(proof_flag)) return FALSE;
	return TRUE;
      }

    case EF_ELEC_PROOF:
      {
	proof_flag = OF_ELEC_PROOF;
	*ident = TRUE;
	if (!el_proof(proof_flag)) return FALSE;
	return TRUE;
      }

    case EF_FIRE_PROOF:
      {
	proof_flag = OF_FIRE_PROOF;
	*ident = TRUE;
	if (!el_proof(proof_flag)) return FALSE;
	return TRUE;
      }

    case EF_COLD_PROOF:
      {
	proof_flag = OF_COLD_PROOF;
	*ident = TRUE;
	if (!el_proof(proof_flag)) return FALSE;
	return TRUE;
      }
    
   case EF_STARLIGHT:
      {
	/* Message. */
	if (!p_ptr->blind)
	  {
	    msg_print("The staff glitters with unearthly light.");
	  }
	
	/* Starbursts everywhere. */
	do_starlight(randint0(8) + 7, 12, FALSE);
	
	/* Hard not to *identify. */
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_DETECT_EVIL:
      {
	if (detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CURE_MEDIUM:
      {
	if (hp_player(randint1(20) + 10)) *ident = TRUE;
	(void)set_cut(p_ptr->cut - 10);
	return TRUE;
      }
      
    case EF_CURING:
      {
	if (set_blind(0)) *ident = TRUE;
	if (set_poisoned(0)) *ident = TRUE;
	if (set_confused(0)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(0)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_BANISHMENT:
      {
	if (banish_evil(80)) 
	  {
	    *ident = TRUE;
	    msg_print("A mighty force drives away evil!");
	  }
	return TRUE;
      }
      
    case EF_SLEEP_MONSTERS:
      {
	if (sleep_monsters(p_ptr->lev + 10)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SLOW_MONSTERS:
      {
	if (slow_monsters(p_ptr->lev + 10)) *ident = TRUE;
	return TRUE;
      }
          
    case EF_SPEED2:
      {
	if (!p_ptr->fast)
	  {
	    if (set_fast(randint1(30) + 15)) *ident = TRUE;
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
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
	if (dispel_evil(60)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_POWER:
      {
	if (dispel_monsters(100)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_HOLINESS:
      {
	if (dispel_evil(120)) *ident = TRUE;
	k = 2 * p_ptr->lev;
	if (set_protevil(p_ptr->protevil + randint1(25) + k)) *ident = TRUE;
	if (set_poisoned((p_ptr->poisoned / 2)-10)) *ident = TRUE;
	if (set_afraid(0)) *ident = TRUE;
	if (hp_player(50)) *ident = TRUE;
	if (set_stun(0)) *ident = TRUE;
	if (set_cut(0)) *ident = TRUE;
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
	
	msg_print("Mighty magics rend your enemies!");
	fire_sphere(GF_MANA, 0,
		    randint1(75) + 125, 5, 20);
	if (!(check_ability(SP_DEVICE_EXPERT)))
	  {
	    (void)take_hit(20, "unleashing magics too mighty to control");
	  }
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_STARBURST:
      {
	msg_print("Light bright beyond enduring dazzles your foes!");
	fire_sphere(GF_LITE, 0,
		    randint1(67) + 100, 5, 20);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_MASS_CONFU:
      {
	if (confu_monsters(p_ptr->lev + 30)) *ident = TRUE;
	return TRUE;
      }
      
      
    case EF_STARFIRE:
      {
	/* Message. */
	if (!p_ptr->blind)
	  {
	    msg_print("The staff blazes with unearthly light.");
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
	msg_print("A howling whirlwind rises in wrath around you.");
	fire_sphere(GF_FORCE, 0, randint1(100) + 100, 6, 20);
	
	/* Whisk around the player and nearby monsters.  This is 
	 * actually kinda amusing to see...
	 */
	fire_ball(GF_AWAY_ALL, 0, 12, 6, FALSE);
	teleport_player(6, TRUE);
	
	/* *Identify */
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_MENELTARMA:
      {
	msg_print("You envoke binding magics upon the undead nearby!");
	
	/* Attempt to Hold all undead in LOS. */
	if (hold_undead()) *ident = TRUE;
	
	return TRUE;
      }
      
    case EF_KELVAR:
      {
	/* Try to learn about the dungeon and traps in it from animals. */
	if (listen_to_natural_creatures())
	  msg_print("You listen and learn from natural creatures.");
	else msg_print("You found no animals nearby to learn from.");
	
	/* *Identify */
	*ident = TRUE;
	return TRUE;
      }
    
    case EF_TELEPORT_AWAY2:
      {
	if (teleport_monster(dir, 55 + (plev/2))) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DISARMING:
      {
	if (disarm_trap(dir)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DOOR_DEST:
      {
	if (destroy_door(dir)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_STONE_TO_MUD:
      {
	if (wall_to_mud(dir)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_LITE:
      {
	msg_print("A line of blue shimmering light appears.");
	lite_line(dir);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_SLEEP_MONSTER2:
      {
	if (sleep_monster(dir, plev + 15)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SLOW_MONSTER2:
      {
	if (slow_monster(dir, plev + 15)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_CONFUSE_MONSTER:
      {
	if (confuse_monster(dir, plev + 15)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_FEAR_MONSTER:
      {
	if (fear_monster(dir, plev + 20)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DRAIN_LIFE1:
      {
	if (drain_life(dir, 50 + plev)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_POLYMORPH:
      {
	if (poly_monster(dir)) *ident = TRUE;
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
	fire_bolt_or_beam(plev, GF_ACID, dir, damroll(5 + plev / 10, 8));
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_ELEC_BOLT1:
      {
	fire_bolt_or_beam(plev, GF_ELEC, dir, damroll(3 + plev / 14, 8));
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_FIRE_BOLT1:
      {
	fire_bolt_or_beam(plev, GF_FIRE, dir, damroll(6 + plev / 8, 8));
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_COLD_BOLT1:
      {
	fire_bolt_or_beam(plev, GF_COLD, dir, damroll(4 + plev / 12, 8));
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
			if (effect_wonder(dir, randint1(100) + p_ptr->lev / 5,
				beam)) *ident = TRUE;
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
	
	if (tmp == 1) fire_arc(GF_ACID, dir, 200, 9, 90);
	if (tmp == 2) fire_arc(GF_ELEC, dir, 180, 9, 90);
	if (tmp == 3) fire_arc(GF_COLD, dir, 190, 9, 90);
	if (tmp == 4) fire_arc(GF_FIRE, dir, 210, 9, 90);
	if (tmp == 5) fire_arc(GF_POIS, dir, 200, 7, 120);
	
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_ANNIHILATION:
      {
	if (drain_life(dir, 100 + randint1(plev * 4))) *ident = TRUE;
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
	msg_print("Deadly venom spurts and steams from your wand.");
	TARGET_PRESERVE
	  fire_bolt(GF_POIS, dir, damroll(plev / 2, 11));
	TARGET_RESTORE
	  fire_cloud(GF_POIS, dir, 30, 6);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_BEGUILING:
      {
	msg_print("You speak soft, beguiling words.");
	
	if (randint0(2) == 0) 
	  {
	    if (slow_monster(dir, plev * 2)) *ident = TRUE;
	  }
	if (randint0(2) == 0) 
	  {
	    if (confuse_monster(dir, plev * 2)) *ident = TRUE;
	  }
	else
	  {
	    if (sleep_monster(dir, plev * 2)) *ident = TRUE;
	  }
	
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_UNMAKING:
      {
	msg_print("You envoke the powers of Unmaking!");
	unmake(dir);
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_OSSE:
      {
	msg_print("You raise a foam-crested tidal wave."); 
	fire_arc(GF_WATER, dir, 3 * plev + randint1(100), 14, 90);
	*ident = TRUE;
	return TRUE;
      }
    case EF_RESTORATION:
      {
	if (restore_level()) *ident = TRUE;
	if (do_res_stat(A_STR)) *ident = TRUE;
	if (do_res_stat(A_INT)) *ident = TRUE;
	if (do_res_stat(A_WIS)) *ident = TRUE;
	if (do_res_stat(A_DEX)) *ident = TRUE;
	if (do_res_stat(A_CON)) *ident = TRUE;
	if (do_res_stat(A_CHR)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_TELEPORT_AWAY1:
      {
	if (teleport_monster(dir, 45 + (plev/3))) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SLEEP_MONSTER1:
      {
	if (sleep_monster(dir, plev + 10)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_SLOW_MONSTER1:
      {
	if (slow_monster(dir, plev + 10)) *ident = TRUE;
	return TRUE;
      }
      
    case EF_DRAIN_LIFE2:
      {
	if (drain_life(dir, 45 + 3 * plev / 2)) *ident = TRUE;
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
	fire_bolt_or_beam(plev/2 - 10, GF_FIRE, dir, 
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
	/* Aimed at oneself, this rod creates a room. */
	if ((dir == 5) && (p_ptr->target_row == p_ptr->py) && 
	    (p_ptr->target_col == p_ptr->px))
	  {
	    /* Lots of damage to creatures of stone. */
	    fire_sphere(GF_KILL_WALL, 0, 300, 4, 20);
	  }
	
	/* Otherwise, an extremely powerful destroy wall/stone. */
	else
	  {
	    extern bool wall_to_mud_hack(int dir, int dam);
	    (void) wall_to_mud_hack(dir, 160 + randint1(240));
	  }
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_SHADOW:
      {
	/* Hack - Extra good for those who backstab. */
	if (check_ability(SP_BACKSTAB))
	  {
	    if (p_ptr->superstealth) 
	      (void)set_superstealth(p_ptr->superstealth + 30,FALSE);
	    else (void)set_superstealth(p_ptr->superstealth + 75,TRUE);
	  }
	else
	  {
	    if (p_ptr->superstealth) 
	      (void)set_superstealth(p_ptr->superstealth + 20,FALSE);
	    else (void)set_superstealth(p_ptr->superstealth + 50,TRUE);
	  }
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_AIR:
      {
	msg_print("You raise your rod skyward and call upon the powers of Air.");
	ele_air_smite();
	*ident = TRUE;
	return TRUE;
      }
      
    case EF_PORTALS:
      {
	msg_print("Choose a location to teleport to.");
	msg_print(NULL);
	dimen_door();
	*ident = TRUE;
	return TRUE;
      }
      


    case EF_GWINDOR:
      {
	msg_print("The lantern wells with clear light...");
	lite_area(damroll(2, 15), 3);
	o_ptr->timeout = randint0(10) + 10;
	return TRUE;
      }
    case EF_NIMPHELOS:
      {
	msg_print("The pearl shines brightly...");
	map_area(0, 0, FALSE);
	o_ptr->timeout = randint0(40) + 40;
	return TRUE;
      }
    case EF_AVALLONE:
      {
	/* Hack - 'show' affected region only with
	 * the first detect */
	msg_print("The palantir clouds, then clears...");
	wiz_lite(FALSE);
	(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	(void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	o_ptr->timeout = randint0(120) + 120;
	return TRUE;
      }
    case EF_DWARVES:
      {
	msg_print("The silmaril shows the Light of the Trees...");
	wiz_lite(FALSE);
	(void)detect_all(DUNGEON_WID, TRUE);
	o_ptr->timeout = randint0(200) + 200;
	return TRUE;
      }
    case EF_ELESSAR:
      {
	msg_print("The elfstone glows deep green...");
	msg_print("You feel a warm tingling inside...");
	(void)hp_player(500);
	(void)set_cut(0);
	restore_level();
	o_ptr->timeout = 500;
	return TRUE;
      }
    case EF_RAZORBACK:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become Dragonking of Storms.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    msg_print("You are surrounded by lightning...");
	    for (i = 0; i < 8; i++) fire_ball(GF_ELEC, ddd[i], 150, 3, FALSE);
	  }
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_BLADETURNER:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become an Avatar of Dragonkind.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    msg_print("Your scales glow many colours...");
	    (void)hp_player(30);
	    (void)set_afraid(0);
	    (void)set_shero(p_ptr->shero + randint1(50) + 50);
	    (void)set_blessed(p_ptr->blessed + randint1(50) + 50);
	    (void)set_oppose_acid(p_ptr->oppose_acid + randint1(50) + 50);
	    (void)set_oppose_elec(p_ptr->oppose_elec + randint1(50) + 50);
	    (void)set_oppose_fire(p_ptr->oppose_fire + randint1(50) + 50);
	    (void)set_oppose_cold(p_ptr->oppose_cold + randint1(50) + 50);
	    (void)set_oppose_pois(p_ptr->oppose_pois + randint1(50) + 50);
	  }
	o_ptr->timeout = 400;
	return TRUE;
      }
    case EF_SOULKEEPER:
      {
	msg_print("Your armor glows a bright white...");
	msg_print("You feel much better...");
	(void)hp_player(1000);
	(void)set_cut(0);
	o_ptr->timeout = 888;
	return TRUE;
      }
    case EF_BELEGENNON:
      {
	msg_print("Your armor twists space around you...");
	teleport_player(10, TRUE);
	o_ptr->timeout = 2;
	return TRUE;
      }
    case EF_CELEBORN:
      {
	msg_print("Your armor glows deep blue...");
	(void)genocide();
	o_ptr->timeout = 1500;
	return TRUE;
      }
    case EF_HIMRING:
      {
	msg_print("A shrill wailing sound surrounds you.");
	(void)set_protevil(p_ptr->protevil + randint1(25) + plev);
	o_ptr->timeout = randint0(200) + 200;
	return TRUE;
      }
    case EF_ELEMENTS:
      {
	msg_print("Your shield glows many colours...");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	o_ptr->timeout = 160;
	return TRUE;
      }
      
    case EF_EARENDIL:
      {
	(void)set_poisoned(0);
	(void)set_confused(0);
	(void)set_blind(0);
	(void)set_stun(0);
	(void)set_cut(0);
	
	o_ptr->timeout = 100;
	return TRUE;
      }
      
    case EF_GIL_GALAD:
      {
	msg_print("Your shield gleams with blinding light...");
	fire_sphere(GF_LITE, 0, 75, 6, 20);
	confu_monsters(3 * plev / 2);
	o_ptr->timeout = 250;
	return TRUE;
      }
      
    case EF_HOLHENNETH:
      {
	msg_print("Your helm glows bright white...");
	msg_print("An image forms in your mind...");
	detect_all(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = randint0(55) + 55;
	return TRUE;
      }
    case EF_VINYAMAR:
      {
	msg_print("A thrilling battle song awakes the warrior within you!");
	(void)hp_player(10);
	(void)set_afraid(0);
	(void)set_hero(p_ptr->hero + randint1(25) + 25);
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_NARGOTHROND:
      {
	msg_print("Your crown glows deep blue...");
	msg_print("You feel a warm tingling inside...");
	(void)hp_player(500);
	(void)set_cut(0);
	o_ptr->timeout = 500;
	return TRUE;
      }
      
    case EF_VALINOR:
      {
	msg_print("Your cloak glows many colours...");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	(void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + 20);
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_HOLCOLLETH:
      {
	msg_print("Your cloak glows deep blue...");
	sleep_monsters_touch(3 * plev / 2 + 10);
	o_ptr->timeout = 55;
	return TRUE;
      }
    case EF_THINGOL:
      {
	msg_print("Your cloak glows bright yellow...");
	recharge(180);
	o_ptr->timeout = 90;
	return TRUE;
      }
    case EF_COLANNON:
      {
	msg_print("Your cloak twists space around you...");
	teleport_player(100, TRUE);
	o_ptr->timeout = 45;
	return TRUE;
      }
    case EF_LUTHIEN:
      {
	msg_print("Your cloak glows a deep red...");
	restore_level();
	o_ptr->timeout = 450;
	return TRUE;
      }
      
    case EF_CAMMITHRIM:
      {
	msg_print("Your gloves glow extremely brightly...");
		lite_line(dir);
	o_ptr->timeout = 5;
	return TRUE;
      }
    case EF_MAEGLIN:
      {
	msg_print("Your gauntlets radiate dark energy...");
		fire_bolt(GF_SPIRIT, dir, damroll(9, 8));
	take_hit(damroll(1, 6), "the dark arts");
	o_ptr->timeout = randint0(7) + 7;
	return TRUE;
      }
    case EF_PAURNIMMEN:
      {
	msg_print("Your gauntlets are covered in frost...");
	set_ele_attack(ATTACK_COLD, 50);
	o_ptr->timeout = randint0(50) + 100;
	return TRUE;
      }
    case EF_PAURNEN:
      {
	msg_print("Your gauntlets are covered in acid...");
	set_ele_attack(ATTACK_ACID, 30);
	o_ptr->timeout = randint0(50) + 100;
	return TRUE;
      }
    case EF_FINGOLFIN:
      {
	msg_print("Your cesti gleam brown and grey...");
		wall_to_mud(dir);
	o_ptr->timeout = randint0(3) + 3;
	return TRUE;
      }
      
      
    case EF_FEANOR:
      {
	msg_print("Your boots glow bright green...");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint1(20) + 20);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_DAL:
      {
	msg_print("Your boots glow deep blue...");
	(void)set_afraid(0);
	(void)set_poisoned(0);
	o_ptr->timeout = 5;
	return TRUE;
      }
      
    case EF_NARTHANC:
      {
	msg_print("Your dagger is covered in fire...");
		fire_bolt(GF_FIRE, dir, damroll(6, 8));
	o_ptr->timeout = randint0(7) + 7;
	return TRUE;
      }
    case EF_NIMTHANC:
      {
	msg_print("Your dagger is covered in frost...");
		fire_bolt(GF_COLD, dir, damroll(5, 8));
	o_ptr->timeout = randint0(6) + 6;
	return TRUE;
      }
    case EF_DETHANC:
      {
	msg_print("Your dagger is covered in sparks...");
		fire_bolt(GF_ELEC, dir, damroll(4, 8));
	o_ptr->timeout = randint0(5) + 5;
	return TRUE;
      }
    case EF_RILIA:
      {
	msg_print("Your dagger throbs deep green...");
		fire_ball(GF_POIS, dir, 12, 3, FALSE);
	o_ptr->timeout = randint0(4) + 4;
	return TRUE;
      }
    case EF_BELANGIL:
      {
	msg_print("Your dagger is covered in frost...");
		fire_ball(GF_COLD, dir, 3 * p_ptr->lev / 2, 2, FALSE);
	o_ptr->timeout = randint0(5) + 5;
	return TRUE;
      }
    case EF_ARANRUTH:
      {
	msg_print("Your sword glows a pale blue...");
		fire_bolt(GF_COLD, dir, damroll(12, 8));
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RINGIL:
      {
	msg_print("Your sword glows an intense blue-white...");
		fire_arc(GF_ICE, dir, 250, 10, 40);
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_NARSIL:
      {
	msg_print("Your sword glows an intense red...");
		fire_ball(GF_FIRE, dir, 150, 2, FALSE);
	o_ptr->timeout = 200;
	return TRUE;
      }

    case EF_MANWE:
      {
	msg_print("Your sword glows pure white...");
		fire_arc(GF_FORCE, dir, 300, 10, 180);
	o_ptr->timeout = 200;
	return TRUE;
      }
      
    case EF_AEGLOS:
      {
	msg_print("Your spear glows a bright white...");
		fire_ball(GF_COLD, dir, 100, 2, FALSE);
	o_ptr->timeout = 500;
	return TRUE;
      }
    case EF_OROME:
      {
	msg_print("Your spear pulsates...");
		wall_to_mud(dir);
	o_ptr->timeout = 5;
	return TRUE;
      }
    case EF_EONWE:
      {
	msg_print("Your axe lets out a long, shrill note...");
	(void)mass_genocide();
	o_ptr->timeout = 1500;
	return TRUE;
      }
    case EF_LOTHARANG:
      {
	msg_print("Your battle axe radiates deep purple...");
	hp_player(damroll(4, 12));
	(void)set_cut((p_ptr->cut / 2) - 50);
	o_ptr->timeout = randint0(3) + 3;
	return TRUE;
      }
    case EF_ULMO:
      {
	msg_print("Your trident glows deep red...");
		teleport_monster(dir, 45 + (plev/3));
	o_ptr->timeout = 75;
	return TRUE;
      }
    case EF_AVAVIR:
      {
	msg_print("Your scythe glows soft white...");
	if (!word_recall(randint0(20) + 15))
	  return TRUE;
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_TOTILA:
      {
	msg_print("Your flail glows in scintillating colours...");
		confuse_monster(dir, 3 * plev / 2 + 5);
	o_ptr->timeout = 15;
	return TRUE;
      }
      
    case EF_FIRESTAR:
      {
	msg_print("Your morning star rages in fire...");
		fire_ball(GF_FIRE, dir, 125, 3, FALSE);
	o_ptr->timeout = 150;
	return TRUE;
      }
    case EF_TARATOL:
      {
	msg_print("Your mace glows bright green...");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint1(20) + 20);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = randint0(100) + 100;
	return TRUE;
      }
    case EF_ERIRIL:
      {
	msg_print("Your quarterstaff glows yellow...");
	if (!ident_spell()) return FALSE;
	o_ptr->timeout = 10;
	return TRUE;
      }
    case EF_OLORIN:
      {
	msg_print("Your quarterstaff glows brightly...");
	probing();
	o_ptr->timeout = 20;
	return TRUE;
      }
    case EF_TURMIL:
      {
	msg_print("Your hammer glows white...");
	drain_life(dir, 90);
	o_ptr->timeout = 70;
	return TRUE;
      }
      
      /* Activations for dragon scale mails. */
    case EF_DRAGON_BLACK:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become an acidic dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    sound(MSG_BR_ACID);
	    msg_print("You breathe acid.");
	    fire_arc(GF_ACID, dir, (plev/10 + 1) * 45, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_BLUE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a storm dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    sound(MSG_BR_ELEC);
	    msg_print("You breathe lightning.");
	    fire_arc(GF_ELEC, dir, (plev/10 + 1) * 40, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_WHITE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become an icy dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    msg_print("You breathe frost.");
	    fire_arc(GF_COLD, dir, (plev/10 + 1) * 45, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_RED:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a fire dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    sound(MSG_BR_FIRE);
	    msg_print("You breathe fire.");
	    fire_arc(GF_FIRE, dir, (plev/10 + 1) * 50, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_GREEN:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a poisonous dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    sound(MSG_BR_GAS);
	    msg_print("You breathe poison gas.");
	    fire_arc(GF_POIS, dir, (plev/10 + 1) * 45, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_MULTIHUED:
      {
	static const struct
	{
	  int sound;
	  const char *msg;
	  int typ;
	} mh[] =
	    {
	      { MSG_BR_ELEC,  "lightning",  GF_ELEC },
	      { MSG_BR_FROST, "frost",      GF_COLD },
	      { MSG_BR_ACID,  "acid",       GF_ACID },
	      { MSG_BR_GAS,   "poison gas", GF_POIS },
	      { MSG_BR_FIRE,  "fire",       GF_FIRE }
	    };
	
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a powerful dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    chance = randint0(5);
	    sound(mh[chance].sound);
	    msg_format("You breathe %s.",
		       ((chance == 1) ? "lightning" :
			((chance == 2) ? "frost" :
			 ((chance == 3) ? "acid" :
			  ((chance == 4) ? "poison gas" : "fire")))));
	    fire_arc(((chance == 1) ? GF_ELEC :
		      ((chance == 2) ? GF_COLD :
		       ((chance == 3) ? GF_ACID :
			((chance == 4) ? GF_POIS : GF_FIRE)))),
		     dir, (plev/10 + 1) * 60, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_SHINING:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a glowing dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    chance = randint0(2);
	    sound(((chance == 0 ? MSG_BR_LIGHT : MSG_BR_DARK)));
	    msg_format("You breathe %s.",
		       ((chance == 0 ? "light" : "darkness")));
	    fire_arc((chance == 0 ? GF_LITE : GF_DARK), dir, 
		     (plev/10 + 1) * 50, 10, 40);
	  }			
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_LAW:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon of Order.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    chance = randint0(2);
	    sound(((chance == 1 ? MSG_BR_SOUND : MSG_BR_SHARDS)));
	    msg_format("You breathe %s.",
		       ((chance == 1 ? "sound" : "shards")));
	    fire_arc((chance == 1 ? GF_SOUND : GF_SHARD),
		     dir, (plev/10 + 1) * 60, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_BRONZE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a mystifying dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    sound(MSG_BR_CONF);
	    msg_print("You breathe confusion.");
	    fire_arc(GF_CONFUSION, dir, (plev/10 + 1) * 40, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_GOLD:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon with a deafening roar.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    sound(MSG_BR_SOUND);
	    msg_print("You breathe sound.");
	    fire_arc(GF_SOUND, dir, (plev/10 + 1) * 40, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_CHAOS:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon of Chaos.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    chance = randint0(2);
	    sound(((chance == 1 ? MSG_BR_CHAOS : MSG_BR_DISENCHANT)));
	    msg_format("You breathe %s.",
		       ((chance == 1 ? "chaos" : "disenchantment")));
	    fire_arc((chance == 1 ? GF_CHAOS : GF_DISENCHANT),
		     dir, (plev/10 + 1) * 55, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_BALANCE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon of Balance.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    chance = randint0(4);
	    sound(((chance == 1) ? MSG_BR_CHAOS :
		   ((chance == 2) ? MSG_BR_DISENCHANT :
		    ((chance == 3) ? MSG_BR_SOUND : MSG_BR_SHARDS))));
	    msg_format("You breathe %s.",
		       ((chance == 1) ? "chaos" :
			((chance == 2) ? "disenchantment" :
			 ((chance == 3) ? "sound" : "shards"))));
	    fire_arc(((chance == 1) ? GF_CHAOS :
		      ((chance == 2) ? GF_DISENCHANT :
		       ((chance == 3) ? GF_SOUND : GF_SHARD))),
		     dir, (plev/10 + 1) * 65, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
    case EF_DRAGON_POWER:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a wonderous dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    	    sound(MSG_BR_ELEMENTS);
	    msg_print("You breathe the elements.");
	    fire_arc(GF_ALL, dir, (plev/10 + 1) * 75, 10, 40);
	  }
	o_ptr->timeout = randint0(50) + 50;
	return TRUE;
      }
      
      /* Activations for rings. */
    case EF_RING_ACID:
      {
		fire_ball(GF_ACID, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	o_ptr->timeout = randint0(100) + 50;
	return TRUE;
      }
    case EF_RING_ELEC:
      {
		fire_ball(GF_ELEC, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	o_ptr->timeout = randint0(100) + 50;
	return TRUE;
      }
    case EF_RING_FIRE:
      {
		fire_ball(GF_FIRE, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	o_ptr->timeout = randint0(100) + 50;
	return TRUE;
      }
    case EF_RING_COLD:
      {
		fire_ball(GF_COLD, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	o_ptr->timeout = randint0(100) + 50;
	return TRUE;
      }
    case EF_RING_POIS:
      {
		fire_ball(GF_POIS, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + 20);
	o_ptr->timeout = randint0(100) + 50;
	return TRUE;
      }
      
      
      /* Activations for amulets. */
    case EF_AMULET_ESCAPING:
      {
	teleport_player(40,TRUE);
	o_ptr->timeout = randint0(40) + 40;
	return TRUE;
      }
      
    case EF_AMULET_LION:
      {
	/* Already a Lion */
	if (p_ptr->schange == SHAPE_LION) return TRUE;
	
	msg_print("You become a fierce Lion.");
	shapechange(SHAPE_LION);
	o_ptr->timeout = 200;
	return TRUE;
      }
      
    case EF_AMULET_METAMORPH:
      {
	/* Already changed */
	if (p_ptr->schange) return TRUE;
	
	shapechange(randint1(MAX_SHAPE));
	o_ptr->timeout = 300;
	return TRUE;
      }
      
      
      /* Activations for random artifacts, and available for use elsewhere. */
    case EF_RFIRE1:
      {
	msg_print("You launch a bolt of fire.");
		fire_bolt(GF_FIRE, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = randint0(7) + 7;
	return TRUE;
      }
    case EF_RFIRE2:
      {
	msg_print("You feel a sphere of fire form between your hands.");
		fire_sphere(GF_FIRE, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RFIRE3:
      {
	msg_print("The fires of Anor rise in wrath!");
	fire_sphere(GF_FIRE, 0, 150, 5, 20);
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RCOLD1:
      {
	msg_print("You launch a bolt of frost.");
		fire_bolt(GF_COLD, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = randint0(7) + 7;
	return TRUE;
      }
    case EF_RCOLD2:
      {
	msg_print("You hurl a sphere of killing frost.");
		fire_sphere(GF_COLD, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RCOLD3:
      {
	msg_print("A wild Northland frost storms uncontrollably!");
	fire_sphere(GF_COLD, 0, 150, 5, 20);
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RACID1:
      {
	msg_print("You launch a bolt of acid.");
		fire_bolt(GF_ACID, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = randint0(7) + 7;
	return TRUE;
      }
    case EF_RACID2:
      {
	msg_print("A sphere of deadly acid forms upon your hand.");
		fire_sphere(GF_ACID, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RACID3:
      {
	msg_print("A tornado of acid melts armour and flesh!");
	fire_sphere(GF_ACID, 0, 160, 3, 20);
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RELEC1:
      {
	msg_print("You launch a bolt of electricity.");
		fire_bolt(GF_ELEC, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = randint0(7) + 7;
	return TRUE;
      }
    case EF_RELEC2:
      {
	msg_print("You summon ball lightning to your aid.");
		fire_sphere(GF_ELEC, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RELEC3:
      {
	msg_print("A massive stroke of lightning smites the ground!");
	fire_sphere(GF_ELEC, 0, 130, 2, 20);
	msg_print("Boom!");
	fire_sphere(GF_SOUND, 0, 25, 9, 20);
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RPOIS1:
      {
	msg_print("You launch a poison dart.");
		fire_bolt(GF_POIS, dir, damroll(3 + plev / 10, 8));
	o_ptr->timeout = randint0(22) + 22;
	return TRUE;
      }
    case EF_RPOIS2:
      {
	msg_print("Deadly gases blanket the area.");
		fire_sphere(GF_POIS, 0, 110, 9, 30);
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RLIGHT1:
      {
	msg_print("You throw a radiant sphere...");
		TARGET_PRESERVE 
	  fire_ball(GF_LITE, dir, 50, 0, FALSE);
	TARGET_RESTORE
	  fire_ball(GF_CONFUSION, dir, 10, 0, FALSE);
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RLIGHT2:
      {
	msg_print("You bathe the area in radiant light!");
	dispel_light_hating(175);
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RDISPEL_UNDEAD:
      {
	msg_print("A tide of life surrounds you!");
	(void)dispel_undead(100);
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RDISPEL_EVIL:
      {
	msg_print("A wave of goodness washes over you...");
	(void)dispel_evil(100);
	
	if (check_ability(SP_EVIL))
	  {
	    msg_print("Your black soul is hit!");
	    take_hit(25, "struck down by Good");
	  }
	o_ptr->timeout = 400;
	return TRUE;
      }
    case EF_RSMITE_UNDEAD:
      {
	msg_print("Spells to dispel an undead antagonist surround you...");
		dispel_an_undead(dir, damroll(plev / 4, 33));
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_RSMITE_DEMON:
      {
	msg_print("Spells to dispel a demonic adversary surround you...");
		dispel_a_demon(dir, damroll(plev / 4, 33));
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_RSMITE_DRAGON:
      {
	msg_print("Spells to dispel a dragonic foe surround you...");
		dispel_a_dragon(dir, damroll(plev / 4, 33));
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_RHOLY_ORB:
      {
	msg_print("A cleansing ball materializes on your fingertips.");
		fire_sphere(GF_HOLY_ORB, dir, 60, 1, 20);
	o_ptr->timeout = 175;
	return TRUE;
      }
    case EF_RBLESS:
      {
	msg_print("You feel blessed for battle.");
	if (!p_ptr->blessed)
	  {
	    (void)set_blessed(p_ptr->blessed + randint1(24) + 24);
	  }
	else
	  {
	    (void)set_blessed(p_ptr->blessed + randint1(12) + 12);
	  }
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_RFRIGHTEN_ALL:
      {
	msg_print("You reveal yourself in wrath; your enemies tremble!");
	(void)fear_monsters((3 * plev / 2) + 5);
	o_ptr->timeout = randint0(120) + 120;
	return TRUE;
      }
    case EF_RHEAL1:
      {
	(void)hp_player(damroll(5, 20));
	(void)set_cut(p_ptr->cut - 5);
	(void)set_poisoned(p_ptr->poisoned - 5);
	o_ptr->timeout = 85;
	return TRUE;
      }
    case EF_RHEAL2:
      {
	(void)hp_player(damroll(7, 40));
	(void)set_cut((p_ptr->cut / 2) - 5);
	(void)set_poisoned((p_ptr->poisoned / 2) - 5);
	o_ptr->timeout = 225;
	return TRUE;
      }
    case EF_RHEAL3:
      {
	(void)hp_player(damroll(10, 60));
	(void)set_cut(0);
	(void)set_poisoned(0);
	o_ptr->timeout = 500;
	return TRUE;
      }
    case EF_RCURE:
      {
	msg_print("Tender hands massage your hurts away.");
	(void)set_stun(0);
	(void)set_cut(0);
	(void)set_poisoned(0);
	(void)set_confused(0);
	(void)set_blind(0);
	(void)do_res_stat(A_CON);
	o_ptr->timeout = 500;
	return TRUE;
      }
    case EF_RPROT_FROM_EVIL:
      {
	msg_print("A shrill wail surrounds you.");
	
	if (!p_ptr->protevil)
	  {
	    (void)set_protevil(p_ptr->protevil + randint1(24) + 24);
	  }
	else
	  {
	    (void)set_protevil(p_ptr->protevil + randint1(30));
	  }
	msg_print("You feel somewhat safer.");
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RCHAOS:
      {
	msg_print("You unleash the powers of Unmaking!");
		fire_ball(GF_CHAOS, dir, randint1(320), 2, FALSE);
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RSHARD_SOUND:
      {
	msg_print("You invoke the powers of Law...");
		if (randint1(2) == 1)
	  {
	    msg_print("...and razor-sharp obsidian chips hail upon your foes!");
	    fire_ball(GF_SHARD, dir, 150, 4, FALSE);
	  }
	else
	  {
	    msg_format("...and an awful cacophony shakes %s!", 
		       locality_name[stage_map[p_ptr->stage][LOCALITY]]);
	    fire_ball(GF_SOUND, dir, 150, 4, FALSE);
	  }
	o_ptr->timeout = 600;
	return TRUE;
      }
    case EF_RNETHR:
      {
	msg_print("You cast a gleaming orb of midnight hue!");
		fire_sphere(GF_NETHER, dir, 100, 1, 20);
	o_ptr->timeout = 400;
	return TRUE;
      }
    case EF_RLINE_LIGHT:
      {
		msg_print("A line of shimmering yellow light appears.");
	lite_line(dir);
	o_ptr->timeout = 6 + randint1(6);
	return TRUE;
      }
    case EF_RSTARLIGHT:
      {
	msg_print("Light radiates outward in all directions.");
	for (k = 0; k < 8; k++) lite_line(ddd[k]);
	o_ptr->timeout = 8 + randint1(8);
	return TRUE;
      }
    case EF_REARTHQUAKE:
      {
	msg_print("You strike the floor, and the earth crumbles!");
	earthquake(p_ptr->py, p_ptr->px, 10, FALSE);
	o_ptr->timeout = 40 + randint1(40);
	return TRUE;
      }
    case EF_RSPEED:
      {
	msg_print("All around you move with dreamlike slowness.");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint1(20) + 20);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = randint0(120) + 120;
	return TRUE;
      }
    case EF_RTELEPORT_AWAY:
      {
	msg_print("You weave a pattern of rejection and denial.");
		(void)teleport_monster(dir, 55 + (plev/2));
	o_ptr->timeout = 110;
	return TRUE;
      }
    case EF_RHEROISM:
      {
	msg_print("A thrilling battle song awakes the warrior within you!");
	(void)hp_player(10);
	(void)set_afraid(0);
	(void)set_hero(p_ptr->hero + randint1(25) + 25);
	o_ptr->timeout = 200;
	return TRUE;
      }
    case EF_RSTORM_DANCE:
      {
	msg_print("Wild music plays, and you dance up a storm...");
	fire_sphere(GF_SOUND, 0, 24, 8, 20);
	fire_sphere(GF_SHARD, 0, 32, 8, 20);
	fire_sphere(GF_CONFUSION, 0, 8, 8, 20);
	
	if (randint1(2) == 1) 
	  {
	    msg_print("Your wild movements exhaust you!");
	    take_hit(damroll(1, 12), "danced to death");
	  }
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RRESIST_ELEMENTS:
      {
	msg_print("Quadricolored magics swirl around you protectingly.");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	o_ptr->timeout = 400;
	return TRUE;
      }
    case EF_RRESIST_ALL:
      {
	msg_print("Penticolored magics swirl around you protectingly.");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	(void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + 20);
	o_ptr->timeout = 400;
	return TRUE;
      }
    case EF_RTELEPORT1:
      {
	msg_print("You pass through a transparent gateway...");
	teleport_player(30,TRUE);
	o_ptr->timeout = 10 + randint1(10);
	return TRUE;
      }
    case EF_RTELEPORT2:
      {
	msg_print("Time and space twist about you...");
	teleport_player(200,TRUE);
	o_ptr->timeout = 80;
	return TRUE;
      }
    case EF_RRECALL:
      {
	if(!word_recall(randint0(20) + 15))
	  return TRUE;
	o_ptr->timeout = 350;
	return TRUE;
      }
    case EF_RREGAIN:
      {
	msg_print("Surrounded by darkness, you envoke light and beauty.");
	msg_print("Your spirit regains its natural vitality.");
	
	(void)restore_level();
	o_ptr->timeout = 800;
	return TRUE;
      }
    case EF_RRESTORE:
      {
	msg_print("A multicolored mist surounds you, restoring body and mind.");
	(void)do_res_stat(A_STR);
	(void)do_res_stat(A_INT);
	(void)do_res_stat(A_WIS);
	(void)do_res_stat(A_DEX);
	(void)do_res_stat(A_CON);
	(void)do_res_stat(A_CHR);
	o_ptr->timeout = 800;
	return TRUE;
      }
    case EF_RSHIELD:
      {
	msg_print("Magics coalesce to form a shimmering barrier.");
	if (!p_ptr->shield)
	  {
	    (void)set_shield(p_ptr->shield + randint1(25) + 25);
	  }
	else
	  {
	    (void)set_shield(p_ptr->shield + randint1(15) + 15);
	  }
	o_ptr->timeout = 400;
	return TRUE;
      }
      
    case EF_RSUPER_SHOOTING:
      {
	/* Get the correct name for the missile, if possible. */
	missile_name = "missile";
	if ((o_ptr->sval == SV_LIGHT_XBOW) || 
	    (o_ptr->sval == SV_HEAVY_XBOW))
	  missile_name = "bolt";
	if ((o_ptr->sval == SV_LONG_BOW) || 
	    (o_ptr->sval == SV_LONG_BOW))
	  missile_name = "arrow";
	if (o_ptr->sval == SV_SLING) missile_name = "shot";
	
	msg_format("The %s you have ready to hand gleams with deadly power.", 
		   missile_name);
	p_ptr->special_attack |= (ATTACK_SUPERSHOT);
	o_ptr->timeout = 200 + randint1(200);
	
	/* Redraw the state */
	p_ptr->redraw |= (PR_STATUS);
	
	return TRUE;
      }
    case EF_RDETECT_MONSTERS:
      {
	msg_print("You search for monsters.");
	(void)detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = 4 + randint1(4);
	return TRUE;
      }
    case EF_RDETECT_EVIL:
      {
	msg_print("You hunt for evil creatures...");
	(void)detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = 4 + randint1(4);
	return TRUE;
      }
    case EF_RDETECT_ALL:
      {
	msg_print("You sense the area around you.");
	detect_all(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = 30 + randint1(30);
	return TRUE;
      }
    case EF_RMAGIC_MAP:
      {
	msg_print("A mental image of your surroundings is fixed in your mind.");
	map_area(0, 0, FALSE);
	o_ptr->timeout = 30 + randint1(30);
	return TRUE;
      }
    case EF_RDETECT_D_S_T:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	msg_print("The secrets of traps and doors are revealed.");
	(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	(void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	o_ptr->timeout = 10 + randint1(10);
	return TRUE;
      }
    case EF_RCONFU_FOE:
      {
	msg_print("You chant runes of confusing...");
		if (confuse_monster(dir, 5 * plev / 3))	
	  msg_print("...which utterly baffle your foe!");
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RSLEEP_FOE:
      {
	msg_print("A fine dust appears in your hand, and you throw it...");
		if (sleep_monster(dir, 5 * plev / 3)) 
	  msg_print("...sending a foe to the realm of dreams!");
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RTURN_FOE:
      {
	msg_print("You lock eyes with an enemy...");
		if (fear_monster(dir, 5 * plev / 3)) 
	  msg_print("...and break his courage!");
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RSLOW_FOE:
      {
	msg_print("You focus on the mind of an opponent...");
		if (slow_monster(dir, 5 * plev / 3)) 
	  msg_print("...and sap his strength!");
	o_ptr->timeout = 250;
	return TRUE;
      }
    case EF_RBANISH_EVIL:
      {
	msg_print("A mighty hand drives your foes from you!");
	(void)banish_evil(80);
	o_ptr->timeout = 400;
	return TRUE;
      }
    case EF_RDISARM:
      {
	msg_print("You feel skilled hands guiding your disarming.");
		(void)disarm_trap(dir);
	o_ptr->timeout = 7 + randint1(7);
	return TRUE;
      }
    case EF_RCONFU_FOES:
      {
	msg_print("You intone a bewildering hex...");
	if (confu_monsters(3 * plev / 2))	
	  msg_print("...which utterly baffles your foes!");
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RSLEEP_FOES:
      {
	msg_print("Soft, soothing music washes over you..");
	if (sleep_monsters(3 * plev / 2)) 
	  msg_print("...and sends your enemies to sleep!");
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RTURN_FOES:
      {
	msg_print("You reveal yourself in wrath; your enemies tremble!");
	(void)fear_monsters(3 * plev / 2);
	o_ptr->timeout = 300;
	return TRUE;
      }
    case EF_RSLOW_FOES:
      {
	msg_print("An opaque cloud blankets the area...");
	if (slow_monsters(3 * plev / 2)) 
	  msg_print("...and dissipates, along with your opponents' strength!");
	else msg_print("...and dissipates without effect.");
	o_ptr->timeout = 300;
	return TRUE;
      }

      /* Activations for rings of power */
    case EF_ACID_BLAST:
      {
	/* Acid, confusion, fear */
	msg_print("A blast of corrosion strikes your foes!");
	fire_sphere(GF_ACID, 0, randint1(100) + 100, 5, 20);
	(void)confu_monsters(100);
	(void)fear_monsters(100);

	o_ptr->timeout = 1500;
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

	msg_print("The lightning of Manwe leaps from your hands!");

	/* Initialise */
	for (i = 0; i < 100; i++) avail_mon[i] = 0;
	targ_y = cur_y;
	targ_x = cur_x;

	/* Start striking */
	for (i = 0; i < strikes; i++)
	  {
	    /* No targets yet */
	    avail_mon_num = 0;

	    /* Find something in range */
	    for (y = cur_y - 5; y <= cur_y + 5; y++)
	      for (x = cur_x - 5; x <= cur_x + 5; x++)
		{
		  int dist = distance(cur_y, cur_x, y, x);

		  /* Skip distant grids */
		  if (dist > 5) continue;

		  /* Skip grids that are out of bounds */
		  if (!in_bounds_fully(y, x)) continue;
	      
		  /* Skip grids that are not projectable */
		  if (projectable(cur_y, cur_x, y, x, flag) != PROJECT_CLEAR)
		    continue;
	      
		  /* Skip grids with no monster (including player) */
		  if (!cave_m_idx[y][x]) continue;

		  /* Record the monster */
		  avail_mon[avail_mon_num++] = cave_m_idx[y][x];
		}

	    /* Maybe we're at a dead end */
	    if (!avail_mon_num) return TRUE;

	    /* Pick a target... */
	    target = randint0(avail_mon_num);
	    if (avail_mon[target] == -1) 
	      {
		targ_y = p_ptr->py;
		targ_x = p_ptr->px;
	      }
	    else
	      {
		targ_y = m_list[avail_mon[target]].fy;
		targ_x = m_list[avail_mon[target]].fx;
	      }
	  
	    /* Paranoia */
	    if (!cave_m_idx[targ_y][targ_x]) return TRUE;

	    /* ...and hit it */
	    project(cur_mon, 1, targ_y, targ_x, damroll(30, 4), GF_ELEC, flag, 
		    0, 0);

	    /* Set current monster (may be dead) */
	    cur_y = targ_y;
	    cur_x = targ_x;
	    cur_mon = avail_mon[target];
	  }
      
	o_ptr->timeout = 2000;
	return TRUE;
      }
    case EF_LAVA_POOL:
      {
	int y, x;
	int py = p_ptr->py, px = p_ptr->px;

	msg_print("Lava wells around you!");
	
	/* Everything in range */
	for (y = py - 7; y <= py + 7; y++)
	  for (x = px - 7; x <= px + 7; x++)
	    {
	      int dist = distance(py, px, y, x);
	      feature_type *f_ptr = &f_info[cave_feat[y][x]];

	      /* Skip distant grids */
	      if (dist > 7) continue;

	      /* Skip grids that are out of bounds */
	      if (!in_bounds_fully(y, x)) continue;
	      
	      /* Skip grids that are permanent */
	      if (f_ptr->flags & TF_PERMANENT) continue;
	      
	      /* Skip grids in vaults */
	      if (cave_info[y][x] & CAVE_ICKY) continue;
	      
	      /* Lava now */
	      cave_set_feat(y, x, FEAT_LAVA);
	    }
      
	o_ptr->timeout = 2000;
	return TRUE;
      }
    case EF_ICE_WHIRLPOOL:
      {
	/* Unleash a whirlpool. */
	msg_print("A howling vortex of ice rises in wrath around you.");
	fire_sphere(GF_ICE, 0, randint1(100) + 100, 5, 10);
	
	/* Whisk around and slow the nearby monsters. */
	fire_ball(GF_AWAY_ALL, 0, 12, 6, FALSE);
	slow_monsters(50);
	
	o_ptr->timeout = 1000;
	return TRUE;
      }
    case EF_GROW_FOREST:
      {
	msg_print("A leafy forest surrounds you!");
	grow_trees_and_grass(TRUE);
	o_ptr->timeout = 2000;
	return TRUE;
      }
    case EF_RESTORE_AND_ENHANCE:
      {
	msg_print("You feel life flow through your body!");
	restore_level();
	(void)set_poisoned(0);
	(void)set_blind(0);
	(void)set_confused(0);
	(void)set_image(0);
	(void)set_stun(0);
	(void)set_cut(0);
	(void)do_res_stat(A_STR);
	(void)do_res_stat(A_CON);
	(void)do_res_stat(A_DEX);
	(void)do_res_stat(A_WIS);
	(void)do_res_stat(A_INT);
	(void)do_res_stat(A_CHR);
	hp_player(2000);
	if (p_ptr->black_breath)
	  {
	    msg_print("The hold of the Black Breath on you is broken!");
	  }
	p_ptr->black_breath = FALSE;
	(void)set_afraid(0);
	(void)set_hero(p_ptr->hero + randint1(25) + 25);
	(void)set_shero(p_ptr->shero + randint1(25) + 25);
	(void)set_blessed(p_ptr->blessed + randint1(25) + 25);
	(void)set_protevil(p_ptr->protevil + randint1(25) + 25);
	o_ptr->timeout = 3000;
	return TRUE;
      }
    case EF_ZONE_OF_CHAOS:
      {
	int y, x;
	int py = p_ptr->py, px = p_ptr->px;

	msg_print("The forces of chaos surround you!");

	/* Everything in range */
	for (y = py - 10; y <= py + 10; y++)
	  for (x = px - 10; x <= px + 10; x++)
	    {
	      int dist = distance(py, px, y, x);

	      /* Skip distant grids */
	      if (dist > 10) continue;

	      /* 20% chance */
	      if (randint1(5) == 1) continue;

	      /* Hit it */
	      (void)fire_meteor(-1, GF_CHAOS, y, x, 50, 0, FALSE);
	    }

	o_ptr->timeout = 3000;
	return TRUE;
      }
    case EF_PRESSURE_WAVE:
      {
	msg_print("Your foes are thrown backward!");
	fire_ball(GF_FORCE, 0, 50, 20, FALSE);

	o_ptr->timeout = 1500;
	return TRUE;
      }
    case EF_ENERGY_DRAIN:
      {
	int y, x;
	int py = p_ptr->py, px = p_ptr->px;
	monster_type *m_ptr;

	msg_print("Your foes slow, and you seem to have an eternity to act...");
	
	/* Everything in range */
	for (y = py - 5; y <= py + 5; y++)
	  for (x = px - 5; x <= px + 5; x++)
	    {
	      int dist = distance(py, px, y, x);

	      /* Skip distant grids */
	      if (dist > 5) continue;

	      /* Skip grids with no monster */
	      if (cave_m_idx[y][x] <= 0) continue;

	      /* Skip grids without LOS */
	      if (!player_has_los_bold(y, x)) continue;

	      /* Get the monster */
	      m_ptr = &m_list[cave_m_idx[y][x]];
	      
	      /* Take the energy */
	      p_ptr->energy += m_ptr->energy;
	      m_ptr->energy = 0;
	    }
      
	o_ptr->timeout = 3000;
	return TRUE;
      }
    case EF_MASS_STASIS:
      {
	msg_print("You command your foes to be still!");
	hold_all();
	o_ptr->timeout = 1000;
	return TRUE;
      }
    case EF_LIGHT_FROM_ABOVE:
      {
	int y, x;
	int py = p_ptr->py, px = p_ptr->px;

	msg_print("The light of the Valar shines from above!");

	/* Everything in range */
	for (y = py - 10; y <= py + 10; y++)
	  for (x = px - 10; x <= px + 10; x++)
	    {
	      int dist = distance(py, px, y, x);

	      /* Skip distant grids */
	      if (dist > 10) continue;

	      /* Skip grids with no monster */
	      if (cave_m_idx[y][x] <= 0) continue;

	      /* Hit it */
	      (void)fire_meteor(-1, GF_LITE, y, x, 100, 1, FALSE);
	    }

	o_ptr->timeout = 1500;
	return TRUE;
      }
    case EF_MASS_POLYMORPH:
      {
	msg_print("Reality warps...");
	poly_all(p_ptr->depth);

	o_ptr->timeout = 2500;
	return TRUE;
      }
    case EF_GRAVITY_WAVE:
      {
	msg_print("Gravity crushes, then releases, your foes!");
	fire_ball(GF_GRAVITY, 0, 100, 20, FALSE);

	o_ptr->timeout = 2000;
	return TRUE;
      }
    case EF_ENLIST_EARTH:
      {
	int m_idx, targ_y, targ_x, group;

	msg_print("You call on the Earth to bring forth allies!");

	/* Must target a monster */
	if (!get_aim_dir(&dir)) return FALSE;
	if (p_ptr->target_who <= 0) 
	  {
	    msg_print("You must target a monster.");
	    return FALSE;
	  }

	targ_y = m_list[p_ptr->target_who].fy;
	targ_x = m_list[p_ptr->target_who].fx;
	group = m_list[p_ptr->target_who].group;

	/* Summon golems */
	summon_specific(targ_y, targ_x,	FALSE, p_ptr->depth, SUMMON_GOLEM);

	/* Hack - make all local golems hostile to the target */
	for (m_idx = 0; m_idx < z_info->m_max; m_idx++)
	  {
	    monster_type *m_ptr = &m_list[m_idx];
	    monster_race *r_ptr = &r_info[m_ptr->r_idx];

	    if ((distance(targ_y, targ_x, m_ptr->fy, m_ptr->fx) < 7) &&
		(r_ptr->d_char = 'g') && (!(rf_has(r_ptr->flags, RF_DRAGON))))
	      m_ptr->hostile = p_ptr->target_who;
	  }

	o_ptr->timeout = 3000;
	return TRUE;
      }
    case EF_TELEPORT_ALL:
      {
	teleport_all(80);
	o_ptr->timeout = 2000;
	return TRUE;
      }
      
      
      
      /* Activations for ego-items. */
    case EF_BALROG_WHIP:
      {
	/* A crude way to simulate a long-range melee blow... */	
	msg_print("You lash out at a nearby foe.");
	fire_arc(GF_FIRE, dir, damroll(o_ptr->dd, o_ptr->ds), 2, 0);
	o_ptr->timeout = 0;
	return TRUE;
      }
      
      /* Activation for magestaffs */
    case EF_MAGESTAFF:
      {
	if (p_ptr->csp < p_ptr->msp)
	  {
	    p_ptr->csp += 10;
	    if (p_ptr->csp > p_ptr->msp) 
	      {
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
	      }
	    msg_print("Magical power flows from your staff.");
	    p_ptr->redraw |= (PR_MANA);
	    p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	    o_ptr->timeout = randint0(10) + 15;
	  }
	return TRUE;
      }
      


    default:
      {  
	/* Mistake */
	msg_print("Oops.  Effect type unrecognized.");
	return FALSE;
      }
    }
}

