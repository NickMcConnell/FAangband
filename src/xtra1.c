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


/**
 * Converts stat num into a six-char (right justified) string
 */
void cnv_stat(int val, char *out_val)
{
  /* Above 18 */
  if (val > 18)
    {
      int bonus = (val - 18);
      
      if (bonus >= 220)
	{
	  sprintf(out_val, "18/%3s", "***");
	}
      else if (bonus >= 100)
	{
	  sprintf(out_val, "18/%03d", bonus);
	}
      else
	{
	  sprintf(out_val, " 18/%02d", bonus);
	}
    }
  
  /* From 3 to 18 */
  else
    {
      sprintf(out_val, "    %2d", val);
    }
}



/**
 * Modify a stat value by a "modifier", return new value
 *
 * Stats go up: 3,4,...,17,18,18/10,18/20,...,18/220
 * Or even: 18/13, 18/23, 18/33, ..., 18/220
 *
 * Stats go down: 18/220, 18/210,..., 18/10, 18, 17, ..., 3
 * Or even: 18/13, 18/03, 18, 17, ..., 3
 */
s16b modify_stat_value(int value, int amount)
{
  int i;
  
  /* Reward */
  if (amount > 0)
    {
      /* Apply each point */
      for (i = 0; i < amount; i++)
	{
	  /* One point at a time */
	  if (value < 18) value++;
	  
	  /* Ten "points" at a time */
	  else value += 10;
	}
    }
  
  /* Penalty */
  else if (amount < 0)
    {
      /* Apply each point */
      for (i = 0; i < (0 - amount); i++)
	{
	  /* Ten points at a time */
	  if (value >= 18+10) value -= 10;
	  
	  /* Hack -- prevent weirdness */
	  else if (value > 18) value = 18;
	  
	  /* One point at a time */
	  else if (value > 3) value--;
		}
    }
  
  /* Return new value */
  return (value);
}



/**
 * Print character info at given row, column in a 13 char field
 */
static void prt_field(cptr info, int row, int col)
{
  /* Dump 12 spaces to clear */
  if (SIDEBAR_WID == 12) c_put_str(TERM_WHITE, "            ", row, col);

  /* Dump 13 spaces to clear */
  else c_put_str(TERM_WHITE, "             ", row, col);
  
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
  if (p_ptr->stat_cur[stat] < p_ptr->stat_max[stat])
    {
      put_str(stat_names_reduced[stat], ROW_STAT + stat, COL_STAT);
      cnv_stat(p_ptr->stat_use[stat], tmp);
      c_put_str(TERM_YELLOW, tmp, ROW_STAT + stat, COL_STAT + 6);
    }
  
  /* Display "healthy" stat */
  else
    {
      put_str(stat_names[stat], ROW_STAT + stat, COL_STAT);
      cnv_stat(p_ptr->stat_use[stat], tmp);
      c_put_str(TERM_L_GREEN, tmp, ROW_STAT + stat, COL_STAT + 6);
    }
  
  /* Indicate natural maximum */
  if (p_ptr->stat_max[stat] == 18+100)
    {
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
  if (p_ptr->wizard)
    {
      p = "[=-WIZARD-=]";
    }
  
  /* Winner */
  else if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL))
    {
      p = "***WINNER***";
    }
  
  /* Normal */
  else
    {
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
  
  if (p_ptr->lev >= p_ptr->max_lev)
    {
      put_str("LEVEL ", ROW_LEVEL, 0);
      c_put_str(TERM_L_GREEN, tmp, ROW_LEVEL, COL_LEVEL + 6);
    }
  else
    {
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
  
  if (p_ptr->lev < PY_MAX_LEVEL)
    {
      long val = (long)((player_exp[p_ptr->lev - 1]) - p_ptr->exp);
      
      /* Boundary check */
      if (val < 0) val = 0;
      
      sprintf(out_val, "%7ld", val);
      
      if (p_ptr->exp >= p_ptr->max_exp)
	{
	  put_str("NEXT ", ROW_EXP, 0);
	  c_put_str(TERM_L_GREEN, out_val, ROW_EXP, COL_EXP + 5);
	}
      else
	{
	  put_str("Next ", ROW_EXP, 0);
	  c_put_str(TERM_YELLOW, out_val, ROW_EXP, COL_EXP + 5);
	}
    }
  else
    {
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
  sprintf(tmp, "%9ld", (long)p_ptr->au);
  c_put_str(TERM_L_GREEN, tmp, ROW_GOLD, COL_GOLD + 3);
}

/**
 * Prints current shape, if not normal.   -LM-
 */
static void prt_shape(void)
{
  char *shapedesc = "";
  
  switch (p_ptr->schange)
    {
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
  else c_put_str(TERM_RED, shapedesc, ROW_SHAPE, COL_SHAPE);

}


/**
 * Prints current AC
 */
static void prt_ac(void)
{
  char tmp[32];
  
  put_str("Cur AC ", ROW_AC, COL_AC);
  sprintf(tmp, "%5d", p_ptr->dis_ac + p_ptr->dis_to_a);
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
  if (p_ptr->chp >= p_ptr->mhp) return;
  
  if (p_ptr->chp > (p_ptr->mhp * op_ptr->hitpoint_warn) / 10)
    {
      color = TERM_YELLOW;
    }
  else
    {
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
  if (!mp_ptr->spell_book) return;
  
  
  put_str("SP          ", ROW_SP, COL_SP);
  
  len = sprintf(tmp, "%d:%d", p_ptr->csp, p_ptr->msp);
  
  c_put_str(TERM_L_GREEN, tmp, ROW_SP, COL_SP + 12 - len);
  
  /* Done? */
  if (p_ptr->csp >= p_ptr->msp) return;
  
  if (p_ptr->csp > (p_ptr->msp * op_ptr->hitpoint_warn) / 10)
    {
      color = TERM_YELLOW;
    }
  else
    {
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
  
  if (small_screen)
    {
      if (level)
	sprintf(loc, "%s%3d", short_locality_name[region], level);
      else
	sprintf(loc, "%s", short_locality_name[region]);
    }
  else
    {
      if (level)
	sprintf(loc, "%s %d", locality_name[region], level);
      else
	sprintf(loc, "%s", locality_name[region]);
    }
  
  /* Get color of level based on feeling  -JSV- */
  if ((p_ptr->depth) && (do_feeling))
    {
      if (p_ptr->themed_level) attr = TERM_BLUE;
      else if (feeling ==  1) attr = TERM_VIOLET;
      else if (feeling ==  2) attr = TERM_RED;
      else if (feeling ==  3) attr = TERM_L_RED;
      else if (feeling ==  4) attr = TERM_ORANGE;
      else if (feeling ==  5) attr = TERM_ORANGE;
      else if (feeling ==  6) attr = TERM_YELLOW;
      else if (feeling ==  7) attr = TERM_YELLOW;
      else if (feeling ==  8) attr = TERM_WHITE;
      else if (feeling ==  9) attr = TERM_WHITE;
      else if (feeling == 10) attr = TERM_L_WHITE;
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
struct state_info
{
  int value;
  const char *str;
  size_t len;
  byte attr;
};

/** p_ptr->hunger descriptions */
static const struct state_info hunger_data[] =
{
  { PY_FOOD_FAINT, S("Faint"),    TERM_RED },
  { PY_FOOD_WEAK,  S("Weak"),     TERM_ORANGE },
  { PY_FOOD_ALERT, S("Hungry"),   TERM_YELLOW },
  { PY_FOOD_FULL,  S(""),         TERM_L_GREEN },
  { PY_FOOD_MAX,   S("Full"),     TERM_L_GREEN },
  { PY_FOOD_UPPER, S("Gorged"),   TERM_GREEN },
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
  if (p_ptr->resting)
    {
      int i;
      int n = p_ptr->resting;
      
      /* Start with "Rest" */
      my_strcpy(text, "Rest      ", sizeof(text));
      
      /* Extensive (timed) rest */
      if (n >= 1000)
	{
	  i = n / 100;
	  text[9] = '0';
	  text[8] = '0';
	  text[7] = I2D(i % 10);
	  if (i >= 10)
	    {
	      i = i / 10;
	      text[6] = I2D(i % 10);
	      if (i >= 10)
		{
		  text[5] = I2D(i / 10);
		}
	    }
	}
      
      /* Long (timed) rest */
      else if (n >= 100)
	{
	  i = n;
	  text[9] = I2D(i % 10);
	  i = i / 10;
	  text[8] = I2D(i % 10);
	  text[7] = I2D(i / 10);
	}
      
      /* Medium (timed) rest */
      else if (n >= 10)
	{
	  i = n;
	  text[9] = I2D(i % 10);
	  text[8] = I2D(i / 10);
	}
      
      /* Short (timed) rest */
      else if (n > 0)
	{
	  i = n;
	  text[9] = I2D(i);
	}
      
      /* Rest until healed */
      else if (n == -1)
	{
	  text[5] = text[6] = text[7] = text[8] = text[9] = '*';
	}

      /* Rest until done */
      else if (n == -2)
	{
	  text[5] = text[6] = text[7] = text[8] = text[9] = '&';
	}
    }
  
  /* Repeating */
  else if (p_ptr->command_rep)
    {
      if (p_ptr->command_rep > 999)
	strnfmt(text, sizeof(text), "Rep. %3d00", p_ptr->command_rep / 100);
      else
	strnfmt(text, sizeof(text), "Repeat %3d", p_ptr->command_rep);
    }
  
  /* Searching */
  else if (p_ptr->searching)
    {
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
  if (p_ptr->searching) i += 10;
  
  /* Fast */
  if (i > 110)
    {
      attr = TERM_L_GREEN;
      sprintf(buf, "Fast (+%d)", (i - 110));
    }
  
  /* Slow */
  else if (i < 110)
    {
      attr = TERM_L_UMBER;
      sprintf(buf, "Slow (-%d)", (110 - i));
    }
  
  /* Display the speed */
  c_put_str((byte)attr, format("%-9s", buf), row, col);

  return strlen(buf);
}

/**
 * Prints trap detection status
 */
static size_t prt_dtrap(int row, int col)
{
  byte info = cave_info2[p_ptr->py][p_ptr->px];
  
  /* The player is in a trap-detected grid */
  if (info & (CAVE2_DTRAP))
    {
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
  if (p_ptr->new_spells)
    {
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
  if (p_ptr->new_specialties)
    {
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
  if (p_ptr->blind)
    {
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
  if (p_ptr->confused)
    {
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
  if (p_ptr->afraid)
    {
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
  if (p_ptr->paralyzed)
    {
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
  if (p_ptr->poisoned)
    {
      c_put_str(TERM_ORANGE, "Poisoned", row, col);
      return 8;
    }
  return 0;
}


/** Useful typedef */
typedef size_t status_f(int row, int col);

status_f *status_handlers[] =
{ prt_state, prt_hunger, prt_study, prt_spec, prt_blind, prt_confused, 
  prt_afraid, prt_paralyzed, prt_poisoned, prt_dtrap };


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

  if (normal_screen)
    {
      /* Clear the remainder of the line */
      prt("", row, col);
      
      /* Display those which need redrawing */
      for (i = 0; i < N_ELEMENTS(status_handlers); i++)
	col += status_handlers[i](row, col);
      
      /* Print speed, record where the status info ends */
      status_end = col + prt_speed(row, col);
      
      /* Redo the location info */
      prt_depth();
    }

  /* Print the mouse buttons */
  if (mouse_buttons)
    for (j = 0; j < num_buttons; j++)
      {
	sprintf(buf,"[%s]", mse_button[j].label);
	c_put_str(TERM_SLATE, buf, row, 
		  button_end - mse_button[j].left);
      }

  /* Reposition the cursor */
  if (!bad) (void)Term_gotoxy(x, y);
}


static void prt_cut(void)
{
  int c = p_ptr->cut;
  
  if (c > 1000)
    {
      c_put_str(TERM_L_RED, (bottom_status ? "Mtl wnd" : "Mortal wound"), 
			     ROW_CUT, COL_CUT);
    }
  else if (c > 200)
    {
      c_put_str(TERM_RED, (bottom_status ? "Dp gash" : "Deep gash   "), 
		ROW_CUT, COL_CUT);
    }
  else if (c > 100)
    {
      c_put_str(TERM_RED, (bottom_status ? "Svr cut" : "Severe cut  "), 
		ROW_CUT, COL_CUT);
    }
  else if (c > 50)
    {
      c_put_str(TERM_ORANGE, (bottom_status ? "Nst cut" : "Nasty cut   "), 
		ROW_CUT, COL_CUT);
    }
  else if (c > 25)
    {
      c_put_str(TERM_ORANGE, (bottom_status ? "Bad cut" : "Bad cut     "), 
		ROW_CUT, COL_CUT);
    }
  else if (c > 10)
    {
      c_put_str(TERM_YELLOW, (bottom_status ? "Lgt cut" : "Light cut   "), 
		ROW_CUT, COL_CUT);
    }
  else if (c)
    {
      c_put_str(TERM_YELLOW, (bottom_status ? "Graze  " : "Graze       "), 
		ROW_CUT, COL_CUT);
    }
  else
    {
      put_str((bottom_status ? "       " : "            "), ROW_CUT, COL_CUT);
    }
}



static void prt_stun(void)
{
  int s = p_ptr->stun;
  
  if (s > 100)
    {
      c_put_str(TERM_RED, (bottom_status ? "Knc out" : "Knocked out "), 
		ROW_STUN, COL_STUN);
    }
  else if (s > 50)
    {
      c_put_str(TERM_ORANGE, (bottom_status ? "Hvy stn" : "Heavy stun  "), 
		ROW_STUN, COL_STUN);
    }
  else if (s)
    {
      c_put_str(TERM_ORANGE, (bottom_status ? "Stun   " : "Stun        "), 
		ROW_STUN, COL_STUN);
    }
  else
    {
      put_str((bottom_status ? "       " : "            "), ROW_STUN, COL_STUN);
    }
}



static void prt_blank(void)
{
  int i,j;
  
  j = (panel_extra_rows ? 2 : 0);
  
  if ((Term->hgt > (j + 24)) && (!bottom_status))
    {
      for (i = 23; i < (Term->hgt - 1 - j); i++)
	{
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
  if (!p_ptr->health_who)
    {
      /* Erase the health bar */
      Term_erase(COL_INFO, ROW_INFO, 12);
    }
  
  /* Tracking an unseen monster */
  else if (!m_list[p_ptr->health_who].ml)
    {
      /* Indicate that the monster health is "unknown" */
      Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");
    }
  
  /* Tracking a hallucinatory monster */
  else if (p_ptr->image)
    {
      /* Indicate that the monster health is "unknown" */
      Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");
    }
  
  /* Tracking a dead monster (???) */
  else if (!m_list[p_ptr->health_who].hp < 0)
    {
      /* Indicate that the monster health is "unknown" */
      Term_putstr(COL_INFO, ROW_INFO, 12, TERM_WHITE, "[----------]");
    }

  /* Tracking a visible monster */
  else
    {
      int pct, len;
      
      monster_type *m_ptr = &m_list[p_ptr->health_who];
      
      /* Default to almost dead */
      byte attr = TERM_RED;
      
      /* Extract the "percent" of health */
      pct = 100L * m_ptr->hp / m_ptr->maxhp;
      
      /* Badly wounded */
      if (pct >= 10) attr = TERM_L_RED;
      
      /* Wounded */
      if (pct >= 25) attr = TERM_ORANGE;
      
      /* Somewhat Wounded */
      if (pct >= 60) attr = TERM_YELLOW;
      
      /* Healthy */
      if (pct >= 100) attr = TERM_L_GREEN;
      
      /* Afraid */
      if (m_ptr->monfear) attr = TERM_VIOLET;
      
      /* Asleep */
      if (m_ptr->csleep) attr = TERM_BLUE;
      
      /* Black Breath */
      if (m_ptr->black_breath) attr = TERM_L_DARK;
      
      /* Stasis */
      if (m_ptr->stasis) attr = TERM_GREEN;
      
      
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
  if (!p_ptr->health_who)
    {
      
      /* Erase the health bar */
      Term_erase(COL_MON_MANA, ROW_MON_MANA, 12);
    }
  
  /* Tracking an unseen monster */
  else if (!m_list[p_ptr->health_who].ml)
    {
      
      /* Indicate that the monster health is "unknown" */
      Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
    }
  
  /* Tracking a hallucinatory monster */
  else if (p_ptr->image)
    {
      /* Indicate that the monster health is "unknown" */
      Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
    }

  /* Tracking a dead monster (?) */
  else if (!m_list[p_ptr->health_who].hp < 0)
    {
      
      /* Indicate that the monster health is "unknown" */
      Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
    }
  
  /* Tracking a visible monster */
  else
    {
      int pct, len;
      
      monster_type *m_ptr = &m_list[p_ptr->health_who];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Default to out of mana*/
      byte attr = TERM_RED;
      
      /*no mana, stop here*/
      if (!r_ptr->mana)
	{
	  /* Erase the health bar */
	  Term_erase(COL_MON_MANA, ROW_MON_MANA, 12);
	  
	  return;
	}
      
      /* Extract the "percent" of health */
      pct = 100L * m_ptr->mana / r_ptr->mana;
      
      /* almost no mana */
      if (pct >= 10) attr = TERM_L_RED;
      
      /* some mana */
      if (pct >= 25) attr = TERM_ORANGE;
      
      /* most mana */
      if (pct >= 60) attr = TERM_YELLOW;
      
      /* full mana */
      if (pct >= 100) attr = TERM_L_GREEN;
      
      /* Convert percent into "health" */
      len = (pct < 10) ? 1 : (pct < 90) ? (pct / 10 + 1) : 10;
      
      /* Default to "unknown" */
      Term_putstr(COL_MON_MANA, ROW_MON_MANA, 12, TERM_WHITE, "[----------]");
      
      /* Dump the current "mana"*/
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
  {0, 0, TERM_L_WHITE, 5, 2}, /* Bless */
  {0, 0, TERM_L_WHITE, 4, 2}, /* Hero */
  {0, 0, TERM_L_WHITE, 5, 2}, /* Bersk */
  {0, 0, TERM_WHITE, 6, 3},   /* SpShot */
  {0, 0, TERM_SLATE, 6, 3},   /* RsAcid */
  {0, 0, TERM_WHITE, 6, 3},   /* RsCold */
  {0, 0, TERM_BLUE, 6, 3},    /* RsElec */
  {0, 0, TERM_RED, 6, 3},     /* RsFire */
  {0, 0, TERM_GREEN, 6, 3},   /* RsPois */
  {0, 0, TERM_L_BLUE, 7, 4},  /* PrtEvil */
  {0, 0, TERM_L_BLUE, 6, 2},  /* Shield */
  {0, 0, TERM_L_GREEN, 5, 2}, /* Haste */
  {0, 0, TERM_L_UMBER, 6, 3}, /* Slower */
  {0, 0, TERM_L_BLUE, 5, 3},  /* Infra */
  {0, 0, TERM_L_BLUE, 6, 2},  /* SInvis */
  {0, 0, TERM_L_GREEN, 3, 3}, /* ESP */
  {0, 0, TERM_YELLOW, 6, 3},  /* Halluc */
  {0, 0, TERM_L_BLUE, 6, 3},  /* Recall */
  {0, 0, TERM_SLATE, 7, 4},   /* AttConf */
  {0, 0, TERM_WHITE, 7, 5},   /* Att1234 */
  {0, 0, TERM_WHITE, 7, 5},   /* AttHoly or AttEvil */
  {0, 0, TERM_BLUE, 6, 4},    /* HitRun */
  {0, 0, TERM_WHITE, 5, 3},   /* MgDef */
  {0, 0, TERM_L_DARK, 4, 1},  /* Hide */
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
  for (i = 0; i < STATUS_MAX; i++)
    {
      /* Access the info */
      status_type *sp = &status_info[i];
      
      /* Save the column */
      sp->col = col;
      
      /* Save the row */
      sp->row = row;
      
      /* Move past this message */
      col += (small_screen ? sp->sm_width : sp->width) + 1;
      
      /* This is not the last message */
      if (i < STATUS_MAX - 1)
	{
	  /* There isn't room for the next message on this line */
	  if (((!small_screen) && (col + status_info[i + 1].width >= 80)) ||
	      ((small_screen) && (col + status_info[i + 1].sm_width >= 48)))
	    {
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
  if (!panel_extra_rows) return;
  
  /* Initialize */
  init_status();
  
  /* Clear the rows */
  Term_erase(0, Term->hgt - 3, 255);
  Term_erase(0, Term->hgt - 2, 255);
  
  /* Check each status message */
  for (i = 0; i < STATUS_MAX; i++)
    {
      /* Access the info */
      status_type *sp = &status_info[i];
      
      /* Get the default attribute */
      byte attr = sp->attr;
      
      /* Assume empty display */
      char *t = s;
      
      /* Examine */
      switch (i)
	{
	case STATUS_BLESSED:
	  if (force || p_ptr->blessed) t = "Bless";
	  break;
	  
	case STATUS_HERO:
	  if (force || p_ptr->hero) t = "Hero";
	  break;
	  
	case STATUS_SHERO:
	  if (force || p_ptr->shero) t = "Bersk";
	  break;
	  
	case STATUS_SUPERSHOT:
	  if (force || p_ptr->special_attack & ATTACK_SUPERSHOT) t = "SpShot";
	  break;
	  
	case STATUS_OPPOSE_ACID:
	  if (force || p_ptr->oppose_acid) t = "RsAcid";
	  break;
	  
	case STATUS_OPPOSE_COLD:
	  if (force || p_ptr->oppose_cold) t = "RsCold";
	  break;
	  
	case STATUS_OPPOSE_ELEC:
	  if (force || p_ptr->oppose_elec) t = "RsElec";
	  break;
	  
	case STATUS_OPPOSE_FIRE:
	  if (force || p_ptr->oppose_fire) t = "RsFire";
	  break;
	  
	case STATUS_OPPOSE_POIS:
	  if (force || p_ptr->oppose_pois) t = "RsPois";
	  break;
	  
	case STATUS_PROTEVIL:
	  if (force || p_ptr->protevil) t = "PrtEvil";
	  break;
	  
	case STATUS_SHIELD:
	  if (force || p_ptr->shield) t = "Shield";
	  break;
	  
	case STATUS_FAST:
	  if (force || p_ptr->fast) t = "Haste";
	  break;
	  
	case STATUS_SLOW:
	  if (force || p_ptr->slow) t = "Slower";
	  break;
	  
	case STATUS_TIM_INFRA:
	  if (force || p_ptr->tim_infra) t = "Infra";
	  else t = "     ";
	  break;
	  
	case STATUS_SEE_INVIS:
	  if (force || p_ptr->tim_invis) t = "SInvis";
	  break;
	  
	case STATUS_ESP:
	  if (force || p_ptr->tim_esp) t = "ESP";
	  break;
	  
	case STATUS_RECALL:
	  if (force || p_ptr->word_recall) t = "Recall";
	  break;
	  
	case STATUS_IMAGE:
	  if (force || p_ptr->image) t = "Halluc";
	  break;
	  
	case STATUS_ATT_CONF:
	  if (force || p_ptr->special_attack & ATTACK_CONFUSE) t = "AttConf";
	  break;
	  
	case STATUS_ELE_ATTACK:
	  if (force || p_ptr->ele_attack)
	    {
	      if (force || p_ptr->special_attack & ATTACK_ACID)
		{
		  attr = TERM_L_DARK;
		  t = "AttAcid";
		}
	      else if (p_ptr->special_attack & ATTACK_ELEC)
		{
		  attr = TERM_BLUE;
		  t = "AttElec";
		}
	      else if (p_ptr->special_attack & ATTACK_FIRE)
		{
		  attr = TERM_RED;
		  t = "AttFire";
		}
	      else if (p_ptr->special_attack & ATTACK_COLD)
		{
		  attr = TERM_WHITE;
		  t = "AttCold";
		}
	      else if (p_ptr->special_attack & ATTACK_POIS)
		{
		  attr = TERM_GREEN;
		  t = "AttPois";
		}
	    }
	  break;
	  
	case STATUS_HOLY_OR_BREATH:
	  if (force || (p_ptr->special_attack & ATTACK_HOLY))
	    {
	      t = "AttHoly";
	    }
	  else if (p_ptr->special_attack & ATTACK_BLKBRTH)
	    {
	      attr = TERM_L_DARK;
	      t = "AttEvil";
	    }
	  break;
	  
	case STATUS_HIT_AND_RUN:
	  if (force || (p_ptr->special_attack & ATTACK_FLEE)) t = "HitRun";
	  break;
	  
	case STATUS_MAGICDEF:
	  if (force || p_ptr->magicdef) t = "MgDef";
	  break;
	  
	case STATUS_STEALTH:
	  if (force || p_ptr->superstealth) t = "Hidden";
	  break;
	}
      
      /* XXX Hack -- Always show */
      if (force) attr = TERM_L_DARK;
      
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
  for (i = 0; i < A_MAX; i++) prt_stat(i);
  
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
  
  /* redraw monster mana*/
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_INVEN))) continue;
      
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
  for (j = 0; j < 8; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_MONLIST))) continue;
      
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
  for (j = 0; j < 8; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_ITEMLIST))) continue;
      
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_EQUIP))) continue;
      
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_PLAYER_0))) continue;
      
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_PLAYER_1))) continue;
      
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_MESSAGE))) continue;
      
      /* Activate */
      Term_activate(angband_term[j]);
      
      /* Get size */
      Term_get_size(&w, &h);
      
      /* Dump messages */
      for (i = 0; i < h; i++)
	{
	  byte color = message_color((s16b)i);
	  
	  /* Dump the message on the appropriate line */
	  Term_putstr(0, (h - 1) - i, -1, color, message_str((s16b)i));
	  
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_OVERHEAD))) continue;
      
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_MONSTER))) continue;
      
      /* Activate */
      Term_activate(angband_term[j]);
      
      /* Display monster race info */
      if (p_ptr->monster_race_idx) display_roff(p_ptr->monster_race_idx);
      
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
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      term *old = Term;
      
      /* No window */
      if (!angband_term[j]) continue;
      
      /* No relevant flags */
      if (!(op_ptr->window_flag[j] & (PW_OBJECT))) continue;
      
      /* Activate */
      Term_activate(angband_term[j]);
      
      /* Display monster race info */
      if (p_ptr->object_kind_idx) display_koff(p_ptr->object_kind_idx);
      
      /* Fresh */
      Term_fresh();

      /* Restore */
      Term_activate(old);
    }
}


