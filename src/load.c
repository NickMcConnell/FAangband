/** \file load.c
    \brief Up-to-date savefile handling.
 
 
 * We attempt to prevent corrupt savefiles from inducing memory errors.
 *
 * Note that this file should not use the random number generator, the
 * object flavors, the visual attr/char mappings, or anything else which
 * is initialized *after* or *during* the "load character" function.
 *
 * This file assumes that the monster/object records are initialized
 * to zero, and the race/kind tables have been loaded correctly.  The
 * order of object stacks is currently not saved in the savefiles, but
 * the "next" pointers are saved, so all necessary knowledge is present.
 *
 * We should implement simple "savefile extenders" using some form of
 * "sized" chunks of bytes, with a {size,type,data} format, so everyone
 * can know the size, interested people can know the type, and the actual
 * data is available to the parsing routines that acknowledge the type.
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
#include "cave.h"
#include "history.h"
#include "monster.h"
#include "tvalsval.h"
#include "savefile.h"
#include "squelch.h"


/* Object, trap constants */
byte of_size = 0, cf_size = 0, if_size = 0, trf_size = 0;
byte max_p_res = 0, a_max = 0, max_p_bonus = 0, max_p_slay = 0, max_p_brand = 0;



/**
 * Read an object
 *
 * This function attempts to "repair" old savefiles, and to extract
 * the most up to date values for various object fields.
 *
 * Change extra might to non-pval form if needed -LM-
 *
 * Deal with themed level, glyphs/traps count, player ghosts -LM-
 */
static int rd_item(object_type *o_ptr)
{
    int i;

    char buf[128];

    byte tmp;
    u32b tmp32u;
    u16b time_base, time_dice, time_sides;
  
  
    /* Kind */
    rd_s16b(&o_ptr->k_idx);
  
    /* Paranoia */
    if ((o_ptr->k_idx < 0) || (o_ptr->k_idx >= z_info->k_max))
	return (-1);
    o_ptr->kind = &k_info[o_ptr->k_idx];

    /* Location */
    rd_byte(&o_ptr->iy);
    rd_byte(&o_ptr->ix);
  
    /* Type/Subtype */
    rd_byte(&o_ptr->tval);
    rd_byte(&o_ptr->sval);
  
    /* Special pval */
    rd_s16b(&o_ptr->pval);
  
    rd_byte(&o_ptr->discount);
    rd_byte(&o_ptr->number);
    rd_s16b(&o_ptr->weight);
  
    rd_byte(&o_ptr->name1);
    rd_byte(&o_ptr->name2);
    rd_s16b(&o_ptr->timeout);
  
    rd_s16b(&o_ptr->to_h);
    rd_s16b(&o_ptr->to_d);
    rd_s16b(&o_ptr->to_a);
  
    rd_s16b(&o_ptr->ac);

    /* SJGU keep save dice */
    rd_byte(&o_ptr->dd);
    rd_byte(&o_ptr->ds);


    rd_byte(&o_ptr->ident);
  
    rd_byte(&o_ptr->marked);
  
    rd_byte(&o_ptr->origin);
    rd_byte(&o_ptr->origin_stage);
    rd_u16b(&o_ptr->origin_xtra);

    /* Flags */
    for (i = 0; i < of_size; i++)
        rd_byte(&o_ptr->flags_obj[i]);
    for (i = 0; i < cf_size; i++)
        rd_byte(&o_ptr->flags_curse[i]);
    for (i = 0; i < cf_size; i++)
        rd_byte(&o_ptr->id_curse[i]);
    for (i = 0; i < of_size; i++)
        rd_byte(&o_ptr->id_obj[i]);
    for (i = 0; i < if_size; i++)
        rd_byte(&o_ptr->id_other[i]);

    /* New percentage resists -NRM- */
    for (i = 0; i < max_p_res; i++)
    {
	rd_byte(&tmp);
	o_ptr->percent_res[i] = tmp;
    }
      
    /* Bonuses and multiples - nasty byte hack */
    for (i = 0; i < a_max; i++)
    {
	rd_byte(&tmp);
	if (tmp < 129) o_ptr->bonus_stat[i] = (int)tmp;
	else o_ptr->bonus_stat[i] = (int)(tmp - 256);
    }
    for (i = 0; i < max_p_bonus; i++)
    {
	rd_byte(&tmp);
	if (tmp < 129) o_ptr->bonus_other[i] = (int)tmp;
	else o_ptr->bonus_other[i] = (int)(tmp - 256);
    }
    for (i = 0; i < max_p_slay; i++)
    {
	rd_byte(&tmp);
	if (tmp < 129) o_ptr->multiple_slay[i] = (int)tmp;
	else o_ptr->multiple_slay[i] = (int)(tmp - 256);
    }
    for (i = 0; i < max_p_brand; i++)
    {
	rd_byte(&tmp);
	if (tmp < 129) o_ptr->multiple_brand[i] = (int)tmp;
	else o_ptr->multiple_brand[i] = (int)(tmp - 256);
    }
  
    /* Monster holding object */
    rd_s16b(&o_ptr->held_m_idx);
	
    /* Activation */
    rd_u16b(&o_ptr->effect);
    rd_u16b(&time_base);
    rd_u16b(&time_dice);
    rd_u16b(&time_sides);
    o_ptr->time.base = time_base;
    o_ptr->time.dice = time_dice;
    o_ptr->time.sides = time_sides;
  
  
    rd_byte(&o_ptr->feel);
  
    /* Inscription */
    rd_string(buf, 128);
  
    /* Save the inscription */
    if (buf[0]) o_ptr->note = quark_add(buf);
  
    /* Expansion */
    rd_u32b(&tmp32u);

    /* Success */
    return (0);
}



/**
 * Read a monster
 */
