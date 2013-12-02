/*
 * File: trap.c
 * Purpose: Trap triggering, selection, and placement
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
#include "generate.h"
#include "mapmode.h"
#include "monster.h"
#include "quest.h"
#include "spells.h"
#include "squelch.h"
#include "trap.h"
#include "ui-menu.h"

/**
 * Is there a specific kind of trap in this grid?
 */
bool cave_trap_specific(int y, int x, int t_idx)
{
	int i;

	/* First, check the trap marker */
	if (!sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* We found a trap of the right kind */
			if (t_ptr->t_idx == t_idx)
				return (TRUE);
		}
	}

	/* Report failure */
	return (FALSE);
}

/**
 * Is there a trap with a given flag in this grid?
 */
bool cave_trap_flag(int y, int x, int flag)
{
	int i;

	/* First, check the trap marker */
	if (!sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* We found a trap with the right flag */
			if (trf_has(t_ptr->flags, flag))
				return (TRUE);
		}
	}

	/* Report failure */
	return (FALSE);
}

/**
 * Check for a basic monster trap 
 */
bool cave_basic_monster_trap(int y, int x)
{
	/* Look for a monster trap */
	return (cave_trap_specific(y, x, MTRAP_BASE));
}

/**
 * Check for an advanced monster trap 
 */
bool cave_advanced_monster_trap(int y, int x)
{
	/* Look for a monster trap */
	return (cave_trap_flag(y, x, TRF_M_TRAP) &&
			!cave_trap_specific(y, x, MTRAP_BASE));
}

/**
 * Check for any monster trap 
 */
bool cave_monster_trap(int y, int x)
{
	/* Look for a monster trap */
	return (cave_trap_flag(y, x, TRF_M_TRAP));
}

/**
 * Return the index of a monster trap 
 */
int monster_trap_idx(int y, int x)
{
	int i;

	if (!cave_monster_trap(y, x))
		return -1;

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a monster trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x) &&
			trf_has(t_ptr->flags, TRF_M_TRAP))
			return (i);
	}

	/* Paranoia */
	return -1;
}

/**
 * Determine if a trap actually exists in this grid.
 *
 * Called with vis = 0 to accept any trap, = 1 to accept only visible
 * traps, and = -1 to accept only invisible traps.
 *
 * Clear the SQUARE_TRAP flag if none exist.
 */
static bool verify_trap(int y, int x, int vis)
{
	int i;
	bool trap = FALSE;

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Accept any trap */
			if (!vis)
				return (TRUE);

			/* Accept traps that match visibility requirements */
			if (vis == 1) {
				if (trf_has(t_ptr->flags, TRF_VISIBLE))
					return (TRUE);
			}

			if (vis == -1) {
				if (!trf_has(t_ptr->flags, TRF_VISIBLE))
					return (TRUE);
			}

			/* Note that a trap does exist */
			trap = TRUE;
		}
	}

	/* No traps in this location. */
	if (!trap) {
		/* No traps */
		sqinfo_off(cave_info[y][x], SQUARE_TRAP);

		/* No reason to mark this grid, ... */
		sqinfo_off(cave_info[y][x], SQUARE_MARK);

		/* ... unless certain conditions apply */
		note_spot(y, x);
	}

	/* Report failure */
	return (FALSE);
}

/**
 * Is there a visible trap in this grid?
 */
bool cave_visible_trap(int y, int x)
{
	/* Look for a visible trap */
	return (cave_trap_flag(y, x, TRF_VISIBLE));
}

/**
 * Is there an invisible trap in this grid?
 */
bool cave_invisible_trap(int y, int x)
{
	/* First, check the trap marker */
	if (!sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Verify trap, require that it be invisible */
	return (verify_trap(y, x, -1));
}

/**
 * Is there a player trap in this grid?
 */
bool cave_player_trap(int y, int x)
{
	/* Look for a player trap */
	return (cave_trap_flag(y, x, TRF_TRAP));
}

/**
 * Return the index of any visible trap 
 */
int visible_trap_idx(int y, int x)
{
	int i;

	if (!cave_visible_trap(y, x))
		return -1;

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a visible trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x) &&
			trf_has(t_ptr->flags, TRF_VISIBLE))
			return (i);
	}

	/* Paranoia */
	return -1;
}

/**
 * Is there a web in this grid?
 */
bool cave_web(int y, int x)
{
	return (cave_trap_specific(y, x, OBST_WEB));
}



/**
 * Get the graphics of a listed trap.
 *
 * We should probably have better handling of stacked traps, but that can
 * wait until we do, in fact, have stacked traps under normal conditions.
 *
 */
bool get_trap_graphics(int t_idx, int *a, wchar_t * c,
					   bool require_visible)
{
	trap_type *t_ptr = &trap_list[t_idx];

	/* Trap is visible, or we don't care */
	if (!require_visible || trf_has(t_ptr->flags, TRF_VISIBLE)) {
		/* Get the graphics */
		*a = t_ptr->kind->x_attr;
		*c = t_ptr->kind->x_char;

		/* We found a trap */
		return (TRUE);
	}

	/* No traps found with the requirement */
	return (FALSE);
}


/**
 * Reveal some of the traps in a grid
 */
bool reveal_trap(int y, int x, int chance, bool domsg)
{
	int i;
	int found_trap = 0;

	/* Check the trap marker */
	if (!sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Trap is invisible */
			if (!trf_has(t_ptr->flags, TRF_VISIBLE)) {
				/* See the trap */
				trf_on(t_ptr->flags, TRF_VISIBLE);
				sqinfo_on(cave_info[y][x], SQUARE_MARK);

				/* We found a trap */
				found_trap++;

				/* If chance is < 100, sometimes stop */
				if ((chance < 100) && (randint1(100) > chance))
					break;
			}
		}
	}

	/* We found at least one trap (or loose rock) */
	if (found_trap) {
		/* We want to talk about it */
		if (domsg) {
			if (found_trap == 1)
				msg("You have found a trap.");
			else
				msg("You have found %d traps.", found_trap);
		}

		/* Memorize */
		sqinfo_on(cave_info[y][x], SQUARE_MARK);

		/* Redraw */
		light_spot(y, x);
	}

	/* Return TRUE if we found any traps */
	return (found_trap != 0);
}

/**
 * Hide a particular trap
 */
void hide_trap_idx(int idx)
{
	trap_type *t_ptr;

	/* Paranoia */
	if ((idx < 0) || (idx >= trap_max))
		return;

	/* Access the trap */
	t_ptr = &trap_list[idx];

	/* Hide it if it's a player trap */
	if (trf_has(t_ptr->flags, TRF_TRAP))
		trf_off(t_ptr->flags, TRF_VISIBLE);

	return;
}

/**
 * Count the number of player traps in this location.
 *
 * Called with vis = 0 to accept any trap, = 1 to accept only visible
 * traps, and = -1 to accept only invisible traps.
 */
int num_traps(int y, int x, int vis)
{
	int i, num;

	/* Scan the current trap list */
	for (num = 0, i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find all traps in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Require that trap be capable of affecting the character */
			if (!trf_has(t_ptr->kind->flags, TRF_TRAP))
				continue;

			/* Require correct visibility */
			if (vis >= 1) {
				if (trf_has(t_ptr->flags, TRF_VISIBLE))
					num++;
			} else if (vis <= -1) {
				if (!trf_has(t_ptr->flags, TRF_VISIBLE))
					num++;
			} else {
				num++;
			}
		}
	}

	/* Return the number of traps */
	return (num);
}

