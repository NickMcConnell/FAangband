/** \file cmd1.c
    \brief Commands, part 1

 *  Searching, pickup, effects of traps, move one square (including special 
 * terrain effects), and the running algorithm.   
 *
 * Tim Baker's easy patch installed.
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
#include "cmds.h"
#include "generate.h"
#include "history.h"
#include "monster.h"
#include "tvalsval.h"
#include "object.h"
#include "spells.h"
#include "squelch.h"
#include "trap.h"

/**
 * Search for hidden things
 */
bool search(bool verbose)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int y, x, chance;

    bool found = FALSE;

    object_type *o_ptr;

    /* Start with base search ability */
    chance = p_ptr->state.skills[SKILL_SEARCH];

    /* Penalize various conditions */
    if (p_ptr->timed[TMD_BLIND] || no_light())
	chance = chance / 10;
    if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_IMAGE])
	chance = chance / 10;

    /* Prevent fruitless searches */
    if (chance <= 0)
    {
	if (verbose)
	{
	    msg("You can't make out your surroundings well enough to search.");

	    /* Cancel repeat */
	    disturb(0, 0);
	}

	return FALSE;
    }

    /* Search the nearby grids, which are always in bounds */
    for (y = (py - 1); y <= (py + 1); y++) 
    {
	for (x = (px - 1); x <= (px + 1); x++) 
	{
	    /* Sometimes, notice things */
	    if (randint0(100) < chance) 
	    {
		/* Invisible trap */
		if (cave_invisible_trap(y, x)) 
		{
		    found = TRUE;

		    /* Reveal one or more traps, display a message */
		    if (reveal_trap(y, x, chance, TRUE))
		    {
			/* Disturb */
			disturb(0, 0);
		    }
		}

		/* Secret door */
		if (cave_feat[y][x] == FEAT_SECRET) 
		{
		    found = TRUE;

		    /* Message */
		    msg("You have found a secret door.");

		    /* Pick a door */
		    place_closed_door(y, x);

		    /* Disturb */
		    disturb(0, 0);
		}

		/* Scan all objects in the grid */
		for (o_ptr = get_first_object(y, x); o_ptr; 
		     o_ptr = get_next_object(o_ptr)) 
		{
		    /* Skip non-chests */
		    if (o_ptr->tval != TV_CHEST)
			continue;

		    /* Skip disarmed chests */
		    if (o_ptr->pval <= 0) continue;

		    /* Skip non-trapped chests */
		    if (!chest_traps[o_ptr->pval])
			continue;

		    /* Identify once */
		    if (!object_known_p(o_ptr)) {
			found = TRUE;

			/* Message */
			msg("You have discovered a trap on the chest!");

			/* Know the trap */
			object_known(o_ptr);

			/* Notice it */
			disturb(0, 0);
		    }
		}
	    }
	}
    }
    if (verbose && !found)
    {
	if (chance >= 100)
	    msg("There are no secrets here.");
	else
	    msg("You found nothing.");
    }

    return TRUE;
}


/*** Pickup ***/

/*
 * Pickup all gold at the player's current location.
 */
