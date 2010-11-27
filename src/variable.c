/** \file variable.c 
    \brief Definitions of global variables

 * The copyright.  Definitions for a large number of variables, arrays,
 * and pointers, plus the color table and sound names. 
 *
 * Copyright (c) 2009 Nick McConnell, Andi Sidwell, Leon Marrick, Bahman Rabii, 
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


/*
 * Hack -- Link a copyright message into the executable
 */
const char *copyright =
  "Copyright (c) 1987-2009 Angband contributors.\n"
  "\n"
  "This work is free software; you can redistribute it and/or modify it\n"
  "under the terms of either:\n"
  "\n"
  "a) the GNU General Public License as published by the Free Software\n"
  "   Foundation, version 2, or\n"
  "\n"
  "b) the Angband licence:\n"
  "   This software may be copied and distributed for educational, research,\n"
  "   and not for profit purposes provided that this copyright and statement\n"
  "   are included in all such copies.  Other copyrights may also apply.\n";


/*
 * Version of FAangband.
 */
byte version_major = VERSION_MAJOR;
byte version_minor = VERSION_MINOR;
byte version_patch = VERSION_PATCH;
byte version_extra = VERSION_EXTRA;

/*
 * Savefile version 
 */
byte sf_major;		/**< Savefile's "version_major" */
byte sf_minor;		/* Savefile's "version_minor" */
byte sf_patch;		/* Savefile's "version_patch" */
byte sf_extra;		/* Savefile's "version_extra". Used for enryption */


/*
 * Savefile information
 */
u32b sf_xtra;			/* Operating system info */
u32b sf_when;			/* Time when savefile created */
u16b sf_lives;			/* Number of past "lives" with this file */
u16b sf_saves;			/* Number of "saves" during this life */

/*
 * Run-time arguments
 */
bool arg_fiddle;		/* Command arg -- Request fiddle mode */
bool arg_wizard;		/* Command arg -- Request wizard mode */
bool arg_sound;			/* Command arg -- Request special sounds */
bool arg_graphics;		/* Command arg -- Request graphics mode */
bool arg_graphics_nice;	        /* Command arg -- Request nice graphics mode */
bool arg_monochrome;		/* Command arg -- Request monochrome mode */
bool arg_force_original;	/* Command arg -- Request original keyset */
bool arg_force_roguelike;	/* Command arg -- Request roguelike keyset */

/*
 * Various things
 */

bool game_start;                /* Restart after death? */
bool character_quickstart;      /* The character is based on the last one */
bool character_generated;	/* The character exists */
bool character_dungeon;		/* The character has a dungeon */
bool character_loaded;		/* The character was loaded from a savefile */
bool character_saved;		/* The character was just saved to a savefile */

s16b character_icky;		/* Depth of the game in special mode */
s16b character_xtra;		/* Depth of the game in startup mode */

u32b seed_flavor;		/* Hack -- consistent object colors */
u32b seed_town[10];		/* Hack -- consistent town layout */

s16b num_repro;			/* Current reproducer count */
s16b object_level;		/* Current object creation level */
s16b monster_level;		/* Current monster creation level */

char summon_kin_type;		/* Hack -- See summon_specific() */

s32b turn;			/* Current game turn */
bool is_daylight;               /* If there is natural light */

s32b do_feeling;		/* Hack -- Level feeling indicator */

int use_graphics;		/* "graphics" mode */
bool use_graphics_nice;	        /* The 'nice' "graphics" mode is enabled */
bool use_trptile = FALSE;       /* The triple tile mode is enabled */
bool use_dbltile = FALSE;       /* The double tile mode is enabled */
bool use_bigtile = FALSE;       /* The bigtile mode is enabled */
bool small_screen = FALSE;      /* Small screen mode for portables */
bool use_transparency = FALSE;  /* Use transparent tiles */
char notes_start[80];           /* Opening line of notes */

int image_count;  		/* Grids until next random image    */
                  		/* Optimizes the hallucination code */

s16b signal_count;		/* Hack -- Count interupts */

bool msg_flag;			/* Player has pending message */

bool inkey_base;		/* See the "inkey()" function */
bool inkey_xtra;		/* See the "inkey()" function */
u32b inkey_scan;		/* See the "inkey()" function */
bool inkey_flag;		/* See the "inkey()" function */

