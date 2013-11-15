/*
 * File: pathfind.c
 * Purpose: Pathfinding and running code.
 *
 * Copyright (c) 1988 Christopher J Stuart (running code)
 * Copyright (c) 2004-2007 Christophe Cavalaria, Leon Marrick (pathfinding)
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
#include "squelch.h"
#include "trap.h"

/****** Pathfinding code ******/
/**
 * Maximum size around the player to consider in the pathfinder
 */
#define MAX_PF_RADIUS 50

/**
 * Maximum distance to consider in the pathfinder
 */
#define MAX_PF_LENGTH 250

static int terrain[MAX_PF_RADIUS][MAX_PF_RADIUS];
char pf_result[MAX_PF_LENGTH];
int pf_result_index;

static int ox, oy, ex, ey;

bool is_valid_pf(int y, int x)
{
    feature_type *f_ptr = NULL;
    int feat = cave_feat[y][x];

    /* Hack -- assume unvisited is permitted */
    if (!sqinfo_has(cave_info[y][x], SQUARE_MARK))
	return (TRUE);

    /* Get mimiced feat */
    f_ptr = &f_info[f_info[feat].mimic];

    /* Optionally alter known traps/doors on movement */
    if (OPT(easy_alter) && (tf_has(f_ptr->flags, TF_DOOR_CLOSED)
			    || cave_visible_trap(y, x))) 
    {
	return (TRUE);
    }
    else if (cave_visible_trap(y, x) && cave_player_trap(y, x))
	return (FALSE);

    /* Require moveable space */
    if (tf_has(f_ptr->flags, TF_WALL))
	return (FALSE);

    /* Don't move over lava or void */
    if (tf_has(f_ptr->flags, TF_FIERY) || tf_has(f_ptr->flags, TF_FALL))
	return (FALSE);

    /* Otherwise good */
    return (TRUE);
}

static void fill_terrain_info(void)
{
    int i, j;

    ox = MAX(p_ptr->px - MAX_PF_RADIUS / 2, 0);
    oy = MAX(p_ptr->py - MAX_PF_RADIUS / 2, 0);

    ex = MIN(p_ptr->px + MAX_PF_RADIUS / 2 - 1, DUNGEON_WID);
    ey = MIN(p_ptr->py + MAX_PF_RADIUS / 2 - 1, DUNGEON_HGT);

    for (i = 0; i < MAX_PF_RADIUS * MAX_PF_RADIUS; i++)
	terrain[0][i] = -1;

    for (j = oy; j < ey; j++)
	for (i = ox; i < ex; i++)
	    if (is_valid_pf(j, i))
		terrain[j - oy][i - ox] = MAX_PF_LENGTH;

    terrain[p_ptr->py - oy][p_ptr->px - ox] = 1;
}

#define MARK_DISTANCE(c,d) if ((c <= MAX_PF_LENGTH) && (c > d)) \
                              { c = d; try_again = (TRUE); }

