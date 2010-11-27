/** \file cmd6.c 
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


/**
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(void)
{
  int item, ident, lev;
  
  object_type *o_ptr;
  
  cptr q, s;
  
  
  /* Restrict choices to food */
  item_tester_tval = TV_FOOD;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Eat which item? ";
      s = "You have nothing to eat.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  
  /* Sound */
  sound(MSG_EAT);
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Identity not known yet */
  ident = FALSE;
  
  /* Object level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Analyze the food */
  switch (o_ptr->sval)
    {
    case SV_FOOD_POISON:
      {
	if (pois_hit(15)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_BLINDNESS:
      {
	if (!p_ptr->no_blind)
	  {
	    if (set_blind(p_ptr->blind + rand_int(200) + 200))
	      {
		ident = TRUE;
	      }
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	break;
      }
      
    case SV_FOOD_PARANOIA:
      {
	if (!p_ptr->no_fear)
	  {
	    if (set_afraid(p_ptr->afraid + rand_int(10) + 10))
	      {
		ident = TRUE;
	      }
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_FEARLESS, 0);
	break;
      }
      
    case SV_FOOD_CONFUSION:
      {
	if (!p_resist_good(P_RES_CONFU))
	  {
	    if (set_confused(p_ptr->confused + rand_int(10) + 10))
	      {
		ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_other(IF_RES_CONFU, 0);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_FOOD_HALLUCINATION:
      {
	if (!p_resist_good(P_RES_CHAOS))
	  {
	    if (set_image(p_ptr->image + rand_int(250) + 250))
	      {
		ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_other(IF_RES_CHAOS, 0);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_FOOD_PARALYSIS:
      {
	if (!p_ptr->free_act)
	  {
	    if (set_paralyzed(p_ptr->paralyzed + rand_int(10) + 10))
	      {
		ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_obj(OF_FREE_ACT, 0);
	    ident = TRUE;
	  }
	break;
		}
      
    case SV_FOOD_WEAKNESS:
      {
	take_hit(damroll(6, 6), "poisonous food.");
	(void)do_dec_stat(A_STR);
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_SICKNESS:
      {
	take_hit(damroll(6, 6), "poisonous food.");
	(void)do_dec_stat(A_CON);
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_STUPIDITY:
      {
	take_hit(damroll(8, 8), "poisonous food.");
	(void)do_dec_stat(A_INT);
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_NAIVETY:
      {
	take_hit(damroll(8, 8), "poisonous food.");
	(void)do_dec_stat(A_WIS);
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_UNHEALTH:
      {
	take_hit(damroll(10, 10), "poisonous food.");
	(void)do_dec_stat(A_CON);
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_DISEASE:
      {
	take_hit(damroll(10, 10), "poisonous food.");
	(void)do_dec_stat(A_STR);
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_CURE_POISON:
      {
	if (set_poisoned(0)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_CURE_BLINDNESS:
      {
	if (set_blind(0)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_CURE_PARANOIA:
      {
	if (set_afraid(0)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_CURE_CONFUSION:
      {
	if (set_confused(0)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_CURE_SERIOUS:
      {
	if (hp_player(damroll(4, 8))) ident = TRUE;
	break;
      }
      
    case SV_FOOD_RESTORE_STR:
      {
	if (do_res_stat(A_STR)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_RESTORE_CON:
      {
	if (do_res_stat(A_CON)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_RESTORING:
      {
	if (do_res_stat(A_STR)) ident = TRUE;
	if (do_res_stat(A_INT)) ident = TRUE;
	if (do_res_stat(A_WIS)) ident = TRUE;
	if (do_res_stat(A_DEX)) ident = TRUE;
	if (do_res_stat(A_CON)) ident = TRUE;
	if (do_res_stat(A_CHR)) ident = TRUE;
	break;
      }
      
    case SV_FOOD_ATHELAS:
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
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_BEORNING:
      {
	msg_print("The cakes of the Beornings are tasty.");
	(void)hp_player(damroll(5, 8));
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_RATION:
    case SV_FOOD_BISCUIT:
    case SV_FOOD_JERKY:
    case SV_FOOD_SLIME_MOLD:
      {
	msg_print("That tastes good.");
	ident = TRUE;
	break;
      }
      
      /* Waybread is always fully satisfying. */
    case SV_FOOD_WAYBREAD:
      {
	msg_print("That tastes good.");
	(void)set_food(PY_FOOD_MAX - 1);
	(void)set_poisoned(p_ptr->poisoned / 2);
	(void)hp_player(damroll(5, 10));
	ident = TRUE;
	break;
      }
      
    case SV_FOOD_PINT_OF_ALE:
    case SV_FOOD_PINT_OF_WINE:
      {
	msg_print("That tastes good.");
	ident = TRUE;
	break;
      }
    }
  
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* We have tried it */
  object_tried(o_ptr);
  
  /* The player is now aware of the object */
  if (ident && !object_aware_p(o_ptr))
    {
      object_aware(o_ptr);
      gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
      if (o_ptr->number > 1) apply_autoinscription(o_ptr);
    }
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  
  /* Food can feed the player */
  (void)set_food(p_ptr->food + o_ptr->pval);
  
  
  /* Destroy a food in the pack */
  if (item >= 0)
    {
      inven_item_increase(item, -1);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
  
  /* Destroy a food on the floor */
  else
    {
      floor_item_increase(0 - item, -1);
      floor_item_describe(0 - item);
      floor_item_optimize(0 - item);
    }

  /* Forget the item_tester_tval restriction */
  item_tester_tval = 0;
  
}




/**
 * Quaff a potion (from the pack or the floor)
 */
void do_cmd_quaff_potion(void)
{
  int item, ident, lev;
  
  object_type *o_ptr;
  
  cptr q, s;
  
  
  /* Restrict choices to potions */
  item_tester_tval = TV_POTION;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Quaff which potion? ";
      s = "You have no potions to quaff.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  
  /* Sound */
  sound(MSG_QUAFF);


  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Not identified yet */
  ident = FALSE;
  
  /* Object level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Analyze the potion */
  switch (o_ptr->sval)
    {
    case SV_POTION_WATER:
    case SV_POTION_APPLE_JUICE:
    case SV_POTION_SLIME_MOLD:
      {
	msg_print("You feel less thirsty.");
	ident = TRUE;
	break;
      }
      
    case SV_POTION_SLOWNESS:
      {
	if (set_slow(p_ptr->slow + randint(25) + 15)) ident = TRUE;
	break;
      }
      
    case SV_POTION_SALT_WATER:
      {
	msg_print("The potion makes you vomit!");
	(void)set_food(PY_FOOD_STARVE - 1);
	(void)set_poisoned(0);
	(void)set_paralyzed(p_ptr->paralyzed + 4);
	ident = TRUE;
	break;
      }
      
    case SV_POTION_POISON:
      {
	if (pois_hit(30)) ident = TRUE;
	break;
      }
      
    case SV_POTION_BLINDNESS:
      {
	if (!p_ptr->no_blind)
	  {
	    if (set_blind(p_ptr->blind + rand_int(100) + 100))
	      {
		ident = TRUE;
	      }
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	break;
      }
      
    case SV_POTION_CONFUSION:
      {
	if (!p_resist_good(P_RES_CONFU))
	  {
	    if (set_confused(p_ptr->confused + rand_int(20) + 15))
	      {
		ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_other(IF_RES_CONFU, 0);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_SLEEP:
      {
	if (!p_ptr->free_act)
	  {
	    if (set_paralyzed(p_ptr->paralyzed + rand_int(4) + 4))
	      {
		ident = TRUE;
	      }
	  }
	else 
	  {
	    notice_obj(OF_FREE_ACT, 0);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_LOSE_MEMORIES:
      {
	if (!p_ptr->hold_life && (p_ptr->exp > 0))
	  {
	    msg_print("You feel your memories fade.");
	    lose_exp(p_ptr->exp / 4);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_RUINATION:
      {
	msg_print("Your nerves and muscles feel weak and lifeless!");
	take_hit(damroll(5, 10), "a potion of Ruination");
	(void)dec_stat(A_DEX, 25, FALSE);
	(void)dec_stat(A_WIS, 25, FALSE);
	(void)dec_stat(A_CON, 25, FALSE);
	(void)dec_stat(A_STR, 25, FALSE);
	(void)dec_stat(A_CHR, 25, FALSE);
	(void)dec_stat(A_INT, 25, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_POTION_DEC_STR:
      {
	if (do_dec_stat(A_STR)) ident = TRUE;
	break;
      }
      
    case SV_POTION_DEC_INT:
      {
	if (do_dec_stat(A_INT)) ident = TRUE;
	break;
      }
      
    case SV_POTION_DEC_WIS:
      {
	if (do_dec_stat(A_WIS)) ident = TRUE;
	break;
      }
      
    case SV_POTION_DEC_DEX:
      {
	if (do_dec_stat(A_DEX)) ident = TRUE;
	break;
      }
      
    case SV_POTION_DEC_CON:
      {
	if (do_dec_stat(A_CON)) ident = TRUE;
	break;
      }
      
    case SV_POTION_DEC_CHR:
      {
	if (do_dec_stat(A_CHR)) ident = TRUE;
	break;
      }
      
    case SV_POTION_DETONATIONS:
      {
	msg_print("Massive explosions rupture your body!");
	take_hit(damroll(50, 20), "a potion of Detonation");
	(void)set_stun(p_ptr->stun + 75);
	(void)set_cut(p_ptr->cut + 5000);
	ident = TRUE;
	break;
      }
      
    case SV_POTION_DEATH:
      {
	msg_print("A feeling of Death flows through your body.");
	take_hit(p_ptr->chp, "a potion of Death");
	ident = TRUE;
	break;
      }
      
    case SV_POTION_INFRAVISION:
      {
	if (set_tim_infra(p_ptr->tim_infra + 100 + randint(100)))
	  {
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_DETECT_INVIS:
      {
	if (set_tim_invis(p_ptr->tim_invis + 12 + randint(12)))
	  {
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_SLOW_POISON:
      {
	if (set_poisoned(p_ptr->poisoned / 2)) ident = TRUE;
	break;
      }
      
    case SV_POTION_CURE_POISON:
      {
	if (set_poisoned(0)) ident = TRUE;
	break;
      }
      
    case SV_POTION_BOLDNESS:
      {
	if (set_afraid(0)) ident = TRUE;
	break;
      }
      
    case SV_POTION_SPEED:
      {
	if (!p_ptr->fast)
	  {
	    if (set_fast(randint(20) + 15)) ident = TRUE;
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	break;
      }
      
    case SV_POTION_RESIST_HEAT_COLD:
      {
	if (set_oppose_fire(p_ptr->oppose_fire + randint(30) + 20))
	  {
	    ident = TRUE;
	  }
	if (set_oppose_cold(p_ptr->oppose_cold + randint(30) + 20))
	  {
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_RESIST_ACID_ELEC:
      {
	if (set_oppose_acid(p_ptr->oppose_acid + randint(30) + 20))
	  {
	    ident = TRUE;
	  }
	if (set_oppose_elec(p_ptr->oppose_elec + randint(30) + 20))
	  {
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_RESIST_ALL:
      {
	if (set_oppose_fire(p_ptr->oppose_fire + randint(25) + 15))
	  {
	    ident = TRUE;
	  }
	if (set_oppose_cold(p_ptr->oppose_cold + randint(25) + 15))
	  {
	    ident = TRUE;
	  }
	if (set_oppose_acid(p_ptr->oppose_acid + randint(25) + 15))
	  {
	    ident = TRUE;
	  }
	if (set_oppose_elec(p_ptr->oppose_elec + randint(25) + 15))
	  {
	    ident = TRUE;
	  }
	if (set_oppose_pois(p_ptr->oppose_pois + randint(25) + 15))
	  {
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_HEROISM:
      {
	if (hp_player(10)) ident = TRUE;
	if (set_afraid(0)) ident = TRUE;
	if (set_hero(p_ptr->hero + randint(25) + 25)) ident = TRUE;
	break;
      }
      
    case SV_POTION_BERSERK_STR:
      {
	if (hp_player(30)) ident = TRUE;
	if (set_afraid(0)) ident = TRUE;
	if (set_shero(p_ptr->shero + randint(25) + 25)) ident = TRUE;
	break;
      }
      
    case SV_POTION_CURE_LIGHT:
      {
	if (hp_player(damroll(2, 10))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_cut(p_ptr->cut - 10)) ident = TRUE;
	break;
      }
      
    case SV_POTION_CURE_SERIOUS:
      {
	if (hp_player(damroll(4, 10))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_cut(p_ptr->cut - 30)) ident = TRUE;
	break;
      }
      
    case SV_POTION_CURE_CRITICAL:
      {
	if (hp_player(damroll(6, 10))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned((p_ptr->poisoned / 2)-10)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(p_ptr->cut - 50)) ident = TRUE;
	break;
      }
      
    case SV_POTION_HEALING:
      {
	if (hp_player(300)) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned(p_ptr->poisoned - 200)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_HEALING:
      {
	if (hp_player(600)) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	if (p_ptr->black_breath)
	  {
	    msg_print("The hold of the Black Breath on you is broken!");
	    ident = TRUE;
	  }
	p_ptr->black_breath = FALSE;
	break;
      }
      
    case SV_POTION_LIFE:
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
	ident = TRUE;
	break;
      }
      
    case SV_POTION_RESTORE_MANA:
      {
	if (p_ptr->csp < p_ptr->msp)
	  {
	    p_ptr->csp = p_ptr->msp;
	    p_ptr->csp_frac = 0;
	    msg_print("Your magical powers are completely restored!");
	    p_ptr->redraw |= (PR_MANA);
	    p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_RESTORE_EXP:
      {
	if (restore_level()) ident = TRUE;
	break;
      }
      
    case SV_POTION_RES_STR:
      {
	if (do_res_stat(A_STR)) ident = TRUE;
	break;
      }
      
    case SV_POTION_RES_INT:
      {
	if (do_res_stat(A_INT)) ident = TRUE;
	break;
      }
      
    case SV_POTION_RES_WIS:
      {
	if (do_res_stat(A_WIS)) ident = TRUE;
	break;
      }
      
    case SV_POTION_RES_DEX:
      {
	if (do_res_stat(A_DEX)) ident = TRUE;
	break;
      }
      
    case SV_POTION_RES_CON:
      {
	if (do_res_stat(A_CON)) ident = TRUE;
	break;
      }
      
    case SV_POTION_RES_CHR:
      {
	if (do_res_stat(A_CHR)) ident = TRUE;
	break;
      }
      
    case SV_POTION_INC_STR:
      {
	if (do_inc_stat(A_STR, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_INC_INT:
      {
	if (do_inc_stat(A_INT, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_INC_WIS:
      {
	if (do_inc_stat(A_WIS, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_INC_DEX:
      {
	if (do_inc_stat(A_DEX, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_INC_CON:
      {
	if (do_inc_stat(A_CON, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_INC_CHR:
      {
	if (do_inc_stat(A_CHR, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_INC_STR:
      {
	if (do_inc_stat(A_STR, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_INC_INT:
      {
	if (do_inc_stat(A_INT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_INC_WIS:
      {
	if (do_inc_stat(A_WIS, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_INC_DEX:
      {
	if (do_inc_stat(A_DEX, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_INC_CON:
      {
	if (do_inc_stat(A_CON, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_INC_CHR:
      {
	if (do_inc_stat(A_CHR, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_AUGMENTATION:
      {
	if (do_inc_stat(A_STR, TRUE)) ident = TRUE;
	if (do_inc_stat(A_INT, TRUE)) ident = TRUE;
	if (do_inc_stat(A_WIS, TRUE)) ident = TRUE;
	if (do_inc_stat(A_DEX, TRUE)) ident = TRUE;
	if (do_inc_stat(A_CON, TRUE)) ident = TRUE;
	if (do_inc_stat(A_CHR, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_POTION_ENLIGHTENMENT:
      {
	msg_print("An image of your surroundings forms in your mind...");
	wiz_lite(FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_POTION_STAR_ENLIGHTENMENT:
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
	identify_pack();
	self_knowledge(TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_POTION_SELF_KNOWLEDGE:
      {
	msg_print("You begin to know yourself a little better...");
	msg_print(NULL);
	self_knowledge(TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_POTION_EXPERIENCE:
      {
	if (p_ptr->exp < PY_MAX_EXP)
	  {
	    s32b ee = (p_ptr->exp / 2) + 10;
	    if (ee > 100000L) ee = 100000L;
	    msg_print("You feel more experienced.");
	    gain_exp(ee);
	    ident = TRUE;
	  }
	break;
      }
      
    case SV_POTION_VAMPIRE:
      {
	
	/* Already a Vampire */
	if (p_ptr->schange == SHAPE_VAMPIRE) break;
	
	/* Priests/Paladins can't be Vampires */
	if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	  {
	    msg_print("You reject the unholy serum.");
	    take_hit(damroll(10, 6), "dark forces");
	    break;
	  }
	
	/* Druids/Rangers can't be Vampires */
	if (mp_ptr->spell_book == TV_DRUID_BOOK)
	  {
	    msg_print("You reject the unnatural serum.");
	    take_hit(damroll(10, 6), "dark forces");
	    break;
	  }
	
	/* Others can */
	msg_print("You are infused with dark power.");
	
	/* But it hurts */
	take_hit(damroll(3, 6), "shapeshifting stress");
	shapechange(SHAPE_VAMPIRE);
	ident = TRUE;
	break;
      }
      
    }
  
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* The item has been tried */
  object_tried(o_ptr);
  
  /* An identification was made */
  if (ident && !object_aware_p(o_ptr))
    {
      object_aware(o_ptr);
      gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
      if (o_ptr->number > 1) apply_autoinscription(o_ptr);
    }
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  
  /* Potions can feed the player */
  (void)set_food(p_ptr->food + o_ptr->pval);
  
  
  /* Destroy a potion in the pack */
  if (item >= 0)
    {
      inven_item_increase(item, -1);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
  
  /* Destroy a potion on the floor */
  else
    {
      floor_item_increase(0 - item, -1);
      floor_item_describe(0 - item);
      floor_item_optimize(0 - item);
    }

  /* Forget the item_tester_tval restriction */
  item_tester_tval = 0;
  
}


/**
 * Read a scroll (from the pack or floor).
 *
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a turn, though.
 */
void do_cmd_read_scroll(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int item, k, used_up, ident, lev, dir;
  
  object_type *o_ptr;
  
  cptr q, s;

  u32b proof_flag;  
  
  /* Check some conditions */
  if (p_ptr->blind)
    {
      msg_print("You can't see anything.");
      return;
    }
  if (no_lite())
    {
      msg_print("You have no light to read by.");
      return;
    }
  if (p_ptr->confused)
    {
      msg_print("You are too confused!");
      return;
    }
  
  
  /* Restrict choices to scrolls */
  item_tester_tval = TV_SCROLL;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Read which scroll? ";
      s = "You have no scrolls to read.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Not identified yet */
  ident = FALSE;
  
  /* Object level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Assume the scroll will get used up */
  used_up = TRUE;
  
  /* Analyze the scroll */
  switch (o_ptr->sval)
    {
    case SV_SCROLL_DARKNESS:
      {
	if (!p_ptr->no_blind)
	  {
	    (void)set_blind(p_ptr->blind + 3 + randint(5));
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	if (unlite_area(10, 3)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_AGGRAVATE_MONSTER:
      {
	msg_print("There is a high pitched humming noise.");
	(void) aggravate_monsters(1, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_CURSE_ARMOR:
      {
	if (curse_armor()) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_CURSE_WEAPON:
      {
	if (curse_weapon()) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_SUMMON_MONSTER:
      {
	sound(MSG_SUM_MONSTER);
	for (k = 0; k < randint(3); k++)
	  {
	    if (summon_specific(py, px, FALSE, p_ptr->depth, 0))
	      {
		ident = TRUE;
	      }
	  }
	break;
      }
      
    case SV_SCROLL_SUMMON_UNDEAD:
      {
	sound(MSG_SUM_UNDEAD);
	for (k = 0; k < randint(3); k++)
	  {
	    if (summon_specific(py, px, FALSE, p_ptr->depth, 
				SUMMON_UNDEAD))
	      {
		ident = TRUE;
	      }
		      }
	break;
      }
      
    case SV_SCROLL_TRAP_CREATION:
      {
	if (trap_creation()) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_PHASE_DOOR:
      {
	teleport_player(10, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_TELEPORT:
      {
	teleport_player(100, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_TELEPORT_LEVEL:
      {
	(void)teleport_player_level(TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_WORD_OF_RECALL:
      {
	if (!word_recall(rand_int(20) + 15))
	  used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_IDENTIFY:
      {
	ident = TRUE;
	if (!ident_spell()) used_up = FALSE;
	break;
      }

    case SV_SCROLL_REVEAL_CURSES:
      {
	ident = TRUE;
	if (!identify_fully()) used_up = FALSE;
	break;
      }
 
    case SV_SCROLL_BRANDING:
      {
	ident = TRUE;
	if (!brand_missile(0, 0)) used_up = FALSE;
	break;
      }
      
    case SV_SCROLL_FRIGHTENING:
      {
	if (!get_aim_dir(&dir)) return;
	if (fear_monster(dir, p_ptr->lev + 5)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_REMOVE_CURSE:
      {
	if (remove_curse())
	  {
	    msg_print("You feel as if someone is watching over you.");
	  }
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_STAR_REMOVE_CURSE:
      {
	if (remove_curse_good())
	  {
	    msg_print("You feel as if someone is watching over you.");
	  }
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_ENCHANT_ARMOR:
      {
	ident = TRUE;
	if (!enchant_spell(0, 0, 1)) used_up = FALSE;
	break;
      }
      
    case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
      {
	if (!enchant_spell(1, 0, 0)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
      {
	if (!enchant_spell(0, 1, 0)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_STAR_RECHARGING:
      {
	if (!recharge(200)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_STAR_ENCHANT_ARMOR:
      {
	if (!enchant_spell(0, 0, randint(3) + 2)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_STAR_ENCHANT_WEAPON:
      {
	if (!enchant_spell(randint(3), randint(3), 0)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_RECHARGING:
      {
	if (!recharge(130)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_LIGHT:
      {
	if (lite_area(damroll(2, 8), 2)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_MAPPING:
      {
	map_area(0, 0, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DETECT_GOLD:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_treasure(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	if (detect_objects_gold(DETECT_RAD_DEFAULT, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DETECT_ITEM:
      {
	if (detect_objects_normal(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DETECT_TRAP:
      {
	if (detect_traps(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DETECT_DOOR:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_doors(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	if (detect_stairs(DETECT_RAD_DEFAULT, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DETECT_INVIS:
      {
	if (detect_monsters_invis(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_SATISFY_HUNGER:
      {
	if (set_food(PY_FOOD_MAX - 1)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_BLESSING:
      {
	if (set_blessed(p_ptr->blessed + randint(12) + 6)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_HOLY_CHANT:
      {
	if (set_blessed(p_ptr->blessed + randint(24) + 12)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_HOLY_PRAYER:
      {
	if (set_blessed(p_ptr->blessed + randint(48) + 24)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_MONSTER_CONFUSION:
      {
	if (!(p_ptr->special_attack & (ATTACK_CONFUSE)))
	  {
	    msg_print("Your hands begin to glow.");
	    p_ptr->special_attack |= (ATTACK_CONFUSE);
	    ident = TRUE;
	    p_ptr->redraw |= PR_STATUS;
	  }
	break;
      }
      
    case SV_SCROLL_PROTECTION_FROM_EVIL:
      {
	k = 3 * p_ptr->lev;
	if (set_protevil(p_ptr->protevil + randint(25) + k)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_RUNE_OF_PROTECTION:
      {
	/* Use up scroll only if warding_glyph is created. */
	if (!lay_rune(RUNE_PROTECT)) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DOOR_DESTRUCTION:
      {
	if (destroy_doors_touch()) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_STAR_DESTRUCTION:
      {
	destroy_area(py, px, 15, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_DISPEL_UNDEAD:
      {
	if (dispel_undead(60)) ident = TRUE;
	break;
      }
      
    case SV_SCROLL_GENOCIDE:
      {
	if (!genocide()) used_up = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_MASS_GENOCIDE:
      {
	(void)mass_genocide();
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_ACQUIREMENT:
      {
	acquirement(py, px, 1, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_STAR_ACQUIREMENT:
      {
	acquirement(py, px, randint(2) + 1, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_SCROLL_ELE_ATTACKS:		/* -LM- */
      {
	k = rand_int(5);
	
	/* Give an elemental attack for 400 turns. */
	if (k == 0) set_ele_attack(ATTACK_ACID, 400);
	if (k == 1) set_ele_attack(ATTACK_ELEC, 400);
	if (k == 2) set_ele_attack(ATTACK_FIRE, 400);
	if (k == 3) set_ele_attack(ATTACK_COLD, 400);
	if (k == 4) set_ele_attack(ATTACK_POIS, 400);
	ident = TRUE;
	
	break;
      }
    case SV_SCROLL_ACID_PROOF:
      {
	proof_flag = OF_ACID_PROOF;
	if (!el_proof(proof_flag)) used_up = FALSE;
	ident = TRUE;
	break;
      }

    case SV_SCROLL_ELEC_PROOF:
      {
	proof_flag = OF_ELEC_PROOF;
	if (!el_proof(proof_flag)) used_up = FALSE;
	ident = TRUE;
	break;
      }

    case SV_SCROLL_FIRE_PROOF:
      {
	proof_flag = OF_FIRE_PROOF;
	if (!el_proof(proof_flag)) used_up = FALSE;
	ident = TRUE;
	break;
      }

    case SV_SCROLL_COLD_PROOF:
      {
	proof_flag = OF_COLD_PROOF;
	if (!el_proof(proof_flag)) used_up = FALSE;
	ident = TRUE;
	break;
      }

    }
  
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* The item was tried */
  object_tried(o_ptr);
  
  /* An identification was made */
  if (ident && !object_aware_p(o_ptr))
    {
      object_aware(o_ptr);
      gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
      if (o_ptr->number > 1) apply_autoinscription(o_ptr);
    }
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  
  /* Hack -- allow certain scrolls to be "preserved" */
  if (!used_up) 
    {
      item_tester_tval = 0;
      return;
    } 
  
  /* Destroy a scroll in the pack */
  if (item >= 0)
    {
      inven_item_increase(item, -1);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
  
  /* Destroy a scroll on the floor */
  else
    {
      floor_item_increase(0 - item, -1);
      floor_item_describe(0 - item);
      floor_item_optimize(0 - item);
    }

  /* Forget the item_tester_tval restriction */
  item_tester_tval = 0;
  
}


/**
 * Use a staff.  Staffs may be fully identified through use.
 *
 * One charge of one staff disappears.
 *
 * Hack -- staffs of identify can be "cancelled".
 */
void do_cmd_use_staff(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int item, ident, chance, k, lev;
  
  object_type *o_ptr;
  object_kind *k_ptr;
  
  /* Hack -- let staffs of identify get aborted */
  bool use_charge = TRUE;
  
  cptr q, s;
  
  
  /* Restrict choices to staffs */
  item_tester_tval = TV_STAFF;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Use which staff? ";
      s = "You have no staff to use.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  
  /* Mega-Hack -- refuse to use a pile from the ground */
  if ((item < 0) && (o_ptr->number > 1))
    {
      msg_print("You must first pick up the staffs.");
      item_tester_tval = 0;
      return;
    }
  
  k_ptr = &k_info[o_ptr->k_idx];
  
  /* Take a turn. */
  p_ptr->energy_use = 100;
  
  /* Not identified yet */
  ident = FALSE;
  
  /* Extract the item level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Base chance of success */
  chance = p_ptr->skill_dev;
  
  /* Confusion hurts skill */
  if (p_ptr->confused) chance = chance / 2;
  
  /* High level objects are harder */
  chance = chance - ((lev > 50) ? 50 : lev);
  
  /* Give everyone a (slight) chance */
  if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
    {
      chance = USE_DEVICE;
    }
  
  /* Roll for usage */
  if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
    {
      if (flush_failure) flush();
      msg_print("You failed to use the staff properly.");
      item_tester_tval = 0;
      return;
    }
  
  /* Notice empty staffs */
  if (o_ptr->pval <= 0)
    {
      if (flush_failure) flush();
      msg_print("The staff has no charges left.");
      o_ptr->ident |= (IDENT_EMPTY);
      
      /* Combine / Reorder the pack (later) */
      p_ptr->notice |= (PN_COMBINE | PN_REORDER);
      p_ptr->window |= (PW_INVEN);
      item_tester_tval = 0;
      return;
    }
  
  
  /* Sound */
  sound(MSG_USE_STAFF);
  
  
  /* Analyze the staff */
  switch (o_ptr->sval)
    {
    case SV_STAFF_DARKNESS:
      {
	if (!p_ptr->no_blind)
	  {
	    if (set_blind(p_ptr->blind + 3 + randint(5))) ident = TRUE;
	  }
	else if (object_aware_p(o_ptr)) notice_obj(OF_SEEING, 0);
	if (unlite_area(10, 3)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_SLOWNESS:
      {
	if (set_slow(p_ptr->slow + randint(30) + 15)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_HASTE_MONSTERS:
      {
	if (speed_monsters()) ident = TRUE;
	break;
      }
      
    case SV_STAFF_SUMMONING:
      {
	sound(MSG_SUM_MONSTER);
	for (k = 0; k < randint(4); k++)
	  {
	    if (summon_specific(py, px, FALSE, p_ptr->depth, 0))
	      {
		ident = TRUE;
	      }
	  }
	break;
      }
      
    case SV_STAFF_TELEPORTATION:
      {
	teleport_player(100, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_IDENTIFY:
      {
	if (!ident_spell()) use_charge = FALSE;
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_REMOVE_CURSE:
      {
	if (remove_curse())
	  {
	    if (!p_ptr->blind)
	      {
		msg_print("The staff glows blue for a moment...");
	      }
	  }
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_STARLIGHT:
      {
	/* Message. */
	if (!p_ptr->blind)
	  {
	    msg_print("The staff glitters with unearthly light.");
	  }
	
	/* Starbursts everywhere. */
	do_starlight(rand_int(8) + 7, 12, FALSE);
	
	/* Hard not to identify. */
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_LITE:
      {
	if (lite_area(damroll(2, 8), 2)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_MAPPING:
      {
	map_area(0, 0, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECT_GOLD:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_treasure(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	if (detect_objects_gold(DETECT_RAD_DEFAULT, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECT_ITEM:
      {
	if (detect_objects_normal(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECT_TRAP:
      {
	if (detect_traps(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECT_DOOR:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_doors(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	if (detect_stairs(DETECT_RAD_DEFAULT, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECT_INVIS:
      {
	if (detect_monsters_invis(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECT_EVIL:
      {
	if (detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_CURE_MEDIUM:
      {
	if (hp_player(randint(20) + 10)) ident = TRUE;
	(void)set_cut(p_ptr->cut - 10);
	break;
      }
      
    case SV_STAFF_CURING:
      {
	if (set_blind(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_HEALING:
      {
	if (hp_player(300)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_BANISHMENT:
      {
	if (banish_evil(80)) 
	  {
	    ident = TRUE;
	    msg_print("A mighty force drives away evil!");
	  }
	break;
      }
      
    case SV_STAFF_SLEEP_MONSTERS:
      {
	if (sleep_monsters(p_ptr->lev + 10)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_SLOW_MONSTERS:
      {
	if (slow_monsters(p_ptr->lev + 10)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_SPEED:
      {
	if (!p_ptr->fast)
	  {
	    if (set_fast(randint(30) + 15)) ident = TRUE;
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	break;
      }
      
    case SV_STAFF_PROBING:
      {
	probing();
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_DISPEL_EVIL:
      {
	if (dispel_evil(60)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_POWER:
      {
	if (dispel_monsters(100)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_HOLINESS:
      {
	if (dispel_evil(120)) ident = TRUE;
	k = 2 * p_ptr->lev;
	if (set_protevil(p_ptr->protevil + randint(25) + k)) ident = TRUE;
	if (set_poisoned((p_ptr->poisoned / 2)-10)) ident = TRUE;
	if (set_afraid(0)) ident = TRUE;
	if (hp_player(50)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	break;
      }
      
    case SV_STAFF_EARTHQUAKES:
      {
	earthquake(py, px, 10, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_DESTRUCTION:
      {
	destroy_area(py, px, 15, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_DETECTION:
      {
	detect_all(DETECT_RAD_DEFAULT, TRUE);
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_MSTORM:
      {
	
	msg_print("Mighty magics rend your enemies!");
	fire_sphere(GF_MANA, 0,
		    randint(75) + 125, 5, 20);
	if (!(check_ability(SP_DEVICE_EXPERT)))
	  {
	    (void)take_hit(20, "unleashing magics too mighty to control");
	  }
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_STARBURST:
      {
	msg_print("Light bright beyond enduring dazzles your foes!");
	fire_sphere(GF_LITE, 0,
		    randint(67) + 100, 5, 20);
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_MASS_CONFU:
      {
	if (confu_monsters(p_ptr->lev + 30)) ident = TRUE;
	break;
      }
      
      
    case SV_STAFF_GANDALF:
      {
	/* Message. */
	if (!p_ptr->blind)
	  {
	    msg_print("The staff blazes with unearthly light.");
	  }
	
	/* (large) Starbursts everywhere. */
	do_starlight(rand_int(8) + 7, 30, TRUE);
	
	/* Hard not to identify. */
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_WINDS:
      {
	/* Raise a storm. */
	msg_print("A howling whirlwind rises in wrath around you.");
	fire_sphere(GF_FORCE, 0, randint(100) + 100, 6, 20);
	
	/* Whisk around the player and nearby monsters.  This is 
	 * actually kinda amusing to see...
	 */
	fire_ball(GF_AWAY_ALL, 0, 12, 6, FALSE);
	teleport_player(6, TRUE);
	
	/* Identify */
	ident = TRUE;
	break;
      }
      
    case SV_STAFF_MENELTARMA:
      {
	msg_print("You envoke binding magics upon the undead nearby!");
	
	/* Attempt to Hold all undead in LOS. */
	if (hold_undead()) ident = TRUE;
	
	break;
      }
      
    case SV_STAFF_RADAGAST:
      {
	/* Try to learn about the dungeon and traps in it from animals. */
	if (listen_to_natural_creatures())
	  msg_print("You listen and learn from natural creatures.");
	else msg_print("You found no animals nearby to learn from.");
	
	/* Identify */
	ident = TRUE;
	break;
      }
    }
  
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  p_ptr->window |= (PW_INVEN);
  
  /* Tried the item */
  object_tried(o_ptr);
  
  /* An identification was made */
  if (ident && !object_aware_p(o_ptr))
    {
      object_aware(o_ptr);
      gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
      apply_autoinscription(o_ptr);
      k_ptr->known_effect = TRUE;
    }
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  
  /* Hack -- some uses are "free" */
  if (!use_charge) 
    {
      item_tester_tval = 0;
      return;
    }
  
  
  /* Use a single charge */
  o_ptr->pval--;
  
  /* XXX Hack -- unstack if necessary */
  if ((item >= 0) && (o_ptr->number > 1))
    {
      object_type *i_ptr;
      object_type object_type_body;
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Obtain a local object */
      object_copy(i_ptr, o_ptr);
      
      /* Modify quantity */
      i_ptr->number = 1;
      
      /* Restore the charges */
      o_ptr->pval++;
      
      /* Unstack the used item */
      o_ptr->number--;
      p_ptr->total_weight -= i_ptr->weight;
      item = inven_carry(i_ptr);
      
      /* Message */
      msg_print("You unstack your staff.");
    }
  
  /* Describe charges in the pack */
  if (item >= 0)
    {
      inven_item_charges(item);
    }
  
  /* Describe charges on the floor */
  else
    {
      floor_item_charges(0 - item);
    }
  
  
  
  
  /* Make it possible to fully identify staffs through use. */
  if ((object_known_p(o_ptr)) && (!k_ptr->known_effect) &&
      (rand_int((check_ability(SP_DEVICE_EXPERT)) ? 25 : 35) == 0))
    {
      /* Mark all objects of that type as fully known. */
      k_ptr->known_effect = TRUE;
      
      /* If we actually give more information now, let the player know. */
      if (strlen(obj_special_info[1][o_ptr->sval]))
	{
	  if (artifact_p(o_ptr))
	    msg_format("You feel you know more about the Staff %s.",
		       a_name + a_info[o_ptr->name1].name);
	  
	  else msg_format("You feel you know more about staffs of %s.",
			  k_name + k_ptr->name);
	}
    }
  
  /* Forget the item_tester_tval restriction */
  item_tester_tval = 0;
  
}


/**
 * Aim a wand (from the pack or floor).  Wands may be fully identified 
 * through use.
 *
 * Use a single charge from an item or stack.
 *
 * There are no wands which can "destroy" themselves, in the inventory
 * or on the ground, so we can ignore this possibility.  Note that this
 * required giving "wand of wonder" the ability to ignore destruction
 * by electric balls.
 *
 * All wands can be "cancelled" at the "Direction?" prompt for free.
 *
 * Note that the basic "bolt" wands do slightly less damage than the
 * basic "bolt" rods, but the basic "ball" wands do the same damage
 * as the basic "ball" rods.
 */
void do_cmd_aim_wand(void)
{
  int item, lev, ident, chance, dir, sval;
  int plev = p_ptr->lev;
  
  object_type *o_ptr;
  object_kind *k_ptr;
  
  cptr q, s;
  
  TARGET_DECLARE
    
    /* Restrict choices to wands */
    item_tester_tval = TV_WAND;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Aim which wand? ";
      s = "You have no wand to aim.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  
  /* Allow direction to be cancelled for free */
  if (!get_aim_dir(&dir)) 
    {
      item_tester_tval = 0;
      return;
    }
  
  k_ptr = &k_info[o_ptr->k_idx];
  
  /* Take a turn. */
  p_ptr->energy_use = 100;
  
  /* Not identified yet */
  ident = FALSE;
  
  /* Get the level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Base chance of success */
  chance = p_ptr->skill_dev;
  
  /* Confusion hurts skill */
  if (p_ptr->confused) chance = chance / 2;
  
  /* High level objects are harder */
  chance = chance - ((lev > 50) ? 50 : lev);
  
  /* Give everyone a (slight) chance */
  if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
    {
      chance = USE_DEVICE;
    }
  
  /* Roll for usage */
  if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
    {
      if (flush_failure) flush();
      msg_print("You failed to use the wand properly.");
      item_tester_tval = 0;
      return;
    }
  
  /* The wand is already empty! */
  if (o_ptr->pval <= 0)
    {
      if (flush_failure) flush();
      msg_print("The wand has no charges left.");
      o_ptr->ident |= (IDENT_EMPTY);
      
      /* Combine / Reorder the pack (later) */
      p_ptr->notice |= (PN_COMBINE | PN_REORDER);
      item_tester_tval = 0;
      return;
    }
  
  
  /* Sound */
  sound(MSG_ZAP_ROD);
  
  
  
  /* XXX Hack -- Extract the "sval" effect */
  sval = o_ptr->sval;
  
  
  /* XXX Hack -- Wand of wonder can do anything before it */
  if (sval == SV_WAND_WONDER) sval = rand_int(SV_WAND_WONDER);
  
  /* Analyze the wand */
  switch (sval)
    {
    case SV_WAND_HEAL_MONSTER:
      {
	if (heal_monster(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_HASTE_MONSTER:
      {
	if (speed_monster(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_CLONE_MONSTER:
      {
	if (clone_monster(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_TELEPORT_AWAY:
      {
	if (teleport_monster(dir, 55 + (plev/2))) ident = TRUE;
	break;
      }
      
    case SV_WAND_DISARMING:
      {
	if (disarm_trap(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_DOOR_DEST:
      {
	if (destroy_door(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_STONE_TO_MUD:
      {
	if (wall_to_mud(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_LITE:
      {
	msg_print("A line of blue shimmering light appears.");
	lite_line(dir);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_SLEEP_MONSTER:
      {
	if (sleep_monster(dir, plev + 15)) ident = TRUE;
	break;
      }
      
    case SV_WAND_SLOW_MONSTER:
      {
	if (slow_monster(dir, plev + 15)) ident = TRUE;
	break;
      }
      
    case SV_WAND_CONFUSE_MONSTER:
      {
	if (confuse_monster(dir, plev + 15)) ident = TRUE;
	break;
      }
      
    case SV_WAND_FEAR_MONSTER:
      {
	if (fear_monster(dir, plev + 20)) ident = TRUE;
	break;
      }
      
    case SV_WAND_DRAIN_LIFE:
      {
	if (drain_life(dir, 50 + plev)) ident = TRUE;
	break;
      }
      
    case SV_WAND_POLYMORPH:
      {
	if (poly_monster(dir)) ident = TRUE;
	break;
      }
      
    case SV_WAND_STINKING_CLOUD:
      {
	fire_ball(GF_POIS, dir, 12, 2, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_MAGIC_MISSILE:
      {
	fire_bolt_or_beam(20, GF_MANA, dir, damroll(2, 6));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ACID_BOLT:
      {
	fire_bolt_or_beam(plev, GF_ACID, dir, damroll(5 + plev / 10, 8));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ELEC_BOLT:
      {
	fire_bolt_or_beam(plev, GF_ELEC, dir, damroll(3 + plev / 14, 8));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_FIRE_BOLT:
      {
	fire_bolt_or_beam(plev, GF_FIRE, dir, damroll(6 + plev / 8, 8));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_COLD_BOLT:
      {
	fire_bolt_or_beam(plev, GF_COLD, dir, damroll(4 + plev / 12, 8));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ACID_BALL:
      {
	fire_ball(GF_ACID, dir, 60 + 3 * plev / 5, 3, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ELEC_BALL:
      {
	fire_ball(GF_ELEC, dir, 40 + 3 * plev / 5, 3, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_FIRE_BALL:
      {
	fire_ball(GF_FIRE, dir, 70 + 3 * plev / 5, 3, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_COLD_BALL:
      {
	fire_ball(GF_COLD, dir, 50 + 3 * plev / 5, 3, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_WONDER:
      {
	msg_print("Oops.  Wand of wonder activated.");
	break;
      }
      
    case SV_WAND_DRAGON_FIRE:
      {
	fire_arc(GF_FIRE, dir, 160, 7, 90);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_DRAGON_COLD:
      {
	fire_arc(GF_COLD, dir, 160, 7, 90);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_DRAGON_BREATH:
      {
	int tmp = randint(5);
	
	if (tmp == 1) fire_arc(GF_ACID, dir, 200, 9, 90);
	if (tmp == 2) fire_arc(GF_ELEC, dir, 180, 9, 90);
	if (tmp == 3) fire_arc(GF_COLD, dir, 190, 9, 90);
	if (tmp == 4) fire_arc(GF_FIRE, dir, 210, 9, 90);
	if (tmp == 5) fire_arc(GF_POIS, dir, 200, 7, 120);
	
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ANNIHILATION:
      {
	if (drain_life(dir, 100 + randint(plev * 4))) ident = TRUE;
	break;
      }
      
    case SV_WAND_STRIKING:
      {
	fire_bolt(GF_METEOR, dir, damroll(10 + plev / 3, 9));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_STORMS:
      {
	fire_bolt(GF_STORM, dir, damroll(25 + plev / 5, 4));
	ident = TRUE;
	break;
      }
      
      
    case SV_WAND_SHARD_BOLT:
      {
	fire_bolt(GF_SHARD, dir, damroll(4 + plev / 14, 8));
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ILKORIN:
      {
	msg_print("Deadly venom spurts and steams from your wand.");
	TARGET_PRESERVE
	  fire_bolt(GF_POIS, dir, damroll(plev / 2, 11));
	TARGET_RESTORE
	  fire_cloud(GF_POIS, dir, 30, 6);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_SARUMAN:
      {
	msg_print("You speak soft, beguiling words.");
	
	if (rand_int(2) == 0) 
	  {
	    if (slow_monster(dir, plev * 2)) ident = TRUE;
	  }
	if (rand_int(2) == 0) 
	  {
	    if (confuse_monster(dir, plev * 2)) ident = TRUE;
	  }
	else
	  {
	    if (sleep_monster(dir, plev * 2)) ident = TRUE;
	  }
	
	ident = TRUE;
	break;
      }
      
    case SV_WAND_UNMAKING:
      {
	msg_print("You envoke the powers of Unmaking!");
	unmake(dir);
	ident = TRUE;
	break;
      }
      
    case SV_WAND_ULPION:
      {
	msg_print("You raise a foam-crested tidal wave."); 
	fire_arc(GF_WATER, dir, 3 * plev + randint(100), 14, 90);
	ident = TRUE;
	break;
      }
    }
  
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Mark it as tried */
  object_tried(o_ptr);
  
  /* Apply identification */
  if (ident && !object_aware_p(o_ptr))
    {
      object_aware(o_ptr);
      gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
      apply_autoinscription(o_ptr);
      k_ptr->known_effect = TRUE;
    }
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  
  /* Use a single charge */
  o_ptr->pval--;
  
  
  /* Describe the charges in the pack */
  if (item >= 0)
    {
      inven_item_charges(item);
    }
  
  /* Describe the charges on the floor */
  else
    {
      floor_item_charges(0 - item);
    }
  
  
  /* Make it possible to fully identify wands through use. */
  if ((object_known_p(o_ptr)) && (!k_ptr->known_effect) &&
      (rand_int((check_ability(SP_DEVICE_EXPERT)) ? 30 : 40) == 0))
    {
      /* Mark all objects of that type as fully known. */
      k_ptr->known_effect = TRUE;
      
      /* If we actually give more information now, let the player know. */
      if (strlen(obj_special_info[2][o_ptr->sval]))
	{
	  if (artifact_p(o_ptr))
	    msg_format("You feel you know more about the Wand %s.",
		       a_name + a_info[o_ptr->name1].name);
	  
	  else msg_format("You feel you know more about wands of %s.",
			  k_name + k_ptr->name);
	}
    }

  /* Forget the item_tester_tval restriction */
  item_tester_tval = 0;
  
}


/**
 * Activate (zap) a Rod.    Rods may be fully identified through use 
 * (although it's not easy).  Rods now use timeouts to determine charging 
 * status, and pvals have become the cost of zapping a rod (how long it 
 * takes between zaps).  Pvals are defined for each rod in k_info. -LM-
 *
 * Hack -- rods of perception/genocide can be "cancelled"
 * All rods can be cancelled at the "Direction?" prompt
 */
void do_cmd_zap_rod(void)
{
  int item, ident, chance, dir, lev;
  int plev = p_ptr->lev;
  
  object_type *o_ptr;
  object_kind *k_ptr;
  
  /* Hack -- let perception get aborted */
  bool use_charge = TRUE;
  
  cptr q, s;
  
  
  /* Restrict choices to rods */
  item_tester_tval = TV_ROD;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Zap which rod? ";
      s = "You have no rod to zap.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  /* Get the object kind. */
  k_ptr = &k_info[o_ptr->k_idx];
  
  
  /* Get a direction (unless KNOWN not to need it) */
  if (((o_ptr->sval >= SV_ROD_MIN_DIRECTION) &&
       (o_ptr->sval <= SV_ROD_GLAURUNGS)) ||
      (o_ptr->sval == SV_ROD_DELVING) ||
      !object_aware_p(o_ptr))
    {
      /* Get a direction, allow cancel */
      if (!get_aim_dir(&dir)) 
	{
	  item_tester_tval = 0;
	  return;
	}
    }
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Not identified yet */
  ident = FALSE;
  
  /* Extract the item level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Base chance of success */
  chance = p_ptr->skill_dev;
  
  /* Confusion hurts skill */
  if (p_ptr->confused) chance = chance / 2;
  
  /* High level objects are harder */
  chance = chance - ((lev > 50) ? 50 : lev);
  
  /* Give everyone a (slight) chance */
  if ((chance < USE_DEVICE) && (rand_int(USE_DEVICE - chance + 1) == 0))
    {
      chance = USE_DEVICE;
    }
  
  /* Roll for usage */
  if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
    {
      if (flush_failure) flush();
      msg_print("You failed to use the rod properly.");
      item_tester_tval = 0;
      return;
    }
  
  /* A single rod is still charging */
  if ((o_ptr->number == 1) && (o_ptr->timeout))
    {
      if (flush_failure) flush();
      msg_print("The rod is still charging.");
      item_tester_tval = 0;
      return;
    }
  /* A stack of rods lacks enough energy. */
  else if ((o_ptr->number > 1) && (o_ptr->timeout > o_ptr->pval - k_ptr->pval))
    {
      if (flush_failure) flush();
      msg_print("The rods are all still charging.");
      item_tester_tval = 0;
      return;
    }
  
  k_ptr = &k_info[o_ptr->k_idx];
  
  /* Sound */
  sound(MSG_ZAP_ROD);
  
  /* Increase the timeout by the rod kind's pval. */
  o_ptr->timeout += k_ptr->pval;
  
  /* Analyze the rod */
  switch (o_ptr->sval)
    {
    case SV_ROD_DETECT_TRAP:
      {
	if (detect_traps(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	break;
      }
      
    case SV_ROD_DETECT_DOOR:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	if (detect_doors(DETECT_RAD_DEFAULT, TRUE)) ident = TRUE;
	if (detect_stairs(DETECT_RAD_DEFAULT, FALSE)) ident = TRUE;
	break;
      }
      
    case SV_ROD_IDENTIFY:
      {
	ident = TRUE;
	if (!ident_spell()) use_charge = FALSE;
	break;
      }
      
    case SV_ROD_RECALL:
      {
#if 1
	if (!word_recall(rand_int(20) + 15))
	  use_charge = FALSE;
#else
	if (p_ptr->word_recall == 0)
	  {
	    msg_print("The air about you becomes charged...");
	    p_ptr->word_recall = 15 + randint(20);
	  }
	else
	  {
	    msg_print("A tension leaves the air around you...");
	    p_ptr->word_recall = 0;
	  }
	p_ptr->redraw |= PR_STATUS;
#endif
	ident = TRUE;
	break;
      }
      
    case SV_ROD_ILLUMINATION:
      {
	if (lite_area(damroll(2, 8), 2)) ident = TRUE;
	break;
      }
      
    case SV_ROD_MAPPING:
      {
	map_area(0, 0, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_ROD_PROBING:
      {
	probing();
	ident = TRUE;
	break;
      }
      
    case SV_ROD_CURING:
      {
	if (set_blind(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	break;
      }
      
    case SV_ROD_HEALING:
      {
	if (hp_player(500)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	break;
      }
      
    case SV_ROD_RESTORATION:
      {
	if (restore_level()) ident = TRUE;
	if (do_res_stat(A_STR)) ident = TRUE;
	if (do_res_stat(A_INT)) ident = TRUE;
	if (do_res_stat(A_WIS)) ident = TRUE;
	if (do_res_stat(A_DEX)) ident = TRUE;
	if (do_res_stat(A_CON)) ident = TRUE;
	if (do_res_stat(A_CHR)) ident = TRUE;
	break;
      }
      
    case SV_ROD_SPEED:
      {
	if (!p_ptr->fast)
	  {
	    if (set_fast(randint(30) + 15)) ident = TRUE;
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	break;
      }
      
    case SV_ROD_TELEPORT_AWAY:
      {
	if (teleport_monster(dir, 45 + (plev/3))) ident = TRUE;
	break;
      }
      
    case SV_ROD_DISARMING:
      {
	if (disarm_trap(dir)) ident = TRUE;
	break;
      }
      
    case SV_ROD_LITE:
      {
	msg_print("A line of blue shimmering light appears.");
	lite_line(dir);
	ident = TRUE;
	break;
      }
      
    case SV_ROD_SLEEP_MONSTER:
      {
	if (sleep_monster(dir, plev + 10)) ident = TRUE;
	break;
      }
      
    case SV_ROD_SLOW_MONSTER:
      {
	if (slow_monster(dir, plev + 10)) ident = TRUE;
	break;
      }
      
    case SV_ROD_DRAIN_LIFE:
      {
	if (drain_life(dir, 45 + 3 * plev / 2)) ident = TRUE;
	break;
      }
      
    case SV_ROD_POLYMORPH:
      {
	if (poly_monster(dir)) ident = TRUE;
	break;
      }
      
    case SV_ROD_ACID_BOLT:
      {
	fire_bolt(GF_ACID, dir, damroll(6 + plev / 10, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_ELEC_BOLT:
      {
	fire_bolt(GF_ELEC, dir, damroll(4 + plev / 14, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_FIRE_BOLT:
      {
	fire_bolt(GF_FIRE, dir, damroll(7 + plev / 8, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_COLD_BOLT:
      {
	fire_bolt(GF_COLD, dir, damroll(5 + plev / 12, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_ACID_BALL:
      {
	fire_ball(GF_ACID, dir, 60 + 4 * plev / 5, 1, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_ROD_ELEC_BALL:
      {
	fire_ball(GF_ELEC, dir, 40 + 4 * plev / 5, 1, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_ROD_FIRE_BALL:
      {
	fire_ball(GF_FIRE, dir, 70 + 4 * plev / 5, 1, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_ROD_COLD_BALL:
      {
	fire_ball(GF_COLD, dir, 50 + 4 * plev / 5, 1, FALSE);
	ident = TRUE;
	break;
      }
      
    case SV_ROD_LIGHTINGSTRIKE:
      {
	fire_bolt_or_beam(plev / 2 - 10, GF_ELEC, dir, 
			  damroll(18 + plev / 3, 8));
	ident = TRUE;
	break;
      }

    case SV_ROD_NORTHWINDS:
      {
	fire_bolt_or_beam(plev / 2 - 10, GF_COLD, dir, 
			  damroll(21 + plev / 3, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_DRAGONFIRE:
      {
	fire_bolt_or_beam(plev/2 - 10, GF_FIRE, dir, 
			  damroll(24 + plev / 3, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_GLAURUNGS:
      {
	fire_bolt_or_beam(plev / 2 - 10, GF_ACID, dir, 
			  damroll(27 + plev / 3, 8));
	ident = TRUE;
	break;
      }
      
    case SV_ROD_ROUSE_LEVEL:
      {
	msg_print("A mighty blast of horns shakes the air, and you hear stirring everwhere!");
	(void) aggravate_monsters(1, TRUE);
	ident = TRUE;
	break;
      }
      
      
    case SV_ROD_DELVING:
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
	    (void) wall_to_mud_hack(dir, 160 + randint(240));
	  }
	ident = TRUE;
	break;
      }
      
    case SV_ROD_SHADOW:
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
	ident = TRUE;
	break;
      }
      
    case SV_ROD_AIR:
      {
	msg_print("You raise your rod skyward and call upon the powers of Air.");
	ele_air_smite();
	ident = TRUE;
	break;
      }
      
    case SV_ROD_PORTALS:
      {
	msg_print("Choose a location to teleport to.");
	msg_print(NULL);
	dimen_door();
	ident = TRUE;
	break;
      }
      
    }
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Tried the object */
  object_tried(o_ptr);
  
  /* Successfully determined the object function */
  if (ident && !object_aware_p(o_ptr))
    {
      object_aware(o_ptr);
      gain_exp((lev + (p_ptr->lev >> 1)) / p_ptr->lev);
      apply_autoinscription(o_ptr);
      k_ptr->known_effect = TRUE;
    }
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  /* Hack -- deal with cancelled zap */
  if (!use_charge)
    {
      o_ptr->timeout -= k_ptr->pval;
      item_tester_tval = 0;
      return;
    }
  
  
  /* Make it possible to fully identify rods through use. */
  if ((object_known_p(o_ptr)) && (!k_ptr->known_effect) &&
      (rand_int((check_ability(SP_DEVICE_EXPERT)) ? 45 : 65) == 0))
    {
      /* Mark all objects of that type as fully known. */
      k_ptr->known_effect = TRUE;
      
      /* If we actually give more information now, let the player know. */
      if (strlen(obj_special_info[3][o_ptr->sval]))
	{
	  if (artifact_p(o_ptr))
	    msg_format("You feel you know more about the Rod %s.",
		       a_name + a_info[o_ptr->name1].name);
	  
	  else msg_format("You feel you know more about rods of %s.",
			  k_name + k_ptr->name);
	}
    }

  /* Forget the item_tester_tval restriction */
  item_tester_tval = 0;
  
}



/**
 * Hook to determine if an object is activatable.  Revised in Oangband.
 */
static bool item_tester_hook_activate(object_type *o_ptr)
{
  /* Big hack - only mages and necros can activate magestaffs */
  if (o_ptr->activation == ACT_MAGESTAFF)
    {
      if (!check_ability(SP_STRONG_MAGIC)) return (FALSE);
      if (mp_ptr->spell_stat != A_INT) return (FALSE);
    }

  /* Only objects with an activation can be activated. */
  if (o_ptr->activation) return (TRUE);
  
  /* Nope. */
  return (FALSE);
}





/**
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 *
 * Any object given an activation will perform the 
 * activation corresponding to that index when used.
 *
 * The player has a (4%) chance to learn more about any dragon armour 
 * activation.  If he does, detailed information about it will appear 
 * when the item is 'I'nspected. -LM-
 *
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 */
void do_cmd_activate(void)
{
  int item, i, k, dir, lev, chance;
  int plev = p_ptr->lev;
  
  object_type *o_ptr;
  
  cptr q, s;
  cptr missile_name = "";
  
  TARGET_DECLARE
    
    /* Prepare the hook */
    item_tester_hook = item_tester_hook_activate;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Activate which item? ";
      s = "You have nothing to activate.";
      if (!get_item(&item, q, s, (USE_EQUIP))) return;
    }

  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  
  /* Extract the item level */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Hack -- use artifact level instead */
  if (artifact_p(o_ptr)) lev = a_info[o_ptr->name1].level;
  
  /* Base chance of success */
  chance = p_ptr->skill_dev;
  
  /* Confusion hurts skill */
  if (p_ptr->confused) chance = chance / 2;
  
  /* High level objects are harder.  Altered in Oangband. */
  chance = chance - (((2 * lev / 3) > 75) ? 75 : 2 * lev / 3);

  
  /* Give everyone a (slight) chance, more for rings, amulets. */
  if ((chance < USE_DEVICE))
    {
      if ((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET))
	{
	  if (randint(3) >= 2) chance = USE_DEVICE * 2;
	}
      else if ((chance < USE_DEVICE) && 
	       (rand_int(USE_DEVICE - chance + 1) == 0))
	{
	  chance = USE_DEVICE;
	}
    }
  
  /* Roll for usage */
  if ((chance < USE_DEVICE) || (randint(chance) < USE_DEVICE))
    {
      if (flush_failure) flush();
      msg_print("You failed to activate it properly.");
      item_tester_tval = 0;
  
      /* Take a turn */
      p_ptr->energy_use = 100;
      return;
    }
  
  /* Check the recharge */
  if (o_ptr->timeout)
    {
      msg_print("It whines, glows and fades...");
      item_tester_tval = 0;
  
      /* Take a turn */
      p_ptr->energy_use = 100;
      return;
    }
  
  
  /* Activate the artifact */
  message(MSG_ZAP_ROD, 0, "You activate it...");
  
  /* Sound */
  sound(MSG_ACT_ARTIFACT);
  
  /* Choose effect. */
  switch (o_ptr->activation)
    {
    case ACT_GWINDOR:
      {
	msg_print("The lantern wells with clear light...");
	lite_area(damroll(2, 15), 3);
	o_ptr->timeout = rand_int(10) + 10;
	break;
      }
    case ACT_NIMPHELOS:
      {
	msg_print("The pearl shines brightly...");
	map_area(0, 0, FALSE);
	o_ptr->timeout = rand_int(40) + 40;
	break;
      }
    case ACT_AVALLONE:
      {
	/* Hack - 'show' affected region only with
	 * the first detect */
	msg_print("The palantir clouds, then clears...");
	wiz_lite(FALSE);
	(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	(void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	o_ptr->timeout = rand_int(120) + 120;
	break;
      }
    case ACT_CARLAMMAS:
      {
	msg_print("The amulet lets out a shrill wail...");
	k = 3 * p_ptr->lev;
	if (banish_evil(60)) 
	  msg_print("You thrust your evil enemies back!");
	(void)set_protevil(p_ptr->protevil + randint(25) + k);
	o_ptr->timeout = rand_int(225) + 225;
	break;
      }
    case ACT_INGWE:
      {
	msg_print("The amulet floods the area with goodness...");
	dispel_evil(p_ptr->lev * 4);
	o_ptr->timeout = rand_int(300) + 300;
	break;
      }
    case ACT_DWARVES:
      {
	msg_print("The silmaril shows the Light of the Trees...");
	wiz_lite(FALSE);
	(void)detect_all(DUNGEON_WID, TRUE);
	o_ptr->timeout = rand_int(200) + 200;
	break;
      }
    case ACT_BOROMIR:
      {
	msg_print("You wind a mighty blast; your enemies tremble!");
	(void)fear_monsters((3 * plev / 2) + 10);
	o_ptr->timeout = rand_int(40) + 40;
	break;
      }
    case ACT_ELESSAR:
      {
	msg_print("The elfstone glows deep green...");
	msg_print("You feel a warm tingling inside...");
	(void)hp_player(500);
	(void)set_cut(0);
	restore_level();
	o_ptr->timeout = 500;
	break;
      }
    case ACT_TULKAS:
      {
	msg_print("The ring glows brightly...");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint(75) + 75);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = rand_int(150) + 150;
	break;
      }
    case ACT_NARYA:
      {
	msg_print("The ring glows deep red...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_FIRE, dir, 225, 3, FALSE);
	o_ptr->timeout = rand_int(275) + 275;
	break;
      }
    case ACT_NENYA:
      {
	msg_print("The ring glows bright white...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_COLD, dir, 250, 3, FALSE);
	o_ptr->timeout = rand_int(325) + 325;
	break;
      }
    case ACT_VILYA:
      {
	msg_print("The ring glows deep blue...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_ELEC, dir, 275, 3, FALSE);
	o_ptr->timeout = rand_int(375) + 375;
	break;
      }
      /* The Stone of Lore is perilous, for the sake of game balance. */
    case ACT_STONE_LORE:
      {
	msg_print("The stone reveals hidden mysteries...");
	if (!ident_spell()) return;
	
	if (mp_ptr->spell_book)
	  {
	    /* Sufficient mana */
	    if (20 <= p_ptr->csp)
	      {
		/* Use some mana */
		p_ptr->csp -= 20;
	      }
	    
	    /* Over-exert the player */
	    else
	      {
		int oops = 20 - p_ptr->csp;
		
		/* No mana left */
		p_ptr->csp = 0;
		p_ptr->csp_frac = 0;
		
		/* Message */
		msg_print("You are too weak to control the stone!");
		
		/* Hack -- Bypass free action */
		(void)set_paralyzed(p_ptr->paralyzed + randint(5 * oops + 1));
		
		/* Confusing. */
		(void)set_confused(p_ptr->confused + randint(5 * oops + 1));
	      }
	    
	    /* Redraw mana */
	    p_ptr->redraw |= (PR_MANA);
	  }
	
	take_hit(damroll(1, 12), "perilous secrets");
	
	/* Confusing. */
	if (rand_int(5) == 0) 
	  (void)set_confused(p_ptr->confused + randint(10));
	
	/* Exercise a little care... */
	if (rand_int(20) == 0) take_hit(damroll(4, 10), "perilous secrets");
	o_ptr->timeout = 0;
	break;
      }
      
    case ACT_RAZORBACK:
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
	break;
      }
    case ACT_BLADETURNER:
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
	    (void)set_shero(p_ptr->shero + randint(50) + 50);
	    (void)set_blessed(p_ptr->blessed + randint(50) + 50);
	    (void)set_oppose_acid(p_ptr->oppose_acid + randint(50) + 50);
	    (void)set_oppose_elec(p_ptr->oppose_elec + randint(50) + 50);
	    (void)set_oppose_fire(p_ptr->oppose_fire + randint(50) + 50);
	    (void)set_oppose_cold(p_ptr->oppose_cold + randint(50) + 50);
	    (void)set_oppose_pois(p_ptr->oppose_pois + randint(50) + 50);
	  }
	o_ptr->timeout = 400;
	break;
      }
    case ACT_SOULKEEPER:
      {
	msg_print("Your armor glows a bright white...");
	msg_print("You feel much better...");
	(void)hp_player(1000);
	(void)set_cut(0);
	o_ptr->timeout = 888;
	break;
      }
    case ACT_BELEGENNON:
      {
	msg_print("Your armor twists space around you...");
	teleport_player(10, TRUE);
	o_ptr->timeout = 2;
	break;
      }
    case ACT_CELEBORN:
      {
	msg_print("Your armor glows deep blue...");
	(void)genocide();
	o_ptr->timeout = 1500;
	break;
      }
    case ACT_CASPANION:
      {
	msg_print("Your armor glows bright red...");
	destroy_doors_touch();
	o_ptr->timeout = 0;
	break;
      }
    case ACT_HIMRING:
      {
	msg_print("A shrill wailing sound surrounds you.");
	(void)set_protevil(p_ptr->protevil + randint(25) + plev);
	o_ptr->timeout = rand_int(200) + 200;
	break;
      }
    case ACT_ELEMENTS:
      {
	msg_print("Your shield glows many colours...");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint(20) + 20);
	o_ptr->timeout = 160;
	break;
      }
      
    case ACT_EARENDIL:
      {
	(void)set_poisoned(0);
	(void)set_confused(0);
	(void)set_blind(0);
	(void)set_stun(0);
	(void)set_cut(0);
	
	o_ptr->timeout = 100;
	break;
      }
      
    case ACT_GIL_GALAD:
      {
	msg_print("Your shield gleams with blinding light...");
	fire_sphere(GF_LITE, 0, 75, 6, 20);
	confu_monsters(3 * plev / 2);
	o_ptr->timeout = 250;
	break;
      }
      
    case ACT_HOLHENNETH:
      {
	msg_print("Your helm glows bright white...");
	msg_print("An image forms in your mind...");
	detect_all(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = rand_int(55) + 55;
	break;
      }
    case ACT_VINYAMAR:
      {
	msg_print("A thrilling battle song awakes the warrior within you!");
	(void)hp_player(10);
	(void)set_afraid(0);
	(void)set_hero(p_ptr->hero + randint(25) + 25);
	o_ptr->timeout = 200;
	break;
      }
    case ACT_GONDOR:
      {
	msg_print("Your crown glows deep blue...");
	msg_print("You feel a warm tingling inside...");
	(void)hp_player(500);
	(void)set_cut(0);
	o_ptr->timeout = 500;
	break;
      }
      
    case ACT_VALINOR:
      {
	msg_print("Your cloak glows many colours...");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint(20) + 20);
	(void)set_oppose_pois(p_ptr->oppose_pois + randint(20) + 20);
	o_ptr->timeout = 200;
	break;
      }
    case ACT_HOLCOLLETH:
      {
	msg_print("Your cloak glows deep blue...");
	sleep_monsters_touch(3 * plev / 2 + 10);
	o_ptr->timeout = 55;
	break;
      }
    case ACT_THINGOL:
      {
	msg_print("Your cloak glows bright yellow...");
	recharge(180);
	o_ptr->timeout = 90;
	break;
      }
    case ACT_COLANNON:
      {
	msg_print("Your cloak twists space around you...");
	teleport_player(100, TRUE);
	o_ptr->timeout = 45;
	break;
      }
    case ACT_LUTHIEN:
      {
	msg_print("Your cloak glows a deep red...");
	restore_level();
	o_ptr->timeout = 450;
	break;
      }
      
    case ACT_CAMMITHRIM:
      {
	msg_print("Your gloves glow extremely brightly...");
	if (!get_aim_dir(&dir)) return;
	lite_line(dir);
	o_ptr->timeout = 5;
	break;
      }
    case ACT_EOL:
      {
	msg_print("Your gauntlets radiate dark energy...");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_SPIRIT, dir, damroll(9, 8));
	take_hit(damroll(1, 6), "the dark arts");
	o_ptr->timeout = rand_int(7) + 7;
	break;
      }
    case ACT_PAURNIMMEN:
      {
	msg_print("Your gauntlets are covered in frost...");
	set_ele_attack(ATTACK_COLD, 50);
	o_ptr->timeout = rand_int(50) + 100;
	break;
      }
    case ACT_PAURNEN:
      {
	msg_print("Your gauntlets are covered in acid...");
	set_ele_attack(ATTACK_ACID, 30);
	o_ptr->timeout = rand_int(50) + 100;
	break;
      }
    case ACT_FINGOLFIN:
      {
	msg_print("Your cesti gleam brown and grey...");
	if (!get_aim_dir(&dir)) return;
	wall_to_mud(dir);
	o_ptr->timeout = rand_int(3) + 3;
	break;
      }
      
      
    case ACT_FEANOR:
      {
	msg_print("Your boots glow bright green...");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint(20) + 20);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = 200;
	break;
      }
    case ACT_DAL:
      {
	msg_print("Your boots glow deep blue...");
	(void)set_afraid(0);
	(void)set_poisoned(0);
	o_ptr->timeout = 5;
	break;
      }
      
    case ACT_GIMLI:
      {
	msg_print("You understand the structure of the area around you.");
	map_area(0, 0, FALSE);
	o_ptr->timeout = 35 + randint(35);
	break;
      }
      
    case ACT_NARTHANC:
      {
	msg_print("Your dagger is covered in fire...");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_FIRE, dir, damroll(6, 8));
	o_ptr->timeout = rand_int(7) + 7;
	break;
      }
    case ACT_NIMTHANC:
      {
	msg_print("Your dagger is covered in frost...");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_COLD, dir, damroll(5, 8));
	o_ptr->timeout = rand_int(6) + 6;
	break;
      }
    case ACT_DETHANC:
      {
	msg_print("Your dagger is covered in sparks...");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_ELEC, dir, damroll(4, 8));
	o_ptr->timeout = rand_int(5) + 5;
	break;
      }
    case ACT_RILIA:
      {
	msg_print("Your dagger throbs deep green...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_POIS, dir, 12, 3, FALSE);
	o_ptr->timeout = rand_int(4) + 4;
	break;
      }
    case ACT_BELANGIL:
      {
	msg_print("Your dagger is covered in frost...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_COLD, dir, 3 * p_ptr->lev / 2, 2, FALSE);
	o_ptr->timeout = rand_int(5) + 5;
	break;
      }
    case ACT_ARUNRUTH:
      {
	msg_print("Your sword glows a pale blue...");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_COLD, dir, damroll(12, 8));
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RINGIL:
      {
	msg_print("Your sword glows an intense blue-white...");
	if (!get_aim_dir(&dir)) return;
	fire_arc(GF_ICE, dir, 250, 10, 40);
	o_ptr->timeout = 200;
	break;
      }
    case ACT_ANDURIL:
      {
	msg_print("Your sword glows an intense red...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_FIRE, dir, 150, 2, FALSE);
	o_ptr->timeout = 200;
	break;
      }

    case ACT_MANWE:
      {
	msg_print("Your sword glows pure white...");
	if (!get_aim_dir(&dir)) return;
	fire_arc(GF_FORCE, dir, 300, 10, 180);
	o_ptr->timeout = 200;
	break;
      }
      
    case ACT_THEODEN:
      {
	msg_print("Your axe blade glows black...");
	if (!get_aim_dir(&dir)) return;
	drain_life(dir, 120);
	o_ptr->timeout = 400;
	break;
      }
    case ACT_AEGLOS:
      {
	msg_print("Your spear glows a bright white...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_COLD, dir, 100, 2, FALSE);
	o_ptr->timeout = 500;
	break;
      }
    case ACT_OROME:
      {
	msg_print("Your spear pulsates...");
	if (!get_aim_dir(&dir)) return;
	wall_to_mud(dir);
	o_ptr->timeout = 5;
	break;
      }
    case ACT_EONWE:
      {
	msg_print("Your axe lets out a long, shrill note...");
	(void)mass_genocide();
	o_ptr->timeout = 1500;
	break;
      }
    case ACT_LOTHARANG:
      {
	msg_print("Your battle axe radiates deep purple...");
	hp_player(damroll(4, 12));
	(void)set_cut((p_ptr->cut / 2) - 50);
	o_ptr->timeout = rand_int(3) + 3;
	break;
      }
    case ACT_ULMO:
      {
	msg_print("Your trident glows deep red...");
	if (!get_aim_dir(&dir)) return;
	teleport_monster(dir, 45 + (plev/3));
	o_ptr->timeout = 75;
	break;
      }
    case ACT_AVAVIR:
      {
	msg_print("Your scythe glows soft white...");
	if (!word_recall(rand_int(20) + 15))
	  break;
	o_ptr->timeout = 200;
	break;
      }
    case ACT_TOTILA:
      {
	msg_print("Your flail glows in scintillating colours...");
	if (!get_aim_dir(&dir)) return;
	confuse_monster(dir, 3 * plev / 2 + 5);
	o_ptr->timeout = 15;
	break;
      }
      
    case ACT_FIRESTAR:
      {
	msg_print("Your morning star rages in fire...");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_FIRE, dir, 125, 3, FALSE);
	o_ptr->timeout = 150;
	break;
      }
    case ACT_TARATOL:
      {
	msg_print("Your mace glows bright green...");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint(20) + 20);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = rand_int(100) + 100;
	break;
      }
    case ACT_ERIRIL:
      {
	msg_print("Your quarterstaff glows yellow...");
	if (!ident_spell()) return;
	o_ptr->timeout = 10;
	break;
      }
    case ACT_OLORIN:
      {
	msg_print("Your quarterstaff glows brightly...");
	probing();
	o_ptr->timeout = 20;
	break;
      }
    case ACT_TURMIL:
      {
	msg_print("Your hammer glows white...");
	if (!get_aim_dir(&dir)) return;
	drain_life(dir, 90);
	o_ptr->timeout = 70;
	break;
      }
    case ACT_HARAD:
      {
	msg_print("The bolt you have ready to hand gleams with deadly power.");
	p_ptr->special_attack |= (ATTACK_SUPERSHOT);
	o_ptr->timeout = 200 + randint(200);
	
	/* Redraw the state */
	p_ptr->redraw |= (PR_STATUS);
	
	break;
      }
    case ACT_CUBRAGOL:
      {
	msg_print("Your crossbow glows deep red...");
	(void)brand_missile(TV_BOLT, EGO_FLAME);
	o_ptr->timeout = 999;
	break;
      }
    case ACT_BUCKLAND:
      {
	msg_print("Your sling glows with power...");
	(void)brand_missile(TV_SHOT, 0);
	o_ptr->timeout = 1500;
	break;
      }
      
      
      /* Activations for dragon scale mails. */
    case ACT_DRAGON_BLACK:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become an acidic dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_ACID);
	    msg_print("You breathe acid.");
	    fire_arc(GF_ACID, dir, (plev/10 + 1) * 45, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_BLUE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a storm dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_ELEC);
	    msg_print("You breathe lightning.");
	    fire_arc(GF_ELEC, dir, (plev/10 + 1) * 40, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_WHITE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become an icy dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    msg_print("You breathe frost.");
	    fire_arc(GF_COLD, dir, (plev/10 + 1) * 45, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_RED:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a fire dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_FIRE);
	    msg_print("You breathe fire.");
	    fire_arc(GF_FIRE, dir, (plev/10 + 1) * 50, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_GREEN:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a poisonous dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_GAS);
	    msg_print("You breathe poison gas.");
	    fire_arc(GF_POIS, dir, (plev/10 + 1) * 45, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_MULTIHUED:
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
	    if (!get_aim_dir(&dir)) return;
	    chance = rand_int(5);
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
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_SHINING:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a glowing dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    chance = rand_int(2);
	    sound(((chance == 0 ? MSG_BR_LIGHT : MSG_BR_DARK)));
	    msg_format("You breathe %s.",
		       ((chance == 0 ? "light" : "darkness")));
	    fire_arc((chance == 0 ? GF_LITE : GF_DARK), dir, 
		     (plev/10 + 1) * 50, 10, 40);
	  }			
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_LAW:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon of Order.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    chance = rand_int(2);
	    sound(((chance == 1 ? MSG_BR_SOUND : MSG_BR_SHARDS)));
	    msg_format("You breathe %s.",
		       ((chance == 1 ? "sound" : "shards")));
	    fire_arc((chance == 1 ? GF_SOUND : GF_SHARD),
		     dir, (plev/10 + 1) * 60, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_BRONZE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a mystifying dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_CONF);
	    msg_print("You breathe confusion.");
	    fire_arc(GF_CONFUSION, dir, (plev/10 + 1) * 40, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_GOLD:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon with a deafening roar.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_SOUND);
	    msg_print("You breathe sound.");
	    fire_arc(GF_SOUND, dir, (plev/10 + 1) * 40, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_CHAOS:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon of Chaos.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    chance = rand_int(2);
	    sound(((chance == 1 ? MSG_BR_CHAOS : MSG_BR_DISENCHANT)));
	    msg_format("You breathe %s.",
		       ((chance == 1 ? "chaos" : "disenchantment")));
	    fire_arc((chance == 1 ? GF_CHAOS : GF_DISENCHANT),
		     dir, (plev/10 + 1) * 55, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_BALANCE:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a dragon of Balance.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    chance = rand_int(4);
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
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
    case ACT_DRAGON_POWER:
      {
	if ((p_ptr->schange) != SHAPE_WYRM)
	  {
	    msg_print("You become a wonderous dragon.");
	    shapechange(SHAPE_WYRM);
	  }
	else
	  {
	    if (!get_aim_dir(&dir)) return;
	    sound(MSG_BR_ELEMENTS);
	    msg_print("You breathe the elements.");
	    fire_arc(GF_ALL, dir, (plev/10 + 1) * 75, 10, 40);
	  }
	o_ptr->timeout = rand_int(50) + 50;
	break;
      }
      
      /* Activations for rings. */
    case ACT_RING_ACID:
      {
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_ACID, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_acid(p_ptr->oppose_acid + randint(20) + 20);
	o_ptr->timeout = rand_int(100) + 50;
	break;
      }
    case ACT_RING_ELEC:
      {
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_ELEC, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint(20) + 20);
	o_ptr->timeout = rand_int(100) + 50;
	break;
      }
    case ACT_RING_FIRE:
      {
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_FIRE, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint(20) + 20);
	o_ptr->timeout = rand_int(100) + 50;
	break;
      }
    case ACT_RING_COLD:
      {
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_COLD, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint(20) + 20);
	o_ptr->timeout = rand_int(100) + 50;
	break;
      }
    case ACT_RING_POIS:
      {
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_POIS, dir, 45 + 3 * plev / 2, 3, FALSE);
	(void)set_oppose_pois(p_ptr->oppose_pois + randint(20) + 20);
	o_ptr->timeout = rand_int(100) + 50;
	break;
      }
      
      
      /* Activations for amulets. */
    case ACT_AMULET_ESCAPING:
      {
	teleport_player(40,TRUE);
	o_ptr->timeout = rand_int(40) + 40;
	break;
      }
      
    case ACT_AMULET_LION:
      {
	/* Already a Lion */
	if (p_ptr->schange == SHAPE_LION) break;
	
	msg_print("You become a fierce Lion.");
	shapechange(SHAPE_LION);
	o_ptr->timeout = 200;
	break;
      }
      
    case ACT_AMULET_METAMORPH:
      {
	/* Already changed */
	if (p_ptr->schange) break;
	
	shapechange(randint(MAX_SHAPE));
	o_ptr->timeout = 300;
	break;
      }
      
      
      /* Activations for random artifacts, and available for use elsewhere. */
    case ACT_RANDOM_FIRE1:
      {
	msg_print("You launch a bolt of fire.");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_FIRE, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = rand_int(7) + 7;
	break;
      }
    case ACT_RANDOM_FIRE2:
      {
	msg_print("You feel a sphere of fire form between your hands.");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_FIRE, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_FIRE3:
      {
	msg_print("The fires of Anor rise in wrath!");
	fire_sphere(GF_FIRE, 0, 150, 5, 20);
	o_ptr->timeout = 600;
	break;
      }
    case ACT_RANDOM_COLD1:
      {
	msg_print("You launch a bolt of frost.");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_COLD, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = rand_int(7) + 7;
	break;
      }
    case ACT_RANDOM_COLD2:
      {
	msg_print("You hurl a sphere of killing frost.");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_COLD, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_COLD3:
      {
	msg_print("A wild Northland frost storms uncontrollably!");
	fire_sphere(GF_COLD, 0, 150, 5, 20);
	o_ptr->timeout = 600;
	break;
      }
    case ACT_RANDOM_ACID1:
      {
	msg_print("You launch a bolt of acid.");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_ACID, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = rand_int(7) + 7;
	break;
      }
    case ACT_RANDOM_ACID2:
      {
	msg_print("A sphere of deadly acid forms upon your hand.");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_ACID, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_ACID3:
      {
	msg_print("A tornado of acid melts armour and flesh!");
	fire_sphere(GF_ACID, 0, 160, 3, 20);
	o_ptr->timeout = 600;
	break;
      }
    case ACT_RANDOM_ELEC1:
      {
	msg_print("You launch a bolt of electricity.");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_ELEC, dir, damroll(3 + plev / 8, 8));
	o_ptr->timeout = rand_int(7) + 7;
	break;
      }
    case ACT_RANDOM_ELEC2:
      {
	msg_print("You summon ball lightning to your aid.");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_ELEC, dir, 90, 1, 20);
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_ELEC3:
      {
	msg_print("A massive stroke of lightning smites the ground!");
	fire_sphere(GF_ELEC, 0, 130, 2, 20);
	msg_print("Boom!");
	fire_sphere(GF_SOUND, 0, 25, 9, 20);
	o_ptr->timeout = 600;
	break;
      }
    case ACT_RANDOM_POIS1:
      {
	msg_print("You launch a poison dart.");
	if (!get_aim_dir(&dir)) return;
	fire_bolt(GF_POIS, dir, damroll(3 + plev / 10, 8));
	o_ptr->timeout = rand_int(22) + 22;
	break;
      }
    case ACT_RANDOM_POIS2:
      {
	msg_print("Deadly gases blanket the area.");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_POIS, 0, 110, 9, 30);
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RANDOM_LIGHT1:
      {
	msg_print("You throw a radiant sphere...");
	if (!get_aim_dir(&dir)) return;
	TARGET_PRESERVE 
	  fire_ball(GF_LITE, dir, 50, 0, FALSE);
	TARGET_RESTORE
	  fire_ball(GF_CONFUSION, dir, 10, 0, FALSE);
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_LIGHT2:
      {
	msg_print("You bathe the area in radiant light!");
	dispel_light_hating(175);
	o_ptr->timeout = 600;
	break;
      }
    case ACT_RANDOM_DISPEL_UNDEAD:
      {
	msg_print("A tide of life surrounds you!");
	(void)dispel_undead(100);
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RANDOM_DISPEL_EVIL:
      {
	msg_print("A wave of goodness washes over you...");
	(void)dispel_evil(100);
	
	if (check_ability(SP_EVIL))
	  {
	    msg_print("Your black soul is hit!");
	    take_hit(25, "struck down by Good");
	  }
	o_ptr->timeout = 400;
	break;
      }
    case ACT_RANDOM_SMITE_UNDEAD:
      {
	msg_print("Spells to dispel an undead antagonist surround you...");
	if (!get_aim_dir(&dir)) return;
	dispel_an_undead(dir, damroll(plev / 4, 33));
	o_ptr->timeout = 200;
	break;
      }
    case ACT_RANDOM_SMITE_DEMON:
      {
	msg_print("Spells to dispel a demonic adversary surround you...");
	if (!get_aim_dir(&dir)) return;
	dispel_a_demon(dir, damroll(plev / 4, 33));
	o_ptr->timeout = 200;
	break;
      }
    case ACT_RANDOM_SMITE_DRAGON:
      {
	msg_print("Spells to dispel a dragonic foe surround you...");
	if (!get_aim_dir(&dir)) return;
	dispel_a_dragon(dir, damroll(plev / 4, 33));
	o_ptr->timeout = 200;
	break;
      }
    case ACT_RANDOM_HOLY_ORB:
      {
	msg_print("A cleansing ball materializes on your fingertips.");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_HOLY_ORB, dir, 60, 1, 20);
	o_ptr->timeout = 175;
	break;
      }
    case ACT_RANDOM_BLESS:
      {
	msg_print("You feel blessed for battle.");
	if (!p_ptr->blessed)
	  {
	    (void)set_blessed(p_ptr->blessed + randint(24) + 24);
	  }
	else
	  {
	    (void)set_blessed(p_ptr->blessed + randint(12) + 12);
	  }
	o_ptr->timeout = 200;
	break;
      }
    case ACT_RANDOM_FRIGHTEN_ALL:
      {
	msg_print("You reveal yourself in wrath; your enemies tremble!");
	(void)fear_monsters((3 * plev / 2) + 5);
	o_ptr->timeout = rand_int(120) + 120;
	break;
      }
    case ACT_RANDOM_HEAL1:
      {
	(void)hp_player(damroll(5, 20));
	(void)set_cut(p_ptr->cut - 5);
	(void)set_poisoned(p_ptr->poisoned - 5);
	o_ptr->timeout = 85;
	break;
      }
    case ACT_RANDOM_HEAL2:
      {
	(void)hp_player(damroll(7, 40));
	(void)set_cut((p_ptr->cut / 2) - 5);
	(void)set_poisoned((p_ptr->poisoned / 2) - 5);
	o_ptr->timeout = 225;
	break;
      }
    case ACT_RANDOM_HEAL3:
      {
	(void)hp_player(damroll(10, 60));
	(void)set_cut(0);
	(void)set_poisoned(0);
	o_ptr->timeout = 500;
	break;
      }
    case ACT_RANDOM_CURE:
      {
	msg_print("Tender hands massage your hurts away.");
	(void)set_stun(0);
	(void)set_cut(0);
	(void)set_poisoned(0);
	(void)set_confused(0);
	(void)set_blind(0);
	(void)do_res_stat(A_CON);
	o_ptr->timeout = 500;
	break;
      }
    case ACT_RANDOM_PROT_FROM_EVIL:
      {
	msg_print("A shrill wail surrounds you.");
	
	if (!p_ptr->protevil)
	  {
	    (void)set_protevil(p_ptr->protevil + randint(24) + 24);
	  }
	else
	  {
	    (void)set_protevil(p_ptr->protevil + randint(30));
	  }
	msg_print("You feel somewhat safer.");
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_CHAOS:
      {
	msg_print("You unleash the powers of Unmaking!");
	if (!get_aim_dir(&dir)) return;
	fire_ball(GF_CHAOS, dir, randint(320), 2, FALSE);
	o_ptr->timeout = 600;
	break;
      }
    case ACT_RANDOM_SHARD_SOUND:
      {
	msg_print("You invoke the powers of Law...");
	if (!get_aim_dir(&dir)) return;
	if (randint(2) == 1)
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
	break;
      }
    case ACT_RANDOM_NETHR:
      {
	msg_print("You cast a gleaming orb of midnight hue!");
	if (!get_aim_dir(&dir)) return;
	fire_sphere(GF_NETHER, dir, 100, 1, 20);
	o_ptr->timeout = 400;
	break;
      }
    case ACT_RANDOM_LINE_LIGHT:
      {
	if (!get_aim_dir(&dir)) return;
	msg_print("A line of shimmering yellow light appears.");
	lite_line(dir);
	o_ptr->timeout = 6 + randint(6);
	break;
      }
    case ACT_RANDOM_STARLIGHT:
      {
	msg_print("Light radiates outward in all directions.");
	for (k = 0; k < 8; k++) lite_line(ddd[k]);
	o_ptr->timeout = 8 + randint(8);
	break;
      }
    case ACT_RANDOM_EARTHQUAKE:
      {
	msg_print("You strike the floor, and the earth crumbles!");
	earthquake(p_ptr->py, p_ptr->px, 10, FALSE);
	o_ptr->timeout = 40 + randint(40);
	break;
      }
    case ACT_RANDOM_IDENTIFY:
      {
	msg_print("You unveil hidden secrets.");
	if (ident_spell()) o_ptr->timeout = 30;
	break;
      }
    case ACT_RANDOM_SPEED:
      {
	msg_print("All around you move with dreamlike slowness.");
	if (!p_ptr->fast)
	  {
	    (void)set_fast(randint(20) + 20);
	  }
	else
	  {
	    (void)set_fast(p_ptr->fast + 5);
	  }
	o_ptr->timeout = rand_int(120) + 120;
	break;
      }
    case ACT_RANDOM_TELEPORT_AWAY:
      {
	msg_print("You weave a pattern of rejection and denial.");
	if (!get_aim_dir(&dir)) return;
	(void)teleport_monster(dir, 55 + (plev/2));
	o_ptr->timeout = 110;
	break;
      }
    case ACT_RANDOM_HEROISM:
      {
	msg_print("A thrilling battle song awakes the warrior within you!");
	(void)hp_player(10);
	(void)set_afraid(0);
	(void)set_hero(p_ptr->hero + randint(25) + 25);
	o_ptr->timeout = 200;
	break;
      }
    case ACT_RANDOM_STORM_DANCE:
      {
	msg_print("Wild music plays, and you dance up a storm...");
	fire_sphere(GF_SOUND, 0, 24, 8, 20);
	fire_sphere(GF_SHARD, 0, 32, 8, 20);
	fire_sphere(GF_CONFUSION, 0, 8, 8, 20);
	
	if (randint(2) == 1) 
	  {
	    msg_print("Your wild movements exhaust you!");
	    take_hit(damroll(1, 12), "danced to death");
	  }
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RANDOM_RESIST_ELEMENTS:
      {
	msg_print("Quadricolored magics swirl around you protectingly.");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint(20) + 20);
	o_ptr->timeout = 400;
	break;
      }
    case ACT_RANDOM_RESIST_ALL:
      {
	msg_print("Penticolored magics swirl around you protectingly.");
	(void)set_oppose_acid(p_ptr->oppose_acid + randint(20) + 20);
	(void)set_oppose_elec(p_ptr->oppose_elec + randint(20) + 20);
	(void)set_oppose_fire(p_ptr->oppose_fire + randint(20) + 20);
	(void)set_oppose_cold(p_ptr->oppose_cold + randint(20) + 20);
	(void)set_oppose_pois(p_ptr->oppose_pois + randint(20) + 20);
	o_ptr->timeout = 400;
	break;
      }
    case ACT_RANDOM_TELEPORT1:
      {
	msg_print("You pass through a transparent gateway...");
	teleport_player(30,TRUE);
	o_ptr->timeout = 10 + randint(10);
	break;
      }
    case ACT_RANDOM_TELEPORT2:
      {
	msg_print("Time and space twist about you...");
	teleport_player(200,TRUE);
	o_ptr->timeout = 80;
	break;
      }
    case ACT_RANDOM_RECALL:
      {
	if(!word_recall(rand_int(20) + 15))
	  break;
	o_ptr->timeout = 350;
	break;
      }
    case ACT_RANDOM_REGAIN:
      {
	msg_print("Surrounded by darkness, you envoke light and beauty.");
	msg_print("Your spirit regains its natural vitality.");
	
	(void)restore_level();
	o_ptr->timeout = 800;
	break;
      }
    case ACT_RANDOM_RESTORE:
      {
	msg_print("A multicolored mist surounds you, restoring body and mind.");
	(void)do_res_stat(A_STR);
	(void)do_res_stat(A_INT);
	(void)do_res_stat(A_WIS);
	(void)do_res_stat(A_DEX);
	(void)do_res_stat(A_CON);
	(void)do_res_stat(A_CHR);
	o_ptr->timeout = 800;
	break;
      }
    case ACT_RANDOM_SHIELD:
      {
	msg_print("Magics coalesce to form a shimmering barrier.");
	if (!p_ptr->shield)
	  {
	    (void)set_shield(p_ptr->shield + randint(25) + 25);
	  }
	else
	  {
	    (void)set_shield(p_ptr->shield + randint(15) + 15);
	  }
	o_ptr->timeout = 400;
	break;
      }
      
    case ACT_RANDOM_BRAND_MISSILE:
      {
	msg_print("Your missile launcher glows with power...");
	(void)brand_missile(0, 0);
	o_ptr->timeout = 1750;
	break;
      }
    case ACT_RANDOM_SUPER_SHOOTING:
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
	o_ptr->timeout = 200 + randint(200);
	
	/* Redraw the state */
	p_ptr->redraw |= (PR_STATUS);
	
	break;
      }
    case ACT_RANDOM_DETECT_MONSTERS:
      {
	msg_print("You search for monsters.");
	(void)detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = 4 + randint(4);
	break;
      }
    case ACT_RANDOM_DETECT_EVIL:
      {
	msg_print("You hunt for evil creatures...");
	(void)detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = 4 + randint(4);
	break;
      }
    case ACT_RANDOM_DETECT_ALL:
      {
	msg_print("You sense the area around you.");
	detect_all(DETECT_RAD_DEFAULT, TRUE);
	o_ptr->timeout = 30 + randint(30);
	break;
      }
    case ACT_RANDOM_MAGIC_MAP:
      {
	msg_print("A mental image of your surroundings is fixed in your mind.");
	map_area(0, 0, FALSE);
	o_ptr->timeout = 30 + randint(30);
	break;
      }
    case ACT_RANDOM_DETECT_D_S_T:
      {
	/* Hack - 'show' effected region only with
	 * the first detect */
	msg_print("The secrets of traps and doors are revealed.");
	(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	(void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	(void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	o_ptr->timeout = 10 + randint(10);
	break;
      }
    case ACT_RANDOM_CONFU_FOE:
      {
	msg_print("You chant runes of confusing...");
	if (!get_aim_dir(&dir)) return;
	if (confuse_monster(dir, 5 * plev / 3))	
	  msg_print("...which utterly baffle your foe!");
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_SLEEP_FOE:
      {
	msg_print("A fine dust appears in your hand, and you throw it...");
	if (!get_aim_dir(&dir)) return;
	if (sleep_monster(dir, 5 * plev / 3)) 
	  msg_print("...sending a foe to the realm of dreams!");
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_TURN_FOE:
      {
	msg_print("You lock eyes with an enemy...");
	if (!get_aim_dir(&dir)) return;
	if (fear_monster(dir, 5 * plev / 3)) 
	  msg_print("...and break his courage!");
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_SLOW_FOE:
      {
	msg_print("You focus on the mind of an opponent...");
	if (!get_aim_dir(&dir)) return;
	if (slow_monster(dir, 5 * plev / 3)) 
	  msg_print("...and sap his strength!");
	o_ptr->timeout = 250;
	break;
      }
    case ACT_RANDOM_BANISH_EVIL:
      {
	msg_print("A mighty hand drives your foes from you!");
	(void)banish_evil(80);
	o_ptr->timeout = 400;
	break;
      }
    case ACT_RANDOM_DISARM:
      {
	msg_print("You feel skilled hands guiding your disarming.");
	if (!get_aim_dir(&dir)) return;
	(void)disarm_trap(dir);
	o_ptr->timeout = 7 + randint(7);
	break;
      }
    case ACT_RANDOM_CONFU_FOES:
      {
	msg_print("You intone a bewildering hex...");
	if (confu_monsters(3 * plev / 2))	
	  msg_print("...which utterly baffles your foes!");
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RANDOM_SLEEP_FOES:
      {
	msg_print("Soft, soothing music washes over you..");
	if (sleep_monsters(3 * plev / 2)) 
	  msg_print("...and sends your enemies to sleep!");
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RANDOM_TURN_FOES:
      {
	msg_print("You reveal yourself in wrath; your enemies tremble!");
	(void)fear_monsters(3 * plev / 2);
	o_ptr->timeout = 300;
	break;
      }
    case ACT_RANDOM_SLOW_FOES:
      {
	msg_print("An opaque cloud blankets the area...");
	if (slow_monsters(3 * plev / 2)) 
	  msg_print("...and dissipates, along with your opponents' strength!");
	else msg_print("...and dissipates without effect.");
	o_ptr->timeout = 300;
	break;
      }

      /* Activations for rings of power */
    case ACT_POWER_ACID_BLAST:
      {
	/* Acid, confusion, fear */
	msg_print("A blast of corrosion strikes your foes!");
	fire_sphere(GF_ACID, 0, randint(100) + 100, 5, 20);
	(void)confu_monsters(100);
	(void)fear_monsters(100);

	o_ptr->timeout = 1500;
	break;
      }
    case ACT_POWER_CHAIN_LIGHTNING:
      {
	int strikes = 2 + randint(10);
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
	    if (!avail_mon_num) break;

	    /* Pick a target... */
	    target = rand_int(avail_mon_num);
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
	    if (!cave_m_idx[targ_y][targ_x]) break;

	    /* ...and hit it */
	    project(cur_mon, 1, targ_y, targ_x, damroll(30, 4), GF_ELEC, flag, 
		    0, 0);

	    /* Set current monster (may be dead) */
	    cur_y = targ_y;
	    cur_x = targ_x;
	    cur_mon = avail_mon[target];
	  }
      
	o_ptr->timeout = 2000;
	break;
      }
    case ACT_POWER_LAVA_POOL:
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
	break;
      }
    case ACT_POWER_ICE_WHIRLPOOL:
      {
	/* Unleash a whirlpool. */
	msg_print("A howling vortex of ice rises in wrath around you.");
	fire_sphere(GF_ICE, 0, randint(100) + 100, 5, 10);
	
	/* Whisk around and slow the nearby monsters. */
	fire_ball(GF_AWAY_ALL, 0, 12, 6, FALSE);
	slow_monsters(50);
	
	o_ptr->timeout = 1000;
	break;
      }
    case ACT_POWER_GROW_FOREST:
      {
	msg_print("A leafy forest surrounds you!");
	grow_trees_and_grass(TRUE);
	o_ptr->timeout = 2000;
	break;
      }
    case ACT_POWER_RESTORE_AND_ENHANCE:
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
	(void)set_hero(p_ptr->hero + randint(25) + 25);
	(void)set_shero(p_ptr->shero + randint(25) + 25);
	(void)set_blessed(p_ptr->blessed + randint(25) + 25);
	(void)set_protevil(p_ptr->protevil + randint(25) + 25);
	o_ptr->timeout = 3000;
	break;
      }
    case ACT_POWER_ZONE_OF_CHAOS:
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
	      if (randint(5) == 1) continue;

	      /* Hit it */
	      (void)fire_meteor(-1, GF_CHAOS, y, x, 50, 0, FALSE);
	    }

	o_ptr->timeout = 3000;
	break;
      }
    case ACT_POWER_PRESSURE_WAVE:
      {
	msg_print("Your foes are thrown backward!");
	fire_ball(GF_FORCE, 0, 50, 20, FALSE);

	o_ptr->timeout = 1500;
	break;
      }
    case ACT_POWER_ENERGY_DRAIN:
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
	break;
      }
    case ACT_POWER_MASS_STASIS:
      {
	msg_print("You command your foes to be still!");
	hold_all();
	o_ptr->timeout = 1000;
	break;
      }
    case ACT_POWER_LIGHT_FROM_ABOVE:
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
	break;
      }
    case ACT_POWER_MASS_POLYMORPH:
      {
	msg_print("Reality warps...");
	poly_all(p_ptr->depth);

	o_ptr->timeout = 2500;
	break;
      }
    case ACT_POWER_GRAVITY_WAVE:
      {
	msg_print("Gravity crushes, then releases, your foes!");
	fire_ball(GF_GRAVITY, 0, 100, 20, FALSE);

	o_ptr->timeout = 2000;
	break;
      }
    case ACT_POWER_ENLIST_EARTH:
      {
	int m_idx, targ_y, targ_x, group;

	msg_print("You call on the Earth to bring forth allies!");

	/* Must target a monster */
	if (!get_aim_dir(&dir)) return;
	if (p_ptr->target_who <= 0) 
	  {
	    msg_print("You must target a monster.");
	    return;
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
		(r_ptr->d_char = 'g') && (!(r_ptr->flags3 & RF3_DRAGON)))
	      m_ptr->hostile = p_ptr->target_who;
	  }

	o_ptr->timeout = 3000;
	break;
      }
    case ACT_POWER_TELEPORT_ALL:
      {
	teleport_all(80);
	o_ptr->timeout = 2000;
	break;
      }
      
      
      
      /* Activations for ego-items. */
    case ACT_BALROG_WHIP:
      {
	/* A crude way to simulate a long-range melee blow... */
	if (!get_aim_dir(&dir)) return;
	
	msg_print("You lash out at a nearby foe.");
	fire_arc(GF_FIRE, dir, damroll(o_ptr->dd, o_ptr->ds), 2, 0);
	o_ptr->timeout = 0;
	break;
      }
      
      /* Activation for magestaffs */
    case ACT_MAGESTAFF:
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
	    o_ptr->timeout = rand_int(10) + 15;
	  }
	break;
      }
      


    default:
      
      /* Mistake */
      msg_print("Oops.  Activation type unrecognized.");
      break;
    }
  
  /* Take a turn */
  p_ptr->energy_use = 100;

  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
  
  
  /*
   * Give the player a chance to learn more about the effects of
   * non-artifact dragon scale mails, amulets, and rings.
   */
  if (((o_ptr->tval == TV_DRAG_ARMOR) || (o_ptr->tval == TV_AMULET) ||
       (o_ptr->tval == TV_RING)) && (!artifact_p(o_ptr)) &&
      (!k_info[o_ptr->k_idx].known_effect) && (rand_int(25) == 0))
    {
      int chooser = 0;
      
      if (o_ptr->tval == TV_DRAG_ARMOR) chooser = 0;
      if (o_ptr->tval == TV_AMULET) chooser = 1;
      if (o_ptr->tval == TV_RING) chooser = 2;
      
      /* We now (theoretically) know more about the object's powers */
      k_info[o_ptr->k_idx].known_effect = TRUE;
      
      /* If we actually give more information now, let the player know. */
      if (strlen(obj_special_info[chooser][o_ptr->sval]))
	{
	  char o_name[120];
	  object_desc(o_name, o_ptr, TRUE, 0);
	  
	  msg_format("You feel you know more about the effects of %s.", 
		     o_name);
	}
    }

    /* Free the hook */
    item_tester_hook = NULL;
  
  /* Success */
  return;
}

