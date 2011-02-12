/** \file cmd3.c 
    \brief Commands, part 3

 * Inventory and equipment display and management interface, observing an 
 * object, inscribing, refuelling, (l)ooking around the screen and 
 * Looking around the dungeon, help info on textual chars ("8" is the home, 
 * etc.), monster memory interface, stealing and setting monster traps.
 * 
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "ui-menu.h"


/**
 * Display inventory - new - not quite ready
 */
void do_cmd_inven_new(void)
{
  int i, k, z = 0, col, len = 3;

  char tmp_val[80];
  
  object_type *o_ptr;
  
  /* Count number of missiles in the quiver slots. */
  int ammo_num = quiver_count();

  /* Find the "final" slot */
  for (i = 0; i < INVEN_PACK; i++)
    {
      o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Track */
      z = i + 1;
    }
  
  /* 
   * Add notes about slots used by the quiver, if we have space, want 
   * to show all slots, and have items in the quiver.
   */

  /* Find the column to start in */
  if (len > Term->wid - 4 ) col = 0;
  else if ((Term->wid - len) / 2 < 12) col = (Term->wid -len) / 2;
  else col = 12;
  
  if ((p_ptr->pack_size_reduce) && 
      (z <= (INVEN_PACK - p_ptr->pack_size_reduce)))
    {
      /* Insert a blank dividing line, if we have the space. */
      if (z <= ((INVEN_PACK - 1) - p_ptr->pack_size_reduce))
	{
	  z++;
	  prt("", z, col ? col - 2 : col);
	}
      
      for (i = 1; i <= p_ptr->pack_size_reduce; i++)
	{
	  /* Go to next line. */
	  z++;
	  prt("", z, col ? col - 2 : col);
	  
	  /* Determine index, print it out. */
	  sprintf(tmp_val, "%c)", index_to_label(INVEN_PACK - i));
	  put_str(tmp_val, z, col);
	  
	  /* Note amount of ammo */
	  k = (ammo_num > 99) ? 99 : ammo_num;
	  
	  /* Hack -- use "(Ready Ammunition)" as a description. */
	  c_put_str(TERM_BLUE, format("(Ready Ammunition) [%2d]", k), 
		    z, col + 3);

	  /* Reduce ammo count */
	  ammo_num -= k;
	}
    }

  do_cmd_show_obj();
}




/**
 * Display inventory
 */
void do_cmd_inven(void)
{
  char string[80];
  
  /* Note that we are in "inventory" mode. */
  p_ptr->command_wrk = (USE_INVEN);
  
  
  /* Save screen */
  screen_save();
  
  /* Hack -- show empty slots */
  item_tester_full = TRUE;
  
  /* Display the inventory */
  show_inven();
  
  /* Hack -- hide empty slots */
  item_tester_full = FALSE;
  
  /* Insert the total burden and character capacity into a string. */
  if (OPT(use_metric)) 
    sprintf(string,"(Inventory) burden %d.%d kg (%d%% of capacity). Command: ",
	    make_metric(p_ptr->total_weight) / 10, 
	    make_metric(p_ptr->total_weight) % 10, 
	    p_ptr->total_weight / adj_str_wgt[p_ptr->stat_ind[A_STR]]);
  else 
    sprintf(string, 
	    "(Inventory) burden %d.%d lb (%d%% of capacity). Command: ",
	    p_ptr->total_weight / 10, p_ptr->total_weight % 10, 
	    p_ptr->total_weight / adj_str_wgt[p_ptr->stat_ind[A_STR]]);


  /* Output that string, and prompt for a command. */
  prt(string, 0, 0);
  
  /* Hack -- Get a new command */
  p_ptr->command_new = inkey();
  
  /* Load screen */
  screen_load();
  
  
  /* Hack -- Process "Escape" */
  if (p_ptr->command_new == ESCAPE)
    {
      /* Reset stuff */
      p_ptr->command_new = 0;
    }
  
  /* Hack -- Process normal keys */
  else
    {
      /* Hack -- Use "display" mode */
      p_ptr->command_see = TRUE;
    }
}


/**
 * Display equipment
 */
void do_cmd_equip(void)
{
  char string[80];
  
  /* Note that we are in "equipment" mode */
  p_ptr->command_wrk = (USE_EQUIP);
  
  
  /* Save screen */
  screen_save();
  
  /* Hack -- show empty slots */
  item_tester_full = TRUE;
  
  /* Display the equipment */
  show_equip();
  
  /* Hack -- undo the hack above */
  item_tester_full = FALSE;
  
  
  /* Insert the total burden and character capacity into a string. */
  if (OPT(use_metric)) 
    sprintf(string, 
	    "(Equipment) burden %d.%d kg (%d%% of capacity). Command: ",
	    make_metric(p_ptr->total_weight) / 10, 
	    make_metric(p_ptr->total_weight) % 10, 
	    p_ptr->total_weight / adj_str_wgt[p_ptr->stat_ind[A_STR]]);
  else 
    sprintf(string, 
	    "(Equipment) burden %d.%d lb (%d%% of capacity). Command: ",
	    p_ptr->total_weight / 10, p_ptr->total_weight % 10, 
	    p_ptr->total_weight / adj_str_wgt[p_ptr->stat_ind[A_STR]]);
  
  
  /* Output that string, and prompt for a command. */
  prt(string, 0, 0);
  
  /* Hack -- Get a new command */
  p_ptr->command_new = inkey();
  
  /* Load screen */
  screen_load();
  
  
  /* Hack -- Process "Escape" */
  if (p_ptr->command_new == ESCAPE)
    {
      /* Reset stuff */
      p_ptr->command_new = 0;
    }
  
  /* Hack -- Process normal keys */
  else
    {
      /* Enter "display" mode */
      p_ptr->command_see = TRUE;
    }
}




