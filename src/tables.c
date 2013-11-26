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
struct druid_blows d_blow[NUM_D_BLOWS] =
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
