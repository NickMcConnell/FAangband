/** \file monster2.c 
    \brief Monster generation, learning and removal

 * Monster processing, compacting, generation, goody drops, deletion, 
 * place the player, monsters, and their escorts at a given location, 
 * generation of monsters, summoning, monster reaction to pain 
 * levels, monster learning.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick & Bahman Rabii, 
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
 * Delete a monster by index.
 *
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(int i)
{
  int x, y;
  
  monster_type *m_ptr = &m_list[i];
  
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  monster_lore *l_ptr = &l_list[m_ptr->r_idx];
  
  s16b this_o_idx, next_o_idx = 0;
  
  
  /* Get location */
  y = m_ptr->fy;
  x = m_ptr->fx;
  
  
  /* Hack -- Reduce the racial counter */
  r_ptr->cur_num--;
  
  /* Hack -- count the number of "reproducers" */
  if (r_ptr->flags2 & (RF2_MULTIPLY)) num_repro--;
  
  /* Hack -- remove target monster */
  if (p_ptr->target_who == i) target_set_monster(0);
  
  /* Hack -- remove tracked monster */
  if (p_ptr->health_who == i) health_track(0);
  
  
  /* Monster is gone */
  cave_m_idx[y][x] = 0;
  
  /* Total Hack -- If the monster was a player ghost, remove it from 
   * the monster memory, ensure that it never appears again, clear 
   * its bones file selector and allow the next ghost to speak.
   */
  if (r_ptr->flags2 & (RF2_PLAYER_GHOST))
    {
      l_ptr->sights = 0;
      l_ptr->deaths = 0;
      l_ptr->pkills = 0;
      l_ptr->tkills = 0;
      bones_selector = 0;
      ghost_has_spoken = FALSE;
      r_ptr->rarity = 0;
    }
  
  /* Delete objects */
  for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Hack -- efficiency */
      o_ptr->held_m_idx = 0;
      
      /* Delete the object */
      delete_object_idx(this_o_idx);
    }
  

  /* Wipe the Monster */
  (void)WIPE(m_ptr, monster_type);
  
  /* Count monsters */
  m_cnt--;
  
  
  /* Visual update */
  lite_spot(y, x);
}


/**
 * Delete the monster, if any, at a given location
 */
void delete_monster(int y, int x)
{
  /* Paranoia */
  if (!in_bounds(y, x)) return;
  
  /* Delete the monster (if any) */
  if (cave_m_idx[y][x] > 0) delete_monster_idx(cave_m_idx[y][x]);
}


/**
 * Move an object from index i1 to index i2 in the object list
 */
static void compact_monsters_aux(int i1, int i2)
{
  int y, x;
  
  monster_type *m_ptr;
  
  s16b this_o_idx, next_o_idx = 0;
  
  
  /* Do nothing */
  if (i1 == i2) return;
  
  
  /* Old monster */
  m_ptr = &m_list[i1];
  
  /* Location */
  y = m_ptr->fy;
  x = m_ptr->fx;
  
  /* Update the cave */
  cave_m_idx[y][x] = i2;
  
  /* Repair objects being carried by monster */
  for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Reset monster pointer */
      o_ptr->held_m_idx = i2;
    }
  
  /* Hack -- Update the target */
  if (p_ptr->target_who == i1) p_ptr->target_who = i2;
  
  /* Hack -- Update the health bar */
  if (p_ptr->health_who == i1) p_ptr->health_who = i2;
  
  /* Hack -- move monster */
  (void)COPY(&m_list[i2], &m_list[i1], monster_type);
  
  /* Hack -- wipe hole */
  (void)WIPE(&m_list[i1], monster_type);
}


/**
 * Compact and Reorder the monster list
 *
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" monsters, we base the saving throw
 * on a combination of monster level, distance from player, and
 * current "desperation".
 *
 * After "compacting" (if needed), we "reorder" the monsters into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_monsters(int size)
{
  int i, j, num, cnt;
  
  int cur_lev, cur_dis, chance;
  
  
  /* Message (only if compacting) */
  if (size) msg_print("Compacting monsters...");
  
  
  /* Compact at least 'size' objects */
  for (num = 0, cnt = 1; num < size; cnt++)
    {
      /* Get more vicious each iteration */
      cur_lev = 5 * cnt;
      
      /* Get closer each iteration */
      cur_dis = 5 * (20 - cnt);
      
      j = 0;
      
      /* Check all the monsters */
      for (i = 1; i < m_max; i++)
	{
	  monster_type *m_ptr = &m_list[i];
	  
	  monster_race *r_ptr = &r_info[m_ptr->r_idx];
	  
	  /* Paranoia -- skip "dead" monsters */
	  if (!m_ptr->r_idx) 
	    {
	      if (j++ >= size) return;
	      else continue;
	    }
	  
	  /* Hack -- High level monsters start out "immune" */
	  if (r_ptr->level > cur_lev) continue;
	  
	  /* Ignore nearby monsters */
	  if ((cur_dis > 0) && (m_ptr->cdis < cur_dis)) continue;
	  
	  /* Saving throw chance */
	  chance = 90;
	  
	  /* Only compact "Quest" Monsters in emergencies */
	  if ((r_ptr->flags1 & (RF1_QUESTOR)) && (cnt < 1000)) chance = 100;
	  
	  /* Try not to compact Unique Monsters */
	  if (r_ptr->flags1 & (RF1_UNIQUE)) chance = 99;
	  
	  /* All monsters get a saving throw */
	  if (rand_int(100) < chance) continue;
	  
	  /* Delete the monster */
	  delete_monster_idx(i);
	  
	  /* Count the monster */
	  num++;
	}
    }
  
  
  /* Excise dead monsters (backwards!) */
  for (i = m_max - 1; i >= 1; i--)
    {
      /* Get the i'th monster */
      monster_type *m_ptr = &m_list[i];
      
      /* Skip real monsters */
      if (m_ptr->r_idx) continue;
      
      /* Move last monster into open hole */
      compact_monsters_aux(m_max - 1, i);
      
      /* Compress "m_max" */
      m_max--;
    }
}


/**
 * Delete/Remove all the monsters when the player leaves the level
 *
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 */
void wipe_m_list(void)
{
  int i;
  
  /* Delete all the monsters */
  for (i = m_max - 1; i >= 1; i--)
    {
      monster_type *m_ptr = &m_list[i];
      
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      monster_lore *l_ptr = &l_list[m_ptr->r_idx];
      
      /* Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Total Hack -- Clear player ghost information. */
      if (r_ptr->flags2 & (RF2_PLAYER_GHOST))
	{
	  l_ptr->sights = 0;
	  l_ptr->deaths = 0;
	  l_ptr->pkills = 0;
	  l_ptr->tkills = 0;
	  bones_selector = 0;
	  ghost_has_spoken = FALSE;
	  r_ptr->rarity = 0;
	}
      
      /* Hack -- Reduce the racial counter */
      r_ptr->cur_num--;
      
      /* Monster is gone */
      cave_m_idx[m_ptr->fy][m_ptr->fx] = 0;
      
      /* Wipe the Monster */
      (void)WIPE(m_ptr, monster_type);
    }

  /* Hack - wipe the player */
  cave_m_idx[p_ptr->py][p_ptr->px] = 0;
  
  /* Reset "m_max" */
  m_max = 1;
  
  /* Reset "m_cnt" */
  m_cnt = 0;
  
  /* Hack -- reset "reproducer" count */
  num_repro = 0;
  
  /* Hack -- no more target */
  target_set_monster(0);
  
  /* Hack -- no more tracking */
  health_track(0);
  
  /* Hack -- make sure there is no player ghost */
  bones_selector = 0;
}


/**
 * Acquires and returns the index of a "free" monster.
 *
 * This routine should almost never fail, but it *can* happen.
 */
s16b m_pop(void)
{
  int i;
  
  
  /* Normal allocation */
  if (m_max < z_info->m_max)
    {
      /* Access the next hole */
      i = m_max;
      
      /* Expand the array */
      m_max++;
      
      /* Count monsters */
      m_cnt++;
      
      /* Return the index */
      return (i);
    }
  
  
  /* Recycle dead monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr;
      
      /* Acquire monster */
      m_ptr = &m_list[i];
      
      /* Skip live monsters */
      if (m_ptr->r_idx) continue;
      
      /* Count monsters */
      m_cnt++;
      
      /* Use this monster */
      return (i);
    }
  
  
  /* Warn the player (except during dungeon creation) */
  if (character_dungeon) msg_print("Too many monsters!");
  
  /* Try not to crash */
  return (0);
}


/**
 * Apply a "monster restriction function" to the "monster allocation table"
 */
errr get_mon_num_prep(void)
{
  int i;
  
  /* Scan the allocation table */
  for (i = 0; i < alloc_race_size; i++)
    {
      /* Get the entry */
      alloc_entry *entry = &alloc_race_table[i];
      
      /* Accept monsters which pass the restriction, if any */
      if (!get_mon_num_hook || (*get_mon_num_hook)(entry->index))
	{
	  /* Accept this monster */
	  entry->prob2 = entry->prob1;
	}
      
      /* Do not use this monster */
      else
	{
	  /* Decline this monster */
	  entry->prob2 = 0;
	}
    }
  
  /* Success */
  return (0);
}