static int rd_monster(monster_type *m_ptr)
{
    byte tmp8u;
    s16b tmp16u;
    u32b tmp32u;

    /* Read the monster race */
    rd_s16b(&m_ptr->r_idx);
  
    /* Read the other information */
    rd_byte(&m_ptr->fy);
    rd_byte(&m_ptr->fx);
    rd_s16b(&m_ptr->hp);
    rd_s16b(&m_ptr->maxhp);
    rd_s16b(&m_ptr->csleep);
    rd_byte(&m_ptr->mspeed);
    rd_byte(&m_ptr->energy);
    rd_byte(&m_ptr->stunned);
    rd_byte(&m_ptr->confused);
    rd_byte(&m_ptr->monfear);
  
    /* Oangband 0.3.3 added monster stasis. */
    rd_byte(&m_ptr->stasis);
  
  
    /* Oangband 0.5.0 saves 'smart learn' flags and Black Breath state */
    rd_byte(&tmp8u);
    m_ptr->black_breath = tmp8u;
    rd_u32b(&m_ptr->smart);
  
    /* Oangband 0.5.0 saves some more data not yet in use */
    rd_byte(&tmp8u);

    /* Shapechange counter */
    rd_byte(&m_ptr->schange);

    /* Original form of a shapechanger */
    rd_s16b(&m_ptr->orig_idx);
  
    /* Monster extra desire to cast harassment spells */
    rd_byte(&m_ptr->harass);
  
    /*  Monster mana  */
    rd_byte(&m_ptr->mana);
  
    /* Racial type */
    rd_byte(&m_ptr->p_race);
    rd_byte(&m_ptr->old_p_race);
      
    /* AI info */
    rd_s16b(&m_ptr->hostile);
    rd_u16b(&m_ptr->group);
    rd_u16b(&m_ptr->group_leader);
      
    /* Territorial info */
    rd_u16b(&m_ptr->y_terr);
    rd_u16b(&m_ptr->x_terr);


    /* Spare */
    rd_s16b(&tmp16u);
    
    /* Expansion */
    rd_u32b(&tmp32u);

    /* Success */
    return (0);
}


/**
 * Read a trap record
 */
static void rd_trap(trap_type *t_ptr)
{
    int i;

    rd_byte(&t_ptr->t_idx);
    t_ptr->kind = &trap_info[t_ptr->t_idx];
    rd_byte(&t_ptr->fy);
    rd_byte(&t_ptr->fx);
    rd_byte(&t_ptr->xtra);

    for (i = 0; i < trf_size; i++)
	rd_byte(&t_ptr->flags[i]);
}

/**
 * Read RNG state (added in 2.8.0)
 */
int rd_randomizer(u32b version)
{
    int i;
    u32b noop;

    /* current value for the simple RNG */
    rd_u32b(&Rand_value);

    /* state index */
    rd_u32b(&state_i);

    /* for safety, make sure state_i < RAND_DEG */
    state_i = state_i % RAND_DEG;
    
    /* RNG variables */
    rd_u32b(&z0);
    rd_u32b(&z1);
    rd_u32b(&z2);
    
    /* RNG state */
    for (i = 0; i < RAND_DEG; i++)
	rd_u32b(&STATE[i]);

    /* NULL padding */
    for (i = 0; i < 59 - RAND_DEG; i++)
	rd_u32b(&noop);

    Rand_quick = FALSE;

    /* Success */
    return (0);
}



/**
 * Read options (ignore most pre-2.8.0 options)
 *
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
int rd_options(u32b version)
{
    int i, n;
  
    byte b;
  
    u16b tmp16u;

    u32b flag[8];
    u32b mask[8];
  
  
  
    /*** Timed Autosave, inspired by Zangband. ***/
    rd_byte(&b);
    autosave = b;
    rd_s16b(&autosave_freq);
  
  
    /*** Special info */
  
    /* Read "delay_factor" */
    rd_byte(&b);
    op_ptr->delay_factor = b;
  
    /* Read "hitpoint_warn" */
    rd_byte(&b);
    op_ptr->hitpoint_warn = b;
  
    /* Read "panel_change" */
    rd_byte(&b); 
    op_ptr->panel_change = b;

    /* Read lazymove delay */  
    rd_u16b(&tmp16u);
    lazymove_delay = (tmp16u < 1000) ? tmp16u : 0;
  
    /*** Normal Options ***/
  
    while (1) {
	byte value;
	char name[20];
	rd_string(name, sizeof name);

	if (!name[0])
	    break;

	rd_byte(&value);
	option_set(name, !!value);
    }
  
    /*** Window Options ***/
  
    /* Read the window flags */
    for (n = 0; n < ANGBAND_TERM_MAX; n++) rd_u32b(&flag[n]);
  
    /* Read the window masks */
    for (n = 0; n < ANGBAND_TERM_MAX; n++) rd_u32b(&mask[n]);
  
    /* Analyze the options */
    for (n = 0; n < ANGBAND_TERM_MAX; n++)
    {
	/* Analyze the options */
	for (i = 0; i < 32; i++)
	{
	    /* Process valid flags */
	    if (window_flag_desc[i])
	    {
				/* Blank invalid flags */
				if (!(mask[n] & (1L << i)))
				{
					flag[n] &= ~(1L << i);
				}
#if 0
		/* Process valid flags */
		if (mask[n] & (1L << i))
		{
		    /* Set */
		    if (flag[n] & (1L << i))
		    {
			/* Set */
			op_ptr->window_flag[n] |= (1L << i);
		    }
		}
#endif
	    }
	}
    }
  

    /* Set up the subwindows */
    subwindows_set_flags(flag, ANGBAND_TERM_MAX);

    /* Success */
    return (0);
}



/**
 * Read the saved messages
 */
int rd_messages(u32b version)
{
  int i;
  char buf[128];
  u16b tmp16u;
  
  s16b num;
  
  /* Total */
  rd_s16b(&num);
  
  /* Read the messages */
  for (i = 0; i < num; i++)
    {
      /* Read the message */
      rd_string(buf, 128);
      
      /* Read the message type */
      rd_u16b(&tmp16u);
      
      /* Save the message */
      message_add(buf, tmp16u);
      
    }

    /* Success */
    return (0);
}





/**
 * Read the monster lore
 */