/**
 * Destroy an item
 *
 * No longer takes a turn -BR-
 */
void do_cmd_destroy(cmd_code code, cmd_arg args[])
{
  int item, amt;
  int old_number;
  
  object_type *o_ptr;
  object_kind *k_ptr;
  
  char o_name[120];
  
  char out_val[160];
  
  cptr q, s;

  bool cursed = FALSE;
  
  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }

  /* Get an item */
  else
    {
      q = "Destroy which item? ";
      s = "You have nothing to destroy.";
      if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR | CAN_SQUELCH))) 
	return;
    }
  
  /* Deal with squelched items */
  if (item == ALL_SQUELCHED)
    {
      squelch_items();
      return;
    }
  
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
  
  /* Get the object kind. */
  k_ptr = &k_info[o_ptr->k_idx];
  
  /* Get a quantity */
  amt = get_quantity(NULL, o_ptr->number);
  
  /* Allow user abort */
  if (amt <= 0) return;

  /* Sticky carry curse stops destruction */
  if (o_ptr->flags_curse & CF_STICKY_CARRY) cursed = TRUE;
  
  /* Describe the object */
  old_number = o_ptr->number;
  o_ptr->number = amt;
  object_desc(o_name, o_ptr, TRUE, 3);
  o_ptr->number = old_number;
  
  /* Verify destruction */
  if (OPT(verify_destroy) && (OPT(verify_destroy_junk) || (object_value(o_ptr) >= 1)))
    {
      int result;
      
      /* Prompt */
      sprintf(out_val, "Really destroy %s? ", o_name);

      /* Give squelch as an option for aware objects */
      if (object_aware_p(o_ptr) &&
	  ((k_info[o_ptr->k_idx].tval < TV_SHOT) || 
	   (k_info[o_ptr->k_idx].tval > TV_DRAG_ARMOR)) &&
	  !(k_info[o_ptr->k_idx].flags_kind & (KF_INSTA_ART)))
	{

	  result = get_check_other(out_val, 's');
	  
	  /* returned "no" */
	  if (!result) return;
	  
	  /* return of 2 sets item to squelch */
	  else if (result == 2)
	    {
	      object_kind *k_ptr = &k_info[o_ptr->k_idx];
	      char o_name2[80];

	      /* make a fake object so we can give a proper message */
	      object_type *i_ptr;
	      object_type object_type_body;

	      /* Get local object */
	      i_ptr = &object_type_body;

	      /* Wipe the object */
	      object_wipe(i_ptr);

	      /* Create the object */
	      object_prep(i_ptr, o_ptr->k_idx);
	      
	      /*make it plural*/
	      i_ptr->number = 2;
	      
	      /*now describe with correct amount*/
	      object_desc(o_name2, i_ptr, FALSE, 0);
	      
	      /*set to squelch*/
	      k_ptr->squelch = TRUE;

	      /* Message - no good routine for extracting the plain name */
	      msg_format("All %^s will always be squelched.", o_name2);

	      /* Mark the view to be updated */
	      p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW);;
	    }
	}

      /* Unaware object, simple yes/no prompt */
      else if (!get_check(out_val)) return;
    }
  
  /* Artifacts cannot be destroyed */
  if (artifact_p(o_ptr))
    {
      int feel = FEEL_SPECIAL;
      
      /* Message */
      msg_format("You cannot destroy %s.", o_name);
      
      /* Hack -- inscribe the artifact, if not identified. */
      if (!object_known_p(o_ptr)) o_ptr->feel = feel;
      
      /* We have "felt" it (again) */
      o_ptr->ident |= (IDENT_SENSE);
      
      /* Combine the pack */
      p_ptr->notice |= (PN_COMBINE);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN | PW_EQUIP);
      
      /* Done */
      return;
    }
  
  /* Hack -- Cannot destroy some cursed items */
  if (o_ptr->flags_curse & CF_STICKY_CARRY)
    {
      /* Oops */
      msg_print("Hmmm, it seems to be cursed.");

      /* Notice */
      notice_curse(CF_STICKY_CARRY, item + 1);
      
      /* Nope */
      return;
    }
  
  /* Message */
  msg_format("You destroy %s.", o_name);
  
  /* Reduce the charges of rods/wands */
  reduce_charges(o_ptr, amt);
  
  /* Eliminate the item (from the pack) */
  if (item >= 0)
    {
      inven_item_increase(item, -amt);
      inven_item_describe(item);
      inven_item_optimize(item);
    }
  
  /* Eliminate the item (from the floor) */
  else
    {
      floor_item_increase(0 - item, -amt);
      floor_item_describe(0 - item);
      floor_item_optimize(0 - item);
    }
}


