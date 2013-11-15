/** \file monmove.c 
    \brief Monster movement

 * Monster learning, monster distance attacks and spells, fear, flow/
 * movement, monster AI affecting movement and spells, process a monster 
 * (with spells and actions of all kinds, reproduction, effects of any 
 * terrain on monster movement, picking up and destroying objects), 
 * process all monsters.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick & Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * Additional code and concepts by David Reeves Sward, Keldon Jones, 
 * and others.
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
#include "files.h"
#include "monster.h" 
#include "spells.h"
#include "squelch.h"
#include "trap.h"

/**
 * Monsters will run up to 25 grids away
 */
#define FLEE_RANGE      MAX_SIGHT + 5

/**
 * Terrified monsters will turn to fight if they are slower than the
 * character, and closer to him than this distance.
 */
#define TURN_RANGE      3

/**
 * Calculate minimum and desired combat ranges.  -BR-
 */
static void find_range(monster_type * m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    u16b p_lev, m_lev;
    u16b p_chp, p_mhp;
    u16b m_chp, m_mhp;
    u32b p_val, m_val;

    /* All "afraid" monsters will run away */
    if (m_ptr->monfear)
	m_ptr->min_range = FLEE_RANGE;

    /* Some monsters run when low on mana */
    else if ((rf_has(r_ptr->flags, RF_LOW_MANA_RUN))
	     && (m_ptr->mana < r_ptr->mana / 6))
	m_ptr->min_range = FLEE_RANGE;

    else {

	/* Minimum distance - stay at least this far if possible */
	m_ptr->min_range = 1;

	/* Examine player power (level) */
	p_lev = p_ptr->lev;

	/* Hack - increase p_lev based on specialty abilities */

	/* Examine monster power (level plus morale) */
	m_lev = r_ptr->level + (cave_m_idx[m_ptr->fy][m_ptr->fx] & 0x08) + 25;

	/* Optimize extreme cases below */
	if (m_lev + 3 < p_lev)
	    m_ptr->min_range = FLEE_RANGE;
	else if (m_lev - 5 < p_lev) {

	    /* Examine player health */
	    p_chp = p_ptr->chp;
	    p_mhp = p_ptr->mhp;

	    /* Examine monster health */
	    m_chp = m_ptr->hp;
	    m_mhp = m_ptr->maxhp;

	    /* Prepare to optimize the calculation */
	    p_val = (p_lev * p_mhp) + (p_chp << 2);	/* div p_mhp */
	    m_val = (m_lev * m_mhp) + (m_chp << 2);	/* div m_mhp */

	    /* Strong players scare strong monsters */
	    if (p_val * m_mhp > m_val * p_mhp)
		m_ptr->min_range = FLEE_RANGE;
	}
    }

    if (m_ptr->min_range < FLEE_RANGE) {
	/* Creatures that don't move never like to get too close */
	if (rf_has(r_ptr->flags, RF_NEVER_MOVE))
	    m_ptr->min_range += 3;

	/* Spellcasters that don't stike never like to get too close */
	if (rf_has(r_ptr->flags, RF_NEVER_BLOW))
	    m_ptr->min_range += 3;
    }

    /* Maximum range to flee to (reduced elsewhere for themed levels */
    if (!(m_ptr->min_range < FLEE_RANGE))
	m_ptr->min_range = FLEE_RANGE;

    /* Nearby monsters that cannot run away will not become run unless
     * completely afraid */
    else if ((m_ptr->cdis < TURN_RANGE) && 
	     (m_ptr->mspeed < p_ptr->state.pspeed))
	m_ptr->min_range = 1;

    /* Now find prefered range */
    m_ptr->best_range = m_ptr->min_range;

    /* Archers are quite happy at a good distance */
    if (rf_has(r_ptr->flags, RF_ARCHER))
	m_ptr->best_range += 3;

    if (r_ptr->freq_ranged > 24) {
	/* Heavy spell casters will sit back and cast */
	if (m_ptr->mana > r_ptr->mana / 4)
	    m_ptr->best_range += 3;

	/* Breathers like point blank range */
	if ((flags_test
	     (r_ptr->spell_flags, RSF_SIZE, RSF_BREATH_MASK, FLAG_END))
	    && (m_ptr->best_range < 6) && (m_ptr->hp > m_ptr->maxhp / 2))
	    m_ptr->best_range = 6;
    }
}


/**
 * Given a central direction at position [dir #][0], return a series 
 * of directions radiating out on both sides from the central direction 
 * all the way back to its rear.
 * 
 * Side directions come in pairs; for example, directions '1' and '3' 
 * flank direction '2'.  The code should know which side to consider 
 * first.  If the left, it must add 10 to the central direction to 
 * access the second part of the table.
 */
static byte side_dirs[20][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},	/* bias right */
    {1, 4, 2, 7, 3, 8, 6, 9},
    {2, 1, 3, 4, 6, 7, 9, 8},
    {3, 2, 6, 1, 9, 4, 8, 7},
    {4, 7, 1, 8, 2, 9, 3, 6},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {6, 3, 9, 2, 8, 1, 7, 4},
    {7, 8, 4, 9, 1, 6, 2, 3},
    {8, 9, 7, 6, 4, 3, 1, 2},
    {9, 6, 8, 3, 7, 2, 4, 1},

    {0, 0, 0, 0, 0, 0, 0, 0},	/* bias left */
    {1, 2, 4, 3, 7, 6, 8, 9},
    {2, 3, 1, 6, 4, 9, 7, 8},
    {3, 6, 2, 9, 1, 8, 4, 7},
    {4, 1, 7, 2, 8, 3, 9, 6},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {6, 9, 3, 8, 2, 7, 1, 4},
    {7, 4, 8, 1, 9, 2, 6, 3},
    {8, 7, 9, 4, 6, 1, 3, 2},
    {9, 8, 6, 7, 3, 4, 2, 1}
};


/**
 * Get and return the strength (age) of scent in a given grid.
 *
 * Return "-1" if no scent exists in the grid.
 */
int get_scent(int y, int x)
{
    int age;
    int scent;

    /* Check Bounds */
    if (!(in_bounds(y, x)))
	return (-1);

    /* Sent trace? */
    scent = cave_when[y][x];

    /* No scent at all */
    if (!scent)
	return (-1);

    /* Get age of scent */
    age = scent - scent_when;

    /* Return the age of the scent */
    return (age);
}


/**
 * Can the monster catch a whiff of the character?
 *
 * Many more monsters can smell, but they find it hard to smell and 
 * track down something at great range.
 */
static bool monster_can_smell(monster_type * m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    int age;

    /* Get the age of the scent here */
    age = get_scent(m_ptr->fy, m_ptr->fx);

    /* No scent */
    if (age == -1)
	return (FALSE);

    /* Scent is too old */
    if (age > SMELL_STRENGTH)
	return (FALSE);

    /* Canines and Zephyer Hounds are amazing trackers */
    if (strchr("CZ", r_ptr->d_char)) {
	/* I smell a character! */
	return (TRUE);
    }

    /* So are the Nazgul */
    else if ((strchr("W", r_ptr->d_char)) && (rf_has(r_ptr->flags, RF_UNIQUE))) {
	/* Bloodscent! */
	return (TRUE);
    }

    /* Other monsters can sometimes make good use of scent */
    else if (strchr("fkoqyHORTY", r_ptr->d_char)) {
	if (age <= SMELL_STRENGTH - 10) {
	    /* Something's in the air... */
	    return (TRUE);
	}
    }


    /* You're imagining things. */
    return (FALSE);
}

/**
 * Determine if there is a space near the player in which
 * a summoned creature can appear
 */
static int summon_possible(int y1, int x1)
{
    int y, x;
    int num_clear = 0;

    /* Start at the player's location, and check 2 grids in each dir */
    for (y = y1 - 2; y <= y1 + 2; y++) {
	for (x = x1 - 2; x <= x1 + 2; x++) {
	    /* Ignore illegal locations */
	    if (!in_bounds(y, x))
		continue;

	    /* Only check a circular area */
	    if (distance(y1, x1, y, x) > 2)
		continue;

	    /* Hack: no summon on glyph of warding */
	    if (cave_trap_specific(y, x, RUNE_PROTECT))
		continue;

	    /* Require empty floor grid in line of sight */
	    if (cave_empty_bold(y, x) && los(y1, x1, y, x)) {
		num_clear++;
	    }
	}
    }

    return (num_clear);
}

/**
 * Fully update monster's knowledge of the player.
 * Used by player ghost (and all monsters with birth_ai_cheat).
 */
static void update_smart_cheat(int m_idx)
{
    monster_type *m_ptr = &m_list[m_idx];

    /* Know weirdness */
    if (p_ptr->state.free_act)
	m_ptr->smart |= (SM_IMM_FREE);
    if (!p_ptr->msp)
	m_ptr->smart |= (SM_IMM_MANA);
    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
	m_ptr->smart |= (SM_GOOD_SAVE);
    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
	m_ptr->smart |= (SM_PERF_SAVE);

    /* Know immunities */
    if (p_immune(P_RES_ACID))
	m_ptr->smart |= (SM_IMM_ACID);
    if (p_immune(P_RES_ELEC))
	m_ptr->smart |= (SM_IMM_ELEC);
    if (p_immune(P_RES_FIRE))
	m_ptr->smart |= (SM_IMM_FIRE);
    if (p_immune(P_RES_COLD))
	m_ptr->smart |= (SM_IMM_COLD);

    /* Know oppositions */
    if (p_resist_strong(P_RES_ACID))
	m_ptr->smart |= (SM_RES_STRONG_ACID);
    if (p_resist_strong(P_RES_ELEC))
	m_ptr->smart |= (SM_RES_STRONG_ELEC);
    if (p_resist_strong(P_RES_FIRE))
	m_ptr->smart |= (SM_RES_STRONG_FIRE);
    if (p_resist_strong(P_RES_COLD))
	m_ptr->smart |= (SM_RES_STRONG_COLD);
    if (p_resist_strong(P_RES_POIS))
	m_ptr->smart |= (SM_RES_STRONG_POIS);

    /* Know resistances */
    if (p_resist_good(P_RES_ACID))
	m_ptr->smart |= (SM_RES_ACID);
    if (p_resist_good(P_RES_ELEC))
	m_ptr->smart |= (SM_RES_ELEC);
    if (p_resist_good(P_RES_FIRE))
	m_ptr->smart |= (SM_RES_FIRE);
    if (p_resist_good(P_RES_COLD))
	m_ptr->smart |= (SM_RES_COLD);
    if (p_resist_good(P_RES_POIS))
	m_ptr->smart |= (SM_RES_POIS);
    if (p_ptr->state.no_fear)
	m_ptr->smart |= (SM_RES_FEAR);
    if (p_resist_good(P_RES_LIGHT))
	m_ptr->smart |= (SM_RES_LIGHT);
    if (p_resist_good(P_RES_DARK))
	m_ptr->smart |= (SM_RES_DARK);
    if (p_ptr->state.no_blind)
	m_ptr->smart |= (SM_RES_BLIND);
    if (p_resist_good(P_RES_CONFU))
	m_ptr->smart |= (SM_RES_CONFU);
    if (p_resist_good(P_RES_SOUND))
	m_ptr->smart |= (SM_RES_SOUND);
    if (p_resist_good(P_RES_SHARD))
	m_ptr->smart |= (SM_RES_SHARD);
    if (p_resist_good(P_RES_NEXUS))
	m_ptr->smart |= (SM_RES_NEXUS);
    if (p_resist_good(P_RES_NETHR))
	m_ptr->smart |= (SM_RES_NETHR);
    if (p_resist_good(P_RES_CHAOS))
	m_ptr->smart |= (SM_RES_CHAOS);
    if (p_resist_good(P_RES_DISEN))
	m_ptr->smart |= (SM_RES_DISEN);

    return;
}

/**
 * Used to determine the player's known level of resistance to a
 * particular spell.
 *
 * The LRN_xxx constant determines what type of resistance is
 * applicable.  The monster's SM_xxx flags, as well as player
 * condistions, are refered to as needed.
 * -BR-
 */
static int find_resist(int m_idx, int spell_lrn)
{
    monster_type *m_ptr = &m_list[m_idx];

    int a;
    u32b smart;

    /* Nothing Known */
    if (!m_ptr->smart)
	return (0);

    /* get smart flags */
    smart = m_ptr->smart;

    /* Which spell */
    switch (spell_lrn) {
	/* Spells 'resisted' by AC, Dex, etc. Currently no assessment is made */
    case LRN_ARCH:
	{
	    return (0);
	}
	/* As above, but poisonous. */
    case LRN_PARCH:
	{
	    if (smart & (SM_RES_POIS))
		return (10);
	    else
		return (0);
	}
	/* Acid Spells */
    case LRN_ACID:
	{
	    if (smart & (SM_IMM_ACID))
		return (100);
	    else if (smart & (SM_RES_STRONG_ACID))
		return (70);
	    else if (smart & (SM_RES_ACID))
		return (40);
	    else
		return (0);
	}
	/* Lightning Spells */
    case LRN_ELEC:
	{
	    if (smart & (SM_IMM_ELEC))
		return (100);
	    else if (smart & (SM_RES_STRONG_ELEC))
		return (70);
	    else if (smart & (SM_RES_ELEC))
		return (40);
	    else
		return (0);
	}
	/* Fire Spells */
    case LRN_FIRE:
	{
	    if (smart & (SM_IMM_FIRE))
		return (100);
	    else if (smart & (SM_RES_STRONG_FIRE))
		return (70);
	    else if (smart & (SM_RES_FIRE))
		return (40);
	    else
		return (0);
	}
	/* Cold Spells */
    case LRN_COLD:
	{
	    if (smart & (SM_IMM_COLD))
		return (100);
	    else if (smart & (SM_RES_STRONG_COLD))
		return (70);
	    else if (smart & (SM_RES_COLD))
		return (40);
	    else
		return (0);
	}
	/* Ice Spells */
    case LRN_ICE:
	{
	    if (smart & (SM_IMM_COLD))
		a = 90;
	    else if (smart & (SM_RES_STRONG_COLD))
		a = 60;
	    else if (smart & (SM_RES_COLD))
		a = 30;
	    else
		a = 0;
	    if (smart & (SM_RES_SOUND))
		a += 5;
	    if (smart & (SM_RES_SHARD))
		a += 5;
	    return (a);
	}
	/* Poison Spells */
    case LRN_POIS:
	{
	    if (smart & (SM_RES_STRONG_POIS))
		return (80);
	    else if (smart & (SM_RES_POIS))
		return (55);
	    else
		return (0);
	}
	/* Plasma Spells */
    case LRN_PLAS:
	{
	    a = 0;
	    if (smart & (SM_IMM_FIRE))
		a += 50;
	    else if (smart & (SM_RES_STRONG_FIRE))
		a += 35;
	    else if (smart & (SM_RES_FIRE))
		a += 20;
	    if (smart & (SM_IMM_ELEC))
		a += 50;
	    else if (smart & (SM_RES_STRONG_ELEC))
		a += 35;
	    else if (smart & (SM_RES_ELEC))
		a += 20;
	    return (a);
	}
	/* Dragonfire Spells */
    case LRN_DFIRE:
	{
	    a = 0;
	    if (smart & (SM_IMM_FIRE))
		a += 50;
	    else if (smart & (SM_RES_STRONG_FIRE))
		a += 35;
	    else if (smart & (SM_RES_FIRE))
		a += 20;
	    if (smart & (SM_RES_STRONG_POIS))
		a += 80;
	    else if (smart & (SM_RES_POIS))
		a += 55;
	    if (smart & (SM_IMM_FREE))
		a += 5;
	    if (smart & (SM_RES_CHAOS))
		a += 5;
	    return (a);
	}
	/* Light Spells */
    case LRN_LIGHT:
	{
	    if (smart & (SM_RES_LIGHT))
		return (30);
	    else
		return (0);
	}
	/* Darkness Spells */
    case LRN_DARK:
	{
	    if (smart & (SM_RES_DARK))
		return (30);
	    else
		return (0);
	}
	/* Confusion Spells, damage dealing */
    case LRN_CONFU:
	{
	    if (smart & (SM_RES_CONFU))
		return (30);
	    else
		return (0);
	}
	/* Sound Spells */
    case LRN_SOUND:
	{
	    a = 0;
	    if (smart & (SM_RES_SOUND))
		a += 30;
	    if (smart & (SM_RES_CONFU))
		a += 10;
	    if (smart & (SM_PERF_SAVE))
		a += 10;
	    else if (smart & (SM_GOOD_SAVE))
		a += 5;
	    else
		return (a);
	}
	/* Unresistable, but sound prevents stun */
    case LRN_SOUND2:
	{
	    if (smart & (SM_RES_SOUND))
		return (5);
	    else
		return (0);
	}
	/* Shards Spells */
    case LRN_SHARD:
	{
	    if (smart & (SM_RES_SHARD))
		return (30);
	    else
		return (0);
	}
	/* Nexus Spells */
    case LRN_NEXUS:
	{
	    if (smart & (SM_RES_NEXUS))
		return (30);
	    else
		return (0);
	}
	/* Nether Spells */
    case LRN_NETHR:
	{
	    if (smart & (SM_RES_NETHR))
		return (30);
	    else
		return (0);
	}
	/* Chaos Spells */
    case LRN_CHAOS:
	{
	    if (smart & (SM_RES_CHAOS))
		return (30);
	    else
		return (0);
	}
	/* Disenchantment Spells */
    case LRN_DISEN:
	{
	    if (smart & (SM_RES_DISEN))
		return (30);
	    else
		return (0);
	}
	/* Storm Spells */
    case LRN_STORM:
	{
	    a = 0;
	    if (smart & (SM_IMM_ELEC))
		a += 15;
	    else if (smart & (SM_RES_STRONG_ELEC))
		a += 10;
	    else if (smart & (SM_RES_ELEC))
		a += 5;
	    if (smart & (SM_RES_COLD))
		a += 5;
	    if (smart & (SM_RES_ACID))
		a += 5;
	    if (smart & (SM_RES_CONFU))
		a += 10;
	    return (a);
	}
	/* Water Spells */
    case LRN_WATER:
	{
	    a = 0;
	    if (smart & (SM_RES_CONFU))
		a += 10;
	    if (smart & (SM_RES_SOUND))
		a += 5;
	    return (a);
	}
	/* Spells that attack player mana */
    case LRN_MANA:
	{
	    if (smart & (SM_IMM_MANA))
		return (100);
	    else
		return (0);
	}
	/* Spells Requiring Save or Resist Nexus */
    case LRN_NEXUS_SAVE:
	{
	    if (smart & (SM_RES_NEXUS))
		return (100);
	    else if (smart & (SM_PERF_SAVE))
		return (100);
	    else if (smart & (SM_GOOD_SAVE))
		return (30);
	    else
		return (0);
	}
	/* Spells Requiring Save or Resist Fear */
    case LRN_FEAR_SAVE:
	{
	    a = 0;
	    if (smart & (SM_RES_FEAR))
		a = 100;
	    else if (smart & (SM_PERF_SAVE))
		a = 100;
	    else {
		if (smart & (SM_GOOD_SAVE))
		    a += 30;
		if (p_ptr->timed[TMD_AFRAID])
		    a += 50;
	    }
	    return (a);
	}
	/* Spells Requiring Save or Resist Blindness */
    case LRN_BLIND_SAVE:
	{
	    a = 0;
	    if (smart & (SM_RES_BLIND))
		a = 100;
	    else if (smart & (SM_PERF_SAVE))
		a = 100;
	    else {
		if (smart & (SM_GOOD_SAVE))
		    a += 30;
		if (p_ptr->timed[TMD_BLIND])
		    a += 50;
	    }
	    return (a);
	}
	/* Spells Requiring Save or Resist Confusion */
    case LRN_CONFU_SAVE:
	{
	    a = 0;
	    if (smart & (SM_RES_CONFU))
		a = 100;
	    else if (smart & (SM_PERF_SAVE))
		a = 100;
	    else {
		if (smart & (SM_GOOD_SAVE))
		    a += 30;
		if (p_ptr->timed[TMD_CONFUSED])
		    a += 50;
	    }
	    return (a);
	}
	/* Spells Requiring Save or Free Action */
    case LRN_FREE_SAVE:
	{
	    a = 0;
	    if (smart & (SM_IMM_FREE))
		a = 100;
	    else if (smart & (SM_PERF_SAVE))
		a = 100;
	    else if (p_ptr->timed[TMD_PARALYZED])
		a = 80;
	    else {
		if (smart & (SM_GOOD_SAVE))
		    a += 30;
		if (p_ptr->timed[TMD_SLOW])
		    a += 50;
	    }
	    return (a);
	}

	/* Spells Requiring Save */
    case LRN_SAVE:
	{
	    if (smart & (SM_PERF_SAVE))
		return (100);
	    else if (smart & (SM_GOOD_SAVE))
		return (30);
	    else
		return (0);
	}
	/* All element spells */
    case LRN_ALL:
	{
	    int accumulate = 0;

	    /* Check them all */
	    accumulate += find_resist(m_idx, LRN_ACID);
	    accumulate += find_resist(m_idx, LRN_FIRE);
	    accumulate += find_resist(m_idx, LRN_ELEC);
	    accumulate += find_resist(m_idx, LRN_COLD);
	    accumulate += find_resist(m_idx, LRN_POIS);
	    accumulate += find_resist(m_idx, LRN_LIGHT);
	    accumulate += find_resist(m_idx, LRN_DARK);
	    accumulate += find_resist(m_idx, LRN_CONFU);
	    accumulate += find_resist(m_idx, LRN_SOUND);
	    accumulate += find_resist(m_idx, LRN_SHARD);
	    accumulate += find_resist(m_idx, LRN_NEXUS);
	    accumulate += find_resist(m_idx, LRN_NETHR);
	    accumulate += find_resist(m_idx, LRN_CHAOS);
	    accumulate += find_resist(m_idx, LRN_DISEN);
	    return (accumulate / 14);
	}
	/* Anything else */
    default:
	{
	    return (0);
	}
    }
}