/**
 * Choose a monster race that seems "appropriate" to the given level
 *
 * We use this function, not only to pick a monster but to build a 
 * table of probabilities.  This table can be used again and again, if 
 * certain conditions (generation level being the most important) don't 
 * change.
 *
 * This function uses the "prob2" field of the "monster allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" monster, in
 * a relatively efficient manner.
 *
 * Note that "town" monsters will *only* be created in the town, and
 * "normal" monsters will *never* be created in the town, unless the
 * "level" is "modified", for example, by polymorph or summoning.
 *
 * There is a small chance (1/40) of "boosting" the given depth by
 * a small amount (up to four levels), except in the town.
 *
 * It is (slightly) more likely to acquire a monster of the given level
 * than one of a lower level.  This is done by choosing several monsters
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no monsters are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
s16b get_mon_num(int level)
{
  int i, d;
  
  int r_idx;
  
  long value;
  int failure = 0;
  int temp_level = level;
  
  monster_race *r_ptr;
  
  alloc_entry *table = alloc_race_table;
  
  /* Low-level monsters avoid the deep dungeon. */
  int depth_rare = 2 * level / 3;
  int depth_very_rare = level / 3;
  
  
  /* Sometimes, monsters in the dungeon can be out of depth */
  if (p_ptr->depth != 0)
    {
      /* Occasional boost to maximum level */
      if (rand_int(NASTY_MON) == 0)
	{
	  /* Pick a level bonus */
	  d = level / 10 + 1;
	  
	  /* Boost the level */
	  temp_level += ((d < 5) ? d : 5);
	  
	  /* Occasional second boost */
	  if (rand_int(NASTY_MON) == 0)
	    {
	      /* Pick a level bonus */
	      d = level / 10 + 1;
	      
	      /* Boost the level */
	      temp_level += ((d < 5) ? d : 5);
	    }
	}
    }
  
  
  /* Try hard to find a suitable monster */
  while (TRUE)
    {
      /* Reset sum of final monster probabilities. */
      alloc_race_total = 0L;
      
      /* Process probabilities */
      for (i = 0; i < alloc_race_size; i++)
	{
	  /* Assume no probability */
	  table[i].prob3 = 0;
	  
	  /* Ignore illegal monsters */
	  if (!table[i].prob2) continue;
	  
	  /* Monsters are sorted by depth */
	  if (table[i].level > temp_level) continue;
	  
	  /* Hack -- No town monsters in dungeon */
	  if ((p_ptr->depth != 0) && (table[i].level < 1)) continue;
	  
	  /* Get the monster index */
	  r_idx = table[i].index;
	  
	  /* Get the actual race */
	  r_ptr = &r_info[r_idx];
	  
	  /* Hack -- some monsters are unique */
	  if ((r_ptr->flags1 & (RF1_UNIQUE)) && 
	      (r_ptr->cur_num >= r_ptr->max_num)) continue;
	  
	  /* Forced-depth monsters only appear at their level. */
	  if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && 
	      (r_ptr->level != p_ptr->depth)) continue;
	  
	  /* Hack - handle dungeon specific -NRM- */
	  if ((r_ptr->flags2 & (RF2_RUDH)) &&
	      (stage_map[p_ptr->stage][LOCALITY] != AMON_RUDH))
	    continue;
	  
	  if ((r_ptr->flags2 & (RF2_NARGOTHROND)) && 
	      (stage_map[p_ptr->stage][LOCALITY] != NARGOTHROND))
	    continue;
	  
	  if ((r_ptr->flags2 & (RF2_DUNGORTHEB)) &&
	      (stage_map[p_ptr->stage][LOCALITY] != NAN_DUNGORTHEB))
	    continue;
	  
	  if ((r_ptr->flags2 & (RF2_GAURHOTH)) &&
	      (stage_map[p_ptr->stage][LOCALITY] != TOL_IN_GAURHOTH))
	    continue;
	  
	  if ((r_ptr->flags2 & (RF2_ANGBAND)) &&
	      (stage_map[p_ptr->stage][LOCALITY] != ANGBAND))
	    continue;
	  
	  /* Hack - dungeon-only monsters */
	  if ((r_ptr->flags3 & (RF3_DUNGEON)) &&
	      (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE))
	    continue;
      
	  /* Hack - choose flying monsters for mountaintop */
	  if ((stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP) && 
	      !(r_ptr->flags2 & (RF2_FLYING)))
	    continue;
      
	  /* Accept */
	  table[i].prob3 = table[i].prob2;

	  /* Now modifications for locality etc follow -NRM- */

	  /* Nan Dungortheb is spiderland, bad for humans and humanoids */
	  if (stage_map[p_ptr->stage][LOCALITY] == NAN_DUNGORTHEB) 
	    {
	      if (r_ptr->d_char == 'S') 
		table[i].prob3 *= 5;
	      if ((r_ptr->d_char == 'p') || (r_ptr->d_char == 'h'))
		table[i].prob3 /= 3;
	    }
	  
	  /* Tol-In-Gaurhoth is full of wolves and undead */
	  if (stage_map[p_ptr->stage][LOCALITY] == TOL_IN_GAURHOTH)
	    {
	      if ((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z')) 
		table[i].prob3 *= 4;
	      else if (r_ptr->flags3 & RF3_UNDEAD)
		table[i].prob3 *=2;
	    }

	  /* Most animals don't like desert and mountains */
	  if ((stage_map[p_ptr->stage][STAGE_TYPE] == DESERT) ||
	      (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAIN))
	    {
	      if ((r_ptr->d_char == 'R') || (r_ptr->d_char == 'J') ||
		  (r_ptr->d_char == 'c')) 
		table[i].prob3 *= 2;
	      else if (r_ptr->flags3 & RF3_ANIMAL)
		table[i].prob3 /=2;
	    }
	  
	  /* Most animals do like forest */
	  if (stage_map[p_ptr->stage][STAGE_TYPE] == FOREST)
	    {
	      if (r_ptr->d_char == 'R') 
		table[i].prob3 /= 2;
	      else if ((r_ptr->flags3 & RF3_ANIMAL) && (r_ptr->d_char != 'Z'))
		table[i].prob3 *=2;
	    }
	  
	      
	  
	  /* Keep low-level monsters rare */
	  if (table[i].level < depth_rare) table[i].prob3 /= 4;
	  if (table[i].level < depth_very_rare) table[i].prob3 /= 4;
	  
	  /* Sum up probabilities */
	  alloc_race_total += table[i].prob3;
	}
      
      /* No legal monsters */
      if (alloc_race_total == 0) 
	{
	  failure++;
	  
	  if (failure == 1)
	    {
	      /* Try relaxing the level restrictions */
	      if (p_ptr->themed_level) temp_level += 20;
	      else temp_level += 10;
	    }
	  else
	    {
	      /* Our monster restrictions are too stringent. */
	      return (0);
	    }
	}
      
      /* Success */
      else break;
    }
  
  /* Pick a monster */
  value = rand_int(alloc_race_total);
  
  /* Find the monster */
  for (i = 0; i < alloc_race_size; i++)
    {
      /* Found the entry */
      if (value < table[i].prob3) break;
      
      /* Decrement */
      value = value - table[i].prob3;
    }
  
  /* Result */
  return (table[i].index);
}



/**
 * A replacement for "get_mon_num()", for use when that function has 
 * built up a suitable table of monster probabilities, and all we want 
 * to do is pull another monster from it.
 *
 * Usage of this function has a curious and quite intentional effect:  
 * on rare occasion, 1 in NASTY_MON times, the effective generation level 
 * is raised somewhat.  Most monsters will actually end up being in depth, 
 * but not all...
 */
s16b get_mon_num_quick(int level)
{
  int i;
  long value;
  alloc_entry *table = alloc_race_table;
  
  /* 
   * No monsters available.  XXX XXX - try using the standard 
   * function again, although it probably failed the first time.
   */
  if (!alloc_race_total) return (get_mon_num(level));
  
  
  /* Pick a monster */
  value = rand_int(alloc_race_total);
  
  /* Find the monster */
  for (i = 0; i < alloc_race_size; i++)
    {
      /* Found the entry */
      if (value < table[i].prob3) break;
      
      /* Decrement */
      value = value - table[i].prob3;
    }
  
  /* Result */
  return (table[i].index);
}

/**
 * Display visible monsters in a window
 */
void display_monlist(void)
{
  int idx, n, i;
  int line = 0;
  
  char *m_name;
  char buf[80];
  
  monster_type *m_ptr;
  monster_race *r_ptr;
  
  u16b *race_counts, *neutral_counts;
  
  /* Allocate the arrays */
  C_MAKE(race_counts, z_info->r_max, u16b);
  C_MAKE(neutral_counts, z_info->r_max, u16b);
  
  /* Iterate over m_list */
  for (idx = 1; idx < z_info->m_max; idx++)
    {
      m_ptr = &m_list[idx];
      
      /* Only visible monsters */
      if (!m_ptr->ml) continue;
      
      /* Bump the count for this race */
      race_counts[m_ptr->r_idx]++;

      /* Check neutrality */
      if (m_ptr->hostile >= 0) neutral_counts[m_ptr->r_idx]++;
    }
  
  
  /* Iterate over m_list ( again :-/ ) */
  for (idx = 1; idx < z_info->m_max; idx++)
    {
      int attitudes = 1;

      m_ptr = &m_list[idx];
      
      /* Only visible monsters */
      if (!m_ptr->ml) continue;
      
      /* Do each race only once */
      if (!race_counts[m_ptr->r_idx]) continue;
      
      /* Get monster race */
      r_ptr = &r_info[m_ptr->r_idx];
      
      /* Get the monster name */
      m_name = r_name + r_ptr->name;
      
      /* Obtain the length of the description */
      n = strlen(m_name);
      
      /* Hack - translate if we do that */
      if (Term->xchar_hook)
	xstr_trans(m_name, (Term->xchar_hook(128) == 128));

      /* See if there are any neutrals */
      if (neutral_counts[m_ptr->r_idx] > 0) attitudes++;

      /* Extract hostile count */
      race_counts[m_ptr->r_idx] -= neutral_counts[m_ptr->r_idx];

      /* Display the entry itself */
      for (i = 0; i < attitudes; i++)
	{
	  /* Skip straight to neutrals if no hostiles, finish if no neutrals */
	  if ((race_counts[m_ptr->r_idx] == 0) && (i == 0)) continue;
	  if ((neutral_counts[m_ptr->r_idx] == 0) && (i == 1)) break;

	  /* Name */
	  Term_putstr(0, line, n, TERM_WHITE, m_name);

	  /* Attitude */
	  if (attitudes > 1) 
	    {
	      Term_addstr(-1, TERM_WHITE, 
			  (i == 0) ? " (hostile)" : " (neutral)");
	      n += 10;
	    }
      
	  /* Append the "standard" attr/char info */
	  Term_addstr(-1, TERM_WHITE, " ('");
	  Term_addch(r_ptr->d_attr, r_ptr->d_char);
	  Term_addstr(-1, TERM_WHITE, "')");
	  n += 6;
      
	  /* Monster graphic on one line */
	  if (!(use_dbltile) && !(use_trptile))
	    {
	      /* Append the "optional" attr/char info */
	      Term_addstr(-1, TERM_WHITE, "/('");
	      
	      Term_addch(r_ptr->x_attr, r_ptr->x_char);
	      
	      if (use_bigtile)
		{
		  if (r_ptr->x_attr & 0x80)
		    Term_addch(255, -1);
		  else
		    Term_addch(0, ' ');
		  
		  n++;
		}
	      
	      Term_addstr(-1, TERM_WHITE, "'):");
	      n += 7;
	    }

	  /* Add race count */
	  if (i == 0) sprintf(buf, "%d", race_counts[m_ptr->r_idx]);
	  else sprintf(buf, "%d", neutral_counts[m_ptr->r_idx]);
	  Term_addch(TERM_WHITE, '[');
	  Term_addstr(strlen(buf), TERM_WHITE, buf);
	  Term_addch(TERM_WHITE, ']');
	  n += strlen(buf) + 2;
      
	  /* Don't do this race again */
	  if (i == 0) race_counts[m_ptr->r_idx] = 0;
	  else neutral_counts[m_ptr->r_idx] = 0;
      
	  /* Erase the rest of the line */
	  Term_erase(n, line, 255);
	  
	  /* Bump line counter */
	  line++;
	}
    }
  
  /* Free the race counters */
  FREE(race_counts); 
  FREE(neutral_counts); 
  
  /* Erase the rest of the window */
  for (idx = line; idx < Term->hgt; idx++)
    {
      /* Erase the line */
      Term_erase(0, idx, 255);
    }
}


/**
 * Build a string describing a monster in some way.
 *
 * We can correctly describe monsters based on their visibility.
 * We can force all monsters to be treated as visible or invisible.
 * We can build nominatives, objectives, possessives, or reflexives.
 * We can selectively pronominalize hidden, visible, or all monsters.
 * We can use definite or indefinite descriptions for hidden monsters.
 * We can use definite or indefinite descriptions for visible monsters.
 *
 * Pronominalization involves the gender whenever possible and allowed,
 * so that by cleverly requesting pronominalization / visibility, you
 * can get messages like "You hit someone.  She screams in agony!".
 *
 * Reflexives are acquired by requesting Objective plus Possessive.
 *
 * I am assuming that no monster name is more than 65 characters long,
 * so that "char desc[80];" is sufficiently large for any result, even
 * when the "offscreen" notation is added.
 *
 * Note that the "possessive" for certain unique monsters will look
 * really silly, as in "Morgoth, King of Darkness's".  We should
 * perhaps add a flag to "remove" any "descriptives" in the name.
 *
 * Note that "offscreen" monsters will get a special "(offscreen)"
 * notation in their name if they are visible but offscreen.  This
 * may look silly with possessives, as in "the rat's (offscreen)".
 * Perhaps the "offscreen" descriptor should be abbreviated.
 *
 * Mode Flags:
 *   - 0x01 --> Objective (or Reflexive)
 *   - 0x02 --> Possessive (or Reflexive)
 *   - 0x04 --> Use indefinites for hidden monsters ("something")
 *   - 0x08 --> Use indefinites for visible monsters ("a kobold")
 *   - 0x10 --> Pronominalize hidden monsters
 *   - 0x20 --> Pronominalize visible monsters
 *   - 0x40 --> Assume the monster is hidden
 *   - 0x80 --> Assume the monster is visible
 *
 * Useful Modes:
 *   - 0x00 --> Full nominative name ("the kobold") or "it"
 *   - 0x04 --> Full nominative name ("the kobold") or "something"
 *   - 0x80 --> Genocide resistance name ("the kobold")
 *   - 0x88 --> Killing name ("a kobold")
 *   - 0x22 --> Possessive, genderized if visable ("his") or "its"
 *   - 0x23 --> Reflexive, genderized if visable ("himself") or "itself"
 */
