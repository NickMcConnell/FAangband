/** \file gen-room.c 
    \brief Dungeon room generation
 
 * Code for making rooms of every kind, pits, vaults (inc. interpretation of 
 * v_info.txt)
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
#include "tvalsval.h"


/**************************************************************/
/*                                                            */
/*                   The room-building code                   */
/*                                                            */
/**************************************************************/


/**
 * Place objects, up to the number asked for, in a rectangle centered on 
 * y0, x0.  Accept values for maximum vertical and horizontal displacement.
 *
 * Return prematurely if the code starts looping too much (this may happen 
 * if y0 or x0 are out of bounds, or the floor is already filled).
 */
static void spread_objects(int depth, int num, int y0, int x0, int dy, int dx)
{
    int i, j;			/* Limits on loops */
    int count;
    int y = y0, x = x0;


    /* Try to place objects within our rectangle of effect. */
    for (count = 0, i = 0; ((count < num) && (i < 50)); i++) {
	/* Get a location */
	if ((dy == 0) && (dx == 0)) {
	    y = y0;
	    x = x0;
	    if (!in_bounds(y, x))
		return;
	} else {
	    for (j = 0; j < 10; j++) {
		y = rand_spread(y0, dy);
		x = rand_spread(x0, dx);
		if (!in_bounds(y, x)) {
		    if (j < 9)
			continue;
		    else
			return;
		}
		break;
	    }
	}

	/* Require "clean" floor space */
	if (!cave_clean_bold(y, x))
	    continue;

	/* Place an item */
	if (randint0(100) < 67) {
	    place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
	}

	/* Place gold */
	else {
	    place_gold(y, x);
	}

	/* Count the object, reset the loop count */
	count++;
	i = 0;
    }
}


/**
 * Place traps, up to the number asked for, in a rectangle centered on 
 * y0, x0.  Accept values for maximum vertical and horizontal displacement.
 *
 * Return prematurely if the code starts looping too much (this may happen 
 * if y0 or x0 are out of bounds, or the floor is already filled).
 */
static void spread_traps(int num, int y0, int x0, int dy, int dx)
{
    int i, j;			/* Limits on loops */
    int count;
    int y = y0, x = x0;
    feature_type *f_ptr;

    /* Try to create traps within our rectangle of effect. */
    for (count = 0, i = 0; ((count < num) && (i < 50)); i++) {
	/* Get a location */
	if ((dy == 0) && (dx == 0)) {
	    y = y0;
	    x = x0;
	    if (!in_bounds(y, x))
		return;
	} else {
	    for (j = 0; j < 10; j++) {
		y = rand_spread(y0, dy);
		x = rand_spread(x0, dx);
		if (!in_bounds(y, x)) {
		    if (j < 9)
			continue;
		    else
			return;
		}
		break;
	    }
	}

	/* Require "naked" floor grids */
	f_ptr = &f_info[cave_feat[y][x]];
	if (!(cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_TRAP)))
	    continue;

	/* Place the trap */
	place_trap(y, x, -1, p_ptr->depth);

	/* Count the trap, reset the loop count */
	count++;
	i = 0;
    }
}



/**
 * Generate helper -- create a new room with optional light
 * 
 * Return FALSE if the room is not fully within the dungeon.
 */
static bool generate_room(int y1, int x1, int y2, int x2, int light)
{
    int y, x;

    /* Confirm that room is in bounds. */
    if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	return (FALSE);

    for (y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++) {
	    sqinfo_on(cave_info[y][x], SQUARE_ROOM);
	    if (light)
		sqinfo_on(cave_info[y][x], SQUARE_GLOW);
	}
    }

    /* Success. */
    return (TRUE);
}


/**
 * Generate helper -- fill a rectangle with a feature
 */
static void generate_fill(int y1, int x1, int y2, int x2, int feat)
{
    int y, x;

    for (y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++) {
	    cave_set_feat(y, x, feat);
	}
    }
}


/**
 * Generate helper -- mark a rectangle with a set of cave_info flags
 */
extern void generate_mark(int y1, int x1, int y2, int x2, int flg)
{
    int y, x;

    for (y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++) {
	    sqinfo_on(cave_info[y][x], flg);
	}
    }
}


/**
 * Generate helper -- draw a rectangle with a feature
 */
static void generate_draw(int y1, int x1, int y2, int x2, int feat)
{
    int y, x;

    for (y = y1; y <= y2; y++) {
	cave_set_feat(y, x1, feat);
	cave_set_feat(y, x2, feat);
    }

    for (x = x1; x <= x2; x++) {
	cave_set_feat(y1, x, feat);
	cave_set_feat(y2, x, feat);
    }
}


/**
 * Generate helper -- split a rectangle with a feature
 */
static void generate_plus(int y1, int x1, int y2, int x2, int feat)
{
    int y, x;
    int y0, x0;

    /* Center */
    y0 = (y1 + y2) / 2;
    x0 = (x1 + x2) / 2;

    for (y = y1; y <= y2; y++) {
	cave_set_feat(y, x0, feat);
    }

    for (x = x1; x <= x2; x++) {
	cave_set_feat(y0, x, feat);
    }
}


/**
 * Generate helper -- open all sides of a rectangle with a feature
 */
static void generate_open(int y1, int x1, int y2, int x2, int feat)
{
    int y0, x0;

    /* Center */
    y0 = (y1 + y2) / 2;
    x0 = (x1 + x2) / 2;

    /* Open all sides */
    cave_set_feat(y1, x0, feat);
    cave_set_feat(y0, x1, feat);
    cave_set_feat(y2, x0, feat);
    cave_set_feat(y0, x2, feat);
}


/**
 * Generate helper -- open one side of a rectangle with a feature
 */
static void generate_hole(int y1, int x1, int y2, int x2, int feat)
{
    int y0, x0;

    /* Center */
    y0 = (y1 + y2) / 2;
    x0 = (x1 + x2) / 2;

    /* Open random side */
    switch (randint0(4)) {
    case 0:
	{
	    cave_set_feat(y1, x0, feat);
	    break;
	}
    case 1:
	{
	    cave_set_feat(y0, x1, feat);
	    break;
	}
    case 2:
	{
	    cave_set_feat(y2, x0, feat);
	    break;
	}
    case 3:
	{
	    cave_set_feat(y0, x2, feat);
	    break;
	}
    }
}


/**
 * Find a good spot for the next room.  
 *
 * Find and allocate a free space in the dungeon large enough to hold 
 * the room calling this function.
 *
 * We allocate space in 11x11 blocks, but want to make sure that rooms 
 * align neatly on the standard screen.  Therefore, we make them use 
 * blocks in few 11x33 rectangles as possible.
 *
 * Be careful to include the edges of the room in height and width!
 *
 * Return TRUE and values for the center of the room if all went well.  
 * Otherwise, return FALSE.
 */
static bool find_space(int *y, int *x, int height, int width)
{
    int i;
    int by, bx, by1, bx1, by2, bx2;
    int block_y, block_x;

    bool filled;


    /* Find out how many blocks we need. */
    int blocks_high = 1 + ((height - 1) / BLOCK_HGT);
    int blocks_wide = 1 + ((width - 1) / BLOCK_WID);

    /* Sometimes, little rooms like to have more space. */
    if ((blocks_wide == 2) && (randint0(3) == 0))
	blocks_wide = 3;
    else if ((blocks_wide == 1) && (randint0(2) == 0))
	blocks_wide = 1 + randint1(2);


    /* We'll allow twenty-five guesses. */
    for (i = 0; i < 25; i++) {
	filled = FALSE;

	/* Pick a top left block at random */
	block_y = randint0(dun->row_rooms + blocks_high);
	block_x = randint0(dun->col_rooms + blocks_wide);


	/* Itty-bitty rooms can shift about within their rectangle */
	if (blocks_wide < 3) {
	    /* Rooms that straddle a border must shift. */
	    if ((blocks_wide == 2) && ((block_x % 3) == 2)) {
		if (randint0(2) == 0)
		    block_x--;
		else
		    block_x++;
	    }
	}

	/* Rooms with width divisible by 3 get fitted to a rectangle. */
	else if ((blocks_wide % 3) == 0) {
	    /* Align to the left edge of a 11x33 rectangle. */
	    if ((block_x % 3) == 2)
		block_x++;
	    if ((block_x % 3) == 1)
		block_x--;
	}

	/* 
	 * Big rooms that do not have a width divisible by 3 get 
	 * aligned towards the edge of the dungeon closest to them.
	 */
	else {
	    /* Shift towards left edge of dungeon. */
	    if (block_x + (blocks_wide / 2) <= dun->col_rooms / 2) {
		if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
		    block_x--;
		if ((block_x % 3) == 1)
		    block_x--;
	    }

	    /* Shift toward right edge of dungeon. */
	    else {
		if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
		    block_x++;
		if ((block_x % 3) == 1)
		    block_x++;
	    }
	}

	/* Extract blocks */
	by1 = block_y + 0;
	bx1 = block_x + 0;
	by2 = block_y + blocks_high;
	bx2 = block_x + blocks_wide;

	/* Never run off the screen */
	if ((by1 < 0) || (by2 > dun->row_rooms))
	    continue;
	if ((bx1 < 0) || (bx2 > dun->col_rooms))
	    continue;

	/* Verify available space */
	for (by = by1; by < by2; by++) {
	    for (bx = bx1; bx < bx2; bx++) {
		if (dun->room_map[by][bx]) {
		    filled = TRUE;
		}
	    }
	}

	/* If space filled, try again. */
	if (filled)
	    continue;


	/* It is *extremely* important that the following calculation */
	/* be *exactly* correct to prevent memory errors XXX XXX XXX */

	/* Acquire the location of the room */
	(*y) = ((by1 + by2) * BLOCK_HGT) / 2;
	(*x) = ((bx1 + bx2) * BLOCK_WID) / 2;


	/* Save the room location */
	if (dun->cent_n < CENT_MAX) {
	    dun->cent[dun->cent_n].y = *y;
	    dun->cent[dun->cent_n].x = *x;
	    dun->cent_n++;
	}

	/* Reserve some blocks.  Mark each with the room index. */
	for (by = by1; by < by2; by++) {
	    for (bx = bx1; bx < bx2; bx++) {
		dun->room_map[by][bx] = dun->cent_n;
	    }
	}

	/* Success. */
	return (TRUE);
    }

    /* Failure. */
    return (FALSE);
}



/**
 * Is a feature passable (at least in theory) by the character?
 */
extern bool passable(int feat)
{
    feature_type *f_ptr = &f_info[feat];

    /* Some kinds of terrain are passable. */
    if (tf_has(f_ptr->flags, TF_PASSABLE))
	return (TRUE);

    /* Doors are passable. */
    if (tf_has(f_ptr->flags, TF_DOOR_CLOSED))
	return (TRUE);

    /* Everything else is not passable. */
    return (FALSE);
}