s16b coin_type;			/* Hack -- force coin type */
bool magic_throw;               /* Hack -- for magical throw spell */
bool opening_chest;		/* Hack -- prevent chest generation */
u16b j_level;                   /* Hack -- object level for jewellery */

bool shimmer_monsters;	/* Hack -- optimize multi-hued monsters */
bool shimmer_objects;	/* Hack -- optimize multi-hued objects */

bool repair_mflag_show;	/* Hack -- repair monster flags (show) */
bool repair_mflag_mark;	/* Hack -- repair monster flags (mark) */

s16b o_max = 1;			/* Number of allocated objects */
s16b o_cnt = 0;			/* Number of live objects */

s16b m_max = 1;			/* Number of allocated monsters */
s16b m_cnt = 0;			/* Number of live monsters */

u16b group_id = 1;              /* Number of group IDs allocated */    

/*
 * Height of dungeon map on screen.
 * Moved to defines.h -NRM-
s16b SCREEN_HGT = 22;
s16b SCREEN_WID = 66; */

/*
 * Dungeon variables
 */

s16b feeling;			/* Most recent feeling */
s16b rating;			/* Level's current rating */
bool good_item_flag;	        /* True if "Artifact" on this level */

bool closing_flag;		/* Dungeon is closing */

bool fake_monochrome;	        /* Use fake monochrome for effects */


/*
 * Dungeon size info
 */

s16b max_panel_rows, max_panel_cols;
s16b panel_row_min, panel_row_max;
s16b panel_col_min, panel_col_max;
s16b panel_col_prt, panel_row_prt;
bool panel_extra_rows=FALSE;

/*
 * Player info
 */
int player_uid;
int player_euid;
int player_egid;


/**
 * Buffer to hold the current savefile name
 */
char savefile[1024];


/**
 * Number of active macros.
 */
s16b macro__num;

/**
 * Array of macro patterns [MACRO_MAX]
 */
char **macro__pat;

/**
 * Array of macro actions [MACRO_MAX]
 */
char **macro__act;


/**
 * The number of quarks (first quark is NULL)
 */
s16b quark__num = 1;

/**
 * The array[QUARK_MAX] of pointers to the quarks
 */
char **quark__str;


/**
 * The next "free" index to use
 */
u16b message__next;

/**
 * The index of the oldest message (none yet)
 */
u16b message__last;

/**
 * The next "free" offset
 */
u16b message__head;

/**
 * The offset to the oldest used char (none yet)
 */
u16b message__tail;

/**
 * The array[MESSAGE_MAX] of offsets, by index
 */
u16b *message__ptr;

/**
 * The array[MESSAGE_BUF] of chars, by offset
 */
char *message__buf;

/**
 * The array[MESSAGE_MAX] of u16b for the types of messages
 */
u16b *message__type;


/**
 * Table of colors associated to message-types
 */
byte message__color[MSG_MAX];


/**
 * The array[TERM_WIN_MAX] of window pointers
 */
term *angband_term[TERM_WIN_MAX];


/**
 * The array[TERM_WIN_MAX] of window names (modifiable?)
 */
char angband_term_name[TERM_WIN_MAX][16] =
{
  "FAangband",
  "Term-1",
  "Term-2",
  "Term-3",
  "Term-4",
  "Term-5",
  "Term-6",
  "Term-7"
};



int max_macrotrigger = 0;
char *macro_template = NULL;
char *macro_modifier_chr;
char *macro_modifier_name[MAX_MACRO_MOD];
char *macro_trigger_name[MAX_MACRO_TRIGGER];
char *macro_trigger_keycode[2][MAX_MACRO_TRIGGER];



/**
 * Global table of color definitions (mostly zeros)
 */
