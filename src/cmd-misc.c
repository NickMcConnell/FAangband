/** \file cmd-misc.c 
    \brief Miscellaneous Commands

 * Code for miscellaneous commands.
 *
 * Copyright (c) 2011 Nick McConnell
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
#include "monster.h"
#include "spells.h"
#include "trap.h"
#include "ui-menu.h"

/**
 * Return the features around (or under) the character
 */
void get_feats(int *surroundings)
{
    int d;
    int xx, yy;

    /* Check around (and under) the character */
    for (d = 0; d < 9; d++) 
    {
	/* Initialise */
	surroundings[d] = FEAT_FLOOR;

	/* Extract adjacent (legal) location */
	yy = p_ptr->py + ddy_ddd[d];
	xx = p_ptr->px + ddx_ddd[d];

	/* Paranoia */
	if (!in_bounds_fully(yy, xx))
	    continue;

	/* Must have knowledge */
	if (!sqinfo_has(cave_info[yy][xx], SQUARE_MARK))
	    continue;

	/* Record the feature */
	surroundings[d] = cave_feat[yy][xx];
    }

    /* All done */
    return;
}


/**
* Menu functions
*/
char comm[22];
cmd_code comm_code[22];
const char *comm_descr[22];
int poss, item;

/**
* Item tag/command key
*/
static char show_tag(menu_type * menu, int oid)
{
    /* Caution - could be a problem here if KTRL commands were used */
    return comm[oid];
}

/**
* Display an entry on a command menu
*/
static void show_display(menu_type * menu, int oid, bool cursor, int row,
			 int col, int width)
{
    byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);

    /* Write the description */
    Term_putstr(col + 4, row, -1, attr, comm_descr[oid]);
}


/**
* Handle user input from a command menu
*/
static bool show_action(menu_type * menu, const ui_event * e, int oid)
{
    /* Handle enter and mouse */
    if (e->type == EVT_SELECT) 
    {
	cmd_insert(comm_code[oid]);
	if (comm_code[oid] == CMD_NULL)
	    Term_keypress(comm[oid],0);
	cmd_set_arg_item(cmd_get_top(), 0, item);
    }
    return FALSE;
}

/**
* Display a list of commands.
*/
bool show_cmd_menu(bool object)
{
    menu_type menu;
    menu_iter commands_menu = { show_tag, 0, show_display, show_action, 0 };
    region area = { 15, (object ? 2 : 1), 20, 0 };

    ui_event evt = EVENT_EMPTY;
    int cursor = 0;

    /* Size of menu */
    area.page_rows = poss + (object ? 1 : 0);

    /* Set up the menu */
    WIPE(&menu, menu);
    menu_init(&menu, MN_SKIN_SCROLL, &commands_menu);
    menu.cmd_keys = "\x8B\x8C\n\r";
    area.page_rows = poss + (object ? 1 : 0);
    menu_setpriv(&menu, poss, comm);
    menu_layout(&menu, &area);

    /* Select an entry */
    evt = menu_select(&menu, cursor, TRUE);

    return (evt.type != EVT_ESCAPE);
}

/**
 * Bring up player actions 
 */
