/**
 * \file effect-handler-attack.c
 * \brief Handler functions for attack effects
 *
 * Copyright (c) 2007 Andi Sidwell
 * Copyright (c) 2016 Ben Semmler, Nick McConnell
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

#include "effect-handler.h"
#include "game-input.h"
#include "init.h"
#include "mon-desc.h"
#include "mon-make.h"
#include "mon-spell.h"
#include "mon-util.h"
#include "obj-desc.h"
#include "obj-knowledge.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-timed.h"
#include "player-util.h"
#include "project.h"
#include "trap.h"


int eff_level(struct player *p)
{
	int plev = p->lev;

	/* Specialty abilities */
	if (player_has(player, PF_HEIGHTEN_MAGIC))
		plev += 1 + ((player->heighten_power + 5) / 10);
	if (player_has(player, PF_CHANNELING))
		plev += player_get_channeling_boost(player);

	return plev;
}

static void get_target(struct source origin, int dir, struct loc *grid,
					   int *flags)
{
	switch (origin.what) {
		case SRC_MONSTER: {
			struct monster *monster = cave_monster(cave, origin.which.monster);
			int conf_level, accuracy = 100;

			if (!monster) break;

			conf_level = monster_effect_level(monster, MON_TMD_CONF);
			while (conf_level) {
				accuracy *= (100 - CONF_RANDOM_CHANCE);
				accuracy /= 100;
				conf_level--;
			}

			*flags |= (PROJECT_PLAY);

			if (randint1(100) > accuracy) {
				dir = randint1(9);
				*grid = loc_sum(monster->grid, ddgrid[dir]);
			} else if (monster->target.midx > 0) {
				struct monster *mon = cave_monster(cave, monster->target.midx);
				*grid = mon->grid;
			} else {
				if (monster_is_decoyed(monster)) {
					*grid = cave_find_decoy(cave);
				} else {
					*grid = player->grid;
				}
			}

			break;
		}

		case SRC_PLAYER:
			if (dir == DIR_TARGET && target_okay()) {
				target_get(grid);
			} else {
				/* Use the adjacent grid in the given direction as target */
				*grid = loc_sum(player->grid, ddgrid[dir]);
			}

			break;

		default:
			*flags |= PROJECT_PLAY;
			*grid = player->grid;
			break;
	}
}

/**
 * Apply the project() function in a direction, or at a target
 */
static bool project_aimed(struct source origin,
						  int typ, int dir, int dam, int flg,
						  const struct object *obj)
{
	struct loc grid = loc(-1, -1);

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

	get_target(origin, dir, &grid, &flg);

	/* Aim at the target, do NOT explode */
	return (project(origin, 0, grid, dam, typ, flg, 0, 0, obj));
}

/**
 * Apply the project() function to grids around the target
 */
static bool project_touch(int dam, int rad, int typ, bool aware,
						  const struct object *obj)
{
	struct loc pgrid = player->grid;

	int flg = PROJECT_GRID | PROJECT_KILL | PROJECT_HIDE | PROJECT_ITEM | PROJECT_THRU;
	if (aware) flg |= PROJECT_AWARE;
	return (project(source_player(), rad, pgrid, dam, typ, flg, 0, 0, obj));
}

/**
 * Cast a bolt spell
 * Stop if we hit a monster, as a bolt
 * Affect monsters (not grids or objects)
 */
bool effect_handler_BOLT(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int flg = PROJECT_STOP | PROJECT_KILL;
	(void) project_aimed(context->origin, context->subtype, context->dir, dam,
						 flg, context->obj);
	if (!player->timed[TMD_BLIND])
		context->ident = true;
	return true;
}

/**
 * Cast a beam spell
 * Pass through monsters, as a beam
 * Affect monsters (not grids or objects)
 */
bool effect_handler_BEAM(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int flg = PROJECT_BEAM | PROJECT_KILL;
	(void) project_aimed(context->origin, context->subtype, context->dir, dam,
						 flg, context->obj);
	if (!player->timed[TMD_BLIND])
		context->ident = true;
	return true;
}

/**
 * Cast a bolt spell, or rarely, a beam spell
 * context->other is used as any adjustment to the regular beam chance
 */
bool effect_handler_BOLT_OR_BEAM(effect_handler_context_t *context)
{
	int beam = context->beam + context->other;

	if (randint0(100) < beam)
		return effect_handler_BEAM(context);
	else
		return effect_handler_BOLT(context);
}

/**
 * Cast a line spell
 * Pass through monsters, as a beam
 * Affect monsters and grids (not objects)
 */
bool effect_handler_LINE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
	if (project_aimed(context->origin, context->subtype, context->dir, dam, flg, context->obj))
		context->ident = true;
	return true;
}

/**
 * Cast an alter spell
 * Affect objects and grids (not monsters)
 */
bool effect_handler_ALTER(effect_handler_context_t *context)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	if (project_aimed(context->origin, context->subtype, context->dir, 0, flg, context->obj))
		context->ident = true;
	return true;
}

/**
 * Heal the player by a given percentage of their wounds, or a minimum
 * amount, whichever is larger.
 *
 * context->value.base should be the minimum, and
 * context->value.m_bonus the percentage
 */
bool effect_handler_HEAL_HP(effect_handler_context_t *context)
{
	int num, minh;

	/* Always ID */
	context->ident = true;

	/* No healing needed */
	if (player->chp >= player->mhp) return (true);

	/* Figure percentage healing level */
	num = ((player->mhp - player->chp) * context->value.m_bonus) / 100;

	/* Enforce minimum */
	minh = context->value.base
		+ damroll(context->value.dice, context->value.sides);
	if (num < minh) num = minh;
	if (num <= 0) {
		/*
		 * There's no healing: either because not damaged enough for
		 * the bonus amount to matter or the effect was misconfigured.
		 */
		return true;
	}

	/* Gain hitpoints */
	player->chp += num;

	/* Enforce maximum */
	if (player->chp >= player->mhp) {
		player->chp = player->mhp;
		player->chp_frac = 0;
	}

	/* Redraw */
	player->upkeep->redraw |= (PR_HP);

	/* Print a nice message */
	if (num < 5)
		msg("You feel a little better.");
	else if (num < 15)
		msg("You feel better.");
	else if (num < 35)
		msg("You feel much better.");
	else
		msg("You feel very good.");

	return (true);
}

/**
 * Monster self-healing.
 */
bool effect_handler_MON_HEAL_HP(effect_handler_context_t *context)
{
	assert(context->origin.what == SRC_MONSTER);

	int midx = context->origin.which.monster;
	struct monster *mon = midx > 0 ? cave_monster(cave, midx) : NULL;

	int amount = effect_calculate_value(context, false);
	char m_name[80], m_poss[80];
	bool seen;

	if (!mon) return true;

	/* Get the monster name (or "it") */
	monster_desc(m_name, sizeof(m_name), mon, MDESC_STANDARD);

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(m_poss, sizeof(m_poss), mon, MDESC_PRO_VIS | MDESC_POSS);

	seen = (!player->timed[TMD_BLIND] && monster_is_visible(mon));

	/* Heal some */
	mon->hp += amount;

	/* Fully healed */
	if (mon->hp >= mon->maxhp) {
		mon->hp = mon->maxhp;

		if (seen)
			msg("%s looks REALLY healthy!", m_name);
		else
			msg("%s sounds REALLY healthy!", m_name);
	} else if (seen) { /* Partially healed */
		msg("%s looks healthier.", m_name);
	} else {
		msg("%s sounds healthier.", m_name);
	}

	/* Redraw (later) if needed */
	if (player->upkeep->health_who == mon)
		player->upkeep->redraw |= (PR_HEALTH);

	/* Cancel fear */
	if (mon->m_timed[MON_TMD_FEAR]) {
		mon_clear_timed(mon, MON_TMD_FEAR, MON_TMD_FLG_NOMESSAGE);
		msg("%s recovers %s courage.", m_name, m_poss);
	}

	/* ID */
	context->ident = true;

	return true;
}

/**
 * Monster healing of kin.
 */
bool effect_handler_MON_HEAL_KIN(effect_handler_context_t *context)
{
	assert(context->origin.what == SRC_MONSTER);

	int midx = context->origin.which.monster;
	struct monster *mon = midx > 0 ? cave_monster(cave, midx) : NULL;
	if (!mon) return true;

	int amount = effect_calculate_value(context, false);
	char m_name[80], m_poss[80];
	bool seen;

	/* Find a nearby monster */
	mon = choose_nearby_injured_kin(cave, mon);
	if (!mon) return true;

	/* Get the monster name (or "it") */
	monster_desc(m_name, sizeof(m_name), mon, MDESC_STANDARD);

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(m_poss, sizeof(m_poss), mon, MDESC_PRO_VIS | MDESC_POSS);

	seen = (!player->timed[TMD_BLIND] && monster_is_visible(mon));

	/* Heal some */
	mon->hp = MIN(mon->hp + amount, mon->maxhp);

	if (seen) {
		if (mon->hp == mon->maxhp) {
			msg("%s looks REALLY healthy!", m_name);
		} else if (seen) { /* Partially healed */
			msg("%s looks healthier.", m_name);
		}
	}

	/* Redraw (later) if needed */
	if (player->upkeep->health_who == mon)
		player->upkeep->redraw |= (PR_HEALTH);

	/* Cancel fear */
	if (mon->m_timed[MON_TMD_FEAR]) {
		mon_clear_timed(mon, MON_TMD_FEAR, MON_TMD_FLG_NOMESSAGE);
		msg("%s recovers %s courage.", m_name, m_poss);
	}

	/* ID */
	context->ident = true;

	return true;
}