byte angband_color_table[256][4] =
{
  {0x00, 0x00, 0x00, 0x00},	/* TERM_DARK */
  {0x00, 0xFF, 0xFF, 0xFF},	/* TERM_WHITE */
  {0x00, 0x80, 0x80, 0x80},	/* TERM_SLATE */
  {0x00, 0xFF, 0x80, 0x00},	/* TERM_ORANGE */
  {0x00, 0xC0, 0x00, 0x00},	/* TERM_RED */
  {0x00, 0x00, 0x80, 0x40},	/* TERM_GREEN */
  {0x00, 0x00, 0x40, 0xFF},	/* TERM_BLUE */
  {0x00, 0x80, 0x40, 0x00},	/* TERM_UMBER */
  {0x00, 0x60, 0x60, 0x60},	/* TERM_L_DARK */
  {0x00, 0xC0, 0xC0, 0xC0},	/* TERM_L_WHITE */
  {0x00, 0xFF, 0x00, 0xFF},	/* TERM_VIOLET */
  {0x00, 0xFF, 0xFF, 0x00},	/* TERM_YELLOW */
  {0x00, 0xFF, 0x00, 0x00},	/* TERM_L_RED */
  {0x00, 0x00, 0xFF, 0x00},	/* TERM_L_GREEN */
  {0x00, 0x00, 0xFF, 0xFF},	/* TERM_L_BLUE */
  {0x00, 0xC0, 0x80, 0x40},	/* TERM_L_UMBER */

  /*
   * Values for shades at compile time, taken from shades.prf
   * Hack -- TERM_WHITE (Shade 1) comes from font-x11.prf, because
   * we must ensure that all colors are different.
   */
  {0x00, 0x00, 0x00, 0x00},	/* TERM_DARK	(Shade 1) */
  {0x00, 0xAF, 0xFF, 0xFF},	/* TERM_WHITE 	(Shade 1) */
  {0x00, 0xA0, 0xA0, 0xA0},	/* TERM_SLATE 	(Shade 1) */
  {0x00, 0xDC, 0x64, 0x00},	/* TERM_ORANGE 	(Shade 1) */
  {0x00, 0xF0, 0x00, 0x00},	/* TERM_RED 	(Shade 1) */
  {0x00, 0x00, 0x70, 0x00},	/* TERM_GREEN 	(Shade 1) */
  {0x00, 0x00, 0x80, 0xFF},	/* TERM_BLUE 	(Shade 1) */
  {0x00, 0xC8, 0x64, 0x00},	/* TERM_UMBER 	(Shade 1) */
  {0x00, 0x78, 0x64, 0x64},	/* TERM_L_DARK 	(Shade 1) */
  {0x00, 0xE8, 0xD0, 0xC0},	/* TERM_L_WHITE	(Shade 1) */
  {0x00, 0xA5, 0x00, 0xFF},	/* TERM_VIOLET 	(Shade 1) */
  {0x00, 0xC8, 0xC8, 0x00},	/* TERM_YELLOW 	(Shade 1) */
  {0x00, 0xB4, 0x46, 0x32},	/* TERM_L_RED 	(Shade 1) */
  {0x00, 0x00, 0xDC, 0x64},	/* TERM_L_GREEN (Shade 1) */
  {0x00, 0x64, 0xAA, 0xC8},	/* TERM_L_BLUE  (Shade 1) */
  {0x00, 0xC8, 0xAA, 0x46} 	/* TERM_L_UMBER (Shade 1) */
};


/**
 * Standard sound names (modifiable?)
 */
