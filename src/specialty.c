/** \file specialty.c 
    \brief Commands, part 5

    * Player specialties.
    *
    * Copyright (c) 1997-2011 Ben Harrison, James E. Wilson, Robert A. Koeneke,
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
#include "button.h"
#include "history.h"
#include "ui-menu.h"





/*** Code for dealing with specialty, race and class abilities ***/

/*
 * Entries for player ability descriptions
 */
typedef struct {
    u16b index;			/* Specialty index */
    const char *name;		/* Specialty name */
    const char *desc;		/* Specialty description */
    int type;                   /* Specialty type */
} ability;

/*
 * Read in the descriptions.
 */
static const ability abilities[] = {
#define PF(x, y, z, w)    { PF_##x, y, z, w }
#include "list-player-flags.h"
#undef PF
};


/**
 * Check if we can gain a specialty ability -BR-
 */
bool check_specialty_gain(int specialty)
{
    int i;
    bool allowed = FALSE;

    /* Can't learn it if we already know it */
    for (i = 0; i < MAX_SPECIALTIES; i++) {
	if (p_ptr->specialty_order[i] == specialty)
	    return (FALSE);
    }

    /* Is it allowed for this class? */
    if (pf_has(cp_ptr->specialties, specialty) &&
	(abilities[specialty].type == PLAYER_FLAG_SPECIAL))
	return (TRUE);

    return FALSE;
}

/**
 * Gain specialty code 
 */

static char gain_spec_tag(menu_type * menu, int oid)
{
    return I2A(oid);
}

/**
 * Display an entry on the gain specialty menu
 */
void gain_spec_display(menu_type * menu, int oid, bool cursor, int row, int col,
		       int width)
{
    const int *choices = menu->menu_data;
    int idx = choices[oid];
    int x, y;

    byte attr = (cursor ? TERM_L_GREEN : TERM_GREEN);

    /* Print it */
    c_put_str(attr, abilities[idx].name, row, col);

    /* Help text */
    if (cursor) {
	/* Locate the cursor */
	Term_locate(&x, &y);

	/* Move the cursor */
	Term_gotoxy(3, menu->count + 2);
	text_out_indent = 3;
	text_out_to_screen(TERM_L_BLUE, abilities[choices[oid]].desc);

	/* Restore */
	Term_gotoxy(x, y);
    }
}

/**
 * Deal with events on the gain specialty menu
 */
bool gain_spec_action(menu_type * menu, const ui_event_data * e, int oid)
{
    return TRUE;
}


/**
 * Display list available specialties.
 */
bool gain_spec_menu(int *pick)
{
    menu_type menu;
    menu_iter menu_f = { gain_spec_tag, 0, gain_spec_display,
	gain_spec_action, 0
    };
    ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
    int cursor = 0;

    region loc = { -60, 1, 60, -99 };

    int choices[255];
    int num = 0;
    bool done = FALSE;

    size_t i;

    char buf[80];

    /* Find the learnable specialties */
    for (num = 0, i = 0; i < PF_MAX; i++) {
	if (check_specialty_gain(i)) {
	    choices[num] = i;
	    num++;
	}
    }

    /* We are out of specialties - should never happen */
    if (!num) {
	msg_print("No specialties available.");
	screen_load();
	return FALSE;
    }

    /* Save the screen and clear it */
    screen_save();

    /* Prompt choices */
    sprintf(buf,
	    "(Specialties: %c-%c, ESC=exit) Gain which specialty (%d available)? ",
	    I2A(0), I2A(num - 1), p_ptr->new_specialties);

    /* Set up the menu */
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
    menu.title = buf;
    menu_setpriv(&menu, num, choices);
    menu_layout(&menu, &SCREEN_REGION);

    while (!done) {
	evt = menu_select(&menu, 0);
	done = (evt.type == EVT_ESCAPE);
	if (evt.type == EVT_SELECT)
	    done = get_check("Are you sure? ");
    }

    if (evt.type == EVT_SELECT)
	*pick = evt.index;

    /* Load screen */
    screen_load();

    return (evt.type != EVT_ESCAPE);
}

/**
 * Gain a new specialty ability
 * Adapted from birth.c get_player_choice -BR-
 */
