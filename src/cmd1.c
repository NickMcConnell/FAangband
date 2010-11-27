/** \file cmd1.c
    \brief Commands, part 1

 *  Searching, pickup, effects of traps, move one square (including special 
 * terrain effects), and the running algorithm.   
 *
 * Tim Baker's easy patch installed.
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
 * Search for hidden things
 */
void search(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, chance;
  
  s16b this_o_idx, next_o_idx = 0;
  
  
  /* Start with base search ability */
  chance = p_ptr->skill_srh;
  
  /* Penalize various conditions */
  if (p_ptr->blind || no_lite()) chance = chance / 10;
  if (p_ptr->confused || p_ptr->image) chance = chance / 10;
  
  /* Search the nearby grids, which are always in bounds */
  for (y = (py - 1); y <= (py + 1); y++)
    {
      for (x = (px - 1); x <= (px + 1); x++)
	{
	  feature_type *f_ptr = &f_info[cave_feat[y][x]];

	  /* Sometimes, notice things */
	  if (rand_int(100) < chance)
	    {
	      /* Invisible trap */
	      if (f_ptr->flags & TF_TRAP_INVIS)
		{
		  /* Pick a trap */
		  pick_trap(y, x);
		  
		  /* Message */
		  msg_print("You have found a trap.");
		  
		  /* Disturb */
		  disturb(0, 0);
		}
	      
	      /* Secret door */
	      if (cave_feat[y][x] == FEAT_SECRET)
		{
		  /* Message */
		  msg_print("You have found a secret door.");
		  
		  /* Pick a door */
		  place_closed_door(y, x);
		  
		  /* Disturb */
		  disturb(0, 0);
		}
	      
	      /* Scan all objects in the grid */
	      for (this_o_idx = cave_o_idx[y][x]; this_o_idx; 
		   this_o_idx = next_o_idx)
		{
		  object_type *o_ptr;
		  
		  /* Acquire object */
		  o_ptr = &o_list[this_o_idx];
		  
		  /* Acquire next object */
		  next_o_idx = o_ptr->next_o_idx;
		  
		  /* Skip non-chests */
		  if (o_ptr->tval != TV_CHEST) continue;
		  
		  /* Skip non-trapped chests */
		  if (!chest_traps[o_ptr->pval]) continue;
		  
		  /* Identify once */
		  if (!object_known_p(o_ptr))
		    {
		      /* Message */
		      msg_print("You have discovered a trap on the chest!");
		      
		      /* Know the trap */
		      object_known(o_ptr);
		      
		      /* Notice it */
		      disturb(0, 0);
		    }
		}
	    }
	}
    }
}


/**
 * Return TRUE if the given object is inscribed with "=g".
 *
 * Alternatively, also return TRUE if any similar item in the
 * backpack is marked "=g".
 */
static bool auto_pickup_check(object_type *o_ptr, bool check_pack)
{
	cptr s;
	
	/* Check inscription */
	if (o_ptr->note)
	{
		/* Find a '=' */
		s = strchr(quark_str(o_ptr->note), '=');

		/* Process preventions */
		while (s)
		{
			/* =g ('g'et) means auto pickup */
			if (s[1] == 'g') return (TRUE);

			/* Find another '=' */
			s = strchr(s + 1, '=');
		}

		/* Throwing weapons and ammo need no extra inscription */
		s = strchr(quark_str(o_ptr->note), '@');

		/* Process preventions */
		while (s)
		{
			/* =g ('g'et) means auto pickup */
			if ((s[1] == 'f') || (s[1] == 'v')) return (TRUE);

			/* Find another '=' */
			s = strchr(s + 1, '=');
		}
	}

	/* Optionally, check the backpack */
	if (check_pack)
	{
		int j;

		/* Look for similar and inscribed */
		for (j = 0; j < INVEN_PACK - p_ptr->pack_size_reduce; j++)
		{
			object_type *j_ptr = &inventory[j];

			/* Skip non-objects */
			if (!j_ptr->k_idx) continue;

			/* The two items must be able to combine */
			if (!object_similar(j_ptr, o_ptr)) continue;

			/* The backpack item must be inscribed */
			if (!j_ptr->note) continue;

			/* Find a '=' */
			s = strchr(quark_str(j_ptr->note), '=');

			/* Process preventions */
			while (s)
			{
				/* =g ('g'et) means auto pickup */
				if (s[1] == 'g') return (TRUE);

				/* Find another '=' */
				s = strchr(s + 1, '=');
			}
		}
	}

	/* Don't auto pickup */
	return (FALSE);
}

/**
 * Automatically carry ammunition and throwing weapons in the quiver,
 * if it is inscribed with "=g", or it matches something already in
 * the quiver.
 */
bool quiver_carry(object_type *o_ptr, int o_idx)
{
  int i;
  
  bool throwing;
  
  int ammo_num, added_ammo_num;
  int attempted_quiver_slots;
  
  bool blind = ((p_ptr->blind) || (no_lite()));
  bool autop;
  int old_num;
  
  object_type *i_ptr;
  

  /* Must be ammo or throwing weapon. */
  if ((!is_missile(o_ptr)) && (!(o_ptr->flags_obj & OF_THROWING)))
    {
      return (FALSE);
    }
  
  /* Throwing weapon? */
  throwing = (((o_ptr->flags_obj & OF_THROWING) && (!is_missile(o_ptr))) 
	      ? TRUE : FALSE);
  
  /* Count number of missiles in the quiver slots. */
  ammo_num = quiver_count();
  
  /* Check for autopickup */
  autop = auto_pickup_check(o_ptr, FALSE);
  
  /* No missiles to combine with and no autopickup. */
  if (!ammo_num && !autop) return (FALSE);
  
  /* Effective added count */
  added_ammo_num = (throwing ?
		    o_ptr->number * THROWER_AMMO_FACTOR : o_ptr->number);
  
  /* How many quiver slots would be needed */
  attempted_quiver_slots = ((ammo_num + added_ammo_num + 98) / 99);
  
  /* Is there room, given normal inventory? */
  if (attempted_quiver_slots + p_ptr->inven_cnt > INVEN_PACK)
    {
      return (FALSE);
    }
  
  
  /* Check quiver for similar objects or empty space. */
  for (i = INVEN_Q0; i <= INVEN_Q9; i++)
    {
      /* Assume no carry */
      bool flag = FALSE;
      
      /* Get object in that slot. */
      i_ptr = &inventory[i];
      
      /* Allow auto-pickup to empty slots */
      if ((!i_ptr->k_idx) && (autop))
	{
	  /* Nothing there */
	  old_num = 0;
	  
	  /* Wield it */
	  object_copy(i_ptr, o_ptr);
	  
	  flag = TRUE;
	}
      
      /* Look for similar */
      else if (object_similar(i_ptr, o_ptr))
	{
	  /* How many did we have before? */
	  old_num = i_ptr->number;
	  
	  /* Don't absorb unless there is space for all of it */
	  if ((old_num + o_ptr->number) > 99) return (FALSE);
	  
	  /* Absorb floor object. */
	  object_absorb(i_ptr, o_ptr);
	  
	  flag = TRUE;
	}
      
      /* We want to carry it */
      if (flag)
	{
	  char o_name[120];
	  
	  /* Increase carried weight */
	  p_ptr->total_weight += i_ptr->weight * (i_ptr->number - old_num);
	  
	  /* Get the object again */
	  o_ptr = &inventory[i];
	  
	  /* Describe the object */
	  if (blind) object_desc(o_name, o_ptr, TRUE, 0);
	  else       object_desc(o_name, o_ptr, TRUE, 3);
	  
	  /* Message */
	  msg_format("You have %s (%c).", o_name, index_to_label(i));
	  
	  /* Delete the object */
	  delete_object_idx(o_idx);
	  
	  /* Recalculate quiver size */
	  find_quiver_size();
	  
	  /* Recalculate bonuses */
	  p_ptr->update |= (PU_BONUS);
	  
	  /* Reorder the quiver */
	  p_ptr->notice |= (PN_COMBINE);
	  
	  /* Window stuff */
	  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
	  
	  /* Redraw equippy chars */
	  p_ptr->redraw |= (PR_EQUIPPY);
	  
	  return (TRUE);
	}
    }

  /* Didn't find a slot with similar objects, or an empty slot. */
  return (FALSE);
}


/**
 * Return TRUE if the given object can be automatically picked up
 */
static bool auto_pickup_okay(object_type *o_ptr)
{
  cptr s;
  int j;

  /* Can't pick up if no room */
  if (!inven_carry_okay(o_ptr)) return FALSE;

  /* Pickup things that match the inventory */
  if (pickup_inven) 
    for (j = 0; j < INVEN_PACK - p_ptr->pack_size_reduce; j++)
      {
	object_type *j_ptr = &inventory[j];
	
	/* Skip non-objects */
	if (!j_ptr->k_idx) continue;
	
	/* Check if the two items can be combined */
	if (object_similar(j_ptr, o_ptr)) return (TRUE);
      }
  
  /* No inscription */
  if (!o_ptr->note)
    return (FALSE);
  
  /* Find a '=' */
  s = strchr(quark_str(o_ptr->note), '=');
  
  /* Process preventions */
  while (s)
    {
      /* =g ('g'et) means auto pickup */
      if (s[1] == 'g') return (TRUE);
      
      /* Find another '=' */
      s = strchr(s + 1, '=');
    }
  
  /* Don't auto pickup */
  return (FALSE);
}


/**
 * Carry an object and delete it.
 */
extern void py_pickup_aux(int o_idx)
{
  int slot;
  
  char o_name[120];
  object_type *o_ptr;
  object_type *i_ptr = &inventory[INVEN_LITE];
  
  o_ptr = &o_list[o_idx];
  
  /* Carry the object */
  slot = inven_carry(o_ptr);
  
  /* Get the object again */
  o_ptr = &inventory[slot];
  
  /* Set squelch status */
  p_ptr->notice |= PN_SQUELCH;

  /* Hack - set the turn found for artifacts */
  if (o_ptr->name1)
    {
      if (a_info[o_ptr->name1].creat_turn < 2)
	{
	  a_info[o_ptr->name1].creat_turn = turn;
	  a_info[o_ptr->name1].p_level = p_ptr->lev;
	}
    }

  /* Stone of Lore gives id on pickup */
  if (!object_known_p(o_ptr)) 
    {
      if (i_ptr->sval == SV_STONE_LORE) 
	identify_object(o_ptr);

      /* Otherwise pseudo-ID */
      else
	{
	  bool heavy = FALSE;
	  int feel;
	  
	  /* Heavy sensing */
	  heavy = (check_ability(SP_PSEUDO_ID_HEAVY));
	  
	  /* Type of feeling */
	  feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
	  
	  /* We have "felt" it */
	  o_ptr->ident |= (IDENT_SENSE);
	  
	  /* Inscribe it textually */
	  o_ptr->feel = feel;
	  
	  /* Set squelch flag as appropriate */
	  p_ptr->notice |= PN_SQUELCH;
	}
    }

  /* Notice dice and other obvious stuff */
  notice_other(IF_DD_DS, slot + 1);
  o_ptr->id_obj |= ((o_ptr->flags_obj) & OF_OBVIOUS_MASK);

  /* Average things are average */
  if ((o_ptr->feel == FEEL_AVERAGE) && (is_weapon(o_ptr) || is_armour(o_ptr)))
    notice_other((IF_AC | IF_TO_A | IF_TO_H | IF_TO_D), slot + 1);

  /* Recalculate the bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Window stuff */
  p_ptr->window |= (PW_EQUIP | PW_INVEN | PW_ITEMLIST);

  /* Describe the object */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Message */
  msg_format("You have %s (%c).", o_name, index_to_label(slot));
  
  /* Delete the object */
  delete_object_idx(o_idx);
}


