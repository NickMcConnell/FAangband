/**
 * \file game-world.c
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

#include "angband.h"
#include "cmds.h"
#include "effects.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-make.h"
#include "mon-move.h"
#include "mon-util.h"
#include "obj-curse.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "source.h"
#include "target.h"
#include "trap.h"

u16b daycount = 0;
u32b seed_randart;		/* Hack -- consistent random artifacts */
u32b seed_flavor;		/* Hack -- consistent object colors */
s32b turn;				/* Current game turn */
bool character_generated;	/* The character exists */
bool character_dungeon;		/* The character has a dungeon */
struct level_map *maps;
struct level_map *world;

/**
 * This table allows quick conversion from "speed" to "energy"
 * The basic function WAS ((S>=110) ? (S-110) : (100 / (120-S)))
 * Note that table access is *much* quicker than computation.
 *
 * Note that the table has been changed at high speeds.  From
 * "Slow (-40)" to "Fast (+30)" is pretty much unchanged, but
 * at speeds above "Fast (+30)", one approaches an asymptotic
 * effective limit of 50 energy per turn.  This means that it
 * is relatively easy to reach "Fast (+30)" and get about 40
 * energy per turn, but then speed becomes very "expensive",
 * and you must get all the way to "Fast (+50)" to reach the
 * point of getting 45 energy per turn.  After that point,
 * furthur increases in speed are more or less pointless,
 * except to balance out heavy inventory.
 *
 * Note that currently the fastest monster is "Fast (+30)".
 */
const byte extract_energy[200] =
{
	/* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* S-50 */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	/* S-40 */     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	/* S-30 */     2,  2,  2,  2,  2,  2,  2,  3,  3,  3,
	/* S-20 */     3,  3,  3,  3,  3,  4,  4,  4,  4,  4,
	/* S-10 */     5,  5,  5,  5,  6,  6,  7,  7,  8,  9,
	/* Norm */    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	/* F+10 */    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	/* F+20 */    30, 31, 32, 33, 34, 35, 36, 36, 37, 37,
	/* F+30 */    38, 38, 39, 39, 40, 40, 40, 41, 41, 41,
	/* F+40 */    42, 42, 42, 43, 43, 43, 44, 44, 44, 44,
	/* F+50 */    45, 45, 45, 45, 45, 46, 46, 46, 46, 46,
	/* F+60 */    47, 47, 47, 47, 47, 48, 48, 48, 48, 48,
	/* F+70 */    49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
	/* Fast */    49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
};

/**
 * ------------------------------------------------------------------------
 * Map-related functions
 * ------------------------------------------------------------------------ */
/**
 * Specific levels on which there should never be a vault
 */
bool no_vault(int place)
{
	struct level *lev = &world->levels[place];

	/* No vaults on mountaintops */
	if (lev->topography == TOP_MOUNTAINTOP)
		return true;

	/* No vaults on dungeon entrances */
	if ((lev->topography != TOP_CAVE) && (lev->down))
		return true;

	/* Anywhere else is OK */
	return false;
}

/**
 * List of { locality, name } pairs.
 */
static const grouper locality_names[] =
{
	#define LOC(a, b) { LOC_##a, b },
	#include "list-localities.h"
	#undef LOC
};

/**
 * Get a locality name
 */
const char *locality_name(enum locality locality)
{
	return locality_names[locality].name;
}

/**
 * Form a level's name
 */
char *level_name(struct level *lev)
{
	char *name;

	if (lev->depth) {
		name = format("%s %d", locality_name(lev->locality), lev->depth);
	} else {
		name = format("%s Town", locality_name(lev->locality));
	}
	return name;
}

/**
 * Find a level by its name
 */
struct level *level_by_name(struct level_map *map, char *name)
{
	int i;
	for (i = 0; i < map->num_levels; i++) {
		struct level *lev = &map->levels[i];
		if (streq(name, level_name(lev))) {
			return lev;
		}
	}
	return NULL;
}

/**
 * Find a town by its name
 */
struct town *town_by_name(struct level_map *map, char *name)
{
	int i;

