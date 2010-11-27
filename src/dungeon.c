/** \file dungeon.c 
    \brief Turn-by-turn processing

 * Char & monster regeneration, town and dungeon management,
 * all timed character, monster, and object states, entry into Wizard, 
 * debug, and borg mode, definitions of user commands, process player, 
 * the basic function for interacting with the dungeon (including what 
 * happens when a wizard cheats death).
 *
 * Copyright (c) 1997-2009 Nick McConnell, Ben Harrison, James E. Wilson, 
 * Robert A. Koeneke
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
#include "z-file.h"
#include "cmds.h"



/**
 * Regenerate hit points
 */
static void regenhp(int percent)
{
  s32b new_chp, new_chp_frac;
  int old_chp;
  
  /* Save the old hitpoints */
  old_chp = p_ptr->chp;
  
  /* Extract the new hitpoints */
  new_chp = ((long)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
  p_ptr->chp += (s16b)(new_chp >> 16);   /* div 65536 */
  
  /* check for overflow */
  if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;
  new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
  if (new_chp_frac >= 0x10000L)
    {
      p_ptr->chp_frac = (u16b)(new_chp_frac - 0x10000L);
      p_ptr->chp++;
    }
  else
    {
      p_ptr->chp_frac = (u16b)(new_chp_frac);
    }
  
  /* Fully healed */
  if (p_ptr->chp >= p_ptr->mhp)
    {
      p_ptr->chp = p_ptr->mhp;
      p_ptr->chp_frac = 0;
    }
  
  /* Notice changes */
  if (old_chp != p_ptr->chp)
    {
      /* Redraw */
      p_ptr->redraw |= (PR_HP);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
    }
}


/**
 * Regenerate mana points
 */
static void regenmana(int percent)
{
  s32b new_mana, new_mana_frac;
  int old_csp;
  
  old_csp = p_ptr->csp;
  new_mana = ((long)p_ptr->msp) * percent + PY_REGEN_MNBASE;
  p_ptr->csp += (s16b)(new_mana >> 16);	/* div 65536 */

  /* check for overflow */
  if ((p_ptr->csp < 0) && (old_csp > 0))
    {
      p_ptr->csp = MAX_SHORT;
    }
  new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
  if (new_mana_frac >= 0x10000L)
    {
      p_ptr->csp_frac = (u16b)(new_mana_frac - 0x10000L);
      p_ptr->csp++;
    }
  else
    {
      p_ptr->csp_frac = (u16b)new_mana_frac;
    }
  
  /* Must set frac to zero even if equal */
  if (p_ptr->csp >= p_ptr->msp)
    {
      p_ptr->csp = p_ptr->msp;
      p_ptr->csp_frac = 0;
    }
  
  /* Redraw mana */
  if (old_csp != p_ptr->csp)
    {
      /* Redraw */
      p_ptr->redraw |= (PR_MANA);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
    }
}

/**
 * If player has inscribed the object with "!!", let him know when it's 
 * recharged. -LM-
 */
static void recharged_notice(object_type *o_ptr)
{
  char o_name[120];
  
  cptr s;
  
  /* No inscription */
  if (!o_ptr->note) return;
  
  /* Find a '!' */
  s = strchr(quark_str(o_ptr->note), '!');
  
  /* Process notification request. */
  while (s)
    {
      /* Find another '!' */
      if (s[1] == '!')
        {
          /* Describe (briefly) */
          object_desc(o_name, o_ptr, FALSE, 0);
          
          /* Notify the player */
          if (o_ptr->number > 1) 
            msg_format("Your %s are recharged.", o_name);
          else msg_format("Your %s is recharged.", o_name);

          /* Disturb the player */
          disturb(0, 0);
          
          /* Done. */
          return;
        }
      
      /* Keep looking for '!'s */
      s = strchr(s + 1, '!');
    }
}

/**
 * Remove light-sensitive monsters from sunlt areas
 */
void sun_banish(void)
{
  bool some_gone = FALSE;
  
  monster_type *m_ptr;
  monster_race *r_ptr;
  
  int i;
  
  /* Process all monsters */
  for (i = 1; i < m_max - 1; i++)
    {
      /* Access the monster */
      m_ptr = &m_list[i];
      
      
      /* Check if it hates light */
      r_ptr = &r_info[m_ptr->r_idx];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Hack -- Skip Unique Monsters */
      if (r_ptr->flags1 & (RF1_UNIQUE)) continue;
      
      /* Skip monsters not hurt by light */
      if (!(r_ptr->flags3 & (RF3_HURT_LITE))) continue;
      
      /* Banish */
      delete_monster_idx(i);
      some_gone = TRUE; 
    }
  
  process_monsters(0);
  if (some_gone)
    msg_print("Creatures of the darkness flee from the sunlight!");
}


static void play_ambient_sound(void)
{
  /* Town sound */
  if (p_ptr->depth == 0) 
    {
      /* Hack - is it daytime or nighttime? */
      if (turn % (10L * TOWN_DAWN) < TOWN_DAWN / 2)
	{
	  /* It's day. */
	  sound(MSG_AMBIENT_DAY);
	} 
      else 
	{
	  /* It's night. */
	  sound(MSG_AMBIENT_NITE);
	}
      
    }
  
  /* Dungeon level 1-20 */
  else if (p_ptr->depth <= 20) 
    {
      sound(MSG_AMBIENT_DNG1);
    }
  
  /* Dungeon level 21-40 */
  else if (p_ptr->depth <= 40) 
    {
      sound(MSG_AMBIENT_DNG2);
    }
  
  /* Dungeon level 41-60 */
  else if (p_ptr->depth <= 60) 
    {
      sound(MSG_AMBIENT_DNG3);
    }
  
  /* Dungeon level 61-80 */
  else if (p_ptr->depth <= 80) 
    {
      sound(MSG_AMBIENT_DNG4);
    }
  
  /* Dungeon level 80- */
  else  
    {
      sound(MSG_AMBIENT_DNG5);
    }
}

/**
 * Handle certain things once every 10 game turns.
 */
static void process_world(void)
{
  int i, j, temp;
  
  int regen_amount, mana_regen_amount, chance;
  int plev = p_ptr->lev;
  object_type *o_ptr;
  object_kind *k_ptr;
  
  bool was_ghost=FALSE;
  
  bool extend_magic = FALSE;
  
  bool divine = (check_ability(SP_DIVINE));
  bool hardy = (check_ability(SP_HARDY));
  
  bool dawn;
  
  /* Every 10 game turns */
  if (turn % 10) return;
  
  /* Play an ambient sound at regular intervals. */
  if (!(turn % ((10L * TOWN_DAWN) / 4)))
    {
      play_ambient_sound();
    }
  
  /* Hack - beneficial effects timeout at 2/3 speed with ENHANCE_MAGIC */
  if ((check_ability(SP_ENHANCE_MAGIC)) && ((turn/10) % EXTEND_MAGIC_FRACTION))
    extend_magic = TRUE;
  
  /*** Check the Time ***/
  
  if (!(turn % 1000))
    {
      /* Check time */
      if (0 != check_time())
        {
          /* Warning */
          if (closing_flag <= 2)
            {
              /* Disturb */
              disturb(0, 0);
              
              /* Count warnings */
              closing_flag++;
              
              /* Message */
              msg_print("The gates to ANGBAND are closing...");
              msg_print("Please finish up and/or save your game.");
            }
          
          /* Slam the gate */
          else
            {
              /* Message */
              msg_print("The gates to ANGBAND are now closed.");
              
              /* Stop playing */
              p_ptr->playing = FALSE;
              
              /* Leaving */
              p_ptr->leaving = TRUE;
            }
        }
    }
  
  /*** Attempt timed autosave.  From Zangband. ***/
  if (autosave && autosave_freq)
    {
      if (!(turn % ((s32b) autosave_freq * 10 )))
        {
          is_autosave = TRUE;
          do_cmd_save_game();
          is_autosave = FALSE;
          msg_print(NULL);
        }
    }
  
  /*** Handle the "outside" (stores and sunshine) ***/
  
  /* Determine daylight */
  is_daylight = (((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)) && 
		 (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) &&
		 (stage_map[p_ptr->stage][STAGE_TYPE] != VALLEY) &&
		 ((p_ptr->stage < 151) || (p_ptr->stage > 153)));
    
  /* While not in cave, including cave towns */
  if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) &&
      (stage_map[p_ptr->stage][STAGE_TYPE] != VALLEY) &&
      ((p_ptr->stage < 151) || (p_ptr->stage > 153)))
    {
      /* Hack -- Daybreak/Nightfall outside */
      if (!(turn % ((10L * TOWN_DAWN) / 2)))
        {
          /* Check for dawn */
          dawn = (!(turn % (10L * TOWN_DAWN)));
          
          /* Day breaks */
          if (dawn)
            {
              /* Message */
              msg_print("The sun has risen.");
              sun_banish(); 
              if (p_ptr->schange == SHAPE_VAMPIRE)
                shapechange(SHAPE_NORMAL);
	      update_view();
            }
          
          /* Night falls */
          else
            {
              /* Message */
              msg_print("The sun has set.");
	      update_view();
            }
          
          /* Illuminate */
          if (!p_ptr->depth)
            town_illuminate(dawn, FALSE);
        }
    }
  
  
  /* Update the stores once a day (while out of town) */
  if (p_ptr->depth)
    if (!(turn % (10L * STORE_TURNS))) stores_maint(1);
  
  
  /*** Process the monsters ***/
  
  /* Hack - see if there is already a player ghost on the level */
  if (bones_selector) was_ghost=TRUE;

  /* Check for creature generation, except on themed levels */
  if ((rand_int(MAX_M_ALLOC_CHANCE) == 0) && (!p_ptr->themed_level))
    {
      int n;
      
      /* Hack for small towns */
      if (!p_ptr->depth)
        n = MAX_SIGHT - 5;
      else
        n = MAX_SIGHT + 5;
      
      /* Make a new monster */
      (void)alloc_monster(n, FALSE, FALSE);
    }
  
  /* Hack - if there is a ghost now, and there was not before, 
   * give a challenge */
  if ((bones_selector) && (!(was_ghost))) ghost_challenge();
  
  /*** Damage over Time ***/
  
  /* Take damage from poison */
  if (p_ptr->poisoned)
    {
      /* Take damage */
      take_hit(randint(p_ptr->poisoned > 300 ? 20 : 
                       (p_ptr->poisoned + 14) / 15), "poison");
    }
  
  /* Take damage from cuts */
  if (p_ptr->cut)
    {
      /* Mortal wound or Deep Gash */
      if (p_ptr->cut > 200)
        {
          i = 3;
        }
      
      /* Severe cut */
      else if (p_ptr->cut > 100)
        {
          i = 2;
        }
      
      /* Other cuts */
      else
        {
          i = 1;
        }
      
      /* Take damage */
      take_hit(i, "a fatal wound");
    }
  
  
  /*** Check the Food, and Regenerate ***/
  
  /* Digest normally */
  if (p_ptr->food < PY_FOOD_MAX)
    {
      /* Every 100 game turns */
      if (!(turn % 100))
	{
	  /* Basic digestion rate based on speed */
	  i = extract_energy[p_ptr->pspeed] * 2;
          
          /* Regeneration takes more food */
          if (p_ptr->regenerate) i += 30;
          
	  /* Fast digestion takes more food */
	  if (p_ptr->fast_digest) 
	    {
	      notice_curse(CF_HUNGRY, 0);
	      i += 20;
	    }
	  
	  /* Slow digestion takes less food */
	  if (p_ptr->slow_digest) 
	    {
	      notice_obj(OF_SLOW_DIGEST, 0);
	      i -= 10;
	    }
	  
	  /* Minimal digestion */
	  if (i < 1) i = 1;
	  
	  /* Digest some food */
	  (void)set_food(p_ptr->food - i);
	}
    }
  
  /* Digest quickly when gorged */
  else
    {
      /* Digest a lot of food */
      (void)set_food(p_ptr->food - 100);
    }
  
  /* Starve to death (slowly) */
  if (p_ptr->food < PY_FOOD_STARVE)
    {
      /* Calculate damage */
      i = (PY_FOOD_STARVE - p_ptr->food) / 10;
      
      /* Take damage */
      take_hit(i, "starvation");
    }
  
  /* Default regeneration */
  regen_amount = PY_REGEN_NORMAL;
  
  /* Getting Weak */
  if (p_ptr->food < PY_FOOD_WEAK)
    {
      /* Lower regeneration */
      if (p_ptr->food < PY_FOOD_STARVE)
	{
	  regen_amount = 0;
	}
      else if (p_ptr->food < PY_FOOD_FAINT)
	{
	  regen_amount = PY_REGEN_FAINT;
	}
      else
	{
	  regen_amount = PY_REGEN_WEAK;
	}
      
      /* Getting Faint */
      if (p_ptr->food < PY_FOOD_FAINT)
        {
          /* Faint occasionally */
          if (!p_ptr->paralyzed && (rand_int(100) < 10))
            {
              /* Message */
              msg_print("You faint from the lack of food.");
              disturb(1, 0);
              
              /* Hack -- faint (bypass free action) */
              (void)set_paralyzed(p_ptr->paralyzed + 1 + rand_int(5));
            }
        }
    }
  
  
  /* Searching or Resting */
  if (p_ptr->searching || p_ptr->resting)
    {
      regen_amount = regen_amount * 2;
    }
  
  /* Regeneration ability.  A lesser effect on mana in Oangband. */
  if (p_ptr->regenerate)
    {
      notice_obj(OF_REGEN, 0);
      regen_amount = regen_amount * 2;
      mana_regen_amount = 3 * regen_amount / 2;
    }
  
  /* Otherwise, the basic mana regen is the same as that of HPs. */
  else  mana_regen_amount = regen_amount;
  
  /* Consider specialty abilities */
  if (check_ability(SP_REGENERATION)) regen_amount *= 2;
  if (check_ability(SP_MEDITATION)) mana_regen_amount *= 2;
  
  /* Regenerate the mana */
  if (p_ptr->csp < p_ptr->msp)
    {
      regenmana(mana_regen_amount);
    }
  
  /* Various things interfere with healing */
  if (p_ptr->paralyzed) regen_amount = 0;
  if (p_ptr->poisoned) regen_amount = 0;
  if (p_ptr->stun) regen_amount = 0;
  if (p_ptr->cut) regen_amount = 0;
  if (p_ptr->slow_regen) 
    {
      regen_amount /= 2;
      notice_curse(CF_SLOW_REGEN, 0);
    }
  
  /* Regenerate Hit Points if needed */
  if (p_ptr->chp < p_ptr->mhp)
	{
	  regenhp(regen_amount);
	}

  
  /*** Timeout Various Things ***/
  
  /* Hack -- Hallucinating */
  if (p_ptr->image)
    {
      /* Maiar recover quickly from anything. */
      if (divine)
        (void)set_image(p_ptr->image - 2);
      else (void)set_image(p_ptr->image - 1);
    }
  
  /* Blindness */
  if (p_ptr->blind)
    {
      /* Maiar recover quickly from anything. */
      if (divine)
        (void)set_blind(p_ptr->blind - 2);
      
      else (void)set_blind(p_ptr->blind - 1);
    }
  
  /* Timed see-invisible */
  if ((p_ptr->tim_invis) && (!extend_magic))
    {
      (void)set_tim_invis(p_ptr->tim_invis - 1);
    }
  
  /* Timed Telepathy */
  if ((p_ptr->tim_esp) && (!extend_magic))
    {
      (void)set_tim_esp(p_ptr->tim_esp - 1);
    }
  
  /* Timed near-complete stealth -LM- */
  if ((p_ptr->superstealth) && (!extend_magic))
    {
	  if (!((check_ability(SP_WOODEN)) && 
            (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags & TF_TREE)))
	  {
        (void)set_superstealth(p_ptr->superstealth - 1,TRUE);
      
        /* Warn the player that he's going to be revealed soon. */
        if (p_ptr->superstealth == 5) msg_print("You sense your mantle of shadow fading...");
	  }
    }
  
  /* Timed temporary elemental brands. -LM- */
  if ((p_ptr->ele_attack) && (!extend_magic))
    {
      p_ptr->ele_attack--;
      
      /* Clear all temporary elemental brands. */
      if (!p_ptr->ele_attack) set_ele_attack(0, 0);
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATUS);
      
      /* Handle stuff */
      handle_stuff();
    }
  
  /* Timed infra-vision */
  if ((p_ptr->tim_infra) && (!extend_magic))
    {
      (void)set_tim_infra(p_ptr->tim_infra - 1);
    }
  
  /* Paralysis */
  if (p_ptr->paralyzed)
    {
      /* Maiar recover quickly from anything. */
      if (divine)
        (void)set_paralyzed(p_ptr->paralyzed - 2);
      
      else (void)set_paralyzed(p_ptr->paralyzed - 1);
    }
  
  /* Confusion */
  if (p_ptr->confused)
    {
      /* Maiar recover quickly from anything. */
      if (divine)
        (void)set_confused(p_ptr->confused - 2);
      
      else (void)set_confused(p_ptr->confused - 1);
    }
  
  /* Afraid */
  if (p_ptr->afraid && !(p_ptr->fear))
    {
      /* Maiar recover quickly from anything. */
      if (divine)
        (void)set_afraid(p_ptr->afraid - 2);
      
      else (void)set_afraid(p_ptr->afraid - 1);
    }

  /* Permanent fear */
  else if (p_ptr->fear) 
    {
      p_ptr->afraid = 1;
      notice_curse(CF_AFRAID, 0);
    }
  
  /* Fast */
  if ((p_ptr->fast) && (!extend_magic))
    {
      (void)set_fast(p_ptr->fast - 1);
    }
  
  /* Slow */
  if (p_ptr->slow)
    {
      /* Maiar recover quickly from anything. */
      if (divine)
        (void)set_slow(p_ptr->slow - 2);
      
      else (void)set_slow(p_ptr->slow - 1);
    }
  
  /* Protection from evil. */
  if ((p_ptr->protevil) && (!extend_magic))
    {
      (void)set_protevil(p_ptr->protevil - 1);
    }
  
  /* Increased Magical Defences. -LM- */
  if ((p_ptr->magicdef) && (!extend_magic))
    {
      (void)set_extra_defences(p_ptr->magicdef - 1);
    }
  
  /* Heroism. */
  if ((p_ptr->hero) && (!extend_magic))
    {
      (void)set_hero(p_ptr->hero - 1);
    }
  
  /* Berserk. */
  if ((p_ptr->shero) && (!extend_magic))
    {
      (void)set_shero(p_ptr->shero - 1);
    }
  
  /* Blessed */
  if ((p_ptr->blessed) && (!extend_magic))
    {
      (void)set_blessed(p_ptr->blessed - 1);
    }
  
  /* Shield */
  if ((p_ptr->shield) && (!extend_magic))
    {
      (void)set_shield(p_ptr->shield - 1);
    }
  
  /* Oppose Cold. */
  if ((p_ptr->oppose_cold) && (!extend_magic))
    {
      (void)set_oppose_cold(p_ptr->oppose_cold - 1);
    }
  
  /* Oppose Acid. */
  if ((p_ptr->oppose_acid) && (!extend_magic))
    {
      (void)set_oppose_acid(p_ptr->oppose_acid - 1);
    }
  
  /* Oppose Lightning */
  if ((p_ptr->oppose_elec) && (!extend_magic))
    {
      (void)set_oppose_elec(p_ptr->oppose_elec - 1);
    }
  
  /* Oppose Fire */
  if ((p_ptr->oppose_fire) && (!extend_magic))
    {
      (void)set_oppose_fire(p_ptr->oppose_fire - 1);
    }
  
  /* Oppose Poison */
  if ((p_ptr->oppose_pois) && (!extend_magic))
    {
      (void)set_oppose_pois(p_ptr->oppose_pois - 1);
    }
  
  
  /*** Poison and Stun and Cut ***/
  
  /* Poison */
  if (p_ptr->poisoned)
    {
      int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);
      
      /* Hobbits are sturdy. */
      if (hardy) adjust++;
      
      /* Maiar recover quickly from anything. */
      if (divine) adjust = 3 * adjust / 2;
      
      /* Apply some healing */
      (void)set_poisoned(p_ptr->poisoned - adjust);
    }
  
  /* Stun */
  if (p_ptr->stun)
    {
      int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);
      
      /* Maiar recover quickly from anything. */
      if (divine)adjust = 3 * adjust / 2;
      
      /* Apply some healing */
      (void)set_stun(p_ptr->stun - adjust);
    }
  
  /* Cut */
  if (p_ptr->cut)
    {
      int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);
      
      /* Hobbits are sturdy. */
      if (hardy) adjust++;
      
      /* Maiar recover quickly from anything. */
      if (divine) adjust = 3 * adjust / 2;
      
      /* Hack -- Truly "mortal" wound */
      if (p_ptr->cut > 1000) adjust = 0;
      
      /* Apply some healing */
      (void)set_cut(p_ptr->cut - adjust);
    }
  
  /* Every 500 turns, warn about any Black Breath not gotten from an equipped 
   * object, and stop any resting. -LM-
   */
  if (!(turn % 5000) && (p_ptr->black_breath))
    {
      bool be_silent = FALSE;
      
      /* check all equipment for the Black Breath flag. */
      for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
        {
          o_ptr = &inventory[i];
          
          /* Skip non-objects */
          if (!o_ptr->k_idx) continue;
          
          /* No messages if object has the flag, to avoid annoyance. */
          if (o_ptr->flags_curse & CF_DRAIN_EXP) be_silent = TRUE;
          
        }
      /* If we are allowed to speak, warn and disturb. */
      
      if (be_silent == FALSE)
        {
          msg_print("The Black Breath saps your soul!");
          disturb(0, 0);
        }
    }
  
  /* Decay special heighten power */
  if (p_ptr->heighten_power)
    {
      int decrement;
      
      /* 
       * Mega-Hack - To keep it from being a free ride for high speed 
       * characters, Heighten Power decays quickly when highly charged
       */
      decrement = 10 + (p_ptr->heighten_power / 55);
      
      if (p_ptr->heighten_power > decrement) 
        p_ptr->heighten_power -= decrement;
      else p_ptr->heighten_power = 0;
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
    }
  
  /* Decay special speed boost */
  if (p_ptr->speed_boost)
    {
      if (p_ptr->speed_boost > 10) p_ptr->speed_boost -= 10;
      else p_ptr->speed_boost = 0;
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
    }
  
  /*** Process Light ***/

  /* Check for light being wielded */
  o_ptr = &inventory[INVEN_LITE];
  
  /* Burn some fuel in the current lite */
  if (o_ptr->tval == TV_LITE)
    {
      /* Hack -- Use some fuel (except on artifacts) */
      if (!artifact_p(o_ptr) && (o_ptr->pval > 0) && (!is_daylight))
	  
        {
          /* Decrease life-span */
          o_ptr->pval--;
          
          /* Hack -- notice interesting fuel steps */
          if ((o_ptr->pval < 100) || (!(o_ptr->pval % 100)))
            {
              /* Window stuff */
              p_ptr->window |= (PW_EQUIP);
            }
          
          /* Hack -- Special treatment when blind */
          if (p_ptr->blind)
            {
              /* Hack -- save some light for later */
              if (o_ptr->pval == 0) o_ptr->pval++;
            }
          
          /* The light is now out */
          else if (o_ptr->pval == 0)
            {
              disturb(0, 0);
              msg_print("Your light has gone out!");
            }
          
          /* The light is getting dim */
          else if ((o_ptr->pval < 100) && (!(o_ptr->pval % 10)))
            {
              if (disturb_minor) disturb(0, 0);
              msg_print("Your light is growing faint.");
	    }
	}
    }
  
  /* Calculate torch radius */
  p_ptr->update |= (PU_TORCH);
  
  
  /*** Process Inventory ***/
  
  /* Handle experience draining.  In Oangband, the effect is worse, 
   * especially for high-level characters.  As per Tolkein, hobbits 
   * are resistant.
   */
  if (p_ptr->black_breath)
    {
      if (hardy) chance = 2;
      else chance = 5;
      
      if ((rand_int(100) < chance) && (p_ptr->exp > 0))
        {
          p_ptr->exp -= 1 + plev / 5;
          p_ptr->max_exp -= 1 + plev / 5;
          check_experience();
        }
    }
  
  /* Process equipment */
  for (j = 0, i = INVEN_WIELD; i < INVEN_TOTAL; i++)
    {
      /* Get the object */
      o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Recharge activatable objects */
      if (o_ptr->timeout > 0)
        {
          /* Recharge */
          o_ptr->timeout--;
          
          /* Notice changes, provide message if object is inscribed. */
          if (!(o_ptr->timeout)) 
            {
              recharged_notice(o_ptr);
              j++;
            }
        }
    }
  
  /* Notice changes */
  if (j)
    {
      /* Window stuff */
      p_ptr->window |= (PW_EQUIP);
    }
  
  /* Recharge rods.  Rods now use timeout to control charging status, 
   * and each charging rod in a stack decreases the stack's timeout by 
   * one per turn. -LM-
   */
  for (j = 0, i = 0; i < INVEN_PACK; i++)
    {
      o_ptr = &inventory[i];
      k_ptr = &k_info[o_ptr->k_idx];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Examine all charging rods or stacks of charging rods. */
      if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
        {
          
          /* Some rods should never get discharged at all */
          if (!k_ptr->pval) temp = o_ptr->timeout;
          
          /* Determine how many rods are charging. */
          else
            {
              temp = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
              if (temp > o_ptr->number) temp = o_ptr->number;
            }
          
          /* Decrease timeout by that number. */
          o_ptr->timeout -= temp;
          
          /* Boundary control. */
          if (o_ptr->timeout < 0) o_ptr->timeout = 0;
          
          /* Update if any rods are recharged */
          if (temp > (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval)
            {
              /* Update window */
              j++;
              /* Message if whole stack is recharged, if inscribed */
              if (!(o_ptr->timeout)) recharged_notice(o_ptr);
            }
        }
    }
  
 
  /* Notice changes */
  if (j)
    {
      /* Combine pack */
      p_ptr->notice |= (PN_COMBINE);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN);
    }
  
  /*** Process Objects ***/
  
  /* Process objects */
  for (i = 1; i < o_max; i++)
    {
      /* Access object */
      o_ptr = &o_list[i];
      
      /* Skip dead objects */
      if (!o_ptr->k_idx) continue;
      
      /* Recharge rods on the ground.  No messages. */
      if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout))
        {
          /* Charge it */
          o_ptr->timeout -= o_ptr->number;
          
          /* Boundary control. */
          if (o_ptr->timeout < 0) o_ptr->timeout = 0;
        }
    }
  
  
  /*** Random curse effects ***/
  
  /* Random teleportation */
  if ((p_ptr->teleport) && (rand_int(100) < 1))
    {
      /* Teleport player */
      teleport_player(40,FALSE);

      /* Notice */
      notice_curse(CF_TELEPORT, 0);
    }

  /* Random aggravation (with hasting of awake monsters */
  if ((p_ptr->rand_aggro) && (rand_int(500) < 1))
    {
      /* Aggravate, and notice if seen */
      if (aggravate_monsters(1, FALSE)) notice_curse(CF_AGGRO_RAND, 0);
    }

  /* Random poisoning */
  if ((p_ptr->rand_pois) && (rand_int(300) < 1))
    {
      /* Damage player */
      pois_hit(5 + randint(5));

      /* Notice */
      notice_curse(CF_POIS_RAND, 0);
    }
    
  /* Random poison cloud */
  if ((p_ptr->rand_pois_bad) && (rand_int(300) < 1)) 
    {
      /* Weak poison cloud, enough to wake nearby monsters */
      fire_sphere(GF_POIS, 0, 5, 3, 40);

      /* Hurt the player */
      pois_hit(10 + randint(10));

      /* Notice */
      notice_curse(CF_POIS_RAND_BAD, 0);
    }
    
  /* Random cuts */
  if ((p_ptr->rand_cuts) && (rand_int(200) < 1))
    {
      /* Wound the player, usually notice */
      if (set_cut(p_ptr->cut + 5 + rand_int(10))) notice_curse(CF_CUT_RAND, 0);
    }

  /* Random bad cuts */
  if ((p_ptr->rand_cuts_bad) && (rand_int(200) < 1))
    {
      /* Wound the player, usually notice */
      if (set_cut(p_ptr->cut + 20 + rand_int(30))) 
	notice_curse(CF_CUT_RAND_BAD, 0);
    }

  /* Random hallucination */
  if ((p_ptr->rand_hallu) && (rand_int(300) < 1))
    {
      /* Start hallucinating */
      (void)set_image(p_ptr->image + 10 + randint(10));

      /* Notice */
      notice_curse(CF_HALLU_RAND, 0);
    }

  /* Droppable weapons  */
  if (p_ptr->drop_weapon)
    {
      /* Check the whole inventory */
      for (i = 0; i < INVEN_TOTAL; i++)
	{
	  o_ptr = &inventory[i];

	  /* Chance for each item */
	  if ((o_ptr->flags_curse & CF_DROP_WEAPON) && (rand_int(200) < 1))
	    {
	      /* Notice */
	      notice_curse(CF_DROP_WEAPON, i + 1);

	      /* Dropped it */
	      inven_drop(i, randint(o_ptr->number));
	    }
	}
    }

  /* Summon demons */
  if ((p_ptr->attract_demon) && (rand_int(500) < 1))
    {
      /* Message */
      msg_print("You have attracted a demon.");

      /* Here it comes */
      summon_specific(p_ptr->py, p_ptr->px, FALSE, p_ptr->depth, SUMMON_DEMON);

      /* Notice */
      notice_curse(CF_ATTRACT_DEMON, 0);
    }

  /* Summon undead */
  if ((p_ptr->attract_undead) && (rand_int(500) < 1))
    {
      /* Message */
      msg_print("A call goes out beyond the grave.");

      /* Here it comes */
      summon_specific(p_ptr->py, p_ptr->px, FALSE, p_ptr->depth, SUMMON_UNDEAD);

      /* Notice */
      notice_curse(CF_ATTRACT_UNDEAD, 0);
    }

  /* Random paralysis - nasty! */
  if (((p_ptr->rand_paral) || (p_ptr->rand_paral_all)) && (rand_int(600) < 1))
    {
      /* Paralyzed for 1 turn */
      (void)set_paralyzed(p_ptr->paralyzed + 1);

      /* Notice */
      if (p_ptr->rand_paral) notice_curse(CF_PARALYZE, 0);
      else notice_curse(CF_PARALYZE_ALL, 0);
    }

  /* Experience drain */
  if ((p_ptr->drain_exp) && (rand_int(100) < 1) && (p_ptr->exp > 0))
        {
	  /* Drain */
          p_ptr->exp -= MIN(1 + plev / 5, p_ptr->exp);
          p_ptr->max_exp -= MIN(1 + plev / 5, p_ptr->exp);
          check_experience();

	  /* Notice */
	  notice_curse(CF_DRAIN_EXP, 0);
        }

  /* Mana drain */
  if ((p_ptr->drain_mana) && (rand_int(100) < 1) && (p_ptr->csp > 0))
    {
      /* Drain */
      p_ptr->csp -= MIN(1 + plev / 5, p_ptr->csp);
      
      /* Notice */
      notice_curse(CF_DRAIN_MANA, 0);
      
      /* Redraw mana */
      p_ptr->redraw |= (PR_MANA);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
    }

  /* Drain a random stat */  
  if ((p_ptr->drain_stat) && (rand_int(1000) < 1))
    {
      /* Drain */
      if (do_dec_stat(rand_int(A_MAX))) notice_curse(CF_DRAIN_STAT, 0);
    }

  /* Drain charges - code from monster attack */
  if ((p_ptr->drain_charge) && (rand_int(200) < 1))
    {
      /* Blindly hunt five times for an item. */
      for (j = 0; j < 5; j++)
	{
	  bool has_charges = FALSE;

	  /* Pick an item */
	  i = rand_int(INVEN_PACK);
	  
	  /* Obtain the item */
	  o_ptr = &inventory[i];
	  k_ptr = &k_info[o_ptr->k_idx];
	  
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
		  (o_ptr->pval)) has_charges = TRUE;
	      
	      /* case of (at least partially) 
	       * charged rods. */
	      if ((o_ptr->tval == TV_ROD) &&
		  (o_ptr->timeout < o_ptr->pval)) has_charges = TRUE;
	      
	      if (has_charges)
		{
		  /* Message */
		  msg_print("Energy drains from your pack!");
		  
		  /* Obvious */
		  notice_curse(CF_DRAIN_CHARGE, 0);
			    
		  /* Uncharge */
		  if ((o_ptr->tval == TV_STAFF) || 
		      (o_ptr->tval == TV_WAND)) 
		    o_ptr->pval = 0;
		  
		  /* New-style rods. */
		  if (o_ptr->tval == TV_ROD) 
		    o_ptr->timeout = o_ptr->pval;
		  
		  
		  /* Combine / Reorder the pack */
		  p_ptr->notice |= 
		    (PN_COMBINE | PN_REORDER);
		  
		  /* Window stuff */
		  p_ptr->window |= (PW_INVEN);
		  
		  /* Not more than one inventory slot affected. */
		  break;
		}
	    }
	}
    }
  
  /* Delayed Word-of-Recall */
  if (p_ptr->word_recall)
    {
      /* Count down towards recall */
      p_ptr->word_recall--;
      
      /* Activate the recall */
      if (!p_ptr->word_recall)
	{
          /* Disturbing! */
          disturb(0, 0);
          
	  /* Sound */
	  sound(MSG_TPLEVEL);

          /* Determine the level */
          if (p_ptr->stage != p_ptr->home)
            {
              msg_print("You feel yourself yanked homewards!");
              
              /* Homeward bound */
              p_ptr->last_stage = NOWHERE;
              p_ptr->stage = p_ptr->home;
              
              /* Reset depth */
              p_ptr->depth = 0;
              
              /* Leaving */
	      p_ptr->leaving = TRUE;
            }
          else
            {
              msg_print("You feel yourself yanked away!");
              
              /* New stage */
              p_ptr->last_stage = NOWHERE;
              p_ptr->stage = p_ptr->recall_pt;
              
              /* Reset depth */
              p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
              
              /* Leaving */
              p_ptr->leaving = TRUE;
            }
          
          /* Sound */
          sound(MSG_TPLEVEL);
          
          p_ptr->redraw |= PR_STATUS;
        }
    }
  
  /* Delayed level feelings */
  if ((p_ptr->depth) && (!p_ptr->leaving) && (!do_feeling) && (!(turn % 100))
      && (!p_ptr->themed_level))
    {
      
      int chance = 40;
      
      /* After sufficient time, can learn about the level */
      if ((rand_int(80) < p_ptr->skill_srh) &&
          (rand_int(80) < chance))
        {
          /* Now have a feeling */
          do_feeling = TRUE;
          
          /* Announce feeling */
          do_cmd_feeling();
          
          /* Update the level indicator */
          p_ptr->redraw |= (PR_DEPTH);
          
          /* Disturb */
          disturb(0, 0);
        }
    }
}