/**
 * Hack -- instantiate a trap
 *
 * This function has been altered in Oangband to (in a hard-coded fashion) 
 * control trap level. -LM-
 */
static int pick_trap(int feat, int trap_level)
{
	int trap = 0;
	feature_type *f_ptr = &f_info[feat];

	trap_kind *trap_ptr;
	bool trap_is_okay = FALSE;

	/* Paranoia */
	if (!tf_has(f_ptr->flags, TF_TRAP))
		return -1;

	/* Try to create a trap appropriate to the level.  Make certain that at
	 * least one trap type can be made on any possible level. -LM- */
	while (!trap_is_okay) {
		/* Pick at random. */
		trap = TRAP_HEAD + randint0(TRAP_TAIL - TRAP_HEAD + 1);

		/* Get this trap */
		trap_ptr = &trap_info[trap];

		/* Require that trap_level not be too low */
		if (trap_ptr->min_depth > trap_level)
			continue;

		/* Assume legal until proven otherwise. */
		trap_is_okay = TRUE;

		/* Tree? */
		if (tf_has(f_ptr->flags, TF_TREE) &&
			!trf_has(trap_ptr->flags, TRF_TREE))
			trap_is_okay = FALSE;

		/* Floor? */
		if (tf_has(f_ptr->flags, TF_FLOOR) &&
			!trf_has(trap_ptr->flags, TRF_FLOOR))
			trap_is_okay = FALSE;

		/* Check legality. */
		switch (trap) {
			/* trap doors */
		case TRAP_TRAPDOOR:
			{
				/* Hack -- no trap doors on quest levels */
				if (is_quest(p_ptr->stage))
					trap_is_okay = FALSE;

				/* Hack -- no trap doors at the bottom of dungeons */
				if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
					&& (!stage_map[p_ptr->stage][DOWN]))
					trap_is_okay = FALSE;

				/* Trap doors only in dungeons or normal wilderness */
				if (stage_map[p_ptr->stage][STAGE_TYPE] > CAVE)
					trap_is_okay = FALSE;

				/* Trap doors only in dungeons in ironman */
				if (MODE(IRONMAN)
					&& (stage_map[p_ptr->stage][STAGE_TYPE] < CAVE))
					trap_is_okay = FALSE;

				break;
			}

		case TRAP_PIT:
		case TRAP_DART:
		case TRAP_SPOT:
		case TRAP_GAS:
		case TRAP_SUMMON:
		case TRAP_ALTER:
		case TRAP_HEX:
		case TRAP_PORTAL:
		case TRAP_MURDER:
		case TRAP_BRANCH:
			{
				/* No special restrictions */
				break;
			}

			/* Any other selection is not defined. */
		default:
			{
				trap_is_okay = FALSE;

				break;
			}
		}
	}

	/* Return our chosen trap */
	return (trap);
}



/**
 * Determine if a cave grid is allowed to have traps in it.
 */
bool cave_trap_allowed(int y, int x)
{
	/*
	 * We currently forbid multiple traps in a grid under normal conditions.
	 * If this changes, various bits of code elsewhere will have to change too.
	 */
	if (sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Check the feature trap flag */
	return (tf_has(f_info[cave_feat[y][x]].flags, TF_TRAP));
}

/**
 * Make a new trap of the given type.  Return TRUE if successful.
 *
 * We choose a trap at random if the index is not legal.
 *
 * This should be the only function that places traps in the dungeon
 * except the savefile loading code.
 */
bool place_trap(int y, int x, int t_idx, int trap_level)
{
	int i;

	/* Require the correct terrain */
	if (!cave_trap_allowed(y, x))
		return (FALSE);

	/* Hack -- don't use up all the trap slots during dungeon generation */
	if (!character_dungeon) {
		if (trap_max > z_info->l_max - 50)
			return (FALSE);
	}

	/* We've been called with an illegal index; choose a random trap */
	if ((t_idx <= 0) || (t_idx >= z_info->trap_max)) {
		t_idx = pick_trap(cave_feat[y][x], trap_level);
	}

	/* Note failure */
	if (t_idx < 0)
		return (FALSE);


	/* Scan the entire trap list */
	for (i = 1; i < z_info->l_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* This space is available */
		if (!t_ptr->t_idx) {
			/* Fill in the trap index */
			t_ptr->t_idx = t_idx;
			t_ptr->kind = &trap_info[t_idx];

			/* Fill in the trap details */
			t_ptr->fy = y;
			t_ptr->fx = x;

			trf_copy(t_ptr->flags, trap_info[t_ptr->t_idx].flags);

			/* Adjust trap count if necessary */
			if (i + 1 > trap_max)
				trap_max = i + 1;

			/* We created a rune */
			if (trf_has(t_ptr->flags, TRF_RUNE))
				num_runes_on_level[t_idx - 1]++;

			/* We created a monster trap */
			if (trf_has(t_ptr->flags, TRF_M_TRAP))
				num_trap_on_level++;

			/* Toggle on the trap marker */
			sqinfo_on(cave_info[y][x], SQUARE_TRAP);

			/* Redraw the grid */
			light_spot(y, x);

			/* Report success */
			return (TRUE);
		}
	}

	/* No luck -- report failure */
	return (FALSE);
}

/**
 * Determine if a trap affects the player.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_trap_hit(int power)
{
	int k, ac;
	bool hit = FALSE;

	/* This function is called from the trap attack code, which generally uses
	 * the simple RNG.  We temporarily switch over to the complex RNG for true
	 * randomness. - LM- */
	Rand_quick = FALSE;

	/* Percentile dice */
	k = randint0(100);

	/* 5% minimum chance to hit, 5% minimum chance to miss */
	if (k < 10)
		hit = (k < 5);

	/* Total armor */
	ac = p_ptr->state.ac + p_ptr->state.to_a;

	/* Power competes against Armor */
	if ((power > 0) && (randint1(power) >= (ac * 3 / 4)))
		hit = TRUE;

	/* Resume use of the simple RNG. */
	Rand_quick = TRUE;

	/* Return hit or miss. */
	return (hit);

}



/**
 * Handle player hitting a real trap.  Rewritten in Oangband to allow a 
 * greater variety of traps, with effects controlled by dungeon level.  
 * To allow a trap to choose one of a variety of effects consistantly, 
 * the quick RNG is often used, and xy coordinates input as a seed value. 
 */
