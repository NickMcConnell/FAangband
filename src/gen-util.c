/** \file gen-util.c 
    \brief Dungeon generation utilities
 
 * Helper functions making and stocking levels when generated.  
 *
 * Copyright (c) 2011 
 * Nick McConnell, Leon Marrick, Ben Harrison, James E. Wilson, 
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
#include "cave.h"
#include "generate.h"
#include "monster.h"
#include "trap.h"


/**************************************************************/
/*                                                            */
/*            General dungeon-generation functions            */
/*                                                            */
/**************************************************************/


/**
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y, x)"
 */
int next_to_walls(int y, int x)
{
    int i, k = 0;
    feature_type *f_ptr;

    /* Count the adjacent wall grids */
    for (i = 0; i < 4; i++) 
    {
	/* Extract the terrain info */
	f_ptr = &f_info[cave_feat[y + ddy_ddd[i]][x + ddx_ddd[i]]];

	if (tf_has(f_ptr->flags, TF_WALL) &&
	    !tf_has(f_ptr->flags, TF_DOOR_ANY))
	    k++;
    }

    return (k);
}


/**
 * Returns co-ordinates for the player.  Player prefers to be near 
 * walls, because large open spaces are dangerous.
 */
void new_player_spot(void)
{
    int i = 0;
    int y, x;
    feature_type *f_ptr;

    /* 
     * Check stored stair locations, then search at random.
     */
    while (TRUE) {
	i++;

	/* Scan stored locations first. */
	if (i < dun->stair_n) {
	    /* Get location */
	    y = dun->stair[i].y;
	    x = dun->stair[i].x;

	    /* Require exactly three adjacent walls */
	    if (next_to_walls(y, x) != 3)
		continue;

	    /* If character starts on stairs, ... */
	    if (!MODE(NO_STAIRS) || !p_ptr->depth) {
		/* Accept stairs going the right way or floors. */
		if (p_ptr->create_stair) {
		    /* Accept correct stairs */
		    if (cave_feat[y][x] == p_ptr->create_stair)
			break;

		    /* Accept floors, build correct stairs. */
		    f_ptr = &f_info[cave_feat[y][x]];
		    if (cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR)) 
		    {
			cave_set_feat(y, x, p_ptr->create_stair);
			break;
		    }
		}
	    }

	    /* If character doesn't start on stairs, ... */
	    else {
		/* Accept only "naked" floor grids */
		f_ptr = &f_info[cave_feat[y][x]];
		if (cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR))		    
		    break;
	    }
	}
	
	/* Then, search at random */
	else {
	    /* Pick a random grid */
	    y = randint0(DUNGEON_HGT);
	    x = randint0(DUNGEON_WID);

	    /* Refuse to start on anti-teleport (vault) grids */
	    if (cave_has(cave_info[y][x], CAVE_ICKY))
		continue;

	    /* Must be a "naked" floor grid */
	    f_ptr = &f_info[cave_feat[y][x]];
	    if (!(cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR)))
		continue;

	    /* Player prefers to be near walls. */
	    if (i < 300 && (next_to_walls(y, x) < 2))
		continue;
	    else if (i < 600 && (next_to_walls(y, x) < 1))
		continue;

	    /* Success */
	    break;
	}
    }

    /* Place the player */
    player_place(y, x);
}


/**
 * Convert existing terrain type to rubble
 */
static void place_rubble(int y, int x)
{
    /* Create rubble */
    cave_set_feat(y, x, FEAT_RUBBLE);
}


/**
 * Convert existing terrain type to "up stairs"
 */
static void place_up_stairs(int y, int x)
{
    /* Create up stairs */
    cave_set_feat(y, x, FEAT_LESS);
}


/**
 * Convert existing terrain type to "down stairs"
 */
static void place_down_stairs(int y, int x)
{
    /* Create down stairs */
    cave_set_feat(y, x, FEAT_MORE);
}


/**
 * Place an up/down staircase at given location
 */
void place_random_stairs(int y, int x)
{
    /* Paranoia */
    if (!cave_clean_bold(y, x))
	return;

    /* Choose a staircase */
    if (!p_ptr->depth) 
    {
	place_down_stairs(y, x);
    } 
    else if (((p_ptr->map == MAP_DUNGEON) || (p_ptr->map == MAP_FANILLA)) 
	     && !stage_map[p_ptr->stage][DOWN]) 
    {
	place_up_stairs(y, x);
    } 
    else if (randint0(100) < 50) 
    {
	place_down_stairs(y, x);
    } 
    else 
    {
	place_up_stairs(y, x);
    }
}


/**
 * Place a secret door at the given location
 */
void place_secret_door(int y, int x)
{
    /* Create secret door */
    cave_set_feat(y, x, FEAT_SECRET);
}


/**
 * Place an unlocked door at the given location
 */
void place_unlocked_door(int y, int x)
{
    /* Create secret door */
    cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
}


/**
 * Place a random type of closed door at the given location.
 */