static void py_pickup_gold(void)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    s32b total_gold = 0L;
    byte *treasure;

    s16b this_o_idx, next_o_idx = 0;

    object_type *o_ptr;

    int sound_msg;
    bool verbal = FALSE;

    /* Allocate an array of ordinary gold objects */
    treasure = C_ZNEW(SV_GOLD_MAX, byte);


    /* Pick up all the ordinary gold objects */
    for (this_o_idx = cave_o_idx[py][px]; this_o_idx; this_o_idx = next_o_idx)
    {
	/* Get the object */
	o_ptr = &o_list[this_o_idx];

	/* Get the next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Ignore if not legal treasure */
	if ((o_ptr->tval != TV_GOLD) ||
	    (o_ptr->sval > SV_GOLD_MAX)) continue;

	/* Note that we have this kind of treasure */
	treasure[o_ptr->sval-1]++;

	/* Remember whether feedback message is in order */
	if (!squelch_item_ok(o_ptr))
	    verbal = TRUE;

	/* Increment total value */
	total_gold += (s32b)o_ptr->pval;

	/* Delete the gold */
	delete_object_idx(this_o_idx);
    }

    /* Pick up the gold, if present */
    if (total_gold)
    {
	char buf[1024];
	char tmp[80];
	int i, count, total, k_idx;

	/* Build a message */
	(void)strnfmt(buf, sizeof(buf), "You have found %ld gold pieces worth of ", (long)total_gold);

	/* Count the types of treasure present */
	for (total = 0, i = 0; i < SV_GOLD_MAX; i++)
	{
	    if (treasure[i]) total++;
	}

	/* List the treasure types */
	for (count = 0, i = 0; i < SV_GOLD_MAX; i++)
	{
	    /* Skip if no treasure of this type */
	    if (!treasure[i]) continue;

	    /* Get this object index */
	    k_idx = lookup_kind(TV_GOLD, i+1);

	    /* Skip past errors  XXX */
	    if (k_idx <= 0) continue;

	    /* Get the object name */
	    object_kind_name(tmp, sizeof tmp, k_idx, TRUE);

	    /* Build up the pickup string */
	    my_strcat(buf, tmp, sizeof(buf));

	    /* Added another kind of treasure */
	    count++;

	    /* Add a comma if necessary */
	    if ((total > 2) && (count < total)) my_strcat(buf, ",", sizeof(buf));

	    /* Add an "and" if necessary */
	    if ((total >= 2) && (count == total-1)) my_strcat(buf, " and", sizeof(buf));

	    /* Add a space or period if necessary */
	    if (count < total) my_strcat(buf, " ", sizeof(buf));
	    else               my_strcat(buf, ".", sizeof(buf));
	}

	/* Determine which sound to play */
	if      (total_gold < 200) sound_msg = MSG_MONEY1;
	else if (total_gold < 600) sound_msg = MSG_MONEY2;
	else                       sound_msg = MSG_MONEY3;

	/* Display the message */
	if (verbal)
	    msgt(sound_msg, buf);

	/* Add gold to purse */
	p_ptr->au += total_gold;

	/* Redraw gold */
	p_ptr->redraw |= (PR_GOLD);
    }

    /* Free the gold array */
    FREE(treasure);
}



/**
 * Return TRUE if the given object can be automatically picked up
 */
static bool auto_pickup_okay(object_type * o_ptr)
{
	if (!inven_carry_okay(o_ptr)) return FALSE;
	if (OPT(pickup_always) || check_for_inscrip(o_ptr, "=g")) return TRUE;
	if (OPT(pickup_inven) && inven_stack_okay(o_ptr)) return TRUE;

	return FALSE;
}


/**
 * Carry an object and delete it.
 */
