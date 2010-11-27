/** \file cmd0.c
    \brief Deal with command processing.
 *
 * Copyright (c) 2009 Nick McConnell, Andrew Sidwell, Ben Harrison
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

/**
 * This file contains (several) big lists of commands, so that they can be
 * easily maniuplated for e.g. help displays, or if a port wants to provide a
 * native menu containing a command list.
 *
 * Consider a two-paned layout for the command menus. XXX
 *
 * This file still needs some clearing up. XXX
 */

/*** Big list of commands ***/

/**
 * Useful typedef 
 */
typedef void do_cmd_type(void);


/**
 * Forward declare these, because they're really defined later 
 */
static do_cmd_type do_cmd_wizard, do_cmd_try_debug, do_cmd_quit, 
            do_cmd_xxx_options, do_cmd_menu, do_cmd_monlist, do_cmd_itemlist, 
            do_cmd_reshape;
#ifdef ALLOW_BORG
static do_cmd_type do_cmd_try_borg;
#endif


/**
 * Holds a generic command.
 */
typedef struct
{
	const char *desc;
	unsigned char key;
	do_cmd_type *hook;
} command_type;


/**
 * Magic use 
 */
static command_type cmd_magic[] =
{
  { "Cast a spell",               'm', do_cmd_cast_or_pray },
  { "Pray a prayer",              'p', do_cmd_cast_or_pray },
  { "Browse a book",              'b', do_cmd_browse },
  { "Gain new spells or prayers", 'G', do_cmd_study }
};

/**
 * General actions 
 */
static command_type cmd_action[] =
{
  { "Disarm a trap or chest",             'D', do_cmd_disarm },
  { "Rest for a while",                   'R', do_cmd_rest },
  { "Look around",                        'l', do_cmd_look },
  { "Target monster or location",         '*', do_cmd_target },
  { "Dig a tunnel",                       'T', do_cmd_tunnel },
  { "Open a door or a chest",             'o', do_cmd_open },
  { "Close a door",                       'c', do_cmd_close },
  { "Search for traps/doors",             's', do_cmd_search },
  { "Alter a grid/set monster trap",      '+', do_cmd_alter },
  { "Go up staircase",                    '<', do_cmd_go_up },
  { "Go down staircase",                  '>', do_cmd_go_down },
  { "Toggle search mode",                 'S', do_cmd_toggle_search },
  { "Jam a door shut",                    'j', do_cmd_spike },
  { "Bash a door open",                   'B', do_cmd_bash },
  { "Racial shapechange/End shapechange", ']', do_cmd_reshape},
  { "Move house",                         'H', do_cmd_move_house} 
};

/**
 * Item use commands 
 */
static command_type cmd_item_use[] =
{
  { "Fire your missile weapon",   'f', do_cmd_fire },
  { "Throw an item",              'v', do_cmd_throw },
  { "Read a scroll",              'r', do_cmd_read_scroll },
  { "Quaff a potion",             'q', do_cmd_quaff_potion },
  { "Use a staff",                'u', do_cmd_use_staff },
  { "Aim a wand",                 'a', do_cmd_aim_wand },
  { "Zap a rod",                  'z', do_cmd_zap_rod },
  { "Activate an object",         'A', do_cmd_activate },
  { "Eat some food",              'E', do_cmd_eat_food },
  { "Fuel your light source",     'F', do_cmd_refill }
};

/**
 * Item management commands 
 */
static command_type cmd_item_manage[]  =
{
  { "Display equipment listing", 'e', do_cmd_equip },
  { "Display inventory listing", 'i', do_cmd_inven },
  { "Pick up objects",           'g', do_cmd_pickup },
  { "Wear/wield an item",        'w', do_cmd_wield },
  { "Take/unwield off an item",  't', do_cmd_takeoff },
  { "Drop an item",              'd', do_cmd_drop },
  { "Destroy an item",           'k', do_cmd_destroy },
  { "Examine an item",           'I', do_cmd_observe },
  { "Inscribe an object",        '{', do_cmd_inscribe },
  { "Uninscribe an object",      '}', do_cmd_uninscribe }
};

/**
 * Information access commands 
 */
