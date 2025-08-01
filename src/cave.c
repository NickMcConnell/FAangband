/**
 * \file cave.c
 * \brief chunk allocation and utility functions
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "cmds.h"
#include "cmd-core.h"
#include "game-event.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-group.h"
#include "monster.h"
#include "obj-ignore.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "object.h"
#include "player-timed.h"
#include "trap.h"
#include "z-queue.h"

struct feature *f_info;
struct chunk *cave = NULL;

int FEAT_NONE;
int FEAT_FLOOR;
int FEAT_CLOSED;
int FEAT_OPEN;
int FEAT_BROKEN;
int FEAT_LESS;
int FEAT_MORE;
int FEAT_SECRET;
int FEAT_RUBBLE;
int FEAT_PASS_RUBBLE;
int FEAT_MAGMA;
int FEAT_QUARTZ;
int FEAT_MAGMA_K;
int FEAT_QUARTZ_K;
int FEAT_GRANITE;
int FEAT_PERM;
int FEAT_LESS_NORTH;
int FEAT_MORE_NORTH;
int FEAT_LESS_EAST;
int FEAT_MORE_EAST;
int FEAT_LESS_SOUTH;
int FEAT_MORE_SOUTH;
int FEAT_LESS_WEST;
int FEAT_MORE_WEST;
int FEAT_LAVA;
int FEAT_WATER;
int FEAT_TREE;
int FEAT_TREE2;
int FEAT_GRASS;
int FEAT_ROAD;
int FEAT_VOID;
int FEAT_DUNE;

/**
 * Global array for looping through the "keypad directions".
 */
const int16_t ddd[9] =
{ 2, 8, 6, 4, 3, 1, 9, 7, 5 };

/**
 * Global arrays for converting "keypad direction" into "offsets".
 */
const int16_t ddx[10] =
{ 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 };

const int16_t ddy[10] =
{ 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 };


const struct loc ddgrid[10] =
{ {0, 0}, {-1, 1}, {0, 1}, {1, 1}, {-1, 0}, {0, 0}, {1, 0}, {-1, -1}, {0, -1},
  {1, -1} };

/**
 * Global arrays for optimizing "ddx[ddd[i]]", "ddy[ddd[i]]" and
 * "loc(ddx[ddd[i]], ddy[ddd[i]])".
 *
 * This means that each entry in this array corresponds to the direction
 * with the same array index in ddd[].
 */
const int16_t ddx_ddd[9] =
{ 0, 0, 1, -1, 1, -1, 1, -1, 0 };

const int16_t ddy_ddd[9] =
{ 1, -1, 0, 0, 1, 1, -1, -1, 0 };

const struct loc ddgrid_ddd[9] =
{{0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}, {0, 0}};

/* Can mult these by 45 deg or 1.5 o'clock e.g. [6] -> 270 deg or 9 o'clock */
const int16_t clockwise_ddd[9] =
{ 8, 9, 6, 3, 2, 1, 4, 7, 5 };

const struct loc clockwise_grid[9] =
{{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, 0}};

/**
 * Precompute a bunch of calls to distance().
 *
 * The pair of arrays dist_offsets_y[n] and dist_offsets_x[n] contain the
 * offsets of all the locations with a distance of n from a central point,
 * with an offset of (0,0) indicating no more offsets at this distance.
 *
 * This is, of course, fairly unreadable, but it eliminates multiple loops
 * from the previous version.
 *
 * It is probably better to replace these arrays with code to compute
 * the relevant arrays, even if the storage is pre-allocated in hard
 * coded sizes.  At the very least, code should be included which is
 * able to generate and dump these arrays (ala "los()").  XXX XXX XXX
 */


static const int d_off_y_0[] =
{ 0 };

static const int d_off_x_0[] =
{ 0 };


static const int d_off_y_1[] =
{ -1, -1, -1, 0, 0, 1, 1, 1, 0 };

static const int d_off_x_1[] =
{ -1, 0, 1, -1, 1, -1, 0, 1, 0 };


static const int d_off_y_2[] =
{ -1, -1, -2, -2, -2, 0, 0, 1, 1, 2, 2, 2, 0 };

static const int d_off_x_2[] =
{ -2, 2, -1, 0, 1, -2, 2, -2, 2, -1, 0, 1, 0 };