extern void py_pickup_aux(int o_idx, bool domsg)
{
    int slot, quiver_slot = 0;

    char o_name[120];
    object_type *o_ptr = &o_list[o_idx];
    object_type *i_ptr = &p_ptr->inventory[INVEN_LIGHT];
    bitflag f[OF_SIZE], obvious_mask[OF_SIZE];


    flags_init(obvious_mask, OF_SIZE, OF_OBVIOUS_MASK, FLAG_END);
    of_copy(f, o_ptr->flags_obj);
    
    /* Carry the object */
    slot = inven_carry(p_ptr, o_ptr);

    /* Handle errors (paranoia) */
    if (slot < 0) return;

    /* If we have picked up ammo which matches something in the quiver, note
     * that it so that we can wield it later (and suppress pick up message) */
    if (obj_is_quiver_obj(o_ptr)) 
    {
	int i;
	for (i = QUIVER_START; i < QUIVER_END; i++) 
	{
	    if (!p_ptr->inventory[i].k_idx) continue;
	    if (!object_similar(&p_ptr->inventory[i], o_ptr,
				OSTACK_QUIVER)) continue;
	    quiver_slot = i;
	    break;
	}
    }

    /* Get the object again */
    o_ptr = &p_ptr->inventory[slot];

    /* Set squelch status */
    p_ptr->notice |= PN_SQUELCH;

    /* Stone of Lore gives id on pickup */
    if (!object_known_p(o_ptr)) {
	if (i_ptr->sval == SV_STONE_LORE)
	    identify_object(o_ptr);

	/* Otherwise pseudo-ID */
	else {
	    bool heavy = FALSE;
	    int feel;

	    /* Heavy sensing */
	    heavy = (player_has(PF_PSEUDO_ID_HEAVY));

	    /* Type of feeling */
	    feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));

	    /* We have "felt" it */
	    o_ptr->ident |= (IDENT_SENSE);

	    /* Inscribe it textually */
	    o_ptr->feel = feel;

	    /* Set squelch flag as appropriate */
	    p_ptr->notice |= PN_SQUELCH;
	}
    }

    /* Log artifacts if found */
    if (artifact_p(o_ptr))
	history_add_artifact(o_ptr->name1, object_is_known(o_ptr), TRUE);

    /* Notice dice and other obvious stuff */
    notice_other(IF_DD_DS, slot + 1);
    (void) of_inter(f, obvious_mask);
    of_union(o_ptr->id_obj, f);

    /* Average things are average */
    if ((o_ptr->feel == FEEL_AVERAGE) && (is_weapon(o_ptr) || is_armour(o_ptr))){
	notice_other(IF_AC, slot + 1);
	notice_other(IF_TO_A, slot + 1);
	notice_other(IF_TO_H, slot + 1);
	notice_other(IF_TO_D, slot + 1);
    }

    /* Recalculate the bonuses */
    p_ptr->update |= (PU_BONUS);

    /* Optionally, display a message */
    if (domsg && !quiver_slot)
    {
	/* Describe the object */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

	/* Message */
	msg("You have %s (%c).", o_name, index_to_label(slot));
    }


    /* Delete the object */
    delete_object_idx(o_idx);

    /* If we have a quiver slot that this item matches, use it */
    if (quiver_slot) wield_item(o_ptr, slot, quiver_slot);
}


/**
 * Pick up objects and treasure on the floor, now also used for telekinesis.
 *
 * Called with pickup:
 * 0 to grab gold and describe non-gold objects.
 * 1 to pick up objects either with or without displaying a menu.
 * 2 to pick up objects, forcing a menu for multiple objects.
 * 3 to pick up objects, forcing a menu for any number of objects.
 *
 * Scan the list of objects in that floor grid.   Pick up gold automatically.
 * Pick up objects automatically until pile or backpack space is full if 
 * auto-pickup option is on, carry_query_floor option is not, and menus are 
 * not forced (which the "get" command does). Otherwise, store objects on 
 * floor in an array, and tally both how many there are and can be picked up.
 *
 * If the player is not picking up objects, describe a single object or 
 * indicate the presence of a floor stack.  If player is picking up objects, 
 * name a single object, or indicate a stack of objects, that cannot go in 
 * the backpack.
 *
 * Pick up a single object without menus, unless menus for single items are 
 * forced.  Confirm pickup if that option is on.
 *
 * Pick up multiple objects (unless using autopickup, no confirm) using Tim
 * Baker's menu system.   Recursively call this function (forcing menus for any 
 * number of objects) until objects are gone, backpack is full, or player is 
 * satisfied.
 *
 * Keep track of number of objects picked up (to calculate time spent).
 */