/**
 * Display specialized object information.  -LM-
 *
 * Unidentified:
 *	Weapons and armour -> description of specific object type (dagger, 
 *	  etc.).
 *	Others -> descrtiption only of general object kind (scroll, etc.)
 * Identified or aware:
 *	Artifacts -> artifact-specific description.
 *	Most objects -> description of specific object type.
 *	Scrolls, potions, spellbooks, rings, and amulets -> description	 
 *	  only of general object kind.
 * *Identified*:
 *	All -> description of object type or artifact description, 
 *	  complete listing of attributes and flags.
 *
 * Objects may also be members of a class with known effects.  If so, 
 * extra information about effects when used will appear if the effects 
 * can be quantified.
 *
 * Boolean value "in_store" only takes effect if player is in some store.
 */
void do_cmd_observe(void)
{
  int item;

  object_type *o_ptr;  

  cptr q, s;

  /* Do we have an item? */
  if (p_ptr->command_item) 
    {
      item = handle_item();
      if (!get_item_allow(item)) return;
    }
  
  /* Get an item */
  else
    {
      q = "Examine which item? ";
      s = "You have nothing to examine.";
      if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) 
	return;
    }
  
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
  
  /* Describe */
  object_info_screen(o_ptr, FALSE);

}



/**
 * Target command
 */
void do_cmd_target(void)
{
  /* Target set */
  if (target_set_interactive(TARGET_KILL))
    {
      msg_print("Target Selected.");
    }
  
  /* Target aborted */
  else
    {
      msg_print("Target Aborted.");
    }
}

/**
 * Target closest command
 */
void do_cmd_target_closest(void)
{
	target_set_closest(TARGET_KILL);
}



/**
 * Look command
 */
void do_cmd_look(void)
{
  /* Look around */
  if (target_set_interactive(TARGET_LOOK))
    {
      msg_print("Target Selected.");
    }
}



/**
 * Allow the player to examine other sectors on the map
 */
void do_cmd_locate(void)
{
  int dir, y1, x1, y2, x2;
  char tmp_val[80];
  char out_val[160];
  
  
  /* Start at current panel */
  y2 = y1 = panel_row_min;
  x2 = x1 = panel_col_min;
  
  /* Show panels until done */
  while (1)
    {
      /* Describe the location */
      if ((y2 == y1) && (x2 == x1))
	{
	  tmp_val[0] = '\0';
	}
      else
	{
	  if (small_screen)
	    sprintf(tmp_val, " %s%s of",
		    ((y2 < y1) ? "N" : (y2 > y1) ? "S" : ""),
		    ((x2 < x1) ? "W" : (x2 > x1) ? "E" : ""));
	  else
	    sprintf(tmp_val, "%s%s of",
		    ((y2 < y1) ? " North" : (y2 > y1) ? " South" : ""),
		    ((x2 < x1) ? " West" : (x2 > x1) ? " East" : ""));
	}
      
      /* Prepare to ask which way to look */
      if (small_screen)
	sprintf(out_val, "Sector [%d(%02d),%d(%02d)], %s yours.  Dir or ESC?",
	      y2 / (SCREEN_HGT / 2), y2 % (SCREEN_HGT / 2),
	      x2 / (SCREEN_WID / 2), x2 % (SCREEN_WID / 2), tmp_val);

      else
	sprintf(out_val, "Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction or ESC?",
		y2 / (SCREEN_HGT / 2), y2 % (SCREEN_HGT / 2),
		x2 / (SCREEN_WID / 2), x2 % (SCREEN_WID / 2), tmp_val);

      /* Assume no direction */
      dir = 0;
      
      /* Get a direction */
      while (!dir)
	{
	  ui_event_data ke = EVENT_EMPTY;
	  
	  /* Get a command (or Cancel) */
	  if (!get_com_ex(out_val, &ke)) break;
	  
	  /* Extract the action (if any) */
	  if (ke.key == '\xff')
	    {
	      if (!ke.mousey) break;
	      dir = mouse_dir(ke, TRUE);
	    }
	  else
	    dir = target_dir(ke.key);
	  
	  /* Error */
	  if (!dir) bell(NULL);
	}
      
      /* No direction */
      if (!dir) break;
      
      /* Apply the motion */
      if (change_panel(ddy[dir], ddx[dir]))
	{
	  y2 = panel_row_min;
	  x2 = panel_col_min;
	}
    }
  
  
  /* Recenter the map around the player */
  verify_panel();
  
  /* Update stuff */
  p_ptr->update |= (PU_MONSTERS);
  
  /* Redraw map */
  p_ptr->redraw |= (PR_MAP);
  
  /* Window stuff */
  p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
  
  /* Handle stuff */
  handle_stuff();
}



/**
 * The table of "symbol info" -- each entry is a string of the form
 * "X:desc" where "X" is the trigger, and "desc" is the "info".
 */
