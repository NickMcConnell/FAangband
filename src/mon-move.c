/**
 * \file mon-move.c
 * \brief Monster movement
 *
 * Monster AI affecting movement and spells, process a monster 
 * (with spells and actions of all kinds, reproduction, effects of any 
 * terrain on monster movement, picking up and destroying objects), 
 * process all monsters.
 *
 * Copyright (c) 1997 Ben Harrison, David Reeve Sward, Keldon Jones.
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
#include "init.h"
#include "monster.h"
#include "mon-attack.h"
#include "mon-desc.h"
#include "mon-group.h"
#include "mon-lore.h"
#include "mon-make.h"
#include "mon-move.h"
#include "mon-predicate.h"
#include "mon-spell.h"
#include "mon-util.h"
#include "mon-timed.h"
#include "obj-desc.h"
#include "obj-ignore.h"
#include "obj-knowledge.h"
#include "obj-pile.h"
#include "obj-slays.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "project.h"
#include "trap.h"


/**
 * ------------------------------------------------------------------------
 * Routines to enable decisions on monster behaviour
 * ------------------------------------------------------------------------ */
/**
 * From Will Asher in DJA:
 * Find whether a monster is near a permanent wall
 *
 * this decides whether PASS_WALL & KILL_WALL monsters use the monster flow code
 */
static bool monster_near_permwall(const struct monster *mon)
{
	struct loc gp[512], t_grid = monster_target_loc(mon);
	int path_grids, j;

	/* If player is in LOS, there's no need to go around walls */
    if (projectable(cave, mon->grid, t_grid, PROJECT_NONE)) return false;

    /* PASS_WALL & KILL_WALL monsters occasionally flow for a turn anyway */
    if (randint0(99) < 5) return true;

	/* Find the shortest path */
	path_grids = project_path(cave, gp, z_info->max_sight, mon->grid, t_grid,
							  PROJECT_ROCK);

	/* See if we can "see" the player without hitting permanent wall */
	for (j = 0; j < path_grids; j++) {
		if (square_isperm(cave, gp[j])) return true;
		if (loc_eq(gp[j], t_grid)) return false;
	}

	return false;
}

/**
 * Check if the monster can see the target
 */
static bool monster_can_see_target(struct monster *mon)
{
	/* Check for cover tracks */
	if ((mon->target.midx == -1) && player->timed[TMD_COVERTRACKS] &&
		(mon->cdis > z_info->max_sight / 4)) {
		return false;
	}
	return projectable(cave, mon->grid, monster_target_loc(mon),
					   PROJECT_NONE);
}

/**
 * Check if the monster can hear anything
 */
static bool monster_can_hear(struct monster *mon)
{
	int hearing = mon->race->hearing;
	struct heatmap noise_map = { 0 };

	/* Set target info */
	if (mon->target.midx == -1) {
		noise_map = cave->noise;
		hearing -= player->state.skills[SKILL_STEALTH] / 3;
	} else if (mon->target.midx > 0) {
		noise_map = cave_monster(cave, mon->target.midx)->noise;
	} else {
		return false;
	}

	/* Try and hear */
	if (noise_map.grids[mon->grid.y][mon->grid.x] == 0) {
		return false;
	}
	return hearing > noise_map.grids[mon->grid.y][mon->grid.x];
}

/**
 * Check if the monster can smell anything
 */
static bool monster_can_smell(struct monster *mon)
{
	struct heatmap scent_map = { 0 };

	/* Set target info */
	if (mon->target.midx == -1) {
		scent_map = cave->scent;
	} else if (mon->target.midx > 0) {
		scent_map = cave_monster(cave, mon->target.midx)->scent;
	} else {
		return false;
	}

	/* Try and hear */
	if (scent_map.grids[mon->grid.y][mon->grid.x] == 0) {
		return false;
	}
	return mon->race->smell > scent_map.grids[mon->grid.y][mon->grid.x];
}

/**
 * Compare the "strength" of two monsters XXX XXX XXX
 */
static int compare_monsters(const struct monster *mon1,
							const struct monster *mon2)
{
	uint32_t mexp1 = (mon1->original_race) ?
		mon1->original_race->mexp : mon1->race->mexp;
	uint32_t mexp2 = (mon2->original_race) ?
		mon2->original_race->mexp : mon2->race->mexp;

	/* Compare */
	if (mexp1 < mexp2) return (-1);
	if (mexp1 > mexp2) return (1);

	/* Assume equal */
	return (0);
}

/**
 * Check if the monster can kill any monster on the relevant grid
 */
static bool monster_can_kill(struct monster *mon, struct loc grid)
{
	struct monster *mon1 = square_monster(cave, grid);

	/* No monster */
	if (!mon1) return true;

	/* No trampling uniques */
	if (monster_is_unique(mon1)) {
		return false;
	}

	if (rf_has(mon->race->flags, RF_KILL_BODY) &&
		compare_monsters(mon, mon1) > 0) {
		return true;
	}

	return false;
}

/**
 * Check if the monster can move any monster on the relevant grid
 */
static bool monster_can_move(struct monster *mon, struct loc grid)
{
	struct monster *mon1 = square_monster(cave, grid);

	/* No monster */
	if (!mon1) return true;

	if (rf_has(mon->race->flags, RF_MOVE_BODY) &&
		compare_monsters(mon, mon1) > 0) {
		return true;
	}

	return false;
}

/**
 * Check if the monster can occupy a grid safely
 */
static bool monster_hates_grid(struct monster *mon, struct loc grid)
{
	/* Only some creatures can handle damaging terrain */
	if (square_isdamaging(cave, grid) &&
		!rf_has(mon->race->flags, square_feat(cave, grid)->resist_flag)) {
		return true;
	}
	return false;
}

/**
 * ------------------------------------------------------------------------
 * Monster movement routines
 * These routines, culminating in get_move(), choose if and where a monster
 * will move on its turn
 * ------------------------------------------------------------------------ */
/**
 * Calculate minimum and desired combat ranges.  -BR-
 *
 * Afraid monsters will set this to their maximum flight distance.
 * Currently this is recalculated every turn - if it becomes a significant
 * overhead it could be calculated only when something has changed (monster HP,
 * chance of escaping, etc)
 *
 * Note that it is assumed that the player is the main source of danger to the
 * monster, even if it has another monster or grid as a target.
 */
static void get_move_find_range(struct monster *mon)
{
	uint16_t p_lev, m_lev;
	uint16_t p_chp, p_mhp;
	uint16_t m_chp, m_mhp;
	uint32_t p_val, m_val;

	/* Monsters will run up to z_info->flee_range grids out of sight */
	int flee_range = z_info->max_sight + z_info->flee_range;

	/* All "afraid" monsters will run away */
	if (mon->m_timed[MON_TMD_FEAR] || rf_has(mon->race->flags, RF_FRIGHTENED)) {
		mon->min_range = flee_range;
	} else if (mon->group_info[PRIMARY_GROUP].role == MON_GROUP_BODYGUARD) {
		/* Bodyguards don't flee */
		mon->min_range = 1;
	} else {
		/* Minimum distance - stay at least this far if possible */
		mon->min_range = 1;

		/* Taunted monsters just want to get in your face */
		if (player->timed[TMD_TAUNT]) return;

		/* Examine player power (level) */
		p_lev = player->lev;

		/* Hack - increase p_lev based on specialty abilities */

		/* Examine monster power (level plus morale) */
		m_lev = mon->race->level + (mon->midx & 0x08) + 25;

		/* Simple cases first */
		if (m_lev + 3 < p_lev) {
			mon->min_range = flee_range;
		} else if (m_lev - 5 < p_lev) {

			/* Examine player health */
			p_chp = player->chp;
			p_mhp = player->mhp;

			/* Examine monster health */
			m_chp = mon->hp;
			m_mhp = mon->maxhp;

			/* Prepare to optimize the calculation */
			p_val = (p_lev * p_mhp) + (p_chp << 2);	/* div p_mhp */
			m_val = (m_lev * m_mhp) + (m_chp << 2);	/* div m_mhp */

			/* Strong players scare strong monsters */
			if (p_val * m_mhp > m_val * p_mhp)
				mon->min_range = flee_range;
		}
	}

	if (mon->min_range < flee_range) {
		/* Creatures that don't move never like to get too close */
		if (rf_has(mon->race->flags, RF_NEVER_MOVE))
			mon->min_range += 3;

		/* Spellcasters that don't strike never like to get too close */
		if (rf_has(mon->race->flags, RF_NEVER_BLOW))
			mon->min_range += 3;
	}

	/* Maximum range to flee to */
	if (!(mon->min_range < flee_range)) {
		mon->min_range = flee_range;
	} else if (mon->cdis < z_info->turn_range) {
		/* Nearby monsters won't run away */
		mon->min_range = 1;
	}

	/* Now find preferred range */
	mon->best_range = mon->min_range;

	/* Archers are quite happy at a good distance */
	if (monster_loves_archery(mon)) {
		mon->best_range += 3;
	}

	/* Breathers like point blank range */
	if (mon->race->freq_innate > 24) {
		if (monster_breathes(mon) && (mon->hp > mon->maxhp / 2)) {
			mon->best_range = MAX(1, mon->best_range);
		}
	} else if (mon->race->freq_spell > 24) {
		/* Other spell casters will sit back and cast */
		mon->best_range += 3;
	}
}