byte py_pickup(int pickup, int y, int x)
{
    s16b this_o_idx, next_o_idx = 0;

    char o_name[120];
    object_type *o_ptr;

    /* Objects picked up.  Used to determine time cost of command. */
    byte objs_picked_up = 0;

    size_t floor_num = 0;
    int floor_list[MAX_FLOOR_STACK + 1], floor_o_idx = 0;

    int can_pickup = 0;
    bool call_function_again = FALSE;
    bool blind = ((p_ptr->timed[TMD_BLIND]) || (no_light()));
    bool domsg = TRUE;
    bool telekinesis = ((y != p_ptr->py) || (x != p_ptr->px));

    /* Always pickup gold, effortlessly */
    if (!telekinesis) py_pickup_gold();

    /* Nothing to pick up -- return */
    if (!cave_o_idx[y][x]) return (objs_picked_up);

    /* Scan the pile of objects */
    for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {

	/* Access the object */
	o_ptr = &o_list[this_o_idx];

	/* Access the next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Ordinary pickup */
	if (!telekinesis) {

	    /* Ignore all hidden objects and non-objects */
	    if (squelch_hide_item(o_ptr) || !o_ptr->k_idx)
		continue;

	    /* Hack -- disturb */
	    disturb(0, 0);

	    /* Automatically pick up some items */
	    if (auto_pickup_okay(o_ptr)) {
		/* Pick up the object */
		py_pickup_aux(this_o_idx, TRUE);
		objs_picked_up++;

		/* Check the next object */
		continue;
	    }
	}

	/* Tally objects and store them in an array. */
	
	/* Remember this object index */
	floor_list[floor_num] = this_o_idx;
	
	/* Count non-gold objects that remain on the floor. */
	floor_num++;
	
	/* Tally objects that can be picked up.*/
	if (inven_carry_okay(o_ptr))
	    can_pickup++;
    }

    /* There are no non-gold objects */
    if (!floor_num)
	return (objs_picked_up);

    /* Get hold of the last floor index */
    floor_o_idx = floor_list[floor_num - 1];

    /* Mention the objects if player is not picking them up. */
    if (pickup == 0 || !(can_pickup || telekinesis))
    {
	const char *p = "see";

	/* One object */
	if (floor_num == 1)
	{
	    if (!can_pickup)	p = "have no room for";
	    else if (blind)     p = "feel";

	    /* Get the object */
	    o_ptr = &o_list[floor_o_idx];

	    /* Describe the object.  Less detail if blind. */
	    if (blind)
		object_desc(o_name, sizeof(o_name), o_ptr,
			    ODESC_PREFIX | ODESC_BASE);
	    else
		object_desc(o_name, sizeof(o_name), o_ptr,
			    ODESC_PREFIX | ODESC_FULL);

	    /* Message */
	    message_flush();
	    msg("You %s %s.", p, o_name);
	}
	else
	{
	    /* Display more information about floor items */
	    ui_event e;

	    if (!can_pickup)
		p = "have no room for the following objects";
	    else if (blind)
		p = "feel something on the floor";

	    /* Scan all marked objects in the grid */
	    floor_num = scan_floor(floor_list, N_ELEMENTS(floor_list), y, x, 0x03);

	    /* Save screen */
	    screen_save();

	    /* Display objects on the floor */
	    show_floor(floor_list, floor_num, (OLIST_WEIGHT));

	    /* Display prompt */
	    prt(format("You %s: ", p), 0, 0);

	    /* Wait for it.  Use key as next command. */
	    e = inkey_ex();
	    Term_event_push(&e);

	    /* Restore screen */
	    screen_load();
	}

	/* Done */
	return (objs_picked_up);
    }

    /* We can pick up objects.  Menus are not requested (yet). */
    if (pickup == 1)
    {
	/* Scan floor (again) */
	floor_num = scan_floor(floor_list, N_ELEMENTS(floor_list), y, x, 0x03);

	/* Use a menu interface for multiple objects, or get single objects */
	if (floor_num > 1)
	    pickup = 2;
	else
	    this_o_idx = floor_o_idx;
    }


    /* Display a list if requested. */
    if (pickup == 2)
    {
	const char *q, *s;
	int item;

	/* Get an object or exit. */
	q = "Get which item?";
	s = "You see nothing there.";

	/* Telekinesis */
	if (telekinesis) {
	    item_tester_hook = inven_carry_okay;

	    if (!get_item(&item, q, s, CMD_PICKUP, USE_TARGET))
		return (objs_picked_up);

	    this_o_idx = 0 - item;
	}
	else {
	    /* Restrict the choices */
	    item_tester_hook = inven_carry_okay;

	    if (!get_item(&item, q, s, CMD_PICKUP, USE_FLOOR))
		return (objs_picked_up);

	    this_o_idx = 0 - item;
	    call_function_again = TRUE;
	}

	/* With a list, we do not need explicit pickup messages */
	domsg = FALSE;
    }

    /* Pick up object, if legal */
    if (this_o_idx)
    {
	/* Regular pickup or telekinesis with pack not full */
	if (can_pickup) {
	    /* Pick up the object */
	    py_pickup_aux(this_o_idx, domsg);
	}
	/* Telekinesis with pack full */
	else {
	    /* Access the object */
	    o_ptr = &o_list[this_o_idx];
	    
	    /* Drop it */
	    drop_near(o_ptr, -1, p_ptr->py, p_ptr->px, TRUE);
	    
	    /* Delete the old object */
	    delete_object_idx(this_o_idx);
	}
    }

    /* Indicate an object picked up. */
    objs_picked_up = 1;

    /* If requested, call this function recursively.  Count objects picked up.
     * Force the display of a menu in all cases. */
    if (call_function_again)
	objs_picked_up += py_pickup(2, y, x);

    /* Indicate how many objects have been picked up. */
    return (objs_picked_up);
}


