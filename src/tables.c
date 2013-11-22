/** \file tables.c 
    \brief Various standard tables

 * Mapping of directions, what classes are legal for what races.  Stat 
 * effects, blows per round, store owners, speed-to-energy, base exp level 
 * cost, player sexes, races, classes, spell table (for each class), spell 
 * names by index, conversion of +to_d to Deadliness, traps on chests, 
 * class names, color names, stat abbrevs, window flags.  Druid blows.  
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

/**
 * Global array for looping through the "keypad directions"
 */
s16b ddd[9] =
  { 2, 8, 6, 4, 3, 1, 9, 7, 5 };

/**
 * Global arrays for converting "keypad direction" into offsets
 */
s16b ddx[10] =
  {  0, -1,  0,  1, -1,  0,  1, -1,  0,  1 };

/**
 * Global arrays for converting "keypad direction" into offsets
 */
s16b ddy[10] =
  {  0,  1,  1,  1,  0,  0,  0, -1, -1, -1 };

/**
 * Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
s16b ddx_ddd[9] =
  {  0,  0,  1, -1,  1, -1,  1, -1,  0 };

/**
 * Global arrays for optimizing "ddx[ddd[i]]" and "ddy[ddd[i]]"
 */
s16b ddy_ddd[9] =
  {  1, -1,  0,  0,  1,  1, -1, -1,  0 };

/**
 * Global array for converting numbers to uppercase hecidecimal digit
 * This array can also be used to convert a number to an octal digit
 */
char hexsym[16] =
  {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
  };


/**
 * Stat Table (INT/WIS) -- Minimum failure rate (percentage)
 */
byte adj_mag_fail[STAT_RANGE] =
  {
    99	/* 3 */,
    99	/* 4 */,
    99	/* 5 */,
    99	/* 6 */,
    50	/* 7 */,
    40	/* 8 */,
    30	/* 9 */,
    20	/* 10 */,
    15	/* 11 */,
    12	/* 12 */,
    11	/* 13 */,
    10	/* 14 */,
    9	/* 15 */,
    8	/* 16 */,
    7	/* 17 */,
    6	/* 18/00-18/09 */,
    6	/* 18/10-18/19 */,
    5	/* 18/20-18/29 */,
    5	/* 18/30-18/39 */,
    5	/* 18/40-18/49 */,
    4	/* 18/50-18/59 */,
    4	/* 18/60-18/69 */,
    4	/* 18/70-18/79 */,
    4	/* 18/80-18/89 */,
    3	/* 18/90-18/99 */,
    3	/* 18/100-18/109 */,
    2	/* 18/110-18/119 */,
    2	/* 18/120-18/129 */,
    2	/* 18/130-18/139 */,
    2	/* 18/140-18/149 */,
    1	/* 18/150-18/159 */,
    1	/* 18/160-18/169 */,
    1	/* 18/170-18/179 */,
    1	/* 18/180-18/189 */,
    1	/* 18/190-18/199 */,
    0	/* 18/200-18/209 */,
    0	/* 18/210-18/219 */,
    0	/* 18/220+ */
  };


/**
 * Stat Table (INT/WIS) -- Reduction of failure rate
 */
byte adj_mag_stat[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    1	/* 8 */,
    1	/* 9 */,
    1	/* 10 */,
    1	/* 11 */,
    1	/* 12 */,
    1	/* 13 */,
    1	/* 14 */,
    2	/* 15 */,
    2	/* 16 */,
    2	/* 17 */,
    3	/* 18/00-18/09 */,
    3	/* 18/10-18/19 */,
    3	/* 18/20-18/29 */,
    3	/* 18/30-18/39 */,
    3	/* 18/40-18/49 */,
    4	/* 18/50-18/59 */,
    4	/* 18/60-18/69 */,
    5	/* 18/70-18/79 */,
    6	/* 18/80-18/89 */,
    7	/* 18/90-18/99 */,
    8	/* 18/100-18/109 */,
    9	/* 18/110-18/119 */,
    10	/* 18/120-18/129 */,
    11	/* 18/130-18/139 */,
    12	/* 18/140-18/149 */,
    13	/* 18/150-18/159 */,
    14	/* 18/160-18/169 */,
    15	/* 18/170-18/179 */,
    16	/* 18/180-18/189 */,
    17	/* 18/190-18/199 */,
    18	/* 18/200-18/209 */,
    19	/* 18/210-18/219 */,
    20	/* 18/220+ */
  };


/**
 * Stat Table (CHR) -- payment percentage of normal.  Effect of CHR 
 * increased because of changes elsewhere. -LM-
 */
byte adj_chr_gold[STAT_RANGE] =
  {
    150	/* 3 */,
    143	/* 4 */,
    138	/* 5 */,
    132	/* 6 */,
    127	/* 7 */,
    123	/* 8 */,
    120	/* 9 */,
    117	/* 10 */,
    114	/* 11 */,
    112	/* 12 */,
    110	/* 13 */,
    108	/* 14 */,
    106	/* 15 */,
    104	/* 16 */,
    102	/* 17 */,
    100	/* 18/00-18/09 */,
    99	/* 18/10-18/19 */,
    98	/* 18/20-18/29 */,
    97	/* 18/30-18/39 */,
    96	/* 18/40-18/49 */,
    95	/* 18/50-18/59 */,
    94	/* 18/60-18/69 */,
    93	/* 18/70-18/79 */,
    92	/* 18/80-18/89 */,
    91	/* 18/90-18/99 */,
    90	/* 18/100-18/109 */,
    89	/* 18/110-18/119 */,
    88	/* 18/120-18/129 */,
    87	/* 18/130-18/139 */,
    86	/* 18/140-18/149 */,
    85	/* 18/150-18/159 */,
    84	/* 18/160-18/169 */,
    83	/* 18/170-18/179 */,
    82	/* 18/180-18/189 */,
    81	/* 18/190-18/199 */,
    80	/* 18/200-18/209 */,
    80	/* 18/210-18/219 */,
    80	/* 18/220+ */
  };


/**
 * Stat Table (DEX) -- chance of avoiding "theft" and "falling".  Modified to 
 * make both theft and security from theft less of a sure thing by LM.
 */
byte adj_dex_safe[STAT_RANGE] =
  {
    5	/* 3 */,
    5	/* 4 */,
    6	/* 5 */,
    6	/* 6 */,
    7	/* 7 */,
    7	/* 8 */,
    8	/* 9 */,
    8	/* 10 */,
    9	/* 11 */,
    10	/* 12 */,
    12	/* 13 */,
    14	/* 14 */,
    16	/* 15 */,
    18	/* 16 */,
    20	/* 17 */,
    22	/* 18/00-18/09 */,
    25	/* 18/10-18/19 */,
    27	/* 18/20-18/29 */,
    30	/* 18/30-18/39 */,
    32	/* 18/40-18/49 */,
    35	/* 18/50-18/59 */,
    40	/* 18/60-18/69 */,
    45	/* 18/70-18/79 */,
    50	/* 18/80-18/89 */,
    55	/* 18/90-18/99 */,
    60	/* 18/100-18/109 */,
    65	/* 18/110-18/119 */,
    70	/* 18/120-18/129 */,
    75	/* 18/130-18/139 */,
    80	/* 18/140-18/149 */,
    82	/* 18/150-18/159 */,
    85	/* 18/160-18/169 */,
    87	/* 18/170-18/179 */,
    90	/* 18/180-18/189 */,
    92	/* 18/190-18/199 */,
    95	/* 18/200-18/209 */,
    95	/* 18/210-18/219 */,
    95	/* 18/220+ */
  };


/**
 * Stat Table (CON) -- base regeneration rate
 */
byte adj_con_fix[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    0	/* 8 */,
    0	/* 9 */,
    0	/* 10 */,
    0	/* 11 */,
    0	/* 12 */,
    0	/* 13 */,
    1	/* 14 */,
    1	/* 15 */,
    1	/* 16 */,
    1	/* 17 */,
    2	/* 18/00-18/09 */,
    2	/* 18/10-18/19 */,
    2	/* 18/20-18/29 */,
    2	/* 18/30-18/39 */,
    2	/* 18/40-18/49 */,
    3	/* 18/50-18/59 */,
    3	/* 18/60-18/69 */,
    3	/* 18/70-18/79 */,
    3	/* 18/80-18/89 */,
    3	/* 18/90-18/99 */,
    4	/* 18/100-18/109 */,
    4	/* 18/110-18/119 */,
    5	/* 18/120-18/129 */,
    6	/* 18/130-18/139 */,
    6	/* 18/140-18/149 */,
    7	/* 18/150-18/159 */,
    7	/* 18/160-18/169 */,
    8	/* 18/170-18/179 */,
    8	/* 18/180-18/189 */,
    8	/* 18/190-18/199 */,
    9	/* 18/200-18/209 */,
    9	/* 18/210-18/219 */,
    9	/* 18/220+ */
  };


/**
 * This table allows quick conversion from "speed" to "energy"
 * The basic function WAS ((S>=110) ? (S-110) : (100 / (120-S)))
 * Note that table access is *much* quicker than computation.
 *
 * Note that the table has been changed at high speeds.  From
 * "Slow (-40)" to "Fast (+30)" is pretty much unchanged, but
 * at speeds above "Fast (+30)", one approaches an asymptotic
 * effective limit of 50 energy per turn.  This means that it
 * is relatively easy to reach "Fast (+30)" and get about 40
 * energy per turn, but then speed becomes very "expensive",
 * and you must get all the way to "Fast (+50)" to reach the
 * point of getting 45 energy per turn.  After that point,
 * furthur increases in speed are more or less pointless,
 * except to balance out heavy inventory.
 *
 * Note that currently the fastest monster is "Fast (+30)".
 *
 * It should be possible to lower the energy threshhold from
 * 100 units to 50 units, though this may interact badly with
 * the (compiled out) small random energy boost code.  It may
 * also tend to cause more "clumping" at high speeds.
 */
byte extract_energy[200] =
  {
    /* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* Slow */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* S-50 */     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    /* S-40 */     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    /* S-30 */     2,  2,  2,  2,  2,  2,  2,  3,  3,  3,
    /* S-20 */     3,  3,  3,  3,  3,  4,  4,  4,  4,  4,
    /* S-10 */     5,  5,  5,  5,  6,  6,  7,  7,  8,  9,
    /* Norm */    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    /* F+10 */    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    /* F+20 */    30, 31, 32, 33, 34, 35, 36, 36, 37, 37,
    /* F+30 */    38, 38, 39, 39, 40, 40, 40, 41, 41, 41,
    /* F+40 */    42, 42, 42, 43, 43, 43, 44, 44, 44, 44,
    /* F+50 */    45, 45, 45, 45, 45, 46, 46, 46, 46, 46,
    /* F+60 */    47, 47, 47, 47, 47, 48, 48, 48, 48, 48,
    /* F+70 */    49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
    /* Fast */    49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
  };



/**
 * Experience levels
 */
s32b player_exp[PY_MAX_LEVEL] =
  {
    10,		/* 1 */
    25,
    45,
    70,
    105,
    150,
    210,
    290,
    390,
    530,		/* 10 */
    720,
    960,
    1270,
    1700,
    2250,
    3000,
    4000,
    5300,
    6900,
    9100,		/* 20 */
    12100,
    16000,
    21000,
    27500,
    36500,
    48500,
    64000,
    84500L,
    111000L,
    144000L,	/* 30 */
    183000L,
    230000L,
    285000L,
    350000L,
    430000L,
    525000L,
    640000L,
    780000L,
    945000L,
    1140000L,	/* 40 */
    1380000L,
    1660000L,
    2000000L,
    2400000L,
    2900000L,
    3500000L,
    4200000L,
    5000000L,
    6000000L,
    7200000L	/* 50 */
  };


/**
 * Player Sexes
 *
 *	Title,
 *	Winner
 */
player_sex sex_info[MAX_SEXES] =
  {
    {
      "Female",
      "Queen"
    },
    
    {
      "Male",
      "King"
    }
  };



/**
 * Conversion of plusses to Deadliness to a percentage added to damage.  
 * Much of this table is not intended ever to be used, and is included 
 * only to handle possible inflation elsewhere. -LM-
 */
byte deadliness_conversion[151] = 
  {
    0, 
    5,  10,  14,  18,  22,  26,  30,  33,  36,  39, 
    42,  45,  48,  51,  54,  57,  60,  63,  66,  69, 
    72,  75,  78,  81,  84,  87,  90,  93,  96,  99, 
    102, 104, 107, 109, 112, 114, 117, 119, 122, 124, 
    127, 129, 132, 134, 137, 139, 142, 144, 147, 149, 
    152, 154, 157, 159, 162, 164, 167, 169, 172, 174, 
    176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 
    196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 
    216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 
    236, 238, 240, 242, 244, 246, 248, 250, 251, 253, 
    
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255
  };

/**
 * Each chest has a certain set of traps, determined by pval (which 
 * also controls the quality of treasure).
 * Table revised for Oangband.
 *
 * Note that traps can actually be 4 entries past the nominal "best" comment 
 */
int chest_traps[100] =
  {
    0,					/* 0 == empty */
    (CHEST_POISON),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    0,
    (CHEST_POISON),
    (CHEST_POISON),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    (CHEST_POISON),
    (CHEST_SCATTER),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_SUMMON),			/* 15 == best small wooden */
    0,
    (CHEST_LOSE_STR),
    (CHEST_SCATTER),
    (CHEST_PARALYZE),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_SUMMON),
    (CHEST_PARALYZE),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    (CHEST_EXPLODE),			/* 25 == best large wooden */
    0,
    (CHEST_E_SUMMON),
    (CHEST_POISON | CHEST_LOSE_CON),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_POISON | CHEST_SUMMON),
    (CHEST_E_SUMMON),
    (CHEST_EXPLODE),
    (CHEST_BIRD_STORM),	
    0,
    (CHEST_SUMMON),
    (CHEST_EXPLODE),
    (CHEST_E_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON), /* 40 == best small iron */
    (CHEST_BIRD_STORM),
    (CHEST_EXPLODE),
    (CHEST_BIRD_STORM),
    (CHEST_E_SUMMON),
    (CHEST_BIRD_STORM),
    0,
    (CHEST_E_SUMMON),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_STR),
    (CHEST_E_SUMMON),	/* 50 == best large iron */
    (CHEST_BIRD_STORM),
    (CHEST_E_SUMMON),
    (CHEST_H_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_POISON | CHEST_PARALYZE),
    (CHEST_BIRD_STORM),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_E_SUMMON),
    (CHEST_H_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_SCATTER),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_H_SUMMON),	/* 65 == best small steel */
    (CHEST_BIRD_STORM),
    (CHEST_POISON | CHEST_PARALYZE | CHEST_LOSE_CON),
    (CHEST_BIRD_STORM),
    (CHEST_SCATTER),
    (CHEST_H_SUMMON),
    (CHEST_POISON | CHEST_PARALYZE | CHEST_SCATTER),
    (CHEST_H_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_H_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),	/* 75 == best large steel */
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_H_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_H_SUMMON | CHEST_SCATTER),
    (CHEST_H_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_H_SUMMON),	/* 95 == best small jeweled */
    (CHEST_H_SUMMON),
    (CHEST_H_SUMMON),
    (CHEST_E_SUMMON),
    (CHEST_H_SUMMON),
    (CHEST_RUNES_OF_EVIL),
    (CHEST_POISON | CHEST_PARALYZE),
    (CHEST_H_SUMMON),
    (CHEST_RUNES_OF_EVIL),
    (CHEST_H_SUMMON),
    (CHEST_RUNES_OF_EVIL),	/* 95 == best large jeweled */
    (CHEST_RUNES_OF_EVIL),
    (CHEST_H_SUMMON),
    (CHEST_RUNES_OF_EVIL),
    (CHEST_RUNES_OF_EVIL | CHEST_EXPLODE),
  };



/**
 * Hack -- the "basic" color names (see "TERM_xxx")
 */
const char *color_names[16] =
  {
    "Dark",
    "White",
    "Slate",
    "Orange",
    "Red",
    "Green",
    "Blue",
    "Umber",
    "Light Dark",
    "Light Slate",
    "Violet",
    "Yellow",
    "Light Red",
    "Light Green",
    "Light Blue",
    "Light Umber",
  };

/*
 * The next two tables are useful for generating list of items of
 * different types.  "group_item" comes from wizard1.c, which is where
 * it was previously used.
 */

/**
 * Index into "grouper" for general item types.
 *
 * Must be synced to non-NULL entries in group_item.
 *
 * This is a little silly - the right solution is to initialize based
 * on the group_item table at load time.
 */
int new_group_index[] =
  { 0,  3,  4,  8, 11, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 36,
    -1};

/**
 * The basic items categorized by type
 */
grouper group_item[] =
  {
    { TV_SHOT,		"Ammo" },
    { TV_ARROW,		  NULL },
    { TV_BOLT,		  NULL },
    
    { TV_BOW,		"Bows" },
    
    { TV_SWORD,		"Weapons" },
    { TV_POLEARM,	  NULL },
    { TV_HAFTED,	  NULL },
    { TV_DIGGING,	  NULL },
    
    { TV_SOFT_ARMOR,	"Armour (Body)" },
    { TV_HARD_ARMOR,	  NULL },
    { TV_DRAG_ARMOR,	  NULL },
    
    { TV_CLOAK,		"Armour (Misc)" },
    { TV_SHIELD,	  NULL },
    { TV_HELM,		  NULL },
    { TV_CROWN,		  NULL },
    { TV_GLOVES,	  NULL },
    { TV_BOOTS,		  NULL },
    
    { TV_AMULET,	"Amulets" },
    { TV_RING,		"Rings" },
    { TV_SCROLL,	"Scrolls" },
    { TV_POTION,	"Potions" },
    { TV_FOOD,		"Food" },
    { TV_ROD,		"Rods" },
    { TV_WAND,		"Wands" },
    { TV_STAFF,		"Staffs" },
    
    { TV_MAGIC_BOOK,	"Books (Mage)" },
    { TV_PRAYER_BOOK,	"Books (Priest)" },
    { TV_DRUID_BOOK,	"Stones (Druid)" },
    { TV_NECRO_BOOK,	"Books (Necro)" },
    
    { TV_CHEST,		"Chests" },
    
    { TV_SPIKE,		"Various" },
    { TV_LIGHT,		  NULL },
    { TV_FLASK,		  NULL },
    { TV_JUNK,		  NULL },
    { TV_BOTTLE,	  NULL },
    { TV_SKELETON,	  NULL },
    
    { 0, "" }
  };


/**
 * Abbreviations of healthy stats
 */
const char *stat_names[A_MAX] =
  {
    "STR: ", "INT: ", "WIS: ", "DEX: ", "CON: ", "CHR: "
  };

/**
 * Abbreviations of damaged stats
 */
const char *stat_names_reduced[A_MAX] =
  {
    "Str: ", "Int: ", "Wis: ", "Dex: ", "Con: ", "Chr: "
  };


/**
 * Certain "screens" always use the main screen, including News, Birth,
 * Dungeon, Tomb-stone, High-scores, Macros, Colors, Visuals, Options.
 *
 * Later, special flags may allow sub-windows to "steal" stuff from the
 * main window, including File dump (help), File dump (artifacts, uniques),
 * Character screen, Small scale map, Previous Messages, Store screen, etc.
 */
const char *window_flag_desc[32] =
  {
    "Display inven/equip",
    "Display equip/inven",
    "Display player (basic)",
    "Display player (extra)",
    NULL,
    NULL,
    "Display messages",
    "Display overhead view",
    "Display monster recall",
    "Display object recall",
    NULL,
    "Display snap-shot",
    "Display monster list",
    "Display item list",
    "Display borg messages",
    "Display borg status",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };




/**
 * Table of Druid blows. -LM- 
 */
druid_blows d_blow[NUM_D_BLOWS] =
  {
    { "punch",		  1, 5 },
    { "kick",		  2, 4 },
    { "knee",		  1,12 },
    { "chop",		  2, 7 },
    { "uppercut",	  3, 6 },
    { "boot",		  3, 9 },
    { "bang on",	  6, 4 },
    { "slam",		  4, 9 },
    { "grapple with",	 13, 3 },
    { "hammer",		  9, 6 },
    { "head butt",	  3,24 },
    { "strangle",	  8,10 },
    { "roundhouse kick",  5,19 },
    { "assault",	 10,11 },
    { "crush",		 11,11 },
    { "double-kick",	 21, 6 },
    { "thunderclap belt", 8,19 },
    { "blizzard gouge",	 14,11 },
    { "tsunami whirl",	  7,26 },
    { "stormwind chop",	 10,22 }
  };

