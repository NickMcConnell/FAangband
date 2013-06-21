/** \file cmd3.c 
    \brief Commands, part 3

 * Inventory and equipment display and management interface, observing an 
 * object, inscribing, refuelling, (l)ooking around the screen and 
 * Looking around the dungeon, help info on textual chars ("8" is the home, 
 * etc.), monster memory interface, stealing and setting monster traps.
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
#include "button.h"
#include "cave.h"
#include "monster.h"
#include "player.h"
#include "spells.h"
#include "squelch.h"
#include "target.h"
#include "ui-menu.h"



/**
 * Display inventory
 */
void do_cmd_inven(void)
{
    ui_event e;
    int diff = weight_remaining();

    /* Note that we are in "inventory" mode. */
    p_ptr->command_wrk = (USE_INVEN);

    /* Save screen */
    screen_save();

    /* Hack -- show empty slots */
    item_tester_full = TRUE;

    /* Display the inventory */
    show_inven(OLIST_WEIGHT | OLIST_QUIVER);

    /* Hack -- hide empty slots */
    item_tester_full = FALSE;

    /* Insert the total burden and character capacity into a string. */
    prt(format("(Inventory) Burden %d.%d lb (%d.%d lb %s). Command: ",
	       p_ptr->total_weight / 10, p_ptr->total_weight % 10,
	       abs(diff) / 10, abs(diff) % 10,
	       (diff < 0 ? "overweight" : "remaining")), 0, 0);


    /* Get a new command */
    e = inkey_ex();
    if (!(e.type == EVT_KBRD && e.key.code == ESCAPE))
	Term_event_push(&e);

    /* Load screen */
    screen_load();
}


/**
 * Display equipment
 */
void do_cmd_equip(void)
{
    ui_event e;

    /* Note that we are in "equipment" mode */
    p_ptr->command_wrk = (USE_EQUIP);

    /* Save screen */
    screen_save();

    /* Hack -- show empty slots */
    item_tester_full = TRUE;

    /* Display the equipment */
    show_equip(OLIST_WEIGHT);

    /* Hack -- undo the hack above */
    item_tester_full = FALSE;

    /* Prompt for a command */
    prt("(Equipment) Command: ", 0, 0);

    /* Get a new command */
    e = inkey_ex();
    if (!(e.type == EVT_KBRD && e.key.code == ESCAPE))
	Term_event_push(&e);

    /* Load screen */
    screen_load();
}


bool needs_two_hands(const object_type *o_ptr)
{
    int str_adjust = (o_ptr->weight / 50 > 8) ? 8 : (o_ptr->weight / 50);

    if (of_has(o_ptr->flags_obj, OF_TWO_HANDED_REQ)) 
	return TRUE;
    
    if (of_has(o_ptr->flags_obj, OF_TWO_HANDED_DES) && 
	(p_ptr->state.stat_ind[A_STR] < 29 + str_adjust))
	return TRUE;

    return FALSE;
}

/**
 * Wield or wear a single item from the pack or floor
 */