/**
 * Cast a bolt spell
 * Stop if we hit a monster, as a bolt
 * Affect monsters (not grids or objects)
 * Like BOLT, but only identifies on noticing an effect
 */
bool effect_handler_BOLT_STATUS(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int flg = PROJECT_STOP | PROJECT_KILL;
	if (project_aimed(context->origin, context->subtype, context->dir, dam, flg, context->obj))
		context->ident = true;
	return true;
}

/**
 * Cast a bolt spell
 * Stop if we hit a monster, as a bolt
 * Affect monsters (not grids or objects)
 * The same as BOLT_STATUS, but done as a separate function to aid descriptions
 */
bool effect_handler_BOLT_STATUS_DAM(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int flg = PROJECT_STOP | PROJECT_KILL;
	if (project_aimed(context->origin, context->subtype, context->dir, dam, flg, context->obj))
		context->ident = true;
	return true;
}

/**
 * Cast a bolt spell
 * Stop if we hit a monster, as a bolt
 * Affect monsters (not grids or objects)
 * Notice stuff based on awareness of the effect
 */
bool effect_handler_BOLT_AWARE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int flg = PROJECT_STOP | PROJECT_KILL;
	if (context->aware) flg |= PROJECT_AWARE;
	if (project_aimed(context->origin, context->subtype, context->dir, dam, flg, context->obj))
		context->ident = true;
	return true;
}

/**
 * Affect adjacent grids (radius 1 ball attack)
 */
bool effect_handler_TOUCH(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int rad = context->radius ? context->radius : 1;

	if (context->origin.what == SRC_MONSTER) {
		struct monster *mon = cave_monster(cave, context->origin.which.monster);
		struct monster *t_mon = monster_target_monster(context);
		struct loc decoy = cave_find_decoy(cave);

		/* Target decoy */
		if (decoy.y && decoy.x) {
			int flg = PROJECT_GRID | PROJECT_KILL | PROJECT_HIDE | PROJECT_ITEM | PROJECT_THRU;
			return (project(source_trap(square_trap(cave, decoy)),
					rad, decoy, dam, context->subtype,flg, 0, 0, context->obj));
		}

		/* Monster cast at monster */
		if (t_mon) {
			int flg = PROJECT_GRID | PROJECT_KILL | PROJECT_HIDE | PROJECT_ITEM | PROJECT_THRU;
			return (project(source_monster(mon->target.midx), rad,
							t_mon->grid, dam, context->subtype,
							flg, 0, 0, context->obj));
		}
	}

	if (project_touch(dam, rad, context->subtype, false, context->obj))
		context->ident = true;
	return true;
}

/**
 * Affect adjacent grids (radius 1 ball attack)
 * Notice stuff based on awareness of the effect
 */
bool effect_handler_TOUCH_AWARE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int rad = context->radius ? context->radius : 1;
	if (project_touch(dam, rad, context->subtype, context->aware, context->obj))
		context->ident = true;
	return true;
}

/**
 * Deal damage from the current monster or trap to the player
 */
bool effect_handler_DAMAGE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, false);
	char killer[80];

	/* Always ID */
	context->ident = true;

	switch (context->origin.what) {
		case SRC_MONSTER: {
			struct monster *mon = cave_monster(cave,
											   context->origin.which.monster);
			struct monster *t_mon = monster_target_monster(context);
			struct loc decoy = cave_find_decoy(cave);

			/* Damage another monster */
			if (t_mon) {
				bool fear = false;

				mon_take_nonplayer_hit(dam, t_mon, MON_MSG_NONE, MON_MSG_DIE);
				if (fear && monster_is_visible(t_mon)) {
					add_monster_message(t_mon, MON_MSG_FLEE_IN_TERROR, true);
				}
				return true;
			}

			/* Destroy a decoy */
			if (decoy.y && decoy.x) {
				square_destroy_decoy(cave, decoy);
				return true;
			}

			monster_desc(killer, sizeof(killer), mon, MDESC_DIED_FROM);
			break;
		}

		case SRC_TRAP: {
			struct trap *trap = context->origin.which.trap;
			const char *article = is_a_vowel(trap->kind->desc[0]) ? "an " : "a ";
			strnfmt(killer, sizeof(killer), "%s%s", article, trap->kind->desc);
			break;
		}

		case SRC_OBJECT: {
			/* Must be a cursed weapon */
			struct object *obj = context->origin.which.object;
			object_desc(killer, sizeof(killer), obj,
				ODESC_PREFIX | ODESC_BASE, player);
			break;
		}

		case SRC_CHEST_TRAP: {
			struct chest_trap *trap = context->origin.which.chest_trap;
			strnfmt(killer, sizeof(killer), "%s", trap->msg_death);
			break;
		}

		case SRC_PLAYER: {
			if (context->msg) {
				my_strcpy(killer, context->msg, sizeof(killer));
			} else {
				my_strcpy(killer, "yourself", sizeof(killer));
			}
			break;
		}

		case SRC_NONE: {
			my_strcpy(killer, "a bug", sizeof(killer));
			break;
		}
		default: break;
	}

	/* Hit the player */
	dam = player_apply_damage_reduction(player, dam);
	if (dam && OPT(player, show_damage)) {
		msg("You take %d damage.", dam);
	}
	take_hit(player, dam, killer);

	return true;
}

/**
 * Project from the starting grid at the player, with full intensity out to
 * its radius
 * Affect the player (even when player-cast), grids, objects, and monsters
 */
bool effect_handler_SPOT(effect_handler_context_t *context)
{
	struct loc grid;
	int dam = effect_calculate_value(context, false);
	int rad = context->radius ? context->radius : 0;

	int flg = PROJECT_STOP | PROJECT_PLAY | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_SELF;

	/* Get the centre of the spot */
	if (context->origin.what == SRC_PLAYER) {
		grid = player->grid;
	} else if (context->origin.what == SRC_MONSTER) {
		grid = cave_monster(cave, context->origin.which.monster)->grid;
	} else {
		/* Must be a trap */
		assert(context->origin.what == SRC_TRAP);
		grid = context->origin.which.trap->grid;
	}

	/* Handle increasing radius with player level */
	if (context->other && context->origin.what == SRC_PLAYER) {
		rad += eff_level(player) / context->other;
	}

	/* Aim at the target, explode */
	if (project(context->origin, rad, grid, dam, context->subtype, flg, 0,
				rad, NULL))
		context->ident = true;

	return true;
}

/**
 * Project from the player's grid, act as a ball, with full intensity out as
 * far as the given diameter
 * Affect grids, objects, and monsters
 */
bool effect_handler_SPHERE(effect_handler_context_t *context)
{
	struct loc pgrid = player->grid;
	int dam = effect_calculate_value(context, false);
	int rad = context->radius ? context->radius : 0;
	int diameter_of_source = context->other ? context->other : 0;

	int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Aim at the target, explode */
	if (project(context->origin, rad, pgrid, dam, context->subtype, flg, 0,
				diameter_of_source, NULL))
		context->ident = true;

	return true;
}

/**
 * Hit context->other% of grids in the given radius of the player or the given
 * coordinates
 * Affect grids, objects, and monsters
 */
bool effect_handler_ZONE(effect_handler_context_t *context)
{
	struct loc grid, centre = player->grid;
	int dam = effect_calculate_value(context, false);
	int rad = context->radius;
	int type = context->subtype;
	int chance = context->other;
	int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL |
		PROJECT_JUMP;

	/* Check for coordinates */
	if (context->x && context->y) {
		centre = loc(context->x, context->y);
	}

	/* Everything in range */
	for (grid.y = centre.y - rad; grid.y <= centre.y + rad; grid.y++) {
		for (grid.x = centre.x - rad; grid.x <= centre.x + rad; grid.x++) {
			int dist = distance(centre, grid);

			/* Skip distant grids */
			if (dist > rad) continue;

			/* Percentage chance of hitting */
			if (randint0(100) >= chance) continue;

			project(source_player(), 1, grid, dam, type, flg, 0, 0, NULL);
		}
	}
	return true;
}

