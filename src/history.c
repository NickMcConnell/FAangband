/*
 * File: history.c
 * Purpose: Character auto-history creation, management, and display
 *
 * Copyright (c) 2007 J.D. White
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
#include "history.h"
#include "mapmode.h"

/*
 * Number of slots available at birth in the player history list.  Defaults to
 * 10 and will expand automatically as new history entries are added, up the
 * the maximum defined value.
 */
#define HISTORY_BIRTH_SIZE  10
#define HISTORY_MAX 5000

/* max length of history output lines */
#define LINEWRAP        75

/* The historical list for the character */
history_info *history_list;

/* Index of first writable entry */
static size_t history_ctr;

/* Current size of history list */
static size_t history_size;


#define LIMITLOW(a, b) if (a < b) a = b;
#define LIMITHI(a, b) if (a > b) a = b;



/*
 * Initialise an empty history list.
 */
static void history_init(size_t entries)
{
	history_ctr = 0;
	history_size = entries;
	history_list = C_ZNEW(history_size, history_info);
}


/*
 * Clear any existing history.
 */
void history_clear(void)
{
	if (!history_list)
		return;

	FREE(history_list);
	history_ctr = 0;
	history_size = 0;
}


/*
 * Set the number of history items.
 */
static bool history_set_num(size_t num)
{
	history_info *new_list;

	if (num > HISTORY_MAX)
		num = HISTORY_MAX;

	if (num < history_size)
		return FALSE;
	if (num == history_size)
		return FALSE;

	/* Allocate new memory, copy across */
	/* XXX Should use mem_realloc() */
	new_list = C_ZNEW(num, history_info);
	C_COPY(new_list, history_list, history_ctr, history_info);
	FREE(history_list);

	history_list = new_list;
	history_size = num;

	return TRUE;
}


/*
 * Return the number of history entries.
 */
size_t history_get_num(void)
{
	return history_ctr;
}


/*
 * Mark artifact number `id` as known.
 */
static bool history_know_artifact(byte a_idx)
{
	size_t i = history_ctr;

	while (i--) {
		if (history_list[i].a_idx == a_idx) {
			history_list[i].type = HISTORY_ARTIFACT_KNOWN;
			return TRUE;
		}
	}

	return FALSE;
}


/*
 * Mark artifact number `id` as lost forever, either due to leaving it on a
 * level, or due to a store purging its inventory after the player sold it.
 */
bool history_lose_artifact(byte a_idx)
{
	size_t i = history_ctr;

	while (i--) {
		if (history_list[i].a_idx == a_idx) {
			history_list[i].type |= HISTORY_ARTIFACT_LOST;
			return TRUE;
		}
	}

	/* If we lost an artifact that didn't previously have a history, then we missed it */
	history_add_artifact(a_idx, FALSE, FALSE);

	return FALSE;
}


/*
 * Add an entry with text `event` to the history list, with type `type`
 * ("HISTORY_xxx" in defines.h), and artifact number `id` (0 for everything
 * else).
 *
 * Return TRUE on success.
 */
bool history_add_full(u16b type, byte a_idx, s16b place, s16b clev,
					  s32b turn, const char *text)
{
	/* Allocate the history list if needed */
	if (!history_list)
		history_init(HISTORY_BIRTH_SIZE);

	/* Expand the history list if appropriate */
	else if ((history_ctr == history_size)
			 && !history_set_num(history_size + 10))
		return FALSE;

	/* History list exists and is not full.  Add an entry at the current counter location. */
	history_list[history_ctr].type = type;
	history_list[history_ctr].place = place;
	history_list[history_ctr].clev = clev;
	history_list[history_ctr].a_idx = a_idx;
	history_list[history_ctr].turn = turn;
	my_strcpy(history_list[history_ctr].event,
			  text, sizeof(history_list[history_ctr].event));

	history_ctr++;

	return TRUE;
}


/*
 * Add an entry with text `event` to the history list, with type `type`
 * ("HISTORY_xxx" in defines.h), and artifact number `id` (0 for everything
 * else).
 *
 * Returne TRUE on success.
 */
