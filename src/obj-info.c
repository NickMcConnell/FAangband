/*
 * File: obj-info.c
 * Purpose: Object description code.
 *
 * Copyright (c) 2010 Andi Sidwell
 * Copyright (c) 2004 Robert Ruehlmann
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
#include "effects.h"
#include "cmds.h"
#include "tvalsval.h"
#include "z-textblock.h"

/*
 * Describes a flag-name pair.
 */
typedef struct
{
	int flag;
	const char *name;
} flag_type;


/*
 * Describes a colour-name pair.
 */
typedef struct
{
	byte attr;
	const char *name;
} property_type;


/*** Utility code ***/

/*
 * Given an array of strings, as so:
 *  { "intelligence", "fish", "lens", "prime", "number" },
 *
 * ... output a list like "intelligence, fish, lens, prime, number.\n".
 */
static void info_out_list(textblock *tb, const char *list[], size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
	{
		textblock_append(tb, list[i]);
		if (i != (count - 1)) textblock_append(tb, ", ");
	}

	textblock_append(tb, ". ");
}


/*
 *
 */
static size_t info_collect(const flag_type list[], size_t max,
		const bitflag flags[OF_SIZE], const char *recepticle[])
{
	size_t i, count = 0;

	for (i = 0; i < max; i++)
	{
		if (of_has(flags, list[i].flag))
			recepticle[count++] = list[i].name;
	}

	return count;
}


/*** Big fat data tables ***/

static const char *statname[] = 
{ 
    "strength", 
    "intelligence", 
    "wisdom",
    "dexterity", 
    "constitution", 
    "charisma"
};

static const char *othername[] = 
{ 
    "magic mastery", 
    "stealth", 
    "searching",
    "infravision", 
    "tunnelling", 
    "speed",
    "shooting speed", 
    "shooting power"
};

static const char *slayee[] = 
{ 
    "animals", 
    "evil creatures", 
    "undead", 
    "demons",
    "orcs", 
    "trolls", 
    "giants", 
    "dragons"
};

static const char *brandee[] = 
{ 
    "acid", 
    "lightning", 
    "fire", 
    "cold", 
    "poison" 
};

static const property_type resists[]= 
{
    { TERM_SLATE, "acid"},
    { TERM_BLUE, "electricity"},
    { TERM_RED, "fire"},
    { TERM_L_WHITE, "frost"},
    { TERM_GREEN, "poison"},
    { TERM_ORANGE, "light"},
    { TERM_L_DARK, "darkness"},
    { TERM_L_UMBER, "confusion"},
    { TERM_YELLOW, "sound"},
    { TERM_UMBER, "shards"},
    { TERM_L_PURPLE, "nexus"},
    { TERM_L_GREEN, "nether"},
    { TERM_VIOLET, "chaos"},
    { TERM_L_VIOLET, "disenchantment"},
};


static const flag_type ignore_flags[] =
{
	{ OF_ACID_PROOF, "acid" },
	{ OF_ELEC_PROOF, "electricity" },
	{ OF_FIRE_PROOF, "fire" },
	{ OF_COLD_PROOF, "cold" },
};

static const flag_type sustain_flags[] =
{
	{ OF_SUSTAIN_STR, "strength" },
	{ OF_SUSTAIN_INT, "intelligence" },
	{ OF_SUSTAIN_WIS, "wisdom" },
	{ OF_SUSTAIN_DEX, "dexterity" },
	{ OF_SUSTAIN_CON, "constitution" },
	{ OF_SUSTAIN_CHR, "charisma" },
};

static const flag_type misc_flags[] =
{
    { OF_FEARLESS,           "Renders you fearless" },
    { OF_SEEING,             "Provides resistance to blindness" },
    { OF_LIGHT,              "Provides permanent light" },
    { OF_IMPACT,             "Induces earthquakes" },
    { OF_DARKNESS,           "Allows you to see in the dark" },
    { OF_CHAOTIC,            "Causes chaotic effects" },
    { OF_BLESSED,            "Blessed by the gods" },
    { OF_SLOW_DIGEST,        "Slows your metabolism" },
    { OF_FEATHER,            "Makes you fall lightly" },
    { OF_REGEN,              "Speeds regeneration" },
    { OF_FREE_ACT,           "Prevents paralysis" },
    { OF_HOLD_LIFE,          "Sustains your life force" },
    { OF_TELEPATHY,          "Grants telepathy" },
    { OF_SEE_INVIS,          "Grants the ability to see invisible things" },
};

static const flag_type curses[] =
{
    { CF_TELEPORT,        "Induces random teleportation" },
    { CF_NO_TELEPORT,     "Prevents teleportation" },
    { CF_AGGRO_PERM,      "Aggravates nearby creatures" },
    { CF_AGGRO_RAND,      "Occasionally greatly aggravates nearby creatures" },
    { CF_SLOW_REGEN,      "Slows your regeneration" },
    { CF_AFRAID,          "Makes you afraid" },
    { CF_HUNGRY,          "Speeds your digestion" },
    { CF_POIS_RAND,       "Occasionally poisons you" },
    { CF_POIS_RAND_BAD,   "Randomly envelops you in poison" },
    { CF_CUT_RAND,        "Occasionally cuts you" },
    { CF_CUT_RAND_BAD,    "Will suddenly cause deep wounds" },
    { CF_HALLU_RAND,      "Sometimes makes you hallucinate" },
    { CF_DROP_WEAPON,     "Will suddenly leap from your grasp" },
    { CF_ATTRACT_DEMON,   "Makes demons suddenly appear nearby" },
    { CF_ATTRACT_UNDEAD,  "Calls the undead from their slumber" },
    { CF_STICKY_CARRY,    "Cannot be dropped from your pack" },
    { CF_STICKY_WIELD,    "Sticks to you if wielded" },
    { CF_PARALYZE,        "Will briefly paralyze you" },
    { CF_PARALYZE_ALL,    "Can paralyze you even if you feel immune" },
    { CF_DRAIN_EXP,       "Drains experience" },
    { CF_DRAIN_MANA,      "Drains mana" },
    { CF_DRAIN_STAT,      "Will sometimes lower a stat" },
    { CF_DRAIN_CHARGE,    "Will take energy from your magic devices" },
};



/**
 * General information about classes of objects. -LM-
 * 
 * Index is tval.
 */
