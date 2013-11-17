/** \file wizard1.c 
    \brief Spoilers

 * Generation of object, artifact, and monster spoilers.
 *
 * This file has been updated to work with Oangband 0.5.1.
 *
 * Copyright (c) 1997 Ben Harrison, and others
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
#include "cmds.h"
#include "monster.h"

#ifdef ALLOW_SPOILERS


/**
 * The spoiler file being created
 */
static ang_file *fff = NULL;






/*
 * Item Spoilers by Ben Harrison (benh@phial.com)
 */






/**
 * Describe the kind
 */
static void kind_info(char *buf, char *dam, char *wgt, int *lev,
					  s32b * val, int k)
{
	object_kind *k_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	/* Hack */
	size_t buf_len = 80;

	/* Get local object */
	i_ptr = &object_type_body;

	/* Prepare a fake item */
	object_prep(i_ptr, k, MAXIMISE);

	/* Obtain the "kind" info */
	k_ptr = &k_info[i_ptr->k_idx];

	/* It is known */
	i_ptr->ident |= (IDENT_KNOWN);

	/* Cancel bonuses */
	i_ptr->pval = 0;
	i_ptr->to_a = 0;
	i_ptr->to_h = 0;
	i_ptr->to_d = 0;


	/* Level */
	(*lev) = k_ptr->level;

	/* Value */
	(*val) = object_value(i_ptr);


	/* Hack */
	if (!buf || !dam || !wgt)
		return;


	/* Description (too brief) */
	object_desc(buf, buf_len, i_ptr, ODESC_BASE | ODESC_SPOIL);


	/* Misc info */
	strcpy(dam, "");

	/* Damage */
	switch (i_ptr->tval) {
		/* Bows */
	case TV_BOW:
		{
			break;
		}

		/* Ammo */
	case TV_SHOT:
	case TV_BOLT:
	case TV_ARROW:
		{
			sprintf(dam, "%dd%d", i_ptr->dd, i_ptr->ds);
			break;
		}

		/* Weapons */
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
		{
			sprintf(dam, "%dd%d", i_ptr->dd, i_ptr->ds);
			break;
		}

		/* Armour */
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_CLOAK:
	case TV_CROWN:
	case TV_HELM:
	case TV_SHIELD:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
		{
			sprintf(dam, "%d", i_ptr->ac);
			break;
		}
	}


	/* Weight */
	sprintf(wgt, "%3d.%d", i_ptr->weight / 10, i_ptr->weight % 10);

}


/**
 * Create a spoiler file for items
 */