	/* No hometown */
	if (!name) return NULL;

	for (i = 0; i < map->num_towns; i++) {
		struct town *town = &map->towns[i];
		struct level *lev = &map->levels[town->index];
		if (streq(name, level_name(lev))) {
			return town;
		}
	}
	return NULL;
}

/**
 * Return a current world level's topography
 */
int level_topography(int index)
{
	return world->levels[index].topography;
}

/**
 * ------------------------------------------------------------------------
 * Functions for handling turn-based events
 * ------------------------------------------------------------------------ */
/**
 * Say whether it's daytime or not
 */
bool is_daytime(void)
{
	if ((turn % (10L * z_info->day_length)) < ((10L * z_info->day_length) / 2)) 
		return true;

	return false;
}

/**
 * Say whether we're out where the sun shines
 */
bool outside(void)
{
	if (level_topography(player->place) == TOP_CAVE) return false;
	if (level_topography(player->place) == TOP_VALLEY) return false;
	return true;
}

/**
 * Say whether it's daylight or not
 */
bool is_daylight(void)
{
	return is_daytime() && outside();
}

/**
 * The amount of energy gained in a turn by a player or monster
 */
int turn_energy(int speed)
{
	return extract_energy[speed] * z_info->move_energy / 100;
}

/**
 * If player has inscribed the object with "!!", let him know when it's
 * recharged. -LM-
 * Also inform player when first item of a stack has recharged. -HK-
 * Notify all recharges w/o inscription if notify_recharge option set -WP-
 */
static void recharged_notice(const struct object *obj, bool all)
{
	char o_name[120];

	const char *s;

	bool notify = false;

	if (OPT(player, notify_recharge)) {
		notify = true;
	} else if (obj->note) {
		/* Find a '!' */
		s = strchr(quark_str(obj->note), '!');

		/* Process notification request */
		while (s) {
			/* Find another '!' */
			if (s[1] == '!') {
				notify = true;
				break;
			}

			/* Keep looking for '!'s */
			s = strchr(s + 1, '!');
		}
	}

	if (!notify) return;

	/* Describe (briefly) */
	object_desc(o_name, sizeof(o_name), obj, ODESC_BASE);

	/* Disturb the player */
	disturb(player);

	/* Notify the player */
	if (obj->number > 1) {
		if (all) msg("Your %s have recharged.", o_name);
		else msg("One of your %s has recharged.", o_name);
	} else if (obj->artifact)
		msg("The %s has recharged.", o_name);
	else
		msg("Your %s has recharged.", o_name);
}


/**
 * Recharge activatable objects in the player's equipment
 * and rods in the inventory and on the ground.
 */
static void recharge_objects(void)
{
	int i;
	bool discharged_stack;
	struct object *obj;

	/* Recharge carried gear */
	for (obj = player->gear; obj; obj = obj->next) {
		/* Skip non-objects */
		assert(obj->kind);

		/* Recharge equipment */
		if (object_is_equipped(player->body, obj)) {
			/* Recharge activatable objects */
			if (recharge_timeout(obj)) {
				/* Message if an item recharged */
				recharged_notice(obj, true);

				/* Window stuff */
				player->upkeep->redraw |= (PR_EQUIP);
			}
		} else {
			/* Recharge the inventory */
			discharged_stack =
				(number_charging(obj) == obj->number) ? true : false;

			/* Recharge rods, and update if any rods are recharged */
			if (tval_can_have_timeout(obj) && recharge_timeout(obj)) {
				/* Entire stack is recharged */
				if (obj->timeout == 0)
					recharged_notice(obj, true);

				/* Previously exhausted stack has acquired a charge */
				else if (discharged_stack)
					recharged_notice(obj, false);

				/* Combine pack */
				player->upkeep->notice |= (PN_COMBINE);

				/* Redraw stuff */
				player->upkeep->redraw |= (PR_INVEN);
			}
		}
	}

	/* Recharge other level objects */
	for (i = 1; i < cave->obj_max; i++) {
		obj = cave->objects[i];
		if (!obj) continue;

		/* Recharge rods */
		if (tval_can_have_timeout(obj))
			recharge_timeout(obj);
	}
}


