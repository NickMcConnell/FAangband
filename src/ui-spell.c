/*
 * File: ui-spell.c
 * Purpose: Spell UI handing
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
#include "cave.h"
#include "tvalsval.h"
#include "game-cmd.h"
#include "spells.h"
#include "ui.h"
#include "ui-menu.h"





/**
 * Warriors will eventually learn to pseudo-probe monsters.  If they use 
 * the browse command, give ability information. -LM-
 */
static void warrior_probe_desc(void)
{
    /* Save screen */
    screen_save();

    /* Erase the screen */
    Term_clear();

    /* Set the indent */
    text_out_indent = 5;

    /* Title in light blue. */
    text_out_to_screen(TERM_L_BLUE, "Warrior Pseudo-Probing Ability:");
    text_out_to_screen(TERM_WHITE, "\n\n");

    /* Print out information text. */
    text_out_to_screen(TERM_WHITE,
		       "Warriors learn to probe monsters at level 35.  This costs nothing except a full turn.  When you reach this level, type 'm', and target the monster you would like to learn more about.  This reveals the monster's race, approximate HPs, and basic resistances.  Be warned:  the information given is not always complete...");
    text_out_to_screen(TERM_WHITE, "\n\n\n");

    /* The "exit" sign. */
    text_out_to_screen(TERM_WHITE, "    (Press any key to continue.)\n");

    /* Wait for it. */
    (void) inkey_ex();

    /* Load screen */
    screen_load();
}

/**
 * Spell menu data struct
 */
struct spell_menu_data {
    int spells[PY_MAX_SPELLS];
    int n_spells;
    int book_sval;
    bool browse;
    bool (*is_valid)(int spell);

    int selected_spell;
};


/**
 * Is item oid valid?
 */
static int spell_menu_valid(menu_type *m, int oid)
{
    struct spell_menu_data *d = menu_priv(m);
    int *spells = d->spells;

    return d->is_valid(spells[oid]);
}

/**
 * Display a row of the spell menu
 */