bool findpath(int y, int x)
{
    int i, j, k, dir, starty = 0, startx = 0, start_index;
    bool try_again;
    int cur_distance;
    int findir[] = { 1, 4, 7, 8, 9, 6, 3, 2 };

    fill_terrain_info();

    terrain[p_ptr->py - oy][p_ptr->px - ox] = 1;

    if ((x >= ox) && (x < ex) && (y >= oy) && (y < ey)) {
	if ((cave_m_idx[y][x] > 0) && (m_list[cave_m_idx[y][x]].ml)) {
	    terrain[y - oy][x - ox] = MAX_PF_LENGTH;
	}
	/* else if (terrain[y - oy][x - ox] != MAX_PF_LENGTH) { bell("Target
	 * blocked"); return (FALSE); } */
	terrain[y - oy][x - ox] = MAX_PF_LENGTH;
    } else {
	bell("Target out of range.");
	return (FALSE);
    }

    if (terrain[y - oy][x - ox] == -1) {
	bell("Target space forbidden");
	return (FALSE);
    }


    /* 
     * And now starts the very naive and very 
     * inefficient pathfinding algorithm
     */
    do {
	try_again = (FALSE);
	for (j = oy + 1; j < ey - 1; j++)
	    for (i = ox + 1; i < ex - 1; i++) {
		cur_distance = terrain[j - oy][i - ox] + 1;
		if ((cur_distance > 0) && (cur_distance < MAX_PF_LENGTH)) {
		    for (dir = 1; dir < 10; dir++) {
			if (dir == 5)
			    continue;
			MARK_DISTANCE(terrain[j - oy + ddy[dir]]
				      [i - ox + ddx[dir]], cur_distance);
		    }
		}
	    }
	if (terrain[y - oy][x - ox] < MAX_PF_LENGTH)
	    try_again = (FALSE);
    } while (try_again);

    /* Failure */
    if (terrain[y - oy][x - ox] == MAX_PF_LENGTH) {
	bell("Target space unreachable.");
	return (FALSE);
    }

    /* Success */
    i = x;
    j = y;

    pf_result_index = 0;

    while ((i != p_ptr->px) || (j != p_ptr->py)) {
	int xdiff = i - p_ptr->px, ydiff = j - p_ptr->py;

	cur_distance = terrain[j - oy][i - ox] - 1;

	/* Starting direction */
	if (xdiff < 0)
	    startx = 1;
	else if (xdiff > 0)
	    startx = -1;
	else
	    startx = 0;

	if (ydiff < 0)
	    starty = 1;
	else if (ydiff > 0)
	    starty = -1;
	else
	    starty = 0;

	for (dir = 1; dir < 10; dir++)
	    if ((ddx[dir] == startx) && (ddy[dir] == starty))
		break;

	/* Should never happend */
	if ((dir % 5) == 0) {
	    bell("Wtf ?");
	    return (FALSE);
	}

	for (start_index = 0; findir[start_index % 8] != dir; start_index++);

	for (k = 0; k < 5; k++) {
	    dir = findir[(start_index + k) % 8];
	    if (terrain[j - oy + ddy[dir]][i - ox + ddx[dir]]
		== cur_distance)
		break;
	    dir = findir[(8 + start_index - k) % 8];
	    if (terrain[j - oy + ddy[dir]][i - ox + ddx[dir]]
		== cur_distance)
		break;
	}

	/* Should never happend */
	if (k == 5) {
	    bell("Heyyy !");
	    return (FALSE);
	}

	pf_result[pf_result_index++] = '0' + (char) (10 - dir);
	i += ddx[dir];
	j += ddy[dir];
	starty = 0;
	startx = 0;
    }
    pf_result_index--;
    return (TRUE);
}


/**
 * Hack -- Check for a "known wall" (see below)
 */
static int see_wall(int dir, int y, int x)
{
    feature_type *f_ptr;

    /* Get the new location */
    y += ddy[dir];
    x += ddx[dir];

    /* Illegal grids are not known walls XXX XXX XXX */
    if (!in_bounds(y, x))
	return (FALSE);

    f_ptr = &f_info[cave_feat[y][x]];

    /* Non-wall grids are not known walls */
    if (!tf_has(f_ptr->flags, TF_WALL))
	return (FALSE);

    /* Unknown walls are not known walls */
    if (!sqinfo_has(cave_info[y][x], SQUARE_MARK))
	return (FALSE);

    /* Default */
    return (TRUE);
}


/*
 * Calculates and returns the angle to the target or in the given
 * direction.
 *
 * Note:  If a compass direction is supplied, we ignore any target.
 * Note:  We supply the angle divided by 2.
 */
