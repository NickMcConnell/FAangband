/** \file identify.c 
    \brief Pseusdo-ID, ID by use.
 
 * Feelings on items, dubious items, whether an item is an ego-item,
 * noticing of item properties.
 *
 * Copyright (c) 2010 Nick McConnell, Si Griffin
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
 * Test an item for any negative qualities
 */
bool item_dubious(const object_type *o_ptr, bool unknown)
{
  int i;
  object_kind *k_ptr = &k_info[o_ptr->k_idx];

  /* Resists, bonuses, multiples */
  for (i = 0; i < MAX_P_RES; i++)
    if (o_ptr->percent_res[i] > RES_LEVEL_BASE) return TRUE;
  for (i = 0; i < A_MAX; i++)
    if (o_ptr->bonus_stat[i] < BONUS_BASE) return TRUE;
  for (i = 0; i < MAX_P_BONUS; i++)
    if (o_ptr->bonus_other[i] < BONUS_BASE) return TRUE;
  for (i = 0; i < MAX_P_SLAY; i++)
    if (o_ptr->multiple_slay[i] < MULTIPLE_BASE) return TRUE;
  for (i = 0; i < MAX_P_BRAND; i++)
    if (o_ptr->multiple_brand[i] < MULTIPLE_BASE) return TRUE;

  /* To skill, to deadliness, to AC */
  if (o_ptr->to_h + o_ptr->to_d < k_ptr->to_h) return TRUE;
  if (o_ptr->to_a < 0) return TRUE;

  /* Only check curses if NOT known, so we can infer dubious items with
   * no other bad properties must be cursed */
  if (unknown)
    {
      /* Cursed */
      if (cursed_p(o_ptr)) return TRUE;
    }
  
  /* Must be OK */
  return FALSE;
}


/**
 * Return a "feeling" (or FEEL_NONE) about an item.  Method 1 (Heavy).
 */
int value_check_aux1(object_type *o_ptr)
{
  int slot = wield_slot(o_ptr);

  /* Wieldable? */
  if (slot < 0) return FEEL_NONE;

  /* No pseudo for lights */
  if (slot == INVEN_LITE) return FEEL_NONE;

  /* Artifacts */
  if (artifact_p(o_ptr))
    {
      /* All return special now */
      return FEEL_SPECIAL;
    }
  
  /* Ego-Items */
  if (ego_item_p(o_ptr))
    {
      /* Dubious egos (including jewellery) */
      if (item_dubious(o_ptr, TRUE)) return FEEL_PERILOUS;
      
      /* Normal */
      o_ptr->ident |= (IDENT_UNCURSED | IDENT_KNOW_CURSES);
      return FEEL_EXCELLENT;
    }
  
  /* Dubious items */
  if (item_dubious(o_ptr, TRUE)) return FEEL_DUBIOUS_STRONG;
  
  /* Known not cursed now */
  o_ptr->ident |= (IDENT_UNCURSED | IDENT_KNOW_CURSES);

  /* No average jewellery */
  if ((slot >= INVEN_LEFT) && (slot <= INVEN_NECK)) return FEEL_GOOD_STRONG;

  /* Good "armor" bonus */
  if (o_ptr->to_a > 0) return FEEL_GOOD_STRONG;
  
  /* Good "weapon" bonus */
  if (o_ptr->to_h + o_ptr->to_d > 0) return FEEL_GOOD_STRONG;
  
  /* Default to "average" */
  return FEEL_AVERAGE;
}


/**
 * Return a "feeling" (or FEEL_NONE) about an item.  Method 2 (Light).
 */
