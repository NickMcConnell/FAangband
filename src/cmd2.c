/** \file cmd2.c 
    \brief Commands, part 2

 * Going up and down stairs, items that a chest may contain, opening 
 * chests, tunnelling, disarming, opening doors, alter adjacent grid, 
 * spiking, starting various movement and resting routines, moving house.
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
 * Move house to the current town
 */

void do_cmd_move_house(void)
{
  char buf[80];
  int old_home = 0, new_home = 0, i = 0;
  store_type temp;
  
  cptr town = locality_name[stage_map[p_ptr->stage][LOCALITY]];
  
  if (!p_ptr->depth)
    {
      /* Already home */
      if (p_ptr->stage == p_ptr->home)
	{
	  msg_print("You already live here!");
	  flush();
	  return;
	}

      /* Check */
      sprintf(buf, "Do you really want to move to %s?", town);
      if (!get_check(buf)) return;

      /* No need to move for thrall mode */
      if (p_ptr->home)
	{
	  /* Get the current home */
	  while (1)
	    {
	      while (type_of_store[old_home] != STORE_HOME) old_home++;
	      if (p_ptr->home == towns[i]) break;
	      old_home++;
	      i++;
	    }
	  i = 0;
	  
	  /* Get the new home */
	  while (1)
	    {
	      while (type_of_store[new_home] != STORE_HOME) new_home++;
	      if (p_ptr->stage == towns[i]) break;
	      new_home++;
	      i++;
	    }
	  
	  /* Transfer the gear */
	  temp = store[new_home];
	  store[new_home] = store[old_home];
	  store[old_home] = temp;
	}

      /* Set the new town */
      p_ptr->home = p_ptr->stage;
      msg_print("Your home will be here when you return.");

      /* Moved house */
      sprintf(buf, "Moved house to %s.", town);
      
      /* Write message */
      make_note(buf,  p_ptr->stage, NOTE_MOVE, p_ptr->lev);
      
    }
  else
    msg_print("You can only move to another town!");
  
  /* Flush messages */
  flush();
  
  return;
}


/**
 * Go to less danger
 */
void do_cmd_go_up(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  byte pstair = cave_feat[py][px];
  feature_type *f_ptr = &f_info[pstair];
  
  /* Check for appropriate terrain  */
  if (!(f_ptr->flags & TF_STAIR))
    {
      msg_print("I see no path or staircase here.");
      return;
    }
  /* Even for < */
  else if (pstair & 0x01)
    {
      if (pstair > FEAT_TRAP_HEAD)
    {
      msg_print("This is a path to greater danger.");
      return;
    }
      else
	{
	  msg_print("This staircase leads down.");
	  return;
	}
    }
  
  /* Handle ironman */
  if (adult_ironman)
    {
      int next = stage_map[p_ptr->stage][2 + (pstair - FEAT_LESS_NORTH)/2];

      /* Upstairs */
      if (pstair < FEAT_LESS_NORTH)
	{
	  msg_print("Nothing happens.");
	  return;
	}
      else 
	{
	  /* New towns are OK */
	  if ((next == p_ptr->last_stage) || stage_map[next][DEPTH]) 
	    {
	      msg_print("Nothing happens.");
	      return;
	    }
	}
    }

  /* Make certain the player really wants to leave a themed level. -LM- */
  if (p_ptr->themed_level)
    if (!get_check("This level will never appear again.  Really leave?")) 
      return;
      
  
  /* Hack -- take a turn */
  p_ptr->energy_use = 100;
  
   /* Success */
  if (pstair == FEAT_LESS)
    {
      /* Magical portal for dungeon-only games */
      if (adult_dungeon && (p_ptr->depth != 1) &&
	  ((stage_map[p_ptr->stage][LOCALITY]) !=
	   (stage_map[stage_map[p_ptr->stage][UP]][LOCALITY])))
	{
	  /* Land properly */
	  p_ptr->last_stage = NOWHERE;
	  
	  /* Portal */
	  message(MSG_STAIRS_UP, 0, "You trigger a magic portal.");
	  
	  /* New stage */
	  p_ptr->stage = stage_map[p_ptr->stage][UP];
	  
	  /* New depth */
	  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
	  
	  /* Leaving */
	  p_ptr->leaving = TRUE;
	  
	  return;
	}
  
      /* stairs */
      message(MSG_STAIRS_DOWN, 0, "You enter a maze of up staircases.");
      
      /* make the way back */
      p_ptr->create_stair = FEAT_MORE;
    }
  else if (pstair == FEAT_LESS_SHAFT)
    {
      /* Magical portal for dungeon-only games */
      if (adult_dungeon && (p_ptr->depth != 2) &&
	  ((stage_map[p_ptr->stage][LOCALITY]) !=
	   (stage_map[stage_map[stage_map[p_ptr->stage][UP]][UP]][LOCALITY])))
	{
	  /* Land properly */
	  p_ptr->last_stage = NOWHERE;

	  /* Portal */
	  message(MSG_STAIRS_UP, 0, "You trigger a magic portal.");

	  /* New stage */
	  p_ptr->stage = stage_map[stage_map[p_ptr->stage][UP]][UP];
	  
	  /* New depth */
	  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
	  
	  /* Leaving */
	  p_ptr->leaving = TRUE;

	  return;
	}

      /* shaft */
      message(MSG_STAIRS_DOWN, 0, "You enter a maze of up staircases.");

      /* make the way back */
      p_ptr->create_stair = FEAT_MORE_SHAFT;
    }
  else
    {
      /* path */
      message(MSG_STAIRS_DOWN, 0, "You enter a winding path to less danger.");
      
      /* make the way back */
      p_ptr->create_stair = pstair ^ 0x05;
    }
  
  /* Remember where we came from */
  p_ptr->last_stage = p_ptr->stage;
  
  /* New stage (really need a check here...) */
  if (pstair >= FEAT_LESS_NORTH)
    p_ptr->stage = stage_map[p_ptr->stage][2 + (pstair - FEAT_LESS_NORTH)/2];
  else if (pstair == FEAT_LESS_SHAFT)
    p_ptr->stage = stage_map[stage_map[p_ptr->stage][UP]][UP];
  else
    p_ptr->stage = stage_map[p_ptr->stage][UP];

  /* Handle underworld stuff */
  if (stage_map[p_ptr->last_stage][LOCALITY] == UNDERWORLD)
    {  
      /* Reset */
      stage_map[255][UP] = 0;
      stage_map[p_ptr->stage][DOWN] = 0;
      stage_map[255][DEPTH] = 0;

      /* No way back */
      p_ptr->create_stair = 0;
    }

  /* Record the non-obvious exit coordinate */
  if (pstair & 0x02)
    p_ptr->path_coord = py;
  else
    p_ptr->path_coord = px;
  
  /* Set the depth */
  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
  
  /* Leaving */
  p_ptr->leaving = TRUE;
}


/**
 * Go to greater danger
 */