void show_player(void)
{
    int i, j, fy, fx;
    int adj_grid[9];
    bool exist_rock = FALSE;
    bool exist_door = FALSE;
    bool exist_open_door = FALSE;
    bool exist_trap = FALSE;
    bool exist_mtrap = FALSE;
    bool exist_floor = FALSE;
    bool exist_monster = FALSE;

    feature_type *f_ptr;

    /* No commands yet */
    poss = 0;

    /* Get surroundings */
    get_feats(adj_grid);

    /* Analyze surroundings */
    for (i = 0; i < 8; i++) 
    {
	int yy = p_ptr->py + ddy_ddd[i];
	int xx = p_ptr->px + ddx_ddd[i];
	f_ptr = &f_info[adj_grid[i]];

	if (cave_visible_trap(yy, xx))
	    exist_trap = TRUE;
	if (tf_has(f_ptr->flags, TF_DOOR_CLOSED))
	    exist_door = TRUE;
	if (tf_has(f_ptr->flags, TF_ROCK))
	    exist_rock = TRUE;
	if (cave_monster_trap(yy, xx))
	    exist_mtrap = TRUE;
	if (adj_grid[i] == FEAT_OPEN)
	    exist_open_door = TRUE;
	if (tf_has(f_ptr->flags, TF_MTRAP))
	    exist_floor = TRUE;
	if (cave_m_idx[yy][xx] > 0)
	    exist_monster = TRUE;
    }
    f_ptr = &f_info[adj_grid[8]];

    /* In a web? */
    if (cave_web(p_ptr->py, p_ptr->px))
	exist_trap = TRUE;

    /* Alter a grid */
    if (exist_trap || exist_door || exist_rock || exist_mtrap
	|| exist_open_door || count_chests(&fy, &fx, TRUE)
	|| count_chests(&fy, &fx, FALSE) || (player_has(PF_TRAP)
					     && exist_floor)
	|| (player_has(PF_STEAL) && exist_monster && (!SCHANGE))) {
	comm[poss] = '+';
	comm_code[poss] = CMD_ALTER;
	comm_descr[poss++] = "Alter";
    }

    /* Dig a tunnel */
    if (exist_door || exist_rock) {
	comm[poss] = 'T';
	comm_code[poss] = CMD_TUNNEL;
	comm_descr[poss++] = "Tunnel";
    }

    /* Begin Running -- Arg is Max Distance */
    {
	comm[poss] = '.';
	comm_code[poss] = CMD_RUN;
	comm_descr[poss++] = "Run";
    }

    /* Hold still for a turn.  Pickup objects if auto-pickup is true. */
    {
	comm[poss] = ',';
	comm_code[poss] = CMD_HOLD;
	comm_descr[poss++] = "Stand still";
    }

    /* Pick up objects. */
    if (cave_o_idx[p_ptr->py][p_ptr->px]) {
	comm[poss] = 'g';
	comm_code[poss] = CMD_PICKUP;
	comm_descr[poss++] = "Pick up";
    }

    /* Rest -- Arg is time */
    {
	comm[poss] = 'R';
	comm_code[poss] = CMD_REST;
	comm_descr[poss++] = "Rest";
    }

    /* Search for traps/doors */
    {
	comm[poss] = 's';
	comm_code[poss] = CMD_SEARCH;
	comm_descr[poss++] = "Search";
    }

    /* Look around */
    {
	comm[poss] = 'l';
	comm_code[poss] = CMD_NULL;
	comm_descr[poss++] = "Look";
    }

    /* Scroll the map */
    {
	comm[poss] = 'L';
	comm_code[poss] = CMD_NULL;
	comm_descr[poss++] = "Scroll map";
    }

    /* Show the map */
    {
	comm[poss] = 'M';
	comm_code[poss] = CMD_NULL;
	comm_descr[poss++] = "Level map";
    }

    /* Knowledge */
    {
	comm[poss] = '~';
	comm_code[poss] = CMD_NULL;
	comm_descr[poss++] = "Knowledge";
    }

    /* Options */
    {
	comm[poss] = '=';
	comm_code[poss] = CMD_NULL;
	comm_descr[poss++] = "Options";
    }

    /* Toggle search mode */
    {
	comm[poss] = 'S';
	comm_code[poss] = CMD_TOGGLE_SEARCH;
	comm_descr[poss++] = "Toggle searching";
    }


    /* Go up staircase */
    if ((adj_grid[8] == FEAT_LESS)
	|| (tf_has(f_ptr->flags, TF_PATH) &&
	    tf_has(f_ptr->flags, TF_UPSTAIR)))
    {
	comm[poss] = '<';
	comm_code[poss] = CMD_GO_UP;
	comm_descr[poss++] = "Take stair/path";
    }

    /* Go down staircase */
    if ((adj_grid[8] == FEAT_MORE)
	|| (tf_has(f_ptr->flags, TF_PATH) &&
	    tf_has(f_ptr->flags, TF_DOWNSTAIR)))
    {
	comm[poss] = '>';
	comm_code[poss] = CMD_GO_DOWN;
	comm_descr[poss++] = "Take stair/path";
    }

    /* Open a door or chest */
    if (exist_door || count_chests(&fy, &fx, TRUE)
	|| count_chests(&fy, &fx, FALSE)) {
	comm[poss] = 'o';
	comm_code[poss] = CMD_OPEN;
	comm_descr[poss++] = "Open";
    }

    /* Close a door */
    if (exist_open_door) {
	comm[poss] = 'c';
	comm_code[poss] = CMD_CLOSE;
	comm_descr[poss++] = "Close";
    }

    /* Jam a door with spikes */
    if (exist_door) {
	comm[poss] = 'j';
	comm_code[poss] = CMD_JAM;
	comm_descr[poss++] = "Jam";
    }

    /* Bash a door */
    if (exist_door) {
	comm[poss] = 'B';
	comm_code[poss] = CMD_BASH;
	comm_descr[poss++] = "Bash";
    }

    /* Disarm a trap or chest */
    if (count_chests(&fy, &fx, TRUE) || exist_trap || exist_mtrap) {
	comm[poss] = 'D';
	comm_code[poss] = CMD_DISARM;
	comm_descr[poss++] = "Disarm";
    }

    /* Shapechange */
    if ((SCHANGE) || (player_has(PF_BEARSKIN))) {
	comm[poss] = ']';
	comm_code[poss] = CMD_NULL;
	comm_descr[poss++] = "Shapechange";
    }

    /* Save screen */
    screen_save();

    /* Prompt */
    put_str("Choose a command, or ESC:", 0, 0);

    /* Hack - delete exact graphics rows */
    if (tile_height > 1) {
	j = poss + 1;
	while ((j % tile_height) && (j <= SCREEN_ROWS))
	    prt("", ++j, 0);
    }

    /* Get a choice */
    (void) show_cmd_menu(FALSE);

    /* Load screen */
    screen_load();
}


