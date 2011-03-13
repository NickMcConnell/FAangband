/** \file xtra1.c 
    \brief Display of character data

 * Display of stats to the user from internal figures, char info shown on 
 * main screen and status displays, monster health bar, display various
 * things in sub-windows, spell management, calculation of max mana, max
 * HP, light radius, and weight limit.  Apply and display all modifiers,
 * attributes, etc. of the player, his gear, and temporary conditions to
 * the player.  Includes all racial and class attributes, effects of Bless
 * and the like, encumbrance, blows table inputs, and over-heavy weapons.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick & Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
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
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val)
{
    /* Above 18 */
    if (val > 18) {
	int bonus = (val - 18);

	if (bonus >= 220) {
	    sprintf(out_val, "18/%3s", "***");
	} else if (bonus >= 100) {
	    sprintf(out_val, "18/%03d", bonus);
	} else {
	    sprintf(out_val, " 18/%02d", bonus);
	}
    }

    /* From 3 to 18 */
    else {
	sprintf(out_val, "    %2d", val);
    }
}




/**
 * Print character info at given row, column in a 13 char field
 */
static void prt_field(cptr info, int row, int col)
{
    /* Dump 12 spaces to clear */
    if (SIDEBAR_WID == 12)
	c_put_str(TERM_WHITE, "            ", row, col);

    /* Dump 13 spaces to clear */
    else
	c_put_str(TERM_WHITE, "             ", row, col);

    /* Dump the info itself */
    c_put_str(TERM_L_BLUE, info, row, col);
}




/**
 * Print character stat in given row, column
 */
static void prt_stat(int stat)
{
    char tmp[32];

    /* Display "injured" stat */
    if (p_ptr->stat_cur[stat] < p_ptr->stat_max[stat]) {
	put_str(stat_names_reduced[stat], ROW_STAT + stat, COL_STAT);
	cnv_stat(p_ptr->state.stat_use[stat], tmp);
	c_put_str(TERM_YELLOW, tmp, ROW_STAT + stat, COL_STAT + 6);
    }

    /* Display "healthy" stat */
    else {
	put_str(stat_names[stat], ROW_STAT + stat, COL_STAT);
	cnv_stat(p_ptr->state.stat_use[stat], tmp);
	c_put_str(TERM_L_GREEN, tmp, ROW_STAT + stat, COL_STAT + 6);
    }

    /* Indicate natural maximum */
    if (p_ptr->stat_max[stat] == 18 + 100) {
	put_str("!", ROW_STAT + stat, COL_STAT + 3);
    }
}




/**
 * Prints "title", including "wizard" or "winner" as needed.
 */
static void prt_title(void)
{
    cptr p = "";

    /* Wizard */
    if (p_ptr->wizard) {
	p = "[=-WIZARD-=]";
    }

    /* Winner */
    else if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL)) {
	p = "***WINNER***";
    }

    /* Normal */
    else {
	p = c_text + cp_ptr->title[(p_ptr->lev - 1) / 5];
    }

    prt_field(p, ROW_TITLE, COL_TITLE);
}


/**
 * Prints level
 */
static void prt_level(void)
{
    char tmp[32];

    sprintf(tmp, "%6d", p_ptr->lev);

    if (p_ptr->lev >= p_ptr->max_lev) {
	put_str("LEVEL ", ROW_LEVEL, 0);
	c_put_str(TERM_L_GREEN, tmp, ROW_LEVEL, COL_LEVEL + 6);
    } else {
	put_str("Level ", ROW_LEVEL, 0);
	c_put_str(TERM_YELLOW, tmp, ROW_LEVEL, COL_LEVEL + 6);
    }
}


/**
 * Display the experience
 */
static void prt_exp(void)
{
    char out_val[8];

    if (p_ptr->lev < PY_MAX_LEVEL) {
	long val = (long) ((player_exp[p_ptr->lev - 1]) - p_ptr->exp);

	/* Boundary check */
	if (val < 0)
	    val = 0;

	sprintf(out_val, "%7ld", val);

	if (p_ptr->exp >= p_ptr->max_exp) {
	    put_str("NEXT ", ROW_EXP, 0);
	    c_put_str(TERM_L_GREEN, out_val, ROW_EXP, COL_EXP + 5);
	} else {
	    put_str("Next ", ROW_EXP, 0);
	    c_put_str(TERM_YELLOW, out_val, ROW_EXP, COL_EXP + 5);
	}
    } else {
	put_str("NEXT ", ROW_EXP, 0);
	c_put_str(TERM_L_GREEN, "*******", ROW_EXP, COL_EXP + 5);
    }
}

/**
 * Prints current gold
 */
static void prt_gold(void)
{
    char tmp[32];

    put_str("AU ", ROW_GOLD, COL_GOLD);
    sprintf(tmp, "%9ld", (long) p_ptr->au);
    c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);
}

/**
 * Prints current shape, if not normal.   -LM-
 */