/**
 * Pick up objects and treasure on the floor, now also used for telekinesis.
 *
 * Called with pickup:
 * 0 to grab gold and describe non-gold objects.
 * 1 to pick up objects either with or without displaying a menu.
 * 2 to pick up objects, forcing a menu for multiple objects.
 * 3 to pick up objects, forcing a menu for any number of objects.
 *
 * Scan the list of objects in that floor grid.   Pick up gold automatically.
 * Pick up objects automatically until pile or backpack space is full if 
 * auto-pickup option is on, carry_query_floor option is not, and menus are 
 * not forced (which the "get" command does). Otherwise, store objects on 
 * floor in an array, and tally both how many there are and can be picked up.
 *
 * If the player is not picking up objects, describe a single object or 
 * indicate the presence of a floor stack.  If player is picking up objects, 
 * name a single object, or indicate a stack of objects, that cannot go in 
 * the backpack.
 *
 * Pick up a single object without menus, unless menus for single items are 
 * forced.  Confirm pickup if that option is on.
 *
 * Pick up multiple objects (unless using autopickup, no confirm) using Tim
 * Baker's menu system.   Recursively call this function (forcing menus for any 
 * number of objects) until objects are gone, backpack is full, or player is 
 * satisfied.
 *
 * Keep track of number of objects picked up (to calculate time spent).
 */
byte py_pickup(int pickup, int y, int x)
{
  s16b this_o_idx, next_o_idx = 0;
  
  char o_name[120];
  object_type *o_ptr;
  
  /* Objects picked up.  Used to determine time cost of command. */
  byte objs_picked_up = 0;
  
  int floor_num = 0, floor_list[24], floor_o_idx = 0;
  
  int can_pickup = 0;
  bool call_function_again = FALSE;
  bool do_ask = TRUE;
  bool telekinesis;
  
  /* Set telekinesis flag */
  telekinesis = (!(y == p_ptr->py) || !(x == p_ptr->px));

  /* Scan the pile of objects */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      bool menu_flag = FALSE;

      /* Access the object */
      o_ptr = &o_list[this_o_idx];
      
      /* Describe the object */
      object_desc(o_name, o_ptr, TRUE, 3);
      
      /* Access the next object */
      next_o_idx = o_ptr->next_o_idx;

      /* Ordinary pickup */
      if (!telekinesis)
      {
      
	/* Ignore all hidden objects and non-objects */
	if (squelch_hide_item(o_ptr) || !o_ptr->k_idx) continue;

	/* Hack -- disturb */
	disturb(0, 0);
	
	/* Pick up gold */
	if (o_ptr->tval == TV_GOLD)
	  {
	    /* Message */
	    msg_format("You have found %ld gold pieces worth of %s.",
		       (long)o_ptr->pval, o_name);
	    
	    /* Collect the gold */
	    p_ptr->au += o_ptr->pval;

	    /* Limit to avoid buffer overflow */
	    if (p_ptr->au > PY_MAX_GOLD) p_ptr->au = PY_MAX_GOLD;
	    
	    /* Redraw gold */
	    p_ptr->redraw |= (PR_GOLD);
	    
	    /* Window stuff */
	    p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	    
	    /* Delete the gold */
	    delete_object_idx(this_o_idx);
	    
	    /* Check the next object */
	    continue;
	  }
	
	/* Automatically pick up objects like those in the quiver. */
	if (quiver_carry(o_ptr, this_o_idx)) continue;
	
	/* Automatically pick up some items */
	if (inven_carry_okay(o_ptr) && auto_pickup_okay(o_ptr))
	  {
	    /* Pick up the object */
	    py_pickup_aux(this_o_idx);
	    
	    /* Check the next object */
	    continue;
	  }
      	
	/* Pick up objects automatically, without menus, if pickup is 
	 * allowed and menus are not forced, auto-pickup is on, and 
	 * query_floor is off.
	 */
	else if ((pickup == 1) && (always_pickup) && (!carry_query_flag))
	  {
	    /* If backpack is not full, carry the item. */
	    if (inven_carry_okay(o_ptr))
	      {
		/* Pick up the object */
		py_pickup_aux(this_o_idx);
	      }
	
	    /* Otherwise, add one to the number of floor objects. */
	    else
	      {
		/* Count non-gold objects that remain on the floor. */
		floor_num++;
		
		/* Remember this index */
		floor_o_idx = this_o_idx;
	      }
	    
	  }
	/* Otherwise, activate the menu system.	 */
	else menu_flag = TRUE;
      }
      
      /* Tally objects and store them in an array. */
      if (telekinesis || menu_flag)
	{
	  /* Remember this object index */
	  floor_list[floor_num] = this_o_idx;
	  
	  /* Count non-gold objects that remain on the floor. */
	  floor_num++;
	  
	  /* Remember this index */
	  floor_o_idx = this_o_idx;
	  
	  /* Tally objects that can be picked up.*/
	  if (inven_carry_okay(o_ptr))
	    {
	      can_pickup++;
	    }
	}
    }
  
  /* There are no non-gold objects */
  if (!floor_num) return (objs_picked_up);
  
  
  /* Mention the objects or stack, if player is not picking them up. */
  if (!pickup)
    {
      /* One object */
      if (floor_num == 1)
	{
	  /* Access the object */
	  o_ptr = &o_list[floor_o_idx];
	  
	  /* Describe the object.   Less detail if blind. */
	  if (p_ptr->blind) object_desc(o_name, o_ptr, TRUE, 0);
	  else object_desc(o_name, o_ptr, TRUE, 3);
	  
	  /* Message */
	  msg_format("You %s %s.", (p_ptr->blind ? "feel" : "see"), o_name);
	}
      
      /* Multiple objects */
      else
	{
	  /* Message */
	  msg_format("You %s a pile of %d items.", 
		     (p_ptr->blind ? "feel" : "see"), floor_num);
	}
      
      /* Done */
      return (objs_picked_up);
    }
  
  /* The player has no room for anything on the floor. */
  else if (!(can_pickup || telekinesis))
    {
      /* One object */
      if (floor_num == 1)
	{
	  /* Access the object */
	  o_ptr = &o_list[floor_o_idx];
	  
	  /* Describe the object.   Less detail if blind. */
	  if (p_ptr->blind) object_desc(o_name, o_ptr, TRUE, 0);
	  else object_desc(o_name, o_ptr, TRUE, 3);
	  
	  /* Message */
	  msg_format("You have no room for %s.", o_name);
	}
      
      /* Multiple objects */
      else
	{
	  /* Message */
	  msg_print("You have no room for any of the objects on the floor.");
	}
      
      /* Done */
      return (objs_picked_up);
    }
  
  /* Simple display of a single object, if menus for single objects are 
   * not forced.
   */
  else if ((floor_num == 1) && (pickup != 3) && (!telekinesis))
    {
      /* Hack -- query every object */
      if (carry_query_flag)
	{
	  char out_val[160];
	  int answer = 0;
	  
	  /* Access the object */
	  o_ptr = &o_list[floor_o_idx];
	  
	  /* Describe the object.   Less detail if blind. */
	  if (p_ptr->blind) object_desc(o_name, o_ptr, TRUE, 0);
	  else object_desc(o_name, o_ptr, TRUE, 3);
	  
	  /* Build a prompt */
	  (void) sprintf(out_val, "Pick up %s? ", o_name);
	  
	  /* Ask the user to confirm */
	  answer = get_check_other(out_val, (rogue_like_commands 
					     ? KTRL('D') : 'k'));
	  if (answer == 0)
	    {
	      /* Done */
	      return (objs_picked_up);
	    }
	  else if (answer == 2)
	    {
	      p_ptr->command_item = -floor_o_idx;
	      do_cmd_destroy();
	      return (objs_picked_up);
	    }
	  /* Otherwise continue */
	}
      
      /* Don't ask */
      do_ask = FALSE;
      
      /* Remember the object to pick up */
      this_o_idx = floor_o_idx;
    }
  
  /* Display a list if no other query took precedence. */
  else
    {
      cptr q, s;
      
      int item;
      
      /* Get an object or exit. */
      q = "Get which item? ";
      s = "You see nothing there.";

      /* Telekinesis */
      if (telekinesis)
	{
	  /* Don't restrict the choices */
	  item_tester_hook = NULL;
      
	  if (get_item(&item, q, s, (USE_TARGET)))
	    {
	      this_o_idx = 0 - item;
	    }
	  else
	    {
	      return (objs_picked_up);
	    }
	}
      /* Ordinary pickup */
      else
	{
	  /* Restrict the choices */
	  item_tester_hook = inven_carry_okay;
      
	  if (get_item(&item, q, s, (USE_FLOOR)))
	    {
	      this_o_idx = 0 - item;
	      call_function_again = TRUE;
	    }
	  else
	    {
	      return (objs_picked_up);
	    }
	}
    }
  
  /* Into quiver via telekinesis if possible */
  if (telekinesis)
    {
      /* Access the object */
      o_ptr = &o_list[this_o_idx];

      if (quiver_carry(o_ptr, this_o_idx)) return (1);
    }

  /* Regular pickup or telekinesis with pack not full */
  if (can_pickup)
    {
      /* Pick up the object */
      py_pickup_aux(this_o_idx);
    }
  /* Telekinesis with pack full */
  else
    {
      /* Access the object */
      o_ptr = &o_list[this_o_idx];

      /* Drop it */
      drop_near(o_ptr, -1, p_ptr->py, p_ptr->px);

      /* Delete the old object */
      delete_object_idx(this_o_idx);
    }
  
  /* Indicate an object picked up. */
  objs_picked_up = 1;
  
  /* If requested, call this function recursively.  Count objects picked 
   * up.  Force the display of a menu in all cases.
   */
  if (call_function_again) objs_picked_up += py_pickup(3, y, x);
  
  /* Indicate how many objects have been picked up. */
  return (objs_picked_up);
}


/**
 * Determine if a trap affects the player.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_trap_hit(int power)
{
  int k, ac;
  bool hit = FALSE;
  
  /* This function is called from the trap attack code, which generally
   * uses the simple RNG.   We temporarily switch over to the complex 
   * RNG for true randomness. - LM-
   */
  Rand_quick = FALSE;
  
  /* Percentile dice */
  k = rand_int(100);
  
  /* 5% minimum chance to hit, 5% minimum chance to miss */
  if (k < 10) hit = (k < 5);
  
  /* Total armor */
  ac = p_ptr->ac + p_ptr->to_a;
  
  /* Power competes against Armor */
  if ((power > 0) && (randint(power) >= (ac * 3 / 4))) hit = TRUE;
  
  /* Resume use of the simple RNG. */
  Rand_quick = TRUE;
  
  /* Return hit or miss. */
  return (hit);
  
}



/**
 * Handle player hitting a real trap.  Rewritten in Oangband to allow a 
 * greater variety of traps, with effects controlled by dungeon level.  
 * To allow a trap to choose one of a variety of effects consistantly, 
 * the quick RNG is often used, and xy coordinates input as a seed value. 
 */