/**
 * Remove light-sensitive monsters from sunlit areas
 */
void sun_banish(void)
{
	bool some_gone = false;
	struct monster *mon;
	struct monster_race *race;
	int i;

	/* Process all monsters */
	for (i = 1; i < cave_monster_max(cave) - 1; i++) {
		/* Access the monster */
		mon = cave_monster(cave, i);

		/* Check if it hates light */
		race = mon->race;

		/* Paranoia -- Skip dead monsters */
		if (!race) continue;

		/* Skip Unique Monsters */
		if (rf_has(race->flags, RF_UNIQUE)) continue;

		/* Skip monsters not hurt by light */
		if (!(rf_has(race->flags, RF_HURT_LIGHT))) continue;

		/* Banish */
		delete_monster_idx(i);
		some_gone = true;
	}

	process_monsters(cave, 0);
	if (some_gone) {
		msg("Creatures of the darkness flee from the sunlight!");
	}
}

/**
 * Play an ambient sound dependent on dungeon level, and day or night in town
 */
void play_ambient_sound(void)
{
	if (player->depth == 0) {
		if (is_daytime())
			sound(MSG_AMBIENT_DAY);
		else 
			sound(MSG_AMBIENT_NITE);
	} else if (player->depth <= 20) {
		sound(MSG_AMBIENT_DNG1);
	} else if (player->depth <= 40) {
		sound(MSG_AMBIENT_DNG2);
	} else if (player->depth <= 60) {
		sound(MSG_AMBIENT_DNG3);
	} else if (player->depth <= 80) {
		sound(MSG_AMBIENT_DNG4);
	} else {
		sound(MSG_AMBIENT_DNG5);
	}
}

/**
 * Helper for process_world -- decrement player->timed[] and curse effect fields
 */
static void decrease_timeouts(void)
{
	int adjust = (adj_con_fix[player->state.stat_ind[STAT_CON]] + 1);
	int i;

	/* Most timed effects decrement by 1 */
	for (i = 0; i < TMD_MAX; i++) {
		int decr = 1;
		if (!player->timed[i])
			continue;

		/* Special cases */
		switch (i) {
            case TMD_FOOD:
            {
                /* Handled separately */
                decr = 0;
                break;
            }

			case TMD_CUT:
			{
				/* Check for truly "mortal" wound */
				if (player_timed_grade_eq(player, i, "Mortal Wound")) {
					decr = 0;
				} else {
					decr = adjust;
					if (player_has(player, PF_HARDY)) adjust++;
				}

				/* Rock players just maintain */
				if (player_has(player, PF_ROCK)) {
					decr = 0;
				}

				break;
			}

			case TMD_STUN:
			{
				decr = adjust;
				break;
			}

			case TMD_POISONED:
			{
				decr = adjust;
				if (player_has(player, PF_HARDY)) adjust++;
				break;
			}

			case TMD_COMMAND:
			{
				struct monster *mon = get_commanded_monster();
				if (!los(cave, player->grid, mon->grid)) {
					/* Out of sight is out of mind */
					mon_clear_timed(mon, MON_TMD_COMMAND, MON_TMD_FLG_NOTIFY);
					player_clear_timed(player, TMD_COMMAND, true);
				} else {
					/* Keep monster timer aligned */
					mon_dec_timed(mon, MON_TMD_COMMAND, decr, 0);
				}
				break;
			}
		}
		/* Specialty Enhance Magic - beneficial effects timeout at 2/3 speed */
		if (player_has(player, PF_ENHANCE_MAGIC) && !(turn % 3) &&
			player_timed_effect_is_beneficial(i)) {
			continue;
		}

		/* Decrement the effect */
		player_dec_timed(player, i, decr, false);
	}

	/* Curse effects always decrement by 1 */
	for (i = 0; i < player->body.count; i++) {
		struct curse_data *curse = NULL;
		if (player->body.slots[i].obj == NULL) {
			continue;
		}
		curse = player->body.slots[i].obj->curses;
		if (curse) {
			int j;
			for (j = 0; j < z_info->curse_max; j++) {
				if (curse[j].power) {
					curse[j].timeout--;
					if (!curse[j].timeout) {
						struct curse *c = &curses[j];
						if (do_curse_effect(j, player->body.slots[i].obj)) {
							player_learn_curse(player, c);
						}
						curse[j].timeout = randcalc(c->obj->time, 0, RANDOMISE);
					}
				}
			}
		}
	}

	return;
}