static command_type cmd_info[] =
{
  { "Check/learn specialties",      'O', do_cmd_specialty }, 
  { "Full dungeon map",             'M', do_cmd_view_map },
  { "Locate player on map",         'L', do_cmd_locate },
  { "Help",                         '?', do_cmd_help },
  { "Identify symbol",              '/', do_cmd_query_symbol },
  { "Character description",        'C', do_cmd_change_name },
  { "Interact with options",        '=', do_cmd_xxx_options },
  { "Check knowledge",              '~', do_cmd_knowledge },
  { "Display visible monster list", '[', do_cmd_monlist },
  { "Display visible item list", KTRL('I'), do_cmd_itemlist },
  { "Repeat level feeling",   KTRL('F'), do_cmd_feeling },
  { "Show previous message",  KTRL('O'), do_cmd_message_one },
  { "Show previous messages", KTRL('P'), do_cmd_messages },
  { "Show the time of day",   KTRL('T'), do_cmd_time }
};

/**
 * Utility/assorted commands 
 */
static command_type cmd_util[] =
{
  { "Save and don't quit",  KTRL('S'), do_cmd_save_game },
  { "Save and quit",        KTRL('X'), do_cmd_quit },
  { "Quit (commit suicide)",      'Q', do_cmd_suicide },
  { "Redraw the screen",    KTRL('R'), do_cmd_redraw },
  { "Load \"screen dump\"",       '(', do_cmd_load_screen },
  { "Save \"screen dump\"",       ')', do_cmd_save_screen }
};

/**
 * Commands that shouldn't be shown to the user 
 */ 
static command_type cmd_hidden[] =
{
  { "Take notes",                      ':', do_cmd_note },
  { "Version info",                    'V', do_cmd_version },
  { "Mouse click",                  '\xff', do_cmd_mousepress },
  { "Enter a store",                   '_', do_cmd_store },
  { "Toggle windows",            KTRL('E'), toggle_inven_equip }, /* XXX */
  { "Walk",                            ';', do_cmd_walk },
  { "Start running",                   '.', do_cmd_run },
  { "Stand still",                     ',', do_cmd_hold },
  { "Jump (into a trap)",              '-', do_cmd_jump },
  { "Display menu of actions",        '\r', do_cmd_menu },
  { "Display inventory for selection", '|', do_cmd_show_obj },
  { "Toggle wizard mode",        KTRL('W'), do_cmd_wizard },

#ifdef ALLOW_DEBUG
  { "Debug mode commands",       KTRL('A'), do_cmd_try_debug },
#endif
#ifdef ALLOW_BORG
  { "Borg commands",             KTRL('Z'), do_cmd_try_borg }
#endif
};



/**
 * A categorised list of all the command lists.
 */
typedef struct
{
	const char *name;
	command_type *list;
  	size_t len;
} command_list;

static command_list cmds_all[] =
{
  { "Use magic/Pray",  cmd_magic,       N_ELEMENTS(cmd_magic) },
  { "Action commands", cmd_action,      N_ELEMENTS(cmd_action) },
  { "Use item",        cmd_item_use,    N_ELEMENTS(cmd_item_use) },
  { "Manage items",    cmd_item_manage, N_ELEMENTS(cmd_item_manage) },
  { "Information",     cmd_info,        N_ELEMENTS(cmd_info) },
  { "Utility",         cmd_util,        N_ELEMENTS(cmd_util) },
  { "Hidden",          cmd_hidden,      N_ELEMENTS(cmd_hidden) }
};


/**
 * Divide up the screen into mousepress regions 
 */

int click_area(event_type ke)
{

  if ((ke.mousey) && (ke.mousex > COL_MAP) && 
      (ke.mousey < Term->hgt - (panel_extra_rows ? 2 : 0) - 
       (bottom_status ? 6 : 0) - 1))
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
	   (ke.mousex < COL_CUT + (bottom_status ? 6 : 7)))
    return MOUSE_STAND;
  else if ((ke.mousey == ROW_REPEAT) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + (bottom_status ? 6 : 8)))
    return MOUSE_REPEAT;
  else if ((ke.mousey == ROW_RETURN) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + (bottom_status ? 6 : 8 )))
    return MOUSE_RETURN;
  else if ((ke.mousey == ROW_ESCAPE) && (ke.mousex >= COL_CUT) && 
	   (ke.mousex < COL_CUT + 5))
    return MOUSE_ESCAPE;
  else return MOUSE_NULL;
}