char *obj_class_info[101] = {
    "", "", "", "", "",
    "", "",
    "Chests may have some really good treasures hidden inside, but can be perilous to open...",
    "", "",

    "", "", "", "", "",
    "", "", "", "", 
    //"", "Sling ammo.", "Bow ammo.", "Crossbow ammo.",
    "Missile launchers allow you to inflict damage from a distance without using magic.",

    "Diggers, especially heavy ones, are invaluable for forced entry and escape and can make a lucky miner rich.",
    "Hafted weapons rely on blunt force to inflict damage.  Since they spill relatively little blood, priests much prefer to carry one.",
    "Pole-mounted weapons are often cumbersome and may require two hands to wield, but some offer both a high degree of protection and powerful attacks.",
    "The effectiveness of edged weapons depends on keen edges and sharp points.  They tend to be quite light and are easy to use, but some may not deal enough damage for your liking.",
    "",
    "", "", "", "", "",

    "Footwear protects the feet only, but some rare items of this type have magics to render them fleet, light, or steady.",
    "Your hands would benefit from protection too, but most magic users need to keep their fingers unencumbered or magically supple.",
    "Many a blow will be struck upon your head, and protection here will certainly be helpful.  Some rare items may protect and enhance your mind.",
    "Many a blow will be struck upon your head, and protection here will certainly be helpful.  Some rare items may protect and enhance your mind.",
    "Shields can be worn on your arm, or on your back if you need both hands to use your weapon.  So protective can a shield be that it can reduce damage as much or more than body armour, and you can perhaps deflect physical missiles (even shards) or take advantage of opportunities to bash your foe if you have one on your arm.",
    "Experienced adventurers wrap a cloak around their body.  Some rare items of this type may allow you to slip silently around less alert enemies.",
    "Some kind of body protection will become a necessity as you face ever more dangerous opponents; rare items of this type may hold many and varied protective magics.",
    "Some kind of body protection will become a necessity as you face ever more dangerous opponents; rare items of this type may hold many and varied protective magics.",
    "Armour made of dragon scales is rare indeed, and powerful dragon magics allow you to sometimes breathe even as great dragons do.",
    "An adventurer who cannot see is jackal food.  The further away your illumination ends, the greater your chance to ready yourself for desperate combat.",

    "Amulets slip around your neck, and almost all have magics wondrous or perilous bound inside.",
    "", "", "", "",
    "You may wear a ring upon each of your two ring fingers, and benefit or suffer from the magics it contains.",
    "", "", "", "",

    "", "", "", "", "",
    "Staffs are heavy, and take up plenty of space in your backpack, but can hold a lot of sometimes powerful spells that affect large areas.  Staffs recharge easily and well.",
    "", "", "", "",

    "", "", "", "", "",
    "Wands hold a variety of spells, useful both in combat and for exploration.  Bolt spells in wands often beam, and ball spells affect large areas.  Once its charges are exhausted, a wand is useless until recharged.",
    "The magics stored in rods never run out, given enough time between uses to recover.  Rods can do a lot of damage, but they affect only small areas.  Bolt spells in rods seldom or never beam.",
    "", "", "",

    "One will often find sheets of parchment with magic spells.  They are easy to read, and are a warrior or paladin's best chance at making use of magic.",
    "", "", "", "",
    "Healers, alchemists, and sages create potions in vast quantities, and store them in small flasks of clear glass or gleaming crystal.  Once quaffed, a potion is guaranteed to work, but not every strange liquid was mixed for your benefit...",
    "", "", "", "",

    "Deep in the murky dungeons strange mushrooms grow, and monsters rout about sealed packets of food that their owners will never need again.",
    "", "", "", "",
    "", "", "", "", "",

    //"A manual of sorcerous magics, bound in fine linen dyed deep red.",
    //"A shining gospel of piety in flawless white and gold.",
    //"A runed stone with earthly and natural magics chiseled in brown and rich greens.",
    //"A black tome of necromantic rituals, with shadows lying deep around it.",
    "",
    "", "", "", "", "",
    "", "", "", "",

    "Small valuables and coins."
};


/*** Code that makes use of the data tables ***/

/*
 * Describe an item's curses.
 */
static bool describe_curses(textblock *tb, const object_type *o_ptr,
			    oinfo_detail_t mode)
{
    size_t i;
    bool printed = FALSE;
    bool full = mode & OINFO_FULL;
    bool terse = mode & OINFO_TERSE;
    bool dummy = mode & OINFO_DUMMY;

    for (i = 0; i < N_ELEMENTS(curses); i++)
    {
	if (cf_has(full ? o_ptr->flags_curse : o_ptr->id_curse, curses[i].flag))
	{
	    if (!printed) textblock_append(tb, "\nCurses:  ");
	    textblock_append(tb, "%s. ", curses[i].name);
	    printed = TRUE;
	}
    }

    if (printed)
    {
    	textblock_append(tb, "\n");
	if (of_has(o_ptr->flags_obj, OF_FRAGILE))
	    textblock_append(tb, "Attempting to uncurse it may destroy it.\n");
    }

    if (terse || dummy) return printed;

    /* Only look at wieldables */
    if (wield_slot(o_ptr) >= INVEN_WIELD)
    {
	/* All normal properties known */
	if (o_ptr->ident & IDENT_KNOWN)
        {
	    /* Everything known */
	    if (o_ptr->ident & (IDENT_KNOW_CURSES | IDENT_UNCURSED))
	    {
		textblock_append(tb, "You know all about this object.\n");
		printed = TRUE;
	    }

	    /* Some unknown curses */
	    else 
	    {
		textblock_append(tb, "You know all the enchantments on this object.\n");
		printed = TRUE;
	    }
        }
	
	/* Curses known */
	else
        {
	    /* Known uncursed */
	    if (o_ptr->ident & IDENT_UNCURSED)
	    {
		textblock_append(tb, "It is not cursed.\n");
		printed = TRUE;
	    }

	    /* Say if all curses are known */
	    else if (o_ptr->ident & IDENT_KNOW_CURSES) 
	    {
		textblock_append(tb, "You know all the curses on this object.\n");
		printed = TRUE;
	    }	    
        }
    }

    return printed;
}


/*
 * Describe stat modifications.
 */
static bool describe_stats(textblock *tb, const object_type *o_ptr,
		oinfo_detail_t mode)
{
    int j, min = 0, max = 0, count = 0;
    bool full = mode & OINFO_FULL;
    bool dummy = mode & OINFO_DUMMY;
    bool terse = mode & OINFO_TERSE;

    if (((o_ptr->ident) & IDENT_WORN) || dummy || full) {
	for (j = 0; j < A_MAX; j++) {
	    if (o_ptr->bonus_stat[j] != 0)
		count++;
	    if (o_ptr->bonus_stat[j] < min)
		min = o_ptr->bonus_stat[j];
	    if (o_ptr->bonus_stat[j] > max)
		max = o_ptr->bonus_stat[j];
	}
    }

    if (count > 0) {
	byte attr = (o_ptr->bonus_stat[A_STR] > 0 ? TERM_L_GREEN : TERM_ORANGE);

	if (!terse) {	
	    textblock_append(tb, "It gives ");
	    if (dummy) textblock_append(tb, "up to ");
	}

	/* Special case: all stats */
	if (min == max) 
	    textblock_append_c(tb, attr, "%d to all your stats. ", min);

	/* Some stats */
	else {
	    for (j = 0; j < A_MAX; j++) {
		if (o_ptr->bonus_stat[j] == 0)
		    continue;
		attr = (o_ptr->bonus_stat[j] > 0 ? TERM_L_GREEN : TERM_ORANGE);
		textblock_append_c(tb, attr, "%d ", o_ptr->bonus_stat[j]);
		if (!terse) textblock_append(tb, "to your ");
		textblock_append_c(tb, attr, statname[j]);
		if (count >= (terse ? 2 : 3))
		    textblock_append(tb, ", ");
		if ((count == 2) && !terse)
		    textblock_append(tb, " and ");
		if (count == 1)
		    textblock_append(tb, ". ");
		count--;
	    }
	}

	textblock_append(tb, "\n");
	return TRUE;
    }
	
    return FALSE;
}



/*
 * Describe other bonuses.
 */
static bool describe_bonus(textblock *tb, const object_type *o_ptr,
		oinfo_detail_t mode)
{
    int j, count = 0;
    bool full = mode & OINFO_FULL;
    bool dummy = mode & OINFO_DUMMY;
    bool terse = mode & OINFO_TERSE;

    if (((o_ptr->ident) & IDENT_WORN) || dummy || full) {
	for (j = 0; j < MAX_P_BONUS; j++) {
	    if (o_ptr->bonus_other[j] != 0)
		count++;
	}
    }

    if (count > 0) {
	byte attr = (o_ptr->bonus_other[0] > 0 ? TERM_L_GREEN : TERM_ORANGE);

	if (!terse) {	
	    textblock_append(tb, "It gives ");
	    if (dummy) textblock_append(tb, "up to ");
	}

	/* Bonuses */
	for (j = 0; j < MAX_P_BONUS; j++) {
	    if (o_ptr->bonus_other[j] == 0)
		continue;
	    attr = (o_ptr->bonus_other[j] > 0 ? TERM_L_GREEN : TERM_ORANGE);
	    textblock_append_c(tb, attr, "%d ", o_ptr->bonus_other[j]);
	    if (!terse) textblock_append(tb, "to your ");
	    textblock_append_c(tb, attr, othername[j]);
	    if (count >= (terse ? 2 : 3))
		textblock_append(tb, ", ");
	    if ((count == 2) && !terse)
		textblock_append(tb, " and ");
	    if (count == 1)
		textblock_append(tb, ". ");
	    count--;
	}
	
	textblock_append(tb, "\n");
	return TRUE;
    }
	
    return FALSE;
}


