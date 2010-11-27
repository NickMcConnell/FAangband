/** \file load.c
    \brief Up-to-date savefile handling.
 
 
 * We attempt to prevent corrupt savefiles from inducing memory errors.
 *
 * Note that this file should not use the random number generator, the
 * object flavors, the visual attr/char mappings, or anything else which
 * is initialized *after* or *during* the "load character" function.
 *
 * This file assumes that the monster/object records are initialized
 * to zero, and the race/kind tables have been loaded correctly.  The
 * order of object stacks is currently not saved in the savefiles, but
 * the "next" pointers are saved, so all necessary knowledge is present.
 *
 * We should implement simple "savefile extenders" using some form of
 * "sized" chunks of bytes, with a {size,type,data} format, so everyone
 * can know the size, interested people can know the type, and the actual
 * data is available to the parsing routines that acknowledge the type.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick, Bahman Rabii, Ben Harrison
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




/**
 * Local "savefile" pointer
 */
static ang_file	*fff;

/**
 * Hack -- old "encryption" byte
 */
static byte	xor_byte;

/**
 * Hack -- simple "checksum" on the actual values
 */
static u32b	v_check = 0L;

/**
 * Hack -- simple "checksum" on the encoded bytes
 */
static u32b	x_check = 0L;



/**
 * This function determines if the version of the savefile
 * currently being read is older than FAangband version "x.y.z".
 */
static bool older_than(byte x, byte y, byte z)
{
  /* Much older, or much more recent */
  if (sf_major < x) return (TRUE);
  if (sf_major > x) return (FALSE);
  
  /* Distinctly older, or distinctly more recent */
  if (sf_minor < y) return (TRUE);
  if (sf_minor > y) return (FALSE);
  
  /* From FAangband 1.0.0, this function doesn't see patch differences */
  if (sf_major >= 1) return (FALSE);

  /* Barely older, or barely more recent */
  if (sf_patch < z) return (TRUE);
  if (sf_patch > z) return (FALSE);

  /* Identical versions */
  return (FALSE);
}




/**
 * Hack -- Show information on the screen, one line at a time.
 *
 * Avoid the top two lines, to avoid interference with "msg_print()".
 */
static void note(cptr msg)
{
  static int y = 2;
  
  /* Draw the message */
  prt(msg, y, 0);
  
  /* Advance one line (wrap if needed) */
  if (++y >= 24) y = 2;
  
  /* Flush it */
  Term_fresh();
}


/**
 * The following functions are used to load the basic building blocks
 * of savefiles.  They also maintain the "checksum" info for 2.7.0+
 */

static byte sf_get(void)
{
  byte c, v;
  
  /* Get a character, decode the value */
  if (!file_readc(fff, &c)) return (EOF);
  v = c ^ xor_byte;
  xor_byte = c;
  
  /* Maintain the checksum info */
  v_check += v;
  x_check += xor_byte;
  
  /* Return the value */
  return (v);
}

static void rd_byte(byte *ip)
{
  *ip = sf_get();
}

static void rd_u16b(u16b *ip)
{
  (*ip) = sf_get();
  (*ip) |= ((u16b)(sf_get()) << 8);
}

static void rd_s16b(s16b *ip)
{
  rd_u16b((u16b*)ip);
}

static void rd_u32b(u32b *ip)
{
  (*ip) = sf_get();
  (*ip) |= ((u32b)(sf_get()) << 8);
  (*ip) |= ((u32b)(sf_get()) << 16);
  (*ip) |= ((u32b)(sf_get()) << 24);
}

static void rd_s32b(s32b *ip)
{
  rd_u32b((u32b*)ip);
}


/**
 * Hack -- read a string
 */
static void rd_string(char *str, int max)
{
  int i;
  
  /* Read the string */
  for (i = 0; TRUE; i++)
    {
      byte tmp8u;

      /* Read a byte */
      rd_byte(&tmp8u);
      
      /* Collect string while legal */
      if (i < max) str[i] = tmp8u;
      
      /* End of string */
      if (!tmp8u) break;
    }

  /* Terminate */
  str[max-1] = '\0';
}


/**
 * Hack -- strip some bytes
 */
static void strip_bytes(int n)
{
  byte tmp8u;
  
  /* Strip the bytes */
  while (n--) rd_byte(&tmp8u);
}




/**
 * Read an object
 *
 * This function attempts to "repair" old savefiles, and to extract
 * the most up to date values for various object fields.
 *
 * Change extra might to non-pval form if needed -LM-
 *
 * Deal with themed level, glyphs/traps count, player ghosts -LM-
 */