/**
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 *
 * Note that this function induces various "status" messages,
 * which must be bypassed until the character is created.
 */
static void calc_spells(void)
{
  int i, j, k, levels;
  int num_allowed, num_known;
  
  magic_type *s_ptr;
  
  cptr p = "";
  
  
  /* Hack -- must be literate */
  if (!mp_ptr->spell_book) return;
  
  /* Hack -- wait for creation */
  if (!character_generated) return;
  
  /* Hack -- handle "xtra" mode */
  if (character_xtra) return;
  
  /* Determine magic description. */
  if (mp_ptr->spell_book == TV_MAGIC_BOOK) p = "spell";
  if (mp_ptr->spell_book == TV_PRAYER_BOOK) p = "prayer";
  if (mp_ptr->spell_book == TV_DRUID_BOOK) p = "druidic lore";
  if (mp_ptr->spell_book == TV_NECRO_BOOK) p = "ritual";
  
  /* Determine the number of spells allowed */
  levels = p_ptr->lev - mp_ptr->spell_first + 1;
  
  /* Hack -- no negative spells */
  if (levels < 0) levels = 0;
  
  
  /* Extract total allowed spells */
  num_allowed = (adj_mag_study[p_ptr->stat_ind[mp_ptr->spell_stat]] *
		 levels / 2);
  
  /* Boundary control. */
  if (num_allowed > mp_ptr->spell_number) num_allowed = mp_ptr->spell_number;
  
  
  /* Assume none known */
  num_known = 0;
  
  /* Count the number of spells we know */
  for (j = 0; j < mp_ptr->spell_number; j++)
    {
      /* Count known spells */
      if ((j < 32) ?
	  (p_ptr->spell_learned1 & (1L << j)) :
	  (p_ptr->spell_learned2 & (1L << (j - 32))))
	{
	  num_known++;
	}
    }
  
  /* See how many spells we must forget or may learn */
  p_ptr->new_spells = num_allowed - num_known;
  
  
  
  /* Forget spells which are too hard */
  for (i = 63; i >= 0; i--)
    {
      /* Efficiency -- all done */
      if (!p_ptr->spell_learned1 && !p_ptr->spell_learned2) break;
      
      /* Access the spell */
      j = p_ptr->spell_order[i];
      
      /* Skip non-spells */
      if (j >= 99) continue;
      
      /* Get the spell */
      s_ptr = &mp_ptr->info[j];
      
      /* Skip spells we are allowed to know */
      if (s_ptr->slevel <= p_ptr->lev) continue;
      
      /* Is it known? */
      if ((j < 32) ? (p_ptr->spell_learned1 & (1L << j)) :
	  (p_ptr->spell_learned2 & (1L << (j - 32))))
	{
	  /* Mark as forgotten */
	  if (j < 32)
	    {
	      p_ptr->spell_forgotten1 |= (1L << j);
	    }
	  else
	    {
	      p_ptr->spell_forgotten2 |= (1L << (j - 32));
	    }
	  
	  /* No longer known */
	  if (j < 32)
	    {
	      p_ptr->spell_learned1 &= ~(1L << j);
	    }
	  else
	    {
	      p_ptr->spell_learned2 &= ~(1L << (j - 32));
	    }
	  
	  /* Message */
	  msg_format("You have forgotten the %s of %s.", p,
		     spell_names[s_ptr->index]);
	  
	  /* One more can be learned */
	  p_ptr->new_spells++;
	}
    }
  
  
  /* Forget spells if we know too many spells */
  for (i = 63; i >= 0; i--)
    {
      /* Stop when possible */
      if (p_ptr->new_spells >= 0) break;
      
      /* Efficiency -- all done */
      if (!p_ptr->spell_learned1 && !p_ptr->spell_learned2) break;
      
      /* Get the (i+1)th spell learned */
      j = p_ptr->spell_order[i];
      
      /* Skip unknown spells */
      if (j >= 99) continue;
      
      /* Get the spell */
      s_ptr = &mp_ptr->info[j];
      
      /* Forget it (if learned) */
      if ((j < 32) ?
	  (p_ptr->spell_learned1 & (1L << j)) :
	  (p_ptr->spell_learned2 & (1L << (j - 32))))
	{
	  /* Mark as forgotten */
	  if (j < 32)
	    {
	      p_ptr->spell_forgotten1 |= (1L << j);
	    }
	  else
	    {
	      p_ptr->spell_forgotten2 |= (1L << (j - 32));
	    }
	  
	  /* No longer known */
	  if (j < 32)
	    {
	      p_ptr->spell_learned1 &= ~(1L << j);
	    }
	  else
	    {
	      p_ptr->spell_learned2 &= ~(1L << (j - 32));
	    }
	  
	  /* Message */
	  msg_format("You have forgotten the %s of %s.", p,
		     spell_names[s_ptr->index]);
	  
	  /* One more can be learned */
	  p_ptr->new_spells++;
	}
    }
  
  
  /* Check for spells to remember */
  for (i = 0; i < 64; i++)
    {
      /* None left to remember */
      if (p_ptr->new_spells <= 0) break;
      
      /* Efficiency -- all done */
      if (!p_ptr->spell_forgotten1 && !p_ptr->spell_forgotten2) break;
      
      /* Get the next spell we learned */
      j = p_ptr->spell_order[i];
      
      /* Skip unknown spells */
      if (j >= 99) break;
      
      /* Access the spell */
      s_ptr = &mp_ptr->info[j];
      
      /* Skip spells we cannot remember */
      if (s_ptr->slevel > p_ptr->lev) continue;
      
      /* First set of spells */
      if ((j < 32) ?
	  (p_ptr->spell_forgotten1 & (1L << j)) :
	  (p_ptr->spell_forgotten2 & (1L << (j - 32))))
	{
	  /* No longer forgotten */
	  if (j < 32)
	    {
	      p_ptr->spell_forgotten1 &= ~(1L << j);
	    }
	  else
	    {
	      p_ptr->spell_forgotten2 &= ~(1L << (j - 32));
	    }
	  
	  /* Known once more */
	  if (j < 32)
	    {
	      p_ptr->spell_learned1 |= (1L << j);
	    }
	  else
	    {
	      p_ptr->spell_learned2 |= (1L << (j - 32));
	    }
	  
	  /* Message */
	  msg_format("You have remembered the %s of %s.",
		     p, spell_names[s_ptr->index]);
	  
	  /* One less can be learned */
	  p_ptr->new_spells--;
	}
    }
  
  
  /* Assume no spells available */
  k = 0;
  
  /* Count spells that can be learned */
  for (j = 0; j < mp_ptr->spell_number; j++)
    {
      /* Access the spell */
      s_ptr = &mp_ptr->info[j];
      
      /* Skip spells we cannot remember */
      if (s_ptr->slevel > p_ptr->lev) continue;
      
      /* Skip spells we already know */
      if ((j < 32) ?
	  (p_ptr->spell_learned1 & (1L << j)) :
	  (p_ptr->spell_learned2 & (1L << (j - 32))))
	{
	  continue;
	}
      
      /* Count it */
      k++;
    }
  
  /* Cannot learn more spells than exist */
  if (p_ptr->new_spells > k) p_ptr->new_spells = k;
  
  /* Spell count changed */
  if (p_ptr->old_spells != p_ptr->new_spells)
    {
      /* Message if needed */
      if (p_ptr->new_spells)
	{
	  /* Message */
	  msg_format("You can learn %d more %s%s.", p_ptr->new_spells, p, 
		     ((p_ptr->new_spells != 1) && 
		      (mp_ptr->spell_book != TV_DRUID_BOOK)) ? "s" : "");
	}
      
      /* Save the new_spells value */
      p_ptr->old_spells = p_ptr->new_spells;
      
      /* Redraw Study Status */
      p_ptr->redraw |= (PR_STUDY);
    }
}