/*
 * Describe slays.
 */
static bool describe_slays(textblock *tb, const object_type *o_ptr,
		oinfo_detail_t mode)
{
    int j, slay = 0, kill = 0;
    bool full = mode & OINFO_FULL;
    bool terse = mode & OINFO_TERSE;
    byte attr = TERM_RED;

    for (j = 0; j < MAX_P_SLAY; j++)
	if (if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j) || 
	    (full && (o_ptr->multiple_slay[j] > MULTIPLE_BASE))) {
	    slay++;

	    /* Hack for great banes */
	    if ((j == P_SLAY_ANIMAL)
		&& (o_ptr->multiple_slay[j] > SLAY_BOOST_MINOR)) {
		slay--;
		kill++;
	    }
	    if ((j == P_SLAY_EVIL)
		&& (o_ptr->multiple_slay[j] > SLAY_BOOST_SMALL)) {
		slay--;
		kill++;
	    }
	    if ((j > P_SLAY_EVIL)
		&& (o_ptr->multiple_slay[j] > SLAY_BOOST_NORMAL)) {
		slay--;
		kill++;
	    }
	}

    if (!slay && !kill) return FALSE;

    if (slay > 0) {
	textblock_append(tb, "It slays ");

	/* Slays */
	for (j = 0; j < MAX_P_SLAY; j++) {
	    if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j) && 
		!(full && (o_ptr->multiple_slay[j] > MULTIPLE_BASE)))
		continue;
	    if ((j == P_SLAY_ANIMAL) && 
		(o_ptr->multiple_slay[j] > SLAY_BOOST_MINOR))
		continue;
	    if ((j == P_SLAY_EVIL) && 
		(o_ptr->multiple_slay[j] > SLAY_BOOST_SMALL))
		continue;
	    if ((j > P_SLAY_EVIL) && 
		(o_ptr->multiple_slay[j] > SLAY_BOOST_NORMAL))
		continue;
	    textblock_append_c(tb, attr, slayee[j]);
	    if (slay >= 3)
		textblock_append(tb, ", ");
	    if (slay == 2)
		textblock_append(tb, " and ");
	    if (slay == 1)
		textblock_append(tb, ". ");
	    slay--;
	}
    }
    
    if (kill > 0) {
	if (terse) textblock_append(tb, "Great bane of ");
	else textblock_append(tb, "It is a great bane of ");

	/* Great banes */
	for (j = 0; j < MAX_P_SLAY; j++) {
	    if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j) && 
		!(full && (o_ptr->multiple_slay[j] > MULTIPLE_BASE))) 
		continue;
	    if ((j == P_SLAY_ANIMAL) && 
		(o_ptr->multiple_slay[j] <= SLAY_BOOST_MINOR))
		continue;
	    if ((j == P_SLAY_EVIL) && 
		(o_ptr->multiple_slay[j] <= SLAY_BOOST_SMALL))
		continue;
	    if ((j > P_SLAY_EVIL) && 
		(o_ptr->multiple_slay[j] <= SLAY_BOOST_NORMAL))
		continue;
	    textblock_append_c(tb, attr, slayee[j]);
	    if (slay >= 3)
		textblock_append(tb, ", ");
	    if (slay == 2)
		textblock_append(tb, " and ");
	    if (slay == 1)
		textblock_append(tb, ". ");
	    slay--;
	}
    }
    
    textblock_append(tb, "\n");
    return TRUE;
}


/*
 * Describe brands.
 */
static bool describe_brands(textblock *tb, const object_type *o_ptr,
		oinfo_detail_t mode)
{
    int j, brand = 0;
    bool full = mode & OINFO_FULL;
    bool terse = mode & OINFO_TERSE;
    byte attr = TERM_L_UMBER;

    for (j = 0; j < MAX_P_BRAND; j++)
	if (if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + j) || 
	    (full && (o_ptr->multiple_brand[j] > MULTIPLE_BASE))) 
	    brand++;
    
    if (brand > 0) {
	if (terse) textblock_append(tb, "Branded with ");
	else textblock_append(tb, "It does extra damage from ");

	/* Brands */
	for (j = 0; j < MAX_P_BRAND; j++) {
	    if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + j) && 
		!(full && (o_ptr->multiple_brand[j] > MULTIPLE_BASE)))
		continue;
	    textblock_append_c(tb, attr, brandee[j]);
	    if (brand >= 3)
		textblock_append(tb, ", ");
	    if (brand == 2)
		textblock_append(tb, " and ");
	    if (brand == 1)
		textblock_append(tb, ". ");
	    brand--;
	}
    
	textblock_append(tb, "\n");
	return TRUE;
    }

    return FALSE;
}


/*
 * Describe immunities granted by an object.
 */
static bool describe_immune(textblock *tb, const object_type *o_ptr, 
			    oinfo_detail_t mode)
{
    int res = 0, imm = 0, vul = 0, j;

    bool full = mode & OINFO_FULL;
    bool dummy = mode & OINFO_DUMMY;
    bool terse = mode & OINFO_TERSE;
    bool prev = FALSE;

    /* Check for resists and vulnerabilities */
    for (j = 0; j < MAX_P_RES; j++) {
	if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)  && !full) 
	    continue;
	if (o_ptr->percent_res[j] == RES_BOOST_IMMUNE) 
	    imm++;
	else if (o_ptr->percent_res[j] < RES_LEVEL_BASE) {
	    res++;
	}
	else if (o_ptr->percent_res[j] > RES_LEVEL_BASE)
	    vul++;
    }

    /* Immunities */
    if (imm) {
	textblock_append(tb, "Provides ");
	textblock_append_c(tb, TERM_BLUE, "immunity ");
	textblock_append(tb, "to ");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 4; j++) {
	    if (o_ptr->percent_res[j] > RES_BOOST_IMMUNE)
		continue;
	    if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)  && !full)
		continue;

	    /* List the attribute description, in its proper place. */
	    if (terse)
		textblock_append(tb, resists[j].name);
	    else
		textblock_append_c(tb, resists[j].attr, resists[j].name);
	    if (imm >= (terse ? 2 : 3))
		textblock_append(tb, ", ");
	    if ((imm == 2) && !terse)
		textblock_append(tb, " and ");
	    imm--;
	}

	/* End sentence. */
	textblock_append(tb, ". ");
	prev = TRUE;
    }

    /* Resistances */
    if (res) {
	textblock_append(tb, "Provides ");
	textblock_append_c(tb, TERM_L_BLUE, "resistance ");
	textblock_append(tb, "to ");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < MAX_P_RES; j++) {
	    if (o_ptr->percent_res[j] >= RES_LEVEL_BASE)
		continue;
	    if (o_ptr->percent_res[j] == RES_BOOST_IMMUNE)
		continue;
	    if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)  && !full)
		continue;

	    /* List the attribute description, in its proper place. */
	    if (terse)
		textblock_append(tb, resists[j].name);
	    else
		textblock_append_c(tb, resists[j].attr, resists[j].name);
	    textblock_append(tb, "(");
	    if (dummy) textblock_append(tb, "about ");
	    textblock_append(tb, "%d%%)", RES_LEVEL_BASE - o_ptr->percent_res[j]);
	    if (res >= (terse ? 2 : 3))
		textblock_append(tb, ", ");
	    if ((res == 2) && !terse)
		textblock_append(tb, " and ");
	    res--;
	}

	/* End sentence. */
	textblock_append(tb, ". ");
	prev = TRUE;
    }

    /* Vulnerabilities */
    if (vul) {
	textblock_append(tb, "Makes you ");
	textblock_append_c(tb, TERM_ORANGE, "vulnerable ");
	textblock_append(tb, "to ");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < MAX_P_RES; j++) {
	    if (o_ptr->percent_res[j] <= RES_LEVEL_BASE)
		continue;
	    if (!if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j))
		continue;

	    /* List the attribute description, in its proper place. */
	    if (terse)
		textblock_append(tb, resists[j].name);
	    else
		textblock_append_c(tb, resists[j].attr, resists[j].name);
	    textblock_append(tb, "(");
	    if (dummy) textblock_append(tb, "about ");
	    textblock_append(tb, "%d%%)", o_ptr->percent_res[j] - RES_LEVEL_BASE);
	    if (vul >= (terse ? 2 : 3))
		textblock_append(tb, ", ");
	    if ((vul == 2) && !terse)
		textblock_append(tb, " and ");
	    vul--;
	}

	/* End sentence. */
	textblock_append(tb, ". ");
	prev = TRUE;
    }

    return prev;
}


