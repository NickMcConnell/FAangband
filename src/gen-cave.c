/** \file gen-cave.c 
    \brief Dungeon generation
 
 * Code for generating dungeon levels.
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


/**
 * Room type information
 */
typedef struct room_data room_data;

struct room_data {
	/* Allocation information. */
	s16b room_gen_num[11];

	/* Minimum level on which room can appear. */
	byte min_level;
};

/**
 * Table of values that control how many times each type of room will, 
 * on average, appear on 100 levels at various depths.  Each type of room 
 * has its own row, and each column corresponds to dungeon levels 0, 10, 
 * 20, and so on.  The final value is the minimum depth the room can appear 
 * at.  -LM-
 *
 * Level 101 and below use the values for level 100.
 *
 * Rooms with lots of monsters or loot may not be generated if the object or 
 * monster lists are already nearly full.  Rooms will not appear above their 
 * minimum depth.  No type of room (other than type 1) can appear more than 
 * DUN_ROOMS/2 times in any level.
 *
 * The entries for room type 1 are blank because these rooms are built once 
 * all other rooms are finished -- until the level fills up, or the room 
 * count reaches the limit (DUN_ROOMS).
 */
static room_data room[ROOM_MAX] = {
	/* Depth: 0 10 20 30 40 50 60 70 80 90 100 min */

	/* Nothing */ {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0},
	/* Simple */ {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0},
	/* Overlap */ {{60, 80, 100, 120, 140, 165, 180, 200, 220, 240, 260}, 1},
	/* Cross */ {{0, 25, 50, 70, 85, 100, 110, 120, 130, 140, 150}, 3},
	/* Large */ {{0, 25, 50, 70, 85, 100, 110, 120, 130, 140, 150}, 3},
	/* Pit */ {{0, 5, 12, 25, 30, 35, 38, 40, 40, 40, 40}, 5},
	/* Chambers */ {{0, 2, 6, 12, 15, 18, 19, 20, 20, 20, 20}, 5},
	/* I.Room */ {{50, 60, 70, 80, 80, 80, 80, 80, 80, 80, 80}, 0},
	/* L.Vault */ {{0, 1, 4, 9, 16, 27, 40, 55, 70, 80, 90}, 5},
	/* G.Vault */ {{0, 0, 1, 2, 3, 4, 6, 7, 8, 10, 12}, 20},
	/* Huge */ {{0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4}, 41}
};





/**
 * Value "1" means the grid will be changed, value "0" means it won't.
 *
 * We have 47 entries because 47 is not divisible by any reasonable 
 * figure for streamer width.
 */
static bool streamer_change_grid[47] = {
	0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1,
	1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0
};


/**
 * Places "streamers" of rock through dungeon.
 *
 * Note that there are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 */
static void build_streamer(int feat, int chance)
{
	int table_start;
	int i;
	int y, x, dy, dx;
	int start_dir, dir;
	int out1, out2;
	bool change;

	/* Initialize time until next turn, and time until next treasure */
	int time_to_treas = randint1(chance * 2);
	int time_to_turn = randint1(DUN_STR_CHG * 2);


	/* Set standard width.  Vary width sometimes. */
	int width = 2 * DUN_STR_WID + 1;
	if (randint0(6) == 0)
		width += randint1(3);
	else if (randint0(6) == 0)
		width -= randint1(3);
	if (width < 1)
		width = 1;

	/* Set expansion outward from centerline. */
	out1 = width / 2;
	out2 = (width + 1) / 2;


	/* Hack -- Choose starting point */
	y = rand_spread(DUNGEON_HGT / 2, DUNGEON_HGT / 4);
	x = rand_spread(DUNGEON_WID / 2, DUNGEON_WID / 4);

	/* Choose a random compass direction */
	dir = start_dir = ddd[randint0(8)];

	/* Get an initial start position on the grid alteration table. */
	table_start = randint0(47);

	/* Place streamer into dungeon */
	while (TRUE) {
		feature_type *f_ptr;

		/* Advance streamer width steps on the table. */
		table_start += width;

		/* 
		 * Change grids outwards along sides.  If moving diagonally, 
		 * change a cross-shaped area.
		 */
		if (ddy[dir]) {
			for (dx = x - out1; dx <= x + out2; dx++) {
				/* Stay within dungeon. */
				if (!in_bounds(y, dx))
					continue;

				f_ptr = &f_info[cave_feat[y][dx]];

				/* Only convert "granite" walls */
				if (!tf_has(f_ptr->flags, TF_GRANITE) ||
					tf_has(f_ptr->flags, TF_DOOR_ANY))
					continue;

				i = table_start + dx - x;

				if ((i < 47) && (i >= 0))
					change = streamer_change_grid[i];
				else
					change = streamer_change_grid[i % 47];

				/* No change to be made. */
				if (!change)
					continue;

				/* Clear previous contents, add proper vein type */
				cave_set_feat(y, dx, feat);

				/* Count down time to next treasure. */
				time_to_treas--;

				/* Hack -- Add some (known) treasure */
				if (time_to_treas == 0) {
					time_to_treas = randint1(chance * 2);
					cave_feat[y][dx] += 0x04;
				}
			}
		}

		if (ddx[dir]) {
			for (dy = y - out1; dy <= y + out2; dy++) {
				/* Stay within dungeon. */
				if (!in_bounds(dy, x))
					continue;

				f_ptr = &f_info[cave_feat[dy][x]];

				/* Only convert "granite" walls */
				if (!tf_has(f_ptr->flags, TF_GRANITE) ||
					tf_has(f_ptr->flags, TF_DOOR_ANY))
					continue;

				i = table_start + dy - y;

				if ((i < 47) && (i >= 0))
					change = streamer_change_grid[i];
				else
					change = streamer_change_grid[i % 47];

				/* No change to be made. */
				if (!change)
					continue;

				/* Clear previous contents, add proper vein type */
				cave_set_feat(dy, x, feat);

				/* Count down time to next treasure. */
				time_to_treas--;

				/* Hack -- Add some (known) treasure */
				if (time_to_treas == 0) {
					time_to_treas = randint1(chance * 2);
					cave_feat[dy][x] += 0x04;
				}
			}
		}

		/* Count down to next direction change. */
		time_to_turn--;

		/* Sometimes, vary direction slightly. */
		if (time_to_turn == 0) {
			/* Get time until next turn. */
			time_to_turn = randint1(DUN_STR_CHG * 2);

			/* Randomizer. */
			i = randint0(3);

			/* New direction is always close to start direction. */
			if (start_dir == 2) {
				if (i == 0)
					dir = 2;
				if (i == 1)
					dir = 1;
				else
					dir = 3;
			} else if (start_dir == 8) {
				if (i == 0)
					dir = 8;
				if (i == 1)
					dir = 9;
				else
					dir = 7;
			} else if (start_dir == 6) {
				if (i == 0)
					dir = 6;
				if (i == 1)
					dir = 3;
				else
					dir = 9;
			} else if (start_dir == 4) {
				if (i == 0)
					dir = 4;
				if (i == 1)
					dir = 7;
				else
					dir = 1;
			} else if (start_dir == 3) {
				if (i == 0)
					dir = 3;
				if (i == 1)
					dir = 2;
				else
					dir = 6;
			} else if (start_dir == 1) {
				if (i == 0)
					dir = 1;
				if (i == 1)
					dir = 4;
				else
					dir = 2;
			} else if (start_dir == 9) {
				if (i == 0)
					dir = 9;
				if (i == 1)
					dir = 6;
				else
					dir = 8;
			} else if (start_dir == 7) {
				if (i == 0)
					dir = 7;
				if (i == 1)
					dir = 8;
				else
					dir = 4;
			}
		}

		/* Advance the streamer */
		y += ddy[dir];
		x += ddx[dir];

		/* Stop at dungeon edge */
		if (!in_bounds(y, x))
			break;
	}
}



