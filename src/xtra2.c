/** \file xtra2.c 
    \brief Monster death, player attributes, targetting

 * Handlers for most of the player's temporary attributes, resistances,
 * nutrition, experience.  Monsters that drop a specific treasure, monster
 * death and subsequent events, screen scrolling, monster health descrip-
 * tions.  Sorting, targetting, what and how squares appear when looked at,
 * prompting for a direction to aim at and move in.
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
#include "cmds.h"
#include "squelch.h"

#ifdef _WIN32_WCE
#include "angbandcw.h"
#endif /* _WIN32_WCE */


/**
 * Set "p_ptr->word_recall", notice observable changes
 */
bool word_recall(int v)
{
  if (!p_ptr->word_recall)
    {
      return set_recall(v);
    }
  else
    {
      return set_recall(0);
    }
}


/**
 * Advance experience levels and print experience
 */
void check_experience(void)
{
  int i;
  
  
  /* Note current level */
  i = p_ptr->lev;
  
  
  /* Hack -- lower limit */
  if (p_ptr->exp < 0) p_ptr->exp = 0;
  
  /* Hack -- lower limit */
  if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;
  
  /* Hack -- upper limit */
  if (p_ptr->exp > PY_MAX_EXP) p_ptr->exp = PY_MAX_EXP;
  
  /* Hack -- upper limit */
  if (p_ptr->max_exp > PY_MAX_EXP) p_ptr->max_exp = PY_MAX_EXP;
  
  
  /* Hack -- maintain "max" experience */
  if (p_ptr->exp > p_ptr->max_exp) p_ptr->max_exp = p_ptr->exp;
  
  /* Redraw experience */
  p_ptr->redraw |= (PR_EXP);
  
  /* Handle stuff */
  handle_stuff();
  
  
  /* Lose levels while possible */
  while ((p_ptr->lev > 1) && (p_ptr->exp < (player_exp[p_ptr->lev-2])))
    {
      /* Lose a level */
      p_ptr->lev--;
      
      /* Update some stuff */
      p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);
      
      /* Redraw some stuff */
      p_ptr->redraw |= (PR_EXP | PR_LEV | PR_TITLE);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
      
      /* Handle stuff */
      handle_stuff();
    }
  
  
  /* Gain levels while possible */
  while ((p_ptr->lev < PY_MAX_LEVEL) && 
	 (p_ptr->exp >= (player_exp[p_ptr->lev-1])))
    {
      bool first_time = FALSE;

      /* Gain a level */
      p_ptr->lev++;
      
      /* Save the highest level */
      if (p_ptr->lev > p_ptr->max_lev) 
	{
	  p_ptr->max_lev = p_ptr->lev;
	  first_time = TRUE;
	}
      
      /* Sound */
      sound(MSG_LEVEL);
      
      /* Message */
      message_format(MSG_LEVEL, p_ptr->lev, "Welcome to level %d.", 
		     p_ptr->lev);
      
      /* Write a note to the file every 5th level. */
      
      if (((p_ptr->lev % 5) == 0) && first_time)
	
	{
	  
	  char buf[120];
	  
	  /* Build the message */
	  sprintf(buf, "Reached level %d", p_ptr->lev);
	  
	  /* Write message */
	  make_note(buf,  p_ptr->stage, NOTE_LEVEL, p_ptr->lev);
	  
	}
      
      /* Update some stuff */
      p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);
      
      /* Redraw some stuff */
      p_ptr->redraw |= (PR_EXP | PR_LEV | PR_TITLE);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
      
      /* Handle stuff */
      handle_stuff();
    }
  
  /* Gain max levels while possible 
   * Called rarely - only when leveling while experience is drained.  */
  while ((p_ptr->max_lev < PY_MAX_LEVEL) &&
	 (p_ptr->max_exp >= (player_exp[p_ptr->max_lev-1])))
    {
      /* Gain max level */
      p_ptr->max_lev++;
      
      /* Update some stuff */
      p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);
      
      /* Redraw some stuff */
      p_ptr->redraw |= (PR_LEV | PR_TITLE);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
      
      /* Handle stuff */
      handle_stuff();
    }
}


/**
 * Gain experience
 */
void gain_exp(s32b amount)
{
  /* Gain some experience */
  p_ptr->exp += amount;
  
  /* Slowly recover from experience drainage */
  if (p_ptr->exp < p_ptr->max_exp)
    {
      /* Gain max experience (10%) */
      p_ptr->max_exp += amount / 10;
    }
  
  /* Check Experience */
  check_experience();
}


/**
 * Lose experience
 */
void lose_exp(s32b amount)
{
  /* Never drop below zero experience */
  if (amount > p_ptr->exp) amount = p_ptr->exp;
  
  /* Lose some experience */
  p_ptr->exp -= amount;
  
  /* Check Experience */
  check_experience();
}




/**
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 *
 * Note the use of actual "monster names".  XXX XXX XXX
 */
static int get_coin_type(monster_race *r_ptr)
{
  cptr name = (r_name + r_ptr->name);
  
  /* Analyze "coin" monsters */
  if (r_ptr->d_char == '$')
    {
      /* Look for textual clues */
      if (strstr(name, " copper ")) return (lookup_kind(TV_GOLD, SV_COPPER));
      if (strstr(name, " silver ")) return (lookup_kind(TV_GOLD, SV_SILVER));
      if (strstr(name, " gold ")) return (lookup_kind(TV_GOLD, SV_GOLD));
      if (strstr(name, " mithril ")) 
	return (lookup_kind(TV_GOLD, SV_MITHRIL));
      if (strstr(name, " adamantite ")) 
	return (lookup_kind(TV_GOLD, SV_ADAMANTITE));
      
      /* Look for textual clues */
      if (strstr(name, "Copper ")) return (lookup_kind(TV_GOLD, SV_COPPER));
      if (strstr(name, "Silver ")) return (lookup_kind(TV_GOLD, SV_SILVER));
      if (strstr(name, "Gold ")) return (lookup_kind(TV_GOLD, SV_GOLD));
      if (strstr(name, "Mithril ")) 
	return (lookup_kind(TV_GOLD, SV_MITHRIL));
      if (strstr(name, "Adamantite ")) 
	return (lookup_kind(TV_GOLD, SV_ADAMANTITE));
    }
  
  /* Assume nothing */
  return (0);
}


/**
 * Create magical stairs after finishing a quest monster.
 */
static void build_quest_stairs(int y, int x, char *portal)
{
  int ny, nx;
  
  
  /* Stagger around */
  while (!cave_valid_bold(y, x))
    {
      int d = 1;
      
      /* Pick a location */
      scatter(&ny, &nx, y, x, d, 0);
      
      /* Stagger */
      y = ny; x = nx;
    }
  
  /* Destroy any objects */
  delete_object(y, x);
  
  /* Explain the staircase */
  msg_format("A magical %s appears...", portal);
  
  /* Create stairs down */
  cave_set_feat(y, x, FEAT_MORE);
  
  /* Update the visuals */
  p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
}