/**
 * Make a starburst room. -LM-
 *
 * Starburst rooms are made in three steps:
 * 1: Choose a room size-dependant number of arcs.  Large rooms need to 
 *    look less granular and alter their shape more often, so they need 
 *    more arcs.
 * 2: For each of the arcs, calculate the portion of the full circle it 
 *    includes, and its maximum effect range (how far in that direction 
 *    we can change features in).  This depends on room size, shape, and 
 *    the maximum effect range of the previous arc.
 * 3: Use the table "get_angle_to_grid" to supply angles to each grid in 
 *    the room.  If the distance to that grid is not greater than the 
 *    maximum effect range that applies at that angle, change the feature 
 *    if appropriate (this depends on feature type).
 *
 * Usage notes:
 * - This function uses a table that cannot handle distances larger than 
 *   20, so it calculates a distance conversion factor for larger rooms.
 * - This function is not good at handling rooms much longer along one axis 
 *   than the other, so it divides such rooms up, and calls itself to handle
 *   each section.  
 * - It is safe to call this function on areas that might contain vaults or 
 *   pits, because "icky" and occupied grids are left untouched.
 *
 * - Mixing these rooms (using normal floor) with rectangular ones on a 
 *   regular basis produces a somewhat chaotic looking dungeon.  However, 
 *   this code does works well for lakes, etc.
 *
 */
extern bool generate_starburst_room(int y1, int x1, int y2, int x2, bool light,
				    int feat, bool special_ok)
{
    int y0, x0, y, x, ny, nx;
    int i, d;
    int dist, max_dist, dist_conv, dist_check;
    int height, width;
    int degree_first, center_of_arc, degree;

    /* Special variant room.  Discovered by accident. */
    bool make_cloverleaf = FALSE;

    /* Holds first degree of arc, maximum effect distance in arc. */
    int arc[45][2];

    /* Number (max 45) of arcs. */
    int arc_num;

    feature_type *f_ptr = &f_info[feat];

    /* Make certain the room does not cross the dungeon edge. */
    if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	return (FALSE);

    /* Robustness -- test sanity of input coordinates. */
    if ((y1 + 2 >= y2) || (x1 + 2 >= x2))
	return (FALSE);


    /* Get room height and width. */
    height = 1 + y2 - y1;
    width = 1 + x2 - x1;


    /* Handle long, narrow rooms by dividing them up. */
    if ((height > 5 * width / 2) || (width > 5 * height / 2)) {
	int tmp_ay, tmp_ax, tmp_by, tmp_bx;

	/* Get bottom-left borders of the first room. */
	tmp_ay = y2;
	tmp_ax = x2;
	if (height > width)
	    tmp_ay = y1 + 2 * height / 3;
	else
	    tmp_ax = x1 + 2 * width / 3;

	/* Make the first room. */
	(void) generate_starburst_room(y1, x1, tmp_ay, tmp_ax, light, feat,
				       FALSE);


	/* Get top_right borders of the second room. */
	tmp_by = y1;
	tmp_bx = x1;
	if (height > width)
	    tmp_by = y1 + 1 * height / 3;
	else
	    tmp_bx = x1 + 1 * width / 3;

	/* Make the second room. */
	(void) generate_starburst_room(tmp_by, tmp_bx, y2, x2, light, feat,
				       FALSE);


	/* 
	 * Hack -- extend a "corridor" between room centers, to ensure 
	 * that the rooms are connected together.
	 */
	if (tf_has(f_ptr->flags, TF_FLOOR)) {
	    for (y = (y1 + tmp_ay) / 2; y <= (tmp_by + y2) / 2; y++) {
		for (x = (x1 + tmp_ax) / 2; x <= (tmp_bx + x2) / 2; x++) {
		    cave_set_feat(y, x, feat);
		}
	    }
	}

	/* 
	 * Hack -- Because rubble and trees are added randomly, and because 
	 * water should have smooth edges, we need to fill any gap between 
	 * two starbursts of these kinds of terrain.
	 */
	if ((feat == FEAT_TREE) || (feat == FEAT_TREE2) || (feat == FEAT_WATER)
	    || (feat == FEAT_RUBBLE)) {
	    int tmp_cy1, tmp_cx1, tmp_cy2, tmp_cx2;

	    if (height > width) {
		tmp_cy1 = y1 + (height - width) / 2;
		tmp_cx1 = x1;
		tmp_cy2 = tmp_cy1 - (height - width) / 2;
		tmp_cx2 = x2;
	    } else {
		tmp_cy1 = y1;
		tmp_cx1 = x1 + (width - height) / 2;
		tmp_cy2 = y2;
		tmp_cx2 = tmp_cx1 + (width - height) / 2;

		tmp_cy1 = y1;
		tmp_cx1 = x1;
	    }

	    /* Make the third room. */
	    (void) generate_starburst_room(tmp_cy1, tmp_cx1, tmp_cy2, tmp_cx2,
					   light, feat, FALSE);
	}

	/* Return. */
	return (TRUE);
    }


    /* Get a shrinkage ratio for large rooms, as table is limited. */
    if ((width > 44) || (height > 44)) {
	if (width > height)
	    dist_conv = 10 * width / 44;
	else
	    dist_conv = 10 * height / 44;
    } else
	dist_conv = 10;


    /* Make a cloverleaf room sometimes. */
    if ((special_ok) && (height > 10) && (randint0(20) == 0)) {
	arc_num = 12;
	make_cloverleaf = TRUE;
    }

    /* Usually, we make a normal starburst. */
    else {
	/* Ask for a reasonable number of arcs. */
	arc_num = 8 + (height * width / 80);
	arc_num = arc_num + 3 - randint0(7);
	if (arc_num < 8)
	    arc_num = 8;
	if (arc_num > 45)
	    arc_num = 45;
    }


    /* Get the center of the starburst. */
    y0 = y1 + height / 2;
    x0 = x1 + width / 2;

    /* Start out at zero degrees. */
    degree_first = 0;


    /* Determine the start degrees and expansion distance for each arc. */
    for (i = 0; i < arc_num; i++) {
	/* Get the first degree for this arc. */
	arc[i][0] = degree_first;

	/* Get a slightly randomized start degree for the next arc. */
	degree_first += (180 + randint0(arc_num)) / arc_num;
	if (degree_first < 180 * (i + 1) / arc_num)
	    degree_first = 180 * (i + 1) / arc_num;
	if (degree_first > (180 + arc_num) * (i + 1) / arc_num)
	    degree_first = (180 + arc_num) * (i + 1) / arc_num;


	/* Get the center of the arc. */
	center_of_arc = degree_first + arc[i][0];


	/* Calculate a reasonable distance to expand vertically. */
	if (((center_of_arc > 45) && (center_of_arc < 135))
	    || ((center_of_arc > 225) && (center_of_arc < 315))) {
	    arc[i][1] = height / 4 + randint0((height + 3) / 4);
	}

	/* Calculate a reasonable distance to expand horizontally. */
	else if (((center_of_arc < 45) || (center_of_arc > 315))
		 || ((center_of_arc < 225) && (center_of_arc > 135))) {
	    arc[i][1] = width / 4 + randint0((width + 3) / 4);
	}

	/* Handle arcs that count as neither vertical nor horizontal */
	else if (i != 0) {
	    if (make_cloverleaf)
		arc[i][1] = 0;
	    else
		arc[i][1] = arc[i - 1][1] + 3 - randint0(7);
	}


	/* Keep variability under control. */
	if ((!make_cloverleaf) && (i != 0) && (i != arc_num - 1)) {
	    /* Water edges must be quite smooth. */
	    if (feat == FEAT_WATER) {
		if (arc[i][1] > arc[i - 1][1] + 2)
		    arc[i][1] = arc[i - 1][1] + 2;

		if (arc[i][1] > arc[i - 1][1] - 2)
		    arc[i][1] = arc[i - 1][1] - 2;
	    } else {
		if (arc[i][1] > 3 * (arc[i - 1][1] + 1) / 2)
		    arc[i][1] = 3 * (arc[i - 1][1] + 1) / 2;

		if (arc[i][1] < 2 * (arc[i - 1][1] - 1) / 3)
		    arc[i][1] = 2 * (arc[i - 1][1] - 1) / 3;
	    }
	}

	/* Neaten up final arc of circle by comparing it to the first. */
	if ((i == arc_num - 1) && (ABS(arc[i][1] - arc[0][1]) > 3)) {
	    if (arc[i][1] > arc[0][1])
		arc[i][1] -= randint0(arc[i][1] - arc[0][1]);
	    else if (arc[i][1] < arc[0][1])
		arc[i][1] += randint0(arc[0][1] - arc[i][1]);
	}
    }


    /* Precalculate check distance. */
    dist_check = 21 * dist_conv / 10;

    /* Change grids between (and not including) the edges. */
    for (y = y1 + 1; y < y2; y++) {
	for (x = x1 + 1; x < x2; x++) {
	    /* Do not touch "icky" grids. */
	    if (sqinfo_has(cave_info[y][x], SQUARE_ICKY))
		continue;

	    /* Do not touch occupied grids. */
	    if (cave_m_idx[y][x] != 0)
		continue;
	    if (cave_o_idx[y][x] != 0)
		continue;


	    /* Get distance to grid. */
	    dist = distance(y0, x0, y, x);

	    /* Look at the grid if within check distance. */
	    if (dist < dist_check) {
		/* Convert and reorient grid for table access. */
		ny = 20 + 10 * (y - y0) / dist_conv;
		nx = 20 + 10 * (x - x0) / dist_conv;

		/* Illegal table access is bad. */
		if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40))
		    continue;

		/* Get angle to current grid. */
		degree = get_angle_to_grid[ny][nx];

		/* Scan arcs to find the one that applies here. */
		for (i = arc_num - 1; i >= 0; i--) {
		    if (arc[i][0] <= degree) {
			max_dist = arc[i][1];

			/* Must be within effect range. */
			if (max_dist >= dist) {
			    /* If new feature is not passable, or floor, always 
			     * place it. */
			    if ((tf_has(f_ptr->flags, TF_FLOOR)) || (!passable(feat))) 
			    {
				cave_set_feat(y, x, feat);

				if (tf_has(f_ptr->flags, TF_FLOOR))
				    sqinfo_on(cave_info[y][x], SQUARE_ROOM);
				else
				    sqinfo_off(cave_info[y][x], SQUARE_ROOM);

				if (light)
				    sqinfo_on(cave_info[y][x], SQUARE_GLOW);
				else
				    sqinfo_off(cave_info[y][x], SQUARE_GLOW);
			    }

			    /* If new feature is non-floor passable terrain,
			     * place it only over floor. */
			    else {
				/* Replace old feature in some cases. */
				if ((feat == FEAT_TREE) || (feat == FEAT_TREE2)
				    || (feat == FEAT_RUBBLE)) {
				    /* Make denser in the middle. */
				    if ((tf_has(f_info[cave_feat[y][x]].flags, TF_FLOOR))
					&& (randint1(max_dist + 5) >= dist + 5))
					cave_set_feat(y, x, feat);
				}
				if ((feat == FEAT_WATER) || (feat == FEAT_LAVA)) {
				    if (tf_has(f_info[cave_feat[y][x]].flags, TF_FLOOR))
					cave_set_feat(y, x, feat);
				}

				/* Light grid. */
				if (light)
				    sqinfo_on(cave_info[y][x], SQUARE_GLOW);
			    }
			}

			/* Arc found.  End search */
			break;
		    }
		}
	    }
	}
    }

    /* 
     * If we placed floors or dungeon granite, all dungeon granite next 
     * to floors needs to become outer wall.
     */
    if ((tf_has(f_ptr->flags, TF_FLOOR)) || (feat == FEAT_WALL_EXTRA)) {
	for (y = y1 + 1; y < y2; y++) {
	    for (x = x1 + 1; x < x2; x++) {
		/* Floor grids only */
		if (tf_has(f_info[cave_feat[y][x]].flags, TF_FLOOR)) {
		    /* Look in all directions. */
		    for (d = 0; d < 8; d++) {
			/* Extract adjacent location */
			int yy = y + ddy_ddd[d];
			int xx = x + ddx_ddd[d];

			/* Join to room */
			sqinfo_on(cave_info[yy][xx], SQUARE_ROOM);

			/* Illuminate if requested. */
			if (light)
			    sqinfo_on(cave_info[yy][xx], SQUARE_GLOW);

			/* Look for dungeon granite. */
			if (cave_feat[yy][xx] == FEAT_WALL_EXTRA) {
			    /* Turn into outer wall. */
			    cave_set_feat(yy, xx, FEAT_WALL_OUTER);
			}
		    }
		}
	    }
	}
    }

    /* Success */
    return (TRUE);
}






