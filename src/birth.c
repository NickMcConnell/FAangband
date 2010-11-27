/** \file birth.c 
    \brief Character birth

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
#include "cmds.h"

/**
 * Forward declare
 */
typedef struct birther birther;

/**
 * A structure to hold "rolled" information
 */
struct birther
{
	s16b age;
	s16b wt;
	s16b ht;
	s16b sc;

	s32b au;
  
  s16b stat[A_MAX];
  
  char history[4][60];
};



/**
 * The last character displayed
 */
static birther prev;


/**
 * Current stats (when rolling a character).
 */
static s16b stat_use[A_MAX];



/**
 * Save the currently rolled data for later.
 */
static void save_prev_data(void)
{
  int i;
  
  
  /*** Save the current data ***/
  
  /* Save the data */
  prev.age = p_ptr->age;
  prev.wt = p_ptr->wt;
  prev.ht = p_ptr->ht;
  prev.sc = p_ptr->sc;
  prev.au = p_ptr->au;
  
  /* Save the stats */
  for (i = 0; i < A_MAX; i++)
    {
      prev.stat[i] = p_ptr->stat_max[i];
    }
  
  /* Save the history */
  for (i = 0; i < 4; i++)
    {
      strcpy(prev.history[i], p_ptr->history[i]);
    }
}


/**
 * Load the previously rolled data.
 */
static void load_prev_data(void)
{
  int i;
  
  birther temp;
  
  
  /*** Save the current data ***/
  
  /* Save the data */
  temp.age = p_ptr->age;
  temp.wt = p_ptr->wt;
  temp.ht = p_ptr->ht;
  temp.sc = p_ptr->sc;
  temp.au = p_ptr->au;
  
  /* Save the stats */
  for (i = 0; i < A_MAX; i++)
    {
      temp.stat[i] = p_ptr->stat_max[i];
    }
  
  /* Save the history */
  for (i = 0; i < 4; i++)
    {
      strcpy(temp.history[i], p_ptr->history[i]);
    }
  
  /*** Load the previous data ***/
  
  /* Load the data */
  p_ptr->age = prev.age;
  p_ptr->wt = prev.wt;
  p_ptr->ht = prev.ht;
  p_ptr->sc = prev.sc;
  p_ptr->au = prev.au;
  
  /* Load the stats */
  for (i = 0; i < A_MAX; i++)
    {
      p_ptr->stat_max[i] = prev.stat[i];
      p_ptr->stat_cur[i] = prev.stat[i];
    }
  
  /* Load the history */
  for (i = 0; i < 4; i++)
    {
      strcpy(p_ptr->history[i], prev.history[i]);
    }
  
  /*** Save the current data ***/
  
  /* Save the data */
  prev.age = temp.age;
  prev.wt = temp.wt;
  prev.ht = temp.ht;
  prev.sc = temp.sc;
  prev.au = temp.au;
  
  /* Save the stats */
  for (i = 0; i < A_MAX; i++)
    {
      prev.stat[i] = temp.stat[i];
    }
  
  /* Save the history */
  for (i = 0; i < 4; i++)
    {
      strcpy(prev.history[i], temp.history[i]);
    }
}


/**
 * Adjust a stat by an amount.
 *
 * This just uses "modify_stat_value()".
 *
 * The "auto_roll" flag selects "maximal" changes for use with the
 * auto-roller initialization code.  Otherwise, the changes are 
 * fixed.
 */
static int adjust_stat(int value, int amount, int auto_roll)
{
  return (modify_stat_value(value, amount));
}


/**
 * Roll for a characters stats
 * For efficiency, we include a chunk of "calc_bonuses()".
 */
static void get_stats(void)
{
  int i, j;
  int bonus;
  
  int dice[3 * A_MAX];
  
  int min = 7 * A_MAX;
  int max = 9 * A_MAX;

  /* Roll and verify some stats */
  while (TRUE)
    {
      /* Roll some dice */
      for (j = i = 0; i < 3 * A_MAX; i++)
        {
          /* Roll the dice */
          dice[i] = randint(3 + i % 3);
			j += dice[i];
        }
      
      /* Verify totals */
      if ((j > min) && (j < max)) break;
    }
  
  /* Roll the stats */
  for (i = 0; i < A_MAX; i++)
    {
      /* Extract 5 + 1d3 + 1d4 + 1d5 */
      j = 5 + dice[3*i] + dice[3*i+1] + dice[3*i+2];
      
      /* Save that value */
      p_ptr->stat_max[i] = j;
      
      /* Obtain a "bonus" for "race" and "class" */
      bonus = rp_ptr->r_adj[i] + cp_ptr->c_adj[i];
      
      /* Start fully healed */
      p_ptr->stat_cur[i] = p_ptr->stat_max[i];
      
      /* Efficiency -- Apply the racial/class bonuses */
      stat_use[i] = modify_stat_value(p_ptr->stat_max[i], bonus);
    }
}


/**
 * Roll for some info that the auto-roller ignores
 *
 * No characters will have HPs at level 50 that differ from the average.
 * HPs will never differ from the average by more than 2 * character hitdice
 * on any level.
 */
static void get_extra(void)
{
  int i, j;
  int final_hps;
  
  
  /* Level one */
  p_ptr->max_lev = p_ptr->lev = 1;
  
  /* Hitdice */
  p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp;
  
  /* Initial hitpoints */
  p_ptr->mhp = p_ptr->hitdie;
  
  /* Final hitpoints = hitdice + (49 * (average roll)) */
  final_hps = p_ptr->hitdie + (((PY_MAX_LEVEL + 1) * (p_ptr->hitdie + 1))/ 2);
  
  /* Pre-calculate level 1 hitdice */
  p_ptr->player_hp[0] = p_ptr->hitdie;
  
  
  /* Roll out the hitpoints */
  for (i = 1; i < PY_MAX_LEVEL - 1; i++)
    {
      /* Expected previous level's HPs. */
      int average = p_ptr->hitdie + (((i-1) * (p_ptr->hitdie + 1)) / 2);
      
      /* Difference between previous level's HPs and the average */
      int diff = average - p_ptr->player_hp[i-1];
      
      /* Make adjustments near the end or where necessary */
      if (i >= (PY_MAX_LEVEL - 6) || rand_int(p_ptr->hitdie * 2) < ABS(diff))
        {
          /* If previous level's HPs < average, bias for a large gain. */
          if (average > p_ptr->player_hp[i-1])
            {
              /* Strong bias if needed. */
              if (ABS(diff) >= p_ptr->hitdie)
                j = p_ptr->hitdie;
              
              /* Relatively small one otherwise. */
              else
                j = p_ptr->hitdie - rand_int(p_ptr->hitdie - ABS(diff));
            }
          
          /* If previous level's HPs > average, bias for a small gain. */
          else if (average < p_ptr->player_hp[i-1])
            {
              /* Strong bias if needed. */
              if (ABS(diff) >= p_ptr->hitdie)
                j = 1;
              
              /* Relatively small one otherwise. */
              else
                j = 1 + rand_int(p_ptr->hitdie - 1 - ABS(diff));
            }
          
          /* No bias necessary. */
          else j = randint(p_ptr->hitdie - 1);
        }
      
      /* Usually no bias -- average gain of half the hitdice. */
      else j = randint(p_ptr->hitdie - 1);
      
      /* Add this level's HP gain to the previous level's HPs. */
      p_ptr->player_hp[i] = p_ptr->player_hp[i-1] + j;
    }
  
  /* 
   * Final HPs are always constant. 
   */
  p_ptr->player_hp[PY_MAX_LEVEL-1] = final_hps;
}

/**
 * Get the racial history, and social class, using the "history charts".
 */