static void rd_item(object_type *o_ptr)
{
  int i;

  char buf[128];

  byte tmp;
  
  
  /* Kind */
  rd_s16b(&o_ptr->k_idx);
  
  /* Location */
  rd_byte(&o_ptr->iy);
  rd_byte(&o_ptr->ix);
  
  /* Type/Subtype */
  rd_byte(&o_ptr->tval);
  rd_byte(&o_ptr->sval);
  
  /* Special pval */
  rd_s16b(&o_ptr->pval);
  
  rd_byte(&o_ptr->discount);
  rd_byte(&o_ptr->number);
  rd_s16b(&o_ptr->weight);
  
  rd_byte(&o_ptr->name1);
  rd_byte(&o_ptr->name2);
  rd_s16b(&o_ptr->timeout);
  
  rd_s16b(&o_ptr->to_h);
  rd_s16b(&o_ptr->to_d);
  rd_s16b(&o_ptr->to_a);
  
  rd_s16b(&o_ptr->ac);

  /* SJGU keep save dice */
  rd_byte(&o_ptr->dd);
  rd_byte(&o_ptr->ds);


  rd_byte(&o_ptr->ident);
  
  rd_byte(&o_ptr->marked);
  
  if (!older_than(1, 0, 0))
    {
      rd_u32b(&o_ptr->flags_obj);	/* New object flags -NRM- */
      rd_u32b(&o_ptr->flags_curse);	/* New curse flags  -NRM- */
      rd_u32b(&o_ptr->id_curse);	/* New curse id  -NRM- */
    }

  if (!older_than(1, 1, 0))
    {
      rd_u32b(&o_ptr->id_obj);	/* New object flags id  -NRM- */
      rd_u32b(&o_ptr->id_other);	/* New object other id  -NRM- */
    }
  else if (object_known_p(o_ptr))
    {
      o_ptr->id_obj = o_ptr->flags_obj;
      o_ptr->id_other = flags_other(o_ptr);
      o_ptr->ident |= IDENT_WORN;
    }

  /* Percentage resists -NRM- */
  if (older_than(0, 3, 0))
    {
      strip_bytes(8);
    }

  else
    {  
      /* New percentage resists -NRM- */
      for (i = 0; i < MAX_P_RES; i++)
	{
	  rd_byte(&tmp);
	  o_ptr->percent_res[i] = tmp;
	}
      
      /* Element proofing */
      if (older_than(1, 0, 0))
	strip_bytes(1);
    }

  /* Bonuses and multiples - nasty byte hack */
  if (!older_than(1, 0, 0))
    {
      for (i = 0; i < A_MAX; i++)
	{
	  rd_byte(&tmp);
	  if (tmp < 129) o_ptr->bonus_stat[i] = (int)tmp;
	  else o_ptr->bonus_stat[i] = (int)(tmp - 256);
	}
      for (i = 0; i < MAX_P_BONUS; i++)
	{
	  rd_byte(&tmp);
	  if (tmp < 129) o_ptr->bonus_other[i] = (int)tmp;
	  else o_ptr->bonus_other[i] = (int)(tmp - 256);
	}
      for (i = 0; i < MAX_P_SLAY; i++)
	{
	  rd_byte(&tmp);
	  if (tmp < 129) o_ptr->multiple_slay[i] = (int)tmp;
	  else o_ptr->multiple_slay[i] = (int)(tmp - 256);
	}
      for (i = 0; i < MAX_P_BRAND; i++)
	{
	  rd_byte(&tmp);
	  if (tmp < 129) o_ptr->multiple_brand[i] = (int)tmp;
	  else o_ptr->multiple_brand[i] = (int)(tmp - 256);
	}
    }
  
  /* Where found */
  rd_u32b(&o_ptr->found);

  /* Monster holding object */
  rd_s16b(&o_ptr->held_m_idx);
	
  /* Activation */
  if (older_than(1, 0, 0)) strip_bytes(1);
  rd_byte(&o_ptr->activation);
  
  
  rd_byte(&o_ptr->feel);
  
  /* Inscription */
  rd_string(buf, 128);
  
  /* Save the inscription */
  if (buf[0]) o_ptr->note = quark_add(buf);
  
#if 0 /* All unnecessary? */  
  /* Obtain the "kind" template */
  k_ptr = &k_info[o_ptr->k_idx];
  
  /* Obtain tval/sval from k_info */
  o_ptr->tval = k_ptr->tval;
  o_ptr->sval = k_ptr->sval;
  
  /* Repair non "wearable" items */
  if (!wearable_p(o_ptr))
    {
      /* Acquire correct fields */
      o_ptr->to_h = k_ptr->to_h;
      o_ptr->to_d = k_ptr->to_d;
      o_ptr->to_a = k_ptr->to_a;
      
      /* Acquire correct fields */
      o_ptr->ac = k_ptr->ac;
      o_ptr->dd = k_ptr->dd;
      o_ptr->ds = k_ptr->ds;
      
      /* Acquire correct weight, unless an artifact. */
      if (!o_ptr->name1) o_ptr->weight = k_ptr->weight;
      
      
      /* All done */
      return;
    }


  /* Verify, convert artifacts if needed. */
  if (o_ptr->name1)
    {
      artifact_type *a_ptr;
      
      /* Obtain the artifact info */
      a_ptr = &a_info[o_ptr->name1];
      
      /* Verify that artifact */
      if (!a_ptr->name) o_ptr->name1 = 0;
    }
  
  /* Paranoia */
  if (o_ptr->name2)
    {
      ego_item_type *e_ptr;
      
      /* Obtain the ego-item info */
      e_ptr = &e_info[o_ptr->name2];
      
      /* Verify that ego-item */
      if (!e_ptr->name) o_ptr->name2 = 0;
    }

  /* Acquire standard fields. */
  o_ptr->ac = k_ptr->ac;

  if (o_ptr->name2 == EGO_DWARVEN) o_ptr->ac += 5;

  /* Artifacts */
  if (o_ptr->name1)
    {
      artifact_type *a_ptr;
      
      /* Obtain the artifact info */
      a_ptr = &a_info[o_ptr->name1];
      
      /* Acquire new artifact "pval" */
      o_ptr->pval = a_ptr->pval;
      
      /* Acquire new artifact fields */
      o_ptr->ac = a_ptr->ac;
      o_ptr->dd = a_ptr->dd;
      o_ptr->ds = a_ptr->ds;
      
      /* Acquire new artifact weight */
      o_ptr->weight = a_ptr->weight;
      
      /* If artifact index includes an activation, assign it. */
      if (a_ptr->activation)
	{
	  o_ptr->activation = a_ptr->activation;
	}
    }
  
  /* Ego items */
  if (o_ptr->name2)
    {
      ego_item_type *e_ptr;
      
      /* Obtain the ego-item info */
      e_ptr = &e_info[o_ptr->name2];
    }
#endif
}



/**
 * Read a monster
 */
static void rd_monster(monster_type *m_ptr)
{
  byte tmp8u;
  s16b tmp16u;
  
  /* Read the monster race */
  rd_s16b(&m_ptr->r_idx);
  
  /* Read the other information */
  rd_byte(&m_ptr->fy);
  rd_byte(&m_ptr->fx);
  rd_s16b(&m_ptr->hp);
  rd_s16b(&m_ptr->maxhp);
  rd_s16b(&m_ptr->csleep);
  rd_byte(&m_ptr->mspeed);
  rd_byte(&m_ptr->energy);
  rd_byte(&m_ptr->stunned);
  rd_byte(&m_ptr->confused);
  rd_byte(&m_ptr->monfear);
  
  /* Oangband 0.3.3 added monster stasis. */
  rd_byte(&m_ptr->stasis);
  
  
  /* Oangband 0.5.0 saves 'smart learn' flags and Black Breath state */
  rd_byte(&tmp8u);
  m_ptr->black_breath = tmp8u;
  rd_u32b(&m_ptr->smart);
  
  /* Oangband 0.5.0 saves some more data not yet in use */
  rd_byte(&tmp8u);

  /* Shapechange counter */
  rd_byte(&m_ptr->schange);

  /* Original form of a shapechanger */
  rd_s16b(&m_ptr->orig_idx);
  
  /* Monster extra desire to cast harassment spells */
  rd_byte(&m_ptr->harass);
  
  /*  Monster mana  */
  rd_byte(&m_ptr->mana);
  
  if (!older_than(1, 0, 0))
    {
      /* Racial type */
      rd_byte(&m_ptr->p_race);
      rd_byte(&m_ptr->old_p_race);
      
      /* AI info */
      rd_s16b(&m_ptr->hostile);
      rd_u16b(&m_ptr->group);
      rd_u16b(&m_ptr->group_leader);
      
      /* Territorial info */
      rd_u16b(&m_ptr->y_terr);
      rd_u16b(&m_ptr->x_terr);
    }

  /* Spare */
  rd_s16b(&tmp16u);
  
}





/**
 * Read the monster lore
 */
static void rd_lore(int r_idx)
{
  byte tmp8u;
  
  monster_race *r_ptr = &r_info[r_idx];
  monster_lore *l_ptr = &l_list[r_idx];
  
  
  /* Count sights/deaths/kills */
  rd_s16b(&l_ptr->sights);
  rd_s16b(&l_ptr->deaths);
  rd_s16b(&l_ptr->pkills);
  rd_s16b(&l_ptr->tkills);
  
  /* Count wakes and ignores */
  rd_byte(&l_ptr->wake);
  rd_byte(&l_ptr->ignore);
  
  /* Extra stuff */
  rd_byte(&l_ptr->xtra1);
  rd_byte(&l_ptr->xtra2);
  
  /* Count drops */
  rd_byte(&l_ptr->drop_gold);
  rd_byte(&l_ptr->drop_item);
  
  /* Count spells */
  rd_byte(&l_ptr->cast_inate);
  rd_byte(&l_ptr->cast_spell);
  
  /* Count blows of each type */
  rd_byte(&l_ptr->blows[0]);
  rd_byte(&l_ptr->blows[1]);
  rd_byte(&l_ptr->blows[2]);
  rd_byte(&l_ptr->blows[3]);
  
  /* Memorize flags */
  rd_u32b(&l_ptr->flags1);
  rd_u32b(&l_ptr->flags2);
  rd_u32b(&l_ptr->flags3);
  rd_u32b(&l_ptr->flags4);
  rd_u32b(&l_ptr->flags5);
  rd_u32b(&l_ptr->flags6);
  
  
  /* Read the "Racial" monster limit per level */
  rd_byte(&r_ptr->max_num);
  
  /* Later (?) */
  rd_byte(&tmp8u);
  rd_byte(&tmp8u);
  rd_byte(&tmp8u);
  
  /* Repair the lore flags */
  l_ptr->flags1 &= r_ptr->flags1;
  l_ptr->flags2 &= r_ptr->flags2;
  l_ptr->flags3 &= r_ptr->flags3;
  l_ptr->flags4 &= r_ptr->flags4;
  l_ptr->flags5 &= r_ptr->flags5;
  l_ptr->flags6 &= r_ptr->flags6;
}




/**
 * Read a store
 */