char angband_sound_name[SOUND_MAX][16] =
{
  "",
  "hit",
  "miss",
  "flee",
  "drop",
  "kill",
  "level",
  "death",
  "study",
  "teleport",
  "shoot",
  "quaff",
  "zap_rod",
  "walk",
  "tpother",
  "hitwall",
  "eat",
  "store1",
  "store2",
  "store3",
  "store4",
  "dig",
  "opendoor",
  "shutdoor",
  "tplevel",
  "bell",
  "nothing_to_open",
  "lockpick_fail",
  "stairs_down", 
  "hitpoint_warn",
  "act_artifact", 
  "use_staff", 
  "destroy", 
  "mon_hit", 
  "mon_touch", 
  "mon_punch", 
  "mon_kick", 
  "mon_claw", 
  "mon_bite", 
  "mon_sting", 
  "mon_butt", 
  "mon_crush", 
  "mon_engulf", 
  "mon_crawl", 
  "mon_drool", 
  "mon_spit", 
  "mon_gaze", 
  "mon_wail", 
  "mon_spore", 
  "mon_beg", 
  "mon_insult", 
  "mon_moan", 
  "recover", 
  "blind", 
  "confused", 
  "poisoned", 
  "afraid", 
  "paralyzed", 
  "drugged", 
  "speed", 
  "slow", 
  "shield", 
  "blessed", 
  "hero", 
  "berserk", 
  "prot_evil", 
  "invuln", 
  "see_invis", 
  "infrared", 
  "res_acid", 
  "res_elec", 
  "res_fire", 
  "res_cold", 
  "res_pois", 
  "stun", 
  "cut", 
  "stairs_up", 
  "store_enter", 
  "store_leave", 
  "store_home", 
  "money1", 
  "money2", 
  "money3", 
  "shoot_hit", 
  "store5", 
  "lockpick", 
  "disarm", 
  "identify_bad", 
  "identify_ego", 
  "identify_art", 
  "breathe_elements", 
  "breathe_frost", 
  "breathe_elec", 
  "breathe_acid", 
  "breathe_gas", 
  "breathe_fire", 
  "breathe_confu", 
  "breathe_disen", 
  "breathe_chaos", 
  "breathe_shards", 
  "breathe_sound", 
  "breathe_light", 
  "breathe_dark", 
  "breathe_nether", 
  "breathe_nexus", 
  "breathe_time", 
  "breathe_inertia", 
  "breathe_gravity", 
  "breathe_plasma", 
  "breathe_force", 
  "summon_monster", 
  "summon_angel", 
  "summon_undead", 
  "summon_animal", 
  "summon_spider", 
  "summon_hound", 
  "summon_hydra", 
  "summon_demon", 
  "summon_dragon", 
  "summon_gr_undead", 
  "summon_gr_dragon", 
  "summon_gr_demon", 
  "summon_wraith", 
  "summon_unique", 
  "wield", 
  "cursed", 
  "pseudo_id", 
  "hungry", 
  "notice", 
  "ambient_day", 
  "ambient_nite", 
  "ambient_dng1", 
  "ambient_dng2", 
  "ambient_dng3", 
  "ambient_dng4", 
  "ambient_dng5", 
  "mon_create_trap", 
  "mon_shriek", 
  "mon_cast_fear", 
  "hit_good", 
  "hit_great", 
  "hit_superb", 
  "hit_hi_great", 
  "hit_hi_superb", 
  "cast_spell", 
  "pray_prayer",
  "kill_unique",
  "kill_king",
  "drain_stat",
  "multiply"
};


/**
 * Array[VIEW_MAX] used by "update_view()"
 */
sint view_n = 0;
u16b *view_g;

/* 
 * Variables for dealing with the vinfo array used by update_view() -NRM-
 */

int  vinfo_grids;
int  vinfo_slopes;
u32b vinfo_bits_3;
u32b vinfo_bits_2;
u32b vinfo_bits_1;
u32b vinfo_bits_0;


/*
 * Arrays[TEMP_MAX] used for various things
 */
sint temp_n = 0;
u16b *temp_g;
byte *temp_y;
byte *temp_x;

/* 
 * Arrays[NUM_STAGES][NUM_STAGES] of numbers of paths between nearby stages
 */
u16b (*adjacency)[NUM_STAGES];
u16b (*stage_path)[NUM_STAGES];
u16b (*temp_path)[NUM_STAGES];

/** 
 * Array[NUM_STAGES][32] of racial probability boosts for each stage; will need
 * to be expanded if z_info->p_max goes above 32.
 */
u16b (*race_prob)[32];

/**
 * Array[DUNGEON_HGT][256] of cave grid info flags (padded)
 *
 * These arrays are padded to a width of 256 to allow fast access to elements
 * in the array via "grid" values (see the GRID() macros).
 */
byte (*cave_info)[256];
byte (*cave_info2)[256];

/**
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid feature codes
 */
byte (*cave_feat)[DUNGEON_WID];


/**
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid object indexes
 *
 * Note that this array yields the index of the top object in the stack of
 * objects in a given grid, using the "next_o_idx" field in that object to
 * indicate the next object in the stack, and so on, using zero to indicate
 * "nothing".  This array replicates the information contained in the object
 * list, for efficiency, providing extremely fast determination of whether
 * any object is in a grid, and relatively fast determination of which objects
 * are in a grid.
 */
