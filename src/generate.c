/** \file generate.c 
    \brief Dungeon generation
 
 * Code generating a new level.  Level feelings and other 
 * messages, autoscummer behavior.  Creation of the town.  
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
#include "store.h"
#include "trap.h"


/*
 * Level generation is not an important bottleneck, though it can be 
 * annoyingly slow on older machines...  Thus we emphasize simplicity 
 * and correctness over speed.  See individual functions for notes.
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * The "v_info.txt" file is used to store vault generation info.
 */


/**
 * Dungeon generation data -- see  cave_gen()
 */
dun_data *dun;

/**
 * Is the level moria-style?
 */
bool moria_level;

/**
 * Is the level underworld?
 */
bool underworld;

/**
 * Number of wilderness vaults
 */
int wild_vaults;


/**
 * Builds a store at a given pseudo-location
 *
 * As of 2.8.1 (?) the town is actually centered in the middle of a
 * complete level, and thus the top left corner of the town itself
 * is no longer at (0,0), but rather, at (qy,qx), so the constants
 * in the comments below should be mentally modified accordingly.
 *
 * As of 2.7.4 (?) the stores are placed in a more "user friendly"
 * configuration, such that the four "center" buildings always
 * have at least four grids between them, to allow easy running,
 * and the store doors tend to face the middle of town.
 *
 * The stores now lie inside boxes from 3-9 and 12-18 vertically,
 * and from 7-17, 21-31, 35-45, 49-59.  Note that there are thus
 * always at least 2 open grids between any disconnected walls.
 * 
 * The home only appears if it is the player's home town.
 *
 * Note the use of town_illuminate() to handle all "illumination"
 * and "memorization" issues.
 */
static void build_store(int n, int yy, int xx, int stage)
{
	int y, x, y0, x0, y1, x1, y2, x2, tmp;

	int qy = 0;
	int qx = 0;


	/* Find the "center" of the store */
	y0 = qy + yy * 9 + 6;
	x0 = qx + xx * 11 + 11;

	/* Determine the store boundaries */
	y1 = y0 - (1 + randint1((yy == 0) ? 2 : 1));
	y2 = y0 + (1 + randint1((yy == 1) ? 2 : 1));
	x1 = x0 - (1 + randint1(3));
	x2 = x0 + (1 + randint1(3));

	/* Build an invulnerable rectangular building */
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			/* Create the building (or not ... NRM) */
			if ((n != 7) || (p_ptr->home == stage))
				cave_set_feat(y, x, FEAT_PERM_EXTRA);
			else
				cave_set_feat(y, x, FEAT_FLOOR);
		}
	}

	/* Pick a door direction (S,N,E,W) */
	tmp = randint0(4);

	/* Re-roll "annoying" doors */
	if (((tmp == 0) && (yy == 1)) || ((tmp == 1) && (yy == 0))
		|| ((tmp == 2) && (xx == 3)) || ((tmp == 3) && (xx == 0))) {
		/* Pick a new direction */
		tmp = randint0(4);
	}

	/* Extract a "door location" */
	switch (tmp) {
		/* Bottom side */
	case 0:
		{
			y = y2;
			x = rand_range(x1, x2);
			break;
		}

		/* Top side */
	case 1:
		{
			y = y1;
			x = rand_range(x1, x2);
			break;
		}

		/* Right side */
	case 2:
		{
			y = rand_range(y1, y2);
			x = x2;
			break;
		}

		/* Left side */
	default:
		{
			y = rand_range(y1, y2);
			x = x1;
			break;
		}
	}

	/* Clear previous contents, add a store door */
	if ((n != 7) || (p_ptr->home == stage))
		cave_set_feat(y, x, FEAT_SHOP_HEAD + n);
	else
		cave_set_feat(y, x, FEAT_FLOOR);
}




/**
 * Generate the "consistent" town features, and place the player
 *
 * Hack -- play with the R.N.G. to always yield the same town
 * layout, including the size and shape of the buildings, the
 * locations of the doorways, and the location of the stairs.
 */
