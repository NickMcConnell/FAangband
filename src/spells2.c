/** \file spells2.c 
    \brief Spells

 * Healing spells, glyphs of warding, reducing or sustaining a stat, ID 
 * everything, chance for enchant spells to fail, remove curses, regain 
 * exp, detection spells, create stairs.  Definitions of armour & weapons, 
 * enchantment, branding, temporary branding, cursing, and ID code, what 
 * items are rechargable and the recharging code.  Various special object 
 * spells.  Spells that effect an area or LOS, lighten & darken rooms and 
 * areas, casting ball, projection, beam, and bolt spells.  Some miscel-
 * lanious non-damage spell functions.
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

/* Element to be proofed against in element-proofing */
static byte el_to_proof = 0;

/*
 * Increase players hit points, notice effects
 */
bool hp_player(int num)
{
  /* Healing needed */
  if (p_ptr->chp < p_ptr->mhp)
    {
      /* Gain hitpoints */
      p_ptr->chp += num;
      
      /* Enforce maximum */
      if (p_ptr->chp >= p_ptr->mhp)
	{
	  p_ptr->chp = p_ptr->mhp;
	  p_ptr->chp_frac = 0;
	}
      
      /* Redraw */
      p_ptr->redraw |= (PR_HP);
      
      /* Window stuff */
      p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
      
      /* Heal 0-4 */
      if (num < 5)
	{
	  msg_print("You feel a little better.");
	}
      
      /* Heal 5-14 */
      else if (num < 15)
	{
	  msg_print("You feel better.");
	}
      
      /* Heal 15-34 */
      else if (num < 35)
	{
	  msg_print("You feel much better.");
	}
      
      /* Heal 35+ */
      else
	{
	  msg_print("You feel very good.");
	}
      
      /* Notice */
      return (TRUE);
    }
  
  /* Ignore */
  return (FALSE);
}

/**
 * Jam a closed door with a magical spike.  
 * Code is taken from do_cmd_spike. -LM-
 */
void magic_spiking(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int y, x, i, dir;
  
  
  /* Get a direction (or abort) */
  if (!get_rep_dir(&dir)) return;
  
  /* Get location */
  y = py + ddy[dir];
  x = px + ddx[dir];
  
  
  /* Verify legality */
  if (!do_cmd_spike_test(y, x)) return;
  
  /* Monster */
  if (cave_m_idx[y][x] > 0)
    {
      /* Message */
      msg_print("There is a monster in the way!");
      
      /* Attack */
      if (py_attack(y, x, TRUE)) return;
    }
  
  /* Go for it */
  else
    {
      /* Verify legality */
      if (!do_cmd_spike_test(y, x)) return;
      
      /* Successful jamming */
      msg_print("You magically jam the door.");
      
      /* Convert "locked" to "stuck" XXX XXX XXX */
      if (cave_feat[y][x] < FEAT_DOOR_HEAD + 0x08)
	{
	  cave_feat[y][x] += 0x08;
	}
      
      /* Add three magical spikes to the door. */
      for (i = 0; i < 3; i++)
	{
	  if (cave_feat[y][x] < FEAT_DOOR_TAIL)
	    {
	      cave_feat[y][x] += 0x01;
	    }
	}
    }
}

/** Maximum numbers of runes of the various types */
int max_runes[] =  
  {
    4,   /* Rune of the Elements */
    4,   /* Rune of Magic Defence */
    4,   /* Rune of Instability */
    1,   /* Rune of Mana */
    4,   /* Rune of Protection */
    1,   /* Rune of Power */
    1    /* Rune of Speed */
  };


/**
 * Leave a rune 
 */
bool lay_rune(int type)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  s16b this_o_idx, next_o_idx = 0;
  object_type *o_ptr;

  /* If we're standing on a rune of mana, we can add mana to it */
  if ((type == RUNE_MANA) && (cave_feat[py][px] == FEAT_RUNE_MANA))
    {
      /* Standard mana amount */
      int mana = 40;

      /* Already full? */
      if (mana_reserve >= MAX_MANA_RESERVE)
	{
	  msg_print("The rune cannot hold more mana");
	  return (FALSE);
	}

      /* Don't put in more than we have */
      if (p_ptr->csp < mana) mana = p_ptr->csp;

      /* Don't put in more than it will hold */
      mana_reserve += mana; 
      if (mana_reserve > MAX_MANA_RESERVE) mana_reserve = MAX_MANA_RESERVE;
      return (TRUE);
    }

  /* XXX XXX XXX */
  if (!cave_trappable_bold(py, px))
    {
      msg_print("You cannot lay a rune here.");
      return (FALSE);
    }
  
  /* Scan all objects in the grid */
  for (this_o_idx = cave_o_idx[py][px]; this_o_idx; this_o_idx = next_o_idx)
    {
      /* Acquire object */
      o_ptr = &o_list[this_o_idx];
      
      /* Acquire next object */
      next_o_idx = o_ptr->next_o_idx;
      
      /* Artifact */
      if (o_ptr->name1)
	{
	  msg_print("There is an indestructible object here.");
	  return (FALSE);
	}
    }
  
  /* Verify */
  if (cave_o_idx[py][px]) 
    {
      if (!get_check("Destroy all items and lay a rune?")) return (FALSE);
      else
	{
	  for (this_o_idx = cave_o_idx[py][px]; this_o_idx; this_o_idx = next_o_idx)
	    {
	      /* Acquire object */
	      o_ptr = &o_list[this_o_idx];
	      
	      /* Acquire next object */
	      next_o_idx = o_ptr->next_o_idx;
	      
	      /* Delete the object */
	      delete_object_idx(this_o_idx);
	    }
	  
	  /* Redraw */
	  lite_spot(py, px);
	}
    }
  
  /* Limit total number of runes. */
  if (num_runes_on_level[type] >= max_runes[type])
    {
      msg_print("You have reached the maximum number of runes of this type.");
      return (FALSE);
    }
  
  /* Create a rune */
  cave_set_feat(py, px, FEAT_RUNE_HEAD + type);
  
  /* Increment the rune count. */
  num_runes_on_level[type]++;
  
  /* Warning. */
  if (num_runes_on_level[type] == max_runes[type])
    {
      msg_print("You have now reached your limit for runes of this type.");  
      msg_print("In order to set more, remove some.");
    }
  
  return (TRUE);
}




/**
 * Array of stat "descriptions"
 */
static cptr desc_stat_pos[] =
{
  "strong",
  "smart",
  "wise",
  "dextrous",
  "healthy",
  "cute"
};


/**
 * Array of stat "descriptions"
 */
static cptr desc_stat_neg[] =
{
  "weak",
  "stupid",
  "naive",
  "clumsy",
  "sickly",
  "ugly"
};


/**
 * Lose a "point"
 */
bool do_dec_stat(int stat)
{
  bool sust = FALSE;
  bool clarity = (check_ability(SP_CLARITY));
  bool athletics = (check_ability(SP_ATHLETICS));
  
  /* Access the "sustain" and specialty skills */
  switch (stat)
    {
    case A_STR: if (p_ptr->sustain_str)
      sust = TRUE; break;
    case A_INT: if ((p_ptr->sustain_int) || 
		    (clarity && (rand_int(2) != 0)))
      sust = TRUE; break;
    case A_WIS: if ((p_ptr->sustain_wis) || 
		    (clarity && (rand_int(2) != 0)))
      sust = TRUE; break;
    case A_DEX: if ((p_ptr->sustain_dex) || 
		    (athletics && (rand_int(2) != 0)))
      sust = TRUE; break;
    case A_CON: if ((p_ptr->sustain_con) || 
		    (athletics && (rand_int(2) != 0)))
      sust = TRUE; break;
    case A_CHR: if (p_ptr->sustain_chr)
      sust = TRUE; break;}
  
  
  /* Sustain */
  if (sust)
    {
      /* Message */
      msg_format("You feel very %s for a moment, but the feeling passes.",
		 desc_stat_neg[stat]);
      
      /* Notice effect */
      notice_obj((OBJECT_RAND_BASE_SUSTAIN << stat), 0);
      return (TRUE);
    }
  
  /* Attempt to reduce the stat */
  if (dec_stat(stat, 10, FALSE))
    {
      /* Message */
      msg_format("You feel very %s.", desc_stat_neg[stat]);
      
      /* Notice effect */
      return (TRUE);
    }
  
  /* Nothing obvious */
  return (FALSE);
}


/**
 * Restore lost "points" in a stat
 */
bool do_res_stat(int stat)
{
  /* Attempt to increase */
  if (res_stat(stat))
    {
      /* Message */
      msg_format("You feel less %s.", desc_stat_neg[stat]);
      
      /* Notice */
      return (TRUE);
    }
  
  /* Nothing obvious */
  return (FALSE);
}


/**
 * Gain a "point" in a stat
 */
bool do_inc_stat(int stat, bool star)
{
  bool res;
  
  /* Restore stst */
  res = res_stat(stat);
  
  /* Attempt to increase */
  if (inc_stat(stat, star))
    {
      /* Message */
      msg_format("You feel very %s!", desc_stat_pos[stat]);
      
      /* Notice */
      return (TRUE);
    }
  
  /* Restoration worked */
  if (res)
    {
      /* Message */
      msg_format("You feel less %s.", desc_stat_neg[stat]);
      
      /* Notice */
      return (TRUE);
    }
  
  /* Nothing obvious */
  return (FALSE);
}

void identify_object(object_type *o_ptr)
{
  object_kind *k_ptr;
  bool was_dubious = FALSE;
  bool aware = FALSE;

  /* Get the object kind. */
  k_ptr = &k_info[o_ptr->k_idx];

  /* See what we thougth of it before */
  if ((o_ptr->feel == FEEL_PERILOUS) ||
      (o_ptr->feel == FEEL_DUBIOUS_WEAK) ||
      (o_ptr->feel == FEEL_DUBIOUS_STRONG))
    was_dubious = TRUE;
  
  /* Remember awareness */
  if (object_aware_p(o_ptr)) aware = TRUE;

  /* Identify it fully */
  object_aware(o_ptr);
  object_known(o_ptr);

  /* Now we know about any ego-item type */
  if (o_ptr->name2) e_info[o_ptr->name2].everseen = TRUE;

  /* Get sensation ID (id_other is already done by the notice_other call) */
  if ((o_ptr == &inventory[INVEN_LEFT]) || 
      (o_ptr == &inventory[INVEN_RIGHT]) ||
      (o_ptr == &inventory[INVEN_NECK]))
    p_ptr->id_obj |= o_ptr->id_obj;
  
  /* Check for known curses */
  if ((o_ptr->flags_obj & OF_SHOW_CURSE) ||
      (artifact_p(o_ptr) && (o_ptr->name1 < ART_MIN_RANDOM)))
    {
      o_ptr->id_curse = o_ptr->flags_curse;
      o_ptr->ident |= IDENT_KNOW_CURSES;
      if (o_ptr->flags_curse) o_ptr->ident |= IDENT_CURSED;
      else o_ptr->ident |= IDENT_UNCURSED;
    }

  /* If it seemed dubious but has no identifiable negatives, it's cursed */
  if (!item_dubious(o_ptr, FALSE) && was_dubious) 
    o_ptr->ident |= IDENT_CURSED;
  
  /* If artifact, write a note if applicable */
  if ((o_ptr->name1) && (o_ptr->found))
    {
      int artifact_stage, lev;
      char note[120];
      char shorter_desc[120];
      s32b real_turn = turn;
      
      /* Get a shorter description to fit the notes file */
      object_desc(shorter_desc, o_ptr, TRUE, 0);
      
      /* Build note and write */
      sprintf(note, "Found %s", shorter_desc);
      
      /* Record the depth where the artifact was created */
      artifact_stage = o_ptr->found;
      
      /* Hack - record the turn when the artifact was first picked up
       * or wielded by the player.  This may result in out of order
       * entries in the notes file, which really should be re-ordered 
       */
      turn = a_info[o_ptr->name1].creat_turn;
      if (turn < 2) turn = real_turn;
      lev = (int)a_info[o_ptr->name1].p_level;
      if (lev == 0) lev = (int)p_ptr->lev;
      make_note(note, artifact_stage, NOTE_ARTIFACT, (s16b)lev);
      turn = real_turn;
      
      /*
       * Mark item creation depth 0, which will indicate the artifact
       * has been previously identified.  This prevents an artifact
       * from showing up on the notes list twice if the artifact had
       * been previously identified.  JG
       */
      o_ptr->found = 0 ;
    }

  /* If the object is flavored, also make all items of that type, 
   * except for variable rings and amulets, fully known. */
  if (k_ptr->flavor)
    {
      if ((o_ptr->tval == TV_FOOD) || (o_ptr->tval == TV_STAFF) || 
	  (o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD)) 
	k_ptr->known_effect = TRUE;
    }
}

/**
 * Identify everything being carried.
 * Done by a potion of "*Enlightenment*".
 */
void identify_pack(void)
{
  int i;
  
  /* Simply identify and know every item */
  for (i = 0; i < INVEN_TOTAL; i++)
    {
      object_type *o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Identify it */
      identify_object(o_ptr);
    }
  
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
}






/**
 * Used by the "enchant" function (chance of failure)
 */
static int enchant_table[16] =
{
  0, 10, 50, 100, 200,
  300, 400, 500, 700, 950,
  990, 992, 995, 997, 999,
  1000
};


static bool item_tester_cursed(object_type *o_ptr)
{
  if (known_cursed_p(o_ptr) && !(o_ptr->flags_obj & OF_PERMA_CURSE))
    return TRUE;
  else
    return FALSE;
}

/**
 * Attempts to remove curses from items in inventory
 *
 * Note that Items which are "Perma-Cursed" 
 * can NEVER be uncursed.
 *
 * If good is TRUE, there is a better chance of removal.  Using this spell
 * once makes an item fragile, which means it is likely to be destroyed on
 * a second attempt.
 */
static bool remove_curse_aux(int good)
{
  int i, item, slot;
  
  object_type *o_ptr;
  
  char o_name[120];
  
  cptr q, s;

  u32b curses = 0L;

  int destroy_chance = 50;
  int uncurse_chance = 50;

  bool heavy = FALSE;
  int feel;

  /* Only cursed items */
  item_tester_hook = item_tester_cursed;
  
  /* Don't restrict choices */
  item_tester_tval = 0;
  
  /* Get an item.   */
  q = "Attempt to uncurse which item? ";
  s = "You have no curses which can be removed.";
  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) 
    return (FALSE);
  
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

  /* Artifacts are harder to uncurse and destroy */
  if (artifact_p(o_ptr))
    {
      destroy_chance -= 25;
      uncurse_chance -= 25;
    }

  /* Try every curse, even unknown ones */
  for (i = 0; i < OBJECT_RAND_SIZE_CURSE; i++)
    if (o_ptr->flags_curse & (1L << i))
      {
	/* If fragile, bad things can happen */
	if ((o_ptr->flags_obj & OF_FRAGILE) && 
	    (rand_int(100) < destroy_chance - (good ? 10 : 0)))
	  {
	      /* Message */
	      msg_print("There is a bang and a flash!");
	      
	      /* Damage */
	      take_hit(damroll(5, 5), "Failed uncursing");

	      /* Gone */
	      inven_item_increase(item, -1);
	      inven_item_optimize(item);
	      return (FALSE);
	  }

	/* Try once */
	if (rand_int(100) < uncurse_chance) curses |= (1L << i);

	/* If good, try again */
	if (good && (rand_int(100) < uncurse_chance)) curses |= (1L << i);
      }
      
  /* Uncurse it */
  o_ptr->flags_curse &= ~(curses);
  o_ptr->id_curse &= ~(curses);

  /* May not be cursed any more */
  if (!o_ptr->id_curse) o_ptr->ident &= ~(IDENT_CURSED);

  /* Fragile now */
  o_ptr->flags_obj |= (OF_FRAGILE);
  o_ptr->id_obj |= (OF_FRAGILE);
  
  /* Known objects get free curse notification now */
  if (object_known_p(o_ptr))
    {
      if (o_ptr->flags_curse) o_ptr->ident |= IDENT_CURSED;
      else o_ptr->ident |= (IDENT_UNCURSED | IDENT_KNOW_CURSES);
    }
  /* Redo feeling if it's not known */
  else
    {
      /* Heavy sensing */
      heavy = (check_ability(SP_PSEUDO_ID_HEAVY));
      
      /* Type of feeling */
      feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
      
      /* Check the slot */
      slot = wield_slot(o_ptr);
  
      /* Redo feeling */
      if (!(o_ptr->feel == feel))
	{
	  /* Get an object description */
	  object_desc(o_name, o_ptr, FALSE, 0);
	  
	  msg_format("You feel the %s (%c) you are %s %s now %s...",
		     o_name, index_to_label(slot), describe_use(slot),
		     ((o_ptr->number == 1) ? "is" : "are"), feel_text[feel]);
      
	  /* We have "felt" it */
	  o_ptr->ident |= (IDENT_SENSE);
	  
	  /* Inscribe it textually */
	  o_ptr->feel = feel;
	  
	  /* Set squelch flag as appropriate */
	  p_ptr->notice |= PN_SQUELCH;
	}
    }
  
  /* Recalculate the bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Window stuff */
  p_ptr->window |= (PW_EQUIP | PW_INVEN);
  
  /* Return "something uncursed" */
  return (curses ? TRUE : FALSE);
}