/*
 * Describe 'ignores' of an object.
 */
static bool describe_ignores(textblock *tb, const object_type *o_ptr, 
			      oinfo_detail_t mode)
{
    bool full = mode & OINFO_FULL;
    const char *descs[N_ELEMENTS(ignore_flags)];
    size_t count = info_collect(ignore_flags, N_ELEMENTS(ignore_flags),
				full ? o_ptr->flags_obj : o_ptr->id_obj, 
				descs);
    
    if (!count)
	return FALSE;
    
    textblock_append(tb, "Cannot be harmed by ");
    info_out_list(tb, descs, count);
    
    return TRUE;
}


/*
 * Describe stat sustains.
 */
static bool describe_sustains(textblock *tb, const object_type *o_ptr, 
			      oinfo_detail_t mode)
{

    bool full = mode & OINFO_FULL;
    const char *descs[N_ELEMENTS(sustain_flags)];
    size_t count = info_collect(sustain_flags, N_ELEMENTS(sustain_flags),
				full ? o_ptr->flags_obj : o_ptr->id_obj, 
				descs);

    if (!count)
	return FALSE;

    if (count == 6) textblock_append(tb, "It sustains all your stats. ");
    else {
	textblock_append(tb, "Sustains ");
	info_out_list(tb, descs, count);
    }

    return TRUE;
}


/*
 * Describe miscellaneous powers.
 */
static bool describe_misc_magic(textblock *tb, const object_type *o_ptr, 
			      oinfo_detail_t mode)
{
    bool full = mode & OINFO_FULL;
    size_t i;
    bool printed = FALSE;
    bitflag objflags[OF_SIZE];

    of_wipe(objflags);
    of_copy(objflags, o_ptr->flags_obj);
    if (!full)
	of_inter(objflags, o_ptr->id_obj);
	

    for (i = 0; i < N_ELEMENTS(misc_flags); i++)
    {
	if (of_has(objflags, misc_flags[i].flag))
	{
	    if (!printed) textblock_append(tb, "\nPowers:  ");
	    textblock_append(tb, "%s. ", misc_flags[i].name);
	    printed = TRUE;
	}
    }

    if (printed)
	textblock_append(tb, "\n");

    return printed;
}


#define CHECK_FIRST(txt, first) \
    if ((first)) { (first) = FALSE; textblock_append(tb, (txt)); }	\
  else textblock_append(tb, ", ");

/**
 * Display the damage done with a multiplier
 */
static void output_dam(textblock *tb, player_state *state, 
		       const object_type * o_ptr, int mult, 
		       const char *against, bool * first, 
		       oinfo_detail_t mode)
{
    int dam, die_average, add = 0, i, deadliness, crit, chance;
    bool full = mode & OINFO_FULL;

    /* Average damage for one standard side (x10) */
    die_average = (10 * (o_ptr->ds + 1)) / 2;

    /* Multiply by slay or brand (x10) */
    die_average *= mult;

    /* Record the addend */
    if (mult > MULTIPLE_BASE)
	add = (mult - MULTIPLE_BASE);

    /* Multiply by deadliness (x100) */
    deadliness = state->dis_to_d;
    if (if_has(o_ptr->id_other, IF_TO_D) || full)
	deadliness += o_ptr->to_d;
    if (deadliness > 150)
	deadliness = 150;
    if (deadliness < -150)
	deadliness = -150;
    if (deadliness >= 0)
	die_average *= (100 + deadliness_conversion[deadliness]);
    else {
	i = deadliness_conversion[ABS(deadliness)];
	if (i >= 100)
	    die_average = 0;
	else
	    die_average *= (100 - i);
    }

    /* Factor in criticals (x 10) */
    chance = state->skills[SKILL_TO_HIT_MELEE] + state->dis_to_h;
    if (object_known_p(o_ptr))
	chance += o_ptr->to_h;
    chance = (100 * chance) / (chance + 240);
    if (player_has(PF_ARMSMAN))
	chance = 100 - (83 * (100 - chance)) / 100;
    crit = 259 * chance;
    crit /= 1000;

    /* Multiply by number of sides */
    dam = die_average * (o_ptr->dd * 10 + crit);
    
    CHECK_FIRST("", *first);
    if ((dam > 50000) || (add > 0))
	textblock_append_c(tb, TERM_L_GREEN, "%d", add + dam / 100000);
    else
	textblock_append_c(tb, TERM_L_RED, "0");
    textblock_append(tb, " against %s", against);
}

/**
 * Outputs the damage we do/would do with the weapon
 */
