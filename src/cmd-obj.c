/*
 * File: cmd-obj.c
 * Purpose: Handle objects in various ways
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2007-9 Andrew Sidwell, Chris Carr, Ed Graham, Erik Osheim
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
#include "cmds.h"
#include "effects.h"
#include "game-cmd.h"
#include "object.h"
#include "tvalsval.h"
#include "spells.h"
#include "squelch.h"

/*** Utility bits and bobs ***/

/*
 * Check to see if the player can use a rod/wand/staff/activatable object.
 */
static int check_devices(object_type *o_ptr)
{
	int fail;
	const char *msg;
	const char *what = NULL;

	/* Get the right string */
	switch (o_ptr->tval)
	{
		case TV_ROD:   msg = "zap the rod";   break;
		case TV_WAND:  msg = "use the wand";  what = "wand";  break;
		case TV_STAFF: msg = "use the staff"; what = "staff"; break;
		default:       msg = "activate it";  break;
	}

	/* Figure out how hard the item is to use */
	fail = get_use_device_chance(o_ptr);

	/* Roll for usage */
	if (randint1(1000) < fail)
	{
		if (OPT(flush_failure)) flush();
		msg_format("You failed to %s properly.", msg);
		return FALSE;
	}

	/* Notice empty staffs */
	if (what && o_ptr->pval <= 0)
	{
		if (OPT(flush_failure)) flush();
		msg_format("The %s has no charges left.", what);
		o_ptr->ident |= (IDENT_EMPTY);
		return FALSE;
	}

	return TRUE;
}


typedef enum {
	ART_TAG_NONE,
	ART_TAG_NAME,
	ART_TAG_KIND,
	ART_TAG_VERB,
	ART_TAG_VERB_IS
} art_tag_t;

static art_tag_t art_tag_lookup(const char *tag)
{
	if (strncmp(tag, "name", 4) == 0)
		return ART_TAG_NAME;
	else if (strncmp(tag, "kind", 4) == 0)
		return ART_TAG_KIND;
	else if (strncmp(tag, "s", 1) == 0)
		return ART_TAG_VERB;
	else if (strncmp(tag, "is", 2) == 0)
		return ART_TAG_VERB_IS;
	else
		return ART_TAG_NONE;
}



/*** Inscriptions ***/



/**
 * Remove the inscription from an object
 * XXX Mention item (when done)?
 */
void do_cmd_uninscribe(cmd_code code, cmd_arg args[])
{
  int item;
  
  object_type *o_ptr;
  
  cptr q, s;
  
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Un-inscribe which item? ";
      s = "You have nothing to un-inscribe.";
      if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;
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
  
  /* Nothing to remove */
  if (!o_ptr->note)
    {
      msg_print("That item had no inscription to remove.");
      return;
    }
  
  /* Message */
  msg_print("Inscription removed.");
  
  /* Remove the incription */
  o_ptr->note = 0;
  
  /* Combine the pack (or the quiver) */
  p_ptr->notice |= (PN_COMBINE | PN_SQUELCH);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP);
}


/**
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(cmd_code code, cmd_arg args[])
{
  int item;
  
  object_type *o_ptr;
  
  char o_name[120];
  
  char tmp[81];
  
  cptr q, s;
  
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Inscribe which item? ";
      s = "You have nothing to inscribe.";
      if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;
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
  
  /* Describe the activity */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Message */
  msg_format("Inscribing %s.", o_name);
  msg_print(NULL);
  
  /* Start with nothing */
  strcpy(tmp, "");
  
  /* Use old inscription */
  if (o_ptr->note)
    {
      /* Start with the old inscription */
      strcpy(tmp, quark_str(o_ptr->note));
    }
  
  /* Get a new inscription (possibly empty) */
  if (get_string("Inscription: ", tmp, 81))
    {
      /* Save the inscription */
      o_ptr->note = quark_add(tmp);
      
      /* Combine the pack (or the quiver) */
      p_ptr->notice |= (PN_COMBINE | PN_SQUELCH);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN | PW_EQUIP);
    }
}

void textui_obj_inscribe(object_type *o_ptr, int item)
{
	char o_name[80];
	char tmp[80] = "";

	//	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);
	object_desc(o_name, o_ptr, TRUE, 3);
	msg_format("Inscribing %s.", o_name);
	message_flush();

	/* Use old inscription */
	if (o_ptr->note)
		strnfmt(tmp, sizeof(tmp), "%s", quark_str(o_ptr->note));

	/* Get a new inscription (possibly empty) */
	if (get_string("Inscription: ", tmp, sizeof(tmp)))
	{
		cmd_insert(CMD_INSCRIBE);
		cmd_set_arg_item(cmd_get_top(), 0, item);
		cmd_set_arg_string(cmd_get_top(), 1, tmp);
	}
}


/*** Examination ***/
void textui_obj_examine(object_type *o_ptr, int item)
{
	char header[120];

	textblock *tb;
	region area = { 0, 0, 0, 0 };

	track_object(item);

	//tb = object_info(o_ptr, OINFO_NONE);
  /* Describe */
  object_info_screen(o_ptr, FALSE);
  //object_desc(header, sizeof(header), o_ptr, ODESC_PREFIX | ODESC_FULL);

  //textui_textblock_show(tb, area, format("%^s", header));
  //textblock_free(tb);
}




/*** Taking off/putting on ***/