void monster_desc(char *desc, monster_type *m_ptr, int mode)
{
  cptr res;
  
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  cptr name = (r_name + r_ptr->name);
  char undead_name[40] = "oops";
  
  bool seen, pron;
  
  /* Can we "see" it (forced, or not hidden + visible) */
  seen = ((mode & (0x80)) || (!(mode & (0x40)) && m_ptr->ml));
  
  /* Sexed Pronouns (seen and forced, or unseen and allowed) */
  pron = ((seen && (mode & (0x20))) || (!seen && (mode & (0x10))));
  
  
  /* First, try using pronouns, or describing hidden monsters */
  if (!seen || pron)
    {
      /* an encoding of the monster "sex" */
      int kind = 0x00;
      
      /* Clear the description. */
      strcpy(desc, "                         ");
      
      /* Extract the gender (if applicable) */
      if (r_ptr->flags1 & (RF1_FEMALE)) kind = 0x20;
      else if (r_ptr->flags1 & (RF1_MALE)) kind = 0x10;
      
      /* Ignore the gender (if desired) */
      if (!m_ptr || !pron) kind = 0x00;
      
      
      /* Assume simple result */
      res = "it";

      /* Brute force: split on the possibilities */
      switch (kind + (mode & 0x07))
	{
	  /* Neuter, or unknown */
	case 0x00: res = "it"; break;
	case 0x01: res = "it"; break;
	case 0x02: res = "its"; break;
	case 0x03: res = "itself"; break;
	case 0x04: res = "something"; break;
	case 0x05: res = "something"; break;
	case 0x06: res = "something's"; break;
	case 0x07: res = "itself"; break;
	  
	  /* Male (assume human if vague) */
	case 0x10: res = "he"; break;
	case 0x11: res = "him"; break;
	case 0x12: res = "his"; break;
	case 0x13: res = "himself"; break;
	case 0x14: res = "someone"; break;
	case 0x15: res = "someone"; break;
	case 0x16: res = "someone's"; break;
	case 0x17: res = "himself"; break;
	  
	  /* Female (assume human if vague) */
	case 0x20: res = "she"; break;
	case 0x21: res = "her"; break;
	case 0x22: res = "her"; break;
	case 0x23: res = "herself"; break;
	case 0x24: res = "someone"; break;
	case 0x25: res = "someone"; break;
	case 0x26: res = "someone's"; break;
	case 0x27: res = "herself"; break;
	}
      
      /* Copy the result */
      strcpy(desc, res);
    }
  
  
  /* Handle visible monsters, "reflexive" request */
  else if ((mode & 0x02) && (mode & 0x01))
    {
      /* The monster is visible, so use its gender */
      if (r_ptr->flags1 & (RF1_FEMALE)) strcpy(desc, "herself");
      else if (r_ptr->flags1 & (RF1_MALE)) strcpy(desc, "himself");
      else strcpy(desc, "itself");
    }
  
  
  /* Handle all other visible monster requests */
  else
    {
      cptr race_name = NULL;
      
      /* Get a racial prefix if necessary */
      if (m_ptr->p_race != NON_RACIAL) 
	race_name = p_name + p_info[m_ptr->p_race].name;
	
      /* It could be a player ghost. */
      if (r_ptr->flags2 & (RF2_PLAYER_GHOST))
	{
	  /* Get the ghost name. */
	  strcpy(desc, ghost_name);
	  
	  /* Get the undead name. */
	  strcpy(undead_name, r_name + r_ptr->name);
	  
	  /* Build the ghost name. */
	  strcat(desc, ", the ");
	  strcat(desc, undead_name);
	}
      
      /* It could be a Unique */
      else if (r_ptr->flags1 & (RF1_UNIQUE))
	{
	  /* Start with the name (thus nominative and objective) */
	  strcpy(desc, name);
	}
      
      /* It could be an indefinite monster */
      else if (mode & 0x08)
	{
	  bool vowel;
	  char first[2];

	  if (race_name) vowel = is_a_vowel(race_name[0]);
	  else vowel = is_a_vowel(name[0]);

	  /* XXX Check plurality for "some" */
	  
	  /* Indefinite monsters need an indefinite article */
	  strcpy(desc, vowel ? "an " : "a ");

	  /* Hack - no capital if there's a race name first */
	  if (race_name) 
	    {
	      strcat(desc, race_name);
	      strcat(desc, " ");
	      first[0] = my_tolower(name[0]);
	      first[1] = '\0';
	      strcat(desc, first);
	      strcat(desc, name + 1);
	    }
	  else strcat(desc, name);
	}
      
      /* It could be a normal, definite, monster */
      else
	{
	  char first[2];

	  /* Definite monsters need a definite article */
	  strcpy(desc, "the ");
	  /* Hack - no capital if there's a race name first */
	  if (race_name) 
	    {
	      strcat(desc, race_name);
	      strcat(desc, " ");
	      first[0] = my_tolower(name[0]);
	      first[1] = '\0';
	      strcat(desc, first);
	      strcat(desc, name + 1);
	    }
	  else strcat(desc, name);
	}
      
      /* Handle the Possessive as a special afterthought */
      if (mode & 0x02)
	{
	  /* XXX Check for trailing "s" */
	  
	  /* Simply append "apostrophe" and "s" */
	  strcat(desc, "'s");
	}
      
      /* Mention "offscreen" monsters XXX XXX */
      if (!panel_contains(m_ptr->fy, m_ptr->fx))
	{
	  /* Append special notation */
	  strcat(desc, " (offscreen)");
	}
    }
}


/**
 * Build a string describing a monster race, currently used for quests.
 *
 * Assumes a singular monster.  This may need to be run through the
 * plural_aux function in the quest.c file.  (Changes "wolf" to
 * wolves, etc.....)
 *
 * I am assuming that no monster name is more than 65 characters long,
 * so that "char desc[80];" is sufficiently large for any result, even
 * when the "offscreen" notation is added.
 *
 */
void monster_desc_race(char *desc, size_t max, int r_idx)
{
  monster_race *r_ptr = &r_info[r_idx];
  
  cptr name = (r_name + r_ptr->name);
  
  /* Write the name */
  my_strcpy(desc, name, max);
}



/**
 * Learn about a monster (by "probing" it)
 */
void lore_do_probe(int m_idx)
{
  monster_type *m_ptr = &m_list[m_idx];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  monster_lore *l_ptr = &l_list[m_ptr->r_idx];
  
  /* Hack -- Memorize some flags */
  l_ptr->flags1 = r_ptr->flags1;
  l_ptr->flags2 = r_ptr->flags2;
  l_ptr->flags3 = r_ptr->flags3;
  
  /* Update monster recall window */
  if (p_ptr->monster_race_idx == m_ptr->r_idx)
    {
      /* Window stuff */
      p_ptr->window |= (PW_MONSTER);
    }
}


/**
 * Take note that the given monster just dropped some treasure
 *
 * Note that learning the "GOOD"/"GREAT" flags gives information
 * about the treasure (even when the monster is killed for the first
 * time, such as uniques, and the treasure has not been examined yet).
 *
 * This "indirect" method is used to prevent the player from learning
 * exactly how much treasure a monster can drop from observing only
 * a single example of a drop.  This method actually observes how much
 * gold and items are dropped, and remembers that information to be
 * described later by the monster recall code.
 */
void lore_treasure(int m_idx, int num_item, int num_gold)
{
  monster_type *m_ptr = &m_list[m_idx];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  monster_lore *l_ptr = &l_list[m_ptr->r_idx];
  
  /* Note the number of things dropped */
  if (num_item > l_ptr->drop_item) l_ptr->drop_item = num_item;
  if (num_gold > l_ptr->drop_gold) l_ptr->drop_gold = num_gold;
  
  /* Hack -- memorize the good/great/chest flags */
  if (r_ptr->flags1 & (RF1_DROP_GOOD)) l_ptr->flags1 |= (RF1_DROP_GOOD);
  if (r_ptr->flags1 & (RF1_DROP_GREAT)) l_ptr->flags1 |= (RF1_DROP_GREAT);
  if (r_ptr->flags1 & (RF1_DROP_CHEST)) l_ptr->flags1 |= (RF1_DROP_CHEST);
  
  /* Update monster recall window */
  if (p_ptr->monster_race_idx == m_ptr->r_idx)
    {
      /* Window stuff */
      p_ptr->window |= (PW_MONSTER);
    }
}



/**
 * This function updates the monster record of the given monster
 *
 * This involves extracting the distance to the player (if requested),
 * and then checking for visibility (natural, infravision, see-invis,
 * telepathy), updating the monster visibility flag, redrawing (or
 * erasing) the monster when its visibility changes, and taking note
 * of any interesting monster flags (cold-blooded, invisible, etc).
 *
 * Note the new "mflag" field which encodes several monster state flags,
 * including "view" for when the monster is currently in line of sight,
 * and "mark" for when the monster is currently visible via detection.
 *
 * The only monster fields that are changed here are "cdis" (the
 * distance from the player), "ml" (visible to the player), and
 * "mflag" (to maintain the MFLAG_VIEW flag).
 *
 * Note the special update_monsters() function which can be used to
 * call this function once for every monster.
 *
 * Note the "full" flag which requests that the "cdis" field be updated,
 * this is only needed when the monster (or the player) has moved.
 *
 * Every time a monster moves, we must call this function for that
 * monster, and update the distance, and the visibility.  Every time
 * the player moves, we must call this function for every monster, and
 * update the distance, and the visibility.  Whenever the player "state"
 * changes in certain ways ("blindness", "infravision", "telepathy",
 * and "see invisible"), we must call this function for every monster,
 * and update the visibility.
 *
 * Routines that change the "illumination" of a grid must also call this
 * function for any monster in that grid, since the "visibility" of some
 * monsters may be based on the illumination of their grid.
 *
 * Note that this function is called once per monster every time the
 * player moves.  When the player is running, this function is one
 * of the primary bottlenecks, along with update_view() and the
 * process_monsters() code, so efficiency is important.
 *
 * Note the optimized "inline" version of the distance() function.
 *
 * A monster is "visible" to the player if (1) it has been detected
 * by the player, (2) it is close to the player and the player has
 * telepathy, or (3) it is close to the player, and in line of sight
 * of the player, and it is "illuminated" by some combination of
 * infravision, torch light, or permanent light (invisible monsters
 * are only affected by "light" if the player can see invisible).
 *
 * Monsters which are not on the current panel may be "visible" to
 * the player, and their descriptions will include an "offscreen"
 * reference.  Currently, offscreen monsters cannot be targetted
 * or viewed directly, but old targets will remain set.   XXX XXX
 *
 * The player can choose to be disturbed by several things, including
 * disturb_move (monster which is viewable moves in some way), and
 * disturb_near (monster which is "easily" viewable moves in some
 * way).  Note that "moves" includes "appears" and "disappears".
 */
void update_mon(int m_idx, bool full)
{
  monster_type *m_ptr = &m_list[m_idx];
  
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  monster_lore *l_ptr = &l_list[m_ptr->r_idx];
  
  int d;
  
  /* Current location */
  int fy = m_ptr->fy;
  int fx = m_ptr->fx;
  
  /* Seen at all */
  bool flag = FALSE;
  
  /* Seen by vision */
  bool easy = FALSE;
  
  
  /* Compute distance */
  if (full)
    {
      int py = p_ptr->py;
      int px = p_ptr->px;
      
      /* Distance components */
      int dy = (py > fy) ? (py - fy) : (fy - py);
      int dx = (px > fx) ? (px - fx) : (fx - px);
      
      /* Approximate distance */
      d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));
      
      /* Restrict distance */
      if (d > 255) d = 255;
      
      /* Save the distance */
      m_ptr->cdis = d;
    }
  
  /* Extract distance */
  else
    {
      /* Extract the distance */
      d = m_ptr->cdis;
    }
  
  
  /* Detected */
  if (m_ptr->mflag & (MFLAG_MARK)) flag = TRUE;
  
  
  /* Nearby */
  if (d <= (p_ptr->themed_level ? MAX_SIGHT / 2 : MAX_SIGHT))
    {
      /* Basic telepathy */
      if (p_ptr->telepathy || p_ptr->tim_esp)
	{
	  /* Empty mind, no telepathy */
	  if (r_ptr->flags2 & (RF2_EMPTY_MIND))
	    {
	      /* Memorize flags */
	      l_ptr->flags2 |= (RF2_EMPTY_MIND);
	    }
	  
	  /* Weird mind, occasional telepathy */
	  else if (r_ptr->flags2 & (RF2_WEIRD_MIND))
	    {
	      /* Monster is rarely detectable */
	      if (((turn / 10) % 10) == (m_idx % 10))
		{
		  /* Detectable */
		  notice_obj(OF_TELEPATHY, 0);
		  flag = TRUE;
		  
		  /* Memorize flags */
		  l_ptr->flags2 |= (RF2_WEIRD_MIND);
		  
		  /* Hack -- Memorize mental flags */
		  if (r_ptr->flags2 & (RF2_SMART)) 
		    l_ptr->flags2 |= (RF2_SMART);
		  if (r_ptr->flags2 & (RF2_STUPID)) 
		    l_ptr->flags2 |= (RF2_STUPID);
		}
	    }
	  
	  /* Normal mind, allow telepathy */
	  else
	    {
	      /* Detectable */
	      notice_obj(OF_TELEPATHY, 0);
	      flag = TRUE;
	      
	      /* Hack -- Memorize mental flags */
	      if (r_ptr->flags2 & (RF2_SMART)) l_ptr->flags2 |= (RF2_SMART);
	      if (r_ptr->flags2 & (RF2_STUPID)) l_ptr->flags2 |= (RF2_STUPID);
	    }
	}
      
      /* Normal line of sight, and not blind */
      if (player_has_los_bold(fy, fx) && !p_ptr->blind)
	{
	  bool do_invisible = FALSE;
	  bool do_cold_blood = FALSE;
	  
	  /* Use "infravision" */
	  if (d <= p_ptr->see_infra)
	    {
	      /* Handle "cold blooded" monsters */
	      if (r_ptr->flags2 & (RF2_COLD_BLOOD))
		{
		  /* Take note */
		  do_cold_blood = TRUE;
		}
	      
	      /* Handle "warm blooded" monsters */
	      else
		{
		  /* Easy to see */
		  easy = flag = TRUE;
		}
	    }
	  
	  /* Use "illumination" */
	  if (player_can_see_bold(fy, fx))
	    {
	      /* Handle "invisible" monsters */
	      if (r_ptr->flags2 & (RF2_INVISIBLE))
		{
		  /* Take note */
		  do_invisible = TRUE;
		  
		  /* See invisible */
		  if (p_ptr->see_inv)
		    {
		      /* Easy to see */
		      notice_obj(OF_SEE_INVIS, 0);
		      easy = flag = TRUE;
		    }
		}
	      
	      /* Handle "normal" monsters */
	      else
		{
		  /* Easy to see */
		  easy = flag = TRUE;
		}
	    }
	  
	  /* Visible */
	  if (flag)
	    {
	      /* Memorize flags */
	      if (do_invisible) l_ptr->flags2 |= (RF2_INVISIBLE);
	      if (do_cold_blood) l_ptr->flags2 |= (RF2_COLD_BLOOD);
	    }
	}
    }

  
  /* The monster is now visible */
  if (flag)
    {
      /* It was previously unseen */
      if (!m_ptr->ml)
	{
	  /* Mark as visible */
	  m_ptr->ml = TRUE;
	  
	  /* Draw the monster */
	  lite_spot(fy, fx);
	  
	  /* Update health bar as needed */
	  if (p_ptr->health_who == m_idx) 
	    p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);
	  
	  /* Hack -- Count "fresh" sightings */
	  if (l_ptr->sights < MAX_SHORT) l_ptr->sights++;
	  
	  /* Disturb on appearance */
	  if (disturb_move && (m_ptr->hostile == -1)) disturb(1, 0);
	  
	  /* Window stuff */
	  p_ptr->window |= PW_MONLIST;
	  
	}
    }
  
  /* The monster is not visible */
  else
    {
      /* It was previously seen */
      if (m_ptr->ml)
	{
	  /* Mark as not visible */
	  m_ptr->ml = FALSE;
	  
	  /* Erase the monster */
	  lite_spot(fy, fx);
	  
	  /* Update health bar as needed */
	  if (p_ptr->health_who == m_idx) 
	    p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);
	  
	  /* Disturb on disappearance */
	  if (disturb_move && (m_ptr->hostile == -1)) disturb(1, 0);
	  
	  /* Window stuff */
	  p_ptr->window |= PW_MONLIST;
	  
	}
    }
  
  
  /* The monster is now easily visible */
  if (easy)
    {
      /* Change */
      if (!(m_ptr->mflag & (MFLAG_VIEW)))
	{
	  /* Mark as easily visible */
	  m_ptr->mflag |= (MFLAG_VIEW);
	  
	  /* Disturb on appearance */
	  if (disturb_near && (m_ptr->hostile == -1)) disturb(1, 0);
	}
    }
  
  /* The monster is not easily visible */
  else
    {
      /* Change */
      if (m_ptr->mflag & (MFLAG_VIEW))
	{
	  /* Mark as not easily visible */
	  m_ptr->mflag &= ~(MFLAG_VIEW);
	  
	  /* Disturb on disappearance */
	  if (disturb_near && (m_ptr->hostile == -1)) disturb(1, 0);
	}
    }
}




