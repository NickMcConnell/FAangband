/**
 * \file trap.c
 * \brief The trap layer - player traps, runes and door locks
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
#include "effects.h"
#include "game-input.h"
#include "game-world.h"
#include "init.h"
#include "mon-desc.h"
#include "mon-attack.h"
#include "mon-util.h"
#include "obj-knowledge.h"
#include "player-attack.h"
#include "player-quest.h"
#include "player-timed.h"
#include "player-util.h"
#include "trap.h"
#include "ui-input.h"
#include "ui-menu.h"

/**
 * ------------------------------------------------------------------------
 * General trap routines
 * ------------------------------------------------------------------------ */
struct trap_kind *trap_info;

/**
 * Find a trap kind based on its short description
 */
struct trap_kind *lookup_trap(const char *desc)
{
	int i;
	struct trap_kind *closest = NULL;

	/* Look for it */
	for (i = 1; i < z_info->trap_max; i++) {
		struct trap_kind *kind = &trap_info[i];
		if (!kind->name)
			continue;

		/* Test for equality */
		if (streq(desc, kind->desc))
			return kind;

		/* Test for close matches */
		if (!closest && my_stristr(kind->desc, desc))
			closest = kind;
	}

	/* Return our best match */
	return closest;
}

/**
 * Is there a specific kind of trap in this square?
 */
bool square_trap_specific(struct chunk *c, struct loc grid, int t_idx)
{
    struct trap *trap = square_trap(c, grid);
	
    /* First, check the trap marker */
    if (!square_istrap(c, grid))
		return false;
	
    /* Scan the square trap list */
    while (trap) {
		/* We found a trap of the right kind */
		if (trap->t_idx == t_idx)
			return true;
		trap = trap->next;
	}

    /* Report failure */
    return false;
}

/**
 * Is there a trap with a given flag in this square?
 */
bool square_trap_flag(struct chunk *c, struct loc grid, int flag)
{
    struct trap *trap = square_trap(c, grid);

    /* First, check the trap marker */
    if (!square_istrap(c, grid))
		return false;
	
    /* Scan the square trap list */
    while (trap) {
		/* We found a trap with the right flag */
		if (trf_has(trap->flags, flag))
			return true;
		trap = trap->next;
    }

    /* Report failure */
    return false;
}

/**
 * Determine if a trap actually exists in this square.
 *
 * Called with vis = 0 to accept any trap, = 1 to accept only visible
 * traps, and = -1 to accept only invisible traps.
 *
 * Clear the SQUARE_TRAP flag if none exist.
 */
static bool square_verify_trap(struct chunk *c, struct loc grid, int vis)
{
    struct trap *trap = square_trap(c, grid);
    bool trap_exists = false;

    /* Scan the square trap list */
    while (trap) {
		/* Accept any trap */
		if (!vis)
			return true;

		/* Accept traps that match visibility requirements */
		if ((vis == 1) && trf_has(trap->flags, TRF_VISIBLE)) 
			return true;

		if ((vis == -1)  && !trf_has(trap->flags, TRF_VISIBLE)) 
			return true;

		/* Note that a trap does exist */
		trap_exists = true;
    }

    /* No traps in this location. */
    if (!trap_exists) {
		/* No traps */
		sqinfo_off(square(c, grid)->info, SQUARE_TRAP);

		/* Take note */
		square_note_spot(c, grid);
    }

    /* Report failure */
    return false;
}

/**
 * Free memory for all traps on a grid
 */
void square_free_trap(struct chunk *c, struct loc grid)
{
	struct trap *next, *trap = square_trap(c, grid);

	while (trap) {
		next = trap->next;
		mem_free(trap);
		trap = next;
	}
}

/**
 * Remove all traps from a grid.
 *
 * Return true if traps were removed.
 */
bool square_remove_all_traps(struct chunk *c, struct loc grid)
{
	struct trap *trap = square(c, grid)->trap;
	struct trap_kind *rune = lookup_trap("glyph of warding");
	bool were_there_traps = trap == NULL ? false : true;

	assert(square_in_bounds(c, grid));
	while (trap) {
		struct trap *next_trap = trap->next;
		/* Keep count of glyphs of warding */
		if (trap->kind == rune) {
			c->feeling_squares -= (1 << 8);
		}
		mem_free(trap);
		trap = next_trap;
	}

	square_set_trap(c, grid, NULL);

	/* Refresh grids that the character can see */
	if (square_isseen(c, grid)) {
		square_light_spot(c, grid);
	}

	(void)square_verify_trap(c, grid, 0);

	return were_there_traps;
}