bool history_add(const char *event, u16b type, byte a_idx)
{
	return history_add_full(type, a_idx, p_ptr->stage, p_ptr->lev, turn,
							event);
}


/*
 * Returns TRUE if the artifact denoted by a_idx is KNOWN in the history log.
 */
bool history_is_artifact_known(byte a_idx)
{
	size_t i = history_ctr;

	while (i--) {
		if (history_list[i].a_idx == a_idx
			&& (history_list[i].type & HISTORY_ARTIFACT_KNOWN))
			return TRUE;
	}
	return FALSE;
}


/*
 * Returns TRUE if the artifact denoted by a_idx is an active entry in
 * the history log (i.e. is not marked HISTORY_ARTIFACT_LOST).  This permits
 * proper handling of the case where the player loses an artifact but (in
 * preserve mode) finds it again later.
 */
static bool history_is_artifact_logged(byte a_idx)
{
	size_t i = history_ctr;

	while (i--) {
		/* Don't count ARTIFACT_LOST entries; then we can handle
		 * re-finding previously lost artifacts in preserve mode  */
		if (history_list[i].type & HISTORY_ARTIFACT_LOST)
			continue;

		if (history_list[i].a_idx == a_idx)
			return TRUE;
	}

	return FALSE;
}


/*
 * Adding artifacts to the history list is trickier than other operations.
 * This is a wrapper function that gets some of the logic out of places
 * where it really doesn't belong.  Call this to add an artifact to the history
 * list or make the history entry visible--history_add_artifact will make that
 * determination depending on what object_is_known returns for the artifact.
 */
bool history_add_artifact(byte a_idx, bool known, bool found)
{
	object_type object_type_body;
	object_type *o_ptr = &object_type_body;

	char o_name[80];
	char buf[80];
	u16b type;

	/* Make fake artifact for description purposes */
	object_wipe(o_ptr);
	make_fake_artifact(o_ptr, a_idx);
	object_desc(o_name, sizeof(o_name), o_ptr,
				ODESC_PREFIX | ODESC_BASE | ODESC_SPOIL);
	strnfmt(buf, sizeof(buf), (found) ? "Found %s" : "Missed %s", o_name);

	/* Known objects gets different treatment */
	if (known) {
		/* Try revealing any existing artifact, otherwise log it */
		if (history_is_artifact_logged(a_idx))
			history_know_artifact(a_idx);
		else
			history_add(buf, HISTORY_ARTIFACT_KNOWN, a_idx);
	} else {
		if (!history_is_artifact_logged(a_idx)) {
			type =
				HISTORY_ARTIFACT_UNKNOWN | (found ? 0 :
											HISTORY_ARTIFACT_LOST);
			history_add(buf, type, a_idx);
		} else {
			return FALSE;
		}
	}

	return TRUE;
}


/*
 * Convert all ARTIFACT_UNKNOWN history items to HISTORY_ARTIFACT_KNOWN.
 * Use only after player retirement/death for the final character dump.
 */
void history_unmask_unknown(void)
{
	size_t i = history_ctr;

	while (i--) {
		if (history_list[i].type & HISTORY_ARTIFACT_UNKNOWN) {
			history_list[i].type &= ~(HISTORY_ARTIFACT_UNKNOWN);
			history_list[i].type |= HISTORY_ARTIFACT_KNOWN;
		}
	}
}


/*
 * Used to determine whether the history entry is visible in the listing or not.
 * Returns TRUE if the item is masked -- that is, if it is invisible
 *
 * All artifacts are now sensed on pickup, so nothing is now invisible. The
 * KNOWN / UNKNOWN distinction is if we had fully identified it or not
 */
static bool history_masked(size_t i)
{
	if (history_list[i].type & HISTORY_ARTIFACT_KNOWN)
		return FALSE;

	return TRUE;
}

/*
 * Finds the index of the last printable (non-masked) item in the history list.
 */