/**
 * Cast a ball spell
 * Stop if we hit a monster or the player, act as a ball
 * Allow target mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool effect_handler_BALL(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int rad = context->radius ? context->radius : 2;
	struct loc target = loc(-1, -1);

	int flg = PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Player or monster? */
	switch (context->origin.what) {
		case SRC_MONSTER: {
			struct monster *mon = cave_monster(cave, context->origin.which.monster);
			int conf_level, accuracy = 100;
			struct monster *t_mon = monster_target_monster(context);

			assert(mon);

			conf_level = monster_effect_level(mon, MON_TMD_CONF);
			while (conf_level) {
				accuracy *= (100 - CONF_RANDOM_CHANCE);
				accuracy /= 100;
				conf_level--;
			}

			/* Powerful monster */
			if (monster_is_powerful(mon)) {
				rad++;
			}

			flg |= PROJECT_PLAY;
			flg &= ~(PROJECT_STOP | PROJECT_THRU);

			if (randint1(100) > accuracy) {
				/* Confused direction */
				int dir = randint1(9);
				target = loc_sum(mon->grid, ddgrid[dir]);
			} else if (t_mon) {
				/* Target monster */
				target = t_mon->grid;
			} else {
				/* Target player */
				if (monster_is_decoyed(mon)) {
					target = cave_find_decoy(cave);
				} else {
					target = player->grid;
				}
			}

			break;
		}

		case SRC_TRAP: {
			struct trap *trap = context->origin.which.trap;
			flg |= PROJECT_PLAY;
			target = trap->grid;
			break;
		}

		case SRC_PLAYER:
			/* Ask for a target if no direction given */
			if (context->dir == DIR_TARGET && target_okay()) {
				flg &= ~(PROJECT_STOP | PROJECT_THRU);
				target_get(&target);
			} else {
				target = loc_sum(player->grid, ddgrid[context->dir]);
			}

			if (context->other) rad += eff_level(player) / context->other;
			break;

		default:
			break;
	}

	/* Aim at the target, explode */
	if (project(context->origin, rad, target, dam, context->subtype, flg, 0, 0, context->obj))
		context->ident = true;

	return true;
}

/**
 * Cast a cloud spell
 * Stop if we hit a monster or the player, act as a ball
 * Allow target mode to pass over monsters
 * Affect grids, objects, monsters and the player (even if player cast)
 */
bool effect_handler_CLOUD(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int rad = context->radius ? context->radius : 2;
	struct loc target = loc(-1, -1);

	int flg = PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAY | PROJECT_SELF;

	/* Player or monster? */
	switch (context->origin.what) {
		case SRC_MONSTER: {
			struct monster *mon = cave_monster(cave, context->origin.which.monster);
			int conf_level, accuracy = 100;
			struct monster *t_mon = monster_target_monster(context);

			assert(mon);

			conf_level = monster_effect_level(mon, MON_TMD_CONF);
			while (conf_level) {
				accuracy *= (100 - CONF_RANDOM_CHANCE);
				accuracy /= 100;
				conf_level--;
			}

			/* Powerful monster */
			if (monster_is_powerful(mon)) {
				rad++;
			}

			flg &= ~(PROJECT_STOP | PROJECT_THRU);

			if (randint1(100) > accuracy) {
				/* Confused direction */
				int dir = randint1(9);
				target = loc_sum(mon->grid, ddgrid[dir]);
			} else if (t_mon) {
				/* Target monster */
				target = t_mon->grid;
			} else {
				/* Target player */
				if (monster_is_decoyed(mon)) {
					target = cave_find_decoy(cave);
				} else {
					target = player->grid;
				}
			}

			break;
		}

		case SRC_TRAP: {
			struct trap *trap = context->origin.which.trap;
			target = trap->grid;
			break;
		}

		case SRC_CHEST_TRAP: {
			target = context->obj->grid;
			break;
		}

		case SRC_PLAYER:
			/* Ask for a target if no direction given */
			if (context->dir == DIR_TARGET && target_okay()) {
				flg &= ~(PROJECT_STOP | PROJECT_THRU);
				target_get(&target);
			} else {
				target = loc_sum(player->grid, ddgrid[context->dir]);
			}

			if (context->other) rad += eff_level(player) / context->other;
			break;

		default:
			break;
	}

	/* Aim at the target, explode */
	if (project(context->origin, rad, target, dam, context->subtype, flg, 0, 0, context->obj))
		context->ident = true;

	return true;
}


/**
 * Breathe an element, in a cone from the breather
 * Affect grids, objects, and monsters
 * context->subtype is element, context->other degrees of arc
 * If context->radius is set it is radius of breath, but it usually isn't
 */
bool effect_handler_BREATH(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, false);
	int type = context->subtype;

	struct loc target = loc(-1, -1);

	/* Diameter of source starts at 4, so full strength up to 3 grids from
	 * the breather. */
	int diameter_of_source = 4;

	/* Minimum breath width is 20 degrees */
	int degrees_of_arc = MAX(context->other, 20);

	int flg = PROJECT_ARC | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Distance breathed generally has no fixed limit. */
	int rad = context->radius ? context->radius : z_info->max_range;

	/* Player or monster? */
	if (context->origin.what == SRC_MONSTER) {
		struct monster *mon = cave_monster(cave, context->origin.which.monster);
		struct monster *t_mon = monster_target_monster(context);
		int conf_level, accuracy = 100;

		flg |= PROJECT_PLAY;

		conf_level = monster_effect_level(mon, MON_TMD_CONF);
		while (conf_level) {
			accuracy *= (100 - CONF_RANDOM_CHANCE);
			accuracy /= 100;
			conf_level--;
		}

		if (randint1(100) > accuracy) {
			/* Confused direction. */
			int dir = randint1(9);

			target = loc_sum(mon->grid, ddgrid[dir]);
		} else if (t_mon) {
			/* Target monster. */
			target = t_mon->grid;
		} else {
			/* Target player. */
			if (monster_is_decoyed(mon)) {
				target = cave_find_decoy(cave);
			} else {
				target = player->grid;
			}
		}

		dam = breath_dam(type, mon->hp);

		/* Powerful monster */
		if (monster_is_powerful(mon)) {
			/* Breath is now full strength at 5 grids */
			diameter_of_source *= 3;
			diameter_of_source /= 2;
		}
	} else if (context->origin.what == SRC_PLAYER) {
		msgt(projections[type].msgt, "You breathe %s.", projections[type].desc);

		/* Ask for a target if no direction given */
		if (context->dir == DIR_TARGET && target_okay()) {
			target_get(&target);
		} else {
			target = loc_sum(player->grid, ddgrid[context->dir]);
		}
	}

	/* Adjust the diameter of the energy source */
	if (degrees_of_arc < 60) {
		/* Narrower cone means energy drops off less quickly. We now have:
		 * - 30 degree regular breath  | full strength at 7 grids
		 * - 30 degree powerful breath | full strength at 11 grids
		 * - 20 degree regular breath  | full strength at 11 grids
		 * - 20 degree powerful breath | full strength at 17 grids
		 * where grids are measured from the breather. */
		diameter_of_source = diameter_of_source * 60 / degrees_of_arc;

		/* Max */
		if (diameter_of_source > 25)
			diameter_of_source = 25;
	}

	/* Breathe at the target */
	if (project(context->origin, rad, target, dam, type, flg, degrees_of_arc,
				diameter_of_source, context->obj))
		context->ident = true;

	return true;
}

/**
 * Cast an arc-shaped spell.  This is nothing more than a sphere spell
 * centered on the caster with a value for degrees_of_arc (how many degrees
 * wide the arc is) that is not 360, essentially the same as a breath.
 * The direction given will be the center of the arc, which travels outwards
 * from the caster to a distance given by rad. -LM-
 *
 * Because all arcs start out as being one grid wide, arc spells with a
 * value for degrees_of_arc less than (roughly) 60 do not dissipate as
 * quickly.
 *
 * Affect grids, objects, and monsters
 * context->subtype is element, context->radius radius,
 * context->other degrees of arc (minimum 20)
 */
bool effect_handler_ARC(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int type = context->subtype;
	int rad = context->radius;

	struct loc target = loc(-1, -1);

	/* Diameter of source starts at 4, so full strength up to 3 grids from
	 * the caster. */
	int diameter_of_source = 4;

	/* Short beams now have their own effect, so we set a minimum arc width */
	int degrees_of_arc = MAX(context->other, 20);

	int flg = PROJECT_ARC | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Radius of zero means no fixed limit. */
	if (rad == 0) {
		rad = z_info->max_range;
	}

	/* Player or monster? */
	if (context->origin.what == SRC_MONSTER) {
		flg |= PROJECT_PLAY;
		target =  player->grid;
	} else if (context->origin.what == SRC_PLAYER) {
		/* Ask for a target if no direction given */
		if (context->dir == DIR_TARGET && target_okay()) {
			target_get(&target);
		} else {
			target = loc_sum(player->grid, ddgrid[context->dir]);
		}
	}

	/* Diameter of the energy source. */
	if (degrees_of_arc < 60) {
		diameter_of_source = diameter_of_source * 60 / degrees_of_arc;
	}

	/* Max */
	if (diameter_of_source > 25) {
		diameter_of_source = 25;
	}

	/* Aim at the target */
	if (project(context->origin, rad, target, dam, type, flg, degrees_of_arc,
				diameter_of_source, context->obj)) {
		context->ident = true;
	}

	return true;
}

/**
 * Cast an defined length beam spell.
 *
 * Affect grids, objects, and monsters
 * context->subtype is element, context->radius radius
 * context->other allows an added radius of 1 every time the player level
 * increases by a multiple of context->other, and will only take effect for
 * player spells
 */
