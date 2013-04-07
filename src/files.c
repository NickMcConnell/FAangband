/** \file files.c 
    \brief High level routines dealing with external files

 * Code for multiuser machines, Colors for skill descriptions, the char-
 * acter screens (inc. resistance flags for races, etc.), equippy chars, 
 * online-help, extraction of base name (for savefiles), saving, death 
 * (with inventory, equip, etc. display), calculating and displaying 
 * scores, creating tombstones, winners, panic saves, reading a random 
 * line from a file, and signal handling.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke, Leon 
 * Marrick, Bahman Rabii, Nick McConnell
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
#include "button.h"
#include "cave.h"
#include "cmds.h"
#include "files.h"
#include "game-cmd.h"
#include "history.h"
#include "tvalsval.h"
#include "option.h"
#include "savefile.h"
#include "ui-menu.h"


/**
 * Hack -- pass color info around this file
 */
static byte likert_color = TERM_WHITE;


/**
 * Returns a "rating" of x depending on y
 */
static const char *likert(int x, int y)
{
    char description[10];

    /* Paranoia */
    if (y <= 0)
	y = 1;

    /* Negative value */
    if (x < 0) {
	likert_color = TERM_L_DARK;
	return ("Awful");
    }

    /* Analyze the value */
    switch ((x / y)) {
    case 0:
	{
	    likert_color = TERM_RED;
	    strncpy(description, "Very Bad", 10);
	    break;
	}
    case 1:
	{
	    likert_color = TERM_L_RED;
	    strncpy(description, "Bad", 10);
	    break;
	}
    case 2:
	{
	    likert_color = TERM_ORANGE;
	    strncpy(description, "Poor", 10);
	    break;
	}
    case 3:
	{
	    likert_color = TERM_ORANGE;
	    strncpy(description, "Mediocre", 10);
	    break;
	}
    case 4:
	{
	    likert_color = TERM_YELLOW;
	    strncpy(description, "Fair", 10);
	    break;
	}
    case 5:
	{
	    likert_color = TERM_YELLOW;
	    strncpy(description, "Good", 10);
	    break;
	}
    case 6:
    case 7:
	{
	    likert_color = TERM_YELLOW;
	    strncpy(description, "Very Good", 10);
	    break;
	}
    case 8:
    case 9:
    case 10:
	{
	    likert_color = TERM_L_GREEN;
	    strncpy(description, "Excellent", 10);
	    break;
	}
    case 11:
    case 12:
    case 13:
	{
	    likert_color = TERM_L_GREEN;
	    strncpy(description, "Superb", 10);
	    break;
	}
    case 14:
    case 15:
    case 16:
    case 17:
	{
	    likert_color = TERM_BLUE;
	    strncpy(description, "Heroic", 10);
	    break;
	}
    default:
	{
	    likert_color = TERM_BLUE;
	    strncpy(description, "Legendary", 10);
	    break;
	}
    }
    return (format("%s(%d)", description, x));
}


/**
 * Obtain the "flags" for the player as if he was an item.  Currently includes 
 * race, class, and shapechange (optionally). -LM-
 *
 * Mega - Hack
 * 'shape' should be set on when calling this function for display purposes, 
 * but off when actually applying 'intrinsic flags' in xtra1.c.
 *
 * Shapeshift flags are displayed like race/class flags, but actually 
 * applied differently.
 */

void player_flags(bitflag *flags)
{
    /* Clear */
    of_wipe(flags);

    /* Add racial flags */
    of_union(flags, rp_ptr->flags_obj);

    /* Warrior. */
    if (player_has(PF_RELENTLESS)) {
	if (p_ptr->lev >= 30)
	    of_on(flags, OF_FEARLESS);
	if (p_ptr->lev >= 40)
	    of_on(flags, OF_REGEN);
    }

    /* Shapechange, if any. */
    switch (p_ptr->schange) {
    case SHAPE_BEAR:
    case SHAPE_NORMAL:
    case SHAPE_MOUSE:
	{
	    break;
	}
    case SHAPE_FERRET:
	{
	    of_on(flags, OF_REGEN);
	    break;
	}
    case SHAPE_HOUND:
	{
	    of_on(flags, OF_TELEPATHY);
	    break;
	}
    case SHAPE_LION:
	{
	    of_on(flags, OF_FEARLESS);
	    of_on(flags, OF_REGEN);
	    break;
	}
    case SHAPE_ENT:
	{
	    of_on(flags, OF_FEARLESS);
	    of_on(flags, OF_SEE_INVIS);
	    of_on(flags, OF_FREE_ACT);
	    break;
	}
    case SHAPE_BAT:
	{
	    of_on(flags, OF_SEEING);
	    of_on(flags, OF_FEATHER);
	    break;
	}
    case SHAPE_WEREWOLF:
	{
	    of_on(flags, OF_REGEN);
	    break;
	}
    case SHAPE_VAMPIRE:
	{
	    of_on(flags, OF_SEE_INVIS);
	    of_on(flags, OF_HOLD_LIFE);
	    of_on(flags, OF_REGEN);
	    break;
	}
    case SHAPE_WYRM:
	{
	    object_type *o_ptr = &p_ptr->inventory[INVEN_BODY];

	    /* Paranoia */
	    if (o_ptr->tval != TV_DRAG_ARMOR)
		break;

	    /* Add 'extra' power if any */
	    switch (o_ptr->sval) {

	    case (SV_DRAGON_GREEN):
		{
		    of_on(flags, OF_REGEN);
		    break;
		}
	    case (SV_DRAGON_SHINING):
		{
		    of_on(flags, OF_SEE_INVIS);
		    break;
		}
	    case (SV_DRAGON_LAW):
	    case (SV_DRAGON_CHAOS):
		{
		    of_on(flags, OF_HOLD_LIFE);
		    break;
		}
	    case (SV_DRAGON_BRONZE):
	    case (SV_DRAGON_GOLD):
		{
		    of_on(flags, OF_FREE_ACT);
		    break;
		}

	    }
	    break;
	}
    }
}


/**
 * Obtain information about player negative mods.
 * Currently includes shapechange and race effects.
 *
 * We do not include AGGRAVATE, which is inherantly bad.  We only use
 * 'reversed' effects.
 *
 * The only effects that we *do* include are those which either totally
 * negate a resist/ability or those which have a negatively effective
 * pval.
 *
 * Based on player_flags, but for display purposes only.
 */
void player_weakness_dis(bitflag * flags)
{
    /* Clear */
    of_wipe(flags);

    /* HACK - add weakness of some races */
    if (player_has(PF_WOODEN)) {
	of_on(flags, OF_FEATHER);
    }

    /* Shapechange, if any. */
    switch (p_ptr->schange) {
    case SHAPE_NORMAL:
	{
	    break;
	}
    case SHAPE_MOUSE:
    case SHAPE_FERRET:
    case SHAPE_HOUND:
    case SHAPE_GAZELLE:
    case SHAPE_LION:
    case SHAPE_BAT:
    case SHAPE_WEREWOLF:
    case SHAPE_BEAR:
    case SHAPE_WYRM:
	{
	    break;
	}
    case SHAPE_ENT:
	{
	    of_on(flags, OF_FEATHER);
	    break;
	}
    case SHAPE_VAMPIRE:
	{
	    of_on(flags, OF_LIGHT);
	    break;
	}
    }

}

/**
 * Race and shapechange resists 
 */
void get_player_resists(int *player_resists)
{
    int i;

    for (i = 0; i < MAX_P_RES; i++)
	player_resists[i] = rp_ptr->percent_res[i];

    /* Shapechange, if any. */
    switch (p_ptr->schange) {
    case SHAPE_NORMAL:
    case SHAPE_MOUSE:
    case SHAPE_FERRET:
    case SHAPE_HOUND:
    case SHAPE_GAZELLE:
    case SHAPE_LION:
    case SHAPE_BAT:
    case SHAPE_WEREWOLF:
    case SHAPE_BEAR:
	{
	    break;
	}
    case SHAPE_ENT:
	{
	    apply_resist(&player_resists[P_RES_COLD], RES_BOOST_NORMAL);
	    apply_resist(&player_resists[P_RES_POIS], RES_BOOST_NORMAL);

	    /* Avoid double jeopardy */
	    if (!player_has(PF_WOODEN))
		apply_resist(&player_resists[P_RES_FIRE], RES_CUT_MINOR);
	    break;
	}
    case SHAPE_VAMPIRE:
	{
	    apply_resist(&player_resists[P_RES_COLD], RES_BOOST_NORMAL);
	    apply_resist(&player_resists[P_RES_LIGHT], RES_CUT_MINOR);
	    break;
	}
    case SHAPE_WYRM:
	{
	    object_type *o_ptr = &p_ptr->inventory[INVEN_BODY];

	    /* Paranoia */
	    if (o_ptr->tval != TV_DRAG_ARMOR)
		break;

	    /* Add 'extra' power if any */
	    switch (o_ptr->sval) {
	    case (SV_DRAGON_BLACK):
		{
		    apply_resist(&player_resists[P_RES_ACID], RES_BOOST_IMMUNE);
		    break;
		}
	    case (SV_DRAGON_BLUE):
		{
		    apply_resist(&player_resists[P_RES_ELEC], RES_BOOST_IMMUNE);
		    break;
		}
	    case (SV_DRAGON_WHITE):
		{
		    apply_resist(&player_resists[P_RES_COLD], RES_BOOST_IMMUNE);
		    break;
		}
	    case (SV_DRAGON_RED):
		{
		    apply_resist(&player_resists[P_RES_FIRE], RES_BOOST_IMMUNE);
		    break;
		}
	    }
	    break;
	}
    }
}

/**
 * Shapechange bonuses - level-based race bonuses are not shown
 */
