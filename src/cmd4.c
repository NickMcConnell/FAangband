/** \file cmd4.c
    \brief Commands, part 4

 * Various kinds of browsing functions.
 *
 * Copyright (c) 1997-2007 Robert A. Koeneke, James E. Wilson, Ben Harrison,
 * Eytan Zweig, Andrew Doull, Pete Mack.
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
#include "files.h"
#include "history.h"
#include "prefs.h"
#include "ui.h"


/**
 * Hack -- redraw the screen
 *
 * This command performs various low level updates, clears all the "extra"
 * windows, does a total redraw of the main window, and requests all of the
 * interesting updates and redraws that I can think of.
 *
 * This command is also used to "instantiate" the results of the user
 * selecting various things, such as graphics mode, so it must call
 * the "TERM_XTRA_REACT" hook before redrawing the windows.
 *
 */
void do_cmd_redraw(void)
{
  int j;
  
  term *old = Term;
  
  
  /* Low level flush */
  Term_flush();
  
  /* Reset "inkey()" */
  flush();
  
  if (character_dungeon)
    verify_panel();
  
  
  /* Hack -- React to changes */
  Term_xtra(TERM_XTRA_REACT, 0);
  
  
  /* Combine and Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  
  /* Update torch */
  p_ptr->update |= (PU_TORCH);
  
  /* Update stuff */
  p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
  
  /* Fully update the visuals */
  p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
  
  /* Redraw everything */
  p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP | PR_INVEN | PR_EQUIP |
		    PR_MESSAGE | PR_MONSTER | PR_OBJECT |
		    PR_MONLIST | PR_ITEMLIST);
  
  /* Clear screen */
  Term_clear();

  /* Hack -- update */
  handle_stuff(p_ptr);
  
  /* Place the cursor on the player */
  if (0 != character_dungeon)
    move_cursor_relative(p_ptr->px, p_ptr->py);
  
  
  /* Redraw every window */
  for (j = 0; j < ANGBAND_TERM_MAX; j++)
    {
      /* Dead window */
      if (!angband_term[j]) continue;
      
      /* Activate */
      Term_activate(angband_term[j]);
      
      /* Redraw */
      Term_redraw();
      
      /* Refresh */
      Term_fresh();
      
      /* Restore */
      Term_activate(old);
    }
}


/**
 * Redraw a term when it is resized
 */
void redraw_window(void)
{
  /* Only if the dungeon exists */
  if (!character_dungeon) return;
  
  /* Hack - Activate term zero for the redraw */
  Term_activate(&term_screen[0]);
  
  /* Hack -- react to changes */
  Term_xtra(TERM_XTRA_REACT, 0);
  
  /* Hack -- update */
  handle_stuff(p_ptr);
  
  /* Redraw */
  Term_redraw();
  
  /* Refresh */
  Term_fresh();
}

/**
 * Hack -- change name
 */