const char *magic_desc[REALM_MAX][MD_MAX] = 
{
    {"","","","",""},
    {"spell",        "cast",     "You failed to get the spell off!",
     "magic book", "magic book"},
    {"prayer",       "pray",     "You lost your concentration!",
     "holy book",  "holy book"},
    {"druidic lore", "use",     "You lost your concentration!",
     "stone",      "stone of lore"},
    {"ritual",       "perform", "You perform the ritual incorrectly!",
     "tome",       "tome"},
};

const char *feel_text[FEEL_MAX] =
  {
    NULL,	 /* FEEL_NONE */
    "dubious",	 /* FEEL_DUBIOUS_STRONG */
    "perilous",	 /* FEEL_PERILOUS */
    "dubious",   /* FEEL_DUBIOUS_WEAK */
    "average",	 /* FEEL_AVERAGE */
    "good",	 /* FEEL_GOOD_STRONG */
    "excellent", /* FEEL_EXCELLENT */
    "good",	 /* FEEL_GOOD_WEAK */
    "special",	 /* FEEL_SPECIAL */
  };

const grouper object_text_order [] =
{
	{TV_SWORD,		"Sword"			},
	{TV_POLEARM,		"Polearm"		},
	{TV_HAFTED,		"Hafted Weapon" },
	{TV_BOW,		"Bow"			},
	{TV_ARROW,		"Ammunition"	},
	{TV_BOLT,		NULL			},
	{TV_SHOT,		NULL			},
	{TV_SHIELD,		"Shield"		},
	{TV_CROWN,		"Crown"			},
	{TV_HELM,		"Helm"			},
	{TV_GLOVES,		"Gloves"		},
	{TV_BOOTS,		"Boots"			},
	{TV_CLOAK,		"Cloak"			},
	{TV_DRAG_ARMOR,		"Dragon Scale Mail" },
	{TV_HARD_ARMOR,		"Hard Armor"	},
	{TV_SOFT_ARMOR,		"Soft Armor"	},
	{TV_RING,		"Ring"			},
	{TV_AMULET,		"Amulet"		},
	{TV_LIGHT,		"Light"			},
	{TV_POTION,		"Potion"		},
	{TV_SCROLL,		"Scroll"		},
	{TV_WAND,		"Wand"			},
	{TV_STAFF,		"Staff"			},
	{TV_ROD,		"Rod"			},
	{TV_PRAYER_BOOK,	"Priest Book"	},
	{TV_MAGIC_BOOK,		"Magic Book"	},
        {TV_DRUID_BOOK,	        "Stone of Lore" },
	{TV_NECRO_BOOK,	        "Necromantic Tome" },
	{TV_SPIKE,		"Spike"			},
	{TV_DIGGING,		"Digger"		},
	{TV_FOOD,		"Food"			},
	{TV_FLASK,		"Flask"			},
	{TV_JUNK,		"Junk"			},
	{0,			NULL			}
};

/**
 * Monster mana used 
 */
byte mana_cost[RSF_MAX]=
  {
    1,			/* RSF_SHREIK */
    0,			/* RSF_LASH */
    0,			/* RSF_BOULDER */
    0,			/* RSF_SHOT */
    0,			/* RSF_ARROW */
    0,			/* RSF_BOLT */
    0,			/* RSF_MISSL */
    0,			/* RSF_PMISSL */
    0,			/* RSF_BRTH_ACID */
    0,			/* RSF_BRTH_ELEC */
    0,			/* RSF_BRTH_FIRE */
    0,			/* RSF_BRTH_COLD */
    0,			/* RSF_BRTH_POIS */
    0,			/* RSF_BRTH_PLAS */
    0,			/* RSF_BRTH_LIGHT */
    0,			/* RSF_BRTH_DARK */
    0,			/* RSF_BRTH_CONFU */
    0,			/* RSF_BRTH_SOUND */
    0,			/* RSF_BRTH_SHARD */
    0,			/* RSF_BRTH_INER */
    0,			/* RSF_BRTH_GRAV */
    0,			/* RSF_BRTH_FORCE */
    0,			/* RSF_BRTH_NEXUS */
    0,			/* RSF_BRTH_NETHR */
    0,			/* RSF_BRTH_CHAOS */
    0,			/* RSF_BRTH_DISEN */
    0,			/* RSF_BRTH_TIME */
    0,			/* RSF_BRTH_STORM */
    0,			/* RSF_BRTH_DFIRE */
    0,			/* RSF_BRTH_ICE */
    0,			/* RSF_ALL */
    0,			/* RSF_XXX1 */
    4,			/* RSF_BALL_ACID */
    4,			/* RSF_BALL_ELEC */
    4,			/* RSF_BALL_FIRE */
    4,			/* RSF_BALL_COLD */
    4,			/* RSF_BALL_POIS */
    5, 			/* RSF_BALL_LIGHT */
    5, 			/* RSF_BALL_DARK */
    6, 			/* RSF_BALL_CONFU */
    4, 			/* RSF_BALL_SOUND */
    4, 			/* RSF_BALL_SHARD */
    5, 			/* RSF_BALL_STORM */
    6, 			/* RSF_BALL_NETHR */
    7, 			/* RSF_BALL_CHAOS */
    5, 			/* RSF_BALL_MANA */
    0, 			/* RSF_BALL_ALL */
    0, 			/* RSF_XXX2 */
    4, 			/* RSF_BOLT_ACID */
    4, 			/* RSF_BOLT_ELEC */
    4, 			/* RSF_BOLT_FIRE */
    4, 			/* RSF_BOLT_COLD */
    4, 			/* RSF_BOLT_POIS */
    4, 			/* RSF_BOLT_PLAS */
    4, 			/* RSF_BOLT_ICE */
    5, 			/* RSF_BOLT_WATER */
    5, 			/* RSF_BOLT_NETHER */
    4, 			/* RSF_BOLT_DARK */
    0, 			/* RSF_XXX3 */
    3, 			/* RSF_BEAM_ELEC */
    6, 			/* RSF_BEAM_ICE */
    7, 			/* RSF_BEAM_NETHER */
    7, 			/* RSF_ARC_HFIRE */
    5, 			/* RSF_ARC_FORCE */
    6, 			/* RSF_HASTE */
    0, 			/* RSF_ADD_MANA */
    0, 			/* RSF_HEAL */
    3, 			/* RSF_CURE */
    1, 			/* RSF_BLINK */	
    6, 			/* RSF_TPORT */
    0, 			/* RSF_XXX4 */
    4, 			/* RSF_TELE_SELF_TO */
    4, 			/* RSF_TELE_TO */
    8, 			/* RSF_TELE_AWAY */
    8, 			/* RSF_TELE_LEVEL */
    0, 			/* RSF_XXX5 */
    1, 			/* RSF_DARKNESS */
    2, 			/* RSF_TRAPS */
    6, 			/* RSF_FORGET */
    0, 			/* RSF_DRAIN_MANA */
    5, 			/* RSF_DISPEL */ 
    0, 			/* RSF_XXX6 */ 
    3, 			/* RSF_MIND_BLAST */
    4, 			/* RSF_BRAIN_SMASH */
    4, 			/* RSF_WOUND */
    0, 			/* RSF_XXX7 */
    4, 			/* RSF_SHAPECHANGE */ 
    0, 			/* RSF_XXX8 */
    0, 			/* RSF_XXX9 */
    2, 			/* RSF_HUNGER */ 
    0, 			/* RSF_XX10 */ 
    1, 			/* RSF_SCARE */ 
    3, 			/* RSF_BLIND */ 
    4, 			/* RSF_CONF */ 
    5, 			/* RSF_SLOW */ 
    6, 			/* RSF_HOLD */ 
    12,			/* RSF_S_KIN */ /* Summon - 6 */
    0, 			/* RSF_S_XXX1 */
    0, 			/* RSF_S_XXX2 */
    10,			/* RSF_S_MONSTER */ /* Summon - 1 */
    15,		        /* RSF_S_MONSTERS */ /* Summon - 8 */
    0,			/* RSF_S_XXX3 */
    0, 			/* RSF_S_XXX4 */
    0, 			/* RSF_S_XXX5 */
    10, 		/* RSF_S_ANT */ /* Summon - 6 */
    12,			/* RSF_S_SPIDER */ /* Summon - 6 */
    14,			/* RSF_S_HOUND */ /* Summon - 6 */
    15,			/* RSF_S_ANIMAL */ /* Summon - 6 */
    0, 			/* RSF_S_XXX6 */
    0, 			/* RSF_S_XXX7 */
    15,			/* RSF_S_THIEF */ /* Summon - 6 */
    15,			/* RSF_S_SWAMP */ /* Summon - 2 */
    0, 			/* RSF_S_XXX8 */
    0, 			/* RSF_S_XXX9 */
    0, 			/* RSF_S_XX10 */
    0, 			/* RSF_S_XX11 */
    14,			/* RSF_S_DRAGON */ /* Summon - 1 */
    20,			/* RSF_S_HI_DRAGON */ /* Summon - 8 */
    0, 			/* RSF_S_XX12 */
    0, 			/* RSF_S_XX13 */
    14,			/* RSF_S_DEMON */ /* Summon - 1 / 2-3 */
    20,			/* RSF_S_HI_DEMON */ /* Summon - 8 */
    0, 			/* RSF_S_XX14 */
    0, 			/* RSF_APPROP */
    12,			/* RSF_S_UNDEAD */ /* Summon - 1 */
    20,			/* RSF_S_HI_UNDEAD */ /* Summon - 8 */
    20,			/* RSF_S_QUEST */ /* Summon - 8 */
    20 			/* RSF_S_UNIQUE */ /* Summon - 8 */
  };

/**
 * d_base:     base desirability for AI.
 * d_summ:     desriability for AI per monster level
 *                  times 0-3 based on number of clear spaces
 * d_hurt:     desirability for AI per monster spell power
 *                  times 0-3 based on damage taken
 * d_mana:     desirability for AI per monster spell power
 *                  times 0-2 based on mana shortage
 * d_esc:      desirability for AI per monster level
 *                  times 0-3 based on fear, and damage taken
 * d_tact:     desirability for AI per monster level, modified
 *                  times 0-3 based on proximity, min_range, and best_range
 * d_res:      category of 'resistability' checked by monster AI
 *                 for purposes of desirability.
 * d_range:    % of spell desirability retained for each step past 'range'
 */

