/**
 * \file player-properties.c 
 * \brief Class and race abilities
 *
 * Copyright (c) 1997-2020 Ben Harrison, James E. Wilson, Robert A. Koeneke,
 * Leon Marrick, Bahman Rabii, Nick McConnell
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
#include "game-input.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-properties.h"
#include "game-input.h"

/**
 * ------------------------------------------------------------------------
 * Ability utilities
 * ------------------------------------------------------------------------ */
struct player_ability *lookup_ability(const char *type, int index, int value)
{
	struct player_ability *ability;
	for (ability = player_abilities; ability; ability = ability->next) {
		if (!streq(ability->type, type)) continue;
		if (ability->index != index) continue;
		if (streq(type, "element") && (ability->value != value)) continue;
		return ability;
	}
	return NULL;
}

bool class_has_ability(const struct player_class *class,
					   struct player_ability *ability)
{
	if (streq(ability->type, "player") &&
		pf_has(class->pflags, ability->index)) {
		return true;
	} else if (streq(ability->type, "object") &&
			   of_has(class->flags, ability->index)) {
		return true;
	}
	return false;
}

bool race_has_ability(const struct player_race *race,
					  struct player_ability *ability)
{
	if (streq(ability->type, "player") &&
		pf_has(race->pflags, ability->index)) {
		return true;
	} else if (streq(ability->type, "object") &&
			   of_has(race->flags, ability->index)) {
		return true;
	} else if (streq(ability->type, "element") &&
			   (race->el_info[ability->index].res_level == ability->value)) {
		return true;
	}

	return false;
}

/**
 * ------------------------------------------------------------------------
 * Code for gaining specialty abilities and viewing all abilities
 * ------------------------------------------------------------------------ */
/**
 * Gain a new specialty ability
 * Adapted from birth.c get_player_choice -BR-
 */
void gain_specialty(void)
{
	int pick;

	/* Make one choice */
	if (gain_specialty_menu(&pick)) {
		char buf[120];
		struct player_ability *ability = lookup_ability("player", pick, 0);

		/* Add it to the specialties */
		pf_on(player->specialties, pick);

		/* Specialty taken */
		strnfmt(buf, sizeof(buf), "Gained the %s specialty.", ability->name);

		/* Write a note */
		history_add(player, buf, HIST_GAIN_SPECIALTY);

		/* Update some stuff */
		player->upkeep->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS |
								   PU_SPECIALTY | PU_TORCH);

		/* Redraw Study Status */
		player->upkeep->redraw |= (PR_STUDY);
	}
}



/**
 * Browse known abilities -BR-
 */
void view_abilities(void)
{
	int i;
	struct player_ability *ability;
	int num_abilities = 0;
	struct player_ability ability_list[32];

	/* Count the number of specialties we know */
	num_abilities = 0;
	for (i = 0; i < PF_MAX; i++) {
		if (!pf_has(player->specialties, i)) continue;
		memcpy(&ability_list[num_abilities], lookup_ability("player", i, 0),
			   sizeof(struct player_ability));
		ability_list[num_abilities++].group = PLAYER_FLAG_SPECIAL;
	}

	/* Count the number of class powers we have */
	for (ability = player_abilities; ability; ability = ability->next) {
		if (class_has_ability(player->class, ability)) {
			memcpy(&ability_list[num_abilities], ability,
				   sizeof(struct player_ability));
			ability_list[num_abilities++].group = PLAYER_FLAG_CLASS;
		}
	}

	/* Count the number of race powers we have */
	for (ability = player_abilities; ability; ability = ability->next) {
		if (race_has_ability(player->race, ability)) {
			memcpy(&ability_list[num_abilities], ability,
				   sizeof(struct player_ability));
			ability_list[num_abilities++].group = PLAYER_FLAG_RACE;
		}
	}

	/* View choices until user exits */
	view_ability_menu(ability_list, num_abilities);

	return;
}


/**
 * Interact with abilities -BR-
 */
void do_cmd_abilities(void)
{
	/* Might want to gain a new ability or browse old ones */
	if (player->upkeep->new_specialties > 0) {
		interact_with_specialties();
	} else {
		/* View existing specialties is the only option */
		view_abilities();
		return;
	}

	return;
}
