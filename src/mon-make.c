/**
 * \file mon-make.c
 * \brief Monster creation / placement code.
 *
 * Copyright (c) 1997-2007 Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "alloc.h"
#include "game-world.h"
#include "init.h"
#include "mon-group.h"
#include "mon-lore.h"
#include "mon-make.h"
#include "mon-predicate.h"
#include "mon-spell.h"
#include "mon-timed.h"
#include "mon-util.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-quest.h"
#include "player-timed.h"
#include "target.h"

int **race_prob;
/**
 * ------------------------------------------------------------------------
 * Monster race allocation
 *
 * Monster race allocation is done using an allocation table (see alloc.h).
 * This table is sorted by depth.  Each line of the table contains the
 * monster race index, the monster race level, and three probabilities:
 * - prob1 is the base probability of the race, calculated from monster.txt.
 * - prob2 is calculated by get_mon_num_prep(), which decides whether a
 *         monster is appropriate based on a secondary function; prob2 is
 *         always either prob1 or 0.
 * - prob3 is calculated by get_mon_num(), which checks whether universal
 *         restrictions apply (for example, unique monsters can only appear
 *         once on a given level); prob3 is always either prob2 or 0.
 * ------------------------------------------------------------------------ */
static s16b alloc_race_size;
static struct alloc_entry *alloc_race_table;

/**
 * Initialize monster allocation info
 */
