/** \file object2.c 
    \brief Object generation and management

 * Object stacks, compaction, generation, IDing, "trying", pricing, 
 * stacking.  Creating special artifacts, Adding charges to wands and 
 * staffs, bonuses to weapons and armour, powers and higher pvals to 
 * amulets and rings, fuel to lights, trap difficulty to chests, creating
 * ego items, what consititutes "cursed", "good", and "great" items,
 * generate objects (inc. Acquirement code) and treasures, object &
 * inventory routines, inventory sorting, equipment.
 *
 * Copyright (c) 2009 Nick McConnell, Si Griffin, Leon Marrick & Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
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
 * Excise a dungeon object from any stacks
 */
void excise_object_idx(int o_idx)
{
  object_type *j_ptr;
  
  s16b this_o_idx, next_o_idx = 0;
  
  s16b prev_o_idx = 0;
  
  
  /* Object */
  j_ptr = &o_list[o_idx];
  
  /* Monster */
  if (j_ptr->held_m_idx)
    {
      monster_type *m_ptr;
      
      /* Monster */
      m_ptr = &m_list[j_ptr->held_m_idx];
      
      /* Scan all objects in the grid */
      for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
	  object_type *o_ptr;
	  
	  /* Acquire object */
	  o_ptr = &o_list[this_o_idx];
	  
	  /* Acquire next object */
	  next_o_idx = o_ptr->next_o_idx;
	  
	  /* Done */
	  if (this_o_idx == o_idx)
	    {
	      /* No previous */
	      if (prev_o_idx == 0)
		{
		  /* Remove from list */
		  m_ptr->hold_o_idx = next_o_idx;
		}
	      
	      /* Real previous */
	      else
		{
		  object_type *i_ptr;
		  
		  /* Previous object */
		  i_ptr = &o_list[prev_o_idx];
		  
		  /* Remove from list */
		  i_ptr->next_o_idx = next_o_idx;
		}
	      
	      /* Forget next pointer */
	      o_ptr->next_o_idx = 0;
	      
	      /* Done */
	      break;
			}
	  
	  /* Save prev_o_idx */
	  prev_o_idx = this_o_idx;
	}
    }
  
  /* Dungeon */
  else
    {
      int y = j_ptr->iy;
      int x = j_ptr->ix;
      
      /* Scan all objects in the grid */
      for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
	  object_type *o_ptr;
	  
	  /* Acquire object */
	  o_ptr = &o_list[this_o_idx];
	  
	  /* Acquire next object */
	  next_o_idx = o_ptr->next_o_idx;
	  
	  /* Done */
	  if (this_o_idx == o_idx)
	    {
	      /* No previous */
	      if (prev_o_idx == 0)
		{
		  /* Remove from list */
		  cave_o_idx[y][x] = next_o_idx;
		}
	      
	      /* Real previous */
	      else
		{
		  object_type *i_ptr;
		  
		  /* Previous object */
		  i_ptr = &o_list[prev_o_idx];
		  
		  /* Remove from list */
		  i_ptr->next_o_idx = next_o_idx;
		}
	      
	      /* Forget next pointer */
	      o_ptr->next_o_idx = 0;
	      
	      /* Done */
	      break;
	    }
	  
	  /* Save prev_o_idx */
	  prev_o_idx = this_o_idx;
	}
    }
}


/**
 * Delete a dungeon object
 *
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(int o_idx)
{
  object_type *j_ptr;
  
  /* Excise */
  excise_object_idx(o_idx);
  
  /* Object */
  j_ptr = &o_list[o_idx];
  
  /* Dungeon floor */
  if (!(j_ptr->held_m_idx))
    {
      int y, x;
      
      /* Location */
      y = j_ptr->iy;
      x = j_ptr->ix;
      
      /* Visual update */
      lite_spot(y, x);
    }
  
  /* Wipe the object */
  object_wipe(j_ptr);
  
  /* Count objects */
  o_cnt--;

  /* Update item list */
  p_ptr->window |= PW_ITEMLIST;
}


/**
 * Deletes all objects at given location
 */
void delete_object(int y, int x)
{
  s16b this_o_idx, next_o_idx = 0;
  
  
  /* Paranoia */
  if (!in_bounds(y, x)) return;
  
  
  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Wipe the object */
      object_wipe(o_ptr);
      
      /* Count objects */
      o_cnt--;
    }
  
  /* Objects are gone */
  cave_o_idx[y][x] = 0;
  
  /* Visual update */
  lite_spot(y, x);

  /* Update item list */
  p_ptr->window |= PW_ITEMLIST;
}



/**
 * Move an object from index i1 to index i2 in the object list
 */
static void compact_objects_aux(int i1, int i2)
{
  int i;
  
  object_type *o_ptr;
  
  
  /* Do nothing */
  if (i1 == i2) return;
  
  
  /* Repair objects */
  for (i = 1; i < o_max; i++)
    {
      /* Acquire object */
      o_ptr = &o_list[i];
      
      /* Skip "dead" objects */
      if (!o_ptr->k_idx) continue;
      
      /* Repair "next" pointers */
      if (o_ptr->next_o_idx == i1)
	{
	  /* Repair */
	  o_ptr->next_o_idx = i2;
	}
    }
  
  
  /* Acquire object */
  o_ptr = &o_list[i1];
  
  
  /* Monster */
  if (o_ptr->held_m_idx)
    {
      monster_type *m_ptr;
      
      /* Acquire monster */
      m_ptr = &m_list[o_ptr->held_m_idx];
      
      /* Repair monster */
      if (m_ptr->hold_o_idx == i1)
	{
	  /* Repair */
	  m_ptr->hold_o_idx = i2;
	}
    }
  
  /* Dungeon */
  else
    {
      int y, x;
      
      /* Acquire location */
      y = o_ptr->iy;
      x = o_ptr->ix;
      
      /* Repair grid */
      if (cave_o_idx[y][x] == i1)
	{
	  /* Repair */
	  cave_o_idx[y][x] = i2;
	}
    }
  
  
  /* Hack -- move object */
  (void)COPY(&o_list[i2], &o_list[i1], object_type);
  
  /* Hack -- wipe hole */
  object_wipe(o_ptr);
}


/**
 * Compact and Reorder the object list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" objects, we base the saving throw on a
 * combination of object level, distance from player, and current
 * "desperation".  
 *
 * Worthless, aware objects are always deleted in the 
 * first round. -LM-
 *
 * After "compacting" (if needed), we "reorder" the objects into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_objects(int size)
{
  int py = p_ptr->py;
  int px = p_ptr->px;

  int i, y, x, num, cnt;
  
  int cur_lev, cur_dis, chance;
  
  
  /* Compact */
  if (size)
    {
      /* Message */
      msg_print("Compacting objects...");
      
      /* Redraw map */
      p_ptr->redraw |= (PR_MAP);
      
      /* Window stuff */
      p_ptr->window |= (PW_OVERHEAD);
    }
  
  
  /* First do gold */
  for (i = 1; (i < o_max) && (size); i++)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Nuke gold or squelched items */
      if (o_ptr->tval == TV_GOLD || squelch_item_ok(o_ptr))
	{
	  delete_object_idx(i);
	  size--;
	}
    }

  /* Compact at least 'size' objects */
  for (num = 0, cnt = 1; num < size; cnt++)
    {
      /* Get more vicious each iteration */
      cur_lev = 5 * cnt;
      
      /* Get closer each iteration */
      cur_dis = 5 * (20 - cnt);
      
      /* Examine the objects */
      for (i = 1; i < o_max; i++)
	{
	  object_type *o_ptr = &o_list[i];
	  
	  object_kind *k_ptr = &k_info[o_ptr->k_idx];
	  
	  /* Skip dead objects */
	  if (!o_ptr->k_idx) continue;
	  
	  /* Hack -- immediately delete all intrinsically worthless (or 
	   * almost worthless) objects that the player is already aware 
	   * of.  This statement emphasizes speed over smarts. -LM-
	   */
	  if ((cnt == 1) && (object_aware_p(o_ptr)) && (k_ptr->cost < 3)) 
	    {
	      /* Delete the object. */
	      delete_object_idx(i);
	      
	      /* Count it. */
	      num++;
	      
	      /* Continue to the next object. */
	      continue;
	    }
	  
	  /* Hack -- High level objects start out "immune" */
	  if (k_ptr->level > cur_lev && !k_ptr->squelch) continue;
	  
	  /* Monster */
	  if (o_ptr->held_m_idx)
	    {
	      monster_type *m_ptr;
	      
	      /* Acquire monster */
	      m_ptr = &m_list[o_ptr->held_m_idx];
	      
	      /* Get the location */
	      y = m_ptr->fy;
	      x = m_ptr->fx;
	      
	      /* Monsters protect their objects */
	      if ((rand_int(100) < 90) && !k_ptr->squelch) continue;
	    }
	  
	  /* Dungeon */
	  else
	    {
	      /* Get the location */
	      y = o_ptr->iy;
	      x = o_ptr->ix;
	    }
	  
	  /* Nearby objects start out "immune" */
	  if ((cur_dis > 0) && (distance(py, px, y, x) < cur_dis) && 
	      !k_ptr->squelch) continue;
	  
	  /* Saving throw */
	  chance = 90;
	  
	  /* Hack -- only compact artifacts in emergencies */
	  if (artifact_p(o_ptr) && (cnt < 1000)) chance = 100;
	  
	  /* Apply the saving throw */
	  if (rand_int(100) < chance) continue;
	  
	  /* Delete the object */
	  delete_object_idx(i);
	  
	  /* Count it */
	  num++;
	}
    }
  
  
  /* Excise dead objects (backwards!) */
  for (i = o_max - 1; i >= 1; i--)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Skip real objects */
      if (o_ptr->k_idx) continue;
      
      /* Move last object into open hole */
      compact_objects_aux(o_max - 1, i);
      
      /* Compress "o_max" */
      o_max--;
    }

  /* Update item list */
  p_ptr->window |= PW_ITEMLIST;
}


/**
 * Delete all the items when player leaves the level
 *
 * Note -- we do NOT visually reflect these (irrelevant) changes
 *
 * Hack -- we clear the "cave_o_idx[y][x]" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 */
void wipe_o_list(void)
{
  int i;
  
  /* Delete the existing objects */
  for (i = 1; i < o_max; i++)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Skip dead objects */
      if (!o_ptr->k_idx) continue;
      
      /* Hack -- Preserve unknown artifacts */
      if (artifact_p(o_ptr) && !object_known_p(o_ptr))
	{
	  /* Mega-Hack -- Preserve the artifact */
	  a_info[o_ptr->name1].creat_turn = 0;
	}
	
      
      /* Monster */
      if (o_ptr->held_m_idx)
	{
	  monster_type *m_ptr;
	  
	  /* Monster */
	  m_ptr = &m_list[o_ptr->held_m_idx];

	  /* Hack -- see above */
	  m_ptr->hold_o_idx = 0;
	}
      
      /* Dungeon */
      else
	{
	  /* Access location */
	  int y = o_ptr->iy;
	  int x = o_ptr->ix;
	  
	  /* Hack -- see above */
	  cave_o_idx[y][x] = 0;
	}
      
      /* Wipe the object */
      (void)WIPE(o_ptr, object_type);
    }
  
  /* Reset "o_max" */
  o_max = 1;
  
  /* Reset "o_cnt" */
  o_cnt = 0;
}


/**
 * Acquires and returns the index of a "free" object.
 *
 * This routine should almost never fail, but in case it does,
 * we must be sure to handle "failure" of this routine.
 */
s16b o_pop(void)
{
  int i;
  
  
  /* Initial allocation */
  if (o_max < z_info->o_max)
    {
      /* Get next space */
      i = o_max;
      
      /* Expand object array */
      o_max++;
      
      /* Count objects */
      o_cnt++;
      
      /* Use this object */
      return (i);
    }
  
  
  /* Recycle dead objects */
  for (i = 1; i < o_max; i++)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[i];
      
      /* Skip live objects */
      if (o_ptr->k_idx) continue;
      
      /* Count objects */
      o_cnt++;
      
      /* Use this object */
      return (i);
    }
  
  
  /* Warn the player (except during dungeon creation) */
  if (character_dungeon) msg_print("Too many objects!");
  
  /* Oops */
  return (0);
}


/**
 * Apply a "object restriction function" to the "object allocation table"
 */
errr get_obj_num_prep(void)
{
  int i;
  
  /* Get the entry */
  alloc_entry *table = alloc_kind_table;
  
  /* Scan the allocation table */
  for (i = 0; i < alloc_kind_size; i++)
    {
      /* Accept objects which pass the restriction, if any */
      if (!get_obj_num_hook || (*get_obj_num_hook)(table[i].index))
	{
	  /* Accept this object */
	  table[i].prob2 = table[i].prob1;
	}
      
      /* Do not use this object */
      else
	{
	  /* Decline this object */
	  table[i].prob2 = 0;
	}
    }
  
  /* Success */
  return (0);
}



/**
 * Choose an object kind that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "object allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" object, in
 * a relatively efficient manner.
 *
 * It is (slightly) more likely to acquire an object of the given level
 * than one of a lower level.  This is done by choosing several objects
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no objects are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_obj_num(int level)
{
  int i, j, p;
  
  int k_idx;
  
  long value, total;
  
  object_kind *k_ptr;
  
  alloc_entry *table = alloc_kind_table;
  
  
  /* Boost level */
  if (level > 0)
    {
      /* Occasional boost to allowed level.  Less generous in Oangband. */
      if (rand_int(GREAT_OBJ) == 0) level += randint(20) + randint(level / 2);
    }
  
  
  /* Reset total */
  total = 0L;
  
  /* Process probabilities */
  for (i = 0; i < alloc_kind_size; i++)
    {
      /* Objects are sorted by depth */
      if (table[i].level > level) break;
      
      /* Default */
      table[i].prob3 = 0;
      
      /* Access the index */
      k_idx = table[i].index;
      
      /* Access the actual kind */
      k_ptr = &k_info[k_idx];
      
      /* Hack -- prevent embedded chests */
      if (opening_chest && (k_ptr->tval == TV_CHEST)) continue;
      
      /* Accept */
      table[i].prob3 = table[i].prob2;
      
      /* Total */
      total += table[i].prob3;
    }
  
  /* No legal objects */
  if (total <= 0) return (0);
  
  
  /* Pick an object */
  value = rand_int(total);
  
  /* Find the object */
  for (i = 0; i < alloc_kind_size; i++)
    {
      /* Found the entry */
      if (value < table[i].prob3) break;
      
      /* Decrement */
      value = value - table[i].prob3;
    }
  
  
  /* Power boost */
  p = rand_int(100);
  
  /* Hack -- chests should have decent stuff, and chests should themselves 
   * be decent, so we guarantee two more tries for better objects. -LM-
   */
  if ((opening_chest) || (required_tval == TV_CHEST))  p = 0;

  /* Try for a "better" object once (60%) or twice (10%) */
  if (p < 60)
    {
      /* Save old */
      j = i;
      
      /* Pick an object */
      value = rand_int(total);
      
      /* Find the object */
      for (i = 0; i < alloc_kind_size; i++)
	{
	  /* Found the entry */
	  if (value < table[i].prob3) break;
	  
	  /* Decrement */
	  value = value - table[i].prob3;
	}
      
      /* Keep the "best" one */
      if (table[i].level < table[j].level) i = j;
    }
  
  /* Try for a "better" object twice (10%) */
  if (p < 10)
    {
      /* Save old */
      j = i;
      
      /* Pick an object */
      value = rand_int(total);
      
      /* Find the object */
      for (i = 0; i < alloc_kind_size; i++)
	{
	  /* Found the entry */
	  if (value < table[i].prob3) break;
	  
	  /* Decrement */
	  value = value - table[i].prob3;
	}
      
      /* Keep the "best" one */
      if (table[i].level < table[j].level) i = j;
    }

  /* Hack - keep level for jewellery generation */
  j_level = table[i].level;
  
  /* Result */
  return (table[i].index);
}



/**
 * Known is true when the "attributes" of an object are "known".
 * These include tohit, todam, toac, cost, and pval (charges, bonuses, etc.).
 *
 * Note that "knowing" an object gives you everything that an "awareness"
 * gives you, and much more.  In fact, the player is always "aware" of any
 * item of which he has full "knowledge".
 *
 * But having full knowledge of, say, one "wand of wonder", does not, by
 * itself, give you knowledge, or even awareness, of other "wands of wonder".
 * It happens that most "identify" routines (including "buying from a shop")
 * will make the player "aware" of the object as well as fully "know" it.
 *
 * This routine also removes any inscriptions generated by "feelings".
 */
void object_known(object_type *o_ptr)
{
  /* Forget the feeling */
  o_ptr->feel = FEEL_NONE;
  
  /* Clear the "Felt" info */
  o_ptr->ident &= ~(IDENT_SENSE);
  
  /* Clear the "Empty" info */
  o_ptr->ident &= ~(IDENT_EMPTY);
  
  /* Now we know about the item */
  o_ptr->ident |= (IDENT_KNOWN);
  
  /* ID knowledge is better than wearing knowledge */
  o_ptr->ident |= (IDENT_WORN);

  /* Get all the id flags */
  o_ptr->id_obj = o_ptr->flags_obj;
  o_ptr->id_other = flags_other(o_ptr);
}





/**
 * The player is now aware of the effects of the given object.
 */
void object_aware(object_type *o_ptr)
{
  /* Fully aware of the effects */
  k_info[o_ptr->k_idx].aware = TRUE;
}



/**
 * Something has been "sampled"
 */
void object_tried(object_type *o_ptr)
{
  /* Mark it as tried (even if "aware") */
  k_info[o_ptr->k_idx].tried = TRUE;
}



/**
 * Return the "value" of an "unknown" item
 * Make a guess at the value of non-aware items
 */
static s32b object_value_base(object_type *o_ptr)
{
  object_kind *k_ptr = &k_info[o_ptr->k_idx];
  
  /* For most items, the "aware" value is simply that of the object kind. */
  s32b aware_cost = k_ptr->cost;
  
  /* Because weapons and ammo may have enhanced damage dice, 
   * their aware value may vary. -LM-
   */
  switch (o_ptr->tval)
    {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	if (o_ptr->ds > k_ptr->ds)
	  {
	    aware_cost += (1 + o_ptr->ds - k_ptr->ds) * 
	      (2 + o_ptr->ds - k_ptr->ds) * 1L;
	  }
	if (o_ptr->dd > k_ptr->dd)
	  {
	    aware_cost += (1 + o_ptr->dd - k_ptr->dd) * 
	      (2 + o_ptr->dd - k_ptr->dd) * 1L;
	  }
	break;
      }
      
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
      {
	if (o_ptr->ds > k_ptr->ds)
	  {
	    aware_cost += (o_ptr->ds - k_ptr->ds) * 
	      (o_ptr->ds - k_ptr->ds) * 
	      o_ptr->dd * o_ptr->dd * 16L;
	  }
	if (o_ptr->dd > k_ptr->dd)
	  {
	    aware_cost += (o_ptr->dd - k_ptr->dd) * 
	      (o_ptr->ds - k_ptr->dd) * 
	      o_ptr->ds * o_ptr->ds * 16L;
	  }
	break;
      }
    }
  
  /* Aware item -- use template cost, modified by enhanced dice if needed. */
  if (object_aware_p(o_ptr)) return (aware_cost);
  
  /* Analyze the type */
  switch (o_ptr->tval)
    {
      /* Un-aware Food */
    case TV_FOOD: return (5L);
      
      /* Un-aware Potions */
    case TV_POTION: return (20L);
      
      /* Un-aware Scrolls */
    case TV_SCROLL: return (20L);
      
      /* Un-aware Staffs */
    case TV_STAFF: return (80L);
      
      /* Un-aware Wands */
    case TV_WAND: return (80L);
      
      /* Un-aware Rods */
    case TV_ROD: return (200L);
      
      /* Un-aware Rings (shouldn't happen) */
    case TV_RING: return (45L);
      
      /* Un-aware Amulets (shouldn't happen) */
    case TV_AMULET: return (45L);
    }
  
  /* Paranoia -- Oops */
  return (0L);
}