static void spell_menu_display(menu_type *m, int oid, bool cursor,
			       int row, int col, int wid)
{
    struct spell_menu_data *d = menu_priv(m);
    int spell = d->spells[oid];
    const magic_type *s_ptr = &mp_ptr->info[spell];

    char help[30];

    int attr_name, attr_extra, attr_book;
    int tval = mp_ptr->spell_book;
    const char *comment = NULL;

    /* Choose appropriate spellbook color. */
    if (tval == TV_MAGIC_BOOK) 
    {
	if (d->book_sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_RED;
	else attr_book = TERM_RED;
    }
    else if (tval == TV_PRAYER_BOOK) 
    {
	if (d->book_sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_BLUE;
	else attr_book = TERM_BLUE;
    }
    else if (tval == TV_DRUID_BOOK) 
    {
	if (d->book_sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_GREEN;
	else attr_book = TERM_GREEN;
    }
    else if (tval == TV_NECRO_BOOK) 
    {
	if (d->book_sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_PURPLE;
	else attr_book = TERM_PURPLE;
    }
    else attr_book = TERM_WHITE;
  
    if (p_ptr->spell_flags[spell] & PY_SPELL_FORGOTTEN) {
	comment = " forgotten";
	attr_name = TERM_L_WHITE;
	attr_extra = TERM_L_WHITE;
    } 
   else if (p_ptr->spell_flags[spell] & PY_SPELL_LEARNED) {
       if (p_ptr->spell_flags[spell] & PY_SPELL_WORKED) {
	   /* Get extra info */
	   get_spell_info(mp_ptr->spell_book, s_ptr->index, help, sizeof(help));
	   comment = help;
	   attr_name = attr_book;
	   attr_extra = TERM_DEEP_L_BLUE;
	} 
       else {
	   comment = " untried";
	   attr_name = TERM_WHITE;
	   attr_extra = TERM_WHITE;
       }
   } 
   else if (s_ptr->slevel <= p_ptr->lev) {
       comment = " unknown";
       attr_extra = TERM_L_WHITE;
       attr_name = TERM_WHITE;
   }
   else {
       comment = " difficult";
       attr_extra = TERM_MUD;
       attr_name = TERM_MUD;
   }
   
    /* Dump the spell --(-- */
    c_put_str(attr_name, format("%-30s", get_spell_name(s_ptr->index)), row, col);
    put_str(format("%2d %4d %3d%%", s_ptr->slevel, 
		   s_ptr->smana, spell_chance(spell)), row, col + 30);
    c_put_str(attr_extra, format("%s", comment), row, col + 42);
}

/**
 * Handle an event on a menu row.
 */
static bool spell_menu_handler(menu_type *m, const ui_event *e, int oid)
{
    struct spell_menu_data *d = menu_priv(m);

    if (e->type == EVT_SELECT) {
	d->selected_spell = d->spells[oid];
	return d->browse ? TRUE : FALSE;
    }

    return TRUE;
}

/**
 * Show spell long description when browsing
 */
static void spell_menu_browser(int oid, void *data, const region *loc)
{
    struct spell_menu_data *d = data;
    int spell = d->spells[oid];
    const magic_type *s_ptr = &mp_ptr->info[spell];

    /* Redirect output to the screen */
    text_out_hook = text_out_to_screen;
    text_out_wrap = loc->col + 60;
    text_out_indent = loc->col - 1;
    text_out_pad = 1;

    Term_gotoxy(loc->col, loc->row + loc->page_rows);
    text_out_c(TERM_DEEP_L_BLUE, format("\n%s\n", s_info[s_ptr->index].text));

    /* XXX */
    text_out_pad = 0;
    text_out_indent = 0;
    text_out_wrap = 0;
}

static const menu_iter spell_menu_iter = {
    NULL,	/* get_tag = NULL, just use lowercase selections */
    spell_menu_valid,
    spell_menu_display,
    spell_menu_handler,
    NULL	/* no resize hook */
};

/** Create and initialise a spell menu, given an object and a validity hook */
static menu_type *spell_menu_new(const object_type *o_ptr,
		bool (*is_valid)(int spell))
{
    menu_type *m = menu_new(MN_SKIN_SCROLL, &spell_menu_iter);
    struct spell_menu_data *d = mem_alloc(sizeof *d);

    region loc = { -65, 1, 65, -99 };

    /* collect spells from object */
    d->n_spells = spell_collect_from_book(o_ptr, d->spells);
    if (d->n_spells == 0 || !spell_okay_list(is_valid, d->spells, d->n_spells))
    {
	mem_free(m);
	mem_free(d);
	return NULL;
    }

    /* copy across private data */
    d->is_valid = is_valid;
    d->selected_spell = -1;
    d->browse = FALSE;
    d->book_sval = o_ptr->sval;

    menu_setpriv(m, d->n_spells, d);

    /* set flags */
    m->header = "Name                             Lv Mana Fail Info";
    m->flags = MN_CASELESS_TAGS;
    m->selections = lower_case;
    m->browse_hook = spell_menu_browser;

    /* set size */
    loc.page_rows = d->n_spells + 1;
    menu_layout(m, &loc);

    return m;
}

/** Clean up a spell menu instance */
static void spell_menu_destroy(menu_type *m)
{
    struct spell_menu_data *d = menu_priv(m);
    mem_free(d);
    mem_free(m);
}

/**
 * Run the spell menu to select a spell.
 */
static int spell_menu_select(menu_type *m, const char *noun, const char *verb)
{
    struct spell_menu_data *d = menu_priv(m);
    char buf[80];

    screen_save();
    region_erase_bordered(&m->active);

    /* Format, capitalise and display */
    strnfmt(buf, sizeof buf, "%s which %s? ", verb, noun);
    my_strcap(buf);
    prt(buf, 0, 0);

    menu_select(m, 0, TRUE);

    screen_load();

    return d->selected_spell;
}

/**
 * Run the spell menu, without selections.
 */
static void spell_menu_browse(menu_type *m, const char *noun)
{
    struct spell_menu_data *d = menu_priv(m);

    screen_save();

    region_erase_bordered(&m->active);
    prt(format("Browsing %ss.  Press Escape to exit.", noun), 0, 0);

    d->browse = TRUE;
    menu_select(m, 0, TRUE);

    screen_load();
}


/**
 * Interactively select a spell.
 *
 * Returns the spell selected, or -1.
 */
static int get_spell(const object_type *o_ptr, const char *verb,
		bool (*spell_test)(int spell))
{
    menu_type *m;
    const char *noun;

    noun = magic_desc[mp_ptr->spell_realm][SPELL_NOUN];

    m = spell_menu_new(o_ptr, spell_test);
    if (m) {
	int spell = spell_menu_select(m, noun, verb);
	spell_menu_destroy(m);
	return spell;
    }

    return -1;
}

/**
 * Browse a given book.
 */
void textui_book_browse(const object_type *o_ptr)
{
    menu_type *m;

    m = spell_menu_new(o_ptr, spell_okay_to_browse);
    if (m) {
	spell_menu_browse(m, magic_desc[mp_ptr->spell_realm][SPELL_NOUN]);
	spell_menu_destroy(m);
    } else {
	msg("You cannot browse that.");
    }
}

/**
 * Browse the given book.
 */
void textui_spell_browse(void)
{
    int item;

    char q[80];
    char s[80];

    if (mp_ptr->spell_realm == REALM_NONE) {
	if (player_has(PF_PROBE))
	    warrior_probe_desc();
	else
	    msg("You cannot read books!");
	return;
    }

    strnfmt(q, sizeof(q), "Browse which %s?", 
	    magic_desc[mp_ptr->spell_realm][BOOK_NOUN]);
    strnfmt(s, sizeof(s), " You have no %ss that you can read.", 
	    magic_desc[mp_ptr->spell_realm][BOOK_LACK]);

    item_tester_hook = obj_can_browse;
    if (!get_item(&item, q, s, CMD_BROWSE_SPELL, 
		  (USE_INVEN | USE_FLOOR | IS_HARMLESS)))
	return;

    /* Track the object kind */
    track_object(item);
    handle_stuff(p_ptr);

    textui_book_browse(object_from_item_idx(item));
}

/**
 * Study a book to gain a new spell
 */
void textui_obj_study(void)
{
    int item;
    char q[80];
    char s[80];

    if (mp_ptr->spell_realm == REALM_NONE) {
	msg("You cannot read books!");
	return;
    }

    strnfmt(q, sizeof(q), "Study which %s?", 
	    magic_desc[mp_ptr->spell_realm][BOOK_NOUN]);
    strnfmt(s, sizeof(s), " You have no %ss that you can study.", 
	    magic_desc[mp_ptr->spell_realm][BOOK_LACK]);

    item_tester_hook = obj_can_study;
    if (!get_item(&item, q, s, CMD_STUDY_BOOK, (USE_INVEN | USE_FLOOR)))
	return;

    track_object(item);
    handle_stuff(p_ptr);

    if (mp_ptr->spell_book != TV_PRAYER_BOOK) {
	int spell = get_spell(object_from_item_idx(item),
			      "study", spell_okay_to_study);
	if (spell >= 0) {
	    cmd_insert(CMD_STUDY_SPELL);
	    cmd_set_arg_choice(cmd_get_top(), 0, spell);
	}
    } else {
	cmd_insert(CMD_STUDY_BOOK);
	cmd_set_arg_item(cmd_get_top(), 0, item);
    }
}

/**
 * Cast a spell from a book.
 */
void textui_obj_cast(void)
{
    int item;
    int spell;

    const char *verb = magic_desc[mp_ptr->spell_realm][SPELL_VERB];
    char q[80];
    char s[80];

    if (mp_ptr->spell_realm == REALM_NONE) {
	msg("You cannot read books!");
	return;
    }

    strnfmt(q, sizeof(q), "Use which %s?", 
	    magic_desc[mp_ptr->spell_realm][BOOK_NOUN]);
    strnfmt(s, sizeof(s), " You have no %ss that you can use.", 
	    magic_desc[mp_ptr->spell_realm][BOOK_LACK]);

    item_tester_hook = obj_can_cast_from;
    if (!get_item(&item, q, s, CMD_CAST, (USE_INVEN | USE_FLOOR)))
	return;

    /* Track the object kind */
    track_object(item);

    /* Ask for a spell */
    spell = get_spell(object_from_item_idx(item), verb, spell_okay_to_cast);
    if (spell >= 0) {
	cmd_insert(CMD_CAST);
	cmd_set_arg_choice(cmd_get_top(), 0, spell);
    }
}