/**
 * Handle falling off cliffs 
 */
void fall_off_cliff(void)
{
    int i = 0, dam;

    msg("You fall into the darkness!");

    /* Where we fell from */
    p_ptr->last_stage = p_ptr->stage;

    /* From the mountaintop */
    if (stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP) {
	p_ptr->stage = stage_map[p_ptr->stage][DOWN];
	p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	/* Reset */
	stage_map[MOUNTAINTOP_STAGE][DOWN] = 0;
	stage_map[p_ptr->stage][UP] = 0;
	stage_map[MOUNTAINTOP_STAGE][DEPTH] = 0;

	if (p_ptr->state.ffall) {
	    notice_obj(OF_FEATHER, 0);
	    dam = damroll(2, 8);
	    (void) inc_timed(TMD_STUN, damroll(2, 8), TRUE);
	    (void) inc_timed(TMD_CUT, damroll(2, 8), TRUE);
	} else {
	    dam = damroll(4, 8);
	    (void) inc_timed(TMD_STUN, damroll(4, 8), TRUE);
	    (void) inc_timed(TMD_CUT, damroll(4, 8), TRUE);
	}
	take_hit(dam, "falling off a precipice");
    }

    /* Nan Dungortheb */
    else {
	/* Fall at least one level */
	for (i = 0; i < 1; i = randint0(3)) {
	    p_ptr->stage = stage_map[p_ptr->stage][SOUTH];
	    p_ptr->depth++;
	    if (p_ptr->state.ffall) {
		notice_obj(OF_FEATHER, 0);
		dam = damroll(2, 8);
		(void) inc_timed(TMD_STUN, damroll(2, 8), TRUE);
		(void) inc_timed(TMD_CUT, damroll(2, 8), TRUE);
	    } else {
		dam = damroll(4, 8);
		(void) inc_timed(TMD_STUN, damroll(4, 8), TRUE);
		(void) inc_timed(TMD_CUT, damroll(4, 8), TRUE);
	    }
	    take_hit(dam, "falling off a precipice");
	    if (p_ptr->depth == 70)
		break;
	}

	/* Check for quests */
	if (OPT(adult_dungeon) && is_quest(p_ptr->stage)
	    && (p_ptr->depth < 100)) {
	    int i;
	    monster_race *r_ptr = NULL;

	    /* Find the questor */
	    for (i = 0; i < z_info->r_max; i++) {
		r_ptr = &r_info[i];
		if ((rf_has(r_ptr->flags, RF_QUESTOR))
		    && (r_ptr->level == p_ptr->depth))
		    break;
	    }

	    /* Announce */
	    msg("This level is home to %s.", r_ptr->name);
	}

    }

    /* Leaving */
    p_ptr->leaving = TRUE;

}