void do_cmd_change_name(void)
{
  ui_event ke;
  
  int col = 0;
  int last_line = 0;
  int top_line = 0;

  const char *p;

  /* Prompt */
  p = "['c' change name, 'f' to file, scroll, or ESC]";
  
  /* Save screen */
  screen_save();

  /* Adjust the buttons */
  button_backup_all();
  button_kill_all();
  button_add("ESC", ESCAPE);
  button_add("Spc", ' ');
  button_add("-", '-');
  button_add("c", 'c');
  button_add("f", 'f');
  button_add("->", ARROW_RIGHT);
  button_add("<-", ARROW_LEFT);
  p_ptr->redraw |= PR_BUTTONS;

  /* Make the array of lines */
  C_WIPE(dumpline, DUMP_MAX_LINES, char_attr_line);
  last_line = make_dump(dumpline, 2);

  /* Forever */
  while (1)
    {
      /* Display the player */
      display_dump(dumpline, top_line, top_line + Term->hgt - 1, col);

      redraw_stuff(p_ptr);

      /* Clear the bottom line */
      prt("", Term->hgt - 1, 0);
      
      /* Prompt */
      Term_putstr(0, Term->hgt - 1, -1, TERM_WHITE, p);
     
      /* Query */
      ke = inkey_ex();
      
      /* Exit */
      if (ke.key.code == ESCAPE) break;
      
      /* Change name */
      if (ke.key.code == 'c')
        {
	  char namebuf[32] = "";

	  if (get_name(namebuf, sizeof namebuf))
	  {
	      /* Set player name */
	      my_strcpy(op_ptr->full_name, namebuf,
			sizeof(op_ptr->full_name));
	      
	      /* Don't change savefile name. */
	      process_player_name(FALSE);
	  }
	  //(void) get_name(namebuf, sizeof namebuf);
	  (void) make_dump(dumpline, 2);
        }
      
      /* File dump */
      else if (ke.key.code == 'f')
	{
	  char ftmp[80];
	  
	  strnfmt(ftmp, sizeof ftmp, "%s.txt", op_ptr->base_name);
	  
	  if (get_string("File name: ", ftmp, 80))
	    {
	      if (ftmp[0] && (ftmp[0] != ' '))
		{
		  if (file_character(ftmp, dumpline, last_line))
		    msg("Character dump failed!");
		  else
		    msg("Character dump successful.");
		}
	    }
	}
      
      /* Scroll down */
      else if (ke.key.code == ARROW_DOWN)
	{
	  if (top_line + Term->hgt - 2 < last_line)
	    top_line++;
	}
      
      /* Page down */
      else if (ke.key.code == ' ')
	{
	  top_line = MIN(last_line - Term->hgt + 2, 
			 top_line + (Term->hgt - 2));
	}
      
      /* Scroll up */
      else if (ke.key.code == ARROW_UP)
	{
	  if (top_line)
	    top_line--;
	}
      
      /* Page up */
      else if (ke.key.code == '-')
	{
	  top_line -= (Term->hgt - 2) / 2;
	  if (top_line < 0) top_line = 0;
	}
      
      /* Scroll left */
      else if (ke.key.code == ARROW_LEFT)
	{
	  if (col)
	    col--;
	}
      
      /* Scroll right */
      else if (ke.key.code == ARROW_RIGHT)
	{
	  if (col < 32)
	    col++;
	}
      
      
      /* Oops */
      else
	{
	  bell(NULL);
	}
      
      /* Flush messages */
      message_flush();
    }

  /* Adjust the buttons */
  button_restore();

  /* Load screen */
  screen_load();
}


/**
 * Recall the most recent message
 */
void do_cmd_message_one(void)
{
  /* Recall one message XXX XXX XXX */
  c_prt(message_color(0), format( "> %s", message_str(0)), 0, 0);
}


/**
 * Show previous messages to the user
 *
 * The screen format uses line 0 and 23 for headers and prompts,
 * skips line 1 and 22, and uses line 2 thru 21 for old messages.
 *
 * This command shows you which commands you are viewing, and allows
 * you to "search" for strings in the recall.
 *
 * Note that messages may be longer than 80 characters, but they are
 * displayed using "infinite" length, with a special sub-command to
 * "slide" the virtual display to the left or right.
 *
 * Attempt to only hilight the matching portions of the string.
 */