/**
 * Build a destroyed level
 */
void destroy_level(bool new_level)
{
	int y1, x1, y, x, k, t, n, epicenter_max;


	/* Note destroyed levels. */
	if (OPT(cheat_room) && new_level)
		msg("Destroyed Level");

	/* determine the maximum number of epicenters. */
	if (new_level)
		epicenter_max = randint1(5) + 1;
	else
		epicenter_max = randint1(5) + 5;

	/* Drop a few epi-centers */
	for (n = 0; n < epicenter_max; n++) {
		/* Pick an epi-center */
		x1 = rand_range(5, DUNGEON_WID - 1 - 5);
		y1 = rand_range(5, DUNGEON_HGT - 1 - 5);

		/* Big area of affect */
		for (y = (y1 - 15); y <= (y1 + 15); y++) {
			for (x = (x1 - 15); x <= (x1 + 15); x++) {
				/* Skip illegal grids */
				if (!in_bounds_fully(y, x))
					continue;

				/* Extract the distance */
				k = distance(y1, x1, y, x);

				/* Stay in the circle of death */
				if (k >= 16)
					continue;

				/* Delete the monster (if any) */
				delete_monster(y, x);

				/* Destroy valid grids */
				if (cave_valid_bold(y, x)) {
					/* Delete objects */
					delete_object(y, x);

					/* Wall (or floor) type */
					t = randint0(200);

					/* Granite */
					if (t < 20) {
						/* Create granite wall */
						cave_set_feat(y, x, FEAT_WALL_EXTRA);
					}

					/* Quartz */
					else if (t < 60) {
						/* Create quartz vein */
						cave_set_feat(y, x, FEAT_QUARTZ);
					}

					/* Magma */
					else if (t < 80) {
						/* Create magma vein */
						cave_set_feat(y, x, FEAT_MAGMA);
					}

					/* Rubble. */
					else if (t < 130) {
						/* Create rubble */
						cave_set_feat(y, x, FEAT_RUBBLE);
					}

					/* Floor */
					else {
						/* Create floor */
						cave_set_feat(y, x, FEAT_FLOOR);
					}

					/* No longer part of a room or vault */
					sqinfo_off(cave_info[y][x], SQUARE_ROOM);
					sqinfo_off(cave_info[y][x], SQUARE_ICKY);

					/* No longer illuminated */
					sqinfo_off(cave_info[y][x], SQUARE_GLOW);
				}
			}
		}
	}
}






/**
 * Helper function that reads the room data table and returns the number 
 * of rooms, of a given type, we should build on this level.
 */
static int num_rooms_allowed(int room_type)
{
	int allowed = 0;
	int base_num, num_tries, mod, i;

	/* Point to the room information. */
	room_data *rm_ptr = &room[room_type];


	/* No rooms allowed above their minimum depth. */
	if (p_ptr->depth < rm_ptr->min_level)
		return (0);

	/* No "nothing" rooms. */
	if (room_type == 0)
		return (0);

	/* No special limit on ordinary rooms. */
	if (room_type == 1)
		return (DUN_ROOMS);


	/* If below level 100, use the rarity value for level 100. */
	if (p_ptr->depth > 100) {
		base_num = rm_ptr->room_gen_num[10];
	} else {
		mod = p_ptr->depth % 10;

		/* If depth is divisable by 10, use the appropriate table value. */
		if (mod == 0) {
			base_num = rm_ptr->room_gen_num[p_ptr->depth / 10];
		}
		/* Otherwise, use a weighted average of the nearest values. */
		else {
			base_num =
				((mod * rm_ptr->room_gen_num[(p_ptr->depth + 9) / 10])
				 + ((10 - mod) * rm_ptr->room_gen_num[p_ptr->depth / 10]
				 )) / 10;
		}
	}

	/* Nightmare */
	if (OPT(night_mare))
		base_num *= room_type;

	/* Find out how many times we'll try to boost the room count. */
	num_tries = 3 * base_num / 100;
	if (num_tries < 2)
		num_tries = (base_num < 12 ? 1 : 2);
	if (num_tries > DUN_ROOMS / 2)
		num_tries = DUN_ROOMS / 2;

	/* No multiple huge rooms on any level. */
	if (room_type == 10)
		num_tries = 1;

	/* Try several times to increase the number of rooms to build. */
	for (i = 0; i < num_tries; i++) {
		if (randint0(1000) < 10 * base_num / num_tries) {
			allowed++;
		}
	}

	/* Return the number of rooms of that type we should build. */
	return (allowed);
}



/**************************************************************/
/*                                                            */
/*                     The tunnelling code                    */
/*                                                            */
/**************************************************************/


/**
 * Given a current position (y1, x1), move towards the target grid 
 * (y2, x2) either vertically or horizontally.
 *
 * If both vertical and horizontal directions seem equally good, 
 * prefer to move horizontally.
 */
extern void correct_dir(int *row_dir, int *col_dir, int y1, int x1, int y2,
						int x2)
{
	/* Move vertically if vertical distance to target is greater. */
	if (ABS(y1 - y2) > ABS(x1 - x2)) {
		*row_dir = ((y1 < y2) ? 1 : -1);
		*col_dir = 0;
	}

	/* Prefer to move horizontally. */
	else {
		*row_dir = 0;
		*col_dir = ((x1 < x2) ? 1 : -1);
	}
}


/**
 * Go in a semi-random direction from current location to target location.  
 * Do not actually head away from the target grid.  Always make a turn.
 */
extern void adjust_dir(int *row_dir, int *col_dir, int y1, int x1, int y2,
					   int x2)
{
	/* Always turn 90 degrees. */
	if (*row_dir == 0) {
		*col_dir = 0;

		/* On the y-axis of target - freely choose a side to turn to. */
		if (y1 == y2)
			*row_dir = ((randint0(2) == 0) ? -1 : 1);

		/* Never turn away from target. */
		else
			*row_dir = ((y1 < y2) ? 1 : -1);
	} else {
		*row_dir = 0;

		/* On the x-axis of target - freely choose a side to turn to. */
		if (x1 == x2)
			*col_dir = ((randint0(2) == 0) ? -1 : 1);

		/* Never turn away from target. */
		else
			*col_dir = ((x1 < x2) ? 1 : -1);
	}
}


/**
 * Go in a completely random orthongonal direction.  If we turn around 
 * 180 degrees, save the grid; it may be a good place to place stairs 
 * and/or the player.
 */
static void rand_dir(int *row_dir, int *col_dir, int y, int x)
{
	/* Pick a random direction */
	int i = randint0(4);

	/* Extract the dy/dx components */
	int row_dir_tmp = ddy_ddd[i];
	int col_dir_tmp = ddx_ddd[i];

	/* Save useful grids. */
	if ((-(*row_dir) == row_dir_tmp) && (-(*col_dir) == col_dir_tmp)) {
		/* Save the current tunnel location if surrounded by walls. */
		if ((in_bounds_fully(y, x)) && (dun->stair_n < STAIR_MAX)
			&& (next_to_walls(y, x) == 4)) {
			dun->stair[dun->stair_n].y = y;
			dun->stair[dun->stair_n].x = x;
			dun->stair_n++;
		}
	}

	/* Save the new direction. */
	*row_dir = row_dir_tmp;
	*col_dir = col_dir_tmp;
}