void hit_trap(int y, int x)
{
  int i, j, k, num;
  int dam = 0;
  
  int nastyness, selection;

  feature_type *f_ptr = &f_info[cave_feat[y][x]];
  
  cptr name = f_name + f_ptr->name;
  
  /* Use the "simple" RNG to insure that traps are consistant. */
  Rand_quick = TRUE;
  
  /* Use the coordinates of the trap to seed the RNG. */
  Rand_value = y * x;
  
  /* Disturb the player */
  disturb(0, 0);
  
  /* Analyze XXX XXX XXX */
  switch (cave_feat[y][x])
    {
      /* trap door. */
    case FEAT_TRAP_HEAD + 0x00:
      {
	Rand_quick = FALSE;
	
	/* Paranoia  -NRM- */
	if (((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) ||
	     (stage_map[p_ptr->stage][STAGE_TYPE] == VALLEY)) &&
	    (!stage_map[p_ptr->stage][DOWN]))
	  {
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_FLOOR);
	    msg_print("The trap fails!");
	    break;
	  }
	
	
	msg_print("You fall through a trap door!");
	if (p_ptr->ffall)
	  {
	    notice_obj(OF_FEATHER, 0);
	    msg_print("You float gently down to the next level.");
	  }
	else
	  {
	    dam = damroll(2, 8);
	    take_hit(dam, name);
	  }
	/* Remember where we came from */
	p_ptr->last_stage = p_ptr->stage;

	if (!stage_map[p_ptr->stage][DOWN])
	  {
	    /* Set the ways forward and back */
	    stage_map[255][UP] = p_ptr->stage;
	    stage_map[p_ptr->stage][DOWN] = 255;
	    stage_map[255][DEPTH] = p_ptr->depth + 1;
	  }
	  
	/* New stage */
	p_ptr->stage = stage_map[p_ptr->stage][DOWN];
	
	/* New depth */
	p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	/* Leaving */
	p_ptr->leaving = TRUE;
	
	Rand_quick = TRUE;
	
	break;
      }
      
      /* pits. */
    case FEAT_TRAP_HEAD + 0x01:
      {
	/* determine how dangerous the trap is allowed to be. */
	nastyness = randint(p_ptr->depth);
	if (randint(20) == 1) nastyness += 20;
	else if (randint(5) == 1) nastyness += 10;
	
	/* Player is now in pit. */
	monster_swap(p_ptr->py, p_ptr->px, y, x);
	
	/* Center on player. */
	y = p_ptr->py;
	x = p_ptr->px;
	
	/* pit of daggers. */
	if ((nastyness > 80) && (randint(3) != 3))
	  {
	    msg_print("You fall into a pit of daggers!");
	    
	    if (p_ptr->ffall)
	      {
		notice_obj(OF_FEATHER, 0);
		msg_print("You float gently to the floor of the pit.");
		msg_print("You carefully avoid setting off the daggers.");
	      }
	    
	    else
	      {
		/* a trap of morgul. */
		if (randint(6) == 1)
		  {
		    Rand_quick = FALSE;
		    
		    
		    msg_print("A single coldly gleaming dagger pierces you deeply!");
		    msg_print("You feel a deadly chill slowly withering your soul.");
		    
		    /* activate the Black Breath. */
		    p_ptr->black_breath = TRUE;
		    
		    /* lots of damage. */
		    dam = damroll(20,15);
		    
		    /* undead may be attracted. */
		    if (randint(2) == 1)
		      {
			msg_print("Undead suddenly appear and call you to them!");
			
			k = randint(3) + 2;
			for (i = 0; i < k; i++)
			  {
			    summon_specific(y, x, FALSE, p_ptr->depth, SUMMON_UNDEAD);
			  }
		      }
		    
		    /* morgul-traps are one-time only. */
		    cave_info[y][x] &= ~(CAVE_MARK);
		    cave_set_feat(y, x, FEAT_FLOOR);
		    
		    Rand_quick = TRUE;
		  }
		
		else
		  {
		    Rand_quick = FALSE;
		    
		    /* activate the ordinary daggers. */
		    msg_print("Daggers pierce you everywhere!");
		    
		    k = randint(10) + 5;
		    for (i = 0; i < k; i++)
		      {
			dam += damroll(3, 4);
		      }
		    
		    Rand_quick = TRUE;
		  }
		
		/* cut the player. */
		(void)set_cut(p_ptr->cut + randint(dam));
		
		/* Take the damage. */
		take_hit(dam, name);
	      }
	  }
	
	/* poisoned spiked pit. */
	else if ((nastyness > 55) && (randint(3) != 3))
	  {
	    msg_print("You fall into a spiked pit!");
	    
	    if (p_ptr->ffall)
	      {
		notice_obj(OF_FEATHER, 0);
		msg_print("You float gently to the floor of the pit.");
		msg_print("You carefully avoid touching the spikes.");
	      }
	    
	    else
	      {
		Rand_quick = FALSE;
		
		/* Base damage */
		dam = damroll(2, 6);
		
		/* Extra spike damage */
		if (rand_int(100) < 85)
		  {
		    bool was_poisoned;
		    
		    msg_print("You are impaled on poisonous spikes!");
		    
		    dam = dam * (randint(6) + 3);
		    (void)set_cut(p_ptr->cut + randint(dam));
		    
		    was_poisoned = pois_hit(dam);
		    
		    if (!was_poisoned) 
		      msg_print("The poison does not affect you!");
		  }
		
		/* Take the damage */
		take_hit(dam, name);
		
		Rand_quick = TRUE;
	      }
	  }
	
	/* spiked pit. */
	else if ((nastyness > 30) && (randint(3) != 3))
	  {
	    msg_print("You fall into a spiked pit!");
	    
	    if (p_ptr->ffall)
	      {
		notice_obj(OF_FEATHER, 0);
		msg_print("You float gently to the floor of the pit.");
		msg_print("You carefully avoid touching the spikes.");
	      }
	    
	    else
	      {
		Rand_quick = FALSE;
		
		/* Base damage */
		dam = damroll(2, 6);
		
		/* Extra spike damage */
		if (rand_int(100) < 85)
		  {
		    msg_print("You are impaled!");
		    
		    dam = dam * (2 + randint(4));
		    (void)set_cut(p_ptr->cut + randint(dam));
		  }
		
		/* Take the damage */
		take_hit(dam, name);
		
		Rand_quick = TRUE;
	      }
	  }
	
	/* ordinary pit in all other cases. */
	else
	  {
	    msg_print("You fall into a pit!");
	    if (p_ptr->ffall)
	      {
		notice_obj(OF_FEATHER, 0);
		msg_print("You float gently to the bottom of the pit.");
	      }
	    else
	      {
		Rand_quick = FALSE;
		
		dam = damroll(2, 6);
		take_hit(dam, name);
		
		Rand_quick = TRUE;
	      }
	  }
	
	break;
      }
      
      /* stat-reducing dart traps. */
    case FEAT_TRAP_HEAD + 0x02:
      {
	/* decide if the dart hits. */
	if (check_trap_hit(50 + p_ptr->depth))
	  {
	    /* select a stat to drain. */
	    selection = rand_int(6);
	    
	    Rand_quick = FALSE;
	    
	    msg_print("A small dart hits you!");
	    dam = damroll(1, 4);
	    take_hit(dam, name);
	    
	    /* Determine how dangerous the trap is allowed to be. */
	    nastyness = randint(p_ptr->depth);
	    
	    /* decide how much to drain the stat by. */
	    if ((nastyness > 50) && (randint(3) == 1))
	      {
		num = randint(4);
	      }
	    else num = 1;
	    
	    /* drain the stat. */
	    for (i = 0; i < num; i++)
	      {
		(void)do_dec_stat(selection);
	      }
	    
	    Rand_quick = TRUE;
	  }
	else
	  {
	    msg_print("A small dart barely misses you.");
	  }
	break;
      }
      
      /* discolored spots. */
    case FEAT_TRAP_HEAD + 0x03:
      {
	/* determine how dangerous the trap is allowed to be. */
	nastyness = randint(p_ptr->depth);
	if (randint(5) == 1) nastyness += 10;
	
	/* pick a elemental attack type. */
	selection = randint(4);
	
	
	/* electicity trap. */
	if (selection == 1)
	  {
	    if ((nastyness >= 50) && (randint(2) == 1))
	      {
		Rand_quick = FALSE;
		
		msg_print("You are struck by lightning!");
		dam = damroll(6, 30);
		
		Rand_quick = TRUE;
	      }
	    else
	      {
		Rand_quick = FALSE;
		
		msg_print("You get zapped!");
		dam = damroll(4, 8);
		
		Rand_quick = TRUE;
	      }
	    Rand_quick = FALSE;
	    elec_dam(dam, "an electricity trap");
	    Rand_quick = TRUE;
	    
	  }
	
	/* frost trap. */
	if (selection == 2)
	  {
	    if ((nastyness >= 50) && (randint(2) == 1))
	      {
		Rand_quick = FALSE;
		
		msg_print("You are lost within a blizzard!");
		dam = damroll(6, 30);
		
		Rand_quick = TRUE;
	      }
	    else
	      {
		Rand_quick = FALSE;
		
		msg_print("You are coated in frost!");
		dam = damroll(4, 8);
		
		Rand_quick = TRUE;
	      }
	    Rand_quick = FALSE;
	    cold_dam(dam, "a frost trap");
	    Rand_quick = TRUE;
	  }
	
	/* fire trap. */
	if (selection == 3)
	  {
	    if ((nastyness >= 50) && (randint(2) == 1))
	      {
		Rand_quick = FALSE;
		
		msg_print("You are enveloped in a column of fire!");
		dam = damroll(6, 30);
		
		Rand_quick = TRUE;
	      }
	    else
	      {
		Rand_quick = FALSE;
		
		msg_print("You are surrounded by flames!");
		dam = damroll(4, 8);
		
		Rand_quick = TRUE;
	      }
	    Rand_quick = FALSE;
	    fire_dam(dam, "a fire trap");
	    Rand_quick = TRUE;
	  }
	
	/* acid trap. */
	if (selection == 4)
	  {
	    if ((nastyness >= 50) && (randint(2) == 1))
	      {
		Rand_quick = FALSE;
		
		msg_print("A cauldron of acid is tipped over your head!");
		dam = damroll(6, 30);
		
		Rand_quick = TRUE;
	      }
	    else
	      {
		Rand_quick = FALSE;
		
		msg_print("You are splashed with acid!");
		dam = damroll(4, 8);
		
		Rand_quick = TRUE;
	      }
	    Rand_quick = FALSE;
	    acid_dam(dam, "an acid trap");
	    Rand_quick = TRUE;
	  }
	
	break;
      }
      
      /* gas traps. */
    case FEAT_TRAP_HEAD + 0x04:
      {
	selection = randint(4);
	
	/* blinding trap. */
	if (selection == 1)
	  {
	    msg_print("You are surrounded by a black gas!");
	    if (!p_ptr->no_blind)
	      {
		Rand_quick = FALSE;
		
		(void)set_blind(p_ptr->blind + rand_int(30) + 15);
		
		Rand_quick = TRUE;
	      }
	  }
	else notice_obj(OF_SEEING, 0);
	
	/* confusing trap. */
	if (selection == 2)
	  {
	    msg_print("You are surrounded by a gas of scintillating colors!");
	    if (!p_resist_good(P_RES_CONFU))
	      {
		Rand_quick = FALSE;
		
		(void)set_confused(p_ptr->confused + rand_int(20) + 10);
		
		Rand_quick = TRUE;
	      }
	    else notice_other(IF_RES_CONFU, 0);
	  }
	
	/* poisoning trap. */
	if (selection == 3)
	  {
	    msg_print("You are surrounded by a pungent green gas!");
	    
	    Rand_quick = FALSE;
	    
	    pois_hit(25);
	    
	    Rand_quick = TRUE;
	  }
	
	/* sleeping trap. */
	if (selection == 4)
	  {
	    msg_print("You are surrounded by a strange white mist!");
	    if (!p_ptr->free_act)
	      {
		(void)set_paralyzed(p_ptr->paralyzed + rand_int(10) + 5);
	      }
	    else notice_obj(OF_FREE_ACT, 0);
	  }
	
	break;
      }
      
      /* summoning traps. */
    case FEAT_TRAP_HEAD + 0x05:
      {
	sound(MSG_SUM_MONSTER);
	/* sometimes summon thieves. */
	if ((p_ptr->depth > 8) && (randint(5) == 1))
	  {
	    msg_print("You have aroused a den of thieves!");
	    
	    Rand_quick = FALSE;
	    
	    num = 2 + randint(3);
	    for (i = 0; i < num; i++)
	      {
		(void)summon_specific(y, x, FALSE, p_ptr->depth, SUMMON_THIEF);
	      }
	    
	    Rand_quick = TRUE;
	  }
	
	/* sometimes summon a nasty unique. */
	else if (randint(8) == 1)
	  {
	    msg_print("You are enveloped in a cloud of smoke!");
	    
	    Rand_quick = FALSE;
	    
	    (void)summon_specific(y, x, FALSE, p_ptr->depth + 5, 
				  SUMMON_UNIQUE);
	    
	    Rand_quick = TRUE;
	  }
	
	/* otherwise, the ordinary summon monsters. */
	else 
	  {
	    msg_print("You are enveloped in a cloud of smoke!");
	    
	    Rand_quick = FALSE;
	    
	    num = 2 + randint(3);
	    for (i = 0; i < num; i++)
	      {
		(void)summon_specific(y, x, FALSE, p_ptr->depth, 0);
	      }
	    
	    Rand_quick = TRUE;
	  }
	
	/* these are all one-time traps. */
	cave_info[y][x] &= ~(CAVE_MARK);
	cave_set_feat(y, x, FEAT_FLOOR);
	
	break;
      }
      
      /* dungeon alteration traps. */
    case FEAT_TRAP_HEAD + 0x06:
      {
	/* determine how dangerous the trap is allowed to be. */
	nastyness = randint(p_ptr->depth);
	if (randint(5) == 1) nastyness += 10;
	
	/* make room for alterations. */
	cave_info[y][x] &= ~(CAVE_MARK);
	cave_set_feat(y, x, FEAT_FLOOR);
	
	/* Everything truely random from here on. */
	Rand_quick = FALSE;
	
	/* dungeon destruction trap. */
	if ((nastyness > 60) && (randint(12) == 1))
	  {
	    msg_print("A ear-splitting howl shatters your mind as the dungeon is smashed by hammer blows!");
	    
	    (void)destroy_level(FALSE);
	    
	    /* the player is hard-hit. */
	    (void)set_confused(p_ptr->confused + rand_int(20) + 10);
	    (void)set_blind(p_ptr->blind + rand_int(30) + 15);
	    (void)set_stun(p_ptr->stun + randint(50) + 50);
	    dam = damroll(15,15);
	    take_hit(dam, name);
	  }
	
	/* earthquake trap. */
	else if ((nastyness > 20) && (randint(4) == 1))
	  {
	    msg_print("A tremor shakes the earth around you");
	    earthquake(y, x, 10, FALSE);
	  }
	
	/* falling rock trap. */
	else if ((nastyness > 4) && (randint(2) == 1))
	  {
	    msg_print("A rock falls on your head.");
	    dam = damroll(2,10);
	    take_hit(dam, name);
	    
	    (void)set_stun(p_ptr->stun + randint(10) + 10);
	  }
	
	/* a few pebbles. */
	else
	  {
	    msg_print("A bunch of pebbles rain down on you.");
	    dam = damroll(1,8);
	    take_hit(dam, name);
	  }
	
	Rand_quick = TRUE;
	
	break;
      }
      
      /* various char and equipment-alteration traps, lumped together 
       * to avoid any one effect being too common (some of them can be 
       * rather nasty).
       */
    case FEAT_TRAP_HEAD + 0x07:
      {
	/* determine how dangerous the trap is allowed to be. */
	nastyness = rand_int(100);
	
	/* these are all one-time traps. */
	cave_info[y][x] &= ~(CAVE_MARK);
	cave_set_feat(y, x, FEAT_FLOOR);
	
	/* Everything truely random from here on. */
	Rand_quick = FALSE;
	
	/* trap of drain wands. */
	if (nastyness < 15)
	  {
	    /* Hold the object information. */
	    object_type *o_ptr;
	    
	    /* Find an item */
	    for (i = 0; i < 20; i++)
	      {
		/* Pick an item */
		i = rand_int(INVEN_PACK - p_ptr->pack_size_reduce);
		
		/* Obtain the item */
		o_ptr = &inventory[i];
		
		/* use "num" to decide if a item can be
		 * uncharged.  By default, assume it can't. */
		num = 0;
		
		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;
		
		/* Drain charged wands/staffs/rods */
		if ((o_ptr->tval == TV_STAFF) ||
		    (o_ptr->tval == TV_WAND) ||
		    (o_ptr->tval == TV_ROD))
		  {
		    /* case of charged wands/staffs. */
		    if (((o_ptr->tval == TV_STAFF) ||
			 (o_ptr->tval == TV_WAND)) &&
			(o_ptr->pval)) num = 1;
		    
		    /* case of charged rods. */
		    if ((o_ptr->tval == TV_ROD) &&
			(o_ptr->timeout < (o_ptr->pval * o_ptr->number))) 
		      num = 1;
		    
		    
		    if (num == 1)
		      {
			/* Message */
			msg_print("Energy drains from your pack!");
			
			/* Uncharge */
			if ((o_ptr->tval == TV_STAFF) ||
			    (o_ptr->tval == TV_WAND)) 
			  o_ptr->pval = 0;
			
			if (o_ptr->tval == TV_ROD) 
			  o_ptr->timeout = o_ptr->pval * o_ptr->number * 2;
			
			
			/* Combine / Reorder the pack */
			p_ptr->notice |= (PN_COMBINE | 
					  PN_REORDER);
			
			/* Window stuff */
			p_ptr->window |= (PW_INVEN);
			
			/* not more than one inventory 
			 * slot effected. */
			break;
		      }
		    else continue;
		  }
	      }
	  }
	
	/* trap of forgetting. */
	else if (nastyness < 35)
	  {
	    if (check_save(100))
	      {
		msg_print("You hang on to your memories!");
	      }
	    else if (lose_all_info())
	      {
		msg_print("Your memories fade away.");
	      }
	  }
	
	/* trap of alter reality. */
	else if (nastyness < 50)
	  {
	    msg_print("The world changes!");
	    
	    /* Leaving */
	    p_ptr->leaving = TRUE;
	  }
	
	/* trap of remold player. */
	else if (nastyness < 75)
	  {
	    int max1, cur1, max2, cur2, ii, jj;
	    
	    msg_print("You feel yourself being twisted by wild magic!");
	    
	    if (check_save(100))
	      {
		msg_print("You resist the effects!");
	      }
	    else
	      {
		msg_print("Your body starts to scramble...");
		
		/* Pick a pair of stats */
		ii = rand_int(6);
		for (jj = ii; jj == ii; jj = rand_int(6)) /* loop */;
		
		max1 = p_ptr->stat_max[ii];
		cur1 = p_ptr->stat_cur[ii];
		max2 = p_ptr->stat_max[jj];
		cur2 = p_ptr->stat_cur[jj];
		
		p_ptr->stat_max[ii] = max2;
		p_ptr->stat_cur[ii] = cur2;
		p_ptr->stat_max[jj] = max1;
		p_ptr->stat_cur[jj] = cur1;
		
		p_ptr->update |= (PU_BONUS);
	      }
	  }
	
	/* time ball trap. */
	else if (nastyness < 90)
	  {
	    msg_print("You feel time itself assault you!");
	    
	    /* Target the player with a radius 0 ball attack. */
	    fire_meteor(0, GF_TIME, p_ptr->py, p_ptr->px, 
			75, 0, TRUE);
	  }
	
	/* trap of bugs gone berserk. */
	else
	  {
	    /* explain what the dickens is going on. */
	    msg_print("GRUESOME Gnawing Bugs leap out at you!");
	    
	    if (!p_resist_good(P_RES_CONFU))
	      {
		(void)set_confused(p_ptr->confused + rand_int(20) + 10);
	      }
	    else notice_other(IF_RES_CONFU, 0);

	    if (!p_resist_good(P_RES_CHAOS))
	      {
		(void)set_image(p_ptr->image + randint(40));
	      }
	    else notice_other(IF_RES_CHAOS, 0);
	    
	    /* XXX (hard coded) summon 3-6 bugs. */
	    k = randint(4) + 2;
	    for (i = 0; i < k; ++i)
	      {
		/* Look for a location */
		for (j = 0; j < 20; ++j)
		  {
		    /* Pick a (scattered) distance. */
		    int d = (j / 10) + randint(3);
		    
		    /* Pick a location */
		    scatter(&y, &x, y, x, d, 0);
		    
		    /* Require passable terrain */
		    if (!cave_passable_bold(y, x)) continue;
		    
		    /* Hack -- no summon on glyph of warding */
		    if (cave_feat[y][x] == FEAT_RUNE_PROTECT) continue;
		    
		    /* Okay */
		    break;
		  }
		
		/* Attempt to place the awake bug */
		place_monster_aux(y, x, 453, FALSE, TRUE);
	      }
	    
	    /* herald the arrival of bugs. */
	    msg_print("AAAAAAAHHHH! THEY'RE EVERYWHERE!");
	  }
	
	Rand_quick = TRUE;
	
	break;
      }
      
      /* teleport trap */
    case FEAT_TRAP_HEAD + 0x08:
      {
	if (stage_map[p_ptr->stage][STAGE_TYPE] >= CAVE)
	  msg_print("You teleport across the dungeon.");
	else
	  msg_print("You teleport across the wilderness.");
	
	Rand_quick = FALSE;
	
	teleport_player(250, FALSE);
	
	Rand_quick = TRUE;
	
	break;
      }
      
      /* murder holes. */
    case FEAT_TRAP_HEAD + 0x09:
      {
	/* hold the object info. */
	object_type *o_ptr;
	object_type object_type_body;
	
	/* hold the missile type and name. */
	int sval = 0;
	int tval = 0;
	cptr missile_name = "";

	
	
	/* Determine the missile type and base damage. */
	if (randint(3) == 1)
	  {
	    if (p_ptr->depth < 40)
	      {
		missile_name = "shot";
		dam = damroll(2,3);
		tval = TV_SHOT;
		sval = SV_AMMO_NORMAL;
	      }
	    else
	      {
		missile_name = "seeker shot";
		dam = damroll(3,7);
		tval = TV_SHOT;
		sval = SV_AMMO_HEAVY;
	      }
	  }
	
	else if (randint(2) == 1)
	  {
	    if (p_ptr->depth < 55)
	      {
		missile_name = "arrow";
		dam = damroll(2,4);
		tval = TV_ARROW;
		sval = SV_AMMO_NORMAL;
	      }
	    else
	      {
		missile_name = "seeker arrow";
		dam = damroll(3,9);
		tval = TV_ARROW;
		sval = SV_AMMO_HEAVY;
	      }
	  }
	
	else
	  {
	    if (p_ptr->depth < 65)
	      {
		missile_name = "bolt";
		dam = damroll(2,5);
		tval = TV_BOLT;
		sval = SV_AMMO_NORMAL;
	      }
	    else
	      {
		missile_name = "seeker bolt";
		dam = damroll(3,11);
		tval = TV_BOLT;
		sval = SV_AMMO_HEAVY;
	      }
	  }
	
	/* determine if the missile hits. */
	if (check_trap_hit(75 + p_ptr->depth))
	  {
	    msg_format("A %s hits you from above.", missile_name);
	    
	    Rand_quick = FALSE;
	    
	    /* critical hits. */
	    if (randint(2) == 1)
	      {
		msg_print("It was well-aimed!");
		dam *= 1 + randint(2);
	      }
	    if (randint(2) == 1)
	      {
		msg_print("It gouges you!");
		dam = 3 * dam / 2;
		
		/* cut the player. */
		(void)set_cut(p_ptr->cut + randint(dam));
	      }
	    
	    Rand_quick = TRUE;
	    
	    take_hit(dam, name);
	  }
	
	/* Explain what just happened. */
	else msg_format("A %s wizzes by your head.", missile_name);
	
	/* these will eventually run out of ammo. */

	Rand_quick = FALSE;
	
	if (rand_int(8) == 0)
	  {
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_FLOOR);
	  }

	Rand_quick = TRUE;
	
	/* Get local object */
	o_ptr = &object_type_body;
	
	/* Make a missile, identify it, and drop it near the player. */
	object_prep(o_ptr, lookup_kind(tval, sval));
	object_aware(o_ptr);
	object_known(o_ptr);
	drop_near(o_ptr, -1, y, x);
	
	break;
      }
      
      /* falling tree branch */
    case FEAT_TRAP_HEAD + 0x0A:
      {
	/* determine if the missile hits. */
	if (check_trap_hit(75 + p_ptr->depth))
	  {
	    /* Take damage */
	    dam = damroll(3,5);
	    msg_print("A branch hits you from above.");
	    
	    Rand_quick = FALSE;
	    
	    /* critical hits. */
	    if (randint(2) == 1)
	      {
		msg_print("It was heavy!");
		dam = 3 * dam / 2;
		
		/* stun the player. */
		(void)set_stun(p_ptr->cut + randint(dam));
	      }
	    
	    Rand_quick = TRUE;
	    
	    take_hit(dam, name);
	  }
	
	/* Explain what just happened. */
	else msg_print("A falling branch just misses you.");
	
	/* No more */
	cave_info[y][x] &= ~(CAVE_MARK);
	cave_set_feat(y, x, FEAT_TREE);
	
	break;
      }
      
      /* falling tree branch */
    case FEAT_TRAP_HEAD + 0x0B:
      {
	/* determine if the missile hits. */
	if (check_trap_hit(75 + p_ptr->depth))
	  {
	    /* Take damage */
	    dam = damroll(3,5);
	    msg_print("A branch hits you from above.");
	    
	    Rand_quick = FALSE;
	    
	    /* critical hits. */
	    if (randint(2) == 1)
	      {
		msg_print("It was heavy!");
		dam = 3 * dam / 2;
		
		/* stun the player. */
		(void)set_stun(p_ptr->cut + randint(dam));
	      }
	    
	    Rand_quick = TRUE;
	    
	    take_hit(dam, name);
	  }
	
	/* Explain what just happened. */
	else msg_print("A falling branch just misses you.");
	
	/* No more */
	cave_info[y][x] &= ~(CAVE_MARK);
	cave_set_feat(y, x, FEAT_TREE2);
	
	break;
      }
      
      /* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0C:
      {
	msg_print("A dagger is thrown at you from the shadows!");
	dam = damroll(3,4);
	take_hit(dam, name);
	
	break;
      }
      
      /* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0D:
      {
	msg_print("A dagger is thrown at you from the shadows!");
	dam = damroll(3,4);
	take_hit(dam, name);
	
	break;
      }
      
      /* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0E:
      {
	msg_print("A dagger is thrown at you from the shadows!");
	dam = damroll(3,4);
	take_hit(dam, name);
	
	break;
      }
      
      /* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0F:
      {
	msg_print("A dagger is thrown at you from the shadows!");
	dam = damroll(3,4);
	take_hit(dam, name);
	
	break;
      }
      
    }
  
  /* Revert to usage of the complex RNG. */
  Rand_quick = FALSE;
}