static void town_gen_hack(void)
{
	int i, y, x, k, n, py = 1, px = 1;

	int qy = DUNGEON_HGT / 3;
	int qx = DUNGEON_WID / 3;
	int stage = p_ptr->stage;
	int last_stage = p_ptr->last_stage;

	int rooms[MAX_STORES_BIG + 1];
	int per_row = (MAP(DUNGEON) || MAP(FANILLA) ? 5 : 4);

	bool place = FALSE;
	bool major = FALSE;

	/* Hack -- Use the "simple" RNG */
	Rand_quick = TRUE;

	/* Hack -- Induce consistent town layout */
	for (i = 0; i < 10; i++)
		if (stage == towns[i])
			Rand_value = seed_town[i];

	if (MAP(DUNGEON) || MAP(FANILLA))
		Rand_value = seed_town[0];

	/* Set major town flag if necessary */
	if ((stage > GLADDEN_FIELDS_TOWN) || MAP(DUNGEON) || MAP(FANILLA))
		major = TRUE;

	/* Hack - reduce width for minor towns */
	if (!major)
		qx /= 2;

	/* Prepare an Array of "remaining stores", and count them */
	if (major)
		for (n = 0; n < MAX_STORES_BIG; n++)
			rooms[n] = n;
	else {
		rooms[0] = 9;
		rooms[1] = 3;
		rooms[2] = 4;
		rooms[3] = 7;
		n = 4;
	}

	if (MAP(DUNGEON) || MAP(FANILLA))
		rooms[n++] = 9;

	/* No stores for ironmen away from home */
	if ((!MODE(IRONMAN)) || (p_ptr->stage == p_ptr->home)) {
		/* Place two rows of stores */
		for (y = 0; y < 2; y++) {
			/* Place two, four or five stores per row */
			for (x = 0; x < per_row; x++) {
				/* Pick a random unplaced store */
				k = ((n <= 1) ? 0 : randint0(n));
				
				/* Build that store at the proper location */
				build_store(rooms[k], y, x, stage);
				
				/* Shift the stores down, remove one store */
				rooms[k] = rooms[--n];
				
				/* Cut short if a minor town */
				if ((x > 0) && !major)
					break;
			}
		}
		/* Hack -- Build the 9th store.  Taken from Zangband */
		if (major && (MAP(COMPRESSED) || MAP(FANILLA)))
			build_store(rooms[0], randint0(2), 4, stage);
	}
	
	if (MAP(DUNGEON) || MAP(FANILLA)) {
		/* Place the stairs */
		while (TRUE) {
			feature_type *f_ptr;
			
			/* Pick a location at least "three" from the outer walls */
			y = 1 + rand_range(3, DUNGEON_HGT / 3 - 4);
			x = 1 + rand_range(3, DUNGEON_WID / 3 - 4);
			
			/* Require a floor grid */
			f_ptr = &f_info[cave_feat[y][x]];
			if (tf_has(f_ptr->flags, TF_FLOOR))
				break;
		}
		
		/* Clear previous contents, add down stairs */
		cave_set_feat(y, x, FEAT_MORE);
		
		
		/* Place the player */
		player_place(y, x);
	}

	else {
		/* Place the paths */
		for (n = 2; n < 6; n++) {
			/* Pick a path direction for the player if not obvious */
			if (((!last_stage) || (last_stage == 255))
				&& (stage_map[stage][n]))
				last_stage = stage_map[stage][n];

			/* Where did we come from? */
			if ((last_stage) && (last_stage == stage_map[stage][n]))
				place = TRUE;

			/* Pick a location at least "three" from the corners */
			y = 1 + rand_range(3, qy - 4);
			x = 1 + rand_range(3, qx - 4);

			/* Shove it to the wall, place the path */
			switch (n) {
			case NORTH:
				{
					y = 1;
					if (stage_map[stage][n])
						cave_set_feat(y, x, FEAT_MORE_NORTH);
					break;
				}
			case EAST:
				{
					x = qx - 2;
					if (stage_map[stage][n])
						cave_set_feat(y, x, FEAT_MORE_EAST);
					break;
				}
			case SOUTH:
				{
					y = qy - 2;
					if (stage_map[stage][n])
						cave_set_feat(y, x, FEAT_MORE_SOUTH);
					break;
				}
			case WEST:
				{
					x = 1;
					if (stage_map[stage][n])
						cave_set_feat(y, x, FEAT_MORE_WEST);
				}
			}
			if (place) {
				py = y;
				px = x;
				place = FALSE;
			}
		}

		/* Place the player */
		player_place(py, px);
	}

	/* Hack -- use the "complex" RNG */
	Rand_quick = FALSE;
}




/**
 * Town logic flow for generation of new town
 *
 * We start with a fully wiped cave of normal floors.
 *
 * Note that town_gen_hack() plays games with the R.N.G.
 *
 * This function does NOT do anything about the owners of the stores,
 * nor the contents thereof.  It only handles the physical layout.
 *
 * We place the player on the stairs at the same time we make them.
 *
 * Hack -- since the player always leaves the dungeon by the stairs,
 * he is always placed on the stairs, even if he left the dungeon via
 * word of recall or teleport level.
 */