void wield_item(object_type *o_ptr, int item, int slot)
{
    object_type *l_ptr = &p_ptr->inventory[INVEN_LIGHT];
    object_type object_type_body;
    object_type *i_ptr = &object_type_body;

    const char *fmt;
    char o_name[80];

    bool combine_quiver = FALSE;
    int num = 1;

    bitflag f[OF_SIZE];

    flags_init(f, OF_SIZE, OF_OBVIOUS_MASK, FLAG_END);

    /* If we are stacking things in the quiver */
    if (obj_is_quiver_obj(o_ptr) && (slot >= QUIVER_START))
    {
	num = o_ptr->number;
	combine_quiver = object_similar(o_ptr, &p_ptr->inventory[slot],
					OSTACK_QUIVER);
    }

    /* Take a turn */
    p_ptr->energy_use = 100;

    /* Obtain local object */
    object_copy(i_ptr, o_ptr);

    /* Modify quantity */
    i_ptr->number = num;

    /* Decrease the item (from the pack) */
    if (item >= 0)
    {
	inven_item_increase(item, -num);
	inven_item_optimize(item);
    }

    /* Decrease the item (from the floor) */
    else
    {
	floor_item_increase(0 - item, -num);
	floor_item_optimize(0 - item);
    }

    /* Get the wield slot */
    o_ptr = &p_ptr->inventory[slot];

    if (combine_quiver)
    {
	/* Add the new ammo to the already-quiver-ed stuff */
	object_absorb(o_ptr, i_ptr);
    }
    else 
    {
	/* Take off existing item */
	if (o_ptr->k_idx)
	    (void)inven_takeoff(slot, 255);

	/* If we are wielding ammo we may need to "open" the slot by shifting
	 * later ammo up the quiver; this is because we already called the
	 * inven_item_optimize() function. */
	if (slot >= QUIVER_START)
	    open_quiver_slot(slot);
	
	/* Wear the new stuff */
	object_copy(o_ptr, i_ptr);

	/* Increment the equip counter by hand */
	p_ptr->equip_cnt++;
    }

    /* Increase the weight */
    p_ptr->total_weight += i_ptr->weight * num;

    /* If he wields a weapon that requires two hands, or hasn't the strength
     * to wield a weapon that is usually wielded with both hands one-handed
     * (normally requires 18/140 to 18/160 STR), the character will
     * automatically carry any equipped shield on his back. -LM- */
    if ((slot == INVEN_WIELD) && (p_ptr->inventory[INVEN_ARM].k_idx)) {
	if (needs_two_hands(o_ptr))
	    p_ptr->state.shield_on_back = TRUE;
	else
	    p_ptr->state.shield_on_back = FALSE;
    }

    /* A character using both hands to wield his melee weapon will use his
     * back to carry an equipped shield. -LM- */
    if ((slot == INVEN_ARM) && (p_ptr->inventory[INVEN_WIELD].k_idx)) {
	/* Access the wield slot */
	object_type *o = &p_ptr->inventory[INVEN_WIELD];

	if (needs_two_hands(o)) 
	    p_ptr->state.shield_on_back = TRUE;
	else
	    p_ptr->state.shield_on_back = FALSE;
    }

    /* Set item handling -GS- and checking turn found for artifacts -NRM- */
    if (o_ptr->name1) {

	/* Is the artifact a set item? */
	artifact_type *a_ptr = &a_info[o_ptr->name1];
	if (a_ptr->set_no != 0) {
	    /* 
	     * The object is part of a set. Check to see if rest of
	     * set is equiped.
	     */
	    if (check_set(a_ptr->set_no)) {
		/* add bonuses */
		apply_set(a_ptr->set_no);
	    }
	}
    }
    
    /* Wielding off the floor will id if holding the Stone of Lore */
    if (item < 0) {
	if ((!object_known_p(o_ptr)) && (l_ptr->sval == SV_STONE_LORE))
	    identify_object(o_ptr);
	
	/* And we autoinscribe here too */
	apply_autoinscription(o_ptr);
    }

    /* Notice dice, AC, jewellery sensation ID and other obvious stuff */
    notice_other(IF_AC, slot + 1);
    notice_other(IF_DD_DS, slot + 1);
    of_inter(f, o_ptr->flags_obj);
    of_union(o_ptr->id_obj, f);
    if (is_armour(o_ptr) && (k_info[o_ptr->k_idx].to_h.base))
	notice_other(IF_TO_H, slot + 1);


    /* Average things are average */
    if ((o_ptr->feel == FEEL_AVERAGE) && (is_weapon(o_ptr) || is_armour(o_ptr)))
    {
	notice_other(IF_AC, slot + 1);
	notice_other(IF_TO_A, slot + 1);
	notice_other(IF_TO_H, slot + 1);
	notice_other(IF_TO_D, slot + 1);
    }

    /* Object has been worn */
    o_ptr->ident |= IDENT_WORN;

    /* Where is the item now */
    if (slot == INVEN_WIELD)
	fmt = "You are wielding %s (%c).";
    else if (slot == INVEN_BOW)
	fmt = "You are shooting with %s (%c).";
    else if (slot == INVEN_LIGHT)
	fmt = "Your light source is %s (%c).";
    else if (combine_quiver)
	fmt = "You combine %s in your quiver (%c).";
    else if (slot >= QUIVER_START && slot < QUIVER_END)
	fmt = "You add %s to your quiver (%c).";
    else
	fmt = "You are wearing %s (%c).";

    /* Describe the result */
    object_desc(o_name, sizeof(o_name), o_ptr, ODESC_PREFIX | ODESC_FULL);

    /* Message */
    msgt(MSG_WIELD, fmt, o_name, index_to_label(slot));

    if (!object_known_p(o_ptr)) {
	int feel;
	bool heavy = FALSE;

	heavy = (player_has(PF_PSEUDO_ID_HEAVY));

	/* Check for a feeling */
	feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));

	if (!(o_ptr->feel == feel)) {
	    /* Get an object description */
	    object_desc(o_name, sizeof(o_name), o_ptr, ODESC_BASE);

	    msg("You feel the %s (%c) you are %s %s %s...", o_name,
		       index_to_label(slot), describe_use(slot),
		       ((o_ptr->number == 1) ? "is" : "are"), feel_text[feel]);

	    /* We have "felt" it */
	    o_ptr->ident |= (IDENT_SENSE);

	    /* Inscribe it textually */
	    o_ptr->feel = feel;

	    /* Set squelch flag as appropriate */
	    p_ptr->notice |= PN_SQUELCH;
	}
    }

    /* Save quiver size */
    save_quiver_size(p_ptr);

    /* See if we have to overflow the pack */
    pack_overflow();

    /* Recalculate bonuses, torch, mana */
    p_ptr->notice |= PN_SORT_QUIVER;
    p_ptr->update |= (PU_BONUS | PU_TORCH | PU_MANA);
    p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
}



