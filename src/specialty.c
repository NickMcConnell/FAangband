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
#include "cave.h"
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
 * Specialty menu data struct
 */
struct spec_menu_data {
	int specialties[CLASS_SPECIALTIES];
	int spec_known;
	int selected_spec;
};

/**
 * Check if we can gain a specialty ability -BR-
 */
bool check_specialty_gain(int specialty)
{
    int i;

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
    struct spec_menu_data *d = menu_priv(menu);
    int idx = d->specialties[oid];

    byte attr = (cursor ? TERM_L_GREEN : TERM_GREEN);

    /* Print it */
    c_put_str(attr, abilities[idx].name, row, col);

}

/**
 * Deal with events on the gain specialty menu
 */
bool gain_spec_action(menu_type * menu, const ui_event * e, int oid)
{
    struct spec_menu_data *d = menu_priv(menu);
    static int i;
    if (oid) i = oid;

    if (e->type == EVT_SELECT)
    {
	d->selected_spec = d->specialties[i];
	return FALSE;
    }

    return TRUE;
}


/**
 * Show spell long description when browsing
 */
static void  gain_spec_menu_browser(int oid, void *data, const region *loc)
{
    struct spec_menu_data *d = data;
    
    /* Redirect output to the screen */
    text_out_hook = text_out_to_screen;
    text_out_wrap = 0;
    text_out_indent = loc->col - 1;
    text_out_pad = 1;
    
    
    clear_from(loc->row + loc->page_rows);
    Term_gotoxy(loc->col, loc->row + loc->page_rows);
    text_out_to_screen(TERM_DEEP_L_BLUE, (char *) abilities[d->specialties[oid]].desc);
    
    /* XXX */
    text_out_pad = 0;
    text_out_indent = 0;
}

/**
 * Display list available specialties.
 */