static cptr ident_info[] =
{
  " :A dark grid",
  "!:A potion (or oil)",
  "\":An amulet (or necklace)",
  "#:A wall (or secret door)",
  "$:Treasure (gold or gems)",
  "%:A vein (magma or quartz)",
  /* "&:unused", */
  "':An open door",
  "(:Soft armor",
  "):A shield",
  "*:A vein with treasure",
  "+:A closed door",
  ",:Food (or mushroom patch)",
  "-:A wand (or rod)",
  ".:Floor",
  "/:A polearm (Axe/Pike/etc)",
  /* "0:unused", */
  "1:Entrance to General Store",
  "2:Entrance to Armory",
  "3:Entrance to Weaponsmith",
  "4:Entrance to Temple",
  "5:Entrance to Alchemy shop",
  "6:Entrance to Magic store",
  "7:Entrance to Black Market",
  "8:Entrance to your home",
  "9:Entrance to Bookseller",
  "::Rubble",
  ";:A glyph of warding",
  "<:An up staircase",
  "=:A ring",
  ">:A down staircase",
  "?:A scroll",
  "@:You",
  /* "A:unused", */
  "B:Bird",
  "C:Canine",
  "D:Flying Dragon",
  "E:Elemental",
  "F:Dragon Fly",
  "G:Ghost",
  "H:Hybrid",
  "I:Insect",
  "J:Snake",
  "K:Killer Beetle",
  "L:Lich",
  "M:Mummy",
  /* "N:unused", */
  "O:Ogre",
  "P:Giant Humanoid",
  "Q:Quylthulg (Pulsing Flesh Mound)",
  "R:Reptile/Amphibian",
  "S:Spider/Scorpion/Tick",
  "T:Troll",
  "U:Major Demon",
  "V:Vampire",
  "W:Wight/Wraith/etc",
  "X:Xorn/Xaren/etc",
  "Y:Yeti",
  "Z:Zephyr Hound",
  "[:Hard armor",
  "\\:A hafted weapon (mace/whip/etc)",
  "]:Misc. armor",
  "^:A trap",
  "_:A staff",
  "`:A tool or junk",
  "a:Ant",
  "b:Bat",
  "c:Centipede",
  "d:Dragon",
  "e:Floating Eye",
  "f:Feline",
  "g:Golem",
  "h:Hobbit/Elf/Dwarf",
  "i:Icky Thing",
  "j:Jelly",
  "k:Kobold",
  "l:Louse",
  "m:Mold",
  "n:Naga",
  "o:Orc",
  "p:Person/Human",
  "q:Quadruped",
  "r:Rodent",
  "s:Skeleton",
  "t:Townsperson",
  "u:Minor Demon",
  "v:Vortex",
  "w:Worm/Worm-Mass",
  /* "x:unused", */
  "y:Yeek",
  "z:Zombie/Mummy",
  "{:A missile (arrow/bolt/shot)",
  "|:An edged weapon (sword/dagger/etc)",
  "}:A launcher (bow/crossbow/sling)",
  "~:A chest or light",
  NULL
};



/**
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
bool ang_sort_comp_hook(void *u, void *v, int a, int b)
{
  u16b *who = (u16b*)(u);
  
  u16b *why = (u16b*)(v);
  
  int w1 = who[a];
  int w2 = who[b];
  
  int z1, z2;
  
  
  /* Sort by player kills */
  if (*why >= 4)
    {
      /* Extract player kills */
      z1 = l_list[w1].pkills;
      z2 = l_list[w2].pkills;
      
      /* Compare player kills */
      if (z1 < z2) return (TRUE);
      if (z1 > z2) return (FALSE);
    }
  
  
  /* Sort by total kills */
  if (*why >= 3)
    {
      /* Extract total kills */
      z1 = l_list[w1].tkills;
      z2 = l_list[w2].tkills;
      
      /* Compare total kills */
      if (z1 < z2) return (TRUE);
      if (z1 > z2) return (FALSE);
    }
  
  
  /* Sort by monster level */
  if (*why >= 2)
    {
      /* Extract levels */
      z1 = r_info[w1].level;
      z2 = r_info[w2].level;
      
      /* Compare levels */
      if (z1 < z2) return (TRUE);
      if (z1 > z2) return (FALSE);
    }
  
  
  /* Sort by monster experience */
  if (*why >= 1)
    {
      /* Extract experience */
      z1 = r_info[w1].mexp;
      z2 = r_info[w2].mexp;

      /* Compare experience */
      if (z1 < z2) return (TRUE);
      if (z1 > z2) return (FALSE);
    }
  
  
  /* Compare indexes */
  return (w1 <= w2);
}


/**
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
void ang_sort_swap_hook(void *u, void *v, int a, int b)
{
  u16b *who = (u16b*)(u);
  
  u16b holder;
  
  /* Swap */
  holder = who[a];
  who[a] = who[b];
  who[b] = holder;
}



/**
 * Identify a character, allow recall of monsters
 *
 * Several "special" responses recall "mulitple" monsters:
 *   ^A (all monsters)
 *   ^U (all unique monsters)
 *   ^N (all non-unique monsters)
 *
 * The responses may be sorted in several ways, see below.
 *
 */