/**
 * Take off an item, if not shapechanged.
 */
void do_cmd_takeoff(cmd_code code, cmd_arg args[])
{
  int item;
  
  object_type *o_ptr;
  
  cptr q, s;
  
  if (SCHANGE)
    {
      msg_print("You cannot take off equipment while shapechanged.");
      msg_print("Use the ']' command to return to your normal form.");
      return;
    }
  
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Take off which item? ";
      s = "You are not wearing anything to take off.";
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
  
  
  /* Item is cursed */
  if (o_ptr->flags_curse & CF_STICKY_WIELD)
    {
      /* Oops */
      msg_print("Hmmm, it seems to be cursed.");

      /* Notice */
      notice_curse(CF_STICKY_WIELD, item + 1);
      
      /* Nope */
      return;
    }
  
  
  /* Take a partial turn */
  p_ptr->energy_use = 50;
  
  /* Ensure that the shield hand is used, if a shield is available. */
  if (item == INVEN_WIELD) p_ptr->shield_on_back = FALSE;
  
  /* Take off the item */
  (void)inven_takeoff(item, 255);
}


/**
 * The "wearable" tester
 */
static bool item_tester_hook_wear(const object_type *o_ptr)
{
  /* Check for a usable slot */
  if (wield_slot((object_type *)o_ptr) >= INVEN_WIELD) return (TRUE);
  
  /* Assume not wearable */
  return (FALSE);
}

/**
 * Wield or wear a single item from the pack or floor, if not shapechanged.
 */