void do_cmd_go_down(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  byte pstair = cave_feat[py][px];
  feature_type *f_ptr = &f_info[pstair];
  
  /* Check for appropriate terrain  */
  if (!(f_ptr->flags & TF_STAIR))
    {
      msg_print("I see no path or staircase here.");
      return;
    }
  /* Odd for > */
  else if (!(pstair & 0x01))
    {
      if (pstair > FEAT_TRAP_HEAD) 
    {
      msg_print("This is a path to less danger.");
      return;
    }
      else
	{
	  msg_print("This staircase leads up.");
	  return;
	}
    }
  
  
  /* Handle ironman */
  if (adult_ironman && !p_ptr->depth && !adult_dungeon)
    {
      int i, other;
      int next = stage_map[p_ptr->stage][2 + (pstair - FEAT_MORE_NORTH)/2];
      
      /* Check if this is the right way out of town */
      for (i = NORTH; i <= WEST; i++)
	{
	  other = stage_map[p_ptr->stage][i];
	  if (stage_map[next][DEPTH] < stage_map[other][DEPTH]) 
	    {
	      msg_print("Nothing happens.");
	      return;
	    }
	}
    }

  /* Make certain the player really wants to leave a themed level. -LM- */
  if (p_ptr->themed_level)
    if (!get_check("This level will never appear again.  Really leave?")) 
      return;
  
  
  /* Hack -- take a turn */
  p_ptr->energy_use = 100;
  
  /* Success */

  /* Remember where we came from */
  p_ptr->last_stage = p_ptr->stage;
  
  if (pstair == FEAT_MORE)
    {
      int location;

      /* Magical portal for ironman */
      if (adult_ironman && !stage_map[p_ptr->stage][DOWN] && !adult_dungeon)
	{
	  /* Get choice */
	  if (!jump_menu(p_ptr->depth + 1, &location))
	    return;
	  
	  /* Land properly */
	  p_ptr->last_stage = NOWHERE;

	  /* New stage */
	  p_ptr->stage = location;
	  
	  /* New depth */
	  p_ptr->depth = stage_map[location][DEPTH];
	  
	  /* Leaving */
	  p_ptr->leaving = TRUE;

	  return;
	}

      /* Magical portal for dungeon-only games */
      if (adult_dungeon && (p_ptr->depth) &&
	  ((stage_map[p_ptr->stage][LOCALITY]) !=
	   (stage_map[stage_map[p_ptr->stage][DOWN]][LOCALITY])))
	{
	  /* Land properly */
	  p_ptr->last_stage = NOWHERE;

	  /* Portal */
	  message(MSG_STAIRS_DOWN, 0, "You trigger a magic portal.");

	  /* New stage */
	  p_ptr->stage = stage_map[p_ptr->stage][DOWN];
	  
	  /* New depth */
	  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
	  
	  /* Leaving */
	  p_ptr->leaving = TRUE;

	  return;
	}

      /* stairs */
      message(MSG_STAIRS_DOWN, 0, "You enter a maze of down staircases.");

      /* New stage */
      p_ptr->stage = stage_map[p_ptr->stage][DOWN];
      
      /* make the way back */
      p_ptr->create_stair = FEAT_LESS;
    }
  else if (pstair == FEAT_MORE_SHAFT)
    {
      /* Magical portal for dungeon-only games */
      if (adult_dungeon && 
	  ((stage_map[p_ptr->stage][LOCALITY]) !=
	   (stage_map[stage_map[stage_map[p_ptr->stage][DOWN]][DOWN]][LOCALITY])))
	{
	  /* Land properly */
	  p_ptr->last_stage = NOWHERE;

	  /* Portal */
	  message(MSG_STAIRS_DOWN, 0, "You trigger a magic portal.");

	  /* New stage */
	  p_ptr->stage = stage_map[stage_map[p_ptr->stage][DOWN]][DOWN];
	  
	  /* New depth */
	  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
	  
	  /* Leaving */
	  p_ptr->leaving = TRUE;

	  return;
	}

      /* stairs */
      message(MSG_STAIRS_DOWN, 0, "You enter a maze of down staircases.");

      /* New stage */
      p_ptr->stage = stage_map[stage_map[p_ptr->stage][DOWN]][DOWN];
      
      /* make the way back */
      p_ptr->create_stair = FEAT_LESS_SHAFT;
    }
  else
    {
      /* New stage */
      p_ptr->stage = stage_map[p_ptr->stage][2 + (pstair - FEAT_MORE_NORTH)/2];

      /* Check for Nan Dungortheb */
      if (stage_map[p_ptr->stage][LOCALITY] == NAN_DUNGORTHEB)

	/* scree slope */
	message(MSG_STAIRS_DOWN, 0, "You slide down amidst a small avalanche.");

      else
	{
	  /* Make the way back */
	  p_ptr->create_stair = pstair ^ 0x05;
	  
	  /* path */
	  message(MSG_STAIRS_DOWN, 0, "You enter a winding path to greater danger.");
	}
    }
  
  /* Handle mountaintop stuff */
  if (stage_map[p_ptr->last_stage][LOCALITY] == MOUNTAIN_TOP)
    {  
      /* Reset */
      stage_map[256][DOWN] = 0;
      stage_map[p_ptr->stage][UP] = 0;
      stage_map[256][DEPTH] = 0;

      /* No way back */
      p_ptr->create_stair = 0;
    }

  /* Record the non-obvious exit coordinate */
  if (pstair & 0x02)
    p_ptr->path_coord = py;
  else
    p_ptr->path_coord = px;
  
  /* Set the depth */
  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
  
  /* Check for quests */
  if ((adult_dungeon) && is_quest(p_ptr->stage) && (p_ptr->depth < 100))
    {
      int i;
      monster_race *r_ptr = NULL;

      /* Find the questor */
      for (i = 0; i < z_info->r_max; i++)
	{
	  r_ptr = &r_info[i];
	  if ((r_ptr->flags1 & RF1_QUESTOR) && (r_ptr->level == p_ptr->depth))
	    break;
	}

      /* Give the info */
      msg_format("This level is home to %s.", r_name + r_ptr->name);
    }

  /* Leaving */
  p_ptr->leaving = TRUE;
  
}


/**
 * Simple command to "search" for one turn
 */
void do_cmd_search(void)
{
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Search */
  search();
}


/**
 * Hack -- toggle search mode
 */
void do_cmd_toggle_search(void)
{
  /* Stop searching */
  if (p_ptr->searching)
    {
      /* Clear the searching flag */
      p_ptr->searching = FALSE;
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
    }
  
  /* Start searching */
  else
    {
      /* Set the searching flag */
      p_ptr->searching = TRUE;
      
      /* Update stuff */
      p_ptr->update |= (PU_BONUS);
      
      /* Redraw stuff */
      p_ptr->redraw |= (PR_STATE | PR_SPEED);
    }
}



/**
 * Determine if a grid contains a chest
 */
static s16b chest_check(int y, int x)
{
  s16b this_o_idx, next_o_idx = 0;
  
  
  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Skip unknown chests XXX XXX */
      /* if (!o_ptr->marked) continue; */
      
      /* Check for chest */
      if (o_ptr->tval == TV_CHEST) return (this_o_idx);
    }
  
  /* No chest */
  return (0);
}

/**
 * A function that returns the tval of the items that will be generated
 * when a chest is opened. -LM-
 */
static byte get_choice(void)
{
  byte choice;
  
  choice = randint(100);
  
  switch (p_ptr->pclass)
    {
    case CLASS_WARRIOR:
      {
	if (choice < 2) return (TV_SHOT);
	if (choice < 5) return (TV_ARROW);
	if (choice < 9) return (TV_BOLT);
	if (choice < 13) return (TV_BOW);
	if (choice < 25) return (TV_HAFTED);
	if (choice < 37) return (TV_POLEARM);
	if (choice < 49) return (TV_SWORD);
	if (choice < 54) return (TV_BOOTS);
	if (choice < 59) return (TV_GLOVES);
	if (choice < 64) return (TV_HELM);
	if (choice < 67) return (TV_CROWN);
	if (choice < 72) return (TV_SHIELD);
	if (choice < 76) return (TV_CLOAK);
	if (choice < 79) return (TV_SOFT_ARMOR);
	if (choice < 89) return (TV_HARD_ARMOR);
	if (choice < 95) return (TV_SCROLL);
	if (choice < 101) return (TV_POTION);
	break;
      }
      
    case CLASS_MAGE:
      {
	if (choice < 4) return (TV_BOOTS);
	if (choice < 7) return (TV_HELM);
	if (choice < 11) return (TV_CROWN);
	if (choice < 16) return (TV_SHIELD);
	if (choice < 22) return (TV_CLOAK);
	if (choice < 28) return (TV_SOFT_ARMOR);
	if (choice < 34) return (TV_SCROLL);
	if (choice < 40) return (TV_POTION);
	if (choice < 46) return (TV_RING);
	if (choice < 52) return (TV_AMULET);
	if (choice < 64) return (TV_WAND);
	if (choice < 76) return (TV_STAFF);
	if (choice < 88) return (TV_ROD);
	if (choice < 101) return (TV_MAGIC_BOOK);
	break;
      }
      
    case CLASS_PRIEST:
      {
	if (choice < 4) return (TV_BOOTS);
	if (choice < 7) return (TV_HELM);
	if (choice < 12) return (TV_CROWN);
	if (choice < 16) return (TV_SHIELD);
	if (choice < 21) return (TV_GLOVES);
	if (choice < 27) return (TV_CLOAK);
	if (choice < 33) return (TV_SOFT_ARMOR);
	if (choice < 39) return (TV_SCROLL);
	if (choice < 46) return (TV_POTION);
	if (choice < 53) return (TV_RING);
	if (choice < 60) return (TV_AMULET);
	if (choice < 69) return (TV_HAFTED);
	if (choice < 76) return (TV_WAND);
	if (choice < 81) return (TV_STAFF);
	if (choice < 86) return (TV_ROD);
	if (choice < 101) return (TV_PRAYER_BOOK);
	break;
      }
      
    case CLASS_ROGUE:
      {
	if (choice < 11) return (TV_SHOT);
	if (choice < 17) return (TV_ARROW);
	if (choice < 20) return (TV_BOLT);
	if (choice < 29) return (TV_BOW);
	if (choice < 33) return (TV_HAFTED);
	if (choice < 37) return (TV_POLEARM);
	if (choice < 44) return (TV_SWORD);
	if (choice < 48) return (TV_BOOTS);
	if (choice < 50) return (TV_GLOVES);
	if (choice < 54) return (TV_HELM);
	if (choice < 58) return (TV_CROWN);
	if (choice < 62) return (TV_SHIELD);
	if (choice < 68) return (TV_CLOAK);
	if (choice < 71) return (TV_SOFT_ARMOR);
	if (choice < 74) return (TV_HARD_ARMOR);
	if (choice < 80) return (TV_SCROLL);
	if (choice < 86) return (TV_POTION);
	if (choice < 92) return (TV_STAFF);
	if (choice < 101) return (TV_MAGIC_BOOK);
	break;
      }
      
    case CLASS_RANGER:
      {
	if (choice < 15) return (TV_ARROW);
	if (choice < 21) return (TV_BOLT);
	if (choice < 31) return (TV_BOW);
	if (choice < 34) return (TV_HAFTED);
	if (choice < 37) return (TV_POLEARM);
	if (choice < 40) return (TV_SWORD);
	if (choice < 45) return (TV_RING);
	if (choice < 50) return (TV_AMULET);
	if (choice < 54) return (TV_BOOTS);
	if (choice < 58) return (TV_GLOVES);
	if (choice < 62) return (TV_HELM);
	if (choice < 66) return (TV_CROWN);
	if (choice < 70) return (TV_SHIELD);
	if (choice < 74) return (TV_CLOAK);
	if (choice < 78) return (TV_SOFT_ARMOR);
	if (choice < 82) return (TV_ROD);
	if (choice < 87) return (TV_WAND);
	if (choice < 92) return (TV_STAFF);
	if (choice < 101) return (TV_DRUID_BOOK);
	break;
      }
      
    case CLASS_PALADIN:
      {
	if (choice < 4) return (TV_BOOTS);
	if (choice < 8) return (TV_HELM);
	if (choice < 12) return (TV_CROWN);
	if (choice < 19) return (TV_SHIELD);
	if (choice < 23) return (TV_GLOVES);
	if (choice < 28) return (TV_CLOAK);
	if (choice < 35) return (TV_HARD_ARMOR);
	if (choice < 40) return (TV_SCROLL);
	if (choice < 45) return (TV_POTION);
	if (choice < 52) return (TV_RING);
	if (choice < 59) return (TV_AMULET);
	if (choice < 77) return (TV_HAFTED);
	if (choice < 82) return (TV_ROD);
	if (choice < 87) return (TV_WAND);
	if (choice < 92) return (TV_STAFF);
	if (choice < 101) return (TV_PRAYER_BOOK);
	break;
      }
      
      
    case CLASS_DRUID:
      {
	if (choice < 3) return (TV_BOOTS);
	if (choice < 5) return (TV_HELM);
	if (choice < 11) return (TV_CROWN);
	if (choice < 14) return (TV_SHIELD);
	if (choice < 24) return (TV_CLOAK);
	if (choice < 27) return (TV_SOFT_ARMOR);
	if (choice < 37) return (TV_SCROLL);
	if (choice < 48) return (TV_POTION);
	if (choice < 58) return (TV_RING);
	if (choice < 68) return (TV_AMULET);
	if (choice < 74) return (TV_WAND);
	if (choice < 80) return (TV_STAFF);
	if (choice < 86) return (TV_ROD);
	if (choice < 101) return (TV_DRUID_BOOK);
	break;
      }
      
      
    case CLASS_NECRO:
      {
	if (choice < 3) return (TV_BOOTS);
	if (choice < 5) return (TV_HELM);
	if (choice < 11) return (TV_CROWN);
	if (choice < 14) return (TV_SHIELD);
	if (choice < 24) return (TV_CLOAK);
	if (choice < 27) return (TV_SOFT_ARMOR);
	if (choice < 37) return (TV_SCROLL);
	if (choice < 48) return (TV_POTION);
	if (choice < 58) return (TV_RING);
	if (choice < 68) return (TV_AMULET);
	if (choice < 74) return (TV_WAND);
	if (choice < 80) return (TV_STAFF);
	if (choice < 86) return (TV_ROD);
	if (choice < 101) return (TV_NECRO_BOOK);
	break;
      }
      
    case CLASS_ASSASSIN:
      {
	if (choice < 11) return (TV_SHOT);
	if (choice < 17) return (TV_ARROW);
	if (choice < 20) return (TV_BOLT);
	if (choice < 29) return (TV_BOW);
	if (choice < 33) return (TV_HAFTED);
	if (choice < 37) return (TV_POLEARM);
	if (choice < 44) return (TV_SWORD);
	if (choice < 48) return (TV_BOOTS);
	if (choice < 50) return (TV_GLOVES);
	if (choice < 54) return (TV_HELM);
	if (choice < 58) return (TV_CROWN);
	if (choice < 62) return (TV_SHIELD);
	if (choice < 68) return (TV_CLOAK);
	if (choice < 71) return (TV_SOFT_ARMOR);
	if (choice < 74) return (TV_HARD_ARMOR);
	if (choice < 80) return (TV_SCROLL);
	if (choice < 86) return (TV_POTION);
	if (choice < 92) return (TV_STAFF);
	if (choice < 101) return (TV_NECRO_BOOK);
	break;
      }
    }
  /* If the function fails, do not specify a tval */
  return (0);
}