/**
 * Remove most curses
 */
bool remove_curse(void)
{
  return (remove_curse_aux(FALSE));
}

/**
 * Remove all curses
 */
bool remove_curse_good(void)
{
  return (remove_curse_aux(TRUE));
}



/**
 * Restores any drained experience
 */
bool restore_level(void)
{
  /* Restore experience */
  if (p_ptr->exp < p_ptr->max_exp)
    {
      /* Message */
      msg_print("You feel your life energies returning.");
      
      /* Restore the experience */
      p_ptr->exp = p_ptr->max_exp;
      
      /* Check the experience */
      check_experience();
      
      /* Did something */
      return (TRUE);
    }
  
  /* No effect */
  return (FALSE);
}


void do_cmd_bear_shape(void)
{
  /* Sanity */
  if ((SCHANGE) || (!check_ability(SP_BEARSKIN))) return;
  
  /* Confirm */
  if (!get_check("Assume the form of a bear? ")) return;
  
  /* Change */
  shapechange(SHAPE_BEAR);
  
  /* Use some energy */
  p_ptr->energy_use = 100;
}


/**
 * Stop doing a shapechange.  From Sangband.
 */
void do_cmd_unchange(void)
{
  if (!SCHANGE)
    {
      msg_print("You aren't in another form right now.");
      return;
    }
  
  /* Confirm */
  if (!get_check("Really return to normal? "))
    return;
  
  /* Return to normal form */
  shapechange(SHAPE_NORMAL);
  
  /* Hack - refund mana (2/3 mana when shapeshifted). */
  if (p_ptr->csp > 0)
    {
      /* Hack - Recalculate mana now, even though we will */
      /* update it fully, to ensure the refund to current */
      /* mana doesn't get cleared */
      p_ptr->msp *= 3;
      p_ptr->msp /= 2;
      
      /* Refund current mana - removed, as mana is not reduced in the other
       * shapechange
      p_ptr->csp *= 3;
      p_ptr->csp /= 2;
      if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp; */
      
      /* Display mana later */
      p_ptr->redraw |= (PR_MANA);
    }
  
  /* Recalculate mana. */
  p_ptr->update |= (PU_MANA);

  /* Show or hide shapechange on main window. */
  p_ptr->redraw |= (PR_SHAPE);

  /* Use some energy */
  p_ptr->energy_use = 100;
}


/**
 * Forget everything
 */
bool lose_all_info(void)
{
  /* Mega-Hack -- Forget the map */
  wiz_dark();
  
  /* It worked */
  return (TRUE);
}


/* Recall point code */

/* Are we heading home? */
bool inward = FALSE;

/* the number of available points */
int num_points = 0;

static char recall_tag(menu_type *menu, int oid)
{
  return I2A(oid);
}

/**
 * Display an entry on the recall menu
 */
void recall_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  const u16b *choice = menu->menu_data;
  int idx = choice[oid];
  char stage[30];

  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  if (idx < num_points)
    {
      int region = stage_map[p_ptr->recall[idx]][LOCALITY];
      int level  = stage_map[p_ptr->recall[idx]][DEPTH];
      
      /* Get the recall point description */
      if (level)
	sprintf(stage, "%s %d   ", locality_name[region], level);
      else if (region)
	sprintf(stage, "%s Town   ", locality_name[region]);
      else
	sprintf(stage, "%s     ", locality_name[region]);	      
    }
  else
    sprintf(stage,  "Don't replace     ");

  /* Print it */
  c_put_str(attr, format("%s", stage), row, col);
}

/**
 * Deal with events on the recall menu
 */
bool recall_action(char cmd, void *db, int oid)
{
  u16b *choice = db;
  
  int idx = choice[oid];

  if (inward)
    {
      int stage;
	      
      /* Find the point, being careful about underworld etc */
      if ((p_ptr->stage == 255) || (p_ptr->stage == 256))
	stage = p_ptr->last_stage;
      else 
	stage = p_ptr->stage;
      
      /* Check for replacement */
      if (idx < num_points)
	p_ptr->recall[idx] = stage;

      /* Set it */
      p_ptr->recall_pt = stage;
    }
  else
    {
      if (p_ptr->recall[idx] == NOWHERE) return (FALSE);
      p_ptr->recall_pt = p_ptr->recall[idx];
    }
  
  return TRUE;
}


/**
 * Display list of recall points.
 */
bool recall_menu(void)
{
  menu_type menu;
  menu_iter menu_f = { 0, recall_tag, 0, recall_display, recall_action };
  region area = { (small_screen ? 0 : 15), 1, 48, -1 };
  event_type evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;
  int num_entries;
  int num_poss = adult_dungeon ? 1 : 4;
  
  size_t i;
  
  u16b *choice;

  /* See how many recall points - show exactly one Nowhere if going home */
  num_points = 0;
  for (i = 0; i < num_poss; i++)
    if (p_ptr->recall[i]) num_points++;

  if (inward && (num_points < num_poss)) num_points++;
  if (inward) num_entries = num_points + 1;
  else num_entries = num_points;
  
  /* Create the array */
  choice = C_ZNEW(num_entries, u16b);
  
  /* Obvious */
  for (i = 0; i < num_entries; i++)
    choice[i] = i;

  /* Clear space */
  area.page_rows = num_entries + 2;
  
  /* Return here if there is nowhere to recall to */
  if (!num_entries)
    {
      FREE(choice);
      return FALSE;
    }
  
  
  /* Save the screen and clear it */
  screen_save();
  
  /* Help text */
  
  /* Set up the menu */
  WIPE(&menu, menu);
  if (inward)
    menu.title = "Which recall point will you replace (or ESC):";
  else
    menu.title = "Which recall point do you want to go to?";
  menu.cmd_keys = " \n\r";
  menu.count = num_entries;
  menu.menu_data = choice;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &area);
  
  /* Select an entry */
  evt = menu_select(&menu, &cursor, 0);
  
  /* Free memory */
  FREE(choice);
  
  /* Load screen */
  screen_load();
  return ((evt.type != EVT_ESCAPE) && (evt.type != EVT_BACK));
}

/**
 * Set "p_ptr->word_recall", notice observable changes
 */
bool set_recall(int v)
{

  bool notice = FALSE;
  
  /* No use until the player has been somewhere */
  if (((p_ptr->stage == p_ptr->home) && (!p_ptr->recall_pt)) || 
      (!p_ptr->home) || (adult_ironman && !p_ptr->total_winner))
    {
      msg_print("Nothing happens.");
      return(FALSE);
    }
  
  /* Hack -- Force good values */
  v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
  
  /* Open */
  if (v)
    {
      if (!p_ptr->word_recall)
	{
	  inward = (p_ptr->stage != p_ptr->home);
	  
	  if (!recall_menu()) return FALSE;
	  msg_print("The air about you becomes charged...");
	  notice = TRUE;
	}
    }
  
  /* Shut */
  else
    {
      if (p_ptr->word_recall)
	{
	  msg_print("A tension leaves the air around you...");
	  notice = TRUE;
	}
    }
  
  /* Use the value */
  p_ptr->word_recall = v;
  
  /* Nothing to notice */
  if (!notice) return (FALSE);
  
  /* Disturb */
  if (disturb_state) disturb(0, 0);
  
  /* Redraw status */
  p_ptr->redraw |= PR_STATUS;
  
  /* Handle stuff */
  handle_stuff();
  
  /* Result */
  return (TRUE);
}


/**
 * Hack - displays areas effected by detection spells.
 *
 */
static void animate_detect(int rad)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int x, y;

  byte a, c;
  
  int msec = op_ptr->delay_factor * op_ptr->delay_factor;
  
  /* Exit if not desired */
  if (!show_detect) return;
  
  /* Hack - Needs to last a bit longer to be visible */
  msec *= 6;
  
  /* Scan the maximal area of detection */
  for (y = py - rad; y <= py + rad; y++)
    {
      for (x = px - rad; x <= px + rad; x++)
	{
	  
	  /* Ignore "illegal" locations */
	  if (!in_bounds(y, x)) continue;
	  
	  /* Enforce a "circular" area */
	  if (distance(py, px, y, x) > rad) continue;
	  
	  /* Only show the region that the player can see */
	  if (panel_contains(y, x))
	    {
	      /* Hack - Obtain attr/char */
	      a = misc_to_attr[0x3B];
	      c = misc_to_char[0x3B];

	      /* Hack -- Visual effects -- Display a yellow star */
	      print_rel(c, a, y, x);
	    }
	}
    }
  
  /* Flush the image of detected region */
  if (fresh_before) Term_fresh();
  
  /* Delay (efficiently) */
  Term_xtra(TERM_XTRA_DELAY, msec);
  
  /* Now erase the effect */
  for (y = py - rad; y <= py + rad; y++)
    {
      for (x = px - rad; x <= px + rad; x++)
	{
	  /* Ignore "illegal" locations */
	  if (!in_bounds(y, x)) continue;
	  
	  /* Enforce a "circular" area */
	  if (distance(py, px, y, x) > rad) continue;
	  
	  /* Hack -- Erase only if needed */
	  if (panel_contains(y, x))
	    {
	      lite_spot(y, x);
	    }
	}
    }
  
  /* Hack -- center the cursor */
  move_cursor_relative(py, px);
  
  /* Flush screen back to normal */
  if (fresh_before) Term_fresh();
  
  /* Exit */
  return;
	
}

/**
 * Detect all traps within range 
 */
bool detect_traps(int range, bool show)
{
  int y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int num = 0;
  
  feature_type *f_ptr = NULL;

  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan the map */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Set the feature */
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* check range */
	  if (distance(py, px, y, x) <= range)
	    {
	      /* Detect invisible traps */
	      if (f_ptr->flags & TF_TRAP_INVIS)
		{
		  /* Pick a trap */
		  pick_trap(y, x);
		  
		  /* Reset the feature */
		  f_ptr = &f_info[cave_feat[y][x]];
		}
	      
	      /* Detect traps */
	      if (f_ptr->flags & TF_TRAP)
		{
		  /* Hack -- Memorize */
		  cave_info[y][x] |= (CAVE_MARK);
		  
		  /* Redraw */
		  lite_spot(y, x);
		  
		  /* increment number found */
		  num++;
		}
	      
	      /* Mark grid as detected */
	      cave_info2[y][x] |= (CAVE2_DTRAP);
	    }
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      /* Print success message */
      msg_print("You detect traps.");
    }
  
  /* Redraw DTrap Status */
  p_ptr->redraw |= (PR_DTRAP);
  
  /* Result - trap detection items are easy to recognize for now -BR- */
  return (TRUE);
}



/**
 * Detect all doors within range 
 */
bool detect_doors(int range, bool show)
{
  int y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool detect = FALSE;
  
  int num = 0;

  feature_type *f_ptr = NULL;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan the map */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  
	  /* check range */
	  if (distance(py, px, y, x) <= range)
	    {
	      /* Detect secret doors */
	      if (cave_feat[y][x] == FEAT_SECRET)
		{
		  /* Pick a door */
		  place_closed_door(y, x);
		}
	      
	      /* Set the feature */
	      f_ptr = &f_info[cave_feat[y][x]];

	      /* Detect doors */
	      if (f_ptr->flags & TF_DOOR_ANY)
		{
		  /* Hack -- Memorize */
		  cave_info[y][x] |= (CAVE_MARK);
		  
		  /* Redraw */
		  lite_spot(y, x);
		  
		  /* increment number found */
		  num++;
		}
	    }
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      detect = TRUE;
      
      /* Print success message */
      msg_print("You detect doors.");
    }
  
  /* Result */
  return (detect);
}



/**
 * Detect all stairs within range 
 */
bool detect_stairs(int range, bool show)
{
  int y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int num=0;
  
  bool detect = FALSE;
 
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan the map */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  
	  /* check range */
	  if (distance(py, px, y, x) <= range)
	    {
		  feature_type *f_ptr = &f_info[cave_feat[y][x]];
	      /* Detect stairs */

/*	      if ((cave_feat[y][x] == FEAT_LESS) ||
		  (cave_feat[y][x] == FEAT_MORE)) */

		  if (f_ptr->flags & TF_STAIR)

		{
		  /* Hack -- Memorize */
		  cave_info[y][x] |= (CAVE_MARK);
		  
		  /* Redraw */
		  lite_spot(y, x);
		  
		  /* increment number found */
		  num++;
		}
	    }
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      detect = TRUE;
      
      /* Print success message */
      msg_print("You detect stairs.");
    }
  
  /* Result */
  return (detect);
}


/**
 * Detect any treasure within range
 */
