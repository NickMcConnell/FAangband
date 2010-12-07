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
#include "ui.h"


/* max length of note output */
#define LINEWRAP        75

static void dump_pref_file(void (*dump)(ang_file *), const char *title, int row)
{
	char ftmp[80];
	char buf[1024];

	/* Prompt */
	prt(format("%s to a pref file", title), row, 0);
	
	/* Prompt */
	prt("File: ", row + 2, 0);
	
	/* Default filename */
	strnfmt(ftmp, sizeof ftmp, "%s.prf", op_ptr->base_name);
	
	/* Get a filename */
	if (!askfor_aux(ftmp, sizeof ftmp, NULL)) return;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, ftmp);
	
	if (!prefs_save(buf, dump, title))
	{
		prt("", 0, 0);
		msg_print("Failed");
		return;
	}

	/* Message */
	prt("", 0, 0);
	msg_print(format("Dumped %s", strstr(title, " ")+1));
}

static void do_cmd_pref_file_hack(long row);



/* Flag value for missing array entry */
#define MISSING -17

#define APP_MACRO	101
#define ASK_MACRO	103
#define DEL_MACRO	104
#define NEW_MACRO	105
#define APP_KEYMAP	106
#define ASK_KEYMAP	107
#define DEL_KEYMAP	108
#define NEW_KEYMAP	109
#define ENTER_ACT	110
#define LOAD_PREF	111
#define DUMP_MON	112
#define DUMP_OBJ	113
#define DUMP_FEAT	114
#define DUMP_FLAV	115
#define MOD_MON		116
#define	MOD_OBJ		117
#define MOD_FEAT	118
#define MOD_FLAV	119
#define DUMP_COL	120
#define MOD_COL		121
#define RESET_VIS	122

#define OPTION_MENU     140
#define VISUAL_MENU     141
#define COLOR_MENU	142
#define KNOWLEDGE_MENU  143
#define MACRO_MENU	144




typedef struct
{
  int maxnum;       /* Maximum possible item count for this class */
  bool easy_know;   /* Items don't need to be IDed to recognize membership */
  
  const char *(*name)(int gid);               /* Name of this group */
  
  /* Compare, in group and display order (optional if already sorted) */
  int (*gcomp)(const void *, const void *);   /* Compares gids of two oids */
  int (*group)(int oid);                      /* Returns gid for an oid */
  
  /* Summary function for the "object" information. */
  void (*summary)(int gid, const int *object_list, int n, int top, int row, 
		  int col);
  
} group_funcs;

typedef struct
{
  /* Displays an entry at specified location, including kill-count and 
   * graphics */

  void (*display_member)(int col, int row, bool cursor, int oid);
  
  void (*lore)(int oid);       /* Displays lore for an oid */
  
  
  /* Required only for objects with modifiable display attributes */
  /* Unknown 'flavors' return flavor attributes */

  char *(*xchar)(int oid);     /* Get character attr for OID (by address) */
  byte *(*xattr)(int oid);     /* Get color attr for OID (by address) */

  const char *(*xtra_prompt)(int oid);  /* Returns optional extra prompt */
  void (*xtra_act)(char ch, int oid);   /* Handles optional extra actions */
  
  bool is_visual;                     /* Does this kind have visual editing? */

} member_funcs;


/* Helper class for generating joins */
typedef struct join
{
  int oid;
  int gid;
} join_t;

/* A default group-by */
static join_t *default_join;
static int default_group(int oid) { return default_join[oid].gid; }


static int *obj_group_order;

/*
 * Description of each monster group.
 */
static struct
{
  cptr chars;
  cptr name;
} monster_group[] =
  {
    { (cptr)-1,     "Uniques" },
    { "a",          "Ants" },
    { "b",          "Bats" },
    { "B",          "Birds" },
    { "C",		"Canines" },
    { "c",              "Centipedes" },
    { "uU",             "Demons" },
    { "dD",             "Dragons" },
    { "E",              "Elementals" },
    { "e",              "Eyes" },
    { "f",              "Felines" },
    { "G",              "Ghosts" },
    { "OP",             "Giants/Ogres" },
    { "g",              "Golems" },
    { "H",              "Hybrids" },
    { "h",              "Hominids" },
    { "i",              "Icky Things" },
    { "FI",             "Insects" },
    { "j",              "Jellies" },
    { "K",              "Killer Beetles" },
    { "k",              "Kobolds" },
    { "l",              "Lice" },
    { "L",              "Liches" },
    { "$?!_=.-|*~",     "Mimics" },
    { "m",              "Molds" },
    { "M",              "Mummies" },
    { ",",              "Mushroom Patches" },
    { "n",              "Nagas" },
    { "o",              "Orcs" },
    { "tp",             "People" },
    { "q",              "Quadrupeds" },
    { "Q",              "Quylthulgs" },
    { "R",              "Reptiles/Amphibians" },
    { "r",		"Rodents" },
    { "S",		"Scorpions/Spiders" },
    { "s",		"Skeletons/Drujs" },
    { "J",              "Snakes" },
    { "T",              "Trolls" },
    { "V",              "Vampires" },
    { "v",              "Vortices" },
    { "W",              "Wights/Wraiths" },
    { "w",              "Worms/Worm Masses" },
    { "X#",             "Xorns/Xarens" },
    { "Y",		"Yeti" },
    { "Z",		"Zephyr Hounds" },
    { "z",		"Zombies" },
    { NULL,		NULL }
  };

/*
 * Description of each feature group.
 */
const char *feature_group_text[] = 
  {
    "Ground",
    "Traps",
    "Doors",
    "Stairs/Paths",
    "Walls",
    "Streamers",
    "Obstructions",
    "Stores",
    "Other",
    NULL
  };



/* Useful method declarations */
static void display_visual_list(int col, int row, int height, int width,
				byte attr_top, char char_left);

static bool visual_mode_command(event_type ke, bool *visual_list_ptr, 
				int height, int width, 
				byte *attr_top_ptr, char *char_left_ptr, 
				byte *cur_attr_ptr, char *cur_char_ptr,
				int col, int row, int *delay);

static void place_visual_list_cursor(int col, int row, byte a,
				     byte c, byte attr_top, byte char_left);


/*
 * Clipboard variables for copy&paste in visual mode
 */
static byte attr_idx = 0;
static byte char_idx = 0;

/*
 * Return a specific ordering for the features
 */
int feat_order(int feat)
{
  feature_type *f_ptr = &f_info[feat];

  /* Hack until feature flags are done -NRM- */
  char ch = f_ptr->d_char;
  if (streq(f_name + f_ptr->name, "web")) ch = ':';
  if (streq(f_name + f_ptr->name, "trees")) ch = ':';
  if (streq(f_name + f_ptr->name, "lava")) ch = ':';
  if (streq(f_name + f_ptr->name, "water")) ch = ':';
  if (streq(f_name + f_ptr->name, "empty space")) ch = ':';
  
  switch (ch)
    {
    case '.': 			return 0;
    case '^': 			return 1;
    case '\'': case '+': 	return 2;
    case '<': case '>':		return 3;
    case '#':			return 4;
    case '*': case '%' :	return 5;
    case ';': case ':' :	return 6;
      
    default:
      {
	if (isdigit(f_ptr->d_char)) return 7;
	return 8;
      }
    }
}


/** Emit a 'graphical' symbol and a padding character if appropriate */
static void big_pad(int col, int row, byte a, byte c)
{
  Term_putch(col, row, a, c);
  if (!use_bigtile) return;
  if (a &0x80) Term_putch(col+1, row, 255, -1);
  else Term_putch(col+1, row, 1, ' ');
}

static int actual_width(int width) {
  if (use_trptile) width = width * 3;
  else if (use_dbltile) width *= 2;
  if (use_bigtile) width *= 2;
  return width;
}


static int actual_height(int height) {
  if (use_bigtile) height *= 2;
  if (use_trptile) height = height * 3 / 2;
  else if (use_dbltile) height *= 2;
  return height;
}

static int logical_width(int width)
{
  int div = 1;
  if (use_trptile) div = 3;
  else if (use_dbltile) div *= 2;
  if (use_bigtile) div *= 2;
  return width / div;
}

static int logical_height(int height)
{
  int div = 1;
  if (use_trptile)
    {
      height *= 2;
      div = 3;
    }
  else if (use_dbltile) div = 2;
  if (use_bigtile) div *= 2;
  return height / div;
}


static void display_group_member(menu_type *menu, int oid,
				 bool cursor, int row, int col, int wid)
{
  const member_funcs *o_funcs = menu->menu_data;
  byte attr = curs_attrs[CURS_KNOWN][cursor == oid];
  
  /* Print the interesting part */
  o_funcs->display_member(col, row, cursor, oid);
  
  /* Do visual mode */
  if (o_funcs->is_visual && o_funcs->xattr)
    {
      char c = *o_funcs->xchar(oid);
      byte a = *o_funcs->xattr(oid);
      c_put_str(attr, format((c & 0x80) ? "%02x/%02x" : "%02x/%d", a, c), 
                row, 60);
    }
}


#define swap(a, b) (swapspace = (void*)(a)), ((a) = (b)), ((b) = swapspace)

/**
 * Interactive group by. 
 * Recognises inscriptions, graphical symbols, lore
 */
static void display_knowledge(const char *title, int *obj_list, int o_count,
			      group_funcs g_funcs, member_funcs o_funcs,
			      const char *otherfields)
{
  /* maximum number of groups to display */
  int max_group = g_funcs.maxnum < o_count ? g_funcs.maxnum : o_count ;
  
  /* This could (should?) be (void **) */
  int *g_list, *g_offset;
  
  const char **g_names;
  
  int g_name_len = 8;  /* group name length, minumum is 8 */
  
  int grp_cnt = 0; /* total number groups */
  
  int g_cur = 0, grp_old = -1;  /* group list positions */
  int o_cur = 0;		/* object list positions */
  int g_o_count = 0;	        /* object count for group */
  int oid = -1;  		/* object identifiers */
  
  region title_area = { 0, 0, 0, 4 };
  region group_region = { 0, 6, MISSING, -2 };
  region object_region = { MISSING, 6, 0, -2 };
  
  /* display state variables */
  bool visual_list = FALSE;
  byte attr_top = 0;
  char char_left = 0;
  
  int delay = 0;
  
  menu_type group_menu;
  menu_type object_menu;
  menu_iter object_iter;
  
  /* Panel state */
  /* These are swapped in parallel whenever the actively browsing " */
  /* changes */
  int *active_cursor = &g_cur, *inactive_cursor = &o_cur;
  menu_type *active_menu = &group_menu, *inactive_menu = &object_menu;
  int panel = 0;
  
  void *swapspace;
  bool do_swap = FALSE;
  
  bool flag = FALSE;
  bool redraw = TRUE;
  
  int browser_rows;
  int wid, hgt;
  int i;
  int prev_g = -1;
  
  int omode = rogue_like_commands;
  
  
  /* Get size */
  Term_get_size(&wid, &hgt);
  browser_rows = hgt - 8;
  
  /* Disable the roguelike commands for the duration */
  rogue_like_commands = FALSE;
  
  
  
  /* Do the group by. ang_sort only works on (void **) */
  /* Maybe should make this a precondition? */
  if (g_funcs.gcomp)
    qsort(obj_list, o_count, sizeof(*obj_list), g_funcs.gcomp);
  
  
  /* Sort everything into group order */
  C_MAKE(g_list, max_group + 1, int);
  C_MAKE(g_offset, max_group + 1, int);
  
  for (i = 0; i < o_count; i++)
    {
      if (prev_g != g_funcs.group(obj_list[i]))
	{
	  prev_g = g_funcs.group(obj_list[i]);
	  g_offset[grp_cnt] = i;
	  g_list[grp_cnt++] = prev_g;
	}
    }
  
  g_offset[grp_cnt] = o_count;
  g_list[grp_cnt] = -1;
  
  
  /* The compact set of group names, in display order */
  C_MAKE(g_names, grp_cnt, const char **);
  
  for (i = 0; i < grp_cnt; i++)
    {
      int len;
      g_names[i] = g_funcs.name(g_list[i]);
      len = strlen(g_names[i]);
      if (len > g_name_len) g_name_len = len;
    }
  
  /* Reasonable max group name len */
  if (g_name_len >= 20) g_name_len = 20;
  
  object_region.col = g_name_len + 3;
  group_region.width = g_name_len;
  
  
  /* Leave room for the group summary information */
  if (g_funcs.summary) object_region.page_rows = -3;
  
  
  /* Set up the two menus */
  WIPE(&group_menu, menu_type);
  group_menu.count = grp_cnt;
  group_menu.cmd_keys = "\n\r6\x8C";  /* Ignore these as menu commands */
  group_menu.menu_data = g_names;
  
  WIPE(&object_menu, menu_type);
  object_menu.menu_data = &o_funcs;
  WIPE(&object_iter, object_iter);
  object_iter.display_row = display_group_member;
  
  o_funcs.is_visual = FALSE;
  
  menu_init(&group_menu, MN_SCROLL, MN_STRING, &group_region);
  menu_init2(&object_menu, find_menu_skin(MN_SCROLL), &object_iter, 
             &object_region);

  /* Wipe buttons */
  backup_buttons();
  kill_all_buttons();
  
  
  /* This is the event loop for a multi-region panel */
  /* Panels are -- text panels, two menus, and visual browser */
  /* with "pop-up menu" for lore */
  while ((!flag) && (grp_cnt))
    {
      event_type ke, ke0;
      
      if (redraw)
	{
	  /* Print the title bits */
	  region_erase(&title_area);
	  prt(format("Knowledge - %s", title), 2, 0);
	  prt("Group", 4, 0);
	  prt("Name", 4, g_name_len + 3);
	  
	  if (otherfields)
	    prt(otherfields, 4, 55);
	  
	  
          /* Print dividers: horizontal and vertical */
          for (i = 0; i < 79; i++)
            Term_putch(i, 5, TERM_WHITE, '=');
          
          for (i = 0; i < browser_rows; i++)
            Term_putch(g_name_len + 1, 6 + i, TERM_WHITE, '|');
	  
	  
	  /* Reset redraw flag */
	  redraw = FALSE;
	}
      
      if (g_cur != grp_old)
	{
	  grp_old = g_cur;
	  o_cur = 0;
	  g_o_count = g_offset[g_cur+1] - g_offset[g_cur];
	  menu_set_filter(&object_menu, obj_list + g_offset[g_cur], g_o_count);
	  group_menu.cursor = g_cur;
	  object_menu.cursor = 0;
	}
      
      /* HACK ... */
      if (!visual_list)
	{
	  /* ... The object menu may be browsing the entire group... */
	  o_funcs.is_visual = FALSE;
	  menu_set_filter(&object_menu, obj_list + g_offset[g_cur], g_o_count);
	  object_menu.cursor = o_cur;
	}
      else
	{
	  /* ... or just a single element in the group. */
	  o_funcs.is_visual = TRUE;
	  menu_set_filter(&object_menu, obj_list + o_cur + g_offset[g_cur], 1);
	  object_menu.cursor = 0;
	}
      
      oid = obj_list[g_offset[g_cur]+o_cur];
      
      /* Print prompt and buttons */
      {
	const char *pedit = (!o_funcs.xattr) ? "" :
	  (!(attr_idx|char_idx) ? ", 'c' to copy" : ", 'c', 'p' to paste");
	const char *xtra = o_funcs.xtra_prompt ? o_funcs.xtra_prompt(oid) : "";
	const char *pvs = "";
	char prompt[70];
	
        if (visual_list) pvs = ", ENTER to accept";
        else if (o_funcs.xattr) pvs = ", 'v' for visuals";
        
        strnfmt(prompt, sizeof(prompt), 
		"<dir>, 'r' to recall%s%s%s, ESC", pvs, pedit, xtra);
	if (small_screen) strcpy(prompt, "");
	prt(prompt, hgt - 1, 0);
	prompt_end = (small_screen ? 0 : strlen(prompt));
	add_button("ESC", ESCAPE);
	add_button("r", 'r');
	if (o_funcs.xattr)
	  {
	    if (attr_idx|char_idx)
	      {
		add_button("p", 'p');
		kill_button('c');
	      }
	    else
	      {
		kill_button('p');
		add_button("c", 'c');
	      }
	  }
	else
	  {
	    kill_button('p');
	    kill_button('c');
	  }
	if (visual_list) 
	  {
	    add_button("Ent", '\r');
	    kill_button('v');
	  }
	else
	  {
	    kill_button('\r');
	    add_button("v", 'v');
	  }
	update_statusline();
      }
      
      if (do_swap)
	{
	  do_swap = FALSE;
	  swap(active_menu, inactive_menu);
	  swap(active_cursor, inactive_cursor);
	  panel = 1 - panel;
	}
      
      if (g_funcs.summary && !visual_list)
        {
	  g_funcs.summary(g_cur, obj_list, g_o_count, g_offset[g_cur],
			  object_menu.boundary.row + 
			  object_menu.boundary.page_rows,
			  object_region.col);
        }
      
      menu_refresh(inactive_menu);
      menu_refresh(active_menu);
      
      handle_stuff();
      
      if (visual_list)
	{
	  display_visual_list(g_name_len + 3, 7, browser_rows-1,
			      wid - (g_name_len + 3), attr_top, char_left);
	  place_visual_list_cursor(g_name_len + 3, 7, *o_funcs.xattr(oid), 
				   *o_funcs.xchar(oid), attr_top, char_left);
	}
      
      if (delay)
	{
	  /* Force screen update */
	  Term_fresh();
	  
	  /* Delay */
	  Term_xtra(TERM_XTRA_DELAY, delay);
	  
	  delay = 0;
	}
      
      ke = inkey_ex();
      
      /* Do visual mode command if needed */
      if (o_funcs.xattr && o_funcs.xchar &&
	  visual_mode_command(ke, &visual_list,
			      browser_rows-1, wid - (g_name_len + 3),
			      &attr_top, &char_left,
			      o_funcs.xattr(oid), o_funcs.xchar(oid),
			      g_name_len + 3, 7, &delay))
	{
	  continue;
	}
      
      if (ke.type == EVT_MOUSE)
	{
	  /* Change active panels */
	  if (region_inside(&inactive_menu->boundary, &ke))
	    {
	      swap(active_menu, inactive_menu);
	      swap(active_cursor, inactive_cursor);
              panel = 1-panel;
            }
        }
      ke0 = run_event_loop(&active_menu->target, 0, &ke);
      if (ke0.type != EVT_AGAIN) ke = ke0;
      switch(ke.type) {
      case EVT_KBRD:
        {
          break;
	}
	
      case ESCAPE:
	{
	  flag = TRUE;
	  continue;
	}
	
      case EVT_SELECT:
	{
	  if (panel == 1 && oid >= 0 && o_cur == active_menu->cursor)
	    {
	      o_funcs.lore(oid);
	      redraw = TRUE;
	    }
	}
	
      case EVT_MOVE:
	{
	  *active_cursor = active_menu->cursor;
	  continue;
	}
	
      case EVT_BACK:
	{
	  if (panel == 1)
	    do_swap = TRUE;
	}
	
	/* XXX Handle EVT_RESIZE */
	
      default:
	{
	  continue;
	}
      }
      
      switch (ke.key)
        {
          
        case ESCAPE:
          {
            flag = TRUE;
	    break;
	  }
	  
	case 'R':
	case 'r':
	  {
	    /* Recall on screen */
	    if (oid >= 0)
	      o_funcs.lore(oid);
	    
	    redraw = TRUE;
	    break;
	  }
	  
	default:
	  {
	    int d = target_dir(ke.key);
	    
	    /* Handle key-driven motion between panels */
	    if (ddx[d] && ((ddx[d] < 0) == (panel == 1)))
	      {
		/* Silly hack -- diagonal arithmetic */
		*inactive_cursor += ddy[d];
		if (*inactive_cursor < 0) *inactive_cursor = 0;
		else if (g_cur >= grp_cnt) g_cur = grp_cnt -1;
		else if (o_cur >= g_o_count) o_cur = g_o_count-1;
		do_swap = TRUE;
	      }
	    else if (o_funcs.xtra_act)
	      {
		o_funcs.xtra_act(ke.key, oid);
	      }
	    
	    break;
	  }
        }
    }
  /* Restore buttons */
  kill_all_buttons();
  restore_buttons();
  update_statusline();
  
  /* Restore roguelike option */
  rogue_like_commands = omode;
  
  /* Prompt */
  if (!grp_cnt)
    prt(format("No %s known.", title), 15, 0);
  
  FREE(g_names);
  FREE(g_offset);
  FREE(g_list);
}