bool gain_spec_menu(int *pick)
{
    menu_type menu;
    menu_iter menu_f = { gain_spec_tag, 0, gain_spec_display,
			 gain_spec_action, 0 };
    ui_event evt = { 0 };
    struct spec_menu_data *d = mem_alloc(sizeof *d);
    region loc = { 0, 0, 70, -99 };
    bool done = FALSE;

    size_t i;

    char buf[80];

    /* Find the learnable specialties */
    d->spec_known = 0;
    for (i = 0; i < PF_MAX; i++) {
	if (check_specialty_gain(i)) {
	    d->specialties[d->spec_known] = i;
	    d->spec_known++;
	}
    }

    /* We are out of specialties - should never happen */
    if (!d->spec_known) {
	msg("No specialties available.");
	screen_load();
	return FALSE;
    }

    /* Save the screen and clear it */
    screen_save();
    clear_from(0);

    /* Prompt choices */
    sprintf(buf,
	    "(Specialties: %c-%c, ESC=exit) Gain which specialty (%d available)? ",
	    I2A(0), I2A(d->spec_known - 1), p_ptr->new_specialties);

    /* Set up the menu */
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
    menu.title = buf;
    menu_setpriv(&menu, d->spec_known, d);
    loc.page_rows = d->spec_known + 2;
    menu.browse_hook = gain_spec_menu_browser;
    menu.flags = MN_DBL_TAP;
    region_erase_bordered(&loc);
    menu_layout(&menu, &loc);

    while (!done) {
	evt = menu_select(&menu, EVT_SELECT, TRUE);
	done = (evt.type == EVT_ESCAPE);
	if (!done && (d->selected_spec))
	{
    menu_layout(&menu, &loc);
	    done = get_check(format("Definitely choose %s? ", 
				    abilities[d->selected_spec].name));
	}
    }

    if (evt.type != EVT_ESCAPE)
	*pick = d->selected_spec;

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
    int k;
    int pick;

    /* Find the next open entry in "specialty_order[]" */
    for (k = 0; k < MAX_SPECIALTIES; k++) {
	/* Stop at the first empty space */
	if (p_ptr->specialty_order[k] == PF_NO_SPECIALTY)
	    break;
    }

    /* Check if specialty array is full */
    if (k >= MAX_SPECIALTIES - 1) {
	msg("Maximum specialties known.");
	return;
    }

    /* Make one choice */
    if (gain_spec_menu(&pick)) {
	char buf[120];

	/* Add new specialty */
	p_ptr->specialty_order[k] = pick;

	/* Add it to the player flags */
	pf_on(p_ptr->pflags, pick);

	/* Increment next available slot */
	k++;

	/* Update specialties available count */
	p_ptr->new_specialties--;
	p_ptr->old_specialties = p_ptr->new_specialties;

	/* Specialty taken */
	sprintf(buf, "Gained the %s specialty.",
		abilities[pick].name);

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
static void view_abilities_aux(char *desc)
{
    bitflag flags[OF_SIZE];

    int attr_num, attr_listed, j;
    int max = 100;

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
	    my_strcat(desc, "Your stats are all sustained.  ", max);
	} else {
	    my_strcat(desc, "Your", max);

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
		    my_strcat(desc, ",", max);

		/* "and" before final member of a list of more than one. */
		if ((attr_num > 1) && (j != 0)) {
		    if (attr_num == attr_listed)
			my_strcat(desc, " and", max);
		}

		/* List the attribute description, in its proper place. */
		if (j == 0)
		    my_strcat(desc, " strength", max);
		if (j == 1)
		    my_strcat(desc, " intelligence", max);
		if (j == 2)
		    my_strcat(desc, " wisdom", max);
		if (j == 3)
		    my_strcat(desc, " dexterity", max);
		if (j == 4)
		    my_strcat(desc, " constitution", max);
		if (j == 5)
		    my_strcat(desc, " charisma", max);
	    }
	}

	/* Pluralize the verb. */
	if (attr_num > 1)
	    my_strcat(desc, " are", max);
	else
	    my_strcat(desc, " is", max);

	/* End sentence.  Go to next line. */
	my_strcat(desc, " sustained. ", max);
    }

    /* Clear number of items to list, and items listed. */
    attr_num = 0;
    attr_listed = 0;

    /* Elemental immunities. */
    for (j = 0; j < 4; j++)
	if (rp_ptr->percent_res[j] == RES_BOOST_IMMUNE)
	    attr_num++;

    if (attr_num > 0) {
	my_strcat(desc, "You are immune to ", max);

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 4; j++) {
	    if (rp_ptr->percent_res[j] > RES_BOOST_IMMUNE)
		continue;

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		my_strcat(desc, "acid", max);
	    if (j == 1)
		my_strcat(desc, "electricity", max);
	    if (j == 2)
		my_strcat(desc, "fire", max);
	    if (j == 3)
		my_strcat(desc, "frost", max);
	    if (attr_num >= 3)
		my_strcat(desc, ", ", max);
	    if (attr_num == 2)
		my_strcat(desc, " and ", max);
	    if (attr_num == 1)
		my_strcat(desc, ". ", max);
	    attr_num--;
	}

	/* End sentence.  Go to next line. */
	my_strcat(desc, ". ", max);
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
	my_strcat(desc, "You resist", max);

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
		my_strcat(desc, ",", max);

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    my_strcat(desc, " and", max);
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == P_RES_ACID)
		my_strcat(desc, " acid", max);
	    if (j == P_RES_ELEC)
		my_strcat(desc, " electricity", max);
	    if (j == P_RES_FIRE)
		my_strcat(desc, " fire", max);
	    if (j == P_RES_COLD)
		my_strcat(desc, " frost", max);
	    if (j == P_RES_POIS)
		my_strcat(desc, " poison", max);
	    if (j == P_RES_LIGHT)
		my_strcat(desc, " light", max);
	    if (j == P_RES_DARK)
		my_strcat(desc, " darkness", max);
	    if (j == P_RES_CONFU)
		my_strcat(desc, " confusion", max);
	    if (j == P_RES_SOUND)
		my_strcat(desc, " sound", max);
	    if (j == P_RES_SHARD)
		my_strcat(desc, " shards", max);
	    if (j == P_RES_NEXUS)
		my_strcat(desc, " nexus", max);
	    if (j == P_RES_NETHR)
		my_strcat(desc, " nether", max);
	    if (j == P_RES_CHAOS)
		my_strcat(desc, " chaos", max);
	    if (j == P_RES_DISEN)
		my_strcat(desc, " disenchantment", max);

	    sprintf(buf, "(%d%%)", 100 - rp_ptr->percent_res[j]);
	    my_strcat(desc, buf, max);
	}

	/* End sentence.  Go to next line. */
	my_strcat(desc, ". ", max);
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

	my_strcat(desc, "You are vulnerable to", max);

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
		my_strcat(desc, ",", max);

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    my_strcat(desc, " and", max);
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == P_RES_ACID)
		my_strcat(desc, " acid", max);
	    if (j == P_RES_ELEC)
		my_strcat(desc, " electricity", max);
	    if (j == P_RES_FIRE)
		my_strcat(desc, " fire", max);
	    if (j == P_RES_COLD)
		my_strcat(desc, " frost", max);
	    if (j == P_RES_POIS)
		my_strcat(desc, " poison", max);
	    if (j == P_RES_LIGHT)
		my_strcat(desc, " light", max);
	    if (j == P_RES_DARK)
		my_strcat(desc, " darkness", max);
	    if (j == P_RES_SOUND)
		my_strcat(desc, " sound", max);
	    if (j == P_RES_SHARD)
		my_strcat(desc, " shards", max);
	    if (j == P_RES_NEXUS)
		my_strcat(desc, " nexus", max);
	    if (j == P_RES_NETHR)
		my_strcat(desc, " nether", max);
	    if (j == P_RES_CHAOS)
		my_strcat(desc, " chaos", max);
	    if (j == P_RES_DISEN)
		my_strcat(desc, " disenchantment", max);

	    sprintf(buf, "(%d%%)", rp_ptr->percent_res[j] - 100);
	    my_strcat(desc, buf, max);
	}

	/* End sentence.  Go to next line. */
	my_strcat(desc, ". ", max);
    }




    /* Clear a listing variable. */
    attr_num = 0;

    /* Special processing for two "survival resists" */
    if (of_has(flags, OF_FEARLESS))
	attr_num++;
    if (of_has(flags, OF_SEEING))
	attr_num++;

    if (of_has(flags, OF_FEARLESS)) {
	my_strcat(desc, "You are fearless", max);
	if (attr_num == 1)
	    my_strcat(desc, ".  ", max);
	else
	    my_strcat(desc, ", and", max);
    }

    if (of_has(flags, OF_SEEING)) {
	if ((attr_num > 1) && (of_has(flags, OF_FEARLESS)))
	    my_strcat(desc, " can not be blinded.  ", max);
	else
	    my_strcat(desc, "You can not be blinded.  ", max);
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

	my_strcat(desc, "You", max);

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
		my_strcat(desc, ",", max);

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    my_strcat(desc, " and", max);
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		my_strcat(desc, " digest slowly", max);
	    if (j == 1)
		my_strcat(desc, " falling gently", max);
	    if (j == 2)
		my_strcat(desc, " glow with permanent light", max);
	    if (j == 3)
		my_strcat(desc, " regenerative quickly", max);
	    if (j == 4)
		my_strcat(desc, " have telepathic powers", max);
	    if (j == 5)
		my_strcat(desc, " can see invisible monsters", max);
	    if (j == 6)
		my_strcat(desc, " are immune to paralysis", max);
	    if (j == 7)
		my_strcat(desc, " are resistant to life draining", max);
	}

	/* End sentence.  Go to next line. */
	my_strcat(desc, ".", max);
    }

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
    char buf[80];
    byte color;
    ability *choices = menu->menu_data;

    switch (choices[oid].type)
    {
    case PLAYER_FLAG_SPECIAL:
    {
	sprintf(buf, "Specialty Ability: %s", choices[oid].name);
	color = TERM_GREEN;
	break;
    } 
    case PLAYER_FLAG_CLASS: 
    {
	sprintf(buf, "Class: %s", choices[oid].name);
	color = TERM_UMBER;
	break;
    } 
    case PLAYER_FLAG_RACE: 
    {
	sprintf(buf, "Racial: %s", choices[oid].name);
	color = TERM_ORANGE;
	break;
    } 
    default:
    {
	sprintf(buf, "Racial: Other");
	color = TERM_ORANGE;
    }
    }

    /* Print it */
    c_put_str(cursor ? get_color(color, ATTR_HIGH, 1) : color, buf, row, col);

}