static bool describe_weapon_damage(textblock *tb, const object_type *o_ptr,
				   oinfo_detail_t mode)
{
    object_type *i_ptr;
    int i, j;

    bool full = mode & OINFO_FULL;
    bool terse = mode & OINFO_TERSE;
    bool first = TRUE;
    int show_m_tohit;
    int brand[MAX_P_BRAND], slay[MAX_P_SLAY];

    player_state state;
    object_type inven[INVEN_TOTAL];

    /* Abort if we've nothing to say */
    if (mode & OINFO_DUMMY) return FALSE;

    /* Extract the slays and brands */
    for (j = 0; j < MAX_P_SLAY; j++)
	slay[j] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j) || full)
	    ? o_ptr->multiple_slay[j] : MULTIPLE_BASE;
    for (j = 0; j < MAX_P_BRAND; j++)
	brand[j] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + j) || full)
	    ? o_ptr->multiple_brand[j] : MULTIPLE_BASE;

    /* Check rings for additional brands (slays) */
    for (i = 0; i < 2; i++) {
	i_ptr = &p_ptr->inventory[INVEN_LEFT + i];

	/* If wearing a ring */
	if (i_ptr->k_idx) {
	    /* Pick up any brands (and slays!) */
	    for (j = 0; j < MAX_P_SLAY; j++)
		slay[j] = MAX(slay[j], ((if_has(i_ptr->id_other, OBJECT_ID_BASE_SLAY + j) || full)
					? i_ptr->multiple_slay[j] : MULTIPLE_BASE));
	    for (j = 0; j < MAX_P_BRAND; j++)
		brand[j] =
		    MAX(brand[j], ((if_has(i_ptr->id_other, OBJECT_ID_BASE_BRAND + j) || full)
				   ? i_ptr->multiple_brand[j] : MULTIPLE_BASE));
			
	}
    }

    /* temporary elemental brands */
    if (p_ptr->special_attack & (ATTACK_ACID))
	brand[P_BRAND_ACID] = MAX(brand[P_BRAND_ACID], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_ELEC))
	brand[P_BRAND_ELEC] = MAX(brand[P_BRAND_ELEC], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_FIRE))
	brand[P_BRAND_FIRE] = MAX(brand[P_BRAND_FIRE], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_COLD))
	brand[P_BRAND_COLD] = MAX(brand[P_BRAND_COLD], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_POIS))
	brand[P_BRAND_POIS] = MAX(brand[P_BRAND_POIS], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_HOLY))
	slay[P_SLAY_EVIL] = MAX(slay[P_SLAY_EVIL], SLAY_BOOST_SMALL);

    /*
     * Get the player's hypothetical state, were they to be
     * wielding this item (setting irrelevant shield state).
     */
    memcpy(inven, p_ptr->inventory, INVEN_TOTAL * sizeof(object_type));
    inven[INVEN_WIELD] = *o_ptr;
    state.shield_on_back = FALSE;

    calc_bonuses(inven, &state, TRUE);

    show_m_tohit = state.dis_to_h;
    if (if_has(o_ptr->id_other, IF_TO_H) || full)
	show_m_tohit += o_ptr->to_h;

    if (terse) {
	textblock_append_c(tb, TERM_L_GREEN, "%d ", state.num_blow);
	textblock_append(tb, "blow%s av. dam. ", (state.num_blow) ? "s" : "");
    } else {
	textblock_append(tb, "\nWielding it you would have ");
	textblock_append_c(tb, TERM_L_GREEN, "%d ", state.num_blow);
	textblock_append(tb, "blow%s and do an average damage per blow of ",
			 (state.num_blow) ? "s" : "");
    }

    for (i = 0; i < MAX_P_SLAY; i++) {
	if (slay[i] > MULTIPLE_BASE)
	    output_dam(tb, &state, o_ptr, slay[i], slayee[i], &first, mode);
    }

    for (i = 0; i < MAX_P_BRAND; i++) {
	if (brand[i] > MULTIPLE_BASE) {
	    char buf[40];

	    strnfmt(buf, sizeof(buf), "non %s resistant creatures", brandee[i]);
	    output_dam(tb, &state, o_ptr, brand[i], buf, &first, mode);
	}
    }

    output_dam(tb, &state, o_ptr, MULTIPLE_BASE,
	       (first) ? "all monsters" : "other monsters", &first, mode);

    if (terse) {
	textblock_append(tb, ".  + ");
	textblock_append_c(tb, TERM_L_GREEN, "%d", show_m_tohit);
	textblock_append(tb, " to skill. ");
    } else {
	textblock_append(tb, ".  Your + to Skill would be ");
	textblock_append_c(tb, TERM_L_GREEN, "%d", show_m_tohit);
	textblock_append(tb, ". ");
    }

    return TRUE;
}


/**
 * Display the ammo damage done with a multiplier
 */
static void output_ammo_dam(textblock *tb, player_state *state, 
			    const object_type * o_ptr, int mult,
			    const char *against, bool * first, bool perfect,
			    oinfo_detail_t mode)
{
    object_type *b_ptr = &p_ptr->inventory[INVEN_BOW];

    int dam, die_average, add = 0, i, deadliness, crit, chance, dice =
	o_ptr->dd;

    /* Throwing weapon, or launched missile? */
    bool thrown = !is_missile(o_ptr);
    bool full = mode & OINFO_FULL;

    /* Average damage for one standard side (x10) */
    die_average = (10 * (o_ptr->ds + 1)) / 2;

    /* Apply the launcher multiplier. */
    if (!thrown)
	die_average *= p_ptr->state.ammo_mult;

    /* Multiply by slay or brand (x10) */
    die_average *= mult;

    /* Record the addend */
    if (mult > 10)
	add = (mult - 10);

    /* Multiply by deadliness (x100) */
    deadliness = state->dis_to_d;
    if (if_has(o_ptr->id_other, IF_TO_D) || full)
	deadliness += o_ptr->to_d;
    if (if_has(b_ptr->id_other, IF_TO_D) || full)
	deadliness += b_ptr->to_d;
    if (deadliness > 150)
	deadliness = 150;
    if (deadliness < -150)
	deadliness = -150;
    if (deadliness >= 0)
	die_average *= (100 + deadliness_conversion[deadliness]);
    else {
	i = deadliness_conversion[ABS(deadliness)];
	if (i >= 100)
	    die_average = 0;
	else
	    die_average *= (100 - i);
    }

    /* Get critical chance (x 10) */
    chance = state->skills[SKILL_TO_HIT_BOW] + state->dis_to_h;
    if (if_has(o_ptr->id_other, IF_TO_H) || full)
	chance += o_ptr->to_h;
    if ((!thrown) && (if_has(b_ptr->id_other, IF_TO_H) || full))
	chance += b_ptr->to_h;
    if (thrown)
	chance = chance * 3 / 2;
    chance = (100 * chance) / (chance + 360);
    if (player_has(PF_MARKSMAN))
	chance = 100 - (83 * (100 - chance)) / 100;
    crit = 116 * chance;
    crit /= 1000;

    /* Increase dice */
    if (thrown && perfect)
	dice *= 2;
    dice = dice * 10 + crit;
    if (thrown)
	dice *= 2 + p_ptr->lev / 12;

    /* Multiply by number of sides */
    dam = die_average * dice;

    CHECK_FIRST("", *first);
    if ((dam > 50000) || (add > 0))
	textblock_append_c(tb, TERM_L_GREEN, "%d", add + dam / 100000);
    else
	textblock_append_c(tb, TERM_L_RED, "0");
    textblock_append(tb, " against %s", against);
}

/**
 * Outputs the damage we do/would do with the current bow and this ammo
 */