#ifdef ALLOW_BORG

/**
 * Verify use of "borg" mode
 */
static bool verify_borg_mode(void)
{
  static int verify = 1;
  
  /* Ask first time */
  if (verify && verify_special)
    {
      /* Mention effects */
      msg_print("You are about to use the dangerous, unsupported, borg commands!");
      msg_print("Your machine may crash, and your savefile may become corrupted!");
      msg_print(NULL);
      
      /* Verify request */
      if (!get_check("Are you sure you want to use the borg commands? "))
        {
          return (FALSE);
        }
    }
  
  /* Verified */
  verify = 0;
  
  /* Mark savefile */
  p_ptr->noscore |= 0x0010;
  
  /* Okay */
  return (TRUE);
}


/**
 * Hack -- Declare the Borg Routines
 */
extern void do_cmd_borg(void);

#endif





/**
 * Hack -- helper function for "process_player()"
 *
 * Check for changes in the "monster memory"
 */
static void process_player_aux(void)
{
  static int old_monster_race_idx = 0;
  
  static u32b old_r_flags1 = 0L;
  static u32b old_r_flags2 = 0L;
  static u32b old_r_flags3 = 0L;
  static u32b old_r_flags4 = 0L;
  static u32b old_r_flags5 = 0L;
  static u32b old_r_flags6 = 0L;
  
  static byte old_r_blows0 = 0;
  static byte old_r_blows1 = 0;
  static byte old_r_blows2 = 0;
  static byte old_r_blows3 = 0;
  
  static byte old_r_cast_inate = 0;
  static byte old_r_cast_spell = 0;
  
  
  /* Tracking a monster */
  if (p_ptr->monster_race_idx)
    {
      /* Get the monster lore */
      monster_lore *l_ptr = &l_list[p_ptr->monster_race_idx];
      
      /* Check for change of any kind */
      if ((old_monster_race_idx != p_ptr->monster_race_idx) ||
          (old_r_flags1 != l_ptr->flags1) ||
          (old_r_flags2 != l_ptr->flags2) ||
          (old_r_flags3 != l_ptr->flags3) ||
          (old_r_flags4 != l_ptr->flags4) ||
          (old_r_flags5 != l_ptr->flags5) ||
          (old_r_flags6 != l_ptr->flags6) ||
          (old_r_blows0 != l_ptr->blows[0]) ||
          (old_r_blows1 != l_ptr->blows[1]) ||
          (old_r_blows2 != l_ptr->blows[2]) ||
          (old_r_blows3 != l_ptr->blows[3]) ||
          (old_r_cast_inate != l_ptr->cast_inate) ||
          (old_r_cast_spell != l_ptr->cast_spell))
        {
          /* Memorize old race */
          old_monster_race_idx = p_ptr->monster_race_idx;
          
          /* Memorize flags */
          old_r_flags1 = l_ptr->flags1;
          old_r_flags2 = l_ptr->flags2;
          old_r_flags3 = l_ptr->flags3;
          old_r_flags4 = l_ptr->flags4;
          old_r_flags5 = l_ptr->flags5;
          old_r_flags6 = l_ptr->flags6;
          
          /* Memorize blows */
          old_r_blows0 = l_ptr->blows[0];
          old_r_blows1 = l_ptr->blows[1];
          old_r_blows2 = l_ptr->blows[2];
          old_r_blows3 = l_ptr->blows[3];
          
          /* Memorize castings */
          old_r_cast_inate = l_ptr->cast_inate;
          old_r_cast_spell = l_ptr->cast_spell;
          
          /* Window stuff */
          p_ptr->window |= (PW_MONSTER);

	  /* Window stuff */
	  window_stuff();
	}
    }
}

