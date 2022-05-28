/**
 * \file project-player.c
 * \brief projection effects on the player
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
#include "init.h"
#include "mon-desc.h"
#include "mon-predicate.h"
#include "mon-util.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "project.h"
#include "source.h"
#include "trap.h"

/**
 * ------------------------------------------------------------------------
 * Damage adjustment
 * ------------------------------------------------------------------------ */

/**
 * Attack is water based
 */
static bool attack_is_water_based(int type)
{
	return (type == PROJ_WATER) || (type == PROJ_STORM);
}

/**
 * Attack is cold based
 */
static bool attack_is_cold_based(int type)
{
	return (type == PROJ_COLD) || (type == PROJ_ICE);
}

/**
 * Attack is fire based
 */
static bool attack_is_fire_based(int type)
{
	return (type == PROJ_FIRE) || (type == PROJ_PLASMA) ||
		(type == PROJ_HELLFIRE) || (type == PROJ_DRAGONFIRE);
}

/**
 * Adjust damage according to terrain and attack type for player or a monster
 * (but not both).
 *
 * \param p is the player
 * \param mon is the monster
 * \param type is the attack type we are checking.
 * \param dam is the unadjusted damage.
 * \param grid is the grid the damage is being calculated at
 */
int terrain_adjust_dam(struct player *p, struct monster *mon, int type, int dam)
{
	struct loc grid = loc(0, 0);

	/* Set grid */
	if (p) {
		assert(mon == NULL);
		grid = p->grid;
	} else {
		assert(mon);
		grid = mon->grid;
	}

	/* Trees and rubble offer general protection */
	if (square_isprotect(cave, grid)) {
		dam -= (dam * projections[type].terrain_factor) / (p ? 12 : 8);
	}

	/* Water: Fire-based spells suffer, but other spells benefit slightly
	 * (player is easier to hit).  Water spells come into their own. */
	if (square_iswatery(cave, grid)) {
		if (attack_is_fire_based(type)) {
			dam -= (dam * projections[type].terrain_factor) / (p ? 8 : 4);
		} else if (attack_is_water_based(type)) {
			dam += (dam * projections[type].terrain_factor) / (p ? 8 : 6);
		} else if (p) {
			dam += (dam * projections[type].terrain_factor) / 20;
		}
	}

	/* Lava: Cold and water-based spells suffer, fire-based spells benefit. */
	if (square_isfiery(cave, grid)) {
		if (attack_is_fire_based(type)) {
			dam += (dam * projections[type].terrain_factor) / (p ? 8 : 10);
		} else if (attack_is_water_based(type) || attack_is_cold_based(type)) {
			dam -= (dam * projections[type].terrain_factor) / (p ? 8 : 6);
		}
	}

	return dam;
}

/**
 * Adjust damage according to resistance or vulnerability.
 *
 * \param p is the player
 * \param type is the attack type we are checking.
 * \param dam is the unadjusted damage.
 * \param actual is whether this is actually happening or a test
 */
int adjust_dam(struct player *p, int type, int dam, bool actual)
{
	int dam_percent = 100;

	/* If an actual player exists, get their actual resist */
	if (p && p->race) {
		int special_dam = 0;

		/* Handle special cases */
		if (type == PROJ_ICE) {
			return adjust_dam(p, PROJ_COLD, dam, actual);
		} else if (type == PROJ_PLASMA) {
			special_dam = adjust_dam(p, PROJ_FIRE, dam / 2, actual);
			special_dam += adjust_dam(p, PROJ_ELEC, dam / 2, actual);
			return special_dam;
		} else if (type == PROJ_STORM) {
			/* Extra storm damage will come later */
			return dam / 2;
		} else if (type == PROJ_DRAGONFIRE) {
			special_dam = adjust_dam(p, PROJ_FIRE, dam / 2, actual);
			special_dam += adjust_dam(p, PROJ_POIS, dam / 2, actual);
			return special_dam;
		} else if (type == PROJ_HELLFIRE) {
			special_dam = adjust_dam(p, PROJ_FIRE, 2 * dam / 3, actual);
			special_dam += adjust_dam(p, PROJ_DARK, dam / 3, actual);
			return special_dam;
		} else if (type < ELEM_MAX) {
			/* For the regular elements, the stored resistance level is the
			 * percentage of damage taken */
			dam_percent = p->state.el_info[type].res_level;

			/* Notice element stuff */
			if (actual) {
				equip_learn_element(p, type);
			}
		}
	}

	/* Immune */
	if (dam_percent == RES_LEVEL_MAX) return 0;

	/* Acid damage is halved by armour */
	if (type == PROJ_ACID && p && minus_ac(p)) {
		dam = (dam + 1) / 2;
	}

	/* High resists get randomised, especially chaos */
	if ((type >= ELEM_HIGH_MIN) && (dam_percent <= RES_LEVEL_BASE)) {
		/* How far is the closer of zero and 100 */
		int max_range =	dam_percent > 50 ? 100 - dam_percent : dam_percent;

		/* Random range is 1 or 2 times 10% of that range */
		int range = (((type == PROJ_CHAOS) ? 2 : 1) * max_range) / 10;

		/* Randomize */
		dam_percent = dam_percent - range + randint0((2 * range) + 1);
	}

	/* Apply the percentage resistance, rounded up */
	return ((dam * dam_percent) + 99) / 100;
}