static void prt_shape(void)
{
    char *shapedesc = "";

    switch (p_ptr->schange) {
    case SHAPE_MOUSE:
	shapedesc = "Mouse     ";
	break;
    case SHAPE_FERRET:
	shapedesc = "Ferret    ";
	break;
    case SHAPE_HOUND:
	shapedesc = "Hound     ";
	break;
    case SHAPE_GAZELLE:
	shapedesc = "Gazelle   ";
	break;
    case SHAPE_LION:
	shapedesc = "Lion      ";
	break;
    case SHAPE_ENT:
	shapedesc = "Ent       ";
	break;
    case SHAPE_BAT:
	shapedesc = "Bat       ";
	break;
    case SHAPE_WEREWOLF:
	shapedesc = "Werewolf  ";
	break;
    case SHAPE_VAMPIRE:
	shapedesc = "Vampire   ";
	break;
    case SHAPE_WYRM:
	shapedesc = "Dragon    ";
	break;
    case SHAPE_BEAR:
	shapedesc = "Bear      ";
	break;
    default:
	shapedesc = "          ";
	break;
    }

    /* Display (or write over) the shapechange with pretty colors. */
    if (mp_ptr->spell_book == TV_DRUID_BOOK)
	c_put_str(TERM_GREEN, shapedesc, ROW_SHAPE, COL_SHAPE);
    else if (mp_ptr->spell_book == TV_NECRO_BOOK)
	c_put_str(TERM_VIOLET, shapedesc, ROW_SHAPE, COL_SHAPE);
    else
	c_put_str(TERM_RED, shapedesc, ROW_SHAPE, COL_SHAPE);

}


/**
 * Prints current AC
 */
static void prt_ac(void)
{
    char tmp[32];

    put_str("Cur AC ", ROW_AC, COL_AC);
    sprintf(tmp, "%5d", p_ptr->state.dis_ac + p_ptr->state.dis_to_a);
    c_put_str(TERM_L_GREEN, tmp, ROW_AC, COL_AC + 7);
}


/**
 * Prints Cur/Max hit points
 */
static void prt_hp(void)
{
    char tmp[32];
    int len;
    byte color;

    put_str("HP          ", ROW_HP, COL_HP);

    len = sprintf(tmp, "%d:%d", p_ptr->chp, p_ptr->mhp);

    c_put_str(TERM_L_GREEN, tmp, ROW_HP, COL_HP + 12 - len);

    /* Done? */
    if (p_ptr->chp >= p_ptr->mhp)
	return;

    if (p_ptr->chp > (p_ptr->mhp * op_ptr->hitpoint_warn) / 10) {
	color = TERM_YELLOW;
    } else {
	color = TERM_RED;
    }

    /* Show current hitpoints using another color */
    sprintf(tmp, "%d", p_ptr->chp);

    c_put_str(color, tmp, ROW_HP, COL_HP + 12 - len);
}


/**
 * Prints players max/cur spell points
 */
static void prt_sp(void)
{
    char tmp[32];
    byte color;
    int len;

    /* Do not show mana unless it matters */
    if (!mp_ptr->spell_book)
	return;


    put_str("SP          ", ROW_SP, COL_SP);

    len = sprintf(tmp, "%d:%d", p_ptr->csp, p_ptr->msp);

    c_put_str(TERM_L_GREEN, tmp, ROW_SP, COL_SP + 12 - len);

    /* Done? */
    if (p_ptr->csp >= p_ptr->msp)
	return;

    if (p_ptr->csp > (p_ptr->msp * op_ptr->hitpoint_warn) / 10) {
	color = TERM_YELLOW;
    } else {
	color = TERM_RED;
    }


    /* Show current mana using another color */
    sprintf(tmp, "%d", p_ptr->csp);

    c_put_str(color, tmp, ROW_SP, COL_SP + 12 - len);
}


/**
 * Prints depth in stat area
 */
static void prt_depth(void)
{
    char loc[32];
    s16b attr = TERM_L_BLUE;
    int region, level;

    region = stage_map[p_ptr->stage][LOCALITY];

    level = stage_map[p_ptr->stage][DEPTH];

    if (small_screen) {
	if (level)
	    sprintf(loc, "%s%3d", short_locality_name[region], level);
	else
	    sprintf(loc, "%s", short_locality_name[region]);
    } else {
	if (level)
	    sprintf(loc, "%s %d", locality_name[region], level);
	else
	    sprintf(loc, "%s", locality_name[region]);
    }

    /* Get color of level based on feeling -JSV- */
    if ((p_ptr->depth) && (do_feeling)) {
	if (p_ptr->themed_level)
	    attr = TERM_BLUE;
	else if (feeling == 1)
	    attr = TERM_VIOLET;
	else if (feeling == 2)
	    attr = TERM_RED;
	else if (feeling == 3)
	    attr = TERM_L_RED;
	else if (feeling == 4)
	    attr = TERM_ORANGE;
	else if (feeling == 5)
	    attr = TERM_ORANGE;
	else if (feeling == 6)
	    attr = TERM_YELLOW;
	else if (feeling == 7)
	    attr = TERM_YELLOW;
	else if (feeling == 8)
	    attr = TERM_WHITE;
	else if (feeling == 9)
	    attr = TERM_WHITE;
	else if (feeling == 10)
	    attr = TERM_L_WHITE;
    }

    /* Right-Adjust the "depth", and clear old values */
    if (small_screen)
	c_prt(attr, format("%8s", loc), Term->hgt - 1, Term->wid - 9);
    else
	c_prt(attr, format("%19s", loc), Term->hgt - 1, Term->wid - 22);

    /* Record the column this display starts */
    depth_start = Term->wid - (small_screen ? 9 : 22);
}