int rd_monster_memory(u32b version)
{
    byte tmp8u, rf_size, rsf_size, monster_blow_max;
    int r_idx;
    u16b tmp16u;
  
    /* Monster Memory */
    rd_u16b(&tmp16u);
	
    /* Incompatible save files */
    if (tmp16u > z_info->r_max)
    {
	note(format("Too many (%u) monster races!", tmp16u));
	return (-1);
    }
	
    /* Monster flags */
    rd_byte(&rf_size);
	
    /* Incompatible save files */
    if (rf_size > RF_SIZE)
    {
	note(format("Too many (%u) monster flags!", rf_size));
	return (-1);
    }
	
    /* Monster spell flags */
    rd_byte(&rsf_size);
	
    /* Incompatible save files */
    if (rsf_size > RSF_SIZE)
    {
	note(format("Too many (%u) monster spell flags!", rsf_size));
	return (-1);
    }
	
    /* Monster blows */
    rd_byte(&monster_blow_max);
	
    /* Incompatible save files */
    if (monster_blow_max > MONSTER_BLOW_MAX)
    {
	note(format("Too many (%u) monster blows!", monster_blow_max));
	return (-1);
    }
	
    /* Read the available records */
    for (r_idx = 0; r_idx < tmp16u; r_idx++)
    {
	size_t i;

	monster_race *r_ptr = &r_info[r_idx];
	monster_lore *l_ptr = &l_list[r_idx];
 
	/* Count sights/deaths/kills */
	rd_s16b(&l_ptr->sights);
	rd_s16b(&l_ptr->deaths);
	rd_s16b(&l_ptr->pkills);
	rd_s16b(&l_ptr->tkills);
  
	/* Count wakes and ignores */
	rd_byte(&l_ptr->wake);
	rd_byte(&l_ptr->ignore);
  
	/* Extra stuff */
	rd_byte(&l_ptr->xtra1);
	rd_byte(&l_ptr->xtra2);
  
	/* Count drops */
	rd_byte(&l_ptr->drop_gold);
	rd_byte(&l_ptr->drop_item);
  
	/* Count spells */
	rd_byte(&l_ptr->cast_spell);
  
	/* Count blows of each type */
	for (i = 0; i < monster_blow_max; i++)
	    rd_byte(&l_ptr->blows[i]);
  
	/* Memorize flags */
	for (i = 0; i < rf_size; i++)
	    rd_byte(&l_ptr->flags[i]);
	for (i = 0; i < rsf_size; i++)
	    rd_byte(&l_ptr->spell_flags[i]);
  
	/* Read the "Racial" monster limit per level */
	rd_byte(&r_ptr->max_num);
  
	/* Later (?) */
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);
	
	/* Repair the lore flags */
	rf_inter(l_ptr->flags, r_ptr->flags);
	
	/* Repair the spell lore flags */
	rsf_inter(l_ptr->spell_flags, r_ptr->spell_flags);
    }
	
    /* Success */
    return (0);
}

int rd_object_memory(u32b version)
{
    int i;
    u16b tmp16u;
	
    /* Object Memory */
    rd_u16b(&tmp16u);
	
    /* Incompatible save files */
    if (tmp16u > z_info->k_max)
    {
	note(format("Too many (%u) object kinds!", tmp16u));
	return (-1);
    }
	
    /* Object flags */
    rd_byte(&of_size);
	
    /* Incompatible save files */
    if (of_size > OF_SIZE)
    {
	note(format("Too many (%u) object flags!", of_size));
	return (-1);
    }
	
    /* Curse flags */
    rd_byte(&cf_size);
	
    /* Incompatible save files */
    if (cf_size > CF_SIZE)
    {
	note(format("Too many (%u) curse flags!", cf_size));
	return (-1);
    }
	
    /* ID flags */
    rd_byte(&if_size);
	
    /* Incompatible save files */
    if (if_size > IF_SIZE)
    {
	note(format("Too many (%u) id flags!", if_size));
	return (-1);
    }
	
    /* Resists */
    rd_byte(&max_p_res);
	
    /* Incompatible save files */
    if (max_p_res > MAX_P_RES)
    {
	note(format("Too many (%u) resists!", max_p_res));
	return (-1);
    }
	
    /* Stats */
    rd_byte(&a_max);
	
    /* Incompatible save files */
    if (a_max > A_MAX)
    {
	note(format("Too many (%u) stats!", a_max));
	return (-1);
    }
	
    /* Bonuses */
    rd_byte(&max_p_bonus);
	
    /* Incompatible save files */
    if (max_p_bonus > MAX_P_BONUS)
    {
	note(format("Too many (%u) bonuses!", max_p_bonus));
	return (-1);
    }
	
    /* Slays */
    rd_byte(&max_p_slay);
	
    /* Incompatible save files */
    if (max_p_slay > MAX_P_SLAY)
    {
	note(format("Too many (%u) slays!", max_p_slay));
	return (-1);
    }
	
    /* Brands */
    rd_byte(&max_p_brand);
	
    /* Incompatible save files */
    if (max_p_brand > MAX_P_BRAND)
    {
	note(format("Too many (%u) brands!", max_p_brand));
	return (-1);
    }
	
    /* Read the object memory */
    for (i = 0; i < tmp16u; i++)
    {
	byte tmp8u;
	u32b tmp32u;
	object_kind *k_ptr = &k_info[i];
      
	rd_byte(&tmp8u);
      
	k_ptr->aware = (tmp8u & 0x01) ? TRUE: FALSE;
	k_ptr->tried = (tmp8u & 0x02) ? TRUE: FALSE;
	k_ptr->known_effect = (tmp8u & 0x04) ? TRUE: FALSE;
	k_ptr->squelch = (tmp8u & 0x08) ? TRUE: FALSE;

        rd_byte(&tmp8u);
        
        k_ptr->everseen = (tmp8u & 0x01) ? TRUE: FALSE;
    
	/* Expansion */
	rd_u32b(&tmp32u);
    }
	
    return 0;
}

int rd_quests(u32b version)
{
    int i;
    byte tmp8u;
    u16b tmp16u, num_quests;
	
    /* Load the Quests */
    rd_u16b(&num_quests);
  
    /* Incompatible save files */
    if (num_quests > MAX_Q_IDX)
    {
	note(format("Too many (%u) quests!", num_quests));
	return (-1);
    }
  
    /* Load the Quests */
    for (i = 0; i < num_quests; i++)
    {
	rd_u16b(&tmp16u);
	q_list[i].stage = tmp16u;
	rd_byte(&tmp8u);
	rd_byte(&tmp8u);
    }

    return 0;
}