/**
 * Handle falling off cliffs 
 */
void fall_off_cliff(void)
{
  int i = 0, dam;

  msg_print("You fall into the darkness!");

  /* Where we fell from */
  p_ptr->last_stage = p_ptr->stage;

  /* From the mountaintop */
  if (stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP)
    {
      p_ptr->stage = stage_map[p_ptr->stage][DOWN];
      p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

      /* Reset */
      stage_map[256][DOWN] = 0;
      stage_map[p_ptr->stage][UP] = 0;
      stage_map[256][DEPTH] = 0;

      if (p_ptr->ffall)
	{
	  notice_obj(OF_FEATHER, 0);
	  dam = damroll(2, 8);
	  set_stun(p_ptr->stun + damroll(2, 8));
	  set_cut(p_ptr->cut + damroll(2, 8));
	}
      else
	{
	  dam = damroll(4, 8);
	  set_stun(p_ptr->stun + damroll(4, 8));
	  set_cut(p_ptr->cut + damroll(4, 8));
	}
      take_hit(dam, "falling off a precipice");
    }

  /* Nan Dungortheb */
  else
    {
      /* Fall at least one level */
      for (i = 0; i < 1; i = rand_int(3))
	{
	  p_ptr->stage = stage_map[p_ptr->stage][SOUTH];
	  p_ptr->depth++;
	  if (p_ptr->ffall)
	    {
	      notice_obj(OF_FEATHER, 0);
	      dam = damroll(2, 8);
	      set_stun(p_ptr->stun + damroll(2, 8));
	      set_cut(p_ptr->cut + damroll(2, 8));
	    }
	  else
	    {
	      dam = damroll(4, 8);
	      set_stun(p_ptr->stun + damroll(4, 8));
	      set_cut(p_ptr->cut + damroll(4, 8));
	    }
	  take_hit(dam, "falling off a precipice");
	  if (p_ptr->depth == 70) break;
	}

      /* Check for quests */
      if ((adult_dungeon) && is_quest(p_ptr->stage) && (p_ptr->depth < 100))
	{
	  int i;
	  monster_race *r_ptr = NULL;
	  
	  /* Find the questor */
	  for (i = 0; i < z_info->r_max; i++)
	    {
	      r_ptr = &r_info[i];
	      if ((r_ptr->flags1 & RF1_QUESTOR) && 
		  (r_ptr->level == p_ptr->depth))
		break;
	    }
	  
	  /* Announce */
	  msg_format("This level is home to %s.", r_name + r_ptr->name);
	}

    }
  
  /* Leaving */
  p_ptr->leaving = TRUE;
	
}