/**
 * Return the "real" price of a "known" item, not including discounts
 *
 * Wand and staffs get cost for each charge
 *
 * Armor is worth an extra 100 gold per bonus point to armor class.
 *
 * Weapons are worth an extra 100 gold per bonus point (AC,TH,TD).
 *
 * Missiles are only worth 5 gold per bonus point, since they
 * usually appear in groups of 20, and we want the player to get
 * the same amount of cash for any "equivalent" item.  Note that
 * missiles never have any of the "pval" flags, and in fact, they
 * only have a few of the available flags, primarily of the "slay"
 * and "brand" and "ignore" variety.
 *
 * Now reworked to handle all item properties -NRM-
 */
static s32b object_value_real(object_type *o_ptr)
{
  int i;

  s32b value;
  
  object_kind *k_ptr = &k_info[o_ptr->k_idx];
  
  
  /* Hack -- "worthless" items */
  if (!k_ptr->cost) return (0L);
  
  /* Base cost */
  value = k_ptr->cost;
  
  
  /* Artifact */
  if (o_ptr->name1)
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      
      /* Hack -- Use the artifact cost instead */
      return (a_ptr->cost);
    }
  
  /* Ego-Item */
  else if (has_ego_properties(o_ptr))
    {
      ego_item_type *e_ptr = &e_info[o_ptr->name2];
      
      /* Hack -- Reward the ego-item with a bonus */
      value += e_ptr->cost;
    }
  
  /* Analyze resists, bonuses and multiples */
  switch (o_ptr->tval)
    {
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_LITE:
    case TV_AMULET:
    case TV_RING:
      {

	/* Give credit for resists */
	if (o_ptr->id_other & IF_RES_ACID)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_ACID]) * 60L;
	if (o_ptr->id_other & IF_RES_ELEC)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_ELEC]) * 60L;
	if (o_ptr->id_other & IF_RES_FIRE)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_FIRE]) * 60L;
	if (o_ptr->id_other & IF_RES_COLD)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_COLD]) * 60L;
	if (o_ptr->id_other & IF_RES_POIS)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_POIS]) * 120L;
	if (o_ptr->id_other & IF_RES_LITE)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_LITE]) * 60L;
	if (o_ptr->id_other & IF_RES_DARK)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_DARK]) * 60L;
	if (o_ptr->id_other & IF_RES_CONFU)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_CONFU]) * 90L;
	if (o_ptr->id_other & IF_RES_SOUND)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_SOUND]) * 60L;
	if (o_ptr->id_other & IF_RES_SHARD)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_SHARD]) * 60L;
	if (o_ptr->id_other & IF_RES_NEXUS)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_NEXUS]) * 60L;
	if (o_ptr->id_other & IF_RES_NETHR)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_NETHR]) * 90L;
	if (o_ptr->id_other & IF_RES_CHAOS)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_CHAOS]) * 90L;
	if (o_ptr->id_other & IF_RES_DISEN)
	  value += (RES_LEVEL_BASE - o_ptr->percent_res[P_RES_DISEN]) * 120L;

	/* Wearing shows all bonuses */
	if (o_ptr->ident & IDENT_WORN)
	  {
	    /* Give credit for stat bonuses. */
	    for (i = 0; i < A_MAX; i++)
	      {
		if (o_ptr->bonus_stat[i] > BONUS_BASE) 
		  value += (((o_ptr->bonus_stat[i] + 1) * 
			     (o_ptr->bonus_stat[i] + 1)) * 100L);
		else if (o_ptr->bonus_stat[i] < BONUS_BASE) 
		  value -= (o_ptr->bonus_stat[i] * o_ptr->bonus_stat[i] * 100L);
	      }
	    
	    /* Give credit for magic items bonus. */
	    value += (o_ptr->bonus_other[P_BONUS_M_MASTERY] * 600L);
	    
	    /* Give credit for stealth and searching */
	    value += (o_ptr->bonus_other[P_BONUS_STEALTH] * 100L);
	    value += (o_ptr->bonus_other[P_BONUS_SEARCH] * 40L);
	    
	    /* Give credit for infra-vision */
	    value += (o_ptr->bonus_other[P_BONUS_INFRA] * 50L);
	    
	    /* Give credit for tunneling bonus above that which a	
	     * digger possesses intrinsically.
	     */
	    value += ((o_ptr->bonus_other[P_BONUS_TUNNEL] - 
		       k_ptr->bonus_other[P_BONUS_TUNNEL]) * 50L);
      
      
	    /* Give credit for speed bonus.   Formula changed to avoid
	     * excessively valuable low-speed items.
	     */
	    if (o_ptr->bonus_other[P_BONUS_SPEED] > 0)
	      value += (o_ptr->bonus_other[P_BONUS_SPEED] * 
			o_ptr->bonus_other[P_BONUS_SPEED] * 5000L);
	    else if (o_ptr->bonus_other[P_BONUS_SPEED] < 0)
	      value -= (o_ptr->bonus_other[P_BONUS_SPEED] * 
			o_ptr->bonus_other[P_BONUS_SPEED] * 2000L);
	    
	    /* Give credit for extra shots */
	    value += (o_ptr->bonus_other[P_BONUS_SHOTS] * 8000L);
	    
	    /* Give credit for extra might */
	    value += (o_ptr->bonus_other[P_BONUS_MIGHT] * 6000L);
	  }	

	/* Give credit for slays */
	if (o_ptr->id_other & IF_SLAY_ANIMAL)
	  value += (o_ptr->multiple_slay[P_SLAY_ANIMAL] - MULTIPLE_BASE) * 150L;
	if (o_ptr->id_other & IF_SLAY_EVIL)
	  value += (o_ptr->multiple_slay[P_SLAY_EVIL]   - MULTIPLE_BASE) * 500L;
	if (o_ptr->id_other & IF_SLAY_UNDEAD)
	  value += (o_ptr->multiple_slay[P_SLAY_UNDEAD] - MULTIPLE_BASE) * 200L;
	if (o_ptr->id_other & IF_SLAY_DEMON)
	  value += (o_ptr->multiple_slay[P_SLAY_DEMON]  - MULTIPLE_BASE) * 200L;
	if (o_ptr->id_other & IF_SLAY_ORC)
	  value += (o_ptr->multiple_slay[P_SLAY_ORC]    - MULTIPLE_BASE) * 100L;
	if (o_ptr->id_other & IF_SLAY_TROLL)
	  value += (o_ptr->multiple_slay[P_SLAY_TROLL]  - MULTIPLE_BASE) * 150L;
	if (o_ptr->id_other & IF_SLAY_GIANT)
	  value += (o_ptr->multiple_slay[P_SLAY_GIANT]  - MULTIPLE_BASE) * 100L;
	if (o_ptr->id_other & IF_SLAY_DRAGON)
	  value += (o_ptr->multiple_slay[P_SLAY_DRAGON] - MULTIPLE_BASE) * 200L;
      
	/* Give credit for brands */
	if (o_ptr->id_other & IF_BRAND_ACID)
	  value += (o_ptr->multiple_brand[P_BRAND_ACID] - MULTIPLE_BASE) * 300L;
	if (o_ptr->id_other & IF_BRAND_ELEC)
	  value += (o_ptr->multiple_brand[P_BRAND_ELEC] - MULTIPLE_BASE) * 300L;
	if (o_ptr->id_other & IF_BRAND_FIRE)
	  value += (o_ptr->multiple_brand[P_BRAND_FIRE] - MULTIPLE_BASE) * 200L;
	if (o_ptr->id_other & IF_BRAND_COLD)
	  value += (o_ptr->multiple_brand[P_BRAND_COLD] - MULTIPLE_BASE) * 200L;
	if (o_ptr->id_other & IF_BRAND_POIS)
	  value += (o_ptr->multiple_brand[P_BRAND_POIS] - MULTIPLE_BASE) * 150L;

	/* Give credit for object flags */
	if (o_ptr->id_obj & OF_SUSTAIN_STR)    value += 500L;
	if (o_ptr->id_obj & OF_SUSTAIN_INT)    value += 300L;
	if (o_ptr->id_obj & OF_SUSTAIN_WIS)    value += 300L;
	if (o_ptr->id_obj & OF_SUSTAIN_DEX)    value += 400L;
	if (o_ptr->id_obj & OF_SUSTAIN_CON)    value += 400L;
	if (o_ptr->id_obj & OF_SUSTAIN_CHR)    value += 50L;
	if (o_ptr->id_obj & OF_SLOW_DIGEST) value += 200L;
	if (o_ptr->id_obj & OF_FEATHER)     value += 200L;
	if (o_ptr->id_obj & OF_REGEN)       value += 400L;
	if (o_ptr->id_obj & OF_TELEPATHY)   value += 5000L;
	if (o_ptr->id_obj & OF_SEE_INVIS)   value += 700L;
	if (o_ptr->id_obj & OF_FREE_ACT)    value += 800L;
	if (o_ptr->id_obj & OF_HOLD_LIFE)   value += 700L;
	if (o_ptr->id_obj & OF_SEEING)      value += 700L;
	if (o_ptr->id_obj & OF_FEARLESS)    value += 500L;
	if (o_ptr->id_obj & OF_LITE)        value += 300L;
	if (o_ptr->id_obj & OF_BLESSED)     value += 1000L;
	if (o_ptr->id_obj & OF_IMPACT)      value += 50L;
	if (o_ptr->id_obj & OF_ACID_PROOF)  value += 100L;
	if (o_ptr->id_obj & OF_ELEC_PROOF)  value += 100L;
	if (o_ptr->id_obj & OF_FIRE_PROOF)  value += 100L;
	if (o_ptr->id_obj & OF_COLD_PROOF)  value += 100L;
	if (o_ptr->id_obj & OF_DARKNESS)    value += 1000L;

	/* Give 'credit' for curse flags */
	if (o_ptr->id_curse & CF_TELEPORT)       value -= 200L;
	if (o_ptr->id_curse & CF_NO_TELEPORT)    value -= 1000L;
	if (o_ptr->id_curse & CF_AGGRO_PERM)     value -= 2000L;
	if (o_ptr->id_curse & CF_AGGRO_RAND)     value -= 700L;
	if (o_ptr->id_curse & CF_SLOW_REGEN)     value -= 200L;
	if (o_ptr->id_curse & CF_AFRAID)         value -= 500L;
	if (o_ptr->id_curse & CF_HUNGRY)         value -= 200L;
	if (o_ptr->id_curse & CF_POIS_RAND)      value -= 200L;
	if (o_ptr->id_curse & CF_POIS_RAND_BAD)  value -= 500L;
	if (o_ptr->id_curse & CF_CUT_RAND)       value -= 200L;
	if (o_ptr->id_curse & CF_CUT_RAND_BAD)   value -= 400L;
	if (o_ptr->id_curse & CF_HALLU_RAND)     value -= 800L;
	if (o_ptr->id_curse & CF_DROP_WEAPON)    value -= 500L;
	if (o_ptr->id_curse & CF_ATTRACT_DEMON)  value -= 800L;
	if (o_ptr->id_curse & CF_ATTRACT_UNDEAD) value -= 900L;
	if (o_ptr->id_curse & CF_STICKY_WIELD)   value -= 2500L;
	if (o_ptr->id_curse & CF_STICKY_CARRY)   value -= 1000L;
	if (o_ptr->id_curse & CF_PARALYZE)       value -= 800L;
	if (o_ptr->id_curse & CF_PARALYZE_ALL)   value -= 2000L;
	if (o_ptr->id_curse & CF_DRAIN_EXP)      value -= 800L;
	if (o_ptr->id_curse & CF_DRAIN_MANA)     value -= 1000L;
	if (o_ptr->id_curse & CF_DRAIN_STAT)     value -= 1500L;
	if (o_ptr->id_curse & CF_DRAIN_CHARGE)   value -= 1500L;

	break;
      }
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	/* Give credit for slays */
	if (o_ptr->id_other & IF_SLAY_ANIMAL)
	  value += (o_ptr->multiple_slay[P_SLAY_ANIMAL] - MULTIPLE_BASE) * 15L;
	if (o_ptr->id_other & IF_SLAY_EVIL)
	  value += (o_ptr->multiple_slay[P_SLAY_EVIL]   - MULTIPLE_BASE) * 50L;
	if (o_ptr->id_other & IF_SLAY_UNDEAD)
	  value += (o_ptr->multiple_slay[P_SLAY_UNDEAD] - MULTIPLE_BASE) * 20L;
	if (o_ptr->id_other & IF_SLAY_DEMON)
	  value += (o_ptr->multiple_slay[P_SLAY_DEMON]  - MULTIPLE_BASE) * 20L;
	if (o_ptr->id_other & IF_SLAY_ORC)
	  value += (o_ptr->multiple_slay[P_SLAY_ORC]    - MULTIPLE_BASE) * 10L;
	if (o_ptr->id_other & IF_SLAY_TROLL)
	  value += (o_ptr->multiple_slay[P_SLAY_TROLL]  - MULTIPLE_BASE) * 15L;
	if (o_ptr->id_other & IF_SLAY_GIANT)
	  value += (o_ptr->multiple_slay[P_SLAY_GIANT]  - MULTIPLE_BASE) * 10L;
	if (o_ptr->id_other & IF_SLAY_DRAGON)
	  value += (o_ptr->multiple_slay[P_SLAY_DRAGON] - MULTIPLE_BASE) * 20L;
      
	/* Give credit for brands */
	if (o_ptr->id_other & IF_BRAND_ACID)
	  value += (o_ptr->multiple_brand[P_BRAND_ACID] - MULTIPLE_BASE) * 30L;
	if (o_ptr->id_other & IF_BRAND_ELEC)
	  value += (o_ptr->multiple_brand[P_BRAND_ELEC] - MULTIPLE_BASE) * 30L;
	if (o_ptr->id_other & IF_BRAND_FIRE)
	  value += (o_ptr->multiple_brand[P_BRAND_FIRE] - MULTIPLE_BASE) * 20L;
	if (o_ptr->id_other & IF_BRAND_COLD)
	  value += (o_ptr->multiple_brand[P_BRAND_COLD] - MULTIPLE_BASE) * 20L;
	if (o_ptr->id_other & IF_BRAND_POIS)
	  value += (o_ptr->multiple_brand[P_BRAND_POIS] - MULTIPLE_BASE) * 15L;
     
	break;
      }
    }
  
  
  /* Analyze the item */
  switch (o_ptr->tval)
    {
      /* Wands/Staffs */
    case TV_WAND:
      {
	/* Pay extra for charges, depending on standard number of 
	 * charges.  Handle new-style wands correctly.
	 */
	value += (value * o_ptr->pval / o_ptr->number / (k_ptr->pval * 2));
	
	/* Done */
	break;
      }
    case TV_STAFF:
      {
	/* Pay extra for charges, depending on standard number of 
	 * charges. 
	 */
	value += (value * o_ptr->pval / (k_ptr->pval * 2));
	
	/* Done */
	break;
      }
      
      /* Rings/Amulets */
    case TV_RING:
    case TV_AMULET:
      {
	/* Give credit for bonuses */
	if (o_ptr->id_other & IF_TO_H)
	  value += o_ptr->to_h * 100L; 
	if (o_ptr->id_other & IF_TO_D)
	  value += o_ptr->to_d * 100L; 
	if (o_ptr->id_other & IF_TO_A)
	  value += o_ptr->to_a * 100L;
	
	/* Done */
	break;
      }
      
      /* Armor */
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
      {
	/* If it differs from the object kind's base armour Skill penalty 
	 * (this penalty must be in the range +0 to -12), give a debit or 
	 * credit for any modification to Skill. -LM-
	 */
	if (((o_ptr->to_h != k_ptr->to_h) || (o_ptr->to_h < -12) || 
	     (o_ptr->to_h > 0)) && (o_ptr->id_other & IF_TO_H)) 
	  value += (o_ptr->to_h * 100L);
	
	/* Standard debit or credit for to_d and to_a bonuses. */
	if (o_ptr->id_other & IF_TO_D)
	  value += (o_ptr->to_d) * 100L;
	if (o_ptr->id_other & IF_TO_A)
	  value += (o_ptr->to_a) * 100L;
	
	/* Done */
	break;
      }

      /* Sharp weapons */
    case TV_DIGGING:
    case TV_SWORD:
    case TV_POLEARM:
      {
	/* Blessed */
	if (o_ptr->id_obj & OF_BLESSED) value += 400L;

	/* Fall through */
      }
      
      /* Bows/Weapons */
    case TV_BOW:
    case TV_HAFTED:
      {
	/* Because +to_hit and +to_dam work differently now, this
	 * formula also needed to change.  Although long, it now
	 * yields results that match the increase in the weapon's
	 * combat power.  -LM-
	 */
	int hit = (o_ptr->id_other & IF_TO_H) ? o_ptr->to_h : 0;
	int dam = (o_ptr->id_other & IF_TO_D) ? o_ptr->to_d : 0;
	int arm = (o_ptr->id_other & IF_TO_A) ? o_ptr->to_a : 0;
	value += (((hit + 10) * (dam + 10) - 100) * 
		  (k_ptr->cost + 1000L) / 250) + (arm * 100);
      
	
	/* Hack -- Factor in improved damage dice.  If you want to buy
	 * one of the truly dangerous weapons, be prepared to pay for it.
	 */
	if ((o_ptr->ds > k_ptr->ds) && (o_ptr->dd == k_ptr->dd)
	    && (o_ptr->id_other & IF_DD_DS))
	  {
	    value += (o_ptr->ds - k_ptr->ds) * (o_ptr->ds - k_ptr->ds) 
	      * o_ptr->dd * o_ptr->dd * 16L;
	    
	    /* Naturally, ego-items with high base dice should be 
	     * very expensive. */
	    if (has_ego_properties(o_ptr))
	      {
		value += (5 * value / 2);
	      }
	  }
	
	/* Give credit for perfect balance. */
	if (o_ptr->id_obj & OF_PERFECT_BALANCE) value += o_ptr->dd * 200L;
	
	/* Done */
	break;
      }

      /* Ammo */
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	/* Factor in the bonuses */
	if (o_ptr->id_other & IF_TO_H)
	  value += o_ptr->to_h * 2L; 
	if (o_ptr->id_other & IF_TO_D)
	  value += o_ptr->to_d * 2L;
	
	
	/* Hack -- Factor in improved damage dice. -LM- */
	if ((o_ptr->ds > k_ptr->ds) && (o_ptr->id_other & IF_DD_DS))
	  {
	    value += (1 + o_ptr->ds - k_ptr->ds) * 
	      (2 + o_ptr->ds - k_ptr->ds) * 1L;
	    
	    /* Naturally, ego-items with high base dice should be expensive. */
	    if (has_ego_properties(o_ptr))
	      {
		value += (5 * value / 2);
	      }
	  }
	
	/* Done */
	break;
      }
      
      /* Who wants an empty chest?  */
    case TV_CHEST:
      {
	if (o_ptr->pval == 0) return (0L);
	break;
      }
    }
  
  
  /* Return the value */
  return (value < 0 ? 0L : value);
}


/**
 * Distribute charges of rods or wands.
 *
 * o_ptr = source item
 * q_ptr = target item, must be of the same type as o_ptr
 * amt	 = number of items that are transfered
 */
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt)
{
  /*
   * Hack -- If rods or wands are dropped, the total maximum timeout or
   * charges need to be allocated between the two stacks.   If all the items
   * are being dropped, it makes for a neater message to leave the original
   * stack's pval alone. -LM-
   */
  if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD))
    {
      q_ptr->pval = o_ptr->pval * amt / o_ptr->number;
      if (amt < o_ptr->number)
	o_ptr->pval -= q_ptr->pval;
      
      /* Hack -- Rods also need to have their timeouts distributed.  The
       * dropped stack will accept all time remaining to charge up to its
       * maximum.
       */
      if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
	{
	  if (q_ptr->pval > o_ptr->timeout)
	    q_ptr->timeout = o_ptr->timeout;
	  else
	    q_ptr->timeout = q_ptr->pval;
	  
	  if (amt < o_ptr->number)
	    o_ptr->timeout -= q_ptr->timeout;
	}
    }
}