/**
 * Allocate objects upon opening a chest.
 *
 * Disperse treasures from the given chest, centered at (x,y).
 *
 * In Oangband, chests are nice finds.  Small chests distribute 3-5 items,
 * while large chests can distribute 5-7.  Item types are biased to be
 * useful for the character, and they can frequently be of good quality 
 * (or better).   Code in object2.c helps these items be even better. -LM-
 *
 * The "value" of the items in a chest is based on the "power" of the chest,
 * which is in turn based on the level on which the chest is generated.
 */
static void chest_death(bool scatter, int y, int x, s16b o_idx)
{
  int number, i;
  bool obj_success=FALSE;
  object_type *o_ptr;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  /* Access chest */
  o_ptr = &o_list[o_idx];
  
  /* Determine how much to drop. */
  if (o_ptr->sval >= SV_CHEST_MIN_LARGE) number = 4 + randint(3);
  else number = 2 + randint(3);
  
  /* Zero pval means empty chest */
  if (!o_ptr->pval) number = 0;
  
  /* Opening a chest */
  opening_chest = TRUE;
  
  /* Determine the "value" of the items */
  object_level = ABS(o_ptr->pval);
  
  /* Select an item type that the chest will disperse. */
  required_tval = get_choice();
  
  /* Drop some objects (non-chests) */
  for (; number > 0; --number)
    {
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Make an object with a specified tval.  Grant a possibility for 
       * items to be forced good, or even great.  With the new definitions 
       * of goodness, this can make for quite interesting loot.  -LM-
       */
      switch (required_tval)
	{
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_BOW:
	case TV_SWORD:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_DIGGING:
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
	  {
	    if (randint(200) < object_level)
	      {
		obj_success=make_object(i_ptr, TRUE, TRUE, TRUE);
		break;
	      }
	    
	    else if (randint(40) < object_level)
	      {
		obj_success=make_object(i_ptr, TRUE, FALSE, TRUE);
		break;
	      }
	    else
	      {
		obj_success=make_object(i_ptr, FALSE, FALSE, TRUE);
		break;
	      }
	  }
	  
	case TV_MAGIC_BOOK:
	case TV_PRAYER_BOOK:
	case TV_DRUID_BOOK:
	case TV_NECRO_BOOK:
	  {
	    if (randint(80) < object_level)
	      {
		obj_success=make_object(i_ptr, TRUE, FALSE, TRUE);
	      }
	    else
	      {
		obj_success=make_object(i_ptr, FALSE, FALSE, TRUE);
	      }
	    
	    break;
	  }
	  
	case TV_SCROLL:
	case TV_POTION:
	case TV_RING:
	case TV_AMULET:
	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
	  {
	    if (randint(100) < (object_level - 10) / 2)
	      {
		obj_success=make_object(i_ptr, TRUE, FALSE, TRUE);
	      }
	    
	    else
	      {
		obj_success=make_object(i_ptr, FALSE, FALSE, TRUE);
	      }
	    
	    break;
	  }
	  
	default:
	  {
	    obj_success=make_object(i_ptr, FALSE, FALSE, TRUE);
	    break;
	  }
	}
      
      /* If no object was made, we need to try another tval. */
      if (!obj_success)
	{
	  required_tval = get_choice();
	}
      
      /* If chest scatters its contents, pick any floor square. */
      if (scatter)
	{
	  for (i = 0; i < 200; i++)
	    {
	      /* Pick a totally random spot. */
	      y = rand_int(DUNGEON_HGT);
	      x = rand_int(DUNGEON_WID);
	      
	      /* Must be an empty floor. */
	      if (!cave_empty_bold(y, x)) continue;
	      
	      /* Place the object there. */
	      if (obj_success)
		drop_near(i_ptr, -1, y, x);
	      
	      /* Done. */
	      break;
	    }
	}
      /* Normally, drop object near the chest. */
      else 
	if (obj_success)
	  drop_near(i_ptr, -1, y, x);
    }
  
  /* Clear this global variable, to avoid messing up object generation. */
  required_tval = 0;
  
  /* Reset the object level */
  object_level = p_ptr->depth;
  
  /* No longer opening a chest */
  opening_chest = FALSE;
  
  /* Empty */
  o_ptr->pval = 0;
  
  /* Known */
  object_known(o_ptr);
}


/**
 * Chests have traps too.  High-level chests can be very dangerous, no 
 * matter what level they are opened at.  Various traps added in Oangband. -LM-
 *
 * Exploding chest destroys contents (and traps).
 * Note that the chest itself is never destroyed.
 */