/**
 * Choose the best direction for a bodyguard.
 *
 * The idea is to stay close to the group leader, but attack the player if the
 * chance arises
 */
static bool get_move_bodyguard(struct monster *mon)
{
	int i;
	struct monster *leader = monster_group_leader(cave, mon);
	int dist;
	struct loc best;
	bool found = false;

	if (!leader) return false;

	/* Get distance */
	dist = distance(mon->grid, leader->grid);

	/* If currently adjacent to the leader, we can afford a move */
	if (dist <= 1) return false;

	/* If the leader's too out of sight and far away, save yourself */
	if (!los(cave, mon->grid, leader->grid) && (dist > 10)) return false;

	/* Check nearby adjacent grids and assess */
	for (i = 0; i < 8; i++) {
		/* Get the location */
		struct loc grid = loc_sum(mon->grid, ddgrid_ddd[i]);
		int new_dist = distance(grid, leader->grid);
		int target_dist = distance(grid, monster_target_loc(leader));

		/* Bounds check */
		if (!square_in_bounds(cave, grid)) {
			continue;
		}

		/* There's a monster blocking that we can't deal with */
		if (!monster_can_kill(mon, grid) && !monster_can_move(mon, grid)){
			continue;
		}

		/* There's damaging terrain */
		if (monster_hates_grid(mon, grid)) {
			continue;
		}

		/* Closer to the leader is always better */
		if (new_dist < dist) {
			best = grid;
			found = true;
			/* If there's a grid that's also closer to the target, that wins */
			if (target_dist < mon->cdis) {
				break;
			}
		}
	}

	/* If we found one, set the target */
	if (found) {
		mon->target.grid = best;
		return true;
	}

	return false;
}


/**
 * Choose the best direction to advance toward the player, using sound or scent.
 *
 * Ghosts and rock-eaters generally just head straight for the player. Other
 * monsters try sight, then current sound as saved in cave->noise.grids[y][x],
 * then current scent as saved in cave->scent.grids[y][x].
 *
 * This function assumes the monster is moving to an adjacent grid, and so the
 * noise can be louder by at most 1.  The monster target grid set by sound or
 * scent tracking in this function will be a grid they can step to in one turn,
 * so is the preferred option for get_move() unless there's some reason
 * not to use it.
 *
 * Tracking by 'scent' means that monsters end up near enough the player to
 * switch to 'sound' (noise), or they end up somewhere the player left via 
 * teleport.  Teleporting away from a location will cause the monsters who
 * were chasing the player to converge on that location as long as the player
 * is still near enough to "annoy" them without being close enough to chase
 * directly.
 *
 * Note that the above applies equally if the target is a monster rather than
 * the player.
 */
static bool get_move_advance(struct monster *mon, bool *track)
{
	int i;
	struct loc target = monster_target_loc(mon);

	int hearing = mon->race->hearing;
	struct heatmap noise_map = { 0 };
	struct heatmap scent_map = { 0 };

	struct loc best_grid;
	struct loc backup_grid;
	bool found = false;
	bool found_backup = false;

	/* Bodyguards are special */
	if (mon->group_info[PRIMARY_GROUP].role == MON_GROUP_BODYGUARD) {
		if (get_move_bodyguard(mon)) {
			return true;
		}
	}

	/* If the monster can pass through nearby walls, do that */
	if (monster_passes_walls(mon) && !monster_near_permwall(mon)) {
		mon->target.grid = target;
		return true;
	}

	/* If the monster can see the target, run towards it */
	if (monster_can_see_target(mon)) {
		mon->target.grid = target;
		return true;
	}

	/* Set target info */
	if (mon->target.midx == -1) {
		/* Player */
		noise_map = cave->noise;
		scent_map = cave->scent;
		hearing -= player->state.skills[SKILL_STEALTH] / 3;
	} else if (mon->target.midx > 0) {
		/* Monster */
		noise_map = cave_monster(cave, mon->target.midx)->noise;
	} else {
		/* Location */
		best_grid = target;
	}


	/* Try to use sound */
	if (monster_can_hear(mon)) {
		int current_noise = hearing - noise_map.grids[mon->grid.y][mon->grid.x];

		/* Check nearby sound, giving preference to the cardinal directions */
		for (i = 0; i < 8; i++) {
			/* Get the location */
			struct loc grid = loc_sum(mon->grid, ddgrid_ddd[i]);
			int heard_noise = hearing - noise_map.grids[grid.y][grid.x];

			/* Bounds check */
			if (!square_in_bounds(cave, grid)) {
				continue;
			}

			/* Must be some noise */
			if (noise_map.grids[grid.y][grid.x] == 0) {
				continue;
			}

			/* There's a monster blocking that we can't deal with */
			if (!monster_can_kill(mon, grid) && !monster_can_move(mon, grid)) {
				continue;
			}

			/* There's damaging terrain */
			if (monster_hates_grid(mon, grid)) {
				continue;
			}

			/* If it's better than the current noise, choose this direction */
			if (heard_noise > current_noise) {
				best_grid = grid;
				found = true;
				break;
			} else if (heard_noise == current_noise) {
				/* Possible move if we can't actually get closer */
				backup_grid = grid;
				found_backup = true;
				continue;
			}
		}
	}

	/* If both vision and sound are no good, use scent */
	if (monster_can_smell(mon) && !found) {
		int best_scent = 0;
		for (i = 0; i < 8; i++) {
			/* Get the location */
			struct loc grid = loc_sum(mon->grid, ddgrid_ddd[i]);
			int scent = mon->race->smell - scent_map.grids[grid.y][grid.x];

			if ((scent > best_scent) &&	scent_map.grids[grid.y][grid.x]) {
				best_scent = scent;
				best_grid = grid;
				found = true;
			}
		}
	}

	/* Set the target */
	if (found) {
		mon->target.grid = best_grid;
		*track = true;
		return true;
	} else if (found_backup) {
		/* Move around to try and improve position */
		mon->target.grid = backup_grid;
		*track = true;
		return true;
	}

	/* No reason to advance */
	return false;
}

/**
 * Choose a random passable grid adjacent to the monster since is has no better
 * strategy.
 */