static void get_history(void)
{
  int i, n, chart, roll, social_class;
  
  char *s, *t;
  
  char buf[240];
  
  
  
  /* Clear the previous history strings */
  for (i = 0; i < 4; i++) p_ptr->history[i][0] = '\0';
  
  
  /* Clear the history text */
  buf[0] = '\0';
  
  /* Initial social class */
  social_class = randint(4);
  
  /* Starting place */
  chart = rp_ptr->hist;
  
  
  /* Process the history */
  while (chart)
    {
      /* Start over */
      i = 0;
      
      /* Roll for nobility */
      roll = randint(100);
      
      /* Get the proper entry in the table */
      while ((chart != h_info[i].chart) || (roll > h_info[i].roll)) i++;
      
      /* Get the textual history */
      strcat(buf, (h_text + h_info[i].text));
      
      /* Add in the social class */
      social_class += (int)(h_info[i].bonus) - 50;

      /* Enter the next chart */
      chart = h_info[i].next;
    }
  
  
  
  /* Verify social class */
  if (social_class > 100) social_class = 100;
  else if (social_class < 1) social_class = 1;
  
  /* Save the social class */
  p_ptr->sc = social_class;
  
  
  /* Skip leading spaces */
  for (s = buf; *s == ' '; s++) /* loop */;
  
  /* Get apparent length */
  n = strlen(s);
  
  /* Kill trailing spaces */
  while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';
  
  
  /* Start at first line */
  i = 0;
  
  /* Collect the history */
  while (TRUE)
    {
      /* Extract remaining length */
      n = strlen(s);
      
      /* All done */
      if (n < 60)
	{
	  /* Save one line of history */
	  strcpy(p_ptr->history[i++], s);
          
	  /* All done */
	  break;
	}
      
      /* Find a reasonable break-point */
      for (n = 60; ((n > 0) && (s[n-1] != ' ')); n--) /* loop */;
      
      /* Save next location */
      t = s + n;
      
      /* Wipe trailing spaces */
      while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';
      
      /* Save one line of history */
      strcpy(p_ptr->history[i++], s);
      
      /* Start next line */
      for (s = t; *s == ' '; s++) /* loop */;
    }
}

/** 
 * Get a stat for the autoroller - adapted from askfor_aux() -NRM- 
 */
bool get_stat(char *buf, int val)
{
  int y, x, wid, minval, len = 9;
  
  int k = 0;
  
  event_type ke;
  
  bool changed = FALSE;
  bool done = FALSE;

  /* Set max width */
  wid = (small_screen ? 48 : 80);

  /* Convert to local format */
  if (val > 18) val = 18 + (val - 18) / 10;
  minval = val;
  
  ke.key = '\0';
  
  /* Locate the cursor */
  Term_locate(&x, &y);
  
  /* Paranoia -- check len */
  if (len < 1) len = 1;
  
  /* Paranoia -- check column */
  if ((x < 0) || (x >= wid)) x = 0;
  
  /* Restrict the length */
  if (x + len > wid) len = wid - x;
  
  /* Paranoia -- Clip the default entry */
  buf[len-1] = '\0';
  
  /* Display the default answer */
  Term_erase(x, y, len);
  Term_putstr(x, y, -1, TERM_YELLOW, buf);
  
  /* Process input */
  while (!done)
    {
      char *s;
      char temp[10];

      /* Place cursor */
      Term_gotoxy(x + k, y);
      
      /* Get a key */
      ke = inkey_ex();
      
      /* Analyze the key */
      switch (ke.key)
        {
        case ESCAPE:
          {
            k = 0;
            done = TRUE;
            break;
          }
          
        case '\n':
        case '\r':
          {
            k = strlen(buf);
            done = TRUE;
            break;
          }
          
        case 0x7F:
        case '\010':
          {
            if (k > 0) k--;
            break;
          }
          
	case '-':
	  {
	    val--;
	    changed = TRUE;
	    break;
	  }
	case '+': 
	  {
	    val++;
	    changed = TRUE;
	    break;
	  }
        default:
          {
            if ((k < len - 1) && (isprint(ke.key)))
              {
                buf[k++] = ke.key;
                buf[k] = '\0';
              }
            else
              {
                bell("Illegal edit key!");
              }
            break;
          }
        }
      
      /* Look for changes */
      if (changed)
        {
          if (val > 18)
            {
              sprintf(buf, "18/%01d0", (val - 18));
            }
          else
            {
              sprintf(buf, "%2d", val);
            }
          changed = FALSE;
          k = strlen(buf);
        }

      /* Terminate */
      buf[k] = '\0';

      /* Re-synchronise */
      strcpy(temp, buf);

      /* Hack -- add a fake slash */
      strcat(temp, "/");
      
      /* Hack -- look for the "slash" */
      s = strchr(temp, '/');
      
      /* Hack -- Nuke the slash */
      *s++ = '\0';
      
      /* Hack -- Extract an input */
      val = atoi(temp) + atoi(s) / 10;

      /* Check it */
      if (val > minval + 9)
        {
          val = minval + 9;
          k = 0;
          done = FALSE;
          buf[k] = '\0';
        }
      
      /* Update the entry */
      Term_erase(x, y, len);
      Term_putstr(x, y, -1, TERM_WHITE, buf);
    }

  if (ke.key == ESCAPE) return FALSE;
  else return TRUE;
}


/**
 * Sets the character's starting level -NRM-
 */

static void get_level(void)
{

  /* Check if they're an "advanced race" */
  if ((rp_ptr->start_lev - 1) && (!adult_thrall) && (!adult_dungeon))
    {
      /* Add the experience */
      p_ptr->exp = player_exp[rp_ptr->start_lev - 2];
      p_ptr->max_exp = player_exp[rp_ptr->start_lev - 2];
      
      /* Set the level */
      p_ptr->lev = rp_ptr->start_lev;
      p_ptr->max_lev = rp_ptr->start_lev;
    }
  else /* Paranoia */
    {
      /* Add the experience */
      p_ptr->exp = 0;
      p_ptr->max_exp = 0;
      
      /* Set the level */
      p_ptr->lev = 1;
      p_ptr->max_lev = 1;
    }

  /* Set home town */
  if (adult_thrall) p_ptr->home = 0;
  else if (adult_dungeon) p_ptr->home = 1;
  else p_ptr->home = towns[rp_ptr->hometown];
}




/**
 * Computes character's age, height, and weight
 */