void hit_trap_aux(int y, int x, int trap)
{
	int i, j, k, num;
	int dam = 0;

	int nastyness, selection;

	trap_type *t_ptr = &trap_list[trap];

	const char *name = t_ptr->kind->name;

	/* Use the "simple" RNG to insure that traps are consistant. */
	Rand_quick = TRUE;

	/* Use the coordinates of the trap to seed the RNG. */
	Rand_value = y * x;

	/* Disturb the player */
	disturb(0, 0);

	/* Analyze XXX XXX XXX */
	switch (t_ptr->t_idx) {
		/* trap door. */
	case TRAP_TRAPDOOR:
		{
			Rand_quick = FALSE;

			/* Paranoia -NRM- */
			if (((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
				 || (stage_map[p_ptr->stage][STAGE_TYPE] == VALLEY))
				&& (!stage_map[p_ptr->stage][DOWN])) {
				sqinfo_off(cave_info[y][x], SQUARE_MARK);
				remove_trap(y, x, FALSE, trap);
				msg("The trap fails!");
				break;
			}


			msg("You fall through a trap door!");
			if (p_ptr->state.ffall) {
				notice_obj(OF_FEATHER, 0);
				msg("You float gently down to the next level.");
			} else {
				dam = damroll(2, 8);
				take_hit(dam, name);
			}
			/* Remember where we came from */
			p_ptr->last_stage = p_ptr->stage;

			if (!stage_map[p_ptr->stage][DOWN]) {
				/* Set the ways forward and back */
				stage_map[UNDERWORLD_STAGE][UP] = p_ptr->stage;
				stage_map[p_ptr->stage][DOWN] = UNDERWORLD_STAGE;
				stage_map[UNDERWORLD_STAGE][DEPTH] = p_ptr->depth + 1;
			}

			/* New stage */
			p_ptr->stage = stage_map[p_ptr->stage][DOWN];

			/* New depth */
			p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

			/* Leaving */
			p_ptr->leaving = TRUE;

			Rand_quick = TRUE;

			break;
		}

		/* pits. */
	case TRAP_PIT:
		{
			/* determine how dangerous the trap is allowed to be. */
			nastyness = randint1(p_ptr->depth);
			if (randint1(20) == 1)
				nastyness += 20;
			else if (randint1(5) == 1)
				nastyness += 10;

			/* Player is now in pit. */
			monster_swap(p_ptr->py, p_ptr->px, y, x);

			/* Center on player. */
			y = p_ptr->py;
			x = p_ptr->px;

			/* pit of daggers. */
			if ((nastyness > 80) && (randint1(3) != 3)) {
				msg("You fall into a pit of daggers!");

				if (p_ptr->state.ffall) {
					notice_obj(OF_FEATHER, 0);
					msg("You float gently to the floor of the pit.");
					msg("You carefully avoid setting off the daggers.");
				}

				else {
					/* a trap of morgul. */
					if (randint1(6) == 1) {
						Rand_quick = FALSE;


						msg("A single coldly gleaming dagger pierces you deeply!");
						msg("You feel a deadly chill slowly withering your soul.");

						/* activate the Black Breath. */
						p_ptr->black_breath = TRUE;

						/* lots of damage. */
						dam = damroll(20, 15);

						/* undead may be attracted. */
						if (randint1(2) == 1) {
							msg("Undead suddenly appear and call you to them!");

							k = randint1(3) + 2;
							for (i = 0; i < k; i++) {
								summon_specific(y, x, FALSE, p_ptr->depth,
												SUMMON_UNDEAD);
							}
						}

						/* morgul-traps are one-time only. */
						remove_trap(y, x, FALSE, trap);

						Rand_quick = TRUE;
					}

					else {
						Rand_quick = FALSE;

						/* activate the ordinary daggers. */
						msg("Daggers pierce you everywhere!");

						k = randint1(10) + 5;
						for (i = 0; i < k; i++) {
							dam += damroll(3, 4);
						}

						Rand_quick = TRUE;
					}

					/* cut the player. */
					(void) inc_timed(TMD_CUT, randint1(dam), TRUE);

					/* Take the damage. */
					take_hit(dam, name);
				}
			}

			/* poisoned spiked pit. */
			else if ((nastyness > 55) && (randint1(3) != 3)) {
				msg("You fall into a spiked pit!");

				if (p_ptr->state.ffall) {
					notice_obj(OF_FEATHER, 0);
					msg("You float gently to the floor of the pit.");
					msg("You carefully avoid touching the spikes.");
				}

				else {
					Rand_quick = FALSE;

					/* Base damage */
					dam = damroll(2, 6);

					/* Extra spike damage */
					if (randint0(100) < 85) {
						bool was_poisoned;

						msg("You are impaled on poisonous spikes!");

						dam = dam * (randint1(6) + 3);
						(void) inc_timed(TMD_CUT, randint1(dam), TRUE);

						was_poisoned = pois_hit(dam);

						if (!was_poisoned)
							msg("The poison does not affect you!");
					}

					/* Take the damage */
					take_hit(dam, name);

					Rand_quick = TRUE;
				}
			}

			/* spiked pit. */
			else if ((nastyness > 30) && (randint1(3) != 3)) {
				msg("You fall into a spiked pit!");

				if (p_ptr->state.ffall) {
					notice_obj(OF_FEATHER, 0);
					msg("You float gently to the floor of the pit.");
					msg("You carefully avoid touching the spikes.");
				}

				else {
					Rand_quick = FALSE;

					/* Base damage */
					dam = damroll(2, 6);

					/* Extra spike damage */
					if (randint0(100) < 85) {
						msg("You are impaled!");

						dam = dam * (2 + randint1(4));
						(void) inc_timed(TMD_CUT, randint1(dam), TRUE);
					}

					/* Take the damage */
					take_hit(dam, name);

					Rand_quick = TRUE;
				}
			}

			/* ordinary pit in all other cases. */
			else {
				msg("You fall into a pit!");
				if (p_ptr->state.ffall) {
					notice_obj(OF_FEATHER, 0);
					msg("You float gently to the bottom of the pit.");
				} else {
					Rand_quick = FALSE;

					dam = damroll(2, 6);
					take_hit(dam, name);

					Rand_quick = TRUE;
				}
			}

			/* Change to pit terrain */
			remove_trap(y, x, FALSE, trap);
			cave_set_feat(y, x, FEAT_PIT);

			break;
		}

		/* stat-reducing dart traps. */
	case TRAP_DART:
		{
			/* decide if the dart hits. */
			if (check_trap_hit(50 + p_ptr->depth)) {
				/* select a stat to drain. */
				selection = randint0(6);

				Rand_quick = FALSE;

				msg("A small dart hits you!");
				dam = damroll(1, 4);
				take_hit(dam, name);

				/* Determine how dangerous the trap is allowed to be. */
				nastyness = randint1(p_ptr->depth);

				/* decide how much to drain the stat by. */
				if ((nastyness > 50) && (randint1(3) == 1)) {
					num = randint1(4);
				} else
					num = 1;

				/* drain the stat. */
				for (i = 0; i < num; i++) {
					(void) do_dec_stat(selection);
				}

				Rand_quick = TRUE;
			} else {
				msg("A small dart barely misses you.");
			}
			break;
		}

		/* discolored spots. */
	case TRAP_SPOT:
		{
			/* determine how dangerous the trap is allowed to be. */
			nastyness = randint1(p_ptr->depth);
			if (randint1(5) == 1)
				nastyness += 10;

			/* pick a elemental attack type. */
			selection = randint1(4);


			/* electicity trap. */
			if (selection == 1) {
				if ((nastyness >= 50) && (randint1(2) == 1)) {
					Rand_quick = FALSE;

					msg("You are struck by lightning!");
					dam = damroll(6, 30);

					Rand_quick = TRUE;
				} else {
					Rand_quick = FALSE;

					msg("You get zapped!");
					dam = damroll(4, 8);

					Rand_quick = TRUE;
				}
				Rand_quick = FALSE;
				elec_dam(dam, "an electricity trap");
				Rand_quick = TRUE;

			}

			/* frost trap. */
			if (selection == 2) {
				if ((nastyness >= 50) && (randint1(2) == 1)) {
					Rand_quick = FALSE;

					msg("You are lost within a blizzard!");
					dam = damroll(6, 30);

					Rand_quick = TRUE;
				} else {
					Rand_quick = FALSE;

					msg("You are coated in frost!");
					dam = damroll(4, 8);

					Rand_quick = TRUE;
				}
				Rand_quick = FALSE;
				cold_dam(dam, "a frost trap");
				Rand_quick = TRUE;
			}

			/* fire trap. */
			if (selection == 3) {
				if ((nastyness >= 50) && (randint1(2) == 1)) {
					Rand_quick = FALSE;

					msg("You are enveloped in a column of fire!");
					dam = damroll(6, 30);

					Rand_quick = TRUE;
				} else {
					Rand_quick = FALSE;

					msg("You are surrounded by flames!");
					dam = damroll(4, 8);

					Rand_quick = TRUE;
				}
				Rand_quick = FALSE;
				fire_dam(dam, "a fire trap");
				Rand_quick = TRUE;
			}

			/* acid trap. */
			if (selection == 4) {
				if ((nastyness >= 50) && (randint1(2) == 1)) {
					Rand_quick = FALSE;

					msg("A cauldron of acid is tipped over your head!");
					dam = damroll(6, 30);

					Rand_quick = TRUE;
				} else {
					Rand_quick = FALSE;

					msg("You are splashed with acid!");
					dam = damroll(4, 8);

					Rand_quick = TRUE;
				}
				Rand_quick = FALSE;
				acid_dam(dam, "an acid trap");
				Rand_quick = TRUE;
			}

			break;
		}

		/* gas traps. */
	case TRAP_GAS:
		{
			selection = randint1(4);

			/* blinding trap. */
			if (selection == 1) {
				msg("You are surrounded by a black gas!");
				if (!p_ptr->state.no_blind) {
					Rand_quick = FALSE;

					(void) inc_timed(TMD_BLIND, randint0(30) + 15, TRUE);

					Rand_quick = TRUE;
				}
			} else
				notice_obj(OF_SEEING, 0);

			/* confusing trap. */
			if (selection == 2) {
				msg("You are surrounded by a gas of scintillating colors!");
				if (!p_resist_good(P_RES_CONFU)) {
					Rand_quick = FALSE;

					(void) inc_timed(TMD_CONFUSED, randint0(20) + 10,
									 TRUE);

					Rand_quick = TRUE;
				} else
					notice_other(IF_RES_CONFU, 0);
			}

			/* poisoning trap. */
			if (selection == 3) {
				msg("You are surrounded by a pungent green gas!");

				Rand_quick = FALSE;

				pois_hit(25);

				Rand_quick = TRUE;
			}

			/* sleeping trap. */
			if (selection == 4) {
				msg("You are surrounded by a strange white mist!");
				if (!p_ptr->state.free_act) {
					(void) inc_timed(TMD_PARALYZED, randint0(10) + 5,
									 TRUE);
				} else
					notice_obj(OF_FREE_ACT, 0);
			}

			break;
		}

		/* summoning traps. */
	case TRAP_SUMMON:
		{
			sound(MSG_SUM_MONSTER);
			/* sometimes summon thieves. */
			if ((p_ptr->depth > 8) && (randint1(5) == 1)) {
				msg("You have aroused a den of thieves!");

				Rand_quick = FALSE;

				num = 2 + randint1(3);
				for (i = 0; i < num; i++) {
					(void) summon_specific(y, x, FALSE, p_ptr->depth,
										   SUMMON_THIEF);
				}

				Rand_quick = TRUE;
			}

			/* sometimes summon a nasty unique. */
			else if (randint1(8) == 1) {
				msg("You are enveloped in a cloud of smoke!");

				Rand_quick = FALSE;

				(void) summon_specific(y, x, FALSE, p_ptr->depth + 5,
									   SUMMON_UNIQUE);

				Rand_quick = TRUE;
			}

			/* otherwise, the ordinary summon monsters. */
			else {
				msg("You are enveloped in a cloud of smoke!");

				Rand_quick = FALSE;

				num = 2 + randint1(3);
				for (i = 0; i < num; i++) {
					(void) summon_specific(y, x, FALSE, p_ptr->depth, 0);
				}

				Rand_quick = TRUE;
			}

			/* these are all one-time traps. */
			remove_trap(y, x, FALSE, trap);

			break;
		}

		/* dungeon alteration traps. */
	case TRAP_ALTER:
		{
			/* determine how dangerous the trap is allowed to be. */
			nastyness = randint1(p_ptr->depth);
			if (randint1(5) == 1)
				nastyness += 10;

			/* make room for alterations. */
			remove_trap(y, x, FALSE, trap);

			/* Everything truely random from here on. */
			Rand_quick = FALSE;

			/* dungeon destruction trap. */
			if ((nastyness > 60) && (randint1(12) == 1)) {
				msg("A ear-splitting howl shatters your mind as the dungeon is smashed by hammer blows!");

				(void) destroy_level(FALSE);

				/* the player is hard-hit. */
				(void) inc_timed(TMD_CONFUSED, randint0(20) + 10, TRUE);
				(void) inc_timed(TMD_BLIND, randint0(30) + 15, TRUE);
				(void) inc_timed(TMD_STUN, randint1(50) + 50, TRUE);
				dam = damroll(15, 15);
				take_hit(dam, name);
			}

			/* earthquake trap. */
			else if ((nastyness > 20) && (randint1(4) == 1)) {
				msg("A tremor shakes the earth around you");
				earthquake(y, x, 10, FALSE);
			}

			/* falling rock trap. */
			else if ((nastyness > 4) && (randint1(2) == 1)) {
				msg("A rock falls on your head.");
				dam = damroll(2, 10);
				take_hit(dam, name);

				(void) inc_timed(TMD_STUN, randint1(10) + 10, TRUE);
			}

			/* a few pebbles. */
			else {
				msg("A bunch of pebbles rain down on you.");
				dam = damroll(1, 8);
				take_hit(dam, name);
			}

			Rand_quick = TRUE;

			break;
		}

		/* various char and equipment-alteration traps, lumped together to
		 * avoid any one effect being too common (some of them can be rather
		 * nasty). */
	case TRAP_HEX:
		{
			/* determine how dangerous the trap is allowed to be. */
			nastyness = randint0(100);

			/* these are all one-time traps. */
			remove_trap(y, x, FALSE, trap);

			/* Everything truely random from here on. */
			Rand_quick = FALSE;

			/* trap of drain wands. */
			if (nastyness < 15) {
				/* Hold the object information. */
				object_type *o_ptr;

				/* Find an item */
				for (i = 0; i < 20; i++) {
					/* Pick an item */
					i = randint0(INVEN_PACK - p_ptr->pack_size_reduce);

					/* Obtain the item */
					o_ptr = &p_ptr->inventory[i];

					/* use "num" to decide if a item can be uncharged.  By
					 * default, assume it can't. */
					num = 0;

					/* Skip non-objects */
					if (!o_ptr->k_idx)
						continue;

					/* Drain charged wands/staffs/rods */
					if ((o_ptr->tval == TV_STAFF)
						|| (o_ptr->tval == TV_WAND)
						|| (o_ptr->tval == TV_ROD)) {
						/* case of charged wands/staffs. */
						if (((o_ptr->tval == TV_STAFF)
							 || (o_ptr->tval == TV_WAND)) && (o_ptr->pval))
							num = 1;

						/* case of charged rods. */
						if ((o_ptr->tval == TV_ROD)
							&& (o_ptr->timeout < randcalc(o_ptr->time, 0,
														  MINIMISE)))
							num = 1;


						if (num == 1) {
							/* Message */
							msg("Energy drains from your pack!");

							/* Uncharge */
							if ((o_ptr->tval == TV_STAFF)
								|| (o_ptr->tval == TV_WAND))
								o_ptr->pval = 0;

							if (o_ptr->tval == TV_ROD)
								o_ptr->timeout =
									randcalc(o_ptr->time, 0,
											 RANDOMISE) * o_ptr->number *
									2;


							/* Combine / Reorder the pack */
							p_ptr->notice |= (PN_COMBINE | PN_REORDER);

							/* not more than one inventory slot effected. */
							break;
						} else
							continue;
					}
				}
			}

			/* trap of forgetting. */
			else if (nastyness < 35) {
				if (check_save(100)) {
					msg("You hang on to your memories!");
				} else if (lose_all_info()) {
					msg("Your memories fade away.");
				}
			}

			/* trap of alter reality. */
			else if (nastyness < 50) {
				if (MODE(IRONMAN))
					msg("Nothing happens.");
				else {
					msg("The world changes!");

					/* Leaving */
					p_ptr->leaving = TRUE;
				}
			}

			/* trap of remold player. */
			else if (nastyness < 75) {
				int max1, cur1, max2, cur2, ii, jj;

				msg("You feel yourself being twisted by wild magic!");

				if (check_save(100)) {
					msg("You resist the effects!");
				} else {
					msg("Your body starts to scramble...");

					/* Pick a pair of stats */
					ii = randint0(6);
					for (jj = ii; jj == ii; jj = randint0(6))	/* loop */
						;

					max1 = p_ptr->stat_max[ii];
					cur1 = p_ptr->stat_cur[ii];
					max2 = p_ptr->stat_max[jj];
					cur2 = p_ptr->stat_cur[jj];

					p_ptr->stat_max[ii] = max2;
					p_ptr->stat_cur[ii] = cur2;
					p_ptr->stat_max[jj] = max1;
					p_ptr->stat_cur[jj] = cur1;

					p_ptr->update |= (PU_BONUS);
				}
			}

			/* time ball trap. */
			else if (nastyness < 90) {
				msg("You feel time itself assault you!");

				/* Target the player with a radius 0 ball attack. */
				fire_meteor(0, GF_TIME, p_ptr->py, p_ptr->px, 75, 0, TRUE);
			}

			/* trap of bugs gone berserk. */
			else {
				/* explain what the dickens is going on. */
				msg("GRUESOME Gnawing Bugs leap out at you!");

				if (!p_resist_good(P_RES_CONFU)) {
					(void) inc_timed(TMD_CONFUSED, randint0(20) + 10,
									 TRUE);
				} else
					notice_other(IF_RES_CONFU, 0);

				if (!p_resist_good(P_RES_CHAOS)) {
					(void) inc_timed(TMD_IMAGE, randint1(40), TRUE);
				} else
					notice_other(IF_RES_CHAOS, 0);

				/* XXX (hard coded) summon 3-6 bugs. */
				k = randint1(4) + 2;
				for (i = 0; i < k; ++i) {
					/* Look for a location */
					for (j = 0; j < 20; ++j) {
						feature_type *f_ptr;

						/* Pick a (scattered) distance. */
						int d = (j / 10) + randint1(3);

						/* Pick a location */
						scatter(&y, &x, y, x, d, 0);

						/* Require passable terrain */
						f_ptr = &f_info[cave_feat[y][x]];
						if (!tf_has(f_ptr->flags, TF_PASSABLE))
							continue;

						/* Hack -- no summon on glyph of warding */
						if (cave_trap_specific(y, x, RUNE_PROTECT))
							continue;

						/* Okay */
						break;
					}

					/* Attempt to place the awake bug */
					place_monster_aux(y, x, 453, FALSE, TRUE);
				}

				/* herald the arrival of bugs. */
				msg("AAAAAAAHHHH! THEY'RE EVERYWHERE!");
			}

			Rand_quick = TRUE;

			break;
		}

		/* teleport trap */
	case TRAP_PORTAL:
		{
			if (stage_map[p_ptr->stage][STAGE_TYPE] >= CAVE)
				msg("You teleport across the dungeon.");
			else
				msg("You teleport across the wilderness.");

			Rand_quick = FALSE;

			teleport_player(250, FALSE);

			Rand_quick = TRUE;

			break;
		}

		/* murder holes. */
	case TRAP_MURDER:
		{
			/* hold the object info. */
			object_type *o_ptr;
			object_type object_type_body;

			/* hold the missile type and name. */
			int sval = 0;
			int tval = 0;
			const char *missile_name = "";



			/* Determine the missile type and base damage. */
			if (randint1(3) == 1) {
				if (p_ptr->depth < 40) {
					missile_name = "shot";
					dam = damroll(2, 3);
					tval = TV_SHOT;
					sval = SV_AMMO_NORMAL;
				} else {
					missile_name = "seeker shot";
					dam = damroll(3, 7);
					tval = TV_SHOT;
					sval = SV_AMMO_HEAVY;
				}
			}

			else if (randint1(2) == 1) {
				if (p_ptr->depth < 55) {
					missile_name = "arrow";
					dam = damroll(2, 4);
					tval = TV_ARROW;
					sval = SV_AMMO_NORMAL;
				} else {
					missile_name = "seeker arrow";
					dam = damroll(3, 9);
					tval = TV_ARROW;
					sval = SV_AMMO_HEAVY;
				}
			}

			else {
				if (p_ptr->depth < 65) {
					missile_name = "bolt";
					dam = damroll(2, 5);
					tval = TV_BOLT;
					sval = SV_AMMO_NORMAL;
				} else {
					missile_name = "seeker bolt";
					dam = damroll(3, 11);
					tval = TV_BOLT;
					sval = SV_AMMO_HEAVY;
				}
			}

			/* determine if the missile hits. */
			if (check_trap_hit(75 + p_ptr->depth)) {
				msg("A %s hits you from above.", missile_name);

				Rand_quick = FALSE;

				/* critical hits. */
				if (randint1(2) == 1) {
					msg("It was well-aimed!");
					dam *= 1 + randint1(2);
				}
				if (randint1(2) == 1) {
					msg("It gouges you!");
					dam = 3 * dam / 2;

					/* cut the player. */
					(void) inc_timed(TMD_CUT, randint1(dam), TRUE);
				}

				Rand_quick = TRUE;

				take_hit(dam, name);
			}

			/* Explain what just happened. */
			else
				msg("A %s wizzes by your head.", missile_name);

			/* these will eventually run out of ammo. */

			Rand_quick = FALSE;

			if (randint0(8) == 0)
				remove_trap(y, x, FALSE, trap);

			Rand_quick = TRUE;

			/* Get local object */
			o_ptr = &object_type_body;

			/* Make a missile, identify it, and drop it near the player. */
			object_prep(o_ptr, lookup_kind(tval, sval), MINIMISE);
			object_aware(o_ptr);
			object_known(o_ptr);
			drop_near(o_ptr, -1, y, x, TRUE);

			break;
		}

		/* falling tree branch */
	case TRAP_BRANCH:
		{
			/* determine if the missile hits. */
			if (check_trap_hit(75 + p_ptr->depth)) {
				/* Take damage */
				dam = damroll(3, 5);
				msg("A branch hits you from above.");

				Rand_quick = FALSE;

				/* critical hits. */
				if (randint1(2) == 1) {
					msg("It was heavy!");
					dam = 3 * dam / 2;

					/* stun the player. */
					(void) inc_timed(TMD_STUN, randint1(dam), TRUE);
				}

				Rand_quick = TRUE;

				take_hit(dam, name);
			}

			/* Explain what just happened. */
			else
				msg("A falling branch just misses you.");

			/* No more */
			remove_trap(y, x, FALSE, trap);

			break;
		}

	}

	/* Revert to usage of the complex RNG. */
	Rand_quick = FALSE;
}

/**
 * Hit a trap. 
 */
extern void hit_trap(int y, int x)
{
	int i;

	/* Count the hidden traps here */
	int num = num_traps(y, x, -1);

	/* Oops.  We've walked right into trouble. */
	if (num == 1)
		msg("You stumble upon a trap!");
	else if (num > 1)
		msg("You stumble upon some traps!");


	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find all traps in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Fire off the trap */
			hit_trap_aux(y, x, i);

			/* Trap becomes visible (always XXX) */
			trf_on(t_ptr->flags, TRF_VISIBLE);
			sqinfo_on(cave_info[y][x], SQUARE_MARK);
		}
	}

	/* Verify traps (remove marker if appropriate) */
	(void) verify_trap(y, x, 0);
}