/* Temporary spell flags */
static bitflag mon_spells[RSF_SIZE];


/**
 * Used to exclude spells which are too expensive for the
 * monster to cast.  Excludes all spells that cost more than the
 * current available mana.
 *
 * Smart monsters may also exclude spells that use a lot of mana,
 * even if they have enough.
 *
 * -BR-
 */
static void remove_expensive_spells(int m_idx)
{
    monster_type *m_ptr = &m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    int i, max_cost;

    /* Determine maximum amount of mana to be spent */
    /* Smart monsters will usually not blow all their mana on one spell */
    if (rf_has(r_ptr->flags, RF_SMART))
	max_cost = (m_ptr->mana * (4 + randint0(7))) / 10;

    /* Otherwise spend up to the full current mana */
    else
	max_cost = m_ptr->mana;

    /* check innate spells for mana available */
    for (i = FLAG_START; i < RSF_MAX; i++) {
	if (mana_cost[i] > max_cost)
	    (void) rsf_off(mon_spells, i);
    }
}

/**
 * Intellegent monsters use this function to filter away spells
 * which have no benefit.
 */
static void remove_useless_spells(int m_idx, bool los)
{
    monster_type *m_ptr = &m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    /* Don't regain mana if full */
    if (m_ptr->mana >= r_ptr->mana)
	rsf_off(mon_spells, RSF_ADD_MANA);

    /* Don't heal if not enough mana to make it useful */
    if (m_ptr->mana < r_ptr->spell_power / 4)
	rsf_off(mon_spells, RSF_HEAL);

    /* Don't heal if full */
    if (m_ptr->hp >= m_ptr->maxhp)
	rsf_off(mon_spells, RSF_HEAL);

    /* Don't lash if too far or close */
    if ((m_ptr->cdis > 3) || (m_ptr->cdis < 2))
	rsf_off(mon_spells, RSF_LASH);

    /* Don't Haste if Hasted */
    if (m_ptr->mspeed > r_ptr->speed + 5)
	rsf_off(mon_spells, RSF_HASTE);

    /* Don't cure if not needed */
    if (!((m_ptr->stunned) || (m_ptr->monfear)
	  || (m_ptr->mspeed < r_ptr->speed - 5) || (m_ptr->black_breath)))
	rsf_off(mon_spells, RSF_CURE);

    /* Don't jump in already close, or don't want to be close */
    if (!(m_ptr->cdis > m_ptr->best_range) && los)
	rsf_off(mon_spells, RSF_TELE_SELF_TO);
    if (m_ptr->min_range > 5)
	rsf_off(mon_spells, RSF_TELE_SELF_TO);
}

/**
 * Count the number of castable spells.
 *
 * If exactly 1 spell is availble cast it.  If more than more is
 * available, and the random bit is set, pick one.
 *
 * Used as a short cut in 'choose_attack_spell' to circumvent AI 
 * when there is only 1 choice. (random=FALSE)
 *
 * Also used in 'choose_attack_spell' to circumvent AI when 
 * casting randomly (random=TRUE), as with dumb monsters.
 */
static int choose_attack_spell_fast(int m_idx, bool random)
{
    int i, num = 0;
    byte spells[32 * RSF_SIZE];

    /* Extract all spells: "innate", "normal", "bizarre" */
    for (i = FLAG_START, num = 0; i < RSF_MAX; i++) {
	if (rsf_has(mon_spells, i))
	    spells[num++] = i;
    }

    /* Paranoia */
    if (num == 0)
	return 0;

    /* Go quick if possible */
    if (num == 1) {
	/* Hack - Don't cast if known to be immune, unless casting randomly
	 * anyway.  */
	if (!(random)) {
	    if (find_resist(m_idx, spell_desire[spells[0]][D_RES]) == 100)
		return (0);
	}


	/* Otherwise cast the one spell */
	else
	    return (spells[0]);
    }

    /* 
     * If we aren't allowed to choose at random
     * and we have multiple spells left, give up on quick
     * selection
     */
    if (!(random))
	return (0);

    /* Pick at random */
    return (spells[randint0(num)]);
}


/**
 * Have a monster choose a spell.
 *
 * Monster at m_idx uses this function to select a legal attack spell.
 * Spell casting AI is based here.
 *
 * First the code will try to save time by seeing if
 * choose_attack_spell_fast is helpful.  Otherwise, various AI
 * parameters are used to calculate a 'desirability' for each spell.
 * There is some randomness.  The most desirable spell is cast.  
 *
 * archery_only can be used to restrict us to arrow/boulder type attacks.
 *
 * Returns the spell number, of '0' if no spell is selected.
 *
 *-BR-
 */
int choose_ranged_attack(int m_idx, bool archery_only, int shape_rate)
{
    monster_type *m_ptr = &m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    bool rand = FALSE;

    bool los = TRUE;

    bool is_harass = FALSE;
    bool is_best_harass = FALSE;
    bool is_breath = FALSE;

    int i, py = p_ptr->py, px = p_ptr->px, ty, tx;
    int breath_hp, breath_maxhp, path, spaces;

    int want_hps = 0, want_escape = 0, want_mana = 0, want_summon = 0;
    int want_tactic = 0, cur_range = 0;

    int best_spell = 0, best_spell_rating = 0;
    int cur_spell_rating;

    /* Extract the racial spell flags f4 = r_ptr->flags4; f5 = r_ptr->flags5;
     * f6 = r_ptr->flags6; f7 = r_ptr->flags7; */
    rsf_copy(mon_spells, r_ptr->spell_flags);

    /* Player is the target */
    if (m_ptr->hostile < 0) {
	ty = py;
	tx = px;
    }
    /* Another monster is the target */
    else if (m_ptr->hostile > 0) {
	ty = m_list[m_ptr->hostile].fy;
	tx = m_list[m_ptr->hostile].fx;
    }
    /* No spell target */
    else
	return (0);

    /* Check what kinds of spells can hit target */
    path = projectable(m_ptr->fy, m_ptr->fx, ty, tx, PROJECT_CHCK);

    /* do we have the target in sight at all? */
    if (path == PROJECT_NO) {
	/* Flat out 75% chance of not casting if the player is not in sight */
	/* In addition, most spells don't work without a player around */
	if (randint0(4) != 0)
	    return (0);

	los = FALSE;
    }

    /* Are we restricted to archery? */
    /* Note - we have assumed for speed that no archery attacks cost mana, all
     * are 'bolt' like, and that the player can not be highly resistant to
     * them. */
    if (archery_only) {
	/* check for a clean shot */
	if (!(path == PROJECT_CLEAR))
	    return (0);

	/* restrict to archery */
	flags_mask(mon_spells, RSF_SIZE, RSF_ARCHERY_MASK, FLAG_END);

	/* choose at random from restricted list */
	return (choose_attack_spell_fast(m_idx, TRUE));
    }

    /* Remove spells the 'no-brainers' */
    /* Spells that require LOS */
    if (!los) {
	flags_mask(mon_spells, RSF_SIZE, RSF_NO_PLAYER_MASK, FLAG_END);
    } else if (path == PROJECT_NOT_CLEAR) {
	flags_clear(mon_spells, RSF_SIZE, RSF_BOLT_MASK, FLAG_END);
    }

    /* No spells left */
    if (rsf_is_empty(mon_spells))
	return (0);

    /* Spells we can not afford */
    remove_expensive_spells(m_idx);

    /* No spells left */
    if (rsf_is_empty(mon_spells))
	return (0);

    /* Stupid monsters choose at random. */
    if (rf_has(r_ptr->flags, RF_STUPID))
	return (choose_attack_spell_fast(m_idx, TRUE));

    /* Remove spells that have no benefit Does not include the effects of
     * player resists/immunities */
    remove_useless_spells(m_idx, los);

    /* No spells left */
    if (rsf_is_empty(mon_spells))
	return (0);

    /* Sometimes non-dumb monsters cast randomly (though from the restricted
     * list) */
    if ((rf_has(r_ptr->flags, RF_SMART)) && (randint0(10) == 0))
	rand = TRUE;
    if ((!(rf_has(r_ptr->flags, RF_SMART))) && (randint0(5) == 0))
	rand = TRUE;

    /* Try 'fast' selection first. If there is only one spell, choose that
     * spell. If there are multiple spells, choose one randomly if the 'rand'
     * flag is set. Otherwise fail, and let the AI choose. */
    best_spell = choose_attack_spell_fast(m_idx, rand);
    if (best_spell)
	return (best_spell);

    /* If we get this far, we are using the full-up AI.  Calculate some
     * parameters. */

    /* Figure out if we are hurt */
    if (m_ptr->hp < m_ptr->maxhp / 8)
	want_hps += 3;
    else if (m_ptr->hp < m_ptr->maxhp / 4)
	want_hps += 2;
    else if (m_ptr->hp < m_ptr->maxhp / 2)
	want_hps++;

    /* Figure out if we want mana */
    if (m_ptr->mana < r_ptr->mana / 4)
	want_mana += 2;
    else if (m_ptr->mana < r_ptr->mana / 2)
	want_mana++;

    /* Figure out if we want to scram */
    if (want_hps)
	want_escape = want_hps - 1;
    if (m_ptr->min_range == FLEE_RANGE)
	want_escape++;

    /* Desire to keep minimum distance */
    if (m_ptr->cdis < m_ptr->best_range)
	want_tactic++;
    if (m_ptr->cdis < m_ptr->min_range)
	want_tactic += (m_ptr->min_range - m_ptr->cdis + 1) / 2;
    if (want_tactic > 3)
	want_tactic = 3;

    /* Check terrain for purposes of summoning spells */
    spaces = summon_possible(py, px);
    if (spaces > 10)
	want_summon = 3;
    else if (spaces > 3)
	want_summon = 2;
    else if (spaces > 0)
	want_summon = 1;

    /* Find monster properties; Add an offset so that things are OK near zero */
    breath_hp = (m_ptr->hp > 2000 ? m_ptr->hp : 2000);
    breath_maxhp = (m_ptr->maxhp > 2000 ? m_ptr->maxhp : 2000);

    /* Cheat if requested, or if a player ghost. */
    if (MODE(AI_CHEAT) || (rf_has(r_ptr->flags, RF_PLAYER_GHOST)))
	update_smart_cheat(m_idx);

    /* Now check every remaining spell */
    for (i = FLAG_START; i < RSF_MAX; i++) {
	bitflag temp[RSF_SIZE];

	/* Do we even have this spell? */
	if (!rsf_has(mon_spells, i))
	    continue;

	/* Is it a breath? */
	flags_init(temp, RSF_SIZE, RSF_BREATH_MASK, FLAG_END);
	is_breath = rsf_has(temp, i);

	/* Is it a harassing spell? */
	flags_init(temp, RSF_SIZE, RSF_HARASS_MASK, FLAG_END);
	is_harass = rsf_has(temp, i);

	/* Base Desirability */
	cur_spell_rating = spell_desire[i][D_RES];

	/* modified for breath weapons */
	if (is_breath)
	    cur_spell_rating = (cur_spell_rating * breath_hp) / breath_maxhp;

	/* Bonus if want summon and this spell is helpful */
	if (spell_desire[i][D_SUMM] && want_summon)
	    cur_spell_rating += want_summon * spell_desire[i][D_SUMM];

	/* Bonus if wounded and this spell is helpful */
	if (spell_desire[i][D_HURT] && want_hps)
	    cur_spell_rating += want_hps * spell_desire[i][D_HURT];

	/* Bonus if low on mana and this spell is helpful */
	if (spell_desire[i][D_MANA] && want_mana)
	    cur_spell_rating += want_mana * spell_desire[i][D_MANA];

	/* Bonus if want to flee and this spell is helpful */
	if (spell_desire[i][D_ESC] && want_escape)
	    cur_spell_rating += want_escape * spell_desire[i][D_ESC];

	/* Bonus if want a tactical move and this spell is helpful */
	if (spell_desire[i][D_TACT] && want_tactic)
	    cur_spell_rating += want_tactic * spell_desire[i][D_TACT];

	/* Penalty if this spell is resisted */
	if (spell_desire[i][D_RES])
	    cur_spell_rating =
		(cur_spell_rating *
		 (100 - find_resist(m_idx, spell_desire[i][D_RES]))) / 100;

	/* Penalty for range if attack drops off in power */
	if (spell_range[i]) {
	    cur_range = m_ptr->cdis;
	    while (cur_range-- > spell_range[i])
		cur_spell_rating =
		    (cur_spell_rating * spell_desire[i][D_RANGE]) / 100;
	}

	/* Bonus for harassment spells at first */
	if (is_harass && m_ptr->harass)
	    cur_spell_rating += 2 * cur_spell_rating / 3;

	/* Random factor; less random for smart monsters */
	if (rf_has(r_ptr->flags, RF_SMART))
	    cur_spell_rating *= 16 + randint0(9);
	else
	    cur_spell_rating *= 12 + randint0(17);

	/* Deflate for testing purposes */
	cur_spell_rating /= 20;

	/* Is this the best spell yet? */
	if (cur_spell_rating > best_spell_rating) {
	    best_spell_rating = cur_spell_rating;
	    best_spell = i;
	    is_best_harass = is_harass;
	}

    }

    /* See if we've beaten the shapeshift rating */
    if (best_spell_rating < shape_rate) {

	if (p_ptr->wizard)
	    msg("Spell rating: %i.", shape_rate);

	/* Shapeshift is best */
	return (RSF_SHAPECHANGE);
    }

    if (p_ptr->wizard)
	msg("Spell rating: %i.", best_spell_rating);

    /* If we used a harassment spell, lower the bias to use them early */
    if (is_best_harass && m_ptr->harass)
	m_ptr->harass--;

    /* If we're choosing a shapeshift, return the rating */
    if (shape_rate == -1)
	return (best_spell_rating);

    /* Otherwise return best spell */
    else
	return (best_spell);
}



/**
 * Can the monster exist in this grid?
 *
 * Because this function is designed for use in monster placement and
 * generation as well as movement, it cannot accept monster-specific
 * data, but must rely solely on racial information.
 */
bool cave_exist_mon(monster_race * r_ptr, int y, int x, bool occupied_ok)
{
    int feat;
    feature_type *f_ptr;

    /* Check Bounds */
    if (!in_bounds_fully(y, x))
	return (FALSE);

    /* Check location */
    feat = cave_feat[y][x];
    f_ptr = &f_info[feat];

    /* The grid is already occupied. */
    if (cave_m_idx[y][x] != 0) 
    {
	if (!occupied_ok)
	    return (FALSE);
    }

    /* Glyphs -- must break first */
    if (cave_trap_specific(y, x, RUNE_PROTECT))
	return (FALSE);
	
    /*** Check passability of various features. ***/

    /* Feature is passable */
    if (tf_has(f_ptr->flags, TF_PASSABLE)) 
    {
	/* Floor -- safe for everything */
	if (tf_has(f_ptr->flags, TF_FLOOR))
	    return (TRUE);

	/* Earthbound demons, firebreathers, and red elementals cannot handle
	 * water */
	if (tf_has(f_ptr->flags, TF_WATERY))
	{
	    if (rf_has(r_ptr->flags, RF_FLYING))
		return (TRUE);
	    
	    if ((rsf_has(r_ptr->flags, RSF_BRTH_FIRE))
		|| (strchr("uU", r_ptr->d_char))
		|| ((strchr("E", r_ptr->d_char))
		    && ((r_ptr->d_attr == TERM_RED)
			|| (r_ptr->d_attr == TERM_L_RED)))) {
		return (FALSE);
	    }
	    
	    else
		return (TRUE);
	}
	
	
	/* Only fiery or strong flying creatures can handle lava */
	if (tf_has(f_ptr->flags, TF_FIERY))
	{
	    if (rf_has(r_ptr->flags, RF_IM_FIRE))
		return (TRUE);
	    else if (rf_has(r_ptr->flags, RF_FLYING)) 
	    {
		/* Get HPs */
		int hp = (rf_has(r_ptr->flags, RF_FORCE_MAXHP)
			  ? (r_ptr->hdice * r_ptr->hside) : 
			  (r_ptr->hdice * (r_ptr->hside + 1) / 2));
		
		/* Only strong monsters */
		if (hp > 49)
		    return (TRUE);
	    }
	    
	    return (FALSE);
	}

	/* Only flying monsters can, well, fly */
	if (tf_has(f_ptr->flags, TF_FALL)) 
	{
	    if (rf_has(r_ptr->flags, RF_FLYING))
		return (TRUE);
	    else
		return (FALSE);
	}

	/* Anything else that's not a wall we assume to be legal. */
	else
	    return (TRUE);
    }


    /* Feature is a wall */
    else
    {
	/* Permanent walls are never OK */
	if (tf_has(f_ptr->flags, TF_PERMANENT) && tf_has(f_ptr->flags, TF_WALL))
	    return (FALSE);
	
	/* Otherwise, test by the monster's ability to live in walls. */
	if ((rf_has(r_ptr->flags, RF_PASS_WALL))
	    || (rf_has(r_ptr->flags, RF_KILL_WALL)))
	    return (TRUE);
	
	else
	    return (FALSE);
    }
    
    /* Catch weirdness */
    return (FALSE);
}