void do_cmd_wield(cmd_code code, cmd_arg args[])
{
  int item, slot, num;
  
  object_type *o_ptr;
  
  object_type *i_ptr;

  object_type *l_ptr = &inventory[INVEN_LITE];

  object_type object_type_body;

  object_kind *k_ptr;
  
  cptr act;
  
  cptr q, s;
  
  char o_name[120];

  bool throwing;
  
  if (SCHANGE)
    {
      msg_print("You cannot wield equipment while shapechanged.");
      msg_print("Use the ']' command to return to your normal form.");
      return;
    }
  
  /* Restrict the choices */
  item_tester_hook = item_tester_hook_wear;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    { 
      q = "Wear/Wield which item? ";
      s = "You have nothing you can wear or wield.";
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
  
  /* Get the object kind */
  k_ptr = &k_info[o_ptr->k_idx];

  /* Throwing weapon or ammo? */
  throwing = ((o_ptr->flags_obj & OF_THROWING) ? TRUE : FALSE);
  
  /* Check the slot */
  slot = wield_slot(o_ptr);
  
  /* Ask for ring to replace */
  if ((o_ptr->tval == TV_RING) &&
      inventory[INVEN_LEFT].k_idx &&
      inventory[INVEN_RIGHT].k_idx)
    {
      /* Restrict the choices */
      item_tester_tval = TV_RING;
      
      /* Choose a ring from the equipment only */
      q = "Replace which ring? ";
      s = "Oops.";
      if (!get_item(&slot, q, s, USE_EQUIP)) return;
    }
  
  /* Ask where to put a throwing weapon */
  if (((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_HAFTED) || 
       (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)) && throwing)
    {
      /* Stick it in the belt? */
      if (get_check("Equip in throwing belt?")) slot = INVEN_Q0;
      
      /* Flush */
      msg_print(NULL);
    }

  /* Prevent wielding into a cursed slot */
  if (slot < INVEN_Q0) 
    {
      object_type *i_ptr = &inventory[slot];

      if (i_ptr->flags_curse & CF_STICKY_WIELD)
	{
	  /* Describe it */
	  object_desc(o_name, i_ptr, FALSE, 0);
	  
	  /* Message */
	  msg_format("The %s you are %s appears to be cursed.",
		     o_name, describe_use(slot));
	  
	  /* Notice */
	  notice_curse(CF_STICKY_WIELD, slot + 1);

	  /* Cancel the command */
	  return;
	}
    }
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Obtain local object */
  object_copy(i_ptr, o_ptr);
  
  
  /* Usually, we wear or wield only one item. */
  num = 1;
  
  /* Ammo goes in quiver slots, which have special rules. */
  if (slot == INVEN_Q0)
    {
      int ammo_num = 0;
      int added_ammo_num;
      int attempted_quiver_slots;
      
      /* Get a quantity */
      num = get_quantity(NULL, o_ptr->number);
      
      /* Cancel */
      if (!num) return;
      
      /* Count number of missiles in the quiver slots. */
      ammo_num = quiver_count();
      
      /* Effective added count */
      added_ammo_num = (throwing ? num * THROWER_AMMO_FACTOR : num);
      
      /*
       * If the ammo now being added will make the quiver take up another
       * backpack slot, and there are none available, refuse to wield
       * the new ammo.
       */
      
      /* How many quiver slots would be needed */
      /* Rebate one if the equip action would free a space */
      attempted_quiver_slots = ((ammo_num + added_ammo_num + 98) / 99) - 
	(((item >= 0) && (num == o_ptr->number)) ? 1 : 0);
      
      /* Is there room, given normal inventory? */
      if ((attempted_quiver_slots + p_ptr->inven_cnt) > INVEN_PACK)
	{
	  msg_print("Your quiver needs more backpack space.");
	  return;
	}
      
      /* Find a slot that can hold more ammo. */
      slot = process_quiver(num, o_ptr);
      
      if (!slot)
	{
	  /* No space. */
	  msg_print("Your quiver is full.");
	  return;
	}
      
      /* Quiver will be reorganized (again) later. */
      p_ptr->notice |= (PN_COMBINE);
    }
  
  /* Modify quantity */
  i_ptr->number = num;
  
  
  /* Decrease the item (from the pack) */
  if (item >= 0)
    {
      inven_item_increase(item, -num);
      inven_item_optimize(item);
    }
  
  /* Decrease the item (from the floor) */
  else
    {
      floor_item_increase(0 - item, -num);
      floor_item_optimize(0 - item);
    }
  
  /* Access the wield slot */
  o_ptr = &inventory[slot];
  
  
  /* Handle existing item. */
  if (o_ptr->k_idx)
    {
      /* Take off existing item, unless in the quiver. */
      if ((slot < INVEN_Q0) || (slot > INVEN_Q9))
	{
	  (void)inven_takeoff(slot, 255);
	}
      
      /* Combine existing ammo with new. */
      else
	{
	  p_ptr->equip_cnt--;
	  p_ptr->total_weight -= o_ptr->weight * o_ptr->number;
	  i_ptr->number += o_ptr->number;
	}
    }
  
  /* Wear the new stuff */
  object_copy(o_ptr, i_ptr);
  
  /* Increase the weight */
  p_ptr->total_weight += i_ptr->weight * i_ptr->number;
  
  /* Increment the equip counter by hand */
  p_ptr->equip_cnt++;
  
  
  /* If he wields a weapon that requires two hands, or hasn't the 
   * strength to wield a weapon that is usually wielded with both hands
   * one-handed (normally requires 18/140 to 18/160 STR), the 
   * character will automatically carry any equipped shield on his back. -LM-
   */
  if ((slot == INVEN_WIELD) && (inventory[INVEN_ARM].k_idx))
    {
      if ((o_ptr->flags_obj & OF_TWO_HANDED_REQ) || 
	  ((o_ptr->flags_obj & OF_TWO_HANDED_DES) && 
	   (p_ptr->stat_ind[A_STR] < 
	    29 + ((o_ptr->weight / 50 > 8) ? 8 : (o_ptr->weight / 50)))))
	{
	  p_ptr->shield_on_back = TRUE;
	}
      else 
	p_ptr->shield_on_back = FALSE;
    }
  
  /* A character using both hands to wield his melee weapon will use his 
   * back to carry an equipped shield. -LM-
   */
  if ((slot == INVEN_ARM) && (inventory[INVEN_WIELD].k_idx))
    {
      /* Access the wield slot */
      i_ptr = &inventory[INVEN_WIELD];
      
      if ((i_ptr->flags_obj & OF_TWO_HANDED_REQ) || 
	  ((i_ptr->flags_obj & OF_TWO_HANDED_DES) && 
	   (p_ptr->stat_ind[A_STR] < 
	    29 + (i_ptr->weight / 50 > 8 ? 8 : i_ptr->weight / 50))))
	{
	  p_ptr->shield_on_back = TRUE;
	}
      else p_ptr->shield_on_back = FALSE;
      
    }

  /* Set item handling -GS- and checking turn found for artifacts -NRM- */
  if (o_ptr->name1)
    {
      
      /* Is the artifact a set item? */
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      if (a_ptr->set_no!=0)
	{
	  /*
	   * The object is part of a set. Check to see if rest of
	   * set is equiped.
	   */
	  if (check_set(a_ptr->set_no)) 
	    {
	      /* add bonuses */
	      apply_set(a_ptr->set_no);
	    }
	}

      /* Have we registered this as found before ? */
      if (a_info[o_ptr->name1].creat_turn < 2)
	  {
		  a_info[o_ptr->name1].creat_turn = turn;
		  a_info[o_ptr->name1].p_level = p_ptr->lev;
	  }
    }

  /* Wielding off the floor will id if holding the Stone of Lore */

  if (item < 0)
  {
    if ((!object_known_p(o_ptr)) && (l_ptr->sval == SV_STONE_LORE)) 
      identify_object(o_ptr);

    /* And we autoinscribe here too */
    apply_autoinscription(o_ptr);

    /* And notice dice and AC */

  }

  /* Where is the item now */
  if (slot == INVEN_WIELD)
    {
      act = "You are wielding";
    }
  else if (slot == INVEN_BOW)
    {
      act = "You are shooting with";
    }
  else if (slot == INVEN_LITE)
    {
      act = "Your light source is";
    }
  else if ((slot < INVEN_Q0) || (slot > INVEN_Q9))
    {
      act = "You are wearing";
    }
  else
    {
      act = "You have readied";
    }
  
  /* Notice dice, AC, jewellery sensation ID and other obvious stuff */
  notice_other((IF_DD_DS | IF_AC), slot + 1);
  o_ptr->id_obj |= ((o_ptr->flags_obj) & OF_OBVIOUS_MASK);
  if (is_armour(o_ptr) && k_ptr->to_h) notice_other(IF_TO_H, slot + 1);
  if ((slot == INVEN_RIGHT) || (slot == INVEN_LEFT) || (slot == INVEN_NECK))
    {
      notice_obj(p_ptr->id_obj, slot + 1);
      notice_other(p_ptr->id_other, slot + 1);
    }

  /* Average things are average */
  if ((o_ptr->feel == FEEL_AVERAGE) && (is_weapon(o_ptr) || is_armour(o_ptr)))
    notice_other((IF_AC | IF_TO_A | IF_TO_H | IF_TO_D), slot + 1);

  /* Object has been worn */
  o_ptr->ident |= IDENT_WORN;

  /* Describe the result */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Message */
  sound(MSG_WIELD);
  msg_format("%s %s (%c).", act, o_name, index_to_label(slot));

  if (!object_known_p(o_ptr))
    {
      int feel;
      bool heavy = FALSE;
      
      heavy = (check_ability(SP_PSEUDO_ID_HEAVY));
      
      /* Check for a feeling */
      feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
      
      if (!(o_ptr->feel == feel))
	{
	  /* Get an object description */
	  object_desc(o_name, o_ptr, FALSE, 0);
	  
	  msg_format("You feel the %s (%c) you are %s %s %s...",
		     o_name, index_to_label(slot), describe_use(slot),
		     ((o_ptr->number == 1) ? "is" : "are"), feel_text[feel]);
	  
	  /* We have "felt" it */
	  o_ptr->ident |= (IDENT_SENSE);
	  
	  /* Inscribe it textually */
	  o_ptr->feel = feel;
	  
	  /* Set squelch flag as appropriate */
	  p_ptr->notice |= PN_SQUELCH;
	}
    }

  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Recalculate torch */
  p_ptr->update |= (PU_TORCH);
  
  /* Recalculate mana */
  p_ptr->update |= (PU_MANA);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1 
		    | PW_ITEMLIST);
}