byte spell_desire[RSF_MAX][D_MAX] =
  {
  /*  d_base   d_hurt   d_esc	 d_res				    */
  /*	 d_summ   d_mana  d_tact	   d_range		    */
    { 30,  0,   0,   5,	0,   0,	   0	  ,  100}, /* RSF_SHRIEK    */
    { 40,  0,   0,   5,	0,   0,	   0	  ,    0}, /* RSF_LASH	    */
    { 40,  0,   0,   5,	0,   0, LRN_ARCH  ,  100}, /* RSF_BOULDER   */
    { 40,  0,   0,   5,	0,   0, LRN_ARCH  ,  100}, /* RSF_SHOT	    */
    { 40,  0,   0,   5,	0,   0, LRN_ARCH  ,  100}, /* RSF_ARROW	    */
    { 40,  0,   0,   5,	0,   0, LRN_ARCH  ,  100}, /* RSF_BOLT	    */
    { 35,  0,   0,   5,	0,   0, LRN_ARCH  ,  100}, /* RSF_MISSL	    */
    { 40,  0,   0,   5,	0,   0, LRN_PARCH ,  100}, /* RSF_PMISSL    */
    { 65,  0,   0,   5,	0,   0, LRN_ACID  ,   90}, /* RSF_BRTH_ACID */
    { 65,  0,   0,   5,	0,   0, LRN_ELEC  ,   90}, /* RSF_BRTH_ELEC */
    { 65,  0,   0,   5,	0,   0, LRN_FIRE  ,   90}, /* RSF_BRTH_FIRE */
    { 65,  0,   0,   5,	0,   0, LRN_COLD  ,   90}, /* RSF_BRTH_COLD */
    { 65,  0,   0,   5,	0,   0, LRN_POIS  ,   90}, /* RSF_BRTH_POIS */
    { 65,  0,   0,   5,	0,   0, LRN_PLAS  ,   90}, /* RSF_BRTH_PLAS */
    { 50,  0,   0,   5,	0,   0, LRN_LIGHT  ,   90}, /* RSF_BRTH_LIGHT */
    { 50,  0,   0,   5,	0,   0, LRN_DARK  ,   90}, /* RSF_BRTH_DARK */
    { 50,  0,   0,   5,	0,   0, LRN_CONFU ,   90}, /* RSF_BRTH_CONFU*/
    { 50,  0,   0,   5,	0,   0, LRN_SOUND ,   90}, /* RSF_BRTH_SOUND*/
    { 50,  0,   0,   5,	0,   0, LRN_SHARD ,   90}, /* RSF_BRTH_SHARD*/
    { 50,  0,   0,   5,	0,   0,	   0	  ,   90}, /* RSF_BRTH_INER */
    { 50,  0,   0,   5,	0,   0, LRN_SOUND2,   90}, /* RSF_BRTH_GRAV */
    { 50,  0,   0,   5,	0,   0, LRN_SOUND2,   90}, /* RSF_BRTH_FORCE*/
    { 50,  0,   0,   5,	0,   0, LRN_NEXUS ,   90}, /* RSF_BRTH_NEXUS*/
    { 50,  0,   0,   5,	0,   0, LRN_NETHR ,   90}, /* RSF_BRTH_NETHR*/
    { 50,  0,   0,   5,	0,   0, LRN_CHAOS ,   90}, /* RSF_BRTH_CHAOS*/
    { 50,  0,   0,   5,	0,   0, LRN_DISEN ,   90}, /* RSF_BRTH_DISEN*/
    { 50,  0,   0,   5,	0,   0,	   0	  ,   90}, /* RSF_BRTH_TIME */
    { 65,  0,   0,   5,	0,   0,	LRN_STORM ,   90}, /* RSF_BRTH_STORM */
    { 65,  0,   0,   5,	0,   0,	LRN_DFIRE ,   90}, /* RSF_BRTH_DFIRE */
    { 65,  0,   0,   5,	0,   0,	LRN_ICE	  ,   90}, /* RSF_BRTH_ICE */
    { 70,  0,   0,   5,	0,   0,	LRN_ALL	  ,  100}, /* RSF_BRTH_ALL */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_XXX1 */
  /*  d_base    d_hurt  d_esc	 d_res				    */
  /*     d_summ	  d_mana  d_tact	   d_range		    */
    { 50,  0,   0,   0,	0,   0, LRN_ACID  ,  100}, /* RSF_BALL_ACID */
    { 50,  0,   0,   0,	0,   0, LRN_ELEC  ,  100}, /* RSF_BALL_ELEC */
    { 50,  0,   0,   0,	0,   0, LRN_FIRE  ,  100}, /* RSF_BALL_FIRE */
    { 50,  0,   0,   0,	0,   0, LRN_COLD  ,  100}, /* RSF_BALL_COLD */
    { 50,  0,   0,   0,	0,   0, LRN_POIS  ,  100}, /* RSF_BALL_POIS */
    { 40,  0,   0,   0,	0,   0, LRN_LIGHT  ,  100}, /* RSF_BALL_LIGHT */
    { 40,  0,   0,   0,	0,   0, LRN_DARK  ,  100}, /* RSF_BALL_DARK */
    { 40,  0,   0,   0,	0,   0, LRN_CONFU ,  100}, /* RSF_BALL_CONFU*/
    { 40,  0,   0,   0,	0,   0, LRN_SOUND ,  100}, /* RSF_BALL_SOUND*/
    { 40,  0,   0,   0,	0,   0, LRN_SHARD ,  100}, /* RSF_BALL_SHARD*/
    { 40,  0,   0,   0,	0,   0, LRN_STORM ,  100}, /* RSF_BALL_STORM*/
    { 40,  0,   0,   0,	0,   0, LRN_NETHR ,  100}, /* RSF_BALL_NETHR*/
    { 40,  0,   0,   0,	0,   0, LRN_CHAOS ,  100}, /* RSF_BALL_CHAOS*/
    { 40,  0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_BALL_MANA */
    { 50,  0,   0,   0,	0,   0,	LRN_ALL	  ,  100}, /* RSF_BALL_ALL */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_XXX2 */
    { 40,  0,   0,   0,	0,   0, LRN_ACID  ,  100}, /* RSF_BOLT_ACID */
    { 40,  0,   0,   0,	0,   0, LRN_ELEC  ,  100}, /* RSF_BOLT_ELEC */
    { 40,  0,   0,   0,	0,   0, LRN_FIRE  ,  100}, /* RSF_BOLT_FIRE */
    { 40,  0,   0,   0,	0,   0, LRN_COLD  ,  100}, /* RSF_BOLT_COLD */
    { 40,  0,   0,   0,	0,   0, LRN_POIS  ,  100}, /* RSF_BOLT_POIS */
    { 50,  0,   0,   0,	0,   0, LRN_PLAS  ,  100}, /* RSF_BOLT_PLAS */
    { 50,  0,   0,   0,	0,   0, LRN_ICE	  ,  100}, /* RSF_BOLT_ICE  */
    { 35,  0,   0,   0,	0,   0, LRN_WATER ,  100}, /* RSF_BOLT_WATER*/
    { 35,  0,   0,   0,	0,   0, LRN_NETHR ,  100}, /* RSF_BOLT_NETHR*/
    { 40,  0,   0,   0,	0,   0,	LRN_DARK  ,  100}, /* RSF_BOLT_DARK */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_XXX3	    */
    { 50,  0,   0,   0,	0,   0, LRN_ELEC  ,   90}, /* RSF_BEAM_ELEC */
    { 50,  0,   0,   0,	0,   0, LRN_ICE	  ,   90}, /* RSF_BEAM_ICE  */
    { 50,  0,   0,   0,	0,   0, LRN_NETHR ,   90}, /* RSF_BEAM_NETHR*/
    { 50,  0,   0,   0,	0,   0, LRN_FIRE  ,   95}, /* RSF_ARC_HFIRE*/
    { 40,  0,   0,   0,	0,   0,	   0	  ,   90},/* RSF_ARC_FORCE*/
  /*  d_base   d_hurt  d_esc	 d_res			                    */
  /*     d_summ	  d_mana   d_tact	       d_range		            */
    { 50,  0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_HASTE	    */
    { 15,  0,   0,   25,0,   0,	   0	      ,  100}, /* RSF_ADD_MANA      */
    { 10,  0,   20,  0,	0,   0,	   0	      ,  100}, /* RSF_HEAL	    */
    { 50,  0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_CURE	    */
    { 27,  0,   0,   0,	10,  15,   0	      ,  100}, /* RSF_BLINK	    */
    { 10,  0,   0,   0,	20,  10,   0	      ,  100}, /* RSF_TPORT	    */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XXX4	    */
    { 30,  0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_TELE_SELF_TO  */
    { 30,  0,   0,   0,	0,   10,   0	      ,  100}, /* RSF_TELE_TO       */
    { 10,  0,   0,   0,	20,  10,   0	      ,  100}, /* RSF_TELE_AWAY     */
    { 10,  0,   0,   0,	20,  10,LRN_NEXUS_SAVE,  100}, /* RSF_TELE_LEVEL    */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XXX5	    */
    { 20,  0,   0,   0,	5,   0,	   0	      ,  100}, /* RSF_DARKNESS      */
    { 25,  0,   0,   0,	5,   0,	   0	      ,  100}, /* RSF_TRAPS	    */
    { 25,  0,   0,   0,	5,   0, LRN_SAVE      ,  100}, /* RSF_FORGET        */
    { 25,  0,   0,   15,0,   0, LRN_MANA      ,  100}, /* RSF_DRAIN_MANA    */
    { 25,  0,   0,   5,	0,   0,	   0          ,  100}, /* RSF_DISPEL        */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XXX6	    */
    { 30,  0,   0,   0,	0,   0, LRN_SAVE      ,  100}, /* RSF_MIND_BLAST    */
    { 40,  0,   0,   0,	0,   0, LRN_SAVE      ,  100}, /* RSF_BRAIN_SMASH   */
    { 40,  0,   0,   0,	0,   0, LRN_SAVE      ,  100}, /* RSF_WOUND	    */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XXX7	    */
    { 30,  0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_SHAPECHANGE   */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XXX8	    */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XXX9	    */
    { 25,  0,   0,   0,	0,   0,	 LRN_SAVE     ,  100}, /* RSF_HUNGER        */
    { 0,   0,   0,   0,	0,   0,	   0	      ,  100}, /* RSF_XX10	    */
    { 25,  0,   0,   0,	0,   0, LRN_FEAR_SAVE ,  100}, /* RSF_SCARE	    */
    { 30,  0,   0,   0,	0,   0, LRN_BLIND_SAVE,  100}, /* RSF_BLIND	    */
    { 30,  0,   0,   0,	0,   0, LRN_CONFU_SAVE,  100}, /* RSF_CONF	    */
    { 40,  0,   0,   0,	0,   0, LRN_FREE_SAVE,   100}, /* RSF_SLOW	    */
    { 35,  0,   0,   0,	0,   0, LRN_FREE_SAVE,   100}, /* RSF_HOLD	    */
/*   d_base	d_hurt d_esc	 d_res				    */
/*	   d_summ  d_mana  d_tact	   d_range		    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_KIN	    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX1    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX2    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_MONSTER */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_MONSTERS*/
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX3    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX4    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX5    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_ANT	    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_SPIDER  */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_HOUND   */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_ANIMAL  */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX6    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX7    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_THIEF   */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_SWAMP   */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX8    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XXX9    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XX10    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XX11    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_DRAGON  */
    { 0,   17,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_HI_DRAGON*/
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XX12    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XX13    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_DEMON   */
    { 0,   17,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_HI_DEMON*/
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_XX14    */
    { 0,   0,   0,   0,	0,   0,	   0	  ,  100}, /* RSF_APPROP    */
    { 0,   15,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_UNDEAD  */
    { 0,   17,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_HI_UNDEAD*/
    { 0,   20,  0,   0,	0,   0,	   0	  ,  100}, /* RSF_S_QUEST  */
    { 0,   18,  0,   0,	0,   0,	   0	  ,  100}  /* RSF_S_UNIQUE  */
};

/**
 * Optimal Ranges for various spells.
 * 6 is optimal for Breath Weapons, Beams, and Arcs.
 * 3 is optimal for Lash/Spit.
 * 0 indicates no range limitation for other spells.
 *
 * This range is considered a preference if d_range in spell_desire is > 0.
 * It is a hard limit if d_range = 0.
 */
byte spell_range[RSF_MAX] =
  {
    0,3,0,0,0,0,0,0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6,6,6,6,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

/**
 * List of all the stages and what is near them.  Order is
 * locality, level, stage numbers in the six directions (north, east, south,
 * west, up, down), stage type.
 * -NRM-
 */ 
int stage_map[NUM_STAGES][9] = { {0} };


int extended_map[NUM_STAGES][9] =
  {
  /* num     locality          lev    N    E    S    W    U    D    type */
  /* 0   */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 1   */ {HITHAEGLIR      ,   2,   0, 149,   0, 151,   0,   0,   MOUNTAIN},
  /* 2   */ {HITHAEGLIR      ,   3,   0, 151,   0,   3,   0,   0,   MOUNTAIN},
  /* 3   */ {ERIADOR         ,   4,   0,   2,   0,   4,   0,   0,   PLAIN},
  /* 4   */ {ERIADOR         ,   5,   0,   3,   0,   5,   0,   0,   PLAIN},
  /* 5   */ {ERIADOR         ,   6,   0,   4,   6,   7,   0,   0,   PLAIN},
  /* 6   */ {ERIADOR         ,   0,   5,   0, 248,   0,   0,   0,   TOWN},
  /* 7   */ {ERIADOR         ,   7,   0,   5,   0,   8,   0,   0,   PLAIN},
  /* 8   */ {ERIADOR         ,   8,   0,   7,   0,   9,   0,   0,   PLAIN},
  /* 9   */ {ERIADOR         ,   9,   0,   8,   0,  10,   0,   0,   PLAIN},
  /* 10  */ {ERIADOR         ,  10,   0,   9,   0, 152,   0,   0,   MOUNTAIN},
  /* 11  */ {ERED_LUIN       ,  10, 152,   0,  27,   0,   0,   0,   MOUNTAIN},
  /* 12  */ {ERED_LUIN       ,  11,  13,   0, 152,   0,   0,   0,   MOUNTAIN},
  /* 13  */ {ERED_LUIN       ,  12,  14,   0,  12,   0,   0,   0,   MOUNTAIN},
  /* 14  */ {ERED_LUIN       ,  13,  15,   0,  13,   0,   0,   0,   MOUNTAIN},
  /* 15  */ {ERED_LUIN       ,  14,  16,   0,  14,   0,   0,   0,   MOUNTAIN},
  /* 16  */ {ERED_LUIN       ,  15,  17,   0,  15,   0,   0,   0,   MOUNTAIN},
  /* 17  */ {ERED_LUIN       ,  16,  18,   0,  16,   0,   0,   0,   MOUNTAIN},
  /* 18  */ {ERED_LUIN       ,  17,  19,   0,  17,   0,   0,   0,   MOUNTAIN},
  /* 19  */ {ERED_LUIN       ,  18,  20,   0,  18,   0,   0,   0,   MOUNTAIN},
  /* 20  */ {ERED_LUIN       ,  19,  21,   0,  19,   0,   0,   0,   MOUNTAIN},
  /* 21  */ {ERED_LUIN       ,  20,  22,   0,  20,   0,   0,   0,   MOUNTAIN},
  /* 22  */ {ERED_LUIN       ,  22,  23,   0,  21,   0,   0,   0,   MOUNTAIN},
  /* 23  */ {ERED_LUIN       ,  24,  24,   0,  22,   0,   0,   0,   MOUNTAIN},
  /* 24  */ {ERED_LUIN       ,  26,   0,   0,  23,  25,   0,   0,   MOUNTAIN},
  /* 25  */ {ERED_LUIN       ,  28,  26,  24,   0,   0,   0,   0,   MOUNTAIN},
  /* 26  */ {ERED_LUIN       ,  30,   0,   0,  25, 136,   0,   0,   MOUNTAIN},
  /* 27  */ {OSSIRIAND       ,   9,  11,   0,  28,   0,   0,   0,   FOREST},
  /* 28  */ {OSSIRIAND       ,   8,  27,   0,  29,   0,   0,   0,   FOREST},
  /* 29  */ {OSSIRIAND       ,   7,  28,   0,   0,  30,   0,   0,   FOREST},
  /* 30  */ {OSSIRIAND       ,   0,  51,  29,  31,  38,   0,   0,   TOWN},
  /* 31  */ {OSSIRIAND       ,   6,  30,   0,  32,   0,   0,   0,   FOREST},
  /* 32  */ {OSSIRIAND       ,   5,  31,   0,  33,   0,   0,   0,   FOREST},
  /* 33  */ {OSSIRIAND       ,   4,  32,   0,  34,   0,   0,   0,   FOREST},
  /* 34  */ {OSSIRIAND       ,   3,  33,   0,  35,   0,   0,   0,   FOREST},
  /* 35  */ {OSSIRIAND       ,   2,  34,  36,   0,   0,   0,   0,   FOREST},
  /* 36  */ {ERED_LUIN_SOUTH ,   1,   0,  37,   0,  35,   0,   0,   MOUNTAIN},
  /* 37  */ {ERED_LUIN_SOUTH ,   0,   0, 252,   0,  36,   0,   0,   TOWN},
  /* 38  */ {ENT_PATH        ,   6,   0,  30,   0,  39,   0,   0,   FOREST},
  /* 39  */ {ENT_PATH        ,   5,   0,  38,  40,   0,   0,   0,   PLAIN},
  /* 40  */ {ENT_PATH        ,   4,  39,   0,  41,   0,   0,   0,   PLAIN},
  /* 41  */ {ENT_PATH        ,   3,  40,   0,   0,  42,   0,   0,   FOREST},
  /* 42  */ {ENT_PATH        ,   2,   0,  41,   0,  43,   0,   0,   FOREST},
  /* 43  */ {ENT_PATH        ,   1,   0,  42,   0,  44,   0,   0,   FOREST},
  /* 44  */ {TAUR_IM_DUINATH ,   0,  45,  43,   0,   0,   0,   0,   TOWN},
  /* 45  */ {TAUR_IM_DUINATH ,   1,  46,   0,  44,   0,   0,   0,   FOREST},
  /* 46  */ {TAUR_IM_DUINATH ,   2,  47,   0,  45,   0,   0,   0,   FOREST},
  /* 47  */ {EAST_BELERIAND  ,   3,  48,   0,  46,   0,   0,   0,   PLAIN},
  /* 48  */ {EAST_BELERIAND  ,   4,  49,   0,  47,   0,   0,   0,   PLAIN},
  /* 49  */ {EAST_BELERIAND  ,   5,  50,   0,  48,   0,   0,   0,   PLAIN},
  /* 50  */ {EAST_BELERIAND  ,   6,  70,   0,  49,   0,   0,   0,   PLAIN},
  /* 51  */ {EAST_BELERIAND  ,   7,   0,   0,  30,  52,   0,   0,   FOREST},
  /* 52  */ {EAST_BELERIAND  ,   8,   0,  51,   0,  53,   0,   0,   PLAIN},
  /* 53  */ {EAST_BELERIAND  ,   9,   0,  52,   0,  54,   0,   0,   PLAIN},
  /* 54  */ {EAST_BELERIAND  ,  10,   0,  53,   0,  55,   0,   0,   PLAIN},
  /* 55  */ {EAST_BELERIAND  ,  11,   0,  54,   0,  56,   0,   0,   PLAIN},
  /* 56  */ {EAST_BELERIAND  ,  12,   0,  55,   0,  57,   0,   0,   PLAIN},
  /* 57  */ {EAST_BELERIAND  ,  13,   0,  56,   0,  58,   0,   0,   PLAIN},
  /* 58  */ {EAST_BELERIAND  ,  14,  59,  57,   0,   0,   0,   0,   PLAIN},
  /* 59  */ {EAST_BELERIAND  ,  15,  82,   0,  58,   0,   0,   0,   PLAIN},
  /* 60  */ {EAST_BELERIAND  ,  16,   0,  69,   0,  61,   0,   0,   PLAIN},
  /* 61  */ {EAST_BELERIAND  ,  17,   0,  60,   0,  62,   0,   0,   PLAIN},
  /* 62  */ {EAST_BELERIAND  ,  18,   0,  61,   0,  63,   0,   0,   PLAIN},
  /* 63  */ {EAST_BELERIAND  ,  19,   0,  62,   0,  64,   0,   0,   PLAIN},
  /* 64  */ {EAST_BELERIAND  ,  20,   0,  63,   0,  89,   0,   0,   PLAIN},
  /* 65  */ {THARGELION      ,  11,   0, 152,   0,  66,   0,   0,   FOREST},
  /* 66  */ {THARGELION      ,  12,  67,  65,   0,   0,   0,   0,   RIVER},
  /* 67  */ {THARGELION      ,  13,  68,   0,  66,   0,   0,   0,   RIVER},
  /* 68  */ {THARGELION      ,  14,  69,   0,  67,   0,   0,   0,   RIVER},
  /* 69  */ {THARGELION      ,  15,   0,   0,  68,  60,   0,   0,   RIVER},
  /* 70  */ {ANDRAM          ,   7,  71,   0,  50,   0,   0,   0,   MOUNTAIN},
  /* 71  */ {ANDRAM          ,   8,  72,   0,  70,   0,   0,   0,   MOUNTAIN},
  /* 72  */ {ANDRAM          ,   9,  73,   0,  71,   0,   0,   0,   MOUNTAIN},
  /* 73  */ {ANDRAM          ,  10,  74,   0,  72,   0,   0,   0,   MOUNTAIN},
  /* 74  */ {WEST_BELERIAND  ,  11,  75,   0,  73,   0,   0,   0,   SWAMP},
  /* 75  */ {WEST_BELERIAND  ,  12,  76,   0,  74,   0,   0,   0,   PLAIN},
  /* 76  */ {WEST_BELERIAND  ,  13,  77,   0,  75,   0,   0,   0,   PLAIN},
  /* 77  */ {WEST_BELERIAND  ,  14, 155,   0,  76,   0,   0,   0,   MOUNTAIN},
  /* 78  */ {WEST_BELERIAND  ,  16,  79,   0, 155,   0,   0,   0,   PLAIN},
  /* 79  */ {WEST_BELERIAND  ,  17,  80,   0,  78,   0,   0,   0,   PLAIN},
  /* 80  */ {WEST_BELERIAND  ,  18,  81,   0,  79,   0,   0,   0,   PLAIN},
  /* 81  */ {WEST_BELERIAND  ,  19, 115,   0,  80,   0,   0,   0,   PLAIN},
  /* 82  */ {DORIATH         ,  16,  83,   0,  59,   0,   0,   0,   FOREST},
  /* 83  */ {DORIATH         ,  17,  84,   0,  82,   0,   0,   0,   FOREST},
  /* 84  */ {DORIATH         ,  18,  85,   0,  83,   0,   0,   0,   FOREST},
  /* 85  */ {DORIATH         ,  19, 153,   0,  84,   0,   0,   0,   FOREST},
  /* 86  */ {DORIATH         ,  20,  87,   0, 153,   0,   0,   0,   FOREST},
  /* 87  */ {DORIATH         ,  22,  88,   0,  86,   0,   0,   0,   FOREST},
  /* 88  */ {DORIATH         ,  24,  93,   0,  87,   0,   0,   0,   FOREST},
  /* 89  */ {HIMLAD          ,  22,   0,  64,   0,  90,   0,   0,   PLAIN},
  /* 90  */ {HIMLAD          ,  24,   0,  89,   0,  91,   0,   0,   PLAIN},
  /* 91  */ {HIMLAD          ,  26,   0,  90,   0,  92,   0,   0,   PLAIN},
  /* 92  */ {HIMLAD          ,  28,   0,  91,   0,  95,   0,   0,   PLAIN},
  /* 93  */ {DOR_DINEN       ,  26,  94,   0,  88,   0,   0,   0,   PLAIN},
  /* 94  */ {DOR_DINEN       ,  28,  95,   0,  93,   0,   0,   0,   PLAIN},
  /* 95  */ {DOR_DINEN       ,  30,  96,  92,  94,   0,   0,   0,   PLAIN},
  /* 96  */ {DORTHONION      ,  32,  97,   0,  95,   0,   0,   0,   MOUNTAIN},
  /* 97  */ {DORTHONION      ,  34,  98,   0,  96,   0,   0,   0,   MOUNTAIN},
  /* 98  */ {DORTHONION      ,  36,   0,   0,  97,  99,   0,   0,   FOREST},
  /* 99  */ {DORTHONION      ,  38,   0,  98, 181, 100,   0,   0,   FOREST},
  /* 100 */ {DORTHONION      ,  40,   0,  99,   0, 101,   0,   0,   FOREST},
  /* 101 */ {DORTHONION      ,  42,   0, 100,   0, 102,   0,   0,   FOREST},
  /* 102 */ {DORTHONION      ,  44,   0, 101,   0, 103,   0,   0,   FOREST},
  /* 103 */ {DORTHONION      ,  46, 104, 102,   0,   0,   0,   0,   FOREST},
  /* 104 */ {DORTHONION      ,  48,   0,   0, 103, 131,   0,   0,   PLAIN},
  /* 105 */ {TALATH_DIRNEN   ,  20,   0, 115, 106,   0,   0,   0,   PLAIN},
  /* 106 */ {TALATH_DIRNEN   ,  21, 105,   0,   0, 107,   0,   0,   PLAIN},
  /* 107 */ {TALATH_DIRNEN   ,  22,   0, 106, 108,   0,   0,   0,   PLAIN},
  /* 108 */ {TALATH_DIRNEN   ,  23, 107,   0,   0, 109,   0,   0,   PLAIN},
  /* 109 */ {TALATH_DIRNEN   ,  24,   0, 108, 110,   0,   0,   0,   PLAIN},
  /* 110 */ {TALATH_DIRNEN   ,  25, 109,   0,   0, 111,   0,   0,   PLAIN},
  /* 111 */ {TALATH_DIRNEN   ,  26,   0, 110, 112,   0,   0,   0,   PLAIN},
  /* 112 */ {TALATH_DIRNEN   ,  27, 111,   0,   0, 113,   0,   0,   PLAIN},
  /* 113 */ {TALATH_DIRNEN   ,  28,   0, 112,   0, 114,   0,   0,   PLAIN},
  /* 114 */ {TALATH_DIRNEN   ,  29,   0, 113,   0,   0,   0, 222,   RIVER},
  /* 115 */ {EPHEL_BRANDIR   ,   0, 116,   0,  81, 105,   0,   0,   TOWN},
  /* 116 */ {BRETHIL         ,  20, 117,   0, 115,   0,   0,   0,   FOREST},
  /* 117 */ {BRETHIL         ,  22, 118,   0, 116,   0,   0,   0,   FOREST},
  /* 118 */ {BRETHIL         ,  24, 119,   0, 117,   0,   0,   0,   FOREST},
  /* 119 */ {BRETHIL         ,  26, 120,   0, 118,   0,   0,   0,   FOREST},
  /* 120 */ {SIRION_VALE     ,  28, 121, 154, 119,   0,   0,   0,   PLAIN},
  /* 121 */ {SIRION_VALE     ,  30, 122,   0, 120,   0,   0,   0,   RIVER},
  /* 122 */ {SIRION_VALE     ,  32, 123,   0, 121,   0,   0,   0,   RIVER},
  /* 123 */ {SIRION_VALE     ,  34, 124,   0, 122,   0,   0,   0,   RIVER},
  /* 124 */ {SIRION_VALE     ,  36, 125,   0, 123,   0,   0,   0,   RIVER},
  /* 125 */ {SIRION_VALE     ,  38, 126,   0, 124,   0,   0, 273,   RIVER},
  /* 126 */ {SIRION_VALE     ,  40, 127,   0, 125,   0,   0,   0,   RIVER},
  /* 127 */ {FEN_OF_SERECH   ,  42, 128,   0, 126,   0,   0,   0,   SWAMP},
  /* 128 */ {FEN_OF_SERECH   ,  44, 129,   0, 127,   0,   0,   0,   SWAMP},
  /* 129 */ {ANFAUGLITH      ,  46, 130,   0, 128,   0,   0,   0,   DESERT},
  /* 130 */ {ANFAUGLITH      ,  48, 131,   0, 129,   0,   0,   0,   DESERT},
  /* 131 */ {ANFAUGLITH      ,  50, 132, 104, 130,   0,   0,   0,   DESERT},
  /* 132 */ {ANFAUGLITH      ,  52, 133,   0, 131,   0,   0,   0,   DESERT},
  /* 133 */ {ANFAUGLITH      ,  54, 134,   0, 132,   0,   0,   0,   DESERT},
  /* 134 */ {ANFAUGLITH      ,  56, 135,   0, 133,   0,   0,   0,   DESERT},
  /* 135 */ {ANFAUGLITH      ,  58,   0, 148, 134,   0,   0, 344,   DESERT},
  /* 136 */ {LOTHLANN        ,  32, 137,  26,   0,   0,   0,   0,   PLAIN},
  /* 137 */ {LOTHLANN        ,  34,   0,   0, 136, 138,   0,   0,   PLAIN},
  /* 138 */ {LOTHLANN        ,  36, 139, 137,   0,   0,   0,   0,   PLAIN},
  /* 139 */ {LOTHLANN        ,  38,   0,   0, 138, 140,   0,   0,   PLAIN},
  /* 140 */ {LOTHLANN        ,  40,   0, 139,   0, 141,   0,   0,   PLAIN},
  /* 141 */ {LOTHLANN        ,  42,   0, 140,   0, 142,   0,   0,   PLAIN},
  /* 142 */ {LOTHLANN        ,  44,   0, 141,   0, 143,   0,   0,   PLAIN},
  /* 143 */ {LOTHLANN        ,  46,   0, 142,   0, 144,   0,   0,   DESERT},
  /* 144 */ {LOTHLANN        ,  48,   0, 143,   0, 145,   0,   0,   DESERT},
  /* 145 */ {LOTHLANN        ,  50,   0, 144,   0, 146,   0,   0,   DESERT},
  /* 146 */ {LOTHLANN        ,  52,   0, 145,   0, 147,   0,   0,   DESERT},
  /* 147 */ {LOTHLANN        ,  54,   0, 146,   0, 148,   0,   0,   DESERT},
  /* 148 */ {LOTHLANN        ,  56,   0, 147,   0, 135,   0,   0,   DESERT},
  /* 149 */ {ANDUIN_VALE     ,   1,   0, 150,   0,   1,   0,   0,   PLAIN},
  /* 150 */ {GLADDEN_FIELDS  ,   0,   0,   0,   0, 149,   0,   0,   TOWN},
  /* 151 */ {KHAZAD_DUM      ,   0,   0,   1,   0,   2,   0,   0,   TOWN},
  /* 152 */ {BELEGOST        ,   0,  12,  10,  11,  65,   0,   0,   TOWN},
  /* 153 */ {MENEGROTH       ,   0,  86,   0,  85,   0,   0,   0,   TOWN},
  /* 154 */ {GONDOLIN        ,   0,   0,   0,   0, 120,   0,   0,   TOWN},
  /* 155 */ {AMON_RUDH       ,  15,  78,   0,  77,   0,   0, 156,   MOUNTAIN},
  /* 156 */ {AMON_RUDH       ,  16,   0,   0,   0,   0, 155, 157,   CAVE},
  /* 157 */ {AMON_RUDH       ,  17,   0,   0,   0,   0, 156, 158,   CAVE},
  /* 158 */ {AMON_RUDH       ,  18,   0,   0,   0,   0, 157, 159,   CAVE},
  /* 159 */ {AMON_RUDH       ,  19,   0,   0,   0,   0, 158, 160,   CAVE},
  /* 160 */ {AMON_RUDH       ,  20,   0,   0,   0,   0, 159, 161,   CAVE},
  /* 161 */ {AMON_RUDH       ,  21,   0,   0,   0,   0, 160, 162,   CAVE},
  /* 162 */ {AMON_RUDH       ,  22,   0,   0,   0,   0, 161, 163,   CAVE},
  /* 163 */ {AMON_RUDH       ,  23,   0,   0,   0,   0, 162, 164,   CAVE},
  /* 164 */ {AMON_RUDH       ,  24,   0,   0,   0,   0, 163, 165,   CAVE},
  /* 165 */ {AMON_RUDH       ,  25,   0,   0,   0,   0, 164, 166,   CAVE},
  /* 166 */ {AMON_RUDH       ,  26,   0,   0,   0,   0, 165, 167,   CAVE},
  /* 167 */ {AMON_RUDH       ,  27,   0,   0,   0,   0, 166, 168,   CAVE},
  /* 168 */ {AMON_RUDH       ,  28,   0,   0,   0,   0, 167, 169,   CAVE},
  /* 169 */ {AMON_RUDH       ,  29,   0,   0,   0,   0, 168, 170,   CAVE},
  /* 170 */ {AMON_RUDH       ,  30,   0,   0,   0,   0, 169,   0,   CAVE},
  /* 171 */ {NOWHERE         ,  30,   0,   0,   0,   0,   0,   0,   0},
  /* 172 */ {NOWHERE         ,  31,   0,   0,   0,   0,   0,   0,   0},
  /* 173 */ {NOWHERE         ,  32,   0,   0,   0,   0,   0,   0,   0},
  /* 174 */ {NOWHERE         ,  33,   0,   0,   0,   0,   0,   0,   0},
  /* 175 */ {NOWHERE         ,  34,   0,   0,   0,   0,   0,   0,   0},
  /* 176 */ {NOWHERE         ,  35,   0,   0,   0,   0,   0,   0,   0},
  /* 177 */ {NOWHERE         ,  36,   0,   0,   0,   0,   0,   0,   0},
  /* 178 */ {NOWHERE         ,  37,   0,   0,   0,   0,   0,   0,   0},
  /* 179 */ {NOWHERE         ,  38,   0,   0,   0,   0,   0,   0,   0},
  /* 180 */ {NOWHERE         ,  39,   0,   0,   0,   0,   0,   0,   0},
  /* 181 */ {NAN_DUNGORTHEB  ,  40,   0,   0, 182,   0,   0,   0,   VALLEY},
  /* 182 */ {NAN_DUNGORTHEB  ,  41,   0,   0, 183,   0,   0,   0,   VALLEY},
  /* 183 */ {NAN_DUNGORTHEB  ,  42,   0,   0, 184,   0,   0,   0,   VALLEY},
  /* 184 */ {NAN_DUNGORTHEB  ,  43,   0,   0, 185,   0,   0,   0,   VALLEY},
  /* 185 */ {NAN_DUNGORTHEB  ,  44,   0,   0, 186,   0,   0,   0,   VALLEY},
  /* 186 */ {NAN_DUNGORTHEB  ,  45,   0,   0, 187,   0,   0,   0,   VALLEY},
  /* 187 */ {NAN_DUNGORTHEB  ,  46,   0,   0, 188,   0,   0,   0,   VALLEY},
  /* 188 */ {NAN_DUNGORTHEB  ,  47,   0,   0, 189,   0,   0,   0,   VALLEY},
  /* 189 */ {NAN_DUNGORTHEB  ,  48,   0,   0, 190,   0,   0,   0,   VALLEY},
  /* 190 */ {NAN_DUNGORTHEB  ,  49,   0,   0, 191,   0,   0,   0,   VALLEY},
  /* 191 */ {NAN_DUNGORTHEB  ,  50,   0,   0, 192,   0,   0,   0,   VALLEY},
  /* 192 */ {NAN_DUNGORTHEB  ,  51,   0,   0, 193,   0,   0,   0,   VALLEY},
  /* 193 */ {NAN_DUNGORTHEB  ,  52,   0,   0, 194,   0,   0,   0,   VALLEY},
  /* 194 */ {NAN_DUNGORTHEB  ,  53,   0,   0, 195,   0,   0,   0,   VALLEY},
  /* 195 */ {NAN_DUNGORTHEB  ,  54,   0,   0, 196,   0,   0,   0,   VALLEY},
  /* 196 */ {NAN_DUNGORTHEB  ,  55,   0,   0, 197,   0,   0,   0,   VALLEY},
  /* 197 */ {NAN_DUNGORTHEB  ,  56,   0,   0, 198,   0,   0,   0,   VALLEY},
  /* 198 */ {NAN_DUNGORTHEB  ,  57,   0,   0, 199,   0,   0,   0,   VALLEY},
  /* 199 */ {NAN_DUNGORTHEB  ,  58,   0,   0, 200,   0,   0,   0,   VALLEY},
  /* 200 */ {NAN_DUNGORTHEB  ,  59,   0,   0, 201,   0,   0,   0,   VALLEY},
  /* 201 */ {NAN_DUNGORTHEB  ,  60,   0,   0, 202,   0,   0,   0,   VALLEY},
  /* 202 */ {NAN_DUNGORTHEB  ,  61,   0,   0, 203,   0,   0,   0,   VALLEY},
  /* 203 */ {NAN_DUNGORTHEB  ,  62,   0,   0, 204,   0,   0,   0,   VALLEY},
  /* 204 */ {NAN_DUNGORTHEB  ,  63,   0,   0, 205,   0,   0,   0,   VALLEY},
  /* 205 */ {NAN_DUNGORTHEB  ,  64,   0,   0, 206,   0,   0,   0,   VALLEY},
  /* 206 */ {NAN_DUNGORTHEB  ,  65,   0,   0, 207,   0,   0,   0,   VALLEY},
  /* 207 */ {NAN_DUNGORTHEB  ,  66,   0,   0, 208,   0,   0,   0,   VALLEY},
  /* 208 */ {NAN_DUNGORTHEB  ,  67,   0,   0, 209,   0,   0,   0,   VALLEY},
  /* 209 */ {NAN_DUNGORTHEB  ,  68,   0,   0, 210,   0,   0,   0,   VALLEY},
  /* 210 */ {NAN_DUNGORTHEB  ,  69,   0,   0, 211,   0,   0,   0,   VALLEY},
  /* 211 */ {NAN_DUNGORTHEB  ,  70,   0,   0,  88,   0,   0,   0,   VALLEY},
  /* 212 */ {NOWHERE         ,  71,   0,   0,   0,   0,   0,   0,   0},
  /* 213 */ {NOWHERE         ,  72,   0,   0,   0,   0,   0,   0,   0},
  /* 214 */ {NOWHERE         ,  73,   0,   0,   0,   0,   0,   0,   0},
  /* 215 */ {NOWHERE         ,  74,   0,   0,   0,   0,   0,   0,   0},
  /* 216 */ {NOWHERE         ,  75,   0,   0,   0,   0,   0,   0,   0},
  /* 217 */ {NOWHERE         ,  76,   0,   0,   0,   0,   0,   0,   0},
  /* 218 */ {NOWHERE         ,  77,   0,   0,   0,   0,   0,   0,   0},
  /* 219 */ {NOWHERE         ,  78,   0,   0,   0,   0,   0,   0,   0},
  /* 220 */ {NOWHERE         ,  79,   0,   0,   0,   0,   0,   0,   0},
  /* 221 */ {NOWHERE         ,  80,   0,   0,   0,   0,   0,   0,   0},
  /* 222 */ {NARGOTHROND     ,  30,   0,   0,   0,   0, 114, 223,   CAVE},
  /* 223 */ {NARGOTHROND     ,  31,   0,   0,   0,   0, 222, 224,   CAVE},
  /* 224 */ {NARGOTHROND     ,  32,   0,   0,   0,   0, 223, 225,   CAVE},
  /* 225 */ {NARGOTHROND     ,  33,   0,   0,   0,   0, 224, 226,   CAVE},
  /* 226 */ {NARGOTHROND     ,  34,   0,   0,   0,   0, 225, 227,   CAVE},
  /* 227 */ {NARGOTHROND     ,  35,   0,   0,   0,   0, 226, 228,   CAVE},
  /* 228 */ {NARGOTHROND     ,  36,   0,   0,   0,   0, 227, 229,   CAVE},
  /* 229 */ {NARGOTHROND     ,  37,   0,   0,   0,   0, 228, 230,   CAVE},
  /* 230 */ {NARGOTHROND     ,  38,   0,   0,   0,   0, 229, 231,   CAVE},
  /* 231 */ {NARGOTHROND     ,  39,   0,   0,   0,   0, 230, 232,   CAVE},
  /* 232 */ {NARGOTHROND     ,  40,   0,   0,   0,   0, 231, 233,   CAVE},
  /* 233 */ {NARGOTHROND     ,  41,   0,   0,   0,   0, 232, 234,   CAVE},
  /* 234 */ {NARGOTHROND     ,  42,   0,   0,   0,   0, 233, 235,   CAVE},
  /* 235 */ {NARGOTHROND     ,  43,   0,   0,   0,   0, 234, 236,   CAVE},
  /* 236 */ {NARGOTHROND     ,  44,   0,   0,   0,   0, 235, 237,   CAVE},
  /* 237 */ {NARGOTHROND     ,  45,   0,   0,   0,   0, 236, 238,   CAVE},
  /* 238 */ {NARGOTHROND     ,  46,   0,   0,   0,   0, 237, 239,   CAVE},
  /* 239 */ {NARGOTHROND     ,  47,   0,   0,   0,   0, 238, 240,   CAVE},
  /* 240 */ {NARGOTHROND     ,  48,   0,   0,   0,   0, 239, 241,   CAVE},
  /* 241 */ {NARGOTHROND     ,  49,   0,   0,   0,   0, 240, 242,   CAVE},
  /* 242 */ {NARGOTHROND     ,  50,   0,   0,   0,   0, 241, 243,   CAVE},
  /* 243 */ {NARGOTHROND     ,  51,   0,   0,   0,   0, 242, 244,   CAVE},
  /* 244 */ {NARGOTHROND     ,  52,   0,   0,   0,   0, 243, 245,   CAVE},
  /* 245 */ {NARGOTHROND     ,  53,   0,   0,   0,   0, 244, 246,   CAVE},
  /* 246 */ {NARGOTHROND     ,  54,   0,   0,   0,   0, 245, 247,   CAVE},
  /* 247 */ {NARGOTHROND     ,  55,   0,   0,   0,   0, 246,   0,   CAVE},
  /* 248 */ {ERIADOR_SOUTH   ,   5,   6,   0, 249,   0,   0,   0,   PLAIN},
  /* 249 */ {ERIADOR_SOUTH   ,   4, 248,   0,   0, 250,   0,   0,   PLAIN},
  /* 250 */ {ERIADOR_SOUTH   ,   3,   0, 249, 251,   0,   0,   0,   PLAIN},
  /* 251 */ {ERIADOR_SOUTH   ,   2, 250,   0,   0, 252,   0,   0,   PLAIN},
  /* 252 */ {ERIADOR_SOUTH   ,   1,   0, 251,   0,  37,   0,   0,   MOUNTAIN},
  /* 253 */ {NOWHERE         ,  61,   0,   0,   0,   0,   0,   0,   0},
  /* 254 */ {NOWHERE         ,  62,   0,   0,   0,   0,   0,   0,   0},
  /* 255 */ {UNDERWORLD      ,   0,   0,   0,   0,   0,   0,   0,   CAVE},
  /* 256 */ {MOUNTAIN_TOP    ,   0,   0,   0,   0,   0,   0,   0, MOUNTAINTOP},
  /* 257 */ {NOWHERE         ,  65,   0,   0,   0,   0,   0,   0,   0},
  /* 258 */ {NOWHERE         ,  66,   0,   0,   0,   0,   0,   0,   0},
  /* 259 */ {NOWHERE         ,  67,   0,   0,   0,   0,   0,   0,   0},
  /* 260 */ {NOWHERE         ,  68,   0,   0,   0,   0,   0,   0,   0},
  /* 261 */ {NOWHERE         ,  69,   0,   0,   0,   0,   0,   0,   0},
  /* 262 */ {NOWHERE         ,  70,   0,   0,   0,   0,   0,   0,   0},
  /* 263 */ {NOWHERE         ,  30,   0,   0,   0,   0,   0,   0,   0},
  /* 264 */ {NOWHERE         ,  31,   0,   0,   0,   0,   0,   0,   0},
  /* 265 */ {NOWHERE         ,  32,   0,   0,   0,   0,   0,   0,   0},
  /* 266 */ {NOWHERE         ,  33,   0,   0,   0,   0,   0,   0,   0},
  /* 267 */ {NOWHERE         ,  34,   0,   0,   0,   0,   0,   0,   0},
  /* 268 */ {NOWHERE         ,  35,   0,   0,   0,   0,   0,   0,   0},
  /* 269 */ {NOWHERE         ,  36,   0,   0,   0,   0,   0,   0,   0},
  /* 270 */ {NOWHERE         ,  37,   0,   0,   0,   0,   0,   0,   0},
  /* 271 */ {NOWHERE         ,  38,   0,   0,   0,   0,   0,   0,   0},
  /* 272 */ {NOWHERE         ,  39,   0,   0,   0,   0,   0,   0,   0},
  /* 273 */ {TOL_IN_GAURHOTH ,  40,   0,   0,   0,   0, 125, 274,   CAVE},
  /* 274 */ {TOL_IN_GAURHOTH ,  41,   0,   0,   0,   0, 273, 275,   CAVE},
  /* 275 */ {TOL_IN_GAURHOTH ,  42,   0,   0,   0,   0, 274, 276,   CAVE},
  /* 276 */ {TOL_IN_GAURHOTH ,  43,   0,   0,   0,   0, 275, 277,   CAVE},
  /* 277 */ {TOL_IN_GAURHOTH ,  44,   0,   0,   0,   0, 276, 278,   CAVE},
  /* 278 */ {TOL_IN_GAURHOTH ,  45,   0,   0,   0,   0, 277, 279,   CAVE},
  /* 279 */ {TOL_IN_GAURHOTH ,  46,   0,   0,   0,   0, 278, 280,   CAVE},
  /* 280 */ {TOL_IN_GAURHOTH ,  47,   0,   0,   0,   0, 279, 281,   CAVE},
  /* 281 */ {TOL_IN_GAURHOTH ,  48,   0,   0,   0,   0, 280, 282,   CAVE},
  /* 282 */ {TOL_IN_GAURHOTH ,  49,   0,   0,   0,   0, 281, 283,   CAVE},
  /* 283 */ {TOL_IN_GAURHOTH ,  50,   0,   0,   0,   0, 282, 284,   CAVE},
  /* 284 */ {TOL_IN_GAURHOTH ,  51,   0,   0,   0,   0, 283, 285,   CAVE},
  /* 285 */ {TOL_IN_GAURHOTH ,  52,   0,   0,   0,   0, 284, 286,   CAVE},
  /* 286 */ {TOL_IN_GAURHOTH ,  53,   0,   0,   0,   0, 285, 287,   CAVE},
  /* 287 */ {TOL_IN_GAURHOTH ,  54,   0,   0,   0,   0, 286, 288,   CAVE},
  /* 288 */ {TOL_IN_GAURHOTH ,  55,   0,   0,   0,   0, 287, 289,   CAVE},
  /* 289 */ {TOL_IN_GAURHOTH ,  56,   0,   0,   0,   0, 288, 290,   CAVE},
  /* 290 */ {TOL_IN_GAURHOTH ,  57,   0,   0,   0,   0, 289, 291,   CAVE},
  /* 291 */ {TOL_IN_GAURHOTH ,  58,   0,   0,   0,   0, 290, 292,   CAVE},
  /* 292 */ {TOL_IN_GAURHOTH ,  59,   0,   0,   0,   0, 291, 293,   CAVE},
  /* 293 */ {TOL_IN_GAURHOTH ,  60,   0,   0,   0,   0, 292, 294,   CAVE},
  /* 294 */ {TOL_IN_GAURHOTH ,  61,   0,   0,   0,   0, 293, 295,   CAVE},
  /* 295 */ {TOL_IN_GAURHOTH ,  62,   0,   0,   0,   0, 294, 296,   CAVE},
  /* 296 */ {TOL_IN_GAURHOTH ,  63,   0,   0,   0,   0, 295, 297,   CAVE},
  /* 297 */ {TOL_IN_GAURHOTH ,  64,   0,   0,   0,   0, 296, 298,   CAVE},
  /* 298 */ {TOL_IN_GAURHOTH ,  65,   0,   0,   0,   0, 297, 299,   CAVE},
  /* 299 */ {TOL_IN_GAURHOTH ,  66,   0,   0,   0,   0, 298, 300,   CAVE},
  /* 300 */ {TOL_IN_GAURHOTH ,  67,   0,   0,   0,   0, 299, 301,   CAVE},
  /* 301 */ {TOL_IN_GAURHOTH ,  68,   0,   0,   0,   0, 300, 302,   CAVE},
  /* 302 */ {TOL_IN_GAURHOTH ,  69,   0,   0,   0,   0, 301, 303,   CAVE},
  /* 303 */ {TOL_IN_GAURHOTH ,  70,   0,   0,   0,   0, 302, 304,   CAVE},
  /* 304 */ {TOL_IN_GAURHOTH ,  71,   0,   0,   0,   0, 303, 305,   CAVE},
  /* 305 */ {TOL_IN_GAURHOTH ,  72,   0,   0,   0,   0, 304, 306,   CAVE},
  /* 306 */ {TOL_IN_GAURHOTH ,  73,   0,   0,   0,   0, 305, 307,   CAVE},
  /* 307 */ {TOL_IN_GAURHOTH ,  74,   0,   0,   0,   0, 306, 308,   CAVE},
  /* 308 */ {TOL_IN_GAURHOTH ,  75,   0,   0,   0,   0, 307, 309,   CAVE},
  /* 309 */ {TOL_IN_GAURHOTH ,  76,   0,   0,   0,   0, 308, 310,   CAVE},
  /* 310 */ {TOL_IN_GAURHOTH ,  77,   0,   0,   0,   0, 309, 311,   CAVE},
  /* 311 */ {TOL_IN_GAURHOTH ,  78,   0,   0,   0,   0, 310, 312,   CAVE},
  /* 312 */ {TOL_IN_GAURHOTH ,  79,   0,   0,   0,   0, 311, 313,   CAVE},
  /* 313 */ {TOL_IN_GAURHOTH ,  80,   0,   0,   0,   0, 312, 314,   CAVE},
  /* 314 */ {TOL_IN_GAURHOTH ,  81,   0,   0,   0,   0, 313, 315,   CAVE},
  /* 315 */ {TOL_IN_GAURHOTH ,  82,   0,   0,   0,   0, 314, 316,   CAVE},
  /* 316 */ {TOL_IN_GAURHOTH ,  83,   0,   0,   0,   0, 315, 317,   CAVE},
  /* 317 */ {TOL_IN_GAURHOTH ,  84,   0,   0,   0,   0, 316, 318,   CAVE},
  /* 318 */ {TOL_IN_GAURHOTH ,  85,   0,   0,   0,   0, 317,   0,   CAVE},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 339 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 340 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 341 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 342 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 343 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 344 */ {ANGBAND         ,  60,   0,   0,   0,   0, 135, 345,   CAVE},
  /* 345 */ {ANGBAND         ,  61,   0,   0,   0,   0, 344, 346,   CAVE},
  /* 346 */ {ANGBAND         ,  62,   0,   0,   0,   0, 345, 347,   CAVE},
  /* 347 */ {ANGBAND         ,  63,   0,   0,   0,   0, 346, 348,   CAVE},
  /* 348 */ {ANGBAND         ,  64,   0,   0,   0,   0, 347, 349,   CAVE},
  /* 349 */ {ANGBAND         ,  65,   0,   0,   0,   0, 348, 350,   CAVE},
  /* 350 */ {ANGBAND         ,  66,   0,   0,   0,   0, 349, 351,   CAVE},
  /* 351 */ {ANGBAND         ,  67,   0,   0,   0,   0, 350, 352,   CAVE},
  /* 352 */ {ANGBAND         ,  68,   0,   0,   0,   0, 351, 353,   CAVE},
  /* 353 */ {ANGBAND         ,  69,   0,   0,   0,   0, 352, 354,   CAVE},
  /* 354 */ {ANGBAND         ,  70,   0,   0,   0,   0, 353, 355,   CAVE},
  /* 355 */ {ANGBAND         ,  71,   0,   0,   0,   0, 354, 356,   CAVE},
  /* 356 */ {ANGBAND         ,  72,   0,   0,   0,   0, 355, 357,   CAVE},
  /* 357 */ {ANGBAND         ,  73,   0,   0,   0,   0, 356, 358,   CAVE},
  /* 358 */ {ANGBAND         ,  74,   0,   0,   0,   0, 357, 359,   CAVE},
  /* 359 */ {ANGBAND         ,  75,   0,   0,   0,   0, 358, 360,   CAVE},
  /* 360 */ {ANGBAND         ,  76,   0,   0,   0,   0, 359, 361,   CAVE},
  /* 361 */ {ANGBAND         ,  77,   0,   0,   0,   0, 360, 362,   CAVE},
  /* 362 */ {ANGBAND         ,  78,   0,   0,   0,   0, 361, 363,   CAVE},
  /* 363 */ {ANGBAND         ,  79,   0,   0,   0,   0, 362, 364,   CAVE},
  /* 364 */ {ANGBAND         ,  80,   0,   0,   0,   0, 363, 365,   CAVE},
  /* 365 */ {ANGBAND         ,  81,   0,   0,   0,   0, 364, 366,   CAVE},
  /* 366 */ {ANGBAND         ,  82,   0,   0,   0,   0, 365, 367,   CAVE},
  /* 367 */ {ANGBAND         ,  83,   0,   0,   0,   0, 366, 368,   CAVE},
  /* 368 */ {ANGBAND         ,  84,   0,   0,   0,   0, 367, 369,   CAVE},
  /* 369 */ {ANGBAND         ,  85,   0,   0,   0,   0, 368, 370,   CAVE},
  /* 370 */ {ANGBAND         ,  86,   0,   0,   0,   0, 369, 371,   CAVE},
  /* 371 */ {ANGBAND         ,  87,   0,   0,   0,   0, 370, 372,   CAVE},
  /* 372 */ {ANGBAND         ,  88,   0,   0,   0,   0, 371, 373,   CAVE},
  /* 373 */ {ANGBAND         ,  89,   0,   0,   0,   0, 372, 374,   CAVE},
  /* 374 */ {ANGBAND         ,  90,   0,   0,   0,   0, 373, 375,   CAVE},
  /* 375 */ {ANGBAND         ,  91,   0,   0,   0,   0, 374, 376,   CAVE},
  /* 376 */ {ANGBAND         ,  92,   0,   0,   0,   0, 375, 377,   CAVE},
  /* 377 */ {ANGBAND         ,  93,   0,   0,   0,   0, 376, 378,   CAVE},
  /* 378 */ {ANGBAND         ,  94,   0,   0,   0,   0, 377, 379,   CAVE},
  /* 379 */ {ANGBAND         ,  95,   0,   0,   0,   0, 378, 380,   CAVE},
  /* 380 */ {ANGBAND         ,  96,   0,   0,   0,   0, 379, 381,   CAVE},
  /* 381 */ {ANGBAND         ,  97,   0,   0,   0,   0, 380, 382,   CAVE},
  /* 382 */ {ANGBAND         ,  98,   0,   0,   0,   0, 381, 383,   CAVE},
  /* 383 */ {ANGBAND         ,  99,   0,   0,   0,   0, 382, 384,   CAVE},
  /* 384 */ {ANGBAND         , 100,   0,   0,   0,   0, 383, 385,   CAVE},
  /* 385 */ {ANGBAND         , 101,   0,   0,   0,   0, 384, 386,   CAVE},
  /* 386 */ {ANGBAND         , 102,   0,   0,   0,   0, 385, 387,   CAVE},
  /* 387 */ {ANGBAND         , 103,   0,   0,   0,   0, 386, 388,   CAVE},
  /* 388 */ {ANGBAND         , 104,   0,   0,   0,   0, 387, 389,   CAVE},
  /* 389 */ {ANGBAND         , 105,   0,   0,   0,   0, 388, 390,   CAVE},
  /* 390 */ {ANGBAND         , 106,   0,   0,   0,   0, 389, 391,   CAVE},
  /* 391 */ {ANGBAND         , 107,   0,   0,   0,   0, 390, 392,   CAVE},
  /* 392 */ {ANGBAND         , 108,   0,   0,   0,   0, 391, 393,   CAVE},
  /* 393 */ {ANGBAND         , 109,   0,   0,   0,   0, 392, 394,   CAVE},
  /* 394 */ {ANGBAND         , 110,   0,   0,   0,   0, 393, 395,   CAVE},
  /* 395 */ {ANGBAND         , 111,   0,   0,   0,   0, 394, 396,   CAVE},
  /* 396 */ {ANGBAND         , 112,   0,   0,   0,   0, 395, 397,   CAVE},
  /* 397 */ {ANGBAND         , 113,   0,   0,   0,   0, 396, 398,   CAVE},
  /* 398 */ {ANGBAND         , 114,   0,   0,   0,   0, 397, 399,   CAVE},
  /* 399 */ {ANGBAND         , 115,   0,   0,   0,   0, 398, 400,   CAVE},
  /* 400 */ {ANGBAND         , 116,   0,   0,   0,   0, 399, 401,   CAVE},
  /* 401 */ {ANGBAND         , 117,   0,   0,   0,   0, 400, 402,   CAVE},
  /* 402 */ {ANGBAND         , 118,   0,   0,   0,   0, 401, 403,   CAVE},
  /* 403 */ {ANGBAND         , 119,   0,   0,   0,   0, 402, 404,   CAVE},
  /* 404 */ {ANGBAND         , 120,   0,   0,   0,   0, 403, 405,   CAVE},
  /* 405 */ {ANGBAND         , 121,   0,   0,   0,   0, 404, 406,   CAVE},
  /* 406 */ {ANGBAND         , 122,   0,   0,   0,   0, 405, 407,   CAVE},
  /* 407 */ {ANGBAND         , 123,   0,   0,   0,   0, 406, 408,   CAVE},
  /* 408 */ {ANGBAND         , 124,   0,   0,   0,   0, 407, 409,   CAVE},
  /* 409 */ {ANGBAND         , 125,   0,   0,   0,   0, 408, 410,   CAVE},
  /* 410 */ {ANGBAND         , 126,   0,   0,   0,   0, 409, 411,   CAVE},
  /* 411 */ {ANGBAND         , 127,   0,   0,   0,   0, 410,   0,   CAVE}};


int compressed_map[NUM_STAGES][9] =
  {
  /* num     locality          lev    N    E    S    W    U    D    type */
  /* 0   */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 1   */ {HITHAEGLIR      ,   2,   0,  78,   0,  80,   0,   0,   MOUNTAIN},
  /* 2   */ {HITHAEGLIR      ,   3,   0,  80,   0,   3,   0,   0,   MOUNTAIN},
  /* 3   */ {ERIADOR         ,   4,   0,   2,   0,   4,   0,   0,   PLAIN},
  /* 4   */ {ERIADOR         ,   6,   0,   3,   5,   6,   0,   0,   PLAIN},
  /* 5   */ {ERIADOR         ,   0,   4,   0,   7,   0,   0,   0,   TOWN},
  /* 6   */ {ERIADOR         ,   9,   0,   4,   0,  81,   0,   0,   MOUNTAIN},
  /* 7   */ {ERIADOR_SOUTH   ,   5,   5,   0,   8,   0,   0,   0,   PLAIN},
  /* 8   */ {ERIADOR_SOUTH   ,   4,   7,   0,   0,   9,   0,   0,   PLAIN},
  /* 9   */ {ERIADOR_SOUTH   ,   3,   0,   8,  10,   0,   0,   0,   PLAIN},
  /* 10  */ {ERIADOR_SOUTH   ,   2,   9,   0,   0,  11,   0,   0,   PLAIN},
  /* 11  */ {ERIADOR_SOUTH   ,   1,   0,  10,   0,  24,   0,   0,   MOUNTAIN},
  /* 12  */ {ERED_LUIN       ,  10,  81,   0,  19,   0,   0,   0,   MOUNTAIN},
  /* 13  */ {ERED_LUIN       ,  12,  14,   0,  81,   0,   0,   0,   MOUNTAIN},
  /* 14  */ {ERED_LUIN       ,  15,  15,   0,  13,   0,   0,   0,   MOUNTAIN},
  /* 15  */ {ERED_LUIN       ,  18,  16,   0,  14,   0,   0,   0,   MOUNTAIN},
  /* 16  */ {ERED_LUIN       ,  21,  17,   0,  15,   0,   0,   0,   MOUNTAIN},
  /* 17  */ {ERED_LUIN       ,  24,  18,   0,  16,   0,   0,   0,   MOUNTAIN},
  /* 18  */ {ERED_LUIN       ,  28,   0,   0,  17,  71,   0,   0,   MOUNTAIN},
  /* 19  */ {OSSIRIAND       ,   8,  12,   0,   0,  20,   0,   0,   FOREST},
  /* 20  */ {OSSIRIAND       ,   0,  30,  19,  21,  25,   0,   0,   TOWN},
  /* 21  */ {OSSIRIAND       ,   6,  20,   0,  22,   0,   0,   0,   FOREST},
  /* 22  */ {OSSIRIAND       ,   4,  21,   0,  23,   0,   0,   0,   FOREST},
  /* 23  */ {ERED_LUIN_SOUTH ,   2,  22,  24,   0,   0,   0,   0,   MOUNTAIN},
  /* 24  */ {ERED_LUIN_SOUTH ,   0,   0,  11,   0,  23,   0,   0,   TOWN},
  /* 25  */ {ENT_PATH        ,   4,   0,  20,  26,   0,   0,   0,   FOREST},
  /* 26  */ {ENT_PATH        ,   1,  25,   0,   0,  27,   0,   0,   FOREST},
  /* 27  */ {TAUR_IM_DUINATH ,   0,  28,  26,   0,   0,   0,   0,   TOWN},
  /* 28  */ {TAUR_IM_DUINATH ,   2,  29,   0,  27,   0,   0,   0,   FOREST},
  /* 29  */ {EAST_BELERIAND  ,   5,  37,   0,  28,   0,   0,   0,   PLAIN},
  /* 30  */ {EAST_BELERIAND  ,   8,   0,   0,  20,  31,   0,   0,   PLAIN},
  /* 31  */ {EAST_BELERIAND  ,  10,  32,  30,   0,   0,   0,   0,   PLAIN},
  /* 32  */ {EAST_BELERIAND  ,  13,  43,  35,  31,  40,   0,   0,   PLAIN},
  /* 33  */ {EAST_BELERIAND  ,  17,  34,  36,   0,   0,   0,   0,   PLAIN},
  /* 34  */ {EAST_BELERIAND  ,  20,  47,   0,  33,   0,   0,   0,   PLAIN},
  /* 35  */ {THARGELION      ,  11,  36,  81,   0,  32,   0,   0,   FOREST},
  /* 36  */ {THARGELION      ,  14,   0,   0,  35,  33,   0,   0,   RIVER},
  /* 37  */ {ANDRAM          ,   7,  38,   0,  29,   0,   0,   0,   MOUNTAIN},
  /* 38  */ {ANDRAM          ,   9,  39,   0,  37,   0,   0,   0,   MOUNTAIN},
  /* 39  */ {WEST_BELERIAND  ,  11,  40,   0,  38,   0,   0,   0,   SWAMP},
  /* 40  */ {WEST_BELERIAND  ,  14,  86,  32,  39,   0,   0,   0,   MOUNTAIN},
  /* 41  */ {WEST_BELERIAND  ,  17,  42,   0,  86,   0,   0,   0,   PLAIN},
  /* 42  */ {WEST_BELERIAND  ,  19,  59,   0,  41,   0,   0,   0,   PLAIN},
  /* 43  */ {DORIATH         ,  16,  44,   0,  32,   0,   0,   0,   FOREST},
  /* 44  */ {DORIATH         ,  18,  82,   0,  43,   0,   0,   0,   FOREST},
  /* 45  */ {DORIATH         ,  20,  46,   0,  82,   0,   0,   0,   FOREST},
  /* 46  */ {DORIATH         ,  24,  49,   0,  45,   0,   0,   0,   FOREST},
  /* 47  */ {HIMLAD          ,  24,   0,   0,  34,  48,   0,   0,   PLAIN},
  /* 48  */ {HIMLAD          ,  28,   0,  47,   0,  50,   0,   0,   PLAIN},
  /* 49  */ {DOR_DINEN       ,  28,  50,   0,  46,   0,   0,   0,   PLAIN},
  /* 50  */ {DOR_DINEN       ,  32,   0,  48,  49,  51,   0,   0,   PLAIN},
  /* 51  */ {DORTHONION      ,  36,  52,  50,  95,   0,   0,   0,   MOUNTAIN},
  /* 52  */ {DORTHONION      ,  40,   0,   0,  51,  53,   0,   0,   FOREST},
  /* 53  */ {DORTHONION      ,  44,  54,  52,   0,   0,   0,   0,   FOREST},
  /* 54  */ {DORTHONION      ,  48,   0,   0,  53,  68,   0,   0,   PLAIN},
  /* 55  */ {TALATH_DIRNEN   ,  20,   0,  59,  56,   0,   0,   0,   PLAIN},
  /* 56  */ {TALATH_DIRNEN   ,  23,  55,   0,  57,   0,   0,   0,   PLAIN},
  /* 57  */ {TALATH_DIRNEN   ,  26,  56,   0,   0,  58,   0,   0,   PLAIN},
  /* 58  */ {TALATH_DIRNEN   ,  29,   0,  57,   0,   0,   0, 111,   RIVER},
  /* 59  */ {EPHEL_BRANDIR   ,   0,  60,   0,  42,  55,   0,   0,   TOWN},
  /* 60  */ {BRETHIL         ,  20,  61,   0,  59,   0,   0,   0,   FOREST},
  /* 61  */ {BRETHIL         ,  24,  62,   0,  60,   0,   0,   0,   FOREST},
  /* 62  */ {SIRION_VALE     ,  28,  63,  83,  61,   0,   0,   0,   PLAIN},
  /* 63  */ {SIRION_VALE     ,  32,  64,   0,  62,   0,   0,   0,   RIVER},
  /* 64  */ {SIRION_VALE     ,  36,  65,   0,  63,   0,   0, 124,   RIVER},
  /* 65  */ {SIRION_VALE     ,  40,  66,   0,  64,   0,   0,   0,   RIVER},
  /* 66  */ {FEN_OF_SERECH   ,  44,  67,   0,  65,   0,   0,   0,   SWAMP},
  /* 67  */ {ANFAUGLITH      ,  48,  68,   0,  66,   0,   0,   0,   DESERT},
  /* 68  */ {ANFAUGLITH      ,  52,  69,  54,  67,   0,   0,   0,   DESERT},
  /* 69  */ {ANFAUGLITH      ,  56,  70,   0,  68,   0,   0,   0,   DESERT},
  /* 70  */ {ANFAUGLITH      ,  58,   0,  77,  69,   0,   0, 140,   DESERT},
  /* 71  */ {LOTHLANN        ,  32,  72,  18,   0,   0,   0,   0,   PLAIN},
  /* 72  */ {LOTHLANN        ,  36,   0,   0,  71,  73,   0,   0,   PLAIN},
  /* 73  */ {LOTHLANN        ,  40,   0,  72,   0,  74,   0,   0,   PLAIN},
  /* 74  */ {LOTHLANN        ,  44,   0,  73,   0,  75,   0,   0,   PLAIN},
  /* 75  */ {LOTHLANN        ,  48,   0,  74,   0,  76,   0,   0,   DESERT},
  /* 76  */ {LOTHLANN        ,  52,   0,  75,   0,  77,   0,   0,   DESERT},
  /* 77  */ {LOTHLANN        ,  56,   0,  76,   0,  70,   0,   0,   DESERT},
  /* 78  */ {ANDUIN_VALE     ,   1,   0,  79,   0,   1,   0,   0,   PLAIN},
  /* 79  */ {GLADDEN_FIELDS  ,   0,   0,   0,   0,  78,   0,   0,   TOWN},
  /* 80  */ {KHAZAD_DUM      ,   0,   0,   1,   0,   2,   0,   0,   TOWN},
  /* 81  */ {BELEGOST        ,   0,  13,   6,  12,  35,   0,   0,   TOWN},
  /* 82  */ {MENEGROTH       ,   0,  45,   0,  44,   0,   0,   0,   TOWN},
  /* 83  */ {GONDOLIN        ,   0,   0,   0,   0,  62,   0,   0,   TOWN},
  /* 84  */ {UNDERWORLD      ,   0,   0,   0,   0,   0,   0,   0,   CAVE},
  /* 85  */ {MOUNTAIN_TOP    ,   0,   0,   0,   0,   0,   0,   0, MOUNTAINTOP},
  /* 86  */ {AMON_RUDH       ,  15,  41,   0,  40,   0,   0,  87,   MOUNTAIN},
  /* 87  */ {AMON_RUDH       ,  16,   0,   0,   0,   0,  86,  88,   CAVE},
  /* 88  */ {AMON_RUDH       ,  18,   0,   0,   0,   0,  87,  89,   CAVE},
  /* 89  */ {AMON_RUDH       ,  20,   0,   0,   0,   0,  88,  90,   CAVE},
  /* 90  */ {AMON_RUDH       ,  22,   0,   0,   0,   0,  89,  91,   CAVE},
  /* 91  */ {AMON_RUDH       ,  24,   0,   0,   0,   0,  90,  92,   CAVE},
  /* 92  */ {AMON_RUDH       ,  26,   0,   0,   0,   0,  91,  93,   CAVE},
  /* 93  */ {AMON_RUDH       ,  28,   0,   0,   0,   0,  92,  94,   CAVE},
  /* 94  */ {AMON_RUDH       ,  30,   0,   0,   0,   0,  93,   0,   CAVE},
  /* 95  */ {NAN_DUNGORTHEB  ,  40,   0,   0,  96,   0,   0,   0,   VALLEY},
  /* 96  */ {NAN_DUNGORTHEB  ,  42,   0,   0,  97,   0,   0,   0,   VALLEY},
  /* 97  */ {NAN_DUNGORTHEB  ,  44,   0,   0,  98,   0,   0,   0,   VALLEY},
  /* 98  */ {NAN_DUNGORTHEB  ,  46,   0,   0,  99,   0,   0,   0,   VALLEY},
  /* 99  */ {NAN_DUNGORTHEB  ,  48,   0,   0, 100,   0,   0,   0,   VALLEY},
  /* 100 */ {NAN_DUNGORTHEB  ,  50,   0,   0, 101,   0,   0,   0,   VALLEY},
  /* 101 */ {NAN_DUNGORTHEB  ,  52,   0,   0, 102,   0,   0,   0,   VALLEY},
  /* 102 */ {NAN_DUNGORTHEB  ,  54,   0,   0, 103,   0,   0,   0,   VALLEY},
  /* 103 */ {NAN_DUNGORTHEB  ,  56,   0,   0, 104,   0,   0,   0,   VALLEY},
  /* 104 */ {NAN_DUNGORTHEB  ,  58,   0,   0, 105,   0,   0,   0,   VALLEY},
  /* 105 */ {NAN_DUNGORTHEB  ,  60,   0,   0, 106,   0,   0,   0,   VALLEY},
  /* 106 */ {NAN_DUNGORTHEB  ,  62,   0,   0, 107,   0,   0,   0,   VALLEY},
  /* 107 */ {NAN_DUNGORTHEB  ,  64,   0,   0, 108,   0,   0,   0,   VALLEY},
  /* 108 */ {NAN_DUNGORTHEB  ,  66,   0,   0, 109,   0,   0,   0,   VALLEY},
  /* 109 */ {NAN_DUNGORTHEB  ,  68,   0,   0, 110,   0,   0,   0,   VALLEY},
  /* 110 */ {NAN_DUNGORTHEB  ,  70,   0,   0,  46,   0,   0,   0,   VALLEY},
  /* 111 */ {NARGOTHROND     ,  31,   0,   0,   0,   0,  58, 112,   CAVE},
  /* 112 */ {NARGOTHROND     ,  33,   0,   0,   0,   0, 111, 113,   CAVE},
  /* 113 */ {NARGOTHROND     ,  35,   0,   0,   0,   0, 112, 114,   CAVE},
  /* 114 */ {NARGOTHROND     ,  37,   0,   0,   0,   0, 113, 115,   CAVE},
  /* 115 */ {NARGOTHROND     ,  39,   0,   0,   0,   0, 114, 116,   CAVE},
  /* 116 */ {NARGOTHROND     ,  41,   0,   0,   0,   0, 115, 117,   CAVE},
  /* 117 */ {NARGOTHROND     ,  43,   0,   0,   0,   0, 116, 118,   CAVE},
  /* 118 */ {NARGOTHROND     ,  45,   0,   0,   0,   0, 117, 119,   CAVE},
  /* 119 */ {NARGOTHROND     ,  47,   0,   0,   0,   0, 118, 120,   CAVE},
  /* 120 */ {NARGOTHROND     ,  49,   0,   0,   0,   0, 119, 121,   CAVE},
  /* 121 */ {NARGOTHROND     ,  51,   0,   0,   0,   0, 120, 122,   CAVE},
  /* 122 */ {NARGOTHROND     ,  53,   0,   0,   0,   0, 121, 123,   CAVE},
  /* 123 */ {NARGOTHROND     ,  55,   0,   0,   0,   0, 122,   0,   CAVE},
  /* 124 */ {TOL_IN_GAURHOTH ,  40,   0,   0,   0,   0,  64, 125,   CAVE},
  /* 125 */ {TOL_IN_GAURHOTH ,  43,   0,   0,   0,   0, 124, 126,   CAVE},
  /* 126 */ {TOL_IN_GAURHOTH ,  46,   0,   0,   0,   0, 125, 127,   CAVE},
  /* 127 */ {TOL_IN_GAURHOTH ,  49,   0,   0,   0,   0, 126, 128,   CAVE},
  /* 128 */ {TOL_IN_GAURHOTH ,  52,   0,   0,   0,   0, 127, 129,   CAVE},
  /* 129 */ {TOL_IN_GAURHOTH ,  55,   0,   0,   0,   0, 128, 130,   CAVE},
  /* 130 */ {TOL_IN_GAURHOTH ,  58,   0,   0,   0,   0, 129, 131,   CAVE},
  /* 131 */ {TOL_IN_GAURHOTH ,  61,   0,   0,   0,   0, 130, 132,   CAVE},
  /* 132 */ {TOL_IN_GAURHOTH ,  64,   0,   0,   0,   0, 131, 133,   CAVE},
  /* 133 */ {TOL_IN_GAURHOTH ,  67,   0,   0,   0,   0, 132, 134,   CAVE},
  /* 134 */ {TOL_IN_GAURHOTH ,  70,   0,   0,   0,   0, 133, 135,   CAVE},
  /* 135 */ {TOL_IN_GAURHOTH ,  73,   0,   0,   0,   0, 134, 136,   CAVE},
  /* 136 */ {TOL_IN_GAURHOTH ,  76,   0,   0,   0,   0, 135, 137,   CAVE},
  /* 137 */ {TOL_IN_GAURHOTH ,  79,   0,   0,   0,   0, 136, 138,   CAVE},
  /* 138 */ {TOL_IN_GAURHOTH ,  82,   0,   0,   0,   0, 137, 139,   CAVE},
  /* 139 */ {TOL_IN_GAURHOTH ,  85,   0,   0,   0,   0, 138,   0,   CAVE},
  /* 140 */ {ANGBAND         ,  60,   0,   0,   0,   0,  70, 141,   CAVE},
  /* 141 */ {ANGBAND         ,  61,   0,   0,   0,   0, 140, 142,   CAVE},
  /* 142 */ {ANGBAND         ,  62,   0,   0,   0,   0, 141, 143,   CAVE},
  /* 143 */ {ANGBAND         ,  63,   0,   0,   0,   0, 142, 144,   CAVE},
  /* 144 */ {ANGBAND         ,  64,   0,   0,   0,   0, 143, 145,   CAVE},
  /* 145 */ {ANGBAND         ,  65,   0,   0,   0,   0, 144, 146,   CAVE},
  /* 146 */ {ANGBAND         ,  66,   0,   0,   0,   0, 145, 147,   CAVE},
  /* 147 */ {ANGBAND         ,  67,   0,   0,   0,   0, 146, 148,   CAVE},
  /* 148 */ {ANGBAND         ,  68,   0,   0,   0,   0, 147, 149,   CAVE},
  /* 149 */ {ANGBAND         ,  69,   0,   0,   0,   0, 148, 150,   CAVE},
  /* 150 */ {ANGBAND         ,  70,   0,   0,   0,   0, 149, 151,   CAVE},
  /* 151 */ {ANGBAND         ,  71,   0,   0,   0,   0, 150, 152,   CAVE},
  /* 152 */ {ANGBAND         ,  72,   0,   0,   0,   0, 151, 153,   CAVE},
  /* 153 */ {ANGBAND         ,  73,   0,   0,   0,   0, 152, 154,   CAVE},
  /* 154 */ {ANGBAND         ,  74,   0,   0,   0,   0, 153, 155,   CAVE},
  /* 155 */ {ANGBAND         ,  75,   0,   0,   0,   0, 154, 156,   CAVE},
  /* 156 */ {ANGBAND         ,  76,   0,   0,   0,   0, 155, 157,   CAVE},
  /* 157 */ {ANGBAND         ,  77,   0,   0,   0,   0, 156, 158,   CAVE},
  /* 158 */ {ANGBAND         ,  78,   0,   0,   0,   0, 157, 159,   CAVE},
  /* 159 */ {ANGBAND         ,  79,   0,   0,   0,   0, 158, 160,   CAVE},
  /* 160 */ {ANGBAND         ,  80,   0,   0,   0,   0, 159, 161,   CAVE},
  /* 161 */ {ANGBAND         ,  81,   0,   0,   0,   0, 160, 162,   CAVE},
  /* 162 */ {ANGBAND         ,  82,   0,   0,   0,   0, 161, 163,   CAVE},
  /* 163 */ {ANGBAND         ,  83,   0,   0,   0,   0, 162, 164,   CAVE},
  /* 164 */ {ANGBAND         ,  84,   0,   0,   0,   0, 163, 165,   CAVE},
  /* 165 */ {ANGBAND         ,  85,   0,   0,   0,   0, 164, 166,   CAVE},
  /* 166 */ {ANGBAND         ,  86,   0,   0,   0,   0, 165, 167,   CAVE},
  /* 167 */ {ANGBAND         ,  87,   0,   0,   0,   0, 166, 168,   CAVE},
  /* 168 */ {ANGBAND         ,  88,   0,   0,   0,   0, 167, 169,   CAVE},
  /* 169 */ {ANGBAND         ,  89,   0,   0,   0,   0, 168, 170,   CAVE},
  /* 170 */ {ANGBAND         ,  90,   0,   0,   0,   0, 169, 171,   CAVE},
  /* 171 */ {ANGBAND         ,  91,   0,   0,   0,   0, 170, 172,   CAVE},
  /* 172 */ {ANGBAND         ,  92,   0,   0,   0,   0, 171, 173,   CAVE},
  /* 173 */ {ANGBAND         ,  93,   0,   0,   0,   0, 172, 174,   CAVE},
  /* 174 */ {ANGBAND         ,  94,   0,   0,   0,   0, 173, 175,   CAVE},
  /* 175 */ {ANGBAND         ,  95,   0,   0,   0,   0, 174, 176,   CAVE},
  /* 176 */ {ANGBAND         ,  96,   0,   0,   0,   0, 175, 177,   CAVE},
  /* 177 */ {ANGBAND         ,  97,   0,   0,   0,   0, 176, 178,   CAVE},
  /* 178 */ {ANGBAND         ,  98,   0,   0,   0,   0, 177, 179,   CAVE},
  /* 179 */ {ANGBAND         ,  99,   0,   0,   0,   0, 178, 180,   CAVE},
  /* 180 */ {ANGBAND         , 100,   0,   0,   0,   0, 179, 181,   CAVE},
  /* 181 */ {ANGBAND         , 101,   0,   0,   0,   0, 180, 182,   CAVE},
  /* 182 */ {ANGBAND         , 102,   0,   0,   0,   0, 181, 183,   CAVE},
  /* 183 */ {ANGBAND         , 103,   0,   0,   0,   0, 182, 184,   CAVE},
  /* 184 */ {ANGBAND         , 104,   0,   0,   0,   0, 183, 185,   CAVE},
  /* 185 */ {ANGBAND         , 105,   0,   0,   0,   0, 184, 186,   CAVE},
  /* 186 */ {ANGBAND         , 106,   0,   0,   0,   0, 185, 187,   CAVE},
  /* 187 */ {ANGBAND         , 107,   0,   0,   0,   0, 186, 188,   CAVE},
  /* 188 */ {ANGBAND         , 108,   0,   0,   0,   0, 187, 189,   CAVE},
  /* 189 */ {ANGBAND         , 109,   0,   0,   0,   0, 188, 190,   CAVE},
  /* 190 */ {ANGBAND         , 110,   0,   0,   0,   0, 189, 191,   CAVE},
  /* 191 */ {ANGBAND         , 111,   0,   0,   0,   0, 190, 192,   CAVE},
  /* 192 */ {ANGBAND         , 112,   0,   0,   0,   0, 191, 193,   CAVE},
  /* 193 */ {ANGBAND         , 113,   0,   0,   0,   0, 192, 194,   CAVE},
  /* 194 */ {ANGBAND         , 114,   0,   0,   0,   0, 193, 195,   CAVE},
  /* 195 */ {ANGBAND         , 115,   0,   0,   0,   0, 194, 196,   CAVE},
  /* 196 */ {ANGBAND         , 116,   0,   0,   0,   0, 195, 197,   CAVE},
  /* 197 */ {ANGBAND         , 117,   0,   0,   0,   0, 196, 198,   CAVE},
  /* 198 */ {ANGBAND         , 118,   0,   0,   0,   0, 197, 199,   CAVE},
  /* 199 */ {ANGBAND         , 119,   0,   0,   0,   0, 198, 200,   CAVE},
  /* 200 */ {ANGBAND         , 120,   0,   0,   0,   0, 199, 201,   CAVE},
  /* 201 */ {ANGBAND         , 121,   0,   0,   0,   0, 200, 202,   CAVE},
  /* 202 */ {ANGBAND         , 122,   0,   0,   0,   0, 201, 203,   CAVE},
  /* 203 */ {ANGBAND         , 123,   0,   0,   0,   0, 202, 204,   CAVE},
  /* 204 */ {ANGBAND         , 124,   0,   0,   0,   0, 203, 205,   CAVE},
  /* 205 */ {ANGBAND         , 125,   0,   0,   0,   0, 204, 206,   CAVE},
  /* 206 */ {ANGBAND         , 126,   0,   0,   0,   0, 205, 207,   CAVE},
  /* 207 */ {ANGBAND         , 127,   0,   0,   0,   0, 206,   0,   CAVE},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 000 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0}};


/**
 * List of stages for dungeon only mode
 */

int dungeon_map[NUM_STAGES][9] =
  {
  /* num     locality          lev    N    E    S    W    U    D    type */
  /* 0   */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 1   */ {KHAZAD_DUM      ,   0,   0,   0,   0,   0,   0,   2,   TOWN},
  /* 2   */ {HITHAEGLIR      ,   1,   0,   0,   0,   0,   1,   3,   CAVE},
  /* 3   */ {HITHAEGLIR      ,   2,   0,   0,   0,   0,   2,   4,   CAVE},
  /* 4   */ {HITHAEGLIR      ,   3,   0,   0,   0,   0,   3,   5,   CAVE},
  /* 5   */ {HITHAEGLIR      ,   4,   0,   0,   0,   0,   4,   6,   CAVE},
  /* 6   */ {HITHAEGLIR      ,   5,   0,   0,   0,   0,   5,   7,   CAVE},
  /* 7   */ {HITHAEGLIR      ,   6,   0,   0,   0,   0,   6,   8,   CAVE},
  /* 8   */ {HITHAEGLIR      ,   7,   0,   0,   0,   0,   7,   9,   CAVE},
  /* 9   */ {HITHAEGLIR      ,   8,   0,   0,   0,   0,   8,  10,   CAVE},
  /* 10  */ {HITHAEGLIR      ,   9,   0,   0,   0,   0,   9,  11,   CAVE},
  /* 11  */ {HITHAEGLIR      ,  10,   0,   0,   0,   0,  10,  12,   CAVE},
  /* 12  */ {HITHAEGLIR      ,  11,   0,   0,   0,   0,  11,  13,   CAVE},
  /* 13  */ {HITHAEGLIR      ,  12,   0,   0,   0,   0,  12,  14,   CAVE},
  /* 14  */ {HITHAEGLIR      ,  13,   0,   0,   0,   0,  13,  15,   CAVE},
  /* 15  */ {HITHAEGLIR      ,  14,   0,   0,   0,   0,  14,  16,   CAVE},
  /* 16  */ {HITHAEGLIR      ,  15,   0,   0,   0,   0,  15,  17,   CAVE},
  /* 17  */ {AMON_RUDH       ,  16,   0,   0,   0,   0,  16,  18,   CAVE},
  /* 18  */ {AMON_RUDH       ,  17,   0,   0,   0,   0,  17,  19,   CAVE},
  /* 19  */ {AMON_RUDH       ,  18,   0,   0,   0,   0,  18,  20,   CAVE},
  /* 20  */ {AMON_RUDH       ,  19,   0,   0,   0,   0,  19,  21,   CAVE},
  /* 21  */ {AMON_RUDH       ,  20,   0,   0,   0,   0,  20,  22,   CAVE},
  /* 22  */ {AMON_RUDH       ,  21,   0,   0,   0,   0,  21,  23,   CAVE},
  /* 23  */ {AMON_RUDH       ,  22,   0,   0,   0,   0,  22,  24,   CAVE},
  /* 24  */ {AMON_RUDH       ,  23,   0,   0,   0,   0,  23,  25,   CAVE},
  /* 25  */ {AMON_RUDH       ,  24,   0,   0,   0,   0,  24,  26,   CAVE},
  /* 26  */ {AMON_RUDH       ,  25,   0,   0,   0,   0,  25,  27,   CAVE},
  /* 27  */ {AMON_RUDH       ,  26,   0,   0,   0,   0,  26,  28,   CAVE},
  /* 28  */ {AMON_RUDH       ,  27,   0,   0,   0,   0,  27,  29,   CAVE},
  /* 29  */ {AMON_RUDH       ,  28,   0,   0,   0,   0,  28,  30,   CAVE},
  /* 30  */ {AMON_RUDH       ,  29,   0,   0,   0,   0,  29,  31,   CAVE},
  /* 31  */ {AMON_RUDH       ,  30,   0,   0,   0,   0,  30,  32,   CAVE},
  /* 32  */ {NARGOTHROND     ,  31,   0,   0,   0,   0,   0,  33,   CAVE},
  /* 33  */ {NARGOTHROND     ,  32,   0,   0,   0,   0,  32,  34,   CAVE},
  /* 34  */ {NARGOTHROND     ,  33,   0,   0,   0,   0,  33,  35,   CAVE},
  /* 35  */ {NARGOTHROND     ,  34,   0,   0,   0,   0,  34,  36,   CAVE},
  /* 36  */ {NARGOTHROND     ,  35,   0,   0,   0,   0,  35,  37,   CAVE},
  /* 37  */ {NARGOTHROND     ,  36,   0,   0,   0,   0,  36,  38,   CAVE},
  /* 38  */ {NARGOTHROND     ,  37,   0,   0,   0,   0,  37,  39,   CAVE},
  /* 39  */ {NARGOTHROND     ,  38,   0,   0,   0,   0,  38,  40,   CAVE},
  /* 40  */ {NARGOTHROND     ,  39,   0,   0,   0,   0,  39,  41,   CAVE},
  /* 41  */ {NARGOTHROND     ,  40,   0,   0,   0,   0,  40,  42,   CAVE},
  /* 42  */ {NARGOTHROND     ,  41,   0,   0,   0,   0,  41,  43,   CAVE},
  /* 43  */ {NARGOTHROND     ,  42,   0,   0,   0,   0,  42,  44,   CAVE},
  /* 44  */ {NARGOTHROND     ,  43,   0,   0,   0,   0,  43,  45,   CAVE},
  /* 45  */ {NARGOTHROND     ,  44,   0,   0,   0,   0,  44,  46,   CAVE},
  /* 46  */ {NARGOTHROND     ,  45,   0,   0,   0,   0,  45,  47,   CAVE},
  /* 47  */ {NARGOTHROND     ,  46,   0,   0,   0,   0,  46,  48,   CAVE},
  /* 48  */ {NARGOTHROND     ,  47,   0,   0,   0,   0,  47,  49,   CAVE},
  /* 49  */ {NARGOTHROND     ,  48,   0,   0,   0,   0,  48,  50,   CAVE},
  /* 50  */ {NARGOTHROND     ,  49,   0,   0,   0,   0,  49,  51,   CAVE},
  /* 51  */ {NARGOTHROND     ,  50,   0,   0,   0,   0,  50,  52,   CAVE},
  /* 52  */ {NARGOTHROND     ,  51,   0,   0,   0,   0,  51,  53,   CAVE},
  /* 53  */ {NARGOTHROND     ,  52,   0,   0,   0,   0,  52,  54,   CAVE},
  /* 54  */ {NARGOTHROND     ,  53,   0,   0,   0,   0,  53,  55,   CAVE},
  /* 55  */ {NARGOTHROND     ,  54,   0,   0,   0,   0,  54,  56,   CAVE},
  /* 56  */ {NARGOTHROND     ,  55,   0,   0,   0,   0,  55,  57,   CAVE},
  /* 57  */ {NAN_DUNGORTHEB  ,  56,   0,   0,  58,   0,   0,   0,   VALLEY},
  /* 58  */ {NAN_DUNGORTHEB  ,  57,   0,   0,  59,   0,   0,   0,   VALLEY},
  /* 59  */ {NAN_DUNGORTHEB  ,  58,   0,   0,  60,   0,   0,   0,   VALLEY},
  /* 60  */ {NAN_DUNGORTHEB  ,  59,   0,   0,  61,   0,   0,   0,   VALLEY},
  /* 61  */ {NAN_DUNGORTHEB  ,  60,   0,   0,  62,   0,   0,   0,   VALLEY},
  /* 62  */ {NAN_DUNGORTHEB  ,  61,   0,   0,  63,   0,   0,   0,   VALLEY},
  /* 63  */ {NAN_DUNGORTHEB  ,  62,   0,   0,  64,   0,   0,   0,   VALLEY},
  /* 64  */ {NAN_DUNGORTHEB  ,  63,   0,   0,  65,   0,   0,   0,   VALLEY},
  /* 65  */ {NAN_DUNGORTHEB  ,  64,   0,   0,  66,   0,   0,   0,   VALLEY},
  /* 66  */ {NAN_DUNGORTHEB  ,  65,   0,   0,  67,   0,   0,   0,   VALLEY},
  /* 67  */ {NAN_DUNGORTHEB  ,  66,   0,   0,  68,   0,   0,   0,   VALLEY},
  /* 68  */ {NAN_DUNGORTHEB  ,  67,   0,   0,  69,   0,   0,   0,   VALLEY},
  /* 69  */ {NAN_DUNGORTHEB  ,  68,   0,   0,  70,   0,   0,   0,   VALLEY},
  /* 70  */ {NAN_DUNGORTHEB  ,  69,   0,   0,  71,   0,   0,   0,   VALLEY},
  /* 71  */ {NAN_DUNGORTHEB  ,  70,   0,   0,   0,   0,   0,  72,   VALLEY},
  /* 72  */ {TOL_IN_GAURHOTH ,  71,   0,   0,   0,   0,   0,  73,   CAVE},
  /* 73  */ {TOL_IN_GAURHOTH ,  72,   0,   0,   0,   0,  72,  74,   CAVE},
  /* 74  */ {TOL_IN_GAURHOTH ,  73,   0,   0,   0,   0,  73,  75,   CAVE},
  /* 75  */ {TOL_IN_GAURHOTH ,  74,   0,   0,   0,   0,  74,  76,   CAVE},
  /* 76  */ {TOL_IN_GAURHOTH ,  75,   0,   0,   0,   0,  75,  77,   CAVE},
  /* 77  */ {TOL_IN_GAURHOTH ,  76,   0,   0,   0,   0,  76,  78,   CAVE},
  /* 78  */ {TOL_IN_GAURHOTH ,  77,   0,   0,   0,   0,  77,  79,   CAVE},
  /* 79  */ {TOL_IN_GAURHOTH ,  78,   0,   0,   0,   0,  78,  80,   CAVE},
  /* 80  */ {TOL_IN_GAURHOTH ,  79,   0,   0,   0,   0,  79,  81,   CAVE},
  /* 81  */ {TOL_IN_GAURHOTH ,  80,   0,   0,   0,   0,  80,  82,   CAVE},
  /* 82  */ {TOL_IN_GAURHOTH ,  81,   0,   0,   0,   0,  81,  83,   CAVE},
  /* 83  */ {TOL_IN_GAURHOTH ,  82,   0,   0,   0,   0,  82,  84,   CAVE},
  /* 84  */ {TOL_IN_GAURHOTH ,  83,   0,   0,   0,   0,  83,  85,   CAVE},
  /* 85  */ {TOL_IN_GAURHOTH ,  84,   0,   0,   0,   0,  84,  86,   CAVE},
  /* 86  */ {TOL_IN_GAURHOTH ,  85,   0,   0,   0,   0,  85,  87,   CAVE},
  /* 87  */ {ANGBAND         ,  86,   0,   0,   0,   0,   0,  88,   CAVE},
  /* 88  */ {ANGBAND         ,  87,   0,   0,   0,   0,  87,  89,   CAVE},
  /* 89  */ {ANGBAND         ,  88,   0,   0,   0,   0,  88,  90,   CAVE},
  /* 90  */ {ANGBAND         ,  89,   0,   0,   0,   0,  89,  91,   CAVE},
  /* 91  */ {ANGBAND         ,  90,   0,   0,   0,   0,  90,  92,   CAVE},
  /* 92  */ {ANGBAND         ,  91,   0,   0,   0,   0,  91,  93,   CAVE},
  /* 93  */ {ANGBAND         ,  92,   0,   0,   0,   0,  92,  94,   CAVE},
  /* 94  */ {ANGBAND         ,  93,   0,   0,   0,   0,  93,  95,   CAVE},
  /* 95  */ {ANGBAND         ,  94,   0,   0,   0,   0,  94,  96,   CAVE},
  /* 96  */ {ANGBAND         ,  95,   0,   0,   0,   0,  95,  97,   CAVE},
  /* 97  */ {ANGBAND         ,  96,   0,   0,   0,   0,  96,  98,   CAVE},
  /* 98  */ {ANGBAND         ,  97,   0,   0,   0,   0,  97,  99,   CAVE},
  /* 99  */ {ANGBAND         ,  98,   0,   0,   0,   0,  98, 100,   CAVE},
  /* 100 */ {ANGBAND         ,  99,   0,   0,   0,   0,  99, 101,   CAVE},
  /* 101 */ {ANGBAND         , 100,   0,   0,   0,   0, 100, 102,   CAVE},
  /* 102 */ {ANGBAND         , 101,   0,   0,   0,   0, 101, 103,   CAVE},
  /* 103 */ {ANGBAND         , 102,   0,   0,   0,   0, 102, 104,   CAVE},
  /* 104 */ {ANGBAND         , 103,   0,   0,   0,   0, 103, 105,   CAVE},
  /* 105 */ {ANGBAND         , 104,   0,   0,   0,   0, 104, 106,   CAVE},
  /* 106 */ {ANGBAND         , 105,   0,   0,   0,   0, 105, 107,   CAVE},
  /* 107 */ {ANGBAND         , 106,   0,   0,   0,   0, 106, 108,   CAVE},
  /* 108 */ {ANGBAND         , 107,   0,   0,   0,   0, 107, 109,   CAVE},
  /* 109 */ {ANGBAND         , 108,   0,   0,   0,   0, 108, 110,   CAVE},
  /* 110 */ {ANGBAND         , 109,   0,   0,   0,   0, 109, 111,   CAVE},
  /* 111 */ {ANGBAND         , 110,   0,   0,   0,   0, 110, 112,   CAVE},
  /* 112 */ {ANGBAND         , 111,   0,   0,   0,   0, 111, 113,   CAVE},
  /* 113 */ {ANGBAND         , 112,   0,   0,   0,   0, 112, 114,   CAVE},
  /* 114 */ {ANGBAND         , 113,   0,   0,   0,   0, 113, 115,   CAVE},
  /* 115 */ {ANGBAND         , 114,   0,   0,   0,   0, 114, 116,   CAVE},
  /* 116 */ {ANGBAND         , 115,   0,   0,   0,   0, 115, 117,   CAVE},
  /* 117 */ {ANGBAND         , 116,   0,   0,   0,   0, 116, 118,   CAVE},
  /* 118 */ {ANGBAND         , 117,   0,   0,   0,   0, 117, 119,   CAVE},
  /* 119 */ {ANGBAND         , 118,   0,   0,   0,   0, 118, 120,   CAVE},
  /* 120 */ {ANGBAND         , 119,   0,   0,   0,   0, 119, 121,   CAVE},
  /* 121 */ {ANGBAND         , 120,   0,   0,   0,   0, 120, 122,   CAVE},
  /* 122 */ {ANGBAND         , 121,   0,   0,   0,   0, 121, 123,   CAVE},
  /* 123 */ {ANGBAND         , 122,   0,   0,   0,   0, 122, 124,   CAVE},
  /* 124 */ {ANGBAND         , 123,   0,   0,   0,   0, 123, 125,   CAVE},
  /* 125 */ {ANGBAND         , 124,   0,   0,   0,   0, 124, 126,   CAVE},
  /* 126 */ {ANGBAND         , 125,   0,   0,   0,   0, 125, 127,   CAVE},
  /* 127 */ {ANGBAND         , 126,   0,   0,   0,   0, 126, 128,   CAVE},
  /* 128 */ {ANGBAND         , 127,   0,   0,   0,   0, 127,   0,   CAVE},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0}
  };

/**
 * List of stages for dungeon only mode
 */

int fanilla_map[NUM_STAGES][9] =
  {
  /* num     locality          lev    N    E    S    W    U    D    type */
  /* 0   */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 1   */ {ANGBAND         ,   0,   0,   0,   0,   0,   0,   2,   TOWN},
  /* 2   */ {ANGBAND         ,   1,   0,   0,   0,   0,   1,   3,   CAVE},
  /* 3   */ {ANGBAND         ,   2,   0,   0,   0,   0,   2,   4,   CAVE},
  /* 4   */ {ANGBAND         ,   3,   0,   0,   0,   0,   3,   5,   CAVE},
  /* 5   */ {ANGBAND         ,   4,   0,   0,   0,   0,   4,   6,   CAVE},
  /* 6   */ {ANGBAND         ,   5,   0,   0,   0,   0,   5,   7,   CAVE},
  /* 7   */ {ANGBAND         ,   6,   0,   0,   0,   0,   6,   8,   CAVE},
  /* 8   */ {ANGBAND         ,   7,   0,   0,   0,   0,   7,   9,   CAVE},
  /* 9   */ {ANGBAND         ,   8,   0,   0,   0,   0,   8,  10,   CAVE},
  /* 10  */ {ANGBAND         ,   9,   0,   0,   0,   0,   9,  11,   CAVE},
  /* 11  */ {ANGBAND         ,  10,   0,   0,   0,   0,  10,  12,   CAVE},
  /* 12  */ {ANGBAND         ,  11,   0,   0,   0,   0,  11,  13,   CAVE},
  /* 13  */ {ANGBAND         ,  12,   0,   0,   0,   0,  12,  14,   CAVE},
  /* 14  */ {ANGBAND         ,  13,   0,   0,   0,   0,  13,  15,   CAVE},
  /* 15  */ {ANGBAND         ,  14,   0,   0,   0,   0,  14,  16,   CAVE},
  /* 16  */ {ANGBAND         ,  15,   0,   0,   0,   0,  15,  17,   CAVE},
  /* 17  */ {ANGBAND         ,  16,   0,   0,   0,   0,  16,  18,   CAVE},
  /* 18  */ {ANGBAND         ,  17,   0,   0,   0,   0,  17,  19,   CAVE},
  /* 19  */ {ANGBAND         ,  18,   0,   0,   0,   0,  18,  20,   CAVE},
  /* 20  */ {ANGBAND         ,  19,   0,   0,   0,   0,  19,  21,   CAVE},
  /* 21  */ {ANGBAND         ,  20,   0,   0,   0,   0,  20,  22,   CAVE},
  /* 22  */ {ANGBAND         ,  21,   0,   0,   0,   0,  21,  23,   CAVE},
  /* 23  */ {ANGBAND         ,  22,   0,   0,   0,   0,  22,  24,   CAVE},
  /* 24  */ {ANGBAND         ,  23,   0,   0,   0,   0,  23,  25,   CAVE},
  /* 25  */ {ANGBAND         ,  24,   0,   0,   0,   0,  24,  26,   CAVE},
  /* 26  */ {ANGBAND         ,  25,   0,   0,   0,   0,  25,  27,   CAVE},
  /* 27  */ {ANGBAND         ,  26,   0,   0,   0,   0,  26,  28,   CAVE},
  /* 28  */ {ANGBAND         ,  27,   0,   0,   0,   0,  27,  29,   CAVE},
  /* 29  */ {ANGBAND         ,  28,   0,   0,   0,   0,  28,  30,   CAVE},
  /* 30  */ {ANGBAND         ,  29,   0,   0,   0,   0,  29,  31,   CAVE},
  /* 31  */ {ANGBAND         ,  30,   0,   0,   0,   0,  30,  32,   CAVE},
  /* 32  */ {ANGBAND         ,  31,   0,   0,   0,   0,   0,  33,   CAVE},
  /* 33  */ {ANGBAND         ,  32,   0,   0,   0,   0,  32,  34,   CAVE},
  /* 34  */ {ANGBAND         ,  33,   0,   0,   0,   0,  33,  35,   CAVE},
  /* 35  */ {ANGBAND         ,  34,   0,   0,   0,   0,  34,  36,   CAVE},
  /* 36  */ {ANGBAND         ,  35,   0,   0,   0,   0,  35,  37,   CAVE},
  /* 37  */ {ANGBAND         ,  36,   0,   0,   0,   0,  36,  38,   CAVE},
  /* 38  */ {ANGBAND         ,  37,   0,   0,   0,   0,  37,  39,   CAVE},
  /* 39  */ {ANGBAND         ,  38,   0,   0,   0,   0,  38,  40,   CAVE},
  /* 40  */ {ANGBAND         ,  39,   0,   0,   0,   0,  39,  41,   CAVE},
  /* 41  */ {ANGBAND         ,  40,   0,   0,   0,   0,  40,  42,   CAVE},
  /* 42  */ {ANGBAND         ,  41,   0,   0,   0,   0,  41,  43,   CAVE},
  /* 43  */ {ANGBAND         ,  42,   0,   0,   0,   0,  42,  44,   CAVE},
  /* 44  */ {ANGBAND         ,  43,   0,   0,   0,   0,  43,  45,   CAVE},
  /* 45  */ {ANGBAND         ,  44,   0,   0,   0,   0,  44,  46,   CAVE},
  /* 46  */ {ANGBAND         ,  45,   0,   0,   0,   0,  45,  47,   CAVE},
  /* 47  */ {ANGBAND         ,  46,   0,   0,   0,   0,  46,  48,   CAVE},
  /* 48  */ {ANGBAND         ,  47,   0,   0,   0,   0,  47,  49,   CAVE},
  /* 49  */ {ANGBAND         ,  48,   0,   0,   0,   0,  48,  50,   CAVE},
  /* 50  */ {ANGBAND         ,  49,   0,   0,   0,   0,  49,  51,   CAVE},
  /* 51  */ {ANGBAND         ,  50,   0,   0,   0,   0,  50,  52,   CAVE},
  /* 52  */ {ANGBAND         ,  51,   0,   0,   0,   0,  51,  53,   CAVE},
  /* 53  */ {ANGBAND         ,  52,   0,   0,   0,   0,  52,  54,   CAVE},
  /* 54  */ {ANGBAND         ,  53,   0,   0,   0,   0,  53,  55,   CAVE},
  /* 55  */ {ANGBAND         ,  54,   0,   0,   0,   0,  54,  56,   CAVE},
  /* 56  */ {ANGBAND         ,  55,   0,   0,   0,   0,  55,  57,   CAVE},
  /* 57  */ {ANGBAND         ,  56,   0,   0,   0,   0,  56,  58,   CAVE},
  /* 58  */ {ANGBAND         ,  57,   0,   0,   0,   0,  57,  59,   CAVE},
  /* 59  */ {ANGBAND         ,  58,   0,   0,   0,   0,  58,  60,   CAVE},
  /* 60  */ {ANGBAND         ,  59,   0,   0,   0,   0,  59,  61,   CAVE},
  /* 61  */ {ANGBAND         ,  60,   0,   0,   0,   0,  60,  62,   CAVE},
  /* 62  */ {ANGBAND         ,  61,   0,   0,   0,   0,  61,  63,   CAVE},
  /* 63  */ {ANGBAND         ,  62,   0,   0,   0,   0,  62,  64,   CAVE},
  /* 64  */ {ANGBAND         ,  63,   0,   0,   0,   0,  63,  65,   CAVE},
  /* 65  */ {ANGBAND         ,  64,   0,   0,   0,   0,  64,  66,   CAVE},
  /* 66  */ {ANGBAND         ,  65,   0,   0,   0,   0,  65,  67,   CAVE},
  /* 67  */ {ANGBAND         ,  66,   0,   0,   0,   0,  66,  68,   CAVE},
  /* 68  */ {ANGBAND         ,  67,   0,   0,   0,   0,  67,  69,   CAVE},
  /* 69  */ {ANGBAND         ,  68,   0,   0,   0,   0,  68,  70,   CAVE},
  /* 70  */ {ANGBAND         ,  69,   0,   0,   0,   0,  69,  71,   CAVE},
  /* 71  */ {ANGBAND         ,  70,   0,   0,   0,   0,  70,  72,   CAVE},
  /* 72  */ {ANGBAND         ,  71,   0,   0,   0,   0,  71,  73,   CAVE},
  /* 73  */ {ANGBAND         ,  72,   0,   0,   0,   0,  72,  74,   CAVE},
  /* 74  */ {ANGBAND         ,  73,   0,   0,   0,   0,  73,  75,   CAVE},
  /* 75  */ {ANGBAND         ,  74,   0,   0,   0,   0,  74,  76,   CAVE},
  /* 76  */ {ANGBAND         ,  75,   0,   0,   0,   0,  75,  77,   CAVE},
  /* 77  */ {ANGBAND         ,  76,   0,   0,   0,   0,  76,  78,   CAVE},
  /* 78  */ {ANGBAND         ,  77,   0,   0,   0,   0,  77,  79,   CAVE},
  /* 79  */ {ANGBAND         ,  78,   0,   0,   0,   0,  78,  80,   CAVE},
  /* 80  */ {ANGBAND         ,  79,   0,   0,   0,   0,  79,  81,   CAVE},
  /* 81  */ {ANGBAND         ,  80,   0,   0,   0,   0,  80,  82,   CAVE},
  /* 82  */ {ANGBAND         ,  81,   0,   0,   0,   0,  81,  83,   CAVE},
  /* 83  */ {ANGBAND         ,  82,   0,   0,   0,   0,  82,  84,   CAVE},
  /* 84  */ {ANGBAND         ,  83,   0,   0,   0,   0,  83,  85,   CAVE},
  /* 85  */ {ANGBAND         ,  84,   0,   0,   0,   0,  84,  86,   CAVE},
  /* 86  */ {ANGBAND         ,  85,   0,   0,   0,   0,  85,  87,   CAVE},
  /* 87  */ {ANGBAND         ,  86,   0,   0,   0,   0,   0,  88,   CAVE},
  /* 88  */ {ANGBAND         ,  87,   0,   0,   0,   0,  87,  89,   CAVE},
  /* 89  */ {ANGBAND         ,  88,   0,   0,   0,   0,  88,  90,   CAVE},
  /* 90  */ {ANGBAND         ,  89,   0,   0,   0,   0,  89,  91,   CAVE},
  /* 91  */ {ANGBAND         ,  90,   0,   0,   0,   0,  90,  92,   CAVE},
  /* 92  */ {ANGBAND         ,  91,   0,   0,   0,   0,  91,  93,   CAVE},
  /* 93  */ {ANGBAND         ,  92,   0,   0,   0,   0,  92,  94,   CAVE},
  /* 94  */ {ANGBAND         ,  93,   0,   0,   0,   0,  93,  95,   CAVE},
  /* 95  */ {ANGBAND         ,  94,   0,   0,   0,   0,  94,  96,   CAVE},
  /* 96  */ {ANGBAND         ,  95,   0,   0,   0,   0,  95,  97,   CAVE},
  /* 97  */ {ANGBAND         ,  96,   0,   0,   0,   0,  96,  98,   CAVE},
  /* 98  */ {ANGBAND         ,  97,   0,   0,   0,   0,  97,  99,   CAVE},
  /* 99  */ {ANGBAND         ,  98,   0,   0,   0,   0,  98, 100,   CAVE},
  /* 100 */ {ANGBAND         ,  99,   0,   0,   0,   0,  99, 101,   CAVE},
  /* 101 */ {ANGBAND         , 100,   0,   0,   0,   0, 100, 102,   CAVE},
  /* 102 */ {ANGBAND         , 101,   0,   0,   0,   0, 101, 103,   CAVE},
  /* 103 */ {ANGBAND         , 102,   0,   0,   0,   0, 102, 104,   CAVE},
  /* 104 */ {ANGBAND         , 103,   0,   0,   0,   0, 103, 105,   CAVE},
  /* 105 */ {ANGBAND         , 104,   0,   0,   0,   0, 104, 106,   CAVE},
  /* 106 */ {ANGBAND         , 105,   0,   0,   0,   0, 105, 107,   CAVE},
  /* 107 */ {ANGBAND         , 106,   0,   0,   0,   0, 106, 108,   CAVE},
  /* 108 */ {ANGBAND         , 107,   0,   0,   0,   0, 107, 109,   CAVE},
  /* 109 */ {ANGBAND         , 108,   0,   0,   0,   0, 108, 110,   CAVE},
  /* 110 */ {ANGBAND         , 109,   0,   0,   0,   0, 109, 111,   CAVE},
  /* 111 */ {ANGBAND         , 110,   0,   0,   0,   0, 110, 112,   CAVE},
  /* 112 */ {ANGBAND         , 111,   0,   0,   0,   0, 111, 113,   CAVE},
  /* 113 */ {ANGBAND         , 112,   0,   0,   0,   0, 112, 114,   CAVE},
  /* 114 */ {ANGBAND         , 113,   0,   0,   0,   0, 113, 115,   CAVE},
  /* 115 */ {ANGBAND         , 114,   0,   0,   0,   0, 114, 116,   CAVE},
  /* 116 */ {ANGBAND         , 115,   0,   0,   0,   0, 115, 117,   CAVE},
  /* 117 */ {ANGBAND         , 116,   0,   0,   0,   0, 116, 118,   CAVE},
  /* 118 */ {ANGBAND         , 117,   0,   0,   0,   0, 117, 119,   CAVE},
  /* 119 */ {ANGBAND         , 118,   0,   0,   0,   0, 118, 120,   CAVE},
  /* 120 */ {ANGBAND         , 119,   0,   0,   0,   0, 119, 121,   CAVE},
  /* 121 */ {ANGBAND         , 120,   0,   0,   0,   0, 120, 122,   CAVE},
  /* 122 */ {ANGBAND         , 121,   0,   0,   0,   0, 121, 123,   CAVE},
  /* 123 */ {ANGBAND         , 122,   0,   0,   0,   0, 122, 124,   CAVE},
  /* 124 */ {ANGBAND         , 123,   0,   0,   0,   0, 123, 125,   CAVE},
  /* 125 */ {ANGBAND         , 124,   0,   0,   0,   0, 124, 126,   CAVE},
  /* 126 */ {ANGBAND         , 125,   0,   0,   0,   0, 125, 127,   CAVE},
  /* 127 */ {ANGBAND         , 126,   0,   0,   0,   0, 126, 128,   CAVE},
  /* 128 */ {ANGBAND         , 127,   0,   0,   0,   0, 127,   0,   CAVE},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 320 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 321 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 322 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 323 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 324 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 325 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 326 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 327 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 329 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 330 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 331 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 332 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 333 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 334 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 335 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 336 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 337 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 338 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 319 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0},
  /* 328 */ {NOWHERE         ,   0,   0,   0,   0,   0,   0,   0,   0}
  };

/**
 * Names of localities 
 */

const char *locality_name[MAX_LOCALITIES] = 
  {
    "Nowhere",
    "Hithaeglir",
    "Eriador",
    "Ered Luin",
    "Ered Luin South",
    "Ossiriand",
    "Taur-Im-Duinath",
    "East Beleriand",
    "Andram",
    "West Beleriand",
    "Thargelion",
    "Doriath",
    "Himlad",
    "Dor Dínen",
    "Dorthonion",
    "Talath Dirnen",
    "Brethil",
    "Sirion Vale",
    "Fen of Serech",
    "Anfauglith",
    "Lothlann",
    "Amon Rûdh",
    "Nan Dungortheb",
    "Nargothrond",
    "Tol-In-Gaurhoth",
    "Angband",
    "Anduin Vale",
    "Gladden Fields",
    "Khazad Dûm",
    "Belegost",
    "Menegroth",
    "Ephel Brandir",
    "Gondolin",
    "Ent Path",
    "Eriador South",
    "Underworld",
    "Mountain Top"
  };


/**
 * Names of localities 
 */

const char *short_locality_name[MAX_LOCALITIES] = 
  {
    "Nowhere",
    "Hith",
    "Eriad",
    "ErLu",
    "ErLuS",
    "Ossir",
    "T-I-D",
    "EBel",
    "Andr",
    "WBel",
    "Tharg",
    "Drath",
    "Hmld",
    "DrDín",
    "Drthn",
    "TDirn",
    "Breth",
    "SVale",
    "FenSh",
    "Anfau",
    "Lothl",
    "ARûdh",
    "NDng",
    "Narg",
    "T-I-G",
    "Ang",
    "AVale",
    "GladFlds",
    "KhazdDûm",
    "Belegost",
    "Menegrth",
    "EBrandir",
    "Gondolin",
    "EntP",
    "EriaS",
    "Uwrld",
    "MtnTp"
  };


/**
 * Stage numbers of towns 
 */
int towns[10] = {6, 30, 37, 44, 115, 150, 151, 152, 153, 154};
int extended_towns[10] = {6, 30, 37, 44, 115, 150, 151, 152, 153, 154};
int compressed_towns[10] = {5, 20, 24, 27, 59, 79, 80, 81, 82, 83};


/**
 * Probabilities of races appearing in the respective towns 
 */

int race_town_prob[10][14] = 
  {
    /* Eriador         */ {60, 0, 0, 7, 0, 4, 2, 5, 4, 0, 5, 7, 3, 2},
    /* Ossiriand       */ {0, 75, 7, 0, 0, 0, 0, 0, 0, 2, 4, 7, 5, 0},
    /* Ered Luin South */ {2, 0, 0, 0, 90, 2, 2, 2, 0, 0, 2, 0, 0, 0},
    /* Taur-Im-Duinath */ {0, 5, 5, 0, 0, 0, 2, 0, 2, 1, 5, 40, 40, 0},
    /* Ephel Brandir   */ {7, 0, 2, 1, 0, 0, 40, 0, 40, 2, 2, 2, 2, 2},
    /* Gladden Fields  */ {5, 0, 0, 25, 0, 0, 5, 5, 5, 0, 25, 5, 0, 25},
    /* Khazad Dum      */ {0, 0, 0, 0, 5, 20, 0, 70, 0, 0, 5, 0, 0, 0},
    /* Belegost        */ {0, 0, 0, 0, 5, 70, 0, 20, 0, 0, 5, 0, 0, 0},
    /* Menegroth       */ {0, 5, 70, 0, 0, 0, 5, 0, 5, 5, 5, 0, 5, 0},
    /* Gondolin        */ {0, 5, 20, 0, 0, 0, 0, 0, 0, 75, 0, 0, 0, 0}};

/**
 * Store types by index 
 */

byte type_of_store[MAX_STORES] = 
  {
    /* Eriador        */ STORE_MERCH, STORE_TEMPLE, STORE_ALCH, STORE_HOME,
    /* Ossiriand      */ STORE_MERCH, STORE_TEMPLE, STORE_ALCH, STORE_HOME,
    /* Ered Luin South*/ STORE_MERCH, STORE_TEMPLE, STORE_ALCH, STORE_HOME,
    /* Taur-Im-Duinath*/ STORE_MERCH, STORE_TEMPLE, STORE_ALCH, STORE_HOME,
    /* Ephel Brandir  */ STORE_MERCH, STORE_TEMPLE, STORE_ALCH, STORE_HOME,
    /* Gladden Fields */ STORE_MERCH, STORE_TEMPLE, STORE_ALCH, STORE_HOME,
    /* Khazad Dum     */ STORE_GEN, STORE_ARMORY, STORE_WEAPON, STORE_TEMPLE,
                          STORE_ALCH, STORE_MAGIC, STORE_BLACKM, STORE_HOME,
                          STORE_BOOK,
    /* Belegost       */ STORE_GEN, STORE_ARMORY, STORE_WEAPON, STORE_TEMPLE,
                          STORE_ALCH, STORE_MAGIC, STORE_BLACKM, STORE_HOME,
                          STORE_BOOK,
    /* Menegroth      */ STORE_GEN, STORE_ARMORY, STORE_WEAPON, STORE_TEMPLE,
                          STORE_ALCH, STORE_MAGIC, STORE_BLACKM, STORE_HOME,
                          STORE_BOOK,
    /* Gondolin       */ STORE_GEN, STORE_ARMORY, STORE_WEAPON, STORE_TEMPLE,
                          STORE_ALCH, STORE_MAGIC, STORE_BLACKM, STORE_HOME,
                          STORE_BOOK};
    
/*
 * Accept values for y and x (considered as the endpoints of lines) between
 * 0 and 40, and return an angle in degrees (divided by two).  -LM-
 *
 * This table's input and output need some processing:
 *
 * Because this table gives degrees for a whole circle, up to radius 20, its
 * origin is at (x,y) = (20, 20).  Therefore, the input code needs to find
 * the origin grid (where the lines being compared come from), and then map
 * it to table grid 20,20.  Do not, however, actually try to compare the
 * angle of a line that begins and ends at the origin with any other line -
 * it is impossible mathematically, and the table will return the value "255".
 *
 * The output of this table also needs to be massaged, in order to avoid the
 * discontinuity at 0/180 degrees.  This can be done by:
 *   rotate = 90 - first value
 *   this rotates the first input to the 90 degree line)
 *   tmp = ABS(second value + rotate) % 180
 *   diff = ABS(90 - tmp) = the angular difference (divided by two) between
 *   the first and second values.
 *
 * Note that grids diagonal to the origin have unique angles.
 */
byte get_angle_to_grid[41][41] =
{
  {  68,  67,  66,  65,  64,  63,  62,  62,  60,  59,  58,  57,  56,  55,  53,  52,  51,  49,  48,  46,  45,  44,  42,  41,  39,  38,  37,  35,  34,  33,  32,  31,  30,  28,  28,  27,  26,  25,  24,  24,  23 },
  {  69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  59,  58,  56,  55,  54,  52,  51,  49,  48,  47,  45,  43,  42,  41,  39,  38,  36,  35,  34,  32,  31,  30,  29,  28,  27,  26,  25,  24,  24,  23,  22 },
  {  69,  69,  68,  67,  66,  65,  64,  63,  62,  61,  60,  58,  57,  56,  54,  53,  51,  50,  48,  47,  45,  43,  42,  40,  39,  37,  36,  34,  33,  32,  30,  29,  28,  27,  26,  25,  24,  24,  23,  22,  21 },
  {  70,  69,  69,  68,  67,  66,  65,  64,  63,  61,  60,  59,  58,  56,  55,  53,  52,  50,  48,  47,  45,  43,  42,  40,  38,  37,  35,  34,  32,  31,  30,  29,  27,  26,  25,  24,  24,  23,  22,  21,  20 },
  {  71,  70,  69,  69,  68,  67,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  22,  21,  20,  19 },
  {  72,  71,  70,  69,  69,  68,  67,  65,  64,  63,  62,  60,  59,  58,  56,  54,  52,  51,  49,  47,  45,  43,  41,  39,  38,  36,  34,  32,  31,  30,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18 },
  {  73,  72,  71,  70,  69,  69,  68,  66,  65,  64,  63,  61,  60,  58,  57,  55,  53,  51,  49,  47,  45,  43,  41,  39,  37,  35,  33,  32,  30,  29,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17 },
  {  73,  73,  72,  71,  70,  70,  69,  68,  66,  65,  64,  62,  61,  59,  57,  56,  54,  51,  49,  47,  45,  43,  41,  39,  36,  34,  33,  31,  29,  28,  26,  25,  24,  23,  21,  20,  20,  19,  18,  17,  17 },
  {  75,  74,  73,  72,  72,  71,  70,  69,  68,  66,  65,  63,  62,  60,  58,  56,  54,  52,  50,  47,  45,  43,  40,  38,  36,  34,  32,  30,  28,  27,  25,  24,  23,  21,  20,  19,  18,  18,  17,  16,  15 },
  {  76,  75,  74,  74,  73,  72,  71,  70,  69,  68,  66,  65,  63,  61,  59,  57,  55,  53,  50,  48,  45,  42,  40,  37,  35,  33,  31,  29,  27,  25,  24,  23,  21,  20,  19,  18,  17,  16,  16,  15,  14 },
  {  77,  76,  75,  75,  74,  73,  72,  71,  70,  69,  68,  66,  64,  62,  60,  58,  56,  53,  51,  48,  45,  42,  39,  37,  34,  32,  30,  28,  26,  24,  23,  21,  20,  19,  18,  17,  16,  15,  15,  14,  13 },
  {  78,  77,  77,  76,  75,  75,  74,  73,  72,  70,  69,  68,  66,  64,  62,  60,  57,  54,  51,  48,  45,  42,  39,  36,  33,  30,  28,  26,  24,  23,  21,  20,  18,  17,  16,  15,  15,  14,  13,  13,  12 },
  {  79,  79,  78,  77,  77,  76,  75,  74,  73,  72,  71,  69,  68,  66,  63,  61,  58,  55,  52,  49,  45,  41,  38,  35,  32,  29,  27,  24,  23,  21,  19,  18,  17,  16,  15,  14,  13,  13,  12,  11,  11 },
  {  80,  80,  79,  79,  78,  77,  77,  76,  75,  74,  73,  71,  69,  68,  65,  63,  60,  57,  53,  49,  45,  41,  37,  33,  30,  27,  25,  23,  21,  19,  17,  16,  15,  14,  13,  13,  12,  11,  11,  10,  10 },
  {  82,  81,  81,  80,  80,  79,  78,  78,  77,  76,  75,  73,  72,  70,  68,  65,  62,  58,  54,  50,  45,  40,  36,  32,  28,  25,  23,  20,  18,  17,  15,  14,  13,  12,  12,  11,  10,  10,   9,   9,   8 },
  {  83,  83,  82,  82,  81,  81,  80,  79,  79,  78,  77,  75,  74,  72,  70,  68,  64,  60,  56,  51,  45,  39,  34,  30,  26,  23,  20,  18,  16,  15,  13,  12,  11,  11,  10,   9,   9,   8,   8,   7,   7 },
  {  84,  84,  84,  83,  83,  83,  82,  81,  81,  80,  79,  78,  77,  75,  73,  71,  68,  63,  58,  52,  45,  38,  32,  27,  23,  19,  17,  15,  13,  12,  11,  10,   9,   9,   8,   7,   7,   7,   6,   6,   6 },
  {  86,  86,  85,  85,  85,  84,  84,  84,  83,  82,  82,  81,  80,  78,  77,  75,  72,  68,  62,  54,  45,  36,  28,  23,  18,  15,  13,  12,  10,   9,   8,   8,   7,   6,   6,   6,   5,   5,   5,   4,   4 },
  {  87,  87,  87,  87,  86,  86,  86,  86,  85,  85,  84,  84,  83,  82,  81,  79,  77,  73,  68,  58,  45,  32,  23,  17,  13,  11,   9,   8,   7,   6,   6,   5,   5,   4,   4,   4,   4,   3,   3,   3,   3 },
  {  89,  88,  88,  88,  88,  88,  88,  88,  88,  87,  87,  87,  86,  86,  85,  84,  83,  81,  77,  68,  45,  23,  13,   9,   7,   6,   5,   4,   4,   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   1 },
  {  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  {  91,  92,  92,  92,  92,  92,  92,  92,  92,  93,  93,  93,  94,  94,  95,  96,  97,  99, 103, 113, 135, 158, 167, 171, 173, 174, 175, 176, 176, 177, 177, 177, 178, 178, 178, 178, 178, 178, 178, 178, 179 },
  {  93,  93,  93,  93,  94,  94,  94,  94,  95,  95,  96,  96,  97,  98,  99, 101, 103, 107, 113, 122, 135, 148, 158, 163, 167, 169, 171, 172, 173, 174, 174, 175, 175, 176, 176, 176, 176, 177, 177, 177, 177 },
  {  94,  94,  95,  95,  95,  96,  96,  96,  97,  98,  98,  99, 100, 102, 103, 105, 108, 113, 118, 126, 135, 144, 152, 158, 162, 165, 167, 168, 170, 171, 172, 172, 173, 174, 174, 174, 175, 175, 175, 176, 176 },
  {  96,  96,  96,  97,  97,  97,  98,  99,  99, 100, 101, 102, 103, 105, 107, 109, 113, 117, 122, 128, 135, 142, 148, 153, 158, 161, 163, 165, 167, 168, 169, 170, 171, 171, 172, 173, 173, 173, 174, 174, 174 },
  {  97,  97,  98,  98,  99,  99, 100, 101, 101, 102, 103, 105, 106, 108, 110, 113, 116, 120, 124, 129, 135, 141, 146, 150, 154, 158, 160, 162, 164, 165, 167, 168, 169, 169, 170, 171, 171, 172, 172, 173, 173 },
  {  98,  99,  99, 100, 100, 101, 102, 102, 103, 104, 105, 107, 108, 110, 113, 115, 118, 122, 126, 130, 135, 140, 144, 148, 152, 155, 158, 160, 162, 163, 165, 166, 167, 168, 168, 169, 170, 170, 171, 171, 172 },
  { 100, 100, 101, 101, 102, 103, 103, 104, 105, 106, 107, 109, 111, 113, 115, 117, 120, 123, 127, 131, 135, 139, 143, 147, 150, 153, 155, 158, 159, 161, 163, 164, 165, 166, 167, 167, 168, 169, 169, 170, 170 },
  { 101, 101, 102, 103, 103, 104, 105, 106, 107, 108, 109, 111, 113, 114, 117, 119, 122, 125, 128, 131, 135, 139, 142, 145, 148, 151, 153, 156, 158, 159, 161, 162, 163, 164, 165, 166, 167, 167, 168, 169, 169 },
  { 102, 103, 103, 104, 105, 105, 106, 107, 108, 110, 111, 113, 114, 116, 118, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 152, 154, 156, 158, 159, 160, 162, 163, 164, 165, 165, 166, 167, 167, 168 },
  { 103, 104, 105, 105, 106, 107, 108, 109, 110, 111, 113, 114, 116, 118, 120, 122, 124, 127, 129, 132, 135, 138, 141, 143, 146, 148, 150, 152, 154, 156, 158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167 },
  { 104, 105, 106, 106, 107, 108, 109, 110, 111, 113, 114, 115, 117, 119, 121, 123, 125, 127, 130, 132, 135, 138, 140, 143, 145, 147, 149, 151, 153, 155, 156, 158, 159, 160, 161, 162, 163, 164, 164, 165, 166 },
  { 105, 106, 107, 108, 108, 109, 110, 111, 113, 114, 115, 117, 118, 120, 122, 124, 126, 128, 130, 133, 135, 137, 140, 142, 144, 146, 148, 150, 152, 153, 155, 156, 158, 159, 160, 161, 162, 162, 163, 164, 165 },
  { 107, 107, 108, 109, 110, 110, 111, 113, 114, 115, 116, 118, 119, 121, 123, 124, 126, 129, 131, 133, 135, 137, 139, 141, 144, 146, 147, 149, 151, 152, 154, 155, 156, 158, 159, 160, 160, 161, 162, 163, 163 },
  { 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 119, 120, 122, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 148, 150, 151, 153, 154, 155, 156, 158, 159, 159, 160, 161, 162, 163 },
  { 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 120, 121, 122, 124, 126, 128, 129, 131, 133, 135, 137, 139, 141, 142, 144, 146, 148, 149, 150, 152, 153, 154, 155, 157, 158, 159, 159, 160, 161, 162 },
  { 109, 110, 111, 112, 113, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 159, 159, 160, 161 },
  { 110, 111, 112, 113, 114, 114, 115, 116, 117, 119, 120, 121, 122, 124, 125, 127, 128, 130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 146, 148, 149, 150, 151, 153, 154, 155, 156, 157, 158, 159, 159, 160 },
  { 111, 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 122, 123, 124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 144, 146, 147, 148, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 159 },
  { 112, 113, 114, 114, 115, 116, 117, 118, 119, 120, 121, 122, 124, 125, 126, 128, 129, 131, 132, 133, 135, 137, 138, 139, 141, 142, 144, 145, 146, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159 },
  { 113, 114, 114, 115, 116, 117, 118, 118, 120, 121, 122, 123, 124, 125, 127, 128, 129, 131, 132, 134, 135, 136, 138, 139, 141, 142, 143, 145, 146, 147, 148, 149, 150, 152, 152, 153, 154, 155, 156, 157, 158 }
};


const byte g_info[] = 
{
    /* Easterling */
    100,110,110,105,120,115,110,115,110,110,105,105,115,100,

    /* Green Elf */
    110,100,100,110,125,120,110,120,110,105,105,100,105,110,

    /* Grey Elf */
    120,100,100,110,130,130,110,125,115,130,105,100,105,120,

    /* Hobbit */
    110,110,110, 95,110,110,110,110,110,105,105,105,110,105,

    /* Petty-Dwarf */
    115,120,120,110, 95,110,115,110,130,130,105,115,125,115,

    /* Dwarf */
    110,125,130,105,110, 95,110,100,110,130,105,125,125,105,

    /* Druadan */
    110,110,110,115,115,115, 95,115,100,110,105,115,105,115,

    /* Longbeard */
    110,115,115,105,110,100,110, 95,110,115,105,115,120,110,

    /* Edain */
    110,105,105,110,130,120,100,120,100,105,105,105,115,110,

    /* High-Elf */
    120,110,120,110,130,130,105,130,105,100,105,120,105,120,

    /* Maia */
    105,105,105,105,110,105,100,105,105,105,100,105,105,105,

    /* Dark Elf */
    105,110,115,105,125,125,115,120,110,115,105, 95,105,105,

    /* Ent */
    115,100,100,105,130,125,105,125,115,100,105,100,100,115,

    /* Beorning */
    100,110,110,105,125,120,105,120,105,110,105,110,105,100
};