bool effect_handler_SHORT_BEAM(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, false);
	int type = context->subtype;
	bool add = (context->origin.what == SRC_PLAYER) && (context->other > 0);
	int rad = context->radius + (add ? eff_level(player) / context->other : 0);

	struct loc target = loc(-1, -1);

	/* Diameter of source is the same as the radius, so the effect is
	 * essentially full strength for its entire length. */
	int diameter_of_source = rad;

	int flg = PROJECT_ARC | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Player or monster? */
	if (context->origin.what == SRC_MONSTER) {
		flg |= PROJECT_PLAY;
		target = player->grid;
	} else if (context->origin.what == SRC_PLAYER) {
		/* Ask for a target if no direction given */
		if (context->dir == DIR_TARGET && target_okay()) {
			target_get(&target);
		} else {
			target = loc_sum(player->grid, ddgrid[context->dir]);
		}
	}

	/* Check bounds */
	if (diameter_of_source > 25) {
		diameter_of_source = 25;
	}

	/* Aim at the target */
	if (project(context->origin, rad, target, dam, type, flg, 0,
				diameter_of_source, context->obj)) {
		context->ident = true;
	}

	return true;
}

/**
 * Crack a whip, or spit at the player; actually just a finite length beam
 * Affect grids, objects, and monsters
 * context->radius is length of beam
 */
bool effect_handler_LASH(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, false);
	int rad = context->radius;

	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_ARC;
	int type;

	struct loc target = loc(-1, -1);

	/* Diameter of source is the same as the radius, so the effect is
	 * essentially full strength for its entire length. */
	int diameter_of_source = rad;

	/* Monsters only */
	if (context->origin.what == SRC_MONSTER) {
		struct monster *mon = cave_monster(cave, context->origin.which.monster);
		struct monster *t_mon = monster_target_monster(context);
		int i;

		flg |= PROJECT_PLAY;

		/* Target player or monster? */
		if (t_mon) {
			target = t_mon->grid;
		} else {
			if (monster_is_decoyed(mon)) {
				target = cave_find_decoy(cave);
			} else {
				target = player->grid;
			}
		}

		/* Paranoia */
		if (rad > z_info->max_range) rad = z_info->max_range;

		/* Get the type (default is PROJ_MISSILE) */
		type = mon->race->blow[0].effect->lash_type;

		/* Scan through all blows for damage */
		for (i = 0; i < z_info->mon_blows_max; i++) {
			/* Extract the attack infomation */
			random_value dice = mon->race->blow[i].dice;

			/* Full damage of first blow, plus half damage of others */
			dam += randcalc(dice, mon->race->level, RANDOMISE) / (i ? 2 : 1);
			if (!mon->race->blow[i].next) break;
		}

		/* No damaging blows */
		if (!dam) return false;
	} else {
		return false;
	}

	/* Check bounds */
	if (diameter_of_source > 25) {
		diameter_of_source = 25;
	}

	/* Lash the target */
	if (project(context->origin, rad, target, dam, type, flg, 0,
				diameter_of_source, context->obj)) {
		context->ident = true;
	}

	return true;
}

/**
 * Cast multiple non-jumping ball spells at the same target.
 *
 * Targets absolute coordinates instead of a specific monster, so that
 * the death of the monster doesn't change the target's location.
 */
bool effect_handler_SWARM(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int num = context->value.m_bonus;

	struct loc target = loc_sum(player->grid, ddgrid[context->dir]);

	int flg = PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Ask for a target if no direction given (early detonation) */
	if ((context->dir == DIR_TARGET) && target_okay()) {
		target_get(&target);
	}

	while (num--) {
		/* Aim at the target.  Hurt items on floor. */
		if (project(source_player(), context->radius, target, dam,
					context->subtype, flg, 0, 0, context->obj))
			context->ident = true;
	}

	return true;
}

/**
 * Strike the target with a ball from above
 */
bool effect_handler_STRIKE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	/*
	 * If the target is not a passable grid in the line of sight or is a
	 * direction, target the player.
	 */
	struct loc target = player->grid;
	/* Aim at the target.  Hurt items on floor. */
	int flg = PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	if (context->dir == DIR_TARGET && target_okay()) {
		target_get(&target);
		if (!projectable(cave, player->grid, target, PROJECT_NONE)) {
			target = player->grid;
		}
	}

	if (project(source_player(), context->radius, target, dam, context->subtype,
				flg, 0, 0, context->obj)) {
		context->ident = true;
	}

	return true;
}

/**
 * Cast a line spell in every direction
 * Stop if we hit a monster, act as a ball
 * Affect grids, objects, and monsters
 */
bool effect_handler_STAR(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int i;
	struct loc target;

	int flg = PROJECT_THRU | PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;

	/* Describe */
	if (!player->timed[TMD_BLIND])
		msg("Light shoots in all directions!");

	for (i = 0; i < 8; i++) {
		/* Use the current direction */
		target = loc_sum(player->grid, ddgrid_ddd[i]);

		/* Aim at the target */
		if (project(source_player(), 0, target, dam, context->subtype, flg, 0,
					0, context->obj))
			context->ident = true;
	}

	return true;
}

/**
 * Cast a ball spell in every direction
 * Stop if we hit a monster, act as a ball
 * Affect grids, objects, and monsters
 */
bool effect_handler_STAR_BALL(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int i;
	struct loc target;

	int flg = PROJECT_STOP | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	for (i = 0; i < 8; i++) {
		/* Use the current direction */
		target = loc_sum(player->grid, ddgrid_ddd[i]);

		/* Aim at the target, explode */
		if (project(source_player(), context->radius, target, dam,
					context->subtype, flg, 0, 0, context->obj))
			context->ident = true;
	}
	return true;
}

/**
 * Special code for staff of starlight and the Staff of Starfire.  Most 
 * effective against monsters that start out in darkness, and against 
 * those who hate light. -LM-
 */
bool effect_handler_STAR_BURST(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int i, j, radius = context->radius, burst_number = 7 + randint0(8);
	struct loc grid = loc(0, 0);
	int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL |
		PROJECT_JUMP;

	/* Is the player in a square already magically lit? */
	bool player_lit = square_isglow(cave, player->grid);

	context->ident = true;
	for (i = 0; i < burst_number; i++) {
		/* First, we find the spot. */
		for (j = 0; j < 20; j++) {
			/* Pick a (scattered) distance. */
			int d = 2 + randint0(4);

			/* Admit failure.  Switch to Plan B. */
			if (j == 19) {
				grid = player->grid;
				break;
			}

			/* Pick a location */
			scatter(cave, &grid, player->grid, d, true);

			/* Not on top of the player. */
			if (loc_eq(grid, player->grid)) continue;

			/* Require passable terrain */
			if (!square_ispassable(cave, grid)) continue;

			/* Spot chosen. */
			break;
		}

		/* Then we hit the spot. */

		/* Confusing to be suddenly lit up. */
		if (!square_isglow(cave, grid)) {
			project(source_player(), radius, grid, dam, PROJ_MON_CONF, flg, 0,
					0, NULL);
		}

		/* The actual burst of light. */
		project(source_player(), radius + 1, grid, dam, PROJ_LIGHT_WEAK, flg, 0,
				0, NULL);
		project(source_player(), radius, grid, dam, PROJ_LIGHT, flg, 0,	0,NULL);

		/* Hack - assume that the player's square is typical of the area, and
		 * only light those squares that weren't already magically lit
		 * temporarily. */
		if (!player_lit) {
			project(source_player(), radius + 1, grid, dam, PROJ_LIGHT_WEAK,
					flg, 0, 0, NULL);
		}
	}
	return true;
}

/**
 * Unleash the wrath of the beings of Air. -LM-
 */
bool effect_handler_AIR_SMITE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, true);
	int i, j;
	struct loc grid = loc(0, 0);
	int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL |
		PROJECT_JUMP;

	context->ident = true;

	/* Due warning. */
	msg("The powers of Air rain down destruction!");

	/* Multiple gravity, light, and electricity balls. */
	for (i = 0; i < 8; i++) {
		/* First, we find the spot. */
		for (j = 0; j < 20; j++) {
			/* Pick a (short) distance. */
			int d = randint1(3);

			/* Admit failure.  Switch to Plan B. */
			if (j == 19) {
				grid = player->grid;
				break;
			}

			/* Pick a location */
			scatter(cave, &grid, player->grid, d, true);

			/* Not on top of the player. */
			if (loc_eq(grid, player->grid)) continue;

			/* Require passable terrain */
			if (!square_ispassable(cave, grid)) continue;

			/* Slight preference for actual monsters. */
			if (square_monster(cave, grid)) {
				break;
			} else if (j > 3) {
				/* Will accept any passable grid after a few tries. */
				break;
			}
		}

		/* Choice of 3 */
		if (one_in_(3)) {
			project(source_player(), 1, grid, dam, PROJ_GRAVITY, flg, 0, 0,
					NULL);
		} else if (one_in_(2)) {
			project(source_player(), 1, grid, dam, PROJ_LIGHT, flg, 0, 0, NULL);
		} else {
			project(source_player(), 1, grid, dam, PROJ_ELEC, flg, 0, 0, NULL);
		}

		/* This is a bombardment.  Make it look like one. */
		event_signal_pause(EVENT_PAUSE, 10);
	}
	return true;
}

/**
 * Cast a chain of bolts which leap from monster to monster
 * Affect monsters
 */