/**
 * Drop an item
 */
void do_cmd_drop(cmd_code code, cmd_arg args[])
{
  int item, amt;
  
  object_type *o_ptr;
  
  cptr q, s;
  
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Drop which item? ";
      s = "You have nothing to drop.";
      if (SCHANGE)
	{
	  if (!get_item(&item, q, s, (USE_INVEN))) return;
	}
      else
	{
	  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN))) return;
	}
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
  
  /* Get a quantity */
  amt = get_quantity(NULL, o_ptr->number);
  
  /* Allow user abort */
  if (amt <= 0) return;
  
  /* Hack -- Cannot drop some cursed items */
  if (o_ptr->flags_curse & CF_STICKY_CARRY)
    {
      /* Oops */
      msg_print("Hmmm, it seems to be cursed.");

      /* Notice */
      notice_curse(CF_STICKY_CARRY, item + 1);
      
      /* Nope */
      return;
    }
  
  if ((item >= INVEN_WIELD) && (o_ptr->flags_curse & CF_STICKY_WIELD))
    {
      /* Oops */
      msg_print("Hmmm, it seems to be cursed.");

      /* Notice */
      notice_curse(CF_STICKY_WIELD, item + 1);
      
      /* Nope */
      return;
    }
  
  /* Ensure that the shield hand is used, if a shield is available. */
  if (item == INVEN_WIELD) p_ptr->shield_on_back = FALSE;
  
  /* Take a partial turn */
  p_ptr->energy_use = 50;
  
  /* Drop (some of) the item */
  inven_drop(item, amt);
}

void textui_obj_wield(object_type *o_ptr, int item)
{
	int slot = wield_slot(o_ptr);

	/* Usually if the slot is taken we'll just replace the item in the slot,
	 * but in some cases we need to ask the user which slot they actually
	 * want to replace */
	if (inventory[slot].k_idx)
	{
		if (o_ptr->tval == TV_RING)
		{
			cptr q = "Replace which ring? ";
			cptr s = "Error in obj_wield, please report";
			item_tester_hook = obj_is_ring;
			if (!get_item(&slot, q, s, USE_EQUIP)) return;
		}

		if (is_missile(o_ptr) && !object_similar(&inventory[slot],
			o_ptr))
		{
			cptr q = "Replace which ammunition? ";
			cptr s = "Error in obj_wield, please report";
			item_tester_hook = obj_is_ammo;
			if (!get_item(&slot, q, s, USE_EQUIP)) return;
		}
	}

	cmd_insert(CMD_WIELD);
	cmd_set_arg_item(cmd_get_top(), 0, item);
	cmd_set_arg_number(cmd_get_top(), 1, slot);
}



/*** Using items the traditional way ***/

/*
 * Use an object the right way.
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
 */