/**
 * This function simply updates all the (non-dead) monsters (see above).
 */
void update_monsters(bool full)
{
  int i;
  
  /* Update each (live) monster */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      
      /* Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Update the monster */
      update_mon(i, full);
    }
}




/**
 * Make a monster carry an object
 */
s16b monster_carry(int m_idx, object_type *j_ptr)
{
  s16b o_idx;
  
  s16b this_o_idx, next_o_idx = 0;
  
  monster_type *m_ptr = &m_list[m_idx];
  
  /* Scan objects already being held for combination */
  for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
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
    }
  
  
  /* Make an object */
  o_idx = o_pop();
  
  /* Success */
  if (o_idx)
    {
      object_type *o_ptr;
      
      /* Get new object */
      o_ptr = &o_list[o_idx];
      
      /* Copy object */
      object_copy(o_ptr, j_ptr);
      
      /* Forget mark */
      o_ptr->marked = FALSE;
      
      /* Forget location */
      o_ptr->iy = o_ptr->ix = 0;
      
      /* Memorize monster */
      o_ptr->held_m_idx = m_idx;
      
      /* Build stack */
      o_ptr->next_o_idx = m_ptr->hold_o_idx;
      
      /* Build stack */
      m_ptr->hold_o_idx = o_idx;
    }
  
  /* Result */
  return (o_idx);
}

/**
 * See whether all surrounding squares are trap detected 
 */
bool is_detected(int y, int x)
{
  int d, xx, yy;
  
  /* Check around (and under) the character */
  for (d = 0; d < 9; d++)
    {
      /* Extract adjacent (legal) location */
      yy = y + ddy_ddd[d];
      xx = x + ddx_ddd[d];
      
      /* Paranoia */
      if (!in_bounds_fully(yy, xx)) continue;
      
      /* Only check trappable grids */
      if (!cave_floor_bold(yy, xx)) continue;
      
      /* Return false if undetected */
      if ((cave_info2[yy][xx] & (CAVE2_DTRAP)) == 0) return (FALSE);
    }
  
  /* Must be OK */
  return (TRUE);
}




/**
 * Swap the players/monsters (if any) at two locations XXX XXX XXX
 */
void monster_swap(int y1, int x1, int y2, int x2)
{
  int m1, m2;
  bool player_moved = FALSE;
  
  monster_type *m_ptr;
  
  
  /* Monsters */
  m1 = cave_m_idx[y1][x1];
  m2 = cave_m_idx[y2][x2];
  
  
  /* Update grids */
  cave_m_idx[y1][x1] = m2;
  cave_m_idx[y2][x2] = m1;
  
  
  /* Monster 1 */
  if (m1 > 0)
    {
      m_ptr = &m_list[m1];
      
      /* Move monster */
      m_ptr->fy = y2;
      m_ptr->fx = x2;
      
      /* Update monster */
      update_mon(m1, TRUE);
    }
  
  /* Player 1 */
  else if (m1 < 0)
    {
      /* Move player */
      p_ptr->py = y2;
      p_ptr->px = x2;
      player_moved = TRUE;
    }
  
  /* Monster 2 */
  if (m2 > 0)
    {
      m_ptr = &m_list[m2];
      
      /* Move monster */
      m_ptr->fy = y1;
      m_ptr->fx = x1;
      
      /* Update monster */
      update_mon(m2, TRUE);
    }
  
  /* Player 2 */
  else if (m2 < 0)
    {
      /* Move player */
      p_ptr->py = y1;
      p_ptr->px = x1;
      player_moved = TRUE;
    }
  
  /* Did the player move? */
  if (player_moved)
    {
      bool old_dtrap, new_dtrap;
      
      /* Calculate changes in dtrap status */
      old_dtrap = cave_info2[y1][x1] & (CAVE2_DTRAP);
      new_dtrap = is_detected(y2, x2);
      
      /* Note the change in the detect status */
      p_ptr->redraw |= (PR_DTRAP);
      
      /* Update the panel */
      p_ptr->update |= (PU_PANEL);
      
      /* Update the visuals (and monster distances) */
      p_ptr->update |= (PU_UPDATE_VIEW | PU_DISTANCE);
      
      /* Window stuff */
      p_ptr->window |= (PW_OVERHEAD);
      
      /* Warn when leaving trap detected region */
      if (disturb_trap_detect && old_dtrap && !new_dtrap)
	{
	  /* Disturb to break runs */
	  msg_print("*Edge of trap detect region!*");
	  disturb(0, 0);
	}
    }
  
  /* Redraw */
  lite_spot(y1, x1);
  lite_spot(y2, x2);
}


/**
 * Place the player in the dungeon XXX XXX
 */
s16b player_place(int y, int x)
{
  /* Paranoia XXX XXX */
  if (cave_m_idx[y][x] != 0) return (0);
  
  
  /* Save player location */
  p_ptr->py = y;
  p_ptr->px = x;
  
  /* Mark cave grid */
  cave_m_idx[y][x] = -1;
  
  /* Success */
  return (-1);
}


/**
 * Place a copy of a monster in the dungeon XXX XXX
 */
s16b monster_place(int y, int x, monster_type *n_ptr)
{
  s16b m_idx;
  
  monster_type *m_ptr;
  monster_race *r_ptr;
  
  
  /* Paranoia XXX XXX */
  if (cave_m_idx[y][x] != 0) return (0);
  
  
  /* Get a new record */
  m_idx = m_pop();
  
  /* Oops */
  if (m_idx)
    {
      /* Make a new monster */
      cave_m_idx[y][x] = m_idx;
      
      /* Acquire new monster */
      m_ptr = &m_list[m_idx];
      
      /* Copy the monster XXX */
      (void)COPY(m_ptr, n_ptr, monster_type);
      
      /* Location */
      m_ptr->fy = y;
      m_ptr->fx = x;
      
      /* Update the monster */
      update_mon(m_idx, TRUE);
      
      /* Acquire new race */
      r_ptr = &r_info[m_ptr->r_idx];
      
      /* Hack -- Notice new multi-hued monsters */
      if (r_ptr->flags1 & (RF1_ATTR_MULTI)) shimmer_monsters = TRUE;
      
      /* Hack -- Count the number of "reproducers" */
      if (r_ptr->flags2 & (RF2_MULTIPLY)) num_repro++;
      
      /* Count racial occurances */
      r_ptr->cur_num++;
    }
  
  /* Result */
  return (m_idx);
}


/**
 * Group mode for making mono-racial groups 
 */
static bool group_mode = FALSE;

/**
 * Group race for making mono-racial groups 
 */
static byte group_race = NON_RACIAL;

/**
 * Current group leader 
 */
static u16b group_leader;

/**
 * Find an appropriate same race monster 
 */
static bool get_racial_monster(int r_idx)
{
  monster_race *r_ptr = &r_info[r_idx];

  if (!(r_ptr->flags3 & (RF3_RACIAL))) return (FALSE);

  return (TRUE);
}

/**
 * Attempt to place a monster of the given race at the given location.
 *
 * To give the player a sporting chance, any monster that appears in
 * line-of-sight and is extremely dangerous can be marked as
 * "FORCE_SLEEP", which will cause them to be placed with low energy,
 * which often (but not always) lets the player move before they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * XXX XXX XXX Use special "here" and "dead" flags for unique monsters,
 * remove old "cur_num" and "max_num" fields.
 *
 * XXX XXX XXX Actually, do something similar for artifacts, to simplify
 * the "preserve" mode, and to make the "what artifacts" flag more useful.
 *
 * This is the only function which may place a monster in the dungeon,
 * except for the savefile loading code.
 */