/**
 * Can the monster enter this grid?  How easy is it for them to do so?
 *
 * The code that uses this function sometimes assumes that it will never 
 * return a value greater than 100.
 *
 * The usage of exp to determine whether one monster can kill another is 
 * a kludge.  Maybe use HPs, plus a big bonus for acidic monsters 
 * against monsters that don't like acid.
 *
 * The usage of exp to determine whether one monster can push past 
 * another is also a tad iffy, but ensures that black orcs can always 
 * push past other black orcs.
 */
static int cave_passable_mon(monster_type *m_ptr, int y, int x, bool *bash)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    /* Assume nothing in the grid other than the terrain hinders movement */
    int move_chance = 100;

    int feat;
    feature_type *f_ptr;

    /* Check Bounds */
    if (!in_bounds(y, x))
	return (FALSE);

    /* Check location */
    feat = cave_feat[y][x];
    f_ptr = &f_info[feat];

    /* The grid is occupied by the player. */
    if (cave_m_idx[y][x] < 0) 
    {
	/* Monster has no melee blows - character's grid is off-limits. */
	if (rf_has(r_ptr->flags, RF_NEVER_BLOW))
	    return (0);
	
	/* Any monster with melee blows can attack the character. */
	else
	    move_chance = 100;
    }

    /* The grid is occupied by a monster. */
    else if (cave_m_idx[y][x] > 0) 
    {
	monster_type *n_ptr = &m_list[cave_m_idx[y][x]];
	monster_race *nr_ptr = &r_info[n_ptr->r_idx];

	/* Kill weaker monsters */
	if ((rf_has(r_ptr->flags, RF_KILL_BODY))
	    && (!(rf_has(r_ptr->flags, RF_UNIQUE)))
	    && (r_ptr->mexp > nr_ptr->mexp)) 
	{
	    move_chance = 100;
	}

	/* Push past weaker or similar monsters */
	else if ((rf_has(r_ptr->flags, RF_MOVE_BODY))
		 && (r_ptr->mexp >= nr_ptr->mexp)) 
	{
	    /* It's easier to push past weaker monsters */
	    if (r_ptr->mexp == nr_ptr->mexp)
		move_chance = 50;
	    else
		move_chance = 80;
	}

	/* Cannot do anything to clear away the other monster */
	else
	    return (0);
    }

    /* Paranoia -- move_chance must not be more than 100 */
    if (move_chance > 100)
	move_chance = 100;

    /* Glyphs */
    if (cave_trap_specific(y, x, RUNE_PROTECT)) 
    {
	/* Glyphs are hard to break */
	return (MIN(100 * r_ptr->level / BREAK_GLYPH, move_chance));
    }


    /*** Check passability of various features. ***/

    /* Feature techincally passable */
    if (tf_has(f_ptr->flags, TF_PASSABLE)) 
    {
	if (tf_has(f_ptr->flags, TF_WATERY)) 
	{
	    /* Flying monsters can always cross water */
	    if (rf_has(r_ptr->flags, RF_FLYING))
		return (move_chance);
	    
	    /* Earthbound demons and firebreathers cannot cross water */
	    if (rsf_has(r_ptr->flags, RSF_BRTH_FIRE))
		return (0);
	    if (strchr("uU", r_ptr->d_char))
		return (0);
	    
	    /* "Red" elementals cannot cross water */
	    if (strchr("E", r_ptr->d_char)
		&& ((r_ptr->d_attr == TERM_RED)
		    || (r_ptr->d_attr == TERM_L_RED))) {
		return (0);
	    }

	    /* Humanoid-type monsters are slowed a little */
	    if (strchr("ghkoOpPTXy", r_ptr->d_char))
		return (MIN(75, move_chance));
	    
	    /* Undead are slowed a little more (a la Ringwraiths) */
	    if (strchr("GLMVWz", r_ptr->d_char))
		return (MIN(50, move_chance));
	    
	    /* Everything else has no problems crossing water */
	    return (move_chance);
	    
	}


	/* Lava */
	if (tf_has(f_ptr->flags, TF_FIERY)) 
	{
	    /* Only fiery or strong flying creatures will cross lava */
	    if (rf_has(r_ptr->flags, RF_IM_FIRE))
		return (move_chance);
	    
	    if ((rf_has(r_ptr->flags, RF_FLYING)) && (m_ptr->hp > 49))
		return (move_chance);
	    
	    return (0);
	}

	/* Rubble */
	if (tf_has(f_ptr->flags, TF_ROCK)) 
	{
	    /* Some monsters move easily through rubble */
	    if ((rf_has(r_ptr->flags, RF_PASS_WALL))
		|| (rf_has(r_ptr->flags, RF_KILL_WALL))) 
	    {
		return (move_chance);
	    }

	    /* For most monsters, rubble takes more time to cross. */
	    else
		return MIN(50, move_chance);
	}

	/* Trees */
	if (tf_has(f_ptr->flags, TF_TREE)) 
	{
	    /* Some monsters can pass right through trees */
	    if (rf_has(r_ptr->flags, RF_PASS_WALL))
		return (move_chance);

	    /* Some monsters can fly over trees, or know them well */
	    if ((rf_has(r_ptr->flags, RF_FLYING))
		|| (rf_has(r_ptr->flags, RF_ANIMAL))) 
	    {
		return (move_chance);
	    }

	    else 
	    {
		/* For many monsters, trees take more time to cross. */
		return (MIN(50, move_chance));
	    }
	}

	/* Void */
	if (tf_has(f_ptr->flags, TF_FALL))
	{
	    /* Have to be able to fly */
	    if (rf_has(r_ptr->flags, RF_FLYING))
		return (move_chance);
	    else
		return (0);
	}

	/* Anything else that's not a wall we assume to be passable. */
	return (move_chance);
    }


    /* Feature is tecnically impassable as it stands */
    else 
    {
	/* Can the monster move easily through walls? */
	bool move_wall = FALSE;
	if ((rf_has(r_ptr->flags, RF_PASS_WALL))
	    || (rf_has(r_ptr->flags, RF_KILL_WALL))) {
	    move_wall = TRUE;
	}


	/* Walls */	
	if (tf_has(f_ptr->flags, TF_WALL) && !tf_has(f_ptr->flags, TF_DOOR_ANY))
	{
	    /* Permanent walls are never passable */
	    if (tf_has(f_ptr->flags, TF_PERMANENT))
		return (0);

	    /* Standard dungeon granite and seams */
	    {
		/* Impassible except for monsters that move through walls */
		if (move_wall)
		    return (move_chance);
		else
		    return (0);
	    }
	}

	/* Doors */
	if (tf_has(f_ptr->flags, TF_DOOR_ANY))
	{
	    int unlock_chance = 0;
	    int bash_chance = 0;

	    /* Monster can open doors */
	    if (rf_has(r_ptr->flags, RF_OPEN_DOOR)) 
	    {
		/* 
		 * Locked doors (not jammed).  Monsters know how hard 
		 * doors in their neighborhood are to unlock.
		 */
		if (tf_has(f_ptr->flags, TF_DOOR_LOCKED))
		{
		    int lock_power, ability;

		    /* Door power (from 35 to 245) */
		    lock_power = 35 * (f_ptr->locked);

		    /* Calculate unlocking ability (usu. 11 to 200) */
		    ability = r_ptr->level + 10;
		    if (rf_has(r_ptr->flags, RF_SMART))
			ability *= 2;
		    if (strchr("ph", r_ptr->d_char))
			ability = 3 * ability / 2;

		    /* 
		     * Chance varies from 5% to over 100%.  XXX XXX -- 
		     * we ignore the fact that it takes extra time to 
		     * open the door and walk into the entranceway.
		     */
		    unlock_chance = (MAX(5, (100 * ability / lock_power)));
		}

		/* Closed doors and secret doors */
		else if (!tf_has(f_ptr->flags, TF_DOOR_JAMMED)) 
		{
		    /* 
		     * Note:  This section will have to be rewritten if 
		     * secret doors can be jammed or locked as well.
		     */


		    /* 
		     * It usually takes two turns to open a door 
		     * and move into the doorway.
		     */
		    return (MIN(50, move_chance));
		}

	    }

	    /* Monster can bash doors */
	    if (rf_has(r_ptr->flags, RF_BASH_DOOR)) 
	    {
		int door_power, bashing_power;

		/* Door power (from 60 to 420) */
		door_power = 60 + 60 * (f_ptr->jammed);

		/* 
		 * Calculate bashing ability (usu. 21 to 300).  Note:  
		 * This formula assumes Oangband-style HPs.
		 */
		bashing_power = 20 + r_ptr->level + m_ptr->hp / 15;

		if ((rf_has(r_ptr->flags, RF_GIANT))
		    || (rf_has(r_ptr->flags, RF_TROLL)))
		    bashing_power = 3 * bashing_power / 2;

		/* 
		 * Chance varies from 2% to over 100%.  Note that 
		 * monsters "fall" into the entranceway in the same 
		 * turn that they bash the door down.
		 */
		bash_chance = (MAX(2, (100 * bashing_power / door_power)));
	    }

	    /* 
	     * A monster cannot both bash and unlock a door in the same 
	     * turn.  It needs to pick one of the two methods to use.
	     */
	    if (unlock_chance > bash_chance)
		*bash = FALSE;
	    else
		*bash = TRUE;

	    return MIN(move_chance, (MAX(unlock_chance, bash_chance)));
	}

	/* Any wall grid that isn't explicitly made passible is impassible. */
	return (0);
    }

    /* Catch weirdness */
    return (0);
}


/**
 * Helper function for monsters that want to advance toward the character.
 * Assumes that the monster isn't frightened, and is not in LOS of the 
 * character.
 *
 * Ghosts and rock-eaters do not use flow information, because they 
 * can - in general - move directly towards the character.  We could make 
 * them look for a grid at their preferred range, but the character 
 * would then be able to avoid them better (it might also be a little 
 * hard on those poor warriors...).
 *
 * Other monsters will use target information, then their ears, then their
 * noses (if they can), and advance blindly if nothing else works.
 * 
 * When flowing, monsters prefer non-diagonal directions.
 *
 * XXX - At present, this function does not handle difficult terrain 
 * intelligently.  Monsters using flow may bang right into a door that 
 * they can't handle.  Fixing this may require code to set monster 
 * paths.
 */
static void get_move_advance(monster_type * m_ptr, int *ty, int *tx)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int i, y, x, y1, x1;

    int lowest_cost = 250;

    bool use_psound = FALSE;
    bool use_scent = FALSE;

    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    /* Monster can go through rocks - head straight for target */
    if ((rf_has(r_ptr->flags, RF_PASS_WALL))
	|| (rf_has(r_ptr->flags, RF_KILL_WALL))) {
	/* Player is the target */
	if (m_ptr->hostile < 0) {
	    *ty = py;
	    *tx = px;
	}
	/* Another monster is the target */
	else if (m_ptr->hostile > 0) {
	    *ty = m_list[m_ptr->hostile].fy;
	    *tx = m_list[m_ptr->hostile].fx;
	}
	return;
    }

    /* Monster location */
    y1 = m_ptr->fy;
    x1 = m_ptr->fx;

    /* Use target information if available */
    if ((m_ptr->ty) && (m_ptr->tx)) {
	*ty = m_ptr->ty;
	*tx = m_ptr->tx;
	return;
    }

    /* If we can hear noises, advance towards them */
    if (cave_cost[y1][x1]) {
	use_psound = TRUE;
    }

    /* Otherwise, try to follow a scent trail */
    else if (monster_can_smell(m_ptr)) {
	use_scent = TRUE;
    }

    /* Otherwise, advance blindly */
    if ((!use_psound) && (!use_scent)) {
	/* Player is the target */
	if (m_ptr->hostile < 0) {
	    *ty = py;
	    *tx = px;
	}
	/* Another monster is the target */
	else if (m_ptr->hostile > 0) {
	    *ty = m_list[m_ptr->hostile].fy;
	    *tx = m_list[m_ptr->hostile].fx;
	}
	return;
    }

    /* Using flow information.  Check nearby grids, diagonals first. */
    for (i = 7; i >= 0; i--) {
	/* Get the location */
	y = y1 + ddy_ddd[i];
	x = x1 + ddx_ddd[i];

	/* Check Bounds */
	if (!in_bounds(y, x))
	    continue;

	/* We're following a scent trail */
	if (use_scent) {
	    int age = get_scent(y, x);
	    if (age == -1)
		continue;

	    /* Accept younger scent */
	    if (lowest_cost < age)
		continue;
	    lowest_cost = age;
	}

	/* We're using sound */
	else {
	    int cost = cave_cost[y][x];

	    /* Accept louder sounds */
	    if ((cost == 0) || (lowest_cost < cost))
		continue;
	    lowest_cost = cost;
	}

	/* Save the location */
	*ty = y;
	*tx = x;
    }
}


/**
 * "Do not be seen."
 *
 * Monsters in LOS that want to retreat are primarily interested in 
 * finding a nearby place that the character can't see into.
 * Search for such a place with the lowest cost to get to up to 15 
 * grids away.
 *
 * Look outward from the monster's current position in a square-
 * shaped search pattern.  Calculate the approximate cost in monster 
 * turns to get to each passable grid, using a crude route finder.  Penal-
 * ize grids close to or approaching the character.  Ignore hiding places
 * with no safe exit.  Once a passable grid is found that the character
 * can't see, the code will continue to search a little while longer,
 * depending on how pricey the first option seemed to be.
 *
 * If the search is successful, the monster will target that grid, 
 * and (barring various special cases) run for it until it gets there.
 *
 * We use a limited waypoint system (see function "get_route_to_target()"
 * to reduce the likelihood that monsters will get stuck at a wall between
 * them and their target (which is kinda embarrassing...).
 *
 * This function does not yield perfect results; it is known to fail 
 * in cases where the previous code worked just fine.  The reason why 
 * it is used is because its failures are less common and (usually) 
 * less embarrassing than was the case before.  In particular, it makes 
 * monsters great at not being seen.
 *
 * This function is fairly expensive.  Call it only when necessary.
 */
static bool find_safety(monster_type * m_ptr, int *ty, int *tx)
{
    int i, j, d;

    /* Scanning range for hiding place search. */
    byte scan_range = 15;

    int y, x, yy, xx;

    int countdown = scan_range;

    int least_cost = 100;
    int least_cost_y = 0;
    int least_cost_x = 0;
    int chance, cost, parent_cost;
    bool dummy;

    /* Factors for converting table to actual dungeon grids */
    int conv_y, conv_x;

    /* 
     * Allocate and initialize a table of movement costs.
     * Both axis must be (2 * scan_range + 1).
     */
    byte safe_cost[31][31];

    for (i = 0; i < 31; i++) {
	for (j = 0; j < 31; j++) {
	    safe_cost[i][j] = 0;
	}
    }

    conv_y = scan_range - m_ptr->fy;
    conv_x = scan_range - m_ptr->fx;

    /* Mark the origin */
    safe_cost[scan_range][scan_range] = 1;

    /* If the character's grid is in range, mark it as being off-limits */
    if ((ABS(m_ptr->fy - p_ptr->py) <= scan_range)
	&& (ABS(m_ptr->fx - p_ptr->px) <= scan_range)) {
	safe_cost[p_ptr->py + conv_y][p_ptr->px + conv_x] = 100;
    }

    /* Work outward from the monster's current position */
    for (d = 0; d < scan_range; d++) {
	for (y = scan_range - d; y <= scan_range + d; y++) {
	    for (x = scan_range - d; x <= scan_range + d;) {
		int x_tmp;

		/* 
		 * Scan all grids of top and bottom rows, just 
		 * outline other rows.
		 */
		if ((y != scan_range - d) && (y != scan_range + d)) {
		    if (x == scan_range + d)
			x_tmp = 999;
		    else
			x_tmp = scan_range + d;
		} else
		    x_tmp = x + 1;

		/* Grid and adjacent grids must be legal */
		if (!in_bounds_fully(y - conv_y, x - conv_x)) {
		    x = x_tmp;
		    continue;
		}

		/* Grid is inaccessable (or at least very difficult to enter) */
		if ((safe_cost[y][x] == 0) || (safe_cost[y][x] >= 100)) {
		    x = x_tmp;
		    continue;
		}

		/* Get the accumulated cost to enter this grid */
		parent_cost = safe_cost[y][x];

		/* Scan all adjacent grids */
		for (i = 0; i < 8; i++) {
		    yy = y + ddy_ddd[i];
		    xx = x + ddx_ddd[i];

		    /* check bounds */
		    if ((yy < 0) || (yy > 30) || (xx < 0) || (xx > 30))
			continue;

		    /* 
		     * Handle grids with empty cost and passable grids
		     * with costs we have a chance of beating.
		     */
		    if ((safe_cost[yy][xx] == 0)
			|| ((safe_cost[yy][xx] > parent_cost + 1)
			    && (safe_cost[yy][xx] < 100))) {
			/* Get the cost to enter this grid */
			chance =
			    cave_passable_mon(m_ptr, yy - conv_y, xx - conv_x,
					      &dummy);

			/* Impassable */
			if (!chance) {
			    /* Cannot enter this grid */
			    safe_cost[yy][xx] = 100;
			    continue;
			}

			/* Calculate approximate cost (in monster turns) */
			cost = 100 / chance;

			/* Next to character */
			if (distance
			    (yy - conv_y, xx - conv_x, p_ptr->py,
			     p_ptr->px) <= 1) {
			    /* Don't want to maneuver next to the character */
			    cost += 3;
			}

			/* Mark this grid with a cost value */
			safe_cost[yy][xx] = parent_cost + cost;

			/* Character can't see this grid */
			if (!player_can_see_bold(yy - conv_y, xx - conv_x)) {
			    int this_cost = safe_cost[yy][xx];

			    /* Penalize grids that approach character */
			    if (ABS(p_ptr->py - (yy - conv_y)) <
				ABS(m_ptr->fy - (yy - conv_y))) {
				this_cost *= 2;
			    }
			    if (ABS(p_ptr->px - (xx - conv_x)) <
				ABS(m_ptr->fx - (xx - conv_x))) {
				this_cost *= 2;
			    }

			    /* Accept lower-cost, sometimes accept same-cost
			     * options */
			    if ((least_cost > this_cost)
				|| (least_cost == this_cost
				    && randint0(2) == 0)) {
				bool has_escape = FALSE;

				/* Scan all adjacent grids for escape routes */
				for (j = 0; j < 8; j++) {
				    /* Calculate real adjacent grids */
				    int yyy = yy - conv_y + ddy_ddd[i];
				    int xxx = xx - conv_x + ddx_ddd[i];

				    /* Check bounds */
				    if (!in_bounds(yyy, xxx))
					continue;

				    /* Look for any passable grid that isn't in 
				     * LOS */
				    if ((!player_can_see_bold(yyy, xxx))
					&&
					(cave_passable_mon
					 (m_ptr, yyy, xxx, &dummy))) {
					/* Not a one-grid cul-de-sac */
					has_escape = TRUE;
					break;
				    }
				}

				/* Ignore cul-de-sacs */
				if (has_escape == FALSE)
				    continue;

				least_cost = this_cost;
				least_cost_y = yy;
				least_cost_x = xx;

				/* 
				 * Look hard for alternative
				 * hiding places if this one
				 * seems pricey.
				 */
				countdown = 1 + least_cost - d;
			    }
			}
		    }
		}

		/* Adjust x as instructed */
		x = x_tmp;
	    }
	}

	/* 
	 * We found a good place a while ago, and haven't done better
	 * since, so we're probably done.
	 */
	if (countdown-- == 0)
	    break;
    }

    /* We found a place that can be reached in reasonable time */
    if (least_cost < 50) {
	/* Convert to actual dungeon grid. */
	y = least_cost_y - conv_y;
	x = least_cost_x - conv_x;

	/* Move towards the hiding place */
	*ty = y;
	*tx = x;

	/* Target the hiding place */
	m_ptr->ty = y;
	m_ptr->tx = x;

	return (TRUE);
    }


    /* No good place found */
    return (FALSE);
}


