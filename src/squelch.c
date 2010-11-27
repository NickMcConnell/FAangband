/** \file squelch.c
    \brief Item destruction
 
 *
 * The squelch code has a long history.  Originally it started out as a simple
 * sval-dependent item destroyer, but then ego-item and quality squelch was
 * added too, and then squelched items on the dungeon floor were marked by
 * purple dots, and by this time the code was quite unmaintainable and pretty
 * much impossible to work with.
 *
 * Luckily, though, it's been cleaned up.  There is now only sval-dependent
 * squelch and quality-based squelch, and the two don't interact -- 
 * quality-based is for items that get pseudo-id'd and sval-dependent is for 
 * potions and the like.
 *
 * The squelch code figures most things out itself.  Simply do:
 *     p_ptr->notice |= PN_SQUELCH;
 * whenever you want to make the game check for squelched items.
 *
 * The quality-dependent squelch is much reduced in scope from how it used to
 * be.  If less "general" settings are desired, they can be added easily enough
 * by changing entries in type_tvals[][], adding more TYPE_* constants, and
 * updating type_names.  Savefile compatibility is automatically ensured.
 *
 *
 * The UI code is much cleaner than it was before, but the interface itself
 * still needs some design.  XXX
 *

 * Copyright (c) 2009 David T. Blackston, Iain McFall, DarkGod, Jeff Greene,
 * David Vestal, Pete Mack, Andrew Sidwell, Nick McConnell.
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
 * Names of categories.
 */
static const char *type_names[TYPE_MAX] =
{
  "Swords",
  "Polearms",
  "Blunt weapons",
  "Missile weapons",
  "Sling ammunition",
  "Bow ammunition",
  "Crossbow ammunition",
  "Soft armor",
  "Hard armor",
  "Dragon Scale Mail",
  "Cloaks",
  "Shields",
  "Helms",
  "Crowns",
  "Gloves",
  "Boots",
  "Diggers",
  "Rings",
  "Amulets",
};

/**
 * Mapping of tval -> type 
 */
static int tvals[TYPE_MAX] =
{
  TV_SWORD, 
  TV_POLEARM,
  TV_HAFTED,
  TV_BOW,
  TV_SHOT,
  TV_ARROW,
  TV_BOLT,
  TV_SOFT_ARMOR,
  TV_HARD_ARMOR,
  TV_DRAG_ARMOR,
  TV_CLOAK,
  TV_SHIELD,
  TV_HELM,
  TV_CROWN,
  TV_GLOVES,
  TV_BOOTS,
  TV_DIGGING,
  TV_RING,
  TV_AMULET
};

byte squelch_level[TYPE_MAX];
size_t squelch_size = TYPE_MAX;


/**
 * The different kinds of quality squelch
 */
enum
{
  SQUELCH_NONE,
  SQUELCH_CURSED,
  SQUELCH_DUBIOUS,
  SQUELCH_DUBIOUS_NON,
  SQUELCH_NON_EGO,
  SQUELCH_AVERAGE,
  SQUELCH_GOOD_STRONG,
  SQUELCH_GOOD_WEAK,
  SQUELCH_ALL,
  
  SQUELCH_MAX
};

/**
 * The names for the various kinds of quality
 */
static const char *quality_names[SQUELCH_MAX] =
{
  "none",				             /* SQUELCH_NONE */
  "cursed (objects known to have a curse)",    	     /* SQUELCH_CURSED */
  "dubious (all dubious items)",                     /* SQUELCH_DUBIOUS */
  "dubious non-ego (strong pseudo-ID)",              /* SQUELCH_DUBIOUS_NON */
  "non-ego (all but ego-items - strong pseudo-ID)",  /* SQUELCH_NON_EGO */
  "average (everything not good or better)",         /* SQUELCH_AVERAGE */
  "good (strong pseudo-ID or identify)",             /* SQUELCH_GOOD_STRONG */
  "good (weak pseudo-ID)",		             /* SQUELCH_GOOD_WEAK */
  "everything except artifacts",	             /* SQUELCH_ALL */
};


/**
 * Structure to describe tval/description pairings. 
 */
typedef struct
{
  int tval;
  const char *desc;
} tval_desc;

/**
 * Categories for sval-dependent squelch. 
 */