/**
 * Handle the "death" of a monster.
 *
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 *
 * Check for "Quest" completion when a quest monster is killed.
 *
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 *
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 */
void monster_death(int m_idx)
{
  int i, j, y, x;
  
  int dump_item = 0;
  int dump_gold = 0;
  
  int number = 0;
  
  s16b this_o_idx, next_o_idx = 0;
  
  monster_type *m_ptr = &m_list[m_idx];
  
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  bool visible = (m_ptr->ml || (rf_has(r_ptr->flags, RF_UNIQUE)));
  
  bool good = (rf_has(r_ptr->flags, RF_DROP_GOOD)) ? TRUE : FALSE;
  bool great = (rf_has(r_ptr->flags, RF_DROP_GREAT)) ? TRUE : FALSE;
  
  bool do_gold = (!(rf_has(r_ptr->flags, RF_ONLY_ITEM)));
  bool do_item = (!(rf_has(r_ptr->flags, RF_ONLY_GOLD)));
  
  int force_coin = get_coin_type(r_ptr);
  
  object_type *i_ptr;
  object_type object_type_body;
  
  /* If this monster was a group leader, others become leaderless */
  for (i = m_max - 1; i >= 1; i--)
    {
      /* Access the monster */
      monster_type *n_ptr = &m_list[i];

      /* Check if this was the leader */
      if (n_ptr->group_leader == m_idx) n_ptr->group_leader = 0;
    }


  
  /* Get the location */
  y = m_ptr->fy;
  x = m_ptr->fx;

  /* Drop objects being carried */
  for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Paranoia */
      o_ptr->held_m_idx = 0;
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Copy the object */
      object_copy(i_ptr, o_ptr);
      
      /* Delete the object */
      delete_object_idx(this_o_idx);
      
      /* Drop it */
      drop_near(i_ptr, -1, y, x);
    }
  
  /* Forget objects */
  m_ptr->hold_o_idx = 0;
  
  
  /* Mega-Hack -- drop guardian treasures */
  if (rf_has(r_ptr->flags, RF_DROP_CHOSEN))
    {
      /* Morgoth */
      if (r_ptr->level == 100)
	{
	  /* Get local object */
	  i_ptr = &object_type_body;
	  
	  /* Mega-Hack -- Prepare to make "Grond" */
	  object_prep(i_ptr, lookup_kind(TV_HAFTED, SV_GROND));
	  
	  /* Mega-Hack -- Mark this item as "Grond" */
	  i_ptr->name1 = ART_GROND;
	  
	  /* Mega-Hack -- Actually create "Grond" */
	  apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);
	  
	  /* Drop it in the dungeon */
	  drop_near(i_ptr, -1, y, x);
	  
	  
	  /* Get local object */
	  i_ptr = &object_type_body;
	  
	  /* Mega-Hack -- Prepare to make "Morgoth" */
	  object_prep(i_ptr, lookup_kind(TV_CROWN, SV_MORGOTH));
	  
	  /* Mega-Hack -- Mark this item as "Morgoth" */
	  i_ptr->name1 = ART_MORGOTH;
	  
	  /* Mega-Hack -- Actually create "Morgoth" */
	  apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);
	  
	  /* Drop it in the dungeon */
	  drop_near(i_ptr, -1, y, x);
	}

      /* Ungoliant */
      else if (r_ptr->level == 70)
	{
	  /* Get local object */
	  i_ptr = &object_type_body;
	  
	  /* Mega-Hack -- Prepare to make "Ungoliant" */
	  object_prep(i_ptr, lookup_kind(TV_CLOAK, SV_UNLIGHT_CLOAK));
	  
	  /* Mega-Hack -- Mark this item as "Ungoliant" */
	  i_ptr->name1 = ART_UNGOLIANT;
	  
	  /* Mega-Hack -- Actually create "Ungoliant" */
	  apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);
	  
	  /* Drop it in the dungeon */
	  drop_near(i_ptr, -1, y, x);
	}
    } 
  
  /* Determine how much we can drop */
  if (rf_has(r_ptr->flags, RF_DROP_60) && (randint0(100) < 60)) number++;
  if (rf_has(r_ptr->flags, RF_DROP_90) && (randint0(100) < 90)) number++;
  
  /* Hack -- nothing's more annoying than a chest that doesn't appear. */
  if (rf_has(r_ptr->flags, RF_DROP_CHEST) && rf_has(r_ptr->flags, RF_DROP_90)) 
    number = 1;
  
  if (rf_has(r_ptr->flags, RF_DROP_1D2)) number += damroll(1, 2);
  if (rf_has(r_ptr->flags, RF_DROP_2D2)) number += damroll(2, 2);
  if (rf_has(r_ptr->flags, RF_DROP_3D2)) number += damroll(3, 2);
  if (rf_has(r_ptr->flags, RF_DROP_4D2)) number += damroll(4, 2);
  
  /* Hack -- handle creeping coins */
  coin_type = force_coin;
  
  /* Average dungeon and monster levels */
  object_level = (p_ptr->depth + r_ptr->level) / 2;
  
  /* Drop some objects */
  for (j = 0; j < number; j++)
    {
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Make Gold.  Reduced to 30% chance instead of 50%. */
      if (do_gold && (!do_item || (randint0(100) < 30)))
	{
	  
	  /* Make some gold */
	  if (make_gold(i_ptr)) 
	    {
	      
	      /* Assume seen XXX XXX XXX */
	      dump_gold++;
	      
	      /* Drop it in the dungeon */
	      drop_near(i_ptr, -1, y, x);
	    }
	}
      
      /* Make chest. */
      else if (rf_has(r_ptr->flags, RF_DROP_CHEST))
	{
	  required_tval = TV_CHEST;
	  if (make_object(i_ptr, FALSE, FALSE, TRUE)) 
	    {
	      
	      /* Assume seen XXX XXX XXX */
	      dump_item++;
	      
	      /* Drop it in the dungeon */
	      drop_near(i_ptr, -1, y, x);
	    }
	  required_tval = 0;
	}
      
      /* Make Object */
      else
	{
	  /* Make an object */
	  if (make_object(i_ptr, good, great, FALSE))
	    {
	      
	      /* Assume seen XXX XXX XXX */
	      dump_item++;
	      
	      /* Drop it in the dungeon */
	      drop_near(i_ptr, -1, y, x);
	    }
	}
      
    }
  
  /* Reset the object level */
  object_level = p_ptr->depth;
  
  /* Reset "coin" type */
  coin_type = 0;
  
  
  /* Take note of any dropped treasure */
  if (visible && (dump_item || dump_gold))
    {
      /* Take notes on treasure */
      lore_treasure(m_idx, dump_item, dump_gold);
    }
  
  /* Update monster, item list windows */
  p_ptr->window |= (PW_MONLIST | PW_ITEMLIST);
  
  
  /* If the player kills a unique, write a note.*/
  
  if (rf_has(r_ptr->flags, RF_UNIQUE))
    {
      
      char note2[120];
      char real_name[120];
      
      /* write note for player ghosts */
      if (rf_has(r_ptr->flags, RF_PLAYER_GHOST))
	{
	  my_strcpy(note2, format("Destroyed %^s, the %^s", ghost_name, 
				  r_name + r_ptr->name), sizeof (note2));
	}
      
      /* All other uniques */
      else
	{
	  /* Get the monster's real name for the notes file */
	  monster_desc_race(real_name, sizeof(real_name), m_ptr->r_idx);
	  
	  /* Write note */
	  if ((rf_has(r_ptr->flags, RF_DEMON)) ||
	      (rf_has(r_ptr->flags, RF_UNDEAD)) ||
	      (rf_has(r_ptr->flags, RF_STUPID)) ||
	      (strchr("Evg", r_ptr->d_char)))
	    my_strcpy(note2, format("Destroyed %s", real_name), 
		      sizeof (note2));
	  else my_strcpy(note2, format("Killed %s", real_name), 
			 sizeof (note2));
	}
      
      make_note(note2, p_ptr->stage, NOTE_UNIQUE, p_ptr->lev);
    }
  
  /* Only process "Quest Monsters" */
  if (!(rf_has(r_ptr->flags, RF_QUESTOR))) return;
  
  
  /* Hack -- Mark quests as complete */
  for (i = 0; i < MAX_Q_IDX; i++)
    {
      /* Hack -- note completed quests */
      if (stage_map[q_list[i].stage][1] == r_ptr->level) q_list[i].stage = 0;
    }

  /* Hack -- Mark Sauron's other forms as dead */
  if ((r_ptr->level == 85) && (rf_has(r_ptr->flags, RF_QUESTOR)))
    for (i = 1; i < 4; i++) r_info[m_ptr->r_idx - i].max_num--;

  /* Make a staircase for Morgoth */
  if (r_ptr->level == 100)
    build_quest_stairs(y, x, "staircase");
  
  /* ...or a portal for ironmen wilderness games */
  else if (OPT(adult_ironman) && !OPT(adult_dungeon) && (p_ptr->depth != 100))
    build_quest_stairs(y, x, "portal"); 
  
  /* or a path out of Nan Dungortheb for wilderness games*/
  else if ((r_ptr->level == 70) && (p_ptr->depth == 70) && !OPT(adult_dungeon))
    {
      /* Make a path */
      for (y = p_ptr->py; y < DUNGEON_HGT - 2; y++)
	cave_set_feat(y, p_ptr->px, FEAT_FLOOR);
      cave_set_feat(DUNGEON_HGT - 2, p_ptr->px, FEAT_LESS_SOUTH);

      /* Announce it */
      msg_print("The way out of Nan Dungortheb is revealed!");
    }

  /* Increment complete quests */
  p_ptr->quests++;
  
  /* Check for new specialties */
  p_ptr->update |= (PU_SPECIALTY);
  
  /* Update */
  update_stuff();
  
  /* Was it the big one? */
  if (r_ptr->level == 100)
    {
      /* Total winner */
      p_ptr->total_winner = TRUE;

      /* Have a home now */
      if (!p_ptr->home) p_ptr->home = towns[rp_ptr->hometown];
      
      /* Redraw the "title" */
      p_ptr->redraw |= (PR_TITLE);
      
      /* Congratulations */
      msg_print("*** CONGRATULATIONS ***");
      msg_print("You have won the game!");
      msg_print("You may retire (commit suicide) when you are ready.");
    }
}




/**
 * Decrease a monster's hit points, handle monster death.
 *
 * We return TRUE if the monster has been killed (and deleted).
 *
 * We announce monster death (using an optional "death message"
 * if given, and a otherwise a generic killed/destroyed message).
 *
 * Only "physical attacks" can induce the "You have slain" message.
 * Missile and Spell attacks will induce the "dies" message, or
 * various "specialized" messages.  Note that "You have destroyed"
 * and "is destroyed" are synonyms for "You have slain" and "dies".
 *
 * Invisible monsters induce a special "You have killed it." message.
 *
 * Hack -- we "delay" fear messages by passing around a "fear" flag.
 */
bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note)
{
  monster_type *m_ptr = &m_list[m_idx];
  
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  monster_lore *l_ptr = &l_list[m_ptr->r_idx];
  
  s32b div, new_exp, new_exp_frac;
  
  char	path[1024];
  
  /* Hack - Monsters in stasis are invulnerable. */
  if (m_ptr->stasis) return (FALSE);
  
  /* Redraw (later) if needed */
  if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
  
  /* Wake it up */
  m_ptr->csleep = 0;
  
  /* Always go active */
  m_ptr->mflag |= (MFLAG_ACTV);
  
  /* Hack - Cancel any special player stealth magics. */
  if (p_ptr->superstealth)
    {
      set_superstealth(0,TRUE);
    }
  
  /* Complex message. Moved from melee and archery, now allows spell and 
   * magical items damage to be debugged by wizards.  -LM-
   */
  if (p_ptr->wizard)
    {
      msg_format("You do %d (out of %d) damage.", dam, m_ptr->hp);
    }
  
  /* Hurt it */
  m_ptr->hp -= dam;
  
  /* It is dead now */
  if (m_ptr->hp < 0)
    {
      char m_name[80];
      char m_poss[80];
      
      /* Shapeshifters die as their original form */
      if (m_ptr->schange)
	{
	  /* Paranoia - make sure _something_ dies */
	  if (!m_ptr->orig_idx) m_ptr->orig_idx = m_ptr->r_idx;	   

	  /* Extract monster name */
	  monster_desc(m_name, m_ptr, 0);
      
	  /* Get the monster possessive */
	  monster_desc(m_poss, m_ptr, 0x22);
  
	  /* Do the change */
	  m_ptr->r_idx = m_ptr->orig_idx;
	  m_ptr->orig_idx = 0;
	  m_ptr->p_race = m_ptr->old_p_race;
	  m_ptr->old_p_race = NON_RACIAL;
	  r_ptr = &r_info[m_ptr->r_idx];

	  /* Note the change */
	  msg_format("%^s is revealed in %s true form.", m_name, m_poss);
	}
  
      /* Extract monster name */
      monster_desc(m_name, m_ptr, 0);
      
      /* Make a sound */
      sound(MSG_KILL);
      
      /* Specialty Ability SOUL_SIPHON */
      if ((check_ability(SP_SOUL_SIPHON)) && 
	  (p_ptr->csp < p_ptr->msp) && 
	  (!((rf_has(r_ptr->flags, RF_DEMON)) || 
	     (rf_has(r_ptr->flags, RF_UNDEAD)) || 
	     (rf_has(r_ptr->flags, RF_STUPID)))))
	{
	  p_ptr->mana_gain += 2 + (m_ptr->maxhp / 30);
	}
      
      /* Increase the noise level slightly. */
      if (add_wakeup_chance <= 8000) add_wakeup_chance += 500;

      /* Death by Missile/Spell attack */
      if (note)
	{
	  message_format(MSG_KILL, p_ptr->lev, "%^s%s", m_name, note);
	}
      
      /* Death by physical attack -- invisible monster */
      else if (!m_ptr->ml)
	{
	  message_format(MSG_KILL, p_ptr->lev, "You have killed %s.", m_name);
	}
      
      /* Death by Physical attack -- non-living monster */
      else if ((rf_has(r_ptr->flags, RF_DEMON)) ||
	       (rf_has(r_ptr->flags, RF_UNDEAD)) ||
	       (rf_has(r_ptr->flags, RF_STUPID)) ||
	       (strchr("Evg", r_ptr->d_char)))
	{
	  message_format(MSG_KILL, p_ptr->lev, "You have destroyed %s.", 
			 m_name);
	}
      
      /* Death by Physical attack -- living monster */
      else
	{
	  message_format(MSG_KILL, p_ptr->lev, "You have slain %s.", m_name);
	}
      
      /* Maximum player level */
      div = p_ptr->max_lev;
      
      /* Give some experience for the kill */
      new_exp = ((long)r_ptr->mexp * r_ptr->level) / div;
      
      /* Handle fractional experience */
      new_exp_frac = ((((long)r_ptr->mexp * r_ptr->level) % div)
		      * 0x10000L / div) + p_ptr->exp_frac;
      
      /* Keep track of experience */
      if (new_exp_frac >= 0x10000L)
	{
	  new_exp++;
	  p_ptr->exp_frac = (u16b)(new_exp_frac - 0x10000L);
	}
      else
	{
	  p_ptr->exp_frac = (u16b)new_exp_frac;
	}
      
      /* Gain experience */
      gain_exp(new_exp);
      
      /* Generate treasure */
      monster_death(m_idx);

      /* When the player kills a Unique, it stays dead */
      if (rf_has(r_ptr->flags, RF_UNIQUE)) r_ptr->max_num--;
      
      /* When the player kills a player ghost, the bones file that 
       * it used is (often) deleted.
       */
      if (rf_has(r_ptr->flags, RF_PLAYER_GHOST))
	{
	  if (randint1(3) != 1)
	    {
	      sprintf(path, "%s/bone.%03d", ANGBAND_DIR_BONE, 
		      bones_selector);
#ifdef _WIN32_WCE
	      {
		TCHAR wcpath[1024];
		mbstowcs(wcpath, path, 1024);
		
		DeleteFile(wcpath);
	      }
#else
 	      remove(path);
#endif
	    }
	}
      
      /* Recall even invisible uniques or winners */
      if (m_ptr->ml || (rf_has(r_ptr->flags, RF_UNIQUE)))
	{
	  /* Count kills this life */
	  if (l_ptr->pkills < MAX_SHORT) l_ptr->pkills++;

	  /* Add to score if the first time */
	  if (l_ptr->pkills == 1) p_ptr->score += (check_ability(SP_DIVINE) ? new_exp / 2 : new_exp);
	  
	  /* Count kills in all lives */
	  if (l_ptr->tkills < MAX_SHORT) l_ptr->tkills++;
	  
	  /* Hack -- Auto-recall */
	  monster_race_track(m_ptr->r_idx);
	}
      
      /* Delete the monster */
      delete_monster_idx(m_idx);
      
      /* Not afraid */
      (*fear) = FALSE;
      
      /* Monster is dead */
      return (TRUE);
    }
  
  
  /* Mega-Hack -- Pain cancels fear */
  if (m_ptr->monfear && (dam > 0))
    {
      int tmp = randint1(dam);
      
      /* Cure a little fear */
      if (tmp < m_ptr->monfear)
	{
	  /* Reduce fear */
	  m_ptr->monfear -= tmp;
	}
      
      /* Cure all the fear */
      else
	{
	  /* Cure fear */
	  m_ptr->monfear = 0;
	  
	  /* Flag minimum range for recalculation */
	  m_ptr->min_range = 0;
	  
	  /* No more fear */
	  (*fear) = FALSE;
	}
    }
  
  /* Sometimes a monster gets scared by damage */
  if (!m_ptr->monfear && !(rf_has(r_ptr->flags, RF_NO_FEAR)))
    {
      int percentage;
      
      /* Percentage of fully healthy */
      percentage = (100L * m_ptr->hp) / m_ptr->maxhp;
      
      /*
       * Run (sometimes) if at 10% or less of max hit points,
       * or (usually) when hit for half its current hit points
       */
      if ((dam > 0) && ((randint1(10) >= percentage) || 
			((dam >= m_ptr->hp) && (randint0(100) < 80))))
	{
	  /* Hack -- note fear */
	  (*fear) = TRUE;
	  
	  /* Hack -- Add some timed fear */
	  m_ptr->monfear = (randint1(10) +
			    (((dam >= m_ptr->hp) && (percentage > 7)) ?
			     20 : ((11 - percentage) * 5)));
	  
	  /* Flag minimum range for recalculation */
	  m_ptr->min_range = 0;
	}
    }
  
  /* Recalculate desired minimum range */
  if (dam > 0) m_ptr->min_range = 0;
  
  /* Not dead yet */
  return (FALSE);
}