/**
 * Room building routines.
 *
 * Nine basic room types:
 *   1 -- normal
 *   2 -- overlapping
 *   3 -- cross shaped
 *   4 -- large room with features
 *   5 -- monster pits
 *   6 -- chambered rooms
 *   7 -- interesting rooms
 *   8 -- simple vaults
 *   9 -- greater vaults
 *
 */


/**
 * Special kind of room type 1, found only in moria-style dungeons.  Uses 
 * the "starburst room" code.
 */
static bool build_type1_moria(bool light)
{
    int y0, x0, y1, x1, y2, x2;
    int i;
    int height, width;
    int feat;

    int select = randint0(120);

    /* Pick a room size */
    height = BLOCK_HGT;
    width = BLOCK_WID;


    /* Try twice to find space for a room. */
    for (i = 0; i < 2; i++) {
	/* Really large room - only on first try. */
	if ((i == 0) && (select % 15 == 0)) {
	    height = (1 + randint1(2)) * height;
	    width = (2 + randint1(3)) * width;
	}

	/* Long, narrow room.  Sometimes tall and thin. */
	else if (select % 4 != 0) {
	    if (select % 15 == 5)
		height = (2 + (select % 2)) * height;
	    else
		width = (2 + (select % 3)) * width;
	}

	/* Find and reserve some space in the dungeon.  Get center of room. */
	if (!find_space(&y0, &x0, height, width)) {
	    if (i == 0)
		continue;
	    if (i == 1)
		return (FALSE);
	} else
	    break;
    }

    /* Locate the room */
    y1 = y0 - height / 2;
    x1 = x0 - width / 2;
    y2 = y1 + height - 1;
    x2 = x1 + width - 1;


    /* Generate starburst room.  Return immediately if out of bounds. */
    if (!generate_starburst_room(y1, x1, y2, x2, light, FEAT_FLOOR, TRUE)) {
	return (FALSE);
    }

    /* Sometimes, the room may have rubble, water, or trees in it. */
    select = randint0(100);

    if (select >= 99)
	feat = FEAT_TREE;
    else if (select >= 98)
	feat = FEAT_TREE2;
    else if (select >= 92)
	feat = FEAT_WATER;
    else if (select >= 88)
	feat = FEAT_RUBBLE;
    else
	feat = 0;

    /* Generate special terrain if necessary. */
    if (feat) {
	(void) generate_starburst_room(y1 + randint0(height / 4),
				       x1 + randint0(width / 4),
				       y2 - randint0(height / 4),
				       x2 - randint0(width / 4), FALSE, feat,
				       FALSE);
    }

    /* Success */
    return (TRUE);
}


/**
 * Type 1 -- normal rectangular rooms
 *
 * These rooms have the lowest build priority (this means that they 
 * should not be very large), and are by far the most common type.
 */
static bool build_type1(void)
{
    int y, x, rand;
    int y0, x0;

    int y1, x1, y2, x2;

    bool light = FALSE;

    /* Occasional light */
    if ((p_ptr->depth <= randint1(35)) && (!underworld))
	light = TRUE;


    /* Use an alternative function if on a moria level. */
    if (moria_level)
	return (build_type1_moria(light));



    /* Pick a room size (less border walls) */
    x = 1 + randint1(11) + randint1(11);
    y = 1 + randint1(4) + randint1(4);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y0, &x0, y + 2, x + 2))
	return (FALSE);

    /* Locate the room */
    y1 = y0 - y / 2;
    x1 = x0 - x / 2;
    y2 = y1 + y - 1;
    x2 = x1 + x - 1;


    /* Generate new room.  Quit immediately if out of bounds. */
    if (!generate_room(y1 - 1, x1 - 1, y2 + 1, x2 + 1, light))
	return (FALSE);


    /* Generate outer walls */
    generate_draw(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_WALL_OUTER);

    /* Make a standard room. */
    generate_fill(y1, x1, y2, x2, FEAT_FLOOR);

    /* Sometimes, we get creative. */
    if (randint0(36) == 0) {
	/* Choose a room type.  Some types require odd dimensions. */
	if ((y % 2 == 0) || (x % 2 == 0))
	    rand = 60 + randint0(40);
	else
	    rand = randint0(100);

	/* Pillar room (requires odd dimensions) */
	if (rand < 30) {
	    int offsety = 0;
	    int offsetx = 0;
	    if (randint0(2) == 0)
		offsety = 1;
	    if (randint0(2) == 0)
		offsetx = 1;

	    for (y = y1 + offsety; y <= y2 - offsety; y += 2) {
		for (x = x1 + offsetx; x <= x2 - offsetx; x += 2) {
		    cave_set_feat(y, x, FEAT_WALL_INNER);
		}
	    }
	}

	/* Ragged-edge room (requires odd dimensions) */
	else if (rand < 60) {
	    int offset = 0;
	    if (randint0(2) == 0)
		offset = 1;

	    for (y = y1 + offset; y <= y2 - offset; y += 2) {
		cave_set_feat(y, x1, FEAT_WALL_INNER);
		cave_set_feat(y, x2, FEAT_WALL_INNER);
	    }

	    for (x = x1 + offset; x <= x2 - offset; x += 2) {
		cave_set_feat(y1, x, FEAT_WALL_INNER);
		cave_set_feat(y2, x, FEAT_WALL_INNER);
	    }
	}

	/* The ceiling has collapsed. */
	else if (rand < 68) {
	    for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
		    /* Wall (or floor) type */
		    int t = randint0(100);

		    /* Granite */
		    if (t < 5) {
			/* Create granite wall */
			cave_set_feat(y, x, FEAT_WALL_EXTRA);
		    }

		    /* Quartz */
		    else if (t < 15) {
			/* Create quartz vein */
			cave_set_feat(y, x, FEAT_QUARTZ);
		    }

		    /* Magma */
		    else if (t < 25) {
			/* Create magma vein */
			cave_set_feat(y, x, FEAT_MAGMA);
		    }

		    /* Rubble. */
		    else if (t < 70) {
			/* Create rubble */
			cave_set_feat(y, x, FEAT_RUBBLE);
		    }

		    /* Floor */
		    else {
			/* Create floor */
			cave_set_feat(y, x, FEAT_FLOOR);
		    }
		}
	    }

	    /* Here, creatures of Earth dwell. */
	    if ((p_ptr->depth > 35) && (randint0(3) == 0)) {
		spread_monsters('X', p_ptr->depth, 2 + randint1(3), y0, x0, 3,
				9);

		/* No normal monsters. */
		generate_mark(y1, x1, y2, x2, SQUARE_TEMP);
	    }
	}

	/* A lake and/or a forest now fills the room. */
	else {
	    bool water_room = FALSE;

	    /* Where there is water, ... */
	    if (generate_starburst_room
		(y1 + randint0(3), x1 + randint0(5), y2 - randint0(3),
		 x2 - randint0(5), FALSE, FEAT_WATER, FALSE)) {
		/* ... there may be water creatures, ... */
		if ((p_ptr->depth > 15) && (randint0(4) == 0)) {
		    spread_monsters('6', p_ptr->depth, 2 + randint1(4), y0, x0,
				    3, 7);

		    water_room = TRUE;
		}
	    }

	    /* ... or a little nature preserve. */
	    if ((!water_room) && (randint0(2) == 0)) {
		int feat = FEAT_TREE;

		if (randint0(2) == 0)
		    feat = FEAT_TREE2;

		/* From light and earth... */
		/* Note that we also have to light the boundary walls. */
		if (!light) {
		    for (y = y1 - 1; y <= y2 + 1; y++) {
			for (x = x1 - 1; x <= x2 + 1; x++) {
			    sqinfo_on(cave_info[y][x], SQUARE_GLOW);
			}
		    }
		}

		/* ... spring trees. */
		(void) generate_starburst_room(y1, x1, y2, x2, FALSE, feat,
					       FALSE);

		/* Animals love trees. */
		if (randint0(6) == 0)
		    spread_monsters('3', p_ptr->depth, 2 + randint0(6), y0, x0,
				    4, 11);
		else if (randint0(3) == 0)
		    spread_monsters('A', p_ptr->depth, 2 + randint0(5), y0, x0,
				    4, 11);
	    }
	}
    }

    /* Success */
    return (TRUE);
}


/**
 * Type 2 -- Overlapping rectangular rooms
 */
static bool build_type2(void)
{
    int y1a, x1a, y2a, x2a;
    int y1b, x1b, y2b, x2b;
    int y0, x0;
    int height, width;

    int light = FALSE;

    /* Occasional light */
    if (p_ptr->depth <= randint1(35))
	light = TRUE;


    /* Determine extents of room (a) */
    y1a = randint1(4);
    x1a = randint1(13);
    y2a = randint1(3);
    x2a = randint1(9);

    /* Determine extents of room (b) */
    y1b = randint1(3);
    x1b = randint1(9);
    y2b = randint1(4);
    x2b = randint1(13);


    /* Calculate height */
    height = 11;

    /* Calculate width */
    if ((x1a < 8) && (x2a < 9) && (x1b < 8) && (x2b < 9))
	width = 22;
    else
	width = 33;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y0, &x0, height, width))
	return (FALSE);

    /* locate room (a) */
    y1a = y0 - y1a;
    x1a = x0 - x1a;
    y2a = y0 + y2a;
    x2a = x0 + x2a;

    /* locate room (b) */
    y1b = y0 - y1b;
    x1b = x0 - x1b;
    y2b = y0 + y2b;
    x2b = x0 + x2b;


    /* Generate new room (a).  Quit immediately if out of bounds. */
    if (!generate_room(y1a - 1, x1a - 1, y2a + 1, x2a + 1, light))
	return (FALSE);

    /* Generate new room (b).  Quit immediately if out of bounds. */
    if (!generate_room(y1b - 1, x1b - 1, y2b + 1, x2b + 1, light))
	return (FALSE);


    /* Generate outer walls (a) */
    generate_draw(y1a - 1, x1a - 1, y2a + 1, x2a + 1, FEAT_WALL_OUTER);

    /* Generate outer walls (b) */
    generate_draw(y1b - 1, x1b - 1, y2b + 1, x2b + 1, FEAT_WALL_OUTER);

    /* Generate inner floors (a) */
    generate_fill(y1a, x1a, y2a, x2a, FEAT_FLOOR);

    /* Generate inner floors (b) */
    generate_fill(y1b, x1b, y2b, x2b, FEAT_FLOOR);


    /* Sometimes, we get creative. */
    if ((p_ptr->depth >= 12) && (randint0(40) == 0)) {
	/* Demons have taken up residence. */
	if (randint0(3) == 0) {
	    /* Pool of lava */
	    (void) generate_starburst_room(y0 - randint1(2), x0 - randint1(3),
					   y0 + randint1(2), x0 + randint1(3),
					   FALSE, FEAT_LAVA, FALSE);

	    if (p_ptr->depth > 45)
		spread_monsters('U', p_ptr->depth, 3 + randint0(5), y0, x0,
				height / 2, width / 2);
	    else
		spread_monsters('u', p_ptr->depth, 2 + randint0(3), y0, x0,
				height / 2, width / 2);
	}

	/* Beings of frost or fire */
	else {
	    /* Get some monsters. */
	    spread_monsters('7', p_ptr->depth, 2 + randint0(5), y0, x0,
			    height / 2, width / 2);
	}
    }

    /* Success */
    return (TRUE);
}