static struct loc get_move_random(struct monster *mon)
{
	int attempts[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	int nleft = 8;

	while (nleft > 0) {
		int itry = randint0(nleft);
		struct loc trygrid;

		trygrid = loc_sum(mon->grid, ddgrid_ddd[attempts[itry]]);
		if (square_is_monster_walkable(cave, trygrid) &&
				!monster_hates_grid(mon, trygrid)) {
			return ddgrid_ddd[attempts[itry]];
		} else {
			int tmp = attempts[itry];

			--nleft;
			attempts[itry] = attempts[nleft];
			attempts[nleft] = tmp;
		}
	}

	return loc(0, 0);
}

/**
 * Choose a "safe" location near a monster for it to run toward.
 *
 * A location is "safe" if it can be reached quickly and the player
 * is not able to fire into it (it isn't a "clean shot").  So, this will
 * cause monsters to "duck" behind walls.  Hopefully, monsters will also
 * try to run towards corridor openings if they are in a room.
 *
 * This function may take lots of CPU time if lots of monsters are fleeing.
 *
 * Note that it is assumed that the player is the main source of danger to the
 * monster, even if it has another monster or grid as a target.
 *
 * Return true if a safe location is available.
 */
static bool get_move_find_safety(struct monster *mon)
{
	int i, dy, dx, d, dis, gdis = 0;

	const int *y_offsets;
	const int *x_offsets;

	/* Start with adjacent locations, spread further */
	for (d = 1; d < 10; d++) {
		struct loc best = loc(0, 0);

		/* Get the lists of points with a distance d from (fx, fy) */
		y_offsets = dist_offsets_y[d];
		x_offsets = dist_offsets_x[d];

		/* Check the locations */
		for (i = 0, dx = x_offsets[0], dy = y_offsets[0];
		     dx != 0 || dy != 0;
		     i++, dx = x_offsets[i], dy = y_offsets[i]) {
			struct loc grid = loc_sum(mon->grid, loc(dx, dy));

			/* Skip illegal locations */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Skip locations in a wall */
			if (!square_ispassable(cave, grid)) continue;

			/* Ignore too-distant grids */
			if (cave->noise.grids[grid.y][grid.x] >
				cave->noise.grids[mon->grid.y][mon->grid.x] + 2 * d)
				continue;

			/* Ignore damaging terrain if they can't handle it */
			if (monster_hates_grid(mon, grid)) continue;

			/* Check for absence of shot (more or less) */
			if (!square_isview(cave, grid)) {
				/* Calculate distance from player */
				dis = distance(grid, player->grid);

				/* Remember if further than previous */
				if (dis > gdis) {
					best = grid;
					gdis = dis;
				}
			}
		}

		/* Check for success */
		if (gdis > 0) {
			/* Good location */
			mon->target.grid = best;
			return (true);
		}
	}

	/* No safe place */
	return (false);
}

/**
 * Choose a good hiding place near a monster for it to run toward.
 *
 * Pack monsters will use this to "ambush" the target and lure it out
 * of corridors into open space so they can swarm it.
 *
 * Return true if a good location is available.
 */
static bool get_move_find_hiding(struct monster *mon)
{
	struct loc target = monster_target_loc(mon);
	int i, dy, dx, d, dis, gdis = 999, min;
	const int *y_offsets, *x_offsets;

	/* Closest distance to get */
	min = distance(target, mon->grid) * 3 / 4 + 2;

	/* Start with adjacent locations, spread further */
	for (d = 1; d < 10; d++) {
		struct loc best = loc(0, 0);

		/* Get the lists of points with a distance d from monster */
		y_offsets = dist_offsets_y[d];
		x_offsets = dist_offsets_x[d];

		/* Check the locations */
		for (i = 0, dx = x_offsets[0], dy = y_offsets[0];
		     dx != 0 || dy != 0;
		     i++, dx = x_offsets[i], dy = y_offsets[i]) {
			struct loc grid = loc_sum(mon->grid, loc(dx, dy));

			/* Skip illegal locations */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Skip occupied locations */
			if (!square_isempty(cave, grid)) continue;

			/* Check for hidden, available grid */
			if (!square_isview(cave, grid) &&
				projectable(cave, mon->grid, grid, PROJECT_STOP)) {
				/* Calculate distance from target */
				dis = distance(grid, target);

				/* Remember if closer than previous */
				if (dis < gdis && dis >= min) {
					best = grid;
					gdis = dis;
				}
			}
		}

		/* Check for success */
		if (gdis < 999) {
			/* Good location */
			mon->target.grid = best;
			return (true);
		}
	}

	/* No good place */
	return (false);
}

/**
 * Provide a location to flee to, but give the player a wide berth.
 *
 * A monster may wish to flee to a location that is behind the player,
 * but instead of heading directly for it, the monster should "swerve"
 * around the player so that it has a smaller chance of getting hit.
 *
 * Note that it is assumed that the player is the main source of danger to the
 * monster, even if it has another monster or grid as a target.
 */
static bool get_move_flee(struct monster *mon)
{
	int i;
	struct loc best = loc(0, 0);
	int best_score = -1;

	/* Taking damage from terrain makes moving vital */
	if (!monster_taking_terrain_damage(cave, mon)) {
		/* If the player is not currently near the monster, no reason to flow */
		if (mon->cdis >= mon->best_range) {
			return false;
		}

		/* Monster is too far away to use sound or scent */
		if (!monster_can_hear(mon) && !monster_can_smell(mon)) {
			return false;
		}
	}

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--) {
		int dis, score;

		/* Get the location */
		struct loc grid = loc_sum(mon->grid, ddgrid_ddd[i]);

		/* Bounds check */
		if (!square_in_bounds(cave, grid)) continue;

		/* Calculate distance of this grid from our target */
		dis = distance(grid, mon->target.grid);

		/* Score this grid
		 * First half of calculation is inversely proportional to distance
		 * Second half is inversely proportional to grid's distance from player
		 */
		score = 5000 / (dis + 3) - 500 /(cave->noise.grids[grid.y][grid.x] + 1);

		/* No negative scores */
		if (score < 0) score = 0;

		/* Ignore lower scores */
		if (score < best_score) continue;

		/* Save the score */
		best_score = score;

		/* Save the location */
		best = grid;
	}

	/* Set the immediate target */
	mon->target.grid = best;

	/* Success */
	return true;
}

/**
 * Choose the basic direction of movement, and whether to bias left or right
 * if the main direction is blocked.
 *
 * Note that the input is an offset to the monster's current position, and
 * the output direction is intended as an index into the side_dirs array.
 */
static int get_move_choose_direction(struct loc offset)
{
	int dir = 0;
	int dx = offset.x, dy = offset.y;

	/* Extract the "absolute distances" */
	int ay = ABS(dy);
	int ax = ABS(dx);

	/* We mostly want to move vertically */
	if (ay > (ax * 2)) {
		/* Choose between directions '8' and '2' */
		if (dy > 0) {
			/* We're heading down */
			dir = 2;
			if ((dx > 0) || (dx == 0 && turn % 2 == 0))
				dir += 10;
		} else {
			/* We're heading up */
			dir = 8;
			if ((dx < 0) || (dx == 0 && turn % 2 == 0))
				dir += 10;
		}
	}

	/* We mostly want to move horizontally */
	else if (ax > (ay * 2)) {
		/* Choose between directions '4' and '6' */
		if (dx > 0) {
			/* We're heading right */
			dir = 6;
			if ((dy < 0) || (dy == 0 && turn % 2 == 0))
				dir += 10;
		} else {
			/* We're heading left */
			dir = 4;
			if ((dy > 0) || (dy == 0 && turn % 2 == 0))
				dir += 10;
		}
	}

	/* We want to move down and sideways */
	else if (dy > 0) {
		/* Choose between directions '1' and '3' */
		if (dx > 0) {
			/* We're heading down and right */
			dir = 3;
			if ((ay < ax) || (ay == ax && turn % 2 == 0))
				dir += 10;
		} else {
			/* We're heading down and left */
			dir = 1;
			if ((ay > ax) || (ay == ax && turn % 2 == 0))
				dir += 10;
		}
	}

	/* We want to move up and sideways */
	else {
		/* Choose between directions '7' and '9' */
		if (dx > 0) {
			/* We're heading up and right */
			dir = 9;
			if ((ay > ax) || (ay == ax && turn % 2 == 0))
				dir += 10;
		} else {
			/* We're heading up and left */
			dir = 7;
			if ((ay < ax) || (ay == ax && turn % 2 == 0))
				dir += 10;
		}
	}

	return dir;
}

/**
 * Choose "logical" directions for monster movement
 *
 * This function is responsible for deciding where the monster wants to move,
 * and so is the core of monster "AI".
 *
 * First, we work out how best to advance toward the target:
 * - Try to head toward the target directly if we can pass through walls or
 *   if we can see it
 * - Failing that follow the target by sound, or failing that by scent
 * - If none of that works, just head in the general direction
 * Then we look at possible reasons not to just advance:
 * - If we're part of a pack, try to lure the target into the open
 * - If we're afraid, try to find a safe place to run to, and if no safe place
 *   just run in the opposite direction to the advance move
 * - If we can see the target and we're part of a group, try and surround them
 *
 * The function then returns false if we're already where we want to be, and
 * otherwise sets the chosen direction to step and returns true.
 */
