/** \file cmd2.c 
    \brief Commands, part 2

 * Going up and down stairs, items that a chest may contain, opening 
 * chests, tunnelling, disarming, opening doors, alter adjacent grid, 
 * spiking, starting various movement and resting routines, moving house.
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
#include "files.h"
#include "history.h"
#include "monster.h"
#include "spells.h"
#include "squelch.h"
#include "trap.h"

/** 
 * Move house to the current town
 */

void do_cmd_move_house(void)
{
    char buf[80];
    int old_home = 0, new_home = 0, i = 0;
    store_type temp;

    const char *town = locality_name[stage_map[p_ptr->stage][LOCALITY]];

    if (!p_ptr->depth) {
	/* Already home */
	if (p_ptr->stage == p_ptr->home) {
	    msg("You already live here!");
	    flush();
	    return;
	}

	/* Check */
	sprintf(buf, "Do you really want to move to %s?", town);
	if (!get_check(buf))
	    return;

	/* No need to move for thrall mode */
	if (p_ptr->home) {
	    /* Get the current home */
	    while (1) {
		while (type_of_store[old_home] != STORE_HOME)
		    old_home++;
		if (p_ptr->home == towns[i])
		    break;
		old_home++;
		i++;
	    }
	    i = 0;

	    /* Get the new home */
	    while (1) {
		while (type_of_store[new_home] != STORE_HOME)
		    new_home++;
		if (p_ptr->stage == towns[i])
		    break;
		new_home++;
		i++;
	    }

	    /* Transfer the gear */
	    temp = store[new_home];
	    store[new_home] = store[old_home];
	    store[old_home] = temp;
	}

	/* Set the new town */
	p_ptr->home = p_ptr->stage;
	msg("Your home will be here when you return.");

	/* Moved house */
	sprintf(buf, "Moved house to %s.", town);

	/* Write message */
	history_add(buf, HISTORY_MOVE_HOUSE, 0);

    } else
	msg("You can only move to another town!");

    /* Flush messages */
    flush();

    return;
}

/** Path translations */

typedef struct {
    int path;
    int direction;
    int return_path;
    bool eastwest;
} pather;

static const pather path_data[] =
{
    {FEAT_LESS_NORTH, NORTH, FEAT_MORE_SOUTH, FALSE},
    {FEAT_MORE_NORTH, NORTH, FEAT_LESS_SOUTH, FALSE},
    {FEAT_LESS_EAST, EAST, FEAT_MORE_WEST, TRUE},
    {FEAT_MORE_EAST, EAST, FEAT_LESS_WEST, TRUE},
    {FEAT_LESS_SOUTH, SOUTH, FEAT_MORE_NORTH, FALSE},
    {FEAT_MORE_SOUTH, SOUTH, FEAT_LESS_NORTH, FALSE},
    {FEAT_LESS_WEST, WEST, FEAT_MORE_EAST, TRUE},
    {FEAT_MORE_WEST, WEST, FEAT_LESS_EAST, TRUE}
};


/**
 * Go to less danger
 */
void do_cmd_go_up(cmd_code code, cmd_arg args[])
{
    int py = p_ptr->py;
    int px = p_ptr->px;
    size_t i;

    byte pstair = cave_feat[py][px];
    feature_type *f_ptr = &f_info[pstair];

    /* Get the path data */
    for (i = 0; i < N_ELEMENTS(path_data); i++)
    {
	if (path_data[i].path == pstair) break;
    }

    /* Undefined path */
    if (!(tf_has(f_ptr->flags, TF_STAIR) || tf_has(f_ptr->flags, TF_PATH)))
    {
	msg("I see no path or staircase here.");
	return;
    }

    /* Check for appropriate terrain */
    if (!(tf_has(f_ptr->flags, TF_STAIR) || tf_has(f_ptr->flags, TF_PATH)))
    {
	msg("I see no path or staircase here.");
	return;
    }
    else if (tf_has(f_ptr->flags, TF_DOWNSTAIR))
    {
	if (tf_has(f_ptr->flags, TF_PATH))
	{
	    msg("This is a path to greater danger.");
	    return;
	}
	else
	{
	    msg("This staircase leads down.");
	    return;
	}
    }

    /* Handle ironman */
    if (OPT(adult_ironman))
    {
	/* Upstairs */
	if (tf_has(f_ptr->flags, TF_STAIR))
	{
	    msg("Nothing happens.");
	    return;
	}
	else
	{
	    int next = stage_map[p_ptr->stage][path_data[i].direction];

	    /* New towns are OK */
	    if ((next == p_ptr->last_stage) || stage_map[next][DEPTH])
	    {
		msg("Nothing happens.");
		return;
	    }
	}
    }

    /* Make certain the player really wants to leave a themed level. -LM- */
    if (p_ptr->themed_level)
	if (!get_check("This level will never appear again.  Really leave?"))
	    return;


    /* Hack -- take a turn */
    p_ptr->energy_use = 100;

    /* Success */
    if (pstair == FEAT_LESS)
    {
	/* Magical portal for dungeon-only games */
	if (OPT(adult_dungeon) && (p_ptr->depth != 1)
	    && ((stage_map[p_ptr->stage][LOCALITY]) !=
		(stage_map[stage_map[p_ptr->stage][UP]][LOCALITY])))
	{
	    /* Land properly */
	    p_ptr->last_stage = NOWHERE;

	    /* Portal */
	    msgt(MSG_STAIRS_UP, "You trigger a magic portal.");

	    /* New stage */
	    p_ptr->stage = stage_map[p_ptr->stage][UP];

	    /* New depth */
	    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	    /* Leaving */
	    p_ptr->leaving = TRUE;

	    return;
	}

	/* stairs */
	msgt(MSG_STAIRS_UP, "You enter a maze of up staircases.");

	/* make the way back */
	p_ptr->create_stair = FEAT_MORE;
    }
    else if (pstair == FEAT_LESS_SHAFT)
    {
	/* Magical portal for dungeon-only games */
	if (OPT(adult_dungeon) && (p_ptr->depth != 2)
	    && ((stage_map[p_ptr->stage][LOCALITY]) !=
		(stage_map[stage_map[stage_map[p_ptr->stage][UP]][UP]]
		 [LOCALITY])))
	{
	    /* Land properly */
	    p_ptr->last_stage = NOWHERE;

	    /* Portal */
	    msgt(MSG_STAIRS_UP, "You trigger a magic portal.");

	    /* New stage */
	    p_ptr->stage = stage_map[stage_map[p_ptr->stage][UP]][UP];

	    /* New depth */
	    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	    /* Leaving */
	    p_ptr->leaving = TRUE;

	    return;
	}

	/* shaft */
	msgt(MSG_STAIRS_UP, "You enter a maze of up staircases.");

	/* make the way back */
	p_ptr->create_stair = FEAT_MORE_SHAFT;
    }
    else
    {
	/* path */
	msgt(MSG_STAIRS_UP, "You enter a winding path to less danger.");

	/* make the way back */
	p_ptr->create_stair = path_data[i].return_path;
    }

    /* Remember where we came from */
    p_ptr->last_stage = p_ptr->stage;

    /* New stage (really need a check here...) */
    if (tf_has(f_ptr->flags, TF_PATH))
	p_ptr->stage =
	    stage_map[p_ptr->stage][path_data[i].direction];
    else if (pstair == FEAT_LESS_SHAFT)
	p_ptr->stage = stage_map[stage_map[p_ptr->stage][UP]][UP];
    else
	p_ptr->stage = stage_map[p_ptr->stage][UP];

    /* Handle underworld stuff */
    if (stage_map[p_ptr->last_stage][LOCALITY] == UNDERWORLD)
    {
	/* Reset */
	stage_map[UNDERWORLD_STAGE][UP] = 0;
	stage_map[p_ptr->stage][DOWN] = 0;
	stage_map[UNDERWORLD_STAGE][DEPTH] = 0;

	/* No way back */
	p_ptr->create_stair = 0;
    }

    /* Record the non-obvious exit coordinate */
    if (tf_has(f_ptr->flags, TF_PATH))
    {
	if (path_data[i].eastwest)
	    p_ptr->path_coord = py;
	else
	    p_ptr->path_coord = px;
    }

    /* Set the depth */
    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

    /* Leaving */
    p_ptr->leaving = TRUE;
}


/**
 * Go to greater danger
 */