int value_check_aux2(object_type *o_ptr)
{
  int slot = wield_slot(o_ptr);

  /* Wieldable? */
  if (slot < 0) return FEEL_NONE;

  /* No pseudo for lights */
  if (slot == INVEN_LITE) return FEEL_NONE;

  /* Dubious items (all of them) */
  if (item_dubious(o_ptr, TRUE)) return FEEL_DUBIOUS_WEAK;

  /* Known not cursed now */  
  o_ptr->ident |= (IDENT_UNCURSED | IDENT_KNOW_CURSES);

  /* Artifacts -- except dubious ones */
  if (artifact_p(o_ptr)) return FEEL_GOOD_WEAK;
  
  /* Ego-Items -- except dubious ones */
  if (ego_item_p(o_ptr)) return FEEL_GOOD_WEAK;
  
  /* Good armor bonus */
  if (o_ptr->to_a > 0) return FEEL_GOOD_WEAK;
  
  /* Good weapon bonuses */
  if (o_ptr->to_h + o_ptr->to_d > 0) return FEEL_GOOD_WEAK;

/* SJGU */
  /* Default to "average" */
  return FEEL_AVERAGE;
}



/**
 * Determine if an item has the properties to be an ego item
 */
bool has_ego_properties(object_type *o_ptr)
{
  ego_item_type *e_ptr = &e_info[o_ptr->name2];

  /* Has to be an ego item */
  if (!(o_ptr->name2)) return FALSE;

  /* ID'd items are known */
  if (o_ptr->ident & IDENT_KNOWN) return TRUE;

  /* This ego type has to have been seen */
  if (!e_ptr->everseen) return FALSE;

  /* A curse matches */
  if (e_ptr->id_curse & o_ptr->id_curse) return TRUE;

  /* An object flag matches */
  if (e_ptr->id_obj & o_ptr->id_obj) return TRUE;

  /* Another property matches */
  if (e_ptr->id_other & o_ptr->id_other) return TRUE;

  /* Nothing */
  return FALSE;
}


/** 
 * Label an item as an ego item if it has the required flags
 */
void label_as_ego(object_type *o_ptr, int item)
{
  char o_name[120];
  int j;
  u32b temp_flag;
  ego_item_type *e_ptr = &e_info[o_ptr->name2];

  /* All ego object flags now known */
  o_ptr->id_obj |= e_ptr->flags_obj;

  /* All shown curses are now known */
  if (e_ptr->flags_obj & OF_SHOW_CURSE) o_ptr->id_curse |= e_ptr->flags_curse;

  /* Know all ego resists */
  for (j = 0; j < MAX_P_RES; j++)
    {
      temp_flag = OBJECT_ID_BASE_RESIST << j;
      if (e_ptr->percent_res[j] != RES_LEVEL_BASE)
	o_ptr->id_other |= temp_flag;
    }

  /* Know all ego slays */
  for (j = 0; j < MAX_P_SLAY; j++)
    {
      temp_flag = OBJECT_ID_BASE_SLAY << j;
      if (e_ptr->multiple_slay[j] != MULTIPLE_BASE)
	o_ptr->id_other |= temp_flag;
    }
  
  /* Know all ego brands */
  for (j = 0; j < MAX_P_BRAND; j++)
    {
      temp_flag = OBJECT_ID_BASE_BRAND << j;
      if (e_ptr->multiple_brand[j] != MULTIPLE_BASE)
	o_ptr->id_other |= temp_flag;
    }

  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
  /* Handle stuff */
  handle_stuff();
  
  /* Description */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Describe */
  if (item - 1 >= INVEN_WIELD)
    {
      msg_format("%^s: %s (%c).",
		 describe_use(item - 1), o_name, index_to_label(item - 1));
    }
  else if (item - 1 >= 0)
    {
      msg_format("In your pack: %s (%c).", o_name, 
		 index_to_label(item));
    }
}  

/**
 * Show all the id_other flags an object is going to get
 */