void do_cmd_query_symbol(void)
{
  int i, j, n, r_idx;
  int start = 0, last_level = 0;
  char sym;
  ui_event_data query = EVENT_EMPTY;
  char search_str[60] = "";
  char monster_name[80];
  char buf[128];
  char *sp;
  
  bool all = FALSE;
  bool uniq = FALSE;
  bool norm = FALSE;
  
  bool recall = FALSE;
  
  u16b why = 0;
  u16b *who = malloc(z_info->r_max * sizeof(*who));
  
  /* Get a character, or abort */
  if (!get_com("Enter character to be identified: ", &sym)) 
    {
      free(who);
      return;
    }
  
  /* Find that character info, and describe it */
  for (i = 0; ident_info[i]; ++i)
    {
      if (sym == ident_info[i][0]) break;
    }
  
  /* Describe */
  if (sym == KTRL('A'))
    {
      all = TRUE;
      strcpy(buf, "Full monster list.");
    }
  else if (sym == KTRL('U'))
    {
      all = uniq = TRUE;
      strcpy(buf, "Unique monster list.");
    }
  else if (sym == KTRL('N'))
    {
      all = norm = TRUE;
      strcpy(buf, "Non-unique monster list.");
    }
  else if (sym == KTRL('F'))
    {
      if (!get_string("Substring to search: ", search_str, sizeof(search_str)))
	{
	  free(who);
	  return;
	}
      
      for(sp = search_str; *sp; sp++) *sp = tolower(*sp);
      
      sprintf(buf, "Monsters matching '%s'", search_str);
      all = FALSE;
    }
  else if (ident_info[i])
    {
      sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
    }
  else
    {
      sprintf(buf, "%c - %s.", sym, "Unknown Symbol");
    }
  
  /* Display the result */
  prt(buf, 0, 0);
  
  
  /* Collect matching monsters */
  for (n = 0, i = 1; i < z_info->r_max; i++)
    {
      monster_race *r_ptr = &r_info[i];
      monster_lore *l_ptr = &l_list[i];
      
      /* Nothing to recall 
       * Even cheat_know does not allow viewing non-existant
       * player ghosts.
       */
      if ((!OPT(cheat_know) || (r_ptr->flags2 & (RF2_PLAYER_GHOST))) 
	  && !l_ptr->sights) 
	continue;
      
      /* Require non-unique monsters if needed */
      if (norm && (r_ptr->flags1 & (RF1_UNIQUE))) continue;
      
      /* Require unique monsters if needed */
      if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE))) continue;
      
      /* Collect "appropriate" monsters */
      if (*search_str)
	{
	  strncpy(monster_name, r_name + r_ptr->name, sizeof monster_name);
	  for(sp = monster_name; *sp; sp++) *sp = tolower(*sp);
	  if (strstr(monster_name, search_str)) who[n++] = i;
	  continue;
	}
      if (all || (r_ptr->d_char == sym)) who[n++] = i;
    }
  
  /* Nothing to recall */
  if (!n) 
    {
      free(who);
      return;
    }
  
  
  /* Prompt */
  put_str("Recall details? (k/p/y/n): ", 0, 40);
  backup_buttons();
  kill_all_buttons();
  button_add("ESC", ESCAPE);
  button_add("k", 'k');
  button_add("p", 'p');
  button_add("y", 'y');
  button_add("n", 'n');
  update_statusline();
  
  /* Query */
  query = inkey_ex();
  
  /* Restore */
  prt(buf, 0, 0);
  

  /* Sort by kills (and level) */
  if (query.key == 'k')
    {
      why = 4;
      query.key = 'y';
    }
  
  /* Sort by level */
  if (query.key == 'p')
    {
      why = 2;
      query.key = 'y';
    }
  
  /* Catch "escape" */
  if (query.key != 'y') 
    {
      kill_all_buttons();
      restore_buttons();
      update_statusline();
      free(who);
      return;
    }
  else
    {
      button_kill('k');
      button_kill('p');
      button_kill('y');
      button_add("r", 'r');
    }
  
  /* Sort if needed */
  if (why)
    {
      /* Select the sort method */
      ang_sort_comp = ang_sort_comp_hook;
      ang_sort_swap = ang_sort_swap_hook;
      
      /* Sort the array */
      ang_sort(who, &why, n);
    }
  
  
  /* Start at the current level */
  i = 0;
  
  /* 
   * Find the monster whose level is the closest to the 
   * current depth, without being greater than it.
   */
  for (j = 0; j < n; j++)
    {
      monster_race *r_ptr = &r_info[who[j]];
      
      if ((r_ptr->level <= p_ptr->depth) && (r_ptr->level >= last_level))
	{
	  start = j;
	  last_level = r_ptr->level;
	}
    }
  
  /* Start at the chosen monster. */
  i = start;
  
  /* Scan the monster memory */
  while (TRUE)
    {
      /* Extract a race */
      r_idx = who[i];
      
      /* Hack -- Auto-recall */
      monster_race_track(r_idx);
      
      /* Hack -- Handle stuff */
      handle_stuff();
      
      /* Hack -- Begin the prompt */
      roff_top(r_idx);
      
      /* Hack -- Complete the prompt */
      Term_addstr(-1, TERM_WHITE, "[(r)ecall, ESC]");
      prt(format("(#%d of %d)", i + 1, n), 0 , 65);
      
      /* Interact */
      while (TRUE)
	{
	  /* Recall */
	  if (recall)
	    {
	      /* Save screen */
	      screen_save();
	      
	      /* Recall on screen */
	      screen_roff(who[i]);
	      
	      /* Hack -- Complete the prompt (again) */
	      Term_addstr(-1, TERM_WHITE, "[(r)ecall, ESC]");
	      prt(format("(#%d of %d)", i + 1, n), 0 , 65);
	      
	    }

	  button_add("-", '-');
	  update_statusline();
	  
	  /* Command */
	  query = inkey_ex();
	  
	  /* Unrecall */
	  if (recall)
	    {
	      /* Load screen */
	      screen_load();
	    }
	  
	  /* Normal commands */
	  if (query.key != 'r') break;
	  
	  /* Toggle recall */
	  recall = !recall;
	}
      
      /* Stop scanning */
      if (query.key == ESCAPE) break;
      
      /* Move to "prev" monster */
      if (query.key == '-')
	{
	  if (i-- == 0)
	    {
	      i = n - 1;
	    }
	}
      
      /* Move to "next" monster */
      else
	{
	  if (++i == n)
	    {
	      i = 0;
	    }
	}
    }
  
  
  /* Re-display the identity */
  prt(buf, 0, 0);

  kill_all_buttons();
  restore_buttons();
  update_statusline();
  free(who);
}