s16b (*cave_o_idx)[DUNGEON_WID];

/**
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid monster indexes
 *
 * Note that this array yields the index of the monster or player in a grid,
 * where negative numbers are used to represent the player, positive numbers
 * are used to represent a monster, and zero is used to indicate "nobody".
 * This array replicates the information contained in the monster list and
 * the player structure, but provides extremely fast determination of which,
 * if any, monster or player is in any given grid.
 */
s16b (*cave_m_idx)[DUNGEON_WID];


#ifdef MONSTER_FLOW

/**
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid flow "cost" values
 * Used to simulate character noise.
 */
byte (*cave_cost)[DUNGEON_WID];

/**
 * Array[DUNGEON_HGT][DUNGEON_WID] of cave grid flow "when" stamps.
 * Used to store character scent trails.
 */
byte (*cave_when)[DUNGEON_WID];

/**
 * Current scent age marker.  Counts down from 250 to 0 and then loops.
 */
int scent_when = 250;


/*
 * Centerpoints of the last flow (noise) rebuild and the last flow update.
 */
int flow_center_y;
int flow_center_x;
int update_center_y;
int update_center_x;

/**
 * Flow cost at the center grid of the current update.
 */
int cost_at_center = 0;

#endif	/* MONSTER_FLOW */

/**
 * Array[z_info->o_max] of dungeon objects
 */
object_type *o_list;

/**
 * Array[z_info->m_max] of dungeon monsters
 */
monster_type *m_list;

/**
 * Array[z_info->m_max] of monster lore
 */
monster_lore *l_list;

/**
 * Hack -- Array[MAX_Q_IDX] of quests
 */
quest *q_list;

/**
 * Hack -- Array[NOTES_MAX_LINES] of note records
 */
note_info *notes;


/**
 * Array[MAX_STORES] of stores
 */
store_type *store;

/**
 * Array[INVEN_TOTAL] of objects in the player's inventory
 */
object_type *inventory;


/**
 * The size of "alloc_kind_table" (at most z_info->k_max * 4)
 */
s16b alloc_kind_size;

/**
 * The array[alloc_kind_size] of entries in the "kind allocator table"
 */
alloc_entry *alloc_kind_table;

/**
 * The size of the "alloc_ego_table"
 */
s16b alloc_ego_size;

/**
 * The array[alloc_ego_size] of entries in the "ego allocator table"
 */
alloc_entry *alloc_ego_table;

/**
 * The size of "alloc_race_table" (at most z_info->r_max)
 */
s16b alloc_race_size;

/**
 * The array[alloc_race_size] of entries in the "race allocator table"
 */
alloc_entry *alloc_race_table;

/**
 * The total of all final monster generation probabilities 
 */
u32b alloc_race_total;

/*
 * Specify attr/char pairs for visual special effects
 * Be sure to use "index & 0xFF" to avoid illegal access
 */
byte misc_to_attr[256];
char misc_to_char[256];


/**
 * Specify color for inventory item text display (by tval)
 * Be sure to use "index & 0x7F" to avoid illegal access
 */
byte tval_to_attr[128];


/**
 * Current (or recent) macro action
 */
char macro_buffer[1024];


/**
 * Keymaps for each "mode" associated with each keypress.
 */
char *keymap_act[KEYMAP_MODES][256];



/*** Player information ***/

/**
 * Pointer to the player tables (sex, race, class, magic)
 */
player_sex *sp_ptr;
player_race *rp_ptr;
player_class *cp_ptr;
player_magic *mp_ptr;

/**
 * The player other record (static)
 */
static player_other player_other_body;

/**
 * Pointer to the player other record
 */
player_other *op_ptr = &player_other_body;

/**
 * The player info record (static)
 */
static player_type player_type_body;

/**
 * Pointer to the player info record
 */
player_type *p_ptr = &player_type_body;

/**
 * The character generates both directed (extra) noise (by doing noisy 
 * things) and ambiant noise (the combination of directed and innate 
 * noise).  Directed noise can immediately wake up monsters in LOS. 
 * Ambient noise determines how quickly monsters wake up and how often 
 * they get new information on the current character position.
 *
 * Each player turn, more noise accumulates.  Every time monster 
 * temporary conditions are processed, all non-innate noise is cleared.
 */