void gain_specialty(void)
{
    int i, k;
    int choices[255];
    int pick;

    /* Find the next open entry in "specialty_order[]" */
    for (k = 0; k < MAX_SPECIALTIES; k++) {
	/* Stop at the first empty space */
	if (p_ptr->specialty_order[k] == PF_NO_SPECIALTY)
	    break;
    }

    /* Check if specialty array is full */
    if (k >= MAX_SPECIALTIES - 1) {
	msg_print("Maximum specialties known.");
	return;
    }

    /* Make one choice */
    if (gain_spec_menu(&pick)) {
	char buf[120];

	/* Add new specialty */
	p_ptr->specialty_order[k] = choices[pick];

	/* Add it to the player flags */
	pf_on(p_ptr->pflags, choices[pick]);

	/* Increment next available slot */
	k++;

	/* Update specialties available count */
	p_ptr->new_specialties--;
	p_ptr->old_specialties = p_ptr->new_specialties;

	/* Specialty taken */
	sprintf(buf, "Gained the %s specialty.",
		abilities[choices[pick]].name);

	/* Write a note */
	history_add(buf, HISTORY_GAIN_SPECIALTY, 0);

	/* Update some stuff */
	p_ptr->update |=
	    (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY | PU_TORCH);

	/* Redraw Study Status */
	p_ptr->redraw |= (PR_STUDY);
    }

    /* exit */
    return;
}


/*
 * View specialty code 
 */
int spec_known;
ability spec_list[32];
char race_other_desc[1000] = "";

/**
 * For a string describing the player's intrinsic racial flags.
 */