int rd_artifacts(u32b version)
{
    int i, k;
    byte tmp8u;
    u16b tmp16u, base, dice, sides;
    u32b tmp32u;
    int total_artifacts = 0;
    int random_artifacts = 0;
    int art_min_random = 0;
    char buf[128];
  
    /* Load the Artifacts */
    rd_u16b(&tmp16u);
    total_artifacts = tmp16u;
  
    rd_u16b(&tmp16u);
    art_min_random = tmp16u;
    random_artifacts = total_artifacts - art_min_random;
  
    /* Incompatible save files */
    if (total_artifacts > z_info->a_max)
    {
	note(format("Too many (%u) artifacts!", total_artifacts));
	return (-1);
    }
    else if (random_artifacts > z_info->a_max - ART_MIN_RANDOM)
    {
	note(format("Too many (%u) random artifacts!", random_artifacts));
	return (-1);
    }
  
  /* Read the artifact info. */
  for (i = 0; i < art_min_random; i++)
    {
	rd_byte(&tmp8u);
	a_info[i].created = tmp8u;
	rd_byte(&tmp8u);
	a_info[i].seen = tmp8u;
	rd_byte(&tmp8u);
	a_info[i].set_bonus = (tmp8u ? TRUE : FALSE);
    
	/* Expansion */
	rd_u32b(&tmp32u);
    }

      
  /* Random artifacts are specific to each player. */
  for (i = art_min_random; i < total_artifacts; i++)
  {
      rd_string(buf, sizeof(buf));
      if (buf[0]) //my_strcpy(a_info[i].name, buf, sizeof(buf));
	  a_info[i].name = string_make(buf);
      rd_byte(&tmp8u);
      a_info[i].tval = tmp8u;
      rd_byte(&tmp8u);
      a_info[i].sval = tmp8u;
      rd_u16b(&tmp16u);
      a_info[i].pval = tmp16u;
	  
      rd_u16b(&tmp16u);
      a_info[i].to_h = tmp16u;
      rd_u16b(&tmp16u);
      a_info[i].to_d = tmp16u;
      rd_u16b(&tmp16u);
      a_info[i].to_a = tmp16u;
	  
      rd_byte(&tmp8u);
      a_info[i].dd = tmp8u;
      rd_byte(&tmp8u);
      a_info[i].ds = tmp8u;
	  
      rd_u16b(&tmp16u);
      a_info[i].ac = tmp16u;
      rd_u16b(&tmp16u);
      a_info[i].weight = tmp16u;
	  
      rd_u32b(&tmp32u);
      a_info[i].cost = tmp32u;
	  
      for (k = 0; k < of_size; k++) {
	  rd_byte(&tmp8u);
	  a_info[i].flags_obj[k] = tmp8u;
      }
      for (k = 0; k < cf_size; k++) {
	  rd_byte(&tmp8u);
	  a_info[i].flags_curse[k] = tmp8u;
      }

      rd_byte(&tmp8u);
      a_info[i].level = tmp8u;
      rd_byte(&tmp8u);
      a_info[i].rarity = tmp8u;

      rd_byte(&tmp8u);
      a_info[i].created = tmp8u;
      rd_byte(&tmp8u);
      a_info[i].seen = tmp8u;

      rd_byte(&tmp8u);
      a_info[i].effect = tmp8u;
      rd_u16b(&base);
      rd_u16b(&dice);
      rd_u16b(&sides);
      a_info[i].time.base = base;
      a_info[i].time.dice = dice;
      a_info[i].time.sides = sides;

      rd_u16b(&base);
      rd_u16b(&dice);
      rd_u16b(&sides);
      a_info[i].charge.base = base;
      a_info[i].charge.dice = dice;
      a_info[i].charge.sides = sides;

      for (k = 0; k < max_p_res; k++)
      {
	  rd_byte(&tmp8u);
	  a_info[i].percent_res[k] = tmp8u;
      }
      for (k = 0; k < a_max; k++)
      {
	  rd_byte(&tmp8u);
	  if (tmp8u < 129) a_info[i].bonus_stat[k] = (int)tmp8u;
	  else a_info[i].bonus_stat[k] = (int)(tmp8u - 256);
      }
      for (k = 0; k < max_p_bonus; k++)
      {
	  rd_byte(&tmp8u);
	  if (tmp8u < 129) a_info[i].bonus_other[k] = (int)tmp8u;
	  else a_info[i].bonus_other[k] = (int)(tmp8u - 256);
      }
      for (k = 0; k < max_p_slay; k++)
      {
	  rd_byte(&tmp8u);
	  a_info[i].multiple_slay[k] = tmp8u;
      }
      for (k = 0; k < max_p_brand; k++)
      {
	  rd_byte(&tmp8u);
	  a_info[i].multiple_brand[k] = tmp8u;
      }
	      
      /* Extra space. */
      rd_u32b(&tmp32u);
  }

  return 0;
}  
  
  
/*
 * Read the "extra" information
 */