/**
 * ------------------------------------------------------------------------
 * Player handlers
 * ------------------------------------------------------------------------ */

/**
 * Drain stats at random
 *
 * \param num is the number of points to drain
 */
static void project_player_drain_stats(int num)
{
	int i, k = 0;
	const char *act = NULL;

	for (i = 0; i < num; i++) {
		switch (randint1(5)) {
			case 1: k = STAT_STR; act = "strong"; break;
			case 2: k = STAT_INT; act = "bright"; break;
			case 3: k = STAT_WIS; act = "wise"; break;
			case 4: k = STAT_DEX; act = "agile"; break;
			case 5: k = STAT_CON; act = "hale"; break;
		}

		msg("You're not as %s as you used to be...", act);
		player_stat_dec(player, k, false);
	}

	return;
}

typedef struct project_player_handler_context_s {
	/* Input values */
	const struct source origin;
	const int r;
	const struct loc grid;
	int dam; /* May need adjustment */
	const int type;
	const int power;

	/* Return values */
	bool obvious;
} project_player_handler_context_t;

typedef int (*project_player_handler_f)(project_player_handler_context_t *);

static int project_player_handler_ACID(project_player_handler_context_t *context)
{
	if (player_is_immune(player->state, ELEM_ACID)) return 0;
	inven_damage(player, PROJ_ACID, MIN(context->dam * 5, 300));
	return 0;
}

static int project_player_handler_ELEC(project_player_handler_context_t *context)
{
	if (player_is_immune(player->state, ELEM_ELEC)) return 0;
	inven_damage(player, PROJ_ELEC, MIN(context->dam * 5, 300));
	return 0;
}

static int project_player_handler_FIRE(project_player_handler_context_t *context)
{
	if (player_is_immune(player->state, ELEM_FIRE)) return 0;
	inven_damage(player, PROJ_FIRE, MIN(context->dam * 5, 300));

	/* Occasional side-effects for powerful fire attacks */
	if (context->power >= 80) {
		if (randint0(context->dam) > 500) {
			msg("The intense heat saps you.");
			effect_simple(EF_DRAIN_STAT, source_none(), "0", STAT_STR, 0, 0, 0,
						  0, &context->obvious);
		}
		if (randint0(context->dam) > 500) {
			if (player_inc_timed(player, TMD_BLIND,
					randint1(context->dam / 100), true,
					true, true)) {
				msg("Your eyes fill with smoke!");
			}
		}
		if (randint0(context->dam) > 500) {
			if (player_inc_timed(player, TMD_POISONED,
					randint1(context->dam / 10), true,
					true, true)) {
				msg("You are assailed by poisonous fumes!");
			}
		}
	}
	return 0;
}