/* ------------------------------------------------------------------------
 * Status line display functions
 * ------------------------------------------------------------------------ */

/** Simple macro to initialise structs */
#define S(s)		s, sizeof(s)

/**
 * Struct to describe different timed effects
 */
struct state_info {
    int value;
    const char *str;
    size_t len;
    byte attr;
};

/** p_ptr->hunger descriptions */
static const struct state_info hunger_data[] = {
    {PY_FOOD_FAINT, S("Faint"), TERM_RED},
    {PY_FOOD_WEAK, S("Weak"), TERM_ORANGE},
    {PY_FOOD_ALERT, S("Hungry"), TERM_YELLOW},
    {PY_FOOD_FULL, S(""), TERM_L_GREEN},
    {PY_FOOD_MAX, S("Full"), TERM_L_GREEN},
    {PY_FOOD_UPPER, S("Gorged"), TERM_GREEN},
};

#define PRINT_STATE(sym, data, index, row, col) \
{ \
	size_t i; \
	\
	for (i = 0; i < N_ELEMENTS(data); i++) \
	{ \
		if (index sym data[i].value) \
		{ \
			if (data[i].str[0]) \
			{ \
				c_put_str(data[i].attr, data[i].str, row, col); \
				return data[i].len; \
			} \
			else \
			{ \
				return 0; \
			} \
		} \
	} \
}


/**
 * Prints status of hunger
 */
static size_t prt_hunger(int row, int col)
{
    PRINT_STATE(<, hunger_data, p_ptr->food, row, col);
    return 0;
}



/**
 * Prints Searching, Resting, or 'count' status
 * Display is always exactly 10 characters wide (see below)
 *
 * This function was a major bottleneck when resting, so a lot of
 * the text formatting code was optimized in place below.
 */
static size_t prt_state(int row, int col)
{
    byte attr = TERM_WHITE;

    char text[16] = "";


    /* Resting */
    if (p_ptr->resting) {
	int i;
	int n = p_ptr->resting;

	/* Start with "Rest" */
	my_strcpy(text, "Rest      ", sizeof(text));

	/* Extensive (timed) rest */
	if (n >= 1000) {
	    i = n / 100;
	    text[9] = '0';
	    text[8] = '0';
	    text[7] = I2D(i % 10);
	    if (i >= 10) {
		i = i / 10;
		text[6] = I2D(i % 10);
		if (i >= 10) {
		    text[5] = I2D(i / 10);
		}
	    }
	}

	/* Long (timed) rest */
	else if (n >= 100) {
	    i = n;
	    text[9] = I2D(i % 10);
	    i = i / 10;
	    text[8] = I2D(i % 10);
	    text[7] = I2D(i / 10);
	}

	/* Medium (timed) rest */
	else if (n >= 10) {
	    i = n;
	    text[9] = I2D(i % 10);
	    text[8] = I2D(i / 10);
	}

	/* Short (timed) rest */
	else if (n > 0) {
	    i = n;
	    text[9] = I2D(i);
	}

	/* Rest until healed */
	else if (n == -1) {
	    text[5] = text[6] = text[7] = text[8] = text[9] = '*';
	}

	/* Rest until done */
	else if (n == -2) {
	    text[5] = text[6] = text[7] = text[8] = text[9] = '&';
	}
    }

    /* Searching */
    else if (p_ptr->searching) {
	my_strcpy(text, "Searching ", sizeof(text));
    }

    /* Display the info (or blanks) */
    c_put_str(attr, text, row, col);

    return strlen(text);
}


/**
 * Prints the speed of a character.  		-CJS-
 */
static size_t prt_speed(int row, int col)
{
    int i = p_ptr->pspeed;

    int attr = TERM_WHITE;
    char buf[32] = "";

    /* Hack -- Visually "undo" the Search Mode Slowdown */
    if (p_ptr->searching)
	i += 10;

    /* Fast */
    if (i > 110) {
	attr = TERM_L_GREEN;
	sprintf(buf, "Fast (+%d)", (i - 110));
    }

    /* Slow */
    else if (i < 110) {
	attr = TERM_L_UMBER;
	sprintf(buf, "Slow (-%d)", (110 - i));
    }

    /* Display the speed */
    c_put_str((byte) attr, format("%-9s", buf), row, col);

    return strlen(buf);
}