void get_player_bonus(int *player_bonus)
{
    int i;

    for (i = 0; i < MAX_P_BONUS; i++)
	player_bonus[i] = 0;

    /* Shapechange, if any. */
    switch (p_ptr->schange) {
    case SHAPE_BEAR:
    case SHAPE_NORMAL:
	{
	    break;
	}
    case SHAPE_MOUSE:
	{
	    player_bonus[P_BONUS_STEALTH] +=
		(30 + p_ptr->state.skills[SKILL_STEALTH]) / 2;
	    player_bonus[P_BONUS_INFRA] += 2;
	    player_bonus[P_BONUS_M_MASTERY] -=
		3 * p_ptr->state.skills[SKILL_DEVICE] / 40;
	    break;
	}
    case SHAPE_FERRET:
	{
	    player_bonus[P_BONUS_INFRA] += 2;
	    player_bonus[P_BONUS_SPEED] += 2;
	    player_bonus[P_BONUS_SEARCH] += 10;
	    player_bonus[P_BONUS_M_MASTERY] -=
		p_ptr->state.skills[SKILL_DEVICE] / 20;
	    break;
	}
    case SHAPE_HOUND:
	{
	    player_bonus[P_BONUS_M_MASTERY] -=
		p_ptr->state.skills[SKILL_DEVICE] / 20;
	    player_bonus[P_BONUS_INFRA] += 3;
	    break;
	}
    case SHAPE_GAZELLE:
	{
	    player_bonus[P_BONUS_M_MASTERY] -=
		p_ptr->state.skills[SKILL_DEVICE] / 20;
	    player_bonus[P_BONUS_SPEED] += 6;
	    break;
	}
    case SHAPE_LION:
	{
	    player_bonus[P_BONUS_M_MASTERY] -=
		p_ptr->state.skills[SKILL_DEVICE] / 20;
	    player_bonus[P_BONUS_SPEED] += 1;
	    break;
	}
    case SHAPE_ENT:
	{
	    player_bonus[P_BONUS_TUNNEL] += 15;
	    break;
	}
    case SHAPE_BAT:
	{
	    player_bonus[P_BONUS_INFRA] += 6;
	    player_bonus[P_BONUS_SPEED] += 5;
	    player_bonus[P_BONUS_M_MASTERY] -=
		3 * p_ptr->state.skills[SKILL_DEVICE] / 40;
	    break;
	}
    case SHAPE_WEREWOLF:
	{
	    player_bonus[P_BONUS_M_MASTERY] -=
		p_ptr->state.skills[SKILL_DEVICE] / 20;
	    player_bonus[P_BONUS_INFRA] += 3;
	    break;
	}
    case SHAPE_VAMPIRE:
	{
	    player_bonus[P_BONUS_INFRA] += 3;
	    player_bonus[P_BONUS_STEALTH] += 1;
	    player_bonus[P_BONUS_M_MASTERY] += 3;
	    break;
	}
    case SHAPE_WYRM:
	{
	    player_bonus[P_BONUS_STEALTH] -= 3;
	    player_bonus[P_BONUS_M_MASTERY] += 1;
	    break;
	}

    }
}

/**
 * Hack -- see below
 */
static u32b display_player_powers[10] = {

    OF_SLOW_DIGEST,
    OF_FEATHER,
    OF_LIGHT,
    OF_REGEN,
    OF_TELEPATHY,
    OF_SEE_INVIS,
    OF_FREE_ACT,
    OF_HOLD_LIFE,
    OF_SEEING,
    OF_FEARLESS
};

/**
 * Hack -- see below
 */
static const char *display_player_resist_names[2][7] = {
    {
     "Acid:",			/* P_RES_ACID */
     "Elec:",			/* P_RES_ELEC */
     "Fire:",			/* P_RES_FIRE */
     "Cold:",			/* P_RES_COLD */
     "Pois:",			/* P_RES_POIS */
     "Lght:",			/* P_RES_LIGHT */
     "Dark:"			/* P_RES_DARK */
     },

    {
     "Confu:",			/* P_RES_CONFU */
     "Sound:",			/* P_RES_SOUND */
     "Shard:",			/* P_RES_SHARD */
     "Nexus:",			/* P_RES_NEXUS */
     "Nethr:",			/* P_RES_NETHR */
     "Chaos:",			/* P_RES_CHAOS */
     "Disen:"			/* P_RES_DISEN */
     }
};


static const char *display_player_power_names[10] = {
    "S.Dig:",			/* OF_SLOW_DIGEST */
    "Feath:",			/* OF_FEATHER */
    "PLght:",			/* OF_LIGHT */
    "Regen:",			/* OF_REGEN */
    "Telep:",			/* OF_TELEPATHY */
    "Invis:",			/* OF_SEE_INVIS */
    "FrAct:",			/* OF_FREE_ACT */
    "HLife:",			/* OF_HOLD_LIFE */
    "Blind:",			/* OF_SEEING */
    "NFear:"			/* OF_FEARLESS */
};

static const char *display_player_bonus_names[8] = {
    "M-Mas:",			/* P_BONUS_M_MASTERY */
    "Stea.:",			/* P_BONUS_STEALTH */
    "Sear.:",			/* P_BONUS_SEARCH */
    "Infra:",			/* P_BONUS_INFRA */
    "Tunn.:",			/* P_BONUS_TUNNEL */
    "Speed:",			/* P_BONUS_SPEED */
    "Shots:",			/* P_BONUS_SHOTS */
    "Might:",			/* P_BONUS_MIGHT */
};

/*
 * Read in the specialty names.
 */
static const char *specialty_names[] = {
#define PF(x, y, z, w)    y
#include "list-player-flags.h"
#undef PF
""
};

/**
 * Display the character on the screen (various modes)
 *
 * Completely redone for FA 030 to use the new 'C' screen display.
 *
 * Mode 0 = basic display with skills and history
 * Mode 1 = extra display with powers and resistances
 *
 */
void display_player(int mode)
{
    /* Erase screen */
    clear_from(0);

    /* Make the array of lines */
    (void) make_dump(((mode == 0) ? pline0 : pline1), mode);

    /* Display the player */
    display_dump(((mode == 0) ? pline0 : pline1), 0, 25, 0);

}

/*
 * Hack - include a chunk of the old display code to deal with small screen
 * birth
 */


/**
 * Print long number with header at given row, column
 * Use the color for the number, not the header
 */
static void prt_lnum(const char *header, s32b num, int row, int col, byte color)
{
    int len = strlen(header);
    char out_val[32];
    put_str(header, row, col);
    sprintf(out_val, "%9ld", (long) num);
    c_put_str(color, out_val, row, col + len);
}

/**
 * Print number with header at given row, column
 */
static void prt_num(const char *header, int num, int row, int col, byte color)
{
    int len = strlen(header);
    char out_val[32];
    put_str(header, row, col);
    put_str("   ", row, col + len);
    sprintf(out_val, "%6ld", (long) num);
    c_put_str(color, out_val, row, col + len);
}

/**
 * Print decimal number with header at given row, column
 */
static void prt_deci(const char *header, int num, int deci, int row, int col,
		     byte color)
{
    int len = strlen(header);
    char out_val[32];
    put_str(header, row, col);
    put_str("   ", row, col + len);
    sprintf(out_val, "%8ld", (long) deci);
    c_put_str(color, out_val, row, col + len);
    sprintf(out_val, "%6ld", (long) num);
    c_put_str(color, out_val, row, col + len);
    sprintf(out_val, ".");
    c_put_str(color, out_val, row, col + len + 6);
}

/**
 * Display the character for small screen birth
 *
 */
void display_player_sml(void)
{
    int i, j;

    byte a;
    wchar_t c;

    char buf[100];
 
    int show_m_tohit = p_ptr->state.dis_to_h;
    int show_a_tohit = p_ptr->state.dis_to_h;
    int show_m_todam = p_ptr->state.dis_to_d;
    int show_a_todam = p_ptr->state.dis_to_d;

    object_type *o_ptr;
    char tmp[32];

    /* Erase screen */
    clear_from(0);

    /* Name, Sex, Race, Class */
    put_str("Name    :", 1, 1);
    put_str("Sex     :", 2, 1);
    put_str("Race    :", 1, 27);
    put_str("Class   :", 2, 27);

    c_put_str(TERM_L_BLUE, op_ptr->full_name, 1, 11);
    c_put_str(TERM_L_BLUE, sp_ptr->title, 2, 11);

    c_put_str(TERM_L_BLUE, rp_ptr->name, 1, 37);
    c_put_str(TERM_L_BLUE, cp_ptr->name, 2, 37);

    /* Header and Footer */
    put_str("abcdefghijkl", 3, 25);

    /* Display the stats */
    for (i = 0; i < A_MAX; i++) {
	/* Assume uppercase stat name */
	put_str(stat_names[i], 4 + i, 0);

	/* Indicate natural maximum */
	if (p_ptr->stat_max[i] == 18 + 100) {
	    put_str("!", 4 + i, 3);
	}

	/* Obtain the current stat (modified) */
	cnv_stat(p_ptr->state.stat_use[i], buf, sizeof(buf));

	/* Display the current stat (modified) */
	c_put_str(TERM_L_GREEN, buf, 4 + i, 8);
    }

    /* Process equipment */
    for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
	/* Access object */
	o_ptr = &p_ptr->inventory[i];

	/* Initialize color based of sign of pval. */
	for (j = 0; j < A_MAX; j++) {
	    /* Initialize color based of sign of bonus. */
	    a = TERM_SLATE;
	    c = L'.';

	    /* Boost */
	    if (o_ptr->bonus_stat[j] != 0) {
		/* Default */
		c = L'*';

		/* Good */
		if (o_ptr->bonus_stat[j] > 0) {
		    /* Good */
		    a = TERM_L_GREEN;

		    /* Label boost */
		    if (o_ptr->bonus_stat[j] < 10)
			c = L'0' + o_ptr->bonus_stat[j];
		}

		/* Bad */
		if (o_ptr->bonus_stat[j] < 0) {
		    /* Bad */
		    a = TERM_RED;

		    /* Label boost */
		    if (o_ptr->bonus_stat[j] < 10)
			c = L'0' - o_ptr->bonus_stat[j];
		}
	    }

	    /* Sustain */
	    if (of_has(o_ptr->id_obj, OF_SUSTAIN_STR + j)) {
		/* Dark green, "s" if no stat bonus. */
		a = TERM_GREEN;
		if (c == L'.')
		    c = L's';
	    }

	    /* Dump proper character */
	    Term_putch(i + 1, 4 + j, a, c);
	}

    }

    /* Dump the fighting bonuses to hit/dam */

    put_str("       (Fighting)    ", 14, 1);
    prt_num("Blows/Round      ", p_ptr->state.num_blow, 15, 1, TERM_L_BLUE);
    prt_num("+ to Skill       ", show_m_tohit, 16, 1, TERM_L_BLUE);

    if (show_m_todam >= 0)
	prt_num("Deadliness (%)   ", deadliness_conversion[show_m_todam], 17, 1,
		TERM_L_BLUE);
    else
	prt_num("Deadliness (%)   ", -deadliness_conversion[-show_m_todam], 17,
		1, TERM_L_BLUE);

    /* Dump the shooting bonuses to hit/dam */

    put_str("       (Shooting)    ", 14, 26);

    prt_deci("Shots/Round   ", p_ptr->state.num_fire / 10, p_ptr->state.num_fire % 10, 15,
	     26, TERM_L_BLUE);

    prt_num("+ to Skill      ", show_a_tohit, 16, 26, TERM_L_BLUE);

    if (show_a_todam > 0)
	prt_num("Deadliness (%)  ", deadliness_conversion[show_a_todam], 17, 26,
		TERM_L_BLUE);
    else
	prt_num("Deadliness (%)  ", -deadliness_conversion[-show_a_todam], 17,
		26, TERM_L_BLUE);

    /* Dump the base and bonus armor class */
    put_str("AC/+ To AC", 21, 26);

    sprintf(tmp, "%3d", p_ptr->state.dis_ac);
    c_put_str(TERM_L_BLUE, tmp, 21, 41);

    put_str("/", 21, 44);

    sprintf(tmp, "%3d", p_ptr->state.dis_to_a);
    c_put_str(TERM_L_BLUE, tmp, 21, 45);

    prt_num("Level            ", (int) p_ptr->lev, 19, 1, TERM_L_GREEN);

    prt_lnum("Experience    ", p_ptr->exp, 20, 1, TERM_L_GREEN);

    prt_lnum("Max Exp       ", p_ptr->max_exp, 21, 1, TERM_L_GREEN);

    prt_lnum("Exp to Adv.  ", (s32b) (player_exp[p_ptr->lev - 1]), 19, 26,
	     TERM_L_GREEN);

    prt_lnum("Gold         ", p_ptr->au, 20, 26, TERM_L_GREEN);

    prt_num("Max Hit Points   ", p_ptr->mhp, 11, 1, TERM_L_GREEN);

    prt_num("Cur Hit Points   ", p_ptr->chp, 12, 1, TERM_L_GREEN);


    prt_num("Max SP (Mana)   ", p_ptr->msp, 11, 26, TERM_L_GREEN);

    prt_num("Cur SP (Mana)   ", p_ptr->csp, 12, 26, TERM_L_GREEN);


}