static void init_race_allocs(void) {
	int i;
	struct monster_race *race;
	alloc_entry *table;
	s16b *num = mem_zalloc(z_info->max_depth * sizeof(s16b));
	s16b *already_counted = mem_zalloc(z_info->max_depth * sizeof(s16b));

	/* Size of "alloc_race_table" */
	alloc_race_size = 0;

	/* Scan the monsters (not the ghost) */
	for (i = 1; i < z_info->r_max - 1; i++) {
		/* Get the i'th race */
		race = &r_info[i];

		/* Legal monsters */
		if (race->rarity) {
			/* Count the entries */
			alloc_race_size++;

			/* Group by level */
			num[race->level]++;
		}
	}

	/* Calculate the cumultive level totals */
	for (i = 1; i < z_info->max_depth; i++) {
		/* Group by level */
		num[i] += num[i - 1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town monsters!");

	/* Allocate the alloc_race_table */
	alloc_race_table = mem_zalloc(alloc_race_size * sizeof(alloc_entry));

	/* Get the table entry */
	table = alloc_race_table;

	/* Scan the monsters (not the ghost) */
	for (i = 1; i < z_info->r_max - 1; i++) {
		/* Get the i'th race */
		race = &r_info[i];

		/* Count valid races */
		if (race->rarity) {
			int p, lev, prev_lev_count, race_index;

			/* Extract this race's level */
			lev = race->level;

			/* Extract the base probability */
			p = (100 / race->rarity);

			/* Multiply by depth factor (experimental) */
			p *= (1 + lev / 10);

			/* Skip entries preceding this monster's level */
			prev_lev_count = (lev > 0) ? num[lev - 1] : 0;

			/* Skip entries already counted for this level */
			race_index = prev_lev_count + already_counted[lev];

			/* Load the entry */
			table[race_index].index = i;
			table[race_index].level = lev;
			table[race_index].prob1 = p;
			table[race_index].prob2 = p;
			table[race_index].prob3 = p;

			/* Another entry complete for this locale */
			already_counted[lev]++;
		}
	}
	mem_free(already_counted);
	mem_free(num);
}

static void cleanup_race_allocs(void) {
	mem_free(alloc_race_table);
}


/**
 * Apply a monster restriction function to the monster allocation table.
 * This way, we can use get_mon_num() to get a level-appropriate monster that
 * satisfies certain conditions (such as belonging to a particular monster
 * family).
 */
void get_mon_num_prep(bool (*get_mon_num_hook)(struct monster_race *race))
{
	int i;

	/* Scan the allocation table */
	for (i = 0; i < alloc_race_size; i++) {
		alloc_entry *entry = &alloc_race_table[i];

		/* Check the restriction, if any */
		if (!get_mon_num_hook || (*get_mon_num_hook)(&r_info[entry->index])) {
			/* Accept this monster */
			entry->prob2 = entry->prob1;

		} else {
			/* Do not use this monster */
			entry->prob2 = 0;
		}
	}
}

/**
 * Helper function for get_mon_num(). Excludes monsters from selection
 * based on time, uniqueness, depth, locality, or topography
 */
static bool get_mon_forbidden(struct monster_race *race)
{
	time_t cur_time = time(NULL);
	struct tm *date = localtime(&cur_time);
	struct level *lev = &world->levels[player->place];

	/* No seasonal monsters outside of Christmas */
	if (rf_has(race->flags, RF_SEASONAL) &&
		!(date->tm_mon == 11 && date->tm_mday >= 24 && date->tm_mday <= 26))
		return true;

	/* Only one copy of a a unique must be around at the same time */
	if (rf_has(race->flags, RF_UNIQUE) && (race->cur_num >= race->max_num))
		return true;

	/* Some monsters never appear out of depth */
	if (rf_has(race->flags, RF_FORCE_DEPTH) && race->level > player->depth)
		return true;

	/* Some monsters only appear in a given dungeon... */
	if (rf_has(race->flags, RF_ANGBAND) && (lev->locality != LOC_ANGBAND))
		return true;

	/* ...if it exists on the current map */
	if (!streq(world->name, "Angband Dungeon")) {
		if (rf_has(race->flags, RF_AMON_RUDH) &&
			(lev->locality != LOC_AMON_RUDH)) {
			return true;
		} else if (rf_has(race->flags, RF_NARGOTHROND) &&
				   (lev->locality != LOC_NARGOTHROND)) {
			return true;
		} else if (rf_has(race->flags, RF_DUNGORTHEB) &&
				   (lev->locality != LOC_NAN_DUNGORTHEB)) {
			return true;
		} else if (rf_has(race->flags, RF_GAURHOTH) &&
				   (lev->locality != LOC_TOL_IN_GAURHOTH)) {
			return true;
		}
	}

	/* Dungeon-only monsters */
	if (rf_has(race->flags, RF_DUNGEON)	&& (lev->topography != TOP_CAVE)) {
		return true;
	}

	/* Flying monsters for mountaintop */
	if (!rf_has(race->flags, RF_FLYING) &&
		(lev->topography == TOP_MOUNTAINTOP)) {
		return true;
	}

	return false;
}

/**
 * Helper function for get_mon_num().  Adjust the probabilities of monsters
 * based on the locality and topography into which they are vying to spawn
 *
 * Ideally topography and locality and their effects on monsters would be
 * read from datafiles
 */
static int get_mon_adjust(int prob, struct monster_race *race)
{
	struct level *lev = &world->levels[player->place];

	/* Locality adjustments */
	if (lev->locality == LOC_NAN_DUNGORTHEB) {
		/* Nan Dungortheb is spiderland, bad for humans and humanoids */
		if (streq(race->base->name, "spider")) {
			prob *= 5;
		} else if (streq(race->base->name, "person") ||
				   streq(race->base->name, "humanoid")) {
			prob /= 3;
		}
	} else if (lev->locality == LOC_TOL_IN_GAURHOTH) {
		/* Tol-In-Gaurhoth is full of wolves and undead */
		if (streq(race->base->name, "wolf")) {
			prob *= 4;
		} else if (rf_has(race->flags, RF_UNDEAD)) {
			prob *= 2;
		}
	}

	/* Topography adjustments */
	if ((lev->topography == TOP_DESERT)	|| (lev->topography == TOP_MOUNTAIN)) {
		/* Some animals love desert and mountains, most don't */
		if (streq(race->base->name, "reptile") ||
			streq(race->base->name, "snake") ||
			streq(race->base->name, "centipede")) {
			prob *= 2;
		} else if (rf_has(race->flags, RF_ANIMAL)) {
			prob /= 2;
		}
	} else if (lev->topography == TOP_FOREST) {
		/* Most animals do like forest */
		if (streq(race->base->name, "reptile")) {
			prob /= 2;
		} else if (rf_has(race->flags, RF_ANIMAL) &&
				   !streq(race->base->name, "zephyr hound")) {
			prob *= 2;
		}
	}

	return prob;
}

/**
 * Helper function for get_mon_num(). Scans the prepared monster allocation
 * table and picks a random monster. Returns the index of a monster in
 * `table`.
 */
static struct monster_race *get_mon_race_aux(long total,
											 const alloc_entry *table)
{
	int i;

	/* Pick a monster */
	long value = randint0(total);

	/* Find the monster */
	for (i = 0; i < alloc_race_size; i++) {
		/* Found the entry */
		if (value < table[i].prob3) break;

		/* Decrement */
		value -= table[i].prob3;
	}

	return &r_info[table[i].index];
}

/**
 * Chooses a monster race that seems appropriate to the given level
 *
 * \param generated_level is the level to use when choosing the race.
 * \param current_level is the level where the monster will be placed - used
 * for checks on an out-of-depth monster.
 *
 * This function uses the "prob2" field of the monster allocation table,
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an appropriate monster, in
 * a relatively efficient manner.
 *
 * Note that town monsters will *only* be created in the town, and
 * "normal" monsters will *never* be created in the town, unless the
 * level is modified, for example, by polymorph or summoning.
 *
 * There is a small chance (1/25) of boosting the given depth by
 * a small amount (up to four levels), except in the town.
 *
 * It is (slightly) more likely to acquire a monster of the given level
 * than one of a lower level.  This is done by choosing several monsters
 * appropriate to the given level and keeping the deepest one.
 *
 * Note that if no monsters are appropriate, then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
struct monster_race *get_mon_num(int generated_level, int current_level)
{
	int i, p;
	long total;
	struct monster_race *race;
	alloc_entry *table = alloc_race_table;

	/* Occasionally produce a nastier monster in the dungeon */
	if (generated_level > 0 && one_in_(z_info->ood_monster_chance))
		generated_level += MIN(generated_level / 4 + 2,
			z_info->ood_monster_amount);

	total = 0L;

	/* Process probabilities */
	for (i = 0; i < alloc_race_size; i++) {
		/* Monsters are sorted by depth */
		if (table[i].level > generated_level) break;

		/* Default */
		table[i].prob3 = 0;

		/* No town monsters in dungeon */
		if (generated_level > 0 && table[i].level <= 0) continue;

		/* Get the chosen monster */
		race = &r_info[table[i].index];

		/* Some monsters will not be allowed on the current level */
		if (get_mon_forbidden(race)) continue;

		/* Accept */
		table[i].prob3 = table[i].prob2;

		/* Adjust for locality and topography */
		table[i].prob3 = get_mon_adjust(table[i].prob3, race);

		/* Total */
		total += table[i].prob3;
	}

	/* No legal monsters */
	if (total <= 0) return NULL;

	/* Pick a monster */
	race = get_mon_race_aux(total, table);

	/* Try for a "harder" monster once (50%) or twice (10%) */
	p = randint0(100);

	if (p < 60) {
		struct monster_race *old = race;

		/* Pick a new monster */
		race = get_mon_race_aux(total, table);

		/* Keep the deepest one */
		if (race->level < old->level) race = old;
	}

	/* Try for a "harder" monster twice (10%) */
	if (p < 10) {
		struct monster_race *old = race;

		/* Pick a monster */
		race = get_mon_race_aux(total, table);

		/* Keep the deepest one */
		if (race->level < old->level) race = old;
	}

	/* Result */
	return race;
}

/**
 * ------------------------------------------------------------------------
 * Handling of 'player race' monsters
 * ------------------------------------------------------------------------ */
/**
 * Initialize the racial probability array
 */
void init_race_probs(void)
{
	int i, j, k, n;
	int **adjacency, **lev_path, **temp_path;

	/* Make the array */
	race_prob = mem_zalloc(z_info->p_race_max * sizeof(int*));
	for (i = 0; i < z_info->p_race_max; i++) {
		race_prob[i] = mem_zalloc(world->num_levels * sizeof(int));
	}

	/* Prepare temporary adjacency arrays */
	adjacency = mem_zalloc(world->num_levels * sizeof(int*));
	lev_path = mem_zalloc(world->num_levels * sizeof(int*));
	temp_path = mem_zalloc(world->num_levels * sizeof(int*));
	for (i = 0; i < world->num_levels; i++) {
		adjacency[i] = mem_zalloc(world->num_levels * sizeof(int));
		lev_path[i] = mem_zalloc(world->num_levels * sizeof(int));
		temp_path[i] = mem_zalloc(world->num_levels * sizeof(int));
	}

	/* Make the adjacency matrix */
	for (i = 0; i < world->num_levels; i++) {
		/* Get the horizontally adjacent levels */
		struct level *lev = &world->levels[i];
		struct level *north = NULL;
		struct level *east = NULL;
		struct level *south = NULL;
		struct level *west = NULL;
		if (lev->north) north = level_by_name(world, lev->north);
		if (lev->east) east = level_by_name(world, lev->east);
		if (lev->south) south = level_by_name(world, lev->south);
		if (lev->west) west = level_by_name(world, lev->west);

		/* Initialise this row */
		for (k = 0; k < world->num_levels; k++) {
			adjacency[i][k] = 0;
			lev_path[i][k] = 0;
			temp_path[i][k] = 0;
		}

		/* Add 1s where there's an adjacent stage (not up or down) */
		if (north) {
			adjacency[i][north->index] = 1;
			temp_path[i][north->index] = 1;
		}
		if (east) {
			adjacency[i][east->index] = 1;
			temp_path[i][east->index] = 1;
		}
		if (south) {
			adjacency[i][south->index] = 1;
			temp_path[i][south->index] = 1;
		}
		if (west) {
			adjacency[i][west->index] = 1;
			temp_path[i][west->index] = 1;
		}
	}

	/* Power it up (squaring 3 times gives eighth power) */
	for (n = 0; n < 3; n++) {
		/* Square */
		for (i = 0; i < world->num_levels; i++) {
			for (j = 0; j < world->num_levels; j++) {
				lev_path[i][j] = 0;
				for (k = 0; k < world->num_levels; k++) {
					lev_path[i][j] += temp_path[i][k] * temp_path[k][j];
				}
			}
		}

		/* Copy it over for the next squaring or final multiply */
		for (i = 0; i < world->num_levels; i++) {
			for (j = 0; j < world->num_levels; j++) {
				temp_path[i][j] = lev_path[i][j];
			}
		}
	}

	/* Get the max of length 8 and length 9 paths */
	for (i = 0; i < world->num_levels; i++) {
		for (j = 0; j < world->num_levels; j++) {
			/* Multiply to get the length 9s */
			lev_path[i][j] = 0;
			for (k = 0; k < world->num_levels; k++) {
				lev_path[i][j] += temp_path[i][k] * adjacency[k][j];
			}

			/* Now replace by the length 8s if it's larger */
			if (lev_path[i][j] < temp_path[i][j]) {
				lev_path[i][j] = temp_path[i][j];
			}
		}
	}

	/* We now have the maximum of the number of paths of length 8 and the
	 * number of paths of length 9 (we need to try odd and even length paths,
	 * as using just one leads to anomalies) from any level to any other,
	 * which we will use as a basis for the racial probability table for
	 * racially based monsters in any given level.  For a level, we give
	 * every race a 1, then add the number of paths of length 8 from their
	 * hometown to that level.  We then turn every row entry into the
	 * cumulative total of the row to that point.  Whenever a racially based
	 * monster is called for, we will take a random integer less than the
	 * last entry of the row for that level, and proceed along the row,
	 * allocating the race corresponding to the position where we first
	 * exceed that integer.
	 */
	for (i = 0; i < world->num_levels; i++) {
		int prob = 0;
		struct player_race *p_race = races;
		while (p_race) {
			int hometown = 	strstr(world->name, "Dungeon") ? 1 :
				level_by_name(world, p_race->hometown)->index;
			/* Enter the cumulative probability */
			prob += 1 + lev_path[world->levels[hometown].index][i];
			race_prob[p_race->ridx][i] = prob;
			p_race = p_race->next;
		}
	}

	/* Free the temporary arrays */
	for (i = 0; i < world->num_levels; i++) {
		mem_free(temp_path[i]);
		mem_free(adjacency[i]);
		mem_free(lev_path[i]);
	}
	mem_free(temp_path);
	mem_free(adjacency);
	mem_free(lev_path);
}

void free_race_probs(void)
{
	int i;
	for (i = 0; i < z_info->p_race_max; i++) {
		mem_free(race_prob[i]);
	}
	mem_free(race_prob);
}

/**
 * ------------------------------------------------------------------------
 * Player ghost code
 * ------------------------------------------------------------------------ */
#define PLAYER_GHOST_RACE z_info->r_max - 1
#define GHOST_NAME_LENGTH 15

/**
 * Adjust various player ghost attributes depending on race and class.
 */
static void process_ghost_race_class(struct player_race *p_race,
									 struct player_class *class,
									 struct monster_race *race)
{
	struct ghost *g;
	int hurt = 0;
	for (g = ghosts; g; g = g->next) {
		struct ghost_level *lev;
		if (strcmp(p_race->name, g->name) && strcmp(class->name, g->name)) {
			continue;
		}

		/* Iterate through properties that kick in at various dungeon levels */
		for (lev = g->level; lev; lev = lev->next) {
			/* Only handle deep enough properties */
			if (race->level < lev->level) break;

			/* Spell power */
			if (lev->spell_power.param1) {
				race->spell_power *= lev->spell_power.param1;
				race->spell_power /= lev->spell_power.param2;
			}

			/* Hearing */
			race->hearing += lev->hearing;

			/* Hit points */
			if (lev->avg_hp.param1) {
				race->avg_hp *= lev->avg_hp.param1;
				race->avg_hp /= lev->avg_hp.param2;
			}

			/* Armor class */
			if (lev->ac.param1) {
				race->ac *= lev->ac.param1;
				race->ac /= lev->ac.param2;
			}

			/* Increase in blow sides */
			if (lev->blow_sides.param1) {
				int i;
				for (i = 0; i < z_info->mon_blows_max; i++) {
					if (race->blow[i].method) {
						race->blow[i].dice.sides *= lev->blow_sides.param1;
						race->blow[i].dice.sides /= lev->blow_sides.param2;
					}
				}
			}

			/* Speed */
			if (lev->speed.param1) {
				race->speed += lev->speed.param1;
				race->speed = MIN(race->speed, lev->speed.param2);
			}

			/* New blow effects */
			if (lev->blow_effect1) {
				int i;
				for (i = 0; i < z_info->mon_blows_max && (hurt == 0); i++) {
					if (!race->blow[i].effect) break;
					if (strcmp(race->blow[i].effect->name, "HURT") == 0) {
						race->blow[i].effect =
							lookup_monster_blow_effect(lev->blow_effect1);
						hurt++;
						break;
					}
				}
			}
			if (lev->blow_effect2) {
				int i;
				for (i = 0; i < z_info->mon_blows_max && (hurt == 1); i++) {
					if (!race->blow[i].effect) break;
					if (strcmp(race->blow[i].effect->name, "HURT") == 0) {
						race->blow[i].effect =
							lookup_monster_blow_effect(lev->blow_effect1);
						hurt++;
						break;
					}
				}
			}

			/* Race flags, add and remove */
			rf_union(race->flags, lev->flags);
			rf_diff(race->flags, lev->flags_off);

			/* Spells, add and remove */
			rf_union(race->spell_flags, lev->spell_flags);
			rf_diff(race->spell_flags, lev->spell_flags_off);
		}

		/* Spell frequency */
		if (g->freq_if_zero) {
			bitflag innate_spells[RSF_SIZE], non_innate_spells[RSF_SIZE];

			/* Check for innate spells */
			create_mon_spell_mask(innate_spells, RST_INNATE, RST_NONE);
			rsf_inter(innate_spells, race->spell_flags);
			if (!rsf_is_empty(innate_spells) && !race->freq_innate) {
				race->freq_innate = g->freq_if_zero;
			}

			/* Check for regular spells */
			rsf_copy(non_innate_spells, race->spell_flags);
			rsf_diff(non_innate_spells, innate_spells);
			if (!rsf_is_empty(non_innate_spells) && !race->freq_spell) {
				race->freq_spell = g->freq_if_zero;
			}
		}
		if (g->freq_if_positive) {
			bitflag innate_spells[RSF_SIZE], non_innate_spells[RSF_SIZE];

			/* Check for innate spells */
			create_mon_spell_mask(innate_spells, RST_INNATE, RST_NONE);
			rsf_inter(innate_spells, race->spell_flags);
			if (race->freq_innate) {
				if (!rsf_is_empty(innate_spells)) {
					race->freq_innate += g->freq_if_positive;
				} else {
					race->freq_innate = 0;
				}
			}

			/* Check for regular spells */
			rsf_copy(non_innate_spells, race->spell_flags);
			rsf_diff(non_innate_spells, innate_spells);
			if (race->freq_spell) {
				if (!rsf_is_empty(non_innate_spells)) {
					race->freq_spell += g->freq_if_positive;
				} else {
					race->freq_spell = 0;
				}
			}
		}
	}
}

/**
 * Once a monster with the flag "PLAYER_GHOST" is generated, it needs
 * to have a little color added, if it hasn't been prepared before.
 * This function uses a bones file to get a name and adds flags depending on
 * the race and class of the slain adventurer.  -LM-
 */
bool prepare_ghost(struct chunk *c, int r_idx, struct monster *mon,
				   bool from_savefile)
{
	int ghost_race, ghost_class = 0;
	byte try, i;

	struct monster_race *race = &r_info[r_idx];
	struct monster_lore *lore = get_lore(race);
	struct player_race *p_race;
	struct player_class *class;

	ang_file *fp = NULL;
	char path[1024];
	char buf[80];

	/* Paranoia. */
	assert(c->ghost);
	if (!rf_has(race->flags, RF_PLAYER_GHOST))
		return false;

	/* Hack -- No easy player ghosts, unless the ghost is from a savefile.
	 * This also makes player ghosts much rarer, and effectively (almost)
	 * prevents more than one from being on a level. From 0.5.1, other code
	 * makes it is formally impossible to have more than one ghost at a time.
	 * -BR- */
	if ((race->level < c->depth - 5) && (from_savefile == false))
		return false;

	/* Store the index of the base race. */
	c->ghost->race = r_idx;

	/* Copy the info from the template to the special "ghost slot", and use
	 * that from here on */
	memcpy(&r_info[PLAYER_GHOST_RACE], race, sizeof(*race));
	race = &r_info[PLAYER_GHOST_RACE];
	mon->race = race;

	/* Set rarity to 0 so the ghost can't rise again */
	race->rarity = 0;

	/* Choose a bones file.  Use the variable bones_selector if it has any
	 * information in it (this allows saved ghosts to reacquire all special
	 * features), then use the current depth, and finally pick at random.
	 *
	 * Note the hackery with bitflags to choose between a genuine player
	 * ghost and a preloaded one, and the chance that a ghost will be
	 * different on reloading on a multiplayer server if someone else has
	 * killed the original one in the mean time. */
	for (try = 0; try < 40; ++try) {
		/* Prepare a path, and store the file number for future reference. */
		if (try == 0) {
			if (!c->ghost->bones_selector) {
				/* Try for an actual ghost of the player's depth first */
				c->ghost->bones_selector = player->depth;
				path_build(path, sizeof(path), ANGBAND_DIR_BONE,
						   format("bone.%03d", c->ghost->bones_selector));
			} else {
				/* Loading a preloaded ghost from savefile */
				if (c->ghost->bones_selector & 0x80) {
					c->ghost->bones_selector &= 0x7F;
					path_build(path, sizeof(path), ANGBAND_DIR_GHOST,
							   format("bone.%03d", c->ghost->bones_selector));
					c->ghost->bones_selector |= 0x80;
				} else {
					/* Loading an actual ghost from savefile */
					path_build(path, sizeof(path), ANGBAND_DIR_BONE,
							   format("bone.%03d", c->ghost->bones_selector));
				}
			}
		} else {
			/* Just pick a random depth if we can't get the player's */
			c->ghost->bones_selector = randint1(z_info->max_depth - 1);

			/* After the first attempt, randomly try preloaded ghosts */
			if (!one_in_(3)) {
				path_build(path, sizeof(path), ANGBAND_DIR_BONE,
						   format("bone.%03d", c->ghost->bones_selector));
			} else {
				path_build(path, sizeof(path), ANGBAND_DIR_GHOST,
						   format("bone.%03d", c->ghost->bones_selector));
				c->ghost->bones_selector |= 0x80;
			}
		}

		/* Check we actually have a path */
		if (strlen(path) == 0) continue;

		/* Attempt to open the bones file. */
		fp = file_open(path, MODE_READ, FTYPE_TEXT);

		/* No bones file with that number, try again. */
		if (!fp) {
			c->ghost->bones_selector = 0;
			continue;
		}

		/* Success. */
		if (fp) break;
	}

	/* No bones file found, so no Ghost is made. */
	if (!fp) return false;

	/* Scan the file */

	/* Name */
	if (!file_getl(fp, buf, sizeof(buf))) return false;
	my_strcpy(c->ghost->name, buf, GHOST_NAME_LENGTH);

	/* Race */
	if (!file_getl(fp, buf, sizeof(buf))) return false;
	if (1 != sscanf(buf, "%d", &ghost_race)) return false;

	/* Class */
	if (!file_getl(fp, buf, sizeof(buf))) return false;
	if (1 != sscanf(buf, "%d", &ghost_class)) return false;

	/* String type (maybe) */
	if (file_getl(fp, buf, sizeof(buf))) {
		if (1 != sscanf(buf, "%d", &c->ghost->string_type))
			c->ghost->string_type = 0;
	}

	/* String (maybe) */
	if (strlen(buf) > 2) {
		my_strcpy(c->ghost->string, buf + 2, strlen(buf));
	} else {
		c->ghost->string_type = 0;
	}

	/* Close the file */
	file_close(fp);

	/* Process the ghost name and store it. */
	for (i = 0; (i < 16) && (c->ghost->name[i]) &&
			 (c->ghost->name[i] != ','); i++);

	/* Terminate the name */
	c->ghost->name[i] = '\0';

	/* Force a name */
	if (!c->ghost->name[0]) {
		my_strcpy(c->ghost->name, "Nobody", strlen(c->ghost->name));
	}

	/* Capitalize the name */
	if (islower(c->ghost->name[0])) {
		c->ghost->name[0] = toupper(c->ghost->name[0]);
	}

	/* Process race and class. */
	p_race = player_id2race(ghost_race);
	class = player_id2class(ghost_class);
	process_ghost_race_class(p_race, class, race);

	/* A little extra help for the deepest ghosts */
	if (c->depth > 75) {
		race->spell_power += 3 * (c->depth - 75) / 2;
	}

	/* Increase the level feeling */
	c->mon_rating += 10;

	/* Player ghosts are "seen" whenever generated. */
	lore->sights = 1;

	/* Success */
	return true;
}


/**
 * ------------------------------------------------------------------------
 * Deleting of monsters and monster list handling
 * ------------------------------------------------------------------------ */
/**
 * Deletes a monster by index.
 *
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(int m_idx)
{
	struct monster *mon = cave_monster(cave, m_idx);
	struct loc grid;

	assert(m_idx > 0);
	assert(square_in_bounds(cave, mon->grid));
	grid = mon->grid;

	/* Hack -- Reduce the racial counter */
	if (mon->original_race) mon->original_race->cur_num--;
	else mon->race->cur_num--;

	/* Count the number of "reproducers" */
	if (rf_has(mon->race->flags, RF_MULTIPLY)) {
		cave->num_repro--;
	}

	/* Affect light? */
	if (mon->race->light != 0)
		player->upkeep->update |= PU_UPDATE_VIEW | PU_MONSTERS;

	/* Hack -- remove target monster */
	if (target_get_monster() == mon)
		target_set_monster(NULL);

	/* Hack -- remove tracked monster */
	if (player->upkeep->health_who == mon)
		health_track(player->upkeep, NULL);

	/* Hack -- remove any command status */
	if (mon->m_timed[MON_TMD_COMMAND]) {
		(void) player_clear_timed(player, TMD_COMMAND, true);
	}

	/* Monster is gone from square and group, and no longer targeted */
	square_set_mon(cave, grid, 0);
	monster_remove_from_groups(cave, mon);
	monster_remove_from_targets(cave, mon);

	/* Free any heatmaps */
	if (mon->noise.grids) {
		heatmap_free(cave, mon->noise);
	}
	if (mon->scent.grids) {
		heatmap_free(cave, mon->scent);
	}

	/* Delete objects */
	struct object *obj = mon->held_obj;
	while (obj) {
		struct object *next = obj->next;

		/* Preserve unseen artifacts (we assume they were created as this
		 * monster's drop) - this will cause unintended behaviour in preserve
		 * off mode if monsters can pick up artifacts */
		if (obj->artifact && !obj_is_known_artifact(obj)) {
			mark_artifact_created(obj->artifact, false);
		}

		/* Delete the object.  Since it's in the cave's list do
		 * some additional bookkeeping. */
		if (obj->known) {
			/* It's not in a floor pile so remove it completely.
			 * Once compatibility with old savefiles isn't needed
			 * can skip the test and simply delist and delete
			 * since any obj->known from a monster's inventory
			 * will not be in a floor pile. */
			if (loc_is_zero(obj->known->grid)) {
				delist_object(player->cave, obj->known);
				object_delete(player->cave, NULL, &obj->known);
			}
		}
		delist_object(cave, obj);
		object_delete(cave, player->cave, &obj);
		obj = next;
	}

	/* Delete mimicked objects */
	if (mon->mimicked_obj) {
		square_delete_object(cave, mon->grid, mon->mimicked_obj, true, false);
	}

	/* If the monster was a player ghost, remove it from the monster memory,
	 * ensure that it never appears again, clear its data and allow the
	 * next ghost to speak. */
	if (rf_has(mon->race->flags, RF_PLAYER_GHOST)) {
		struct monster_lore *lore = get_lore(mon->race);
		lore->sights = 0;
		lore->deaths = 0;
		lore->pkills = 0;
		lore->tkills = 0;
		if (player->upkeep->monster_race == mon->race) {
			player->upkeep->monster_race = NULL;
		}
		cave->ghost->bones_selector = 0;
		cave->ghost->has_spoken = false;
		cave->ghost->string_type = 0;
		my_strcpy(cave->ghost->string, "", sizeof(cave->ghost->string));
	}

	/* Wipe the Monster */
	memset(mon, 0, sizeof(struct monster));

	/* Count monsters */
	cave->mon_cnt--;

	/* Visual update */
	square_light_spot(cave, grid);
}


