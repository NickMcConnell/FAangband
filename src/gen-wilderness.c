/**
 * \file gen-wilderness.c 
 * \brief Wilderness generation
 *
 * Code for creation of wilderness.
 *
 * Copyright (c) 2019
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
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-make.h"
#include "monster.h"
#include "player-util.h"
#include "project.h"


/**
 * Number and type of "vaults" in wilderness levels 
 * These need to be set at the start of each wilderness generation routine.
 */
int num_wild_vaults;


/**
 * ------------------------------------------------------------------------
 * Various wilderness helper routines
 * ------------------------------------------------------------------------ */
#define HIGHLAND_TREE_CHANCE 30

#define MAX_PATHS 13

/**
 * Set the number of wilderness vaults
 */
void set_num_vaults(struct chunk *c)
{
	int max = 2;
	num_wild_vaults = 0;

	if (c->depth > 10)
		num_wild_vaults += randint0(max);
	if (c->depth > 20)
		num_wild_vaults += randint0(max);
	if (c->depth > 30)
		num_wild_vaults += randint0(max);
	if (c->depth > 40)
		num_wild_vaults += randint0(max);

	if (no_vault(player->place))
		num_wild_vaults = 0;
}

/**
 * Makes "paths to nowhere" from interplace paths toward the middle of the
 * current place.  Adapted from tunnelling code.
 */
static void path_to_nowhere(struct chunk *c, struct loc start,
							struct loc target, struct loc *pathend, int *num)
{
	struct loc direction, grid, end = start;
	int i, j;


	bool done = false;

	/* make sure targets are in bounds, reflect back in if not */
	target.y += ABS(target.y) - target.y
		- ABS(c->height - 1 - target.y) + (c->height - 1 - target.y);
	target.x += ABS(target.x) - target.x
		- ABS(c->width - 1 - target.x) + (c->width - 1 - target.x);

	/* Start */
	correct_dir(&direction, start, target);
	grid = loc_sum(start, direction);
	if (square_in_bounds_fully(c, grid)) {
		/* Take a step */
		square_set_feat(c, grid, FEAT_ROAD);
		end = grid;
	} else {
		/* No good, just finish at the start */
		done = true;
	}

	/* 100 steps should be enough */
	for (i = 0; i < 50 && !done; i++) {
		/* Try one randomish step... */
		adjust_dir(&direction, grid, target);
		grid = loc_sum(grid, direction);
		if (square_in_bounds_fully(c, grid)) {
			square_set_feat(c, grid, FEAT_ROAD);
			end = grid;
		} else {
			break;
		}

		/* ...and one good one */
		correct_dir(&direction, grid, target);
		grid = loc_sum(grid, direction);
		if (square_in_bounds_fully(c, grid)) {
			square_set_feat(c, grid, FEAT_ROAD);
			end = grid;
		} else {
			break;
		}

		/* Near enough is good enough */
		if ((ABS(grid.x - target.x) < 3) && (ABS(grid.y - target.y) < 3))
			break;
	}

	/* Record where we have finished */
	end = grid;

	/* Store the end */
	for (j = MAX_PATHS - 1; j >= 0; j--) {
		/* This is the first one, record it and finish */
		if (j == 0) {
			pathend[j] = end;
		}

		/* Continue until we find where this grid is in x order */
		if (pathend[j].x == 0)
			continue;
		if (pathend[j].x < end.x) {
			pathend[j + 1] = pathend[j];
			continue;
		}

		/* Found it, record */
		pathend[j + 1] = end;
		(*num)++;
		break;
	}
}

/**
 * Move the path if it might land in a river
 */
static void river_move(struct chunk *c, int *xp)
{
	int x = (*xp), diff;

	diff = x - c->width / 2;
	if (ABS(diff) < 10)
		x = (diff < 0) ? (x - 10) : (x + 10);

	(*xp) = x;
	return;
}

/**
 * Places paths to adjacent surface places, and joins them.  Does each 
 * direction separately, which is a bit repetitive -NRM-
 */