/**
 * Hack -- possible victim outcry. -LM- 
 */
static const char *desc_victim_outcry[] = {
	"'My money, where's my money?'",
	"'Thief! Thief! Thief! Baggins! We hates it forever!'",
	"'Tell me, have you seen a purse wandering around?'",
	"'Thieves, Fire, Murder!'",
	"''Ere, 'oo are you?'",
	"'Hey, look what I've copped!'",
	"'How dare you!'",
	"'Help yourself again, thief, there is plenty and to spare!'",
	"'All the world detests a thief.'",
	"'Catch me this thief!'",
	"'Hi! Ho! Robbery!'",
	"'My gold, my precious gold!'",
	"'My gold is costly, thief!'",
	"'Your blood for my gold?  Agreed!'",
	"'I scrimp, I save, and now it's gone!'",
	"'Robbers like you are part of the problem!'",
	"'Banditti!  This place is just not safe anymore!'",
	"'Ruined!  I'm ruined!'",
	"'Where, where is the common decency?'",
	"'Your knavish tricks end here and now!'",
};



/**
 * Rogues may steal gold from monsters.  The monster needs to have 
 * something to steal (it must drop some form of loot), and should 
 * preferably be asleep.  Humanoids and dragons are a rogue's favorite
 * targets.  Steal too often on a level, and monsters will be more wary, 
 * and the hue and cry will be eventually be raised.  Having every 
 * monster on the level awake and aggravated is not pleasant. -LM-
 */