bool detect_treasure(int range, bool show)
{
  int y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool detect = FALSE;
  
  int num=0;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan the map */
  for (y = 0; y<DUNGEON_HGT; y++)
    {
      for (x = 0; x<DUNGEON_WID; x++)
	{
	  
	  /* check range */
	  if (distance(py, px, y, x) <= range)
	    {
	      /* Notice embedded gold */
	      if ((cave_feat[y][x] == FEAT_MAGMA_H) ||
		  (cave_feat[y][x] == FEAT_QUARTZ_H))
		{
		  /* Expose the gold */
		  cave_feat[y][x] += 0x02;
		}
	      
	      /* Magma/Quartz + Known Gold */
	      if ((cave_feat[y][x] == FEAT_MAGMA_K) || 
		  (cave_feat[y][x] == FEAT_QUARTZ_K))
		{
		  /* Hack -- Memorize */
		  cave_info[y][x] |= (CAVE_MARK);
		  
		  /* Redraw */
		  lite_spot(y, x);
		  
		  /* increment number found */
		  num++;
		}
	    }
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      detect = TRUE;
      
      /* Print success message */
      msg_print("You detect buried treasure.");
      
    }
  
  /* Result */
  return (detect);
}



/**
 * Detect all "gold" objects within range
 */
bool detect_objects_gold(int range, bool show)
{
  int i, y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int num=0;
  
  bool detect = FALSE;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan objects */
  for (i = 1; i < o_max; i++)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Skip dead objects */
      if (!o_ptr->k_idx) continue;
      
      /* Skip held objects */
      if (o_ptr->held_m_idx) continue;
      
      /* Location */
      y = o_ptr->iy;
      x = o_ptr->ix;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Detect "gold" objects */
      if (o_ptr->tval == TV_GOLD)
	{
	  /* Hack -- memorize it */
	  o_ptr->marked = TRUE;
	  
	  /* Redraw */
	  lite_spot(y, x);
	  
	  /* increment number found */
	  num++;
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      detect = TRUE;
      msg_print("You detect treasure.");
      
    }
  
  /* Result */
  return (detect);
  
}


/**
 * Detect all "normal" objects within range
 */
bool detect_objects_normal(int range, bool show)
{
  int i, y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int num=0;
  
  bool detect = FALSE;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan objects */
  for (i = 1; i < o_max; i++)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Skip dead objects */
      if (!o_ptr->k_idx) continue;
      
      /* Skip held objects */
      if (o_ptr->held_m_idx) continue;
      
      /* Location */
      y = o_ptr->iy;
      x = o_ptr->ix;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Detect "real" objects */
      if (o_ptr->tval != TV_GOLD)
	{
	  /* Hack -- memorize it */
	  o_ptr->marked = TRUE;
	  
	  /* Redraw */
	  lite_spot(y, x);
	  
	  /* increment number found */
	  if (!squelch_hide_item(o_ptr))
	    num++;
	  
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      detect = TRUE;
      
      /* Print success message */
      msg_print("You detect objects.");
      
    }
  
  /* Result */
  return (detect);
}


/**
 * Detect all "magic" objects within range.
 *
 * This will light up all spaces with "magic" items, including artifacts,
 * ego-items, potions, scrolls, books, rods, wands, staves, amulets, rings,
 * and "enchanted" items of the "good" variety.
 *
 * It can probably be argued that this function is now too powerful.
 */
bool detect_objects_magic(int range, bool show)
{
  int i, y, x, tv;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool detect = FALSE;
  
  int num=0;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan all objects */
  for (i = 1; i < o_max; i++)
    {
      object_type *o_ptr = &o_list[i];
      
      /* Skip dead objects */
      if (!o_ptr->k_idx) continue;
      
      /* Skip held objects */
      if (o_ptr->held_m_idx) continue;
      
      /* Location */
      y = o_ptr->iy;
      x = o_ptr->ix;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Examine the tval */
      tv = o_ptr->tval;
      
      /* Artifacts, misc magic items, or enchanted wearables */
      if (artifact_p(o_ptr) || ego_item_p(o_ptr) ||
	  (tv == TV_AMULET) || (tv == TV_RING) ||
	  (tv == TV_STAFF) || (tv == TV_WAND) || (tv == TV_ROD) ||
	  (tv == TV_SCROLL) || (tv == TV_POTION) ||
	  (tv == TV_MAGIC_BOOK) || (tv == TV_PRAYER_BOOK) ||
	  (tv == TV_DRUID_BOOK) || (tv == TV_NECRO_BOOK) ||
	  ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0)))
	{
	  /* Memorize the item */
	  o_ptr->marked = TRUE;
	  
	  /* Redraw */
	  lite_spot(y, x);
	  
	  /* increment number found */
	  if (!squelch_hide_item(o_ptr))
	    num++;
	  
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      detect = TRUE;
      
      /* Print success message */
      msg_print("You detect magic objects.");
      
    }
  
  /* Return result */
  return (detect);
}


/**
 * Detect all "normal" monsters within range
 */
bool detect_monsters_normal(int range, bool show)
{
  int i, y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool flag = FALSE;
  
  int num=0;
  int num_off=0;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Location */
      y = m_ptr->fy;
      x = m_ptr->fx;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Detect all non-invisible monsters */
      if (!(r_ptr->flags2 & (RF2_INVISIBLE)))
	{
	  /* Optimize -- Repair flags */
	  repair_mflag_mark = repair_mflag_show = TRUE;
	  
	  /* Hack -- Detect the monster */
	  m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);
	  
	  /* Update the monster */
	  update_mon(i, FALSE);
	  
	  /* increment number found */
	  num++;
	  
	  /* increment number found offscreen */
	  if (!panel_contains(y, x)) num_off++;
	  
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      flag = TRUE;
      
      /* Print success message */
      if (num_off > 0) msg_format("You detect monsters (%i offscreen).",
				  num_off);
      else msg_print("You detect monsters.");
    }
  
  /* Result */
  return (flag);
}


/**
 * Detect all "invisible" monsters within range
 */
bool detect_monsters_invis(int range, bool show)
{
  int i, y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool flag = FALSE;
  
  int num=0;
  int num_off=0;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      monster_lore *l_ptr = &l_list[m_ptr->r_idx];
      
      /* Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Location */
      y = m_ptr->fy;
      x = m_ptr->fx;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Detect invisible monsters */
      if (r_ptr->flags2 & (RF2_INVISIBLE))
	{
	  /* Take note that they are invisible */
	  l_ptr->flags2 |= (RF2_INVISIBLE);
	  
	  /* Update monster recall window */
	  if (p_ptr->monster_race_idx == m_ptr->r_idx)
	    {
	      /* Window stuff */
	      p_ptr->window |= (PW_MONSTER);
	    }
	  
	  /* Optimize -- Repair flags */
	  repair_mflag_mark = repair_mflag_show = TRUE;
	  
	  /* Hack -- Detect the monster */
	  m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);
	  
	  /* Update the monster */
	  update_mon(i, FALSE);
	  
	  /* increment number found */
	  num++;
	  
	  /* increment number found offscreen */
	  if (!panel_contains(y, x)) num_off++;
	  
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      flag = TRUE;
      
      /* Print success message */
      if (num_off > 0) 
	msg_format("You detect invisible creatures (%i offscreen).", num_off);
      else msg_print("You detect invisible creatures.");
    }
  
  /* Result */
  return (flag);
}



/**
 * Detect all "evil" monsters within range
 */
bool detect_monsters_evil(int range, bool show)
{
  int i, y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool flag = FALSE;
  
  int num=0;
  int num_off=0;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      monster_lore *l_ptr = &l_list[m_ptr->r_idx];
      
      /* Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Location */
      y = m_ptr->fy;
      x = m_ptr->fx;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Detect evil monsters */
      if (r_ptr->flags3 & (RF3_EVIL))
	{
	  /* Take note that they are evil */
	  l_ptr->flags3 |= (RF3_EVIL);
	  
	  /* Update monster recall window */
	  if (p_ptr->monster_race_idx == m_ptr->r_idx)
	    {
	      /* Window stuff */
	      p_ptr->window |= (PW_MONSTER);
	    }
	  
	  /* Optimize -- Repair flags */
	  repair_mflag_mark = repair_mflag_show = TRUE;
	  
	  /* Detect the monster */
	  m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);
	  
	  /* Update the monster */
	  update_mon(i, FALSE);
	  
	  /* increment number found */
	  num++;
	  
	  /* increment number found offscreen */
	  if (!panel_contains(y, x)) num_off++;
	  
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      flag = TRUE;
      
      /* Print success message */
      if (num_off > 0) 
	msg_format("You detect evil creatures (%i offscreen).", num_off);
      else msg_print("You detect evil creatures.");
      
    }
  
  /* Result */
  return (flag);
  
  
}


/**
 * Detect all "living" monsters within range.
 */
bool detect_monsters_living(int range, bool show)
{
  int i, y, x;
  
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  bool flag = FALSE;
  
  int num=0;
  int num_off=0;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Scan monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Location */
      y = m_ptr->fy;
      x = m_ptr->fx;
      
      /* check range */
      if (distance(py, px, y, x) > range) continue;
      
      /* Hack -- Detect all living monsters. */
      if ((!strchr("Egv", r_ptr->d_char)) && 
	  (!(r_ptr->flags3 & (RF3_UNDEAD))))
	{
	  /* Optimize -- Repair flags */
	  repair_mflag_mark = repair_mflag_show = TRUE;
	  
	  /* Hack -- Detect the monster */
	  m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);
	  
	  /* Update the monster */
	  update_mon(i, FALSE);
	  
	  /* increment number found */
	  num++;
	  
	  /* increment number found offscreen */
	  if (!panel_contains(y, x)) num_off++;
	}
    }
  
  /* Found some */
  if (num > 0)
    {
      
      /* Obvious */
      flag = TRUE;
      
      /* Print success message */
      if (num_off > 0) 
	msg_format("You detect living creatures (%i offscreen).", num_off);
      else msg_print("You detect living creatures.");
      
    }
  
  /* Now detect trees */
  num = 0;

  /* Scan the maximal area of mapping */
  for (y = py - range; y <= py + range; y++)
    {
      for (x = px - range; x <= px + range; x++)
	{
	  feature_type *f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Ignore "illegal" locations */
	  if (!in_bounds(y, x)) continue;
	  
	  /* Enforce a "circular" area */
	  if (distance(py, px, y, x) > range) continue;
	  
	  /* Notice trees */
	  if (f_ptr->flags & TF_TREE)
	    {
	      /* Mark it */
	      cave_info[y][x] |= CAVE_MARK;
	      
	      /* Count it */
	      num++;
	    }
	}
    }

  /* Found some */
  if (num > 0)
    {
      flag = TRUE;      

      /* Print message */
      msg_print("You detect trees.");

      /* Update */
      p_ptr->redraw |= PR_MAP;

      redraw_stuff();
    }


  /* Result */
  return (flag);
}



/**
 * Detect everything
 */
bool detect_all(int range, bool show)
{
  bool detect = FALSE;
  
  /* Hack - flash the effected region on the current panel */
  if (show) animate_detect(range);
  
  /* Detect everything */
  /* Do not 'show' the affected region for each
   * detect individually 
   */
  if (detect_traps(range, FALSE)) detect = TRUE;
  if (detect_doors(range, FALSE)) detect = TRUE;
  if (detect_stairs(range, FALSE)) detect = TRUE;
  if (detect_treasure(range, FALSE)) detect = TRUE;
  if (detect_objects_gold(range, FALSE)) detect = TRUE;
  if (detect_objects_normal(range, FALSE)) detect = TRUE;
  if (detect_monsters_invis(range, FALSE)) detect = TRUE;
  if (detect_monsters_normal(range, FALSE)) detect = TRUE;
  
  /* Result */
  return (detect);
}



/**
 * Create stairs at the player location
 */
void stair_creation(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  /* XXX XXX XXX */
  if (!cave_valid_bold(py, px))
    {
      msg_print("The object resists the spell.");
      return;
    }

  /* Doesn't work outside caves */
  if (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
    {
      msg_print("You can only create stairs in caves!");
      return;
    }
  
  /* XXX XXX XXX */
  delete_object(py, px);
  
  /* Create a staircase */
  if (is_quest(p_ptr->stage) || (!stage_map[p_ptr->stage][DOWN]))
    {
      cave_set_feat(py, px, FEAT_LESS);
    }
  else if (rand_int(100) < 50)
    {
      cave_set_feat(py, px, FEAT_MORE);
    }
  else
    {
      cave_set_feat(py, px, FEAT_LESS);
    }
}




/**
 * Hook to specify "weapon"
 */
static bool item_tester_hook_weapon(object_type *o_ptr)
{
  switch (o_ptr->tval)
    {
    case TV_SWORD:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING:
    case TV_BOW:
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
      {
	return (TRUE);
      }
    }
  
  return (FALSE);
}


/**
 * Hook to specify "ammunition"
 */
static bool item_tester_hook_ammo(object_type *o_ptr)
{
  switch (o_ptr->tval)
    {
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT:
      {
	return (TRUE);
      }
    }
  
  return (FALSE);
}


/**
 * Hook to specify "armour"
 */
static bool item_tester_hook_armour(object_type *o_ptr)
{
  switch (o_ptr->tval)
    {
    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_BOOTS:
    case TV_GLOVES:
      {
	return (TRUE);
      }
    }
  
  return (FALSE);
}

static bool item_tester_unknown(object_type *o_ptr)
{
  if (object_known_p(o_ptr))
    return FALSE;
  else
    return TRUE;
}

static bool item_tester_unknown_curse(object_type *o_ptr)
{
  if (object_known_p(o_ptr) && (o_ptr->ident & IDENT_KNOW_CURSES))
    return FALSE;
  else
    return TRUE;
}


static bool item_tester_unproofed(object_type *o_ptr)
{
  if (o_ptr->number != 1)
    return FALSE;
  if (o_ptr->flags_obj & el_to_proof)
    return FALSE;
  else
    return TRUE;
}


/**
 * Enchant an item
 *
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item.  -CFT
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 */
bool enchant(object_type *o_ptr, int n, int eflag)
{
  int i, chance, prob;
  
  bool res = FALSE;
  
  bool a = artifact_p(o_ptr);
  
  
  /* Large piles resist enchantment */
  prob = o_ptr->number * 100;

  /* Missiles are easy to enchant */
  if ((o_ptr->tval == TV_BOLT) ||
      (o_ptr->tval == TV_ARROW) ||
      (o_ptr->tval == TV_SHOT))
    {
      prob = prob / 35;
    }
  
  /* Try "n" times */
  for (i = 0; i < n; i++)
    {
      /* Hack -- Roll for pile resistance */
      if ((prob > 100) && (rand_int(prob) >= 100)) continue;
      
      /* Enchant to hit */
      if (eflag & (ENCH_TOHIT))
	{
	  if (o_ptr->to_h < 0) chance = 0;
	  else if (o_ptr->to_h > 15) chance = 1000;
	  else chance = enchant_table[o_ptr->to_h];
	  
	  /* Attempt to enchant */
	  
	  if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
	    {
	      res = TRUE;
	      
	      /* Enchant */
	      o_ptr->to_h++;
	    }
	}
      
      /* Enchant to damage */
      if (eflag & (ENCH_TODAM))
	{
	  if (o_ptr->to_d < 0) chance = 0;
	  else if (o_ptr->to_d > 15) chance = 1000;
	  else chance = enchant_table[o_ptr->to_d];
	  
	  if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
	    {
	      res = TRUE;
	      
	      /* Enchant */
	      o_ptr->to_d++;
	    }
	}
      
      /* Enchant to armor class */
      if (eflag & (ENCH_TOAC))
	{
	  if (o_ptr->to_a < 0) chance = 0;
	  else if (o_ptr->to_a > 15) chance = 1000;
	  else chance = enchant_table[o_ptr->to_a];
	  
	  if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
	    {
	      res = TRUE;
	      
	      /* Enchant */
	      o_ptr->to_a++;
	    }
	}
    }
  
  /* Failure */
  if (!res) return (FALSE);
  
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
  /* Success */
  return (TRUE);
}



/**
 * Enchant an item (in the inventory or on the floor)
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell(int num_hit, int num_dam, int num_ac)
{
  int item;
  bool okay = FALSE;
  
  object_type *o_ptr;
  
  char o_name[120];
  
  cptr q, s;
  
  
  /* Assume enchant weapon */
  item_tester_hook = item_tester_hook_weapon;
  
  /* Don't restrict choices */
  item_tester_tval = 0;
  
  /* Enchant armor if requested */
  if (num_ac) item_tester_hook = item_tester_hook_armour;
  
  /* Get an item */
  q = "Enchant which item? ";
  s = "You have nothing to enchant.";
  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) 
    return (FALSE);
  
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
  
  
  /* Description */
  object_desc(o_name, o_ptr, FALSE, 0);
  
  /* Describe */
  msg_format("%s %s glow%s brightly!", ((item >= 0) ? "Your" : "The"), o_name,
	     ((o_ptr->number > 1) ? "" : "s"));
  
  /* Enchant */
  if (enchant(o_ptr, num_hit, ENCH_TOHIT)) okay = TRUE;
  if (enchant(o_ptr, num_dam, ENCH_TODAM)) okay = TRUE;
  if (enchant(o_ptr, num_ac, ENCH_TOAC)) okay = TRUE;
  
  /* Failure */
  if (!okay)
    {
      /* Flush */
      if (flush_failure) flush();
      
      /* Message */
      msg_print("The enchantment failed.");
    }
  
  /* Something happened */
  return (TRUE);
}

/**
 * Enchant some missiles and give them an elemental brand
 *
 * Combines the old brand_bolts and brand_missiles routines.
 *
 * ammo_type is the tval of the relevant ammunition.  
 * If set to 0, any ammunition is enchantable.
 *
 * Brand type is the EGO flag for the relevant type element.
 * If set to 0, a non-poison brand is picked randomly.
 *
 */