/**
 * Remove all traps with the given index.
 *
 * Return true if traps were removed.
 */
bool square_remove_trap(struct chunk *c, struct loc grid, int t_idx_remove)
{
	bool removed = false;

	/* Look at the traps in this grid */
	struct trap *prev_trap = NULL;
	struct trap *trap = square(c, grid)->trap;

	assert(square_in_bounds(c, grid));
	while (trap) {
		struct trap *next_trap = trap->next;

		if (t_idx_remove == trap->t_idx) {
			mem_free(trap);
			removed = true;

			if (prev_trap) {
				prev_trap->next = next_trap;
			} else {
				square_set_trap(c, grid, next_trap);
			}

			break;
		}

		prev_trap = trap;
		trap = next_trap;
	}

	/* Refresh grids that the character can see */
	if (square_isseen(c, grid))
		square_light_spot(c, grid);

	(void)square_verify_trap(c, grid, 0);

	return removed;
}

/**
 * ------------------------------------------------------------------------
 * Player traps
 * ------------------------------------------------------------------------ */
/**
 * Determine if a cave grid is allowed to have player traps in it.
 */
bool square_player_trap_allowed(struct chunk *c, struct loc grid)
{

    /* We currently forbid multiple traps in a grid under normal conditions.
     * If this changes, various bits of code elsewhere will have to change too.
     */
    if (square_istrap(c, grid))
		return false;

    /* We currently forbid traps in a grid with objects. */
    if (square_object(c, grid))
		return false;

    /* Check it's a trappable square */
    return (square_istrappable(c, grid));
}

/**
 * Instantiate a player trap
 */
static int pick_trap(struct chunk *c, int feat, int trap_level)
{
    int i, pick;
	int *trap_probs = NULL;
	int trap_prob_max = 0;

    /* Paranoia */
    if (!feat_is_trap_holding(feat))
		return -1;

    /* No traps in town */
    if (c->depth == 0)
		return -1;

    /* Get trap probabilities */
	trap_probs = mem_zalloc(z_info->trap_max * sizeof(int));
	for (i = 0; i < z_info->trap_max; i++) {
		/* Get this trap */
		struct trap_kind *kind = &trap_info[i];
		trap_probs[i] = trap_prob_max;

		/* Ensure that this is a valid player trap */
		if (!kind->name) continue;
		if (!kind->rarity) continue;
		if (!trf_has(kind->flags, TRF_TRAP)) continue;

		/* Require that trap_level not be too low */
		if (kind->min_depth > trap_level) continue;

		/* Tree? */
		if (feat_is_tree(feat) && !trf_has(kind->flags, TRF_TREE))
			continue;

		/* Floor? */
		if (feat_is_floor(feat) && !trf_has(kind->flags, TRF_FLOOR))
			continue;

		/* Dungeon? */
		if ((level_topography(player->place) != TOP_CAVE) &&
			trf_has(kind->flags, TRF_DUNGEON))
			continue;

		/* Check legality of trapdoors. */
		if (trf_has(kind->flags, TRF_DOWN)) {
			struct level *current = &world->levels[player->place];
			
			/* No trap doors on quest levels */
			if (quest_forbid_downstairs(player->place)) continue;

			/* No trap doors on the deepest level */
			if (!current->down && !underworld_possible(current->index))
				continue;

			/* No trap doors with persistent levels (for now) */
			if (OPT(player, birth_levels_persist))
				continue;
	    }

		/* Trap is okay, store the cumulative probability */
		trap_probs[i] += (100 / kind->rarity);
		trap_prob_max = trap_probs[i];
	}

	/* No valid trap */
	if (trap_prob_max == 0) {
		mem_free(trap_probs);
		return -1;
	}

	/* Pick at random. */
	pick = randint0(trap_prob_max);
	for (i = 0; i < z_info->trap_max; i++) {
		if (pick < trap_probs[i]) {
			break;
		}
	}

	mem_free(trap_probs);

    /* Return our chosen trap */
    return i < z_info->trap_max ? i : -1;
}