extern void py_steal(int y, int x)
{
	const char *act = NULL;

	monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	char m_name[80];

	int i;
	int effect, theft_protection;
	int filching_power = 0;
	int purse = 0;

	bool thief = FALSE;
	bool success = FALSE;

	/* Check intent */
	if ((m_ptr->hostile != -1)
		&& !get_check("Do you want to steal from this being?")) {
		py_attack(y, x, FALSE);
		return;
	}

	/* Hard limit on theft. */
	if (number_of_thefts_on_level > 4) {
		msg("Everyone is keeping a lookout for you.  You can steal nothing here.");
		return;
	}

	/* Determine the cunning of the thief. */
	filching_power = 2 * p_ptr->lev;

	/* Penalize some conditions */
	if (p_ptr->timed[TMD_BLIND] || no_light())
		filching_power = filching_power / 10;
	if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE])
		filching_power = filching_power / 10;

	/* Determine how much protection the monster has. */
	theft_protection = (7 * (r_ptr->level + 2) / 4);
	theft_protection += (m_ptr->mspeed - p_ptr->state.pspeed);
	if (theft_protection < 1)
		theft_protection = 1;

	/* Send a thief to catch a thief. */
	for (i = 0; i < 4; i++) {
		/* Extract infomation about the blow effect */
		effect = r_ptr->blow[i].effect;
		if (effect == RBE_EAT_GOLD)
			thief = TRUE;
		if (effect == RBE_EAT_ITEM)
			thief = TRUE;
	}
	if (thief)
		theft_protection += 30;

	if (m_ptr->csleep)
		theft_protection = 3 * theft_protection / 5;

	/* Special player stealth magics aid stealing, but are lost in the process */
	if (p_ptr->timed[TMD_SSTEALTH]) {
		theft_protection = 3 * theft_protection / 5;
		(void) clear_timed(TMD_SSTEALTH, TRUE);
	}

	/* The more you steal on a level, the more wary the monsters. */
	theft_protection += number_of_thefts_on_level * 15;

	/* Did the theft succeed? */
	if (randint1(theft_protection) < filching_power)
		success = TRUE;


	/* If the theft succeeded, determine the value of the purse. */
	if (success) {
		purse = (r_ptr->level + 1) + randint1(3 * (r_ptr->level + 1) / 2);

		/* Uniques are juicy targets. */
		if (rf_has(r_ptr->flags, RF_UNIQUE))
			purse *= 3;

		/* But some monsters are dirt poor. */
		if (!(rf_has(r_ptr->flags, RF_DROP_60))
			|| rf_has(r_ptr->flags, RF_DROP_90)
			|| rf_has(r_ptr->flags, RF_DROP_1D2)
			|| rf_has(r_ptr->flags, RF_DROP_2D2)
			|| rf_has(r_ptr->flags, RF_DROP_3D2)
			|| rf_has(r_ptr->flags, RF_DROP_4D2))
			purse = 0;

		/* Some monster races are far better to steal from than others. */
		if ((r_ptr->d_char == 'D') || (r_ptr->d_char == 'd')
			|| (r_ptr->d_char == 'p') || (r_ptr->d_char == 'h'))
			purse *= 2 + randint1(3) + randint1(r_ptr->level / 20);
		else if ((r_ptr->d_char == 'P') || (r_ptr->d_char == 'o')
				 || (r_ptr->d_char == 'O') || (r_ptr->d_char == 'T')
				 || (r_ptr->d_char == 'n') || (r_ptr->d_char == 'W')
				 || (r_ptr->d_char == 'k') || (r_ptr->d_char == 'L')
				 || (r_ptr->d_char == 'V') || (r_ptr->d_char == 'y'))
			purse *= 1 + randint1(3) + randint1(r_ptr->level / 30);

		/* Pickings are scarce in a land of many thieves. */
		purse = purse * (p_ptr->depth + 5) / (p_ptr->recall[0] + 5);

		/* Increase player gold. */
		p_ptr->au += purse;

		/* Limit to avoid buffer overflow */
		if (p_ptr->au > PY_MAX_GOLD)
			p_ptr->au = PY_MAX_GOLD;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Announce the good news. */
		if (purse)
			msg("You burgle %d gold.", purse);

		/* Pockets are empty. */
		else
			msg("You burgle only dust.");
	}

	/* The victim normally, but not always, wakes up and is aggravated. */
	if (randint1(4) != 1) {
		m_ptr->csleep = 0;
		m_ptr->mflag |= (MFLAG_ACTV);
		if (m_ptr->mspeed < r_ptr->speed + 3)
			m_ptr->mspeed += 10;

		/* Become hostile */
		m_ptr->hostile = -1;

		/* Occasionally, amuse the player with a message. */
		if ((randint1(5) == 1) && (purse)
			&& (rf_has(r_ptr->flags, RF_SMART))) {
			monster_desc(m_name, sizeof(m_name), m_ptr, 0);
			act = desc_victim_outcry[randint0(20)];
			my_strcap(m_name);
			msg("%s cries out %s", m_name, act);
		}
		/* Otherwise, simply explain what happened. */
		else {
			monster_desc(m_name, sizeof(m_name), m_ptr, 0);
			msg("You have aroused %s.", m_name);
		}
	}

	/* The thief also speeds up, but only for just long enough to escape. */
	if (!p_ptr->timed[TMD_FAST])
		p_ptr->timed[TMD_FAST] += 2;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff(p_ptr);


	/* Increment the number of thefts, and possibly raise the hue and cry. */
	number_of_thefts_on_level++;

	if (number_of_thefts_on_level > 4) {
		/* Notify the player of the trouble he's in. */
		msg("All the level is in an uproar over your misdeeds!");

		/* Aggravate and speed up all monsters on level. */
		(void) aggravate_monsters(1, TRUE);
	}

	else if ((number_of_thefts_on_level > 2) || (randint1(8) == 1)) {
		msg("You hear hunting parties scouring the area for a notorious burgler.");

		/* Aggravate monsters nearby. */
		(void) aggravate_monsters(1, FALSE);
	}

	/* Rogue "Hit and Run" attack. */
	if (p_ptr->special_attack & (ATTACK_FLEE)) {
		/* Cancel the fleeing spell */
		p_ptr->special_attack &= ~(ATTACK_FLEE);

		/* Message */
		msg("You escape into the shadows!");

		/* Teleport. */
		teleport_player(6 + p_ptr->lev / 5, TRUE);

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATUS);
	}
}


