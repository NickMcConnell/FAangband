/**
 * \file cmd-misc.c
 * \brief Deal with miscellaneous commands.
 *
 * Copyright (c) 2010 Andi Sidwell
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
#include "buildid.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "effects.h"
#include "game-input.h"
#include "game-world.h"
#include "init.h"
#include "mon-lore.h"
#include "mon-util.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-history.h"
#include "player-util.h"
#include "target.h"


/**
 * Toggle wizard mode
 */
void do_cmd_wizard(void)
{
	/* Verify first time */
	if (!(player->noscore & NOSCORE_WIZARD)) {
		/* Mention effects */
		msg("You are about to enter 'wizard' mode for the very first time!");
		msg("This is a form of cheating, and your game will not be scored!");
		event_signal(EVENT_MESSAGE_FLUSH);

		/* Verify request */
		if (!get_check("Are you sure you want to enter wizard mode? "))
			return;

		/* Mark savefile */
		player->noscore |= NOSCORE_WIZARD;
	}

	/* Toggle mode */
	if (player->wizard) {
		player->wizard = false;
		msg("Wizard mode off.");
	} else {
		player->wizard = true;
		msg("Wizard mode on.");
	}

	/* Update monsters */
	player->upkeep->update |= (PU_MONSTERS);

	/* Redraw "title" */
	player->upkeep->redraw |= (PR_TITLE);
}

/**
 * Commit suicide
 */
void do_cmd_suicide(struct command *cmd)
{
	/* Commit suicide */
	player->is_dead = true;

	/* Cause of death */
	my_strcpy(player->died_from, "Quitting", sizeof(player->died_from));
}

/**
 * Change shape
 */
void do_cmd_reshape(struct command *cmd)
{
	/* Sanity */
	if (!(player_is_shapechanged(player) || player_has(player, PF_BEARSKIN))) {
		return;
	}

	/* Change back or change */
	if (player_is_shapechanged(player)) {
		if (get_check("Change back to your original form? " )) {
			player_resume_normal_shape(player);
		}
		return;
	} else {
		bool ident;
		const char *shape_name = (player->lev < 20) ? "bear cub" :
			((player->lev < 40) ? "bear" : "great bear");

		/* Confirm */
		if (!get_check(format("Assume the form of a %s? ", shape_name)))
			return;

		/* Change */
		effect_simple(EF_SHAPECHANGE, source_none(), "0",
					  shape_name_to_idx(shape_name), 0, 0, 0, 0, &ident);

		/* Use some energy */
		player->upkeep->energy_use = z_info->move_energy;
	}
}

/**
 * Record the player's thoughts as a note.
 *
 * This both displays the note back to the player and adds it to the game log.
 * Two fancy note types are supported: notes beginning with "/say" will be
 * written as 'Frodo says: "____"', and notes beginning with "/me" will
 * be written as 'Frodo ____'.
 */
void do_cmd_note(void)
{
	/* Allocate/Initialize strings to get and format user input. */
	char tmp[70];
	char note[90];
	my_strcpy(tmp, "", sizeof(tmp));
	my_strcpy(note, "", sizeof(note));

	/* Read a line of input from the user */
	if (!get_string("Note: ", tmp, sizeof(tmp))) return;

	/* Ignore empty notes */
	if (!tmp[0] || (tmp[0] == ' ')) return;

	/* Format the note correctly, supporting some cute /me commands */
	if (strncmp(tmp, "/say ", 5) == 0)
		strnfmt(note, sizeof(note), "-- %s says: \"%s\"", player->full_name,
				&tmp[5]);
	else if (strncmp(tmp, "/me", 3) == 0)
		strnfmt(note, sizeof(note), "-- %s%s", player->full_name, &tmp[3]);
	else
		strnfmt(note, sizeof(note), "-- Note: %s", tmp);

	/* Display the note (omitting the "-- " prefix) */
	msg("%s", &note[3]);

	/* Add a history entry */
	history_add(player, note, HIST_USER_INPUT);
}

/**
 * Display the time and date
 */
void do_cmd_time(void)
{
	int32_t len = 10L * z_info->day_length;
	int32_t tick = turn % len + len / 4;

	int day = turn / len + 1;
	int hour = (24 * tick / len) % 24;
	int min = (1440 * tick / len) % 60;


	/* Message */
	msg("This is day %d. The time is %d:%02d %s.", day,
		(hour % 12 == 0) ? 12 : (hour % 12), min,
		(hour < 12) ? "AM" : "PM");
}
