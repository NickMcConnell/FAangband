/*
 * File: birth.c
 * Purpose: Character creation
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
#include "files.h"
#include "game-event.h"
#include "game-cmd.h"
#include "tvalsval.h"
#include "squelch.h"
#include "ui-menu.h"

/*
 * Overview
 * ========
 * This file contains the game-mechanical part of the birth process.
 * To follow the code, start at player_birth towards the bottom of
 * the file - that is the only external entry point to the functions
 * defined here.
 *
 * Player (in the Angband sense of character) birth is modelled as a
 * a series of commands from the UI to the game to manipulate the
 * character and corresponding events to inform the UI of the outcomes
 * of these changes.
 *
 * The current aim of this section is that after any birth command
 * is carried out, the character should be left in a playable state.
 * In particular, this means that if a savefile is supplied, the
 * character will be set up according to the "quickstart" rules until
 * another race or class is chosen, or until the stats are reset by
 * the UI.
 *
 * Once the UI signals that the player is happy with the character, the
 * game does housekeeping to ensure the character is ready to start the
 * game (clearing the history log, making sure options are set, etc)
 * before returning control to the game proper.
 */


/*
 * Forward declare
 */
typedef struct birther /*lovely*/ birther; /*sometimes we think she's a dream*/

/*
 * A structure to hold "rolled" information, and any
 * other useful state for the birth process.
 *
 * XXX Demand Obama's birth certificate
 */
struct birther
{
	byte sex;
	byte race;
	byte class;

	s16b age;
	s16b wt;
	s16b ht;
	s16b sc;

	s32b au;

	s16b stat[A_MAX];

	char history[4][60];
};



/*
 * Save the currently rolled data into the supplied 'player'.
 */
static void save_roller_data(birther *player)
{
	int i;

	/* Save the data */
	player->sex = p_ptr->psex;
	player->race = p_ptr->prace;
	player->class = p_ptr->pclass;
	player->age = p_ptr->age;
	player->wt = p_ptr->wt_birth;
	player->ht = p_ptr->ht_birth;
	player->sc = p_ptr->sc_birth;
	player->au = p_ptr->au_birth;

	/* Save the stats */
	for (i = 0; i < A_MAX; i++)
	{
		player->stat[i] = p_ptr->stat_birth[i];
	}

	/* Save the history */
	for (i = 0; i < 4; i++)
	  my_strcpy(player->history[i], p_ptr->history[i], sizeof(player->history[i]));
}


/*
 * Load stored player data from 'player' as the currently rolled data,
 * optionally placing the current data in 'prev_player' (if 'prev_player'
 * is non-NULL).
 *
 * It is perfectly legal to specify the same "birther" for both 'player'
 * and 'prev_player'.
 */
static void load_roller_data(birther *player, birther *prev_player)
{
	int i;

     /* The initialisation is just paranoia - structure assignment is
        (perhaps) not strictly defined to work with uninitialised parts
        of structures. */
	birther temp;
	WIPE(&temp, birther);

	/*** Save the current data if we'll need it later ***/
	if (prev_player) save_roller_data(&temp);

	/*** Load the previous data ***/

	/* Load the data */
	p_ptr->psex = player->sex;
	p_ptr->prace = player->race;
	p_ptr->pclass = player->class;
	p_ptr->age = player->age;
	p_ptr->wt = p_ptr->wt_birth = player->wt;
	p_ptr->ht = p_ptr->ht_birth = player->ht;
	p_ptr->sc = p_ptr->sc_birth = player->sc;
	p_ptr->au_birth = player->au;
	p_ptr->au = STARTING_GOLD;

	/* Load the stats */
	for (i = 0; i < A_MAX; i++)
	{
		p_ptr->stat_max[i] = p_ptr->stat_cur[i] = p_ptr->stat_birth[i] = player->stat[i];
	}

	/* Load the history */
	for (i = 0; i < 4; i++)
	  my_strcpy(p_ptr->history[i], player->history[i], sizeof(p_ptr->history[i]));


	/*** Save the current data if the caller is interested in it. ***/
	if (prev_player) *prev_player = temp;
}


/*
 * Adjust a stat by an amount.
 *
 * This just uses "modify_stat_value()" unless "maximize" mode is false,
 * and a positive bonus is being applied, in which case, a special hack
 * is used.
 */