int get_angle_to_target(int y0, int x0, int y1, int x1, int dir)
{
	int ny, nx;
	int dist_conv;

	/* No valid compass direction given */
	if ((dir == 0) || (dir == 5) || (dir > 9))
	{
		/* Check for a valid target */
		if ((y1) && (x1))
		{
			/* Get absolute distance between source and target */
			int dy = ABS(y1 - y0);
			int dx = ABS(x1 - x0);

			/* Calculate distance conversion factor */
			if ((dy > 20) || (dx > 20))
			{
				/* Must shrink the distance to avoid illegal table access */
				if (dy > dx) dist_conv = 1 + (10 * dy / 20);
				else         dist_conv = 1 + (10 * dx / 20);
			}
			else
			{
				dist_conv = 10;
			}
			/* Convert and reorient grid for table access */
			ny = 20 + 10 * (y1 - y0) / dist_conv;
			nx = 20 + 10 * (x1 - x0) / dist_conv;

			/* Illegal table access is bad */
			if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40))
			{
				/* Note error */
				return (-1);
			}
		}

		/* No compass direction and no target --> note error */
		else
		{
			return (-1);
		}
	}

	/* We have a valid compass direction */
	else
	{
		/* Step in that direction a bunch of times, get target */
		y1 = y0 + (ddy_ddd[dir] * 10);
		x1 = x0 + (ddx_ddd[dir] * 10);

		/* Convert to table grids */
		ny = 20 + (y1 - y0);
		nx = 20 + (x1 - x0);
	}

	/* Get angle to target. */
	return (get_angle_to_grid[ny][nx]);
}

/**
 * The running algorithm  -CJS-
 *
 * Basically, once you start running, you keep moving until something
 * interesting happens.   In an enclosed space, you run straight, but
 * you follow corners as needed (i.e. hallways).  In an open space,
 * you run straight, but you stop before entering an enclosed space
 * (i.e. a room with a doorway).  In a semi-open space (with walls on
 * one side only), you run straight, but you stop before entering an
 * enclosed space or an open space (i.e. running along side a wall).
 *
 * All discussions below refer to what the player can see, that is,
 * an unknown wall is just like a normal floor.   This means that we
 * must be careful when dealing with "illegal" grids.
 *
 * No assumptions are made about the layout of the dungeon, so this
 * algorithm works in hallways, rooms, town, destroyed areas, etc.
 *
 * In the diagrams below, the player has just arrived in the grid
 * marked as '@', and he has just come from a grid marked as 'o',
 * and he is about to enter the grid marked as 'x'.
 *
 * Running while confused is not allowed, and so running into a wall
 * is only possible when the wall is not seen by the player.  This
 * will take a turn and stop the running.
 *
 * Several conditions are tracked by the running variables.
 *
 *   p_ptr->run_open_area (in the open on at least one side)
 *   p_ptr->run_break_left (wall on the left, stop if it opens)
 *   p_ptr->run_break_right (wall on the right, stop if it opens)
 *
 * When running begins, these conditions are initialized by examining
 * the grids adjacent to the requested destination grid (marked 'x'),
 * two on each side (marked 'L' and 'R').  If either one of the two
 * grids on a given side is a wall, then that side is considered to
 * be "closed".   Both sides enclosed yields a hallway.
 *<pre>
 *    LL		     @L
 *    @x      (normal)	     RxL   (diagonal)
 *    RR      (east)	      R	   (south-east)
 *</pre>
 * In the diagram below, in which the player is running east along a
 * hallway, he will stop as indicated before attempting to enter the
 * intersection (marked 'x').  Starting a new run in any direction
 * will begin a new hallway run.
 *<pre>
 * #.#
 * ##.##
 * o@x..
 * ##.##
 * #.#
 </pre>
 * Note that a minor hack is inserted to make the angled corridor
 * entry (with one side blocked near and the other side blocked
 * further away from the runner) work correctly. The runner moves
 * diagonally, but then saves the previous direction as being
 * straight into the gap. Otherwise, the tail end of the other
 * entry would be perceived as an alternative on the next move.
 *
 * In the diagram below, the player is running east down a hallway,
 * and will stop in the grid (marked '1') before the intersection.
 * Continuing the run to the south-east would result in a long run
 * stopping at the end of the hallway (marked '2').
 *<pre>
 * ##################
 * o@x	     1
 * ########### ######
 * #2	       #
 * #############
 *</pre>
 * After each step, the surroundings are examined to determine if
 * the running should stop, and to determine if the running should
 * change direction.  We examine the new current player location
 * (at which the runner has just arrived) and the direction from
 * which the runner is considered to have come.
 *
 * Moving one grid in some direction places you adjacent to three
 * or five new grids (for straight and diagonal moves respectively)
 * to which you were not previously adjacent (marked as '!').
 *<pre>
 *   ...!	       ...
 *   .o@!  (normal)    .o.!  (diagonal)
 *   ...!  (east)      ..@!  (south east)
 *			!!!
 *</pre>
 * If any of the newly adjacent grids are "interesting" (monsters,
 * objects, some terrain features) then running stops.
 *
 * If any of the newly adjacent grids seem to be open, and you are
 * looking for a break on that side, then running stops.
 *
 * If any of the newly adjacent grids do not seem to be open, and
 * you are in an open area, and the non-open side was previously
 * entirely open, then running stops.
 *
 * If you are in a hallway, then the algorithm must determine if
 * the running should continue, turn, or stop.  If only one of the
 * newly adjacent grids appears to be open, then running continues
 * in that direction, turning if necessary.  If there are more than
 * two possible choices, then running stops.  If there are exactly
 * two possible choices, separated by a grid which does not seem
 * to be open, then running stops.  Otherwise, as shown below, the
 * player has probably reached a "corner".
 *<pre>
 *    ###	      o##
 *    o@x  (normal)   #@!   (diagonal)
 *    ##!  (east)     ##x   (south east)
 *</pre>
 * In this situation, there will be two newly adjacent open grids,
 * one touching the player on a diagonal, and one directly adjacent.
 * We must consider the two "option" grids further out (marked '?').
 * We assign "option" to the straight-on grid, and "option2" to the
 * diagonal grid.  For some unknown reason, we assign "check_dir" to
 * the grid marked 's', which may be incorrectly labelled.
 *
 *    ###s
 *    o@x?   (may be incorrect diagram!)
 *    ##!?
 *
 * If both "option" grids are closed, then there is no reason to enter
 * the corner, and so we can cut the corner, by moving into the other
 * grid (diagonally).  If we choose not to cut the corner, then we may
 * go straight, but we pretend that we got there by moving diagonally.
 * Below, we avoid the obvious grid (marked 'x') and cut the corner
 * instead (marked 'n').
 *<pre>
 *    ###:		 o##
 *    o@x#   (normal)	 #@n	(maybe?)
 *    ##n#   (east)	 ##x#
 *			 ####
 *</pre>
 * If one of the "option" grids is open, then we may have a choice, so
 * we check to see whether it is a potential corner or an intersection
 * (or room entrance).  If the grid two spaces straight ahead, and the
 * space marked with 's' are both open, then it is a potential corner
 * and we enter it if requested.  Otherwise, we stop, because it is
 * not a corner, and is instead an intersection or a room entrance.
 *<pre>
 *    ###
 *    o@x
 *    ##!#
 *</pre>
 * I do not think this documentation is correct.
 */