static void town_gen(void)
{
	int i, y, x;

	int residents;
	int stage = p_ptr->stage;

	int qy = 0;
	int qx = 0;
	int width = DUNGEON_WID / 3;

	bool dummy;

	/* Hack - smaller for minor towns */
	if ((stage < KHAZAD_DUM_TOWN) && !MAP(DUNGEON)	&& !MAP(FANILLA))
		width /= 2;

	/* Day time */
	if (is_daylight) {
		/* Number of residents */
		residents = MIN_M_ALLOC_TD;
	}

	/* Night time or cave */
	else {
		/* Number of residents */
		residents = MIN_M_ALLOC_TN;
	}

	/* Start with solid walls */
	for (y = 0; y < DUNGEON_HGT; y++) {
		for (x = 0; x < DUNGEON_WID; x++) {
			/* Create "solid" perma-wall */
			cave_set_feat(y, x, FEAT_PERM_SOLID);
		}
	}

	/* Boundary walls (see town_illuminate() */
	for (x = qx; x < qx + width; x++) {
		cave_set_feat(qy, x, FEAT_PERM_INNER);
		cave_set_feat(qy + (DUNGEON_HGT / 3) - 1, x, FEAT_PERM_INNER);
	}

	/* Boundary walls (see town_illuminate() */
	for (y = qy; y < qy + (DUNGEON_HGT / 3); y++) {
		cave_set_feat(y, qx, FEAT_PERM_INNER);
		cave_set_feat(y, qx + width - 1, FEAT_PERM_INNER);
	}

	/* Then place some floors */
	for (y = qy + 1; y < qy + (DUNGEON_HGT / 3) - 1; y++) {
		for (x = qx + 1; x < qx + width - 1; x++) {
			/* Create empty floor */
			cave_set_feat(y, x, FEAT_FLOOR);
		}
	}

	/* Build stuff */
	town_gen_hack();

	/* Remove restrictions */
	(void) mon_restrict('\0', 0, &dummy, FALSE);

	/* Make some residents */
	for (i = 0; i < residents; i++) {
		/* Make a resident */
		(void) alloc_monster(3, TRUE, FALSE);
	}
}





/**
 * Clear the dungeon, ready for generation to begin.
 */
static void clear_cave(void)
{
	int x, y;

	wipe_o_list();
	wipe_m_list();
	wipe_trap_list();
	/* Clear flags and flow information. */
	for (y = 0; y < DUNGEON_HGT; y++) {
		for (x = 0; x < DUNGEON_WID; x++) {
			/* No features */
			cave_feat[y][x] = 0;

			/* No flags */
			sqinfo_wipe(cave_info[y][x]);

			/* No flow */
			cave_cost[y][x] = 0;
			cave_when[y][x] = 0;

			/* Clear any left-over monsters (should be none) and the player. */
			cave_m_idx[y][x] = 0;
		}
	}

	/* Mega-Hack -- no player in dungeon yet */
	p_ptr->px = p_ptr->py = 0;

	/* Hack -- illegal panel */
	Term->offset_y = DUNGEON_HGT;
	Term->offset_x = DUNGEON_WID;


	/* Nothing good here yet */
	rating = 0;
}



/**
 * Generate a random dungeon level
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 *
 * Note that this function resets flow data and grid flags directly.
 * Note that this function does not reset features, monsters, or objects.  
 * Features are left to the town and dungeon generation functions, and 
 * wipe_m_list() and wipe_o_list() handle monsters and objects.
 */