static void alloc_paths(struct chunk *c, struct player *p, int place,
						int last_place)
{
	int y, x, i, j, num, path;
	int pcoord = player->upkeep->path_coord;
	struct loc grid, pgrid, tgrid;

	struct level *lev = &world->levels[place];
	struct level *last_lev = &world->levels[last_place];
	struct level *north = NULL;
	struct level *east = NULL;
	struct level *south = NULL;
	struct level *west = NULL;

	struct loc pathend[20], gp[512];
	int pspot, num_paths = 0, path_grids = 0;

	bool jumped = true;
	bool river;

	if (lev->north) north = level_by_name(world, lev->north);
	if (lev->east) east = level_by_name(world, lev->east);
	if (lev->south) south = level_by_name(world, lev->south);
	if (lev->west) west = level_by_name(world, lev->west);

	/* River levels need special treatment */
	river = (lev->topography == TOP_RIVER);

	for (i = 0; i < MAX_PATHS; i++) {
		pathend[i].y = 0;
		pathend[i].x = 0;
	}

	/* Hack for finishing Nan Dungortheb */
	//if ((last_place == q_list[2].place) && (last_place != 0))
	//	north = last_place;

	/* North */
	if (north) {
		/* Harder or easier? */
		if (north->depth > lev->depth) {
			path = FEAT_MORE_NORTH;
		} else {
			path = FEAT_LESS_NORTH;
		}

		/* Way back */
		if ((north == last_lev) && (player->upkeep->create_stair)) {
			/* No paths in river */
			if (river)
				river_move(c, &pcoord);

			pgrid = loc(pcoord, 1);
			square_set_feat(c, pgrid, path);
			square_mark(c, pgrid);
			jumped = false;

			/* Make path to nowhere */
			tgrid = loc(pgrid.x + randint1(40) - 20,
						1 + c->height / 3 + randint1(20) - 10);
			path_to_nowhere(c, pgrid, tgrid, pathend, &num_paths);
		}

		/* Decide number of paths */
		num = rand_range(2, 3);

		/* Place "num" paths */
		for (i = 0; i < num; i++) {
			x = 1 + randint0(c->width / num - 2) +
				i * c->width / num;

			/* No paths in river */
			if (river)
				river_move(c, &x);

			grid = loc(x, 1);
			square_set_feat(c, grid, path);
			square_mark(c, grid);

			/* Make paths to nowhere */
			tgrid = loc(grid.x + randint1(40) - 20,
						1 + c->height / 3 + randint1(20) - 10);
			path_to_nowhere(c, grid, tgrid, pathend, &num_paths);
		}
	}

	/* East */
	if (east) {
		/* Harder or easier? */
		if (east->depth > lev->depth) {
			path = FEAT_MORE_EAST;
		} else {
			path = FEAT_LESS_EAST;
		}

		/* Way back */
		if ((east == last_lev) && (player->upkeep->create_stair)) {
			pgrid = loc(c->width - 2, pcoord);
			square_set_feat(c, pgrid, path);
			square_mark(c, pgrid);
			jumped = false;

			/* Make path to nowhere */
			tgrid = loc(c->width - c->height / 3 - randint1(20) + 8,
						pgrid.y + randint1(40) - 20);
			path_to_nowhere(c, pgrid, tgrid, pathend, &num_paths);
		}

		/* Decide number of paths */
		num = rand_range(2, 3);

		/* Place "num" paths */
		for (i = 0; i < num; i++) {
			y = 1 + randint0(c->height / num - 2) +	i * c->height / num;
			grid = loc(c->width - 2, y);
			square_set_feat(c, grid, path);
			square_mark(c, grid);

			/* Make paths to nowhere */
			tgrid = loc(c->width - c->height / 3 - randint1(20) + 8,
						grid.y + randint1(40) - 20);
			path_to_nowhere(c, grid, tgrid, pathend, &num_paths);
		}
	}

	/* South */
	if (south) {
		/* Harder or easier? */
		if (south->depth > lev->depth) {
			path = FEAT_MORE_SOUTH;
		} else {
			path = FEAT_LESS_SOUTH;
		}

		/* Way back */
		if ((south == last_lev) && (p->upkeep->create_stair)) {
			/* No paths in river */
			if (river)
				river_move(c, &pcoord);

			pgrid = loc(pcoord, c->height - 2);
			square_set_feat(c, pgrid, path);
			square_mark(c, pgrid);
			jumped = false;

			/* Make path to nowhere */
			tgrid = loc(pgrid.x + randint1(40) - 20,
						c->height - c->height / 3 - randint1(20) + 8);
			path_to_nowhere(c, pgrid, tgrid, pathend, &num_paths);
		}

		/* Decide number of paths */
		num = rand_range(2, 3);

		/* Place "num" paths */
		for (i = 0; i < num; i++) {
			x = 1 + randint0(c->width / num - 2) +
				i * c->width / num;

			/* No paths in river */
			if (river)
				river_move(c, &x);

			grid = loc(x, c->height - 2);
			square_set_feat(c, grid, path);
			square_mark(c, grid);

			/* Make paths to nowhere */
			tgrid = loc(grid.x + randint1(40) - 20,
						c->height - c->height / 3 - randint1(20) + 8);
			path_to_nowhere(c, grid, tgrid, pathend, &num_paths);
		}
	}

	/* West */
	if (west) {
		/* Harder or easier? */
		if (west->depth > lev->depth) {
			path = FEAT_MORE_WEST;
		} else {
			path = FEAT_LESS_WEST;
		}

		/* Way back */
		if ((west == last_lev) && (p->upkeep->create_stair)) {
			pgrid = loc(1, pcoord);
			square_set_feat(c, pgrid, path);
			square_mark(c, pgrid);
			jumped = false;

			/* Make path to nowhere */
			tgrid = loc(1 + c->height / 3 + randint1(20) - 10,
						pgrid.y + randint1(40) - 20);
			path_to_nowhere(c, pgrid, tgrid, pathend, &num_paths);
		}

		/* Decide number of paths */
		num = rand_range(2, 3);

		/* Place "num" paths */
		for (i = 0; i < num; i++) {
			y = 1 + randint0(c->height / num - 2) +	i * c->height / num;
			grid = loc(1, y);
			square_set_feat(c, grid, path);
			square_mark(c, grid);

			/* make paths to nowhere */
			tgrid = loc(1 + c->height / 3 + randint1(20) - 10,
						grid.y + randint1(40) - 20);
			path_to_nowhere(c, grid, tgrid, pathend, &num_paths);
		}
	}

	/* Find the middle of the paths */
	num_paths /= 2;

	/* Make them paths to somewhere */
	for (i = 0; i < MAX_PATHS - 1; i++) {
		/* All joined? */
		if (!pathend[i + 1].x)
			break;

		/* Find the shortest path */
		path_grids = project_path(c, gp, 512, pathend[i], pathend[i + 1],
								  PROJECT_NONE);

		/* Get the jumped player spot */
		if ((i == num_paths) && (jumped)) {
			pspot = path_grids / 2;
			pgrid = gp[pspot];
		}

		/* Make the path, adding an adjacent grid 8/9 of the time */
		for (j = 0; j < path_grids; j++) {
			struct loc offset = loc(randint0(3) - 1, randint0(3) - 1);
			square_set_feat(c, gp[j], FEAT_ROAD);
			grid = loc_sum(gp[j], offset);
			if (square_in_bounds_fully(c, grid)) {
				square_set_feat(c, loc_sum(gp[j], offset), FEAT_ROAD);
			}
		}
	}

	/* Mark all the roads, so we know not to overwrite them */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square(c, grid)->feat == FEAT_ROAD) {
				square_mark(c, grid);
			}
		}
	}

	/* Place the player, unless we've just come upstairs */
	if ((last_lev->topography == TOP_CAVE) &&
		(last_lev->locality != LOC_UNDERWORLD) &&
		(last_lev->locality != LOC_ARENA))
		return;

	player_place(c, player, pgrid);
}

