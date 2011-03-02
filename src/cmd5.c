/** \file cmd5.c 
    \brief Commands, part 5

 * Warrior probing.  Selection, browsing, learning, and casting of spells 
 * and prayers.  Includes definitions of all spells and prayers.  Shape-
 * shifting and making Athelas.
 *
 * Copyright (c) 1997-2009 Ben Harrison, James E. Wilson, Robert A. Koeneke,
 * Leon Marrick, Bahman Rabii, Nick McConnell
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
#include "ui-menu.h"




/**
 * Alter player's shape.  Taken from Sangband.
 */
void shapechange(s16b shape)
{
  char *shapedesc = "";
  bool landing = FALSE;

  /* Were we flying? */
  landing = ((p_ptr->schange == SHAPE_BAT) || (p_ptr->schange == SHAPE_WYRM));
  
  /* Wonder Twin powers -- Activate! */
  p_ptr->schange = (byte) shape;
  p_ptr->update |= PU_BONUS;
  
  switch (shape)
    {
    case SHAPE_MOUSE:
      shapedesc = "mouse";
      break;
    case SHAPE_FERRET:
      shapedesc = "ferret";
      break;
    case SHAPE_HOUND:
      shapedesc = "hound";
      break;
    case SHAPE_GAZELLE:
      shapedesc = "gazelle";
      break;
    case SHAPE_LION:
      shapedesc = "lion";
      break;
    case SHAPE_ENT:
      shapedesc = "elder ent";
      break;
    case SHAPE_BAT:
      shapedesc = "bat";
      break;
    case SHAPE_WEREWOLF:
      shapedesc = "werewolf";
      break;
    case SHAPE_VAMPIRE:
      if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) &&
	  (stage_map[p_ptr->stage][STAGE_TYPE] != VALLEY) &&
	  ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)))
	{
	  msg_print("The sunlight prevents your shapechange!");
	  shape = SHAPE_NORMAL;
	  p_ptr->schange = (byte) shape;
	  break;
	}
      shapedesc = "vampire";
      break;
    case SHAPE_WYRM:
      shapedesc = "dragon";
      break;
    case SHAPE_BEAR:
      shapedesc = "bear";
      break;
    default:
      msg_print("You return to your normal form.");
      break;
    }
  
  if (shape)
    {
      msg_format("You assume the form of a %s.", shapedesc);
      msg_print("Your equipment merges into your body.");
    }
  
  /* Recalculate mana. */
  p_ptr->update |= (PU_MANA);
  
  /* Show or hide shapechange on main window. */
  p_ptr->redraw |= (PR_SHAPE);
  
  if (landing)
    {
      int y = p_ptr->py, x = p_ptr->px;
 
      if (cave_feat[y][x] == FEAT_VOID)
	{
	  fall_off_cliff();
	}
      
      else if ((cave_feat[y][x] == FEAT_INVIS) ||
	       (cave_feat[y][x] == FEAT_GRASS_INVIS) ||
	       (cave_feat[y][x] == FEAT_TREE_INVIS) ||
	       (cave_feat[y][x] == FEAT_TREE2_INVIS))
	{
	  /* Disturb */
	  disturb(0, 0);
	  
	  /* Message */
	  msg_print("You stumble upon a trap!");
	  
	  /* Pick a trap */
	  pick_trap(y, x);
	  
	  /* Hit the floor trap. */
	  hit_trap(y, x);
	}
      
      /* Set off a visible trap */
      else if ((cave_feat[y][x] >= FEAT_TRAP_HEAD) && 
	       (cave_feat[y][x] <= FEAT_TRAP_TAIL))
	{
	  /* Disturb */
	  disturb(0, 0);
	  
	  /* Hit the floor trap. */
	  hit_trap(y, x);
	}
      
    }
}

/**
 * Type for choosing an elemental attack 
 */
typedef struct ele_attack_type
{
  char *desc;
  u32b type;
} ele_attack_type;

static ele_attack_type ele_attack[] = 
  {
    {"Fire Brand", ATTACK_FIRE},
    {"Cold Brand", ATTACK_COLD},
    {"Acid Brand", ATTACK_ACID},
    {"Elec Brand", ATTACK_ELEC}
  };

char el_tag(menu_type *menu, int oid)
{
  return I2A(oid);
}

/**
 * Display an entry on the sval menu
 */