/**
 * Display visuals.
 */
static void display_visual_list(int col, int row, int height, int width, 
                                byte attr_top, char char_left)
{
  int i, j;
  
  /* Clear the display lines */
  for (i = 0; i < height; i++)
    Term_erase(col, row + i, width);
  
  width = logical_width(width);
  
  /* Display lines until done */
  for (i = 0; i < height; i++)
    {
      /* Display columns until done */
      for (j = 0; j < width; j++)
	{
	  byte a;
	  char c;
	  int x = col + actual_width(j);
	  int y = row + actual_width(i);
	  int ia, ic;
	  
	  ia = attr_top + i;
	  ic = char_left + j;
	  
	  a = (byte)ia;
	  c = (char)ic;
	  
	  /* Display symbol */
	  big_pad(x, y, a, c);
	}
    }
}


/**
 * Place the cursor at the collect position for visual mode
 */
static void place_visual_list_cursor(int col, int row, byte a, byte c, 
                                     byte attr_top, byte char_left)
{
  int i = a - attr_top;
  int j = c - char_left;
  
  int x = col + actual_width(j);
  int y = row + actual_height(i);
  
  /* Place the cursor */
  Term_gotoxy(x, y);
}


/**
 *  Do visual mode command -- Change symbols
 */
static bool visual_mode_command(event_type ke, bool *visual_list_ptr, 
				int height, int width, 
				byte *attr_top_ptr, char *char_left_ptr, 
				byte *cur_attr_ptr, char *cur_char_ptr,
				int col, int row, int *delay)
{
  static byte attr_old = 0;
  static char char_old = 0;
  
  int frame_left = logical_width(10);
  int frame_right = logical_width(10);
  int frame_top = logical_height(4);
  int frame_bottom = logical_height(4);
  
  switch (ke.key)
    {
    case ESCAPE:
      {
	if (*visual_list_ptr)
	  {
	    /* Cancel change */
	    *cur_attr_ptr = attr_old;
	    *cur_char_ptr = char_old;
	    *visual_list_ptr = FALSE;
	    
	    return TRUE;
	  }
	
	break;
      }
      
    case '\n':
    case '\r':
      {
	if (*visual_list_ptr)
	  {
	    /* Accept change */
	    *visual_list_ptr = FALSE;
	    return TRUE;
	  }
	
	break;
      }
      
    case 'V':
    case 'v':
      {
	if (!*visual_list_ptr)
	  {
	    *visual_list_ptr = TRUE;
	    
	    *attr_top_ptr = (byte)MAX(0, (int)*cur_attr_ptr - frame_top);
	    *char_left_ptr = (char)MAX(-128, (int)*cur_char_ptr - frame_left);
	    
	    attr_old = *cur_attr_ptr;
	    char_old = *cur_char_ptr;
	  }
	else
	  {
	    /* Cancel change */
	    *cur_attr_ptr = attr_old;
	    *cur_char_ptr = char_old;
	    *visual_list_ptr = FALSE;
	  }
	
	return TRUE;
      }
      
    case 'C':
    case 'c':
      {
	/* Set the visual */
	attr_idx = *cur_attr_ptr;
	char_idx = *cur_char_ptr;
	
	return TRUE;
      }
      
    case 'P':
    case 'p':
      {
	if (attr_idx)
	  {
	    /* Set the char */
	    *cur_attr_ptr = attr_idx;
	    *attr_top_ptr = (byte)MAX(0, (int)*cur_attr_ptr - frame_top);
	  }
	
	if (char_idx)
          {
            /* Set the char */
            *cur_char_ptr = char_idx;
            *char_left_ptr = (char)MAX(-128, 
                                       (int)*cur_char_ptr - frame_left);
          }
        
        return TRUE;
      }
      
    default:
      {
	if (*visual_list_ptr)
	  {
	    int eff_width = actual_width(width);
	    int eff_height = actual_height(height);
	    int d = target_dir(ke.key);
	    byte a = *cur_attr_ptr;
	    char c = *cur_char_ptr;
	    
	    /* Get mouse movement */
	    if (ke.key == '\xff')
	      {
		int my = ke.mousey - row;
		int mx = ke.mousex - col;
		
                my = logical_height(my);
                mx = logical_width(mx);
                
                if ((my >= 0) && (my < eff_height) && (mx >= 0) && 
                    (mx < eff_width)
                    && ((ke.index) || (a != *attr_top_ptr + my)
                        || (c != *char_left_ptr + mx)) )
                  {
		    /* Set the visual */
		    *cur_attr_ptr = a = *attr_top_ptr + my;
		    *cur_char_ptr = c = *char_left_ptr + mx;
		    
                    /* Move the frame */
                    if (*char_left_ptr > MAX(-128, (int)c - frame_left))
                      (*char_left_ptr)--;
                    if (*char_left_ptr + eff_width < 
                        MIN(127, (int)c + frame_right))
                      (*char_left_ptr)++;
                    if (*attr_top_ptr > MAX(0, (int)a - frame_top))
                      (*attr_top_ptr)--;
                    if (*attr_top_ptr + eff_height < 
                        MIN(255, (int)a + frame_bottom))
                      (*attr_top_ptr)++;
                    
                    /* Delay */
		    *delay = 100;
		    
		    /* Accept change */
		    if (ke.index) *visual_list_ptr = FALSE;
		    
		    return TRUE;
		  }
		
		/* Cancel change */
		else if (ke.index)
		  {
		    *cur_attr_ptr = attr_old;
		    *cur_char_ptr = char_old;
		    *visual_list_ptr = FALSE;
		    
		    return TRUE;
		  }
	      }
	    else
	      {
                /* Restrict direction */
                if ((a == 0) && (ddy[d] < 0)) d = 0;
                if ((c == (char) -128) && (ddx[d] < 0)) d = 0;
		if ((a == 255) && (ddy[d] > 0)) d = 0;
		if ((c == 127) && (ddx[d] > 0)) d = 0;
                
                a += ddy[d];
                c += ddx[d];
		
		/* Set the visual */
		*cur_attr_ptr = a;
                *cur_char_ptr = c;
                
                /* Move the frame */
                if ((ddx[d] < 0) && *char_left_ptr > 
                    MAX(-128, (int)c - frame_left))
                  (*char_left_ptr)--;
                if ((ddx[d] > 0) && *char_left_ptr + eff_width <
                    MIN(127, (int)c + frame_right))
		  (*char_left_ptr)++;
		
		if ((ddy[d] < 0) && *attr_top_ptr > MAX(0, (int)a - frame_top))
		  (*attr_top_ptr)--;
		if ((ddy[d] > 0) && *attr_top_ptr + eff_height <
		    MIN(255, (int)a + frame_bottom))
		  (*attr_top_ptr)++;
		
		if (d != 0) return TRUE;
	      }
	  }
      }
    }
  
  /* Visual mode command is not used */
  return FALSE;
}


/** The following sections implement "subclasses" of the
 * abstract classes represented by member_funcs and group_funcs
 */

/** =================== MONSTERS ==================================== */
/** Many-to-many grouping - use default auxiliary join */

/**
 * Display a monster
 */
static void display_monster(int col, int row, bool cursor, int oid)
{
  /* HACK Get the race index. (Should be a wrapper function) */
  int r_idx = default_join[oid].oid;
  
  /* Access the race */
  monster_race *r_ptr = &r_info[r_idx];
  monster_lore *l_ptr = &l_list[r_idx];
  
  /* Choose colors */
  byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
  byte a = r_ptr->x_attr;
  byte c = r_ptr->x_char;
  
  /* Display the name */
  c_prt(attr, r_name + r_ptr->name, row, col);
  
  if (use_dbltile || use_trptile)
    return;

  /* Display symbol */
  big_pad((small_screen ? 40 : 66), row, a, c);
  
  /* Display kills */
  if (r_ptr->flags1 & (RF1_UNIQUE))
    put_str(format("%s", (r_ptr->max_num == 0)?  " dead" : "alive"), row, 70);
  else put_str(format("%5d", l_ptr->pkills), row, 70);
}


static int m_cmp_race(const void *a, const void *b) {
  monster_race *r_a = &r_info[default_join[*(int*)a].oid];
  monster_race *r_b = &r_info[default_join[*(int*)b].oid];
  int gid = default_join[*(int*)a].gid;
  
  /* Group by */
  int c = gid - default_join[*(int*)b].gid;
  if (c) return c;
  
  /* Order results */
  c = r_a->d_char - r_b->d_char;
  if (c && gid != 0)
    {
      /* UNIQUE group is ordered by level & name only */
      /* Others by order they appear in the group symbols */
      return strchr(monster_group[gid].chars, r_a->d_char)
	- strchr(monster_group[gid].chars, r_b->d_char);
    }
  c = r_a->level - r_b->level;
  if (c) return c;
  
  return strcmp(r_name + r_a->name, r_name + r_b->name);
}

static char *m_xchar(int oid) { return &r_info[default_join[oid].oid].x_char; }
static byte *m_xattr(int oid) { return &r_info[default_join[oid].oid].x_attr; }
static const char *race_name(int gid) { return monster_group[gid].name; }
static void mon_lore(int oid) { screen_roff(default_join[oid].oid); 
 inkey_ex(); }

static void mon_summary(int gid, const int *object_list, int n, int top, 
                        int row, int col)
{
  int i;
  int kills = 0;
  
  /* Access the race */
  
  for (i = 0; i < n; i++)
    {
      int oid = default_join[object_list[i+top]].oid;
      kills += l_list[oid].pkills;
    }
  
  /* Different display for the first item if we've got uniques to show */
  if (gid == 0 && ((&r_info[default_join[object_list[0]].oid])->flags1 & 
		   (RF1_UNIQUE)))
    {
      c_prt(TERM_L_BLUE, format("%d known uniques, %d slain.", n, kills),
	    row, col);
    }
  else
    {
      int tkills = 0;
      
      for (i = 0; i < z_info->r_max; i++) 
        tkills += l_list[i].pkills;
      
      c_prt(TERM_L_BLUE, format("Creatures slain: %d/%d (in group/in total)", 
                                kills, tkills), row, col);
    }
}

static int count_known_monsters(void)
{
  int m_count = 0;
  int i;
  size_t j;
  
  for (i = 0; i < z_info->r_max; i++)
    {
      monster_race *r_ptr = &r_info[i];
      if (!cheat_know && !l_list[i].sights) continue;
      if (!r_ptr->name) continue;
      
      if (r_ptr->flags1 & RF1_UNIQUE) m_count++;
      
      for (j = 1; j < N_ELEMENTS(monster_group) - 1; j++)
	{
	  const char *pat = monster_group[j].chars;
	  if (strchr(pat, r_ptr->d_char)) m_count++;
	}
    }
  
  return m_count;
}

/**
 * Display known monsters.
 */