void reduce_charges(object_type *o_ptr, int amt)
{
  /*
   * Hack -- If rods or wand are destroyed, the total maximum timeout or
   * charges of the stack needs to be reduced, unless all the items are
   * being destroyed. -LM-
   */
  if (((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD)) &&
      (amt < o_ptr->number))
    {
      o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
    }
}

/**
 * Return the price of an item including plusses (and charges)
 *
 * This function returns the "value" of the given item (qty one)
 *
 * Never notice "unknown" bonuses or properties, including "curses",
 * since that would give the player information he did not have.
 *
 * Note that discounted items stay discounted forever, even if
 * the discount is "forgotten" by the player via memory loss.
 */
s32b object_value(object_type *o_ptr)
{
  s32b value;
  
  
  /* Known items -- acquire the actual value */
  if (object_known_p(o_ptr) || ((wield_slot(o_ptr) > 0) && !artifact_p(o_ptr)))
    {
      /* Real value (see above) */
      value = object_value_real(o_ptr);
    }
  
  /* Unknown items -- acquire a base value */
  else
    {
      /* Base value (see above) */
      value = object_value_base(o_ptr);
    }
  
  
  /* Apply discount (if any) */
  if (o_ptr->discount) value -= (value * o_ptr->discount / 100L);
  
  
  /* Return the final value */
  return (value);
}





/**
 * Determine if an item can "absorb" a second item
 *
 * See "object_absorb()" for the actual "absorption" code.
 *
 * If permitted, we allow staffs (if they are known to have equal charges 
 * and both are either known or confirmed empty) and wands (if both are 
 * either known or confirmed empty) and rods (in all cases) to combine. 
 * Staffs will unstack (if necessary) when they are used, but wands and 
 * rods will only unstack if one is dropped. -LM-
 *
 * If permitted, we allow weapons/armor to stack, if fully "known".
 *
 * Missiles combine if both stacks have the same "known" status.  This 
 * helps make unidentified stacks of missiles useful.  Stacks of missiles 
 * without plusses will combine if both are at least sensed. -LM-
 *
 * Food, potions, scrolls, rods, and "easy know" items always stack.
 *
 * Chests, and activatable items, never stack (for various reasons).
 */
bool object_similar(object_type *o_ptr, object_type *j_ptr)
{
  int i;
  int total = o_ptr->number + j_ptr->number;
  
  
  /* Require identical object types */
  if (o_ptr->k_idx != j_ptr->k_idx) return (FALSE);

  /* Require identical activations */
  if (o_ptr->activation != j_ptr->activation) return (FALSE);
  
  /* Require identical flags */
  if (o_ptr->flags_obj != j_ptr->flags_obj) return (FALSE);
  if (o_ptr->flags_curse != j_ptr->flags_curse) return (FALSE);
  
  /* Require identical resists, etc */
  for (i = 0; i < MAX_P_RES; i++)
    if (o_ptr->percent_res[i] != j_ptr->percent_res[i]) return (FALSE);
  for (i = 0; i < A_MAX; i++)
    if (o_ptr->bonus_stat[i] != j_ptr->bonus_stat[i]) return (FALSE);
  for (i = 0; i < MAX_P_BONUS; i++)
    if (o_ptr->bonus_other[i] != j_ptr->bonus_other[i]) return (FALSE);
  for (i = 0; i < MAX_P_SLAY; i++)
    if (o_ptr->multiple_slay[i] != j_ptr->multiple_slay[i]) return (FALSE);
  for (i = 0; i < MAX_P_BRAND; i++)
    if (o_ptr->multiple_brand[i] != j_ptr->multiple_brand[i]) return (FALSE);
 
  
  /* Analyze the items */
  switch (o_ptr->tval)
    {
      /* Chests */
    case TV_CHEST:
      {
	/* Never okay */
	return (FALSE);
      }
      
      /* Food and Potions and Scrolls */
    case TV_FOOD:
    case TV_POTION:
    case TV_SCROLL:
      {
	/* Assume okay */
	break;
      }
      
      /* Staffs */
    case TV_STAFF:
      {
	/* Require either knowledge or known empty for both staffs. */
	if ((!(o_ptr->ident & (IDENT_EMPTY)) && !object_known_p(o_ptr)) || 
	    (!(j_ptr->ident & (IDENT_EMPTY)) && !object_known_p(j_ptr))) 
	  return(FALSE);
	
	/* Require identical charges, since staffs are bulky. */
	if (o_ptr->pval != j_ptr->pval) return (FALSE);
	
	/* Assume okay */
	break;
      }
      /* Wands */
    case TV_WAND:
      {
	/* Assume okay */
	break;
      }
      
      /* Rods */
    case TV_ROD:
      {
	/* Assume okay. */
	break;
      }
      
      /* Rings, Amulets, Lites */
    case TV_RING:
    case TV_AMULET:
    case TV_LITE:
      /* Weapons and Armor */
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
      /* Missiles */
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
      {
	/* Require identical "bonuses" */
	if (o_ptr->to_h != j_ptr->to_h) return (FALSE);
	if (o_ptr->to_d != j_ptr->to_d) return (FALSE);
	if (o_ptr->to_a != j_ptr->to_a) return (FALSE);
	
	/* Require identical "pval" code */
	if (o_ptr->pval != j_ptr->pval) return (FALSE);
	
	/* Require identical "artifact" names */
	if (o_ptr->name1 != j_ptr->name1) return (FALSE);
	
	/* Require identical "ego-item" names */
	if (o_ptr->name2 != j_ptr->name2) return (FALSE);
	
	/* Hack -- Never stack items with different activations */
	if (o_ptr->activation != j_ptr->activation)
	  return (FALSE);
	
	/* Hack -- Never stack recharging items */
	if (o_ptr->timeout || j_ptr->timeout) return (FALSE);
	
	/* Require identical "values" */
	if (o_ptr->ac != j_ptr->ac) return (FALSE);
	if (o_ptr->dd != j_ptr->dd) return (FALSE);
	if (o_ptr->ds != j_ptr->ds) return (FALSE);
	
	/* Probably okay */
	break;
      }
      
      /* Various */
    default:
      {
	/* Require knowledge */
	if (!object_known_p(o_ptr) || !object_known_p(j_ptr)) return (FALSE);
	
	/* Probably okay */
	break;
      }
    }
  
  
  /* Hack -- require semi-matching "inscriptions" */
  if (o_ptr->note && j_ptr->note && (o_ptr->note != j_ptr->note)) 
    return (FALSE);
  
  /* Hack -- normally require matching "inscriptions" */
  if (!stack_force_notes && (o_ptr->note != j_ptr->note)) return (FALSE);
  
  /* Hack -- normally require matching "discounts" */
  if (!stack_force_costs && (o_ptr->discount != j_ptr->discount)) 
    return (FALSE);
  
  
  /* Maximal "stacking" limit */
  if (total >= MAX_STACK_SIZE) return (FALSE);
  
  
  /* They match, so they must be similar */
  return (TRUE);
}


/**
 * Allow one item to "absorb" another, assuming they are similar
 *
 * Handle new wands/rods -LM-
 */
void object_absorb(object_type *o_ptr, object_type *j_ptr)
{
  int total = o_ptr->number + j_ptr->number;
  
  /* Add together the item counts */
  o_ptr->number = ((total < MAX_STACK_SIZE) ? total : (MAX_STACK_SIZE - 1));
  
  /* Hack -- if wands are stacking, combine the charges. 
   * If one was unknown, now both are. */
  if (o_ptr->tval == TV_WAND)
    {
      o_ptr->pval += j_ptr->pval;
      if (o_ptr->pval != 0) o_ptr->ident &= ~(IDENT_EMPTY);
      if (!(j_ptr->ident & IDENT_KNOWN)) o_ptr->ident &= ~(IDENT_KNOWN);
      if (!(o_ptr->ident & IDENT_KNOWN)) j_ptr->ident &= ~(IDENT_KNOWN);
    }
  
  /* Hack -- blend "known" status */
  if (object_known_p(j_ptr)) object_known(o_ptr);
  if (object_known_p(o_ptr)) object_known(j_ptr);
  
  /* Hack -- blend "feelings" */
  if (j_ptr->feel) o_ptr->feel = j_ptr->feel;
  if (o_ptr->feel) j_ptr->feel = o_ptr->feel;
  
  /* Hack -- blend feeling status */
  if (j_ptr->ident & IDENT_SENSE) o_ptr->ident |= IDENT_SENSE;
  if (o_ptr->ident & IDENT_SENSE) j_ptr->ident |= IDENT_SENSE;
  
  /* Hack -- Blend store status */
  if (j_ptr->ident & (IDENT_STORE)) o_ptr->ident |= (IDENT_STORE);
  if (o_ptr->ident & (IDENT_STORE)) j_ptr->ident |= (IDENT_STORE);

  /* Hack -- blend "inscriptions" */
  if (j_ptr->note) o_ptr->note = j_ptr->note;
  if (o_ptr->note) j_ptr->note = o_ptr->note;
  
  /* Hack -- could average discounts XXX XXX XXX */
  /* Hack -- save largest discount XXX XXX XXX */
  if (o_ptr->discount < j_ptr->discount) o_ptr->discount = j_ptr->discount;
  
  /* Hack -- if rods are stacking, add the pvals (maximum timeouts) and 
   * current timeouts together. */
  if (o_ptr->tval == TV_ROD)
    {
      o_ptr->pval += j_ptr->pval;
      o_ptr->timeout += j_ptr->timeout;
    }
}



/**
 * Find the index of the object_kind with the given tval and sval
 */
s16b lookup_kind(int tval, int sval)
{
  int k;
  
  /* Look for it */
  for (k = 1; k < z_info->k_max; k++)
    {
      object_kind *k_ptr = &k_info[k];
      
      /* Found a match */
      if ((k_ptr->tval == tval) && (k_ptr->sval == sval)) return (k);
    }
  
  /* Oops */
  msg_format("No object (%d,%d)", tval, sval);
  
  /* Oops */
  return (0);
}


/**
 * Wipe an object clean.
 */
void object_wipe(object_type *o_ptr)
{
  /* Wipe the structure */
  (void)WIPE(o_ptr, object_type);
}


/**
 * Prepare an object based on an existing object
 */
void object_copy(object_type *o_ptr, object_type *j_ptr)
{
  /* Copy the structure */
  (void)COPY(o_ptr, j_ptr, object_type);
}


/**
 * Prepare an object based on an object kind.
 */
void object_prep(object_type *o_ptr, int k_idx)
{
  int i;

  object_kind *k_ptr = &k_info[k_idx];
  
  /* Clear the record */
  (void)WIPE(o_ptr, object_type);
  
  /* Save the kind index */
  o_ptr->k_idx = k_idx;
  
  /* Efficiency -- tval/sval */
  o_ptr->tval = k_ptr->tval;
  o_ptr->sval = k_ptr->sval;
  
  /* Default "pval" */
  o_ptr->pval = k_ptr->pval;
  
  /* Default number */
  o_ptr->number = 1;
  
  /* Default weight */
  o_ptr->weight = k_ptr->weight;
  
  /* Default magic */
  o_ptr->to_h = k_ptr->to_h;
  o_ptr->to_d = k_ptr->to_d;
  o_ptr->to_a = k_ptr->to_a;
  
  /* Default power */
  o_ptr->ac = k_ptr->ac;
  o_ptr->dd = k_ptr->dd;
  o_ptr->ds = k_ptr->ds;

  /* Default activation */
  o_ptr->activation = k_ptr->activation;

  /* Default flags */
  o_ptr->flags_obj = k_ptr->flags_obj;
  o_ptr->flags_curse = k_ptr->flags_curse;

  /* Default resists, bonuses, multiples */
  for (i = 0; i < MAX_P_RES; i++)
    o_ptr->percent_res[i] = k_ptr->percent_res[i];
  for (i = 0; i < A_MAX; i++)
    o_ptr->bonus_stat[i] = k_ptr->bonus_stat[i];
  for (i = 0; i < MAX_P_BONUS; i++)
    o_ptr->bonus_other[i] = k_ptr->bonus_other[i];
  for (i = 0; i < MAX_P_SLAY; i++)
    o_ptr->multiple_slay[i] = k_ptr->multiple_slay[i];
  for (i = 0; i < MAX_P_BRAND; i++)
    o_ptr->multiple_brand[i] = k_ptr->multiple_brand[i];
  
  /* Not found anywhere yet */
  o_ptr->found = 0;
}


/**
 * Help determine an "enchantment bonus" for an object.
 *
 * To avoid floating point but still provide a smooth distribution of bonuses,
 * we simply round the results of division in such a way as to "average" the
 * correct floating point value.
 *
 * This function has been changed.  It uses "Rand_normal()" to choose values
 * from a normal distribution, whose mean moves from zero towards the max as
 * the level increases, and whose standard deviation is equal to 1/4 of the
 * max, and whose values are forced to lie between zero and the max, inclusive.
 *
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very
 * rare to get the "full" enchantment on an object, even at deep levels.
 *
 * It is always possible (albeit unlikely) to get the "full" enchantment.
 *
 * A sample distribution of values from "m_bonus(10, N)" is shown below:
 *<pre>
 *   N	     0	   1	 2     3     4	   5	 6     7     8	   9	10
 * ---	  ----	----  ----  ----  ----	----  ----  ----  ----	----  ----
 *   0	 66.37 13.01  9.73  5.47  2.89	1.31  0.72  0.26  0.12	0.09  0.03
 *   8	 46.85 24.66 12.13  8.13  4.20	2.30  1.05  0.36  0.19	0.08  0.05
 *  16	 30.12 27.62 18.52 10.52  6.34	3.52  1.95  0.90  0.31	0.15  0.05
 *  24	 22.44 15.62 30.14 12.92  8.55	5.30  2.39  1.63  0.62	0.28  0.11
 *  32	 16.23 11.43 23.01 22.31 11.19	7.18  4.46  2.13  1.20	0.45  0.41
 *  40	 10.76	8.91 12.80 29.51 16.00	9.69  5.90  3.43  1.47	0.88  0.65
 *  48	  7.28	6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80	1.22  0.94
 *  56	  4.41	4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68	1.96  1.78
 *  64	  2.81	3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04	3.04  2.64
 *  72	  1.87	1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52	4.42  4.62
 *  80	  1.02	1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28	6.52  7.33
 *  88	  0.70	0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58	8.47 10.38
 *  96	  0.27	0.60  1.25  2.28  4.30	7.60 10.77 22.52 22.51 11.37 16.53
 * 104	  0.22	0.42  0.77  1.36  2.62	5.33  8.93 13.05 29.54 15.23 22.53
 * 112	  0.15	0.20  0.56  0.87  2.00	3.83  6.86 10.06 17.89 27.31 30.27
 * 120	  0.03	0.11  0.31  0.46  1.31	2.48  4.60  7.78 11.67 25.53 45.72
 * 128	  0.02	0.01  0.13  0.33  0.83	1.41  3.24  6.17  9.57 14.22 64.07
 </pre>
 */
extern s16b m_bonus(int max, int level)
{
  int bonus, stand, extra, value;
  
  
  /* Paranoia -- enforce maximal "level" */
  if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;

  /* 0 means 0 */
  if (max == 0) return (0);  
  
  /* The "bonus" moves towards the max */
  bonus = ((max * level) / MAX_DEPTH);
  
  /* Hack -- determine fraction of error */
  extra = ((max * level) % MAX_DEPTH);
  
  /* Hack -- simulate floating point computations */
  if (rand_int(MAX_DEPTH) < extra) bonus++;
  
  
  /* The "stand" is equal to one quarter of the max */
  stand = (max / 4);
  
  /* Hack -- determine fraction of error */
  extra = (max % 4);
  
  /* Hack -- simulate floating point computations */
  if (rand_int(4) < extra) stand++;
  
  
  /* Choose an "interesting" value */
  value = Rand_normal(bonus, stand);
  
  /* Enforce the minimum value */
  if (value < 0) return (0);
  
  /* Enforce the maximum value */
  if (value > max) return (max);
  
  /* Result */
  return (value);
}




/**
 * Cheat -- describe a created object for the user
 */
static void object_mention(object_type *o_ptr)
{
  char o_name[120];
  
  /* Describe */
  object_desc_store(o_name, o_ptr, FALSE, 0);
  
  /* Artifact */
  if (artifact_p(o_ptr))
    {
      /* Silly message */
      msg_format("Artifact (%s)", o_name);
    }
  
  /* Ego-item */
  else if (ego_item_p(o_ptr))
    {
      /* Silly message */
      msg_format("Ego-item (%s)", o_name);
    }
  
  /* Normal item */
  else
    {
      /* Silly message */
      msg_format("Object (%s)", o_name);
    }
}

/**
 * Test an ego item for any negative qualities
 */
bool good_ego(const ego_item_type *e_ptr)
{
  int i, to_h, to_d;

  /* Resists, bonuses, multiples */
  for (i = 0; i < MAX_P_RES; i++)
    if (e_ptr->percent_res[i] > RES_LEVEL_BASE) return FALSE;
  for (i = 0; i < A_MAX; i++)
    if (e_ptr->bonus_stat[i] < BONUS_BASE) return FALSE;
  for (i = 0; i < MAX_P_BONUS; i++)
    if (e_ptr->bonus_other[i] < BONUS_BASE) return FALSE;
  for (i = 0; i < MAX_P_SLAY; i++)
    if (e_ptr->multiple_slay[i] < MULTIPLE_BASE) return FALSE;
  for (i = 0; i < MAX_P_BRAND; i++)
    if (e_ptr->multiple_brand[i] < MULTIPLE_BASE) return FALSE;

  /* To skill, to deadliness, to AC */
  if ((e_ptr->max_to_h > 0) && (e_ptr->max_to_h < 129)) to_h = e_ptr->max_to_h;
  else to_h = 128 - e_ptr->max_to_h;
  if ((e_ptr->max_to_d > 0) && (e_ptr->max_to_d < 129)) to_d = e_ptr->max_to_d;
  else to_d = 128 - e_ptr->max_to_d;
  if (to_h + to_d < 0) return FALSE;
  if (e_ptr->max_to_a > 128) return FALSE;

  /* Curses */
  if ((e_ptr->flags_curse) || (e_ptr->flags_kind & KF_RAND_CURSE)) return FALSE;

  
  /* Must be OK */
  return TRUE;
}

/**
 * Attempt to change an object into an ego-item -MWK-
 * Better only called by apply_magic().
 * The return value says if we picked a cursed item (if allowed) and is
 * passed on to a_m_aux1/2().
 * If no legal ego item is found, this routine returns 0, resulting in
 * an unenchanted item.
 *
 * Modified for Oangband to specifically create cursed/non-cursed egos.
 * This is to keep the probabilities separate between cursed and
 * non-cursed ego types.
 */
static int make_ego_item(object_type *o_ptr, int power)
{
  int i, j, level;
  
  int e_idx;
  
  long value, total;

  ego_item_type *e_ptr;
  
  alloc_entry *table = alloc_ego_table;
  
  
  /* Fail if object already is ego or artifact */
  if (o_ptr->name1) return (FALSE);
  if (o_ptr->name2) return (FALSE);
  
  level = object_level;
  
  /* Boost level (like with object base types) */
  if (level > 0)
    {
      /* Occasional "boost" */
      if (rand_int(GREAT_EGO) == 0)
	{
	  /* The bizarre calculation again */
	  level = 1 + (level * MAX_DEPTH / randint(MAX_DEPTH));
	}
    }
  
  /* Reset total */
  total = 0L;
  
  /* Process probabilities */
  for (i = 0; i < alloc_ego_size; i++)
    {
      /* Default */
      table[i].prob3 = 0;
      
      /* Objects are sorted by depth */
      if (table[i].level > level) continue;
      
      /* Get the index */
      e_idx = table[i].index;
      
      /* Get the actual kind */
      e_ptr = &e_info[e_idx];
      
      /* If we force good/great, don't create cursed */
      if ((power > 0) && (!good_ego(e_ptr))) continue;
      
      /* If we force cursed, don't create good */
      if ((power < 0) && good_ego(e_ptr)) continue;
      
      /* Test if this is a legal ego-item type for this object */
      for (j = 0; j < EGO_TVALS_MAX; j++)
	{
	  /* Require identical base type */
	  if (o_ptr->tval == e_ptr->tval[j])
	    {
	      /* Require sval in bounds, lower */
	      if (o_ptr->sval >= e_ptr->min_sval[j])
		{
		  /* Require sval in bounds, upper */
		  if (o_ptr->sval <= e_ptr->max_sval[j])
		    {
		      /* Accept */
		      table[i].prob3 = table[i].prob2;
		    }
		}
	    }
	}
      
      /* Total */
      total += table[i].prob3;
    }
  
  /* No legal ego-items -- create a normal unenchanted one */
  if (total == 0) return (0);
  
  
  /* Pick an ego-item */
  value = rand_int(total);
  
  /* Find the object */
  for (i = 0; i < alloc_ego_size; i++)
    {
      /* Found the entry */
      if (value < table[i].prob3) break;
      
      /* Decrement */
      value = value - table[i].prob3;
    }
  
  /* We have one */
  e_idx = (byte)table[i].index;
  o_ptr->name2 = e_idx;
  
  return ((e_info[e_idx].flags_curse || 
	   e_info[e_idx].flags_kind & KF_RAND_CURSE) ? -2 : 2);
}

