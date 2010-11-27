/** \file save.c 
    \brief Creation of savefiles  

 * Loading a player from a savefile.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick, Bahman Rabii, Ben Harrison
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
 * Some "local" parameters, used to help write savefiles
 */

static ang_file	*fff;		/* Current save "file" */

static byte	xor_byte;	/* Simple encryption */

static u32b	v_stamp = 0L;	/* A simple "checksum" on the actual values */
static u32b	x_stamp = 0L;	/* A simple "checksum" on the encoded bytes */



/**
 * These functions place information into a savefile a byte at a time
 */

static void sf_put(byte v)
{
  /* Encode the value, write a character */
  xor_byte ^= v;
  (void)file_writec(fff, xor_byte);
  
  /* Maintain the checksum info */
  v_stamp += v;
  x_stamp += xor_byte;
}

static void wr_byte(byte v)
{
  sf_put(v);
}

static void wr_u16b(u16b v)
{
  sf_put((byte)(v & 0xFF));
  sf_put((byte)((v >> 8) & 0xFF));
}

static void wr_s16b(s16b v)
{
  wr_u16b((u16b)v);
}

static void wr_u32b(u32b v)
{
  sf_put((byte)(v & 0xFF));
  sf_put((byte)((v >> 8) & 0xFF));
  sf_put((byte)((v >> 16) & 0xFF));
  sf_put((byte)((v >> 24) & 0xFF));
}

static void wr_s32b(s32b v)
{
  wr_u32b((u32b)v);
}

static void wr_string(cptr str)
{
  while (*str)
    {
      wr_byte(*str);
      str++;
    }
  wr_byte(*str);
}


/*
 * These functions write info in larger logical records
 */


/**
 * Write an "item" record
 */
static void wr_item(object_type *o_ptr)
{
  int i;

  wr_s16b(o_ptr->k_idx);
  
  /* Location */
  wr_byte(o_ptr->iy);
  wr_byte(o_ptr->ix);
  
  wr_byte(o_ptr->tval);
  wr_byte(o_ptr->sval);
  wr_s16b(o_ptr->pval);
  
  wr_byte(o_ptr->discount);
  wr_byte(o_ptr->number);
  wr_s16b(o_ptr->weight);
  
  wr_byte(o_ptr->name1);
  wr_byte(o_ptr->name2);
  wr_s16b(o_ptr->timeout);
  
  wr_s16b(o_ptr->to_h);
  wr_s16b(o_ptr->to_d);
  wr_s16b(o_ptr->to_a);
  wr_s16b(o_ptr->ac);
  wr_byte(o_ptr->dd);
  wr_byte(o_ptr->ds);
  
  wr_byte(o_ptr->ident);
  
  wr_byte(o_ptr->marked);
  
  wr_u32b(o_ptr->flags_obj);	/* New object flags -NRM- */
  wr_u32b(o_ptr->flags_curse);	/* New curse flags  -NRM- */
  wr_u32b(o_ptr->id_curse);	/* New curse id  -NRM- */
  wr_u32b(o_ptr->id_obj);	/* New object flags id  -NRM- */
  wr_u32b(o_ptr->id_other);	/* New object other id  -NRM- */

  /* Resists, bonuses, multiples -NRM- */
  for (i = 0; i < MAX_P_RES; i++)
    wr_byte(o_ptr->percent_res[i]);
  for (i = 0; i < A_MAX; i++)
    wr_byte(o_ptr->bonus_stat[i]);
  for (i = 0; i < MAX_P_BONUS; i++)
    wr_byte(o_ptr->bonus_other[i]);
  for (i = 0; i < MAX_P_SLAY; i++)
    wr_byte(o_ptr->multiple_slay[i]);
  for (i = 0; i < MAX_P_BRAND; i++)
    wr_byte(o_ptr->multiple_brand[i]);

  /* Where found */
  wr_u32b(o_ptr->found);
  
  /* Held by monster index */
  wr_s16b(o_ptr->held_m_idx);
  
  /* Activation */
  wr_byte(o_ptr->activation);
  
  /* Feeling */
  wr_byte(o_ptr->feel);
  
  /* Save the inscription (if any) */
  if (o_ptr->note)
    {
      wr_string(quark_str(o_ptr->note));
    }
  else
    {
      wr_string("");
    }
}
  

/**
 * Write a "monster" record
 */