static bool place_monster_one(int y, int x, int r_idx, bool slp)
{
  int i, r1_idx;
  
  monster_race *r_ptr;
  
  monster_type *n_ptr;
  monster_type monster_type_body;
  
  cptr name;
  
  /* Save previous monster restriction value. */
  bool (*get_mon_num_hook_temp)(int r_idx) = get_mon_num_hook;
  
  /* Paranoia */
  if (!r_idx) return (FALSE);
  
  /* Paranoia */
  if (!in_bounds(y, x)) return (FALSE);
  
  /* Race */
  if (group_mode)
    { 
      /* Set the hook */
      get_mon_num_hook = get_racial_monster;

      /* Prepare allocation table */
      get_mon_num_prep();
      
      /* XXX - rebuild monster table */
      (void)get_mon_num(monster_level);

      /* Try to get a monster */
      r1_idx = get_mon_num(monster_level);

      /* Reset the hook */
      get_mon_num_hook = get_mon_num_hook_temp;

      /* Prepare allocation table */
      get_mon_num_prep();
      
      /* XXX - rebuild monster table */
      (void)get_mon_num(monster_level);

      /* Success? */
      if (r1_idx == 0) return (FALSE);
    }
  else r1_idx = r_idx;

  r_ptr = &r_info[r1_idx];
  
  /* No light hating monsters in daytime */
  if ((r_ptr->flags3 & (RF3_HURT_LITE)) && 	
      ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)) &&
      (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) &&
      (stage_map[p_ptr->stage][STAGE_TYPE] != VALLEY))
 
    return (FALSE);
  
  /* The monster must be able to exist in this grid */
  if (!cave_exist_mon(r_ptr, y, x, FALSE)) return (FALSE);
  
  /* Paranoia */
  if (!r_ptr->name) return (FALSE);
  
  /* Name */
  name = (r_name + r_ptr->name);
  
  /* Hack -- "unique" monsters must be "unique" */
  if ((r_ptr->flags1 & (RF1_UNIQUE)) && (r_ptr->cur_num >= r_ptr->max_num))
    {
      /* Cannot create */
      return (FALSE);
    }
  
  /* Hack -- only 1 player ghost at a time */
  if ((r_ptr->flags2 & (RF2_PLAYER_GHOST)) && bones_selector)
    {
      /* Cannot create */
      return (FALSE);
    }
  
  /* Depth monsters may NOT be created out of depth */
  if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && (p_ptr->depth < r_ptr->level))
    {
      /* Cannot create */
      return (FALSE);
    }
  
  /* Monsters only there to be shapechanged into can never be placed */
  if ((r_ptr->flags2 & (RF2_NO_PLACE)))
    {
      /* Cannot create */
      return (FALSE);
    }
  
  
  /* Get local monster */
  n_ptr = &monster_type_body;
  
  /* Clean out the monster */
  (void)WIPE(n_ptr, monster_type);
  
  
  /* Save the race */
  n_ptr->r_idx = r1_idx;
  
  
  /* 
   * If the monster is a player ghost, perform various manipulations 
   * on it, and forbid ghost creation if something goes wrong.
   */
  if (r_ptr->flags2 & (RF2_PLAYER_GHOST))
    {
      if (!prepare_ghost(r_idx, n_ptr, FALSE)) return (FALSE);
      
      name = format("%s, the %s", ghost_name, name);

      /* Hack - point to the special "ghost slot" */
      r_ptr = &r_info[PLAYER_GHOST_RACE];
      n_ptr->r_idx = PLAYER_GHOST_RACE;
    }
  
  /* Enforce sleeping if needed */
  if (slp && r_ptr->sleep)
    {
      int val = r_ptr->sleep;
      n_ptr->csleep = ((val * 2) + randint(val * 10));
    }
  else if (!((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)) &&
	   (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) && 
	   (p_ptr->depth) && (!character_dungeon))
    n_ptr->csleep += 20;
  
  /* Enforce no sleeping if needed */
  if ((stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAINTOP) &&
      (cave_feat[y][x] == FEAT_VOID))
    n_ptr->csleep = 0;
  
  /* Assign maximal hitpoints */
  if (r_ptr->flags1 & (RF1_FORCE_MAXHP))
    {
      n_ptr->maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
    }
  else
    {
      n_ptr->maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }
  
  
  /* Initialize distance at which monster likes to operate */
  
  /* Mark minimum range for recalculation */
  n_ptr->min_range = 0;
  
  /* Monsters like to use harassment spells at first */
  if (r_ptr->level > 20) n_ptr->harass = BASE_HARASS;
  
  /* Weaker monsters don't live long enough to spend forever harassing */
  else n_ptr->harass = LOW_HARASS;
  
  /* Initialize mana */
  n_ptr->mana = r_ptr->mana;
  
  /* And start out fully healthy */
  n_ptr->hp = n_ptr->maxhp;
  
  /* Extract the monster base speed */
  n_ptr->mspeed = r_ptr->speed;
  
  /* Hack -- small racial variety */
  if (!(r_ptr->flags1 & (RF1_UNIQUE)))
    {
      /* Allow some small variation per monster */
      i = extract_energy[r_ptr->speed] / 10;
      if (i) n_ptr->mspeed += rand_spread(0, i);
    }
  
  
  /* Force monster to wait for player */
  if (r_ptr->flags1 & (RF1_FORCE_SLEEP))
    {
      /* Give a random starting energy */
      n_ptr->energy = 0;
    }
  else
    {
      /* Give a random starting energy */
      n_ptr->energy = rand_int(50);
    }

  /* Set the group leader, if there is one */
  n_ptr->group_leader = group_leader;
  
  /* Initialize racial monster */
  if (r_ptr->flags3 & (RF3_RACIAL))
    {
      int chance, k;

      /* Race is already chosen for group mode */
      if (group_mode) 
	{
	  n_ptr->p_race = group_race;
	  n_ptr->group = group_id;
	}

      /* Choose a race */
      else
	{
	  k = rand_int(race_prob[p_ptr->stage][z_info->p_max - 1]);
	  
	  for (i = 0; i < z_info->p_max; i++)
	    if (race_prob[p_ptr->stage][i] > k) 
	      {
		n_ptr->p_race = i;
		break;
	      }

	  /* Hack for Estolad themed level - Edain or Druedain */
	  if (p_ptr->themed_level == THEME_ESTOLAD)
	    n_ptr->p_race = (rand_int(100) < 50 ? 6 : 8);

	  /* Set group ID */
	  n_ptr->group = ++group_id;

	  /* Go into group mode if necessary */
	  if (r_ptr->flags1 & (RF1_FRIEND | RF1_FRIENDS))
	    {
	      group_mode = TRUE;
	      group_race = n_ptr->p_race;
	    }
	}
	  
      /* Set old race */
      n_ptr->old_p_race = NON_RACIAL;

      /* Set hostility */
      chance = g_info[(n_ptr->p_race * z_info->p_max) + p_ptr->prace] - 100;
      if (chance < 0) chance = 0;
      k = rand_int(chance + 20);
      if ((k > 20)  || (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) ||
	  (p_ptr->themed_level == THEME_WARLORDS) || 
	  (cave_info[y][x] & CAVE_ICKY)) 
	n_ptr->hostile = -1;
      else n_ptr->hostile = 0;
    }

  else 
    {
      n_ptr->p_race = NON_RACIAL;
      n_ptr->old_p_race = NON_RACIAL;
      n_ptr->hostile = -1;
      n_ptr->group = ++group_id;
    }

  /* Mark territorial monster's home */
  if (r_ptr->flags3 & (RF3_TERRITORIAL))
    {
      n_ptr->y_terr = y;
      n_ptr->x_terr = x;
    }

  /* Place the monster in the dungeon */
  if (!monster_place(y, x, n_ptr)) return (FALSE);
  
    /* Deep unique monsters */
    if ((r_ptr->flags1 & (RF1_UNIQUE)) &&
        (r_ptr->level > p_ptr->depth))
    {
        /* Message */
        if (cheat_hear) msg_format("Deep Unique (%s).", name);

        /* Boost rating by twice delta-depth */
        rating += (r_ptr->level - p_ptr->depth) * 2;
    }

    /* Note any unique monster, even if not out of depth */
    else if (r_ptr->flags1 & (RF1_UNIQUE))
    {
        /* Message */
        if (cheat_hear) msg_format("Unique (%s).", name);
    }

    /* Deep normal monsters */
    else if (r_ptr->level > p_ptr->depth + 4)
    {
        /* Message */
        if (cheat_hear) msg_format("Deep Monster (%s).", name);

        /* Boost rating by a function of delta-depth */
        rating += ((r_ptr->level - p_ptr->depth) * (r_ptr->level - p_ptr->depth)) / 25;
    }

  
  /* Success */
  return (TRUE);
}


/**
 * Maximum size of a group of monsters
 */
#define GROUP_MAX	32


/**
 * Attempt to place a group of monsters around the given location.
 *
 * Hack -- A group of monsters counts as a single individual for the 
 * level rating.
 */
static bool place_monster_group(int y, int x, int r_idx, bool slp, 
				s16b group_size)
{
  monster_race *r_ptr = &r_info[r_idx];
  
  int old, n, i;
  int reduce;
  
  int hack_n = 0;
  
  byte hack_y[GROUP_MAX];
  byte hack_x[GROUP_MAX];
  
  /* Hard monsters, smaller groups */
  if (r_ptr->level > p_ptr->depth)
    {
      reduce = (r_ptr->level - p_ptr->depth) / 2;
      group_size -= randint(reduce);
    }
  
  /* Easy monsters, slightly smaller groups  -BR- */
  else if (r_ptr->level < p_ptr->depth)
    {
      reduce = (p_ptr->depth - r_ptr->level) / 6;
      group_size -= randint(reduce);
    }
  
  /* Minimum size */
  if (group_size < 1) group_size = 1;

  /* Maximum size */
  if (group_size > GROUP_MAX) group_size = GROUP_MAX;
  
  
  /* Save the rating */
  old = rating;
  
  /* Start on the monster */
  hack_n = 1;
  hack_x[0] = x;
  hack_y[0] = y;
  
  /* Puddle monsters, breadth first, up to group_size */
  for (n = 0; (n < hack_n) && (hack_n < group_size); n++)
    {
      /* Grab the location */
      int hx = hack_x[n];
      int hy = hack_y[n];
      
      /* Check each direction, up to group_size */
      for (i = 0; (i < 8) && (hack_n < group_size); i++)
	{
	  int mx = hx + ddx_ddd[i];
	  int my = hy + ddy_ddd[i];
	  
	  /* Walls and monsters block flow */
	  if (!cave_empty_bold(my, mx)) continue;
	  
	  /* Attempt to place another monster */
	  if (place_monster_one(my, mx, r_idx, slp))
	    {
	      /* Add it to the "hack" set */
	      hack_y[hack_n] = my;
	      hack_x[hack_n] = mx;
	      hack_n++;
	    }
	}
    }
  
  /* Cancel group mode */
  group_mode = FALSE;
  group_race = NON_RACIAL;
  
  /* Hack -- restore the rating */
  rating = old;
  
  
  /* Success */
  return (TRUE);
}

/**
 * Hack -- help pick an escort type
 */
static int place_monster_idx = 0;

/**
 * Hack -- help pick an escort type
 */
static bool place_monster_okay(int r_idx)
{
  monster_race *r_ptr = &r_info[place_monster_idx];
  
  monster_race *z_ptr = &r_info[r_idx];
  
  /* Require similar "race" */
  if (z_ptr->d_char != r_ptr->d_char) return (FALSE);
  
  /* Skip more advanced monsters */
  if (z_ptr->level > r_ptr->level) return (FALSE);
  
  /* Skip unique monsters */
  if (z_ptr->flags1 & (RF1_UNIQUE)) return (FALSE);
  
  /* Paranoia -- Skip identical monsters */
  if (place_monster_idx == r_idx) return (FALSE);
  
  /* Okay */
  return (TRUE);
}



/**
 * Attempt to place an escort of monsters around the given location
 */
static void place_monster_escort(int y, int x, int leader_idx, bool slp)
{
  int escort_size, escort_idx;
  int n, i;
  
  monster_race *r_ptr = &r_info[leader_idx];
  
  int hack_n = 0;
  
  byte hack_y[GROUP_MAX];
  byte hack_x[GROUP_MAX];
  
  /* Save previous monster restriction value. */
  bool (*get_mon_num_hook_temp)(int r_idx) = get_mon_num_hook;
  
  
  /* Calculate the number of escorts we want. */
  if (r_ptr->flags1 & (RF1_ESCORTS)) escort_size = 6 + rand_int(15);
  else escort_size = 2 + rand_int(7);
  
  
  /* Use the leader's monster type to restrict the escorts. */
  place_monster_idx = leader_idx;
  
  /* Set the escort hook */
  get_mon_num_hook = place_monster_okay;
  
  /* Prepare allocation table */
  get_mon_num_prep();
  
  /* Build monster table */
  (void)get_mon_num(monster_level);
  
  /* Start on the monster */
  hack_n = 1;
  hack_x[0] = x;
  hack_y[0] = y;
  
  /* Puddle monsters, breadth first, up to escort_size */
  for (n = 0; (n < hack_n) && (hack_n < escort_size); n++)
    {
      /* Grab the location */
      int hx = hack_x[n];
      int hy = hack_y[n];
      
      /* Check each direction, up to group_size */
      for (i = 0; (i < 8) && (hack_n < escort_size); i++)
	{
	  int mx = hx + ddx_ddd[i];
	  int my = hy + ddy_ddd[i];
	  
	  /* Walls block flow */
	  if (!cave_project(my, mx)) continue;
	  
	  /* Flow past occupied grids. */
	  if (cave_m_idx[my][mx] != 0)
	    {
	      /* Add grid to the "hack" set */
	      hack_y[hack_n] = my;
	      hack_x[hack_n] = mx;
	      hack_n++;
	      
	      /* Hack -- Count as "free" grid. */
	      if (escort_size < GROUP_MAX) escort_size++;
	    }
	  
	  if (!cave_empty_bold(my, mx)) continue;
	  
	  /* Get an appropriate monster (quickly). */
	  escort_idx = get_mon_num_quick(r_ptr->level);
	  
	  /* Attempt to place another monster */
	  if (place_monster_one(my, mx, escort_idx, slp))
	    {
	      /* Add grid to the "hack" set */
	      hack_y[hack_n] = my;
	      hack_x[hack_n] = mx;
	      hack_n++;
	    }
	  else continue;
	  
	  /* Place a group of escorts if needed */
	  if (r_info[escort_idx].flags1 & (RF1_FRIENDS))
	    {
	      /* Place a group of monsters */
	      (void)place_monster_group(my, mx, escort_idx, slp, 
					(s16b)(5 + randint(10)));
	    }
	  else if (r_info[escort_idx].flags1 & (RF1_FRIEND))
	    {
	      /* Place a group of monsters */
	      (void)place_monster_group(my, mx, escort_idx, slp, 
					(s16b)(1 + randint(2)));
	    }
	}
    }
  
  /* Return to previous monster restrictions (usually none) */
  get_mon_num_hook = get_mon_num_hook_temp;
  
  /* Prepare allocation table */
  get_mon_num_prep();
  
  /* XXX - rebuild monster table */
  (void)get_mon_num(monster_level);
}