/**
 * Make the boundaries of the place - these may be ragged, and Nan Dungortheb
 * requires special handling
 */
static void make_edges(struct chunk *c, bool ragged, bool valley)
{
	int i, j, num = 2;
	int path_x[3];
	struct loc grid;

	/* Prepare places for down slides */
	num += randint0(2);
	for (i = 0; i < num; i++)
		path_x[i] =	1 + randint0(c->width / num - 2) + i * c->width / num;

	/* Special boundary walls -- Top */
	i = (valley ? 5 : 4);
	for (grid.x = 0; grid.x < c->width; grid.x++) {
		if (ragged) {
			i += 1 - randint0(3);
			if (i > (valley ? 10 : 7))
				i = (valley ? 10 : 7);
			if (i < 1)
				i = 1;
			for (grid.y = 0; grid.y < i; grid.y++) {
				/* Clear previous contents, add perma-wall */
				if (!square_ismark(c, grid)) {
					square_set_feat(c, grid, FEAT_PERM);
				}
			}
			if (valley && (grid.x > 0) &&
				(grid.x == player->upkeep->path_coord)) {
				if (grid.y == 0) {
					grid.y++;
				}
				square_set_feat(c, grid, FEAT_PASS_RUBBLE);
				player_place(c, player, grid);
			}
		} else {
			grid.y = 0;

			/* Clear previous contents, add perma-wall */
			square_set_feat(c, grid, FEAT_PERM);
		}
	}


	/* Special boundary walls -- Bottom */
	i = (valley ? 5 : 4);
	j = 0;
	for (grid.x = 0; grid.x < c->width; grid.x++) {
		if (ragged && !(valley && (player->depth == 70))) {
			i += 1 - randint0(3);
			if (i > (valley ? 10 : 7))
				i = (valley ? 10 : 7);
			if (i < 1)
				i = 1;
			for (grid.y = c->height - 1; grid.y > c->height - 1 - i; grid.y--) {
				/* Clear previous contents, add perma-wall or void */
				if (!square_ismark(c, grid)) {
					square_set_feat(c, grid, (valley ? FEAT_VOID : FEAT_PERM));
				}
			}
			/* Down slides */
			if ((j < num) && valley)
				if (grid.x == path_x[j]) {
					square_set_feat(c, grid, FEAT_MORE_SOUTH);
					j++;
				}
		} else {
			grid.y = c->height - 1;

			/* Clear previous contents, add perma-wall */
			square_set_feat(c, grid, FEAT_PERM);
		}
	}

	/* Special boundary walls -- Left */
	i = 5;
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		if (ragged) {
			i += 1 - randint0(3);
			if (i > 10)
				i = 10;
			if (i < 1)
				i = 1;
			for (grid.x = 0; grid.x < i; grid.x++) {
				/* Clear previous contents, add perma-wall */
				if (!square_ismark(c, grid)) {
					square_set_feat(c, grid, FEAT_PERM);
				}
			}
		} else {
			grid.x = 0;

			/* Clear previous contents, add perma-wall */
			square_set_feat(c, grid, FEAT_PERM);
		}
	}

	/* Special boundary walls -- Right */
	i = 5;
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		if (ragged) {
			i += 1 - randint0(3);
			if (i > 10)
				i = 10;
			if (i < 1)
				i = 1;
			for (grid.x = c->width - 1; grid.x > c->width - 1 - i; grid.x--) {
				/* Clear previous contents, add perma-wall */
				if (!square_ismark(c, grid)) {
					square_set_feat(c, grid, FEAT_PERM);
				}
			}
		} else {
			grid.x = c->width - 1;

			/* Clear previous contents, add perma-wall */
			square_set_feat(c, grid, FEAT_PERM);
		}
	}
}

/**
 * Check whether there is space for a wilderness vault with the given corners
 */
static bool check_vault_space(struct chunk *c, struct loc avoid,
							  struct loc top_left, struct loc bottom_right)
{
	struct loc grid;

	/* Require corners to be in bounds */
	if (!square_in_bounds_fully(c, top_left)) return false;
	if (!square_in_bounds_fully(c, bottom_right)) return false;

	/* Check every square */
	for (grid.y = top_left.y; grid.y < bottom_right.y; grid.y++) {
		for (grid.x = top_left.x; grid.x < bottom_right.x; grid.x++) {
			if (square_ispath(c, grid)) return false;
			if (distance(grid, avoid) < 20) return false;
			if (square_ismark(c, grid)) return false;
			if (square(c, grid)->mon) return false;
			if (square_isplayertrap(c, grid)) return false;
			if (square_object(c, grid)) return false;
			if (square_iswebbed(c, grid)) return false;
		}
	}

	return true;
}

/**
 * Make a formation - a randomish group of terrain squares. -NRM-
 * Care probably needed with declaring feat[].
 *
 * As of FAangband 0.2.2, wilderness "vaults" are now made here.  These
 * are less structured than cave vaults or webs; in particular other
 * formations or even "vaults" can bleed into them.
 *
 */