static void wr_monster(monster_type *m_ptr)
{
  monster_race *r_ptr = &r_info[m_ptr->r_idx];

  /* Special treatment for player ghosts */
  if (r_ptr->flags2 & (RF2_PLAYER_GHOST))
    wr_s16b(r_ghost);
  else
    wr_s16b(m_ptr->r_idx);

  wr_byte(m_ptr->fy);
  wr_byte(m_ptr->fx);
  wr_s16b(m_ptr->hp);
  wr_s16b(m_ptr->maxhp);
  wr_s16b(m_ptr->csleep);
  wr_byte(m_ptr->mspeed);
  wr_byte(m_ptr->energy);
  wr_byte(m_ptr->stunned);
  wr_byte(m_ptr->confused);
  wr_byte(m_ptr->monfear);
  wr_byte(m_ptr->stasis);
  
  wr_byte(m_ptr->black_breath);
  
  wr_u32b(m_ptr->smart); /* Flags for 'smart-learn' */
  
  /* Dummy writes for features soon to be implemented */
  wr_byte(0);   

  /* Shapechange counter */
  wr_byte(m_ptr->schange);

  /* Original form of shapechanger */      
  wr_s16b(m_ptr->orig_idx);	 
  
  /* Extra desire to cast harassment spells */
  wr_byte(m_ptr->harass);
  
  /* Current Mana */   
  wr_byte(m_ptr->mana);    
  
  /* Racial type */
  wr_byte(m_ptr->p_race);
  wr_byte(m_ptr->old_p_race);

  /* AI info */
  wr_s16b(m_ptr->hostile);
  wr_u16b(m_ptr->group);
  wr_u16b(m_ptr->group_leader);

  /* Territorial info */
  wr_u16b(m_ptr->y_terr);
  wr_u16b(m_ptr->x_terr);
          
  /* Spare */
  wr_s16b(0); 
}


/**
 * Write a "lore" record
 */
static void wr_lore(int r_idx)
{
  monster_race *r_ptr = &r_info[r_idx];
  monster_lore *l_ptr = &l_list[r_idx];
  
  /* Count sights/deaths/kills */
  wr_s16b(l_ptr->sights);
  wr_s16b(l_ptr->deaths);
  wr_s16b(l_ptr->pkills);
  wr_s16b(l_ptr->tkills);
  
  /* Count wakes and ignores */
  wr_byte(l_ptr->wake);
  wr_byte(l_ptr->ignore);
  
  /* Extra stuff */
  wr_byte(l_ptr->xtra1);
  wr_byte(l_ptr->xtra2);
  
  /* Count drops */
  wr_byte(l_ptr->drop_gold);
  wr_byte(l_ptr->drop_item);
  
  /* Count spells */
  wr_byte(l_ptr->cast_inate);
  wr_byte(l_ptr->cast_spell);
  
  /* Count blows of each type */
  wr_byte(l_ptr->blows[0]);
  wr_byte(l_ptr->blows[1]);
  wr_byte(l_ptr->blows[2]);
  wr_byte(l_ptr->blows[3]);
  
  /* Memorize flags */
  wr_u32b(l_ptr->flags1);
  wr_u32b(l_ptr->flags2);
  wr_u32b(l_ptr->flags3);
  wr_u32b(l_ptr->flags4);
  wr_u32b(l_ptr->flags5);
  wr_u32b(l_ptr->flags6);
  
  
  /* Monster limit per level */
  wr_byte(r_ptr->max_num);
  wr_byte(0);
  wr_byte(0);
  wr_byte(0);
}


/**
 * Write an "xtra" record.  Records knowledge of object kinds.
 */
static void wr_xtra(int k_idx)
{
  byte tmp8u = 0;
  
  object_kind *k_ptr = &k_info[k_idx];
  
  if (k_ptr->aware) tmp8u |= 0x01;
  if (k_ptr->tried) tmp8u |= 0x02;
  if (k_ptr->known_effect) tmp8u |= 0x04;
  if (k_ptr->squelch) tmp8u |= 0x08;
  
  wr_byte(tmp8u);

  tmp8u = 0;
  if (k_ptr->everseen) tmp8u |= 0x01;
  wr_byte(tmp8u);
}


/**
 * Write a "store" record
 */
static void wr_store(store_type *st_ptr)
{
  int j;
  
  /* Save the "open" counter */
  wr_u32b(st_ptr->store_open);
  
  /* Save the "insults" */
  wr_s16b(st_ptr->insult_cur);
  
  /* Save the current owner*/
  wr_byte(st_ptr->owner);
  
  /* Save the stock size */
  wr_byte((byte)st_ptr->stock_num);
  
  /* Save the "haggle" info */
  wr_s16b(st_ptr->good_buy);
  wr_s16b(st_ptr->bad_buy);
  
  /* Save the stock */
  for (j = 0; j < st_ptr->stock_num; j++)
    {
      /* Save each item in stock */
      wr_item(&st_ptr->stock[j]);
    }

  /* Save the ordered stock */
  if (st_ptr->type == STORE_MERCH)
    
    /* Look at the ordered slots */
      for (j = 24; j < 32; j++)
	{
	  /* Save the table entry */
	  wr_s16b(st_ptr->table[j]);
	}
}


/**
 * Write RNG state
 */
static errr wr_randomizer(void)
{
  int i;
  
  /* Zero */
  wr_u16b(0);
  
  /* Place */
  wr_u16b(Rand_place);

  /* State */
  for (i = 0; i < RAND_DEG; i++)
    {
      wr_u32b(Rand_state[i]);
    }
  
  /* Success */
  return (0);
}


