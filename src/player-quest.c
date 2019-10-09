/**
 * \file player-quest.c
 * \brief All quest-related code
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
#include "datafile.h"
#include "game-world.h"
#include "init.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-quest.h"

/**
 * Array of quests
 */
struct quest *quests;

static const char *localities[] = {
	#define LOC(a, b) #a,
	#include "list-localities.h"
	#undef LOC
	NULL
};

/**
 * Parsing functions for quest.txt
 */
static enum parser_error parse_quest_name(struct parser *p) {
	const char *name = parser_getstr(p, "name");
	struct quest *h = parser_priv(p);

	struct quest *q = mem_zalloc(sizeof(*q));
	q->next = h;
	parser_setpriv(p, q);
	q->name = string_make(name);

	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_type(struct parser *p) {
	struct quest *q = parser_priv(p);
	const char *name = parser_getstr(p, "type");

	if (streq(name, "monster")) {
		q->type = QUEST_MONSTER;
	} else if (streq(name, "unique")) {
		q->type = QUEST_UNIQUE;
		q->max_num = 1;
	} else if (streq(name, "place")) {
		q->type = QUEST_PLACE;
	} else if (streq(name, "final")) {
		q->type = QUEST_FINAL;
	} else {
		return PARSE_ERROR_INVALID_QUEST_TYPE;
	}
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_race(struct parser *p) {
	struct quest *q = parser_priv(p);
	const char *name = parser_getstr(p, "race");
	assert(q);

	q->race = lookup_monster(name);
	if (!q->race)
		return PARSE_ERROR_INVALID_MONSTER;

	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_artifact(struct parser *p) {
	struct quest *q = parser_priv(p);
	int chance = parser_getuint(p, "chance");
	struct quest_artifact *arts;
	struct artifact *art;

	if (!q)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	if ((q->type != QUEST_UNIQUE) && (q->type != QUEST_FINAL)) {
		return PARSE_ERROR_ARTIFACT_IN_WRONG_QUEST;
	}
	art = lookup_artifact_name(parser_getstr(p, "name"));
	if (!art)
		return PARSE_ERROR_NO_ARTIFACT_NAME;
	arts = mem_zalloc(sizeof(*arts));
	arts->art = art;
	arts->chance = chance;
	arts->next = q->arts;
	q->arts = arts;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_number(struct parser *p) {
	struct quest *q = parser_priv(p);
	assert(q);

	q->max_num = parser_getuint(p, "number");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_map(struct parser *p) {
	struct quest *q = parser_priv(p);
	const char *name = parser_getstr(p, "mapname");
	struct level_map *map = maps;
	struct quest_place *q_place = q->place;
	assert(q);

	if (q_place) {
		while (q_place->next) {
			q_place = q_place->next;
		}
		q_place->next = mem_zalloc(sizeof(*(q->place)));
		q_place = q_place->next;
	} else {
		q_place = mem_zalloc(sizeof(*(q->place)));
		q->place = q_place;
	}

	while (map) {
		if (streq(name, map->name)) {
			q_place->map = map;
			break;
		}
		map = map->next;
	}

	if (!q_place->map) return PARSE_ERROR_INVALID_MAP;

	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_place(struct parser *p) {
	struct quest *q = parser_priv(p);
	const char *loc_name = parser_getsym(p, "locality");
	const int depth = parser_getint(p, "depth");
	struct quest_place *q_place = q->place;
	char *lev_name;
	const char *block;
	int i;
	assert(q);
	assert(q->place);

	while (q_place->next) {
		q_place = q_place->next;
	}

	if (grab_name("locality", loc_name, localities, N_ELEMENTS(localities), &i))
		return PARSE_ERROR_INVALID_LOCALITY;
	lev_name = string_make(format("%s %d", locality_name(i), depth));
	if (!q_place->map) return PARSE_ERROR_INVALID_MAP;
	q_place->place = level_by_name(q_place->map, lev_name)->index;

	if (parser_hasval(p, "block")) {
		block = parser_getsym(p, "block");
		if (streq(block, "block")) {
			q_place->block = true;
		} else {
			return PARSE_ERROR_INVALID_STRING;
		}
	} else {
		q_place->block = false;
	}
	return PARSE_ERROR_NONE;
}


struct parser *init_parse_quest(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_quest_name);
	parser_reg(p, "type str type", parse_quest_type);
	parser_reg(p, "race str race", parse_quest_race);
	parser_reg(p, "artifact uint chance str name", parse_quest_artifact);
	parser_reg(p, "number uint number", parse_quest_number);
	parser_reg(p, "map str mapname", parse_quest_map);
	parser_reg(p, "place sym locality int depth ?sym block", parse_quest_place);
	return p;
}

static errr run_parse_quest(struct parser *p) {
	return parse_file_quit_not_found(p, "quest");
}

static errr finish_parse_quest(struct parser *p) {
	quests = parser_priv(p);
	parser_destroy(p);
	return 0;
}

static void cleanup_quest(void)
{
	struct quest *quest = quests, *next;

	while (quest) {
		next = quest->next;
		string_free(quest->name);
		mem_free(quest);
		quest = next;
	}
}

struct file_parser quests_parser = {
	"quest",
	init_parse_quest,
	run_parse_quest,
	finish_parse_quest,
	cleanup_quest
};

/**
 * Set all the quests to incomplete.
 */
void quests_reset(void)
{
	struct quest *quest = quests;
	while (quest) {
		quest->complete = false;
		quest = quest->next;
	}
}

/**
 * If the given place is a quest place, return the relevant quest.
 */
struct quest *find_quest(int place)
{
	struct quest *quest = quests;

	while (quest) {
		struct quest_place *q_place = quest->place;
		while (q_place) {
			if (q_place->map != world) {
				q_place = q_place->next;
				continue;
			}
			if (q_place->place == place) {
				return quest;
			} else {
				break;
			}
		}
		quest = quest->next;
	}

	return NULL;
}

/**
 * Find the place for a given quest.
 */
static struct quest_place *find_quest_place(struct quest *quest)
{
	struct quest_place *q_place = quest->place;
	while (q_place) {
		if (q_place->map == world) {
			return q_place;
		}
		q_place = q_place->next;
	}
	return NULL;
}

/**
 * Return whether the given place is a quest place with downstairs blocked.
 */
bool quest_forbid_downstairs(int place)
{
	struct quest *quest = find_quest(place);
	if (quest) {
		struct quest_place *q_place = find_quest_place(quest);
		if (!quest->complete && q_place->block) return true;
	}
	return false;
}

/**
 * Creates magical stairs or paths after finishing a quest.
 *
 * This assumes that any exit from the quest level except upstairs is
 * blocked until the quest is complete.
 */
static void build_quest_stairs(struct loc grid)
{
	struct loc new_grid = grid;
	struct level *lev = &world->levels[player->place];
	bool path_msg = false;

	/* Downstairs */
	if (lev->down) {
		/* Stagger around */
		while (!square_changeable(cave, grid) &&
			   !square_iswall(cave, grid) &&
			   !square_isdoor(cave, grid)) {
			/* Pick a location */
			scatter(cave, &new_grid, grid, 1, false);

			/* Stagger */
			grid = new_grid;
		}

		/* Push any objects */
		push_object(grid);

		/* Explain the staircase */
		msg("A magical staircase appears...");

		/* Create stairs down */
		square_set_feat(cave, grid, FEAT_MORE);

		/* Update the visuals */
		player->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}

	/* Other directions */
	if (lev->north) {
		struct loc path_grid = grid;
		struct level *north = level_by_name(world, lev->north);
		while (path_grid.y > 1) {
			if (square_ispermanent(cave, path_grid)) break;
			square_set_feat(cave, path_grid, FEAT_ROAD);
			path_grid.y--;
		}
		if (north->depth > lev->depth) {
			square_set_feat(cave, path_grid, FEAT_MORE_NORTH);
		} else {
			square_set_feat(cave, path_grid, FEAT_LESS_NORTH);
		}
		path_msg = true;
	}
	if (lev->east) {
		struct loc path_grid = grid;
		struct level *east = level_by_name(world, lev->east);
		while (path_grid.x < cave->width - 2) {
			if (square_ispermanent(cave, path_grid)) break;
			square_set_feat(cave, path_grid, FEAT_ROAD);
			path_grid.x++;
		}
		if (east->depth > lev->depth) {
			square_set_feat(cave, path_grid, FEAT_MORE_EAST);
		} else {
			square_set_feat(cave, path_grid, FEAT_LESS_EAST);
		}
		path_msg = true;
	}
	if (lev->south) {
		struct loc path_grid = grid;
		struct level *south = level_by_name(world, lev->south);
		while (path_grid.y < cave->height - 2) {
			if (square_ispermanent(cave, path_grid)) break;
			square_set_feat(cave, path_grid, FEAT_ROAD);
			path_grid.y++;
		}
		if (south->depth > lev->depth) {
			square_set_feat(cave, path_grid, FEAT_MORE_SOUTH);
		} else {
			square_set_feat(cave, path_grid, FEAT_LESS_SOUTH);
		}
		path_msg = true;
	}
	if (lev->west) {
		struct loc path_grid = grid;
		struct level *west = level_by_name(world, lev->west);
		while (path_grid.x > 1) {
			if (square_ispermanent(cave, path_grid)) break;
			square_set_feat(cave, path_grid, FEAT_ROAD);
			path_grid.x--;
		}
		if (west->depth > lev->depth) {
			square_set_feat(cave, path_grid, FEAT_MORE_WEST);
		} else {
			square_set_feat(cave, path_grid, FEAT_LESS_WEST);
		}
		path_msg = true;
	}

	if (path_msg) {
		msg("The way out of %s is revealed!", locality_name(lev->locality));
	}
}

/**
 * Check if a monster is a unique quest monster
 */
bool quest_unique_monster_check(const struct monster_race *race)
{
	struct quest *quest = quests;

	while (quest) {
		if ((quest->type == QUEST_UNIQUE) && (quest->race == race )) {
			return true;
		}
		quest = quest->next;
	}
	return false;
}

/**
 * Check if this (now dead) monster is a quest monster, and act appropriately
 */
bool quest_monster_death_check(const struct monster *mon)
{
	struct quest *quest = find_quest(player->place);

	/* Simple checks */
	if (!quest) return false;
	if ((mon->race != quest->race) || quest->complete) return false;

	/* Increment count, check for completion */
	quest->cur_num++;
	if (quest->cur_num >= quest->max_num) {
		struct quest_place *place = find_quest_place(quest);
		quest->complete = true;

		/* Build magical stairs if needed */
		if (place->block) {
			build_quest_stairs(mon->grid);
		}
	}

	/* Game over... */
	if ((quest->type == QUEST_FINAL) && quest->complete) {
		player->total_winner = true;
		player->upkeep->redraw |= (PR_TITLE);
		msg("*** CONGRATULATIONS ***");
		msg("You have won the game!");
		msg("You may retire (commit suicide) when you are ready.");
	}

	return true;
}

/**
 * Check if the player has arrived at a quest place
 */
bool quest_place_check(void)
{
	struct quest *quest = find_quest(player->place);
	if (!quest) return false;
	if (quest->type != QUEST_PLACE) return false;
	quest->complete = true;
	msg("You have completed the %s quest!", quest->name);
	return true;
}