/**
 * Prints trap detection status
 */
static size_t prt_dtrap(int row, int col)
{
    byte info = cave_info2[p_ptr->py][p_ptr->px];

    /* The player is in a trap-detected grid */
    if (info & (CAVE2_DTRAP)) {
	c_put_str(TERM_GREEN, "DTrap", row, col);
	return 5;
    }

    return 0;
}



/**
 * Print whether a character is studying or not.
 */
static size_t prt_study(int row, int col)
{
    if (p_ptr->new_spells) {
	char *text = format("Study (%d)", p_ptr->new_spells);
	put_str(text, row, col);
	return strlen(text) + 1;
    }

    return 0;
}

/**
 * Print whether a character is due a specialty or not.
 */
static size_t prt_spec(int row, int col)
{
    if (p_ptr->new_specialties) {
	char *text = format("Spec. (%d)", p_ptr->new_specialties);
	c_put_str(TERM_VIOLET, text, row, col);
	return strlen(text) + 1;
    }

    return 0;
}


/**
 * Print blind.
 */
static size_t prt_blind(int row, int col)
{
    if (p_ptr->timed[TMD_BLIND]) {
	c_put_str(TERM_ORANGE, "Blind", row, col);
	return 5;
    }
    return 0;
}

/**
 * Print confused.
 */
static size_t prt_confused(int row, int col)
{
    if (p_ptr->timed[TMD_CONFUSED]) {
	c_put_str(TERM_ORANGE, "Confused", row, col);
	return 8;
    }
    return 0;
}

/**
 * Print afraid.
 */
static size_t prt_afraid(int row, int col)
{
    if (p_ptr->timed[TMD_AFRAID]) {
	c_put_str(TERM_ORANGE, "Afraid", row, col);
	return 6;
    }
    return 0;
}

/**
 * Print paralyzed.
 */
static size_t prt_paralyzed(int row, int col)
{
    if (p_ptr->timed[TMD_PARALYZED]) {
	c_put_str(TERM_RED, "Paralyzed!", row, col);
	return 10;
    }
    return 0;
}

/**
 * Print poisoned.
 */
static size_t prt_poisoned(int row, int col)
{
    if (p_ptr->timed[TMD_POISONED]) {
	c_put_str(TERM_ORANGE, "Poisoned", row, col);
	return 8;
    }
    return 0;
}


/** Useful typedef */
typedef size_t status_f(int row, int col);

status_f *status_handlers[] =
    { prt_state, prt_hunger, prt_study, prt_spec, prt_blind, prt_confused,
    prt_afraid, prt_paralyzed, prt_poisoned, prt_dtrap
};


/**
 * Print the status line.
 */
extern void update_statusline(void)
{
    errr bad;
    int x, y;
    int row = Term->hgt - 1;
    int col = 0;
    int button_end = (normal_screen ? depth_start : Term->wid - 2);
    size_t i;
    int j;
    char buf[10];

    /* Save the cursor position */
    bad = Term_locate(&x, &y);

    if (normal_screen) {
	/* Clear the remainder of the line */
	prt("", row, col);

	/* Display those which need redrawing */
	for (i = 0; i < N_ELEMENTS(status_handlers); i++)
	    col += status_handlers[i] (row, col);

	/* Print speed, record where the status info ends */
	status_end = col + prt_speed(row, col);

	/* Redo the location info */
	prt_depth();
    }

    /* Reposition the cursor */
    if (!bad)
	(void) Term_gotoxy(x, y);
}


static void prt_cut(void)
{
    int c = p_ptr->timed[TMD_CUT];

    if (c > 1000) {
	c_put_str(TERM_L_RED, (OPT(bottom_status) ? "Mtl wnd" : "Mortal wound"),
		  ROW_CUT, COL_CUT);
    } else if (c > 200) {
	c_put_str(TERM_RED, (OPT(bottom_status) ? "Dp gash" : "Deep gash   "),
		  ROW_CUT, COL_CUT);
    } else if (c > 100) {
	c_put_str(TERM_RED, (OPT(bottom_status) ? "Svr cut" : "Severe cut  "),
		  ROW_CUT, COL_CUT);
    } else if (c > 50) {
	c_put_str(TERM_ORANGE,
		  (OPT(bottom_status) ? "Nst cut" : "Nasty cut   "), ROW_CUT,
		  COL_CUT);
    } else if (c > 25) {
	c_put_str(TERM_ORANGE,
		  (OPT(bottom_status) ? "Bad cut" : "Bad cut     "), ROW_CUT,
		  COL_CUT);
    } else if (c > 10) {
	c_put_str(TERM_YELLOW,
		  (OPT(bottom_status) ? "Lgt cut" : "Light cut   "), ROW_CUT,
		  COL_CUT);
    } else if (c) {
	c_put_str(TERM_YELLOW,
		  (OPT(bottom_status) ? "Graze  " : "Graze       "), ROW_CUT,
		  COL_CUT);
    } else {
	put_str((OPT(bottom_status) ? "       " : "            "), ROW_CUT,
		COL_CUT);
    }
}