/**
 * Calculates current boundaries
 */
void panel_recalc_bounds(void)
{
  int wid, hgt;
  
  /* Get size */
  Term_get_size(&wid, &hgt);
  
  if (hgt > (OPT(bottom_status) ? 31 : 25)) 
    {
      panel_extra_rows = TRUE;
    }
  else panel_extra_rows = FALSE;
  
  panel_row_max = panel_row_min + SCREEN_HGT - 1;
  panel_col_max = panel_col_min + SCREEN_WID - 1;
}


/**
 * Handle a request to change the current panel
 *
 * Return TRUE if the panel was changed.
 *
 * Also used in do_cmd_locate
 */
bool change_panel(int dy, int dx)
{
  int y, x;
  int y_min, x_min, y_max, x_max;

  bool small_town = ((p_ptr->stage < 151) && (!OPT(adult_dungeon)));
  
  /* Apply the motion */
  y = panel_row_min + dy * (SCREEN_HGT / 2);
  x = panel_col_min + dx * (SCREEN_WID / 2);

  /* Desired bounds */
  y_min = (p_ptr->depth ? 0 : DUNGEON_HGT / 3);
  x_min = (p_ptr->depth ? 0 : DUNGEON_WID / 3);
  y_max = (p_ptr->depth ? DUNGEON_HGT : 2 * DUNGEON_HGT / 3);
  x_max = (p_ptr->depth ? DUNGEON_WID : 2 * DUNGEON_WID / 3);
  if ((!p_ptr->depth) && small_town) x_max = DUNGEON_WID / 2;
  
  /* Bounds */
  if (y > y_max - SCREEN_HGT) y = y_max - SCREEN_HGT;
  if (y < y_min) y = y_min;
  if (x > x_max - SCREEN_WID) x = x_max - SCREEN_WID;
  if (x < x_min) x = x_min;
	
  /* Handle "changes" */
  if ((y != panel_row_min) || (x != panel_col_min))
    {
      /* Save the new panel info */
      panel_row_min = y;
      panel_col_min = x;
      
      /* Recalculate the boundaries */
      panel_recalc_bounds();
      
      /* Update stuff */
      p_ptr->update |= (PU_MONSTERS);
      
      /* Redraw map */
      p_ptr->redraw |= (PR_MAP);
      
      /* Handle stuff */
      handle_stuff();

      /* Redraw for big graphics */
      if ((tile_width > 1) || (tile_height > 1)) redraw_stuff();
      
      /* Success */
      return (TRUE);
    }
  
  /* No change */
  return (FALSE);
}


void verify_panel(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y = py;
  int x = px;
  
  int y_min, x_min, y_max, x_max;
  
  int prow_min;
  int pcol_min;
  
  int max_prow_min;
  int max_pcol_min;
  
  int change_col = 4 * (1 + op_ptr->panel_change);
  int change_row = 2 * (1 + op_ptr->panel_change);

  /* This seems a good idea with all the config options now -NRM- */
  panel_recalc_bounds();
  
  /* Hack - in town */
  if (!p_ptr->depth && !small_screen && (SCREEN_HGT >= (DUNGEON_HGT / 3) - 1) 
      && (((p_ptr->stage < 151) && (SCREEN_WID >= (DUNGEON_WID / 6))) || 
	  (((p_ptr->stage > 150) || OPT(adult_dungeon)) && 
	   (SCREEN_WID >= (DUNGEON_WID / 3)))))
    {
      
      prow_min = DUNGEON_HGT / 3;
      pcol_min = DUNGEON_WID / 3;

    }
  
  else
    {
      max_prow_min = DUNGEON_HGT - SCREEN_HGT;
      max_pcol_min = DUNGEON_WID - SCREEN_WID;
      
      /* Bounds checking */
      if (max_prow_min < 0) max_prow_min = 0;
      if (max_pcol_min < 0) max_pcol_min = 0;
      
      /* Center on player */
      if (OPT(center_player) && (OPT(center_running) || !p_ptr->running))
	{
	  /* Center vertically */
	  prow_min = y - SCREEN_HGT / 2;
	  if (prow_min < 0) prow_min = 0;
	  else if (prow_min > max_prow_min) prow_min = max_prow_min;
	  
	  /* Center horizontally */
	  pcol_min = x - SCREEN_WID / 2;
	  if (pcol_min < 0) pcol_min = 0;
	  else if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
	}
      else
	{
	  prow_min = panel_row_min;
	  pcol_min = panel_col_min;
	  
	  /* Make certain the initial values are valid. */
	  if (prow_min > max_prow_min) prow_min = max_prow_min;
	  if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
	  
	  /* Scroll screen when change_row grids from top/bottom edge */
	  if ((y > panel_row_max - change_row) || 
	      (y < panel_row_min + change_row))
	    {
	      /* Center vertically */
	      prow_min = y - SCREEN_HGT / 2;
	      if (prow_min < 0) prow_min = 0;
	      else if (prow_min > max_prow_min) prow_min = max_prow_min;
	    }
	  
	  /* Scroll screen when change_col grids from left/right edge */
	  if ((x > panel_col_max - change_col) ||
	      (x < panel_col_min + change_col))
	    {
	      /* Center horizontally */
	      pcol_min = x - SCREEN_WID / 2;
	      if (pcol_min < 0) pcol_min = 0;
	      else if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
	    }
      	}

      /* Hack - inefficient anti-scrolling for town */
      if (!p_ptr->depth)
	{
	  /* Desired bounds */
	  y_min = DUNGEON_HGT / 3;
	  x_min = DUNGEON_WID / 3;
	  y_max = 2 * DUNGEON_HGT / 3 - SCREEN_HGT;
	  x_max = ((p_ptr->stage < 151) && !OPT(adult_dungeon)) ? 
		   (DUNGEON_WID / 2) - SCREEN_WID
		   : (2 * DUNGEON_WID / 3) - SCREEN_WID;
      
	  /* Bounds */
	  if ((prow_min > y_max) && (py > y_max)) prow_min = y_max;
	  if (prow_min < y_min) prow_min = y_min;
	  if ((pcol_min > x_max) && (px > x_max)) pcol_min = x_max;
	  if (pcol_min < x_min) pcol_min = x_min;
	}
	
    }
  /* Check for "no change" */
  if ((prow_min == panel_row_min) && (pcol_min == panel_col_min)) return;
  
  /* Save the new panel info */
  panel_row_min = prow_min;
  panel_col_min = pcol_min;
  
  /* Hack -- optional disturb on "panel change" */
  if (OPT(disturb_panel) && !OPT(center_player)) disturb(0, 0);
  
  /* Recalculate the boundaries */
  panel_recalc_bounds();
  
  /* Update stuff */
  p_ptr->update |= (PU_MONSTERS);
  
  /* Redraw map */
  p_ptr->redraw |= (PR_MAP);
  
  /* Window stuff */
  p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/**
 * Monster health description
 */
void look_mon_desc(int m_idx, char *buf, size_t max)
{
  monster_type *m_ptr = &m_list[m_idx];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  bool living = TRUE;
  int perc;
  
  
  /* Determine if the monster is "living" (vs "undead") */
  if (rf_has(r_ptr->flags, RF_UNDEAD)) living = FALSE;
  if (rf_has(r_ptr->flags, RF_DEMON)) living = FALSE;
  if (strchr("Egv", r_ptr->d_char)) living = FALSE;
  
  
  /* Healthy monsters */
  if (m_ptr->hp >= m_ptr->maxhp)
    {
      /* No damage */
      if (small_screen)
	my_strcpy(buf, living ? "unht" : "undm", max);
      else
	my_strcpy(buf, (living ? "unhurt" : "undamaged"), max);
    }
  else 
    {
      /* Calculate a health "percentage" */
      perc = 100L * m_ptr->hp / m_ptr->maxhp;
  
      if (perc >= 60)
        {
          if (small_screen) {
            my_strcpy(buf, (living ? "slwd" : "sldm"), max);
          } else {
            my_strcpy(buf, living ? "somewhat wounded" : "somewhat damaged", max);
          }
        }
      else if (perc >= 25)
        {
          if (small_screen)
            my_strcpy(buf, (living ? "wnd" : "dmg"), max);
          else
            my_strcpy(buf, living ? "wounded" : "damaged", max);
        }
      else if (perc >= 10)
        {
          if (small_screen) {
            my_strcpy(buf, (living ? "bdwd" : "bddm"), max);
          } else {
            my_strcpy(buf, (living ? "badly wounded" : "badly damaged"), max);
          }
        }
      else 
        {
          if (small_screen) {
            my_strcpy(buf, "aldd", max);
          } else {
            my_strcpy(buf, (living ? "almost dead" : "almost destroyed"), max);
          }
        }
    }

  /* Append monster status */
  if (m_ptr->csleep) {
    if (small_screen) {
      my_strcat(buf, ",aslp", max);
    } else {
      my_strcat(buf, ",asleep", max);
    }
  }
  if (m_ptr->confused) {
    if (small_screen) {
      my_strcat(buf, ",conf", max);
    } else {
      my_strcat(buf, ",confused", max);
    }
  }
  if (m_ptr->monfear) {
    if (small_screen) {
      my_strcat(buf, ",afrd", max);
    } else {
      my_strcat(buf, ",afraid", max);
    }
  }
  if (m_ptr->stunned) {
    if (small_screen) {
      my_strcat(buf, ",stun", max);
    } else {
      my_strcat(buf, ",stunned", max);
    }
  }
}

/**
 * Monster attitude description
 */
cptr look_mon_host(int m_idx)
{
  monster_type *m_ptr = &m_list[m_idx];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];

  /* Non-racial - irrelevant */
  if (!(rf_has(r_ptr->flags, RF_RACIAL)))
    return ("");
  
  /* Hostile monsters */
  if (m_ptr->hostile < 0)
    {
      /* No damage */
      if (small_screen)
	return (",host");
      else
	return (",hostile");
    }
  
  /* Not hostile to the player */
  else 
    {
      if (small_screen)
	return (",neut");
      else
	return (",neutral");
    }
  
}