static int project_player_handler_COLD(project_player_handler_context_t *context)
{
	if (player_is_immune(player->state, ELEM_COLD)) return 0;
	inven_damage(player, PROJ_COLD, MIN(context->dam * 5, 300));

	/* Occasional side-effects for powerful cold attacks */
	if (context->power >= 80) {
		if (randint0(context->dam) > 500) {
			msg("The cold seeps into your bones.");
			effect_simple(EF_DRAIN_STAT, source_none(), "0", STAT_DEX, 0, 0, 0,
						  0, &context->obvious);
		}
		if (randint0(context->dam) > 500) {
			if (player_of_has(player, OF_HOLD_LIFE)) {
				equip_learn_flag(player, OF_HOLD_LIFE);
			} else {
				int drain = context->dam;
				msg("The cold withers your life force!");
				player_exp_lose(player, drain, false);
			}
		}
	}
	return 0;
}

static int project_player_handler_POIS(project_player_handler_context_t *context)
{
	int xtra = 0;

	if (!player_inc_timed(player, TMD_POISONED, 10 + randint1(context->dam),
			true, true, true)) {
		msg("You resist the effect!");
	}

	/* Occasional side-effects for powerful poison attacks */
	if (context->power >= 60) {
		if (randint0(context->dam) > 200) {
			if (!player_is_immune(player->state, ELEM_ACID)) {
				int dam = context->dam / 5;
				msg("The venom stings your skin!");
				inven_damage(player, PROJ_ACID, dam);
				xtra += adjust_dam(player, PROJ_ACID, dam, true);
			}
		}
		if (randint0(context->dam) > 200) {
			msg("The stench sickens you.");
			effect_simple(EF_DRAIN_STAT, source_none(), "0", STAT_CON, 0, 0, 0,
						  0, &context->obvious);
		}
	}
	return xtra;
}

static int project_player_handler_LIGHT(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_LIGHT)) {
		msg("You resist the effect!");
		return 0;
	}

	(void)player_inc_timed(player, TMD_BLIND, 2 + randint1(5), true, true,
		true);

	/* Confusion for strong unresisted light */
	if (context->dam > 300) {
		msg("You are dazzled!");
		(void)player_inc_timed(player, TMD_CONFUSED,
			2 + randint1(context->dam / 100), true, true, true);
	}
	return 0;
}

static int project_player_handler_DARK(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_DARK)) {
		msg("You resist the effect!");
		return 0;
	}

	(void)player_inc_timed(player, TMD_BLIND, 2 + randint1(5), true, true,
		true);

	/* Unresisted dark from powerful monsters is bad news */
	if (context->power >= 70) {
		/* Life draining */
		if (randint0(context->dam) > 100) {
			if (player_of_has(player, OF_HOLD_LIFE)) {
				equip_learn_flag(player, OF_HOLD_LIFE);
			} else {
				int drain = context->dam;
				msg("The darkness steals your life force!");
				player_exp_lose(player, drain, false);
			}
		}

		/* Slowing */
		if (randint0(context->dam) > 200) {
			msg("You feel unsure of yourself in the darkness.");
			(void)player_inc_timed(player, TMD_SLOW,
				context->dam / 100, true, true, false);
		}

		/* Amnesia */
		if (randint0(context->dam) > 300) {
			msg("Darkness penetrates your mind!");
			(void)player_inc_timed(player, TMD_AMNESIA,
				context->dam / 100, true, true, false);
		}
	}
	return 0;
}

static int project_player_handler_SOUND(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_SOUND)) {
		msg("You resist the effect!");
		return 0;
	}

	/* Stun */
	if (!player_of_has(player, OF_PROT_STUN)) {
		int duration = 5 + randint1(context->dam / 3);
		if (duration > 35) duration = 35;
		(void)player_inc_timed(player, TMD_STUN, duration, true, true,
			true);
	} else {
		equip_learn_flag(player, OF_PROT_STUN);
	}

	/* Confusion for strong unresisted sound */
	if (context->dam > 300) {
		msg("The noise disorients you.");
		(void)player_inc_timed(player, TMD_CONFUSED,
			2 + randint1(context->dam / 100), true, true, true);
	}
	return 0;
}

static int project_player_handler_SHARD(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_SHARD)) {
		msg("You resist the effect!");
		return 0;
	}

	/* Cuts */
	(void)player_inc_timed(player, TMD_CUT, randint1(context->dam), true,
		true, false);
	return 0;
}