/** Terrain type is unalterable and impassable. */
static bool unalterable(byte feat)
{

	/* A few features are unalterable. */
	if (tf_has(f_info[feat].flags, TF_PERMANENT)) {
		return (TRUE);
	}

	/* Assume alterable */
	return (FALSE);
}

/**
 * Given a set of coordinates, return the index number of the room occupying 
 * the dungeon block this location is in.
 */
static int get_room_index(int y, int x)
{
	/* Which block are we in? */
	int by = y / BLOCK_HGT;
	int bx = x / BLOCK_WID;

	/* Paranoia -- confirm that block is in the dungeon. */
	if ((by > MAX_ROOMS_ROW) || (by > MAX_ROOMS_ROW))
		return (-1);

	/* Get the room index. */
	return (dun->room_map[by][bx] - 1);
}



/**
 * Search for a vault entrance.
 *
 * Notes:
 * - This function looks in both directions, and chooses the nearest 
 *   entrance (if it has a choice).
 * - We assume rooms will have outer walls surrounding them.
 * - We assume the vault designer hasn't designed false entrances, or
 *   done something else really sneaky.
 */
static bool find_entrance(int row_dir, int col_dir, int *row1, int *col1)
{
	int i, j;
	int y;
	int x;
	int dy, dx;

	/* 
	 * Initialize entrances found while looking in both directions, and 
	 * the distances to them.
	 */
	int target_y[2] = { 0, 0 };
	int target_x[2] = { 0, 0 };
	int grids[2] = { 0, 0 };


	/* Search in both directions. */
	for (i = 0; i < 2; i++) {
		bool stop_loop = FALSE;

		y = *row1;
		x = *col1;

		dy = row_dir;
		dx = col_dir;

		/* Keep running through the steps. */
		while (TRUE) {
			int dy_tmp = dy;

			/* Search grids on both sides for more impassable walls. */
			for (j = i; j < 2 + i; j++) {
				if (dy_tmp == 0) {
					dy = ((j == 1) ? -1 : 1);
					dx = 0;
				} else {
					dy = 0;
					dx = ((j == 1) ? -1 : 1);
				}

				/* Look in chosen direction. */
				if ((!unalterable(cave_feat[y + dy][x + dx]))
					&& (cave_feat[y + dy][x + dx] != FEAT_WALL_OUTER)) {
					/* 
					 * Check the grid after this one.  If it belongs 
					 * to the same room, we've found an entrance.
					 */
					if (get_room_index(y + dy, x + dx) ==
						get_room_index(y + dy + dy, x + dx + dx)) {
						target_y[i] = y + dy;
						target_x[i] = x + dx;
						break;
					}
				}

				/* Look again. */
				else if (unalterable(cave_feat[y + dy][x + dx])) {
					break;
				}

				/* We're out on some kind of weird spur. */
				else if (j == (1 + i)) {
					/* Stop travelling in this direction. */
					stop_loop = TRUE;
					break;
				}
			}

			/* Success or (known) failure. */
			if (target_y[i] && target_x[i])
				break;
			if (stop_loop)
				break;

			/* Keep heading in the same direction. */
			while (TRUE) {
				/* Advance to new grid in our direction of travel. */
				y += dy;
				x += dx;

				/* Count the number of grids we've travelled */
				grids[i]++;

				/* 
				 * We're back where we started.  Room either has no 
				 * entrances, or we can't find them.
				 */
				if ((y == *row1) && (x == *col1)) {
					stop_loop = TRUE;
					break;
				}

				/* We have hit the dungeon edge. */
				if (!in_bounds_fully(y + dy, x + dx)) {
					stop_loop = TRUE;
					break;
				}

				/* Next grid is outer wall. */
				if (cave_feat[y + dy][x + dx] == FEAT_WALL_OUTER) {
					/* We need to make another turn. */
					break;
				}

				/* Next grid is alterable, and not outer wall. */
				else if (!unalterable(cave_feat[y + dy][x + dx])) {
					/* 
					 * Check the grid after this one.  If it belongs 
					 * to the same room, we've found an entrance.
					 */
					if (get_room_index(y + dy, x + dx) ==
						get_room_index(y + dy + dy, x + dx + dx)) {
						target_y[i] = y + dy;
						target_x[i] = x + dx;
						break;
					}

					/* 
					 * If we're in the same room, our likely best move 
					 * is to keep moving along the permanent walls.
					 */
					else {
						break;
					}
				}
			}

			/* Success. */
			if (target_y[i] && target_x[i])
				break;

			/* Failure. */
			if (stop_loop)
				break;
		}
	}

	/* 
	 * Compare reports.  Pick the only target available, or choose 
	 * the target that took less travelling to get to.
	 */
	if ((target_y[0] && target_x[0]) && (target_y[1] && target_x[1])) {
		if (grids[0] < grids[1]) {
			*row1 = target_y[0];
			*col1 = target_x[0];
		} else {
			*row1 = target_y[1];
			*col1 = target_x[1];
		}

		return (TRUE);
	}

	else if (target_y[0] && target_x[0]) {
		*row1 = target_y[0];
		*col1 = target_x[0];
		return (TRUE);
	} else if (target_y[1] && target_x[1]) {
		*row1 = target_y[1];
		*col1 = target_x[1];
		return (TRUE);
	}

	/* No entrances found. */
	else
		return (FALSE);
}

/**
 * Tests suitability of potential entranceways, and places doors if 
 * appropriate.
 */
static void try_entrance(int y0, int x0)
{
	int i, k;

	/* Require walls on at least two sides. */
	for (k = 0, i = 0; i < 4; i++) {
		/* Extract the location */
		int y = y0 + ddy_ddd[i];
		int x = x0 + ddx_ddd[i];
		feature_type *f_ptr = &f_info[cave_feat[y][x]];

		/* Ignore non-walls. */
		if (!tf_has(f_ptr->flags, TF_WALL) ||
			tf_has(f_ptr->flags, TF_DOOR_ANY))
			continue;

		/* We require at least two walls. */
		if ((k++) == 2)
			place_random_door(y0, x0);
	}
}

/**
 * Places door at y, x position if at least 2 walls and two corridor spaces 
 * found
 */