/**
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort_aux(void *u, void *v, int p, int q)
{
  int z, a, b;
  
  /* Done sort */
  if (p >= q) return;
  
  /* Pivot */
  z = p;
  
  /* Begin */
  a = p;
  b = q;
  
  /* Partition */
  while (TRUE)
    {
      /* Slide i2 */
      while (!(*ang_sort_comp)(u, v, b, z)) b--;
      
      /* Slide i1 */
      while (!(*ang_sort_comp)(u, v, z, a)) a++;
      
      /* Done partition */
      if (a >= b) break;
      
      /* Swap */
      (*ang_sort_swap)(u, v, a, b);
      
      /* Advance */
      a++, b--;
    }
  
  /* Recurse left side */
  ang_sort_aux(u, v, p, b);
  
  /* Recurse right side */
  ang_sort_aux(u, v, b+1, q);
}


/**
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(void *u, void *v, int n)
{
  /* Sort the array */
  ang_sort_aux(u, v, 0, n-1);
}





/*** Targetting Code ***/


/**
 * Extract a direction (or zero) from a character
 */
int target_dir(char ch)
{
  int d = 0;
  
  int mode;
  
  cptr act;
  
  cptr s;
  
  
  /* Already a direction? */
  if (isdigit(ch))
    {
      /* Use that */
      d = D2I(ch);
    }
  
  else if (isarrow(ch))
    {
      switch (ch)
	{
	case ARROW_DOWN:	d = 2; break;
	case ARROW_LEFT:	d = 4; break;
	case ARROW_RIGHT:	d = 6; break;
	case ARROW_UP:		d = 8; break;
	}
    }
  /* Look up keymap */
  else
    {
      /* Roguelike */
      if (OPT(rogue_like_commands))
	{
	  mode = KEYMAP_MODE_ROGUE;
	}
      
      /* Original */
      else
	{
	  mode = KEYMAP_MODE_ORIG;
	}
      
      /* Extract the action (if any) */
      act = keymap_act[mode][(byte)(ch)];
      
      /* Analyze */
      if (act)
	{
	  /* Convert to a direction */
	  for (s = act; *s; ++s)
	    {
	      /* Use any digits in keymap */
	      if (isdigit(*s)) d = D2I(*s);
	    }
	}
    }
  
  /* Paranoia */
  if (d == 5) d = 0;
  
  /* Return direction */
  return (d);
}


/**
 * Extract a direction (or zero) from a mousepress
 */
extern int mouse_dir(ui_event_data ke, bool locating)
{
  int i, y, x;

  int gy = KEY_GRID_Y(ke), gx = KEY_GRID_X(ke);

  int py = p_ptr->py, px = p_ptr->px;

  if (locating) 
    {
      gy = ke.mousey;
      gx = ke.mousex;
      py = Term->hgt / 2;
      px = Term->wid / 2;
    }
  
  y = ABS(gy - py);
  x = ABS(gx - px);

  if ((y == 0) && (x == 0)) return (0);

  /* Click next to player */
  if ((y <= 1) && (x <= 1)) 
    {
      /* Get the direction */
      for (i = 0; i < 10; i++)
	if ((ddx[i] == (gx - px)) && (ddy[i] == (gy - py))) break;

      /* Take it */
      return (i);
    }
  
  if (2 * y < x) 
    {
      y = 0;
      x = (x / (gx - px));
    }
  else if (2 * x < y) 
    {
      x = 0;
      y = (y / (gy - py));
    }
  else
    {
      y = (y / (gy - py));
      x = (x / (gx - px));
    }

  for (i = 0; i < 10; i++)
    if ((ddy[i] == y) && (ddx[i] == x)) break;

  /* paranoia */
  if ((i % 5) == 0) i = 0;

  return (i);
}



/**
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targetting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(int m_idx)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  monster_type *m_ptr;
  
  /* No monster */
  if (m_idx <= 0) return (FALSE);
  
  /* Get monster */
  m_ptr = &m_list[m_idx];
  
  /* Monster must be alive */
  if (!m_ptr->r_idx) return (FALSE);
  
  /* Monster must be visible */
  if (!m_ptr->ml) return (FALSE);
  
  /* Monster must be projectable */
  if (!projectable(py, px, m_ptr->fy, m_ptr->fx, 0)) return (FALSE);
  
  /* Hack -- no targeting hallucinations */
  if (p_ptr->timed[TMD_HALLUC]) return (FALSE);
  
  /* Hack -- Never target trappers XXX XXX XXX */
  /* if (CLEAR_ATTR && (CLEAR_CHAR)) return (FALSE); */
  
  /* Assume okay */
  return (TRUE);
}

/**
 * Determine if an object makes a reasonable target
 *
 * The player can target any location, or any "target-able" object.
 *
 * Currently, an object is "target_able" if the player can hit it with a 
 * projection, and the player is not hallucinating.  This allows use of 
 * "use closest target" macros.
 *
 * This is used for the Telekinesis spell -NRM-
 */
bool target_able_obj(int o_idx)
{
  int py = p_ptr->py;
  int px = p_ptr->px;

  int this_o_idx, next_o_idx;

  bool only_gold = TRUE;
  
  object_type *o_ptr;
  
  /* No object */
  if (o_idx <= 0) return (FALSE);
  
  /* Scan all objects in the grid */
  for (this_o_idx = o_idx; this_o_idx; this_o_idx = next_o_idx)
    {
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Found a non-gold object */
      if (o_ptr->tval != TV_GOLD) 
	{
	  only_gold = FALSE;
	  break;
	}
    }
  
  /* Object must exist */
  if (!o_ptr->k_idx) return (FALSE);

  /* There must be a non-gold object */
  if (only_gold) return (FALSE);
  
  /* Object must be projectable */
  if (!projectable(py, px, o_ptr->iy, o_ptr->ix, 0)) return (FALSE);
  
  /* Hack -- no targeting hallucinations */
  if (p_ptr->timed[TMD_HALLUC]) return (FALSE);
  
  /* Assume okay */
  return (TRUE);
}




/**
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(void)
{
  /* No target */
  if (!p_ptr->target_set) return (FALSE);
  
  /* Accept "location" targets */
  if ((p_ptr->target_who == 0) && (p_ptr->target_what == 0)) return (TRUE);
  
  /* Check "monster" targets */
  if (p_ptr->target_who > 0)
    {
      int m_idx = p_ptr->target_who;
      
      /* Accept reasonable targets */
      if (target_able(m_idx))
	{
	  monster_type *m_ptr = &m_list[m_idx];
	  
	  /* Acquire monster location */
	  p_ptr->target_row = m_ptr->fy;
	  p_ptr->target_col = m_ptr->fx;
	  
	  /* Good target */
	  return (TRUE);
	}
    }
  
  /* Check "object" targets */
  if (p_ptr->target_what > 0)
    {
      int o_idx = p_ptr->target_what;
      
      /* Accept reasonable targets */
      if (target_able_obj(o_idx))
	{
	  object_type *o_ptr = &o_list[o_idx];
	  
	  /* Acquire object location */
	  p_ptr->target_row = o_ptr->iy;
	  p_ptr->target_col = o_ptr->ix;
	  
	  /* Good target */
	  return (TRUE);
	}
    }

  /* Assume no target */
  return (FALSE);
}


/**
 * Set the target to a monster (or nobody)
 */
void target_set_monster(int m_idx)
{
  /* Acceptable target */
  if ((m_idx > 0) && target_able(m_idx))
    {
      monster_type *m_ptr = &m_list[m_idx];
      
      /* Save target info */
      p_ptr->target_set = TRUE;
      p_ptr->target_who = m_idx;
      p_ptr->target_what = 0;
      p_ptr->target_row = m_ptr->fy;
      p_ptr->target_col = m_ptr->fx;
    }
  
  /* Clear target */
  else
    {
      /* Reset target info */
      p_ptr->target_set = FALSE;
      p_ptr->target_who = 0;
      p_ptr->target_what = 0;
      p_ptr->target_row = 0;
      p_ptr->target_col = 0;
    }
}


/**
 * Set the target to an object
 */
void target_set_object(int o_idx)
{
  /* Acceptable target */
  if ((o_idx > 0) && target_able_obj(o_idx))
    {
      object_type *o_ptr = &o_list[o_idx];
      
      /* Save target info */
      p_ptr->target_set = TRUE;
      p_ptr->target_who = 0;
      p_ptr->target_what = o_idx;
      p_ptr->target_row = o_ptr->iy;
      p_ptr->target_col = o_ptr->ix;
    }
  
  /* Clear target */
  else
    {
      /* Reset target info */
      p_ptr->target_set = FALSE;
      p_ptr->target_who = 0;
      p_ptr->target_what = 0;
      p_ptr->target_row = 0;
      p_ptr->target_col = 0;
    }
}


/**
 * Set the target to a location
 */
void target_set_location(int y, int x)
{
  /* Legal target */
  if (in_bounds_fully(y, x))
    {
      /* Save target info */
      p_ptr->target_set = TRUE;
      p_ptr->target_who = 0;
      p_ptr->target_what = 0;
      p_ptr->target_row = y;
      p_ptr->target_col = x;
    }
  
  /* Clear target */
  else
    {
      /* Reset target info */
      p_ptr->target_set = FALSE;
      p_ptr->target_who = 0;
      p_ptr->target_what = 0;
      p_ptr->target_row = 0;
      p_ptr->target_col = 0;
    }
}