/**
 * Write the "options"
 */
static void wr_options(void)
{
  int i, k;
  
  u32b flag[8];
  u32b mask[8];
  
  
  /*** Oops ***/
  
  /* Once contained options.  Reduced from four to three and 1/4 
   * longints in Oangband. */
  for (i = 0; i < 3; i++) wr_u32b(0L);
  wr_byte(0);
  
  
  /*** Timed Autosave (inspired by Zangband) ***/
  wr_byte(autosave);
  wr_s16b(autosave_freq);
  
  
  /*** Special Options ***/
  
  /* Write "delay_factor" */
  wr_byte((byte)op_ptr->delay_factor);
  
  /* Write "hitpoint_warn" */
  wr_byte((byte)op_ptr->hitpoint_warn);
  
  /* Write "panel_change" */
  wr_byte((byte)op_ptr->panel_change);
  
  /* Unused */
  wr_byte(0);
  
  
  /*** Normal options ***/
  
  /* Reset */
  for (i = 0; i < 8; i++)
    {
      flag[i] = 0L;
      mask[i] = 0L;
    }
  
  /* Analyze the options */
  for (i = 0; i < OPT_MAX; i++)
    {
      int os = i / 32;
      int ob = i % 32;
      
      /* Process real entries */
      if (option_text[i])
	{
	  /* Set flag */
	  if (op_ptr->opt[i])
	    {
	      /* Set */
	      flag[os] |= (1L << ob);
	    }
	  
	  /* Set mask */
	  mask[os] |= (1L << ob);
	}
    }
  
  /* Dump the flags */
  for (i = 0; i < 8; i++) wr_u32b(flag[i]);
  
  /* Dump the masks */
  for (i = 0; i < 8; i++) wr_u32b(mask[i]);
  
  
  /*** Window options ***/
  
  /* Reset */
  for (i = 0; i < 8; i++)
    {
      /* Flags */
      flag[i] = op_ptr->window_flag[i];
      
      /* Mask */
      mask[i] = 0L;
      
      /* Build the mask */
      for (k = 0; k < 32; k++)
	{
	  /* Set mask */
	  if (window_flag_desc[k])
	    {
	      mask[i] |= (1L << k);
	    }
	}
    }
  
  /* Dump the flags */
  for (i = 0; i < 8; i++) wr_u32b(flag[i]);
  
  /* Dump the masks */
  for (i = 0; i < 8; i++) wr_u32b(mask[i]);
}


/**
 * Hack -- Write the "ghost" info
 */
static void wr_ghost(void)
{
  int i;
  
  /* Name */
  wr_string("Broken Ghost");
  
  /* Hack -- stupid data */
  for (i = 0; i < 60; i++) wr_byte(0);
}


/**
 * Write autoinscribe & squelch item-quality submenu to the savefile
 */
static void wr_squelch(void)
{
  int i;
  
  /* Write number of squelch bytes */
  wr_byte(TYPE_MAX);
  for (i = 0; i < TYPE_MAX; i++)
    wr_byte(squelch_level[i]);
  
  /* Write ego-item squelch bits */
  wr_u16b(z_info->e_max);
  for (i = 0; i < z_info->e_max; i++)
    {
      byte flags = 0;
      
      /* Figure out and write the everseen flag */
      if (e_info[i].squelch) flags |= 0x01;
      if (e_info[i].everseen) flags |= 0x02;
      wr_byte(flags);
    }
  
  /* Write the current number of auto-inscriptions */
  wr_u16b(inscriptions_count);
  
  /* Write the autoinscriptions array */
  for (i = 0; i < inscriptions_count; i++)
    {
      wr_s16b(inscriptions[i].kind_idx);
      wr_string(quark_str(inscriptions[i].inscription_idx));
    }
  
  return;
}


/**
 * Write some "extra" info
 */