/**
 * Move player in the given direction, with the given "pickup" flag.
 *
 * This routine should only be called when energy has been expended.
 *
 * Note that this routine handles monsters in the destination grid,
 * and also handles attempting to move into walls/doors/etc.
 */
void move_player(int dir, int do_pickup)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  byte str_escape, dex_escape;
  
  /* Permit the player to move? */
  bool can_move = FALSE;

  /* Player is jumping off a cliff */
  bool falling = FALSE;

  /* Player hits a trap (always unless flying) */
  bool trapped = TRUE;
  
  int temp;
  int y, x;
  
  /* Find the result of moving */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Hack -- attack monsters */
  if (cave_m_idx[y][x] > 0)
    {
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* It takes some dexterity, or failing that, strength, to get out of 
   * pits.
   */
  if (cave_feat[p_ptr->py][p_ptr->px] == (FEAT_TRAP_HEAD + 0x01))
    {
      str_escape = adj_dex_dis[p_ptr->stat_ind[A_STR]];
      dex_escape = adj_dex_dis[p_ptr->stat_ind[A_DEX]];
      
      /* First attempt to leap out of the pit, */
      if ((dex_escape + 1) * 2 < randint(16))
	{
	  /* then attempt to climb out of the pit. */
	  if (str_escape + 3 < randint(16))
	    {
	      /* Failure costs a turn. */
	      msg_print("You remain stuck in the pit.");
	      return;
	    }
	  else msg_print("You clamber out of the pit.");
	}
      else msg_print("You leap out of the pit.");
    }
  
  
  /* Option to disarm a visible trap. -TNB- */
  /* Hack - Rogues can walk over their own trap - BR */
  if ((easy_disarm) && 
      (cave_feat[y][x] >= FEAT_TRAP_HEAD) &&
      (cave_feat[y][x] <= FEAT_TRAP_TAIL))
    {
      /* Optional auto-repeat. */
      if (always_repeat && (p_ptr->command_arg <= 0))
	{
	  /* Repeat 99 times */
	  p_ptr->command_arg = 99;
	}
      
      do_cmd_disarm();
      return;
    }
  
  /* Some terrain is impassable for the player, such as stone walls. */
  else if (!cave_passable_bold(y, x))
    {
      /* Disturb the player */
      disturb(0, 0);
      
      /* Notice unknown obstacles */
      if (!(cave_info[y][x] & (CAVE_MARK)))
	{
	  /* Closed door */
	  if (cave_feat[y][x] < FEAT_SECRET)
	    {
	      message(MSG_HITWALL, 0, 
		      "You feel a door blocking your way.");
	      cave_info[y][x] |= (CAVE_MARK);
	      lite_spot(y, x);
	    }
	  
	  /* Wall (or secret door) */
	  else
	    {
	      message(MSG_HITWALL, 0, 
		      "You feel a wall blocking your way.");
	      cave_info[y][x] |= (CAVE_MARK);
	      lite_spot(y, x);
	    }
	}
      
      /* Mention known obstacles */
      else
	{
	  /* Closed door */
	  if (cave_feat[y][x] < FEAT_SECRET)
	    {
	      /* Option to automatically open doors. -TNB- */
	      if (easy_open)
		{
		  /* Optional auto-repeat. */
		  if (always_repeat && (p_ptr->command_arg <= 0))
		    {
		      /* Repeat 99 times */
		      p_ptr->command_arg = 99;
		    }
		  
		  do_cmd_open();
		  return;
		}
	      
	      /* Otherwise, a message. */
	      message(MSG_HITWALL, 0, 
		      "There is a door blocking your way.");
	    }
	  
	  /* Wall (or secret door) */
	  else
	    {
	      message(MSG_HITWALL, 0, 
		      "There is a wall blocking your way.");
	    }
	}
      
      /* Sound */
      sound(MSG_HITWALL);
    }
  
  /* Normal movement */
  else
    {
      /* Sound XXX XXX XXX */
      /* sound(MSG_WALK); */
      
      /*** Handle traversable terrain.  ***/
      switch(cave_feat[y][x])
	{
	case FEAT_RUBBLE:
	  {
	    /* Dwarves move easily through rubble */
	    if (check_ability(SP_DWARVEN)) 
	      can_move = TRUE;
	    
	    /* Bats, dragons can fly */
	    else if ((p_ptr->schange == SHAPE_BAT) || 
		     (p_ptr->schange == SHAPE_WYRM))
	      can_move = TRUE;
	    
	    else if (player_is_crossing == dir) 
	      can_move = TRUE;
	    else 
	      {
		player_is_crossing = dir;
		
		/* Automate 2nd movement command, if not disturbed. */
		p_ptr->command_cmd = 59;
		p_ptr->command_rep = 1;
		p_ptr->command_dir = dir;
	      }
	    
	    break;
	  }
	case FEAT_TREE: case FEAT_TREE2:
	  {
	    /* Druids, rangers, elves and ents (SJGU) slip easily under trees */
	    if (((check_ability(SP_WOODSMAN)) || (check_ability(SP_ELVEN))) || 
		(check_ability(SP_WOODEN)))
	      can_move = TRUE;
	    
	    /* Bats, dragons can fly */
	    else if ((p_ptr->schange == SHAPE_BAT) || 
		     (p_ptr->schange == SHAPE_WYRM))
	      can_move = TRUE;
	    
	    /* Allow movement only if partway through already. */
	    else if (player_is_crossing == dir) 
	      can_move = TRUE;
	    else 
	      {
		player_is_crossing = dir;
		
		/* Automate 2nd movement command, if not disturbed. */
		p_ptr->command_cmd = 59;
		p_ptr->command_rep = 1;
		p_ptr->command_dir = dir;
	      }
	    
	    break;
	  }
	case FEAT_WATER: /* Water now slows rather than stopping -NRM- */
	  {
	    /* Stop any run. */
	    disturb(0,0);
	    
	    can_move = TRUE;
	    
	    /* Speed will need updating */
	    p_ptr->update |= PU_BONUS;
	    
	    break;
	  }
	case FEAT_LAVA:
	  {
	    /* Assume player will continue. */
	    temp = TRUE;
	    
	    /* Smart enough to stop running. */
	    if (p_ptr->running) 
	      {
		if (!get_check("Lava blocks your path.  Step into it? "))
		  {
		    temp = FALSE;
		    p_ptr->running = 0;
		  }
	      }
	    
	    /* Smart enough to sense trouble. */
	    else if ((!p_resist_pos(P_RES_FIRE)) ||
		     (!p_resist_strong(P_RES_FIRE) && (p_ptr->chp <= 100)) 
		     || (!p_immune(P_RES_FIRE) && (p_ptr->chp <= 30)))
	      {
		if (!get_check("The heat of the lava scalds you! Really enter? "))
		  {
		    temp = FALSE;
		  }
	      }
	    
	    /* Enter if OK or confirmed. */
	    if (temp)
	      {
		/* Can always cross. */
		can_move = TRUE;
		
		/* Feather fall makes one lightfooted. */
		if (p_ptr->ffall) 
		  {
		    notice_obj(OF_FEATHER, 0);
		    temp = 49 + randint(51);
		  }
		else temp = 124 + randint(126);
		
		/* Will take serious fire damage. */
		fire_dam(temp, "burnt to a cinder in molten lava");
	      }
	    break;
	  }
	case FEAT_VOID:
	  {
	    /* Bats, dragons can fly */
	    if ((p_ptr->schange == SHAPE_BAT) || 
		(p_ptr->schange == SHAPE_WYRM))
	      can_move = TRUE;
	    else
	      {
		/* Assume player will continue. */
		temp = TRUE;
		
		/* Smart enough to stop running. */
		if (p_ptr->running) 
		  {
		    if (!get_check("You have come to a cliff.  Step off it? "))
		      {
			temp = FALSE;
			p_ptr->running = 0;
		      }
		  }
		
		/* Smart enough to sense trouble. */
		else if (!p_ptr->blind)
		  {
		    if (!get_check("It's a cliff! Really step off it? "))
		      {
			temp = FALSE;
		      }
		  }
		
		/* Step off if confirmed. */
		if (temp)
		  {
		    /* Can always jump. */
		    can_move = TRUE;
		    
		    /* Will take serious damage. */
		    falling = TRUE;
		  }
	      }
	    break;
	  }
	default:
	  {
	    /* All other terrain can be traversed normally. */
	    can_move = TRUE;
	  }
	}
      
      /* If the player can move, handle various things. */
      if (can_move)
	{
	  /* Move player */
	  monster_swap(py, px, y, x);
	  
	  /* Update speed if stepping out of water */
	  if (cave_feat[py][px] == FEAT_WATER)
	    p_ptr->update |= PU_BONUS;
	  
	  /* Update stealth for Unlight */
	  if (check_ability(SP_UNLIGHT))
	    p_ptr->update |= PU_BONUS;
	  
	  /* Superstealth for ents in trees SJGU */
	  if ((check_ability(SP_WOODEN)) && 
	      (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags & TF_TREE))
	    {
	      if (!(f_info[cave_feat[py][px]].flags & TF_TREE) || 
		  !(p_ptr->superstealth))
		{
		  set_superstealth(p_ptr->superstealth + 1,FALSE);
		  p_ptr->update |= (PU_BONUS);
		}
	    }
	  else if ((check_ability(SP_WOODEN)) && 
		   (f_info[cave_feat[py][px]].flags & TF_TREE))
	    {
	      if (p_ptr->superstealth)
		{
		  set_superstealth(p_ptr->superstealth - 1,FALSE);
		  p_ptr->update |= (PU_BONUS);
		}
	    }
	  
	  /* New location */
	  y = py = p_ptr->py;
	  x = px = p_ptr->px;
	  
	  /* No longer traversing. */
	  player_is_crossing = 0;
	  
	  /* Fall off a cliff */
	  if (falling)
	    fall_off_cliff();
	  
	  /* Spontaneous Searching */
	  if (p_ptr->skill_fos > 49)
	    {
	      search();
	    }
	  else if (0 == rand_int(50 - p_ptr->skill_fos))
	    {
	      search();
	    }
	  
	  /* Continuous Searching */
	  if (p_ptr->searching)
	    {
	      search();
	    }
	  
	  /* Handle "objects".  Do not use extra energy for 
	   * objects picked up.
	   */
	  (void)py_pickup(do_pickup, p_ptr->py, p_ptr->px);
	  
	  /* Handle "store doors" */
	  if ((cave_feat[y][x] >= FEAT_SHOP_HEAD) &&
	      (cave_feat[y][x] <= FEAT_SHOP_TAIL))
	    {
	      /* Disturb */
	      disturb(0, 0);
	      
	      /* Hack -- Enter store */
	      p_ptr->command_new = '_';
	    }
	  
	  /* Flying players have a chance to miss traps */
	  if ((p_ptr->schange == SHAPE_BAT) || 
	      (p_ptr->schange == SHAPE_WYRM))
	    {
	      if (((cave_feat[y][x] == FEAT_INVIS) ||
		   (cave_feat[y][x] == FEAT_GRASS_INVIS)) && 
		  (rand_int(3) != 0))
		trapped = FALSE;
	      else if ((cave_feat[y][x] >= FEAT_TRAP_HEAD) &&
		       (cave_feat[y][x] <= FEAT_TRAP_TAIL) &&
		       (rand_int(10) != 0))
		trapped = FALSE;
	    }
	  
	  /* Discover invisible traps */
	  else if (((cave_feat[y][x] == FEAT_INVIS) ||
		    (cave_feat[y][x] == FEAT_GRASS_INVIS) ||
		    (cave_feat[y][x] == FEAT_TREE_INVIS) ||
		    (cave_feat[y][x] == FEAT_TREE2_INVIS)) && trapped)
	    {
	      /* Disturb */
	      disturb(0, 0);
	      
	      /* Message */
	      msg_print("You stumble upon a trap!");
	      
	      /* Pick a trap */
	      pick_trap(y, x);
	      
	      /* Hit the floor trap. */
	      hit_trap(y, x);
	    }
	  
	  /* Set off a visible trap */
	  else if ((cave_feat[y][x] >= FEAT_TRAP_HEAD) &&
		   (cave_feat[y][x] <= FEAT_TRAP_TAIL) && trapped)
	    {
	      /* Disturb */
	      disturb(0, 0);
	      
	      /* Hit the floor trap. */
	      hit_trap(y, x);
	    }
	  
	  /* Walk on a monster trap */
	  else if ((cave_feat[y][x] >= FEAT_MTRAP_HEAD) &&
		   (cave_feat[y][x] <= FEAT_MTRAP_TAIL))
	    {
	      msg_print("You inspect your cunning trap.");
	    }
	}
    }
}