/**
 * ------------------------------------------------------------------------
 * Main turn-by-turn processing functions
 * ------------------------------------------------------------------------ */
/**
 * Handle things that need updating once every 10 game turns
 */
void process_world(struct chunk *c)
{
	int i, y, x;

	/* Compact the monster list if we're approaching the limit */
	if (cave_monster_count(c) + 32 > z_info->level_monster_max)
		compact_monsters(64);

	/* Too many holes in the monster list - compress */
	if (cave_monster_count(c) + 32 < cave_monster_max(c))
		compact_monsters(0);

	/*** Check the Time ***/

	/* Play an ambient sound at regular intervals. */
	if (!(turn % ((10L * z_info->day_length) / 4)))
		play_ambient_sound();

	/* Handle stores and sunshine */
	if (outside()) {
		/* Daybreak/Nightfall in town */
		if (!(turn % ((10L * z_info->day_length) / 2))) {
			/* Check for dawn */
			bool dawn = (!(turn % (10L * z_info->day_length)));

			if (dawn) {
				/* Day breaks */
				msg("The sun has risen.");
				sun_banish();
			} else {
				/* Night falls */
				msg("The sun has fallen.");
			}

			/* Illuminate */
			cave_illuminate(c, dawn);
		}
	}

	/* Update the stores once a day.  The changes are not actually made until
	 * return to town, to avoid giving details away in the knowledge menu. */
	if (player->depth && !(turn % (10L * z_info->store_turns))) daycount++;

	/* Check for light change */
	if (player_has(player, PF_UNLIGHT)) {
		player->upkeep->update |= PU_BONUS;
	}

	/* Check for creature generation */
	if (one_in_(z_info->alloc_monster_chance))
		(void)pick_and_place_distant_monster(c, player, z_info->max_sight + 5,
											 true, player->depth);

	/*** Damage (or healing) over Time ***/

	/* Take damage from poison */
	if (player->timed[TMD_POISONED])
		take_hit(player, 1, "poison");

	/* Take damage from cuts, worse from serious cuts */
	if (player->timed[TMD_CUT]) {
		if (player_has(player, PF_ROCK)) {
			/* Rock players just maintain */
			i = 0;
		} else if (player_timed_grade_eq(player, TMD_CUT, "Mortal Wound") ||
				   player_timed_grade_eq(player, TMD_CUT, "Deep Gash")) {
			i = 3;
		} else if (player_timed_grade_eq(player, TMD_CUT, "Severe Cut")) {
			i = 2;
		} else {
			i = 1;
		}

		/* Take damage */
		take_hit(player, i, "a fatal wound");
	}

	/* Side effects of diminishing bloodlust */
	if (player->timed[TMD_BLOODLUST]) {
		player_over_exert(player, PY_EXERT_HP | PY_EXERT_CUT | PY_EXERT_SLOW,
						  MAX(0, 10 - player->timed[TMD_BLOODLUST]),
						  player->chp / 10);
	}

	/* Timed healing */
	if (player->timed[TMD_HEAL]) {
		bool ident = false;
		effect_simple(EF_HEAL_HP, source_player(), "30", 0, 0, 0, 0, 0, &ident);
	}

	/* Effects of Black Breath */
	if (player->timed[TMD_BLACKBREATH]) {
		bool hardy = player_has(player, PF_HARDY);

		if (one_in_(hardy ? 4 : 2)) {
			msg("The Black Breath sickens you.");
			player_stat_dec(player, STAT_CON, false);
		}
		if (one_in_(hardy ? 4 : 2)) {
			msg("The Black Breath saps your strength.");
			player_stat_dec(player, STAT_STR, false);
		}
		if (one_in_(hardy ? 4 : 2)) {
			/* Life draining */
			int drain = 100 + (player->exp / 100) * z_info->life_drain_percent;
			msg("The Black Breath dims your life force.");
			player_exp_lose(player, drain, false);
		}
	}

	/*** Check the Food, and Regenerate ***/

	/* Digest normally */
	if (!(turn % 100)) {
		/* Basic digestion rate based on speed */
		i = turn_energy(player->state.speed);

		/* Adjust for food value */
		i = (i * 100) / z_info->food_value;

		/* Regeneration takes more food */
		if (player_of_has(player, OF_REGEN)) i *= 2;

		/* Slow digestion takes less food */
		if (player_of_has(player, OF_SLOW_DIGEST)) i /= 2;

		/* Minimal digestion */
		if (i < 1) i = 1;

		/* Digest some food */
		player_dec_timed(player, TMD_FOOD, i, false);
	}

	/* Fast metabolism */
	if (player->timed[TMD_HEAL]) {
		player_dec_timed(player, TMD_FOOD, 8 * z_info->food_value, false);
		if (player->timed[TMD_FOOD] < PY_FOOD_HUNGRY) {
			player_set_timed(player, TMD_HEAL, 0, true);
		}
	}

	/* Faint or starving */
	if (player_timed_grade_eq(player, TMD_FOOD, "Faint")) {
		/* Faint occasionally */
		if (!player->timed[TMD_PARALYZED] && one_in_(10)) {
			/* Message */
			msg("You faint from the lack of food.");
			disturb(player);

			/* Faint (bypass free action) */
			(void)player_inc_timed(player, TMD_PARALYZED, 1 + randint0(5),
								   true, false);
		}
	} else if (player_timed_grade_eq(player, TMD_FOOD, "Starving")) {
		/* Calculate damage */
		i = (PY_FOOD_STARVE - player->timed[TMD_FOOD]) / 10;

		/* Take damage */
		take_hit(player, i, "starvation");
	}

	/* Regenerate Hit Points if needed */
	if (player->chp < player->mhp)
		player_regen_hp(player);

	/* Regenerate or lose mana */
	player_regen_mana(player);

	/* Timeout various things */
	decrease_timeouts();

	/* Decay special heighten power */
	if (player->heighten_power) {
		/* To keep it from being a free ride for high speed 
		 * characters, Heighten Power decays quickly when highly charged */
		int decrement = 10 + (player->heighten_power / 55);
		player->heighten_power = MAX(0, player->heighten_power - decrement);
		player->upkeep->update |= (PU_BONUS);
	}

	/* Decay special speed boost */
	if (player->speed_boost) {
		player->speed_boost = MAX(player->speed_boost - 10, 0);
		player->upkeep->update |= (PU_BONUS);
	}

	/* Process light */
	player_update_light(player);

	/* Update noise and scent (not if resting) */
	if (!player_is_resting(player)) {
		make_noise(cave, player, NULL);
		update_scent(cave, player, NULL);
	}

	/*** Process Inventory ***/

	/* Handle experience draining */
	if (player_of_has(player, OF_DRAIN_EXP)) {
		if ((player->exp > 0) && one_in_(10)) {
			s32b d = damroll(10, 6) +
				(player->exp / 100) * z_info->life_drain_percent;
			player_exp_lose(player, d / 10, false);
		}

		equip_learn_flag(player, OF_DRAIN_EXP);
	}

	/* Recharge activatable objects and rods */
	recharge_objects();

	/* Notice things after time */
	if (!(turn % 100))
		equip_learn_after_time(player);

	/* Decrease trap timeouts */
	for (y = 0; y < c->height; y++) {
		for (x = 0; x < c->width; x++) {
			struct loc grid = loc(x, y);
			struct trap *trap = square(c, grid)->trap;
			while (trap) {
				if (trap->timeout) {
					trap->timeout--;
					if (!trap->timeout)
						square_light_spot(c, grid);
				}
				trap = trap->next;
			}
		}
	}


	/*** Involuntary Movement ***/

	/* Delayed Word-of-Recall */
	if (player->word_recall) {
		/* Count down towards recall */
		player->word_recall--;

		/* Activate the recall */
		if (!player->word_recall) {
			/* Disturbing! */
			disturb(player);

			/* Determine the level */
			if (player->place != player->home) {
				int new_last_place = (player->upkeep->arena_level) ? player->last_place : 0;
				msgt(MSG_TPLEVEL, "You feel yourself yanked homewards!");
				player_change_place(player, player->home);
				player->last_place = new_last_place;
			} else {
				msgt(MSG_TPLEVEL, "You feel yourself yanked away!");
				player_change_place(player, player->recall_pt);
				player->last_place = 0;
			}
		}
	}

	/* Delayed Deep Descent */
	if (player->deep_descent) {
		/* Count down towards descent */
		player->deep_descent--;

		/* Activate the descent */
		if (player->deep_descent == 0) {
			int increment;
			int target_place = player->max_depth;
			struct level *lev;

			/* Calculate target depth */
			//increment = (4 / z_info->stair_skip) + 1;
			increment = 5;
			target_place = player_get_next_place(player->max_depth, "down",
												 increment);
			lev = &world->levels[target_place];

			disturb(player);

			/* Determine the level */
			if (lev->depth > player->depth) {
				msgt(MSG_TPLEVEL, "The floor opens beneath you!");
				player_change_place(player, target_place);
			} else {
				/* Otherwise do something disastrous */
				msgt(MSG_TPLEVEL, "You are thrown back in an explosion!");
				effect_simple(EF_DESTRUCTION, source_none(), "0", 0, 5, 0, 0, 0, NULL);
			}
		}
	}
}