/**
 * Destroy an item
 *
 * No longer takes a turn -BR-
 */
void do_cmd_destroy(cmd_code code, cmd_arg args[])
{
    int item, amt;

    object_type *o_ptr;

    object_type destroyed_obj;

    char o_name[120];

    item = args[0].item;
    amt = args[1].number;

    /* Deal with squelched items */
    if (item == ALL_SQUELCHED) {
	squelch_items();
	return;
    }
    else if (object_from_item_idx(item)->kind->squelch)
	return;

    if (!item_is_available(item, NULL, USE_INVEN | USE_EQUIP | USE_FLOOR))
    {
	msg("You do not have that item to destroy it.");
	return;
    }

    o_ptr = object_from_item_idx(item);

    /* Hack -- Cannot destroy some cursed items */
    if (cf_has(o_ptr->flags_curse, CF_STICKY_CARRY) || 
	cf_has(o_ptr->flags_curse, CF_STICKY_WIELD)) {
	/* Oops */
	msg("Hmmm, it seems to be cursed.");

	/* Notice */
	notice_curse(CF_STICKY_CARRY, item + 1);

	/* Nope */
	return;
    }

    /* Describe the destroyed object by taking a copy with the right "amt" */
    object_copy_amt(&destroyed_obj, o_ptr, amt);
    object_desc(o_name, sizeof(o_name), &destroyed_obj,
		ODESC_PREFIX | ODESC_FULL);

    /* Artifacts cannot be destroyed */
    if (artifact_p(o_ptr)) {
	int feel = FEEL_SPECIAL;

	/* Message */
	msg("You cannot destroy %s.", o_name);

	/* Hack -- inscribe the artifact, if not identified. */
	if (!object_known_p(o_ptr))
	    o_ptr->feel = feel;

	/* We have "felt" it (again) */
	o_ptr->ident |= (IDENT_SENSE);

	/* Combine the pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

	/* Done */
	return;
    }

    /* Message */
    msg("You destroy %s.", o_name);

    /* Reduce the charges of rods/wands */
    reduce_charges(o_ptr, amt);

    /* Eliminate the item (from the pack) */
    if (item >= 0) {
	inven_item_increase(item, -amt);
	inven_item_describe(item);
	inven_item_optimize(item);
    }

    /* Eliminate the item (from the floor) */
    else {
	floor_item_increase(0 - item, -amt);
	floor_item_describe(0 - item);
	floor_item_optimize(0 - item);
    }
}