/**
 * Hack -- Check for a "known wall" (see below)
 */
static int see_wall(int dir, int y, int x)
{
  /* Get the new location */
  y += ddy[dir];
  x += ddx[dir];
  
  /* Illegal grids are not known walls XXX XXX XXX */
  if (!in_bounds(y, x)) return (FALSE);
  
  /* Non-wall grids are not known walls */
  if ((cave_feat[y][x] < FEAT_SECRET) || (cave_feat[y][x] > FEAT_SHOP_HEAD)) 
    return (FALSE);
  
  /* Unknown walls are not known walls */
  if (!(cave_info[y][x] & (CAVE_MARK))) return (FALSE);
  
  /* Default */
  return (TRUE);
}


/**
 * Hack -- Check for an "unknown corner" (see below)
 */
static int see_nothing(int dir, int y, int x)
{
  /* Get the new location */
  y += ddy[dir];
  x += ddx[dir];
  
  /* Illegal grids are unknown XXX XXX XXX */
  if (!in_bounds(y, x)) return (TRUE);
  
  /* Memorized grids are always known */
  if (cave_info[y][x] & (CAVE_MARK)) return (FALSE);
  
  /* Default */
  return (TRUE);
}





/**
 * The running algorithm  -CJS-
 *
 * Basically, once you start running, you keep moving until something
 * interesting happens.   In an enclosed space, you run straight, but
 * you follow corners as needed (i.e. hallways).  In an open space,
 * you run straight, but you stop before entering an enclosed space
 * (i.e. a room with a doorway).  In a semi-open space (with walls on
 * one side only), you run straight, but you stop before entering an
 * enclosed space or an open space (i.e. running along side a wall).
 *
 * All discussions below refer to what the player can see, that is,
 * an unknown wall is just like a normal floor.   This means that we
 * must be careful when dealing with "illegal" grids.
 *
 * No assumptions are made about the layout of the dungeon, so this
 * algorithm works in hallways, rooms, town, destroyed areas, etc.
 *
 * In the diagrams below, the player has just arrived in the grid
 * marked as '@', and he has just come from a grid marked as 'o',
 * and he is about to enter the grid marked as 'x'.
 *
 * Running while confused is not allowed, and so running into a wall
 * is only possible when the wall is not seen by the player.  This
 * will take a turn and stop the running.
 *
 * Several conditions are tracked by the running variables.
 *
 *   p_ptr->run_open_area (in the open on at least one side)
 *   p_ptr->run_break_left (wall on the left, stop if it opens)
 *   p_ptr->run_break_right (wall on the right, stop if it opens)
 *
 * When running begins, these conditions are initialized by examining
 * the grids adjacent to the requested destination grid (marked 'x'),
 * two on each side (marked 'L' and 'R').  If either one of the two
 * grids on a given side is a wall, then that side is considered to
 * be "closed".   Both sides enclosed yields a hallway.
 *<pre>
 *    LL		     @L
 *    @x      (normal)	     RxL   (diagonal)
 *    RR      (east)	      R	   (south-east)
 *</pre>
 * In the diagram below, in which the player is running east along a
 * hallway, he will stop as indicated before attempting to enter the
 * intersection (marked 'x').  Starting a new run in any direction
 * will begin a new hallway run.
 *<pre>
 * #.#
 * ##.##
 * o@x..
 * ##.##
 * #.#
 </pre>
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * In the diagram below, the player is running east down a hallway,
 * and will stop in the grid (marked '1') before the intersection.
 * Continuing the run to the south-east would result in a long run
 * stopping at the end of the hallway (marked '2').
 *<pre>
 * ##################
 * o@x	     1
 * ########### ######
 * #2	       #
 * #############
 *</pre>
 * After each step, the surroundings are examined to determine if
 * the running should stop, and to determine if the running should
 * change direction.  We examine the new current player location
 * (at which the runner has just arrived) and the direction from
 * which the runner is considered to have come.
 *
 * Moving one grid in some direction places you adjacent to three
 * or five new grids (for straight and diagonal moves respectively)
 * to which you were not previously adjacent (marked as '!').
 *<pre>
 *   ...!	       ...
 *   .o@!  (normal)    .o.!  (diagonal)
 *   ...!  (east)      ..@!  (south east)
 *			!!!
 *</pre>
 * If any of the newly adjacent grids are "interesting" (monsters,
 * objects, some terrain features) then running stops.
 *
 * If any of the newly adjacent grids seem to be open, and you are
 * looking for a break on that side, then running stops.
 *
 * If any of the newly adjacent grids do not seem to be open, and
 * you are in an open area, and the non-open side was previously
 * entirely open, then running stops.
 *
 * If you are in a hallway, then the algorithm must determine if
 * the running should continue, turn, or stop.  If only one of the
 * newly adjacent grids appears to be open, then running continues
 * in that direction, turning if necessary.  If there are more than
 * two possible choices, then running stops.  If there are exactly
 * two possible choices, separated by a grid which does not seem
 * to be open, then running stops.  Otherwise, as shown below, the
 * player has probably reached a "corner".
 *<pre>
 *    ###	      o##
 *    o@x  (normal)   #@!   (diagonal)
 *    ##!  (east)     ##x   (south east)
 *</pre>
 * In this situation, there will be two newly adjacent open grids,
 * one touching the player on a diagonal, and one directly adjacent.
 * We must consider the two "option" grids further out (marked '?').
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid.  For some unknown reason, we assign "check_dir" to
 * the grid marked 's', which may be incorrectly labelled.
 *
 *    ###s
 *    o@x?   (may be incorrect diagram!)
 *    ##!?
 *
 * If both "option" grids are closed, then there is no reason to enter
 * the corner, and so we can cut the corner, by moving into the other
 * grid (diagonally).  If we choose not to cut the corner, then we may
 * go straight, but we pretend that we got there by moving diagonally.
 * Below, we avoid the obvious grid (marked 'x') and cut the corner
 * instead (marked 'n').
 *<pre>
 *    ###:		 o##
 *    o@x#   (normal)	 #@n	(maybe?)
 *    ##n#   (east)	 ##x#
 *			 ####
 *</pre>
 * If one of the "option" grids is open, then we may have a choice, so
 * we check to see whether it is a potential corner or an intersection
 * (or room entrance).  If the grid two spaces straight ahead, and the
 * space marked with 's' are both open, then it is a potential corner
 * and we enter it if requested.  Otherwise, we stop, because it is
 * not a corner, and is instead an intersection or a room entrance.
 *<pre>
 *    ###
 *    o@x
 *    ##!#
 *</pre>
 * I do not think this documentation is correct.
 */