static int adjust_stat(int value, int amount)
{
  return (modify_stat_value(value, amount));
}




/*
 * Roll for a characters stats
 *
 * For efficiency, we include a chunk of "calc_bonuses()".
 */
static void get_stats(int stat_use[A_MAX])
{
	int i, j;

	int bonus;

	int dice[18];


	/* Roll and verify some stats */
	while (TRUE)
	{
		/* Roll some dice */
		for (j = i = 0; i < 18; i++)
		{
			/* Roll the dice */
			dice[i] = randint1(3 + i % 3);

			/* Collect the maximum */
			j += dice[i];
		}

		/* Verify totals */
		if ((j > 42) && (j < 54)) break;
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

		/* Apply the bonus to the stat (somewhat randomly) */
		stat_use[i] = adjust_stat(p_ptr->stat_max[i], bonus);

		/* Save the resulting stat maximum */
		p_ptr->stat_cur[i] = p_ptr->stat_max[i] = stat_use[i];
		
		p_ptr->stat_birth[i] = p_ptr->stat_max[i];
	}
}


static void roll_hp(void)
{
	int i, j, min_value, max_value;

	/* Minimum hitpoints at highest level */
	min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 3) / 8;
	min_value += PY_MAX_LEVEL;

	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 5) / 8;
	max_value += PY_MAX_LEVEL;

	/* Roll out the hitpoints */
	while (TRUE)
	{
		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			j = randint1(p_ptr->hitdie);
			p_ptr->player_hp[i] = p_ptr->player_hp[i-1] + j;
		}

		/* XXX Could also require acceptable "mid-level" hitpoints */

		/* Require "valid" hitpoints at highest level */
		if (p_ptr->player_hp[PY_MAX_LEVEL-1] < min_value) continue;
		if (p_ptr->player_hp[PY_MAX_LEVEL-1] > max_value) continue;

		/* Acceptable */
		break;
	}
}