void do_cmd_use(cmd_code code, cmd_arg args[])
{
	int item = args[0].item;
	object_type *o_ptr = object_from_item_idx(item);
	int effect;
	bool ident = FALSE, used = FALSE;
	bool was_aware = object_aware_p(o_ptr);
	int dir = 5;
	int px = p_ptr->px, py = p_ptr->py;
	int snd, boost, level;
	use_type use;
	int items_allowed = 0;

	/* Determine how this item is used. */
	if (obj_is_rod(o_ptr))
	{
		if (!obj_can_zap(o_ptr))
		{
			msg_print("That rod is still charging.");
			return;
		}

		use = USE_TIMEOUT;
		snd = MSG_ZAP_ROD;
		items_allowed = USE_INVEN | USE_FLOOR;
	}
	else if (obj_is_wand(o_ptr))
	{
		if (!obj_has_charges(o_ptr))
		{
			msg_print("That wand has no charges.");
			return;
		}

		use = USE_CHARGE;
		snd = MSG_ZAP_ROD;
		items_allowed = USE_INVEN | USE_FLOOR;
	}
	else if (obj_is_staff(o_ptr))
	{
		if (!obj_has_charges(o_ptr))
		{
			msg_print("That staff has no charges.");
			return;
		}

		use = USE_CHARGE;
		snd = MSG_USE_STAFF;
		items_allowed = USE_INVEN | USE_FLOOR;
	}
	else if (obj_is_food(o_ptr))
	{
		use = USE_SINGLE;
		snd = MSG_EAT;
		items_allowed = USE_INVEN | USE_FLOOR;
	}
	else if (obj_is_potion(o_ptr))
	{
		use = USE_SINGLE;
		snd = MSG_QUAFF;
		items_allowed = USE_INVEN | USE_FLOOR;
	}
	else if (obj_is_scroll(o_ptr))
	{
		/* Check player can use scroll */
		if (!player_can_read())
			return;

		use = USE_SINGLE;
		snd = MSG_GENERIC;
		items_allowed = USE_INVEN | USE_FLOOR;
	}
	else if (obj_is_activatable(o_ptr))
	{
		if (!obj_can_activate(o_ptr))
		{
			msg_print("That item is still charging.");
			return;
		}

		use = USE_TIMEOUT;
		snd = MSG_ACT_ARTIFACT;
		items_allowed = USE_EQUIP;
	}
	else
	{
		msg_print("The item cannot be used at the moment");
	}

	/* Check if item is within player's reach. */
	if (items_allowed == 0 || !item_is_available(item, NULL, items_allowed))
	{
		msg_print("You cannot use that item from its current location.");
		return;
	}

	/* track the object used */
	track_object(item);

	/* Figure out effect to use */
	effect = object_effect(o_ptr);

	/* If the item requires a direction, get one (allow cancelling) */
	if (obj_needs_aim(o_ptr))
		dir = args[1].direction;

	/* Check for use if necessary, and execute the effect */
	if ((use != USE_CHARGE && use != USE_TIMEOUT) ||
	    check_devices(o_ptr))
	{
	        /* Make a noise! */
	        sound(snd);
		level = k_info[o_ptr->k_idx].level;

		/* Quit if the item wasn't used and no knowledge was gained */
		if (!used && (was_aware || !ident)) return;
	}

	/* If the item is a null pointer or has been wiped, be done now */
	if (!o_ptr || o_ptr->k_idx <= 1) return;

	/* Food feeds the player */
	if (o_ptr->tval == TV_FOOD || o_ptr->tval == TV_POTION)
		(void)set_food(p_ptr->food + o_ptr->pval);

	/* Use the turn */
	p_ptr->energy_use = 100;

	/* Mark as tried and redisplay */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_OBJECT);

	/*
	 * If the player becomes aware of the item's function, then mark it as
	 * aware and reward the player with some experience.  Otherwise, mark
	 * it as "tried".
	 */
	if (ident && !was_aware)
	{
		/* Object level */
		int lev = k_info[o_ptr->k_idx].level;

		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev / 2)) / p_ptr->lev);
		p_ptr->notice |= PN_SQUELCH;
	}
	else if (used)
	{
		object_tried(o_ptr);
	}

	/* If there are no more of the item left, then we're done. */
	if (!o_ptr->number) return;

	/* Chargeables act differently to single-used items when not used up */
	if (used && use == USE_CHARGE)
	{
		/* Use a single charge */
		o_ptr->pval--;

		/* Describe charges */
		if (item >= 0)
			inven_item_charges(item);
		else
			floor_item_charges(0 - item);
	}
	else if (used && use == USE_SINGLE)
	{
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
	}
	
}


/*** Refuelling ***/
/**
 * An "item_tester_hook" for refilling lanterns
 */
static bool item_tester_refill_lantern(const object_type *o_ptr)
{
  /* Flasks of oil are okay */
  if (o_ptr->tval == TV_FLASK) return (TRUE);
  
  /* Non-empty lanterns are okay */
  if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN) &&
      (o_ptr->pval > 0))
    {
      return (TRUE);
    }
  
  /* Assume not okay */
  return (FALSE);
}



/**
 * Refill the players lamp (from the pack or floor)
 */
static void refill_lamp(void)
{
  int item;
  
  object_type *o_ptr;
  object_type *j_ptr;
  
  cptr q, s;
  
  
  /* Restrict the choices */
  item_tester_hook = item_tester_refill_lantern;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Refill with which flask? ";
      s = "You have no flasks of oil.";
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
  
  
  /* Access the lantern */
  j_ptr = &inventory[INVEN_LITE];
  
  /* Refuel */
  j_ptr->pval += o_ptr->pval;
  
  /* Message */
  msg_print("You fuel your lamp.");
  
  /* Comment */
  if (j_ptr->pval >= FUEL_LAMP)
    {
      j_ptr->pval = FUEL_LAMP;
      msg_print("Your lamp is full.");
    }
  
  /* Use fuel from a lantern */
  if (o_ptr->sval == SV_LITE_LANTERN)
    {
      /* No more fuel */
      o_ptr->pval = 0;
      
      /* Combine / Reorder the pack (later) */
      p_ptr->notice |= (PN_COMBINE | PN_REORDER);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN);
    }
  
  /* Decrease the item (from the pack) */
  else if (item >= 0)
    {
      inven_item_increase(item, -1);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
  
  /* Decrease the item (from the floor) */
  else
    {
      floor_item_increase(0 - item, -1);
      floor_item_describe(0 - item);
      floor_item_optimize(0 - item);
    }
  
  /* Recalculate torch */
  p_ptr->update |= (PU_TORCH);
}