static void prt_stun(void)
{
    int s = p_ptr->timed[TMD_STUN];

    if (s > 100) {
	c_put_str(TERM_RED, (OPT(bottom_status) ? "Knc out" : "Knocked out "),
		  ROW_STUN, COL_STUN);
    } else if (s > 50) {
	c_put_str(TERM_ORANGE,
		  (OPT(bottom_status) ? "Hvy stn" : "Heavy stun  "), ROW_STUN,
		  COL_STUN);
    } else if (s) {
	c_put_str(TERM_ORANGE,
		  (OPT(bottom_status) ? "Stun   " : "Stun        "), ROW_STUN,
		  COL_STUN);
    } else {
	put_str((OPT(bottom_status) ? "       " : "            "), ROW_STUN,
		COL_STUN);
    }
}



static void prt_blank(void)
{
    int i, j;

    j = (panel_extra_rows ? 2 : 0);

    if ((Term->hgt > (j + 24)) && (!OPT(bottom_status))) {
	for (i = 23; i < (Term->hgt - 1 - j); i++) {
	    put_str("            ", i, 0);
	}
    }
}


/**
 * Redraw the "monster health bar"
 *
 * The "monster health bar" provides visual feedback on the "health"
 * of the monster currently being "tracked".  There are several ways
 * to "track" a monster, including targetting it, attacking it, and
 * affecting it (and nobody else) with a ranged attack.   When nothing
 * is being tracked, we clear the health bar.  If the monster being
 * tracked is not currently visible, a special health bar is shown.
 */
static void health_redraw(void)
{
    /* Not tracking */
    if (!p_ptr->health_who) {
	/* Erase the health bar */
	Term_erase(COL_INFO, ROW_INFO, 12);
    }

    /* Tracking an unseen monster */
    else if (!m_list[p_ptr->health_who].ml) {
	/* Indicate that the monster health is "unknown" */
	Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");
    }

    /* Tracking a hallucinatory monster */
    else if (p_ptr->timed[TMD_HALLUC]) {
	/* Indicate that the monster health is "unknown" */
	Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");
    }

    /* Tracking a dead monster (???) */
    else if (!m_list[p_ptr->health_who].hp < 0) {
	/* Indicate that the monster health is "unknown" */
	Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");
    }

    /* Tracking a visible monster */
    else {
	int pct, len;

	monster_type *m_ptr = &m_list[p_ptr->health_who];

	/* Default to almost dead */
	byte attr = TERM_RED;

	/* Extract the "percent" of health */
	pct = 100L * m_ptr->hp / m_ptr->maxhp;

	/* Badly wounded */
	if (pct >= 10)
	    attr = TERM_L_RED;

	/* Wounded */
	if (pct >= 25)
	    attr = TERM_ORANGE;

	/* Somewhat Wounded */
	if (pct >= 60)
	    attr = TERM_YELLOW;

	/* Healthy */
	if (pct >= 100)
	    attr = TERM_L_GREEN;

	/* Afraid */
	if (m_ptr->monfear)
	    attr = TERM_VIOLET;

	/* Asleep */
	if (m_ptr->csleep)
	    attr = TERM_BLUE;

	/* Black Breath */
	if (m_ptr->black_breath)
	    attr = TERM_L_DARK;

	/* Stasis */
	if (m_ptr->stasis)
	    attr = TERM_GREEN;


	/* Convert percent into "health" */
	len = (pct < 10) ? 1 : (pct < 90) ? (pct / 10 + 1) : 10;

	/* Default to "unknown" */
	Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");

	/* Dump the current "health" (handle monster stunning, confusion) */
	if (m_ptr->confused)
	    Term_putstr(COL_INFO + 1, ROW_INFO, len, attr, "cccccccccc");
	else if (m_ptr->stunned)
	    Term_putstr(COL_INFO + 1, ROW_INFO, len, attr, "ssssssssss");
	else
	    Term_putstr(COL_INFO + 1, ROW_INFO, len, attr, "**********");
    }



}


/**
 * Redraw the "monster mana bar"
 *
 * The "monster mana bar" provides visual feedback on the "mana"
 * of the monster currently being "tracked".  It follows the lead of the 
 * monster health bar for who to track.
 */