/**
 * Mega-Hack -- Attempt to create one of the "Special Objects".
 *
 * This function now does not run down the list sequentially, which 
 * favored some artifacts at the expense of others, but starts at 
 * a random index, and then wraps around.
 *
 * We are only called from "make_object()", and we assume that
 * "apply_magic()" is called immediately after we return.
 *
 * Note -- see "make_artifact()" and "apply_magic()"
 */
static bool make_artifact_special(object_type *o_ptr)
{
  int i, first_pick;
  
  int k_idx = 0;
  
  /* No artifacts in the town */
  if (!p_ptr->depth) return (FALSE);
  
  first_pick = rand_int(artifact_special_cnt);
  
  for (i = 0; i < artifact_special_cnt; i++)
    {
      int j = (first_pick + i) % artifact_special_cnt;
      int choice = artifact_special[j];
      artifact_type *a_ptr = &a_info[choice];
      
      /* Skip "empty" artifacts */
      if (!a_ptr->name) continue;
      
      /* Cannot make an artifact twice */
      if (a_ptr->creat_turn) continue;
      
      /* Enforce minimum "depth" (loosely) */
      if (a_ptr->level > p_ptr->depth)
	{
	  /* Acquire the "out-of-depth factor" */
	  int d = (a_ptr->level - p_ptr->depth) * 2;
	  
	  /* Roll for out-of-depth creation */
	  if (rand_int(d) != 0) continue;
	}
      
      /* Artifact "rarity roll" */
      if (rand_int(a_ptr->rarity) != 0) continue;
      
      /* Find the base object */
      k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
      
      /* Enforce minimum "object" level (loosely) */
      if (k_info[k_idx].level > object_level)
	{
	  /* Acquire the "out-of-depth factor" */
	  int d = (k_info[k_idx].level - object_level) * 5;
	  
	  /* Roll for out-of-depth creation */
	  if (rand_int(d) != 0) continue;
	}
      
      /* Assign the template */
      object_prep(o_ptr, k_idx);
      
      /* Mega-Hack -- mark the item as an artifact */
      o_ptr->name1 = choice;
      
      /* Success */
      return (TRUE);
    }
  
  /* Failure */
  return (FALSE);
}


/**
 * Attempt to change an object into an artifact
 *
 * This routine should only be called by "apply_magic()"
 *
 * Note -- see "make_artifact_special()" and "apply_magic()"
 */
static bool make_artifact(object_type *o_ptr)
{
  int i;
  
  
  /* No artifacts in the town */
  if (!p_ptr->depth) return (FALSE);
  
  /* Paranoia -- no "plural" artifacts */
  if (o_ptr->number != 1) return (FALSE);
  
  /* Check the artifact list (skip the "specials") */
  for (i = 0; i < artifact_normal_cnt; i++)
    {
      int choice = artifact_normal[i];
      artifact_type *a_ptr = &a_info[choice];
      
      /* Skip "empty" items */
      if (!a_ptr->name) continue;
      
      /* Cannot make an artifact twice */
      if (a_ptr->creat_turn) continue;
      
      /* Must have the correct fields */
      if (a_ptr->tval != o_ptr->tval) continue;
      if (a_ptr->sval != o_ptr->sval) continue;
      
      /* XXX XXX Enforce minimum "depth" (loosely) */
      if (a_ptr->level > p_ptr->depth)
	{
	  /* Acquire the "out-of-depth factor" */
	  int d = (a_ptr->level - p_ptr->depth) * 2;
	  
	  /* Roll for out-of-depth creation */
	  if (rand_int(d) != 0) continue;
	}
      
      /* We must make the "rarity roll" */
      if (rand_int(a_ptr->rarity) != 0) continue;
      
      /* Hack -- mark the item as an artifact */
      o_ptr->name1 = choice;
      
      /* Success */
      return (TRUE);
    }
  
  /* Failure */
  return (FALSE);
}


/**
 * Apply magic to an item known to be a "weapon"
 *
 * Hack -- note special base damage dice boosting
 * Hack -- note special processing for weapon/digger
 * Hack -- note special rating boost for dragon scale mail
 */
static void a_m_aux_1(object_type *o_ptr, int level, int power)
{
  int tohit1 = randint(5) + m_bonus(5, level);
  int todam1 = randint(5) + m_bonus(5, level);
  
  int tohit2 = m_bonus(10, level);
  int todam2 = m_bonus(10, level);
  
  /* Good */
  if (power > 0)
    {
      /* Enchant */
      o_ptr->to_h += tohit1;
      o_ptr->to_d += todam1;
      
      /* Very good */
      if (power > 1)
	{
	  /* Enchant again */
	  o_ptr->to_h += tohit2;
	  o_ptr->to_d += todam2;
	}
    }
  
  /* Bad */
  else if (power < 0)
    {
      /* Penalize */
      o_ptr->to_h -= tohit1;
      o_ptr->to_d -= todam1;
      
      /* Perilous - more random now */
      if (power < -1)
	{
	  /* Shake it up */
	  if (rand_int(3) == 0) o_ptr->to_h = 0 - o_ptr->to_h;
	  if (rand_int(3) == 0) o_ptr->to_d = 0 - o_ptr->to_d;
	  if (rand_int(2) == 0) o_ptr->to_h -= tohit2;
	  else o_ptr->to_h += tohit2;
	  if (rand_int(2) == 0) o_ptr->to_d -= todam2;
	  else o_ptr->to_d += todam2;
	}
    }
  
  
  /* Analyze type */
  switch (o_ptr->tval)
    {
    case TV_DIGGING:
      {
	/* Very bad */
	if (power < -1)
	  {
	    /* Hack -- Horrible digging bonus */
	    o_ptr->bonus_other[P_BONUS_TUNNEL] = 0 - (5 + randint(5));
	  }
	
	/* Bad */
	else if (power < 0)
	  {
	    /* Hack -- Reverse digging bonus */
	    o_ptr->bonus_other[P_BONUS_TUNNEL] = 
	      0 - (o_ptr->bonus_other[P_BONUS_TUNNEL]);
	  }
	
	break;
      }
      
      
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
      {
	/* Very bad */
	if (power < -1)
	  {
	    if (o_ptr->to_d < 0) o_ptr->to_d *= 2;
	  }
	
	break;
      }
      
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
      {
	/* First, a little hack to keep certain values sane. */
	/* If at least good, */
	if (power > 0)
	  {
	    /* chop the bonuses in half, rounded up. */
	    o_ptr->to_h -= o_ptr->to_h / 2;
	    o_ptr->to_d -= o_ptr->to_d / 2;
	    
	  }
      }
    }
}


/**
 * Apply magic to an item known to be "armor"
 *
 * Hack -- note special processing for crown/helm
 * Hack -- note special processing for robe of permanence
 */
static void a_m_aux_2(object_type *o_ptr, int level, int power)
{
  int toac1, toac2;
  
  /* Boots and handgear aren't as important to total AC. */
  if ((o_ptr->tval == TV_BOOTS) || (o_ptr->tval == TV_GLOVES))
    {
      toac1 = randint(3) + m_bonus(4, level);
      toac2 = m_bonus(8, level);
    }
  else
    {
      toac1 = randint(5) + m_bonus(5, level);
      toac2 = m_bonus(10, level);
    }
  
  
  /* Good */
  if (power > 0)
    {
      /* Enchant */
      o_ptr->to_a += toac1;
      
      /* Very good */
      if (power > 1)
	{
	  /* Enchant again */
	  o_ptr->to_a += toac2;
	}
    }
  
  /* Bad */
  else if (power < 0)
    {
      /* Penalize */
      o_ptr->to_a -= toac1;
      
      /* Very bad */
      if (power < -1)
	{
	  /* Penalize again */
	  o_ptr->to_a -= toac2;
	}
    }
  
  
  /* Analyze type */
  switch (o_ptr->tval)
    {
    case TV_DRAG_ARMOR:
      {
	/* Rating boost */
	rating += 20;
	
	/* Mention the item */
	if (cheat_peek) object_mention(o_ptr);
	
	break;
      }
      
      
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
    case TV_SHIELD:
    case TV_GLOVES:
    case TV_BOOTS:
    case TV_CROWN:
    case TV_HELM:
    case TV_CLOAK:
      {
	break;
      }
    }
}




/**
 * Apply magic to miscellanious items
 *
 * Hack -- note the special code for various items
 */
static void a_m_aux_4(object_type *o_ptr, int level, int power)
{
  object_kind *k_ptr = &k_info[o_ptr->k_idx];
  
  /* Apply magic (good or bad) according to type */
  switch (o_ptr->tval)
    {
    case TV_LITE:
      {
	/* Hack -- Torches -- random fuel */
	if (o_ptr->sval == SV_LITE_TORCH)
	  {
	    if (o_ptr->pval > 0) o_ptr->pval = randint(o_ptr->pval);
	  }
	
	/* Hack -- Lanterns -- random fuel */
	if (o_ptr->sval == SV_LITE_LANTERN)
	  {
	    if (o_ptr->pval > 0) o_ptr->pval = randint(o_ptr->pval);
	  }
	
	break;
      }
      
    case TV_WAND:
    case TV_STAFF:
      {
	/* The wand or staff gets a number of initial charges equal 
	 * to between 1/2 (+1) and the full object kind's pval.
	 */
	o_ptr->pval = k_ptr->pval / 2 + randint((k_ptr->pval + 1) / 2);
	break;
      }
      
    case TV_ROD:
      {
	/* Transfer the pval. */
	o_ptr->pval = k_ptr->pval;
	break;
      }
      
    case TV_CHEST:	/* changed by LM */
      {
	/* Hack -- pick a "value/difficulty", capable of being
	 * as little as chest level - 16... */
	o_ptr->pval = (k_info[o_ptr->k_idx].level) + 20 - damroll(4, 9);
	
	/* ...and never exceeding chest level + 4. */
	if (o_ptr->pval > k_info[o_ptr->k_idx].level)
	  o_ptr->pval = k_info[o_ptr->k_idx].level + randint(4);
	
	/* Value/difficulty cannot be less than 5. */
	if (o_ptr->pval < 5) o_ptr->pval = 5;
	
	/* Never exceed "value/difficulty" of 99. */
	if (o_ptr->pval > 99) o_ptr->pval = 99;
	
	break;
      }
    }
}

/**
 * Apply slightly randomised percentage resistances -NRM-
 * Also add element proofing flags where appropriate
 */
extern void apply_resistances(object_type *o_ptr, int lev, u32b flags)
{
  int i, res = 0;

  /* Get the ego type resists */  
  if (o_ptr->name2)
    {
      ego_item_type *e_ptr = &e_info[o_ptr->name2];
      
      for (i = 0; i < MAX_P_RES; i++)
	{
	  /* Get the resistance value */
	  if (e_ptr->percent_res[i] != RES_LEVEL_BASE)
	    {
	      res = o_ptr->percent_res[i] * e_ptr->percent_res[i] / 100;
	      o_ptr->percent_res[i] = res;
	    }
	  
	}
    }

  /* Add extra resists */
  
  /* Vulnerability */
  if (flags & KF_RAND_RES_NEG)
  {
    int roll = 0;
    /* This gets done more often as we go deeper */
    while (roll < 2)
    {
      /* Low resists more likely at low levels */
      int k = rand_int(128);
      bool low = (k < (128 - lev));

      k = (low ? rand_int(4) : rand_int(10) + 4);
      o_ptr->percent_res[k] += 10 + rand_int(5) + m_bonus(30, lev);

      roll = rand_int(10 - (lev/30));
    }
  }

  /* Rings of Protection, etc */
  if (flags & KF_RAND_RES_SML)
  {
    int roll = 0;
    /* This gets done more often as we go deeper */
    while (roll < 2)
    {
      /* Low resists more likely at low levels */
      int k = rand_int(128);
      bool low = (k < (128 - lev));

      k = (low ? rand_int(4) : rand_int(10) + 4);
      o_ptr->percent_res[k] -= 4 + rand_int(8) + m_bonus(10, lev);

      /* Twice as many of these */
      k = rand_int(128);
      low = (k < (128 - lev));
      k = (low ? rand_int(4) : rand_int(10) + 4);
      o_ptr->percent_res[k] -= 4 + rand_int(8) + m_bonus(10, lev);

      /* Occasionally reverse one */
      if (rand_int(3) == 0) 
	o_ptr->percent_res[k] = RES_LEVEL_MAX - o_ptr->percent_res[k];

      roll = rand_int(10 - (lev/30));
    }
  }

  /* Armour of Resistance etc */
  if (flags & KF_RAND_RES)
  {
    int roll = 0;
    /* This gets done more often as we go deeper */
    while (roll < 2)
    {
      /* Low resists more likely at low levels */
      int k = rand_int(128);
      bool low = (k < (128 - lev));

      k = (low ? rand_int(4) : rand_int(10) + 4);
      o_ptr->percent_res[k] -= 20 + rand_int(5) + m_bonus(20, lev);
      
      roll = rand_int(10 - (lev/30));
    }
  }

  /* Former one high resist objects */
  if (flags & KF_RAND_RES_XTRA)
  {
    int roll = 0;
    /* This gets done more often as we go deeper */
    while (roll < 2)
    {
      /* Low resists less likely here */
      int k = ((rand_int(10) == 0) ? rand_int(4) : rand_int(10) + 4);
      o_ptr->percent_res[k] -= 30 + rand_int(5) + m_bonus(20, lev);
      
      roll = rand_int(10 - (lev/30));
    }
  }


  /* Randomise a bit */
  for (i = 0; i < MAX_P_RES; i++)
    {
      res = 100 - o_ptr->percent_res[i];
      
      /* Only randomise proper resistances */
      if ((res > 0) && (res < 100))
	{
	  if (rand_int(2) == 0) 
	    o_ptr->percent_res[i] -= rand_int(res >> 2);
	  else 
	    o_ptr->percent_res[i] += rand_int(res >> 2);
	}

      /* Enforce bounds - no item gets better than 80% resistance */
      if (o_ptr->percent_res[i] < RES_CAP_ITEM)
	o_ptr->percent_res[i] = RES_CAP_ITEM;
      if (o_ptr->percent_res[i] > RES_LEVEL_MAX)
	o_ptr->percent_res[i] = RES_LEVEL_MAX;
    }

  /* Low resist means object is proof against that element */
  if (o_ptr->percent_res[P_RES_ACID] < RES_LEVEL_BASE) 
    o_ptr->flags_obj |= OF_ACID_PROOF;
  if (o_ptr->percent_res[P_RES_ELEC] < RES_LEVEL_BASE) 
    o_ptr->flags_obj |= OF_ELEC_PROOF;
  if (o_ptr->percent_res[P_RES_FIRE] < RES_LEVEL_BASE) 
    o_ptr->flags_obj |= OF_FIRE_PROOF;
  if (o_ptr->percent_res[P_RES_COLD] < RES_LEVEL_BASE) 
    o_ptr->flags_obj |= OF_COLD_PROOF;
}

/**
 * Complete the "creation" of an object by applying "magic" to the item
 *
 * This includes not only rolling for random bonuses, but also putting the
 * finishing touches on ego-items and artifacts, giving charges to wands and
 * staffs, giving fuel to lites, and placing traps on chests.
 *
 * In particular, note that "Instant Artifacts", if "created" by an external
 * routine, must pass through this function to complete the actual creation.
 *
 * The base "chance" of the item being "good" increases with the "level"
 * parameter, which is usually derived from the dungeon level, being equal
 * to the level plus 10, up to a maximum of 75.   If "good" is true, then
 * the object is guaranteed to be "good".  If an object is "good", then
 * the chance that the object will be "great" (ego-item or artifact), also
 * increases with the "level", being equal to half the level, plus 5, up to
 * a maximum of 20.  If "great" is true, then the object is guaranteed to be
 * "great".  At dungeon level 65 and below, 15/100 objects are "great".
 *
 * If the object is not "good", there is a chance it will be "cursed", and
 * if it is "cursed", there is a chance it will be "broken".  These chances
 * are related to the "good" / "great" chances above.
 *
 * Otherwise "normal" rings and amulets will be "good" half the time and
 * "cursed" half the time, unless the ring/amulet is always good or cursed.
 *
 * If "okay" is true, and the object is going to be "great", then there is
 * a chance that an artifact will be created.  This is true even if both the
 * "good" and "great" arguments are false.  As a total hack, if "great" is
 * true, then the item gets 3 extra "attempts" to become an artifact.
 *
 */