/**
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
static bool ang_sort_comp_distance(void *u, void *v, int a, int b)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  byte *x = (byte*)(u);
  byte *y = (byte*)(v);
  
  int da, db, kx, ky;
  
  /* Absolute distance components */
  kx = x[a]; kx -= px; kx = ABS(kx);
  ky = y[a]; ky -= py; ky = ABS(ky);
  
  /* Approximate Double Distance to the first point */
  da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));
  
  /* Absolute distance components */
  kx = x[b]; kx -= px; kx = ABS(kx);
  ky = y[b]; ky -= py; ky = ABS(ky);
  
  /* Approximate Double Distance to the first point */
  db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));
  
  /* Compare the distances */
  return (da <= db);
}


/**
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
static void ang_sort_swap_distance(void *u, void *v, int a, int b)
{
  byte *x = (byte*)(u);
  byte *y = (byte*)(v);
  
  byte temp;
  
  /* Swap "x" */
  temp = x[a];
  x[a] = x[b];
  x[b] = temp;
  
  /* Swap "y" */
  temp = y[a];
  y[a] = y[b];
  y[b] = temp;
}



/**
 * Hack -- help "select" a location (see below)
 */
static s16b target_pick(int y1, int x1, int dy, int dx)
{
  int i, v;
  
  int x2, y2, x3, y3, x4, y4;
  
  int b_i = -1, b_v = 9999;
  
  
  /* Scan the locations */
  for (i = 0; i < temp_n; i++)
    {
      /* Point 2 */
      x2 = temp_x[i];
      y2 = temp_y[i];
      
      /* Directed distance */
      x3 = (x2 - x1);
      y3 = (y2 - y1);
      
      /* Verify quadrant */
      if (dx && (x3 * dx <= 0)) continue;
      if (dy && (y3 * dy <= 0)) continue;
      
      /* Absolute distance */
      x4 = ABS(x3);
      y4 = ABS(y3);
      
      /* Verify quadrant */
      if (dy && !dx && (x4 > y4)) continue;
      if (dx && !dy && (y4 > x4)) continue;
      
      /* Approximate Double Distance */
      v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));
      
      /* Penalize location XXX XXX XXX */
      
      /* Track best */
      if ((b_i >= 0) && (v >= b_v)) continue;
      
      /* Track best */
      b_i = i; b_v = v;
    }
  
  /* Result */
  return (b_i);
}


/**
 * Hack -- determine if a given location is "interesting"
 */
static bool target_set_interactive_accept(int y, int x)
{
  s16b this_o_idx, next_o_idx = 0;
  
  /* Bounds */
  if (!(in_bounds(y, x))) return (FALSE);
  
  /* Player grids are always interesting */
  if (cave_m_idx[y][x] < 0) return (TRUE);
  
  
  /* Handle hallucination */
  if (p_ptr->timed[TMD_HALLUC]) return (FALSE);
  
  
  /* Visible monsters */
  if (cave_m_idx[y][x] > 0)
    {
      monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
      
      /* Visible monsters */
      if (m_ptr->ml) return (TRUE);
    }
  
  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      object_type *o_ptr;
      
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Memorized object */
      if (o_ptr->marked && !squelch_hide_item(o_ptr)) return (TRUE);
    }
  
  /* Interesting memorized features */
  if (cave_info[y][x] & (CAVE_MARK))
    {
      feature_type *f_ptr = &f_info[cave_feat[y][x]];

      /* Notice interesting things */
      if (f_ptr->flags & TF_INTERESTING) return (TRUE);
    }
  
  /* Nope */
  return (FALSE);
}


/**
 * Prepare the "temp" array for "target_interactive_set"
 *
 * Return the number of target_able monsters in the set.
 */
static void target_set_interactive_prepare(int mode)
{
  int y, x;
  
  /* Reset "temp" array */
  temp_n = 0;
  
  /* Scan the current panel */
  for (y = panel_row_min; y <= panel_row_max; y++)
    {
      for (x = panel_col_min; x <= panel_col_max; x++)
	{
	  /* Require line of sight, unless "look" is "expanded" */
	  if (!OPT(expand_look) && !player_has_los_bold(y, x)) continue;
	  
	  /* Require "interesting" contents */
	  if (!target_set_interactive_accept(y, x)) continue;
	  
	  /* Monster mode */
	  if (mode & (TARGET_KILL))
	    {
	      /* Must contain a monster */
	      if (!(cave_m_idx[y][x] > 0)) continue;
	      
	      /* Must be a targettable monster */
	      if (!target_able(cave_m_idx[y][x])) continue;
	    }
	  
	  /* Object mode */
	  if (mode & (TARGET_OBJ))
	    {
	      /* Must contain an object */
	      if (!(cave_o_idx[y][x] > 0)) continue;
	      
	      /* Must be a targettable object */
	      if (!target_able_obj(cave_o_idx[y][x])) continue;
	    }
	  
	  /* Save the location */
	  temp_x[temp_n] = x;
	  temp_y[temp_n] = y;
	  temp_n++;
	}
    }
  
  /* Set the sort hooks */
  ang_sort_comp = ang_sort_comp_distance;
  ang_sort_swap = ang_sort_swap_distance;
  
  /* Sort the positions */
  ang_sort(temp_x, temp_y, temp_n);
}

/** 
 * Abbreviated monster names for small screen
 */
void cut_down(char *name)
{
  bool kill = FALSE;
  bool copy = TRUE;
  bool had_space = FALSE;

  int i, j = 0;

  char new[80];

  for (i = 0; name[i] != '\0'; i++)
    {
      /* Record first space, skip subsequent ones */
      if (name[i] == ' ')
	{
	  copy = !had_space;
	  kill = FALSE;
	  had_space = TRUE;
	}

      /* Add non-space non-letters */ 
      else if (!isalpha(name[i])) 
	{
	  copy = TRUE;
	  kill = FALSE;
	}

      /* Add initials and letters before the first space */
      else if (!kill)
	{
	  copy = TRUE;
	  kill = had_space;
	}

      /* Non-initial letters */
      else 
	{
	  copy = FALSE;
	  kill = TRUE;
	}

      /* Copy */
      if (copy) new[j++] = name[i];
    }

  /* Terminate */
  new[j] = '\0';

  /* Shortened name */
  for (i = 0; i < j + 1; i++) name[i] = new[i];
}



/**
 * Examine a grid, return a keypress.
 *
 * The "mode" argument contains the "TARGET_LOOK" bit flag, which
 * indicates that the "space" key should scan through the contents
 * of the grid, instead of simply returning immediately.  This lets
 * the "look" command get complete information, without making the
 * "target" command annoying.
 *
 * The "info" argument contains the "commands" which should be shown
 * inside the "[xxx]" text.  This string must never be empty, or grids
 * containing monsters will be displayed with an extra comma.
 *
 * Note that if a monster is in the grid, we update both the monster
 * recall info and the health bar info to track that monster.
 *
 * This function correctly handles multiple objects per grid, and objects
 * and terrain features in the same grid, though the latter never happens.
 *
 * This function uses Tim Baker's code to look at objects.
 *
 * This function must handle blindness/hallucination.
 */