static errr rd_store(int n)
{
  store_type *st_ptr = &store[n];
  
  int j;
  
  byte num;
  
  /* Read the basic info */
  rd_s32b(&st_ptr->store_open);
  rd_s16b(&st_ptr->insult_cur);
  rd_byte(&st_ptr->owner);
  rd_byte(&num);
  rd_s16b(&st_ptr->good_buy);
  rd_s16b(&st_ptr->bad_buy);
  
  /* Read the items */
  for (j = 0; j < num; j++)
    {
      object_type *i_ptr;
      object_type object_type_body;
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Read the item */
      rd_item(i_ptr);
      
      /* Acquire valid items */
      if (st_ptr->stock_num < STORE_INVEN_MAX)
	{
	  int k = st_ptr->stock_num++;
	  
	  /* Acquire the item */
	  object_copy(&st_ptr->stock[k], i_ptr);
	}
    }

  /* Load the order information */
  if ((!older_than(0, 2, 0)) && (st_ptr->type == STORE_MERCH))
    {
    
      /* Look at the ordered slots */
      for (j = 24; j < 32; j++)
	{
	  /* Save the table entry */
	  rd_s16b(&st_ptr->table[j]);
	}
    }
  
  /* Success */
  return (0);
}



/**
 * Read RNG state (added in 2.8.0)
 */
static void rd_randomizer(void)
{
  int i;
  
  u16b tmp16u;
  
  /* Tmp */
  rd_u16b(&tmp16u);
  
  /* Place */
  rd_u16b(&Rand_place);
  
  /* State */
  for (i = 0; i < RAND_DEG; i++)
    {
      rd_u32b(&Rand_state[i]);
    }
  
  /* Accept */
  Rand_quick = FALSE;
}



/**
 * Read options (ignore most pre-2.8.0 options)
 *
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
static void rd_options(void)
{
  int i, n;
  
  byte b;
  
  u32b flag[8];
  u32b mask[8];
  
  
  
  /*** Oops ***/
  
  /* Ignore old options.  Reduced from 16 to 13 in Oangband. */
  /* Reduction accounts for autosave options */
  strip_bytes(13);
  
  
  /*** Timed Autosave, inspired by Zangband. ***/
  rd_byte(&b);
  autosave = b;
  rd_s16b(&autosave_freq);
  
  
  /*** Special info */
  
  /* Read "delay_factor" */
  rd_byte(&b);
  op_ptr->delay_factor = b;
  
  /* Read "hitpoint_warn" */
  rd_byte(&b);
  op_ptr->hitpoint_warn = b;
  
  /* Read "panel_change" */
  rd_byte(&b); 
  op_ptr->panel_change = b;
  
  /* Unused */
  rd_byte(&b);
  
  /*** Normal Options ***/
  
  /* Read the option flags */
  for (n = 0; n < 8; n++) rd_u32b(&flag[n]);
  
  /* Read the option masks */
  for (n = 0; n < 8; n++) rd_u32b(&mask[n]);
  
  /* Analyze the options */
  for (i = 0; i < OPT_MAX; i++)
    {
      int os = i / 32;
      int ob = i % 32;
      
      /* Process real entries */
      if (option_text[i])
	{
	  
	  /* Process saved entries */
	  if (mask[os] & (1L << ob))
	    {
	      /* Set flag */
	      if (flag[os] & (1L << ob))
		{
		  /* Set */
		  op_ptr->opt[i] = TRUE;
		}
	      /* Clear flag */
	      else
		{
		  /* Set */
		  op_ptr->opt[i] = FALSE;
		}
	    }
	}
    }
  
  
  /*** Window Options ***/
  
  /* Read the window flags */
  for (n = 0; n < 8; n++) rd_u32b(&flag[n]);
  
  /* Read the window masks */
  for (n = 0; n < 8; n++) rd_u32b(&mask[n]);
  
  /* Analyze the options */
  for (n = 0; n < 8; n++)
    {
      /* Analyze the options */
      for (i = 0; i < 32; i++)
	{
	  /* Process valid flags */
	  if (window_flag_desc[i])
	    {
	      /* Process valid flags */
	      if (mask[n] & (1L << i))
		{
		  /* Set */
		  if (flag[n] & (1L << i))
		    {
		      /* Set */
		      op_ptr->window_flag[n] |= (1L << i);
		    }
		}
	    }
	}
    }
  
}





/**
 * Hack -- strip the old-style "player ghost" info.
 *
 * XXX XXX XXX This is such a nasty hack it hurts.
 */
static void rd_ghost(void)
{
  char buf[64];
  
  /* Strip name */
  rd_string(buf, 64);
  
  /* Strip old data */
  strip_bytes(60);
  
}


/**
 * String-handling function from Greg Wooledge's random artifact generator. 
 */
static char *my_strdup (const char *s)
{
  char *t = malloc(strlen (s) + 1);
  if (t) strcpy (t, s);
  return t;
}

/**
 * Hack - hard code this here 
 */
#define MAX_A_IDX       250     /* Max size for "a_info[]" */


/**
 * Read the saved random artifacts from a savefile, and add them to the 
 * a_name structure.  This code is adopted from Greg Wooledge's random 
 * artifacts.
 */
static int convert_saved_names(void)
{
  size_t name_size;
  char *a_base;
  char *a_next;
  char temp[64];
  
  int i;
  
  /* Temporary space for names, while reading and randomizing them. */
  char *names[MAX_A_IDX];
  
  /* Add the permanent artifact names to the temporary array. */
  for (i = 0; i < ART_MIN_RANDOM; i++)
    {
      artifact_type *a_ptr = &a_info[i];
      names[i] = a_name + a_ptr->name;
    }
  
  /* Add the random artifact names to the temporary array. */
  for (i = ART_MIN_RANDOM; i < MAX_A_IDX; i++)
    {
      rd_string(temp, 64);
      
      names[i] = my_strdup(temp);
    }
  
  
  /* Convert our names array into an a_name structure for later use. */
  name_size = 1;
  for (i = 0; i < MAX_A_IDX; i++)
    {
      name_size += strlen(names[i]) + 1;	/* skip first char */
    }
  if ((a_base = ralloc(name_size)) == NULL)
    {
      note("Memory allocation error");
      return 1;
    }
  
  
  a_next = a_base + 1;	/* skip first char */
  for (i = 0; i < MAX_A_IDX; i++)
    {
      strcpy(a_next, names[i]);
      if (a_info[i].tval > 0)		/* skip unused! */
	a_info[i].name = a_next - a_base;
      a_next += strlen(names[i]) + 1;
    }
  
  
  /* Free some of our now unneeded memory. */
  KILL (a_name);
  for (i = ART_MIN_RANDOM; i < MAX_A_IDX; i++)
    {
      free(names[i]);
    }
  a_name = a_base;
  
  return 0;
}


/**
 * Convert a pre-0.2.2 stage 
 */
int conv_stage(int old)
{
  int new = old;
  
  /* Ered Luin */
  if ((old > 21) && (old < 27)) new = 21 + ((old - 21) / 2);

  /* Doriath */
  if (old == 88) new = 87;

  /* Himlad */
  if ((old > 88) && (old < 93)) new = 88 + ((old - 88) / 2);

  /* Dor Dinen, Dorthonion */
  if ((old > 92) && (old < 105)) new = 92 + ((old - 92) / 2);

  /* Brethil to Angband */
  if ((old > 116) && (old < 136)) new = 116 + ((old - 116) / 2);

  /* Lothlann */
  if ((old > 135) && (old < 149)) new = 135 + ((old - 135) / 2);

  /* Nan Dungortheb */
  if ((old > 170) && (old < 181)) new = 181;
  if ((old > 209) && (old < 221)) new = 210;
  if (old == 221) new = 211;

  /* Tol-In-Gaurhoth */
  if ((old > 262) && (old < 273)) new = 273;
  if ((old > 317) && (old < 323)) new = 317;
  if (old == 323) new = 318;

  /* Angband */
  if ((old > 323) && (old < 344)) new = 344;
  if ((old > 317) && (old < 323)) new = 317;
  if (old == 323) new = 318;

  return (new);
}