/**
 * Helper function for monsters that want to retreat from the character.
 * Used for any monster that is terrified, frightened, is looking for a 
 * temporary hiding spot, or just wants to open up some space between it 
 * and the character.
 *
 * If the monster is well away from danger, let it relax.
 * If the monster's current target is not in LOS, use it (+).
 * If the monster is not in LOS, and cannot pass through walls, try to 
 * use flow (noise) information.
 * If the monster is in LOS, even if it can pass through walls, 
 * search for a hiding place (helper function "find_safety()"):
 * search for a hiding place (helper function "find_safety()").
 * If no hiding place is found, and there seems no way out, go down
 * fighting.
 *
 * If none of the above solves the problem, run away blindly.
 *
 * (+) There is one exception to the automatic usage of a target.  If the 
 * target is only out of LOS because of "knight's move" rules (distance 
 * along one axis is 2, and along the other, 1), then the monster will try 
 * to find another adjacent grid that is out of sight.  What all this boils 
 * down to is that monsters can now run around corners properly!
 *
 * Return TRUE if the monster did actually want to do anything.
 */
static bool get_move_retreat(monster_type * m_ptr, int *ty, int *tx)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    int i;
    int y, x;

    bool done = FALSE;
    bool dummy;


    /* If the monster is well away from danger, let it relax. */
    if (m_ptr->cdis >= FLEE_RANGE) {
	return (FALSE);
    }

    /* Monster has a target */
    if ((m_ptr->ty) && (m_ptr->tx)) {
	/* It's out of LOS; keep using it, except in "knight's move" cases */
	if (!player_has_los_bold(m_ptr->ty, m_ptr->tx)) {
	    /* Get axis distance from character to current target */
	    int dist_y = ABS(p_ptr->py - m_ptr->ty);
	    int dist_x = ABS(p_ptr->px - m_ptr->tx);

	    /* It's only out of LOS because of "knight's move" rules */
	    if (((dist_y == 2) && (dist_x == 1))
		|| ((dist_y == 1) && (dist_x == 2))) {
		/* 
		 * If there is another grid adjacent to the monster that 
		 * the character cannot see into, and it isn't any harder 
		 * to enter, use it instead.  Prefer diagonals.
		 */
		for (i = 7; i >= 0; i--) {
		    y = m_ptr->fy + ddy_ddd[i];
		    x = m_ptr->fx + ddx_ddd[i];

		    /* Check Bounds */
		    if (!in_bounds(y, x))
			continue;

		    if (player_has_los_bold(y, x))
			continue;

		    if ((y == m_ptr->ty) && (x == m_ptr->tx))
			continue;

		    if (cave_passable_mon(m_ptr, m_ptr->ty, m_ptr->tx, &dummy) >
			cave_passable_mon(m_ptr, y, x, &dummy))
			continue;

		    m_ptr->ty = y;
		    m_ptr->tx = x;
		    break;
		}
	    }

	    /* Move towards the target */
	    *ty = m_ptr->ty;
	    *tx = m_ptr->tx;
	    return (TRUE);
	}

	/* It's in LOS; cancel it. */
	else {
	    m_ptr->ty = 0;
	    m_ptr->tx = 0;
	}
    }

    /* The monster is not in LOS, but thinks it's still too close. */
    if (!player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
	/* Monster cannot pass through walls */
	if (!((rf_has(r_ptr->flags, RF_PASS_WALL))
	      || (rf_has(r_ptr->flags, RF_KILL_WALL)))) {
	    /* Run away from noise */
	    if (cave_cost[m_ptr->fy][m_ptr->fx]) {
		int start_cost = cave_cost[m_ptr->fy][m_ptr->fx];

		/* Look at adjacent grids, diagonals first */
		for (i = 7; i >= 0; i--) {
		    y = m_ptr->fy + ddy_ddd[i];
		    x = m_ptr->fx + ddx_ddd[i];

		    /* Check Bounds */
		    if (!in_bounds(y, x))
			continue;

		    /* Accept the first non-visible grid with a higher cost */
		    if (cave_cost[y][x] > start_cost) {
			if (!player_has_los_bold(y, x)) {
			    *ty = y;
			    *tx = x;
			    done = TRUE;
			    break;
			}
		    }
		}

		/* Return if successful */
		if (done)
		    return (TRUE);
	    }
	}

	/* No flow info, or don't need it -- see bottom of function */
    }

    /* The monster is in line of sight. */
    else {
	int prev_cost = cave_cost[m_ptr->fy][m_ptr->fx];
	int start = randint0(8);

	/* Look for adjacent hiding places */
	for (i = start; i < 8 + start; i++) {
	    y = m_ptr->fy + ddy_ddd[i % 8];
	    x = m_ptr->fx + ddx_ddd[i % 8];

	    /* Check Bounds */
	    if (!in_bounds(y, x))
		continue;

	    /* No grids in LOS */
	    if (player_has_los_bold(y, x))
		continue;

	    /* Grid must be pretty easy to enter */
	    if (cave_passable_mon(m_ptr, y, x, &dummy) < 50)
		continue;

	    /* Accept any grid that doesn't have a lower flow (noise) cost. */
	    if (cave_cost[y][x] >= prev_cost) {
		*ty = y;
		*tx = x;
		prev_cost = cave_cost[y][x];

		/* Success */
		return (TRUE);
	    }
	}

	/* Find a nearby grid not in LOS of the character. */
	if (find_safety(m_ptr, ty, tx) == TRUE)
	    return (TRUE);

	/* 
	 * No safe place found.  If monster is in LOS and close,
	 * it will turn to fight.
	 */
	if ((player_has_los_bold(m_ptr->fy, m_ptr->fx))
	    && (m_ptr->cdis < TURN_RANGE)) {
	    byte fear = m_ptr->monfear;

	    /* Turn and fight */
	    m_ptr->monfear = 0;

	    /* Recalculate combat range (later) */
	    m_ptr->min_range = 0;

	    /* Visible */
	    if ((m_ptr->ml) && fear) {
		char m_name[80];

		/* Get the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		/* Dump a message */
		msg("%s turns to fight!", m_name);
	    }

	    /* Charge! */
	    *ty = p_ptr->py;
	    *tx = p_ptr->px;
	    return (TRUE);
	}
    }

    /* Move directly away from character. */
    *ty = -(p_ptr->py - m_ptr->fy);
    *tx = -(p_ptr->px - m_ptr->fx);

    /* We want to run away */
    return (TRUE);
}


/**
 * Choose the probable best direction for a monster to move in.  This 
 * is done by choosing a target grid and then finding the direction that 
 * best approaches it.
 *
 * Monsters that cannot move always attack if possible.
 * Frightened monsters retreat.
 * Monsters adjacent to the character attack if possible.
 *
 * Monster packs lure the character into open ground and then leap 
 * upon him.  Monster groups try to surround the character.  -KJ-
 *
 * Monsters not in LOS always advance (this avoids player frustration).  
 * Monsters in LOS will advance to the character, up to their standard
 * combat range, to a grid that allows them to target the character, or
 * just stay still if they are happy where they are, depending on the
 * tactical situation and the monster's preferred and minimum combat
 * ranges.
 * NOTE:  Here is an area that would benefit from more development work.
 *
 * Non-trivial movement calculations are performed by the helper 
 * functions "get_move_advance" and "get_move_retreat", which keeps 
 * this function relatively simple.
 *
 * The variable "must_use_target" is used for monsters that can't 
 * currently perceive the character, but have a known target to move 
 * towards.  With a bit more work, this will lead to semi-realistic
 * "hunting" behavior.
 *
 * Return FALSE if monster doesn't want to move or can't.
 */
static bool get_move(monster_type * m_ptr, int *ty, int *tx, bool * fear,
		     bool must_use_target)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_lore *l_ptr = &l_list[m_ptr->r_idx];

    int i, start;
    int y, x, targ_y = DUNGEON_HGT / 2, targ_x = DUNGEON_WID / 2;

    int py = p_ptr->py;
    int px = p_ptr->px;

    /* Assume no movement */
    *ty = m_ptr->fy;
    *tx = m_ptr->fx;


    /* 
     * Monster is only allowed to use targetting information.
     */
    if (must_use_target) {
	*ty = m_ptr->ty;
	*tx = m_ptr->tx;
	return (TRUE);
    }

    /* Who is the monster after? */
    if (m_ptr->hostile < 0) {
	targ_y = py;
	targ_x = px;
    } else if (m_ptr->hostile > 0) {
	targ_y = m_list[m_ptr->hostile].fy;
	targ_x = m_list[m_ptr->hostile].fx;
    }

    /* 
     * Monsters that cannot move will attack the character if he is 
     * adjacent.
     */
    if (rf_has(r_ptr->flags, RF_NEVER_MOVE)) {
	/* Hack -- memorize lack of moves after a while. */
	if (!(rf_has(l_ptr->flags, RF_NEVER_MOVE))) {
	    if ((m_ptr->ml) && (randint1(20) == 1))
		rf_on(l_ptr->flags, RF_NEVER_MOVE);
	}

	/* Is character in range? */
	if (m_ptr->cdis <= 1) {
	    /* Monster can't melee either (pathetic little creature) */
	    if (rf_has(r_ptr->flags, RF_NEVER_BLOW)) {
		/* Hack -- memorize lack of attacks after a while */
		if (!(rf_has(l_ptr->flags, RF_NEVER_BLOW))) {
		    if ((m_ptr->ml) && (randint1(10) == 1))
			rf_on(l_ptr->flags, RF_NEVER_BLOW);
		}
		return (FALSE);
	    }

	    /* Kill. */
	    *fear = FALSE;
	    *ty = targ_y;
	    *tx = targ_x;
	    return (TRUE);
	}

	/* If we can't hit anything, do not move */
	else {
	    return (FALSE);
	}
    }


  /*** Handle monster fear -- only for monsters that can move ***/

    /* Is the monster scared? */
    if ((m_ptr->min_range == FLEE_RANGE) || (m_ptr->monfear))
	*fear = TRUE;
    else
	*fear = FALSE;

    /* Monster is frightened or terrified. */
    if (*fear) {
	/* The character is too close to avoid, and faster than we are */
	if ((!m_ptr->monfear) && (m_ptr->cdis < TURN_RANGE)
	    && (p_ptr->state.pspeed > m_ptr->mspeed)) {
	    /* Recalculate range */
	    find_range(m_ptr);

	    /* Note changes in monster attitude */
	    if (m_ptr->min_range < m_ptr->cdis) {
		/* Cancel fear */
		*fear = FALSE;

		/* No message -- too annoying */

		/* Charge! */
		*ty = targ_y;
		*tx = targ_x;

		return (TRUE);
	    }
	}

	/* The monster is within 25 grids of the character */
	else if (m_ptr->cdis < FLEE_RANGE) {
	    /* Find and move towards a hidey-hole */
	    get_move_retreat(m_ptr, ty, tx);
	    return (TRUE);
	}

	/* Monster is well away from danger */
	else {
	    /* No need to move */
	    return (FALSE);
	}
    }


    /* If the character is adjacent, attack or back off.  */
    if ((!*fear) && (m_ptr->cdis <= 1)) {
	/* Monsters that cannot attack back off. */
	if (rf_has(r_ptr->flags, RF_NEVER_BLOW)) {
	    /* Hack -- memorize lack of attacks after a while */
	    if (!(rf_has(l_ptr->flags, RF_NEVER_BLOW))) {
		if ((m_ptr->ml) && (randint1(10) == 1))
		    rf_on(l_ptr->flags, RF_NEVER_BLOW);
	    }

	    /* Back away */
	    *fear = TRUE;
	}

	else {
	    /* All other monsters attack. */
	    *ty = targ_y;
	    *tx = targ_x;
	    return (TRUE);
	}
    }


    /* Uninjured animals in packs try to lure the character into the open. */
    if ((!*fear) && (rf_has(r_ptr->flags, RF_FRIENDS))
	&& (rf_has(r_ptr->flags, RF_ANIMAL)) && (m_ptr->hp == m_ptr->maxhp)
	&& (!((rf_has(r_ptr->flags, RF_PASS_WALL))
	      || (rf_has(r_ptr->flags, RF_KILL_WALL))))) {
	/* Animal has to be willing to melee */
	if (m_ptr->min_range == 1) {
	    /* 
	     * If character vulnerability has not yet been 
	     * calculated this turn, calculate it now.
	     */
	    if (p_ptr->vulnerability == 0) 
	    {
		feature_type *f_ptr;

		/* Count passable grids next to target */
		for (i = 0; i < 8; i++) {
		    y = targ_y + ddy_ddd[i];
		    x = targ_x + ddx_ddd[i];

		    /* Check Bounds */
		    if (!in_bounds(y, x))
			continue;

		    /* Count passable grids */
		    f_ptr = &f_info[cave_feat[y][x]];
		    if (tf_has(f_ptr->flags, TF_PASSABLE))
			p_ptr->vulnerability++;
		}
		

		/* Dead ends are not all that safe */
		if (p_ptr->vulnerability == 1)
		    p_ptr->vulnerability++;

		/* 
		 * Take character weakness into account (this 
		 * always adds at least one)
		 */
		if (p_ptr->chp <= 3)
		    p_ptr->vulnerability = 100;
		else
		    p_ptr->vulnerability += (p_ptr->mhp / p_ptr->chp);
	    }

	    /* Character is insufficiently vulnerable */
	    if (p_ptr->vulnerability <= 4) {
		/* If we're in sight, find a hiding place */
		if (cave_has(cave_info[m_ptr->fy][m_ptr->fx], SQUARE_SEEN)) {
		    /* Find a safe spot to lurk in */
		    if (get_move_retreat(m_ptr, ty, tx)) {
			*fear = TRUE;
		    } else {
			/* No safe spot -- charge */
			*ty = targ_y;
			*tx = targ_x;
		    }
		}

		/* If we're not viewable, we advance cautiously */
		else {
		    /* Advance, ... */
		    get_move_advance(m_ptr, ty, tx);

		    /* ... but make sure we stay hidden. */
		    *fear = TRUE;
		}

		/* done */
		return (TRUE);
	    }
	}
    }

    /* Monster groups try to surround the character. */
    if ((!*fear) && (rf_has(r_ptr->flags, RF_FRIENDS)) && (m_ptr->cdis <= 3)) {
	start = randint0(8);

	/* Find a random empty square next to the player to head for */
	for (i = start; i < 8 + start; i++) {
	    /* Pick squares near player */
	    y = targ_y + ddy_ddd[i % 8];
	    x = targ_x + ddx_ddd[i % 8];

	    /* Check Bounds */
	    if (!in_bounds(y, x))
		continue;

	    /* Ignore occupied grids */
	    if (cave_m_idx[y][x] != 0)
		continue;

	    /* Ignore grids that monster can't enter immediately */
	    if (!cave_exist_mon(r_ptr, y, x, FALSE))
		continue;

	    /* Accept */
	    *ty = y;
	    *tx = x;
	    return (TRUE);
	}
    }

    /* Monster can go through rocks - head straight for target */
    if ((!*fear)
	&& ((rf_has(r_ptr->flags, RF_PASS_WALL))
	    || (rf_has(r_ptr->flags, RF_KILL_WALL)))) {
	*ty = targ_y;
	*tx = targ_x;
	return (TRUE);
    }


    /* No special moves made -- use standard movement */

    /* Not frightened */
    if (!*fear) {
	/* 
	 * XXX XXX -- The monster cannot see the character.  Make it 
	 * advance, so the player can have fun ambushing it.
	 */
	if (!player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
	    /* Advance */
	    get_move_advance(m_ptr, ty, tx);
	}

	/* Monster can see the player (or monster target) */
	else if (m_ptr->hostile != 0) {
	    /* Always reset the monster's target */
	    m_ptr->ty = targ_y;
	    m_ptr->tx = targ_x;

	    /* Monsters too far away will advance. */
	    if (m_ptr->cdis > m_ptr->best_range) {
		*ty = targ_y;
		*tx = targ_x;
	    }

	    /* Monsters not too close will often advance */
	    else if ((m_ptr->cdis > m_ptr->min_range) && (randint0(2) == 0)) {
		*ty = targ_y;
		*tx = targ_x;
	    }

	    /* Monsters that can't target the character will advance. */
	    else if (projectable(m_ptr->fy, m_ptr->fx, targ_y, targ_x, 0)
		     == PROJECT_NO) {
		*ty = targ_y;
		*tx = targ_x;
	    }

	    /* Otherwise they will stay still or move randomly. */
	    else {
		/* 
		 * It would be odd if monsters that move randomly 
		 * were to stay still.
		 */
		if (rf_has(r_ptr->flags, RF_RAND_50)
		    || rf_has(r_ptr->flags, RF_RAND_25)) {
		    /* pick a random grid next to the monster */
		    int i = randint0(8);

		    *ty = m_ptr->fy + ddy_ddd[i];
		    *tx = m_ptr->fx + ddx_ddd[i];
		}

		/* Monsters could look for better terrain... */
	    }
	}
    }

    /* Monster is frightened */
    else {
	/* Back away -- try to be smart about it */
	get_move_retreat(m_ptr, ty, tx);
    }


    /* We do not want to move */
    if ((*ty == m_ptr->fy) && (*tx == m_ptr->fx))
	return (FALSE);

    /* We want to move */
    return (TRUE);
}




/**
 * A simple method to help fleeing monsters who are having trouble getting
 * to their target.  It's very limited, but works fairly well in the 
 * situations it is called upon to resolve.  XXX
 *
 * If this function claims success, ty and tx must be set to a grid 
 * adjacent to the monster.
 *
 * Return TRUE if this function actually did any good.
 */