/**
 * Deletes the monster, if any, at the given location.
 */
void delete_monster(struct loc grid)
{
	assert(square_in_bounds(cave, grid));

	/* Delete the monster (if any) */
	if (square(cave, grid)->mon > 0)
		delete_monster_idx(square(cave, grid)->mon);
}


/**
 * Move a monster from index i1 to index i2 in the monster list.
 *
 * This should only be called when there is an actual monster at i1
 */
void monster_index_move(int i1, int i2)
{
	struct monster *mon;
	struct object *obj;

	/* Do nothing */
	if (i1 == i2) return;

	/* Old monster */
	mon = cave_monster(cave, i1);
	if (!mon) return;

	/* Update the cave */
	square_set_mon(cave, mon->grid, i2);

	/* Update midx */
	mon->midx = i2;

	/* Update group */
	if (!monster_group_change_index(cave, i2, i1)) {
		quit("Bad monster group info!") ;
		monster_groups_verify(cave);
	}

	/* Repair objects being carried by monster */
	for (obj = mon->held_obj; obj; obj = obj->next)
		obj->held_m_idx = i2;

	/* Move mimicked objects (heh) */
	if (mon->mimicked_obj)
		mon->mimicked_obj->mimicking_m_idx = i2;

	/* Update the target */
	if (target_get_monster() == mon)
		target_set_monster(cave_monster(cave, i2));

	/* Update the health bar */
	if (player->upkeep->health_who == mon)
		player->upkeep->health_who = cave_monster(cave, i2);

	/* Move monster */
	memcpy(cave_monster(cave, i2),
			cave_monster(cave, i1),
			sizeof(struct monster));

	/* Wipe hole */
	memset(cave_monster(cave, i1), 0, sizeof(struct monster));
}