/**
 * Read squelch and autoinscription submenu for all known objects
 */
static int rd_squelch(void)
{
  int i;
  byte tmp8u = 24;
  u16b file_e_max;
  
  /* Handle old versions */
  if (older_than(0, 3, 0))
    return 0;


  /* Read how many squelch bytes we have */
  rd_byte(&tmp8u);

  /* Check against current number */
  if (tmp8u != TYPE_MAX)
    {
      strip_bytes(tmp8u);
    }
  else
    {
      for (i = 0; i < TYPE_MAX; i++)
	rd_byte(&squelch_level[i]);
    }
  
  
  /* Read the number of saved ego-item */
  rd_u16b(&file_e_max);
  
  for (i = 0; i < file_e_max; i++)
    {
      if (i < z_info->e_max)
	{
	  byte flags;
	  
	  /* Read and extract the flag */
	  rd_byte(&flags);
	  if (!older_than(0, 3, 2)) 
	    e_info[i].squelch = (flags & 0x01) ? TRUE : FALSE;
	  e_info[i].everseen = (flags & 0x02) ? TRUE : FALSE;
	}
    }
  
  /* Read the current number of auto-inscriptions */
  rd_u16b(&inscriptions_count);
  
  /* Write the autoinscriptions array*/
  for (i = 0; i < inscriptions_count; i++)
    {
      char tmp[80];
      
      rd_s16b(&inscriptions[i].kind_idx);
      rd_string(tmp, sizeof(tmp));
      
      inscriptions[i].inscription_idx = quark_add(tmp);
    }
  
  return 0;
}


/**
 * Read the "extra" information
 */
static errr rd_extra(void)
{
  int i;
  
  byte tmp8u;
  u16b tmp16u;
  
  rd_string(op_ptr->full_name, 32);
  
  rd_string(p_ptr->died_from, 80);
  
  for (i = 0; i < 4; i++)
    {
      rd_string(p_ptr->history[i], 60);
    }
  
  /* Class/Race/Gender/Spells */
  rd_byte(&p_ptr->prace);
  rd_byte(&p_ptr->pclass);
  rd_byte(&p_ptr->psex);
  rd_byte(&tmp8u);	/* oops */
  
  /* Repair psex (2.8.1 beta) */
  if (p_ptr->psex > MAX_SEXES - 1) p_ptr->psex = MAX_SEXES - 1;
  
  /* Special Race/Class info */
  rd_byte(&p_ptr->hitdie);
  strip_bytes(1);
  
  /* Age/Height/Weight */
  rd_s16b(&p_ptr->age);
  rd_s16b(&p_ptr->ht);
  rd_s16b(&p_ptr->wt);
  
  /* Read the stat info */
  for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_max[i]);
  for (i = 0; i < 6; i++) rd_s16b(&p_ptr->stat_cur[i]);

  /* Barehand damage */  
  for (i = 0; i < 12; i++) rd_u16b(&p_ptr->barehand_dam[i]);
  
  rd_s32b(&p_ptr->au);
  
  rd_s32b(&p_ptr->max_exp);
  rd_s32b(&p_ptr->exp);
  rd_u16b(&p_ptr->exp_frac);
  rd_s16b(&p_ptr->lev);
  rd_s16b(&p_ptr->home);
  
  rd_s16b(&p_ptr->mhp);
  rd_s16b(&p_ptr->chp);
  rd_u16b(&p_ptr->chp_frac);
  
  rd_s16b(&p_ptr->msp);
  rd_s16b(&p_ptr->csp);
  rd_u16b(&p_ptr->csp_frac);
  
  rd_s16b(&p_ptr->max_lev);
  for (i = 0; i < 4; i++)	rd_s16b(&p_ptr->recall[i]);
  rd_s16b(&p_ptr->recall_pt);

  if (older_than(0, 2, 2))
    {
      /* Convert recall points */
      for (i = 0; i < 4; i++)
	{
	  p_ptr->recall[i] = conv_stage(p_ptr->recall[i]);
	}
      p_ptr->recall_pt = conv_stage(p_ptr->recall_pt);
    }

  /* Hack -- Repair maximum player level */
  if (p_ptr->max_lev < p_ptr->lev) p_ptr->max_lev = p_ptr->lev;
  
  
  /* More info */
  rd_s16b(&p_ptr->speed_boost);
  rd_s16b(&p_ptr->heighten_power);
  
  strip_bytes(4);
  rd_s16b(&p_ptr->sc);
  strip_bytes(2);
  
  /* Read the flags */
  strip_bytes(2);	/* Old "rest" */
  rd_s16b(&p_ptr->blind);
  rd_s16b(&p_ptr->paralyzed);
  rd_s16b(&p_ptr->confused);
  rd_s16b(&p_ptr->food);
  strip_bytes(4);	/* Old "food_digested" / "protection" */
  rd_s16b(&p_ptr->energy);
  rd_s16b(&p_ptr->fast);
  rd_s16b(&p_ptr->slow);
  rd_s16b(&p_ptr->afraid);
  rd_s16b(&p_ptr->cut);
  rd_s16b(&p_ptr->stun);
  rd_s16b(&p_ptr->poisoned);
  rd_s16b(&p_ptr->image);
  rd_s16b(&p_ptr->protevil);
  rd_s16b(&p_ptr->magicdef);
  rd_s16b(&p_ptr->hero);
  rd_s16b(&p_ptr->shero);
  rd_s16b(&p_ptr->shield);
  rd_s16b(&p_ptr->blessed);
  rd_s16b(&p_ptr->tim_invis);
  rd_s16b(&p_ptr->tim_esp);
  rd_s16b(&p_ptr->superstealth);
  rd_s16b(&p_ptr->ele_attack);
  rd_s16b(&p_ptr->word_recall);
  rd_s16b(&p_ptr->see_infra);
  
  
  rd_s16b(&p_ptr->tim_infra);
  rd_s16b(&p_ptr->oppose_fire);
  rd_s16b(&p_ptr->oppose_cold);
  rd_s16b(&p_ptr->oppose_acid);
  rd_s16b(&p_ptr->oppose_elec);
  rd_s16b(&p_ptr->oppose_pois);
  
  
  /* Bit flags for various attack modifiers. */
  rd_u32b(&p_ptr->special_attack);
  
  rd_byte(&tmp8u);	/* oops */
  rd_byte(&tmp8u);	/* oops */
  
  rd_byte((byte *) &p_ptr->black_breath);	/* Status of Black Breath. */
  
  rd_byte(&p_ptr->searching);
  
  /* Was maximize mode */
  rd_byte(&tmp8u);
  
  /* If using an old Angband or a version of O based on an old Angband,
   * use the old preserve byte.
   */
  
  rd_byte(&tmp8u);
  
  /* Current shapechange. Note: this byte is related to random artifacts
   * in some versions of Standard Angband */
  rd_byte(&p_ptr->schange);
  
  /* The number of the bone file (if any) that player ghosts should use to 
   * reacquire a name, sex, class, and race.
   */
  rd_byte(&bones_selector);
  
  /* Find out how many thefts have already occured on this level. */
  rd_byte(&number_of_thefts_on_level);
  
  /* Read number of monster traps on level. */
  rd_byte(&num_trap_on_level);
  
  /* Read number of glyphs on level. */
  if (older_than(1, 0, 0))
    {
      rd_byte(&tmp8u);
      num_runes_on_level[RUNE_PROTECT] = tmp8u;
    }
  else
    {
      for (i = 0; i < MAX_RUNE; i++)
	{
	  rd_byte(&tmp8u);
	  num_runes_on_level[i] = tmp8u;
	}
      rd_byte(&tmp8u);
      mana_reserve = tmp8u;
    }
  
  /* Is the level themed and, if so, which theme is it? */
  rd_byte(&p_ptr->themed_level);
  
  /* What themed levels have already appeared? */
  rd_u32b(&p_ptr->themed_level_appeared);

  /* Old squelch */
  if (older_than(0, 3, 0))
      strip_bytes(39);
  
  /* Item Squelch */
  if (rd_squelch()) return -1;

 
  /* Specialty Abilities -BR- */
  for (i = 0; i < 10; i++) rd_byte(&p_ptr->specialty_order[i]);
  
  /* Skip the flags */
  strip_bytes(2);
  
  /* Hack -- the two types of "special seeds" */
  rd_u32b(&seed_flavor);
  for (i = 0;i < 10;i++)
    rd_u32b(&seed_town[i]);
  
  
  /* Special stuff */
  rd_u16b(&p_ptr->quests);
  if (!older_than(1, 1, 0)) rd_u32b(&p_ptr->score);
  else 
    {
      rd_u16b(&tmp16u);
      p_ptr->score = (u32b) tmp16u;
    }
  rd_u16b(&p_ptr->total_winner);
  rd_u16b(&p_ptr->noscore);
  
  
  /* Read "death" */
  rd_byte(&tmp8u);
  p_ptr->is_dead = tmp8u;
  
  /* Read "feeling" */
  rd_byte(&tmp8u);
  feeling = tmp8u;
  
  /* Turn of last "feeling" */
  rd_s32b(&do_feeling);
  
  /* Current turn */
  rd_s32b(&turn);
  
  /* Read the player_hp array */
  rd_u16b(&tmp16u);
  
  /* Incompatible save files */
  if (tmp16u > PY_MAX_LEVEL)
    {
      note(format("Too many (%u) hitpoint entries!", tmp16u));
      return (25);
    }
  
  /* Read the player_hp array */
  for (i = 0; i < tmp16u; i++)
    {
      rd_s16b(&p_ptr->player_hp[i]);
    }
  
  
  /* Read spell info */
  rd_u32b(&p_ptr->spell_learned1);
  rd_u32b(&p_ptr->spell_learned2);
  rd_u32b(&p_ptr->spell_worked1);
  rd_u32b(&p_ptr->spell_worked2);
  rd_u32b(&p_ptr->spell_forgotten1);
  rd_u32b(&p_ptr->spell_forgotten2);
  
  for (i = 0; i < 64; i++)
    {
      rd_byte(&p_ptr->spell_order[i]);
    }
  
  /* Read sensation ID data */
  if (!older_than(1, 1, 0))
    {
      rd_u32b(&p_ptr->id_obj);
      rd_u32b(&p_ptr->id_other);
    }
  
  return (0);
}