static tval_desc sval_dependent[] =
{
  { TV_STAFF,		"Staffs" },
  { TV_WAND,		"Wands" },
  { TV_ROD,		"Rods" },
  { TV_SCROLL,		"Scrolls" },
  { TV_POTION,		"Potions" },
  { TV_FOOD,		"Food" },
  { TV_MAGIC_BOOK,	"Magic books" },
  { TV_PRAYER_BOOK,	"Prayer books" },
  { TV_DRUID_BOOK,	"Stones of Lore" },
  { TV_NECRO_BOOK,	"Necromantic tomes" },
  { TV_SPIKE,		"Spikes" },
  { TV_LITE,		"Lights" },
  { TV_FLASK,           "Oil" },
  { TV_SKELETON,        "Skeletons" },
  { TV_BOTTLE,          "Bottles" },
  { TV_JUNK,            "Junk" }
};


/*** Autoinscription stuff ***/

/**
 * This code needs documenting.
 */
int get_autoinscription_index(s16b k_idx)
{
  int i;
  
  for (i = 0; i < inscriptions_count; i++)
    {
      if (k_idx == inscriptions[i].kind_idx)
	return i;
    }
  
  return -1;
}

/**
 * DOCUMENT ME!
 */
const char *get_autoinscription(s16b kind_idx)
{
  int i;
  
  for (i = 0; i < inscriptions_count; i++)
    {
      if (kind_idx == inscriptions[i].kind_idx)
	return quark_str(inscriptions[i].inscription_idx);
    }
  
  return 0;
}

/**
 * Put the autoinscription on an object 
 */
int apply_autoinscription(object_type *o_ptr)
{
  char o_name[80];
  cptr note = get_autoinscription(o_ptr->k_idx);
  cptr existing_inscription = quark_str(o_ptr->note);
  
  /* Don't inscribe unaware objects */
  if (!note || !object_aware_p(o_ptr))
    return 0;
  
  /* Don't re-inscribe if it's already correctly inscribed */
  if (existing_inscription && streq(note, existing_inscription))
    return 0;
  
  /* Get an object description */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  if (note[0] != 0)
    o_ptr->note = quark_add(note);
  else
    o_ptr->note = 0;
  
  msg_format("You autoinscribe %s.", o_name);
  
  return 1;
}


int remove_autoinscription(s16b kind)
{
  int i = get_autoinscription_index(kind);
  
  /* It's not here. */
  if (i == -1) return 0;
  
  while (i < inscriptions_count - 1)
    {
      inscriptions[i] = inscriptions[i+1];
      i++;
    }
  
  inscriptions_count--;
  
  return 1;
}


int add_autoinscription(s16b kind, cptr inscription)
{
  int index;
  
  /* Paranoia */
  if (kind == 0) return 0;
  
  /* If there's no inscription, remove it */
  if (!inscription || (inscription[0] == 0))
    return remove_autoinscription(kind);
  
  index = get_autoinscription_index(kind);
  
  if (index == -1)
    index = inscriptions_count;
  
  if (index >= AUTOINSCRIPTIONS_MAX)
    {
      msg_format("This inscription (%s) cannot be added because the inscription array is full!", inscription);
      return 0;
    }
  
  inscriptions[index].kind_idx = kind;
  inscriptions[index].inscription_idx = quark_add(inscription);
  
  /* Only increment count if inscription added to end of array */
  if (index == inscriptions_count)
    inscriptions_count++;
  
  return 1;
}


void autoinscribe_ground(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  s16b this_o_idx, next_o_idx = 0;
  
  /* Scan the pile of objects */
  for (this_o_idx = cave_o_idx[py][px]; this_o_idx; this_o_idx = next_o_idx)
    {
      /* Get the next object */
      next_o_idx = o_list[this_o_idx].next_o_idx;
      
      /* Apply an autoinscription */
      apply_autoinscription(&o_list[this_o_idx]);
    }
}

void autoinscribe_pack(void)
{
  int i;
  
  /* Cycle through the inventory */
  for (i = INVEN_PACK; i > 0; i--)
    {
      /* Skip empty items */
      if (!inventory[i].k_idx) continue;
      
      /* Apply the inscription */
      apply_autoinscription(&inventory[i]);
    }
  
  return;
}

#ifdef _WIN32_WCE
/**
 * WinCE has no bsearch 
 */
typedef int (*comp_fn) (const void *, const void *);

void *my_bsearch(const void *key, const void *base, size_t num, size_t size, 
	       comp_fn my_comp);


void *my_bsearch(const void *key, const void *base, size_t num, size_t size,
	       int (*my_comp)(const void *, const void *))
{
  size_t odd_mask, bytes;
  const char *centre, *high, *low;
  int comp;
  
  odd_mask = ((size ^ (size - 1)) >> 1) + 1;
  low = base;
  bytes = num == 0 ? size : size + 1;
  centre = low + num * size;
  comp = 0;
  while (bytes != size) 
    {
      if (comp > 0) 
	{
	  low = centre;
	} 
      else 
	{
	  high = centre;
	}
      bytes = high - low;
      centre = low + ((bytes & odd_mask ? bytes - size : bytes) >> 1);
      comp = my_comp(key, centre);
      if (comp == 0) 
	{
	  return (void *)centre;
	}
    }
  return NULL;
}
#endif