/**
 * Compacts and reorders the monster list.
 *
 * This function can be very dangerous, use with caution!
 *
 * When `num_to_compact` is 0, we just reorder the monsters into a more compact
 * order, eliminating any "holes" left by dead monsters. If `num_to_compact` is
 * positive, then we delete at least that many monsters and then reorder.
 * We try not to delete monsters that are high level or close to the player.
 * Each time we make a full pass through the monster list, if we haven't
 * deleted enough monsters, we relax our bounds a little to accept
 * monsters of a slightly higher level, and monsters slightly closer to
 * the player.
 */
void compact_monsters(struct chunk *c, int num_to_compact)
{
	int m_idx, num_compacted, iter;

	int max_lev, min_dis, chance;


	/* Message (only if compacting) */
	if (num_to_compact)
		msg("Compacting monsters...");


	/* Compact at least 'num_to_compact' objects */
	for (num_compacted = 0, iter = 1; num_compacted < num_to_compact; iter++) {
		/* Get more vicious each iteration */
		max_lev = 5 * iter;

		/* Get closer each iteration */
		min_dis = 5 * (20 - iter);

		/* Check all the monsters */
		for (m_idx = 1; m_idx < cave_monster_max(c); m_idx++) {
			struct monster *mon = cave_monster(c, m_idx);

			/* Skip "dead" monsters */
			if (!mon->race) continue;

			/* High level monsters start out "immune" */
			if (mon->race->level > max_lev) continue;

			/* Ignore nearby monsters */
			if ((min_dis > 0) && (mon->cdis < min_dis)) continue;

			/* Saving throw chance */
			chance = 90;

			/* Only compact "Quest" Monsters in emergencies */
			if (quest_unique_monster_check(mon->race) && (iter < 1000))
				chance = 100;

			/* Try not to compact Unique Monsters */
			if (rf_has(mon->race->flags, RF_UNIQUE)) chance = 99;

			/* All monsters get a saving throw */
			if (randint0(100) < chance) continue;

			/* Delete the monster */
			delete_monster(mon->grid);

			/* Count the monster */
			num_compacted++;
		}
	}


	/* Excise dead monsters (backwards!) */
	for (m_idx = cave_monster_max(c) - 1; m_idx >= 1; m_idx--) {
		struct monster *mon = cave_monster(c, m_idx);

		/* Skip real monsters */
		if (mon->race) continue;

		/* Move last monster into open hole */
		monster_index_move(cave_monster_max(c) - 1, m_idx);

		/* Compress "c->mon_max" */
		c->mon_max--;
	}
}