u32b flags_other(object_type *o_ptr)
{
  int j;
  u32b temp_flag, all_flags = 0x00000000; 

  /* Resists */
  for (j = 0; j < MAX_P_RES; j++)
    {
      temp_flag = OBJECT_ID_BASE_RESIST << j;
      if (o_ptr->percent_res[j] != RES_LEVEL_BASE)
	all_flags |= temp_flag;
    }
  
  /* Slays */
  for (j = 0; j < MAX_P_SLAY; j++)
    {
      temp_flag = OBJECT_ID_BASE_SLAY << j;
      if (o_ptr->multiple_slay[j] != MULTIPLE_BASE)
	all_flags |= temp_flag;
    }
  
  /* Brands */
  for (j = 0; j < MAX_P_BRAND; j++)
    {
      temp_flag = OBJECT_ID_BASE_BRAND << j;
      if (o_ptr->multiple_brand[j] != MULTIPLE_BASE)
	all_flags |= temp_flag ;
    }
  
  /* Others */
  if ((o_ptr->to_h) || is_weapon(o_ptr) || (o_ptr->flags_obj & OF_SHOW_MODS)) 
    all_flags |= IF_TO_H;
  if ((o_ptr->to_d) || is_weapon(o_ptr) || (o_ptr->flags_obj & OF_SHOW_MODS)) 
    all_flags |= IF_TO_D;
  if ((o_ptr->to_a) || is_armour(o_ptr)) all_flags |= IF_TO_A;
  if ((o_ptr->ac) || is_armour(o_ptr)) all_flags |= (IF_AC | IF_TO_A);
  all_flags |= IF_DD_DS;

  return all_flags;
}

/**
 * Does the item have stat or other bonuses?
 */
bool has_bonuses(object_type *o_ptr)
{
  int j;

  /* Affects stats? */
  for (j = 0; j < A_MAX; j++)
    if (o_ptr->bonus_stat[j] != 0) return TRUE;

  /* Affects something else? */
  for (j = 0; j < MAX_P_BONUS; j++)
    if (o_ptr->bonus_other[j] != 0) return TRUE;

  /* Doesn't look like it */
  return FALSE;
}

/**
 * Determine if all the properties of a wieldable item are known,
 * but it's not formally identified
 */
bool known_really(object_type *o_ptr)
{
  u32b otherflags = flags_other(o_ptr);
  bool needs_to_be_worn = has_bonuses(o_ptr);

  /* Any ego-item type must be known */
  if (o_ptr->name2) 
    if (!(e_info[o_ptr->name2].everseen)) return FALSE;

  /* Object flags must be known */
  if ((o_ptr->id_obj) != (o_ptr->flags_obj | o_ptr->id_obj)) return FALSE;

  /* Other flags must be known */
  if ((o_ptr->id_other) != (otherflags | o_ptr->id_other)) return FALSE;

  /* Objects with bonuses need to be worn to see the bonuses */
  if (needs_to_be_worn && (!(o_ptr->ident & IDENT_WORN))) return FALSE;

  /* No need to identify if it already has been */  
  if (o_ptr->ident & IDENT_KNOWN) return FALSE;

  /* Has to be wieldable */
  if (wield_slot(o_ptr) < 0) return FALSE;

  /* Must be OK */
  return TRUE;
}


/**
 * Notice random effect curses
 */
void notice_curse(u32b curse_flag, int item)
{
  object_type *o_ptr;
  int i;
  bool already_ego = FALSE;

  if (item) 
    {
      if (item > 0)
	o_ptr = &inventory[item - 1];
      else o_ptr = &o_list[0 - item];

      already_ego = has_ego_properties(o_ptr);

      if (o_ptr->flags_curse & curse_flag) 
	{
	  o_ptr->id_curse |= (o_ptr->flags_curse & curse_flag);
	  o_ptr->ident |= IDENT_CURSED;

	  /* Ego item? */
	  if (already_ego != has_ego_properties(o_ptr))
	    label_as_ego(o_ptr, item);
	}
      /* Fully identified now? */
      if (known_really(o_ptr)) identify_object(o_ptr);

      return;
    }

  for (i = INVEN_WIELD; i <= INVEN_FEET; i++)
    {
      o_ptr = &inventory[i];
      
      already_ego = has_ego_properties(o_ptr);

      /* Look for curses */
      if (o_ptr->flags_curse & curse_flag)
	{ 
	  /* Found one */
	  o_ptr->id_curse |= (o_ptr->flags_curse & curse_flag);
	  o_ptr->ident |= IDENT_CURSED;

	  /* Ego item? */
	  if (already_ego != has_ego_properties(o_ptr)) 
	    label_as_ego(o_ptr, item);
	}

      /* Fully identified now? */
      if (known_really(o_ptr)) identify_object(o_ptr);
    }
}