static char *view_abilities_aux(char *desc)
{
    bitflag flags[OF_SIZE];

    int attr_num, attr_listed, j;

    bool res = FALSE, vul = FALSE;

    char buf[10] = "";

    of_copy(flags, rp_ptr->flags_obj);

    /* Sustain stats. */
    if ((of_has(flags, OF_SUSTAIN_STR)) || (of_has(flags, OF_SUSTAIN_INT))
	|| (of_has(flags, OF_SUSTAIN_WIS)) || (of_has(flags, OF_SUSTAIN_DEX))
	|| (of_has(flags, OF_SUSTAIN_CON)) || (of_has(flags, OF_SUSTAIN_CHR))) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	/* How many attributes need to be listed? */
	if (of_has(flags, OF_SUSTAIN_STR))
	    attr_num++;
	if (of_has(flags, OF_SUSTAIN_INT))
	    attr_num++;
	if (of_has(flags, OF_SUSTAIN_WIS))
	    attr_num++;
	if (of_has(flags, OF_SUSTAIN_DEX))
	    attr_num++;
	if (of_has(flags, OF_SUSTAIN_CON))
	    attr_num++;
	if (of_has(flags, OF_SUSTAIN_CHR))
	    attr_num++;

	/* Special case: sustain all stats */
	if (attr_num == 6) {
	    strcat(desc, "Your stats are all sustained.  ");
	} else {
	    strcat(desc, "Your");

	    /* Loop for number of attributes in this group. */
	    for (j = 0; j < 6; j++) {
		bool list_ok = FALSE;

		if ((j == 0) && (of_has(flags, OF_SUSTAIN_STR)))
		    list_ok = TRUE;
		if ((j == 1) && (of_has(flags, OF_SUSTAIN_INT)))
		    list_ok = TRUE;
		if ((j == 2) && (of_has(flags, OF_SUSTAIN_WIS)))
		    list_ok = TRUE;
		if ((j == 3) && (of_has(flags, OF_SUSTAIN_DEX)))
		    list_ok = TRUE;
		if ((j == 4) && (of_has(flags, OF_SUSTAIN_CON)))
		    list_ok = TRUE;
		if ((j == 5) && (of_has(flags, OF_SUSTAIN_CHR)))
		    list_ok = TRUE;

		if (!list_ok)
		    continue;

		/* Listing another attribute. */
		attr_listed++;

		/* Commas separate members of a list of more than two. */
		if ((attr_num > 2) && (attr_listed > 1))
		    strcat(desc, ",");

		/* "and" before final member of a list of more than one. */
		if ((attr_num > 1) && (j != 0)) {
		    if (attr_num == attr_listed)
			strcat(desc, " and");
		}

		/* List the attribute description, in its proper place. */
		if (j == 0)
		    strcat(desc, " strength");
		if (j == 1)
		    strcat(desc, " intelligence");
		if (j == 2)
		    strcat(desc, " wisdom");
		if (j == 3)
		    strcat(desc, " dexterity");
		if (j == 4)
		    strcat(desc, " constitution");
		if (j == 5)
		    strcat(desc, " charisma");
	    }
	}

	/* Pluralize the verb. */
	if (attr_num > 1)
	    strcat(desc, " are");
	else
	    strcat(desc, " is");

	/* End sentence.  Go to next line. */
	strcat(desc, " sustained. ");
    }

    /* Clear number of items to list, and items listed. */
    attr_num = 0;
    attr_listed = 0;

    /* Elemental immunities. */
    for (j = 0; j < 4; j++)
	if (rp_ptr->percent_res[j] == RES_BOOST_IMMUNE)
	    attr_num++;

    if (attr_num > 0) {
	strcat(desc, "You are immune to ");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 4; j++) {
	    if (rp_ptr->percent_res[j] > RES_BOOST_IMMUNE)
		continue;

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		strcat(desc, "acid");
	    if (j == 1)
		strcat(desc, "electricity");
	    if (j == 2)
		strcat(desc, "fire");
	    if (j == 3)
		strcat(desc, "frost");
	    if (attr_num >= 3)
		strcat(desc, ", ");
	    if (attr_num == 2)
		strcat(desc, " and ");
	    if (attr_num == 1)
		strcat(desc, ". ");
	    attr_num--;
	}

	/* End sentence.  Go to next line. */
	strcat(desc, ". ");
    }


    /* Check for resists and vulnerabilities */
    for (j = 0; j < MAX_P_RES; j++) {
	if ((rp_ptr->percent_res[j] < 100) && (rp_ptr->percent_res[j] > 0))
	    res = TRUE;
	else if (rp_ptr->percent_res[j] > 100)
	    vul = TRUE;
    }


    /* Resistances. */
    if (res) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	for (j = 0; j < MAX_P_RES; j++) {
	    if ((rp_ptr->percent_res[j] < 100) && (rp_ptr->percent_res[j] > 0))
		attr_num++;
	}

	/* How many attributes need to be listed? */
	strcat(desc, "You resist");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < MAX_P_RES; j++) {
	    bool list_ok = FALSE;

	    if ((rp_ptr->percent_res[j] < 100) && (rp_ptr->percent_res[j] > 0))
		list_ok = TRUE;
	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		strcat(desc, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    strcat(desc, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == P_RES_ACID)
		strcat(desc, " acid");
	    if (j == P_RES_ELEC)
		strcat(desc, " electricity");
	    if (j == P_RES_FIRE)
		strcat(desc, " fire");
	    if (j == P_RES_COLD)
		strcat(desc, " frost");
	    if (j == P_RES_POIS)
		strcat(desc, " poison");
	    if (j == P_RES_LIGHT)
		strcat(desc, " light");
	    if (j == P_RES_DARK)
		strcat(desc, " darkness");
	    if (j == P_RES_CONFU)
		strcat(desc, " confusion");
	    if (j == P_RES_SOUND)
		strcat(desc, " sound");
	    if (j == P_RES_SHARD)
		strcat(desc, " shards");
	    if (j == P_RES_NEXUS)
		strcat(desc, " nexus");
	    if (j == P_RES_NETHR)
		strcat(desc, " nether");
	    if (j == P_RES_CHAOS)
		strcat(desc, " chaos");
	    if (j == P_RES_DISEN)
		strcat(desc, " disenchantment");

	    sprintf(buf, "(%d%%)", 100 - rp_ptr->percent_res[j]);
	    strcat(desc, buf);
	}

	/* End sentence.  Go to next line. */
	strcat(desc, ". ");
    }


    /* Vulnerabilities. */
    if (vul) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	for (j = 0; j < MAX_P_RES; j++) {
	    if (rp_ptr->percent_res[j] > 100)
		attr_num++;
	}

	strcat(desc, "You are vulnerable to");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < MAX_P_RES; j++) {
	    bool list_ok = FALSE;

	    if (rp_ptr->percent_res[j] > 100)
		list_ok = TRUE;

	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		strcat(desc, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    strcat(desc, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == P_RES_ACID)
		strcat(desc, " acid");
	    if (j == P_RES_ELEC)
		strcat(desc, " electricity");
	    if (j == P_RES_FIRE)
		strcat(desc, " fire");
	    if (j == P_RES_COLD)
		strcat(desc, " frost");
	    if (j == P_RES_POIS)
		strcat(desc, " poison");
	    if (j == P_RES_LIGHT)
		strcat(desc, " light");
	    if (j == P_RES_DARK)
		strcat(desc, " darkness");
	    if (j == P_RES_SOUND)
		strcat(desc, " sound");
	    if (j == P_RES_SHARD)
		strcat(desc, " shards");
	    if (j == P_RES_NEXUS)
		strcat(desc, " nexus");
	    if (j == P_RES_NETHR)
		strcat(desc, " nether");
	    if (j == P_RES_CHAOS)
		strcat(desc, " chaos");
	    if (j == P_RES_DISEN)
		strcat(desc, " disenchantment");

	    sprintf(buf, "(%d%%)", rp_ptr->percent_res[j] - 100);
	    strcat(desc, buf);
	}

	/* End sentence.  Go to next line. */
	strcat(desc, ". ");
    }




    /* Clear a listing variable. */
    attr_num = 0;

    /* Special processing for two "survival resists" */
    if (of_has(flags, OF_FEARLESS))
	attr_num++;
    if (of_has(flags, OF_SEEING))
	attr_num++;

    if (of_has(flags, OF_FEARLESS)) {
	strcat(desc, "You are fearless");
	if (attr_num == 1)
	    strcat(desc, ".  ");
	else
	    strcat(desc, ", and");
    }

    if (of_has(flags, OF_SEEING)) {
	if ((attr_num > 1) && (of_has(flags, OF_FEARLESS)))
	    strcat(desc, " can not be blinded.  ");
	else
	    strcat(desc, "You can not be blinded.  ");
    }

    /* Miscellaneous abilities. */
    if ((of_has(flags, OF_SLOW_DIGEST)) || (of_has(flags, OF_FEATHER))
	|| (of_has(flags, OF_LIGHT)) || (of_has(flags, OF_REGEN))
	|| (of_has(flags, OF_TELEPATHY)) || (of_has(flags, OF_SEE_INVIS))
	|| (of_has(flags, OF_FREE_ACT)) || (of_has(flags, OF_HOLD_LIFE))) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	/* How many attributes need to be listed? */
	if (of_has(flags, OF_SLOW_DIGEST))
	    attr_num++;
	if (of_has(flags, OF_FEATHER))
	    attr_num++;
	if (of_has(flags, OF_LIGHT))
	    attr_num++;
	if (of_has(flags, OF_REGEN))
	    attr_num++;
	if (of_has(flags, OF_TELEPATHY))
	    attr_num++;
	if (of_has(flags, OF_SEE_INVIS))
	    attr_num++;
	if (of_has(flags, OF_FREE_ACT))
	    attr_num++;
	if (of_has(flags, OF_HOLD_LIFE))
	    attr_num++;

	strcat(desc, "You");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 8; j++) {
	    bool list_ok = FALSE;

	    if ((j == 0) && (of_has(flags, OF_SLOW_DIGEST)))
		list_ok = TRUE;
	    if ((j == 1) && (of_has(flags, OF_FEATHER)))
		list_ok = TRUE;
	    if ((j == 2) && (of_has(flags, OF_LIGHT)))
		list_ok = TRUE;
	    if ((j == 3) && (of_has(flags, OF_REGEN)))
		list_ok = TRUE;
	    if ((j == 4) && (of_has(flags, OF_TELEPATHY)))
		list_ok = TRUE;
	    if ((j == 5) && (of_has(flags, OF_SEE_INVIS)))
		list_ok = TRUE;
	    if ((j == 6) && (of_has(flags, OF_FREE_ACT)))
		list_ok = TRUE;
	    if ((j == 7) && (of_has(flags, OF_HOLD_LIFE)))
		list_ok = TRUE;

	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		strcat(desc, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    strcat(desc, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		strcat(desc, " digest slowly");
	    if (j == 1)
		strcat(desc, " falling gently");
	    if (j == 2)
		strcat(desc, " glow with permanent light");
	    if (j == 3)
		strcat(desc, " regenerative quickly");
	    if (j == 4)
		strcat(desc, " have telepathic powers");
	    if (j == 5)
		strcat(desc, " can see invisible monsters");
	    if (j == 6)
		strcat(desc, " are immune to paralysis");
	    if (j == 7)
		strcat(desc, " are resistant to life draining");
	}

	/* End sentence.  Go to next line. */
	strcat(desc, ".");
    }

    return (desc);

}