static bool get_move(struct monster *mon, int *dir, bool *good)
{
	struct loc target = monster_target_loc(mon);
	bool group_ai = rf_has(mon->race->flags, RF_GROUP_AI);

	/* Offset to current position to move toward */
	struct loc grid = loc(0, 0);

	/* Monsters will run up to z_info->flee_range grids out of sight */
	int flee_range = z_info->max_sight + z_info->flee_range;

	bool done = false;

	/* Calculate range */
	get_move_find_range(mon);

	/* Assume we're heading towards the target */
	if (get_move_advance(mon, good)) {
		/* We have a good move, use it */
		grid = loc_diff(mon->target.grid, mon->grid);
		mflag_on(mon->mflag, MFLAG_TRACKING);
	} else {
		/* Try to follow someone who knows where they're going */
		struct monster *tracker = group_monster_tracking(cave, mon);
		if (tracker && los(cave, mon->grid, tracker->grid)) { /* Need los? */
			grid = loc_diff(tracker->grid, mon->grid);
			/* No longer tracking */
			mflag_off(mon->mflag, MFLAG_TRACKING);
		} else {
			if (mflag_has(mon->mflag, MFLAG_TRACKING)) {
				/* Keep heading to the most recent goal. */
				grid = loc_diff(mon->target.grid, mon->grid);
			}
			if (loc_is_zero(grid)) {
				/* Try a random move and no longer track. */
				grid = get_move_random(mon);
				mflag_off(mon->mflag, MFLAG_TRACKING);
			}
		}
	}

	/* Monster is taking damage from terrain */
	if (monster_taking_terrain_damage(cave, mon)) {
		/* Try to find safe place */
		if (get_move_find_safety(mon)) {
			/* Set a course for the safe place */
			get_move_flee(mon);
			grid = loc_diff(mon->target.grid, mon->grid);
			done = true;
		}
	}

	/* Normal animal packs try to get the target out of corridors. */
	if (!done && group_ai && !monster_passes_walls(mon)) {
		int i, open = 0;
		bool strong = false;

		/* Count empty grids next to target */
		for (i = 0; i < 8; i++) {
			/* Check grid around the target for room interior (room walls count)
			 * or other empty space */
			struct loc test = loc_sum(target, ddgrid_ddd[i]);
			if (square_ispassable(cave, test) || square_isroom(cave, test)) {
				/* One more open grid */
				open++;
			}
		}

		/* Assess strength of target */
		if (mon->target.midx == -1) {
			strong = (player->chp > player->mhp / 2) ? true : false;
		} else if (mon->target.midx > 0) {
			struct monster *t_mon = cave_monster(cave, mon->target.midx);
			strong = (mon->hp > t_mon->hp) ? true : false;
		}

		/* Not in an empty space and strong player */
		if ((open < 5) && strong) {
			/* Find hiding place for an ambush */
			if (get_move_find_hiding(mon)) {
				done = true;
				grid = loc_diff(mon->target.grid, mon->grid);

				/* No longer tracking */
				mflag_off(mon->mflag, MFLAG_TRACKING);
			}
		}
	}

	/* Not hiding and monster is afraid */
	if (!done && (mon->min_range == flee_range)) {
		/* Try to find safe place */
		if (get_move_find_safety(mon)) {
			/* Set a course for the safe place */
			get_move_flee(mon);
			grid = loc_diff(mon->target.grid, mon->grid);
		} else {
			/* Just leg it away from the player */
			grid = loc_diff(loc(0, 0), grid);
		}

		/* No longer tracking */
		mflag_off(mon->mflag, MFLAG_TRACKING);
		done = true;
	}

	/* Monster groups try to surround the player if they're in sight */
	if (!done && group_ai && square_isview(cave, mon->grid)) {
		int i;
		struct loc grid1 = mon->target.grid;

		/* If we are not already adjacent */
		if (mon->cdis > 1) {
			/* Find an empty square near the player to fill */
			int tmp = randint0(8);
			for (i = 0; i < 8; i++) {
				/* Pick squares near target (pseudo-randomly) */
				grid1 = loc_sum(target, ddgrid_ddd[(tmp + i) % 8]);

				/* Ignore filled grids */
				if (!square_isempty(cave, grid1)) continue;

				/* Try to fill this hole */
				break;
			}
		}

		/* Head in the direction of the chosen grid */
		grid = loc_diff(grid1, mon->grid);
	}

	/* Check if the monster has already reached its target */
	if (loc_is_zero(grid)) return (false);

	/* Pick the correct direction */
	*dir = get_move_choose_direction(grid);

	/* Want to move */
	return (true);
}


/**
 * ------------------------------------------------------------------------
 * Monster turn routines
 * These routines, culminating in monster_turn(), decide how a monster uses
 * its turn
 * ------------------------------------------------------------------------ */
/**
 * Lets the given monster attempt to reproduce.
 *
 * Note that "reproduction" REQUIRES empty space.
 *
 * Returns true if the monster successfully reproduced.
 */
bool multiply_monster(const struct monster *mon)
{
	struct loc grid;
	bool result;
	struct monster_group_info info = { 0, 0, 0 };

	/*
	 * Pick an empty location except for uniques:  they can never
	 * multiply (need a check here as the ones in place_new_monster()
	 * are not sufficient for a unique shape of a shapechanged monster
	 * since it may have zero for cur_num in the race structure for the
	 * shape).
	 */
	if (!monster_is_shape_unique(mon) && scatter_ext(cave, &grid,
			1, mon->grid, 1, true, square_isempty) > 0) {
		/* Create a new monster (awake, no groups) */
		result = place_new_monster(cave, grid, mon->race, false, false,
			info, ORIGIN_DROP_BREED);
		/*
		 * Fix so multiplying a revealed camouflaged monster creates
		 * another revealed camouflaged monster.
		 */
		if (result) {
			struct monster *child = square_monster(cave, grid);

			if (child && monster_is_camouflaged(child)
					&& !monster_is_camouflaged(mon)) {
				become_aware(cave, child);
			}
		}
	} else {
		result = false;
	}

	/* Result */
	return (result);
}

/**
 * Attempt to reproduce, if possible.  All monsters are checked here for
 * lore purposes, the unfit fail.
 */
static bool monster_turn_multiply(struct monster *mon)
{
	int k = 0, y, x;

	struct monster_lore *lore = get_lore(mon->race);

	/* Too many breeders on the level already */
	if (cave->num_repro >= z_info->repro_monster_max) return false;

	/* No breeding in single combat */
	if (player->upkeep->arena_level) return false;	

	/* Count the adjacent monsters */
	for (y = mon->grid.y - 1; y <= mon->grid.y + 1; y++)
		for (x = mon->grid.x - 1; x <= mon->grid.x + 1; x++)
			if (square(cave, loc(x, y))->mon > 0) k++;

	/* Multiply slower in crowded areas */
	if ((k < 4) && (k == 0 || one_in_(k * z_info->repro_monster_rate))) {
		/* Successful breeding attempt, learn about that now */
		if (monster_is_visible(mon))
			rf_on(lore->flags, RF_MULTIPLY);

		/* Leave now if not a breeder */
		if (!rf_has(mon->race->flags, RF_MULTIPLY))
			return false;

		/* Try to multiply */
		if (multiply_monster(mon)) {
			/* Make a sound */
			if (monster_is_visible(mon))
				sound(MSG_MULTIPLY);

			/* Multiplying takes energy */
			return true;
		}
	}

	return false;
}

/**
 * Check if a monster should stagger (that is, step at random) or not.
 * Always stagger when confused, but also deal with random movement for
 * RAND_25 and RAND_50 monsters.
 */
static enum monster_stagger monster_turn_should_stagger(struct monster *mon)
{
	struct monster_lore *lore = get_lore(mon->race);
	int chance = 0, confused_chance, roll;

	/* Increase chance of being erratic for every level of confusion */
	int conf_level = monster_effect_level(mon, MON_TMD_CONF);
	while (conf_level) {
		int accuracy = 100 - chance;
		accuracy *= (100 - CONF_ERRATIC_CHANCE);
		accuracy /= 100;
		chance = 100 - accuracy;
		conf_level--;
	}
	confused_chance = chance;

	/* RAND_25 and RAND_50 are cumulative */
	if (rf_has(mon->race->flags, RF_RAND_25)) {
		chance += 25;
		if (monster_is_visible(mon))
			rf_on(lore->flags, RF_RAND_25);
	}

	if (rf_has(mon->race->flags, RF_RAND_50)) {
		chance += 50;
		if (monster_is_visible(mon))
			rf_on(lore->flags, RF_RAND_50);
	}

	roll = randint0(100);
	return (roll < confused_chance) ?
		 CONFUSED_STAGGER :
		 ((roll < chance) ? INNATE_STAGGER : NO_STAGGER);
}


/**
 * Helper function for monster_turn_can_move() to display a message for a
 * confused move into non-passable terrain.
 */
static void monster_display_confused_move_msg(struct monster *mon,
											  const char *m_name,
											  struct loc new)
{
	if (monster_is_visible(mon) && monster_is_in_view(mon)) {
		const char *m = square_feat(cave, new)->confused_msg;

		msg("%s %s.", m_name, (m) ? m : "stumbles");
	}
}