static void get_bonuses(void)
{
	/* Calculate the bonuses and hitpoints */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Update stuff */
	update_stuff();

	/* Fully healed */
	p_ptr->chp = p_ptr->mhp;

	/* Fully rested */
	p_ptr->csp = p_ptr->msp;
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
  social_class = randint1(4);
  
  /* Starting place */
  chart = rp_ptr->hist;
  
  
  /* Process the history */
  while (chart)
    {
      /* Start over */
      i = 0;
      
      /* Roll for nobility */
      roll = randint1(100);
      
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

/*
 * Computes character's age, height, and weight
 */
static void get_ahw(void)
{
        /* Calculate the age */
	p_ptr->age = rp_ptr->b_age + randint1(rp_ptr->m_age);

	/* Calculate the height/weight for males */
	if (p_ptr->psex == SEX_MALE)
	{
		p_ptr->ht = p_ptr->ht_birth = Rand_normal(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
		p_ptr->wt = p_ptr->wt_birth = Rand_normal(rp_ptr->m_b_wt, rp_ptr->m_m_wt);
	}

	/* Calculate the height/weight for females */
	else if (p_ptr->psex == SEX_FEMALE)
	{
		p_ptr->ht = p_ptr->ht_birth = Rand_normal(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
		p_ptr->wt = p_ptr->wt_birth = Rand_normal(rp_ptr->f_b_wt, rp_ptr->f_m_wt);
	}
}



void player_init(struct player_type *p) {
	int i;

	if (inventory)
		mem_free(inventory);

	/* Wipe the player */
	(void)WIPE(p, struct player_type);

	/* Start with no artifacts made yet */
	for (i = 0; z_info && i < z_info->a_max; i++)
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
	p->food = PY_FOOD_FULL - 1;


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

	inventory = C_ZNEW(INVEN_TOTAL, struct object_type);

	/* First turn. */
	turn = old_turn = 1;
	p_ptr->energy = 0;
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
  if (OPT(adult_thrall))
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
          if ((!OPT(adult_thrall)) && (!OPT(adult_dungeon))) 
	    object_upgrade(i_ptr);
          
          object_aware(i_ptr);
          object_known(i_ptr);
	  apply_autoinscription(i_ptr);
	  (void)inven_carry(i_ptr);
	  k_info[k_idx].everseen = TRUE;
        }
    }

  /* Dungeon gear for escaping thralls */
  if (OPT(adult_thrall))
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

/*
 * Cost of each "point" of a stat.
 */
static const int birth_stat_costs[18 + 1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 4 };

/* It was feasible to get base 17 in 3 stats with the autoroller */
#define MAX_BIRTH_POINTS 24 /* 3 * (1+1+1+1+1+1+2) */

static void recalculate_stats(int *stats, int points_left)
{
	int i;

	/* Process stats */
	for (i = 0; i < A_MAX; i++)
	{
		/* Obtain a "bonus" for "race" and "class" */
	        int bonus = rp_ptr->r_adj[i] + cp_ptr->c_adj[i];

		/* Apply the racial/class bonuses */
		p_ptr->stat_cur[i] = p_ptr->stat_max[i] = 
		  p_ptr->stat_birth[i] =
		  modify_stat_value(stats[i], bonus);
		
	}

	/* Gold is inversely proportional to cost */
	p_ptr->au_birth = STARTING_GOLD + (50 * points_left);

	/* Update bonuses, hp, etc. */
	get_bonuses();

	/* Tell the UI about all this stuff that's changed. */
	event_signal(EVENT_GOLD);
	event_signal(EVENT_AC);
	event_signal(EVENT_HP);
	event_signal(EVENT_STATS);
}

static void reset_stats(int stats[A_MAX], int points_spent[A_MAX], int *points_left)
{
	int i;

	/* Calculate and signal initial stats and points totals. */
	*points_left = MAX_BIRTH_POINTS;

	for (i = 0; i < A_MAX; i++)
	{
		/* Initial stats are all 10 and costs are zero */
		stats[i] = 10;
		points_spent[i] = 0;
	}

	/* Use the new "birth stat" values to work out the "other"
	   stat values (i.e. after modifiers) and tell the UI things have 
	   changed. */
	recalculate_stats(stats, *points_left);
	event_signal_birthpoints(points_spent, *points_left);	
}

static bool buy_stat(int choice, int stats[A_MAX], int points_spent[A_MAX], int *points_left)
{
	/* Must be a valid stat, and have a "base" of below 18 to be adjusted */
	if (!(choice >= A_MAX || choice < 0) &&	(stats[choice] < 18))
	{
		/* Get the cost of buying the extra point (beyond what
		   it has already cost to get this far). */
		int stat_cost = birth_stat_costs[stats[choice] + 1];

		if (stat_cost <= *points_left)
		{
			stats[choice]++;
			points_spent[choice] += stat_cost;
			*points_left -= stat_cost;

			/* Tell the UI the new points situation. */
			event_signal_birthpoints(points_spent, *points_left);

			/* Recalculate everything that's changed because
			   the stat has changed, and inform the UI. */
			recalculate_stats(stats, *points_left);

			return TRUE;
		}
	}

	/* Didn't adjust stat. */
	return FALSE;
}


static bool sell_stat(int choice, int stats[A_MAX], int points_spent[A_MAX],
					  int *points_left)
{
	/* Must be a valid stat, and we can't "sell" stats below the base of 10. */
	if (!(choice >= A_MAX || choice < 0) && (stats[choice] > 10))
	{
		int stat_cost = birth_stat_costs[stats[choice]];

		stats[choice]--;
		points_spent[choice] -= stat_cost;
		*points_left += stat_cost;

		/* Tell the UI the new points situation. */
		event_signal_birthpoints(points_spent, *points_left);

		/* Recalculate everything that's changed because
		   the stat has changed, and inform the UI. */
		recalculate_stats(stats, *points_left);

		return TRUE;
	}

	/* Didn't adjust stat. */
	return FALSE;
}


/*
 * This picks some reasonable starting values for stats based on the
 * current race/class combo, etc.  For now I'm disregarding concerns
 * about role-playing, etc, and using the simple outline from
 * http://angband.oook.cz/forum/showpost.php?p=17588&postcount=6:
 *
 * 0. buy base STR 17
 * 1. if possible buy adj DEX of 18/10
 * 2. spend up to half remaining points on each of spell-stat and con, 
 *    but only up to max base of 16 unless a pure class 
 *    [mage or priest or warrior]
 * 3. If there are any points left, spend as much as possible in order 
 *    on DEX, non-spell-stat, CHR. 
 */
static void generate_stats(int stats[A_MAX], int points_spent[A_MAX], 
						   int *points_left)
{
	int step = 0;
	int maxed[A_MAX] = { 0 };
	bool pure = FALSE;

	/* Determine whether the class is "pure" */
	if (check_ability(SP_XTRA_SPECIALTY) || check_ability(SP_STRONG_MAGIC))
	{
		pure = TRUE;
	}

	while (*points_left && step >= 0)
	{
		switch (step)
		{
			/* Buy base STR 17 */
			case 0:				
			{
				if (!maxed[A_STR] && stats[A_STR] < 17)
				{
					if (!buy_stat(A_STR, stats, points_spent, points_left))
						maxed[A_STR] = TRUE;
				}
				else
				{
					step++;
				}

				break;
			}

			/* Try and buy adj DEX of 18/10 */
			case 1:
			{
				if (!maxed[A_DEX] && p_ptr->stat_top[A_DEX] < 18+10)
				{
					if (!buy_stat(A_DEX, stats, points_spent, points_left))
						maxed[A_DEX] = TRUE;
				}
				else
				{
					step++;
				}

				break;
			}

			/* If we can't get 18/10 dex, sell it back. */
			case 2:
			{
				if (p_ptr->stat_top[A_DEX] < 18+10)
				{
					while (stats[A_DEX] > 10)
						sell_stat(A_DEX, stats, points_spent, points_left);

					maxed[A_DEX] = FALSE;
				}
				
				step++;
			}

			/* 
			 * Spend up to half remaining points on each of spell-stat and 
			 * con, but only up to max base of 16 unless a pure class 
			 * [mage or priest or warrior]
			 */
			case 3:
			{
				int points_trigger = *points_left / 2;

				if (mp_ptr->spell_stat)
				{
					while (!maxed[mp_ptr->spell_stat] &&
						   (pure || stats[mp_ptr->spell_stat] < 16) &&
						   points_spent[mp_ptr->spell_stat] < points_trigger)
					{						
						if (!buy_stat(mp_ptr->spell_stat, stats, points_spent,
									  points_left))
						{
							maxed[mp_ptr->spell_stat] = TRUE;
						}

						if (points_spent[mp_ptr->spell_stat] > points_trigger)
						{
							sell_stat(mp_ptr->spell_stat, stats, points_spent, 
									  points_left);
							maxed[mp_ptr->spell_stat] = TRUE;
						}
					}
				}

				while (!maxed[A_CON] &&
					   (pure || stats[A_CON] < 16) &&
					   points_spent[A_CON] < points_trigger)
				{						
					if (!buy_stat(A_CON, stats, points_spent,points_left))
					{
						maxed[A_CON] = TRUE;
					}
					
					if (points_spent[A_CON] > points_trigger)
					{
						sell_stat(A_CON, stats, points_spent, points_left);
						maxed[A_CON] = TRUE;
					}
				}
				
				step++;
				break;
			}

			/* 
			 * If there are any points left, spend as much as possible in 
			 * order on DEX, non-spell-stat, CHR. 
			 */
			case 4:
			{				
				int next_stat;

				if (!maxed[A_DEX])
				{
					next_stat = A_DEX;
				}
				else if (!maxed[A_INT] && mp_ptr->spell_stat != A_INT)
				{
					next_stat = A_INT;
				}
				else if (!maxed[A_WIS] && mp_ptr->spell_stat != A_WIS)
				{
					next_stat = A_WIS;
				}
				else if (!maxed[A_CHR])
				{
					next_stat = A_CHR;
				}
				else
				{
					step++;
					break;
				}

				/* Buy until we can't buy any more. */
				while (buy_stat(next_stat, stats, points_spent, points_left));
				maxed[next_stat] = TRUE;

				break;
			}

			default:
			{
				step = -1;
				break;
			}
		}
	}
}

/*
 * This fleshes out a full player based on the choices currently made,
 * and so is called whenever things like race or class are chosen.
 */
void player_generate(struct player_type *p, player_sex *s,
                     struct player_race *r, player_class *c)
{
	if (!s) s = &sex_info[p->psex];
	if (!c) c = &c_info[p->pclass];
	if (!r) r = &p_info[p->prace];

	sp_ptr = s;
	cp_ptr = c;
	mp_ptr = &magic_info[p->pclass];
	rp_ptr = r;

	/* Level 1 */
	p->max_lev = p->lev = 1;

	/* Hitdice */
	p->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp;

	/* Initial hitpoints */
	p->mhp = p->hitdie;

	/* Pre-calculate level 1 hitdice */
	p->player_hp[0] = p->hitdie;

	/* Roll for age/height/weight */
	get_ahw();

	get_history();
}


/* Reset everything back to how it would be on loading the game. */
static void do_birth_reset(bool use_quickstart, birther *quickstart_prev)
{

	/* If there's quickstart data, we use it to set default
	   character choices. */
	if (use_quickstart && quickstart_prev)
		load_roller_data(quickstart_prev, NULL);

	player_generate(p_ptr, NULL, NULL, NULL);

	/* Update stats with bonuses, etc. */
	get_bonuses();
}


/*
 * Create a new character.
 *
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
void player_birth(bool quickstart_allowed)
{
        int i, j;
	game_command blank = { CMD_NULL, 0, {{0}} };
	game_command *cmd = &blank;

	int stats[A_MAX];
	int points_spent[A_MAX];
	int points_left;
	char *buf;
	int success;

	bool rolled_stats = FALSE;

	/*
	 * The last character displayed, to allow the user to flick between two.
	 * We rely on prev.age being zero to determine whether there is a stored
	 * character or not, so initialise it here.
	 */
	birther prev = { 0, 0, 0, 0, 0, 0, 0, 0, {0}, "" };

	/*
	 * If quickstart is allowed, we store the old character in this,
	 * to allow for it to be reloaded if we step back that far in the
	 * birth process.
	 */
	birther quickstart_prev = {0, 0, 0, 0, 0, 0, 0, 0, {0}, "" };

	char long_day[25];
  
#ifdef _WIN32_WCE
	unsigned long fake_time(unsigned long* fake_time_t);
	time_t ct = fake_time(0);
      
#else
	time_t ct = time((time_t*)0);
#endif
	/*
	 * If there's a quickstart character, store it for later use.
	 * If not, default to whatever the first of the choices is.
	 */
	if (quickstart_allowed)
		save_roller_data(&quickstart_prev);
	else
	{
		p_ptr->psex = 0;
		p_ptr->pclass = 0;
		p_ptr->prace = 0;
		player_generate(p_ptr, NULL, NULL, NULL);
	}

	/* Handle incrementing name suffix */
	buf = find_roman_suffix_start(op_ptr->full_name);

	if (buf)
	{
		/* Try to increment the roman suffix */
		success = int_to_roman((roman_to_int(buf) + 1), buf,
			(sizeof(op_ptr->full_name) - (buf -
			(char *)&op_ptr->full_name)));
			
		if (!success) msg_print("Sorry, could not deal with suffix");
	}
	

	/* We're ready to start the interactive birth process. */
	event_signal_flag(EVENT_ENTER_BIRTH, quickstart_allowed);

	/* 
	 * Loop around until the UI tells us we have an acceptable character.
	 * Note that it is possible to quit from inside this loop.
	 */
	while (cmd->command != CMD_ACCEPT_CHARACTER)
	{
		/* Grab a command from the queue - we're happy to wait for it. */
		if (cmd_get(CMD_BIRTH, &cmd, TRUE) != 0) continue;

		if (cmd->command == CMD_BIRTH_RESET)
		{
			player_init(p_ptr);
			reset_stats(stats, points_spent, &points_left);
			do_birth_reset(quickstart_allowed, &quickstart_prev);
			rolled_stats = FALSE;
		}
		else if (cmd->command == CMD_CHOOSE_SEX)
		{
			p_ptr->psex = cmd->arg[0].choice; 
			player_generate(p_ptr, NULL, NULL, NULL);
		}
		else if (cmd->command == CMD_CHOOSE_RACE)
		{
			p_ptr->prace = cmd->arg[0].choice;
			player_generate(p_ptr, NULL, NULL, NULL);

			reset_stats(stats, points_spent, &points_left);
			generate_stats(stats, points_spent, &points_left);
			rolled_stats = FALSE;
		}
		else if (cmd->command == CMD_CHOOSE_CLASS)
		{
			p_ptr->pclass = cmd->arg[0].choice;
			player_generate(p_ptr, NULL, NULL, NULL);

			reset_stats(stats, points_spent, &points_left);
			generate_stats(stats, points_spent, &points_left);
			rolled_stats = FALSE;
		}
		else if (cmd->command == CMD_FINALIZE_OPTIONS)
		{
			/* Set adult options from birth options */
			for (i = OPT_BIRTH; i < OPT_CHEAT; i++)
			{
				op_ptr->opt[OPT_ADULT + (i - OPT_BIRTH)] =
					op_ptr->opt[i];
			}

			/* Reset score options from cheat options */
			for (i = OPT_CHEAT; i < OPT_ADULT; i++)
			{
				op_ptr->opt[OPT_SCORE + (i - OPT_CHEAT)] =
					op_ptr->opt[i];
			}
		}
		else if (cmd->command == CMD_BUY_STAT)
		{
			/* .choice is the stat to buy */
			if (!rolled_stats)
				buy_stat(cmd->arg[0].choice, stats, points_spent, &points_left);
		}
		else if (cmd->command == CMD_SELL_STAT)
		{
			/* .choice is the stat to sell */
			if (!rolled_stats)
				sell_stat(cmd->arg[0].choice, stats, points_spent, &points_left);
		}
		else if (cmd->command == CMD_RESET_STATS)
		{
			/* .choice is whether to regen stats */
			reset_stats(stats, points_spent, &points_left);

			if (cmd->arg[0].choice)
				generate_stats(stats, points_spent, &points_left);

			rolled_stats = FALSE;
		}
		else if (cmd->command == CMD_ROLL_STATS)
		{
			int i;

			save_roller_data(&prev);

			/* Get a new character */
			get_stats(stats);

			/* Update stats with bonuses, etc. */
			get_bonuses();

			/* There's no real need to do this here, but it's tradition. */
			get_ahw();
			get_history();

			event_signal(EVENT_GOLD);
			event_signal(EVENT_AC);
			event_signal(EVENT_HP);
			event_signal(EVENT_STATS);

			/* Give the UI some dummy info about the points situation. */
			points_left = 0;
			for (i = 0; i < A_MAX; i++)
			{
				points_spent[i] = 0;
			}

			event_signal_birthpoints(points_spent, points_left);

			/* Lock out buying and selling of stats based on rolled stats. */
			rolled_stats = TRUE;
		}
		else if (cmd->command == CMD_PREV_STATS)
		{
			/* Only switch to the stored "previous"
			   character if we've actually got one to load. */
			if (prev.age)
			{
				load_roller_data(&prev, &prev);
				get_bonuses();
			}

			event_signal(EVENT_GOLD);
			event_signal(EVENT_AC);
			event_signal(EVENT_HP);
			event_signal(EVENT_STATS);
		}
		else if (cmd->command == CMD_NAME_CHOICE)
		{
			/* Set player name */
			my_strcpy(op_ptr->full_name, cmd->arg[0].string,
					  sizeof(op_ptr->full_name));

			string_free((void *) cmd->arg[0].string);

			/* Don't change savefile name.  If the UI
			   wants it changed, they can do it. XXX (Good idea?) */
			process_player_name(FALSE);
		}
		/* Various not-specific-to-birth commands. */
		else if (cmd->command == CMD_HELP)
		{
			char buf[80];

			strnfmt(buf, sizeof(buf), "birth.txt");
			screen_save();
			show_file(buf, NULL, 0, 0);
			screen_load();
		}
		else if (cmd->command == CMD_QUIT)
		{
			quit(NULL);
		}
	}

	roll_hp();

	squelch_birth_init();

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
	if (OPT(easy_more))
	  {
	    /* Arcane weirdness */
	    msg_print(" ");
	    message_flush();
	  }
	
	/* Hack -- outfit the player */
	player_outfit();
	
	/* Set map, quests */
	if (OPT(adult_dungeon))
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
  
	/* Now we're really done.. */
	event_signal(EVENT_LEAVE_BIRTH);
}