static bool describe_ammo_damage(textblock *tb, const object_type * o_ptr,
				oinfo_detail_t mode)
{
    int i;
    object_type *i_ptr;

    bool full = mode & OINFO_FULL;
    bool terse = mode & OINFO_TERSE;
    bool first = TRUE;
    bool perfect = (of_has(o_ptr->id_obj, OF_PERFECT_BALANCE) ||
		    (of_has(o_ptr->flags_obj, OF_PERFECT_BALANCE) && full));

    int brand[MAX_P_BRAND], slay[MAX_P_SLAY];

    /* Extract the slays and brands */
    for (i = 0; i < MAX_P_SLAY; i++)
	slay[i] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + i) || full)
	    ? o_ptr->multiple_slay[i] : MULTIPLE_BASE;
    for (i = 0; i < MAX_P_BRAND; i++)
	brand[i] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + i) || full)
	    ? o_ptr->multiple_brand[i] : MULTIPLE_BASE;

    /* Hack -- paladins (and priests) cannot take advantage of temporary
     * elemental brands to rescue their lousy shooting skill. Missile
     * weapons are "kind of" edged, right? */
    if (!player_has(PF_BLESS_WEAPON) || (o_ptr->tval > TV_BOLT)) {
	if (p_ptr->special_attack & (ATTACK_ACID))
	    brand[P_BRAND_ACID] = MAX(brand[P_BRAND_ACID], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_ELEC))
	    brand[P_BRAND_ELEC] = MAX(brand[P_BRAND_ELEC], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_FIRE))
	    brand[P_BRAND_FIRE] = MAX(brand[P_BRAND_FIRE], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_COLD))
	    brand[P_BRAND_COLD] = MAX(brand[P_BRAND_COLD], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_POIS))
	    brand[P_BRAND_POIS] = MAX(brand[P_BRAND_POIS], BRAND_BOOST_NORMAL);
    }
    if (p_ptr->special_attack & (ATTACK_HOLY))
	slay[P_SLAY_EVIL] = MAX(slay[P_SLAY_EVIL], 15);

    if (o_ptr->tval > TV_BOLT) {
	if (terse)
	    textblock_append(tb, "Av. throwing dam. ");
	else
	    textblock_append(tb, "Throwing it you would do an average damage of ");
	for (i = 0; i < MAX_P_SLAY; i++)
	    if (slay[i] > MULTIPLE_BASE)
		output_ammo_dam(tb, &p_ptr->state, o_ptr, slay[i], slayee[i], 
				&first, perfect, mode);

	for (i = 0; i < MAX_P_BRAND; i++)
	    if (brand[i] > MULTIPLE_BASE) {
		char buf[40];

		strnfmt(buf, sizeof(buf), "non %s resistant creatures",
			brandee[i]);
		output_ammo_dam(tb, &p_ptr->state, o_ptr, brand[i], buf, 
				&first, perfect, mode);
	    }

	output_ammo_dam(tb, &p_ptr->state, o_ptr, MULTIPLE_BASE, 
			(first) ? "all monsters" : "other monsters", &first,
			perfect, mode);
	textblock_append(tb, ". ");

	return TRUE;
    } else if (p_ptr->state.ammo_tval == o_ptr->tval) {
	/* Check launcher for additional brands (slays) */
	i_ptr = &p_ptr->inventory[INVEN_BOW];

	/* If wielding a launcher - sanity check */
	if (i_ptr->k_idx) {
	    /* Pick up any brands (and slays!) */
	    for (i = 0; i < MAX_P_SLAY; i++)
		slay[i] =
		    MAX(slay[i], (if_has(i_ptr->id_other, OBJECT_ID_BASE_SLAY + i) || full)
			? i_ptr->multiple_slay[i] : MULTIPLE_BASE);
	    for (i = 0; i < MAX_P_BRAND; i++)
		brand[i] =
		    MAX(brand[i], (if_has(i_ptr->id_other, OBJECT_ID_BASE_BRAND + i) || full)
			? i_ptr->multiple_brand[i] : MULTIPLE_BASE);
	}

	if (terse)
	    textblock_append(tb, "Av shooting dam. ");
	else
	    textblock_append(tb, "\nUsing it with your current launcher you would do an average damage per shot of ");

	for (i = 0; i < MAX_P_SLAY; i++)
	    if (slay[i] > MULTIPLE_BASE)
		output_ammo_dam(tb, &p_ptr->state, o_ptr, slay[i], slayee[i], 
				&first, perfect, mode);

	for (i = 0; i < MAX_P_BRAND; i++)
	    if (brand[i] > MULTIPLE_BASE) {
		char buf[40];

		strnfmt(buf, sizeof(buf), "non %s resistant creatures",
			brandee[i]);
		output_ammo_dam(tb, &p_ptr->state, o_ptr, brand[i], buf, 
				&first, perfect, mode);
	    }

	output_ammo_dam(tb, &p_ptr->state, o_ptr, MULTIPLE_BASE,
			(first) ? "all monsters" : "other monsters", &first,
			perfect, mode);
	textblock_append(tb, ". ");

	return TRUE;
    } else if (!terse){
	textblock_append(tb, "\nYou cannot use this missile with your current launcher. ");

	return TRUE;

    }
    return FALSE;

}

static bool describe_combat(textblock *tb, const object_type *o_ptr,
		oinfo_detail_t mode)
{
    bool combat = FALSE;

    /* Print melee damage info */
    if ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)
	|| (o_ptr->tval == TV_HAFTED) || (o_ptr->tval == TV_DIGGING))
	combat = describe_weapon_damage(tb, o_ptr, mode);

    /* Print range damage info */
    if (is_missile(o_ptr) || of_has(o_ptr->flags_obj, OF_THROWING))
	combat = describe_ammo_damage(tb, o_ptr, mode) || combat;

    return combat;
}

/*
 * Describe objects that can be used for digging.
 */
static bool describe_digger(textblock *tb, const object_type *o_ptr,
		oinfo_detail_t mode)
{
    player_state st;

    object_type inven[INVEN_TOTAL];

    int sl = wield_slot(o_ptr);
    int i;

    int chances[4]; /* These are out of 1600 */
    static const char *names[4] = { "rubble", "magma veins", "quartz veins", "granite" };

    /* abort if we are a dummy object */
    if (mode & OINFO_DUMMY) return FALSE;

    if (sl < 0 || (sl != INVEN_WIELD && 
		   (o_ptr->bonus_other[P_BONUS_TUNNEL] == 0)))
	return FALSE;

    memcpy(inven, p_ptr->inventory, INVEN_TOTAL * sizeof(object_type));

    /*
     * Hack -- if we examine a ring that is worn on the right finger,
     * we shouldn't put a copy of it on the left finger before calculating
     * digging skills.
     */
    if (o_ptr != &p_ptr->inventory[INVEN_RIGHT])
	inven[sl] = *o_ptr;

    st.shield_on_back = FALSE;
    calc_bonuses(inven, &st, TRUE);

    chances[0] = st.skills[SKILL_DIGGING] * 8;
    chances[1] = (st.skills[SKILL_DIGGING] - 10) * 4;
    chances[2] = (st.skills[SKILL_DIGGING] - 20) * 2;
    chances[3] = (st.skills[SKILL_DIGGING] - 40) * 1;

    for (i = 0; i < 4; i++)
    {
	int chance = MAX(0, MIN(1600, chances[i]));
	int decis = chance ? (16000 / chance) : 0;

	if (i == 0 && chance > 0) {
	    if (sl == INVEN_WIELD)
		textblock_append(tb, "Clears ");
	    else
		textblock_append(tb, "With this item, your current weapon clears ");
	}

	if (i == 3 || (i != 0 && chance == 0))
	    textblock_append(tb, "and ");

	if (chance == 0) {
	    textblock_append_c(tb, TERM_L_RED, "doesn't affect ");
	    textblock_append(tb, "%s.\n", names[i]);
	    break;
	}

	textblock_append(tb, "%s in ", names[i]);

	if (chance == 1600) {
	    textblock_append_c(tb, TERM_L_GREEN, "1 ");
	} else if (decis < 100) {
	    textblock_append_c(tb, TERM_GREEN, "%d.%d ", decis/10, decis%10);
	} else {
	    textblock_append_c(tb, (decis < 1000) ? TERM_YELLOW : TERM_RED,
			       "%d ", (decis+5)/10);
	}

	textblock_append(tb, "turn%s%s", decis == 10 ? "" : "s",
			 (i == 3) ? ".\n" : ", ");
    }

    return TRUE;
}


static bool describe_food(textblock *tb, const object_type *o_ptr,
		bool subjective, bool full)
{
    /* Describe boring bits */
    if ((o_ptr->tval == TV_FOOD || o_ptr->tval == TV_POTION) &&
	o_ptr->pval)
    {
	/* Sometimes adjust for player speed */
	int multiplier = extract_energy[p_ptr->state.pspeed];
	if (!subjective) multiplier = 10;

	if (object_known_p(o_ptr) || full) {
	    textblock_append(tb, "Nourishes for around ");
	    textblock_append_c(tb, TERM_L_GREEN, "%d", (o_ptr->pval / 2) *
			       multiplier / 10);
	    textblock_append(tb, " turns.\n");
	} else {
	    textblock_append(tb, "Provides some nourishment.\n");
	}

	return TRUE;
    }

    return FALSE;
}


/*
 * Describe things that look like lights.
 */