/**
 * Make a new trap of the given type.  Return true if successful.
 *
 * We choose a player trap at random if the index is not legal. This means that
 * things which are not player traps must be picked by passing a valid index.
 *
 * This should be the only function that places traps in the dungeon
 * except the savefile loading code.
 */
void place_trap(struct chunk *c, struct loc grid, int t_idx, int trap_level)
{
	struct trap *new_trap;

    /* We've been called with an illegal index; choose a random trap */
    if ((t_idx <= 0) || (t_idx >= z_info->trap_max)) {
		/* Require the correct terrain */
		if (!square_player_trap_allowed(c, grid)) return;

		t_idx = pick_trap(c, square(c, grid)->feat, trap_level);
    }

    /* Failure */
    if (t_idx < 0) return;

	/* Allocate a new trap for this grid (at the front of the list) */
	new_trap = mem_zalloc(sizeof(*new_trap));
	new_trap->next = square_trap(c, grid);
	square_set_trap(c, grid, new_trap);

	/* Set the details */
	new_trap->t_idx = t_idx;
	new_trap->kind = &trap_info[t_idx];
	new_trap->grid = grid;
	new_trap->power = randcalc(new_trap->kind->power, trap_level, RANDOMISE);
	trf_copy(new_trap->flags, trap_info[t_idx].flags);

	/* We created a monster trap */
	if (trf_has(new_trap->flags, TRF_M_TRAP)) {
		player->num_traps++;
	}

	/* Toggle on the trap marker */
	sqinfo_on(square(c, grid)->info, SQUARE_TRAP);

	/* Redraw the grid */
	square_note_spot(c, grid);
	square_light_spot(c, grid);
}

/**
 * Reveal some of the player traps in a square
 */
bool square_reveal_trap(struct chunk *c, struct loc grid, bool always,
						bool domsg)
{
    int found_trap = 0;
	struct trap *trap = square_trap(c, grid);
    
    /* Check there is a player trap */
    if (!square_isplayertrap(c, grid))
		return false;

	/* Scan the grid */
	while (trap) {
		/* Skip non-player traps */
		if (!trf_has(trap->flags, TRF_TRAP)) {
			trap = trap->next;
			continue;
		}
		
		/* Skip traps the player doesn't notice */
		if (!always && player->state.skills[SKILL_SEARCH] < trap->power) {
			trap = trap->next;
			continue;
		}

		/* Trap is invisible */
		if (!trf_has(trap->flags, TRF_VISIBLE)) {
			/* See the trap (actually, see all the traps) */
			trf_on(trap->flags, TRF_VISIBLE);
			square_memorize_traps(c, grid);

			/* We found a trap */
			found_trap++;
		}
		trap = trap->next;
	}

    /* We found at least one trap */
    if (found_trap) {
		/* We want to talk about it */
		if (domsg) {
			if (found_trap == 1)
				msg("You have found a trap.");
			else
				msg("You have found %d traps.", found_trap);
		}

		/* Memorize */
		square_memorize(c, grid);

		/* Redraw */
		square_light_spot(c, grid);
    }

    /* Return true if we found any traps */
    return (found_trap != 0);
}

/**
 * Memorize all the visible traps on a square
 */
void square_memorize_traps(struct chunk *c, struct loc grid)
{
	struct trap *trap = square(c, grid)->trap;
	struct trap *current = NULL;
	if (c != cave) return;

	/* Clear current knowledge */
	square_remove_all_traps(player->cave, grid);

	/* Copy all visible traps to the known cave */
	while (trap) {
		if (square_isvisibletrap(c, grid)) {
			struct trap *next;
			if (current) {
				next = mem_zalloc(sizeof(*next));
				current->next = next;
				current = next;
			} else {
				current = mem_zalloc(sizeof(*current));
				player->cave->squares[grid.y][grid.x].trap = current;
			}
			memcpy(current, trap, sizeof(*trap));
			current->next = NULL;
		}
		trap = trap->next;
	}
}

/**
 * Hit a trap. 
 */