static ui_event_data target_set_interactive_aux(int y, int x, int mode, cptr info)
{
  s16b this_o_idx, next_o_idx = 0;
  
  cptr s1, s2, s3, s4, s5;
  
  bool boring;
  
  int feat;
  
  ui_event_data query = EVENT_EMPTY;
  
  char out_val[160];
  
  int floor_list[24], floor_num;
  
  
  /* Repeat forever */
  while (1)
    {
      /* Paranoia */
      query.key = ' ';
      
      /* Assume boring */
      boring = TRUE;
      
      /* Default */
      s1 = "You see ";
      s2 = "";
      s3 = "";
      
      
      /* The player */
      if (cave_m_idx[y][x] < 0)
	{
	  /* Description */
	  s1 = "You are ";
	  
	  /* Preposition */
	  s2 = "on ";
	}
      
      
      /* Hack -- hallucination */
      if (p_ptr->timed[TMD_HALLUC])
	{
	  cptr name = "something strange";
	  
	  /* Display a message */
	  sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
	  prt(out_val, 0, 0);
	  move_cursor_relative(y, x);
	  query = inkey_ex();
	  
	  /* Stop on everything but "return" */
	  if ((query.key != '\r') && (query.key != '\n')) break;
	  
	  /* Repeat forever */
	  continue;
	}
      
      
      /* Actual monsters */
      if (cave_m_idx[y][x] > 0)
	{
	  monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
	  monster_race *r_ptr = &r_info[m_ptr->r_idx];
	  
	  /* Visible */
	  if (m_ptr->ml)
	    {
	      bool recall = FALSE;
	      
	      char m_name[80];
	      
	      /* Not boring */
	      boring = FALSE;
	      
	      /* Get the monster name ("a kobold") */
	      monster_desc(m_name, m_ptr, 0x08);
	      
	      /* Hack -- track this monster race */
	      monster_race_track(m_ptr->r_idx);
	      
	      /* Hack -- health bar for this monster */
	      health_track(cave_m_idx[y][x]);
	      
	      /* Hack -- handle stuff */
	      handle_stuff();
	      
	      /* Interact */
	      while (1)
		{
		  /* Recall */
		  if (recall)
		    {
		      /* Save screen */
		      screen_save();
		      
		      /* Recall on screen */
		      screen_roff(m_ptr->r_idx);
		      
		      /* Hack -- Complete the prompt (again) */
		      Term_addstr(-1, TERM_WHITE, format("  [r,%s]", info));
		      
		      /* Command */
		      query = inkey_ex();
		      
		      /* Load screen */
		      screen_load();
		    }
		  
		  /* Normal */
		  else
		    {
                      char m_desc[80];
		      /* Describe, and prompt for recall */
		      if (small_screen) cut_down(m_name);
                      look_mon_desc(cave_m_idx[y][x], m_desc, sizeof(m_desc));
		      sprintf(out_val, "%s%s%s%s (%s%s) [r,%s]",
			      s1, s2, s3, m_name, 
			      m_desc, 
			      look_mon_host(cave_m_idx[y][x]), info);
		      prt(out_val, 0, 0);
		      
		      /* Place cursor */
		      move_cursor_relative(y, x);
		      
		      /* Command */
		      query = inkey_ex();

		      /* Check for mouse recall */
		      if ((query.key == '\xff') && (KEY_GRID_Y(query) == y) &&
			  (KEY_GRID_X(query) == x)) query.key = 'r';
		    }
		  
		  /* Normal commands */
		  if (query.key != 'r') break;
		  
		  /* Toggle recall */
		  recall = !recall;
		}
	      
	      /* Always stop at "normal" keys */
	      if ((query.key != '\r') && (query.key != '\n') && 
		  (query.key != ' ')) break;
	      
	      /* Sometimes stop at "space" key */
	      if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;
	      
	      /* Change the intro */
	      s1 = "It is ";
	      
	      /* Hack -- take account of gender */
	      if (rf_has(r_ptr->flags, RF_FEMALE)) s1 = "She is ";
	      else if (rf_has(r_ptr->flags, RF_MALE)) s1 = "He is ";
	      
	      /* Use a preposition */
	      s2 = "carrying ";
	      
	      /* Scan all objects being carried */
	      for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; 
		   this_o_idx = next_o_idx)
		{
		  char o_name[120];
		  
		  object_type *o_ptr;
		  
		  /* Acquire object */
		  o_ptr = &o_list[this_o_idx];
		  
		  /* Acquire next object */
		  next_o_idx = o_ptr->next_o_idx;
		  
		  /* Obtain an object description */
		  object_desc(o_name, o_ptr, TRUE, 3);
		  
		  /* Describe the object */
		  sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
		  prt(out_val, 0, 0);
		  move_cursor_relative(y, x);
		  query = inkey_ex();
		  
		  /* Always stop at "normal" keys */
		  if ((query.key != '\r') && (query.key != '\n') && 
		      (query.key != ' ')) 
		    break;
		  
		  /* Sometimes stop at "space" key */
		  if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;
		  
		  /* Change the intro */
		  s2 = "also carrying ";
		}
	      
	      /* Double break */
	      if (this_o_idx) break;
	      
	      /* Use a preposition */
	      s2 = "on ";
	    }
	}
      
      
      /* Scan all objects in the grid */
      if (scan_floor(floor_list, &floor_num, y, x, 0x02))
	{				
	  /* Not boring */
	  boring = FALSE;
	  
	  while (1)
	    {
	      if (floor_num == 1)
		{
		  char o_name[120];
		  
		  object_type *o_ptr;
		  
		  /* Acquire object */
		  o_ptr = &o_list[floor_list[0]];
		  
		  /* Describe the object */
		  object_desc(o_name, o_ptr, TRUE, 3);
		  
		  /* Message */
		  sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
		}
	      else
		{
		  /* Message */
		  if (OPT(rogue_like_commands)) 
		    sprintf(out_val, "%s%s%sa pile of %d items [x,%s]",
			    s1, s2, s3, floor_num, info);
		  else sprintf(out_val, "%s%s%sa pile of %d items [l,%s]",
			       s1, s2, s3, floor_num, info);
		}
	      
	      prt(out_val, 0, 0);
	      move_cursor_relative(y, x);
	      
	      /* Command */
	      query = inkey_ex();
	      
	      /* Display list of items (query == "el", not "won") */
	      if ((floor_num > 1) && 
		  ((OPT(rogue_like_commands) ? (query.key == 'x') : 
		    (query.key == 'l')) || (query.key == ' ') || 
		   (query.key == '*') || (query.key == '?')))
		
		{
		  ui_event_data tmp = EVENT_EMPTY;

		  /* Save screen */
		  screen_save();
		  
		  /* Display */
		  show_floor(floor_list, floor_num);
		  
		  /* Prompt */
		  prt("Hit any key to continue", 0, 0);
		  
		  /* Wait */
		  tmp = inkey_ex();
		  
		  /* Load screen */
		  screen_load();
		}
	      else
		{
		  /* Stop */
		  break;
		}
	    }
	  
	  /* Stop */
	  break;
	}
      
      
      /* Scan all objects in the grid */
      for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
	  object_type *o_ptr;
	  
	  /* Acquire object */
	  o_ptr = &o_list[this_o_idx];
	  
	  /* Acquire next object */
	  next_o_idx = o_ptr->next_o_idx;
	  
	  /* Describe it */
	  if (o_ptr->marked)
	    {
	      char o_name[120];
	      
	      /* Not boring */
	      boring = FALSE;
	      
	      /* Obtain an object description */
	      object_desc(o_name, o_ptr, TRUE, 3);
	      
	      /* Describe the object */
	      sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
	      prt(out_val, 0, 0);
	      move_cursor_relative(y, x);
	      query = inkey_ex();
	      
	      /* Always stop at "normal" keys */
	      if ((query.key != '\r') && (query.key != '\n') && 
		  (query.key != ' ')) break;
	      
	      /* Sometimes stop at "space" key */
	      if ((query.key == ' ') && !(mode & (TARGET_LOOK))) break;
	      
	      /* Change the intro */
	      s1 = "It is ";
	      
	      /* Plurals */
	      if (o_ptr->number != 1) s1 = "They are ";
	      
	      /* Preposition */
	      s2 = "on ";
	    }
	}
      
      /* Double break */
      if (this_o_idx) break;
      
      
      /* Feature (apply "mimic") */
      feat = f_info[cave_feat[y][x]].mimic;
      
      /* Require knowledge about grid, or ability to see grid */
      if (!(cave_info[y][x] & (CAVE_MARK)) && !player_can_see_bold(y,x))
	{
	  /* Forget feature */
	  feat = FEAT_NONE;
	}
      
      /* Terrain feature if needed */
      if (boring || (feat > FEAT_INVIS))
	{
	  cptr name = f_name + f_info[feat].name;
	  
	  /* Hack -- handle unknown grids */
	  if (feat == FEAT_NONE) name = "unknown grid";
	  
	  /* Pick a prefix */
	  if (*s2 && (feat >= FEAT_DOOR_HEAD)) s2 = "in ";
	  
	  /* Pick proper indefinite article */
	  s3 = (is_a_vowel(name[0])) ? "an " : "a ";
	  
	  /* Hack -- special treatment for certain terrain features. */
	  if (feat >= FEAT_MIN_SPECIAL)
	    {
	      s3 = "";
	    }
	  
	  /* Hack -- special introduction for store doors */
	  if ((feat >= FEAT_SHOP_HEAD) && (feat <= FEAT_SHOP_TAIL))
	    {
	      s3 = "the entrance to the ";
	    }
	  
	  /* Hack - destination of surface paths */
	  if ((feat >= FEAT_LESS_NORTH) && (feat <= FEAT_MORE_WEST))
	    {
	      s4 = " to ";
	      s5 = locality_name[stage_map[stage_map[p_ptr->stage]
					   [NORTH + (feat - FEAT_LESS_NORTH)/2]]
				 [LOCALITY]];
	    }
	  else
	    {
	      s4 = "";
	      s5 = "";
	    }
	  
	  /* Display a message */
	  sprintf(out_val, "%s%s%s%s%s%s [%s]", 
		  s1, s2, s3, name, s4, s5, info);
	  prt(out_val, 0, 0);
	  move_cursor_relative(y, x);
	  query = inkey_ex();
	  
	  /* Always stop at "normal" keys */
	  if ((query.key != '\r') && (query.key != '\n') && 
	      (query.key != ' ')) break;
	}
      
      /* Stop on everything but "return" */
      if ((query.key != '\r') && (query.key != '\n')) break;
    }
  
  /* Keep going */
  return (query);
}


/**
 * Draw a visible path over the squares between (x1,y1) and (x2,y2).
 * The path consists of "*", which are white except where there is a
 * monster, object or feature in the grid.
 *
 * This routine has (at least) three weaknesses:
 * - remembered objects/walls which are no longer present are not shown,
 * - squares which (e.g.) the player has walked through in the dark are
 *   treated as unknown space.
 * - walls which appear strange due to hallucination aren't treated correctly.
 *
 * The first two result from information being lost from the dungeon arrays,
 * which requires changes elsewhere
 *
 * From NPPangband
 */
static int draw_path(u16b *path, char *c, byte *a,
    int y1, int x1, int y2, int x2)
{
  int i;
  int max;
  bool on_screen;
  char c1;
  byte a1;
  
  /* Find the path. */
  max = project_path(path, MAX_RANGE, y1, x1, y2, x2, PROJECT_THRU);
  
  /* No path, so do nothing. */
  if (max < 1) return 0;
  
  /* The starting square is never drawn, but notice if it is being
   * displayed. In theory, it could be the last such square.
   */
  on_screen = panel_contains(y1, x1);
  
  /* Draw the path. */
  for (i = 0; i < max; i++)
    {
      byte colour;

      /* Find the co-ordinates on the level. */
      int y = GRID_Y(path[i]);
      int x = GRID_X(path[i]);
      /*
       * As path[] is a straight line and the screen is oblong,
       * there is only section of path[] on-screen.
       * If the square being drawn is visible, this is part of it.
       * If none of it has been drawn, continue until some of it
       * is found or the last square is reached.
       * If some of it has been drawn, finish now as there are no
       * more visible squares to draw.
       *
       */
      if (panel_contains(y,x)) on_screen = TRUE;
      else if (on_screen) break;
      else continue;
      
      /* Find the position on-screen */
      move_cursor_relative(y,x);
      
      /* This square is being overwritten, so save the original. */
      Term_what(Term->scr->cx, Term->scr->cy, a+i, c+i);
      
      /* Choose a colour. */
      /* Visible monsters are red. */
      if (cave_m_idx[y][x] && m_list[cave_m_idx[y][x]].ml)
	{
	  colour = TERM_L_RED;
	}
      
      /* Known objects are yellow. */
      else if (cave_o_idx[y][x] && o_list[cave_o_idx[y][x]].marked)
	{
	  colour = TERM_YELLOW;
	}
      
      /* Known walls are blue. */
      else if (!cave_project(y,x) && (cave_info[y][x] & (CAVE_MARK) ||
					 player_can_see_bold(y,x)))
	{
	  colour = TERM_BLUE;
	}
      /* Unknown squares are grey. */
      else if (!(cave_info[y][x] & (CAVE_MARK)) && !player_can_see_bold(y,x))
	{
	  colour = TERM_L_DARK;
	}
      /* Unoccupied squares are white. */
      else
	{
	  colour = TERM_WHITE;
	}
      
      /* Hack - Obtain attr/char */
      a1 = misc_to_attr[0x30 + colour];
      c1 = misc_to_char[0x30 + colour];

      /* Draw the path segment */
      print_rel(c1, a1, y, x);
    }
  return i;
}

/**
 * Load the attr/char at each point along "path" which is on screen from
 * "a" and "c". This was saved in draw_path().
 *
 * From NPPangband
 */
static void load_path(int max, u16b *path, char *c, byte *a)
{
  int i;
  for (i = 0; i < max; i++)
    {
      if (!panel_contains(GRID_Y(path[i]), GRID_X(path[i]))) continue;
      
      move_cursor_relative(GRID_Y(path[i]), GRID_X(path[i]));
      
      (void)Term_addch(a[i], c[i]);
    }
  
  Term_fresh();
}


bool target_set_closest(int mode)
{
	int y, x, m_idx;
	monster_type *m_ptr;
	char m_name[80];
	bool visibility;

	/* Cancel old target */
	target_set_monster(0);

	/* Get ready to do targetting */
	target_set_interactive_prepare(mode);

	/* If nothing was prepared, then return */
	if (temp_n < 1)
	{
		msg_print("No Available Target.");
		return FALSE;
	}

	/* Find the first monster in the queue */
	y = temp_y[0];
	x = temp_x[0];
	m_idx = cave_m_idx[y][x];
	
	/* Target the monster, if possible */
	if ((m_idx <= 0) || !target_able(m_idx))
	{
		msg_print("No Available Target.");
		return FALSE;
	}

	/* Target the monster */
	m_ptr = &m_list[m_idx];
	monster_desc(m_name, m_ptr, 0x00);
	if (!(mode & TARGET_QUIET))
		msg_format("%^s is targeted.", m_name);
	Term_fresh();

	/* Set up target information */
	monster_race_track(m_ptr->r_idx);
	health_track(cave_m_idx[y][x]);
	target_set_monster(m_idx);

	/* Visual cue */
	Term_get_cursor(&visibility);
	(void)Term_set_cursor(TRUE);
	move_cursor_relative(y, x);
	Term_redraw_section(x, y, x, y);

	/* TODO: what's an appropriate amount of time to spend highlighting */
	Term_xtra(TERM_XTRA_DELAY, 150);
	(void)Term_set_cursor(visibility);

	return TRUE;
}