/**
 * Attempt to place a monster of the given race at the given location
 *
 * Monsters may have some friends, or lots of friends.  They may also 
 * have a few escorts, or lots of escorts.
 *
 * Note the use of the new "monster allocation table" code to restrict
 * the "get_mon_num()" function to legal escort types.
 */
bool place_monster_aux(int y, int x, int r_idx, bool slp, bool grp)
{
  monster_race *r_ptr = &r_info[r_idx];
  
  /* Place one monster, or fail */
  if (!place_monster_one(y, x, r_idx, slp)) 
    {
      /* Hack - cancel group mode */
      group_mode = FALSE;
      group_race = NON_RACIAL;
      return (FALSE);
    }
  
  /* Require the "group" flag */
  if (!grp) 
    {
      /* Cancel group mode */
      group_mode = FALSE;

      return (TRUE);
    }

  /* The original monster is the group leader */
  group_leader = cave_m_idx[y][x];  
  
  /* Friends for certain monsters */
  if (r_ptr->flags1 & (RF1_FRIENDS))
    {
      /* Attempt to place a large group */
      (void)place_monster_group(y, x, r_idx, slp, (s16b)damroll(3, 7));
    }
  
  else if (r_ptr->flags1 & (RF1_FRIEND))
    {
      /* Attempt to place a small group */
      (void)place_monster_group(y, x, r_idx, slp, (s16b)randint(3));
    }
  
  /* Escorts for certain monsters */
  if ((r_ptr->flags1 & (RF1_ESCORT)) || (r_ptr->flags1 & (RF1_ESCORTS)))
    {
      place_monster_escort(y, x, r_idx, slp);
    }

  /* Cancel group leader */
  group_leader = 0;
  
  /* Cancel group mode */
  group_mode = FALSE;
  
  /* Success */
  return (TRUE);
}

/**
 * Hack -- attempt to place a monster at the given location
 *
 * Attempt to find a monster appropriate to the "monster_level"
 */
bool place_monster(int y, int x, bool slp, bool grp, bool quick)
{
  int r_idx;
  
  /* Pick a monster - regular method */
  if (!quick) r_idx = get_mon_num(monster_level);
  
  /* Pick a monster - quick method */
  else r_idx = get_mon_num_quick(monster_level);
  
  /* Handle failure */
  if (!r_idx) return (FALSE);
  
  /* Attempt to place the monster */
  if (place_monster_aux(y, x, r_idx, slp, grp)) return (TRUE);
  
  /* Oops */
  return (FALSE);
}


/**
 * Attempt to allocate a random monster in the dungeon.
 *
 * Place the monster at least "dis" distance from the player.
 *
 * Use "slp" to choose the initial "sleep" status
 *
 * Use "monster_level" for the monster level
 *
 * Use "quick" to either rebuild the monster generation table, or 
 * just draw another monster from it.
 */
bool alloc_monster(int dis, bool slp, bool quick)
{
  monster_race *r_ptr;
  
  int r_idx;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x;
  
  /* Pick a monster - regular method */
  if (!quick) r_idx = get_mon_num(monster_level);
  
  /* Pick a monster - quick method */
  else r_idx = get_mon_num_quick(monster_level);
  
  /* Handle failure */
  if (!r_idx) return (FALSE);
  
  /* Get the monster */
  r_ptr = &r_info[r_idx];
  
  /* Find a legal, distant, unoccupied, space */
  while (TRUE)
    {
      /* Pick a location */
      y = rand_int(DUNGEON_HGT);
      x = rand_int(DUNGEON_WID);

      /* Require a grid that the monster can exist in. */
      if (!cave_exist_mon(r_ptr, y, x, FALSE)) continue;

      /* Monsters flying only on mountaintop */
      if ((cave_feat[y][x] == FEAT_VOID) && 
	  (stage_map[p_ptr->stage][STAGE_TYPE] != MOUNTAINTOP)) 
	continue;
      
      /* Do not put random monsters in marked rooms. */
      if ((!character_dungeon) && (cave_info[y][x] & (CAVE_TEMP))) 
	continue;
      
      /* Accept far away grids */
      if ((dis == 0) || (distance(y, x, py, px) > dis)) break;
    }
  
  /* Attempt to place the monster, allow groups  */
  if (place_monster_aux(y, x, r_idx, slp, TRUE)) return (TRUE);
  
  /* Nope */
  return (FALSE);
}




/**
 * Hack -- the "type" of the current "summon specific"
 */
static int summon_specific_type = 0;


/**
 * Hack -- help decide if a monster race is "okay" to summon
 */