/**
 * Helper function for monster_turn_can_move() to slightly stun a monster
 * on occasion due to bumbling into something.
 */
static void monster_slightly_stun_by_move(struct monster *mon)
{
	if (mon->m_timed[MON_TMD_STUN] < 5 && one_in_(3)) {
		mon_inc_timed(mon, MON_TMD_STUN, 3, 0);
	}
}

/**
 * Work out what chance a monster has of moving through a passable grid.
 *
 * Returns the true if the monster succeeded in moving
 */
static bool monster_can_move_passable(struct monster *mon, struct loc grid)
{
	struct monster_race *race = mon->race;

	/* Go through all the passable terrain types that affect movement speed */
	if (square_iswatery(cave, grid)) {
		if (rf_has(race->flags, RF_FLYING)) {
			/* Flying monsters can always cross water */
			return true;
		} else if (rsf_has(race->spell_flags, RSF_BR_FIRE)) {
			/* Firebreathers cannot cross water */
			return false;
		} else if (rf_has(race->flags, RF_DEMON)) {
			/* Earthbound demons cannot cross water */
			return false;
		} else if (rf_has(race->flags, RF_HUMANOID)) {
			/* Humanoid-type monsters are slowed a little */
			return one_in_(4) ? false : true;
		} else if (rf_has(race->flags, RF_UNDEAD)) {
			/* Undead are slowed a little more (a la Ringwraiths) */
			return one_in_(2) ? true : false;
		} else {
			/* Everything else has no problems crossing water */
			return true;
		}
	} else if (square_isfiery(cave, grid)) {
		/* Only fiery or strong flying creatures will cross lava */
		if (rf_has(race->flags, RF_IM_FIRE) ||
			(rf_has(race->flags, RF_FLYING) && (mon->hp >= 50))) {
			return true;
		} else {
			return false;
		}
	} else if (square_isrubble(cave, grid)) {
		/* Some monsters move easily through rubble */
		if (rf_has(race->flags, RF_PASS_WALL) ||
			rf_has(race->flags, RF_KILL_WALL) ||
			rf_has(race->flags, RF_SMASH_WALL)) {
			return true;
		} else {
			/* For most monsters, rubble takes more time to cross. */
			return one_in_(2) ? true : false;
		}
	} else if (square_istree(cave, grid)) {
		if (rf_has(race->flags, RF_PASS_WALL)) {
			/* Some monsters can pass right through trees */
			return true;
		} else if (rf_has(race->flags, RF_FLYING)) {
			/* Some monsters can fly over trees */
			return true;
		} else if (rf_has(race->flags, RF_ANIMAL)) {
			/* Some monsters can fly over trees, or know them well */
			return true;
		} else {
			/* For many monsters, trees take more time to cross. */
			return one_in_(2) ? true : false;
		}
	} else if (square_isfall(cave, grid)) {
		/* Have to be able to fly */
		if (rf_has(race->flags, RF_FLYING)) {
			return true;
		} else {
			return false;
		}
	}

	/* Everything else is fine */
	return true;
}

/**
 * Work out if a monster can move through the grid, if necessary bashing 
 * down doors in the way.
 *
 * Returns true if the monster is able to move through the grid.
 */
static bool monster_turn_can_move(struct monster *mon, const char *m_name,
								  struct loc new, bool confused,
								  bool *did_something)
{
	struct monster_lore *lore = get_lore(mon->race);

	/* Always allow an attack upon the player or decoy. */
	if (square_isplayer(cave, new) || square_isdecoyed(cave, new)) {
		return true;
	}

	/* Dangerous terrain in the way */
	if (!confused && monster_hates_grid(mon, new)) {
		return false;
	}

	/* See if we can pass the square; some terrain requires thought */
	if (square_ispassable(cave, new)) {
		/* Check to see if we got to move */
		return monster_can_move_passable(mon, new);
	}

	/* Permanent wall in the way */
	if (square_isperm(cave, new)) {
		if (confused) {
			*did_something = true;
			monster_display_confused_move_msg(mon, m_name, new);
			monster_slightly_stun_by_move(mon);
		}
		return false;
	}

	/* Normal wall, door, or secret door in the way */

	/* There's some kind of feature in the way, so learn about
	 * kill-wall and pass-wall now */
	if (monster_is_visible(mon)) {
		rf_on(lore->flags, RF_PASS_WALL);
		rf_on(lore->flags, RF_KILL_WALL);
		rf_on(lore->flags, RF_SMASH_WALL);
	}

	/* Monster may be able to deal with walls and doors */
	if (rf_has(mon->race->flags, RF_PASS_WALL)) {
		return true;
	} else if (rf_has(mon->race->flags, RF_SMASH_WALL)) {
		/* Remove the wall and much of what's nearby */
		square_smash_wall(cave, new);

		/* Note changes to viewable region */
		player->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

		return true;
	} else if (rf_has(mon->race->flags, RF_KILL_WALL)) {
		/* Remove the wall */
		square_destroy_wall(cave, new);

		/* Note changes to viewable region */
		if (square_isview(cave, new))
			player->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

		return true;
	} else if (square_iscloseddoor(cave, new)|| square_issecretdoor(cave, new)){
		/* Don't allow a confused move to open a door. */
		bool can_open = rf_has(mon->race->flags, RF_OPEN_DOOR) &&
			!confused;
		/* During a confused move, a monster only bashes sometimes. */
		bool can_bash = rf_has(mon->race->flags, RF_BASH_DOOR) &&
			(!confused || one_in_(3));
		bool will_bash = false;

		/* Take a turn */
		if (can_open || can_bash) *did_something = true;

		/* Learn about door abilities */
		if (!confused && monster_is_visible(mon)) {
			rf_on(lore->flags, RF_OPEN_DOOR);
			rf_on(lore->flags, RF_BASH_DOOR);
		}

		/* If creature can open or bash doors, make a choice */
		if (can_open) {
			/* Sometimes bash anyway (impatient) */
			if (can_bash) {
				will_bash = one_in_(2) ? true : false;
			}
		} else if (can_bash) {
			/* Only choice */
			will_bash = true;
		} else {
			/* Door is an insurmountable obstacle */
			if (confused) {
				*did_something = true;
				monster_display_confused_move_msg(mon, m_name, new);
				monster_slightly_stun_by_move(mon);
			}
			return false;
		}

		/* Now outcome depends on type of door */
		if (square_islockeddoor(cave, new)) {
			/* Locked door -- test monster strength against door strength */
			int k = square_door_power(cave, new);
			if (randint0(mon->hp / 10) > k) {
				if (will_bash) {
					msg("%s slams against the door.", m_name);
				} else {
					msg("%s fiddles with the lock.", m_name);
				}

				/* Reduce the power of the door by one */
				square_set_door_lock(cave, new, k - 1);
			}
			if (confused) {
				/* Didn't learn above; apply now since attempted to bash. */
				if (monster_is_visible(mon)) {
					rf_on(lore->flags, RF_BASH_DOOR);
				}
				/* When confused, can stun itself while bashing. */
				monster_slightly_stun_by_move(mon);
			}
		} else {
			/* Closed or secret door -- always open or bash */
			if (square_isview(cave, new))
				player->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

			if (will_bash) {
				square_smash_door(cave, new);

				msg("You hear a door burst open!");
				disturb(player);

				if (confused) {
					/* Didn't learn above; apply since bashed the door. */
					if (monster_is_visible(mon)) {
						rf_on(lore->flags, RF_BASH_DOOR);
					}
					/* When confused, can stun itself while bashing. */
					monster_slightly_stun_by_move(mon);
				}

				/* Fall into doorway */
				return true;
			} else {
				square_open_door(cave, new);
			}
		}
	} else if (confused) {
		*did_something = true;
		monster_display_confused_move_msg(mon, m_name, new);
		monster_slightly_stun_by_move(mon);
	}

	return false;
}

/**
 * Try to break a glyph.
 */
static bool monster_turn_attack_glyph(struct monster *mon, struct loc new)
{
	assert(square_iswarded(cave, new));

	/* Break the ward */
	if (randint1(z_info->glyph_hardness) < mon->race->level) {
		struct trap_kind *rune = lookup_trap("glyph of warding");

		/* Describe observable breakage */
		if (square_isseen(cave, new)) {
			msg("The rune of protection is broken!");
		}

		/* Break the rune */
		assert(rune);
		square_remove_all_traps_of_type(cave, new, rune->tidx);

		return true;
	}

	/* Unbroken ward - can't move */
	return false;
}