static char view_spec_tag(menu_type *menu, int oid)
{
    return I2A(oid);
}

/**
 * Display an entry on the gain specialty menu
 */
void view_spec_display(menu_type *menu, int oid, bool cursor, int row, int col,
		       int width)
{
    int x, y;
    char buf[80];
    byte color;
    ability *choices = menu->menu_data;

    switch (choices[oid].type)
    {
    case PLAYER_FLAG_SPECIAL:
    {
	sprintf(buf, "Specialty Ability: %s", choices[oid].name);
	color = cursor ? TERM_L_GREEN : TERM_GREEN;
	break;
    } 
    case PLAYER_FLAG_CLASS: 
    {
	sprintf(buf, "Class: %s", choices[oid].name);
	color = cursor ? TERM_L_UMBER : TERM_UMBER;
	break;
    } 
    case PLAYER_FLAG_RACE: 
    {
	sprintf(buf, "Racial: %s", choices[oid].name);
	color = cursor ? TERM_YELLOW : TERM_ORANGE;
	break;
    } 
    default:
    {
	sprintf(buf, "Racial: Other");
	color = cursor ? TERM_YELLOW : TERM_ORANGE;
    }
    }

    /* Print it */
    c_put_str(color, buf, row, col);

    /* Help text */
    if (cursor) {
	/* Locate the cursor */
	Term_locate(&x, &y);

	/* Move the cursor */
	Term_gotoxy(3, menu->count + 2);
	text_out_indent = 3;
	text_out_to_screen(TERM_L_BLUE, (char *)choices[oid].desc);

	/* Restore */
	Term_gotoxy(x, y);
    }
}