extern void hit_trap(struct loc grid, int delayed)
{
	bool ident = false;
	struct trap *trap;
	struct effect *effect;

	/* The player is safe from all traps */
	if (player_is_trapsafe(player)) return;

	/* Look at the traps in this grid */
	for (trap = square_trap(cave, grid); trap; trap = trap->next) {
		int flag;
		bool saved = false;

		/* Require that trap be capable of affecting the character */
		if (!trf_has(trap->kind->flags, TRF_TRAP)) continue;
		if (trap->timeout) continue;

		if (delayed != trf_has(trap->kind->flags, TRF_DELAY) &&
		    delayed != -1)
			continue;

		/* Disturb the player */
		disturb(player);

		/* Trap immune player learns the rune */
		if (player_of_has(player, OF_TRAP_IMMUNE)) {
			equip_learn_flag(player, OF_TRAP_IMMUNE);
			break;
		}

		/* Give a message */
		if (trap->kind->msg)
			msg("%s", trap->kind->msg);

		/* Test for save due to flag */
		for (flag = of_next(trap->kind->save_flags, FLAG_START);
			 flag != FLAG_END;
			 flag = of_next(trap->kind->save_flags, flag + 1))
			if (player_of_has(player, flag)) {
				saved = true;
				equip_learn_flag(player, flag);
			}

		/* Test for save due to armor */
		if (trf_has(trap->kind->flags, TRF_SAVE_ARMOR)
			&& !check_hit(player, 125))
			saved = true;

		/* Test for save due to saving throw */
		if (trf_has(trap->kind->flags, TRF_SAVE_THROW) &&
			(randint0(100) < player->state.skills[SKILL_SAVE]))
			saved = true;

		/* Save, or fire off the trap */
		if (saved) {
			if (trap->kind->msg_good)
				msg("%s", trap->kind->msg_good);
		} else {
			if (trap->kind->msg_bad)
				msg("%s", trap->kind->msg_bad);
			effect = trap->kind->effect;
			effect_do(effect, source_trap(trap), NULL, &ident, false, 0, 0, 0, NULL);

			/* Do any extra effects */
			if (trap->kind->effect_xtra && one_in_(2)) {
				if (trap->kind->msg_xtra)
					msg("%s", trap->kind->msg_xtra);
				effect = trap->kind->effect_xtra;
				effect_do(effect, source_trap(trap), NULL, &ident, false,
						  0, 0, 0, NULL);
			}
		}

		/* Some traps drop you a dungeon level */
		if (trf_has(trap->kind->flags, TRF_DOWN)) {
			int target_place = player_get_next_place(player->place, "down", 1);
			player_change_place(player,	target_place);
		}

		/* Some traps drop you onto them */
		if (trf_has(trap->kind->flags, TRF_PIT))
			monster_swap(player->grid, trap->grid);

		/* Some traps disappear after activating, all have a chance to */
		if (trf_has(trap->kind->flags, TRF_ONETIME) || one_in_(3)) {
			square_destroy_trap(cave, grid);
			square_forget(cave, grid);
		}

		/* Trap may have gone */
		if (!square_trap(cave, grid)) break;

		/* Trap becomes visible (always XXX) */
		trf_on(trap->flags, TRF_VISIBLE);
	}

    /* Verify traps (remove marker if appropriate) */
    if (square_verify_trap(cave, grid, 0)) {
	/* At least one trap left.  Memorize the visible ones and the grid. */
	square_memorize_traps(cave, grid);
	square_memorize(cave, grid);
    }
    if (square_isseen(cave, grid)) {
	square_light_spot(cave, grid);
    }
}

/**
 * Disable a trap for the specified number of turns
 */
bool square_set_trap_timeout(struct chunk *c, struct loc grid, bool domsg,
							 int t_idx, int time)
{
    bool trap_exists;
	struct trap *current_trap = NULL;

	/* Bounds check */
	assert(square_in_bounds(c, grid));

	/* Look at the traps in this grid */
	current_trap = square(c, grid)->trap;
	while (current_trap) {
		/* Get the next trap (may be NULL) */
		struct trap *next_trap = current_trap->next;

		/* If called with a specific index, skip others */
		if ((t_idx >= 0) && (t_idx != current_trap->t_idx)) {
			if (!next_trap) break;
			current_trap = next_trap;
			continue;
		}

		/* Set the timer */
		current_trap->timeout = time;

		/* Message if requested */
		msg("You have disabled the %s.", current_trap->kind->name);

		/* Replace with the next trap */
		current_trap = next_trap;
    }

    /* Refresh grids that the character can see */
    if (square_isseen(c, grid))
		square_light_spot(c, grid);

    /* Verify traps (remove marker if appropriate) */
    trap_exists = square_verify_trap(c, grid, 0);

    /* Report whether any traps exist in this grid */
    return (!trap_exists);
}