/**
 * Move player in the given direction, with the given "pickup" flag.
 *
 * This routine should only be called when energy has been expended.
 *
 * Note that this routine handles monsters in the destination grid,
 * and also handles attempting to move into walls/doors/etc.
 */
void move_player(int dir)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    byte str_escape, dex_escape;

    /* Permit the player to move? */
    bool can_move = FALSE;

    /* Player is jumping off a cliff */
    bool falling = FALSE;

    /* Player hits a trap (always unless flying) */
    bool trapped = TRUE;

    int temp;
    int y, x;

    feature_type *f_ptr;

    /* Find the result of moving */
    y = py + ddy[dir];
    x = px + ddx[dir];
    f_ptr = &f_info[cave_feat[y][x]];


    /* Hack -- attack monsters */
    if (cave_m_idx[y][x] > 0) 
    {
	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* It takes some dexterity, or failing that strength, to get out of pits */
    if (cave_feat[py][px] == FEAT_PIT) 
    {
	str_escape = adj_dex_dis[p_ptr->state.stat_ind[A_STR]];
	dex_escape = adj_dex_dis[p_ptr->state.stat_ind[A_DEX]];

	/* First attempt to leap out of the pit, */
	if ((dex_escape + 1) * 2 < randint1(16)) 
	{
	    /* then attempt to climb out of the pit. */
	    if (str_escape + 3 < randint1(16)) 
	    {
		/* Failure costs a turn. */
		msg("You remain stuck in the pit.");
		return;
	    } 
	    else
		msg("You clamber out of the pit.");
	} 
	else
	    msg("You leap out of the pit.");
    }


    /* Option to disarm a visible trap. -TNB- */
    /* Hack - Rogues can walk over their own trap - BR */
    if (OPT(easy_alter) && cave_visible_trap(y, x) && cave_player_trap(y, x))
    {
	bool more = FALSE;
	/* Auto-repeat if not already repeating */
	if (cmd_get_nrepeats() == 0)
	    cmd_set_repeat(99);
	
	more = do_cmd_disarm_aux(y, x);

	/* Cancel repeat unless we may continue */
	if (!more)
	    disturb(0, 0);
	return;
    }

    /* Some terrain is impassable for the player, such as stone walls. */
    else if (!tf_has(f_ptr->flags, TF_PASSABLE)) {
	/* Disturb the player */
	disturb(0, 0);

	/* Notice unknown obstacles */
	if (!cave_has(cave_info[y][x], CAVE_MARK)) {
	    /* Closed door */
	    if (tf_has(f_ptr->flags, TF_DOOR_CLOSED)) {
		msgt(MSG_HITWALL, "You feel a door blocking your way.");
		cave_on(cave_info[y][x], CAVE_MARK);
		light_spot(y, x);
	    }

	    /* Wall (or secret door) */
	    else {
		msgt(MSG_HITWALL, "You feel a wall blocking your way.");
		cave_on(cave_info[y][x], CAVE_MARK);
		light_spot(y, x);
	    }
	}

	/* Mention known obstacles */
	else {
	    /* Closed door */
	    if (tf_has(f_ptr->flags, TF_DOOR_CLOSED)) {
		/* Option to automatically open doors. -TNB- */
		if (OPT(easy_alter)) {
		    bool more = FALSE;

		    /* Auto-repeat if not already repeating */
		    if (cmd_get_nrepeats() == 0)
			cmd_set_repeat(99);
		    
		    /* Open the door */
		    more = do_cmd_open_aux(y, x);
		    
		    /* Cancel repeat unless we may continue */
		    if (!more)
			disturb(0, 0);
		    return;
		}
		
		/* Otherwise, a message. */
		msgt(MSG_HITWALL, "There is a door blocking your way.");
	    }

	    /* Wall (or secret door) */
	    else {
		msgt(MSG_HITWALL, "There is a wall blocking your way.");
	    }
	}

	/* Sound */
	sound(MSG_HITWALL);
    }

    /* Normal movement */
    else 
    {
	/* Assume terrain can be traversed normally. */
	can_move = TRUE;
	
	/*** Handle traversable terrain.  ***/
	if (tf_has(f_ptr->flags, TF_ROCK))
	{
	    /* Dwarves move easily through rubble */
	    if (player_has(PF_DWARVEN))
		can_move = TRUE;
	    
	    /* Bats, dragons can fly */
	    else if ((p_ptr->schange == SHAPE_BAT)
		     || (p_ptr->schange == SHAPE_WYRM))
		can_move = TRUE;

	    /* Require more energy */
	    else 
	    {
		can_move = TRUE;
		p_ptr->energy_use += 100;
	    }
	}

	if (tf_has(f_ptr->flags, TF_TREE))
	{
	    /* Druids, rangers, elves and ents (SJGU) slip easily under
	     * trees */
	    if (((player_has(PF_WOODSMAN)) || (player_has(PF_ELVEN)))
		|| (player_has(PF_WOODEN)))
		can_move = TRUE;
	    
	    /* Bats, dragons can fly */
	    else if ((p_ptr->schange == SHAPE_BAT)
		     || (p_ptr->schange == SHAPE_WYRM))
		can_move = TRUE;

	    /* Require more energy */
	    else 
	    {
		can_move = TRUE;
		p_ptr->energy_use += 100;
	    }
	}

	/* Water now slows rather than stopping -NRM- */
	if (tf_has(f_ptr->flags, TF_WATERY))
	{
	    /* Stop any run. */
	    disturb(0, 0);
	    
	    can_move = TRUE;
	    
	    /* Speed will need updating */
	    p_ptr->update |= PU_BONUS;
	}
	
	if (tf_has(f_ptr->flags, TF_FIERY))
	{
	    /* Assume player will continue. */
	    temp = TRUE;
	    
	    /* Smart enough to stop running. */
	    if (p_ptr->running) {
		if (!get_check("Lava blocks your path.  Step into it? ")) {
		    temp = FALSE;
		    p_ptr->running = 0;
		}
	    }

	    /* Smart enough to sense trouble. */
	    else if ((!p_resist_pos(P_RES_FIRE))
		     || (!p_resist_strong(P_RES_FIRE)
			 && (p_ptr->chp <= 100))
		     || (!p_immune(P_RES_FIRE) && (p_ptr->chp <= 30))) 
	    {
		if (!get_check
		    ("The heat of the lava scalds you! Really enter? ")) 
		{
		    temp = FALSE;
		}
	    }
	    
	    /* Enter if OK or confirmed. */
	    if (temp) 
	    {
		/* Can always cross. */
		can_move = TRUE;
		
		    /* Feather fall makes one lightfooted. */
		    if (p_ptr->state.ffall) 
		    {
			notice_obj(OF_FEATHER, 0);
			temp = 49 + randint1(51);
		    } 
		    else
			temp = 124 + randint1(126);

		    /* Will take serious fire damage. */
		    fire_dam(temp, "burnt to a cinder in molten lava");
	    }
	}
	
	if (tf_has(f_ptr->flags, TF_FALL))
	{
	    /* Bats, dragons can fly */
	    if (!(p_ptr->schange == SHAPE_BAT) &&
		!(p_ptr->schange == SHAPE_WYRM))
	    {
		/* Assume player will continue. */
		temp = TRUE;
		
		/* Smart enough to stop running. */
		if (p_ptr->running) 
		{
		    if (!get_check
			("You have come to a cliff.  Step off it? ")) 
		    {
			can_move = FALSE;
			temp = FALSE;
			p_ptr->running = 0;
		    }
		}
		
		/* Smart enough to sense trouble. */
		else if (!p_ptr->timed[TMD_BLIND]) 
		{
		    if (!get_check("It's a cliff! Really step off it? ")) 
		    {
			can_move = FALSE;
			temp = FALSE;
		    }
		}
		
		/* Step off if confirmed. */
		if (temp) 
		{
		    /* Will take serious damage. */
		    falling = TRUE;
		}
	    }
	}
    }

    /* If the player can move, handle various things. */
    if (can_move) 
    {
	/* Move player */
	monster_swap(py, px, y, x);
	
	/* Update speed if stepping out of water */
	if (tf_has(f_info[cave_feat[py][px]].flags, TF_WATERY))
	    p_ptr->update |= PU_BONUS;
	
	/* Update stealth for Unlight */
	if (player_has(PF_UNLIGHT))
	    p_ptr->update |= PU_BONUS;
	
	/* Update speed for elven woodspersons */
	if (player_has(PF_WOODSMAN) && player_has(PF_ELVEN))
	    p_ptr->update |= PU_BONUS;
	
	/* Superstealth for ents in trees SJGU */
	if ((player_has(PF_WOODEN))	&&
	    (tf_has(f_info[cave_feat[p_ptr->py][p_ptr->px]].flags, TF_TREE))) 
	{
	    if (!(tf_has(f_info[cave_feat[py][px]].flags, TF_TREE))
		|| !(p_ptr->timed[TMD_SSTEALTH])) 
	    {
		(void) inc_timed(TMD_SSTEALTH, 1, FALSE);
		p_ptr->update |= (PU_BONUS);
	    }
	} 
	else if ((player_has(PF_WOODEN))
		   && (tf_has(f_info[cave_feat[py][px]].flags, TF_TREE))) 
	{
	    if (p_ptr->timed[TMD_SSTEALTH]) 
	    {
		    (void) dec_timed(TMD_SSTEALTH, 1, FALSE);
		    p_ptr->update |= (PU_BONUS);
	    }
	}

	/* New location */
	y = py = p_ptr->py;
	x = px = p_ptr->px;
	f_ptr = &f_info[cave_feat[y][x]];
	
	/* Fall off a cliff */
	if (falling)
	    fall_off_cliff();
	
	/* Spontaneous Searching */
	if (p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] > 49) {
	    (void) search(FALSE);
	} else if (0 == randint0(50 - p_ptr->state.skills[SKILL_SEARCH_FREQUENCY])) 
	{
	    (void) search(FALSE);
	}
	
	/* Continuous Searching */
	if (p_ptr->searching) {
	    (void) search(FALSE);
	}
	
	/* Handle "store doors" */
	if (tf_has(f_ptr->flags, TF_SHOP)) {
	    /* Disturb */
	    disturb(0, 0);
	    cmd_insert(CMD_ENTER_STORE);
	}
	
	/* All other grids (including traps) */
	else
	{
	    /* Handle objects (later) */
	    p_ptr->notice |= (PN_PICKUP);
	}
	
	/* Flying players have a chance to miss traps */
	if ((p_ptr->schange == SHAPE_BAT) || (p_ptr->schange == SHAPE_WYRM)) {
	    if (cave_invisible_trap(y, x) && cave_player_trap(y, x) &&
		(randint0(3) != 0))
		trapped = FALSE;
	    else if (cave_visible_trap(y, x) && cave_player_trap(y, x) && 
		     (randint0(10) != 0))
		trapped = FALSE;
	}
	
	/* Discover invisible traps */
	if (cave_invisible_trap(y, x) && trapped) 
	{
	    /* Disturb */
	    disturb(0, 0);
	    
	    /* Hit the trap. */
	    hit_trap(y, x);
	}
	
	/* Set off a visible trap */
	else if (cave_visible_trap(y, x) && cave_player_trap(y, x) && 
		 trapped)
	{
	    /* Disturb */
	    disturb(0, 0);
	    
	    /* Hit the trap. */
	    hit_trap(y, x);
	}
	
	/* Walk on a monster trap */
	else if (cave_monster_trap(y, x))
	{
	    msg("You inspect your cunning trap.");
	}
    }
}