/**
 * Helper function for mana gained through special means
 * (Power Siphon and Soul Siphon).
 */
static void special_mana_gain(void)
{
  if (p_ptr->mana_gain)
    {
      /* Message */
      if (p_ptr->csp < p_ptr->msp) msg_print("You gain mana.");
      
      /* Partial fill */
      if (p_ptr->mana_gain < p_ptr->msp - p_ptr->csp)
        {
          p_ptr->csp += p_ptr->mana_gain;
          p_ptr->mana_gain = 0;
        }
      /* Complete Fill */
      else
        {
          p_ptr->mana_gain -= p_ptr->msp - p_ptr->csp;
          p_ptr->csp = p_ptr->msp;
        }
      
      /*
       * Hack - If there is a lot of mana left, it can do damage
       * Mega-Hack - Restrict to Necromancers to make it affect Soul Siphon
       * and not Power Siphon.
       */
      if ((p_ptr->mana_gain > p_ptr->lev) && (check_ability(SP_EVIL)))
        {
          msg_print("You absorb too much mana!");
          take_hit(damroll(2, 8), "mana burn");
        }
      
      /* Paranioa */
      if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
      
      /* Clear mana gain */
      p_ptr->mana_gain = 0;
      
      /* Redraw */
      p_ptr->redraw |= (PR_MANA);
    }
}