/**
 * Hack -- possible victim outcry. -LM- 
 */
static cptr desc_victim_outcry[] =
{
  "'My money, where's my money?'",
  "'Thief! Thief! Thief! Baggins! We hates it forever!'",
  "'Tell me, have you seen a purse wandering around?'",
  "'Thieves, Fire, Murder!'",
  "''Ere, 'oo are you?'",
  "'Hey, look what I've copped!'",
  "'How dare you!'",
  "'Help yourself again, thief, there is plenty and to spare!'",
  "'All the world detests a thief.'",
  "'Catch me this thief!'",
  "'Hi! Ho! Robbery!'",
  "'My gold, my precious gold!'",
  "'My gold is costly, thief!'",
  "'Your blood for my gold?  Agreed!'",
  "'I scrimp, I save, and now it's gone!'",
  "'Robbers like you are part of the problem!'",
  "'Banditti!  This place is just not safe anymore!'",
  "'Ruined!  I'm ruined!'",
  "'Where, where is the common decency?'",
  "'Your knavish tricks end here and now!'",
};



/**
 * Rogues may steal gold from monsters.  The monster needs to have 
 * something to steal (it must drop some form of loot), and should 
 * preferably be asleep.  Humanoids and dragons are a rogue's favorite
 * targets.  Steal too often on a level, and monsters will be more wary, 
 * and the hue and cry will be eventually be raised.  Having every 
 * monster on the level awake and aggravated is not pleasant. -LM-
 */