static bool describe_light(textblock *tb, const object_type *o_ptr, bool terse)
{
    int rad = 0;

    bool artifact = artifact_p(o_ptr);
    bool is_light = (o_ptr->tval == TV_LIGHT) ? TRUE : FALSE;

    if (o_ptr->tval != TV_LIGHT && !of_has(o_ptr->flags_obj, OF_LIGHT))
	return FALSE;

    /* Work out radius */
    if (artifact && is_light)
	rad = 3;
    else if (is_light)
	rad = 2;
    if (of_has(o_ptr->flags_obj, OF_LIGHT))
	rad++;

    /* Describe here */
    if (!terse) textblock_append(tb, "It provides radius ");
    else textblock_append(tb, "Radius ");
    textblock_append_c(tb, TERM_L_GREEN, format("%d", rad));
    if (artifact) textblock_append(tb, " light forever.");
    else textblock_append(tb, " light.");

    if (!terse && is_light && !artifact)
    {
	const char *name = (o_ptr->sval == SV_LIGHT_TORCH) ? "torches" : "lanterns";
	int turns = (o_ptr->sval == SV_LIGHT_TORCH) ? FUEL_TORCH : FUEL_LAMP;

	textblock_append(tb, "  Refills other %s up to %d turns of fuel. ", name, turns);
    }

    textblock_append(tb, "\n");

    return TRUE;
}



/*
 * Describe an object's effect, if any.
 */
static bool describe_effect(textblock *tb, const object_type *o_ptr, 
			    oinfo_detail_t mode)
{
    const object_kind *k_ptr = &k_info[o_ptr->k_idx];
    const char *desc;
    random_value timeout = {0, 0, 0, 0};
    bool full = mode & OINFO_FULL;
    bool subjective = mode & OINFO_SUBJ;
    bool terse = mode & OINFO_TERSE;

    int effect = 0, fail;

    if (wearable_p(o_ptr)) {
	/* Wearable + effect <=> activates */
	if ((o_ptr->ident & IDENT_WORN) || full) {
	    effect = o_ptr->effect;
	    timeout = o_ptr->time;
	}
	else if (object_effect(o_ptr))
	{
	    textblock_append(tb, "It can be activated.\n");
	    return TRUE;
	}
    }
    else {
	/* Sometimes only print activation info */
	if (terse) return FALSE;

	if ((object_aware_p(o_ptr) && kf_has(k_ptr->flags_kind, KF_EASY_KNOW)) 
	    || full) {
	    effect = o_ptr->effect;
	    timeout = o_ptr->time;
	}
	else if (object_effect(o_ptr)) {
	    if (effect_aim(k_ptr->effect))
		textblock_append(tb, "It can be aimed.\n");
	    else if (o_ptr->tval == TV_FOOD)
		textblock_append(tb, "It can be eaten.\n");
	    else if (o_ptr->tval == TV_POTION)
		textblock_append(tb, "It can be drunk.\n");
	    else if (o_ptr->tval == TV_SCROLL)
		textblock_append(tb, "It can be read.\n");
	    else textblock_append(tb, "It can be activated.\n");

	    return TRUE;
	}
    }

    /* Forget it without an effect */
    if (!effect) return FALSE;

    /* Obtain the description */
    desc = effect_desc(effect);
    if (!desc) return FALSE;

    if (effect_aim(effect))
	textblock_append(tb, "When aimed, it ");
    else if (o_ptr->tval == TV_FOOD)
	textblock_append(tb, "When eaten, it ");
    else if (o_ptr->tval == TV_POTION)
	textblock_append(tb, "When drunk, it ");
    else if (o_ptr->tval == TV_SCROLL)
	textblock_append(tb, "When read, it ");
    else
	textblock_append(tb, "When activated, it ");

    /* Print a colourised description */
    do
    {
	if (isdigit((unsigned char) *desc))
	    textblock_append_c(tb, TERM_L_GREEN, "%c", *desc);
	else
	    textblock_append(tb, "%c", *desc);
    } while (*desc++);

    textblock_append(tb, ".\n");

    if (randcalc(timeout, 0, MAXIMISE) > 0)
    {
	int min_time, max_time;

	/* Sometimes adjust for player speed */
	int multiplier = extract_energy[p_ptr->state.pspeed];
	if (!subjective) multiplier = 10;

	textblock_append(tb, "Takes ");

	/* Correct for player speed */
	min_time = randcalc(timeout, 0, MINIMISE) * multiplier / 10;
	max_time = randcalc(timeout, 0, MAXIMISE) * multiplier / 10;

	textblock_append_c(tb, TERM_L_GREEN, "%d", min_time);

	if (min_time != max_time)
	{
	    textblock_append(tb, " to ");
	    textblock_append_c(tb, TERM_L_GREEN, "%d", max_time);
	}

	textblock_append(tb, " turns to recharge");
	if (subjective && p_ptr->state.pspeed != 110)
	    textblock_append(tb, " at your current speed");

	textblock_append(tb, ".\n");
    }

    if (!subjective || o_ptr->tval == TV_FOOD || o_ptr->tval == TV_POTION ||
	o_ptr->tval == TV_SCROLL)
    {
	return TRUE;
    }
    else
    {
	fail = get_use_device_chance(o_ptr);
	textblock_append(tb, "Your chance of success is %d.%d%%\n", (1000 - fail) /
			 10, (1000 - fail) % 10);
    }

    return TRUE;
}


bool describe_origin(textblock *tb, const object_type *o_ptr)
{
    char origin_text[80];

    /* Format location of origin */
    if (stage_map[o_ptr->origin_stage][DEPTH])
	strnfmt(origin_text, sizeof(origin_text), "%s Level %d",
		locality_name[stage_map[o_ptr->origin_stage][LOCALITY]], 
		stage_map[o_ptr->origin_stage][DEPTH]);
    else
	strnfmt(origin_text, sizeof(origin_text), "%s %s",
		locality_name[stage_map[o_ptr->origin_stage][LOCALITY]], 
		"Town");

    switch (o_ptr->origin)
    {
    case ORIGIN_NONE:
    case ORIGIN_MIXED:
	return FALSE;

    case ORIGIN_BIRTH:
	textblock_append(tb, "An inheritance from your family.\n");
	break;

    case ORIGIN_STORE:
	textblock_append(tb, "Bought from a store in %s.\n", origin_text);
	break;

    case ORIGIN_FLOOR:
	textblock_append(tb, "Found lying on the ground in %s.\n", origin_text);
	break;

    case ORIGIN_DROP:
    {
	const char *name = r_info[o_ptr->origin_xtra].name;

	textblock_append(tb, "Dropped by ");

	if (rf_has(r_info[o_ptr->origin_xtra].flags, RF_UNIQUE) && 
	    !rf_has(r_info[o_ptr->origin_xtra].flags, RF_PLAYER_GHOST))
	    textblock_append(tb, "%s", name);
	else
	    textblock_append(tb, "%s%s", is_a_vowel(name[0]) ? "an " : "a ", 
			     name);

	textblock_append(tb, " in %s.\n", origin_text);
	break;
    }

    case ORIGIN_DROP_UNKNOWN:
	textblock_append(tb, "Dropped by an unknown monster in %s.\n",
			 origin_text);
	break;

    case ORIGIN_ACQUIRE:
	textblock_append(tb, "Conjured forth by magic in %s.\n", origin_text);
	break;

    case ORIGIN_CHEAT:
	textblock_append(tb, "Created by debug option.\n");
	break;

    case ORIGIN_CHEST:
	//if (o_ptr->origin_xtra)
	if (0) /* Add in when player ghost issues are fixed */
	{
	    const char *name = r_info[o_ptr->origin_xtra].name;

	    textblock_append(tb, "Found in a chest dropped by ");

	    if (rf_has(r_info[o_ptr->origin_xtra].flags, RF_UNIQUE) &&
		!rf_has(r_info[o_ptr->origin_xtra].flags, RF_PLAYER_GHOST))
		textblock_append(tb, "%s", name);
	    else
		textblock_append(tb, "%s%s", is_a_vowel(name[0]) ? "an " : "a ",
 				 name);

	    textblock_append(tb, " in %s.\n", origin_text);
	    break;
	}
	textblock_append(tb, "Found in a chest from %s.\n",
			 origin_text);
	break;

    case ORIGIN_RUBBLE:
	textblock_append(tb, "Found under some rubble in %s.\n",
			 origin_text);
	break;

    case ORIGIN_VAULT:
	textblock_append(tb, "Found in a vault in %s.\n",
			 origin_text);
	break;

    case ORIGIN_CHAOS:
	textblock_append(tb, "Created by the forces of chaos in %s.\n");
	break;

    }

    return TRUE;
}