/*** Squelch code ***/

/**
 * Determines whether a tval is eligable for sval-squelch.
 */
bool squelch_tval(int tval)
{
  size_t i;
  
  /* Only squelch if the tval's allowed */
  for (i = 0; i < N_ELEMENTS(sval_dependent); i++)
    {
      if (tval == sval_dependent[i].tval)
	return TRUE;
    }
  
  return FALSE;
}


/**
 * Determines if an object is eligable for squelching.
 */
extern bool squelch_item_ok(object_type *o_ptr)
{
  size_t i;
  int num = -1;
  
  object_kind *k_ptr = &k_info[o_ptr->k_idx];
  bool fullid = object_known_p(o_ptr);
  bool sensed = (o_ptr->ident & IDENT_SENSE) || fullid;
  byte feel   = fullid ? value_check_aux1(o_ptr) : o_ptr->feel;
  
  
  /* Don't squelch artifacts */
  if (artifact_p(o_ptr)) return FALSE;
  
  /* Don't squelch stuff inscribed not to be destroyed (!k) */
  if (check_for_inscrip(o_ptr, "!k") || check_for_inscrip(o_ptr, "!*"))
    {
      return FALSE;
    }
  
  /* Auto-squelch dead chests */
  if (o_ptr->tval == TV_CHEST && o_ptr->pval == 0)
    return TRUE;
  
  /* Do squelching by sval, if we 'know' the flavour. */
  if (k_ptr->squelch && (k_ptr->flavor == 0 || k_ptr->aware))
    {
      if (squelch_tval(k_info[o_ptr->k_idx].tval))
	return TRUE;
    }
  
  /* Squelch some ego items if known */
  if (has_ego_properties(o_ptr) && (e_info[o_ptr->name2].squelch))
    {
      return TRUE;
    }

  
  /* Don't check pseudo-ID for nonsensed things */
  if (!sensed) return FALSE;
  
  
  
  /* Find the appropriate squelch group */
  for (i = 0; i < N_ELEMENTS(tvals); i++)
    {
      if (tvals[i] == o_ptr->tval)
	{
	  num = i;
	  break;
	}
    }
  
  /* Never squelched */
  if (num == -1)
    return FALSE;
  
  
  /* Get result based on the feeling and the squelch_level */
  switch (squelch_level[num])
    {
    case SQUELCH_CURSED:
      {
	if (o_ptr->ident & IDENT_CURSED)
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_DUBIOUS:
      {
	if ((feel == FEEL_DUBIOUS_WEAK) || (feel == FEEL_PERILOUS) ||
	    (feel == FEEL_DUBIOUS_STRONG))
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_DUBIOUS_NON:
      {
	if (feel == FEEL_DUBIOUS_STRONG)
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_NON_EGO:
      {
	if ((feel == FEEL_DUBIOUS_STRONG) || (feel == FEEL_AVERAGE) || 
	    (feel == FEEL_GOOD_STRONG))
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_AVERAGE:
      {
	if ((feel == FEEL_DUBIOUS_WEAK) || (feel == FEEL_PERILOUS) ||
	    (feel == FEEL_DUBIOUS_STRONG) || (feel == FEEL_AVERAGE))
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_GOOD_STRONG:
      {
	if ((feel == FEEL_PERILOUS) || (feel == FEEL_DUBIOUS_STRONG) || 
	    (feel == FEEL_AVERAGE) || (feel == FEEL_GOOD_STRONG))
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_GOOD_WEAK:
      {
	if ((feel == FEEL_PERILOUS) || (feel == FEEL_DUBIOUS_STRONG) || 
	    (feel == FEEL_AVERAGE) || (feel == FEEL_GOOD_WEAK) ||
	    (feel == FEEL_GOOD_STRONG))
	  {
	    return TRUE;
	  }
	
	break;
      }
      
    case SQUELCH_ALL:
      {
	return TRUE;
	break;
      }
    }
  
  /* Failure */
  return FALSE;
}


/**
 * Returns TRUE if an item should be hidden due to the player's
 * current settings.
 */
bool squelch_hide_item(object_type *o_ptr)
{
  return (hide_squelchable ? squelch_item_ok(o_ptr) : FALSE);
}


/**
 * Destroy all {squelch}able items.
 *
 * Imported, with thanks, from Ey... much cleaner than the original.
 */
void squelch_items(void)
{
  int floor_list[MAX_FLOOR_STACK];
  int floor_num, n;
  int count = 0;
  
  object_type *o_ptr;
  
  /* Set the hook and scan the floor */
  item_tester_hook = squelch_item_ok;
  (void)scan_floor(floor_list, &floor_num, p_ptr->py, p_ptr->px, 0x01);
  
  if (floor_num)
    {
      for (n = 0; n < floor_num; n++)
	{
	  o_ptr = &o_list[floor_list[n]];
	  
	  /* Avoid artifacts */
	  if (artifact_p(o_ptr)) continue;
	  
	  if (item_tester_okay(o_ptr))
	    {
	      /* Destroy item */
	      floor_item_increase(floor_list[n], -o_ptr->number);
	      floor_item_optimize(floor_list[n]);
	      count++;
	    }
	}
    }
  
  /* Scan through the slots backwards */
  for (n = INVEN_PACK - 1; n >= 0; n--)
    {
      o_ptr = &inventory[n];
      
      /* Skip non-objects and artifacts */
      if (!o_ptr->k_idx) continue;
      if (artifact_p(o_ptr)) continue;
      
      if (item_tester_okay(o_ptr))
	{
	  /* Destroy item */
	  inven_item_increase(n, -o_ptr->number);
	  inven_item_optimize(n);
	  count++;
	}
    }
  
  item_tester_hook = NULL;
  
  /* Mention casualties */
  if (count > 0)
    {
      message_format(MSG_GENERIC, 0, "%d item%s squelched.",
		     count, ((count > 1) ? "s" : ""));
      
      /* Combine/reorder the pack */
      p_ptr->notice |= (PN_COMBINE | PN_REORDER);
    }
}


/**
 * Drop all {squelch}able items.
 */
extern void squelch_drop(void)
{
  int n;
  
  /* Scan through the slots backwards */
  for (n = INVEN_PACK - 1; n >= 0; n--)
    {
      object_type *o_ptr = &inventory[n];
      
      /* Skip non-objects and unsquelchable objects */
      if (!o_ptr->k_idx) continue;
      if (!squelch_item_ok(o_ptr)) continue;

      /* Check for curses */
      if (o_ptr->flags_curse & (CF_STICKY_CARRY)) continue;
      
      /* Check for !d (no drop) inscription */
      if (!check_for_inscrip(o_ptr, "!d") && !check_for_inscrip(o_ptr, "!*"))
	{
	  /* We're allowed to drop it. */
	  inven_drop(n, o_ptr->number);
	}
    }
  
  /* Combine/reorder the pack */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
}



/*** Ego item squelch menu ***/

static tval_desc raw_tvals[] =
  {
    {TV_SKELETON, "Skeletons"},
    {TV_BOTTLE, "Bottles"},
    {TV_JUNK, "Junk"},
    {TV_SPIKE, "Spikes"},
    {TV_CHEST, "Chests"},
    {TV_SHOT, "Shots"},
    {TV_ARROW, "Arrows"},
    {TV_BOLT, "Bolts"},
    {TV_BOW, "Launchers"},
    {TV_DIGGING, "Diggers"},
    {TV_HAFTED, "Blunt weapons"},
    {TV_POLEARM, "Polearms"},
    {TV_SWORD, "Swords"},
    {TV_BOOTS, "Boots"},
    {TV_GLOVES, "Gloves"},
    {TV_HELM, "Helmets"},
    {TV_CROWN, "Crowns"},
    {TV_SHIELD, "Shields"},
    {TV_CLOAK, "Cloaks"},
    {TV_SOFT_ARMOR, "Soft Armor"},
    {TV_HARD_ARMOR, "Hard Armor"},
    {TV_DRAG_ARMOR, "DSMails"},
    {TV_LITE, "Lights"},
    {TV_AMULET, "Amulets"},
    {TV_RING, "Rings"},
    {TV_STAFF, "Staves"},
    {TV_WAND, "Wands"},
    {TV_ROD, "Rods"},
    {TV_SCROLL, "Scrolls"},
    {TV_POTION, "Potions"},
    {TV_FLASK, "Flaskes"},
    {TV_FOOD, "Food"},
    {TV_MAGIC_BOOK, "Magic Books"},
    {TV_PRAYER_BOOK, "Prayer Books"},
    {TV_DRUID_BOOK, "Stones of Lore"},
    {TV_NECRO_BOOK, "Necromantic Tomes"}
};

#define NUM_RAW_TVALS (sizeof(raw_tvals) / sizeof(raw_tvals[0]))

typedef struct ego_desc
{
  s16b e_idx;
  const char *short_name;
} ego_desc;

/**
 * Skip common prefixes in ego-item names.
 */
static const char *strip_ego_name(const char *name)
{
  if (prefix(name, "of the "))	return name + 7;
  if (prefix(name, "of "))	return name + 3;
  return name;
}

/**
 * Utility function used to find/sort tval names.
 */
static int tval_comp_func(const void *a_ptr, const void *b_ptr)
{
  int a = ((tval_desc *)a_ptr)->tval;
  int b = ((tval_desc *)b_ptr)->tval;
  return a - b;
}

/**
 * Display an ego-item type on the screen.
 */
int ego_item_name(char *buf, size_t buf_size, ego_desc *d_ptr)
{
  byte tval_table[EGO_TVALS_MAX], i, n = 0;
  char *end;
  size_t prefix_size;
  const char *long_name;
  
  ego_item_type *e_ptr = &e_info[d_ptr->e_idx];
  
  /* Copy the valid tvals of this ego-item type */
  for (i = 0; i < EGO_TVALS_MAX; i++)
    {
      /* Ignore "empty" entries */
      if (e_ptr->tval[i] < 1) continue;
      
      /* Append valid tvals */
      tval_table[n++] = e_ptr->tval[i];
    }
  
  /* Sort the tvals using bubble sort  */
  for (i = 0; i < n; i++)
    {
      int j;
      
      for (j = i + 1; j < n; j++)
	{
	  if (tval_table[i] > tval_table[j])
	    {
	      byte temp = tval_table[i];
	      tval_table[i] = tval_table[j];
	      tval_table[j] = temp;
	    }
	}
    }
  
  /* Initialize the buffer */
  end = my_fast_strcat(buf, NULL, "[ ] ", buf_size);
  
  /* Concatenate the tval' names */
  for (i = 0; i < n; i++)
    {
      /* Fast searching */
      tval_desc key, *result;
      const char *tval_name;
      
      /* Find the tval's name using binary search */
      key.tval = tval_table[i];
      key.desc = NULL;
      
#ifdef _WIN32_WCE
      result = my_bsearch(&key, raw_tvals, NUM_RAW_TVALS, 
		       sizeof(raw_tvals[0]), tval_comp_func);
#else
      result = bsearch(&key, raw_tvals, NUM_RAW_TVALS, 
		       sizeof(raw_tvals[0]), tval_comp_func);
#endif
    
      if (result) tval_name = result->desc;
      /* Paranoia */
      else	tval_name = "????";
      
      /* Append the proper separator first, if any */
      if (i > 0)
	{
	  end = my_fast_strcat(buf, end, (i < n - 1) ? ", ": " and ", buf_size);
	}
      
      /* Append the name */
      end = my_fast_strcat(buf, end, tval_name, buf_size);
    }
  
  /* Append an extra space */
  end = my_fast_strcat(buf, end, " ", buf_size);
  
  /* Get the full ego-item name */
  long_name = e_name + e_ptr->name;
  
  /* Get the length of the common prefix, if any */
  prefix_size = (d_ptr->short_name - long_name);
  
  /* Found a prefix? */
  if (prefix_size > 0)
    {
      char prefix[100];
      
      /* Get a copy of the prefix */
      my_strcpy(prefix, long_name, prefix_size + 1);
      
      /* Append the prefix */
      end = my_fast_strcat(buf, end, prefix, buf_size);
    }
  /* Set the name to the right length */
  return (int)(buf - end);
}

/**
 * Utility function used for sorting an array of ego-item indices by
 * ego-item name.
 */
static int ego_comp_func(const void *a_ptr, const void *b_ptr)
{
  const ego_desc *a = a_ptr;
  const ego_desc *b = b_ptr;
  
  /* Note the removal of common prefixes */
  return (strcmp(a->short_name, b->short_name));
}

/*** Ego squelch menu ***/

/**
 * Display an entry on the sval menu
 */
static void ego_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  char buf[80] = "";
  int length;
  ego_desc *choice = (ego_desc *)menu->menu_data;
  ego_item_type *e_ptr = &e_info[choice[oid].e_idx];

  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  byte sq_attr = (e_ptr->squelch ? TERM_L_RED : TERM_L_GREEN);
  
  /* Acquire the "name" of object "i" */
  length = ego_item_name(buf, sizeof(buf), &choice[oid]);
  
  /* Print it */
  c_put_str(attr, format("%s", buf), row, col);

  /* Show squelch mark, if any */
  if (e_ptr->squelch) c_put_str(TERM_L_RED, "*", row, col + 1);
  
  /* Show the stripped ego-item name using another colour */
  c_put_str(sq_attr, choice[oid].short_name, row, col + strlen(buf));
}

/**
 * Deal with events on the sval menu
 */
static bool ego_action(char cmd, void *db, int oid)
{
  ego_desc *choice = db;
  
  /* Toggle */
  if (cmd == '\n' || cmd == '\r' || cmd == '\xff')
    {
      ego_item_type *e_ptr = &e_info[choice[oid].e_idx];
      e_ptr->squelch = !e_ptr->squelch;
      
      return TRUE;
    }
  
  return FALSE;
}

/**
 * Display list of ego items to be squelched.
 */
static void ego_menu(void *unused, const char *also_unused)
{
  int idx, max_num = 0;
  ego_item_type *e_ptr;
  ego_desc *choice;
  
  menu_type menu;
  menu_iter menu_f = { 0, 0, 0, ego_display, ego_action };
  region area = { 1, 5, -1, -1 };
  event_type evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;
  
  size_t i;
  
  
  /* Hack - Used to sort the tval table for the first time */
  static bool sort_tvals = TRUE;
  
  /* Sort the tval table if needed */
  if (sort_tvals)
    {
      sort_tvals = FALSE;
      
      qsort(raw_tvals, NUM_RAW_TVALS, sizeof(raw_tvals[0]), tval_comp_func);
    }
  
  /* Create the array */
  choice = C_ZNEW(alloc_ego_size, ego_desc);
  
  /* Get the valid ego-items */
  for (i = 0; i < alloc_ego_size; i++)
    {
      idx = alloc_ego_table[i].index;
      
      e_ptr = &e_info[idx];
      
      /* Only valid known ego-items allowed */
      if (!e_ptr->name || !e_ptr->everseen) continue;
      
      /* Append the index */
      choice[max_num].e_idx = idx;
      choice[max_num].short_name = strip_ego_name(e_name + e_ptr->name);
      
      ++max_num;
    }
  
  /* Quickly sort the array by ego-item name */
  qsort(choice, max_num, sizeof(choice[0]), ego_comp_func);
  
  /* Return here if there are no objects */
  if (!max_num)
    {
      FREE(choice);
      return;
    }
  
  
  /* Save the screen and clear it */
  screen_save();
  clear_from(0);
  
  /* Buttons */
  add_button("Up", ARROW_UP);
  add_button("Down", ARROW_DOWN);
  add_button("Toggle", '\r');
  update_statusline();
  
  /* Output to the screen */
  text_out_hook = text_out_to_screen;
  
  /* Indent output */
  text_out_indent = 1;
  text_out_wrap = 79;
  Term_gotoxy(1, 0);
  
  /* Display some helpful information */
  text_out("Use the ");
  text_out_c(TERM_L_GREEN, "movement keys");
  text_out(" to scroll the list or ");
  text_out_c(TERM_L_GREEN, "ESC");
  text_out(" to return to the previous menu.  ");
  text_out_c(TERM_L_BLUE, "Enter");
  text_out(" toggles the current setting.");
  
  text_out_indent = 0;
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.cmd_keys = " \n\r";
  menu.count = max_num;
  menu.menu_data = choice;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);
  
  /* Select an entry */
  while (evt.key != ESCAPE)
    evt = menu_select(&menu, &cursor, 0);
  
  /* Free memory */
  FREE(choice);
  
  /* Load screen */
  screen_load();

  /* Buttons */
  kill_button(ARROW_UP);
  kill_button(ARROW_DOWN);
  kill_button('\r');
  update_statusline();

  return;
}

/*** Quality-squelch menu ***/

/**
 * Display an entry in the menu.
 */
static void quality_display(menu_type *menu, int oid, bool cursor, int row, 
			    int col, int width)
{
  const char *name = type_names[oid];
  
  byte level = squelch_level[oid];
  const char *level_name = quality_names[level];
  
  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  
  c_put_str(attr, format("%-20s : %s", name, level_name), row, col);
}


/**
 * Display the quality squelch subtypes.
 */
static void quality_subdisplay(menu_type *menu, int oid, bool cursor, int row, 
			       int col, int width)
{
  const char *name = quality_names[oid];
  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  c_put_str(attr, name, row, col);
}

/**
 * Handle "Enter".  :(
 */
static bool quality_subaction(char cmd, void *db, int oid)
{
  return TRUE;
}


/**
 * Handle keypresses.
 */
static bool quality_action(char cmd, void *db, int oid)
{
  menu_type menu;
  menu_iter menu_f = { 0, 0, 0, quality_subdisplay, quality_subaction };
  region area = { 24, 1, 44, SQUELCH_MAX };
  event_type evt;
  int cursor;
  
  /* Display at the right point */
  area.row += oid;
  cursor = squelch_level[oid];
  
  /* Save */
  screen_save();
  
  /* Run menu */
  WIPE(&menu, menu);
  menu.cmd_keys = "\n\r";
  menu.count = SQUELCH_MAX;
  if ((tvals[oid] == TV_AMULET) || (tvals[oid] == TV_RING))
    menu.count = area.page_rows = SQUELCH_CURSED + 1;
  
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);
  window_make(area.col - 2, area.row - 1, area.col + area.width + 2, 
	      area.row + area.page_rows);

  evt = menu_select(&menu, &cursor, 0);
  
  /* Set the new value appropriately */
  if (evt.key != ESCAPE && evt.type != EVT_BACK)
    squelch_level[oid] = cursor;
  
  /* Load and finish */
  screen_load();
  return TRUE;
}

/**
 * Display quality squelch menu.
 */
static void quality_menu(void *unused, const char *also_unused)
{
  menu_type menu;
  menu_iter menu_f = { 0, 0, 0, quality_display, quality_action };
  region area = { 1, 3, -1, -1 };
  event_type evt = EVENT_EMPTY;
  int cursor = 0;
  
  /* Save screen */
  screen_save();
  clear_from(0);
  
  /* Help text */
  prt("Quality squelch menu", 0, 0);
  
  Term_gotoxy(1, 1);
  text_out_to_screen(TERM_L_RED, "Use the movement keys to navigate, and Enter to change settings.");
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.cmd_keys = " \n\r";
  menu.count = TYPE_MAX;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);
  
  /* Select an entry */
  while (evt.key != ESCAPE)
    evt = menu_select(&menu, &cursor, 0);
  
  /* Load screen */
  screen_load();
  return;
}



/*** Sval-dependent menu ***/

/**
 * Display an entry on the sval menu
 */
static void sval_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  char buf[80];
  const u16b *choice = menu->menu_data;
  int idx = choice[oid];

  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  
  /* Acquire the "name" of object "i" */
  object_kind_name(buf, sizeof(buf), idx, TRUE);
  
  /* Print it */
  c_put_str(attr, format("[ ] %s", buf), row, col);
  if (k_info[idx].squelch)
    c_put_str(TERM_L_RED, "*", row, col + 1);
}

/**
 * Deal with events on the sval menu
 */
static bool sval_action(char cmd, void *db, int oid)
{
  u16b *choice = db;
  
  /* Toggle */
  if (cmd == '\n' || cmd == '\r' || cmd == '\xff')
    {
      int idx = choice[oid];
      k_info[idx].squelch = !k_info[idx].squelch;
      
      return TRUE;
    }
  
  return FALSE;
}

/**
 * Hack - special artifacts are not squelchable 
 */
#define LAST_NORMAL 730

/**
 * Display list of svals to be squelched.
 */
static bool sval_menu(int tval, const char *desc)
{
  menu_type menu;
  menu_iter menu_f = { 0, 0, 0, sval_display, sval_action };
  region area = { 1, 5, -1, -1 };
  event_type evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;
  
  int num = 0;
  size_t i;
  
  u16b *choice;
  
  
  /* Create the array */
  choice = C_ZNEW(LAST_NORMAL, u16b);
  
  /* Iterate over all possible object kinds, finding ones which 
   * can be squelched */
  for (i = 1; i < LAST_NORMAL; i++)
    {
      object_kind *k_ptr = &k_info[i];
      
      /* Skip empty objects, unseen objects, incorrect tvals and artifacts */
      if (!k_ptr->name) continue;
      if (!k_ptr->everseen) continue;
      if (k_ptr->tval != tval) continue;
      if (k_ptr->flags_kind & KF_INSTA_ART) continue;

      /* Add this item to our possibles list */
      choice[num++] = i;
    }
  
  /* Return here if there are no objects */
  if (!num)
    {
      FREE(choice);
      return FALSE;
    }
  
  
  /* Save the screen and clear it */
  screen_save();
  clear_from(0);
  
  /* Buttons */
  add_button("Up", ARROW_UP);
  add_button("Down", ARROW_DOWN);
  add_button("Toggle", '\r');
  update_statusline();
  
  /* Output to the screen */
  text_out_hook = text_out_to_screen;
  
  /* Indent output */
  text_out_indent = 1;
  text_out_wrap = 79;
  Term_gotoxy(1, 0);
  
  /* Display some helpful information */
  text_out("Use the ");
  text_out_c(TERM_L_GREEN, "movement keys");
  text_out(" to scroll the list or ");
  text_out_c(TERM_L_GREEN, "ESC");
  text_out(" to return to the previous menu.  ");
  text_out_c(TERM_L_BLUE, "Enter");
  text_out(" toggles the current setting.");
  
  text_out_indent = 0;
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.cmd_keys = " \n\r";
  menu.count = num;
  menu.menu_data = choice;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);
  
  /* Select an entry */
  while (evt.key != ESCAPE)
    evt = menu_select(&menu, &cursor, 0);
  
  /* Free memory */
  FREE(choice);
  
  /* Load screen */
  screen_load();

  /* Buttons */
  kill_button(ARROW_UP);
  kill_button(ARROW_DOWN);
  kill_button('\r');
  update_statusline();

  return TRUE;
}