void place_closed_door(int y, int x)
{
    int tmp;

    /* Choose an object */
    tmp = randint0(400);

    /* Closed doors (300/400) */
    if (tmp < 300) {
	/* Create closed door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
    }

    /* Locked doors (99/400) */
    else if (tmp < 399) {
	/* Create locked door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + randint1(7));
    }

    /* Stuck doors (1/400) */
    else {
	/* Create jammed door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x08 + randint0(8));
    }
}


/**
 * Place a random type of door at the given location.
 */
void place_random_door(int y, int x)
{
    int tmp;

    /* Choose an object */
    tmp = randint0(1000);

    /* Open doors (300/1000) */
    if (tmp < 300) {
	/* Create open door */
	cave_set_feat(y, x, FEAT_OPEN);
    }

    /* Broken doors (100/1000) */
    else if (tmp < 400) {
	/* Create broken door */
	cave_set_feat(y, x, FEAT_BROKEN);
    }

    /* Secret doors (200/1000) */
    else if (tmp < 600) {
	/* Create secret door */
	cave_set_feat(y, x, FEAT_SECRET);
    }

    /* Closed, locked, or stuck doors (400/1000) */
    else {
	/* Create closed door */
	place_closed_door(y, x);
    }
}






/**
 * Places some staircases near walls
 */
void alloc_stairs(int feat, int num, int walls)
{
    int y, x, i, j;
    feature_type *f_ptr;
    bool no_down_shaft = (!stage_map[stage_map[p_ptr->stage][DOWN]][DOWN]
			  || is_quest(stage_map[p_ptr->stage][DOWN])
			  || is_quest(p_ptr->stage));
    bool no_up_shaft = (!stage_map[stage_map[p_ptr->stage][UP]][UP]);
    bool morgy = is_quest(p_ptr->stage)
	&& stage_map[p_ptr->stage][DEPTH] == 100;


    /* Place "num" stairs */
    for (i = 0; i < num; i++) {
	/* Try hard to place the stair */
	for (j = 0; j < 3000; j++) {
	    /* Cut some slack if necessary. */
	    if ((j > dun->stair_n) && (walls > 2))
		walls = 2;
	    if ((j > 1000) && (walls > 1))
		walls = 1;
	    if (j > 2000)
		walls = 0;

	    /* Use the stored stair locations first. */
	    if (j < dun->stair_n) {
		y = dun->stair[j].y;
		x = dun->stair[j].x;
	    }

	    /* Then, search at random. */
	    else {
		/* Pick a random grid */
		y = randint0(DUNGEON_HGT);
		x = randint0(DUNGEON_WID);
	    }

	    /* Require "naked" floor grid */
	    f_ptr = &f_info[cave_feat[y][x]];
	    if (!(cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR)))
		continue;

	    /* Require a certain number of adjacent walls */
	    if (next_to_walls(y, x) < walls)
		continue;

	    /* If we've asked for a shaft and they're forbidden, fail */
	    if (no_down_shaft && (feat == FEAT_MORE_SHAFT))
		return;
	    if (no_up_shaft && (feat == FEAT_LESS_SHAFT))
		return;

	    /* Town or no way up -- must go down */
	    if ((!p_ptr->depth) || (!stage_map[p_ptr->stage][UP])) {
		/* Clear previous contents, add down stairs */
		if (feat != FEAT_MORE_SHAFT)
		    cave_set_feat(y, x, FEAT_MORE);
	    }

	    /* Bottom of dungeon, Morgoth or underworld -- must go up */
	    else if ((!stage_map[p_ptr->stage][DOWN]) || underworld || morgy) {
		/* Clear previous contents, add up stairs */
		if (feat != FEAT_LESS_SHAFT)
		    cave_set_feat(y, x, FEAT_LESS);
	    }

	    /* Requested type */
	    else {
		/* Clear previous contents, add stairs */
		cave_set_feat(y, x, feat);
	    }

	    /* Finished with this staircase. */
	    break;
	}
    }
}


/**
 * Allocates some objects (using "place" and "type")
 */
void alloc_object(int set, int typ, int num)
{
    int y, x, k;
    feature_type *f_ptr;

    /* Place some objects */
    for (k = 0; k < num; k++) {
	/* Pick a "legal" spot */
	while (TRUE) {
	    bool room;

	    /* Location */
	    y = randint0(DUNGEON_HGT);
	    x = randint0(DUNGEON_WID);
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Paranoia - keep objects out of the outer walls */
	    if (!in_bounds_fully(y, x))
		continue;

	    /* Require "naked" floor grid */
	    f_ptr = &f_info[cave_feat[y][x]];
	    if (!(cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR)))
		continue;

	    /* Check for "room" */
	    room = cave_has(cave_info[y][x], CAVE_ROOM) ? TRUE : FALSE;

	    /* Require corridor? */
	    if ((set == ALLOC_SET_CORR) && room)
		continue;

	    /* Require room? */
	    if ((set == ALLOC_SET_ROOM) && !room)
		continue;

	    /* Accept it */
	    break;
	}

	/* Place something */
	switch (typ) {
	case ALLOC_TYP_RUBBLE:
	    {
		place_rubble(y, x);
		break;
	    }

	case ALLOC_TYP_TRAP:
	    {
		place_trap(y, x, -1, p_ptr->depth);
		break;
	    }

	case ALLOC_TYP_GOLD:
	    {
		place_gold(y, x);
		break;
	    }

	case ALLOC_TYP_OBJECT:
	    {
		place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
		break;
	    }
	}
    }
}