void do_cmd_messages(void)
{
  ui_event ke;
  
  int i, j, n, q;
  int wid, hgt;
  
  char shower[80];
  char finder[80];
  char p[80];  
  
  /* Wipe finder */
  my_strcpy(finder, "", sizeof(shower));
  
  /* Wipe shower */
  my_strcpy(shower, "", sizeof(finder));
  
  
  /* Total messages */
  n = messages_num();
  
  /* Start on first message */
  i = 0;
  
  /* Start at leftmost edge */
  q = 0;
  
  /* Get size */
  Term_get_size(&wid, &hgt);
  
  /* Prompt */
  strncpy(p, "[Press 'p' for older, 'n' for newer, ..., or ESCAPE]", 80);

  /* Save screen */
  screen_save();
  
  /* Adjust the buttons */
  button_backup_all();
  button_kill_all();
  button_add("ESC", ESCAPE);
  button_add("-", '-');
  button_add("=", '=');
  button_add("/", '/');
  button_add("p", 'p');
  button_add("n", 'n');
  button_add("+", '+');
  button_add("->", '6');
  button_add("<-", '4');
  p_ptr->redraw |= (PR_BUTTONS);

  /* Process requests until done */
  while (1)
    {
      /* Clear screen */
      Term_clear();
      
      /* Dump messages */
      for (j = 0; (j < hgt - 4) && (i + j < n); j++)
	{
	  const char *msg = message_str((s16b)(i+j));
	  byte attr = message_color((s16b)(i+j));
	  
	  /* Apply horizontal scroll */
	  msg = ((int)strlen(msg) >= q) ? (msg + q) : "";
	  
	  /* Dump the messages, bottom to top */
	  Term_putstr(0, hgt - 3 - j, -1, attr, msg);
	  
	  /* Hilight "shower" */
	  if (shower[0])
	    {
	      const char *str = msg;
	      
	      /* Display matches */
	      while ((str = strstr(str, shower)) != NULL)
		{
		  int len = strlen(shower);
		  
		  /* Display the match */
		  Term_putstr(str-msg, hgt - 3 - j, len, TERM_YELLOW, shower);
		  
		  /* Advance */
		  str += len;
		}
	    }
	}
      
      /* Display header XXX XXX XXX */
      prt(format("Message Recall (%d-%d of %d), Offset %d",
		 i, i + j - 1, n, q), 0, 0);
      
      /* Display prompt (not very informative) */
      prt(p, hgt - 1, 0);
      redraw_stuff(p_ptr);
      
      /* Get a command */
      ke = inkey_ex();
      
      /* Exit on Escape */
      if (ke.key.code == ESCAPE) break;
      
      /* Hack -- Save the old index */
      j = i;
      
      /* Horizontal scroll */
      if (ke.key.code == '4')
	{
	  /* Scroll left */
	  q = (q >= wid / 2) ? (q - wid / 2) : 0;
	  
	  /* Success */
	  continue;
	}
      
      /* Horizontal scroll */
      if (ke.key.code == '6')
	{
	  /* Scroll right */
	  q = q + wid / 2;
	  
	  /* Success */
	  continue;
	}
      
      /* Hack -- handle show */
      if (ke.key.code == '=')
	{
	  /* Prompt */
	  prt("Show: ", hgt - 1, 0);
	  
	  /* Get a "shower" string, or continue */
	  if (!askfor_aux(shower, sizeof shower, NULL)) continue;
	  
	  /* Okay */
	  continue;
	}
      
      /* Hack -- handle find */
      if (ke.key.code == '/')
	{
	  s16b z;
	  
	  /* Prompt */
	  prt("Find: ", hgt - 1, 0);
	  
	  /* Get a "finder" string, or continue */
	  if (!askfor_aux(finder, sizeof finder, NULL)) continue;
	  
	  /* Show it */
	  my_strcpy(shower, finder, sizeof(shower));
	  
	  /* Scan messages */
	  for (z = i + 1; z < n; z++)
	    {
	      const char *msg = message_str(z);
	      
	      /* Search for it */
	      if (strstr(msg, finder))
		{
		  /* New location */
		  i = z;
		  
		  /* Done */
		  break;
		}
	    }
	}
      
      /* Recall 20 older messages */
      if ((ke.key.code == 'p') || (ke.key.code == KTRL('P')) || (ke.key.code == ' '))
	{
	  /* Go older if legal */
	  if (i + 20 < n) i += 20;
	}
      
      /* Recall 10 older messages */
      if (ke.key.code == '+')
	{
	  /* Go older if legal */
	  if (i + 10 < n) i += 10;
	}
      
      /* Recall 1 older message */
      if ((ke.key.code == '8') || (ke.key.code == KC_ENTER))
	{
	  /* Go older if legal */
	  if (i + 1 < n) i += 1;
	}
      
      /* Recall 20 newer messages */
      if ((ke.key.code == 'n') || (ke.key.code == KTRL('N')))
	{
	  /* Go newer (if able) */
	  i = (i >= 20) ? (i - 20) : 0;
	}
      
      /* Recall 10 newer messages */
      if (ke.key.code == '-')
	{
	  /* Go newer (if able) */
	  i = (i >= 10) ? (i - 10) : 0;
	}
      
      /* Recall 1 newer messages */
      if (ke.key.code == '2')
	{
	  /* Go newer (if able) */
	  i = (i >= 1) ? (i - 1) : 0;
	}
      
      /* Scroll forwards or backwards using mouse clicks */
      if (ke.mouse.button)
      {
	  if (ke.mouse.y <= hgt / 2)
	  {
	      /* Go older if legal */
	      if (i + 20 < n) i += 20;
	  }
	  else
	  {
	      /* Go newer (if able) */
	      i = (i >= 20) ? (i - 20) : 0;
	  }
      }
      
      /* Hack -- Error of some kind */
      if (i == j) bell(NULL);
    }
  
  /* Adjust the buttons */
  button_restore();

  /* Load screen */
  screen_load();
}