static void describe_flavor_text(textblock *tb, const object_type *o_ptr)
{
    /* Display the known artifact description */
    if (o_ptr->name1 &&	object_known_p(o_ptr) && a_info[o_ptr->name1].text)
    {
	textblock_append(tb, "%s\n\n", a_info[o_ptr->name1].text);
    }

    /* Display the known object description */
    else if (object_aware_p(o_ptr) || object_known_p(o_ptr))
    {
	bool did_desc = FALSE;

	if (k_info[o_ptr->k_idx].text)
	{
	    textblock_append(tb, "%s", k_info[o_ptr->k_idx].text);
	    did_desc = TRUE;
	}

	/* Display an additional ego-item description */
	if (has_ego_properties(o_ptr) && e_info[o_ptr->name2].text)
	{
	    if (did_desc) textblock_append(tb, "  ");
	    textblock_append(tb, "%s\n\n", e_info[o_ptr->name2].text);
	}
	else if (did_desc)
	{
	    textblock_append(tb, "\n\n");
	}
    }
}


bool describe_ego(textblock *tb, const object_type *o_ptr)
{
    struct ego_item *ego = &e_info[o_ptr->name2];
    int num = 0, i;

    /* Count ego flags */
    for (i = 0; i < 7; i++) {
	if (kf_has(ego->flags_kind, KF_RAND_RES_NEG + i))
	    num++;
    }

    if (has_ego_properties(o_ptr) && num)
    {
	const char *xtra[] = { "vulnerability", "pair of small resistances",
			       "resistance", "high resistance", "sustain", 
			       "ability", "curse"
	};
	const char *punct[] = { ", ", ".", " and " };
	textblock_append(tb, "It provides ");
	for (i = 0; i < 7; i++) {
	    if (kf_has(ego->flags_kind, KF_RAND_RES_NEG + i)) {
		textblock_append(tb, "at least one random %s%s", xtra[i], 
				 (num > 2) ? punct[0] : punct[num]);
		num--;
	    }
	}

	return TRUE;
    }

    return FALSE;
}


bool describe_set(textblock *tb, const object_type *o_ptr, 
			    oinfo_detail_t mode)
{
    bool full = mode & OINFO_FULL;
    
    if (!full && !object_known_p(o_ptr)) return FALSE;
    if (!o_ptr->name1) return FALSE;
    else {
	artifact_type *a_ptr = &a_info[o_ptr->name1];
	
	/* Is it a set item? */
	if (a_ptr->set_no) {
	    set_type *set_ptr = &set_info[a_ptr->set_no];
	    
	    /* Description */
	    textblock_append(tb, "\n");
	    textblock_append_c(tb, TERM_BLUE, set_ptr->text);
	    
	    /* End sentence */
	    textblock_append_c(tb, TERM_BLUE, ".");
	}
	else return FALSE;
    }
    
    return TRUE;
}


/*
 * Output object information
 */
static textblock *object_info_out(const object_type *o_ptr, oinfo_detail_t mode)
{
    bool full = mode & OINFO_FULL;
    bool terse = mode & OINFO_TERSE;
    bool subjective = mode & OINFO_SUBJ;
    bool dummy = mode & OINFO_DUMMY;
    bool ego = mode & OINFO_EGO;

    textblock *tb = textblock_new();

    if (subjective) (void) describe_origin(tb, o_ptr);
    if (!terse) (void) describe_flavor_text(tb, o_ptr);

    (void) describe_set(tb, o_ptr, mode);
    (void) describe_stats(tb, o_ptr, mode);
    (void) describe_bonus(tb, o_ptr, mode);
    (void) describe_slays(tb, o_ptr, mode);
    (void) describe_brands(tb, o_ptr, mode);
    (void) describe_immune(tb, o_ptr, mode);
    (void) describe_sustains(tb, o_ptr, mode);
    (void) describe_misc_magic(tb, o_ptr, mode);
    if (ego) (void) describe_ego(tb, o_ptr);
    (void) describe_ignores(tb, o_ptr, mode);
    (void) describe_curses(tb, o_ptr, mode);
    (void) describe_effect(tb, o_ptr, mode);
	
    if (subjective && describe_combat(tb, o_ptr, mode))
    {
	textblock_append(tb, "\n");
    }

    (void) describe_food(tb, o_ptr, subjective, full);
    (void) describe_light(tb, o_ptr, terse);
    (void) describe_digger(tb, o_ptr, mode);

    if (!terse && !dummy) {
	textblock_append(tb, "\n");
	textblock_append(tb, obj_class_info[o_ptr->tval]);
	textblock_append(tb, "\n");
    }
    return tb;
}


/**
 * Provide information on an item, including how it would affect the current
 * player's state.
 *
 * mode OINFO_FULL should be set if actual player knowledge should be ignored
 * in favour of full knowledge.
 *
 * returns TRUE if anything is printed.
 */
textblock *object_info(const object_type *o_ptr, oinfo_detail_t mode)
{
	mode |= OINFO_SUBJ;
	return object_info_out(o_ptr, mode);
}

/**
 * Provide information on an ego-item type
 */
textblock *object_info_ego(struct ego_item *ego)
{
	object_kind *kind = NULL;
	object_type obj = { 0 };
	int i;

	for (i = 0; i < z_info->k_max; i++) {
		kind = &k_info[i];
		if (!kind->name)
			continue;
		if (kind->tval == ego->tval[0])
			break;
	}

	obj.kind = kind;
	obj.tval = kind->tval;
	obj.sval = kind->sval;
	obj.name2 = ego->eidx;
	of_union(obj.flags_obj, ego->flags_obj);
	cf_union(obj.flags_curse, ego->flags_curse);
	for (i = 0; i < MAX_P_RES; i++) 
	    obj.percent_res[i] = ego->percent_res[i];
	for (i = 0; i < A_MAX; i++) 
	    obj.bonus_stat[i] = ego->bonus_stat[i];
	for (i = 0; i < MAX_P_BONUS; i++) 
	    obj.bonus_other[i] = ego->bonus_other[i];
	for (i = 0; i < MAX_P_SLAY; i++)
	    obj.multiple_slay[i] = ego->multiple_slay[i];
	for (i = 0; i < MAX_P_BRAND; i++)
	    obj.multiple_brand[i] = ego->multiple_brand[i];

	return object_info_out(&obj, OINFO_FULL | OINFO_EGO | OINFO_DUMMY);
}



/**
 * Provide information on an item suitable for writing to the character dump - keep it brief.
 */
void object_info_chardump(const object_type *o_ptr, char_attr_line **line, 
			  int *current_line, int indent, int wrap)
{
	textblock *tb = object_info_out(o_ptr, OINFO_TERSE | OINFO_SUBJ);
	textblock_dump(tb, line, current_line, indent, wrap);
	textblock_free(tb);
}


/**
 * Provide spoiler information on an item.
 *
 * Practically, this means that we should not print anything which relies upon
 * the player's current state, since that is not suitable for spoiler material.
 */
void object_info_spoil(ang_file *f, const object_type *o_ptr, int wrap)
{
	textblock *tb = object_info_out(o_ptr, OINFO_FULL);
	textblock_to_file(tb, f, 0, wrap);
	textblock_free(tb);
}