/**
 * Bring up objects to act on 
 */
void do_cmd_show_obj(void)
{
    const char *q, *s;
    int j;
    object_type *o_ptr;

    char o_name[120];
    byte out_color;
    bool accepted = FALSE;

    /* No restrictions */
    item_tester_tval = 0;
    item_tester_hook = NULL;

    /* See what's available */
    q = "Pick an item to use:";
    s = "You have no items to hand.";
    if (!get_item(&item, q, s, CMD_NULL, (USE_INVEN | USE_EQUIP | USE_FLOOR)))
	return;

    /* Got it */
    if (item >= 0) {
	o_ptr = &p_ptr->inventory[item];
    }

    /* Get the item (on the floor) */
    else {
	o_ptr = &o_list[0 - item];
    }

    /* Is it really an item? */
    if (!o_ptr->k_idx)
	return;

    /* No commands yet */
    poss = 0;

    /* Describe the object */
    object_desc(o_name, sizeof(o_name), o_ptr, ODESC_FULL);

    /* Hack -- enforce max length */
    o_name[Term->wid - 3] = '\0';

    /* Acquire inventory color.  Apply spellbook hack. */
    out_color = proc_list_color_hack(o_ptr);

    /* Wear/wield */
    if ((wield_slot(o_ptr) >= INVEN_WIELD) && (!SCHANGE)
	&& (item < INVEN_WIELD)) {
	comm[poss] = 'w';
	comm_code[poss] = CMD_WIELD;
	comm_descr[poss++] = "Wield";
    }

    /* Take off equipment */
    if ((item >= INVEN_WIELD) && (item < INVEN_TOTAL) && (!SCHANGE)) {
	comm[poss] = 't';
	comm_code[poss] = CMD_TAKEOFF;
	comm_descr[poss++] = "Take off";
    }

    /* Drop an item */
    if ((item >= 0) && ((item < INVEN_WIELD) || (!SCHANGE))) {
	comm[poss] = 'd';
	comm_code[poss] = CMD_DROP;
	comm_descr[poss++] = "Drop";
    }


    /* Destroy an item */
    if (item < INVEN_WIELD) {
	comm[poss] = 'k';
	comm_code[poss] = CMD_DESTROY;
	comm_descr[poss++] = "Destroy";
    }


    /* Identify an object */
    comm[poss] = 'I';
    comm_code[poss] = CMD_NULL;
    comm_descr[poss++] = "Inspect";

    /* Pick up an object */
    if ((item < 0) && inven_carry_okay(o_ptr)) {
	comm[poss] = 'g';
	comm_code[poss] = CMD_PICKUP;
	comm_descr[poss++] = "Pick up";
    }

    /* Book learnin' */
    if (mp_ptr->spell_book == o_ptr->tval) {
	if (p_ptr->new_spells) {
	    comm[poss] = 'G';
	    comm_code[poss] = CMD_STUDY_SPELL;
	    comm_descr[poss++] = "Gain a spell";
	}
	comm[poss] = 'b';
	comm_code[poss] = CMD_BROWSE_SPELL;
	comm_descr[poss++] = "Browse";
	comm[poss] = 'm';
	comm_code[poss] = CMD_CAST;
	comm_descr[poss++] = "Cast a spell";
    }

    /* Inscribe an object */
    comm[poss] = '{';
    comm_code[poss] = CMD_INSCRIBE;
    comm_descr[poss++] = "Inscribe";

    /* Uninscribe an object */
    if (o_ptr->note) {
	comm[poss] = '}';
	comm_code[poss] = CMD_UNINSCRIBE;
	comm_descr[poss++] = "Uninscribe";
    }

    /* Activate equipment */
    if ((o_ptr->effect) && (item >= INVEN_WIELD)) {
	comm[poss] = 'A';
	comm_code[poss] = CMD_ACTIVATE;
	comm_descr[poss++] = "Activate";
    }

    /* Eat some food */
    if (o_ptr->tval == TV_FOOD) {
	comm[poss] = 'E';
	comm_code[poss] = CMD_EAT;
	comm_descr[poss++] = "Eat";
    }

    if ((item < INVEN_WIELD)
	&& ((o_ptr->tval == TV_LIGHT) || (o_ptr->tval == TV_FLASK))) {
	object_type *o1_ptr = &p_ptr->inventory[INVEN_LIGHT];

	if (((o1_ptr->sval == SV_LIGHT_LANTERN)
	     && ((o_ptr->tval == TV_FLASK)
		 || ((o_ptr->tval == TV_LIGHT)
		     && (o_ptr->sval == SV_LIGHT_LANTERN))))
	    || ((o1_ptr->sval == SV_LIGHT_TORCH) && (o_ptr->tval == TV_LIGHT)
		&& (o_ptr->sval == SV_LIGHT_TORCH)))
	{
	    comm[poss] = 'F';
	    comm_code[poss] = CMD_REFILL;
	    comm_descr[poss++] = "Refuel";
	}
    }

    /* Fire an item */
    if (p_ptr->state.ammo_tval == o_ptr->tval) {
	comm[poss] = 'f';
	comm_code[poss] = CMD_FIRE;
	comm_descr[poss++] = "Fire";
    }

    /* Throw an item */
    if ((item < INVEN_WIELD) || (!SCHANGE)) {
	comm[poss] = 'v';
	comm_code[poss] = CMD_THROW;
	comm_descr[poss++] = "Throw";
    }

    /* Aim a wand */
    if (o_ptr->tval == TV_WAND) {
	comm[poss] = 'a';
	comm_code[poss] = CMD_USE_WAND;
	comm_descr[poss++] = "Aim";
    }

    /* Zap a rod */
    if (o_ptr->tval == TV_ROD) {
	comm[poss] = 'z';
	comm_code[poss] = CMD_USE_ROD;
	comm_descr[poss++] = "Zap";
    }

    /* Quaff a potion */
    if (o_ptr->tval == TV_POTION) {
	comm[poss] = 'q';
	comm_code[poss] = CMD_QUAFF;
	comm_descr[poss++] = "Quaff";
    }

    /* Read a scroll */
    if (o_ptr->tval == TV_SCROLL) {
	comm[poss] = 'r';
	comm_code[poss] = CMD_READ_SCROLL;
	comm_descr[poss++] = "Read";
    }

    /* Use a staff */
    if (o_ptr->tval == TV_STAFF) {
	comm[poss] = 'u';
	comm_code[poss] = CMD_USE_STAFF;
	comm_descr[poss++] = "Use";
    }

    /* Set up the screen */
    screen_save();

    /* Prompt */
    put_str("Choose a command, or ESC:", 0, 0);

    /* Clear the line */
    prt("", 1, 0);

    /* Display the item */
    c_put_str(out_color, o_name, 1, 0);

    /* Hack - delete exact graphics rows */
    if (tile_height > 1) {
	j = poss + 2;
	while ((j % tile_height) && (j <= SCREEN_ROWS))
	    prt("", ++j, 0);
    }

    /* Get a choice */
    accepted = show_cmd_menu(TRUE);

    /* Load de screen */
    screen_load();

    /* Now set the item if valid */
    if (accepted)
	cmd_set_arg_item(cmd_get_top(), 0, item);
}