static void try_door(int y0, int x0)
{
	int i, y, x;
	int k = 0;
	feature_type *f_ptr = &f_info[cave_feat[y0][x0]];


	/* Ignore non-floors */
	if (!tf_has(f_ptr->flags, TF_FLOOR))
		return;

	/* Ignore room grids */
	if (sqinfo_has(cave_info[y0][x0], SQUARE_ROOM))
		return;

	/* Occasional door (if allowed) */
	if (randint0(100) < DUN_TUN_JCT) {
		/* Count the adjacent non-wall grids */
		for (i = 0; i < 4; i++) {
			/* Extract the location */
			y = y0 + ddy_ddd[i];
			x = x0 + ddx_ddd[i];
			f_ptr = &f_info[cave_feat[y][x]];

			/* Skip grids without clear space */
			if (!tf_has(f_ptr->flags, TF_PROJECT))
				continue;

			/* Skip grids inside rooms */
			if (sqinfo_has(cave_info[y][x], SQUARE_ROOM))
				continue;

			/* We require at least two walls outside of rooms. */
			if ((k++) == 2)
				break;
		}

		if (k == 2) {
			int walls = 0;

			/* Check Vertical */
			for (i = -1; i <= 1; i += 2) {
				f_ptr = &f_info[cave_feat[y0 + i][x0]];
				if (tf_has(f_ptr->flags, TF_WALL) &&
					!tf_has(f_ptr->flags, TF_DOOR_ANY))
					walls++;
			}

			if (walls == 2) {
				place_random_door(y0, x0);
				return;
			}

			/* Check Horizontal */
			for (i = -1; i <= 1; i += 2) {
				f_ptr = &f_info[cave_feat[y0][x0 + i]];
				if (tf_has(f_ptr->flags, TF_WALL) &&
					!tf_has(f_ptr->flags, TF_DOOR_ANY))
					walls++;
			}

			if (walls == 2) {
				place_random_door(y0, x0);
				return;
			}
		}
	}
}




/**
 * Constructs a tunnel between two points. 
 *
 * The tunnelling code connects room centers together.  It is the respon-
 * sibility of rooms to ensure all grids in them are accessable from the 
 * center, or from a passable grid nearby if the center is a wall.
 *
 * (warnings)
 * This code is still beta-quality.  Use with care.  Known areas of 
 * weakness include: 
 * - A group of rooms may be connected to each other, and not to the rest 
 *   of the dungeon.  This problem is rare.
 * - While the entrance-finding code is very useful, sometimes the tunnel 
 *   gets lost on the way.
 * - On occasion, a tunnel will travel far too long.  It can even (rarely) 
 *   happen that it would lock up the game if not artifically stopped.
 * - There are number of minor but annoying problems, both old and new, 
 *   like excessive usage of tunnel grids, tunnels turning areas of the
 *   dungeon into Swiss cheese, and so on.
 * - This code is awfully, awfully long.
 *
 * (Handling the outer walls of rooms)
 * In order to place doors correctly, know when a room is connected, and 
 * keep entances and exits to rooms neat, we set and use several different 
 * kinds of granite.  Because of this, we must call this function before 
 * making streamers.
 * - "Outer" walls must surround rooms.  The code can handle outer walls 
 * up to two grids thick (which is common in non-rectangular rooms).
 * - When outer wall is pierced, "solid" walls are created along the axis 
 * perpendicular to the direction of movement for three grids in each 
 * direction.  This makes entrances tidy.
 * 
 * (Handling difficult terrain)
 * When an unalterable (permanent) wall is encountered, this code is 
 * capable of finding entrances and of using waypoints.  It is anticipated 
 * that this will make vaults behave better than they did.
 *
 * Useful terrain values:
 *   FEAT_WALL_EXTRA -- granite walls
 *   FEAT_WALL_INNER -- inner room walls
 *   FEAT_WALL_OUTER -- outer room walls
 *   FEAT_WALL_SOLID -- solid room walls
 *   FEAT_PERM_INNER -- inner room walls (perma)
 *   FEAT_PERM_OUTER -- outer room walls (perma)
 *   FEAT_PERM_SOLID -- dungeon border (perma)
 */