static void chest_trap(int y, int x, s16b o_idx)
{
  int i, trap, nasty_tricks_count;
  int j;
  
  object_type *o_ptr = &o_list[o_idx];
  
  /* Compensate for the averaging routine in the summon monster function. */
  int summon_level = o_ptr->pval + o_ptr->pval - p_ptr->depth;
  
  /* Ignore disarmed chests */
  if (o_ptr->pval <= 0) return;
  
  /* Obtain the traps */
  trap = chest_traps[o_ptr->pval];
  
  /* Lose strength */
  if (trap & (CHEST_LOSE_STR))
    {
      msg_print("A small needle has pricked you!");
      take_hit(damroll(1, 4), "a poison needle");
      (void)do_dec_stat(A_STR);
    }
  
  /* Lose constitution */
  if (trap & (CHEST_LOSE_CON))
    {
      msg_print("A small needle has pricked you!");
      take_hit(damroll(1, 4), "a poison needle");
      (void)do_dec_stat(A_CON);
    }
  
  /* Poison */
  if (trap & (CHEST_POISON))
    {
      msg_print("A puff of green gas surrounds you!");
      pois_hit(25);
    }
  
  /* Paralyze */
  if (trap & (CHEST_PARALYZE))
    {
      msg_print("A puff of yellow gas surrounds you!");
      if (!p_ptr->free_act)
	{
	  (void)set_paralyzed(p_ptr->paralyzed + 10 + randint(20));
	}
      else notice_obj(OF_FREE_ACT, 0);
    }
  
  /* Summon monsters */
  if (trap & (CHEST_SUMMON))
    {
      int num = 2 + randint(3);
      msg_print("You are enveloped in a cloud of smoke!");
      sound(MSG_SUM_MONSTER);
      for (i = 0; i < num; i++)
	{
	  (void)summon_specific(y, x, FALSE, summon_level, 0);
	}
    }
  
  /* Explode */
  if (trap & (CHEST_EXPLODE))
    {
      msg_print("There is a sudden explosion!");
      msg_print("Everything inside the chest is destroyed!");
      o_ptr->pval = 0;
      take_hit(damroll(5, 8), "an exploding chest");
    }
  
  /* Scatter contents. */
  if (trap & (CHEST_SCATTER))
    {
      if (stage_map[p_ptr->stage][STAGE_TYPE] >= CAVE)
	msg_print("The contents of the chest scatter all over the dungeon!");
      else
	msg_print("The contents of the chest scatter to the four winds!");		  
      chest_death(TRUE, y, x, o_idx);
      o_ptr->pval = 0;
    }
  
  /* Elemental summon. */
  if (trap & (CHEST_E_SUMMON))
    {
      j = randint(3) + 5;
      msg_print("Elemental beings appear to protect their treasures!");
      for (i = 0; i < j; i++)
	{
	  summon_specific(y, x, FALSE, summon_level, SUMMON_ELEMENTAL);
	}
    }
  
  /* Force clouds, then summon birds. */
  if (trap & (CHEST_BIRD_STORM))
    {
      msg_print("A storm of birds swirls around you!");
      
      j = randint(3) + 3;
      for (i = 0; i < j; i++)
	(void)fire_meteor(0, GF_FORCE, y, x, o_ptr->pval / 5, 7, TRUE);
      
      j = randint(5) + o_ptr->pval /5;
      for (i = 0; i < j; i++)
	{
	  summon_specific(y, x, TRUE, summon_level, SUMMON_BIRD);
	}
    }
  
  /* Various colorful summonings. */
  if (trap & (CHEST_H_SUMMON))
    {
      /* Summon demons. */
      if (rand_int(4) == 0)
	{
	  msg_print("Demons materialize in clouds of fire and brimstone!");
	  
	  j = randint(3) + 2;
	  for (i = 0; i < j; i++)
	    {
	      (void)fire_meteor(0, GF_FIRE, y, x, 10, 5, TRUE);
	      summon_specific(y, x, FALSE, summon_level, SUMMON_DEMON);
	    }
	}
      
      /* Summon dragons. */
      else if (rand_int(3) == 0)
	{
	  msg_print("Draconic forms loom out of the darkness!");
	  
	  j = randint(3) + 2;
	  for (i = 0; i < j; i++)
	    {
	      summon_specific(y, x, FALSE, summon_level, SUMMON_DRAGON);
	    }
	}
      
      /* Summon hybrids. */
      else if (rand_int(2) == 0)
	{
	  msg_print("Creatures strange and twisted assault you!");
	  
	  j = randint(5) + 3;
	  for (i = 0; i < j; i++)
	    {
	      summon_specific(y, x, FALSE, summon_level, SUMMON_HYBRID);
	    }
	}
      
      /* Summon vortices (scattered) */
      else
	{
	  msg_print("Vortices coalesce and wreak destruction!");
	  
	  j = randint(3) + 2;
	  for (i = 0; i < j; i++)
	    {
	      summon_specific(y, x, TRUE, summon_level, SUMMON_VORTEX);
	    }
	}
    }
  
  /* Dispel player. */
  if (trap & (CHEST_RUNES_OF_EVIL))
    {
      /* Message. */
      msg_print("Hideous voices bid: 'Let the darkness have thee!'");
      
      /* Determine how many nasty tricks can be played. */
      nasty_tricks_count = 4 + rand_int(3);
      
      /* This is gonna hurt... */
      for (; nasty_tricks_count > 0; nasty_tricks_count--)
	{
	  /* ...but a high saving throw does help a little. */
	  if (!check_save(2 * o_ptr->pval))
	    {
	      if (rand_int(6) == 0) 
		take_hit(damroll(5, 20), "a chest dispel-player trap");
	      else if (rand_int(5) == 0) (void)set_cut(p_ptr->cut + 200);
	      else if (rand_int(4) == 0)
		{
		  if (!p_ptr->free_act) 
		    (void)set_paralyzed(p_ptr->paralyzed + 2 + 
					rand_int(6));
		  else 
		    {
		      (void)set_stun(p_ptr->stun + 10 + rand_int(100));
		      notice_obj(OF_FREE_ACT, 0);
		    }

		}
	      else if (rand_int(3) == 0) apply_disenchant(0);
	      else if (rand_int(2) == 0)
		{
		  (void)do_dec_stat(A_STR);
		  (void)do_dec_stat(A_DEX);
		  (void)do_dec_stat(A_CON);
		  (void)do_dec_stat(A_INT);
		  (void)do_dec_stat(A_WIS);
		  (void)do_dec_stat(A_CHR);
		}
	      else (void)fire_meteor(0, GF_NETHER, y, x, 150, 1, TRUE);
	    }
	}
    }
}


/**
 * Attempt to open the given chest at the given location
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_open_chest(int y, int x, s16b o_idx)
{
  int i, j;
  
  bool flag = TRUE;
  
  bool more = FALSE;
  
  object_type *o_ptr = &o_list[o_idx];
  
  
  /* Attempt to unlock it */
  if (o_ptr->pval > 0)
    {
      /* Assume locked, and thus not open */
      flag = FALSE;
      
      /* Get the "disarm" factor */
      i = p_ptr->skill_dis;
      
      /* Penalize some conditions */
      if (p_ptr->blind || no_lite()) i = i / 10;
      if (p_ptr->confused || p_ptr->image) i = i / 10;
      
      /* Difficulty rating.  Tweaked to compensate for higher pvals. */
      j = i - 2 * o_ptr->pval / 3;
      
      /* Always have a small chance of success */
      if (j < 2) j = 2;
      
      /* Success -- May still have traps */
      if (rand_int(100) < j)
	{
	  msg_print("You have picked the lock.");
	  gain_exp(o_ptr->pval);
	  flag = TRUE;
	}
      
      /* Failure -- Keep trying */
      else
	{
	  /* We may continue repeating */
	  more = TRUE;
	  if (flush_failure) flush();
	  message(MSG_LOCKPICK_FAIL, 0, "You failed to pick the lock.");
	}
    }
  
  /* Allowed to open */
  if (flag)
    {
      /* Apply chest traps, if any */
      chest_trap(y, x, o_idx);
      
      /* Let the Chest drop items */
      chest_death(FALSE, y, x, o_idx);

      /* Squelch chest if autosquelch calls for it */
      p_ptr->notice |= PN_SQUELCH;
    }
  
  /* Result */
  return (more);
}


/**
 * Attempt to disarm the chest at the given location
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_disarm_chest(int y, int x, s16b o_idx)
{
  int i, j;
  
  bool more = FALSE;
  
  object_type *o_ptr = &o_list[o_idx];
  
  
  /* Get the "disarm" factor */
  i = p_ptr->skill_dis;
  
  /* Penalize some conditions */
  if (p_ptr->blind || no_lite()) i = i / 10;
  if (p_ptr->confused || p_ptr->image) i = i / 10;
  
  /* Difficulty rating. */
  j = i - (5 + o_ptr->pval / 2);
  
  /* Always have a small chance of success */
  if (j < 2) j = 2;
  
  /* Must find the trap first. */
  if (!object_known_p(o_ptr))
    {
      msg_print("I don't see any traps.");
    }
  
  /* Already disarmed/unlocked */
  else if (o_ptr->pval <= 0)
    {
      msg_print("The chest is not trapped.");
    }
  
  /* No traps to find. */
  else if (!chest_traps[o_ptr->pval])
    {
      msg_print("The chest is not trapped.");
    }
  
  /* Success (get a fair amount of experience) */
  else if (rand_int(100) < j)
    {
      msg_print("You have disarmed the chest.");
      gain_exp(o_ptr->pval * o_ptr->pval / 10);
      o_ptr->pval = (0 - o_ptr->pval);
    }
  
  /* Failure -- Keep trying */
  else if ((i > 5) && (randint(i) > 5))
    {
      /* We may keep trying */
      more = TRUE;
      if (flush_failure) flush();
      msg_print("You failed to disarm the chest.");
    }
  
  /* Failure -- Set off the trap */
  else
    {
      msg_print("You set off a trap!");
      chest_trap(y, x, o_idx);
    }
  
  /* Result */
  return (more);
}


/**
 * Return TRUE if the given feature is an open door
 */
static bool is_open(int feat)
{
  return (feat == FEAT_OPEN);
}


/**
 * Return TRUE if the given feature is a closed door
 */
static bool is_closed(int feat)
{
  return ((feat >= FEAT_DOOR_HEAD) &&
	        (feat <= FEAT_DOOR_TAIL));
}


/**
 * Return TRUE if the given feature is a trap
 */
static bool is_trap(int feat)
{
  return ((feat >= FEAT_TRAP_HEAD) && (feat <= FEAT_TRAP_TAIL));
}


/**
 * Return the number of doors/traps around (or under) the character
 */
