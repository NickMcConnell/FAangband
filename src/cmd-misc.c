#include "angband.h"
#include "cmds.h"

/**
 * Divide up the screen into mousepress regions 
 */

int click_area(ui_event_data ke)
{

  if ((ke.mousey) && (ke.mousex > COL_MAP) && 
      (ke.mousey < Term->hgt - (panel_extra_rows ? 2 : 0) - 
       (OPT(bottom_status) ? 6 : 0) - 1))
    return MOUSE_MAP;
  else if ((ke.mousey >= ROW_RACE) && (ke.mousey < ROW_RACE + 6) && 
	   (ke.mousex < 12))
    return MOUSE_CHAR;
  else if ((ke.mousey == ROW_HP) && (ke.mousex > COL_HP) && 
	   (ke.mousex < COL_HP + 12))
    return MOUSE_HP;
  else if ((ke.mousey == ROW_SP) && (ke.mousex > COL_SP) &&
	   (ke.mousex < COL_SP + 12))
    return MOUSE_SP;
  else if ((ke.mousey == Term->hgt - 1) && (ke.mousex >= COL_STUDY) &&
	   (ke.mousex < COL_STUDY + 5))
    return MOUSE_STUDY;
  else if (!ke.mousey)
    return MOUSE_MESSAGE;
  else if ((ke.mousey == Term->hgt - 1) && 
	   (ke.mousex > Term->wid - (small_screen ? 8 : 21)))
    return MOUSE_PLACE;
  else if ((ke.mousey >= ROW_STAT) && (ke.mousey < ROW_STAT + 6) && 
	   (ke.mousex >= COL_STAT) && (ke.mousex < COL_STAT + 12))
    return MOUSE_OBJECTS;
  else if ((ke.mousey == ROW_STAND) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + (OPT(bottom_status) ? 6 : 7)))
    return MOUSE_STAND;
  else if ((ke.mousey == ROW_REPEAT) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + (OPT(bottom_status) ? 6 : 8)))
    return MOUSE_REPEAT;
  else if ((ke.mousey == ROW_RETURN) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + (OPT(bottom_status) ? 6 : 8 )))
    return MOUSE_RETURN;
  else if ((ke.mousey == ROW_ESCAPE) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + 5))
    return MOUSE_ESCAPE;
  else return MOUSE_NULL;
}

/**
 * Return the features around (or under) the character
 */
void get_feats(int *surroundings)
{
  int d;
  int xx, yy;
  int count;
  
  /* Count how many matches */
  count = 0;
  
  /* Check around (and under) the character */
  for (d = 0; d < 9; d++)
    {
      /* Initialise */
      surroundings[d] = FEAT_FLOOR;

      /* Extract adjacent (legal) location */
      yy = p_ptr->py + ddy_ddd[d];
      xx = p_ptr->px + ddx_ddd[d];
      
      /* Paranoia */
      if (!in_bounds_fully(yy, xx)) continue;
      
      /* Must have knowledge */
      if (!(cave_info[yy][xx] & (CAVE_MARK))) continue;
      
      /* Record the feature */
      surroundings[d] = cave_feat[yy][xx];
    }
  
  /* All done */
  return;
}


/**
 * Bring up player actions 
 */