/**
 * Deletes all the monsters when the player leaves the level.
 *
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 *
 * Note that we must delete the objects the monsters are carrying, but we
 * do nothing with mimicked objects.
 */
void wipe_mon_list(struct chunk *c, struct player *p)
{
	int m_idx, i;

	/* Delete all the monsters */
	for (m_idx = cave_monster_max(c) - 1; m_idx >= 1; m_idx--) {
		struct monster *mon = cave_monster(c, m_idx);
		struct object *held_obj = mon ? mon->held_obj : NULL;

		/* Skip dead monsters */
		if (!mon->race) continue;

		/* Delete all the objects */
		if (held_obj) {
			/* Go through all held objects and check for artifacts */
			struct object *obj = held_obj;
			while (obj) {
				if (obj->artifact && !obj_is_known_artifact(obj)) {
					mark_artifact_created(obj->artifact,
						false);
				}
				/*
				 * Also, remove from the cave's object list.
				 * That way, the scan for orphaned objects
				 * in cave_free() doesn't attempt to
				 * access freed memory or free memory
				 * twice.
				 */
				if (obj->oidx) {
					c->objects[obj->oidx] = NULL;
				}
				obj = obj->next;
			}
			object_pile_free(c, (p && c == cave) ? p->cave : NULL,
				held_obj);
		}

		/* Reduce the racial counter */
		if (mon->original_race) mon->original_race->cur_num--;
		else mon->race->cur_num--;

		/* Monster is gone from square */
		square_set_mon(c, mon->grid, 0);

		/* If the monster was a player ghost, remove it from the monster memory,
		 * ensure that it never appears again, clear its data and allow the
		 * next ghost to speak. */
		if (rf_has(mon->race->flags, RF_PLAYER_GHOST)) {
			struct monster_lore *lore = get_lore(mon->race);
			lore->sights = 0;
			lore->deaths = 0;
			lore->pkills = 0;
			lore->tkills = 0;
			cave->ghost->bones_selector = 0;
			cave->ghost->has_spoken = false;
			my_strcpy(cave->ghost->name, "", sizeof(cave->ghost->name));
			my_strcpy(cave->ghost->string, "", sizeof(cave->ghost->string));
			cave->ghost->string_type = 0;
			if (player->upkeep->monster_race == mon->race) {
				player->upkeep->monster_race = NULL;
			}
		}

		/* Wipe the Monster */
		memset(mon, 0, sizeof(struct monster));
	}

	/* Delete the player ghost record completely */
	memset(&r_info[PLAYER_GHOST_RACE], 0, sizeof(struct monster_race));

	/* Delete all the monster groups */
	for (i = 1; i < z_info->level_monster_max; i++) {
		if (c->monster_groups[i]) {
			monster_group_free(c, c->monster_groups[i]);
		}
	}

	/* Reset "cave->mon_max" */
	c->mon_max = 1;

	/* Reset "mon_cnt" */
	c->mon_cnt = 0;

	/* Reset "reproducer" count */
	c->num_repro = 0;

	/* Hack -- no more target */
	target_set_monster(0);

	/* Hack -- no more tracking */
	health_track(p->upkeep, 0);
}

/**
 * ------------------------------------------------------------------------
 * Monster creation utilities:
 *  Getting a new monster index
 *  Creating objects for monsters to carry or mimic
 *  Calculating hitpoints
 * ------------------------------------------------------------------------ */
/**
 * Returns the index of a "free" monster, or 0 if no slot is available.
 *
 * This routine should almost never fail, but it *can* happen.
 * The calling code must check for and handle a 0 return.
 */
s16b mon_pop(struct chunk *c)
{
	int m_idx;

	/* Normal allocation */
	if (cave_monster_max(c) < z_info->level_monster_max) {
		/* Get the next hole */
		m_idx = cave_monster_max(c);

		/* Expand the array */
		c->mon_max++;

		/* Count monsters */
		c->mon_cnt++;

		return m_idx;
	}

	/* Recycle dead monsters if we've run out of room */
	for (m_idx = 1; m_idx < cave_monster_max(c); m_idx++) {
		struct monster *mon = cave_monster(c, m_idx);

		/* Skip live monsters */
		if (!mon->race) {
			/* Count monsters */
			c->mon_cnt++;

			/* Use this monster */
			return m_idx;
		}
	}

	/* Warn the player if no index is available */
	if (character_dungeon)
		msg("Too many monsters!");

	/* Try not to crash */
	return 0;
}


/**
 * Return the number of things dropped by a monster.
 *
 * \param race is the monster race.
 * \param maximize should be set to false for a random number, true to find
 * out the maximum count.
 * \param specific if true, specific drops will be included in the total
 * returned; otherwise they are excluded from that value.
 * \param specific_count if not NULL, *specific_count will be set to the
 * number of specific objects (either a random value or the maximum if maximize
 * is true).
 */
int mon_create_drop_count(const struct monster_race *race, bool maximize,
	bool specific, int *specific_count)
{
	int number = 0;
	int specnum = 0;
	static const int drop_4_max = 6;
	static const int drop_3_max = 4;
	static const int drop_2_max = 3;
	struct monster_drop *drop;

	if (maximize) {
		if (rf_has(race->flags, RF_DROP_20)) number++;
		if (rf_has(race->flags, RF_DROP_40)) number++;
		if (rf_has(race->flags, RF_DROP_60)) number++;
		if (rf_has(race->flags, RF_DROP_4)) number += drop_4_max;
		if (rf_has(race->flags, RF_DROP_3)) number += drop_3_max;
		if (rf_has(race->flags, RF_DROP_2)) number += drop_2_max;
		if (rf_has(race->flags, RF_DROP_1)) number++;
		for (drop = race->drops; drop; drop = drop->next) {
			specnum += drop->max;
		}
	} else {
		if (rf_has(race->flags, RF_DROP_20) && randint0(100) < 20) number++;
		if (rf_has(race->flags, RF_DROP_40) && randint0(100) < 40) number++;
		if (rf_has(race->flags, RF_DROP_60) && randint0(100) < 60) number++;
		if (rf_has(race->flags, RF_DROP_4)) number += rand_range(2, drop_4_max);
		if (rf_has(race->flags, RF_DROP_3)) number += rand_range(2, drop_3_max);
		if (rf_has(race->flags, RF_DROP_2)) number += rand_range(1, drop_2_max);
		if (rf_has(race->flags, RF_DROP_1)) number++;
		for (drop = race->drops; drop; drop = drop->next) {
			if ((unsigned int)randint0(100) <
					drop->percent_chance) {
				specnum += randint0(drop->max - drop->min) +
					drop->min;
			}
		}
	}
	if (specific) {
		number += specnum;
	}
	if (specific_count) {
		*specific_count = specnum;
	}
	return number;
}

/**
 * Get a correct origin race - needed for player ghosts
 */
static struct monster_race *get_origin_race(struct monster_race *race)
{
	return rf_has(race->flags, RF_PLAYER_GHOST) ? &r_info[race->ridx] : race;
}

/**
 * Creates a specific monster's drop, including any drops specified
 * in the monster.txt file.
 *
 * Returns true if anything is created, false if nothing is.
 */