/**
 * Housekeeping after the processing of a player command
 */
static void process_player_cleanup(void)
{
	int i;

	/* Significant */
	if (player->upkeep->energy_use) {
		/* Use some energy */
		player->energy -= player->upkeep->energy_use;

		/* Increment the total energy counter */
		player->total_energy += player->upkeep->energy_use;

		/* Player can be damaged by terrain */
		player_take_terrain_damage(player, player->grid);

		/* Player can be moved by terrain (or lack of it) */
		if (square_isfall(cave, player->grid)) {
			player_fall_off_cliff(player);
			return;
		}

		/* Do nothing else if player has auto-dropped stuff */
		if (!player->upkeep->dropping) {
			/* Hack -- constant hallucination */
			if (player->timed[TMD_IMAGE])
				player->upkeep->redraw |= (PR_MAP);

			/* Shimmer multi-hued monsters */
			for (i = 1; i < cave_monster_max(cave); i++) {
				struct monster *mon = cave_monster(cave, i);
				if (!mon->race)
					continue;
				if (!rf_has(mon->race->flags, RF_ATTR_MULTI))
					continue;
				square_light_spot(cave, mon->grid);
			}

			/* Clear NICE flag, and show marked monsters */
			for (i = 1; i < cave_monster_max(cave); i++) {
				struct monster *mon = cave_monster(cave, i);
				mflag_off(mon->mflag, MFLAG_NICE);
				if (mflag_has(mon->mflag, MFLAG_MARK)) {
					if (!mflag_has(mon->mflag, MFLAG_SHOW)) {
						mflag_off(mon->mflag, MFLAG_MARK);
						update_mon(mon, cave, false);
					}
				}
			}
		}
	}

	/* Clear SHOW flag and player drop status */
	for (i = 1; i < cave_monster_max(cave); i++) {
		struct monster *mon = cave_monster(cave, i);
		mflag_off(mon->mflag, MFLAG_SHOW);
	}
	player->upkeep->dropping = false;

	/* Hack - update needed first because inventory may have changed */
	update_stuff(player);
	redraw_stuff(player);
}