/**
 * Calculate number of specialties player should have. -BR-
 */
static void calc_specialty(void)
{
  int i;
  int num_known, questortwo = 2;
  
  /* Calculate number allowed */
  if (p_ptr->quests < 2)   /* -NRM- */
    questortwo = p_ptr->quests;
  p_ptr->specialties_allowed = 1 + questortwo;
  if (check_ability(SP_XTRA_SPECIALTY)) p_ptr->specialties_allowed++;
  if (p_ptr->specialties_allowed > MAX_SPECIALTIES) 
    p_ptr->specialties_allowed = MAX_SPECIALTIES;
  
  /* Assume none known */
  num_known = 0;
  
  /* Count the number of specialties we know */
  for (i = 0; i < MAX_SPECIALTIES; i++)
    {
      if (p_ptr->specialty_order[i] != SP_NO_SPECIALTY) num_known++;
    }
  
  /* See how many spells we must forget or may learn */
  p_ptr->new_specialties = p_ptr->specialties_allowed - num_known;
  
  /* Check if specialty array is full */
  if ((num_known == MAX_SPECIALTIES) && (p_ptr->new_specialties > 0)) 
    p_ptr->new_specialties = 0;
  
  /* More specialties are available (or fewer forgotten) */
  if (p_ptr->old_specialties < p_ptr->new_specialties)
    {
      if (p_ptr->old_specialties < 0) 
	msg_print("You have regained specialist abilities.");
      if (p_ptr->new_specialties > 0) 
	msg_print("You may learn a specialist ability using the 'O' key.");
      
      /* Redraw Study Status */
      p_ptr->redraw |= (PR_STUDY);
    }
  
  /* Fewer specialties are available (or more forgotten) */
  if (p_ptr->old_specialties > p_ptr->new_specialties)
    {
      if (p_ptr->new_specialties < 0) 
	msg_print("You have lost specialist abilities.");
      
      /* Redraw Study Status */
      p_ptr->redraw |= (PR_STUDY);
    }
  
  /* Save the new_spells value */
  p_ptr->old_specialties = p_ptr->new_specialties;
  
}


/**
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor, and
 * by a shapeshift.
 *
 * This function induces status messages.
 *
 * New treatment of encumberance by LM
 */
static void calc_mana(void)
{
  int msp, levels, cur_wgt, max_wgt, penalty_wgt, armor_penalty;
  
  object_type *o_ptr;
  
  
  /* Hack -- Must possess some magical realm. */
  if (!mp_ptr->spell_realm) return;
  
  /* Hack -- handle "xtra" mode */
  if (character_xtra) return;
  
  /* Extract "effective" player level */
  levels = (p_ptr->lev - mp_ptr->spell_first) + 1;
  
  /* Hack -- no negative mana */
  if (levels < 0) levels = 0;

  /* Extract total mana, using standard rounding. */
  msp = (adj_mag_mana[p_ptr->stat_ind[mp_ptr->spell_stat]] * levels + 5) / 10;
  
  /* The weak spellcasters get half as much mana (rounded up) in Oangband. */
  if (!(check_ability(SP_STRONG_MAGIC)))
    msp = (msp + 1) / 2;
  
  /* Hack -- usually add one mana */
  if (msp) msp++;
  
  /* Modest boost for Clarity ability */
  if (check_ability(SP_CLARITY)) msp += msp / 20;
  
  /* Only mage and Necromancer-type spellcasters are affected by gloves. */
  if ((mp_ptr->spell_book == TV_MAGIC_BOOK) || 
      (mp_ptr->spell_book == TV_NECRO_BOOK))
    {
      /* Assume player is not encumbered by gloves */
      p_ptr->cumber_glove = FALSE;
      
      /* Get the gloves */
      o_ptr = &inventory[INVEN_HANDS];
      
      /* Normal gloves hurt mage or necro-type spells.  Now, only 
       * Free Action or magic mastery stops this effect.
       */
      if (o_ptr->k_idx &&
	  !(o_ptr->flags_obj & OF_FREE_ACT) && 
	  (o_ptr->bonus_other[P_BONUS_M_MASTERY] <= 0))
	{
	  /* Encumbered */
	  p_ptr->cumber_glove = TRUE;
	  
	  /* Reduce mana */
	  msp = 3 * msp / 4;
	}
    }

  
  /* Assume player not encumbered by armor */
  p_ptr->cumber_armor = FALSE;
  
  /* Weigh the armor */
  cur_wgt = 0;
  cur_wgt += inventory[INVEN_BODY].weight;
  cur_wgt += inventory[INVEN_HEAD].weight;
  cur_wgt += inventory[INVEN_ARM].weight;
  cur_wgt += inventory[INVEN_OUTER].weight;
  cur_wgt += inventory[INVEN_HANDS].weight;
  cur_wgt += inventory[INVEN_FEET].weight;
  
  /* Determine the weight allowance */
  max_wgt = mp_ptr->spell_weight1;
  penalty_wgt = mp_ptr->spell_weight2;
  
  /* Specialist Ability */
  if (check_ability(SP_ARMOR_PROFICIENCY))
    {
      max_wgt += 50;
      penalty_wgt += 150;
    }
  
  /* Heavy armor penalizes mana by a percentage. */
  if (cur_wgt > max_wgt)
    {
      /* Calculate Penalty */
      armor_penalty = (msp * (cur_wgt - max_wgt)) / penalty_wgt;
      
      /* If non-zero */
      if (armor_penalty)
	{
	  /* Encumbered */
	  p_ptr->cumber_armor = TRUE;
	  msp -= armor_penalty;
	}
    }
  
  /* Any non-humaniod shape penalizes mana, unless prevented by specialty 
   * ability */
  if (p_ptr->schange)
    {
      /* Chop mana to 2/3. */
      msp = 2 * msp / 3;
    }
  
  
  /* Mana can never be negative */
  if (msp < 0) msp = 0;
  
  
  /* Maximum mana has changed */
  if (p_ptr->msp != msp)
    {
      /* Save new limit */
      p_ptr->msp = msp;
      
      /* Enforce new limit */
      if (p_ptr->csp >= msp)
	{
	  p_ptr->csp = msp;
	  p_ptr->csp_frac = 0;
	}
      
      /* Display mana later */
      p_ptr->redraw |= (PR_MANA);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
    }
  
  
  /* Take note when "glove state" changes */
  if (p_ptr->old_cumber_glove != p_ptr->cumber_glove)
    {
      /* Message */
      if (p_ptr->cumber_glove)
	{
	  msg_print("Your covered hands feel unsuitable for spellcasting.");
	}
      else
	{
	  msg_print("Your hands feel more suitable for spellcasting.");
	}
      
      /* Save it */
      p_ptr->old_cumber_glove = p_ptr->cumber_glove;
    }
  
  
  /* Take note when "armor state" changes */
  if (p_ptr->old_cumber_armor != p_ptr->cumber_armor)
    {
      /* Message */
      if (p_ptr->cumber_armor)
	{
	  msg_print("The weight of your armor encumbers your movement.");
	}
      else
	{
	  msg_print("You feel able to move more freely.");
	}
      
      /* Save it */
      p_ptr->old_cumber_armor = p_ptr->cumber_armor;
    }
}



/**
 * Calculate the players (maximal) hit points
 *
 * Adjust current hitpoints if necessary
 */
static void calc_hitpoints(void)
{
  int bonus, mhp;
  
  /* Hack -- handle "xtra" mode */
  if (character_xtra) return;
  
  /* Un-inflate "half-hitpoint bonus per level" value */
  bonus = ((int)(adj_con_mhp[p_ptr->stat_ind[A_CON]]) - 128);
  
  /* Calculate hitpoints */
  mhp = p_ptr->player_hp[p_ptr->lev-1] + (bonus * p_ptr->lev / 2);
  
  /* Always have at least one hitpoint per level */
  if (mhp < p_ptr->lev + 1) mhp = p_ptr->lev + 1;
  
  /* Modest boost for Athletics ability */
  if (check_ability(SP_ATHLETICS)) mhp += mhp / 20;

  /* New maximum hitpoints */
  if (p_ptr->mhp != mhp)
    {
      /* Save new limit */
      p_ptr->mhp = mhp;
      
      /* Enforce new limit */
      if (p_ptr->chp >= mhp)
	{
	  p_ptr->chp = mhp;
	  p_ptr->chp_frac = 0;
	}
      
      /* Display hitpoints (later) */
      p_ptr->redraw |= (PR_HP);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
    }
}



/**
 * Extract and set the current "lite radius"
 */
static void calc_torch(void)
{
  object_type *o_ptr = &inventory[INVEN_LITE];
  
  /* Assume no light */
  p_ptr->cur_lite = 0;
  
  /* Player is glowing */
  if (p_ptr->lite) 
    {
      notice_obj(OF_LITE, 0);
      p_ptr->cur_lite += 1;
    }
  
  /* Examine actual lites */
  if (o_ptr->tval == TV_LITE)
    {
      /* Torches (with fuel) provide some light */
      if ((o_ptr->sval == SV_LITE_TORCH) && (o_ptr->pval > 0))
	{
	  p_ptr->cur_lite += 1;
	}
      
      /* Lanterns (with fuel) provide more light */
      if ((o_ptr->sval == SV_LITE_LANTERN) && (o_ptr->pval > 0))
	{
	  p_ptr->cur_lite += 2;
	}
      
      /* Artifact Lites provide permanent, bright, light */
      if (artifact_p(o_ptr)) p_ptr->cur_lite += 3;
    }
  
  /* Priests and Paladins get a bonus to light radius at level 35 and
   * 45, respectively.
   */
  if (check_ability(SP_HOLY))
    {
      /* Hack -- the "strong caster" check here is a hack.  
       * What is the better option?  */
      if ((p_ptr->lev > 44) || ((p_ptr->lev > 34) && 
				(check_ability(SP_STRONG_MAGIC))))
	{
	  p_ptr->cur_lite += 1;
	}
    }
  
  
  /* Special ability Holy Light */
  if (check_ability(SP_HOLY_LIGHT)) p_ptr->cur_lite++;
  
  /* Special ability Unlight */
  if (check_ability(SP_UNLIGHT) || p_ptr->darkness) 
    {
      notice_obj(OF_DARKNESS, 0);
      p_ptr->cur_lite--;
    }
  
  /* Reduce lite when running if requested */
  if (p_ptr->running && view_reduce_lite)
    {
      /* Reduce the lite radius if needed */
      if (p_ptr->cur_lite > 1) p_ptr->cur_lite = 1;
    }
  
  /* Notice changes in the "lite radius" */
  if (p_ptr->old_lite != p_ptr->cur_lite)
    {
      /* Update the visuals */
      p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
      
      /* Remember the old lite */
      p_ptr->old_lite = p_ptr->cur_lite;
    }
}



/**
 * Computes current weight limit.
 */
static int weight_limit(void)
{
  int i;
  
  /* Weight limit based only on strength */
  i = adj_str_wgt[p_ptr->stat_ind[A_STR]] * 100;
  
  /* Return the result */
  return (i);
}

/** Calculate all class-based bonuses and penalties to melee Skill.  Oangband
 * recognizes that it takes a great deal of training to get critical hits with
 * a large, heavy weapon - training that many classes simply do not have the
 * time or inclination for.  -LM- 
 */
sint add_special_melee_skill (byte pclass, s16b weight, object_type *o_ptr)
{
  int add_skill = 0;
  int max_weight = 0;
  
  /* Druids and Martial Artists love to fight barehanded */
  if (!o_ptr->k_idx)
    {
      if (check_ability(SP_UNARMED_COMBAT)) add_skill = 14 + (p_ptr->lev);
      else if (check_ability(SP_MARTIAL_ARTS)) add_skill = p_ptr->lev / 2;
    }
  
  
  
  /* Otherwise, check weapon weight */
  else
    {
      /* Maximum compfortable weight depends on class and level */
      max_weight = cp_ptr->max_1 + 
	((p_ptr->lev * (cp_ptr->max_50 - cp_ptr->max_1)) / 50);
      
      /* Too heavy */
      if (weight > max_weight)
	{
	  /* Penalize */
	  add_skill = - ((weight - max_weight) * cp_ptr->penalty) / 100;
	  if (add_skill < - (cp_ptr->max_penalty)) 
	    add_skill = - (cp_ptr->max_penalty);
	}
      
      /* Some classes (Rogues and Assasins) benefit from extra light weapons */
      else if (cp_ptr->bonus)
	{
	  /* Apply bonus */
	  add_skill = ((max_weight - weight) * cp_ptr->bonus) / 100;
	  if (add_skill > cp_ptr->max_bonus) add_skill = cp_ptr->max_bonus;
	}
    }

  /* Priest penalty for non-blessed edged weapons. */
  if ((check_ability(SP_BLESS_WEAPON)) && (check_ability(SP_STRONG_MAGIC)) &&
      ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)) && 
      (!p_ptr->bless_blade))
    {
      add_skill -= 10 + p_ptr->lev / 2;
      
      /* Icky weapon */
      p_ptr->icky_wield = TRUE;
    }

  /* Paladin bonus for blessed weapons. */
  if ((check_ability(SP_BLESS_WEAPON)) && (!check_ability(SP_STRONG_MAGIC)) && 
      (p_ptr->bless_blade)) add_skill += 10 + p_ptr->lev / 4;
  
  /* Now, special racial abilities and limitations are 
   * considered.  Most modifiers are relatively small, to 
   * keep options open to the player. */
  if (o_ptr->tval == TV_SWORD)
    {
      if (check_ability(SP_SWORD_SKILL)) add_skill += 3 + p_ptr->lev / 7;
      else if (check_ability(SP_SWORD_UNSKILL)) 
	add_skill -= 3 + p_ptr->lev / 7;
    }
  
  else if (o_ptr->tval == TV_POLEARM)
    {
      if (check_ability(SP_POLEARM_SKILL)) add_skill += 3 + p_ptr->lev / 7;
      else if (check_ability(SP_POLEARM_UNSKILL)) 
	add_skill -= 3 + p_ptr->lev / 7;
    }
  
  else if (o_ptr->tval == TV_HAFTED)
    {
      if (check_ability(SP_HAFTED_SKILL)) add_skill += 3 + p_ptr->lev / 7;
      else if (check_ability(SP_HAFTED_UNSKILL)) 
	add_skill -= 3 + p_ptr->lev / 7;
    }
  
  return (add_skill);
}

/** Calculate all class and race-based bonuses and 
 * penalties to missile Skill 
 */