/**
 * Menu functions 
 */
char comm[22];
cptr comm_descr[22];
int poss;

/**
 * Item tag/command key 
 */
static char show_tag(menu_type *menu, int oid)
{
  /* Caution - could be a problem here if KTRL commands were used */
  return comm[oid];
}

/**
 * Display an entry on a command menu 
 */
static void show_display(menu_type *menu, int oid, bool cursor, int row, 
			  int col, int width)
{
  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  /* Write the description */
  Term_putstr(col + 4, row, -1, attr, comm_descr[oid]);
}


/**
 * Handle user input from a command menu 
 */
static bool show_action(char cmd, void *db, int oid)
{
  /* Handle enter and mouse */
  if (cmd == '\n' || cmd == '\r' || cmd == '\xff')
    {
      p_ptr->command_new = comm[oid];
    }
  return TRUE;
}

/**
 * Display a list of commands.
 */
void show_cmd_menu(bool object)
{
  menu_type menu;
  menu_iter commands_menu = { 0, show_tag, 0, show_display, show_action };
  region area = { 0, (object ? 2 : 1), 20, 0 };
  
  event_type evt = EVENT_EMPTY;
  int cursor = 0;
  
  /* Size of menu */
  area.page_rows = poss + (object ? 1 : 0);

  /* Set up the menu */
  WIPE(&menu, menu);
  menu.cmd_keys = "\x8B\x8C\n\r";
  menu.count = poss;
  menu.menu_data = comm;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &commands_menu, &area);
  
  /* Select an entry */
  evt = menu_select(&menu, &cursor, 0);
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

/* Hugo Kornelis' item handling patch */

/**
 * Allow user to "prevent" certain choices.
 *
 * This is a slightly modified copy of get_item_allow in object1.c
 * This is used when 'h'andling items. The use has already confirmed
 * for any !* or !h inscription; now we check for the inscription
 * that fits the object type (e.g. when handling a scroll, check !r)
 */
bool handle_item_allow(int item, s16b command)
{
  cptr s;
  
  object_type *o_ptr;
  
  /* Inventory */
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Floor */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  /* No inscription */
  if (!o_ptr->note) return (TRUE);
  
  /* Find a '!' */
  s = strchr(quark_str(o_ptr->note), '!');
  
  /* Process preventions */
  while (s)
    {
      /* Check the "restriction" */
      if (s[1] == command)
	{
	  /* Verify the choice */
	  if (!verify_item("Really try", item)) return (FALSE);
	}
      
      /* Find another '!' */
      s = strchr(s + 1, '!');
    }
  
  /* Allow it */
  return (TRUE);
}


/**
 * Hook to determine if an object can be handled
 *
 * Accept all items on floor and in inventory, but only activateable items 
 * in equipment
 */
static bool item_tester_hook_handle(object_type *o_ptr)
{
  /* The procedure to determining if an item is equipped is a big hack */
  /* Unfortunately, we have no choice since we can't tell from the object 
   * info */

  /* Determine where the object kind is normally equipped */
  s16b slot = wield_slot(o_ptr);
  
  /* If the object kind is equippable at all, then compare item in equipment 
   * slot with item to test - if the pointers are equal, it's the same item */
  if ((slot >= INVEN_WIELD) && (&inventory[slot] == o_ptr))
    {
      /* The item is equipped. Test if players knows that it can be activated */
      
      /* Check activation flag */
      if (o_ptr->activation) return (TRUE);
      
      /* Assume not */
      return (FALSE);
    }
  /* All unequipped items are all okay */
  else return (TRUE);
}


/**
 * Handle an item (generic use command)
 *
 * Accepts all items in inventory, equipment or floor;
 * calls command handler to execute most appropriate command
 * for the object kind.
 */