static bool mon_create_drop(struct chunk *c, struct monster *mon, byte origin)
{
	struct monster_drop *drop;
	struct monster_lore *lore = get_lore(mon->race);

	bool great, good, gold_ok, item_ok;
	bool extra_roll = false;
	bool any = false;

	int number = 0, level, j, monlevel;

	struct object *obj;

	assert(mon);

	great = (rf_has(mon->race->flags, RF_DROP_GREAT));
	good = great || (rf_has(mon->race->flags, RF_DROP_GOOD));
	gold_ok = (!rf_has(mon->race->flags, RF_ONLY_ITEM));
	item_ok = (!rf_has(mon->race->flags, RF_ONLY_GOLD));

	/* Determine how much we can drop */
	number = mon_create_drop_count(mon->race, false, false, NULL);

	/* Uniques that have been stolen from get their quantity reduced */
	if (rf_has(mon->race->flags, RF_UNIQUE)) {
		number = MAX(0, number - lore->thefts);
	}

	/* Give added bonus for unique monsters */
	monlevel = mon->race->level;
	if (rf_has(mon->race->flags, RF_UNIQUE)) {
		monlevel = MIN(monlevel + 15, monlevel * 2);
		extra_roll = true;
	}

	/* Take the best of (average of monster level and current depth)
	   and (monster level) - to reward fighting OOD monsters */
	level = MAX((monlevel + c->depth) / 2, monlevel);
	level = MIN(level, 100);

	/* Check for quest artifacts */
	if (quest_unique_monster_check(mon->race)) {
		struct quest *quest = find_quest(player->place);
		if (quest && quest->arts) {
			struct quest_artifact *arts = quest->arts;
			while (arts) {
				struct artifact *art = &a_info[arts->index];
				struct object_kind *kind = lookup_kind(art->tval, art->sval);

				/* Check chance */
				if (randint1(100) > arts->chance) {
					arts = arts->next;
					continue;
				}

				/* Allocate by hand, prep, apply magic */
				obj = mem_zalloc(sizeof(*obj));
				object_prep(obj, kind, 100, RANDOMISE);
				obj->artifact = art;
				copy_artifact_data(obj, obj->artifact);
				mark_artifact_created(art, true);

				/* Set origin details */
				obj->origin = origin;
				obj->origin_depth = c->depth;
				obj->origin_place = c->place;
				obj->origin_race = get_origin_race(mon->race);
				obj->number = 1;

				/* Try to carry */
				if (monster_carry(c, mon, obj)) {
					any = true;
				} else {
					mark_artifact_created(obj->artifact, false);
					object_wipe(obj);
					mem_free(obj);
				}
				arts = arts->next;
			}
		}
	}

	/* Specified drops */
	for (drop = mon->race->drops; drop; drop = drop->next) {
		if ((unsigned int)randint0(100) >= drop->percent_chance)
			continue;

		/* Specified by tval or by kind */
		if (drop->kind) {
			/* Allocate by hand, prep, apply magic */
			obj = mem_zalloc(sizeof(*obj));
			object_prep(obj, drop->kind, level, RANDOMISE);
			apply_magic(obj, level, true, good, great, extra_roll);
		} else {
			/* Choose by set tval */
			assert(drop->tval);
			obj = make_object(c, level, good, great, extra_roll, NULL,
							  drop->tval);
		}

		/* Skip if the object couldn't be created. */
		if (!obj) continue;

		/* Set origin details */
		obj->origin = origin;
		obj->origin_depth = c->depth;
		obj->origin_place = c->place;
		obj->origin_race = get_origin_race(mon->race);
		obj->number = (obj->artifact) ?
			1 : randint0(drop->max - drop->min) + drop->min;

		/* Try to carry */
		if (monster_carry(c, mon, obj)) {
			any = true;
		} else {
			object_wipe(obj);
			mem_free(obj);
		}
	}

	/* Make some objects */
	for (j = 0; j < number; j++) {
		if (gold_ok && (!item_ok || (randint0(100) < 50))) {
			obj = make_gold(level, "any");
		} else {
			obj = make_object(c, level, good, great, extra_roll, NULL, 0);
			if (!obj) continue;
		}

		/* Set origin details */
		obj->origin = origin;
		obj->origin_depth = c->depth;
		obj->origin_place = c->place;
		obj->origin_race = get_origin_race(mon->race);

		/* Try to carry */
		if (monster_carry(c, mon, obj)) {
			any = true;
		} else {
			if (obj->artifact) {
				mark_artifact_created(obj->artifact, false);
			}
			object_wipe(obj);
			mem_free(obj);
		}
	}

	return any;
}


/**
 * Creates the onbject a mimic is imitating.
 */
void mon_create_mimicked_object(struct chunk *c, struct monster *mon, int index)
{
	struct object *obj;
	struct object_kind *kind = mon->race->mimic_kinds->kind;
	struct monster_mimic *mimic_kind;
	int i = 1;
	bool dummy = true;

	/* Pick a random object kind to mimic */
	for (mimic_kind = mon->race->mimic_kinds;
		 mimic_kind;
		 mimic_kind = mimic_kind->next, i++) {
		if (one_in_(i)) {
			kind = mimic_kind->kind;
		}
	}

	if (tval_is_money_k(kind)) {
		obj = make_gold(c->depth, kind->name);
	} else {
		obj = object_new();
		object_prep(obj, kind, mon->race->level, RANDOMISE);
		apply_magic(obj, mon->race->level, true, false, false, false);
		obj->number = 1;
		obj->origin = ORIGIN_DROP_MIMIC;
		obj->origin_depth = c->depth;
		obj->origin_place = c->place;
	}

	obj->mimicking_m_idx = index;
	mon->mimicked_obj = obj;

	/* Put the object on the floor if it goes, otherwise no mimicry */
	if (floor_carry(c, mon->grid, obj, &dummy)) {
		list_object(c, obj);
	} else {
		/* Clear the mimicry */
		obj->mimicking_m_idx = 0;
		mon->mimicked_obj = NULL;

		/* Give the object to the monster if appropriate */
		if (rf_has(mon->race->flags, RF_MIMIC_INV)) {
			monster_carry(c, mon, obj);
		} else {
			/* Otherwise delete the mimicked object */
			object_delete(c, NULL, &obj);
		}
	}
}

/**
 * Calculates hp for a monster. This function assumes that the Rand_normal
 * function has limits of +/- 4x std_dev. If that changes, this function
 * will become inaccurate.
 *
 * \param race is the race of the monster in question.
 * \param hp_aspect is the hp calc we want (min, max, avg, random).
 */
int mon_hp(const struct monster_race *race, aspect hp_aspect)
{
	int std_dev = (((race->avg_hp * 10) / 8) + 5) / 10;

	if (race->avg_hp > 1) std_dev++;

	switch (hp_aspect) {
		case MINIMISE:
			return (race->avg_hp - (4 * std_dev));
		case MAXIMISE:
		case EXTREMIFY:
			return (race->avg_hp + (4 * std_dev));
		case AVERAGE:
			return race->avg_hp;
		case RANDOMISE:
			return Rand_normal(race->avg_hp, std_dev);
	}

	assert(0 && "Should never reach here");
	return 0;
}

/**
 * Get a player race according to the probabilites in race_prob[][]
 */
struct player_race *get_player_race(void)
{
	struct player_race *p_race = races;
	int i, k;

	/* Special case -  Estolad themed level - Edain or Druedain */
	if (player->themed_level == themed_level_index("Estolad")) {
		/* Pick one of the races from Sphel Brandir */
		bool pick = (one_in_(2) ? true : false);
		while (p_race) {
			if (streq(p_race->hometown, "Ephel Brandir Town")) {
				if (pick) {
					return p_race;
				} else {
					pick = true;
				}
			}
			p_race = p_race->next;
		}
	}

	/* Pick one according to the probablilities */
	k = randint0(race_prob[z_info->p_race_max - 1][player->place]);
	for (i = 0; i < z_info->p_race_max; i++) {
		if (race_prob[i][player->place] > k) break;
	}

	/* Find the actual race */
	for (p_race = races; p_race; p_race = p_race->next) {
		if (i == 0) break;
		i--;
	}
	return p_race;
}

/**
 * Decides whether a player race monster is neutral
 */
static bool mon_is_neutral(struct chunk *c, struct monster *mon,
						   struct loc grid)
{
	struct player_race_list *dislikes = mon->player_race->dislikes;
	int k, chance;

	while (dislikes) {
		if (dislikes->race == player->race) break;
		dislikes = dislikes->next;
	}

	chance = MAX(dislikes->rel - 100, 0);
	k = randint0(chance + 20);
	if ((k > 20) || (level_topography(player->place) == TOP_CAVE) ||
		(player->themed_level == themed_level_index("Warlords")) ||
		square_isvault(c, grid)) {
		return false;
	}

	return true;
}

/**
 * ------------------------------------------------------------------------
 * Placement of a single monster
 * These are the functions that actually put the monster into the world
 * ------------------------------------------------------------------------ */

/**
 * Attempts to place a copy of the given monster at the given position in
 * the dungeon.
 *
 * All of the monster placement routines eventually call this function. This
 * is what actually puts the monster in the dungeon (i.e., it notifies the cave
 * and sets the monster's position). The dungeon loading code also calls this
 * function directly.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.) The dungeon loading code calls this with origin = 0,
 * which prevents the monster's drops from being generated again.
 *
 * Returns the m_idx of the newly copied monster, or 0 if the placement fails.
 */