/**
 * Returns TRUE if there's anything to display a menu of 
 */
extern bool seen_tval(int tval)
{
  int i;
  
  for (i = 1; i < z_info->k_max; i++)
    {
      object_kind *k_ptr = &k_info[i];
      
      /* Skip empty objects, unseen objects, and incorrect tvals */
      if (!k_ptr->name) continue;
      if (!k_ptr->everseen) continue;
      if (k_ptr->tval != tval) continue;
      
      return TRUE;
    }
  
  
  return FALSE;
}


/** Extra options on the "item options" menu */
struct
{
  char tag;
  char *name;
  void (*action)(void *unused, const char *also_unused);
} extra_item_options[] =
  {
    { 'Q', "Quality squelching options", quality_menu },
    { 'E', "Ego squelching options", ego_menu },
    { '{', "Autoinscription setup", do_cmd_knowledge_objects },
  };

static char tag_options_item(menu_type *menu, int oid)
{
  size_t line = (size_t) oid;
  
  if (line < N_ELEMENTS(sval_dependent))
    return I2A(oid);
  
  /* Separator - blank line. */
  if (line == N_ELEMENTS(sval_dependent))
    return 0;
  
  line = line - N_ELEMENTS(sval_dependent) - 1;
  
  if (line < N_ELEMENTS(extra_item_options))
    return extra_item_options[line].tag;
  
  return 0;
}

