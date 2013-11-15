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
#include "squelch.h"
#include "history.h"
#include "monster.h"
#include "option.h"
#include "savefile.h"


/*
 * Write a description of the character
 */
void wr_description(void)
{
    char buf[1024];

    if (p_ptr->is_dead)
	strnfmt(buf, sizeof buf, "%s, dead (%s)", op_ptr->full_name, 
		p_ptr->died_from);
    else
	strnfmt(buf, sizeof buf, "%s, L%d %s %s",
		op_ptr->full_name,
		p_ptr->lev,
		rp_ptr->name,
		cp_ptr->name);

    wr_string(buf);
}


/**
 * Write an "item" record
 */
static void wr_item(object_type *o_ptr)
{
    size_t i;

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
  
    wr_byte(o_ptr->origin);
    wr_byte(o_ptr->origin_stage);
    wr_u16b(o_ptr->origin_xtra);

    /* Flags */
    for (i = 0; i < OF_SIZE; i++)
	wr_byte(o_ptr->flags_obj[i]);
    for (i = 0; i < CF_SIZE; i++)
	wr_byte(o_ptr->flags_curse[i]);
    for (i = 0; i < CF_SIZE; i++)
	wr_byte(o_ptr->id_curse[i]);
    for (i = 0; i < OF_SIZE; i++)
	wr_byte(o_ptr->id_obj[i]);
    for (i = 0; i < IF_SIZE; i++)
	wr_byte(o_ptr->id_other[i]);

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

    /* Held by monster index */
    wr_s16b(o_ptr->held_m_idx);
  
    /* Activation */
    wr_u16b(o_ptr->effect);
    wr_u16b(o_ptr->time.base);
    wr_u16b(o_ptr->time.dice);
    wr_u16b(o_ptr->time.sides);
  
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

    /* Expansion */
    wr_u32b(0);
}
  

/**
 * Write a "monster" record
 */
static void wr_monster(monster_type *m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    /* Special treatment for player ghosts */
    if (rf_has(r_ptr->flags, RF_PLAYER_GHOST))
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

    /* Expansion */
    wr_u32b(0);
}

/**
 * Write a trap record
 */
static void wr_trap(trap_type *t_ptr)
{
    size_t i;

    wr_byte(t_ptr->t_idx);
    wr_byte(t_ptr->fy);
    wr_byte(t_ptr->fx);
    wr_byte(t_ptr->xtra);

    for (i = 0; i < TRF_SIZE; i++)
	wr_byte(t_ptr->flags[i]);
}

/**
 * Write RNG state
 */
void wr_randomizer(void)
{
    int i;

    /* current value for the simple RNG */
    wr_u32b(Rand_value);

    /* state index */
    wr_u32b(state_i);

    /* RNG variables */
    wr_u32b(z0);
    wr_u32b(z1);
    wr_u32b(z2);

    /* RNG state */
    for (i = 0; i < RAND_DEG; i++)
	wr_u32b(STATE[i]);

    /* NULL padding */
    for (i = 0; i < 59 - RAND_DEG; i++)
	wr_u32b(0);
}


/**
 * Write the "options"
 */
void wr_options(void)
{
    int i, k;
  
    u32b flag[8];
    u32b mask[8];
  
  
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
  
    /* Write lazymove delay */
    wr_u16b(lazymove_delay);
  
  
    /*** Normal options ***/
  
    for (i = 0; i < OPT_MAX; i++) {
	const char *name = option_name(i);
	if (!name)
	    continue;

	wr_string(name);
	wr_byte(op_ptr->opt[i]);
    }

    /* Sentinel */
    wr_byte(0);

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


void wr_messages(void)
{
	s16b i;
	u16b num;

	num = messages_num();
	if (num > 80) num = 80;
	wr_u16b(num);

	/* Dump the messages (oldest first!) */
	for (i = num - 1; i >= 0; i--)
	{
		wr_string(message_str(i));
		wr_u16b(message_type(i));
	}
}


/**
 * Write the monster memory, including flag sizes
 */
void wr_monster_memory(void)
{
    size_t i;
    int r_idx;

    wr_u16b(z_info->r_max);
    wr_byte(RF_SIZE);
    wr_byte(RSF_SIZE);
    wr_byte(MONSTER_BLOW_MAX);
    for (r_idx = 0; r_idx < z_info->r_max; r_idx++)
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
	wr_byte(l_ptr->cast_spell);
  
	/* Count blows of each type */
	for (i = 0; i < MONSTER_BLOW_MAX; i++)
	    wr_byte(l_ptr->blows[i]);
  
	/* Memorize flags */
	for (i = 0; i < RF_SIZE; i++)
	    wr_byte(l_ptr->flags[i]);
	for (i = 0; i < RSF_SIZE; i++)
	    wr_byte(l_ptr->spell_flags[i]);

  
  
	/* Monster limit per level */
	wr_byte(r_ptr->max_num);
	wr_byte(0);
	wr_byte(0);
	wr_byte(0);
    }
}