void show_player(void)
{
  int i, j, fy, fx, tile_hgt;
  int adj_grid[9];
  bool exist_rock_or_web = FALSE;
  bool exist_door = FALSE;
  bool exist_open_door = FALSE;
  bool exist_trap = FALSE;
  bool exist_mtrap = FALSE;
  bool exist_floor = FALSE;
  bool exist_monster = FALSE;

  /* Set to display list */
  p_ptr->command_see = TRUE;

  /* No command */
  p_ptr->command_new = 0;

  /* No commands yet */
  poss = 0;

  /* Get surroundings */
  get_feats(adj_grid);

  /* Analyze surroundings */
  for (i = 0; i < 8; i++)
    {
      if ((adj_grid[i] >= FEAT_TRAP_HEAD) && (adj_grid[i] <= FEAT_TRAP_TAIL))
	exist_trap = TRUE;
      if ((adj_grid[i] >= FEAT_DOOR_HEAD) && (adj_grid[i] <= FEAT_DOOR_TAIL))
	exist_door = TRUE;
      if ((adj_grid[i] == FEAT_WEB) || ((adj_grid[i] >= FEAT_SECRET) &&
					(adj_grid[i] <= FEAT_PERM_SOLID)))
	exist_rock_or_web = TRUE;
      if ((adj_grid[i] >= FEAT_MTRAP_HEAD) && (adj_grid[i] <= FEAT_MTRAP_TAIL))
	exist_mtrap = TRUE;
      if (adj_grid[i] == FEAT_OPEN)
	exist_open_door = TRUE;
      if (cave_naked_bold(p_ptr->py + ddy_ddd[i], p_ptr->px + ddx_ddd[i]))
	exist_floor = TRUE;
      if (cave_m_idx[ddy_ddd[i]][ddx_ddd[i]] > 0) 
	exist_monster = TRUE;
    }

  /* In a web? */
  if (adj_grid[8] == FEAT_WEB) exist_rock_or_web = TRUE;
  
  /* Alter a grid */
  if (exist_trap || exist_door || exist_rock_or_web || exist_mtrap || 
      exist_open_door || count_chests(&fy, &fx, TRUE) || 
      count_chests(&fy, &fx, FALSE) || (check_ability(SP_TRAP) && exist_floor)
      || (check_ability(SP_STEAL) && exist_monster && (!SCHANGE)))
    {
      comm[poss] = '+';
      comm_descr[poss++] = "Alter";
    }
  
  /* Dig a tunnel */
  if (exist_door || exist_rock_or_web)
    {
      comm[poss] = 'T';
      comm_descr[poss++] = "Tunnel";
    }
      
  /* Begin Running -- Arg is Max Distance */
  {
    comm[poss] = '.';
    comm_descr[poss++] = "Run";
  }
      
  /* Hold still for a turn.  Pickup objects if auto-pickup is true. */
  {
    comm[poss] = ',';
    comm_descr[poss++] = "Stand still";
  }
      
  /* Pick up objects. */
  if (cave_o_idx[p_ptr->py][p_ptr->px])
  {
    comm[poss] = 'g';
    comm_descr[poss++] = "Pick up";
  }
      
  /* Rest -- Arg is time */
  {
    comm[poss] = 'R';
    comm_descr[poss++] = "Rest";
  }
      
  /* Search for traps/doors */
  {
    comm[poss] = 's';
    comm_descr[poss++] = "Search";
  }
      
  /* Look around */
  {
    comm[poss] = 'l';
    comm_descr[poss++] = "Look";
  }
      
  /* Scroll the map */
  {
    comm[poss] = 'L';
    comm_descr[poss++] = "Scroll map";
  }
      
  /* Show the map */
  {
    comm[poss] = 'M';
    comm_descr[poss++] = "Level map";
  }
      
  /* Knowledge */
  {
    comm[poss] = '~';
    comm_descr[poss++] = "Knowledge";
  }
      
  /* Options */
  {
    comm[poss] = '=';
    comm_descr[poss++] = "Options";
  }
      
  /* Toggle search mode */
  {
    comm[poss] = 'S';
    comm_descr[poss++] = "Toggle searching";
  }
      
  
  /* Go up staircase */
  if ((adj_grid[8] == FEAT_LESS) || ((adj_grid[8] >= FEAT_LESS_NORTH) && 
				     (!(adj_grid[8] % 2))))
  {
    comm[poss] = '<';
    comm_descr[poss++] = "Take stair/path";
  }
      
  /* Go down staircase */
  if ((adj_grid[8] == FEAT_MORE) || ((adj_grid[8] >= FEAT_LESS_NORTH) && 
				     (adj_grid[8] % 2)))
  {
    comm[poss] = '>';
    comm_descr[poss++] = "Take stair/path";
  }
      
  /* Open a door or chest */
  if (exist_door || count_chests(&fy, &fx, TRUE) || 
      count_chests(&fy, &fx, FALSE))
    {
      comm[poss] = 'o';
      comm_descr[poss++] = "Open";
    }
  
  /* Close a door */
  if (exist_open_door)
    {
      comm[poss] = 'c';
      comm_descr[poss++] = "Close";
    }
      
    /* Jam a door with spikes */
  if (exist_door)
    {
      comm[poss] = 'j';
      comm_descr[poss++] = "Jam";
    }
      
  /* Bash a door */
  if (exist_door)
    {
      comm[poss] = 'B';
      comm_descr[poss++] = "Bash";
    }
      
  /* Disarm a trap or chest */
  if (count_chests(&fy, &fx, TRUE) || exist_trap || exist_mtrap)
    {
      comm[poss] = 'D';
      comm_descr[poss++] = "Disarm";
    }

  /* Shapechange */
  if ((SCHANGE) || (check_ability(SP_BEARSKIN))) 
    {
      comm[poss] = ']';
      comm_descr[poss++] = "Shapechange";
    }
      
  /* Save screen */
  screen_save();

  /* Prompt */
  put_str("Choose a command, or ESC:", 0, 0); 
  
  /* Hack - delete exact graphics rows */
  if (use_trptile || use_dbltile)
    { 
      j = poss + 1;
      tile_hgt = (use_trptile ? 3 : 2);
      while ((j % tile_hgt) && (j <= SCREEN_ROWS)) 
	prt("", ++j, 0);
    }

  /* Get a choice */
  show_cmd_menu(FALSE);

  /* Load screen */
  screen_load();

  if (p_ptr->command_new) repeat_push(p_ptr->command_new);
  
  /* Turn off lists */
  p_ptr->command_see = FALSE;

}    