static int make_formation(struct chunk *c, struct player *p, struct loc grid,
						  int base_feat1, int base_feat2, int *feat,
						  char *name, int prob)
{
	int step, j, jj, i = 0, total = 0;
	int *all_feat = mem_zalloc(prob * sizeof(*all_feat));
	struct loc tgrid = grid;

	/* Need to make some wilderness vaults */
	if (num_wild_vaults) {
		struct vault *v;
		struct loc top_left, bottom_right;
		bool good_place = true;

		/* Greater vault? */
		bool greater = randint0(100 - p->depth) < 9;

		if (greater) {
			v = random_vault(p->depth, format("%s greater", name),
							 "Wilderness greater");
		} else {
			v = random_vault(p->depth, format("%s lesser", name),
							 "Wilderness lesser");
		}
		if (v) {
			/* Check to see if it will fit here (only avoid edges) */
			top_left = loc(grid.x - v->wid / 2, grid.y - v->hgt / 2);
			bottom_right = loc_sum(top_left, loc(v->wid, v->hgt));
			good_place = check_vault_space(c, p->grid, top_left, bottom_right);

			/* We've found a place */
			if (good_place) {
				/* Build the vault (never lit, icky) */
				if (!build_vault(c, grid, v)) {
					mem_free(all_feat);
					return 0;
				}

				/* Boost the rating */
				c->mon_rating += v->rat;

				/* Message */
				if (OPT(p, cheat_room))
					msg("%s. ", v->name);

				/* One less to make */
				num_wild_vaults--;

				/* Takes up some space */
				mem_free(all_feat);
				return (v->hgt * v->wid);
			}
		}
	}

	/* Extend the array of terrain types to length prob */
	jj = 0;
	for (j = 0; j < prob - 1; j++) {
		if (feat[jj] == FEAT_NONE)
			jj = 0;
		all_feat[j] = feat[jj];
		jj++;
	}

	/* Make a formation */
	while (i != (prob - 1)) {
		/* Avoid paths, stay in bounds */
		if (((square_feat(c, tgrid)->fidx != base_feat1) &&
			 (square_feat(c, tgrid)->fidx != base_feat2))
			|| !square_in_bounds_fully(c, tgrid) || square_ismark(c, tgrid)) {
			mem_free(all_feat);
			return (total);
		}

		/* Check for treasure */
		if ((all_feat[i] == FEAT_MAGMA) && (one_in_(dun->profile->str.mc))) {
			all_feat[i] = FEAT_MAGMA_K;
		} else if ((all_feat[i] == FEAT_QUARTZ) &&
				   (one_in_(dun->profile->str.qc))) {
			all_feat[i] = FEAT_QUARTZ_K;
		}

		/* Set the feature */
		square_set_feat(c, tgrid, all_feat[i]);
		square_mark(c, tgrid);

		/* Choose a random step for next feature, try to keep going */
		step = randint0(8) + 1;
		if (step > 4)
			step++;
		for (j = 0; j < 100; j++) {
			tgrid = loc_sum(tgrid, ddgrid[step]);
			if (!square_in_bounds_fully(c, tgrid)) break;
			if (!square_ismark(c, tgrid)) break;
		}

		/* Count */
		total++;

		/* Pick the next terrain, or finish */
		i = randint0(prob);
	}

	mem_free(all_feat);
	return (total);
}

/**
 * Place some monsters, objects and traps
 */
static int populate(struct chunk *c, bool valley)
{
	int i, j;
	struct loc grid;

	/* Basic "amount" */
	int k = (c->depth / 2);

	if (valley) {
		if (k > 30)
			k = 30;
	} else {
		/* Gets hairy north of the mountains */
		if (c->depth > 40)
			k += 10;
	}

    /* Pick a base number of monsters */
    i = z_info->level_monster_min + randint1(8) + k;

	/* Build the monster probability table. */
	(void) get_mon_num(c->depth);

	/* Put some monsters in the dungeon */
	for (j = i + k; j > 0; j--) {
		/* Always have some random monsters */
		if (j < 5) {
			/* Remove all monster restrictions. */
			get_mon_num_prep(NULL);

			/* Build the monster probability table. */
			(void) get_mon_num(c->depth);
		}

		/*
		 * Place a random monster (quickly), but not in grids marked
		 * "SQUARE_TEMP".
		 */
		(void) pick_and_place_distant_monster(c, player, 10, true, c->depth);
	}

	/* Place some traps in the dungeon. */
	alloc_objects(c, SET_BOTH, TYP_TRAP, randint1(k), c->depth, 0);

	/* Put some objects in rooms */
	alloc_objects(c, SET_BOTH, TYP_OBJECT, Rand_normal(z_info->room_item_av, 3),
				  c->depth, ORIGIN_FLOOR);

	/* Put some objects/gold in the dungeon */
	alloc_objects(c, SET_BOTH, TYP_OBJECT, Rand_normal(z_info->both_item_av, 3),
				 c->depth, ORIGIN_FLOOR);
	alloc_objects(c, SET_BOTH, TYP_GOLD, Rand_normal(z_info->both_gold_av, 3),
				 c->depth, ORIGIN_FLOOR);

	/* Paranoia - remake the dungeon walls */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if ((grid.y == 0) || (grid.x == 0) || (grid.y == c->height - 1)
				|| (grid.x == c->width - 1))
				square_set_feat(c, grid, FEAT_PERM);
		}

		/* Super paranoia */
		if (feat_is_permanent(c->squares[grid.y][grid.x].feat)) {
			struct monster *mon = square_monster(c, grid);
			struct object *obj = square_object(c, grid);
			if (mon) {
				delete_monster(grid);
			}
			if (obj) {
				square_delete_object(c, grid, obj, false, false);
			}
		}
	}

	return k;
}

/**
 * Connect a grid to the main path through a mountain level
 */
static void mtn_connect(struct chunk *c, struct loc grid1, struct loc grid2)
{
	struct loc gp[512];
	int path_grids, j;

	/* Find the shortest path */
	path_grids = project_path(c, gp, 512, grid1, grid2, PROJECT_ROCK);

	/* Make the path, adding an adjacent grid 8/9 of the time */
	for (j = 0; j < path_grids; j++) {
		if ((square_feat(c, gp[j])->fidx == FEAT_ROAD) ||
			(!square_in_bounds_fully(c, gp[j])))
			break;
		square_set_feat(c, gp[j], FEAT_ROAD);
		square_mark(c, gp[j]);
	}
}