bool brand_missile(int ammo_type, int brand_type)
{
  int item, choice;
  object_type *o_ptr;
  cptr q, s;
  bool status;
  
  /* Restrict choices
   * Hack - check for restricted choice */
  if ((ammo_type >= TV_SHOT) && (ammo_type <= TV_BOLT)) 
    item_tester_tval = ammo_type;
  
  /* Otherwise any ammo will do */
  else item_tester_hook = item_tester_hook_ammo;
  
  /* Get an item */
  q = "Enchant which ammunition? ";
  s = "You have no ammunition to brand.";
  status = get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR));
  
  /* Hack - if failed, return, but only after resetting the ammo hack */
  if (!status) return (FALSE);
  
  if (item >= 0)
    {
      o_ptr = &inventory[item];
    }
  
  /* Get the item (on the floor) */
  else
    {
      o_ptr = &o_list[0 - item];
    }
  
  /*
   * Don't enchant artifacts or ego-items
   */
  if (artifact_p(o_ptr) || ego_item_p(o_ptr))
    {
      /* Flush */
      if (flush_failure) flush();
      
      /* Fail */
      msg_print("The ammunition enchantment failed.");
      
      /* Notice */
      return (TRUE);
    }
  
  /* Type of brand may be restricted */
  if (brand_type) choice = brand_type;
  
  /* Otherwise choose randomly
   * Hack - Never get poison brand randomly */
  else choice = rand_int(4) + EGO_ACIDIC;
  
  switch (choice)
    {
    case EGO_FLAME:
      {
	/* Print message and fire brand missiles. */
	msg_print("Your missiles are covered in a fiery aura!");
	break;
      }
      
    case EGO_FROST:
      {
	/* Print message and frost brand missiles. */
	msg_print("Your missiles are covered in a frosty sheath!");
	break;
      }
      
    case EGO_ACIDIC:
      {
	/* Print message and acid brand missiles. */
	msg_print("Your missiles sizzle with acid!");
	break;
      }
      
    case EGO_ELECT:
      {
	/* Print message and electric brand missiles. */
	msg_print("Your missiles are covered in sparks!");
	break;
      }
      
    case EGO_POISON:
      {
	/* Print message and poison brand missiles. */
	msg_print("Your missiles drip with deadly poison!");
	break;
      }
      
    default:
      {
	/* Oops */
	return (FALSE);
      }
    }
  
  /* Brand */
  o_ptr->name2 = choice;
  o_ptr->multiple_brand[choice - EGO_ACIDIC] = BRAND_BOOST_NORMAL;

  /* Known now */
  identify_object(o_ptr);
  
  /* Enchant */
  enchant(o_ptr, rand_int(4) + 3, ENCH_TOHIT | ENCH_TODAM);
  
  /* Prevent money-making. */
  o_ptr->discount = 80;
  
  /* Notice */
  return (TRUE);
}

/**
 * Set a temporary elemental brand.  Clear all other brands.  Print status 
 * messages. -LM-
 */
void set_ele_attack(u32b attack_type, int duration)
{
  /* Clear all elemental attacks (only one is allowed at a time). */
  if ((p_ptr->special_attack & (ATTACK_ACID)) && (attack_type != ATTACK_ACID))
    {
      p_ptr->special_attack &= ~(ATTACK_ACID);
      msg_print("Your temporary acidic brand fades away.");
    }
  if ((p_ptr->special_attack & (ATTACK_ELEC)) && (attack_type != ATTACK_ELEC))
    {
      p_ptr->special_attack &= ~(ATTACK_ELEC);
      msg_print("Your temporary electrical brand fades away.");
    }
  if ((p_ptr->special_attack & (ATTACK_FIRE)) && (attack_type != ATTACK_FIRE))
    {
      p_ptr->special_attack &= ~(ATTACK_FIRE);
      msg_print("Your temporary fiery brand fades away.");
    }
  if ((p_ptr->special_attack & (ATTACK_COLD)) && (attack_type != ATTACK_COLD))
    {
      p_ptr->special_attack &= ~(ATTACK_COLD);
      msg_print("Your temporary frost brand fades away.");
    }
  if ((p_ptr->special_attack & (ATTACK_POIS)) && (attack_type != ATTACK_POIS))
    {
      p_ptr->special_attack &= ~(ATTACK_POIS);
      msg_print("Your temporary poison brand fades away.");
    }
  
  if ((duration) && (attack_type))
    {
      /* Set attack type. */
      p_ptr->special_attack |= (attack_type);
      
      /* Set duration. */
      p_ptr->ele_attack = duration;
      
      /* Message. */
      msg_format("For a while, the blows you deal will %s",
		 ((attack_type == ATTACK_ACID) ? "melt with acid!" :
		  ((attack_type == ATTACK_ELEC) ? "shock your foes!" :
		   ((attack_type == ATTACK_FIRE) ? "burn with fire!" : 
		    ((attack_type == ATTACK_COLD) ? "chill to the bone!" : 
		     ((attack_type == ATTACK_POIS) ? "poison your enemies!" : 
		      "do nothing special."))))));
    }
  
  /* Redraw the state */
  p_ptr->redraw |= (PR_STATUS);
  
  /* Handle stuff */
  handle_stuff();
}

/**
 * Proof an object against an element
 */
bool el_proof(u32b flag)
{
  object_type *o_ptr;

  int item;

  cptr q, s;

  /* Set the element */
  el_to_proof = flag;
  
  /* Only unproofed items */
  item_tester_hook = item_tester_unproofed;
  
  /* Don't restrict choices */
  item_tester_tval = 0;

  /* Get an item */
  q = "Proof which single item? ";
  s = "You have no single item to proof.";
  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) 
    return (FALSE);
  
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

  /* Proof it */
  o_ptr->flags_obj |= flag;

  /* Done */
  return (TRUE);
}

/**
 * Curse the players armor
 */
bool curse_armor(void)
{
  object_type *o_ptr;
  
  char o_name[120];
  
  int slot = INVEN_BODY;
  
  /* Curse the body armor */
  o_ptr = &inventory[INVEN_BODY];
  
  /* Nothing to curse */
  if (!o_ptr->k_idx) return (FALSE);
  
  /* Describe */
  object_desc(o_name, o_ptr, FALSE, 3);
  
  /* Attempt a saving throw for artifacts */
  if (artifact_p(o_ptr) && (rand_int(100) < 50))
    {
      /* Cool */
      msg_format("A %s tries to %s, but your %s resists the effects!",
		 "terrible black aura", "surround your armor", o_name);
    }
  
  /* not artifact or failed save... */
  else
    {
      int i, feel;
      bool heavy = FALSE;

      /* Oops */
      msg_format("A terrible black aura blasts your %s!", o_name);
      
      /* Try every curse */
      for (i = 0; i < OBJECT_RAND_SIZE_CURSE; i++)
	{
	  if (rand_int(100) < 10)
	    {
	      /* Try once */
	      o_ptr->flags_curse |= (1L << i);
	    }
	}

      /* Hack - no sticky curses on permacursed things */
      if (o_ptr->flags_obj & OF_PERMA_CURSE) 
	o_ptr->flags_curse &= ~(CF_STICKY_WIELD | CF_STICKY_CARRY);

      /* Not uncursed */
      o_ptr->ident &= ~(IDENT_UNCURSED | IDENT_KNOW_CURSES);

      /* Heavy sensing */
      heavy = (check_ability(SP_PSEUDO_ID_HEAVY));
      
      /* Type of feeling */
      feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
      
      /* Redo feeling */
      if (!(o_ptr->feel == feel))
	{
	  /* Get an object description */
	  object_desc(o_name, o_ptr, FALSE, 0);
	  
	  msg_format("You feel the %s (%c) you are %s %s now %s...",
		     o_name, index_to_label(slot), describe_use(slot),
		     ((o_ptr->number == 1) ? "is" : "are"), feel_text[feel]);
	  
	  /* We have "felt" it */
	  o_ptr->ident |= (IDENT_SENSE);
	  
	  /* Inscribe it textually */
	  o_ptr->feel = feel;
	  
	  /* Set squelch flag as appropriate */
	  p_ptr->notice |= PN_SQUELCH;
	}
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
      
      /* Recalculate mana */
      p_ptr->update |= (PU_MANA);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
    }
  
  return (TRUE);
}


/**
 * Curse the players weapon
 */
bool curse_weapon(void)
{
  object_type *o_ptr;
  
  char o_name[120];

  int slot = INVEN_WIELD;  
  
  /* Curse the weapon */
  o_ptr = &inventory[INVEN_WIELD];
  
  /* Nothing to curse */
  if (!o_ptr->k_idx) return (FALSE);
  
  
  /* Describe */
  object_desc(o_name, o_ptr, FALSE, 3);
  
  /* Attempt a saving throw */
  if (artifact_p(o_ptr) && (rand_int(100) < 50))
    {
      /* Cool */
      msg_format("A %s tries to %s, but your %s resists the effects!",
		 "terrible black aura", "surround your weapon", o_name);
    }
  
  /* not artifact or failed save... */
  else
    {
      int i, feel;
      bool heavy = FALSE;
      
      /* Oops */
      msg_format("A terrible black aura blasts your %s!", o_name);
      
      /* Try every curse */
      for (i = 0; i < OBJECT_RAND_SIZE_CURSE; i++)
	{
	  if (rand_int(100) < 10)
	    {
	      /* Try once */
	      o_ptr->flags_curse |= (1L << i);
	    }
	}

      /* Hack - no sticky curses on permacursed things */
      if (o_ptr->flags_obj & OF_PERMA_CURSE) 
	o_ptr->flags_curse &= ~(CF_STICKY_WIELD | CF_STICKY_CARRY);

      /* Not uncursed */
      o_ptr->ident &= ~(IDENT_UNCURSED | IDENT_KNOW_CURSES);

      /* Heavy sensing */
      heavy = (check_ability(SP_PSEUDO_ID_HEAVY));
      
      /* Type of feeling */
      feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
      
      /* Redo feeling */
      if (!(o_ptr->feel == feel))
	{
	  /* Get an object description */
	  object_desc(o_name, o_ptr, FALSE, 0);
	  
	  msg_format("You feel the %s (%c) you are %s %s now %s...",
		     o_name, index_to_label(slot), describe_use(slot),
		     ((o_ptr->number == 1) ? "is" : "are"), feel_text[feel]);
	  
	  /* We have "felt" it */
	  o_ptr->ident |= (IDENT_SENSE);
	  
	  /* Inscribe it textually */
	  o_ptr->feel = feel;
	  
	  /* Set squelch flag as appropriate */
	  p_ptr->notice |= PN_SQUELCH;
	}
      
     /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
      
      /* Recalculate mana */
      p_ptr->update |= (PU_MANA);
      
      /* Window stuff */
      p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
    }
  
  /* Notice */
  return (TRUE);
}


/**
 * Identify an object in the inventory, apart from curses.
 *
 * This routine returns TRUE if an item was identified.
 */
bool ident_spell(void)
{
  int item;
  int squelch=0;
  
  object_type *o_ptr;
  
  char o_name[120];
  
  cptr q, s;
  
  /* Only un-id'ed items */
  item_tester_hook = item_tester_unknown;
  
  /* Don't restrict choices */
  item_tester_tval = 0;
  
  /* Get an item.   */
  q = "Identify which item? ";
  s = "You have nothing to identify.";
  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) 
    return (FALSE);
  
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
  
  /* Identify it */
  identify_object(o_ptr);
 
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
  /* Handle stuff */
  handle_stuff();
  
  /* Description */
  object_desc(o_name, o_ptr, TRUE, 3);
  
  /* Possibly play a sound depending on object quality. */
  if (o_ptr->name1 != 0)
    {
      /* We have a good artifact. */
      sound(MSG_IDENT_ART);
    }
  else if (o_ptr->name2 != 0)
    {
      /* We have a good ego item. */
      sound(MSG_IDENT_EGO);
    }

  /* Describe */
  if (item >= INVEN_WIELD)
    {
      msg_format("%^s: %s (%c).",
		 describe_use(item), o_name, index_to_label(item));
    }
  else if (item >= 0)
    {
      msg_format("In your pack: %s (%c).  %s",
		 o_name, index_to_label(item),
		 ((squelch==1) ? "(Squelch)" :
		  ((squelch==-1) ? "(Squelch Failed)" : "")));
    }
  else
    {
      msg_format("On the ground: %s. %s", o_name, 
		 ((squelch==1) ? "(Squelch)" : ((squelch==-1) ? 
						"(Squelch Failed)" : "")));
      
    }
  
  /* If artifact, check for Set Item */
  if (o_ptr->name1)
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      if (a_ptr->set_no != 0)
	{
	  msg_print("This item is part of a set!");
	}
    }
  
  /* Success */
  return (TRUE);
}

/**
 * Identify an object in the inventory, including curses.
 *
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(void)
{
  int item;
  
  object_type *o_ptr;
  
  cptr q, s;
  
  /* Only un-*id*'ed items */
  item_tester_hook = item_tester_unknown_curse;
  
  /* Don't restrict choices */
  item_tester_tval = 0;
  
  /* Get an item.   */
  q = "Reveal curses on which item? ";
  s = "You have nothing to reveal curses on.";
  if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) 
    return (FALSE);
  
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
  
  /* Identify it */
  identify_object(o_ptr);

  /* Know the curses */
  o_ptr->id_curse = o_ptr->flags_curse;
  o_ptr->ident |= IDENT_KNOW_CURSES;
  if (!o_ptr->flags_curse) 
    {
      o_ptr->ident |= IDENT_UNCURSED;
      msg_print("This item has no curses.");
    }
  else object_info_screen(o_ptr, FALSE);
 
  /* Recalculate bonuses */
  p_ptr->update |= (PU_BONUS);
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER_0 | PW_PLAYER_1);
  
  /* Handle stuff */
  handle_stuff();
 
  /* Success */
  return (TRUE);
}

/**
 * Hook for "get_item()".  Determine if something is rechargable.
 */
static bool item_tester_hook_recharge(object_type *o_ptr)
{
  /* Recharge staffs */
  if (o_ptr->tval == TV_STAFF) return (TRUE);
  
  /* Recharge wands */
  if (o_ptr->tval == TV_WAND) return (TRUE);
  
  /* Recharge rods */
  if (o_ptr->tval == TV_ROD) return (TRUE);
  
  /* Nope */
  return (FALSE);
}


/**
 * Recharge a wand/staff/rod from the pack or on the floor.
 * This function has been rewritten in Oangband. -LM-
 *
 * Mage -- Recharge I --> recharge(85)
 * Mage -- Recharge II --> recharge(150)
 * Mage -- Recharge III --> recharge(220)
 *
 * Druid -- Infusion --> recharge(125)
 *
 * Priest or Necromancer -- Recharge --> recharge(140)
 *
 * Scroll of recharging --> recharge(130)
 * Scroll of *recharging* --> recharge(200)
 *
 * It is harder to recharge high level, and highly charged wands, 
 * staffs, and rods.  The more wands in a stack, the more easily and 
 * strongly they recharge.  Staffs, however, each get fewer charges if 
 * stacked.
 *
 * XXX XXX XXX Beware of "sliding index errors".
 */