int rd_player(u32b version)
{
    int i;

    byte max_recall_pts, tmd_max, rune_tail, max_specialties;
  
    byte tmp8u;
    u32b tmp32u;
  
    rd_string(op_ptr->full_name, sizeof(op_ptr->full_name));
  
    rd_string(p_ptr->died_from, 80);
  
    rd_string(p_ptr->history, 250);
  
    /* Player race */
    rd_byte(&p_ptr->prace);

    /* Verify player race */
    if (p_ptr->prace >= z_info->p_max)
    {
	note(format("Invalid player race (%d).", p_ptr->prace));
	return (-1);
    }
    rp_ptr = &p_info[p_ptr->prace];
    p_ptr->race = rp_ptr;

    /* Player class */
    rd_byte(&p_ptr->pclass);

    /* Verify player class */
    if (p_ptr->pclass >= z_info->c_max)
    {
	note(format("Invalid player class (%d).", p_ptr->pclass));
	return (-1);
    }
    cp_ptr = &c_info[p_ptr->pclass];
    p_ptr->class = cp_ptr;
    mp_ptr = &cp_ptr->magic;

    /* Player gender */
    rd_byte(&p_ptr->psex);
    sp_ptr = &sex_info[p_ptr->psex];
    p_ptr->sex = sp_ptr;

    /* Numeric name suffix */
    rd_byte(&op_ptr->name_suffix);
  
    /* Special Race/Class info */
    rd_byte(&p_ptr->hitdie);
    strip_bytes(1);
  
    /* Age/Height/Weight */
    rd_s16b(&p_ptr->age);
    rd_s16b(&p_ptr->ht);
    rd_s16b(&p_ptr->wt);
  
    /* Read the stat info */
    for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->stat_max[i]);
    for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->stat_cur[i]);
    for (i = 0; i < a_max; i++) rd_s16b(&p_ptr->stat_birth[i]);

    rd_s16b(&p_ptr->ht_birth);
    rd_s16b(&p_ptr->wt_birth);
    rd_s16b(&p_ptr->sc_birth);
    rd_s32b(&p_ptr->au_birth);

    /* Barehand damage */  
    for (i = 0; i < 12; i++) rd_u16b(&p_ptr->barehand_dam[i]);
  
    rd_s32b(&p_ptr->au);
  
    rd_s32b(&p_ptr->max_exp);
    rd_s32b(&p_ptr->exp);
    rd_u16b(&p_ptr->exp_frac);
    rd_s16b(&p_ptr->lev);
    rd_s16b(&p_ptr->home);
  
    rd_s16b(&p_ptr->mhp);
    rd_s16b(&p_ptr->chp);
    rd_u16b(&p_ptr->chp_frac);
  
    rd_s16b(&p_ptr->msp);
    rd_s16b(&p_ptr->csp);
    rd_u16b(&p_ptr->csp_frac);
  
    rd_s16b(&p_ptr->max_lev);
    rd_byte(&max_recall_pts);
    for (i = 0; i < max_recall_pts; i++)	
	rd_s16b(&p_ptr->recall[i]);
    rd_s16b(&p_ptr->recall_pt);

    /* Hack -- Repair maximum player level */
    if (p_ptr->max_lev < p_ptr->lev) p_ptr->max_lev = p_ptr->lev;
  
    /* More info */
    rd_s16b(&p_ptr->speed_boost);
    rd_s16b(&p_ptr->heighten_power);
  
    rd_s16b(&p_ptr->sc);
  
    /* Read the flags */
    rd_s16b(&p_ptr->food);
    rd_s16b(&p_ptr->energy);
    rd_s16b(&p_ptr->word_recall);
    rd_s16b(&p_ptr->state.see_infra);
  
    /* Find the number of timed effects */
    rd_byte(&tmd_max);	

    if (tmd_max <= TMD_MAX)
    {
	/* Read all the effects */
	for (i = 0; i < tmd_max; i++)
	    rd_s16b(&p_ptr->timed[i]);

	/* Initialize any entries not read */
	if (tmd_max < TMD_MAX)
	    C_WIPE(p_ptr->timed + tmd_max, TMD_MAX - tmd_max, s16b);
    }
    else
    {
	/* Probably in trouble anyway */
	for (i = 0; i < TMD_MAX; i++)
	    rd_s16b(&p_ptr->timed[i]);

	/* Discard unused entries */
	strip_bytes(2 * (tmd_max - TMD_MAX));
	note("Discarded unsupported timed effects");
    }

  
    /* Bit flags for various attack modifiers. */
    rd_u32b(&p_ptr->special_attack);
  
    rd_byte(&tmp8u);	/* oops */
    rd_byte(&tmp8u);	/* oops */
  
    rd_byte((byte *) &p_ptr->black_breath);	/* Status of Black Breath. */
  
    rd_byte(&p_ptr->searching);
  
    /* Was maximize, preserve modes */
    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
  
    /* Current shapechange. */
    rd_byte(&p_ptr->schange);
  
    /* The number of the bone file (if any) that player ghosts should use to 
     * reacquire a name, sex, class, and race.
     */
    rd_byte(&bones_selector);
  
    /* Find out how many thefts have already occured on this level. */
    rd_byte(&number_of_thefts_on_level);
  
    /* Read number of monster traps on level. */
    rd_byte(&num_trap_on_level);
  
    rd_byte(&rune_tail);
    if (rune_tail <= RUNE_TAIL)
    {
	for (i = 0; i < rune_tail; i++)
	{
	    rd_byte(&tmp8u);
	    num_runes_on_level[i] = tmp8u;
	}

	/* Initialize any entries not read */
	if (rune_tail < RUNE_TAIL)
	    for (i = rune_tail; i < RUNE_TAIL; i++)
	    {
		num_runes_on_level[i] = 0;
	    }
    }
    else
    {
	/* Probably in trouble anyway */
	for (i = 0; i < RUNE_TAIL; i++)
	{
	    rd_byte(&tmp8u);
	    num_runes_on_level[i] = tmp8u;
	}

	/* Discard unused entries */
	strip_bytes(2 * (rune_tail - RUNE_TAIL));
	note("Discarded unsupported runes");
    }

    rd_byte(&tmp8u);
    mana_reserve = tmp8u;
    
  
    /* Is the level themed and, if so, which theme is it? */
    rd_byte(&p_ptr->themed_level);
  
    /* What themed levels have already appeared? */
    rd_u32b(&p_ptr->themed_level_appeared);

    rd_byte(&max_specialties);
    /* Specialty Abilities -BR- */
    for (i = 0; i < max_specialties; i++) 
    {
	rd_byte(&p_ptr->specialty_order[i]);
	if (p_ptr->specialty_order[i] != PF_NO_SPECIALTY)
	    pf_on(p_ptr->pflags, p_ptr->specialty_order[i]);
    }
  
    /* Skip */
    strip_bytes(2);
    
    /* Expansion */
    rd_u32b(&tmp32u);
    rd_u32b(&tmp32u);

    return 0;
}

/**
 * Read squelch and autoinscription submenu for all known objects
 */
int rd_squelch(u32b version)
{
    int i;
    byte tmp8u;
    u16b file_e_max;
    u32b tmp32u;
  
    /* Read how many squelch bytes we have */
    rd_byte(&tmp8u);

    /* Check against current number */
    if (tmp8u != TYPE_MAX)
    {
	strip_bytes(tmp8u);
    }
    else
    {
	for (i = 0; i < TYPE_MAX; i++)
	    rd_byte(&squelch_level[i]);
    }
  
  
    /* Read the number of saved ego-item */
    rd_u16b(&file_e_max);
  
    for (i = 0; i < file_e_max; i++)
    {
	if (i < z_info->e_max)
	{
	    byte flags;
	  
	    /* Read and extract the flag */
	    rd_byte(&flags);
	    e_info[i].squelch = (flags & 0x01) ? TRUE : FALSE;
	    e_info[i].everseen = (flags & 0x02) ? TRUE : FALSE;
    
	    /* Expansion */
	    rd_u32b(&tmp32u);
	}
    }
  
    /* Read the current number of auto-inscriptions */
    rd_u16b(&inscriptions_count);
  
    /* Write the autoinscriptions array*/
    for (i = 0; i < inscriptions_count; i++)
    {
	char tmp[80];
      
	rd_s16b(&inscriptions[i].kind_idx);
	rd_string(tmp, sizeof(tmp));
      
	inscriptions[i].inscription_idx = quark_add(tmp);
    }
  
    return 0;
}