/**
 * Read the notes. Every new savefile has at least NOTES_MARK.
 */
static errr rd_notes(void)
{
  int i = 0;
  int num;
  s32b tmp32s;

  char tmpstr[100];

  if (older_than(0, 3, 0))
    {  
      /* Ignore the notes */
      while (TRUE)
	{
	  rd_string(tmpstr, sizeof(tmpstr));
	  
	  /* Found the end? */
	  if (strstr(tmpstr, NOTES_MARK))
	    {
	      break;
	    }
	}
    }
  
  else
    {
      rd_string(notes_start, 120);
      
      rd_s32b(&tmp32s);
      num = tmp32s;
      
      for (i = 0; i < num; i++)
	{
	  rd_s32b(&notes[i].turn);
	  rd_s32b(&tmp32s);
	  notes[i].place = tmp32s;
	  rd_s32b(&tmp32s);
	  notes[i].level = (int)tmp32s;
	  rd_byte(&notes[i].type);
	  rd_string(notes[i].note, 120);
	}
    }
  
  return (0);
}




/**
 * Read the player inventory
 *
 * Note that the inventory changed in Angband 2.7.4.  Two extra
 * pack slots were added and the equipment was rearranged.  Note
 * that these two features combine when parsing old save-files, in
 * which items from the old "aux" slot are "carried", perhaps into
 * one of the two new "inventory" slots.
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(void)
{
  int slot = 0;
  
  object_type *i_ptr;
  object_type object_type_body;
  
  
  /* Read until done */
  while (1)
    {
      u16b n;
      
      /* Get the next item index */
      rd_u16b(&n);
      
      /* Nope, we reached the end */
      if (n == 0xFFFF) break;
      
      /* Get local object */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Read the item */
      rd_item(i_ptr);
      
      /* Hack -- verify item */
      if (!i_ptr->k_idx) return(53);
      
      /* Wield equipment */
      if (n >= INVEN_WIELD)
	{
	  /* Copy object */
	  object_copy(&inventory[n], i_ptr);
	  
	  /* Add the weight */
	  p_ptr->total_weight += (i_ptr->number * i_ptr->weight);
	  
	  /* One more item */
	  p_ptr->equip_cnt++;

	  /* Mark as worn for old savefiles */
	  if (older_than(1, 1, 0)) inventory[n].ident |= IDENT_WORN;

	  /* Notice dice and other obvious stuff */
	  notice_other(IF_DD_DS, n + 1);
	  inventory[n].id_obj |= ((inventory[n].flags_obj) & OF_OBVIOUS_MASK);
	}
      
      /* Warning -- backpack is full */
      else if (p_ptr->inven_cnt == INVEN_PACK)
	{
	  /* Oops */
	  note("Too many items in the inventory!");
	  
	  /* Fail */
	  return (54);
	}
      
      /* Carry inventory */
      else
	{
	  /* Get a slot */
	  n = slot++;
	  
	  /* Copy object */
	  object_copy(&inventory[n], i_ptr);
	  
	  /* Add the weight */
	  p_ptr->total_weight += (i_ptr->number * i_ptr->weight);
	  
	  /* One more item */
	  p_ptr->inven_cnt++;

	  /* Notice dice and other obvious stuff */
	  notice_other(IF_DD_DS, n + 1);
	  inventory[n].id_obj |= ((inventory[n].flags_obj) & OF_OBVIOUS_MASK);
	}
    }
  
  /* Success */
  return (0);
}



/**
 * Read the saved messages
 */
static void rd_messages(void)
{
  int i;
  char buf[128];
  u16b tmp16u;
  
  s16b num;
  
  /* Total */
  rd_s16b(&num);
  
  /* Read the messages */
  for (i = 0; i < num; i++)
    {
      /* Read the message */
      rd_string(buf, 128);
      
      /* Read the message type */
      rd_u16b(&tmp16u);
      
      /* Save the message */
      message_add(buf, tmp16u);
      
    }
}




/**
 * Read the dungeon
 *
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 *
 * Note that the size of the dungeon is now hard-coded to
 * DUNGEON_HGT by DUNGEON_WID, and any dungeon with another
 * size will be silently discarded by this routine.
 */