bool recharge(int power)
{
  int item, lev;
  int recharge_strength, recharge_amount;
  
  object_type *o_ptr;
  object_kind *k_ptr;
  
  bool fail = FALSE;
  byte fail_type = 1;
  
  cptr q, s;
  char o_name[120];
  
  
  /* Only accept legal items */
  item_tester_hook = item_tester_hook_recharge;
  
   /* Don't restrict choices */
  item_tester_tval = 0;
  
 /* Get an item */
  q = "Recharge which item? ";
  s = "You have nothing to recharge.";
  if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return (FALSE);
  
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
  
  /* Extract the object "level" */
  lev = k_info[o_ptr->k_idx].level;
  
  
  /* Recharge a rod */
  if (o_ptr->tval == TV_ROD)
    {
      /* Extract a recharge strength by comparing object level to power. */
      recharge_strength = ((power > lev) ? (power - lev) : 0) / 5;
      
      
      /* Back-fire */
      if (rand_int(recharge_strength) == 0)
	{
	  /* Activate the failure code. */
	  fail = TRUE;
	}
      
      /* Recharge */
      else
	{
	  /* Recharge amount */
	  recharge_amount = (power * damroll(3, 2));
	  
	  /* Recharge by that amount */
	  if (o_ptr->timeout > recharge_amount)
	    o_ptr->timeout -= recharge_amount;
	  else
	    o_ptr->timeout = 0;
	}
    }

  
  /* Recharge wand/staff */
  else
    {
      /* Extract a recharge strength by comparing object level to power. 
       * Divide up a stack of wands' charges to calculate charge penalty.
       */
      if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
	recharge_strength = (100 + power - lev - 
			     (8 * o_ptr->pval / o_ptr->number)) / 15;
      
      /* All staffs, unstacked wands. */
      else recharge_strength = (100 + power - lev - 
				(8 * o_ptr->pval)) / 15;
      
      
      /* Back-fire */
      if ((recharge_strength < 0) || (rand_int(recharge_strength) == 0))
	{
	  /* Activate the failure code. */
	  fail = TRUE;
	}
      
      /* If the spell didn't backfire, recharge the wand or staff. */
      else
	{
	  /* Recharge based on the standard number of charges. */
	  recharge_amount = randint(1 + k_ptr->pval / 2);
	  
	  /* Multiple wands in a stack increase recharging somewhat. */
	  if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
	    {
	      recharge_amount += 
		(randint(recharge_amount * (o_ptr->number - 1))) / 2;
	      if (recharge_amount < 1) recharge_amount = 1;
	      if (recharge_amount > 12) recharge_amount = 12;
	    }
	  
	  /* But each staff in a stack gets fewer additional charges, 
	   * although always at least one.
	   */
	  if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1))
	    {
	      recharge_amount /= o_ptr->number;
	      if (recharge_amount < 1) recharge_amount = 1;
	    }
	  
	  /* Recharge the wand or staff. */
	  o_ptr->pval += recharge_amount;
	  
	  /* Hack - Artifacts have a maximum # of charges. */
	  if (artifact_p(o_ptr) && (o_ptr->pval > k_ptr->pval)) 
	    o_ptr->pval = k_ptr->pval;
	  
	  /* Hack -- we no longer "know" the item */
	  o_ptr->ident &= ~(IDENT_KNOWN);
	  
	  /* Hack -- we no longer think the item is empty */
	  o_ptr->ident &= ~(IDENT_EMPTY);
	}
    }
  
  
  /* Inflict the penalties for failing a recharge. */
  if (fail)
    {
      /* Artifacts are never destroyed. */
      if (artifact_p(o_ptr))
	{
	  object_desc(o_name, o_ptr, TRUE, 0);
	  msg_format("The recharging backfires - %s is completely drained!", 
		     o_name);
	  
	  /* Artifact rods. */
	  if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout < 10000)) 
	    o_ptr->timeout = (o_ptr->timeout + 100) * 2;
	  
	  /* Artifact wands and staffs. */
	  else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF)) 
	    o_ptr->pval = 0;
	}
      else 
	{
	  /* Get the object description */
	  object_desc(o_name, o_ptr, FALSE, 0);
	  
	  /*** Determine Seriousness of Failure ***/
	  
	  /* Mages recharge objects more safely. */
	  if (check_ability(SP_DEVICE_EXPERT))
	    {
	      /* 10% chance to blow up one rod, otherwise draining. */
	      if (o_ptr->tval == TV_ROD)
		{
		  if (randint(10) == 1) fail_type = 2;
		  else fail_type = 1;
		}
	      /* 67% chance to blow up one wand, otherwise draining. */
	      else if (o_ptr->tval == TV_WAND)
		{
		  if (randint(3) != 1) fail_type = 2;
		  else fail_type = 1;
		}
	      /* 50% chance to blow up one staff, otherwise no effect. */
	      else if (o_ptr->tval == TV_STAFF)
		{
		  if (randint(2) == 1) fail_type = 2;
		  else fail_type = 0;
		}
	    }
	  
	  /* All other classes get no special favors. */
	  else
	    {
	      /* 33% chance to blow up one rod, otherwise draining. */
	      if (o_ptr->tval == TV_ROD)
		{
		  if (randint(3) == 1) fail_type = 2;
		  else fail_type = 1;
		}
	      /* 20% chance of the entire stack, else destroy one wand. */
	      else if (o_ptr->tval == TV_WAND)
		{
		  if (randint(5) == 1) fail_type = 3;
		  else fail_type = 2;
		}
	      /* Blow up one staff. */
	      else if (o_ptr->tval == TV_STAFF)
		{
		  fail_type = 2;
		}
	    }
	  
	  /*** Apply draining and destruction. ***/
	  
	  /* Drain object or stack of objects. */
	  if (fail_type == 1)
	    {
	      if (o_ptr->tval == TV_ROD)
		{
		  msg_print("The recharge backfires, draining the rod further!");
		  if (o_ptr->timeout < 10000) 
		    o_ptr->timeout = (o_ptr->timeout + 100) * 2;
		}
	      else if (o_ptr->tval == TV_WAND)
		{
		  msg_format("You save your %s from destruction, but all charges are lost.", o_name);
		  o_ptr->pval = 0;
		}
	      /* Staffs aren't drained. */
	    }
	  
	  /* Destroy an object or one in a stack of objects. */
	  if (fail_type == 2)
	    {
	      if (o_ptr->number > 1)
		msg_format("Wild magic consumes one of your %s!", o_name);
	      else
		msg_format("Wild magic consumes your %s!", o_name);
	      
	      /* Reduce rod stack maximum timeout, drain wands. */
	      if (o_ptr->tval == TV_ROD) o_ptr->pval -= k_ptr->pval;
	      if (o_ptr->tval == TV_WAND) o_ptr->pval = 0;
	      
	      /* Reduce and describe inventory */
	      if (item >= 0)
		{
		  inven_item_increase(item, -1);
		  inven_item_describe(item);
		  inven_item_optimize(item);
		}
	      
	      /* Reduce and describe floor item */
	      else
		{
		  floor_item_increase(0 - item, -1);
		  floor_item_describe(0 - item);
		  floor_item_optimize(0 - item);
		}
	    }
	  
	  /* Destroy some members of a stack of objects. */
	  if (fail_type == 3)
	    {
	      int num_gone = -2;

	      if (o_ptr->number > 1)
		msg_format("Wild magic consumes some of your %s!", o_name);
	      else
		msg_format("Wild magic consumes your %s!", o_name);
	      
	      /* At least 2 gone, roll for others */
	      while ((o_ptr->number + num_gone) > 0)
		if (rand_int(4) == 0) num_gone--;

	      
	      /* Reduce and describe inventory */
	      if (item >= 0)
		{
		  inven_item_increase(item, num_gone);
		  inven_item_describe(item);
		  inven_item_optimize(item);
		}
	      
	      /* Reduce and describe floor item */
	      else
		{
		  floor_item_increase(0 - item, num_gone);
		  floor_item_describe(0 - item);
		  floor_item_optimize(0 - item);
		}
	    }
	}
    }
  
  /* Combine / Reorder the pack (later) */
  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
  
  /* Window stuff */
  p_ptr->window |= (PW_INVEN);
  
  /* Something was done */
  return (TRUE);
}


/**
 * Mages can get mana from magical objects at need. -LM-
 */
void tap_magical_energy(void)
{
  int item, lev;
  int energy = 0;
  
  object_type *o_ptr;
  
  cptr q, s;
  cptr item_name = "";
  
  
  /* Only accept legal items */
  item_tester_hook = item_tester_hook_recharge;
  
  /* Get an item */
  q = "Drain charges from which item? ";
  s = "You have nothing to drain charges from.";
  if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
  
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
  
  /* Extract the object "level" */
  lev = k_info[o_ptr->k_idx].level;
  
  /* Extract the object's energy and get its generic name. */
  if (o_ptr->tval == TV_ROD) 
    {
      /* Rods have little usable energy, for obvious balance reasons... */
      energy = (lev * o_ptr->number * 2)/3;
      
      /* No tapping rods with instant recharge */
      if (!(o_ptr->pval)) energy = 0;
      
      /* Modify Based on charged-ness */ 
      if (o_ptr->pval)
	energy = (energy * (o_ptr->pval-o_ptr->timeout))/ o_ptr->pval;
      item_name = "rod";
    }
  if (o_ptr->tval == TV_STAFF)
    {
      energy = (5 + lev) * o_ptr->pval;
      
      item_name = "staff";
    }
  if (o_ptr->tval == TV_WAND)
    {
      energy = (5 + lev) * 3 * o_ptr->pval / 2;
      
      item_name = "wand";
    }
  
  /* Turn energy into mana. */
  
  /* Require a resonable amount of energy */
  if (energy < 36)
    {
      /* Notify of failure. */
      msg_format("That %s had no useable energy", item_name);
    }
  else
    {
      /* If mana below maximum, increase mana and drain object. */
      if (p_ptr->csp < p_ptr->msp)
	{
	  /* Drain the object. */
	  if (o_ptr->tval == TV_ROD) o_ptr->timeout = o_ptr->pval;
	  else o_ptr->pval = 0;
	  

	  /* Combine / Reorder the pack (later) */
	  p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	  
	  /* Window stuff */
	  p_ptr->window |= (PW_INVEN);
	  
	  /* Increase mana. */
	  p_ptr->csp += energy / 12;
	  p_ptr->csp_frac = 0;
	  if (p_ptr->csp > p_ptr->msp) (p_ptr->csp = p_ptr->msp);
	  
	  msg_print("You feel your head clear.");
	  
	  p_ptr->redraw |= (PR_MANA);
	  p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	}
      
      /* Player is a smart cookie. */
      else 
	msg_format("Your mana was already at its maximum.  %^s not drained.", 
		   item_name);
    }
}


/**
 * Special code for staff of starlight and the Staff of Gandalf.  Most 
 * effective against monsters that start out in darkness, and against 
 * those who hate light. -LM-
 */
void do_starlight(int burst_number, int dam, bool strong)
{
  int i, j, y, x;
  
  /* Is the player in a square already magically lit? */
  bool player_lit = cave_info[p_ptr->py][p_ptr->px] & (CAVE_GLOW);
  
  for (i = 0; i < burst_number; i++)
    {
      /* First, we find the spot. */
      for (j = 0; j < 20; j++)
	{
	  /* Pick a (scattered) distance. */
	  int d = 2 + rand_int(4);
	  
	  /* Admit failure.  Switch to Plan B. */
	  if (j == 19)
	    {
	      y = p_ptr->py;
	      x = p_ptr->px;
	      break;
	    }
	  
	  /* Pick a location */
	  scatter(&y, &x, p_ptr->py, p_ptr->px, d, 0);
	  
	  /* Not on top of the player. */
	  if (cave_m_idx[y][x] < 0) continue;
	  
	  /* Require passable terrain */
	  if (!cave_passable_bold(y, x)) continue;
	  
	  /* Spot chosen. */
	  break;
	}
      
      /* Then we hit the spot. */
      
      /* Confusing to be suddenly lit up. */
      if (!(cave_info[y][x] & (CAVE_GLOW))) 
	fire_meteor(-1, GF_CONFUSION, y, x, dam, strong ? 1 : 0, FALSE);
      
      /* The actual burst of light. */
      fire_meteor(-1, GF_LITE_WEAK, y, x, dam, strong ? 2 : 1, FALSE);
      fire_meteor(-1, GF_LITE, y, x, dam, strong ? 1 : 0, FALSE);
      
      
      /* Hack - assume that the player's square is typical of the area, 
       * and only light those squares that weren't already magically lit 
       * temporarily.
       */
      if (!player_lit) 
	fire_meteor(-1, GF_DARK_WEAK, y, x, 0, strong ? 2 : 1, FALSE);
    }
  
  /* Hard not to notice. */
  add_wakeup_chance = 10000;
}


/**
 * Find some animals on the current dungeon level, and magic map the dungeon 
 * near them.  Learn about traps on the entire level if at least one natural 
 * creature is found. -LM-
 */
bool listen_to_natural_creatures(void)
{
  int i, y, x;
  int count = 0;
  
  /* Check all the monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Paranoia -- skip "dead" monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Only natural creatures are eligible, and some 
       * don't feel like talking. */
      if ((r_ptr->flags3 & (RF3_ANIMAL)) && (rand_int(2) == 0))
	{
	  /* Learn about their surroundings. */
	  map_area(m_ptr->fy, m_ptr->fx, FALSE);
	  
	  /* increment the count. */
	  count++;
	}
      
      /* Avoid excessive processing time. */
      if (count > 15) break;
    }
  
  /* No natural allies. */
  if (!count) return(FALSE);
  
  /* Find every trap on the level. */
  
  /* Scan all normal grids */
  for (y = 1; y < DUNGEON_HGT-1; y++)
    {
      /* Scan all normal grids */
      for (x = 1; x < DUNGEON_WID-1; x++)
	{
	  feature_type *f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Detect invisible traps */
	  if (f_ptr->flags & TF_TRAP_INVIS)
	    {
	      /* Pick a trap */
	      pick_trap(y, x);
	    }

	  /* Reset the feature */
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Detect traps */
	  if (f_ptr->flags & TF_TRAP)
	    {
	      /* Hack -- Memorize */
	      cave_info[y][x] |= (CAVE_MARK);
	      
	      /* Redraw */
	      lite_spot(y, x);
	    }
	}
    }
  
  /* Report success. */
  return(TRUE);
}

/**
 * Grow some trees and grass in player's line of sight
 */
void grow_trees_and_grass(bool powerful)
{
  int y, x;
  int py = p_ptr->py, px = p_ptr->px;

  /* Check everything in line of sight */
  for (y = py - 20; y <= py + 20; y++)
    for (x = px - 20; x <= px + 20; x++)
      {
	int dist = distance(py, px, y, x);

	/* Skip distant grids */
	if (dist > 20) continue;

	/* Skip grids the player can't see */
	if (!player_has_los_bold(y, x)) continue;

	/* Skip grids with objects */
	if ((cave_o_idx[y][x] > 0) && (!powerful)) continue;

	/* Skip grids that aren't floor */
	if (cave_feat[y][x] != FEAT_FLOOR) continue;

	/* Skip grids that have monsters */
	if ((cave_m_idx[y][x] > 0) && (!powerful)) continue;

	/* Maybe grow something */
	if ((rand_int(dist + 2) != 0) && (!powerful)) continue;

	/* Probably grass, otherwise a tree */
	if ((rand_int(4) == 0) || powerful)
	  {
	    if (p_ptr->depth < 40)
	      cave_set_feat(y, x, FEAT_TREE);
	    else
	      cave_set_feat(y, x, FEAT_TREE2);
	  }
	else cave_set_feat(y, x, FEAT_GRASS);
      }
  
  return;
}

/**
 * Only the bold and the desperate dare to unleash chaos. -LM-
 */
void unmake(int dir)
{
  byte chaotic_effect;
  int i;
  bool repeat = TRUE;
  
  while (repeat)
    {
      /* Pick an effect. */
      chaotic_effect = (byte)rand_int(18);
      
      switch (chaotic_effect)
	{
	  /* Massive chaos bolt. */
	case 0: 
	case 1: 
	case 2: 
	case 3: 
	case 4: 
	case 5: 
	case 6: 
	case 7:
	  {
	    fire_bolt(GF_CHAOS, dir, randint(500));
	    break;
	  }
	  /* Chaos balls in every directioon */
	case 8: 
	case 9:
	  {
	    for (i = 0; i < 8; i++) 
	      fire_ball(GF_CHAOS, ddd[i], randint(400), 2, FALSE);
	    break;
	  }
	  /* Tear up the dungeon. */
	case 10:
	  {
	    destroy_area(p_ptr->py, p_ptr->px, 5 + randint(20), TRUE);
	    break;
	  }
	  /* Chaos cloud right on top of the poor caster. */
	case 11:
	  {
	    fire_cloud(GF_CHAOS, 0, randint(400), 6);	
	    break;
	  }
	  /* Chaos spray. */
	case 12: 
	case 13: 
	case 14: 
	case 15: 
	case 16:
	  {
	    fire_arc(GF_CHAOS, dir, randint(600), 8, 90);
	    break;
	  }
	  /* Unmake the caster. */
	case 17:
	  {
	    (void)dec_stat(A_STR, 20, (rand_int(3) == 0));
	    (void)dec_stat(A_INT, 20, (rand_int(3) == 0));
	    (void)dec_stat(A_WIS, 20, (rand_int(3) == 0));
	    (void)dec_stat(A_DEX, 20, (rand_int(3) == 0));
	    (void)dec_stat(A_CON, 20, (rand_int(3) == 0));
	    (void)dec_stat(A_CHR, 20, (rand_int(3) == 0));
	    break;	
	  }
	}
      
      /* Chaos, once unleashed, likes to stay... */
      if (rand_int(4) == 0) repeat = TRUE;
      else repeat = FALSE;
    }
}