void py_steal(int y, int x)
{
  cptr act = NULL;
  
  monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
  monster_race *r_ptr = &r_info[m_ptr->r_idx];
  
  char m_name[80];
  
  int i;
  int effect, theft_protection;
  int filching_power = 0;
  int purse = 0;

  bool thief = FALSE;
  bool success = FALSE;

  /* Check intent */
  if ((m_ptr->hostile != -1) && 
      !get_check("Do you want to steal from this being?"))
    {
      py_attack(y, x, FALSE);
      return;
    }
  
  /* Hard limit on theft. */
  if (number_of_thefts_on_level > 4)
    {
      msg_print("Everyone is keeping a lookout for you.  You can steal nothing here.");
      return;
    }
  
  /* Determine the cunning of the thief. */
  filching_power = 2 * p_ptr->lev;
  
  /* Penalize some conditions */
  if (p_ptr->blind || no_lite()) filching_power = filching_power / 10;
  if (p_ptr->confused || p_ptr->image) filching_power = filching_power / 10;
  
  /* Determine how much protection the monster has. */
  theft_protection = (7 * (r_ptr->level + 2) / 4);
  theft_protection += (m_ptr->mspeed - p_ptr->pspeed);
  if (theft_protection < 1) theft_protection = 1;
  
  /* Send a thief to catch a thief. */
  for (i = 0; i < 4; i++)
    {
      /* Extract infomation about the blow effect */
      effect = r_ptr->blow[i].effect;
      if (effect == RBE_EAT_GOLD) thief = TRUE;
      if (effect == RBE_EAT_ITEM) thief = TRUE;
    }
  if (thief) theft_protection += 30;
  
  if (m_ptr->csleep) theft_protection = 3 * theft_protection / 5;
  
  /* Special player stealth magics aid stealing, but are lost in the process */
  if (p_ptr->superstealth)
    {
      theft_protection = 3 * theft_protection / 5;
      set_superstealth(0,TRUE);
    }

  /* The more you steal on a level, the more wary the monsters. */
  theft_protection += number_of_thefts_on_level * 15;
  
  /* Did the theft succeed?  */
  if (randint1(theft_protection) < filching_power) success = TRUE;
  
  
  /* If the theft succeeded, determine the value of the purse. */
  if (success)
    {
      purse = (r_ptr->level + 1) + randint1(3 * (r_ptr->level + 1) / 2);
      
      /* Uniques are juicy targets. */
      if (r_ptr->flags1 & (RF1_UNIQUE)) purse *= 3;
      
      /* But some monsters are dirt poor. */
      if (!((r_ptr->flags1 & (RF1_DROP_60)) || 
	    (r_ptr->flags1 & (RF1_DROP_90)) || 
	    (r_ptr->flags1 & (RF1_DROP_1D2)) || 
	    (r_ptr->flags1 & (RF1_DROP_2D2)) || 
	    (r_ptr->flags1 & (RF1_DROP_3D2)) || 
	    (r_ptr->flags1 & (RF1_DROP_4D2)))) purse = 0;
      
      /* Some monster races are far better to steal from than others. */
      if ((r_ptr->d_char == 'D') || (r_ptr->d_char == 'd') || 
	  (r_ptr->d_char == 'p') || (r_ptr->d_char == 'h')) 
	purse *= 2 + randint1(3) + randint1(r_ptr->level / 20);
      else if ((r_ptr->d_char == 'P') || (r_ptr->d_char == 'o') || 
	       (r_ptr->d_char == 'O') || (r_ptr->d_char == 'T') ||
	       (r_ptr->d_char == 'n') || (r_ptr->d_char == 'W') ||
	       (r_ptr->d_char == 'k') || (r_ptr->d_char == 'L') ||
	       (r_ptr->d_char == 'V') || (r_ptr->d_char == 'y')) 
	purse *= 1 + randint1(3) + randint1(r_ptr->level / 30);
      
      /* Pickings are scarce in a land of many thieves. */
      purse = purse * (p_ptr->depth + 5) / (p_ptr->recall[0] + 5);
      
      /* Increase player gold. */
      p_ptr->au += purse;
      
      /* Limit to avoid buffer overflow */
      if (p_ptr->au > PY_MAX_GOLD) p_ptr->au = PY_MAX_GOLD;
	    
      /* Redraw gold */
      p_ptr->redraw |= (PR_GOLD);
      
      /* Announce the good news. */
      if (purse) msg_format("You burgle %d gold.", purse);
      
      /* Pockets are empty. */
      else msg_print("You burgle only dust.");
    }
  
  /* The victim normally, but not always, wakes up and is aggravated. */
  if (randint1(4) != 1)
    {
      m_ptr->csleep = 0;
      m_ptr->mflag |= (MFLAG_ACTV);
      if (m_ptr->mspeed < r_ptr->speed + 3) m_ptr->mspeed += 10;

      /* Become hostile */
      m_ptr->hostile = -1;      
      
      /* Occasionally, amuse the player with a message. */
      if ((randint1(5) == 1) && (purse) && (r_ptr->flags2 & (RF2_SMART)))
	{
	  monster_desc(m_name, m_ptr, 0);
	  act = desc_victim_outcry[randint0(20)];
	  msg_format("%^s cries out %s", m_name, act);
	}
      /* Otherwise, simply explain what happened. */
      else 
	{
	  monster_desc(m_name, m_ptr, 0);
	  msg_format("You have aroused %s.", m_name);
	}
    }
  
  /* The thief also speeds up, but only for just long enough to escape. */
  if (!p_ptr->fast) p_ptr->fast += 2;
  
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Handle stuff */
  handle_stuff();
  
  
  /* Increment the number of thefts, and possibly raise the hue and cry. */
  number_of_thefts_on_level++;
  
  if (number_of_thefts_on_level > 4)
    {
      /* Notify the player of the trouble he's in. */
      msg_print("All the level is in an uproar over your misdeeds!");
      
      /* Aggravate and speed up all monsters on level. */
      (void) aggravate_monsters(1, TRUE);
    }
  
  else if ((number_of_thefts_on_level > 2) || (randint1(8) == 1))
    {
      msg_print("You hear hunting parties scouring the area for a notorious burgler.");
      
      /* Aggravate monsters nearby. */
      (void) aggravate_monsters(1, FALSE);
    }
  
  /* Rogue "Hit and Run" attack. */
  if (p_ptr->special_attack & (ATTACK_FLEE))
    {
      /* Cancel the fleeing spell */
      p_ptr->special_attack &= ~(ATTACK_FLEE);
      
      /* Message */
      msg_print("You escape into the shadows!");
      
      /* Teleport. */
      teleport_player(6 + p_ptr->lev / 5, TRUE);
      
      /* Redraw the state */
      p_ptr->redraw |= (PR_STATUS);
    }
}


/**
 * Rogues may set traps.  Only one such trap may exist at any one time, 
 * but an old trap can be disarmed to free up equipment for a new trap.
 * -LM-
 */