/**
 * Notice object flags
 */
void notice_obj(u32b obj_flag, int item)
{
  object_type *o_ptr;
  int i;
  bool already_ego = FALSE;

  if (item) 
    {
      if (item > 0)
	o_ptr = &inventory[item - 1];
      else o_ptr = &o_list[0 - item];
      
      already_ego = has_ego_properties(o_ptr);

      /* Add properties */
      o_ptr->id_obj |= (o_ptr->flags_obj & obj_flag);

      /* Get sensation based jewellery knowledge */
      if (((item - 1) == INVEN_RIGHT) || 
	  ((item - 1) == INVEN_LEFT) || 
	  ((item - 1) == INVEN_NECK))
	{
	  p_ptr->id_obj |= (o_ptr->flags_obj & obj_flag);
	}

      /* Ego item? */
      if (already_ego != has_ego_properties(o_ptr)) label_as_ego(o_ptr, item);

      /* Fully identified now? */
      if (known_really(o_ptr)) identify_object(o_ptr);
      
      return;
    }

  for (i = INVEN_WIELD; i <= INVEN_FEET; i++)
    {
      o_ptr = &inventory[i];
      
      already_ego = has_ego_properties(o_ptr);

      /* Add properties */
      o_ptr->id_obj |= (o_ptr->flags_obj & obj_flag);

      /* Get sensation based jewellery knowledge */
      if ((i == INVEN_RIGHT) || (i == INVEN_LEFT) || (i == INVEN_NECK))
	{
	  p_ptr->id_obj |= (o_ptr->flags_obj & obj_flag);
	}

      /* Ego item? */
      if (already_ego != has_ego_properties(o_ptr)) label_as_ego(o_ptr, item);

      /* Fully identified now? */
      if (known_really(o_ptr)) identify_object(o_ptr);
    }
}

/**
 * Notice other properties
 */