/**
 * Verify use of "wizard" mode
 */
static bool enter_wizard_mode(void)
{
    /* Ask first time */
    if (!(p_ptr->noscore & 0x0002)) {
	/* Mention effects */
	msg
	    ("You are about to enter 'wizard' mode for the very first time!");
	msg
	    ("This is a form of cheating, and your game will not be scored!");
	message_flush();

	/* Verify request */
	if (!get_check("Are you sure you want to enter wizard mode? ")) {
	    return (FALSE);
	}

	/* Mark savefile */
	p_ptr->noscore |= 0x0002;
    }

    /* Success */
    return (TRUE);
}



/**
 * Toggle wizard mode
 */
extern void do_cmd_wizard(void)
{
    if (enter_wizard_mode()) {
	/* Toggle mode */
	if (p_ptr->wizard) {
	    p_ptr->wizard = FALSE;
	    msg("Wizard mode off.");
	} else {
	    p_ptr->wizard = TRUE;
	    msg("Wizard mode on.");
	}

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw "title" */
	p_ptr->redraw |= (PR_TITLE);
    }
}




#ifdef ALLOW_DEBUG

/**
 * Verify use of "debug" mode
 */
static bool verify_debug_mode(void)
{
    static int verify = 1;

    /* Ask first time, unless the savefile is already in debug mode. */
    if (verify && (!(p_ptr->noscore & 0x0008))) {
	/* Mention effects */
	msg
	    ("You are about to use the dangerous, unsupported, debug commands!");
	msg
	    ("Your machine may crash, and your savefile may become corrupted!");
	msg("Using the debug commands will also mark your savefile.");
	message_flush();

	/* Verify request */
	if (!get_check("Are you sure you want to use the debug commands? ")) {
	    return (FALSE);
	}
    }

    /* Verified */
    verify = 0;

    /* Mark savefile */
    p_ptr->noscore |= 0x0008;

    /* Okay */
    return (TRUE);
}