static int project_player_handler_NEXUS(project_player_handler_context_t *context)
{
	struct monster *mon = NULL;
	if (context->origin.what == SRC_MONSTER) {
		mon = cave_monster(cave, context->origin.which.monster);
	}

	if (player_resists_effects(player->state, ELEM_NEXUS)) {
		msg("You resist the effect!");
		return 0;
	}

	/* Stat swap */
	if (randint0(100) < player->state.skills[SKILL_SAVE]) {
		msg("You avoid the effect!");
	} else {
		player_inc_timed(player, TMD_SCRAMBLE, randint0(20) + 20, true,
			true, true);
	}

	if (one_in_(3) && mon) { /* Teleport to */
		effect_simple(EF_TELEPORT_TO, context->origin, "0", 0, 0, 0,
					  mon->grid.y, mon->grid.x, NULL);
	} else if (one_in_(4)) { /* Teleport level */
		if (randint0(100) < player->state.skills[SKILL_SAVE]) {
			msg("You avoid the effect!");
			return 0;
		}
		effect_simple(EF_TELEPORT_LEVEL, context->origin, "0", 0, 0, 0, 0, 0,
					  NULL);
	} else { /* Teleport */
		const char *miles = "200";
		effect_simple(EF_TELEPORT, context->origin, miles, 1, 0, 0, 0, 0, NULL);
	}
	return 0;
}

static int project_player_handler_NETHER(project_player_handler_context_t *context)
{
	int drain = 200 + (player->exp / 100) * z_info->life_drain_percent;

	if (player_resists_effects(player->state, ELEM_NETHER) ||
		player_of_has(player, OF_HOLD_LIFE)) {
		msg("You resist the effect!");
		equip_learn_flag(player, OF_HOLD_LIFE);
		return 0;
	}

	/* Life draining */
	msg("You feel your life force draining away!");
	player_exp_lose(player, drain, false);

	/* Powerful nether attacks have further side-effects */
	if (context->power >= 80) {
		/* Mana loss */
		if ((randint0(context->dam) > 100) && player->msp) {
			msg("Your mind is dulled.");
			player->csp -= MIN(player->csp, context->dam / 10);
			player->upkeep->redraw |= PR_MANA;
		}

		/* Loss of energy */
		if (randint0(context->dam) > 200) {
			msg("Your energy is sapped!");
			player->energy = 0;
		}
	}
	return 0;
}

static int project_player_handler_CHAOS(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_CHAOS)) {
		msg("You resist the effect!");
		return 0;
	}

	/* Hallucination */
	(void)player_inc_timed(player, TMD_IMAGE, randint1(10), true, true,
		false);

	/* Confusion */
	(void)player_inc_timed(player, TMD_CONFUSED, 10 + randint0(20), true,
		true, true);

	/* Life draining */
	if (!player_of_has(player, OF_HOLD_LIFE)) {
		int drain = ((player->exp * 3)/ (100 * 2)) * z_info->life_drain_percent;
		msg("You feel your life force draining away!");
		player_exp_lose(player, drain, false);
	} else {
		equip_learn_flag(player, OF_HOLD_LIFE);
	}
	return 0;
}

static int project_player_handler_DISEN(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_DISEN)) {
		msg("You resist the effect!");
		return 0;
	}

	/* Disenchant gear */
	effect_simple(EF_DISENCHANT, context->origin, "0", 0, 0, 0, 0, 0, NULL);
	return 0;
}

static int project_player_handler_WATER(project_player_handler_context_t *context)
{
	/* Confusion */
	(void)player_inc_timed(player, TMD_CONFUSED, 5 + randint1(5), true,
		true, true);

	/* Stun */
	(void)player_inc_timed(player, TMD_STUN, randint1(40), true, true,
		true);
	return 0;
}

static int project_player_handler_ICE(project_player_handler_context_t *context)
{
	if (!player_is_immune(player->state, ELEM_COLD))
		inven_damage(player, PROJ_COLD, MIN(context->dam * 5, 300));

	/* Cuts */
	if (!player_resists_effects(player->state, ELEM_SHARD)) {
		(void)player_inc_timed(player, TMD_CUT, damroll(5, 8), true, true,
							   false);
	} else {
		msg("You resist the effect!");
	}

	/* Stun */
	(void)player_inc_timed(player, TMD_STUN, randint1(15), true, true,
		true);
	return 0;
}