void notice_other(u32b other_flag, int item)
{
  int i, j;
  u32b temp_flag;
  bool already_ego = FALSE;
  object_type *o_ptr;

  if (item) 
    {
      if (item > 0) o_ptr = &inventory[item - 1];
      else o_ptr = &o_list[0 - item];

      already_ego = has_ego_properties(o_ptr);

      /* Resists */
      for (j = 0; j < MAX_P_RES; j++)
	{
	  temp_flag = OBJECT_ID_BASE_RESIST << j;
	  if (o_ptr->percent_res[j] != RES_LEVEL_BASE)
	    {
	      o_ptr->id_other |= (temp_flag & other_flag);
	      if (((item - 1) == INVEN_RIGHT) || 
		  ((item - 1) == INVEN_LEFT) || 
		  ((item - 1) == INVEN_NECK))
		{
		  p_ptr->id_other |= (temp_flag & other_flag);
		}
	    }
	}

      /* Slays */
      for (j = 0; j < MAX_P_SLAY; j++)
	{
	  temp_flag = OBJECT_ID_BASE_SLAY << j;
	  if (o_ptr->multiple_slay[j] != MULTIPLE_BASE)
	    {
	      o_ptr->id_other |= (temp_flag & other_flag);
	      if (((item - 1) == INVEN_RIGHT) || 
		  ((item - 1) == INVEN_LEFT) || 
		  ((item - 1) == INVEN_NECK))
		{
		  p_ptr->id_other |= (temp_flag & other_flag);
		}
	    }
	}

      /* Brands */
      for (j = 0; j < MAX_P_BRAND; j++)
	{
	  temp_flag = OBJECT_ID_BASE_BRAND << j;
	  if (o_ptr->multiple_brand[j] != MULTIPLE_BASE)
	    {
	      o_ptr->id_other |= (temp_flag & other_flag);
	      if (((item - 1) == INVEN_RIGHT) || 
		  ((item - 1) == INVEN_LEFT) || 
		  ((item - 1) == INVEN_NECK))
		{
		  p_ptr->id_other |= (temp_flag & other_flag);
		}
	    }
	}

      /* Others */
      if ((other_flag & IF_TO_H) && ((o_ptr->to_h) || is_weapon(o_ptr))) 
	o_ptr->id_other |= IF_TO_H;
      if ((other_flag & IF_TO_D) && ((o_ptr->to_d) || is_weapon(o_ptr))) 
	o_ptr->id_other |= IF_TO_D;
      if ((other_flag & IF_TO_A) && ((o_ptr->ac) || is_armour(o_ptr))) 
	o_ptr->id_other |= IF_TO_A;
      if ((other_flag & IF_AC) && ((o_ptr->ac) || is_armour(o_ptr)))
	o_ptr->id_other |= IF_AC;
      if (other_flag & IF_DD_DS) o_ptr->id_other |= IF_DD_DS;

      /* Ego item? */
      if (already_ego != has_ego_properties(o_ptr)) label_as_ego(o_ptr, item);

      /* Fully identified now? */
      if (known_really(o_ptr)) identify_object(o_ptr);

      return;
    }
  
  for (i = INVEN_WIELD; i <= INVEN_FEET; i++)
    {
      o_ptr = &inventory[i];
      
      already_ego = has_ego_properties(o_ptr);

      /* Resists */
      for (j = 0; j < MAX_P_RES; j++)
	{
	  temp_flag = OBJECT_ID_BASE_RESIST << j;
	  if (o_ptr->percent_res[j] != RES_LEVEL_BASE)
	    {
	      o_ptr->id_other |= (temp_flag & other_flag);
	      if ((j == INVEN_RIGHT) || (j == INVEN_LEFT) || (j == INVEN_NECK))
		{
		  p_ptr->id_other |= (temp_flag & other_flag);
		}
	    }
	}

      /* Slays */
      for (j = 0; j < MAX_P_SLAY; j++)
	{
	  temp_flag = OBJECT_ID_BASE_SLAY << j;
	  if (o_ptr->multiple_slay[j] != MULTIPLE_BASE)
	    {
	      o_ptr->id_other |= (temp_flag & other_flag);
	      if ((j == INVEN_RIGHT) || (j == INVEN_LEFT) || (j == INVEN_NECK))
		{
		  p_ptr->id_other |= (temp_flag & other_flag);
		}
	    }
	}

      /* Brands */
      for (j = 0; j < MAX_P_BRAND; j++)
	{
	  temp_flag = OBJECT_ID_BASE_BRAND << j;
	  if (o_ptr->multiple_brand[j] != MULTIPLE_BASE)
	    {
	      o_ptr->id_other |= (temp_flag & other_flag);
	      if ((j == INVEN_RIGHT) || (j == INVEN_LEFT) || (j == INVEN_NECK))
		{
		  p_ptr->id_other |= (temp_flag & other_flag);
		}
	    }
	}

      /* Others */
      if ((other_flag & IF_TO_H) && ((o_ptr->to_h) || is_weapon(o_ptr))) 
	o_ptr->id_other |= IF_TO_H;
      if ((other_flag & IF_TO_D) && ((o_ptr->to_d) || is_weapon(o_ptr))) 
	o_ptr->id_other |= IF_TO_D;
      if ((other_flag & IF_TO_A) && ((o_ptr->ac) || is_armour(o_ptr))) 
	o_ptr->id_other |= IF_TO_A;
      if ((other_flag & IF_AC) && ((o_ptr->ac) || is_armour(o_ptr)))
	o_ptr->id_other |= IF_AC;
      if (other_flag & IF_DD_DS) o_ptr->id_other |= IF_DD_DS;

      /* Ego item? */
      if (already_ego != has_ego_properties(o_ptr)) label_as_ego(o_ptr, item);

      /* Fully identified now? */
      if (known_really(o_ptr)) identify_object(o_ptr);
    }
}









