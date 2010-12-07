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


#ifdef CHECK_TIME

/**
 * Operating hours for ANGBAND (defaults to non-work hours)
 */
static char days[7][29] =
{
  "SUN:XXXXXXXXXXXXXXXXXXXXXXXX",
  "MON:XXXXXXXX.........XXXXXXX",
  "TUE:XXXXXXXX.........XXXXXXX",
  "WED:XXXXXXXX.........XXXXXXX",
  "THU:XXXXXXXX.........XXXXXXX",
  "FRI:XXXXXXXX.........XXXXXXX",
  "SAT:XXXXXXXXXXXXXXXXXXXXXXXX"
};

/**
 * Restict usage (defaults to no restrictions)
 */
static bool check_time_flag = FALSE;

#endif


/**
 * Handle CHECK_TIME
 */
errr check_time(void)
{

#ifdef CHECK_TIME

  time_t c;
  struct tm *tp;
  
  /* No restrictions */
  if (!check_time_flag) return (0);
  
  /* Check for time violation */
  c = time((time_t *)0);
  tp = localtime(&c);
  
  /* Violation */
  if (days[tp->tm_wday][tp->tm_hour + 4] != 'X') return (1);
  
#endif
  
  /* Success */
  return (0);
}



/**
 * Initialize CHECK_TIME
 */
errr check_time_init(void)
{

#ifdef CHECK_TIME

  FILE *fp;
  
  char buf[1024];
  
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_FILE, "time.txt");
  
  /* Open the file */
  fp = my_fopen(buf, "r");
  
  /* No file, no restrictions */
  if (!fp) return (0);
  
  /* Assume restrictions */
  check_time_flag = TRUE;
  
  /* Parse the file */
  while (0 == my_fgets(fp, buf, 80))
    {
      /* Skip comments and blank lines */
      if (!buf[0] || (buf[0] == '#')) continue;
      
      /* Chop the buffer */
      buf[29] = '\0';
      
      /* Extract the info */
      if (prefix(buf, "SUN:")) strcpy(days[0], buf);
      if (prefix(buf, "MON:")) strcpy(days[1], buf);
      if (prefix(buf, "TUE:")) strcpy(days[2], buf);
      if (prefix(buf, "WED:")) strcpy(days[3], buf);
      if (prefix(buf, "THU:")) strcpy(days[4], buf);
      if (prefix(buf, "FRI:")) strcpy(days[5], buf);
      if (prefix(buf, "SAT:")) strcpy(days[6], buf);
    }
  
  /* Close it */
  my_fclose(fp);
  
#endif
  
  /* Success */
  return (0);
}


/**
 * Hack -- pass color info around this file
 */
static byte likert_color = TERM_WHITE;


/**
 * Returns a "rating" of x depending on y
 */