void build_tunnel(int start_room, int end_room)
{
	int i = 0, j = 0, tmp, y, x;
	int y0, x0, y1, x1;
	int dy, dx;

	int row_dir, col_dir;


	/* Get start and target grids. */
	int row1 = dun->cent[start_room].y;
	int col1 = dun->cent[start_room].x;
	int row2 = dun->cent[end_room].y;
	int col2 = dun->cent[end_room].x;
	int tmp_row = row1, tmp_col = col1;


	/* Store initial target, because we may have to use waypoints. */
	int initial_row2 = row2;
	int initial_col2 = col2;

	/* Not yet worried about our progress */
	int desperation = 0;

	/* Start out not allowing the placement of doors */
	bool door_flag = FALSE;

	/* Don't leave just yet */
	bool leave = FALSE;

	/* Not heading for a known entrance. */
	bool head_for_entrance = FALSE;

	/* Initialize some movement counters */
	int adjust_dir_timer = randint1(DUN_TUN_ADJ * 2);
	int rand_dir_timer = randint1(DUN_TUN_RND * 2);
	int correct_dir_timer = 0;


	/* Set number of tunnel grids and room entrances to zero. */
	dun->tunn_n = 0;
	dun->wall_n = 0;

	/* Start out heading in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or look like we're getting nowhere). */
	while ((row1 != initial_row2) || (col1 != initial_col2)) {
		/* Stop when tunnel is too long, or we want to stop. */
		if ((leave) || (dun->tunn_n == TUNN_MAX) || (j++ == 400))
			break;

		/* 
		 * If we've reached a waypoint, the source and destination rooms 
		 * should be connected to each other now, but they may not be to 
		 * the rest of the network.  Get another room center at random, 
		 * and move towards it.
		 */
		if ((row1 == row2) && (col1 == col2)) {
			while (TRUE) {
				i = randint0(dun->cent_n);
				if ((i != start_room) && (i != end_room))
					break;
			}

			row2 = initial_row2 = dun->cent[i].y;
			col2 = initial_col2 = dun->cent[i].x;

			head_for_entrance = FALSE;
		}

		/* Try moving randomly if we seem stuck. */
		else if ((row1 != tmp_row) && (col1 != tmp_col)) {
			desperation++;

			/* Try a 90 degree turn. */
			if (desperation == 1) {
				adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
				adjust_dir_timer = 3;
			}

			/* Try turning randomly. */
			else if (desperation < 4) {
				rand_dir(&row_dir, &col_dir, row1, col1);
				correct_dir_timer = 2;
			} else {
				/* We've run out of ideas.  Stop wasting time. */
				break;
			}
		}

		/* We're making progress. */
		else {
			/* No worries. */
			desperation = 0;

			/* Check room. */
			tmp = get_room_index(row1, col1);

			/* We're in our destination room - head straight for target. */
			if ((tmp == end_room) &&
				sqinfo_has(cave_info[row1][col1], SQUARE_ROOM)) {
				correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
			}

			else {
				/* Count down times until next movement changes. */
				if (adjust_dir_timer > 0)
					adjust_dir_timer--;
				if (rand_dir_timer > 0)
					rand_dir_timer--;
				if (correct_dir_timer > 0)
					correct_dir_timer--;

				/* Make a random turn, set timer. */
				if (rand_dir_timer == 0) {
					rand_dir(&row_dir, &col_dir, row1, col1);

					rand_dir_timer = randint1(DUN_TUN_RND * 2);
					correct_dir_timer = randint1(4);
				}

				/* Adjust direction, set timer. */
				else if (adjust_dir_timer == 0) {
					adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);

					adjust_dir_timer = randint1(DUN_TUN_ADJ * 2);
				}


				/* Go in correct direction. */
				else if (correct_dir_timer == 0) {
					correct_dir(&row_dir, &col_dir, row1, col1, row2,
								col2);

					/* Don't use again unless needed. */
					correct_dir_timer = -1;
				}
			}
		}


		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;

		/* Do not leave the dungeon */
		if (!in_bounds_fully(tmp_row, tmp_col)) {
			/* Adjust direction */
			adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;

			/* Our destination is illegal - stop. */
			if (!in_bounds_fully(tmp_row, tmp_col))
				break;
		}

		/* Tunnel through dungeon granite. */
		if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_EXTRA) {
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the current tunnel location */
			if (dun->tunn_n < TUNN_MAX) {
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}

			/* Allow door in next grid */
			door_flag = TRUE;

			continue;
		}


		/* Pierce outer walls of rooms. */
		else if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_OUTER) {
			/* Look ahead */
			y0 = tmp_row + row_dir;
			x0 = tmp_col + col_dir;

			/* No annoying little alcoves near edge. */
			if (!in_bounds_fully(y0, x0))
				continue;


			/* Disallow door in next grid */
			door_flag = FALSE;

			/* Hack -- delay turns */
			adjust_dir_timer++;
			rand_dir_timer++;

			/* Navigate around various kinds of walls */
			if ((cave_feat[y0][x0] == FEAT_WALL_SOLID)
				|| (cave_feat[y0][x0] == FEAT_WALL_INNER)
				|| (unalterable(cave_feat[y0][x0]))) {
				for (i = 0; i < 2; i++) {
					if (i == 0) {
						/* Check the more direct route first. */
						adjust_dir(&row_dir, &col_dir, row1, col1, row2,
								   col2);

						/* Verify that we haven't just been here. */
						if ((dun->tunn_n == 0)
							|| !(dun->tunn[dun->tunn_n - 1].y ==
								 row1 + row_dir)
							|| !(dun->tunn[dun->tunn_n - 1].x ==
								 col1 + col_dir)) {
							tmp_row = row1 + row_dir;
							tmp_col = col1 + col_dir;
						}

						else
							continue;
					}

					else {
						/* If that didn't work, try the other side. */
						tmp_row = row1 - row_dir;
						tmp_col = col1 - col_dir;
					}

					if ((!unalterable(cave_feat[tmp_row][tmp_col]))
						&& (cave_feat[tmp_row][tmp_col] != FEAT_WALL_SOLID)
						&& (cave_feat[tmp_row][tmp_col] != FEAT_WALL_OUTER)
						&& (cave_feat[tmp_row][tmp_col] !=
							FEAT_WALL_INNER)) {
						/* Accept the location */
						row1 = tmp_row;
						col1 = tmp_col;

						/* Save the current tunnel location */
						if (dun->tunn_n < TUNN_MAX) {
							dun->tunn[dun->tunn_n].y = row1;
							dun->tunn[dun->tunn_n].x = col1;
							dun->tunn_n++;
						}

						/* Continue */
						break;
					}

					/* No luck. */
					if (i == 1)
						continue;
				}
			}

			/* Handle a double line of outer walls robustly. */
			else if (cave_feat[y0][x0] == FEAT_WALL_OUTER) {
				/* Look ahead (again). */
				y1 = y0 + row_dir;
				x1 = x0 + col_dir;

				/* We've found something passable. */
				if (passable(cave_feat[y1][x1])) {
					/* Turn both outer wall grids into floor. */
					cave_set_feat(tmp_row, tmp_col, FEAT_FLOOR);
					cave_set_feat(y0, x0, FEAT_FLOOR);

					/* Save the wall location */
					if (dun->wall_n < WALL_MAX) {
						dun->wall[dun->wall_n].y = tmp_row;
						dun->wall[dun->wall_n].x = tmp_col;
						dun->wall_n++;
					}

					/* Accept this location */
					row1 = tmp_row = y0;
					col1 = tmp_col = x0;
				}

				/* No luck - look at the sides. */
				else {
					for (i = 0; i < 2; i++) {
						if (i == 0) {
							/* Check the more direct route first. */
							adjust_dir(&row_dir, &col_dir, row1, col1,
									   row2, col2);

							tmp_row = row1 + row_dir;
							tmp_col = col1 + col_dir;
						} else {
							/* If that didn't work, try the other side. */
							tmp_row = row1 - row_dir;
							tmp_col = col1 - col_dir;
						}

						if ((!unalterable(cave_feat[tmp_row][tmp_col]))
							&& (cave_feat[tmp_row][tmp_col] !=
								FEAT_WALL_SOLID)
							&& (cave_feat[tmp_row][tmp_col] !=
								FEAT_WALL_OUTER)
							&& (cave_feat[tmp_row][tmp_col] !=
								FEAT_WALL_INNER)) {
							/* Accept the location */
							row1 = tmp_row;
							col1 = tmp_col;

							/* Save the current tunnel location */
							if (dun->tunn_n < TUNN_MAX) {
								dun->tunn[dun->tunn_n].y = row1;
								dun->tunn[dun->tunn_n].x = col1;
								dun->tunn_n++;
							}

							/* Continue */
							break;
						}
					}
				}
			}

			/* Second grid contains any other kind of terrain. */
			else {
				/* Accept this location */
				row1 = tmp_row;
				col1 = tmp_col;

				/* Convert to floor grid */
				cave_set_feat(row1, col1, FEAT_FLOOR);

				/* Save the wall location */
				if (dun->wall_n < WALL_MAX) {
					dun->wall[dun->wall_n].y = row1;
					dun->wall[dun->wall_n].x = col1;
					dun->wall_n++;
				}
			}

			/* Forbid re-entry near this piercing. */
			if ((!unalterable(cave_feat[row1 + row_dir][col1 + col_dir]))
				&& sqinfo_has(cave_info[row1][col1], SQUARE_ROOM)) {
				if (row_dir) {
					for (x = col1 - 3; x <= col1 + 3; x++) {
						/* Convert adjacent "outer" walls */
						if ((in_bounds(row1, x))
							&& (cave_feat[row1][x] == FEAT_WALL_OUTER)) {
							/* Change the wall to a "solid" wall */
							cave_set_feat(row1, x, FEAT_WALL_SOLID);
						}
					}
				} else {
					for (y = row1 - 3; y <= row1 + 3; y++) {
						/* Convert adjacent "outer" walls */
						if ((in_bounds(y, col1))
							&& (cave_feat[y][col1] == FEAT_WALL_OUTER)) {
							/* Change the wall to a "solid" wall */
							cave_set_feat(y, col1, FEAT_WALL_SOLID);
						}
					}
				}

				/* Get current room. */
				tmp = get_room_index(row1, col1);

				/* Record our success. */
				if ((tmp != start_room) && (tmp != -1)) {
					/* If this room is connected, now our start room is too. */
					if (dun->connected[tmp]) {
						dun->connected[start_room] = TRUE;

						/* If our destination room is connected, we're done. */
						if (dun->connected[end_room])
							leave = TRUE;
					}

					/* If our start room was connected, this one is too. */
					else if (dun->connected[start_room])
						dun->connected[tmp] = TRUE;
				}

				continue;
			}
		}


		/* 
		 * We've hit a feature that can't be altered.
		 */
		else if (unalterable(cave_feat[tmp_row][tmp_col])) {
			/* We don't know what to do. */
			if (!head_for_entrance) {
				/* Get the room that occupies this block. */
				tmp = get_room_index(tmp_row, tmp_col);

				/* We're in our starting room. */
				if (tmp == start_room) {
					/* Look at next grid. */
					y = tmp_row + row_dir;
					x = tmp_col + col_dir;

					/* If the next grid is outer wall, we know we need to find
					 * an entrance.  Otherwise, travel through the wall. */
					if (cave_feat[y][x] != FEAT_WALL_OUTER) {
						row1 = tmp_row;
						col1 = tmp_col;
						continue;
					}
				}

				y = tmp_row;
				x = tmp_col;

				/* We need to find an entrance to this room. */
				if (!find_entrance(row_dir, col_dir, &y, &x)) {
					/* No entrance means insoluable trouble. */
					leave = TRUE;
					continue;
				}

				/* We're in our starting room. */
				if (tmp == start_room) {
					/* Jump immediately to entrance. */
					row1 = tmp_row = y;
					col1 = tmp_col = x;

					/* Look for outer wall to head for. */
					for (i = 0; i < 4; i++) {
						y = row1 + ddy_ddd[i];
						x = col1 + ddx_ddd[i];

						if (cave_feat[y][x] == FEAT_WALL_OUTER) {
							/* Aim for outer wall. */
							row_dir = ddy_ddd[i];
							col_dir = ddx_ddd[i];

							adjust_dir_timer = 2;
						}
					}
				}

				/* We're anywhere else. */
				else {
					/* Aim for given waypoint. */
					row2 = y;
					col2 = x;

					/* Reset the final target. */
					initial_row2 = y;
					initial_col2 = x;

					/* Enter "head for entrance" mode. */
					head_for_entrance = TRUE;
				}
			}

			/* We're heading for an entrance to a vault. */
			if (head_for_entrance) {
				/* Check both sides. */
				for (i = 0; i < 2; i++) {
					/* 
					 * Try going in the direction that best approches 
					 * the target first.  On the 2nd try, check the 
					 * opposite side.
					 */
					if (col_dir == 0) {
						dy = 0;
						if (i == 0)
							dx = ((col1 < col2) ? 1 : -1);
						else
							dx = ((col1 < col2) ? -1 : 1);
					} else {
						dx = 0;
						if (i == 0)
							dy = ((row1 < row2) ? 1 : -1);
						else
							dy = ((row1 < row2) ? -1 : 1);
					}

					/* Do not accept floor unless necessary. */
					/* if ((cave_feat[row1 + dy][col1 + dx] == FEAT_FLOOR) &&
					 * (i == 0)) continue; */


					/* Check to see if grid to this side is alterable. */
					if (!unalterable(cave_feat[row1 + dy][col1 + dx])) {
						/* Change direction. */
						row_dir = dy;
						col_dir = dx;

						/* Accept this location */
						row1 += row_dir;
						col1 += col_dir;

						/* Clear previous contents, add a floor */
						cave_set_feat(row1, col1, FEAT_FLOOR);

						/* Return to main loop. */
						break;
					}

					/* We seem to be in trouble. */
					else if (i == 1) {
						/* If we previously found floor, accept the floor. */
						if (cave_feat[row1 - (dy)][col1 - (dx)] ==
							FEAT_FLOOR) {
							/* Change direction. */
							row_dir = -(dy);
							col_dir = -(dx);

							/* Accept this location */
							row1 += row_dir;
							col1 += col_dir;

							break;
						}

						/* Otherwise, go backwards. */
						{
							/* Change direction. */
							row_dir = -(row_dir);
							col_dir = -(col_dir);

							/* Accept this location */
							row1 += row_dir;
							col1 += col_dir;

							break;
						}
					}
				}
			}
		}

		/* We've hit a solid wall. */
		else if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_SOLID) {
			/* check both sides, most direct route first. */
			for (i = 0; i < 2; i++) {
				if (i == 0) {
					/* Check the more direct route first. */
					adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);

					tmp_row = row1 + row_dir;
					tmp_col = col1 + col_dir;
				} else {
					/* If that didn't work, try the other side. */
					tmp_row = row1 - row_dir;
					tmp_col = col1 - col_dir;
				}

				if ((!unalterable(cave_feat[tmp_row][tmp_col]))
					&& (cave_feat[tmp_row][tmp_col] != FEAT_WALL_SOLID)) {
					/* Accept the location */
					row1 = tmp_row;
					col1 = tmp_col;

					/* Save the current tunnel location */
					if (dun->tunn_n < TUNN_MAX) {
						dun->tunn[dun->tunn_n].y = row1;
						dun->tunn[dun->tunn_n].x = col1;
						dun->tunn_n++;
					}

					/* Move on. */
					i = 2;
				}
			}

			continue;
		}

		/* Travel quickly through rooms. */
		else if (sqinfo_has(cave_info[tmp_row][tmp_col], SQUARE_ROOM)) {
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			continue;
		}

		/* 
		 * Handle all passable terrain outside of rooms (this is 
		 * usually another corridor).
		 */
		else if (passable(cave_feat[tmp_row][tmp_col])) {
			/* We've hit another tunnel. */
			if (cave_feat[tmp_row][tmp_col] == FEAT_FLOOR) {
				/* Collect legal door locations */
				if (door_flag) {
					/* Save the door location */
					if (dun->door_n < DOOR_MAX) {
						dun->door[dun->door_n].y = tmp_row;
						dun->door[dun->door_n].x = tmp_col;
						dun->door_n++;
					}

					/* No door in next grid */
					door_flag = FALSE;
				}

				/* Mark start room connected. */
				dun->connected[start_room] = TRUE;

				/* 
				 * If our destination room isn't connected, jump to 
				 * its center, and head towards the start room.
				 */
				if (dun->connected[end_room] == FALSE) {
					/* Swap rooms. */
					tmp = end_room;
					end_room = start_room;
					start_room = tmp;

					/* Re-initialize */
					row1 = dun->cent[start_room].y;
					col1 = dun->cent[start_room].x;
					row2 = dun->cent[end_room].y;
					col2 = dun->cent[end_room].x;
					initial_row2 = row2;
					initial_col2 = col2;
					tmp_row = row1, tmp_col = col1;
				} else {
					/* All done. */
					leave = TRUE;
				}

				continue;
			}

			/* Grid is not another tunnel.  Advance, make no changes. */
			row1 = tmp_row;
			col1 = tmp_col;

			continue;
		}
	}

	/* Turn the tunnel into corridor */
	for (i = 0; i < dun->tunn_n; i++) {
		/* Access the grid */
		y = dun->tunn[i].y;
		x = dun->tunn[i].x;

		/* Clear previous contents, add a floor */
		cave_set_feat(y, x, FEAT_FLOOR);
	}

	/* Make doors in entranceways. */
	for (i = 0; i < dun->wall_n; i++) {
		/* Access the grid */
		y = dun->wall[i].y;
		x = dun->wall[i].x;

		/* Sometimes, make a door in the entranceway */
		if (randint0(100) < DUN_TUN_PEN)
			try_entrance(y, x);
	}


	/* We've reached the target.  If one room was connected, now both are. */
	if ((row1 == initial_row2) && (col1 == initial_col2)) {
		if (dun->connected[start_room])
			dun->connected[end_room] = TRUE;
		else if (dun->connected[end_room])
			dun->connected[start_room] = TRUE;
	}
}