/**
 * Process the player
 *
 * Notice the annoying code to handle "pack overflow", which
 * must come first just in case somebody manages to corrupt
 * the savefiles by clever use of menu commands or something.
 *
 * Notice the annoying code to handle "monster memory" changes,
 * which allows us to avoid having to update the window flags
 * every time we change any internal monster memory field, and
 * also reduces the number of times that the recall window must
 * be redrawn.
 *
 * Note that the code to check for user abort during repeated commands
 * and running and resting can be disabled entirely with an option, and
 * even if not disabled, it will never check during "special" resting
 * (codes -1 and -2), and it will only check during every 16th player
 * turn of "normal" resting.
 *
 * This function is no longer responsible for adding player energy.
 */
static void process_player(void)
{
  int i;
  
  int temp_wakeup_chance;
  
  byte feat = cave_feat[p_ptr->py][p_ptr->px];
  feature_type *f_ptr = &f_info[feat];

  /*** Check for interupts ***/
  
  /* Complete resting */
  if (p_ptr->resting < 0)
    {
      /* Basic resting */
      if (p_ptr->resting == -1)
	{
	  /* Stop resting */
	  if ((p_ptr->chp == p_ptr->mhp) &&
	      (p_ptr->csp == p_ptr->msp))
	    {
	      disturb(0, 0);
	    }
	}
      
      /* Complete resting */
      else if (p_ptr->resting == -2)
	{
	  /* Stop resting */
	  if ((p_ptr->chp == p_ptr->mhp) &&
	      (p_ptr->csp == p_ptr->msp) &&
	      !p_ptr->blind && !p_ptr->confused &&
	      !p_ptr->poisoned && !p_ptr->afraid &&
	      !p_ptr->stun && !p_ptr->cut &&
	      !p_ptr->slow && !p_ptr->paralyzed &&
	      !p_ptr->image && !p_ptr->word_recall)
	    {
	      disturb(0, 0);
	    }
	}
      
      /* Resting until sunrise/set */
      else if (p_ptr->resting == -3)
	{
	  /* Stop resting */
	  if (!(((turn / 10) * 10) % (10L * TOWN_DAWN)))
	    {
	      msg_print("It is daybreak.");
	      disturb(0, 0);
	    }
	  else if (!(((turn / 10) * 10) % ((10L * TOWN_DAWN) / 2)))
            {
	      msg_print("It is nightfall.");
              disturb(0, 0);
            }
        }
    }
  
  /* Handle "abort" */
  if (!avoid_abort)
    {
      /*
       * Originally, with "resting < 0" you could not abort.
       * In "resting && !(resting & 0x0F)":
       *     -1 & 0x0F == 15
       *     -2 & 0x0F == 16
       */
      
      /* Check for "player abort" */
      if (p_ptr->running ||
          p_ptr->command_rep ||
          (p_ptr->resting && !((turn * 10) % 0x0F)))
        {
          /* Do not wait */
          inkey_scan = SCAN_INSTANT;
          
          /* Check for a key */
          if (anykey())
            {
              /* Flush input */
              flush();

	      /* Disturb */
	      disturb(0, 0);
	      
	      /* Hack -- Show a Message */
              msg_print("Cancelled.");
            }
        }
    }
  
  /* Add context-sensitive mouse buttons */
  
  if ((f_ptr->flags & TF_STAIR) && !(feat % 2))
    add_button("<", '<');
  else 
    kill_button('<');
  
  if ((f_ptr->flags & TF_STAIR) && (feat % 2))
    add_button(">", '>');
  else
    kill_button('>');
  
  if (cave_o_idx[p_ptr->py][p_ptr->px] > 0)
    add_button("g", 'g');
  else
    kill_button('g');
  
  update_statusline();
  

  /*** Hack - handle special mana gain ***/
  special_mana_gain();
  
  /*** Handle actual user input ***/
  
  /* Repeat until energy is reduced */
  do
    {
      /* Notice stuff (if needed) */
      if (p_ptr->notice) notice_stuff();
      
      /* Update stuff (if needed) */
      if (p_ptr->update) update_stuff();
      
      /* Redraw stuff (if needed) */
      if (p_ptr->redraw) redraw_stuff();
      
      /* Redraw stuff (if needed) */
      if (p_ptr->window) window_stuff();
      
      
      /* Place the cursor on the player */
      move_cursor_relative(p_ptr->py, p_ptr->px);
      
      /* Refresh (optional) */
      if (fresh_before) Term_fresh();
      
      
      /* Hack -- Pack Overflow */
      if (inventory[INVEN_PACK - p_ptr->pack_size_reduce].k_idx)
        {
          int item;
          
          for (item = INVEN_PACK; 
               item >= INVEN_PACK - p_ptr->pack_size_reduce; item--)
            {
              char o_name[120];
              
              object_type *o_ptr;
              
              /* Access the slot to be dropped */
              o_ptr = &inventory[item];
              
              /* Skip non-objects */
              if (!o_ptr->k_idx) continue;
              
              /* Skip non-droppables */
              if (o_ptr->flags_curse & CF_STICKY_CARRY) 
		{
		  notice_curse(CF_STICKY_CARRY, item + 1);
		  continue;
		}
              
              /* Disturbing */
              disturb(0, 0);
              
	      /* Warning */
              msg_print("Your pack overflows!");
              
              /* Describe */
              object_desc(o_name, o_ptr, TRUE, 3);
              
              /* Message */
              msg_format("You drop %s (%c).", o_name, index_to_label(item));

	      /* Drop it (carefully) near the player */
	      drop_near(o_ptr, 0, p_ptr->py, p_ptr->px);
	      
	      /* Modify, Describe, Optimize */
	      inven_item_increase(item, -255);
	      inven_item_describe(item);
	      inven_item_optimize(item);
	      
	      /* Notice stuff (if needed) */
	      if (p_ptr->notice) notice_stuff();
	      
	      /* Update stuff (if needed) */
	      if (p_ptr->update) update_stuff();
	      
	      /* Redraw stuff (if needed) */
	      if (p_ptr->redraw) redraw_stuff();
	      
              /* Window stuff (if needed) */
              if (p_ptr->window) window_stuff();
            }
        }
      
      
      /* Hack -- cancel "lurking browse mode" */
      if (!p_ptr->command_new) p_ptr->command_see = FALSE;
      
      /* Mega-hack -- show lists */
      if (show_lists) p_ptr->command_see = TRUE;      

      /* Hack - update visible monster list */
      p_ptr->window |= (PW_MONLIST | PW_ITEMLIST);

      /* Assume free turn */
      p_ptr->energy_use = 0;
      
      
      /* Paralyzed or Knocked Out */
      if ((p_ptr->paralyzed) || (p_ptr->stun >= 100))
        {
          /* Take a turn */
          p_ptr->energy_use = 100;
        }
      
      /* Resting */
      else if (p_ptr->resting)
        {
	  /* Timed rest */
	  if (p_ptr->resting > 0)
	    {
	      /* Reduce rest count */
	      p_ptr->resting--;
	      
	      /* Redraw the state */
	      p_ptr->redraw |= (PR_STATE);
	    }
	  
	  /* Take a turn */
	  p_ptr->energy_use = 100;
	}
      
      /* Running */
      else if (p_ptr->running)
	{
	  /* Take a step */
	  run_step(0);
	}
      
      /* Repeated command */
      else if (p_ptr->command_rep)
        {
          /* Count this execution */
          p_ptr->command_rep--;
          
          /* Redraw the state */
          p_ptr->redraw |= (PR_STATE);
          
          /* Redraw stuff */
          /* redraw_stuff(); */
          
          /* Hack -- Assume messages were seen */
          msg_flag = FALSE;
          
	  /* Clear the top line */
          prt("", 0, 0);
          
          /* Process the command */
          process_command(TRUE);
        }
      
      /* Normal command */
      else
	{
	  /* Check monster recall */
	  process_player_aux();
	  
          /* Place the cursor on the player */
          move_cursor_relative(p_ptr->py, p_ptr->px);
          
          /* Process the command */
          process_command(FALSE);
	  
          /* Mega hack - complete redraw if big graphics */
          if (use_dbltile || use_trptile) 
            p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP 
                              | PR_EQUIPPY);
	  
        }
      
      /*** Hack - handle special mana gain ***/
      special_mana_gain();
      
      /*** Clean up ***/
      
      /* Significant */
      if (p_ptr->energy_use)
	{
	  /* Use some energy */
	  p_ptr->energy -= p_ptr->energy_use;
          
          
          /* Hack -- constant hallucination */
          if (p_ptr->image) p_ptr->redraw |= (PR_MAP);
          
          
          /* Shimmer monsters if needed */
          if (!avoid_other && shimmer_monsters)
            {
              /* Clear the flag */
              shimmer_monsters = FALSE;
              
              /* Shimmer multi-hued monsters */
              for (i = 1; i < m_max; i++)
                {
                  monster_type *m_ptr;
                  monster_race *r_ptr;
                  
                  /* Access monster */
                  m_ptr = &m_list[i];
                  
                  /* Skip dead monsters */
                  if (!m_ptr->r_idx) continue;
                  
                  /* Access the monster race */
                  r_ptr = &r_info[m_ptr->r_idx];
                  
                  /* Skip non-multi-hued monsters */
		  if (!(r_ptr->flags1 & (RF1_ATTR_MULTI))) continue;
		  
		  /* Reset the flag */
		  shimmer_monsters = TRUE;
		  
		  /* Redraw regardless */
		  lite_spot(m_ptr->fy, m_ptr->fx);
		}
	    }
	  
	  /* Repair "mark" flags */
	  if (repair_mflag_mark)
	    {
	      /* Reset the flag */
              repair_mflag_mark = FALSE;
              
              /* Process the monsters */
              for (i = 1; i < m_max; i++)
                {
                  monster_type *m_ptr;
                  
                  /* Access monster */
                  m_ptr = &m_list[i];
                  
                  /* Skip dead monsters */
                  /* if (!m_ptr->r_idx) continue; */
		  
		  /* Repair "mark" flag */
		  if (m_ptr->mflag & (MFLAG_MARK))
		    {
		      /* Skip "show" monsters */
		      if (m_ptr->mflag & (MFLAG_SHOW))
			{
			  /* Repair "mark" flag */
			  repair_mflag_mark = TRUE;
			  
			  /* Skip */
			  continue;
			}
		      
		      /* Forget flag */
		      m_ptr->mflag &= ~(MFLAG_MARK);
		      
		      /* Update the monster */
		      update_mon(i, FALSE);
		    }
		}
	    }
	}
      
      /* Repair "show" flags */
      if (repair_mflag_show)
	{
	  /* Reset the flag */
          repair_mflag_show = FALSE;
          
          /* Process the monsters */
          for (i = 1; i < m_max; i++)
            {
              monster_type *m_ptr;
              
              /* Access monster */
              m_ptr = &m_list[i];
              
              /* Skip dead monsters */
              /* if (!m_ptr->r_idx) continue; */
	      
	      /* Clear "show" flag */
	      m_ptr->mflag &= ~(MFLAG_SHOW);
            }
        }
    }
  while (!p_ptr->energy_use && !p_ptr->leaving);
  
  /*
   * Characters are noisy.
   * Use the amount of extra (directed) noise the player has made 
   * this turn to calculate the total amount of ambiant noise.
   */
  temp_wakeup_chance = p_ptr->base_wakeup_chance + add_wakeup_chance;
  
  /* People don't make much noise when resting. */
  if (p_ptr->resting) temp_wakeup_chance /= 2;
  
  /* Characters hidden in shadow have almost perfect stealth. */
  if ((p_ptr->superstealth) && (!p_ptr->aggravate))
    {
      if (temp_wakeup_chance > 200) temp_wakeup_chance = 200;
    }
  
  
  /* Increase noise level if necessary. */
  if (temp_wakeup_chance > total_wakeup_chance) 
    total_wakeup_chance = temp_wakeup_chance;
  
  
  /* Update noise flow information */
  update_noise();
  
  /* Update scent trail */
  update_smell();
  
  
  /* 
   * Reset character vulnerability.  Will be calculated by 
   * the first member of an animal pack that has a use for it.
   */
  p_ptr->vulnerability = 0;
  
}