static void do_cmd_knowledge_monsters(void *obj, const char *name)
{
  group_funcs r_funcs = {N_ELEMENTS(monster_group), FALSE, race_name,
			 m_cmp_race, default_group, mon_summary};
  
  member_funcs m_funcs = {display_monster, mon_lore, m_xchar, m_xattr, 0, 0, 
			  0};
  
  
  int *monsters;
  int m_count = 0;
  int i;
  size_t j;
  
  for (i = 0; i < z_info->r_max; i++)
    {
      monster_race *r_ptr = &r_info[i];
      if (!cheat_know && !l_list[i].sights) continue;
      if (!r_ptr->name) continue;
      
      if (r_ptr->flags1 & RF1_UNIQUE) m_count++;
      
      for (j = 1; j < N_ELEMENTS(monster_group) - 1; j++)
	{
	  const char *pat = monster_group[j].chars;
	  if (strchr(pat, r_ptr->d_char)) m_count++;
	}
    }
  
  C_MAKE(default_join, m_count, join_t);
  C_MAKE(monsters, m_count, int);
  
  m_count = 0;
  for (i = 0; i < z_info->r_max; i++)
    {
      monster_race *r_ptr = &r_info[i];
      if (!cheat_know && !l_list[i].sights) continue;
      if (!r_ptr->name) continue;
      
      for (j = 0; j < N_ELEMENTS(monster_group)-1; j++)
	{
	  const char *pat = monster_group[j].chars;
	  if (j == 0 && !(r_ptr->flags1 & RF1_UNIQUE)) 
	    continue;
	  else if (j > 0 && !strchr(pat, r_ptr->d_char))
	    continue;
	  
	  monsters[m_count] = m_count;
	  default_join[m_count].oid = i;
	  default_join[m_count++].gid = j;
	}
    }
  
  display_knowledge("monsters", monsters, m_count, r_funcs, m_funcs,
		    "          Sym  Kills");
  KILL(default_join);
  FREE(monsters);
}

/* =================== ARTIFACTS ==================================== */
/* Many-to-one grouping */

/**
 * Display an artifact label
 */
static void display_artifact(int col, int row, bool cursor, int oid)
{
  char o_name[80];
  object_type object_type_body;
  object_type *o_ptr = &object_type_body;
  
  /* Choose a color */
  byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
  
  /* Get local object */
  o_ptr = &object_type_body;
  
  /* Wipe the object */
  object_wipe(o_ptr);
  
  /* Make fake artifact */
  make_fake_artifact(o_ptr, oid);
  
  /* Get its name */
  object_desc_spoil(o_name, o_ptr, TRUE, 0);
  
  /* Display the name */
  c_prt(attr, o_name, row, col);
}

/**
 * Show artifact lore
 */
static void desc_art_fake(int a_idx)
{
  object_type *o_ptr;
  object_type object_type_body;
  
  /* Get local object */
  o_ptr = &object_type_body;
  
  /* Wipe the object */
  object_wipe(o_ptr);
  
  /* Make fake artifact */
  make_fake_artifact(o_ptr, a_idx);
  o_ptr->ident |= IDENT_KNOWN;
  
  /* Hack -- Handle stuff */
  handle_stuff();
  
  object_info_screen(o_ptr, TRUE);
}

static int a_cmp_tval(const void *a, const void *b)
{
  artifact_type *a_a = &a_info[*(int*)a];
  artifact_type *a_b = &a_info[*(int*)b];
  
  /*group by */
  int ta = obj_group_order[a_a->tval];
  int tb = obj_group_order[a_b->tval];
  int c = ta - tb;
  if (c) return c;
  
  /* order by */
  c = a_a->sval - a_b->sval;
  if (c) return c;
  return strcmp(a_name+a_a->name, a_name+a_b->name);
}

static const char *kind_name(int gid) { return object_text_order[gid].name; }
static int art2gid(int oid) { return obj_group_order[a_info[oid].tval]; }


/** If 'artifacts' is NULL, it counts the number of known artifacts, otherwise
   it collects the list of known artifacts into 'artifacts' as well. */
static int collect_known_artifacts(int *artifacts, size_t artifacts_len)
{
  int a_count = 0;
  int i, j;
  
  if (artifacts)
    assert(artifacts_len >= z_info->a_max);
  ;
  
  for (j = 0; j < z_info->a_max; j++)
    {
      /* If the artifact has been created (or we're cheating) */
      if ((cheat_xtra || a_info[j].creat_turn) && a_info[j].name)
	{
	  bool valid = TRUE;
	  
	  for (i = 0; !cheat_xtra && i < z_info->o_max; i++)
	    {
	      int a = o_list[i].name1;
	      
	      /* If we haven't actually identified the artifact yet */
	      if (a && a == j && !object_known_p(&o_list[i]))
		{
		  valid = FALSE;
		}
	    }
	  
	  if (valid)
	    {
	      if (artifacts)
		artifacts[a_count++] = j;
	      else
		a_count++;
	    }
	}
    }
  
  return a_count;
}

/**
 * Display known artifacts
 */
static void do_cmd_knowledge_artifacts(void *obj, const char *name)
{
  /* HACK -- should be TV_MAX */
  group_funcs obj_f = {TV_GOLD, FALSE, kind_name, a_cmp_tval, art2gid, 0};
  member_funcs art_f = {display_artifact, desc_art_fake, 0, 0, 0, 0, 0};
  
  
  int *artifacts;
  int a_count = 0;
  
  C_MAKE(artifacts, z_info->a_max, int);
  
  
  /* Collect valid artifacts */
  a_count = collect_known_artifacts(artifacts, z_info->a_max);
  
  display_knowledge("artifacts", artifacts, a_count, obj_f, art_f, 0);
  FREE(artifacts);
}

/* =================== EGO ITEMS  ==================================== */
/* Many-to-many grouping (uses default join) */

/* static u16b *e_note(int oid) {return &e_info[default_join[oid].oid].note;}*/
static const char *ego_grp_name(int gid) 
{ 
  return object_text_order[gid].name; 
}

static void display_ego_item(int col, int row, bool cursor, int oid)
{
  /* HACK: Access the object */
  ego_item_type *e_ptr = &e_info[default_join[oid].oid];
  
  /* Choose a color */
  byte attr = curs_attrs[0 != (int)e_ptr->everseen][0 != (int)cursor];
  
  /* Display the name */
  c_prt(attr, e_name + e_ptr->name, row, col);
}

/**
 * Describe fake ego item "lore"
 */
static void desc_ego_fake(int oid)
{
  /* Hack: dereference the join */
  const char *xtra[] = { "vulnerability", "pair of small resistances", 
			 "resistance", "high resistance", "sustain", "ability",
			 "curse" };
  int num = 0, i;
  
  int e_idx = default_join[oid].oid;
  ego_item_type *e_ptr = &e_info[e_idx];
  
  object_type dummy;
  WIPE(&dummy, dummy);
  
  /* Save screen */
  screen_save();
  
  /* Set text_out hook */
  text_out_hook = text_out_to_screen;
  
  /* Dump the name */
  c_prt(TERM_L_BLUE, format("%s %s", ego_grp_name(default_group(oid)),
			    e_name + e_ptr->name), 0, 0);
  
  /* Begin recall */
  Term_gotoxy(0, 1);
  if (e_ptr->text)
    {
      int x, y;
      text_out(e_text + e_ptr->text);
      Term_locate(&x, &y);
      Term_gotoxy(0, y+1);
    }
  
  
  /* Count ego flags */
  for (i = 0; i < 7; i++)
    {
      if (e_ptr->flags_kind & (KF_RAND_RES_NEG << i)) num++;
    }

  if (num) text_out("It provides ");

  /* List ego flags */
  for (i = 0; i < 7; i++)
    {
      char punct[10];
      if (e_ptr->flags_kind & (KF_RAND_RES_NEG << i))
	{
	  if (num > 2) strcpy(punct, ", ");
	  else if (num == 2) strcpy(punct, " and ");
	  else if (num == 1) strcpy(punct, ".");
	  num--;
	  text_out(format("one random %s%s", xtra[i], punct));
	}
    }

  
  Term_flush();
  
  (void)inkey_ex();
  
  screen_load();
}

/** TODO? Currently ego items will order by e_idx */
static int e_cmp_tval(const void *a, const void *b)
{
  ego_item_type *ea = &e_info[default_join[*(int*)a].oid];
  ego_item_type *eb = &e_info[default_join[*(int*)b].oid];
  
  /* Group by */
  int c = default_join[*(int*)a].gid - default_join[*(int*)b].gid;
  if (c) return c;
  
  /* Order by */
  return strcmp(e_name + ea->name, e_name + eb->name);
}

/**
 * Display known ego_items
 */
static void do_cmd_knowledge_ego_items(void *obj, const char *name)
{
  group_funcs obj_f =
    {TV_GOLD, FALSE, ego_grp_name, e_cmp_tval, default_group, 0};
  
  member_funcs ego_f = {display_ego_item, desc_ego_fake, 0, 0, 0, 0, 0};
  
  int *egoitems;
  int e_count = 0;
  int i, j;
  
  /* HACK: currently no more than 3 tvals for one ego type */
  C_MAKE(egoitems, z_info->e_max * EGO_TVALS_MAX, int);
  C_MAKE(default_join, z_info->e_max * EGO_TVALS_MAX, join_t);
  
  for (i = 0; i < z_info->e_max; i++)
    {
      if (e_info[i].everseen || cheat_xtra)
        {
          for (j = 0; j < EGO_TVALS_MAX && e_info[i].tval[j]; j++)
            {
	      int gid = obj_group_order[e_info[i].tval[j]];
	      
	      /* Ignore duplicate gids */
	      if (j > 0 && gid == default_join[e_count-1].gid)
		continue;
	      egoitems[e_count] = e_count;
	      default_join[e_count].oid = i;
	      default_join[e_count++].gid = gid; 
	    }
	}
    }
  
  display_knowledge("ego items", egoitems, e_count, obj_f, ego_f, "");
  
  KILL(default_join);
  FREE(egoitems);
}

/* =================== ORDINARY OBJECTS  =================================== */
/* Many-to-one grouping */

/**
 * Display the objects in a group.
 */
static void display_object(int col, int row, bool cursor, int oid)
{
  int k_idx = oid;
  
  /* Access the object */
  object_kind *k_ptr = &k_info[k_idx];
  const char *inscrip = get_autoinscription(oid);
  
  char o_name[80];
  
  
  /* Choose a color */
  bool aware = (k_ptr->flavor == 0) || (k_ptr->aware);
  byte a = (aware && k_ptr->x_attr) ?
    k_ptr->x_attr : flavor_info[k_ptr->flavor].x_attr;
  byte c = aware ? k_ptr->x_char : flavor_info[k_ptr->flavor].x_char;
  byte attr = curs_attrs[(int)k_ptr->flavor == 0 || k_ptr->aware][(int)cursor];
  
  /* Symbol is unknown.  This should never happen.*/    
  if (!k_ptr->aware && !k_ptr->flavor && !p_ptr->wizard)
    {
      assert(0);
      c = ' ';
      a = TERM_DARK;
    }
  
  /* Tidy name */
  strip_name(o_name, k_idx, cheat_know);
  
  /* Display the name */
  c_prt(attr, o_name, row, col);
  
  /* Show autoinscription if around */
  if (aware && inscrip)
    c_put_str(TERM_YELLOW, inscrip, row, 55);
  
  /* Hack - don't use if double tile */
  if (use_dbltile || use_trptile)
    return;
  
  /* Display symbol */
  big_pad(76, row, a, c);
}

/**
 * Describe fake object
 */
static void desc_obj_fake(int k_idx)
{
  object_type object_type_body;
  object_type *o_ptr = &object_type_body;
  
  /* Wipe the object */
  object_wipe(o_ptr);
  
  /* Create the artifact */
  object_prep(o_ptr, k_idx);
  
  /* Hack -- its in the store */
  if (k_info[k_idx].aware) o_ptr->ident |= (IDENT_STORE);
  
  /* It's fully know */
  if (!k_info[k_idx].flavor) object_known(o_ptr);
  
  /* Hack -- Handle stuff */
  handle_stuff();
  
  /* Describe */
  object_info_screen(o_ptr, FALSE);
}

static int o_cmp_tval(const void *a, const void *b)
{
  object_kind *k_a = &k_info[*(int*)a];
  object_kind *k_b = &k_info[*(int*)b];
  
  /* Group by */
  int ta = obj_group_order[k_a->tval];
  int tb = obj_group_order[k_b->tval];
  int c = ta - tb;
  if (c) return c;
  
  /* Order by */
  c = k_a->aware - k_b->aware;
  if (c) return -c; /* aware has low sort weight */
  if (!k_a->aware)
    {
      return strcmp(flavor_text + flavor_info[k_a->flavor].text,
		    flavor_text +flavor_info[k_b->flavor].text);
    }
  c = k_a->cost - k_b->cost;
  if (c) return c;
  
  return strcmp(k_name + k_a->name, k_name + k_b->name);
}

static int obj2gid(int oid) { return obj_group_order[k_info[oid].tval]; }

static char *o_xchar(int oid)
{
  object_kind *k_ptr = &k_info[oid];
  
  if (!k_ptr->flavor || k_ptr->aware)
    return &k_ptr->x_char;
  else
    return &flavor_info[k_ptr->flavor].x_char;
}

static byte *o_xattr(int oid)
{
  object_kind *k_ptr = &k_info[oid];
  
  if (!k_ptr->flavor || k_ptr->aware)
    return &k_ptr->x_attr;
  else
    return &flavor_info[k_ptr->flavor].x_attr;
}

/**
 * Display special prompt for object inscription.
 */
static const char *o_xtra_prompt(int oid)
{
  object_kind *k_ptr = &k_info[oid];
  s16b idx = get_autoinscription_index(oid);
  
  const char *no_insc = ", '{'";
  const char *with_insc = ", '{', '}'";
  
  
  /* Forget it if we've never seen the thing */
  if (k_ptr->flavor && !k_ptr->aware)
    return "";
  
  /* If it's already inscribed */
  if (idx != -1)
    return with_insc;
  
  return no_insc;
}

/**
 * Special key actions for object inscription.
 */
static void o_xtra_act(char ch, int oid)
{
  object_kind *k_ptr = &k_info[oid];
  s16b idx = get_autoinscription_index(oid);
  
  /* Forget it if we've never seen the thing */
  if (!k_ptr->everseen)
    return;
  
  /* Uninscribe */
  if (ch == '}')
    {
      if (idx) remove_autoinscription(oid);
      return;
    }
  
  /* Inscribe */
  else if (ch == '{')
    {
      char note_text[80] = "";
      
      /* Avoid the prompt getting in the way */
      screen_save();
      
      /* Prompt */
      prt("Inscribe with: ", 0, 0);
      
      /* Default note */
      if (idx != -1)
	strnfmt(note_text, sizeof(note_text), "%s", get_autoinscription(oid));
      
      /* Get an inscription */
      if (askfor_aux(note_text, sizeof(note_text), NULL))
	{
	  /* Remove old inscription if existent */
	  if (idx != -1)
	    remove_autoinscription(oid);
	  
	  /* Add the autoinscription */
	  add_autoinscription(oid, note_text);
	  
	  /* Notice stuff (later) */
	  p_ptr->notice |= (PN_AUTOINSCRIBE);
	  p_ptr->window |= (PW_INVEN | PW_EQUIP);
	}
      
      /* Reload the screen */
      screen_load();
    }
}

/* Hack - stop short of special arts */
#define LAST_NORMAL 730

/**
 * Display known objects
 */
void do_cmd_knowledge_objects(void *obj, const char *name)
{
  group_funcs kind_f = {TV_GOLD, FALSE, kind_name, o_cmp_tval, obj2gid, 0};
  member_funcs obj_f = {display_object, desc_obj_fake, o_xchar, o_xattr, 
			o_xtra_prompt, o_xtra_act, 0};
  
  int *objects;
  int o_count = 0;
  int i;
  
  C_MAKE(objects, LAST_NORMAL, int);
  
  for (i = 0; i < LAST_NORMAL; i++)
    {
      if (k_info[i].everseen || k_info[i].flavor || cheat_xtra)
        {
          int c = obj_group_order[k_info[i].tval];
          if (c >= 0) objects[o_count++] = i;
	}
    }
  
  display_knowledge("known objects", objects, o_count, kind_f, obj_f, 
		    "Inscribed          Sym");
  
  FREE(objects);
}

/* =================== TERRAIN FEATURES ==================================== */
/* Many-to-one grouping */

/**
 * Display the features in a group.
 */