/**
 * Hack -- allow quick "cycling" through the legal directions
 */
static byte cycle[] = { 1, 2, 3, 6, 9, 8, 7, 4, 1, 2, 3, 6, 9, 8, 7, 4, 1 };

/**
 * Hack -- map each direction into the "middle" of the "cycle[]" array
 */
static byte chome[] = { 0, 8, 9, 10, 7, 0, 11, 6, 5, 4 };



/**
 * Initialize the running algorithm for a new direction.
 *
 * Diagonal Corridor -- allow diaginal entry into corridors.
 *
 * Blunt Corridor -- If there is a wall two spaces ahead and
 * we seem to be in a corridor, then force a turn into the side
 * corridor, must be moving straight into a corridor here. ???
 *
 * Diagonal Corridor	Blunt Corridor (?)
 *	 # #		      #
 *	 #x#		     @x#
 *	 @p.		      p
 */
static void run_init(int dir)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int i, row, col;

    bool deepleft, deepright;
    bool shortleft, shortright;


    /* Save the direction */
    p_ptr->run_cur_dir = dir;

    /* Assume running straight */
    p_ptr->run_old_dir = dir;

    /* If it's wilderness, done -NRM- */
    if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
	&& (stage_map[p_ptr->stage][STAGE_TYPE] != TOWN))
	return;

    /* Assume looking for open area */
    p_ptr->run_open_area = TRUE;

    /* Assume not looking for breaks */
    p_ptr->run_break_right = FALSE;
    p_ptr->run_break_left = FALSE;

    /* Assume no nearby walls */
    deepleft = deepright = FALSE;
    shortright = shortleft = FALSE;

    /* Find the destination grid */
    row = py + ddy[dir];
    col = px + ddx[dir];

    /* Extract cycle index */
    i = chome[dir];

    /* Check for nearby wall */
    if (see_wall(cycle[i + 1], py, px)) {
	p_ptr->run_break_left = TRUE;
	shortleft = TRUE;
    }

    /* Check for distant wall */
    else if (see_wall(cycle[i + 1], row, col)) {
	p_ptr->run_break_left = TRUE;
	deepleft = TRUE;
    }

    /* Check for nearby wall */
    if (see_wall(cycle[i - 1], py, px)) {
	p_ptr->run_break_right = TRUE;
	shortright = TRUE;
    }

    /* Check for distant wall */
    else if (see_wall(cycle[i - 1], row, col)) {
	p_ptr->run_break_right = TRUE;
	deepright = TRUE;
    }

    /* Looking for a break */
    if (p_ptr->run_break_left && p_ptr->run_break_right) {
	/* Not looking for open area */
	p_ptr->run_open_area = FALSE;

	/* Hack -- allow angled corridor entry */
	if (dir & 0x01) {
	    if (deepleft && !deepright) {
		p_ptr->run_old_dir = cycle[i - 1];
	    } else if (deepright && !deepleft) {
		p_ptr->run_old_dir = cycle[i + 1];
	    }
	}

	/* Hack -- allow blunt corridor entry */
	else if (see_wall(cycle[i], row, col)) {
	    if (shortleft && !shortright) {
		p_ptr->run_old_dir = cycle[i - 2];
	    } else if (shortright && !shortleft) {
		p_ptr->run_old_dir = cycle[i + 2];
	    }
	}
    }
}