static int project_player_handler_GRAVITY(project_player_handler_context_t *context)
{
	msg("Gravity warps around you.");

	/* Blink */
	if (randint1(127) > player->lev) {
		const char *five = "5";
		effect_simple(EF_TELEPORT, context->origin, five, 1, 0, 0, 0, 0, NULL);
	}

	/* Slow */
	(void)player_inc_timed(player, TMD_SLOW, 4 + randint0(4), true, true,
		false);

	/* Stun */
	if (!player_of_has(player, OF_PROT_STUN)) {
		int duration = 5 + randint1(context->dam / 3);
		if (duration > 35) duration = 35;
		(void)player_inc_timed(player, TMD_STUN, duration, true, true,
			true);
	} else {
		equip_learn_flag(player, OF_PROT_STUN);
	}
	return 0;
}

static int project_player_handler_INERTIA(project_player_handler_context_t *context)
{
	/* Slow */
	(void)player_inc_timed(player, TMD_SLOW, 4 + randint0(4), true, true,
		false);
	return 0;
}

static int project_player_handler_FORCE(project_player_handler_context_t *context)
{
	struct loc centre = origin_get_loc(context->origin);

	/* Player gets pushed in a random direction if on the trap */
	if (context->origin.what == SRC_TRAP &&	loc_eq(player->grid, centre)) {
		int d = randint0(8);
		centre = loc_sum(centre, ddgrid_ddd[d]);
	}

	/* Stun */
	(void)player_inc_timed(player, TMD_STUN, randint1(20), true, true,
		true);

	/* Thrust player away. */
	thrust_away(centre, context->grid, 3 + context->dam / 20);
	return 0;
}

static int project_player_handler_TIME(project_player_handler_context_t *context)
{
	if (one_in_(2)) {
		/* Life draining */
		int drain = 100 + (player->exp / 100) * z_info->life_drain_percent;
		msg("You feel your life force draining away!");
		player_exp_lose(player, drain, false);
	} else if (!one_in_(5)) {
		/* Drain some stats */
		project_player_drain_stats(2);
	} else {
		/* Drain all stats */
		int i;
		msg("You're not as powerful as you used to be...");

		for (i = 0; i < STAT_MAX; i++)
			player_stat_dec(player, i, false);
	}
	return 0;
}

