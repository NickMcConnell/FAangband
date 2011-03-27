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
#include "squelch.h"


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
    if (p_ptr->timed[TMD_CONFUSED] || p_ptr->timed[TMD_HALLUC])
	chance = chance / 10;

    /* Prevent fruitless searches */
    if (chance <= 0)
    {
	if (verbose)
	{
	    msg_print("You can't make out your surroundings well enough to search.");

	    /* Cancel repeat */
	    disturb(0, 0);
	}

	return FALSE;
    }

    /* Search the nearby grids, which are always in bounds */
    for (y = (py - 1); y <= (py + 1); y++) {
	for (x = (px - 1); x <= (px + 1); x++) {
	    feature_type *f_ptr = &f_info[cave_feat[y][x]];

	    /* Sometimes, notice things */
	    if (randint0(100) < chance) {
		/* Invisible trap */
		if (tf_has(f_ptr->flags, TF_TRAP_INVIS)) {
		    found = TRUE;

		    /* Pick a trap */
		    pick_trap(y, x);

		    /* Message */
		    msg_print("You have found a trap.");

		    /* Disturb */
		    disturb(0, 0);
		}

		/* Secret door */
		if (cave_feat[y][x] == FEAT_SECRET) {
		    found = TRUE;

		    /* Message */
		    msg_print("You have found a secret door.");

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
			msg_print("You have discovered a trap on the chest!");

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
	    msg_print("There are no secrets here.");
	else
	    msg_print("You found nothing.");
    }

    return TRUE;
}


/**
 * Return TRUE if the given object is inscribed with "=g".
 *
 * Alternatively, also return TRUE if any similar item in the
 * backpack is marked "=g".
 */
static bool auto_pickup_check(object_type * o_ptr, bool check_pack)
{
    cptr s;

    /* Check inscription */
    if (o_ptr->note) {
	/* Find a '=' */
	s = strchr(quark_str(o_ptr->note), '=');

	/* Process preventions */
	while (s) {
	    /* =g ('g'et) means auto pickup */
	    if (s[1] == 'g')
		return (TRUE);

	    /* Find another '=' */
	    s = strchr(s + 1, '=');
	}

	/* Throwing weapons and ammo need no extra inscription */
	s = strchr(quark_str(o_ptr->note), '@');

	/* Process preventions */
	while (s) {
	    /* =g ('g'et) means auto pickup */
	    if ((s[1] == 'f') || (s[1] == 'v'))
		return (TRUE);

	    /* Find another '=' */
	    s = strchr(s + 1, '=');
	}
    }

    /* Optionally, check the backpack */
    if (check_pack) {
	int j;

	/* Look for similar and inscribed */
	for (j = 0; j < INVEN_PACK - p_ptr->pack_size_reduce; j++) {
	    object_type *j_ptr = &p_ptr->inventory[j];

	    /* Skip non-objects */
	    if (!j_ptr->k_idx)
		continue;

	    /* The two items must be able to combine */
	    if (!object_similar(j_ptr, o_ptr))
		continue;

	    /* The backpack item must be inscribed */
	    if (!j_ptr->note)
		continue;

	    /* Find a '=' */
	    s = strchr(quark_str(j_ptr->note), '=');

	    /* Process preventions */
	    while (s) {
		/* =g ('g'et) means auto pickup */
		if (s[1] == 'g')
		    return (TRUE);

		/* Find another '=' */
		s = strchr(s + 1, '=');
	    }
	}
    }

    /* Don't auto pickup */
    return (FALSE);
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
extern void py_pickup_aux(int o_idx)
{
    int slot, quiver_slot = 0;

    char o_name[120];
    object_type *o_ptr = &o_list[o_idx];
    object_type *i_ptr = &p_ptr->inventory[INVEN_LIGHT];
    bitflag f[OF_SIZE], obvious_mask[OF_SIZE];


    flags_init(obvious_mask, OF_SIZE, OF_OBVIOUS_MASK, FLAG_END);
    of_copy(f, o_ptr->flags_obj);
    
    /* Carry the object */
    slot = inven_carry(o_ptr);

    /* Handle errors (paranoia) */
    if (slot < 0) return;

    /* If we have picked up ammo which matches something in the quiver, note
     * that it so that we can wield it later (and suppress pick up message) */
    if (obj_is_ammo(o_ptr)) 
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
    if (!quiver_slot)
    {
	/* Describe the object */
	object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

	/* Message */
	msg_format("You have %s (%c).", o_name, index_to_label(slot));
    }


    /* Delete the object */
    delete_object_idx(o_idx);

	/* If we have a quiver slot that this ammo matches, use it */
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

    int floor_num = 0, floor_list[24], floor_o_idx = 0;

    int can_pickup = 0;
    bool call_function_again = FALSE;
    bool do_ask = TRUE;
    bool telekinesis;

    /* Set telekinesis flag */
    telekinesis = (!(y == p_ptr->py) || !(x == p_ptr->px));

    /* Scan the pile of objects */
    for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx) {
	object_type *o_ptr;
	bool menu_flag = FALSE;

	/* Access the object */
	o_ptr = &o_list[this_o_idx];

	/* Describe the object */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Access the next object */
	next_o_idx = o_ptr->next_o_idx;

	/* Ordinary pickup */
	if (!telekinesis) {

	    /* Ignore all hidden objects and non-objects */
	    if (squelch_hide_item(o_ptr) || !o_ptr->k_idx)
		continue;

	    /* Hack -- disturb */
	    disturb(0, 0);

	    /* Pick up gold */
	    if (o_ptr->tval == TV_GOLD) {
		/* Message */
		msg_format("You have found %ld gold pieces worth of %s.",
			   (long) o_ptr->pval, o_name);

		/* Collect the gold */
		p_ptr->au += o_ptr->pval;

		/* Limit to avoid buffer overflow */
		if (p_ptr->au > PY_MAX_GOLD)
		    p_ptr->au = PY_MAX_GOLD;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Delete the gold */
		delete_object_idx(this_o_idx);

		/* Check the next object */
		continue;
	    }

	    /* Automatically pick up objects like those in the quiver. */
	    if (quiver_carry(o_ptr, this_o_idx))
		continue;

	    /* Automatically pick up some items */
	    if (inven_carry_okay(o_ptr) && auto_pickup_okay(o_ptr)) {
		/* Pick up the object */
		py_pickup_aux(this_o_idx);

		/* Check the next object */
		continue;
	    }

	    /* Pick up objects automatically, without menus, if pickup is
	     * allowed and menus are not forced, auto-pickup is on, and
	     * query_floor is off. */
	    else if ((pickup == 1) && (OPT(always_pickup))
		     && (!OPT(carry_query_flag))) {
		/* If backpack is not full, carry the item. */
		if (inven_carry_okay(o_ptr)) {
		    /* Pick up the object */
		    py_pickup_aux(this_o_idx);
		}

		/* Otherwise, add one to the number of floor objects. */
		else {
		    /* Count non-gold objects that remain on the floor. */
		    floor_num++;

		    /* Remember this index */
		    floor_o_idx = this_o_idx;
		}

	    }
	    /* Otherwise, activate the menu system.  */
	    else
		menu_flag = TRUE;
	}

	/* Tally objects and store them in an array. */
	if (telekinesis || menu_flag) {
	    /* Remember this object index */
	    floor_list[floor_num] = this_o_idx;

	    /* Count non-gold objects that remain on the floor. */
	    floor_num++;

	    /* Remember this index */
	    floor_o_idx = this_o_idx;

	    /* Tally objects that can be picked up. */
	    if (inven_carry_okay(o_ptr)) {
		can_pickup++;
	    }
	}
    }

    /* There are no non-gold objects */
    if (!floor_num)
	return (objs_picked_up);


    /* Mention the objects or stack, if player is not picking them up. */
    if (!pickup) {
	/* One object */
	if (floor_num == 1) {
	    /* Access the object */
	    o_ptr = &o_list[floor_o_idx];

	    /* Describe the object.  Less detail if blind. */
	    if (p_ptr->timed[TMD_BLIND])
		object_desc(o_name, o_ptr, TRUE, 0);
	    else
		object_desc(o_name, o_ptr, TRUE, 3);

	    /* Message */
	    msg_format("You %s %s.", (p_ptr->timed[TMD_BLIND] ? "feel" : "see"),
		       o_name);
	}

	/* Multiple objects */
	else {
	    /* Message */
	    msg_format("You %s a pile of %d items.",
		       (p_ptr->timed[TMD_BLIND] ? "feel" : "see"), floor_num);
	}

	/* Done */
	return (objs_picked_up);
    }

    /* The player has no room for anything on the floor. */
    else if (!(can_pickup || telekinesis)) {
	/* One object */
	if (floor_num == 1) {
	    /* Access the object */
	    o_ptr = &o_list[floor_o_idx];

	    /* Describe the object.  Less detail if blind. */
	    if (p_ptr->timed[TMD_BLIND])
		object_desc(o_name, o_ptr, TRUE, 0);
	    else
		object_desc(o_name, o_ptr, TRUE, 3);

	    /* Message */
	    msg_format("You have no room for %s.", o_name);
	}

	/* Multiple objects */
	else {
	    /* Message */
	    msg_print("You have no room for any of the objects on the floor.");
	}

	/* Done */
	return (objs_picked_up);
    }

    /* Simple display of a single object, if menus for single objects are not
     * forced. */
    else if ((floor_num == 1) && (pickup != 3) && (!telekinesis)) {
	/* Hack -- query every object */
	if (OPT(carry_query_flag)) {
	    char out_val[160];
	    int answer = 0;

	    /* Access the object */
	    o_ptr = &o_list[floor_o_idx];

	    /* Describe the object.  Less detail if blind. */
	    if (p_ptr->timed[TMD_BLIND])
		object_desc(o_name, o_ptr, TRUE, 0);
	    else
		object_desc(o_name, o_ptr, TRUE, 3);

	    /* Build a prompt */
	    (void) sprintf(out_val, "Pick up %s? ", o_name);

	    /* Ask the user to confirm */
	    answer = get_check_other(out_val, (OPT(rogue_like_commands)
					       ? KTRL('D') : 'k'));
	    if (answer == 0) {
		/* Done */
		return (objs_picked_up);
	    } else if (answer == 2) {
		p_ptr->command_item = -floor_o_idx;
		textui_cmd_destroy();
		return (objs_picked_up);
	    }
	    /* Otherwise continue */
	}

	/* Don't ask */
	do_ask = FALSE;

	/* Remember the object to pick up */
	this_o_idx = floor_o_idx;
    }

    /* Display a list if no other query took precedence. */
    else {
	cptr q, s;

	int item;

	/* Get an object or exit. */
	q = "Get which item? ";
	s = "You see nothing there.";

	/* Telekinesis */
	if (telekinesis) {
	    /* Don't restrict the choices */
	    item_tester_hook = NULL;

	    if (get_item(&item, q, s, (USE_TARGET))) {
		this_o_idx = 0 - item;
	    } else {
		return (objs_picked_up);
	    }
	}
	/* Ordinary pickup */
	else {
	    /* Restrict the choices */
	    item_tester_hook = inven_carry_okay;

	    if (get_item(&item, q, s, (USE_FLOOR))) {
		this_o_idx = 0 - item;
		call_function_again = TRUE;
	    } else {
		return (objs_picked_up);
	    }
	}
    }

    /* Into quiver via telekinesis if possible */
    if (telekinesis) {
	/* Access the object */
	o_ptr = &o_list[this_o_idx];

	if (quiver_carry(o_ptr, this_o_idx))
	    return (1);
    }

    /* Regular pickup or telekinesis with pack not full */
    if (can_pickup) {
	/* Pick up the object */
	py_pickup_aux(this_o_idx);
    }
    /* Telekinesis with pack full */
    else {
	/* Access the object */
	o_ptr = &o_list[this_o_idx];

	/* Drop it */
	drop_near(o_ptr, -1, p_ptr->py, p_ptr->px);

	/* Delete the old object */
	delete_object_idx(this_o_idx);
    }

    /* Indicate an object picked up. */
    objs_picked_up = 1;

    /* If requested, call this function recursively.  Count objects picked up.
     * Force the display of a menu in all cases. */
    if (call_function_again)
	objs_picked_up += py_pickup(3, y, x);

    /* Indicate how many objects have been picked up. */
    return (objs_picked_up);
}


/**
 * Handle falling off cliffs 
 */
void fall_off_cliff(void)
{
    int i = 0, dam;

    msg_print("You fall into the darkness!");

    /* Where we fell from */
    p_ptr->last_stage = p_ptr->stage;

    /* From the mountaintop */
    if (stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP) {
	p_ptr->stage = stage_map[p_ptr->stage][DOWN];
	p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	/* Reset */
	stage_map[256][DOWN] = 0;
	stage_map[p_ptr->stage][UP] = 0;
	stage_map[256][DEPTH] = 0;

	if (p_ptr->ffall) {
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
	    if (p_ptr->ffall) {
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
	    msg_format("This level is home to %s.", r_name + r_ptr->name);
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
void move_player(int dir, int do_pickup)
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

    /* Find the result of moving */
    y = py + ddy[dir];
    x = px + ddx[dir];


    /* Hack -- attack monsters */
    if (cave_m_idx[y][x] > 0) {
	/* Attack */
	if (py_attack(y, x, TRUE))
	    return;
    }

    /* It takes some dexterity, or failing that, strength, to get out of pits. */
    if (cave_feat[p_ptr->py][p_ptr->px] == (FEAT_TRAP_HEAD + 0x01)) {
	str_escape = adj_dex_dis[p_ptr->stat_ind[A_STR]];
	dex_escape = adj_dex_dis[p_ptr->stat_ind[A_DEX]];

	/* First attempt to leap out of the pit, */
	if ((dex_escape + 1) * 2 < randint1(16)) {
	    /* then attempt to climb out of the pit. */
	    if (str_escape + 3 < randint1(16)) {
		/* Failure costs a turn. */
		msg_print("You remain stuck in the pit.");
		return;
	    } else
		msg_print("You clamber out of the pit.");
	} else
	    msg_print("You leap out of the pit.");
    }


    /* Option to disarm a visible trap. -TNB- */
    /* Hack - Rogues can walk over their own trap - BR */
    if (OPT(easy_disarm) && (cave_feat[y][x] >= FEAT_TRAP_HEAD)
	&& (cave_feat[y][x] <= FEAT_TRAP_TAIL)) {
	/* Optional auto-repeat. */
	if (OPT(always_repeat) && (p_ptr->command_arg <= 0)) {
	    /* Repeat 99 times */
	    p_ptr->command_arg = 99;
	}

	(void) do_cmd_disarm_aux(y, x);
	return;
    }

    /* Some terrain is impassable for the player, such as stone walls. */
    else if (!cave_passable_bold(y, x)) {
	/* Disturb the player */
	disturb(0, 0);

	/* Notice unknown obstacles */
	if (!(cave_info[y][x] & (CAVE_MARK))) {
	    /* Closed door */
	    if (cave_feat[y][x] < FEAT_SECRET) {
		message(MSG_HITWALL, 0, "You feel a door blocking your way.");
		cave_info[y][x] |= (CAVE_MARK);
		light_spot(y, x);
	    }

	    /* Wall (or secret door) */
	    else {
		message(MSG_HITWALL, 0, "You feel a wall blocking your way.");
		cave_info[y][x] |= (CAVE_MARK);
		light_spot(y, x);
	    }
	}

	/* Mention known obstacles */
	else {
	    /* Closed door */
	    if (cave_feat[y][x] < FEAT_SECRET) {
		/* Option to automatically open doors. -TNB- */
		if (OPT(easy_open)) {
		    /* Optional auto-repeat. */
		    if (OPT(always_repeat) && (p_ptr->command_arg <= 0)) {
			/* Repeat 99 times */
			p_ptr->command_arg = 99;
		    }

		    (void) do_cmd_open_aux(y, x);
		    return;
		}

		/* Otherwise, a message. */
		message(MSG_HITWALL, 0, "There is a door blocking your way.");
	    }

	    /* Wall (or secret door) */
	    else {
		message(MSG_HITWALL, 0, "There is a wall blocking your way.");
	    }
	}

	/* Sound */
	sound(MSG_HITWALL);
    }

    /* Normal movement */
    else {
	/* Sound XXX XXX XXX */
	/* sound(MSG_WALK); */

      /*** Handle traversable terrain.  ***/
	switch (cave_feat[y][x]) {
	case FEAT_RUBBLE:
	    {
		/* Dwarves move easily through rubble */
		if (player_has(PF_DWARVEN))
		    can_move = TRUE;

		/* Bats, dragons can fly */
		else if ((p_ptr->schange == SHAPE_BAT)
			 || (p_ptr->schange == SHAPE_WYRM))
		    can_move = TRUE;

		else if (player_is_crossing == dir)
		    can_move = TRUE;
		else {
		    player_is_crossing = dir;

		    /* Automate 2nd movement command, if not disturbed. */
		    p_ptr->command_cmd = 59;
		    p_ptr->command_rep = 1;
		    p_ptr->command_dir = dir;
		}

		break;
	    }
	case FEAT_TREE:
	case FEAT_TREE2:
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

		/* Allow movement only if partway through already. */
		else if (player_is_crossing == dir)
		    can_move = TRUE;
		else {
		    player_is_crossing = dir;

		    /* Automate 2nd movement command, if not disturbed. */
		    p_ptr->command_cmd = 59;
		    p_ptr->command_rep = 1;
		    p_ptr->command_dir = dir;
		}

		break;
	    }
	case FEAT_WATER:	/* Water now slows rather than stopping -NRM- */
	    {
		/* Stop any run. */
		disturb(0, 0);

		can_move = TRUE;

		/* Speed will need updating */
		p_ptr->update |= PU_BONUS;

		break;
	    }
	case FEAT_LAVA:
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
			 || (!p_immune(P_RES_FIRE) && (p_ptr->chp <= 30))) {
		    if (!get_check
			("The heat of the lava scalds you! Really enter? ")) {
			temp = FALSE;
		    }
		}

		/* Enter if OK or confirmed. */
		if (temp) {
		    /* Can always cross. */
		    can_move = TRUE;

		    /* Feather fall makes one lightfooted. */
		    if (p_ptr->ffall) {
			notice_obj(OF_FEATHER, 0);
			temp = 49 + randint1(51);
		    } else
			temp = 124 + randint1(126);

		    /* Will take serious fire damage. */
		    fire_dam(temp, "burnt to a cinder in molten lava");
		}
		break;
	    }
	case FEAT_VOID:
	    {
		/* Bats, dragons can fly */
		if ((p_ptr->schange == SHAPE_BAT)
		    || (p_ptr->schange == SHAPE_WYRM))
		    can_move = TRUE;
		else {
		    /* Assume player will continue. */
		    temp = TRUE;

		    /* Smart enough to stop running. */
		    if (p_ptr->running) {
			if (!get_check
			    ("You have come to a cliff.  Step off it? ")) {
			    temp = FALSE;
			    p_ptr->running = 0;
			}
		    }

		    /* Smart enough to sense trouble. */
		    else if (!p_ptr->timed[TMD_BLIND]) {
			if (!get_check("It's a cliff! Really step off it? ")) {
			    temp = FALSE;
			}
		    }

		    /* Step off if confirmed. */
		    if (temp) {
			/* Can always jump. */
			can_move = TRUE;

			/* Will take serious damage. */
			falling = TRUE;
		    }
		}
		break;
	    }
	default:
	    {
		/* All other terrain can be traversed normally. */
		can_move = TRUE;
	    }
	}

	/* If the player can move, handle various things. */
	if (can_move) {
	    /* Move player */
	    monster_swap(py, px, y, x);

	    /* Update speed if stepping out of water */
	    if (cave_feat[py][px] == FEAT_WATER)
		p_ptr->update |= PU_BONUS;

	    /* Update stealth for Unlight */
	    if (player_has(PF_UNLIGHT))
		p_ptr->update |= PU_BONUS;

	    /* Superstealth for ents in trees SJGU */
	    if ((player_has(PF_WOODEN))
		&&
		(tf_has
		 (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags, TF_TREE))) {
		if (!(tf_has(f_info[cave_feat[py][px]].flags, TF_TREE))
		    || !(p_ptr->timed[TMD_SSTEALTH])) {
		    (void) inc_timed(TMD_SSTEALTH, 1, FALSE);
		    p_ptr->update |= (PU_BONUS);
		}
	    } else if ((player_has(PF_WOODEN))
		       && (tf_has(f_info[cave_feat[py][px]].flags, TF_TREE))) {
		if (p_ptr->timed[TMD_SSTEALTH]) {
		    (void) dec_timed(TMD_SSTEALTH, 1, FALSE);
		    p_ptr->update |= (PU_BONUS);
		}
	    }

	    /* New location */
	    y = py = p_ptr->py;
	    x = px = p_ptr->px;

	    /* No longer traversing. */
	    player_is_crossing = 0;

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
	    if ((cave_feat[y][x] >= FEAT_SHOP_HEAD)
		&& (cave_feat[y][x] <= FEAT_SHOP_TAIL)) {
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
		if (((cave_feat[y][x] == FEAT_INVIS)
		     || (cave_feat[y][x] == FEAT_GRASS_INVIS))
		    && (randint0(3) != 0))
		    trapped = FALSE;
		else if ((cave_feat[y][x] >= FEAT_TRAP_HEAD)
			 && (cave_feat[y][x] <= FEAT_TRAP_TAIL)
			 && (randint0(10) != 0))
		    trapped = FALSE;
	    }

	    /* Discover invisible traps */
	    else if (((cave_feat[y][x] == FEAT_INVIS)
		      || (cave_feat[y][x] == FEAT_GRASS_INVIS)
		      || (cave_feat[y][x] == FEAT_TREE_INVIS)
		      || (cave_feat[y][x] == FEAT_TREE2_INVIS)) && trapped) {
		/* Disturb */
		disturb(0, 0);

		/* Message */
		msg_print("You stumble upon a trap!");

		/* Pick a trap */
		pick_trap(y, x);

		/* Hit the floor trap. */
		hit_trap(y, x);
	    }

	    /* Set off a visible trap */
	    else if ((cave_feat[y][x] >= FEAT_TRAP_HEAD)
		     && (cave_feat[y][x] <= FEAT_TRAP_TAIL) && trapped) {
		/* Disturb */
		disturb(0, 0);

		/* Hit the floor trap. */
		hit_trap(y, x);
	    }

	    /* Walk on a monster trap */
	    else if ((cave_feat[y][x] >= FEAT_MTRAP_HEAD)
		     && (cave_feat[y][x] <= FEAT_MTRAP_TAIL)) {
		msg_print("You inspect your cunning trap.");
	    }
	}
    }
}