static void wr_extra(void)
{
  int i;
  
  wr_string(op_ptr->full_name);
  
  wr_string(p_ptr->died_from);
  
  for (i = 0; i < 4; i++)
    {
      wr_string(p_ptr->history[i]);
    }
  
  /* Race/Class/Gender/Spells */
  wr_byte(p_ptr->prace);
  wr_byte(p_ptr->pclass);
  wr_byte(p_ptr->psex);
  wr_byte(0);	/* oops */
  
  wr_byte(p_ptr->hitdie);
  wr_byte(0);
  
  wr_s16b(p_ptr->age);
  wr_s16b(p_ptr->ht);
  wr_s16b(p_ptr->wt);
  
  
  /* Dump the stats (maximum and current) */
  for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_max[i]);
  for (i = 0; i < 6; ++i) wr_s16b(p_ptr->stat_cur[i]);
  
  /* Ignore the transient stats */
  for (i = 0; i < 12; ++i) wr_u16b(p_ptr->barehand_dam[i]);
  
  wr_u32b(p_ptr->au);
  
  wr_u32b(p_ptr->max_exp);
  wr_u32b(p_ptr->exp);
  wr_u16b(p_ptr->exp_frac);
  wr_s16b(p_ptr->lev);
  wr_s16b(p_ptr->home);
  
  wr_s16b(p_ptr->mhp);
  wr_s16b(p_ptr->chp);
  wr_u16b(p_ptr->chp_frac);
  
  wr_s16b(p_ptr->msp);
  wr_s16b(p_ptr->csp);
  wr_u16b(p_ptr->csp_frac);
  
  /* Max Player and Dungeon Levels */
  wr_s16b(p_ptr->max_lev);
  for (i = 0; i < 4; i++) wr_s16b(p_ptr->recall[i]);
  wr_s16b(p_ptr->recall_pt);
  
  /* More info */
  wr_s16b(p_ptr->speed_boost);	/* Specialty Fury */
  wr_s16b(p_ptr->heighten_power);	/* Specialty Heighten Magic */
  wr_s16b(0);	/* oops */
  wr_s16b(0);	/* oops */
  wr_s16b(p_ptr->sc);
  wr_s16b(0);	/* oops */
  
  wr_s16b(0);		/* old "rest" */
  wr_s16b(p_ptr->blind);
  wr_s16b(p_ptr->paralyzed);
  wr_s16b(p_ptr->confused);
  wr_s16b(p_ptr->food);
  wr_s16b(0);	/* old "food_digested" */
  wr_s16b(0);	/* old "protection" */
  wr_s16b(p_ptr->energy);
  wr_s16b(p_ptr->fast);
  wr_s16b(p_ptr->slow);
  wr_s16b(p_ptr->afraid);
  wr_s16b(p_ptr->cut);
  wr_s16b(p_ptr->stun);
  wr_s16b(p_ptr->poisoned);
  wr_s16b(p_ptr->image);
  wr_s16b(p_ptr->protevil);
  wr_s16b(p_ptr->magicdef);
  wr_s16b(p_ptr->hero);
  wr_s16b(p_ptr->shero);
  wr_s16b(p_ptr->shield);
  wr_s16b(p_ptr->blessed);
  wr_s16b(p_ptr->tim_invis);
  wr_s16b(p_ptr->tim_esp);
  wr_s16b(p_ptr->superstealth);
  wr_s16b(p_ptr->ele_attack);
  wr_s16b(p_ptr->word_recall);
  wr_s16b(p_ptr->see_infra);
  wr_s16b(p_ptr->tim_infra);
  wr_s16b(p_ptr->oppose_fire);
  wr_s16b(p_ptr->oppose_cold);
  wr_s16b(p_ptr->oppose_acid);
  wr_s16b(p_ptr->oppose_elec);
  wr_s16b(p_ptr->oppose_pois);
  
  wr_u32b(p_ptr->special_attack); /* Attack modifier flags. */
  wr_byte(0);	/* oops */
  wr_byte(0);	/* oops */
  wr_byte(p_ptr->black_breath);	/* Now used to store Black Breath. */
  wr_byte(p_ptr->searching);
  wr_byte(0);	/* formerly maximize */
  wr_byte(0);	/* formerly preserve */
  wr_byte(p_ptr->schange); /* Now used to store shapechange. */
  
  /* Store the bones file selector, if the player is not dead. -LM- */
  if (!(p_ptr->is_dead)) wr_byte(bones_selector);
  else wr_byte(0);
  
  /* Store the number of thefts on the level. -LM- */
  wr_byte(number_of_thefts_on_level);
  
  /* Store number of monster traps on this level. -LM- */
  wr_byte(num_trap_on_level);
  
  /* Store number of runes on this level, mana reserve for rune of mana. */
  for (i = 0; i < MAX_RUNE; i++)
    wr_byte(num_runes_on_level[i]);
  wr_byte((byte)mana_reserve);
  
  wr_byte(p_ptr->themed_level); /* Stores the current themed level. -LM- */
  
  /* Stores what themed levels have already appeared. -LM- */
  wr_u32b(p_ptr->themed_level_appeared);
  
  /* Squelch */
  wr_squelch();
  
  /* Specialty abilties */
  for (i = 0; i < 10; i++) wr_byte(p_ptr->specialty_order[i]);
  
  /* Ignore some flags */
  wr_s16b(0L);	/* oops */
  
  /* Write the "object seeds" */
  wr_u32b(seed_flavor);
  for (i = 0; i < 10; i++) wr_u32b(seed_town[i]);
  
  
  /* Special stuff */
  wr_u16b(p_ptr->quests);
  wr_u32b(p_ptr->score);
  wr_u16b(p_ptr->total_winner);
  wr_u16b(p_ptr->noscore);
  
  
  /* Write death */
  wr_byte(p_ptr->is_dead);
  
  /* Write feeling */
  wr_byte((byte)feeling);
  
  /* Turn of last "feeling" */
  wr_s32b(do_feeling);
  
  /* Current turn */
  wr_s32b(turn);
}