/**
 * Type 3 -- Cross shaped rooms
 *
 * Room "a" runs north/south, and Room "b" runs east/east
 * So a "central pillar" would run from x1a,y1b to x2a,y2b.
 *
 * Note that currently, the "center" is always 3x3, but I think that
 * the code below will work for 5x5 (and perhaps even for asymmetric
 * values like 4x3 or 5x3 or 3x4 or 3x5).
 */
static bool build_type3(void)
{
    int y, x;
    int y0, x0;
    int height, width;

    int y1a, x1a, y2a, x2a;
    int y1b, x1b, y2b, x2b;

    int dy, dx, wy, wx;

    int light = FALSE;

    /* Occasional light */
    if (p_ptr->depth <= randint1(35))
	light = TRUE;


    /* Pick inner dimension */
    wy = 1;
    wx = 1;

    /* Pick outer dimension */
    dy = rand_range(3, 4);
    dx = rand_range(3, 11);

    /* Determine extents of room (a) */
    y1a = dy;
    x1a = wx;
    y2a = dy;
    x2a = wx;

    /* Determine extents of room (b) */
    y1b = wy;
    x1b = dx;
    y2b = wy;
    x2b = dx;

    /* Calculate height */
    if ((y1a + y2a + 1) > (y1b + y2b + 1))
	height = y1a + y2a + 1;
    else
	height = y1b + y2b + 1;

    /* Calculate width */
    if ((x1a + x2a + 1) > (x1b + x2b + 1))
	width = x1a + x2a + 1;
    else
	width = x1b + x2b + 1;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y0, &x0, height, width))
	return (FALSE);

    /* locate room (b) */
    y1a = y0 - dy;
    x1a = x0 - wx;
    y2a = y0 + dy;
    x2a = x0 + wx;

    /* locate room (b) */
    y1b = y0 - wy;
    x1b = x0 - dx;
    y2b = y0 + wy;
    x2b = x0 + dx;


    /* Generate new room (a).  Quit immediately if out of bounds. */
    if (!generate_room(y1a - 1, x1a - 1, y2a + 1, x2a + 1, light))
	return (FALSE);

    /* Generate new room (b).  Quit immediately if out of bounds. */
    if (!generate_room(y1b - 1, x1b - 1, y2b + 1, x2b + 1, light))
	return (FALSE);


    /* Generate outer walls (a) */
    generate_draw(y1a - 1, x1a - 1, y2a + 1, x2a + 1, FEAT_WALL_OUTER);

    /* Generate outer walls (b) */
    generate_draw(y1b - 1, x1b - 1, y2b + 1, x2b + 1, FEAT_WALL_OUTER);

    /* Generate inner floors (a) */
    generate_fill(y1a, x1a, y2a, x2a, FEAT_FLOOR);

    /* Generate inner floors (b) */
    generate_fill(y1b, x1b, y2b, x2b, FEAT_FLOOR);


    /* Special features */
    switch (randint1(4)) {
	/* Nothing */
    case 1:
	{
	    break;
	}

	/* Large solid middle pillar */
    case 2:
	{
	    /* Generate a small inner solid pillar */
	    generate_fill(y1b, x1a, y2b, x2a, FEAT_WALL_INNER);

	    break;
	}

	/* Inner treasure vault */
    case 3:
	{
	    /* Generate a small inner vault */
	    generate_draw(y1b, x1a, y2b, x2a, FEAT_WALL_INNER);

	    /* Open the inner vault with a secret door */
	    generate_hole(y1b, x1a, y2b, x2a, FEAT_SECRET);

	    /* Place a treasure in the vault */
	    object_level = p_ptr->depth + 2;
	    place_object(y0, x0, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
	    object_level = p_ptr->depth;

	    /* Let's guard the treasure well */
	    monster_level = p_ptr->depth + 4;
	    (void) place_monster(y0, x0, TRUE, TRUE, FALSE);
	    monster_level = p_ptr->depth;

	    /* Traps, naturally. */
	    spread_traps(randint1(3), y0, x0, 4, 4);

	    break;
	}

	/* Something else */
    case 4:
	{
	    /* Occasionally pinch the center shut */
	    if (randint0(3) == 0) {
		/* Pinch the east/west sides */
		for (y = y1b; y <= y2b; y++) {
		    if (y == y0)
			continue;
		    cave_set_feat(y, x1a - 1, FEAT_WALL_INNER);
		    cave_set_feat(y, x2a + 1, FEAT_WALL_INNER);
		}

		/* Pinch the north/south sides */
		for (x = x1a; x <= x2a; x++) {
		    if (x == x0)
			continue;
		    cave_set_feat(y1b - 1, x, FEAT_WALL_INNER);
		    cave_set_feat(y2b + 1, x, FEAT_WALL_INNER);
		}

		/* Open sides with secret doors */
		if (randint0(3) == 0) {
		    generate_open(y1b - 1, x1a - 1, y2b + 1, x2a + 1,
				  FEAT_SECRET);
		}
	    }

	    /* Occasionally put a "plus" in the center */
	    else if (randint0(3) == 0) {
		generate_plus(y1b, x1a, y2b, x2a, FEAT_WALL_INNER);
	    }

	    /* Occasionally put a "pillar" in the center */
	    else if (randint0(3) == 0) {
		cave_set_feat(y0, x0, FEAT_WALL_INNER);
	    }

	    break;
	}
    }

    /* Success */
    return (TRUE);
}


/**
 * Type 4 -- Large room with an inner room
 *
 * Possible sub-types:
 *	1 - An inner room with a small inner room
 *	2 - An inner room with a pillar or pillars
 *	3 - An inner room with a checkerboard
 *	4 - An inner room with four compartments
 */
static bool build_type4(void)
{
    int y, x, y1, x1, y2, x2;
    int y0, x0;

    int light = FALSE;

    /* Occasional light */
    if (p_ptr->depth <= randint1(35))
	light = TRUE;


    /* Pick a room size (less border walls) */
    y = 9;
    x = 23;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y0, &x0, y + 2, x + 2))
	return (FALSE);

    /* Locate the room */
    y1 = y0 - y / 2;
    x1 = x0 - x / 2;
    y2 = y1 + y - 1;
    x2 = x1 + x - 1;


    /* Generate new room.  Quit immediately if out of bounds. */
    if (!generate_room(y1 - 1, x1 - 1, y2 + 1, x2 + 1, light))
	return (FALSE);


    /* Generate outer walls */
    generate_draw(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_WALL_OUTER);

    /* Generate inner floors */
    generate_fill(y1, x1, y2, x2, FEAT_FLOOR);


    /* The inner room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* Generate inner walls */
    generate_draw(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_WALL_INNER);

    /* Inner room variations */
    switch (randint1(4)) {
	/* An inner room with a small inner room */
    case 1:
	{
	    /* Open the inner room with a secret door */
	    generate_hole(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_SECRET);

	    /* Place another inner room */
	    generate_draw(y0 - 1, x0 - 1, y0 + 1, x0 + 1, FEAT_WALL_INNER);

	    /* Open the inner room with a locked door */
	    generate_hole(y0 - 1, x0 - 1, y0 + 1, x0 + 1,
			  FEAT_DOOR_HEAD + randint1(7));

	    /* Monsters on guard */
	    spread_monsters('\0', p_ptr->depth + 2, 4, y0, x0, 2, 6);

	    /* Object (80%) */
	    if (randint0(100) < 80) {
		object_level = p_ptr->depth + 2;
		place_object(y0, x0, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
		object_level = p_ptr->depth;
	    }

	    /* Stairs (20%) */
	    else {
		place_random_stairs(y0, x0);
	    }

	    /* Traps */
	    spread_traps(randint0(3) + 1, y0, x0, 2, 4);

	    break;
	}


	/* An inner room with an inner pillar or pillars */
    case 2:
	{
	    /* Open the inner room with a secret door */
	    generate_hole(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_SECRET);

	    /* Inner pillar */
	    generate_fill(y0 - 1, x0 - 1, y0 + 1, x0 + 1, FEAT_WALL_INNER);

	    /* Occasionally, two more large inner pillars */
	    if (randint0(2) == 0) {
		/* Three spaces */
		if (randint0(100) < 50) {
		    /* Inner pillar */
		    generate_fill(y0 - 1, x0 - 7, y0 + 1, x0 - 5,
				  FEAT_WALL_INNER);

		    /* Inner pillar */
		    generate_fill(y0 - 1, x0 + 5, y0 + 1, x0 + 7,
				  FEAT_WALL_INNER);
		}

		/* Two spaces */
		else {
		    /* Inner pillar */
		    generate_fill(y0 - 1, x0 - 6, y0 + 1, x0 - 4,
				  FEAT_WALL_INNER);

		    /* Inner pillar */
		    generate_fill(y0 - 1, x0 + 4, y0 + 1, x0 + 6,
				  FEAT_WALL_INNER);
		}
	    }

	    /* Occasionally, some Inner rooms */
	    if (randint0(3) == 0) {
		/* Inner rectangle */
		generate_draw(y0 - 1, x0 - 5, y0 + 1, x0 + 5, FEAT_WALL_INNER);

		/* Secret doors (random top/bottom) */
		place_secret_door(y0 - 3 + (randint1(2) * 2), x0 - 3);
		place_secret_door(y0 - 3 + (randint1(2) * 2), x0 + 3);

		/* Monsters */
		spread_monsters('\0', p_ptr->depth, randint1(4), y0, x0, 2, 7);

		/* Objects */
		if (randint0(3) == 0)
		    place_object(y0, x0 - 2, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
		if (randint0(3) == 0)
		    place_object(y0, x0 + 2, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
	    }

	    break;
	}

	/* An inner room with a checkerboard */
    case 3:
	{
	    /* Open the inner room with a secret door */
	    generate_hole(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_SECRET);

	    /* Checkerboard */
	    for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
		    if ((x + y) & 0x01) {
			cave_set_feat(y, x, FEAT_WALL_INNER);
		    }
		}
	    }

	    /* Monsters (especially undead) just love mazes. */
	    if (randint0(3) == 0)
		spread_monsters('N', p_ptr->depth, randint1(6), y0, x0, 2, 9);
	    else if (randint0(3) == 0)
		spread_monsters('*', p_ptr->depth, randint1(6), y0, x0, 2, 9);
	    else
		spread_monsters('\0', p_ptr->depth, randint1(6), y0, x0, 2, 9);

	    /* No random monsters. */
	    generate_mark(y1, x1, y2, x2, SQUARE_TEMP);

	    /* Traps make them entertaining. */
	    spread_traps(2 + randint0(4), y0, x0, 2, 9);

	    /* Mazes should have some pretty good treasure too. */
	    spread_objects(p_ptr->depth, 2 + randint0(4), y0, x0, 2, 9);

	    break;
	}

	/* Four small rooms. */
    case 4:
	{
	    /* Inner "cross" */
	    generate_plus(y1, x1, y2, x2, FEAT_WALL_INNER);

	    /* Doors into the rooms */
	    if (randint0(100) < 50) {
		int i = randint1(10);
		place_secret_door(y1 - 1, x0 - i);
		place_secret_door(y1 - 1, x0 + i);
		place_secret_door(y2 + 1, x0 - i);
		place_secret_door(y2 + 1, x0 + i);
	    } else {
		int i = randint1(3);
		place_secret_door(y0 + i, x1 - 1);
		place_secret_door(y0 - i, x1 - 1);
		place_secret_door(y0 + i, x2 + 1);
		place_secret_door(y0 - i, x2 + 1);
	    }

	    /* Treasure, centered at the center of the cross */
	    spread_objects(p_ptr->depth, 2 + randint1(2), y0, x0, 1, 1);

	    /* Gotta have some monsters */
	    spread_monsters('\0', p_ptr->depth, 6 + randint0(11), y0, x0, 2, 9);

	    break;
	}
    }

    /* Success */
    return (TRUE);
}


