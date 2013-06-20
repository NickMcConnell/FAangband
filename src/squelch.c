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
#include "button.h"
#include "cmds.h"
#include "squelch.h"
#include "ui-menu.h"


/**
 * Categories for quality squelch - must be the same as the one in cmd3.c
 */
static tval_desc quality_choices[TYPE_MAX] = 
{
    { TV_SWORD, "Swords" }, 
    { TV_POLEARM,"Polearms"  },
    { TV_HAFTED,"Blunt weapons"  },
    { TV_BOW, "Missile weapons" },
    { TV_SHOT, "Sling ammunition" },
    { TV_ARROW,"Bow ammunition"  },
    { TV_BOLT,"Crossbow ammunition"  },
    { TV_SOFT_ARMOR, "Soft armor" },
    { TV_HARD_ARMOR,"Hard armor"  },
    { TV_DRAG_ARMOR, "Dragon Scale Mail" },
    { TV_CLOAK,"Cloaks"  },
    { TV_SHIELD, "Shields" },
    { TV_HELM, "Helms" },
    { TV_CROWN, "Crowns" },
    { TV_GLOVES, "Gloves" },
    { TV_BOOTS,"Boots"  },
    { TV_DIGGING,"Diggers"  },
    { TV_RING, "Rings" },
    { TV_AMULET, "Amulets" },
};

bool squelch_profile[TYPE_MAX][SQUELCH_MAX];


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
  { TV_LIGHT,		"Lights" },
  { TV_FLASK,           "Oil" },
  { TV_SKELETON,        "Skeletons" },
  { TV_BOTTLE,          "Bottles" },
  { TV_JUNK,            "Junk" }
};


/*
 * Reset the player's squelch choices for a new game.
 */
void squelch_birth_init(void)
{
    int i, j;

    /* Reset squelch bits */
    for (i = 0; i < z_info->k_max; i++)
	k_info[i].squelch = FALSE;

    /* Reset ego squelch */
    for (i = 0; i < z_info->e_max; i++)
	e_info[i].squelch = FALSE;
  
    /* Clear the squelch bytes */
    for (i = 0; i < TYPE_MAX; i++)
	for (j = 0; j < SQUELCH_MAX; j++)
	    squelch_profile[i][j] = SQUELCH_NONE;
}

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
  const char *note = get_autoinscription(o_ptr->k_idx);
  const char *existing_inscription = quark_str(o_ptr->note);
  
  /* Don't inscribe unaware objects */
  if (!note || !object_aware_p(o_ptr))
    return 0;
  
  /* Don't re-inscribe if it's already correctly inscribed */
  if (existing_inscription && streq(note, existing_inscription))
    return 0;
  
  /* Get an object description */
  object_desc(o_name, sizeof(o_name), o_ptr,
	      ODESC_PREFIX | ODESC_FULL);
  
  if (note[0] != 0)
    o_ptr->note = quark_add(note);
  else
    o_ptr->note = 0;
  
  msg("You autoinscribe %s.", o_name);
  
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


int add_autoinscription(s16b kind, const char *inscription)
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
      msg("This inscription (%s) cannot be added because the inscription array is full!", inscription);
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
      if (!p_ptr->inventory[i].k_idx) continue;
      
      /* Apply the inscription */
      apply_autoinscription(&p_ptr->inventory[i]);
    }
  
  return;
}


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

/*
 * Find the squelch type of the object, or TYPE_MAX if none
 */
int squelch_type_of(const object_type *o_ptr)
{
    size_t i;
    
    /* Find the appropriate squelch group */
    for (i = 0; i < TYPE_MAX; i++)
    {
	if (quality_choices[i].tval == o_ptr->tval)
	    return i;
    }
    
    return TYPE_MAX;
}

int feel_to_squelch_level(int feel)
{
    switch (feel)
    {
    case FEEL_NONE: 
	return SQUELCH_NONE;
    case FEEL_DUBIOUS_STRONG: 
	return SQUELCH_KNOWN_DUBIOUS;
    case FEEL_PERILOUS: 
	return SQUELCH_KNOWN_PERILOUS;
    case FEEL_DUBIOUS_WEAK: 
	return SQUELCH_FELT_DUBIOUS;
    case FEEL_AVERAGE: 
	return SQUELCH_AVERAGE;
    case FEEL_GOOD_STRONG: 
	return SQUELCH_KNOWN_GOOD;
    case FEEL_EXCELLENT: 
	return SQUELCH_KNOWN_EXCELLENT;
    case FEEL_GOOD_WEAK: 
	return SQUELCH_FELT_GOOD;
    case FEEL_SPECIAL: 
	return SQUELCH_NONE;
    }

    return SQUELCH_NONE;
}

/**
 * Determines if an object is eligable for squelching.
 */
extern bool squelch_item_ok(const object_type *o_ptr)
{
    size_t i;
    int num = -1;
  
    object_kind *k_ptr = &k_info[o_ptr->k_idx];
    bool fullid = object_known_p(o_ptr);
    bool sensed = (o_ptr->ident & IDENT_SENSE) || fullid;
    byte feel   = fullid ? value_check_aux1((object_type *)o_ptr) : o_ptr->feel;
    int quality_squelch = SQUELCH_NONE;
  
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
    for (i = 0; i < N_ELEMENTS(quality_choices); i++)
    {
	if (quality_choices[i].tval == o_ptr->tval)
	{
	    num = i;
	    break;
	}
    }
  
    /* Never squelched */
    if (num == -1)
	return FALSE;
  
    /* Get result based on the feeling and the squelch_profile */
    quality_squelch = feel_to_squelch_level(feel);

    if (quality_squelch == SQUELCH_NONE)
	return FALSE;
    else
	return squelch_profile[num][quality_squelch];
}


/**
 * Returns TRUE if an item should be hidden due to the player's
 * current settings.
 */
bool squelch_hide_item(object_type *o_ptr)
{
  return (OPT(hide_squelchable) ? squelch_item_ok(o_ptr) : FALSE);
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
  floor_num = scan_floor(floor_list, N_ELEMENTS(floor_list), p_ptr->py, 
			 p_ptr->px, 0x01);
  
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
      o_ptr = &p_ptr->inventory[n];
      
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
      msgt(MSG_GENERIC, "%d item%s squelched.",
		     count, ((count > 1) ? "s" : ""));
      
      /* Combine/reorder the pack */
      p_ptr->notice |= (PN_COMBINE | PN_REORDER | PN_SORT_QUIVER);
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
      object_type *o_ptr = &p_ptr->inventory[n];
      
      /* Skip non-objects and unsquelchable objects */
      if (!o_ptr->k_idx) continue;
      if (!squelch_item_ok(o_ptr)) continue;

      /* Check for curses */
      if (cf_has(o_ptr->flags_curse, CF_STICKY_CARRY)) continue;
      
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