/**
 * Rogues may set traps.  Only one such trap may exist at any one time, 
 * but an old trap can be disarmed to free up equipment for a new trap.
 * -LM-
 */
extern bool py_set_trap(int y, int x)
{
	int max_traps;

	max_traps =
		1 + ((p_ptr->lev >= 25) ? 1 : 0) +
		(player_has(PF_EXTRA_TRAP) ? 1 : 0);

	if (p_ptr->timed[TMD_BLIND] || no_light()) {
		msg("You can not see to set a trap.");
		return FALSE;
	}

	if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE]) {
		msg("You are too confused.");
		return FALSE;
	}

	/* Paranoia -- Forbid more than max_traps being set. */
	if (num_trap_on_level >= max_traps) {
		msg("You must disarm your existing trap to free up your equipment.");
		return FALSE;
	}

	/* No setting traps while shapeshifted */
	if (SCHANGE) {
		msg("You can not set traps while shapechanged.");
		msg("Use the ']' command to return to your normal form.");
		return FALSE;
	}
#if 0
	/* Scan all objects in the grid */
	for (this_o_idx = cave_o_idx[y][x]; this_o_idx;
		 this_o_idx = next_o_idx) {
		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Artifact */
		if (o_ptr->name1) {
			msg("There is an indestructible object here.");
			return FALSE;
		}

		/* Visible object to be destroyed */
		if (!squelch_hide_item(o_ptr))
			destroy_message = TRUE;
	}

	/* Verify */
	if (cave_o_idx[y][x]) {
		if (destroy_message)
			if (!get_check("Destroy all items and set a trap?"))
				return FALSE;

		for (this_o_idx = cave_o_idx[y][x]; this_o_idx;
			 this_o_idx = next_o_idx) {
			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Delete the object */
			delete_object_idx(this_o_idx);
		}

		/* Redraw */
		light_spot(y, x);
	}