void textui_cmd_destroy_menu(int item)
{
    int amt, type, sq_val;

    object_type *o_ptr;
    object_type obj_to_destroy;
    object_kind *k_ptr;
    ego_item_type *e_ptr;

    char o_name[120];
    char out_val[160];

    menu_type *m;
    region r;
    int selected;

    bool fullid;
    bool sensed;
    byte feel;
    bool heavy = (player_has(PF_PSEUDO_ID_HEAVY));

    /* Deal with squelched items */
    if (item == ALL_SQUELCHED) {
	cmd_insert(CMD_DESTROY);
	cmd_set_arg_item(cmd_get_top(), 0, item);
	return;
    }

    /* Get all the object info */
    o_ptr = object_from_item_idx(item);
    k_ptr = &k_info[o_ptr->k_idx];
    if (!k_ptr)
	return;
    fullid = object_known_p(o_ptr);
    sensed = (o_ptr->ident & IDENT_SENSE) || fullid;
    feel   = fullid ? value_check_aux1((object_type *)o_ptr) : o_ptr->feel;
    
    /* Ask if player would prefer squelching instead of destruction */
    
    /* Get a quantity */
    amt = get_quantity(NULL, o_ptr->number);
    if (amt <= 0)
	return;
    
    /* Describe the destroyed object by taking a copy with the right "amt" */
    object_copy_amt(&obj_to_destroy, o_ptr, amt);
    object_desc(o_name, sizeof(o_name), &obj_to_destroy,
		ODESC_PREFIX | ODESC_FULL);
    
    m = menu_dynamic_new();
    m->selections = lower_case;
    
    /* Basic ignore option */
    menu_dynamic_add(m, "Destroy this item only", DESTROY_THIS_ITEM);
    
    /* Flavour-aware squelch */
    if (squelch_tval(k_info[o_ptr->k_idx].tval) && !(k_ptr->squelch)
	&& (k_ptr->flavor == 0 || k_ptr->aware))
    {
	char tmp[70];

	object_desc(tmp, sizeof(tmp), o_ptr, ODESC_BASE | ODESC_PLURAL);
	strnfmt(out_val, sizeof(out_val), "Squelch all %s", tmp);
	menu_dynamic_add(m, out_val, DESTROY_THIS_FLAVOR);
    }
    else
    {    
	/* Quality squelching */
	type = squelch_type_of(o_ptr);

	for (sq_val = SQUELCH_NONE + 1; sq_val < SQUELCH_MAX; sq_val++)
	{
	    if (!sensed) break;
	    if (heavy && ((sq_val == SQUELCH_FELT_DUBIOUS) || 
			  (sq_val == SQUELCH_FELT_GOOD)))
		break;
	    if (feel_to_squelch_level(feel) != sq_val)
		continue;
	    
	    strnfmt(out_val, sizeof(out_val), "Squelch %s %s", 
		    quality_strings[sq_val].adjective, 
		    quality_choices[type].desc);
	    menu_dynamic_add(m, out_val, sq_val);
	}
	    
	/* Ego squelching */
	if (has_ego_properties(o_ptr))
	{
	    ego_desc choice;
	    char buf[80] = "";
	    
	    e_ptr = &e_info[o_ptr->name2];
	    choice.e_idx = e_ptr->eidx;
	    choice.short_name = "";
	    (void) ego_item_name(buf, sizeof(buf), &choice);
	    strnfmt(out_val, sizeof out_val, "Squelch all %s", buf + 4);
	    menu_dynamic_add(m, out_val, DESTROY_THIS_EGO);
	}
    }
	

    /* work out display region */
    r.width = menu_dynamic_longest_entry(m) + 3 + 2; /* +3 for tag, 2 for pad */
    r.col = 80 - r.width;
    r.row = 1;
    r.page_rows = m->count;

    screen_save();
    menu_layout(m, &r);
    region_erase_bordered(&r);

    prt("Enter to select, ESC:", 0, 0);
    selected = menu_dynamic_select(m);

    screen_load();

    if (selected == DESTROY_THIS_ITEM) 
    {
	cmd_insert(CMD_DESTROY);
	cmd_set_arg_item(cmd_get_top(), 0, item);
	cmd_set_arg_number(cmd_get_top(), 1, amt);
    } 
    else if (selected == DESTROY_THIS_FLAVOR) 
    {
	k_ptr->squelch = !k_ptr->squelch;
    } 
    else if (selected == DESTROY_THIS_EGO) 
    {
	e_ptr->squelch = !e_ptr->squelch;
    }
    else
    {
	for (sq_val = SQUELCH_NONE + 1; sq_val < SQUELCH_MAX; sq_val++)
	{
	    if (selected == sq_val)
		squelch_profile[type][sq_val] = TRUE;
	}
    }
    
    p_ptr->notice |= PN_SQUELCH;

    menu_dynamic_free(m);
}