/** 
 * Get the right colour for a given resistance
 */
byte resist_colour(int resist_value)
{
    if (resist_value == 0)
	return TERM_WHITE;
    else if (resist_value <= 20)
	return TERM_BLUE;
    else if (resist_value <= 80)
	return TERM_L_GREEN;
    else if (resist_value < 100)
	return TERM_YELLOW;
    else if (resist_value == 100)
	return TERM_L_WHITE;
    else if (resist_value < 130)
	return TERM_ORANGE;
    else if (resist_value < 160)
	return TERM_L_RED;
    else
	return TERM_RED;
}


/** 
 * Make a char_attr array of character information for file dump or screen
 * reading.  Mode 0 and 1 show 24 lines of player info for minor windows.
 */
extern int make_dump(char_attr_line * line, int mode)
{
    int i, j, x, y, col;
    bool quiver_empty = TRUE;

    const char *paren = ")";

    int k, which = 0;

    store_type *st_ptr = NULL;

    char o_name[120];

    char buf[100];
    char buf1[20];
    char buf2[20];

    bool red;

    int show_m_tohit = p_ptr->state.dis_to_h;
    int show_a_tohit = p_ptr->state.dis_to_h;
    int show_m_todam = p_ptr->state.dis_to_d;
    int show_a_todam = p_ptr->state.dis_to_d;

    object_type *o_ptr;
    int value;

    int xthn, xthb, xfos, xsrh;
    int xdis, xdev, xsav, xstl;
    const char *desc;

    int n;

    u32b flag;
    const char *name1;

    int player_resists[MAX_P_RES];
    int player_bonus[MAX_P_BONUS];

    bitflag player_flags_obj[OF_SIZE];

    int current_line = 0;

    bool dead = FALSE;
    bool have_home = (p_ptr->home != 0);

    /* Get the store number of the home */
    if (have_home) {
	if (OPT(adult_dungeon))
	    which = NUM_TOWNS_SMALL * 4 + STORE_HOME;
	else {
	    for (k = 0; k < NUM_TOWNS; k++) {
		/* Found the town */
		if (p_ptr->home == towns[k]) {
		    which += (k < NUM_TOWNS_SMALL ? 3 : STORE_HOME);
		    break;
		}
		/* Next town */
		else
		    which +=
			(k <
			 NUM_TOWNS_SMALL ? MAX_STORES_SMALL : MAX_STORES_BIG);
	    }
	}

	/* Activate the store */
	st_ptr = &store[which];
    }

    dump_ptr = (char_attr *) &line[current_line];

    /* Hack - skip all this for mode 1 */
    if (mode != 1) {
	/* Begin dump */
	sprintf(buf, "[FAangband %s Character Dump]", VERSION_STRING);
	dump_put_str(TERM_WHITE, buf, 2);
	current_line++;

	/* Start of player screen mode 0 */
	if (mode == 0) {
	    current_line = 0;

	    /* Hack - clear the top line */
	    dump_put_str(TERM_WHITE, "", 0);
	}

	current_line++;

	dump_ptr = (char_attr *) &line[current_line];

	/* Name, Sex, Race, Class */
	dump_put_str(TERM_WHITE, "Name    : ", 1);
	dump_put_str(TERM_L_BLUE, op_ptr->full_name, 11);
	dump_put_str(TERM_WHITE, "Age", 27);
	sprintf(buf1, "%10d", (int) p_ptr->age);
	dump_put_str(TERM_L_BLUE, buf1, 42);
	red = (p_ptr->stat_cur[0] < p_ptr->stat_max[0]);
	value = p_ptr->state.stat_use[0];
	cnv_stat(value, buf1, sizeof(buf1));
	value = p_ptr->state.stat_top[0];
	cnv_stat(value, buf2, sizeof(buf2));
	dump_put_str(TERM_WHITE, (red ? "Str" : "STR"), 53);
	dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[0] == 18 + 100) ? "!" : " "),
		     56);
	if (red) {
	    dump_put_str(TERM_YELLOW, buf1, 61);
	    dump_put_str(TERM_L_GREEN, buf2, 70);
	} else
	    dump_put_str(TERM_L_GREEN, buf1, 61);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "Sex     : ", 1);
	dump_put_str(TERM_L_BLUE, sp_ptr->title, 11);
	dump_put_str(TERM_WHITE, "Height", 27);
	sprintf(buf1, "%10d", (int) p_ptr->ht);
	dump_put_str(TERM_L_BLUE, buf1, 42);
	red = (p_ptr->stat_cur[1] < p_ptr->stat_max[1]);
	value = p_ptr->state.stat_use[1];
	cnv_stat(value, buf1, sizeof(buf1));
	value = p_ptr->state.stat_top[1];
	cnv_stat(value, buf2, sizeof(buf2));
	dump_put_str(TERM_WHITE, (red ? "Int" : "INT"), 53);
	dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[1] == 18 + 100) ? "!" : " "),
		     56);
	if (red) {
	    dump_put_str(TERM_YELLOW, buf1, 61);
	    dump_put_str(TERM_L_GREEN, buf2, 70);
	} else
	    dump_put_str(TERM_L_GREEN, buf1, 61);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "Race    : ", 1);
	dump_put_str(TERM_L_BLUE, rp_ptr->name, 11);
	dump_put_str(TERM_WHITE, "Weight", 27);
	sprintf(buf1, "%10d", (int) p_ptr->wt);
	dump_put_str(TERM_L_BLUE, buf1, 42);
	red = (p_ptr->stat_cur[2] < p_ptr->stat_max[2]);
	value = p_ptr->state.stat_use[2];
	cnv_stat(value, buf1, sizeof(buf1));
	value = p_ptr->state.stat_top[2];
	cnv_stat(value, buf2, sizeof(buf2));
	dump_put_str(TERM_WHITE, (red ? "Wis" : "WIS"), 53);
	dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[2] == 18 + 100) ? "!" : " "),
		     56);
	if (red) {
	    dump_put_str(TERM_YELLOW, buf1, 61);
	    dump_put_str(TERM_L_GREEN, buf2, 70);
	} else
	    dump_put_str(TERM_L_GREEN, buf1, 61);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "Class   : ", 1);
	dump_put_str(TERM_L_BLUE, cp_ptr->name, 11);
	dump_put_str(TERM_WHITE, "Social Class", 27);
	sprintf(buf1, "%10d", (int) p_ptr->sc);
	dump_put_str(TERM_L_BLUE, buf1, 42);
	red = (p_ptr->stat_cur[3] < p_ptr->stat_max[3]);
	value = p_ptr->state.stat_use[3];
	cnv_stat(value, buf1, sizeof(buf1));
	value = p_ptr->state.stat_top[3];
	cnv_stat(value, buf2, sizeof(buf2));
	dump_put_str(TERM_WHITE, (red ? "Dex" : "DEX"), 53);
	dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[3] == 18 + 100) ? "!" : " "),
		     56);
	if (red) {
	    dump_put_str(TERM_YELLOW, buf1, 61);
	    dump_put_str(TERM_L_GREEN, buf2, 70);
	} else
	    dump_put_str(TERM_L_GREEN, buf1, 61);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	if (p_ptr->total_winner)
	    dump_put_str(TERM_VIOLET, "***WINNER***", 0);
	red = (p_ptr->stat_cur[4] < p_ptr->stat_max[4]);
	value = p_ptr->state.stat_use[4];
	cnv_stat(value, buf1, sizeof(buf1));
	value = p_ptr->state.stat_top[4];
	cnv_stat(value, buf2, sizeof(buf2));
	dump_put_str(TERM_WHITE, (red ? "Con" : "CON"), 53);
	dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[4] == 18 + 100) ? "!" : " "),
		     56);
	if (red) {
	    dump_put_str(TERM_YELLOW, buf1, 61);
	    dump_put_str(TERM_L_GREEN, buf2, 70);
	} else
	    dump_put_str(TERM_L_GREEN, buf1, 61);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	red = (p_ptr->stat_cur[5] < p_ptr->stat_max[5]);
	value = p_ptr->state.stat_use[5];
	cnv_stat(value, buf1, sizeof(buf1));
	value = p_ptr->state.stat_top[5];
	cnv_stat(value, buf2, sizeof(buf2));
	dump_put_str(TERM_WHITE, (red ? "Chr" : "CHR"), 53);
	dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[5] == 18 + 100) ? "!" : " "),
		     56);
	if (red) {
	    dump_put_str(TERM_YELLOW, buf1, 61);
	    dump_put_str(TERM_L_GREEN, buf2, 70);
	} else
	    dump_put_str(TERM_L_GREEN, buf1, 61);
	current_line += 2;

	/* Get the bonuses to hit/dam */

	o_ptr = &p_ptr->inventory[INVEN_WIELD];
	if (if_has(o_ptr->id_other, IF_TO_H))
	    show_m_tohit += o_ptr->to_h;
	if (if_has(o_ptr->id_other, IF_TO_D))
	    show_m_todam += o_ptr->to_d;

	o_ptr = &p_ptr->inventory[INVEN_BOW];
	if (if_has(o_ptr->id_other, IF_TO_H))
	    show_a_tohit += o_ptr->to_h;
	if (if_has(o_ptr->id_other, IF_TO_D))
	    show_a_todam += o_ptr->to_d;

	dump_ptr = (char_attr *) &line[current_line];
	dump_num("Max Hit Points   ", p_ptr->mhp, 1, TERM_L_GREEN);
	if (p_ptr->lev >= p_ptr->max_lev)
	    dump_num("Level            ", (int) p_ptr->lev, 27, TERM_L_GREEN);
	else
	    dump_num("Level            ", (int) p_ptr->lev, 27, TERM_YELLOW);
	dump_num("Max SP (Mana)    ", p_ptr->msp, 53, TERM_L_GREEN);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	if (p_ptr->chp >= p_ptr->mhp)
	    dump_num("Cur Hit Points   ", p_ptr->chp, 1, TERM_L_GREEN);
	else if (p_ptr->chp > (p_ptr->mhp * op_ptr->hitpoint_warn) / 10)
	    dump_num("Cur Hit Points   ", p_ptr->chp, 1, TERM_YELLOW);
	else
	    dump_num("Cur Hit Points   ", p_ptr->chp, 1, TERM_RED);
	if (p_ptr->exp >= p_ptr->max_exp)
	    dump_lnum("Experience    ", p_ptr->exp, 27, TERM_L_GREEN);
	else
	    dump_lnum("Experience    ", p_ptr->exp, 27, TERM_YELLOW);
	if (p_ptr->csp >= p_ptr->msp)
	    dump_num("Cur SP (Mana)    ", p_ptr->csp, 53, TERM_L_GREEN);
	else if (p_ptr->csp > (p_ptr->msp * op_ptr->hitpoint_warn) / 10)
	    dump_num("Cur SP (Mana)    ", p_ptr->csp, 53, TERM_YELLOW);
	else
	    dump_num("Cur SP (Mana)    ", p_ptr->csp, 53, TERM_RED);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_lnum("Max Exp       ", p_ptr->max_exp, 27, TERM_L_GREEN);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "(Fighting)", 8);
	if (p_ptr->lev >= PY_MAX_LEVEL) {
	    dump_put_str(TERM_WHITE, "Exp to Adv.   ", 27);
	    sprintf(buf, "       *****");
	} else
	    dump_lnum("Exp to Adv.   ", (s32b) (player_exp[p_ptr->lev - 1]), 27,
		      TERM_L_GREEN);
	dump_put_str(TERM_WHITE, "(Shooting)", 60);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_num("Blows/Round      ", p_ptr->state.num_blow, 1, TERM_L_BLUE);
	dump_lnum("Gold          ", p_ptr->au, 27, TERM_L_GREEN);
	dump_deci("Shots/Round    ", p_ptr->state.num_fire / 10, p_ptr->state.num_fire % 10,
		  53, TERM_L_BLUE);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	dump_num("+ to Skill       ", show_m_tohit, 1, TERM_L_BLUE);
	dump_put_str(TERM_WHITE, "Score", 27);
	sprintf(buf1, "%12d", (int) p_ptr->score);
	dump_put_str(TERM_L_GREEN, buf1, 38);
	dump_num("+ to Skill       ", show_a_tohit, 53, TERM_L_BLUE);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	/* Show damage per blow if no weapon wielded */
	if (!p_ptr->inventory[INVEN_WIELD].k_idx) {
	    int sum = 0;

	    for (i = 0; i < 12; i++)
		sum += (int) p_ptr->barehand_dam[i];
	    dump_num("Av. Damage/Blow  ", sum / 12, 1, TERM_L_BLUE);
	} else {
	    if (show_m_todam >= 0)
		dump_num("Deadliness (%)   ",
			 deadliness_conversion[show_m_todam], 1, TERM_L_BLUE);
	    else
		dump_num("Deadliness (%)   ",
			 -deadliness_conversion[-show_m_todam], 1, TERM_L_BLUE);
	}

	dump_put_str(TERM_WHITE, "Base AC/+ To AC", 27);
	sprintf(buf1, "%3d", p_ptr->state.dis_ac);
	dump_put_str(TERM_L_BLUE, buf1, 43);
	dump_put_str(TERM_WHITE, "/", 46);
	sprintf(buf1, "%3d", p_ptr->state.dis_to_a);
	dump_put_str(TERM_L_BLUE, buf1, 47);

	if (show_a_todam > 0)
	    dump_num("Deadliness (%)   ", deadliness_conversion[show_a_todam],
		     53, TERM_L_BLUE);
	else
	    dump_num("Deadliness (%)   ", -deadliness_conversion[-show_a_todam],
		     53, TERM_L_BLUE);

	current_line++;
	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "Game Turn", 27);
	sprintf(buf1, "%12d", (int) turn);
	dump_put_str(TERM_L_GREEN, buf1, 38);
	current_line += 2;

	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "(Character Abilities)", 28);


	/* Fighting Skill (with current weapon) */
	xthn =
	    p_ptr->state.skills[SKILL_TO_HIT_MELEE] +
	    (show_m_tohit * BTH_PLUS_ADJ);

	/* Shooting Skill (with current bow and normal missile) */
	xthb =
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] +
	    (show_a_tohit * BTH_PLUS_ADJ);

	/* Basic abilities */
	xdis = p_ptr->state.skills[SKILL_DISARM];
	xdev = p_ptr->state.skills[SKILL_DEVICE];
	xsav = p_ptr->state.skills[SKILL_SAVE];
	xstl = p_ptr->state.skills[SKILL_STEALTH];
	xsrh = p_ptr->state.skills[SKILL_SEARCH];
	xfos = p_ptr->state.skills[SKILL_SEARCH_FREQUENCY];


	desc = likert(xthn, 10);
	dump_put_str(TERM_WHITE, "Fighting   :", 1);
	dump_put_str(likert_color, desc, 13);

	desc = likert(xstl, 1);
	dump_put_str(TERM_WHITE, "Stealth    :", 27);
	dump_put_str(likert_color, desc, 39);

	desc = likert(xdis, 8);
	dump_put_str(TERM_WHITE, "Disarming  :", 53);
	dump_put_str(likert_color, desc, 65);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	desc = likert(xthb, 10);
	dump_put_str(TERM_WHITE, "Bows/Throw :", 1);
	dump_put_str(likert_color, desc, 13);

	desc = likert(xfos, 6);
	dump_put_str(TERM_WHITE, "Perception :", 27);
	dump_put_str(likert_color, desc, 39);

	desc = likert(xdev, 8);
	dump_put_str(TERM_WHITE, "MagicDevice:", 53);
	dump_put_str(likert_color, desc, 65);
	current_line++;

	dump_ptr = (char_attr *) &line[current_line];
	desc = likert(xsav, 7);
	dump_put_str(TERM_WHITE, "SavingThrow:", 1);
	dump_put_str(likert_color, desc, 13);

	desc = likert(xsrh, 6);
	dump_put_str(TERM_WHITE, "Searching  :", 27);
	dump_put_str(likert_color, desc, 39);