/**
 * Type 5 -- Monster pits
 *
 * A monster pit is a 11x33 room, with an inner room filled with monsters.
 * 
 * The type of monsters is determined by inputing the current dungeon 
 * level into "mon_symbol_at_depth", and accepting the character returned.
 * After translating this into a set of selection criteria, monsters are 
 * chosen and arranged in the inner room.
 *
 * Monster pits will never contain unique monsters.
 *
 */
static bool build_type5(void)
{
    int y, x, y0, x0, y1, x1, y2, x2;
    int i, j;
    int depth;

    char *name;
    char symbol;

    bool ordered = FALSE;
    bool dummy;
    int light = FALSE;


    /* Pick a room size (less border walls) */
    y = 9;
    x = 23;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y0, &x0, y + 2, x + 2))
	return (FALSE);

    /* Locate the room */
    y1 = y0 - y / 2;
    x1 = x0 - x / 2;
    y2 = y1 + y - 1;
    x2 = x1 + x - 1;


    /* Generate new room.  Quit immediately if out of bounds. */
    if (!generate_room(y1 - 1, x1 - 1, y2 + 1, x2 + 1, light))
	return (FALSE);


    /* Generate outer walls */
    generate_draw(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_WALL_OUTER);

    /* Generate inner floors */
    generate_fill(y1, x1, y2, x2, FEAT_FLOOR);

    /* Advance to the center room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* Generate inner walls */
    generate_draw(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_WALL_INNER);

    /* Open the inner room with a secret door */
    generate_hole(y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_SECRET);


    /* Get a legal depth. */
    depth = p_ptr->depth + randint0(11) - 5;
    if (depth > 60)
	depth = 60;
    if (depth < 5)
	depth = 5;

    /* Choose a monster type, using that depth. */
    symbol = mon_symbol_at_depth[depth / 5 - 1][randint0(7)];

    /* Allow tougher monsters. */
    depth = p_ptr->depth + 3 + (p_ptr->depth < 70 ? p_ptr->depth / 7 : 10);


    /* 
     * Set monster generation restrictions.  Decide how to order 
     * monsters.  Get a description of the monsters.
     */
    name = mon_restrict(symbol, (byte) depth, &ordered, FALSE);

    /* A default description probably means trouble, so stop. */
    if (streq(name, "misc") || !name[0])
	return (TRUE);

    /* Build the monster probability table.  Leave the room empty on failure. */
    if (!get_mon_num(depth))
	return (TRUE);


    /* Arrange the monsters in the room randomly. */
    if (!ordered) {
	int r_idx = 0;

	/* Place some monsters */
	for (y = y0 - 2; y <= y0 + 2; y++) {
	    for (x = x0 - 9; x <= x0 + 9; x++) {
		/* Get a monster index */
		r_idx = get_mon_num_quick(depth);

		/* Place a single monster */
		(void) place_monster_aux(y, x, r_idx, FALSE, FALSE);
	    }
	}
    }

    /* Arrange the monsters in the room in an orderly fashion. */
    else {
	s16b what[16];

	/* Pick some monster types */
	for (i = 0; i < 16; i++) {
	    /* Get a monster index */
	    what[i] = get_mon_num_quick(depth);
	}

	/* Sort the monsters */
	for (i = 0; i < 16 - 1; i++) {
	    for (j = 0; j < 16 - 1; j++) {
		int i1 = j;
		int i2 = j + 1;

		int p1 = r_info[what[i1]].level;
		int p2 = r_info[what[i2]].level;

		/* Bubble sort */
		if (p1 > p2) {
		    int tmp = what[i1];
		    what[i1] = what[i2];
		    what[i2] = tmp;
		}
	    }
	}


	/* Top and bottom rows (outer) */
	for (x = x0 - 9; x <= x0 - 4; x++) {
	    place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE);
	    place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE);
	}
	for (x = x0 + 4; x <= x0 + 9; x++) {
	    place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE);
	    place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE);
	}

	/* Top and bottom rows (inner) */
	for (x = x0 - 3; x <= x0 + 3; x++) {
	    place_monster_aux(y0 - 2, x, what[3], FALSE, FALSE);
	    place_monster_aux(y0 + 2, x, what[3], FALSE, FALSE);
	}

	/* Middle columns */
	for (y = y0 - 1; y <= y0 + 1; y++) {
	    place_monster_aux(y, x0 - 9, what[2], FALSE, FALSE);
	    place_monster_aux(y, x0 + 9, what[2], FALSE, FALSE);

	    place_monster_aux(y, x0 - 8, what[4], FALSE, FALSE);
	    place_monster_aux(y, x0 + 8, what[4], FALSE, FALSE);

	    place_monster_aux(y, x0 - 7, what[5], FALSE, FALSE);
	    place_monster_aux(y, x0 + 7, what[5], FALSE, FALSE);

	    place_monster_aux(y, x0 - 6, what[6], FALSE, FALSE);
	    place_monster_aux(y, x0 + 6, what[6], FALSE, FALSE);

	    place_monster_aux(y, x0 - 5, what[7], FALSE, FALSE);
	    place_monster_aux(y, x0 + 5, what[7], FALSE, FALSE);

	    place_monster_aux(y, x0 - 4, what[8], FALSE, FALSE);
	    place_monster_aux(y, x0 + 4, what[8], FALSE, FALSE);

	    place_monster_aux(y, x0 - 3, what[9], FALSE, FALSE);
	    place_monster_aux(y, x0 + 3, what[9], FALSE, FALSE);

	    place_monster_aux(y, x0 - 2, what[11], FALSE, FALSE);
	    place_monster_aux(y, x0 + 2, what[11], FALSE, FALSE);
	}

	/* Above/Below the center monster */
	for (x = x0 - 1; x <= x0 + 1; x++) {
	    place_monster_aux(y0 + 1, x, what[12], FALSE, FALSE);
	    place_monster_aux(y0 - 1, x, what[12], FALSE, FALSE);
	}

	/* Next to the center monster */
	place_monster_aux(y0, x0 + 1, what[14], FALSE, FALSE);
	place_monster_aux(y0, x0 - 1, what[14], FALSE, FALSE);

	/* Center monster */
	place_monster_aux(y0, x0, what[15], FALSE, FALSE);
    }

    /* Remove restrictions */
    (void) mon_restrict('\0', (byte) depth, &dummy, FALSE);


    /* Describe */
    if (OPT(cheat_room)) {
	/* Room type */
	msg("Monster pit (%s)", name);
    }

    /* Increase the level rating */
    rating += 10;

    /* Success */
    return (TRUE);
}



/**
 * Helper function to "build_type6".  Fill a room matching 
 * the rectangle input with magma, and surround it with inner wall.  
 * Create a door in a random inner wall grid along the border of the 
 * rectangle.
 */
static void make_chamber(int c_y1, int c_x1, int c_y2, int c_x2)
{
    int i, d, y, x;
    int count;

    /* Fill with soft granite (will later be replaced with floor). */
    generate_fill(c_y1 + 1, c_x1 + 1, c_y2 - 1, c_x2 - 1, FEAT_MAGMA);

    /* Generate inner walls over dungeon granite and magma. */
    for (y = c_y1; y <= c_y2; y++) {
	/* left wall */
	x = c_x1;

	if ((cave_feat[y][x] == FEAT_WALL_EXTRA)
	    || (cave_feat[y][x] == FEAT_MAGMA))
	    cave_set_feat(y, x, FEAT_WALL_INNER);
    }

    for (y = c_y1; y <= c_y2; y++) {
	/* right wall */
	x = c_x2;

	if ((cave_feat[y][x] == FEAT_WALL_EXTRA)
	    || (cave_feat[y][x] == FEAT_MAGMA))
	    cave_set_feat(y, x, FEAT_WALL_INNER);
    }

    for (x = c_x1; x <= c_x2; x++) {
	/* top wall */
	y = c_y1;

	if ((cave_feat[y][x] == FEAT_WALL_EXTRA)
	    || (cave_feat[y][x] == FEAT_MAGMA))
	    cave_set_feat(y, x, FEAT_WALL_INNER);
    }

    for (x = c_x1; x <= c_x2; x++) {
	/* bottom wall */
	y = c_y2;

	if ((cave_feat[y][x] == FEAT_WALL_EXTRA)
	    || (cave_feat[y][x] == FEAT_MAGMA))
	    cave_set_feat(y, x, FEAT_WALL_INNER);
    }

    /* Try a few times to place a door. */
    for (i = 0; i < 20; i++) {
	/* Pick a square along the edge, not a corner. */
	if (randint0(2) == 0) {
	    /* Somewhere along the (interior) side walls. */
	    if (randint0(2))
		x = c_x1;
	    else
		x = c_x2;
	    y = c_y1 + randint0(1 + ABS(c_y2 - c_y1));
	} else {
	    /* Somewhere along the (interior) top and bottom walls. */
	    if (randint0(2))
		y = c_y1;
	    else
		y = c_y2;
	    x = c_x1 + randint0(1 + ABS(c_x2 - c_x1));
	}

	/* If not an inner wall square, try again. */
	if (cave_feat[y][x] != FEAT_WALL_INNER)
	    continue;

	/* Paranoia */
	if (!in_bounds_fully(y, x))
	    continue;

	/* Reset wall count */
	count = 0;

	/* If square has not more than two adjacent walls, and no adjacent
	 * doors, place door. */
	for (d = 0; d < 9; d++) {
	    /* Extract adjacent (legal) location */
	    int yy = y + ddy_ddd[d];
	    int xx = x + ddx_ddd[d];

	    /* No doors beside doors. */
	    if (cave_feat[yy][xx] == FEAT_OPEN)
		break;

	    /* Count the inner walls. */
	    if (cave_feat[yy][xx] == FEAT_WALL_INNER)
		count++;

	    /* No more than two walls adjacent (plus the one we're on). */
	    if (count > 3)
		break;

	    /* Checked every direction? */
	    if (d == 8) {
		/* Place an open door. */
		cave_set_feat(y, x, FEAT_OPEN);

		/* Success. */
		return;
	    }
	}
    }
}