/**
 * Update the current "run" path
 *
 * Return TRUE if the running should be stopped
 */
static bool run_test(void)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int prev_dir;
    int new_dir;
    int left_dir;
    int right_dir;

    int row, col;
    int i, max, inv;
    int option, option2;

    feature_type *f_ptr;

    /* No options yet */
    option = 0;
    option2 = 0;

    /* Where we came from */
    prev_dir = p_ptr->run_old_dir;

    /* Range of newly adjacent grids */
    max = (prev_dir & 0x01) + 1;

    /* Simplistic running for outdoors -NRM- */
    if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
	&& (stage_map[p_ptr->stage][STAGE_TYPE] != TOWN)) {
	/* Look at every newly adjacent square. */
	for (i = -max; i <= max; i++) {
	    s16b this_o_idx, next_o_idx = 0;


	    /* New direction */
	    new_dir = cycle[chome[prev_dir] + i];

	    /* New location */
	    row = py + ddy[new_dir];
	    col = px + ddx[new_dir];


	    /* Visible monsters abort running */
	    if (cave_m_idx[row][col] > 0) {
		monster_type *m_ptr = &m_list[cave_m_idx[row][col]];

		/* Visible monster */
		if (m_ptr->ml)
		    return (TRUE);
	    }

	    /* Visible objects abort running */
	    for (this_o_idx = cave_o_idx[row][col]; this_o_idx;
		 this_o_idx = next_o_idx) {
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Visible object */
		if (o_ptr->marked && !squelch_hide_item(o_ptr))
		    return (TRUE);
	    }
	}

	/* Assume main direction */
	new_dir = p_ptr->run_old_dir;
	row = py + ddy[new_dir];
	col = px + ddx[new_dir];
	f_ptr = &f_info[cave_feat[row][col]];


	/* Step if there's a path in the right direction */
	if (tf_has(f_ptr->flags, TF_RUN1) && !cave_visible_trap(row, col)) 
	{
	    p_ptr->run_cur_dir = new_dir;
	    return (FALSE);
	}

	/* Check to the left */
	left_dir = cycle[chome[prev_dir] - 1];
	row = py + ddy[left_dir];
	col = px + ddx[left_dir];
	f_ptr = &f_info[cave_feat[row][col]];
	if (tf_has(f_ptr->flags, TF_RUN1) && !cave_visible_trap(row, col)) 
	    option = left_dir;

	/* Check to the right */
	right_dir = cycle[chome[prev_dir] + 1];
	row = py + ddy[right_dir];
	col = px + ddx[right_dir];
	f_ptr = &f_info[cave_feat[row][col]];
	if (tf_has(f_ptr->flags, TF_RUN1) && !cave_visible_trap(row, col)) 
	    option2 = right_dir;

	/* Stop if it's a fork */
	if (option && option2)
	    return (TRUE);

	/* Otherwise step in the secondary direction */
	if (option) {
	    p_ptr->run_cur_dir = left_dir;
	    return (FALSE);
	} else if (option2) {
	    p_ptr->run_cur_dir = right_dir;
	    return (FALSE);
	}

	/* No paths, so try grass */
	row = py + ddy[new_dir];
	col = px + ddx[new_dir];
	f_ptr = &f_info[cave_feat[row][col]];


	/* Step if there's grass in the right direction */
	if (tf_has(f_ptr->flags, TF_RUN2) && !cave_visible_trap(row, col)) 
	{
	    p_ptr->run_cur_dir = new_dir;
	    return (FALSE);
	}

	/* Check to the left */
	row = py + ddy[left_dir];
	col = px + ddx[left_dir];
	f_ptr = &f_info[cave_feat[row][col]];
	if (tf_has(f_ptr->flags, TF_RUN2) && !cave_visible_trap(row, col)) 
	    option = left_dir;

	/* Check to the right */
	right_dir = cycle[chome[prev_dir] + 1];
	row = py + ddy[right_dir];
	col = px + ddx[right_dir];
	f_ptr = &f_info[cave_feat[row][col]];
	if (tf_has(f_ptr->flags, TF_RUN2) && !cave_visible_trap(row, col)) 
	    option2 = right_dir;

	/* Stop if it's a fork */
	if (option && option2)
	    return (TRUE);

	/* Otherwise step in the secondary direction */
	if (option) {
	    p_ptr->run_cur_dir = left_dir;
	    return (FALSE);
	} else if (option2) {
	    p_ptr->run_cur_dir = right_dir;
	    return (FALSE);
	}

    }

    /* Look at every newly adjacent square. */
    for (i = -max; i <= max; i++) {
	s16b this_o_idx, next_o_idx = 0;


	/* New direction */
	new_dir = cycle[chome[prev_dir] + i];

	/* New location */
	row = py + ddy[new_dir];
	col = px + ddx[new_dir];


	/* Visible monsters abort running */
	if (cave_m_idx[row][col] > 0) {
	    monster_type *m_ptr = &m_list[cave_m_idx[row][col]];

	    /* Visible monster */
	    if (m_ptr->ml)
		return (TRUE);
	}

	/* Visible objects abort running */
	for (this_o_idx = cave_o_idx[row][col]; this_o_idx;
	     this_o_idx = next_o_idx) {
	    object_type *o_ptr;

	    /* Acquire object */
	    o_ptr = &o_list[this_o_idx];

	    /* Acquire next object */
	    next_o_idx = o_ptr->next_o_idx;

	    /* Visible object */
	    if (o_ptr->marked)
		return (TRUE);
	}


	/* Assume unknown */
	inv = TRUE;

	/* Check memorized grids */
	if (sqinfo_has(cave_info[row][col], SQUARE_MARK)) 
	{
	    bool notice = TRUE;

	    /* Examine the terrain */
	    f_ptr = &f_info[cave_feat[row][col]];
	    
	    /* Open doors */
	    if (tf_has(f_ptr->flags, TF_DOOR_ANY) && 
		tf_has(f_ptr->flags, TF_PASSABLE))
	    {
		/* Option -- ignore */
		if (OPT(run_ignore_doors))
		    notice = FALSE;
	    }

	    /* Stairs */
	    if (tf_has(f_ptr->flags, TF_STAIR)) 
	    {
		/* Option -- ignore */
		if (OPT(run_ignore_stairs))
		    notice = FALSE;
	    }

	    /* Boring grids */
	    if (!tf_has(f_ptr->flags, TF_INTERESTING) &&
		!cave_visible_trap(row, col))
	    {
		/* Ignore */
		notice = FALSE;
	    }

	    /* Interesting feature */
	    if (notice)
		return (TRUE);

	    /* The grid is "visible" */
	    inv = FALSE;
	}

	/* Analyze unknown grids and floors */
	f_ptr = &f_info[cave_feat[row][col]];
	if (inv || tf_has(f_ptr->flags, TF_FLOOR)) {
	    /* Looking for open area */
	    if (p_ptr->run_open_area) {
		/* Nothing */
	    }

	    /* The first new direction. */
	    else if (!option) {
		option = new_dir;
	    }

	    /* Three new directions. Stop running. */
	    else if (option2) {
		return (TRUE);
	    }

	    /* Two non-adjacent new directions.  Stop running. */
	    else if (option != cycle[chome[prev_dir] + i - 1]) {
		return (TRUE);
	    }

	    /* Two new (adjacent) directions (case 1) */
	    else if (new_dir & 0x01) {
		option2 = new_dir;
	    }

	    /* Two new (adjacent) directions (case 2) */
	    else {
		option2 = option;
		option = new_dir;
	    }
	}

	/* Obstacle, while looking for open area */
	else {
	    if (p_ptr->run_open_area) {
		if (i < 0) {
		    /* Break to the right */
		    p_ptr->run_break_right = TRUE;
		}

		else if (i > 0) {
		    /* Break to the left */
		    p_ptr->run_break_left = TRUE;
		}
	    }
	}
    }


    /* Look at every soon to be newly adjacent square. */
    for (i = -max; i <= max; i++)
    {		
	/* New direction */
	new_dir = cycle[chome[prev_dir] + i];
		
	/* New location */
	row = py + ddy[prev_dir] + ddy[new_dir];
	col = px + ddx[prev_dir] + ddx[new_dir];
		
	/* HACK: Ugh. Sometimes we come up with illegal bounds. This will
	 * treat the symptom but not the disease. */
	if (row >= DUNGEON_HGT || col >= DUNGEON_WID) continue;
	if (row < 0 || col < 0) continue;

	    /* Visible monsters abort running */
	    if (cave_m_idx[row][col] > 0) {
		monster_type *m_ptr = &m_list[cave_m_idx[row][col]];

		/* Visible monster */
		if (m_ptr->ml)
		    return (TRUE);
	    }

	/* Visible monsters abort running */
	if (cave_m_idx[row][col] > 0)
	{
	    monster_type *m_ptr = &m_list[cave_m_idx[row][col]];
			
	    /* Visible monster */
	    if (m_ptr->ml) return (TRUE);			
	}
    }

    /* Looking for open area */
    if (p_ptr->run_open_area) {
	/* Hack -- look again */
	for (i = -max; i < 0; i++) {
	    new_dir = cycle[chome[prev_dir] + i];

	    row = py + ddy[new_dir];
	    col = px + ddx[new_dir];
	    f_ptr = &f_info[cave_feat[row][col]];

	    /* Unknown grid or non-wall */
	    /* Was: cave_floor_bold(row, col) */
	    if (!sqinfo_has(cave_info[row][col], SQUARE_MARK)
		|| !tf_has(f_ptr->flags, TF_ROCK))
	    {
		/* Looking to break right */
		if (p_ptr->run_break_right) 
		    return (TRUE);
	    }

	    /* Obstacle */
	    else 
	    {
		/* Looking to break left */
		if (p_ptr->run_break_left) 
		    return (TRUE);
	    }
	}
	
	/* Hack -- look again */
	for (i = max; i > 0; i--) {
	    new_dir = cycle[chome[prev_dir] + i];

	    row = py + ddy[new_dir];
	    col = px + ddx[new_dir];
	    f_ptr = &f_info[cave_feat[row][col]];

	    /* Unknown grid or non-wall */
	    /* Was: cave_floor_bold(row, col) */
	    if (!sqinfo_has(cave_info[row][col], SQUARE_MARK)
		|| !tf_has(f_ptr->flags, TF_ROCK))
	    {
		/* Looking to break left */
		if (p_ptr->run_break_left) 
		    return (TRUE);
	    }

	    /* Obstacle */
	    else 
	    {
		/* Looking to break right */
		if (p_ptr->run_break_right) 
		    return (TRUE);
	    }
	}
    }
    

    /* Not looking for open area */
    else 
    {
	/* No options */
	if (!option) 
	    return (TRUE);
	
	/* One option */
	else if (!option2) 
	{
	    /* Primary option */
	    p_ptr->run_cur_dir = option;

	    /* No other options */
	    p_ptr->run_old_dir = option;
	}

	/* Two options, examining corners */
	else
	{
	    /* Primary option */
	    p_ptr->run_cur_dir = option;

	    /* Hack -- allow curving */
	    p_ptr->run_old_dir = option2;
	}
    }


    /* About to hit a known wall, stop */
    if (see_wall(p_ptr->run_cur_dir, py, px)) 
	return (TRUE);
    

    /* Failure */
    return (FALSE);
}