void do_cmd_go_down(cmd_code code, cmd_arg args[])
{
    int py = p_ptr->py;
    int px = p_ptr->px;
    size_t i;

    byte pstair = cave_feat[py][px];
    feature_type *f_ptr = &f_info[pstair];

    /* Get the path data */
    for (i = 0; i < N_ELEMENTS(path_data); i++)
    {
	if (path_data[i].path == pstair) break;
    }

    /* Check for appropriate terrain */
    if (!(tf_has(f_ptr->flags, TF_STAIR) || tf_has(f_ptr->flags, TF_PATH)))
    {
	msg("I see no path or staircase here.");
	return;
    }
    else if (tf_has(f_ptr->flags, TF_UPSTAIR))
    {
	if (tf_has(f_ptr->flags, TF_PATH))
	{
	    msg("This is a path to less danger.");
	    return;
	}
	else
	{
	    msg("This staircase leads up.");
	    return;
	}
    }

    /* Handle ironman */
    if (OPT(adult_ironman) && !p_ptr->depth && !OPT(adult_dungeon)) {
	int j, other;
	int next = stage_map[p_ptr->stage][path_data[i].direction];

	/* Check if this is the right way out of town */
	for (j = NORTH; j <= WEST; j++) {
	    other = stage_map[p_ptr->stage][j];
	    if (stage_map[next][DEPTH] < stage_map[other][DEPTH]) {
		msg("Nothing happens.");
		return;
	    }
	}
    }

    /* Make certain the player really wants to leave a themed level. -LM- */
    if (p_ptr->themed_level)
	if (!get_check("This level will never appear again.  Really leave?"))
	    return;


    /* Hack -- take a turn */
    p_ptr->energy_use = 100;

    /* Success */

    /* Remember where we came from */
    p_ptr->last_stage = p_ptr->stage;

    if (pstair == FEAT_MORE) {
	int location;

	/* Magical portal for ironman */
	if (OPT(adult_ironman) && !stage_map[p_ptr->stage][DOWN]
	    && !OPT(adult_dungeon)) {
	    /* Get choice */
	    if (!jump_menu(p_ptr->depth + 1, &location))
		return;

	    /* Land properly */
	    p_ptr->last_stage = NOWHERE;

	    /* New stage */
	    p_ptr->stage = location;

	    /* New depth */
	    p_ptr->depth = stage_map[location][DEPTH];

	    /* Leaving */
	    p_ptr->leaving = TRUE;

	    return;
	}

	/* Magical portal for dungeon-only games */
	if (OPT(adult_dungeon) && (p_ptr->depth)
	    && ((stage_map[p_ptr->stage][LOCALITY]) !=
		(stage_map[stage_map[p_ptr->stage][DOWN]][LOCALITY]))) {
	    /* Land properly */
	    p_ptr->last_stage = NOWHERE;

	    /* Portal */
	    msgt(MSG_STAIRS_DOWN, "You trigger a magic portal.");

	    /* New stage */
	    p_ptr->stage = stage_map[p_ptr->stage][DOWN];

	    /* New depth */
	    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	    /* Leaving */
	    p_ptr->leaving = TRUE;

	    return;
	}

	/* stairs */
	msgt(MSG_STAIRS_DOWN, "You enter a maze of down staircases.");

	/* New stage */
	p_ptr->stage = stage_map[p_ptr->stage][DOWN];

	/* make the way back */
	p_ptr->create_stair = FEAT_LESS;
    }
    else if (pstair == FEAT_MORE_SHAFT)
    {
	/* Magical portal for dungeon-only games */
	if (OPT(adult_dungeon)
	    && ((stage_map[p_ptr->stage][LOCALITY]) !=
		(stage_map[stage_map[stage_map[p_ptr->stage][DOWN]][DOWN]]
		 [LOCALITY]))) {
	    /* Land properly */
	    p_ptr->last_stage = NOWHERE;

	    /* Portal */
	    msgt(MSG_STAIRS_DOWN, "You trigger a magic portal.");

	    /* New stage */
	    p_ptr->stage = stage_map[stage_map[p_ptr->stage][DOWN]][DOWN];

	    /* New depth */
	    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	    /* Leaving */
	    p_ptr->leaving = TRUE;

	    return;
	}

	/* stairs */
	msgt(MSG_STAIRS_DOWN, "You enter a maze of down staircases.");

	/* New stage */
	p_ptr->stage = stage_map[stage_map[p_ptr->stage][DOWN]][DOWN];

	/* make the way back */
	p_ptr->create_stair = FEAT_LESS_SHAFT;
    }
    else
    {
	/* New stage */
	p_ptr->stage =
	    stage_map[p_ptr->stage][path_data[i].direction];

	/* Check for Nan Dungortheb */
	if (stage_map[p_ptr->stage][LOCALITY] == NAN_DUNGORTHEB)

	    /* scree slope */
	    msgt(MSG_STAIRS_DOWN,
		    "You slide down amidst a small avalanche.");

	else
	{
	    /* Make the way back */
	    p_ptr->create_stair = path_data[i].return_path;

	    /* path */
	    msgt(MSG_STAIRS_DOWN,
		    "You enter a winding path to greater danger.");
	}
    }

    /* Handle mountaintop stuff */
    if (stage_map[p_ptr->last_stage][LOCALITY] == MOUNTAIN_TOP)
    {
	/* Reset */
	stage_map[MOUNTAINTOP_STAGE][DOWN] = 0;
	stage_map[p_ptr->stage][UP] = 0;
	stage_map[MOUNTAINTOP_STAGE][DEPTH] = 0;

	/* No way back */
	p_ptr->create_stair = 0;
    }

    /* Record the non-obvious exit coordinate */
    if (path_data[i].eastwest)
	p_ptr->path_coord = py;
    else
	p_ptr->path_coord = px;

    /* Set the depth */
    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

    /* Check for quests */
    if (OPT(adult_dungeon) && is_quest(p_ptr->stage) && (p_ptr->depth < 100)) {
	int i;
	monster_race *r_ptr = NULL;

	/* Find the questor */
	for (i = 0; i < z_info->r_max; i++) {
	    r_ptr = &r_info[i];
	    if ((rf_has(r_ptr->flags, RF_QUESTOR))
		&& (r_ptr->level == p_ptr->depth))
		break;
	}

	/* Give the info */
	msg("This level is home to %s.", r_ptr->name);
    }

    /* Leaving */
    p_ptr->leaving = TRUE;

}


/**
 * Simple command to "search" for one turn
 */
void do_cmd_search(cmd_code code, cmd_arg args[])
{
    /* Only take a turn if attempted */
    if (search(TRUE))
	p_ptr->energy_use = 100;
}


/**
 * Hack -- toggle search mode
 */
void do_cmd_toggle_search(cmd_code code, cmd_arg args[])
{
    /* Stop searching */
    if (p_ptr->searching) {
	/* Clear the searching flag */
	p_ptr->searching = FALSE;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);
    }

    /* Start searching */
    else {
	/* Set the searching flag */
	p_ptr->searching = TRUE;

	/* Update stuff */
	p_ptr->update |= (PU_BONUS);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_STATE | PR_SPEED);
    }
}



/**
 * Determine if a grid contains a chest
 */
static s16b chest_check(int y, int x)
{
    s16b this_o_idx, next_o_idx = 0;


    /* Scan all objects in the grid */
    for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr;

	/* Acquire object */
	o_ptr = &o_list[this_o_idx];

	/* Acquire next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Skip unknown chests XXX XXX */
	/* if (!o_ptr->marked) continue; */

	/* Check for chest */
	if (o_ptr->tval == TV_CHEST)
	    return (this_o_idx);
    }

    /* No chest */
    return (0);
}

/**
 * A function that returns the tval of the items that will be generated
 * when a chest is opened. -LM-
 */