static void mana_redraw(void)
{

    /* Not tracking, or hiding a mimic */
    if (!p_ptr->health_who) {

	/* Erase the health bar */
	Term_erase(COL_MON_MANA, ROW_MON_MANA, 12);
    }

    /* Tracking an unseen monster */
    else if (!m_list[p_ptr->health_who].ml) {

	/* Indicate that the monster health is "unknown" */
	Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
    }

    /* Tracking a hallucinatory monster */
    else if (p_ptr->timed[TMD_HALLUC]) {
	/* Indicate that the monster health is "unknown" */
	Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
    }

    /* Tracking a dead monster (?) */
    else if (!m_list[p_ptr->health_who].hp < 0) {

	/* Indicate that the monster health is "unknown" */
	Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
    }

    /* Tracking a visible monster */
    else {
	int pct, len;

	monster_type *m_ptr = &m_list[p_ptr->health_who];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Default to out of mana */
	byte attr = TERM_RED;

	/* no mana, stop here */
	if (!r_ptr->mana) {
	    /* Erase the health bar */
	    Term_erase(COL_MON_MANA, ROW_MON_MANA, 12);

	    return;
	}

	/* Extract the "percent" of health */
	pct = 100L * m_ptr->mana / r_ptr->mana;

	/* almost no mana */
	if (pct >= 10)
	    attr = TERM_L_RED;

	/* some mana */
	if (pct >= 25)
	    attr = TERM_ORANGE;

	/* most mana */
	if (pct >= 60)
	    attr = TERM_YELLOW;

	/* full mana */
	if (pct >= 100)
	    attr = TERM_L_GREEN;

	/* Convert percent into "health" */
	len = (pct < 10) ? 1 : (pct < 90) ? (pct / 10 + 1) : 10;

	/* Default to "unknown" */
	Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");

	/* Dump the current "mana" */
	Term_putstr(COL_MON_MANA + 1, ROW_MON_MANA, len, attr, "**********");

    }

}


/**
 * Constants for extra status messages
 */
enum {
    STATUS_BLESSED,
    STATUS_HERO,
    STATUS_SHERO,
    STATUS_SUPERSHOT,
    STATUS_OPPOSE_ACID,
    STATUS_OPPOSE_COLD,
    STATUS_OPPOSE_ELEC,
    STATUS_OPPOSE_FIRE,
    STATUS_OPPOSE_POIS,
    STATUS_PROTEVIL,
    STATUS_SHIELD,
    STATUS_FAST,
    STATUS_SLOW,
    STATUS_TIM_INFRA,
    STATUS_SEE_INVIS,
    STATUS_ESP,
    STATUS_IMAGE,
    STATUS_RECALL,
    STATUS_ATT_CONF,
    STATUS_ELE_ATTACK,
    STATUS_HOLY_OR_BREATH,
    STATUS_HIT_AND_RUN,
    STATUS_MAGICDEF,
    STATUS_STEALTH,
    STATUS_MAX
};

/**
 * One of these exists for every extra status message.
 *
 * Col and row tell us where to draw.
 * Attr is the TERM_XXX color.
 * Width is the maximum field width.
 */
typedef struct {
    int col, row;
    byte attr;
    int width, sm_width;
} status_type;

/**
 * Table of extra status message info.
 *
 * Order must match that of the STATUS_XXX constants.
 * Notice that col and row are initialized in init_status();
 * The attr field may be overridden in prt_status().
 */
status_type status_info[] = {
    {0, 0, TERM_L_WHITE, 5, 2},	/* Bless */
    {0, 0, TERM_L_WHITE, 4, 2},	/* Hero */
    {0, 0, TERM_L_WHITE, 5, 2},	/* Bersk */
    {0, 0, TERM_WHITE, 6, 3},	/* SpShot */
    {0, 0, TERM_SLATE, 6, 3},	/* RsAcid */
    {0, 0, TERM_WHITE, 6, 3},	/* RsCold */
    {0, 0, TERM_BLUE, 6, 3},	/* RsElec */
    {0, 0, TERM_RED, 6, 3},	/* RsFire */
    {0, 0, TERM_GREEN, 6, 3},	/* RsPois */
    {0, 0, TERM_L_BLUE, 7, 4},	/* PrtEvil */
    {0, 0, TERM_L_BLUE, 6, 2},	/* Shield */
    {0, 0, TERM_L_GREEN, 5, 2},	/* Haste */
    {0, 0, TERM_L_UMBER, 6, 3},	/* Slower */
    {0, 0, TERM_L_BLUE, 5, 3},	/* Infra */
    {0, 0, TERM_L_BLUE, 6, 2},	/* SInvis */
    {0, 0, TERM_L_GREEN, 3, 3},	/* ESP */
    {0, 0, TERM_YELLOW, 6, 3},	/* Halluc */
    {0, 0, TERM_L_BLUE, 6, 3},	/* Recall */
    {0, 0, TERM_SLATE, 7, 4},	/* AttConf */
    {0, 0, TERM_WHITE, 7, 5},	/* Att1234 */
    {0, 0, TERM_WHITE, 7, 5},	/* AttHoly or AttEvil */
    {0, 0, TERM_BLUE, 6, 4},	/* HitRun */
    {0, 0, TERM_WHITE, 5, 3},	/* MgDef */
    {0, 0, TERM_L_DARK, 4, 1},	/* Hide */
};



/**
 * Initialize the extra status messages.
 */