static int count_feats(int *y, int *x, bool (*test)(int feat), bool under)
{
  int d;
  int xx, yy;
  int count;
  
  /* Count how many matches */
  count = 0;
  
  /* Check around (and under) the character */
  for (d = 0; d < 9; d++)
    {
      /* Not searching under the character */
      if ((d == 8) && !under) continue;
      
      /* Extract adjacent (legal) location */
      yy = p_ptr->py + ddy_ddd[d];
      xx = p_ptr->px + ddx_ddd[d];
      
      /* Paranoia */
      if (!in_bounds_fully(yy, xx)) continue;
      
      /* Must have knowledge */
      if (!(cave_info[yy][xx] & (CAVE_MARK))) continue;
      
      /* Not looking for this feature */
      if (!(*test)(cave_feat[yy][xx])) continue;
      
      /* Count it */
      ++count;
      
      /* Remember the location of the last door found */
      *y = yy;
      *x = xx;
    }
  
  /* All done */
  return count;
}

/**
 * Return the number of chests around (or under) the character. -TNB-
 * If requested, count only trapped chests.
 */
extern int count_chests(int *y, int *x, bool trapped)
{
  int d, count, o_idx;
  
  object_type *o_ptr;
  
  /* Count how many matches */
  count = 0;
  
  /* Check around (and under) the character */
  for (d = 0; d < 9; d++) 
    {
      /* Extract adjacent (legal) location */
      int yy = p_ptr->py + ddy_ddd[d];
      int xx = p_ptr->px + ddx_ddd[d];
      
      /* No (visible) chest is there */
      if ((o_idx = chest_check(yy, xx)) == 0) continue;
      
      /* Grab the object */
      o_ptr = &o_list[o_idx];
      
      /* Already open */
      if (o_ptr->pval == 0) continue;
      
      /* No (known) traps here */
      if (trapped &&
	  (!object_known_p(o_ptr) ||
	   (o_ptr->pval < 0) ||
	   !chest_traps[o_ptr->pval])) continue;
      
      /* OK */
      ++count;
      
      /* Remember the location of the last chest found */
      *y = yy;
      *x = xx;
    }
  
  /* All done */
  return count;
}

/**
 * Convert an adjacent location to a direction.
 */
static int coords_to_dir(int y, int x)
{
  int d[3][3] = 
    { 
      { 7, 4, 1 },
      { 8, 5, 2 },
      { 9, 6, 3 }
    };
  int dy, dx;
  
  dy = y - p_ptr->py;
  dx = x - p_ptr->px;
  
  /* Paranoia */
  if (ABS(dx) > 1 || ABS(dy) > 1) return (0);
  
  return d[dx + 1][dy + 1];
}


/**
 * Determine if a given grid may be "opened"
 */