static byte get_choice(void)
{
    byte choice;

    choice = randint1(100);

    switch (p_ptr->pclass) {
    case CLASS_WARRIOR:
	{
	    if (choice < 2)
		return (TV_SHOT);
	    if (choice < 5)
		return (TV_ARROW);
	    if (choice < 9)
		return (TV_BOLT);
	    if (choice < 13)
		return (TV_BOW);
	    if (choice < 25)
		return (TV_HAFTED);
	    if (choice < 37)
		return (TV_POLEARM);
	    if (choice < 49)
		return (TV_SWORD);
	    if (choice < 54)
		return (TV_BOOTS);
	    if (choice < 59)
		return (TV_GLOVES);
	    if (choice < 64)
		return (TV_HELM);
	    if (choice < 67)
		return (TV_CROWN);
	    if (choice < 72)
		return (TV_SHIELD);
	    if (choice < 76)
		return (TV_CLOAK);
	    if (choice < 79)
		return (TV_SOFT_ARMOR);
	    if (choice < 89)
		return (TV_HARD_ARMOR);
	    if (choice < 95)
		return (TV_SCROLL);
	    if (choice < 101)
		return (TV_POTION);
	    break;
	}

    case CLASS_MAGE:
	{
	    if (choice < 4)
		return (TV_BOOTS);
	    if (choice < 7)
		return (TV_HELM);
	    if (choice < 11)
		return (TV_CROWN);
	    if (choice < 16)
		return (TV_SHIELD);
	    if (choice < 22)
		return (TV_CLOAK);
	    if (choice < 28)
		return (TV_SOFT_ARMOR);
	    if (choice < 34)
		return (TV_SCROLL);
	    if (choice < 40)
		return (TV_POTION);
	    if (choice < 46)
		return (TV_RING);
	    if (choice < 52)
		return (TV_AMULET);
	    if (choice < 64)
		return (TV_WAND);
	    if (choice < 76)
		return (TV_STAFF);
	    if (choice < 88)
		return (TV_ROD);
	    if (choice < 101)
		return (TV_MAGIC_BOOK);
	    break;
	}

    case CLASS_PRIEST:
	{
	    if (choice < 4)
		return (TV_BOOTS);
	    if (choice < 7)
		return (TV_HELM);
	    if (choice < 12)
		return (TV_CROWN);
	    if (choice < 16)
		return (TV_SHIELD);
	    if (choice < 21)
		return (TV_GLOVES);
	    if (choice < 27)
		return (TV_CLOAK);
	    if (choice < 33)
		return (TV_SOFT_ARMOR);
	    if (choice < 39)
		return (TV_SCROLL);
	    if (choice < 46)
		return (TV_POTION);
	    if (choice < 53)
		return (TV_RING);
	    if (choice < 60)
		return (TV_AMULET);
	    if (choice < 69)
		return (TV_HAFTED);
	    if (choice < 76)
		return (TV_WAND);
	    if (choice < 81)
		return (TV_STAFF);
	    if (choice < 86)
		return (TV_ROD);
	    if (choice < 101)
		return (TV_PRAYER_BOOK);
	    break;
	}

    case CLASS_ROGUE:
	{
	    if (choice < 11)
		return (TV_SHOT);
	    if (choice < 17)
		return (TV_ARROW);
	    if (choice < 20)
		return (TV_BOLT);
	    if (choice < 29)
		return (TV_BOW);
	    if (choice < 33)
		return (TV_HAFTED);
	    if (choice < 37)
		return (TV_POLEARM);
	    if (choice < 44)
		return (TV_SWORD);
	    if (choice < 48)
		return (TV_BOOTS);
	    if (choice < 50)
		return (TV_GLOVES);
	    if (choice < 54)
		return (TV_HELM);
	    if (choice < 58)
		return (TV_CROWN);
	    if (choice < 62)
		return (TV_SHIELD);
	    if (choice < 68)
		return (TV_CLOAK);
	    if (choice < 71)
		return (TV_SOFT_ARMOR);
	    if (choice < 74)
		return (TV_HARD_ARMOR);
	    if (choice < 80)
		return (TV_SCROLL);
	    if (choice < 86)
		return (TV_POTION);
	    if (choice < 92)
		return (TV_STAFF);
	    if (choice < 101)
		return (TV_MAGIC_BOOK);
	    break;
	}

    case CLASS_RANGER:
	{
	    if (choice < 15)
		return (TV_ARROW);
	    if (choice < 21)
		return (TV_BOLT);
	    if (choice < 31)
		return (TV_BOW);
	    if (choice < 34)
		return (TV_HAFTED);
	    if (choice < 37)
		return (TV_POLEARM);
	    if (choice < 40)
		return (TV_SWORD);
	    if (choice < 45)
		return (TV_RING);
	    if (choice < 50)
		return (TV_AMULET);
	    if (choice < 54)
		return (TV_BOOTS);
	    if (choice < 58)
		return (TV_GLOVES);
	    if (choice < 62)
		return (TV_HELM);
	    if (choice < 66)
		return (TV_CROWN);
	    if (choice < 70)
		return (TV_SHIELD);
	    if (choice < 74)
		return (TV_CLOAK);
	    if (choice < 78)
		return (TV_SOFT_ARMOR);
	    if (choice < 82)
		return (TV_ROD);
	    if (choice < 87)
		return (TV_WAND);
	    if (choice < 92)
		return (TV_STAFF);
	    if (choice < 101)
		return (TV_DRUID_BOOK);
	    break;
	}

    case CLASS_PALADIN:
	{
	    if (choice < 4)
		return (TV_BOOTS);
	    if (choice < 8)
		return (TV_HELM);
	    if (choice < 12)
		return (TV_CROWN);
	    if (choice < 19)
		return (TV_SHIELD);
	    if (choice < 23)
		return (TV_GLOVES);
	    if (choice < 28)
		return (TV_CLOAK);
	    if (choice < 35)
		return (TV_HARD_ARMOR);
	    if (choice < 40)
		return (TV_SCROLL);
	    if (choice < 45)
		return (TV_POTION);
	    if (choice < 52)
		return (TV_RING);
	    if (choice < 59)
		return (TV_AMULET);
	    if (choice < 77)
		return (TV_HAFTED);
	    if (choice < 82)
		return (TV_ROD);
	    if (choice < 87)
		return (TV_WAND);
	    if (choice < 92)
		return (TV_STAFF);
	    if (choice < 101)
		return (TV_PRAYER_BOOK);
	    break;
	}


    case CLASS_DRUID:
	{
	    if (choice < 3)
		return (TV_BOOTS);
	    if (choice < 5)
		return (TV_HELM);
	    if (choice < 11)
		return (TV_CROWN);
	    if (choice < 14)
		return (TV_SHIELD);
	    if (choice < 24)
		return (TV_CLOAK);
	    if (choice < 27)
		return (TV_SOFT_ARMOR);
	    if (choice < 37)
		return (TV_SCROLL);
	    if (choice < 48)
		return (TV_POTION);
	    if (choice < 58)
		return (TV_RING);
	    if (choice < 68)
		return (TV_AMULET);
	    if (choice < 74)
		return (TV_WAND);
	    if (choice < 80)
		return (TV_STAFF);
	    if (choice < 86)
		return (TV_ROD);
	    if (choice < 101)
		return (TV_DRUID_BOOK);
	    break;
	}


    case CLASS_NECRO:
	{
	    if (choice < 3)
		return (TV_BOOTS);
	    if (choice < 5)
		return (TV_HELM);
	    if (choice < 11)
		return (TV_CROWN);
	    if (choice < 14)
		return (TV_SHIELD);
	    if (choice < 24)
		return (TV_CLOAK);
	    if (choice < 27)
		return (TV_SOFT_ARMOR);
	    if (choice < 37)
		return (TV_SCROLL);
	    if (choice < 48)
		return (TV_POTION);
	    if (choice < 58)
		return (TV_RING);
	    if (choice < 68)
		return (TV_AMULET);
	    if (choice < 74)
		return (TV_WAND);
	    if (choice < 80)
		return (TV_STAFF);
	    if (choice < 86)
		return (TV_ROD);
	    if (choice < 101)
		return (TV_NECRO_BOOK);
	    break;
	}

    case CLASS_ASSASSIN:
	{
	    if (choice < 11)
		return (TV_SHOT);
	    if (choice < 17)
		return (TV_ARROW);
	    if (choice < 20)
		return (TV_BOLT);
	    if (choice < 29)
		return (TV_BOW);
	    if (choice < 33)
		return (TV_HAFTED);
	    if (choice < 37)
		return (TV_POLEARM);
	    if (choice < 44)
		return (TV_SWORD);
	    if (choice < 48)
		return (TV_BOOTS);
	    if (choice < 50)
		return (TV_GLOVES);
	    if (choice < 54)
		return (TV_HELM);
	    if (choice < 58)
		return (TV_CROWN);
	    if (choice < 62)
		return (TV_SHIELD);
	    if (choice < 68)
		return (TV_CLOAK);
	    if (choice < 71)
		return (TV_SOFT_ARMOR);
	    if (choice < 74)
		return (TV_HARD_ARMOR);
	    if (choice < 80)
		return (TV_SCROLL);
	    if (choice < 86)
		return (TV_POTION);
	    if (choice < 92)
		return (TV_STAFF);
	    if (choice < 101)
		return (TV_NECRO_BOOK);
	    break;
	}
    }
    /* If the function fails, do not specify a tval */
    return (0);
}

/**
 * Allocate objects upon opening a chest.
 *
 * Disperse treasures from the given chest, centered at (x,y).
 *
 * In Oangband, chests are nice finds.  Small chests distribute 3-5 items,
 * while large chests can distribute 5-7.  Item types are biased to be
 * useful for the character, and they can frequently be of good quality 
 * (or better).   Code in object2.c helps these items be even better. -LM-
 *
 * The "value" of the items in a chest is based on the "power" of the chest,
 * which is in turn based on the level on which the chest is generated.
 */
static void chest_death(bool scatter, int y, int x, s16b o_idx)
{
    int number, i;
    bool obj_success = FALSE;
    object_type *o_ptr;

    object_type *i_ptr;
    object_type object_type_body;

    /* Access chest */
    o_ptr = &o_list[o_idx];

    /* Determine how much to drop. */
    if (o_ptr->sval >= SV_CHEST_MIN_LARGE)
	number = 4 + randint1(3);
    else
	number = 2 + randint1(3);

    /* Zero pval means empty chest */
    if (!o_ptr->pval)
	number = 0;

    /* Opening a chest */
    opening_chest = TRUE;

    /* Determine the "value" of the items */
    object_level = ABS(o_ptr->pval);

    /* Select an item type that the chest will disperse. */
    required_tval = get_choice();

    /* Drop some objects (non-chests) */
    for (; number > 0; --number) {
	/* Get local object */
	i_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(i_ptr);

	/* Make an object with a specified tval.  Grant a possibility for items 
	 * to be forced good, or even great.  With the new definitions of
	 * goodness, this can make for quite interesting loot.  -LM- */
	switch (required_tval) {
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_BOW:
	case TV_SWORD:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_DIGGING:
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
	    {
		if (randint1(200) < object_level) {
		    obj_success = make_object(i_ptr, TRUE, TRUE, TRUE);
		    break;
		}

		else if (randint1(40) < object_level) {
		    obj_success = make_object(i_ptr, TRUE, FALSE, TRUE);
		    break;
		} else {
		    obj_success = make_object(i_ptr, FALSE, FALSE, TRUE);
		    break;
		}
	    }

	case TV_MAGIC_BOOK:
	case TV_PRAYER_BOOK:
	case TV_DRUID_BOOK:
	case TV_NECRO_BOOK:
	    {
		if (randint1(80) < object_level) {
		    obj_success = make_object(i_ptr, TRUE, FALSE, TRUE);
		} else {
		    obj_success = make_object(i_ptr, FALSE, FALSE, TRUE);
		}

		break;
	    }

	case TV_SCROLL:
	case TV_POTION:
	case TV_RING:
	case TV_AMULET:
	case TV_WAND:
	case TV_STAFF:
	case TV_ROD:
	    {
		if (randint1(100) < (object_level - 10) / 2) {
		    obj_success = make_object(i_ptr, TRUE, FALSE, TRUE);
		}

		else {
		    obj_success = make_object(i_ptr, FALSE, FALSE, TRUE);
		}

		break;
	    }

	default:
	    {
		obj_success = make_object(i_ptr, FALSE, FALSE, TRUE);
		break;
	    }
	}

	/* If no object was made, we need to try another tval. */
	if (!obj_success) {
	    required_tval = get_choice();
	}

	/* Record origin */
	i_ptr->origin = ORIGIN_CHEST;
	i_ptr->origin_stage = o_ptr->origin_stage;
	i_ptr->origin_xtra = o_ptr->origin_xtra;

	/* If chest scatters its contents, pick any floor square. */
	if (scatter) {
	    for (i = 0; i < 200; i++) {
		/* Pick a totally random spot. */
		y = randint0(DUNGEON_HGT);
		x = randint0(DUNGEON_WID);

		/* Must be an empty floor. */
		if (!cave_clean_bold(y, x))
		    continue;

		/* Place the object there. */
		if (obj_success)
		    drop_near(i_ptr, -1, y, x, TRUE);

		/* Done. */
		break;
	    }
	}
	/* Normally, drop object near the chest. */
	else if (obj_success)
	    drop_near(i_ptr, -1, y, x, TRUE);
    }

    /* Clear this global variable, to avoid messing up object generation. */
    required_tval = 0;

    /* Reset the object level */
    object_level = p_ptr->depth;

    /* No longer opening a chest */
    opening_chest = FALSE;

    /* Empty */
    o_ptr->pval = 0;

    /* Known */
    object_known(o_ptr);
}