void el_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  const u16b *choice = menu->menu_data;
  int idx = choice[oid];

  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  
  /* Print it */
  c_put_str(attr, format("%s", ele_attack[idx].desc), row, col);
}

/**
 * Deal with events on the sval menu
 */
bool el_action(menu_type *menu, const ui_event_data *e, int oid)
{
  u16b *choice = &menu->menu_data;
  
  /* Choose */
  if (e->type == EVT_SELECT)
    {
      int idx = choice[oid];
      set_ele_attack(ele_attack[idx].type, 200);
      
      return TRUE;
    }
  
  else if (e->type == EVT_ESCAPE) return FALSE;

  else 
    {
      int idx = choice[oid];
      set_ele_attack(ele_attack[idx].type, 200);
      
      return TRUE;
    }
  
  return FALSE;
}


/**
 * Display list of svals to be squelched.
 */
bool el_menu(void)
{
  menu_type menu;
  menu_iter menu_f = { el_tag, 0, el_display, el_action, 0 };
  region area = { (small_screen ? 0 : 15), 1, 48, -1 };
  ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;
  
  int num = 0;
  size_t i;
  
  u16b *choice;
  
  /* See how many attacks available */
  num = (p_ptr->lev - 20) / 7;
  
  /* Create the array */
  choice = C_ZNEW(num, u16b);
  
  /* Obvious */
  for (i = 0; i < num; i++)
    {
      /* Add this item to our possibles list */
      choice[i] = i;
    }

  /* Clear space */
  area.page_rows = num + 2;
  
  /* Return here if there are no attacks */
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
  menu.title = "Choose a temporary elemental brand";
  menu.cmd_keys = "\n\r";
  menu.count = num;
  menu.menu_data = choice;
  menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
  
  /* Select an entry */
  evt = menu_select(&menu, cursor);
  
  /* Free memory */
  FREE(choice);
  
  /* Load screen */
  screen_load();
  return (evt.type != EVT_ESCAPE);
}

/**
 * Choose a paladin elemental attack. -LM-
 */
static bool choose_ele_attack(void)
{
  bool brand = FALSE;

  /* Save screen */
  screen_save();
  
  /* Choose */
  if (!el_menu())
    msg_print("You cancel the temporary branding.");
  else
    brand = TRUE;
  
  /* Load screen */
  screen_load();

  return brand;
}


/**
 * Array of elemental resistances 
 */
const char *ele_resist[] = 
  {
    "Fire Resistance",
    "Cold Resistance",
    "Acid Resistance",
    "Electricity Resistance",
    "Poison Resistance"
  };

char res_tag(menu_type *menu, int oid)
{
  return I2A(oid);
}

/**
 * Display an entry on the sval menu
 */
void res_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  const u16b *choice = menu->menu_data;
  int idx = choice[oid];

  byte attr = (cursor ? TERM_L_BLUE : TERM_WHITE);
  
  
  /* Print it */
  c_put_str(attr, format("%s", ele_resist[idx]), row, col);
}

/**
 * Deal with events on the sval menu
 */
bool res_action(menu_type *menu, const ui_event_data *e, int oid)
{
  u16b *choice = &menu->menu_data;
  int plev = p_ptr->lev;
  
  /* Choose */
  if (e->type == EVT_ESCAPE) return FALSE;

  switch (choice[oid])
    {
    case 0:
      {
	(void)inc_timed(TMD_OPP_FIRE, randint1(plev) + plev, TRUE);
	return TRUE;
      }
    case 1:
      {
	(void)inc_timed(TMD_OPP_COLD, randint1(plev) + plev, TRUE);
	return TRUE;
      }
    case 2:
      {
	(void)inc_timed(TMD_OPP_ACID, randint1(plev) + plev, TRUE);
	return TRUE;
      }
    case 3:
      {
	(void)inc_timed(TMD_OPP_ELEC, randint1(plev) + plev, TRUE);
	return TRUE;
      }
    case 4:
      {
	(void)inc_timed(TMD_OPP_POIS, randint1(plev) + plev, TRUE);
	return TRUE;
      }
    default:
      {
	return FALSE;
      }
    }
}

/**
 * Display list of svals to be squelched.
 */