#endif
	/* Set the trap, and draw it. */
	place_trap(y, x, MTRAP_BASE, 0);

	/* Notify the player. */
	msg("You set a monster trap.");

	/* A trap has been set */
	return TRUE;
}

/**
 * Trap coordinates 
 */
static int mtrap_y = 0;
static int mtrap_x = 0;

static char *mtrap_desc[] = {
	"Sturdy Trap      (less likely to break)",
	"Netted Trap      (effective versus flyers)",
	"Confusion Trap   (confuses monsters)",
	"Poison Gas Trap  (creates a toxic cloud)",
	"Spirit Trap      (effective versus spirits)",
	"Lightning Trap   (shoots a lightning bolt)",
	"Explosive Trap   (causes area damage)",
	"Portal Trap      (teleports monsters)",
	"Stasis Trap      (freezes time for a monster)",
	"Drain Life Trap  (hurts living monsters)",
	"Unmagic Trap     (damages and reduces mana)",
	"Dispelling Trap  (hurts all monsters in sight)",
	"Genocide Trap    (removes nearby like monsters)"
};

char mtrap_tag(menu_type * menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the sval menu
 */
void mtrap_display(menu_type * menu, int oid, bool cursor, int row,
				   int col, int width)
{
	const u16b *choice = menu_priv(menu);
	int idx = choice[oid];

	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);

	/* Print it */
	c_put_str(attr, format("%s", mtrap_desc[idx]), row, col);
}

/**
 * Deal with events on the trap menu
 */
bool mtrap_action(menu_type * menu, const ui_event * db, int oid)
{
	u16b *choice = menu_priv(menu);

	int idx = choice[oid];

	/* Get the old trap */
	int old_trap = monster_trap_idx(mtrap_y, mtrap_x);

	/* Remove the old trap */
	remove_trap(mtrap_y, mtrap_x, FALSE, old_trap);

	/* Place the new trap */
	place_trap(mtrap_y, mtrap_x, MTRAP_BASE + 1 + idx, 0);

	return FALSE;
}


/**
 * Display list of monster traps.
 */
bool mtrap_menu(void)
{
	menu_type menu;
	menu_iter menu_f = { mtrap_tag, 0, mtrap_display, mtrap_action, 0 };
	region area = { 15, 1, 48, -1 };
	ui_event evt = { 0 };

	size_t i, num = 0;

	u16b *choice;

	/* See how many traps available */
	if (player_has(PF_EXTRA_TRAP))
		num = 1 + (p_ptr->lev / 4);
	else
		num = 1 + (p_ptr->lev / 6);

	/* Create the array */
	choice = C_ZNEW(num, u16b);

	/* Obvious */
	for (i = 0; i < num; i++) {
		choice[i] = i;
	}

	/* Clear space */
	area.page_rows = num + 2;

	/* Return here if there are no traps */
	if (!num) {
		FREE(choice);
		return FALSE;
	}


	/* Save the screen and clear it */
	screen_save();

	/* Help text */

	/* Set up the menu */
	WIPE(&menu, menu);
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu.title = "Choose an advanced monster trap (ESC to cancel):";
	menu_setpriv(&menu, num, choice);
	menu_layout(&menu, &area);
	prt("", area.row + 1, area.col);

	/* Select an entry */
	evt = menu_select(&menu, 0, TRUE);

	/* Free memory */
	FREE(choice);

	/* Load screen */
	screen_load();
	return (evt.type != EVT_ESCAPE);
}


/** 
 * Turn a basic monster trap into an advanced one -BR-
 */
extern bool py_modify_trap(int y, int x)
{
	if (p_ptr->timed[TMD_BLIND] || no_light()) {
		msg("You can not see to modify your trap.");
		return FALSE;
	}

	if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE]) {
		msg("You are too confused.");
		return FALSE;
	}

	/* No setting traps while shapeshifted */
	if (SCHANGE) {
		msg("You can not set traps while shapechanged.");
		msg("Use the ']' command to return to your normal form.");
		return FALSE;
	}

	mtrap_y = y;
	mtrap_x = x;

	/* get choice */
	if (mtrap_menu()) {
		/* Notify the player. */
		msg("You modify the monster trap.");
	}

	/* Trap was modified */
	return TRUE;
}