/**
 * Chests have traps too.  High-level chests can be very dangerous, no 
 * matter what level they are opened at.  Various traps added in Oangband. -LM-
 *
 * Exploding chest destroys contents (and traps).
 * Note that the chest itself is never destroyed.
 */
static void chest_trap(int y, int x, s16b o_idx)
{
    int i, trap, nasty_tricks_count;
    int j;

    object_type *o_ptr = &o_list[o_idx];

    /* Compensate for the averaging routine in the summon monster function. */
    int summon_level = o_ptr->pval + o_ptr->pval - p_ptr->depth;

    /* Ignore disarmed chests */
    if (o_ptr->pval <= 0)
	return;

    /* Obtain the traps */
    trap = chest_traps[o_ptr->pval];

    /* Lose strength */
    if (trap & (CHEST_LOSE_STR)) {
	msg("A small needle has pricked you!");
	take_hit(damroll(1, 4), "a poison needle");
	(void) do_dec_stat(A_STR);
    }

    /* Lose constitution */
    if (trap & (CHEST_LOSE_CON)) {
	msg("A small needle has pricked you!");
	take_hit(damroll(1, 4), "a poison needle");
	(void) do_dec_stat(A_CON);
    }

    /* Poison */
    if (trap & (CHEST_POISON)) {
	msg("A puff of green gas surrounds you!");
	pois_hit(25);
    }

    /* Paralyze */
    if (trap & (CHEST_PARALYZE)) {
	msg("A puff of yellow gas surrounds you!");
	if (!p_ptr->state.free_act) {
	    (void) inc_timed(TMD_PARALYZED, 10 + randint1(20), TRUE);
	} else
	    notice_obj(OF_FREE_ACT, 0);
    }

    /* Summon monsters */
    if (trap & (CHEST_SUMMON)) {
	int num = 2 + randint1(3);
	msg("You are enveloped in a cloud of smoke!");
	sound(MSG_SUM_MONSTER);
	for (i = 0; i < num; i++) {
	    (void) summon_specific(y, x, FALSE, summon_level, 0);
	}
    }

    /* Explode */
    if (trap & (CHEST_EXPLODE)) {
	msg("There is a sudden explosion!");
	msg("Everything inside the chest is destroyed!");
	o_ptr->pval = 0;
	take_hit(damroll(5, 8), "an exploding chest");
    }

    /* Scatter contents. */
    if (trap & (CHEST_SCATTER)) {
	if (stage_map[p_ptr->stage][STAGE_TYPE] >= CAVE)
	    msg
		("The contents of the chest scatter all over the dungeon!");
	else
	    msg("The contents of the chest scatter to the four winds!");
	chest_death(TRUE, y, x, o_idx);
	o_ptr->pval = 0;
    }

    /* Elemental summon. */
    if (trap & (CHEST_E_SUMMON)) {
	j = randint1(3) + 5;
	msg("Elemental beings appear to protect their treasures!");
	for (i = 0; i < j; i++) {
	    summon_specific(y, x, FALSE, summon_level, SUMMON_ELEMENTAL);
	}
    }

    /* Force clouds, then summon birds. */
    if (trap & (CHEST_BIRD_STORM)) {
	msg("A storm of birds swirls around you!");

	j = randint1(3) + 3;
	for (i = 0; i < j; i++)
	    (void) fire_meteor(0, GF_FORCE, y, x, o_ptr->pval / 5, 7, TRUE);

	j = randint1(5) + o_ptr->pval / 5;
	for (i = 0; i < j; i++) {
	    summon_specific(y, x, TRUE, summon_level, SUMMON_BIRD);
	}
    }

    /* Various colorful summonings. */
    if (trap & (CHEST_H_SUMMON)) {
	/* Summon demons. */
	if (randint0(4) == 0) {
	    msg("Demons materialize in clouds of fire and brimstone!");

	    j = randint1(3) + 2;
	    for (i = 0; i < j; i++) {
		(void) fire_meteor(0, GF_FIRE, y, x, 10, 5, TRUE);
		summon_specific(y, x, FALSE, summon_level, SUMMON_DEMON);
	    }
	}

	/* Summon dragons. */
	else if (randint0(3) == 0) {
	    msg("Draconic forms loom out of the darkness!");

	    j = randint1(3) + 2;
	    for (i = 0; i < j; i++) {
		summon_specific(y, x, FALSE, summon_level, SUMMON_DRAGON);
	    }
	}

	/* Summon hybrids. */
	else if (randint0(2) == 0) {
	    msg("Creatures strange and twisted assault you!");

	    j = randint1(5) + 3;
	    for (i = 0; i < j; i++) {
		summon_specific(y, x, FALSE, summon_level, SUMMON_HYBRID);
	    }
	}

	/* Summon vortices (scattered) */
	else {
	    msg("Vortices coalesce and wreak destruction!");

	    j = randint1(3) + 2;
	    for (i = 0; i < j; i++) {
		summon_specific(y, x, TRUE, summon_level, SUMMON_VORTEX);
	    }
	}
    }

    /* Dispel player. */
    if (trap & (CHEST_RUNES_OF_EVIL)) {
	/* Message. */
	msg("Hideous voices bid: 'Let the darkness have thee!'");

	/* Determine how many nasty tricks can be played. */
	nasty_tricks_count = 4 + randint0(3);

	/* This is gonna hurt... */
	for (; nasty_tricks_count > 0; nasty_tricks_count--) {
	    /* ...but a high saving throw does help a little. */
	    if (!check_save(2 * o_ptr->pval)) {
		if (randint0(6) == 0)
		    take_hit(damroll(5, 20), "a chest dispel-player trap");
		else if (randint0(5) == 0)
		    (void) inc_timed(TMD_CUT, 200, TRUE);
		else if (randint0(4) == 0) {
		    if (!p_ptr->state.free_act)
			(void) inc_timed(TMD_PARALYZED, 2 + randint0(6), TRUE);
		    else {
			(void) inc_timed(TMD_STUN, 10 + randint0(100), TRUE);
			notice_obj(OF_FREE_ACT, 0);
		    }

		} else if (randint0(3) == 0)
		    apply_disenchant(0);
		else if (randint0(2) == 0) {
		    (void) do_dec_stat(A_STR);
		    (void) do_dec_stat(A_DEX);
		    (void) do_dec_stat(A_CON);
		    (void) do_dec_stat(A_INT);
		    (void) do_dec_stat(A_WIS);
		    (void) do_dec_stat(A_CHR);
		} else
		    (void) fire_meteor(0, GF_NETHER, y, x, 150, 1, TRUE);
	    }
	}
    }
}


/**
 * Attempt to open the given chest at the given location
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_open_chest(int y, int x, s16b o_idx)
{
    int i, j;

    bool flag = TRUE;

    bool more = FALSE;

    object_type *o_ptr = &o_list[o_idx];


    /* Attempt to unlock it */
    if (o_ptr->pval > 0) {
	/* Assume locked, and thus not open */
	flag = FALSE;

	/* Get the "disarm" factor */
	i = p_ptr->state.skills[SKILL_DISARM];

	/* Penalize some conditions */
	if (p_ptr->timed[TMD_BLIND] || no_light())
	    i = i / 10;
	if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE])
	    i = i / 10;

	/* Difficulty rating.  Tweaked to compensate for higher pvals. */
	j = i - 2 * o_ptr->pval / 3;

	/* Always have a small chance of success */
	if (j < 2)
	    j = 2;

	/* Success -- May still have traps */
	if (randint0(100) < j) {
	    msgt(MSG_LOCKPICK, "You have picked the lock.");
	    gain_exp(o_ptr->pval);
	    flag = TRUE;
	}

	/* Failure -- Keep trying */
	else {
	    /* We may continue repeating */
	    more = TRUE;
	    flush();
	    msgt(MSG_LOCKPICK_FAIL, "You failed to pick the lock.");
	}
    }

    /* Allowed to open */
    if (flag) {
	/* Apply chest traps, if any */
	chest_trap(y, x, o_idx);

	/* Let the Chest drop items */
	chest_death(FALSE, y, x, o_idx);

	/* Squelch chest if autosquelch calls for it */
	p_ptr->notice |= PN_SQUELCH;

	/* Redraw chest, to be on the safe side (it may have been squelched) */
	light_spot(y, x);
    }

    /* Result */
    return (more);
}