/**
 * Take one step along the current "run" path
 *
 * Called with a real direction to begin a new run, and with zero
 * to continue a run in progress.
 */
void run_step(int dir)
{
    /* Start run */
    if (dir) {
	/* Paranoia */
	p_ptr->running_withpathfind = 0;

	/* Initialize */
	run_init(dir);

	/* Hack -- Set the run counter */
	p_ptr->running = (p_ptr->command_arg ? p_ptr->command_arg : 1000);

	/* Calculate torch radius */
	p_ptr->update |= (PU_TORCH);
    }

    /* Continue run */
    else {
	if (!p_ptr->running_withpathfind) {
	    /* Update run */
	    if (run_test()) {
		/* Disturb */
		disturb(0, 0);

		/* Done */
		return;
	    }
	}

	else {
	    /* Abort if we have finished */
	    if (pf_result_index < 0) {
		disturb(0, 0);
		p_ptr->running_withpathfind = FALSE;
		return;
	    }
	    /* Abort if we would hit a wall */
	    else if (pf_result_index == 0) {
		int y, x;

		/* Get next step */
		y = p_ptr->py + ddy[pf_result[pf_result_index] - '0'];
		x = p_ptr->px + ddx[pf_result[pf_result_index] - '0'];

		/* Known wall */
		if (sqinfo_has(cave_info[y][x], SQUARE_MARK) && !is_valid_pf(y, x))
		{
		    disturb(0, 0);
		    p_ptr->running_withpathfind = FALSE;
		    return;
		}
	    }
	    /* Hack -- walking stick lookahead. If the player has computed a
	     * path that is going to end up in a wall, we notice this and
	     * convert to a normal run. This allows us to click on unknown
	     * areas to explore the map. We have to look ahead two, otherwise
	     * we don't know which is the last direction moved and don't
	     * initialise the run properly. */
	    else if (pf_result_index > 0) {
		int y, x;

		/* Get next step */
		y = p_ptr->py + ddy[pf_result[pf_result_index] - '0'];
		x = p_ptr->px + ddx[pf_result[pf_result_index] - '0'];

		/* Known wall */
		if (sqinfo_has(cave_info[y][x], SQUARE_MARK) && !is_valid_pf(y, x))
		{
		    disturb(0, 0);
		    p_ptr->running_withpathfind = FALSE;
		    return;
		}

		/* Get step after */
		y = y + ddy[pf_result[pf_result_index - 1] - '0'];
		x = x + ddx[pf_result[pf_result_index - 1] - '0'];

		/* Known wall */
		if (sqinfo_has(cave_info[y][x], SQUARE_MARK) && !is_valid_pf(y, x))
		{
		    p_ptr->running_withpathfind = FALSE;

		    run_init(pf_result[pf_result_index] - '0');
		}
	    }

	    p_ptr->run_cur_dir = pf_result[pf_result_index--] - '0';
	}
    }

    /* Decrease counter */
    p_ptr->running--;

    /* Take time */
    p_ptr->energy_use = 100;

    /* Move the player */
    move_player(p_ptr->run_cur_dir);
}