/**
 * Try to push past / kill another monster.  Returns true on success.
 */
static bool monster_turn_try_push(struct monster *mon, const char *m_name,
								  struct loc new, bool *dead)
{
	struct monster *mon1 = square_monster(cave, new);
	struct monster_lore *lore = get_lore(mon->race);

	/* Kill weaker monsters */
	int kill_ok = monster_can_kill(mon, new);

	/* Move weaker monsters if they can swap places */
	/* (not in a wall) */
	int move_ok = (monster_can_move(mon, new) &&
				   square_ispassable(cave, mon->grid));

	if (kill_ok || move_ok) {
		/* Get the names of the monsters involved */
		char n_name[80];
		monster_desc(n_name, sizeof(n_name), mon1, MDESC_IND_HID);

		/* Learn about pushing and shoving */
		if (monster_is_visible(mon)) {
			rf_on(lore->flags, RF_KILL_BODY);
			rf_on(lore->flags, RF_MOVE_BODY);
		}

		/* Reveal camouflaged monsters */
		if (monster_is_camouflaged(mon1))
			become_aware(cave, mon1);

		/* Note if visible */
		if (monster_is_visible(mon) && monster_is_in_view(mon))
			msg("%s %s %s.", m_name, kill_ok ? "tramples over" : "pushes past",
				n_name);

		/* Monster ate another monster */
		if (kill_ok)
			delete_monster(cave, new);

		monster_swap(mon->grid, new);

		/* Check for monster traps */
		if (square_ismonstertrap(cave, new)) {
			monster_hit_trap(mon, new, dead);
		}
		return true;
	}

	return false;
}

/**
 * Grab all objects from the grid.
 */
static void monster_turn_grab_objects(struct monster *mon, const char *m_name,
									  struct loc new)
{
	struct monster_lore *lore = get_lore(mon->race);
	struct object *obj;
	bool visible = monster_is_visible(mon);

	/* Learn about item pickup behavior */
	for (obj = square_object(cave, new); obj; obj = obj->next) {
		if (!tval_is_money(obj) && visible) {
			rf_on(lore->flags, RF_TAKE_ITEM);
			rf_on(lore->flags, RF_KILL_ITEM);
			break;
		}
	}

	/* Abort if can't pickup/kill */
	if (!rf_has(mon->race->flags, RF_TAKE_ITEM) &&
		!rf_has(mon->race->flags, RF_KILL_ITEM)) {
		return;
	}

	/* Take or kill objects on the floor */
	obj = square_object(cave, new);
	while (obj) {
		char o_name[80];
		bool safe = obj->artifact ? true : false;
		struct object *next = obj->next;

		/* Skip gold */
		if (tval_is_money(obj)) {
			obj = next;
			continue;
		}

		/* Skip mimicked objects */
		if (obj->mimicking_m_idx) {
			obj = next;
			continue;
		}

		/* Get the object name */
		object_desc(o_name, sizeof(o_name), obj,
			ODESC_PREFIX | ODESC_FULL, player);

		/* React to objects that hurt the monster */
		if (react_to_slay(obj, mon))
			safe = true;

		/* Try to pick up, or crush */
		if (safe) {
			/* Only give a message for "take_item" */
			if (rf_has(mon->race->flags, RF_TAKE_ITEM)
					&& visible
					&& square_isview(cave, new)
					&& !ignore_item_ok(player, obj)) {
				/* Dump a message */
				msg("%s tries to pick up %s, but fails.", m_name, o_name);
			}
		} else if (rf_has(mon->race->flags, RF_TAKE_ITEM)) {
			/*
			 * Make a copy so the original can remain as a
			 * placeholder if the player remembers seeing the
			 * object.
			 */
			struct object *taken = object_new();

			object_copy(taken, obj);
			taken->oidx = 0;
			if (obj->known) {
				taken->known = object_new();
				object_copy(taken->known, obj->known);
				taken->known->oidx = 0;
				taken->known->grid = loc(0, 0);
			}

			/* Try to carry the copy */
			if (monster_carry(cave, mon, taken)) {
				/* Describe observable situations */
				if (square_isseen(cave, new) && !ignore_item_ok(player, obj)) {
					msg("%s picks up %s.", m_name, o_name);
				}

				/* Delete the object */
				square_delete_object(cave, new, obj, true, true);
			} else {
				if (taken->known) {
					object_delete(player->cave, NULL, &taken->known);
				}
				object_delete(cave, player->cave, &taken);
			}
		} else {
			/* Describe observable situations */
			if (square_isseen(cave, new) && !ignore_item_ok(player, obj)) {
				msgt(MSG_DESTROY, "%s crushes %s.", m_name, o_name);
			}

			/* Delete the object */
			square_delete_object(cave, new, obj, true, true);
		}

		/* Next object */
		obj = next;
	}
}


/**
 * Process a monster's turn
 *
 * In several cases, we directly update the monster lore
 *
 * Note that a monster is only allowed to "reproduce" if there
 * are a limited number of "reproducing" monsters on the current
 * level.  This should prevent the level from being "swamped" by
 * reproducing monsters.  It also allows a large mass of mice to
 * prevent a louse from multiplying, but this is a small price to
 * pay for a simple multiplication method.
 *
 * XXX Monster fear is slightly odd, in particular, monsters will
 * fixate on opening a door even if they cannot open it.  Actually,
 * the same thing happens to normal monsters when they hit a door
 *
 * In addition, monsters which *cannot* open or bash down a door
 * will still stand there trying to open it...  XXX XXX XXX
 *
 * Technically, need to check for monster in the way combined
 * with that monster being in a wall (or door?) XXX
 */