bool py_set_trap(int y, int x)
{
  int max_traps;
  s16b this_o_idx, next_o_idx = 0;
  object_type *o_ptr;
  bool destroy_message = FALSE;
	  
  max_traps = 1 + ((p_ptr->lev >= 25) ? 1 : 0) + 
    (check_ability(SP_EXTRA_TRAP) ? 1 : 0);
  
  if (p_ptr->blind || no_lite())
    {
      msg_print("You can not see to set a trap.");
      return FALSE;
    }
  
  if (p_ptr->confused || p_ptr->image)
    {
      msg_print("You are too confused.");
      return FALSE;
    }
  
  /* Paranoia -- Forbid more than max_traps being set. */
  if (num_trap_on_level >= max_traps)
    {
      msg_print("You must disarm your existing trap to free up your equipment.");
      return FALSE;
    }
  
  /* No setting traps while shapeshifted */
  if (SCHANGE)
    {
      msg_print("You can not set traps while shapechanged.");
      msg_print("Use the ']' command to return to your normal form.");
      return FALSE;
    }
  
  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
    {
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Artifact */
      if (o_ptr->name1)
	{
	  msg_print("There is an indestructible object here.");
	  return FALSE;
	}

      /* Visible object to be destroyed */
      if (!squelch_hide_item(o_ptr)) destroy_message = TRUE;
    }

  /* Verify */
  if (cave_o_idx[y][x]) 
    {
      if (destroy_message)
	if (!get_check("Destroy all items and set a trap?")) return FALSE;

      for (this_o_idx = cave_o_idx[y][x]; this_o_idx; this_o_idx = next_o_idx)
	{
	  /* Acquire object */
	  o_ptr = &o_list[this_o_idx];
	  
	  /* Acquire next object */
	  next_o_idx = o_ptr->next_o_idx;
	  
	  /* Delete the object */
	  delete_object_idx(this_o_idx);
	}
      
      /* Redraw */
      lite_spot(y, x);
    }
    
  /* Set the trap, and draw it. */
  cave_set_feat(y, x, FEAT_MTRAP_BASE);
  
  /* Notify the player. */
  msg_print("You set a monster trap.");
  
  /* Increment the number of monster traps. */
  num_trap_on_level++;

  /* A trap has been set */
  return TRUE;
}

/**
 * Trap coordinates 
 */
static int trap_y = 0;
static int trap_x = 0;

static char *trap_type[] = 
{
  "Sturdy Trap      (less likely to break)",
  "Netted Trap      (effective versus flyers)",
  "Confusion Trap   (confuses monsters)",
  "Poison Gas Trap  (creates a toxic cloud)",
  "Spirit Trap      (effective versus spirits)",
  "Lightning Trap   (shoots a lightning bolt)",
  "Explosive Trap   (causes area damage)",
  "Portal Trap      (teleports monsters)",
  "Stasis Trap      (freezes time for a monster)",
  "Drain Life Trap  (hurts living monsters)",
  "Unmagic Trap     (damages and reduces mana)",
  "Dispelling Trap  (hurts all monsters in sight)",
  "Genocide Trap    (removes nearby like monsters)" 
};

char trap_tag(menu_type *menu, int oid)
{
  return I2A(oid);
}

/**
 * Display an entry on the sval menu
 */
void trap_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  const u16b *choice = menu->menu_data;
  int idx = choice[oid];

  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  
  /* Print it */
  c_put_str(attr, format("%s", trap_type[idx]), row, col);
}

/**
 * Deal with events on the trap menu
 */
bool trap_action(menu_type *menu, const ui_event_data *db, int oid)
{
  u16b *choice = db;
  
  int idx = choice[oid];
  cave_set_feat(trap_y, trap_x, FEAT_MTRAP_BASE + 1 + idx);
  
  return TRUE;
}


/**
 * Display list of monster traps.
 */
bool trap_menu(void)
{
  menu_type menu;
  menu_iter menu_f = { trap_tag, 0, trap_display, trap_action, 0 };
  region area = { (small_screen ? 0 : 15), 1, 48, -1 };
  ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;
  
  int num = 0;
  size_t i;
  
  u16b *choice;
  
  /* See how many traps available */
  if (check_ability(SP_EXTRA_TRAP)) num = 1 + (p_ptr->lev / 4);
  else num = 1 + (p_ptr->lev / 6);
  
  /* Create the array */
  choice = C_ZNEW(num, u16b);
  
  /* Obvious */
  for (i = 0; i < num; i++)
    {
      choice[i] = i;
    }

  /* Clear space */
  area.page_rows = num + 2;
  
  /* Return here if there are no traps */
  if (!num)
    {
      FREE(choice);
      return FALSE;
    }
  
  
  /* Save the screen and clear it */
  screen_save();
  
  /* Help text */
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.title = "Choose an advanced monster trap (ESC to cancel):";
  menu.cmd_keys = " \n\r";
  menu.count = num;
  menu.menu_data = choice;
  menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
  
  /* Select an entry */
  evt = menu_select(&menu, 0);
  
  /* Free memory */
  FREE(choice);
  
  /* Load screen */
  screen_load();
  return (evt.type != EVT_ESCAPE);
}


/** 
 * Turn a basic monster trap into an advanced one -BR-
 */
bool py_modify_trap(int y, int x)
{
  if (p_ptr->blind || no_lite())
    {
      msg_print("You can not see to modify your trap.");
      return FALSE;
    }
  
  if (p_ptr->confused || p_ptr->image)
    {
      msg_print("You are too confused.");
      return FALSE;
    }
  
  /* No setting traps while shapeshifted */
  if (SCHANGE)
    {
      msg_print("You can not set traps while shapechanged.");
      msg_print("Use the ']' command to return to your normal form.");
      return FALSE;
    }
  
  trap_y = y;
  trap_x = x;

  /* get choice */
  if (trap_menu())
    {
      /* Notify the player. */
      msg_print("You modify the monster trap.");
    }

  /* Trap was modified */
  return TRUE;
}