/**
 * Write object memory, including flag sizes and other constants.
 */
void wr_object_memory(void)
{
    int k_idx;

    wr_u16b(z_info->k_max);
    wr_byte(OF_SIZE);
    wr_byte(CF_SIZE);
    wr_byte(IF_SIZE);
    wr_byte(MAX_P_RES);
    wr_byte(A_MAX);
    wr_byte(MAX_P_BONUS);
    wr_byte(MAX_P_SLAY);
    wr_byte(MAX_P_BRAND);
    for (k_idx = 0; k_idx < z_info->k_max; k_idx++)
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

	/* Expansion */
	wr_u32b(0);
    }
}


void wr_quests(void)
{
    int i;
    u16b tmp16u;

    /* Hack -- Dump the quests */
    tmp16u = MAX_Q_IDX;
    wr_u16b(tmp16u);
    for (i = 0; i < tmp16u; i++)
    {
	wr_u16b((u16b)q_list[i].stage);
	wr_byte(0);
	wr_byte(0);
    }
}


void wr_artifacts(void)
{
    size_t i;
    u16b tmp16u;

    /* Record the total number of artifacts. */
    tmp16u = z_info->a_max;
    wr_u16b(tmp16u);
  
    /* Record the first random artifact index. */
    tmp16u = ART_MIN_RANDOM;
    wr_u16b(tmp16u);
  
  
    /* Write the artifact info. */
    for (i = 0; i < ART_MIN_RANDOM; i++)
    {
	artifact_type *a_ptr = &a_info[i];
	
	wr_byte(a_ptr->created);
	wr_byte(a_ptr->seen);
	/* Store set completion info */
	wr_byte(a_ptr->set_bonus ? 1 : 0);
	
	/* Expansion */
	wr_u32b(0);
    }

    /* Random artifacts are specific to each player. */
    for (i = ART_MIN_RANDOM; i < z_info->a_max; i++)
    {
	artifact_type *a_ptr = &a_info[i];
	size_t j;

	wr_string(format("%s", a_ptr->name));
	
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
	  
	for (j = 0; j < OF_SIZE; j++)
	    wr_byte(a_ptr->flags_obj[j]);
	for (j = 0; j < CF_SIZE; j++)
	    wr_byte(a_ptr->flags_curse[j]);
	  
	wr_byte(a_ptr->level);
	wr_byte(a_ptr->rarity);
	  
	wr_byte(a_ptr->created);
	wr_byte(a_ptr->seen);
	wr_byte(a_ptr->effect);
	wr_u16b(a_ptr->time.base);
	wr_u16b(a_ptr->time.dice);
	wr_u16b(a_ptr->time.sides);
	wr_u16b(a_ptr->charge.base);
	wr_u16b(a_ptr->charge.dice);
	wr_u16b(a_ptr->charge.sides);

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


/**
 * Write some player info
 */
void wr_player(void)
{
    int i;
  
    wr_string(op_ptr->full_name);
  
    wr_string(p_ptr->died_from);
  
    wr_string(p_ptr->history);
  
    /* Race/Class/Gender/Spells */
    wr_byte(p_ptr->prace);
    wr_byte(p_ptr->pclass);
    wr_byte(p_ptr->psex);
    wr_byte(op_ptr->name_suffix);
  
    wr_byte(p_ptr->hitdie);
    wr_byte(0);
  
    wr_s16b(p_ptr->age);
    wr_s16b(p_ptr->ht);
    wr_s16b(p_ptr->wt);
  
  
    /* Dump the stats (maximum and current) */
    for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_max[i]);
    for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_cur[i]);
    for (i = 0; i < A_MAX; ++i) wr_s16b(p_ptr->stat_birth[i]);
	
    wr_s16b(p_ptr->ht_birth);
    wr_s16b(p_ptr->wt_birth);
    wr_s16b(p_ptr->sc_birth);
    wr_u32b(p_ptr->au_birth);

    /* Druid damage */
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
    wr_byte(MAX_RECALL_PTS);
    for (i = 0; i < MAX_RECALL_PTS; i++) wr_s16b(p_ptr->recall[i]);
    wr_s16b(p_ptr->recall_pt);

    /* Game map and modes */
    wr_byte(p_ptr->map);
    wr_byte(GAME_MODE_MAX);
    for (i = 0; i < GAME_MODE_MAX; i++)
	wr_byte(p_ptr->game_mode[i]);
  
    /* More info */
    wr_s16b(p_ptr->speed_boost);	/* Specialty Fury */
    wr_s16b(p_ptr->heighten_power);	/* Specialty Heighten Magic */
    wr_s16b(p_ptr->sc);
  
    wr_s16b(p_ptr->food);
    wr_s16b(p_ptr->energy);
    wr_s16b(p_ptr->word_recall);
    wr_s16b(p_ptr->state.see_infra);

    /* Find the number of timed effects */
    wr_byte(TMD_MAX);

    /* Read all the effects, in a loop */
    for (i = 0; i < TMD_MAX; i++)
	wr_s16b(p_ptr->timed[i]);

  
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
    wr_byte(RUNE_TAIL);
    for (i = 0; i < RUNE_TAIL; i++)
	wr_byte(num_runes_on_level[i]);
    wr_byte((byte)mana_reserve);
  
    wr_byte(p_ptr->themed_level); /* Stores the current themed level. -LM- */
  
    /* Stores what themed levels have already appeared. -LM- */
    wr_u32b(p_ptr->themed_level_appeared);
  
    /* Specialty abilties */
    wr_byte(MAX_SPECIALTIES);
    for (i = 0; i < MAX_SPECIALTIES; i++) wr_byte(p_ptr->specialty_order[i]);
  
    /* Ignore some flags */
    wr_s16b(0L);	/* oops */
  
	/* Expansion */
	wr_u32b(0);
	wr_u32b(0);
}