static bool get_route_to_target(monster_type * m_ptr, int *ty, int *tx)
{
    int i, j;
    int y, x, yy, xx;
    int target_y, target_x, dist_y, dist_x;

    bool dummy;
    bool below = FALSE;
    bool right = FALSE;

    target_y = 0;
    target_x = 0;

    /* Is the target further away vertically or horizontally? */
    dist_y = ABS(m_ptr->ty - m_ptr->fy);
    dist_x = ABS(m_ptr->tx - m_ptr->fx);

    /* Target is further away vertically than horizontally */
    if (dist_y > dist_x) {
	/* Find out if the target is below the monster */
	if (m_ptr->ty - m_ptr->fy > 0)
	    below = TRUE;

	/* Search adjacent grids */
	for (i = 0; i < 8; i++) {
	    y = m_ptr->fy + ddy_ddd[i];
	    x = m_ptr->fx + ddx_ddd[i];

	    /* Check Bounds (fully) */
	    if (!in_bounds_fully(y, x))
		continue;

	    /* Grid is not passable */
	    if (!cave_passable_mon(m_ptr, y, x, &dummy))
		continue;

	    /* Grid will take me further away */
	    if (((below) && (y < m_ptr->fy)) || ((!below) && (y > m_ptr->fy))) {
		continue;
	    }

	    /* Grid will not take me closer or further */
	    else if (y == m_ptr->fy) {
		/* See if it leads to better things */
		for (j = 0; j < 8; j++) {
		    yy = y + ddy_ddd[j];
		    xx = x + ddx_ddd[j];

		    /* Grid does lead to better things */
		    if (((below) && (yy > m_ptr->fy))
			|| ((!below) && (yy < m_ptr->fy))) {
			/* But it is not passable */
			if (!cave_passable_mon(m_ptr, yy, xx, &dummy))
			    continue;

			/* Accept (original) grid, but don't immediately claim
			 * success */
			*ty = y;
			*tx = x;
		    }
		}
	    }

	    /* Grid will take me closer */
	    else {
		/* Don't look this gift horse in the mouth. */
		*ty = y;
		*tx = x;
		return (TRUE);
	    }
	}
    }

    /* Target is further away horizontally than vertically */
    else if (dist_x > dist_y) {
	/* Find out if the target is right of the monster */
	if (m_ptr->tx - m_ptr->fx > 0)
	    right = TRUE;

	/* Search adjacent grids */
	for (i = 0; i < 8; i++) {
	    y = m_ptr->fy + ddy_ddd[i];
	    x = m_ptr->fx + ddx_ddd[i];

	    /* Check Bounds (fully) */
	    if (!in_bounds_fully(y, x))
		continue;

	    /* Grid is not passable */
	    if (!cave_passable_mon(m_ptr, y, x, &dummy))
		continue;

	    /* Grid will take me further away */
	    if (((right) && (x < m_ptr->fx)) || ((!right) && (x > m_ptr->fx))) {
		continue;
	    }

	    /* Grid will not take me closer or further */
	    else if (x == m_ptr->fx) {
		/* See if it leads to better things */
		for (j = 0; j < 8; j++) {
		    yy = y + ddy_ddd[j];
		    xx = x + ddx_ddd[j];

		    /* Grid does lead to better things */
		    if (((right) && (xx > m_ptr->fx))
			|| ((!right) && (xx < m_ptr->fx))) {
			/* But it is not passable */
			if (!cave_passable_mon(m_ptr, yy, xx, &dummy))
			    continue;

			/* Accept (original) grid, but don't immediately claim
			 * success */
			target_y = y;
			target_x = x;
		    }
		}
	    }

	    /* Grid will take me closer */
	    else {
		/* Don't look this gift horse in the mouth. */
		*ty = y;
		*tx = x;
		return (TRUE);
	    }
	}
    }

    /* Target is the same distance away along both axes. */
    else {
	/* XXX XXX - code something later to fill this hole. */
	return (FALSE);
    }

    /* If we found a solution, claim success */
    if ((target_y) && (target_x)) {
	*ty = target_y;
	*tx = target_x;
	return (TRUE);
    }

    /* No luck */
    return (FALSE);
}


/**
 * Confused monsters bang into walls and doors, and wander into lava or 
 * water.  This function assumes that the monster does not belong in this 
 * grid, and therefore should suffer for trying to enter it.
 */
static void make_confused_move(monster_type * m_ptr, int y, int x)
{
    char m_name[80];

    int feat;
    feature_type *f_ptr;

    bool seen = FALSE;
    bool stun = FALSE;
    bool fear = FALSE;
    bool death = TRUE;

    /* Check Bounds (fully) */
    if (!in_bounds_fully(y, x))
	return;

    /* Check location */
    feat = cave_feat[y][x];
    f_ptr = &f_info[feat];

    /* Check visibility */
    if ((m_ptr->ml) && cave_has(cave_info[y][x], SQUARE_SEEN))
	seen = TRUE;


    /* Get the monster name/poss */
    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);


    /* Feature is a (known) door */
    if (tf_has(f_ptr->flags, TF_DOOR_CLOSED))
    {
	if (seen)
	    msg("%s bangs into a door.", m_name);
	stun = TRUE;
    }

    /* Rubble */
    else if (tf_has(f_ptr->flags, TF_ROCK))
    {
	if (seen)
	    msg("%s bumps into some rocks.", m_name);
	stun = TRUE;
    }
    
    /* Tree */
    else if (tf_has(f_ptr->flags, TF_TREE))
    {
	if (seen)
	    msg("%s wanders into a tree.", m_name);
	stun = TRUE;
    }

    /* Wall */
    else if (tf_has(f_ptr->flags, TF_WALL))
    {
	if (seen)
	    msg("%s bashes into a wall.", m_name);
	stun = TRUE;
    }

    /* Lava */
    else if (tf_has(f_ptr->flags, TF_FIERY))
    {
	/* Assume death */
	const char *note_dies = " is burnt to death in lava!";
	
	if (mon_take_hit
	    (cave_m_idx[m_ptr->fy][m_ptr->fx], 50 + m_ptr->maxhp / 50,
	     &fear, note_dies)) {
	    death = TRUE;
	} 
	else 
	{
	    if (seen)
		msg("%s is burnt by lava.", m_name);
	}
    }

    /* Water */
    else if (tf_has(f_ptr->flags, TF_WATERY)) 
    {
	/* Assume death */
	const char *note_dies = " is drowned!";
	
	if (mon_take_hit(cave_m_idx[m_ptr->fy][m_ptr->fx], 
			 5 + m_ptr->maxhp / 20, &fear, note_dies)) 
	{
	    death = TRUE;
	} 
	else 
	{
	    if (seen)
		msg("%s staggers into the water.", m_name);
	}
    }

    /* Void */
    else if (tf_has(f_ptr->flags, TF_FALL)) 
    {
	if (seen)
	    msg("%s stumbles off a cliff.", m_name);
	
	/* It's gone */
	delete_monster(y, x);
	return;
    }
 
    /* Sometimes stun the monster, but only lightly */
    if (stun && (randint0(3) == 0) && (m_ptr->stunned < 5))
	m_ptr->stunned += 3;

    /* Monster is frightened */
    if ((!death) && (fear) && (seen)) {
	msg("%s panics!", m_name);
    }
}


/**
 * Given a target grid, calculate the grid the monster will actually 
 * attempt to move into.
 *
 * The simplest case is when the target grid is adjacent to us and 
 * able to be entered easily.  Usually, however, one or both of these 
 * conditions don't hold, and we must pick an initial direction, than 
 * look at several directions to find that most likely to be the best 
 * choice.  If so, the monster needs to know the order in which to try 
 * other directions on either side.  If there is no good logical reason
 * to prioritize one side over the other, the monster will act on the 
 * "spur of the moment", using current turn as a randomizer.
 *
 * The monster then attempts to move into the grid.  If it fails, this 
 * function returns FALSE and the monster ends its turn.
 *
 * The variable "fear" is used to invoke any special rules for monsters 
 * wanting to retreat rather than advance.  For example, such monsters 
 * will not leave an non-viewable grid for a viewable one and will try 
 * to avoid the character.
 *
 * The variable "bash" remembers whether a monster had to bash a door 
 * or not.  This has to be remembered because the choice to bash is 
 * made in a different function than the actual bash move.  XXX XXX  If
 * the number of such variables becomes greater, a structure to hold them
 * would look better than passing them around from function to function.
 */
static bool make_move(monster_type * m_ptr, int *ty, int *tx, bool fear,
		      bool *bash)
{
    int i, j;

    /* Start direction, current direction */
    int dir0, dir;

    /* Deltas, absolute axis distances from monster to target grid */
    int dy, ay, dx, ax;

    /* Existing monster location, proposed new location */
    int oy, ox, ny, nx;

    bool avoid = FALSE;
    bool passable = FALSE;
    bool look_again = FALSE;

    int chance;

    /* Remember where monster is */
    oy = m_ptr->fy;
    ox = m_ptr->fx;

    /* Get the change in position needed to get to the target */
    dy = oy - *ty;
    dx = ox - *tx;

    /* Is the target grid adjacent to the current monster's position? */
    if ((!fear) && (dy >= -1) && (dy <= 1) && (dx >= -1) && (dx <= 1)) {
	/* If it is, try the shortcut of simply moving into the grid */

	/* Get the probability of entering this grid */
	chance = cave_passable_mon(m_ptr, *ty, *tx, bash);

	/* Grid must be pretty easy to enter, or monster must be confused */
	if ((m_ptr->confused) || (chance >= 50)) {
	    /* 
	     * Amusing messages and effects for confused monsters trying 
	     * to enter terrain forbidden to them.
	     */
	    if (chance == 0) {
		/* Sometimes hurt the poor little critter */
		if (randint0(5) == 0)
		    make_confused_move(m_ptr, *ty, *tx);

		/* Do not actually move */
		return (FALSE);
	    }

	    /* We can enter this grid */
	    if ((chance == 100) || (chance > randint0(100))) {
		return (TRUE);
	    }

	    /* Failure to enter grid.  Cancel move */
	    else {
		return (FALSE);
	    }
	}
    }



    /* Calculate vertical and horizontal distances */
    ay = ABS(dy);
    ax = ABS(dx);

    /* We mostly want to move vertically */
    if (ay > (ax * 2)) {
	/* Choose between directions '8' and '2' */
	if (dy > 0) {
	    /* We're heading up */
	    dir0 = 8;
	    if ((dx > 0) || (dx == 0 && turn % 2 == 0))
		dir0 += 10;
	} else {
	    /* We're heading down */
	    dir0 = 2;
	    if ((dx < 0) || (dx == 0 && turn % 2 == 0))
		dir0 += 10;
	}
    }

    /* We mostly want to move horizontally */
    else if (ax > (ay * 2)) {
	/* Choose between directions '4' and '6' */
	if (dx > 0) {
	    /* We're heading left */
	    dir0 = 4;
	    if ((dy < 0) || (dy == 0 && turn % 2 == 0))
		dir0 += 10;
	} else {
	    /* We're heading right */
	    dir0 = 6;
	    if ((dy > 0) || (dy == 0 && turn % 2 == 0))
		dir0 += 10;
	}
    }

    /* We want to move up and sideways */
    else if (dy > 0) {
	/* Choose between directions '7' and '9' */
	if (dx > 0) {
	    /* We're heading up and left */
	    dir0 = 7;
	    if ((ay < ax) || (ay == ax && turn % 2 == 0))
		dir0 += 10;
	} else {
	    /* We're heading up and right */
	    dir0 = 9;
	    if ((ay > ax) || (ay == ax && turn % 2 == 0))
		dir0 += 10;
	}
    }

    /* We want to move down and sideways */
    else {
	/* Choose between directions '1' and '3' */
	if (dx > 0) {
	    /* We're heading down and left */
	    dir0 = 1;
	    if ((ay > ax) || (ay == ax && turn % 2 == 0))
		dir0 += 10;
	} else {
	    /* We're heading down and right */
	    dir0 = 3;
	    if ((ay < ax) || (ay == ax && turn % 2 == 0))
		dir0 += 10;
	}
    }


    /* 
     * Now that we have an initial direction, we must determine which 
     * grid to actually move into.  
     */
    if (TRUE) 
    {
	/* Build a structure to hold movement data */
	typedef struct move_data move_data;
	struct move_data {
	    int move_chance;
	    bool move_bash;
	};
	move_data moves_data[8];


	/* 
	 * Scan each of the eight possible directions, in the order of 
	 * priority given by the table "side_dirs", choosing the one that 
	 * looks like it will get the monster to the character - or away 
	 * from him - most effectively.
	 */
	for (i = 0; i <= 8; i++) {
	    /* Out of options */
	    if (i == 8)
		break;

	    /* Get the actual direction */
	    dir = side_dirs[dir0][i];

	    /* Get the grid in our chosen direction */
	    ny = oy + ddy[dir];
	    nx = ox + ddx[dir];

	    /* Check Bounds */
	    if (!in_bounds(ny, nx))
		continue;

	    /* Store this grid's movement data. */
	    moves_data[i].move_chance = cave_passable_mon(m_ptr, ny, nx, bash);
	    moves_data[i].move_bash = *bash;


	    /* Confused monsters must choose the first grid */
	    if (m_ptr->confused)
		break;

	    /* If this grid is totally impassable, skip it */
	    if (moves_data[i].move_chance == 0)
		continue;

	    /* Frightened monsters work hard not to be seen. */
	    if (fear) {
		/* Monster is having trouble navigating to its target. */
		if ((m_ptr->ty) && (m_ptr->tx) && (i >= 2)) {
		    /* Look for an adjacent grid leading to the target */
		    if (get_route_to_target(m_ptr, ty, tx)) {
			int chance;

			/* Calculate the chance to enter the grid */
			chance = cave_passable_mon(m_ptr, *ty, *tx, bash);

			/* Try to move into the grid */
			if ((chance < 100) && (randint1(100) > chance)) {
			    /* Can't move */
			    return (FALSE);
			}

			/* Can move */
			return (TRUE);
		    }
		}

		/* Attacking the character as a first choice? */
		if ((i == 0) && (ny == p_ptr->py) && (nx == p_ptr->px)) {
		    /* Need to rethink some plans XXX XXX XXX */
		    m_ptr->ty = 0;
		    m_ptr->tx = 0;
		}

		/* Monster is visible */
		if (m_ptr->ml) {
		    /* And is in LOS */
		    if (player_has_los_bold(oy, ox)) {
			/* Accept any easily passable grid out of LOS */
			if ((!player_has_los_bold(ny, nx))
			    && (moves_data[i].move_chance > 40)) {
			    break;
			}
		    }

		    else {
			/* Do not enter a grid in LOS */
			if (player_has_los_bold(ny, nx)) {
			    moves_data[i].move_chance = 0;
			    continue;
			}
		    }
		}

		/* Monster can't be seen, and is not in a "seen" grid. */
		if ((!m_ptr->ml) && (!cave_has(cave_info[oy][ox], SQUARE_SEEN))) {
		    /* Do not enter a "seen" grid */
		    if (cave_has(cave_info[ny][nx], SQUARE_SEEN)) {
			moves_data[i].move_chance = 0;
			continue;
		    }
		}
	    }

	    /* XXX XXX -- Sometimes attempt to break glyphs. */
	    if (cave_trap_specific(ny, nx, RUNE_PROTECT) && (!fear)
		&& ((randint0(5) == 0) || (cave_m_idx[ny][nx] < 0))) {
		break;
	    }

	    /* Initial direction is almost certainly the best one */
	    if ((i == 0) && (moves_data[i].move_chance >= 80)) {
		/* 
		 * If backing away and close, try not to walk next 
		 * to the character, or get stuck fighting him.
		 */
		if ((fear) && (m_ptr->cdis <= 2)
		    && (distance(p_ptr->py, p_ptr->px, ny, nx) <= 1)) {
		    avoid = TRUE;
		}

		else
		    break;
	    }

	    /* Either of the first two side directions looks good */
	    else if (((i == 1) || (i == 2))
		     && (moves_data[i].move_chance >= 50)) {
		/* Accept the central direction if at least as good */
		if ((moves_data[0].move_chance >= moves_data[i].move_chance)) {
		    if (avoid) {
			/* Frightened monsters try to avoid the character */
			if (distance(p_ptr->py, p_ptr->px, ny, nx) == 0) {
			    i = 0;
			}
		    } else {
			i = 0;
		    }
		}

		/* Accept this direction */
		break;
	    }

	    /* This is the first passable direction */
	    if (!passable) {
		/* Note passable */
		passable = TRUE;

		/* All the best directions are blocked. */
		if (i >= 3) {
		    /* Settle for "good enough" */
		    break;
		}
	    }

	    /* We haven't made a decision yet; look again. */
	    if (i == 7)
		look_again = TRUE;
	}


	/* We've exhausted all the easy answers. */
	if (look_again) {
	    /* There are no passable directions. */
	    if (!passable) {
		return (FALSE);
	    }

	    /* We can move. */
	    for (j = 0; j < 8; j++) {
		/* Accept the first option, however poor.  XXX */
		if (moves_data[j].move_chance) {
		    i = j;
		    break;
		}
	    }
	}

	/* If no direction was acceptable, end turn */
	if (i >= 8) {
	    return (FALSE);
	}

	/* Get movement information (again) */
	dir = side_dirs[dir0][i];
	*bash = moves_data[i].move_bash;

	/* No good moves, so we just sit still and wait. */
	if ((dir == 5) || (dir == 0)) {
	    return (FALSE);
	}

	/* Get grid to move into */
	*ty = oy + ddy[dir];
	*tx = ox + ddx[dir];

	/* 
	 * Amusing messages and effects for confused monsters trying 
	 * to enter terrain forbidden to them.
	 */
	if ((m_ptr->confused) && (moves_data[i].move_chance == 0)) {
	    /* Sometimes hurt the poor little critter */
	    if (randint0(5) == 0)
		make_confused_move(m_ptr, *ty, *tx);

	    /* Do not actually move */
	    return (FALSE);
	}

	/* Try to move in the chosen direction.  If we fail, end turn. */
	if ((moves_data[i].move_chance < 100)
	    && (randint1(100) > moves_data[i].move_chance)) {
	    return (FALSE);
	}
    }


    /* Monster is frightened, and is obliged to fight. */
    if ((fear) && (cave_m_idx[*ty][*tx] < 0)) {
	/* Turn and fight */
	m_ptr->monfear = 0;
	fear = FALSE;

	/* Recalculate combat range (later) */
	m_ptr->min_range = 0;

	/* Message if seen */
	if (m_ptr->ml) {
	    char m_name[80];

	    /* Get the monster name */
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

	    /* Dump a message */
	    msg("%s turns to fight!", m_name);
	}
    }

    /* We can move. */
    return (TRUE);
}




/**
 * If one monster moves into another monster's grid, they will 
 * normally swap places.  If the second monster cannot exist in the 
 * grid the first monster left, this can't happen.  In such cases, 
 * the first monster tries to push the second out of the way.
 */
