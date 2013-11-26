/*
 * File: quest.c
 * Purpose: All quest-related code
 *
 * Copyright (c) 2013 Angband developers
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
#include "mapmode.h"
#include "quest.h"

/*
 * Array[MAX_Q_IDX] of quests
 */
quest q_list[MAX_Q_IDX];

/**
 * Check if a level is a "quest" level
 */
bool is_quest(int stage)
{
	int i;

	/* Check quests */
	for (i = 0; i < MAX_Q_IDX; i++) {
		/* Check for quest */
		if (q_list[i].stage == stage)
			return (TRUE);
	}

	/* Nope */
	return (FALSE);
}


/**
 * Create magical stairs after finishing a quest monster.
 */
void build_quest_stairs(int y, int x, char *portal)
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

/*
 * Check if this (now dead) monster is a quest monster, and act appropriately
 */
bool quest_check(const struct monster *m) {
	size_t i, y;
	monster_race *r_ptr = &r_info[m->r_idx];

	/* Only process "Quest Monsters" */
	if (!(rf_has(r_ptr->flags, RF_QUESTOR)))
		return FALSE;


	/* Mark quests as complete */
	for (i = 0; i < MAX_Q_IDX; i++) {
		/* Note completed quests */
		if (stage_map[q_list[i].stage][1] == r_ptr->level)
			q_list[i].stage = 0;
	}

	/* Hack - mark Sauron's other forms as dead */
	if (((r_ptr->level == 85) || (r_ptr->level == 99))
		&& (rf_has(r_ptr->flags, RF_QUESTOR)))
		for (i = 1; i < 4; i++)
			r_info[m->r_idx - i].max_num--;

	/* Make a staircase for Morgoth (or Sauron) */
	if ((r_ptr->level == 100) || (r_ptr->level == 99))
		build_quest_stairs(m->fy, m->fx, "staircase");

	/* ...or a portal for ironmen wilderness games */
	else if (MODE(IRONMAN) && (p_ptr->map != MAP_DUNGEON) &&
			 (p_ptr->map != MAP_FANILLA) && (p_ptr->depth != 100))
		build_quest_stairs(m->fy, m->fx, "portal");

	/* or a path out of Nan Dungortheb for wilderness games */
	else if ((r_ptr->level == 70) && (p_ptr->depth == 70)
			 && (p_ptr->map != MAP_DUNGEON) && (p_ptr->map != MAP_FANILLA)) {
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

	return TRUE;
}


/**
 * Initialise/free the quest list.
 *
 * This used to dynamically allocate an array of length 5, but
 * now it just makes sure the existing one is clear.
 */
void quest_init(void) {
	memset(q_list, 0, sizeof q_list);
	return;
}

void quest_free(void) {
	return;
}