/**
 * Write the notes into the savefile. Every savefile has at least NOTES_MARK.
 */
static void wr_notes(void)
{
  int i = 0;

  wr_string(notes_start);

  /* Count the entries */
  while (notes[i].turn) i++;
  wr_s32b(i);

  /* Write the entries */
  i = 0;
  while (notes[i].turn)
    {
      wr_s32b(notes[i].turn);
      wr_s32b(notes[i].place);
      wr_s32b((s32b)notes[i].level);
      wr_byte(notes[i].type);
      wr_string(notes[i].note);
      i++;
    }
}



/**
 * The cave grid flags that get saved in the savefile
 */
#define IMPORTANT_FLAGS (CAVE_MARK | CAVE_GLOW | CAVE_ICKY | CAVE_ROOM)


/**
 * Write the current dungeon
 */
static void wr_dungeon(void)
{
  int i, y, x;
  
  byte tmp8u;
  
  byte count;
  byte prev_char;
  
  
  /*** Basic info ***/
  
  /* Dungeon specific info follows */
  wr_u16b(p_ptr->stage);
  wr_u16b(p_ptr->last_stage);
  wr_u16b(p_ptr->py);
  wr_u16b(p_ptr->px);
  wr_u16b(DUNGEON_HGT);
  wr_u16b(DUNGEON_WID);
  wr_u16b(0);
  wr_u16b(0);
  
  
  /*** Simple "Run-Length-Encoding" of cave ***/
  
  /* Note that this will induce two wasted bytes */
  count = 0;
  prev_char = 0;
  
  /* Dump the cave */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Extract the important cave_info flags */
	  tmp8u = (cave_info[y][x] & (IMPORTANT_FLAGS));
	  
	  /* If the run is broken, or too full, flush it */
	  if ((tmp8u != prev_char) || (count == MAX_UCHAR))
	    {
	      wr_byte((byte)count);
	      wr_byte((byte)prev_char);
	      prev_char = tmp8u;
	      count = 1;
	    }
	  
	  /* Continue the run */
	  else
	    {
	      count++;
	    }
	}
    }
  
  /* Flush the data (if any) */
  if (count)
    {
      wr_byte((byte)count);
      wr_byte((byte)prev_char);
    }
  
  /* Note that this will induce two wasted bytes */
  count = 0;
  prev_char = 0;
  
  /* Dump the cave */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /*  Keep all the information from info2 */
	  tmp8u = cave_info2[y][x];
	  
	  /* If the run is broken, or too full, flush it */
	  if ((tmp8u != prev_char) || (count == MAX_UCHAR))
	    {
	      wr_byte((byte)count);
	      wr_byte((byte)prev_char);
	      prev_char = tmp8u;
	      count = 1;
	    }
	  
	  /* Continue the run */
	  else
	    {
	      count++;
	    }
	}
    }
  
  /* Flush the data (if any) */
  if (count)
    {
      wr_byte((byte)count);
      wr_byte((byte)prev_char);
    }
  
  
  /*** Simple "Run-Length-Encoding" of cave ***/
  
  /* Note that this will induce two wasted bytes */
  count = 0;
  prev_char = 0;
  
  /* Dump the cave */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Extract a byte */
	  tmp8u = cave_feat[y][x];
	  
	  /* If the run is broken, or too full, flush it */
	  if ((tmp8u != prev_char) || (count == MAX_UCHAR))
	    {
	      wr_byte((byte)count);
	      wr_byte((byte)prev_char);
	      prev_char = tmp8u;
	      count = 1;
	    }
	  
	  /* Continue the run */
	  else
	    {
	      count++;
	    }
	}
    }
  
  /* Flush the data (if any) */
  if (count)
    {
      wr_byte((byte)count);
      wr_byte((byte)prev_char);
    }
  
  
  /*** Compact ***/
  
  /* Compact the objects */
  compact_objects(0);
  
  /* Compact the monsters */
  compact_monsters(0);
  
  
  /*** Dump objects ***/
  
  /* Total objects */
  wr_u16b(o_max);
  
  /* Dump the objects */
  for (i = 1; i < o_max; i++)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Dump it */
      wr_item(o_ptr);
    }
  
  
  /*** Dump the monsters ***/
  
  /* Total monsters */
  wr_u16b(m_max);
  
  /* Dump the monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      
      /* Dump it */
      wr_monster(m_ptr);
    }
}



/**
 * Actually write a save-file
 */