/**
 * Verify use of "debug" mode
 */
extern void do_cmd_try_debug(void)
{
    /* Ask first time */
    if (verify_debug_mode()) {

	/* Okay */
	do_cmd_debug();
    }
}

#endif				/* ALLOW_DEBUG */



#ifdef ALLOW_BORG

/**
 * Verify use of "borg" mode
 */
extern bool do_cmd_try_borg(void)
{
    /* Ask first time */
    if (!(p_ptr->noscore & NOSCORE_BORG)) {
	/* Mention effects */
	msg
	    ("You are about to use the dangerous, unsupported, borg commands!");
	msg
	    ("Your machine may crash, and your savefile may become corrupted!");
	message_flush();

	/* Verify request */
	if (!get_check("Are you sure you want to use the borg commands? "))
	    return;

	/* Mark savefile */
	p_ptr->noscore |= NOSCORE_BORG;
    }

    /* Okay */
    do_cmd_borg();
}

#endif				/* ALLOW_BORG */




/**
 * Quit the game.
 */
extern void do_cmd_quit(cmd_code code, cmd_arg args[])
{
    /* Stop playing */
    p_ptr->playing = FALSE;

    /* Leaving */
    p_ptr->leaving = TRUE;
}


/**
 * Display the options and redraw afterward.
 */