/**
 * Unleash the wrath of the beings of Air. -LM-
 */
void ele_air_smite(void)
{
  byte i, j;
  int y, x;
  
  /* Due warning. */
  msg_print("The powers of Air rain down destruction!");
  
  /* Multiple gravity, light, and electricity balls. */
  for (i = 0; i < 8; i++)
    {
      /* Select a legal nearby location at random. */
      for (j = 0; j < 20; j++)
	{
	  /* Pick a (short) distance. */
	  int d = randint(3);
	  
	  /* Admit failure.  Switch to Plan B. */
	  if (j == 19)
	    {
	      y = p_ptr->py;
	      x = p_ptr->px;
	      break;
	    }
	  /* Pick a location */
	  scatter(&y, &x, p_ptr->py, p_ptr->px, d, 0);
	  
	  /* Not on top of the player. */
	  if (cave_m_idx[y][x] < 0) continue;
	  
	  /* Require passable terrain */
	  if (!cave_passable_bold(y, x)) continue;
	  
	  /* Slight preference for actual monsters. */
	  if (cave_m_idx[y][x] > 0) break;
	  
	  /* Will accept any passable grid after a few tries. */
	  else if (j > 3) break;
	}
      
      if (rand_int(3) == 0) 
	(void)fire_meteor(-1, GF_GRAVITY, y, x, 100, 1, FALSE);
      else if (rand_int(2) == 0)
	(void)fire_meteor(-1, GF_LITE, y, x, 100, 1, FALSE);
      else
	(void)fire_meteor(-1, GF_ELEC, y, x, 100, 1, FALSE);
      
      
      /* This is a bombardment.  Make it look like one. */
      Term_xtra(TERM_XTRA_DELAY, 10);
    }
  
  /* I would /probably/ be awake at this point... */
  add_wakeup_chance = 10000;
}


/**
 * Apply a "project()" directly to all monsters in view of a certain spot.
 *
 * Note that affected monsters are NOT auto-tracked by this usage.
 *
 * This function is not optimized for efficieny.  It should only be used
 * in non-bottleneck functions such as spells. It should not be used in 
 * functions that are major code bottlenecks such as process monster 
 * or update_view. -JG
 */
bool project_los_not_player(int y1, int x1, int dam, int typ)
{
  int i, x, y;
  
  u32b flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
  
  bool obvious = FALSE;
  
  /* Affect all (nearby) monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Location */
      y = m_ptr->fy;
      x = m_ptr->fx;
      
      /*The LOS function doesn't do well with long distances*/
      if (distance(y1, x1, y, x) > MAX_RANGE) continue;
      
      /* Require line of sight or the monster being right on the square */
      if ((y != y1) || (x != x1))
	{
	  
	  if (!los(y1, x1, y, x)) continue;
	  
	}
      
      /* Jump directly to the target monster */
      if (project(-1, 0, y, x, dam, typ, flg,0 ,0)) obvious = TRUE;
    }
  
  /* Result */
  return (obvious);
}


/**
 * Apply a "project()" directly to all viewable monsters
 *
 * Note that affected monsters are NOT auto-tracked by this usage.
 */
static bool project_hack(int typ, int dam)
{
  int i, x, y;
  
  int flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
  
  bool obvious = FALSE;
  
  
  /* Affect all (nearby) monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Location */
      y = m_ptr->fy;
      x = m_ptr->fx;
      
      /* Require line of sight */
      if (!player_has_los_bold(y, x)) continue;
      
      /* Jump directly to the target monster */
      if (project(-1, 0, y, x, dam, typ, flg, 0, 0)) obvious = TRUE;
    }
  
  /* Result */
  return (obvious);
}


/**
 * Speed monsters
 */
bool speed_monsters(void)
{
  return (project_hack(GF_OLD_SPEED, p_ptr->lev));
}

/**
 * Slow monsters
 */
bool slow_monsters(int dam)
{
  return (project_hack(GF_OLD_SLOW, dam));
}

/**
 * Sleep monsters
 */
bool sleep_monsters(int dam)
{
  return (project_hack(GF_OLD_SLEEP, dam));
}

/**
 * Frighten monsters. -LM-
 */
bool fear_monsters(int dam)
{
  return (project_hack(GF_TURN_ALL, dam));
}

/**
 * Confuse monsters. -LM-
 */
bool confu_monsters(int dam)
{
  return (project_hack(GF_OLD_CONF, dam));
}

/**
 * Banish evil monsters
 */
bool banish_evil(int dist)
{
  return (project_hack(GF_AWAY_EVIL, dist));
}


/**
 * Turn undead
 */
bool turn_undead(int dam)
{
  return (project_hack(GF_TURN_UNDEAD, dam));
}

/**
 * Turn evil. -LM-
 */
bool turn_evil(int dam)
{
  return (project_hack(GF_TURN_EVIL, dam));
}

/**
 * Dispel undead monsters
 */
bool dispel_undead(int dam)
{
  return (project_hack(GF_DISP_UNDEAD, dam));
}

/**
 * Dispel evil monsters
 */
bool dispel_evil(int dam)
{
  return (project_hack(GF_DISP_EVIL, dam));
}

/**
 * Dispel demons
 */
bool dispel_demons(int dam)
{
  return (project_hack(GF_DISP_DEMON, dam));
}

/**
 * Dispel non-evil monsters
 */
bool dispel_not_evil(int dam)
{
  return (project_hack(GF_DISP_NOT_EVIL, dam));
}

/**
 * Dispel all monsters
 */
bool dispel_monsters(int dam)
{
  return (project_hack(GF_DISP_ALL, dam));
}

/**
 * Dispel all small monsters
 */
bool dispel_small_monsters(int dam)
{
  return (project_hack(GF_DISP_SMALL_ALL, dam));
}

/**
 * Dispel all living creatures.   From Sangband.
 */
bool dispel_living(int dam)
{
  return (project_hack(GF_SPIRIT, dam));
}

/**
 * Dispel monsters who can't stand bright light. -LM-
 */
bool dispel_light_hating(int dam)
{
  return (project_hack(GF_LITE_WEAK, dam));
}

/** 
 * Trees and grass hurt monsters
 */
bool nature_strike(int dam)
{
  return (fire_meteor(-1, GF_NATURE, p_ptr->py, p_ptr->px, dam, 5, FALSE));
}


/**
 * Put undead monsters into stasis. -LM-
 */
bool hold_undead(void)
{
  return (project_hack(GF_HOLD_UNDEAD, 0));
}

/**
 * Put all monsters into stasis. 
 */
bool hold_all(void)
{
  return (project_hack(GF_HOLD, 0));
}

/**
 * Put all monsters into stasis. 
 */
bool poly_all(int dam)
{
  return (project_hack(GF_OLD_POLY, dam));
}

/**
 * Put all monsters into stasis. 
 */
bool teleport_all(int dam)
{
  return (project_hack(GF_AWAY_ALL, dam));
}

/**
 * Sound blast monsters in line of sight.
 */
bool cacophony(int dam)
{
  return (project_hack(GF_SOUND, dam));
}

/**
 * Wake up all monsters, and speed up "los" monsters.
 */
bool aggravate_monsters(int who, bool the_entire_level)
{
  int i;
  
  bool sleep = FALSE;
  bool known = FALSE;
  
  /* Aggravate everyone nearby */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Skip aggravating monster (or player) */
      if (i == who) continue;
      
      /* Wake up and hasten all monsters. No additional messages. */
      if (the_entire_level)
	{
	  /* Wake up */
	  if (m_ptr->csleep)
	    {
	      /* Wake up */
	      m_ptr->csleep = 0;
	    }
	  
	  /* Go active */
	  m_ptr->mflag |= (MFLAG_ACTV);
	  
	  /* Get mad. */
	  if (m_ptr->mspeed < r_ptr->speed + 10) 
	    m_ptr->mspeed = r_ptr->speed + 10;
	}
      
      /* Standard aggravation */
      else 
	{
	  /* Wake up nearby sleeping monsters */
	  if (m_ptr->cdis < (p_ptr->themed_level ? 
			     MAX_SIGHT : MAX_SIGHT * 2))
	    {
	      /* Wake up */
	      if (m_ptr->csleep)
		{
		  /* Wake up */
		  m_ptr->csleep = 0;
		  sleep = TRUE;
		  
		  /* Do not necessarily go active */
		}

	      /* Random equipment aggravation */
	      else if (p_ptr->rand_aggro)
		{
		  /* Go active */
		  m_ptr->mflag |= (MFLAG_ACTV);
		  
		  /* Get mad. */
		  if (m_ptr->mspeed < r_ptr->speed + 10) 
		    m_ptr->mspeed = r_ptr->speed + 10;
		}

	      /* Know we've aggravated */
	      known = TRUE;
	    }
	}
    }
  
  /* Messages */
  if (sleep) msg_print("You hear a sudden stirring in the distance!");

  return (known);
}



/**
 * Delete all non-unique monsters of a given "type" from the level
 */
bool genocide(void)
{
  int i;
  
  char typ;
  
  
  /* Mega-Hack -- Get a monster symbol */

  if (!get_com("Choose a monster race (by symbol) to genocide: ", &typ))
    return (FALSE);
  
  /* Delete the monsters of that "type" */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Hack -- Skip Unique Monsters */
      if (r_ptr->flags1 & (RF1_UNIQUE)) continue;
      
      /* Skip "wrong" monsters */
      if (r_ptr->d_char != typ) continue;
      
      /* Ignore monsters in icky squares */
      if ((cave_info[m_ptr->fy][m_ptr->fx] & CAVE_ICKY) == CAVE_ICKY) continue;
      
      /* Delete the monster */
      delete_monster_idx(i);
      
      /* Take some damage */
      take_hit(randint(4), "the strain of casting Genocide");
    }
  
  return (TRUE);
}


/**
 * Delete all nearby (non-unique) monsters
 */
bool mass_genocide(void)
{
  int i;
  
  /* Delete the (nearby) monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Hack -- Skip unique monsters */
      if (r_ptr->flags1 & (RF1_UNIQUE)) continue;
      
      /* Skip distant monsters */
      if (m_ptr->cdis > MAX_SIGHT) continue;
      
      /* Ignore monsters in icky squares */
      if ((cave_info[m_ptr->fy][m_ptr->fx] & CAVE_ICKY) == CAVE_ICKY) continue;
      
      /* Delete the monster */
      delete_monster_idx(i);
      
      /* Take some damage */
      take_hit(randint(3), "the strain of casting Mass Genocide");
    }
  
  return (TRUE);
}



/**
 * Probe nearby monsters
 */
bool probing(void)
{
  int i;
  
  bool probe = FALSE;
  
  
  /* Probe all (nearby) monsters */
  for (i = 1; i < m_max; i++)
    {
      monster_type *m_ptr = &m_list[i];
      monster_race *r_ptr = &r_info[m_ptr->r_idx];
      
      /* Paranoia -- Skip dead monsters */
      if (!m_ptr->r_idx) continue;
      
      /* Require line of sight */
      if (!player_has_los_bold(m_ptr->fy, m_ptr->fx)) continue;
      
      /* Probe visible monsters */
      if (m_ptr->ml)
	{
	  char m_name[80];
	  
	  /* Start the message */
	  if (!probe) msg_print("Probing...");
	  
	  /* Get "the monster" or "something" */
	  monster_desc(m_name, m_ptr, 0x04);
	  
	  /* Describe the monster */
	  if (!(r_ptr->mana))
	    msg_format("%^s has %d hit points.", m_name, m_ptr->hp);
	  else
	    msg_format("%^s has %d hit points and %d mana.", 
		       m_name, m_ptr->hp, m_ptr->mana);
	  
	  /* Learn all of the non-spell, non-treasure flags */
	  lore_do_probe(i);
	  
	  /* Probe worked */
	  probe = TRUE;
	}
    }
  
  /* Done */
  if (probe)
    {
      msg_print("That's all.");
    }
  
  /* Result */
  return (probe);
}



/**
 * The spell of destruction
 *
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 */
void destroy_area(int y1, int x1, int r, bool full)
{
  int y, x, k, t;
  
  bool flag = FALSE;
  
  
  /* XXX XXX */
  full = full ? full : 0;
  
  /* Big area of affect */
  for (y = (y1 - r); y <= (y1 + r); y++)
    {
      for (x = (x1 - r); x <= (x1 + r); x++)
	{
	  /* Skip illegal grids */
	  if (!in_bounds_fully(y, x)) continue;
	  
	  /* Extract the distance */
	  k = distance(y1, x1, y, x);
	  
	  /* Stay in the circle of death */
	  if (k > r) continue;
	  
	  /* Ignore icky squares */
	  if ((cave_info[y][x] & CAVE_ICKY) == CAVE_ICKY) continue;
	  
	  /* Lose room and vault */
	  cave_info[y][x] &= ~(CAVE_ROOM);
	  
	  /* Lose light and knowledge */
	  cave_info[y][x] &= ~(CAVE_GLOW | CAVE_MARK);
	  
	  /* Hack -- Notice player affect */
	  if (cave_m_idx[y][x] < 0)
	    {
	      /* Hurt the player later */
	      flag = TRUE;
	      
	      /* Do not hurt this grid */
	      continue;
	    }
	  
	  /* Hack -- Skip the epicenter */
	  if ((y == y1) && (x == x1)) continue;
	  
	  /* Delete the monster (if any) */
	  delete_monster(y, x);
	  
	  /* Destroy "valid" grids */
	  if (cave_valid_bold(y, x))
	    {
	      int feat = FEAT_FLOOR;
	      feature_type *f_ptr = &f_info[cave_feat[y][x]];
	      
	      /* Delete objects */
	      delete_object(y, x);
	      
	      /* Decrement the trap or rune count. */
	      if (f_ptr->flags & TF_M_TRAP)
		num_trap_on_level--;
	      else if (f_ptr->flags & TF_RUNE)
		num_runes_on_level[cave_feat[y][x] - FEAT_RUNE_HEAD]--;
	      
	      
	      /* Wall (or floor) type */
	      t = rand_int(200);
	      
	      /* Granite */
	      if (t < 20)
		{
		  /* Create granite wall */
		  feat = FEAT_WALL_EXTRA;
		}
	      
	      /* Quartz */
	      else if (t < 70)
		{
		  /* Create quartz vein */
		  feat = FEAT_QUARTZ;
		}
	      
	      /* Magma */
	      else if (t < 100)
		{
		  /* Create magma vein */
		  feat = FEAT_MAGMA;
		}
	      
	      /* Change the feature */
	      cave_set_feat(y, x, feat);
	    }
	}
    }
  
  
  /* Hack -- Affect player */
  if (flag)
    {
      /* Message */
      msg_print("There is a searing blast of light!");
      
      /* Blind the player */
      if (!p_ptr->no_blind && !p_resist_good(P_RES_LITE))
	{
	  /* Become blind */
	  (void)set_blind(p_ptr->blind + 10 + randint(10));
	}
      else 
	{
	  notice_other(IF_RES_LITE, 0);
	  notice_obj(OF_SEEING, 0);
	}
    }
  
  
  /* Fully update the visuals */
  p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
  
  /* Redraw map */
  p_ptr->redraw |= (PR_MAP);
  
  /* Window stuff */
  p_ptr->window |= (PW_OVERHEAD);
}