/**
 * Process player commands from the command queue, finishing when there is a
 * command using energy (any regular game command), or we run out of commands
 * and need another from the user, or the character changes level or dies, or
 * the game is stopped.
 *
 * Notice the annoying code to handle "pack overflow", which
 * must come first just in case somebody manages to corrupt
 * the savefiles by clever use of menu commands or something. (Can go? NRM)
 *
 * Notice the annoying code to handle "monster memory" changes,
 * which allows us to avoid having to update the window flags
 * every time we change any internal monster memory field, and
 * also reduces the number of times that the recall window must
 * be redrawn.
 */
void process_player(void)
{
	/* Check for interrupts */
	player_resting_complete_special(player);
	event_signal(EVENT_CHECK_INTERRUPT);

	/* Repeat until energy is reduced */
	do {
		/* Refresh */
		notice_stuff(player);
		handle_stuff(player);
		event_signal(EVENT_REFRESH);

		/* Hack -- Pack Overflow */
		pack_overflow(NULL);

		/* Assume free turn */
		player->upkeep->energy_use = 0;

		/* Dwarves detect treasure */
		if (player_has(player, PF_SEE_ORE)) {
			/* Only if they are in good shape */
			if (!player->timed[TMD_IMAGE] &&
				!player->timed[TMD_CONFUSED] &&
				!player->timed[TMD_AMNESIA] &&
				!player->timed[TMD_STUN] &&
				!player->timed[TMD_PARALYZED] &&
				!player->timed[TMD_TERROR] &&
				!player->timed[TMD_AFRAID])
				effect_simple(EF_DETECT_GOLD, source_none(), "0", 0, 0, 0, 3, 3, NULL);
		}

		/* Paralyzed or Knocked Out player gets no turn */
		if (player->timed[TMD_PARALYZED] ||
			player_timed_grade_eq(player, TMD_STUN, "Knocked Out")) {
			cmdq_push(CMD_SLEEP);
		}

		/* Prepare for the next command */
		if (cmd_get_nrepeats() > 0) {
			event_signal(EVENT_COMMAND_REPEAT);
		} else {
			/* Check monster recall */
			if (player->upkeep->monster_race)
				player->upkeep->redraw |= (PR_MONSTER);

			/* Place cursor on player/target */
			event_signal(EVENT_REFRESH);
		}

		/* Get a command from the queue if there is one */
		if (!cmdq_pop(CTX_GAME))
			break;

		if (!player->upkeep->playing)
			break;

		process_player_cleanup();
	} while (!player->upkeep->energy_use &&
			 !player->is_dead &&
			 !player->upkeep->generate_level);

	/* Notice stuff (if needed) */
	notice_stuff(player);
}