static int project_player_handler_PLASMA(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_STORM(project_player_handler_context_t *context)
{
	int xtra = 0;

	/* Electrical damage. */
	if (one_in_(3)) {
		/* Lightning strikes. */
		msg("You are struck by lightning!");
		xtra += adjust_dam(player, PROJ_ELEC, context->dam / 2, true);
		/* Lightning doesn't strike - at least not directly. */
		xtra += adjust_dam(player, PROJ_ELEC, context->dam / 4, true);
	}

	/* Possibly cold and/or acid damage. */
	if (one_in_(2)) {
		if (!one_in_(3)) {
			msg("You are blasted by freezing winds.");
		} else {
			msg("You are bombarded with hail.");
		}
		xtra += adjust_dam(player, PROJ_COLD, context->dam / 4, true);
	}
	if (one_in_(2)) {
		msg("You are drenched by acidic rain.");
		xtra += adjust_dam(player, PROJ_ACID, context->dam / 4, true);
	}

	/* Sometimes, confuse the player. */
	if (one_in_(2)) {
		(void) player_inc_timed(player, TMD_CONFUSED,
								5 + randint1(context->dam / 3),
								true, true, true);
	}

	/* Sometimes, if no feather fall, throw the player around. */
	if (!player_of_has(player, OF_FEATHER) && !one_in_(3) &&
		(randint0(context->dam / 2) > player->lev)) {
		const char *six = "6";
		msg("The wind grabs you, and whirls you around!");
		effect_simple(EF_TELEPORT, context->origin, six, 1, 0, 0, 0, 0, NULL);
	}
	equip_learn_flag(player, OF_FEATHER);

	return xtra;
}

static int project_player_handler_DRAGONFIRE(project_player_handler_context_t *context)
{
	/* Side-effects for powerful dragonfire attacks */
	if (context->power >= 80) {
		if (!player_of_has(player, OF_FREE_ACT)) {
			msg("The stench overwhelms you, and you faint away!");
			(void) player_inc_timed(player, TMD_PARALYZED, randint0(3) + 2,
									true, true, true);
		}
		if (!player_resists_effects(player->state, ELEM_CHAOS)) {
			msg("The fumes affect your vision!");
			(void) player_inc_timed(player, TMD_IMAGE, randint0(17) + 16, true,
									true, true);
		}
	}

	return 0;
}

static int project_player_handler_HELLFIRE(project_player_handler_context_t *context)
{
	/* Blind the player */
	(void) player_inc_timed(player, TMD_BLIND, 2 + randint1(5),
							true, true, true);

	/* Allow a save against further effects */
	if (randint0(context->power) < player->state.skills[SKILL_SAVE]) {
		msg("Visions of hell invade your mind!");

		/* Possible fear, hallucination and confusion. */
		(void) player_inc_timed(player, TMD_AFRAID,
								randint1(30) + context->power * 2,
								true, true, true);
		(void) player_inc_timed(player, TMD_IMAGE, randint1(101) + 100,
								true, true, true);
		(void) player_inc_timed(player, TMD_CONFUSED, randint1(31) + 30,
								true, true, true);
	}
	return 0;
}

static int project_player_handler_METEOR(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MISSILE(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MANA(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_HOLY_ORB(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_ARROW(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_LIGHT_WEAK(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_DARK_WEAK(project_player_handler_context_t *context)
{
	if (player_resists_effects(player->state, ELEM_DARK)) {
		if (!player_has(player, PF_UNLIGHT)) {
			msg("You resist the effect!");
		}
		return 0;
	}

	(void)player_inc_timed(player, TMD_BLIND, 3 + randint1(5), true, true,
		true);
	return 0;
}

static int project_player_handler_KILL_WALL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_KILL_DOOR(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_KILL_TRAP(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MAKE_DOOR(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MAKE_TRAP(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_AWAY_UNDEAD(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_AWAY_EVIL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_AWAY_SPIRIT(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_AWAY_ALL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_TURN_UNDEAD(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_TURN_EVIL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_TURN_LIVING(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_TURN_ALL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_DISP_UNDEAD(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_DISP_DEMON(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_DISP_DRAGON(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_DISP_EVIL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_DISP_ALL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_SLEEP_UNDEAD(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_SLEEP_EVIL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_SLEEP_ALL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_CLONE(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_POLY(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_HEAL(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_SPEED(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_SLOW(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_CONF(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_HOLD(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_HOLD_UNDEAD(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_STUN(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_DRAIN(project_player_handler_context_t *context)
{
	return 0;
}

static int project_player_handler_MON_CRUSH(project_player_handler_context_t *context)
{
	return 0;
}

static const project_player_handler_f player_handlers[] = {
	#define ELEM(a) project_player_handler_##a,
	#include "list-elements.h"
	#undef ELEM
	#define PROJ(a) project_player_handler_##a,
	#include "list-projections.h"
	#undef PROJ
	NULL
};

/**
 * Called from project() to affect the player
 *
 * Called for projections with the PROJECT_PLAY flag set, which includes
 * bolt, beam, ball and breath effects.
 *
 * \param src is the origin of the effect
 * \param r is the distance from the centre of the effect
 * \param y the coordinates of the grid being handled
 * \param x the coordinates of the grid being handled
 * \param dam is the "damage" from the effect at distance r from the centre
 * \param typ is the projection (PROJ_) type
 * \return whether the effects were obvious
 *
 * If "r" is non-zero, then the blast was centered elsewhere; the damage
 * is reduced in project() before being passed in here.  This can happen if a
 * monster breathes at the player and hits a wall instead.
 *
 * We assume the player is aware of some effect, and always return "true".
 */
bool project_p(struct source origin, int r, struct loc grid, int dam, int typ,
			   int power, bool self)
{
	bool blind = (player->timed[TMD_BLIND] ? true : false);
	bool seen = !blind;
	bool obvious = true;
	struct monster *mon = NULL;

	/* Monster or trap name (for damage) */
	char killer[80];

	project_player_handler_f player_handler = player_handlers[typ];
	project_player_handler_context_t context = {
		origin,
		r,
		grid,
		dam,
		typ,
		power,
		obvious
	};

	/* Decoy has been hit */
	if (square_isdecoyed(cave, grid) && context.dam) {
		square_destroy_decoy(cave, grid);
	}

	/* No player here */
	if (!square_isplayer(cave, grid)) {
		return false;
	}

	/* Determine if terrain is capable of preventing physical damage. */
	if (square_isprotect(cave, grid)) {
		/* A player behind rubble can duck. */
		if (square_isrubble(cave, grid) && one_in_(10)) {
			msg("You duck behind a boulder!");
			return false;
		}

		/* Rangers, elves and druids can take cover in trees. */
		if (square_istree(cave, grid) && one_in_(8) &&
			(player_has(player, PF_WOODSMAN) || player_has(player, PF_ELVEN))) {
			msg("You dodge behind a tree!");
			return false;
		}
	}

	switch (origin.what) {
		case SRC_PLAYER: {
			/* Don't affect projector unless explicitly allowed */
			if (!self) return false;

			break;
		}

		case SRC_MONSTER: {
			mon = cave_monster(cave, origin.which.monster);

			/* Check it is visible */
			if (!monster_is_visible(mon))
				seen = false;

			/* Get the monster's real name */
			monster_desc(killer, sizeof(killer), mon, MDESC_DIED_FROM);

			/* Monster sees what is going on */
			update_smart_learn(mon, player, 0, 0, typ);

			break;
		}

		case SRC_TRAP: {
			struct trap *trap = origin.which.trap;

			/* Get the trap name */
			strnfmt(killer, sizeof(killer), "a %s", trap->kind->desc);

			break;
		}

		case SRC_OBJECT: {
			struct object *obj = origin.which.object;
			object_desc(killer, sizeof(killer), obj,
				ODESC_PREFIX | ODESC_BASE, player);
			break;
		}

		case SRC_CHEST_TRAP: {
			struct chest_trap *trap = origin.which.chest_trap;

			/* Get the trap name */
			strnfmt(killer, sizeof(killer), "%s", trap->msg_death);

			break;
		}

		case SRC_GRID:
		case SRC_NONE: {
			/* Assume the caller has set the killer variable */
			break;
		}
		default: break;
	}

	/* Try to evade or deflect missiles */
	if ((typ == ELEM_ARROW) && mon) {
		/* Get effective armor values */
		int armor = player->state.ac + player->state.to_a;
		struct object *shld = slot_object(player, slot_by_name(player, "arm"));
		int shield_ac = shld ? shld->ac : 0;
		if (player->state.shield_on_back) {
			shield_ac = 0;
		} else if (player_has(player, PF_SHIELD_MAST)) {
			shield_ac += 3;
		}

		if (randint1(80) <= player->state.evasion_chance) {
			/* Evasion */
			msg("You evade the missile!");
			disturb(player);
			return true;
		} else if (MIN(armor, 150) > randint1((10 + mon->race->level) * 20)) {
			/* Armor deflection */
			msg("The missile glances off your armour.");
		} else if (shield_ac > randint0(20 * 6)) {
			/* Shield deflection (20 is max shield AC) */
			msg("The missile ricochets off your shield.");
		}
	}

	/* Let player know what is going on */
	if (!seen) {
		msg("You are hit by %s!", projections[typ].blind_desc);
	}

	/* Adjust damage for terrain and element properties, and apply it */
	context.dam = terrain_adjust_dam(player, NULL, typ, context.dam);
	context.dam = adjust_dam(player, typ, context.dam, true);
	if (dam) {
		/* Self-inflicted damage is scaled down */
		if (self) {
			context.dam /= 10;
		}
		take_hit(player, context.dam, killer);
	}

	/* Handle side effects, possibly including extra damage */
	if (player_handler != NULL && player->is_dead == false) {
		int xtra = player_handler(&context);
		if (xtra) take_hit(player, xtra, killer);
	}

	/* Disturb */
	disturb(player);

	/* Return "Anything seen?" */
	return context.obvious;
}