static void display_feature(int col, int row, bool cursor, int oid )
{
  /* Get the feature index */
  int f_idx = oid;
  
  /* Access the feature */
  feature_type *f_ptr = &f_info[f_idx];
  
  /* Choose a color */
  byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
  
  /* Display the name */
  c_prt(attr, f_name + f_ptr->name, row, col);
  
  
  if (use_dbltile || use_trptile) return;
  
  /* Display symbol */
  big_pad(68, row, f_ptr->x_attr, f_ptr->x_char);
  
  /* ILLUMINATION AND DARKNESS GO HERE */
  
}


static int f_cmp_fkind(const void *a, const void *b) {
  feature_type *fa = &f_info[*(int*)a];
  feature_type *fb = &f_info[*(int*)b];
  /* group by */
  int c = feat_order(*(int*)a) - feat_order(*(int*)b);
  if (c) return c;
  /* order by feature name */
  return strcmp(f_name + fa->name, f_name + fb->name);
}

static const char *fkind_name(int gid) { return feature_group_text[gid]; }
static byte *f_xattr(int oid) { return &f_info[oid].x_attr; }
static char *f_xchar(int oid) { return &f_info[oid].x_char; }
static void feat_lore(int oid) { 
  feature_type *f_ptr = &f_info[oid];

  /* Dump the name */
  c_prt(TERM_L_BLUE, format("- %s", f_name + f_ptr->name), 0, 3);

  /* Display symbol */
  big_pad(0, 0, f_ptr->x_attr, f_ptr->x_char);
  
  /* Dump the description */
  Term_gotoxy(0, 1);
  if (f_ptr->text)
    {
      text_out_indent = 0;
      text_out_to_screen(TERM_WHITE, f_text + f_ptr->text);
      text_out_to_screen(TERM_WHITE, "\n");
    }
  

  inkey_ex(); 
}

/**
 * Interact with feature visuals.
 */
static void do_cmd_knowledge_features(void *obj, const char *name)
{
  group_funcs fkind_f = {N_ELEMENTS(feature_group_text), FALSE,
			 fkind_name, f_cmp_fkind, feat_order, 0};
  
  member_funcs feat_f = {display_feature, feat_lore, f_xchar, f_xattr, 0, 0, 
			 0};
  
  int *features;
  int f_count = 0;
  int i;
  C_MAKE(features, z_info->f_max, int);
  
  for (i = 0; i < z_info->f_max; i++)
    {
      if (f_info[i].name == 0) continue;
      features[f_count++] = i; /* Currently no filter for features */
    }
  
  display_knowledge("features", features, f_count, fkind_f, feat_f, 
		    "           Sym");
  FREE(features);
}

/* =================== HOMES AND STORES ==================================== */



void do_cmd_knowledge_home()  
{
  /* TODO */
}


/* =================== END JOIN DEFINITIONS ================================ */


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
  p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY | PR_BUTTONS);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1 |
                    PW_MESSAGE | PW_OVERHEAD | PW_MONSTER | PW_OBJECT |
                    PW_MONLIST | PW_ITEMLIST);
  
  /* Clear screen */
  Term_clear();

  /* Hack -- update */
  handle_stuff();
  
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
 * Map resizing whenever the main term changes size
 */