static size_t last_printable_item(void)
{
	size_t i = history_ctr;

	while (i--) {
		if (!history_masked(i))
			break;
	}

	return i;
}

static void print_history_header(void)
{
	char buf[80];

	/* Print the header (character name and title) */
	strnfmt(buf, sizeof(buf), "%s the %s %s", op_ptr->full_name,
			rp_ptr->name, cp_ptr->name);

	c_put_str(TERM_WHITE, buf, 0, 0);
	c_put_str(TERM_WHITE,
			  "============================================================",
			  1, 0);
	c_put_str(TERM_WHITE, "CHAR.", 2, 34);
	c_put_str(TERM_WHITE, "|   TURN  |      LOCATION        |LEVEL| EVENT",
			  3, 0);
	c_put_str(TERM_WHITE,
			  "============================================================",
			  4, 0);
}

static void history_get_place(char *place, size_t len, int i)
{
	int region = stage_map[history_list[i].place][LOCALITY];
	int lev = stage_map[history_list[i].place][DEPTH];
	char buf[30];

	/* Get the location name */
	if (lev)
		strnfmt(buf, sizeof(buf), "%15s%4d ", locality_name[region], lev);
	else if ((region != UNDERWORLD) && (region != MOUNTAIN_TOP))
		strnfmt(buf, sizeof(buf), "%15s Town", locality_name[region]);
	else
		strnfmt(buf, sizeof(buf), "%15s     ", locality_name[region]);

	my_strcpy(place, buf, len);
}
/* Handles all of the display functionality for the history list. */
void history_display(void)
{
	int row, wid, hgt, page_size;
	char buf[90];
	static size_t first_item = 0;
	size_t max_item = last_printable_item();
	size_t i;

	Term_get_size(&wid, &hgt);

	/* Six lines provide space for the header and footer */
	page_size = hgt - 6;

	screen_save();

	while (1) {
		struct keypress ch;

		Term_clear();

		/* Print everything to screen */
		print_history_header();

		row = 0;
		for (i = first_item; row <= page_size && i < history_ctr; i++) {
			char location[30];

			/* Skip messages about artifacts not yet IDed. */
			if (history_masked(i))
				continue;

			/* Get location name */
			history_get_place(location, sizeof(location), i);

			strnfmt(buf, sizeof(buf), "%10d%22s%5d    %s",
					history_list[i].turn,
					location, history_list[i].clev, history_list[i].event);

			if (history_list[i].type & HISTORY_ARTIFACT_LOST)
				my_strcat(buf, " (LOST)", sizeof(buf));

			/* Size of header = 5 lines */
			prt(buf, row + 5, 0);
			row++;
		}
		prt("[Arrow keys scroll, p for previous page, n for next page, ESC to exit.]", hgt - 1, 0);

		ch = inkey();

		if (ch.code == 'n') {
			size_t scroll_to = first_item + page_size;

			while (history_masked(scroll_to)
				   && scroll_to < history_ctr - 1)
				scroll_to++;

			first_item = (scroll_to < max_item ? scroll_to : max_item);
		} else if (ch.code == 'p') {
			int scroll_to = first_item - page_size;

			while (history_masked(scroll_to) && scroll_to > 0)
				scroll_to--;

			first_item = (scroll_to >= 0 ? scroll_to : 0);
		} else if (ch.code == ARROW_DOWN) {
			size_t scroll_to = first_item + 1;

			while (history_masked(scroll_to)
				   && scroll_to < history_ctr - 1)
				scroll_to++;

			first_item = (scroll_to < max_item ? scroll_to : max_item);
		} else if (ch.code == ARROW_UP) {
			int scroll_to = first_item - 1;

			while (history_masked(scroll_to) && scroll_to > 0)
				scroll_to--;

			first_item = (scroll_to >= 0 ? scroll_to : 0);
		} else if (ch.code == ESCAPE)
			break;
	}

	screen_load();

	return;
}

/**
 * Get a history entry colour
 */