static errr rd_dungeon(void)
{
  int i, n, y, x;
  
  s16b stage;
  s16b last_stage;
  s16b py, px;
  s16b ymax, xmax;
  
  byte count;
  byte tmp8u;
  u16b tmp16u;
  
  u16b limit;
  
  
  /*** Basic info ***/
  
  /* Hack - rewrite stage_map if necessary */
  if (adult_dungeon)
    {
      for (i = 0; i < NUM_STAGES; i++)
	for (n = 0; n < 9; n++)
	  stage_map[i][n] = dungeon_map[i][n];
    }



  /* Header info */
  rd_s16b(&stage);
  rd_s16b(&last_stage);
  rd_s16b(&py);
  rd_s16b(&px);
  rd_s16b(&ymax);
  rd_s16b(&xmax);
  rd_u16b(&tmp16u);
  rd_u16b(&tmp16u);
  
  
  /* Ignore illegal dungeons */
  if ((stage < 0) || (stage >= NUM_STAGES))
    {
      note(format("Ignoring illegal stage (%d)", stage));
      return (0);
    }
  
  /* Ignore illegal dungeons */
  if ((ymax != DUNGEON_HGT) || (xmax != DUNGEON_WID))
    {
      /* XXX XXX XXX */
      note(format("Ignoring illegal dungeon size (%d,%d).", xmax, ymax));
      return (0);
    }
  
  /* Ignore illegal dungeons */
  if ((px < 0) || (px >= DUNGEON_WID) ||
      (py < 0) || (py >= DUNGEON_HGT))
    {
      note(format("Ignoring illegal player location (%d,%d).", px, py));
      return (1);
    }
  
  
  
  /*** Run length decoding ***/
  
  /* Load the dungeon data */
  for (x = y = 0; y < DUNGEON_HGT; )
    {
      /* Grab RLE info */
      rd_byte(&count);
      rd_byte(&tmp8u);
      
      /* Apply the RLE info */
      for (i = count; i > 0; i--)
	{
	  /* Extract "info" */
	  cave_info[y][x] = tmp8u;
	  
	  /* Advance/Wrap */
	  if (++x >= DUNGEON_WID)
	    {
	      /* Wrap */
	      x = 0;
	      
	      /* Advance/Wrap */
	      if (++y >= DUNGEON_HGT) break;
	    }
	}
    }

  if (!older_than(0,2,0))
    {  
      /* Load the dungeon data */
      for (x = y = 0; y < DUNGEON_HGT; )
	{
	  /* Grab RLE info */
	  rd_byte(&count);
	  rd_byte(&tmp8u);
	  
	  /* Apply the RLE info */
	  for (i = count; i > 0; i--)
	    {
	      /* Extract "info" */
	      cave_info2[y][x] = tmp8u;
	      
	      /* Advance/Wrap */
	      if (++x >= DUNGEON_WID)
		{
		  /* Wrap */
		  x = 0;
		  
		  /* Advance/Wrap */
		  if (++y >= DUNGEON_HGT) break;
		}
	    }
	}
    }
 
  /*** Run length decoding ***/
  
  /* Load the dungeon data */
  for (x = y = 0; y < DUNGEON_HGT; )
    {
      /* Grab RLE info */
      rd_byte(&count);
      rd_byte(&tmp8u);
      
      /* Apply the RLE info */
      for (i = count; i > 0; i--)
	{
	  /* Extract "feat" */
	  cave_set_feat(y, x, tmp8u);
	  
	  /* Advance/Wrap */
	  if (++x >= DUNGEON_WID)
	    {
	      /* Wrap */
	      x = 0;
	      
	      /* Advance/Wrap */
	      if (++y >= DUNGEON_HGT) break;
	    }
	}
    }
  
  
  /*** Player ***/
  
  /* Save stage */
  p_ptr->stage = (older_than(0, 2, 2) ? conv_stage(stage) : stage);
  p_ptr->last_stage = (older_than(0, 2, 2) ? conv_stage(last_stage) : 
		       last_stage);
  
  /* Place player in dungeon */
  if (!player_place(py, px))
    {
      note(format("Cannot place player (%d,%d)!", py, px));
      return (162);
    }
  
  /* Hack -- Repair dungeon level */
  if (stage_map[p_ptr->stage][LOCALITY] == UNDERWORLD) 
    {
      stage_map[p_ptr->stage][UP] = p_ptr->last_stage;
      stage_map[p_ptr->last_stage][DOWN] = p_ptr->stage;
      stage_map[p_ptr->stage][DEPTH] = stage_map[p_ptr->last_stage][DEPTH] + 1;
    }
  else if (stage_map[p_ptr->stage][LOCALITY] == MOUNTAIN_TOP) 
    {
      stage_map[p_ptr->stage][DOWN] = p_ptr->last_stage;
      stage_map[p_ptr->last_stage][UP] = p_ptr->stage;
      stage_map[p_ptr->stage][DEPTH] = stage_map[p_ptr->last_stage][DEPTH] + 1;
    }

  
  /*** Objects ***/
  
  /* Read the item count */
  rd_u16b(&limit);
  
  /* Verify maximum */
  if (limit >= z_info->o_max)
    {
      note(format("Too many (%d) object entries!", limit));
      return (151);
    }
  
  /* Read the dungeon items */
  for (i = 1; i < limit; i++)
    {
      object_type *i_ptr;
      object_type object_type_body;
      
      
      /* Acquire place */
      i_ptr = &object_type_body;
      
      /* Wipe the object */
      object_wipe(i_ptr);
      
      /* Read the item */
      rd_item(i_ptr);
      
      
      /* Monster */
      if (i_ptr->held_m_idx)
	{
	  /* Give the object to the monster */
	  if (!monster_carry(i_ptr->held_m_idx, i_ptr))
	    {
	      note(format("Cannot place object %d!", o_max));
	      return (152);
	    }
	}
      
      /* Dungeon */
      else
	{
	  /* Give the object to the floor */
	  if (!floor_carry(i_ptr->iy, i_ptr->ix, i_ptr))
	    {
	      note(format("Cannot place object %d!", o_max));
	      return (152);
	    }
	}
    }
  
  
  /*** Monsters ***/
  
  /* Read the monster count */
  rd_u16b(&limit);
  
  /* Hack -- verify */
  if (limit >= z_info->m_max)
    {
      note(format("Too many (%d) monster entries!", limit));
      return (161);
    }
  
  /* Read the monsters */
  for (i = 1; i < limit; i++)
    {
      monster_type *n_ptr;
      monster_type monster_type_body;
      monster_race *r_ptr;
      
      int r_idx;
      
      /* Get local monster */
      n_ptr = &monster_type_body;
      
      /* Clear the monster */
      (void)WIPE(n_ptr, monster_type);
      
      /* Read the monster */
      rd_monster(n_ptr);
      
      /* Hack -- ignore "broken" monsters */
      if (n_ptr->r_idx <= 0) continue;
      
      /* Hack -- no illegal monsters. */
      if (n_ptr->r_idx >= z_info->r_max) continue;
      
      
      /* Access the "r_idx" of the chosen monster */
      r_idx = n_ptr->r_idx;
      
      /* Access the actual race */
      r_ptr = &r_info[r_idx];
      
      /* If a player ghost, some special features need to be added. */
      if (r_ptr->flags2 & (RF2_PLAYER_GHOST))
	{
	  prepare_ghost(n_ptr->r_idx, n_ptr, TRUE);
	}
      
      /* Place monster in dungeon */
      if (!monster_place(n_ptr->fy, n_ptr->fx, n_ptr))
	{
	  note(format("Cannot place monster %d", i));
	  return (162);
	}
      
      /* mark minimum range for recalculation */
		n_ptr->min_range = 0;
    }
  
  /*
   * Attach objects carried by a monster to the monster again.
   * We look for the each object in o_list[] that is carried by
   * a monster. If the monster isn't carrying any object yet,
   * then assign it the object. The object with the highest
   * o_idx is assumed to be at the head of the list of objects
   * carried by a monster. -TNB-
   */
  for (i = o_max; i > 0; i--)
    {
      object_type *o_ptr = &o_list[i];
      if (!o_ptr->held_m_idx) continue;
      if (m_list[o_ptr->held_m_idx].hold_o_idx) continue;
      m_list[o_ptr->held_m_idx].hold_o_idx = i;
    }
  
  /*** Success ***/
  
  /* The dungeon is ready */
  character_dungeon = TRUE;
  
  /* Success */
  return (0);
}



/**
 * Actually read the savefile
 */