void generate_cave(void)
{
	int y, x, num;

	level_hgt = DUNGEON_HGT;
	level_wid = DUNGEON_WID;
	clear_cave();

	/* The dungeon is not ready */
	character_dungeon = FALSE;

	/* Don't know feeling yet */
	do_feeling = FALSE;

	/* Assume level is not themed. */
	p_ptr->themed_level = 0;

	/* Generate */
	for (num = 0; TRUE; num++) {
		int max = 2;
		bool okay = TRUE;
		const char *why = NULL;

		/* Reset monsters and objects */
		o_max = 1;
		m_max = 1;


		/* Clear flags and flow information. */
		for (y = 0; y < DUNGEON_HGT; y++) {
			for (x = 0; x < DUNGEON_WID; x++) {
				/* No flags */
				sqinfo_wipe(cave_info[y][x]);

				/* No flow */
				cave_cost[y][x] = 0;
				cave_when[y][x] = 0;

			}
		}


		/* Mega-Hack -- no player in dungeon yet */
		cave_m_idx[p_ptr->py][p_ptr->px] = 0;
		p_ptr->px = p_ptr->py = 0;

		/* Reset the monster generation level */
		monster_level = p_ptr->depth;

		/* Reset the object generation level */
		object_level = p_ptr->depth;

		/* Nothing good here yet */
		rating = 0;

		/* Only group is the player */
		group_id = 1;

		/* Set the number of wilderness "vaults" */
		wild_vaults = 0;
		if (OPT(night_mare))
			max += 2;

		if (p_ptr->depth > 10)
			wild_vaults += randint0(max);
		if (p_ptr->depth > 20)
			wild_vaults += randint0(max);
		if (p_ptr->depth > 30)
			wild_vaults += randint0(max);
		if (p_ptr->depth > 40)
			wild_vaults += randint0(max);

		if (no_vault())
			wild_vaults = 0;

		/* Build the town */
		if (!p_ptr->depth) {
			/* Make a town */
			town_gen();
		}

		/* Not town */
		else {
			/* It is possible for levels to be themed. */
			if ((randint0(THEMED_LEVEL_CHANCE) == 0)
				&& build_themed_level()) {
				/* Message. */
				if (OPT(cheat_room))
					msg("Themed level");
			}

			/* Build a real stage */
			else {
				switch (stage_map[p_ptr->stage][STAGE_TYPE]) {
				case CAVE:
					{
						cave_gen();
						break;
					}

				case VALLEY:
					{
						valley_gen();
						break;
					}

				case MOUNTAIN:
					{
						mtn_gen();
						break;
					}

				case MOUNTAINTOP:
					{
						mtntop_gen();
						break;
					}

				case FOREST:
					{
						forest_gen();
						break;
					}

				case SWAMP:
					{
						swamp_gen();
						break;
					}

				case RIVER:
					{
						river_gen();
						break;
					}

				case DESERT:
					{
						desert_gen();
						break;
					}

				case PLAIN:
					{
						plain_gen();
					}
				}
			}
		}

		okay = TRUE;


		/* Extract the feeling */
		if (rating > 50 + p_ptr->depth)
			feeling = 2;
		else if (rating > 40 + 4 * p_ptr->depth / 5)
			feeling = 3;
		else if (rating > 30 + 3 * p_ptr->depth / 5)
			feeling = 4;
		else if (rating > 20 + 2 * p_ptr->depth / 5)
			feeling = 5;
		else if (rating > 15 + 1 * p_ptr->depth / 3)
			feeling = 6;
		else if (rating > 10 + 1 * p_ptr->depth / 5)
			feeling = 7;
		else if (rating > 5 + 1 * p_ptr->depth / 10)
			feeling = 8;
		else if (rating > 0)
			feeling = 9;
		else
			feeling = 10;

		/* Hack -- no feeling in the town */
		if (!p_ptr->depth)
			feeling = 0;


		/* Prevent object over-flow */
		if (o_max >= z_info->o_max) {
			/* Message */
			why = "too many objects";

			/* Message */
			okay = FALSE;
		}

		/* Prevent monster over-flow */
		if (m_max >= z_info->m_max) {
			/* Message */
			why = "too many monsters";

			/* Message */
			okay = FALSE;
		}

		/* Mega-Hack -- "auto-scum" */
		if (OPT(auto_scum) && (num < 100) && !(p_ptr->themed_level)) {
			int fudge = (no_vault()? 3 : 0);

			/* Require "goodness" */
			if ((feeling > fudge + 9)
				|| ((p_ptr->depth >= 5) && (feeling > fudge + 8))
				|| ((p_ptr->depth >= 10) && (feeling > fudge + 7))
				|| ((p_ptr->depth >= 20) && (feeling > fudge + 6))) {
				/* Give message to cheaters */
				if (OPT(cheat_room) || OPT(cheat_hear) || OPT(cheat_peek)
					|| OPT(cheat_xtra)) {
					/* Message */
					why = "boring level";
				}

				/* Try again */
				okay = FALSE;
			}
		}

		/* Message */
		if ((OPT(cheat_room)) && (why))
			msg("Generation restarted (%s)", why);

		/* Accept */
		if (okay)
			break;

		/* Wipe the objects */
		wipe_o_list();

		/* Wipe the monsters */
		wipe_m_list();

		/* A themed level was generated */
		if (p_ptr->themed_level) {
			/* Allow the themed level to be generated again */
			p_ptr->themed_level_appeared &=
				~(1L << (p_ptr->themed_level - 1));

			/* This is not a themed level */
			p_ptr->themed_level = 0;
		}
	}


	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* Reset path_coord */
	p_ptr->path_coord = 0;

	/* Verify the panel */
	verify_panel();

	/* Apply illumination */
	illuminate();

	/* Reset the number of traps, runes, and thefts on the level. */
	num_trap_on_level = 0;
	number_of_thefts_on_level = 0;
	for (num = 0; num < RUNE_TAIL; num++)
		num_runes_on_level[num] = 0;
}