int add_wakeup_chance = 0;
u32b total_wakeup_chance = 0;

/**
 * Structure (not array) of size limits
 */
maxima *z_info;

/**
 * The vault generation arrays
 */
vault_type *v_info;
char *v_name;
char *v_text;

/**
 * The themed level generation arrays. -LM-
 */
vault_type *t_info;
char *t_name;
char *t_text;

/**
 * The terrain feature arrays
 */
feature_type *f_info;
char *f_name;
char *f_text;

/**
 * The object kind arrays
 */
object_kind *k_info;
char *k_name;
char *k_text;

/**
 * The artifact arrays
 */
artifact_type *a_info;
char *a_name;
char *a_text;

/**
 * The set item arrays
 */
set_type *s_info;
char *s_name;
char *s_text;

/**
 * The ego-item arrays
 */
ego_item_type *e_info;
char *e_name;
char *e_text;

/**
 * The monster race arrays
 */
monster_race *r_info;
char *r_name;
char *r_text;

/**
 * The player race arrays
 */
player_race *p_info;
char *p_name;
char *p_text;


/**
 * The player class arrays
 */
player_class *c_info;
char *c_name;
char *c_text;


/**
 * The player history arrays
 */
hist_type *h_info;
char *h_text;

/**
 * The shop owner arrays
 */
owner_type *b_info;
char *b_name;

/**
 * The racial price adjustment arrays
 */
byte *g_info;


/**
 * The object flavor arrays
 */
flavor_type *flavor_info;
char *flavor_name;
char *flavor_text;


/**
 * Hack -- The special Angband "System Suffix"
 * This variable is used to choose an appropriate "pref-xxx" file
 */
const char *ANGBAND_SYS = "xxx";

/**
 * Hack -- The special Angband "Graphics Suffix"
 * This variable is used to choose an appropriate "graf-xxx" file
 */
const char *ANGBAND_GRAF = "old";

/**
 * Path name: The main "lib" directory
 * This variable is not actually used anywhere in the code
 */
char *ANGBAND_DIR;

/**
 * High score files (binary)
 * These files may be portable between platforms
 */
char *ANGBAND_DIR_APEX;

/**
 * Bone files for player ghosts (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_BONE;

/**
 * Binary image files for the "*_info" arrays (binary)
 * These files are not portable between platforms
 */
char *ANGBAND_DIR_DATA;

/**
 * Textual template files for the "*_info" arrays (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_EDIT;

/**
 * Script files
 * These files are portable between platforms.
 */
char *ANGBAND_DIR_SCRIPT;

/**
 * Various extra files (ascii)
 * These files may be portable between platforms
 */
char *ANGBAND_DIR_FILE;

/**
 * Help files (normal) for the online help (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_HELP;

/**
 * Miscellanious text files, also contains any spoilers.
 * These files are portable between platforms
 */
char *ANGBAND_DIR_INFO;

/**
 * Standard "preference" files (ascii)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_PREF;

/**
 * Savefiles for current characters (binary)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_SAVE;

/**
 * User "preference" files (ascii)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_USER;

/**
 * Various extra files (binary)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_XTRA;

/** 
 * Various xtra/ subdirectories.
 */
char *ANGBAND_DIR_XTRA_FONT;
char *ANGBAND_DIR_XTRA_GRAF;
char *ANGBAND_DIR_XTRA_SOUND;
char *ANGBAND_DIR_XTRA_HELP;


/**
 * Total Hack -- allow all items to be listed (even empty ones)
 * This is only used by "do_cmd_inven_e()" and is cleared there.
 */
bool item_tester_full;


/**
 * Here is a "pseudo-hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
byte item_tester_tval;


/**
 * Here is a "hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
bool (*item_tester_hook)(object_type*);



/**
 * Current "comp" function for ang_sort()
 */
bool (*ang_sort_comp)(vptr u, vptr v, int a, int b);

/**
 * Current "swap" function for ang_sort()
 */
void (*ang_sort_swap)(vptr u, vptr v, int a, int b);


/**
 * Hack -- function hook to restrict "get_mon_num_prep()" function
 */