/**
 * Give the remaining time for a trap to be disabled; note it chooses the first
 * appropriate trap on the grid
 */
int square_trap_timeout(struct chunk *c, struct loc grid, int t_idx)
{
	struct trap *current_trap = square(c, grid)->trap;
	while (current_trap) {
		/* Get the next trap (may be NULL) */
		struct trap *next_trap = current_trap->next;

		/* If called with a specific index, skip others */
		if ((t_idx >= 0) && (t_idx != current_trap->t_idx)) {
			if (!next_trap) break;
			current_trap = next_trap;
			continue;
		}

		/* If the timer is set, return the value */
		if (current_trap->timeout)
			return current_trap->timeout;

		/* Replace with the next trap */
		current_trap = next_trap;
    }

	return 0;
}

/**
 * ------------------------------------------------------------------------
 * Door locks
 * ------------------------------------------------------------------------ */
/**
 * Lock a closed door to a given power
 */
void square_set_door_lock(struct chunk *c, struct loc grid, int power)
{
	struct trap_kind *lock = lookup_trap("door lock");
	struct trap *trap;

	/* Verify it's a closed door */
	if (!square_iscloseddoor(c, grid))
		return;

	/* If there's no lock there, add one */
	if (!square_trap_specific(c, grid, lock->tidx))
		place_trap(c, grid, lock->tidx, 0);

	/* Set the power (of all locks - there should be only one) */
	trap = square_trap(c, grid);
	while (trap) {
		if (trap->kind == lock)
			trap->power = power;
		trap = trap->next;
	}
}

/**
 * Return the power of the lock on a door
 */
int square_door_power(struct chunk *c, struct loc grid)
{
	struct trap_kind *lock = lookup_trap("door lock");
	struct trap *trap;

	/* Verify it's a closed door */
	if (!square_iscloseddoor(c, grid))
		return 0;

	/* Is there a lock there? */
	if (!square_trap_specific(c, grid, lock->tidx))
		return 0;

	/* Get the power and return it */
	trap = square_trap(c, grid);
	while (trap) {
		if (trap->kind == lock)
			return trap->power;
		trap = trap->next;
	}

	return 0;
}

/**
 * ------------------------------------------------------------------------
 * Monster traps
 * ------------------------------------------------------------------------ */
/**
 * Check the effect of a monster trap.  Certain traps may be avoided
 * by certain monsters.  Traps may be disarmed by smart monsters.
 * If the trap works, the effects vary depending on trap type. -BR-
 *
 * "death" tells the calling function if the monster is killed by the trap.
 */