/**
 * Hack -- allow quick "cycling" through the legal directions
 */
static byte cycle[] =
{ 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/**
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] =
{ 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };



/**
 * Initialize the running algorithm for a new direction.
 *
 * Diagonal Corridor -- allow diaginal entry into corridors.
 *
 * Blunt Corridor -- If there is a wall two spaces ahead and
 * we seem to be in a corridor, then force a turn into the side
 * corridor, must be moving straight into a corridor here. ???
 *
 * Diagonal Corridor	Blunt Corridor (?)
 *	 # #		      #
 *	 #x#		     @x#
 *	 @p.		      p
 */
static void run_init(int dir)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int i, row, col;
  
  bool deepleft, deepright;
  bool shortleft, shortright;
  
  
  /* Save the direction */
  p_ptr->run_cur_dir = dir;
  
  /* Assume running straight */
  p_ptr->run_old_dir = dir;
  
  /* If it's wilderness, done -NRM- */
  if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
      && (stage_map[p_ptr->stage][STAGE_TYPE] != TOWN)) return;
  
  /* Assume looking for open area */
  p_ptr->run_open_area = TRUE;
  
  /* Assume not looking for breaks */
  p_ptr->run_break_right = FALSE;
  p_ptr->run_break_left = FALSE;
  
  /* Assume no nearby walls */
  deepleft = deepright = FALSE;
  shortright = shortleft = FALSE;
  
  /* Find the destination grid */
  row = py + ddy[dir];
  col = px + ddx[dir];
  
  /* Extract cycle index */
  i = chome[dir];
  
  /* Check for nearby wall */
  if (see_wall(cycle[i+1], py, px))
    {
      p_ptr->run_break_left = TRUE;
      shortleft = TRUE;
    }
  
  /* Check for distant wall */
  else if (see_wall(cycle[i+1], row, col))
    {
      p_ptr->run_break_left = TRUE;
      deepleft = TRUE;
    }
  
  /* Check for nearby wall */
  if (see_wall(cycle[i-1], py, px))
    {
      p_ptr->run_break_right = TRUE;
      shortright = TRUE;
    }
  
  /* Check for distant wall */
  else if (see_wall(cycle[i-1], row, col))
    {
      p_ptr->run_break_right = TRUE;
      deepright = TRUE;
    }
  
  /* Looking for a break */
  if (p_ptr->run_break_left && p_ptr->run_break_right)
    {
      /* Not looking for open area */
      p_ptr->run_open_area = FALSE;
      
      /* Hack -- allow angled corridor entry */
      if (dir & 0x01)
	{
	  if (deepleft && !deepright)
	    {
	      p_ptr->run_old_dir = cycle[i - 1];
	    }
	  else if (deepright && !deepleft)
	    {
	      p_ptr->run_old_dir = cycle[i + 1];
	    }
	}
      
      /* Hack -- allow blunt corridor entry */
      else if (see_wall(cycle[i], row, col))
	{
	  if (shortleft && !shortright)
	    {
	      p_ptr->run_old_dir = cycle[i - 2];
	    }
	  else if (shortright && !shortleft)
	    {
	      p_ptr->run_old_dir = cycle[i + 2];
	    }
	}
    }
}


/**
 * Update the current "run" path
 *
 * Return TRUE if the running should be stopped
 */