sint add_special_missile_skill (byte pclass, s16b weight, object_type *o_ptr)
{
  int add_skill = 0;
  
  /* Nice bonus for most favored weapons - if no tradeoff */
  if ((((check_ability(SP_BOW_SPEED_GREAT)) && 
       (p_ptr->ammo_tval == TV_ARROW)) ||
      ((check_ability(SP_SLING_SPEED_GREAT)) && 
       (p_ptr->ammo_tval == TV_SHOT)) ||
      ((check_ability(SP_XBOW_SPEED_GREAT)) && (p_ptr->ammo_tval == TV_BOLT)))
      && (!check_ability(SP_RAPID_FIRE)))
    {
      /* Big bonus */
      add_skill = 3 + p_ptr->lev / 4;
    }
  
  /* Hack - Unarmed fighters (i.e. Druids) do a bit better with slings*/
  if ((check_ability(SP_UNARMED_COMBAT)) & (p_ptr->ammo_tval == TV_SHOT))
    {
      add_skill = p_ptr->lev / 7;
    }
  
  /* Now, special racial abilities and limitations 
   * are considered.  The choice of race can be of 
   * some significance.
   */
  
  if (p_ptr->ammo_tval == TV_BOLT)
    {
      if (check_ability(SP_XBOW_SKILL)) add_skill += 3 + p_ptr->lev / 7;
      else if (check_ability(SP_XBOW_UNSKILL)) add_skill -= 3 + p_ptr->lev / 7;
    }
  else if (p_ptr->ammo_tval == TV_ARROW)
    {
      if (check_ability(SP_BOW_SKILL)) add_skill += 3 + p_ptr->lev / 7;
      else if (check_ability(SP_BOW_UNSKILL)) add_skill -= 3 + p_ptr->lev / 7;
    }
  else if (p_ptr->ammo_tval == TV_SHOT)
    {
      if (check_ability(SP_SLING_SKILL)) add_skill += 3 + p_ptr->lev / 7;
      else if (check_ability(SP_SLING_UNSKILL)) 
	add_skill -= 3 + p_ptr->lev / 7;
    }
  return (add_skill);
}


/**
 * Paranoid bounds checking on player resistance arrays.
 *
 * These now represent what percentage of damage the player takes -NRM-
 */
static void resistance_limits(void)
{
  int i;
  
  /* Check all extremes */
  for (i = 0; i < MAX_P_RES; i++)
    {
      if (p_ptr->res_list[i] > RES_LEVEL_MAX) 
	p_ptr->res_list[i] = RES_LEVEL_MAX;
      if (p_ptr->res_list[i] < RES_LEVEL_MIN) 
	p_ptr->res_list[i] = RES_LEVEL_MIN;
      if (p_ptr->dis_res_list[i] > RES_LEVEL_MAX) 
	p_ptr->dis_res_list[i] = RES_LEVEL_MAX;
      if (p_ptr->dis_res_list[i] < RES_LEVEL_MIN) 
	p_ptr->dis_res_list[i] = RES_LEVEL_MIN;
    }
}

/**
 * Apply a percentage resistance to the existing player resistance level.
 */
extern void apply_resist(int *player_resist, int item_resist)
{
  *player_resist = (int) (*player_resist * item_resist)/100;
} 



/** Applies vital statistic changes from a shapeshift 
 * to the player.
*/
static void shape_change_stat(void)
{
  switch (p_ptr->schange)
    {
    case SHAPE_NORMAL:
      break;
    case SHAPE_MOUSE:
      {
	p_ptr->stat_add[A_STR] -= 2;
	p_ptr->stat_add[A_INT] -= 7;
	p_ptr->stat_add[A_CON] -= 1;
	p_ptr->stat_add[A_CHR] -= 5;
	break;
      }
    case SHAPE_FERRET:
      {
	p_ptr->stat_add[A_DEX] += 4;
	p_ptr->stat_add[A_CHR] -= 2;
	break;
      }
    case SHAPE_HOUND:
      {
	p_ptr->stat_add[A_CON] += 2;
	p_ptr->stat_add[A_INT] -= 2;
	p_ptr->stat_add[A_CHR] -= 2;
	break;
      }
    case SHAPE_GAZELLE:
      {
	p_ptr->stat_add[A_STR] -= 2;
	p_ptr->stat_add[A_DEX] += 2;
	p_ptr->stat_add[A_CON] -= 1;
	p_ptr->stat_add[A_WIS] -= 2;
	break;
      }
    case SHAPE_LION:
      {
	p_ptr->stat_add[A_STR] += 3;
	p_ptr->stat_add[A_CHR] -= 4;
	p_ptr->stat_add[A_WIS] -= 2;
	p_ptr->stat_add[A_INT] -= 2;
	break;
      }
    case SHAPE_ENT:
      {
	p_ptr->stat_add[A_STR] += 4;
	p_ptr->stat_add[A_WIS] += 1;
	p_ptr->stat_add[A_DEX] -= 5;
	p_ptr->stat_add[A_CON] += 4;
	p_ptr->stat_add[A_CHR] -= 1;
	break;
      }
    case SHAPE_BAT:
      {
	p_ptr->stat_add[A_STR] -= 1;
	p_ptr->stat_add[A_WIS] -= 2;
	p_ptr->stat_add[A_INT] -= 2;
	p_ptr->stat_add[A_CHR] -= 2;
	break;
      }
    case SHAPE_WEREWOLF:
      {
	p_ptr->stat_add[A_STR] += 2;
	p_ptr->stat_add[A_CHR] -= 5;
	p_ptr->stat_add[A_INT] -= 2;
	break;
      }
    case SHAPE_VAMPIRE:
      {
	p_ptr->stat_add[A_STR] += 2;
	p_ptr->stat_add[A_CON] += 1;
	p_ptr->stat_add[A_INT] += 2;
	p_ptr->stat_add[A_CHR] -= 3;
	break;
      }
    case SHAPE_WYRM:
      {
	p_ptr->stat_add[A_STR] += 2;
	p_ptr->stat_add[A_CON] += 1;
	p_ptr->stat_add[A_WIS] += 1;
	p_ptr->stat_add[A_INT] += 1;
	p_ptr->stat_add[A_DEX] -= 1;
	p_ptr->stat_add[A_CHR] -= 1;
	break;
      }
    case SHAPE_BEAR:
      {
	p_ptr->stat_add[A_STR] += 1;
	if (p_ptr->lev >= 10) p_ptr->stat_add[A_STR] += 1;
	if (p_ptr->lev >= 20) p_ptr->stat_add[A_CON] += 1;
	if (p_ptr->lev >= 30) p_ptr->stat_add[A_CON] += 1;
	if (p_ptr->lev >= 40) p_ptr->stat_add[A_STR] += 1;
	p_ptr->stat_add[A_INT] -= 1;
	p_ptr->stat_add[A_CHR] -= 1;
	break;
      }
    }
}

/** A Sangband-derived function to apply all non-stat changes from a shapeshift 
 * to the player.  Any alterations also need to be added to the character 
 * screen (files.c, function "player_flags"), and all timed states 
 * (opposition to the elements for example) must be hacked into the timing of 
 * player states in  dungeon.c.  -LM-
 */