static bool push_aside(monster_type * m_ptr, monster_type * n_ptr)
{
    /* Get racial information about the second monster */
    monster_race *nr_ptr = &r_info[n_ptr->r_idx];

    int y, x, i;
    int dir = 0;


    /* 
     * Translate the difference between the locations of the two 
     * monsters into a direction of travel.
     */
    for (i = 0; i < 10; i++) {
	/* Require correct difference along the y-axis */
	if ((n_ptr->fy - m_ptr->fy) != ddy[i])
	    continue;

	/* Require correct difference along the x-axis */
	if ((n_ptr->fx - m_ptr->fx) != ddx[i])
	    continue;

	/* Found the direction */
	dir = i;
	break;
    }

    /* Favor either the left or right side on the "spur of the moment". */
    if (turn % 2 == 0)
	dir += 10;

    /* Check all directions radiating out from the initial direction. */
    for (i = 0; i < 7; i++) {
	int side_dir = side_dirs[dir][i];

	y = n_ptr->fy + ddy[side_dir];
	x = n_ptr->fx + ddx[side_dir];

	/* Illegal grid */
	if (!in_bounds_fully(y, x))
	    continue;

	/* Grid is not occupied, and the 2nd monster can exist in it. */
	if (cave_exist_mon(nr_ptr, y, x, FALSE)) {
	    /* Push the 2nd monster into the empty grid. */
	    monster_swap(n_ptr->fy, n_ptr->fx, y, x);
	    return (TRUE);
	}
    }

    /* We didn't find any empty, legal grids */
    return (FALSE);
}

/**
 * Check the effect of the Rogue's monster trap.  Certain traps may be avoided 
 * by certain monsters.  Traps may be disarmed by smart monsters.
 * If the trap works, the effects vary depending on trap type. -BR-
 *
 * "death" tells the calling function if the monster is killed by the trap.
 */
static void apply_monster_trap(monster_type * m_ptr, int y, int x, bool * death)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_lore *l_ptr = &l_list[m_ptr->r_idx];
    int dis_chance;

    int trap = monster_trap_idx(y, x);
    trap_type *t_ptr;

    /* Assume monster not frightened by trap */
    bool fear = FALSE;

    /* Assume the trap works */
    bool trap_hit = TRUE;

    /* Assume trap is not destroyed */
    bool trap_destroyed = FALSE;

    /* Assume monster lives */
    bool mon_dies = FALSE;

    char m_name[80];

    /* Sanity check */
    if (!cave_monster_trap(y, x))
	return;

    if ((trap < 0) || (trap >= trap_max))
	return;
    else 
	t_ptr = &trap_list[trap];

    /* Get "the monster" or "it" */
    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

    /* Non-flying monsters usually avoid netted traps */
    if (t_ptr->t_idx == MTRAP_NET) 
    {
	if (!(rf_has(r_ptr->flags, RF_FLYING))
	    && (randint0(3) != 0 || rf_has(r_ptr->flags, RF_PASS_WALL))) {
	    if (m_ptr->ml)
		msg("%s avoids your netted trap.", m_name);
	    trap_hit = FALSE;
	}
    }

    /* Spirit traps only affect insubstantial creatures */
    else if (t_ptr->t_idx == MTRAP_SPIRIT) 
    {
	if (!(rf_has(r_ptr->flags, RF_PASS_WALL))) {
	    if (m_ptr->ml)
		msg("%s ignores your spirit trap.", m_name);
	    trap_hit = FALSE;
	}
    }

    /* Stasis traps affect everyone */
    else if (t_ptr->t_idx == MTRAP_STASIS) {
    }

    /* Lightning traps affect all but ghosts */
    else if (t_ptr->t_idx == MTRAP_ELEC) {
	if ((rf_has(r_ptr->flags, RF_PASS_WALL)) && (randint0(4) != 0)) {
	    if (m_ptr->ml)
		msg("%s flies over your trap.", m_name);
	    trap_hit = FALSE;
	}
    }

    /* Other traps seldom affect flying monsters or ghosts. */
    else if (((rf_has(r_ptr->flags, RF_PASS_WALL))
	      || (rf_has(r_ptr->flags, RF_FLYING))) && (randint0(4) != 0)) 
    {
	if (m_ptr->ml)
	    msg("%s flies over your trap.", m_name);
	trap_hit = FALSE;
    }

    /* Find the monsters base skill at disarming */
    dis_chance = 40 + (2 * r_ptr->level);

    /* Keep it in line for high level monsters */
    if (dis_chance > 215)
	dis_chance = 215;

    /* Smart monsters may attempts to disarm traps which would affect them */
    if ((trap_hit) && (rf_has(r_ptr->flags, RF_SMART))
	&& (randint1(dis_chance) > p_ptr->state.skills[SKILL_DISARM] - 15)) 
    {
	if (m_ptr->ml)
	    msg("%s finds your trap and disarms it.", m_name);

	/* Trap is gone */
	trap_destroyed = TRUE;

	/* Didn't work */
	trap_hit = FALSE;
    }

    /* Monsters can be wary of traps */
    if ((trap_hit) && (m_ptr->mflag & (MFLAG_WARY))) {
	/* Check for avoidance */
	if (randint1(dis_chance) > (p_ptr->state.skills[SKILL_DISARM] - 15) / 2) {
	    if (m_ptr->ml)
		msg("%s avoids your trap.", m_name);

	    /* Didn't work */
	    trap_hit = FALSE;
	}
    }

    /* I thought traps only affected players! Unfair! */
    if (trap_hit) 
    {
	/* Assume a default death */
	const char *note_dies = " dies.";

	int n, trap_power;

	/* Some monsters get "destroyed" */
	if ((rf_has(r_ptr->flags, RF_DEMON))
	    || (rf_has(r_ptr->flags, RF_UNDEAD))
	    || (rf_has(r_ptr->flags, RF_STUPID))
	    || (strchr("Evg", r_ptr->d_char))) {
	    /* Special note at death */
	    note_dies = " is destroyed.";
	}

	/* Monster becomes hostile */
	m_ptr->hostile = -1;

	/* Players sees the monster */
	if (m_ptr->ml)
	    msg("%s sets off your cunning trap!", m_name);

	/* Not seen but in line of sight */
	else if (player_has_los_bold(y, x))
	    msg("Something sets off your cunning trap!");

	/* Monster is not seen or in LOS */
	else 
	{
	    /* HACK - no message for non-damaging traps */
	    if (!(t_ptr->t_idx == MTRAP_CONF) && !(t_ptr->t_idx == MTRAP_PORTAL)
		&& !(t_ptr->t_idx == MTRAP_STASIS)) {
		msg("You hear anguished yells in the distance.");
	    }
	}

	/* Explosion traps are always destroyed. */
	if ((t_ptr->t_idx == MTRAP_EXPLOSIVE) || (t_ptr->t_idx == MTRAP_STASIS)
	    || (t_ptr->t_idx == MTRAP_GENOCIDE)) 
	{
	    trap_destroyed = TRUE;
	}

	/* Some traps are rarely destroyed */
	else if (t_ptr->t_idx == MTRAP_STURDY) 
	{
	    if (randint0(8) == 0)
		trap_destroyed = TRUE;
	}

	/* Most traps are destroyed 1 time in 3 */
	else if (randint0(3) == 0)
	    trap_destroyed = TRUE;

	/* Find the 'power' of the trap effect */
	n = p_ptr->lev + ((p_ptr->lev * p_ptr->lev) / 25);
	trap_power = 3 + randint1(n) + (n / 2);

	/* Monsters can be wary of traps */
	if (m_ptr->mflag & (MFLAG_WARY))
	    trap_power /= 3;

	/* Trap 'critical' based on disarming skill (if not wary) */
	else if (randint1(p_ptr->state.skills[SKILL_DISARM]) >
		 50 + r_ptr->level)
	    trap_power += trap_power / 2;

	/* Affect the monster. */
	/* The basic trap does full damage */
	if (t_ptr->t_idx == MTRAP_BASE) 
	{
	    if (mon_take_hit(cave_m_idx[y][x], trap_power, &fear, note_dies))
		mon_dies = TRUE;
	}

	/* Confusion trap */
	else if (t_ptr->t_idx == MTRAP_CONF) 
	{
	    int tmp;
	    tmp = randint0((3 * trap_power) / 2) - r_ptr->level - 10;

	    /* Confuse the monster */
	    if (rf_has(r_ptr->flags, RF_NO_CONF)) {
		if (m_ptr->ml) {
		    rf_on(l_ptr->flags, RF_NO_CONF);
		    msg("%s is unaffected.", m_name);
		}
	    } else if (tmp < 0) {
		if (m_ptr->ml)
		    msg("%s is unaffected.", m_name);
	    } else {
		/* Confuse the target */
		if (m_ptr->confused) {
		    m_ptr->confused += 2 + tmp / 2;
		    if (m_ptr->ml)
			msg("%s is more confused.", m_name);
		} else {
		    m_ptr->confused += 4 + tmp;
		    if (m_ptr->ml)
			msg("%s is confused.", m_name);
		}
	    }
	}

	else if (t_ptr->t_idx == MTRAP_POISON) {
	    (void) fire_meteor(0, GF_POIS, y, x, trap_power / 2, 3, FALSE);
	    if (!(m_ptr->r_idx))
		mon_dies = TRUE;
	}

	else if (t_ptr->t_idx == MTRAP_ELEC) {
	    (void) fire_meteor(0, GF_ELEC, y, x, (7 * trap_power) / 8, 0,
			       FALSE);
	    if (!(m_ptr->r_idx))
		mon_dies = TRUE;
	}

	else if (t_ptr->t_idx == MTRAP_EXPLOSIVE) {
	    (void) fire_meteor(0, GF_FIRE, y, x, (3 * trap_power) / 8, 1,
			       FALSE);
	    (void) fire_meteor(0, GF_SHARD, y, x, (3 * trap_power) / 8, 1,
			       FALSE);
	    if (!(m_ptr->r_idx))
		mon_dies = TRUE;
	}

	else if (t_ptr->t_idx == MTRAP_PORTAL) {
	    if (m_ptr->ml)
		msg("%s is teleported.", m_name);
	    teleport_away(cave_m_idx[y][x], 5 + (trap_power / 10));
	}

	else if (t_ptr->t_idx == MTRAP_STASIS) {
	    /* Stasis the target */
	    m_ptr->stasis = 3 + (trap_power / 12);
	    if (m_ptr->ml)
		msg("%s is caught in stasis!", m_name);
	}

	else if (t_ptr->t_idx == MTRAP_DRAIN_LIFE) {
	    (void) fire_meteor(0, GF_OLD_DRAIN, y, x, (3 * trap_power) / 4, 3,
			       FALSE);
	    if (!(m_ptr->r_idx))
		mon_dies = TRUE;
	}

	else if (t_ptr->t_idx == MTRAP_UNMAGIC) {
	    (void) fire_meteor(0, GF_DISEN, y, x, (3 * trap_power) / 4, 0,
			       FALSE);
	    if (!(m_ptr->r_idx))
		mon_dies = TRUE;
	}

	else if (t_ptr->t_idx == MTRAP_DISPEL_M) {
	    /* 50% - 75% damage of trap power to all creatures within LOS of
	     * trap */
	    int dam = ((trap_power / 2) + randint1(trap_power / 4));

	    (void) project_los_not_player(y, x, dam, GF_DISP_ALL);
	    if (!(m_ptr->r_idx))
		mon_dies = TRUE;
	}

	else if (t_ptr->t_idx == MTRAP_GENOCIDE) {
	    int i;
	    char typ = r_ptr->d_char;

	    if (m_ptr->ml)
		msg("%s vanishes!", m_name);

	    /* Delete the monsters of that "type" */
	    for (i = 1; i < m_max; i++) {
		monster_type *n_ptr = &m_list[i];
		monster_race *r1_ptr = &r_info[n_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!n_ptr->r_idx)
		    continue;

		/* Hack -- Skip Unique Monsters */
		if (rf_has(r1_ptr->flags, RF_UNIQUE))
		    continue;

		/* Skip "wrong" monsters */
		if (r1_ptr->d_char != typ)
		    continue;

		/* Ignore monsters in icky squares */
		if (cave_has(cave_info[n_ptr->fy][n_ptr->fx], SQUARE_ICKY))
		    continue;

		/* Ignore monsters too far away */
		if (distance(n_ptr->fy, n_ptr->fx, y, x) > 5)
		    continue;

		/* Delete the monster */
		delete_monster_idx(i);
	    }

	}

	/* Other traps (sturdy, net, spirit) default to 75% of normal damage */
	else 
	{
	    if (mon_take_hit
		(cave_m_idx[y][x], (3 * trap_power) / 4, &fear, note_dies))
		mon_dies = TRUE;
	}

	/* Take note */
	if (!mon_dies && fear && m_ptr->ml)
	    msg("%s flees in terror!", m_name);

	/* May become wary if not dumb */
	if ((!(rf_has(r_ptr->flags, RF_STUPID)))
	    && ((randint0(4) == 0) || (m_ptr->hp < m_ptr->maxhp / 2)))
	    m_ptr->mflag |= (MFLAG_WARY);
    }

    if (trap_destroyed) 
    {
	/* Kill the trap */
	(void) remove_trap(y, x, FALSE, trap);
    }

    /* Report death */
    if (mon_dies)
	(*death) = TRUE;

    /* Return */
    return;
}


/**
 * Process a monster's move.
 *
 * All the plotting and planning has been done, and all this function 
 * has to do is move the monster into the chosen grid.
 *
 * This may involve attacking the character, breaking a glyph of 
 * warding, bashing down a door, etc..  Once in the grid, monsters may 
 * stumble into monster traps, hit a scent trail, pick up or destroy 
 * objects, and so forth.
 *
 * A monster's move may disturb the character, depending on which 
 * disturbance options are set.
 */