/**
 * Show specialty long description when browsing
 */
static void view_spec_menu_browser(int oid, void *data, const region *loc)
{
	ability *choices = data;

	/* Redirect output to the screen */
	text_out_hook = text_out_to_screen;
	text_out_wrap = 0;
	text_out_indent = loc->col - 1;
	text_out_pad = 1;

	clear_from(loc->row + loc->page_rows);
	Term_gotoxy(loc->col, loc->row + loc->page_rows);
	if (choices[oid].index == PF_MAX)
	    text_out_c(TERM_L_BLUE, "\n%s\n", race_other_desc);
	else
	    text_out_c(TERM_L_BLUE, "\n%s\n", (char *)choices[oid].desc);

	/* XXX */
	text_out_pad = 0;
	text_out_indent = 0;
}

/**
 * Display list available specialties.
 */
void view_spec_menu(void)
{
    menu_type menu;
    menu_iter menu_f = { view_spec_tag, 0, view_spec_display, 0, 0 };
    region loc = { 0, 0, 70, -99 };
    char buf[80];

    /* Save the screen and clear it */
    screen_save();

    /* Prompt choices */
    sprintf(buf, "Race, class, and specialties abilities (%c-%c, ESC=exit): ",
	    I2A(0), I2A(spec_known - 1));

    /* Set up the menu */
    menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
    menu.header = buf;
    menu_setpriv(&menu, spec_known, spec_list);
    loc.page_rows = spec_known + 1;
    menu.flags = MN_DBL_TAP;
    menu.browse_hook = view_spec_menu_browser;
    region_erase_bordered(&loc);
    menu_layout(&menu, &loc);

    menu_select(&menu, 0, FALSE);

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
	    spec_list[spec_known++] = abilities[p_ptr->specialty_order[i]];
    }

    /* Count the number of class powers we have */
    for (i = 0; i < PF_MAX; i++) {
	if (player_class_has(i)) {
	    spec_list[spec_known++] = abilities[i];
	}
    }

    /* Count the number of race powers we have */
    for (i = 0; i < PF_MAX; i++) {
	if (player_race_has(i)) {
	    spec_list[spec_known++] = abilities[i];
	}
    }

    /* Standard racial flags */
    if (!of_is_empty(rp_ptr->flags_obj) || !cf_is_empty(rp_ptr->flags_curse)) 
    {
	spec_list[spec_known].index = PF_MAX;
	spec_list[spec_known].name = "";
	spec_list[spec_known].desc = "";
	view_abilities_aux(race_other_desc);
	spec_list[spec_known++].type = PF_MAX;
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
    ui_event answer;

    /* Might want to gain a new ability or browse old ones */
    if (p_ptr->new_specialties > 0) {
	/* Buttons */
	button_add("l", 'l');
	button_add("v", 'v');

	/* Interact and choose. */
	while (get_com_ex
	       ("View abilities or Learn specialty (l/v/ESC)?", &answer)) {
	    /* New ability */
	    if ((answer.key.code == 'L') || (answer.key.code == 'l')) {
		gain_specialty();
		break;
	    }

	    /* View Current */
	    if ((answer.key.code == 'V') || (answer.key.code == 'v')) {
		view_specialties();
		break;
	    }

	    /* Exit */
	    else if (answer.key.code == ESCAPE)
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