/**
 * Attempt to disarm the chest at the given location
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_disarm_chest(int y, int x, s16b o_idx)
{
    int i, j;

    bool more = FALSE;

    object_type *o_ptr = &o_list[o_idx];


    /* Get the "disarm" factor */
    i = p_ptr->state.skills[SKILL_DISARM];

    /* Penalize some conditions */
    if (p_ptr->timed[TMD_BLIND] || no_light())
	i = i / 10;
    if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE])
	i = i / 10;

    /* Difficulty rating. */
    j = i - (5 + o_ptr->pval / 2);

    /* Always have a small chance of success */
    if (j < 2)
	j = 2;

    /* Must find the trap first. */
    if (!object_known_p(o_ptr)) {
	msg("I don't see any traps.");
    }

    /* Already disarmed/unlocked */
    else if (o_ptr->pval <= 0) {
	msg("The chest is not trapped.");
    }

    /* No traps to find. */
    else if (!chest_traps[o_ptr->pval]) {
	msg("The chest is not trapped.");
    }

    /* Success (get a fair amount of experience) */
    else if (randint0(100) < j) {
	msg("You have disarmed the chest.");
	gain_exp(o_ptr->pval * o_ptr->pval / 10);
	o_ptr->pval = (0 - o_ptr->pval);
    }

    /* Failure -- Keep trying */
    else if ((i > 5) && (randint1(i) > 5)) {
	/* We may keep trying */
	more = TRUE;
	flush();
	msg("You failed to disarm the chest.");
    }

    /* Failure -- Set off the trap */
    else {
	msg("You set off a trap!");
	chest_trap(y, x, o_idx);
    }

    /* Result */
    return (more);
}


/**
 * Return the number of terrain features with a given flag around (or under) 
 * the character
 */
int count_feats(int *y, int *x, int flag, bool under)
{
    int d;
    int xx, yy;
    int count;
    feature_type *f_ptr;

    /* Count how many matches */
    count = 0;

    /* Check around (and under) the character */
    for (d = 0; d < 9; d++) {
	/* Not searching under the character */
	if ((d == 8) && !under)
	    continue;

	/* Extract adjacent (legal) location */
	yy = p_ptr->py + ddy_ddd[d];
	xx = p_ptr->px + ddx_ddd[d];
	f_ptr = &f_info[cave_feat[yy][xx]];

	/* Paranoia */
	if (!in_bounds_fully(yy, xx))
	    continue;

	/* Must have knowledge */
	if (!cave_has(cave_info[yy][xx], CAVE_MARK))
	    continue;

	/* Not looking for this feature */
	if (!tf_has(f_ptr->flags, flag))
	    continue;

	/* Count it */
	++count;

	/* Remember the location of the last one found */
	*y = yy;
	*x = xx;
    }

    /* All done */
    return count;
}

/**
 * Return the number of visible traps around (or under) the character
 */
int count_traps(int *y, int *x)
{
    int d;
    int xx, yy;
    int count;

    /* Count how many matches */
    count = 0;

    /* Check around (and under) the character */
    for (d = 0; d < 9; d++) 
    {
	/* Extract adjacent (legal) location */
	yy = p_ptr->py + ddy_ddd[d];
	xx = p_ptr->px + ddx_ddd[d];

	/* Paranoia */
	if (!in_bounds_fully(yy, xx))
	    continue;

	/* Must have knowledge */
	if (!cave_has(cave_info[yy][xx], CAVE_MARK))
	    continue;

	/* No trap */
	if (!cave_visible_trap(yy, xx))
	    continue;

	/* Count it */
	++count;

	/* Remember the location of the last one found */
	*y = yy;
	*x = xx;
    }

    /* All done */
    return count;
}

/**
 * Return the number of chests around (or under) the character. -TNB-
 * If requested, count only trapped chests.
 */
int count_chests(int *y, int *x, bool trapped)
{
    int d, count, o_idx;

    object_type *o_ptr;

    /* Count how many matches */
    count = 0;

    /* Check around (and under) the character */
    for (d = 0; d < 9; d++) {
	/* Extract adjacent (legal) location */
	int yy = p_ptr->py + ddy_ddd[d];
	int xx = p_ptr->px + ddx_ddd[d];

	/* No (visible) chest is there */
	if ((o_idx = chest_check(yy, xx)) == 0)
	    continue;

	/* Grab the object */
	o_ptr = &o_list[o_idx];

	/* Already open */
	if (o_ptr->pval == 0)
	    continue;

	/* No (known) traps here */
	if (trapped && (!object_known_p(o_ptr) || (o_ptr->pval < 0)
			|| !chest_traps[o_ptr->pval]))
	    continue;

	/* OK */
	++count;

	/* Remember the location of the last chest found */
	*y = yy;
	*x = xx;
    }

    /* All done */
    return count;
}

/**
 * Convert an adjacent location to a direction.
 */
int coords_to_dir(int y, int x)
{
    return (motion_dir(p_ptr->py, p_ptr->px, y, x));
}


/**
 * Determine if a given grid may be "opened"
 */
static bool do_cmd_open_test(int y, int x)
{
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Must have knowledge */
    if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	/* Message */
	msg("You see nothing there.");

	/* Nope */
	return (FALSE);
    }

    /* Must be a closed door */
    if (!tf_has(f_ptr->flags, TF_DOOR_CLOSED)) 
    {
	/* Message */
	msgt(MSG_NOTHING_TO_OPEN, "You see nothing there to open.");

	/* Nope */
	return (FALSE);
    }

    /* Okay */
    return (TRUE);
}


/**
 * Perform the basic "open" command on doors
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
extern bool do_cmd_open_aux(int y, int x)
{
    int i, j;

    bool more = FALSE;
    feature_type *f_ptr = &f_info[cave_feat[y][x]];


    /* Verify legality */
    if (!do_cmd_open_test(y, x))
	return (FALSE);


    /* Jammed door */
    if (tf_has(f_ptr->flags, TF_DOOR_JAMMED))
    {
	/* Stuck */
	msg("The door appears to be stuck.");
    }

    /* Locked door */
    else if (tf_has(f_ptr->flags, TF_DOOR_LOCKED)) 
    {
	/* Disarm factor */
	i = p_ptr->state.skills[SKILL_DISARM];

	/* Penalize some conditions */
	if (p_ptr->timed[TMD_BLIND] || no_light())
	    i = i / 10;
	if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE])
	    i = i / 10;

	/* Extract the lock power */
	j = f_ptr->locked;

	/* Extract the difficulty XXX XXX XXX */
	j = i - (j * 4);

	/* Always have a small chance of success */
	if (j < 2)
	    j = 2;

	/* Success */
	if (randint0(100) < j) {
	    /* Message */
	    msgt(MSG_LOCKPICK, "You have picked the lock.");

	    /* Open the door */
	    cave_set_feat(y, x, FEAT_OPEN);

	    /* Update the visuals */
	    p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	    /* Experience */
	    gain_exp(1);
	}

	/* Failure */
	else {
	    /* Failure */
	    flush();

	    /* Message */
	    msgt(MSG_LOCKPICK_FAIL, "You failed to pick the lock.");

	    /* We may keep trying */
	    more = TRUE;
	}
    }

    /* Closed door */
    else {
	/* Open the door */
	cave_set_feat(y, x, FEAT_OPEN);

	/* Update the visuals */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Sound */
	sound(MSG_OPENDOOR);
    }

    /* Result */
    return (more);
}



/**
 * Open a closed/locked/jammed door or a closed/locked chest.
 *
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(cmd_code code, cmd_arg args[])
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int y, x, dir;

    s16b o_idx;

    bool more = FALSE;

    dir = args[0].direction;

    /* Get location */
    y = py + ddy[dir];
    x = px + ddx[dir];

    /* Check for chests */
    o_idx = chest_check(y, x);

    /* Verify legality */
    if (!o_idx && !do_cmd_open_test(y, x))
    {
	/* Cancel repeat */
	disturb(0, 0);
	return;
    }

    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Apply confusion */
    if (confuse_dir(&dir)) {
	/* Get location */
	y = py + ddy[dir];
	x = px + ddx[dir];

	/* Check for chest */
	o_idx = chest_check(y, x);
    }


    /* Monster */
    if (cave_m_idx[y][x] > 0) {
	/* Message */
	msg("There is a monster in the way!");

	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* Chest */
    if (o_idx) {
	/* Open the chest */
	more = do_cmd_open_chest(y, x, o_idx);
    }

    /* Door */
    else {
	/* Open the door */
	more = do_cmd_open_aux(y, x);
    }

    /* Cancel repeat unless we may continue */
    if (!more)
	disturb(0, 0);
}


/**
 * Determine if a given grid may be "closed"
 */
static bool do_cmd_close_test(int y, int x)
{
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Must have knowledge */
    if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	/* Message */
	msg("You see nothing there.");

	/* Nope */
	return (FALSE);
    }

    /* Require open/broken door */
    if (!tf_has(f_ptr->flags, TF_CLOSABLE))
    {
	/* Message */
	msg("You see nothing there to close.");

	/* Nope */
	return (FALSE);
    }

    /* Monster */
    if (cave_m_idx[y][x] > 0) {
	/* Message */
	msg("There is a monster in the way!");

	/* Nope */
	return (FALSE);
    }

    /* Okay */
    return (TRUE);
}


/**
 * Perform the basic "close" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_close_aux(int y, int x)
{
    bool more = FALSE;


    /* Verify legality */
    if (!do_cmd_close_test(y, x))
	return (FALSE);


    /* Broken door */
    if (cave_feat[y][x] == FEAT_BROKEN) {
	/* Message */
	msg("The door appears to be broken.");
    }

    /* Open door */
    else {
	/* Close the door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);

	/* Update the visuals */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Sound */
	sound(MSG_SHUTDOOR);
    }

    /* Result */
    return (more);
}


/**
 * Close an open door.
 */
void do_cmd_close(cmd_code code, cmd_arg args[])
{
    int y, x, dir;

    bool more = FALSE;

    dir = args[0].direction;

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];


    /* Verify legality */
    if (!do_cmd_close_test(y, x))
	return;


    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Apply confusion */
    if (confuse_dir(&dir)) {
	/* Get location */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];
    }


    /* Close door */
    more = do_cmd_close_aux(y, x);

    /* Cancel repeat unless told not to */
    if (!more)
	disturb(0, 0);
}



/**
 * Determine if a given grid may be "tunneled"
 */