void resize_map(void)
{
  /* Only if the dungeon exists */
  if (!character_dungeon) return;
  
  /* Mega-Hack -- no panel yet */
  panel_row_min = 0;
  panel_row_max = 0;
  panel_col_min = 0;
  panel_col_max = 0;
  
  if (character_dungeon)
    {
      verify_panel();
    }
  
  /* Combine and Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Update torch */
  p_ptr->update |= (PU_TORCH);
  
  /* Update stuff */
  p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
  
  /* Forget view */
  p_ptr->update |= (PU_FORGET_VIEW);
  
  /* Update view */
  p_ptr->update |= (PU_UPDATE_VIEW);
  
  /* Update monsters */
  p_ptr->update |= (PU_MONSTERS);
  
  /* Redraw everything */
  p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
  
  /* Hack -- update */
  handle_stuff();
  
  /* Redraw */
  Term_redraw();
  
  /* Refresh */
  Term_fresh();
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
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER_0 | 
                    PW_PLAYER_1);
  
  /* Window stuff */
  p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | 
                    PW_OBJECT);
  
  /* Hack -- update */
  handle_stuff();
  
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
  event_type ke;
  
  int col = 0;
  int last_line = 0;
  int top_line = 0;

  cptr p;

  bool old_normal_screen = normal_screen;
  
  /* Prompt */
  p = "['c' change name, 'f' to file, scroll, or ESC]";
  
  /* Save screen */
  screen_save();

  /* Not normal */
  normal_screen = FALSE;
  prompt_end = (small_screen ? 0 : strlen(p) + 1);
  
  /* Adjust the buttons */
  backup_buttons();
  kill_all_buttons();
  add_button("ESC", ESCAPE);
  add_button("Spc", ' ');
  add_button("-", '-');
  add_button("c", 'c');
  add_button("f", 'f');
  add_button("->", ARROW_RIGHT);
  add_button("<-", ARROW_LEFT);

  /* Make the array of lines */
  C_WIPE(dumpline, DUMP_MAX_LINES, char_attr_line);
  last_line = make_dump(dumpline, 2);

  /* Forever */
  while (1)
    {
      /* Display the player */
      display_dump(dumpline, top_line, top_line + Term->hgt - 1, col);

      /* Clear the bottom line */
      prt("", Term->hgt - 1, 0);
      
      /* Prompt */
      Term_putstr(0, Term->hgt - 1, -1, TERM_WHITE, p);
     
      /* Draw the buttons */
      update_statusline();
 
      /* Query */
      ke = inkey_ex();
      
      /* Exit */
      if (ke.key == ESCAPE) break;
      
      /* Change name */
      if (ke.key == 'c')
        {
          get_name(FALSE);
	  (void) make_dump(dumpline, 2);
        }
      
      /* File dump */
      else if (ke.key == 'f')
	{
	  char ftmp[80];
	  
	  strnfmt(ftmp, sizeof ftmp, "%s.txt", op_ptr->base_name);
	  
	  if (get_string("File name: ", ftmp, 80))
	    {
	      if (ftmp[0] && (ftmp[0] != ' '))
		{
		  if (file_character(ftmp, dumpline, last_line))
		    msg_print("Character dump failed!");
		  else
		    msg_print("Character dump successful.");
		}
	    }
	}
      
      /* Scroll down */
      else if ((ke.key == '\xff')|| (ke.key == ARROW_DOWN))
	{
	  if (top_line + Term->hgt - 2 < last_line)
	    top_line++;
	}
      
      /* Page down */
      else if (ke.key == ' ')
	{
	  top_line = MIN(last_line - Term->hgt + 2, 
			 top_line + (Term->hgt - 2));
	}
      
      /* Scroll up */
      else if (ke.key == ARROW_UP)
	{
	  if (top_line)
	    top_line--;
	}
      
      /* Page up */
      else if (ke.key == '-')
	{
	  top_line -= (Term->hgt - 2) / 2;
	  if (top_line < 0) top_line = 0;
	}
      
      /* Scroll left */
      else if (ke.key == ARROW_LEFT)
	{
	  if (col)
	    col--;
	}
      
      /* Scroll right */
      else if (ke.key == ARROW_RIGHT)
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

  /* Normal again */
  normal_screen = old_normal_screen;
  prompt_end = 0;

  /* Adjust the buttons */
  restore_buttons();

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
 * Attempt to only hilite the matching portions of the string.
 */
void do_cmd_messages(void)
{
  event_type ke;
  
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
  n = message_num();
  
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
  
  /* Not normal */
  normal_screen = FALSE;
  prompt_end = (small_screen ? 0 : strlen(p) + 1);
  
  /* Adjust the buttons */
  backup_buttons();
  kill_all_buttons();
  add_button("ESC", ESCAPE);
  add_button("-", '-');
  add_button("=", '=');
  add_button("/", '/');
  add_button("p", 'p');
  add_button("n", 'n');
  add_button("+", '+');
  add_button("->", '6');
  add_button("<-", '4');

  /* Process requests until done */
  while (1)
    {
      /* Clear screen */
      Term_clear();
      
      /* Dump messages */
      for (j = 0; (j < hgt - 4) && (i + j < n); j++)
	{
	  cptr msg = message_str((s16b)(i+j));
	  byte attr = message_color((s16b)(i+j));
	  
	  /* Apply horizontal scroll */
	  msg = ((int)strlen(msg) >= q) ? (msg + q) : "";
	  
	  /* Dump the messages, bottom to top */
	  Term_putstr(0, hgt - 3 - j, -1, attr, msg);
	  
	  /* Hilite "shower" */
	  if (shower[0])
	    {
	      cptr str = msg;
	      
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
      update_statusline();
      
      /* Get a command */
      ke = inkey_ex();
      
      /* Exit on Escape */
      if (ke.key == ESCAPE) break;
      
      /* Hack -- Save the old index */
      j = i;
      
      /* Horizontal scroll */
      if (ke.key == '4')
	{
	  /* Scroll left */
	  q = (q >= wid / 2) ? (q - wid / 2) : 0;
	  
	  /* Success */
	  continue;
	}
      
      /* Horizontal scroll */
      if (ke.key == '6')
	{
	  /* Scroll right */
	  q = q + wid / 2;
	  
	  /* Success */
	  continue;
	}
      
      /* Hack -- handle show */
      if (ke.key == '=')
	{
	  /* Prompt */
	  prt("Show: ", hgt - 1, 0);
	  
	  /* Get a "shower" string, or continue */
	  if (!askfor_aux(shower, sizeof shower, NULL)) continue;
	  
	  /* Okay */
	  continue;
	}
      
      /* Hack -- handle find */
      if (ke.key == '/')
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
	      cptr msg = message_str(z);
	      
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
      if ((ke.key == 'p') || (ke.key == KTRL('P')) || (ke.key == ' '))
	{
	  /* Go older if legal */
	  if (i + 20 < n) i += 20;
	}
      
      /* Recall 10 older messages */
      if (ke.key == '+')
	{
	  /* Go older if legal */
	  if (i + 10 < n) i += 10;
	}
      
      /* Recall 1 older message */
      if ((ke.key == '8') || (ke.key == '\n') || (ke.key == '\r'))
	{
	  /* Go older if legal */
	  if (i + 1 < n) i += 1;
	}
      
      /* Recall 20 newer messages */
      if ((ke.key == 'n') || (ke.key == KTRL('N')))
	{
	  /* Go newer (if able) */
	  i = (i >= 20) ? (i - 20) : 0;
	}
      
      /* Recall 10 newer messages */
      if (ke.key == '-')
	{
	  /* Go newer (if able) */
	  i = (i >= 10) ? (i - 10) : 0;
	}
      
      /* Recall 1 newer messages */
      if (ke.key == '2')
	{
	  /* Go newer (if able) */
	  i = (i >= 1) ? (i - 1) : 0;
	}
      
      /* Scroll forwards or backwards using mouse clicks */
      if (ke.key == '\xff')
	{
	  if (ke.index)
	    {
	      if (ke.mousey <= hgt / 2)
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
	}
      
      /* Hack -- Error of some kind */
      if (i == j) bell(NULL);
    }
  
  /* Normal again */
  normal_screen = TRUE;
  prompt_end = 0;

  /* Adjust the buttons */
  restore_buttons();

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
 * Ask for a "user pref file" and process it.
 *
 * This function should only be used by standard interaction commands,
 * in which a standard "Command:" prompt is present on the given row.
 *
 * Allow absolute file names?  XXX XXX XXX
 */
static void do_cmd_pref_file_hack(long row)
{
  char ftmp[80];
  
  /* Prompt */
  prt("Command: Load a user pref file", row, 0);
  
  /* Prompt */
  prt("File: ", row + 2, 0);
  
  /* Default filename */
  strnfmt(ftmp, sizeof ftmp, "%s.prf", op_ptr->base_name);
  
  /* Ask for a file (or cancel) */
  if (!askfor_aux(ftmp, sizeof ftmp, NULL)) return;
  
  /* Process the given filename */
  if (process_pref_file(ftmp))
    {
      /* Mention failure */
      msg_format("Failed to load '%s'!", ftmp);
    }
  else
    {
      /* Mention success */
      msg_format("Loaded '%s'.", ftmp);
    }
  inkey_ex();
}


static void display_option(menu_type *menu, int oid,
			   bool cursor, int row, int col, int width)
{
  byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
  if (small_screen)
    c_prt(attr, format("%-35s: %s", option_text[oid], 
		       op_ptr->opt[oid] ? "yes" : "no "), row, col);
  else
    c_prt(attr, format("%-45s: %s  (%s)", option_desc[oid], 
		       op_ptr->opt[oid] ? "yes" : "no ", option_text[oid]),
	  row, col);
}

/**
 * Handle keypresses for an option entry.
 */
static bool update_option(char key, void *pgdb, int oid)
{
  /* Ignore arrow events */
  if (key == ARROW_LEFT || key == ARROW_RIGHT)
    return TRUE;
  
  switch (toupper((unsigned char) key))
    {
    case 'Y':
      {
	op_ptr->opt[oid] = TRUE;
	break;
      }
      
    case 'N':
      {
	op_ptr->opt[oid] = FALSE;
	break;
      }
      
    case '?':
      {
	show_file(format("optdesc.txt#%s", option_text[oid]), 
		  NULL, 0, 0);
	break;
      }
    default:
      {
	op_ptr->opt[oid] = !op_ptr->opt[oid];
	break;
      }
      
    }
  
  return TRUE;
}

static const menu_iter options_toggle_iter =
{
  0,
  NULL,
  NULL,
  display_option,	/* label */
  update_option		/* updater */
};

static menu_type option_toggle_menu;


/**
 * Interact with some options
 */
static void do_cmd_options_aux(void *vpage, cptr info)
{
  int page = (int)vpage;
  int opt[OPT_PAGE_PER];
  int i, n = 0;
  int cursor_pos = 0;
  
  menu_type *menu = &option_toggle_menu;
  menu->title = info;
  menu_layout(menu, &SCREEN_REGION);
  
  screen_save();
  clear_from(0);

  /* Filter the options for this page */
  for (i = 0; i < OPT_PAGE_PER; i++)
    {
      if (option_page[page][i] != OPT_NONE)
	opt[n++] = option_page[page][i];
    }
  
  menu_set_filter(menu, opt, n);
  menu->menu_data = vpage;
  
  menu_layout(menu, &SCREEN_REGION);
  
  add_button("?", '?');
  while (TRUE)
    {
      event_type cx;
      
      update_statusline();
      
      cx = menu_select(menu, &cursor_pos, EVT_MOVE);
      
      if (cx.key == ESCAPE)
	break;
      else if (cx.type == EVT_MOVE)
	cursor_pos = cx.index;
      else if (cx.type == EVT_SELECT && strchr("YN", toupper(cx.key)))
	cursor_pos++;
      
      cursor_pos = (cursor_pos+n)%n;
    }
  
  kill_button('?');
  
  /* Hack -- Notice use of any "cheat" options */
  for (i = OPT_cheat_start; i < OPT_adult_start; i++)
    {
      if (op_ptr->opt[i])
        {
          /* Set score option */
          op_ptr->opt[OPT_score_start + (i - OPT_cheat_start)] = TRUE;
        }
    }
  screen_load();
}


/**
 * Modify the "window" options
 */
static void do_cmd_options_win(void)
{
  int i, j, d;
  
  int y = 0;
  int x = 0;
  
  event_type ke;
  
  u32b old_flag[ANGBAND_TERM_MAX];
  
  
  /* Memorize old flags */
  for (j = 0; j < ANGBAND_TERM_MAX; j++)
    {
      old_flag[j] = op_ptr->window_flag[j];
    }
  
  
  /* Clear screen */
  clear_from(0);
  
  /* Interact */
  while (1)
    {
      /* Prompt */
      prt("Window flags (<dir> to move, 't' to toggle, or ESC)", 0, 0);
      
      /* Display the windows */
      for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
	  byte a = TERM_WHITE;
	  
	  cptr s = angband_term_name[j];
	  
	  /* Use color */
	  if (j == x) a = TERM_L_BLUE;
	  
	  /* Window name, staggered, centered */
	  Term_putstr(35 + j * 5 - strlen(s) / 2, 2 + j % 2, -1, a, s);
	}
      
      /* Display the options */
      for (i = 0; i < PW_MAX_FLAGS; i++)
	{
	  byte a = TERM_WHITE;
	  
	  cptr str = window_flag_desc[i];
	  
	  /* Use color */
	  if (i == y) a = TERM_L_BLUE;
	  
	  /* Unused option */
	  if (!str) str = "(Unused option)";
	  
	  /* Flag name */
	  Term_putstr(0, i + 5, -1, a, str);
	  
	  /* Display the windows */
	  for (j = 0; j < ANGBAND_TERM_MAX; j++)
	    {
	      byte a = TERM_WHITE;
	      
	      char c = '.';
	      
	      /* Use color */
	      if ((i == y) && (j == x)) a = TERM_L_BLUE;
	      
	      /* Active flag */
	      if (op_ptr->window_flag[j] & (1L << i)) c = 'X';
	      
	      /* Flag value */
	      Term_putch(35 + j * 5, i + 5, a, c);
	    }
	}
      
      /* Place Cursor */
      Term_gotoxy(35 + x * 5, y + 5);
      
      update_statusline();
      
      /* Get key */
      ke = inkey_ex();
      
      /* Allow escape */
      if ((ke.key == ESCAPE) || (ke.key == 'q')) break;
      
      /* Mouse interaction */
      if (ke.key == '\xff')
	{
	  int choicey = ke.mousey - 5;
	  int choicex = (ke.mousex - 35)/5;
	  
	  if ((choicey >= 0) && (choicey < PW_MAX_FLAGS)
	      && (choicex > 0) && (choicex < ANGBAND_TERM_MAX)
	      && !(ke.mousex % 5))
	    {
	      y = choicey;
	      x = (ke.mousex - 35)/5;
	    }
	  
	  /* Toggle using mousebutton later */
	  if (!ke.index) continue;
	}
      
      /* Toggle */
      if ((ke.key == '5') || (ke.key == 't') || (ke.key == '\n') || 
	  (ke.key == '\r') || ((ke.key == '\xff') && (ke.index)))
	{
	  /* Hack -- ignore the main window */
	  if (x == 0)
	    {
	      bell("Cannot set main window flags!");
	    }
	  
	  /* Toggle flag (off) */
	  else if (op_ptr->window_flag[x] & (1L << y))
	    {
	      op_ptr->window_flag[x] &= ~(1L << y);
	    }
	  
	  /* Toggle flag (on) */
	  else
	    {
	      op_ptr->window_flag[x] |= (1L << y);
	    }
	  
	  /* Continue */
	  continue;
	}
      
      /* Extract direction */
      d = target_dir(ke.key);
      
      /* Move */
      if (d != 0)
	{
	  x = (x + ddx[d] + 8) % ANGBAND_TERM_MAX;
	  y = (y + ddy[d] + 16) % PW_MAX_FLAGS;
	}
      
      /* Oops */
      else
	{
	  bell("Illegal command for window options!");
	}
    }
  
  /* Notice changes */
  for (j = 0; j < ANGBAND_TERM_MAX; j++)
    {
      term *old = Term;
      
      /* Dead window */
      if (!angband_term[j]) continue;
      
      /* Ignore non-changes */
      if (op_ptr->window_flag[j] == old_flag[j]) continue;
      
      /* Activate */
      Term_activate(angband_term[j]);
      
      /* Erase */
      Term_clear();
      
      /* Refresh */
      Term_fresh();
      
      /* Restore */
      Term_activate(old);
    }
}


/** Hack -- Base Delay Factor */
void do_cmd_delay(void)
{
  /* Prompt */
  prt("Command: Base Delay Factor", 20, 0);
  add_button("+", '+');      
  add_button("-", '-');      
  update_statusline();      
  
  /* Get a new value */
  while (1)
    {
      event_type ke;
      int msec = op_ptr->delay_factor * op_ptr->delay_factor;
      prt(format("Current base delay factor: %d (%d msec)",
		 op_ptr->delay_factor, msec), 22, 0);
      prt("New base delay factor (0-9 +, - or ESC to accept): ", 21, 0);
      
      ke = inkey_ex();
      if (ke.key == ESCAPE) break;
      if (isdigit(ke.key)) op_ptr->delay_factor = D2I(ke.key);
      if (ke.key == '+') op_ptr->delay_factor++;
      if (ke.key == '-') op_ptr->delay_factor--;
      if (op_ptr->delay_factor > 9) op_ptr->delay_factor = 9;
      if (op_ptr->delay_factor < 0) op_ptr->delay_factor = 0;
    }

  kill_button('+');      
  kill_button('-');
  update_statusline();      
}

/** Hack -- hitpoint warning factor */
void do_cmd_hp_warn(void)
{
  /* Prompt */
  prt("Command: Hitpoint Warning", 20, 0);
  add_button("+", '+');      
  add_button("-", '-');      
  update_statusline();      
  
  /* Get a new value */
  while (1)
    {
      event_type ke;
      prt(format("Current hitpoint warning: %2d%%",
		 op_ptr->hitpoint_warn * 10), 22, 0);
      prt("New hitpoint warning (0-9 +, - or ESC to accept): ", 21, 0);
      
      ke = inkey_ex();
      if (ke.key == ESCAPE) break;
      if (isdigit(ke.key)) op_ptr->hitpoint_warn = D2I(ke.key);
      if (ke.key == '+') op_ptr->hitpoint_warn++;
      if (ke.key == '-') op_ptr->hitpoint_warn--;
      if (op_ptr->hitpoint_warn > 9) op_ptr->hitpoint_warn = 9;
      if (op_ptr->hitpoint_warn < 0) op_ptr->hitpoint_warn = 0;
    }

  kill_button('+');      
  kill_button('-');
  update_statusline();      
}

/** Hack -- hitpoint warning factor */
void do_cmd_panel_change(void)
{
  /* Prompt */
  prt("Command: Panel Change", 20, 0);
  add_button("+", '+');      
  add_button("-", '-');      
  update_statusline();      
  
  /* Get a new value */
  while (1)
    {
      int pdist = (op_ptr->panel_change + 1) * 2;
      event_type ke;
      prt(format("Current panel change: %d (%d / %d)",
                 op_ptr->panel_change, pdist, pdist * 2), 22, 0);
      prt("New panel change (0-4, +, - or ESC to accept): ", 21, 0);

      ke = inkey_ex();
      if (ke.key == ESCAPE) break;
      if (isdigit(ke.key)) op_ptr->panel_change = D2I(ke.key);
      if (ke.key == '+') op_ptr->panel_change++;
      if (ke.key == '-') op_ptr->panel_change--;
      if (op_ptr->panel_change > 4) op_ptr->panel_change = 4;
      if (op_ptr->panel_change < 0) op_ptr->panel_change = 0;
    }

  kill_button('+');      
  kill_button('-');
  update_statusline();      
}

/**
 * Autosave options -- textual names
 */
static cptr autosave_text[1] =
{
  "autosave"
};

/**
 * Autosave options -- descriptions
 */
static cptr autosave_desc[1] =
  {
    "Timed autosave"
  };

s16b toggle_frequency(s16b current)
{
  if (current == 0) return 50;
  if (current == 50) return 100;
  if (current == 100) return 250;
  if (current == 250) return 500;
  if (current == 500) return 1000;
  if (current == 1000) return 2500;
  if (current == 2500) return 5000;
  if (current == 5000) return 10000;
  if (current == 10000) return 25000;
  
  else return 0;
}


/**
 * Interact with autosave options.  From Zangband.
 */
static void do_cmd_options_autosave(void)
{
  event_type ke;
  
  int i, k = 0, n = 1;
  
  char buf[80];
  
  
  /* Clear screen */
  Term_clear();
  
  /* Interact with the player */
  while (TRUE)
    {
      /* Prompt - return taken out as there's only one option... -NRM- */
      sprintf(buf, "Autosave options (y/n to set, 'F' for frequency, ESC to accept) ");
      prt(buf, 0, 0);
      
      /* Display the options */
      for (i = 0; i < n; i++)
        {
          byte a = TERM_WHITE;
          
          /* Color current option */
          if (i == k) a = TERM_L_BLUE;
          
          /* Display the option text */
	  if (small_screen)
	    sprintf(buf, "%-35s: %s",
		    autosave_text[i],
		    autosave ? "yes" : "no ");
	  else
	    sprintf(buf, "%-48s: %s  (%s)",
		    autosave_desc[i],
		    autosave ? "yes" : "no ",
		    autosave_text[i]);
          c_prt(a, buf, i + 2, 0);
          
          prt(format("Timed autosave frequency: every %d turns", 
                     autosave_freq), 5, 0);
        }
      
      
      /* Hilite current option */
      move_cursor(k + 2, 50);

      add_button("F", 'F');
      add_button("n", 'n');
      add_button("y", 'y');
      update_statusline();
      
      /* Get a key */
      ke = inkey_ex();
      
      /* Analyze */
      switch (ke.key)
        {
        case ESCAPE:
          {
	    kill_button('F');
	    kill_button('n');
	    kill_button('y');
	    update_statusline();
            return;
          }
          
        case '-':
        case '8':
          {
            k = (n + k - 1) % n;
            break;
          }
          
        case ' ':
        case '\n':
        case '\r':
        case '2':
          {
            k = (k + 1) % n;
            break;
          }
          
        case 'y':
        case 'Y':
        case '6':
          {
            
            autosave = TRUE;
            k = (k + 1) % n;
            break;
          }
          
        case 'n':
        case 'N':
        case '4':
          {
            autosave = FALSE;
            k = (k + 1) % n;
            break;
          }
          
        case 'f':
        case 'F':
          {
            autosave_freq = toggle_frequency(autosave_freq);
            prt(format("Timed autosave frequency: every %d turns",
                       autosave_freq), 5, 0);
            break;
          }
          
        default:
          {
            bell("Illegal command for Autosave options!");
            break;
          }
        }
    }
}

#ifdef ALLOW_MACROS


/**
 * Hack -- ask for a "trigger" (see below)
 *
 * Note the complex use of the "inkey()" function from "util.c".
 *
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
 */
static void do_cmd_macro_aux(char *buf)
{
  char ch;
  
  int n = 0;
  int curs_x, curs_y;
  
  char tmp[1024] = "";
  
  /* Get cursor position */
  Term_locate(&curs_x, &curs_y);

  /* Flush */
  flush();
  
  
  /* Do not process macros */
  inkey_base = TRUE;
  
  /* First key */
  ch = inkey();
  
  /* Read the pattern */
  while (ch != 0 && ch != '\xff')
    {
      /* Save the key */
      buf[n++] = ch;
      buf[n] = 0;
      
      /* echo */
      ascii_to_text(tmp, sizeof(tmp), buf);
	
	/* Echo it after the prompt */
      Term_erase(curs_x, curs_y, 80);
      Term_gotoxy(curs_x, curs_y);
      Term_addstr(-1, TERM_WHITE, tmp);
		
      /* Do not process macros */
      inkey_base = TRUE;
      
      /* Do not wait for keys */
      inkey_scan = SCAN_INSTANT;
      
      /* Attempt to read a key */
      ch = inkey();
    }
  
  /* Convert the trigger */
  ascii_to_text(tmp, sizeof(tmp), buf);
}


/**
 * Hack -- ask for a keymap "trigger" (see below)
 *
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
 */
static void do_cmd_macro_aux_keymap(char *buf)
{
  char tmp[1024];
  
  
  /* Flush */
  flush();
  
  
  /* Get a key */
  buf[0] = inkey();
  buf[1] = '\0';
  
  
  /* Convert to ascii */
  ascii_to_text(tmp, sizeof(tmp), buf);
  
  /* Hack -- display the trigger */
  Term_addstr(-1, TERM_WHITE, tmp);
  
  
  /* Flush */
  flush();
}


#endif


/**
 * Interact with "macros"
 *
 * Could use some helpful instructions on this page.  XXX XXX XXX
 * CLEANUP
 */

static event_action macro_actions[] =
{
  {LOAD_PREF, "Load a user pref file", 0},
#ifdef ALLOW_MACROS
  {APP_MACRO, "Append macros to a file", 0},
  {ASK_MACRO, "Query a macro", 0},
  {NEW_MACRO, "Create a macro", 0},
  {DEL_MACRO, "Remove a macro", 0},
  {APP_KEYMAP, "Append keymaps to a file", 0},
  {ASK_KEYMAP, "Query a keymap", 0},
  {NEW_KEYMAP, "Create a keymap", 0},
  {DEL_KEYMAP, "Remove a keymap", 0},
  {ENTER_ACT, "Enter a new action", 0}
#endif /* ALLOW_MACROS */
};

static menu_type macro_menu;


void do_cmd_macros(void)
{

  char tmp[1024];
  
  char pat[1024];
  
  int mode;
  int cursor = 0;
  
  region loc = {0, 1, 0, 11};
  
  /* Roguelike */
  if (rogue_like_commands)
    {
      mode = KEYMAP_MODE_ROGUE;
    }
  
  /* Original */
  else
    {
      mode = KEYMAP_MODE_ORIG;
    }
  
  
  screen_save();
  
  menu_layout(&macro_menu, &loc);
  
  /* Process requests until done */
  while (1)
    {
      event_type c;
      int evt;
      
      /* Clear screen */
      clear_from(0);
      
      /* Describe current action */
      prt("Current action (if any) shown below:", 13, 0);
      
      /* Analyze the current action */
      ascii_to_text(tmp, sizeof(tmp), macro_buffer);
      
      /* Display the current action */
      prt(tmp, 14, 0);
      c = menu_select(&macro_menu, &cursor, EVT_CMD);
      
      
      if (ESCAPE == c.key) break;
      if (c.key == ARROW_LEFT || c.key == ARROW_RIGHT) continue;
      evt = macro_actions[cursor].id;
      
      switch(evt)
	{
	case LOAD_PREF:
	  {
	    do_cmd_pref_file_hack(16);
	    break;
	  }
	  
#ifdef ALLOW_MACROS
	case APP_MACRO:
	  {
	    /* Dump the macros */
	    (void)dump_pref_file(macro_dump, "Dump Macros", 15);
	    
	    break;
	  }
	  
	case ASK_MACRO:
		{
		  int k;
		  
		  /* Prompt */
		  prt("Command: Query a macro", 16, 0);
		  
		  /* Prompt */
		  prt("Trigger: ", 18, 0);
		  
		  /* Get a macro trigger */
		  do_cmd_macro_aux(pat);
		  
		  /* Get the action */
		  k = macro_find_exact(pat);
		  
		  /* Nothing found */
		  if (k < 0)
		    {
		      /* Prompt */
		      msg_print("Found no macro.");
		    }

		  /* Found one */
		  else
		    {
		      /* Obtain the action */
		      my_strcpy(macro_buffer, macro__act[k], 
				sizeof(macro_buffer));
		      
		      /* Analyze the current action */
		      ascii_to_text(tmp, sizeof(tmp), macro_buffer);
		      
		      /* Display the current action */
		      prt(tmp, 22, 0);
		      
		      /* Prompt */
		      msg_print("Found a macro.");
		    }
		  break;
		}
		
	case NEW_MACRO:
	  {
	    /* Prompt */
	    prt("Command: Create a macro", 16, 0);
	    
	    /* Prompt */
	    prt("Trigger: ", 18, 0);
	    
	    /* Get a macro trigger */
	    do_cmd_macro_aux(pat);
	    
	    /* Clear */
	    clear_from(20);
	    
	    /* Prompt */
	    prt("Action: ", 20, 0);
	    
	    /* Convert to text */
	    ascii_to_text(tmp, sizeof(tmp), macro_buffer);
	    
	    /* Get an encoded action */
	    if (askfor_aux(tmp, sizeof tmp, NULL))
	      {
		/* Convert to ascii */
		text_to_ascii(macro_buffer, sizeof(macro_buffer), tmp);
		
		/* Link the macro */
		macro_add(pat, macro_buffer);
		
		/* Prompt */
		msg_print("Added a macro.");
	      }
	    break;
	  }
	  
	case DEL_MACRO:
	  {
	    /* Prompt */
	    prt("Command: Remove a macro", 16, 0);
	    
	    /* Prompt */
	    prt("Trigger: ", 18, 0);
	    
	    /* Get a macro trigger */
	    do_cmd_macro_aux(pat);
	    
	    /* Link the macro */
	    macro_add(pat, pat);
	    
	    /* Prompt */
	    msg_print("Removed a macro.");
	    break;
	  }
	case APP_KEYMAP:
	  {
	    /* Dump the keymaps */
	    (void)dump_pref_file(keymap_dump, "Dump Keymaps", 15);
	    break;
	  }
	case ASK_KEYMAP:
	  {
	    cptr act;
	    
	    /* Prompt */
	    prt("Command: Query a keymap", 16, 0);
	    
	    /* Prompt */
	    prt("Keypress: ", 18, 0);
	    
	    /* Get a keymap trigger */
	    do_cmd_macro_aux_keymap(pat);
	    
	    /* Look up the keymap */
	    act = keymap_act[mode][(byte)(pat[0])];
	    
	    /* Nothing found */
	    if (!act)
	      {
		/* Prompt */
		msg_print("Found no keymap.");
	      }
	    
	    /* Found one */
	    else
	      {
		/* Obtain the action */
		my_strcpy(macro_buffer, act, sizeof(macro_buffer));
		
		/* Analyze the current action */
		ascii_to_text(tmp, sizeof(tmp), macro_buffer);
		
		/* Display the current action */
		prt(tmp, 22, 0);
		
		/* Prompt */
		msg_print("Found a keymap.");
	      }
	    break;
	  }
	case NEW_KEYMAP:
	  {
	    /* Prompt */
	    prt("Command: Create a keymap", 16, 0);
	    
	    /* Prompt */
	    prt("Keypress: ", 18, 0);
	    
	    /* Get a keymap trigger */
	    do_cmd_macro_aux_keymap(pat);
	    
	    /* Clear */
	    clear_from(20);
	    
	    /* Prompt */
	    prt("Action: ", 20, 0);
	    
	    /* Convert to text */
	    ascii_to_text(tmp, sizeof(tmp), macro_buffer);
	    
	    /* Get an encoded action */
	    if (askfor_aux(tmp, sizeof tmp, NULL))
	      {
		/* Convert to ascii */
		text_to_ascii(macro_buffer, sizeof(macro_buffer), tmp);
		
		/* Free old keymap */
		string_free(keymap_act[mode][(byte)(pat[0])]);
		
		/* Make new keymap */
		keymap_act[mode][(byte)(pat[0])] = string_make(macro_buffer);
		
		/* Prompt */
		msg_print("Added a keymap.");
	      }
	    break;
	  }
	case DEL_KEYMAP:
	  {
	    /* Prompt */
	    prt("Command: Remove a keymap", 16, 0);
	    
	    /* Prompt */
	    prt("Keypress: ", 18, 0);

	    /* Get a keymap trigger */
	    do_cmd_macro_aux_keymap(pat);
	    
	    /* Free old keymap */
	    string_free(keymap_act[mode][(byte)(pat[0])]);
	    
	    /* Make new keymap */
	    keymap_act[mode][(byte)(pat[0])] = NULL;
	    
	    /* Prompt */
	    msg_print("Removed a keymap.");
	    break;
	  }
	case ENTER_ACT: /* Enter a new action */
	  {
	    /* Prompt */
	    prt("Command: Enter a new action", 16, 0);
	    
	    /* Go to the correct location */
	    Term_gotoxy(0, 22);
	    
	    /* Analyze the current action */
	    ascii_to_text(tmp, sizeof(tmp), macro_buffer);
	    
	    /* Get an encoded action */
	    if (askfor_aux(tmp, sizeof tmp, NULL))
	      {
		/* Extract an action */
		text_to_ascii(macro_buffer, sizeof(macro_buffer), tmp);
	      }
	    break;
	  }
#endif /* ALLOW_MACROS */
	}
      
      /* Flush messages */
      message_flush();
    }
  
  /* Load screen */
  screen_load();
}

/* These two are used to place elements in the grid */
#define COLOR_X(idx) (((idx) / MAX_BASE_COLORS) * (small_screen ? 3 : 5) + 1)
#define COLOR_Y(idx) ((idx) % MAX_BASE_COLORS + 6)

/* We only can edit a portion of the color table */
#define MAX_EDIT_COLORS 128

/* Hack - Note the cast to "int" to prevent overflow */
#define IS_BLACK(idx) \
((int)angband_color_table[idx][1] + (int)angband_color_table[idx][2] + \
 (int)angband_color_table[idx][3] == 0)

/* We show black as dots to see the shape of the grid */
#define BLACK_SAMPLE (small_screen ? ".." : "...")

/* String used to show a color sample */
#define COLOR_SAMPLE (small_screen ? "##" : "###")

/**
 * Asks the player for an extended color. It is done in two steps:
 * 1. Asks for the base color.
 * 2. Asks for a specific shade.
 * It erases the given line.
 * If the user press ESCAPE no changes are made to attr.
 */
static void askfor_shade(byte *attr, int y)
{
  byte base, shade, temp;
  bool changed = FALSE;
  char *msg, *pos;
  event_type ke;
  
  /* Start with the given base color */
  base = GET_BASE_COLOR(*attr);
  
  /* 1. Query for base color */
  while (1)
    {
      /* Clear the line */
      Term_erase(0, y, 255);
      Term_erase(0, y + 1, 255);
      
      /* Prompt */
      c_put_str(TERM_WHITE, "1. Choose base color (use arrows or ++/--/done):"
                , y, 0);
      
      /* Format the query */
      msg = format("%s %s (attr = %d) ", COLOR_SAMPLE, 
                   color_names[base], base);
      
      /* Display it */
      c_put_str(TERM_WHITE, msg, y + 1, 3);
      
      /* Find the sample */
      pos = strstr(msg, COLOR_SAMPLE);
      
      /* Show it using the proper color */
      c_put_str(base, COLOR_SAMPLE, y + 1, pos - msg + 3);
      
      /* Place the cursor at the end of the message */
      Term_gotoxy(strlen(msg) + 3, y + 1);
      
      /* Get a command */
      ke = inkey_ex();

      /* Mouse Input */
      if (ke.key == '\xff')
        {
          if (ke.mousey == 3) ke.key = ESCAPE;
          else if (ke.mousey == y)
            {
              if ((ke.mousex > 35) && (ke.mousex < 38)) ke.key = '6';
              if ((ke.mousex > 38) && (ke.mousex < 41)) ke.key = '4';
              if ((ke.mousex > 41) && (ke.mousex < 46)) ke.key = '\r';
            }
        }
      
      /* Cancel */
      if (ke.key == ESCAPE)
        {
          /* Clear the line */
          Term_erase(0, y, 255);
          Term_erase(0, y + 1, 255);
          return;
        }
      
      /* Accept the current base color */
      if ((ke.key == '\r') || (ke.key == '\n')) break;
      
      /* Move to the previous color if possible */
      if (((ke.key == '4') || (ke.key == '-')) && (base > 0))
        {
          --base;
          /* Reset the shade, see below */
          changed = TRUE;
          continue;
        }
      
      /* Move to the next color if possible */
      if (((ke.key == '6') || (ke.key == '+')) && (base < MAX_BASE_COLORS - 1))
        {
          ++base;
          /* Reset the shade, see below */
          changed = TRUE;
          continue;
        }
    }
  
  /* The player selected a different base color, start from shade 0 */
  if (changed)  shade = 0;
  /* We assume that the player is editing the current shade, go there */
  else          shade = GET_SHADE(*attr);
  
  /* 2. Query for specific shade */
  while (1)
    {
      /* Clear the line */
      Term_erase(0, y, 255);
      Term_erase(0, y + 1, 255);
      
      /* Create the real color */
      temp = MAKE_EXTENDED_COLOR(base, shade);
      
      /* Prompt */
      c_put_str(TERM_WHITE, "2. Choose shade (use arrows or ++/--/done):"
                , y, 0);
      
      /* Format the message */
      msg = format("%s %s (attr = %d) ", COLOR_SAMPLE, 
                   get_ext_color_name(temp), temp);
      
      /* Display it */
      c_put_str(TERM_WHITE, msg, y + 1, 3);
      
      /* Find the sample */
      pos = strstr(msg, COLOR_SAMPLE);
      
      /* Show it using the proper color */
      c_put_str(temp, COLOR_SAMPLE, y + 1, pos - msg + 3);
      
      /* Place the cursor at the end of the message */
      Term_gotoxy(strlen(msg) + 3, y + 1);
      
      /* Get a command */
      ke = inkey_ex();
      
      /* Mouse Input */
      if (ke.key == '\xff')
        {
          if (ke.mousey == 3) ke.key = ESCAPE;
          else if (ke.mousey == y)
            {
              if ((ke.mousex > 30) && (ke.mousex < 33)) ke.key = '6';
              if ((ke.mousex > 33) && (ke.mousex < 36)) ke.key = '4';
              if ((ke.mousex > 36) && (ke.mousex < 41)) ke.key = '\r';
            }
        }
      
      /* Cancel */
      if (ke.key == ESCAPE)
        {
          /* Clear the line */
          Term_erase(0, y, 255);
          Term_erase(0, y + 1, 255);
          return;
        }
      
      /* Accept the current shade */
      if ((ke.key == '\r') || (ke.key == '\n')) break;
      
      /* Move to the previous shade if possible */
      if (((ke.key == '4') || (ke.key == '-')) && (shade > 0))
        {
          --shade;
          continue;
        }
      
      /* Move to the next shade if possible */
      if (((ke.key == '6') || (ke.key == '+')) && (shade < MAX_SHADES - 1))
        {
          ++shade;
          continue;
        }
    }
  
  /* Assign the selected shade */
  *attr = temp;
  
  /* Clear the line. It is needed to fit in the current UI */
  Term_erase(0, y, 255);
  Term_erase(0, y + 1, 255);
}

int modify_attribute(const char *clazz, int oid, const char *name,
		     byte da, char dc, byte *pca, char *pcc)
{
  event_type ke = EVENT_EMPTY;
  const char *empty_symbol = "<< ? >>";
  const char *empty_symbol2 = "\0";
  const char *empty_symbol3 = "\0";
  
  byte ca = (byte)*pca;
  byte cc = (byte)*pcc;
  
  int linec = (use_trptile ? 22 : (use_dbltile ? 21 : 20));
  
  if (use_trptile && use_bigtile)
    {
      empty_symbol = "// ?????? \\\\";
      empty_symbol2 = "   ??????   ";
      empty_symbol3 = "\\\\ ?????? //";
    }
  else if (use_dbltile && use_bigtile)
    {
      empty_symbol = "// ???? \\\\";
      empty_symbol2 = "\\\\ ???? //";
    }
  else if (use_trptile)
    {
      empty_symbol = "// ??? \\\\";
      empty_symbol2 = "   ???   ";
      empty_symbol3 = "\\\\ ??? //";
    }
  else if (use_dbltile)
    {
      empty_symbol = "// ?? \\\\";
                empty_symbol2 = "\\\\ ?? //";
    }
  else if (use_bigtile) empty_symbol = "<< ?? >>";
  
  /* Prompt */
  prt(format("Command: Change %s attr/chars", clazz), 15, 0);
  
  /* Label the object */
  Term_putstr(5, 17, -1, TERM_WHITE, format("%s = %d, Name = %-40.40s", clazz, 
					    oid, name));
  
  /* Label the Default values */
  Term_putstr(10, 19, -1, TERM_WHITE,
              format("Default attr/char = %3u / %3u", da, dc));
  Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
  
  if (use_dbltile || use_trptile) 
    Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol2);
  if (use_trptile) Term_putstr (40, 20, -1, TERM_WHITE, empty_symbol3);
  
  
  big_pad(43, 19, da, dc);
  
  /* Label the Current values */
  Term_putstr(10, linec, -1, TERM_WHITE,
              format("Current attr/char = %3u / %3u", ca, cc));
  Term_putstr(40, linec, -1, TERM_WHITE, empty_symbol);
  if (use_dbltile || use_trptile) 
    Term_putstr (40, linec+1, -1, TERM_WHITE, empty_symbol2); 
  if (use_trptile) Term_putstr (40, linec+2, -1, TERM_WHITE, empty_symbol3); 
  
  big_pad(43, linec, ca, cc);
  
  if (use_trptile) linec++;
  
  /* Prompt */
  Term_putstr(0, linec + 2, -1, TERM_WHITE, "Command (n/N/a/A/c/C): ");

  /* Buttons */
  add_button("s", 's');
  add_button("C", 'C');
  add_button("c", 'c');
  add_button("A", 'A');
  add_button("a", 'a');
  add_button("N", 'N');
  add_button("n", 'n');
  update_statusline();
  
  /* Get a command */
  ke = inkey_ex();
  
  /* Analyze */
  if (ke.key == 'a') *pca = (byte)(ca + 1);
  if (ke.key == 'A') *pca = (byte)(ca - 1);
  if (ke.key == 'c') *pcc = (byte)(cc + 1);
  if (ke.key == 'C') *pcc = (byte)(cc - 1);
  if (ke.key == 's') askfor_shade(pca, 22);
      
  /* Buttons */
  kill_button('s');
  kill_button('C');
  kill_button('c');
  kill_button('A');
  kill_button('a');
  kill_button('N');
  kill_button('n');
  update_statusline();
  
  return ke.key;
}

event_action visual_menu_items [] =
  {
    {LOAD_PREF, "Load a user pref file", 0, 0},
    { DUMP_MON,  "Dump monster attr/chars", 0, 0},
    { DUMP_OBJ,  "Dump object attr/chars", 0, 0 },
    { DUMP_FEAT, "Dump feature attr/chars", 0, 0 },
    { DUMP_FLAV, "Dump flavor attr/chars", 0, 0 },
    { MOD_MON,   "Change monster attr/chars", 0, 0 },
    { MOD_OBJ,   "Change object attr/chars", 0, 0 },
    { MOD_FEAT,  "Change feature attr/chars", 0, 0 },
    { MOD_FLAV,  "Change flavor attr/chars", 0, 0 },
    { RESET_VIS, "Reset visuals", 0, 0 },
  };

static menu_type visual_menu;


/**
 * Interact with "visuals"
 */
void do_cmd_visuals(void)
{
  int cursor = 0;
  
  /* Save screen */
  screen_save();
  
  menu_layout(&visual_menu, &SCREEN_REGION);
  
  /* Interact until done */
  while (1)
    {
      event_type key;
      int evt = -1;
      clear_from(0);
      key = menu_select(&visual_menu, &cursor, EVT_CMD);
      if (key.key == ESCAPE) 
	break;
      
      if (key.key == ARROW_LEFT || key.key == ARROW_RIGHT)
        continue;
      
      assert(cursor >= 0 && cursor < visual_menu.count);
      
      evt = visual_menu_items[cursor].id;
      
      if (evt == LOAD_PREF)
	{
	  /* Ask for and load a user pref file */
	  do_cmd_pref_file_hack(15);
	}
      
#ifdef ALLOW_VISUALS
      
      else if (evt == DUMP_MON)
	{
	  dump_pref_file(dump_monsters, "Dump Monster attr/chars", 15);
	}
      
      else if (evt == DUMP_OBJ)
	{
	  dump_pref_file(dump_objects, "Dump Object attr/chars", 15);
	}
      
      else if (evt == DUMP_FEAT)
	{
	  dump_pref_file(dump_features, "Dump Feature attr/chars", 15);
	}
      
      /* Dump flavor attr/chars */
      else if (evt == DUMP_FLAV) 
	{
	  dump_pref_file(dump_flavors, "Dump Flavor attr/chars", 15);
	}
      
      /* Modify monster attr/chars */
      else if (evt == MOD_MON)
	{
	  static int r = 0;
	  
	  /* Prompt */
	  prt("Command: Change monster attr/chars", 15, 0);
	  
	  /* Hack -- query until done */
	  while (1)
	    {
	      int cx;
	      monster_race *r_ptr = &r_info[r];
	      
	      cx = modify_attribute("Monster", r, r_name + r_ptr->name,
				    r_ptr->d_attr, r_ptr->d_char,
				    &r_ptr->x_attr, &r_ptr->x_char);
	      
	      if (cx == ESCAPE) break;
	      /* Analyze */
	      if (cx == 'n') r = (r + z_info->r_max + 1) % z_info->r_max;
	      if (cx == 'N') r = (r + z_info->r_max - 1) % z_info->r_max;
	    }
	}
      
      /* Modify object attr/chars */
      else if (evt == MOD_OBJ)
	{
	  static int k = 0;
	  
	  /* Hack -- query until done */
	  while (1)
	    {
	      int cx;
	      object_kind *k_ptr = &k_info[k];
	      cx = modify_attribute("Object", k, k_name + k_ptr->name,
				    k_ptr->d_attr, k_ptr->d_char,
				    &k_ptr->x_attr, &k_ptr->x_char);
	      
	      if (cx == ESCAPE) break;
	      if (cx == 'n') k = (k + z_info->k_max + 1) % z_info->k_max;
	      if (cx == 'N') k = (k + z_info->k_max - 1) % z_info->k_max;
	    }
	}
      
      /* Modify feature attr/chars */
      else if (evt == MOD_FEAT)
	{
	  static int f = 0;
	  
	  /* Hack -- query until done */
	  while (1)
	    {
	      feature_type *f_ptr = &f_info[f];
	      int cx = modify_attribute("Feature", f, f_name + f_ptr->name,
					f_ptr->d_attr, f_ptr->d_char,
					&f_ptr->x_attr, &f_ptr->x_char);
	      
	      if (cx == ESCAPE) break;
	      if (cx == 'n') f = (f + z_info->f_max + 1) % z_info->f_max;
	      if (cx == 'N') f = (f + z_info->f_max - 1) % z_info->f_max;
	    }
	}
      /* Modify flavor attr/chars */
      else if (evt == MOD_FLAV)
	{
	  static int f = 0;
	  
	  /* Hack -- query until done */
	  while (1)
	    {
	      flavor_type *x_ptr = &flavor_info[f];
	      int cx = modify_attribute("Flavor", f, flavor_text + x_ptr->text,
					x_ptr->d_attr, x_ptr->d_char,
                                        &x_ptr->x_attr, &x_ptr->x_char);
              
              if (cx == ESCAPE) break;
              if (cx == 'n') 
                f = (f + z_info->flavor_max + 1) % z_info->flavor_max;
              if (cx == 'N') 
                f = (f + z_info->flavor_max - 1) % z_info->flavor_max;
            }
        }
      
#endif /* ALLOW_VISUALS */
      
      /* Reset visuals */
      else if (evt == RESET_VIS)
	{
	  /* Reset */
	  reset_visuals(TRUE);
	  
	  /* Message */
	  msg_print("Visual attr/char tables reset.");
	}
    }
  
  /* Load screen */
  screen_load();
}

/**
 * Asks to the user for specific color values.
 * Returns TRUE if the color was modified.
 */
static bool askfor_color_values(int idx)
{
  char str[10];
  
  int k, r, g, b;
  
  /* Get the default value */
  sprintf(str, "%d", angband_color_table[idx][1]);
  
  /* Query, check for ESCAPE */
  if (!get_string("Red (0-255) ", str, sizeof(str))) return FALSE;
  
  /* Convert to number */
  r = atoi(str);
  
  /* Check bounds */
  if (r < 0) r = 0;
  if (r > 255) r = 255;
  
  /* Get the default value */
  sprintf(str, "%d", angband_color_table[idx][2]);
  
  /* Query, check for ESCAPE */
  if (!get_string("Green (0-255) ", str, sizeof(str))) return FALSE;
  
  /* Convert to number */
  g = atoi(str);
  
  /* Check bounds */
  if (g < 0) g = 0;
  if (g > 255) g = 255;
  
  /* Get the default value */
  sprintf(str, "%d", angband_color_table[idx][3]);
  
  /* Query, check for ESCAPE */
  if (!get_string("Blue (0-255) ", str, sizeof(str))) return FALSE;
  
  /* Convert to number */
  b = atoi(str);
  
  /* Check bounds */
  if (b < 0) b = 0;
  if (b > 255) b = 255;
  
  /* Get the default value */
  sprintf(str, "%d", angband_color_table[idx][0]);
  
  /* Query, check for ESCAPE */
  if (!get_string("Extra (0-255) ", str, sizeof(str))) return FALSE;
  
  /* Convert to number */
  k = atoi(str);
  
  /* Check bounds */
  if (k < 0) k = 0;
  if (k > 255) k = 255;
  
  /* Do nothing if the color is not modified */
  if ((k == angband_color_table[idx][0]) &&
      (r == angband_color_table[idx][1]) &&
      (g == angband_color_table[idx][2]) &&
      (b == angband_color_table[idx][3])) return FALSE;
  
  /* Modify the color table */
  angband_color_table[idx][0] = k;
  angband_color_table[idx][1] = r;
  angband_color_table[idx][2] = g;
  angband_color_table[idx][3] = b;
  
  /* Notify the changes */
  return TRUE;
}


/**
 * The screen used to modify the color table. Only 128 colors can be modified.
 * The remaining entries of the color table are reserved for graphic mode.
 */
static void modify_colors(void)
{
  int x, y, idx, old_idx;
  event_type ke;
  char msg[100];
  
  /* Flags */
  bool do_move, do_update;
  
  /* Clear the screen */
  Term_clear();
  
  /* Draw the color table */
  for (idx = 0; idx < MAX_EDIT_COLORS; idx++)
    {
      /* Get coordinates, the x value is adjusted to show a fake cursor */
      x = COLOR_X(idx) + 1;
      y = COLOR_Y(idx);
      
      /* Show a sample of the color */
      if (IS_BLACK(idx)) c_put_str(TERM_WHITE, BLACK_SAMPLE, y, x);
      else c_put_str(idx, COLOR_SAMPLE, y, x);
    }
  
  /* Show screen commands and help */
  if (small_screen)
    {
      y = 2;
      x = 26;
      c_put_str(TERM_WHITE, "Commands:", y, x);
      c_put_str(TERM_WHITE, "ESC: Return", y + 1, x);
      c_put_str(TERM_WHITE, "Arrows: Move to color", y + 3, x);
      c_put_str(TERM_WHITE, "k,K: Incr,Decr extra", y + 4, x);
      c_put_str(TERM_WHITE, "r,R: Incr,Decr red", y + 5, x);
      c_put_str(TERM_WHITE, "g,G: Incr,Decr green", y + 6, x);
      c_put_str(TERM_WHITE, "b,B: Incr,Decr blue", y + 7, x);
      c_put_str(TERM_WHITE, "c: Copy from color", y + 8, x);
      c_put_str(TERM_WHITE, "v: Set specific vals", y + 9, x);
      c_put_str(TERM_WHITE, "Column 1: base colors", y + 11, x);
      c_put_str(TERM_WHITE, "Column 2: first shade", y + 12, x);  
      c_put_str(TERM_WHITE, "etc.", y + 13, x);  
      c_put_str(TERM_WHITE, "Shades look like base colors in 16 color ports.",
                23, 0);
    }
  else
    {
      y = 2;
      x = 42;
      c_put_str(TERM_WHITE, "Commands:", y, x);
      c_put_str(TERM_WHITE, "ESC: Return", y + 2, x);
      c_put_str(TERM_WHITE, "Arrows: Move to color", y + 3, x);
      c_put_str(TERM_WHITE, "k,K: Incr,Decr extra value", y + 4, x);
      c_put_str(TERM_WHITE, "r,R: Incr,Decr red value", y + 5, x);
      c_put_str(TERM_WHITE, "g,G: Incr,Decr green value", y + 6, x);
      c_put_str(TERM_WHITE, "b,B: Incr,Decr blue value", y + 7, x);
      c_put_str(TERM_WHITE, "c: Copy from color", y + 8, x);
      c_put_str(TERM_WHITE, "v: Set specific values", y + 9, x);
      c_put_str(TERM_WHITE, "First column: base colors", y + 11, x);
      c_put_str(TERM_WHITE, "Second column: first shade, etc.", y + 12, x);  
      c_put_str(TERM_WHITE, "Shades look like base colors in 16 color ports.",
                23, 0);
    }
  
  /* Hack - We want to show the fake cursor */
  do_move = TRUE;
  do_update = TRUE;
  
  /* Start with the first color */
  idx = 0;
  
  /* Used to erase the old position of the fake cursor */
  old_idx = -1;

  /* Buttons */
  add_button("v", 'v');
  add_button("c", 'c');
  add_button("B", 'B');
  add_button("b", 'b');
  add_button("G", 'G');
  add_button("g", 'g');
  add_button("R", 'R');
  add_button("r", 'r');
  add_button("K", 'K');
  add_button("k", 'k');
  update_statusline();
  
  while (1)
    {
      /* Movement request */
      if (do_move)
        {
          
          /* Erase the old fake cursor */
          if (old_idx >= 0)
            {
              /* Get coordinates */
              x = COLOR_X(old_idx);
              y = COLOR_Y(old_idx);
              
              /* Draw spaces */
              c_put_str(TERM_WHITE, " ", y, x);
              c_put_str(TERM_WHITE, " ", y, x + (small_screen ? 3 : 4));
            }
          
          /* Show the current fake cursor */
          /* Get coordinates */
          x = COLOR_X(idx);
          y = COLOR_Y(idx);
          
          /* Draw the cursor */
          c_put_str(TERM_WHITE, ">", y, x);
          c_put_str(TERM_WHITE, "<", y, x + (small_screen ? 3 : 4));
          
          /* Format the name of the color */
          my_strcpy(msg, format("Color = %d (0x%02X), Name = %s", idx, idx,
                                get_ext_color_name(idx)), sizeof(msg));
          
          /* Show the name and some whitespace */
          c_put_str(TERM_WHITE, format("%-40s", msg), 2, 0);
        }
      
      /* Color update request */
      if (do_update)
        {
          /* Get coordinates, adjust x */
          x = COLOR_X(idx) + 1;
          y = COLOR_Y(idx);
          
          /* Hack - Redraw the sample if needed */
          if (IS_BLACK(idx)) c_put_str(TERM_WHITE, BLACK_SAMPLE, y, x);
          else c_put_str(idx, COLOR_SAMPLE, y, x);
          
          /* Notify the changes in the color table to the terminal */
          Term_xtra(TERM_XTRA_REACT, 0);
          
          /* The user is playing with white, redraw all */
          if (idx == TERM_WHITE) Term_redraw();
          
          /* Or reduce flickering by redrawing the changes only */
          else Term_redraw_section(x, y, x + 2, y);
        }

      /* Common code, show the values in the color table */
      if (do_move || do_update)
        {
          /* Format the view of the color values */
          my_strcpy(msg, format("K = %d / R,G,B = %d, %d, %d",
                                angband_color_table[idx][0],
                                angband_color_table[idx][1],
                                angband_color_table[idx][2],
                                angband_color_table[idx][3]), sizeof(msg));
          
          /* Show color values and some whitespace */
          c_put_str(TERM_WHITE, format("%-40s", msg), 4, 0);
          
        }
      
      /* Reset flags */
      do_move = FALSE;
      do_update = FALSE;
      old_idx = -1;
      
      /* Get a command */
      if (!get_com_ex("Command: Modify colors ", &ke)) break;

      /* Mouse input */
      if (ke.key == '\xff')
        {
          /* Set divider */
          x = (small_screen ? 26 : 42);
          if (ke.mousex < x)
            {
              /* Remember cursor */
              old_idx = idx;

              /* Look for new location */
              for (idx = 0; idx < MAX_EDIT_COLORS; idx++)
                {
                  if ((ke.mousey == COLOR_Y(idx)) && (ke.mousex > COLOR_X(idx))
                      && (ke.mousex < COLOR_X(idx) + 4)) 
                    {
                      /* Found it */
                      do_move = TRUE;
                      break;
                    }
                }
              /* Didn't find it */
              if (idx == MAX_EDIT_COLORS)
                {
                  idx = old_idx;
                  old_idx = -1;
                }
            }
        }
      
      switch(ke.key)
        {
          /* Down */
        case '2':
          {
            /* Check bounds */
            if (idx + 1 >= MAX_EDIT_COLORS) break;
            
            /* Erase the old cursor */
            old_idx = idx;
            
            /* Get the new position */
            ++idx;
            
            /* Request movement */
            do_move = TRUE;
            break;
          }
          
          /* Up */
        case '8':
          {
            
            /* Check bounds */
            if (idx - 1 < 0) break;
            
            /* Erase the old cursor */
            old_idx = idx;
            
            /* Get the new position */
            --idx;
            
            /* Request movement */
            do_move = TRUE;
            break;
          }

          /* Left */
        case '4':
          {
            /* Check bounds */
            if (idx - 16 < 0) break;
            
            /* Erase the old cursor */
            old_idx = idx;
            
            /* Get the new position */
            idx -= 16;
            
            /* Request movement */
            do_move = TRUE;
            break;
          }
          
          /* Right */
        case '6':
          {
            /* Check bounds */
            if (idx + 16 >= MAX_EDIT_COLORS) break;
            
            /* Erase the old cursor */
            old_idx = idx;
            
            /* Get the new position */
            idx += 16;
            
            /* Request movement */
            do_move = TRUE;
            break;
          }
          
          /* Copy from color */
        case 'c':
          {
            char str[10];
            int src;
            
            /* Get the default value, the base color */
            sprintf(str, "%d", GET_BASE_COLOR(idx));
            
            /* Query, check for ESCAPE */
            if (!get_string(format("Copy from color (0-%d, def. base) ",
                                   MAX_EDIT_COLORS - 1), 
                            str, sizeof(str))) break;
            
            /* Convert to number */
            src = atoi(str);
            
            /* Check bounds */
            if (src < 0) src = 0;
            if (src >= MAX_EDIT_COLORS) src = MAX_EDIT_COLORS - 1;
            
            /* Do nothing if the colors are the same */
            if (src == idx) break;
            
            /* Modify the color table */
            angband_color_table[idx][0] = angband_color_table[src][0];
            angband_color_table[idx][1] = angband_color_table[src][1];
            angband_color_table[idx][2] = angband_color_table[src][2];
            angband_color_table[idx][3] = angband_color_table[src][3];
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Increase the extra value */
        case 'k':
          {
            /* Get a pointer to the proper value */
            byte *k_ptr = &angband_color_table[idx][0];
            
            /* Modify the value */
            *k_ptr = (byte)(*k_ptr + 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Decrease the extra value */
        case 'K':
          {
            
            /* Get a pointer to the proper value */
            byte *k_ptr = &angband_color_table[idx][0];
            
            /* Modify the value */
            *k_ptr = (byte)(*k_ptr - 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Increase the red value */
        case 'r':
          {
            /* Get a pointer to the proper value */
            byte *r_ptr = &angband_color_table[idx][1];
            
            /* Modify the value */
            *r_ptr = (byte)(*r_ptr + 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Decrease the red value */
        case 'R':
          {
            
            /* Get a pointer to the proper value */
            byte *r_ptr = &angband_color_table[idx][1];
            
            /* Modify the value */
            *r_ptr = (byte)(*r_ptr - 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Increase the green value */
        case 'g':
          {
            /* Get a pointer to the proper value */
            byte *g_ptr = &angband_color_table[idx][2];
            
            /* Modify the value */
            *g_ptr = (byte)(*g_ptr + 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Decrease the green value */
        case 'G':
          {
            /* Get a pointer to the proper value */
            byte *g_ptr = &angband_color_table[idx][2];
            
            /* Modify the value */
            *g_ptr = (byte)(*g_ptr - 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Increase the blue value */
        case 'b':
          {
            /* Get a pointer to the proper value */
            byte *b_ptr = &angband_color_table[idx][3];
            
            /* Modify the value */
            *b_ptr = (byte)(*b_ptr + 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Decrease the blue value */
        case 'B':
          {
            /* Get a pointer to the proper value */
            byte *b_ptr = &angband_color_table[idx][3];
            
            /* Modify the value */
            *b_ptr = (byte)(*b_ptr - 1);
            
            /* Request update */
            do_update = TRUE;
            break;
          }
          
          /* Ask for specific values */
        case 'v':
          {
            do_update = askfor_color_values(idx);
            break;
          }
        }
    }

  /* Buttons */
  kill_button('v');
  kill_button('c');
  kill_button('B');
  kill_button('b');
  kill_button('G');
  kill_button('g');
  kill_button('R');
  kill_button('r');
  kill_button('K');
  kill_button('k');
  update_statusline();
}


static event_action color_events [] =
  {
    {LOAD_PREF, "Load a user pref file", 0, 0},
#ifdef ALLOW_COLORS
    {DUMP_COL, "Dump colors", 0, 0},
    {MOD_COL, "Modify colors", 0, 0}
#endif
  };

static menu_type color_menu;


/**
 * Interact with "colors"
 */
void do_cmd_colors(void)
{
  int cursor = 0;
  
  /* Save screen */
  screen_save();
  
  menu_layout(&color_menu, &SCREEN_REGION);
  
  /* Interact until done */
  while (1)
    {
      event_type key;
      int evt;
      clear_from(0);
      key = menu_select(&color_menu, &cursor, EVT_CMD);
      
      /* Done */
      if (key.key == ESCAPE) break;
      evt = color_events[cursor].id;
      
      /* Load a user pref file */
      if (evt == LOAD_PREF)
	{
	  /* Ask for and load a user pref file */
	  do_cmd_pref_file_hack(8);
	  
	  /* Could skip the following if loading cancelled XXX XXX XXX */
	  
	  /* Mega-Hack -- React to color changes */
	  Term_xtra(TERM_XTRA_REACT, 0);
	  
	  /* Mega-Hack -- Redraw physical windows */
	  Term_redraw();
	}
      
#ifdef ALLOW_COLORS
      
      /* Dump colors */
      else if (evt == DUMP_COL)
	{
	  dump_pref_file(dump_colors, "Dump Colors", 15);
	}
      
      /* Edit colors */
      else if (evt == MOD_COL)
        {
          modify_colors();
        }
      
#endif /* ALLOW_COLORS */
      
      /* Clear screen */
      clear_from(0);
    }
  
  
  /* Load screen */
  screen_load();
}




/**
 * Note something in the message recall or character notes file.  Lifted
 * from NPPangband, patch originally by Chris Kern.
 */
void do_cmd_note(void)
{
  make_note("", p_ptr->stage, NOTE_PLAYER, p_ptr->lev);
}
  
  
/**
 * Note something in the message recall or character notes file.  Lifted
 * from NPPangband, patch originally by Chris Kern.
 *
 * Updated for FA 030 -NRM-
 */
void make_note(char *note, int what_stage, byte note_type, s16b lev)
{
  char buf[80];
  
  int i, num = 0;
  
  /* Default */
  strcpy(buf, "");
  
  /* If a note is passed, use that, otherwise accept user input. */
  if (streq(note, ""))
    {
      if (!get_string("Note: ", buf, 70)) return;
    }
  
  else my_strcpy(buf, note, sizeof(buf));
  
  /* Ignore empty notes */
  if (!buf[0] || (buf[0] == ' ')) return;
  
  /* Find out how many notes already */
  while (notes[num].turn != 0) num++;

  /* Handle too many notes (unlikely) */
  if (num > NOTES_MAX_LINES - 5)
    {
      /* Move everything 10 back */
      for (i = 10; i < num; i++)
	notes[num - 10] = notes[num];

      /* Delete the last 10 */
      for (i = num; i < num + 10; i++)
	notes[num - 10].turn = 0; 

      /* Paranoia - recalculate */
      num = 0;
      while (notes[num].turn != 0) num++;
    }

  /* Search backward */
  if (num > 0)
    while (turn < notes[num - 1].turn)
      {
	/* Move the last entry down */
	notes[num] = notes[num - 1];
	num--;
      }

  /* Should be in the right place */
  notes[num].turn = turn;
  notes[num].place = what_stage;
  notes[num].level = (int)lev;
  notes[num].type = note_type;
  my_strcpy(notes[num].note, buf, sizeof(notes[num].note));
      
}


/**
 * Mention the current version
 */
void do_cmd_version(void)
{
  /* Silly message */
  msg_format("You are playing %s %s.  Type '?' for more info.",
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
      msg_print("Looks like a typical town.");
      return;
    }
  
  /* No useful feelings until enough time has passed */
  if (!do_feeling)
    {
      msg_print("You are still uncertain about this level...");
      return;
    }
  
  /* Display the feeling */
    if (p_ptr->themed_level) msg_format("%s", themed_feeling);
    else msg_print(feeling_text[feeling]);
}

/*
 * Array of feeling strings
 */
static cptr do_cmd_challenge_text[14] =
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
  
  msg_format("%^s, the %^s %s", ghost_name, r_name + r_ptr->name, 
             do_cmd_challenge_text[rand_int(14)]);
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
  
  byte a = 0;
  char c = ' ';
  
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
  msg_print("Screen dump loaded.");
  message_flush();
  
  
  /* Load screen */
  screen_load();
}

void do_cmd_save_screen_text(void)
{
  int y, x;
  
  byte a = 0;
  char c = ' ';
  
  ang_file *fff;
  
  char buf[1024];
  
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
      /* Dump each row */
      for (x = 0; x < 79; x++)
	{
	  /* Get the attr/char */
	  (void)(Term_what(x, y, &a, &c));
	  
	  /* Dump it */
	  buf[x] = c;
	}
      
      /* Terminate */
      buf[x] = '\0';
      
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
  msg_print("Screen dump saved.");
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
      msg_print("Screen dump failed.");
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
  html_screenshot(tmp_val);
  
  /* Recover current graphics settings */
  reset_visuals(TRUE);
  process_pref_file(file_name);
  file_delete(file_name);
  do_cmd_redraw();
  
  msg_print("HTML screen dump saved.");
  message_flush();
}


/**
 * Hack -- save a screen dump to a file
 */
void do_cmd_save_screen(void)
{
  event_type ke;
  msg_print("Dump type [(t)ext; (h)tml; (f)orum embedded html]:");
  add_button("f", 'f');
  add_button("h", 'h');
  add_button("t", 't');
  update_statusline();
  ke = inkey_ex();
  switch(ke.key) 
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
  kill_button('f');
  kill_button('t');
  kill_button('h');
  update_statusline();
  message_flush();
}

/** ISO C wrapper */
static void do_dump_options(void *not_a_function_pointer, const char *title)
{
  dump_pref_file(option_dump, title, 15);
}

/* ========================= MENU DEFINITIONS ========================== */



/*
 * Definition of the options menu.
 *
 * XXX Too many entries.
 */

static event_action option_actions [] = 
{
  {'i', "Interface options", do_cmd_options_aux, (void*)0}, 
  {'d', "Disturbance options", do_cmd_options_aux, (void*)1},
  {'g', "Game-play options", do_cmd_options_aux, (void*)2}, 
  {'e', "Efficiency options", do_cmd_options_aux, (void*)3}, 
  {'b', "Birth options", do_cmd_options_aux, (void*)4}, 
  {'x', "Cheat options", do_cmd_options_aux, (void*)5}, 
  {0, 0, 0, 0}, /* Load and append */
  {'w', "Subwindow display settings", (action_f) do_cmd_options_win, 0}, 
  {'s', "Item squelch settings", (action_f) do_cmd_options_item, 0}, 
  {'f', "Set base delay factor", (action_f) do_cmd_delay, 0}, 
  {'h', "Set hitpoint warning", (action_f) do_cmd_hp_warn, 0}, 
  {'p', "Set panel change factor", (action_f) do_cmd_panel_change, 0},
  {'a', "Autosave options", (action_f) do_cmd_options_autosave, 0},
  {0, 0, 0, 0}, /* Special choices */
  {'l', "Load a user pref file", (action_f) do_cmd_pref_file_hack, (void*)20},
  {'o', "Dump options", do_dump_options, 0}, 
  {0, 0, 0, 0}, /* Interact with */     
  {'m', "Interact with macros (advanced)", (action_f) do_cmd_macros, 0},
  {'v', "Interact with visuals (advanced)", (action_f) do_cmd_visuals, 0},
  {'c', "Interact with colours (advanced)", (action_f) do_cmd_colors, 0},
};

static menu_type option_menu;

static char tag_opt_main(menu_type *menu, int oid)
{
  if (option_actions[oid].id)
    return option_actions[oid].id;
  
  return 0;
}

static int valid_opt_main(menu_type *menu, int oid)
{
  if (option_actions[oid].name)
    return 1;
  
  return 0;
}

static void display_opt_main(menu_type *menu, int oid, bool cursor, int row, 
			     int col, int width)
{
  byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
  
  if (option_actions[oid].name)
    c_prt(attr, option_actions[oid].name, row, col);
}


static const menu_iter options_iter =
  {
    0,
    tag_opt_main,
    valid_opt_main,
    display_opt_main,
        NULL
  };


/**
 * Display the options main menu.
 */
void do_cmd_options(void)
{
  int cursor = 0;
  event_type c = EVENT_EMPTY;
  
  screen_save();
  menu_layout(&option_menu, &SCREEN_REGION);

  normal_screen = FALSE;
  kill_button(',');
  kill_button(' ');
  kill_button('\r');
  kill_button('n');
  kill_button('|');

  while (c.key != ESCAPE)
    {
      clear_from(0);
      c = menu_select(&option_menu, &cursor, 0);
      if (c.type == EVT_SELECT && option_actions[cursor].action)
	{
	  option_actions[cursor].action(option_actions[cursor].data,
					option_actions[cursor].name);
	}
    }
  
  normal_screen = TRUE;
  (void) add_button("Ret", '\r');
  (void) add_button("Spc", ' ');
  (void) add_button("Rpt", 'n');
  (void) add_button("Std", ',');
  (void) add_button("Inv", '|');
  screen_load();
}


/**
 * Definition of the "player knowledge" menu.
 */
static menu_item knowledge_actions[] =
  {
    {{0, "Display object knowledge", do_cmd_knowledge_objects, 0}, 'o'},
    {{0, "Display artifact knowledge", do_cmd_knowledge_artifacts, 0}, 'a'},
    {{0, "Display ego item knowledge", do_cmd_knowledge_ego_items, 0}, 'e'},
    {{0, "Display monster knowledge", do_cmd_knowledge_monsters, 0}, 'm'},
    {{0, "Display feature knowledge", do_cmd_knowledge_features, 0}, 'f'},
  };

static menu_type knowledge_menu;


/**
 * Display the "player knowledge" menu.
 */
void do_cmd_knowledge(void)
{
  int cursor = 0;
  int i;
  event_type c = EVENT_EMPTY;
  region knowledge_region = { 0, 0, -1, 11 };
  
  /* Grey out menu items that won't display anything */
  if (collect_known_artifacts(NULL, 0) > 0)
    knowledge_actions[1].flags = 0;
  else
    knowledge_actions[1].flags = MN_GREYED;
  
  knowledge_actions[2].flags = MN_GREYED;
  for (i = 0; i < z_info->e_max; i++)
    {
      if (e_info[i].everseen || cheat_xtra)
	{
	  knowledge_actions[2].flags = 0;
	  break;
	}
    }
  
  if (count_known_monsters() > 0)
    knowledge_actions[3].flags = 0;
  else
    knowledge_actions[3].flags = MN_GREYED;
  
  screen_save();

  /* Button stuff - keep the same as map screen because at the moment we can
   * only have one level of backup */
  normal_screen = FALSE;
  prompt_end = 0;
  menu_layout(&knowledge_menu, &knowledge_region);
  
  while (c.key != ESCAPE)
    {
      clear_from(0);
      update_statusline();
      c = menu_select(&knowledge_menu, &cursor, 0);
    }

  screen_load();
  normal_screen = TRUE;
  update_statusline();
}


/** Keep macro counts happy. */
static void cleanup_cmds(void)
{
  FREE(obj_group_order);
}


/**
 * Initialise all menus used here.
 */
void init_cmd4_c(void)
{
  /* some useful standard command keys */
  static const char cmd_keys[] = { ARROW_LEFT, ARROW_RIGHT, '\0' };
  
  /* Initialize the menus */
  menu_type *menu;                 
  
  /* options screen selection menu */
  menu = &option_menu;
  WIPE(menu, menu_type);
  menu_set_id(menu, OPTION_MENU);
  menu->title = "Options Menu";
  menu->menu_data = option_actions;
  menu->flags = MN_CASELESS_TAGS;
  menu->cmd_keys = cmd_keys;
  menu->count = N_ELEMENTS(option_actions);
  menu_init2(menu, find_menu_skin(MN_SCROLL), &options_iter, &SCREEN_REGION);
  
  /* Initialize the options toggle menu */
  menu = &option_toggle_menu;
  WIPE(menu, menu_type);
  menu->prompt = "Set option (y/n/t), '?' for information";
  menu->cmd_keys = "?Yy\n\rNnTt\x8C"; /* \x8c = ARROW_RIGHT */
  menu->selections = "abcdefghijklmopqrsuvwxz";
  menu->count = OPT_PAGE_PER;
  menu->flags = MN_DBL_TAP;
  menu_init2(menu, find_menu_skin(MN_SCROLL), &options_toggle_iter, 
	     &SCREEN_REGION);
  
  /* macro menu */
  menu = &macro_menu;
  WIPE(menu, menu_type);
  menu_set_id(menu, MACRO_MENU);
  menu->title = "Interact with macros";
  menu->cmd_keys = cmd_keys;
  menu->selections = default_choice;
  menu->menu_data = macro_actions;
  menu->count = N_ELEMENTS(macro_actions);
  menu_init(menu, MN_SCROLL, MN_EVT, &SCREEN_REGION);
  
  /* visuals menu */
  menu = &visual_menu;
  WIPE(menu, menu_type);
  menu_set_id(menu, VISUAL_MENU);
  menu->title = "Interact with visuals";
  menu->cmd_keys = cmd_keys;
  menu->selections = default_choice;
  menu->menu_data = visual_menu_items;
  menu->count = N_ELEMENTS(visual_menu_items);
  menu_init(menu, MN_SCROLL, MN_EVT, &SCREEN_REGION);
  
  /* colors menu */
  menu = &color_menu;
  WIPE(menu, menu_type);
  menu_set_id(menu, COLOR_MENU);
  menu->title = "Interact with colors";
  menu->cmd_keys = cmd_keys;
  menu->selections = default_choice;
  menu->menu_data = color_events;
  menu->count = N_ELEMENTS(color_events);
  menu_init(menu, MN_SCROLL, MN_EVT, &SCREEN_REGION);
  
  /* knowledge menu */
  menu = &knowledge_menu;
  WIPE(menu, menu_type);
  menu_set_id(menu, KNOWLEDGE_MENU);
  menu->title = "Display current knowledge";
  menu->menu_data = knowledge_actions;
  menu->count = N_ELEMENTS(knowledge_actions);
  menu_init(menu, MN_SCROLL, MN_ACT, &SCREEN_REGION);
  
  /* initialize other static variables */
  if (!obj_group_order)
    {
      int i;
      int gid = -1;
      
      C_MAKE(obj_group_order, TV_GOLD + 1, int);
      atexit(cleanup_cmds);
      
      /* Sllow for missing values */
      for (i = 0; i <= TV_GOLD; i++)
	obj_group_order[i] = -1;
      
      for (i = 0; 0 != object_text_order[i].tval; i++)
	{
	  if (object_text_order[i].name) gid = i;
	  obj_group_order[object_text_order[i].tval] = gid;
	}
    }
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
  msg_format("This is day %d. The time is %d:%02d %s.", day,
             (hour % 12 == 0) ? 12 : (hour % 12), min,
             (hour < 12) ? "AM" : "PM");
}