static void get_ahw(void)
{
  /* Calculate the age */
  p_ptr->age = rp_ptr->b_age + randint(rp_ptr->m_age);
  
  /* Calculate the height/weight for males */
  if (p_ptr->psex == SEX_MALE)
    {
      p_ptr->ht = Rand_normal(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
      p_ptr->wt = Rand_normal(rp_ptr->m_b_wt, rp_ptr->m_m_wt);
    }
  
  /* Calculate the height/weight for females */
  else if (p_ptr->psex == SEX_FEMALE)
    {
      p_ptr->ht = Rand_normal(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
      p_ptr->wt = Rand_normal(rp_ptr->f_b_wt, rp_ptr->f_m_wt);
    }
}




/**
 * Get the player's starting money
 */
static void get_money(void)
{
  int i;
  
  int gold;
  
  /* Social Class determines starting gold */
  gold = (p_ptr->sc * 6) + randint(100) + 300;
  
  /* Process the stats */
  for (i = 0; i < A_MAX; i++)
    {
      /* Mega-Hack -- reduce gold for high stats */
      if (stat_use[i] >= 18+50) gold -= 300;
      else if (stat_use[i] >= 18+20) gold -= 200;
      else if (stat_use[i] > 18) gold -= 150;
      else gold -= (stat_use[i] - 8) * 10;
    }
  
  /* Minimum 100 gold */
  if (gold < 100) gold = 100;
  
  /* She charmed the banker into it! -CJS- */
  /* Mum and Dad figure she won't blow it on beer! -LM- */
  if (p_ptr->psex == SEX_FEMALE) gold += 50; /* restored in Oangband. */
  
  /* Save the gold */
  p_ptr->au = gold;
}



/**
 * Clear all the global "character" data
 */
static void player_wipe(bool really_wipe)
{
  int i;
  
  byte psex, prace, pclass;
  
  if (really_wipe)
    {
      psex = 0;
      prace = 0;
      pclass = 0;
    }
  else
    {
      /* Backup the player choices */
      psex = p_ptr->psex;
      prace = p_ptr->prace;
      pclass = p_ptr->pclass;
    }
  
  /* Wipe the player */
  (void)WIPE(p_ptr, player_type);
  
  /* Restore the choices */
  p_ptr->psex = psex;
  p_ptr->prace = prace;
  p_ptr->pclass = pclass;
  
  /* Clear the inventory */
  for (i = 0; i < INVEN_TOTAL; i++)
    {
      object_wipe(&inventory[i]);
    }
  
  
  /* Start with no artifacts made yet */
  for (i = 0; i < z_info->a_max; i++)
    {
      artifact_type *a_ptr = &a_info[i];
      a_ptr->creat_turn = 0;
      a_ptr->p_level = 0;
    }
  
  
  /* Start with no quests */
  for (i = 0; i < MAX_Q_IDX; i++)
    {
      q_list[i].stage = 0;
    }
  
  /* Mim */
  q_list[0].stage = 170;
  
  /* Glaurung */
  q_list[1].stage = 247;
  
  /* Ungoliant */
  q_list[2].stage = 211;
  
  /* Sauron */
  q_list[3].stage = 318;
  
  /* Morgoth */
  q_list[4].stage = 384;

  /* Start with no score */
  p_ptr->score = 0;
  
  /* Reset the "objects" */
  for (i = 1; i < z_info->k_max; i++)
    {
      object_kind *k_ptr = &k_info[i];
      
      /* Reset "tried" */
      k_ptr->tried = FALSE;
      
      /* Reset "aware" */
      k_ptr->aware = FALSE;
    }
  
  
  /* Reset the "monsters" */
  for (i = 1; i < z_info->r_max; i++)
    {
      monster_race *r_ptr = &r_info[i];
      monster_lore *l_ptr = &l_list[i];
      
      /* Hack -- Reset the counter */
      r_ptr->cur_num = 0;
      
      /* Hack -- Reset the max counter */
      r_ptr->max_num = 100;
      
      /* Hack -- Reset the max counter */
      if (r_ptr->flags1 & (RF1_UNIQUE)) r_ptr->max_num = 1;
      
      /* Clear player kills */
      l_ptr->pkills = 0;
    }
  
  
  /* Hack -- Well fed player */
  p_ptr->food = PY_FOOD_FULL - 1;
  
  /* Clear Specialty Abilities */
  for (i = 0; i < MAX_SPECIALTIES; i++) 
    p_ptr->specialty_order[i] = SP_NO_SPECIALTY;
  
  /* None of the spells have been learned yet */
  for (i = 0; i < 64; i++) p_ptr->spell_order[i] = 99;

  /* Player has no sensation ID knowledge */
  p_ptr->id_obj = 0L;
  p_ptr->id_other = 0L;

  /* Wipe the notes */
  for (i = 0; i < NOTES_MAX_LINES; i++)
    {
      notes[i].turn = 0;
    }
}

/**
 * Upgrade weapons (and potentially other items) for "advanced" races 
 */
static void object_upgrade(object_type *o_ptr)
{
  int i;

  if ((o_ptr->tval >= rp_ptr->re_mint) && (o_ptr->tval <= rp_ptr->re_maxt))
    {      
      o_ptr->name2 = rp_ptr->re_id;
      o_ptr->to_h = rp_ptr->re_skde;
      o_ptr->to_d = rp_ptr->re_skde;
      o_ptr->to_a = rp_ptr->re_ac;

      /* Check for ego item properties */
      if (o_ptr->name2)
	{
	  ego_item_type *e_ptr = &e_info[o_ptr->name2];
	  
	  /* Allocate appropriate ego properties here.  This is a big hack, 
	   * but hard to avoid */
	  switch (o_ptr->name2)
	    { 
	    case EGO_NOGROD:
	      {
		o_ptr->bonus_stat[A_STR] = rp_ptr->re_bonus;
		o_ptr->bonus_other[P_BONUS_SPEED] = rp_ptr->re_bonus;
		o_ptr->bonus_other[P_BONUS_TUNNEL] = rp_ptr->re_bonus;
		/* Nogrod weapons have an extra dice and are light */
		if (((o_ptr->dd + 1) * o_ptr->ds) < 41)
		  {
		    o_ptr->dd += 1; 
		  }
		else
		  {
		    /* if we can't get an extra dice then weapon is even 
		     * lighter */
		    o_ptr->weight = (5 * o_ptr->weight / 6);
		  }
		
		o_ptr->weight = (4 * o_ptr->weight / 5);
		break;
	      }
	    case EGO_NOLDOR:
	      {
		o_ptr->bonus_stat[A_STR] = rp_ptr->re_bonus;
		o_ptr->bonus_stat[A_DEX] = rp_ptr->re_bonus;
		o_ptr->bonus_stat[A_CON] = rp_ptr->re_bonus;
		break;
	      }
	    case EGO_DORIATH:
	      {
		o_ptr->bonus_other[P_BONUS_STEALTH] = rp_ptr->re_bonus;
		o_ptr->flags_obj |= OF_SUSTAIN_CHR;
	      }
	    case EGO_GONDOLIN:
	      {
		o_ptr->flags_obj |= OF_FEATHER;
	      }
	    }
	  
	  /* Get flags */
	  o_ptr->flags_obj |= e_ptr->flags_obj;
	  o_ptr->flags_curse |= e_ptr->flags_curse;

	  for (i = 0; i < MAX_P_RES; i++)
	    {
	      /* Get the resistance value */
	      o_ptr->percent_res[i] = e_ptr->percent_res[i];
	    }
	  for (i = 0; i < MAX_P_SLAY; i++)
	    {
	      /* Get the slay value */
	      o_ptr->multiple_slay[i] = e_ptr->multiple_slay[i];
	    }

	  for (i = 0; i < MAX_P_BRAND; i++)
	    {
	      /* Get the slay value */
	      o_ptr->multiple_brand[i] = e_ptr->multiple_brand[i];
	    }
	  /* Hack - Now we know about the ego-item type */
	  e_ptr->everseen = TRUE;

	}
    }
}



/**
 * Try to wield everything wieldable in the inventory.
 */
static void wield_all(void)
{
  object_type *o_ptr;
  object_type *i_ptr;
  object_type object_type_body;
  
  int slot;
  int item;
  
  /* Scan through the slots backwards */
  for (item = INVEN_PACK - 1; item >= 0; item--)
    {
      o_ptr = &inventory[item];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Make sure we can wield it and there's nothing else in that slot */
      slot = wield_slot(o_ptr);
      if (slot < INVEN_WIELD) continue;
      if (inventory[slot].k_idx) continue;
      
      /* Get local object */
      i_ptr = &object_type_body;
      object_copy(i_ptr, o_ptr);
      
      /* Modify quantity (except for ammo) */
      if ((i_ptr->tval != TV_SHOT) && (i_ptr->tval != TV_ARROW) && 
	  (i_ptr->tval != TV_BOLT)) 
	i_ptr->number = 1;
      
      /* Decrease the item (from the pack) */
      if (item >= 0)
	{
	  inven_item_increase(item, 0 - i_ptr->number);
	  inven_item_optimize(item);
	}
      
      /* Decrease the item (from the floor) */
      else
	{
	  floor_item_increase(0 - item, 0 - i_ptr->number);
	  floor_item_optimize(0 - item);
	}
      
      /* Get the wield slot */
      o_ptr = &inventory[slot];
      
      /* Wear the new stuff */
      object_copy(o_ptr, i_ptr);
      
      /* Increase the weight */
      p_ptr->total_weight += i_ptr->weight;
      
      /* Increment the equip counter by hand */
      p_ptr->equip_cnt++;
    }
  
  return;
}

/**
 * Init players with some belongings
 *
 * Having an item makes the player "aware" of its purpose.
 */
static void player_outfit(void)
{
  int i;
  const start_item *e_ptr;
  object_type *i_ptr;
  object_type object_type_body;
  
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Hack -- Give the player some food */
  if (adult_thrall)
    object_prep(i_ptr, lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
  else 
    object_prep(i_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
  i_ptr->number = (byte)rand_range(3, 7);
  object_aware(i_ptr);
  object_known(i_ptr);
  apply_autoinscription(i_ptr);
  (void)inven_carry(i_ptr);
  k_info[i_ptr->k_idx].everseen = TRUE;
  
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Hack -- Give the player some torches */
  object_prep(i_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
  i_ptr->number = (byte)rand_range(3, 7);
  i_ptr->pval = rand_range(3, 7) * 500;
  object_aware(i_ptr);
  object_known(i_ptr);
  apply_autoinscription(i_ptr);
  (void)inven_carry(i_ptr);
  k_info[i_ptr->k_idx].everseen = TRUE;
  
  /* Hack -- Give the player his equipment */
  for (i = 0; i < MAX_START_ITEMS; i++)
    {
      /* Access the item */
      e_ptr = &(cp_ptr->start_items[i]);
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Hack	-- Give the player an object */
      if (e_ptr->tval > 0)
	{
	  /* Get the object_kind */
	  int k_idx = lookup_kind(e_ptr->tval, e_ptr->sval);
	  
	  /* Valid item? */
	  if (!k_idx) continue;
	  
	  /* Prepare the item */
	  object_prep(i_ptr, k_idx);
	  i_ptr->number = (byte)rand_range(e_ptr->min, e_ptr->max);
          
          /* Nasty hack for "advanced" races -NRM- */
          if ((!adult_thrall) && (!adult_dungeon)) object_upgrade(i_ptr);
          
          object_aware(i_ptr);
          object_known(i_ptr);
	  apply_autoinscription(i_ptr);
	  (void)inven_carry(i_ptr);
	  k_info[k_idx].everseen = TRUE;
        }
    }

  /* Dungeon gear for escaping thralls */
  if (adult_thrall)
    {
      /* Nice amulet */
      object_prep(i_ptr, lookup_kind(TV_AMULET, SV_AMULET_AMETHYST));
      i_ptr->bonus_other[P_BONUS_M_MASTERY] = 4;
      i_ptr->flags_obj |= OF_TELEPATHY;
      object_aware(i_ptr);
      object_known(i_ptr);
      apply_autoinscription(i_ptr);
      (void)inven_carry(i_ptr);
      k_info[i_ptr->k_idx].everseen = TRUE;

      /* Detection */
      object_prep(i_ptr, lookup_kind(TV_STAFF, SV_STAFF_DETECTION));
      object_aware(i_ptr);
      object_known(i_ptr);
      apply_autoinscription(i_ptr);
      (void)inven_carry(i_ptr);
      k_info[i_ptr->k_idx].everseen = TRUE;

      /* Mapping */
      object_prep(i_ptr, lookup_kind(TV_ROD, SV_ROD_MAPPING));
      object_aware(i_ptr);
      object_known(i_ptr);
      apply_autoinscription(i_ptr);
      (void)inven_carry(i_ptr);
      k_info[i_ptr->k_idx].everseen = TRUE;

      /* Destruction */
      object_prep(i_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_STAR_DESTRUCTION));
      i_ptr->number = 5;
      object_aware(i_ptr);
      object_known(i_ptr);
      apply_autoinscription(i_ptr);
      (void)inven_carry(i_ptr);
      k_info[i_ptr->k_idx].everseen = TRUE;

      /* Identify */
      object_prep(i_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_IDENTIFY));
      i_ptr->number = 25;
      object_aware(i_ptr);
      object_known(i_ptr);
      apply_autoinscription(i_ptr);
      (void)inven_carry(i_ptr);
      k_info[i_ptr->k_idx].everseen = TRUE;
    }

  /* Now try wielding everything */
  wield_all();
}


/** Locations of the tables on the screen */
#define HEADER_ROW       (small_screen ? 0 : 1)
#define QUESTION_ROW     (small_screen ? 8 : 7)
#define TABLE_ROW        10
#define QUESTION_COL     (small_screen ? 0 : 2)
#define SEX_COL          2
#define RACE_COL         14
#define RACE_AUX_COL     (small_screen ? 19 : 29)
#define CLASS_COL        29
#define CLASS_AUX_COL    (small_screen ? 30 : 50)
#define ROLLER_COL       44
#define SEX_COL_SML      0
#define RACE_COL_SML     4
#define CLASS_COL_SML    19
#define ROLLER_COL_SML   30

/**
 * Clear the previous question
 */
static void clear_question(void)
{
  int i;
  
  for (i = QUESTION_ROW; i < TABLE_ROW; i++)
    {
      /* Clear line, position cursor */
      Term_erase(0, i, 255);
    }
}
/* =================================================== */

/* gender/race/classs menu selector */

/**
 * Display additional information about each race during the selection.
 */
static void race_aux_hook(int race, void *db, const region *reg)
{
  int i;
  char s[50];
  byte color;
  
  if (race == z_info->p_max) return;
  
  /* Display relevant details. */
  for (i = 0; i < A_MAX; i++)
    {
      strnfmt(s, sizeof(s), "%s%+d", stat_names_reduced[i],
	      p_info[race].r_adj[i]);
      Term_putstr(RACE_AUX_COL, TABLE_ROW + i, -1, TERM_WHITE, s);
    }
  
  strnfmt(s, sizeof(s), "Hit die: %d ", p_info[race].r_mhp);
  Term_putstr(RACE_AUX_COL, TABLE_ROW + A_MAX, -1, TERM_WHITE, s);
  strnfmt(s, sizeof(s), "Infravision: %d ft ", p_info[race].infra * 10);
  Term_putstr(RACE_AUX_COL, TABLE_ROW + A_MAX + 1, -1, TERM_WHITE, s);
  if (!adult_dungeon)
    {
      strnfmt(s, sizeof(s),"Difficulty: ");
      Term_putstr(RACE_AUX_COL, TABLE_ROW + A_MAX + 2, -1, TERM_WHITE, s);
      
      /* Race difficulty factor */
      strnfmt(s, sizeof(s), "Level %d       ",p_info[race].difficulty );
      
      /* Color code difficulty factor */
      if (p_info[race].difficulty < 3) color = TERM_GREEN;
      else if (p_info[race].difficulty < 15) color = TERM_ORANGE;
      else color = TERM_RED;
      
      Term_putstr(RACE_AUX_COL, TABLE_ROW + A_MAX + 3, -1, color, s);
      strnfmt(s, sizeof(s),"Home town: ");
      Term_putstr(RACE_AUX_COL, TABLE_ROW + A_MAX + 4, -1, TERM_WHITE, s);
      strnfmt(s, sizeof(s), "%-15s", 
	      locality_name[stage_map[towns[p_info[race].hometown]][LOCALITY]]);
      
      Term_putstr(RACE_AUX_COL, TABLE_ROW + A_MAX + 5, -1, color, s);
    }
}


/**
 * Display additional information about each class during the selection.
 */
static void class_aux_hook(int class_idx, void *db, const region *loc)
{
  int i;
  char s[128];
  
  if (class_idx == z_info->c_max) return;
  
  /* Display relevant details. */
  for (i = 0; i < A_MAX; i++)
    {
      strnfmt(s, sizeof(s), "%s%+d", stat_names_reduced[i],
	      c_info[class_idx].c_adj[i] + p_info[p_ptr->prace].r_adj[i]);
      Term_putstr(CLASS_AUX_COL, TABLE_ROW + i, -1, TERM_WHITE, s);
    }
  
  strnfmt(s, sizeof(s), "Hit die: %d ", 
	  c_info[class_idx].c_mhp + p_info[p_ptr->prace].r_mhp);
  Term_putstr(CLASS_AUX_COL, TABLE_ROW + A_MAX, -1, TERM_WHITE, s);
}


static region gender_region = {SEX_COL, TABLE_ROW, 15, -2};
static region race_region = {RACE_COL, TABLE_ROW, 15, -2};
static region class_region = {CLASS_COL, TABLE_ROW, 19, -2};
static region roller_region = {ROLLER_COL, TABLE_ROW, 21, -2};
static region mode_region = {ROLLER_COL, TABLE_ROW + 4, 21, -2};
/* There must be a better way to do this... */
static region gender_region_sml = {SEX_COL_SML, TABLE_ROW, 15, -2};
static region race_region_sml = {RACE_COL_SML, TABLE_ROW, 15, -2};
static region class_region_sml = {CLASS_COL_SML, TABLE_ROW, 19, -2};
static region roller_region_sml = {ROLLER_COL_SML, TABLE_ROW, 21, -2};
static region mode_region_sml = {ROLLER_COL_SML, TABLE_ROW + 4, 21, -2};


/** Event handler implementation */
static bool handler_aux(char cmd, int oid, byte *val, int max, int mask, 
			cptr topic)
{
  if (cmd == '\xff' || cmd == '\r') 
    {
      *val = oid;
    }
  else if(cmd == '*') 
    {
      for(;;) 
	{
	  oid = rand_int(max);
	  *val = oid;
	  if(mask & (1L << oid)) break;
	}
    }
  else if(cmd == KTRL('X')) quit(NULL);
  else if(cmd == '?') 
    {
      char buf[80];
      strnfmt(buf, sizeof(buf), "%s#%s", "raceclas.txt", topic);
      screen_save();
      show_file(buf, NULL, 0, 0);
      screen_load();
    }
  else return FALSE;

  sp_ptr = &sex_info[p_ptr->psex];
  rp_ptr = &p_info[p_ptr->prace];
  cp_ptr = &c_info[p_ptr->pclass];
  mp_ptr = &magic_info[p_ptr->pclass];
  return TRUE;
}

/* GENDER */
/** Display a gender */
static void display_gender(menu_type *menu, int oid, bool cursor,
			   int row, int col, int width)
{
  byte attr = curs_attrs[CURS_KNOWN][0 != cursor];
  c_put_str(attr, sex_info[oid].title, row, col);
}

static bool gender_handler(char cmd, void *db, int oid)
{
  return handler_aux(cmd, oid, &p_ptr->psex, SEX_MALE+1,
		     0xffffffff, sex_info[oid].title);
}

/* RACE */
static void display_race(menu_type *menu, int oid, bool cursor,
			 int row, int col, int width)
{
  byte attr = curs_attrs[CURS_KNOWN][0 != cursor];
  c_put_str(attr, p_name + p_info[oid].name, row, col);
}

static bool race_handler(char cmd, void *db, int oid)
{
  return handler_aux(cmd, oid, &p_ptr->prace, z_info->p_max,
		     0xffffffff, p_name + p_info[oid].name);
}

/* CLASS */
static void display_class(menu_type *menu, int oid, bool cursor,
			  int row, int col, int width)
{
  byte attr = curs_attrs[0 != (rp_ptr->choice & (1L << oid))][0 != cursor];
  c_put_str(attr, c_name + c_info[oid].name, row, col);
}

static bool class_handler(char cmd, void *db, int oid)
{
  return handler_aux(cmd, oid, &p_ptr->pclass, z_info->c_max,
		     (rp_ptr->choice), c_name + c_info[oid].name);
}

/* ROLLER */
static void display_roller(menu_type *menu, int oid, bool cursor,
			   int row, int col, int width)
{
  byte attr = curs_attrs[CURS_KNOWN][0 != cursor];
  const char *str;
  
  if (oid == 0)
    str = "Point-based";
  else if (oid == 1)
    str = "Autoroller";
  else
    str = "Standard roller";
  
  c_prt(attr, str, row, col);
}


static byte roller_type = 0;
#define ROLLER_POINT    0
#define ROLLER_AUTO     1
#define ROLLER_STD      2

static bool roller_handler(char cmd, void *db, int oid)
{
  if (cmd == '\xff' || cmd == '\r')
    roller_type = oid;
  else if (cmd == '*')
    roller_type = 2;
  else if(cmd == '=')
    do_cmd_options();
  else if(cmd == KTRL('X'))
    quit(NULL);
  else
    return FALSE;
  
  return TRUE;
}

/* MODE */
static byte mode_type = 0;
static void display_mode(menu_type *menu, int oid, bool cursor,
			   int row, int col, int width)
{
  byte attr = curs_attrs[CURS_KNOWN][0 != cursor];
  const char *str = NULL;
  
  if (oid == 0)
    str = "Use current options";
  else if (oid == 1)
    str = "Go to option screen";
  
  c_prt(attr, str, row, col);
}


static bool mode_handler(char cmd, void *db, int oid)
{
  int i;

  if (cmd == '\xff' || cmd == '\r')
    {
      mode_type = oid;
    }
  else if (cmd == '=')
    {
      do_cmd_options();
    }
  else if(cmd == KTRL('X'))
    quit(NULL);
  else
    return FALSE;

  if (mode_type == 1) do_cmd_options();
	  
  /* Set adult options from birth options */
  for (i = OPT_birth_start; i < OPT_birth_end + 1; i++)
    {
      op_ptr->opt[OPT_adult_start + (i - OPT_birth_start)] = 
	op_ptr->opt[i];
    }
  
  return TRUE;
}


static const menu_iter menu_defs[] = {
  { 0, 0, 0, display_gender, gender_handler },
  { 0, 0, 0, display_race, race_handler },
  { 0, 0, 0, display_class, class_handler },
  { 0, 0, 0, display_roller, roller_handler },
  { 0, 0, 0, display_mode, mode_handler },
};

/** Menu display and selector */

#define ASEX 0
#define ARACE 1
#define ACLASS 2
#define AROLL 3
#define AMODE 4


static bool choose_character(void)
{
  int i = 0;
  
  const region *regions[] = { &gender_region, &race_region, &class_region, 
			      &roller_region, &mode_region };
  byte *values[5]; /* { &p_ptr->psex, &p_ptr->prace, &p_ptr->pclass }; */
  int limits[5]; /* { SEX_MALE +1, z_info->p_max, z_info->c_max }; */
  
  menu_type menu;
  
  const char *hints[] =
    {
      "Males are heavier; females start with more gold.",
      "",
      "Your 'race' determines your home town,", 
      "and various other intrinsic factors and bonuses.",
      "Your class determines your magic realm (if any),", 
      "and various other intrinsic factors and bonuses.",
      "Your choice of character generation.",
      "Point-based is recommended.",
      "If you want to change birth options, go to the",
      "options screen and choose 'b'."
    };
  
  typedef void (*browse_f) (int oid, void *, const region *loc);
  browse_f browse[] = {NULL, race_aux_hook, class_aux_hook, NULL, NULL };

  /* Hack! */  
  if (small_screen) 
    {
      regions[0] = &gender_region_sml;
      regions[1] =  &race_region_sml;
      regions[2] =  &class_region_sml;
      regions[3] = &roller_region_sml;
      regions[4] = &mode_region_sml;
    }

  /* Stupid ISO C array initialization. */
  values[ASEX] = &p_ptr->psex;
  values[ARACE] = &p_ptr->prace;
  values[ACLASS] = &p_ptr->pclass;
  values[AROLL] = &roller_type;
  values[AMODE] = &mode_type;
  limits[ASEX] = SEX_MALE + 1;
  limits[ARACE] = z_info->p_max;
  limits[ACLASS] = z_info->c_max;
  limits[AROLL] = 3;
  limits[AMODE] = 2;
  
  WIPE(&menu, menu);
  menu.cmd_keys = "?*=\r\n\x18";		 /* ?, *, =, \n, <ctl-X> */
  menu.selections = lower_case;
  
  /* Set up buttons */
  normal_screen = FALSE;
  kill_all_buttons();
  add_button("Exit", KTRL('X'));
  add_button("ESC", ESCAPE);
  add_button("Help",'?');
  add_button("Options",'=');
  add_button("Random",'*');
  update_statusline();

  while (i < (int)N_ELEMENTS(menu_defs))
    {
      event_type cx;
      int cursor = *values[i];
      
      menu.flags = MN_DBL_TAP;
      menu.count = limits[i];
      menu.browse_hook = browse[i];
      menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_defs[i], regions[i]);
      
      clear_question();
      Term_putstr(QUESTION_COL, QUESTION_ROW, -1, TERM_YELLOW, hints[2 * i]);
      Term_putstr(QUESTION_COL, QUESTION_ROW + 1, -1, TERM_YELLOW, 
		  hints[2 * i + 1]);
      
      cx = menu_select(&menu, &cursor, 0);
      
      if (cx.key == ESCAPE || cx.type == EVT_BACK)
	{
	  if (i > 0) 
	    {
	      /* Move back one menu */
	      *values[i] = cursor;
	      region_erase(regions[i]);
	      i--;
	      update_statusline();
	    }
	}
      else if (cx.key == '*')
	{
	  /* Force refresh */
	  Term_key_push('6');
	  continue;
	}
      
      /* Selection! */
      else if(cx.key == '\r' || cx.key == '\n' || cx.key == '\xff')
	i++;
    }

  /* Kill buttons */
  kill_button(ESCAPE);
  kill_button(KTRL('X'));
  kill_button('?');
  kill_button('=');
  kill_button('*');
  update_statusline();
  
  return TRUE;
}


/**
 * Helper function for 'player_birth()'.
 *
 * This function allows the player to select a sex, race, and class, and
 * modify options (including the birth options).
 */
static bool player_birth_aux_1(void)
{
  int i;
  
  /*** Instructions ***/
  
  /* Clear screen */
  Term_clear();
  
  /* Output to the screen */
  text_out_hook = text_out_to_screen;
  
  /* Indent output */
  text_out_indent = QUESTION_COL;
  Term_gotoxy(QUESTION_COL, HEADER_ROW);
  
  /* Display some helpful information */
  text_out_c(TERM_L_BLUE,
	     "Please select your character from the menu below:\n\n");
  text_out("Use the ");
  text_out_c(TERM_L_GREEN, "movement keys");
  text_out(" to scroll the menu, ");
  text_out_c(TERM_L_GREEN, "Enter");
  text_out(" to select the current menu item, '");
  text_out_c(TERM_L_GREEN, "*");
  text_out("' for a random menu item, '");
  text_out_c(TERM_L_GREEN, "ESC");
  text_out("' to step back through the birth process, '");
  text_out_c(TERM_L_GREEN, "?");
  text_out("' for help, or '");
  text_out_c(TERM_L_GREEN, "Ctrl-X");
  text_out("' to quit.");
  
  /* Reset text_out() indentation */
  text_out_indent = 0;
  
  if (!choose_character()) return FALSE;
  

  /* Set adult options from birth options */
  for (i = OPT_birth_start; i < OPT_cheat_start; i++)
  op_ptr->opt[OPT_adult_start + (i - OPT_birth_start)] = op_ptr->opt[i];
  
  /* Reset score options from cheat options */
  for (i = OPT_cheat_start; i < OPT_adult_start; i++)
    op_ptr->opt[OPT_score_start + (i - OPT_cheat_start)] = op_ptr->opt[i];
  
  /* Reset squelch bits */
  for (i = 0; i < z_info->k_max; i++)
    k_info[i].squelch = FALSE;
  
  /* Reset ego squelch */
  for (i = 0; i < z_info->e_max; i++)
    e_info[i].squelch = FALSE;
  
  /* Clear the squelch bytes */
  for (i = 0; i < TYPE_MAX; i++)
    squelch_level[i] = 0;
  
  
  /* Done */
  return (TRUE);
}

/** =================================================== */

/**
 * Initial stat costs (initial stats always range from 10 to 18 inclusive).
 */
#define MIN_POINT_STAT_VALUE 10 /* Minimum stat value - no points used */
#define MAX_POINT_STAT_VALUE 18 /* Maximum stat value - full points used */
#define BUY_POINTS 48 /* Number of points available to buy stats */
#define GOLD_POINT 50 /* Each stat point is worth this much gold */

/**
 * Initial stat costs (initial stats always range from 10 to 18 inclusive).
 */
static int birth_stat_costs[(MAX_POINT_STAT_VALUE-MIN_POINT_STAT_VALUE)+1] = 
  { 0, 1, 2, 4, 7, 11, 16, 22, 30 };


/**
 * Helper function for 'player_birth()'.
 *
 * This function handles "point-based" character creation.
 *
 * each costing a certain amount of points (as above), from a pool of
 * BUY_POINTS available points, to which race/class modifiers are then applied.
 * available points, to which race/class modifiers are then applied.
 *
 * Each unused point is converted into GOLD_POINT gold pieces.
 */
static int player_birth_aux_2(void)
{
  int i;
  
  int row;
  int col;
  
  int stat = 0;
  
  int stats[A_MAX];
  
  int cost;
  
  char ch;
  event_type ke;
  
  char buf[80];
  
  bool first_time = TRUE;
  
  /* Set row, column */
  row = (small_screen ? 4 : 1);
  col = (small_screen ? 11 : 42);
  
  /* Initialize stats */
  for (i = 0; i < A_MAX; i++)
    {
      /* Initial stats */
      stats[i] = MIN_POINT_STAT_VALUE;
    }
  
  
  /* Roll for base hitpoints */
  get_extra();
  
  /* Roll for age/height/weight */
  get_ahw();
  
  /* Roll for social class */
  get_history();
  
  /* Raise level if necessary */
  get_level();
  
  /* Interact */
  while (1)
    {
      /* Reset cost */
      cost = 0;
      
      /* Process stats */
      for (i = 0; i < A_MAX; i++)
        {
          /* Reset stats */
          p_ptr->stat_cur[i] = p_ptr->stat_max[i] = stats[i];
          
          /* Total cost */
          cost += birth_stat_costs[stats[i] - MIN_POINT_STAT_VALUE];
        }
      
      /* Restrict cost */
      if (cost > BUY_POINTS)
        {
          /* Warning */
          bell("Excessive stats!");
	  
	  /* Reduce stat */
	  stats[stat]--;
	  
	  /* Recompute costs */
	  continue;
        }
      
      /* Gold is inversely proportional to cost */
      p_ptr->au = (GOLD_POINT * (BUY_POINTS - cost)) + 100;
      
      /* She charmed the banker into it! */
      /* Mum and Dad figure she won't blow it on beer! */
      if (p_ptr->psex == SEX_FEMALE) p_ptr->au += 50; 
      
      /* Calculate the bonuses and hitpoints */
      p_ptr->update |= (PU_BONUS | PU_HP);

      /* Update stuff */
      update_stuff();
      
      /* Fully healed */
      p_ptr->chp = p_ptr->mhp;
      
      /* Fully rested */
      p_ptr->csp = p_ptr->msp;
      
      /* Display the player */
      if (small_screen)
	display_player_sml();
      else
	display_player(0);
      
      /* Display the costs header */
      put_str("Cost", row - 1, col + 32);
      
      /* Display the costs */
      for (i = 0; i < A_MAX; i++)
        {
          /* Display cost */
          sprintf(buf, "%4d", 
                  birth_stat_costs[stats[i] - MIN_POINT_STAT_VALUE]);
          put_str(buf, row + i, col + 32);
        }
      
      
      /* Prompt XXX XXX XXX */
      if (small_screen)
        {
          sprintf(buf, "Cost %2d/%2d.  2/8 move, 4/6 modify, ENTER accept.", 
		  cost, BUY_POINTS);
          c_put_str(TERM_VIOLET, buf, 0, 0);
        }
      else
        {
          sprintf(buf, "Total Cost %2d/%2d.  Use 2/8 to move, 4/6 to modify, ENTER to accept.", cost, BUY_POINTS);
          prt(buf, 0, 0);
        }
      
      /* Buttons */
      clear_from(Term->hgt - 2);
      kill_all_buttons();
      add_button("Back", ESCAPE);
      add_button("Done", '\r');
      add_button("Up", '8');
      add_button("Down", '2');
      add_button("Incr", '6');
      add_button("Decr", '4');
      update_statusline();

      /* Place cursor just after cost of current stat */
      Term_gotoxy(col + 36, row + stat);
      
      /* Get key */
      ke = inkey_ex();
      ch = ke.key;

      /* Go back a step, or back to the start of this step */
      if (ch == ESCAPE) 
	{
	  kill_all_buttons();
	  update_statusline();
	  if (first_time) 
	    return -1;
	  else 
	    return 0;
	}
      
      first_time = FALSE;
      
      /* Done */
      if ((ch == '\r') || (ch == '\n')) break;
      
      ch = target_dir(ch);
      
      /* Prev stat */
      if (ch == 8)
	{
	  stat = (stat + A_MAX - 1) % A_MAX;
	}
      
      /* Next stat */
      if (ch == 2)
	{
	  stat = (stat + 1) % A_MAX;
        }
      
      /* Decrease stat */
      if ((ch == 4) && (stats[stat] > MIN_POINT_STAT_VALUE))
        {
          stats[stat]--;
        }
      
      /* Increase stat */
      if ((ch == 6) && (stats[stat] < MAX_POINT_STAT_VALUE))
        {
          stats[stat]++;
        }
    }
  /* Kill buttons */
  kill_all_buttons();
  update_statusline();
  
  /* Done - advance a step*/
  return +1;
}


/**
 * Helper function for 'player_birth()'.
 *
 * This function handles "auto-rolling" and "random-rolling".
 *
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 */
static int player_birth_aux_3(bool autoroll)
{
  int i, j, m, v;
  
  bool flag;
  bool prev = FALSE;
  
  event_type ke;
  char ch;
  
  char b1 = '[';
  char b2 = ']';
  
  char buf[80];
  
  
  s16b stat_limit[A_MAX];
  
  s32b stat_match[A_MAX];
  
  s32b auto_round = 0L;
  
  s32b last_round;
  
  
  /* Clear */
  Term_clear();
  
  /*** Autoroll ***/
  
  /* Initialize */
  if (autoroll)
    {
      int maxval[A_MAX];
      int minval[A_MAX];
      int indent = (small_screen ? 0 : 5);
      
      char inp[80];
      
      
      /* Set up buttons */
      kill_all_buttons();
      add_button("ESC", ESCAPE);
      add_button("Return", '\r');
      add_button("+", '+');
      add_button("-", '-');
      update_statusline();

      /* Extra info */
      Term_putstr(indent, 3, -1, TERM_WHITE,
                  "The auto-roller will automatically ignore"); 
      Term_putstr(indent, 4, -1, TERM_WHITE,
                  "characters which do not meet the minimum values");
      Term_putstr(indent, 5, -1, TERM_WHITE,
                  "for any stats specified below.  Note that stats");
      Term_putstr(indent, 6, -1, TERM_WHITE,
                  "are not independent, so it is not possible to");
      Term_putstr(indent, 7, -1, TERM_WHITE,
                  "get perfect (or even high) values for all"); 
      Term_putstr(indent, 8, -1, TERM_WHITE,
                  "your stats.");
      
      /* Prompt for the minimum stats */
      put_str("Enter minimum values or", 10, 2);
      put_str("+ to increase, - to decrease: ", 11, 2);
      
      /* Output the maximum stats */
      for (i = 0; i < A_MAX; i++)
	{
	  /* Reset the "success" counter */
	  stat_match[i] = 0;
	  
	  /* Race/Class bonus */
	  j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i];
	  
	  /* Obtain the "maximal" stat */
	  m = adjust_stat(17, j, TRUE);
	  
	  /* Save the maximum */
	  maxval[i] = m;
	  
	  /* Obtain the "minimal" stat */
	  minval[i] = adjust_stat(8, j, TRUE);
	  
          /* Above 18 */
          if (m > 18)
	    {
	      strnfmt(inp, sizeof(inp), "(Max of 18/%02d):", (m - 18));
	    }
	  
	  /* From 3 to 18 */
	  else
	    {
	      strnfmt(inp, sizeof(inp), "(Max of %2d):", m);
	    }
	  
	  /* Prepare a prompt */
	  strnfmt(buf, sizeof(buf), "%-5s%-20s", stat_names[i], inp);
          
          /* Dump the prompt */
          put_str(buf, 12 + i, 15);
        }
      
      /* Input the minimum stats */
      for (i = 0; i < A_MAX; i++)
	{
	  /* Get a minimum stat */
	  while (TRUE)
	    {
              char *s;
              
              /* Move the cursor */
              put_str("", 12 + i, 40);
              
              /* Extract a textual format */
              
              /* Above 18 */
              if (minval[i] > 18)
                {
                  sprintf(inp, "18/%02d", (minval[i] - 18));
                }
              
              /* From 3 to 18 */
              else
                {
                  sprintf(inp, "%2d", minval[i]);
                }
              
              /* Get a response (or escape) */
              if (!get_stat(inp, minval[i]))
		{
		  if (i == 0) 
		    /* Back a step */
		    return -1;
		  else 
		    /* Repeat this step */
		    return 0;
		}
	      
	      
              
              /* Hack -- add a fake slash */
              strcat(inp, "/");
              
              /* Hack -- look for the "slash" */
              s = strchr(inp, '/');
	      
	      /* Hack -- Nuke the slash */
	      *s++ = '\0';
	      
	      /* Hack -- Extract an input */
              v = atoi(inp) + atoi(s);
              
              /* Break on valid input */
              if (v <= maxval[i]) break;
            }
          
          /* Save the minimum stat */
	  stat_limit[i] = (v > 0) ? v : 0;
	}

      /* Remove buttons */
      kill_button(ESCAPE);
      kill_button('+');
      kill_button('-');
      kill_button('\r');
      update_statusline();
    }
  
  /* Clean up */
  clear_from(10);
  
  /*** Generate ***/
  
  /* Roll */
  while (TRUE)
    {
      int col = (small_screen ? 10 : 42);
      
      /* Feedback */
      if (autoroll)
	{
	  Term_clear();
	  
	  /* Label */
	  put_str(" Limit", 2, col+5);
	  
	  /* Label */
	  put_str("  Freq", 2, col+13);
	  
	  /* Label */
	  put_str("  Roll", 2, col+24);
	  
	  /* Put the minimal stats */
	  for (i = 0; i < A_MAX; i++)
	    {
	      /* Label stats */
              put_str(stat_names[i], 3+i, col);
              
              /* Put the stat */
              cnv_stat(stat_limit[i], buf);
              c_put_str(TERM_L_BLUE, buf, 3+i, col+5);
            }
          
	  /* Note when we started */
	  last_round = auto_round;
	  
	  /* Label count */
	  put_str("Round:", 10, col+13);
	  
	  /* Indicate the state */
	  put_str("(Hit ESC to stop)", 12, col+13);
	  
	  /* Auto-roll */
	  while (1)
	    {
	      bool accept = TRUE;
	      
	      /* Get a new character */
	      get_stats();
	      
	      /* Advance the round */
	      auto_round++;
	      
	      /* Hack -- Prevent overflow */
	      if (auto_round >= 1000000L) break;
	      
	      /* Check and count acceptable stats */
	      for (i = 0; i < A_MAX; i++)
		{
		  /* This stat is okay */
		  if (stat_use[i] >= stat_limit[i])
		    {
		      stat_match[i]++;
		    }
		  
		  /* This stat is not okay */
		  else
		    {
		      accept = FALSE;
		    }
		}
	      
	      /* Break if "happy" */
	      if (accept) break;
	      
	      /* Take note every 25 rolls */
	      flag = (!(auto_round % 25L));
	      
	      /* Update display occasionally */
	      if (flag || (auto_round < last_round + 100))
		{
		  /* Put the stats (and percents) */
                  for (i = 0; i < A_MAX; i++)
                    {
                      /* Put the stat */
                      cnv_stat(stat_use[i], buf);
                      c_put_str(TERM_L_GREEN, buf, 3+i, col+24);
                      
                      /* Put the percent */
		      if (stat_match[i])
                        {
                          int p = 1000L * stat_match[i] / auto_round;
                          byte attr = (p < 100) ? TERM_YELLOW : TERM_L_GREEN;
                          sprintf(buf, "%3d.%d%%", p/10, p%10);
                          c_put_str(attr, buf, 3+i, col+13);
                        }
                      
		      /* Never happened */
		      else
			{
			  c_put_str(TERM_RED, "(NONE)", 3+i, col+13);
			}
		    }
		  
		  /* Dump round */
		  put_str(format("%10ld", auto_round), 10, col+20);
		  
                  /* Make sure they see everything */
                  Term_fresh();
                  
                  /* Delay 1/10 second */
                  if (flag) Term_xtra(TERM_XTRA_DELAY, 100);
                  
                  /* Do not wait for a key */
                  inkey_scan = TRUE;
                  
                  /* Check for a keypress */
                  if (anykey()) break;
                }
            }
        }
      
      /* Otherwise just get a character */
      else
	{
	  /* Get a new character */
	  get_stats();
	}
      
      /* Flush input */
      flush();
      
      
      /*** Display ***/
      
      /* Roll for base hitpoints */
      get_extra();
      
      /* Roll for age/height/weight */
      get_ahw();
      
      /* Roll for social class */
      get_history();
      
      /* Set level if necessary */
      get_level();
      
      /* Roll for gold */
      get_money();
      
      /* Input loop */
      while (TRUE)
	{
	  /* Calculate the bonuses and hitpoints */
	  p_ptr->update |= (PU_BONUS | PU_HP);
	  
	  /* Update stuff */
	  update_stuff();
	  
	  /* Fully healed */
	  p_ptr->chp = p_ptr->mhp;
	  
	  /* Fully rested */
	  p_ptr->csp = p_ptr->msp;
	  
	  /* Display the player */
	  if (small_screen)
	    display_player_sml();
	  else
	    display_player(0);
	  
	  /* Add buttons */
	  add_button("ESC", ESCAPE);
	  add_button("Enter", '\r');
	  add_button("r", 'r');
	  if (prev) add_button("p", 'p');
	  clear_from(Term->hgt - 2);
	  update_statusline();

	  /* Prepare a prompt (must squeeze everything in) */
	  Term_gotoxy(0, 0);
	  Term_addch(TERM_WHITE, b1);
	  Term_addstr(-1, TERM_WHITE, "'r' to reroll");
	  if (prev) Term_addstr(-1, TERM_WHITE, ", 'p' for prev");
	  Term_addstr(-1, TERM_WHITE, ", or 'Enter' to accept");
          Term_addch(TERM_WHITE, b2);
          
          /* Prompt and get a command */
          ke = inkey_ex();
          ch = ke.key;

	  /* if we're not autorolling. */
	  if (ch == ESCAPE) 
	    {
	      kill_button('r');
	      kill_button('p');
	      if (autoroll) 
		return 0;
	      else 
		return -1;
	    }
	  
	  /* 'Enter' accepts the roll */
	  if ((ch == '\r') || (ch == '\n')) break;
	  
	  /* Reroll this character */
	  if ((ch == ' ') || (ch == 'r')) break;
	  
	  /* Previous character */
	  if (prev && (ch == 'p'))
	    {
	      load_prev_data();
	      continue;
	    }
	  
	  /* Help */
	  if (ch == '?')
	    {
	      do_cmd_help();
	      continue;
	    }
	  
	  /* Warning */
	  bell("Illegal auto-roller command!");
	}
      
      /* Kill buttons */
      kill_button(ESCAPE);
      kill_button('\r');
      kill_button('r');
      kill_button('p');
      update_statusline();

      /* Are we done? */
      if ((ch == '\r') || (ch == '\n')) break;
      
      /* Save this for the "previous" character */
      save_prev_data();
      
      /* Note that a previous roll exists */
      prev = TRUE;
    }
  
  /* Clear prompt */
  clear_from(23);
  
  /* Done - move on a stage */
  return +1;
}

typedef enum 
  {
    BIRTH_RESTART = 0,
    BIRTH_QUESTIONS,
    BIRTH_STATS,
    BIRTH_NAME,
    BIRTH_FINAL_APPROVAL,
    BIRTH_ACCEPTED
  } birth_stages;


/**
 * Helper function for 'player_birth()'.
 *
 * See "display_player" for screen layout code.
 */
static void player_birth_aux(void)
{
  event_type ke;
  cptr prompt;
  birth_stages state = BIRTH_QUESTIONS;

  if (character_quickstart) state = BIRTH_STATS;

  if (small_screen)
   prompt = "['ESC' step back, 'S' restart, any key continue]";
  else
    prompt = "['ESC' to step back, 'S' to restart, any other key to continue]";

  while (1)
    {
      switch (state)
	{
	case BIRTH_RESTART:
	  {
	    player_wipe(FALSE);
	    state++;
	    break;
	  }
	  
	case BIRTH_QUESTIONS:
	  {
	    /* Race, class, etc. choices */
	    if (player_birth_aux_1()) state++;
	    break;
	  }
	  
	case BIRTH_STATS:
	  {
	    if (roller_type == ROLLER_POINT)
	      {
		/* Fill stats using point-based methods */
		state += player_birth_aux_2();
	      }
	    else
	      {
		/* Fills stats using the standard- or auto-roller */
		state += player_birth_aux_3(roller_type == ROLLER_AUTO);
	      }
	    break;
	  }
	  
        case BIRTH_NAME:
          {
	    /* Hack - clear the bottom line */
	    clear_from(Term->hgt - 1);

            /* Get a name, prepare savefile */
            if (get_name(FALSE)) 
              state++;
            else 
              state--;
	    
	    break;
	  }
	  
	case BIRTH_FINAL_APPROVAL:
	  {
	    /* Display the player */
	    if (small_screen)
	      display_player_sml();
	    else
	      display_player(0);

	    /* Prompt for it */
	    prt("", Term->hgt - 3, 0);
	    prt(prompt, Term->hgt - 2, Term->wid / 2 - strlen(prompt) / 2);
	    prt("", Term->hgt - 1, 0);
	    
	    /* Buttons */
	    kill_all_buttons();
	    add_button("Continue", 'q');
	    add_button("ESC", ESCAPE);
	    add_button("S", 'S');
	    update_statusline();
	    
	    /* Get a key */
	    ke = inkey_ex();
	    
	    /* Start over */
	    if (ke.key == 'S') 
	      state = BIRTH_RESTART;
	    
	    if (ke.key == ESCAPE) 
	      state--;
	    else
	      state++;
	    
	    /* Buttons */
	    kill_all_buttons();
	    update_statusline();
	    
	    /* Clear prompt */
	    clear_from(23);
	    
	    break;
	  }
	  
	case BIRTH_ACCEPTED:
	  {
	    return;
	  }
	  
	}
    }
}


/**
 * Create a new character.
 *
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
void player_birth(void)
{
  int i, j;
  char long_day[25];
  
#ifdef _WIN32_WCE
  unsigned long fake_time(unsigned long* fake_time_t);
  time_t ct = fake_time(0);
      
#else
  time_t ct = time((time_t*)0);
#endif
      
  /* Wipe the player properly */
/*  player_wipe(TRUE); */
  player_wipe(!character_quickstart);
  
  /* Create a new character */
  player_birth_aux();
  
  /* Get date */
#ifdef _WIN32_WCE
  {
    char* fake_ctime(const unsigned long* fake_time_t);
    sprintf(long_day, "%-.6s %-.2s",
            fake_ctime(&ct) + 4, fake_ctime(&ct) + 22);
  }
#else
  (void)strftime(long_day, 25, "%m/%d/%Y at %I:%M %p", localtime(&ct));
#endif
  
  /* Record the start for notes */
  sprintf(notes_start, "Began the quest to kill Morgoth on %s\n",long_day);
  


  /* Now that the player information is available, we are able to generate 
   * random artifacts.
   */
  initialize_random_artifacts();
  
  /* Note player birth in the message recall */
  message_add(" ", MSG_GENERIC);
  message_add("  ", MSG_GENERIC);
  message_add("====================", MSG_GENERIC);
  message_add("  ", MSG_GENERIC);
  message_add(" ", MSG_GENERIC);
  
  
  /* Hack - don't display above for easy_more */
  if (easy_more)
    {
      /* Arcane weirdness */
      msg_print(" ");
      message_flush();
    }
	
  /* Hack -- outfit the player */
  player_outfit();

  /* Set map, quests */
  if (adult_dungeon)
    {
      for (i = 0; i < NUM_STAGES; i++)
	for (j = 0; j < 9; j++)
	  stage_map[i][j] = dungeon_map[i][j];

      /* Mim */
      q_list[0].stage = 31;
      
      /* Glaurung */
      q_list[1].stage = 56;
      
      /* Ungoliant */
      q_list[2].stage = 71;
      
      /* Sauron */
      q_list[3].stage = 86;
      
      /* Morgoth */
      q_list[4].stage = 101;
    }
	
  
  /* Initialize shops */
  store_init();
  
}