static void shape_change_main(void)
{
  object_type *o_ptr;
  switch (p_ptr->schange)
    {
    case SHAPE_NORMAL:
      break;
    case SHAPE_MOUSE:
      {
	p_ptr->skill_stl = (30 + p_ptr->skill_stl) / 2;
	p_ptr->see_infra += 2;
	p_ptr->aggravate = FALSE;
	p_ptr->to_a -= 5;
	p_ptr->dis_to_a -= 5;
	p_ptr->to_h -= 15;
	p_ptr->dis_to_h -= 15;
	p_ptr->to_d -= 25;
	p_ptr->dis_to_d -= 25;
	p_ptr->skill_dev /= 4;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_FERRET:
      {
	p_ptr->see_infra += 2;
	p_ptr->regenerate = TRUE;
	p_ptr->to_d -= 10;
	p_ptr->dis_to_d -= 10;
	p_ptr->pspeed += 2;
	p_ptr->skill_fos += 10;
	p_ptr->skill_srh += 10;
	p_ptr->skill_dev /= 2;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_HOUND:
      {
	p_ptr->see_infra += 3;
	p_ptr->telepathy = TRUE;
	p_ptr->skill_dev /= 2;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_GAZELLE:
      {
	p_ptr->to_a += 5;
	p_ptr->dis_to_a += 5;
	p_ptr->to_d -= 5;
	p_ptr->dis_to_d -= 5;
	p_ptr->pspeed += 6;
	p_ptr->skill_dev /= 2;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_LION:
      {
	p_ptr->no_fear = TRUE;
	p_ptr->regenerate = TRUE;
	p_ptr->to_a += 5;
	p_ptr->dis_to_a += 5;
	p_ptr->to_h += 10;
	p_ptr->dis_to_h += 10;
	p_ptr->to_d += 15;
	p_ptr->dis_to_d += 15;
	p_ptr->pspeed += 1;
	p_ptr->skill_dev /= 2;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_ENT:
      {
	apply_resist(&p_ptr->res_list[P_RES_COLD], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->dis_res_list[P_RES_COLD], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->res_list[P_RES_POIS], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->dis_res_list[P_RES_POIS], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->res_list[P_RES_FIRE], RES_CUT_MINOR);
	apply_resist(&p_ptr->dis_res_list[P_RES_FIRE], RES_CUT_MINOR);
	if (p_ptr->res_list[P_RES_FIRE] < RES_CAP_MODERATE) 
	  p_ptr->res_list[P_RES_FIRE] = RES_CAP_MODERATE;
	if (p_ptr->dis_res_list[P_RES_FIRE] < RES_CAP_MODERATE) 
	  p_ptr->dis_res_list[P_RES_FIRE] = RES_CAP_MODERATE;
	p_ptr->no_fear = TRUE;
	p_ptr->see_inv = TRUE;
	p_ptr->free_act = TRUE;
	p_ptr->ffall = FALSE;
	p_ptr->to_d += 10;
	p_ptr->dis_to_d += 10;
	p_ptr->skill_dig += 150;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_BAT:
      {
	p_ptr->see_infra += 6;
	p_ptr->no_blind = TRUE;
	p_ptr->ffall = TRUE;
	p_ptr->to_h -= 5;
	p_ptr->dis_to_h -= 5;
	p_ptr->to_d -= 15;
	p_ptr->dis_to_d -= 15;
	p_ptr->pspeed += 5;
	p_ptr->skill_dev /= 4;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    case SHAPE_WEREWOLF:
      {
	p_ptr->see_infra += 3;
	p_ptr->regenerate = TRUE;
	p_ptr->aggravate = TRUE;
	p_ptr->to_a += 5;
	p_ptr->dis_to_a += 5;
	p_ptr->to_h += 20;
	p_ptr->dis_to_h += 20;
	p_ptr->to_d += 20;
	p_ptr->dis_to_d += 20;
	p_ptr->skill_dev /= 2;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;	
      }
    case SHAPE_VAMPIRE:
      {
	p_ptr->see_infra += 3;
	if (p_ptr->cur_lite >= 3) p_ptr->cur_lite = 2;
	p_ptr->see_inv = TRUE;
	p_ptr->hold_life = TRUE;
	apply_resist(&p_ptr->res_list[P_RES_COLD], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->dis_res_list[P_RES_COLD], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->res_list[P_RES_LITE], RES_CUT_MINOR);
	apply_resist(&p_ptr->dis_res_list[P_RES_LITE], RES_CUT_MINOR);
	if (p_ptr->res_list[P_RES_LITE] < RES_CAP_EXTREME) 
	  p_ptr->res_list[P_RES_LITE] = RES_CAP_EXTREME;
	if (p_ptr->dis_res_list[P_RES_LITE] < RES_CAP_EXTREME) 
	  p_ptr->dis_res_list[P_RES_LITE] = RES_CAP_EXTREME;
	p_ptr->regenerate = TRUE;
	p_ptr->to_a += 5;
	p_ptr->dis_to_a += 5;
	p_ptr->to_h += 5;
	p_ptr->dis_to_h += 5;
	p_ptr->to_d += 5;
	p_ptr->dis_to_d += 5;
	p_ptr->skill_stl += 1;
	p_ptr->skill_dev += 30;
	p_ptr->skill_thb -= 10;
	p_ptr->skill_tht -= 10;
	break;
      }
    case SHAPE_WYRM:
      {
	o_ptr = &inventory[INVEN_BODY];
	p_ptr->to_a += 10;
	p_ptr->dis_to_a += 10;
	p_ptr->to_d += 5;
	p_ptr->dis_to_d += 5;
	p_ptr->skill_stl -= 3;
	p_ptr->skill_dev += 10;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	
	/* 
	 * Apply an extra bonus power depending on the type
	 * of DSM when in WYRM form 
	 */
	if (o_ptr->tval == TV_DRAG_ARMOR)
	  {
	    /* Elemental DSM -> immunity */
	    if (o_ptr->sval == SV_DRAGON_BLACK)
	      {
		apply_resist(&p_ptr->res_list[P_RES_ACID], RES_BOOST_IMMUNE);
		apply_resist(&p_ptr->dis_res_list[P_RES_ACID], 
			     RES_BOOST_IMMUNE);
	      }
	    else if (o_ptr->sval == SV_DRAGON_BLUE) 
	      {
		apply_resist(&p_ptr->res_list[P_RES_ELEC], RES_BOOST_IMMUNE);
		apply_resist(&p_ptr->dis_res_list[P_RES_ELEC], 
			     RES_BOOST_IMMUNE);
	      }
	    else if (o_ptr->sval == SV_DRAGON_WHITE) 
	      {
		apply_resist(&p_ptr->res_list[P_RES_COLD], RES_BOOST_IMMUNE);
		apply_resist(&p_ptr->dis_res_list[P_RES_COLD], 
			     RES_BOOST_IMMUNE);
	      }
	    else if (o_ptr->sval == SV_DRAGON_RED) 
	      {
		apply_resist(&p_ptr->res_list[P_RES_FIRE], RES_BOOST_IMMUNE);
		apply_resist(&p_ptr->dis_res_list[P_RES_FIRE], 
			     RES_BOOST_IMMUNE);
	      }
	    else if (o_ptr->sval == SV_DRAGON_GREEN) 
	      {
		apply_resist(&p_ptr->res_list[P_RES_POIS], RES_BOOST_IMMUNE);
		apply_resist(&p_ptr->dis_res_list[P_RES_POIS], 
			     RES_BOOST_IMMUNE);
	      }
	    
	    /* Shining DSM -> SI */
	    else if (o_ptr->sval == SV_DRAGON_SHINING) 
	      p_ptr->see_inv = TRUE;
	    
	    /* Law/Chaos DSM -> hold life */
	    else if (o_ptr->sval == SV_DRAGON_LAW) 
	      p_ptr->hold_life = TRUE;
	    else if (o_ptr->sval == SV_DRAGON_CHAOS) 
	      p_ptr->hold_life = TRUE;
	    
	    /* Bronze/Gold DSM -> FA */
	    else if (o_ptr->sval == SV_DRAGON_BRONZE) 
	      p_ptr->free_act = TRUE;
	    else if (o_ptr->sval == SV_DRAGON_GOLD) 
	      p_ptr->free_act = TRUE;
	    
	    /* Multihued, Balance and Power don't need any help */
	  }
	break;
      }
    case SHAPE_BEAR:
      {
	p_ptr->to_a += 5;
	p_ptr->dis_to_a += 5;
	p_ptr->to_h += 5;
	p_ptr->dis_to_h += 5;
	if (p_ptr->lev >= 10) 
	  {
	    p_ptr->to_d += 5;
	    p_ptr->dis_to_d += 5;
	  }
	if (p_ptr->lev >= 20) 
	  {
	    p_ptr->to_d += 5;
	    p_ptr->dis_to_d += 5;
	  }
	if (p_ptr->lev >= 30) 
	  {
	    p_ptr->to_a += 10;
	    p_ptr->dis_to_a += 10;
	  }
	if (p_ptr->lev >= 40) 
	  {
	    p_ptr->to_h += 5;
	    p_ptr->dis_to_h += 5;
	  }
	p_ptr->skill_dev /= 2;
	p_ptr->skill_thb -= 30;
	p_ptr->skill_tht -= 30;
	break;
      }
    }
  
  /* End shape bonuses; do bounds check on resistance levels */
  resistance_limits();
}



/**
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 *
 * This is the "kitchen sink" function!	 I may get around to 
 * segmenting it, simply to make it more readable...  -LM-
 *
 * I have added class-specific modifiers to Skill and enforced a max
 * of one blow/rnd for weapons too heavy to wield effectively. -LM-
 *
 * This function calls itself if the player's STR stat changes. -LM-
 *
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 */
extern void calc_bonuses(bool inspect)
{
  int i, j, hold;
  
  int old_speed;
  
  int old_stealth;
  
  int old_see_infra;
  int old_telepathy;
  int old_see_inv;
  
  int temp_armour;
  
  int old_dis_ac;
  int old_dis_to_a;
  
  int extra_shots;
  int extra_might;
  
  int old_stat_top[A_MAX];
  int old_stat_use[A_MAX];
  int old_stat_ind[A_MAX];
  
  bool enhance = FALSE;
  
  object_type *o_ptr;
  
  /*** Memorize ***/

  
  /* Save the old speed */
  old_speed = p_ptr->pspeed;
  
  /* Save the old stealth */
  old_stealth = p_ptr->skill_stl;
  
  /* Save the old vision stuff */
  old_telepathy = p_ptr->telepathy;
  old_see_inv = p_ptr->see_inv;
  old_see_infra = p_ptr->see_infra;
  
  /* Save the old armor class */
  old_dis_ac = p_ptr->dis_ac;
  old_dis_to_a = p_ptr->dis_to_a;
  
  /* Save the old stats */
  for (i = 0; i < A_MAX; i++)
    {
      old_stat_top[i] = p_ptr->stat_top[i];
      old_stat_use[i] = p_ptr->stat_use[i];
      old_stat_ind[i] = p_ptr->stat_ind[i];
    }
  
  
  /* Hack - If the player's usage of his shield changes, we must
   * recalculate various things.
   */
 calc_again:
  
  
  /*** Reset ***/
  
  /* Reset player speed */
  p_ptr->pspeed = 110;
  
  /* Reset "blow" info */
  p_ptr->num_blow = 1;
  
  /* Reset "fire" info */
  p_ptr->num_fire = 0;
  p_ptr->ammo_mult = 0;
  p_ptr->ammo_tval = 0;
  extra_shots = 0;
  extra_might = 0;
  
  /* Clear the stat modifiers */
  for (i = 0; i < A_MAX; i++) p_ptr->stat_add[i] = 0;
  
  /* Clear the Displayed/Real armor class */
  p_ptr->dis_ac = p_ptr->ac = 0;
  
  /* Clear the Displayed/Real Bonuses */
  p_ptr->dis_to_h = p_ptr->to_h = 0;
  p_ptr->dis_to_d = p_ptr->to_d = 0;
  p_ptr->dis_to_a = p_ptr->to_a = 0;
  
  /* Clear all the flags */
  p_ptr->teleport = FALSE;
  p_ptr->no_teleport = FALSE;
  p_ptr->aggravate = FALSE;
  p_ptr->rand_aggro = FALSE;
  p_ptr->slow_regen = FALSE;
  p_ptr->fear = FALSE;
  p_ptr->fast_digest = FALSE;
  p_ptr->rand_pois = FALSE;
  p_ptr->rand_pois_bad = FALSE;
  p_ptr->rand_cuts = FALSE;
  p_ptr->rand_cuts_bad = FALSE;
  p_ptr->rand_hallu = FALSE;
  p_ptr->drop_weapon = FALSE;
  p_ptr->attract_demon = FALSE;
  p_ptr->attract_undead = FALSE;
  p_ptr->rand_paral = FALSE;
  p_ptr->rand_paral_all = FALSE;
  p_ptr->drain_exp = FALSE;
  p_ptr->drain_mana = FALSE;
  p_ptr->drain_stat = FALSE;
  p_ptr->drain_charge = FALSE;
  p_ptr->bless_blade = FALSE;
  p_ptr->impact = FALSE;
  p_ptr->see_inv = FALSE;
  p_ptr->free_act = FALSE;
  p_ptr->slow_digest = FALSE;
  p_ptr->regenerate = FALSE;
  p_ptr->ffall = FALSE;
  p_ptr->hold_life = FALSE;
  p_ptr->telepathy = FALSE;
  p_ptr->lite = FALSE;
  p_ptr->sustain_str = FALSE;
  p_ptr->sustain_int = FALSE;
  p_ptr->sustain_wis = FALSE;
  p_ptr->sustain_con = FALSE;
  p_ptr->sustain_dex = FALSE;
  p_ptr->sustain_chr = FALSE;
  p_ptr->no_fear = FALSE;
  p_ptr->no_blind = FALSE;
  p_ptr->darkness = FALSE;
  p_ptr->special_attack &= ~(ATTACK_CHAOTIC);
  
  for (i = 0; i < MAX_P_RES; i++)
    {
      p_ptr->res_list[i] = RES_LEVEL_BASE;
      p_ptr->dis_res_list[i] = RES_LEVEL_BASE;
    }
  
  
  /*** Extract race/class info ***/
  
  /* Base infravision (purely racial) */
  p_ptr->see_infra = rp_ptr->infra;
  
  /* Base skill -- disarming */
  p_ptr->skill_dis = rp_ptr->r_dis + cp_ptr->c_dis;
  
  /* Base skill -- magic devices */
  p_ptr->skill_dev = rp_ptr->r_dev + cp_ptr->c_dev;
  
  /* Base skill -- saving throw */
  p_ptr->skill_sav = rp_ptr->r_sav + cp_ptr->c_sav;
  
  /* Base skill -- stealth */
  p_ptr->skill_stl = rp_ptr->r_stl + cp_ptr->c_stl;
  
  /* Base skill -- searching ability */
  p_ptr->skill_srh = rp_ptr->r_srh + cp_ptr->c_srh;
  
  /* Base skill -- searching frequency */
  p_ptr->skill_fos = rp_ptr->r_fos + cp_ptr->c_fos;
  
  /* Base skill -- combat (melee) */
  p_ptr->skill_thn = rp_ptr->r_thn + cp_ptr->c_thn;
  
  /* Base skill -- combat (shooting) */
  p_ptr->skill_thb = rp_ptr->r_thb + cp_ptr->c_thb;
  
  /* Base skill -- combat (throwing) */
  p_ptr->skill_tht = rp_ptr->r_thb + cp_ptr->c_thb;
  
  /* Base skill -- digging */
  p_ptr->skill_dig = 0;
  
  /*** Analyze player ***/
  
  /* Object flags */
  if (rp_ptr->flags_obj & (OF_SUSTAIN_STR)) p_ptr->sustain_str = TRUE;
  if (rp_ptr->flags_obj & (OF_SUSTAIN_INT)) p_ptr->sustain_int = TRUE;
  if (rp_ptr->flags_obj & (OF_SUSTAIN_WIS)) p_ptr->sustain_wis = TRUE;
  if (rp_ptr->flags_obj & (OF_SUSTAIN_DEX)) p_ptr->sustain_dex = TRUE;
  if (rp_ptr->flags_obj & (OF_SUSTAIN_CON)) p_ptr->sustain_con = TRUE;
  if (rp_ptr->flags_obj & (OF_SUSTAIN_CHR)) p_ptr->sustain_chr = TRUE;
  if (rp_ptr->flags_obj & (OF_SLOW_DIGEST)) p_ptr->slow_digest = TRUE;
  if (rp_ptr->flags_obj & (OF_FEATHER)) p_ptr->ffall = TRUE;
  if (rp_ptr->flags_obj & (OF_LITE)) p_ptr->lite = TRUE;
  if (rp_ptr->flags_obj & (OF_REGEN)) p_ptr->regenerate = TRUE;
  if (rp_ptr->flags_obj & (OF_TELEPATHY)) p_ptr->telepathy = TRUE;
  if (rp_ptr->flags_obj & (OF_SEE_INVIS)) p_ptr->see_inv = TRUE;
  if (rp_ptr->flags_obj & (OF_FREE_ACT)) p_ptr->free_act = TRUE;
  if (rp_ptr->flags_obj & (OF_HOLD_LIFE)) p_ptr->hold_life = TRUE;
  if (rp_ptr->flags_obj & (OF_BLESSED)) p_ptr->bless_blade = TRUE;
  if (rp_ptr->flags_obj & (OF_IMPACT)) p_ptr->impact = TRUE;
  if (rp_ptr->flags_obj & (OF_FEARLESS)) p_ptr->no_fear = TRUE;
  if (rp_ptr->flags_obj & (OF_SEEING)) p_ptr->no_blind = TRUE;
  if (rp_ptr->flags_obj & (OF_DARKNESS)) p_ptr->darkness = TRUE;
  if (rp_ptr->flags_obj & (OF_CHAOTIC)) p_ptr->special_attack |= ATTACK_CHAOTIC;
  

  /* Curse flags */
  if (rp_ptr->flags_curse & (CF_TELEPORT)) p_ptr->teleport = TRUE;
  if (rp_ptr->flags_curse & (CF_NO_TELEPORT)) p_ptr->no_teleport = TRUE;
  if (rp_ptr->flags_curse & (CF_AGGRO_PERM)) p_ptr->aggravate = TRUE;
  if (rp_ptr->flags_curse & (CF_AGGRO_RAND)) p_ptr->rand_aggro = TRUE;
  if (rp_ptr->flags_curse & (CF_SLOW_REGEN)) p_ptr->slow_regen = TRUE;
  if (rp_ptr->flags_curse & (CF_AFRAID)) p_ptr->fear = TRUE;
  if (rp_ptr->flags_curse & (CF_HUNGRY)) p_ptr->fast_digest = TRUE;
  if (rp_ptr->flags_curse & (CF_POIS_RAND)) p_ptr->rand_pois = TRUE;
  if (rp_ptr->flags_curse & (CF_POIS_RAND_BAD)) p_ptr->rand_pois_bad = TRUE;
  if (rp_ptr->flags_curse & (CF_CUT_RAND)) p_ptr->rand_cuts = TRUE;
  if (rp_ptr->flags_curse & (CF_CUT_RAND_BAD)) p_ptr->rand_cuts_bad = TRUE;
  if (rp_ptr->flags_curse & (CF_HALLU_RAND)) p_ptr->rand_hallu = TRUE;
  if (rp_ptr->flags_curse & (CF_DROP_WEAPON)) p_ptr->drop_weapon = TRUE;
  if (rp_ptr->flags_curse & (CF_ATTRACT_DEMON)) p_ptr->attract_demon = TRUE;
  if (rp_ptr->flags_curse & (CF_ATTRACT_UNDEAD)) p_ptr->attract_undead = TRUE;
  if (rp_ptr->flags_curse & (CF_PARALYZE)) p_ptr->rand_paral = TRUE;
  if (rp_ptr->flags_curse & (CF_PARALYZE_ALL)) p_ptr->rand_paral_all = TRUE;
  if (rp_ptr->flags_curse & (CF_DRAIN_EXP)) p_ptr->drain_exp = TRUE;
  if (rp_ptr->flags_curse & (CF_DRAIN_MANA)) p_ptr->drain_mana = TRUE;
  if (rp_ptr->flags_curse & (CF_DRAIN_STAT)) p_ptr->drain_stat = TRUE;
  if (rp_ptr->flags_curse & (CF_DRAIN_CHARGE)) p_ptr->drain_charge = TRUE;
  
  /* Resistances */
  for (i = 0; i < MAX_P_RES; i++)
    {
      p_ptr->res_list[i] = rp_ptr->percent_res[i];
      p_ptr->dis_res_list[i] = rp_ptr->percent_res[i];
    }
  
  /* Ent */
  if (check_ability(SP_WOODEN)) 
    {
      /* Ents dig like maniacs, but only with their hands. */
      if (!inventory[INVEN_WIELD].k_idx) 
	p_ptr->skill_dig += p_ptr->lev * 10;
      
      /* Ents get tougher and stronger as they age, but lose dexterity. */
      if (p_ptr->lev > 25) p_ptr->stat_add[A_STR]++;
      if (p_ptr->lev > 40) p_ptr->stat_add[A_STR]++;
      if (p_ptr->lev > 45) p_ptr->stat_add[A_STR]++;
      
      if (p_ptr->lev > 25) p_ptr->stat_add[A_DEX]--;
      if (p_ptr->lev > 40) p_ptr->stat_add[A_DEX]--;
      if (p_ptr->lev > 45) p_ptr->stat_add[A_DEX]--;
      
      if (p_ptr->lev > 25) p_ptr->stat_add[A_CON]++;
      if (p_ptr->lev > 40) p_ptr->stat_add[A_CON]++;
      if (p_ptr->lev > 45) p_ptr->stat_add[A_CON]++;
    }
  
  /* Warrior. */
  if (check_ability(SP_RELENTLESS))
    {
      if (p_ptr->lev >= 30) p_ptr->no_fear = TRUE;
      if (p_ptr->lev >= 40) p_ptr->regenerate = TRUE;
    }
  
  /* Specialty ability Holy Light */
  if (check_ability(SP_HOLY_LIGHT))
    {
      apply_resist(&p_ptr->res_list[P_RES_LITE], RES_BOOST_NORMAL);
      apply_resist(&p_ptr->dis_res_list[P_RES_LITE], RES_BOOST_NORMAL);
    }
  
  /* Specialty ability Unlight */
  if (check_ability(SP_UNLIGHT))
    {
      apply_resist(&p_ptr->res_list[P_RES_DARK], RES_BOOST_NORMAL);
      apply_resist(&p_ptr->dis_res_list[P_RES_DARK], RES_BOOST_NORMAL);
    }
  
  /* End inherent resistances; do bounds check on resistance levels */
  resistance_limits();
  
  /*** Analyze equipment ***/
  
  /* Scan the equipment */
  for (i = INVEN_WIELD; i < INVEN_SUBTOTAL; i++)
    {
      o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Affect stats */
      for (j = 0; j < A_MAX; j++)
	p_ptr->stat_add[j] += o_ptr->bonus_stat[j];
      
      /* Affect stealth */
      p_ptr->skill_stl += o_ptr->bonus_other[P_BONUS_STEALTH];
      
      /* Affect searching ability (factor of five) */
      p_ptr->skill_srh += (o_ptr->bonus_other[P_BONUS_SEARCH] * 5);
      
      /* Affect searching frequency (factor of five) */
      p_ptr->skill_fos += (o_ptr->bonus_other[P_BONUS_SEARCH] * 5);
      
      /* Affect infravision */
      p_ptr->see_infra += o_ptr->bonus_other[P_BONUS_INFRA];
      
      /* Affect digging (factor of 20) */
      p_ptr->skill_dig += (o_ptr->bonus_other[P_BONUS_TUNNEL] * 20);
      
      /* Affect speed */
      p_ptr->pspeed += o_ptr->bonus_other[P_BONUS_SPEED];
      
      p_ptr->skill_dev += 10 * o_ptr->bonus_other[P_BONUS_M_MASTERY];
      
      /* Affect shots.  Altered in Oangband. */
      extra_shots += o_ptr->bonus_other[P_BONUS_SHOTS];
      
      /* Affect might.  Altered in Oangband. */
      extra_might += o_ptr->bonus_other[P_BONUS_MIGHT];
      
      /* Object flags */
      if (o_ptr->flags_obj & (OF_SUSTAIN_STR)) p_ptr->sustain_str = TRUE;
      if (o_ptr->flags_obj & (OF_SUSTAIN_INT)) p_ptr->sustain_int = TRUE;
      if (o_ptr->flags_obj & (OF_SUSTAIN_WIS)) p_ptr->sustain_wis = TRUE;
      if (o_ptr->flags_obj & (OF_SUSTAIN_DEX)) p_ptr->sustain_dex = TRUE;
      if (o_ptr->flags_obj & (OF_SUSTAIN_CON)) p_ptr->sustain_con = TRUE;
      if (o_ptr->flags_obj & (OF_SUSTAIN_CHR)) p_ptr->sustain_chr = TRUE;
      if (o_ptr->flags_obj & (OF_SLOW_DIGEST)) p_ptr->slow_digest = TRUE;
      if (o_ptr->flags_obj & (OF_FEATHER)) p_ptr->ffall = TRUE;
      if (o_ptr->flags_obj & (OF_LITE)) p_ptr->lite = TRUE;
      if (o_ptr->flags_obj & (OF_REGEN)) p_ptr->regenerate = TRUE;
      if (o_ptr->flags_obj & (OF_TELEPATHY)) p_ptr->telepathy = TRUE;
      if (o_ptr->flags_obj & (OF_SEE_INVIS)) p_ptr->see_inv = TRUE;
      if (o_ptr->flags_obj & (OF_FREE_ACT)) p_ptr->free_act = TRUE;
      if (o_ptr->flags_obj & (OF_HOLD_LIFE)) p_ptr->hold_life = TRUE;
      if (o_ptr->flags_obj & (OF_FEARLESS)) p_ptr->no_fear = TRUE;
      if (o_ptr->flags_obj & (OF_SEEING)) p_ptr->no_blind = TRUE;
      if (o_ptr->flags_obj & (OF_IMPACT)) p_ptr->impact = TRUE;
      if (o_ptr->flags_obj & (OF_BLESSED)) p_ptr->bless_blade = TRUE;
      if (o_ptr->flags_obj & (OF_DARKNESS)) p_ptr->darkness = TRUE;
      if (o_ptr->flags_obj & (OF_CHAOTIC)) 
	p_ptr->special_attack |= ATTACK_CHAOTIC;
      
      /* Bad flags */
      if (o_ptr->flags_curse & (CF_TELEPORT)) p_ptr->teleport = TRUE;
      if (o_ptr->flags_curse & (CF_NO_TELEPORT)) p_ptr->no_teleport = TRUE;
      if (o_ptr->flags_curse & (CF_AGGRO_PERM)) p_ptr->aggravate = TRUE;
      if (o_ptr->flags_curse & (CF_AGGRO_RAND)) p_ptr->rand_aggro = TRUE;
      if (o_ptr->flags_curse & (CF_SLOW_REGEN)) p_ptr->slow_regen = TRUE;
      if (o_ptr->flags_curse & (CF_AFRAID)) p_ptr->fear = TRUE;
      if (o_ptr->flags_curse & (CF_HUNGRY)) p_ptr->fast_digest = TRUE;
      if (o_ptr->flags_curse & (CF_POIS_RAND)) p_ptr->rand_pois = TRUE;
      if (o_ptr->flags_curse & (CF_POIS_RAND_BAD)) p_ptr->rand_pois_bad = TRUE;
      if (o_ptr->flags_curse & (CF_CUT_RAND)) p_ptr->rand_cuts = TRUE;
      if (o_ptr->flags_curse & (CF_CUT_RAND_BAD)) p_ptr->rand_cuts_bad = TRUE;
      if (o_ptr->flags_curse & (CF_HALLU_RAND)) p_ptr->rand_hallu = TRUE;
      if (o_ptr->flags_curse & (CF_DROP_WEAPON)) p_ptr->drop_weapon = TRUE;
      if (o_ptr->flags_curse & (CF_ATTRACT_DEMON)) p_ptr->attract_demon = TRUE;
      if (o_ptr->flags_curse & (CF_ATTRACT_UNDEAD)) 
	p_ptr->attract_undead = TRUE;
      if (o_ptr->flags_curse & (CF_PARALYZE)) p_ptr->rand_paral = TRUE;
      if (o_ptr->flags_curse & (CF_PARALYZE_ALL)) p_ptr->rand_paral_all = TRUE;
      if (o_ptr->flags_curse & (CF_DRAIN_EXP)) p_ptr->drain_exp = TRUE;
      if (o_ptr->flags_curse & (CF_DRAIN_MANA)) p_ptr->drain_mana = TRUE;
      if (o_ptr->flags_curse & (CF_DRAIN_STAT)) p_ptr->drain_stat = TRUE;
      if (o_ptr->flags_curse & (CF_DRAIN_CHARGE)) p_ptr->drain_charge = TRUE;
      
      
      for (j = 0; j < MAX_P_RES; j++)
	apply_resist(&p_ptr->res_list[j], o_ptr->percent_res[j]);
      
      /* Known resistance and immunity flags */
      for (j = 0; j < MAX_P_RES; j++)
	if (o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j))
	  apply_resist(&p_ptr->dis_res_list[j], o_ptr->percent_res[j]);
      /* End item resistances; do bounds check on resistance levels */
      resistance_limits();
      
      /* 
       * Modify the base armor class.   Shields worn on back are penalized. 
       * Shield and Armor masters benefit.
       */
      if ((i == INVEN_ARM) && (p_ptr->shield_on_back)) 
	temp_armour = o_ptr->ac / 3;
      else if ((i == INVEN_ARM) && (check_ability(SP_SHIELD_MAST))) 
	temp_armour = o_ptr->ac * 2;
      else if ((i == INVEN_BODY) && (check_ability(SP_ARMOR_MAST))) 
	temp_armour = (o_ptr->ac * 5)/ 3;
      else temp_armour = o_ptr->ac;
      
      p_ptr->ac += temp_armour;
      
      /* The base armor class is always known */
      p_ptr->dis_ac += temp_armour;
      
      /* Apply the bonuses to armor class.  Shields worn on back are 
       * penalized.
       */
      if ((p_ptr->shield_on_back) && (i == INVEN_ARM)) 
			temp_armour = o_ptr->to_a / 2;
      else temp_armour = o_ptr->to_a;
      
      p_ptr->to_a += temp_armour;
      
      /* Apply the mental bonuses to armor class, if known */
      if (o_ptr->id_other & IF_TO_A) p_ptr->dis_to_a += temp_armour;
      
      /* Hack -- do not apply "weapon" bonuses */
      if (i == INVEN_WIELD) continue;
      
      /* Hack -- do not apply "bow" bonuses */
      if (i == INVEN_BOW) continue;
      
      /* Apply the bonuses to hit/damage */
      p_ptr->to_h += o_ptr->to_h;
      p_ptr->to_d += o_ptr->to_d;
      
      /* Apply the mental bonuses tp hit/damage, if known */
      if (o_ptr->id_other & IF_TO_H) p_ptr->dis_to_h += o_ptr->to_h;
      if (o_ptr->id_other & IF_TO_D) p_ptr->dis_to_d += o_ptr->to_d;
    }
  
  /* Hack -- clear a few flags for certain races. */
  
  /* The dark elf's saving grace */
  if ((check_ability(SP_SHADOW)) && (p_ptr->aggravate)) 
    {
      p_ptr->skill_stl -= 3;
      p_ptr->aggravate = FALSE;
    }
  
  /* Nothing, but nothing, can make an Ent lightfooted. */
  if (check_ability(SP_WOODEN)) p_ptr->ffall = FALSE;
  
  
  /*** Analyze shapechanges - statistics only ***/
  shape_change_stat();
  
  /*** (Most) Specialty Abilities ***/
  
  /* Physical stat boost */
  if (check_ability(SP_ATHLETICS))
    {
      p_ptr->stat_add[A_DEX] += 2;
      p_ptr->stat_add[A_CON] += 2;
    }
  
  /* Mental stat boost */
  if (check_ability(SP_CLARITY))
    {
      p_ptr->stat_add[A_INT] += 2;
      p_ptr->stat_add[A_WIS] += 2;
    }
  
  /* Unlight stealth boost */
  if (check_ability(SP_UNLIGHT))
    {
      if ((p_ptr->cur_lite <= 0) && (!is_daylight) &&
	  !(cave_info[p_ptr->py][p_ptr->px] & CAVE_GLOW)) 
	p_ptr->skill_stl += 6; 
      else p_ptr->skill_stl += 3; 
    }
  
  /* Speed Boost (Fury, Phasewalk) */
  if (p_ptr->speed_boost)
    {
      p_ptr->pspeed += (p_ptr->speed_boost + 5) / 10;
    }
  
  /* Speed boost in trees for elven druids and rangers */
  if ((check_ability(SP_WOODSMAN)) && (check_ability(SP_ELVEN)) && 
      (f_info[cave_feat[p_ptr->py][p_ptr->px]].flags & TF_TREE))
    p_ptr->pspeed += 3;

  /* Speed boost for rune of speed */
  if (cave_feat[p_ptr->py][p_ptr->px] == FEAT_RUNE_SPEED)
    p_ptr->pspeed += 10;

  /* Dwarves are good miners */
  if (check_ability(SP_DWARVEN))
    p_ptr->skill_dig += 40;

  /*** Handle stats ***/
  
  /* Calculate stats */
  for (i = 0; i < A_MAX; i++)
    {
      int add, top, use, ind;
      
      /* Extract modifier */
      add = p_ptr->stat_add[i];
      
      /* Modify the stats for race/class */
      add += (rp_ptr->r_adj[i] + cp_ptr->c_adj[i]);
      
      /* Extract the new "stat_top" value for the stat */
      top = modify_stat_value(p_ptr->stat_max[i], add);
      
      /* Save the new value */
      p_ptr->stat_top[i] = top;
      
      /* Extract the new "stat_use" value for the stat */
      use = modify_stat_value(p_ptr->stat_cur[i], add);
      
      /* Save the new value */
      p_ptr->stat_use[i] = use;
      
      /* Values: 3, 4, ..., 17 */
      if (use <= 18) ind = (use - 3);
      
      /* Ranges: 18/00-18/09, ..., 18/210-18/219 */
      else if (use <= 18+219) ind = (15 + (use - 18) / 10);
      
      /* Range: 18/220+ */
      else ind = (37);
      
      /* Save the new index */
      p_ptr->stat_ind[i] = ind;
    }
  
  /* Assume no evasion */
  p_ptr->evasion_chance = 0;
  
  /* Evasion AC boost */
  if (check_ability(SP_EVASION) ||
      ((check_ability(SP_DWARVEN)) && 
       (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAIN)) ||
      ((check_ability(SP_PLAINSMAN)) && 
       (stage_map[p_ptr->stage][STAGE_TYPE] == PLAIN)) ||
      ((check_ability(SP_EDAIN)) && 
       (stage_map[p_ptr->stage][STAGE_TYPE] == FOREST)))
    {
      int cur_wgt = 0;
      int evasion_wgt;
      int max_bonus;
      
      /* Weigh the armor */
      cur_wgt += inventory[INVEN_BODY].weight;
      cur_wgt += inventory[INVEN_HEAD].weight;
      cur_wgt += inventory[INVEN_ARM].weight;
      cur_wgt += inventory[INVEN_OUTER].weight;
      cur_wgt += inventory[INVEN_HANDS].weight;
      cur_wgt += inventory[INVEN_FEET].weight;
      
      /* Highest weight to get any bonus */
      evasion_wgt = 150 + (3 * p_ptr->lev);
      
      /* Highest bonus we can get at this level */
      max_bonus = adj_dex_evas[p_ptr->stat_ind[A_DEX]];
      
      /* Do we get the max bonus? */
      if (cur_wgt <= ((6 * evasion_wgt) / 10))
	{
	  p_ptr->evasion_chance = max_bonus;
	}
      
      /* Do we get any bonus? */
      else if (cur_wgt <= evasion_wgt)
	{
	  p_ptr->evasion_chance = max_bonus / 2;
	}
    }
  
  
  /*** Temporary flags ***/
  
  /* Hack - Temporary bonuses are stronger with Enhance Magic */
  if (check_ability(SP_ENHANCE_MAGIC)) enhance = TRUE;
  
  /* Temporary resists */
  if (p_ptr->oppose_acid)
    {
      int bonus = RES_BOOST_GREAT;
      if (enhance) apply_resist(&bonus, RES_BOOST_MINOR);
      apply_resist(&p_ptr->res_list[P_RES_ACID], bonus);
      apply_resist(&p_ptr->dis_res_list[P_RES_ACID], bonus);
    }
  if (p_ptr->oppose_fire)
    {
      int bonus = RES_BOOST_GREAT;
      if (enhance) apply_resist(&bonus, RES_BOOST_MINOR);
      apply_resist(&p_ptr->res_list[P_RES_FIRE], bonus);
      apply_resist(&p_ptr->dis_res_list[P_RES_FIRE], bonus);
    }
  if (p_ptr->oppose_cold)
    {
      int bonus = RES_BOOST_GREAT;
      if (enhance) apply_resist(&bonus, RES_BOOST_MINOR);
      apply_resist(&p_ptr->res_list[P_RES_COLD], bonus);
      apply_resist(&p_ptr->dis_res_list[P_RES_COLD], bonus);
    }
  if (p_ptr->oppose_elec)
    {
      int bonus = RES_BOOST_GREAT;
      if (enhance) apply_resist(&bonus, RES_BOOST_MINOR);
      apply_resist(&p_ptr->res_list[P_RES_ELEC], bonus);
      apply_resist(&p_ptr->dis_res_list[P_RES_ELEC], bonus);
    }
  if (p_ptr->oppose_pois)
    {
      int bonus = RES_BOOST_GREAT;
      if (enhance) apply_resist(&bonus, RES_BOOST_MINOR);
      apply_resist(&p_ptr->res_list[P_RES_POIS], bonus);
      apply_resist(&p_ptr->dis_res_list[P_RES_POIS], bonus);
    }
  
  /* Apply temporary "stun".  */
  if (p_ptr->stun > 50)
    {
      p_ptr->to_h -= 20;
      p_ptr->dis_to_h -= 20;
      p_ptr->to_d -= 20;
      p_ptr->dis_to_d -= 20;
    }
  else if (p_ptr->stun)
    {
      p_ptr->to_h -= 5;
      p_ptr->dis_to_h -= 5;
      p_ptr->to_d -= 5;
      p_ptr->dis_to_d -= 5;
    }
  
  /* Heightened magical defenses.   Saving Throw effect added later */
  if (p_ptr->magicdef)
    {
      int bonus = ((enhance == TRUE) ? 35 : 25);
      
      p_ptr->to_a += bonus;
      p_ptr->dis_to_a += bonus;
      
      apply_resist(&p_ptr->res_list[P_RES_CONFU], RES_BOOST_NORMAL);
      apply_resist(&p_ptr->dis_res_list[P_RES_CONFU], RES_BOOST_NORMAL);
      p_ptr->no_blind = TRUE;
    }
  
  /* Temporary blessing */
  if (p_ptr->blessed)
    {
      int bonus1 = ((enhance == TRUE) ? 10 : 5);
      int bonus2 = ((enhance == TRUE) ? 15 : 10);
      
      p_ptr->to_a += bonus1;
      p_ptr->dis_to_a += bonus1;
      p_ptr->to_h += bonus2;
      p_ptr->dis_to_h += bonus2;
    }
  
  /* Temporary shield.  Added an exception for Necromancers to keep
   * them in line.
   */
  if ((p_ptr->shield) && (check_ability(SP_EVIL)))
    {
      int bonus = ((enhance == TRUE) ? 50 : 35);
      
      p_ptr->to_a += bonus;
      p_ptr->dis_to_a += bonus;
    }
  else if (p_ptr->shield)
    {
      int bonus = ((enhance == TRUE) ? 65 : 50);
      
      p_ptr->to_a += bonus;
      p_ptr->dis_to_a += bonus;
    }
  
  /* Temporary "Hero".  Now also increases Deadliness. */
  if (p_ptr->hero)
    {
      int bonus1 = ((enhance == TRUE) ? 15 : 10);
      int bonus2 = ((enhance == TRUE) ? 10 : 5);
      
      p_ptr->to_h += bonus1;
      p_ptr->dis_to_h += bonus1;
      p_ptr->to_d += bonus2;
      p_ptr->dis_to_d += bonus2;
    }
  
  /* Temporary "Berserk".   Now also increases Deadliness. */
  if (p_ptr->shero)
    {
      int bonus1 = ((enhance == TRUE) ? 10 : 5);
      int bonus2 = ((enhance == TRUE) ? 20 : 15);
      int bonus3 = ((enhance == TRUE) ? 15 : 10);
      
      p_ptr->to_h += bonus1;
      p_ptr->dis_to_h += bonus1;
      p_ptr->to_d += bonus2;
      p_ptr->dis_to_d += bonus2;
      p_ptr->to_a -= bonus3;
      p_ptr->dis_to_a -= bonus3;
      
      /* but berserkers make *lousy* archers. */
      p_ptr->skill_thb -= 20;
      p_ptr->skill_tht -= 20;
    }
  
  /* Temporary "fast" */
  if (p_ptr->fast)
    {
      int bonus = ((enhance == TRUE) ? 13 : 10);
      
      p_ptr->pspeed += bonus;
    }
  
  /* Temporary "slow" */
  if (p_ptr->slow)
    {
      p_ptr->pspeed -= 10;
    }
  
  /* Temporary see invisible */
  if (p_ptr->tim_invis)
    {
      p_ptr->see_inv = TRUE;
    }
  
  /* Temporary infravision boost.   More useful now. */
  if (p_ptr->tim_infra)
    {
      int bonus = ((enhance == TRUE) ? 5 : 3);
      
      p_ptr->see_infra = p_ptr->see_infra + bonus;
    }
  
  
  /*** Special flags ***/
  
  /* Hack -- Hero/Shero -> Res fear */
  if (p_ptr->hero || p_ptr->shero)
    {
      p_ptr->no_fear = TRUE;
    }

  /* Clear contradictory flags */
  if (p_ptr->no_fear) p_ptr->fear = FALSE;
  if (p_ptr->fast_digest && p_ptr->slow_digest)
    {
      p_ptr->fast_digest = FALSE;
      p_ptr->slow_digest = FALSE;
    }

  if (p_resist_strong(P_RES_POIS))
    {
      p_ptr->rand_pois = FALSE;
      p_ptr->rand_pois_bad = FALSE;
    }

  if (p_resist_good(P_RES_CHAOS))
    p_ptr->rand_hallu = FALSE;

  if (p_ptr->free_act) 
    p_ptr->rand_paral = FALSE;

  /* End normal temporary bonuses; do bounds check on resistance levels */
  resistance_limits();
  
  /*** Analyze weight ***/
  
  /* Extract the current weight (in tenth pounds) */
  j = p_ptr->total_weight;
  
  /* Extract the "weight limit" (in tenth pounds) */
  i = weight_limit();
  
  /* Apply "encumbrance" from weight - more in water, except Maiar, flyers */
  if (j > i/2)
    { 
      if ((cave_feat[p_ptr->py][p_ptr->px] == FEAT_WATER) && 
	  (!check_ability(SP_DIVINE)) && !(p_ptr->schange == SHAPE_BAT) && 
	  !(p_ptr->schange == SHAPE_WYRM))
	p_ptr->pspeed -= 3 * ((j - (i/2)) / (i / 10));
      else
	p_ptr->pspeed -= ((j - (i/2)) / (i / 10));
    }
      
  /* Bloating slows the player down (a little) */
  if (p_ptr->food >= PY_FOOD_MAX) p_ptr->pspeed -= 10;
  
  /* Searching slows the player down */
  if (p_ptr->searching) p_ptr->pspeed -= 10;
  
  /* Sanity check on extreme speeds */
  if (p_ptr->pspeed < 0) p_ptr->pspeed = 0;
  if (p_ptr->pspeed > 199) p_ptr->pspeed = 199;
  
  /*** Apply modifier bonuses ***/
  
  /* Actual Modifier Bonuses (Un-inflate stat bonuses) */
  p_ptr->to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
  p_ptr->to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
  p_ptr->to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
  
  /* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
  p_ptr->dis_to_a += ((int)(adj_dex_ta[p_ptr->stat_ind[A_DEX]]) - 128);
  p_ptr->dis_to_d += ((int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
  p_ptr->dis_to_h += ((int)(adj_dex_th[p_ptr->stat_ind[A_DEX]]) - 128);
  
  
  /*** Modify skills ***/
  
  /* Affect Skill -- stealth (bonus one) */
  p_ptr->skill_stl += 1;
  
  /* Affect Skill -- disarming (DEX and INT) */
  p_ptr->skill_dis += adj_dex_dis[p_ptr->stat_ind[A_DEX]];
  p_ptr->skill_dis += adj_int_dis[p_ptr->stat_ind[A_INT]];
  
  /* Affect Skill -- magic devices (INT) */
  p_ptr->skill_dev += adj_int_dev[p_ptr->stat_ind[A_INT]];
  
  /* Affect Skill -- saving throw (WIS) */
  p_ptr->skill_sav += adj_wis_sav[p_ptr->stat_ind[A_WIS]];
  
  /* Affect Skill -- digging (STR) */
  p_ptr->skill_dig += adj_str_dig[p_ptr->stat_ind[A_STR]];
  
  /* Affect Skill -- disarming (Level, by Class and Race) */
  p_ptr->skill_dis += (cp_ptr->cx_dis * p_ptr->lev / 50);
  p_ptr->skill_dis += (rp_ptr->rx_dis * p_ptr->lev / 50);
  
  /* Affect Skill -- magic devices (Level, by Class and Race) */
  p_ptr->skill_dev += (cp_ptr->cx_dev * p_ptr->lev / 50);
  p_ptr->skill_dev += (rp_ptr->rx_dev * p_ptr->lev / 50);
  
  /* Affect Skill -- saving throw (Level, by Class and Race) */
  p_ptr->skill_sav += (cp_ptr->cx_sav * p_ptr->lev / 50);
  p_ptr->skill_sav += (rp_ptr->rx_sav * p_ptr->lev / 50);
  
  /* Affect Skill -- stealth (Level, by Class and Race) */
  p_ptr->skill_stl += (cp_ptr->cx_stl * p_ptr->lev / 50);
  p_ptr->skill_stl += (rp_ptr->rx_stl * p_ptr->lev / 50);

  /* Affect Skill -- search ability (Level, by Class and Race) */
  p_ptr->skill_srh += (cp_ptr->cx_srh * p_ptr->lev / 50);
  p_ptr->skill_srh += (rp_ptr->rx_srh * p_ptr->lev / 50);
  
  /* Affect Skill -- search frequency (Level, by Class and Race) */
  p_ptr->skill_fos += (cp_ptr->cx_fos * p_ptr->lev / 50);
  p_ptr->skill_fos += (rp_ptr->rx_fos * p_ptr->lev / 50);
  
  /* Affect Skill -- combat (melee) (Level, by Class and Race) */
  p_ptr->skill_thn += (cp_ptr->cx_thn * p_ptr->lev / 50);
  p_ptr->skill_thn += (rp_ptr->rx_thn * p_ptr->lev / 50);
  
  /* Affect Skill -- combat (shooting) (Level, by Class and Race) */
  p_ptr->skill_thb += (cp_ptr->cx_thb * p_ptr->lev / 50);
  p_ptr->skill_thb += (rp_ptr->rx_thb * p_ptr->lev / 50);
  
  /* Affect Skill -- combat (throwing) (Level, by Class and Race) */
  p_ptr->skill_tht += (cp_ptr->cx_thb * p_ptr->lev / 50);
  p_ptr->skill_tht += (rp_ptr->rx_thb * p_ptr->lev / 50);
  
  /* Limit Skill -- digging from 1 up */
  if (p_ptr->skill_dig < 1) p_ptr->skill_dig = 1;
  
  /* Limit Skill -- stealth from 0 to 30 */
  if (p_ptr->skill_stl > 30) p_ptr->skill_stl = 30;
  if (p_ptr->skill_stl < 0) p_ptr->skill_stl = 0;

  /* No negative infravision */
  if (p_ptr->see_infra < 0) p_ptr->see_infra = 0;
  
  /*** Special Saving Throw boosts are calculated after other bonuses ***/
  
  /* Specialty magic resistance; gives great saving throws even above 100 */
  if (check_ability(SP_MAGIC_RESIST))
    {
      if (p_ptr->skill_sav <= 80) 
	p_ptr->skill_sav += (100 - p_ptr->skill_sav) / 2;
      else p_ptr->skill_sav += 10;		
    }
  
  /* Heightened magical defenses.   Halves the difference between saving 
   * throw and 100.  */
  if (p_ptr->magicdef)
    {
      if (p_ptr->skill_sav <= 100) 
	p_ptr->skill_sav += (100 - p_ptr->skill_sav) / 2;
    }

  /* Rune of Magical Defence */
  if (cave_feat[p_ptr->py][p_ptr->px] == FEAT_RUNE_MAGDEF)
    {
      if (p_ptr->skill_sav <= 100) 
	p_ptr->skill_sav += (100 - p_ptr->skill_sav) / 2;
    }

  
  /*** Analyze shapechanges - everything but statistics ***/
  shape_change_main();
  
  
  /* Obtain the "hold" value */
  hold = adj_str_hold[p_ptr->stat_ind[A_STR]];
  
  /*** Analyze current bow ***/
  
  /* Examine the "current bow" */
  o_ptr = &inventory[INVEN_BOW];
  
  /* Assume not heavy */
  p_ptr->heavy_shoot = FALSE;

  /* It is hard to carry a heavy bow */
  if (hold < o_ptr->weight / 10)
    {
      /* Hard to wield a heavy bow */
      p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
      p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);
      
      /* Heavy Bow */
      p_ptr->heavy_shoot = TRUE;
    }
  
  /* Analyze launcher */
  if (o_ptr->k_idx)
    {
      /* Get to shoot */
      p_ptr->num_fire = 10;
      
      /* Launcher multiplier is now simply their damage dice. */
      p_ptr->ammo_mult = o_ptr->dd;
      
      
      /* Analyze the launcher */
      switch (o_ptr->sval)
	{
	  /* Sling and ammo */
	case SV_SLING:
	  {
	    p_ptr->ammo_tval = TV_SHOT;
	    break;
	  }
	  
	  /* Short Bow and Arrow */
	case SV_SHORT_BOW:
	  {
	    p_ptr->ammo_tval = TV_ARROW;
	    break;
	  }
	  
	  /* Long Bow and Arrow */
	case SV_LONG_BOW:
	  {
	    p_ptr->ammo_tval = TV_ARROW;
	    break;
	  }
	  
	  /* Light Crossbow and Bolt */
	case SV_LIGHT_XBOW:
	  {
	    p_ptr->ammo_tval = TV_BOLT;
	    break;
	  }
	  
	  /* Heavy Crossbow and Bolt */
	case SV_HEAVY_XBOW:
	  {
	    p_ptr->ammo_tval = TV_BOLT;
	    break;
	  }
	}
      
      /* Apply special flags */
      if (o_ptr->k_idx && !p_ptr->heavy_shoot)
	{
	  /* Dex factor for shot speed */
	  int dex_factor = (adj_dex_shots[p_ptr->stat_ind[A_DEX]]);
	  
	  /* Extra shots */
	  p_ptr->num_fire += extra_shots * 10;
	  
	  /* Extra might */
	  p_ptr->ammo_mult += extra_might;
	  
	  /* Love your launcher SJGU bonuses reduced */
	  if (((check_ability(SP_BOW_SPEED_GREAT)) && 
	       (p_ptr->ammo_tval == TV_ARROW)) ||
	      ((check_ability(SP_SLING_SPEED_GREAT)) && 
	       (p_ptr->ammo_tval == TV_SHOT)) ||
	      ((check_ability(SP_XBOW_SPEED_GREAT)) && 
	       (p_ptr->ammo_tval == TV_BOLT)))
	    {
	      /* Big bonus... */
	      p_ptr->num_fire += 3 * dex_factor / 4;

	      /* ...and sometimes even more */
	      if (check_ability(SP_RAPID_FIRE))
		p_ptr->num_fire += dex_factor / 4;
	    }
	  
	  /* Like your launcher */
	  else if (((check_ability(SP_BOW_SPEED_GOOD)) && 
		    (p_ptr->ammo_tval == TV_ARROW)) ||
		   ((check_ability(SP_SLING_SPEED_GOOD)) && 
		    (p_ptr->ammo_tval == TV_SHOT)) ||
		   ((check_ability(SP_XBOW_SPEED_GOOD)) && 
		    (p_ptr->ammo_tval == TV_BOLT)))
	    {
	      /* Medium bonus */
	      p_ptr->num_fire += dex_factor / 2;
	    }
	  
	  /* Minimal bonus */
	  else
	    {
	      /* Small bonus */
	      p_ptr->num_fire += dex_factor / 4;
	    }
	  
	  
	  /* See formula "do_cmd_fire" in "attack.c" for Assassin bonus
	   * to Deadliness. */
	}
      
      /* Require at least one shot per round in real terms */
      if (p_ptr->num_fire < 10) p_ptr->num_fire = 10;
    }
  
  /* Add all class and race-specific adjustments to missile Skill. */
  p_ptr->skill_thb += add_special_missile_skill (p_ptr->pclass, o_ptr->weight, 
						 o_ptr);
  
  
  /*** Analyze weapon ***/
  
  /* Examine the "current weapon" */
  o_ptr = &inventory[INVEN_WIELD];
  
  /* Assume that the player is not a Priest wielding an edged weapon. */
  p_ptr->icky_wield = FALSE;
  
  /* Assume the weapon is not too heavy */
  p_ptr->heavy_wield = FALSE;
  
  /* Inflict heavy weapon penalties. */
  if (hold < o_ptr->weight / 10)
    {
      /* Hard to wield a heavy weapon */
      p_ptr->to_h += 2 * (hold - o_ptr->weight / 10);
      p_ptr->dis_to_h += 2 * (hold - o_ptr->weight / 10);
      
      /* Heavy weapon */
      p_ptr->heavy_wield = TRUE;
      
      /* The player gets to swing a heavy weapon only once. */
      p_ptr->num_blow = 1;
    }
  
  /* Normal weapons */
  if (o_ptr->k_idx && !p_ptr->heavy_wield)
    {
      int str_index, dex_index;
      
      int effective_weight = 0, mul = 6;
      
      /* Enforce a minimum weight of three pounds. */
      effective_weight = (o_ptr->weight < 30 ? 30 : o_ptr->weight);
      
      
      /* Compare strength and weapon weight. */
      str_index = mul * adj_str_blow[p_ptr->stat_ind[A_STR]] / 
	effective_weight;
      
      /* Maximal value */
      if (str_index > 11) str_index = 11;
      
      
      /* Index by dexterity */
      dex_index = (adj_dex_blow[p_ptr->stat_ind[A_DEX]]);
      
      /* Maximal value */
      if (dex_index > 11) dex_index = 11;
      
      
      /* Use the blows table */
      p_ptr->num_blow = blows_table[str_index][dex_index];
      
      /* Paranoia - require at least one blow */
      if (p_ptr->num_blow < 1) p_ptr->num_blow = 1;
      
      
      /* Boost digging skill by weapon weight */
      p_ptr->skill_dig += (o_ptr->weight / 10);
    }
  
  /* Everyone gets 2 to 4 blows if not wielding a weapon. */
  else if (!o_ptr->k_idx) p_ptr->num_blow = 2 + (p_ptr->lev / 20);
  
  
  /* Add all other class and race-specific adjustments to melee Skill. */
  p_ptr->skill_thn += add_special_melee_skill(p_ptr->pclass, 
					      o_ptr->weight, o_ptr);
  
  
  /*** Notice changes ***/
  
  /* Analyze stats */
  for (i = 0; i < A_MAX; i++)
    {
      /* Notice changes */
      if (p_ptr->stat_top[i] != old_stat_top[i])
	{
	  /* Redisplay the stats later */
	  p_ptr->redraw |= (PR_STATS);
	  
	  /* Window stuff */
	  p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	}
      
      /* Notice changes */
      if (p_ptr->stat_use[i] != old_stat_use[i])
	{
	  /* Redisplay the stats later */
	  p_ptr->redraw |= (PR_STATS);
	  
	  /* Window stuff */
	  p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	}
      
      /* Notice changes */
      if (p_ptr->stat_ind[i] != old_stat_ind[i])
	{
	  /* Change in STR may affect how shields are used. */
	  if ((i == A_STR) && (inventory[INVEN_ARM].k_idx))
	    {
	      /* Access the wield slot */
	      o_ptr = &inventory[INVEN_WIELD];

	      /* Analyze weapon for two-handed-use. */
	      if (o_ptr->flags_obj & (OF_TWO_HANDED_REQ) || 
		  (o_ptr->flags_obj & (OF_TWO_HANDED_DES) && 
		   (p_ptr->stat_ind[A_STR] < 
		    29 + (o_ptr->weight / 50 > 8 ? 
			  8 : o_ptr->weight / 50))))
		{
		  p_ptr->shield_on_back = TRUE;
		  
		}
	      else p_ptr->shield_on_back = FALSE;
	      
	      /* Hack - recalculate bonuses again. */
	      if (p_ptr->old_shield_on_back != 
		  p_ptr->shield_on_back)
		{
		  /* do not check strength again */
		  old_stat_ind[i] = p_ptr->stat_ind[i];
		  goto calc_again;
		}
	    }
	  
	  /* Change in CON affects Hitpoints */
	  if (i == A_CON)
	    {
	      p_ptr->update |= (PU_HP);
	    }
	  
	  /* Change in INT may affect Mana/Spells */
	  else if (i == A_INT)
	    {
	      if (mp_ptr->spell_stat == A_INT)
		{
		  p_ptr->update |= (PU_MANA | PU_SPELLS);
		}
	    }
	  
	  /* Change in WIS may affect Mana/Spells */
	  else if (i == A_WIS)
	    {
	      if (mp_ptr->spell_stat == A_WIS)
		{
		  p_ptr->update |= (PU_MANA | PU_SPELLS);
		}
	    }
	}
    }
  
  /* Hack -- Telepathy Change */
  if (p_ptr->telepathy != old_telepathy)
    {
      /* Update monster visibility */
      p_ptr->update |= (PU_MONSTERS);
    }
  
  /* Hack -- See Invis Change */
  if (p_ptr->see_inv != old_see_inv)
    {
      /* Update monster visibility */
      p_ptr->update |= (PU_MONSTERS);
    }
  
  /* Hack -- See Invis Change */
  if (p_ptr->see_infra != old_see_infra)
    {
      /* Update monster visibility */
      p_ptr->update |= (PU_MONSTERS);
    }
  
  /* Redraw speed (if needed) */
  if (p_ptr->pspeed != old_speed)
    {
      /* Redraw speed */
      p_ptr->redraw |= (PR_SPEED);
    }
  
  /* Recalculate stealth when needed */
  if (p_ptr->skill_stl != old_stealth)
    {
      /* Assume character is extremely noisy. */
      p_ptr->base_wakeup_chance = 100 * WAKEUP_ADJ;
      
      /* For every increase in stealth past 0, multiply wakeup chance by 0.8 */
      for (i = 0; i < p_ptr->skill_stl; i++)
	{
	  p_ptr->base_wakeup_chance = 4 * p_ptr->base_wakeup_chance / 5;
	  
	  /* Always make at least some innate noise */
	  if (p_ptr->base_wakeup_chance < 100)
	    {
	      p_ptr->base_wakeup_chance = 100;
	      break;
	    }
	}
    }
  
  /* Big Hack - make sure base_wakeup_chance has ever been set */
  if (p_ptr->base_wakeup_chance == 0) 
    p_ptr->base_wakeup_chance = 100 * WAKEUP_ADJ;
  
  /* Redraw armor (if needed) */
  if ((p_ptr->dis_ac != old_dis_ac) || (p_ptr->dis_to_a != old_dis_to_a))
    {
      /* Redraw */
      p_ptr->redraw |= (PR_ARMOR);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
    }
  
  /* Hack -- handle "xtra" mode */
  if (character_xtra || inspect) return;
  
  /* Take note when player moves his shield on and off his back. */
  if (p_ptr->evasion_chance != p_ptr->old_evasion_chance)
    {
      /* Messages */
      if (!p_ptr->old_evasion_chance)
	{
	  msg_print("You are able to Evade attacks.");
	}
      else if (!p_ptr->evasion_chance)
	{
	  msg_print("You are no longer able to Evade attacks");
	}
      /* Mega-Hack - Mask out small changes */
      else if (p_ptr->evasion_chance > (p_ptr->old_evasion_chance + 5))
	{
	  msg_print("You are better able to Evade attacks.");
	}
      else if ((p_ptr->evasion_chance + 5) < p_ptr->old_evasion_chance)
	{
	  msg_print("You are less able to Evade attacks.");
	}
      
      /* Save it */
      p_ptr->old_evasion_chance = p_ptr->evasion_chance;
    }
  
  /* Take note when "heavy bow" changes */
  if (p_ptr->old_heavy_shoot != p_ptr->heavy_shoot)
    {
      /* Message */
      if (p_ptr->heavy_shoot)
	{
	  msg_print("You have trouble wielding such a heavy bow.");
	}
      else if (inventory[INVEN_BOW].k_idx)
	{
	  msg_print("You have no trouble wielding your bow.");
	}
      else
	{
	  msg_print("You feel relieved to put down your heavy bow.");
	}
      
      /* Save it */
      p_ptr->old_heavy_shoot = p_ptr->heavy_shoot;
    }
  
  /* Take note when "heavy weapon" changes */
  if (p_ptr->old_heavy_wield != p_ptr->heavy_wield)
    {
      /* Message */
      if (p_ptr->heavy_wield)
	{
	  msg_print("You have trouble wielding such a heavy weapon.");
	}
      else if (inventory[INVEN_WIELD].k_idx)
	{
	  msg_print("You have no trouble wielding your weapon.");
	}
      else
	{
	  msg_print("You feel relieved to put down your heavy weapon.");
	}
      
      /* Save it */
      p_ptr->old_heavy_wield = p_ptr->heavy_wield;
    }
  
  /* Take note when "illegal weapon" changes */
  if (p_ptr->old_icky_wield != p_ptr->icky_wield)
    {
      /* Message */
      if (p_ptr->icky_wield)
	{
	  msg_print("You do not feel comfortable with your weapon.");
	}
      else if (inventory[INVEN_WIELD].k_idx)
	{
	  notice_obj(OF_BLESSED, INVEN_WIELD + 1);
	  msg_print("You feel comfortable with your weapon.");
	}
      else
	{
	  msg_print("You feel more comfortable after removing your weapon.");
	}
      
      /* Save it */
      p_ptr->old_icky_wield = p_ptr->icky_wield;
    }
  
  /* Take note when player moves his shield on and off his back. */
  if (p_ptr->old_shield_on_back != p_ptr->shield_on_back)
    {
      /* Messages */
      if (p_ptr->shield_on_back)
	{
	  msg_print("You are carrying your shield on your back.");
	}
      else if (inventory[INVEN_ARM].k_idx)
	{
	  msg_print("You are carrying your shield in your hand.");
	}
      
      /* No message for players no longer carrying a shield. */
      
      /* Save it */
      p_ptr->old_shield_on_back = p_ptr->shield_on_back;
    }

  /* Hack - force redraw if stuff has changed */
  if (p_ptr->redraw) p_ptr->update |= (PU_BONUS);
}



/**
 * Handle "p_ptr->notice"
 */
void notice_stuff(void)
{
  /* Notice stuff */
  if (!p_ptr->notice) return;
  
  
  /* Deal with autoinscribe stuff */
  if (p_ptr->notice & PN_AUTOINSCRIBE)
    {
      p_ptr->notice &= ~(PN_AUTOINSCRIBE);
      autoinscribe_pack();
      autoinscribe_ground();
    }
  
  /* Deal with squelch stuff */
  if (p_ptr->notice & PN_SQUELCH)
    {
      p_ptr->notice &= ~(PN_SQUELCH);
      if (hide_squelchable) squelch_drop();
    }

  /* Combine the pack */
  if (p_ptr->notice & (PN_COMBINE))
    {
      p_ptr->notice &= ~(PN_COMBINE);
      combine_pack();
      (void) process_quiver(0, NULL);
    }
  
  /* Reorder the pack */
  if (p_ptr->notice & (PN_REORDER))
    {
      p_ptr->notice &= ~(PN_REORDER);
      reorder_pack();
    }
}


/**
 * Handle "p_ptr->update"
 */
void update_stuff(void)
{
  /* Update stuff */
  if (!p_ptr->update) return;
  
  
  if (p_ptr->update & (PU_BONUS))
    {
      p_ptr->update &= ~(PU_BONUS);
      calc_bonuses(FALSE);
    }
  
  if (p_ptr->update & (PU_TORCH))
    {
      p_ptr->update &= ~(PU_TORCH);
      calc_torch();
    }
  
  if (p_ptr->update & (PU_HP))
    {
      p_ptr->update &= ~(PU_HP);
      calc_hitpoints();
    }
  
  if (p_ptr->update & (PU_MANA))
    {
      p_ptr->update &= ~(PU_MANA);
      calc_mana();
    }
  
  if (p_ptr->update & (PU_SPELLS))
    {
      p_ptr->update &= ~(PU_SPELLS);
      calc_spells();
    }
  
  if (p_ptr->update & (PU_SPECIALTY))
    {
      p_ptr->update &= ~(PU_SPECIALTY);
      calc_specialty();
    }
  
  
  /* Character is not ready yet, no screen updates */
  if (!character_generated) return;
  
  
  /* Character is in "icky" mode, no screen updates */
  if (character_icky) return;
  
  
  if (p_ptr->update & (PU_FORGET_VIEW))
    {
      p_ptr->update &= ~(PU_FORGET_VIEW);
      forget_view();
    }
  
  if (p_ptr->update & (PU_UPDATE_VIEW))
    {
      p_ptr->update &= ~(PU_UPDATE_VIEW);
      update_view();
    }
  
  if (p_ptr->update & (PU_DISTANCE))
    {
      p_ptr->update &= ~(PU_DISTANCE);
      p_ptr->update &= ~(PU_MONSTERS);
      update_monsters(TRUE);
    }
  
  if (p_ptr->update & (PU_MONSTERS))
    {
      p_ptr->update &= ~(PU_MONSTERS);
      update_monsters(FALSE);
    }
  
  
  if (p_ptr->update & (PU_PANEL))
    {
      p_ptr->update &= ~(PU_PANEL);
      verify_panel();
    }
}


/**
 * Handle "p_ptr->redraw"
 */
void redraw_stuff(void)
{
  /* Redraw stuff */
  if (!p_ptr->redraw) return;
  
  
  /* Character is not ready yet, no screen updates */
  if (!character_generated) return;
  
  
  /* Character is in "icky" mode, no screen updates */
  if (character_icky) return;
  
  /* Complete wipe - use with caution! -NRM- */
  if (p_ptr->redraw & (PR_WIPE))
    {
      p_ptr->redraw &= ~(PR_WIPE);
      clear_from(1);
    }
  
  if (p_ptr->redraw & (PR_MAP))
    {
      p_ptr->redraw &= ~(PR_MAP);
      prt_map();
    }
  
  
  if (p_ptr->redraw & (PR_BASIC))
    {
      p_ptr->redraw &= ~(PR_BASIC);
      p_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
      p_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
      p_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA);
      p_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH | PR_MON_MANA);
      prt_frame_basic();
    }
  
  if (p_ptr->redraw & (PR_MISC))
    {
      p_ptr->redraw &= ~(PR_MISC);
      prt_field(p_name + rp_ptr->name, ROW_RACE, COL_RACE);
      prt_field(c_name + cp_ptr->name, ROW_CLASS, COL_CLASS);
    }
  
  if (p_ptr->redraw & (PR_TITLE))
    {
      p_ptr->redraw &= ~(PR_TITLE);
      prt_title();
    }
  
  if (p_ptr->redraw & (PR_LEV))
    {
      p_ptr->redraw &= ~(PR_LEV);
      prt_level();
    }
  
  if (p_ptr->redraw & (PR_EXP))
    {
      p_ptr->redraw &= ~(PR_EXP);
      prt_exp();
    }
  
  if (p_ptr->redraw & (PR_STATS))
    {
      p_ptr->redraw &= ~(PR_STATS);
      prt_stat(A_STR);
      prt_stat(A_INT);
      prt_stat(A_WIS);
      prt_stat(A_DEX);
      prt_stat(A_CON);
      prt_stat(A_CHR);
    }
  
  if (p_ptr->redraw & (PR_ARMOR))
    {
      p_ptr->redraw &= ~(PR_ARMOR);
      prt_ac();
    }
  
  if (p_ptr->redraw & (PR_HP))
    {
      p_ptr->redraw &= ~(PR_HP);
      prt_hp();
      
      /*
       * hack:  redraw player, since the player's color
       * now indicates approximate health.  Note that
       * using this command when graphics mode is on
       * causes the character to be a black square.
       */
      if ((hp_changes_colour) && (arg_graphics == GRAPHICS_NONE))
	{
	  lite_spot(p_ptr->py, p_ptr->px);
	}
      
    }
  
  if (p_ptr->redraw & (PR_MANA))
    {
      p_ptr->redraw &= ~(PR_MANA);
      prt_sp();
    }
  
  if (p_ptr->redraw & (PR_GOLD))
    {
      p_ptr->redraw &= ~(PR_GOLD);
      prt_gold();
    }
  
  if (p_ptr->redraw & (PR_SHAPE))
    {
      p_ptr->redraw &= ~(PR_SHAPE);
      prt_shape();
    }
  
  if (p_ptr->redraw & (PR_DEPTH))
    {
      p_ptr->redraw &= ~(PR_DEPTH);
      prt_depth();
    }
  
  if (p_ptr->redraw & (PR_HEALTH))
    {
      p_ptr->redraw &= ~(PR_HEALTH);
      health_redraw();
    }
  
  if (p_ptr->redraw & (PR_MON_MANA))
    {
      p_ptr->redraw &= ~(PR_MON_MANA);
      mana_redraw();
    }
  
  if (p_ptr->redraw & (PR_EXTRA))
    {
      p_ptr->redraw &= ~(PR_EXTRA);
      p_ptr->redraw &= ~(PR_CUT | PR_STUN);
      p_ptr->redraw &= ~(PR_HUNGER);
      p_ptr->redraw &= ~(PR_BLIND | PR_CONFUSED);
      p_ptr->redraw &= ~(PR_AFRAID | PR_POISONED);
      p_ptr->redraw &= ~(PR_STATE | PR_SPEED | PR_STUDY);
      p_ptr->redraw &= ~(PR_STATUS);
      p_ptr->redraw &= ~(PR_DTRAP);
      prt_frame_extra();
    }
  
  if (p_ptr->redraw & (PR_CUT))
    {
      p_ptr->redraw &= ~(PR_CUT);
      prt_cut();
    }
  
  if (p_ptr->redraw & (PR_STUN))
    {
      p_ptr->redraw &= ~(PR_STUN);
      prt_stun();
    }
  
  if (p_ptr->redraw & (PR_HUNGER))
    {
      p_ptr->redraw &= ~(PR_HUNGER);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_BLIND))
    {
      p_ptr->redraw &= ~(PR_BLIND);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_CONFUSED))
    {
      p_ptr->redraw &= ~(PR_CONFUSED);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_AFRAID))
    {
      p_ptr->redraw &= ~(PR_AFRAID);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_POISONED))
    {
      p_ptr->redraw &= ~(PR_POISONED);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_STATE))
    {
      p_ptr->redraw &= ~(PR_STATE);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_SPEED))
    {
      p_ptr->redraw &= ~(PR_SPEED);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_DTRAP))
    {
      p_ptr->redraw &= ~(PR_DTRAP);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_STUDY))
    {
      p_ptr->redraw &= ~(PR_STUDY);
      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_BUTTONS))
    {
      p_ptr->redraw &= ~(PR_BUTTONS);

      /* temp hack */
      update_statusline();
    }
  
  if (p_ptr->redraw & (PR_STATUS))
    {
      p_ptr->redraw &= ~(PR_STATUS);
      prt_status();
    }
}