bool effect_handler_CHAIN(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, false);
	int k, strikes = 2 + randint1(10);
	struct loc grid, target, cur = player->grid;
	int pick, avail_mon_num;
	int flag = PROJECT_STOP | PROJECT_KILL;
	struct monster *avail_mon[100];
	struct source src = source_player();

	/* Initialise */
	for (k = 0; k < 100; k++) {
		avail_mon[k] = NULL;
	}
	target = cur;

	/* Start striking */
	for (k = 0; k < strikes; k++) {
		/* No targets yet */
		avail_mon_num = 0;

		/* Find something in range */
		for (grid.y = cur.y - 5; grid.y <= cur.y + 5; grid.y++) {
			for (grid.x = cur.x - 5; grid.x <= cur.x + 5; grid.x++) {
				int dist = distance(cur, grid);

				/* Skip distant grids */
				if (dist > 5) continue;

				/* Skip grids that are out of bounds */
				if (!square_in_bounds_fully(cave, grid)) continue;

				/* Skip grids that are not projectable */
				if (!projectable(cave, cur, grid, flag)) continue;

				/* Skip grids with no monster (including player) */
				if (!square_monster(cave, grid)) continue;

				/* Record the monster */
				avail_mon[avail_mon_num++] = square_monster(cave, grid);
			}
		}

		/* Maybe we're at a dead end */
		if (!avail_mon_num)	return true;

		/* Pick a target... */
		pick = randint0(avail_mon_num);
		target = avail_mon[pick]->grid;

		/* Paranoia */
		if (!square_monster(cave, target)) return true;

		/* ...and hit it */
		project(src, 1, target, dam, context->subtype, flag, 0, 0, NULL);

		/* Set next source (current monster may be dead) */
		cur = target;
		if (square_monster(cave, cur) == avail_mon[pick]) {
			src = source_monster(avail_mon[pick]->midx);
		} else {
			src = source_grid(cur);
		}
	}

	return true;
}

/**
 * Apply a "project()" directly to all viewable monsters.  If context->other is
 * set, the effect damage boost is applied.  This is a hack - NRM
 *
 * Note that affected monsters are NOT auto-tracked by this usage.
 */
bool effect_handler_PROJECT_LOS(effect_handler_context_t *context)
{
	int i;
	int dam = effect_calculate_value(context, context->other ? true : false);
	int typ = context->subtype;
	struct loc origin = origin_get_loc(context->origin);
	int flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;

	/* Affect all (nearby) monsters */
	for (i = 1; i < cave_monster_max(cave); i++) {
		struct monster *mon = cave_monster(cave, i);

		/* Paranoia -- Skip dead monsters */
		if (!mon->race) continue;

		/* Don't affect the caster */
		if (mon->midx == cave->mon_current) continue;

		/* Require line of sight */
		if (!los(cave, origin, mon->grid)) continue;

		/* Jump directly to the monster */
		(void)project(source_player(), 0, mon->grid, dam, typ, flg, 0, 0,
					  context->obj);
		context->ident = true;
	}

	/* Result */
	return true;
}

/**
 * Just like PROJECT_LOS except the player's awareness of an object using
 * this effect is relevant.
 *
 * Note that affected monsters are NOT auto-tracked by this usage.
 */
bool effect_handler_PROJECT_LOS_AWARE(effect_handler_context_t *context)
{
	int i;
	int dam = effect_calculate_value(context, context->other ? true : false);
	int typ = context->subtype;

	int flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;

	if (context->aware) flg |= PROJECT_AWARE;

	/* Affect all (nearby) monsters */
	for (i = 1; i < cave_monster_max(cave); i++) {
		struct monster *mon = cave_monster(cave, i);
		struct loc grid;

		/* Paranoia -- Skip dead monsters */
		if (!mon->race) continue;

		/* Don't affect the caster */
		if (mon->midx == cave->mon_current) continue;

		/* Location */
		grid = mon->grid;

		/* Require line of sight */
		if (!square_isview(cave, grid)) continue;

		/* Jump directly to the target monster */
		(void)project(source_player(), 0, grid, dam, typ, flg, 0, 0, context->obj);
		context->ident = true;
	}

	/* Result */
	return true;
}

/**
 * The destruction effect
 *
 * This effect "deletes" monsters (instead of killing them).
 *
 * This is always an effect centred on the player; it is similar to the
 * earthquake effect.
 */
bool effect_handler_DESTRUCTION(effect_handler_context_t *context)
{
	int k, r = context->radius;
	int elem = context->subtype;
	int py = player->grid.y;
	int px = player->grid.x;
	struct loc grid;

	context->ident = true;

	/* No effect in town or arena */
	if ((!player->depth) || (player->upkeep->arena_level)) {
		msg("The ground shakes for a moment.");
		return true;
	}

	/* Big area of affect */
	for (grid.y = (py - r); grid.y <= (py + r); grid.y++) {
		for (grid.x = (px - r); grid.x <= (px + r); grid.x++) {
			/* Skip illegal grids */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Extract the distance */
			k = distance(loc(px, py), grid);

			/* Stay in the circle of death */
			if (k > r) continue;

			/* Lose room and vault */
			sqinfo_off(square(cave, grid)->info, SQUARE_ROOM);
			sqinfo_off(square(cave, grid)->info, SQUARE_VAULT);

			/* Forget completely */
			if (!square_isbright(cave, grid)) {
				sqinfo_off(square(cave, grid)->info, SQUARE_GLOW);
			}
			sqinfo_off(square(cave, grid)->info, SQUARE_SEEN);
			square_forget(cave, grid);
			square_light_spot(cave, grid);

			/* Deal with player later */
			if (loc_eq(grid, player->grid)) continue;

			/* Delete the monster (if any) */
			delete_monster(cave, grid);

			/* Destroy any grid that isn't permament */
			if (!square_ispermanent(cave, grid)) {
				/* Deal with artifacts */
				struct object *obj = square_object(cave, grid);
				while (obj) {
					if (obj->artifact) {
						if (OPT(player, birth_lose_arts) ||
							obj_is_known_artifact(obj)) {
							history_lose_artifact(player, obj->artifact);
							mark_artifact_created(
								obj->artifact,
								true);
						} else {
							mark_artifact_created(
								obj->artifact,
								false);
						}
					}
					obj = obj->next;
				}

				/* Delete objects */
				square_excise_all_imagined(player->cave, cave,
					grid);
				square_excise_pile(player->cave, grid);
				square_excise_pile(cave, grid);
				square_destroy(cave, grid);
			}
		}
	}

	/* Player is affected */
	if (elem == ELEM_LIGHT) {
		msg("There is a searing blast of light!");
		equip_learn_element(player, ELEM_LIGHT);
		if (!player_resists_effects(player->state, ELEM_LIGHT)) {
			(void)player_inc_timed(player, TMD_BLIND, 10 + randint1(10), true,
								   true, true);
		}
	} else if (elem == ELEM_DARK) {
		msg("Darkness seems to crush you!");
		equip_learn_element(player, ELEM_DARK);
		if (!player_resists_effects(player->state, ELEM_DARK)) {
			(void)player_inc_timed(player, TMD_BLIND, 10 + randint1(10), true,
								   true, true);
		}
	}

	/* Fully update the visuals */
	player->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Redraw monster list */
	player->upkeep->redraw |= (PR_MONLIST | PR_ITEMLIST);

	return true;
}

/**
 * Induce an earthquake of the radius context->radius centred on the instigator.
 *
 * This will turn some walls into floors and some floors into walls.
 *
 * The player will take damage and jump into a safe grid if possible,
 * otherwise, he will tunnel through the rubble instantaneously.
 *
 * Monsters will take damage, and jump into a safe grid if possible,
 * otherwise they will be buried in the rubble, disappearing from
 * the level in the same way that they do when banished.
 *
 * Note that players and monsters (except eaters of walls and passers
 * through walls) will never occupy the same grid as a wall (or door).
 */