void do_cmd_handle(void)
{
  int item;
  object_type *o_ptr;
  
  cptr q, s;
  
  
  /* Prepare the hook */
  item_tester_hook = item_tester_hook_handle;
  
  /* Get an item */
  q = "Handle which item? ";
  s = "You have nothing to handle.";
  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;
  
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
  
  
  /* Analyze the object and determine appropriate action */
  switch (o_ptr->tval)
    {
      /* Junk items will be destroyed */
    case TV_SKELETON:
    case TV_BOTTLE:
    case TV_JUNK:
      {
	/* Verify use if inscribed with !k */
	if (!handle_item_allow(item, 'k')) return;

	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Destroy the junk */
	do_cmd_destroy();
	
	return;
      }
      
      /* Spikes will be used to jam a door */
    case TV_SPIKE:
      {
	/* Verify use if inscribed with !j */
	if (!handle_item_allow(item, 'j')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Spike the door */
	do_cmd_spike();
	
	return;
      }
      
      /* Chests will be destroyed if empty, dropped if held, disarmed or 
       * opened if on floor */
    case TV_CHEST:
      {
	/* Chest already empty? */
	if (!o_ptr->pval)		/* no items in chest */
	  {
	    /* Verify use if inscribed with !k */
	    if (!handle_item_allow(item, 'k')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Destroy the empty chest */
	    do_cmd_destroy();
	    
	    return;
	  }
	
	/* Chest currently in the pack? */
	if (item >= 0)
	  {
	    /* Verify use if inscribed with !d */
	    if (!handle_item_allow(item, 'd')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Drop the chest */
	    do_cmd_drop();
	    
	    return;
	  }
	
	/* Any known traps on this chest? */
	if ((o_ptr->pval > 0) &&	/* chest must not yet be disarmed */
	    (chest_traps[o_ptr->pval]) && /* chest must actually be trapped */
	    (object_known_p(o_ptr)))	/* trap must already be found */
	  {
	    /* Verify use if inscribed with !D */
	    if (!handle_item_allow(item, 'D')) return;
	    
	    /* Set the direction */
	    p_ptr->command_dir = 5;

	    /* Disarm */
	    do_cmd_disarm();
	    
	    return;
	  }
	
	/* Not empty, not in pack, no (known) traps - try to open */
	/* Verify use if inscribed with !o */
	if (!handle_item_allow(item, 'o')) return;
	
	/* Set the direction */
	p_ptr->command_dir = 5;

	/* Open */
	do_cmd_open();
	
	return;
      }
      
      /* Ammunition will be fired if correct launcher is equipped, 
       * thrown otherwise */
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
      {
	/* Proper type of launcher equipped? */
	if (o_ptr->tval == p_ptr->ammo_tval)
	  {
	    /* Verify use if inscribed with !f */
	    if (!handle_item_allow(item, 'f')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Fire the ammunition */
	    do_cmd_fire();
	    
	    return;
	  }
	
	/* Wielded launcher unusable for this ammo - throw it instead */
	/* Verify use if inscribed with !v */
	if (!handle_item_allow(item, 'v')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Throw the ammunition */
	do_cmd_throw();
	
	return;
      }
      
      /* Equippable items: mostly activate if currently equipped, wear/wield 
       * otherwise.  Exception for torches and lanterns: refill if same 
       * equipped, wear/wield otherwise */
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_LITE:
    case TV_AMULET:
    case TV_RING:
      {
	/* If item is equipped, try to activate it 
	 * (Note - equipped items without activation can't be handled, see 
	 * item_tester_hook_handle.  This also means that we don't have to 
	 * test for torches or lanterns here) */
	if (item >= INVEN_WIELD)
	  {
	    /* Verify use if inscribed with !A */
	    if (!handle_item_allow(item, 'A')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Activate the item */
	    do_cmd_activate();
	    
	    return;
	  }
	
	/* If item is torch or lantern and same object is equipped, use it 
	 * to refill */
	if ((o_ptr->tval == TV_LITE) &&
	    ((o_ptr->sval == SV_LITE_LANTERN) || 
	     (o_ptr->sval == SV_LITE_TORCH)))
	  {
	    object_type *lite_ptr;
	    
	    /* Get current light */
	    lite_ptr = &inventory[INVEN_LITE];
	    
	    /* Check if light is used, and if type is the same */
	    if ((lite_ptr->tval == TV_LITE) && (lite_ptr->sval == o_ptr->sval))
	      {
		/* Verify use if inscribed with !F */
		if (!handle_item_allow(item, 'F')) return;
		
		/* Set the item */	
		p_ptr->command_item = item;
		
		/* Hack for first in inventory */
		if (p_ptr->command_item == 0) p_ptr->command_item += 100;
		
		/* Refill the light source */
		do_cmd_refill();
		
		return;
	      }
	  }
	
	/* Not equipped, not a torch or lantern that can be used to refill 
	 * - then wear/wield it 
	 * Verify use if inscribed with !w */
	if (!handle_item_allow(item, 'w')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Wear/wield the equipment */
	do_cmd_wield();
	
	return;
      }
      
      /* Staves will be used */
    case TV_STAFF:
      {
	/* Verify use if inscribed with !u */
	if (!handle_item_allow(item, 'u')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Use the staff */
	do_cmd_use_staff();
	
	return;
      }
      
      /* Wands will be aimed */
    case TV_WAND:
      {
	/* Verify use if inscribed with !a */
	if (!handle_item_allow(item, 'a')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Aim the wand */
	do_cmd_aim_wand();
	
	return;
      }
      
      /* Rods will be zapped */
    case TV_ROD:
      {
	/* Verify use if inscribed with !z */
	if (!handle_item_allow(item, 'z')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Zap the rod */
	do_cmd_zap_rod();
	
	return;
      }
      
      /* Scrolls will be read */
    case TV_SCROLL:
      {
	/* Verify use if inscribed with !r */
	if (!handle_item_allow(item, 'r')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Read the scroll */
	do_cmd_read_scroll();
	
	return;
      }
      
      /* Potions will be quaffed, except Potion of Detonations, 
       * which is thrown */
    case TV_POTION:
      {
	/* Check for (known) Potion of Detonations */
	if ((o_ptr->sval == SV_POTION_DETONATIONS) &&
	    (object_aware_p(o_ptr)))
	  {
	    /* Verify use if inscribed with !v */
	    if (!handle_item_allow(item, 'v')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Throw the exploding potion */
	    do_cmd_throw();
	    
	    return;
	  }
	
	/* Not Potion of Detonations or not yet aware - quaff it */
	/* Verify use if inscribed with !q */
	if (!handle_item_allow(item, 'q')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Quaff the potion */
	do_cmd_quaff_potion();
	
	return;
      }
      
      /* Flask of Oil: refill lantern if one is equipped, throw otherwise */
    case TV_FLASK:
      {
	object_type *lite_ptr;
	
	/* Get current light */
	lite_ptr = &inventory[INVEN_LITE];
	
	/* Check if a lantern is used */
	if ((lite_ptr->tval == TV_LITE) && (lite_ptr->sval == SV_LITE_LANTERN))
	  {
	    /* Verify use if inscribed with !F */
	    if (!handle_item_allow(item, 'F')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Refill the light source */
	    do_cmd_refill();
	    
	    return;
	  }
	
	/* No lantern equipped - throw the oil */
	/* Verify use if inscribed with !v */
	if (!handle_item_allow(item, 'v')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Throw the oil */
	do_cmd_throw();
	
	return;
      }
      
      /* Food will be eaten, except Mushrooms of Unhealth / of Disease, 
       * which are thrown */
    case TV_FOOD:
      {
	/* Check for (known) Mushroom of Unhealth or Mushroom of Disease */
	if (((o_ptr->sval == SV_FOOD_UNHEALTH) || 
	     (o_ptr->sval == SV_FOOD_DISEASE)) &&
	    (object_aware_p(o_ptr)))
	  {
	    /* Verify use if inscribed with !v */
	    if (!handle_item_allow(item, 'v')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Throw the exploding potion */
	    do_cmd_throw();
	    
	    return;
	  }
	
	/* Not Mushroom of Unhealth/Disease or not yet aware - eat it */
	/* Verify use if inscribed with !E */
	if (!handle_item_allow(item, 'E')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Eat the food */
	do_cmd_eat_food();
	
	return;
      }
      
      /* Spell books: cast a spell if you're a mage/rogue, destroy otherwise */
    case TV_MAGIC_BOOK:
      {
	/* Is the player able to cast spells? */
	if (mp_ptr->spell_book == TV_MAGIC_BOOK)
	  {
	    /* Verify use if inscribed with !m */
	    if (!handle_item_allow(item, 'm')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Cast a spell from this book */
	    do_cmd_cast_or_pray();
	    
	    return;
	  }
	
	/* Player can't cast spells - destroy the book */
	/* Verify use if inscribed with !k */
	if (!handle_item_allow(item, 'k')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Destroy the book */
	do_cmd_destroy();
	
	return;
      }
      
      /* Prayer books: recite a prayer if you're a priest/paladin, 
       * destroy otherwise */
    case TV_PRAYER_BOOK:
      {
	/* Is the player able to recite prayers? */
	if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	  {
	    /* Verify use if inscribed with !p */
	    if (!handle_item_allow(item, 'p')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Recite a prayer from this book */
	    do_cmd_cast_or_pray();
	    
	    return;
	  }
	
	/* Player can't recite prayers - destroy the book */
	/* Verify use if inscribed with !k */
	if (!handle_item_allow(item, 'k')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Destroy the book */
	do_cmd_destroy();
	
	return;
      }
      
      /* Stones of lore: use a druidic lore if you're a druid/ranger, 
       * destroy otherwise */
    case TV_DRUID_BOOK:
      {
	/* Is the player able to recite prayers? */
	if (mp_ptr->spell_book == TV_DRUID_BOOK)
	  {
	    /* Verify use if inscribed with !p */
	    if (!handle_item_allow(item, 'p')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Recite a prayer from this book */
	    do_cmd_cast_or_pray();
	    
	    return;
	  }
	
	/* Player can't recite prayers - destroy the book */
	/* Verify use if inscribed with !k */
	if (!handle_item_allow(item, 'k')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Destroy the book */
	do_cmd_destroy();
	
	return;
      }
      
      /* Necromantic tomes: perform a ritual if you're a necro/assassin, 
       * destroy otherwise */
    case TV_NECRO_BOOK:
      {
	/* Is the player able to recite prayers? */
	if (mp_ptr->spell_book == TV_NECRO_BOOK)
	  {
	    /* Verify use if inscribed with !p */
	    if (!handle_item_allow(item, 'm')) return;
	    
	    /* Set the item */	
	    p_ptr->command_item = item;
	    
	    /* Hack for first in inventory */
	    if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	    /* Recite a prayer from this book */
	    do_cmd_cast_or_pray();
	    
	    return;
	  }
	
	/* Player can't recite prayers - destroy the book */
	/* Verify use if inscribed with !k */
	if (!handle_item_allow(item, 'k')) return;
	
	/* Set the item */	
	p_ptr->command_item = item;
	
	/* Hack for first in inventory */
	if (p_ptr->command_item == 0) p_ptr->command_item += 100;

	/* Destroy the book */
	do_cmd_destroy();
	
	return;
      }
      
      /* Paranoia */
    default:
      {
	if (flush_failure) flush();
	msg_print("You can't handle it.");
	return;
      }
    }
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
 * Mouse commands in general play - mostly just mimics a keypress
 */
void do_cmd_mousepress(void)
{
  int area, i;
  int y = KEY_GRID_Y(p_ptr->command_cmd_ex);
  int x = KEY_GRID_X(p_ptr->command_cmd_ex);
  
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




/** List indexed by char */
do_cmd_type *converted_list[UCHAR_MAX+1];


/*** Menu functions ***/

/** 
 * Display an entry on a command menu 
 */
static void cmd_sub_entry(menu_type *menu, int oid, bool cursor, int row, 
			  int col, int width)
{
  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  const command_type *commands = menu->menu_data;
  
  /* Write the description */
  Term_putstr(col, row, -1, attr, commands[oid].desc);
  
  /* Include keypress */
  Term_addch(attr, ' ');
  Term_addch(attr, '(');
  
  /* KTRL()ing a control character does not alter it at all */
  if (KTRL(commands[oid].key) == commands[oid].key)
    {
      Term_addch(attr, '^');
      Term_addch(attr, UN_KTRL(commands[oid].key));
    }
  else
    {
      Term_addch(attr, commands[oid].key);
    }
  
  Term_addch(attr, ')');
}


/** 
 * Handle user input from a command menu 
 */
static bool cmd_sub_action(char cmd, void *db, int oid)
{
  /* Only handle enter */
  if (cmd == '\n' || cmd == '\r')
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

/**
 * Display a list of commands.
 */
static bool cmd_menu(command_list *list, void *selection_p)
{
  menu_type menu;
  menu_iter commands_menu = { 0, 0, 0, cmd_sub_entry, cmd_sub_action };
  region area = { 24, 4, 37, 15 };
  
  event_type evt = EVENT_EMPTY;
  int cursor = 0;
  command_type *selection = selection_p;
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.cmd_keys = "\x8B\x8C\n\r";
  menu.count = list->len;
  menu.menu_data = list->list;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &commands_menu, &area);
  
  /* Set up the screen */
  screen_save();
  window_make(22, 3, 62, 19);
  
  /* Select an entry */
  evt = menu_select(&menu, &cursor, 0);
  
  /* Load de screen */
  screen_load();

  if (evt.type == EVT_SELECT)
    {
      *selection = list->list[evt.index];
    }
  
  if (evt.type == EVT_ESCAPE)
    {
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}



static bool cmd_list_action(char cmd, void *db, int oid)
{
  if (cmd == '\n' || cmd == '\r' || cmd == '\xff')
    {
      return cmd_menu(&cmds_all[oid], db);
    }
  else
    {
      return FALSE;
    }
}

static void cmd_list_entry(menu_type *menu, int oid, bool cursor, int row, 
			   int col, int width)
{
  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  Term_putstr(col, row, -1, attr, cmds_all[oid].name);
}

/**
 * Display a list of command types, allowing the user to select one.
 */
static void do_cmd_menu(void)
{
  menu_type menu;
  menu_iter commands_menu = { 0, 0, 0, cmd_list_entry, cmd_list_action };
  region area = { 20, 5, 37, 6 };
  
  event_type evt = EVENT_EMPTY;
  int cursor = 0;
  command_type chosen_command = { NULL, 0, NULL };

  /* Check the option */
  if (!show_menus) return;
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.cmd_keys = "\x8B\x8C\n\r";
  menu.count = N_ELEMENTS(cmds_all) - 1;
  menu.menu_data = &chosen_command;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &commands_menu, &area);
  
  /* Set up the screen */
  screen_save();
  window_make(18, 4, 58, 11);
  
  /* Select an entry */
  evt = menu_select(&menu, &cursor, 0);
  
  /* Load de screen */
  screen_load();
  
  /* If a command was chosen, do it. */
  if (chosen_command.hook)
    {
      chosen_command.hook();
    }
}


/*** Exported functions ***/

/**
 * Initialise the command list.
 */
void cmd_init(void)
{
  size_t i, j;
  
  /* Go through all the lists of commands */
  for (j = 0; j < N_ELEMENTS(cmds_all); j++)
    {
      command_type *commands = cmds_all[j].list;
      
      /* Fill everything in */
      for (i = 0; i < cmds_all[j].len; i++)
	{
	  unsigned char key = commands[i].key;
	  
	  /* Note: at present converted_list is UCHAR_MAX + 1 
	     large, so 'key' is always a valid index. */
	  converted_list[key] = commands[i].hook;
	}
    }
  
  /* Fill in the rest */
  for (i = 0; i < N_ELEMENTS(converted_list); i++)
    {
      switch (i)
	{
	  /* Ignore */
	case ESCAPE:
	case ' ':
	case '\a':
	  {
	    break;
	  }
	  
	default:
	  {
	    if (!converted_list[i])
	      converted_list[i] = do_cmd_unknown;
	  }
	}		
    }
}


/**
 * Parse and execute the current command
 * Give "Warning" on illegal commands.
 */
void process_command(bool no_request)
{
  if (!no_request)
    request_command();

  /* Handle repeating the last command */
  repeat_check();
  
  /* Handle resize events XXX */
  if (p_ptr->command_cmd_ex.type == EVT_RESIZE)
    {
      do_cmd_redraw();
    }
  else
    {
      /* Within these boundaries, the cast to unsigned char will have the 
       * desired effect */
      assert(p_ptr->command_cmd >= CHAR_MIN && p_ptr->command_cmd <= CHAR_MAX);
      
      /* Execute the command */
      if (converted_list[(unsigned char) p_ptr->command_cmd])
	converted_list[(unsigned char) p_ptr->command_cmd]();
    }
}