/**
 * Attempt to place a web of the required type
 */
bool place_web(struct chunk *c, struct player *p, char *type)
{
	struct vault *v;
	int i;
	struct loc grid, top_left, centre;

	bool no_good = false;

	/* Choose a random web, can fail */
	v = random_vault(c->depth, type, NULL);
	if (!v) return false;

	/* Look for somewhere to put it */
	for (i = 0; i < 25; i++) {
		no_good = false;

		/* Random top left corner */
		top_left.y = randint1(c->height - 1 - v->hgt);
		top_left.x = randint1(c->width - 1 - v->wid);

		/* Check to see if it will fit (only avoid big webs and edges) */
		for (grid.y = top_left.y; grid.y < top_left.y + v->hgt; grid.y++) {
			for (grid.x = top_left.x; grid.x < top_left.x + v->wid; grid.x++) {
				if (square_isfall(c, grid) || square_ispermanent(c, grid)
					|| square_isplayer(c, grid)	|| square_isvault(c, grid)) {
					no_good = true;
					break;
				}
			}
			if (no_good) {
				break;
			}
		}
	}

	/* Give up if we couldn't find anywhere */
	if (no_good) return false;

	/* Build the vault */
	centre = loc(top_left.x + v->wid / 2, top_left.y + v->hgt / 2);
	if (!build_vault(c, centre, v)) return false;

	/* Replace granite by webbed trees */
	for (grid.y = top_left.y; grid.y < top_left.y + v->hgt; grid.y++) {
		for (grid.x = top_left.x; grid.x < top_left.x + v->wid; grid.x++) {
			if (square_iswall(c, grid)) {
				if (one_in_(2)) {
					square_set_feat(c, grid, FEAT_TREE);
				} else {
					square_set_feat(c, grid, FEAT_TREE2);
				}
				square_add_web(c, grid);
			}
		}
	}

	/* Boost the rating */
	c->mon_rating += v->rat;

	return true;
}

/**
 * ------------------------------------------------------------------------
 * Wilderness level generation
 * ------------------------------------------------------------------------ */
/**
 * Generate a new plain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *plain_gen(struct player *p, int height, int width)
{
	struct loc grid;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_TREE, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE };
	int ponds[] = { FEAT_WATER, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with grass */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Place some formations */
	while (form_grids < (50 * c->depth + 1000)) {
		/* Choose a place */
		grid.y = randint0(c->height - 1) + 1;
		grid.x = randint0(c->width - 1) + 1;
		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_GRASS,
									 form_feats, "Plains", c->depth + 1);
	}

	/* And some water */
	form_grids = 0;
	while (form_grids < 300) {
		grid.y = randint0(c->height - 1) + 1;
		grid.x = randint0(c->width - 1) + 1;
		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_GRASS, ponds,
									 "Plains", 10);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	return c;
}

/**
 * Generate a new mountain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *mtn_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int i, j;
	int plats;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;
	int min, dist, floors = 0;
	int randpoints[20];
	struct loc pathpoints[20];
	struct loc nearest_point = { height / 2, width / 2 };
	struct loc stairs[3];

	int form_feats[] = { FEAT_PASS_RUBBLE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_GRASS,
						 FEAT_TREE, FEAT_TREE2, FEAT_ROAD, FEAT_NONE };

	bool amon_rudh = false;

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with grass (lets paths work -NRM-) */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, false, false);

	/* Place 2 or 3 paths to neighbouring places, make the paths through the
	 * place, place the player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Turn grass to granite */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_GRASS) {
				square_set_feat(c, grid, FEAT_GRANITE);
			}
		}
	}

	/* Dungeon entrance */
	if (world->levels[place].down) {
		/* Set the flag */
		amon_rudh = true;

		/* Mim's cave on Amon Rudh */
		i = 3;
		while (i) {
			grid.y = randint0(c->height - 2) + 1;
			grid.x = randint0(c->width - 2) + 1;
			if ((square_feat(c, grid)->fidx == FEAT_ROAD) ||
				(square_feat(c, grid)->fidx == FEAT_GRASS)) {
				square_set_feat(c, grid, FEAT_MORE);
				i--;
				stairs[2 - i] = grid;
				if (!i && (level_topography(last_place) == TOP_CAVE))
					player_place(c, p, grid);
			}
		}
	}


	/* Make paths permanent */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_ROAD) {
				/* Hack - prepare for plateaux, connecting */
				square_mark(c, grid);
				floors++;
			}
		}
	}

	/* Pick some joining points */
	for (j = 0; j < 20; j++)
		randpoints[j] = randint0(floors);
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_ROAD)
				floors--;
			else
				continue;
			for (j = 0; j < 20; j++) {
				if (floors == randpoints[j]) {
					pathpoints[j] = grid;
				}
			}
		}
	}

	/* Find the staircases, if any */
	if (amon_rudh) {
		for (j = 0; j < 3; j++) {
			grid = stairs[j];

			/* Now join them up */
			min = c->width + c->height;
			for (i = 0; i < 20; i++) {
				dist = distance(grid, pathpoints[i]);
				if (dist < min) {
					min = dist;
					nearest_point = pathpoints[i];
				}
			}
			mtn_connect(c, grid, nearest_point);
		}
	}

	/* Make a few "plateaux" */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, y;

		/* Try for a plateau */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		y = randint0(c->height - 1) + 1;
		x = randint0(c->width - 1) + 1;
		made_plat =	generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, true);

		/* Success ? */
		if (made_plat) {
			grid = loc(x, y);
			plats--;

			/* Now join it up */
			min = c->width + c->height;
			for (i = 0; i < 20; i++) {
				dist = distance(grid, pathpoints[i]);
				if (dist < min) {
					min = dist;
					nearest_point = pathpoints[i];
				}
			}
			mtn_connect(c, grid, nearest_point);
		}


		/* Done ? */
		if (!plats)
			break;
	}

	/* Make a few formations */
	while (form_grids < 50 * (c->depth)) {
		/* Choose a place */
		grid.y = randint0(c->height - 1) + 1;
		grid.x = randint0(c->width - 1) + 1;
		form_grids += make_formation(c, p, grid, FEAT_GRANITE, FEAT_GRANITE,
									 form_feats, "Mountain", c->depth * 2);
		/* Now join it up */
		min = c->width + c->height;
		for (i = 0; i < 20; i++) {
			dist = distance(grid, pathpoints[i]);
			if (dist < min) {
				min = dist;
				nearest_point = pathpoints[i];
			}
		}
		mtn_connect(c, grid, nearest_point);

	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);

			/* Paranoia - remake the dungeon walls */
			if ((grid.y == 0) || (grid.x == 0) ||
				(grid.y == c->height - 1) || (grid.x == c->width - 1)) {
				square_set_feat(c, grid, FEAT_PERM);
			}
		}
	}
	ensure_connectedness(c);

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	return c;
}