sprintf(buf1, "%d feet", p_ptr->state.see_infra * 10);
	dump_put_str(TERM_WHITE, "Infravision:", 53);
	dump_put_str(TERM_WHITE, buf1, 65);
	current_line += 2;


	/* Display history */
	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "(Character Background)", 28);

	current_line++;
	dump_ptr = (char_attr *) &line[current_line];
	text_out_dump(TERM_WHITE, p_ptr->history, &line, &current_line, 10, 60);
	

	/* End of mode 0 */
	if (mode == 0)
	    return (current_line);

	current_line += 2;

	/* Current, recent and recall points */
	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "[Recent locations]", 2);
	current_line += 2;

	/* Current, previous */
	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "Current Location :", 1);
	sprintf(buf, "%s Level %d",
		locality_name[stage_map[p_ptr->stage][LOCALITY]], p_ptr->depth);
	dump_put_str(TERM_L_GREEN, buf, 20);
	current_line++;

	if (p_ptr->last_stage != 0) {
	    dump_ptr = (char_attr *) &line[current_line];
	    dump_put_str(TERM_WHITE, "Previous Location:", 1);
	    sprintf(buf, "%s Level %d",
		    locality_name[stage_map[p_ptr->last_stage][LOCALITY]],
		    stage_map[p_ptr->last_stage][DEPTH]);
	    dump_put_str(TERM_L_GREEN, buf, 20);
	    current_line++;
	}

	/* Recall points */
	for (i = 0; i < 4; i++) {
	    if (p_ptr->recall[i] == 0)
		continue;
	    dump_ptr = (char_attr *) &line[current_line];
	    sprintf(buf, "Recall Point %d   :", i + 1);
	    dump_put_str(TERM_WHITE, buf, 1);
	    sprintf(buf, "%s Level %d",
		    locality_name[stage_map[p_ptr->recall[i]][LOCALITY]],
		    stage_map[p_ptr->recall[i]][DEPTH]);
	    dump_put_str(TERM_L_GREEN, buf, 20);
	    current_line++;
	}


	/* Heading */
	current_line++;
	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "[Resistances, Powers and Bonuses]", 2);
	current_line += 2;
    }

    /* Start of mode 1 player screen */
    if (mode == 1) {
	current_line = 0;

	/* Hack - clear all the lines */
	for (i = 0; i < 25; i++) {
	    dump_ptr = (char_attr *) &line[i];
	    dump_put_str(TERM_WHITE, "", 0);
	}
    }

    dump_ptr = (char_attr *) &line[current_line];

    /* Header */
    dump_put_str(TERM_WHITE, "abcdefghijkl@            abcdefghijkl@", 6);
    current_line++;

    /* Resistances */
    get_player_resists(player_resists);

    for (y = 0; y < 7; y++) {
	dump_ptr = (char_attr *) &line[current_line];

	for (x = 0; x < 2; x++) {
	    int r = y + 7 * x;

	    /* Extract name */
	    name1 = display_player_resist_names[x][y];

	    /* Name */
	    dump_put_str(resist_colour(p_ptr->state.dis_res_list[y + x * 7]), 
			 name1, 1 + 24 * x);

	    /* Check equipment */
	    for (n = 6 + 25 * x, i = INVEN_WIELD; i < INVEN_TOTAL; i++, n++) {
		object_type *o_ptr;

		/* Object */
		o_ptr = &p_ptr->inventory[i];

		/* Check flags */
		if ((o_ptr->k_idx)
		    && if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + r)) {
		    if (o_ptr->percent_res[r] == RES_LEVEL_MIN)
			dump_put_str(resist_colour(o_ptr->percent_res[r]), "*",
				     n);
		    else if (o_ptr->percent_res[r] < RES_LEVEL_BASE)
			dump_put_str(resist_colour(o_ptr->percent_res[r]), "+",
				     n);
		    else if (o_ptr->percent_res[r] > RES_LEVEL_BASE)
			dump_put_str(resist_colour(o_ptr->percent_res[r]), "-",
				     n);
		}

		/* Default */
		else {
		    dump_put_str((o_ptr->k_idx ? TERM_L_WHITE : TERM_SLATE),
				 ".", n);
		}
	    }

	    /* Check flags */
	    if (player_resists[r] == RES_LEVEL_MIN)
		dump_put_str(resist_colour(player_resists[r]), "*", n);
	    else if (player_resists[r] < RES_LEVEL_BASE)
		dump_put_str(resist_colour(player_resists[r]), "+", n);
	    else if (player_resists[r] > RES_LEVEL_BASE)
		dump_put_str(resist_colour(player_resists[r]), "-", n);
	    else
		dump_put_str(TERM_L_WHITE, ".", n);

	    /* Percentage */
	    sprintf(buf1, "%4d%%", 100 - p_ptr->state.dis_res_list[y + x * 7]);
	    dump_put_str(resist_colour(p_ptr->state.dis_res_list[y + x * 7]), buf1,
			 n + 1);
	}
	current_line++;
    }
    /* Skip the gap for mode 1 */
    if (mode != 1) {
	current_line++;
	dump_ptr = (char_attr *) &line[current_line];

	/* Header */
	dump_put_str(TERM_WHITE, "abcdefghijkl@            abcdefghijkl@", 6);
	current_line++;
    }

    /* Powers and Bonuses */
    for (y = 0; y < 10; y++) {
	byte a = TERM_WHITE;
	char c;

	dump_ptr = (char_attr *) &line[current_line];

	for (x = 0; x < 2; x++) {
	    /* Extract flag */
	    flag = display_player_powers[y];

	    /* Extract name */
	    name1 = display_player_power_names[y];

	    /* Name */
	    dump_put_str(TERM_WHITE, name1, 0);

	    /* Check equipment */
	    for (n = 6, i = INVEN_WIELD; i < INVEN_TOTAL; i++, n++) {
		object_type *o_ptr;
		bitflag objflags[OF_SIZE];

		/* Object */
		o_ptr = &p_ptr->inventory[i];

		/* Set flags */
		of_wipe(objflags);
		of_copy(objflags, o_ptr->flags_obj);
		of_inter(objflags, o_ptr->id_obj);

		/* Check flags */
		if ((o_ptr->k_idx) && of_has(objflags, flag)) {
		    dump_put_str(TERM_WHITE, "+", n);
		}

		/* Default */
		else {
		    dump_put_str((o_ptr->k_idx ? TERM_L_WHITE : TERM_SLATE),
				 ".", n);
		}
	    }

	    /* Check flags */
	    player_flags(player_flags_obj);

	    if (of_has(player_flags_obj, flag)) {
		dump_put_str(TERM_WHITE, "+", n);
	    }

	    else {
		/* Player 'reversed' flags */
		player_weakness_dis(player_flags_obj);

		/* Check flags */
		if (of_has(player_flags_obj, flag)) {
		    dump_put_str(TERM_RED, "-", n);
		}

		else
		    /* Default */
		    dump_put_str(TERM_L_WHITE, ".", n);
	    }

	    /* Hack - only 8 bonuses */
	    if (y >= MAX_P_BONUS)
		continue;

	    /* Extract name */
	    name1 = display_player_bonus_names[y];

	    /* Name */
	    dump_put_str(TERM_WHITE, name1, 25);

	    /* Check equipment */
	    for (n = 31, i = INVEN_WIELD; i < INVEN_TOTAL; i++, n++) {
		object_type *o_ptr;

		/* Object */
		o_ptr = &p_ptr->inventory[i];

		/* Check flags */
		if (o_ptr->bonus_other[y] != BONUS_BASE) {
		    /* Default */
		    c = '*';

		    /* Good */
		    if (o_ptr->bonus_other[y] > 0) {
			/* Good */
			a = TERM_L_GREEN;

			/* Label boost */
			if (o_ptr->bonus_other[y] < 10)
			    c = '0' + o_ptr->bonus_other[y];
		    }

		    /* Bad */
		    if (o_ptr->bonus_other[y] < 0) {
			/* Bad */
			a = TERM_RED;

			/* Label boost */
			if (o_ptr->bonus_other[y] < 10)
			    c = '0' - o_ptr->bonus_other[y];
		    }

		    /* Dump proper character */
		    buf[0] = c;
		    buf[1] = '\0';
		    dump_put_str(a, buf, n);
		}

		/* Default */
		else {
		    dump_put_str((o_ptr->k_idx ? TERM_L_WHITE : TERM_SLATE),
				 ".", n);
		}
	    }

	    /* Player flags */
	    get_player_bonus(player_bonus);

	    /* Check flags */
	    if (player_bonus[y] != BONUS_BASE) {
		/* Default */
		c = '*';

		/* Good */
		if (player_bonus[y] > 0) {
		    /* Good */
		    a = TERM_L_GREEN;

		    /* Label boost */
		    if (player_bonus[y] < 10)
			c = '0' + player_bonus[y];
		}

		/* Bad */
		if (player_bonus[y] < 0) {
		    /* Bad */
		    a = TERM_RED;

		    /* Label boost */
		    if (player_bonus[y] < 10)
			c = '0' - player_bonus[y];
		}

		/* Dump proper character */
		buf[0] = c;
		buf[1] = '\0';
		dump_put_str(a, buf, n);
	    }

	    /* Default */
	    else {
		dump_put_str(TERM_L_WHITE, ".", n);
	    }
	}
	current_line++;
    }

    /* Skip for mode 1 */
    if (mode != 1) {
	/* Skip some lines */
	current_line += 2;

	/* Dump specialties if any */
	if (p_ptr->specialty_order[0] != PF_NO_SPECIALTY) {
	    dump_ptr = (char_attr *) &line[current_line];
	    dump_put_str(TERM_WHITE, "[Specialty Abilities]", 2);
	    current_line += 2;

	    for (i = 0; i < 10; i++) {
		if (p_ptr->specialty_order[i] != PF_NO_SPECIALTY) {
		    dump_ptr = (char_attr *) &line[current_line];
		    sprintf(buf, "%s %s",
			    specialty_names[p_ptr->specialty_order[i]],
			    (i >=
			     p_ptr->specialties_allowed) ? "(forgotten)" : "");
		    dump_put_str(TERM_GREEN, buf, 0);
		    current_line++;
		}
	    }
	    current_line += 2;
	}

	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "[Stat Breakdown]", 2);
	current_line += 2;
    }

    dump_ptr = (char_attr *) &line[current_line];


    /* Print out the labels for the columns */
    dump_put_str(TERM_WHITE, "Stat", 0);
    dump_put_str(TERM_BLUE, "Intrnl", 5);
    dump_put_str(TERM_L_BLUE, "Rce Cls Oth", 12);
    dump_put_str(TERM_L_GREEN, "Actual", 24);
    dump_put_str(TERM_YELLOW, "Currnt", 31);
    dump_put_str(TERM_WHITE, "abcdefghijkl", 42);
    current_line++;

    /* Display the stats */
    for (i = 0; i < A_MAX; i++) {
	dump_ptr = (char_attr *) &line[current_line];

	/* Reduced name of stat */
	dump_put_str(TERM_WHITE, stat_names_reduced[i], 0);

	/* Internal "natural" maximum value */
	cnv_stat(p_ptr->stat_max[i], buf, sizeof(buf));
	dump_put_str(TERM_BLUE, buf, 5);

	/* Race, class, and equipment modifiers */
	sprintf(buf, "%3d", rp_ptr->r_adj[i]);
	dump_put_str(TERM_L_BLUE, buf, 12);
	sprintf(buf, "%3d", cp_ptr->c_adj[i]);
	dump_put_str(TERM_L_BLUE, buf, 16);
	sprintf(buf, "%3d", p_ptr->state.stat_add[i]);
	dump_put_str(TERM_L_BLUE, buf, 20);

	/* Resulting "modified" maximum value */
	cnv_stat(p_ptr->state.stat_top[i], buf, sizeof(buf));
	dump_put_str(TERM_L_GREEN, buf, 24);

	/* Only display stat_use if not maximal */
	if (p_ptr->state.stat_use[i] < p_ptr->state.stat_top[i]) {
	    cnv_stat(p_ptr->state.stat_use[i], buf, sizeof(buf));
	    dump_put_str(TERM_YELLOW, buf, 31);
	}

	for (j = INVEN_WIELD; j < INVEN_TOTAL; j++) {
	    byte a;
	    char c;

	    col = 42 + j - INVEN_WIELD;

	    /* Access object */
	    o_ptr = &p_ptr->inventory[j];

	    /* Initialize color based of sign of bonus. */
	    a = TERM_SLATE;
	    c = '.';

	    /* Boost */
	    if (o_ptr->bonus_stat[i] != 0) {
		/* Default */
		c = '*';

		/* Good */
		if (o_ptr->bonus_stat[i] > 0) {
		    /* Good */
		    a = TERM_L_GREEN;

		    /* Label boost */
		    if (o_ptr->bonus_stat[i] < 10)
			c = '0' + o_ptr->bonus_stat[i];
		}

		/* Bad */
		if (o_ptr->bonus_stat[i] < 0) {
		    /* Bad */
		    a = TERM_RED;

		    /* Label boost */
		    if (o_ptr->bonus_stat[i] < 10)
			c = '0' - o_ptr->bonus_stat[i];
		}
	    }

	    /* Sustain */
	    if (of_has(o_ptr->flags_obj, OF_SUSTAIN_STR + i)) {
		/* Dark green, "s" if no stat bonus. */
		a = TERM_GREEN;
		if (c == '.')
		    c = 's';
	    }

	    /* Dump proper character */
	    buf[0] = c;
	    buf[1] = '\0';
	    dump_put_str(a, buf, col);
	}

	current_line++;
    }

    /* End of mode 1 */
    if (mode == 1)
	return (current_line);

    current_line++;

    /* If dead, dump last messages -- Prfnoff */
    if (p_ptr->is_dead) {
	dump_ptr = (char_attr *) &line[current_line];
	i = messages_num();
	if (i > 15)
	    i = 15;
	dump_put_str(TERM_WHITE, "[Last Messages]", 2);
	current_line += 2;
	while (i-- > 0) {
	    dump_ptr = (char_attr *) &line[current_line];
	    sprintf(buf, "> %s", message_str((s16b) i));
	    dump_put_str(TERM_WHITE, buf, 0);
	    current_line++;
	}
	current_line += 2;
    }

    /* Dump the equipment */
    if (p_ptr->equip_cnt) {
	dump_ptr = (char_attr *) &line[current_line];
	dump_put_str(TERM_WHITE, "[Character Equipment]", 2);
	current_line += 2;
	for (i = INVEN_WIELD; i < ALL_INVEN_TOTAL; i++) {
	    if (i == INVEN_TOTAL) {
		for (j = 0; j < 10; j++) {
		    object_desc(o_name, sizeof(o_name), 
				&p_ptr->inventory[i + j + 1],
				ODESC_PREFIX | ODESC_FULL);
		    if (!streq(o_name, "(nothing)"))
			quiver_empty = FALSE;
		}
		if (!quiver_empty) {
		    current_line++;
		    dump_ptr = (char_attr *) &line[current_line];
		    dump_put_str(TERM_WHITE, "[Quiver]", 9);
		    current_line++;
		}
	    }

	    else {
		dump_ptr = (char_attr *) &line[current_line];
		object_desc(o_name, sizeof(o_name), 
			    &p_ptr->inventory[i],
			    ODESC_PREFIX | ODESC_FULL);
		if (streq(o_name, "(nothing)"))
		    continue;
		sprintf(buf, "%c%s %s", index_to_label(i), paren, o_name);
		dump_put_str(proc_list_color_hack(&p_ptr->inventory[i]), buf, 0);
		current_line++;
	dump_ptr = (char_attr *) &line[current_line];
	object_info_chardump(&p_ptr->inventory[i], &line, &current_line, 5, 75);
	current_line++;

	    }
	}
	current_line += 2;
    }

    /* Dump the inventory */
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, "[Character Inventory]", 2);
    current_line += 2;
    for (i = 0; i < INVEN_PACK; i++) {
	if (!p_ptr->inventory[i].k_idx)
	    break;

	dump_ptr = (char_attr *) &line[current_line];
	object_desc(o_name, sizeof(o_name), &p_ptr->inventory[i],
		    ODESC_PREFIX | ODESC_FULL);
	sprintf(buf, "%c%s %s", index_to_label(i), paren, o_name);
	dump_put_str(proc_list_color_hack(&p_ptr->inventory[i]), buf, 0);
	current_line++;
	dump_ptr = (char_attr *) &line[current_line];
	object_info_chardump(&p_ptr->inventory[i], &line, &current_line, 5, 75);
	current_line++;

    }
    current_line += 2;

    /* Dump the Home -- if anything there */
    if (have_home) {
	if (st_ptr->stock_num) {
	    dump_ptr = (char_attr *) &line[current_line];

	    /* Header */
	    dump_put_str(TERM_WHITE, "[Home Inventory]", 2);
	    current_line += 2;

	    /* Dump all available items */
	    for (i = 0; i < st_ptr->stock_num; i++) {
		dump_ptr = (char_attr *) &line[current_line];
		object_desc(o_name, sizeof(o_name), &st_ptr->stock[i],
			    ODESC_PREFIX | ODESC_FULL);
		sprintf(buf, "%c) %s", I2A(i), o_name);
		dump_put_str(proc_list_color_hack(&st_ptr->stock[i]), buf, 0);
		current_line++;
		dump_ptr = (char_attr *) &line[current_line];
		object_info_chardump(&st_ptr->stock[i], &line, &current_line, 5, 75);
		current_line++;
	    }

	    /* Add an empty line */
	    current_line += 2;
	}
    }

    /* Add in "character start" information */
    dump_ptr = (char_attr *) &line[current_line];
    sprintf(buf, "%s the %s %s", op_ptr->full_name, rp_ptr->name, cp_ptr->name);
    dump_put_str(TERM_WHITE, buf, 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, notes_start, 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, "============================================================", 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, "CHAR.", 34);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, "|   TURN  |      LOCATION        |LEVEL| EVENT",
		 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, "============================================================", 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];

    /* Dump notes */
    dump_history(line, &current_line, &dead);
    dump_put_str(TERM_WHITE,
		 "============================================================",
		 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];

    /* Fake note */
    if (!dead) {
	char info_note[43];
	char place[32];

	/* Get the location name */
	if (p_ptr->depth)
	    strnfmt(place, sizeof(place), "%15s%4d ",
		    locality_name[stage_map[p_ptr->stage][LOCALITY]],
		    p_ptr->depth);
	else if ((stage_map[p_ptr->stage][LOCALITY] != UNDERWORLD)
		 && (stage_map[p_ptr->stage][LOCALITY] != MOUNTAIN_TOP))
	    strnfmt(place, sizeof(place), "%15s Town",
		    locality_name[stage_map[p_ptr->stage][LOCALITY]]);
	else
	    strnfmt(place, sizeof(place), "%15s     ",
		    locality_name[stage_map[p_ptr->stage][LOCALITY]]);

	/* Make preliminary part of note */
	strnfmt(info_note, sizeof(info_note), "%10d%22s%5d    ", turn,
		place, p_ptr->lev);

	/* Write the info note */
	dump_put_str(TERM_WHITE, info_note, 0);
	dump_put_str(TERM_VIOLET, "Still alive", strlen(info_note));

	current_line++;
	dump_ptr = (char_attr *) &line[current_line];
    }
    dump_put_str(TERM_WHITE,
		 "============================================================",
		 0);
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];


    /* Dump options */
    current_line++;
    dump_ptr = (char_attr *) &line[current_line];
    dump_put_str(TERM_WHITE, "[Options]", 2);
    current_line += 2;

    /* Dump options */
    for (i = OPT_ADULT + 4; i < OPT_SCORE; i++) {
	if (option_desc(i)) {
	    dump_ptr = (char_attr *) &line[current_line];
	    sprintf(buf, "%-49s: %s (%s)", option_desc(i),
		    op_ptr->opt[i] ? "yes" : "no ", option_name(i));
	    dump_put_str(TERM_WHITE, buf, 0);
	    current_line++;
	}
    }

    for (i = OPT_SCORE; i < OPT_MAX; i++) {
	if (option_desc(i)) {
	    dump_ptr = (char_attr *) &line[current_line];
	    sprintf(buf, "%-49s: %s (%s)", option_desc(i),
		    op_ptr->opt[i] ? "yes" : "no ", option_name(i));
	    dump_put_str(TERM_WHITE, buf, 0);
	    current_line++;
	}
    }

    /* Success */
    return (current_line);
}