byte history_colour(u16b type)
{
	if (type & HISTORY_PLAYER_BIRTH)
		return TERM_L_UMBER;
	else if (type & HISTORY_PLAYER_DEATH)
		return TERM_VIOLET;
	else if (type & HISTORY_GAIN_SPECIALTY)
		return TERM_RED;
	else if (type & HISTORY_GAIN_LEVEL)
		return TERM_YELLOW;
	else if (type & HISTORY_SLAY_UNIQUE)
		return TERM_UMBER;
	else if (type & HISTORY_MOVE_HOUSE)
		return TERM_GREEN;
	else if (type & HISTORY_ARTIFACT_LOST)
		return TERM_BLUE;
	else if (type & HISTORY_ARTIFACT_KNOWN)
		return TERM_L_BLUE;
	else if (type & HISTORY_USER_INPUT)
		return TERM_WHITE;
	else
		return TERM_WHITE;
}


/* Dump character history to a file, which we assume is already open. */
void dump_history(char_attr_line * line, int *curr_line, bool * dead)
{
	size_t i;
	char buf[90];
	char buf1[90];

	/* Dump notes */
	for (i = 0; i < (last_printable_item() + 1); i++) {
		int length, length_info;
		char info_note[43];
		char place[32];
		byte attr = history_colour(history_list[i].type);

		/* Skip not-yet-IDd artifacts */
		if (history_masked(i))
			continue;

		/* Divider before death */
		if ((history_list[i].type & HISTORY_PLAYER_DEATH) && !(*dead)) {
			*dead = TRUE;
			dump_put_str(TERM_WHITE,
						 "============================================================",
						 0);
			(*curr_line)++;
			dump_ptr = (char_attr *) & line[*curr_line];
		}

		/* Get the note */
		strnfmt(buf, sizeof(buf), "%s", history_list[i].event);

		if (history_list[i].type & HISTORY_ARTIFACT_LOST)
			my_strcat(buf, " (LOST)", sizeof(buf));

		/* Get the location name */
		history_get_place(place, sizeof(place), i);

		/* Make preliminary part of note */
		strnfmt(info_note, sizeof(info_note), "%10d%22s%5d    ",
				history_list[i].turn, place, history_list[i].clev);

		/* Write the info note */
		dump_put_str(TERM_WHITE, info_note, 0);

		/* Get the length of the history_list */
		length_info = strlen(info_note);
		length = strlen(buf);

		/* Break up long notes */
		if ((length + length_info) > LINEWRAP) {
			bool keep_going = TRUE;
			int startpoint = 0;
			int endpoint;

			while (keep_going) {
				/* Don't print more than the set linewrap amount */
				endpoint = startpoint + LINEWRAP - strlen(info_note) + 1;

				/* Find a breaking point */
				while (TRUE) {
					/* Are we at the end of the line? */
					if (endpoint >= length) {
						/* Print to the end */
						endpoint = length;
						keep_going = FALSE;
						break;
					}

					/* Mark the most recent space or dash in the string */
					else if ((buf[endpoint] == ' ')
							 || (buf[endpoint] == '-'))
						break;

					/* No spaces in the line, so break in the middle of text */
					else if (endpoint == startpoint) {
						endpoint =
							startpoint + LINEWRAP - strlen(info_note) + 1;
						break;
					}

					/* check previous char */
					endpoint--;
				}

				/* Make a continued note if applicable */
				if (startpoint)
					dump_put_str(TERM_WHITE,
								 "                                          ",
								 0);

				/* Write that line */
				strnfmt(buf1, endpoint - startpoint + 1, "%s",
						&buf[startpoint]);
				dump_put_str(attr, buf1, strlen(info_note));

				/* Break the line */
				(*curr_line)++;
				dump_ptr = (char_attr *) & line[*curr_line];

				/* Prepare for the next line */
				startpoint = endpoint + 1;

			}
		}

		/* Add note to buffer */
		else {
			/* Print the note */
			dump_put_str(attr, buf, strlen(info_note));

			/* Break the line */
			(*curr_line)++;
			dump_ptr = (char_attr *) & line[*curr_line];
		}
	}

	return;
}