int rd_misc(u32b version)
{
	byte tmp8u;
	u32b tmp32u;
	int i;
	
	/* Hack -- the "special seeds" */
	rd_u32b(&seed_flavor);
	rd_byte(&tmp8u);
	for (i = 0; i < tmp8u; i++){
	    rd_u32b(&tmp32u);
	    if (i < NUM_TOWNS) 
		seed_town[i] = tmp32u;
	}


	/* Special stuff */
	rd_u16b(&p_ptr->quests);
	rd_u32b(&p_ptr->score);
	rd_u16b(&p_ptr->total_winner);
	rd_u16b(&p_ptr->noscore);


	/* Read "death" */
	rd_byte(&tmp8u);
	p_ptr->is_dead = tmp8u;

	/* Read "feeling" */
	rd_byte(&tmp8u);
	feeling = tmp8u;

	/* Turn of last "feeling" */
	rd_byte(&tmp8u);
	do_feeling = tmp8u ? TRUE : FALSE;

	/* Current turn */
	rd_s32b(&turn);

	return 0;
}

int rd_player_hp(u32b version)
{
	int i;
	u16b tmp16u;

	/* Read the player_hp array */
	rd_u16b(&tmp16u);

	/* Incompatible save files */
	if (tmp16u > PY_MAX_LEVEL)
	{
		note(format("Too many (%u) hitpoint entries!", tmp16u));
		return (-1);
	}

	/* Read the player_hp array */
	for (i = 0; i < tmp16u; i++)
		rd_s16b(&p_ptr->player_hp[i]);

	return 0;
}


int rd_player_spells(u32b version)
{
    int i;
    u16b tmp16u;
    u32b tmp32u;
	
    int cnt;
	
    /* Read the number of spells */
    rd_u16b(&tmp16u);
    if (tmp16u > PY_MAX_SPELLS)
    {
	note(format("Too many player spells (%d).", tmp16u));
	return (-1);
    }
	
    /* Read the spell flags */
    for (i = 0; i < tmp16u; i++)
	rd_byte(&p_ptr->spell_flags[i]);
	
    /* Read the spell order */
    for (i = 0, cnt = 0; i < tmp16u; i++, cnt++)
	rd_byte(&p_ptr->spell_order[cnt]);
	
    /* Expansion */
    rd_u32b(&tmp32u);
	
    /* Success */
    return (0);
}

int rd_randarts(u32b version)
{
    return 0;
}

int rd_inventory(u32b version)
{
    int slot = 0;
    byte all_inven_total;
    object_type *i_ptr;
    object_type object_type_body;
  
    rd_byte(&all_inven_total);
    if (all_inven_total > ALL_INVEN_TOTAL)
    {
	note(format("Too many inventory objects (%d).", all_inven_total));
	return (-1);
    }
	
    /* Read until done */
    while (1)
    {
	u16b n;
      
	/* Get the next item index */
	rd_u16b(&n);
      
	/* Nope, we reached the end */
	if (n == 0xFFFF) break;
      
	/* Get local object */
	i_ptr = &object_type_body;
      
	/* Wipe the object */
	object_wipe(i_ptr);
      
	/* Read the item */
	if (rd_item(i_ptr))
	{
	    note("Error reading item");
	    return (-1);
	}
      
	/* Hack -- verify item */
	if (!i_ptr->k_idx) continue;
      
	/* Verify slot */
	if (n >= ALL_INVEN_TOTAL) return (-1);

	/* Wield equipment */
	if (n >= INVEN_WIELD)
	{
	    /* Copy object */
	    object_copy(&p_ptr->inventory[n], i_ptr);
	  
	    /* Add the weight */
	    p_ptr->total_weight += (i_ptr->number * i_ptr->weight);
	  
	    /* One more item */
	    p_ptr->equip_cnt++;
	}
      
	/* Warning -- backpack is full */
	else if (p_ptr->inven_cnt == INVEN_PACK)
	{
	    /* Oops */
	    note("Too many items in the inventory!");
	  
	    /* Fail */
	    return (-1);
	}
      
	/* Carry inventory */
	else
	{
	    /* Get a slot */
	    n = slot++;
	  
	    /* Copy object */
	    object_copy(&p_ptr->inventory[n], i_ptr);
	  
	    /* Add the weight */
	    p_ptr->total_weight += (i_ptr->number * i_ptr->weight);
	  
	    /* One more item */
	    p_ptr->inven_cnt++;
	}
    }

    save_quiver_size(p_ptr);
  
    /* Success */
    return (0);
}



/**
 * Read a store
 */