/**
 * Generate a new mountaintop level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct feature placement.
 */
struct chunk *mtntop_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid, top;
	int i, j, k;
	int plats, a, b;
	int spot, floors = 0;
	bool placed = false;

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with void */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create void */
			square_set_feat(c, grid, FEAT_VOID);
		}
	}

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, false, false);

	/* Make the main mountaintop */
	while (!placed) {
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		top.y = c->height / 2;
		top.x = c->width / 2;
		placed = generate_starburst_room(c, top.y - b, top.x - a, top.y + b,
										 top.x + a, false, FEAT_ROAD, false);
	}

	/* Summit */
	square_set_feat(c, top, FEAT_GRANITE);
	for (i = 0; i < 8; i++) {
		square_set_feat(c, loc_sum(top, ddgrid[i]), FEAT_GRANITE);
	}

	/* Count the floors */
	for (grid.y = top.y - b; grid.y < top.y + b; grid.y++) {
		for (grid.x = top.x - a; grid.x < top.x + a; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_ROAD) {
				floors++;
			}
		}
	}

	/* Choose the player place */
	spot = randint0(floors);

	/* Can we get down? */
	if (one_in_(2)) {
		grid.y = rand_range(top.y - b, top.y + b);
		if (square_feat(c, grid)->fidx != FEAT_VOID) {
			i = one_in_(2) ? 1 : -1;
			for (grid.x = top.x; grid.x != (top.x + i * (a + 1)); grid.x += i) {
				if (square_feat(c, grid)->fidx == FEAT_VOID) break;
			}
			square_set_feat(c, grid, FEAT_MORE);
		}
	}

	/* Adjust the terrain, place the player */
	for (grid.y = top.y - b; grid.y < top.y + b; grid.y++) {
		for (grid.x = top.x - a; grid.x < top.x + a; grid.x++) {
			/* Only change generated stuff */
			if (square_feat(c, grid)->fidx == FEAT_VOID)
				continue;

			/* Leave rock */
			if (square_feat(c, grid)->fidx == FEAT_GRANITE)
				continue;

			/* Leave stair */
			if (square_feat(c, grid)->fidx == FEAT_MORE)
				continue;

			/* Place the player? */
			if (square_feat(c, grid)->fidx == FEAT_ROAD) {
				floors--;
				if (floors == spot) {
					player_place(c, p, grid);
					square_mark(c, grid);
					continue;
				}
			}

			/* Place some rock */
			if (one_in_(5)) {
				square_set_feat(c, grid, FEAT_GRANITE);
				continue;
			}

			/* rubble */
			if (one_in_(8)) {
				square_set_feat(c, grid, FEAT_PASS_RUBBLE);
				continue;
			}

			/* and the odd tree */
			if (one_in_(20)) {
				square_set_feat(c, grid, FEAT_TREE2);
				continue;
			}
		}
	}

	/* Make a few "plateaux" */
	plats = randint0(4);

	/* Try fairly hard */
	for (j = 0; j < 10; j++) {
		/* Try for a plateau */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		top.y = randint0(c->height - 1) + 1;
		top.x = randint0(c->width - 1) + 1;
		made_plat = generate_starburst_room(c, top.y - b, top.x - a, top.y + b,
											top.x + a, false, FEAT_ROAD, false);

		/* Success ? */
		if (made_plat) {
			plats--;

			/* Adjust the terrain a bit */
			for (grid.y = top.y - b; grid.y < top.y + b; grid.y++) {
				for (grid.x = top.x - a; grid.x < top.x + a; grid.x++) {
					/* Only change generated stuff */
					if (square_feat(c, grid)->fidx == FEAT_VOID)
						continue;

					/* Place some rock */
					if (one_in_(5)) {
						square_set_feat(c, grid, FEAT_GRANITE);
						continue;
					}

					/* rubble */
					if (one_in_(8)) {
						square_set_feat(c, grid, FEAT_PASS_RUBBLE);
						continue;
					}

					/* and the odd tree */
					if (one_in_(20)) {
						square_set_feat(c, grid, FEAT_TREE2);
						continue;
					}
				}
			}
		}

		/* Done ? */
		if (!plats)
			break;
	}


	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);

			/* Paranoia - remake the dungeon walls */
			if ((grid.y == 0) || (grid.x == 0) ||
				(grid.y == c->height - 1) || (grid.x == c->width - 1)) {
				square_set_feat(c, grid, FEAT_PERM);
			}
		}
	}

	/* Basic "amount" */
	k = c->depth;

	/* Build the monster probability table. */
	(void) get_mon_num(c->depth);

	/* Put some monsters in the dungeon */
	for (j = k; j > 0; j--) {
		(void) pick_and_place_distant_monster(c, player, 10, true, c->depth);
	}


	/* Put some objects in the dungeon */
	alloc_objects(c, SET_BOTH, TYP_OBJECT, Rand_normal(z_info->both_item_av, 3),
				 c->depth, ORIGIN_FLOOR);


	return c;
}