bool effect_handler_EARTHQUAKE(effect_handler_context_t *context)
{
	int r = context->radius;
	bool targeted = context->subtype ? true : false;

	struct loc pgrid = player->grid;
	int i, y, x;
	struct loc offset, safe_grid = loc(0, 0);
	int safe_grids = 0;
	int damage = 0;
	bool hurt = false;
	bool display_dam = context->origin.what == SRC_PLAYER
		&& OPT(player, show_damage);
	bool map[32][32];

	struct loc centre = origin_get_loc(context->origin);

	context->ident = true;

	/* Sometimes ask for a target */
	if (targeted) {
		int dir = DIR_TARGET;
		if (!get_aim_dir(&dir)) {
			return false;
		}
		if ((dir == DIR_TARGET) && target_okay()) {
			target_get(&centre);
		}
	}

	if ((player->depth) && ((!player->upkeep->arena_level)
							|| (context->origin.what == SRC_MONSTER))) {
		msg("The ground shakes! The ceiling caves in!");
	} else {
		/* No effect in town or arena */
		msg("The ground shakes for a moment.");
		return true;
	}

	/* Paranoia -- Enforce maximum range */
	if (r > 15) r = 15;

	/* Initialize a map of the maximal blast area */
	for (y = 0; y < 32; y++)
		for (x = 0; x < 32; x++)
			map[y][x] = false;

	/* Check around the epicenter */
	for (offset.y = -r; offset.y <= r; offset.y++) {
		for (offset.x = -r; offset.x <= r; offset.x++) {
			/* Extract the location */
			struct loc grid = loc_sum(centre, offset);

			/* Skip illegal grids */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Skip distant grids */
			if (distance(centre, grid) > r) continue;

			/* Lose room and vault */
			sqinfo_off(square(cave, grid)->info, SQUARE_ROOM);
			sqinfo_off(square(cave, grid)->info, SQUARE_VAULT);

			/* Forget completely */
			if (!square_isbright(cave, grid)) {
				sqinfo_off(square(cave, grid)->info, SQUARE_GLOW);
			}
			sqinfo_off(square(cave, grid)->info, SQUARE_SEEN);
			square_forget(cave, grid);
			square_light_spot(cave, grid);

			/* Skip the epicenter */
			if (loc_is_zero(offset)) continue;

			/* Skip most grids */
			if (randint0(100) < 85) continue;

			/* Damage this grid */
			map[16 + grid.y - centre.y][16 + grid.x - centre.x] = true;

			/* Take note of player damage */
			if (loc_eq(grid, pgrid)) hurt = true;
		}
	}

	/* First, determine the effects on the player (if necessary) */
	if (hurt) {
		char dam_text[32] = "";

		/* Check around the player */
		for (i = 0; i < 8; i++) {
			/* Get the location */
			struct loc grid = loc_sum(pgrid, ddgrid_ddd[i]);

			/* Skip non-empty grids - allow pushing into traps and webs */
			if (!square_isopen(cave, grid)) continue;

			/* Important -- Skip grids marked for damage */
			if (map[16 + grid.y - centre.y][16 + grid.x - centre.x]) continue;

			/* Count "safe" grids, apply the randomizer */
			if ((++safe_grids > 1) && (randint0(safe_grids) != 0)) continue;

			/* Save the safe location */
			safe_grid = grid;
		}

		/* Random message */
		switch (randint1(3))
		{
			case 1:
			{
				msg("The cave ceiling collapses on you!");
				break;
			}
			case 2:
			{
				msg("The cave floor twists in an unnatural way!");
				break;
			}
			default:
			{
				msg("The cave quakes!");
				msg("You are pummeled with debris!");
				break;
			}
		}

		/* Hurt the player a lot */
		if (!safe_grids) {
			/* Message and damage */
			damage = player_apply_damage_reduction(player, 300);
			if (damage > 0 && OPT(player, show_damage)) {
				strnfmt(dam_text, sizeof(dam_text), " (%d)",
					damage);
			}
			msg("You are severely crushed!%s", dam_text);
		} else {
			/* Destroy the grid, and push the player to (relative) safety */
			const char *hurt_msg = "";

			switch (randint1(3)) {
				case 1: {
					hurt_msg = "You nimbly dodge the blast!";
					damage = 0;
					break;
				}
				case 2: {
					hurt_msg = "You are bashed by rubble!";
					damage = damroll(10, 4);
					(void)player_inc_timed(player, TMD_STUN,
						randint1(50), true, true, true);
					break;
				}
				case 3: {
					hurt_msg = "You are crushed between the floor and ceiling!";
					damage = damroll(10, 4);
					(void)player_inc_timed(player, TMD_STUN,
						randint1(50), true, true, true);
					break;
				}
			}

			damage = player_apply_damage_reduction(player, damage);
			if (damage > 0 && OPT(player, show_damage)) {
				strnfmt(dam_text, sizeof(dam_text), " (%d)",
					damage);
			}
			msg("%s%s", hurt_msg, dam_text);

			/* Move player */
			monster_swap(pgrid, safe_grid);
			player_handle_post_move(player, true, true);
		}
	}


	/* Examine the quaked region */
	for (offset.y = -r; offset.y <= r; offset.y++) {
		for (offset.x = -r; offset.x <= r; offset.x++) {
			/* Extract the location */
			struct loc grid = loc_sum(centre, offset);

			/* Skip unaffected grids */
			if (!map[16 + grid.y - centre.y][16 + grid.x - centre.x]) continue;

			/* Process monsters */
			if (square(cave, grid)->mon > 0) {
				struct monster *mon = square_monster(cave, grid);

				/* Most monsters cannot co-exist with rock */
				if (!flags_test(mon->race->flags, RF_SIZE, RF_KILL_WALL,
								RF_PASS_WALL, FLAG_END)) {
					int m_dam;

					/* Assume not safe */
					safe_grids = 0;

					/* Monster can move to escape the wall */
					if (!rf_has(mon->race->flags, RF_NEVER_MOVE)) {
						/* Look for safety */
						for (i = 0; i < 8; i++) {
							/* Get the grid */
							struct loc safe = loc_sum(grid, ddgrid_ddd[i]);

							/* Skip non-empty grids */
							if (!square_isempty(cave, safe)) continue;

							/* No safety on glyph of warding */
							if (square_iswarded(cave, safe)) continue;

							/* Important -- Skip quake grids */
							if (map[16 + safe.y - centre.y]
								[16 + safe.x - centre.x]) continue;

							/* Count safe grids, apply the randomizer */
							if ((++safe_grids > 1) &&
								(randint0(safe_grids) != 0))
								continue;

							/* Save the safe grid */
							safe_grid = safe;
						}
					}

					/* Take damage from the quake */
					m_dam = (safe_grids ? damroll(4, 8) : (mon->hp + 1));

					/* Monster is certainly awake, not thinking about player */
					monster_wake(mon, false, 0);

					/* Apply damage directly */
					mon->hp -= m_dam;

					if (mon->hp < 0) {
						if (display_dam) {
							add_monster_message_show_damage(
								mon,
								MON_MSG_QUAKE_DEATH,
								false,
								m_dam);
						} else {
							add_monster_message(mon,
								MON_MSG_QUAKE_DEATH,
								false);
						}

						/*
						 * Delete (not kill) "dead"
						 * monsters.
						 */
						delete_monster(cave, grid);
					} else {
						if (display_dam) {
							add_monster_message_show_damage(
								mon,
								MON_MSG_QUAKE_HURT,
								false,
								m_dam);
						} else {
							add_monster_message(mon,
								MON_MSG_QUAKE_HURT,
								false);
						}

						/* Escape from the rock */
						if (safe_grids) {
							/* Move the monster */
							monster_swap(grid,
								safe_grid);
						}
					}
				}
			}
		}
	}

	/* Important -- no wall on player */
	if (ABS(player->grid.x - centre.x) <= 15
			&& ABS(player->grid.y - centre.y) <= 15) {
		map[16 + player->grid.y - centre.y][16 + player->grid.x - centre.x] = false;
	}

	/* Examine the quaked region and damage marked grids if possible */
	for (offset.y = -r; offset.y <= r; offset.y++) {
		for (offset.x = -r; offset.x <= r; offset.x++) {
			/* Extract the location */
			struct loc grid = loc_sum(centre, offset);

			/* Ignore invalid grids */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Note unaffected grids for light changes, etc. */
			if (!map[16 + grid.y - centre.y][16 + grid.x - centre.x])
				square_light_spot(cave, grid);

			/* Destroy location and all objects (if valid) */
			else if (square_changeable(cave, grid)) {
				square_excise_pile(cave, grid);
				square_earthquake(cave, grid);
			}
		}
	}

	/*
	 * Apply damage to player; done here so messages are ordered properly
	 * if the player dies.
	 */
	take_hit(player, damage, "an earthquake");

	/* Fully update the visuals */
	player->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Update the health bar */
	player->upkeep->redraw |= (PR_HEALTH);

	/* Window stuff */
	player->upkeep->redraw |= (PR_MONLIST | PR_ITEMLIST);

	return true;
}

/**
 * Turn everything in the given radius to lava.
 *
 * To do: allow other grids (eg water)
 */
bool effect_handler_LAVA_POOL(effect_handler_context_t *context)
{
	int r = context->radius;
	struct loc grid, pgrid = player->grid;

	/* Everything in range */
	for (grid.y = pgrid.y - r; grid.y <= pgrid.y + r; grid.y++) {
		for (grid.x = pgrid.x - r; grid.x <= pgrid.x + r; grid.x++) {
			int dist = distance(pgrid, grid);

			/* Skip distant grids */
			if (dist > r) continue;

			/* Skip grids that are out of bounds */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Skip grids that are permanent */
			if (square_ispermanent(cave, grid)) continue;

			/* Skip grids in vaults */
			if (square_isvault(cave, grid)) continue;

			/* Lava now */
			square_set_feat(cave, grid, FEAT_LAVA);
		}
	}
	return true;
}

/**
 * Suck energy from nearby monsters.
 *
 * To do: unite with LAVA_POOL
 */
bool effect_handler_ENERGY_DRAIN(effect_handler_context_t *context)
{
	struct loc grid, pgrid = player->grid;
	int r = context->radius;

	/* Check everything in range */
	for (grid.y = pgrid.y - r; grid.y <= pgrid.y + r; grid.y++) {
		for (grid.x = pgrid.x - r; grid.x <= pgrid.x + r; grid.x++) {
			int dist = distance(pgrid, grid);
			struct monster *mon = NULL;

			/* Skip distant grids */
			if (dist > r) continue;

			/* Skip grids that are out of bounds */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Skip grids the player can't see */
			if (!square_isview(cave, grid)) continue;

			/* Get the monster, skip if none */
			mon = square_monster(cave, grid);
			if (!mon) continue;

			/* Take the energy */
			player->energy += mon->energy;
			mon->energy = 0;
		}
	}

	return true;
}

/**
 * Grow trees wherever you can see.
 *
 * To do: unite with LAVA_POOL
 */
