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
#include "cave.h"
#include "history.h"
#include "monster.h"
#include "player.h"
#include "target.h"
#include "trap.h"


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
    if (rf_has(r_ptr->flags, RF_MULTIPLY))
	num_repro--;

    /* Hack -- remove target monster */
    if (target_get_monster() == i) target_set_monster(0);

    /* Hack -- remove tracked monster */
    if (p_ptr->health_who == i)
	health_track(0);


    /* Monster is gone */
    cave_m_idx[y][x] = 0;

    /* Total Hack -- If the monster was a player ghost, remove it from the
     * monster memory, ensure that it never appears again, clear its bones
     * file selector and allow the next ghost to speak. */
    if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
	l_ptr->sights = 0;
	l_ptr->deaths = 0;
	l_ptr->pkills = 0;
	l_ptr->tkills = 0;
	bones_selector = 0;
	ghost_has_spoken = FALSE;
	r_ptr->rarity = 0;
    }

    /* Delete objects */
    for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
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
    (void) WIPE(m_ptr, monster_type);

    /* Count monsters */
    m_cnt--;


    /* Visual update */
    light_spot(y, x);
}


/**
 * Delete the monster, if any, at a given location
 */
void delete_monster(int y, int x)
{
    /* Paranoia */
    if (!in_bounds(y, x))
	return;

    /* Delete the monster (if any) */
    if (cave_m_idx[y][x] > 0)
	delete_monster_idx(cave_m_idx[y][x]);
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
    if (i1 == i2)
	return;


    /* Old monster */
    m_ptr = &m_list[i1];

    /* Location */
    y = m_ptr->fy;
    x = m_ptr->fx;

    /* Update the cave */
    cave_m_idx[y][x] = i2;

    /* Repair objects being carried by monster */
    for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr;

	/* Acquire object */
	o_ptr = &o_list[this_o_idx];

	/* Acquire next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Reset monster pointer */
	o_ptr->held_m_idx = i2;
    }

    /* Hack -- Update the target */
    if (target_get_monster() == i1) target_set_monster(i2);

    /* Hack -- Update the health bar */
    if (p_ptr->health_who == i1)
	p_ptr->health_who = i2;

    /* Hack -- move monster */
    (void) COPY(&m_list[i2], &m_list[i1], monster_type);

    /* Hack -- wipe hole */
    (void) WIPE(&m_list[i1], monster_type);
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
    if (size)
	msg("Compacting monsters...");


    /* Compact at least 'size' objects */
    for (num = 0, cnt = 1; num < size; cnt++) {
	/* Get more vicious each iteration */
	cur_lev = 5 * cnt;

	/* Get closer each iteration */
	cur_dis = 5 * (20 - cnt);

	j = 0;

	/* Check all the monsters */
	for (i = 1; i < m_max; i++) {
	    monster_type *m_ptr = &m_list[i];

	    monster_race *r_ptr = &r_info[m_ptr->r_idx];

	    /* Paranoia -- skip "dead" monsters */
	    if (!m_ptr->r_idx) {
		if (j++ >= size)
		    return;
		else
		    continue;
	    }

	    /* Hack -- High level monsters start out "immune" */
	    if (r_ptr->level > cur_lev)
		continue;

	    /* Ignore nearby monsters */
	    if ((cur_dis > 0) && (m_ptr->cdis < cur_dis))
		continue;

	    /* Saving throw chance */
	    chance = 90;

	    /* Only compact "Quest" Monsters in emergencies */
	    if ((rf_has(r_ptr->flags, RF_QUESTOR)) && (cnt < 1000))
		chance = 100;

	    /* Try not to compact Unique Monsters */
	    if (rf_has(r_ptr->flags, RF_UNIQUE))
		chance = 99;

	    /* All monsters get a saving throw */
	    if (randint0(100) < chance)
		continue;

	    /* Delete the monster */
	    delete_monster_idx(i);

	    /* Count the monster */
	    num++;
	}
    }


    /* Excise dead monsters (backwards!) */
    for (i = m_max - 1; i >= 1; i--) {
	/* Get the i'th monster */
	monster_type *m_ptr = &m_list[i];

	/* Skip real monsters */
	if (m_ptr->r_idx)
	    continue;

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
    for (i = m_max - 1; i >= 1; i--) {
	monster_type *m_ptr = &m_list[i];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	/* Skip dead monsters */
	if (!m_ptr->r_idx)
	    continue;

	/* Total Hack -- Clear player ghost information. */
	if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
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
	(void) WIPE(m_ptr, monster_type);
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
    if (m_max < z_info->m_max) {
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
    for (i = 1; i < m_max; i++) {
	monster_type *m_ptr;

	/* Acquire monster */
	m_ptr = &m_list[i];

	/* Skip live monsters */
	if (m_ptr->r_idx)
	    continue;

	/* Count monsters */
	m_cnt++;

	/* Use this monster */
	return (i);
    }


    /* Warn the player (except during dungeon creation) */
    if (character_dungeon)
	msg("Too many monsters!");

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
    for (i = 0; i < alloc_race_size; i++) {
	/* Get the entry */
	alloc_entry *entry = &alloc_race_table[i];

	/* Accept monsters which pass the restriction, if any */
	if (!get_mon_num_hook || (*get_mon_num_hook) (entry->index)) {
	    /* Accept this monster */
	    entry->prob2 = entry->prob1;
	}

	/* Do not use this monster */
	else {
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
	if (randint0(NASTY_MON) == 0) 
	{
	    /* Pick a level bonus */
	    d = level / 10 + 1;

	    /* Boost the level */
	    temp_level += ((d < 5) ? d : 5);

	    /* Occasional second boost */
	    if (randint0(NASTY_MON) == 0) 
	    {
		/* Pick a level bonus */
		d = level / 10 + 1;

		/* Boost the level */
		temp_level += ((d < 5) ? d : 5);
	    }
	}
	/* Hard mode wilderness is hard */
	if (OPT(hard_mode) && (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE))
	{
	    temp_level += 5;
	}
    }


    /* Try hard to find a suitable monster */
    while (TRUE) {
	/* Reset sum of final monster probabilities. */
	alloc_race_total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_race_size; i++) {
	    /* Assume no probability */
	    table[i].prob3 = 0;

	    /* Ignore illegal monsters */
	    if (!table[i].prob2)
		continue;

	    /* Monsters are sorted by depth */
	    if (table[i].level > temp_level)
		continue;

	    /* Hack -- No town monsters in dungeon */
	    if ((p_ptr->depth != 0) && (table[i].level < 1))
		continue;

	    /* Get the monster index */
	    r_idx = table[i].index;

	    /* Get the actual race */
	    r_ptr = &r_info[r_idx];

	    /* Hack -- some monsters are unique */
	    if ((rf_has(r_ptr->flags, RF_UNIQUE))
		&& (r_ptr->cur_num >= r_ptr->max_num))
		continue;

	    /* Forced-depth monsters only appear at their level. */
	    if ((rf_has(r_ptr->flags, RF_FORCE_DEPTH))
		&& (r_ptr->level != p_ptr->depth))
		continue;

	    /* Hack - handle dungeon specific -NRM- */
	    if ((rf_has(r_ptr->flags, RF_RUDH))
		&& (stage_map[p_ptr->stage][LOCALITY] != AMON_RUDH))
		continue;

	    if ((rf_has(r_ptr->flags, RF_NARGOTHROND))
		&& (stage_map[p_ptr->stage][LOCALITY] != NARGOTHROND))
		continue;

	    if ((rf_has(r_ptr->flags, RF_DUNGORTHEB))
		&& (stage_map[p_ptr->stage][LOCALITY] != NAN_DUNGORTHEB))
		continue;

	    if ((rf_has(r_ptr->flags, RF_GAURHOTH))
		&& (stage_map[p_ptr->stage][LOCALITY] != TOL_IN_GAURHOTH))
		continue;

	    if ((rf_has(r_ptr->flags, RF_ANGBAND))
		&& (stage_map[p_ptr->stage][LOCALITY] != ANGBAND))
		continue;

	    /* Hack - dungeon-only monsters */
	    if ((rf_has(r_ptr->flags, RF_DUNGEON))
		&& (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE))
		continue;

	    /* Hack - choose flying monsters for mountaintop */
	    if ((stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP)
		&& !(rf_has(r_ptr->flags, RF_FLYING)))
		continue;

	    /* Accept */
	    table[i].prob3 = table[i].prob2;

	    /* Now modifications for locality etc follow -NRM- */

	    /* Nan Dungortheb is spiderland, bad for humans and humanoids */
	    if (stage_map[p_ptr->stage][LOCALITY] == NAN_DUNGORTHEB) {
		if (r_ptr->d_char == 'S')
		    table[i].prob3 *= 5;
		if ((r_ptr->d_char == 'p') || (r_ptr->d_char == 'h'))
		    table[i].prob3 /= 3;
	    }

	    /* Tol-In-Gaurhoth is full of wolves and undead */
	    if (stage_map[p_ptr->stage][LOCALITY] == TOL_IN_GAURHOTH) {
		if (r_ptr->d_char == 'C')
		    table[i].prob3 *= 4;
		else if (rf_has(r_ptr->flags, RF_UNDEAD))
		    table[i].prob3 *= 2;
	    }

	    /* Most animals don't like desert and mountains */
	    if ((stage_map[p_ptr->stage][STAGE_TYPE] == DESERT)
		|| (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAIN)) {
		if ((r_ptr->d_char == 'R') || (r_ptr->d_char == 'J')
		    || (r_ptr->d_char == 'c'))
		    table[i].prob3 *= 2;
		else if (rf_has(r_ptr->flags, RF_ANIMAL))
		    table[i].prob3 /= 2;
	    }

	    /* Most animals do like forest */
	    if (stage_map[p_ptr->stage][STAGE_TYPE] == FOREST) {
		if (r_ptr->d_char == 'R')
		    table[i].prob3 /= 2;
		else if ((rf_has(r_ptr->flags, RF_ANIMAL))
			 && (r_ptr->d_char != 'Z'))
		    table[i].prob3 *= 2;
	    }



	    /* Keep low-level monsters rare */
	    if (table[i].level < depth_rare)
		table[i].prob3 /= 4;
	    if (table[i].level < depth_very_rare)
		table[i].prob3 /= 4;

	    /* Sum up probabilities */
	    alloc_race_total += table[i].prob3;
	}

	/* No legal monsters */
	if (alloc_race_total == 0) {
	    failure++;

	    if (failure == 1) {
		/* Try relaxing the level restrictions */
		if (p_ptr->themed_level)
		    temp_level += 20;
		else
		    temp_level += 10;
	    } else {
		/* Our monster restrictions are too stringent. */
		return (0);
	    }
	}

	/* Success */
	else
	    break;
    }

    /* Pick a monster */
    value = randint0(alloc_race_total);

    /* Find the monster */
    for (i = 0; i < alloc_race_size; i++) {
	/* Found the entry */
	if (value < table[i].prob3)
	    break;

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
    if (!alloc_race_total)
	return (get_mon_num(level));


    /* Pick a monster */
    value = randint0(alloc_race_total);

    /* Find the monster */
    for (i = 0; i < alloc_race_size; i++) {
	/* Found the entry */
	if (value < table[i].prob3)
	    break;

	/* Decrement */
	value = value - table[i].prob3;
    }

    /* Result */
    return (table[i].index);
}


/**
 * Mega-hack - Fix plural names of monsters
 *
 * Taken from PernAngband via EY, modified to fit NPP monster list
 *
 * Note: It should handle all regular Angband monsters.
 *
 * TODO: Specify monster name plurals in monster.txt instead.
 */
void plural_aux(char *name, size_t max)
{
	int name_len = strlen(name);

	if (strstr(name, " of "))
	{
		char *aider = strstr(name, " of ");
		char dummy[80];
		int i = 0;
		char *ctr = name;

		while (ctr < aider)
		{
			dummy[i] = *ctr;
			ctr++;
			i++;
		}

		if (dummy[i - 1] == 's')
		{
			strcpy (&(dummy[i]), "es");
			i++;
		}
		else
		{
			strcpy (&(dummy[i]), "s");
		}

		strcpy(&(dummy[i + 1]), aider);
		my_strcpy(name, dummy, max);
	}
	else if ((strstr(name, "coins")) || (strstr(name, "gems")))
	{
		char dummy[80];
		strcpy (dummy, "Piles of c");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}

	else if (strstr(name, "Greater Servant of"))
	{
		char dummy[80];
		strcpy (dummy, "Greater Servants of ");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}
	else if (strstr(name, "Lesser Servant of"))
	{
		char dummy[80];
		strcpy (dummy, "Greater Servants of ");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}
	else if (strstr(name, "Servant of"))
	{
		char dummy[80];
		strcpy (dummy, "Servants of ");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}
	else if (strstr(name, "Great Wyrm"))
	{
		char dummy[80];
		strcpy (dummy, "Great Wyrms ");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}
	else if (strstr(name, "Spawn of"))
	{
		char dummy[80];
		strcpy (dummy, "Spawn of ");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}
	else if (strstr(name, "Descendant of"))
	{
		char dummy[80];
		strcpy (dummy, "Descendant of ");
		my_strcat (dummy, &(name[1]), sizeof(dummy));
		my_strcpy (name, dummy, max);
		return;
	}
	else if ((strstr(name, "Manes")) || (name[name_len-1] == 'u') || (strstr(name, "Yeti")) ||
		(streq(&(name[name_len-2]), "ua")) || (streq(&(name[name_len-3]), "nee")) ||
		(streq(&(name[name_len-4]), "idhe")))
	{
		return;
	}
	else if (name[name_len-1] == 'y')
	{
		strcpy(&(name[name_len - 1]), "ies");
	}
	else if (streq(&(name[name_len - 4]), "ouse"))
	{
		strcpy (&(name[name_len - 4]), "ice");
	}
	else if (streq(&(name[name_len - 4]), "lung"))
	{
		strcpy (&(name[name_len - 4]), "lungen");
	}
	else if (streq(&(name[name_len - 3]), "sus"))
	{
		strcpy (&(name[name_len - 3]), "si");
	}
	else if (streq(&(name[name_len - 4]), "star"))
	{
		strcpy (&(name[name_len - 4]), "stari");
	}
	else if (streq(&(name[name_len - 3]), "aia"))
	{
		strcpy (&(name[name_len - 3]), "aiar");
	}
	else if (streq(&(name[name_len - 3]), "inu"))
	{
		strcpy (&(name[name_len - 3]), "inur");
	}
	else if (streq(&(name[name_len - 5]), "culus"))
	{
		strcpy (&(name[name_len - 5]), "culi");
	}
	else if (streq(&(name[name_len - 4]), "sman"))
	{
		strcpy (&(name[name_len - 4]), "smen");
	}
	else if (streq(&(name[name_len - 4]), "lman"))
	{
		strcpy (&(name[name_len - 4]), "lmen");
	}
	else if (streq(&(name[name_len - 2]), "ex"))
	{
		strcpy (&(name[name_len - 2]), "ices");
	}
	else if ((name[name_len - 1] == 'f') && (!streq(&(name[name_len - 2]), "ff")))
	{
		strcpy (&(name[name_len - 1]), "ves");
	}
	else if (((streq(&(name[name_len - 2]), "ch")) || (name[name_len - 1] == 's')) &&
			(!streq(&(name[name_len - 5]), "iarch")))
	{
		strcpy (&(name[name_len]), "es");
	}
	else
	{
		strcpy (&(name[name_len]), "s");
	}
}


/**
 * Helper function for display monlist.  Prints the number of creatures,
 * followed by either a singular or plural version of the race name as
 * appropriate.
 */
static void get_mon_name(char *output_name, size_t max,
		const monster_race *r_ptr, int num)
{
	char race_name[80];

	assert(r_ptr);

	my_strcpy(race_name, r_ptr->name, sizeof(race_name));

	/* Unique names don't have a number */
	if (rf_has(r_ptr->flags, RF_UNIQUE))
		my_strcpy(output_name, "[U] ", max);

	/* Normal races*/
	else {
		my_strcpy(output_name, format("%3d ", num), max);

		/* Make it plural, if needed. */
		if (num > 1)
			plural_aux(race_name, sizeof(race_name));
	}

	/* Mix the quantity and the header. */
	my_strcat(output_name, race_name, max);
}


/*
 * Monster data for the visible monster list
 */
typedef struct
{
	u16b count;		/* total number of this type visible */
	u16b asleep;		/* number asleep (not in LOS) */
	u16b neutral;		/* number neutral (not in LOS) */
	u16b los;		/* number in LOS */
	u16b los_asleep;	/* number asleep and in LOS */
	u16b los_neutral;	/* number neutral and in LOS */
	byte attr; /* attr to use for drawing */
} monster_vis;

/*
 * Display visible monsters in a window
 */
void display_monlist(void)
{
	int ii;
	size_t i, j, k;
	int max;
	int line = 1, x = 0;
	int cur_x;
	unsigned total_count = 0, disp_count = 0, type_count = 0, los_count = 0;

	byte attr;

	char m_name[80];
	char buf[80];

	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_race *r2_ptr;

	monster_vis *list;

	u16b *order;

	bool in_term = (Term != angband_term[0]);

	/* Hallucination is weird */
	if (p_ptr->timed[TMD_IMAGE]) {
		if (in_term)
			clear_from(0);
		Term_gotoxy(0, 0);
		text_out_to_screen(TERM_ORANGE,
			"Your hallucinations are too wild to see things clearly.");

		return;
	}

	/* Clear the term if in a subwindow, set x otherwise */
	if (in_term) {
		clear_from(0);
		max = Term->hgt - 1;
	}
	else {
		x = 13;
		max = Term->hgt - 2;
	}

	/* Allocate the primary array */
	list = C_ZNEW(z_info->r_max, monster_vis);

	/* Scan the list of monsters on the level */
	for (ii = 1; ii < m_max; ii++) {
		monster_vis *v;

		m_ptr = &m_list[ii];
		r_ptr = &r_info[m_ptr->r_idx];
		
		/* Only consider visible, known monsters */
		if (!m_ptr->ml) continue;
		
		/* Take a pointer to this monster visibility entry */
		v = &list[m_ptr->r_idx];

		/* Note each monster type and save its display attr (color) */
		if (!v->count) type_count++;
		if (!v->attr) v->attr = m_ptr->attr ? m_ptr->attr : r_ptr->x_attr;

		/* Check for LOS
		 * Hack - we should use (m_ptr->mflag & (MFLAG_VIEW)) here,
		 * but this does not catch monsters detected by ESP which are
		 * targetable, so we cheat and use projectable() instead
		 */
		if (projectable(p_ptr->py, p_ptr->px, m_ptr->fy, m_ptr->fx,
				PROJECT_CHCK))
		{
			/* Increment the total number of in-LOS monsters */
			los_count++;

			/* Increment the LOS count for this monster type */
			v->los++;
			
			/* Check if asleep or neutral and increment */
			if (m_ptr->hostile >= 0)
			    v->los_neutral++;
			else if (m_ptr->csleep)
			    v->los_asleep++;
		}
		/* Not in LOS so increment if asleep */
		else 
		{
		    if (m_ptr->hostile >= 0) v->neutral++;
		    else if (m_ptr->csleep) v->asleep++;
		}

		/* Bump the count for this race, and the total count */
		v->count++;
		total_count++;
	}

	/* Note no visible monsters at all */
	if (!total_count)
	{
		/* Clear display and print note */
		c_prt(TERM_SLATE, "You see no monsters.", 0, 0);
		if (!in_term)
		    Term_addstr(-1, TERM_WHITE, "  (Press any key to continue.)");

		/* Free up memory */
		FREE(list);

		/* Done */
		return;
	}

	/* Allocate the secondary array */
	order = C_ZNEW(type_count, u16b);

	/* Sort, because we cannot rely on monster.txt being ordered */

	/* Populate the ordered array, starting at 1 to ignore @ */
	for (i = 1; i < z_info->r_max; i++)
	{
		/* No monsters of this race are visible */
		if (!list[i].count) continue;

		/* Get the monster info */
		r_ptr = &r_info[i];

		/* Fit this monster into the sorted array */
		for (j = 0; j < type_count; j++)
		{
			/* If we get to the end of the list, put this one in */
			if (!order[j])
			{
				order[j] = i;
				break;
			}

			/* Get the monster info for comparison */
			r2_ptr = &r_info[order[j]];

			/* Monsters are sorted by depth */
			if (r_ptr->level > r2_ptr->level)
			{
				/* Move weaker monsters down the array */
				for (k = type_count - 1; k > j; k--)
				{
					order[k] = order[k - 1];
				}

				/* Put current monster in the right place */
				order[j] = i;
				break;
			}
		}
	}

	/* Message for monsters in LOS - even if there are none */
	if (!los_count) prt(format("You can see no monsters."), 0, 0);
	else prt(format("You can see %d monster%s", los_count, (los_count == 1
		? ":" : "s:")), 0, 0);

	/* Print out in-LOS monsters in descending order */
	for (i = 0; (i < type_count) && (line < max); i++)
	{
		/* Skip if there are none of these in LOS */
		if (!list[order[i]].los) continue;

		/* Reset position */
		cur_x = x;

		/* Note that these have been displayed */
		disp_count += list[order[i]].los;

		/* Get monster race and name */
		r_ptr = &r_info[order[i]];
		get_mon_name(m_name, sizeof(m_name), r_ptr, list[order[i]].los);

		/* Display uniques in a special colour */
		if (rf_has(r_ptr->flags, RF_UNIQUE))
			attr = TERM_VIOLET;
		else if (r_ptr->level > p_ptr->depth)
			attr = TERM_RED;
		else
			attr = TERM_WHITE;

		/* Build the monster name */
		if (list[order[i]].los == 1)
		{
		    if (list[order[i]].los_asleep == 1)
			strnfmt(buf, sizeof(buf), "%s (asleep) ", m_name);
		    else if (list[order[i]].los_neutral == 1)
			strnfmt(buf, sizeof(buf), "%s (neutral) ", m_name);
		    else
			strnfmt(buf, sizeof(buf), "%s ", m_name);
		    
		}
		else
		{
		    if (list[order[i]].los_asleep == 0)
		    {
			if (list[order[i]].los_neutral == 0)
			    strnfmt(buf, sizeof(buf), "%s", m_name);
			else
			    strnfmt(buf, sizeof(buf), "%s (%d neutral) ", 
				    m_name, list[order[i]].los_neutral);
		    }
		    else
		    {
			if (list[order[i]].los_neutral == 0)
			    strnfmt(buf, sizeof(buf), "%s (%d asleep) ", 
				    m_name, list[order[i]].los_asleep);
			else
			    strnfmt(buf, sizeof(buf), 
				    "%s (%d asleep, %d neutral) ", m_name,
				    list[order[i]].los_asleep,
				    list[order[i]].los_neutral);
		    }
		}

		/* Display the pict */
		if ((tile_width == 1) && (tile_height == 1)) {
	        Term_putch(cur_x++, line, list[order[i]].attr, r_ptr->x_char);
			Term_putch(cur_x++, line, TERM_WHITE, L' ');
		}

		/* Print and bump line counter */
		c_prt(attr, buf, line, cur_x);
		line++;

		/* Page wrap */
		if (!in_term && (line == max) && disp_count != total_count) {
			prt("-- more --", line, x);
			anykey();

			/* Clear the screen */
			for (line = 1; line <= max; line++)
				prt("", line, 0);

			/* Reprint Message */
			prt(format("You can see %d monster%s",
				los_count, (los_count > 0 ? (los_count == 1 ?
				":" : "s:") : "s.")), 0, 0);

			/* Reset */
			line = 1;
		}
	}

	/* Message for monsters outside LOS, if there are any */
	if (total_count > los_count) {
		/* Leave a blank line */
		line++;

		prt(format("You are aware of %d %smonster%s",
		(total_count - los_count), (los_count > 0 ? "other " : ""),
		((total_count - los_count) == 1 ? ":" : "s:")), line++, 0);
	}

	/* Print out non-LOS monsters in descending order */
	for (i = 0; (i < type_count) && (line < max); i++) {
		int out_of_los = list[order[i]].count - list[order[i]].los;

		/* Skip if there are none of these out of LOS */
		if (list[order[i]].count == list[order[i]].los) continue;

		/* Reset position */
		cur_x = x;

		/* Note that these have been displayed */
		disp_count += out_of_los;

		/* Get monster race and name */
		r_ptr = &r_info[order[i]];
		get_mon_name(m_name, sizeof(m_name), r_ptr, out_of_los);

		/* Display uniques in a special colour */
		if (rf_has(r_ptr->flags, RF_UNIQUE))
			attr = TERM_VIOLET;
		else if (r_ptr->level > p_ptr->depth)
			attr = TERM_RED;
		else
			attr = TERM_WHITE;

		/* Build the monster name */
		if (out_of_los == 1)
		{
		    if (list[order[i]].asleep == 1)
			strnfmt(buf, sizeof(buf), "%s (asleep) ", m_name);
		    else if (list[order[i]].neutral == 1)
			strnfmt(buf, sizeof(buf), "%s (neutral) ", m_name);
		    else
			strnfmt(buf, sizeof(buf), "%s ", m_name);
		    
		}
		else
		{
		    if (list[order[i]].asleep == 0)
		    {
			if (list[order[i]].neutral == 0)
			    strnfmt(buf, sizeof(buf), "%s", m_name);
			else
			    strnfmt(buf, sizeof(buf), "%s (%d neutral) ", 
				    m_name, list[order[i]].neutral);
		    }
		    else
		    {
			if (list[order[i]].neutral == 0)
			    strnfmt(buf, sizeof(buf), "%s (%d asleep) ", 
				    m_name, list[order[i]].asleep);
			else
			    strnfmt(buf, sizeof(buf), 
				    "%s (%d asleep, %d neutral) ", m_name,
				    list[order[i]].asleep,
				    list[order[i]].neutral);
		    }
		}
		
		/* Display the pict */
		if ((tile_width == 1) && (tile_height == 1)) {
	        Term_putch(cur_x++, line, list[order[i]].attr, r_ptr->x_char);
			Term_putch(cur_x++, line, TERM_WHITE, L' ');
		}

		/* Print and bump line counter */
		c_prt(attr, buf, line, cur_x);
		line++;

		/* Page wrap */
		if (!in_term && (line == max) && disp_count != total_count) {
			prt("-- more --", line, x);
			anykey();

			/* Clear the screen */
			for (line = 1; line <= max; line++)
				prt("", line, 0);

			/* Reprint Message */
			prt(format("You are aware of %d %smonster%s",
				(total_count - los_count), (los_count > 0 ?
				"other " : ""), ((total_count - los_count) > 0
				? ((total_count - los_count) == 1 ? ":" : "s:")
				: "s.")), 0, 0);

			/* Reset */
			line = 1;
		}
	}


	/* Print "and others" message if we've run out of space */
	if (disp_count != total_count) {
	    strnfmt(buf, sizeof buf, "  ...and %d others.",
		    total_count - disp_count);
	    c_prt(TERM_WHITE, buf, line, x);
	}

	/* Otherwise clear a line at the end, for main-term display */
	else
	    prt("", line, x);

	if (!in_term)
	    Term_addstr(-1, TERM_WHITE, "  (Press any key to continue.)");
	
	/* Free the arrays */
	FREE(list);
	FREE(order);
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
 *   - 0x100 --> Capitalise monster name
 *
 * Useful Modes:
 *   - 0x00 --> Full nominative name ("the kobold") or "it"
 *   - 0x04 --> Full nominative name ("the kobold") or "something"
 *   - 0x80 --> Genocide resistance name ("the kobold")
 *   - 0x88 --> Killing name ("a kobold")
 *   - 0x22 --> Possessive, genderized if visable ("his") or "its"
 *   - 0x23 --> Reflexive, genderized if visable ("himself") or "itself"
 */
void monster_desc(char *desc, size_t max, monster_type * m_ptr, int mode)
{
    const char *res;

    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    const char *name = r_ptr->name;
    char undead_name[40] = "oops";

    bool seen, pron;

    /* Can we "see" it (forced, or not hidden + visible) */
    seen = ((mode & (0x80)) || (!(mode & (0x40)) && m_ptr->ml));

    /* Sexed Pronouns (seen and forced, or unseen and allowed) */
    pron = ((seen && (mode & (0x20))) || (!seen && (mode & (0x10))));


    /* First, try using pronouns, or describing hidden monsters */
    if (!seen || pron) {
	/* an encoding of the monster "sex" */
	int kind = 0x00;

	/* Extract the gender (if applicable) */
	if (rf_has(r_ptr->flags, RF_FEMALE))
	    kind = 0x20;
	else if (rf_has(r_ptr->flags, RF_MALE))
	    kind = 0x10;

	/* Ignore the gender (if desired) */
	if (!m_ptr || !pron)
	    kind = 0x00;


	/* Assume simple result */
	res = "it";

	/* Brute force: split on the possibilities */
	switch (kind + (mode & 0x07)) {
	    /* Neuter, or unknown */
	case 0x00:
	    res = "it";
	    break;
	case 0x01:
	    res = "it";
	    break;
	case 0x02:
	    res = "its";
	    break;
	case 0x03:
	    res = "itself";
	    break;
	case 0x04:
	    res = "something";
	    break;
	case 0x05:
	    res = "something";
	    break;
	case 0x06:
	    res = "something's";
	    break;
	case 0x07:
	    res = "itself";
	    break;

	    /* Male (assume human if vague) */
	case 0x10:
	    res = "he";
	    break;
	case 0x11:
	    res = "him";
	    break;
	case 0x12:
	    res = "his";
	    break;
	case 0x13:
	    res = "himself";
	    break;
	case 0x14:
	    res = "someone";
	    break;
	case 0x15:
	    res = "someone";
	    break;
	case 0x16:
	    res = "someone's";
	    break;
	case 0x17:
	    res = "himself";
	    break;

	    /* Female (assume human if vague) */
	case 0x20:
	    res = "she";
	    break;
	case 0x21:
	    res = "her";
	    break;
	case 0x22:
	    res = "her";
	    break;
	case 0x23:
	    res = "herself";
	    break;
	case 0x24:
	    res = "someone";
	    break;
	case 0x25:
	    res = "someone";
	    break;
	case 0x26:
	    res = "someone's";
	    break;
	case 0x27:
	    res = "herself";
	    break;
	}

	/* Copy the result */
	my_strcpy(desc, res, max);
    }


    /* Handle visible monsters, "reflexive" request */
    else if ((mode & 0x02) && (mode & 0x01)) {
	/* The monster is visible, so use its gender */
	if (rf_has(r_ptr->flags, RF_FEMALE))
	    my_strcpy(desc, "herself", max);
	else if (rf_has(r_ptr->flags, RF_MALE))
	    my_strcpy(desc, "himself", max);
	else
	    my_strcpy(desc, "itself", max);
    }


    /* Handle all other visible monster requests */
    else {
	const char *race_name = NULL;

	/* Get a racial prefix if necessary */
	if (m_ptr->p_race != NON_RACIAL)
	    race_name = p_info[m_ptr->p_race].name;

	/* It could be a player ghost. */
	if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
	    /* Get the ghost name. */
	    my_strcpy(desc, ghost_name, max);

	    /* Get the undead name. */
	    my_strcpy(undead_name, r_ptr->name, max);

	    /* Build the ghost name. */
	    my_strcat(desc, ", the ", max);
	    my_strcat(desc, undead_name, max);
	}

	/* It could be a Unique */
	else if (rf_has(r_ptr->flags, RF_UNIQUE)) {
	    /* Start with the name (thus nominative and objective) */
	    my_strcpy(desc, name, max);
	}

	/* It could be an indefinite monster */
	else if (mode & 0x08) {
	    bool vowel;
	    char first[2];

	    if (race_name)
		vowel = is_a_vowel(race_name[0]);
	    else
		vowel = is_a_vowel(name[0]);

	    /* XXX Check plurality for "some" */

	    /* Indefinite monsters need an indefinite article */
	    my_strcpy(desc, vowel ? "an " : "a ", max);

	    /* Hack - no capital if there's a race name first */
	    if (race_name) {
		my_strcat(desc, race_name, max);
		my_strcat(desc, " ", max);
		first[0] = tolower(name[0]);
		first[1] = '\0';
		my_strcat(desc, first, max);
		my_strcat(desc, name + 1, max);
	    } else
		my_strcat(desc, name, max);
	}

	/* It could be a normal, definite, monster */
	else {
	    char first[2];

	    /* Definite monsters need a definite article */
	    my_strcpy(desc, "the ", max);
	    /* Hack - no capital if there's a race name first */
	    if (race_name) {
		my_strcat(desc, race_name, max);
		my_strcat(desc, " ", max);
		first[0] = tolower(name[0]);
		first[1] = '\0';
		my_strcat(desc, first, max);
		my_strcat(desc, name + 1, max);
	    } else
		my_strcat(desc, name, max);
	}

	/* Handle the Possessive as a special afterthought */
	if (mode & 0x02) {
	    /* XXX Check for trailing "s" */

	    /* Simply append "apostrophe" and "s" */
	    my_strcat(desc, "'s", max);
	}

	/* Mention "offscreen" monsters XXX XXX */
	if (!panel_contains(m_ptr->fy, m_ptr->fx)) {
	    /* Append special notation */
	    my_strcat(desc, " (offscreen)", max);
	}
    }
    
    if (mode & 0x100)
	my_strcap(desc);
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

    /* Write the name */
    my_strcpy(desc, r_ptr->name, max);
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
    rf_copy(l_ptr->flags, r_ptr->flags);

    /* Update monster recall window */
    if (p_ptr->monster_race_idx == m_ptr->r_idx) {
	/* Redraw stuff */
	p_ptr->redraw |= (PR_MONSTER);
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
    if (num_item > l_ptr->drop_item)
	l_ptr->drop_item = num_item;
    if (num_gold > l_ptr->drop_gold)
	l_ptr->drop_gold = num_gold;

    /* Hack -- memorize the good/great/chest flags */
    if (rf_has(r_ptr->flags, RF_DROP_GOOD))
	rf_on(l_ptr->flags, RF_DROP_GOOD);
    if (rf_has(r_ptr->flags, RF_DROP_GREAT))
	rf_on(l_ptr->flags, RF_DROP_GREAT);
    if (rf_has(r_ptr->flags, RF_DROP_CHEST))
	rf_on(l_ptr->flags, RF_DROP_CHEST);

    /* Update monster recall window */
    if (p_ptr->monster_race_idx == m_ptr->r_idx) {
	/* Redraw stuff */
	p_ptr->redraw |= (PR_MONSTER);
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
    if (full) {
	int py = p_ptr->py;
	int px = p_ptr->px;

	/* Distance components */
	int dy = (py > fy) ? (py - fy) : (fy - py);
	int dx = (px > fx) ? (px - fx) : (fx - px);

	/* Approximate distance */
	d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

	/* Restrict distance */
	if (d > 255)
	    d = 255;

	/* Save the distance */
	m_ptr->cdis = d;
    }

    /* Extract distance */
    else {
	/* Extract the distance */
	d = m_ptr->cdis;
    }


    /* Detected */
    if (m_ptr->mflag & (MFLAG_MARK))
	flag = TRUE;


    /* Nearby */
    if (d <= (p_ptr->themed_level ? MAX_SIGHT / 2 : MAX_SIGHT)) {
	/* Basic telepathy */
	if (p_ptr->state.telepathy || p_ptr->timed[TMD_TELEPATHY]) {
	    /* Empty mind, no telepathy */
	    if (rf_has(r_ptr->flags, RF_EMPTY_MIND)) {
		/* Memorize flags */
		rf_on(l_ptr->flags, RF_EMPTY_MIND);
	    }

	    /* Weird mind, occasional telepathy */
	    else if (rf_has(r_ptr->flags, RF_WEIRD_MIND)) {
		/* Monster is rarely detectable */
		if (((turn / 10) % 10) == (m_idx % 10)) {
		    /* Detectable */
		    notice_obj(OF_TELEPATHY, 0);
		    flag = TRUE;

		    /* Memorize flags */
		    rf_on(l_ptr->flags, RF_WEIRD_MIND);

		    /* Hack -- Memorize mental flags */
		    if (rf_has(r_ptr->flags, RF_SMART))
			rf_on(l_ptr->flags, RF_SMART);
		    if (rf_has(r_ptr->flags, RF_STUPID))
			rf_on(l_ptr->flags, RF_STUPID);
		}
	    }

	    /* Normal mind, allow telepathy */
	    else {
		/* Detectable */
		notice_obj(OF_TELEPATHY, 0);
		flag = TRUE;

		/* Hack -- Memorize mental flags */
		if (rf_has(r_ptr->flags, RF_SMART))
		    rf_on(l_ptr->flags, RF_SMART);
		if (rf_has(r_ptr->flags, RF_STUPID))
		    rf_on(l_ptr->flags, RF_STUPID);
	    }
	}

	/* Normal line of sight, and not blind */
	if (player_has_los_bold(fy, fx) && !p_ptr->timed[TMD_BLIND]) {
	    bool do_invisible = FALSE;
	    bool do_cold_blood = FALSE;

	    /* Use "infravision" */
	    if (d <= p_ptr->state.see_infra) {
		/* Handle "cold blooded" monsters */
		if (rf_has(r_ptr->flags, RF_COLD_BLOOD)) {
		    /* Take note */
		    do_cold_blood = TRUE;
		}

		/* Handle "warm blooded" monsters */
		else {
		    /* Easy to see */
		    easy = flag = TRUE;
		}
	    }

	    /* Use "illumination" */
	    if (player_can_see_bold(fy, fx)) {
		/* Handle "invisible" monsters */
		if (rf_has(r_ptr->flags, RF_INVISIBLE)) {
		    /* Take note */
		    do_invisible = TRUE;

		    /* See invisible */
		    if (p_ptr->state.see_inv) {
			/* Easy to see */
			notice_obj(OF_SEE_INVIS, 0);
			easy = flag = TRUE;
		    }
		}

		/* Handle "normal" monsters */
		else {
		    /* Easy to see */
		    easy = flag = TRUE;
		}
	    }

	    /* Visible */
	    if (flag) {
		/* Memorize flags */
		if (do_invisible)
		    rf_on(l_ptr->flags, RF_INVISIBLE);
		if (do_cold_blood)
		    rf_on(l_ptr->flags, RF_COLD_BLOOD);
	    }
	}
    }


    /* The monster is now visible */
    if (flag) {
	/* It was previously unseen */
	if (!m_ptr->ml) {
	    /* Mark as visible */
	    m_ptr->ml = TRUE;

	    /* Draw the monster */
	    light_spot(fy, fx);

	    /* Update health bar as needed */
	    if (p_ptr->health_who == m_idx)
		p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);

	    /* Hack -- Count "fresh" sightings */
	    if (l_ptr->sights < MAX_SHORT)
		l_ptr->sights++;

	    /* Redraw stuff */
	    p_ptr->redraw |= PR_MONLIST;

	}
    }

    /* The monster is not visible */
    else {
	/* It was previously seen */
	if (m_ptr->ml) {
	    /* Mark as not visible */
	    m_ptr->ml = FALSE;

	    /* Erase the monster */
	    light_spot(fy, fx);

	    /* Update health bar as needed */
	    if (p_ptr->health_who == m_idx)
		p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);

	    /* Redraw stuff */
	    p_ptr->redraw |= PR_MONLIST;

	}
    }


    /* The monster is now easily visible */
    if (easy) {
	/* Change */
	if (!(m_ptr->mflag & (MFLAG_VIEW))) {
	    /* Mark as easily visible */
	    m_ptr->mflag |= (MFLAG_VIEW);

	    /* Re-draw monster window */
	    p_ptr->redraw |= PR_MONLIST;
	}
    }

    /* The monster is not easily visible */
    else {
	/* Change */
	if (m_ptr->mflag & (MFLAG_VIEW)) {
	    /* Mark as not easily visible */
	    m_ptr->mflag &= ~(MFLAG_VIEW);

	    /* Disturb on disappearance */
	    if (OPT(disturb_near) && (m_ptr->hostile == -1))
		disturb(1, 0);

	    /* Re-draw monster window */
	    p_ptr->redraw |= PR_MONLIST;
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
    for (i = 1; i < m_max; i++) {
	monster_type *m_ptr = &m_list[i];

	/* Skip dead monsters */
	if (!m_ptr->r_idx)
	    continue;

	/* Update the monster */
	update_mon(i, full);
    }
}




/**
 * Make a monster carry an object
 */
s16b monster_carry(int m_idx, object_type * j_ptr)
{
    s16b o_idx;

    s16b this_o_idx, next_o_idx = 0;

    monster_type *m_ptr = &m_list[m_idx];

    /* Scan objects already being held for combination */
    for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr;

	/* Acquire object */
	o_ptr = &o_list[this_o_idx];

	/* Acquire next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Check for combination */
	if (object_similar(o_ptr, j_ptr, OSTACK_MONSTER)) {
	    /* Combine the items */
	    object_absorb(o_ptr, j_ptr);

	    /* Result */
	    return (this_o_idx);
	}
    }


    /* Make an object */
    o_idx = o_pop();

    /* Success */
    if (o_idx) {
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
    feature_type *f_ptr;

    /* Check around (and under) the character */
    for (d = 0; d < 9; d++) {
	/* Extract adjacent (legal) location */
	yy = y + ddy_ddd[d];
	xx = x + ddx_ddd[d];

	/* Paranoia */
	if (!in_bounds_fully(yy, xx))
	    continue;

	/* Only check trappable grids */
	f_ptr = &f_info[cave_feat[yy][xx]];
	if (!tf_has(f_ptr->flags, TF_TRAP))
	    continue;

	/* Return false if undetected */
	if (!cave_has(cave_info[yy][xx], CAVE_DTRAP))
	    return (FALSE);
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
    if (m1 > 0) {
	m_ptr = &m_list[m1];

	/* Move monster */
	m_ptr->fy = y2;
	m_ptr->fx = x2;

	/* Update monster */
	update_mon(m1, TRUE);

	/* Redraw monster list */
	p_ptr->redraw |= (PR_MONLIST);
    }

    /* Player 1 */
    else if (m1 < 0) {
	/* Move player */
	p_ptr->py = y2;
	p_ptr->px = x2;
	player_moved = TRUE;
    }

    /* Monster 2 */
    if (m2 > 0) {
	m_ptr = &m_list[m2];

	/* Move monster */
	m_ptr->fy = y1;
	m_ptr->fx = x1;

	/* Update monster */
	update_mon(m2, TRUE);

	/* Redraw monster list */
	p_ptr->redraw |= (PR_MONLIST);
    }

    /* Player 2 */
    else if (m2 < 0) {
	/* Move player */
	p_ptr->py = y1;
	p_ptr->px = x1;
	player_moved = TRUE;
    }

    /* Did the player move? */
    if (player_moved) {
	bool old_dtrap, new_dtrap;

	/* Calculate changes in dtrap status */
	old_dtrap = cave_has(cave_info[y1][x1], CAVE_DTRAP);
	new_dtrap = is_detected(y2, x2);

	/* Note the change in the detect status */
	p_ptr->redraw |= (PR_DTRAP);

	/* Update the panel */
	p_ptr->update |= (PU_PANEL);

	/* Update the visuals (and monster distances) */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_DISTANCE);

	/* Redraw stuff */
	p_ptr->redraw |= PR_MAP;

	/* Redraw monster list */
	p_ptr->redraw |= (PR_MONLIST);

	/* Warn when leaving trap detected region */
	if (OPT(disturb_detect) && old_dtrap && !new_dtrap) {
	    /* Disturb to break runs */
	    disturb(0, 0);
	}
    }

    /* Redraw */
    light_spot(y1, x1);
    light_spot(y2, x2);
}


/**
 * Place the player in the dungeon XXX XXX
 */
s16b player_place(int y, int x)
{
    /* Paranoia XXX XXX */
    if (cave_m_idx[y][x] != 0)
	return (0);

    /* No stairs if we don't do that */
    if (OPT(adult_no_stairs) && !p_ptr->themed_level && p_ptr->depth && !(OPT(adult_thrall) && (p_ptr->depth == 58) && (turn < 10)))
    {
	if (outside)
	    cave_set_feat(y, x, FEAT_ROAD);
	else
	    cave_set_feat(y, x, FEAT_FLOOR);
    }

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
s16b monster_place(int y, int x, monster_type * n_ptr)
{
    s16b m_idx;

    monster_type *m_ptr;
    monster_race *r_ptr;


    /* Paranoia XXX XXX */
    if (cave_m_idx[y][x] != 0)
	return (0);


    /* Get a new record */
    m_idx = m_pop();

    /* Oops */
    if (m_idx) {
	/* Make a new monster */
	cave_m_idx[y][x] = m_idx;

	/* Acquire new monster */
	m_ptr = &m_list[m_idx];

	/* Copy the monster XXX */
	(void) COPY(m_ptr, n_ptr, monster_type);

	/* Location */
	m_ptr->fy = y;
	m_ptr->fx = x;

	/* Update the monster */
	update_mon(m_idx, TRUE);

	/* Acquire new race */
	r_ptr = &r_info[m_ptr->r_idx];

	/* Hack -- Notice new multi-hued monsters */
	if (rf_has(r_ptr->flags, RF_ATTR_MULTI))
	    shimmer_monsters = TRUE;

	/* Hack -- Count the number of "reproducers" */
	if (rf_has(r_ptr->flags, RF_MULTIPLY))
	    num_repro++;

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

    if (!(rf_has(r_ptr->flags, RF_RACIAL)))
	return (FALSE);

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
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    const char *name;

    /* Save previous monster restriction value. */
    bool(*get_mon_num_hook_temp) (int r_idx) = get_mon_num_hook;

    /* Paranoia */
    if (!r_idx)
	return (FALSE);

    /* Paranoia */
    if (!in_bounds(y, x))
	return (FALSE);

    /* Race */
    if (group_mode) {
	/* Set the hook */
	get_mon_num_hook = get_racial_monster;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* XXX - rebuild monster table */
	(void) get_mon_num(monster_level);

	/* Try to get a monster */
	r1_idx = get_mon_num(monster_level);

	/* Reset the hook */
	get_mon_num_hook = get_mon_num_hook_temp;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* XXX - rebuild monster table */
	(void) get_mon_num(monster_level);

	/* Success? */
	if (r1_idx == 0)
	    return (FALSE);
    } else
	r1_idx = r_idx;

    r_ptr = &r_info[r1_idx];

    /* No light hating monsters in daytime */
    if ((rf_has(r_ptr->flags, RF_HURT_LIGHT))
	&& ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2))
	&& (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
	&& (stage_map[p_ptr->stage][STAGE_TYPE] != VALLEY))

	return (FALSE);

    /* The monster must be able to exist in this grid */
    if (!cave_exist_mon(r_ptr, y, x, FALSE))
	return (FALSE);

    /* Paranoia */
    if (!r_ptr->name)
	return (FALSE);

    /* Name */
    name = r_ptr->name;

    /* Hack -- "unique" monsters must be "unique" */
    if ((rf_has(r_ptr->flags, RF_UNIQUE)) && (r_ptr->cur_num >= r_ptr->max_num))
    {
	/* Cannot create */
	return (FALSE);
    }

    /* Hack -- only 1 player ghost at a time */
    if ((rf_has(r_ptr->flags, RF_PLAYER_GHOST)) && bones_selector) 
    {
	/* Cannot create */
	return (FALSE);
    }

    /* Depth monsters may NOT be created out of depth */
    if ((rf_has(r_ptr->flags, RF_FORCE_DEPTH)) && (p_ptr->depth < r_ptr->level))
    {
	/* Cannot create */
	return (FALSE);
    }

    /* Monsters only there to be shapechanged into can never be placed */
    if ((rf_has(r_ptr->flags, RF_NO_PLACE))) 
    {
	/* Cannot create */
	return (FALSE);
    }


    /* Get local monster */
    n_ptr = &monster_type_body;

    /* Clean out the monster */
    (void) WIPE(n_ptr, monster_type);


    /* Save the race */
    n_ptr->r_idx = r1_idx;


    /* 
     * If the monster is a player ghost, perform various manipulations 
     * on it, and forbid ghost creation if something goes wrong.
     */
    if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) 
    {
	if (!prepare_ghost(r_idx, n_ptr, FALSE))
	    return (FALSE);

	name = format("%s, the %s", ghost_name, name);

	/* Hack - point to the special "ghost slot" */
	r_ptr = &r_info[PLAYER_GHOST_RACE];
	n_ptr->r_idx = PLAYER_GHOST_RACE;
    }

    /* Enforce sleeping if needed */
    if (slp && r_ptr->sleep) 
    {
	int val = r_ptr->sleep;
	n_ptr->csleep = ((val * 2) + randint1(val * 10));
    } else if (!((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2))
	       && (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
	       && (p_ptr->depth) && (!character_dungeon))
	n_ptr->csleep += 20;

    /* Enforce no sleeping if needed */
    if ((stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAINTOP)
	&& (tf_has(f_ptr->flags, TF_FALL)))
	n_ptr->csleep = 0;

    /* Assign maximal hitpoints */
    if (rf_has(r_ptr->flags, RF_FORCE_MAXHP)) 
    {
	n_ptr->maxhp = r_ptr->hdice * r_ptr->hside;
    } 
    else 
    {
	n_ptr->maxhp = damroll(r_ptr->hdice, r_ptr->hside);
    }


    /* Initialize distance at which monster likes to operate */

    /* Mark minimum range for recalculation */
    n_ptr->min_range = 0;

    /* Monsters like to use harassment spells at first */
    if (r_ptr->level > 20)
	n_ptr->harass = BASE_HARASS;

    /* Weaker monsters don't live long enough to spend forever harassing */
    else
	n_ptr->harass = LOW_HARASS;

    /* Initialize mana */
    n_ptr->mana = r_ptr->mana;

    /* And start out fully healthy */
    n_ptr->hp = n_ptr->maxhp;

    /* Extract the monster base speed */
    n_ptr->mspeed = r_ptr->speed;

    /* Hack -- small racial variety */
    if (!(rf_has(r_ptr->flags, RF_UNIQUE))) {
	/* Allow some small variation per monster */
	i = extract_energy[r_ptr->speed] / 10;
	if (i)
	    n_ptr->mspeed += rand_spread(0, i);
    }


    /* Force monster to wait for player */
    if (rf_has(r_ptr->flags, RF_FORCE_SLEEP)) {
	/* Give a random starting energy */
	n_ptr->energy = 0;
    } else {
	/* Give a random starting energy */
	n_ptr->energy = randint0(50);
    }

    /* Set the group leader, if there is one */
    n_ptr->group_leader = group_leader;

    /* Initialize racial monster */
    if (rf_has(r_ptr->flags, RF_RACIAL)) {
	int chance, k;

	/* Race is already chosen for group mode */
	if (group_mode) {
	    n_ptr->p_race = group_race;
	    n_ptr->group = group_id;
	}

	/* Choose a race */
	else {
	    k = randint0(race_prob[z_info->p_max - 1][p_ptr->stage]);

	    for (i = 0; i < z_info->p_max; i++)
		if (race_prob[i][p_ptr->stage] > k) {
		    n_ptr->p_race = i;
		    break;
		}

	    /* Hack for Estolad themed level - Edain or Druedain */
	    if (p_ptr->themed_level == THEME_ESTOLAD)
		n_ptr->p_race = (randint0(100) < 50 ? 6 : 8);

	    /* Set group ID */
	    n_ptr->group = ++group_id;

	    /* Go into group mode if necessary */
	    if (rf_has(r_ptr->flags, RF_FRIEND)
		|| rf_has(r_ptr->flags, RF_FRIENDS)) {
		group_mode = TRUE;
		group_race = n_ptr->p_race;
	    }
	}

	/* Set old race */
	n_ptr->old_p_race = NON_RACIAL;

	/* Set hostility */
	chance = g_info[(n_ptr->p_race * z_info->p_max) + p_ptr->prace] - 100;
	if (chance < 0)
	    chance = 0;
	k = randint0(chance + 20);
	if ((k > 20) || (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
	    || (p_ptr->themed_level == THEME_WARLORDS)
	    || cave_has(cave_info[y][x], CAVE_ICKY))
	    n_ptr->hostile = -1;
	else
	    n_ptr->hostile = 0;
    }

    else {
	n_ptr->p_race = NON_RACIAL;
	n_ptr->old_p_race = NON_RACIAL;
	n_ptr->hostile = -1;
	n_ptr->group = ++group_id;
    }

    /* Mark territorial monster's home */
    if (rf_has(r_ptr->flags, RF_TERRITORIAL)) {
	n_ptr->y_terr = y;
	n_ptr->x_terr = x;
    }

    /* Place the monster in the dungeon */
    if (!monster_place(y, x, n_ptr))
	return (FALSE);

    /* Deep unique monsters */
    if ((rf_has(r_ptr->flags, RF_UNIQUE)) && (r_ptr->level > p_ptr->depth)) {
	/* Message */
	if (OPT(cheat_hear))
	    msg("Deep Unique (%s).", name);

	/* Boost rating by twice delta-depth */
	rating += (r_ptr->level - p_ptr->depth) * 2;
    }

    /* Note any unique monster, even if not out of depth */
    else if (rf_has(r_ptr->flags, RF_UNIQUE)) {
	/* Message */
	if (OPT(cheat_hear))
	    msg("Unique (%s).", name);
    }

    /* Deep normal monsters */
    else if (r_ptr->level > p_ptr->depth + 4) {
	/* Message */
	if (OPT(cheat_hear))
	    msg("Deep Monster (%s).", name);

	/* Boost rating by a function of delta-depth */
	rating +=
	    ((r_ptr->level - p_ptr->depth) * (r_ptr->level -
					      p_ptr->depth)) / 25;
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
    if (r_ptr->level > p_ptr->depth) {
	reduce = (r_ptr->level - p_ptr->depth) / 2;
	group_size -= randint1(reduce);
    }

    /* Easy monsters, slightly smaller groups -BR- */
    else if (r_ptr->level < p_ptr->depth) {
	reduce = (p_ptr->depth - r_ptr->level) / 6;
	group_size -= randint1(reduce);
    }

    /* Minimum size */
    if (group_size < 1)
	group_size = 1;

    /* Maximum size */
    if (group_size > GROUP_MAX)
	group_size = GROUP_MAX;


    /* Save the rating */
    old = rating;

    /* Start on the monster */
    hack_n = 1;
    hack_x[0] = x;
    hack_y[0] = y;

    /* Puddle monsters, breadth first, up to group_size */
    for (n = 0; (n < hack_n) && (hack_n < group_size); n++) {
	/* Grab the location */
	int hx = hack_x[n];
	int hy = hack_y[n];

	/* Check each direction, up to group_size */
	for (i = 0; (i < 8) && (hack_n < group_size); i++) {
	    int mx = hx + ddx_ddd[i];
	    int my = hy + ddy_ddd[i];

	    /* Walls and monsters block flow */
	    if (!cave_empty_bold(my, mx))
		continue;

	    /* Attempt to place another monster */
	    if (place_monster_one(my, mx, r_idx, slp)) {
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
    if (z_ptr->d_char != r_ptr->d_char)
	return (FALSE);

    /* Skip more advanced monsters */
    if (z_ptr->level > r_ptr->level)
	return (FALSE);

    /* Skip unique monsters */
    if (rf_has(z_ptr->flags, RF_UNIQUE))
	return (FALSE);

    /* Paranoia -- Skip identical monsters */
    if (place_monster_idx == r_idx)
	return (FALSE);

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
    bool(*get_mon_num_hook_temp) (int r_idx) = get_mon_num_hook;


    /* Calculate the number of escorts we want. */
    if (rf_has(r_ptr->flags, RF_ESCORTS))
	escort_size = 6 + randint0(15);
    else
	escort_size = 2 + randint0(7);


    /* Use the leader's monster type to restrict the escorts. */
    place_monster_idx = leader_idx;

    /* Set the escort hook */
    get_mon_num_hook = place_monster_okay;

    /* Prepare allocation table */
    get_mon_num_prep();

    /* Build monster table */
    (void) get_mon_num(monster_level);

    /* Start on the monster */
    hack_n = 1;
    hack_x[0] = x;
    hack_y[0] = y;

    /* Puddle monsters, breadth first, up to escort_size */
    for (n = 0; (n < hack_n) && (hack_n < escort_size); n++) {
	/* Grab the location */
	int hx = hack_x[n];
	int hy = hack_y[n];

	/* Check each direction, up to group_size */
	for (i = 0; (i < 8) && (hack_n < escort_size); i++) {
	    int mx = hx + ddx_ddd[i];
	    int my = hy + ddy_ddd[i];

	    /* Walls block flow */
	    if (!cave_project(my, mx))
		continue;

	    /* Flow past occupied grids. */
	    if (cave_m_idx[my][mx] != 0) {
		/* Add grid to the "hack" set */
		hack_y[hack_n] = my;
		hack_x[hack_n] = mx;
		hack_n++;

		/* Hack -- Count as "free" grid. */
		if (escort_size < GROUP_MAX)
		    escort_size++;
	    }

	    if (!cave_empty_bold(my, mx))
		continue;

	    /* Get an appropriate monster (quickly). */
	    escort_idx = get_mon_num_quick(r_ptr->level);

	    /* Attempt to place another monster */
	    if (place_monster_one(my, mx, escort_idx, slp)) {
		/* Add grid to the "hack" set */
		hack_y[hack_n] = my;
		hack_x[hack_n] = mx;
		hack_n++;
	    } else
		continue;

	    /* Place a group of escorts if needed */
	    if (rf_has(r_info[escort_idx].flags, RF_FRIENDS)) {
		/* Place a group of monsters */
		(void) place_monster_group(my, mx, escort_idx, slp,
					   (s16b) (5 + randint1(10)));
	    } else if (rf_has(r_info[escort_idx].flags, RF_FRIEND)) {
		/* Place a group of monsters */
		(void) place_monster_group(my, mx, escort_idx, slp,
					   (s16b) (1 + randint1(2)));
	    }
	}
    }

    /* Return to previous monster restrictions (usually none) */
    get_mon_num_hook = get_mon_num_hook_temp;

    /* Prepare allocation table */
    get_mon_num_prep();

    /* XXX - rebuild monster table */
    (void) get_mon_num(monster_level);
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
    if (!place_monster_one(y, x, r_idx, slp)) {
	/* Hack - cancel group mode */
	group_mode = FALSE;
	group_race = NON_RACIAL;
	return (FALSE);
    }

    /* Require the "group" flag */
    if (!grp) {
	/* Cancel group mode */
	group_mode = FALSE;

	return (TRUE);
    }

    /* The original monster is the group leader */
    group_leader = cave_m_idx[y][x];

    /* Friends for certain monsters */
    if (rf_has(r_ptr->flags, RF_FRIENDS)) {
	/* Attempt to place a large group */
	(void) place_monster_group(y, x, r_idx, slp, (s16b) damroll(3, 7));
    }

    else if (rf_has(r_ptr->flags, RF_FRIEND)) {
	/* Attempt to place a small group */
	(void) place_monster_group(y, x, r_idx, slp, (s16b) randint1(3));
    }

    /* Escorts for certain monsters */
    if ((rf_has(r_ptr->flags, RF_ESCORT)) || (rf_has(r_ptr->flags, RF_ESCORTS))) {
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
    if (!quick)
	r_idx = get_mon_num(monster_level);

    /* Pick a monster - quick method */
    else
	r_idx = get_mon_num_quick(monster_level);

    /* Handle failure */
    if (!r_idx)
	return (FALSE);

    /* Attempt to place the monster */
    if (place_monster_aux(y, x, r_idx, slp, grp))
	return (TRUE);

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
    feature_type *f_ptr;

    int r_idx;

    int py = p_ptr->py;
    int px = p_ptr->px;

    int y, x;

    /* Pick a monster - regular method */
    if (!quick)
	r_idx = get_mon_num(monster_level);

    /* Pick a monster - quick method */
    else
	r_idx = get_mon_num_quick(monster_level);

    /* Handle failure */
    if (!r_idx)
	return (FALSE);

    /* Get the monster */
    r_ptr = &r_info[r_idx];

    /* Find a legal, distant, unoccupied, space */
    while (TRUE) {
	/* Pick a location */
	y = randint0(DUNGEON_HGT);
	x = randint0(DUNGEON_WID);

	/* Require a grid that the monster can exist in. */
	if (!cave_exist_mon(r_ptr, y, x, FALSE))
	    continue;

	/* Monsters flying only on mountaintop */
	f_ptr = &f_info[cave_feat[y][x]];
	if (tf_has(f_ptr->flags, TF_FALL)
	    && (stage_map[p_ptr->stage][STAGE_TYPE] != MOUNTAINTOP))
	    continue;

	/* Do not put random monsters in marked rooms. */
	if ((!character_dungeon) && cave_has(cave_info[y][x], CAVE_TEMP))
	    continue;

	/* Accept far away grids */
	if ((dis == 0) || (distance(y, x, py, px) > dis))
	    break;
    }

    /* Attempt to place the monster, allow groups */
    if (place_monster_aux(y, x, r_idx, slp, TRUE))
	return (TRUE);

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
    if (rf_has(r_ptr->flags, RF_PLAYER_GHOST))
	return (FALSE);

    /* Sauron's forms cannot be summoned. */
    if ((rf_has(r_ptr->flags, RF_GAURHOTH))
	&& (rf_has(r_ptr->flags, RF_FORCE_DEPTH))
	&& (summon_specific_type != SUMMON_SAURON))
	return (FALSE);

    /* Hack -- no specific type specified */
    if (!summon_specific_type)
	return (TRUE);

    /* Check our requirements */
    switch (summon_specific_type) {
    case SUMMON_KIN:
	{
	    okay = ((r_ptr->d_char == summon_kin_type)
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_SAURON:
	{
	    okay = ((rf_has(r_ptr->flags, RF_GAURHOTH))
		    && (rf_has(r_ptr->flags, RF_FORCE_DEPTH)));
	    break;
	}

    case SUMMON_ANT:
	{
	    okay = ((r_ptr->d_char == 'a')
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_SPIDER:
	{
	    okay = ((r_ptr->d_char == 'S')
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_HOUND:
	{
	    okay = (((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z'))
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_ANIMAL:
	{
	    okay = ((rf_has(r_ptr->flags, RF_ANIMAL))
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_DRAGON:
	{
	    okay = ((rf_has(r_ptr->flags, RF_DRAGON))
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_HI_DRAGON:
	{
	    okay = (r_ptr->d_char == 'D');
	    break;
	}

    case SUMMON_DEMON:
	{
	    okay = ((rf_has(r_ptr->flags, RF_DEMON))
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_HI_DEMON:
	{
	    okay = (r_ptr->d_char == 'U');
	    break;
	}

    case SUMMON_UNDEAD:
	{
	    okay = ((rf_has(r_ptr->flags, RF_UNDEAD))
		    && !(rf_has(r_ptr->flags, RF_UNIQUE)));
	    break;
	}

    case SUMMON_HI_UNDEAD:
	{
	    okay = ((r_ptr->d_char == 'L') || (r_ptr->d_char == 'V')
		    || (r_ptr->d_char == 'W'));
	    break;
	}


    case SUMMON_UNIQUE:
	{
	    if ((rf_has(r_ptr->flags, RF_UNIQUE)) != 0)
		okay = TRUE;
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
	    okay = ((r_ptr->d_char == 'g')
		    && (!(rf_has(r_ptr->flags, RF_DRAGON))));
	    break;
	}

    case SUMMON_THIEF:
	{
	    /* Scan through all four blows */
	    for (i = 0; i < 4; i++) {
		/* Extract infomation about the blow effect */
		effect = r_ptr->blow[i].effect;
		if (effect == RBE_EAT_GOLD)
		    okay = TRUE;
		if (effect == RBE_EAT_ITEM)
		    okay = TRUE;
	    }
	    break;
	}

    case SUMMON_SWAMP:
	{
	    okay = ((r_ptr->d_char == 'i') || (r_ptr->d_char == 'j')
		    || (r_ptr->d_char == 'm') || (r_ptr->d_char == 'I')
		    || (r_ptr->d_char == 'F') || (r_ptr->d_char == ','));
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
    feature_type *f_ptr;

    /* Prepare to look at a greater distance if necessary */
    for (j = 0; j < 3; j++) {
	/* Look for a location */
	for (i = 0; i < (scattered ? 40 : 20); ++i) {
	    /* Pick a distance */
	    if (scattered)
		d = randint0(6) + 1 + j;
	    else
		d = (i / 10) + 1 + j;

	    /* Pick a location */
	    scatter(&y, &x, y1, x1, d, 0);

	    /* Require passable terrain, with no other creature or player. */
	    f_ptr = &f_info[cave_feat[y][x]];	    
	    if (!tf_has(f_ptr->flags, TF_PASSABLE))
		continue;
	    if (cave_m_idx[y][x] != 0)
		continue;

	    /* Hack -- no summon on glyph of warding */
	    if (cave_trap_specific(y, x, RUNE_PROTECT))
		continue;

	    /* Okay */
	    found = TRUE;
	    break;
	}

	/* Break if already found */
	if (found)
	    break;
    }


    /* Failure */
    if (!found)
	return (FALSE);


    /* Save the "summon" type */
    summon_specific_type = type;


    /* Require "okay" monsters */
    get_mon_num_hook = summon_specific_okay;

    /* Prepare allocation table */
    get_mon_num_prep();


    /* Pick a monster, using the level calculation */
    r_idx =
	get_mon_num((p_ptr->depth + lev) / 2 + 1 +
		    (p_ptr->depth >= 20 ? 4 : p_ptr->depth / 5));


    /* Remove restriction */
    get_mon_num_hook = NULL;

    /* Prepare allocation table */
    get_mon_num_prep();


    /* Handle failure */
    if (!r_idx)
	return (FALSE);

    /* Attempt to place the monster (awake, allow groups) */
    if (!place_monster_aux(y, x, r_idx, FALSE, TRUE))
	return (FALSE);

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
    feature_type *f_ptr;

    /* Look for a location */
    for (i = 0; i < 20; ++i) {
	/* Pick a distance */
	d = (i / 10) + 1;

	/* Pick a location */
	scatter(&y, &x, y1, x1, d, 0);

	/* Require passable terrain, with no other creature or player. */
	f_ptr = &f_info[cave_feat[y][x]];	    
	if (!tf_has(f_ptr->flags, TF_PASSABLE))
	    continue;
	if (cave_m_idx[y][x] != 0)
	    continue;

	/* Hack -- no summon on glyph of warding */
	if (cave_trap_specific(y, x, RUNE_PROTECT))
	    continue;

	/* Okay */
	break;
    }

    /* Failure */
    if (i == 20)
	return (FALSE);

    /* Get quest monsters */
    for (i = 1; i < z_info->r_max; i++) {
	monster_race *r_ptr = &r_info[i];

	/* Ensure quest monsters */
	if ((rf_has(r_ptr->flags, RF_QUESTOR)) && (r_ptr->cur_num < 1)
	    && (r_ptr->max_num)) {
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
int assess_shapechange(int m_idx, monster_type * m_ptr)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_race *tmp_ptr = NULL;

    int i, j, type = 0, shape, rate, spd_diff, best_shape = 0, best_rate = 0;

    bitflag summons[RSF_SIZE];

    flags_init(summons, RSF_SIZE, RSF_SUMMON_MASK);
    rsf_inter(summons, r_ptr->spell_flags);

    /* Pick some possible monsters, using the level calculation */
    for (i = 0; i < 3; i++) {
	int avdam = 0, poss = 0, temp, class, which;

	/* Non-summoners can shapeshift to anything */
	if (rsf_is_empty(summons))
	    type = 0;

	/* Sauron */
	else if ((rf_has(r_ptr->flags, RF_GAURHOTH))
		 && (rf_has(r_ptr->flags, RF_FORCE_DEPTH)))
	    type = SUMMON_SAURON;

	/* Pick a type of summonable monster */
	else {
	    /* Count possibilities */
	    for (j = rsf_next(summons, FLAG_START); j != FLAG_END;
		 j = rsf_next(summons, j + 1))
		poss++;

	    /* Pick one */
	    which = randint0(poss);
	    for (j = 0, class = FLAG_START; j < which; j++)
		class = rsf_next(summons, class);

	    /* Set the type */
	    switch (class) {
	    case RSF_S_KIN:
		{
		    type = SUMMON_KIN;
		    break;
		}

	    case RSF_S_MONSTER:
	    case RSF_S_MONSTERS:
		{
		    type = 0;
		    break;
		}

	    case RSF_S_ANT:
		{
		    type = SUMMON_ANT;
		    break;
		}

	    case RSF_S_SPIDER:
		{
		    type = SUMMON_SPIDER;
		    break;
		}

	    case RSF_S_HOUND:
		{
		    type = SUMMON_HOUND;
		    break;
		}

	    case RSF_S_ANIMAL:
		{
		    type = SUMMON_ANIMAL;
		    break;
		}

	    case RSF_S_THIEF:
		{
		    type = SUMMON_THIEF;
		    break;
		}

	    case RSF_S_SWAMP:
		{
		    type = SUMMON_SWAMP;
		    break;
		}

	    case RSF_S_DRAGON:
		{
		    type = SUMMON_DRAGON;
		    break;
		}

	    case RSF_S_HI_DRAGON:
		{
		    type = SUMMON_HI_DRAGON;
		    break;
		}

	    case RSF_S_DEMON:
		{
		    type = SUMMON_DEMON;
		    break;
		}

	    case RSF_S_HI_DEMON:
		{
		    type = SUMMON_HI_DEMON;
		    break;
		}

	    case RSF_S_UNDEAD:
		{
		    type = SUMMON_UNDEAD;
		    break;
		}

	    case RSF_S_HI_UNDEAD:
		{
		    type = SUMMON_HI_UNDEAD;
		    break;
		}
		/* Default - shouldn't happen */
		type = 0;
	    }
	}


	/* Save the "summon" type */
	summon_specific_type = type;

	/* Require "okay" monsters */
	get_mon_num_hook = summon_specific_okay;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Get a possible race */
	shape =
	    get_mon_num((p_ptr->depth + r_ptr->level) / 2 + 1 +
			(p_ptr->depth >= 20 ? 4 : p_ptr->depth / 5));

	/* Sauron's hack */
	if (!shape && (type == SUMMON_SAURON))
	    shape = m_ptr->r_idx - 1 - randint0(3);

	/* Temporarily change race */
	tmp_ptr = &r_info[shape];
	temp = m_ptr->orig_idx;
	m_ptr->orig_idx = m_ptr->r_idx;
	m_ptr->r_idx = shape;

	/* Get the best ranged attack for this shapechange */
	rate = choose_ranged_attack(m_idx, FALSE, -1);

	/* Take out a cost for time to change */
	rate = 2 * rate / 3;

	/* Adjust for spell frequency */
	rate = tmp_ptr->freq_ranged * rate / r_ptr->freq_ranged;

	/* Get average melee damage */
	for (j = 0; j < 4; j++)
	    avdam += tmp_ptr->blow[j].d_dice * (tmp_ptr->blow[j].d_side / 2);

	/* Reduce for distance from player */
	avdam /= m_ptr->cdis;

	/* Add to the rate */
	rate += avdam;

	/* Get the speed difference */
	spd_diff = (tmp_ptr->speed - r_ptr->speed) / 10;

	/* Factor in the speed */
	if (spd_diff >= 0)
	    rate *= (spd_diff + 1);
	else
	    rate /= -spd_diff;

	/* See if this is the best yet */
	if (rate > best_rate) {
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

    if (p_ptr->wizard) {
	msg("Shapechange rating: %i.", best_rate);
	msg("Best shape: %i.", best_shape);
    }

    /* Set the shape in case it's used */
    m_ptr->orig_idx = best_shape;

    /* Return the best rate */
    return (best_rate);
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
    for (i = 0; i < 18; i++) {
	int d = 1;

	/* Pick a location */
	scatter(&y, &x, m_ptr->fy, m_ptr->fx, d, 0);

	/* Require an "empty" floor grid */
	if (!cave_empty_bold(y, x))
	    continue;

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
    monster_desc(m_name, sizeof(m_name), m_ptr, 0x100);

    /* Notice non-damage */
    if (dam == 0) {
	msg("%s is unharmed.", m_name);
	return;
    }

    /* Note -- subtle fix -CFT */
    newhp = (long) (m_ptr->hp);
    oldhp = newhp + (long) (dam);
    tmp = (newhp * 100L) / oldhp;
    percentage = (int) (tmp);


    /* Jelly's, Mold's, Vortex's, Quthl's */
    if (strchr("jmvQ", r_ptr->d_char)) {
	if (percentage > 95)
	    msg("%s barely notices.", m_name);
	else if (percentage > 75)
	    msg("%s flinches.", m_name);
	else if (percentage > 50)
	    msg("%s squelches.", m_name);
	else if (percentage > 35)
	    msg("%s quivers in pain.", m_name);
	else if (percentage > 20)
	    msg("%s writhes about.", m_name);
	else if (percentage > 10)
	    msg("%s writhes in agony.", m_name);
	else
	    msg("%s jerks limply.", m_name);
    }

    /* Dogs and Hounds */
    else if (strchr("CZ", r_ptr->d_char)) {
	if (percentage > 95)
	    msg("%s shrugs off the attack.", m_name);
	else if (percentage > 75)
	    msg("%s snarls with pain.", m_name);
	else if (percentage > 50)
	    msg("%s yelps in pain.", m_name);
	else if (percentage > 35)
	    msg("%s howls in pain.", m_name);
	else if (percentage > 20)
	    msg("%s howls in agony.", m_name);
	else if (percentage > 10)
	    msg("%s writhes in agony.", m_name);
	else
	    msg("%s yelps feebly.", m_name);
    }

    /* One type of monsters (ignore,squeal,shriek) */
    else if (strchr("FIKMRSXabclqrst", r_ptr->d_char)) {
	if (percentage > 95)
	    msg("%s ignores the attack.", m_name);
	else if (percentage > 75)
	    msg("%s grunts with pain.", m_name);
	else if (percentage > 50)
	    msg("%s squeals in pain.", m_name);
	else if (percentage > 35)
	    msg("%s shrieks in pain.", m_name);
	else if (percentage > 20)
	    msg("%s shrieks in agony.", m_name);
	else if (percentage > 10)
	    msg("%s writhes in agony.", m_name);
	else
	    msg("%s cries out feebly.", m_name);
    }

    /* Another type of monsters (shrug,cry,scream) */
    else {
	if (percentage > 95)
	    msg("%s shrugs off the attack.", m_name);
	else if (percentage > 75)
	    msg("%s grunts with pain.", m_name);
	else if (percentage > 50)
	    msg("%s cries out in pain.", m_name);
	else if (percentage > 35)
	    msg("%s screams in pain.", m_name);
	else if (percentage > 20)
	    msg("%s screams in agony.", m_name);
	else if (percentage > 10)
	    msg("%s writhes in agony.", m_name);
	else
	    msg("%s cries out feebly.", m_name);
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
    if (rf_has(r_ptr->flags, RF_STUPID))
	return;

    /* Not intelligent, only learn sometimes */
    if (!(rf_has(r_ptr->flags, RF_SMART)) && (randint0(100) < 50))
	return;

    /* XXX XXX XXX */

    /* Analyze the knowledge */
    switch (what) {

	/* Slow/paralyze attacks learn about free action and saving throws */
    case LRN_FREE_SAVE:
	{
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	    if (p_ptr->state.free_act)
		m_ptr->smart |= (SM_IMM_FREE);
	    else
		m_ptr->smart &= ~(SM_IMM_FREE);
	    break;
	}

	/* Mana attacks learn if you have any mana to attack */
    case LRN_MANA:
	{
	    if (!p_ptr->msp)
		m_ptr->smart |= (SM_IMM_MANA);
	    else
		m_ptr->smart &= ~(SM_IMM_MANA);
	    break;
	}

	/* Acid attacks learn about Acid resists and immunities */
    case LRN_ACID:
	{
	    if (p_resist_good(P_RES_ACID))
		m_ptr->smart |= (SM_RES_ACID);
	    else
		m_ptr->smart &= ~(SM_RES_ACID);
	    if (p_resist_strong(P_RES_ACID))
		m_ptr->smart |= (SM_RES_STRONG_ACID);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_ACID);
	    if (p_immune(P_RES_ACID))
		m_ptr->smart |= (SM_IMM_ACID);
	    else
		m_ptr->smart &= ~(SM_IMM_ACID);
	    break;
	}

	/* Electircal attacks learn about Electrical resists and immunities */
    case LRN_ELEC:
	{
	    if (p_resist_good(P_RES_ELEC))
		m_ptr->smart |= (SM_RES_ELEC);
	    else
		m_ptr->smart &= ~(SM_RES_ELEC);
	    if (p_resist_strong(P_RES_ELEC))
		m_ptr->smart |= (SM_RES_STRONG_ELEC);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_ELEC);
	    if (p_immune(P_RES_ELEC))
		m_ptr->smart |= (SM_IMM_ELEC);
	    else
		m_ptr->smart &= ~(SM_IMM_ELEC);
	    break;
	}

	/* Fire attacks learn about Fire resists and immunities */
    case LRN_FIRE:
	{
	    if (p_resist_good(P_RES_FIRE))
		m_ptr->smart |= (SM_RES_FIRE);
	    else
		m_ptr->smart &= ~(SM_RES_FIRE);
	    if (p_resist_strong(P_RES_FIRE))
		m_ptr->smart |= (SM_RES_STRONG_FIRE);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_FIRE);
	    if (p_immune(P_RES_FIRE))
		m_ptr->smart |= (SM_IMM_FIRE);
	    else
		m_ptr->smart &= ~(SM_IMM_FIRE);
	    break;
	}

	/* Cold attacks learn about Cold resists and immunities */
    case LRN_COLD:
	{
	    if (p_resist_good(P_RES_COLD))
		m_ptr->smart |= (SM_RES_COLD);
	    else
		m_ptr->smart &= ~(SM_RES_COLD);
	    if (p_resist_strong(P_RES_COLD))
		m_ptr->smart |= (SM_RES_STRONG_COLD);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_COLD);
	    if (p_immune(P_RES_COLD))
		m_ptr->smart |= (SM_IMM_COLD);
	    else
		m_ptr->smart &= ~(SM_IMM_COLD);
	    break;
	}

	/* Poison attacks learn about Poison resists */
    case LRN_POIS:
	{
	    if (p_resist_good(P_RES_POIS))
		m_ptr->smart |= (SM_RES_POIS);
	    else
		m_ptr->smart &= ~(SM_RES_POIS);
	    if (p_resist_strong(P_RES_POIS))
		m_ptr->smart |= (SM_RES_STRONG_POIS);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_POIS);
	    break;
	}

	/* Fear attacks learn about resist fear and saving throws */
    case LRN_FEAR_SAVE:
	{
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	    if (p_ptr->state.no_fear)
		m_ptr->smart |= (SM_RES_FEAR);
	    else
		m_ptr->smart &= ~(SM_RES_FEAR);
	    break;
	}

	/* Light attacks learn about light and blindness resistance */
    case LRN_LIGHT:
	{
	    if (p_resist_good(P_RES_LIGHT))
		m_ptr->smart |= (SM_RES_LIGHT);
	    else
		m_ptr->smart &= ~(SM_RES_LIGHT);
	    break;
	}

	/* Darkness attacks learn about dark and blindness resistance */
    case LRN_DARK:
	{
	    if (p_resist_good(P_RES_DARK))
		m_ptr->smart |= (SM_RES_DARK);
	    else
		m_ptr->smart &= ~(SM_RES_DARK);
	    break;
	}

	/* 
	 * Some Blindness attacks learn about blindness resistance 
	 * Others (below) do more
	 */
    case LRN_BLIND:
	{
	    if (p_ptr->state.no_blind)
		m_ptr->smart |= (SM_RES_BLIND);
	    else
		m_ptr->smart &= ~(SM_RES_BLIND);
	    break;
	}

	/* 
	 * Some Confusion attacks learn about confusion resistance
	 * Others (below) do more
	 */
    case LRN_CONFU:
	{
	    if (p_resist_good(P_RES_CONFU))
		m_ptr->smart |= (SM_RES_CONFU);
	    else
		m_ptr->smart &= ~(SM_RES_CONFU);
	    break;
	}

	/* 
	 * Some sound attacks learn about sound and confusion resistance, 
	 * and saving throws 
	 * Others (below) do less.
	 */
    case LRN_SOUND:
	{
	    if (p_resist_good(P_RES_SOUND))
		m_ptr->smart |= (SM_RES_SOUND);
	    else
		m_ptr->smart &= ~(SM_RES_SOUND);
	    if (p_resist_good(P_RES_CONFU))
		m_ptr->smart |= (SM_RES_CONFU);
	    else
		m_ptr->smart &= ~(SM_RES_CONFU);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	    break;
	}

	/* Shards attacks learn about shards resistance */
    case LRN_SHARD:
	{
	    if (p_resist_good(P_RES_SHARD))
		m_ptr->smart |= (SM_RES_SHARD);
	    else
		m_ptr->smart &= ~(SM_RES_SHARD);
	    break;
	}

	/* 
	 *  Some Nexus attacks learn about Nexus resistance only
	 *  Others (below) do more
	 */
    case LRN_NEXUS:
	{
	    if (p_resist_good(P_RES_NEXUS))
		m_ptr->smart |= (SM_RES_NEXUS);
	    else
		m_ptr->smart &= ~(SM_RES_NEXUS);
	    break;
	}

	/* Nether attacks learn about Nether resistance */
    case LRN_NETHR:
	{
	    if (p_resist_good(P_RES_NETHR))
		m_ptr->smart |= (SM_RES_NETHR);
	    else
		m_ptr->smart &= ~(SM_RES_NETHR);
	    break;
	}

	/* Chaos attacks learn about Chaos resistance */
    case LRN_CHAOS:
	{
	    if (p_resist_good(P_RES_CHAOS))
		m_ptr->smart |= (SM_RES_CHAOS);
	    else
		m_ptr->smart &= ~(SM_RES_CHAOS);
	    break;
	}

	/* Disenchantment attacks learn about disenchantment resistance */
    case LRN_DISEN:
	{
	    if (p_resist_good(P_RES_DISEN))
		m_ptr->smart |= (SM_RES_DISEN);
	    else
		m_ptr->smart &= ~(SM_RES_DISEN);
	    break;
	}

	/* Some attacks learn only about saving throws (cause wounds, etc) */
    case LRN_SAVE:
	{
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	}

	/* Archery attacks don't learn anything */
    case LRN_ARCH:
	{
	    break;
	}

	/* Poison archery attacks learn about poison resists */
    case LRN_PARCH:
	{
	    if (p_resist_good(P_RES_POIS))
		m_ptr->smart |= (SM_RES_POIS);
	    else
		m_ptr->smart &= ~(SM_RES_POIS);
	    if (p_resist_strong(P_RES_POIS))
		m_ptr->smart |= (SM_RES_STRONG_POIS);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_POIS);
	    break;
	}

	/* Ice attacks learn aboyt sound/shards/cold resists and cold immunity */
    case LRN_ICE:
	{
	    if (p_resist_good(P_RES_COLD))
		m_ptr->smart |= (SM_RES_COLD);
	    else
		m_ptr->smart &= ~(SM_RES_COLD);
	    if (p_resist_strong(P_RES_COLD))
		m_ptr->smart |= (SM_RES_STRONG_COLD);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_COLD);
	    if (p_immune(P_RES_COLD))
		m_ptr->smart |= (SM_IMM_COLD);
	    else
		m_ptr->smart &= ~(SM_IMM_COLD);
	    if (p_resist_good(P_RES_SOUND))
		m_ptr->smart |= (SM_RES_SOUND);
	    else
		m_ptr->smart &= ~(SM_RES_SOUND);
	    if (p_resist_good(P_RES_SHARD))
		m_ptr->smart |= (SM_RES_SHARD);
	    else
		m_ptr->smart &= ~(SM_RES_SHARD);
	    break;
	}

	/* Plasma attacks learn about fire/lightning resists/immunities */
    case LRN_PLAS:
	{
	    if (p_resist_good(P_RES_ELEC))
		m_ptr->smart |= (SM_RES_ELEC);
	    else
		m_ptr->smart &= ~(SM_RES_ELEC);
	    if (p_resist_strong(P_RES_ELEC))
		m_ptr->smart |= (SM_RES_STRONG_ELEC);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_ELEC);
	    if (p_immune(P_RES_ELEC))
		m_ptr->smart |= (SM_IMM_ELEC);
	    else
		m_ptr->smart &= ~(SM_IMM_ELEC);
	    if (p_resist_good(P_RES_FIRE))
		m_ptr->smart |= (SM_RES_FIRE);
	    else
		m_ptr->smart &= ~(SM_RES_FIRE);
	    if (p_resist_strong(P_RES_FIRE))
		m_ptr->smart |= (SM_RES_STRONG_FIRE);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_FIRE);
	    if (p_immune(P_RES_FIRE))
		m_ptr->smart |= (SM_IMM_FIRE);
	    else
		m_ptr->smart &= ~(SM_IMM_FIRE);
	    break;
	}

	/* 
	 * Some sounds attacks learna about sound resistance only
	 * Others (above) do more
	 */
    case LRN_SOUND2:
	{
	    if (p_resist_good(P_RES_SOUND))
		m_ptr->smart |= (SM_RES_SOUND);
	    else
		m_ptr->smart &= ~(SM_RES_SOUND);
	    break;
	}

	/* 
	 * Storm attacks learn about Electrical/Cold/Acid resists/immunities,
	 * and about confusion resist
	 */
    case LRN_STORM:
	{
	    if (p_resist_good(P_RES_ELEC))
		m_ptr->smart |= (SM_RES_ELEC);
	    else
		m_ptr->smart &= ~(SM_RES_ELEC);
	    if (p_resist_strong(P_RES_ELEC))
		m_ptr->smart |= (SM_RES_STRONG_ELEC);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_ELEC);
	    if (p_immune(P_RES_ELEC))
		m_ptr->smart |= (SM_IMM_ELEC);
	    else
		m_ptr->smart &= ~(SM_IMM_ELEC);
	    if (p_resist_good(P_RES_COLD))
		m_ptr->smart |= (SM_RES_COLD);
	    else
		m_ptr->smart &= ~(SM_RES_COLD);
	    if (p_resist_strong(P_RES_COLD))
		m_ptr->smart |= (SM_RES_STRONG_COLD);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_COLD);
	    if (p_immune(P_RES_COLD))
		m_ptr->smart |= (SM_IMM_COLD);
	    else
		m_ptr->smart &= ~(SM_IMM_COLD);
	    if (p_resist_good(P_RES_ACID))
		m_ptr->smart |= (SM_RES_ACID);
	    else
		m_ptr->smart &= ~(SM_RES_ACID);
	    if (p_resist_strong(P_RES_ACID))
		m_ptr->smart |= (SM_RES_STRONG_ACID);
	    else
		m_ptr->smart &= ~(SM_RES_STRONG_ACID);
	    if (p_immune(P_RES_ACID))
		m_ptr->smart |= (SM_IMM_ACID);
	    else
		m_ptr->smart &= ~(SM_IMM_ACID);
	    if (p_resist_good(P_RES_CONFU))
		m_ptr->smart |= (SM_RES_CONFU);
	}

	/* Water attacks learn about sound/confusion resists */
    case LRN_WATER:
	{
	    if (p_resist_good(P_RES_SOUND))
		m_ptr->smart |= (SM_RES_SOUND);
	    else
		m_ptr->smart &= ~(SM_RES_SOUND);
	    if (p_resist_good(P_RES_CONFU))
		m_ptr->smart |= (SM_RES_CONFU);
	    else
		m_ptr->smart &= ~(SM_RES_CONFU);
	}

	/* 
	 * Some nexus attacks learn about Nexus resist and saving throws
	 * Others (above) do more
	 */
    case LRN_NEXUS_SAVE:
	{
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	    if (p_resist_good(P_RES_NEXUS))
		m_ptr->smart |= (SM_RES_NEXUS);
	    break;
	}

	/* 
	 * Some Blindness attacks learn about blindness resistance and 
	 * saving throws
	 * Others (above) do less
	 */
    case LRN_BLIND_SAVE:
	{
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	    if (p_ptr->state.no_blind)
		m_ptr->smart |= (SM_RES_BLIND);
	    break;
	}

	/* 
	 * Some Confusion attacks learn about confusion resistance and 
	 * saving throws
	 * Others (above) do less
	 */
    case LRN_CONFU_SAVE:
	{
	    if (p_ptr->state.skills[SKILL_SAVE] >= 75)
		m_ptr->smart |= (SM_GOOD_SAVE);
	    else
		m_ptr->smart &= ~(SM_GOOD_SAVE);
	    if (p_ptr->state.skills[SKILL_SAVE] >= 100)
		m_ptr->smart |= (SM_PERF_SAVE);
	    else
		m_ptr->smart &= ~(SM_PERF_SAVE);
	    if (p_resist_good(P_RES_CONFU))
		m_ptr->smart |= (SM_RES_CONFU);
	    else
		m_ptr->smart &= ~(SM_RES_CONFU);
	    break;
	}
	/* All element spells learn about all resistables */
    case LRN_ALL:
	{
	    /* Recurse */
	    update_smart_learn(m_idx, LRN_ACID);
	    update_smart_learn(m_idx, LRN_FIRE);
	    update_smart_learn(m_idx, LRN_ELEC);
	    update_smart_learn(m_idx, LRN_COLD);
	    update_smart_learn(m_idx, LRN_POIS);
	    update_smart_learn(m_idx, LRN_LIGHT);
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


/**
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 *
 * Note the use of actual "monster names".  XXX XXX XXX
 */
static int get_coin_type(monster_race * r_ptr)
{
    const char *name = r_ptr->name;

    /* Analyze "coin" monsters */
    if (r_ptr->d_char == '$') {
	/* Look for textual clues */
	if (strstr(name, " copper "))
	    return (lookup_kind(TV_GOLD, SV_COPPER));
	if (strstr(name, " silver "))
	    return (lookup_kind(TV_GOLD, SV_SILVER));
	if (strstr(name, " gold "))
	    return (lookup_kind(TV_GOLD, SV_GOLD));
	if (strstr(name, " mithril "))
	    return (lookup_kind(TV_GOLD, SV_MITHRIL));
	if (strstr(name, " adamantite "))
	    return (lookup_kind(TV_GOLD, SV_ADAMANTITE));

	/* Look for textual clues */
	if (strstr(name, "Copper "))
	    return (lookup_kind(TV_GOLD, SV_COPPER));
	if (strstr(name, "Silver "))
	    return (lookup_kind(TV_GOLD, SV_SILVER));
	if (strstr(name, "Gold "))
	    return (lookup_kind(TV_GOLD, SV_GOLD));
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
    while (!cave_valid_bold(y, x)) {
	int d = 1;

	/* Pick a location */
	scatter(&ny, &nx, y, x, d, 0);

	/* Stagger */
	y = ny;
	x = nx;
    }

    /* Destroy any objects */
    delete_object(y, x);

    /* Explain the staircase */
    msg("A magical %s appears...", portal);

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
    for (i = m_max - 1; i >= 1; i--) {
	/* Access the monster */
	monster_type *n_ptr = &m_list[i];

	/* Check if this was the leader */
	if (n_ptr->group_leader == m_idx)
	    n_ptr->group_leader = 0;
    }



    /* Get the location */
    y = m_ptr->fy;
    x = m_ptr->fx;

    /* Drop objects being carried */
    for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
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
	drop_near(i_ptr, -1, y, x, TRUE);
    }

    /* Forget objects */
    m_ptr->hold_o_idx = 0;


    /* Mega-Hack -- drop guardian treasures */
    if (rf_has(r_ptr->flags, RF_DROP_CHOSEN)) {
	/* Morgoth */
	if (r_ptr->level == 100) {
	    /* Get local object */
	    i_ptr = &object_type_body;

	    /* Mega-Hack -- Prepare to make "Grond" */
	    object_prep(i_ptr, lookup_kind(TV_HAFTED, SV_GROND), MAXIMISE);

	    /* Origin */
	    i_ptr->origin = ORIGIN_DROP;
	    i_ptr->origin_xtra = m_ptr->r_idx;
	    i_ptr->origin_stage = p_ptr->stage;

	    /* Mega-Hack -- Mark this item as "Grond" */
	    i_ptr->name1 = ART_GROND;

	    /* Mega-Hack -- Actually create "Grond" */
	    apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);

	    /* Drop it in the dungeon */
	    drop_near(i_ptr, -1, y, x, TRUE);

	    /* Get local object */
	    i_ptr = &object_type_body;

	    /* Mega-Hack -- Prepare to make "Morgoth" */
	    object_prep(i_ptr, lookup_kind(TV_CROWN, SV_MORGOTH), MAXIMISE);

	    /* Origin */
	    i_ptr->origin = ORIGIN_DROP;
	    i_ptr->origin_xtra = m_ptr->r_idx;
	    i_ptr->origin_stage = p_ptr->stage;

	    /* Mega-Hack -- Mark this item as "Morgoth" */
	    i_ptr->name1 = ART_MORGOTH;

	    /* Mega-Hack -- Actually create "Morgoth" */
	    apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);

	    /* Drop it in the dungeon */
	    drop_near(i_ptr, -1, y, x, TRUE);
	}

	/* Ungoliant */
	else if (r_ptr->level == 70) {
	    /* Get local object */
	    i_ptr = &object_type_body;

	    /* Mega-Hack -- Prepare to make "Ungoliant" */
	    object_prep(i_ptr, lookup_kind(TV_CLOAK, SV_UNLIGHT_CLOAK), MAXIMISE);
	    /* Origin */
	    i_ptr->origin = ORIGIN_DROP;
	    i_ptr->origin_xtra = m_ptr->r_idx;
	    i_ptr->origin_stage = p_ptr->stage;

	    /* Mega-Hack -- Mark this item as "Ungoliant" */
	    i_ptr->name1 = ART_UNGOLIANT;

	    /* Mega-Hack -- Actually create "Ungoliant" */
	    apply_magic(i_ptr, -1, TRUE, TRUE, TRUE);

	    /* Drop it in the dungeon */
	    drop_near(i_ptr, -1, y, x, TRUE);
	}
    }

    /* Determine how much we can drop */
    if (rf_has(r_ptr->flags, RF_DROP_60) && (randint0(100) < 60))
	number++;
    if (rf_has(r_ptr->flags, RF_DROP_90) && (randint0(100) < 90))
	number++;

    /* Hack -- nothing's more annoying than a chest that doesn't appear. */
    if (rf_has(r_ptr->flags, RF_DROP_CHEST) && rf_has(r_ptr->flags, RF_DROP_90))
	number = 1;

    if (rf_has(r_ptr->flags, RF_DROP_1D2))
	number += damroll(1, 2);
    if (rf_has(r_ptr->flags, RF_DROP_2D2))
	number += damroll(2, 2);
    if (rf_has(r_ptr->flags, RF_DROP_3D2))
	number += damroll(3, 2);
    if (rf_has(r_ptr->flags, RF_DROP_4D2))
	number += damroll(4, 2);

    /* Hack -- handle creeping coins */
    coin_type = force_coin;

    /* Average dungeon and monster levels */
    object_level = (p_ptr->depth + r_ptr->level) / 2;

    /* Drop some objects */
    for (j = 0; j < number; j++) {
	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make Gold.  Reduced to 30% chance instead of 50%. */
	if (do_gold && (!do_item || (randint0(100) < 30))) {

	    /* Make some gold */
	    if (make_gold(i_ptr)) {

		/* Assume seen XXX XXX XXX */
		dump_gold++;

		/* Drop it in the dungeon */
		drop_near(i_ptr, -1, y, x, TRUE);
	    }
	}

	/* Make chest. */
	else if (rf_has(r_ptr->flags, RF_DROP_CHEST)) 
	{
	    required_tval = TV_CHEST;
	    if (make_object(i_ptr, FALSE, FALSE, TRUE)) 
	    {
		/* Origin */
		i_ptr->origin = (visible ? ORIGIN_DROP : ORIGIN_DROP_UNKNOWN);
		i_ptr->origin_xtra = m_ptr->r_idx;
		i_ptr->origin_stage = p_ptr->stage;

		/* Assume seen */
		dump_item++;

		/* Drop it in the dungeon */
		drop_near(i_ptr, -1, y, x, TRUE);
	    }
	    required_tval = 0;
	}

	/* Make Object */
	else 
	{
	    /* Make an object */
	    if (make_object(i_ptr, good, great, FALSE)) 
	    {
		/* Origin */
		i_ptr->origin = (visible ? ORIGIN_DROP : ORIGIN_DROP_UNKNOWN);
		i_ptr->origin_stage = p_ptr->stage;
		i_ptr->origin_xtra = m_ptr->r_idx;

		/* Assume seen */
		dump_item++;

		/* Drop it in the dungeon */
		drop_near(i_ptr, -1, y, x, TRUE);
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
    p_ptr->redraw |= (PR_MONLIST | PR_ITEMLIST);


    /* Only process "Quest Monsters" */
    if (!(rf_has(r_ptr->flags, RF_QUESTOR)))
	return;


    /* Mark quests as complete */
    for (i = 0; i < MAX_Q_IDX; i++) {
	/* Note completed quests */
	if (stage_map[q_list[i].stage][1] == r_ptr->level)
	    q_list[i].stage = 0;
    }

    /* Mark Sauron's other forms as dead */
    if ((r_ptr->level == 85) && (rf_has(r_ptr->flags, RF_QUESTOR)))
	for (i = 1; i < 4; i++)
	    r_info[m_ptr->r_idx - i].max_num--;

    /* Make a staircase for Morgoth */
    if (r_ptr->level == 100)
	build_quest_stairs(y, x, "staircase");

    /* ...or a portal for ironmen wilderness games */
    else if (OPT(adult_ironman) && !OPT(adult_dungeon) && (p_ptr->depth != 100))
	build_quest_stairs(y, x, "portal");

    /* or a path out of Nan Dungortheb for wilderness games */
    else if ((r_ptr->level == 70) && (p_ptr->depth == 70)
	     && !OPT(adult_dungeon)) {
	/* Make a path */
	for (y = p_ptr->py; y < DUNGEON_HGT - 2; y++)
	    cave_set_feat(y, p_ptr->px, FEAT_ROAD);
	cave_set_feat(DUNGEON_HGT - 2, p_ptr->px, FEAT_LESS_SOUTH);

	/* Announce it */
	msg("The way out of Nan Dungortheb is revealed!");
    }

    /* Increment complete quests */
    p_ptr->quests++;

    /* Check for new specialties */
    p_ptr->update |= (PU_SPECIALTY);

    /* Update */
    update_stuff(p_ptr);

    /* Was it the big one? */
    if (r_ptr->level == 100) {
	/* Total winner */
	p_ptr->total_winner = TRUE;

	/* Have a home now */
	if (!p_ptr->home)
	    p_ptr->home = towns[rp_ptr->hometown];

	/* Redraw the "title" */
	p_ptr->redraw |= (PR_TITLE);

	/* Congratulations */
	msg("*** CONGRATULATIONS ***");
	msg("You have won the game!");
	msg("You may retire (commit suicide) when you are ready.");
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
bool mon_take_hit(int m_idx, int dam, bool * fear, const char *note)
{
    monster_type *m_ptr = &m_list[m_idx];

    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    monster_lore *l_ptr = &l_list[m_ptr->r_idx];

    s32b div, new_exp, new_exp_frac;

    char path[1024];

    /* Hack - Monsters in stasis are invulnerable. */
    if (m_ptr->stasis)
	return (FALSE);

    /* Redraw (later) if needed */
    if (p_ptr->health_who == m_idx)
	p_ptr->redraw |= (PR_HEALTH);

    /* Wake it up */
    m_ptr->csleep = 0;

    /* Always go active */
    m_ptr->mflag |= (MFLAG_ACTV);

    /* Hack - Cancel any special player stealth magics. */
    if (p_ptr->timed[TMD_SSTEALTH]) {
	clear_timed(TMD_SSTEALTH, TRUE);
    }

    /* Complex message. Moved from melee and archery, now allows spell and
     * magical items damage to be debugged by wizards.  -LM- */
    if (p_ptr->wizard) {
	msg("You do %d (out of %d) damage.", dam, m_ptr->hp);
    }

    /* Hurt it */
    m_ptr->hp -= dam;

    /* It is dead now */
    if (m_ptr->hp < 0) {
	char m_name[80];
	char m_poss[80];

	/* Shapeshifters die as their original form */
	if (m_ptr->schange) {
	    char *str;

	    /* Paranoia - make sure _something_ dies */
	    if (!m_ptr->orig_idx)
		m_ptr->orig_idx = m_ptr->r_idx;

	    /* Extract monster name */
	    monster_desc(m_name, sizeof(m_name), m_ptr, 0);

	    /* Get the monster possessive */
	    monster_desc(m_poss, sizeof(m_name), m_ptr, 0x22);

	    /* Do the change */
	    m_ptr->r_idx = m_ptr->orig_idx;
	    m_ptr->orig_idx = 0;
	    m_ptr->p_race = m_ptr->old_p_race;
	    m_ptr->old_p_race = NON_RACIAL;
	    r_ptr = &r_info[m_ptr->r_idx];

	    /* Note the change */
	    str = format("%s%s", m_name);
	    my_strcap(str);
	    msg("%s is revealed in %s true form.", str, m_poss);
	}

	/* Extract monster name */
	monster_desc(m_name, sizeof(m_name), m_ptr, 0);

	/* Make a sound */
	sound(MSG_KILL);

	/* Specialty Ability SOUL_SIPHON */
	if ((player_has(PF_SOUL_SIPHON)) && (p_ptr->csp < p_ptr->msp)
	    &&
	    (!((rf_has(r_ptr->flags, RF_DEMON))
	       || (rf_has(r_ptr->flags, RF_UNDEAD))
	       || (rf_has(r_ptr->flags, RF_STUPID))))) {
	    p_ptr->mana_gain += 2 + (m_ptr->maxhp / 30);
	}

	/* Increase the noise level slightly. */
	if (add_wakeup_chance <= 8000)
	    add_wakeup_chance += 500;

	/* Death by Missile/Spell attack */
	if (note) {
	    char *str = format("%s%s", m_name, note);
	    my_strcap(str);
	    msgt(MSG_KILL, "%s", str);
	}

	/* Death by physical attack -- invisible monster */
	else if (!m_ptr->ml) {
	    msgt(MSG_KILL, "You have killed %s.", m_name);
	}

	/* Death by Physical attack -- non-living monster */
	else if ((rf_has(r_ptr->flags, RF_DEMON))
		 || (rf_has(r_ptr->flags, RF_UNDEAD))
		 || (rf_has(r_ptr->flags, RF_STUPID))
		 || (strchr("Evg", r_ptr->d_char))) {
	    msgt(MSG_KILL, "You have destroyed %s.",
			   m_name);
	}

	/* Death by Physical attack -- living monster */
	else {
	    msgt(MSG_KILL, "You have slain %s.", m_name);
	}

	/* Maximum player level */
	div = p_ptr->max_lev;

	/* Give some experience for the kill */
	new_exp = ((long) r_ptr->mexp * r_ptr->level) / div;

	/* Handle fractional experience */
	new_exp_frac = ((((long) r_ptr->mexp * r_ptr->level) % div)
			* 0x10000L / div) + p_ptr->exp_frac;

	/* Keep track of experience */
	if (new_exp_frac >= 0x10000L) {
	    new_exp++;
	    p_ptr->exp_frac = (u16b) (new_exp_frac - 0x10000L);
	} else {
	    p_ptr->exp_frac = (u16b) new_exp_frac;
	}

	/* Gain experience */
	gain_exp(new_exp);

	/* Generate treasure */
	monster_death(m_idx);

	/* When the player kills a Unique, it stays dead */
	if (rf_has(r_ptr->flags, RF_UNIQUE)) {
	    char note[120];
	    char real_name[120];

	    r_ptr->max_num--;

	    /* write note for player ghosts */
	    if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
		my_strcpy(note,
			  format("Destroyed %s, the %s", ghost_name,
				 r_ptr->name), sizeof(note));
	    }

	    /* All other uniques */
	    else {
		/* Get the monster's real name for the notes file */
		monster_desc_race(real_name, sizeof(real_name), m_ptr->r_idx);

		/* Write note */
		if (monster_is_unusual(r_ptr))
		    my_strcpy(note, format("Destroyed %s", real_name),
			      sizeof(note));
		else
		    my_strcpy(note, format("Killed %s", real_name), sizeof(note));
	    }

	    history_add(note, HISTORY_SLAY_UNIQUE, 0);
	}



	/* When the player kills a player ghost, the bones file that it used
	 * is (often) deleted. */
	if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
	    if (randint1(3) != 1) {
		sprintf(path, "%s/bone.%03d", ANGBAND_DIR_BONE, bones_selector);
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
	if (m_ptr->ml || (rf_has(r_ptr->flags, RF_UNIQUE))) {
	    /* Count kills this life */
	    if (l_ptr->pkills < MAX_SHORT)
		l_ptr->pkills++;

	    /* Add to score if the first time */
	    if (l_ptr->pkills == 1)
		p_ptr->score +=
		    (player_has(PF_DIVINE) ? new_exp / 2 : new_exp);

	    /* Count kills in all lives */
	    if (l_ptr->tkills < MAX_SHORT)
		l_ptr->tkills++;

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
    if (m_ptr->monfear && (dam > 0)) {
	int tmp = randint1(dam);

	/* Cure a little fear */
	if (tmp < m_ptr->monfear) {
	    /* Reduce fear */
	    m_ptr->monfear -= tmp;
	}

	/* Cure all the fear */
	else {
	    /* Cure fear */
	    m_ptr->monfear = 0;

	    /* Flag minimum range for recalculation */
	    m_ptr->min_range = 0;

	    /* No more fear */
	    (*fear) = FALSE;
	}
    }

    /* Sometimes a monster gets scared by damage */
    if (!m_ptr->monfear && !(rf_has(r_ptr->flags, RF_NO_FEAR))) {
	int percentage;

	/* Percentage of fully healthy */
	percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

	/* 
	 * Run (sometimes) if at 10% or less of max hit points,
	 * or (usually) when hit for half its current hit points
	 */
	if ((dam > 0)
	    && ((randint1(10) >= percentage)
		|| ((dam >= m_ptr->hp) && (randint0(100) < 80)))) {
	    /* Hack -- note fear */
	    (*fear) = TRUE;

	    /* Hack -- Add some timed fear */
	    m_ptr->monfear =
		(randint1(10) +
		 (((dam >= m_ptr->hp)
		   && (percentage > 7)) ? 20 : ((11 - percentage) * 5)));

	    /* Flag minimum range for recalculation */
	    m_ptr->min_range = 0;
	}
    }

    /* Recalculate desired minimum range */
    if (dam > 0)
	m_ptr->min_range = 0;

    /* Not dead yet */
    return (FALSE);
}