/**
 * Handle "target" and "look".
 *
 * Note that this code can be called from "get_aim_dir()".
 *
 * All locations must be on the current panel.  XXX XXX XXX
 *
 * Perhaps consider the possibility of "auto-scrolling" the screen
 * while the cursor moves around.  This may require dynamic updating
 * of the "temp" grid set.  XXX XXX XXX
 *
 * Hack -- targetting/observing an "outer border grid" may induce
 * problems, so this is not currently allowed.
 *
 * The player can use the direction keys to move among "interesting"
 * grids in a heuristic manner, or the "space", "+", and "-" keys to
 * move through the "interesting" grids in a sequential manner, or
 * can enter "location" mode, and use the direction keys to move one
 * grid at a time in any direction.  The "t" (set target) command will
 * only target a monster (as opposed to a location) if the monster is
 * target_able and the "interesting" mode is being used.
 *
 * The current grid is described using the "look" method above, and
 * a new command may be entered at any time, but note that if the
 * "TARGET_LOOK" bit flag is set (or if we are in "location" mode,
 * where "space" has no obvious meaning) then "space" will scan
 * through the description of the current grid until done, instead
 * of immediately jumping to the next "interesting" grid.  This
 * allows the "target" command to retain its old semantics.
 *
 * The "*", "+", and "-" keys may always be used to jump immediately
 * to the next (or previous) interesting grid, in the proper mode.
 *
 * The "return" key may always be used to scan through a complete
 * grid description (forever).
 *
 * This command will cancel any old target, even if used from
 * inside the "look" command.
 */
bool target_set_interactive(int mode)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int i, d, m;
  
  int y = py;
  int x = px;
  
  bool done = FALSE;
  
  bool flag = TRUE;

  bool failure_message = FALSE;

  ui_event_data query = EVENT_EMPTY;
  
  char info[80];
  
  /* These are used for displaying the path to the target */
  u16b *path = malloc(MAX_RANGE * sizeof(*path));
  char *path_char = malloc(MAX_RANGE * sizeof(*path_char));
  byte *path_attr = malloc(MAX_RANGE * sizeof(*path_attr));
  int max = 0;
  
  /* Cancel target */
  target_set_monster(0);
  
  /* Cancel tracking */
  /* health_track(0); */
  
  /*
   * Hack -- Start out by selecting any grid by using the TARGET_GRID
   * flag. I do this so dimen_door() is a bit nicer. -TNB-
   */
  if (mode & TARGET_GRID)
    {
      flag = FALSE;
      mode &= ~TARGET_GRID;
    }
  
  /* Prepare the "temp" array */
  target_set_interactive_prepare(mode);
  
  /* Start near the player */
  m = 0;
  
  /* Interact */
  while (!done)
    {
      /* Interesting grids */
      if (flag && temp_n)
	{
	  y = temp_y[m];
	  x = temp_x[m];
	  
	  /* Allow target */
	  if (((mode & TARGET_KILL) && (cave_m_idx[y][x] > 0) && 
	       target_able(cave_m_idx[y][x])) ||
	      ((mode & TARGET_OBJ) && (cave_o_idx[y][x] > 0) && 
	       target_able_obj(cave_o_idx[y][x])))
	    {
	      strcpy(info, "q,t,p,o,+,-,<dir>, ESC");
	    }
	  
	  /* Dis-allow target */
	  else
	    {
	      strcpy(info, "q,p,o,+,-,<dir>, ESC");
	    }
	  
	  /* Draw the path in "target" mode. If there is one */
	  if (mode & (TARGET_KILL | TARGET_OBJ))
	    max = draw_path (path, path_char, path_attr, py, px, y, x);
	  
	  /* Describe and Prompt */
	  query = target_set_interactive_aux(y, x, mode, info);
	  
	  /* Remove the path */
	  if (max > 0)	load_path (max, path, path_char, path_attr);
	  
	  /* Cancel tracking */
	  /* health_track(0); */
	  
	  /* Assume no "direction" */
	  d = 0;
	  
	  /* Analyze */
	  switch (query.key)
	    {
	    case ESCAPE:
	    case 'q':
	      {
		done = TRUE;
		break;
	      }
	      
	    case ' ':
	    case '*':
	    case '+':
	      {
		if (++m == temp_n)
		  {
		    m = 0;
		    if (!OPT(expand_list)) done = TRUE;
		  }
		break;
	      }

	    case '-':
	      {
		if (m-- == 0)
		  {
		    m = temp_n - 1;
		    if (!OPT(expand_list)) done = TRUE;
		  }
		break;
	      }
	      
	    case 'p':
	      {
		y = py;
		x = px;
	      }
	      
	    case 'o':
	      {
		flag = !flag;
		break;
	      }
	      
	    case 'm':
	      {
		break;
	      }
	      
	    case '\xff':
	      {
		x = KEY_GRID_X(query);
		y = KEY_GRID_Y(query);
		if (!query.mousey)
		  {
		    done = TRUE;
		    break;
		  }
		else if (!(mode & (TARGET_KILL | TARGET_OBJ)))
		  {
		    /*		    target_set_location(y, x);
				    done = TRUE;*/
		    flag = FALSE;
		    break;
		  }
		else;
		/* Fall through */
	      }
	    

	    case 't':
	    case '5':
	    case '0':
	    case '.':
	      {
		if (mode & TARGET_KILL)
		  {
		    int m_idx = cave_m_idx[y][x];
		    
		    if ((m_idx > 0) && target_able(m_idx))
		      {
			health_track(m_idx);
			target_set_monster(m_idx);
			done = TRUE;
		      }
		    else
		      {
			bell("Illegal target!");
		      }
		  }
		else if (mode & TARGET_OBJ)
		  {
		    int o_idx = cave_o_idx[y][x];
		    
		    if ((o_idx > 0) && target_able_obj(o_idx))
		      {
			target_set_object(o_idx);
			done = TRUE;
		      }
		    else
		      {
			bell("Illegal target!");
		      }
		  }
		break;
	      }
	      
	    default:
	      {
		/* Extract direction */
		d = target_dir(query.key);
		
		/* Oops */
		if (!d) bell("Illegal command for target mode!");
		
		break;
	      }
	    }
	  
	  /* Hack -- move around */
	  if (d)
	    {
	      /* Modified to scroll to monster */
	      int y2 = panel_row_min;
	      int x2 = panel_col_min;
	      
	      /* Find a new monster/object */
	      i = target_pick(temp_y[m], temp_x[m], ddy[d], ddx[d]);
	      
	      /* Request to target past last interesting grid */
	      while (i < 0)
		{
		  /* Note the change */
		  if (change_panel(ddy[d], ddx[d]))
		    {
		      int v = temp_y[m];
		      int u = temp_x[m];
		      
		      /* Recalculate interesting grids */
		      target_set_interactive_prepare(mode);
		      
		      /* Look at interesting grids */
		      flag = TRUE;
		      
		      /* Find a new monster */
		      i = target_pick(v, u, ddy[d], ddx[d]);
		      
		      /* Use that grid */
		      if (i >= 0) m = i;
		    }
		  
		  /* Nothing interesting */
		  else
		    {
		      /* Restore previous position */
		      panel_row_min = y2;
		      panel_col_min = x2;
		      panel_recalc_bounds();
		      
		      /* Update stuff */
		      p_ptr->update |= (PU_MONSTERS);
		      
		      /* Redraw map */
		      p_ptr->redraw |= (PR_MAP);
		      
		      /* Window stuff */
		      p_ptr->window |= (PW_OVERHEAD);
		      
		      /* Handle stuff */
		      handle_stuff();
		      
		      /* Recalculate interesting grids */
		      target_set_interactive_prepare(mode);
		      
		      /* Done */
		      break;
		    }
		}
	      
	      /* Use that grid */
	      if (i >= 0) m = i;
	    }
	}

      /* Objects need a specific target */
      else if (mode & TARGET_OBJ)
	{
	  done = TRUE;
	  failure_message = TRUE;
	}
      
      /* Arbitrary grids */
      else
	{
	  /* Default prompt */
	  strcpy(info, "q,t,p,m,+,-,<dir>, ESC");
	  
	  /* Draw the path in "target" mode. If there is one */
	  if (mode & (TARGET_KILL))
	    max = draw_path (path, path_char, path_attr, py, px, y, x);
	  
	  /* Describe and Prompt (enable "TARGET_LOOK") */
	  query = target_set_interactive_aux(y, x, mode | TARGET_LOOK, info);
	  
	  /* Remove the path */
	  if (max > 0)	load_path (max, path, path_char, path_attr);
	  
	  /* Cancel tracking */
	  /* health_track(0); */

	  /* Assume no direction */
	  d = 0;
	  
	  /* Analyze the keypress */
	  switch (query.key)
	    {
	    case ESCAPE:
	    case 'q':
	      {
		done = TRUE;
		break;
	      }
	      
	    case ' ':
	    case '*':
	    case '+':
	    case '-':
	      {
		break;
	      }
	      
	    case 'p':
	      {
		y = py;
		x = px;
	      }
	      
	    case 'o':
	      {
		break;
	      }
	      
	    case 'm':
	      {
		flag = !flag;
		break;
	      }
 
	    case '\xff':
	      {
		/* Escape */
		if (!query.mousey)
		  {
		    done = TRUE;
		  }
		/* Double click selects target */
		else if ((x == KEY_GRID_X(query)) && (y == KEY_GRID_Y(query)))
		  {
		    target_set_location(y, x);
		    done = TRUE;
		    break;
		  }      
		/* Look here */
		else
		  {
		    if (query.mousex > COL_MAP)
		      {
			x = KEY_GRID_X(query);
			y = KEY_GRID_Y(query);
			flag = FALSE;
		      }
		  }
		break;
	      }
	      
	    case 't':
	    case '5':
	    case '0':
	      {
		target_set_location(y, x);
		done = TRUE;
		break;
	      }
	      
	    default:
	      {
		/* Extract a direction */
		d = target_dir(query.key);
		
		/* Oops */
		if (!d) bell("Illegal command for target mode!");
		
		break;
	      }
	    }
	  
	  /* Handle "direction" */
	  if (d)
	    {
	      int dx = ddx[d];
	      int dy = ddy[d];
	      
	      /* Hack to stop looking outside town walls */
	      if (!p_ptr->depth)
		{
		  if (cave_feat[y + dy][x] == FEAT_PERM_SOLID)
		    dy = 0;
		  if (cave_feat[y][x + dx] == FEAT_PERM_SOLID)
		    dx = 0;
		}

	      /* Move */
	      x += dx;
	      y += dy;
	      
	      /* Do not move horizontally if unnecessary */
	      if (((x < panel_col_min + SCREEN_WID / 2) && (dx > 0)) ||
		  ((x > panel_col_min + SCREEN_WID / 2) && (dx < 0)))
		{
		  dx = 0;
		}
	      
	      /* Do not move vertically if unnecessary */
	      if (((y < panel_row_min + SCREEN_HGT / 2) && (dy > 0)) ||
		  ((y > panel_row_min + SCREEN_HGT / 2) && (dy < 0)))
		{
		  dy = 0;
		}
	      
	      /* Apply the motion */
	      if ((y >= panel_row_min + SCREEN_HGT) || (y < panel_row_min) ||
		  (x >= panel_col_min + SCREEN_WID) || (x < panel_col_min))
		{
		  if (change_panel(dy, dx)) 
		    target_set_interactive_prepare(mode);
		}
	      
	      
	      /* Slide into legality */
	      if (x <= 0) x = 0 + 1;
	      else if (x >= DUNGEON_WID - 1) x = DUNGEON_WID - 2;
	      
	      /* Slide into legality */
	      if (y <= 0) y = 0 + 1;
	      else if (y >= DUNGEON_HGT - 1) y = DUNGEON_HGT - 2;
	    }
	}
    }

  /* Forget */
  temp_n = 0;
  
  /* Clear the top line */
  prt("", 0, 0);
  
  /* Recenter the map around the player */
  verify_panel();

  /* Update stuff */
  p_ptr->update |= (PU_MONSTERS);
  
  /* Redraw map */
  p_ptr->redraw |= (PR_MAP);
  
  /* Window stuff */
  p_ptr->window |= (PW_OVERHEAD);
  
  /* Handle stuff */
  handle_stuff();
  
  free(path);
  free(path_char);
  free(path_attr);

  /* Failure to set target */
  if (!p_ptr->target_set) 
    {
      if (failure_message) msg_print("There is nothing within reach.");
      return (FALSE);
    }

  /* Success */
  return (TRUE);
}

