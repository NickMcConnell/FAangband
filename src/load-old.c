/** \file load-old.c
    \brief Old savefile handling.
 

    * This is now just a stub which reads as far as the monster memory and then 
    * exits.  
    *
    *
    * Copyright (c) 2011 Nick McConnell, Leon Marrick, Bahman Rabii, 
    * Ben Harrison
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

/**
 * Local "savefile" pointer
 */
static ang_file	*fff;

/**
 * Hack -- old "encryption" byte
 */
static byte	xor_byte;

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
 * Avoid the top two lines, to avoid interference with "msg()".
 */
static void note(const char *msg)
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
 * Read the monster lore
 */
static void rd_lore(int r_idx)
{
    size_t i;
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
    rd_byte(&tmp8u);
    rd_byte(&l_ptr->cast_spell);
  
    /* Spell types merged now */
    l_ptr->cast_spell += tmp8u;

    /* Count blows of each type */
    rd_byte(&l_ptr->blows[0]);
    rd_byte(&l_ptr->blows[1]);
    rd_byte(&l_ptr->blows[2]);
    rd_byte(&l_ptr->blows[3]);
  
    /* Hack - XXX - MarbleDice - Maximum saveable flags = 96 */
    for (i = 0; i < 12 && i < RF_SIZE; i++)
	rd_byte(&l_ptr->flags[i]);
    if (i < 12) strip_bytes(RF_SIZE - i);

    /* Hack - XXX - MarbleDice - Maximum saveable flags = 96 */
    /* Note the old flags7 was never written, so summoning was forgotten */
    for (i = 0; i < 12 && i < RSF_SIZE; i++)
	rd_byte(&l_ptr->spell_flags[i]);
    if (i < 12) strip_bytes(12 - i);

    /* Read the "Racial" monster limit per level */
    rd_byte(&r_ptr->max_num);
  
    /* Later (?) */
    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
  
    /* Repair the spell lore flags */
    rsf_inter(l_ptr->spell_flags, r_ptr->spell_flags);
}






/**
 * Read RNG state (added in 2.8.0)
 */
static void rd_randomizer(void)
{
    int i;
    u32b noop;

    /* current value for the simple RNG */
    rd_u32b(&Rand_value);

    /* state index */
    rd_u32b(&state_i);

    /* for safety, make sure state_i < RAND_DEG */
    state_i = state_i % RAND_DEG;
    
    /* RNG variables */
    rd_u32b(&z0);
    rd_u32b(&z1);
    rd_u32b(&z2);
    
    /* RNG state */
    for (i = 0; i < RAND_DEG; i++)
	rd_u32b(&STATE[i]);

    /* NULL padding */
    for (i = 0; i < 59 - RAND_DEG; i++)
	rd_u32b(&noop);

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
  
    if (older_than(1, 4, 0))
    {
	/* Read the option flags */
	for (n = 0; n < 8; n++) rd_u32b(&flag[n]);
  
	/* Read the option masks */
	for (n = 0; n < 8; n++) rd_u32b(&mask[n]);
  
	/* Analyze the options */
	for (i = 0; i < OPT_MAX; i++)
	{
	    int os = i / 32;
	    int ob = i % 32;
      
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
    else
    {
	while (1) 
	{
	    byte value;
	    char name[20];
	    rd_string(name, sizeof name);
	    
	    if (!name[0])
		break;
	    
	    rd_byte(&value);
	    option_set(name, !!value);
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
  
    /* Set up the subwindows */
    subwindows_set_flags(flag, ANGBAND_TERM_MAX);

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
 * Actually read the savefile
 */
static errr rd_savefile_new_aux(void)
{
    int i, j;
  
    u16b tmp16u;
  
    byte sf_major = 0;
    byte sf_minor = 0;
    byte sf_patch = 0;
    byte sf_extra = 0;

    file_read(fff, (char*)&sf_major, 1);
    file_read(fff, (char*)&sf_minor, 1);
    file_read(fff, (char*)&sf_patch, 1);
    file_read(fff, (char*)&sf_extra, 1);
  
    /* Mention the savefile version if not current */
    if (older_than(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH))
    {
	msg("Loading an FAangband %d.%d.%d savefile.", 
		   sf_major, sf_minor, sf_patch);
	message_flush();
    }

    if (!get_check("Load your monster and object memory and then start a new character?"))
	return (-1);
  
    /* Strip the Angband version bytes */
    //strip_bytes(4);
  
    /* Hack -- decrypt */
    xor_byte = sf_extra;
  
    /* Strip more junk */
    strip_bytes(16);
  
    /* Read RNG state */
    rd_randomizer();
  
  
    /* Then the options */
    rd_options();
  
    /* Then the "messages" */
    rd_messages();
  
    /* Monster Memory */
    rd_u16b(&tmp16u);
  
    /* Incompatible save files */
    if (tmp16u > z_info->r_max)
    {
	note(format("Too many (%u) monster races!", tmp16u));
	return (-1);
    }
  
    /* Read the available records */
    for (i = 0; i < tmp16u; i++)
    {
	j = i;
      
	/* Read the lore */
	rd_lore(j);
    }
  
    /* Object Memory */
    rd_u16b(&tmp16u);
  
    /* Incompatible save files */
    if (tmp16u > z_info->k_max)
    {
	note(format("Too many (%u) object kinds!", tmp16u));
	return (-1);
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

	rd_byte(&tmp8u);
        
	k_ptr->everseen = (tmp8u & 0x01) ? TRUE: FALSE;
    }
  
    /* Hack - kill player, allow load */
    p_ptr->chp = -1;
    p_ptr->is_dead = TRUE;
    turn = 1;

    return 0;
}


/**
 * Actually read the savefile
 */
int rd_savefile_old(void)
{
    int err;
  
    /* The savefile is a binary file */
    safe_setuid_grab();
    fff = file_open(savefile, MODE_READ, FTYPE_RAW);
    safe_setuid_drop();
  
    /* Paranoia */
    if (!fff) return (-1);
  
    /* Call the sub-function */
    err = rd_savefile_new_aux();
  
    /* Close the file */
    file_close(fff);
  
    /* Result */
    return (err);
}
 