static errr rd_savefile_new_aux(void)
{
  int i, j, k;
  
  int total_artifacts = 0;
  int random_artifacts = 0;
  
  bool need_random_artifacts = FALSE;
  
  byte tmp8u;
  u16b num, tmp16u;
  u32b tmp32u;
  s32b tmp32s;
  
  
#ifdef VERIFY_CHECKSUMS
  u32b n_x_check, n_v_check;
  u32b o_x_check, o_v_check;
#endif
  
  
  /* Mention the savefile version if not current */
  if (older_than(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH))
    {
      msg_format("Loading an FAangband %d.%d.%d savefile.", 
		 sf_major, sf_minor, sf_patch);
      msg_print(NULL);
    }
  
  
  /* Strip the Angband version bytes */
  strip_bytes(4);
  
  /* Hack -- decrypt */
  xor_byte = sf_extra;
  
  
  /* Clear the checksums */
  v_check = 0L;
  x_check = 0L;
  
  
  /* Operating system info */
  rd_u32b(&sf_xtra);
  
  /* Time of savefile creation */
  rd_u32b(&sf_when);
  
  /* Number of resurrections */
  rd_u16b(&sf_lives);
  
  /* Number of times played */
  rd_u16b(&sf_saves);
  
  /* Later use (always zero) */
  rd_u32b(&tmp32u);
  
  
  /* Read RNG state */
  rd_randomizer();
  if (arg_fiddle) note("Loaded Randomizer Info");
  
  
  /* Then the options */
  rd_options();
  if (arg_fiddle) note("Loaded Option Flags");
  
  /* Then the "messages" */
  rd_messages();
  if (arg_fiddle) note("Loaded Messages");
  
  
  /* Monster Memory */
  rd_u16b(&tmp16u);
  
  /* Incompatible save files */
  if (tmp16u > z_info->r_max)
    {
      note(format("Too many (%u) monster races!", tmp16u));
      return (21);
    }
  
  
  
  /* Read the available records */
  for (i = 0; i < tmp16u; i++)
    {
      monster_race *r_ptr;
      monster_lore *l_ptr;
      
      j = i;
      
      /* Read the lore */
      rd_lore(j);
      
      /* Access that monster */
      r_ptr = &r_info[j];
      l_ptr = &l_list[j];
      
    }
  if (arg_fiddle) note("Loaded Monster Memory");
  
  
  /* Object Memory */
  rd_u16b(&tmp16u);
  
  /* Incompatible save files */
  if (tmp16u > z_info->k_max)
    {
      note(format("Too many (%u) object kinds!", tmp16u));
      return (22);
    }
  
  
  /* Read the object memory */
  for (i = 0; i < tmp16u; i++)
    {
      byte tmp8u;
      int obj_index;
      
      object_kind *k_ptr;
      
      /* Object indexes changed in Oangband 0.3.6.  Convert old to new. */
      obj_index = i;
      
      
      /* Obtain the "kind" template */
      k_ptr = &k_info[obj_index];
      
      rd_byte(&tmp8u);
      
      k_ptr->aware = (tmp8u & 0x01) ? TRUE: FALSE;
      k_ptr->tried = (tmp8u & 0x02) ? TRUE: FALSE;
      k_ptr->known_effect = (tmp8u & 0x04) ? TRUE: FALSE;
      k_ptr->squelch = (tmp8u & 0x08) ? TRUE: FALSE;

      if (!older_than(0, 3, 0))
      {
        rd_byte(&tmp8u);
        
        k_ptr->everseen = (tmp8u & 0x01) ? TRUE: FALSE;
      }
    }
  if (arg_fiddle) note("Loaded Object Memory");
  
  
  /* Load the Quests */
  rd_u16b(&tmp16u);
  
  /* Incompatible save files */
  if (tmp16u > MAX_Q_IDX)
    {
      note(format("Too many (%u) quests!", tmp16u));
      return (23);
    }
  
  num = tmp16u;
  
  /* Load the Quests */
  for (i = 0; i < num; i++)
    {
      rd_u16b(&tmp16u);
      q_list[i].stage = tmp16u;
      rd_byte(&tmp8u);
      rd_byte(&tmp8u);
    }
  if (arg_fiddle) note("Loaded Quests");
  
  
  /* Load the Artifacts */
  rd_u16b(&tmp16u);
  total_artifacts = tmp16u;
  
  rd_u16b(&tmp16u);
  random_artifacts = tmp16u;
  
  
  /* Incompatible save files */
  if (total_artifacts > z_info->a_max)
    {
      note(format("Too many (%u) artifacts!", total_artifacts));
      return (24);
    }
  
  
  /* Read the artifact info. */
  for (i = 0; i < total_artifacts; i++)
    {
      /* Most regular artifact info is stored in a_info.raw.  0.3.X 
       * savefiles have 127 regular artifacts, with indexes that need 
       * converting, and newer savefiles have 209, with up-to-date 
       * indexes.
       */
      if (i < ART_MIN_RANDOM)
	{
	  rd_s32b(&tmp32s);

	  /* This is a silly turn if the savefile is older than 0.2.3 */
	  a_info[i].creat_turn = tmp32s;
	  if (!older_than(1, 0, 0))
	    {
	      rd_byte(&tmp8u);
	      a_info[i].p_level = tmp8u;
	      rd_byte(&tmp8u);
	      a_info[i].set_bonus = (tmp8u ? TRUE : FALSE);
	    }
	}
      
      
      /* But random artifacts are specific to each player. */
      else
	{
	  /* Hack - Shift random artifacts for 0.3.X savefiles. */
	  j = i;
	  
	  rd_u16b(&tmp16u);
	  a_info[j].name = tmp16u;
	  rd_u16b(&tmp16u);
	  a_info[j].text = tmp16u;
	  
	  rd_byte(&tmp8u);
	  a_info[j].tval = tmp8u;
	  rd_byte(&tmp8u);
	  a_info[j].sval = tmp8u;
	  rd_u16b(&tmp16u);
	  a_info[j].pval = tmp16u;
	  
	  rd_u16b(&tmp16u);
	  a_info[j].to_h = tmp16u;
	  rd_u16b(&tmp16u);
	  a_info[j].to_d = tmp16u;
	  rd_u16b(&tmp16u);
	  a_info[j].to_a = tmp16u;
	  
	  rd_byte(&tmp8u);
	  a_info[j].dd = tmp8u;
	  rd_byte(&tmp8u);
	  a_info[j].ds = tmp8u;
	  
	  rd_u16b(&tmp16u);
	  a_info[j].ac = tmp16u;
	  rd_u16b(&tmp16u);
	  a_info[j].weight = tmp16u;
	  
	  rd_u32b(&tmp32u);
	  a_info[j].cost = tmp32u;
	  
	  rd_u32b(&tmp32u);
	  a_info[j].flags_obj = tmp32u;
	  rd_u32b(&tmp32u);
	  a_info[j].flags_curse = tmp32u;
	  rd_u32b(&tmp32u);
	  a_info[j].flags_kind = tmp32u;
	  
	  rd_byte(&tmp8u);
	  a_info[j].level = tmp8u;
	  rd_byte(&tmp8u);
	  a_info[j].rarity = tmp8u;
	  if (older_than(0, 2, 3))
	    {
	      rd_byte(&tmp8u);

	      /* Silly turn */
	      a_info[j].creat_turn = tmp8u;
	    }
	  else
	    {
	      rd_s32b(&tmp32s);
	      a_info[j].creat_turn = tmp32s;
	    }
	  if (!older_than(1, 0, 0))
	    {
	      rd_byte(&tmp8u);
	      a_info[j].p_level = tmp8u;
	    }
	  rd_byte(&tmp8u);
	  a_info[j].activation = tmp8u;

	  if (!older_than(0, 3, 0))
	    {
	      /* New percentage resists -NRM- */
	      for (k = 0; k < MAX_P_RES; k++)
		{
		  rd_byte(&tmp8u);
		  a_info[j].percent_res[k] = tmp8u;
		}
	    }
	  
	  if (!older_than(1, 0, 0))
	    {
	      for (k = 0; k < A_MAX; k++)
		{
		  rd_byte(&tmp8u);
		  if (tmp8u < 129) a_info[j].bonus_stat[k] = (int)tmp8u;
		  else a_info[j].bonus_stat[k] = (int)(tmp8u - 256);
		}
	      for (k = 0; k < MAX_P_BONUS; k++)
		{
		  rd_byte(&tmp8u);
		  if (tmp8u < 129) a_info[j].bonus_other[k] = (int)tmp8u;
		  else a_info[j].bonus_other[k] = (int)(tmp8u - 256);
		}
	      for (k = 0; k < MAX_P_SLAY; k++)
		{
		  rd_byte(&tmp8u);
		  a_info[j].multiple_slay[k] = tmp8u;
		}
	      for (k = 0; k < MAX_P_BRAND; k++)
		{
		  rd_byte(&tmp8u);
		  a_info[j].multiple_brand[k] = tmp8u;
		}
	    } 
	      
	  /* Extra space. */
	  rd_u32b(&tmp32u);
	}
    }
  if (arg_fiddle) note("Loaded artifacts.");
  
  
  
  /* If random artifacts do not need to be generated, read the 
   * random artifact names, and add them to the a_name array.
   */
  if (!need_random_artifacts)
    {
      u16b num_of_random_arts;
      
      /* Find out how many random artifacts have stored names. */
      rd_u16b(&num_of_random_arts);
      
      /* Verify that number, and warn the chap who modified the game that 
       * he still has work to do on failure.
       */
      if (num_of_random_arts != (z_info->a_max - ART_MIN_RANDOM))
	{
	  note(format("Number of stored random artifact names (%d) does not", 
		      num_of_random_arts));
	  note(format("match the number of random artifacts in your copy of FAangband (%d).", z_info->a_max - ART_MIN_RANDOM));
	}
      
      
      /* Otherwise, add the new names to the a_name structure. */
      else
	{
	  int err = convert_saved_names();
	  
	  /* Complain if naming fails. */
	  if (err) note("Warning - random artifact naming failed!");
	}
    }
  
  /* Read the extra stuff */
  if (rd_extra()) 
    return (25);
  
  if (arg_fiddle) note("Loaded extra information");
  /* Read the notes if after version 0.1.0 */
  if (!older_than(0, 1, 1)) 
    if (rd_notes()) 
      {
	note("Unable to read notes");
	return (25);
      }
  
  if (arg_fiddle) note("Loaded Notes");
  
  /* Important -- Initialize the sex */
  sp_ptr = &sex_info[p_ptr->psex];
  
  /* Important -- Initialize the race/class */
  rp_ptr = &p_info[p_ptr->prace];
  cp_ptr = &c_info[p_ptr->pclass];
  
  /* Important -- Initialize the magic */
  mp_ptr = &magic_info[p_ptr->pclass];
  
  /* Now that the player information is known, we can generate a set 
   * of random artifacts, if needed.
   */
  if (need_random_artifacts)
    {
      initialize_random_artifacts();
      if (arg_fiddle) note("Generated random artifacts");
    }
  
  
  /* Read the inventory */
  if (rd_inventory())
    {
      note("Unable to read inventory");
      return (21);
    }
  
  /* Read the stores */
  rd_u16b(&tmp16u);
  for (i = 0; i < tmp16u; i++)
    {
      if (rd_store(i)) return (22);
    }
  if (older_than(0,2,0))
    {
      /* Translating the stores - we need to preserve the home inventory */
      int home_no = 0, t, num;
      object_type tmp_item[STORE_INVEN_MAX];

      /* Save the home info */
      store_type *st_ptr = &store[STORE_HOME];
      
      /* Get the number of items */
      num = st_ptr->stock_num;
  
      /* Read the items */
      for (j = 0; j < num; j++)
	{
	  /* Copy the item */
	  object_copy(&tmp_item[j], &st_ptr->stock[j]);
	}

      /* Reinitialise the stores */
      store_init();

      /* Find the home index */
      for (t = 0; t < NUM_TOWNS; t++)
	{
	  if (p_ptr->home == towns[t])
	    {
	      home_no += ((t < NUM_TOWNS_SMALL) ? 3 : STORE_HOME);
	      break;
	    }
	  else
	    home_no += 
	      ((t < NUM_TOWNS_SMALL) ? MAX_STORES_SMALL : MAX_STORES_BIG);
	}
      if (home_no == MAX_STORES)
	/* Oops */
	msg_print("Home inventory lost!");
      else
	{
	  /* Read the items */
	  for (j = 0; j < num; j++)
	    {
	      /* Copy the item */
	      object_copy(&st_ptr->stock[j], &tmp_item[j]);
	    }
	  
	}
    }
  
  /* I'm not dead yet... */
  if (!p_ptr->is_dead)
    {
      /* Dead players have no dungeon */
      note("Restoring Dungeon...");
      if (rd_dungeon())
	{
	  note("Error reading dungeon data");
	  return (34);
	}
      
      /* Read the ghost info */
      rd_ghost();
    }
  
  /* Otherwise optionally look at the dead one, quickstart */
  else
    {
      event_type answer;
      
      /* Clear screen */
      Term_clear();
      
      prt("Would you like to see details of your", 15, 0);
      prt("recently dead character? (yes/no)", 16, 0);
      update_statusline();      
      add_button("Yes", 'y');
      add_button("No", 'n');
      calc_bonuses(FALSE);
      update_statusline();      
      
      answer = inkey_ex();
      
      if ((answer.key == 'Y') || (answer.key == 'y'))
	{
	  /* Buttons */
	  kill_button('y');
	  kill_button('n');
	  update_statusline();
          
	  /* Display */
	  reset_visuals(FALSE);
	  do_cmd_change_name();
        }
  
      /* Clear screen */
      Term_clear();
#if 0
      /* Quickstart */
      prt("Would you like to start a new character", 15, 0);
      prt("based on your last character? (yes/no)", 16, 0);
      add_button("Yes", 'y');
      add_button("No", 'n');
      update_statusline();      
      
      answer = inkey_ex();
      
      if ((answer.key == 'Y') || (answer.key == 'y'))
        {
	  /* Buttons */
	  kill_button('y');
	  kill_button('n');
	  update_statusline();
          
	  /* Set to quickstart */
	  character_quickstart = TRUE;
        }
      
      /* Clear screen */
      Term_clear();
#endif
    } 
  
#ifdef VERIFY_CHECKSUMS
  
  /* Save the checksum */
  n_v_check = v_check;
  
  /* Read the old checksum */
  rd_u32b(&o_v_check);
  
  /* Verify */
  if (o_v_check != n_v_check)
    {
      note("Invalid checksum");
	    return (11);
    }
  
  /* Save the encoded checksum */
  n_x_check = x_check;
  
  /* Read the checksum */
  rd_u32b(&o_x_check);
  
  /* Verify */
  if (o_x_check != n_x_check)
    {
      note("Invalid encoded checksum");
      return (11);
    }
  
  
#endif
  
  
  
  /* Success */
  return (0);

}


/**
 * Actually read the savefile
 */
errr rd_savefile_new(void)
{
  errr err;
  
  /* The savefile is a binary file */
  fff = file_open(savefile, MODE_READ, FTYPE_RAW);
  
  /* Paranoia */
  if (!fff) return (-1);
  
  /* Call the sub-function */
  err = rd_savefile_new_aux();
  
  /* Close the file */
  file_close(fff);
  
  /* Result */
  return (err);
}
 

/**
 * Open the savefile chosen earlier, and read and save Angband and 
 * Oangband version information.  This code is awfully hackish, but it 
 * can be adopted for any additional variant version information. -LM-
 */
errr rd_version_info(void)
{
  ang_file *fd;
  
  /* Open the savefile */
  fd = file_open(savefile, MODE_READ, -1);
  
  /* No file.  Report error. */
  if (!fd) return (-1);
  
  file_read(fd, (char*)&sf_major, 1);
  file_read(fd, (char*)&sf_minor, 1);
  file_read(fd, (char*)&sf_patch, 1);
  file_read(fd, (char*)&sf_extra, 1);
  
  /* Close the file */
  file_close(fd);
  
  /* Success. */
  return(0);
}
 