/**
 * Show the character screen
 */

void display_dump(char_attr_line * line, int top_line, int bottom_line, int col)
{
    int i;
    char_attr *shifted_line;

    /* Start at the top */
    dump_row = 0;

    /* Set the hook */
    dump_line_hook = dump_line_screen;

    /* Dump the lines */
    for (i = top_line; i < bottom_line; i++) {
	shifted_line = (char_attr *) &line[i];
	shifted_line += col;
	dump_line(shifted_line);
    }
}

/**
 * Write a character dump to a file
 */

errr file_character(const char *name, char_attr_line * line, int last_line)
{
    int i;
    char buf[100];

    /* Drop priv's */
    safe_setuid_drop();

    /* Build the filename */
    path_build(buf, 1024, ANGBAND_DIR_USER, name);

    /* Check for existing file */
    if (file_exists(buf)) {
	char out_val[160];

	/* Build query */
	sprintf(out_val, "Replace existing file %s? ", buf);

	/* Ask */
	if (!get_check(out_val))
	    return (-1);
    }

    /* Open the non-existing file */
    dump_out_file = file_open(buf, MODE_WRITE, FTYPE_TEXT);

    /* Grab priv's */
    safe_setuid_grab();


    /* Invalid file */
    if (!dump_out_file) {
	/* Message */
	msg("Character dump failed!");
	message_flush();

	/* Error */
	return (-1);
    }

    /* Set the hook */
    dump_line_hook = dump_line_file;

    /* Dump the lines */
    for (i = 0; i < last_line; i++) {
	dump_ptr = (char_attr *) &line[i];
	dump_line(dump_ptr);
    }

    /* Close it */
    file_close(dump_out_file);

    /* Message */
    msg("Character dump successful.");
    message_flush();

    /* Success */
    return (0);
}


