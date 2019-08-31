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

struct level_map {
	char *name;
	int num_levels;
	int num_towns;
	struct level *levels;
	int *towns;
	struct level_map *next;
};

extern u16b daycount;
extern u32b seed_randart;
extern u32b seed_flavor;
extern s32b turn;
extern bool character_generated;
extern bool character_dungeon;
extern const byte extract_energy[200];
extern struct level_map *maps;
extern struct level_map *world;

bool no_vault(struct level *lev);
const char *locality_name(enum locality locality);
char *level_name(struct level *lev);
struct level *level_by_name(struct level_map *map, char *name);
struct level *level_by_depth(int depth);
bool is_daytime(void);
int turn_energy(int speed);
void play_ambient_sound(void);
void process_world(struct chunk *c);
void on_new_level(void);
void process_player(void);
void run_game_loop(void);

#endif /* !GAME_WORLD_H */