/**
 * An "item_tester_hook" for refilling torches
 */
static bool item_tester_refuel_torch(const object_type *o_ptr)
{
  /* Torches are okay */
  if ((o_ptr->tval == TV_LITE) &&
      (o_ptr->sval == SV_LITE_TORCH)) return (TRUE);
  
  /* Assume not okay */
  return (FALSE);
}


/**
 * Refuel the players torch (from the pack or floor)
 */
static void refuel_torch(void)
{
  int item;
  
  object_type *o_ptr;
  object_type *j_ptr;
  
  cptr q, s;
  
  
  /* Restrict the choices */
  item_tester_hook = item_tester_refuel_torch;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Refuel with which torch? ";
      s = "You have no extra torches.";
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
  
  
  /* Access the primary torch */
  j_ptr = &inventory[INVEN_LITE];
  
  /* Refuel */
  j_ptr->pval += o_ptr->pval + 5;
  
  /* Message */
  msg_print("You combine the torches.");
  
  /* Over-fuel message */
  if (j_ptr->pval >= FUEL_TORCH)
    {
      j_ptr->pval = FUEL_TORCH;
      msg_print("Your torch is fully fueled.");
    }
  
  /* Refuel message */
  else
    {
      msg_print("Your torch glows more brightly.");
    }
  
  /* Decrease the item (from the pack) */
  if (item >= 0)
    {
      inven_item_increase(item, -1);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
  
  /* Decrease the item (from the floor) */
  else
    {
      floor_item_increase(0 - item, -1);
      floor_item_describe(0 - item);
      floor_item_optimize(0 - item);
    }
  
  /* Recalculate torch */
  p_ptr->update |= (PU_TORCH);
}



void do_cmd_refill(cmd_code code, cmd_arg args[])
{
	object_type *j_ptr = &inventory[INVEN_LITE];

	int item = args[0].item;
	object_type *o_ptr = object_from_item_idx(item);

	if (!item_is_available(item, NULL, USE_INVEN | USE_FLOOR))
	{
		msg_print("You do not have that item to refill with it.");
		return;
	}

	if (j_ptr->tval != TV_LITE)
	{
		msg_print("You are not wielding a light.");
		return;
	}

	/* It's a lamp */
	else if (j_ptr->sval == SV_LITE_LANTERN)
		refill_lamp();

	/* It's a torch */
	else if (j_ptr->sval == SV_LITE_TORCH)
		refuel_torch();

	else
	{
		msg_print("Your light cannot be refilled.");
		return;
	}

	p_ptr->energy_use = 50;
}



/*** Spell casting ***/


/**
 * Study a book to gain a new spell/prayer
 */
void do_cmd_study_spell(cmd_code code, cmd_arg args[])
{
	int spell = args[0].choice;

	int item_list[INVEN_TOTAL + MAX_FLOOR_STACK];
	int item_num;
	int i;

	/* Check the player can study at all atm */
	if (!player_can_study())
		return;

	/* Check that the player can actually learn the nominated spell. */
	item_tester_hook = obj_can_browse;
	item_num = scan_items(item_list, N_ELEMENTS(item_list), (USE_INVEN | USE_FLOOR));

	/* Check through all available books */
	for (i = 0; i < item_num; i++)
	{
		if (spell_in_book(spell, item_list[i]))
		{
			if (spell_okay_to_study(spell))
			{
				/* Spell is in an available book, and player is capable. */
				spell_learn(spell);
				p_ptr->energy_use = 100;
			}
			else
			{
				/* Spell is present, but player incapable. */
				msg_format("You cannot learn that spell.");
			}

			return;
		}
	}

  /* Forget the item_tester_hook */
  item_tester_hook = NULL;
  
}

/**
 * Calculate level boost for Channeling ability.
 */
int get_channeling_boost(void)
{
  long max_channeling = 45 + (2 * p_ptr->lev);
  long channeling = 0L;
  int boost;
  
  if (p_ptr->msp > 0) channeling = (max_channeling * p_ptr->csp * p_ptr->csp) 
			/ (p_ptr->msp * p_ptr->msp);
  boost = ((int) channeling + 5) / 10;
  
  return(boost);
}


/**
 * Warriors will eventually learn to pseudo-probe monsters.  If they use 
 * the browse command, give ability information. -LM-
 */
static void warrior_probe_desc(void)
{
  /* Save screen */
  screen_save();
  
  /* Erase the screen */
  Term_clear();

  /* Set the indent */
  text_out_indent = 5;
  
  /* Title in light blue. */
  text_out_to_screen(TERM_L_BLUE, "Warrior Pseudo-Probing Ability:");
  text_out_to_screen(TERM_WHITE, "\n\n");
  
  /* Print out information text. */
  text_out_to_screen(TERM_WHITE, "Warriors learn to probe monsters at level 35.  This costs nothing except a full turn.  When you reach this level, type 'm', and target the monster you would like to learn more about.  This reveals the monster's race, approximate HPs, and basic resistances.  Be warned:  the information given is not always complete...");
  text_out_to_screen(TERM_WHITE, "\n\n\n");
  
  /* The "exit" sign. */
  text_out_to_screen(TERM_WHITE, "    (Press any key to continue.)\n");
  
  /* Wait for it. */
  (void)inkey_ex();
  
  /* Load screen */
  screen_load();
}

/**
 * Warriors will eventually learn to pseudo-probe monsters.  This allows 
 * them to better choose between slays and brands.  They select a target, 
 * and receive (slightly incomplete) infomation about racial type, 
 * basic resistances, and HPs. -LM-
 */
static void pseudo_probe(void)
{
  char m_name[80];
  
  /* Acquire the target monster */
  int m_idx = p_ptr->target_who;
  monster_type *m_ptr = &m_list[m_idx];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  monster_lore *l_ptr = &l_list[m_ptr->r_idx];
  
  int approx_hp;
  int approx_mana=0;
  
  
  /* If no target monster, fail. */
  if (p_ptr->target_who < 1) 
    {
      msg_print("You must actually target a monster.");
      return;
    }
  
  else
    {
      /* Get "the monster" or "something" */
      monster_desc(m_name, m_ptr, 0x04);
      
      /* Approximate monster HPs */
      approx_hp = m_ptr->hp - randint0(m_ptr->hp / 4) + 
	randint0(m_ptr->hp / 4);
      
      /* Approximate monster HPs */
      if (r_ptr->mana)
	approx_mana = m_ptr->mana - randint0(m_ptr->mana / 4) + 
	  randint0(m_ptr->mana / 4);
      
      /* Describe the monster */
      if (!(r_ptr->mana))
	msg_format("%^s has about %d hit points.", m_name, approx_hp);
      else
	msg_format("%^s has about %d hit points and about %d mana.", 
		   m_name, approx_hp, approx_mana);
      
      /* Learn some flags.  Chance of omissions. */
      if ((r_ptr->flags3 & (RF3_ANIMAL)) && (randint0(20) != 1))
	l_ptr->flags3 |= (RF3_ANIMAL);
      if ((r_ptr->flags3 & (RF3_EVIL)) && (randint0(10) != 1))
	l_ptr->flags3 |= (RF3_EVIL);
      if ((r_ptr->flags3 & (RF3_UNDEAD)) && (randint0(20) != 1))
	l_ptr->flags3 |= (RF3_UNDEAD);
      if ((r_ptr->flags3 & (RF3_DEMON)) && (randint0(20) != 1))
	l_ptr->flags3 |= (RF3_DEMON);
      if ((r_ptr->flags3 & (RF3_ORC)) && (randint0(20) != 1))
	l_ptr->flags3 |= (RF3_ORC);
      if ((r_ptr->flags3 & (RF3_TROLL)) && (randint0(20) != 1))
	l_ptr->flags3 |= (RF3_TROLL);
      if ((r_ptr->flags3 & (RF3_GIANT)) && (randint0(10) != 1))
	l_ptr->flags3 |= (RF3_GIANT);
      if ((r_ptr->flags3 & (RF3_DRAGON)) && (randint0(20) != 1))
	l_ptr->flags3 |= (RF3_DRAGON);
      if ((r_ptr->flags3 & (RF3_IM_ACID)) && (randint0(5) != 1))
	l_ptr->flags3 |= (RF3_IM_ACID);
      if ((r_ptr->flags3 & (RF3_IM_ELEC)) && (randint0(5) != 1))
	l_ptr->flags3 |= (RF3_IM_ELEC);
      if ((r_ptr->flags3 & (RF3_IM_FIRE)) && (randint0(5) != 1))
	l_ptr->flags3 |= (RF3_IM_FIRE);
      if ((r_ptr->flags3 & (RF3_IM_COLD)) && (randint0(5) != 1))
	l_ptr->flags3 |= (RF3_IM_COLD);
      if ((r_ptr->flags3 & (RF3_IM_POIS)) && (randint0(5) != 1))
	l_ptr->flags3 |= (RF3_IM_POIS);
      
      /* Confirm success. */
      msg_print("You feel you know more about this monster...");
      
      /* Update monster recall window */
      if (p_ptr->monster_race_idx == m_ptr->r_idx)
	{
	  /* Window stuff */
	  p_ptr->window |= (PW_MONSTER);
	}
    }
}

/**
 * Cast a spell or pray a prayer.
 */
void do_cmd_cast(cmd_code code, cmd_arg args[])
{
	int spell = args[0].choice;
	int dir = args[1].direction;

	int item_list[INVEN_TOTAL + MAX_FLOOR_STACK];
	int item_num;
	int i;

	int chance, beam, item;
  s16b shape = 0;
  
  bool failed = FALSE;
  
  int plev = p_ptr->lev;
  
  object_type *o_ptr;
  
  magic_type *s_ptr;
  
  cptr spell_noun = "";
  cptr book_noun = "";
  cptr verb = "";
  
  cptr q = "";
  cptr s = "";
  
  
      /* Warriors will eventually learn to pseudo-probe monsters. */
      if (check_ability(SP_PROBE)) 
	{
	  if (p_ptr->lev < 35) 
	    msg_print("You do not know how to probe monsters yet.");
	  else if ((p_ptr->confused) || (p_ptr->image))
	    msg_print("You feel awfully confused.");
	  else 
	    {
	      /* Get a target. */
	      msg_print("Target a monster to probe.");
	      if (!get_aim_dir(&dir)) return;
	      
	      /* Low-level probe spell. */
	      pseudo_probe();
	      
	      /* Take a turn */
	      p_ptr->energy_use = 100;
	    }
	}
  
  /* Require spell ability. */
  if (!player_can_cast)

  /* Determine magic description. */
  if (mp_ptr->spell_book == TV_MAGIC_BOOK) spell_noun = "spell";
  if (mp_ptr->spell_book == TV_PRAYER_BOOK) spell_noun = "prayer";
  if (mp_ptr->spell_book == TV_DRUID_BOOK) spell_noun = "druidic lore";
  if (mp_ptr->spell_book == TV_NECRO_BOOK) spell_noun = "ritual";
  
  /* Determine spellbook description. */
  if (mp_ptr->spell_book == TV_MAGIC_BOOK) book_noun = "magic book";
  if (mp_ptr->spell_book == TV_PRAYER_BOOK) book_noun = "holy book";
  if (mp_ptr->spell_book == TV_DRUID_BOOK) book_noun = "stone";
  if (mp_ptr->spell_book == TV_NECRO_BOOK) book_noun = "tome";
  
  /* Determine method description. */
  if (mp_ptr->spell_book == TV_MAGIC_BOOK) verb = "cast";
  if (mp_ptr->spell_book == TV_PRAYER_BOOK) verb = "pray";
  if (mp_ptr->spell_book == TV_DRUID_BOOK) verb = "use";
  if (mp_ptr->spell_book == TV_NECRO_BOOK) verb = "perform";
  
  
  /* Restrict choices to spell books of the player's realm. */
  item_tester_hook = obj_can_browse;
	item_num = scan_items(item_list, N_ELEMENTS(item_list), (USE_INVEN | USE_FLOOR));
  
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get a realm-flavored description. */
  else
    {
      if (mp_ptr->spell_book == TV_MAGIC_BOOK) 
	{
	  q = "Use which magic book? ";
	  s = "You have no magic books that you can use.";
	}
      if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	{
	  q = "Use which holy book? ";
	  s = "You have no holy books that you can use.";
	}
      if (mp_ptr->spell_book == TV_DRUID_BOOK)
	{
	  q = "Use which stone of lore? ";
	  s = "You have no stones that you can use.";
	}
      if (mp_ptr->spell_book == TV_NECRO_BOOK)
	{
	  q = "Use which tome? ";
	  s = "You have no tomes that you can use.";
	}
      
      /* Get an item */
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
  
  /* Track the object kind */
  object_kind_track(o_ptr->k_idx);
  
  /* Hack -- Handle stuff */
  handle_stuff();
  
  
  /* Access the spell */
  s_ptr = &mp_ptr->info[spell];
  
  
  /* Verify "dangerous" spells */
  if (s_ptr->smana > p_ptr->csp)
    {
      /* Warning */
      msg_format("You do not have enough mana to %s this %s.", verb, spell_noun);
      
      /* Verify */
      if (!get_check("Attempt it anyway? ")) return;
    }
  
  
  
  /* Forget the item_tester_hook */
  item_tester_hook = NULL;
  
}


/**
 * Study a book to gain a new spell/prayer
 */
void do_cmd_study_book(cmd_code code, cmd_arg args[])
{
	int book = args[0].item;
	object_type *o_ptr = object_from_item_idx(book);

	int spell = -1;
	int i, k = 0;

  cptr p = "";
  cptr r = "";
  
	/* Check the player can study at all atm */
	if (!player_can_study())
		return;

	/* Check that the player has access to the nominated spell book. */
	if (!item_is_available(book, obj_can_browse, (USE_INVEN | USE_FLOOR)))
	{
		msg_format("That item is not within your reach.");
		return;
	}

  /* Determine magic description. */
  if (mp_ptr->spell_book == TV_MAGIC_BOOK) p = "spell";
  if (mp_ptr->spell_book == TV_PRAYER_BOOK) p = "prayer";
  if (mp_ptr->spell_book == TV_DRUID_BOOK) p = "druidic lore";
  if (mp_ptr->spell_book == TV_NECRO_BOOK) p = "ritual";
  
  /* Determine spellbook description. */
  if (mp_ptr->spell_book == TV_MAGIC_BOOK) r = "magic book";
  if (mp_ptr->spell_book == TV_PRAYER_BOOK) r = "holy book";
  if (mp_ptr->spell_book == TV_DRUID_BOOK) r = "stone";
  if (mp_ptr->spell_book == TV_NECRO_BOOK) r = "tome";
  
  
  /* Track the object kind */
  object_kind_track(o_ptr->k_idx);
  
  /* Hack -- Handle stuff */
  handle_stuff();
  
  
	/* Extract spells */
	for (i = 0; i < SPELLS_PER_BOOK; i++)
	{
		int s = get_spell_index(o_ptr, i);
		
		/* Skip non-OK spells */
		if (s == -1) continue;
		if (!spell_okay_to_study(s)) continue;
		
		/* Apply the randomizer */
		if ((++k > 1) && (randint0(k) != 0)) continue;
		
		/* Track it */
		spell = s;
	}

 /* Nothing to study */
  if (spell < 0)
    {
      /* Message */
      msg_format("You cannot learn any %s%s that %s.", p, 
		 (mp_ptr->spell_book == TV_DRUID_BOOK) ? " from" : "s in", r);
      
      /* Abort */
      return;
    }

  spell_learn(spell);
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Forget the item_tester_hook */
  item_tester_hook = NULL;
  
}