bool res_menu(void)
{
  menu_type menu;
  menu_iter menu_f = { res_tag, 0, res_display, res_action, 0 };
  region area = { (small_screen ? 0 : 15), 1, 48, -1 };
  ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;
  
  size_t i;
  
  u16b *choice;
  
  /* Create the array */
  choice = C_ZNEW(5, u16b);
  
  /* Obvious */
  for (i = 0; i < 5; i++)
    {
      /* Add this item to our possibles list */
      choice[i] = i;
    }

  /* Clear space */
  area.page_rows = 7;
  
  /* Save the screen and clear it */
  screen_save();
  
  /* Help text */
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.title = "Choose a temporary resistance";
  menu.cmd_keys = "\n\r";
  menu.count = 5;
  menu.menu_data = choice;
  menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
  
  /* Select an entry */
  evt = menu_select(&menu, cursor);
  
  /* Free memory */
  FREE(choice);
  
  /* Load screen */
  screen_load();
  return (evt.type != EVT_ESCAPE);
}

/**
 * Choose an elemental resistance
 */
extern bool choose_ele_resist(void)
{
  bool resist = FALSE;

  /* Save screen */
  screen_save();
  
  /* Choose */
  if (!res_menu())
    msg_print("You cancel the temporary resistance.");
  else
    resist = TRUE;
  
  /* Load screen */
  screen_load();

  return resist;
}


/**
 * Hack -- The Athelas-creation code. -LM-
 */
void create_athelas(void)
{
  int py = p_ptr->py;
  int px = p_ptr->px;
  object_type *i_ptr;
  object_type object_type_body;
  
  /* Get local object */
  i_ptr = &object_type_body;
  
  /* Hack -- Make some Athelas, identify it, and drop it near the player. */
  object_prep(i_ptr, lookup_kind(TV_FOOD, SV_FOOD_ATHELAS));
  
  /* Prevent money-making. */
  i_ptr->discount = 80;
  
  object_aware(i_ptr);
  object_known(i_ptr);
  drop_near(i_ptr, -1, py, px);
}


/**
 * Controlled teleportation.  -LM-
 * Idea from PsiAngband, through Zangband.
 */
void dimen_door(void)
{
  int ny;
  int nx;
  bool okay;
  bool old_expand_look = OPT(expand_look);
  
  OPT(expand_look) = TRUE;
  okay = target_set_interactive(TARGET_LOOK | TARGET_GRID);
  OPT(expand_look) = old_expand_look;
  if (!okay) return;
  
  /* grab the target coords. */
  ny = p_ptr->target_row;
  nx = p_ptr->target_col;
  
  /* Test for empty floor, forbid vaults or too large a
   * distance, and insure that this spell is never certain.
   */
  if (!cave_empty_bold(ny,nx) || (cave_info[ny][nx] & CAVE_ICKY) ||
     (distance(ny, nx, p_ptr->py, p_ptr->px) > 25) || 
      (randint0(p_ptr->lev) == 0))
    {
      msg_print("You fail to exit the astral plane correctly!");
      p_ptr->energy -= 50;
      teleport_player(15, FALSE);
      handle_stuff();
    }
  
  /* Controlled teleport. */
  else teleport_player_to(ny,nx,TRUE);
}


/**
 * Rebalance Weapon.  This is a rather powerful spell, because it can be 
 * used with any non-artifact throwing weapon, including ego-items.  It is 
 * therefore high-level, and curses weapons on failure.  Do not give Assas-
 * sins "Break Curse". -LM-
 */
static void rebalance_weapon(void)
{
  object_type *o_ptr;
  char o_name[120];
  
  /* Select the wielded melee weapon */
  o_ptr = &inventory[INVEN_WIELD];
  
  /* Nothing to rebalance */
  if (!o_ptr->k_idx) 
    {
      msg_print("You are not wielding any melee weapon.");
      return;
    }
  /* Artifacts not allowed. */
  if (o_ptr->name1)
    {
      msg_print("Artifacts cannot be rebalanced.");
      return;
    }
  
  /* Not a throwing weapon. */
  if (!(o_ptr->flags_obj & OF_THROWING)) 
    {
      msg_print("The melee weapon you are wielding is not designed for throwing.");
      return;
    }
  
  /* 20% chance to curse weapon. */
  else if (randint1(5) == 1)
    {
      /* Description */
      object_desc(o_name, o_ptr, FALSE, 0);
      
      /* Light curse and lower to_h and to_d by 2 to 5 each. */
      
      o_ptr->to_h -= (s16b) (2 + randint0(4));
      o_ptr->to_d -= (s16b) (2 + randint0(4));
      o_ptr->flags_curse |= 
	(OBJECT_RAND_BASE_CURSE << randint0(OBJECT_RAND_SIZE_CURSE));
      
      /* Describe */
      msg_format("Oh no!  A dreadful black aura surrounds your %s!", o_name);
      
      /* Recalculate bonuses */
      p_ptr->update |= (PU_BONUS);
    }
  
  /* Rebalance. */
  else 
    {
      /* Grant perfect balance. */
      o_ptr->flags_obj |= OF_PERFECT_BALANCE;
      
      /* Description */
      object_desc(o_name, o_ptr, FALSE, 0);
      
      /* Describe */
      msg_format("Your %s gleams steel blue!", o_name);
      
      /* Prevent money-making. */
      o_ptr->discount = 80;
    }
}