void textui_cmd_destroy(void)
{
    int item;

    /* Get an item */
    const char *q = "Destroy which item? ";
    const char *s = "You have nothing to destroy.";
    if (!get_item(&item, q, s, CMD_DESTROY, 
		  (USE_INVEN | USE_EQUIP | USE_FLOOR | CAN_SQUELCH)))
	return;

    textui_cmd_destroy_menu(item);
}

/**
 * Target command
 */
void do_cmd_target(void)
{
    /* Target set */
    if (target_set_interactive(TARGET_KILL, -1, -1)) {
	msg("Target Selected.");
    }

    /* Target aborted */
    else {
	msg("Target Aborted.");
    }
}

/**
 * Target closest command
 */
void do_cmd_target_closest(void)
{
    target_set_closest(TARGET_KILL);
}



/**
 * Look command
 */
void do_cmd_look(void)
{
    /* Look around */
    if (target_set_interactive(TARGET_LOOK, -1, -1)) {
	msg("Target Selected.");
    }
}



/**
 * Allow the player to examine other sectors on the map
 */
void do_cmd_locate(void)
{
    int dir, y1, x1, y2, x2;
    char tmp_val[80];
    char out_val[160];


    /* Start at current panel */
    y2 = y1 = Term->offset_y;
    x2 = x1 = Term->offset_x;

    /* Show panels until done */
    while (1) {
	/* Describe the location */
	if ((y2 == y1) && (x2 == x1)) {
	    tmp_val[0] = '\0';
	} 
	else 
	{
	    sprintf(tmp_val, "%s%s of",
		    ((y2 < y1) ? " North" : (y2 > y1) ? " South" : ""),
		    ((x2 < x1) ? " West" : (x2 > x1) ? " East" : ""));
	}

	/* Prepare to ask which way to look */
	sprintf(out_val,
		"Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction or ESC?",
		    y2 / (SCREEN_HGT / 2), y2 % (SCREEN_HGT / 2),
		    x2 / (SCREEN_WID / 2), x2 % (SCREEN_WID / 2), tmp_val);

	/* More detail */
	if (OPT(center_player))
	{
	    strnfmt(out_val, sizeof(out_val),
		    "Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction?",
		    (y2 / PANEL_HGT), (y2 % PANEL_HGT),
		    (x2 / PANEL_WID), (x2 % PANEL_WID), tmp_val);
	}

	/* Assume no direction */
	dir = 0;

	/* Get a direction */
	while (!dir) {
	    struct keypress command;

	    /* Get a command (or Cancel) */
	    if (!get_com(out_val, &command)) break;

	    /* Extract direction */
	    dir = target_dir(command);

	    /* Error */
	    if (!dir) bell("Illegal direction for locate!");
	}

	/* No direction */
	if (!dir)
	    break;

	/* Apply the motion */
	change_panel(dir);

	/* Handle stuff */
	handle_stuff(p_ptr);
    }


    /* Recenter the map around the player */
    verify_panel();
}