static bool wr_savefile_new(void)
{
  int i, j;
  
  u32b now;
  
  byte tmp8u;
  u16b tmp16u;
  
  
  /* Guess at the current time */
#ifdef _WIN32_WCE
  {
    unsigned long fake_time(unsigned long* fake_time_t);
    now = fake_time(NULL);
  }
#else
  now = time((time_t *)0);
#endif
  
  /* Note the operating system */
  sf_xtra = 0L;
  
  /* Note when the file was saved */
  sf_when = now;
  
  /* Note the number of saves */
  sf_saves++;
  
  
  /*** Actually write the file ***/
  
  /* Dump the file header */
  xor_byte = 0;
  wr_byte(VERSION_MAJOR); 
  xor_byte = 0;
  wr_byte(VERSION_MINOR); 
  xor_byte = 0;
  wr_byte(VERSION_PATCH); 
  xor_byte = 0;
  tmp8u = (byte)rand_int(256);
  wr_byte(tmp8u);
  
  
  /* Reset the checksum */
  v_stamp = 0L;
  x_stamp = 0L;
  
  
  /* Operating system */
  wr_u32b(sf_xtra);
  
  
  /* Time file last saved */
  wr_u32b(sf_when);
  
  /* Number of past lives */
  wr_u16b(sf_lives);
  
  /* Number of times saved */
  wr_u16b(sf_saves);
  
  /* Space */
  wr_u32b(0L);
  
  
  /* Write the RNG state */
  wr_randomizer();
  
  
  /* Write the boolean "options" */
  wr_options();
  
  
  /* Dump the number of "messages" */
  tmp16u = message_num();
  if (compress_savefile && (tmp16u > 40)) tmp16u = 40;
  wr_u16b(tmp16u);
  
  /* Dump the messages (oldest first!) */
  for (i = tmp16u - 1; i >= 0; i--)
    {
      wr_string(message_str((s16b)i));
      wr_u16b(message_type((s16b)i));
    }
  
  
  /* Dump the monster lore */
  tmp16u = z_info->r_max;
  wr_u16b(tmp16u);
  for (i = 0; i < tmp16u; i++) wr_lore(i);
  
  
  /* Dump the object memory */
  tmp16u = z_info->k_max;
  wr_u16b(tmp16u);
  for (i = 0; i < tmp16u; i++) wr_xtra(i);
  
  
  /* Hack -- Dump the quests */
  tmp16u = MAX_Q_IDX;
  wr_u16b(tmp16u);
  for (i = 0; i < tmp16u; i++)
    {
      wr_u16b((u16b)q_list[i].stage);
      wr_byte(0);
      wr_byte(0);
    }
  
  
  /* Record the total number of artifacts. */
  tmp16u = z_info->a_max;
  wr_u16b(tmp16u);
  
  /* Record the number of random artifacts. */
  tmp16u = z_info->a_max - ART_MIN_RANDOM;
  wr_u16b(tmp16u);
  
  
  /* As the least bad of various possible options considered, Oangband now 
   * saves all random artifact data in savefiles.  This requires (44 * 40) 
   * = 1760 extra bytes in the savefile, which does not seem unreasonable.
   */
  /* Write the artifact info. */
  for (i = 0; i < z_info->a_max; i++)
    {
      artifact_type *a_ptr = &a_info[i];
      
      /* Most regular artifact info is stored in a_info.raw. */
      if (i < ART_MIN_RANDOM)
	{
	  wr_s32b(a_ptr->creat_turn);
	  wr_byte(a_ptr->p_level);
	  /* Store set completion info */
	  wr_byte(a_ptr->set_bonus ? 1 : 0);
	}
      /* But random artifacts are specific to each player. */
      else
	{
	  wr_u16b(a_ptr->name);
	  wr_u16b(a_ptr->text);
	  
	  wr_byte(a_ptr->tval);
	  wr_byte(a_ptr->sval);
	  wr_u16b(a_ptr->pval);
	  
	  wr_u16b(a_ptr->to_h);
	  wr_u16b(a_ptr->to_d);
	  wr_u16b(a_ptr->to_a);
	  
	  wr_byte(a_ptr->dd);
	  wr_byte(a_ptr->ds);
	  
	  wr_u16b(a_ptr->ac);
	  wr_u16b(a_ptr->weight);
	  
	  wr_u32b(a_ptr->cost);
	  
	  wr_u32b(a_ptr->flags_obj);
	  wr_u32b(a_ptr->flags_curse);
	  wr_u32b(a_ptr->flags_kind);
	  
	  wr_byte(a_ptr->level);
	  wr_byte(a_ptr->rarity);
	  
	  wr_s32b(a_ptr->creat_turn);
	  wr_byte(a_ptr->p_level);
	  wr_byte(a_ptr->activation);

	  /* Resists, bonuses, multiples -NRM- */
	  for (j = 0; j < MAX_P_RES; j++)
	    wr_byte(a_ptr->percent_res[j]);
	  for (j = 0; j < A_MAX; j++)
	    wr_byte(a_ptr->bonus_stat[j]);
	  for (j = 0; j < MAX_P_BONUS; j++)
	    wr_byte(a_ptr->bonus_other[j]);
	  for (j = 0; j < MAX_P_SLAY; j++)
	    wr_byte(a_ptr->multiple_slay[j]);
	  for (j = 0; j < MAX_P_BRAND; j++)
	    wr_byte(a_ptr->multiple_brand[j]);
	  
	  /* Add some filler space for later expansion. */
	  wr_u32b(0);
	}
    }
  
  /* Note down how many random artifacts have names.  In Oangband 0.5.0 
   * there are 40 random artifact names.
   */
  wr_u16b(z_info->a_max - ART_MIN_RANDOM);
  
  /* Write the list of random artifact names. */
  for (i = ART_MIN_RANDOM; i < z_info->a_max; i++)
    {
      artifact_type *a_ptr = &a_info[i];
      wr_string(format("%s", a_name + a_ptr->name));
    }
  
  
  /* Write the "extra" information */
  wr_extra();
  
  
  /* Dump the "player hp" entries */
  tmp16u = PY_MAX_LEVEL;
  wr_u16b(tmp16u);
  for (i = 0; i < tmp16u; i++)
    {
      wr_s16b(p_ptr->player_hp[i]);
    }
  
  
  /* Write spell data */
  wr_u32b(p_ptr->spell_learned1);
  wr_u32b(p_ptr->spell_learned2);
  wr_u32b(p_ptr->spell_worked1);
  wr_u32b(p_ptr->spell_worked2);
  wr_u32b(p_ptr->spell_forgotten1);
  wr_u32b(p_ptr->spell_forgotten2);
  
  /* Dump the ordered spells */
  for (i = 0; i < 64; i++)
    {
      wr_byte(p_ptr->spell_order[i]);
    }
  
  /* Write sensation ID data */
  wr_u32b(p_ptr->id_obj);
  wr_u32b(p_ptr->id_other);
  
  /* Write the notes */
  wr_notes();
  
  /* Write the inventory */
  for (i = 0; i < INVEN_TOTAL; i++)
    {
      object_type *o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Dump index */
      wr_u16b(i);
      
      /* Dump object */
      wr_item(o_ptr);
    }
  
  /* Add a sentinel */
  wr_u16b(0xFFFF);
  
  
  /* Note the stores */
  tmp16u = MAX_STORES;
  wr_u16b(tmp16u);
  
  /* Dump the stores */
  for (i = 0; i < tmp16u; i++) wr_store(&store[i]);
  
  
  /* Player is not dead, write the dungeon */
  if (!p_ptr->is_dead)
    {
      /* Dump the dungeon */
      wr_dungeon();
      
      /* Dump the ghost */
      wr_ghost();
    }
  
  
  /* Write the "value check-sum" */
  wr_u32b(v_stamp);
  
  /* Write the "encoded checksum" */
  wr_u32b(x_stamp);
  
  /* Successful save */
  return TRUE;
}