s16b place_monster(struct chunk *c, struct loc grid, struct monster *mon,
				   byte origin)
{
	s16b m_idx;
	struct monster *new_mon;
	struct monster_group_info *info = mon->group_info;
	bool loading = mon->midx > 0;

	assert(square_in_bounds(c, grid));
	assert(!square_monster(c, grid));

	/* Get a new record, or recycle the old one */
	if (loading) {
		m_idx = mon->midx;
		c->mon_max++;
		c->mon_cnt++;
	} else {
		m_idx = mon_pop(c);
		if (!m_idx) return 0;
	}

	/* Copy the monster */
	new_mon = cave_monster(c, m_idx);
	memcpy(new_mon, mon, sizeof(struct monster));

	/* Set the ID */
	new_mon->midx = m_idx;

	/* Set the location */
	square_set_mon(c, grid, new_mon->midx);
	new_mon->grid = grid;
	assert(square_monster(c, grid) == new_mon);

	/* Assign monster to its monster group, or update its entry */
	monster_group_assign(c, new_mon, info, loading);

	update_mon(player, new_mon, c, true);

	/* Count the number of "reproducers" */
	if (rf_has(new_mon->race->flags, RF_MULTIPLY)) c->num_repro++;

	/* Count racial occurrences */
	if (new_mon->original_race) new_mon->original_race->cur_num++;
	else new_mon->race->cur_num++;

	/* Create the monster's drop, if any */
	if (origin)
		(void)mon_create_drop(c, new_mon, origin);

	/* Make mimics start mimicking */
	if (origin && new_mon->race->mimic_kinds) {
		mon_create_mimicked_object(c, new_mon, m_idx);
	}

	/* Result */
	return m_idx;
}

/**
 * Attempts to place a monster of the given race at the given location.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 *
 * To give the player a sporting chance, some especially dangerous
 * monsters are marked as "FORCE_SLEEP" in monster.txt, which will
 * cause them to be placed with low energy. This helps ensure that
 * if such a monster suddenly appears in line-of-sight (due to a
 * summon, for instance), the player gets a chance to move before
 * they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * This is the only function which may place a monster in the dungeon,
 * except for the savefile loading code, which calls place_monster()
 * directly.
 */
static bool place_new_monster_one(struct chunk *c, struct loc grid,
								  struct monster_race *race, bool sleep,
								  struct monster_group_info group_info,
								  byte origin)
{
	int i;
	struct monster *mon;
	struct monster monster_body;

	assert(square_in_bounds(c, grid));
	assert(race && race->name);

	/* Not where monsters already are */
	if (square_monster(c, grid)) return false;

	/* Not where the player already is */
	if (loc_eq(player->grid, grid)) return false;

	/* No light hating monsters in daytime */
	if (rf_has(race->flags, RF_HURT_LIGHT) && is_daylight()) return false;

	/* Prevent monsters from being placed where they cannot walk, but allow
	 * other feature types */
	if (!square_is_monster_walkable(c, grid)) return false;
	assert(square_ispassable(c, grid));

	/* No creation on glyphs */
	if (square_iswarded(c, grid) || square_isdecoyed(c, grid)) return false;

	/* "unique" monsters must be "unique" */
	if (rf_has(race->flags, RF_UNIQUE) && (race->cur_num >= race->max_num))
		return false;

	/* Only 1 player ghost at a time */
	if (rf_has(race->flags, RF_PLAYER_GHOST) && c->ghost->bones_selector)
		return false;

	/* Depth monsters may NOT be created out of depth */
	if (rf_has(race->flags, RF_FORCE_DEPTH) && c->depth < race->level)
		return false;

	/* Add to level feeling, note uniques for cheaters */
	c->mon_rating += race->level * race->level;

	/* Check out-of-depth-ness */
	if (race->level > c->depth) {
		if (rf_has(race->flags, RF_UNIQUE)) { /* OOD unique */
			if (OPT(player, cheat_hear))
				msg("Deep unique (%s).", race->name);
		} else { /* Normal monsters but OOD */
			if (OPT(player, cheat_hear))
				msg("Deep monster (%s).", race->name);
		}
		/* Boost rating by power per 10 levels OOD */
		c->mon_rating += (race->level - c->depth) * race->level * race->level;
	} else if (rf_has(race->flags, RF_UNIQUE) && OPT(player, cheat_hear)) {
		msg("Unique (%s).", race->name);
	}

	/* Get local monster */
	mon = &monster_body;

	/* Clean out the monster */
	memset(mon, 0, sizeof(struct monster));

	/* If the monster is a player ghost, perform various manipulations 
	 * on it, and forbid ghost creation if something goes wrong. */
	if (rf_has(race->flags, RF_PLAYER_GHOST)) {
		if (!prepare_ghost(c, race->ridx, mon, false)) return false;

		/* Point to the special "ghost slot" */
		race = &r_info[PLAYER_GHOST_RACE];
	}

	/* Save the race */
	mon->race = race;

	/* Enforce sleeping if needed */
	if (sleep) {
		if (race->sleep) {
			int val = race->sleep;
			mon->m_timed[MON_TMD_SLEEP] = ((val * 2) + randint1(val * 10));
		} else if (is_night() &&
				   (level_topography(player->place) != TOP_TOWN)) {
			mon->m_timed[MON_TMD_SLEEP] += 20;
		}
	}

	/* Uniques get a fixed amount of HP */
	if (rf_has(race->flags, RF_UNIQUE))
		mon->maxhp = race->avg_hp;
	else {
		mon->maxhp = mon_hp(race, RANDOMISE);
		mon->maxhp = MAX(mon->maxhp, 1);
	}

	/* And start out fully healthy */
	mon->hp = mon->maxhp;

	/* Extract the monster base speed */
	mon->mspeed = race->speed;

	/* Hack -- small racial variety */
	if (!rf_has(race->flags, RF_UNIQUE)) {
		/* Allow some small variation per monster */
		i = turn_energy(race->speed) / 10;
		if (i) mon->mspeed += rand_spread(0, i);
	}

	/* Give a random starting energy */
	mon->energy = (byte)randint0(50);

	/* Force monster to wait for player */
	if (rf_has(race->flags, RF_FORCE_SLEEP))
		mflag_on(mon->mflag, MFLAG_NICE);

	/* Affect light? */
	if (mon->race->light != 0)
		player->upkeep->update |= PU_UPDATE_VIEW | PU_MONSTERS;

	/* Is this obviously a monster? (Mimics etc. aren't) */
	if (rf_has(race->flags, RF_UNAWARE))
		mflag_on(mon->mflag, MFLAG_CAMOUFLAGE);
	else
		mflag_off(mon->mflag, MFLAG_CAMOUFLAGE);

	/* Set the color if necessary */
	if (rf_has(race->flags, RF_ATTR_RAND))
		mon->attr = randint1(BASIC_COLORS - 1);

	/* Set the group info */
	mon->group_info[PRIMARY_GROUP].index = group_info.index;
	mon->group_info[PRIMARY_GROUP].role = group_info.role;
	mon->group_info[PRIMARY_GROUP].player_race = group_info.player_race;

	/* Set hostility, or possible neutrality for player race monsters */
	if (rf_has(mon->race->flags, RF_PLAYER)) {
		struct player_race *p_race = monster_group_player_race(c, mon);

		/* Give the monster the special index to indicate it needs updating */
		mon->midx = MIDX_FAKE;

		/* Assign the monster to its group */
		monster_group_assign(c, mon, &group_info, false);

		/* Set player race if needed */
		if (p_race) {
			/* Use the existing race */
			mon->player_race = p_race;
		} else {
			/* Pick a race and set the monster and group races to it */
			p_race = get_player_race();
			mon->player_race = p_race;
			set_monster_group_player_race(c, mon, p_race);
		}

		/* Set hostility */
		if (mon_is_neutral(c, mon, grid)) {
			mon->target.midx = 0;
		} else {
			mon->target.midx = -1;
		}
	} else {
		/* Set to target the player */
		mon->target.midx = -1;
	}

	/* Mark territorial monster's home */
	if (rf_has(race->flags, RF_TERRITORIAL)) {
		mon->home = grid;
	}

	/* Place the monster in the dungeon */
	if (!place_monster(c, grid, mon, origin))
		return (false);

	/* Success */
	return (true);
}


/**
 * ------------------------------------------------------------------------
 * More complex monster placement routines
 * ------------------------------------------------------------------------ */
/**
 * Attempts to place a group of monsters of race `r_idx` around
 * the given location. The number of monsters to place is `total`.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 */