static const int d_off_y_3[] =
{ -1, -1, -2, -2, -3, -3, -3, 0, 0, 1, 1, 2, 2,
  3, 3, 3, 0 };

static const int d_off_x_3[] =
{ -3, 3, -2, 2, -1, 0, 1, -3, 3, -3, 3, -2, 2,
  -1, 0, 1, 0 };


static const int d_off_y_4[] =
{ -1, -1, -2, -2, -3, -3, -3, -3, -4, -4, -4, 0,
  0, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 0 };

static const int d_off_x_4[] =
{ -4, 4, -3, 3, -2, -3, 2, 3, -1, 0, 1, -4, 4,
  -4, 4, -3, 3, -2, -3, 2, 3, -1, 0, 1, 0 };


static const int d_off_y_5[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -4, -4, -5, -5,
  -5, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5, 5,
  5, 0 };

static const int d_off_x_5[] =
{ -5, 5, -4, 4, -4, 4, -2, -3, 2, 3, -1, 0, 1,
  -5, 5, -5, 5, -4, 4, -4, 4, -2, -3, 2, 3, -1,
  0, 1, 0 };


static const int d_off_y_6[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -5, -5,
  -6, -6, -6, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5,
  5, 5, 6, 6, 6, 0 };

static const int d_off_x_6[] =
{ -6, 6, -5, 5, -5, 5, -4, 4, -2, -3, 2, 3, -1,
  0, 1, -6, 6, -6, 6, -5, 5, -5, 5, -4, 4, -2,
  -3, 2, 3, -1, 0, 1, 0 };


static const int d_off_y_7[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -5, -5,
  -6, -6, -6, -6, -7, -7, -7, 0, 0, 1, 1, 2, 2, 3,
  3, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 0 };

static const int d_off_x_7[] =
{ -7, 7, -6, 6, -6, 6, -5, 5, -4, -5, 4, 5, -2,
  -3, 2, 3, -1, 0, 1, -7, 7, -7, 7, -6, 6, -6,
  6, -5, 5, -4, -5, 4, 5, -2, -3, 2, 3, -1, 0,
  1, 0 };


static const int d_off_y_8[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6,
  -6, -6, -7, -7, -7, -7, -8, -8, -8, 0, 0, 1, 1,
  2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
  8, 8, 8, 0 };

static const int d_off_x_8[] =
{ -8, 8, -7, 7, -7, 7, -6, 6, -6, 6, -4, -5, 4,
  5, -2, -3, 2, 3, -1, 0, 1, -8, 8, -8, 8, -7,
  7, -7, 7, -6, 6, -6, 6, -4, -5, 4, 5, -2, -3,
  2, 3, -1, 0, 1, 0 };


static const int d_off_y_9[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6,
  -7, -7, -7, -7, -8, -8, -8, -8, -9, -9, -9, 0,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7,
  7, 8, 8, 8, 8, 9, 9, 9, 0 };

static const int d_off_x_9[] =
{ -9, 9, -8, 8, -8, 8, -7, 7, -7, 7, -6, 6, -4,
  -5, 4, 5, -2, -3, 2, 3, -1, 0, 1, -9, 9, -9,
  9, -8, 8, -8, 8, -7, 7, -7, 7, -6, 6, -4, -5,
  4, 5, -2, -3, 2, 3, -1, 0, 1, 0 };


const int *dist_offsets_y[10] =
{
	d_off_y_0, d_off_y_1, d_off_y_2, d_off_y_3, d_off_y_4,
	d_off_y_5, d_off_y_6, d_off_y_7, d_off_y_8, d_off_y_9
};