/**
 * Display list available specialties.
 */
void view_spec_menu(void)
{
    menu_type menu;
    menu_iter menu_f = { view_spec_tag, 0, view_spec_display, 0, 0 };
    ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
    int cursor = 0;

    bool done = FALSE;

    char buf[80];

    /* Save the screen and clear it */
    screen_save();

    /* Prompt choices */
    sprintf(buf, "Race, class, and specialties abilities (%c-%c, ESC=exit): ",
	    I2A(0), I2A(spec_known - 1));

    /* Set up the menu */
    menu.title = buf;
    menu.count = spec_known;
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
    menu_setpriv(&menu, spec_known, spec_list);
    menu_layout(&menu, &SCREEN_REGION);

    while (!done) {
	evt = menu_select(&menu, cursor);
	done = (evt.type == EVT_ESCAPE);
    }

    /* Load screen */
    screen_load();

    return;
}

/**
 * Browse known specialty abilities -BR-
 * Adapted from birth.c get_player_choice
 */
void view_specialties(void)
{
    int i;

    /* Count the number of specialties we know */
    for (i = 0, spec_known = 0; i < MAX_SPECIALTIES; i++) {
	if (p_ptr->specialty_order[i] != PF_NO_SPECIALTY)
	    spec_list[spec_known++] = abilities[i];
    }

    /* Count the number of race and class powers we have */
    for (i = 0; i < PF_MAX; i++) {
	if (player_class_has(i) || player_race_has(i)) {
	    spec_list[spec_known++] = abilities[i];
	}
    }

    /* Standard racial flags */
    if (!pf_is_empty(rp_ptr->flags_obj) || !pf_is_empty(rp_ptr->flags_curse)) 
    {
	spec_list[spec_known].index = PF_MAX;
	spec_list[spec_known].name = "";
	view_abilities_aux(spec_list[spec_known].desc);
	spec_list[spec_known++].type = PF_MAX;
	//view_abilities_aux(race_other_desc);
    }

    /* View choices until user exits */
    view_spec_menu();

    /* exit */
    return;
}


/**
 * Interact with specialty abilities -BR-
 */
void do_cmd_specialty(void)
{
    ui_event_data answer;

    /* Might want to gain a new ability or browse old ones */
    if (p_ptr->new_specialties > 0) {
	/* Buttons */
	button_add("l", 'l');
	button_add("v", 'v');

	/* Interact and choose. */
	while (get_com_ex
	       ("View abilities or Learn specialty (l/v/ESC)?", &answer)) {
	    /* New ability */
	    if ((answer.key == 'L') || (answer.key == 'l')) {
		gain_specialty();
		break;
	    }

	    /* View Current */
	    if ((answer.key == 'V') || (answer.key == 'v')) {
		view_specialties();
		break;
	    }

	    /* Exit */
	    else if (answer.key == ESCAPE)
		break;


	}
	button_kill('l');
	button_kill('v');
    }

    /* View existing specialties is the only option */
    else {
	view_specialties();
	return;
    }

    return;
}