/**
 * Induce an "earthquake" of the given radius at the given location.
 *
 * This will turn some walls into floors and some floors into walls.
 *
 * The player will take damage and "jump" into a safe grid if possible,
 * otherwise, he will "tunnel" through the rubble instantaneously.
 *
 * Monsters will take damage, and "jump" into a safe grid if possible,
 * otherwise they will be "buried" in the rubble, disappearing from
 * the level in the same way that they do when genocided.
 *
 * Note that players and monsters (except eaters of walls and passers
 * through walls) will never occupy the same grid as a wall (or door).
 */
void earthquake(int cy, int cx, int r, bool volcano)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int i, t, y, x, yy, xx, dy, dx;
  
  int damage = 0;
  
  int sn = 0, sy = 0, sx = 0;

  int lava = 0, water = 0, abyss = 0, total = 0;
  
  bool hurt = FALSE;
  
  bool map[32][32];
  
  
  /* Paranoia -- Enforce maximum range */
  if (r > 12) r = 12;
  
  /* Clear the "maximal blast" area */
  for (y = 0; y < 32; y++)
    {
      for (x = 0; x < 32; x++)
	{
	  map[y][x] = FALSE;
	}
    }
  
  /* Check around the epicenter */
  for (dy = -r; dy <= r; dy++)
    {
      for (dx = -r; dx <= r; dx++)
	{
	  /* Measure of terrain stability */
	  int unstable = 0;
	  
	  /* Extract the location */
	  yy = cy + dy;
	  xx = cx + dx;
	  
	  /* Skip illegal grids */
	  if (!in_bounds_fully(yy, xx)) continue;
	  
	  /* Skip distant grids */
	  if (distance(cy, cx, yy, xx) > r) continue;
	  
	  /* Lose room */
	  cave_info[yy][xx] &= ~(CAVE_ROOM);
	  
	  /* Lose light and knowledge */
	  cave_info[yy][xx] &= ~(CAVE_GLOW | CAVE_MARK);

	  /* Count total, water, lava and void grids */
	  total++;
	  if (cave_feat[yy][xx] == FEAT_WATER) 
	    {
	      water++;
	      unstable++;
	    }
	  if (cave_feat[yy][xx] == FEAT_LAVA) 
	    {
	      lava++;
	      unstable++;
	    }
	  if (cave_feat[yy][xx] == FEAT_VOID) 
	    {
	      abyss++;
	      unstable++;
	    }
	  
	  /* Skip the epicenter */
	  if (!dx && !dy) continue;
	  
	  /* Skip most grids, less if unstable */
	  if (rand_int(100) < (75 - (50 * unstable)/total)) continue;
	  
	  /* Damage this grid */
	  map[16 + yy - cy][16 + xx - cx] = TRUE;
	  
	  /* Hack -- Take note of player damage */
	  if ((yy == py) && (xx == px)) hurt = TRUE;
	}
    }
  
  /* First, affect the player (if necessary) */
  if (hurt)
    {
      /* Check around the player */
      for (i = 0; i < 8; i++)
	{
	  /* Access the grid */
	  y = py + ddy_ddd[i];
	  x = px + ddx_ddd[i];
	  
	  /* Skip non-empty grids */
	  if (!cave_empty_bold(y, x)) continue;
	  
	  /* Important -- Skip "quake" grids */
	  if (map[16 + y - cy][16 + x - cx]) continue;
	  
	  /* Count "safe" grids, apply the randomizer */
	  if ((++sn > 1) && (rand_int(sn) != 0)) continue;
	  
	  /* Save the safe location */
	  sy = y; sx = x;
	}
      
      if (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
	{
	  /* Random message */
	  switch (randint(3))
	    {
	    case 1:
	      {
		msg_print("The cave ceiling collapses!");
		break;
	      }
	    case 2:
	      {
		msg_print("The cave floor twists in an unnatural way!");
		break;
	      }
	    default:
	      {
		msg_print("The cave quakes!");
		msg_print("You are pummeled with debris!");
		break;
	      }
	    }
	}
      else
	{
	  /* Random message */
	  switch (randint(3))
	    {
	    case 1:
	      {
		msg_print("There is a mighty upheaval of the earth!");
		break;
	      }
	    case 2:
	      {
		msg_print("The ground twists in an unnatural way!");
		break;
	      }
	    default:
	      {
		msg_print("The ground quakes!");
		msg_print("You are pummeled with debris!");
		break;
	      }
	    }
	}
      
      
      /* Hurt the player a lot */
      if (!sn)
	{
	  /* Message and damage */
	  msg_print("You are severely crushed!");
	  damage = damroll(5, 80);
	}
      
      /* Destroy the grid, and push the player to safety */
      else
	{
	  /* Calculate results */
	  switch (randint(3))
	    {
	    case 1:
	      {
		msg_print("You nimbly dodge the blast!");
		damage = 0;
		break;
	      }
	    case 2:
	      {
		msg_print("You are bashed by rubble!");
		damage = damroll(10, 4);
		(void)set_stun(p_ptr->stun + randint(50));
		break;
	      }
	    case 3:
	      {
		/* Chance of falling */
		if (abyss > randint(total))
		  {
		    fall_off_cliff();
		  }
		else
		  {
		    msg_print("You are crushed!");
		    damage = damroll(10, 8);
		    (void)set_stun(p_ptr->stun + randint(50));
		    break;
		  }
	      }
	    }
	  
	  /* Move player */
	  monster_swap(py, px, sy, sx);
	}
      
      /* Take some damage */
      if (damage) take_hit(damage, "an earthquake");
    }
  
  
  /* Examine the quaked region */
  for (dy = -r; dy <= r; dy++)
    {
      for (dx = -r; dx <= r; dx++)
	{
	  /* Extract the location */
	  yy = cy + dy;
	  xx = cx + dx;
	  
	  /* Skip unaffected grids */
	  if (!map[16+yy-cy][16+xx-cx]) continue;
	  
	  /* Process monsters */
	  if (cave_m_idx[yy][xx] > 0)
	    {
	      monster_type *m_ptr = &m_list[cave_m_idx[yy][xx]];
	      monster_race *r_ptr = &r_info[m_ptr->r_idx];
	      
	      /* Most monsters cannot co-exist with rock */
	      if (!(r_ptr->flags2 & (RF2_KILL_WALL)) &&
		  !(r_ptr->flags2 & (RF2_PASS_WALL)))
		{
		  char m_name[80];

		  /* Assume not safe */
		  sn = 0;
		  
		  /* Monster can move to escape the wall */
		  if (!(r_ptr->flags1 & (RF1_NEVER_MOVE)))
		    {
		      /* Look for safety */
		      for (i = 0; i < 8; i++)
			{
			  /* Access the grid */
			  y = yy + ddy_ddd[i];
			  x = xx + ddx_ddd[i];
			  
			  /* Skip non-empty grids */
			  if (!cave_empty_bold(y, x)) continue;
			  
			  /* Hack -- no safety on glyph of warding */
			  if (cave_feat[y][x] == FEAT_RUNE_PROTECT) continue;
			  
			  /* Important -- Skip "quake" grids */
			  if (map[16+y-cy][16+x-cx]) continue;
			  
			  /* Count "safe" grids, apply the randomizer */
			  if ((++sn > 1) && (rand_int(sn) != 0)) continue;
			  
			  /* Save the safe grid */
			  sy = y;
			  sx = x;
			}
		    }
		  
		  /* Describe the monster */
		  monster_desc(m_name, m_ptr, 0);
		  
		  /* Scream in pain */
		  msg_format("%^s wails out in pain!", m_name);
		  
		  /* Take damage from the quake */
		  damage = (sn ? damroll(4, 8) : damroll(5, 80));
		  
		  /* Monster is certainly awake */
		  m_ptr->csleep = 0;
		  
		  /* Go active */
		  m_ptr->mflag |= (MFLAG_ACTV);
		  
		  /* Apply damage directly */
		  m_ptr->hp -= damage;
		  
		  /* Delete (not kill) "dead" monsters */
		  if (m_ptr->hp < 0)
		    {
		      /* Message */
		      msg_format("%^s is embedded in the rock!", m_name);
		      
		      /* Delete the monster */
		      delete_monster(yy, xx);
		      
		      /* No longer safe */
		      sn = 0;
		    }
		  
		  /* Hack -- Escape from the rock */
		  if (sn)
		    {
		      /* Move the monster */
		      monster_swap(yy, xx, sy, sx);
		    }
		}
	    }
	}
    }
  
  
  /* XXX XXX XXX */
  
  /* New location */
  py = p_ptr->py;
  px = p_ptr->px;
  
  /* Important -- no wall on player */
  map[16 + py - cy][16 + px - cx] = FALSE;
  
  
  /* Examine the quaked region */
  for (dy = -r; dy <= r; dy++)
    {
      for (dx = -r; dx <= r; dx++)
	{
	  /* Extract the location */
	  yy = cy + dy;
	  xx = cx + dx;
	  
	  /* Skip unaffected grids */
	  if (!map[16 + yy - cy][16 + xx - cx]) continue;
	  
	  /* Paranoia -- never affect player */
	  if ((yy == py) && (xx == px)) continue;
	  
	  /* Destroy location (if valid).   Increment trap/glyph count. */
	  if (cave_valid_bold(yy, xx))
	    {
	      int feat = FEAT_FLOOR;
	      
	      bool floor = cave_floor_bold(yy, xx);
	      
	      monster_type *m_ptr = &m_list[cave_m_idx[yy][xx]];
	      monster_race *r_ptr = &r_info[m_ptr->r_idx];
	      feature_type *f_ptr = &f_info[cave_feat[yy][xx]];
		      
	      /* Allow more things to be destroyed outside */
	      if (stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
		floor = TRUE;

	      /* Delete objects */
	      delete_object(yy, xx);
	      
	      /* Hack -- Increment the trap or glyph count. */
	      if (f_ptr->flags & TF_M_TRAP)
		num_trap_on_level--;
	      else if (f_ptr->flags & TF_RUNE)
		num_runes_on_level[cave_feat[y][x] - FEAT_RUNE_HEAD]--;
	      
	      /* Wall (or floor) type */
	      t = (floor ? rand_int(120) : 200);
	      
	      
	      /* Granite (rubble if monster is present) */
	      if (t < 20)
		{
		  /* Dump rubble on top of monsters. */
		  if (cave_m_idx[yy][xx] > 0) feat = FEAT_RUBBLE;
		  
		  /* Otherwise, create granite wall */
		  else feat = FEAT_WALL_EXTRA;
		}
	      
	      /* Quartz */
	      else if (t < 55)
		{
		  /* Dump rubble on top of monsters. */
		  if (cave_m_idx[yy][xx] > 0) feat = FEAT_RUBBLE;
		  
		  /* If this was a volcanic eruption, create lava near 
		   * center. -LM-
		   */
		  else if ((volcano) && (distance(cy, cx, yy, xx) < 3)) 
		    feat = FEAT_LAVA;
		  
		  /* Otherwise, create quartz vein */
		  else feat = FEAT_QUARTZ;
		}
	      
	      /* Magma */
	      else if (t < 90)
		{
		  /* Dump rubble on top of monsters. */
		  if (cave_m_idx[yy][xx] > 0) feat = FEAT_RUBBLE;
		  
		  /* If this was a volcanic eruption, create lava near 
		   * center. -LM-
		   */
		  else if ((volcano) && (distance(cy, cx, yy, xx) < 3)) 
		    feat = FEAT_LAVA;
		  
		  /* Otherwise, create magma vein */
		  else feat = FEAT_MAGMA;
		}
	      
	      /* Rubble. */
	      else if (t < 120)
		{
		  /* Create rubble */
		  feat = FEAT_RUBBLE;
		}

	      /* Override with water/lava/void */
	      t *= total;

	      /* Water */
	      if (t < water * 150)
		{
		  /* Monster OK in water */
		  if ((r_ptr->flags2 & (RF2_FLYING)) ||
	  	  !((r_ptr->flags4 & (RF4_BRTH_FIRE)) || 
		    (strchr("uU", r_ptr->d_char)) || 
		    ((strchr("E", r_ptr->d_char)) && 
		     ((r_ptr->d_attr == TERM_RED) || 
		      (r_ptr->d_attr == TERM_L_RED)))))
		    
		    /* Create water */
		    feat = FEAT_WATER;
		}
	      else if (t < lava * 150)
		{
		  /* Monster OK in lava */
		  if ((r_ptr->flags3 & (RF3_IM_FIRE)) ||
		      ((r_ptr->flags2 & (RF2_FLYING)) && 
		       ((r_ptr->flags1 & (RF1_FORCE_MAXHP) ?
			 (r_ptr->hdice * r_ptr->hside) :
			 (r_ptr->hdice * (r_ptr->hside + 1) / 2)) > 49)))
		    
		    /* Create lava */
		    feat = FEAT_LAVA;
		}
	      else if (t < abyss * 150)
		{
		  /* Check for monsters */
		  if (cave_m_idx[yy][xx] > 0) 
		    {
		      /* Flying monsters survive */
		      if (!(r_ptr->flags2 & (RF2_FLYING)))
			{
			  /* What was that again ? */
			  char m_name[80];
		      
			  /* Extract monster name */
			  monster_desc(m_name, m_ptr, 0);
			  
			  /* There it goes... */
			  msg_format("%s falls into the dark!", m_name);
			  
			  /* Gone, precious */
			  delete_monster(y, x);
			}
		    }

		  /* Create void */
		  feat = FEAT_VOID;
		}

	      /* Change the feature */
	      cave_set_feat(yy, xx, feat);
	    }
	}
    }


  /* Fully update the visuals */
  p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
  
  /* Redraw map */
  p_ptr->redraw |= (PR_MAP);
  
  /* Update the health and mana bars */
  p_ptr->redraw |= (PR_HEALTH | PR_MON_MANA);
  
  /* Window stuff */
  p_ptr->window |= (PW_OVERHEAD);
}

/** 
 * Small targetable earthquake
 */
bool tremor(void)
{
  int ny;
  int nx;
  bool okay = FALSE;
  bool valid_grid = FALSE;

  /* Choose the epicentre */
  while (!valid_grid)
    {
      okay = target_set_interactive(TARGET_LOOK | TARGET_GRID);
      if (!okay) return (FALSE);

      /* grab the target coords. */
      ny = p_ptr->target_row;
      nx = p_ptr->target_col;
      
      /* Test for empty floor and line of sight, forbid vaults */
      if (cave_empty_bold(ny,nx) && !(cave_info[ny][nx] & CAVE_ICKY) &&
	  (player_has_los_bold(ny, nx)))
	valid_grid = TRUE;
    }

  /* Shake the Earth */
  earthquake(ny, nx, 3, FALSE);

  /* Success */
  return (TRUE);  
}

/**
 * This routine clears the entire "temp" set.
 *
 * This routine will Perma-Lite all "temp" grids.
 *
 * This routine is used (only) by "lite_room()"
 *
 * Dark grids are illuminated.
 *
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 */
static void cave_temp_room_lite(void)
{
  int i;
  
  /* Apply flag changes */
  for (i = 0; i < temp_n; i++)
    {
      int y = temp_y[i];
      int x = temp_x[i];
      
      /* No longer in the array */
      cave_info[y][x] &= ~(CAVE_TEMP);
      
      /* Perma-Lite */
      cave_info[y][x] |= (CAVE_GLOW);
    }
  
  /* Fully update the visuals */
  p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
  
  /* Update stuff */
  update_stuff();
  
  /* Process the grids */
  for (i = 0; i < temp_n; i++)
    {
      int y = temp_y[i];
      int x = temp_x[i];
      
      /* Redraw the grid */
      lite_spot(y, x);
      
      /* Process affected monsters */
      if (cave_m_idx[y][x] > 0)
	{
	  int chance = 25;
	  
	  monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
	  monster_race *r_ptr = &r_info[m_ptr->r_idx];
	  
	  /* Stupid monsters rarely wake up */
	  if (r_ptr->flags2 & (RF2_STUPID)) chance = 10;
	  
	  /* Smart monsters always wake up */
	  if (r_ptr->flags2 & (RF2_SMART)) chance = 100;
	  
	  /* Sometimes monsters wake up */
	  if (m_ptr->csleep && (rand_int(100) < chance))
	    {
	      /* Wake up! */
	      m_ptr->csleep = 0;
	      
	      /* Go active */
	      m_ptr->mflag |= (MFLAG_ACTV);
	      
	      /* Notice the "waking up" */
	      if (m_ptr->ml)
		{
		  char m_name[80];
		  
		  /* Acquire the monster name */
		  monster_desc(m_name, m_ptr, 0);
		  
		  /* Dump a message */
		  msg_format("%^s wakes up.", m_name);
		}
	    }
	}
    }
  
  /* None left */
  temp_n = 0;
}



/**
 * This routine clears the entire "temp" set.
 *
 * This routine will "darken" all "temp" grids.
 *
 * In addition, some of these grids will be "unmarked".
 *
 * This routine is used (only) by "unlite_room()"
 */
static void cave_temp_room_unlite(void)
{
  int i;
  
  /* Apply flag changes */
  for (i = 0; i < temp_n; i++)
    {
      int y = temp_y[i];
      int x = temp_x[i];
      
      /* No longer in the array */
      cave_info[y][x] &= ~(CAVE_TEMP);
      
      /* Darken the grid */
      cave_info[y][x] &= ~(CAVE_GLOW);
      
      /* Hack -- Forget "boring" grids */
      if (cave_feat[y][x] <= FEAT_INVIS)
	{
	  /* Forget the grid */
	  cave_info[y][x] &= ~(CAVE_MARK);
	}
    }
  
  /* Fully update the visuals */
  p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_MONSTERS);
  
  /* Update stuff */
  update_stuff();
  
  /* Process the grids */
  for (i = 0; i < temp_n; i++)
    {
      int y = temp_y[i];
      int x = temp_x[i];
      
      /* Redraw the grid */
      lite_spot(y, x);
    }
  
  /* None left */
  temp_n = 0;
}