static bool do_cmd_tunnel_test(int y, int x)
{
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Must have knowledge */
    if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	/* Message */
	msg("You see nothing there.");

	/* Nope */
	return (FALSE);
    }

    /* Must be a wall/door/etc */
    if (tf_has(f_ptr->flags, TF_ROCK) || tf_has(f_ptr->flags, TF_DOOR_CLOSED)) 
    {
	/* Okay */
	return (TRUE);
    }

    /* Message */
    msg("You see nothing there to tunnel.");

    /* Nope */
    return (FALSE);
}


/**
 * Tunnel through wall.   Assumes valid location.
 *
 * Note that it is impossible to "extend" rooms past their
 * outer walls (which are actually part of the room).
 *
 * Attempting to do so will produce floor grids which are not part
 * of the room, and whose "illumination" status do not change with
 * the rest of the room.
 */
static bool twall(int y, int x)
{
    /* Sound */
    sound(MSG_DIG);

    /* Forget the wall */
    cave_off(cave_info[y][x], CAVE_MARK);

    /* Remove the feature */
    if (outside)
	cave_set_feat(y, x, FEAT_ROAD);
    else
	cave_set_feat(y, x, FEAT_FLOOR);

    /* Update the visuals */
    p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

    /* Result */
    return (TRUE);
}


/**
 * Perform the basic "tunnel" command
 *
 * Assumes that no monster is blocking the destination
 *
 * Uses "twall" (above) to do all "terrain feature changing".
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_tunnel_aux(int y, int x)
{
    bool more = FALSE;
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Verify legality */
    if (!do_cmd_tunnel_test(y, x))
	return (FALSE);


    /* Sound XXX XXX XXX */
    /* sound(MSG_DIG); */

    /* All rock and secret doors */
    if (tf_has(f_ptr->flags, TF_ROCK)) {
	/* Titanium */
	if (tf_has(f_ptr->flags, TF_PERMANENT)) {
	    msg("This seems to be permanent rock.");
	}

	/* Granite */
	else if (tf_has(f_ptr->flags, TF_GRANITE)
		 && !tf_has(f_ptr->flags, TF_DOOR_ANY)) {
	    /* Tunnel */
	    if ((p_ptr->state.skills[SKILL_DIGGING] > 40 + randint0(1600))
		&& twall(y, x)) {
		msg("You have finished the tunnel.");
	    }

	    /* Keep trying */
	    else {
		/* We may continue tunelling */
		msg("You tunnel into the granite wall.");
		more = TRUE;
	    }
	}

	/* Quartz / Magma */
	else if (!tf_has(f_ptr->flags, TF_GRANITE)
		 && tf_has(f_ptr->flags, TF_WALL)) {
	    bool okay = FALSE;

	    /* Quartz */
	    if (tf_has(f_ptr->flags, TF_QUARTZ)) 
	    {
		okay = (p_ptr->state.skills[SKILL_DIGGING] > 
			20 + randint0(800));
	    }

	    /* Magma */
	    else 
	    {
		okay = (p_ptr->state.skills[SKILL_DIGGING] > 
			10 + randint0(400));
	    }

	    /* Success */
	    if (okay && twall(y, x)) {
		/* Found treasure */
		if (tf_has(f_ptr->flags, TF_GOLD)) {
		    /* Place some gold */
		    place_gold(y, x);

		    /* Message */
		    msg("You have found something!");
		}

		/* Found nothing */
		else {
		    /* Message */
		    msg("You have finished the tunnel.");
		}
	    }

	    /* Failure (quartz) */
	    else if (tf_has(f_ptr->flags, TF_QUARTZ)) {
		/* Message, continue digging */
		msg("You tunnel into the quartz vein.");
		more = TRUE;
	    }

	    /* Failure (magma) */
	    else {
		/* Message, continue digging */
		msg("You tunnel into the magma vein.");
		more = TRUE;
	    }
	}

	/* Rubble */
	else if (cave_feat[y][x] == FEAT_RUBBLE) {
	    /* Remove the rubble */
	    if ((p_ptr->state.skills[SKILL_DIGGING] > randint0(200))
		&& twall(y, x)) {
		/* Message */
		msg("You have removed the rubble.");

		/* Hack -- place an object */
		if (randint0(100) == 0) {
		    /* Create a simple object */
		    place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_RUBBLE);

		    /* Observe new object */
		    if (!squelch_hide_item(&o_list[cave_o_idx[y][x]])
			&& player_can_see_bold(y, x)) {
			msg("You have found something!");
		    }
		}
	    }

	    else {
		/* Message, keep digging */
		msg("You dig in the rubble.");
		more = TRUE;
	    }
	}

	/* Secret doors */
	else if (cave_feat[y][x] == FEAT_SECRET) {
	    /* Tunnel */
	    if ((p_ptr->state.skills[SKILL_DIGGING] > 30 + randint0(1200))
		&& twall(y, x)) {
		msg("You have finished the tunnel.");
	    }

	    /* Keep trying */
	    else {
		/* We may continue tunelling */
		msg("You tunnel into the granite wall.");
		more = TRUE;

		/* Occasional Search XXX XXX */
		if (randint0(100) < 25)
		    (void) search(FALSE);
	    }
	}
    }
    /* Doors */
    else {
	/* Tunnel */
	if ((p_ptr->state.skills[SKILL_DIGGING] > 30 + randint0(1200))
	    && twall(y, x)) {
	    msg("You have finished the tunnel.");
	}

	/* Keep trying */
	else {
	    /* We may continue tunelling */
	    msg("You tunnel into the door.");
	    more = TRUE;
	}
    }

    /* Result */
    return (more);
}


/**
 * Tunnel through "walls" (including rubble and secret doors)
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 */
void do_cmd_tunnel(cmd_code code, cmd_arg args[])
{
    int y, x, dir;

    bool more = FALSE;

    dir = args[0].direction;

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];

    /* Oops */
    if (!do_cmd_tunnel_test(y, x))
	return;


    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Apply confusion */
    if (confuse_dir(&dir)) {
	/* Get location */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];
    }

    /* Monster */
    if (cave_m_idx[y][x] > 0) {
	/* Message */
	msg("There is a monster in the way!");

	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* Tunnel through walls */
    more = do_cmd_tunnel_aux(y, x);

    /* Cancel repetition unless we can continue */
    if (!more)
	disturb(0, 0);
}


/**
 * Determine if a given grid may be "disarmed"
 */
static bool do_cmd_disarm_test(int y, int x)
{
    /* Must have knowledge */
    if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	/* Message */
	msg("You see nothing there.");

	/* Nope */
	return (FALSE);
    }

    /* Require an actual trap, web or glyph */
    if (!cave_visible_trap(y, x)) 
    {
	/* Message */
	msg("You see nothing there to disarm.");

	/* Nope */
	return (FALSE);
    }

    /* Okay */
    return (TRUE);
}

/**
 * Perform the basic "disarm" command on a trap or glyph.
 *
 * Assume there is no monster blocking the destination (tested by 
 * do_cmd_disarm).  Traps now have level-dependent power.
 * Decrement Rogue traps and glyphs of warding. -LM-
 *
 * Returns TRUE if repeated commands may continue
 */
extern bool do_cmd_disarm_aux(int y, int x)
{
    int i, j, power, idx;

    bool more = FALSE;

    trap_type *t_ptr;

    /* Verify legality */
    if (!do_cmd_disarm_test(y, x))
	return (FALSE);

    /* Choose trap */
    if (!get_trap(y, x, &idx)) return (FALSE);
    t_ptr = &trap_list[idx];

    /* Get the "disarm" factor */
    i = p_ptr->state.skills[SKILL_DISARM];

    /* Penalize some conditions */
    if (p_ptr->timed[TMD_BLIND] || no_light())
	i = i / 10;
    if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE])
	i = i / 10;


    /* Extract trap "power". */
    power = 5 + p_ptr->depth / 4;

    /* Only player traps grant exp. */
    if (!trf_has(t_ptr->flags, TRF_TRAP))
	power = 0;

    /* Extract the disarm probability */
    j = i - power;

    /* Always have a small chance of success */
    if (j < 2)
	j = 2;

    /* Success */
    if ((power == 0) || (randint0(100) < j)) 
    {
	/* Remove the trap */
	(void) remove_trap(y, x, TRUE, idx);

	/* Reward */
	gain_exp(power);
    }

    /* Failure -- Keep trying */
    else if ((i > 5) && (randint1(i) > 5)) 
    {
	/* Failure */
	flush();

	/* Message */
	msg("You failed to disarm the %s.", t_ptr->kind->name);

	/* We may keep trying */
	more = TRUE;
    }

    /* Failure -- Set off the trap */
    else 
    {
	/* Message */
	msg("You set off the %s!", t_ptr->kind->name);

	/* Hit the trap */
	hit_trap(y, x);
    }

    /* Result */
    return (more);
}


/**
 * Disarms a trap, a glyph, or a chest.
 */
void do_cmd_disarm(cmd_code code, cmd_arg args[])
{
    int y, x, dir;

    s16b o_idx;

    bool more = FALSE;

    dir = args[0].direction;

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];

    /* Check for chests */
    o_idx = chest_check(y, x);


    /* Verify legality */
    if (!o_idx && !do_cmd_disarm_test(y, x))
    {
	/* Cancel repeat */
	disturb(0, 0);
	return;
    }

    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Apply confusion */
    if (confuse_dir(&dir)) 
    {
	/* Get location */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];

	/* Check for chests */
	o_idx = chest_check(y, x);
    }

    /* Monster */
    if (cave_m_idx[y][x] > 0) 
    {
	/* Message */
	msg("There is a monster in the way!");

	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* Chest */
    if (o_idx) 
    {
	/* Disarm the chest */
	more = do_cmd_disarm_chest(y, x, o_idx);
    }

    /* Disarm trap */
    else 
    {
	/* Disarm the trap */
	more = do_cmd_disarm_aux(y, x);
    }

    /* Cancel repeat unless told not to */
    if (!more)
	disturb(0, 0);
}