void monster_hit_trap(struct monster *mon, struct loc grid, bool *death)
{
	struct monster_race *race = mon->race;
	struct trap *trap = square_trap(cave, grid);
	char m_name[80];
	int skill, dis_chance;

	/* Assume the trap works */
	bool trap_hit = true;

	/* Assume trap is not destroyed */
	bool trap_destroyed = false;

	/* Sanity check */
	assert(square_ismonstertrap(cave, grid));

	/* Get the monster or "it" */
	monster_desc(m_name, sizeof(m_name), mon, MDESC_CAPITAL);

	/* Determine trap-setting skill */
	if (trf_has(trap->flags, TRF_MAGICAL)) {
		/* Use magic disarm skill for magical traps */
		skill = player->state.skills[SKILL_DISARM_MAGIC];
	} else {
		/* Use physical disarm skill for other traps */
		skill = player->state.skills[SKILL_DISARM_PHYS];
	}

	/* Determine avoidance of trap types */
	if (trf_has(trap->flags, TRF_UNIVERSAL)) {
		/* These hit everyone */
	} else if (trf_has(trap->flags, TRF_FLYING) &&
			   !rf_has(race->flags, RF_FLYING) &&
			   !one_in_(3)) {
		/* Non-flying monsters usually avoid netted traps */
		if (monster_is_visible(mon)) {
			msg("%s avoids your %s.", m_name, trap->kind->desc);
		}
		trap_hit = false;
	} else if (trf_has(trap->flags, TRF_IMMATERIAL) &&
			   !rf_has(race->flags, RF_PASS_WALL)) {
		/* Solid creatures ignore immaterial traps */
		if (monster_is_visible(mon)) {
			msg("%s ignores your %s.", m_name, trap->kind->desc);
		}
		trap_hit = false;
	} else if (((!trf_has(trap->flags, TRF_IMMATERIAL) &&
				 rf_has(race->flags, RF_PASS_WALL)) ||
				(!trf_has(trap->flags, TRF_FLYING) &&
				 rf_has(race->flags, RF_FLYING))) &&
			   !one_in_(4)) {
		/* Immaterial and flying creatures avoid most traps */
		if (monster_is_visible(mon)) {
			msg("%s flies over your %s.", m_name, trap->kind->desc);
		}
		trap_hit = false;
	}

	/* Find the monsters base skill at disarming, capped for deep monsters */
	dis_chance = MIN(40 + (2 * race->level), 215);

	/* Smart monsters may attempts to disarm traps which would affect them */
	if (trap_hit && rf_has(race->flags, RF_SMART)) {
		/* Compare monster disarming skill against player trap setting skill */
		if (randint1(dis_chance) > skill - 15) {
			if (monster_is_visible(mon)) {
				msg("%s finds your trap and disarms it.", m_name);
			}

			/* Trap is gone */
			trap_destroyed = true;

			/* Didn't work */
			trap_hit = false;
		}
	}

	/* Monsters can be wary of traps */
	if (trap_hit && mflag_has(mon->mflag, MFLAG_WARY)) {
		/* Avoidance is easier than disarming */
		if (randint1(dis_chance) > (skill - 15) / 2) {
			if (monster_is_visible(mon)) {
				msg("%s avoids your trap.", m_name);
			}

			/* Didn't work */
			trap_hit = false;
		}
	}

	/* I thought traps only affected players! Unfair! */
	if (trap_hit) {
		int adjust = 0;
		struct effect *effect = trap->kind->effect;
		bool ident;

		/* Monster becomes hostile */
		mon->target.midx = -1;

		/* Message for the player */
		if (monster_is_visible(mon)) {
			/* Players sees the monster */
			msg("%s sets off your cunning trap!", m_name);
		} else if (square_isview(cave, grid)) {
			/* Not seen but in line of sight */
			msg("Something sets off your cunning trap!");
		} else if (trf_has(trap->flags, TRF_DAMAGING)) {
			/* Monster is not seen or in LOS (damaging traps only) */
			msg("You hear anguished yells in the distance.");
		}

		/* Adjust power of trap */
		if (mflag_has(mon->mflag, MFLAG_WARY)) {
			/* Monsters can be wary of traps */
			adjust = -60;
		} else if (randint1(skill) > 50 + race->level) {
			/* Trap 'critical' based on disarming skill (if not wary) */
			adjust = 50;
		}

		/* Affect the monster. */
		effect_do(effect, source_trap(trap), NULL, &ident, true, 0, 0, adjust,
			NULL);

		/* Some traps disappear after activating */
		if (trf_has(trap->kind->flags, TRF_ONETIME)) {
			trap_destroyed = true;
		} else if (trf_has(trap->kind->flags, TRF_STURDY)) {
			/* Some traps rarely break */
			if (one_in_(8)) trap_destroyed = true;
		} else {
			/* Most traps have a chance to break */
			if (one_in_(3)) trap_destroyed = true;
		}

		/* Monster may become wary if not dumb (and still alive) */
		if (!rf_has(race->flags, RF_STUPID)	&& (mon->midx) &&
			(one_in_(4) || (mon->hp < mon->maxhp / 2))) {
			mflag_on(mon->mflag, MFLAG_WARY);
		}
	}

	if (trap_destroyed) {
		/* Kill the trap */
		player->num_traps--;
		square_remove_all_traps(cave, grid);
	}

	/* Report death */
	if (!mon->midx) {
		(*death) = true;
	}
}

/**
 * Trap location
 */
static struct loc mtrap_grid;