static void process_move(monster_type * m_ptr, int ty, int tx, bool bash)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_lore *l_ptr = &l_list[m_ptr->r_idx];

    /* Existing monster location, proposed new location */
    int oy, ox, ny, nx;

    s16b this_o_idx, next_o_idx = 0;

    int feat;
    int i;
    feature_type *f_ptr;

    /* Default move, default lack of view */
    bool do_move = TRUE;
    bool do_view = FALSE;

    /* Assume nothing */
    bool did_open_door = FALSE;
    bool did_bash_door = FALSE;
    bool did_take_item = FALSE;
    bool did_kill_item = FALSE;
    bool did_move_body = FALSE;
    bool did_kill_body = FALSE;
    bool did_pass_wall = FALSE;
    bool did_kill_wall = FALSE;

    /* Remember where monster is */
    oy = m_ptr->fy;
    ox = m_ptr->fx;

    /* Get the destination */
    ny = ty;
    nx = tx;

    /* Check Bounds */
    if (!in_bounds(ny, nx))
	return;

    /* The grid is occupied by the player. */
    if ((cave_m_idx[ny][nx] < 0) && (m_ptr->hostile < 0)) 
    {
	/* SJGU Reset the monster's target (to deal with players hidden by
	 * superstealth) */
	int px = p_ptr->px;
	int py = p_ptr->py;
	m_ptr->tx = px;
	m_ptr->ty = py;

	/* Attack if possible */
	if (!(rf_has(r_ptr->flags, RF_NEVER_BLOW))) {
	    (void) make_attack_normal(m_ptr, ny, nx);
	}

	/* End move */
	do_move = FALSE;
    }

    /* No player here. */
    else 
    {
	/* Final sanity check on non-moving monsters */
	if (rf_has(r_ptr->flags, RF_NEVER_MOVE))
	    do_move = FALSE;
    }

    /* Check if the monster is in a web */
    if (cave_web(oy, ox)) 
    {
	/* Insects and bats get stuck */
	if (strchr("abcFIKl", r_ptr->d_char))
	    do_move = FALSE;

	/* Spiders go right through */
	else if (strchr("S", r_ptr->d_char));

	/* Insubstantial monsters go right through, observably */
	else if (rf_has(r_ptr->flags, RF_PASS_WALL))
	    did_pass_wall = TRUE;

	/* If you can destroy a wall, you can destroy a web */
	else if (rf_has(r_ptr->flags, RF_KILL_WALL)) 
	{
	    /* Remove the web */
	    remove_trap_kind(oy, ox, FALSE, OBST_WEB);

	    /* Notice */
	    did_kill_wall = TRUE;
	}

	/* Otherwise have to spend a turn tearing the web */
	else 
	{
	    /* Remove the web */
	    remove_trap_kind(oy, ox, FALSE, OBST_WEB);

	    /* Now can't do anything else */
	    do_move = FALSE;
	}
    }


    /* Glyphs */
    if (cave_trap_specific(ny, nx, RUNE_PROTECT)) 
    {
	/* Describe observable breakage */
	if (cave_has(cave_info[ny][nx], SQUARE_MARK)) 
	{
	    msg("The rune of protection is broken!");
	}

	/* Forget the rune */
	cave_off(cave_info[ny][nx], SQUARE_MARK);

	/* Break the rune */
	remove_trap_kind(ny, nx, FALSE, RUNE_PROTECT);
    }

    /* Get the feature in the grid that the monster is trying to enter. */
    feat = cave_feat[ny][nx];
    f_ptr = &f_info[feat];


    /* Entering a wall */
    if (tf_has(f_ptr->flags, TF_WALL)|| tf_has(f_ptr->flags, TF_DOOR_CLOSED)) 
    {
	/* Monster passes through walls (and doors) */
	if (rf_has(r_ptr->flags, RF_PASS_WALL)) 
	{
	    /* Monster went through a wall */
	    did_pass_wall = TRUE;
	}

	/* Monster destroys walls (and doors) */
	else if (rf_has(r_ptr->flags, RF_KILL_WALL)) 
	{
	    /* Monster destroyed a wall */
	    did_kill_wall = TRUE;

	    /* Forget the wall */
	    cave_off(cave_info[ny][nx], SQUARE_MARK);

	    /* Notice */
	    if (outside)
		cave_set_feat(ny, nx, FEAT_ROAD);
	    else
		cave_set_feat(ny, nx, FEAT_FLOOR);

	    /* Note changes to grid - but only if actually seen */
	    if (cave_has(cave_info[ny][nx], SQUARE_SEEN))
		do_view = TRUE;
	}

	/* Doors */
	else if (tf_has(f_ptr->flags, TF_DOOR_ANY))
	{
	    /* Monster bashes the door down */
	    if (bash) 
	    {
		/* Character is not too far away */
		if (m_ptr->cdis < 30) 
		{
		    /* Message */
		    msg("You hear a door burst open!");

		    /* Disturb */
		    disturb(0, 0);
		}

		/* The door was bashed open */
		did_bash_door = TRUE;

		/* Break down the door */
		if (randint0(100) < 50)
		    cave_set_feat(ny, nx, FEAT_BROKEN);
		else
		    cave_set_feat(ny, nx, FEAT_OPEN);

		/* Handle viewable doors */
		if (cave_has(cave_info[ny][nx], SQUARE_SEEN)) 
		    do_view = TRUE;
		
		/* Disturb */
		disturb(0, 0);
	    }

	    /* Monster opens the door */
	    else 
	    {
		/* Locked doors */
		if (tf_has(f_ptr->flags, TF_DOOR_LOCKED)) 
		{
		    /* Unlock the door */
		    cave_set_feat(ny, nx, FEAT_DOOR_HEAD);

		    /* Do not move */
		    do_move = FALSE;
		}

		/* Ordinary doors */
		else 
		{
		    /* The door is open */
		    did_open_door = TRUE;

		    /* Open the door */
		    cave_set_feat(ny, nx, FEAT_OPEN);

		    /* Step into doorway sometimes */
		    if (randint0(5) != 0)
			do_move = FALSE;
		}
	    }
	}

	/* Paranoia */
	else
	    return;
    }

    /* Monster is allowed to move */
    if (do_move) 
    {
	/* The grid is occupied by a monster. */
	if (cave_m_idx[ny][nx] > 0) 
	{
	    monster_type *n_ptr = &m_list[cave_m_idx[ny][nx]];
	    monster_race *nr_ptr = &r_info[n_ptr->r_idx];

	    /* XXX - Kill weaker monsters */
	    if ((rf_has(r_ptr->flags, RF_KILL_BODY))
		&& (!(rf_has(r_ptr->flags, RF_UNIQUE)))
		&& (r_ptr->mexp > nr_ptr->mexp)) 
	    {
		/* Monster ate another monster */
		did_kill_body = TRUE;

		/* Kill the monster */
		delete_monster(ny, nx);
	    }

	    /* Swap with or push aside the other monster */
	    else 
	    {
		/* The other monster cannot switch places */
		if (!cave_exist_mon(nr_ptr, m_ptr->fy, m_ptr->fx, TRUE)) 
		{
		    /* Try to push it aside */
		    if (!push_aside(m_ptr, n_ptr)) 
		    {
			/* Cancel move on failure */
			do_move = FALSE;
		    }
		}

		/* Note */
		if (do_move)
		    did_move_body = TRUE;
	    }
	}
    }


    /* Monster can (still) move */
    if (do_move) 
    {
	/* Move the monster */
	monster_swap(oy, ox, ny, nx);

	/* Cancel target when reached */
	if ((m_ptr->ty == ny) && (m_ptr->tx == nx)) 
	{
	    m_ptr->ty = 0;
	    m_ptr->tx = 0;
	}

	/* Deal with SMASH_WALL */
	if (rf_has(r_ptr->flags, RF_SMASH_WALL)) {
	    int xx, yy;
	    feature_type *f_ptr;

	    for (i = 0; i < 9; i++) {
		/* Extract adjacent (legal) location */
		yy = ny + ddy_ddd[i];
		xx = nx + ddx_ddd[i];

		/* Paranoia */
		if (!in_bounds_fully(yy, xx))
		    continue;

		/* Get the feature */
		f_ptr = &f_info[cave_feat[yy][xx]];

		/* Kill it */
		if (!tf_has(f_ptr->flags, TF_LOS) || 
		    (tf_has(f_ptr->flags, TF_DOOR_ANY))) 
		{
		    /* Monster destroyed a wall */
		    did_kill_wall = TRUE;

		    /* Forget the wall */
		    cave_off(cave_info[yy][xx], SQUARE_MARK);

		    /* Notice */
		    if (outside)
			cave_set_feat(yy, xx, FEAT_ROAD);
		    else
			cave_set_feat(yy, xx, FEAT_FLOOR);

		    /* Note changes to grid - but only if actually seen */
		    if (cave_has(cave_info[yy][xx], SQUARE_SEEN))
			do_view = TRUE;
		}

	    }
	}


	/* Check for monster trap */
	if (cave_monster_trap(ny, nx))
	{
	    bool death = FALSE;

	    /* Apply trap */
	    apply_monster_trap(m_ptr, ny, nx, &death);

	    /* Return if dead */
	    if (death)
		return;
	}

	/* Check for runes of speed, slow if not slowed already */
	if (cave_trap_specific(ny, nx, RUNE_SPEED)
	    && (m_ptr->mspeed > r_ptr->speed - 5)) 
	{
	    char m_name[80];

	    /* Get the monster name */
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

	    /* Get a saving throw */
	    if (randint0(400) < 2 * r_ptr->level) 
	    {
		if (m_ptr->ml)
		    msg("%s is unaffected!", m_name);
	    } 
	    else 
	    {
		if (m_ptr->ml)
		    msg("%s starts moving slower.", m_name);
		m_ptr->mspeed -= 10;
	    }
	}


	/* Check for runes of mana */
	if (cave_trap_specific(ny, nx, RUNE_MANA)) 
	{
	    int drain = BASE_MANA_BURN;
	    char m_name[80];

	    /* Get the monster name */
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

	    /* Can only drain what we have */
	    if (drain > m_ptr->mana)
		drain = m_ptr->mana;

	    /* Drain mana and add to the rune */
	    m_ptr->mana -= drain;
	    mana_reserve += drain;
	    if (mana_reserve > MAX_MANA_RESERVE)
		mana_reserve = MAX_MANA_RESERVE;

	    if (m_ptr->ml)
		msg("%s loses some mana.", m_name);
	}

	/* Check for runes of instability */
	if (cave_trap_specific(ny, nx, RUNE_QUAKE))
	    earthquake(ny, nx, 4, FALSE);

	/* 
	 * If a member of a monster group capable of smelling hits a 
	 * scent trail while out of LOS of the character, it will 
	 * communicate this to similar monsters.
	 */
	if ((!player_has_los_bold(ny, nx)) && (rf_has(r_ptr->flags, RF_FRIENDS))
	    && (monster_can_smell(m_ptr)) && (get_scent(oy, ox) == -1)
	    && (!m_ptr->ty) && (!m_ptr->tx)) 
	{
	    int i;
	    monster_type *n_ptr;
	    monster_race *nr_ptr;

	    /* Scan all other monsters */
	    for (i = m_max - 1; i >= 1; i--) {
		/* Access the monster */
		n_ptr = &m_list[i];
		nr_ptr = &r_info[n_ptr->r_idx];

		/* Ignore dead monsters */
		if (!n_ptr->r_idx)
		    continue;

		/* Ignore monsters with the wrong symbol */
		if (r_ptr->d_char != nr_ptr->d_char)
		    continue;

		/* Ignore monsters with specific orders */
		if ((n_ptr->ty) || (n_ptr->ty))
		    continue;

		/* Ignore monsters picking up a good scent */
		if (get_scent(n_ptr->fy, n_ptr->fx) < SMELL_STRENGTH - 10)
		    continue;

		/* Ignore monsters not in LOS */
		if (!los(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx))
		    continue;

		/* Activate all other monsters and give directions */
		n_ptr->csleep = 0;
		n_ptr->mflag |= (MFLAG_ACTV);
		n_ptr->ty = ny;
		n_ptr->tx = nx;
	    }
	}


	/* Player will always be disturbed if a monster is adjacent */
	if (m_ptr->cdis == 1) 
	    disturb(1, 0);
	
	/* Possible disturb */
	else if (m_ptr->ml && (m_ptr->hostile == -1)
		 && (OPT(disturb_near) && 
		     ((m_ptr->mflag & (MFLAG_VIEW))
		      || ((rf_has(r_ptr->flags, RF_PASS_WALL)
			   || rf_has(r_ptr->flags, RF_KILL_WALL))
			  && (m_ptr->cdis <= 2))))) {
	    /* Disturb */
	    disturb(0, 0);
	}


	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[ny][nx]; this_o_idx;
	     this_o_idx = next_o_idx) 
	{
	    object_type *o_ptr;

	    /* Acquire object */
	    o_ptr = &o_list[this_o_idx];

	    /* Acquire next object */
	    next_o_idx = o_ptr->next_o_idx;

	    /* Skip gold */
	    if (o_ptr->tval == TV_GOLD)
		continue;

	    /* Take or kill objects on the floor */
	    if ((rf_has(r_ptr->flags, RF_TAKE_ITEM))
		|| (rf_has(r_ptr->flags, RF_KILL_ITEM))) {
		bitflag mon_flags[RF_SIZE];

		char m_name[80];
		char o_name[120];

		/* Wipe the flags */
		rf_wipe(mon_flags);

		/* Acquire the object name */
		object_desc(o_name, sizeof(o_name), o_ptr,
			    ODESC_PREFIX | ODESC_FULL);

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x104);

		/* React to objects that hurt the monster */
		if (o_slay(o_ptr, P_SLAY_DRAGON))
		    rf_on(mon_flags, RF_DRAGON);
		if (o_slay(o_ptr, P_SLAY_TROLL))
		    rf_on(mon_flags, RF_TROLL);
		if (o_slay(o_ptr, P_SLAY_GIANT))
		    rf_on(mon_flags, RF_GIANT);
		if (o_slay(o_ptr, P_SLAY_ORC))
		    rf_on(mon_flags, RF_ORC);
		if (o_slay(o_ptr, P_SLAY_DEMON))
		    rf_on(mon_flags, RF_DEMON);
		if (o_slay(o_ptr, P_SLAY_UNDEAD))
		    rf_on(mon_flags, RF_UNDEAD);
		if (o_slay(o_ptr, P_SLAY_ANIMAL))
		    rf_on(mon_flags, RF_ANIMAL);
		if (o_slay(o_ptr, P_SLAY_EVIL))
		    rf_on(mon_flags, RF_EVIL);

		/* The object cannot be picked up by the monster */
		if (artifact_p(o_ptr) || rf_is_inter(r_ptr->flags, mon_flags)) {
		    /* Only give a message for "take_item" */
		    if (rf_has(r_ptr->flags, RF_TAKE_ITEM)) {
			/* Take note */
			did_take_item = TRUE;

			/* Describe observable situations */
			if (m_ptr->ml && player_has_los_bold(ny, nx)
			    && !squelch_hide_item(o_ptr)) {
			    /* Dump a message */
			    msg("%s tries to pick up %s, but fails.",
				       m_name, o_name);
			}
		    }
		}

		/* Pick up the item */
		else if (rf_has(r_ptr->flags, RF_TAKE_ITEM)) {
		    object_type *i_ptr;
		    object_type object_type_body;

		    /* Take note */
		    did_take_item = TRUE;

		    /* Describe observable situations */
		    if (player_has_los_bold(ny, nx)
			&& !squelch_hide_item(o_ptr)) {
			/* Dump a message */
			msg("%s picks up %s.", m_name, o_name);
		    }

		    /* Get local object */
		    i_ptr = &object_type_body;

		    /* Obtain local object */
		    object_copy(i_ptr, o_ptr);

		    /* Delete the object */
		    delete_object_idx(this_o_idx);

		    /* Carry the object */
		    (void) monster_carry(cave_m_idx[m_ptr->fy][m_ptr->fx],
					 i_ptr);
		}

		/* Destroy the item */
		else {
		    /* Take note */
		    did_kill_item = TRUE;

		    /* Describe observable situations */
		    if (player_has_los_bold(ny, nx)
			&& !squelch_hide_item(o_ptr)) {
			/* Dump a message */
			msg("%s crushes %s.", m_name, o_name);
		    }

		    /* Delete the object */
		    delete_object_idx(this_o_idx);
		}
	    }
	}




	/* End of monster's move */
	/* Notice changes in view */
	if (do_view) {
	    /* Update the visuals */
	    p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Learn things from observable monster */
	if (m_ptr->ml) {
	    /* Monster opened a door */
	    if (did_open_door)
		rf_on(l_ptr->flags, RF_OPEN_DOOR);

	    /* Monster bashed a door */
	    if (did_bash_door)
		rf_on(l_ptr->flags, RF_BASH_DOOR);

	    /* Monster tried to pick something up */
	    if (did_take_item)
		rf_on(l_ptr->flags, RF_TAKE_ITEM);

	    /* Monster tried to crush something */
	    if (did_kill_item)
		rf_on(l_ptr->flags, RF_KILL_ITEM);

	    /* Monster pushed past another monster */
	    if (did_move_body)
		rf_on(l_ptr->flags, RF_MOVE_BODY);

	    /* Monster ate another monster */
	    if (did_kill_body)
		rf_on(l_ptr->flags, RF_KILL_BODY);

	    /* Monster passed through a wall */
	    if (did_pass_wall)
		rf_on(l_ptr->flags, RF_PASS_WALL);

	    /* Monster destroyed a wall */
	    if (did_kill_wall)
		rf_on(l_ptr->flags, RF_KILL_WALL);
	}
    }
}

/**
 * Monster takes its turn.
 */
static void process_monster(monster_type * m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_lore *l_ptr = &l_list[m_ptr->r_idx];

    int i, k, y, x;
    int ty, tx;
    int chance = 0;
    int choice = 0;
    int shape_rate = 0;
    int old_shape = m_ptr->orig_idx;
    int old_race = m_ptr->old_p_race;
    int dir;
    int scan_range = (MODE(SMALL_DEVICE) ? r_ptr->aaf / 2 : r_ptr->aaf);
    bool fear = FALSE;

    bool bash;

    /* Assume the monster is able to perceive the player. */
    bool aware = TRUE;
    bool must_use_target = FALSE;

    /* Will the monster move randomly? */
    bool random = FALSE;


    /* If monster is sleeping, or in stasis, it loses its turn. */
    if ((m_ptr->csleep) || (m_ptr->stasis))
	return;

    /* Calculate the monster's preferred combat range when needed */
    if (m_ptr->min_range == 0)
	find_range(m_ptr);

    /* Handle territorial monsters */
    if ((rf_has(r_ptr->flags, RF_TERRITORIAL))
	&& (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)) {
	/* Territorial monsters get a direct energy boost when close to home */
	int from_home =
	    distance(m_ptr->y_terr, m_ptr->x_terr, m_ptr->fy, m_ptr->fx);

	/* Step up in units of a fifth of monster detection range */
	for (i = 5; i > 0; i--) {
	    if ((from_home * i) > (scan_range * 5))
		break;
	}

	/* Add the energy */
	m_ptr->energy += (5 - i);

	/* If target is too far away from home, go back */
	if (distance(m_ptr->y_terr, m_ptr->x_terr, m_ptr->ty, m_ptr->tx) >
	    5 * scan_range) {
	    m_ptr->ty = m_ptr->y_terr;
	    m_ptr->tx = m_ptr->x_terr;
	}
    }


    /* Monster is in active mode. */
    if (m_ptr->mflag & (MFLAG_ACTV)) {
	/* 
	 * Character is outside of scanning range and well outside 
	 * of sighting range.  Monster does not have a target.
	 */
	if ((m_ptr->cdis >= FLEE_RANGE) && (m_ptr->cdis > scan_range)
	    && (!m_ptr->ty) && (!m_ptr->tx)) {
	    /* Monster cannot smell the character */
	    if (!cave_when[m_ptr->fy][m_ptr->fx])
		m_ptr->mflag &= ~(MFLAG_ACTV);
	    else if (!monster_can_smell(m_ptr))
		m_ptr->mflag &= ~(MFLAG_ACTV);
	}
    }

    /* Monster is in passive mode. */
    else {
	/* Character is inside scanning range */
	if (m_ptr->cdis <= scan_range)
	    m_ptr->mflag |= (MFLAG_ACTV);

	/* Monster has a target */
	else if ((m_ptr->ty) && (m_ptr->tx))
	    m_ptr->mflag |= (MFLAG_ACTV);

	/* The monster is catching too much of a whiff to ignore */
	else if (cave_when[m_ptr->fy][m_ptr->fx]) {
	    if (monster_can_smell(m_ptr))
		m_ptr->mflag |= (MFLAG_ACTV);
	}
    }


    /* A monster in passive mode will end its turn at this point. */
    if (!(m_ptr->mflag & (MFLAG_ACTV)))
	return;


    /* Hack -- Always redraw the current target monster health bar */
    if (p_ptr->health_who == cave_m_idx[m_ptr->fy][m_ptr->fx])
	p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);


    /* Attempt to multiply if able to and allowed */
    if ((rf_has(r_ptr->flags, RF_MULTIPLY)) && (num_repro < MAX_REPRO)) {
	/* Count the adjacent monsters */
	for (k = 0, y = m_ptr->fy - 1; y <= m_ptr->fy + 1; y++) {
	    for (x = m_ptr->fx - 1; x <= m_ptr->fx + 1; x++) {
		/* Check Bounds */
		if (!in_bounds(y, x))
		    continue;

		/* Count monsters */
		if (cave_m_idx[y][x] > 0)
		    k++;
	    }
	}

	/* Hack -- multiply slower in crowded areas */
	if ((k < 4) && (!k || !randint0(k * MON_MULT_ADJ))) {
	    /* Try to multiply */
	    if (multiply_monster(cave_m_idx[m_ptr->fy][m_ptr->fx])) {
		/* Take note if visible */
		if (m_ptr->ml) {
		    rf_on(l_ptr->flags, RF_MULTIPLY);

		    /* Make a sound */
		    sound(MSG_MULTIPLY);
		}

		/* Multiplying takes energy */
		return;
	    }
	}
    }


    /* Players hidden in shadow are difficult to see. */
    if ((p_ptr->timed[TMD_SSTEALTH]) && (!p_ptr->state.aggravate)) {
	/* Low-level monsters will find it difficult to locate the player. */
	if ((p_ptr->lev * 2 > r_ptr->level)
	    && (randint0(p_ptr->lev * 2 - r_ptr->level) != 0)) {
	    aware = FALSE;

	    /* Monster has no target */
	    if ((!m_ptr->ty) && (!m_ptr->tx))
		random = TRUE;
	}
    }


    /* Monsters can speak.  -originally by TY- */
    if ((rf_has(r_ptr->flags, RF_SPEAKING))
	&& (player_has_los_bold(m_ptr->fy, m_ptr->fx)) && (!m_ptr->monfear)
	&& (randint0(16) == 0)) {
	char m_name[80];
	char bravado[80];

	/* Acquire the monster name/poss */
	if (m_ptr->ml)
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

	/* Default name */
	else
	    strcpy(m_name, "It");

	if (!get_rnd_line("bravado.txt", bravado))
	    msg("%s %s", m_name, bravado);
    }

    /* Player ghosts may have a unique message they can say. */
    else if ((rf_has(r_ptr->flags, RF_PLAYER_GHOST))
	     && (player_has_los_bold(m_ptr->fy, m_ptr->fx))
	     && (ghost_string_type == 1) && (!ghost_has_spoken)
	     && (randint1(3) == 1)) {
	char m_name[80];

	/* 
	 * Acquire the monster name/poss.  The player ghost will 
	 * always be identified, to heighten the effect.
	 */
	monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

	msg("%s says: '%s'", m_name, ghost_string);
	ghost_has_spoken = TRUE;
    }

    /* Monsters may drop the 'wary of traps' state if they see the player */
    if ((aware) && (player_has_los_bold(m_ptr->fy, m_ptr->fx))) {
	if (randint0(4) == 0)
	    m_ptr->mflag &= ~(MFLAG_WARY);
    }

  /*** Ranged attacks ***/

    /* Extract the ranged attack probability. */
    chance = r_ptr->freq_ranged;

    /* Are we an unchanged shapeshifter? */
    if ((rsf_has(r_ptr->flags, RSF_SHAPECHANGE)) && (!m_ptr->schange))
	shape_rate =
	    assess_shapechange(cave_m_idx[m_ptr->fy][m_ptr->fx], m_ptr);

    /* Cannot use ranged attacks beyond maximum range. */
    if ((chance) && (m_ptr->cdis > MAX_RANGE))
	chance = 0;

    /* Cannot use ranged attacks when confused or not aware. */
    if ((chance) && ((m_ptr->confused) || (!aware)))
	chance = 0;

    /* Stunned monsters use ranged attacks half as often. */
    if ((chance) && (m_ptr->stunned))
	chance /= 2;

    /* Monster can use ranged attacks */
    if ((chance != 0) && (randint0(100) < chance)) {
	/* Pick a ranged attack */
	choice =
	    choose_ranged_attack(cave_m_idx[m_ptr->fy][m_ptr->fx], FALSE,
				 shape_rate);
    }

    /* Roll to use ranged attacks failed, but monster is an archer. */
    if ((choice == 0) && (rf_has(r_ptr->flags, RF_ARCHER))
	&& (!(m_ptr->confused)
	    && aware)) {
	/* Pick an archery attack (usually) */
	if ((randint0(8) != 0) && (m_ptr->cdis > 1))
	    choice =
		choose_ranged_attack(cave_m_idx[m_ptr->fy][m_ptr->fx], TRUE, 0);
    }

    /* Selected a ranged attack? */
    if (choice != 0) {
	/* Execute said attack */
	make_attack_ranged(m_ptr, choice);

	/* Hack - Kill the shapechange */
	if (choice != RSF_SHAPECHANGE) {
	    m_ptr->orig_idx = old_shape;
	    m_ptr->old_p_race = old_race;
	}

	/* End turn */
	return;
    }

  /*** Movement ***/

    /* Assume no movement */
    ty = 0;
    tx = 0;


    /* 
     * Innate semi-random movement.  Monsters adjacent to the character 
     * can always strike accurately at him (monster isn't confused).
     */
    if ((rf_has(r_ptr->flags, RF_RAND_50) || rf_has(r_ptr->flags, RF_RAND_25))
	&& (m_ptr->cdis > 1)) {
	int chance = 0;

	/* RAND_25 and RAND_50 are cumulative */
	if (rf_has(r_ptr->flags, RF_RAND_25)) {
	    chance += 25;
	    if (m_ptr->ml)
		rf_on(l_ptr->flags, RF_RAND_25);
	}
	if (rf_has(r_ptr->flags, RF_RAND_50)) {
	    chance += 50;
	    if (m_ptr->ml)
		rf_on(l_ptr->flags, RF_RAND_50);
	}

	/* Chance of moving randomly */
	if (randint0(100) < chance)
	    random = TRUE;
    }

    /* Monster is neutral */
    if (m_ptr->hostile == 0) {
	/* Already has a target */
	if ((m_ptr->ty) && (m_ptr->tx))
	    must_use_target = TRUE;

	/* Otherwise get a random one */
	else {
	    int tmp_y, tmp_x;

	    tmp_y = randint1(DUNGEON_HGT - 1);
	    tmp_x = randint1(DUNGEON_WID - 1);

	    while (!cave_exist_mon(r_ptr, tmp_y, tmp_x, FALSE)) {
		tmp_y = randint1(DUNGEON_HGT - 1);
		tmp_x = randint1(DUNGEON_WID - 1);
	    }

	    m_ptr->ty = tmp_y;
	    m_ptr->tx = tmp_x;
	}
    }

    /* Monsters near their leader will take the lead from him/her/it */
    if (m_ptr->group_leader) {
	monster_type *n_ptr = &m_list[m_ptr->group_leader];

	if ((!m_ptr->monfear)
	    && (distance(m_ptr->fy, m_ptr->fx, n_ptr->fy, n_ptr->fx) <
		scan_range)) {
	    m_ptr->ty = n_ptr->ty;
	    m_ptr->tx = n_ptr->tx;
	}
    }


    /* Monster cannot perceive the character */
    if ((!aware) && (!random)) {
	/* Monster has a known target */
	if ((m_ptr->ty) && (m_ptr->tx))
	    must_use_target = TRUE;

	/* Monster is just going to have to search at random */
	else
	    random = TRUE;
    }


  /*** Find a target to move to ***/

    /* Monster is genuinely confused */
    if ((m_ptr->confused) && (!(rf_has(r_ptr->flags, RF_NEVER_MOVE)))) {
	/* Choose any direction except five and zero */
	dir = randint0(8);

	/* Monster can try to wander into /anything/... */
	ty = m_ptr->fy + ddy_ddd[dir];
	tx = m_ptr->fx + ddx_ddd[dir];
    }

    /* Monster isn't confused, just moving semi-randomly */
    else if (random) {
	int start = randint0(8);
	bool dummy;

	/* Is the monster scared? */
	if ((!(rf_has(r_ptr->flags, RF_NEVER_MOVE)))
	    && ((m_ptr->min_range == FLEE_RANGE) || (m_ptr->monfear))) {
	    fear = TRUE;
	}

	/* Look at adjacent grids, starting at random. */
	for (i = start; i < 8 + start; i++) {
	    y = m_ptr->fy + ddy_ddd[i % 8];
	    x = m_ptr->fx + ddx_ddd[i % 8];

	    /* Accept first passable grid. */
	    if (cave_passable_mon(m_ptr, y, x, &dummy) != 0) {
		ty = y;
		tx = x;
		break;
	    }
	}

	/* No passable grids found */
	if ((ty == 0) && (tx == 0))
	    return;
    }

    /* Normal movement */
    else {
	/* Choose a pair of target grids, or cancel the move. */
	if (!get_move(m_ptr, &ty, &tx, &fear, must_use_target))
	    return;
    }

    /* Calculate the actual move.  Cancel move on failure to enter grid. */
    if (!make_move(m_ptr, &ty, &tx, fear, &bash))
	return;

    /* Change terrain, move the monster, handle secondary effects. */
    process_move(m_ptr, ty, tx, bash);

    /* End turn */
    return;
}