/**
 * Interact with the current dungeon level.
 *
 * This function will not exit until the level is completed,
 * the user dies, or the game is terminated.
 */
static void dungeon(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;

  bool just_arrived = TRUE;
  
  /* Not leaving */
  p_ptr->leaving = FALSE;
  
  
  /* Reset the "command" vars */
  p_ptr->command_cmd = 0;
  p_ptr->command_new = 0;
  p_ptr->command_rep = 0;
  p_ptr->command_arg = 0;
  p_ptr->command_dir = 0;
  
  
  /* Cancel the target */
  target_set_monster(0);
  
  /* Cancel the health bar */
  health_track(0);
  
  
  /* Reset shimmer flags */
  shimmer_monsters = TRUE;
  shimmer_objects = TRUE;
  
  /* Reset repair flags */
  repair_mflag_show = TRUE;
  repair_mflag_mark = TRUE;
  

  /* Disturb */
  disturb(1, 0);
  
  
  /* Track maximum player level */
  if (p_ptr->max_lev < p_ptr->lev)
    {
      p_ptr->max_lev = p_ptr->lev;
    }
  
  /* Track depth */
  p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
  
  /* No stairs down from Quest */
  if ((is_quest(p_ptr->stage)) && 
      ((p_ptr->create_stair == FEAT_MORE) || 
      (p_ptr->create_stair == FEAT_MORE_SHAFT))) 
      p_ptr->create_stair = 0;
  
  
  /* No stairs from town or if not allowed */
  if (!p_ptr->depth || !dungeon_stair)
    {
      p_ptr->create_stair = 0;
    }
  
  /* Make a staircase */
  if (p_ptr->create_stair)
    {
      
      /* Place a staircase */
      if (cave_valid_bold(py, px))
        {
	  
          /* XXX XXX XXX */
          delete_object(py, px);
          
          /* Make stairs */
          cave_set_feat(py, px, p_ptr->create_stair);
          
          /* Mark the stairs as known */
          cave_info[py][px] |= (CAVE_MARK);
        }
      
      /* Cancel the stair request */
      p_ptr->create_stair = FALSE;
    }
  
  
  /* Choose panel */
  verify_panel();
  
  
  /* Flush messages */
  msg_print(NULL);
  
  
  /* Hack -- Increase "xtra" depth */
  character_xtra++;


  /* Clear */
  Term_clear();
  
  /* Check for vampires */
  if ((p_ptr->schange == SHAPE_VAMPIRE) && 
      ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)))
    shapechange(SHAPE_NORMAL);
  
  /* Update stuff */
  p_ptr->update |= (PU_BONUS | PU_HP | PU_SPELLS | PU_SPECIALTY);
  
  /* Calculate torch radius */
  p_ptr->update |= (PU_TORCH);

  /* Fully update the visuals (and monster distances) */
  p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_DISTANCE);
  
  /* Redraw dungeon */
  p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
  /* Window stuff */
  p_ptr->window |= (PW_MONSTER);
  
  /* Window stuff */
  p_ptr->window |= (PW_OVERHEAD | PW_MONLIST | PW_ITEMLIST);
  
  /* Redraw stuff */
  redraw_stuff();
  
  /* Redraw stuff */
  window_stuff();

  update_stuff();
  

  /* Hack -- Decrease "xtra" depth */
  character_xtra--;
  
  
  /* Update stuff */
  p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);
  
  /* Combine / Reorder the pack */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);

  /* Make basic mouse buttons */
  normal_screen = TRUE;
  (void) add_button("ESC", ESCAPE);
  (void) add_button("Ent", '\r');
  (void) add_button("Spc", ' ');
  (void) add_button("Rpt", 'n');
  (void) add_button("Std", ',');
  (void) add_button("Inv", '|');
  (void) add_button("Rest", 'R');
  (void) add_button("Mgc", 'm');
  
  /* Redraw buttons */
  p_ptr->redraw |= (PR_BUTTONS);
  
  /* Notice stuff */
  notice_stuff();
  
  /* Update stuff */
  update_stuff();
  
  /* Redraw stuff */
  redraw_stuff();
  
  /* Window stuff */
  window_stuff();
  
  /* Refresh */
  Term_fresh();


  /* Handle delayed death */
  if (p_ptr->is_dead) return;
  
  
  /* Announce (or repeat) the feeling */
  if ((p_ptr->depth) && (do_feeling)) do_cmd_feeling();
  
  /* Announce a player ghost challenge. -LM- */
  if (bones_selector) ghost_challenge();

  /* Reset noise */
  total_wakeup_chance = 0;
  add_wakeup_chance = 0;
  
  /*** Process this dungeon level ***/
  
  /* Reset the monster generation level */
  monster_level = p_ptr->depth;
  
  /* Reset the object generation level */
  object_level = p_ptr->depth;
  
  /* Main loop */
  while (TRUE)
    {
      /* Hack -- Compact the monster list occasionally */
      if (m_cnt + 32 > z_info->m_max) compact_monsters(64);
      
      /* Hack -- Compress the monster list occasionally */
      if (m_cnt + 32 < m_max) compact_monsters(0);
      
      
      /* Hack -- Compact the object list occasionally */
      if (o_cnt + 32 > z_info->o_max) compact_objects(64);
      
      /* Hack -- Compress the object list occasionally */
      if (o_cnt + 32 < o_max) compact_objects(0);
      
      
      /*** Apply energy ***/
      
      /* Give the player some energy */
      p_ptr->energy += extract_energy[p_ptr->pspeed];
      
      /* Give the player the first move in a new level */
      if (just_arrived)
	{ 
	  just_arrived = FALSE;
	  if (!p_ptr->leaving)
	    {
	      int i;
	      monster_type *m_ptr;

	      /* Set the player energy to exactly 100 */
	      p_ptr->energy = 100;
	      
	      /* Give the player more energy than any monster */
	      for (i = m_max - 1; i >= 1; i--)
		{
		  /* Access the monster */
		  m_ptr = &m_list[i];
		  
		  /* Ignore dead monsters */
		  if (!m_ptr->r_idx) continue;
		  
		  /* Give the player at least as much energy */
		  if (m_ptr->energy > p_ptr->energy) 
		    p_ptr->energy = m_ptr->energy;
		}
	    }
	}
      
      /* Can the player move? */
      while (p_ptr->energy >= 100 && !p_ptr->leaving)
        {
          /* Process monster with even more energy first */
	  process_monsters((byte)(p_ptr->energy + 1));
	  
	  /* if still alive */
	  if (!p_ptr->leaving)
	    {
	      /* Process the player */
	      process_player();
	    }
	}
      
      /* Notice stuff */
      if (p_ptr->notice) notice_stuff();
      
      /* Update stuff */
      if (p_ptr->update) update_stuff();
      
      /* Redraw stuff */
      if (p_ptr->redraw) redraw_stuff();
      
      /* Redraw stuff */
      if (p_ptr->window) window_stuff();
      
      /* Hack -- Hilite the player */
      move_cursor_relative(p_ptr->py, p_ptr->px);
      
      /* Optional fresh */
      if (fresh_after) Term_fresh();
      
      /* Handle "leaving" */
      if (p_ptr->leaving) break;
      
      /* Process monsters */
      process_monsters(0);
      
      /* Reset Monsters */
      reset_monsters();
      
      /* Notice stuff */
      if (p_ptr->notice) notice_stuff();

      /* Update stuff */
      if (p_ptr->update) update_stuff();
      
      /* Redraw stuff */
      if (p_ptr->redraw) redraw_stuff();
      
      /* Redraw stuff */
      if (p_ptr->window) window_stuff();

      /* Hack -- Hilite the player */
      move_cursor_relative(p_ptr->py, p_ptr->px);
      
      /* Optional fresh */
      if (fresh_after) Term_fresh();
      
      /* Handle "leaving" */
      if (p_ptr->leaving) break;
      

      /* Process the world */
      process_world();
      
      /* Notice stuff */
      if (p_ptr->notice) notice_stuff();
      
      /* Update stuff */
      if (p_ptr->update) update_stuff();
      
      /* Redraw stuff */
      if (p_ptr->redraw) redraw_stuff();
      
      /* Window stuff */
      if (p_ptr->window) window_stuff();

      /* Hack -- Hilite the player */
      move_cursor_relative(p_ptr->py, p_ptr->px);
      
      /* Optional fresh */
      if (fresh_after) Term_fresh();
      
      /* Handle "leaving" */
      if (p_ptr->leaving) break;
      
      /* Count game turns */
      turn++;
    }

  /* Kill basic mouse buttons */
  (void) kill_button(ESCAPE);
  (void) kill_button('\r');
  (void) kill_button(' ');
  (void) kill_button('n');
  (void) kill_button(',');
  (void) kill_button('|');
  (void) kill_button('R');
  (void) kill_button('m');
  
}