/**
 * Housekeeping on arriving on a new level
 */
void on_new_level(void)
{
	/* Arena levels are not really a level change */
	if (!player->upkeep->arena_level) {
		/* Play ambient sound on change of level. */
		play_ambient_sound();

		/* Cancel the target */
		target_set_monster(0);

		/* Cancel the health bar */
		health_track(player->upkeep, NULL);
	}

	/* Disturb */
	disturb(player);

	/* Track maximum player level */
	if (player->max_lev < player->lev)
		player->max_lev = player->lev;

	/* Track maximum dungeon level */
	if (player->max_depth < player->depth)
		player->max_depth = player->depth;

	/* Flush messages */
	event_signal(EVENT_MESSAGE_FLUSH);

	/* Update display */
	event_signal(EVENT_NEW_LEVEL_DISPLAY);

	/* Update player */
	player->upkeep->update |= (PU_BONUS | PU_HP | PU_SPELLS | PU_INVEN);
	player->upkeep->notice |= (PN_COMBINE);
	notice_stuff(player);
	update_stuff(player);
	redraw_stuff(player);

	/* Refresh */
	event_signal(EVENT_REFRESH);

	if (player->upkeep->arena_level) {
		return;
	}

	/* Announce (or repeat) the feeling */
	if (player->depth)
		display_feeling(false);

	/* Check the surroundings */
	search(player);

	/* Give player minimum energy to start a new level, but do not reduce
	 * higher value from savefile for level in progress */
	if (player->energy < z_info->move_energy)
		player->energy = z_info->move_energy;
}