/**
 * Determine if a given grid may be "bashed"
 */
static bool do_cmd_bash_test(int y, int x)
{
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Must have knowledge */
    if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	/* Message */
	msg("You see nothing there.");

	/* Nope */
	return (FALSE);
    }

    /* Require a door */
    if (!tf_has(f_ptr->flags, TF_DOOR_CLOSED))
    {
	/* Message */
	msg("You see nothing there to bash.");

	/* Nope */
	return (FALSE);
    }

    /* Okay */
    return (TRUE);
}


/**
 * Perform the basic "bash" command
 *
 * Assume there is no monster blocking the destination
 *
 * Returns TRUE if repeated commands may continue
 */
static bool do_cmd_bash_aux(int y, int x)
{
    int bash, temp;

    bool more = FALSE;
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Verify legality */
    if (!do_cmd_bash_test(y, x))
	return (FALSE);


    /* Message */
    msg("You smash into the door!");

    /* Make a lot of noise. */
    add_wakeup_chance = 9000;

    /* Hack -- Bash power based on strength */
    /* (Ranges from 14 to 40 to 90 to 110) */
    bash = 10 + adj_str_hold[p_ptr->state.stat_ind[A_STR]];

    /* Extract door power */
    if (tf_has(f_ptr->flags, TF_DOOR_LOCKED))
	temp = f_ptr->locked;
    else
	temp = f_ptr->jammed;

    /* Compare bash power to door power XXX XXX XXX */
    temp = (bash - (temp * 8));

    /* Hack -- always have a chance */
    if (temp < 1)
	temp = 1;

    /* Hack -- attempt to bash down the door */
    if (randint0(100) < temp) {
	/* Break down the door */
	if (randint0(100) < 50) {
	    cave_set_feat(y, x, FEAT_BROKEN);
	}

	/* Open the door */
	else {
	    cave_set_feat(y, x, FEAT_OPEN);
	}

	/* Message */
	msgt(MSG_OPENDOOR, "The door crashes open!");

	/* Update the visuals */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
    }

    /* Saving throw against stun */
    else if (randint0(100) < adj_dex_safe[p_ptr->state.stat_ind[A_DEX]] + 
	     p_ptr->lev) 
    {
	/* Message */
	msg("The door holds firm.");

	/* Allow repeated bashing */
	more = TRUE;
    }

    /* High dexterity yields coolness */
    else {
	/* Message */
	msg("You are off-balance.");

	/* Hack -- Lose balance ala paralysis */
	(void) inc_timed(TMD_PARALYZED, 2 + randint0(2), TRUE);
    }

    /* Result */
    return (more);
}


/**
 * Bash open a door, success based on character strength
 *
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Creatures can also open or bash doors, see elsewhere.
 */
void do_cmd_bash(cmd_code code, cmd_arg args[])
{
    int y, x, dir;
    bool more = FALSE;

    dir = args[0].direction;

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];


    /* Verify legality */
    if (!do_cmd_bash_test(y, x))
    {
	/* Cancel repeat */
	disturb(0, 0);
	return;
    }


    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Apply confusion */
    if (confuse_dir(&dir)) {
	/* Get location */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];
    }


    /* Monster */
    if (cave_m_idx[y][x] > 0) 
    {
	/* Message */
	msg("There is a monster in the way!");
	
	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* Door */
    else
    {
	/* Bash the door */
	more = do_cmd_bash_aux(y, x);
    }

    /* Cancel repeat unless told not to */
    if (!more)
	disturb(0, 0);
}



/**
 * Manipulate an adjacent grid in some way
 *
 * Attack monsters, tunnel through walls, disarm traps, open doors, 
 * or, for rogues, set traps and steal money.
 *
 * This command must always take energy, to prevent free detection
 * of invisible monsters.
 *
 * The "semantics" of this command must be chosen before the player
 * is confused, and it must be verified against the new grid.
 */
void do_cmd_alter_aux(int dir)
{
    int y, x;

    int feat;

    bool more = FALSE;

    monster_type *m_ptr;
    feature_type *f_ptr;

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];


    /* Original feature */
    feat = cave_feat[y][x];
    f_ptr = &f_info[feat];

    /* Must have knowledge to know feature XXX XXX */
    if (!cave_has(cave_info[y][x], CAVE_MARK))
	feat = FEAT_NONE;


    /* Apply confusion */
    if (confuse_dir(&dir)) {
	/* Get location */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];
    }

    /* If a monster is present, and visible, Rogues may steal from it.
     * Otherwise, the player will simply attack. -LM- */
    if (cave_m_idx[y][x] > 0) {
	if ((player_has(PF_STEAL)) && (!SCHANGE)) {
	    m_ptr = &m_list[cave_m_idx[y][x]];
	    if (m_ptr->ml)
		py_steal(y, x);
	    else
		(void) py_attack(y, x, FALSE);
	} else
	    (void) py_attack(y, x, FALSE);
    }

    /* Some players can set traps.  Total number is checked in py_set_trap. */
    else if (player_has(PF_TRAP) && tf_has(f_ptr->flags, TF_MTRAP) && 
	     !cave_monster_trap(y, x)) 
    {
	/* Make sure not to repeat */
	cmd_set_repeat(0);

	if (!py_set_trap(y, x))
	    return;
    }

    /* Disarm advanced monster traps */
    else if (cave_advanced_monster_trap(y, x)) 
    {
	/* Disarm */
	more = do_cmd_disarm_aux(y, x);
    }
    
    /* Disarm advanced monster traps */
    else if (cave_basic_monster_trap(y, x)) 
    {
	/* Modify */
	if (!py_modify_trap(y, x))
	    return;
    }
    
    /* Tunnel through walls */
    else if (tf_has(f_ptr->flags, TF_ROCK)) 
    {
	/* Tunnel */
	more = do_cmd_tunnel_aux(y, x);
    }

    /* Bash jammed doors */
    else if (tf_has(f_ptr->flags, TF_DOOR_JAMMED)) 
    {
	/* Bash */
	more = do_cmd_bash_aux(y, x);
    }

    /* Open closed doors */
    else if (tf_has(f_ptr->flags, TF_DOOR_CLOSED)) 
    {
	/* Close */
	more = do_cmd_open_aux(y, x);
    }

    /* Disarm traps */
    else if (cave_has(cave_info[y][x], CAVE_TRAP)) 
    {
	/* Disarm */
	more = do_cmd_disarm_aux(y, x);
    }

    /* Oops */
    else 
    {
	/* Oops */
	return;
    }

    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Cancel repetition unless we can continue */
    if (!more)
	disturb(0, 0);
}

void do_cmd_alter(cmd_code code, cmd_arg args[])
{
	do_cmd_alter_aux(args[0].direction);
}


/**
 * Find the index of some "spikes", if possible.
 *
 * XXX XXX XXX Let user choose a pile of spikes, perhaps?
 */
static bool get_spike(int *ip)
{
    int i;

    /* Check every item in the pack */
    for (i = 0; i < INVEN_PACK; i++) {
	object_type *o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Check the "tval" code */
	if (o_ptr->tval == TV_SPIKE) {
	    /* Save the spike index */
	    (*ip) = i;

	    /* Success */
	    return (TRUE);
	}
    }

    /* Oops */
    return (FALSE);
}


/**
 * Determine if a given grid may be "spiked"
 */
bool do_cmd_spike_test(int y, int x)
{
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Must have knowledge */
    if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	/* Message */
	msg("You see nothing there.");

	/* Nope */
	return (FALSE);
    }

    /* Require a door */
    if (!tf_has(f_ptr->flags, TF_DOOR_CLOSED)) 
    {
	/* Message */
	msg("You see nothing there to spike.");

	/* Nope */
	return (FALSE);
    }

    /* Okay */
    return (TRUE);
}


/**
 * Jam a closed door with a spike.  Now takes only 4/10ths normal energy
 * if no monster is in the way. -LM-
 *
 * This command may NOT be repeated
 */
void do_cmd_spike(cmd_code code, cmd_arg args[])
{
    int y, x, dir, item = 0;
    feature_type *f_ptr;

    dir = args[0].direction;

    /* Get a spike */
    if (!get_spike(&item)) {
	/* Message */
	msg("You have no spikes!");

	/* Done */
	return;
    }

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];
    f_ptr = &f_info[cave_feat[y][x]];

    /* Verify legality */
    if (!do_cmd_spike_test(y, x))
	return;


    /* Take a partial turn.  Now jamming is more useful. */
    p_ptr->energy_use = 40;

    /* Confuse direction */
    if (confuse_dir(&dir)) 
    {
	/* Get location */
	y = p_ptr->py + ddy[dir];
	x = p_ptr->px + ddx[dir];
	f_ptr = &f_info[cave_feat[y][x]];
    }


    /* Monster.  Make the action now take a full turn */
    if (cave_m_idx[y][x] > 0) 
    {
	/* Message */
	msg("There is a monster in the way!");

	p_ptr->energy_use += 60;

	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* Go for it */
    else 
    {
	/* Verify legality */
	if (!do_cmd_spike_test(y, x))
	    return;

	/* Successful jamming */
	msg("You jam the door with a spike.");

	/* Convert "locked" to "stuck" XXX XXX XXX */
	if (!tf_has(f_ptr->flags, TF_DOOR_JAMMED)) 
	{
	    cave_feat[y][x] += 0x08;
	}

	/* Add one spike to the door */
	if (cave_feat[y][x] != FEAT_DOOR_TAIL) 
	{
	    cave_feat[y][x] += 0x01;
	}

	/* Use up, and describe, a single spike, from the bottom */
	inven_item_increase(item, -1);
	inven_item_describe(item);
	inven_item_optimize(item);
    }
}