void apply_magic(object_type *o_ptr, int lev, bool okay, bool good, bool great)
{
  int i, rolls, good_percent, great_percent, power;
  
  object_kind *k_ptr = &k_info[o_ptr->k_idx];

  u32b flags_kind = k_ptr->flags_kind;
  
  /* Maximum "level" for various things */
  if (lev > MAX_DEPTH - 1) lev = MAX_DEPTH - 1;
  
  /* Base chance of being "good".  Reduced in Oangband. */
  good_percent = 4 * lev / 5 + 10;
  
  /* Maximal chance of being "good".  Reduced in Oangband. */
  if (good_percent > 65) good_percent = 65;
  
  /* Base chance of being "great".  Increased in Oangband. */
  great_percent = (good_percent / 2) + 5;
  
  /* Maximal chance of being "great".  Increased in Oangband. */
  if (great_percent > 35) great_percent = 35;
  
  
  /* Assume normal */
  power = 0;
  
  /* Roll for "good" */
  if (good || (rand_int(100) < good_percent))
    {
      /* Assume "good" */
      power = 1;
      
      /* Roll for "great" */
      if (great || (rand_int(100) < great_percent)) power = 2;
    }
  
  /* Roll for "dubious" */
  else if ((rand_int(100) < good_percent) && (required_tval == 0))
    {
      /* Assume "dubious" */
      power = -1;
      
      /* Roll for "perilous" */
      if (rand_int(100) < great_percent) power = -2;
    }

  /* Assume no rolls */
  rolls = 0;
  
  /* Get one roll if excellent */
  if (power >= 2) rolls = 1;
  
  /* Hack -- Get four rolls if forced great */
  if (great) rolls = 4;
  
  /* Hack -- Get no rolls if not allowed */
  if (!okay || o_ptr->name1) rolls = 0;
  
  /* Roll for artifacts if allowed */
  for (i = 0; i < rolls; i++)
    {
      /* Roll for an artifact */
      if (make_artifact(o_ptr)) break;
    }
  
  
  /* Hack -- analyze artifacts and transfer the activation index. */
  if (o_ptr->name1)
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      
      /* Hack -- Mark the artifact as "created" */
      a_ptr->creat_turn = 1;
      
      /* Extract the other fields */
      o_ptr->pval = a_ptr->pval;
      o_ptr->ac = a_ptr->ac;
      o_ptr->dd = a_ptr->dd;
      o_ptr->ds = a_ptr->ds;
      o_ptr->to_a = a_ptr->to_a;
      o_ptr->to_h = a_ptr->to_h;
      o_ptr->to_d = a_ptr->to_d;
      o_ptr->weight = a_ptr->weight;

      for (i = 0; i < MAX_P_RES; i++)
	o_ptr->percent_res[i] = a_ptr->percent_res[i];
      for (i = 0; i < A_MAX; i++)
	o_ptr->bonus_stat[i] = a_ptr->bonus_stat[i];
      for (i = 0; i < MAX_P_BONUS; i++)
	o_ptr->bonus_other[i] = a_ptr->bonus_other[i];
      for (i = 0; i < MAX_P_SLAY; i++)
	o_ptr->multiple_slay[i] = a_ptr->multiple_slay[i];
      for (i = 0; i < MAX_P_BRAND; i++)
	o_ptr->multiple_brand[i] = a_ptr->multiple_brand[i];

      o_ptr->flags_obj = a_ptr->flags_obj;
      o_ptr->flags_curse = a_ptr->flags_curse;
      
      /* Mark where it was found */
      o_ptr->found = p_ptr->stage;
      
      /* Transfer the activation information. */
      o_ptr->activation = a_ptr->activation;
      
      /* Mega-Hack -- increase the rating */
      rating += 15;
      
      /* Mega-Hack -- increase the rating again */
      rating += a_ptr->cost / 2000;
      
      /* Set the good item flag */
      good_item_flag = TRUE;
      
      /* Cheat -- peek at the item */
      if (cheat_peek) object_mention(o_ptr);
      
      /* Done */
      return;
    }
  
  
  switch (o_ptr->tval)
    {
      /**** 	  damage dice of Melee Weapons. -LM- ****/
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
      {
	/* All melee weapons get a chance to improve their base
	 * damage dice.   Note the maximum value for dd*ds of 40.
	 */
	int newdicesides = 0;
	int oldsides = o_ptr->ds;
	
	if (rand_int((2 * (o_ptr->dd + 1) * (o_ptr->ds + 1))/((lev/20)+1)) == 0)
	{
	  if ((rand_int((o_ptr->dd + 2) * (o_ptr->ds + 2) / 5) == 0) && 
	      (((o_ptr->dd + 1) * o_ptr->ds) < 41))
	    {
	      o_ptr->weight = (o_ptr->weight * (o_ptr->dd + 3)) / 
		(o_ptr->dd + 2);
	      o_ptr->dd += 1; 
	    }
	  else
	    {
	      newdicesides = o_ptr->ds / 2;
	      
	      for (i = 0; i < newdicesides; i++)
		{
		  if ((o_ptr->dd * (o_ptr->ds + 1) < 41) && 
		      (randint(5) != 1)) o_ptr->ds++;
		}
	      
	      if (rand_int((o_ptr->dd + 2) * (o_ptr->ds + 2) / 5) == 0)
		{
		  /* If we get this far have another go at the 
		     dice missed earlier */
		  newdicesides = (11 * oldsides / 6) - o_ptr->ds;
		  
		  for (i = 0; i < newdicesides; i++)
		    {
		      if ((o_ptr->dd * (o_ptr->ds + 1) < 41) && 
			  (randint(5) != 1)) o_ptr->ds++;
		    }
		}
	      
	      if (randint(5) == 1)
		{
		  o_ptr->weight = (o_ptr->weight * 4) / 5;
		}
	      else if (randint(3) == 1)
		{
		  o_ptr->weight = (o_ptr->weight * 5) / 4;
		}
	    }
	}
	break;
      }
      
      /**** Enhance damage dice of missiles. -LM- ****/
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	/* Up to two chances to enhance damage dice. SJGU was 3 and +2 to ds */
	if (randint(6) == 1) 
	  {
	    o_ptr->ds += 1;
	    if (randint(10) == 1)
	      {
		o_ptr->ds += 1;
	      }
	  }
	break;
      }
    }
  /**** Apply Magic ****/
  
  switch (o_ptr->tval)
    {
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOW:
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	if ((power > 1) || (power < -1))
	  {
	    int ego_power;
	    
	    ego_power = make_ego_item(o_ptr, power);
	    
	    if (ego_power) power = ego_power;
	  }
	
	if (power) a_m_aux_1(o_ptr, lev, power);
	break;
      }
      
    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
    case TV_SHIELD:
    case TV_HELM:
    case TV_CROWN:
    case TV_CLOAK:
    case TV_GLOVES:
    case TV_BOOTS:
      {
	/* Hack - extra chance for perilous armour */
	if (!power && (rand_int(100) < great_percent)) power = -2;

	if ((power > 1) || (power < -1))
	  {
	    int ego_power;
	    
	    ego_power = make_ego_item(o_ptr, power);
	    
	    if (ego_power) power = ego_power;
	  }
	
	if (power) a_m_aux_2(o_ptr, lev, power);
	break;
      }
      
    case TV_RING:
    case TV_AMULET:
      {
	/* Ignore power, generate, then we're done */
	design_ring_or_amulet(o_ptr, lev);
	return;
      }
      
    default:
      {
	a_m_aux_4(o_ptr, lev, power);
	break;
      }
    }

  /*
   * Throwing weapons, especially ego-items may sometimes be
   * perfectly  balanced. 
   */
  if (k_ptr->flags_obj & OF_THROWING)
    {
      int j;
      
      /* 22% of standard items, 44% of ego items */
      j = (o_ptr->name2) ? 4 : 2;
      
      if (rand_int(9) < j)
	{
	  o_ptr->flags_obj |= OF_PERFECT_BALANCE;
	}
    }

  /* Kind flags */
  if (o_ptr->name2) flags_kind |= e_info[o_ptr->name2].flags_kind;

  /* Random sustain */
  if (flags_kind & KF_RAND_SUSTAIN)
    {
      o_ptr->flags_obj |= 
	(OBJECT_RAND_BASE_SUSTAIN << rand_int(OBJECT_RAND_SIZE_SUSTAIN));
    }
	  
  /* Random power */
  if (flags_kind & KF_RAND_POWER)
    {
      o_ptr->flags_obj |= 
	(OBJECT_RAND_BASE_POWER << rand_int(OBJECT_RAND_SIZE_POWER));
    }
  
  /* Random curse */
  if (flags_kind & KF_RAND_CURSE)
    {
      bool another_curse = TRUE; 

      /* Mostly only one curse */
      while (another_curse)
	{
	  o_ptr->flags_curse |= 
	    (OBJECT_RAND_BASE_CURSE << rand_int(OBJECT_RAND_SIZE_CURSE));
	  if (rand_int(10) != 0) another_curse = FALSE;
	}

      
      /* Hack - only drop weapons */
      while ((o_ptr->flags_curse & CF_DROP_WEAPON) && (o_ptr->tval > TV_SWORD))
	{
	  o_ptr->flags_curse &= ~(CF_DROP_WEAPON);
	  o_ptr->flags_curse |= 
	    (OBJECT_RAND_BASE_CURSE << rand_int(OBJECT_RAND_SIZE_CURSE));
	}
    }
  
  /* Extra damage dice */
  if (flags_kind & KF_XTRA_DICE)
    {
      if (((o_ptr->dd + 1) * o_ptr->ds) < 41)
	{
	  o_ptr->dd += 1; 
	}
      else
	{
	  /* If we can't get an extra dice then weapon is lighter */
	  o_ptr->weight = (5 * o_ptr->weight / 6);
	}
    }

  if (flags_kind & KF_LIGHT)
    {
      /* Weapons */
      if (o_ptr->tval <= TV_SWORD) o_ptr->weight = (4 * o_ptr->weight / 5);

      /* Armour */
      else o_ptr->weight = (2 * o_ptr->weight / 3);
    }

  if (flags_kind & KF_HEAVY)
    {
      o_ptr->weight = o_ptr->weight * 3;
    }

  /* Add resistances */
  apply_resistances(o_ptr, lev, flags_kind);
  

  /* Extra bonuses for ego-items */
  if (o_ptr->name2)
    {
      ego_item_type *e_ptr = &e_info[o_ptr->name2];
      int av = 0, num = 0, chances;

      /* Get flags */
      o_ptr->flags_obj |= e_ptr->flags_obj;
      o_ptr->flags_curse |= e_ptr->flags_curse;
      
      /* Assign bonuses (random between base and ego) */
      for (i = 0; i < A_MAX; i++)
	{
	  if (e_ptr->bonus_stat[i] > 0)
	    o_ptr->bonus_stat[i] += randint(e_ptr->bonus_stat[i]);
	  else if (e_ptr->bonus_stat[i] < 0)
	    {
	      o_ptr->bonus_stat[i] -= randint(-e_ptr->bonus_stat[i]);
	      av += o_ptr->bonus_stat[i];
	      num++;
	    }
	}

      /* Calculate extra sides for, eg, Angband weapons */
      if (flags_kind & KF_XTRA_SIDES)
	{
	  /* Extra sides to compensate for stat penalties */
	  if (num) 
	    {
	      av /= num;
	      chances = (randint(1 - av) + 1 - av) / 2;
	      
	      for (i = 0; i < chances; i++)
		{
		  if (randint(3 - av) < (1 - av)) o_ptr->ds++;
		}
	    }
	  /* Mainly for bashing shields */
	  else
	    {
	      o_ptr->ds += m_bonus(2, lev);
	    }
	}
      for (i = 0; i < MAX_P_BONUS; i++)
	{
	  if (e_ptr->bonus_other[i] > 0)
	    o_ptr->bonus_other[i] += randint(e_ptr->bonus_other[i]);
	  else if (e_ptr->bonus_other[i] < 0)
	    o_ptr->bonus_other[i] -= randint(-e_ptr->bonus_other[i]);
	}

      /* Assign multiples (same as ego - for now) */
      for (i = 0; i < MAX_P_SLAY; i++)
	o_ptr->multiple_slay[i] = e_ptr->multiple_slay[i];
      for (i = 0; i < MAX_P_BRAND; i++)
	o_ptr->multiple_brand[i] = e_ptr->multiple_brand[i];
      
      
      /* Apply extra bonuses or penalties. */
      if ((e_ptr->max_to_h > 0) && (e_ptr->max_to_h < 129)) 
	o_ptr->to_h += randint(e_ptr->max_to_h);
      else if (e_ptr->max_to_h > 128)
	o_ptr->to_h -= randint(e_ptr->max_to_h - 128);
      
      if ((e_ptr->max_to_d > 0) && (e_ptr->max_to_d < 129)) 
	o_ptr->to_d += randint(e_ptr->max_to_d);
      else if (e_ptr->max_to_d > 128)
	o_ptr->to_d -= randint(e_ptr->max_to_d - 128);
      
      if ((e_ptr->max_to_a > 0) && (e_ptr->max_to_a < 129)) 
	o_ptr->to_a += randint(e_ptr->max_to_a);
      else if (e_ptr->max_to_a > 128)
	o_ptr->to_a -= randint(e_ptr->max_to_a - 128);
      
      
      /* Extra base armour class */
      if (flags_kind & KF_XTRA_AC)
	{
	  o_ptr->ac += 5;
	}
      
      /* Extra skill bonus */
      if (flags_kind & KF_XTRA_TO_H)
	{
	  if (o_ptr->to_h < 0) o_ptr->to_h = 0 - o_ptr->to_h;
	  o_ptr->to_h += m_bonus(10, lev);
	}
      
      /* Extra deadliness bonus */
      if (flags_kind & KF_XTRA_TO_D)
	{
	  if (o_ptr->to_d < 0) o_ptr->to_d = 0 - o_ptr->to_d;
	  o_ptr->to_d += m_bonus(10, lev);
	}

      /* Extra armour class bonus */
      if (flags_kind & KF_XTRA_TO_A)
	{
	  if (o_ptr->to_a < 0) o_ptr->to_a = 0 - o_ptr->to_a;
	  o_ptr->to_a += m_bonus(10, lev);
	}

      /* Hack -- Frequently neaten missile to_h and to_d, for improved 
       * stacking qualities.
       */
      {
	if ((o_ptr->to_h % 2 == 1) && (randint(4) != 1)) o_ptr->to_h++;
	if ((o_ptr->to_d % 2 == 1) && (randint(4) != 1)) o_ptr->to_d++;
      }
      
      
      /* Hack -- apply rating bonus */
      rating += e_ptr->rating;
      
      /* Cheat -- describe the item */
      if (cheat_peek) object_mention(o_ptr);
      
      /* Done */
      return;
    }
}


/**
 * Hack -- determine if a template is the right kind of object. -LM-
 */
static bool kind_is_right_kind(int k_idx)
{
  object_kind *k_ptr = &k_info[k_idx];
  
  /* We are only looking for non-worthless items of the tval asked for. */
  if ((k_ptr->tval == required_tval) && (k_ptr->cost > 0)) return (TRUE);
  else return (FALSE);
}

/**
 * Hack -- determine if a template is "good".  This concept has been consider-
 * ably expanded in Oangband. -LM-
 */
static bool kind_is_good(int k_idx)
{
  object_kind *k_ptr = &k_info[k_idx];
  
  /* Analyze the item type */
  switch (k_ptr->tval)
    {
      /* Armor -- Good unless damaged */
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
      {
	if (k_ptr->to_a < 0) return (FALSE);
	return (TRUE);
      }
      
      /* Weapons -- Good unless damaged.  Diggers are no longer good. */
    case TV_BOW:
    case TV_SWORD:
    case TV_HAFTED:
    case TV_POLEARM:
      {
	if (k_ptr->to_h < 0) return (FALSE);
	if (k_ptr->to_d < 0) return (FALSE);
	return (TRUE);
      }
      
      /* Ammo -- Shots/Arrows/Bolts are good. */
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
      {
	return (TRUE);
      }
      
      /* Books -- High level books are good */
    case TV_MAGIC_BOOK:
    case TV_PRAYER_BOOK:
    case TV_DRUID_BOOK:
    case TV_NECRO_BOOK:
      {
	if (k_ptr->sval >= SV_BOOK_MIN_GOOD) return (TRUE);
	return (FALSE);
      }
      
      /* Rings -- any with a high enough value. */
    case TV_RING:
      {
	
	if (k_ptr->cost >= 500 + p_ptr->depth * 90) return (TRUE);
	
	return (FALSE);
      }
      
      /* Amulets --  any with a high enough value. */
    case TV_AMULET:
      {
	if (k_ptr->cost >= 500 + p_ptr->depth * 90) return (TRUE);
	
	return (FALSE);
      }
      
      /* Chests are always good. */
    case TV_CHEST:
      {
	return (TRUE);
      }
      
      /* Wands of Dragon Fire, Cold, and Breath, and of Annihilation
       * are good, as are any with a high enough value.
       */
    case TV_WAND:
      {
	if (k_ptr->sval == SV_WAND_DRAGON_FIRE) return (TRUE);
	if (k_ptr->sval == SV_WAND_DRAGON_COLD) return (TRUE);
	if (k_ptr->sval == SV_WAND_DRAGON_BREATH) return (TRUE);
	if (k_ptr->sval == SV_WAND_ANNIHILATION) return (TRUE);
	if (k_ptr->sval == SV_WAND_TELEPORT_AWAY) return (TRUE);
		
	if (k_ptr->cost >= 400 + p_ptr->depth * 60) return (TRUE);
	
	return (FALSE);
      }
      
      /* Staffs of Power, Holiness, Mana Storm, and Starburst are good, 
       * as are any with a high enough value.
       */
    case TV_STAFF:
      {
	if (k_ptr->sval == SV_STAFF_POWER) return (TRUE);
	if (k_ptr->sval == SV_STAFF_HOLINESS) return (TRUE);
	if (k_ptr->sval == SV_STAFF_MSTORM) return (TRUE);
	if (k_ptr->sval == SV_STAFF_STARBURST) return (TRUE);
	if (k_ptr->sval == SV_STAFF_BANISHMENT) return (TRUE);
	
	if (k_ptr->cost >= 500 + p_ptr->depth * 60) return (TRUE);
	
	return (FALSE);
      }
      
      /* Rods of ID, Curing, Restoration, Speed, Healing, Lighting
       * Strike, Northwinds, Dragonfire, and Glaurung's Blood are all
       * good, as are any with a high enough value.
       */
    case TV_ROD:
      {
	if (k_ptr->sval == SV_ROD_CURING) return (TRUE);
	if (k_ptr->sval == SV_ROD_RESTORATION) return (TRUE);
	if (k_ptr->sval == SV_ROD_SPEED) return (TRUE);
	if (k_ptr->sval == SV_ROD_HEALING) return (TRUE);
	if (k_ptr->sval == SV_ROD_LIGHTINGSTRIKE) return (TRUE);
	if (k_ptr->sval == SV_ROD_NORTHWINDS) return (TRUE);
	if (k_ptr->sval == SV_ROD_DRAGONFIRE) return (TRUE);
	if (k_ptr->sval == SV_ROD_GLAURUNGS) return (TRUE);
	if (k_ptr->sval == SV_ROD_TELEPORT_AWAY) return (TRUE);
	
	if (k_ptr->cost >= 500 + p_ptr->depth * 100) return (TRUE);
	
	return (FALSE);
      }
      
      /*  Any potion or scroll that costs a certain amount or more 
       * must be good. */
    case TV_POTION:
      {
	if (k_ptr->sval == SV_POTION_HEALING) return (TRUE);
	if (k_ptr->sval == SV_POTION_STAR_HEALING) return (TRUE);
	if (k_ptr->sval == SV_POTION_LIFE) return (TRUE);
	if (k_ptr->sval == SV_POTION_RESTORE_MANA) return (TRUE);
      }
    case TV_SCROLL:
      {
	if (k_ptr->cost >= 10000) return (TRUE);
	if (k_ptr->cost >= 500 + p_ptr->depth * 110) return (TRUE);
	return (FALSE);
      }
    }
  
  /* Assume not good */
  return (FALSE);
}

/**
 * Attempt to make an object (normal, good/great, or with a specified tval)
 *
 * This routine plays nasty games to generate the "special artifacts".
 *
 * This routine uses "object_level" for the "generation level".
 *
 * We assume that the given object has been "wiped".
 *
 * Reject some poor items -LM-
 */