/**
 * Delete/Remove all the traps when the player leaves the level
 */
void wipe_trap_list(void)
{
	int i;

	/* Delete all the traps */
	for (i = trap_max - 1; i >= 0; i--) {
		trap_type *t_ptr = &trap_list[i];

		/* Wipe the trap */
		WIPE(t_ptr, trap_type);
	}

	/* Reset "trap_max" */
	trap_max = 0;

	/* Reset the number of glyphs on the level. */
	for (i = 0; i < RUNE_TAIL; i++)
		num_runes_on_level[i] = 0;

	/* Reset the number of monster traps on the level. */
	num_trap_on_level = 0;
}



/**
 * Remove a trap
 */
static void remove_trap_aux(trap_type * t_ptr, int y, int x, bool domsg)
{
	/* We are clearing a web */
	if (trf_has(t_ptr->flags, TRF_WEB)) {
		if (domsg)
			msg("You clear the web.");
	}

	/* We are deleting a rune */
	else if (trf_has(t_ptr->flags, TRF_RUNE)) {
		if (domsg)
			msg("You have removed the %s.", t_ptr->kind->name);
		num_runes_on_level[t_ptr->t_idx - 1]--;
	} else if (domsg)
		msgt(MSG_DISARM, "You have disarmed the %s.", t_ptr->kind->name);

	/* We are deleting a monster trap */
	if (trf_has(t_ptr->flags, TRF_M_TRAP))
		num_trap_on_level--;

	/* Wipe the trap */
	sqinfo_off(cave_info[y][x], SQUARE_TRAP);
	(void) WIPE(t_ptr, trap_type);
}

/**
 * Remove traps.
 *
 * If called with t_idx < 0, will remove all traps in the location given.
 * Otherwise, will remove the trap with the given index.
 *
 * Return TRUE if no traps now exist in this grid.
 */
bool remove_trap(int y, int x, bool domsg, int t_idx)
{
	int i;
	bool trap_exists;

	/* Called with a specific index */
	if (t_idx >= 0) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[t_idx];

		/* Remove it */
		remove_trap_aux(t_ptr, y, x, domsg);

		/* Note when trap list actually gets shorter */
		if (t_idx == trap_max - 1)
			trap_max--;
	}

	/* No specific index -- remove all traps here */
	else {
		/* Scan the current trap list (backwards) */
		for (i = trap_max - 1; i >= 0; i--) {
			/* Point to this trap */
			trap_type *t_ptr = &trap_list[i];

			/* Find all traps in this position */
			if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
				/* Remove it */
				remove_trap_aux(t_ptr, y, x, domsg);

				/* Note when trap list actually gets shorter */
				if (i == trap_max - 1)
					trap_max--;
			}
		}
	}

	/* Refresh grids that the character can see */
	if (player_can_see_bold(y, x))
		light_spot(y, x);

	/* Verify traps (remove marker if appropriate) */
	trap_exists = verify_trap(y, x, 0);

	/* Report whether any traps exist in this grid */
	return (trap_exists);
}

/**
 * Remove all traps of a specific kind from a location.
 */
void remove_trap_kind(int y, int x, bool domsg, int t_idx)
{
	int i;

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Require that it be of this type */
			if (t_ptr->t_idx == t_idx)
				(void) remove_trap(y, x, domsg, i);
		}
	}
}

/* Trap menu code */

/**
 * Choice of trap
 */
static int trap_choice = 0;

/**
 * Label for trap
 */
static char trap_tag(menu_type * menu, int oid)
{
	return I2A(oid);
}

/**
 * Display an entry on the trap menu
 */
void trap_display(menu_type * menu, int oid, bool cursor, int row, int col,
				  int width)
{
	const u16b *choice = menu_priv(menu);

	byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);

	c_prt(attr, trap_list[choice[oid]].kind->name, row, col);

}

/**
 * Deal with events on the jump menu
 */
bool trap_action(menu_type * menu, const ui_event * evt, int oid)
{
	u16b *choice = menu_priv(menu);

	int idx = choice[oid];

	if (evt->type == EVT_SELECT) {
		trap_choice = idx;
		return FALSE;
	} else
		return TRUE;
}


/**
 * Display list of places to jump to.
 */
bool trap_menu(int y, int x, int *idx)
{
	menu_type menu;
	menu_iter menu_f = { trap_tag, 0, trap_display, trap_action, 0 };
	region area = { 15, 1, 48, -1 };
	ui_event evt = { 0 };
	int cursor = 0, j = 0;
	int i;
	u16b *choice;

	/* Create the array */
	choice = C_ZNEW(trap_max, u16b);

	/* Scan the current trap list */
	for (j = 0, i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find a trap in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Trap must be visible */
			if (!trf_has(t_ptr->flags, TRF_VISIBLE))
				continue;

			/* Count all traps */
			choice[j++] = i;

			/* Remember last trap index */
			*idx = i;
		}
	}

	/* We have no visible traps */
	if (!j)
		return (FALSE);

	/* We have one trap (usual case) */
	if (j == 1)
		return (TRUE);

	/* Clear space */
	area.page_rows = j + 2;

	/* Save the screen and clear it */
	screen_save();

	/* Set up the menu */
	WIPE(&menu, menu);
	menu.title = "Which trap do you choose?";
	menu.cmd_keys = " \n\r";
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu_setpriv(&menu, j, choice);
	menu_layout(&menu, &area);

	/* Select an entry */
	evt = menu_select(&menu, cursor, TRUE);

	/* Set it */
	if (evt.type == EVT_SELECT)
		*idx = trap_choice;

	/* Free memory */
	FREE(choice);

	/* Load screen */
	screen_load();
	return (evt.type != EVT_ESCAPE);
}


/**
 * Choose a trap.  If there is more than one trap in this grid, we display
 * a menu, and let the player choose among them.
 *
 * Possible enhancements to this code include remembering which trap was
 * chosen last so that command repeats can work correctly on grids with
 * multiple traps.
 */
bool get_trap(int y, int x, int *idx)
{
	/* Require that grid be legal */
	if (!in_bounds_fully(y, x))
		return (FALSE);

	/* Look for the trap marker */
	if (!sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Get a trap */
	return trap_menu(y, x, idx);
}


/*
 * Determine if a trap is disarmable
 */
static bool is_disarmable_trap(trap_type * t_ptr)
{
	if (!trf_has(t_ptr->flags, TRF_VISIBLE))
		return (FALSE);
	return (!trf_has(t_ptr->kind->flags, TRF_NO_DISARM));
}

/*
 * Determine if a grid contains disarmable traps.
 */
bool has_disarmable_trap(int y, int x)
{
	int i;

	/* First, check the trap marker */
	if (!sqinfo_has(cave_info[y][x], SQUARE_TRAP))
		return (FALSE);

	/* Scan the current trap list */
	for (i = 0; i < trap_max; i++) {
		/* Point to this trap */
		trap_type *t_ptr = &trap_list[i];

		/* Find all traps in this position */
		if ((t_ptr->fy == y) && (t_ptr->fx == x)) {
			/* Accept first disarmable trap */
			if (is_disarmable_trap(t_ptr))
				return (TRUE);
		}
	}

	/* No disarmable traps found */
	return (FALSE);
}