/**
 * Expand in every direction from a start point, turning magma into rooms.  
 * Stop only when the magma and the open doors totally run out.
 */
static void hollow_out_room(int y, int x)
{
    int d, yy, xx;

    for (d = 0; d < 9; d++) {
	/* Extract adjacent location */
	yy = y + ddy_ddd[d];
	xx = x + ddx_ddd[d];

	/* Change magma to floor. */
	if (cave_feat[yy][xx] == FEAT_MAGMA) {
	    cave_set_feat(yy, xx, FEAT_FLOOR);

	    /* Hollow out the room. */
	    hollow_out_room(yy, xx);
	}
	/* Change open door to broken door. */
	else if (cave_feat[yy][xx] == FEAT_OPEN) {
	    cave_set_feat(yy, xx, FEAT_BROKEN);

	    /* Hollow out the (new) room. */
	    hollow_out_room(yy, xx);
	}
    }
}



/**
 * Type 6 -- Rooms of chambers
 *
 * Build a room, varying in size between 22x22 and 44x66, consisting of 
 * many smaller, irregularly placed, chambers all connected by doors or 
 * short tunnels. -LM-
 *
 * Plop down an area-dependent number of magma-filled chambers, and remove 
 * blind doors and tiny rooms.
 *
 * Hollow out a chamber near the center, connect it to new chambers, and 
 * hollow them out in turn.  Continue in this fashion until there are no 
 * remaining chambers within two squares of any cleared chamber.
 *
 * Clean up doors.  Neaten up the wall types.  Turn floor grids into rooms, 
 * illuminate if requested.
 *
 * Fill the room with up to 35 (sometimes up to 50) monsters of a creature 
 * race or type that offers a challenge at the character's depth.  This is 
 * similar to monster pits, except that we choose among a wider range of 
 * monsters.
 *
 */
static bool build_type6(void)
{
    int i, d;
    int area, num_chambers;
    int y, x, yy, xx;
    int yy1, xx1, yy2, xx2, yy3, xx3;

    int height, width, count;

    int y0, x0, y1, x1, y2, x2;

    /* Deeper in the dungeon, chambers are less likely to be lit. */
    bool light = (randint0(45) > p_ptr->depth) ? TRUE : FALSE;


    /* Calculate a level-dependent room size modifier. */
    if (p_ptr->depth > randint0(160))
	i = 4;
    else if (p_ptr->depth > randint0(100))
	i = 3;
    else
	i = 2;

    /* Calculate the room size. */
    height = BLOCK_HGT * i;
    width = BLOCK_WID * (i + randint0(3));

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y0, &x0, height, width))
	return (FALSE);

    /* Calculate the borders of the room. */
    y1 = y0 - (height / 2);
    x1 = x0 - (width / 2);
    y2 = y0 + (height - 1) / 2;
    x2 = x0 + (width - 1) / 2;

    /* Make certain the room does not cross the dungeon edge. */
    if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	return (FALSE);


    /* Determine how much space we have. */
    area = ABS(y2 - y1) * ABS(x2 - x1);

    /* Calculate the number of smaller chambers to make. */
    num_chambers = 10 + area / 80;

    /* Build the chambers. */
    for (i = 0; i < num_chambers; i++) {
	int c_y1, c_x1, c_y2, c_x2;
	int size, width, height;

	/* Determine size of chamber. */
	size = 3 + randint0(4);
	width = size + randint0(10);
	height = size + randint0(4);

	/* Pick an upper-left corner at random. */
	c_y1 = y1 + randint0(1 + y2 - y1 - height);
	c_x1 = x1 + randint0(1 + x2 - x1 - width);

	/* Determine lower-right corner of chamber. */
	c_y2 = c_y1 + height;
	if (c_y2 > y2)
	    c_y2 = y2;

	c_x2 = c_x1 + width;
	if (c_x2 > x2)
	    c_x2 = x2;

	/* Make me a (magma filled) chamber. */
	make_chamber(c_y1, c_x1, c_y2, c_x2);
    }

    /* Remove useless doors, fill in tiny, narrow rooms. */
    for (y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++) {
	    count = 0;

	    /* Stay legal. */
	    if (!in_bounds_fully(y, x))
		continue;

	    /* Check all adjacent grids. */
	    for (d = 0; d < 8; d++) {
		/* Extract adjacent location */
		yy = y + ddy_ddd[d];
		xx = x + ddx_ddd[d];

		/* Count the walls and dungeon granite. */
		if ((cave_feat[yy][xx] == FEAT_WALL_INNER)
		    || (cave_feat[yy][xx] == FEAT_WALL_EXTRA))
		    count++;
	    }

	    /* Five adjacent walls: Change non-chamber to wall. */
	    if ((count == 5) && (cave_feat[y][x] != FEAT_MAGMA))
		cave_set_feat(y, x, FEAT_WALL_INNER);

	    /* More than five adjacent walls: Change anything to wall. */
	    else if (count > 5)
		cave_set_feat(y, x, FEAT_WALL_INNER);
	}
    }

    /* Pick a random magma spot near the center of the room. */
    for (i = 0; i < 50; i++) {
	y = y1 + ABS(y2 - y1) / 4 + randint0(ABS(y2 - y1) / 2);
	x = x1 + ABS(x2 - x1) / 4 + randint0(ABS(x2 - x1) / 2);
	if (cave_feat[y][x] == FEAT_MAGMA)
	    break;
    }


    /* Hollow out the first room. */
    cave_set_feat(y, x, FEAT_FLOOR);
    hollow_out_room(y, x);


    /* Attempt to change every in-room magma grid to open floor. */
    for (i = 0; i < 100; i++) {
	/* Assume this run will do no useful work. */
	bool joy = FALSE;

	/* Make new doors and tunnels between magma and open floor. */
	for (y = y1; y < y2; y++) {
	    for (x = x1; x < x2; x++) {
		/* Current grid must be magma. */
		if (cave_feat[y][x] != FEAT_MAGMA)
		    continue;

		/* Stay legal. */
		if (!in_bounds_fully(y, x))
		    continue;

		/* Check only horizontal and vertical directions. */
		for (d = 0; d < 4; d++) {
		    /* Extract adjacent location */
		    yy1 = y + ddy_ddd[d];
		    xx1 = x + ddx_ddd[d];

		    /* Find inner wall. */
		    if (cave_feat[yy1][xx1] == FEAT_WALL_INNER) {
			/* Keep going in the same direction. */
			yy2 = yy1 + ddy_ddd[d];
			xx2 = xx1 + ddx_ddd[d];

			/* If we find open floor, place a door. */
			if ((in_bounds(yy2, xx2))
			    && (cave_feat[yy2][xx2] == FEAT_FLOOR)) {
			    joy = TRUE;

			    /* Make a broken door in the wall grid. */
			    cave_set_feat(yy1, xx1, FEAT_BROKEN);

			    /* Hollow out the new room. */
			    cave_set_feat(y, x, FEAT_FLOOR);
			    hollow_out_room(y, x);

			    break;
			}

			/* If we find more inner wall... */
			if ((in_bounds(yy2, xx2))
			    && (cave_feat[yy2][xx2] == FEAT_WALL_INNER)) {
			    /* ...Keep going in the same direction. */
			    yy3 = yy2 + ddy_ddd[d];
			    xx3 = xx2 + ddx_ddd[d];

			    /* If we /now/ find floor, make a tunnel. */
			    if ((in_bounds(yy3, xx3))
				&& (cave_feat[yy3][xx3] == FEAT_FLOOR)) {
				joy = TRUE;

				/* Turn both wall grids into floor. */
				cave_set_feat(yy1, xx1, FEAT_FLOOR);
				cave_set_feat(yy2, xx2, FEAT_FLOOR);

				/* Hollow out the new room. */
				cave_set_feat(y, x, FEAT_FLOOR);
				hollow_out_room(y, x);

				break;
			    }
			}
		    }
		}
	    }
	}

	/* If we could find no work to do, stop. */
	if (!joy)
	    break;
    }


    /* Turn broken doors into a random kind of door, remove open doors. */
    for (y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++) {
	    if (cave_feat[y][x] == FEAT_OPEN)
		cave_set_feat(y, x, FEAT_WALL_INNER);
	    else if (cave_feat[y][x] == FEAT_BROKEN)
		place_random_door(y, x);
	}
    }


    /* Turn all walls and magma not adjacent to floor into dungeon granite. */
    /* Turn all floors and adjacent grids into rooms, sometimes lighting them. */
    for (y = (y1 - 1 > 0 ? y1 - 1 : 0);
	 y < (y2 + 2 < DUNGEON_HGT ? y2 + 2 : DUNGEON_HGT); y++) {
	for (x = (x1 - 1 > 0 ? x1 - 1 : 0);
	     x < (x2 + 2 < DUNGEON_WID ? x2 + 2 : DUNGEON_WID); x++) {
	    if ((cave_feat[y][x] == FEAT_WALL_INNER)
		|| (cave_feat[y][x] == FEAT_MAGMA)) {
		for (d = 0; d < 9; d++) {
		    /* Extract adjacent location */
		    int yy = y + ddy_ddd[d];
		    int xx = x + ddx_ddd[d];

		    /* Stay legal */
		    if (!in_bounds(yy, xx))
			continue;

		    /* No floors allowed */
		    if (cave_feat[yy][xx] == FEAT_FLOOR)
			break;

		    /* Turn me into dungeon granite. */
		    if (d == 8) {
			cave_set_feat(y, x, FEAT_WALL_EXTRA);
		    }
		}
	    }
	    if (tf_has(f_info[cave_feat[y][x]].flags, TF_FLOOR)) {
		for (d = 0; d < 9; d++) {
		    /* Extract adjacent location */
		    int yy = y + ddy_ddd[d];
		    int xx = x + ddx_ddd[d];

		    /* Stay legal */
		    if (!in_bounds(yy, xx))
			continue;

		    /* Turn into room. */
		    sqinfo_on(cave_info[yy][xx], SQUARE_ROOM);

		    /* Illuminate if requested. */
		    if (light)
			sqinfo_on(cave_info[yy][xx], SQUARE_GLOW);
		}
	    }
	}
    }


    /* Turn all inner wall grids adjacent to dungeon granite into outer walls. */
    for (y = (y1 - 1 > 0 ? y1 - 1 : 0);
	 y < (y2 + 2 < DUNGEON_HGT ? y2 + 2 : DUNGEON_HGT); y++) {
	for (x = (x1 - 1 > 0 ? x1 - 1 : 0);
	     x < (x2 + 2 < DUNGEON_WID ? x2 + 2 : DUNGEON_WID); x++) {
	    /* Stay legal. */
	    if (!in_bounds_fully(y, x))
		continue;

	    if (cave_feat[y][x] == FEAT_WALL_INNER) {
		for (d = 0; d < 9; d++) {
		    /* Extract adjacent location */
		    int yy = y + ddy_ddd[d];
		    int xx = x + ddx_ddd[d];

		    /* Look for dungeon granite */
		    if (cave_feat[yy][xx] == FEAT_WALL_EXTRA) {
			/* Turn me into outer wall. */
			cave_set_feat(y, x, FEAT_WALL_OUTER);

			/* Done; */
			break;
		    }
		}
	    }
	}
    }


    /*** Now we get to place the monsters. ***/
    get_chamber_monsters(y1, x1, y2, x2);

    /* Increase the level rating */
    rating += 10;

    /* Success. */
    return (TRUE);
}