static void init_status(void)
{
    int i, col, row;

    col = 0;
    row = Term->hgt - 3;

    /* Check each status message */
    for (i = 0; i < STATUS_MAX; i++) {
	/* Access the info */
	status_type *sp = &status_info[i];

	/* Save the column */
	sp->col = col;

	/* Save the row */
	sp->row = row;

	/* Move past this message */
	col += (small_screen ? sp->sm_width : sp->width) + 1;

	/* This is not the last message */
	if (i < STATUS_MAX - 1) {
	    /* There isn't room for the next message on this line */
	    if (((!small_screen) && (col + status_info[i + 1].width >= 80))
		|| ((small_screen)
		    && (col + status_info[i + 1].sm_width >= 48))) {
		/* Wrap */
		col = 0;
		row++;
	    }
	}
    }
}


/**
 * Display all the extra status messages.
 */
static void prt_status(void)
{
    char *s = "                    ";

    int i;


    /* XXX Hack -- Always print messages (for debugging) */
    bool force = FALSE;

    /* XXX Check for room */
    if (!panel_extra_rows)
	return;

    /* Initialize */
    init_status();

    /* Clear the rows */
    Term_erase(0, Term->hgt - 3, 255);
    Term_erase(0, Term->hgt - 2, 255);

    /* Check each status message */
    for (i = 0; i < STATUS_MAX; i++) {
	/* Access the info */
	status_type *sp = &status_info[i];

	/* Get the default attribute */
	byte attr = sp->attr;

	/* Assume empty display */
	char *t = s;

	/* Examine */
	switch (i) {
	case STATUS_BLESSED:
	    if (force || p_ptr->blessed)
		t = "Bless";
	    break;

	case STATUS_HERO:
	    if (force || p_ptr->hero)
		t = "Hero";
	    break;

	case STATUS_SHERO:
	    if (force || p_ptr->shero)
		t = "Bersk";
	    break;

	case STATUS_SUPERSHOT:
	    if (force || p_ptr->special_attack & ATTACK_SUPERSHOT)
		t = "SpShot";
	    break;

	case STATUS_OPPOSE_ACID:
	    if (force || p_ptr->oppose_acid)
		t = "RsAcid";
	    break;

	case STATUS_OPPOSE_COLD:
	    if (force || p_ptr->oppose_cold)
		t = "RsCold";
	    break;

	case STATUS_OPPOSE_ELEC:
	    if (force || p_ptr->oppose_elec)
		t = "RsElec";
	    break;

	case STATUS_OPPOSE_FIRE:
	    if (force || p_ptr->oppose_fire)
		t = "RsFire";
	    break;

	case STATUS_OPPOSE_POIS:
	    if (force || p_ptr->oppose_pois)
		t = "RsPois";
	    break;

	case STATUS_PROTEVIL:
	    if (force || p_ptr->protevil)
		t = "PrtEvil";
	    break;

	case STATUS_SHIELD:
	    if (force || p_ptr->shield)
		t = "Shield";
	    break;

	case STATUS_FAST:
	    if (force || p_ptr->timed[TMD_FAST])
		t = "Haste";
	    break;

	case STATUS_SLOW:
	    if (force || p_ptr->timed[TMD_SLOW])
		t = "Slower";
	    break;

	case STATUS_TIM_INFRA:
	    if (force || p_ptr->tim_infra)
		t = "Infra";
	    else
		t = "     ";
	    break;

	case STATUS_SEE_INVIS:
	    if (force || p_ptr->tim_invis)
		t = "SInvis";
	    break;

	case STATUS_ESP:
	    if (force || p_ptr->tim_esp)
		t = "ESP";
	    break;

	case STATUS_RECALL:
	    if (force || p_ptr->word_recall)
		t = "Recall";
	    break;

	case STATUS_IMAGE:
	    if (force || p_ptr->timed[TMD_HALLUC])
		t = "Halluc";
	    break;

	case STATUS_ATT_CONF:
	    if (force || p_ptr->special_attack & ATTACK_CONFUSE)
		t = "AttConf";
	    break;

	case STATUS_ELE_ATTACK:
	    if (force || p_ptr->ele_attack) {
		if (force || p_ptr->special_attack & ATTACK_ACID) {
		    attr = TERM_L_DARK;
		    t = "AttAcid";
		} else if (p_ptr->special_attack & ATTACK_ELEC) {
		    attr = TERM_BLUE;
		    t = "AttElec";
		} else if (p_ptr->special_attack & ATTACK_FIRE) {
		    attr = TERM_RED;
		    t = "AttFire";
		} else if (p_ptr->special_attack & ATTACK_COLD) {
		    attr = TERM_WHITE;
		    t = "AttCold";
		} else if (p_ptr->special_attack & ATTACK_POIS) {
		    attr = TERM_GREEN;
		    t = "AttPois";
		}
	    }
	    break;

	case STATUS_HOLY_OR_BREATH:
	    if (force || (p_ptr->special_attack & ATTACK_HOLY)) {
		t = "AttHoly";
	    } else if (p_ptr->special_attack & ATTACK_BLKBRTH) {
		attr = TERM_L_DARK;
		t = "AttEvil";
	    }
	    break;

	case STATUS_HIT_AND_RUN:
	    if (force || (p_ptr->special_attack & ATTACK_FLEE))
		t = "HitRun";
	    break;

	case STATUS_MAGICDEF:
	    if (force || p_ptr->magicdef)
		t = "MgDef";
	    break;

	case STATUS_STEALTH:
	    if (force || p_ptr->superstealth)
		t = "Hidden";
	    break;
	}

	/* XXX Hack -- Always show */
	if (force)
	    attr = TERM_L_DARK;

	/* Display */
	if (small_screen)
	    Term_putstr(sp->col, sp->row, sp->sm_width, attr, t);
	else
	    Term_putstr(sp->col, sp->row, sp->width, attr, t);
    }
}