/**
 * Check if we have a race, class, or specialty ability -BR-
 */
bool check_ability(int ability)
{
  int i;
  
  /* Check for innate racial specialty */
  if ((ability >= SP_RACIAL_START) && (ability <= SP_RACIAL_END))
    {
      int offset = ability - SP_RACIAL_START;
      if (rp_ptr->flags_special & (0x00000001 << offset)) return(TRUE);
    }
  
  /* Check for innate class ability */
  if ((ability >= SP_CLASS_START) && (ability <= SP_CLASS_END))
    {
      int offset = ability - SP_CLASS_START;
      if (cp_ptr->flags_special & (0x00000001 << offset)) return(TRUE);
    }
  
  /* 
   * Check to see if the queried specialty is
   * on the allowed part of the list 
   */
  for (i = 0; i < p_ptr->specialties_allowed; i++)
    {
      if (p_ptr->specialty_order[i] == ability) return(TRUE);
    }
  
  /* Assume false */
  return(FALSE);
}

/**
 * Check if we have a specialty ability -BR-
 */
bool check_ability_specialty(int ability)
{
  int i;
  
  /* 
   * Check to see if the queried specialty is
   * on the allowed part of the list 
   */
  for (i = 0; i < p_ptr->specialties_allowed; i++)
    {
      if (p_ptr->specialty_order[i] == ability) return(TRUE);
    }
  
  /* Assume false */
  return(FALSE);
}

/**
 * Check if we can gain a specialty ability -BR-
 */
bool check_specialty_gain(int specialty)
{
  int i;
  bool allowed = FALSE;
  
  /* Can't learn it if we already know it */
  for (i = 0; i < MAX_SPECIALTIES; i++)
    {
      if (p_ptr->specialty_order[i] == specialty) return(FALSE);
    }
  
  /* Is it allowed for this class? */
  for (i = 0; i < CLASS_SPECIALTIES; i++)
    {
      if (cp_ptr->specialties[i] == specialty)
	{
	  allowed = TRUE;
	  break;
	}
    }
  
  /* return */
  return (allowed);
}

/**
 * Gain specialty code 
 */
int num;


static char gain_spec_tag(menu_type *menu, int oid)
{
  return I2A(oid);
}

/**
 * Display an entry on the gain specialty menu
 */
void gain_spec_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  const int *choices = menu->menu_data;
  int idx = choices[oid];
  int x, y;

  byte attr = (cursor ? TERM_L_GREEN : TERM_GREEN);
  
  /* Print it */
  c_put_str(attr, specialty_names[idx], row, col);

  /* Help text */
  if (cursor)
    {
      /* Locate the cursor */
      Term_locate(&x, &y);
  
      /* Move the cursor */
      Term_gotoxy(3, num + 2); 
      text_out_indent = 3;
      text_out_to_screen(TERM_L_BLUE, specialty_tips[choices[oid]]);

      /* Restore */
      Term_gotoxy(x, y);
    }
}

/**
 * Deal with events on the gain specialty menu
 */
bool gain_spec_action(menu_type *menu, const ui_event_data *e, int oid)
{
  u16b *choice = &menu->menu_data;

    return TRUE;
}


/**
 * Display list available specialties.
 */