/**
 * The table of "symbol info" -- each entry is a string of the form
 * "X:desc" where "X" is the trigger, and "desc" is the "info".
 */
static const char *ident_info[] = {
    " :A dark grid",
    "!:A potion (or oil)",
    "\":An amulet (or necklace)",
    "#:A wall (or secret door)",
    "$:Treasure (gold or gems)",
    "%:A vein (magma or quartz)",
    /* "&:unused", */
    "':An open door",
    "(:Soft armor",
    "):A shield",
    "*:A vein with treasure",
    "+:A closed door",
    ",:Food (or mushroom patch)",
    "-:A wand (or rod)",
    ".:Floor",
    "/:A polearm (Axe/Pike/etc)",
    /* "0:unused", */
    "1:Entrance to General Store",
    "2:Entrance to Armory",
    "3:Entrance to Weaponsmith",
    "4:Entrance to Temple",
    "5:Entrance to Alchemy shop",
    "6:Entrance to Magic store",
    "7:Entrance to Black Market",
    "8:Entrance to your home",
    "9:Entrance to Bookseller",
    "::Rubble",
    ";:A glyph of warding",
    "<:An up staircase",
    "=:A ring",
    ">:A down staircase",
    "?:A scroll",
    "@:You",
    /* "A:unused", */
    "B:Bird",
    "C:Canine",
    "D:Flying Dragon",
    "E:Elemental",
    "F:Dragon Fly",
    "G:Ghost",
    "H:Hybrid",
    "I:Insect",
    "J:Snake",
    "K:Killer Beetle",
    "L:Lich",
    "M:Mummy",
    /* "N:unused", */
    "O:Ogre",
    "P:Giant Humanoid",
    "Q:Quylthulg (Pulsing Flesh Mound)",
    "R:Reptile/Amphibian",
    "S:Spider/Scorpion/Tick",
    "T:Troll",
    "U:Major Demon",
    "V:Vampire",
    "W:Wight/Wraith/etc",
    "X:Xorn/Xaren/etc",
    "Y:Yeti",
    "Z:Zephyr Hound",
    "[:Hard armor",
    "\\:A hafted weapon (mace/whip/etc)",
    "]:Misc. armor",
    "^:A trap",
    "_:A staff",
    "`:A tool or junk",
    "a:Ant",
    "b:Bat",
    "c:Centipede",
    "d:Dragon",
    "e:Floating Eye",
    "f:Feline",
    "g:Golem",
    "h:Hobbit/Elf/Dwarf",
    "i:Icky Thing",
    "j:Jelly",
    "k:Kobold",
    "l:Louse",
    "m:Mold",
    "n:Naga",
    "o:Orc",
    "p:Person/Human",
    "q:Quadruped",
    "r:Rodent",
    "s:Skeleton",
    "t:Townsperson",
    "u:Minor Demon",
    "v:Vortex",
    "w:Worm/Worm-Mass",
    /* "x:unused", */
    "y:Yeek",
    "z:Zombie/Mummy",
    "{:A missile (arrow/bolt/shot)",
    "|:An edged weapon (sword/dagger/etc)",
    "}:A launcher (bow/crossbow/sling)",
    "~:A chest or light",
    NULL
};