/**
 * Medium level player saver
 */
static bool save_player_aux(char *name)
{
  bool ok = FALSE;
  
  /* No file yet */
  fff = NULL;
  
  /* Close the "fd" */
  safe_setuid_grab();
  
  /* Open the savefile */
  fff = file_open(name, MODE_WRITE, FTYPE_SAVE);
  safe_setuid_drop();
  
  /* Successful open */
  if (fff)
    {
      /* Write the savefile */
      if (wr_savefile_new()) ok = TRUE;
      
      /* Attempt to close it */
      if (!file_close(fff)) ok = FALSE;
    }
  
  /* Remove "broken" files */
  if (!ok) file_delete(name);
  
  /* Failure */
  if (!ok) return (FALSE);
  
  /* Successful save */
  character_saved = TRUE;
  
  /* Success */
  return (TRUE);
}



/**
 * Attempt to save the player in a savefile
 */
bool save_player(void)
{
  int result = FALSE;
  
  char safe[1024];
  
  
#ifdef SET_UID
# ifdef SECURE
  
  /* Get "games" permissions */
  beGames();
  
# endif
  
#endif
  
  
  /* New savefile */
  strcpy(safe, savefile);
  strcat(safe, ".new");
  
#ifdef VM
  /* Hack -- support "flat directory" usage on VM/ESA */
  strcpy(safe, savefile);
  strcat(safe, "n");
#endif /* VM */
  
  /* Remove it */
  file_delete(safe);
  
  /* Attempt to save the player */
  if (save_player_aux(safe))
    {
      char temp[1024];
      
      /* Old savefile */
      strcpy(temp, savefile);
      strcat(temp, ".old");
      
#ifdef VM
      /* Hack -- support "flat directory" usage on VM/ESA */
      strcpy(temp, savefile);
      strcat(temp, "o");
#endif /* VM */
      
      /* Remove it */
      file_delete(temp);
      
      /* Preserve old savefile */
      file_move(savefile, temp);
      
      /* Activate new savefile */
      file_move(safe, savefile);
      
      /* Remove preserved savefile */
      file_delete(temp);
      
      /* Hack -- Pretend the character was loaded */
      character_loaded = TRUE;
      
     
      /* Success */
      result = TRUE;
    }
  
  
#ifdef SET_UID
  
# ifdef SECURE
  
  /* Drop "games" permissions */
  bePlayer();
  
# endif
  
#endif
  

  /* Return the result */
  return (result);
}