static bool summon_specific_okay(int r_idx)
{
  monster_race *r_ptr = &r_info[r_idx];
  
  bool okay = FALSE;
  int i, effect;
  
  
  /* Player ghosts cannot be summoned. */
  if (r_ptr->flags2 & (RF2_PLAYER_GHOST)) return (FALSE);
  
  /* Sauron's forms cannot be summoned. */
  if ((r_ptr->flags2 & RF2_GAURHOTH) && (r_ptr->flags1 & RF1_FORCE_DEPTH) &&
      (summon_specific_type != SUMMON_SAURON)) 
    return (FALSE);
  
  /* Hack -- no specific type specified */
  if (!summon_specific_type) return (TRUE);
  
  /* Check our requirements */
  switch (summon_specific_type)
    {
    case SUMMON_KIN:
      {
	okay = ((r_ptr->d_char == summon_kin_type) &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_SAURON:
      {
	okay =  ((r_ptr->flags2 & RF2_GAURHOTH) && 
		 (r_ptr->flags1 & RF1_FORCE_DEPTH));
	break;
      }
      
    case SUMMON_ANT:
      {
	okay = ((r_ptr->d_char == 'a') &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_SPIDER:
      {
	okay = ((r_ptr->d_char == 'S') && !(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_HOUND:
      {
	okay = (((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z')) &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_ANIMAL:
      {
	okay = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_DRAGON:
      {
	okay = ((r_ptr->flags3 & (RF3_DRAGON)) &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_HI_DRAGON:
      {
	okay = (r_ptr->d_char == 'D');
	break;
      }
      
    case SUMMON_DEMON:
      {
	okay = ((r_ptr->flags3 & (RF3_DEMON)) &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_HI_DEMON:
      {
	okay = (r_ptr->d_char == 'U');
	break;
      }
      
    case SUMMON_UNDEAD:
      {
	okay = ((r_ptr->flags3 & (RF3_UNDEAD)) &&
		!(r_ptr->flags1 & (RF1_UNIQUE)));
	break;
      }
      
    case SUMMON_HI_UNDEAD:
      {
	okay = ((r_ptr->d_char == 'L') ||
		(r_ptr->d_char == 'V') ||
		(r_ptr->d_char == 'W'));
	break;
      }
      
      
    case SUMMON_UNIQUE:
      {
	if ((r_ptr->flags1 & (RF1_UNIQUE)) != 0) okay = TRUE;
	break;
      }
      
    case SUMMON_ELEMENTAL:
      {
	okay = (r_ptr->d_char == 'E');
	break;
      }
      
    case SUMMON_VORTEX:
      {
	okay = (r_ptr->d_char == 'v');
	break;
      }
      
    case SUMMON_HYBRID:
      {
	okay = (r_ptr->d_char == 'H');
	break;
      }
      
    case SUMMON_BIRD:
      {
	okay = (r_ptr->d_char == 'B');
	break;
      }

    case SUMMON_GOLEM:
      {
	okay = ((r_ptr->d_char == 'g') && (!(r_ptr->flags3 & RF3_DRAGON)));
	break;
      }
      
    case SUMMON_THIEF:
      {
	/* Scan through all four blows */
	for (i = 0; i < 4; i++)
	  {
	    /* Extract infomation about the blow effect */
	    effect = r_ptr->blow[i].effect;
	    if (effect == RBE_EAT_GOLD) okay = TRUE;
	    if (effect == RBE_EAT_ITEM) okay = TRUE;
	  }
	break;
      }
      
    case SUMMON_SWAMP:
      {
	okay = ((r_ptr->d_char == 'i') ||
		(r_ptr->d_char == 'j') ||
		(r_ptr->d_char == 'm') ||
		(r_ptr->d_char == 'I') ||
		(r_ptr->d_char == 'F') ||
		(r_ptr->d_char == ','));
	break;
      }
    }
  
  /* Result */
  return (okay);
}


/**
 * Place a monster (of the specified "type") near the given
 * location.  Return TRUE if a monster was actually summoned.
 *
 * We will attempt to place the monster up to 10 times before giving up.
 *
 * Note: SUMMON_UNIQUE and SUMMON_WRAITH (XXX) will summon Uniques
 * Note: SUMMON_HI_UNDEAD and SUMMON_HI_DRAGON may summon Uniques
 * Note: None of the other summon codes will ever summon Uniques.
 *
 * This function has been changed.  We now take the "monster level"
 * of the summoning monster as a parameter, and use that, along with
 * the current dungeon level, to help determine the level of the
 * desired monster.  Note that this is an upper bound, and also
 * tends to "prefer" monsters of that level.  Currently, we use
 * the average of the dungeon and monster levels, and then add
 * five to allow slight increases in monster power.
 *
 * Note that we use the new "monster allocation table" creation code
 * to restrict the "get_mon_num()" function to the set of "legal"
 * monsters, making this function much faster and more reliable.
 *
 * Note that this function may not succeed, though this is very rare.
 */
bool summon_specific(int y1, int x1, bool scattered, int lev, int type)
{
  int i, j, x, y, d, r_idx;

  bool found = FALSE;
  
  /* Prepare to look at a greater distance if necessary */
  for (j = 0; j < 3; j++)
    {
      /* Look for a location */
      for (i = 0; i < (scattered ? 40 : 20); ++i)
	{
	  /* Pick a distance */
	  if (scattered) d = rand_int(6) + 1 + j;
	  else d = (i / 10) + 1 + j;
	  
	  /* Pick a location */
	  scatter(&y, &x, y1, x1, d, 0);
	  
	  /* Require passable terrain, with no other creature or player. */
	  if (!cave_passable_bold(y, x)) continue;
	  if (cave_m_idx[y][x] != 0) continue;
	  
	  /* Hack -- no summon on glyph of warding */
	  if (cave_feat[y][x] == FEAT_RUNE_PROTECT) continue;
	  
	  /* Okay */
	  found = TRUE;
	  break;
	}

      /* Break if already found */
      if (found) break;
    }

  
  /* Failure */
  if (!found) return (FALSE);
  
  
  /* Save the "summon" type */
  summon_specific_type = type;
  
  
  /* Require "okay" monsters */
  get_mon_num_hook = summon_specific_okay;
  
  /* Prepare allocation table */
  get_mon_num_prep();
  
  
  /* Pick a monster, using the level calculation */
  r_idx = get_mon_num((p_ptr->depth + lev) / 2 + 1 + 
		      (p_ptr->depth >= 20 ? 4 : p_ptr->depth / 5));
  
  
  /* Remove restriction */
  get_mon_num_hook = NULL;
  
  /* Prepare allocation table */
  get_mon_num_prep();
  
  
  /* Handle failure */
  if (!r_idx) return (FALSE);
  
  /* Attempt to place the monster (awake, allow groups) */
  if (!place_monster_aux(y, x, r_idx, FALSE, TRUE)) return (FALSE);
  
  /* Success */
  return (TRUE);
}


/**
 * Hack - seemed like the most efficient way to summon the quest monsters,
 * given that they are excluded from the monster allocation table for all
 * stages but their quest stage. -NRM-
 */

bool summon_questor(int y1, int x1)
{
  int i, x, y, d;
  
  
  /* Look for a location */
  for (i = 0; i < 20; ++i)
    {
      /* Pick a distance */
      d = (i / 10) + 1;
      
      /* Pick a location */
      scatter(&y, &x, y1, x1, d, 0);
      
      /* Require passable terrain, with no other creature or player. */
      if (!cave_passable_bold(y, x)) continue;
      if (cave_m_idx[y][x] != 0) continue;
      
      /* Hack -- no summon on glyph of warding */
      if (cave_feat[y][x] == FEAT_RUNE_PROTECT) continue;
      
      /* Okay */
      break;
    }
  
  /* Failure */
  if (i == 20) return (FALSE);
  
  /* Get quest monsters */
  for (i = 1; i < z_info->r_max; i++)
    {
      monster_race *r_ptr = &r_info[i];
      
      /* Ensure quest monsters */
      if ((r_ptr->flags1 & (RF1_QUESTOR)) && (r_ptr->cur_num < 1)
	  && (r_ptr->max_num))
	{
	  /* Place the questor */
	  place_monster_aux(y, x, i, FALSE, TRUE);
	  
	  /* Success */
	  return (TRUE);
	}
    }
  
  /* Failure - all dead or summoned */
  return (FALSE);
}

/**
 * Assess shapechange possibilities
 */
int assess_shapechange(int m_idx, monster_type *m_ptr)
{
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  monster_race *tmp_ptr = NULL;

  int i, j, type = 0, shape, rate, spd_diff, best_shape = 0, best_rate = 0; 

  /* Pick some possible monsters, using the level calculation */
  for (i = 0; i < 3; i++)
    {
      int avdam = 0, class = 0, temp;

      /* Non-summoners can shapeshift to anything */
      if (!r_ptr->flags7) 
	{
	  type = 0;
	  class = 1;
	}

      /* Sauron */
      if ((r_ptr->flags2 & RF2_GAURHOTH) && 
	  (r_ptr->flags1 & RF1_FORCE_DEPTH))
	{
	  type = SUMMON_SAURON;
	  class = 1;
	}
      
      /* Pick a type of summonable monster */
      while (class == 0)
	{
	  j = rand_int(32);
	  class = ((1L << j) & (r_ptr->flags7));
	  
	  /* Set the type */
	  switch(class)
	    {
	    case RF7_S_KIN:
	      {
		type = SUMMON_KIN;
		break;
	      }
	      
	    case RF7_S_MONSTER:
	    case RF7_S_MONSTERS:
	      {
		type = 0;
		break;
	      }
	      
	    case RF7_S_ANT:
	      {
		type = SUMMON_ANT;
		break;
	      }
	      
	    case RF7_S_SPIDER:
	      {
		type = SUMMON_SPIDER;
		break;
	      }
	      
	    case RF7_S_HOUND:
	      {
		type = SUMMON_HOUND;
		break;
	      }
	      
	    case RF7_S_ANIMAL:
	      {
		type = SUMMON_ANIMAL;
		break;
	      }
	      
	    case RF7_S_THIEF:
	      {
		type = SUMMON_THIEF;
		break;
	      }
	      
	    case RF7_S_SWAMP:
	      {
		type = SUMMON_SWAMP;
		break;
	      }
	      
	    case RF7_S_DRAGON:
	      {
		type = SUMMON_DRAGON;
		break;
	      }
	      
	    case RF7_S_HI_DRAGON:
	      {
		type = SUMMON_HI_DRAGON;
		break;
	      }
	      
	    case RF7_S_DEMON:
	      {
		type = SUMMON_DEMON;
		break;
	      }
	      
	    case RF7_S_HI_DEMON:
	      {
		type = SUMMON_HI_DEMON;
		break;
	      }
	      
	    case RF7_S_UNDEAD:
	      {
		type = SUMMON_UNDEAD;
		break;
	      }
	      
	    case RF7_S_HI_UNDEAD:
	      {
		type = SUMMON_HI_UNDEAD;
		break;
	      }
	      /* Default */
	      class = 0;
	    }
	}
      
      
      /* Save the "summon" type */
      summon_specific_type = type;
      
      /* Require "okay" monsters */
      get_mon_num_hook = summon_specific_okay;
      
      /* Prepare allocation table */
      get_mon_num_prep();
  
      /* Get a possible race */
      shape = get_mon_num((p_ptr->depth + r_ptr->level) / 2 + 1 + 
			  (p_ptr->depth >= 20 ? 4 : p_ptr->depth / 5));
      
      /* Temporarily change race */
      tmp_ptr = &r_info[shape];
      temp = m_ptr->orig_idx;
      m_ptr->orig_idx = m_ptr->r_idx;
      m_ptr->r_idx = shape;

      /* Get the best ranged attack for this shapechange */
      rate = choose_ranged_attack(m_idx, FALSE, -1);

      /* Take out a cost for time to change */
      rate = 2 * rate/3;

      /* Adjust for spell frequency */
      rate = tmp_ptr->freq_ranged * rate/r_ptr->freq_ranged;

      /* Get average melee damage */
      for (j = 0; j < 4; j++)
	avdam += tmp_ptr->blow[j].d_dice * (tmp_ptr->blow[j].d_side/2);

      /* Reduce for distance from player */
      avdam /= m_ptr->cdis;

      /* Add to the rate */
      rate += avdam;

      /* Get the speed difference */
      spd_diff = (tmp_ptr->speed - r_ptr->speed)/10;

      /* Factor in the speed */
      if (spd_diff >= 0) rate *= (spd_diff + 1);
      else rate /= - spd_diff;

      /* See if this is the best yet */
      if (rate > best_rate)
	{
	  best_rate = rate;
	  best_shape = shape;
	}

      /* Change back */
      m_ptr->r_idx = m_ptr->orig_idx;
      m_ptr->orig_idx = temp;
      
      /* Remove restriction */
      get_mon_num_hook = NULL;
      
      /* Prepare allocation table */
      get_mon_num_prep();

    }  

  if(p_ptr->wizard) 
    {
      msg_format("Shapechange rating: %i.", best_rate);
      msg_format("Best shape: %i.", best_shape);
    }
  
  /* Set the shape in case it's used */
  m_ptr->orig_idx = best_shape;
  
  /* Return the best rate */
  return(best_rate);  
}



/**
 * Let the given monster attempt to reproduce.
 *
 * Note that "reproduction" REQUIRES empty space.
 */
bool multiply_monster(int m_idx)
{
  monster_type *m_ptr = &m_list[m_idx];
  
  int i, y, x;
  
  bool result = FALSE;
  
  /* Try up to 18 times */
  for (i = 0; i < 18; i++)
    {
      int d = 1;
      
      /* Pick a location */
      scatter(&y, &x, m_ptr->fy, m_ptr->fx, d, 0);
      
      /* Require an "empty" floor grid */
      if (!cave_empty_bold(y, x)) continue;
      
      /* Create a new monster (awake, no groups) */
      result = place_monster_aux(y, x, m_ptr->r_idx, FALSE, FALSE);
      
      /* Done */
      break;
    }
  
  /* Result */
  return (result);
}





/**
 * Dump a message describing a monster's reaction to damage
 *
 * Technically should attempt to treat "Beholder"'s as jelly's
 */
void message_pain(int m_idx, int dam)
{
  long oldhp, newhp, tmp;
  int percentage;
  
  monster_type *m_ptr = &m_list[m_idx];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  char m_name[80];
  
  
  /* Get the monster name */
  monster_desc(m_name, m_ptr, 0);
  
  /* Notice non-damage */
  if (dam == 0)
    {
      msg_format("%^s is unharmed.", m_name);
      return;
    }
  
  /* Note -- subtle fix -CFT */
  newhp = (long)(m_ptr->hp);
  oldhp = newhp + (long)(dam);
  tmp = (newhp * 100L) / oldhp;
  percentage = (int)(tmp);
  
  
  /* Jelly's, Mold's, Vortex's, Quthl's */
  if (strchr("jmvQ", r_ptr->d_char))
    {
      if (percentage > 95)
	msg_format("%^s barely notices.", m_name);
      else if (percentage > 75)
	msg_format("%^s flinches.", m_name);
      else if (percentage > 50)
	msg_format("%^s squelches.", m_name);
      else if (percentage > 35)
	msg_format("%^s quivers in pain.", m_name);
      else if (percentage > 20)
	msg_format("%^s writhes about.", m_name);
      else if (percentage > 10)
	msg_format("%^s writhes in agony.", m_name);
      else
	msg_format("%^s jerks limply.", m_name);
    }

  /* Dogs and Hounds */
  else if (strchr("CZ", r_ptr->d_char))
    {
      if (percentage > 95)
	msg_format("%^s shrugs off the attack.", m_name);
      else if (percentage > 75)
	msg_format("%^s snarls with pain.", m_name);
      else if (percentage > 50)
	msg_format("%^s yelps in pain.", m_name);
      else if (percentage > 35)
	msg_format("%^s howls in pain.", m_name);
      else if (percentage > 20)
	msg_format("%^s howls in agony.", m_name);
      else if (percentage > 10)
	msg_format("%^s writhes in agony.", m_name);
      else
	msg_format("%^s yelps feebly.", m_name);
    }
  
  /* One type of monsters (ignore,squeal,shriek) */
  else if (strchr("FIKMRSXabclqrst", r_ptr->d_char))
    {
      if (percentage > 95)
	msg_format("%^s ignores the attack.", m_name);
      else if (percentage > 75)
	msg_format("%^s grunts with pain.", m_name);
      else if (percentage > 50)
	msg_format("%^s squeals in pain.", m_name);
      else if (percentage > 35)
	msg_format("%^s shrieks in pain.", m_name);
      else if (percentage > 20)
	msg_format("%^s shrieks in agony.", m_name);
      else if (percentage > 10)
	msg_format("%^s writhes in agony.", m_name);
      else
	msg_format("%^s cries out feebly.", m_name);
    }

  /* Another type of monsters (shrug,cry,scream) */
  else
    {
      if (percentage > 95)
	msg_format("%^s shrugs off the attack.", m_name);
      else if (percentage > 75)
	msg_format("%^s grunts with pain.", m_name);
      else if (percentage > 50)
	msg_format("%^s cries out in pain.", m_name);
      else if (percentage > 35)
	msg_format("%^s screams in pain.", m_name);
      else if (percentage > 20)
	msg_format("%^s screams in agony.", m_name);
      else if (percentage > 10)
	msg_format("%^s writhes in agony.", m_name);
      else
	msg_format("%^s cries out feebly.", m_name);
    }
}



/**
 * Monster learns about an "observed" resistance.
 *
 * The LRN_xxx const indicates the type of resistance to be
 * investigated.
 *
 * SM_xxx flags are set appropriately.
 *
 * -BR-
 */
void update_smart_learn(int m_idx, int what)
{
  monster_type *m_ptr = &m_list[m_idx];
  
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  /* Too stupid to learn anything */
  if (r_ptr->flags2 & (RF2_STUPID)) return;
  
  /* Not intelligent, only learn sometimes */
  if (!(r_ptr->flags2 & (RF2_SMART)) && (rand_int(100) < 50)) return;
  
  /* XXX XXX XXX */
  
  /* Analyze the knowledge */
  switch (what)
    {
      
      /* Slow/paralyze attacks learn about free action and saving throws */
    case LRN_FREE_SAVE:
      {
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
	if (p_ptr->free_act) m_ptr->smart |= (SM_IMM_FREE);
	else m_ptr->smart &= ~(SM_IMM_FREE);
	break;
      }
      
      /* Mana attacks learn if you have any mana to attack */
    case LRN_MANA:
      {
	if (!p_ptr->msp) m_ptr->smart |= (SM_IMM_MANA);
	else m_ptr->smart &= ~(SM_IMM_MANA);
	break;
      }
      
      /* Acid attacks learn about Acid resists and immunities */
    case LRN_ACID:
      {
	if (p_resist_good(P_RES_ACID)) m_ptr->smart |= (SM_RES_ACID);
	else m_ptr->smart &= ~(SM_RES_ACID);
	if (p_resist_strong(P_RES_ACID)) m_ptr->smart |= (SM_RES_STRONG_ACID);
	else m_ptr->smart &= ~(SM_RES_STRONG_ACID);
	if (p_immune(P_RES_ACID)) m_ptr->smart |= (SM_IMM_ACID);
	else m_ptr->smart &= ~(SM_IMM_ACID);
	break;
      }
      
      /* Electircal attacks learn about Electrical resists and immunities */
    case LRN_ELEC:
      {
	if (p_resist_good(P_RES_ELEC)) m_ptr->smart |= (SM_RES_ELEC);
	else m_ptr->smart &= ~(SM_RES_ELEC);
	if (p_resist_strong(P_RES_ELEC)) m_ptr->smart |= (SM_RES_STRONG_ELEC);
	else m_ptr->smart &= ~(SM_RES_STRONG_ELEC);
	if (p_immune(P_RES_ELEC)) m_ptr->smart |= (SM_IMM_ELEC);
	else m_ptr->smart &= ~(SM_IMM_ELEC);
	break;
      }
      
      /* Fire attacks learn about Fire resists and immunities */
    case LRN_FIRE:
      {
	if (p_resist_good(P_RES_FIRE)) m_ptr->smart |= (SM_RES_FIRE);
	else m_ptr->smart &= ~(SM_RES_FIRE);
	if (p_resist_strong(P_RES_FIRE)) m_ptr->smart |= (SM_RES_STRONG_FIRE);
	else m_ptr->smart &= ~(SM_RES_STRONG_FIRE);
	if (p_immune(P_RES_FIRE)) m_ptr->smart |= (SM_IMM_FIRE);
	else m_ptr->smart &= ~(SM_IMM_FIRE);
	break;
      }
      
      /* Cold attacks learn about Cold resists and immunities */
    case LRN_COLD:
      {
	if (p_resist_good(P_RES_COLD)) m_ptr->smart |= (SM_RES_COLD);
	else m_ptr->smart &= ~(SM_RES_COLD);
	if (p_resist_strong(P_RES_COLD)) m_ptr->smart |= (SM_RES_STRONG_COLD);
	else m_ptr->smart &= ~(SM_RES_STRONG_COLD);
	if (p_immune(P_RES_COLD)) m_ptr->smart |= (SM_IMM_COLD);
	else m_ptr->smart &= ~(SM_IMM_COLD);
	break;
      }
      
      /* Poison attacks learn about Poison resists */
    case LRN_POIS:
      {
	if (p_resist_good(P_RES_POIS)) m_ptr->smart |= (SM_RES_POIS);
	else m_ptr->smart &= ~(SM_RES_POIS);
	if (p_resist_strong(P_RES_POIS)) m_ptr->smart |= (SM_RES_STRONG_POIS);
	else m_ptr->smart &= ~(SM_RES_STRONG_POIS);
	break;
      }
      
      /* Fear attacks learn about resist fear and saving throws */
    case LRN_FEAR_SAVE:
      {
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
	if (p_ptr->no_fear) m_ptr->smart |= (SM_RES_FEAR);
	else m_ptr->smart &= ~(SM_RES_FEAR);
	break;
      }
      
      /* Light attacks learn about light and blindness resistance */
    case LRN_LITE:
      {
	if (p_resist_good(P_RES_LITE)) m_ptr->smart |= (SM_RES_LITE);
	else m_ptr->smart &= ~(SM_RES_LITE);
	break;
      }
      
      /* Darkness attacks learn about dark and blindness resistance */
    case LRN_DARK:
      {
	if (p_resist_good(P_RES_DARK)) m_ptr->smart |= (SM_RES_DARK);
	else m_ptr->smart &= ~(SM_RES_DARK);
	break;
      }
      
      /*
       * Some Blindness attacks learn about blindness resistance 
       * Others (below) do more
       */
    case LRN_BLIND:
      {
	if (p_ptr->no_blind) m_ptr->smart |= (SM_RES_BLIND);
	else m_ptr->smart &= ~(SM_RES_BLIND);
	break;
      }
      
      /* 
       * Some Confusion attacks learn about confusion resistance
       * Others (below) do more
       */
    case LRN_CONFU:
      {
	if (p_resist_good(P_RES_CONFU)) m_ptr->smart |= (SM_RES_CONFU);
	else m_ptr->smart &= ~(SM_RES_CONFU);
	break;
      }
      
      /* 
       * Some sound attacks learn about sound and confusion resistance, 
       * and saving throws 
       * Others (below) do less.
       */
    case LRN_SOUND:
      {
	if (p_resist_good(P_RES_SOUND)) m_ptr->smart |= (SM_RES_SOUND);
	else m_ptr->smart &= ~(SM_RES_SOUND);
	if (p_resist_good(P_RES_CONFU)) m_ptr->smart |= (SM_RES_CONFU);
	else m_ptr->smart &= ~(SM_RES_CONFU);
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
	break;
      }
      
      /* Shards attacks learn about shards resistance */
    case LRN_SHARD:
      {
	if (p_resist_good(P_RES_SHARD)) m_ptr->smart |= (SM_RES_SHARD);
	else m_ptr->smart &= ~(SM_RES_SHARD);
	break;
      }
      
      /*
       *  Some Nexus attacks learn about Nexus resistance only
       *  Others (below) do more
       */
    case LRN_NEXUS:
      {
	if (p_resist_good(P_RES_NEXUS)) m_ptr->smart |= (SM_RES_NEXUS);
	else m_ptr->smart &= ~(SM_RES_NEXUS);
	break;
      }
      
      /* Nether attacks learn about Nether resistance */
    case LRN_NETHR:
      {
	if (p_resist_good(P_RES_NETHR)) m_ptr->smart |= (SM_RES_NETHR);
	else m_ptr->smart &= ~(SM_RES_NETHR);
	break;
      }
      
      /* Chaos attacks learn about Chaos resistance */
    case LRN_CHAOS:
      {
	if (p_resist_good(P_RES_CHAOS)) m_ptr->smart |= (SM_RES_CHAOS);
	else m_ptr->smart &= ~(SM_RES_CHAOS);
	break;
      }
      
      /* Disenchantment attacks learn about disenchantment resistance */
    case LRN_DISEN:
      {
	if (p_resist_good(P_RES_DISEN)) m_ptr->smart |= (SM_RES_DISEN);
	else m_ptr->smart &= ~(SM_RES_DISEN);
	break;
      }
      
      /* Some attacks learn only about saving throws (cause wounds, etc) */
    case LRN_SAVE:
      {
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
      }
      
      /* Archery attacks don't learn anything */
    case LRN_ARCH:
      {
	break;
      }
      
      /* Poison archery attacks learn about poison resists */
    case LRN_PARCH:
      {
	if (p_resist_good(P_RES_POIS)) m_ptr->smart |= (SM_RES_POIS);
	else m_ptr->smart &= ~(SM_RES_POIS);
	if (p_resist_strong(P_RES_POIS)) m_ptr->smart |= (SM_RES_STRONG_POIS);
	else m_ptr->smart &= ~(SM_RES_STRONG_POIS);
	break;
      }
      
      /* Ice attacks learn aboyt sound/shards/cold resists and cold immunity */
    case LRN_ICE:
      {
	if (p_resist_good(P_RES_COLD)) m_ptr->smart |= (SM_RES_COLD);
	else m_ptr->smart &= ~(SM_RES_COLD);
	if (p_resist_strong(P_RES_COLD)) m_ptr->smart |= (SM_RES_STRONG_COLD);
	else m_ptr->smart &= ~(SM_RES_STRONG_COLD);
	if (p_immune(P_RES_COLD)) m_ptr->smart |= (SM_IMM_COLD);
	else m_ptr->smart &= ~(SM_IMM_COLD);
	if (p_resist_good(P_RES_SOUND)) m_ptr->smart |= (SM_RES_SOUND);
	else m_ptr->smart &= ~(SM_RES_SOUND);
	if (p_resist_good(P_RES_SHARD)) m_ptr->smart |= (SM_RES_SHARD);
	else m_ptr->smart &= ~(SM_RES_SHARD);
	break;
      }
      
      /* Plasma attacks learn about fire/lightning resists/immunities */
    case LRN_PLAS:
      {
	if (p_resist_good(P_RES_ELEC)) m_ptr->smart |= (SM_RES_ELEC);
	else m_ptr->smart &= ~(SM_RES_ELEC);
	if (p_resist_strong(P_RES_ELEC)) m_ptr->smart |= (SM_RES_STRONG_ELEC);
	else m_ptr->smart &= ~(SM_RES_STRONG_ELEC);
	if (p_immune(P_RES_ELEC)) m_ptr->smart |= (SM_IMM_ELEC);
	else m_ptr->smart &= ~(SM_IMM_ELEC);
	if (p_resist_good(P_RES_FIRE)) m_ptr->smart |= (SM_RES_FIRE);
	else m_ptr->smart &= ~(SM_RES_FIRE);
	if (p_resist_strong(P_RES_FIRE)) m_ptr->smart |= (SM_RES_STRONG_FIRE);
	else m_ptr->smart &= ~(SM_RES_STRONG_FIRE);
	if (p_immune(P_RES_FIRE)) m_ptr->smart |= (SM_IMM_FIRE);
	else m_ptr->smart &= ~(SM_IMM_FIRE);
	break;
      }
      
      /* 
       * Some sounds attacks learna about sound resistance only
       * Others (above) do more
       */
    case LRN_SOUND2:
      {
	if (p_resist_good(P_RES_SOUND)) m_ptr->smart |= (SM_RES_SOUND);
	else m_ptr->smart &= ~(SM_RES_SOUND);
	break;
      }
      
      /* 
       * Storm attacks learn about Electrical/Cold/Acid resists/immunities,
       * and about confusion resist
       */
    case LRN_STORM:
      {
	if (p_resist_good(P_RES_ELEC)) m_ptr->smart |= (SM_RES_ELEC);
	else m_ptr->smart &= ~(SM_RES_ELEC);
	if (p_resist_strong(P_RES_ELEC)) m_ptr->smart |= (SM_RES_STRONG_ELEC);
	else m_ptr->smart &= ~(SM_RES_STRONG_ELEC);
	if (p_immune(P_RES_ELEC)) m_ptr->smart |= (SM_IMM_ELEC);
	else m_ptr->smart &= ~(SM_IMM_ELEC);
	if (p_resist_good(P_RES_COLD)) m_ptr->smart |= (SM_RES_COLD);
	else m_ptr->smart &= ~(SM_RES_COLD);
	if (p_resist_strong(P_RES_COLD)) m_ptr->smart |= (SM_RES_STRONG_COLD);
	else m_ptr->smart &= ~(SM_RES_STRONG_COLD);
	if (p_immune(P_RES_COLD)) m_ptr->smart |= (SM_IMM_COLD);
	else m_ptr->smart &= ~(SM_IMM_COLD);
	if (p_resist_good(P_RES_ACID)) m_ptr->smart |= (SM_RES_ACID);
	else m_ptr->smart &= ~(SM_RES_ACID);
	if (p_resist_strong(P_RES_ACID)) m_ptr->smart |= (SM_RES_STRONG_ACID);
	else m_ptr->smart &= ~(SM_RES_STRONG_ACID);
	if (p_immune(P_RES_ACID)) m_ptr->smart |= (SM_IMM_ACID);
	else m_ptr->smart &= ~(SM_IMM_ACID);
	if (p_resist_good(P_RES_CONFU)) m_ptr->smart |= (SM_RES_CONFU);
      }
      
      /* Water attacks learn about sound/confusion resists */
    case LRN_WATER:
      {
	if (p_resist_good(P_RES_SOUND)) m_ptr->smart |= (SM_RES_SOUND);
	else m_ptr->smart &= ~(SM_RES_SOUND);
	if (p_resist_good(P_RES_CONFU)) m_ptr->smart |= (SM_RES_CONFU);
	else m_ptr->smart &= ~(SM_RES_CONFU);
      }
      
      /* 
       * Some nexus attacks learn about Nexus resist and saving throws
       * Others (above) do more
       */
    case LRN_NEXUS_SAVE:
      {
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
	if (p_resist_good(P_RES_NEXUS)) m_ptr->smart |= (SM_RES_NEXUS);
	break;
      }
      
      /*
       * Some Blindness attacks learn about blindness resistance and 
       * saving throws
       * Others (above) do less
       */
    case LRN_BLIND_SAVE:
      {
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
	if (p_ptr->no_blind) m_ptr->smart |= (SM_RES_BLIND);
	break;
      }
      
      /* 
       * Some Confusion attacks learn about confusion resistance and 
       * saving throws
       * Others (above) do less
       */
    case LRN_CONFU_SAVE:
      {
	if (p_ptr->skill_sav >= 75) m_ptr->smart |= (SM_GOOD_SAVE);
	else m_ptr->smart &= ~(SM_GOOD_SAVE);
	if (p_ptr->skill_sav >= 100) m_ptr->smart |= (SM_PERF_SAVE);
	else m_ptr->smart &= ~(SM_PERF_SAVE);
	if (p_resist_good(P_RES_CONFU)) m_ptr->smart |= (SM_RES_CONFU);
	else m_ptr->smart &= ~(SM_RES_CONFU);
	break;
      }
      /* All element spells learn about all resistables*/
    case LRN_ALL:
      {
	/* Recurse */
	update_smart_learn(m_idx, LRN_ACID);
	update_smart_learn(m_idx, LRN_FIRE);
	update_smart_learn(m_idx, LRN_ELEC);
	update_smart_learn(m_idx, LRN_COLD);
	update_smart_learn(m_idx, LRN_POIS);
	update_smart_learn(m_idx, LRN_LITE);
	update_smart_learn(m_idx, LRN_DARK);
	update_smart_learn(m_idx, LRN_CONFU);
	update_smart_learn(m_idx, LRN_SOUND);
	update_smart_learn(m_idx, LRN_SHARD);
	update_smart_learn(m_idx, LRN_NEXUS);
	update_smart_learn(m_idx, LRN_NETHR);
	update_smart_learn(m_idx, LRN_CHAOS);
	update_smart_learn(m_idx, LRN_DISEN);
      }
      
    }
}