static int cmp_mexp(const void *a, const void *b)
{
	u16b ia = *(const u16b *)a;
	u16b ib = *(const u16b *)b;
	if (r_info[ia].mexp < r_info[ib].mexp)
		return -1;
	if (r_info[ia].mexp > r_info[ib].mexp)
		return 1;
	return (a < b ? -1 : (a > b ? 1 : 0));
}

static int cmp_level(const void *a, const void *b)
{
	u16b ia = *(const u16b *)a;
	u16b ib = *(const u16b *)b;
	if (r_info[ia].level < r_info[ib].level)
		return -1;
	if (r_info[ia].level > r_info[ib].level)
		return 1;
	return cmp_mexp(a, b);
}

static int cmp_tkill(const void *a, const void *b)
{
	u16b ia = *(const u16b *)a;
	u16b ib = *(const u16b *)b;
	if (l_list[ia].tkills < l_list[ib].tkills)
		return -1;
	if (l_list[ia].tkills > l_list[ib].tkills)
		return 1;
	return cmp_level(a, b);
}

static int cmp_pkill(const void *a, const void *b)
{
	u16b ia = *(const u16b *)a;
	u16b ib = *(const u16b *)b;
	if (l_list[ia].pkills < l_list[ib].pkills)
		return -1;
	if (l_list[ia].pkills > l_list[ib].pkills)
		return 1;
	return cmp_tkill(a, b);
}

int cmp_monsters(const void *a, const void *b)
{
	return cmp_level(a, b);
}

/**
 * Identify a character, allow recall of monsters
 *
 * Several "special" responses recall "mulitple" monsters:
 *   ^A (all monsters)
 *   ^U (all unique monsters)
 *   ^N (all non-unique monsters)
 *
 * The responses may be sorted in several ways, see below.
 *
 */