/**
 * Make a string lower case.
 */
static void string_lower(char *buf)
{
    const char *buf_ptr;

    /* No string */
    if (!buf)
	return;

    /* Lower the string */
    for (buf_ptr = buf; *buf_ptr != 0; buf_ptr++) {
	buf[buf_ptr - buf] = tolower(*buf_ptr);
    }
}

/**
 * Keep track of how recursed the file showing is 
 */
static int push_file = 0;

/**
 * Recursive file perusal.
 *
 * Return FALSE on "ESCAPE", otherwise TRUE.
 *
 * Process various special text in the input file, including
 * the "menu" structures used by the "help file" system.
 *
 * XXX XXX XXX Consider using a temporary file.
 *
 * XXX XXX XXX Allow the user to "save" the current file.
 */

#define MAX_BUF 1024

bool show_file(const char *name, const char *what, int line, int mode)
{
    int i, k, n;
    int wid, hgt;
    int ret;
    ui_event ke;

    /* Number of "real" lines passed by */
    int next = 0;

    /* Number of "real" lines in the file */
    int size = 0;

    /* Backup value for "line" */
    int back = 0;

    /* This screen has sub-screens */
    bool menu = FALSE;

    /* Case sensitive search */
    bool case_sensitive = FALSE;

    /* HACK!! -NRM- */
    ang_file *fff = NULL;

    /* Find this string (if any) */
    const char *find = NULL;

    /* Jump to this tag */
    const char *tag = NULL;

    /* Hold a string to find */
    char finder[81] = "";

    /* Hold a string to show */
    char shower[81] = "";

    /* Filename */
    char *filename;

    /* Describe this thing */
    char caption[128] = "";

    /* Path buffer */
    char *path;

    /* General buffer */
    char *buf;

    /* Small screen back up buffer */
    char *buf2;

    /* Lower case version of the buffer, for searching */
    char *lc_buf;

    /* Sub-menu information */
    char hook[10][32];

    /* Sub-menu mouse position */
    int mouse[24];

    /* mallocs */
    filename = malloc(MAX_BUF);
    path = malloc(MAX_BUF);
    buf = malloc(MAX_BUF);
    buf2 = malloc(MAX_BUF);
    lc_buf = malloc(MAX_BUF);

    /* Get size */
    Term_get_size(&wid, &hgt);

    /* Wipe the hooks */
    for (i = 0; i < 10; i++) {
	hook[i][0] = '\0';
    }

    /* Wipe the mouse menu */
    for (i = 0; i < 24; i++) {
	mouse[i] = 0;
    }

    /* Copy the filename */
    my_strcpy(filename, name, MAX_BUF);

    n = strlen(filename);

    /* Extract the tag from the filename */
    for (i = 0; i < n; i++) {
	if (filename[i] == '#') {
	    filename[i] = '\0';
	    tag = filename + i + 1;
	    break;
	}
    }

    /* Redirect the name */
    name = filename;

    /* Hack XXX XXX XXX */
    if (what) {
	my_strcpy(caption, what, sizeof(caption));

	my_strcpy(path, name, MAX_BUF);
	fff = file_open(path, MODE_READ, -1);
    }

    /* Look in "help" */
    if (!fff) {
	strnfmt(caption, sizeof(caption), "Help file '%s'", name);

	path_build(path, MAX_BUF, ANGBAND_DIR_HELP, name);
	fff = file_open(path, MODE_READ, -1);
    }

    /* Look in "info" */
    if (!fff) {
	strnfmt(caption, sizeof(caption), "Info file '%s'", name);

	path_build(path, sizeof(path), ANGBAND_DIR_INFO, name);
	fff = file_open(path, MODE_READ, -1);
    }

    /* Oops */
    if (!fff) {
	/* Message */
	msg("Cannot open '%s'.", name);
	message_flush();

	/* Oops */
	ret = TRUE;
	goto DONE;

    }

    /* Note we're entering the file */
    push_file++;

    /* Pre-Parse the file */
    while (TRUE) {
	/* Read a line or stop */
	if (!file_getl(fff, buf, MAX_BUF))
	    break;

	/* Check for a mouseable line (note hex parentheses) */
	if ((buf[4] == 0x28) && (isdigit(buf[5])) && (buf[6] == 0x29)) {
	    mouse[next + 2] = D2I(buf[5]);
	}

	/* XXX Parse "menu" items */
	if (prefix(buf, "***** ")) {
	    char b1 = '[', b2 = ']';

	    /* Notice "menu" requests */
	    if ((buf[6] == b1) && isdigit(buf[7]) && (buf[8] == b2)
		&& (buf[9] == ' ')) {
		/* This is a menu file */
		menu = TRUE;

		/* Extract the menu item */
		k = D2I(buf[7]);

		/* Extract the menu item */
		strcpy(hook[k], buf + 10);
	    }

	    /* Notice "tag" requests */
	    else if (buf[6] == '<') {
		if (tag) {
		    /* Remove the closing '>' of the tag */
		    buf[strlen(buf) - 1] = '\0';

		    /* Compare with the requested tag */
		    if (streq(buf + 7, tag)) {
			/* Remember the tagged line */
			line = next;
		    }
		}
	    }

	    /* Skip this */
	    continue;
	}

	/* Count the "real" lines */
	next++;
    }

    /* Save the number of "real" lines */
    size = next;



    /* Display the file */
    while (TRUE) {
	char prompt[80];

	/* Clear screen */
	Term_clear();


	/* Restart when necessary */
	if (line >= size)
	    line = 0;


	/* Re-open the file if needed */
	if (next > line) {
	    /* Close it */
	    file_close(fff);

	    /* Hack -- Re-Open the file */
	    fff = file_open(path, MODE_READ, -1);
	    if (!fff) {
		/* Leaving */
		push_file--;
		ret = TRUE;
		goto DONE;
	    }

	    /* File has been restarted */
	    next = 0;
	}

	/* Goto the selected line */
	while (next < line) {

	    /* Get a line */
	    if (!file_getl(fff, buf, MAX_BUF))
		break;

	    /* Skip tags/links */
	    if (prefix(buf, "***** "))
		continue;

	    /* Count the lines */
	    next++;
	}



	/* Dump the next hgt - 4 lines of the file */
	for (i = 0; i < hgt - 4;) {
	    /* Hack -- track the "first" line */
	    if (!i)
		line = next;

	    /* Get a line of the file or stop */
	    if (!file_getl(fff, buf, MAX_BUF))
		break;

	    /* Hack -- skip "special" lines */
	    if (prefix(buf, "***** "))
		continue;

	    /* Count the "real" lines */
	    next++;

	    /* Make a copy of the current line for searching */
	    my_strcpy(lc_buf, buf, sizeof(lc_buf));

	    /* Make the line lower case */
	    if (!case_sensitive)
		string_lower(lc_buf);

	    /* Hack -- keep searching */
	    if (find && !i && !strstr(lc_buf, find))
		continue;

	    /* Hack -- stop searching */
	    find = NULL;

	    /* Dump the line */
	    Term_putstr(0, i + 2, -1, TERM_WHITE, buf);

	    /* Hilight "shower" */
	    if (shower[0]) {
		const char *str = lc_buf;

		/* Display matches */
		while ((str = strstr(str, shower)) != NULL) {
		    int len = strlen(shower);

		    /* Display the match */
		    Term_putstr(str - lc_buf, i + 2, len, TERM_YELLOW,
				&buf[str - lc_buf]);

		    /* Advance */
		    str += len;
		}
	    }

	    /* Count the printed lines */
	    i++;
	}

	/* Hack -- failed search */
	if (find) {
	    bell("Search string not found!");
	    line = back;
	    find = NULL;
	    continue;
	}


	/* Show a general "title" */
	prt(format
	    ("[FAangband %d.%d.%d, %s, Line %d/%d]", VERSION_MAJOR,
	     VERSION_MINOR, VERSION_PATCH, caption, line, size), 0, 0);


	/* Buttons */
	button_kill_all();
	button_add("ESC", ESCAPE);
	button_add("?", '?');

	/* Prompt -- menu screen */
	if (menu) {
	    /* Wait for it */
	    strncpy(prompt,
		    "[Press a number, ESC for previous file or ? to exit]",
		    80);
	}

	/* Prompt -- small files */
	else if (size <= hgt - 4) {
	    /* Wait for it */
	    strncpy(prompt, "[Press ESC for previous file, ? to exit.]", 80);
	}

	/* Prompt -- large files */
	else {
	    /* Wait for it */
	    strncpy(prompt,
		    "[Space to advance, ESC for previous file, or ? to exit.]",
		    80);
	    
	    
	    /* More buttons */
	    button_add("Spc", ' ');
	    button_add("-", '-');
	}

	/* Finish the status line */
	prt(prompt, hgt - 1, 0);
	button_add("/", '/');
	button_add("!", '!');
	button_add("=", '=');
	if (!menu)
	    button_add("#", '#');
	button_add("%", '%');

	/* Get a keypress */
	ke = inkey_ex();

	/* Mouse input - menus */
	if ((menu) && (ke.mouse.y <= 24) && (ke.mouse.y > 0) && 
	    (mouse[ke.mouse.y])) 
	{
	    /* Recurse on that file */
	    if (!show_file(hook[mouse[ke.mouse.y]], NULL, 0, mode))
		ke.key.code = '?';
	}

	/* Hack -- return to last screen on escape */
	if (ke.key.code == ESCAPE)
	    break;


	/* Toggle case sensitive on/off */
	if (ke.key.code == '!') {
	    case_sensitive = !case_sensitive;
	}

	/* Hack -- try showing */
	if (ke.key.code == '=') {
	    /* Get "shower" */
	    prt("Show: ", hgt - 1, 0);
	    (void) askfor_aux(shower, 80, NULL);

	    /* Make the "shower" lowercase */
	    if (!case_sensitive)
		string_lower(shower);
	}

	/* Hack -- try finding */
	if (ke.key.code == '/') {
	    /* Get "finder" */
	    prt("Find: ", hgt - 1, 0);
	    if (askfor_aux(finder, 80, NULL)) {
		/* Find it */
		find = finder;
		back = line;
		line = line + 1;

		/* Make the "finder" lowercase */
		if (!case_sensitive)
		    string_lower(finder);

		/* Show it */
		strcpy(shower, finder);
	    }
	}

	/* Hack -- go to a specific line */
	if (ke.key.code == '#') {
	    char tmp[81];
	    prt("Goto Line: ", hgt - 1, 0);
	    strcpy(tmp, "0");
	    if (askfor_aux(tmp, 80, NULL)) {
		line = atoi(tmp);
	    }
	}

	/* Hack -- go to a specific file */
	if (ke.key.code == '%') {
	    char tmp[81];
	    prt("Goto File: ", hgt - 1, 0);
	    strcpy(tmp, "help.hlp");
	    if (askfor_aux(tmp, 80, NULL)) {
		if (!show_file(tmp, NULL, 0, mode))
		    ke.key.code = '?';
	    }
	}

	/* Back up one line */
	if (ke.key.code == ARROW_UP || ke.key.code == '8') {
	    line = line - 1;
	}

	/* Hack -- Allow backing up */
	if ((ke.key.code == '-') || (ke.key.code == '9')) {
	    line = line - 10;
	    if (line < 0)
		line = 0;
	}

	/* Hack -- Advance a single line */
	if ((ke.key.code == KC_ENTER) || (ke.key.code == '2')
	    || (ke.key.code == ARROW_DOWN)) {
	    line = line + 1;
	}

	/* Advance one page */
	if ((ke.key.code == ' ') || (ke.key.code == '3')) {
	    line = line + hgt - 4;
	}

	/* Recurse on numbers */
	if (menu && isdigit(ke.key.code) && hook[D2I(ke.key.code)][0]) {
	    /* Recurse on that file */
	    if (!show_file(hook[D2I(ke.key.code)], NULL, 0, mode))
		ke.key.code = '?';
	}

	/* Exit on '?' */
	if (ke.key.code == '?')
	    break;
    }

    /* Close the file */
    file_close(fff);
    push_file--;

    /* Normal return */
    ret = TRUE;

    /* Exit on '?' */
    if (ke.key.code == '?')
	ret = FALSE;

  DONE:
    free(filename);
    free(path);
    free(buf);
    free(buf2);
    free(lc_buf);

    if (push_file == 0)
	button_restore();

    return ret;

}