bool make_object(object_type *j_ptr, bool good, bool great, bool exact_kind)
{
  int prob, base;
  
  object_kind *k_ptr;
  
  /* Chance of "special object" - now depth dependant */
  prob = (1000 - (6 * object_level));
  
  /* Limit base chance of "special object" */
  if (prob < 700) prob = 700;
  
  /* Increase chance of "special object" for good items */
  if (good) prob = 10;
  
  /* Base level for the object.  Only "great" objects get this bonus 
   * in Oangband.
   */
  base = (great ? (object_level + 10) : object_level);
  
  
  /* If an object is rejected because it is sufficiently low-level and 
   * cheap to be of little possible interest, try again.
   */ 
 try_again:
  ;
  
  
  /* Generate a special object, or a normal object */
  if ((rand_int(prob) != 0) || !make_artifact_special(j_ptr))
    {
      int k_idx;
      
      object_kind *k_ptr;
      
      /* Good objects */
      if (good)
	{
	  /* Activate restriction */
	  get_obj_num_hook = kind_is_good;
	  
	  /* Prepare allocation table */
	  get_obj_num_prep();
	}
      
      /* Objects with a particular tval.  Only activate if there is a 
       * valid tval to restrict to. */
      if (exact_kind && required_tval)
	{
	  /* Activate restriction */
	  get_obj_num_hook = kind_is_right_kind;
	  
	  /* Prepare allocation table */
	  get_obj_num_prep();
	}
      
      /* get an object index */
      k_idx = get_obj_num(base);
      
      /* Handle failure */
      if (!k_idx)
	{
	  /* Clear any special object restriction, and prepare the standard
	   * object allocation table.
	   */
	  if (get_obj_num_hook)
	    {
	      /* Clear restriction */
	      get_obj_num_hook = NULL;
	      
	      /* Prepare allocation table */
	      get_obj_num_prep();
	    }
	  
	  /* No object was created. */
	  return (FALSE);
	}
      
      /* Get the object kind. */
      k_ptr = &k_info[k_idx];
      
      /* Usually (67% chance) forbid very low-level objects that are 
       * intrinsically worthless.
       */
      if ((k_ptr->level < object_level / 4) && 
	  (k_ptr->cost < 1) && 
	  (randint(3) != 1)) goto try_again;
      
      /* Sometimes (33% chance) forbid very low-level objects that aren't 
       * worth very much.
       */
      else if ((k_ptr->level < object_level / 4) && 
	       (k_ptr->cost < object_level - 20) && 
	       (randint(3) == 1)) goto try_again;
      
      /* Hack -- If a chest is specifically asked for, almost always 
       * winnow out any not sufficiently high-level.
       */
      if (required_tval == TV_CHEST)
	{
	  /* Need to avoid asking for the impossible. */
	  int tmp = object_level > 100 ? 100 : object_level;
	  
	  if ((k_ptr->level < 2 * (tmp + 3) / 4 + randint(tmp / 4)) && 
	      (randint(10) != 1)) goto try_again;
	}
      
      /* Clear any special object restriction, and prepare the standard 
       * object allocation table.
       */
      if (get_obj_num_hook)
	{
	  /* Clear restriction */
	  get_obj_num_hook = NULL;
	  
	  /* Prepare allocation table */
	  get_obj_num_prep();
	}
      
      /* Prepare the object */
      object_prep(j_ptr, k_idx);
    }
  
  /* Apply magic (allow artifacts) */
  apply_magic(j_ptr, object_level, TRUE, good, great);
  
  /* Get the object kind. */
  k_ptr = &k_info[j_ptr->k_idx];
  
  /* Hack - Certain object types appear in stacks. */
  switch (j_ptr->tval)
    {
    case TV_SPIKE:
      {
	j_ptr->number = damroll(6, 7);
	break;
      }
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	j_ptr->number = damroll(10, 7);
	break;
      }
    case TV_FOOD:
      {
	if ((k_ptr->cost < 150) && (randint(2) == 1)) 
	  j_ptr->number = damroll(2, 3);
	break;
      }
    case TV_HAFTED:
    case TV_SWORD:
    case TV_POLEARM:
      {
	if ((k_ptr->flags_obj & OF_THROWING) && (!j_ptr->name1) && 
	    (randint(2) == 1)) j_ptr->number = damroll(2, 2);
	break;
      }
    default:
      {
	break;
      }
    }
  
  /* Notice "okay" out-of-depth objects */
  if (!cursed_p(j_ptr) && (k_info[j_ptr->k_idx].level > p_ptr->depth))
    {
      /* Rating increase */
      rating += (k_info[j_ptr->k_idx].level - p_ptr->depth);
      
      /* Cheat -- peek at items */
      if (cheat_peek) object_mention(j_ptr);
    }
  
  /* Return */
  return(TRUE);
}



/**
 * Make a treasure object.  This function has been reworked in Oangband, 
 * but is still a bit of a hack.
 */
bool make_gold(object_type *j_ptr)
{
  int i;
  int treasure = 0;
  int gold_depth, first_gold_idx;
  
  /* Hack -- Creeping Coins only generate "themselves" */
  if (coin_type) treasure = coin_type;
  
  
  else
    {
      /* Hack -- Start looking at location of copper. */
      first_gold_idx = lookup_kind(TV_GOLD, SV_COPPER);
      
      /* Normal treasure level is between 1/2 and the full object level. */
      gold_depth = (object_level / 2) + randint((object_level + 1) / 2);
      
      /* Apply "extra" magic. */
      if (rand_int(GREAT_OBJ) == 0) gold_depth += randint(gold_depth);
      
      /* Find the highest-level legal gold object. */
      for (i = first_gold_idx; i < first_gold_idx + SV_GOLD_MAX; i++)
	{
	  /* Paranoia -- Skip objects without value. */
	  if (!k_info[i].cost) continue;
	  
	  /* Stop if we're going too deep. */
	  if (k_info[i].level > gold_depth) break;
	  
	  /* Otherwise, keep this index. */
	  treasure = i;
	}
    }
  
  /* Report failure.  This should not happen. */
  if (!treasure) return (FALSE);
  
  
  /* Prepare a gold object */
  object_prep(j_ptr, treasure);
  
  /* Treasure can be worth between 1/2 and the full maximal value. */
  j_ptr->pval = k_info[treasure].cost / 2 + 
    randint((k_info[treasure].cost + 1) / 2);

  /* Hack - double for no selling */
  if (adult_no_sell) j_ptr->pval *= 2;
  
  /* Success */
  return (TRUE);
}



/**
 * Let the floor carry an object
 */
s16b floor_carry(int y, int x, object_type *j_ptr)
{
  int n = 0;
  
  s16b o_idx;
  
  s16b this_o_idx, next_o_idx = 0;
  
  
  /* Scan objects in that grid for combination */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Check for combination */
      if (object_similar(o_ptr, j_ptr))
	{
	  /* Combine the items */
	  object_absorb(o_ptr, j_ptr);
	  
	  /* Result */
	  return (this_o_idx);
	}
      
      /* Count objects */
      n++;
    }
  
  /* Make an object */
  o_idx = o_pop();
  
  /* Success */
  if (o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[o_idx];
      
      /* Structure Copy */
      object_copy(o_ptr, j_ptr);
      
      /* Location */
      o_ptr->iy = y;
      o_ptr->ix = x;
      
      /* Forget monster */
      o_ptr->held_m_idx = 0;
      
      /* Build a stack */
      o_ptr->next_o_idx = cave_o_idx[y][x];
      
      /* Place the object */
      cave_o_idx[y][x] = o_idx;
      
      /* Notice */
      note_spot(y, x);
      
      /* Redraw */
      lite_spot(y, x);

      /* Redo item list */
      p_ptr->window |= PW_ITEMLIST;
    }
  
  /* Result */
  return (o_idx);
}


/**
 * Let an object fall to the ground at or near a location.
 *
 * The initial location is assumed to be "in_bounds_fully()".
 *
 * This function takes a parameter "chance".  This is the percentage
 * chance that the item will "disappear" instead of drop.  If the object
 * has been thrown, then this is the chance of disappearance on contact.
 *
 * Hack -- this function uses "chance" to determine if it should produce
 * some form of "description" of the drop event (under the player).
 *
 * We check several locations to see if we can find a location at which
 * the object can combine, stack, or be placed.   Artifacts will try very
 * hard to be placed, including "teleporting" to a useful grid if needed.
 */
void drop_near(object_type *j_ptr, int chance, int y, int x)
{
  int i, k, n, d, s;
  
  int bs, bn;
  int by, bx;
  int dy, dx;
  int ty, tx;
  
  s16b this_o_idx, next_o_idx = 0;
  
  char o_name[120];
  
  bool flag = FALSE;
  
  bool plural = FALSE;
  
  
  /* Extract plural */
  if (j_ptr->number != 1) plural = TRUE;

  /* Describe object */
  object_desc(o_name, j_ptr, FALSE, 0);
  
  /* Special case for treasure objects */
  if (j_ptr->tval == TV_GOLD)
    {
      i = 0;
      while (o_name[i] != '\0') i++;
      if (o_name[i - 1] == 's')
	plural = TRUE;
    }
  
  
  /* Handle normal "breakage" */
  if (!artifact_p(j_ptr) && (rand_int(100) < chance))
    {
      /* Message */
      msg_format("The %s disappear%s.",
		 o_name, (plural ? "" : "s"));
      
      /* Debug */
      if (p_ptr->wizard) msg_print("Breakage (breakage).");
      
      /* Failure */
      return;
    }
  
  
  /* Score */
  bs = -1;
  
  /* Picker */
  bn = 0;
  
  /* Default */
  by = y;
  bx = x;
  
  /* Scan local grids */
  for (dy = -3; dy < 4; dy++)
    {
      /* Scan local grids */
      for (dx = -3; dx < 4; dx++)
	{
	  bool comb = FALSE;
	  
	  /* Calculate actual distance */
	  d = (dy * dy) + (dx * dx);
	  
	  /* Ignore distant grids */
	  if (d > 10) continue;
	  
	  /* Location */
	  ty = y + dy;
	  tx = x + dx;
	  
	  /* Skip illegal grids */
	  if (!in_bounds_fully(ty, tx)) continue;
	  
	  /* Require line of sight */
	  if (!los(y, x, ty, tx)) continue;
	  
	  /* Require floor space */
	  if ((cave_feat[ty][tx] != FEAT_FLOOR) && 
	      (cave_feat[ty][tx] != FEAT_GRASS) &&
	      (cave_feat[ty][tx] != FEAT_TREE) &&
	      (cave_feat[ty][tx] != FEAT_TREE2) &&
	      (cave_feat[ty][tx] != FEAT_DUNE) &&
	      (cave_feat[ty][tx] != FEAT_RUBBLE)) continue;
	  
	  /* No objects */
	  k = 0;
	  n = 0;
	  
	  /* Scan objects in that grid */
	  for (this_o_idx = cave_o_idx[ty][tx]; this_o_idx; 
	       this_o_idx = next_o_idx)
	    {
	      object_type *o_ptr;
	      
	      /* Acquire object */
	      o_ptr = &o_list[this_o_idx];
	      
	      /* Acquire next object */
	      next_o_idx = o_ptr->next_o_idx;
	      
	      /* Check for possible combination */
	      if (object_similar(o_ptr, j_ptr)) comb = TRUE;
	      
	      /* Count objects */
	      if (!squelch_hide_item(o_ptr))
		k++;
	      else
		n++;
	    }
	  
	  /* Add new object */
	  if (!comb) k++;
	  
	  /* Paranoia? */
	  if ((k + n) > MAX_FLOOR_STACK) continue;

	  /* Calculate goodness of location, given distance from source of 
	   * drop and number of objects.  Hack - If the player is dropping 
	   * the item, encourage it to pile with up to 19 other items.
	   */
	  if (cave_m_idx[y][x] < 0)
	    {
	      s = 1000 - (d + (k > 20 ? k * 5 : 0));
	    }
	  else 
	    {
	      s = 1000 - (d + k * 5);
	    }
	  
	  /* Skip bad values */
	  if (s < bs) continue;
	  
	  /* New best value */
	  if (s > bs) bn = 0;
	  
	  /* Apply the randomizer to equivalent values */
	  if ((++bn >= 2) && (rand_int(bn) != 0)) continue;
	  
	  /* Keep score */
	  bs = s;
	  
	  /* Track it */
	  by = ty;
	  bx = tx;
	  
	  /* Okay */
	  flag = TRUE;
	}
    }
  
  
  /* Handle lack of space */
  if (!flag && !artifact_p(j_ptr))
    {
      /* Message */
      msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
      
      /* Debug */
      if (p_ptr->wizard) msg_print("Breakage (no floor space).");
      
      /* Failure */
      return;
    }
  
  
  /* Find a grid */
  for (i = 0; !flag; i++)
    {
      /* Bounce around */
      if (i < 1000)
	{
	  ty = rand_spread(by, 1);
	  tx = rand_spread(bx, 1);
	}
      
      /* Random locations */
      else
	{
	  ty = rand_int(DUNGEON_HGT);
	  tx = rand_int(DUNGEON_WID);
	}
      
      /* Require floor space */
      if ((cave_feat[ty][tx] != FEAT_FLOOR) && 
	  (cave_feat[ty][tx] != FEAT_GRASS) &&
	  (cave_feat[ty][tx] != FEAT_TREE) &&
	  (cave_feat[ty][tx] != FEAT_TREE2) &&
	  (cave_feat[ty][tx] != FEAT_DUNE) &&
	  (cave_feat[ty][tx] != FEAT_RUBBLE))continue;
      
      /* Bounce to that location */
      by = ty;
      bx = tx;
      
      /* Require floor space */
      if ((!cave_clean_bold(by, bx)) && (cave_feat[by][bx] != FEAT_TREE) &&
	  (cave_feat[by][bx] != FEAT_TREE2) && (cave_feat[by][bx] != FEAT_DUNE)
	  && (cave_feat[by][bx] != FEAT_RUBBLE)) 
	  continue;
      
      /* Okay */
      flag = TRUE;
    }
  
  
  /* Give it to the floor */
  if (!floor_carry(by, bx, j_ptr))
    {
      /* Message */
      msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
      
      /* Debug */
      if (p_ptr->wizard) msg_print("Breakage (too many objects).");
      
      /* Hack -- Preserve artifacts */
      a_info[j_ptr->name1].creat_turn = 0;
      
      /* Failure */
      return;
    }
  
  
  /* Sound */
  sound(MSG_DROP);

  /* No messages for squelched items */
  if (squelch_hide_item(j_ptr)) return;
  
  /* Mega-Hack -- no message if "dropped" by player */
  if (chance && (cave_m_idx[by][bx] < 0) )
    {
      msg_print("You feel something roll beneath your feet.");
    }

  /* Message when an object falls under the player or in trees or rubble */
  else if (((cave_feat[by][bx] == FEAT_TREE) || 
	    (cave_feat[by][bx] == FEAT_TREE2) || 
	    (cave_feat[by][bx] == FEAT_RUBBLE)) && (!p_ptr->blind))
    msg_format("The %s disappear%s from view.", o_name, (plural ? "" : "s"));
}


/**
 * Scatter some "great" objects near the player
 */
void acquirement(int y1, int x1, int num, bool great)
{
  object_type *i_ptr;
  object_type object_type_body;
  
  /* Acquirement */
  while (num--)
    {
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Make a good (or great) object */
      if (!make_object(i_ptr, TRUE, great, FALSE)) continue;
      
      /* Drop the object */
      drop_near(i_ptr, -1, y1, x1);
      
    }
}


/**
 * Attempt to place an object (normal or good/great) at the given location.
 */
void place_object(int y, int x, bool good, bool great, bool exact_kind)
{
  object_type *i_ptr;
  object_type object_type_body;
  
  /* Paranoia */
  if (!in_bounds(y, x)) return;
  
  /* Hack -- require clean floor space */
  if (!cave_clean_bold(y, x)) return;
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Wipe the object */
  object_wipe(i_ptr);
  
  /* Make an object */
  if (make_object(i_ptr, good, great, exact_kind))
    {
      /* Give it to the floor */
      if (!floor_carry(y, x, i_ptr))
	{
	  /* Hack -- Preserve artifacts */
	  a_info[i_ptr->name1].creat_turn = 0;
	}
    }
  
}


/**
 * Places a treasure (Gold or Gems) at given location
 */
void place_gold(int y, int x)
{
  object_type *i_ptr;
  object_type object_type_body;
  
  /* Paranoia */
  if (!in_bounds(y, x)) return;
  
  /* Require clean floor space */
  if (!cave_clean_bold(y, x)) return;
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Wipe the object */
  object_wipe(i_ptr);
  
  /* Make some gold */
  if (make_gold(i_ptr))
    {
      /* Give it to the floor */
      (void)floor_carry(y, x, i_ptr);
    }
}



/**
 * Hack -- instantiate a trap
 *
 * This function has been altered in Oangband to (in a hard-coded fashion) 
 * control trap level. -LM-
 */
void pick_trap(int y, int x)
{
  int feat = 0;
  feature_type *f_ptr = &f_info[cave_feat[y][x]];
  
  bool trap_is_okay = FALSE;
  
  /* Paranoia */
  if ((cave_feat[y][x] != FEAT_INVIS) && 
      (cave_feat[y][x] != FEAT_GRASS_INVIS) &&
      (cave_feat[y][x] != FEAT_TREE_INVIS) &&
      (cave_feat[y][x] != FEAT_TREE2_INVIS)) return;
  
  /* Try to create a trap appropriate to the level.  Make certain 
   * that at least one trap type can be made on any possible level.  
   * -LM- */
  while (!(trap_is_okay))
    {
      /* Pick at random. */
      feat = FEAT_TRAP_HEAD + rand_int(16);
      
      /* Assume legal until proven otherwise. */
      trap_is_okay = TRUE;
      
      /* Check legality. */
      switch (feat)
	{
	  /* trap doors */
	case FEAT_TRAP_HEAD + 0x00:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* Hack -- no trap doors on quest levels */
	    if (is_quest(p_ptr->stage)) trap_is_okay = FALSE;
	    
	    /* Hack -- no trap doors at the bottom of dungeons */
	    if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) && 
		(!stage_map[p_ptr->stage][DOWN]))  
	      trap_is_okay = FALSE;
	    
	    /* No trap doors at level 1 (instadeath risk). */
	    if (p_ptr->depth < 2) trap_is_okay = FALSE;
	    
	    /* Trap doors only in dungeons or normal wilderness */
	    if (stage_map[p_ptr->stage][STAGE_TYPE] > CAVE)
	      trap_is_okay = FALSE;
	    
	    /* Trap doors only in dungeons in ironman */
	    if ((adult_ironman) && (stage_map[p_ptr->stage][STAGE_TYPE] < CAVE))
	      trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* pits */
	case FEAT_TRAP_HEAD + 0x01:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* No pits at level 1 (instadeath risk). */
	    if (p_ptr->depth < 2) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* stat-reducing darts. */
	case FEAT_TRAP_HEAD + 0x02:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* No darts above level 8. */
	    if (p_ptr->depth < 8) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* discolored spots */
	case FEAT_TRAP_HEAD + 0x03:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* No discolored spots above level 5. */
	    if (p_ptr->depth < 5) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* gas trap */
	case FEAT_TRAP_HEAD + 0x04:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* Gas traps can exist anywhere. */
	    
	    break;
	  }
	  
	  /* summoning rune */
	case FEAT_TRAP_HEAD + 0x05:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* No summoning runes above level 3. */
	    if (p_ptr->depth < 3) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* dungeon alteration */
	case FEAT_TRAP_HEAD + 0x06:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* No dungeon alteration above level 20. */
	    if (p_ptr->depth < 20) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* alter char and reality */
	case FEAT_TRAP_HEAD + 0x07:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;

	    /* No alter char and reality above level 60. */
	    if (p_ptr->depth < 60) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* move player */
	case FEAT_TRAP_HEAD + 0x08:
	  {
	    
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;
   
	    break;
	  }
	  
	  /* arrow, bolt, or shot */
	case FEAT_TRAP_HEAD + 0x09:
	  {
	    /* No trees */
	    if (f_ptr->flags & TF_TREE) trap_is_okay = FALSE;
   
	    /* No arrow, bolt, or shots above level 4. */
	    if (p_ptr->depth < 4) trap_is_okay = FALSE;
	    
	    break;
	  }
	  
	  /* falling tree branch */
	case FEAT_TRAP_HEAD + 0x0A:
	  {
	    /* Need the right trees */
	    if (cave_feat[y][x] != FEAT_TREE_INVIS) trap_is_okay = FALSE;

	    break;
	  }

	  /* falling tree branch */
	case FEAT_TRAP_HEAD + 0x0B:
	  {
	    /* Need the right trees */
	    if (cave_feat[y][x] != FEAT_TREE2_INVIS) trap_is_okay = FALSE;

	    break;
	  }

	  /* Any other selection is not defined. */
	default:
	  {
	    trap_is_okay = FALSE;
	    
	    break;
	  }
	}
    }
  
  /* Activate the trap */
  cave_set_feat(y, x, feat);
}



/**
 * Places a random trap at the given location.
 *
 * The location must be a legal, naked, floor grid.
 *
 * Note that all traps start out as "invisible" and "untyped", and then
 * when they are "discovered" (by detecting them or setting them off),
 * the trap is "instantiated" as a visible, "typed", trap.
 */
void place_trap(int y, int x)
{
  int d, grass = 0, floor = 0;

  feature_type *f_ptr = &f_info[cave_feat[y][x]];
  
  /* Paranoia */
  if (!in_bounds(y, x)) return;

  /* Hack - handle trees */
  if ((f_ptr->flags & TF_TREE) && 
      (cave_o_idx[y][x] == 0) && (cave_m_idx[y][x] >= 0))
    {
      if (cave_feat[y][x] == FEAT_TREE)
	cave_set_feat(y, x, FEAT_TREE_INVIS);
      else if (cave_feat[y][x] == FEAT_TREE2)
	cave_set_feat(y, x, FEAT_TREE2_INVIS);
      return;
    }
  
  /* Require empty, clean, floor grid */
  if (!cave_naked_bold(y, x)) return;
  
  /* Adjacent grids vote for grass or floor */
  for (d = 0; d < 8; d++)
    {
      if (cave_feat[y + ddy_ddd[d]][x + ddx_ddd[d]] == FEAT_FLOOR)
	floor++;
      else if (cave_feat[y + ddy_ddd[d]][x + ddx_ddd[d]] == FEAT_GRASS)
	grass++;
    }
  
  /* Place an invisible trap */
  if (grass > floor)
    cave_set_feat(y, x, FEAT_GRASS_INVIS);
  else
    cave_set_feat(y, x, FEAT_INVIS);
  
}