void do_cmd_query_symbol(void)
{
    int i, n, r_idx;
	struct keypress sym;
    ui_event query;
    char search_str[60] = "";
    char monster_name[80];
    char buf[128];
    char *sp;

    bool all = FALSE;
    bool uniq = FALSE;
    bool norm = FALSE;

    bool recall = FALSE;

    u16b *who;

    /* Get a character, or abort */
    if (!get_com("Enter character to be identified: ", &sym)) 
	return;
 
    /* Find that character info, and describe it */
    for (i = 0; ident_info[i]; ++i) {
	if (sym.code == (unsigned char) ident_info[i][0])
	    break;
    }

    /* Describe */
    if (sym.code == KTRL('A')) {
	all = TRUE;
	strcpy(buf, "Full monster list.");
    } else if (sym.code == KTRL('U')) {
	all = uniq = TRUE;
	strcpy(buf, "Unique monster list.");
    } else if (sym.code == KTRL('N')) {
	all = norm = TRUE;
	strcpy(buf, "Non-unique monster list.");
    } else if (sym.code == KTRL('F'))
    {
	if (!get_string("Substring to search: ", search_str, 
			sizeof(search_str)))
	{
	    return;
	}
      
	for(sp = search_str; *sp; sp++) *sp = tolower(*sp);
      
	sprintf(buf, "Monsters matching '%s'", search_str);
	all = FALSE;
    } else if (ident_info[i]) {
	sprintf(buf, "%c - %s.", sym.code, ident_info[i] + 2);
    } else {
	sprintf(buf, "%c - %s.", sym.code, "Unknown Symbol");
    }

    /* Display the result */
    prt(buf, 0, 0);

    /* Allocate the "who" array */
    who = C_ZNEW(z_info->r_max, u16b);

    /* Collect matching monsters */
    for (n = 0, i = 1; i < z_info->r_max; i++) {
	monster_race *r_ptr = &r_info[i];
	monster_lore *l_ptr = &l_list[i];

	/* Nothing to recall Even cheat_know does not allow viewing
	 * non-existant player ghosts. */
	if ((!OPT(cheat_know) || (rf_has(r_ptr->flags, RF_PLAYER_GHOST)))
	    && !l_ptr->sights)
	    continue;

	/* Require non-unique monsters if needed */
	if (norm && (rf_has(r_ptr->flags, RF_UNIQUE)))
	    continue;

	/* Require unique monsters if needed */
	if (uniq && !(rf_has(r_ptr->flags, RF_UNIQUE)))
	    continue;

	/* Collect "appropriate" monsters */
	if (*search_str) {
	    strncpy(monster_name, r_ptr->name, sizeof monster_name);
	    for (sp = monster_name; *sp; sp++)
		*sp = tolower(*sp);
	    if (strstr(monster_name, search_str))
		who[n++] = i;
	    continue;
	}
	if (all || ((unsigned char) r_ptr->d_char == sym.code))
	    who[n++] = i;
    }

    /* Nothing to recall */
    if (!n) {
	FREE(who);
	return;
    }


    /* Buttons */
    button_add("[y]", 'y');
    button_add("[k]", 'k');
    /* Don't collide with the repeat button */
    button_add("[n]", 'q'); 
    redraw_stuff(p_ptr);

    /* Prompt */
    put_str("Recall details? (y/k/n): ", 0, 40);

    /* Query */
    query = inkey_ex();

    /* Restore */
    prt(buf, 0, 0);

    /* Buttons */
    button_kill('y');
    button_kill('k');
    button_kill('q');
    redraw_stuff(p_ptr);

    /* Interpret the response */
    if (query.key.code == 'k')
    {
	/* Sort by kills (and level) */
	sort(who, n, sizeof(*who), cmp_pkill);
    }
    else if (query.key.code == 'y' || query.key.code == 'p')
    {
	/* Sort by level; accept 'p' as legacy */
	sort(who, n, sizeof(*who), cmp_level);
    }
    else
    {
	/* Any unsupported response is "nope, no history please" */
	
	/* XXX XXX Free the "who" array */
	FREE(who);

	return;
    }

    /* Start at the end */
    i = n - 1;

    /* Button */
    button_add("[r]", 'r');
    button_add("[-]", '-');
    button_add("[+]", '+');
    redraw_stuff(p_ptr);

    /* Scan the monster memory */
    while (TRUE) {
	/* Extract a race */
	r_idx = who[i];

	/* Hack -- Auto-recall */
	monster_race_track(r_idx);

	/* Hack -- Handle stuff */
	handle_stuff(p_ptr);

	/* Hack -- Begin the prompt */
	roff_top(r_idx);

	/* Hack -- Complete the prompt */
	Term_addstr(-1, TERM_WHITE, "[(r)ecall, ESC]");
	prt(format("(#%d of %d)", i + 1, n), 0, 65);

	/* Interact */
	while (TRUE) {
	    /* Recall */
	    if (recall) {
		/* Save screen */
		screen_save();

		/* Recall on screen */
		screen_roff(who[i]);

		/* Hack -- Complete the prompt (again) */
		Term_addstr(-1, TERM_WHITE, "[(r)ecall, ESC]");
		prt(format("(#%d of %d)", i + 1, n), 0, 65);

	    }

	    button_add("-", '-');

	    /* Command */
	    query = inkey_ex();

	    /* Unrecall */
	    if (recall) {
		/* Load screen */
		screen_load();
	    }

	    /* Normal commands */
	    if (query.key.code != 'r')
		break;

	    /* Toggle recall */
	    recall = !recall;
	}

	/* Stop scanning */
	if (query.key.code == ESCAPE)
	    break;

	/* Move to "prev" monster */
	if (query.key.code == '-') {
	    if (i-- == 0) {
		i = n - 1;
	    }
	}

	/* Move to "next" monster */
	else {
	    if (++i == n) {
		i = 0;
	    }
	}
    }


    /* Button */
    button_kill('r');
    button_kill('-');
    button_kill('+');
    redraw_stuff(p_ptr);

    /* Re-display the identity */
    prt(buf, 0, 0);

    /* Free the "who" array */
    FREE(who);
}

/* Centers the map on the player */
void do_cmd_center_map(void)
{
	center_panel();
}