/**
 * Generate a new forest level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *forest_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int j;
	int plats;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_GRASS, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE	};
	int ponds[] = { FEAT_WATER, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with grass so paths work */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Now place trees */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create trees */
			if (square_feat(c, grid)->fidx == FEAT_GRASS) {
				if (randint1(c->depth + HIGHLAND_TREE_CHANCE)
					> HIGHLAND_TREE_CHANCE) {
					square_set_feat(c, grid, FEAT_TREE2);
				} else {
					square_set_feat(c, grid, FEAT_TREE);
				}
			} else {
				/* Hack - prepare for clearings */
				square_mark(c, grid);

			/* Mega hack - remove paths if emerging from Nan Dungortheb */
			//if ((last_place == q_list[2].place)
			//	&& (cave_feat[y][x] == FEAT_MORE_NORTH))
			//	cave_set_feat(y, x, FEAT_GRASS);
			}
		}
	}

	/* Make a few clearings */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, y;

		/* Try for a clearing */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		y = randint0(c->height - 1) + 1;
		x = randint0(c->width - 1) + 1;
		made_plat = generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, true);

		/* Success ? */
		if (made_plat)
			plats--;

		/* Done ? */
		if (!plats)
			break;
	}

	/* Place some formations */
	while (form_grids < (50 * c->depth + 1000)) {
		/* Choose a place */
		grid.y = randint0(c->height - 1) + 1;
		grid.x = randint0(c->width - 1) + 1;
		form_grids += make_formation(c, p, grid, FEAT_TREE, FEAT_TREE2,
									 form_feats, "Forest", c->depth + 1);
	}

	/* And some water */
	form_grids = 0;
	while (form_grids < 300) {
		grid.y = randint0(c->height - 1) + 1;
		grid.x = randint0(c->width - 1) + 1;
		form_grids += make_formation(c, p, grid, FEAT_TREE, FEAT_TREE2, ponds,
									 "Forest", 10);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	return c;
}

/**
 * Generate a new swamp level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *swamp_gen(struct player *p, int height, int width)
{
	struct loc grid;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_TREE, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with grass */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Add water */
	for (grid.y = 1; grid.y < c->height - 1; grid.y++) {
		for (grid.x = 2; grid.x < c->width - 1; grid.x++) {
			if (feat_is_permanent(square_feat(c, grid)->fidx))
				continue;
			if (loc_eq(p->grid, grid) || (randint0(100) < 50)) {
				square_set_feat(c, grid, FEAT_GRASS);
			} else {
				square_set_feat(c, grid, FEAT_WATER);
			}
		}
	}

	/* Place some formations (but not many, and less for more danger) */
	while (form_grids < 20000 / c->depth) {
		/* Choose a place */
		grid.y = randint0(c->height - 1) + 1;
		grid.x = randint0(c->width - 1) + 1;
		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_WATER,
									 form_feats, "Swamp", c->depth);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	return c;
}

/**
 * Generate a new desert level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *desert_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int j, d;
	int plats;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_GRASS, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_DUNE, FEAT_QUARTZ, FEAT_NONE };
	bool made_gate = false;

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with grass so paths work */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Dungeon entrance */
	if (world->levels[place].down) {
		/* No vaults */
		num_wild_vaults = 0;

		/* Angband! */
		for (d = 0; d < c->width; d++) {
			for (grid.y = 0; grid.y < d; grid.y++) {
				grid.x = d - grid.y;
				if (!square_in_bounds_fully(c, grid))
					continue;
				if (square_feat(c, grid)->fidx == FEAT_ROAD) {
					/* The gate of Angband */
					square_set_feat(c, grid, FEAT_MORE);
					made_gate = true;
					if (level_topography(last_place) == TOP_CAVE || turn < 10)
						player_place(c, p, grid);
					break;
				} else {
					/* The walls of Thangorodrim */
					square_set_feat(c, grid, FEAT_GRANITE);
				}
			}
			if (made_gate)
				break;
		}
	}

	/* Now place rubble, sand and magma */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create desert */
			if (square_feat(c, grid)->fidx == FEAT_GRASS) {
				if (one_in_(2))
					square_set_feat(c, grid, FEAT_DUNE);
				else if (one_in_(2))
					square_set_feat(c, grid, FEAT_PASS_RUBBLE);
				else
					square_set_feat(c, grid, FEAT_MAGMA);
			} else
				/* Prepare for clearings */
				square_mark(c, grid);
		}
	}

	/* Make a few clearings */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, y;

		/* Try for a clearing */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		y = randint0(c->height - 1) + 1;
		x = randint0(c->width - 1) + 1;
		made_plat = generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, false);

		/* Success ? */
		if (made_plat)
			plats--;

		/* Done ? */
		if (!plats)
			break;
	}

	/* Place some formations */
	while (form_grids < 20 * c->depth) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_RUBBLE, FEAT_MAGMA,
									 form_feats, "Desert", c->depth);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	return c;
}