static int valid_options_item(menu_type *menu, int oid)
{
  size_t line = (size_t) oid;
  
  if (line < N_ELEMENTS(sval_dependent))
    return 1;
  
  /* Separator - blank line. */
  if (line == N_ELEMENTS(sval_dependent))
    return 0;
  
  line = line - N_ELEMENTS(sval_dependent) - 1;
  
  if (line < N_ELEMENTS(extra_item_options))
    return 1;
  
  return 0;
}

static void display_options_item(menu_type *menu, int oid, bool cursor, 
				 int row, int col, int width)
{
  size_t line = (size_t) oid;
  
  /* First section of menu - the svals */
  if (line < N_ELEMENTS(sval_dependent))
    {
      bool known = seen_tval(sval_dependent[line].tval);
      byte attr = curs_attrs[known ? CURS_KNOWN: CURS_UNKNOWN][(int)cursor];
      
      c_prt(attr, sval_dependent[line].desc, row, col);
    }
  /* Second section - the "extra options" */
  else
    {
      byte attr = curs_attrs[CURS_KNOWN][(int)cursor];
      
      line = line - N_ELEMENTS(sval_dependent) - 1;
      
      if (line < N_ELEMENTS(extra_item_options))
	c_prt(attr, extra_item_options[line].name, row, col);
    }
}