bool effect_handler_GROW_FOREST(effect_handler_context_t *context)
{
	struct loc grid, pgrid = player->grid;
	int r = z_info->max_sight;

	/* Check everything in line of sight */
	for (grid.y = pgrid.y - r; grid.y <= pgrid.y + r; grid.y++) {
		for (grid.x = pgrid.x - r; grid.x <= pgrid.x + r; grid.x++) {
			int dist = distance(pgrid, grid);

			/* Skip distant grids */
			if (dist > r) continue;

			/* Skip grids that are out of bounds */
			if (!square_in_bounds_fully(cave, grid)) continue;

			/* Skip grids the player can't see */
			if (!square_isview(cave, grid)) continue;

			/* Skip grids that aren't floor or road */
			if (!square_isfloor(cave, grid)) continue;

			/* Grow a tree */
			if (player->depth < 40) {
				square_set_feat(cave, grid, FEAT_TREE);
			} else {
				square_set_feat(cave, grid, FEAT_TREE2);
			}
		}
	}

	return true;
}

/**
 * Draw energy from a nearby undead
 */
bool effect_handler_TAP_UNLIFE(effect_handler_context_t *context)
{
	int amount = effect_calculate_value(context, false);
	struct loc target;
	struct monster *mon = NULL;
	char m_name[80];
	int drain = 0;
	bool fear = false;
	bool dead = false;

	context->ident = true;

	/* Closest living monster */
	if (!target_set_closest(TARGET_KILL, monster_is_undead)) {
		return false;
	}
	target_get(&target);
	mon = target_get_monster();

	/* Hurt the monster */
	drain = MIN(mon->hp, amount) / 4;
	monster_desc(m_name, sizeof(m_name), mon, MDESC_TARG);
	if (OPT(player, show_damage)) {
		msg("You draw power from the %s. (%d)", m_name, amount);
	} else {
		msg("You draw power from the %s.", m_name);
	}
	dead = mon_take_hit(mon, player, amount, &fear, " is destroyed!");

	/* Gain mana */
	effect_simple(EF_RESTORE_MANA, context->origin, format("%d", drain), 0, 0,
				  0, 0, 0, NULL);

	if (dead) {
		/* Cancel the targeting of the dead creature. */
		target_set_location(0, 0);
	} else if (monster_is_visible(mon)) {
		/* Handle fear for surviving monsters */
		message_pain(mon, amount);
		if (fear) {
			add_monster_message(mon, MON_MSG_FLEE_IN_TERROR, true);
		}
	}

	return true;
}

/**
 * Curse a monster for direct damage
 */
bool effect_handler_CURSE(effect_handler_context_t *context)
{
	int dam = effect_calculate_value(context, false);
	struct monster *mon = target_get_monster();
	bool fear = false;
	bool dead = false;
	bool display_dam = OPT(player, show_damage);
	char note[32];
	const char *passed_note;

	context->ident = true;

	/* Need to choose a monster, not just point */
	if (!mon) {
		msg("No monster selected!");
		return false;
	}

	/* Hit it */
	if (display_dam) {
		strnfmt(note, sizeof(note), " dies! (%d)", dam);
		passed_note = note;
	} else {
		passed_note = " dies!";
	}
	dead = mon_take_hit(mon, player, dam, &fear, passed_note);

	/* Handle fear for surviving monsters */
	if (!dead && monster_is_visible(mon)) {
		if (display_dam) {
			message_pain_show_damage(mon, dam);
		} else {
			message_pain(mon, dam);
		}
		if (fear) {
			add_monster_message(mon, MON_MSG_FLEE_IN_TERROR, true);
		}
	}

	return true;
}

/**
 * Jump next to a living monster and draw hitpoints and nourishment from it
 */
bool effect_handler_JUMP_AND_BITE(effect_handler_context_t *context)
{
	int amount = effect_calculate_value(context, false);
	struct loc victim, grid;
	int d, first_d = randint0(8);
	struct monster *mon = NULL;
	char m_name[80];
	int drain = 0;
	bool fear = false;
	bool dead = false;

	context->ident = true;

	/* Closest living monster */
	if (!target_set_closest(TARGET_KILL, monster_is_living)) {
		return false;
	}
	target_get(&victim);
	mon = target_get_monster();
	monster_desc(m_name, sizeof(m_name), mon, MDESC_TARG);

	/* Look next to the monster */
	for (d = first_d; d < first_d + 8; d++) {
		grid = loc_sum(victim, ddgrid_ddd[d % 8]);
		if (square_isplayertrap(cave, grid)) continue;
		if (square_iswebbed(cave, grid)) continue;
		if (square_isopen(cave, grid)) break;
	}

	/* Needed to be adjacent */
	if (d == first_d + 8) {
		msg("Not enough room next to %s!", m_name);
		return false;
	}

	/* Sound */
	sound(MSG_TELEPORT);

	/* Move player */
	monster_swap(player->grid, grid);
	player_handle_post_move(player, true, false);

	/* Now bite it */
	drain = MIN(mon->hp + 1, amount);
	assert(drain > 0);
	if (OPT(player, show_damage)) {
		msg("You bite %s. (%d)", m_name, drain);
	} else {
		msg("You bite %s.", m_name);
	}
	dead = mon_take_hit(mon, player, amount, &fear, " is drained dry!");

	/* Heal and nourish */
	effect_simple(EF_HEAL_HP, context->origin, format("%d", drain), 0, 0, 0,
				  0, 0, NULL);
	player_inc_timed(player, TMD_FOOD, drain, false, false, false);

	if (dead) {
		/* Cancel the targeting of the dead creature. */
		target_set_location(0, 0);
	} else if (monster_is_visible(mon)) {
		/* Handle fear for surviving monsters */
		message_pain(mon, amount);
		if (fear) {
			add_monster_message(mon, MON_MSG_FLEE_IN_TERROR, true);
		}
	}

	return true;
}

/**
 * Move up to 4 spaces then do melee blows.
 * Could vary the length of the move without much work.
 */
bool effect_handler_MOVE_ATTACK(effect_handler_context_t *context)
{
	int blows = effect_calculate_value(context, false);
	int moves = 4;
	int d, i;
	struct loc target = player->grid;
	struct loc next_grid, grid_diff;
	bool fear;
	struct monster *mon;

	/* Ask for a target */
	if (context->dir == DIR_TARGET) {
		target_get(&target);
	} else {
		target = loc_sum(player->grid, ddgrid[context->dir]);
	}

	mon = square_monster(cave, target);
	if (mon == NULL || !monster_is_obvious(mon)) {
		msg("This spell must target a monster.");
		return false;
	}

	while (distance(player->grid, target) > 1 && moves > 0) {
		int choice[] = { 0, 1, -1 };
		bool attack = false;
		grid_diff = loc_diff(target, player->grid);

		/* Choice of direction simplified by prioritizing diagonals */
		if (grid_diff.x == 0) {
			d = (grid_diff.y < 0) ? 0 : 4; /* up : down */
		} else if (grid_diff.y == 0) {
			d = (grid_diff.x < 0) ? 6 : 2; /* left : right */
		} else if (grid_diff.x < 0) {
			d = (grid_diff.y < 0) ? 7 : 5; /* up-left : down-left */
		} else {/* grid_diff.x > 0 */
			d = (grid_diff.y < 0) ? 1 : 3; /* up-right : down-right */
		}

		/* We'll give up to 3 choices: d, d + 1, d - 1 */
		for (i = 0; i < 3; i++) {
			int d_test = (d + choice[i] + 8) % 8;
			next_grid = loc_sum(player->grid, clockwise_grid[d_test]);
			if (square_ispassable(cave, next_grid)) {
				d = d_test;
				if (square_monster(cave, next_grid)) attack = true;
				break;
			} else if (i == 2) {
				msg("The way is barred.");
				return moves != 4;
			}
		}

		move_player(clockwise_ddd[d], false);
		moves--;
		if (attack) return false;
	}

	/* Reduce blows based on distance traveled, round to nearest blow */
	blows = (blows * moves + 2) / 4;

	/* Should return some energy if monster dies early */
	while (blows-- > 0) {
		if (py_attack_real(player, target, &fear)) break;
	}

	return true;
}

/**
 * Enter single combat with an enemy
 */
bool effect_handler_SINGLE_COMBAT(effect_handler_context_t *context)
{
	struct monster *mon = target_get_monster();
	context->ident = true;

	/* Already in an arena */
	if (player->upkeep->arena_level) {
		msg("You are already in single combat!");
		return false;
	}

	/* Need to choose a monster, not just point */
	if (mon) {
		int old_idx = mon->midx;

		/* Monsters with high spell power can resist */
		if (randint0(mon->race->spell_power) > eff_level(player)) {
			char m_name[80];
			monster_desc(m_name, sizeof(m_name), mon,
				MDESC_CAPITAL | MDESC_COMMA);
			msg("%s resists!", m_name);
			return true;
		}

		/* Swap the targeted monster with the first in the monster list */
		if (old_idx == 1) {
			/* Do nothing */
			;
		} else if (cave_monster(cave, 1)->race) {
			monster_index_move(cave, old_idx, cave_monster_max(cave));
			monster_index_move(cave, 1, old_idx);
			monster_index_move(cave, cave_monster_max(cave), 1);
		} else {
			monster_index_move(cave, old_idx, 1);
		}
		target_set_monster(cave_monster(cave, 1));
		player->upkeep->health_who = cave_monster(cave, 1);
	} else {
		msg("No monster selected!");
		return false;
	}

	/* Head to the arena */
	player->upkeep->arena_level = true;
	player->old_grid = player->grid;
	player_change_place(player, player->place);
	return true;
}