static bool place_new_monster_group(struct chunk *c, struct loc grid,
									struct monster_race *race, bool sleep,
									struct monster_group_info group_info,
									int total, byte origin)
{
	int n, i;

	int loc_num;

	/* Locations of the placed monsters */
	struct loc *loc_list = mem_zalloc(sizeof(struct loc) *
									  z_info->monster_group_max);

	/* Sanity and bounds check */
	assert(race);
	total = MIN(total, z_info->monster_group_max);

	/* Start on the monster */
	loc_num = 1;
	loc_list[0] = grid;

	/* Puddle monsters, breadth first, up to total */
	for (n = 0; (n < loc_num) && (loc_num < total); n++) {
		/* Check each direction, up to total */
		for (i = 0; (i < 8) && (loc_num < total); i++) {
			struct loc try = loc_sum(loc_list[n], ddgrid_ddd[i]);

			/* Ignore annoying locations */
			if (!square_in_bounds_fully(c, try)) continue;

			/* Walls and Monsters block flow */
			if (!square_isempty(c, try)) continue;

			/* Attempt to place another monster */
			if (place_new_monster_one(c, try, race, sleep, group_info, origin)){
				/* Add it to the "hack" set */
				loc_list[loc_num] = try;
				loc_num++;
			}
		}
	}

	/* Success */
	mem_free(loc_list);
	return (true);
}

static struct monster_base *place_monster_base = NULL;

/**
 * Predicate function for get_mon_num_prep()
 * Check to see if the monster race has the same base as
 * place_monter_base.
 */
static bool place_monster_base_okay(struct monster_race *race)
{
	assert(place_monster_base);
	assert(race);

	/* Check if it matches */
	if (race->base != place_monster_base) return false;

	/* No uniques */
	if (rf_has(race->flags, RF_UNIQUE)) return false;

	return true;
}

/**
 * Helper function to place monsters that appear as friends or escorts
 */
static bool place_friends(struct chunk *c, struct loc grid, struct monster_race *race,
					struct monster_race *friends_race, int total, bool sleep,
					struct monster_group_info group_info, byte origin)
{
	int extra_chance;

	/* Find the difference between current dungeon depth and monster level */
	int level_difference = c->depth - friends_race->level + 5;

	/* Handle unique monsters */
	bool is_unique = rf_has(friends_race->flags, RF_UNIQUE);

	/* Make sure the unique hasn't been killed already */
	if (is_unique && (friends_race->cur_num >= friends_race->max_num)) {
		return false;
	}

	/* More than 4 levels OoD, no groups allowed */
	if (level_difference <= 0 && !is_unique) {
		return false;
	}

	/* Reduce group size within 5 levels of natural depth*/
	if (level_difference < 10 && !is_unique) {
		extra_chance = (total * level_difference) % 10;
		total = total * level_difference / 10;

		/* Instead of flooring the group value, we use the decimal place
		   as a chance of an extra monster */
		if (randint0(10) > extra_chance) {
			total += 1;
		}
	}

	/* No monsters in this group */
	if (total > 0) {
		/* Handle friends same as original monster */
		if (race->ridx == friends_race->ridx) {
			return place_new_monster_group(c, grid, race, sleep, group_info,
										   total, origin);
		} else {
			struct loc new;

			/* Find a nearby place to put the other groups. */
			if (scatter_ext(c, &new, 1, grid,
					z_info->monster_group_dist, false,
					square_isopen) > 0) {
				/* Place the monsters */
				bool success = place_new_monster_one(c, new,
					friends_race, sleep, group_info, origin);

				if (total > 1) {
					success = place_new_monster_group(c,
						new, friends_race, sleep,
						group_info, total, origin);
				}
				return success;
			}
		}
	}

	return false;
}

/**
 * Attempts to place a monster of the given race at the given location.
 *
 * Note that certain monsters are placed with a large group of
 * identical or similar monsters. However, if `group_okay` is false,
 * then such monsters are placed by themselves.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 */
bool place_new_monster(struct chunk *c, struct loc grid,
					   struct monster_race *race, bool sleep, bool group_ok,
					   struct monster_group_info group_info, byte origin)
{
	struct monster_friends *friends;
	struct monster_friends_base *friends_base;
	int total;

	assert(c);
	assert(race);

	/* If we don't have a group index already, make one; our first monster
	 * will be the leader */
	if (!group_info.index) {
		group_info.index = monster_group_index_new(c);
	}

	/* Place one monster, or fail */
	if (!place_new_monster_one(c, grid, race, sleep, group_info, origin)) {
		return (false);
	}

	/* We're done unless the group flag is set */
	if (!group_ok) return (true);

	/* Go through friends flags */
	for (friends = race->friends; friends; friends = friends->next) {
		if ((unsigned int)randint0(100) >= friends->percent_chance)
			continue;

		/* Calculate the base number of monsters to place */
		total = damroll(friends->number_dice, friends->number_side);

		/* Set group role */
		group_info.role = friends->role;

		/* Place them */
		place_friends(c, grid, race, friends->race, total, sleep, group_info,
					  origin);

	}

	/* Go through the friends_base flags */
	for (friends_base = race->friends_base; friends_base;
			friends_base = friends_base->next){
		struct monster_race *friends_race;

		/* Check if we pass chance for the monster appearing */
		if ((unsigned int)randint0(100) >= friends_base->percent_chance)
			continue;

		total = damroll(friends_base->number_dice, friends_base->number_side);

		/* Set the escort index base*/
		place_monster_base = friends_base->base;

		/* Prepare allocation table */
		get_mon_num_prep(place_monster_base_okay);

		/* Pick a random race */
		friends_race = get_mon_num(race->level, c->depth);

		/* Reset allocation table */
		get_mon_num_prep(NULL);

		/* Handle failure */
		if (!friends_race) break;

		/* Set group role */
		group_info.role = friends_base->role;

		/* Place them */
		place_friends(c, grid, race, friends_race, total, sleep, group_info,
					  origin);
	}

	/* Success */
	return (true);
}


/**
 * Picks a monster race, makes a new monster of that race, then attempts to
 * place it in the dungeon. The monster race chosen will be appropriate for
 * dungeon level equal to `depth`.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * If `group_okay` is true, we allow the placing of a group, if the chosen
 * monster appears with friends or an escort.
 *
 * `origin` is the item origin to use for any monster drops (e.g. ORIGIN_DROP,
 * ORIGIN_DROP_PIT, etc.)
 *
 * Returns true if we successfully place a monster.
 */
bool pick_and_place_monster(struct chunk *c, struct loc grid, int depth,
							bool sleep, bool group_okay, byte origin)
{
	/* Pick a monster race, no specified group */
	struct monster_race *race = get_mon_num(depth, c->depth);
	struct monster_group_info info = { 0, 0, 0 };

	/* Enforce sleep at nighttime in the wilderness */
	if (is_night() && (level_topography(player->place) != TOP_TOWN)) {
		sleep = true;
	}

	if (race) {
		return place_new_monster(c, grid, race, sleep, group_okay, info,
								 origin);
	} else {
		return false;
	}
}


/**
 * Picks a monster race, makes a new monster of that race, then attempts to
 * place it in the dungeon at least `dis` away from the player. The monster
 * race chosen will be appropriate for dungeon level equal to `depth`.
 *
 * If `sleep` is true, the monster is placed with its default sleep value,
 * which is given in monster.txt.
 *
 * Returns true if we successfully place a monster.
 */
bool pick_and_place_distant_monster(struct chunk *c, struct player *p, int dis,
		bool sleep, int depth)
{
	struct loc grid;
	int	attempts_left = 10000;

	assert(c);

	/* Find a legal, distant, unoccupied, space */
	while (--attempts_left) {
		/* Pick a location */
		grid = loc(randint0(c->width), randint0(c->height));

		/* Require "naked" floor grid */
		if (!square_isempty(c, grid)) continue;

		/* Do not put random monsters in marked rooms. */
		if ((!character_dungeon) && square_ismon_restrict(c, grid))
			continue;

		/* Accept far away grids */
		if (distance(grid, p->grid) > dis) break;
	}

	if (!attempts_left) {
		if (OPT(p, cheat_xtra) || OPT(p, cheat_hear))
			msg("Warning! Could not allocate a new monster.");

		return false;
	}

	/* Attempt to place the monster, allow groups */
	if (pick_and_place_monster(c, grid, depth, sleep, true, ORIGIN_DROP))
		return (true);

	/* Nope */
	return (false);
}

struct init_module mon_make_module = {
	.name = "monster/mon-make",
	.init = init_race_allocs,
	.cleanup = cleanup_race_allocs
};