/**
 * Ask for a "user pref line" and process it
 */
void do_cmd_pref(void)
{
  char tmp[80];
  
  /* Default */
  my_strcpy(tmp, "", sizeof(tmp));
  
  /* Ask for a "user pref command" */
  if (!get_string("Pref: ", tmp, 80)) return;
  
  /* Process that pref command */
  (void)process_pref_file_command(tmp);
}


/**
 * Note something in the message recall or character notes file.  Lifted
 * from NPPangband, patch originally by Chris Kern.
 */
void do_cmd_note(void)
{
    char tmp[80];

    /* Default */
    my_strcpy(tmp, "", sizeof(tmp));

    /* Input */
    if (!get_string("Note: ", tmp, 80)) return;

    /* Ignore empty notes */
    if (!tmp[0] || (tmp[0] == ' ')) return;

    /* Add the note to the message recall */
    msg("Note: %s", tmp);

    /* Add a history entry */
    history_add(tmp, HISTORY_USER_INPUT, 0);
}
  
  

/**
 * Mention the current version
 */
void do_cmd_version(void)
{
    /* Silly message */
    msg("You are playing %s %s.  Type '?' for more info.",
	       VERSION_NAME, VERSION_STRING);
}



/*
 * Array of feeling strings
 */
static const char *feeling_text[] =
{
  "Looks like any other level.",
  "You feel there is something special about this level.",
  "You have a superb feeling about this level.",
  "You have an excellent feeling...",
  "You have a very good feeling...",
  "You have a good feeling...",
  "You feel strangely lucky...",
  "You feel your luck is turning...",
  "You like the look of this place...",
  "This level can't be all bad...",
  "What a boring place..."
};


/**
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 */
void do_cmd_feeling(void)
{
  /* Verify the feeling */
  if (feeling >= N_ELEMENTS(feeling_text))
    feeling = N_ELEMENTS(feeling_text) - 1;
  
  /* No useful feeling in town */
  if (!p_ptr->depth)
    {
      msg("Looks like a typical town.");
      return;
    }
  
  /* No useful feelings until enough time has passed */
  if (!do_feeling)
    {
      msg("You are still uncertain about this level...");
      return;
    }
  
  /* Display the feeling */
    if (p_ptr->themed_level) msg("%s", themed_feeling);
    else msg(feeling_text[feeling]);
}

/*
 * Array of feeling strings
 */