/**
 * Given a starting position, find the 'n'th closest monster.
 *
 * Note:  "require_visible" only works when this function is looking around
 * the character.
 *
 * Set ty and tx to zero on failure.
 */
void get_closest_los_monster(int n, int y0, int x0, int *ty, int *tx,
			     bool require_visible)
{
  monster_type *m_ptr;
  
  int i, j;
  int r_idx;
  int dist = 100;
  
  int *monster_dist;
  int *monster_index;
  int monster_count = 0;
  
  bool use_view = FALSE;
  
  /* Allocate some arrays */
  monster_dist = C_ZNEW(m_max, int);
  monster_index = C_ZNEW(m_max, int);
  
  /* Note that we're looking from the character's grid */
  if ((y0 == p_ptr->py) && (x0 == p_ptr->px)) use_view = TRUE;
  
  /* Reset target grids */
  *ty = 0;  *tx = 0;
  
  /* N, as input, goes from 1+.  Map it to 0+ for table access */
  if (n > 0) n--;
  
  
  /* Check all the monsters */
  for (i = 1; i < m_max; i++)
    {
      /* Get the monster */
      m_ptr = &m_list[i];
      
      /* Paranoia -- skip "dead" monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Check for visibility */
      if (require_visible)
	{
	  if (!m_ptr->ml) continue;
	}
      
      /* Use CAVE_VIEW information (fast way) */
      if (use_view)
	{
	  if (!(cave_info[m_ptr->fy][m_ptr->fx] & (CAVE_VIEW))) continue;
	  
	  /* Get stored distance */
	  dist = m_ptr->cdis;
	}
      
      /* Monster must be in los from the starting position (slower way) */
      else
	{
	  /* Get distance from starting position */
	  dist = distance(y0, x0, m_ptr->fy, m_ptr->fx);
	  
	  /* Monster location must be within range */
	  if (dist > MAX_SIGHT) continue;
	  
	  /* Require line of sight */
	  if (!los(y0, x0, m_ptr->fy, m_ptr->fx)) continue;
	}
      
      /* Remember this monster */
      monster_dist[monster_count] = dist;
      monster_index[monster_count++] = i;
    }

  /* Not enough monsters found */
  if (monster_count <= n)
    {
      /* Free some arrays */
      FREE(monster_dist);
      FREE(monster_index);
      
      return;
    }
  

  /* Sort the monsters in ascending order of distance */
  for (i = 0; i < monster_count - 1; i++)
    {
      for (j = 0; j < monster_count - 1; j++)
	{
	  int this_dist = monster_dist[j];
	  int next_dist = monster_dist[j + 1];
	  
	  /* Bubble sort */
	  if (this_dist > next_dist)
	    {
	      int tmp_dist  = monster_dist[j];
	      int tmp_index = monster_index[j];
	      
	      monster_dist[j] = monster_dist[j + 1];
	      monster_dist[j + 1] = tmp_dist;
	      
	      monster_index[j] = monster_index[j + 1];
	      monster_index[j + 1] = tmp_index;
	    }
	}
    }
  

  /* Get the nth closest monster's index */
  r_idx = monster_index[n];
  
  /* Get the monster */
  m_ptr = &m_list[r_idx];
  
  /* Set the target to its location */
  *ty = m_ptr->fy;
  *tx = m_ptr->fx;
  
  /* Free some arrays */
  FREE(monster_dist);
  FREE(monster_index);
}



/**
 * Get an "aiming direction" (1,2,3,4,6,7,8,9 or 5) from the user.
 *
 * Return TRUE if a direction was chosen, otherwise return FALSE.
 *
 * The direction "5" is special, and means "use current target".
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", if it is set.
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Currently this function applies confusion directly.
 */
bool get_aim_dir(int *dp)
{
  int dir;
  
  ui_event_data ke = EVENT_EMPTY;
  
  cptr p;
  
  if (repeat_pull(dp))
    {
      /* Verify */
      if ((*dp > 0) && (*dp < 10) && (!(*dp == 5 && !target_okay())))
	{
	  return (TRUE);
	}
      
      /* Invalid repeat - reset it */
      else repeat_clear();
    }
  
  /* Initialize */
  (*dp) = 0;
  
  /* Global direction */
  dir = p_ptr->command_dir;
  
  /* Hack -- auto-target if requested */
  if (OPT(use_old_target) && target_okay()) dir = 5;
  
  /* Make some buttons */
  button_add("*", '*');
  button_add(".",'.');
  if (target_okay())
    button_add("5", '5');

  /* Ask until satisfied */
  while (!dir)
    {

      /* Choose a prompt */
      if (!target_okay())
	{
	  p = "Direction ('*' choose target; '.' closest; ESC)?";
	}
      else
	{
	  if (small_screen)
	    p = "Dir('5' target;'*' re-target;'.' closest; ESC)?";
	  else
	    p = "Direction ('5' target; '*' re-target; '.' closest; ESC)?";
	}
      
      /* Get a command (or Cancel) */
      if (!get_com_ex(p, &ke)) break;

      /* Analyze */
      switch (ke.key)
	{
	  /* Mouse aiming */
	case '\xff':
	  {
	    int y = KEY_GRID_Y(ke);
	    int x = KEY_GRID_X(ke);
	    
	    /* Monster */
	    if (cave_m_idx[y][x] > 0)
	      {
		/* Get monster index */
		int m_idx = cave_m_idx[y][x];
		    
		/* Monster must be in line of fire */
		if (target_able(m_idx))
		  {
		    health_track(m_idx);
		    target_set_monster(m_idx);
		    dir = 5;
		    break;
		  }
	      }

	    /* Next to player */
	    else if ((ABS(p_ptr->py - y) <= 1) && (ABS(p_ptr->px - x) <= 1)) 
	      {
		/* Get the direction */
		for (dir = 0; dir < 10; dir++)
		  if ((ddx[dir] == (x - p_ptr->px)) && 
		      (ddy[dir] == (y - p_ptr->py))) 
		    break;

		/* Take it */
		break;
	      }
	    
	    
	    /* No monster */
	    else
	      {
		target_set_location(y, x);
		dir = 5;
		break;
	      }
	  }
	  
	  /* Set new target, use target if legal */
	case '*':
	  {
	    if (target_set_interactive(TARGET_KILL)) dir = 5;
	    break;
	  }
	  
	  /* Target the closest visible monster in line of fire */
	case '.':
	  {
	    int n = 0;
	    
	    /* Check closest monsters */
	    while (TRUE)
	      {
		int y = 0, x = 0;
		
		/* Get next monster */
		n++;
		
		/* Find the 'n'th closest viewable, visible monster */
		get_closest_los_monster(n, p_ptr->py, p_ptr->px, &y, &x, TRUE);
		
		/* We have a valid target */
		if ((y) && (x) && (cave_m_idx[y][x] > 0))
		  {
		    /* Get monster index */
		    int m_idx = cave_m_idx[y][x];
		    
		    /* Monster must be in line of fire */
		    if (target_able(m_idx))
		      {
			health_track(m_idx);
			target_set_monster(m_idx);
			dir = 5;
			break;
		      }
		  }
		
		/* We've run out of targetable monsters */
		else
		  {
		    bell("No targetable monsters!");
		    break;
		  }
	      }
	    break;
	  }
	  
	  /* Use current target, if set and legal */
	case 't':
	case '5':
	case '0':
	  {
	    if (target_okay()) dir = 5;
	    break;
	  }
	  
	  /* Possible direction */
	default:
	  {
	    dir = target_dir(ke.key);
	    break;
	  }
	}
      
      /* Error */
      if (!dir) bell("Illegal aim direction!");
    }

  /* Clear buttons */  
  button_kill('5');
  button_kill('*');
  button_kill('.');

  /* No direction */
  if (!dir) return (FALSE);
  
  /* Save the direction */
  p_ptr->command_dir = dir;
  
  /* Check for confusion */
  if (p_ptr->timed[TMD_CONFUSED])
    {
      /* Random direction */
      dir = ddd[randint0(8)];
    }
  
  /* Notice confusion */
  if (p_ptr->command_dir != dir)
    {
      /* Warn the user */
      msg_print("You are confused.");
    }
  
  /* Save direction */
  (*dp) = dir;
  
  repeat_push(dir);

  /* A "valid" direction was entered */
  return (TRUE);
}



/**
 * Request a "movement" direction (1,2,3,4,6,7,8,9) from the user.
 *
 * Return TRUE if a direction was chosen, otherwise return FALSE.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.
 *
 * Directions "5" and "0" are illegal and will not be accepted.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", if it is set.
 */
bool get_rep_dir(int *dp)
{
  int dir;
  
  ui_event_data ke = EVENT_EMPTY;
  
  cptr p;
  
  if (repeat_pull(dp))
    {
      return (TRUE);
    }
  
  /* Initialize */
  (*dp) = 0;
  
  /* Global direction */
  dir = p_ptr->command_dir;
  
  /* Get a direction */
  while (!dir)
    {
      /* Choose a prompt */
      p = "Direction (Escape to cancel)? ";
      
      /* Get a command (or Cancel) */
      if (!get_com_ex(p, &ke)) break;
      
      /* Convert keypress into a direction */
      if (ke.key == '\xff')
	dir = mouse_dir(ke, FALSE);
      else
	dir = target_dir(ke.key);
      
      /* Oops */
      if (!dir) bell("Illegal repeatable direction!");
    }
  
  /* Aborted */
  if (!dir) return (FALSE);
  
  /* Save desired direction */
  p_ptr->command_dir = dir;
  
  /* Save direction */
  (*dp) = dir;
  
  repeat_push(dir);
  
  /* Success */
  return (TRUE);
}


/**
 * Apply confusion, if needed, to a direction
 *
 * Display a message and return TRUE if direction changes.
 */
bool confuse_dir(int *dp)
{
  int dir;
  
  /* Default */
  dir = (*dp);
  
  /* Apply "confusion" */
  if (p_ptr->timed[TMD_CONFUSED])
    {
      /* Apply confusion XXX XXX XXX */
      if ((dir == 5) || (randint0(100) < 75))
	{
	  /* Random direction */
	  dir = ddd[randint0(8)];
	}
    }
  
  /* Notice confusion */
  if ((*dp) != dir)
    {
      /* Warn the user */
      msg_print("You are confused.");
      
      /* Save direction */
      (*dp) = dir;
      
      /* Confused */
      return (TRUE);
    }
  
  /* Not confused */
  return (FALSE);
}