/**
 * Housekeeping on leaving a level
 */
static void on_leave_level(void) {
	/* Cancel any command */
	player_clear_timed(player, TMD_COMMAND, false);

	/* Don't allow command repeat if moved away from item used. */
	if (cmdq_does_previous_use_floor_item()) {
		cmd_disable_repeat();
	}

	/* Any pending processing */
	notice_stuff(player);
	update_stuff(player);
	redraw_stuff(player);

	/* Flush messages */
	event_signal(EVENT_MESSAGE_FLUSH);
}


/**
 * The main game loop.
 *
 * This function will run until the player needs to enter a command, or closes
 * the game, or the character dies.
 */
void run_game_loop(void)
{
	/* Tidy up after the player's command */
	process_player_cleanup();

	/* Keep processing the player until they use some energy or
	 * another command is needed */
	while (player->upkeep->playing) {
		process_player();
		if (player->upkeep->energy_use)
			break;
		else
			return;
	}

	/* The player may still have enough energy to move, so we run another
	 * player turn before processing the rest of the world */
	while (player->energy >= z_info->move_energy) {
		/* Do any necessary animations */
		event_signal(EVENT_ANIMATE);
		
		/* Process monster with even more energy first */
		process_monsters(cave, player->energy + 1);
		if (player->is_dead || !player->upkeep->playing ||
			player->upkeep->generate_level)
			break;

		/* Process the player until they use some energy */
		while (player->upkeep->playing) {
			process_player();
			if (player->upkeep->energy_use)
				break;
			else
				return;
		}
	}

	/* Now that the player's turn is fully complete, we run the main loop 
	 * until player input is needed again */
	while (true) {
		notice_stuff(player);
		handle_stuff(player);
		event_signal(EVENT_REFRESH);

		/* Process the rest of the world, give the player energy and 
		 * increment the turn counter unless we need to stop playing or
		 * generate a new level */
		if (player->is_dead || !player->upkeep->playing)
			return;
		else if (!player->upkeep->generate_level) {
			/* Process the rest of the monsters */
			process_monsters(cave, 0);

			/* Mark all monsters as ready to act when they have the energy */
			reset_monsters();

			/* Refresh */
			notice_stuff(player);
			handle_stuff(player);
			event_signal(EVENT_REFRESH);
			if (player->is_dead || !player->upkeep->playing)
				return;

			/* Process the world every ten turns */
			if (!(turn % 10) && !player->upkeep->generate_level) {
				process_world(cave);

				/* Refresh */
				notice_stuff(player);
				handle_stuff(player);
				event_signal(EVENT_REFRESH);
				if (player->is_dead || !player->upkeep->playing)
					return;
			}

			/* Give the player some energy */
			player->energy += turn_energy(player->state.speed);

			/* Count game turns */
			turn++;
		}

		/* Make a new level if requested */
		if (player->upkeep->generate_level) {
			bool arena = false;
			if (character_dungeon) {
				on_leave_level();
				if (cave->name && streq(cave->name, "arena")) {
					arena = true;
				}
			}

			prepare_next_level(&cave, player);
			on_new_level();

			player->upkeep->generate_level = false;

			/* Kill arena monster */
			if (arena) {
				player->upkeep->arena_level = false;
				if (player->upkeep->health_who) kill_arena_monster(player->upkeep->health_who);
			}
		}

		/* If the player has enough energy to move they now do so, after
		 * any monsters with more energy take their turns */
		while (player->energy >= z_info->move_energy) {
			/* Do any necessary animations */
			event_signal(EVENT_ANIMATE);

			/* Process monster with even more energy first */
			process_monsters(cave, player->energy + 1);
			if (player->is_dead || !player->upkeep->playing ||
				player->upkeep->generate_level)
				break;

			/* Process the player until they use some energy */
			while (player->upkeep->playing) {
				process_player();
				if (player->upkeep->energy_use)
					break;
				else
					return;
			}
		}
	}
}