/**
 * Generate a new river level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *river_gen(struct player *p, int height, int width)
{
	struct loc grid, centre;
	int i, y1;
	int *mid;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;
	int path;

	int form_feats[] = { FEAT_TREE, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;
	y1 = c->height / 2;

	/* Start with grass */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Remember the path in case it has to move */
	path = square_feat(c, p->grid)->fidx;

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Place the river, start in the middle third */
	i = c->width / 3 + randint0(c->width / 3);
	mid = mem_zalloc(c->height * sizeof(int));
	for (grid.y = 1; grid.y < c->height - 1; grid.y++) {
		/* Remember the midpoint */
		mid[grid.y] = i;

		for (grid.x = i - randint0(5) - 10; grid.x < i + randint0(5) + 10;
			 grid.x++) {
			/* Make the river */
			square_set_feat(c, grid, FEAT_WATER);
			square_mark(c, grid);
		}
		/* Meander */
		i += randint0(3) - 1;
	}

	/* Mark the centre */
	centre = loc(mid[y1], y1);

	/* Special dungeon entrances */
	if (world->levels[place].down) {
		/* No vaults */
		num_wild_vaults = 0;

		/* If we're at Sauron's Isle... */
		if (world->levels[place].locality == LOC_SIRION_VALE) {
			for (grid.y = y1 - 10; grid.y < y1 + 10; grid.y++) {
				for (grid.x = mid[grid.y] - 10; grid.x < mid[grid.y] + 10;
					 grid.x++) {
					if (distance(centre, grid) < 6) {
						/* ...make Tol-In-Gaurhoth... */
						square_set_feat(c, grid, FEAT_GRASS);
					} else {
						/* ...surround it by water... */
						square_set_feat(c, grid, FEAT_WATER);
					}
					/* ...and build the tower... */
					if (distance(centre, grid) < 2)
						square_set_feat(c, grid, FEAT_GRANITE);
				}
			}
			/* ...with door and stairs */
			square_set_feat(c, loc(mid[y1], y1 + 1), FEAT_CLOSED);
			square_set_feat(c, centre, FEAT_MORE);
			if (level_topography(last_place) == TOP_CAVE) {
				player_place(c, p, centre);
			}
		} else {
			/* Must be Nargothrond */
			/* This place is hard to get into... */
			centre = loc(mid[y1] - 6, y1);
			for (grid.y = y1 - 1; grid.y < y1 + 2; grid.y++) {
				for (grid.x = mid[y1] - 15; grid.x < mid[y1] - 5; grid.x++) {
					if (loc_eq(grid, centre)) {
						square_set_feat(c, grid, FEAT_MORE);
						if (level_topography(last_place) == TOP_CAVE) {
							player_place(c, p, grid);
						}
					} else {
						square_set_feat(c, grid, FEAT_GRANITE);
					}
				}
				for (grid.x = mid[y1] - 5; grid.x < mid[y1]; grid.x++) {
					square_set_feat(c, grid, FEAT_ROAD);
				}
				for (grid.x = mid[y1] - 4; grid.x < mid[y1] + 5; grid.x++) {
					square_set_feat(c, grid, FEAT_WATER);
				}
			}
		}
	}

	/* Place some formations */
	while (form_grids < 50 * c->depth + 1000) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);

		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_GRASS,
									 form_feats, "River", c->depth / 2);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Move the player out of the river */
	grid = p->grid;
	while ((square_feat(c, grid)->fidx == FEAT_WATER)
		   || (square_feat(c, grid)->fidx == FEAT_GRANITE)) {
		grid.x++;
	}

	/* Place player if they had to move */
	if (!loc_eq(grid, p->grid)) {
		player_place(c, p, grid);
		square_set_feat(c, grid, path);
		for (grid.y = p->grid.y; grid.y > 0; grid.y--) {
			if (square_iswall(c, grid)) {
				square_set_feat(c, grid, FEAT_ROAD);
			}
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	mem_free(mid);
	return c;
}



/**
 * Generate a new valley level. Place down slides, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 */
struct chunk *valley_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int i, j, k;
	int plats;
	int form_grids = 0;
	int form_feats[] = { FEAT_GRASS, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	c->depth = p->depth;

	/* Start with trees */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create trees */
			if (randint1(c->depth + HIGHLAND_TREE_CHANCE)
				> HIGHLAND_TREE_CHANCE) {
				square_set_feat(c, grid, FEAT_TREE2);
			} else {
				square_set_feat(c, grid, FEAT_TREE);
			}
		}
	}

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, true);

	/* Make a few clearings */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, y;

		/* Try for a clearing */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		y = randint1(c->height - 3);
		x = randint1(c->width - 2);
		if (square_feat(c, loc(x, y))->fidx == FEAT_VOID) continue;
		made_plat = generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, true);

		/* Success ? */
		if (made_plat)
			plats--;

		/* Done ? */
		if (!plats)
			break;
	}

	/* Place some formations */
	while (form_grids < (40 * c->depth)) {
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_TREE, FEAT_TREE2,
									 form_feats, NULL, c->depth + 1);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	if (!p->upkeep->path_coord) {
		grid.y = c->height / 2 - 10 + randint0(20);
		grid.x = c->width / 2 - 15 + randint0(30);
		square_set_feat(c, grid, FEAT_GRASS);
		player_place(c, p, grid);
		p->upkeep->path_coord = 0;

		/* Make sure a web can't be placed on the player */
		square_mark(c, grid);
	}

	/* Place some webs */
	k = MIN(30, c->depth / 2);
	for (i = 0; i < damroll(k / 20, 4); i++)
		place_web(c, p, "Small web");

	if (one_in_(2))
		place_web(c, p, "Medium web");

	if (one_in_(10) == 0)
		place_web(c, p, "Large web");

	/* Place objects, traps and monsters */
	k = populate(c, true);

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);

			/* Paranoia - remake the dungeon walls */
			if ((grid.y == 0) || (grid.x == 0) ||
				(grid.y == c->height - 1) || (grid.x == c->width - 1)) {
				square_set_feat(c, grid, FEAT_PERM);
			}
		}
	}

	/* Maybe place a few random portals. */
	if (strstr(world->name, "Dungeon") && world->levels[p->place].down) {
		k = randint1(3) + 1;
		while (k > 0) {
			struct loc grid;
			grid.y = randint1(c->height - 1);
			grid.x = randint1(c->width - 1);
			if (square_istree(c, grid)) {
				square_set_feat(c, grid, FEAT_MORE);
				k--;
			}
		}
	}

	return c;
}