/**
 * Display basic info (mostly left of map)
 */
static void prt_frame_basic(void)
{
    int i;

    /* Race and Class */
    prt_field(p_name + rp_ptr->name, ROW_RACE, COL_RACE);
    prt_field(c_name + cp_ptr->name, ROW_CLASS, COL_CLASS);

    /* Title */
    prt_title();

    /* Level/Experience */
    prt_level();
    prt_exp();

    /* All Stats */
    for (i = 0; i < A_MAX; i++)
	prt_stat(i);

    /* Armor */
    prt_ac();

    /* Hitpoints */
    prt_hp();

    /* Spellpoints */
    prt_sp();

    /* Gold */
    prt_gold();

    /* Shape, if not normal. */
    prt_shape();

    /* Current depth */
    prt_depth();

    /* Special */
    health_redraw();

    /* redraw monster mana */
    mana_redraw();
}


/**
 * Display extra info (mostly below map)
 */
static void prt_frame_extra(void)
{
    /* Cut/Stun */
    prt_cut();
    prt_stun();

    /* Replaces a raft of others */
    update_statusline();

    /* Blank spaces in bigscreen mode */
    prt_blank();

    /* Status */
    prt_status();
}


/**
 * Hack -- display inventory in sub-windows
 */
static void fix_inven(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_INVEN)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display inventory */
	display_inven();

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}

/**
 * Hack -- display monsters in sub-windows
 */
static void fix_monlist(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < 8; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_MONLIST)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display visible monsters */
	display_monlist();

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}

/**
 * Hack -- display monsters in sub-windows
 */
static void fix_itemlist(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < 8; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_ITEMLIST)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display visible monsters */
	display_itemlist();

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}



/**
 * Hack -- display equipment in sub-windows
 */
static void fix_equip(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_EQUIP)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display equipment */
	display_equip();

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}


/**
 * Hack -- display player in sub-windows (mode 0)
 */
static void fix_player_0(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_PLAYER_0)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display player */
	display_player(0);

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}



/**
 * Hack -- display player in sub-windows (mode 1)
 */
static void fix_player_1(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_PLAYER_1)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display flags */
	display_player(1);

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}


/**
 * Hack -- display recent messages in sub-windows
 *
 * Adjust for width and split messages.   XXX XXX XXX
 */
static void fix_message(void)
{
    int j, i;
    int w, h;
    int x, y;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_MESSAGE)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Get size */
	Term_get_size(&w, &h);

	/* Dump messages */
	for (i = 0; i < h; i++) {
	    byte color = message_color((s16b) i);

	    /* Dump the message on the appropriate line */
	    Term_putstr(0, (h - 1) - i, -1, color, message_str((s16b) i));

	    /* Cursor */
	    Term_locate(&x, &y);

	    /* Clear to end of line */
	    Term_erase(x, y, 255);
	}

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}


/**
 * Hack -- display overhead view in sub-windows
 *
 * Note that the "player" symbol does NOT appear on the map.
 */
static void fix_overhead(void)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    int j;

    int cy, cx;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_OVERHEAD)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Hack -- Hide player XXX XXX XXX */
	cave_m_idx[py][px] = 0;

	/* Redraw map */
	display_map(&cy, &cx, TRUE);

	/* Hack -- Show player XXX XXX XXX */
	cave_m_idx[py][px] = -1;

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}


/**
 * Hack -- display monster recall in sub-windows
 */
static void fix_monster(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_MONSTER)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display monster race info */
	if (p_ptr->monster_race_idx)
	    display_roff(p_ptr->monster_race_idx);

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}


/**
 * Hack -- display object recall in sub-windows
 */
static void fix_object(void)
{
    int j;

    /* Scan windows */
    for (j = 0; j < TERM_WIN_MAX; j++) {
	term *old = Term;

	/* No window */
	if (!angband_term[j])
	    continue;

	/* No relevant flags */
	if (!(op_ptr->window_flag[j] & (PW_OBJECT)))
	    continue;

	/* Activate */
	Term_activate(angband_term[j]);

	/* Display monster race info */
	if (p_ptr->object_kind_idx)
	    display_koff(p_ptr->object_kind_idx);

	/* Fresh */
	Term_fresh();

	/* Restore */
	Term_activate(old);
    }
}