bool gain_spec_menu(int *pick)
{
  menu_type menu;
  menu_iter menu_f = { gain_spec_tag, 0, gain_spec_display, 
		       gain_spec_action, 0 };
  ui_event_data evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;

  int choices[255];

  bool done = FALSE;
  
  size_t i;

  char buf[80];

  /* Find the learnable specialties */
  for (num = 0, i = 0; i < 255; i++)
    {
      if (check_specialty_gain(i))
	{
	  choices[num] = i;
	  num++;
	}
    }
      
  /* We are out of specialties - should never happen */
  if (!num)
    {
      msg_print("No specialties available.");
      normal_screen = TRUE;
      update_statusline();
      screen_load();
      return FALSE;
    }
      
  /* Save the screen and clear it */
  screen_save();
  
  /* Prompt choices */
  if (small_screen)
    sprintf(buf, "(Specialties: %c-%c, ESC) Gain which (%d left)? ", I2A(0), I2A(num - 1), p_ptr->new_specialties);
  else
    sprintf(buf, "(Specialties: %c-%c, ESC=exit) Gain which specialty (%d available)? ", I2A(0), I2A(num - 1), p_ptr->new_specialties);

  /* Set up the menu */
  WIPE(&menu, menu);
  menu.title = buf;
  menu.cmd_keys = "\n\r";
  menu.count = num;
  menu.menu_data = choices;
  menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
  
  while (!done)
    {
      evt = menu_select(&menu, cursor);
      done = (evt.type == EVT_ESCAPE);
      if (evt.type == EVT_SELECT) done = get_check("Are you sure? ");
    }

  if (evt.type == EVT_SELECT) *pick = evt.index;

  /* Load screen */
  screen_load();

  return (evt.type != EVT_ESCAPE);
}

/**
 * Gain a new specialty ability
 * Adapted from birth.c get_player_choice -BR-
 */
void do_cmd_gain_specialty(void)
{
  int i, k;
  int choices[255];
  int pick;
  
  /* Find the next open entry in "specialty_order[]" */
  for (k = 0; k < MAX_SPECIALTIES; k++)
    {
      /* Stop at the first empty space */
      if (p_ptr->specialty_order[k] == SP_NO_SPECIALTY) break;
    }
  
  /* Check if specialty array is full */
  if (k >= MAX_SPECIALTIES - 1)
    {
      msg_print("Maximum specialties known.");
      return;
    }
  
  /* Find the learnable specialties */
  for (num = 0, i = 0; i < 255; i++)
    {
      if (check_specialty_gain(i))
	{
	  choices[num] = i;
	  num++;
	}
    }
      
  /* Buttons */
  normal_screen = FALSE;
  prompt_end = 0;
  
  /* Make one choice */		
  if (gain_spec_menu(&pick))
    {
      char buf[120];
      
      /* Add new specialty */
      p_ptr->specialty_order[k] = choices[pick];
      
      /* Increment next available slot */
      k++;
      
      /* Update specialties available count */
      p_ptr->new_specialties--;
      p_ptr->old_specialties = p_ptr->new_specialties;
      
      /* Write a note */
      /* Specialty taken */
      sprintf(buf, "Gained the %s specialty.", 
	      specialty_names[choices[pick]]);
      
      /* Write message */
      make_note(buf,  p_ptr->stage, NOTE_SPECIALTY, p_ptr->lev);
      
      /* Update some stuff */
      p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | 
			PU_SPECIALTY | PU_TORCH);
      
      /* Redraw Study Status */
      p_ptr->redraw |= (PR_STUDY);
    }
  
  /* Buttons */
  normal_screen = TRUE;
  update_statusline();
  
  /* exit */
  return;
}


/**
 * Interact with specialty abilities -BR-
 */
void do_cmd_specialty(void)
{
  ui_event_data answer;
  
  /* Might want to gain a new ability or browse old ones */
  if (p_ptr->new_specialties > 0)
    {
      /* Buttons */
      button_add("l", 'l');
      button_add("v", 'v');

      /* Interact and choose. */
      while(get_com_ex("View abilities or Learn specialty (l/v/ESC)?",
		       &answer))
	{
	  /* New ability */
	  if ((answer.key == 'L') || (answer.key == 'l'))
	    {
	      do_cmd_gain_specialty();
	      break;
	    }
	  
	  /* View Current */
	  if ((answer.key == 'V') || (answer.key == 'v'))
	    {
	      do_cmd_view_abilities();
	      break;
	    }
	  
	  /* Exit */
	  else if (answer.key == ESCAPE) break;
	  
	  
	}
      button_kill('l');
      button_kill('v');
    }
  
  /* View existing specialties is the only option */
  else
    {
      do_cmd_view_abilities();
      return;
    }
  
  return;
}