/**
 * Hack -- fill in "vault" rooms and themed levels
 */
extern bool build_vault(int y0, int x0, int ymax, int xmax, const char *data,
			bool light, bool icky, byte vault_type)
{
    int x, y;
    int y1, x1, y2, x2, panic_y = 0, panic_x = 0;
    int temp, races = 0;

    bool placed = FALSE;

    const char *t;
    char racial_symbol[30] = "";

    /* Bail if no vaults allowed on this stage */
    if (no_vault())
	return (FALSE);

    /* Calculate the borders of the vault. */
    if (p_ptr->themed_level) {
	y1 = y0;
	x1 = x0;
	y2 = x0 + ymax - 1;
	x2 = x0 + xmax - 1;
    } else {
	y1 = y0 - (ymax / 2);
	x1 = x0 - (xmax / 2);
	y2 = y1 + ymax - 1;
	x2 = x1 + xmax - 1;
    }

    /* Make certain that the vault does not cross the dungeon edge. */
    if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2)))
	return (FALSE);


    /* No random monsters in vaults and interesting rooms. */
    if (!p_ptr->themed_level)
	generate_mark(y1, x1, y2, x2, SQUARE_TEMP);


    /* 
     * Themed levels usually have monster restrictions that take effect 
     * if no other restrictions are currently in force.  This can be ex-
     * panded to vaults too - imagine a "sorcerer's castle"...
     */
    if (p_ptr->themed_level)
	general_monster_restrictions();


    /* Place dungeon features and objects */
    for (t = data, y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++, t++) {
	    /* Hack -- skip "non-grids" */
	    if (*t == ' ') {
		continue;
	    }

	    /* Lay down a floor or grass */
	    if (((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
		 || (stage_map[p_ptr->stage][STAGE_TYPE] == DESERT)
		 || (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAIN))
		&& (p_ptr->themed_level != THEME_SLAIN))
		cave_set_feat(y, x, FEAT_FLOOR);
	    else
		cave_set_feat(y, x, FEAT_GRASS);

	    /* Part of a vault.  Can be lit.  May be "icky". */
	    if (icky)
	    {
		sqinfo_on(cave_info[y][x], SQUARE_ICKY);
		sqinfo_on(cave_info[y][x], SQUARE_ROOM);
	    }
	    else if (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
		sqinfo_on(cave_info[y][x], SQUARE_ROOM);
	    if (light)
		sqinfo_on(cave_info[y][x], SQUARE_GLOW);

	    /* Analyze the grid */
	    switch (*t) {
		/* Granite wall (outer) or outer edge of dungeon level or web. */
	    case '%':
		{
		    if (p_ptr->themed_level)
			cave_set_feat(y, x, FEAT_PERM_SOLID);
		    else if (stage_map[p_ptr->stage][STAGE_TYPE] == VALLEY)
		    {
			if (randint1(3) == 1)
			    cave_set_feat(y, x, FEAT_FLOOR);
			else if (randint1(2) == 1)
			    cave_set_feat(y, x, FEAT_TREE);
			else
			    cave_set_feat(y, x, FEAT_TREE2);

			place_trap(y, x, OBST_WEB, 0);
		    }
		    else
			cave_set_feat(y, x, FEAT_WALL_OUTER);
		    break;
		}
		/* Granite wall (inner) */
	    case '#':
		{
		    cave_set_feat(y, x, FEAT_WALL_INNER);
		    break;
		}
		/* Permanent wall (inner) */
	    case 'X':
		{
		    cave_set_feat(y, x, FEAT_PERM_INNER);
		    break;
		}
		/* Treasure seam, in either magma or quartz. */
	    case '*':
		{
		    if (randint1(2) == 1)
			cave_set_feat(y, x, FEAT_MAGMA_K);
		    else
			cave_set_feat(y, x, FEAT_QUARTZ_K);
		    break;
		}
		/* Lava. */
	    case '@':
		{
		    cave_set_feat(y, x, FEAT_LAVA);
		    break;
		}
		/* Water. */
	    case 'x':
		{
		    cave_set_feat(y, x, FEAT_WATER);
		    break;
		}
		/* Tree. */
	    case ';':
		{
		    if (randint1(p_ptr->depth + HIGHLAND_TREE_CHANCE)
			> HIGHLAND_TREE_CHANCE)
			cave_set_feat(y, x, FEAT_TREE2);
		    else
			cave_set_feat(y, x, FEAT_TREE);
		    break;
		}
		/* Rubble. */
	    case ':':
		{
		    cave_set_feat(y, x, FEAT_RUBBLE);
		    break;
		}
		/* Sand dune */
	    case '/':
		{
		    cave_set_feat(y, x, FEAT_DUNE);
		    break;
		}
		/* Treasure/trap */
	    case '&':
		{
		    if (randint0(100) < 50) {
			place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_VAULT);
		    } else {
			place_trap(y, x, -1, p_ptr->depth);
		    }
		    break;
		}
		/* Secret doors */
	    case '+':
		{
		    place_secret_door(y, x);
		    break;
		}
		/* Trap */
	    case '^':
		{
		    place_trap(y, x, -1, p_ptr->depth);
		    break;
		}
		/* Up stairs (and player location in themed level).  */
	    case '<':
		{
		    if (stage_map[p_ptr->stage][UP])
			cave_set_feat(y, x, FEAT_LESS);

		    /* Place player only in themed level, and only once. */
		    if ((p_ptr->themed_level) && (!placed)) {
			player_place(y, x);
			placed = TRUE;
		    }

		    break;
		}
		/* Down stairs. */
	    case '>':
		{
		    /* No down stairs at bottom or on quests */
		    if (is_quest(p_ptr->stage)
			|| (!stage_map[p_ptr->stage][DOWN]))
			break;

		    cave_set_feat(y, x, FEAT_MORE);
		    break;
		}
		/* Wilderness paths. */
	    case '\\':
		{
		    int adj;
		    byte dir;
		    bool more;

		    /* Work out which direction */
		    if (y == 1)
			dir = NORTH;
		    else if (x == 1)
			dir = WEST;
		    else if (y == DUNGEON_HGT - 2)
			dir = SOUTH;
		    else if (x == DUNGEON_WID - 2)
			dir = EAST;
		    else
			break;
		    adj = stage_map[p_ptr->stage][dir];

		    /* Cancel the path if nowhere to go */
		    if (!adj)
			break;

		    /* Set the feature */
		    more = (stage_map[adj][DEPTH] > p_ptr->depth);
		    switch (dir) {
		    case NORTH:
			{
			    if (more)
				cave_set_feat(y, x, FEAT_MORE_NORTH);
			    else
				cave_set_feat(y, x, FEAT_LESS_NORTH);
			    break;
			}
		    case EAST:
			{
			    if (more)
				cave_set_feat(y, x, FEAT_MORE_EAST);
			    else
				cave_set_feat(y, x, FEAT_LESS_EAST);
			    break;
			}
		    case SOUTH:
			{
			    if (more)
				cave_set_feat(y, x, FEAT_MORE_SOUTH);
			    else
				cave_set_feat(y, x, FEAT_LESS_SOUTH);
			    break;
			}
		    case WEST:
			{
			    if (more)
				cave_set_feat(y, x, FEAT_MORE_WEST);
			    else
				cave_set_feat(y, x, FEAT_LESS_WEST);
			    break;
			}
		    }

		    /* Place the player? */
		    if ((adj == p_ptr->last_stage) && (p_ptr->themed_level)
			&& (!placed)) {
			player_place(y, x);
			placed = TRUE;
		    } else {
			panic_y = y;
			panic_x = x;
		    }
		    break;
		}
	    }
	}
    }

    /* Place dungeon monsters and objects */
    for (t = data, y = y1; y <= y2; y++) {
	for (x = x1; x <= x2; x++, t++) {
	    /* Hack -- skip "non-grids" */
	    if (*t == ' ')
		continue;

	    /* Most alphabetic characters signify monster races. */
	    if (isalpha(*t) && (*t != 'x') && (*t != 'X')) {
		/* If the symbol is not yet stored, ... */
		if (!strchr(racial_symbol, *t)) {
		    /* ... store it for later processing. */
		    if (races < 30)
			racial_symbol[races++] = *t;
		}
	    }

	    /* Otherwise, analyze the symbol */
	    else
		switch (*t) {
		    /* An ordinary monster, object (sometimes good), or trap. */
		case '1':
		    {
			int rand = randint0(4);

			if (rand < 2) {
			    place_monster(y, x, TRUE, TRUE, FALSE);
			}

			/* I had not intended this function to create
			 * guaranteed "good" quality objects, but perhaps it's
			 * better that it does at least sometimes. */
			else if (rand == 2) {
			    if (randint0(8) == 0)
				place_object(y, x, TRUE, FALSE, FALSE, ORIGIN_VAULT);
			    else
				place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_VAULT);

			} else {
			    place_trap(y, x, -1, p_ptr->depth);
			}
			break;
		    }
		    /* Slightly out of depth monster. */
		case '2':
		    {
			monster_level = p_ptr->depth + 3;
			place_monster(y, x, TRUE, TRUE, FALSE);
			monster_level = p_ptr->depth;
			break;
		    }
		    /* Slightly out of depth object. */
		case '3':
		    {
			object_level = p_ptr->depth + 3;
			place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_VAULT);
			object_level = p_ptr->depth;
			break;
		    }
		    /* Monster and/or object */
		case '4':
		    {
			if (randint0(100) < 50) {
			    monster_level = p_ptr->depth + 4;
			    place_monster(y, x, TRUE, TRUE, FALSE);
			    monster_level = p_ptr->depth;
			}
			if (randint0(100) < 50) {
			    object_level = p_ptr->depth + 4;
			    place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_VAULT);
			    object_level = p_ptr->depth;
			}
			break;
		    }
		    /* Out of depth object. */
		case '5':
		    {
			object_level = p_ptr->depth + 7;
			place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_VAULT);
			object_level = p_ptr->depth;
			break;
		    }
		    /* Out of depth monster. */
		case '6':
		    {
			monster_level = p_ptr->depth + 7;
			place_monster(y, x, TRUE, TRUE, FALSE);
			monster_level = p_ptr->depth;
			break;
		    }
		    /* Very out of depth object. */
		case '7':
		    {
			object_level = p_ptr->depth + 15;
			place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_VAULT);
			object_level = p_ptr->depth;
			break;
		    }
		    /* Very out of depth monster. */
		case '8':
		    {
			monster_level = p_ptr->depth + 20;
			place_monster(y, x, TRUE, TRUE, FALSE);
			monster_level = p_ptr->depth;
			break;
		    }
		    /* Meaner monster, plus "good" (or better) object */
		case '9':
		    {
			monster_level = p_ptr->depth + 15;
			place_monster(y, x, TRUE, TRUE, FALSE);
			monster_level = p_ptr->depth;
			object_level = p_ptr->depth + 5;
			place_object(y, x, TRUE, FALSE, FALSE, ORIGIN_VAULT);
			object_level = p_ptr->depth;
			break;
		    }

		    /* Nasty monster and "great" (or better) object */
		case '0':
		    {
			monster_level = p_ptr->depth + 30;
			place_monster(y, x, TRUE, TRUE, FALSE);
			monster_level = p_ptr->depth;
			object_level = p_ptr->depth + 15;
			place_object(y, x, TRUE, TRUE, FALSE, ORIGIN_VAULT);
			object_level = p_ptr->depth;
			break;
		    }

		    /* A chest. */
		case '~':
		    {
			required_tval = TV_CHEST;

			object_level = p_ptr->depth + 5;
			place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Treasure. */
		case '$':
		    {
			place_gold(y, x);
			break;
		    }
		    /* Armour. */
		case ']':
		    {
			object_level = p_ptr->depth + 3;

			if (randint1(3) == 1)
			    temp = randint1(9);
			else
			    temp = randint1(8);

			if (temp == 1)
			    required_tval = TV_BOOTS;
			else if (temp == 2)
			    required_tval = TV_GLOVES;
			else if (temp == 3)
			    required_tval = TV_HELM;
			else if (temp == 4)
			    required_tval = TV_CROWN;
			else if (temp == 5)
			    required_tval = TV_SHIELD;
			else if (temp == 6)
			    required_tval = TV_CLOAK;
			else if (temp == 7)
			    required_tval = TV_SOFT_ARMOR;
			else if (temp == 8)
			    required_tval = TV_HARD_ARMOR;
			else
			    required_tval = TV_DRAG_ARMOR;

			place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Weapon. */
		case '|':
		    {
			object_level = p_ptr->depth + 3;

			temp = randint1(4);

			if (temp == 1)
			    required_tval = TV_SWORD;
			else if (temp == 2)
			    required_tval = TV_POLEARM;
			else if (temp == 3)
			    required_tval = TV_HAFTED;
			else if (temp == 4)
			    required_tval = TV_BOW;

			place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Ring. */
		case '=':
		    {
			required_tval = TV_RING;

			object_level = p_ptr->depth + 3;
			if (randint1(4) == 1)
			    place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			else
			    place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Amulet. */
		case '"':
		    {
			required_tval = TV_AMULET;

			object_level = p_ptr->depth + 3;
			if (randint1(4) == 1)
			    place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			else
			    place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Potion. */
		case '!':
		    {
			required_tval = TV_POTION;

			object_level = p_ptr->depth + 3;
			if (randint1(4) == 1)
			    place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			else
			    place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Scroll. */
		case '?':
		    {
			required_tval = TV_SCROLL;

			object_level = p_ptr->depth + 3;
			if (randint1(4) == 1)
			    place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			else
			    place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Staff. */
		case '_':
		    {
			required_tval = TV_STAFF;

			object_level = p_ptr->depth + 3;
			if (randint1(4) == 1)
			    place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			else
			    place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Wand or rod. */
		case '-':
		    {
			if (randint0(100) < 50)
			    required_tval = TV_WAND;
			else
			    required_tval = TV_ROD;

			object_level = p_ptr->depth + 3;
			if (randint1(4) == 1)
			    place_object(y, x, TRUE, FALSE, TRUE, ORIGIN_VAULT);
			else
			    place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		    /* Food or mushroom. */
		case ',':
		    {
			required_tval = TV_FOOD;

			object_level = p_ptr->depth + 3;
			place_object(y, x, FALSE, FALSE, TRUE, ORIGIN_VAULT);
			object_level = p_ptr->depth;

			required_tval = 0;

			break;
		    }
		}
	}
    }

    get_vault_monsters(racial_symbol, vault_type, data, y1, y2, x1, x2);

    /* Ensure that the player is always placed in a themed level. */
    if ((p_ptr->themed_level) && (!placed)) {
	if (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
	    new_player_spot();
	else
	    player_place(panic_y, panic_x);
    }

    /* Success. */
    return (TRUE);
}


/**
 * Type 7 -- interesting rooms. -LM-
 */
static bool build_type7(void)
{
    vault_type *v_ptr;
    int i, y, x;
    int *v_idx = malloc(z_info->v_max * sizeof(*v_idx));
    int v_cnt = 0;

    /* Examine each vault */
    for (i = 0; i < z_info->v_max; i++) {
	/* Access the vault */
	v_ptr = &v_info[i];

	/* Accept each interesting room that is acceptable for this depth. */
	if ((v_ptr->typ == 7) && (v_ptr->min_lev <= p_ptr->depth)
	    && (v_ptr->max_lev >= p_ptr->depth)) {
	    v_idx[v_cnt++] = i;
	}
    }

    /* Access a random vault record */
    v_ptr = &v_info[v_idx[randint0(v_cnt)]];

    if (!find_space(&y, &x, v_ptr->hgt, v_ptr->wid)) {
	free(v_idx);
	return (FALSE);
    }

    /* Boost the rating */
    rating += v_ptr->rat;


    /* Build the vault (sometimes lit, not icky, type 7) */
    if (!build_vault
	(y, x, v_ptr->hgt, v_ptr->wid, v_ptr->text,
	 (p_ptr->depth < randint0(37)), FALSE, 7)) {
	free(v_idx);
	return (FALSE);
    }

    free(v_idx);

    return (TRUE);
}

/**
 * Type 8 -- lesser vaults.
 */
static bool build_type8(void)
{
    vault_type *v_ptr;
    int i, y, x;
    int *v_idx = malloc(z_info->v_max * sizeof(*v_idx));
    int v_cnt = 0;

    /* Examine each vault */
    for (i = 0; i < z_info->v_max; i++) {
	/* Access the vault */
	v_ptr = &v_info[i];

	/* Accept each lesser vault that is acceptable for this depth. */
	if ((v_ptr->typ == 8) && (v_ptr->min_lev <= p_ptr->depth)
	    && (v_ptr->max_lev >= p_ptr->depth)) {
	    v_idx[v_cnt++] = i;
	}
    }

    /* Access a random vault record */
    v_ptr = &v_info[v_idx[randint0(v_cnt)]];


    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y, &x, v_ptr->hgt, v_ptr->wid)) {
	free(v_idx);
	return (FALSE);
    }


    /* Message */
    if (OPT(cheat_room))
	msg("Lesser Vault");

    /* Boost the rating */
    rating += v_ptr->rat;

    /* Build the vault (never lit, icky, type 8) */
    if (!build_vault
	(y, x, v_ptr->hgt, v_ptr->wid, v_ptr->text, FALSE, TRUE, 8)) {
	free(v_idx);
	return (FALSE);
    }

    free(v_idx);

    return (TRUE);
}



/**
 * Type 9 -- greater vaults.
 */
static bool build_type9(void)
{
    vault_type *v_ptr;
    int i, y, x;
    int *v_idx = malloc(z_info->v_max * sizeof(int));
    int v_cnt = 0;

    /* Examine each vault */
    for (i = 0; i < z_info->v_max; i++) {
	/* Access the vault */
	v_ptr = &v_info[i];

	/* Accept each greater vault that is acceptable for this depth. */
	if ((v_ptr->typ == 9) && (v_ptr->min_lev <= p_ptr->depth)
	    && (v_ptr->max_lev >= p_ptr->depth)) {
	    v_idx[v_cnt++] = i;
	}
    }

    /* Access a random vault record */
    v_ptr = &v_info[v_idx[randint0(v_cnt)]];


    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(&y, &x, v_ptr->hgt, v_ptr->wid)) {
	free(v_idx);
	return (FALSE);
    }


    /* Message */
    if (OPT(cheat_room))
	msg("Greater Vault");

    /* Boost the rating */
    rating += v_ptr->rat;

    /* Build the vault (never lit, icky, type 9) */
    if (!build_vault
	(y, x, v_ptr->hgt, v_ptr->wid, v_ptr->text, FALSE, TRUE, 9)) {
	free(v_idx);
	return (FALSE);
    }

    free(v_idx);

    return (TRUE);
}





/**
 * Type 10 -- Extremely large rooms.
 * 
 * These are the largest, most difficult to position, and thus highest-
 * priority rooms in the dungeon.  They should be rare, so as not to 
 * interfere with greater vaults.
 * 
 *                     (huge chamber)
 * - A single starburst-shaped room of extreme size, usually dotted or 
 * even divided with irregularly-shaped fields of rubble. No special 
 * monsters.  Appears deeper than level 40.
 *
 */
static bool build_type10(void)
{
    bool light;

    int i, count;

    int y0, x0, y1, x1, y2, x2;
    int y1_tmp, x1_tmp, y2_tmp, x2_tmp;
    int width, height;
    int width_tmp, height_tmp;


    /* Huge cave room */
    if (p_ptr->depth > 40) {
	/* This room is usually lit. */
	if (randint0(3) != 0)
	    light = TRUE;
	else
	    light = FALSE;

	/* Get a size */
	height = (2 + randint1(2)) * BLOCK_HGT;
	width = (3 + randint1(6)) * BLOCK_WID;

	/* Find and reserve some space.  Get center of room. */
	if (!find_space(&y0, &x0, height, width))
	    return (FALSE);

	/* Locate the room */
	y1 = y0 - height / 2;
	x1 = x0 - width / 2;
	y2 = y1 + height - 1;
	x2 = x1 + width - 1;

	/* Make a huge starburst room with optional light. */
	if (!generate_starburst_room(y1, x1, y2, x2, light, FEAT_FLOOR, FALSE))
	    return (FALSE);


	/* Often, add rubble to break things up a bit. */
	if (randint1(5) > 2) {
	    /* Determine how many rubble fields to add (between 1 and 6). */
	    count = height * width * randint1(2) / 1100;

	    /* Make the rubble fields. */
	    for (i = 0; i < count; i++) {
		height_tmp = 8 + randint0(16);
		width_tmp = 10 + randint0(24);

		/* Semi-random location. */
		y1_tmp = y1 + randint0(height - height_tmp);
		x1_tmp = x1 + randint0(width - width_tmp);
		y2_tmp = y1_tmp + height_tmp;
		x2_tmp = x1_tmp + width_tmp;

		/* Make the rubble field. */
		generate_starburst_room(y1_tmp, x1_tmp, y2_tmp, x2_tmp, FALSE,
					FEAT_RUBBLE, FALSE);
	    }
	}

	/* Success. */
	return (TRUE);
    }


    /* No room was selected, so none was built. */
    return (FALSE);
}

/**
 * Build a room of the given type.
 * 
 * Check to see if there will probably be enough space in the monster 
 * and object arrays.
 */
extern bool room_build(int room_type)
{
    /* If trying to build a special room, check some limits first. */
    if (room_type > 4) {
	/* Help prevent object over-flow */
	if (o_max > 3 * z_info->o_max / 4) {
	    return (FALSE);
	}

	/* Help prevent monster over-flow */
	if (m_max > 3 * z_info->m_max / 4) {
	    return (FALSE);
	}
    }


    /* Build a room */
    switch (room_type) {
	/* Find space for, position, and build the room asked for */
    case 10:
	if (!build_type10())
	    return (FALSE);
	break;
    case 9:
	if (!build_type9())
	    return (FALSE);
	break;
    case 8:
	if (!build_type8())
	    return (FALSE);
	break;
    case 7:
	if (!build_type7())
	    return (FALSE);
	break;
    case 6:
	if (!build_type6())
	    return (FALSE);
	break;
    case 5:
	if (!build_type5())
	    return (FALSE);
	break;
    case 4:
	if (!build_type4())
	    return (FALSE);
	break;
    case 3:
	if (!build_type3())
	    return (FALSE);
	break;
    case 2:
	if (!build_type2())
	    return (FALSE);
	break;
    case 1:
	if (!build_type1())
	    return (FALSE);
	break;

	/* Paranoia */
    default:
	return (FALSE);
    }

    /* Success */
    return (TRUE);
}