/**
 * Creation of themed levels.  Use a set of flags to ensure that no level 
 * is built more than once.  Store the current themed level number for later 
 * reference.  -LM-
 *
 * Checking for appropriateness of a themed level to a given stage is now
 * (FAangband 0.4.0) handled by a separate function. -NRM-
 *
 * see "lib/edit/t_info.txt".  
 */
bool themed_level_ok(byte choice)
{
	/* Already appeared */
	if (p_ptr->themed_level_appeared & (1L << (choice - 1)))
		return (FALSE);

	/* Check the location */
	switch (choice) {
	case THEME_ELEMENTAL:
		{
			if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
				&& (p_ptr->depth >= 35) && (p_ptr->depth <= 70))
				break;
			return (FALSE);
		}
	case THEME_DRAGON:
		{
			if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
				&& (p_ptr->depth >= 40) && (p_ptr->depth <= 80))
				break;
			return (FALSE);
		}
	case THEME_WILDERNESS:
		{
			if (stage_map[p_ptr->stage][STAGE_TYPE] == DESERT)
				break;
			return (FALSE);
		}
	case THEME_DEMON:
		{
			if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
				&& (p_ptr->depth >= 60) && (p_ptr->depth <= 255))
				break;
			return (FALSE);
		}
	case THEME_MINE:
		{
			if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
				&& (p_ptr->depth >= 20) && (p_ptr->depth <= 45))
				break;
			return (FALSE);
		}
	case THEME_WARLORDS:
		{
			if (((stage_map[p_ptr->stage][STAGE_TYPE] == FOREST)
				 || (stage_map[p_ptr->stage][STAGE_TYPE] == PLAIN))
				&& (p_ptr->depth >= 20))
				break;
			return (FALSE);
		}
	case THEME_AELUIN:
		{
			if (stage_map[p_ptr->stage][LOCALITY] == DORTHONION)
				break;
			return (FALSE);
		}
	case THEME_ESTOLAD:
		{
			if ((stage_map[p_ptr->stage][LOCALITY] == EAST_BELERIAND)
				&& (p_ptr->depth >= 10))
				break;
			return (FALSE);
		}
	case THEME_SLAIN:
		{
			if (stage_map[p_ptr->stage][LOCALITY] == ANFAUGLITH)
				break;
			return (FALSE);
		}
	default:
		return (FALSE);
	}

	/* Must be OK */
	return (TRUE);
}