/**
 * Determine if a given grid may be "walked"
 */
static bool do_cmd_walk_test(int y, int x)
{
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Assume no monster. */
    monster_type *m_ptr = 0;

    /* Access the monster, if any is present. */
    if (cave_m_idx[y][x] != 0)
	m_ptr = &m_list[cave_m_idx[y][x]];

    /* If a monster can be seen, it can be attacked normally.  Code in cmd1.c
     * controls whether a player can actually move to the destination grid. */
    if ((m_ptr) && (m_ptr->ml))
	return (TRUE);


    /* Hack -- walking obtains knowledge XXX XXX */
    if (!cave_has(cave_info[y][x], CAVE_MARK))
	return (TRUE);

    /* Check for being stuck in a web */
    if (cave_web(p_ptr->py, p_ptr->px))
    {
	msg("You are stuck!");
	return (FALSE);
    }

    /* Require open space */
    if (!tf_has(f_ptr->flags, TF_PASSABLE)) 
    {
	/* Door */
	if (tf_has(f_ptr->flags, TF_DOOR_CLOSED))
	{
	    /* If OPT(easy_alter)_door option is on, doors are legal. */
	    if (OPT(easy_alter))
		return (TRUE);

	    /* Otherwise, let the player know of the door. */
	    else
		msg("There is a door in the way!");
	}

	/* Wall */
	else {
	    /* Message */
	    msg("Your way is blocked!");
	}

	/* Cancel repeat */
	disturb(0, 0);

	/* Nope */
	return (FALSE);
    }

    /* Okay */
    return (TRUE);
}


/**
 * Walk into a grid (pick up objects as set by the auto-pickup option)
 */
void do_cmd_walk(cmd_code code, cmd_arg args[])
{
    int y, x, dir;

    dir = args[0].direction;

    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];


    /* Verify legality */
    if (!do_cmd_walk_test(y, x))
	return;


    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Confuse direction */
    if (confuse_dir(&dir)) 
    {
	/* Get location */
	y = p_ptr-> py + ddy[dir];
	x = p_ptr->px + ddx[dir];
    }


    /* Verify legality */
    if (!do_cmd_walk_test(y, x))
	return;

    /* Move the player */
    move_player(dir);
}


/**
 * Jump into a grid (flip pickup mode)
 */
void do_cmd_jump(cmd_code code, cmd_arg args[])
{
    bool old_easy_alter;

    /* OPT(easy_alter) can be turned off (don't disarm traps) */
    old_easy_alter = OPT(easy_alter);
    OPT(easy_alter) = FALSE;

    do_cmd_walk(code, args);

    /* Restore OPT(easy_alter) */
    OPT(easy_alter) = old_easy_alter;
}


/**
 * Start running.
 *
 * Note that running while confused is not allowed.
 */
void do_cmd_run(cmd_code code, cmd_arg args[])
{
    int y, x, dir;

    dir = args[0].direction;

    /* Hack XXX XXX XXX */
    if (p_ptr->timed[TMD_CONFUSED]) {
	msg("You are too confused!");
	return;
    }


    /* Get location */
    y = p_ptr->py + ddy[dir];
    x = p_ptr->px + ddx[dir];


    /* Verify legality */
    if (!do_cmd_walk_test(y, x))
	return;


    /* Start run */
    run_step(dir);
}



/**
 * Start running with pathfinder.
 *
 * Note that running while confused is not allowed.
 */
void do_cmd_pathfind(cmd_code code, cmd_arg args[])
{
    /* Hack XXX XXX XXX */
    if (p_ptr->timed[TMD_CONFUSED]) {
	msg("You are too confused!");
	return;
    }

    /* Hack -- handle stuck players */
    if (cave_web(p_ptr->py, p_ptr->px)) 
    {
	/* Tell the player */
	msg("You are stuck!");

	return;
    }

    if (findpath(args[0].point.x, args[0].point.y)) {
	p_ptr->running = 1000;
	/* Calculate torch radius */
	p_ptr->update |= (PU_TORCH);
	p_ptr->running_withpathfind = TRUE;
	run_step(0);
    }
}


/**
 * Stay still.  Search.   Enter stores.
 * Pick up treasure and objects if "pickup" is true.
 */
static void do_cmd_hold_or_stay(int pickup)
{
    feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];

    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Spontaneous Searching */
    if (p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] >= 50) 
    {
	(void) search(FALSE);
    } 
    else if (0 == randint0(50 - p_ptr->state.skills[SKILL_SEARCH_FREQUENCY])) 
    {
	(void) search(FALSE);
    }

    /* Continuous Searching */
    if (p_ptr->searching) 
    {
	(void) search(FALSE);
    }

    /* Handle "objects".  Do not charge extra energy for objects picked up. */
    (void) py_pickup(pickup, p_ptr->py, p_ptr->px);

    /* Hack -- enter a store if we are on one */
    if (tf_has(f_ptr->flags, TF_SHOP))
    {
	/* Disturb */
	disturb(0, 0);

	cmd_insert(CMD_ENTER_STORE);

	/* Free turn XXX XXX XXX */
	p_ptr->energy_use = 0;
    }
}


/**
 * Hold still (usually pickup)
 */
void do_cmd_hold(cmd_code code, cmd_arg args[])
{
    /* Hold still (usually pickup) */
    do_cmd_hold_or_stay(OPT(pickup_always));
}


/**
 * Pick up objects on the floor beneath you. -LM-
 */
void do_cmd_pickup(cmd_code code, cmd_arg args[])
{
    int energy_cost;

    /* Pick up floor objects, forcing a menu for multiple objects. */
    energy_cost = py_pickup(1, p_ptr->py, p_ptr->px) * 10;

    /* Maximum time expenditure is a full turn. */
    if (energy_cost > 100)
	energy_cost = 100;

    /* Charge this amount of energy. */
    p_ptr->energy_use = energy_cost;
}


/*
 * Rest (restores hit points and mana and such)
 */
void do_cmd_rest(cmd_code code, cmd_arg args[])
{
    /* 
     * A little sanity checking on the input - only the specified negative 
     * values are valid. 
     */
    if ((args[0].choice < 0)
	&& ((args[0].choice != REST_COMPLETE)
	    && (args[0].choice != REST_ALL_POINTS)
	    && (args[0].choice != REST_SUNLIGHT))) {
	return;
    }

    /* Save the rest code */
    p_ptr->resting = args[0].choice;

    /* Truncate overlarge values */
    if (p_ptr->resting > 9999)
	p_ptr->resting = 9999;

    /* Take a turn XXX XXX XXX (?) */
    p_ptr->energy_use = 100;

    /* Cancel searching */
    p_ptr->searching = FALSE;

    /* Recalculate bonuses */
    p_ptr->update |= (PU_BONUS);

    /* Redraw the state */
    p_ptr->redraw |= (PR_STATE);

    /* Handle stuff */
    handle_stuff(p_ptr);

    /* Refresh XXX XXX XXX */
    Term_fresh();
}


void textui_cmd_rest(void)
{
    /* Prompt for time if needed */
    if (p_ptr->command_arg <= 0) {
	const char *p =
	    "Rest (0-9999, '*' for HP and SP, '&' as needed, '$' until sunrise/set): ";

	char out_val[5] = "& ";

	/* Ask for duration */
	if (!get_string(p, out_val, sizeof(out_val)))
	    return;

	/* Rest until done */
	if (out_val[0] == '&') {
	    cmd_insert(CMD_REST);
	    cmd_set_arg_choice(cmd_get_top(), 0, REST_COMPLETE);
	}

	/* Rest a lot */
	else if (out_val[0] == '*') {
	    cmd_insert(CMD_REST);
	    cmd_set_arg_choice(cmd_get_top(), 0, REST_ALL_POINTS);
	}

	/* Rest until sunrise/sunset */
	else if (out_val[0] == '$') {
	    cmd_insert(CMD_REST);
	    cmd_set_arg_choice(cmd_get_top(), 0, REST_SUNLIGHT);
	}

	/* Rest some */
	else {
	    int turns = atoi(out_val);
	    if (turns <= 0)
		return;
	    if (turns > 9999)
		turns = 9999;

	    cmd_insert(CMD_REST);
	    cmd_set_arg_choice(cmd_get_top(), 0, turns);
	}
    }
}


/**
 * Hack -- commit suicide
 */
void do_cmd_suicide(cmd_code code, cmd_arg args[])
{
    /* Commit suicide */
    p_ptr->is_dead = TRUE;

    /* Stop playing */
    p_ptr->playing = FALSE;

    /* Leaving */
    p_ptr->leaving = TRUE;

    /* Cause of death */
    my_strcpy(p_ptr->died_from, "Quitting", sizeof(p_ptr->died_from));
}


void textui_cmd_suicide(void)
{
    /* Flush input */
    flush();

    /* Verify Retirement */
    if (p_ptr->total_winner) {
	/* Verify */
	if (!get_check("Do you want to retire? "))
	    return;
    }

    /* Verify Suicide */
    else {
	struct keypress ch;

	/* Verify */
	if (!get_check("Do you really want to commit suicide? "))
	    return;

	/* Special Verification for suicide */
	prt("Please verify SUICIDE by typing the '@' sign: ", 0, 0);
	flush();
	ch = inkey();
	prt("", 0, 0);
	if (ch.code != '@')
	    return;
    }

    cmd_insert(CMD_SUICIDE);
}

void do_cmd_save_game(cmd_code code, cmd_arg args[])
{
    save_game();
}