static void monster_turn(struct monster *mon)
{
	struct monster_lore *lore = get_lore(mon->race);

	bool did_something = false;

	int i;
	int dir = 0;
	enum monster_stagger stagger;
	bool tracking = false;
	bool dead = false;
	char m_name[80];

	/* Get the monster name */
	monster_desc(m_name, sizeof(m_name), mon,
		MDESC_CAPITAL | MDESC_IND_HID | MDESC_COMMA);

	/* If we're in a web, deal with that */
	if (square_iswebbed(cave, mon->grid)) {
		/* Learn web behaviour */
		if (monster_is_visible(mon)) {
			rf_on(lore->flags, RF_CLEAR_WEB);
			rf_on(lore->flags, RF_PASS_WEB);
		}

		/* If we can pass, no need to clear */
		if (!rf_has(mon->race->flags, RF_PASS_WEB)) {
			/* Learn wall behaviour */
			if (monster_is_visible(mon)) {
				rf_on(lore->flags, RF_PASS_WALL);
				rf_on(lore->flags, RF_KILL_WALL);
			}

			/* Now several possibilities */
			if (rf_has(mon->race->flags, RF_PASS_WALL)) {
				/* Insubstantial monsters go right through */
			} else if (monster_passes_walls(mon)) {
				/* If you can destroy a wall, you can destroy a web */
				struct trap_kind *web = lookup_trap("web");

				assert(web);
				square_remove_all_traps_of_type(cave,
					mon->grid, web->tidx);
			} else if (rf_has(mon->race->flags, RF_CLEAR_WEB)) {
				/* Clearing costs a turn (assume there are no other "traps") */
				struct trap_kind *web = lookup_trap("web");

				assert(web);
				square_remove_all_traps_of_type(cave,
					mon->grid, web->tidx);
				return;
			} else {
				/* Stuck */
				return;
			}
		}
	}

	/* Handle territorial monsters */
	if (rf_has(mon->race->flags, RF_TERRITORIAL) &&
		(level_topography(player->place) != TOP_CAVE)) {
		/* Territorial monsters get a direct energy boost when close to home */
		int from_home =	distance(mon->home, mon->grid);

		/* Step up in units of a fifth of monster detection range */
		for (i = 5; i > 0; i--) {
			if ((from_home * i) > (mon->race->hearing * 5))
				break;
		}

		/* Add some energy */
		mon->energy += (5 - i);

		/* If target is too far away from home, go back */
		if (distance(mon->home, mon->target.grid) >	5 * mon->race->hearing) {
			mon->target.grid = mon->home;
		}
	}

	/* Let other group monsters know about the player */
	monster_group_rouse(cave, mon);

	/* Try to multiply - this can use up a turn */
	if (monster_turn_multiply(mon))
		return;

	/* Monsters can speak.  -originally by TY- */
	if (one_in_(16) && rf_has(mon->race->flags, RF_SPEAKING) &&
		los(cave, mon->grid, player->grid) && !mon->m_timed[MON_TMD_FEAR]) {
		char bravado[80];

		/* Acquire the monster name/poss */
		if (monster_is_visible(mon)) {
			monster_desc(m_name, sizeof(m_name), mon, MDESC_CAPITAL);
		} else {
			my_strcpy(m_name, "It", sizeof(m_name));
		}

		if (!get_rnd_line("bravado.txt", bravado, sizeof(bravado))) {
			msg("%s %s", m_name, bravado);
		}
	} else if (rf_has(mon->race->flags, RF_PLAYER_GHOST)) {
		assert(cave->ghost);
		if (square_isview(cave, mon->grid) && (cave->ghost->string_type == 1)
			&& !cave->ghost->has_spoken && one_in_(3)) {
			/* Player ghosts may have a unique message they can say. */
			char ghost_name[80];

			/* Acquire the monster name/poss.  The player ghost will 
			 * always be identified, to heighten the effect.*/
			monster_desc(ghost_name, sizeof(ghost_name), mon, MDESC_SHOW);

			msg("%s says: '%s'", ghost_name, cave->ghost->string);
			cave->ghost->has_spoken = true;
		}
	}

	/* Attempt a ranged attack */
	if (make_ranged_attack(mon)) return;

	/* Work out what kind of movement to use - random movement or AI */
	stagger = monster_turn_should_stagger(mon);
	if (stagger == NO_STAGGER) {
		/* If there's no sensible move, we're done */
		if (!get_move(mon, &dir, &tracking)) return;
	}

	/* Try to move first in the chosen direction, or next either side of the
	 * chosen direction, or next at right angles to the chosen direction.
	 * Monsters which are tracking by sound or scent will not move if they
	 * can't move in their chosen direction. */
	for (i = 0; i < 5 && !did_something; i++) {
		/* Get the direction (or stagger) */
		int d = (stagger != NO_STAGGER) ? ddd[randint0(8)] : side_dirs[dir][i];

		/* Get the grid to step to or attack */
		struct loc new = loc_sum(mon->grid, ddgrid[d]);

		/* Tracking monsters have their best direction, don't change */
		if ((i > 0) && stagger == NO_STAGGER &&
			!square_isview(cave, mon->grid) && tracking) {
			break;
		}

		/* Check if we can move */
		if (!monster_turn_can_move(mon, m_name, new,
								   stagger == CONFUSED_STAGGER, &did_something))
			continue;

		/* Try to break the glyph if there is one.  This can happen multiple
		 * times per turn because failure does not break the loop */
		if (square_iswarded(cave, new) && !monster_turn_attack_glyph(mon, new))
			continue;

		/* Break a decoy if there is one */
		if (square_isdecoyed(cave, new)) {
			/* Learn about if the monster attacks */
			if (monster_is_visible(mon))
				rf_on(lore->flags, RF_NEVER_BLOW);

			/* Some monsters never attack */
			if (rf_has(mon->race->flags, RF_NEVER_BLOW))
				continue;

			/* Wait a minute... */
			square_destroy_decoy(cave, new);
			did_something = true;
			break;
		}

		/* The player is in the way. */
		if (square_isplayer(cave, new)) {
			if (mon->target.midx == -1) {
				/* Learn about if the monster attacks */
				if (monster_is_visible(mon))
					rf_on(lore->flags, RF_NEVER_BLOW);

				/* Some monsters never attack */
				if (rf_has(mon->race->flags, RF_NEVER_BLOW))
					continue;

				/* Otherwise, attack the player */
				make_attack_normal(mon, player);

				did_something = true;
				break;
			} else {
				/* Don't want to attack, so look for other options */
			}
		} else {
			/* Some monsters never move */
			if (rf_has(mon->race->flags, RF_NEVER_MOVE)) {
				/* Learn about lack of movement */
				if (monster_is_visible(mon))
					rf_on(lore->flags, RF_NEVER_MOVE);

				return;
			}
		}

		/* A monster is in the way, try to push past/kill */
		if (square_monster(cave, new)) {
			if (square_monster(cave, new) == cave_monster(cave,
														  mon->target.midx)) {
				monster_attack_monster(mon,
									   cave_monster(cave, mon->target.midx));
			} else {
				did_something = monster_turn_try_push(mon, m_name, new, &dead);
			}
		} else {
			/* Otherwise we can just move */
			monster_swap(mon->grid, new);

			/* Check for monster traps */
			if (square_ismonstertrap(cave, new)) {
				monster_hit_trap(mon, new, &dead);
			}
			did_something = true;
		}

		/* Monster may have died */
		if (dead) return;

		/* Scan all objects in the grid, if we reached it */
		if (mon == square_monster(cave, new)) {
			monster_turn_grab_objects(mon, m_name, new);
		}
	}

	if (did_something) {
		/* Learn about no lack of movement */
		if (monster_is_visible(mon))
			rf_on(lore->flags, RF_NEVER_MOVE);

		/* Possible disturb */
		if (monster_is_visible(mon) && monster_is_in_view(mon) && 
			OPT(player, disturb_near))
			disturb(player);		
	}

	/* Out of options - monster is paralyzed by fear (unless attacked) */
	if (!did_something && mon->m_timed[MON_TMD_FEAR]) {
		int amount = mon->m_timed[MON_TMD_FEAR];
		mon_clear_timed(mon, MON_TMD_FEAR, MON_TMD_FLG_NOMESSAGE);
		mon_inc_timed(mon, MON_TMD_HOLD, amount, MON_TMD_FLG_NOTIFY);
	}

	/* If we see an unaware monster do something, become aware of it */
	if (did_something && monster_is_camouflaged(mon))
		become_aware(cave, mon);
}


/**
 * ------------------------------------------------------------------------
 * Processing routines that happen to a monster regardless of whether it
 * gets a turn, and/or to decide whether it gets a turn
 * ------------------------------------------------------------------------ */
/**
 * Determine whether a monster is active or passive
 */
static bool monster_check_active(struct monster *mon)
{
	if ((mon->cdis <= mon->race->hearing) && monster_passes_walls(mon)) {
		/* Character is inside scanning range, monster can go straight there */
		mflag_on(mon->mflag, MFLAG_ACTIVE);
	} else if (mon->hp < mon->maxhp) {
		/* Monster is hurt */
		mflag_on(mon->mflag, MFLAG_ACTIVE);
	} else if (square_isview(cave, mon->grid)) {
		/* Monster can "see" the player (checked backwards) */
		mflag_on(mon->mflag, MFLAG_ACTIVE);
	} else if (monster_can_hear(mon)) {
		/* Monster can hear the player */
		mflag_on(mon->mflag, MFLAG_ACTIVE);
	} else if (monster_can_smell(mon)) {
		/* Monster can smell the player */
		mflag_on(mon->mflag, MFLAG_ACTIVE);
	} else if (monster_taking_terrain_damage(cave, mon)) {
		/* Monster is taking damage from the terrain */
		mflag_on(mon->mflag, MFLAG_ACTIVE);
	} else {
		/* Otherwise go passive */
		mflag_off(mon->mflag, MFLAG_ACTIVE);
	}

	return mflag_has(mon->mflag, MFLAG_ACTIVE) ? true : false;
}

/**
 * Wake a monster or reduce its depth of sleep
 *
 * Chance of waking up is dependent only on the player's stealth, but the
 * amount of sleep reduction takes into account the monster's distance from
 * the player.  Currently straight line distance is used; possibly this
 * should take into account dungeon structure.
 */