/**
 * Bring up objects to act on 
 */
void do_cmd_show_obj(void)
{
  cptr q, s;
  int j, item, tile_hgt;
  object_type *o_ptr;

  char o_name[120];
  byte out_color;
  
  /* Set to display list */
  p_ptr->command_see = TRUE;

  /* No item */
  p_ptr->command_item = 0;

  /* No command */
  p_ptr->command_new = 0;

  /* No restrictions */
  item_tester_tval = 0;
  item_tester_hook = NULL;

  /* See what's available */
  q = "Pick an item to use:";
  s = "You have no items to hand.";
  if (!get_item(&item, q, s, (USE_INVEN | USE_EQUIP | USE_FLOOR))) return;

  /* Got it */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  /* Is it really an item? */
  if (!o_ptr->k_idx)
    return;

  /* No commands yet */
  poss = 0;

  /* Describe the object */
  object_desc(o_name, o_ptr, TRUE, 4);
      
  /* Hack -- enforce max length */
  o_name[Term->wid - 3] = '\0';
      
  /* Acquire inventory color.  Apply spellbook hack. */
  out_color = proc_list_color_hack(o_ptr);
      
  /* Wear/wield */
  if ((wield_slot(o_ptr) >= INVEN_WIELD) && (!SCHANGE) && (item < INVEN_WIELD))
    {
      comm[poss] = 'w';
      comm_descr[poss++] = "Wield";
    }
      
  /* Take off equipment */
  if ((item >= INVEN_WIELD) && (item < INVEN_TOTAL) && (!SCHANGE))
    {
      comm[poss] = 't';
      comm_descr[poss++] = "Take off";
    }
      
  /* Drop an item */
  if ((item >= 0) && ((item < INVEN_WIELD) || (!SCHANGE)))
    {
      comm[poss] = 'd';
      comm_descr[poss++] = "Drop";
    }
      
      
  /* Destroy an item */
  if (item < INVEN_WIELD)
    {
      comm[poss] = 'k';
      comm_descr[poss++] = "Destroy";
    }
      
      
  /* Identify an object */
  comm[poss] = 'I';
  comm_descr[poss++] = "Inspect";
  
  /* Pick up an object */
  if ((item < 0) && inven_carry_okay(o_ptr))
    {
      comm[poss] = 'g';
      comm_descr[poss++] = "Pick up";
    }

  /* Book learnin' */
  if (mp_ptr->spell_book == o_ptr->tval)
    {
      if (p_ptr->new_spells)
	{
	  comm[poss] = 'G';
	  comm_descr[poss++] = "Gain a spell";
	}
      comm[poss] = 'b';
      comm_descr[poss++] = "Browse";
      comm[poss] = 'm';
      comm_descr[poss++] = "Cast a spell";
    }

  /* Inscribe an object */
  comm[poss] =  '{';
  comm_descr[poss++] = "Inscribe";
      
  /* Uninscribe an object */
  if (o_ptr->note)
    {
      comm[poss] = '}';
      comm_descr[poss++] = "Uninscribe";
    }

  /* Activate equipment */
  if ((o_ptr->activation) && (item >= INVEN_WIELD))
    {
      comm[poss] = 'A';
      comm_descr[poss++] = "Activate";
    }
      
  /* Eat some food */
  if (o_ptr->tval == TV_FOOD)
    {
      comm[poss] = 'E';
      comm_descr[poss++] = "Eat";
    }
      
  if ((item < INVEN_WIELD) && ((o_ptr->tval == TV_LITE) || 
			       (o_ptr->tval == TV_FLASK)))
    {
       object_type *o1_ptr = &inventory[INVEN_LITE];

       if (((o1_ptr->sval == SV_LITE_LANTERN) && 
	    ((o_ptr->tval == TV_FLASK) || ((o_ptr->tval == TV_LITE) &&
					   (o_ptr->sval == SV_LITE_LANTERN))))
	   || ((o1_ptr->sval == SV_LITE_TORCH) && (o_ptr->tval == TV_LITE)
	       && (o_ptr->sval == SV_LITE_TORCH)))
 
	 {
	   comm[poss] = 'F';
	   comm_descr[poss++] = "Refuel";
	 }
    }
      
  /* Fire an item */
  if (p_ptr->ammo_tval == o_ptr->tval)
    {
      comm[poss] = 'f';
      comm_descr[poss++] = "Fire";
    }
  
  /* Throw an item */
  if ((item < INVEN_WIELD) || (!SCHANGE))
    {
      comm[poss] = 'v';
      comm_descr[poss++] = "Throw";
    }
      
  /* Aim a wand */
  if (o_ptr->tval == TV_WAND)
    {
      comm[poss] = 'a';
      comm_descr[poss++] = "Aim";
    }
      
  /* Zap a rod */
  if (o_ptr->tval == TV_ROD)
    {
      comm[poss] = 'z';
      comm_descr[poss++] = "Zap";
    }
      
  /* Quaff a potion */
  if (o_ptr->tval == TV_POTION)
    {
      comm[poss] = 'q';
      comm_descr[poss++] = "Quaff";
    }
      
  /* Read a scroll */
  if (o_ptr->tval == TV_SCROLL)
    {
      comm[poss] = 'r';
      comm_descr[poss++] = "Read";
    }
      
  /* Use a staff */
  if (o_ptr->tval == TV_STAFF)
    {
      comm[poss] = 'u';
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
  if (use_trptile || use_dbltile)
    { 
      j = poss + 2;
      tile_hgt = (use_trptile ? 3 : 2);
      while ((j % tile_hgt) && (j <= SCREEN_ROWS)) 
	prt("", ++j, 0);
    }

  /* Get a choice */
  show_cmd_menu(TRUE);

  /* Load de screen */
  screen_load();
  
  if (p_ptr->command_new) repeat_push(p_ptr->command_new);
  
  /* Now set the item if valid */
  if (p_ptr->command_new)
    {
      p_ptr->command_item = item;
      
      /* Hack for first in inventory */
      if (p_ptr->command_item == 0) p_ptr->command_item += 100;
    }

  /* Turn off lists */
  p_ptr->command_see = FALSE;

}    

/**
 * Mouse commands in general play - mostly just mimics a keypress
 */
void handle_mousepress(int y, int x)
{
  int area, i;
  
  /* Exit if the player doesn't want to use the mouse */
  if (!mouse_buttons) return;

  /* Find out where we've clicked */
  area = click_area(p_ptr->command_cmd_ex);

  /* Hack - cancel the item testers */
  item_tester_tval = 0;
  item_tester_hook = NULL;

  /* Click on player -NRM-*/
  if ((p_ptr->py == y) && (p_ptr->px == x)) 
    {
      show_player();
    }
  
  /* Click next to player */
  else if ((ABS(p_ptr->py - y) <= 1) && (ABS(p_ptr->px - x) <= 1)) 
    {
      /* Get the direction */
      for (i = 0; i < 10; i++)
	if ((ddx[i] == (x - p_ptr->px)) && (ddy[i] == (y - p_ptr->py))) break;

      /* Take it */
      Term_keypress(I2D(i));
    }
  
  else 
    {
      switch (area)
	{
	case MOUSE_MAP:
	  {
	    if (p_ptr->confused)
	      {
		p_ptr->command_dir = mouse_dir(p_ptr->command_cmd_ex, FALSE);
		if (p_ptr->command_dir)	do_cmd_walk();
	      }
	    else
	      {
		do_cmd_pathfind(y, x);
	      }
	    break;
	  }
	case MOUSE_CHAR:
	  {
	    Term_keypress('C');
	    break;
	  }
	case MOUSE_HP:
	  {
	    Term_keypress('R');
	    break;
	  }
	case MOUSE_SP:
	  {
	    Term_keypress('m');
	    break;
	  }
	case MOUSE_MESSAGE:
	  {
	    Term_keypress(KTRL('P'));
	    break;
	  }
	case MOUSE_PLACE:
	  {
	    if (p_ptr->depth)
	      Term_keypress(KTRL('F'));
	    else
	      Term_keypress('H');
	    break;
	  }
	case MOUSE_OBJECTS:
	  {
	    do_cmd_show_obj();
	    break;
	  }
	default:
	  {
	    bell("Illegal mouse area!"); 
	  }
	}
    }
}



/**
 * Verify use of "wizard" mode
 */
static bool enter_wizard_mode(void)
{
  /* Ask first time */
  if (!(p_ptr->noscore & 0x0002))
    {
      /* Mention effects */
      msg_print("You are about to enter 'wizard' mode for the very first time!");
      msg_print("This is a form of cheating, and your game will not be scored!");
      msg_print(NULL);
      
      /* Verify request */
      if (!get_check("Are you sure you want to enter wizard mode? "))
	{
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
static void do_cmd_wizard(void)
{
  if(enter_wizard_mode())
    {
	/* Toggle mode */
	if (p_ptr->wizard)
	{
		p_ptr->wizard = FALSE;
		msg_print("Wizard mode off.");
	}
	else
	{
		p_ptr->wizard = TRUE;
		msg_print("Wizard mode on.");
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
  if (verify && verify_special && (!(p_ptr->noscore & 0x0008)))
    {
      /* Mention effects */
      msg_print("You are about to use the dangerous, unsupported, debug commands!");
      msg_print("Your machine may crash, and your savefile may become corrupted!");
      msg_print("Using the debug commands will also mark your savefile.");
      msg_print(NULL);
      
      /* Verify request */
      if (!get_check("Are you sure you want to use the debug commands? "))
	{
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
static void do_cmd_try_debug(void)
{
	/* Ask first time */
	if (verify_debug_mode())
	  {

	    /* Okay */
	    do_cmd_debug();
	  }
}

#endif /* ALLOW_DEBUG */



#ifdef ALLOW_BORG

/**
 * Verify use of "borg" mode
 */
static bool do_cmd_try_borg(void)
{
  /* Ask first time */
  if (!(p_ptr->noscore & NOSCORE_BORG))
    {
      /* Mention effects */
      msg_print("You are about to use the dangerous, unsupported, borg commands!");
      msg_print("Your machine may crash, and your savefile may become corrupted!");
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

#endif /* ALLOW_BORG */




/**
 * Quit the game.
 */
static void do_cmd_quit(void)
{
	/* Stop playing */
	p_ptr->playing = FALSE;

	/* Leaving */
	p_ptr->leaving = TRUE;
}


/**
 * Display the options and redraw afterward.
 */
static void do_cmd_xxx_options(void)
{
	do_cmd_options();
	do_cmd_redraw();
}


/**
 * Display the options and redraw afterward.
 */
static void do_cmd_reshape(void)
{
	if ((!SCHANGE) &&
	    (check_ability(SP_BEARSKIN)))
	  {
	    do_cmd_bear_shape();
	  }
	else do_cmd_unchange();
}


/**
 * Display the main-screen monster list.
 */
static void do_cmd_monlist(void)
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
static void do_cmd_itemlist(void)
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
static void do_cmd_unknown(void)
{
	prt("Type '?' for help.", 0, 0);
}