static bool do_cmd_open_test(int y, int x)
{
  /* Must have knowledge */
  if (!(cave_info[y][x] & (CAVE_MARK)))
    {
      /* Message */
      msg_print("You see nothing there.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Must be a closed door */
  if (!((cave_feat[y][x] >= FEAT_DOOR_HEAD) &&
	(cave_feat[y][x] <= FEAT_DOOR_TAIL)))
    {
      /* Message */
      message(MSG_NOTHING_TO_OPEN, 0, "You see nothing there to open.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}


/**
 * Perform the basic "open" command on doors
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_open_aux(int y, int x)
{
  int i, j;
  
  bool more = FALSE;
  
  
  /* Verify legality */
  if (!do_cmd_open_test(y, x)) return (FALSE);
  
  
  /* Jammed door */
  if (cave_feat[y][x] >= FEAT_DOOR_HEAD + 0x08)
    {
      /* Stuck */
      msg_print("The door appears to be stuck.");
    }
  
  /* Locked door */
  else if (cave_feat[y][x] >= FEAT_DOOR_HEAD + 0x01)
    {
      /* Disarm factor */
      i = p_ptr->skill_dis;
      
      /* Penalize some conditions */
      if (p_ptr->blind || no_lite()) i = i / 10;
      if (p_ptr->confused || p_ptr->image) i = i / 10;
      
      /* Extract the lock power */
      j = cave_feat[y][x] - FEAT_DOOR_HEAD;
      
      /* Extract the difficulty XXX XXX XXX */
      j = i - (j * 4);
      
      /* Always have a small chance of success */
      if (j < 2) j = 2;
      
      /* Success */
      if (rand_int(100) < j)
	{
	  /* Message */
	  message(MSG_OPENDOOR, 0, "You have picked the lock.");
	  
	  /* Open the door */
	  cave_set_feat(y, x, FEAT_OPEN);
	  
	  /* Update the visuals */
	  p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	  
	  /* Sound */
	  sound(MSG_OPENDOOR);
	  
	  /* Experience */
	  gain_exp(1);
	}
      
      /* Failure */
      else
	{
	  /* Failure */
	  if (flush_failure) flush();
	  
	  /* Message */
	  message(MSG_LOCKPICK_FAIL, 0, "You failed to pick the lock.");
	  
	  /* We may keep trying */
	  more = TRUE;
	}
    }
  
  /* Closed door */
  else
    {
      /* Open the door */
      cave_set_feat(y, x, FEAT_OPEN);
      
      /* Update the visuals */
      p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
      
      /* Sound */
      sound(MSG_OPENDOOR);
    }
  
  /* Result */
  return (more);
}



/**
 * Open a closed/locked/jammed door or a closed/locked chest.
 *
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  s16b o_idx;
  
  bool more = FALSE;

  /* Option: Pick a direction -TNB- */
  if (easy_open) 
    {
      int num_doors, num_chests;
      
      /* Count closed doors */
      num_doors = count_feats(&y, &x, is_closed, FALSE);
      
      /* Count chests (locked) */
      num_chests = count_chests(&y, &x, FALSE);
      
      /* See if there's only one target */
      if ((num_doors + num_chests) == 1)
	{
	  p_ptr->command_dir = coords_to_dir(y, x);
	}
    }
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  /* Check for chests */
  o_idx = chest_check(y, x);
  
  /* Verify legality */
  if (!o_idx && !do_cmd_open_test(y, x)) return;
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Apply confusion */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
      
      /* Check for chest */
      o_idx = chest_check(y, x);
    }
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Monster */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");
      
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* Chest */
  if (o_idx)
    {
      /* Open the chest */
      more = do_cmd_open_chest(y, x, o_idx);
    }
  
  /* Door */
  else
    {
      /* Open the door */
      more = do_cmd_open_aux(y, x);
    }
  
  /* Cancel repeat unless we may continue */
  if (!more) disturb(0, 0);
}


/**
 * Determine if a given grid may be "closed"
 */
static bool do_cmd_close_test(int y, int x)
{
  /* Must have knowledge */
  if (!(cave_info[y][x] & (CAVE_MARK)))
    {
      /* Message */
      msg_print("You see nothing there.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Require open/broken door */
  if ((cave_feat[y][x] != FEAT_OPEN) &&
      (cave_feat[y][x] != FEAT_BROKEN))
    {
      /* Message */
      msg_print("You see nothing there to close.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Monster */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");

      /* Nope */
      return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}


/**
 * Perform the basic "close" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_close_aux(int y, int x)
{
  bool more = FALSE;
  
  
  /* Verify legality */
  if (!do_cmd_close_test(y, x)) return (FALSE);
  
  
  /* Broken door */
  if (cave_feat[y][x] == FEAT_BROKEN)
    {
      /* Message */
      msg_print("The door appears to be broken.");
    }
  
  /* Open door */
  else
    {
      /* Close the door */
      cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
      
      /* Update the visuals */
      p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
      
      /* Sound */
      sound(MSG_SHUTDOOR);
    }
  
  /* Result */
  return (more);
}


/**
 * Close an open door.
 */
void do_cmd_close(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  bool more = FALSE;
  
  /* Option: Pick a direction -TNB- */
  if (easy_open)
    {
      /* See if there's only one closeable door */
      if (count_feats(&y, &x, is_open, FALSE) == 1)
	{
	  p_ptr->command_dir = coords_to_dir(y, x);
	}
    }
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Verify legality */
  if (!do_cmd_close_test(y, x)) return;
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Apply confusion */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
    }
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Close door */
  more = do_cmd_close_aux(y, x);
    
  /* Cancel repeat unless told not to */
  if (!more) disturb(0, 0);
}



/**
 * Determine if a given grid may be "tunneled"
 */
static bool do_cmd_tunnel_test(int y, int x)
{
  feature_type *f_ptr = &f_info[cave_feat[y][x]];

  /* Must have knowledge */
  if (!(cave_info[y][x] & (CAVE_MARK)))
    {
      /* Message */
      msg_print("You see nothing there.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Must be a wall/door/etc */
  if ((f_ptr->flags & TF_ROCK) || (f_ptr->flags & TF_DOOR_CLOSED))
    {
      /* Okay */
      return (TRUE);
    }

  /* Message */
  msg_print("You see nothing there to tunnel.");
  
  /* Nope */
  return (FALSE);
}


/**
 * Tunnel through wall.   Assumes valid location.
 *
 * Note that it is impossible to "extend" rooms past their
 * outer walls (which are actually part of the room).
 *
 * Attempting to do so will produce floor grids which are not part
 * of the room, and whose "illumination" status do not change with
 * the rest of the room.
 */
static bool twall(int y, int x)
{
  /* Paranoia -- Require a wall or door or some such */
  if (cave_floor_bold(y, x) || (cave_feat[y][x] == FEAT_TREE) || 
      (cave_feat[y][x] == FEAT_TREE2)) return (FALSE);
  
  /* Sound */
  sound(MSG_DIG);
  
  /* Forget the wall */
  cave_info[y][x] &= ~(CAVE_MARK | CAVE_WALL);
  
  /* Remove the feature */
  cave_set_feat(y, x, FEAT_FLOOR);
  
  /* Update the visuals */
  p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
  
  /* Result */
  return (TRUE);
}


/**
 * Perform the basic "tunnel" command
 *
 * Assumes that no monster is blocking the destination
 *
 * Uses "twall" (above) to do all "terrain feature changing".
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_tunnel_aux(int y, int x)
{
  bool more = FALSE;
  feature_type *f_ptr = &f_info[cave_feat[y][x]];
  
  /* Verify legality */
  if (!do_cmd_tunnel_test(y, x)) return (FALSE);
  
  
  /* Sound XXX XXX XXX */
  /* sound(MSG_DIG); */
  
  /* All rock and secret doors */
  if (f_ptr->flags & TF_ROCK)
    {
      /* Titanium */
      if (f_ptr->flags & TF_PERMANENT)
	{
	  msg_print("This seems to be permanent rock.");
	}
      
      /* Granite */
      else if ((f_ptr->flags & TF_GRANITE) && !(f_ptr->flags & TF_DOOR_ANY))
	{
	  /* Tunnel */
	  if ((p_ptr->skill_dig > 40 + rand_int(1600)) && twall(y, x))
	    {
	      msg_print("You have finished the tunnel.");
	    }
	  
	  /* Keep trying */
	  else
	    {
	      /* We may continue tunelling */
	      msg_print("You tunnel into the granite wall.");
	      more = TRUE;
	    }
	}
      
      /* Quartz / Magma */
      else if (!(f_ptr->flags & TF_GRANITE) && (f_ptr->flags & TF_WALL))
	{
	  bool okay = FALSE;
	  bool gold = FALSE;
	  bool hard = FALSE;
	  
	  /* Found gold */
	  if (cave_feat[y][x] >= FEAT_MAGMA_H)
	    {
	      gold = TRUE;
	    }
	  
	  /* Extract "quartz" flag XXX XXX XXX */
	  if ((cave_feat[y][x] - FEAT_MAGMA) & 0x01)
	    {
	      hard = TRUE;
	    }
	  
	  /* Quartz */
	  if (hard)
	    {
	      okay = (p_ptr->skill_dig > 20 + rand_int(800));
	    }
	  
	  /* Magma */
	  else
	    {
	      okay = (p_ptr->skill_dig > 10 + rand_int(400));
	    }
	  
	  /* Success */
	  if (okay && twall(y, x))
	    {
	      /* Found treasure */
	      if (gold)
		{
		  /* Place some gold */
		  place_gold(y, x);
		  
		  /* Message */
		  msg_print("You have found something!");
		}
	      
	      /* Found nothing */
	      else
		{
		  /* Message */
		  msg_print("You have finished the tunnel.");
		}
	    }
	  
	  /* Failure (quartz) */
	  else if (hard)
	    {
	      /* Message, continue digging */
	      msg_print("You tunnel into the quartz vein.");
	      more = TRUE;
	    }
	  
	  /* Failure (magma) */
	  else
	    {
	      /* Message, continue digging */
	      msg_print("You tunnel into the magma vein.");
	      more = TRUE;
	    }
	}
      
      /* Rubble */
      else if (cave_feat[y][x] == FEAT_RUBBLE)
	{
	  /* Remove the rubble */
	  if ((p_ptr->skill_dig > rand_int(200)) && twall(y, x))
	    {
	      /* Message */
	      msg_print("You have removed the rubble.");
	      
	      /* Hack -- place an object */
	      if (rand_int(100) == 0)
		{
		  /* Create a simple object */
		  place_object(y, x, FALSE, FALSE, FALSE);
		  
		  /* Observe new object */
		  if (!squelch_hide_item(&o_list[cave_o_idx[y][x]]) &&
		      player_can_see_bold(y, x))
		    {
		      msg_print("You have found something!");
		    }
		}
	    }
	  
	  else
	    {
	      /* Message, keep digging */
	      msg_print("You dig in the rubble.");
	      more = TRUE;
	    }
	}
      
      /* Secret doors */
      else if (cave_feat[y][x] == FEAT_SECRET)
	{
	  /* Tunnel */
	  if ((p_ptr->skill_dig > 30 + rand_int(1200)) && twall(y, x))
	    {
	      msg_print("You have finished the tunnel.");
	    }
	  
	  /* Keep trying */
	  else
	    {
	      /* We may continue tunelling */
	      msg_print("You tunnel into the granite wall.");
	  more = TRUE;
	  
	  /* Occasional Search XXX XXX */
	  if (rand_int(100) < 25) search();
	    }
	}
    }
  /* Doors */
  else
    {
      /* Tunnel */
      if ((p_ptr->skill_dig > 30 + rand_int(1200)) && twall(y, x))
	{
	  msg_print("You have finished the tunnel.");
	}
      
      /* Keep trying */
      else
	{
	  /* We may continue tunelling */
	  msg_print("You tunnel into the door.");
	  more = TRUE;
	}
    }
  
  /* Result */
  return (more);
}


/**
 * Tunnel through "walls" (including rubble and secret doors)
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 */
void do_cmd_tunnel(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  bool more = FALSE;
  
  /* Deal with webs first */
  if (cave_feat[py][px] == FEAT_WEB)
    {
      msg_print("You clear the web.");
      cave_set_feat(py, px, FEAT_FLOOR);
  
      /* Take a turn */
      p_ptr->energy_use = 100;

      /* Done */
      return;
    }
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  /* Deal with webs first */
  if (cave_feat[y][x] == FEAT_WEB)
    {
      /* Take a turn */
      p_ptr->energy_use = 100;
  
      /* Apply confusion */
      if (confuse_dir(&dir))
	{
	  /* Get location */
	  y = py + ddy[dir];
	  x = px + ddx[dir];
	}

      /* Monster */
      if (cave_m_idx[y][x] > 0)
	{
	  /* Message */
	  msg_print("There is a monster in the way!");
	  
	  /* Attack */
	  if (py_attack(y, x, TRUE)) return;
	}
  
      if (cave_feat[y][x] == FEAT_WEB)
	{
	  msg_print("You clear the web.");
	  cave_set_feat(y, x, FEAT_FLOOR);
	}
      else
	msg_print("You claw vainly at the web.");

      /* Done */
      return;
    }
  
  /* Oops */
  if (!do_cmd_tunnel_test(y, x)) return;
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Apply confusion */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
    }
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Monster */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");
      
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* Tunnel through walls */
  more = do_cmd_tunnel_aux(y, x);
    
  /* Cancel repetition unless we can continue */
  if (!more) disturb(0, 0);
}


/**
 * Determine if a given grid may be "disarmed"
 */
static bool do_cmd_disarm_test(int y, int x)
{
  /* Must have knowledge */
  if (!(cave_info[y][x] & (CAVE_MARK)))
    {
      /* Message */
      msg_print("You see nothing there.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Require an actual trap or glyph */
  if (!((cave_feat[y][x] >= FEAT_TRAP_HEAD) && 
	(cave_feat[y][x] <= FEAT_TRAP_TAIL)) && 
      !((cave_feat[y][x] >= FEAT_RUNE_HEAD) &&
	(cave_feat[y][x] <= FEAT_RUNE_TAIL)) &&
      !((cave_feat[y][x] >= FEAT_MTRAP_HEAD) &&
	(cave_feat[y][x] <= FEAT_MTRAP_TAIL)))
    {
      /* Message */
      msg_print("You see nothing there to disarm.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}


/**
 * Perform the basic "disarm" command on a trap or glyph.
 *
 * Assume there is no monster blocking the destination (tested by 
 * do_cmd_disarm).  Traps now have level-dependent power.
 * Decrement Rogue traps and glyphs of warding. -LM-
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_disarm_aux(int y, int x)
{
  int i, j, power;
  
  cptr name;
  
  bool more = FALSE;
  
  feature_type *f_ptr = &f_info[cave_feat[y][x]];

  int tree_hack = (cave_feat[y][x] == FEAT_TRAP_HEAD + 0x0B ? 1 : 0);
  
  /* Verify legality */
  if (!do_cmd_disarm_test(y, x)) return (FALSE);
  
  
  /* Access trap or glyph name */
  name = (f_name + f_ptr->name);
  
  /* Get the "disarm" factor */
  i = p_ptr->skill_dis;
  
  /* Penalize some conditions */
  if (p_ptr->blind || no_lite()) i = i / 10;
  if (p_ptr->confused || p_ptr->image) i = i / 10;
  
  
  /* Extract trap "power". */
  power = 5 + p_ptr->depth / 4;
  
  /* Prevent the player's own traps granting exp. */
  if (f_ptr->flags & TF_M_TRAP) power = 0;

  /* Prevent runes granting exp. */
  if (f_ptr->flags & TF_RUNE) power = 0;
  
  /* Extract the disarm probability */
  j = i - power;
  
  /* Always have a small chance of success */
  if (j < 2) j = 2;
  
  /* Success */
  if ((power == 0) || (rand_int(100) < j))
    {
      /* Special message and decrement the count for runes. */
      if (f_ptr->flags & TF_RUNE)
	{
	  msg_format("You have removed the %s.", name);
	  num_runes_on_level[cave_feat[y][x] - FEAT_RUNE_HEAD]--;
	}
      
      /* Normal message otherwise */
      else msg_format("You have disarmed the %s.", name);
      
      /* If a Rogue's monster trap, decrement the trap count. */
      if (f_ptr->flags & TF_M_TRAP)
	num_trap_on_level--;
      
      
      /* Reward */
      gain_exp(power);
      
      /* Forget the trap */
      cave_info[y][x] &= ~(CAVE_MARK);
      
      /* Remove the trap (trees are hackish) */
      if (f_ptr->flags & TF_TREE)
	cave_set_feat(y, x, FEAT_TREE + tree_hack);
      else
	cave_set_feat(y, x, FEAT_FLOOR);
    }
  
  /* Failure -- Keep trying */
  else if ((i > 5) && (randint(i) > 5))
    {
      /* Failure */
      if (flush_failure) flush();
      
      /* Message */
      msg_format("You failed to disarm the %s.", name);
      
      /* We may keep trying */
      more = TRUE;
    }
  
  /* Failure -- Set off the trap */
  else
    {
      /* Message */
      msg_format("You set off the %s!", name);
      
      /* Hit the trap */
      hit_trap(y, x);
    }
  
  /* Result */
  return (more);
}


/**
 * Disarms a trap, a glyph, or a chest.
 */
void do_cmd_disarm(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  s16b o_idx;
  
  bool more = FALSE;
  
  /* Option: Pick a direction -TNB- */
  if (easy_disarm) 
    {
      int num_traps, num_chests;
      
      /* Count visible traps */
      num_traps = count_feats(&y, &x, is_trap, TRUE);
      
      /* Count chests (trapped) */
      num_chests = count_chests(&y, &x, TRUE);
      
      /* See if there's only one target */
      if (num_traps || num_chests)
	{
	  if (num_traps + num_chests <= 1)
	    p_ptr->command_dir = coords_to_dir(y, x);
	}
      
    }
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  /* Check for chests */
  o_idx = chest_check(y, x);
  
  
  /* Verify legality */
  if (!o_idx && !do_cmd_disarm_test(y, x)) return;
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Apply confusion */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
      
      /* Check for chests */
      o_idx = chest_check(y, x);
    }
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Monster */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");
      
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* Chest */
  if (o_idx)
    {
      /* Disarm the chest */
      more = do_cmd_disarm_chest(y, x, o_idx);
    }
  
  /* Disarm trap */
  else
    {
      /* Disarm the trap */
      more = do_cmd_disarm_aux(y, x);
    }
  
  /* Cancel repeat unless told not to */
  if (!more) disturb(0, 0);
}


/**
 * Determine if a given grid may be "bashed"
 */
static bool do_cmd_bash_test(int y, int x)
{
  /* Must have knowledge */
  if (!(cave_info[y][x] & (CAVE_MARK)))
    {
      /* Message */
      msg_print("You see nothing there.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Require a door */
  if (!((cave_feat[y][x] >= FEAT_DOOR_HEAD) &&
	(cave_feat[y][x] <= FEAT_DOOR_TAIL)))
    {
      /* Message */
      msg_print("You see nothing there to bash.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}


/**
 * Perform the basic "bash" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_bash_aux(int y, int x)
{
  int bash, temp;
  
  bool more = FALSE;
  
  
  /* Verify legality */
  if (!do_cmd_bash_test(y, x)) return (FALSE);
  
  
  /* Message */
  msg_print("You smash into the door!");
  
  /* Make a lot of noise. */
  add_wakeup_chance = 9000;
  
  /* Hack -- Bash power based on strength */
  /* (Ranges from 14 to 40 to 90 to 110) */
  bash = 10 + adj_str_hold[p_ptr->stat_ind[A_STR]];
  
  /* Extract door power */
  temp = ((cave_feat[y][x] - FEAT_DOOR_HEAD) & 0x07);
  
  /* Compare bash power to door power XXX XXX XXX */
  temp = (bash - (temp * 8));
  
  /* Hack -- always have a chance */
  if (temp < 1) temp = 1;
  
  /* Hack -- attempt to bash down the door */
  if (rand_int(100) < temp)
    {
      /* Message */
      message(MSG_OPENDOOR, 0, "The door crashes open!");
      
      /* Break down the door */
      if (rand_int(100) < 50)
	{
	  cave_set_feat(y, x, FEAT_BROKEN);
	}
      
      /* Open the door */
      else
	{
	  cave_set_feat(y, x, FEAT_OPEN);
	}
      
      /* Sound */
      sound(MSG_OPENDOOR);
      
      /* Update the visuals */
      p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
    }
  
  /* Saving throw against stun */
  else if (rand_int(100) < adj_dex_safe[p_ptr->stat_ind[A_DEX]] + p_ptr->lev)
    {
      /* Message */
      msg_print("The door holds firm.");
      
      /* Allow repeated bashing */
      more = TRUE;
    }
  
  /* High dexterity yields coolness */
  else
    {
      /* Message */
      msg_print("You are off-balance.");
      
      /* Hack -- Lose balance ala paralysis */
      (void)set_paralyzed(p_ptr->paralyzed + 2 + rand_int(2));
    }
  
  /* Result */
  return (more);
}


/**
 * Bash open a door, success based on character strength
 *
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Creatures can also open or bash doors, see elsewhere.
 */
void do_cmd_bash(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  bool more = FALSE;
  

  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;

  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Verify legality */
  if (!do_cmd_bash_test(y, x)) return;
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Apply confusion */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
    }
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Monster */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");
      
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* Bash the door */
  more = do_cmd_bash_aux(y, x);
  
  /* Cancel repeat unless told not to */
  if (!more) disturb(0, 0);
}



/**
 * Manipulate an adjacent grid in some way
 *
 * Attack monsters, tunnel through walls, disarm traps, open doors, 
 * or, for rogues, set traps and steal money.
 *
 * This command must always take energy, to prevent free detection
 * of invisible monsters.
 *
 * The "semantics" of this command must be chosen before the player
 * is confused, and it must be verified against the new grid.
 */
void do_cmd_alter(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  int feat;
  
  bool more = FALSE;
  
  monster_type *m_ptr;
  feature_type *f_ptr;
  
  /* Get a direction */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Original feature */
  feat = cave_feat[y][x];
  f_ptr = &f_info[feat];
  
  /* Must have knowledge to know feature XXX XXX */
  if (!(cave_info[y][x] & (CAVE_MARK))) feat = FEAT_NONE;
  
  
  /* Apply confusion */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
    }
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* If a monster is present, and visible, Rogues may steal from it.  
   * Otherwise, the player will simply attack. -LM-
   */
  if (cave_m_idx[y][x] > 0)
    {
      if ((check_ability(SP_STEAL)) && (!SCHANGE))
	{
	  m_ptr = &m_list[cave_m_idx[y][x]];
	  if (m_ptr->ml) py_steal(y, x);
	  else (void) py_attack(y, x, FALSE);
	}
      else (void) py_attack(y, x, FALSE);
    }

  /*
   * Some players can set traps.  Total number is checked in py_set_trap.
   */
  else if ((check_ability(SP_TRAP)) && (cave_trappable_bold(y, x)))
    {
      /* Make sure not to repeat */
      p_ptr->command_rep = 0;

      if (!py_set_trap(y, x)) return;
    }
  
  /* Disarm advanced monster traps */
  else if (f_ptr->flags & TF_M_TRAP)
    {
      if (feat == FEAT_MTRAP_HEAD)
	{
	  /* Modify */
	  if (!py_modify_trap(y, x)) return;
	}      
      else
	{
	  /* Disarm */
	  more = do_cmd_disarm_aux(y, x);
	}
    }
  
  /* Tunnel through walls */
  else if (f_ptr->flags & TF_ROCK)
    {
      /* Tunnel */
      more = do_cmd_tunnel_aux(y, x);
    }
  
  /* Bash jammed doors */
  else if (f_ptr->flags & TF_DOOR_JAMMED)
    {
      /* Bash */
      more = do_cmd_bash_aux(y, x);
    }
  
  /* Open closed doors */
  else if (f_ptr->flags & TF_DOOR_CLOSED)
    {
      /* Close */
      more = do_cmd_open_aux(y, x);
    }
  
  /* Disarm traps */
  else if (f_ptr->flags & TF_TRAP)
    {
      /* Disarm */
      more = do_cmd_disarm_aux(y, x);
    }
  
  /* Oops */
  else     {
      /* Oops */
      return;
    }
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Cancel repetition unless we can continue */
  if (!more) disturb(0, 0);
}


/**
 * Find the index of some "spikes", if possible.
 *
 * XXX XXX XXX Let user choose a pile of spikes, perhaps?
 */
static bool get_spike(int *ip)
{
  int i;
  
  /* Check every item in the pack */
  for (i = 0; i < INVEN_PACK; i++)
    {
      object_type *o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Check the "tval" code */
      if (o_ptr->tval == TV_SPIKE)
	{
	  /* Save the spike index */
	  (*ip) = i;
	  
	  /* Success */
	  return (TRUE);
	}
    }
  
  /* Oops */
  return (FALSE);
}


/**
 * Determine if a given grid may be "spiked"
 */
bool do_cmd_spike_test(int y, int x)
{
  /* Must have knowledge */
  if (!(cave_info[y][x] & (CAVE_MARK)))
    {
      /* Message */
      msg_print("You see nothing there.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Require a door */
  if (!((cave_feat[y][x] >= FEAT_DOOR_HEAD) &&
	(cave_feat[y][x] <= FEAT_DOOR_TAIL)))
    {
      /* Message */
      msg_print("You see nothing there to spike.");
      
      /* Nope */
      return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}


/**
 * Jam a closed door with a spike.  Now takes only 4/10ths normal energy
 * if no monster is in the way. -LM-
 *
 * This command may NOT be repeated
 */
void do_cmd_spike(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir, item = 0;
  
  
  /* Get a spike */
  if (!get_spike(&item))
    {
      /* Message */
      msg_print("You have no spikes!");
      
      /* Done */
      return;
    }
  
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Verify legality */
  if (!do_cmd_spike_test(y, x)) return;
  
  
  /* Take a partial turn.   Now jamming is more useful. */
  p_ptr->energy_use = 40;
  
  /* Confuse direction */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
    }
  
  
  /* Monster.  Make the action now take a full turn */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");
      
      p_ptr->energy_use += 60;
      
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* Go for it */
  else
    {
      /* Verify legality */
      if (!do_cmd_spike_test(y, x)) return;
      
      /* Successful jamming */
      msg_print("You jam the door with a spike.");
      
      /* Convert "locked" to "stuck" XXX XXX XXX */
      if (cave_feat[y][x] < FEAT_DOOR_HEAD + 0x08)
	{
	  cave_feat[y][x] += 0x08;
	}
      
      /* Add one spike to the door */
      if (cave_feat[y][x] < FEAT_DOOR_TAIL)
	{
	  cave_feat[y][x] += 0x01;
	}
      
      /* Use up, and describe, a single spike, from the bottom */
      inven_item_increase(item, -1);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
}



/**
 * Determine if a given grid may be "walked"
 */
static bool do_cmd_walk_test(int y, int x)
{
  /* Assume no monster. */
  monster_type *m_ptr = 0;
  
  /* Access the monster, if any is present. */
  if (cave_m_idx[y][x] != 0) m_ptr = &m_list[cave_m_idx[y][x]];
  
  /* If a monster can be seen, it can be attacked normally.  Code in cmd1.c 
   * controls whether a player can actually move to the destination grid.
   */
  if ((m_ptr) && (m_ptr->ml)) return (TRUE);
  
  
  /* Hack -- walking obtains knowledge XXX XXX */
  if (!(cave_info[y][x] & (CAVE_MARK))) return (TRUE);
  
  /* Check for being stuck in a web */
  if (cave_feat[p_ptr->py][p_ptr->px] == FEAT_WEB)
    {
      msg_print("You are stuck!");
      return (FALSE);
    }

  /* Require open space */
  if (!cave_passable_bold(y, x))
    {
      /* Door */
      if (cave_feat[y][x] < FEAT_SECRET)
	{
	  /* If easy_open_door option is on, doors are legal. */
	  if (easy_open) return (TRUE);
	  
	  /* Otherwise, let the player know of the door. */
	  else msg_print("There is a door in the way!");
	}
      
      /* Wall */
      else
	{
	  /* Inside or outside ? */
	  if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) ||
	      (stage_map[p_ptr->stage][STAGE_TYPE] == TOWN))
	    {
	      /* Message */
	      msg_print("There is a wall in the way!");
	    }
	  else
	    {
	      /* Message */
	      msg_print("There is rock in the way!");
	    }
	}
      
      /* Nope */
      return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}


/**
 * Helper function for the "walk" and "jump" commands
 */
static void do_cmd_walk_or_jump(int pickup)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Verify legality */
  if (!do_cmd_walk_test(y, x)) return;
  
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Confuse direction */
  if (confuse_dir(&dir))
    {
      /* Get location */
      y = py + ddy[dir];
      x = px + ddx[dir];
    }
  
  
  /* Verify legality */
  if (!do_cmd_walk_test(y, x)) return;
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Move the player */
  move_player(dir, pickup);
}


/**
 * Walk into a grid (pick up objects as set by the auto-pickup option)
 */
void do_cmd_walk(void)
{
  /* Move (usually pickup) */
  do_cmd_walk_or_jump(always_pickup);
}


/**
 * Jump into a grid (flip pickup mode)
 */
void do_cmd_jump(void)
{
  /* Move (usually do not pickup) */
  do_cmd_walk_or_jump(!always_pickup);
}


/**
 * Start running.
 *
 * Note that running while confused is not allowed.
 */
void do_cmd_run(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, dir;
  
  
  /* Hack XXX XXX XXX */
  if (p_ptr->confused)
    {
      msg_print("You are too confused!");
      return;
    }
  
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Verify legality */
  if (!do_cmd_walk_test(y, x)) return;
  
  
  /* Start run */
  run_step(dir);
}



/**
 * Start running with pathfinder.
 *
 * Note that running while confused is not allowed.
 */
void do_cmd_pathfind(int y, int x)
{
  int py = p_ptr->py;
  int px = p_ptr->px;

  /* Hack XXX XXX XXX */
  if (p_ptr->confused)
    {
      msg_print("You are too confused!");
      return;
    }
  
  /* Hack -- handle stuck players */
  if (cave_feat[py][px] == FEAT_WEB)
    {
      /* Tell the player */
      msg_print("You are stuck!");
      
      return;
    }
  
  if (findpath(y, x))
    {
      p_ptr->running = 1000;

      /* Calculate torch radius */
      p_ptr->update |= (PU_TORCH);

      p_ptr->running_withpathfind = TRUE;
      run_step(0);
    }
}


/**
 * Stay still.  Search.   Enter stores.
 * Pick up treasure and objects if "pickup" is true.
 */
static void do_cmd_hold_or_stay(int pickup)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  
  /* Allow repeated command */
  if (p_ptr->command_arg)
    {
      /* Set repeat count */
      p_ptr->command_rep = p_ptr->command_arg - 1;
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATE);
      
      /* Cancel the arg */
      p_ptr->command_arg = 0;
    }
  
  /* Take a turn */
  p_ptr->energy_use = 100;
  
  /* Spontaneous Searching */
  if (p_ptr->skill_fos >= 50)
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
  
  /* Handle "objects".  Do not charge extra energy for objects picked up. */
  (void)py_pickup(pickup, p_ptr->py, p_ptr->px);
  
  /* Hack -- enter a store if we are on one */
  if ((cave_feat[py][px] >= FEAT_SHOP_HEAD) &&
      (cave_feat[py][px] <= FEAT_SHOP_TAIL))
    {
      /* Disturb */
      disturb(0, 0);
      
      /* Hack -- enter store */
      p_ptr->command_new = '_';
    }
}


/**
 * Hold still (usually pickup)
 */
void do_cmd_hold(void)
{
  /* Hold still (usually pickup) */
  do_cmd_hold_or_stay(always_pickup);
}


/**
 * Pick up objects on the floor beneath you. -LM-
 */
void do_cmd_pickup(void)
{
  int energy_cost = 1, item;
  
  /* Do we have an item? */
  if (p_ptr->command_item)
    {
      /* Get the item */
      item = handle_item();

      /* Pick it up */
      py_pickup_aux(0 - item);

      /* Charge some energy. */
      p_ptr->energy_use = energy_cost;
    }

  /* Get items */
  else
    {
      /* Pick up floor objects, forcing a menu for multiple objects. */
      energy_cost = py_pickup(2, p_ptr->py, p_ptr->px) * 10;
      
      /* Maximum time expenditure is a full turn. */
      if (energy_cost > 100) energy_cost = 100;
      
      /* Charge this amount of energy. */
      p_ptr->energy_use = energy_cost;
    }
}


/**
 * Rest (restores hit points and mana and such)
 */
void do_cmd_rest(void)
{
  bool got_string;

  /* Prompt for time if needed */
  if (p_ptr->command_arg <= 0)
    {
      cptr p;
      char out_val[80];
      
      if (small_screen)
	p  = "Rest ('*': HP/SP, '&': needed, '$': sun): ";
      else
	p  = "Rest (0-9999, '*' for HP/SP, '&' as needed, '$' until sunrise/set): ";
      
      /* Default */
      strcpy(out_val, "&");

      /* Buttons */
      add_button("*", '*');
      add_button("$", '$');
      
      /* Ask for duration */
      got_string = get_string(p, out_val, 5); 
	
      kill_button('*');
      kill_button('$');
	
      if (!got_string) return;
	
      
      /* Rest until done */
      if (out_val[0] == '&')
	{
	  p_ptr->command_arg = (-2);
	}
      
      /* Rest all night (or day) */
      else if (out_val[0] == '$')
	{
	  p_ptr->command_arg = (-3);
	}
      
      /* Rest a lot */
      else if (out_val[0] == '*')
	{
	  p_ptr->command_arg = (-1);
	}
      
      /* Rest some */
      else
	{
	  p_ptr->command_arg = atoi(out_val);
	  if (p_ptr->command_arg <= 0) return;
	}
    }
  
  
  /* Paranoia */
  if (p_ptr->command_arg > 9999) p_ptr->command_arg = 9999;
  
  
  /* Take a turn XXX XXX XXX (?) */
  p_ptr->energy_use = 100;
  
  /* Save the rest code */
  p_ptr->resting = p_ptr->command_arg;
  
  /* Cancel the arg */
  p_ptr->command_arg = 0;
  
  /* Cancel searching */
  p_ptr->searching = FALSE;
  
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Redraw the state */
  p_ptr->redraw |= (PR_STATE);
  
  /* Handle stuff */
  handle_stuff();
  
  /* Refresh XXX XXX XXX */
  if (fresh_before) Term_fresh();
}