extern bool build_themed_level(void)
{
	byte i, choice;

	struct vault *t_ptr;

	/* 
	 * Down stairs aren't allowed on quest levels, and we must give
	 * a chance for the quest monsters to appear.
	 */
	if (is_quest(p_ptr->stage) || no_vault())
		return (FALSE);

	/* Pick a themed level at random.  Our patience gives out after 40 tries. */
	for (i = 0; i < 40; i++) {
		/* Select a random themed level record. */
		choice = randint1(THEME_MAX);

		/* Accept the first themed level, among those not already generated,
		 * appropriate to be generated at this depth. */
		if (themed_level_ok(choice))
			break;

		/* Admit failure. */
		if (i == 39)
			return (FALSE);
	}

	/* Access the chosen themed level */
	t_ptr = &t_info[choice];


	/* Give the player something to read. */
	msg("Please wait.  This may take a little while.");

	/* Indicate that the player is on the selected themed level. */
	p_ptr->themed_level = choice;

	/* Build the themed level. */
	if (!build_vault(0, 0, 66, 198, t_ptr->text, FALSE, FALSE, 0)) {
		/* Oops.  We're /not/ on a themed level. */
		p_ptr->themed_level = 0;

		return (FALSE);
	}

	/* Get feeling text */
	my_strcpy(themed_feeling, t_ptr->message, sizeof(themed_feeling));

	/* Indicate that this theme is built, and should not appear again. */
	p_ptr->themed_level_appeared |= (1L << (choice - 1));

	/* Now have a feeling */
	do_feeling = TRUE;

	/* Update the level indicator */
	p_ptr->redraw |= (PR_DEPTH);

	/* Success. */
	return (TRUE);
}


/**
 * Generate a new dungeon level.  Determine if the level is destroyed, 
 * empty, or themed.  If themed, create a themed level.  Otherwise, build 
 * up to DUN_ROOMS rooms, type by type, in descending order of size and 
 * difficulty.
 *
 * Build the dungeon borders, scramble and connect the rooms.  Place stairs, 
 * doors, and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "icky" to indicate the presence of a vault.
 * We mark grids "temp" to prevent random monsters being placed there.
 *
 * Note that "dun_body" adds about 1100 bytes of memory to the stack.
 */