static const char *do_cmd_challenge_text[14] =
{
  "challenges you from beyond the grave!",
  "thunders 'Prove worthy of your traditions - or die ashamed!'.",
  "desires to test your mettle!",
  "has risen from the dead to test you!",
  "roars 'Fight, or know yourself for a coward!'.",
  "summons you to a duel of life and death!",
  "desires you to know that you face a mighty champion of yore!",
  "demands that you prove your worthiness in combat!",
  "calls you unworthy of your ancestors!",
  "challenges you to a deathmatch!",
  "walks Middle-Earth once more!",
  "challenges you to demonstrate your prowess!",
  "demands you prove yourself here and now!",
  "asks 'Can ye face the best of those who came before?'."
};




/** 
 * Personalize, randomize, and announce the challenge of a player ghost. -LM-
 */
void ghost_challenge(void)
{
    monster_race *r_ptr = &r_info[r_ghost];
    
    msg("%s, the %s %s", ghost_name, r_ptr->name, 
	       do_cmd_challenge_text[randint0(14)]);
}



/**
 * Encode the screen colors
 */
static const char hack[BASIC_COLORS+1] = "dwsorgbuDWvyRGBU";


/**
 * Hack -- load a screen dump from a file
 *
 * ToDo: Add support for loading/saving screen-dumps with graphics
 * and pseudo-graphics.  Allow the player to specify the filename
 * of the dump.
 */
void do_cmd_load_screen(void)
{
  int i, y, x;
  
  int a = 0;
  wchar_t c = L' ';
  
  bool okay = TRUE;
  
  ang_file *fp;
  
  char buf[1024];
  

  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, "dump.txt");
  
  /* Open the file */
  fp = file_open(buf, MODE_READ, -1);

  /* Oops */
  if (!fp) return;
  

  /* Save screen */
  screen_save();

  
  /* Clear the screen */
  Term_clear();
  
  
  /* Load the screen */
  for (y = 0; okay && (y < 24); y++)
    {
      /* Get a line of data */
      if (!file_getl(fp, buf, sizeof(buf))) okay = FALSE;
      
      
      /* Show each row */
      for (x = 0; x < 79; x++)
	{
	    Term_mbstowcs(&c, &buf[x], 1);
	  /* Put the attr/char */
	  Term_draw(x, y, TERM_WHITE, buf[x]);
	}
    }
  
  /* Get the blank line */
  if (!file_getl(fp, buf, sizeof(buf))) okay = FALSE;
  

  /* Dump the screen */
  for (y = 0; okay && (y < 24); y++)
    {
      /* Get a line of data */
      if (!file_getl(fp, buf, sizeof(buf))) okay = FALSE;
      
      /* Dump each row */
      for (x = 0; x < 79; x++)
	{
	  /* Get the attr/char */
	  (void)(Term_what(x, y, &a, &c));

	  /* Look up the attr */
	  for (i = 0; i < BASIC_COLORS; i++)
	    {
	      /* Use attr matches */
	      if (hack[i] == buf[x]) a = i;
	    }
	  
	  /* Put the attr/char */
	  Term_draw(x, y, a, c);
	}
    }
  
  
  /* Close it */
  file_close(fp);
  

  /* Message */
  msg("Screen dump loaded.");
  message_flush();
  
  
  /* Load screen */
  screen_load();
}