static char mtrap_tag(struct menu *menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the sval menu
 */
static void mtrap_display(struct menu *menu, int oid, bool cursor, int row,
				   int col, int width)
{
	const struct trap_kind *choice = menu_priv(menu);
	uint8_t attr = (cursor ? COLOUR_L_BLUE : COLOUR_WHITE);
	char *desc = choice[oid].desc;
	my_strcap(desc);

	/* Print it */
	c_put_str(attr, desc, row, col);
}

/**
 * Deal with events on the trap menu
 */
static bool mtrap_action(struct menu *menu, const ui_event *db, int oid)
{
	const struct trap_kind *choice = menu_priv(menu);

	/* Remove the old trap */
	square_remove_all_traps(cave, mtrap_grid);
	player->num_traps--;

	/* Place the new trap */
	place_trap(cave, mtrap_grid, choice[oid].tidx, 0);

	return false;
}


/**
 * Show monster trap long description when browsing
 */
static void mtrap_menu_browser(int oid, void *data, const region *loc)
{
	struct trap_kind *d = data;

	/* Redirect output to the screen */
	text_out_hook = text_out_to_screen;
	text_out_wrap = 60;
	text_out_indent = loc->col - 1;
	text_out_pad = 1;


	clear_from(loc->row + loc->page_rows);
	Term_gotoxy(loc->col, loc->row + loc->page_rows + 1);
	text_out_to_screen(COLOUR_DEEP_L_BLUE, d[oid].text);

	/* Reset */
	text_out_pad = 0;
	text_out_indent = 0;
	text_out_wrap = 0;
}

/**
 * Display list of monster traps.
 */
static bool mtrap_menu(void)
{
	struct menu menu;
	menu_iter menu_f = { mtrap_tag, 0, mtrap_display, mtrap_action, 0 };
	region area = { 15, 1, 48, -1 };
	ui_event evt = { 0 };

	size_t i, count = 0, num = 0;

	struct trap_kind *choice;
	bool basic_skipped = false;

	/* See how many traps available */
	if (player_has(player, PF_EXTRA_TRAP)) {
		num = 1 + (player->lev / 4);
	} else {
		num = 1 + (player->lev / 6);
	}

	/* Create the array */
	choice = mem_zalloc(num * sizeof(struct trap_kind));

	/* Pick out the monster traps in order */
	for (i = 0; i < z_info->trap_max; i++) {
		if (trf_has(trap_info[i].flags, TRF_M_TRAP)) {
			if (!basic_skipped) {
				basic_skipped = true;
				continue;
			}
			memcpy(&choice[count++], &trap_info[i], sizeof(struct trap_kind));
		}
		if (count == num) break; 
	}

	/* Clear space */
	area.page_rows = num + 2;

	/* Return here if there are no traps */
	if (!num) {
		mem_free(choice);
		return false;
	}


	/* Save the screen and clear it */
	screen_save();

	/* Set up the menu */
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu.title = "Choose an advanced monster trap (ESC to cancel):";
	menu_setpriv(&menu, num, choice);
	menu.browse_hook = mtrap_menu_browser;
	menu.flags = MN_DBL_TAP;
	region_erase_bordered(&area);
	menu_layout(&menu, &area);
	prt("", area.row + 1, area.col);

	/* Select an entry */
	evt = menu_select(&menu, 0, true);

	/* Free memory */
	mem_free(choice);

	/* Load screen */
	screen_load();
	return (evt.type != EVT_ESCAPE);
}

/**
 * Turn a basic monster trap into an advanced one -BR-
 */
bool modify_monster_trap(struct loc grid)
{
	if (player->timed[TMD_BLIND] || no_light(player)) {
		msg("You can not see to modify your trap.");
		return false;
	}

	if (player->timed[TMD_CONFUSED] || player->timed[TMD_IMAGE]) {
		msg("You are too confused.");
		return false;
	}

	/* No setting traps while shapeshifted */
	if (player_is_shapechanged(player)) {
		msg("You cannot do this while in %s form.",	player->shape->name);
		if (get_check("Do you want to change back? " )) {
			player_resume_normal_shape(player);
		} else {
			return false;
		}
	}

	mtrap_grid = grid;

	/* Get choice */
	if (mtrap_menu()) {
		/* Notify the player. */
		msg("You modify the monster trap.");
	}

	/* Trap was modified */
	return true;
}