extern void cave_gen(void)
{
	int i, j, k, y, x, y1, x1;
	int by, bx;
	int num_to_build;
	int room_type;
	int rooms_built = 0;

	/* Build rooms in descending order of difficulty. */
	byte room_build_order[ROOM_MAX] = { 10, 9, 6, 8, 5, 7, 4, 3, 2, 1, 0 };

	bool destroyed = FALSE;
	bool dummy;

	dun_data dun_body;

	/* Global data */
	dun = &dun_body;

	moria_level = FALSE;
	underworld = FALSE;

	/* Teleport level from wilderness */
	if (stage_map[p_ptr->stage][LOCALITY] == UNDERWORLD)
		underworld = TRUE;

	/* It is possible for levels to be moria-style. */
	if (((p_ptr->depth >= 10) && (p_ptr->depth < 40)
		 && (randint0(MORIA_LEVEL_CHANCE) == 0)) || (underworld)) {
		moria_level = TRUE;
		if (OPT(cheat_room))
			msg("Moria level");

		/* Moria levels do not have certain kinds of rooms. */
		for (i = 0; i < ROOM_MAX; i++) {
			if ((room_build_order[i] >= 2) && (room_build_order[i] <= 4))
				room_build_order[i] = 0;
		}
	}

	/* It is possible for levels to be destroyed */
	if ((p_ptr->depth > 10) && (!is_quest(p_ptr->stage))
		&& (randint0(DEST_LEVEL_CHANCE) == 0)) {
		destroyed = TRUE;

		/* Destroyed levels do not have certain kinds of rooms. */
		for (i = 0; i < ROOM_MAX; i++) {
			if (room_build_order[i] > 5)
				room_build_order[i] = 0;
		}
	}


	/* Hack -- Start with basic granite (or floor, if empty) */
	for (y = 0; y < DUNGEON_HGT; y++) {
		for (x = 0; x < DUNGEON_WID; x++) {

/* Empty levels are useful for testing rooms. */
#if 0
			/* Create bare floors */
			cave_feat[y][x] = FEAT_FLOOR;

			break;

#endif							/* End of empty level testing code */

			/* Create granite wall */
			cave_feat[y][x] = FEAT_WALL_EXTRA;
		}
	}

	/* Actual maximum number of rooms on this level */
	dun->row_rooms = DUNGEON_HGT / BLOCK_HGT;
	dun->col_rooms = DUNGEON_WID / BLOCK_WID;

	/* No stair locations yet */
	dun->stair_n = 0;

	/* Initialize the room table */
	for (by = 0; by < dun->row_rooms; by++) {
		for (bx = 0; bx < dun->col_rooms; bx++) {
			dun->room_map[by][bx] = 0;
		}
	}

	/* No rooms are connected yet */
	for (i = 0; i < CENT_MAX; i++) {
		dun->connected[i] = FALSE;
	}

	/* No rooms yet */
	dun->cent_n = 0;


	/* 
	 * Build each type of room in turn until we cannot build any more.
	 */
	for (i = 0; i < ROOM_MAX; i++) {
		/* What type of room are we building now? */
		room_type = room_build_order[i];

		/* Find out how many rooms of this type we can build. */
		num_to_build = num_rooms_allowed(room_type);

		/* No vaults on Quest levels (for now) -BR- */
		if (is_quest(p_ptr->stage)
			&& ((room_type == 8) || (room_type == 9)))
			num_to_build = 0;

		/* Try to build all we are allowed. */
		for (j = 0; j < num_to_build; j++) {
			/* Stop building rooms when we hit the maximum. */
			if (rooms_built >= DUN_ROOMS)
				break;

			/* Build the room. */
			if (room_build(room_type)) {
				/* Increase the room built count. */
				if (room_type == 10)
					rooms_built += 5;
				else if ((room_type == 6) || (room_type == 9))
					rooms_built += 3;
				else if (room_type == 8)
					rooms_built += 2;
				else
					rooms_built++;
			}

			/* Go to next type of room on failure. */
			else
				break;
		}
	}

	/* Special boundary walls -- Top */
	for (x = 0; x < DUNGEON_WID; x++) {
		y = 0;

		/* Clear previous contents, add "solid" perma-wall */
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}

	/* Special boundary walls -- Bottom */
	for (x = 0; x < DUNGEON_WID; x++) {
		y = DUNGEON_HGT - 1;

		/* Clear previous contents, add "solid" perma-wall */
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}

	/* Special boundary walls -- Left */
	for (y = 0; y < DUNGEON_HGT; y++) {
		x = 0;

		/* Clear previous contents, add "solid" perma-wall */
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}

	/* Special boundary walls -- Right */
	for (y = 0; y < DUNGEON_HGT; y++) {
		x = DUNGEON_WID - 1;

		/* Clear previous contents, add "solid" perma-wall */
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}

	/* Hack -- Scramble the room order */
	for (i = 0; i < dun->cent_n; i++) {
		int pick1 = i;
		int pick2 = randint0(dun->cent_n);
		y1 = dun->cent[pick1].y;
		x1 = dun->cent[pick1].x;
		dun->cent[pick1].y = dun->cent[pick2].y;
		dun->cent[pick1].x = dun->cent[pick2].x;
		dun->cent[pick2].y = y1;
		dun->cent[pick2].x = x1;

		/* XXX XXX - swap around room index numbers. */
		for (by = 0; by < 6; by++) {
			for (bx = 0; bx < 18; bx++) {
				if (dun->room_map[by][bx] == pick2 + 1)
					dun->room_map[by][bx] = pick1 + 1;
				else if (dun->room_map[by][bx] == pick1 + 1)
					dun->room_map[by][bx] = pick2 + 1;
			}
		}
	}

	/* Start with no tunnel doors */
	dun->door_n = 0;

	/* Mark the first room as being connected. */
	dun->connected[0] = TRUE;

	/* Connect all the rooms together (and locate grids for tunnel doors) */
	for (i = 0; i < dun->cent_n; i++) {
		/* Connect the room to the next room. */
		if (i == dun->cent_n - 1)
			build_tunnel(dun->cent_n - 1, 0);
		else
			build_tunnel(i, i + 1);
	}

	/* Place tunnel doors */
	for (i = 0; i < dun->door_n; i++) {
		/* Extract junction location */
		y = dun->door[i].y;
		x = dun->door[i].x;

		/* Try placing doors */
		try_door(y, x - 1);
		try_door(y, x + 1);
		try_door(y - 1, x);
		try_door(y + 1, x);
	}


	/* Add some magma streamers */
	for (i = 0; i < DUN_STR_MAG; i++) {
		build_streamer(FEAT_MAGMA, DUN_STR_MC);
	}

	/* Add some quartz streamers */
	for (i = 0; i < DUN_STR_QUA; i++) {
		build_streamer(FEAT_QUARTZ, DUN_STR_QC);
	}


	/* Destroy the level if necessary */
	if (destroyed)
		destroy_level(TRUE);


	/* Place 3 or 4 down stairs near some walls */
	alloc_stairs(FEAT_MORE, rand_range(3, 4), 3);

	/* Place 1 or 2 up stairs near some walls */
	alloc_stairs(FEAT_LESS, rand_range(1, 2), 3);

	/* Place 2 or 3 down shafts near some walls */
	alloc_stairs(FEAT_MORE_SHAFT, rand_range(2, 3), 3);

	/* Place 1 or 2 up stairs near some walls */
	alloc_stairs(FEAT_LESS_SHAFT, rand_range(1, 2), 3);

	/* Determine the character location */
	new_player_spot();


	/* Basic "amount" */
	k = (p_ptr->depth / 3);
	if (k > 10)
		k = 10;
	if (k < 2)
		k = 2;


	/* Pick a base number of monsters */
	i = MIN_M_ALLOC_LEVEL + randint1(8);

	/* Moria levels have a lot more monsters. */
	if (moria_level)
		i *= 2;

	/* Moria levels have a high proportion of cave dwellers. */
	if (moria_level) {
		/* Set global monster restriction variables. */
		if (underworld)
			mon_restrict('x', (byte) p_ptr->depth, &dummy, TRUE);
		else
			mon_restrict('0', (byte) p_ptr->depth, &dummy, TRUE);
	} else {
		/* Remove all monster restrictions. */
		mon_restrict('\0', (byte) p_ptr->depth, &dummy, TRUE);
	}

	/* Build the monster probability table. */
	monster_level = p_ptr->depth;
	(void) get_mon_num(monster_level);

	/* Put some monsters in the dungeon */
	for (j = i + k; j > 0; j--) {
		/* Always have some random monsters */
		if ((get_mon_num_hook) && (j < 5)) {
			/* Remove all monster restrictions. */
			mon_restrict('\0', (byte) p_ptr->depth, &dummy, TRUE);

			/* Build the monster probability table. */
			(void) get_mon_num(p_ptr->depth);
		}

		/* 
		 * Place a random monster (quickly), but not in grids marked 
		 * "SQUARE_TEMP".
		 */
		(void) alloc_monster(0, TRUE, TRUE);
	}

	/* Ensure quest monsters */
	if (is_quest(p_ptr->stage)) {
		/* Ensure quest monsters */
		for (i = 1; i < z_info->r_max; i++) {
			monster_race *r_ptr = &r_info[i];
			/* Ensure quest monsters */
			if ((rf_has(r_ptr->flags, RF_QUESTOR))
				&& (r_ptr->level == p_ptr->depth) && (r_ptr->cur_num < 1)) {
				int y, x;

				/* Pick a location */
				while (1) {
					y = randint0(DUNGEON_HGT);
					x = randint0(DUNGEON_WID);

					if (cave_exist_mon(r_ptr, y, x, FALSE))
						break;


				}

				/* Place the questor */
				place_monster_aux(y, x, i, TRUE, TRUE);


			}
		}
	}

	/* Place some traps in the dungeon. */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

	/* Put some rubble in corridors. */
	alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(k));

	/* Put some objects in rooms */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT,
				 Rand_normal(DUN_AMT_ROOM, 3));

	/* Put some objects/gold in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
				 Rand_normal(DUN_AMT_ITEM, 3));
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD,
				 Rand_normal(DUN_AMT_GOLD, 3));


	/* Clear "temp" flags. */
	for (y = 0; y < DUNGEON_HGT; y++) {
		for (x = 0; x < DUNGEON_WID; x++) {
			sqinfo_off(cave_info[y][x], SQUARE_TEMP);
		}
	}
}