int rd_stores(u32b version)
{
    int i;
    byte num, max_stores, store_inven_max, store_choices;

  
    /* Read the stores */
    rd_byte(&max_stores);
    if (max_stores > MAX_STORES)
    {
	note(format("Too many stores (%d).", max_stores));
	return (-1);
    }

    rd_byte(&store_inven_max);
    if (store_inven_max > STORE_INVEN_MAX)
    {
	note(format("Too large store inventory (%d).", store_inven_max));
	return (-1);
    }

    rd_byte(&store_choices);
    if (store_choices > STORE_CHOICES)
    {
	note(format("Too many store choices (%d).", store_choices));
	return (-1);
    }

    for (i = 0; i < max_stores; i++)
    {
	store_type *st_ptr = &store[i];
	int j;
	u32b tmp32u;

	/* Read the basic info */
	rd_s32b(&st_ptr->store_open);
	rd_s16b(&st_ptr->insult_cur);
	rd_byte(&st_ptr->owner);
	rd_byte(&num);
	rd_s16b(&st_ptr->good_buy);
	rd_s16b(&st_ptr->bad_buy);
  
	/* Read the items */
	for (j = 0; j < num; j++)
	{
	    object_type *i_ptr;
	    object_type object_type_body;
      
	    /* Get local object */
	    i_ptr = &object_type_body;
      
	    /* Wipe the object */
	    object_wipe(i_ptr);
      
	    /* Read the item */
	    rd_item(i_ptr);
      
	    /* Acquire valid items */
	    if (st_ptr->stock_num < store_inven_max)
	    {
		int k = st_ptr->stock_num++;
	  
		/* Acquire the item */
		object_copy(&st_ptr->stock[k], i_ptr);
	    }
	}

	/* Load the order information */
	if (st_ptr->type == STORE_MERCH)
	{
    
	    /* Look at the ordered slots */
	    for (j = store_inven_max; j < store_choices; j++)
	    {
		/* Save the table entry */
		rd_s16b(&st_ptr->table[j]);
	    }
	}

	/* Expansion */
	rd_u32b(&tmp32u);
	
    }
	
    /* Success */
    return (0);
}


/**
 * Read the dungeon
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 *
 * Note that the size of the dungeon is now hard-coded to
 * DUNGEON_HGT by DUNGEON_WID, and any dungeon with another
 * size will be silently discarded by this routine.
 */
int rd_dungeon(u32b version)
{
    int i, n, y, x;
  
    s16b stage;
    s16b last_stage;
    s16b py, px;
    s16b ymax, xmax;
  
    byte count;
    byte tmp8u;
    u16b tmp16u, cave_size;
  
  
    /* Only if the player's alive */
    if (p_ptr->is_dead)
	return 0;

    /*** Basic info ***/
  
    /* Hack - rewrite stage_map if necessary */
    if (p_ptr->map == MAP_DUNGEON)
    {
	for (i = 0; i < NUM_STAGES; i++)
	    for (n = 0; n < 9; n++)
		stage_map[i][n] = dungeon_map[i][n];
    }

    else if (p_ptr->map == MAP_COMPRESSED)
    {
	for (i = 0; i < NUM_STAGES; i++)
	    for (n = 0; n < 9; n++)
		stage_map[i][n] = compressed_map[i][n];

	for (i = 0; i < NUM_TOWNS; i++)
	    towns[i] = compressed_towns[i];
    }
    else if (p_ptr->map == MAP_EXTENDED)
    {
	for (i = 0; i < NUM_STAGES; i++)
	    for (n = 0; n < 9; n++)
		stage_map[i][n] = extended_map[i][n];

    }
    else if (p_ptr->map == MAP_FANILLA)
    {
	for (i = 0; i < NUM_STAGES; i++)
	    for (n = 0; n < 9; n++)
		stage_map[i][n] = fanilla_map[i][n];

	/* Hack of the year */
	for (i = 0; i < z_info->r_max; i++)
	{
	    monster_race *r_ptr = &r_info[i];

	    /* In ths mode, Sauron and his forms need to be level 99 */
	    if (rf_has(r_ptr->flags, RF_GAURHOTH) && (r_ptr->level == 85))
		r_info[i].level = 99;
	}
		
    }


    /* Header info */
    rd_s16b(&stage);
    rd_s16b(&last_stage);
    rd_s16b(&py);
    rd_s16b(&px);
    rd_s16b(&ymax);
    rd_s16b(&xmax);
    rd_u16b(&cave_size);
    rd_u16b(&tmp16u);

    /* Always at least two bytes of cave_info */
    cave_size = MAX(2, cave_size);
  
    /* Ignore illegal dungeons */
    if ((stage < 0) || (stage >= NUM_STAGES))
    {
	note(format("Ignoring illegal stage (%d)", stage));
	return (0);
    }
  
    /* Ignore illegal dungeons */
    if ((ymax != DUNGEON_HGT) || (xmax != DUNGEON_WID))
    {
	/* XXX XXX XXX */
	note(format("Ignoring illegal dungeon size (%d,%d).", xmax, ymax));
	return (0);
    }
  
    /* Ignore illegal dungeons */
    if ((px < 0) || (px >= DUNGEON_WID) || (py < 0) || (py >= DUNGEON_HGT))
    {
	note(format("Ignoring illegal player location (%d,%d).", px, py));
	return (1);
    }
  
  
  
    /*** Run length decoding ***/
  
    /* Loop across bytes of cave_info */
    for (n = 0; n < cave_size; n++)
    {
	/* Load the dungeon data */
	for (x = y = 0; y < DUNGEON_HGT; )
	{
	    /* Grab RLE info */
	    rd_byte(&count);
	    rd_byte(&tmp8u);
	    
	    /* Apply the RLE info */
	    for (i = count; i > 0; i--)
	    {
		/* Extract "info" */
		cave_info[y][x][n] = tmp8u;
	  
		/* Advance/Wrap */
		if (++x >= DUNGEON_WID)
		{
		    /* Wrap */
		    x = 0;
		    
		    /* Advance/Wrap */
		    if (++y >= DUNGEON_HGT) break;
		}
	    }
	}
    }

   /*** Run length decoding ***/
  
    /* Load the dungeon data */
    for (x = y = 0; y < DUNGEON_HGT; )
    {
	/* Grab RLE info */
	rd_byte(&count);
	rd_byte(&tmp8u);
      
	/* Apply the RLE info */
	for (i = count; i > 0; i--)
	{
	    /* Extract "feat" */
	    cave_set_feat(y, x, tmp8u);
	  
	    /* Advance/Wrap */
	    if (++x >= DUNGEON_WID)
	    {
		/* Wrap */
		x = 0;
	      
		/* Advance/Wrap */
		if (++y >= DUNGEON_HGT) break;
	    }
	}
    }
  
  
    /*** Player ***/
  
    /* Save stage */
    p_ptr->stage = stage;
    p_ptr->last_stage = last_stage;
  
    /* Place player in dungeon */
    if (!player_place(py, px))
    {
	note(format("Cannot place player (%d,%d)!", py, px));
	return (-1);
    }
  
    /* Hack -- Repair dungeon level */
    if (stage_map[p_ptr->stage][LOCALITY] == UNDERWORLD) 
    {
	stage_map[p_ptr->stage][UP] = p_ptr->last_stage;
	stage_map[p_ptr->last_stage][DOWN] = p_ptr->stage;
	stage_map[p_ptr->stage][DEPTH] = stage_map[p_ptr->last_stage][DEPTH] + 1;
    }
    else if (stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP) 
    {
	stage_map[p_ptr->stage][DOWN] = p_ptr->last_stage;
	stage_map[p_ptr->last_stage][UP] = p_ptr->stage;
	stage_map[p_ptr->stage][DEPTH] = stage_map[p_ptr->last_stage][DEPTH] + 1;
    }

    /*** Success ***/
  
    /* The dungeon is ready */
    character_dungeon = TRUE;
  
    /* Success */
    return (0);
}

  
/*** Objects ***/
  