/**
 * Peruse the On-Line-Help
 */
void do_cmd_help(void)
{
    /* Save screen */
    screen_save();

    /* Peruse the main help file */
    (void) show_file("help.hlp", NULL, 0, 0);

    /* Load screen */
    screen_load();
}


/**
 * Process the player name.
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(bool sf)
{
    int i, k = 0;


    /* Cannot be too long */
    if (strlen(op_ptr->full_name) > 15) {
	/* Name too long */
	quit_fmt("The name '%s' is too long!", op_ptr->full_name);
    }

    /* Cannot contain "icky" characters */
    for (i = 0; op_ptr->full_name[i]; i++) {
	/* No control characters */
	if (iscntrl(op_ptr->full_name[i])) {
	    /* Illegal characters */
	    quit_fmt("The name '%s' contains control chars!",
		     op_ptr->full_name);
	}
    }


#ifdef MACINTOSH

    /* Extract "useful" letters */
    for (i = 0; op_ptr->full_name[i]; i++) {
	char c = op_ptr->full_name[i];

	/* Convert "colon" and "period" */
	if ((c == ':') || (c == '.'))
	    c = '_';

	/* Accept all the letters */
	op_ptr->base_name[k++] = c;
    }

#else

    /* Extract "useful" letters */
    for (i = 0; op_ptr->full_name[i]; i++) {
	char c = op_ptr->full_name[i];

	/* Accept some letters */
	if (isalpha(c) || isdigit(c))
	    op_ptr->base_name[k++] = c;

	/* Convert space, dot, and underscore to underscore */
	else if (strchr(". _", c))
	    op_ptr->base_name[k++] = '_';
    }