static const menu_iter options_item_iter =
{
  0,
  tag_options_item,
  valid_options_item,
  display_options_item,
  NULL
};


/**
 * Display and handle the main squelching menu.
 */
void do_cmd_options_item(void *unused, cptr title)
{
  int cursor = 0;
  event_type c = EVENT_EMPTY;
  const char cmd_keys[] = { ARROW_LEFT, ARROW_RIGHT, '\0' };
  
  menu_type menu;
  
  WIPE(&menu, menu_type);
  menu.title = title;
  menu.cmd_keys = cmd_keys;
  menu.count = N_ELEMENTS(sval_dependent) + N_ELEMENTS(extra_item_options) + 1;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &options_item_iter, 
	     &SCREEN_REGION);
  
  menu_layout(&menu, &SCREEN_REGION);
  
  /* Save and clear screen */
  screen_save();
  clear_from(0);
  
  while (c.key != ESCAPE)
    {
      clear_from(0);
      c = menu_select(&menu, &cursor, 0);
      
      if (c.type == EVT_SELECT)
	{
	  if ((size_t) cursor < N_ELEMENTS(sval_dependent))
	    {
	      sval_menu(sval_dependent[cursor].tval, sval_dependent[cursor].desc);
	    }
	  else
	    {
	      cursor = cursor - N_ELEMENTS(sval_dependent) - 1;
	      if ((size_t) cursor < N_ELEMENTS(extra_item_options))
		extra_item_options[cursor].action(NULL, NULL);
	    }
	}
    }
  
  /* Load screen */
  screen_load();

  /* Notice changes */
  p_ptr->notice |= PN_SQUELCH;

  return;
}