/**
 * Place a secret door at the given location
 */
void place_secret_door(int y, int x)
{
  /* Create secret door */
  cave_set_feat(y, x, FEAT_SECRET);
}


/**
 * Place an unlocked door at the given location
 */
void place_unlocked_door(int y, int x)
{
  /* Create secret door */
  cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
}


/**
 * Place a random type of closed door at the given location.
 */
void place_closed_door(int y, int x)
{
  int tmp;
  
  /* Choose an object */
  tmp = rand_int(400);
  
  /* Closed doors (300/400) */
  if (tmp < 300)
    {
      /* Create closed door */
      cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
    }
  
  /* Locked doors (99/400) */
  else if (tmp < 399)
    {
      /* Create locked door */
      cave_set_feat(y, x, FEAT_DOOR_HEAD + randint(7));
    }
  
  /* Stuck doors (1/400) */
  else
    {
      /* Create jammed door */
      cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x08 + rand_int(8));
    }
}


/**
 * Place a random type of door at the given location.
 */
void place_random_door(int y, int x)
{
  int tmp;
  
  /* Choose an object */
  tmp = rand_int(1000);
  
  /* Open doors (300/1000) */
  if (tmp < 300)
    {
      /* Create open door */
      cave_set_feat(y, x, FEAT_OPEN);
    }
  
  /* Broken doors (100/1000) */
  else if (tmp < 400)
    {
      /* Create broken door */
      cave_set_feat(y, x, FEAT_BROKEN);
    }
  
  /* Secret doors (200/1000) */
  else if (tmp < 600)
    {
      /* Create secret door */
      cave_set_feat(y, x, FEAT_SECRET);
    }
  
  /* Closed, locked, or stuck doors (400/1000) */
  else
    {
      /* Create closed door */
      place_closed_door(y, x);
    }
}

/**
 * Describe the charges on an item in the inventory.
 */
void inven_item_charges(int item)
{
  object_type *o_ptr = &inventory[item];
  
  /* Require staff/wand */
  if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;
  
  /* Require known item */
  if (!object_known_p(o_ptr)) return;
  
  /* Multiple charges */
  if (o_ptr->pval != 1)
    {
      /* Print a message */
      msg_format("You have %d charges remaining.", o_ptr->pval);
    }
  
  /* Single charge */
  else
    {
      /* Print a message */
      msg_format("You have %d charge remaining.", o_ptr->pval);
    }
}


/**
 * Describe an item in the inventory.
 */
void inven_item_describe(int item)
{
  object_type *o_ptr = &inventory[item];
  
  char o_name[120];
  
  if (artifact_p(o_ptr) && object_known_p(o_ptr))
    {
      
      /* Get a description */
      object_desc(o_name, o_ptr, FALSE, 3);
      
      /* Print a message */
      msg_format("You no longer have the %s (%c).", 
		 o_name, index_to_label(item));
    }
  else
    {
      /* Get a description */
      object_desc(o_name, o_ptr, TRUE, 3);
      
      /* Print a message */
      msg_format("You have %s (%c).", o_name, index_to_label(item));
    }
}


/**
 * Increase the "number" of an item in the inventory
 */
void inven_item_increase(int item, int num)
{
  object_type *o_ptr = &inventory[item];
  
  /* Apply */
  num += o_ptr->number;
  
  /* Bounds check */
  if (num > 255) num = 255;
  else if (num < 0) num = 0;
  
  /* Un-apply */
  num -= o_ptr->number;
  
  /* Change the number and weight */
  if (num)
    {
      /* Add the number */
      o_ptr->number += num;
      
      /* Add the weight */
      p_ptr->total_weight += (num * o_ptr->weight);
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
      
      /* Recalculate mana XXX */
      p_ptr->update |= (PU_MANA);
      
      /* Combine the pack */
      p_ptr->notice |= (PN_COMBINE);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN | PW_EQUIP);
    }
}


/**
 * Erase an inventory slot if it has no more items
 */
void inven_item_optimize(int item)
{
  object_type *o_ptr = &inventory[item];
  
  /* Only optimize real items */
  if (!o_ptr->k_idx) return;
  
  /* Only optimize empty items */
  if (o_ptr->number) return;
  
  /* The item is in the pack */
  if (item < INVEN_WIELD)
    {
      int i;
      
      /* One less item */
      p_ptr->inven_cnt--;
      
      /* Slide everything down */
      for (i = item; i < INVEN_PACK; i++)
	{
	  /* Hack -- slide object */
	  (void)COPY(&inventory[i], &inventory[i+1], object_type);
	}
      
      /* Hack -- wipe hole */
      (void)WIPE(&inventory[i], object_type);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN);
    }
  
  /* The item is being wielded */
  else
    {
      /* One less item */
      p_ptr->equip_cnt--;
      
      /* Erase the empty slot */
      object_wipe(&inventory[item]);
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
      
      /* Recalculate torch */
      p_ptr->update |= (PU_TORCH);
      
      /* Recalculate mana XXX */
      p_ptr->update |= (PU_MANA);
      
      /* Window stuff */
      p_ptr->window |= (PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
    }
}


/**
 * Describe the charges on an item on the floor.
 */
void floor_item_charges(int item)
{
  object_type *o_ptr = &o_list[item];
  
  /* Require staff/wand */
  if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND)) return;
  
  /* Require known item */
  if (!object_known_p(o_ptr)) return;
  
  /* Multiple charges */
  if (o_ptr->pval != 1)
    {
      /* Print a message */
      msg_format("There are %d charges remaining.", o_ptr->pval);
    }
  
  /* Single charge */
  else
    {
      /* Print a message */
      msg_format("There is %d charge remaining.", o_ptr->pval);
    }
}



/**
 * Describe an item in the inventory.
 */
void floor_item_describe(int item)
{
  object_type *o_ptr = &o_list[item];
  
  char o_name[120];
  
  /* Get a description */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Print a message */
  msg_format("You see %s.", o_name);
}


/**
 * Increase the "number" of an item on the floor
 */
void floor_item_increase(int item, int num)
{
  object_type *o_ptr = &o_list[item];
  
  /* Apply */
  num += o_ptr->number;
  
  /* Bounds check */
  if (num > 255) num = 255;
  else if (num < 0) num = 0;
  
  /* Un-apply */
  num -= o_ptr->number;
  
  /* Change the number */
  o_ptr->number += num;
}


/**
 * Optimize an item on the floor (destroy "empty" items)
 */
void floor_item_optimize(int item)
{
  object_type *o_ptr = &o_list[item];
  
  /* Paranoia -- be sure it exists */
  if (!o_ptr->k_idx) return;
  
  /* Only optimize empty items */
  if (o_ptr->number) return;
  
  /* Delete the object */
  delete_object_idx(item);
}





/**
 * Check if we have space for an item in the pack without overflow
 */
bool inven_carry_okay(object_type *o_ptr)
{
  int j;
  
  /* Empty slot? */
  if (p_ptr->inven_cnt < INVEN_PACK - p_ptr->pack_size_reduce) return (TRUE);
  
  /* Similar slot? */
  for (j = 0; j < INVEN_PACK - p_ptr->pack_size_reduce; j++)
    {
      object_type *j_ptr = &inventory[j];
      
      /* Skip non-objects */
      if (!j_ptr->k_idx) continue;
      
      /* Check if the two items can be combined */
      if (object_similar(j_ptr, o_ptr)) return (TRUE);
    }
  
  /* Nope */
  return (FALSE);
}


/**
 * Add an item to the player's inventory, and return the slot used.
 *
 * If the new item can combine with an existing item in the inventory,
 * it will do so, using "object_similar()" and "object_absorb()", else,
 * the item will be placed into the "proper" location in the inventory.
 *
 * This function can be used to "over-fill" the player's pack, but only
 * once, and such an action must trigger the "overflow" code immediately.
 * Note that when the pack is being "over-filled", the new item must be
 * placed into the "overflow" slot, and the "overflow" must take place
 * before the pack is reordered, but (optionally) after the pack is
 * combined.  This may be tricky.  See "dungeon.c" for info.
 *
 * Note that this code must remove any location/stack information
 * from the object once it is placed into the inventory.
 */
s16b inven_carry(object_type *o_ptr)
{
  int i, j, k;
  int n = -1;
  
  object_type *j_ptr;
  
  
  /* Check for combining */
  for (j = 0; j < INVEN_PACK; j++)
    {
      j_ptr = &inventory[j];
      
      /* Skip non-objects */
      if (!j_ptr->k_idx) continue;
      
      /* Hack -- track last item */
      n = j;
      
      /* Check if the two items can be combined */
      if (object_similar(j_ptr, o_ptr))
	{
	  /* Combine the items */
	  object_absorb(j_ptr, o_ptr);
	  
	  /* Increase the weight */
	  p_ptr->total_weight += (o_ptr->number * o_ptr->weight);
	  
	  /* Recalculate bonuses */
	  p_ptr->update |= (PU_BONUS);
	  
	  /* Window stuff */
	  p_ptr->window |= (PW_INVEN);
	  
	  /* Success */
	  return (j);
	}
    }

  
  /* Paranoia */
  if (p_ptr->inven_cnt > INVEN_PACK) return (-1);
  
  
  /* Find an empty slot */
  for (j = 0; j <= INVEN_PACK; j++)
    {
      j_ptr = &inventory[j];
      
      /* Use it if found */
      if (!j_ptr->k_idx) break;
    }
  
  /* Use that slot */
  i = j;
  
  /* Apply an autoinscription */
  apply_autoinscription(o_ptr);

  /* Reorder the pack */
  if (i < INVEN_PACK)
    {
      s32b o_value, j_value;
      
      /* Get the "value" of the item */
      o_value = object_value(o_ptr);
      
      /* Scan every occupied slot */
      for (j = 0; j < INVEN_PACK; j++)
	{
	  j_ptr = &inventory[j];
	  
	  /* Use empty slots */
	  if (!j_ptr->k_idx) break;
	  
	  /* Hack -- readable books always come first */
	  if ((o_ptr->tval == mp_ptr->spell_book) &&
	      (j_ptr->tval != mp_ptr->spell_book)) break;
	  if ((j_ptr->tval == mp_ptr->spell_book) &&
	      (o_ptr->tval != mp_ptr->spell_book)) continue;
	  
	  /* Objects sort by decreasing type */
	  if (o_ptr->tval > j_ptr->tval) break;
	  if (o_ptr->tval < j_ptr->tval) continue;
	  
	  /* Non-aware (flavored) items always come last */
	  if (!object_aware_p(o_ptr)) continue;
	  if (!object_aware_p(j_ptr)) break;
	  
	  /* Objects sort by increasing sval */
	  if (o_ptr->sval < j_ptr->sval) break;
	  if (o_ptr->sval > j_ptr->sval) continue;
	  
	  /* Unidentified objects always come last */
	  if (!object_known_p(o_ptr)) continue;
	  if (!object_known_p(j_ptr)) break;
	  
	  /* Determine the "value" of the pack item */
	  j_value = object_value(j_ptr);
	  
	  /* Objects sort by decreasing value */
	  if (o_value > j_value) break;
	  if (o_value < j_value) continue;
	}
      
      /* Use that slot */
      i = j;
      
      /* Slide objects */
      for (k = n; k >= i; k--)
	{
	  /* Hack -- Slide the item */
	  object_copy(&inventory[k+1], &inventory[k]);
	}
      
      /* Wipe the empty slot */
      object_wipe(&inventory[i]);
    }
  
  
  /* Copy the item */
  object_copy(&inventory[i], o_ptr);
  
  /* Access new object */
  j_ptr = &inventory[i];
  
  /* Forget stack */
  j_ptr->next_o_idx = 0;
  
  /* Forget monster */
  j_ptr->held_m_idx = 0;
  
  /* Forget location */
  j_ptr->iy = j_ptr->ix = 0;
  
  /* No longer marked */
  j_ptr->marked = FALSE;
  
  /* Increase the weight */
  p_ptr->total_weight += (j_ptr->number * j_ptr->weight);
  
  /* Count the items */
  p_ptr->inven_cnt++;
  
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Combine and Reorder pack */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN);
  
  /* Return the slot */
  return (i);
}



/**
 * Take off (some of) a non-cursed equipment item
 *
 * Note that only one item at a time can be wielded per slot.
 *
 * Note that taking off an item when "full" may cause that item
 * to fall to the ground.
 *
 * Return the inventory slot into which the item is placed.
 */
s16b inven_takeoff(int item, int amt)
{
  int slot;
  
  object_type *o_ptr;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  cptr act;
  cptr act2 = "";
  
  char o_name[120];
  
  
  /* Get the item to take off */
  o_ptr = &inventory[item];
  
  /* Paranoia */
  if (amt <= 0) return (-1);
  
  /* Verify */
  if (amt > o_ptr->number) amt = o_ptr->number;
  
  /* Check to see if a Set Item is being removed */
  if (o_ptr->name1) 
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      if (a_ptr->set_no!=0)
	{
	  /* The object is part of a set. Check to see if rest of set
	   * is equiped and if so remove bonuses -GS-
	   */
	  if (check_set(a_ptr->set_no)) 
	    {
	      remove_set(a_ptr->set_no);
	    }
	}				
    }
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Obtain a local object */
  object_copy(i_ptr, o_ptr);
  
  /* Modify quantity */
  i_ptr->number = amt;
  
  /* Describe the object */
  object_desc(o_name, i_ptr, TRUE, 3);
  
  /* Took off weapon */
  if (item == INVEN_WIELD)
    {
      act = "You were wielding";
    }
  
  /* Took off bow */
  else if (item == INVEN_BOW)
    {
      act = "You were holding";
    }
  
  /* Took off light */
  else if (item == INVEN_LITE)
    {
      act= "You were holding";
    }
  
  /* Removed something from the quiver slots */
  else if ((item >= INVEN_Q0) && (item <= INVEN_Q9))
    {
      act = "You removed";
      act2 = " from your quiver";
    }
  
  /* Took off something */
  else
    {
      act = "You were wearing";
    }
  
  /* Modify, Optimize */
  inven_item_increase(item, -amt);
  inven_item_optimize(item);
  
  /* Carry the object */
  slot = inven_carry(i_ptr);
  
  /* Message */
  sound(MSG_WIELD);
  msg_format("%s %s (%c)%s.", act, o_name, index_to_label(slot), act2);
  
  /* Return slot */
  return (slot);
}




/**
 * Drop (some of) a non-cursed inventory/equipment item
 *
 * The object will be dropped "near" the current location
 */
void inven_drop(int item, int amt)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  object_type *o_ptr;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  char o_name[120];
  
  
  /* Access original object */
  o_ptr = &inventory[item];
  
  /* Error check */
  if (amt <= 0) return;
  
  /* Not too many */
  if (amt > o_ptr->number) amt = o_ptr->number;
  
  
  /* Take off equipment */
  if (item >= INVEN_WIELD)
    {
      /* Take off first */
      item = inven_takeoff(item, amt);
      
      /* Access original object */
      o_ptr = &inventory[item];
    }
  
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Obtain local object */
  object_copy(i_ptr, o_ptr);
  
  /* Distribute charges of wands or rods */
  distribute_charges(o_ptr, i_ptr, amt);
  
  /* Modify quantity */
  i_ptr->number = amt;
  
  /* Describe local object */
  object_desc(o_name, i_ptr, TRUE, 3);
  
  /* Message */
  msg_format("You drop %s (%c).", o_name, index_to_label(item));
  
  /* Drop it near the player */
  drop_near(i_ptr, 0, py, px);
  
  /* Modify, Describe, Optimize */
  inven_item_increase(item, -amt);
  inven_item_describe(item);
  inven_item_optimize(item);
}



/**
 * Combine items in the pack
 *
 * Note special handling of the "overflow" slot
 */
void combine_pack(void)
{
  int i, j, k;
  
  object_type *o_ptr;
  object_type *j_ptr;
  
  bool flag = FALSE;
  
  
  /* Combine the pack (backwards) */
  for (i = INVEN_PACK; i > 0; i--)
    {
      /* Get the item */
      o_ptr = &inventory[i];
      
      /* Skip empty items */
      if (!o_ptr->k_idx) continue;
      
      /* Scan the items above that item */
      for (j = 0; j < i; j++)
	{
	  /* Get the item */
	  j_ptr = &inventory[j];
	  
	  /* Skip empty items */
	  if (!j_ptr->k_idx) continue;
	  
	  /* Can we drop "o_ptr" onto "j_ptr"? */
	  if (object_similar(j_ptr, o_ptr))
	    {
	      /* Take note */
	      flag = TRUE;
	      
	      /* Add together the item counts */
	      object_absorb(j_ptr, o_ptr);
	      
	      /* One object is gone */
	      p_ptr->inven_cnt--;
	      
	      /* Slide everything down */
	      for (k = i; k < INVEN_PACK; k++)
		{
		  /* Hack -- slide object */
		  COPY(&inventory[k], &inventory[k+1], object_type);
		}
	      
	      /* Hack -- wipe hole */
	      object_wipe(&inventory[k]);
	      
	      /* Window stuff */
	      p_ptr->window |= (PW_INVEN);
	      
	      /* Done */
	      break;
	    }
	}
    }
  
  /* Message */
  if (flag) msg_print("You combine some items in your pack.");
}


/**
 * Reorder items in the pack.
 *
 * Note special handling of the "overflow" slot
 */
void reorder_pack(void)
{
  int i, j, k;
  
  s32b o_value;
  s32b j_value;
  
  object_type *o_ptr;
  object_type *j_ptr;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  bool flag = FALSE;
  
  
  /* Re-order the pack (forwards) */
  for (i = 0; i < INVEN_PACK; i++)
    {
      /* Mega-Hack -- allow "proper" over-flow */
      if ((i == INVEN_PACK) && (p_ptr->inven_cnt == INVEN_PACK)) break;
      
      /* Get the item */
      o_ptr = &inventory[i];
      
      /* Skip empty slots */
      if (!o_ptr->k_idx) continue;
      
      /* Get the "value" of the item */
      o_value = object_value(o_ptr);
      
      /* Scan every occupied slot */
      for (j = 0; j < INVEN_PACK; j++)
	{
	  /* Get the item already there */
	  j_ptr = &inventory[j];
	  
	  /* Use empty slots */
	  if (!j_ptr->k_idx) break;
	  
	  /* Hack -- readable books always come first */
	  if ((o_ptr->tval == mp_ptr->spell_book) &&
	      (j_ptr->tval != mp_ptr->spell_book)) break;
	  if ((j_ptr->tval == mp_ptr->spell_book) &&
	      (o_ptr->tval != mp_ptr->spell_book)) continue;
	  
	  /* Objects sort by decreasing type */
	  if (o_ptr->tval > j_ptr->tval) break;
	  if (o_ptr->tval < j_ptr->tval) continue;
	  
	  /* Non-aware (flavored) items always come last */
	  if (!object_aware_p(o_ptr)) continue;
	  if (!object_aware_p(j_ptr)) break;
	  
	  /* Objects sort by increasing sval */
	  if (o_ptr->sval < j_ptr->sval) break;
	  if (o_ptr->sval > j_ptr->sval) continue;
	  
	  /* Unidentified objects always come last */
	  if (!object_known_p(o_ptr)) continue;
	  if (!object_known_p(j_ptr)) break;
	  
	  /* Determine the "value" of the pack item */
	  j_value = object_value(j_ptr);
	  
	  /* Objects sort by decreasing value */
	  if (o_value > j_value) break;
	  if (o_value < j_value) continue;
	}
      
      /* Never move down */
      if (j >= i) continue;
      
      /* Take note */
      flag = TRUE;
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Save a copy of the moving item */
      object_copy(i_ptr, &inventory[i]);
      
      /* Slide the objects */
      for (k = i; k > j; k--)
	{
	  /* Slide the item */
	  object_copy(&inventory[k], &inventory[k-1]);
	}
      
      /* Insert the moving item */
      object_copy(&inventory[j], i_ptr);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN);
    }
  
  /* Message */
  if (flag) msg_print("You reorder some items in your pack.");
}