/**
 * Process some user pref files
 */
static void process_some_user_pref_files(void)
{
  char buf[128];
  
  /* Process the "user.prf" file */
  (void)process_pref_file("user.prf");
  
  /* Access the "race" pref file */
  sprintf(buf, "%s.prf", p_name + rp_ptr->name);
  
  /* Process that file */
  process_pref_file(buf);
  
  /* Access the "class" pref file */
  sprintf(buf, "%s.prf", c_name + cp_ptr->name);
  
  /* Process that file */
  process_pref_file(buf);
  
  /* Process the "PLAYER.prf" file */
  sprintf(buf, "%s.prf", op_ptr->base_name);
  
  /* Process the "PLAYER.prf" file */
  (void)process_pref_file(buf);
}


/**
 * Actually play a game
 *
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * player to start a new game.
 *
 * Note that we load the RNG state from savefiles (2.8.0 or later)
 * and so we only initialize it if we were unable to load it, and
 * we mark successful loading using the "Rand_quick" flag.  This
 * is a hack but it optimizes loading of savefiles.  XXX XXX
 */
void play_game(bool new_game)
{

  int i;

  /* No restart */
  game_start = FALSE;
  
  /* Hack -- Increase "icky" depth */
  character_icky++;
  
  
  /* Verify main term */
  if (!angband_term[0])
    {
      quit("main window does not exist");
    }
  
  /* Make sure main term is active */
  Term_activate(angband_term[0]);
  
  /* Verify minimum size */
  if ((Term->hgt < (small_screen ? 12 : 24)) ||
  (Term->wid < (small_screen ? 32 : 80)))
    {
      quit("main window is too small");
    }

  
  /* Hack -- turn off the cursor */
  (void)Term_set_cursor(0);
 
  /* Quickstart */
  character_quickstart = FALSE;
  
  /* Attempt to load */
  if (!load_player())
    {
      /* Oops */
      quit("broken savefile");
    }

  /* Nothing loaded */
  if (!character_loaded)
    {
      /* Reset RNG when dead-character savefile is loaded */
      Rand_quick = TRUE;
      
      /* Make new player */
      new_game = TRUE;
      
      /* The dungeon is not ready */
      character_dungeon = FALSE;
    }
  
  /* Process old character */
  if (!new_game)
    {
    }
  
  /* Init RNG */
  if (Rand_quick)
    {
      u32b seed;
      
      /* Basic seed */
#ifdef _WIN32_WCE
      unsigned long fake_time(unsigned long* fake_time_t);
      seed = fake_time(NULL);
#else
      seed = (time(NULL));
#endif
      
#ifdef SET_UID
      
      /* Mutate the seed on Unix machines */
      seed = ((seed >> 3) * (getpid() << 1));
      
#endif
      
      /* Use the complex RNG */
      Rand_quick = FALSE;
      
      /* Seed the "complex" RNG */
      Rand_state_init(seed);
    }
  
  /* Roll new character */
  if (new_game)
    {
      /* The dungeon is not ready */
      character_dungeon = FALSE;
      
      /* Hack -- seed for flavors */
      seed_flavor = rand_int(0x10000000);
      
      /* Hack -- seed for town layouts -NRM- */
      for (i = 0; i < 10; i++)
        seed_town[i] = rand_int(0x10000000);
      
      /* Roll up a new character */
      player_birth();
      
      /* Start in home town  - or on the stairs to Angband */
      p_ptr->stage = (adult_thrall ? (adult_dungeon ? 87 : 135) : p_ptr->home);
      p_ptr->depth = stage_map[p_ptr->stage][DEPTH];
      
      /* Hack -- enter the world */
      turn = 1;
      
      /* Read the default options */
      process_pref_file("birth.prf");
      
    }
  
  /* Normal machine (process player name) */
  if (savefile[0])
    {
      process_player_name(FALSE);
    }
  
  /* Weird machine (process player name, pick savefile name) */
  else
    {
      process_player_name(TRUE);
    }
  
  /* Flash a message */
  prt("Please wait...", 0, 0);
  
  /* Flush the message */
  Term_fresh();
  
  
  /*** Prepare "vinfo" array ***/
  
  /* Used by "update_view()" - need birth options set as of FAangband 0.3.4*/
  (void)vinfo_init();


  /* Flavor the objects */
  flavor_init();
  
  /* Reset visuals */
  reset_visuals(TRUE);
  
  /* Initialize the artifact allocation lists */
  init_artifacts();
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
  /* Window stuff */
  p_ptr->window |= (PW_MONSTER);
  
  /* Window stuff */
  window_stuff();
  
  /* Hack - load prefs properly */
  reset_visuals(TRUE);

  /* Process some user pref files */
  process_some_user_pref_files();
  
  /* Set or clear "rogue_like_commands" if requested */
  if (arg_force_original) rogue_like_commands = FALSE;
  if (arg_force_roguelike) rogue_like_commands = TRUE;


  /* React to changes */
  Term_xtra(TERM_XTRA_REACT, 0);
  
  
  /* Generate a stage if needed */
  if (!character_dungeon) 
    generate_cave();
  
  
  /* Character is now "complete" */
  character_generated = TRUE;
  
  
  /* Hack -- Decrease "icky" depth */
  character_icky--;
  
  
  /* Start playing */
  p_ptr->playing = TRUE;
  
  /* Hack -- Enforce "delayed death" */
  if (p_ptr->chp < 0) p_ptr->is_dead = TRUE;
  
  /* Verify the (possibly resized) panel */
  verify_panel();
  
  /* Update some stuff not stored in the savefile any more */
  p_ptr->update |= (PU_UPDATE_VIEW);

  /* Update stuff */
  update_stuff();

  /* Process */
  while (TRUE)
    {
      /* Process the level */
      dungeon();
      
      
      /* Notice stuff */
      if (p_ptr->notice) notice_stuff();
      
      /* Update stuff */
      if (p_ptr->update) update_stuff();
      
      /* Redraw stuff */
      if (p_ptr->redraw) redraw_stuff();
      
      /* Window stuff */
      if (p_ptr->window) window_stuff();
      
      
      /* Cancel the target */
      target_set_monster(0);
      
      /* Cancel the health bar */
      health_track(0);
      
      
      /* Forget the view */
      forget_view();
      

      /* Handle "quit and save" */
      if (!p_ptr->playing && !p_ptr->is_dead) break;
      
      /* Erase the old cave */
      wipe_o_list();
      wipe_m_list();
      
      /* XXX XXX XXX */
      msg_print(NULL); 
      
      /* Accidental Death */
      if (p_ptr->playing && p_ptr->is_dead)
	{
	  /* Mega-Hack -- Allow player to cheat death */
	  if ((p_ptr->wizard || cheat_live) && !get_check("Die? "))
	    {
	      /* Mark social class, reset age, if needed */
	      if (p_ptr->sc) p_ptr->sc = p_ptr->age = 0;
	      
	      /* Increase age */
              p_ptr->age++;
              
              /* Mark savefile */
              p_ptr->noscore |= 0x0001;
              
              /* Message */
              msg_print("You invoke wizard mode and cheat death.");
              msg_print(NULL);
              
              /* Cheat death */
              p_ptr->is_dead = FALSE;
	      
	      /* Restore hit points */
	      p_ptr->chp = p_ptr->mhp;
	      p_ptr->chp_frac = 0;
	      
	      /* Restore spell points */
	      p_ptr->csp = p_ptr->msp;
              p_ptr->csp_frac = 0;
              
              /* Hack -- Healing */
              (void)set_blind(0);
              (void)set_confused(0);
              (void)set_poisoned(0);
              (void)set_afraid(0);
              (void)set_paralyzed(0);
              (void)set_image(0);
              (void)set_stun(0);
              (void)set_cut(0);
              p_ptr->black_breath = FALSE;/* accounting for new ailment.-LM- */
              
              /* Hack -- Prevent starvation */
              (void)set_food(PY_FOOD_MAX - 1);
	      
	      /* Hack -- cancel recall */
	      if (p_ptr->word_recall)
                {
                  /* Message */
                  msg_print("A tension leaves the air around you...");
                  msg_print(NULL);
                  
                  /* Hack -- Prevent recall */
                  p_ptr->word_recall = 0;
                }
              
              /* Note cause of death XXX XXX XXX */
              strcpy(p_ptr->died_from, "Cheating death");
              
              /* New stage */
              p_ptr->stage = p_ptr->home;
              
              /* New depth */
              p_ptr->depth = 0;
	      
	      /* Leaving */
	      p_ptr->leaving = TRUE;
	    }
	}
      
      /* Handle "death" */
      if (p_ptr->is_dead) break;
      
      /* Make a new level */
      generate_cave();
    }
  
  /* Close stuff */
  close_game();
  
  /* New game? - later 
  if (p_ptr->is_dead)
  if (get_check("Do you want to play again?")) game_start = TRUE; */
}