static void monster_reduce_sleep(struct monster *mon)
{
	int stealth = player->state.skills[SKILL_STEALTH];
	uint32_t player_noise = ((uint32_t) 1) << (30 - stealth);
	uint32_t notice = (uint32_t) randint0(1024);
	struct monster_lore *lore = get_lore(mon->race);

	/* Aggravation - Shadow players win out here */
	if (player_of_has(player, OF_AGGRAVATE) && !player_has(player, PF_SHADOW)) {
		char m_name[80];

		/* Wake the monster, make it aware */
		monster_wake(mon, false, 100);

		/* Get the monster name */
		monster_desc(m_name, sizeof(m_name), mon,
			MDESC_CAPITAL | MDESC_IND_HID | MDESC_COMMA);

		/* Notify the player if aware */
		if (monster_is_obvious(mon)) {
			msg("%s wakes up.", m_name);
			equip_learn_flag(player, OF_AGGRAVATE);
		}
	} else if ((notice * notice * notice) <= player_noise) {
		int sleep_reduction = 1;
		int local_noise = cave->noise.grids[mon->grid.y][mon->grid.x];
		bool woke_up = false;

		/* Test - wake up faster in hearing distance of the player 
		 * Note no dependence on stealth for now */
		if ((local_noise > 0) && (local_noise < 50)) {
			sleep_reduction = (100 / local_noise);
		}

		/* Note a complete wakeup */
		if (mon->m_timed[MON_TMD_SLEEP] <= sleep_reduction) {
			woke_up = true;
		}

		/* Monster wakes up a bit */
		mon_dec_timed(mon, MON_TMD_SLEEP, sleep_reduction, MON_TMD_FLG_NOTIFY);

		/* Update knowledge */
		if (monster_is_obvious(mon)) {
			if (!woke_up && lore->ignore < UCHAR_MAX)
				lore->ignore++;
			else if (woke_up && lore->wake < UCHAR_MAX)
				lore->wake++;
			lore_update(mon->race, lore);
		}
	}
}

/**
 * Process a monster's timed effects, e.g. decrease them.
 *
 * Returns true if the monster is skipping its turn.
 */
static bool process_monster_timed(struct monster *mon)
{
	/* If the monster is asleep or just woke up, then it doesn't act */
	if (mon->m_timed[MON_TMD_SLEEP]) {
		monster_reduce_sleep(mon);
		return true;
	} else {
		/* Awake, active monsters may become aware */
		if (one_in_(10) && mflag_has(mon->mflag, MFLAG_ACTIVE)) {
			mflag_on(mon->mflag, MFLAG_AWARE);
		}
	}

	if (mon->m_timed[MON_TMD_FAST])
		mon_dec_timed(mon, MON_TMD_FAST, 1, 0);

	if (mon->m_timed[MON_TMD_SLOW])
		mon_dec_timed(mon, MON_TMD_SLOW, 1, 0);

	if (mon->m_timed[MON_TMD_HOLD])
		mon_dec_timed(mon, MON_TMD_HOLD, 1, 0);

	if (mon->m_timed[MON_TMD_DISEN])
		mon_dec_timed(mon, MON_TMD_DISEN, 1, 0);

	if (mon->m_timed[MON_TMD_STUN])
		mon_dec_timed(mon, MON_TMD_STUN, 1, MON_TMD_FLG_NOTIFY);

	if (mon->m_timed[MON_TMD_CONF]) {
		mon_dec_timed(mon, MON_TMD_CONF, 1, MON_TMD_FLG_NOTIFY);
	}

	if (mon->m_timed[MON_TMD_CHANGED]) {
		mon_dec_timed(mon, MON_TMD_CHANGED, 1, MON_TMD_FLG_NOTIFY);
	}

	if (mon->m_timed[MON_TMD_FEAR]) {
		int d = randint1(mon->race->level / 10 + 1);
		mon_dec_timed(mon, MON_TMD_FEAR, d, MON_TMD_FLG_NOTIFY);
	}

	/* Always miss turn if held or commanded, one in STUN_MISS_CHANCE chance
	 * of missing if stunned,  */
	if (mon->m_timed[MON_TMD_HOLD] || mon->m_timed[MON_TMD_COMMAND]) {
		return true;
	} else if (mon->m_timed[MON_TMD_STUN]) {
		return one_in_(STUN_MISS_CHANCE);
	} else {
		return false;
	}
}

/**
 * Monster regeneration of HPs.
 */
static void regen_monster(struct monster *mon, int num)
{
	/* Regenerate (if needed) */
	if (mon->hp < mon->maxhp) {
		/* Base regeneration */
		int frac = mon->maxhp / 100;

		/* Minimal regeneration rate */
		if (!frac) frac = 1;

		/* Some monsters regenerate quickly */
		if (rf_has(mon->race->flags, RF_REGENERATE)) frac *= 2;

		/* Multiply by number of regenerations */
		frac *= num;

		/* Regenerate */
		mon->hp += frac;

		/* Do not over-regenerate */
		if (mon->hp > mon->maxhp) mon->hp = mon->maxhp;

		/* Redraw (later) if needed */
		if (player->upkeep->health_who == mon)
			player->upkeep->redraw |= (PR_HEALTH);
	}
}


/**
 * ------------------------------------------------------------------------
 * Monster processing routines to be called by the main game loop
 * ------------------------------------------------------------------------ */
/**
 * Process all the "live" monsters, once per game turn.
 *
 * During each game turn, we scan through the list of all the "live" monsters,
 * (backwards, so we can excise any "freshly dead" monsters), energizing each
 * monster, and allowing fully energized monsters to move, attack, pass, etc.
 *
 * This function and its children are responsible for a considerable fraction
 * of the processor time in normal situations, greater if the character is
 * resting.
 */
void process_monsters(int minimum_energy)
{
	int i;
	int mspeed;

	/* Only process some things every so often */
	bool regen = false;

	/* Regenerate hitpoints and mana every 100 game turns */
	if (turn % 100 == 0)
		regen = true;

	/* Process the monsters (backwards) */
	for (i = cave_monster_max(cave) - 1; i >= 1; i--) {
		struct monster *mon;
		bool moving;

		/* Handle "leaving" */
		if (player->is_dead || player->upkeep->generate_level) break;

		/* Get a 'live' monster */
		mon = cave_monster(cave, i);
		if (!mon->race) continue;

		/* Ignore monsters that have already been handled */
		if (mflag_has(mon->mflag, MFLAG_HANDLED))
			continue;

		/* Not enough energy to move yet */
		if (mon->energy < minimum_energy) continue;

		/* Does this monster have enough energy to move? */
		moving = mon->energy >= z_info->move_energy ? true : false;

		/* Prevent reprocessing */
		mflag_on(mon->mflag, MFLAG_HANDLED);

		/* Handle monster regeneration if requested */
		if (regen)
			regen_monster(mon, 1);

		/* Calculate the net speed */
		mspeed = mon->mspeed;
		if (mon->m_timed[MON_TMD_FAST])
			mspeed += 10;
		if (mon->m_timed[MON_TMD_SLOW]) {
			int slow_level = monster_effect_level(mon, MON_TMD_SLOW);
			mspeed -= (2 * slow_level);
		}

		/* Give this monster some energy */
		mon->energy += turn_energy(mspeed);

		/* End the turn of monsters without enough energy to move */
		if (!moving)
			continue;

		/* Use up "some" energy */
		mon->energy -= z_info->move_energy;

		/* Mimics lie in wait */
		if (monster_is_mimicking(mon)) continue;

		/* Check if the monster is active */
		if (monster_check_active(mon)) {
			/* Process timed effects - skip turn if necessary */
			if (process_monster_timed(mon))
				continue;

			/* Set this monster to be the current actor */
			cave->mon_current = i;

			/* The monster takes its turn */
			monster_turn(mon);

			/*
			 * For symmetry with the player, monster can take
			 * terrain damage after its turn.
			 */
			monster_take_terrain_damage(mon);

			/* Monster is no longer current */
			cave->mon_current = -1;
		}
	}

	/* Update monster visibility after this */
	/* XXX This may not be necessary */
	player->upkeep->update |= PU_MONSTERS;
}

/**
 * Clear 'moved' status from all monsters.
 *
 * Clear noise if appropriate.
 */
void reset_monsters(void)
{
	int i;
	struct monster *mon;

	/* Process the monsters (backwards) */
	for (i = cave_monster_max(cave) - 1; i >= 1; i--) {
		/* Access the monster */
		mon = cave_monster(cave, i);

		/* Monster is ready to go again */
		mflag_off(mon->mflag, MFLAG_HANDLED);
	}
}

/**
 * Allow monsters on a frozen persistent level to recover
 */
void restore_monsters(void)
{
	int i;
	struct monster *mon;

	/* Get the number of turns that have passed */
	int num_turns = turn - cave->turn;

	/* Process the monsters (backwards) */
	for (i = cave_monster_max(cave) - 1; i >= 1; i--) {
		int status, status_red;

		/* Access the monster */
		mon = cave_monster(cave, i);

		/* Regenerate */
		regen_monster(mon, num_turns / 100);

		/* Handle timed effects */
		status_red = num_turns * turn_energy(mon->mspeed) / z_info->move_energy;
		if (status_red > 0) {
			for (status = 0; status < MON_TMD_MAX; status++) {
				if (mon->m_timed[status]) {
					mon_dec_timed(mon, status, status_red, 0);
				}
			}
		}
	}
}