void do_cmd_save_screen_text(void)
{
  int y, x;
  
  int a = 0;
  wchar_t c = L' ';
  
  ang_file *fff;
  
  char buf[1024];
  char *p;
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_USER, "dump.txt");
  
  /* Append to the file */
  fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);
  
  /* Oops */
  if (!fff) return;
  
  
  /* Save screen */
  screen_save();


  /* Dump the screen */
  for (y = 0; y < 24; y++)
    {
	p = buf;
      /* Dump each row */
      for (x = 0; x < 79; x++)
	{
	  /* Get the attr/char */
	  (void)(Term_what(x, y, &a, &c));
	  
	  /* Dump it */
	  p += wctomb(p, c);
	}
      
      /* Terminate */
      *p = '\0';
      
      /* End the row */
      file_putf(fff, "%s\n", buf);
    }
  
  /* Skip a line */
  file_putf(fff, "\n");
  
  
  /* Dump the screen */
  for (y = 0; y < 24; y++)
    {
      /* Dump each row */
      for (x = 0; x < 79; x++)
	{
	  /* Get the attr/char */
	  (void)(Term_what(x, y, &a, &c));
	  
	  /* Dump it */
	  buf[x] = hack[a & 0x0F];
	}
      
      /* Terminate */
      buf[x] = '\0';
      
      /* End the row */
      file_putf(fff, "%s\n", buf);
    }
  
  /* Skip a line */
  file_putf(fff, "\n");
  
  
  /* Close it */
  file_close(fff);
  
  
  /* Message */
  msg("Screen dump saved.");
  message_flush();
  
  
  /* Load screen */
  screen_load();
}


/**
 * Hack -- save a screen dump to a file in html format
 */
void do_cmd_save_screen_html(int mode)
{
    size_t i;
  
    ang_file *fff;
    char file_name[1024];
    char tmp_val[256];
  
    typedef void (*dump_func)(ang_file *);
    dump_func dump_visuals [] = 
	{ dump_monsters, dump_features, dump_objects, dump_flavors, dump_colors };
  
    /* Ask for a file */
    if (mode == 0) my_strcpy(tmp_val, "dump.html", sizeof(tmp_val));
    else my_strcpy(tmp_val, "dump.txt", sizeof(tmp_val));
    if (!get_string("File: ", tmp_val, sizeof(tmp_val))) return;
  
    /* Save current preferences */
    path_build(file_name, 1024, ANGBAND_DIR_USER, "dump.prf");
    fff = file_open(file_name, MODE_WRITE, (mode == 0 ? FTYPE_HTML : FTYPE_TEXT));
  
    /* Check for failure */
    if (!fff)
    {
	msg("Screen dump failed.");
	message_flush();
	return;
    }
  
    /* Dump all the visuals */
    for (i = 0; i < N_ELEMENTS(dump_visuals); i++)
	dump_visuals[i](fff);
  
    file_close(fff);
  
    /* Dump the screen with raw character attributes */
    reset_visuals(FALSE);
    do_cmd_redraw();
    html_screenshot(tmp_val, mode);
  
    /* Recover current graphics settings */
    reset_visuals(TRUE);
    process_pref_file(file_name, TRUE, FALSE);
    file_delete(file_name);
    do_cmd_redraw();
  
    msg("HTML screen dump saved.");
    message_flush();
}


/**
 * Hack -- save a screen dump to a file
 */
void do_cmd_save_screen(void)
{
    ui_event ke;
    msg("Dump type [(t)ext; (h)tml; (f)orum embedded html]:");
    button_add("f", 'f');
    button_add("h", 'h');
    button_add("t", 't');
    ke = inkey_ex();
    switch(ke.key.code) 
    {
    case ESCAPE:
	break;
    case 't': do_cmd_save_screen_text();
	break;
    case 'h': do_cmd_save_screen_html(0);
	break;
    case 'f': do_cmd_save_screen_html(1);
	break;
    }
    button_kill('f');
    button_kill('t');
    button_kill('h');
    message_flush();
}

/**
 * Display the time and date
 */
void do_cmd_time(void)
{
  s32b len = 10L * TOWN_DAWN;
  s32b tick = turn % len + len / 4;
  
  int day = turn / len + 1;
  int hour = (24 * tick / len) % 24;
  int min = (1440 * tick / len) % 60;
  
  
  /* Message */
  msg("This is day %d. The time is %d:%02d %s.", day,
             (hour % 12 == 0) ? 12 : (hour % 12), min,
             (hour < 12) ? "AM" : "PM");
}