/**
 * Aux function -- see below
 */
static void cave_temp_room_aux(int y, int x)
{
  /* Check in bounds - thanks George */
  if (!in_bounds(y, x)) return;

  /* Avoid infinite recursion */
  if (cave_info[y][x] & (CAVE_TEMP)) return;
  
  /* Do not "leave" the current room */
  if (!(cave_info[y][x] & (CAVE_ROOM))) return;
  
  /* Paranoia -- verify space */
  if (temp_n == TEMP_MAX) return;
  
  /* Mark the grid as "seen" */
  cave_info[y][x] |= (CAVE_TEMP);
  
  /* Add it to the "seen" set */
  temp_y[temp_n] = y;
  temp_x[temp_n] = x;
  temp_n++;
}




/**
 * Illuminate any room containing the given location.
 */
void lite_room(int y1, int x1)
{
  int i, x, y;
  feature_type *f_ptr = NULL;
  
  /* Add the initial grid */
  cave_temp_room_aux(y1, x1);
  
  /* While grids are in the queue, add their neighbors */
  for (i = 0; i < temp_n; i++)
    {
      x = temp_x[i], y = temp_y[i];
      
      /* Walls (but not trees) get lit, but stop light */
      f_ptr = &f_info[cave_feat[y][x]];
      if ((!cave_project(y, x)) && (f_ptr->flags & TF_TREE)) continue;
      
      /* Spread adjacent */
      cave_temp_room_aux(y + 1, x);
      cave_temp_room_aux(y - 1, x);
      cave_temp_room_aux(y, x + 1);
      cave_temp_room_aux(y, x - 1);
      
      /* Spread diagonal */
      cave_temp_room_aux(y + 1, x + 1);
      cave_temp_room_aux(y - 1, x - 1);
      cave_temp_room_aux(y - 1, x + 1);
      cave_temp_room_aux(y + 1, x - 1);
    }
  
  /* Now, lite them all up at once */
  cave_temp_room_lite();
}


/**
 * Darken all rooms containing the given location
 */
void unlite_room(int y1, int x1)
{
  int i, x, y;
  
  /* Add the initial grid */
  cave_temp_room_aux(y1, x1);
  
  /* Spread, breadth first */
  for (i = 0; i < temp_n; i++)
    {
      x = temp_x[i], y = temp_y[i];
      
      /* Walls get dark, but stop darkness */
      if (!cave_project(y, x)) continue;
      
      /* Spread adjacent */
      cave_temp_room_aux(y + 1, x);
      cave_temp_room_aux(y - 1, x);
      cave_temp_room_aux(y, x + 1);
      cave_temp_room_aux(y, x - 1);
      
      /* Spread diagonal */
      cave_temp_room_aux(y + 1, x + 1);
      cave_temp_room_aux(y - 1, x - 1);
      cave_temp_room_aux(y - 1, x + 1);
      cave_temp_room_aux(y + 1, x - 1);
    }
  
  /* Now, darken them all at once */
  cave_temp_room_unlite();
}



/**
 * Hack -- call light around the player
 * Affect all monsters in the projection radius
 */
bool lite_area(int dam, int rad)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int flg = PROJECT_GRID | PROJECT_KILL;
  
  /* Hack -- Message */
  if (!p_ptr->blind)
    {
      msg_print("You are surrounded by a white light.");
    }
  
  /* Hook into the "project()" function */
  (void)project(-1, rad, py, px, dam, GF_LITE_WEAK, flg, 0, 0);
  
  /* Lite up the room */
  lite_room(py, px);
  
  /* Assume seen */
  return (TRUE);
}


/**
 * Hack -- call darkness around the player
 * Affect all monsters in the projection radius
 */
bool unlite_area(int dam, int rad)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int flg = PROJECT_GRID | PROJECT_KILL;
  
  /* Hack -- Message */
  if (!p_ptr->blind)
    {
      msg_print("Darkness surrounds you.");
    }
  
  /* Hook into the "project()" function */
  (void)project(-1, rad, py, px, dam, GF_DARK_WEAK, flg, 0, 0);
  
  /* Lite up the room */
  unlite_room(py, px);
  
  /* Assume seen */
  return (TRUE);
}


/**
 * Cast a ball spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Allow "jump" option to remove the standard "trail" from the caster to 
 * the target. -LM-
 * Affect grids, objects, and monsters
 */
bool fire_ball(int typ, int dir, int dam, int rad, bool jump)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int ty, tx;
  
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
  
  if (jump) flg |= PROJECT_JUMP;
  
  /* Use the given direction */
  ty = py + 99 * ddy[dir];
  tx = px + 99 * ddx[dir];
  
  /* Hack -- Use an actual "target" */
  if ((dir == 5) && target_okay())
    {
      flg &= ~(PROJECT_STOP);
      
      ty = p_ptr->target_row;
      tx = p_ptr->target_col;
    }
  
  /* Analyze the "dir" and the "target".  Hurt items on floor. */
  return (project(-1, rad, ty, tx, dam, typ, flg, 0, 0));
}


/**
 * Fire a sphere, defined as a ball spell that does not lose strength with 
 * distance from the center, up to a given diameter.  This spell is most 
 * often used to cast balls centered on the player with diameter 20, because 
 * it then offers "what you see is what you get" damage to adjacent monsters.
 * It could also be used to cast pinpoints of extremely intense energy (use 
 * a diameter of 5 or even less) more "realistically" than ball spells of 
 * radius 0. -LM-
 */
bool fire_sphere(int typ, int dir, int dam, int rad, byte diameter_of_source)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int ty, tx;
  
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
  
  /* Use the given direction */
  ty = py + 99 * ddy[dir];
  tx = px + 99 * ddx[dir];
  
  /* Hack -- Use an actual "target" */
  if ((dir == 5) && target_okay())
    {
      flg &= ~(PROJECT_STOP);
      
      ty = p_ptr->target_row;
      tx = p_ptr->target_col;
    }
  
  /* Analyze the "dir" and the "target".  Hurt items on floor. */
  return (project(-1, rad, ty, tx, dam, typ, flg, 0, diameter_of_source));
}


/**
 * Fire a cloud, defined as a ball spell that effects the player. -LM-
 */
bool fire_cloud(int typ, int dir, int dam, int rad)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int ty, tx;
  
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | 
    PROJECT_PLAY;
  
  /* Use the given direction */
  ty = py + 99 * ddy[dir];
  tx = px + 99 * ddx[dir];
  
  /* Hack -- Use an actual "target" */
  if ((dir == 5) && target_okay())
    {
      flg &= ~(PROJECT_STOP);
      
      ty = p_ptr->target_row;
      tx = p_ptr->target_col;
    }
  
  /* Analyze the "dir" and the "target".  Hurt items on floor. */
  return (project(-1, rad, ty, tx, dam, typ, flg, 0, 0));
}

/**
 * Cast a meteor spell, defined as a ball spell cast by an arbitary monster, 
 * player, or outside source, that starts out at an arbitrary location, and 
 * that leaves no trail from the "caster" to the target.  This function is 
 * especially useful for bombardments and similar. -LM-
 *
 * Option to hurt the player.
 */
bool fire_meteor(int who, int typ, int y, int x, int dam, int rad, 
		 bool hurt_player)
{
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | 
    PROJECT_JUMP;
  
  if (hurt_player) flg |= PROJECT_PLAY;
  
  
  /* Analyze the "target" and the caster. */
  return (project(who, rad, y, x, dam, typ, flg, 0, 0));
}


/**
 * Cast an arc-shaped spell.  This is nothing more than a sphere spell 
 * centered on the caster with a value for degrees_of_arc (how many degrees 
 * wide the the arc is) that is not 360.  The direction given will be the 
 * center of the arc, which travels outwards from the caster to a distance 
 * given by rad. -LM-
 *
 * Because all arcs start out as being one grid wide, arc spells with a 
 * value for degrees_of_arc less than (roughly) 60 do not dissipate as 
 * quickly.  In the extreme case where degrees_of_arc is 0, the arc is 
 * actually a defined length beam, and loses no strength at all over the 
 * ranges found in the game.
 */
bool fire_arc(int typ, int dir, int dam, int rad, int degrees_of_arc)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  /* Diameter of source of energy is normally, but not always, 20. */
  int diameter_of_source = 20;
  
  int ty, tx;
  
  int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_ARC;
  
  
  /* If a full circle is asked for, just cast a ball spell and have done. */
  if (degrees_of_arc >= 360) return (fire_sphere(typ, 0, dam, rad, 20));
  
  
  /* Use the given direction */
  ty = py + 99 * ddy[dir];
  tx = px + 99 * ddx[dir];
  
  /* Hack -- Use an actual "target" */
  if ((dir == 5) && target_okay())
    {
      flg &= ~(PROJECT_STOP);
      
      ty = p_ptr->target_row;
      tx = p_ptr->target_col;
    }
  
  /* Calculate the effective diameter of the energy source, if necessary. */
  if (degrees_of_arc < 60)
    {
      if (degrees_of_arc == 0) diameter_of_source = rad * 10;
      else diameter_of_source = diameter_of_source * 60 / degrees_of_arc;
    }
  
  /* Max */
  if (diameter_of_source > 250) diameter_of_source = 250;
  
  /* Analyze the "dir" and the "target".  Use the given degrees of arc, 
   * and the calculated source diameter.
   */
  return (project(-1, rad, ty, tx, dam, typ, flg, degrees_of_arc, 
		  (byte)diameter_of_source));
}



/**
 * Hack -- apply a "projection()" in a direction (or at the target)
 */
static bool project_hook(int typ, int dir, int dam, int flg)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int ty, tx;
  
  /* Pass through the target if needed */
  flg |= (PROJECT_THRU);
  
  /* Use the given direction */
  ty = py + ddy[dir];
  tx = px + ddx[dir];
  
  /* Hack -- Use an actual "target" */
  if ((dir == 5) && target_okay())
    {
      ty = p_ptr->target_row;
      tx = p_ptr->target_col;
    }
  
  /* Analyze the "dir" and the "target", do NOT explode */
  return (project(-1, 0, ty, tx, dam, typ, flg, 0, 0));
}


/**
 * Cast a bolt spell
 * Stop if we hit a monster, as a "bolt"
 * Affect monsters (not grids or objects)
 */
bool fire_bolt(int typ, int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(typ, dir, dam, flg));
}

/**
 * Cast a beam spell
 * Pass through monsters, as a "beam"
 * Affect monsters (not grids or objects)
 */
bool fire_beam(int typ, int dir, int dam)
{
  int flg = PROJECT_BEAM | PROJECT_KILL;
  return (project_hook(typ, dir, dam, flg));
}

/**
 * Cast a bolt spell, or rarely, a beam spell
 */
bool fire_bolt_or_beam(int prob, int typ, int dir, int dam)
{
  if (rand_int(100) < prob)
    {
      return (fire_beam(typ, dir, dam));
    }
  else
    {
      return (fire_bolt(typ, dir, dam));
    }
}


/**
 * Some of the old functions
 */

bool lite_line(int dir)
{
  int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
  return (project_hook(GF_LITE_WEAK, dir, damroll(4, 5), flg));
}

bool drain_life(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_DRAIN, dir, dam, flg));
}

bool wall_to_mud(int dir)
{
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
  return (project_hook(GF_KILL_WALL, dir, 20 + randint(30), flg));
}

bool wall_to_mud_hack(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
  return (project_hook(GF_KILL_WALL, dir, dam, flg));
}

bool destroy_door(int dir)
{
  int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
  return (project_hook(GF_KILL_DOOR, dir, 0, flg));
}

/** 
 * This function now casts a radius 0 ball spell on the adjacent square 
 * in whatever direction has been chosen. -LM-
 */
bool disarm_trap(int dir)
{
  /* Use the given direction */
  int ty = p_ptr->py + ddy[dir];
  int tx = p_ptr->px + ddx[dir];
  
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM;
  
  return (project(-1, 0, ty, tx, 0, GF_KILL_TRAP, flg, 0, 0));
  
}

bool heal_monster(int dir)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_HEAL, dir, damroll(4, 6), flg));
}

bool speed_monster(int dir)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_SPEED, dir, p_ptr->lev, flg));
}

bool slow_monster(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_SLOW, dir, dam, flg));
}

bool sleep_monster(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_SLEEP, dir, dam, flg));
}

bool confuse_monster(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_CONF, dir, dam, flg));
}

bool poly_monster(int dir)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_POLY, dir, p_ptr->lev, flg));
}

bool clone_monster(int dir)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_OLD_CLONE, dir, 0, flg));
}

bool fear_monster(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_TURN_ALL, dir, dam, flg));
}

bool dispel_an_undead(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_DISP_UNDEAD, dir, dam, flg));
}

bool dispel_a_demon(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_DISP_DEMON, dir, dam, flg));
}

bool dispel_a_dragon(int dir, int dam)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_DISP_DRAGON, dir, dam, flg));
}

bool teleport_monster(int dir, int dist)
{
  int flg = PROJECT_STOP | PROJECT_KILL;
  return (project_hook(GF_AWAY_ALL, dir, dist, flg));
}



/**
 * Hooks -- affect adjacent grids (radius 1 ball attack)
 */

bool door_creation(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
  return (project(-1, 1, py, px, 0, GF_MAKE_DOOR, flg, 0, 0));
}

bool trap_creation(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;

  /* Mega-hack - trap trees.  Need to fix projection here */
  int i;
  for (i = 0; i < 8; i++)
    {
      if ((cave_feat[py + ddy_ddd[i]][px + ddx_ddd[i]] == FEAT_TREE) ||
	  (cave_feat[py + ddy_ddd[i]][px + ddx_ddd[i]] == FEAT_TREE2))
	place_trap(py + ddy_ddd[i], px + ddx_ddd[i]);
    }
 
  return (project(-1, 1, py, px, 0, GF_MAKE_TRAP, flg, 0, 0));
}

bool destroy_doors_touch(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
  return (project(-1, 1, py, px, 0, GF_KILL_DOOR, flg, 0, 0));
}

bool sleep_monsters_touch(int dam)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  
  int flg = PROJECT_KILL | PROJECT_HIDE;
  return (project(-1, 1, py, px, dam, GF_OLD_SLEEP, flg, 0, 0));
}