static bool run_test(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int prev_dir;
  int new_dir;
  int check_dir = 0;
  int left_dir;
  int right_dir;
  
  int row, col;
  int i, max, inv;
  int option, option2;
  
  
  /* No options yet */
  option = 0;
  option2 = 0;
  
  /* Where we came from */
  prev_dir = p_ptr->run_old_dir;
  
  /* Range of newly adjacent grids */
  max = (prev_dir & 0x01) + 1;
  
  /* Simplistic running for outdoors -NRM- */
  if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) && 
      (stage_map[p_ptr->stage][STAGE_TYPE] != TOWN))
    {
      /* Look at every newly adjacent square. */
      for (i = -max; i <= max; i++)
	{
	  s16b this_o_idx, next_o_idx = 0;
	  
	  
	  /* New direction */
	  new_dir = cycle[chome[prev_dir] + i];
	  
	  /* New location */
	  row = py + ddy[new_dir];
	  col = px + ddx[new_dir];
	  
	  
	  /* Visible monsters abort running */
	  if (cave_m_idx[row][col] > 0)
	    {
	      monster_type *m_ptr = &m_list[cave_m_idx[row][col]];
	      
	      /* Visible monster */
	      if (m_ptr->ml) return (TRUE);
	    }
	  
	  /* Visible objects abort running */
	  for (this_o_idx = cave_o_idx[row][col]; this_o_idx; 
	       this_o_idx = next_o_idx)
	    {
	      object_type *o_ptr;
	      
	      /* Acquire object */
	      o_ptr = &o_list[this_o_idx];
	      
	      /* Acquire next object */
	      next_o_idx = o_ptr->next_o_idx;
	      
	      /* Visible object */
	      if (o_ptr->marked && !squelch_hide_item(o_ptr)) return (TRUE);
	    }
	}
      
      /* Assume main direction */
      new_dir = p_ptr->run_old_dir;
      row = py + ddy[new_dir];
      col = px + ddx[new_dir];
      
      
      /* Step if there's a path in the right direction */
      if ((cave_feat[row][col] == FEAT_FLOOR) ||
	  (cave_feat[row][col] == FEAT_INVIS))
	{
	  p_ptr->run_cur_dir = new_dir;
	  return (FALSE);
	}
      
      /* Check to the left */
      left_dir = cycle[chome[prev_dir] - 1];
      row = py + ddy[left_dir];
      col = px + ddx[left_dir];
      if ((cave_feat[row][col] == FEAT_FLOOR) ||
	  (cave_feat[row][col] == FEAT_INVIS))
	option = left_dir;
      
      /* Check to the right */
      right_dir = cycle[chome[prev_dir] + 1];
      row = py + ddy[right_dir];
      col = px + ddx[right_dir];
      if ((cave_feat[row][col] == FEAT_FLOOR) ||
	  (cave_feat[row][col] == FEAT_INVIS))
	option2 = right_dir;
      
      /* Stop if it's a fork */
      if (option && option2) return (TRUE);
      
      /* Otherwise step in the secondary direction */
      if (option) 
	{
	  p_ptr->run_cur_dir = left_dir;
	  return (FALSE);
	}
      else if (option2)
	{
	  p_ptr->run_cur_dir = right_dir;
	  return (FALSE);
	}
      
      /* No paths, so try grass */
      row = py + ddy[new_dir];
      col = px + ddx[new_dir];
      
      
      /* Step if there's grass in the right direction */
      if ((cave_feat[row][col] == FEAT_GRASS) ||
	  (cave_feat[row][col] == FEAT_GRASS_INVIS))
	{
	  p_ptr->run_cur_dir = new_dir;
	  return (FALSE);
	}
      
      /* Check to the left */
      row = py + ddy[left_dir];
      col = px + ddx[left_dir];
      if ((cave_feat[row][col] == FEAT_GRASS) ||
	  (cave_feat[row][col] == FEAT_GRASS_INVIS))
	option = left_dir;
      
      /* Check to the right */
      right_dir = cycle[chome[prev_dir] + 1];
      row = py + ddy[right_dir];
      col = px + ddx[right_dir];
      if ((cave_feat[row][col] == FEAT_GRASS) ||
	  (cave_feat[row][col] == FEAT_GRASS_INVIS))
	option2 = right_dir;
      
      /* Stop if it's a fork */
      if (option && option2) return (TRUE);
      
      /* Otherwise step in the secondary direction */
      if (option) 
	{
	  p_ptr->run_cur_dir = left_dir;
	  return (FALSE);
	}
      else if (option2)
	{
	  p_ptr->run_cur_dir = right_dir;
	  return (FALSE);
	}
      
    }
  
  /* Look at every newly adjacent square. */
  for (i = -max; i <= max; i++)
    {
      s16b this_o_idx, next_o_idx = 0;
      
      
      /* New direction */
      new_dir = cycle[chome[prev_dir] + i];
      
      /* New location */
      row = py + ddy[new_dir];
      col = px + ddx[new_dir];
      
      
      /* Visible monsters abort running */
      if (cave_m_idx[row][col] > 0)
	{
	  monster_type *m_ptr = &m_list[cave_m_idx[row][col]];
	  
	  /* Visible monster */
	  if (m_ptr->ml) return (TRUE);
	}
      
      /* Visible objects abort running */
      for (this_o_idx = cave_o_idx[row][col]; this_o_idx; 
	   this_o_idx = next_o_idx)
	{
	  object_type *o_ptr;
	  
	  /* Acquire object */
	  o_ptr = &o_list[this_o_idx];
	  
	  /* Acquire next object */
	  next_o_idx = o_ptr->next_o_idx;
	  
	  /* Visible object */
	  if (o_ptr->marked) return (TRUE);
	}
      
      
      /* Assume unknown */
      inv = TRUE;
      
      /* Check memorized grids */
      if (cave_info[row][col] & (CAVE_MARK))
	{
	  bool notice = TRUE;
	  
	  /* Examine the terrain */
	  switch (cave_feat[row][col])
	    {
	      /* Floors */
	    case FEAT_FLOOR:
	      
	      /* Invis traps */
	    case FEAT_INVIS:
	    case FEAT_GRASS_INVIS:
	      
	      /* Secret doors */
	    case FEAT_SECRET:
	      
	      /* Normal veins */
	    case FEAT_MAGMA:
	    case FEAT_QUARTZ:
	      
	      /* Hidden treasure */
	    case FEAT_MAGMA_H:
	    case FEAT_QUARTZ_H:
	      
	      /* Special passable terrain. */
	    case FEAT_LAVA:
	    case FEAT_WATER:
	    case FEAT_TREE:
	    case FEAT_TREE2:
	    case FEAT_GRASS:
	      {
		/* Ignore */
		notice = FALSE;
		
		/* Done */
		break;
	      }
	      
	      /* Walls */
	    case FEAT_WALL_EXTRA:
	    case FEAT_WALL_INNER:
	    case FEAT_WALL_OUTER:
	    case FEAT_WALL_SOLID:
	    case FEAT_PERM_EXTRA:
	    case FEAT_PERM_INNER:
	    case FEAT_PERM_OUTER:
	    case FEAT_PERM_SOLID:
	      {
		/* Ignore */
		notice = FALSE;
		
		/* Done */
		break;
	      }
	      
	      /* Open doors */
	    case FEAT_OPEN:
	    case FEAT_BROKEN:
	      {
		/* Option -- ignore */
		if (run_ignore_doors) notice = FALSE;
		
		/* Done */
		break;
	      }
	      
	      /* Stairs */
	    case FEAT_LESS:
	    case FEAT_MORE:
	      {
		/* Option -- ignore */
		if (run_ignore_stairs) notice = FALSE;
		
		/* Done */
		break;
	      }
	    }
	  
	  /* Interesting feature */
	  if (notice) return (TRUE);
	  
	  /* The grid is "visible" */
	  inv = FALSE;
	}
      
      /* Analyze unknown grids and floors */
      if (inv || cave_floor_bold(row, col))
	{
	  /* Looking for open area */
	  if (p_ptr->run_open_area)
	    {
	      /* Nothing */
	    }
	  
	  /* The first new direction. */
	  else if (!option)
	    {
	      option = new_dir;
	    }
	  
	  /* Three new directions. Stop running. */
	  else if (option2)
	    {
	      return (TRUE);
	    }
	  
	  /* Two non-adjacent new directions.  Stop running. */
	  else if (option != cycle[chome[prev_dir] + i - 1])
	    {
	      return (TRUE);
	    }
	  
	  /* Two new (adjacent) directions (case 1) */
	  else if (new_dir & 0x01)
	    {
	      check_dir = cycle[chome[prev_dir] + i - 2];
	      option2 = new_dir;
	    }
	  
	  /* Two new (adjacent) directions (case 2) */
	  else
	    {
	      check_dir = cycle[chome[prev_dir] + i + 1];
	      option2 = option;
	      option = new_dir;
	    }
	}
      
      /* Obstacle, while looking for open area */
      else
	{
	  if (p_ptr->run_open_area)
	    {
	      if (i < 0)
		{
		  /* Break to the right */
		  p_ptr->run_break_right = TRUE;
		}
	      
	      else if (i > 0)
		{
		  /* Break to the left */
		  p_ptr->run_break_left = TRUE;
		}
	    }
	}
    }
  
  
  /* Looking for open area */
  if (p_ptr->run_open_area)
    {
      /* Hack -- look again */
      for (i = -max; i < 0; i++)
	{
	  new_dir = cycle[chome[prev_dir] + i];
	  
	  row = py + ddy[new_dir];
	  col = px + ddx[new_dir];
	  
	  /* Unknown grid or non-wall */
	  /* Was: cave_floor_bold(row, col) */
	  if (!(cave_info[row][col] & (CAVE_MARK)) ||
	      (cave_feat[row][col] < FEAT_SECRET) ||
	      (cave_feat[row][col] > FEAT_SHOP_HEAD))
	    {
	      /* Looking to break right */
	      if (p_ptr->run_break_right)
		{
		  return (TRUE);
		}
	    }
	  
	  /* Obstacle */
	  else
	    {
	      /* Looking to break left */
	      if (p_ptr->run_break_left)
		{
		  return (TRUE);
		}
	    }
	}
      
      /* Hack -- look again */
      for (i = max; i > 0; i--)
	{
	  new_dir = cycle[chome[prev_dir] + i];
	  
	  row = py + ddy[new_dir];
	  col = px + ddx[new_dir];
	  
	  /* Unknown grid or non-wall */
	  /* Was: cave_floor_bold(row, col) */
	  if (!(cave_info[row][col] & (CAVE_MARK)) ||
	      (cave_feat[row][col] < FEAT_SECRET) ||
	      (cave_feat[row][col] > FEAT_SHOP_HEAD))
	    {
	      /* Looking to break left */
	      if (p_ptr->run_break_left)
		{
		  return (TRUE);
		}
	    }
	  
	  /* Obstacle */
	  else
	    {
	      /* Looking to break right */
	      if (p_ptr->run_break_right)
		{
		  return (TRUE);
		}
	    }
	}
    }
  
  
  /* Not looking for open area */
  else
    {
      /* No options */
      if (!option)
	{
	  return (TRUE);
	}
      
      /* One option */
      else if (!option2)
	{
	  /* Primary option */
	  p_ptr->run_cur_dir = option;
	  
	  /* No other options */
	  p_ptr->run_old_dir = option;
	}
      
      /* Two options, examining corners */
      else if (run_use_corners && !run_cut_corners)
	{
	  /* Primary option */
	  p_ptr->run_cur_dir = option;
	  
	  /* Hack -- allow curving */
	  p_ptr->run_old_dir = option2;
	}
      
      /* Two options, pick one */
      else
	{
	  /* Get next location */
	  row = py + ddy[option];
	  col = px + ddx[option];
	  
	  /* Don't see that it is closed off. */
	  /* This could be a potential corner or an intersection. */
	  if (!see_wall(option, row, col) ||
	      !see_wall(check_dir, row, col))
	    {
	      /* Can not see anything ahead and in the direction we */
	      /* are turning, assume that it is a potential corner. */
	      if (run_use_corners &&
		  see_nothing(option, row, col) &&
		  see_nothing(option2, row, col))
		{
		  p_ptr->run_cur_dir = option;
		  p_ptr->run_old_dir = option2;
		}
	      
	      /* STOP: we are next to an intersection or a room */
	      else
		{
		  return (TRUE);
		}
	    }
	  
	  /* This corner is seen to be enclosed; we cut the corner. */
	  else if (run_cut_corners)
	    {
	      p_ptr->run_cur_dir = option2;
	      p_ptr->run_old_dir = option2;
	    }
	  
	  /* This corner is seen to be enclosed, and we */
	  /* deliberately go the long way. */
	  else
	    {
	      p_ptr->run_cur_dir = option;
	      p_ptr->run_old_dir = option2;
	    }
	}
    }
  
  
  /* About to hit a known wall, stop */
  if (see_wall(p_ptr->run_cur_dir, py, px))
    {
      return (TRUE);
    }
  
  
  /* Failure */
  return (FALSE);
}



/**
 * Take one step along the current "run" path
 *
 * Called with a real direction to begin a new run, and with zero
 * to continue a run in progress.
 */
void run_step(int dir)
{
  /* Start run */
  if (dir)
    {
      /* Paranoia */
      p_ptr->running_withpathfind = 0;

      /* Initialize */
      run_init(dir);
      
      /* Hack -- Set the run counter */
      p_ptr->running = (p_ptr->command_arg ? p_ptr->command_arg : 1000);
      
      /* Calculate torch radius */
      p_ptr->update |= (PU_TORCH);
    }
  
  /* Continue run */
  else
    {
      if (!p_ptr->running_withpathfind)
	{
	  /* Update run */
	  if (run_test())
	    {
	      /* Disturb */
	      disturb(0, 0);
	      
	      /* Done */
	      return;
	    }
	}

      else
	{
	  /* Abort if we have finished */
	  if (pf_result_index < 0)
	    {
	      disturb(0, 0);
	      p_ptr->running_withpathfind = FALSE;
	      return;
	    }
	  /* Abort if we would hit a wall */
	  else if (pf_result_index == 0)
	    {
	      int y, x;
	      
	      /* Get next step */
	      y = p_ptr->py + ddy[pf_result[pf_result_index] - '0'];
	      x = p_ptr->px + ddx[pf_result[pf_result_index] - '0'];
	      
	      /* Known wall */
	      if ((cave_info[y][x] & (CAVE_MARK)) && !is_valid_pf(y,x))
		{
		  disturb(0,0);
		  p_ptr->running_withpathfind = FALSE;
		  return;
		}
	    }
	  /* Hack -- walking stick lookahead.
	   *
	   * If the player has computed a path that is going to end up in a 
	   * wall, we notice this and convert to a normal run. This allows us 
	   * to click on unknown areas to explore the map.
	   *
	   * We have to look ahead two, otherwise we don't know which is the 
	   * last direction moved and don't initialise the run properly.
	   */
	  else if (pf_result_index > 0)
	    {
	      int y, x;
	      
	      /* Get next step */
	      y = p_ptr->py + ddy[pf_result[pf_result_index] - '0'];
	      x = p_ptr->px + ddx[pf_result[pf_result_index] - '0'];
	      
	      /* Known wall */
	      if ((cave_info[y][x] & (CAVE_MARK)) && !is_valid_pf(y,x))
		{
		  disturb(0,0);
		  p_ptr->running_withpathfind = FALSE;
		  return;
		}
	      
	      /* Get step after */
	      y = y + ddy[pf_result[pf_result_index - 1] - '0'];
	      x = x + ddx[pf_result[pf_result_index - 1] - '0'];
	      
	      /* Known wall */
	      if ((cave_info[y][x] & (CAVE_MARK)) && !is_valid_pf(y,x))
		{
		  p_ptr->running_withpathfind = FALSE;
		  
		  run_init(pf_result[pf_result_index] - '0');
		}
	    }
	  
	  if (!player_is_crossing)
	    p_ptr->run_cur_dir = pf_result[pf_result_index--] - '0';
	  
	  /* Hack -- allow easy_alter */
	  p_ptr->command_dir = p_ptr->run_cur_dir;
	}
    }
  
  /* Decrease counter */
  p_ptr->running--;
  
  /* Take time */
  p_ptr->energy_use = 100;
  
  /* Move the player, using the "pickup" flag */
  move_player(p_ptr->run_cur_dir, always_pickup);
}