static cptr likert(int x, int y)
{
  char description[10];

  /* Paranoia */
  if (y <= 0) y = 1;
  
  /* Negative value */
  if (x < 0)
    {
      likert_color = TERM_L_DARK;
      return ("Awful");
    }
  
  /* Analyze the value */
  switch ((x / y))
    {
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
  return (format("%s(%d)", description ,x));
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

void player_flags(u32b *flags)
{
  /* Clear */
  (*flags)  = 0L;
  
  /* Add racial flags */
  (*flags) |= rp_ptr->flags_obj;
  
  /* Warrior. */
  if (check_ability(SP_RELENTLESS))
    {
      if (p_ptr->lev >= 30) (*flags) |= (OF_FEARLESS);
      if (p_ptr->lev >= 40) (*flags) |= (OF_REGEN);
    }
  
  /* Shapechange, if any. */
  switch (p_ptr->schange)
    {
    case SHAPE_BEAR:
    case SHAPE_NORMAL:
    case SHAPE_MOUSE:
      {
	break;
      }
    case SHAPE_FERRET:
      {
	(*flags) |= (OF_REGEN);
	    break;
      }
    case SHAPE_HOUND:
      {
	(*flags) |= (OF_TELEPATHY);
	break;
      }
    case SHAPE_LION:
      {
	(*flags) |= (OF_FEARLESS);
	(*flags) |= (OF_REGEN);
	break;
      }
    case SHAPE_ENT:
      {
	(*flags) |= (OF_FEARLESS);
	(*flags) |= (OF_SEE_INVIS);
	(*flags) |= (OF_FREE_ACT);
	break;
      }
    case SHAPE_BAT:
      {
	(*flags) |= (OF_SEEING);
	(*flags) |= (OF_FEATHER);
	break;
      }
    case SHAPE_WEREWOLF:
      {
	(*flags) |= (OF_REGEN);
	break;	
      }
    case SHAPE_VAMPIRE:
      {
	(*flags) |= (OF_SEE_INVIS);
	(*flags) |= (OF_HOLD_LIFE);
	(*flags) |= (OF_REGEN);
	break;
      }
    case SHAPE_WYRM:
      {
	object_type *o_ptr = &inventory[INVEN_BODY];
	
	/* Paranoia */
	if (o_ptr->tval != TV_DRAG_ARMOR) break;
	
	/* Add 'extra' power if any */
	switch (o_ptr->sval)
	  {
	    
	  case (SV_DRAGON_GREEN):
	    {
	      (*flags) |= (OF_REGEN);
	      break;
	    }
	  case (SV_DRAGON_SHINING):
	    {
	      (*flags) |= (OF_SEE_INVIS);
	      break;
	    }
	  case (SV_DRAGON_LAW):
	  case (SV_DRAGON_CHAOS):
	    {
	      (*flags) |= (OF_HOLD_LIFE);
	      break;
	    }
	  case (SV_DRAGON_BRONZE):
	  case (SV_DRAGON_GOLD):
	    {
	      (*flags) |= (OF_FREE_ACT);
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
void player_weakness_dis(u32b *flags)
{
  /* Clear */
  (*flags) = 0L;
  
  /* HACK - add weakness of some races */
  if (check_ability(SP_WOODEN)) 
    {	
      (*flags) |= (OF_FEATHER);
    }
  
  /* Shapechange, if any. */
  switch (p_ptr->schange)
    {
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
	(*flags) |= (OF_FEATHER);
	break;
      }
    case SHAPE_VAMPIRE:
      {
	(*flags) |= (OF_LITE);
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
  switch (p_ptr->schange)
    {
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
	if (!check_ability(SP_WOODEN))
	  apply_resist(&player_resists[P_RES_FIRE], RES_CUT_MINOR);
	break;
      }
    case SHAPE_VAMPIRE:
      {
	apply_resist(&player_resists[P_RES_COLD], RES_BOOST_NORMAL);
	apply_resist(&player_resists[P_RES_LITE], RES_CUT_MINOR);
	break;
      }
    case SHAPE_WYRM:
      {
	object_type *o_ptr = &inventory[INVEN_BODY];
	
	/* Paranoia */
	if (o_ptr->tval != TV_DRAG_ARMOR) break;
	    
	/* Add 'extra' power if any */
	switch (o_ptr->sval)
	  {
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
  switch (p_ptr->schange)
    {
    case SHAPE_BEAR:
    case SHAPE_NORMAL:
      {
	break;
      }
    case SHAPE_MOUSE:
      {
	player_bonus[P_BONUS_STEALTH] += (30 + p_ptr->skill_stl) / 2;
	player_bonus[P_BONUS_INFRA] += 2;
	player_bonus[P_BONUS_M_MASTERY] -= 3 * p_ptr->skill_dev / 40; 
	break;
      }
    case SHAPE_FERRET:
      {
	player_bonus[P_BONUS_INFRA] += 2;
	player_bonus[P_BONUS_SPEED] += 2;
	player_bonus[P_BONUS_SEARCH] += 10;
	player_bonus[P_BONUS_M_MASTERY] -= p_ptr->skill_dev / 20; 
	break;
      }
    case SHAPE_HOUND:
      {
	player_bonus[P_BONUS_M_MASTERY] -= p_ptr->skill_dev / 20; 
	player_bonus[P_BONUS_INFRA] += 3;
	break;
      }
    case SHAPE_GAZELLE:
      {
	player_bonus[P_BONUS_M_MASTERY] -= p_ptr->skill_dev / 20; 
	player_bonus[P_BONUS_SPEED] += 6;
	break;
      }
    case SHAPE_LION:
      {
	player_bonus[P_BONUS_M_MASTERY] -= p_ptr->skill_dev / 20; 
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
	player_bonus[P_BONUS_M_MASTERY] -= 3 * p_ptr->skill_dev / 40; 
	break;
      }
    case SHAPE_WEREWOLF:
      {
	player_bonus[P_BONUS_M_MASTERY] -= p_ptr->skill_dev / 20; 
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
static u32b display_player_powers[10] =
{
  
  OF_SLOW_DIGEST,
  OF_FEATHER,
  OF_LITE,
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
static cptr display_player_resist_names[2][7] =
{
  {
    "Acid:",	/* P_RES_ACID */
    "Elec:",	/* P_RES_ELEC */
    "Fire:",	/* P_RES_FIRE */
    "Cold:",	/* P_RES_COLD */
    "Pois:",	/* P_RES_POIS */
    "Lite:",	/* P_RES_LITE */
    "Dark:"	/* P_RES_DARK */
  },
  
  {
    "Confu:",	/* P_RES_CONFU */
    "Sound:",	/* P_RES_SOUND */
    "Shard:",	/* P_RES_SHARD */
    "Nexus:",	/* P_RES_NEXUS */
    "Nethr:",	/* P_RES_NETHR */
    "Chaos:",	/* P_RES_CHAOS */
    "Disen:"	/* P_RES_DISEN */
  }
};


static cptr display_player_power_names[10] =
  {
    "S.Dig:",	/* OF_SLOW_DIGEST */
    "Feath:",	/* OF_FEATHER */
    "PLite:",	/* OF_LITE */
    "Regen:",	/* OF_REGEN */
    "Telep:",	/* OF_TELEPATHY */
    "Invis:",	/* OF_SEE_INVIS */
    "FrAct:",	/* OF_FREE_ACT */
    "HLife:",	/* OF_HOLD_LIFE */
    "Blind:",   /* OF_SEEING */
    "NFear:"    /* OF_FEARLESS */
  };

static cptr display_player_bonus_names[8] =
  {
    "M-Mas:",	/* P_BONUS_M_MASTERY */
    "Stea.:",	/* P_BONUS_STEALTH */
    "Sear.:",	/* P_BONUS_SEARCH */
    "Infra:",	/* P_BONUS_INFRA */
    "Tunn.:",	/* P_BONUS_TUNNEL */
    "Speed:",	/* P_BONUS_SPEED */
    "Shots:",	/* P_BONUS_SHOTS */
    "Might:",	/* P_BONUS_MIGHT */
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
  int last_line = 0;

  /* Erase screen */
  clear_from(0);

  /* Make the array of lines */
  last_line = make_dump(((mode == 0) ? pline0 : pline1), mode);

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
static void prt_lnum(cptr header, s32b num, int row, int col, byte color)
{
  int len = strlen(header);
  char out_val[32];
  put_str(header, row, col);
  sprintf(out_val, "%9ld", (long)num);
  c_put_str(color, out_val, row, col + len);
}

/**
 * Print number with header at given row, column
 */
static void prt_num(cptr header, int num, int row, int col, byte color)
{
  int len = strlen(header);
  char out_val[32];
  put_str(header, row, col);
  put_str("   ", row, col + len);
  sprintf(out_val, "%6ld", (long)num);
  c_put_str(color, out_val, row, col + len);
}

/**
 * Print decimal number with header at given row, column
 */
static void prt_deci(cptr header, int num, int deci, int row, int col, 
		     byte color)
{
  int len = strlen(header);
  char out_val[32];
  put_str(header, row, col);
  put_str("   ", row, col + len);
  sprintf(out_val, "%8ld", (long)deci);
  c_put_str(color, out_val, row, col + len);
  sprintf(out_val, "%6ld", (long)num);
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
  char c;
  
  char buf[100];
  
  int show_m_tohit = p_ptr->dis_to_h;
  int show_a_tohit = p_ptr->dis_to_h;
  int show_m_todam = p_ptr->dis_to_d;
  int show_a_todam = p_ptr->dis_to_d;
  
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
  
  c_put_str(TERM_L_BLUE, p_name + rp_ptr->name, 1, 37);
  c_put_str(TERM_L_BLUE, c_name + cp_ptr->name, 2, 37);
  
  /* Header and Footer */
  put_str("abcdefghijkl", 3, 25);
  
  /* Display the stats */
  for (i = 0; i < A_MAX; i++)
    {
      /* Assume uppercase stat name */
      put_str(stat_names[i], 4 + i, 0);
      
      /* Indicate natural maximum */
      if (p_ptr->stat_max[i] == 18+100)
	{
	  put_str("!", 4 + i, 3);
	}
      
      /* Obtain the current stat (modified) */
      cnv_stat(p_ptr->stat_use[i], buf);
      
      /* Display the current stat (modified) */
      c_put_str(TERM_L_GREEN, buf, 4 + i, 8);
    }

  /* Process equipment */
  for (i = INVEN_WIELD; i < INVEN_SUBTOTAL; i++)
    {
      /* Access object */
      o_ptr = &inventory[i];
      
      /* Initialize color based of sign of pval. */
      for (j = 0; j < A_MAX; j++)
	{
	  /* Initialize color based of sign of bonus. */
	  a = TERM_SLATE;
	  c = '.';
	  
	  /* Boost */
	  if (o_ptr->bonus_stat[j] != 0)
	    {
	      /* Default */
	      c = '*';
	      
	      /* Good */
	      if (o_ptr->bonus_stat[j] > 0)
		{
		  /* Good */
		  a = TERM_L_GREEN;
		  
		  /* Label boost */
		  if (o_ptr->bonus_stat[j] < 10) c = '0' + o_ptr->bonus_stat[j];
		}
	      
	      /* Bad */
	      if (o_ptr->bonus_stat[j] < 0)
		{
		  /* Bad */
		  a = TERM_RED;
		  
		  /* Label boost */
		  if (o_ptr->bonus_stat[j] < 10) c = '0' - o_ptr->bonus_stat[j];
		}
	    }
	  
	  /* Sustain */
	  if (o_ptr->id_obj & (OF_SUSTAIN_STR << j))
	    {
	      /* Dark green, "s" if no stat bonus. */
	      a = TERM_GREEN;
	      if (c == '.') c = 's';
	    }
	  
	  /* Dump proper character */
	  buf[0] = c;
	  buf[1] = '\0';
	  Term_putch(i + 1, 4 + j, a, c);
	}
      
    }
  
  /* Dump the fighting bonuses to hit/dam */
  
  put_str("       (Fighting)    ", 14, 1);
  prt_num("Blows/Round      ", p_ptr->num_blow, 15, 1, TERM_L_BLUE);
  prt_num("+ to Skill       ", show_m_tohit, 16, 1, TERM_L_BLUE);
  
  if (show_m_todam >= 0)
    prt_num("Deadliness (%)   ", deadliness_conversion[show_m_todam], 
	    17, 1, TERM_L_BLUE);
  else
    prt_num("Deadliness (%)   ", -deadliness_conversion[-show_m_todam], 
	    17, 1, TERM_L_BLUE);
  
  /* Dump the shooting bonuses to hit/dam */
  
  put_str("       (Shooting)    ", 14, 26);

  prt_deci("Shots/Round   ", p_ptr->num_fire/10, p_ptr->num_fire%10, 
	     15, 26, TERM_L_BLUE);

  prt_num("+ to Skill      ", show_a_tohit, 16, 26, TERM_L_BLUE);

  if (show_a_todam > 0)
    prt_num("Deadliness (%)  ", deadliness_conversion[show_a_todam], 
	    17, 26, TERM_L_BLUE);
  else
    prt_num("Deadliness (%)  ", -deadliness_conversion[-show_a_todam], 
	    17, 26, TERM_L_BLUE);

  /* Dump the base and bonus armor class */
  put_str("AC/+ To AC", 21, 26);
  
  sprintf(tmp, "%3d", p_ptr->dis_ac);
  c_put_str(TERM_L_BLUE, tmp, 21, 41);
  
  put_str("/", 21, 44);
  
  sprintf(tmp, "%3d", p_ptr->dis_to_a);
  c_put_str(TERM_L_BLUE, tmp, 21, 45);
  
  prt_num("Level            ", (int)p_ptr->lev, 19, 1, TERM_L_GREEN);

  prt_lnum("Experience    ", p_ptr->exp, 20, 1, TERM_L_GREEN);
  
  prt_lnum("Max Exp       ", p_ptr->max_exp, 21, 1, TERM_L_GREEN);
  
  prt_lnum("Exp to Adv.  ", (s32b)(player_exp[p_ptr->lev - 1]), 19, 26, 
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
  if (resist_value == 0) return TERM_WHITE;
  else if (resist_value <= 20) return TERM_BLUE;
  else if (resist_value <= 80) return TERM_L_GREEN;
  else if (resist_value < 100) return TERM_YELLOW;
  else if (resist_value == 100) return TERM_L_WHITE;
  else if (resist_value < 130) return TERM_ORANGE;
  else if (resist_value < 160) return TERM_L_RED;
  else return TERM_RED;
}


/** Max length of note output */
#define LINEWRAP        75

/** 
 * Make a char_attr array of character information for file dump or screen
 * reading.  Mode 0 and 1 show 24 lines of player info for minor windows.
 */
extern int make_dump(char_attr_line *line, int mode)
{
  int i, j, x, y, col;
  bool quiver_empty = TRUE;
  
  cptr paren = ")";
  
  int k, which = 0;  

  store_type *st_ptr = NULL;
  
  char o_name[120];
  
  char buf[100];
  char buf1[20];
  char buf2[20];

  bool red;

  int show_m_tohit = p_ptr->dis_to_h;
  int show_a_tohit = p_ptr->dis_to_h;
  int show_m_todam = p_ptr->dis_to_d;
  int show_a_todam = p_ptr->dis_to_d;
  
  object_type *o_ptr;
  int value;

  int xthn, xthb, xfos, xsrh;
  int xdis, xdev, xsav, xstl;
  cptr desc;

  int n;
  
  u32b flag;
  cptr name1;

  int player_resists[MAX_P_RES];
  int player_bonus[MAX_P_BONUS];
  
  u32b player_flags_obj;

  int current_line = 0;

  bool dead = FALSE;
  bool have_home = (p_ptr->home != 0);

  /* Get the store number of the home */
  if (have_home)
    {
      if (adult_dungeon) which = NUM_TOWNS_SMALL * 4 + STORE_HOME;
      else
	{
	  for (k = 0; k < NUM_TOWNS; k++)
	    {
	      /* Found the town */
	      if (p_ptr->home == towns[k])
		{
		  which += (k < NUM_TOWNS_SMALL ? 3 : STORE_HOME);
		  break;
		}
	      /* Next town */
	      else
		which += (k < NUM_TOWNS_SMALL ? MAX_STORES_SMALL : MAX_STORES_BIG);
	    }
	}

      /* Activate the store */
      st_ptr = &store[which];
    }

  dump_ptr = (char_attr *)&line[current_line]; 

  /* Hack - skip all this for mode 1 */
  if (mode != 1)
    {  
      /* Begin dump */
      sprintf(buf, "[FAangband %s Character Dump]", VERSION_STRING);
      dump_put_str(TERM_WHITE, buf, 2);
      current_line++;
      
      /* Start of player screen mode 0 */
      if (mode == 0) 
	{
	  current_line = 0;
	  
	  /* Hack - clear the top line */
	  dump_put_str(TERM_WHITE, "", 0);
	}
      
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      
      /* Name, Sex, Race, Class */
      dump_put_str(TERM_WHITE, "Name    : ", 1);
      dump_put_str(TERM_L_BLUE, op_ptr->full_name, 11);
      dump_put_str(TERM_WHITE, "Age", 27);
      sprintf(buf1, "%10d", (int)p_ptr->age);
      dump_put_str(TERM_L_BLUE, buf1, 42);
      red = (p_ptr->stat_cur[0] < p_ptr->stat_max[0]);
      value = p_ptr->stat_use[0];
      cnv_stat(value, buf1);
      value = p_ptr->stat_top[0];
      cnv_stat(value, buf2);
      dump_put_str(TERM_WHITE, (red ? "Str" : "STR"), 53);
      dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[0] == 18 + 100) ? "!" : " "), 
		   56);
      if (red)
	{
	  dump_put_str(TERM_YELLOW, buf1, 61);
	  dump_put_str(TERM_L_GREEN, buf2, 70);
	}
      else
	dump_put_str(TERM_L_GREEN, buf1, 61);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "Sex     : ", 1);
      dump_put_str(TERM_L_BLUE, sp_ptr->title, 11);
      dump_put_str(TERM_WHITE, "Height", 27);
      sprintf(buf1, "%10d", 
	      (use_metric ? ((int)p_ptr->ht) * 254 / 100 : (int)p_ptr->ht));
      dump_put_str(TERM_L_BLUE, buf1, 42);
      red = (p_ptr->stat_cur[1] < p_ptr->stat_max[1]);
      value = p_ptr->stat_use[1];
      cnv_stat(value, buf1);
      value = p_ptr->stat_top[1];
      cnv_stat(value, buf2);
      dump_put_str(TERM_WHITE, (red ? "Int" : "INT"), 53);
      dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[1] == 18 + 100) ? "!" : " "), 
		   56);
      if (red)
	{
	  dump_put_str(TERM_YELLOW, buf1, 61);
	  dump_put_str(TERM_L_GREEN, buf2, 70);
	}
      else
	dump_put_str(TERM_L_GREEN, buf1, 61);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "Race    : ", 1);
      dump_put_str(TERM_L_BLUE, p_name + rp_ptr->name, 11);
      dump_put_str(TERM_WHITE, "Weight", 27);
      sprintf(buf1, "%10d", 
	      (use_metric ? ((int)p_ptr->wt) * 10 / 22 : (int)p_ptr->wt)); 
      dump_put_str(TERM_L_BLUE, buf1, 42);
      red = (p_ptr->stat_cur[2] < p_ptr->stat_max[2]);
      value = p_ptr->stat_use[2];
      cnv_stat(value, buf1);
      value = p_ptr->stat_top[2];
      cnv_stat(value, buf2);
      dump_put_str(TERM_WHITE, (red ? "Wis" : "WIS"), 53);
      dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[2] == 18 + 100) ? "!" : " "), 
		   56);
      if (red)
	{
	  dump_put_str(TERM_YELLOW, buf1, 61);
	  dump_put_str(TERM_L_GREEN, buf2, 70);
	}
      else
	dump_put_str(TERM_L_GREEN, buf1, 61);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "Class   : ", 1);
      dump_put_str(TERM_L_BLUE, c_name + cp_ptr->name, 11);
      dump_put_str(TERM_WHITE, "Social Class", 27);
      sprintf(buf1, "%10d", (int)p_ptr->sc);
      dump_put_str(TERM_L_BLUE, buf1, 42);
      red = (p_ptr->stat_cur[3] < p_ptr->stat_max[3]);
      value = p_ptr->stat_use[3];
      cnv_stat(value, buf1);
      value = p_ptr->stat_top[3];
      cnv_stat(value, buf2);
      dump_put_str(TERM_WHITE, (red ? "Dex" : "DEX"), 53);
      dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[3] == 18 + 100) ? "!" : " "), 
		   56);
      if (red)
	{
	  dump_put_str(TERM_YELLOW, buf1, 61);
	  dump_put_str(TERM_L_GREEN, buf2, 70);
	}
      else
	dump_put_str(TERM_L_GREEN, buf1, 61);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      if (p_ptr->total_winner) 
	dump_put_str(TERM_VIOLET, "***WINNER***", 0);
      red = (p_ptr->stat_cur[4] < p_ptr->stat_max[4]);
      value = p_ptr->stat_use[4];
      cnv_stat(value, buf1);
      value = p_ptr->stat_top[4];
      cnv_stat(value, buf2);
      dump_put_str(TERM_WHITE, (red ? "Con" : "CON"), 53);
      dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[4] == 18 + 100) ? "!" : " "), 
		   56);
      if (red)
	{
	  dump_put_str(TERM_YELLOW, buf1, 61);
	  dump_put_str(TERM_L_GREEN, buf2, 70);
	}
      else
	dump_put_str(TERM_L_GREEN, buf1, 61);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      red = (p_ptr->stat_cur[5] < p_ptr->stat_max[5]);
      value = p_ptr->stat_use[5];
      cnv_stat(value, buf1);
      value = p_ptr->stat_top[5];
      cnv_stat(value, buf2);
      dump_put_str(TERM_WHITE, (red ? "Chr" : "CHR"), 53);
      dump_put_str(TERM_WHITE, ((p_ptr->stat_cur[5] == 18 + 100) ? "!" : " "), 
		   56);
      if (red)
	{
	  dump_put_str(TERM_YELLOW, buf1, 61);
	  dump_put_str(TERM_L_GREEN, buf2, 70);
	}
      else
	dump_put_str(TERM_L_GREEN, buf1, 61);
      current_line +=2;
      
      /* Get the bonuses to hit/dam */
      
      o_ptr = &inventory[INVEN_WIELD];
      if (o_ptr->id_other & IF_TO_H) show_m_tohit += o_ptr->to_h;
      if (o_ptr->id_other & IF_TO_D) show_m_todam += o_ptr->to_d;

      o_ptr = &inventory[INVEN_BOW];
      if (o_ptr->id_other & IF_TO_H) show_a_tohit += o_ptr->to_h;
      if (o_ptr->id_other & IF_TO_D) show_a_todam += o_ptr->to_d;

      dump_ptr = (char_attr *)&line[current_line];
      dump_num("Max Hit Points   ", p_ptr->mhp, 1, TERM_L_GREEN);
      if (p_ptr->lev >= p_ptr->max_lev)
	dump_num("Level            ", (int)p_ptr->lev, 27, TERM_L_GREEN);
      else
	dump_num("Level            ", (int)p_ptr->lev, 27, TERM_YELLOW);
      dump_num("Max SP (Mana)    ", p_ptr->msp, 53, TERM_L_GREEN);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
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
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_lnum("Max Exp       ", p_ptr->max_exp, 27, TERM_L_GREEN);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "(Fighting)", 8);
      if (p_ptr->lev >= PY_MAX_LEVEL)
	{
	  dump_put_str(TERM_WHITE, "Exp to Adv.   ", 27);
	  sprintf(buf, "       *****");
	}
      else
	dump_lnum("Exp to Adv.   ", (s32b)(player_exp[p_ptr->lev - 1]), 27, 
		  TERM_L_GREEN);
      dump_put_str(TERM_WHITE, "(Shooting)", 60);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_num("Blows/Round      ", p_ptr->num_blow, 1, TERM_L_BLUE);
      dump_lnum("Gold          ", p_ptr->au, 27, TERM_L_GREEN);
      dump_deci("Shots/Round    ", p_ptr->num_fire/10, p_ptr->num_fire%10, 53, 
		TERM_L_BLUE);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_num("+ to Skill       ", show_m_tohit, 1, TERM_L_BLUE);
      dump_put_str(TERM_WHITE, "Score", 27);
      sprintf(buf1, "%12d", (int)p_ptr->score);
      dump_put_str(TERM_L_GREEN, buf1, 38);
      dump_num("+ to Skill       ", show_a_tohit, 53, TERM_L_BLUE);
      current_line++;
      
      dump_ptr = (char_attr *)&line[current_line];
      /* Show damage per blow if no weapon wielded */
      if (!inventory[INVEN_WIELD].k_idx)
	{
	  int sum = 0;

	  for (i = 0; i < 12; i++) sum += (int)p_ptr->barehand_dam[i];
	  dump_num("Av. Damage/Blow  ", sum/12, 1, TERM_L_BLUE);
	}
      else
	{	  
	  if (show_m_todam >= 0)
	dump_num("Deadliness (%)   ", deadliness_conversion[show_m_todam], 
		 1, TERM_L_BLUE);
      else
	dump_num("Deadliness (%)   ", -deadliness_conversion[-show_m_todam], 
		 1, TERM_L_BLUE);
	}

      dump_put_str(TERM_WHITE, "Base AC/+ To AC", 27);
      sprintf(buf1, "%3d", p_ptr->dis_ac);
      dump_put_str(TERM_L_BLUE, buf1, 43);
      dump_put_str(TERM_WHITE, "/", 46);
      sprintf(buf1, "%3d", p_ptr->dis_to_a);
      dump_put_str(TERM_L_BLUE, buf1, 47);

      if (show_a_todam > 0)
	dump_num("Deadliness (%)   ", deadliness_conversion[show_a_todam], 
		 53, TERM_L_BLUE);
      else
	dump_num("Deadliness (%)   ", -deadliness_conversion[-show_a_todam], 
		 53, TERM_L_BLUE);

      current_line++;
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "Game Turn", 27);
      sprintf(buf1, "%12d", (int)turn);
      dump_put_str(TERM_L_GREEN, buf1, 38);
      current_line += 2;
      
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "(Character Abilities)", 28);
      
      
      /* Fighting Skill (with current weapon) */
      xthn = p_ptr->skill_thn + (show_m_tohit * BTH_PLUS_ADJ);
      
      /* Shooting Skill (with current bow and normal missile) */
      xthb = p_ptr->skill_thb + (show_a_tohit * BTH_PLUS_ADJ);
      
      /* Basic abilities */
      xdis = p_ptr->skill_dis;
      xdev = p_ptr->skill_dev;
      xsav = p_ptr->skill_sav;
      xstl = p_ptr->skill_stl;
      xsrh = p_ptr->skill_srh;
      xfos = p_ptr->skill_fos;
      
      
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
      
      dump_ptr = (char_attr *)&line[current_line];
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
      
      dump_ptr = (char_attr *)&line[current_line];
      desc = likert(xsav, 7);
      dump_put_str(TERM_WHITE, "SavingThrow:", 1);
      dump_put_str(likert_color, desc, 13);
      
      desc = likert(xsrh, 6);
      dump_put_str(TERM_WHITE, "Searching  :", 27);
      dump_put_str(likert_color, desc, 39);
      
      if (use_metric)
	sprintf(buf1, "%d meters", p_ptr->see_infra * 3);
      else  
	sprintf(buf1, "%d feet", p_ptr->see_infra * 10);
      dump_put_str(TERM_WHITE, "Infravision:", 53);
      dump_put_str(TERM_WHITE, buf1, 65);
      current_line += 2;
      
      
      /* Display history */
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "(Character Background)", 28);
      
      for (i = 0; i < 4; i++)
	{
	  current_line++;
	  dump_ptr = (char_attr *)&line[current_line];
	  dump_put_str(TERM_WHITE, p_ptr->history[i], 10);
	}
      
      /* End of mode 0 */
      if (mode == 0) return (current_line);
      
      current_line += 2;
      
      /* Current, recent and recall points */
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "[Recent locations]", 2);
      current_line += 2;
      
      /* Current, previous */
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "Current Location :", 1);
      sprintf(buf, "%s Level %d",
	      locality_name[stage_map[p_ptr->stage][LOCALITY]], p_ptr->depth);
      dump_put_str(TERM_L_GREEN, buf, 20);
      current_line++;
      
      if (p_ptr->last_stage != 0)
	{
	  dump_ptr = (char_attr *)&line[current_line];
	  dump_put_str(TERM_WHITE, "Previous Location:", 1);
	  sprintf(buf, "%s Level %d", 
		  locality_name[stage_map[p_ptr->last_stage][LOCALITY]], 
		  stage_map[p_ptr->last_stage][DEPTH]);
	  dump_put_str(TERM_L_GREEN, buf, 20);
	  current_line++;
	}
      
      /* Recall points */
      for (i = 0; i < 4; i++)
	{
	  if (p_ptr->recall[i] == 0) continue;
	  dump_ptr = (char_attr *)&line[current_line];
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
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "[Resistances, Powers and Bonuses]", 2);
      current_line += 2;
    }
      
  /* Start of mode 1 player screen */
  if (mode == 1)
    {
      current_line = 0;

      /* Hack - clear all the lines */
      for (i = 0; i < 25; i++)
	{
	  dump_ptr = (char_attr *)&line[i];
	  dump_put_str(TERM_WHITE, "", 0);
	}
    }

  dump_ptr = (char_attr *)&line[current_line];
  
  /* Header */
  dump_put_str(TERM_WHITE, "abcdefghijkl@            abcdefghijkl@", 6);
  current_line++;

  /* Resistances */
  get_player_resists(player_resists);

  for (y = 0; y < 7; y++)
    {
      dump_ptr = (char_attr *)&line[current_line];
      
      for (x = 0; x < 2; x++)
	{
	  int r = y + 7 * x;

	  /* Extract name */
	  name1 = display_player_resist_names[x][y];
	  
	  /* Name */
	  dump_put_str(resist_colour(p_ptr->dis_res_list[y + x * 7]), name1, 
		       1 + 24 * x);
	  
	  /* Check equipment */
	  for (n = 6 + 25 * x, i = INVEN_WIELD; i < INVEN_SUBTOTAL; i++, n++)
	    {
	      object_type *o_ptr;
	      
	      /* Object */
	      o_ptr = &inventory[i];
	      
	      /* Check flags */
	      if ((o_ptr->k_idx) && 
		  (o_ptr->id_other & (OBJECT_ID_BASE_RESIST << r)))
		{
		  if (o_ptr->percent_res[r] == RES_LEVEL_MIN) 
		    dump_put_str(resist_colour(o_ptr->percent_res[r]), "*", n);
		  else if (o_ptr->percent_res[r] < RES_LEVEL_BASE) 
		    dump_put_str(resist_colour(o_ptr->percent_res[r]), "+", n);
		  else if (o_ptr->percent_res[r] > RES_LEVEL_BASE) 
		    dump_put_str(resist_colour(o_ptr->percent_res[r]), "-", n);
		}
	      
	      /* Default */
	      else
		{
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
	  sprintf(buf1, "%4d%%", 100 - p_ptr->dis_res_list[y + x * 7]);
	  dump_put_str(resist_colour(p_ptr->dis_res_list[y + x * 7]), 
		       buf1, n + 1);
	}	  
      current_line++;
    }
  /* Skip the gap for mode 1 */
  if (mode != 1)
    {
      current_line ++;
      dump_ptr = (char_attr *)&line[current_line];
  
      /* Header */
      dump_put_str(TERM_WHITE, "abcdefghijkl@            abcdefghijkl@", 6);
      current_line++;
    }

  /* Powers and Bonuses */
  for (y = 0; y < 10; y++)
    {
      byte a = TERM_WHITE;
      char c;

      dump_ptr = (char_attr *)&line[current_line];
      
      for (x = 0; x < 2; x++)
	{
	  /* Extract flag */
	  flag = display_player_powers[y];
      
	  /* Extract name */
	  name1 = display_player_power_names[y];

	  /* Name */
	  dump_put_str(TERM_WHITE, name1, 0);

	  /* Check equipment */
	  for (n = 6, i = INVEN_WIELD; i < INVEN_SUBTOTAL; i++, n++)
	    {
	      object_type *o_ptr;
	      
	      /* Object */
	      o_ptr = &inventory[i];
	      
	      /* Check flags */
	      if ((o_ptr->k_idx) && (o_ptr->id_obj & flag))
		{
		  dump_put_str(TERM_WHITE, "+", n);
		}
	      
	      /* Default */
	      else
		{
		  dump_put_str((o_ptr->k_idx ? TERM_L_WHITE : TERM_SLATE), 
			       ".", n);
		}
	    }
	  
	  /* Check flags */
	  player_flags(&player_flags_obj);

	  if (player_flags_obj & flag) 
	    {
	      dump_put_str(TERM_WHITE, "+", n);
	    }
	  
	  else
	    {
	      /* Player 'reversed' flags */
	      player_weakness_dis(&player_flags_obj);
	      
	      /* Check flags */
	      if (player_flags_obj & flag) 
		{
		  dump_put_str(TERM_RED, "-", n);
		}
	      
	      else
		/* Default */
		dump_put_str(TERM_L_WHITE, ".", n);
	    }

	  /* Hack - only 8 bonuses */
	  if (y >= MAX_P_BONUS) continue;

	  /* Extract name */
	  name1 = display_player_bonus_names[y];

	  /* Name */
	  dump_put_str(TERM_WHITE, name1, 25);

	  /* Check equipment */
	  for (n = 31, i = INVEN_WIELD; i < INVEN_SUBTOTAL; i++, n++)
	    {
	      object_type *o_ptr;
	      
	      /* Object */
	      o_ptr = &inventory[i];
	      
	      /* Check flags */
	      if (o_ptr->bonus_other[y] != BONUS_BASE)
		{
		  /* Default */
		  c = '*';
		  
		  /* Good */
		  if (o_ptr->bonus_other[y] > 0)
		    {
		      /* Good */
		      a = TERM_L_GREEN;
		      
		      /* Label boost */
		      if (o_ptr->bonus_other[y] < 10) 
			c = '0' + o_ptr->bonus_other[y];
		    }
	      
		  /* Bad */
		  if (o_ptr->bonus_other[y] < 0)
		    {
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
	      else
		{
		  dump_put_str((o_ptr->k_idx ? TERM_L_WHITE : TERM_SLATE), 
			       ".", n);
		}
	    }
	  
	  /* Player flags */
	  get_player_bonus(player_bonus);
	  
	  /* Check flags */
	  if (player_bonus[y] != BONUS_BASE)
	    {
	      /* Default */
	      c = '*';
	      
	      /* Good */
	      if (player_bonus[y] > 0)
		{
		  /* Good */
		  a = TERM_L_GREEN;
		  
		  /* Label boost */
		  if (player_bonus[y] < 10) c = '0' + player_bonus[y];
		}
	      
	      /* Bad */
	      if (player_bonus[y] < 0)
		{
		  /* Bad */
		  a = TERM_RED;
		  
		  /* Label boost */
		  if (player_bonus[y] < 10) c = '0' - player_bonus[y];
		}
	      
	      /* Dump proper character */
	      buf[0] = c;
	      buf[1] = '\0';
	      dump_put_str(a, buf, n);
	    }
	  
	  /* Default */
	  else
	    {
	      dump_put_str(TERM_L_WHITE, ".", n);
	    }
	}	  
      current_line++;
    }
  
  /* Skip for mode 1 */
  if (mode != 1)
    {
      /* Skip some lines */
      current_line += 2;
      
      /* Dump specialties if any */
      if (p_ptr->specialty_order[0] != SP_NO_SPECIALTY)
	{
	  dump_ptr = (char_attr *)&line[current_line];
	  dump_put_str(TERM_WHITE, "[Specialty Abilities]", 2);
	  current_line += 2;
	  
	  for (i = 0; i < 10; i++)
	    {
	      if (p_ptr->specialty_order[i] != SP_NO_SPECIALTY)
		{
		  dump_ptr = (char_attr *)&line[current_line];
		  sprintf(buf, "%s %s", 
			  specialty_names[p_ptr->specialty_order[i]],
			  (i >= p_ptr->specialties_allowed) ? "(forgotten)" : "");
		  dump_put_str(TERM_GREEN, buf, 0);
		  current_line++; 
		}
	    }
	  current_line += 2;
	}
  
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "[Stat Breakdown]", 2);
      current_line += 2;
    }
  
  dump_ptr = (char_attr *)&line[current_line];
  
  
  /* Print out the labels for the columns */
  dump_put_str(TERM_WHITE, "Stat", 0);
  dump_put_str(TERM_BLUE, "Intrnl", 5);
  dump_put_str(TERM_L_BLUE, "Rce Cls Oth", 12);
  dump_put_str(TERM_L_GREEN, "Actual", 24);
  dump_put_str(TERM_YELLOW, "Currnt", 31);
  dump_put_str(TERM_WHITE, "abcdefghijkl", 42);
  current_line++;
  
  /* Display the stats */
  for (i = 0; i < A_MAX; i++)
    {
      dump_ptr = (char_attr *)&line[current_line];

      /* Reduced name of stat */
      dump_put_str(TERM_WHITE, stat_names_reduced[i], 0);
      
      /* Internal "natural" maximum value */
      cnv_stat(p_ptr->stat_max[i], buf);
      dump_put_str(TERM_BLUE, buf, 5);
      
      /* Race, class, and equipment modifiers */
      sprintf(buf, "%3d", rp_ptr->r_adj[i]);
      dump_put_str(TERM_L_BLUE, buf, 12);
      sprintf(buf, "%3d", cp_ptr->c_adj[i]);
      dump_put_str(TERM_L_BLUE, buf, 16);
      sprintf(buf, "%3d", p_ptr->stat_add[i]);
      dump_put_str(TERM_L_BLUE, buf, 20);
      
      /* Resulting "modified" maximum value */
      cnv_stat(p_ptr->stat_top[i], buf);
      dump_put_str(TERM_L_GREEN, buf, 24);
      
      /* Only display stat_use if not maximal */
      if (p_ptr->stat_use[i] < p_ptr->stat_top[i])
	{
	  cnv_stat(p_ptr->stat_use[i], buf);
	  dump_put_str(TERM_YELLOW, buf, 31);
	}

      for (j = INVEN_WIELD; j < INVEN_SUBTOTAL; j++)
	{
	  byte a;
	  char c;

	  col = 42 + j - INVEN_WIELD;
	  
	  /* Access object */
	  o_ptr = &inventory[j];
	  
	  /* Initialize color based of sign of bonus. */
	  a = TERM_SLATE;
	  c = '.';
	  
	  /* Boost */
	  if (o_ptr->bonus_stat[i] != 0)
	    {
	      /* Default */
	      c = '*';
	      
	      /* Good */
	      if (o_ptr->bonus_stat[i] > 0)
		{
		  /* Good */
		  a = TERM_L_GREEN;
		  
		  /* Label boost */
		  if (o_ptr->bonus_stat[i] < 10) c = '0' + o_ptr->bonus_stat[i];
		}
	      
	      /* Bad */
	      if (o_ptr->bonus_stat[i] < 0)
		{
		  /* Bad */
		  a = TERM_RED;
		  
		  /* Label boost */
		  if (o_ptr->bonus_stat[i] < 10) c = '0' - o_ptr->bonus_stat[i];
		}
	    }
	  
	  /* Sustain */
	  if (o_ptr->flags_obj & (OF_SUSTAIN_STR << i))
	    {
	      /* Dark green, "s" if no stat bonus. */
	      a = TERM_GREEN;
	      if (c == '.') c = 's';
	    }
	  
	  /* Dump proper character */
	  buf[0] = c;
	  buf[1] = '\0';
	  dump_put_str(a, buf, col);
	}
      
      current_line++;
    }
  
  /* End of mode 1 */
  if (mode == 1) return (current_line);
  
  current_line++;  
  
  /* If dead, dump last messages -- Prfnoff */
  if (p_ptr->is_dead)
    {
      dump_ptr = (char_attr *)&line[current_line];
      i = message_num();
      if (i > 15) i = 15;
      dump_put_str(TERM_WHITE, "[Last Messages]", 2);
      current_line += 2;
      while (i-- > 0)
	{
	  dump_ptr = (char_attr *)&line[current_line];
	  sprintf(buf, "> %s", message_str((s16b)i));
	  dump_put_str(TERM_WHITE, buf, 0);
	  current_line++;
	}
      current_line += 2;
    }
  
  /* Dump the equipment */
  if (p_ptr->equip_cnt)
    {
      dump_ptr = (char_attr *)&line[current_line];
      dump_put_str(TERM_WHITE, "[Character Equipment]", 2);
      current_line += 2;
      for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
	  if (i == INVEN_BLANK) 
	    {
	      for (j = 0; j < 10; j++)
		{
		  object_desc(o_name, &inventory[i + j + 1], TRUE, 4);
		  if (!streq(o_name, "(nothing)")) 
		    quiver_empty = FALSE;
		}
	      if (!quiver_empty) 
		{
		  current_line++;
		  dump_ptr = (char_attr *)&line[current_line];
		  dump_put_str(TERM_WHITE, "[Quiver]", 9);
		  current_line++;
		}
	    }	      
	  
	  else
	    {
	      dump_ptr = (char_attr *)&line[current_line];
	      object_desc(o_name, &inventory[i], TRUE, 4);
	      if (streq(o_name,"(nothing)")) continue;
	      sprintf(buf, "%c%s %s", index_to_label(i), paren, o_name);
	      dump_put_str(proc_list_color_hack(&inventory[i]), buf, 0);
	      current_line++;
	    }
	}
      current_line += 2;
    }
  
  /* Dump the inventory */
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, "[Character Inventory]", 2);
  current_line += 2;
  for (i = 0; i < INVEN_PACK; i++)
    {
      if (!inventory[i].k_idx) break;
      
      dump_ptr = (char_attr *)&line[current_line];
      object_desc(o_name, &inventory[i], TRUE, 4);
      sprintf(buf, "%c%s %s", index_to_label(i), paren, o_name);
      dump_put_str(proc_list_color_hack(&inventory[i]), buf, 0);
      current_line++;
    
    }
  current_line += 2;  
  
  /* Dump the Home -- if anything there */
  if (have_home)
    {
      if (st_ptr->stock_num)
	{
	  dump_ptr = (char_attr *)&line[current_line];
	  
	  /* Header */
	  dump_put_str(TERM_WHITE, "[Home Inventory]", 2);
	  current_line += 2;
	  
	  /* Dump all available items */
	  for (i = 0; i < st_ptr->stock_num; i++)
	    {
	      dump_ptr = (char_attr *)&line[current_line];
	      object_desc(o_name, &st_ptr->stock[i], TRUE, 4);
	      sprintf(buf, "%c) %s", I2A(i), o_name);
	      dump_put_str(proc_list_color_hack(&st_ptr->stock[i]), buf, 0);
	      current_line++;
	    }
	  
	  /* Add an empty line */
	  current_line += 2;
	}
    }
  
  /* Add in "character start" information */
  dump_ptr = (char_attr *)&line[current_line];
  sprintf(buf, "%s the %s %s", op_ptr->full_name,
          p_name + rp_ptr->name,
          c_name + cp_ptr->name);
  dump_put_str(TERM_WHITE, buf, 0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, notes_start, 0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, "============================================================", 0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, "CHAR.", 34);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, "|   TURN  |      LOCATION        |LEVEL| EVENT", 
	       0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, "============================================================", 0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];

  /* Dump notes */
  i = 0;
  while (notes[i].turn)
    {
      int length, length_info, region, lev;
      char info_note[43];
      char place[32];

      /* Paranoia */
      if ((notes[i].place > NUM_STAGES) || (notes[i].level > 50) ||
	  (notes[i].type > 15))
	{
	  i++;
	  continue;
	}

      region = stage_map[notes[i].place][LOCALITY];
      lev = stage_map[notes[i].place][DEPTH];

      /* Divider before death */
      if ((notes[i].type == NOTE_DEATH) && (!dead))
	{
	  dead = TRUE;
	  dump_put_str(TERM_WHITE, "============================================================", 0);
	  current_line++;
	  dump_ptr = (char_attr *)&line[current_line];
	}

      /* Get the note */
      sprintf(buf, "%s", notes[i].note);
      
      /* Get the location name */
      if (lev)
	strnfmt(place, sizeof(place), "%15s%4d ", locality_name[region], lev);
      else if ((region != UNDERWORLD) && (region != MOUNTAIN_TOP))
	strnfmt(place, sizeof(place), "%15s Town", locality_name[region]);
      else
	strnfmt(place, sizeof(place), "%15s     ", locality_name[region]);
      
      
      /* Make preliminary part of note */
      strnfmt(info_note, sizeof(info_note), "|%9lu| %s | %2d  | ", 
	      notes[i].turn, place, notes[i].level);
      
      /* Write the info note*/
      dump_put_str(TERM_WHITE, info_note, 0);
      
      /* Get the length of the notes */
      length_info = strlen(info_note);
      length = strlen(buf);
      
      /* Break up long notes */
      if ((length + length_info) > LINEWRAP)
	{
	  bool keep_going = TRUE;
	  int startpoint = 0;
	  int endpoint, n;
	  
	  while (keep_going)
	    {
	      /* Don't print more than the set linewrap amount */
	      endpoint = startpoint + LINEWRAP - strlen(info_note) + 1;
	      
	      /* Find a breaking point */
	      while (TRUE)
		{
		  /* Are we at the end of the line? */
		  if (endpoint >= length)
		    {
		      /* Print to the end */
		      endpoint = length;
		      keep_going = FALSE;
		      break;
		    }
		  
		  /* Mark the most recent space or dash in the string */
		  else if ((buf[endpoint] == ' ') ||
			   (buf[endpoint] == '-')) break;
              
		  /* No spaces in the line, so break in the middle of text */
		  else if (endpoint == startpoint)
		    {
		      endpoint = startpoint + LINEWRAP - strlen(info_note) + 1;
		      break;
		    }
		  
		  /* check previous char */
		  endpoint--;
		}
	      
	      /* Make a continued note if applicable */
	      if (startpoint) 
		dump_put_str(TERM_WHITE, 
			     "|  continued...                  |     |  ", 0);
	      
	      /* Write that line to file */
	      for (n = startpoint; n <= endpoint; n++)
		{
		  char ch;
		  
		  /* Ensure the character is printable */
		  ch = (isprint(buf[n]) ? buf[n] : ' ');
		  
		  /* Write out the character */
		  sprintf(buf1, "%c", ch);
		  dump_put_str(notes[i].type, buf1, 
			       strlen(info_note) + n - startpoint);
		  
		}
	      
	      /* Break the line */
	      current_line++;
	      dump_ptr = (char_attr *)&line[current_line];
	      
	      /* Prepare for the next line */
	      startpoint = endpoint + 1;
	      
	    }
	}
  
      /* Add note to buffer */
      else
	{
	  /* Print the note */
	  dump_put_str(notes[i].type, buf, strlen(info_note));
	  
	  /* Break the line */
	  current_line++;
	  dump_ptr = (char_attr *)&line[current_line];
	}

      /* Next note */
      i++;
    }
  

  dump_put_str(TERM_WHITE, "============================================================", 0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];

  /* Fake note */
  if (!dead)
    {
      char info_note[43];
      char place[32];

      /* Get the location name */
      if (p_ptr->depth)
	strnfmt(place, sizeof(place), "%15s%4d ", locality_name[stage_map[p_ptr->stage][LOCALITY]], p_ptr->depth);
      else if ((stage_map[p_ptr->stage][LOCALITY] != UNDERWORLD) && (stage_map[p_ptr->stage][LOCALITY] != MOUNTAIN_TOP))
	strnfmt(place, sizeof(place), "%15s Town", locality_name[stage_map[p_ptr->stage][LOCALITY]]);
      else
	strnfmt(place, sizeof(place), "%15s     ", locality_name[stage_map[p_ptr->stage][LOCALITY]]);
      
      /* Make preliminary part of note */
      strnfmt(info_note, sizeof(info_note), "|%9lu| %s | %2d  | ", 
              turn, place, p_ptr->lev);
      
      /* Write the info note */
      dump_put_str(TERM_WHITE, info_note, 0);
      dump_put_str(NOTE_DEATH, "Still alive", strlen(info_note));

      current_line++;
      dump_ptr = (char_attr *)&line[current_line];
    }
  dump_put_str(TERM_WHITE, "============================================================", 0);
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  
  
  /* Dump options */
  current_line++;
  dump_ptr = (char_attr *)&line[current_line];
  dump_put_str(TERM_WHITE, "[Options]", 2);
  current_line += 2;

  /* Dump options */
  for (i = OPT_adult_start + 4; i < OPT_adult_end; i++)
    {
      if (option_desc[i])
	{
	  dump_ptr = (char_attr *)&line[current_line];
	  sprintf(buf, "%-49s: %s (%s)",
		  option_desc[i],
		  op_ptr->opt[i] ? "yes" : "no ",
		  option_text[i]);
	  dump_put_str(TERM_WHITE, buf, 0);
	  current_line++;
	}
    }

  for (i = OPT_score_start; i < OPT_score_end; i++)
    {
      if (option_desc[i])
	{
	  dump_ptr = (char_attr *)&line[current_line];
	  sprintf(buf, "%-49s: %s (%s)",
		  option_desc[i],
		  op_ptr->opt[i] ? "yes" : "no ",
		  option_text[i]);
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

void display_dump(char_attr_line *line, int top_line, int bottom_line, int col)
{
  int i;
  char_attr *shifted_line;

  /* Start at the top */
  dump_row = 0;

  /* Set the hook */
  dump_line_hook = dump_line_screen;

  /* Dump the lines */
  for (i = top_line; i < bottom_line; i++)
    {
      shifted_line = (char_attr *) &line[i];
      shifted_line += col;
      dump_line(shifted_line);
    }
}

/**
 * Write a character dump to a file
 */

errr file_character(cptr name, char_attr_line *line, int last_line)
{
  int i;
  char buf[100];
  
  /* Drop priv's */
  safe_setuid_drop();
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, name);
  
  /* Check for existing file */
  if (file_exists(buf))
    {
      char out_val[160];
      
      /* Build query */
      sprintf(out_val, "Replace existing file %s? ", buf);
      
      /* Ask */
      if (!get_check(out_val)) return (-1);
    }
  
  /* Open the non-existing file */
  dump_out_file = file_open(buf, MODE_WRITE, FTYPE_TEXT);
  
  /* Grab priv's */
  safe_setuid_grab();
  
  
  /* Invalid file */
  if (!dump_out_file)
    {
      /* Message */
      msg_format("Character dump failed!");
      msg_print(NULL);
      
      /* Error */
      return (-1);
    }

  /* Set the hook */
  dump_line_hook = dump_line_file;

  /* Dump the lines */
  for (i = 0; i < last_line; i++)
    {
      dump_ptr = (char_attr *)&line[i];
      dump_line(dump_ptr);
    }
  
  /* Close it */
  file_close(dump_out_file);
  
  /* Message */
  msg_print("Character dump successful.");
  msg_print(NULL);
  
  /* Success */
  return (0);
}


/**
 * Make a string lower case.
 */
static void string_lower(char *buf)
{
  cptr buf_ptr;
  
  /* No string */
  if (!buf) return;
  
  /* Lower the string */
  for (buf_ptr = buf; *buf_ptr != 0; buf_ptr++)
    {
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

bool show_file(cptr name, cptr what, int line, int mode)
{
  int i, k, n;
  int wid, hgt;
  int ret;
  event_type ke;
  
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
  cptr find = NULL;
  
  /* Jump to this tag */
  cptr tag = NULL;

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
  
  /* Handle second half of screen */
  bool second_half = FALSE;
  bool line_finished = FALSE;

  /* Normal screen ? */
  bool old_normal_screen = FALSE;

  /* mallocs */
  filename = malloc(MAX_BUF);
  path = malloc(MAX_BUF);
  buf = malloc(MAX_BUF);
  buf2 = malloc(MAX_BUF);
  lc_buf = malloc(MAX_BUF);

  /* Show messages first */
  if (easy_more) messages_easy(FALSE);

  /* Record normal screen if it's the first time in */
  if (push_file==0)
  {
    backup_buttons();
    old_normal_screen = normal_screen;
  }
 
  /* Get size */
  Term_get_size(&wid, &hgt);
  
  /* Wipe the hooks */
  for (i = 0; i < 10; i++) 
    {
      hook[i][0] = '\0';
    }

  /* Wipe the mouse menu */
  for (i = 0; i < 24; i++)
    {
      mouse[i] = 0;
    }
  
  /* Copy the filename */
  my_strcpy(filename, name, MAX_BUF);
  
  n = strlen(filename);
  
  /* Extract the tag from the filename */
  for (i = 0; i < n; i++)
    {
      if (filename[i] == '#')
	{
	  filename[i] = '\0';
	  tag = filename + i + 1;
	  break;
	}
    }
  
  /* Redirect the name */
  name = filename;
  
  /* Hack XXX XXX XXX */
  if (what)
    {
      my_strcpy(caption, what, sizeof(caption));
      
      my_strcpy(path, name, MAX_BUF);
      fff = file_open(path, MODE_READ, -1);
    }
  
  /* Look in "help" */
  if (!fff)
    {
      strnfmt(caption, sizeof(caption), "Help file '%s'", name);
      
      path_build(path, MAX_BUF, ANGBAND_DIR_HELP, name);
      fff = file_open(path, MODE_READ, -1);
    }
  
  /* Look in "info" */
  if (!fff)
    {
      strnfmt(caption, sizeof(caption), "Info file '%s'", name);
      
      path_build(path, sizeof(path), ANGBAND_DIR_INFO, name);
      fff = file_open(path, MODE_READ, -1);
    }
  
  /* Oops */
  if (!fff)
    {
      /* Message */
      msg_format("Cannot open '%s'.", name);
      message_flush();
      
      /* Oops */
      ret = TRUE;
      goto DONE;

    }

  /* Note we're entering the file */
  push_file++;  
  
  /* Pre-Parse the file */
  while (TRUE)
    {
      /* Read a line or stop */
      if (!file_getl(fff, buf, MAX_BUF)) break;
      
      /* Check for a mouseable line (note hex parentheses) */
      if ((buf[4] == 0x28) && (isdigit(buf[5])) && (buf[6] == 0x29))
	{
	  mouse[next + 2] = D2I(buf[5]);
	}
      
      /* XXX Parse "menu" items */
      if (prefix(buf, "***** "))
	{
	  char b1 = '[', b2 = ']';
	  
	  /* Notice "menu" requests */
	  if ((buf[6] == b1) && isdigit(buf[7]) &&
	      (buf[8] == b2) && (buf[9] == ' '))
	    {
	      /* This is a menu file */
	      menu = TRUE;
	      
	      /* Extract the menu item */
	      k = D2I(buf[7]);
	      
	      /* Extract the menu item */
	      strcpy(hook[k], buf + 10);
	    }
	  
	  /* Notice "tag" requests */
	  else if (buf[6] == '<')
	    {
	      if (tag)
		{
		  /* Remove the closing '>' of the tag */
		  buf[strlen(buf) - 1] = '\0';
		  
		  /* Compare with the requested tag */
		  if (streq(buf + 7, tag))
		    {
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
  while (TRUE)
    {
      char prompt[80];

      /* Clear screen */
      Term_clear();
      
      
      /* Restart when necessary */
      if (line >= size) line = 0;
      
      
      /* Re-open the file if needed */
      if (next > line)
	{
	  /* Close it */
	  file_close(fff);
	  
	  /* Hack -- Re-Open the file */
	  fff = file_open(path, MODE_READ, -1);
	  if (!fff) 
	    {
	      /* Leaving */
	      push_file--;
          ret = TRUE;
          goto DONE;
	    }
	  
	  /* File has been restarted */
	  next = 0;
	}
      
      /* Goto the selected line */
      while (next < line)
	{

	  /* Get a line */
	  if (!file_getl(fff, buf, MAX_BUF)) break;
	  
	  /* Skip tags/links */
	  if (prefix(buf, "***** ")) continue;
	  
	  /* Count the lines */
	  next++;
	}
      
      
      
      /* Dump the next hgt - 4 lines of the file */
      for (i = 0; i < hgt - 4; )
	{
	  /* Hack -- track the "first" line */
	  if (!i) line = next;
	  
	  /* Get a line of the file or stop */
	  if (!file_getl(fff, buf, MAX_BUF)) break;
	  
	  /* Hack -- skip "special" lines */
	  if (prefix(buf, "***** ")) continue;
	  
	  /* Count the "real" lines */
	  next++;
	  
	  /* Make a copy of the current line for searching */
	  my_strcpy(lc_buf, buf, sizeof(lc_buf));
	  
	  /* Make the line lower case */
	  if (!case_sensitive)
	    string_lower(lc_buf);
	  
	  /* Hack -- keep searching */
	  if (find && !i && !strstr(lc_buf, find)) continue;
	  
	  /* Hack -- stop searching */
	  find = NULL;
	  
	  /* Check if the line is finished */
	  for (k = 0; k < 32; k++)
	    if (!buf[k]) line_finished = TRUE;

	  /* Dump the line */
	  if ((small_screen) && (second_half))
	    {
	      if (!line_finished)
		{
		  for (k = 0; k < strlen(buf); k++)
		    {
		      buf2[k] = buf[k + 32];
		    }
		  Term_putstr(0, i+2, -1, TERM_WHITE, buf2);
		}
	    }
	  else
	    Term_putstr(0, i+2, -1, TERM_WHITE, buf);
	  
	  /* Reset line */
	  line_finished = FALSE;

	  /* Hilite "shower" */
	  if (shower[0])
	    {
	      cptr str = lc_buf;
	      
	      /* Display matches */
	      while ((str = strstr(str, shower)) != NULL)
		{
		  int len = strlen(shower);
		  
		  /* Display the match */
		  Term_putstr(str-lc_buf, i+2, len, TERM_YELLOW, 
			      &buf[str-lc_buf]);
		  
		  /* Advance */
		  str += len;
		}
	    }
	  
	  /* Count the printed lines */
	  i++;
	}
      
      /* Hack -- failed search */
      if (find)
	{
	  bell("Search string not found!");
	  line = back;
	  find = NULL;
	  continue;
	}
      
      
      /* Show a general "title" */
      prt(format("[FAangband %d.%d.%d, %s, Line %d/%d]",
		 VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
		 caption, line, size), 0, 0);
      
      
      /* Buttons */
      kill_all_buttons();
      normal_screen = FALSE;
      add_button("ESC", ESCAPE);
      add_button("?", '?');
      
      /* Prompt -- menu screen */
      if (menu)
	{
	  /* Wait for it */
	  if (small_screen)
	    {
	      strncpy(prompt, 
		      "[Number, 'h':other half, ESC:previous, ?:exit]", 80);
	    }
	  else
	    {
	      strncpy(prompt, 
		      "[Press a number, ESC for previous file or ? to exit]", 
		      80);
	    }
	}
      
      /* Prompt -- small files */
      else if (size <= hgt - 4)
	{
	  /* Wait for it */
	  if (small_screen)
	    {
	      strncpy(prompt, "['h' other half, ESC previous file, ? exit]", 
		      80);
	    }
	  else
	    {
	      strncpy(prompt, "[Press ESC for previous file, ? to exit.]", 80);
	    }
	}
      
      /* Prompt -- large files */
      else
	{
	  /* Wait for it */
	  if (small_screen)
	    {
	      strncpy(prompt, "['h':other half,Space:advance,ESC:last,?:exit]",
		      80); 
	    }
	  else
	    {
	      strncpy(prompt, "[Space to advance, ESC for previous file, or ? to exit.]", 
		      80);
	    }

	  /* More buttons */
	  add_button("Spc", ' ');
	  add_button("-", '-');
	}

      /* Finish the status line */
      prt(prompt, hgt - 1, 0);
      prompt_end = (small_screen ? 0 : strlen(prompt));
      if (small_screen) add_button("h", 'h');
      add_button("/", '/');
      add_button("!", '!');
      add_button("=", '=');
      if (!menu) add_button("#", '#');
      add_button("%", '%');
      update_statusline();

      /* Get a keypress */
      ke = inkey_ex();

      /* Mouse input - menus */
      if ((ke.key == '\xff') && (menu) && (mouse[ke.mousey]))
	{
	  /* Recurse on that file */
	  if (!show_file(hook[mouse[ke.mousey]], NULL, 0, mode)) ke.key = '?';
	}
      
      /* Hack -- return to last screen on escape */
      if (ke.key == ESCAPE) break;
      
      
      /* Toggle case sensitive on/off */
      if (ke.key == '!')
	{
	  case_sensitive = !case_sensitive;
	}
      
      /* Hack -- try showing */
      if (ke.key == '=')
	{
	  /* Get "shower" */
	  prt("Show: ", hgt - 1, 0);
	  (void)askfor_aux(shower, 80, NULL);
	  
	  /* Make the "shower" lowercase */
	  if (!case_sensitive)
	    string_lower(shower);
	}
      
      /* Hack -- try finding */
      if (ke.key == '/')
	{
	  /* Get "finder" */
	  prt("Find: ", hgt - 1, 0);
	  if (askfor_aux(finder, 80, NULL))
	    {
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
      if (ke.key == '#')
	{
	  char tmp[81];
	  prt("Goto Line: ", hgt - 1, 0);
	  strcpy(tmp, "0");
	  if (askfor_aux(tmp, 80, NULL))
	    {
	      line = atoi(tmp);
	    }
	}
      
      /* Hack -- go to a specific file */
      if (ke.key == '%')
	{
	  char tmp[81];
	  prt("Goto File: ", hgt - 1, 0);
	  strcpy(tmp, "help.hlp");
	  if (askfor_aux(tmp, 80, NULL))
	    {
	      if (!show_file(tmp, NULL, 0, mode)) ke.key = '?';
	    }
	}
      
      /* Back up one line */
      if (ke.key == ARROW_UP || ke.key == '8')
	{
	  line = line - 1;
	}
      
      /* Hack -- Allow backing up */
      if ((ke.key == '-') || (ke.key == '9'))
	{
	  line = line - 10;
	  if (line < 0) line = 0;
	}
      
      /* Hack -- Advance a single line */
      if ((ke.key == '\n') || (ke.key == '\r') || (ke.key == '2') || 
	  (ke.key == ARROW_DOWN))
	{
	  line = line + 1;
	}
      
      /* Switch to other page half */
      if ((small_screen) && (ke.key == 'h'))
	{
	  second_half = !second_half;
	}
      
      /* Advance one page */
      if ((ke.key == ' ') || (ke.key == '3'))
	{
	  line = line + hgt - 4;
	}
      
      /* Recurse on numbers */
      if (menu && isdigit(ke.key) && hook[D2I(ke.key)][0])
	{
	  /* Recurse on that file */
	  if (!show_file(hook[D2I(ke.key)], NULL, 0, mode)) ke.key = '?';
	}
      
      /* Exit on '?' */
      if (ke.key == '?') break;
    }

  /* Kill the buttons */
  if (push_file == 1)
    { 
      normal_screen = old_normal_screen;
      update_statusline();
    }

  /* Close the file */
  file_close(fff);
  push_file--;
  
  /* Normal return */
  ret = TRUE;
 
  /* Exit on '?' */
 if (ke.key == '?') ret = FALSE;
 
DONE:
free(filename);
free(path);
free(buf);
free(buf2);
free(lc_buf);

if (push_file==0)
restore_buttons();

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
  (void)show_file("help.hlp", NULL, 0, 0);
  
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
  if (strlen(op_ptr->full_name) > 15)
    {
      /* Name too long */
      quit_fmt("The name '%s' is too long!", op_ptr->full_name);
    }
  
  /* Cannot contain "icky" characters */
  for (i = 0; op_ptr->full_name[i]; i++)
    {
      /* No control characters */
      if (iscntrl(op_ptr->full_name[i]))
	{
	  /* Illegal characters */
	  quit_fmt("The name '%s' contains control chars!", op_ptr->full_name);
	}
    }
  
  
#ifdef MACINTOSH
  
  /* Extract "useful" letters */
  for (i = 0; op_ptr->full_name[i]; i++)
    {
      char c = op_ptr->full_name[i];
      
      /* Convert "colon" and "period" */
      if ((c == ':') || (c == '.')) c = '_';
      
      /* Accept all the letters */
      op_ptr->base_name[k++] = c;
    }
  
#else
  
  /* Extract "useful" letters */
  for (i = 0; op_ptr->full_name[i]; i++)
    {
      char c = op_ptr->full_name[i];
      
      /* Accept some letters */
      if (isalpha(c) || isdigit(c)) op_ptr->base_name[k++] = c;
      
      /* Convert space, dot, and underscore to underscore */
      else if (strchr(". _", c)) op_ptr->base_name[k++] = '_';
    }
  
#endif
  
  
#if defined(WINDOWS) || defined(MSDOS)
  
  /* Hack -- max length */
  if (k > 8) k = 8;
  
#endif
  
  /* Terminate */
  op_ptr->base_name[k] = '\0';
  
  /* Require a "base" name */
  if (!op_ptr->base_name[0]) strcpy(op_ptr->base_name, "PLAYER");
  
  
#ifdef SAVEFILE_MUTABLE
  
  /* Accept */
  sf = TRUE;
  
#endif
  
  /* Change the savefile name */
  if (sf)
    {
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
#endif /* VM */
      
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
 * Hack -- commit suicide
 */
void do_cmd_suicide(void)
{
  int i;
  
  /* Flush input */
  flush();
  
  /* Verify Retirement */
  if (p_ptr->total_winner)
    {
      /* Verify */
      if (!get_check("Do you want to retire? ")) return;
    }
  
  /* Verify Suicide */
  else
    {
      /* Verify */
      if (!get_check("Do you really want to suicide? ")) return;
      
      /* Special Verification for suicide */
      prt("Please verify SUICIDE by typing the '@' sign: ", 0, 0);
      flush();
      i = inkey();
      prt("", 0, 0);
      if (i != '@') return;
    }
  
  /* Commit suicide */
  p_ptr->is_dead = TRUE;
  
  /* Stop playing */
  p_ptr->playing = FALSE;
  
  /* Leaving */
  p_ptr->leaving = TRUE;
  
  /* Cause of death */
  strcpy(p_ptr->died_from, "Quitting");
}


/**
 * Save the game
 */
void do_cmd_save_game(void)
{
  /* Disturb the player */
  disturb(1, 0);
  
  /* Clear messages */
  message_flush();
  
  /* Handle stuff */
  handle_stuff();
  
  /* Message */
  if (!is_autosave) prt("Saving game...", 0, 0);
  
  /* Refresh */
  Term_fresh();
  
  /* The player is not dead */
  strcpy(p_ptr->died_from, "(saved)");
  
  /* Forbid suspend */
  signals_ignore_tstp();
  
  /* Save the player */
  if (save_player())
    {
      if (!is_autosave) prt("Saving game... done.", 0, 0);
    }
  
  /* Save failed (oops) */
  else
    {
      prt("Saving game... failed!", 0, 0);
    }
  
  /* Allow suspend again */
  signals_handle_tstp();
  
  /* Refresh */
  Term_fresh();
  
  /* Note that the player is not dead */
  strcpy(p_ptr->died_from, "(alive and well)");
}



/**
 * Gets a personalized string for ghosts.  Code originally from get_name. -LM-
 */
static char *get_personalized_string(byte choice)
{
  static char tmp[80], info[80];
  byte n, i;
  
  /* Clear last line */
  clear_from(15);
  
  /* Prompt and ask */
  if (choice == 1)
    { 
      prt("Enter a message for your character's ghost", 15, 0);
      prt("above, or hit ESCAPE.", 16, 0);
    }
  else if (choice == 2) 
    {
      prt("Enter an addition to your character ghost's", 15, 0);
      prt("description above, or hit ESCAPE.", 16, 0);
    }
  else return NULL;

  sprintf(info, "(%d characters maximum.  Entry will be used as", 
	  (small_screen ? 47 : 79));

  prt(info, 17, 0);
  prt("(a) sentence(s).)", 18, 0);
  
  /* Ask until happy */
  while (1)
    {
      /* Start at beginning of field. */
      move_cursor(14, 0);
      
      /* Get an input */
      (void)askfor_aux(tmp, (small_screen ? 47 : 79), NULL);
      
      /* All done */
      break;
    }
  
  /* Pad the string (to clear junk and allow room for a ending) */
  if (small_screen)
    sprintf(tmp, "%-47.47s", tmp);
  else
    sprintf(tmp, "%-79.79s", tmp);
  
  /* Ensure that strings end like a sentence, and neatly clip the string. */
  for (n = (small_screen ? 47 : 79); ; n--)
    {
      if ((tmp[n] == ' ') || (tmp[n] == '\0')) continue;
      else
	{
	  if ((tmp[n] == '!') || (tmp[n] == '.') || (tmp[n] == '?'))
	    {
	      tmp[n + 1] = '\0';
	      for (i = n + 2; i < (small_screen ? 48 : 80); i++) tmp[i] = '\0';
	      break;
	    }
	  else 
	    {
	      tmp[n + 1] = '.';
	      tmp[n + 2] = '\0';
	      for (i = n + 3; i < (small_screen ? 48 : 80); i++) tmp[i] = '\0';
	      break;
	    }
	}
    }
  
  /* Start the sentence with a capital letter. */
  if (islower(tmp[0])) tmp[0] = toupper(tmp[0]);
  
  /* Return the string */
  return tmp;
  
}

/**
 * Save a "bones" file for a dead character.  Now activated and (slightly) 
 * altered.  Allows the inclusion of personalized strings. 
 */
static void make_bones(void)
{
  ang_file *fp;
  
  char str[1024];
  event_type answer;
  byte choice=0;
  
  int i;
  
  /* Ignore wizards and borgs */
  if (!(p_ptr->noscore & 0x00FF))
    {
      /* Ignore people who die in town */
      if (p_ptr->depth)
	{
	  int level;
	  char tmp[128];
	  
	  /* Slightly more tenacious saving routine. */
	  for (i = 0; i < 5; i++)
	    {
	      /* Ghost hovers near level of death. */
	      if (i == 0) level = p_ptr->depth;
	      else level = p_ptr->depth + 5 - damroll(2, 4);
	      if (level < 1) level = randint(4);
	      
	      /* XXX XXX XXX "Bones" name */
	      sprintf(tmp, "bone.%03d", level);
	      
	      /* Build the filename */
	      path_build(str, 1024, ANGBAND_DIR_BONE, tmp);
	      
	      /* Attempt to open the bones file */
	      fp = file_open(str, MODE_READ, FTYPE_TEXT);
	      
	      /* Close it right away */
	      if (fp) file_close(fp);
	      
	      /* Do not over-write a previous ghost */
	      if (fp) continue;
	      
	      /* If no file by that name exists, we can make a new one. */
	      if (!(fp)) break;
	    }
	  
	  /* Failure */
	  if (fp) return;
	  
	  /* Try to write a new "Bones File" */
	  fp = file_open(str, MODE_WRITE, FTYPE_TEXT);
	  
	  /* Not allowed to write it?  Weird. */
	  if (!fp) return;
	  
	  /* Save the info */
	  if (op_ptr->full_name[0] != '\0') 
	    file_putf(fp, "%s\n", op_ptr->full_name);
	  else file_putf(fp, "Anonymous\n");
	  
	  
	  
	  file_putf(fp, "%d\n", p_ptr->psex);
	  file_putf(fp, "%d\n", p_ptr->prace);
	  file_putf(fp, "%d\n", p_ptr->pclass);
	  
	  /* Clear screen */
	  Term_clear();
	  
	  while(1)
	    {
	      /* Ask the player if he wants to 
	       * add a personalized string. 
	       */
	      prt("Information about your character has been saved", 15, 0);
	      prt("in a bones file.  Would you like to give the", 16, 0);
	      prt("ghost a special message or description? (yes/no)", 17, 0);
	      add_button("Yes", 'y');
	      add_button("No", 'n');
	      update_statusline();	
      
	      answer = inkey_ex();
	      
	      /* Clear last line */
	      clear_from(15);
	      clear_from(16);
	      
	      /* Determine what the personalized string will be used for.  */
	      if ((answer.key == 'Y') || (answer.key == 'y'))
		{
		  prt("Will you add something for your ghost to say,", 15, 0);
		  prt("or add to the monster description?", 16, 0);
		  prt("((M)essage/(D)escription)", 17, 0);

		  /* Buttons */
		  kill_button('y');
		  kill_button('n');
		  add_button("M", 'M');
		  add_button("D", 'D');
		  add_button("ESC", ESCAPE);
		  update_statusline();
		  
		  while(1)
		    {
		      answer = inkey_ex();
		      
		      clear_from(15);
		      clear_from(16);
		      
		      if ((answer.key == 'M') || (answer.key == 'm'))
			{
			  choice = 1;
			  break;
			}
		      else if ((answer.key == 'D') || (answer.key == 'd'))
			{
			  choice = 2;
			  break;
			}
		      else
			{
			  choice = 0;
			  break;
			}
		    }
		}
	      else if ((answer.key == 'N') || (answer.key == 'n') || 
		       (answer.key == ESCAPE)) 
		{
		  choice = 0;
		  break;
		}
	      
	      kill_all_buttons();
	      
	      /* If requested, get the personalized string, and write it and 
	       * info on how it should be used in the bones file.  Otherwise, 
	       * indicate the absence of such a string.
	       */
	      if (choice) file_putf(fp, "%d:%s\n", 
				  choice, get_personalized_string(choice));
	      else file_putf(fp, "0: \n");
	      
	      /* Close and save the Bones file */
	      file_close(fp);
	      
	      return;
	    }
	}
    }
}


/**
 * Centers a string within a 31 character string
 */
static void center_string(char *buf, cptr str)
{
  int i, j;
  
  /* Total length */
  i = strlen(str);
  
  /* Necessary border */
  j = 15 - i / 2;
  
  /* Mega-Hack */
  sprintf(buf, "%*s%s%*s", j, "", str, 31 - i - j, "");
}

/**
 * Hack - save the time of death
 */
static time_t death_time = (time_t)0;


/**
 * Encode the screen colors for the closing screen
 */
static char hack[17] = "dwsorgbuDWvyRGBU";

/**
 * Display a "tomb-stone"
 */
static void print_tomb(void)
{
  cptr p;

  int offset = 12;
  
  char tmp[160];
  
  char buf[1024];
  
  ang_file *fp;
  
#ifdef _WIN32_WCE
  time_t ct = fake_time((time_t)0);
#else
  time_t ct = time((time_t)0);
#endif

  bool boat = ((p_ptr->total_winner) && (check_ability(SP_ELVEN)));
  bool tree = ((p_ptr->total_winner) && 
	       (check_ability(SP_WOODEN) || check_ability(SP_DIVINE))); 
  
  /* Clear screen */
  Term_clear();
  
  /* Build the filename */
  if (tree)
   path_build(buf, 1024, ANGBAND_DIR_FILE, 
	      (small_screen ? "tree_s.txt" : "tree.txt"));
  else if (boat)
    path_build(buf, 1024, ANGBAND_DIR_FILE, 
	       (small_screen ? "boat_s.txt" : "boat.txt"));
  else
    path_build(buf, 1024, ANGBAND_DIR_FILE, 
	       (small_screen ? "dead_s.txt" : "dead.txt"));
	       
  /* Open the News file */
  fp = file_open(buf, MODE_READ, FTYPE_TEXT);
  
  /* Dump */
  if (fp)
    {
      int i, y, x;
      
      byte a = 0;
      char c = ' ';
      
      bool okay = TRUE;
      
      int len;
      
      
      /* Load the screen */
      for (y = 0; okay; y++)
	{
	  /* Get a line of data */
	  if (!file_getl(fp, buf, 1024)) okay = FALSE;
	  
	  /* Stop on blank line */
	  if (!buf[0]) break;
	  
	  /* Get the width */
	  len = strlen(buf);
	  
	  /* XXX Restrict to current screen size */
	  if (len >= Term->wid) len = Term->wid;
	  
	  /* Show each row */
	  for (x = 0; x < len; x++)
	    {
	      /* Put the attr/char */
	      Term_draw(x, y, TERM_WHITE, buf[x]);
	    }
	}
      
      /* Get the blank line */
      /* if (my_fgets(fp, buf, 1024)) okay = FALSE; */
      
      
      /* Load the screen */
      for (y = 0; okay; y++)
	{
	  /* Get a line of data */
	  if (!file_getl(fp, buf, 1024)) okay = FALSE;
	  
	  /* Stop on blank line */
	  if (!buf[0]) break;
	  
	  /* Get the width */
	  len = strlen(buf);
	  
	  /* XXX Restrict to current screen size */
	  if (len >= Term->wid) len = Term->wid;
	  
	  /* Show each row */
	  for (x = 0; x < len; x++)
	    {
	      /* Get the attr/char */
	      (void)(Term_what(x, y, &a, &c));
	      
	      /* Look up the attr */
	      for (i = 0; i < 16; i++)
		{
		  /* Use attr matches */
		  if (hack[i] == buf[x]) a = i;
		}
	      
	      /* Put the attr/char */
	      Term_draw(x, y, a, c);
	    }

	  /* Place the cursor */
	  move_cursor(y, x);
	  
	}
      
      
      /* Get the blank line */
      /* if (my_fgets(fp, buf, 1024)) okay = FALSE; */
      
      
      /* Close it */
      file_close(fp);
    }
  
  /* King or Queen */
  if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL))
    {
      p = "Magnificent";
    }
  
  /* Normal */
  else
    {
      p = c_text + cp_ptr->title[(p_ptr->lev - 1) / 5];
    }

  /* Set offset */
  if (small_screen && (!tree)) offset = 3;
  else offset = 11;
  
  center_string(buf, op_ptr->full_name);
  put_str(buf, 6, offset);
  
  center_string(buf, "the");
  put_str(buf, 7, offset);
  
  center_string(buf, p);
  put_str(buf, 8, offset);
  
  
  center_string(buf, c_name + cp_ptr->name);
  put_str(buf, 10, offset);
  
  sprintf(tmp, "Level: %d", (int)p_ptr->lev);
  center_string(buf, tmp);
  put_str(buf, 11, offset);
  
  sprintf(tmp, "Exp: %ld", (long)p_ptr->exp);
  center_string(buf, tmp);
  put_str(buf, 12, offset);
  
  sprintf(tmp, "AU: %ld", (long)p_ptr->au);
  center_string(buf, tmp);
  put_str(buf, 13, offset);
  
  if (p_ptr->depth)
    sprintf(tmp, "Killed in %s level %d", 
	    locality_name[stage_map[p_ptr->stage][LOCALITY]], p_ptr->depth);
  else if (boat)
    sprintf(tmp, "Sailed victorious to Aman.");
  else if (tree)
    sprintf(tmp, "Retired to Fangorn Forest.");
  else
    sprintf(tmp, "Killed in %s town", 
	    locality_name[stage_map[p_ptr->stage][LOCALITY]]);  
  center_string(buf, tmp);
  put_str(buf, 14, offset);
  
  if (!(boat || tree))
    {
      sprintf(tmp, "by %s.", p_ptr->died_from);
      center_string(buf, tmp);
      put_str(buf, 15, offset);
    }
  
#ifdef _WIN32_WCE
  {	
    char* fake_ctime(const unsigned long* fake_time_t);
    sprintf(tmp, "%-.24s", fake_ctime(&ct));
  }
#else
  sprintf(tmp, "%-.24s", ctime(&ct));
#endif
  center_string(buf, tmp);
  put_str(buf, 17, offset);
}


/**
 * Hack - Know inventory and home items upon death
 */
static void death_knowledge(void)
{
  int i, which = 0;
  
  object_type *o_ptr;
  
  store_type *st_ptr = NULL;
  
  /* Get the store number of the home */
  if (adult_dungeon) which = NUM_TOWNS_SMALL * 4 + STORE_HOME;
  else
    {
      for (i = 0; i < NUM_TOWNS; i++)
	{
	  /* Found the town */
	  if (p_ptr->home == towns[i])
	    {
	      which += (i < NUM_TOWNS_SMALL ? 3 : STORE_HOME);
	      break;
	    }
	  /* Next town */
	  else
	    which += (i < NUM_TOWNS_SMALL ? MAX_STORES_SMALL : MAX_STORES_BIG);
	}
    }

  /* Hack -- Know everything in the inven/equip */
  for (i = 0; i < INVEN_TOTAL; i++)
    {
      o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Aware and Known */
      object_aware(o_ptr);
      object_known(o_ptr);
      
      /* Fully known */
      o_ptr->id_curse = o_ptr->flags_curse;
    }
  
  /* Thralls sometimes (often!) don't have a home */
  if (p_ptr->home)
    {
      /* Activate the store */
      st_ptr = &store[which];
      
      
      /* Hack -- Know everything in the home */
      for (i = 0; i < st_ptr->stock_num; i++)
	{
	  o_ptr = &st_ptr->stock[i];
	  
	  /* Skip non-objects */
	  if (!o_ptr->k_idx) continue;
	  
	  /* Aware and Known */
	  object_aware(o_ptr);
	  object_known(o_ptr);
	  
	  /* Fully known */
	  o_ptr->id_curse = o_ptr->flags_curse;
	}
    }
  
  /* Hack -- Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Handle stuff */
  handle_stuff();
}

/**
 * Display some character info
 */
static void show_info(void)
{
  int i, j, k, which = 0;
  
  object_type *o_ptr;
  
  store_type *st_ptr;

  event_type ke;

  bool done = FALSE;
  
  /* Get the store number of the home */
  if (adult_dungeon) which = NUM_TOWNS_SMALL * 4 + STORE_HOME;
  else
    {
      for (i = 0; i < NUM_TOWNS; i++)
	{
	  /* Found the town */
	  if (p_ptr->home == towns[i])
	    {
	      which += (i < NUM_TOWNS_SMALL ? 3 : STORE_HOME);
	      break;
	    }
	  /* Next town */
	  else
	    which += (i < NUM_TOWNS_SMALL ? MAX_STORES_SMALL : MAX_STORES_BIG);
	}
    }

  /* Activate the store */
  st_ptr = &store[which];
  
  /* Display player */
  display_player(0);
  
  /* Prompt for inventory */
  prt("Hit any key to see more information (ESC to abort): ", 23, 0);
  
  /* Buttons */
  backup_buttons();
  kill_all_buttons();
  add_button("ESC", ESCAPE);
  add_button("Continue", 'q');
  update_statusline();

  /* Allow abort at this point */
  ke = inkey_ex();
  if (ke.key == ESCAPE) done = TRUE;
  
  /* Show equipment and inventory */
  
  /* Equipment -- if any */
  if ((p_ptr->equip_cnt) && !done)
    {
      Term_clear();
      item_tester_full = TRUE;
      show_equip();
      prt("You are using: -more-", 0, 0);
      update_statusline();
      ke = inkey_ex();
      if (ke.key == ESCAPE) done = TRUE;
    }

  /* Inventory -- if any */
  if ((p_ptr->inven_cnt) && !done)
    {
      Term_clear();
      item_tester_full = TRUE;
      show_inven();
      prt("You are carrying: -more-", 0, 0);
      update_statusline();
      ke = inkey_ex();
      if (ke.key == ESCAPE) done = TRUE;
    }
  
  
  
  /* Home -- if anything there */
  if ((st_ptr->stock_num) && !done)
    {
      /* Display contents of the home */
      for (k = 0, i = 0; i < st_ptr->stock_num; k++)
	{
	  /* Clear screen */
	  Term_clear();
	  
	  /* Show 12 items */
	  for (j = 0; (j < 12) && (i < st_ptr->stock_num); j++, i++)
	    {
	      byte attr;
	      
	      char o_name[80];
	      char tmp_val[80];
	      
	      /* Get the object */
	      o_ptr = &st_ptr->stock[i];
	      
	      /* Print header, clear line */
	      sprintf(tmp_val, "%c) ", I2A(j));
	      prt(tmp_val, j+2, 4);
	      
	      /* Get the object description */
	      object_desc(o_name, o_ptr, TRUE, 4);
	      
	      /* Get the inventory color */
	      attr = tval_to_attr[o_ptr->tval & 0x7F];
	      
	      /* Display the object */
	      c_put_str(attr, o_name, j+2, 7);
	    }
	  
	  /* Caption */
	  prt(format("Your home contains (page %d): -more-", k+1), 0, 0);
	  update_statusline();
	  
	  /* Wait for it */
	  ke = inkey_ex();
	  if (ke.key == ESCAPE) done = TRUE;
	}
    }
}



/**
 * Special version of 'do_cmd_examine'
 */
static void death_examine(void)
{
  int item;
  
  object_type *o_ptr;
  
  char o_name[80];
  
  cptr q, s;
  
  
  /* Start out in "display" mode */
  p_ptr->command_see = TRUE;
  
  /* Get an item */
  q = "Examine which item? ";
  s = "You have nothing to examine.";
  if (!get_item(&item, q, s, (USE_INVEN | USE_EQUIP))) return;
  
  /* Get the item (in the pack) */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  /* Aware and Known */
  object_aware(o_ptr);
  object_known(o_ptr);
  
  /* Fully known */
  o_ptr->id_curse = o_ptr->flags_curse;
  
  /* Description */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Save screen */
  screen_save();
  
  /* Examine the item. */
  object_info_screen(o_ptr, FALSE);

  /* Load screen */
  screen_load();
}

/**
 * Change the player into a Winner
 */
static void kingly(void)
{
  /* Hack -- retire in town */
  p_ptr->depth = 0;
  
  /* Fake death */
  strcpy(p_ptr->died_from, "Ripe Old Age");
  
  /* Restore the experience */
  p_ptr->exp = p_ptr->max_exp;
  
  /* Restore the level */
  p_ptr->lev = p_ptr->max_lev;
  
  /* Hack -- Instant Gold */
  p_ptr->au += 10000000L;
  
  /* Limit to avoid buffer overflow */
  if (p_ptr->au > PY_MAX_GOLD) p_ptr->au = PY_MAX_GOLD;
	    
  /* Clear screen */
  Term_clear();
  
  /* Display a crown */
  put_str("#", 1, 34);
  put_str("#####", 2, 32);
  put_str("#", 3, 34);
  put_str(",,,  $$$  ,,,", 4, 28);
  put_str(",,=$   \"$$$$$\"   $=,,", 5, 24);
  put_str(",$$        $$$        $$,", 6, 22);
  put_str("*>         <*>         <*", 7, 22);
  put_str("$$         $$$         $$", 8, 22);
  put_str("\"$$        $$$        $$\"", 9, 22);
  put_str("\"$$       $$$       $$\"", 10, 23);
  put_str("*#########*#########*", 11, 24);
  put_str("*#########*#########*", 12, 24);
  
  /* Display a message */
  put_str("Veni, Vidi, Vici!", 15, 26);
  put_str("I came, I saw, I conquered!", 16, 21);
  put_str(format("All Hail the Mighty %s!", sp_ptr->winner), 17, 22);
  
  /* Flush input */
  flush();
  
  /* Wait for response */
  pause_line(23);
}

/**
 * Handle character death
 */
static void close_game_aux(void)
{
  int ch, adj = 0;
  event_type ke;
  
  cptr p, q;
  
  /* Flush all input keys */
  flush();

  /* Easy more? */
  if (easy_more) messages_easy(FALSE);
	
  /* Screen no longer normal */
  normal_screen = FALSE;

  /* Dump bones file */
  make_bones();

  if (small_screen)
    {
      q = "['a' add to notes file,'c' chardump,'t' scores]";
      adj = 22;
    }
  else
    {
      q = "['a' to add a notes file comment, 'c' for a character dump, 't' for scores]";
      adj = 41;
    }
  if (small_screen)
    p = "['i' info, 'm' messages, 'x' items, or ESC]";
  else
    p = "['i' for character info, 'm' for messages, 'x' to examine items, or ESC]";

  /* Handle retirement */
  if (p_ptr->total_winner) kingly();

  /* Get time of death */
#ifdef _WIN32_WCE
  {
    unsigned long fake_time(unsigned long* fake_time_t);
    fake_time(&death_time);
  }
#else
  (void)time(&death_time);
#endif

  /* You are dead */
  print_tomb();

  /* Hack - Know everything upon death */
  death_knowledge();

  /* Enter player in high score list */
  enter_score(&death_time);

  /* Flush all input keys */
  flush();

  /* Flush messages */
  msg_print(NULL);

  /* Forever */
  while (1)
    {
      /* Describe options */
      Term_putstr((small_screen ? 0 : 2), 21, -1, TERM_WHITE, q);
      Term_putstr((small_screen ? 0 : 2), 22, -1, TERM_WHITE, p);
      
      /* Buttons */
      kill_all_buttons();
      add_button("ESC", ESCAPE);
      add_button("x", 'x');
      add_button("m", 'm');
      add_button("i", 'i');
      add_button("t", 't');
      add_button("c", 'c');
      add_button("a", 'a');
      update_statusline();

      /* Query */
      ke = inkey_ex();
      ch = ke.key;
      
      
      /* Exit */
      if (ch == ESCAPE)
	{
	  if (get_check("Do you want to quit? ")) break;
	}

      /* File dump */
      else if (ch == 'c')
	{
	  /* Show a character screen */
	  do_cmd_change_name();
	  
	  /* Flush messages */
	  msg_print(NULL);
	  
	}

      /* Show more info */
      else if (ch == 'i')
	{
	  /* Save screen */
	  screen_save();

	  /* Show the character */
	  show_info();

	  /* Load screen */
	  screen_load();
	}

      /* Show top scores */
      else if (ch == 't')
	{
	  /* Save screen */
	  screen_save();

	  /* Show the scores */
	  show_scores();


	  /* Load screen */
	  screen_load();
	}

      /* Show top scores */
      else if (ch == 'm')
	{
	  /* Save screen */
	  screen_save();

	  /* Show the scores */
	  do_cmd_messages();


	  /* Load screen */
	  screen_load();
	}

      /* Examine an item */
      else if (ch == 'x')
	{
	  death_examine();
	}
      
      /* Add last words to notes file */
      else if (ch == 'a')
	{
	  do_cmd_note();
	  
	}
    }


  /* Save dead player */
  if (!save_player())
    {
      msg_print("death save failed!");
      msg_print(NULL);
    }
  
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
  event_type ke;
  
  /* Handle stuff */
  handle_stuff();
  
  /* Flush the messages */
  msg_print(NULL);
  
  /* Flush the input */
  flush();
  
  
  /* No suspending now */
  signals_ignore_tstp();
  
  
  /* Hack -- Character is now "icky" */
  character_icky = TRUE;
  
  /* Paranoia */
  game_start = FALSE;

  /* Handle death */
  if (p_ptr->is_dead)
    {
      /* Auxiliary routine */
      close_game_aux();
    }
  
  /* Still alive */
  else
    {
      /* Save the game */
      do_cmd_save_game();
      
      /* Prompt for scores XXX XXX XXX */
      prt("Press Return (or Escape).", 0, 40);
      
      /* Predict score (or ESCAPE) */
      ke = inkey_ex();
      if (ke.key != ESCAPE) predict_score();
    }
  
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
  if (!character_generated || character_saved) quit("panic");
  
  /* Mega-Hack -- see "msg_print()" */
  msg_flag = FALSE;
  
  /* Clear the top line */
  prt("", 0, 0);
  
  /* Hack -- turn off some things */
  disturb(1, 0);
  
  /* Hack -- Delay death XXX XXX XXX */
  if (p_ptr->chp < 0) p_ptr->is_dead = FALSE;
  
  /* Hardcode panic save
  p_ptr->panic_save = 1; */
  
  /* Forbid suspend */
  signals_ignore_tstp();
  
  /* Indicate panic save */
  strcpy(p_ptr->died_from, "(panic save)");
  
  /* Panic save, or get worried */
  if (!save_player()) quit("panic save failed!");
  
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
  if (!fp) return (-1);
  
  /* Parse the file */
  if (file_getl(fp, buf, 80))
    lines = atoi(buf);
  else return (1);
  
  /* choose a random line */
  line = randint(lines);
  
  for (counter = 0; counter <= line; counter++)
    {
      if (!(file_getl(fp, buf, 80)))
	return (1);
      else if (counter == line)
	break;
    }
  
  strcpy (output, buf);
  
  /* Close the file */
  file_close(fp);
  
  return (0);
}



#ifdef HANDLE_SIGNALS


#include <signal.h>


/**
 * Handle signals -- suspend
 *
 * Actually suspend the game, and then resume cleanly
 */
static void handle_signal_suspend(int sig)
{
  /* Disable handler */
  (void)signal(sig, SIG_IGN);
  
#ifdef SIGSTOP

  /* Flush output */
  Term_fresh();
  
  /* Suspend the "Term" */
  Term_xtra(TERM_XTRA_ALIVE, 0);
  
  /* Suspend ourself */
  (void)kill(0, SIGSTOP);
  
  /* Resume the "Term" */
  Term_xtra(TERM_XTRA_ALIVE, 1);
  
  /* Redraw the term */
  Term_redraw();
  
  /* Flush the term */
  Term_fresh();
  
#endif

  /* Restore handler */
  (void)signal(sig, handle_signal_suspend);
}


/**
 * Handle signals -- simple (interrupt and quit)
 *
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 *
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 *
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 */
static void handle_signal_simple(int sig)
{
  /* Disable handler */
  (void)signal(sig, SIG_IGN);
  
  
  /* Nothing to save, just quit */
  if (!character_generated || character_saved) quit(NULL);
  
  
  /* Count the signals */
  signal_count++;
  
  
  /* Terminate dead characters */
  if (p_ptr->is_dead)
    {
      /* Mark the savefile */
      strcpy(p_ptr->died_from, "Abortion");
      
      /* Close stuff */
      close_game();
      
      /* Quit */
      quit("interrupt");
    }
  
  /* Allow suicide (after 5) */
  else if (signal_count >= 5)
    {
      /* Cause of "death" */
      strcpy(p_ptr->died_from, "Interrupting");
      
      /* Commit suicide */
      p_ptr->is_dead = TRUE;
      
      /* Stop playing */
      p_ptr->playing = FALSE;
      
      /* Leaving */
      p_ptr->leaving = TRUE;
      
      /* Close stuff */
      close_game();
      
      /* Quit */
      quit("interrupt");
    }
  
  /* Give warning (after 4) */
  else if (signal_count >= 4)
    {
      /* Make a noise */
      Term_xtra(TERM_XTRA_NOISE, 0);
      
      /* Clear the top line */
      Term_erase(0, 0, 255);
      
      /* Display the cause */
      Term_putstr(0, 0, -1, TERM_WHITE, "Contemplating suicide!");
      
      /* Flush */
      Term_fresh();
    }
  
  /* Give warning (after 2) */
  else if (signal_count >= 2)
    {
      /* Make a noise */
      Term_xtra(TERM_XTRA_NOISE, 0);
    }
  
  /* Restore handler */
  (void)signal(sig, handle_signal_simple);
}


/**
 * Handle signal -- abort, kill, etc
 */
static void handle_signal_abort(int sig)
{
  /* Disable handler */
  (void)signal(sig, SIG_IGN);
  
  
  /* Nothing to save, just quit */
  if (!character_generated || character_saved) quit(NULL);
  
  
  /* Clear the bottom line */
  Term_erase(0, 23, 255);
  
  /* Give a warning */
  Term_putstr(0, 23, -1, TERM_RED,
	      "A gruesome software bug LEAPS out at you!");
  
  /* Message */
  Term_putstr(45, 23, -1, TERM_RED, "Panic save...");
  
  /* Flush output */
  Term_fresh();
  
  /* Panic Save
  p_ptr->panic_save = 1; */
  
  /* Panic save */
  strcpy(p_ptr->died_from, "(panic save)");
  
  /* Forbid suspend */
  signals_ignore_tstp();
  
  /* Attempt to save */
  if (save_player())
    {
      Term_putstr(45, 23, -1, TERM_RED, "Panic save succeeded!");
    }
  
  /* Save failed */
  else
    {
      Term_putstr(45, 23, -1, TERM_RED, "Panic save failed!");
    }
  
  /* Flush output */
  Term_fresh();
  
  /* Quit */
  quit("software bug");
}




/**
 * Ignore SIGTSTP signals (keyboard suspend)
 */
void signals_ignore_tstp(void)
{

#ifdef SIGTSTP
  (void)signal(SIGTSTP, SIG_IGN);
#endif

}

/**
 * Handle SIGTSTP signals (keyboard suspend)
 */
void signals_handle_tstp(void)
{

#ifdef SIGTSTP
  (void)signal(SIGTSTP, handle_signal_suspend);
#endif

}


/**
 * Prepare to handle the relevant signals
 */
void signals_init(void)
{

#ifdef SIGHUP
  (void)signal(SIGHUP, SIG_IGN);
#endif


#ifdef SIGTSTP
  (void)signal(SIGTSTP, handle_signal_suspend);
#endif


#ifdef SIGINT
  (void)signal(SIGINT, handle_signal_simple);
#endif

#ifdef SIGQUIT
  (void)signal(SIGQUIT, handle_signal_simple);
#endif


#ifdef SIGFPE
  (void)signal(SIGFPE, handle_signal_abort);
#endif
  
#ifdef SIGILL
  (void)signal(SIGILL, handle_signal_abort);
#endif

#ifdef SIGTRAP
  (void)signal(SIGTRAP, handle_signal_abort);
#endif

#ifdef SIGIOT
  (void)signal(SIGIOT, handle_signal_abort);
#endif

#ifdef SIGKILL
  (void)signal(SIGKILL, handle_signal_abort);
#endif

#ifdef SIGBUS
  (void)signal(SIGBUS, handle_signal_abort);
#endif

#ifdef SIGSEGV
  (void)signal(SIGSEGV, handle_signal_abort);
#endif

#ifdef SIGTERM
  (void)signal(SIGTERM, handle_signal_abort);
#endif

#ifdef SIGPIPE
  (void)signal(SIGPIPE, handle_signal_abort);
#endif

#ifdef SIGEMT
  (void)signal(SIGEMT, handle_signal_abort);
#endif

#ifdef SIGDANGER
  (void)signal(SIGDANGER, handle_signal_abort);
#endif

#ifdef SIGSYS
  (void)signal(SIGSYS, handle_signal_abort);
#endif

#ifdef SIGXCPU
  (void)signal(SIGXCPU, handle_signal_abort);
#endif

#ifdef SIGPWR
  (void)signal(SIGPWR, handle_signal_abort);
#endif

}


#else	/* HANDLE_SIGNALS */


/**
 * Do nothing
 */
void signals_ignore_tstp(void)
{
}

/**
 * Do nothing
 */
void signals_handle_tstp(void)
{
}

/**
 * Do nothing
 */
void signals_init(void)
{
}


#endif	/* HANDLE_SIGNALS */


static void write_html_escape_char(ang_file *htm, char c)
{
  switch (c)
    {
    case '<':
      file_putf(htm, "&lt;");
      break;
    case '>':
      file_putf(htm, "&gt;");
      break;
    case '&':
      file_putf(htm, "&amp;");
      break;
    default:
      file_putf(htm, "%c", c);
      break;
    }
}

bool write_char(int row, int col)
{
  if (use_trptile && ((row % 3) || (col % 3) || ((col % 6) && use_bigtile))) 
    return (FALSE);
  if (use_dbltile && ((row % 2) || (col % 2) || ((col % 4) && use_bigtile))) 
    return (FALSE);
  if (use_bigtile && (col % 2)) return (FALSE);
  return (TRUE);
}
      

/**
 * Get the default (ASCII) tile for a given screen location
 */
static void get_default_tile(int row, int col, byte *a_def, char *c_def)
{
  byte a;
  char c;
  
  int wid, hgt;
  int screen_wid, screen_hgt;
  int x, y, col_factor, row_factor;

  col_factor = (use_trptile ? (use_bigtile ? 6 : 3) : 
		(use_dbltile ? (use_bigtile ? 4 : 2) : 
		 (use_bigtile ? 2 : 1)));
  row_factor = (use_trptile ? 3 : (use_dbltile ? 2 : 1));
  
  x = (col - COL_MAP)/col_factor + panel_col_min;
  y = (row - ROW_MAP)/row_factor + panel_row_min;

  /* Retrieve current screen size */
  Term_get_size(&wid, &hgt);
  
  /* Calculate the size of dungeon map area (ignoring bigscreen) */
  screen_wid = wid - (COL_MAP + 1);
  screen_hgt = hgt - (ROW_MAP + 1);
  
  /* Get the tile from the screen */
  a = Term->scr->a[row][col];
  c = Term->scr->c[row][col];
  
  /* Convert the map display to the default characters */
  if (!character_icky &&
      ((col - COL_MAP) >= 0) && ((col - COL_MAP) < SCREEN_WID * col_factor) &&
      ((row - ROW_MAP) >= 0) && ((row - ROW_MAP) < SCREEN_HGT * row_factor))
    {
      /* Convert dungeon map into default attr/chars */
      if (in_bounds(y, x) && write_char(row - ROW_MAP, col - COL_MAP))
	{
	  /* Retrieve default attr/char */
	  map_info_default(y, x, &a, &c);
	}
      else
	{
	  /* "Out of bounds" is empty */
	  a = TERM_WHITE;
	  c = ' ';
	}
      
      if (c == '\0') c = ' ';
    }
  
  /* Filter out remaining graphics */
  if (a & 0x80)
    {
      /* Replace with "white space" */
      a = TERM_WHITE;
      c = ' ';
    }
  
  /* Return the default tile */
  *a_def = a;
  *c_def = c;
}



/**
 * Take an html screenshot 
 */
void html_screenshot(cptr name)
{
  int y, x;
  int wid, hgt;
  
  byte a = TERM_WHITE;
  byte oa = TERM_WHITE;
  char c = ' ';
  
  ang_file *htm;
  
  char buf[1024];
  
  /* Build the filename */
  path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
  
  /* Append to the file */
  htm = file_open(buf, MODE_WRITE, FTYPE_TEXT);

  /* Oops */
  if (!htm)
    {
      plog_fmt("Cannot write the '%s' file!", buf);
      return;
    }
  
  /* Retrieve current screen size */
  Term_get_size(&wid, &hgt);
  
  file_putf(htm, "<HTML>\n");
  file_putf(htm, "<HEAD>\n");
  file_putf(htm, "<META NAME=\"GENERATOR\" Content=\"FAAngband %s\">\n", 
	  VERSION_STRING);
  file_putf(htm, "<TITLE>%s</TITLE>\n", name);
  file_putf(htm, "</HEAD>\n");
  file_putf(htm, "<BODY TEXT=\"#FFFFFF\" BGCOLOR=\"#000000\">");
  file_putf(htm, "<FONT COLOR=\"#%02X%02X%02X\">\n<PRE><TT>",
	  angband_color_table[TERM_WHITE][1],
	  angband_color_table[TERM_WHITE][2],
	  angband_color_table[TERM_WHITE][3]);
  
  /* Dump the screen */
  for (y = 0; y < hgt; y++)
    {
      for (x = 0; x < wid; x++)
	{
	  
	  /* Get the ASCII tile */
	  get_default_tile(y, x, &a, &c);
	  
	  /* Color change */
	  if (oa != a)
	    {
	      /* From the default white to another color */
	      if (oa == TERM_WHITE)
		{
		  file_putf(htm, "<FONT COLOR=\"#%02X%02X%02X\">",
			  angband_color_table[a][1],
			  angband_color_table[a][2],
			  angband_color_table[a][3]);
		}
	      /* From another color to the default white */
	      else if (a == TERM_WHITE)
		{
		  file_putf(htm, "</FONT>");
		}
	      /* Change colors */
	      else
		{
		  file_putf(htm, "</FONT><FONT COLOR=\"#%02X%02X%02X\">",
			  angband_color_table[a][1],
			  angband_color_table[a][2],
			  angband_color_table[a][3]);
		}
	      
	      /* Remember the last color */
	      oa = a;
	    }
	  
	  /* Write the character and escape special HTML characters */
	  write_html_escape_char(htm, c);
	}
      
      /* End the row */
      file_putf(htm, "\n");
    }
  
  /* Close the last <font> tag if necessary */
  if (a != TERM_WHITE) file_putf(htm, "</FONT>");
  
  file_putf(htm, "</TT></PRE>\n");
  
  file_putf(htm, "</BODY>\n");
  file_putf(htm, "</HTML>\n");
  
  /* Close it */
  file_close(htm);
}