extern void do_cmd_xxx_options(void)
{
    do_cmd_options();
    do_cmd_redraw();
}

void bear_shape(void)
{
    /* Sanity */
    if ((SCHANGE) || (!player_has(PF_BEARSKIN)))
	return;

    /* Confirm */
    if (!get_check("Assume the form of a bear? "))
	return;

    /* Change */
    shapechange(SHAPE_BEAR);

    /* Use some energy */
    p_ptr->energy_use = 100;
}


/**
 * Stop doing a shapechange.  From Sangband.
 */
void unchange(void)
{
    if (!SCHANGE) {
	msg("You aren't in another form right now.");
	return;
    }

    /* Confirm */
    if (!get_check("Really return to normal? "))
	return;

    /* Return to normal form */
    shapechange(SHAPE_NORMAL);

    /* Hack - refund mana (2/3 mana when shapeshifted). */
    if (p_ptr->csp > 0) {
	/* Hack - Recalculate mana now, even though we will */
	/* update it fully, to ensure the refund to current */
	/* mana doesn't get cleared */
	p_ptr->msp *= 3;
	p_ptr->msp /= 2;

	/* Refund current mana - removed, as mana is not reduced in the other
	 * shapechange p_ptr->csp *= 3; p_ptr->csp /= 2; if (p_ptr->csp >
	 * p_ptr->msp) p_ptr->csp = p_ptr->msp; */

	/* Display mana later */
	p_ptr->redraw |= (PR_MANA);
    }

    /* Recalculate mana. */
    p_ptr->update |= (PU_MANA);

    /* Show or hide shapechange on main window. */
    p_ptr->redraw |= (PR_SHAPE);

    /* Use some energy */
    p_ptr->energy_use = 100;
}


/**
 * Display the options and redraw afterward.
 */
extern void do_cmd_reshape(void)
{
    if ((!SCHANGE) && (player_has(PF_BEARSKIN))) {
	bear_shape();
    } else
	unchange();
}


/**
 * Display the main-screen monster list.
 */
extern void do_cmd_monlist(void)
{
    /* Save the screen and display the list */
    screen_save();
    display_monlist();

    /* Wait */
    inkey_ex();

    /* Return */
    screen_load();
}


/**
 * Display the main-screen monster list.
 */
extern void do_cmd_itemlist(void)
{
    /* Save the screen and display the list */
    screen_save();
    display_itemlist();

    /* Wait */
    inkey_ex();

    /* Return */
    screen_load();
}


/**
 * Invoked when the command isn't recognised.
 */
extern void do_cmd_unknown(void)
{
    prt("Type '?' for help.", 0, 0);
}