bool effect_handler_MELEE_BLOWS(effect_handler_context_t *context)
{
	int blows = effect_calculate_value(context, false);
	int dam = context->radius;
	bool fear;
	int taim;
	struct loc target = loc(-1, -1);
	struct loc grid = player->grid;
	struct monster *mon = NULL;

	/* players only for now */
	if (context->origin.what != SRC_PLAYER)
		return false;

	/* Ask for a target if no direction given */
	if (context->dir == DIR_TARGET && target_okay()) {
		target_get(&target);
	} else {
		target = loc_sum(player->grid, ddgrid[context->dir]);
	}

	/* Check target validity */
	taim = distance(grid, target);
	mon = square_monster(cave, target);
	if (taim > 1) {
		msgt(MSG_GENERIC, "Target too far away (%d).", taim);
		return false;
	} else if (!mon) {
		msg("You must attack a monster.");
		return false;
	}

	while ((blows-- > 0) && mon) {
		/* Test for damaging the monster */
		int hp = mon->hp;
		if (py_attack_real(player, target, &fear)) return true;
		/*mon = square_monster(cave, target); */
		if (mon && (mon->hp == hp)) continue;

		/* Apply side-effects */
		if (project(context->origin, 0, target, dam, context->subtype,
					PROJECT_KILL, 0, 0, context->obj)) {
			context->ident = true;
		}
	}
	return true;
}

bool effect_handler_SWEEP(effect_handler_context_t *context)
{
	int blows = effect_calculate_value(context, false);
	bool fear;
	int i;
	struct loc target;

	/* Players only for now */
	if (context->origin.what != SRC_PLAYER)	return false;

	/* Doing these like >1 blows means spinning around multiple times. */
	while (blows-- > 0) {
		for (i = 0; i < 8; i++) {
			target = loc_sum(player->grid, clockwise_grid[i]);
			if (square_monster(cave, target) != NULL)
				py_attack_real(player, target, &fear);
		}
	}

	/* Should return some energy if all enemies killed and blows remain? */
	return true;
}

/**
 * The "wonder" effect.
 *
 * This spell should become more useful (more
 * controlled) as the player gains experience levels.
 * Thus, add 1/5 of the player's level to the die roll.
 * This eliminates the worst effects later on, while
 * keeping the results quite random.  It also allows
 * some potent effects only at high level
 */
bool effect_handler_WONDER(effect_handler_context_t *context)
{
	int plev = eff_level(player);
	int die = effect_calculate_value(context, false);
	int subtype = 0, radius = 0, other = 0, y = 0, x = 0;
	int beam = context->beam;
	effect_handler_f handler = NULL;
	random_value value = { 0, 0, 0, 0 };

	context->ident = true;

	if (die > 100)
		msg("You feel a surge of power!");

	if (die < 8) {
		subtype = PROJ_MON_CLONE;
		handler = effect_handler_BOLT;
	} else if (die < 14) {
		subtype = PROJ_MON_SPEED;
		value.base = 100;
		handler = effect_handler_BOLT;
	} else if (die < 26) {
		subtype = PROJ_MON_HEAL;
		value.dice = 4;
		value.sides = 6;
		handler = effect_handler_BOLT;
	} else if (die < 31) {
		subtype = PROJ_MON_POLY;
		value.base = plev;
		handler = effect_handler_BOLT;
	} else if (die < 36) {
		beam -= 10;
		subtype = PROJ_MISSILE;
		value.dice = 3 + ((plev - 1) / 5);
		value.sides = 4;
		handler = effect_handler_BOLT_OR_BEAM;
	} else if (die < 41) {
		subtype = PROJ_MON_CONF;
		value.base = plev;
		handler = effect_handler_BOLT;
	} else if (die < 46) {
		subtype = PROJ_POIS;
		value.base = 20 + plev / 2;
		radius = 3;
		handler = effect_handler_BALL;
	} else if (die < 51) {
		subtype = PROJ_LIGHT_WEAK;
		value.dice = 6;
		value.sides = 8;
		handler = effect_handler_LINE;
	} else if (die < 56) {
		subtype = PROJ_ELEC;
		value.dice = 3 + ((plev - 5) / 6);
		value.sides = 6;
		handler = effect_handler_BEAM;
	} else if (die < 61) {
		beam -= 10;
		subtype = PROJ_COLD;
		value.dice = 5 + ((plev - 5) / 4);
		value.sides = 8;
		handler = effect_handler_BOLT_OR_BEAM;
	} else if (die < 66) {
		subtype = PROJ_ACID;
		value.dice = 6 + ((plev - 5) / 4);
		value.sides = 8;
		handler = effect_handler_BOLT_OR_BEAM;
	} else if (die < 71) {
		subtype = PROJ_FIRE;
		value.dice = 8 + ((plev - 5) / 4);
		value.sides = 8;
		handler = effect_handler_BOLT_OR_BEAM;
	} else if (die < 76) {
		subtype = PROJ_MON_DRAIN;
		value.base = 75;
		handler = effect_handler_BOLT;
	} else if (die < 81) {
		subtype = PROJ_ELEC;
		value.base = 30 + plev / 2;
		radius = 2;
		handler = effect_handler_BALL;
	} else if (die < 86) {
		subtype = PROJ_ACID;
		value.base = 40 + plev;
		radius = 2;
		handler = effect_handler_BALL;
	} else if (die < 91) {
		subtype = PROJ_ICE;
		value.base = 70 + plev;
		radius = 3;
		handler = effect_handler_BALL;
	} else if (die < 96) {
		subtype = PROJ_FIRE;
		value.base = 80 + plev;
		radius = 3;
		handler = effect_handler_BALL;
	} else if (die < 101) {
		subtype = PROJ_MON_DRAIN;
		value.base = 100 + plev;
		handler = effect_handler_BOLT;
	} else if (die < 104) {
		radius = 12;
		handler = effect_handler_EARTHQUAKE;
	} else if (die < 106) {
		radius = 15;
		handler = effect_handler_DESTRUCTION;
	} else if (die < 108) {
		handler = effect_handler_BANISH;
	} else if (die < 110) {
		subtype = PROJ_DISP_ALL;
		value.base = 120;
		handler = effect_handler_PROJECT_LOS;
	}

	if (handler != NULL) {
		effect_handler_context_t new_context = {
			context->effect,
			context->origin,
			context->obj,
			context->aware,
			context->dir,
			beam,
			context->boost,
			value,
			subtype, radius, other, y, x,
			NULL,
			context->ident,
			context->cmd
		};

		return handler(&new_context);
	} else {
		/* RARE */
		effect_simple(EF_PROJECT_LOS, context->origin, "150", PROJ_DISP_ALL, 0, 0, 0, 0, NULL);
		effect_simple(EF_PROJECT_LOS, context->origin, "20", PROJ_MON_SLOW, 0, 0, 0, 0, NULL);
		effect_simple(EF_PROJECT_LOS, context->origin, "40", PROJ_SLEEP_ALL, 0, 0, 0, 0, NULL);
		effect_simple(EF_HEAL_HP, context->origin, "300", 0, 0, 0, 0, 0, NULL);

		return true;
	}
}

/**
 * Wand of Unmaking activation
 */
bool effect_handler_UNMAKE(effect_handler_context_t *context)
{
	bool repeat = true;

	context->ident = true;

	while (repeat) {
		/* Pick an effect. */
		int chaotic_effect = randint0(18);

		switch (chaotic_effect) {
			/* Massive chaos bolt. */
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			{
				int flg = PROJECT_STOP | PROJECT_KILL;
				(void) project_aimed(context->origin, PROJ_CHAOS, context->dir,
									 randint1(500), flg, NULL);
				break;
			}
			/* Chaos balls in every direction */
			case 8:
			case 9:
			{
				effect_simple(EF_STAR_BALL, source_player(), "1d400",
							  PROJ_CHAOS, 2, 0, 0, 0, NULL);
				break;
			}
			/* Tear up the dungeon. */
			case 10:
			{
				effect_simple(EF_DESTRUCTION, source_player(), "0", 0,
							  5 + randint1(20), 0, 0, 0, NULL);
				break;
			}
			/* Chaos cloud right on top of the poor caster. */
			case 11:
			{
				effect_simple(EF_CLOUD, source_player(), "1d400", PROJ_CHAOS,
							  6, 0, 0, 0, NULL);
				break;
			}
			/* Chaos spray. */
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			{
				effect_simple(EF_ARC, source_player(), "1d600", PROJ_CHAOS,
							  6, 0, 0, 0, NULL);
				break;
			}
			/* Unmake the caster. */
			case 17:
			{
				(void) player_stat_dec(player, STAT_STR, one_in_(3));
				(void) player_stat_dec(player, STAT_INT, one_in_(3));
				(void) player_stat_dec(player, STAT_WIS, one_in_(3));
				(void) player_stat_dec(player, STAT_DEX, one_in_(3));
				(void) player_stat_dec(player, STAT_CON, one_in_(3));
				break;
			}
		}

		/* Chaos, once unleashed, likes to stay... */
		if (!one_in_(4)) repeat = false;
	}

	return false;
}