bool (*get_mon_num_hook)(int r_idx);

/**
 * Hack -- function hook to restrict "get_obj_num_prep()" function
 */
bool (*get_obj_num_hook)(int k_idx);

/**
 * Hack - the destination file for text_out_to_file.
 */
ang_file *text_out_file = NULL;


/**
 * Hack -- function hook to output (colored) text to the
 * screen or to a file.
 */
void (*text_out_hook)(byte a, char *str);


/**
 * Hack -- Where to wrap the text when using text_out().  Use the default
 * value (for example the screen width) when 'text_out_wrap' is 0.
 */
int text_out_wrap = 0;


/**
 * Hack -- Indentation for the text when using text_out().
 */
int text_out_indent = 0;


/**
 * Hack - the destination row for dump_line_screen.
 */
int dump_row = 0;

/**
 * Hack - the destination file for dump_line_file.
 */
ang_file *dump_out_file = NULL;

/**
 * Hack - the destination address for  dump_line_mem.
 */
char_attr *dump_ptr = NULL;

/**
 * Hack -- function hook for new line dump functions.
 */
void (*dump_line_hook)(char_attr *this_line);

/** 
 * Array for character screen/dump
 */
char_attr_line *dumpline;

/** 
 * Arrays for display player subwindows
 */
char_attr_line *pline0;
char_attr_line *pline1;

/**
 * The "highscore" file descriptor, if available.
 */
ang_file *highscore_fd;

/**
 * Themed levels generate their own feeling mesaages. -LM-
 */
char themed_feeling[80];

/**
 * The type of object the item generator should make, if specified. -LM-
 */
byte required_tval = 0;

/** The bones file a restored player ghost should use to collect extra 
 * flags, a sex, and a unique name.  This also indicates that there is 
 * a ghost active.  -LM-
 */
byte bones_selector;

/**
 * The player ghost template index. -LM-
 */
int r_ghost;

/** 
 * The player ghost name is stored here for quick reference by the 
 * description function.  -LM-
 */
char ghost_name[80];

/**
 * The type (if any) of the player ghost's personalized string, and 
 * the string itself. -LM-
 */
int ghost_string_type = 0;
char ghost_string[80];

/**
 * Variable to insure that ghosts say their special message only once.  
 * This variable is deliberately not saved, so reloaded ghosts may speak 
 * again. -LM-
 */
bool ghost_has_spoken = FALSE;


/*
 * Autosave-related global variables adopted from Zangband. -LM- 
 */
bool is_autosave = FALSE;		/* Is the save an autosave */
bool autosave;				/* Timed autosave */
s16b autosave_freq;			/* Autosave frequency */


/**
 * Is the player partly through trees or rubble and, if so, in which 
 * direction is he headed?  Monsters are handled more simply:  They have 
 * a 33% or 50% chance of walking through. -LM-
 */
byte player_is_crossing;


/*
 * Two variables that limit rogue stealing and creation of traps.  
 * One array that limits the number of runes of any type.
 * Mana reserve for runes of mana.
 */
byte number_of_thefts_on_level;
byte num_trap_on_level;
byte num_runes_on_level[MAX_RUNE];
int mana_reserve = 0;


/** XXX Mega-Hack - See main-win.c */
bool angband_keymap_flag = TRUE;

/* Path finding variables
 *
 */
char pf_result[MAX_PF_LENGTH];
int pf_result_index;

/**
 * Sound hook (for playing FX).
 */
void (*sound_hook)(int sound);


/*
 * For autoinscriptions.
 */
autoinscription *inscriptions = 0;
u16b inscriptions_count = 0;


/* 
 * Mouse button handling variables
 */
mouse_button *mse_button;
mouse_button *backup_button;
int status_end    = 0;
int depth_start   = 0;
int button_length = 0;
int num_buttons   = 0;
int prompt_end = 0;
bool normal_screen = TRUE;

/* 
 * Hooks for making and unmaking buttons
 */
int (*add_button_hook)(char *label, unsigned char keypress);
int (*kill_button_hook)(unsigned char keypress);
void (*kill_all_buttons_hook)(void);
void (*backup_buttons_hook)(void);
void (*restore_buttons_hook)(void);
