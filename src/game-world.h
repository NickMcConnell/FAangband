/**
 * \file game-world.h
 * \brief Game core management of the game world
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

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "cave.h"

enum locality {
	#define LOC(x, b)	LOC_##x,
	#include "list-localities.h"
	#undef LOC
};

enum topography {
	#define TOP(x, b)	TOP_##x,
	#include "list-topography.h"
	#undef TOP
};

struct level {
	int index;
	int depth;
	bool visited;
	enum locality locality;
	enum topography topography;
	char *north;
	char *east;
	char *south;
	char *west;
	char *up;
	char *down;
	struct level *next;
};

struct town {
	int index;
	char *code;
	char *ego;
	struct store *stores;
	int num_stores;
};

struct level_map {
	char *name;
	char *help;
	int num_levels;
	int num_towns;
	struct level *levels;
	struct town *towns;
	struct level_map *next;
};

extern uint16_t daycount;
extern uint32_t seed_randart;
extern uint32_t seed_flavor;
extern int32_t turn;
extern bool character_generated;
extern bool character_dungeon;
extern const uint8_t extract_energy[200];
extern struct level_map *maps;
extern struct level_map *world;

bool no_vault(int place);
const char *locality_name(enum locality locality);
char *level_name(struct level *lev);
struct level *level_by_name(struct level_map *map, const char *name);
struct town *town_by_name(struct level_map *map, const char *name);
int level_topography(int index);
struct vault *themed_level(int index);
int themed_level_index(const char *name);
bool is_daytime(void);
bool outside(void);
bool is_daylight(void);
bool is_night(void);
int turn_energy(int speed);
void play_ambient_sound(void);
void process_world(struct chunk *c);
void on_new_level(void);
void process_player(void);
void run_game_loop(void);

#endif /* !GAME_WORLD_H */