const int *dist_offsets_x[10] =
{
	d_off_x_0, d_off_x_1, d_off_x_2, d_off_x_3, d_off_x_4,
	d_off_x_5, d_off_x_6, d_off_x_7, d_off_x_8, d_off_x_9
};


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
const uint8_t side_dirs[20][8] = {
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
 * Given a "start" and "finish" location, extract a "direction",
 * which will move one step from the "start" towards the "finish".
 *
 * Note that we use "diagonal" motion whenever possible.
 *
 * We return DIR_NONE if no motion is needed.
 */
int motion_dir(struct loc start, struct loc finish)
{
	/* No movement required */
	if (loc_eq(start, finish)) return (DIR_NONE);

	/* South or North */
	if (start.x == finish.x) return ((start.y < finish.y) ? DIR_S : DIR_N);

	/* East or West */
	if (start.y == finish.y) return ((start.x < finish.x) ? DIR_E : DIR_W);

	/* South-east or South-west */
	if (start.y < finish.y) return ((start.x < finish.x) ? DIR_SE : DIR_SW);

	/* North-east or North-west */
	if (start.y > finish.y) return ((start.x < finish.x) ? DIR_NE : DIR_NW);

	/* Paranoia */
	return (DIR_NONE);
}

/**
 * Given a grid and a direction, extract the adjacent grid in that direction
 */
struct loc next_grid(struct loc grid, int dir)
{
	return loc(grid.x + ddgrid[dir].x, grid.y + ddgrid[dir].y);
}

/**
 * Find a terrain feature index by name
 */
int lookup_feat(const char *name)
{
	int i;

	/* Look for it */
	for (i = 0; i < z_info->f_max; i++) {
		struct feature *feat = &f_info[i];
		if (!feat->name)
			continue;

		/* Test for equality */
		if (streq(name, feat->name))
			return i;
	}

	/* Fail horribly */
	quit_fmt("Failed to find terrain feature %s", name);
	return -1;
}

/**
 * Set terrain constants to the indices from terrain.txt
 */
void set_terrain(void)
{
	FEAT_NONE = lookup_feat("unknown grid");
	FEAT_FLOOR = lookup_feat("open floor");
	FEAT_CLOSED = lookup_feat("closed door");
	FEAT_OPEN = lookup_feat("open door");
	FEAT_BROKEN = lookup_feat("broken door");
	FEAT_LESS = lookup_feat("up staircase");
	FEAT_MORE = lookup_feat("down staircase");
	FEAT_SECRET = lookup_feat("secret door");
	FEAT_RUBBLE = lookup_feat("pile of rubble");
	FEAT_PASS_RUBBLE = lookup_feat("pile of passable rubble");
	FEAT_MAGMA = lookup_feat("magma vein");
	FEAT_QUARTZ = lookup_feat("quartz vein");
	FEAT_MAGMA_K = lookup_feat("magma vein with treasure");
	FEAT_QUARTZ_K = lookup_feat("quartz vein with treasure");
	FEAT_GRANITE = lookup_feat("granite wall");
	FEAT_PERM = lookup_feat("permanent wall");
	FEAT_LESS_NORTH = lookup_feat("easy path north");
	FEAT_MORE_NORTH = lookup_feat("hard path north");
	FEAT_LESS_EAST = lookup_feat("easy path east");
	FEAT_MORE_EAST = lookup_feat("hard path east");
	FEAT_LESS_SOUTH = lookup_feat("easy path south");
	FEAT_MORE_SOUTH = lookup_feat("hard path south");
	FEAT_LESS_WEST = lookup_feat("easy path west");
	FEAT_MORE_WEST = lookup_feat("hard path west");
	FEAT_LAVA = lookup_feat("lava");
	FEAT_WATER = lookup_feat("water");
	FEAT_TREE = lookup_feat("lowland trees");
	FEAT_TREE2 = lookup_feat("highland trees");
	FEAT_GRASS = lookup_feat("grass");
	FEAT_ROAD = lookup_feat("road");
	FEAT_VOID = lookup_feat("empty space");
	FEAT_DUNE = lookup_feat("sand dune");
}

uint16_t **heatmap_new(struct chunk *c)
{
	uint16_t **grids;
	int y;
	grids = mem_zalloc(c->height * sizeof(uint16_t*));
	for (y = 0; y < c->height; y++) {
		grids[y] = mem_zalloc(c->width * sizeof(uint16_t));
	}
	return grids;
}

void heatmap_free(struct chunk *c, struct heatmap map)
{
	int y;
	for (y = 0; y < c->height; y++) {
		mem_free(map.grids[y]);
	}
	mem_free(map.grids);
}

/**
 * Allocate a new chunk of the world
 */
struct chunk *cave_new(int height, int width) {
	int y, x;

	struct chunk *c = mem_zalloc(sizeof *c);
	c->height = height;
	c->width = width;
	c->feat_count = mem_zalloc((z_info->f_max + 1) * sizeof(int));

	c->squares = mem_zalloc(c->height * sizeof(struct square*));
	c->noise.grids = heatmap_new(c);
	c->scent.grids = heatmap_new(c);
	for (y = 0; y < c->height; y++) {
		c->squares[y] = mem_zalloc(c->width * sizeof(struct square));
		for (x = 0; x < c->width; x++) {
			c->squares[y][x].info = mem_zalloc(SQUARE_SIZE * sizeof(bitflag));
		}
	}

	c->objects = mem_zalloc(OBJECT_LIST_SIZE * sizeof(struct object*));
	c->obj_max = OBJECT_LIST_SIZE - 1;

	c->monsters = mem_zalloc(z_info->level_monster_max *sizeof(struct monster));
	c->mon_max = 1;
	c->mon_current = -1;

	c->monster_groups = mem_zalloc(z_info->level_monster_max *
								   sizeof(struct monster_group*));

	c->ghost = mem_zalloc(sizeof(struct ghost_info));

	c->turn = turn;
	return c;
}

/**
 * Free a linked list of cave connections.
 */
void cave_connectors_free(struct connector *join)
{
	while (join) {
		struct connector *current = join;

		join = current->next;
		mem_free(current->info);
		mem_free(current);
	}
}

/**
 * Free a chunk
 */
void cave_free(struct chunk *c) {
	struct chunk *p_c = (c == cave && player) ? player->cave : NULL;
	int y, x, i;

	cave_connectors_free(c->join);

	/* Look for orphaned objects and delete them. */
	for (i = 1; i < c->obj_max; i++) {
		if (c->objects[i] && loc_is_zero(c->objects[i]->grid)) {
			object_delete(c, p_c, &c->objects[i]);
		}
	}

	for (y = 0; y < c->height; y++) {
		for (x = 0; x < c->width; x++) {
			mem_free(c->squares[y][x].info);
			if (c->squares[y][x].trap)
				square_free_trap(c, loc(x, y));
			if (c->squares[y][x].obj)
				object_pile_free(c, p_c, c->squares[y][x].obj);
		}
		mem_free(c->squares[y]);
	}
	mem_free(c->squares);
	heatmap_free(c, c->noise);
	heatmap_free(c, c->scent);

	mem_free(c->feat_count);
	mem_free(c->objects);
	mem_free(c->monsters);
	mem_free(c->monster_groups);
	if (c->ghost) {
		mem_free(c->ghost);
	}
	if (c->name)
		string_free(c->name);
	mem_free(c);
}


/**
 * Enter an object in the list of objects for the current level/chunk.  This
 * function is robust against listing of duplicates or non-objects
 */
void list_object(struct chunk *c, struct object *obj)
{
	int i, newsize;

	/* Check for duplicates and objects already deleted or combined */
	if (!obj) return;
	for (i = 1; i < c->obj_max; i++)
		if (c->objects[i] == obj)
			return;

	/* Put objects in holes in the object list */
	for (i = 1; i < c->obj_max; i++) {
		/* If there is a known object, skip this slot */
		if ((c == cave) && player->cave && player->cave->objects[i]) {
			continue;
		}

		/* Put the object in a hole */
		if (c->objects[i] == NULL) {
			c->objects[i] = obj;
			obj->oidx = i;
			return;
		}
	}

	/* Extend the list */
	newsize = (c->obj_max + OBJECT_LIST_INCR + 1) * sizeof(struct object*);
	c->objects = mem_realloc(c->objects, newsize);
	c->objects[c->obj_max] = obj;
	obj->oidx = c->obj_max;
	for (i = c->obj_max + 1; i <= c->obj_max + OBJECT_LIST_INCR; i++)
		c->objects[i] = NULL;
	c->obj_max += OBJECT_LIST_INCR;

	/* If we're on the current level, extend the known list */
	if ((c == cave) && player->cave) {
		player->cave->objects = mem_realloc(player->cave->objects, newsize);
		for (i = player->cave->obj_max; i <= c->obj_max; i++)
			player->cave->objects[i] = NULL;
		player->cave->obj_max = c->obj_max;
	}
}

/**
 * Remove an object from the list of objects for the current level/chunk.  This
 * function is robust against delisting of unlisted objects.
 */
void delist_object(struct chunk *c, struct object *obj)
{
	if (!obj->oidx) return;
	assert(c->objects[obj->oidx] == obj);

	/* Don't delist an actual object if it still has a listed known object */
	if ((c == cave) && player->cave->objects[obj->oidx]) return;

	c->objects[obj->oidx] = NULL;
	obj->oidx = 0;
}

/**
 * Check consistency of an object list or a pair of object lists
 *
 * If one list, check the listed objects relate to locations of
 * objects correctly
 */
void object_lists_check_integrity(struct chunk *c, struct chunk *c_k)
{
	int i;
	if (c_k) {
		assert(c->obj_max == c_k->obj_max);
		for (i = 0; i < c->obj_max; i++) {
			struct object *obj = c->objects[i];
			struct object *known_obj = c_k->objects[i];
			if (obj) {
				assert(obj->oidx == i);
				if (!loc_is_zero(obj->grid))
					assert(pile_contains(square_object(c, obj->grid), obj));
			}
			if (known_obj) {
				assert (obj);
				if (player->upkeep->playing) {
					assert(known_obj == obj->known);
				}
				if (!loc_is_zero(known_obj->grid))
					assert (pile_contains(square_object(c_k, known_obj->grid),
										  known_obj));
				assert (known_obj->oidx == i);
			}
		}
	} else {
		for (i = 0; i < c->obj_max; i++) {
			struct object *obj = c->objects[i];
			if (obj) {
				assert(obj->oidx == i);
				if (!loc_is_zero(obj->grid))
					assert(pile_contains(square_object(c, obj->grid), obj));
			}
		}
	}
}

/**
 * Standard "find me a location" function, now with all legal outputs!
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * need_los determines whether line of sight is needed
 */
void scatter(struct chunk *c, struct loc *place, struct loc grid, int d,
			 bool need_los)
{
	(void) scatter_ext(c, place, 1, grid, d, need_los, NULL);
}


/**
 * Try to find a given number of distinct, randomly selected, locations that
 * are within a given distance of a grid, fully in bounds, and, optionally,
 * are in the line of sight of the given grid and satisfy an additional
 * condition.
 * \param c Is the chunk to search.
 * \param places Points to the storage for the locations found.  That storage
 * must have space for at least n grids.
 * \param n Is the number of locations to find.
 * \param grid Is the location to use as the origin for the search.
 * \param d Is the maximum distance, in grids, that a location can be from
 * grid and still be accepted.
 * \param need_los If true, any locations found will also be in the line of
 * sight from grid.
 * \param pred If not NULL, evaluating that function at a found location, lct,
 * will return true, i.e. (*pred)(c, lct) will be true.
 * \return Return the number of locations found.  That number will be less
 * than or equal to n if n is not negative and will be zero if n is negative.
 */
int scatter_ext(struct chunk *c, struct loc *places, int n, struct loc grid,
		int d, bool need_los, bool (*pred)(struct chunk *, struct loc))
{
	int result = 0;
	/* Stores feasible locations. */
	struct loc *feas = mem_alloc(MIN(c->width, (1 + 2 * MAX(0, d)))
			* (size_t) MIN(c->height, (1 + 2 * MAX(0, d)))
			* sizeof(*feas));
	int nfeas = 0;
	struct loc g;

	/* Get the feasible locations. */
	for (g.y = grid.y - d; g.y <= grid.y + d; ++g.y) {
		for (g.x = grid.x - d; g.x <= grid.x + d; ++g.x) {
			if (!square_in_bounds_fully(c, g)) continue;
			if (d > 1 && distance(grid, g) > d) continue;
			if (need_los && !los(c, grid, g)) continue;
			if (pred && !(*pred)(c, g)) continue;
			feas[nfeas] = g;
			++nfeas;
		}
	}

	/* Assemble the result. */
	while (result < n && nfeas > 0) {
		/* Choose one at random and append it to the outgoing list. */
		int choice = randint0(nfeas);

		places[result] = feas[choice];
		++result;
		/* Shift the last feasible one to replace the one selected. */
		--nfeas;
		feas[choice] = feas[nfeas];
	}

	mem_free(feas);
	return result;
}

/**
 * Get a monster on the current level by its index.
 */
struct monster *cave_monster(struct chunk *c, int idx) {
	if (idx <= 0) return NULL;
	return &c->monsters[idx];
}

/**
 * The maximum number of monsters allowed in the level.
 */
int cave_monster_max(struct chunk *c) {
	return c->mon_max;
}

/**
 * The current number of monsters present on the level.
 */
int cave_monster_count(struct chunk *c) {
	return c->mon_cnt;
}

/**
 * Return the number of matching grids around (or under) the character.
 * \param grid If not NULL, *grid is set to the location of the last match.
 * \param test Is the predicate to use when testing for a match.
 * \param under If true, the character's grid is tested as well.
 * Only tests grids that are known and fully in bounds.
 */
int count_feats(struct loc *grid,
				bool (*test)(struct chunk *c, struct loc grid), bool under)
{
	int d;
	struct loc grid1;
	int count = 0; /* Count how many matches */

	/* Check around (and under) the character */
	for (d = 0; d < 9; d++) {
		/* if not searching under player continue */
		if ((d == 8) && !under) continue;

		/* Extract adjacent (legal) location */
		grid1 = loc_sum(player->grid, ddgrid_ddd[d]);

		/* Paranoia */
		if (!square_in_bounds_fully(cave, grid1)) continue;

		/* Must have knowledge */
		if (!square_isknown(cave, grid1)) continue;

		/* Not looking for this feature; test against player's memory */
		if (!((*test)(player->cave, grid1))) continue;

		/* Count it */
		++count;

		/* Remember the location of the last match */
		if (grid) {
			*grid = grid1;
		}
	}

	/* All done */
	return count;
}

/**
 * Return the number of matching grids around a location.
 * \param match If not NULL, *match is set to the location of the last match.
 * \param c Is the chunk to use.
 * \param grid Is the location whose neighbors will be tested.
 * \param test Is the predicate to use when testing for a match.
 * \param under If true, grid is tested as well.
 */
int count_neighbors(struct loc *match, struct chunk *c, struct loc grid,
	bool (*test)(struct chunk *c, struct loc grid), bool under)
{
	int dlim = (under) ? 9 : 8;
	int count = 0; /* Count how many matches */
	int d;
	struct loc grid1;

	/* Check the grid's neighbors and, if under is true, grid */
	for (d = 0; d < dlim; d++) {
		/* Extract adjacent (legal) location */
		grid1 = loc_sum(grid, ddgrid_ddd[d]);
		if (!square_in_bounds(c, grid1)) continue;

		/* Reject those that don't match */
		if (!((*test)(c, grid1))) continue;

		/* Count it */
		++count;

		/* Remember the location of the last match */
		if (match) {
			*match = grid1;
		}
	}

	/* All done */
	return count;
}

struct loc cave_find_decoy(struct chunk *c)
{
	return c->decoy;
}

/**
 * Every turn, the character makes enough noise that nearby monsters can use
 * it to home in.
 *
 * This function actually just computes distance from the player; this is
 * used in combination with the player's stealth value to determine what
 * monsters can hear.  We mark the player's grid with 0, then fill in the noise
 * field of every grid that the player can reach with that "noise"
 * (actally distance) plus the number of steps needed to reach that grid
 * - so higher values mean further from the player.
 *
 * Monsters use this information by moving to adjacent grids with lower noise
 * values, thereby homing in on the player even though twisty tunnels and
 * mazes.  Monsters have a hearing value, which is the largest sound value
 * they can detect.
 *
 * Update: Monsters can also have noise heatmaps generated for them
 */
void make_noise(struct chunk *c, struct player *p, struct monster *mon)
{
	struct loc next = p ? p->grid : mon->grid;
	int y, x, d;
	int noise = 0;
	int noise_increment = p && p->timed[TMD_COVERTRACKS] ? 4 : 1;
    struct queue *queue = q_new(c->height * c->width);
	struct loc decoy = cave_find_decoy(c);
	struct heatmap noise_map = p ? c->noise : mon->noise;

	/* Set all the grids to silence */
	for (y = 1; y < c->height - 1; y++) {
		for (x = 1; x < c->width - 1; x++) {
			noise_map.grids[y][x] = 0;
		}
	}

	/* If there's a decoy, use that instead of the player */
	if (p && !loc_is_zero(decoy)) {
		next = decoy;
	}

	/* Player/monster makes noise */
	noise_map.grids[next.y][next.x] = noise;
	q_push_int(queue, grid_to_i(next, c->width));
	noise += noise_increment;

	/* Propagate noise */
	while (q_len(queue) > 0) {
		/* Get the next grid */
		i_to_grid(q_pop_int(queue), c->width, &next);

		/* If we've reached the current noise level, put it back and step */
		if (noise_map.grids[next.y][next.x] == noise) {
			q_push_int(queue, grid_to_i(next, c->width));
			noise += noise_increment;
			continue;
		}

		/* Assign noise to the children and enqueue them */
		for (d = 0; d < 8; d++)	{
			/* Child location */
			struct loc grid = loc_sum(next, ddgrid_ddd[d]);

			if (!square_in_bounds(c, grid)) continue;

			/* Ignore features that don't transmit sound */
			if (square_isnoflow(c, grid)) continue;

			/* Skip grids that already have noise */
			if (noise_map.grids[grid.y][grid.x] != 0) continue;

			/* Skip the player/monster grid */
			if (p && loc_eq(player->grid, grid)) {
				continue;
			} else if (mon && loc_eq(mon->grid, grid)) {
				continue;
			}

			/* Save the noise */
			noise_map.grids[grid.y][grid.x] = noise;

			/* Enqueue that entry */
			q_push_int(queue, grid_to_i(grid, c->width));
		}
	}

	q_free(queue);
}

/**
 * Characters leave scent trails for perceptive monsters to track.
 *
 * Scent is rather more limited than sound.  Many creatures cannot use
 * it at all, it doesn't extend very far outwards from the character's
 * current position, and monsters can use it to home in the character,
 * but not to run away.
 *
 * Scent is valued according to age.  When a character takes his turn,
 * scent is aged by one, and new scent is laid down.  Monsters have a smell
 * value which indicates the oldest scent they can detect.  Grids where the
 * player has never been will have scent 0.  The player's grid will also have
 * scent 0, but this is OK as no monster will ever be smelling it.
 */
void update_scent(struct chunk *c, struct player *p, struct monster *mon)
{
	int y, x;
	int scent_strength[5][5] = {
		{2, 2, 2, 2, 2},
		{2, 1, 1, 1, 2},
		{2, 1, 0, 1, 2},
		{2, 1, 1, 1, 2},
		{2, 2, 2, 2, 2},
	};
	struct heatmap scent_map = p ? c->scent : mon->scent;

	/* Update scent for all grids */
	for (y = 1; y < c->height - 1; y++) {
		for (x = 1; x < c->width - 1; x++) {
			if (scent_map.grids[y][x] > 0) {
				scent_map.grids[y][x]++;
			}
		}
	}

	/* Scentless player */
	if (player->timed[TMD_COVERTRACKS]) return;

	/* Lay down new scent around the player */
	for (y = 0; y < 5; y++) {
		for (x = 0; x < 5; x++) {
			struct loc scent, grid = p ? player->grid : mon->grid;
			int new_scent = scent_strength[y][x];
			int d;
			bool add_scent = false;

			/* Initialize */
			scent.y = y + grid.y - 2;
			scent.x = x + grid.x - 2;

			/* Ignore invalid or non-scent-carrying grids */
			if (!square_in_bounds(c, scent)) continue;
			if (square_isnoscent(c, scent)) continue;

			/* Check scent is spreading on floors, not going through walls */
			for (d = 0; d < 8; d++)	{
				struct loc adj = loc_sum(scent, ddgrid_ddd[d]);

				if (!square_in_bounds(c, adj)) {
					continue;
				}

				/* Player grid is always valid */
				if (x == 2 && y == 2) {
					add_scent = true;
				}

				/* Adjacent to a closer grid, so valid */
				if (scent_map.grids[adj.y][adj.x] == new_scent - 1) {
					add_scent = true;
				}
			}

			/* Not valid */
			if (!add_scent) {
				continue;
			}

			/* Mark the scent */
			scent_map.grids[scent.y][scent.x] = new_scent;
		}
	}
}