/**
 * Handle "p_ptr->window"
 */
void window_stuff(void)
{
  int j;
  
  u32b mask = 0L;
  
  
  /* Nothing to do */
  if (!p_ptr->window) return;
  
  /* Scan windows */
  for (j = 0; j < TERM_WIN_MAX; j++)
    {
      /* Save usable flags */
      if (angband_term[j])
	{
	  /* Build the mask */
	  mask |= op_ptr->window_flag[j];
	}
    }
  
  /* Apply usable flags */
  p_ptr->window &= (mask);
  
  /* Nothing to do */
  if (!p_ptr->window) return;
  
  
  /* Display inventory */
  if (p_ptr->window & (PW_INVEN))
    {
      p_ptr->window &= ~(PW_INVEN);
      fix_inven();
    }
  
  /* Display monster list */
  if (p_ptr->window & (PW_MONLIST))
    {
      p_ptr->window &= ~(PW_MONLIST);
      fix_monlist();
    }
  
  /* Display monster list */
  if (p_ptr->window & (PW_ITEMLIST))
    {
      p_ptr->window &= ~(PW_ITEMLIST);
      fix_itemlist();
    }
  
  /* Display equipment */
  if (p_ptr->window & (PW_EQUIP))
    {
      p_ptr->window &= ~(PW_EQUIP);
      fix_equip();
    }
  
  /* Display player (mode 0) */
  if (p_ptr->window & (PW_PLAYER_0))
    {
      p_ptr->window &= ~(PW_PLAYER_0);
      fix_player_0();
    }
  
  /* Display player (mode 1) */
  if (p_ptr->window & (PW_PLAYER_1))
    {
      p_ptr->window &= ~(PW_PLAYER_1);
      fix_player_1();
    }
  
  /* Display message */
  if (p_ptr->window & (PW_MESSAGE))
    {
      p_ptr->window &= ~(PW_MESSAGE);
      fix_message();
    }
  
  /* Display overhead view */
  if (p_ptr->window & (PW_OVERHEAD))
    {
      p_ptr->window &= ~(PW_OVERHEAD);
      fix_overhead();
    }
  
  /* Display monster recall */
  if (p_ptr->window & (PW_MONSTER))
    {
      p_ptr->window &= ~(PW_MONSTER);
      fix_monster();
    }
  
  /* Display object recall */
  if (p_ptr->window & (PW_OBJECT))
    {
      p_ptr->window &= ~(PW_OBJECT);
      fix_object();
    }
}


/**
 * Handle "p_ptr->update" and "p_ptr->redraw" and "p_ptr->window"
 */
void handle_stuff(void)
{
  /* Update stuff */
  if (p_ptr->update) update_stuff();
  
  /* Redraw stuff */
  if (p_ptr->redraw) redraw_stuff();
  
  /* Window stuff */
  if (p_ptr->window) window_stuff();
}