#endif


#if defined(WINDOWS) || defined(MSDOS)

    /* Hack -- max length */
    if (k > 8)
	k = 8;

#endif

    /* Terminate */
    op_ptr->base_name[k] = '\0';

    /* Require a "base" name */
    if (!op_ptr->base_name[0])
	strcpy(op_ptr->base_name, "PLAYER");


#ifdef SAVEFILE_MUTABLE

    /* Accept */
    sf = TRUE;

#endif

    /* Change the savefile name */
    if (sf) {
	char temp[128];

#ifdef SAVEFILE_USE_UID
	/* Rename the savefile, using the player_uid and base_name */
	sprintf(temp, "%d.%s", player_uid, op_ptr->base_name);
#else
	/* Rename the savefile, using the base name */
	sprintf(temp, "%s", op_ptr->base_name);
#endif

#ifdef VM
	/* Hack -- support "flat directory" usage on VM/ESA */
	sprintf(temp, "%s.sv", op_ptr->base_name);
#endif				/* VM */

	/* Build the filename */
#ifdef _WIN32_WCE
	/* SJG */
	/* Rename the savefile, using the base name + .faa */
	sprintf(temp, "%s.faa", op_ptr->base_name);

	// The common open file dialog doesn't like
	// anything being farther up than one directory!
	// For now hard code it. I should probably roll my
	// own open file dailog.
	path_build(savefile, 1024, "\\My Documents\\FA", temp);
#else
	path_build(savefile, 1024, ANGBAND_DIR_SAVE, temp);
#endif
    }
}




/**
 * Save the game
 */
void save_game(void)
{
    /* Disturb the player */
    disturb(1, 0);

    /* Clear messages */
    message_flush();

    /* Handle stuff */
    handle_stuff(p_ptr);

    /* Message */
    if (!is_autosave)
	prt("Saving game...", 0, 0);

    /* Refresh */
    Term_fresh();

    /* The player is not dead */
    my_strcpy(p_ptr->died_from, "(saved)", sizeof(p_ptr->died_from));

    /* Forbid suspend */
    signals_ignore_tstp();

    /* Save the player */
    if (savefile_save(savefile)) {
	if (!is_autosave)
	    prt("Saving game... done.", 0, 0);
    }

    /* Save failed (oops) */
    else {
	prt("Saving game... failed!", 0, 0);
    }

    /* Allow suspend again */
    signals_handle_tstp();

    /* Refresh */
    Term_fresh();

    /* Note that the player is not dead */
	my_strcpy(p_ptr->died_from, "(alive and well)", sizeof(p_ptr->died_from));
}




/**
 * Close up the current game (player may or may not be dead)
 *
 * This function is called only from "main.c" and "signals.c".
 *
 * Note that the savefile is not saved until the tombstone is
 * actually displayed and the player has a chance to examine
 * the inventory and such.  This allows cheating if the game
 * is equipped with a "quit without save" method.  XXX XXX XXX
 */
void close_game(void)
{
    ui_event ke;

    /* Handle stuff */
    handle_stuff(p_ptr);

    /* Flush the messages */
    message_flush();

    /* Flush the input */
    flush();


    /* No suspending now */
    signals_ignore_tstp();


    /* Hack -- Increase "icky" depth */
    character_icky++;

    /* Handle death */
    if (p_ptr->is_dead) 
    {
	death_screen();
    }

    /* Still alive */
    else 
    {
	/* Save the game */
	save_game();

	if (Term->mapped_flag)
	{
	    /* Prompt for scores XXX XXX XXX */
	    prt("Press Return (or Escape).", 0, 40);

	    /* Predict score (or ESCAPE) */
	    ke = inkey_ex();
	    if (ke.key.code != ESCAPE) predict_score();
	}
    }

    /* Hack -- Decrease "icky" depth */
    character_icky--;

    /* Allow suspending now */
    signals_handle_tstp();
}


/**
 * Handle abrupt death of the visual system
 *
 * This routine is called only in very rare situations, and only
 * by certain visual systems, when they experience fatal errors.
 *
 * XXX XXX Hack -- clear the death flag when creating a HANGUP
 * save file so that player can see tombstone when restart.
 */
void exit_game_panic(void)
{
    /* If nothing important has happened, just quit */
    if (!character_generated || character_saved)
	quit("panic");

    /* Mega-Hack -- see "msg()" */
    msg_flag = FALSE;

    /* Clear the top line */
    prt("", 0, 0);

    /* Hack -- turn off some things */
    disturb(1, 0);

    /* Hack -- Delay death XXX XXX XXX */
    if (p_ptr->chp < 0)
	p_ptr->is_dead = FALSE;

    /* Hardcode panic save p_ptr->panic_save = 1; */

    /* Forbid suspend */
    signals_ignore_tstp();

    /* Indicate panic save */
    strcpy(p_ptr->died_from, "(panic save)");

    /* Panic save, or get worried */
    if (!savefile_save(savefile))
	quit("panic save failed!");

    /* Successful panic save */
    quit("panic save succeeded!");
}


/**
 * Taken from Zangband.  What a good idea! 
 */
errr get_rnd_line(char *file_name, char *output)
{
    ang_file *fp;
    char buf[1024];
    int lines = 0, line, counter;

    /* Build the filename */
    path_build(buf, 1024, ANGBAND_DIR_FILE, file_name);

    /* Open the file */
    fp = file_open(buf, MODE_READ, FTYPE_TEXT);

    /* Failed */
    if (!fp)
	return (-1);

    /* Parse the file */
    if (file_getl(fp, buf, 80))
	lines = atoi(buf);
    else
	return (1);

    /* choose a random line */
    line = randint1(lines);

    for (counter = 0; counter <= line; counter++) {
	if (!(file_getl(fp, buf, 80)))
	    return (1);
	else if (counter == line)
	    break;
    }

    strcpy(output, buf);

    /* Close the file */
    file_close(fp);

    return (0);
}





static void write_html_escape_char(ang_file * htm, wchar_t c)
{
    switch (c) 
    {
    case L'<':
	file_putf(htm, "&lt;");
	break;
    case L'>':
	file_putf(htm, "&gt;");
	break;
    case L'&':
	file_putf(htm, "&amp;");
	break;
    default:
    {
	char *mbseq = (char*) mem_alloc(sizeof(char)*(MB_CUR_MAX+1));
	byte len;
	len = wctomb(mbseq, c);
	if (len > MB_CUR_MAX) len = MB_CUR_MAX;
	mbseq[len] = '\0';
	file_putf(htm, "%s", mbseq);
	mem_free(mbseq);
	break;
    }
    }
}

/* Take an html screenshot */
void html_screenshot(const char *name, int mode)
{
	int y, x;
	int wid, hgt;

	int a = TERM_WHITE;
	int oa = TERM_WHITE;
	wchar_t c = L' ';

	const char *new_color_fmt = (mode == 0) ?
					"<font color=\"#%02X%02X%02X\">"
				 	: "[COLOR=\"#%02X%02X%02X\"]";
	const char *change_color_fmt = (mode == 0) ?
					"</font><font color=\"#%02X%02X%02X\">"
					: "[/COLOR][COLOR=\"#%02X%02X%02X\"]";
	const char *close_color_fmt = mode ==  0 ? "</font>" : "[/COLOR]";

	ang_file *fp;
	char buf[1024];


	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	fp = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fp)
	{
		plog_fmt("Cannot write the '%s' file!", buf);
		return;
	}

	/* Retrieve current screen size */
	Term_get_size(&wid, &hgt);

	if (mode == 0)
	{
		file_putf(fp, "<!DOCTYPE html><html><head>\n");
		file_putf(fp, "  <meta='generator' content='%s %s'>\n",
	            	VERSION_NAME, VERSION_STRING);
		file_putf(fp, "  <title>%s</title>\n", name);
		file_putf(fp, "</head>\n\n");
		file_putf(fp, "<body style='color: #fff; background: #000;'>\n");
		file_putf(fp, "<pre>\n");
	}
	else 
	{
		file_putf(fp, "[CODE][TT][BC=black][COLOR=white]\n");
	}

	/* Dump the screen */
	for (y = 0; y < hgt; y++)
	{
		for (x = 0; x < wid; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Color change */
			if (oa != a && c != L' ')
			{
				/* From the default white to another color */
				if (oa == TERM_WHITE)
				{
					file_putf(fp, new_color_fmt,
					        angband_color_table[a][1],
					        angband_color_table[a][2],
					        angband_color_table[a][3]);
				}

				/* From another color to the default white */
				else if (a == TERM_WHITE)
				{
					file_putf(fp, close_color_fmt);
				}

				/* Change colors */
				else
				{
					file_putf(fp, change_color_fmt,
					        angband_color_table[a][1],
					        angband_color_table[a][2],
					        angband_color_table[a][3]);
				}

				/* Remember the last color */
				oa = a;
			}

			/* Write the character and escape special HTML characters */
			if (mode == 0) write_html_escape_char(fp, c);
			else
			{
				char mbseq[MB_LEN_MAX+1] = {0};
				wctomb(mbseq, c);
				file_putf(fp, "%s", mbseq);
			}
		}

		/* End the row */
		file_putf(fp, "\n");
	}

	/* Close the last font-color tag if necessary */
	if (oa != TERM_WHITE) file_putf(fp, close_color_fmt);

	if (mode == 0)
	{
		file_putf(fp, "</pre>\n");
		file_putf(fp, "</body>\n");
		file_putf(fp, "</html>\n");
	}
	else 
	{
		file_putf(fp, "[/COLOR][/BC][/TT][/CODE]\n");
	}

	/* Close it */
	file_close(fp);
}