static void spoil_obj_desc(const char *fname)
{
	int i, k, s, t, n = 0;

	u16b who[200];

	char buf[1024];

	char wgt[80];
	char dam[80];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Open the file */
	fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fff) {
		msg("Cannot create spoiler file.");
		return;
	}


	/* Header */
	file_putf(fff, "Spoiler File -- Basic Items (2.?.?)\n\n\n");

	/* More Header */
	file_putf(fff, "%-45s     %8s%7s%5s%9s\n",
			  "Description", "Dam/AC", "Wgt", "Lev", "Cost");
	file_putf(fff, "%-45s     %8s%7s%5s%9s\n",
			  "----------------------------------------",
			  "------", "---", "---", "----");

	/* List the groups */
	for (i = 0; TRUE; i++) {
		/* Write out the group title */
		if (group_item[i].name) {
			/* Hack -- bubble-sort by cost and then level */
			for (s = 0; s < n - 1; s++) {
				for (t = 0; t < n - 1; t++) {
					int i1 = t;
					int i2 = t + 1;

					int e1;
					int e2;

					s32b t1;
					s32b t2;

					kind_info(NULL, NULL, NULL, &e1, &t1, who[i1]);
					kind_info(NULL, NULL, NULL, &e2, &t2, who[i2]);

					if ((t1 > t2) || ((t1 == t2) && (e1 > e2))) {
						int tmp = who[i1];
						who[i1] = who[i2];
						who[i2] = tmp;
					}
				}
			}

			/* Spoil each item */
			for (s = 0; s < n; s++) {
				int e;
				s32b v;

				/* Describe the kind */
				kind_info(buf, dam, wgt, &e, &v, who[s]);

				/* Dump it */
				file_putf(fff, "     %-45s%8s%7s%5d%9ld\n",
						  buf, dam, wgt, e, (long) (v));
			}

			/* Start a new set */
			n = 0;

			/* Notice the end */
			if (!group_item[i].tval)
				break;

			/* Start a new set */
			file_putf(fff, "\n\n%s\n\n", group_item[i].name);
		}

		/* Acquire legal item types */
		for (k = 1; k < z_info->k_max; k++) {
			object_kind *k_ptr = &k_info[k];

			/* Skip wrong tval's */
			if (k_ptr->tval != group_item[i].tval)
				continue;

			/* Hack -- Skip instant-artifacts */
			if (kf_has(k_ptr->flags_kind, KF_INSTA_ART))
				continue;

			/* Save the index */
			who[n++] = k;
		}
	}


	/* Check for errors */
	if (!file_close(fff)) {
		msg("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg("Successfully created a spoiler file.");
}



/*
 * Artifact Spoilers by: randy@PICARD.tamu.edu (Randy Hutson)
 */


/**
 * Returns a "+" string if a number is non-negative and an empty
 * string if negative
 */
#define POSITIZE(v) (((v) >= 0) ? "+" : "")

/**
 * These are used to format the artifact spoiler file. INDENT1 is used
 * to indent all but the first line of an artifact spoiler. INDENT2 is
 * used when a line "wraps". (Bladeturner's resistances cause this.)
 */
#define INDENT1 "    "
#define INDENT2 "      "

/**
 * MAX_LINE_LEN specifies when a line should wrap.
 */
#define MAX_LINE_LEN 75


/**
 * The artifacts categorized by type
 */
static grouper group_artifact[] = {
	{TV_SWORD, "Edged Weapons"},
	{TV_POLEARM, "Polearms"},
	{TV_HAFTED, "Hafted Weapons"},
	{TV_BOW, "Bows"},

	{TV_SOFT_ARMOR, "Body Armor"},
	{TV_HARD_ARMOR, NULL},
	{TV_DRAG_ARMOR, NULL},

	{TV_CLOAK, "Cloaks"},
	{TV_SHIELD, "Shields"},
	{TV_HELM, "Helms/Crowns"},
	{TV_CROWN, NULL},
	{TV_GLOVES, "Gloves"},
	{TV_BOOTS, "Boots"},

	{TV_LIGHT, "Light Sources"},
	{TV_AMULET, "Amulets"},
	{TV_RING, "Rings"},

	{0, NULL}
};



/**
 * Pair together a constant flag with a textual description.
 *
 * Used by both "init.c" and "wiz-spo.c".
 *
 * Note that it sometimes more efficient to actually make an array
 * of textual names, where entry 'N' is assumed to be paired with
 * the flag whose value is "1L << N", but that requires hard-coding.
 */

typedef struct flag_desc flag_desc;

struct flag_desc {
	const int flag;
	const char *const desc;
};



/**
 * These are used for "+3 to STR, DEX", etc. These are separate from
 * the other pval affected traits to simplify the case where an object
 * affects all stats.  In this case, "All stats" is used instead of
 * listing each stat individually.
 */

const char *stat_desc[] = {
	"STR",
	"INT",
	"WIS",
	"DEX",
	"CON",
	"CHR"
};

/**
 * Besides stats, these are the other player traits
 * which may be affected by an object's pval
 */

const char *bonus_desc[] = {
	"Magical Item Skill",
	"Stealth",
	"Searching",
	"Infravision",
	"Tunnelling",
	"Speed",
	"Extra Shots",
	"Extra Might"
};


/**
 * Slaying preferences for weapons
 */

const char *slay_desc[] = {
	"Animal",
	"Evil",
	"Undead",
	"Demon",
	"Orc",
	"Troll",
	"Giant",
	"Dragon"
};

/**
 * Elemental brands for weapons
 */
const char *brand_desc[] = {
	"Acid Brand",
	"Lightning Brand",
	"Flame Tongue",
	"Frost Brand",
	"Poison Brand"
};

/**
 * The basic resistances
 */

const char *resist_desc[] = {
	"Acid",
	"Lightning",
	"Fire",
	"Cold",
	"Poison",
	"Light",
	"Dark",
	"Confusion",
	"Sound",
	"Shards",
	"Nexus",
	"Nether",
	"Chaos",
	"Disenchantment"
};

/**
 * Sustain stats -  these are given their "own" line in the
 * spoiler file, mainly for simplicity
 */
static const flag_desc sustain_flags_desc[] = {
	{OF_SUSTAIN_STR, "STR"},
	{OF_SUSTAIN_INT, "INT"},
	{OF_SUSTAIN_WIS, "WIS"},
	{OF_SUSTAIN_DEX, "DEX"},
	{OF_SUSTAIN_CON, "CON"},
	{OF_SUSTAIN_CHR, "CHR"},
};

static flag_desc flags_obj_desc[] = {
	{OF_THROWING, "Throwing weapon"},
	{OF_PERFECT_BALANCE, "Well-balanced"},
	{OF_SLOW_DIGEST, "Slow Digestion"},
	{OF_FEATHER, "Feather Falling"},
	{OF_LIGHT, "Permanent Light"},
	{OF_REGEN, "Regeneration"},
	{OF_TELEPATHY, "ESP"},
	{OF_SEE_INVIS, "See Invisible"},
	{OF_FREE_ACT, "Free Action"},
	{OF_HOLD_LIFE, "Hold Life"},
	{OF_FEARLESS, "Fearlessness"},
	{OF_SEEING, "Blindness resistance"},
	{OF_BLESSED, "Blessed Blade"},
	{OF_IMPACT, "Earthquake impact on hit"},
	{OF_PERMA_CURSE, "Permanently cursed"}
};


static flag_desc flags_curse_desc[] = {
	{CF_TELEPORT, "Induces random teleportation"},
	{CF_NO_TELEPORT, "Prevents teleportation"},
	{CF_AGGRO_PERM, "Aggravates"},
	{CF_AGGRO_RAND, "Aggravates randomly"},
	{CF_SLOW_REGEN, "Slows regeneration"},
	{CF_AFRAID, "Causes fear"},
	{CF_HUNGRY, "Fast digestion"},
	{CF_POIS_RAND, "Randomly poisons"},
	{CF_POIS_RAND_BAD, "Releases toxic clouds"},
	{CF_CUT_RAND, "Randomly cuts"},
	{CF_CUT_RAND_BAD, "Causes serious wounds"},
	{CF_HALLU_RAND, "Induces hallucination"},
	{CF_DROP_WEAPON, "Drops randomly"},
	{CF_ATTRACT_DEMON, "Summons demons"},
	{CF_ATTRACT_UNDEAD, "Calls undead"},
	{CF_STICKY_CARRY, "Cannot be removed"},
	{CF_STICKY_WIELD, "Cannot be dropped"},
	{CF_PARALYZE, "Paralyzes"},
	{CF_PARALYZE_ALL, "Paralyzes unresistably"},
	{CF_DRAIN_EXP, "Drains experience"},
	{CF_DRAIN_MANA, "Drains mana"},
	{CF_DRAIN_STAT, "Drains random stats"},
	{CF_DRAIN_CHARGE, "Drains magic devices"},
};

/**
 * An "object analysis structure"
 *
 * It will be filled with descriptive strings detailing an object's
 * various magical powers. The "ignore X" traits are not noted since
 * all artifacts ignore "normal" destruction.
 */

typedef struct {
	/* "The Longsword Dragonsmiter (6d4) (+20, +25)" */
	char description[160];

	/* A list of stat boosts granted by an object */
	const char *stats[N_ELEMENTS(stat_desc) + 1];

	/* A list of other bonuses granted by an object */
	const char *bonus[N_ELEMENTS(bonus_desc) + 1];

	/* A list of an object's slaying preferences */
	const char *slays[N_ELEMENTS(slay_desc) + 1];

	/* A list if an object's elemental brands */
	const char *brands[N_ELEMENTS(brand_desc) + 1];

	/* A list of resistances granted by an object */
	const char *resistances[N_ELEMENTS(resist_desc) + 1];

	/* A list of stats sustained by an object */
	const char *sustains[N_ELEMENTS(sustain_flags_desc) - 1 + 1];

	/* A list of various magical qualities an object may have */
	const char *powers[N_ELEMENTS(flags_obj_desc) + 1];

	/* A list of various curses an object may have */
	const char *curses[N_ELEMENTS(flags_curse_desc) + 1 + 1];

	/* A string describing an artifact's activation */
	const char *activation;

	/* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
	char misc_desc[80];

} obj_desc_list;



/**
 * Write out `n' of the character `c' to the spoiler file
 */
static void spoiler_out_n_chars(int n, char c)
{
	while (--n >= 0)
		fputc(c, (FILE *) fff);
}

/**
 * Write out `n' blank lines to the spoiler file
 */
static void spoiler_blanklines(int n)
{
	spoiler_out_n_chars(n, '\n');
}

/**
 * Write a line to the spoiler file and then "underline" it with hypens
 */
static void spoiler_underline(const char *str)
{
	file_putf(fff, "%s\n", str);
	spoiler_out_n_chars(strlen(str), '-');
	file_putf(fff, "\n");
}



/**
 * These functions do most of the actual "analysis". Given a set of bit flags
 * (which will be from one of the flags fields from the object in question),
 * or directly the integer fields of the object
 * a "flag description structure", a "description list", and the number of
 * elements in the "flag description structure", or directly the integer 
 * fields of the object, these functions set the "description list" members to 
 * the appropriate descriptions.
 *
 * The possibly updated description pointer is returned.
 */

static const char **spoiler_flag_aux(const bitflag * art_flags,
									 const flag_desc * flag_x_ptr,
									 const char **desc_x_ptr,
									 const int n_elmnts)
{
	int i;

	for (i = 0; i < n_elmnts; ++i) {
		if (of_has(art_flags, flag_x_ptr[i].flag)) {
			*desc_x_ptr++ = flag_x_ptr[i].desc;
		}
	}

	return desc_x_ptr;
}

static const char **spoiler_int_aux(const int *values, const int base,
									const char **desc,
									const char **desc_x_ptr,
									const int n_elmnts)
{
	int i;

	for (i = 0; i < n_elmnts; ++i) {
		if (values[i] != base) {
			int value = (base == 0 ? values[i] : base - values[i]);
			*desc_x_ptr++ = format("%s%d%s%s", POSITIZE(value), value,
								   (base == 0 ? " " : "% "), desc[i]);
		}
	}

	return desc_x_ptr;
}

static const char **spoiler_frac_aux(const int *values, const int base,
									 const char **desc,
									 const char **desc_x_ptr,
									 const int n_elmnts)
{
	int i;

	for (i = 0; i < n_elmnts; ++i) {
		if (values[i] != base) {
			*desc_x_ptr++ = format("x%d/%d %s", values[i], base, desc[i]);
		}
	}

	return desc_x_ptr;
}


/**
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 */
static void analyze_general(object_type * o_ptr, char *desc_x_ptr)
{
	/* Get a "useful" description of the object */
	object_desc(desc_x_ptr, sizeof(desc_x_ptr), o_ptr,
				ODESC_PREFIX | ODESC_COMBAT | ODESC_SPOIL);
}

/**
 * List stat bonuses.
 */
static void analyze_stats(object_type * o_ptr, const char **stat_list)
{
	/* Are any stats affected? */
	stat_list = spoiler_int_aux(o_ptr->bonus_stat, 0, stat_desc,
								stat_list, N_ELEMENTS(stat_desc));

	/* Terminate the description list */
	*stat_list = NULL;
}

/**
 * List other bonuses.
 */
static void analyze_bonus(object_type * o_ptr, const char **bonus_list)
{
	/* Are any stats affected? */
	bonus_list = spoiler_int_aux(o_ptr->bonus_other, 0, bonus_desc,
								 bonus_list, N_ELEMENTS(bonus_desc));

	/* Terminate the description list */
	*bonus_list = NULL;
}

/**
 * Note the slaying specialties of a weapon 
 */
static void analyze_slay(object_type * o_ptr, const char **slay_list)
{
	slay_list =
		spoiler_frac_aux(o_ptr->multiple_slay, MULTIPLE_BASE, slay_desc,
						 slay_list, N_ELEMENTS(slay_desc));

	/* Terminate the description list */
	*slay_list = NULL;
}

/**
 * Note an object's elemental brands 
 */
static void analyze_brand(object_type * o_ptr, const char **brand_list)
{
	brand_list = spoiler_frac_aux(o_ptr->multiple_brand, MULTIPLE_BASE,
								  brand_desc, brand_list,
								  N_ELEMENTS(brand_desc));

	/* Terminate the description list */
	*brand_list = NULL;
}


/**
 * Note the resistances granted by an object 
 */

static void analyze_resist(object_type * o_ptr, const char **resist_list)
{
	resist_list =
		spoiler_int_aux(o_ptr->percent_res, RES_LEVEL_BASE, resist_desc,
						resist_list, N_ELEMENTS(resist_desc));

	/* Terminate the description list */
	*resist_list = NULL;
}

/**
 * Note which stats an object sustains 
 */

static void analyze_sustains(object_type * o_ptr,
							 const char **sustain_list)
{
	bitflag all_sustains[OF_SIZE];

	of_wipe(all_sustains);
	of_on(all_sustains, OF_SUSTAIN_STR);
	of_on(all_sustains, OF_SUSTAIN_INT);
	of_on(all_sustains, OF_SUSTAIN_WIS);
	of_on(all_sustains, OF_SUSTAIN_DEX);
	of_on(all_sustains, OF_SUSTAIN_CON);
	of_on(all_sustains, OF_SUSTAIN_CHR);


	/* Simplify things if an item sustains all stats */
	if (of_is_subset(o_ptr->flags_obj, all_sustains)) {
		*sustain_list++ = "All stats";
	}

	/* Should we bother? */
	else if (of_is_inter(o_ptr->flags_obj, all_sustains)) {
		sustain_list =
			spoiler_flag_aux(o_ptr->flags_obj, sustain_flags_desc,
							 sustain_list, N_ELEMENTS(sustain_flags_desc));
	}

	/* Terminate the description list */
	*sustain_list = NULL;
}


/**
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 */

static void analyze_powers(object_type * o_ptr, const char **power_list)
{
	/* Hack - put perma curse in with curses */
	bitflag flags[OF_SIZE];

	of_copy(flags, o_ptr->flags_obj);
	of_off(flags, OF_PERMA_CURSE);

	/*
	 * Special flags
	 */
	power_list = spoiler_flag_aux(flags, flags_obj_desc, power_list,
								  N_ELEMENTS(flags_obj_desc));

	/*
	 * Artifact lights -- large radius light.
	 */
	if ((o_ptr->tval == TV_LIGHT) && artifact_p(o_ptr)) {
		*power_list++ = "Permanent Light(3)";
	}

	/* Terminate the description list */
	*power_list = NULL;
}

/**
 * Note artifact curses
 */

static void analyze_curses(object_type * o_ptr, const char **curse_list)
{
	/*
	 * Special flags
	 */
	curse_list = spoiler_flag_aux(o_ptr->flags_curse, flags_curse_desc,
								  curse_list,
								  N_ELEMENTS(flags_curse_desc));

	/*
	 * Artifact lights -- large radius light.
	 */
	if (of_has(o_ptr->flags_obj, OF_PERMA_CURSE)) {
		*curse_list++ = "Permanently cursed";
	}

	/* Terminate the description list */
	*curse_list = NULL;
}




/**
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 */

static void analyze_misc(object_type * o_ptr, char *misc_desc)
{
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	sprintf(misc_desc, "Level %u, Rarity %u, %d.%d lbs, %ld Gold",
			a_ptr->level, a_ptr->rarity, a_ptr->weight / 10,
			a_ptr->weight % 10, (long) a_ptr->cost);
}

/**
 * Fill in an object description structure for a given object
 */

static void object_analyze(object_type * o_ptr, obj_desc_list * desc_x_ptr)
{
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	/* Oangband requires that activations be transferred to the object. */
	if (a_ptr->effect) {
		o_ptr->effect = a_ptr->effect;
	}

	analyze_general(o_ptr, desc_x_ptr->description);

	analyze_stats(o_ptr, desc_x_ptr->stats);

	analyze_bonus(o_ptr, desc_x_ptr->bonus);

	analyze_brand(o_ptr, desc_x_ptr->brands);

	analyze_slay(o_ptr, desc_x_ptr->slays);

	analyze_resist(o_ptr, desc_x_ptr->resistances);

	analyze_sustains(o_ptr, desc_x_ptr->sustains);

	analyze_powers(o_ptr, desc_x_ptr->powers);

	analyze_curses(o_ptr, desc_x_ptr->curses);

	analyze_misc(o_ptr, desc_x_ptr->misc_desc);
}


static void print_header(void)
{
	char buf[80];

	sprintf(buf, "Artifact Spoilers for FAangband Version %d.%d.%d",
			VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	spoiler_underline(buf);
}

/*
 * This is somewhat ugly.
 *
 * Given a header ("Resist", e.g.), a list ("Fire", "Cold", Acid", e.g.),
 * and a separator character (',', e.g.), write the list to the spoiler file
 * in a "nice" format, such as:
 *
 *	Resist Fire, Cold, Acid
 *
 * That was a simple example, but when the list is long, a line wrap
 * should occur, and this should induce a new level of indention if
 * a list is being spread across lines. So for example, Bladeturner's
 * list of resistances should look something like this
 *
 *     Resist Acid, Lightning, Fire, Cold, Poison, Light, Dark, Blindness,
 *	 Confusion, Sound, Shards, Nether, Nexus, Chaos, Disenchantment
 *
 * However, the code distinguishes between a single list of many items vs.
 * many lists. (The separator is used to make this determination.) A single
 * list of many items will not cause line wrapping (since there is no
 * apparent reason to do so). So the lists of Ulmo's miscellaneous traits
 * might look like this:
 *
 *     Free Action; Hold Life; See Invisible; Slow Digestion; Regeneration
 *     Blessed Blade
 *
 * So comparing the two, "Regeneration" has no trailing separator and
 * "Blessed Blade" was not indented. (Also, Ulmo's lists have no headers,
 * but that's not relevant to line wrapping and indention.)
 */

/**
 * ITEM_SEP separates items within a list 
 */
#define ITEM_SEP ','


/**
 * LIST_SEP separates lists 
 */
#define LIST_SEP ';'


static void spoiler_outlist(const char *header, const char **list,
							char separator)
{
	int line_len, buf_len;
	char line[MAX_LINE_LEN + 1], buf[80];

	/* Ignore an empty list */
	if (*list == NULL)
		return;

	/* This function always indents */
	strcpy(line, INDENT1);

	/* Create header (if one was given) */
	if (header && (header[0])) {
		strcat(line, header);
		strcat(line, " ");
	}

	line_len = strlen(line);

	/* Now begin the tedious task */
	while (1) {
		/* Copy the current item to a buffer */
		strcpy(buf, *list);

		/* Note the buffer's length */
		buf_len = strlen(buf);

		/*
		 * If there is an item following this one, pad with separator and
		 * a space and adjust the buffer length
		 */

		if (list[1]) {
			sprintf(buf + buf_len, "%c ", separator);
			buf_len += 2;
		}

		/*
		 * If the buffer will fit on the current line, just append the
		 * buffer to the line and adjust the line length accordingly.
		 */

		if (line_len + buf_len <= MAX_LINE_LEN) {
			strcat(line, buf);
			line_len += buf_len;
		}

		/* Apply line wrapping and indention semantics described above */
		else {
			/*
			 * Don't print a trailing list separator but do print a trailing
			 * item separator.
			 */
			if (line_len > 1 && line[line_len - 1] == ' '
				&& line[line_len - 2] == LIST_SEP) {
				/* Ignore space and separator */
				line[line_len - 2] = '\0';

				/* Write to spoiler file */
				file_putf(fff, "%s\n", line);

				/* Begin new line at primary indention level */
				sprintf(line, "%s%s", INDENT1, buf);
			}

			else {
				/* Write to spoiler file */
				file_putf(fff, "%s\n", line);

				/* Begin new line at secondary indention level */
				sprintf(line, "%s%s", INDENT2, buf);
			}

			line_len = strlen(line);
		}

		/* Advance, with break */
		if (!*++list)
			break;
	}

	/* Write what's left to the spoiler file */
	file_putf(fff, "%s\n", line);
}


/**
 * Create a spoiler file entry for an artifact 
 */

static void spoiler_print_art(obj_desc_list * art_ptr)
{
	/* Don't indent the first line */
	file_putf(fff, "%s\n", art_ptr->description);

	/* Now deal with the description lists */

	spoiler_outlist("", art_ptr->stats, ITEM_SEP);

	spoiler_outlist("", art_ptr->bonus, ITEM_SEP);

	spoiler_outlist("Slay", art_ptr->slays, ITEM_SEP);

	spoiler_outlist("", art_ptr->brands, LIST_SEP);

	spoiler_outlist("Resist", art_ptr->resistances, ITEM_SEP);

	spoiler_outlist("Sustain", art_ptr->sustains, ITEM_SEP);

	spoiler_outlist("", art_ptr->powers, LIST_SEP);

	spoiler_outlist("", art_ptr->curses, LIST_SEP);


	/* Write out the possible activation at the primary indention level */
	if (art_ptr->activation) {
		file_putf(fff, "%sActivates for %s\n", INDENT1,
				  art_ptr->activation);
	}

	/* End with the miscellaneous facts */
	file_putf(fff, "%s%s\n\n", INDENT1, art_ptr->misc_desc);
}


/**
 * Hack -- Create a "forged" artifact
 */
bool make_fake_artifact(object_type * o_ptr, int name1)
{
	int i;

	artifact_type *a_ptr = &a_info[name1];


	/* Ignore "empty" artifacts */
	if (!a_ptr->name)
		return FALSE;

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
	if (!i)
		return (FALSE);

	/* Create the artifact */
	object_prep(o_ptr, i, MAXIMISE);

	/* Save the name */
	o_ptr->name1 = name1;

	/* Extract the fields */
	o_ptr->pval = a_ptr->pval;
	o_ptr->ac = a_ptr->ac;
	o_ptr->dd = a_ptr->dd;
	o_ptr->ds = a_ptr->ds;
	o_ptr->to_a = a_ptr->to_a;
	o_ptr->to_h = a_ptr->to_h;
	o_ptr->to_d = a_ptr->to_d;
	o_ptr->weight = a_ptr->weight;

	for (i = 0; i < MAX_P_RES; i++)
		o_ptr->percent_res[i] = a_ptr->percent_res[i];
	for (i = 0; i < A_MAX; i++)
		o_ptr->bonus_stat[i] = a_ptr->bonus_stat[i];
	for (i = 0; i < MAX_P_BONUS; i++)
		o_ptr->bonus_other[i] = a_ptr->bonus_other[i];
	for (i = 0; i < MAX_P_SLAY; i++)
		o_ptr->multiple_slay[i] = a_ptr->multiple_slay[i];
	for (i = 0; i < MAX_P_BRAND; i++)
		o_ptr->multiple_brand[i] = a_ptr->multiple_brand[i];

	of_copy(o_ptr->flags_obj, a_ptr->flags_obj);
	cf_copy(o_ptr->flags_curse, a_ptr->flags_curse);

	/* Transfer the activation information. */
	if (a_ptr->effect) {
		o_ptr->effect = a_ptr->effect;
	}


	/* Success */
	return (TRUE);
}





/**
 * Show what object kinds appear on the current level
 */
static void spoil_obj_gen(const char *fname)
{
	int i;

	/* Storage */
	u32b artifacts = 0L;
	u32b egoitems = 0L;
	u32b *object = malloc(z_info->k_max * sizeof(*object));
	u32b depth[MAX_DEPTH];

	object_type *i_ptr;
	object_type object_type_body;
	char o_name[120];

	char buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Open the file */
	fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fff) {
		msg("Cannot create spoiler file.");
		free(object);
		return;
	}



	msg("This may take a while...");

	file_putf(fff,
			  "Object Generation Spoiler for FAangband Version %d.%d.%d\n",
			  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	/* Clear storage. */
	for (i = 0; i < z_info->k_max; i++) {
		object[i] = 0L;
	}

	/* Clear storage. */
	for (i = 0; i < MAX_DEPTH; i++) {
		depth[i] = 0L;
	}

	/* Make a lot of objects, and print their names out. */
	for (i = 0L; i < 1000000L; i++) {
		if (i % 10000 == 0) {
			prt(format("%ld objects created", (long) i), 0, 0);
		}


		/* Get local object */
		i_ptr = &object_type_body;

		/* Wipe the object */
		object_wipe(i_ptr);

		/* Create an object - no special conditions */
		make_object(i_ptr, FALSE, FALSE, FALSE);

		/* Count artifacts. */
		if (i_ptr->name1)
			artifacts += 1L;

		/* Count ego-items. */
		if (i_ptr->name2)
			egoitems += 1L;

		/* Count object kinds. */
		object[i_ptr->k_idx] += 1L;

		/* Count objects of that level. */
		depth[k_info[i_ptr->k_idx].level] += 1L;

		/* Mega-Hack -- allow multiple artifacts XXX XXX XXX */
		if (artifact_p(i_ptr))
			a_info[i_ptr->name1].created = FALSE;
	}

	/* Print to file. */
	file_putf(fff, "\n\n\n");
	file_putf(fff, "artifacts:  %ld\n", (long) artifacts);
	file_putf(fff, "ego-items:  %ld\n", (long) egoitems);
	file_putf(fff, "\n\n");
	file_putf(fff, "Number of objects created (1,000,000 total)\n");
	file_putf(fff, "         Generation Level:  %d\n\n", p_ptr->depth);

	for (i = 1; i < z_info->k_max; i++) {
		if (object[i]) {
			object_kind *k_ptr = &k_info[i];
			char *t;
			const char *str = k_ptr->name;

			if (strlen(str) == 0)
				continue;

			/* Skip past leading characters */
			while ((*str == ' ') || (*str == '&'))
				str++;

			/* Copy useful chars */
			for (t = o_name; *str; str++) {
				if (*str != '~')
					*t++ = *str;
			}

			/* Terminate the new name */
			*t = '\0';

			file_putf(fff, "%-40s:%6ld\n", o_name, (long) object[i]);
		}
	}

	file_putf(fff, "\n\n\n");
	file_putf(fff, "Object distribution by depth\n\n");

	for (i = 0; i < MAX_DEPTH; i++) {
		if (depth[i])
			file_putf(fff, "Level %3d:%6ld\n", i, (long) depth[i]);
	}

	free(object);


	/* Check for errors */
	if (!file_close(fff)) {
		msg("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg("Successfully created a spoiler file.");
}





/**
 * Show what monster races appear on the current level
 */
static void spoil_mon_gen(const char *fname)
{
	int i, num;
	struct keypress key;

	/* Storage */
	u32b *monster = malloc(z_info->r_max * sizeof(*monster));
	u32b depth[MAX_DEPTH];

	bool quick;


	char buf[1024];

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Open the file */
	fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fff) {
		msg("Cannot create spoiler file.");
		free(monster);
		return;
	}

	file_putf(fff,
			  "Monster Generation Spoiler for FAangband Version %d.%d.%d\n",
			  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	/* Clear storage. */
	for (i = 0; i < z_info->r_max; i++) {
		monster[i] = 0L;
	}

	/* Clear storage. */
	for (i = 0; i < MAX_DEPTH; i++) {
		depth[i] = 0L;
	}

	/* Clear screen */
	Term_clear();

	/* Info */
	prt("Use the quick monster generator?.", 2, 5);

	/* Prompt for a monster selection method */
	prt("Monster generation uses a three-level table:  The base probability, ", 5, 5);
	prt("gotten from r_info.txt, the modified probability as adjusted by , ", 6, 5);
	prt("special monster restrictions (like only dragons), and the final , ", 7, 5);
	prt("probability, which depends on values in the first two levels plus , ", 8, 5);
	prt("the current generation depth.  , ", 9, 5);

	prt("The quick monster generator builds the third probability level only ,", 11, 5);
	prt("once, and then draws monsters from it.  The standard method is to , ", 12, 5);
	prt("rebuild the third probability level for each monster", 13, 5);


	/* Prompt */
	prt("Command: ", 15, 0);

	/* Get a choice */
	key = inkey();
	i = key.code;

	if (i == 'y')
		quick = TRUE;
	else
		quick = FALSE;


	msg("This may take a while...");

	/* Initialize monster generation. */
	if (quick)
		(void) get_mon_num(p_ptr->depth);

	/* Make a lot of monsters, and print their names out. */
	for (i = 0L; i < 1000000L; i++) {
		if (i % 10000 == 0) {
			prt(format("%ld monsters created", (long) i), 0, 0);
		}


		/* Get a monster index */
		if (quick)
			num = get_mon_num_quick(p_ptr->depth);
		else
			num = get_mon_num(p_ptr->depth);

		/* Count monster races. */
		monster[num] += 1L;

		/* Count monsters of that level. */
		depth[r_info[num].level] += 1L;
	}

	/* Print to file. */
	file_putf(fff, "\n\n\n");
	file_putf(fff,
			  "Number of monsters of various kinds (1,000,000 total)\n");
	file_putf(fff, "         Generation Level:  %d\n\n", p_ptr->depth);

	for (i = 1; i < z_info->r_max; i++) {
		monster_race *r_ptr = &r_info[i];

		const char *name = r_ptr->name;

		if (monster[i]) {
			file_putf(fff, "%-45s:%6ld\n", name, (long) monster[i]);
		}
	}

	file_putf(fff, "\n\n\n");
	file_putf(fff, "Monster distribution by depth\n\n");

	for (i = 0; i < MAX_DEPTH; i++) {
		if (depth[i])
			file_putf(fff, "Level %3d:%6ld\n", i, (long) depth[i]);
	}

	free(monster);

	/* Check for errors */
	if (!file_close(fff)) {
		msg("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg("Successfully created a spoiler file.");
}






/**
 * Create a spoiler file for artifacts
 */
static void spoil_artifact(const char *fname)
{
	int i, j;

	object_type *i_ptr;
	object_type object_type_body;

	obj_desc_list artifact;

	char buf[1024];


	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Open the file */
	fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fff) {
		msg("Cannot create spoiler file.");
		return;
	}

	/* Dump the header */
	print_header();

	/* List the artifacts by tval */
	for (i = 0; group_artifact[i].tval; i++) {
		/* Write out the group title */
		if (group_artifact[i].name) {
			spoiler_blanklines(2);
			spoiler_underline(group_artifact[i].name);
			spoiler_blanklines(1);
		}

		/* Now search through all of the artifacts */
		for (j = 1; j < z_info->a_max; ++j) {
			artifact_type *a_ptr = &a_info[j];

			/* We only want objects in the current group */
			if (a_ptr->tval != group_artifact[i].tval)
				continue;

			/* Get local object */
			i_ptr = &object_type_body;

			/* Wipe the object */
			object_wipe(i_ptr);

			/* Attempt to "forge" the artifact */
			if (!make_fake_artifact(i_ptr, j))
				continue;

			/* Analyze the artifact */
			object_analyze(i_ptr, &artifact);

			/* Write out the artifact description to the spoiler file */
			spoiler_print_art(&artifact);
		}
	}

	/* Check for errors */
	if (!file_close(fff)) {
		msg("Cannot close spoiler file.");
		return;
	}

	/* Message */
	msg("Successfully created a spoiler file.");
}





/**
 * Create a spoiler file for monsters
 */
static void spoil_mon_desc(const char *fname)
{
	int i, n = 0;

	char buf[1024];

	char nam[80];
	char lev[80];
	char rar[80];
	char spd[80];
	char ac[80];
	char hp[80];
	char exp[80];

	u16b *who;

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Open the file */
	fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fff) {
		msg("Cannot create spoiler file.");
		return;
	}

	/* Dump the header */
	file_putf(fff, "Monster Spoilers for FAangband Version %d.%d.%d\n",
			  VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	file_putf(fff, "------------------------------------------\n\n");

	/* Dump the header */
	file_putf(fff, "%-40.40s%4s%4s%6s%8s%4s  %11.11s\n",
			  "Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
	file_putf(fff, "%-40.40s%4s%4s%6s%8s%4s  %11.11s\n",
			  "----", "---", "---", "---", "--", "--", "-----------");


	/* Allocate the "who" array */
	who = C_ZNEW(z_info->r_max, u16b);

	/* Scan the monsters */
	for (i = 1; i < z_info->r_max; i++) {
		monster_race *r_ptr = &r_info[i];

		/* Use that monster */
		if (r_ptr->name)
			who[n++] = i;
	}

	/* Sort the array by dungeon depth of monsters */
	sort(who, n, sizeof(*who), cmp_monsters);

	/* Scan again */
	for (i = 0; i < n; i++) {
		monster_race *r_ptr = &r_info[who[i]];

		const char *name = r_ptr->name;

		/* Get the "name" */
		if (rf_has(r_ptr->flags, RF_QUESTOR)) {
			sprintf(nam, "[Q] %s", name);
		} else if (rf_has(r_ptr->flags, RF_UNIQUE)) {
			sprintf(nam, "[U] %s", name);
		} else {
			sprintf(nam, "The %s", name);
		}


		/* Level */
		sprintf(lev, "%d", r_ptr->level);

		/* Rarity */
		sprintf(rar, "%d", r_ptr->rarity);

		/* Speed */
		if (r_ptr->speed >= 110) {
			sprintf(spd, "+%d", (r_ptr->speed - 110));
		} else {
			sprintf(spd, "-%d", (110 - r_ptr->speed));
		}

		/* Armor Class */
		sprintf(ac, "%d", r_ptr->ac);

		/* Hitpoints */
		if ((rf_has(r_ptr->flags, RF_FORCE_MAXHP)) || (r_ptr->hside == 1)) {
			sprintf(hp, "%d", r_ptr->hdice * r_ptr->hside);
		} else {
			sprintf(hp, "%dd%d", r_ptr->hdice, r_ptr->hside);
		}


		/* Experience */
		sprintf(exp, "%ld", (long) (r_ptr->mexp));

		/* Hack -- use visual instead */
		sprintf(exp, "%s '%c'", attr_to_text(r_ptr->d_attr),
				r_ptr->d_char);

		/* Dump the info */
		file_putf(fff, "%-40.40s%4s%4s%6s%8s%4s  %11.11s\n",
				  nam, lev, rar, spd, hp, ac, exp);
	}

	/* End it */
	file_putf(fff, "\n");

	/* Free the "who" array */
	FREE(who);

	/* Check for errors */
	if (!file_close(fff)) {
		msg("Cannot close spoiler file.");
		return;
	}

	/* Worked */
	msg("Successfully created a spoiler file.");
}




/*
 * Monster spoilers by: smchorse@ringer.cs.utsa.edu (Shawn McHorse)
 *
 * Adapted from the "monster_desc()" code in "monster1.c"
 */

/**
 * Pronoun array
 */
static const char *wd_che[3] = { "It", "He", "She" };

/**
 * Pronoun array
 */
static const char *wd_lhe[3] = { "it", "he", "she" };

/**
 * Buffer text to the given file. (-SHAWN-)
 * This is basically c_roff() from util.c with a few changes.
 */
static void spoil_out(const char *str)
{
	const char *r;

	/* Line buffer */
	static char roff_buf[256];

	/* Current pointer into line roff_buf */
	static char *roff_p = roff_buf;

	/* Last space saved into roff_buf */
	static char *roff_s = NULL;

	/* Special handling for "new sequence" */
	if (!str) {
		if (roff_p != roff_buf)
			roff_p--;
		while (*roff_p == ' ' && roff_p != roff_buf)
			roff_p--;
		if (roff_p == roff_buf)
			file_putf(fff, "\n");
		else {
			*(roff_p + 1) = '\0';
			file_putf(fff, "%s\n\n", roff_buf);
		}
		roff_p = roff_buf;
		roff_s = NULL;
		roff_buf[0] = '\0';
		return;
	}

	/* Scan the given string, character at a time */
	for (; *str; str++) {
		char ch = *str;
		int wrap = (ch == '\n');

		if (!isprint(ch))
			ch = ' ';
		if (roff_p >= roff_buf + 75)
			wrap = 1;
		if ((ch == ' ') && (roff_p + 2 >= roff_buf + 75))
			wrap = 1;

		/* Handle line-wrap */
		if (wrap) {
			*roff_p = '\0';
			r = roff_p;
			if (roff_s && (ch != ' ')) {
				*roff_s = '\0';
				r = roff_s + 1;
			}
			file_putf(fff, "%s\n", roff_buf);
			roff_s = NULL;
			roff_p = roff_buf;
			while (*r)
				*roff_p++ = *r++;
		}

		/* Save the char */
		if ((roff_p > roff_buf) || (ch != ' ')) {
			if (ch == ' ')
				roff_s = roff_p;
			*roff_p++ = ch;
		}
	}
}


/**
 * Create a spoiler file for monsters (-SHAWN-)
 */
static void spoil_mon_info(const char *fname)
{
	char buf[1024];
	int msex, vn, i, j, k, n;
	bool breath, magic, sin;
	const char *p, *q;
	const char *vp[64];

	int spower;

	const char *name;

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_USER, fname);

	/* Open the file */
	fff = file_open(buf, MODE_WRITE, FTYPE_TEXT);

	/* Oops */
	if (!fff) {
		msg("Cannot create spoiler file.");
		return;
	}


	/* Dump the header */
	sprintf(buf, "Monster Spoilers for FAangband Version %d.%d.%d\n",
			VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	spoil_out(buf);
	spoil_out("------------------------------------------\n\n");

	/*
	 * List all monsters in order.
	 */
	for (n = 1; n < z_info->r_max; n++) {
		monster_race *r_ptr = &r_info[n];

		/* Extract the flags */
		breath = FALSE;
		magic = FALSE;
		spower = r_ptr->spell_power;

		/* Get the monster name */
		name = r_ptr->name;

		/* Extract a gender (if applicable) */
		if (rf_has(r_ptr->flags, RF_FEMALE))
			msex = 2;
		else if (rf_has(r_ptr->flags, RF_MALE))
			msex = 1;
		else
			msex = 0;


		/* Prefix */
		if (rf_has(r_ptr->flags, RF_QUESTOR)) {
			spoil_out("[Q] ");
		} else if (rf_has(r_ptr->flags, RF_UNIQUE)) {
			spoil_out("[U] ");
		} else {
			spoil_out("The ");
		}

		/* Name */
		sprintf(buf, "%s  (", r_ptr->name);	/* ---)--- */
		spoil_out(buf);

		/* Color */
		spoil_out(attr_to_text(r_ptr->d_attr));

		/* Symbol --(-- */
		sprintf(buf, " '%c')\n", r_ptr->d_char);
		spoil_out(buf);


		/* Indent */
		sprintf(buf, "=== ");
		spoil_out(buf);

		/* Number */
		sprintf(buf, "Num:%d  ", n);
		spoil_out(buf);

		/* Level */
		sprintf(buf, "Lev:%d  ", r_ptr->level);
		spoil_out(buf);

		/* Rarity */
		sprintf(buf, "Rar:%d  ", r_ptr->rarity);
		spoil_out(buf);

		/* Speed */
		if (r_ptr->speed >= 110) {
			sprintf(buf, "Spd:+%d  ", (r_ptr->speed - 110));
		} else {
			sprintf(buf, "Spd:-%d  ", (110 - r_ptr->speed));
		}
		spoil_out(buf);

		/* Hitpoints */
		if ((rf_has(r_ptr->flags, RF_FORCE_MAXHP)) || (r_ptr->hside == 1)) {
			sprintf(buf, "Hp:%d  ", r_ptr->hdice * r_ptr->hside);
		} else {
			sprintf(buf, "Hp:%dd%d  ", r_ptr->hdice, r_ptr->hside);
		}
		spoil_out(buf);

		/* Armor Class */
		sprintf(buf, "Ac:%d  ", r_ptr->ac);
		spoil_out(buf);

		/* Experience */
		sprintf(buf, "Exp:%ld\n", (long) (r_ptr->mexp));
		spoil_out(buf);


		/* Describe */
		spoil_out(r_ptr->text);
		spoil_out("  ");


		spoil_out("This");

		if (rf_has(r_ptr->flags, RF_ANIMAL))
			spoil_out(" natural");
		if (rf_has(r_ptr->flags, RF_EVIL))
			spoil_out(" evil");
		if (rf_has(r_ptr->flags, RF_UNDEAD))
			spoil_out(" undead");

		if (rf_has(r_ptr->flags, RF_DRAGON))
			spoil_out(" dragon");
		else if (rf_has(r_ptr->flags, RF_DEMON))
			spoil_out(" demon");
		else if (rf_has(r_ptr->flags, RF_GIANT))
			spoil_out(" giant");
		else if (rf_has(r_ptr->flags, RF_TROLL))
			spoil_out(" troll");
		else if (rf_has(r_ptr->flags, RF_ORC))
			spoil_out(" orc");
		else
			spoil_out(" creature");

		spoil_out(" moves");

		if ((rf_has(r_ptr->flags, RF_RAND_50))
			&& (rf_has(r_ptr->flags, RF_RAND_25))) {
			spoil_out(" extremely erratically");
		} else if (rf_has(r_ptr->flags, RF_RAND_50)) {
			spoil_out(" somewhat erratically");
		} else if (rf_has(r_ptr->flags, RF_RAND_25)) {
			spoil_out(" a bit erratically");
		} else {
			spoil_out(" normally");
		}

		if (rf_has(r_ptr->flags, RF_NEVER_MOVE)) {
			spoil_out(", but does not deign to chase intruders");
		}

		spoil_out(".  ");


		if (rf_has(r_ptr->flags, RF_ESCORT)) {
			sprintf(buf, "%s usually appears with ", wd_che[msex]);
			spoil_out(buf);
			if (rf_has(r_ptr->flags, RF_ESCORTS))
				spoil_out("escorts.  ");
			else
				spoil_out("an escort.  ");
		}

		if ((rf_has(r_ptr->flags, RF_FRIEND))
			|| (rf_has(r_ptr->flags, RF_FRIENDS))) {
			sprintf(buf, "%s usually appears in groups.  ", wd_che[msex]);
			spoil_out(buf);
		}


		/* Collect inate attacks */
		vn = 0;
		if (rsf_has(r_ptr->flags, RSF_SHRIEK))
			vp[vn++] = "shriek for help";
		if (rsf_has(r_ptr->flags, RSF_LASH))
			vp[vn++] = "lash you if nearby";
		if (rsf_has(r_ptr->flags, RSF_BOULDER)) {
			if (spower < 10)
				vp[vn++] = "throw rocks";
			else
				vp[vn++] = "throw boulders";
		}
		if (rsf_has(r_ptr->flags, RSF_SHOT)) {
			if (spower < 5)
				vp[vn++] = "sling pebbles";
			else if (spower < 15)
				vp[vn++] = "sling leaden pellets";
			else
				vp[vn++] = "sling seeker shot";
		}
		if (rsf_has(r_ptr->flags, RSF_ARROW)) {
			if (spower < 8)
				vp[vn++] = "shoot little arrows";
			else if (spower < 15)
				vp[vn++] = "shoot arrows";
			else
				vp[vn++] = "shoot seeker arrows";
		}
		if (rsf_has(r_ptr->flags, RSF_BOLT)) {
			if (spower < 8)
				vp[vn++] = "fire bolts";
			else if (spower < 15)
				vp[vn++] = "fire crossbow quarrels";
			else
				vp[vn++] = "fire seeker bolts";
		}
		if (rsf_has(r_ptr->flags, RSF_MISSL)) {
			if (spower < 8)
				vp[vn++] = "fire little missiles";
			else if (spower < 15)
				vp[vn++] = "fire missiles";
			else
				vp[vn++] = "fire heavy missiles";
		}
		if (rsf_has(r_ptr->flags, RSF_PMISSL)) {
			if (rf_has(r_ptr->flags, RF_MORGUL_MAGIC))
				vp[vn++] = "hurl black darts";
			else
				vp[vn++] = "whip poisoned darts";
		}

		if (vn) {
			spoil_out(wd_che[msex]);

			for (i = 0; i < vn; i++) {
				if (!i) {
					spoil_out(" may ");
					if (rf_has(r_ptr->flags, RF_ARCHER))
						spoil_out("frequently ");
				} else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(", or ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect breaths */
		vn = 0;
		if (rsf_has(r_ptr->flags, RSF_BRTH_ACID))
			vp[vn++] = "acid";
		if (rsf_has(r_ptr->flags, RSF_BRTH_ELEC))
			vp[vn++] = "lightning";
		if (rsf_has(r_ptr->flags, RSF_BRTH_FIRE))
			vp[vn++] = "fire";
		if (rsf_has(r_ptr->flags, RSF_BRTH_COLD))
			vp[vn++] = "frost";
		if (rsf_has(r_ptr->flags, RSF_BRTH_POIS))
			vp[vn++] = "poison";
		if (rsf_has(r_ptr->flags, RSF_BRTH_PLAS))
			vp[vn++] = "plasma";
		if (rsf_has(r_ptr->flags, RSF_BRTH_LIGHT))
			vp[vn++] = "light";
		if (rsf_has(r_ptr->flags, RSF_BRTH_DARK)) {
			if (rf_has(r_ptr->flags, RF_MORGUL_MAGIC))
				vp[vn++] = "Night";
			else
				vp[vn++] = "darkness";
		}
		if (rsf_has(r_ptr->flags, RSF_BRTH_CONFU))
			vp[vn++] = "confusion";
		if (rsf_has(r_ptr->flags, RSF_BRTH_SOUND))
			vp[vn++] = "sound";
		if (rsf_has(r_ptr->flags, RSF_BRTH_SHARD))
			vp[vn++] = "shards";
		if (rsf_has(r_ptr->flags, RSF_BRTH_INER))
			vp[vn++] = "inertia";
		if (rsf_has(r_ptr->flags, RSF_BRTH_GRAV))
			vp[vn++] = "gravity";
		if (rsf_has(r_ptr->flags, RSF_BRTH_FORCE))
			vp[vn++] = "force";

		if (rsf_has(r_ptr->flags, RSF_BRTH_NEXUS))
			vp[vn++] = "nexus";
		if (rsf_has(r_ptr->flags, RSF_BRTH_NETHR))
			vp[vn++] = "nether";
		if (rsf_has(r_ptr->flags, RSF_BRTH_CHAOS))
			vp[vn++] = "chaos";
		if (rsf_has(r_ptr->flags, RSF_BRTH_DISEN))
			vp[vn++] = "disenchantment";
		if (rsf_has(r_ptr->flags, RSF_BRTH_TIME))
			vp[vn++] = "time";

		if (rsf_has(r_ptr->flags, RSF_BRTH_STORM))
			vp[vn++] = "storm";
		if (rsf_has(r_ptr->flags, RSF_BRTH_DFIRE))
			vp[vn++] = "dragonfire";
		if (rsf_has(r_ptr->flags, RSF_BRTH_ICE))
			vp[vn++] = "ice";
		if (rsf_has(r_ptr->flags, RSF_BRTH_ALL))
			vp[vn++] = "the elements";
		if (rsf_has(r_ptr->flags, RSF_XXX1))
			vp[vn++] = "something";

		if (vn) {
			breath = TRUE;
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" may breathe ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" or ");
				spoil_out(vp[i]);
			}
			if (rf_has(r_ptr->flags, RF_POWERFUL))
				spoil_out(" powerfully");
		}

		/* Collect spells */
		vn = 0;

		if (rsf_has(r_ptr->flags, RSF_BALL_ACID)) {
			if (spower < 70)
				vp[vn++] = "produce acid balls";
			else
				vp[vn++] = "produce acid storms";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_ELEC)) {
			if (spower < 70)
				vp[vn++] = "produce lightning balls";
			else
				vp[vn++] = "produce lightning storms";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_FIRE)) {
			if (rf_has(r_ptr->flags, RF_UDUN_MAGIC)) {
				if (spower < 70)
					vp[vn++] = "produce balls of hellfire";
				else if (spower < 110)
					vp[vn++] = "invoke storms of Udun-fire";
				else
					vp[vn++] = "call upon the fires of Udun";
			} else {
				if (spower < 70)
					vp[vn++] = "produce fire balls";
				else
					vp[vn++] = "produce fire storms";
			}
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_COLD)) {
			if (rf_has(r_ptr->flags, RF_MORGUL_MAGIC)) {
				if (spower < 70)
					vp[vn++] = "produce spheres of deadly cold";
				else
					vp[vn++] = "invoke storms of deadly cold";
			} else {
				if (spower < 70)
					vp[vn++] = "produce frost balls";
				else
					vp[vn++] = "produce frost storms";
			}
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_POIS)) {
			if (rf_has(r_ptr->flags, RF_MORGUL_MAGIC)) {
				if (spower < 15)
					vp[vn++] = "produce clouds of venom";
				else if (spower < 70)
					vp[vn++] = "produce venomous balls";
				else
					vp[vn++] = "raise storms of venom";
			} else {
				if (spower < 15)
					vp[vn++] = "produce stinking clouds";
				else if (spower < 70)
					vp[vn++] = "produce poison balls";
				else
					vp[vn++] = "produce storms of poison";
			}
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_LIGHT)) {
			if (spower < 15)
				vp[vn++] = "produce spheres of light";
			else if (spower < 70)
				vp[vn++] = "produce explosions of light";
			else
				vp[vn++] = "invoke starbursts";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_DARK)) {
			if (rf_has(r_ptr->flags, RF_MORGUL_MAGIC)) {
				if (spower < 70)
					vp[vn++] = "produce spheres of Night";
				else
					vp[vn++] = "conjure up storms of Night";
			} else {
				if (spower < 70)
					vp[vn++] = "produce balls of darkness";
				else
					vp[vn++] = "produce storms of darkness";
			}
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_CONFU)) {
			if (spower < 70)
				vp[vn++] = "produce confusion balls";
			else
				vp[vn++] = "produce storms of confusion";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_SOUND)) {
			if (spower < 15)
				vp[vn++] = "produce explosions of sound";
			else if (spower < 70)
				vp[vn++] = "produce thunderclaps";
			else
				vp[vn++] = "unleash storms of sound";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_SHARD)) {
			if (spower < 15)
				vp[vn++] = "produce blasts of shards";
			else if (spower < 90)
				vp[vn++] = "produce whirlwinds of shards";
			else
				vp[vn++] = "call up shardstorms";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_STORM)) {
			if (spower < 30)
				vp[vn++] = "produce little storms";
			else if (spower < 70)
				vp[vn++] = "produce whirlpools";
			else
				vp[vn++] = "call up raging storms";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_NETHR)) {
			if (spower < 30)
				vp[vn++] = "produce nether orbs";
			else if (spower < 70)
				vp[vn++] = "produce nether balls";
			else
				vp[vn++] = "invoke nether storms";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_CHAOS)) {
			if (spower < 20)
				vp[vn++] = "produce spheres of chaos";
			else if (spower < 70)
				vp[vn++] = "produce explosions of chaos";
			else
				vp[vn++] = "call up maelstroms of raw chaos";
		}
		if (rsf_has(r_ptr->flags, RSF_BALL_MANA)) {
			if (spower < 40)
				vp[vn++] = "produce manabursts";
			else if (spower < 90)
				vp[vn++] = "produce balls of mana";
			else
				vp[vn++] = "invoke mana storms";
		}
		if (rsf_has(r_ptr->flags, RSF_BOLT_ACID))
			vp[vn++] = "produce acid bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_ELEC))
			vp[vn++] = "produce lightning bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_FIRE))
			vp[vn++] = "produce fire bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_COLD))
			vp[vn++] = "produce frost bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_POIS))
			vp[vn++] = "produce poison bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_PLAS))
			vp[vn++] = "produce plasma bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_ICE))
			vp[vn++] = "produce ice bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_WATER))
			vp[vn++] = "produce water bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_NETHR))
			vp[vn++] = "produce nether bolts";
		if (rsf_has(r_ptr->flags, RSF_BOLT_DARK))
			vp[vn++] = "produce dark bolts";
		if (rsf_has(r_ptr->flags, RSF_BEAM_ELEC))
			vp[vn++] = "shoot sparks of lightning";
		if (rsf_has(r_ptr->flags, RSF_BEAM_ICE))
			vp[vn++] = "cast lances of ice";
		if (rsf_has(r_ptr->flags, RSF_BEAM_NETHR)) {
			if (spower < 40)
				vp[vn++] = "cast lances of nether";
			else if (spower < 90)
				vp[vn++] = "shoot rays of death";
		}
		if (rsf_has(r_ptr->flags, RSF_ARC_HFIRE)) {
			if (rf_has(r_ptr->flags, RF_UDUN_MAGIC)) {
				if (spower < 50)
					vp[vn++] = "produce a column of hellfire";
				else if (spower < 100)
					vp[vn++] = "envelop you in hellfire";
				else
					vp[vn++] = "breath like the Balrog";
			} else {
				if (spower < 50)
					vp[vn++] = "produce a column of fire";
				else if (spower < 100)
					vp[vn++] = "envelop you in fire";
			}
		}
		if (rsf_has(r_ptr->flags, RSF_ARC_FORCE)) {
			if (spower < 50)
				vp[vn++] = "thrust you away";
			else if (spower < 100)
				vp[vn++] = "hurls you away";
			else
				vp[vn++] = "snatches you up, and throws you away";
		}
		if (rsf_has(r_ptr->flags, RSF_HASTE))
			vp[vn++] = "haste-self";
		if (rsf_has(r_ptr->flags, RSF_ADD_MANA))
			vp[vn++] = "restore mana";
		if (rsf_has(r_ptr->flags, RSF_HEAL))
			vp[vn++] = "heal-self";
		if (rsf_has(r_ptr->flags, RSF_CURE))
			vp[vn++] = "cure what ails it";
		if (rsf_has(r_ptr->flags, RSF_BLINK))
			vp[vn++] = "blink-self";
		if (rsf_has(r_ptr->flags, RSF_TPORT))
			vp[vn++] = "teleport-self";
		if (rsf_has(r_ptr->flags, RSF_TELE_TO))
			vp[vn++] = "teleport to";
		if (rsf_has(r_ptr->flags, RSF_TELE_AWAY))
			vp[vn++] = "teleport away";
		if (rsf_has(r_ptr->flags, RSF_TELE_LEVEL))
			vp[vn++] = "teleport level";
		if (rsf_has(r_ptr->flags, RSF_DARKNESS))
			vp[vn++] = "create darkness";
		if (rsf_has(r_ptr->flags, RSF_TRAPS))
			vp[vn++] = "create traps";
		if (rsf_has(r_ptr->flags, RSF_FORGET))
			vp[vn++] = "cause amnesia";
		if (rsf_has(r_ptr->flags, RSF_DRAIN_MANA))
			vp[vn++] = "drain mana";
		if (rsf_has(r_ptr->flags, RSF_DISPEL))
			vp[vn++] = "dispel magic";
		if (rsf_has(r_ptr->flags, RSF_MIND_BLAST))
			vp[vn++] = "cause mind blasting";
		if (rsf_has(r_ptr->flags, RSF_BRAIN_SMASH))
			vp[vn++] = "cause brain smashing";
		if (rsf_has(r_ptr->flags, RSF_WOUND)) {
			if (spower < 7)
				vp[vn++] = "cause light wounds";
			else if (spower < 15)
				vp[vn++] = "cause medium wounds";
			else if (spower < 30)
				vp[vn++] = "cause serious wounds";
			else if (spower < 50)
				vp[vn++] = "cause critical wounds";
			else
				vp[vn++] = "cause mortal wounds";
		}
		if (rsf_has(r_ptr->flags, RSF_SHAPECHANGE))
			vp[vn++] = "change shape";
		if (rsf_has(r_ptr->flags, RSF_SCARE))
			vp[vn++] = "terrify";
		if (rsf_has(r_ptr->flags, RSF_BLIND))
			vp[vn++] = "blind";
		if (rsf_has(r_ptr->flags, RSF_CONF))
			vp[vn++] = "confuse";
		if (rsf_has(r_ptr->flags, RSF_SLOW))
			vp[vn++] = "slow";
		if (rsf_has(r_ptr->flags, RSF_HOLD))
			vp[vn++] = "paralyze";

		if (rsf_has(r_ptr->flags, RSF_S_MONSTER))
			vp[vn++] = "summon a monster";
		if (rsf_has(r_ptr->flags, RSF_S_MONSTERS))
			vp[vn++] = "summon monsters";
		if (rsf_has(r_ptr->flags, RSF_S_ANT))
			vp[vn++] = "summon ants";
		if (rsf_has(r_ptr->flags, RSF_S_SPIDER))
			vp[vn++] = "summon spiders";
		if (rsf_has(r_ptr->flags, RSF_S_HOUND))
			vp[vn++] = "summon hounds";
		if (rsf_has(r_ptr->flags, RSF_S_ANIMAL))
			vp[vn++] = "summon natural creatures";
		if (rsf_has(r_ptr->flags, RSF_S_THIEF))
			vp[vn++] = "summon thieves";
		if (rsf_has(r_ptr->flags, RSF_S_SWAMP))
			vp[vn++] = "summon swamp creatures";
		if (rsf_has(r_ptr->flags, RSF_S_DRAGON))
			vp[vn++] = "summon a dragon";
		if (rsf_has(r_ptr->flags, RSF_S_HI_DRAGON))
			vp[vn++] = "summon Ancient Dragons";
		if (rsf_has(r_ptr->flags, RSF_S_DEMON))
			vp[vn++] = "summon a demon";
		if (rsf_has(r_ptr->flags, RSF_S_HI_DEMON))
			vp[vn++] = "summon Greater Demons";
		if (rsf_has(r_ptr->flags, RSF_S_UNDEAD))
			vp[vn++] = "summon an undead";
		if (rsf_has(r_ptr->flags, RSF_S_HI_UNDEAD))
			vp[vn++] = "summon Greater Undead";
		if (rsf_has(r_ptr->flags, RSF_S_QUEST))
			vp[vn++] = "summon the Quest monsters";
		if (rsf_has(r_ptr->flags, RSF_S_UNIQUE))
			vp[vn++] = "summon Unique Monsters";

		if (vn) {
			magic = TRUE;
			if (breath) {
				spoil_out(", and is also");
			} else {
				spoil_out(wd_che[msex]);
				spoil_out(" is");
			}
			spoil_out(" magical, casting ");

			/* Describe magic */
			if ((rf_has(r_ptr->flags, RF_UDUN_MAGIC))
				&& (rf_has(r_ptr->flags, RF_MORGUL_MAGIC)))
				spoil_out(" perilous spells of Udun and of Morgul");
			if (rf_has(r_ptr->flags, RF_MORGUL_MAGIC))
				spoil_out(" Morgul-spells");
			else if (rf_has(r_ptr->flags, RF_UDUN_MAGIC))
				spoil_out(" spells of Udun");

			else
				spoil_out(" spells");

			if (rf_has(r_ptr->flags, RF_SMART))
				spoil_out(" intelligently");
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" which ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" or ");
				spoil_out(vp[i]);
			}
		}

		if (breath || magic) {
			if (r_ptr->freq_ranged) {
				sprintf(buf, "; 1 time in %d.  ",
						100 / (r_ptr->freq_ranged));
				spoil_out(buf);
			} else {
				sprintf(buf, "; never.  ");
				spoil_out(buf);
			}
		}

		/* Collect special abilities. */
		vn = 0;
		if (rf_has(r_ptr->flags, RF_OPEN_DOOR))
			vp[vn++] = "open doors";
		if (rf_has(r_ptr->flags, RF_BASH_DOOR))
			vp[vn++] = "bash down doors";
		if (rf_has(r_ptr->flags, RF_PASS_WALL))
			vp[vn++] = "pass through walls";
		if (rf_has(r_ptr->flags, RF_KILL_WALL))
			vp[vn++] = "bore through walls";
		if (rf_has(r_ptr->flags, RF_MOVE_BODY))
			vp[vn++] = "push past weaker monsters";
		if (rf_has(r_ptr->flags, RF_KILL_BODY))
			vp[vn++] = "destroy weaker monsters";
		if (rf_has(r_ptr->flags, RF_TAKE_ITEM))
			vp[vn++] = "pick up objects";
		if (rf_has(r_ptr->flags, RF_KILL_ITEM))
			vp[vn++] = "destroy objects";

		if (vn) {
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" can ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
			spoil_out(wd_che[msex]);
			spoil_out(" is a player ghost template.  ");
		}

		if (rf_has(r_ptr->flags, RF_INVISIBLE)) {
			spoil_out(wd_che[msex]);
			spoil_out(" is invisible.  ");
		}
		if (rf_has(r_ptr->flags, RF_COLD_BLOOD)) {
			spoil_out(wd_che[msex]);
			spoil_out(" is cold blooded.  ");
		}
		if (rf_has(r_ptr->flags, RF_EMPTY_MIND)) {
			spoil_out(wd_che[msex]);
			spoil_out(" is not detected by telepathy.  ");
		}
		if (rf_has(r_ptr->flags, RF_WEIRD_MIND)) {
			spoil_out(wd_che[msex]);
			spoil_out(" is rarely detected by telepathy.  ");
		}
		if (rf_has(r_ptr->flags, RF_MULTIPLY)) {
			spoil_out(wd_che[msex]);
			spoil_out(" breeds explosively.  ");
		}
		if (rf_has(r_ptr->flags, RF_REGENERATE)) {
			spoil_out(wd_che[msex]);
			spoil_out(" regenerates quickly.  ");
		}

		/* Collect susceptibilities */
		vn = 0;
		if (rf_has(r_ptr->flags, RF_HURT_ROCK))
			vp[vn++] = "rock remover";
		if (rf_has(r_ptr->flags, RF_HURT_LIGHT))
			vp[vn++] = "bright light";
		if (rf_has(r_ptr->flags, RF_HURT_FIRE))
			vp[vn++] = "fire";
		if (rf_has(r_ptr->flags, RF_HURT_COLD))
			vp[vn++] = "cold";

		if (vn) {
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" is hurt by ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect immunities */
		vn = 0;
		if (rf_has(r_ptr->flags, RF_IM_ACID))
			vp[vn++] = "acid";
		if (rf_has(r_ptr->flags, RF_IM_ELEC))
			vp[vn++] = "lightning";
		if (rf_has(r_ptr->flags, RF_IM_FIRE))
			vp[vn++] = "fire";
		if (rf_has(r_ptr->flags, RF_IM_COLD))
			vp[vn++] = "cold";
		if (rf_has(r_ptr->flags, RF_IM_POIS))
			vp[vn++] = "poison";

		if (vn) {
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" resists ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect resistances */
		vn = 0;
		if (rsf_has(r_ptr->flags, RSF_BRTH_LIGHT))
			vp[vn++] = "light";
		if ((rsf_has(r_ptr->flags, RSF_BRTH_DARK)) ||
			(rf_has(r_ptr->flags, RF_MORGUL_MAGIC)) ||
			(rf_has(r_ptr->flags, RF_ORC)))
			vp[vn++] = "darkness";

		if (rsf_has(r_ptr->flags, RSF_BRTH_CONFU))
			vp[vn++] = "confusion";
		if (rsf_has(r_ptr->flags, RSF_BRTH_SOUND))
			vp[vn++] = "sound";
		if (rsf_has(r_ptr->flags, RSF_BRTH_SHARD))
			vp[vn++] = "shards";
		if (rsf_has(r_ptr->flags, RSF_BRTH_INER))
			vp[vn++] = "inertia";
		if (rsf_has(r_ptr->flags, RSF_BRTH_GRAV))
			vp[vn++] = "gravity";
		if (rsf_has(r_ptr->flags, RSF_BRTH_FORCE))
			vp[vn++] = "force";
		if ((rf_has(r_ptr->flags, RF_RES_WATE)) || (prefix(name, "Water")))
			vp[vn++] = "water";

		if ((rsf_has(r_ptr->flags, RSF_BRTH_PLAS))
			|| (rf_has(r_ptr->flags, RF_RES_PLAS)) || ((vn)
													   &&
													   ((rf_has
														 (r_ptr->flags,
														  RF_IM_ELEC))
														||
														(rf_has
														 (r_ptr->flags,
														  RF_IM_FIRE))))
			|| prefix(name, "Plasma"))
			vp[vn++] = "plasma";

		if ((rf_has(r_ptr->flags, RF_RES_NEXUS)) || prefix(name, "Nexus")
			|| (rsf_has(r_ptr->flags, RSF_BRTH_NEXUS)))
			vp[vn++] = "nexus";
		if ((rf_has(r_ptr->flags, RF_UNDEAD))
			|| (rf_has(r_ptr->flags, RF_RES_NETH))
			|| (rsf_has(r_ptr->flags, RSF_BRTH_NETHR)))
			vp[vn++] = "nether";
		if ((rf_has(r_ptr->flags, RF_RES_DISE))
			|| (rsf_has(r_ptr->flags, RSF_BRTH_DISEN))
			|| prefix(name, "Disen"))
			vp[vn++] = "disenchantment";
		if (rsf_has(r_ptr->flags, RSF_BRTH_TIME))
			vp[vn++] = "time";

		if (vn) {
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" resists ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" and ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		/* Collect non-effects */
		vn = 0;
		if ((rf_has(r_ptr->flags, RF_NO_STUN))
			|| (rsf_has(r_ptr->flags, RSF_BRTH_SOUND))
			|| (rsf_has(r_ptr->flags, RSF_BRTH_FORCE)))
			vp[vn++] = "stunned";
		if (rf_has(r_ptr->flags, RF_NO_FEAR))
			vp[vn++] = "frightened";
		if ((rf_has(r_ptr->flags, RF_NO_CONF))
			|| (rsf_has(r_ptr->flags, RSF_BRTH_CONFU))
			|| (rsf_has(r_ptr->flags, RSF_BRTH_CHAOS)))
			vp[vn++] = "confused";
		if (rf_has(r_ptr->flags, RF_NO_SLEEP))
			vp[vn++] = "slept";

		if (vn) {
			spoil_out(wd_che[msex]);
			for (i = 0; i < vn; i++) {
				if (!i)
					spoil_out(" cannot be ");
				else if (i < vn - 1)
					spoil_out(", ");
				else
					spoil_out(" or ");
				spoil_out(vp[i]);
			}
			spoil_out(".  ");
		}

		spoil_out(wd_che[msex]);
		if (r_ptr->sleep > 200)
			spoil_out(" prefers to ignore");
		else if (r_ptr->sleep > 95)
			spoil_out(" pays very little attention to");
		else if (r_ptr->sleep > 75)
			spoil_out(" pays little attention to");
		else if (r_ptr->sleep > 45)
			spoil_out(" tends to overlook");
		else if (r_ptr->sleep > 25)
			spoil_out(" takes quite a while to see");
		else if (r_ptr->sleep > 10)
			spoil_out(" takes a while to see");
		else if (r_ptr->sleep > 5)
			spoil_out(" is fairly observant of");
		else if (r_ptr->sleep > 3)
			spoil_out(" is observant of");
		else if (r_ptr->sleep > 1)
			spoil_out(" is very observant of");
		else if (r_ptr->sleep > 0)
			spoil_out(" is vigilant for");
		else
			spoil_out(" is ever vigilant for");

		sprintf(buf, " intruders, which %s may notice from %d feet.  ",
				wd_lhe[msex], (MODE(SMALL_DEVICE) ? 5 : 10) * r_ptr->aaf);
		spoil_out(buf);

		i = 0;
		if (rf_has(r_ptr->flags, RF_DROP_60))
			i += 1;
		if (rf_has(r_ptr->flags, RF_DROP_90))
			i += 2;
		if (rf_has(r_ptr->flags, RF_DROP_1D2))
			i += 2;
		if (rf_has(r_ptr->flags, RF_DROP_2D2))
			i += 4;
		if (rf_has(r_ptr->flags, RF_DROP_3D2))
			i += 6;
		if (rf_has(r_ptr->flags, RF_DROP_4D2))
			i += 8;

		/* Drops gold and/or items */
		if (i) {
			sin = FALSE;
			spoil_out(wd_che[msex]);
			spoil_out(" will carry");

			if (i == 1) {
				spoil_out(" a");
				sin = TRUE;
			} else if (i == 2) {
				spoil_out(" one or two");
			} else {
				sprintf(buf, " up to %u", i);
				spoil_out(buf);
			}

			if (rf_has(r_ptr->flags, RF_DROP_GREAT)) {
				if (sin)
					spoil_out("n");
				spoil_out(" exceptional object");
			} else if (rf_has(r_ptr->flags, RF_DROP_GOOD)) {
				spoil_out(" good object");
			} else if (rf_has(r_ptr->flags, RF_DROP_CHEST)) {
				spoil_out(" chest");
			} else if (rf_has(r_ptr->flags, RF_ONLY_ITEM)) {
				if (sin)
					spoil_out("n");
				spoil_out(" object");
			} else if (rf_has(r_ptr->flags, RF_ONLY_GOLD)) {
				spoil_out(" treasure");
			} else {
				if (sin)
					spoil_out("n");
				spoil_out(" object");
				if (i > 1)
					spoil_out("s");
				spoil_out(" or treasure");
			}
			if (i > 1)
				spoil_out("s");

			if (rf_has(r_ptr->flags, RF_DROP_CHOSEN)) {
				spoil_out(", in addition to chosen objects");
			}

			spoil_out(".  ");
		}

		/* Count the actual attacks */
		for (i = 0, j = 0; j < 4; j++) {
			if (r_ptr->blow[j].method)
				i++;
		}

		/* Examine the actual attacks */
		for (k = 0, j = 0; j < 4; j++) {
			if (!r_ptr->blow[j].method)
				continue;

			/* No method yet */
			p = "???";

			/* Acquire the method */
			switch (r_ptr->blow[j].method) {
			case RBM_HIT:
				p = "hit";
				break;
			case RBM_TOUCH:
				p = "touch";
				break;
			case RBM_PUNCH:
				p = "punch";
				break;
			case RBM_KICK:
				p = "kick";
				break;
			case RBM_CLAW:
				p = "claw";
				break;
			case RBM_BITE:
				p = "bite";
				break;
			case RBM_STING:
				p = "sting";
				break;
			case RBM_XXX1:
				break;
			case RBM_BUTT:
				p = "butt";
				break;
			case RBM_CRUSH:
				p = "crush";
				break;
			case RBM_ENGULF:
				p = "engulf";
				break;
			case RBM_XXX2:
				break;
			case RBM_CRAWL:
				p = "crawl on you";
				break;
			case RBM_DROOL:
				p = "drool on you";
				break;
			case RBM_SPIT:
				p = "spit";
				break;
			case RBM_XXX3:
				break;
			case RBM_GAZE:
				p = "gaze";
				break;
			case RBM_WAIL:
				p = "wail";
				break;
			case RBM_SPORE:
				p = "release spores";
				break;
			case RBM_XXX4:
				break;
			case RBM_BEG:
				p = "beg";
				break;
			case RBM_INSULT:
				p = "insult";
				break;
			case RBM_SNEER:
				p = "moan";
				break;
			case RBM_REQUEST:
				p = "offer to trade";
				break;
			}


			/* Default effect */
			q = "???";

			/* Acquire the effect */
			switch (r_ptr->blow[j].effect) {
			case RBE_HURT:
				q = "attack";
				break;
			case RBE_POISON:
				q = "poison";
				break;
			case RBE_UN_BONUS:
				q = "disenchant";
				break;
			case RBE_UN_POWER:
				q = "drain charges";
				break;
			case RBE_EAT_GOLD:
				q = "steal gold";
				break;
			case RBE_EAT_ITEM:
				q = "steal items";
				break;
			case RBE_EAT_FOOD:
				q = "eat your food";
				break;
			case RBE_EAT_LIGHT:
				q = "absorb light";
				break;
			case RBE_ACID:
				q = "shoot acid";
				break;
			case RBE_ELEC:
				q = "electrify";
				break;
			case RBE_FIRE:
				q = "burn";
				break;
			case RBE_COLD:
				q = "freeze";
				break;
			case RBE_BLIND:
				q = "blind";
				break;
			case RBE_CONFUSE:
				q = "confuse";
				break;
			case RBE_TERRIFY:
				q = "terrify";
				break;
			case RBE_PARALYZE:
				q = "paralyze";
				break;
			case RBE_LOSE_STR:
				q = "reduce strength";
				break;
			case RBE_LOSE_INT:
				q = "reduce intelligence";
				break;
			case RBE_LOSE_WIS:
				q = "reduce wisdom";
				break;
			case RBE_LOSE_DEX:
				q = "reduce dexterity";
				break;
			case RBE_LOSE_CON:
				q = "reduce constitution";
				break;
			case RBE_LOSE_CHR:
				q = "reduce charisma";
				break;
			case RBE_LOSE_ALL:
				q = "reduce all stats";
				break;
			case RBE_SHATTER:
				q = "shatter";
				break;
			case RBE_EXP_10:
				q = "lower experience (by 10d6+)";
				break;
			case RBE_EXP_20:
				q = "lower experience (by 20d6+)";
				break;
			case RBE_EXP_40:
				q = "lower experience (by 40d6+)";
				break;
			case RBE_EXP_80:
				q = "lower experience (by 80d6+)";
				break;
			}


			if (!k) {
				spoil_out(wd_che[msex]);
				spoil_out(" can ");
			} else if (k < i - 1) {
				spoil_out(", ");
			} else {
				spoil_out(", and ");
			}

			/* Describe the method */
			spoil_out(p);

			/* Describe the effect, if any */
			if (r_ptr->blow[j].effect) {
				spoil_out(" to ");
				spoil_out(q);
				if (r_ptr->blow[j].d_dice && r_ptr->blow[j].d_side) {
					spoil_out(" with damage");
					if (r_ptr->blow[j].d_side == 1)
						sprintf(buf, " %d", r_ptr->blow[j].d_dice);
					else
						sprintf(buf, " %dd%d",
								r_ptr->blow[j].d_dice,
								r_ptr->blow[j].d_side);
					spoil_out(buf);
				}
			}

			k++;
		}

		if (k) {
			spoil_out(".  ");
		} else if (rf_has(r_ptr->flags, RF_NEVER_BLOW)) {
			sprintf(buf, "%s has no physical attacks.  ", wd_che[msex]);
			spoil_out(buf);
		}

		spoil_out(NULL);
	}

	/* Check for errors */
	if (!file_close(fff)) {
		msg("Cannot close spoiler file.");
		return;
	}

	msg("Successfully created a spoiler file.");
}






/**
 * Forward declare
 */
extern void do_cmd_spoilers(void);

/**
 * Create Spoiler files
 */
void do_cmd_spoilers(void)
{
	int ch;
	struct keypress key;

	/* Save screen */
	screen_save();


	/* Drop priv's */
	safe_setuid_drop();


	/* Interact */
	while (1) {
		/* Clear screen */
		Term_clear();

		/* Info */
		prt("Create a spoiler file.", 2, 0);

		/* Prompt for a file */
		prt("(1) Brief Object Info   (obj-desc.spo)", 5, 5);
		prt("(2) Brief Artifact Info (artifact.spo) (unavailable)", 6, 5);
		prt("(3) Brief Monster Info  (mon-desc.spo)", 7, 5);
		prt("(4) Full Monster Info   (mon-info.spo)", 8, 5);

		prt("(5) See what objects appear on this level  (obj-gen.spo)", 10,
			5);
		prt("(6) See what monsters appear on this level (mon-gen.spo)", 11,
			5);

		/* Prompt */
		prt("Command: ", 12, 0);

		/* Get a choice */
		key = inkey();
		ch = key.code;

		/* Escape */
		if (ch == ESCAPE) {
			break;
		}

		/* Option (1) */
		else if (ch == '1') {
			spoil_obj_desc("obj-desc.spo");
		}

		/* Option (2) */
		else if (ch == '2') {
			bell("I said unavailable!");
			if (0)
				spoil_artifact("artifact.spo");
		}

		/* Option (3) */
		else if (ch == '3') {
			spoil_mon_desc("mon-desc.spo");
		}

		/* Option (4) */
		else if (ch == '4') {
			spoil_mon_info("mon-info.spo");
		}

		/* Option (5) */
		else if (ch == '5') {
			spoil_obj_gen("obj-gen.spo");
		}

		/* Option (6) */
		else if (ch == '6') {
			spoil_mon_gen("mon-gen.spo");
		}

		/* Oops */
		else {
			bell("Illegal command for spoilers!");
		}

		/* Flush messages */
		message_flush();
	}


	/* Grab priv's */
	safe_setuid_grab();


	/* Load screen */
	screen_load();
}


#else

#ifdef MACINTOSH
static int i = 0;
#endif

#endif