/**
 * Hack
 *
 * Check worn items and apply sets as set data is not stored -GS-
 *
 * Should be unnecessary now (FA400), as set details are kept with the objects,
 * and whether the bonus is added is stored with the artifact. -NRM-
 */
void check_item_sets(void)
{
  int j;
  
  /* Check all equipped items. */
  for (j = INVEN_WIELD;j <= INVEN_FEET;j++)
    {
      object_type *o_ptr;
      o_ptr = &inventory[j];
      
      /* Is it an artifact? */
      if (o_ptr->name1)
	{
	  artifact_type *a_ptr = &a_info[o_ptr->name1];
	  
	  /* Is it a set item? */
	  if (a_ptr->set_no != 0)
	    {
	      
	      /* Check for complete set. */
	      if (check_set(a_ptr->set_no)) 
		{
		  
		  /* Apply set bonuses */
		  apply_set(a_ptr->set_no);
		}
	    }
	}
    }
}

/**
 * Attempt to Load a "savefile"
 *
 * Version 2.7.0 introduced a slightly different "savefile" format from
 * older versions, requiring a completely different parsing method.
 *
 * Note that savefiles from 2.7.0 - 2.7.2 are completely obsolete.
 *
 * Pre-2.8.0 savefiles lose some data, see "load2.c" for info.
 *
 * Pre-2.7.0 savefiles lose a lot of things, see "load1.c" for info.
 *
 * On multi-user systems, you may only "read" a savefile if you will be
 * allowed to "write" it later, this prevents painful situations in which
 * the player loads a savefile belonging to someone else, and then is not
 * allowed to save his game when he quits.
 *
 * We return "TRUE" if the savefile was usable, and we set the global
 * flag "character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 */
bool load_player(void)
{
  errr err = 0;
  
  
  cptr what = "generic";
  
  
  /* Paranoia */
  turn = 0;
  
  /* Paranoia */
  p_ptr->is_dead = FALSE;
  
  
  /* Allow empty savefile name */
  if (!savefile[0]) return (TRUE);
  
  
#if !defined(MACINTOSH) && !defined(WINDOWS) && !defined(VM)
  
  /* XXX XXX XXX Fix this */
  
  /* Verify the existance of the savefile */
  if (access(savefile, 0) < 0)
    {
      /* Give a message */
      msg_print("Savefile does not exist.");
      msg_print(NULL);
      
      /* Allow this */
      return (TRUE);
    }
  
#endif


  
  /* Collect the version information. */
  if (!err)
    {
      /* Read file. */
      err = rd_version_info();
      
      /* Describe errors */
      if (err == -1) what = "Cannot open savefile";
    }
  
  /* Process file */
  if (!err)
    {
      /* Clear screen */
      Term_clear();
      
      /* Parse "new" savefiles */
      if (sf_major <= 1)
	{
	  /* Attempt to load */
	  err = rd_savefile_new();
	}
      
      /* Parse "future" savefiles */
      else
	{
	  /* Error XXX XXX XXX */
	  err = -1;
	}
      
      /* Message (below) */
      if (err) what = "Cannot parse savefile";
    }
  
  /* Paranoia */
  if (!err)
    {
      /* Invalid turn */
      if (!turn) err = -1;
      
      /* Message (below) */
      if (err) what = "Broken savefile";
    }
  
  
  /* Okay */
  if (!err)
    {
      /* Give a conversion warning */
      if ((version_major != sf_major) ||
	  (version_minor != sf_minor) ||
	  (version_patch != sf_patch))
	{
	  /* Message */
	  msg_format("Converted an FAangband %d.%d.%d savefile.",
		     sf_major, sf_minor, sf_patch);
	  msg_print(NULL);
	}
      
      /* Player is dead */
      if (p_ptr->is_dead)
	{
	  /* Forget death */
	  p_ptr->is_dead = FALSE;
	  
	  /* Cheat death */
	  if (arg_wizard)
	    {
	      /* A character was loaded */
	      character_loaded = TRUE;
	      
	      /* Done */
	      return (TRUE);
	    }
	  
	  /* Count lives */
	  sf_lives++;
	  
	  /* Forget turns */
	  turn = do_feeling = 0;
	  
	  /* Done */
	  return (TRUE);
	}
      
      /* A character was loaded */
      character_loaded = TRUE;
      
      /* Still alive */
      if (p_ptr->chp >= 0)
	{
	  /* Reset cause of death */
	  strcpy(p_ptr->died_from, "(alive and well)");
	}
      
      /* Success */
      return (TRUE);
    }
  
  
  
  /* Message */
  msg_format("Error (%s) reading an FAangband %d.%d.%d savefile.",
	     what, sf_major, sf_minor, sf_patch);
  
  msg_print(NULL);
  
  /* Oops */
  return (FALSE);
}