/**
 * Count number of missiles in the quiver slots.
 */
int quiver_count(void)
{
  int i;
  int ammo_num = 0;
  
  /* Scan the slots */
  for (i = INVEN_Q0; i <= INVEN_Q9; i++)
    {
      /* Get the item */
      object_type *i_ptr = &inventory[i];
      
      /* Ignore empty. */
      if (!i_ptr->k_idx) continue;
      
      /* Tally up thrown weapons and missiles. */
      if ((i_ptr->flags_obj & OF_THROWING) && (!is_missile(i_ptr)) &&
	  (i_ptr->tval <= TV_SWORD))
	ammo_num += i_ptr->number * THROWER_AMMO_FACTOR;
      else
	ammo_num += i_ptr->number;
    }
  
  /* Return */
  return (ammo_num);
}


/**
 * Calculate and apply the reduction in pack size due to use of the
 * Quiver.
 */
void find_quiver_size(void)
{
  int ammo_num = quiver_count();
  
  /* Every 99 missile-equivalents in the quiver takes up one backpack slot. */
  p_ptr->pack_size_reduce =
    (ammo_num + MAX_STACK_SIZE - 2) / (MAX_STACK_SIZE - 1);
}

/**
 * Update (combine and sort ammo in) the quiver.  If requested, find 
 * the right slot to put new ammo in, make it available, and return it.
 *
 * Items marked with inscriptions of the form "@ [any letter or none] 
 * [any digit]" ("@f4", "@4", etc.) will be placed in the slot the 
 * digit corresponds to.  Everything else will be sorted around them.
 */
int process_quiver(int num_new, object_type *o_ptr)
{
  int i, j, k;
  
  int slot=0;
  
  s32b i_value;
  s32b j_value;
  
  object_type *i_ptr;
  object_type *j_ptr;
  
  object_type object_type_body;
  
  bool flag = FALSE;
  
  bool track = FALSE;
  
  bool temp_slot=FALSE;
  
  /* Combine the quiver (backwards) */
  for (i = INVEN_Q9; i > INVEN_Q0; i--)
    {
      /* Get the item */
      i_ptr = &inventory[i];
      
      /* Skip empty items */
      if (!i_ptr->k_idx) continue;
      
      /* Scan the items above that item */
      for (j = INVEN_Q0; j < i; j++)
	{
	  /* Get the item */
	  j_ptr = &inventory[j];
	  
	  /* Skip empty items */
	  if (!j_ptr->k_idx) continue;
	  
	  /* Can we drop "i_ptr" onto "j_ptr"? */
	  if (object_similar(j_ptr, i_ptr))
	    {
	      /* Take note */
	      flag = TRUE;
	      
	      /* Add together the item counts */
	      object_absorb(j_ptr, i_ptr);

	      /* One object is gone */
	      p_ptr->equip_cnt--;
	      
	      /* Slide everything down */
	      for (k = i; k < INVEN_Q9; k++)
		{
		  /* Hack -- slide object */
		  (void)COPY(&inventory[k], &inventory[k+1], object_type);
		}
	      
	      /* Hack -- wipe hole */
	      object_wipe(&inventory[k]);
	      
	      /* Window stuff */
	      p_ptr->window |= (PW_EQUIP);
	      
	      /* Done */
	      break;
	    }
	}
    }
  
  /* If requested, find a slot for new ammo. */
  if (num_new)
    {
      
      /* Search for available slots. */
      for (i = INVEN_Q0; i <= INVEN_Q9; i++)
	{
	  /* Get the item */
	  i_ptr = &inventory[i];
	  
	  /* Accept empty slot */
	  if (!i_ptr->k_idx)
	    {
	      slot = i;
	      temp_slot = TRUE;
	      continue;
	    }
	  
	  /* Accept slot that has space to absorb more */
	  if ((object_similar(o_ptr, i_ptr)) && 
	      ((num_new + i_ptr->number) < 100))
	    {
	      slot = i;
	      temp_slot = FALSE;
	      break;
	    }
	}
      
      /* TEMPORARILY put the new ammo in the quiver for sorting. */
      if (temp_slot)
	{
	  object_copy(&inventory[slot], o_ptr);
	}
      
    }
  
  /* Re-order the quiver (forwards) */
  for (i = INVEN_Q0; i <= INVEN_Q9; i++)
    {
      /* Get the item */
      i_ptr = &inventory[i];
      
      /* Skip empty slots */
      if (!i_ptr->k_idx) continue;
      
      /* Get the "value" of the item */
      i_value = object_value(i_ptr);
      
      /* Scan every occupied slot */
      for (j = INVEN_Q0; j <= INVEN_Q9; j++)
	{
	  /* Get the item already there */
	  j_ptr = &inventory[j];
	  
	  /* Use empty slots */
	  if (!j_ptr->k_idx) break;
	  
	  /* Objects sort by decreasing type */
	  if (i_ptr->tval > j_ptr->tval) break;
	  if (i_ptr->tval < j_ptr->tval) continue;
	  
	  /* Non-aware (flavored) items always come last */
	  if (!object_aware_p(i_ptr)) continue;
	  if (!object_aware_p(j_ptr)) break;
	  
	  /* Objects sort by increasing sval */
	  if (i_ptr->sval < j_ptr->sval) break;
	  if (i_ptr->sval > j_ptr->sval) continue;
	  
	  /* Unidentified objects always come last */
	  if (!object_known_p(i_ptr)) continue;
	  if (!object_known_p(j_ptr)) break;
	  
	  /* Determine the "value" of the pack item */
	  j_value = object_value(j_ptr);
	  
	  /* Objects sort by decreasing value */
	  if (i_value > j_value) break;
	  if (i_value < j_value) continue;
	}
      
      /* Never move down */
      if (j >= i) continue;
      
      /* Take note */
      flag = TRUE;
      
      /* Get local object */
      j_ptr = &object_type_body;
      
      /* Save a copy of the moving item */
      object_copy(j_ptr, &inventory[i]);
      
      /* Keep track of 'new' item */
      if (slot == i)
	{
	  slot = j;
	  track = TRUE;
	}
      
      /* Slide the objects */
      for (k = i; k > j;)
	{
	  /* Slide the item */
	  object_copy(&inventory[k], &inventory[k - 1]);
	  
	  /* Keep track of 'new' item */
	  if ((slot == k - 1) && (!track)) slot = k;
	  
	  /* Move down to the next alterable slot. */
	  k -= 1;
	}
      
      /* Helper for tracking 'new' ammo */
      track = FALSE;
      
      /* Insert the moving item */
      object_copy(&inventory[j], j_ptr);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN | PW_EQUIP);
    }
  
  /* Remove temporary ammo.  Will be added for real later. */
  if (temp_slot) object_wipe(&inventory[slot]);
  
  /* Calculate backpack reduction */
  find_quiver_size();
  
  /* Message */
  if ((!num_new) && (flag)) msg_print("You reorganize your quiver.");
  
  return (slot);
  
}




/**
 * Hack -- display an object kind in the current window
 *
 * Include list of usable spells for readable books
 */
void display_koff(int k_idx)
{
  int y;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  char o_name[120];
  
  
  /* Erase the window */
  for (y = 0; y < Term->hgt; y++)
    {
      /* Erase the line */
      Term_erase(0, y, 255);
    }
  
  /* No info */
  if (!k_idx) return;
  
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Prepare the object */
  object_wipe(i_ptr);
  
  /* Prepare the object */
  object_prep(i_ptr, k_idx);
  
  
  /* Describe */
  object_desc_store(o_name, i_ptr, FALSE, 0);
  
  /* Mention the object name */
  Term_putstr(0, 0, -1, TERM_WHITE, o_name);
  
  
  /* Warriors are illiterate */
  if (!mp_ptr->spell_book) return;

  /* Display spells in readible books */
  if (i_ptr->tval == mp_ptr->spell_book)
    {
      /* Print spells */
      print_spells(i_ptr->tval, i_ptr->sval, 1, (small_screen ? 0 : 14));
    }
}

/* Object kind lookup functions */

/**
 * Find the tval and sval of object kind `k_idx`, and return via the pointers
 * `tval` and `sval`.
 */
bool lookup_reverse(s16b k_idx, int *tval, int *sval)
{
	object_kind *k_ptr;

	/* Validate k_idx */
	if ((k_idx < 1) || (k_idx > z_info->k_max))
		return FALSE;

	/* Get pointer */
	k_ptr = &k_info[k_idx];
	*tval = k_ptr->tval;
	*sval = k_ptr->sval;

	/* Done */
	return TRUE;
}


/*** Textual<->numeric conversion */

/**
 * List of { tval, name } pairs.
 */
static const grouper tval_names[] =
{
	{ TV_SKELETON,    "skeleton" },
	{ TV_BOTTLE,      "bottle" },
	{ TV_JUNK,        "junk" },
	{ TV_SPIKE,       "spike" },
	{ TV_CHEST,       "chest" },
	{ TV_SHOT,        "shot" },
	{ TV_ARROW,       "arrow" },
	{ TV_BOLT,        "bolt" },
	{ TV_BOW,         "bow" },
	{ TV_DIGGING,     "digger" },
	{ TV_HAFTED,      "hafted" },
	{ TV_POLEARM,     "polearm" },
	{ TV_SWORD,       "sword" },
	{ TV_BOOTS,       "boots" },
	{ TV_GLOVES,      "gloves" },
	{ TV_HELM,        "helm" },
	{ TV_CROWN,       "crown" },
	{ TV_SHIELD,      "shield" },
	{ TV_CLOAK,       "cloak" },
	{ TV_SOFT_ARMOR,  "soft armour" },
	{ TV_HARD_ARMOR,  "hard armour" },
	{ TV_DRAG_ARMOR,  "dragon armour" },
	{ TV_LITE,        "light" },
	{ TV_AMULET,      "amulet" },
	{ TV_RING,        "ring" },
	{ TV_STAFF,       "staff" },
	{ TV_WAND,        "wand" },
	{ TV_ROD,         "rod" },
	{ TV_SCROLL,      "scroll" },
	{ TV_POTION,      "potion" },
	{ TV_FLASK,       "flask" },
	{ TV_FOOD,        "food" },
	{ TV_MAGIC_BOOK,  "magic book" },
	{ TV_PRAYER_BOOK, "prayer book" },
	{ TV_DRUID_BOOK,  "stones of lore" },
	{ TV_NECRO_BOOK,  "necromantic tomes" },
	{ TV_GOLD,        "gold" },
};

/**
 * Return the k_idx of the object kind with the given `tval` and name `name`.
 */
int lookup_name(int tval, const char *name)
{
	int k;

	/* Look for it */
	for (k = 1; k < z_info->k_max; k++)
	{
		object_kind *k_ptr = &k_info[k];
		const char *nm = k_name + k_ptr->name;

		if (*nm == '&' && *(nm+1))
			nm += 2;

		/* Found a match */
		if (k_ptr->tval == tval && !strcmp(name, nm))
			return k;
	}

	msg_format("No object (\"%s\",\"%s\")", tval_find_name(tval), name);
	return -1;
}

/**
 * Return the a_idx of the artifact with the given name
 */
int lookup_artifact_name(const char *name)
{
	int i;
	
	/* Look for it */
	for (i = 1; i < z_info->a_max; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		const char *nm = a_name + a_ptr->name;
		
		/* Found a match */
		if (streq(name, nm))
			return i;
		
	} 
	
	return -1;
}


/**
 * Return the numeric sval of the object kind with the given `tval` and name `name`.
 */
int lookup_sval(int tval, const char *name)
{
	int k;

	/* Look for it */
	for (k = 1; k < z_info->k_max; k++)
	{
		object_kind *k_ptr = &k_info[k];
		const char *nm = k_name + k_ptr->name;

		if (*nm == '&' && *(nm+1))
			nm += 2;

		/* Found a match */
		if (k_ptr->tval == tval && !strcmp(name, nm))
			return k_ptr->sval;
	}

	msg_format("No object (\"%s\",\"%s\")", tval_find_name(tval), name);
	return -1;
}

/**
 * Returns the numeric equivalent tval of the textual tval `name`.
 */
int tval_find_idx(const char *name)
{
	size_t i = 0;

	for (i = 0; i < N_ELEMENTS(tval_names); i++)
	{
		if (!my_stricmp(name, tval_names[i].name))
			return tval_names[i].tval;
	}

	return -1;
}

/**
 * Returns the textual equivalent tval of the numeric tval `name`.
 */
const char *tval_find_name(int tval)
{
	size_t i = 0;

	for (i = 0; i < N_ELEMENTS(tval_names); i++)
	{
		if (tval == tval_names[i].tval)
			return tval_names[i].name;
	}

	return "unknown";
}

/**
 * Sort comparator for objects using only tval and sval.
 * -1 if o1 should be first
 *  1 if o2 should be first
 *  0 if it doesn't matter
 */
static int compare_types(const object_type *o1, const object_type *o2)
{
  if (o1->tval == o2->tval)
    if (o1->sval > o2->sval) return 1;
    else return -1;
  else
    if (o1->tval > o2->tval) return 1;
    else return -1;
}

/**
 * Sort comparator for objects
 * -1 if o1 should be first
 *  1 if o2 should be first
 *  0 if it doesn't matter
 *
 * The sort order is designed with the "list items" command in mind.
 */
static int compare_items(object_type *o1, object_type *o2)
{
	/* known artifacts will sort first */
  if (artifact_p(o1) && object_known_p(o1) && 
      artifact_p(o2) && object_known_p(o1))
    return compare_types(o1, o2);
  if (artifact_p(o1) && object_known_p(o1)) return -1;
  if (artifact_p(o2) && object_known_p(o2)) return 1;

  /* unknown objects will sort next */
  if (!object_aware_p(o1) && !object_aware_p(o2))
    return compare_types(o1, o2);
  if (!object_aware_p(o1)) return -1;
  if (!object_aware_p(o2)) return 1;

  /* if only one of them is worthless, the other comes first */
  if ((object_value(o1) == 0) && (object_value(o2) > 0)) return 1;
  if ((object_value(o1) > 0) && (object_value(o2) == 0)) return 1;

  /* otherwise, just compare tvals and svals */
  /* NOTE: arguably there could be a better order than this */
  return compare_types(o1, o2);
}

/**
 * Display visible items, similar to display_monlist
 */
void display_itemlist(void)
{
  int max;
  int mx, my;
  int num;
  int line = 1, x = 0;
  int cur_x;
  unsigned i;
  unsigned disp_count = 0;
  byte a;
  char c;
  
  object_type *types[MAX_ITEMLIST];
  int counts[MAX_ITEMLIST];
  unsigned counter = 0;
  
  byte attr;
  char buf[80];
  
  int floor_list[MAX_FLOOR_STACK];
  
  /* Clear the term if in a subwindow, set x otherwise */
  if (Term != angband_term[0])
    {
      clear_from(0);
      max = Term->hgt - 1;
    }
  else
    {
      x = 13;
      max = Term->hgt - 2;
    }
  
  /* Look at each square of the dungeon for items */
  for (my = 0; my < DUNGEON_HGT; my++)
    {
      for (mx = 0; mx < DUNGEON_WID; mx++)
	{
	  (void) scan_floor(floor_list, &num, my, mx, 0x02);
	  
	  /* Iterate over all the items found on this square */
	  for (i = 0; i < num; i++)
	    {
	      object_type *o_ptr = &o_list[floor_list[i]];
	      unsigned j;
	      
	      /* Skip gold/squelched */
	      if (o_ptr->tval == TV_GOLD || squelch_hide_item(o_ptr))
		continue;
	      
	      /* See if we've already seen a similar item; if so, just add */
	      /* to its count */
	      for (j = 0; j < counter; j++)
		{
		  if (object_similar(o_ptr, types[j]))
		    {
		      counts[j] += o_ptr->number;
		      break;
		    }
		}
	      
	      /* We saw a new item. So insert it at the end of the list and */
	      /* then sort it forward using compare_items(). The types list */
	      /* is always kept sorted. */
	      if (j == counter)
		{
		  types[counter] = o_ptr;
		  counts[counter] = o_ptr->number;
		  
		  while (j > 0 && compare_items(types[j - 1], types[j]) > 0)
		    {
		      object_type *tmp_o = types[j - 1];
		      int tmpcount;
		      
		      types[j - 1] = types[j];
		      types[j] = tmp_o;
		      tmpcount = counts[j - 1];
		      counts[j - 1] = counts[j];
		      counts[j] = tmpcount;
		      j--;
		    }
		  counter++;
		}
	    }
	}
    }
  
  /* Note no visible items */
  if (!counter)
    {
      /* Clear display and print note */
      c_prt(TERM_SLATE, "You see no items.", 0, 0);
      if (Term == angband_term[0])
	Term_addstr(-1, TERM_WHITE, "  (Press any key to continue.)");
      
      /* Done */
      return;
    }
  else
    {
      /* Reprint Message */
      prt(format("You can see %d item%s:",
		 counter, (counter > 1 ? "s" : "")), 0, 0);
    }
  
  for (i = 0; i < counter; i++)
    {
      /* o_name will hold the object_desc() name for the object. */
      /* o_desc will also need to put a (x4) behind it. */
      /* can there be more than 999 stackable items on a level? */
      char o_name[80];
      char o_desc[86];
      
      object_type *o_ptr = types[i];
      
      /* We shouldn't list coins or squelched items */
      if (o_ptr->tval == TV_GOLD || squelch_hide_item(o_ptr))
	continue;
      
      object_desc(o_name, o_ptr, 0, 3);
      if (counts[i] > 1)
	sprintf(o_desc, "%s (x%d)", o_name, counts[i]);
      else
	sprintf(o_desc, "%s", o_name);
      
      /* Reset position */
      cur_x = x;
      
      /* See if we need to scroll or not */
      if (Term == angband_term[0] && (line == max) && disp_count != counter)
	{
	  prt("-- more --", line, x);
	  anykey();
	  
	  /* Clear the screen */
	  for (line = 1; line <= max; line++)
	    prt("", line, x);
	  
	  /* Reprint Message */
	  prt(format("You can see %d item%s:",
		     counter, (counter > 1 ? "s" : "")), 0, 0);
	  
	  /* Reset */
	  line = 1;
	}
      else if (line == max)
	{
	  continue;
	}
      
      /* Note that the number of items actually displayed */
      disp_count++;
      
      if (artifact_p(o_ptr) && object_known_p(o_ptr))
	/* known artifact */
	attr = TERM_VIOLET;
      else if (!object_aware_p(o_ptr))
	/* unaware of kind */
	attr = TERM_RED;
      else if (object_value(o_ptr) == 0)
	/* worthless */
	attr = TERM_SLATE;
      else
	/* default */
	attr = TERM_WHITE;
      
      a = object_type_attr(o_ptr->k_idx);
      c = object_type_char(o_ptr->k_idx);
      
      /* Display the pict */
      Term_putch(cur_x++, line, a, c);
      if (use_bigtile) Term_putch(cur_x++, line, 255, -1);
      Term_putch(cur_x++, line, TERM_WHITE, ' ');
      
      /* Print and bump line counter */
      c_prt(attr, o_desc, line, cur_x);
      line++;
    }
  
  if (disp_count != counter)
    {
      /* Print "and others" message if we've run out of space */
      strnfmt(buf, sizeof buf, "  ...and %d others.", counter - disp_count);
      c_prt(TERM_WHITE, buf, line, x);
    }
  else
    {
      /* Otherwise clear a line at the end, for main-term display */
      prt("", line, x);
    }
  
  if (Term == angband_term[0])
    Term_addstr(-1, TERM_WHITE, "  (Press any key to continue.)");
}