/**
 * Monster regeneration of HPs and mana, and recovery from all temporary 
 * conditions.
 *
 * This function is called a lot, and is therefore fairly expensive.
 */
static void recover_monster(monster_type * m_ptr, bool regen)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_lore *l_ptr = &l_list[m_ptr->r_idx];

    int frac;
    int scan_range = (MODE(SMALL_DEVICE) ? r_ptr->aaf / 2 : r_ptr->aaf);

    /* Handle stasis */
    if (m_ptr->stasis) {
	int d = 1;

	/* Sudden emergence.  Uniques get a bonus. */
	if (50 + r_ptr->level +
	    (rf_has(r_ptr->flags, RF_UNIQUE) ? r_ptr->level / 2 : 0) >
	    randint1(800)) {
	    m_ptr->stasis = 0;
	}

	/* Gradual emergence. */
	else if (m_ptr->stasis > d)
	    m_ptr->stasis -= d;
	else
	    m_ptr->stasis = 0;

	/* Visual note */
	if ((!m_ptr->stasis) && (m_ptr->ml)) {
	    char m_name[80];
	    char m_poss[80];

	    /* Acquire the monster name/poss */
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);
	    monster_desc(m_poss, sizeof(m_name), m_ptr, 0x22);

	    /* Dump a message */
	    msg("%s emerges from a Holding spell.", m_name);
	}

	/* If monster is still in stasis, it cannot do /anything/. */
	if (m_ptr->stasis)
	    return;

	/* Monster gets its bearings */
	else if (m_ptr->energy > p_ptr->energy)
	    m_ptr->energy = p_ptr->energy;
    }


    /* 
     * Monsters have a (small) chance to recover from the Black Breath; 
     * maybe they have some Athelas handy...
     */
    if ((m_ptr->black_breath) && (randint0(250 - r_ptr->level) == 0)) {
	m_ptr->black_breath = FALSE;
	if (m_ptr->ml) {
	    char m_name[80];

	    /* Acquire the monster name */
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

	    /* Dump a message */
	    msg("%s recovers from the Black Breath.", m_name);
	}
    }


    /* Every 100 game turns, regenerate monsters */
    if (regen) {
	/* Regenerate mana, if needed */
	if (m_ptr->mana < r_ptr->mana) {
	    /* 
	     * Mana comes back 1 point at a time, with probability
	     * based on maximum mana.
	     */
	    if (randint0(200) < (r_ptr->mana))
		m_ptr->mana++;

	    /* Do not over-regenerate */
	    if (m_ptr->mana > r_ptr->mana)
		m_ptr->mana = r_ptr->mana;

	    /* Fully healed -> flag minimum range for recalculation */
	    if (m_ptr->mana == r_ptr->mana)
		m_ptr->min_range = 0;
	}

	/* 
	 * Allow hp regeneration, if needed, unless suffering from 
	 * the Black Breath.
	 */
	if ((m_ptr->hp < m_ptr->maxhp) && (!m_ptr->black_breath)) {
	    /* Base regeneration */
	    frac = m_ptr->maxhp / 100;

	    /* Minimal regeneration rate */
	    if (!frac)
		frac = 1;

	    /* Some monsters regenerate quickly */
	    if (rf_has(r_ptr->flags, RF_REGENERATE))
		frac *= 2;

	    /* Regenerate */
	    m_ptr->hp += frac;

	    /* Do not over-regenerate */
	    if (m_ptr->hp > m_ptr->maxhp)
		m_ptr->hp = m_ptr->maxhp;

	    /* Fully healed -> flag minimum range for recalculation */
	    if (m_ptr->hp == m_ptr->maxhp)
		m_ptr->min_range = 0;
	}
    }


    /* Monster is sleeping, but character is within detection range */
    if ((m_ptr->csleep) && (m_ptr->cdis <= scan_range)) {
	/* Aggravated by the player */
	if (p_ptr->state.aggravate) {
	    /* Reset sleep counter */
	    m_ptr->csleep = 0;

	    /* Notice the "waking up" */
	    if (m_ptr->ml) {
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		/* Dump a message */
		msg("%s wakes up.", m_name);
	    }
	}

	/* See if monster notices player */
	else if (total_wakeup_chance >= (u32b)randint1(10000)) {
	    int d = 1;

	    /* Wake up faster near the player */
	    if (m_ptr->cdis < 50)
		d = (100 / m_ptr->cdis);

	    /* Still asleep */
	    if (m_ptr->csleep > d) {
		/* Monster's sleep is disturbed */
		m_ptr->csleep -= d;

		/* Notice the "not waking up" */
		if (m_ptr->ml) {
		    /* Hack -- Count the ignores */
		    if (l_ptr->ignore < MAX_UCHAR) {
			l_ptr->ignore++;
		    }
		}
	    }

	    /* Just woke up */
	    else {
		/* Reset sleep counter */
		m_ptr->csleep = 0;

		/* Notice the "waking up" */
		if (m_ptr->ml) {
		    char m_name[80];

		    /* Acquire the monster name */
		    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		    /* Dump a message */
		    msg("%s wakes up.", m_name);

		    /* Hack -- Count the wakings */
		    if (l_ptr->wake < MAX_UCHAR) {
			l_ptr->wake++;
		    }
		}

		/* Monster gets its bearings */
		if (m_ptr->energy > p_ptr->energy)
		    m_ptr->energy = p_ptr->energy;
	    }
	}
    }


    /* Chance for extra noise to wake up monsters in LOS */
    if ((m_ptr->csleep) && (add_wakeup_chance > 0)
	&& (player_has_los_bold(m_ptr->fy, m_ptr->fx))) {
	if (add_wakeup_chance > randint0(10000)) {
	    /* Disturb the monster quite a lot. */
	    int d = randint1(add_wakeup_chance / 40);

	    /* Still asleep */
	    if (m_ptr->csleep > d) {
		/* Monster's sleep is disturbed */
		m_ptr->csleep -= d;

		/* Notice */
		if (m_ptr->ml) {
		    char m_name[80];

		    /* Acquire the monster name */
		    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		    /* Warning */
		    msg("%s stirs.", m_name);
		}
	    }

	    /* Just woke up */
	    else {
		/* Reset sleep counter */
		m_ptr->csleep = 0;

		/* Notice the "waking up" */
		if (m_ptr->ml) {
		    char m_name[80];

		    /* Acquire the monster name */
		    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		    /* Dump a message */
		    msg("%s wakes up.", m_name);
		}

		/* Monster gets its bearings */
		if (m_ptr->energy > p_ptr->energy)
		    m_ptr->energy = p_ptr->energy;
	    }
	}
    }


    /* Recover from stuns */
    if (m_ptr->stunned) {
	int d = 1;

	/* Make a "saving throw" against stun. */
	if (randint0(330) < r_ptr->level + 10) {
	    /* Recover fully */
	    d = m_ptr->stunned;
	}

	/* Hack -- Recover from stun */
	if (m_ptr->stunned > d) {
	    /* Recover somewhat */
	    m_ptr->stunned -= d;
	}

	/* Fully recover */
	else {
	    /* Recover fully */
	    m_ptr->stunned = 0;

	    /* Message if visible */
	    if (m_ptr->ml) {
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		/* Dump a message */
		msg("%s is no longer stunned.", m_name);
	    }
	}
    }


    /* Recover from confusion */
    if (m_ptr->confused) {
	int d = randint1(r_ptr->level / 10 + 1);

	/* Still confused */
	if (m_ptr->confused > d) {
	    /* Reduce the confusion */
	    m_ptr->confused -= d;
	}

	/* Recovered */
	else {
	    /* No longer confused */
	    m_ptr->confused = 0;

	    /* Message if visible */
	    if (m_ptr->ml) {
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		/* Dump a message */
		msg("%s is no longer confused.", m_name);
	    }
	}
    }


    /* Return to original shape */
    if (m_ptr->schange) {
	int d = 1;

	/* Still changed */
	if (m_ptr->schange > d) {
	    /* Reduce the counter */
	    m_ptr->schange -= d;
	}

	/* Need to change back */
	else {
	    /* No longer changed */
	    m_ptr->schange = 0;

	    /* Message if visible */
	    if (m_ptr->ml) {
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

		/* Dump a message */
		msg("%s shimmers and changes!", m_name);
	    }

	    /* Restore the original shape */
	    m_ptr->r_idx = m_ptr->orig_idx;
	    m_ptr->orig_idx = 0;
	    m_ptr->p_race = m_ptr->old_p_race;
	    m_ptr->old_p_race = NON_RACIAL;

	    /* Hack - do a complete redraw -- unnecessary? */
	    p_ptr->redraw |= PR_MAP;
	}
    }


    /* Recover courage */
    if (m_ptr->monfear) {
	/* Random recovery from fear */
	int d = randint1(r_ptr->level / 20 + 1);

	/* Still afraid */
	if (m_ptr->monfear > d) {
	    /* Reduce the fear */
	    m_ptr->monfear -= d;
	}

	/* Recover from fear, take note if seen */
	else {
	    /* No longer afraid */
	    m_ptr->monfear = 0;

	    /* Recalculate minimum range immediately */
	    find_range(m_ptr);

	    /* Visual note - only if monster isn't terrified */
	    if ((m_ptr->ml) && (m_ptr->min_range != FLEE_RANGE)) {
		char m_name[80];
		char m_poss[80];

		/* Acquire the monster name/poss */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);
		monster_desc(m_poss, sizeof(m_name), m_ptr, 0x22);

		/* Dump a message */
		msg("%s recovers %s courage.", m_name, m_poss);
	    }
	}
    }


    /* 
     * Handle significantly hasted or slowed creatures.  Random variations 
     * are not large enough to activate this code.
     */
    if ((m_ptr->mspeed > r_ptr->speed + 4)
	|| (m_ptr->mspeed < r_ptr->speed - 4)) {
	int speedup_chance = 67 - (2 * r_ptr->level / 3);

	/* 
	 * 1.5% - 3.0% chance (depending on level) that slowed monsters
	 * will return to normal speed.
	 */
	if ((m_ptr->mspeed < r_ptr->speed)
	    && (randint0(67) == speedup_chance)) {
	    m_ptr->mspeed = r_ptr->speed;

	    /* Visual note */
	    if (m_ptr->ml) {
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0);

		/* Message. */
		msg("%s is no longer slowed.", m_name);
	    }
	}

	/* 1% chance that hasted monsters will return to normal speed. */
	else if ((m_ptr->mspeed > r_ptr->speed) && (randint0(100) == 0)) {
	    m_ptr->mspeed = r_ptr->speed;

	    /* Visual note */
	    if (m_ptr->ml) {
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0);

		/* Message. */
		msg("%s is no longer hasted.", m_name);
	    }
	}
    }


    /* Hack -- Update the health bar (always) */
    if (p_ptr->health_who == cave_m_idx[m_ptr->fy][m_ptr->fx])
	p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);
}


/**
 * Process all living monsters, once per game turn.
 *
 * Scan through the list of all living monsters, (backwards, so we can 
 * excise any "freshly dead" monsters).
 *
 * Every ten game turns, allow monsters to recover from temporary con-
 * ditions.  Every 100 game turns, regenerate monsters.  Give energy to 
 * each monster, and allow fully energized monsters to take their turns.
 *
 * This function and its children are responsible for at least a third of 
 * the processor time in normal situations.  If the character is resting, 
 * this may rise substantially.
 */
void process_monsters(byte minimum_energy)
{
    int i;
    monster_type *m_ptr;

    /* Only process some things every so often */
    bool recover = FALSE;
    bool regen = FALSE;

    /* Time out temporary conditions every ten game turns */
    if (turn % 10 == 0) {
	recover = TRUE;

	/* Regenerate hitpoints and mana every 100 game turns */
	if (turn % 100 == 0)
	    regen = TRUE;
    }

    /* Process the monsters (backwards) */
    for (i = m_max - 1; i >= 1; i--) {
	/* Player is dead or leaving the current level */
	if (p_ptr->leaving)
	    break;

	/* Access the monster */
	m_ptr = &m_list[i];

	/* Ignore dead monsters */
	if (!m_ptr->r_idx)
	    continue;

	/* Ignore monsters that have already been handled */
	if (m_ptr->moved)
	    continue;

	/* Leave monsters without enough energy for later */
	if (m_ptr->energy < minimum_energy)
	    continue;

	/* Prevent reprocessing */
	m_ptr->moved = TRUE;

	/* Handle temporary monster attributes every ten game turns */
	if (recover)
	    recover_monster(m_ptr, regen);

	/* Give this monster some energy */
	m_ptr->energy += extract_energy[m_ptr->mspeed];

	/* End the turn of monsters without enough energy to move */
	if (m_ptr->energy < 100)
	    continue;

	/* Use up some energy */
	m_ptr->energy -= 100;

	/* Let the monster take its turn */
	process_monster(m_ptr);
    }
}


/**
 * Clear 'moved' status from all monsters.
 * 
 * Clear noise if appropriate.
 */
void reset_monsters(void)
{
    int i;
    monster_type *m_ptr;

    /* Process the monsters (backwards) */
    for (i = m_max - 1; i >= 1; i--) {
	/* Access the monster */
	m_ptr = &m_list[i];

	/* Monster is ready to go again */
	m_ptr->moved = FALSE;
    }
    /* Clear the current noise after it is used to wake up monsters */ 
    if (turn % 10 == 0) {
	total_wakeup_chance = 0L;
	add_wakeup_chance = 0;
    }
}