/**
 * Write autoinscribe & squelch item-quality submenu to the savefile
 */
void wr_squelch(void)
{
    int i, j;
  
    /* Write number of quality squelch bytes */
    wr_byte(Q_TV_MAX * SQUELCH_MAX);
    for (i = 0; i < Q_TV_MAX; i++)
	for (j = 0; j < SQUELCH_MAX; j++)
	    wr_byte(squelch_profile[i][j]);
  
    /* Write ego-item squelch bits */
    wr_u16b(z_info->e_max);
    for (i = 0; i < z_info->e_max; i++)
    {
	byte flags = 0;
      
	/* Figure out and write the everseen flag */
	if (e_info[i].squelch) flags |= 0x01;
	if (e_info[i].everseen) flags |= 0x02;
	wr_byte(flags);

	/* Expansion */
	wr_u32b(0);
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


void wr_misc(void)
{
    int i;

    /* Write the "object seeds" */
    wr_u32b(seed_flavor);
    wr_byte(NUM_TOWNS);
    for (i = 0; i < NUM_TOWNS; i++) wr_u32b(seed_town[i]);
  
  
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
    wr_byte(do_feeling);
  
    /* Current turn */
    wr_s32b(turn);

}

/* Dump the "player hp" entries */
void wr_player_hp(void)
{
    int i;

    wr_u16b(PY_MAX_LEVEL);
    for (i = 0; i < PY_MAX_LEVEL; i++)
	wr_s16b(p_ptr->player_hp[i]);
}
  
  
void wr_player_spells(void)
{
    int i;

    wr_u16b(PY_MAX_SPELLS);

    for (i = 0; i < PY_MAX_SPELLS; i++)
	wr_byte(p_ptr->spell_flags[i]);

    for (i = 0; i < PY_MAX_SPELLS; i++)
	wr_byte(p_ptr->spell_order[i]);

    /* Expansion */
    wr_u32b(0);
}

void wr_randarts(void)
{
    return;
}

void wr_inventory(void)
{
    int i;

    wr_byte(ALL_INVEN_TOTAL);
    /* Write the inventory */
    for (i = 0; i < ALL_INVEN_TOTAL; i++)
    {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx) continue;

	/* Dump index */
	wr_u16b((u16b)i);

	/* Dump object */
	wr_item(o_ptr);
    }

    /* Add a sentinel */
    wr_u16b(0xFFFF);

    /* Expansion */
    wr_u32b(0);
}


/**
 * Write a "store" record
 */
void wr_stores(void)
{
    int i;

    wr_byte(MAX_STORES);
    wr_byte(STORE_INVEN_MAX);
    wr_byte(STORE_CHOICES);
    for (i = 0; i < MAX_STORES; i++)
    {
	const store_type *st_ptr = &store[i];
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
	    for (j = STORE_INVEN_MAX; j < STORE_CHOICES; j++)
	    {
		/* Save the table entry */
		wr_s16b(st_ptr->table[j]);
	    }

	/* Expansion */
	wr_u32b(0);
    }
}


/**
 * Write the current dungeon
 */
void wr_dungeon(void)
{
    int y, x;
    size_t i;
  
    byte tmp8u;
  
    byte count;
    byte prev_char;
  
  
    if (p_ptr->is_dead)
	return;

    /*** Basic info ***/
  
    /* Dungeon specific info follows */
    wr_u16b(p_ptr->stage);
    wr_u16b(p_ptr->last_stage);
    wr_u16b(p_ptr->py);
    wr_u16b(p_ptr->px);
    wr_u16b(DUNGEON_HGT);
    wr_u16b(DUNGEON_WID);
    wr_u16b(SQUARE_SIZE);
    wr_u16b(0);
  
  
    /*** Simple "Run-Length-Encoding" of cave ***/
  
    /* Loop across bytes of cave_info */
    for (i = 0; i < SQUARE_SIZE; i++)
    {
	/* Note that this will induce two wasted bytes */
	count = 0;
	prev_char = 0;
	
	/* Dump the cave */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
	    for (x = 0; x < DUNGEON_WID; x++)
	    {
		/* Extract the important cave_info flags */
		tmp8u = cave_info[y][x][i];
	  
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
}
  
/*** Dump objects ***/
  
void wr_objects(void)
{
    int i;

    if (p_ptr->is_dead)
	return;
	
    /* Total objects */
    wr_u16b(o_max);
  
    /* Dump the objects */
    for (i = 1; i < o_max; i++)
    {
	object_type *o_ptr = &o_list[i];
      
	/* Dump it */
	wr_item(o_ptr);
    }

    /* Expansion */
    wr_u32b(0);
}
  
  
/*** Dump the monsters ***/
  
void wr_monsters(void)
{
    int i;

    if (p_ptr->is_dead)
	return;

    /* Total monsters */
    wr_u16b(m_max);
  
    /* Dump the monsters */
    for (i = 1; i < m_max; i++)
    {
	monster_type *m_ptr = &m_list[i];
      
	/* Dump it */
	wr_monster(m_ptr);
    }

    /* Expansion */
    wr_u32b(0);
}


/**
 * Hack -- Write the "ghost" info
 */
void wr_ghost(void)
{
    int i;
  
    /* Name */
    wr_string("Broken Ghost");
  
    /* Hack -- stupid data */
    for (i = 0; i < 60; i++) wr_byte(0);
}



void wr_history(void)
{
	size_t i;
	u32b tmp32u = history_get_num();

	wr_u32b(tmp32u);
	for (i = 0; i < tmp32u; i++)
	{
		wr_u16b(history_list[i].type);
		wr_s32b(history_list[i].turn);
		wr_s16b(history_list[i].place);
		wr_s16b(history_list[i].clev);
		wr_byte(history_list[i].a_idx);
		wr_string(history_list[i].event);
	}

    /* Expansion */
    wr_u32b(0);
}


void wr_traps(void)
{
    int i;

    wr_byte(TRF_SIZE);
    wr_s16b(trap_max);

    for (i = 0; i < trap_max; i++)
    {
	trap_type *t_ptr = &trap_list[i];

	wr_trap(t_ptr);
    }

    /* Expansion */
    wr_u32b(0);
}