int rd_objects(u32b version)
{
    int i;
    u16b limit;

    /* Only if the player's alive */
    if (p_ptr->is_dead)
	return 0;

    /* Read the item count */
    rd_u16b(&limit);
  
    /* Verify maximum */
    if (limit >= z_info->o_max)
    {
	note(format("Too many (%d) object entries!", limit));
	return (-1);
    }
  
    /* Read the dungeon items */
    for (i = 1; i < limit; i++)
    {
	object_type *i_ptr;
	object_type object_type_body;

	s16b o_idx;
	object_type *o_ptr;
      
	/* Acquire place */
	i_ptr = &object_type_body;
      
	/* Wipe the object */
	object_wipe(i_ptr);
      
	/* Read the item */
	if (rd_item(i_ptr))
	{
	    note("Error reading item");
	    return (-1);
	}
      
	/* Make an object */
	o_idx = o_pop();

	/* Paranoia */
	if (o_idx != i)
	{
	    note(format("Cannot place object %d!", i));
	    return (-1);
	}

	/* Get the object */
	o_ptr = &o_list[o_idx];

	/* Structure Copy */
	object_copy(o_ptr, i_ptr);

	/* Dungeon floor */
	if (!i_ptr->held_m_idx)
	{
	    int x = i_ptr->ix;
	    int y = i_ptr->iy;

	    /* ToDo: Verify coordinates */

	    /* Link the object to the pile */
	    o_ptr->next_o_idx = cave_o_idx[y][x];

	    /* Link the floor to the object */
	    cave_o_idx[y][x] = o_idx;
	}
    }

    return 0;
}
  
  
/*** Monsters ***/
  
int rd_monsters(u32b version)
{
    int i;
    u16b limit;

    /* Only if the player's alive */
    if (p_ptr->is_dead)
	return 0;
	
    /* Read the monster count */
    rd_u16b(&limit);
  
    /* Hack -- verify */
    if (limit >= z_info->m_max)
    {
	note(format("Too many (%d) monster entries!", limit));
	return (161);
    }
  
    /* Read the monsters */
    for (i = 1; i < limit; i++)
    {
	monster_type *n_ptr;
	monster_type monster_type_body;
	monster_race *r_ptr;
      
	int r_idx;
      
	/* Get local monster */
	n_ptr = &monster_type_body;
      
	/* Clear the monster */
	(void)WIPE(n_ptr, monster_type);
      
	/* Read the monster */
	rd_monster(n_ptr);
      
	/* Hack -- ignore "broken" monsters */
	if (n_ptr->r_idx <= 0) continue;
      
	/* Hack -- no illegal monsters. */
	if (n_ptr->r_idx >= z_info->r_max) continue;
      
	/* Access the "r_idx" of the chosen monster */
	r_idx = n_ptr->r_idx;
      
	/* Access the actual race */
	r_ptr = &r_info[r_idx];
      
	/* If a player ghost, some special features need to be added. */
	if (rf_has(r_ptr->flags, RF_PLAYER_GHOST))
	{
	    prepare_ghost(n_ptr->r_idx, n_ptr, TRUE);
	}
      
	/* Place monster in dungeon */
	if (!monster_place(n_ptr->fy, n_ptr->fx, n_ptr))
	{
	    note(format("Cannot place monster %d", i));
	    return (-1);
	}
      
	/* mark minimum range for recalculation */
	n_ptr->min_range = 0;
    }
  
    /* Reacquire objects */
    for (i = 1; i < o_max; ++i)
    {
	object_type *o_ptr;
	monster_type *m_ptr;

	/* Get the object */
	o_ptr = &o_list[i];

	/* Ignore dungeon objects */
	if (!o_ptr->held_m_idx) continue;

	/* Verify monster index */
	if (o_ptr->held_m_idx > z_info->m_max)
	{
	    note("Invalid monster index");
	    return (-1);
	}

	/* Get the monster */
	m_ptr = &m_list[o_ptr->held_m_idx];

	/* Link the object to the pile */
	o_ptr->next_o_idx = m_ptr->hold_o_idx;

	/* Link the monster to the object */
	m_ptr->hold_o_idx = i;
    }

    return 0;
}
  


/**
 * Hack -- strip the old-style "player ghost" info.
 *
 * XXX XXX XXX This is such a nasty hack it hurts.
 */
int rd_ghost(u32b version)
{
    char buf[64];
  
    /* Only if the player's alive */
    if (p_ptr->is_dead)
	return 0;	

    /* Strip name */
    rd_string(buf, 64);
  
    /* Strip old data */
    strip_bytes(60);

    return 0;  
}


int rd_history(u32b version)
{
    u32b tmp32u;
    size_t i;
	
    history_clear();
	
    rd_u32b(&tmp32u);
    for (i = 0; i < tmp32u; i++)
    {
	s32b turn;
	s16b dlev, clev;
	u16b type;
	byte art_name;
	char text[80];
		
	rd_u16b(&type);
	rd_s32b(&turn);
	rd_s16b(&dlev);
	rd_s16b(&clev);
	rd_byte(&art_name);
	rd_string(text, sizeof(text));
		
	history_add_full(type, art_name, dlev, clev, turn, text);
    }

    return 0;
}

int rd_traps(u32b version)
{
    int i;
    u32b tmp32u;

    rd_byte(&trf_size);
    rd_s16b(&trap_max);

    for (i = 0; i < trap_max; i++)
    {
	trap_type *t_ptr = &trap_list[i];

	rd_trap(t_ptr);
    }

    /* Expansion */
    rd_u32b(&tmp32u);

    return 0;
}
