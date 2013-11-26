/* player/player.h - player interface */

#ifndef PLAYER_PLAYER_H
#define PLAYER_PLAYER_H

#include "mapmode.h"
#include "object.h"
#include "option.h"

/*
 * Player constants
 */
#define PY_MAX_EXP	99999999L	/* Maximum exp */
#define PY_MAX_GOLD	99999999L	/* Maximum gold */
#define PY_MAX_LEVEL	50		/* Maximum level */
#define PY_MAX_SPELLS   64              /* Maximum number of spells */
#define PY_MAX_BOOKS    11              /* Maximum number of spellbooks */


/*
 * Flags for player_type.spell_flags[]
 */
#define PY_SPELL_LEARNED    0x01 /* Spell has been learned */
#define PY_SPELL_WORKED     0x02 /* Spell has been successfully tried */
#define PY_SPELL_FORGOTTEN  0x04 /* Spell has been forgotten */

/*
 * Player "food" crucial values
 */
#define PY_FOOD_UPPER   20000   /* Upper limit on food counter */
#define PY_FOOD_MAX	15000	/* Food value (Bloated) */
#define PY_FOOD_FULL	10000	/* Food value (Normal) */
#define PY_FOOD_ALERT	2000	/* Food value (Hungry) */
#define PY_FOOD_WEAK	1000	/* Food value (Weak) */
#define PY_FOOD_FAINT	500	/* Food value (Fainting) */
#define PY_FOOD_STARVE	100	/* Food value (Starving) */

/*
 * Player regeneration constants
 */
#define PY_REGEN_NORMAL		197	/* Regen factor*2^16 when full */
#define PY_REGEN_WEAK		98	/* Regen factor*2^16 when weak */
#define PY_REGEN_FAINT		33	/* Regen factor*2^16 when fainting */
#define PY_REGEN_HPBASE		1442	/* Min amount hp regen*2^16 */
#define PY_REGEN_MNBASE		524	/* Min amount mana regen*2^16 */


/** Maximum number of blow types available to Druids. -LM- */
#define NUM_D_BLOWS		20


/**
 * Maximum number of "normal" pack slots, and the index of the "overflow"
 * slot, which can hold an item, but only temporarily, since it causes the
 * pack to "overflow", dropping the "last" item onto the ground.  Since this
 * value is used as an actual slot, it must be less than "INVEN_WIELD" (below).
 * Note that "INVEN_PACK" is probably hard-coded by its use in savefiles, and
 * by the fact that the screen can only show 23 items plus a one-line prompt.
 */
#define INVEN_PACK		23 

/**
 * Like the previous but takes into account the (variably full quiver).
 */
#define INVEN_MAX_PACK  (INVEN_PACK - p_ptr->quiver_slots)

/*
 * Indexes used for various "equipment" slots (hard-coded by savefiles, etc).
 */
#define INVEN_WIELD	24
#define INVEN_BOW       25
#define INVEN_LEFT      26
#define INVEN_RIGHT     27
#define INVEN_NECK      28
#define INVEN_LIGHT      29
#define INVEN_BODY      30
#define INVEN_OUTER     31
#define INVEN_ARM       32
#define INVEN_HEAD      33
#define INVEN_HANDS     34
#define INVEN_FEET      35

#define INVEN_TOTAL	36

/**
 *Quiver
 */
#define QUIVER_START 37
#define QUIVER_SIZE  10
#define QUIVER_END   47

/**
 * Each throwing weapon in the "quiver" takes up the space of this
 * many pieces of ammo. 
 */
#define THROWER_AMMO_FACTOR 5

/**
 * The number of ammo equivalents per quiver slot
 */
#define QUIVER_SLOT_SIZE    99

/**
 * Total number of inventory slots (hard-coded to be <= 47).
 */
#define ALL_INVEN_TOTAL	47

/**
 * Special 'Item' Identifier corresponding to all Squelched items.
 */
#define ALL_SQUELCHED   101


/**
 * A "stack" of items is limited to less than 100 items (hard-coded).
 */
#define MAX_STACK_SIZE	100



/**
 * An item's pval (for charges, amount of gold, etc) is limited to s16b
 */
#define MAX_PVAL  32767

/**
 * Maximum number of objects allowed in a single dungeon grid.
 *
 * The main-screen has a minimum size of 24 rows, so we can always
 * display 23 objects + 1 header line.
 */
#define MAX_FLOOR_STACK			23

/*** Constants for accessing the player struct ***/

/*
 * Timed effects
 */
enum
{
	TMD_FAST = 0, 
	TMD_SLOW, 
	TMD_BLIND, 
	TMD_PARALYZED, 
	TMD_CONFUSED,
	TMD_AFRAID, 
	TMD_IMAGE, 
	TMD_POISONED, 
	TMD_CUT, 
	TMD_STUN, 
	TMD_PROTEVIL,
	TMD_INVULN, 
	TMD_HERO, 
	TMD_SHERO, 
	TMD_SHIELD, 
	TMD_BLESSED, 
	TMD_SINVIS,
	TMD_SINFRA, 
	TMD_TELEPATHY, 
	TMD_SSTEALTH,
	TMD_OPP_ACID, 
	TMD_OPP_ELEC, 
	TMD_OPP_FIRE, 
	TMD_OPP_COLD,
	TMD_OPP_POIS, 
	TMD_ATT_ACID,
	TMD_ATT_ELEC,
	TMD_ATT_FIRE,
	TMD_ATT_COLD,
	TMD_ATT_POIS,

	TMD_MAX
};

/*
 * Skill indexes
 */
enum
{
	SKILL_DISARM,			/* Skill: Disarming */
	SKILL_DEVICE,			/* Skill: Magic Devices */
	SKILL_SAVE,			/* Skill: Saving throw */
	SKILL_STEALTH,			/* Skill: Stealth factor */
	SKILL_SEARCH,			/* Skill: Searching ability */
	SKILL_SEARCH_FREQUENCY,	        /* Skill: Searching frequency */
	SKILL_TO_HIT_MELEE,		/* Skill: To hit (normal) */
	SKILL_TO_HIT_BOW,		/* Skill: To hit (shooting) */
	SKILL_TO_HIT_THROW,		/* Skill: To hit (throwing) */
	SKILL_DIGGING,			/* Skill: Digging */

	SKILL_MAX
};


/*
 * The range of possible indexes into tables based upon stats.
 * Currently things range from 3 to 18/220 = 40.
 */
#define STAT_RANGE 38


/*
 * Player sex constants (hard-coded by save-files, arrays, etc)
 */
#define SEX_FEMALE	0
#define SEX_MALE		1

/*
 * Player class constants (hard-coded by save-files, arrays, etc)
 */
enum
{
    CLASS_WARRIOR = 0,
    CLASS_MAGE,	
    CLASS_PRIEST,	
    CLASS_ROGUE,	
    CLASS_RANGER,	
    CLASS_PALADIN,	
    CLASS_DRUID,	
    CLASS_NECRO,	
    CLASS_ASSASSIN,

    CLASS_MAX
};	

/* Different realms here.  From Sangband */
enum
{
    REALM_NONE = 0,
    REALM_SORCERY,		
    REALM_PIETY,		
    REALM_NATURE,		
    REALM_NECROMANTIC,

    REALM_MAX
};	

/* Values for accessing the magic description array */
enum
{
    SPELL_NOUN = 0,
    SPELL_VERB,
    SPELL_FAIL,
    BOOK_NOUN,
    BOOK_LACK,

    MD_MAX
};

/* 
 * Special values for the number of turns to rest, these need to be
 * negative numbers, as postive numbers are taken to be a turncount,
 * and zero means "not resting". 
 */
enum 
{
	REST_COMPLETE = -2,
	REST_ALL_POINTS = -1,
	REST_SUNLIGHT = -3
};

/*** Structures ***/

/*
 * All the variable state that changes when you put on/take off equipment.
 */
typedef struct player_state {
    s16b num_blow;	/**< Number of blows */
    s16b num_fire;	/**< Number of shots */

    byte ammo_mult;	/**< Ammo multiplier */

    byte ammo_tval;	/**< Ammo variety */

    s16b pspeed;	/**< Current speed */

    s16b stat_use[A_MAX];
			/**< Current modified stats */
    s16b stat_top[A_MAX];
			/**< Maximal modified stats */
    s16b stat_add[A_MAX];
			/**< Equipment stat bonuses */
    s16b stat_ind[A_MAX];
			/**< Indexes into stat tables */

    s16b dis_to_h;	/**< Known bonus to hit */
    s16b dis_to_d;	/**< Known bonus to dam */
    s16b dis_to_a;	/**< Known bonus to ac */

    s16b dis_ac;	/**< Known base ac */

    s16b to_h;		/**< Bonus to hit */
    s16b to_d;		/**< Bonus to dam */
    s16b to_a;		/**< Bonus to ac */

    s16b ac;		/**< Base ac */

    s16b see_infra;	/**< Infravision range */

    s16b skills[SKILL_MAX];	/**< Skills */

    byte evasion_chance;/**< Percentage to avoid attacks with evasion */
    u32b base_wakeup_chance; /**< Derived from stealth.  Revised in Oangband. */

    bool shield_on_back;/**< Player carrying a shield on his back. -LM- */
    bool heavy_wield;	/**< Heavy weapon */
    bool heavy_shoot;	/**< Heavy shooter */
    bool icky_wield;	/**< Icky weapon */

    int res_list[MAX_P_RES];   /**< Resistances and immunities */
    int dis_res_list[MAX_P_RES];   /**< Known resistances and immunities */

    bool no_fear;	/**< Resist fear */
    bool no_blind;	/**< Resist blindness */

    bool sustain_str;
			/**< Keep strength */
    bool sustain_int;
			/**< Keep intelligence */
    bool sustain_wis;
			/**< Keep wisdom */
    bool sustain_dex;
			/**< Keep dexterity */
    bool sustain_con;
			/**< Keep constitution */
    bool sustain_chr;
			/**< Keep charisma */

    bool slow_digest;
			/**< Slower digestion */
    bool ffall;		/**< Feather falling */
    bool light;		/**< Permanent light */
    bool regenerate;	/**< Regeneration */
    bool telepathy;	/**< Telepathy */
    bool see_inv;	/**< See invisible */
    bool free_act;	/**< Free action */
    bool hold_life;	/**< Hold life */
    bool impact;	/**< Earthquake blows */
    bool bless_blade;
			/**< Blessed blade */
    bool darkness;	/**< Darkness */
    bool chaotic;	/**< Chaotic effects */

    bool teleport;	/**< Random teleporting */
    bool no_teleport;
			/**< Teleporting forbidden */
    bool aggravate;	/**< Aggravate monsters */
    bool rand_aggro;	/**< Randomly aggravate monsters */
    bool slow_regen;	/**< Reduced regeneration */
    bool fear;		/**< Permanent fear */
    bool fast_digest;
			/**< Fast digestion */
    bool rand_pois;	/**< Random poisoning */
    bool rand_pois_bad;
			/**< Bad random poisoning */
    bool rand_cuts;	/**< Random cuts */
    bool rand_cuts_bad;
			/**< Bad random cuts */
    bool rand_hallu;	/**< Random hallucination */
    bool drop_weapon;
			/**< Randomly drop weapon */
    bool attract_demon;
			/**< Randomly summon demons */
    bool attract_undead;
			/**< Randomly summon undead */
    bool rand_paral;	/**< Random paralysis */
    bool rand_paral_all;
			/**< Random unresistable paralysis */
    bool drain_exp;	/**< Experience draining */
    bool drain_mana;	/**< Mana draining */
    bool drain_stat;	/**< Random stat draining */
    bool drain_charge;
			/**< Random charge draining */
} player_state;




/**
 * Most of the "player" information goes here.
 *
 * This stucture gives us a large collection of player variables.
 *
 * This entire structure is wiped when a new character is born.
 *
 * This structure is more or less laid out so that the information
 * which must be saved in the savefile precedes all the information
 * which can be recomputed as needed.
 */
typedef struct player {
    s16b py;		/**< Player location */
    s16b px;		/**< Player location */

    byte psex;		/**< Sex index */
    byte prace;		/**< Race index */
    byte pclass;	/**< Class index */
    byte oops;		/**< Unused */

    const struct player_sex *sex;
    const struct player_race *race;
    const struct player_class *class;

    byte hitdie;	/**< Hit dice (sides) */

    byte schange;	/**< Character's new shape, if any. */
    s16b age;		/**< Character's age */
    s16b ht;		/**< Height */
    s16b wt;		/**< Weight */
    s16b sc;		/**< Social Class */

    s32b au;		/**< Current Gold */

    /* s16b max_depth; Max depth */
    /* Keep depth for now because so many things use it -NRM- */
    s16b home;		/**< Home town */
    s16b depth;		/**< Cur depth */

    s16b recall[4];	/**< Recall points */
    s16b recall_pt;	/**< Which recall point is active */
    s16b stage;		/**< Current stage */
    s16b last_stage;	/**< Previous stage */

    s16b max_lev;	/**< Max level */
    s16b lev;		/**< Cur level */

    s32b max_exp;	/**< Max experience */
    s32b exp;		/**< Cur experience */
    u16b exp_frac;	/**< Cur exp frac (times 2^16) */

    s16b mhp;		/**< Max hit pts */
    s16b chp;		/**< Cur hit pts */
    u16b chp_frac;	/**< Cur hit frac (times 2^16) */

    s16b msp;		/**< Max mana pts */
    s16b csp;		/**< Cur mana pts */
    u16b csp_frac;	/**< Cur mana frac (times 2^16) */

    s16b stat_max[A_MAX];/**< Current "maximal" stat values */
    s16b stat_cur[A_MAX];/**< Current "natural" stat values */
    u16b barehand_dam[12];/**< Recent barehand damage values */

    s16b timed[TMD_MAX];/**< Timed effects */
    s16b ele_attack;	/**< Timed -- Temporary Elemental attacks -LM- */

    bool black_breath;
			/**< Major experience draining */
    s16b word_recall;
			/**< Word of recall counter */

    s16b energy;	/**< Current energy */

    s16b food;		/**< Current nutrition */

    u32b special_attack;/**< Special attack capacity -LM- */
    byte searching;	/**< Currently searching */

    byte themed_level; /**< Player in a themed level?  Which one? */
    u32b themed_level_appeared;	/* Flags indicating which themed levels have
				 * already appeared. -LM- */

    byte spell_flags[PY_MAX_SPELLS]; /* Spell flags */
    byte spell_order[PY_MAX_SPELLS];	/* Spell order */

    s16b player_hp[PY_MAX_LEVEL];/**< HP Array */

    char died_from[80];	/**< Cause of death */
    char history[250];/**< Initial history */

    u16b quests;	/**< Number of quests finished */
    u16b total_winner;	/**< Total winner */
    u32b score;		/**< Current score (new!) */

    u16b noscore;	/**< Cheating flags */

    bool is_dead;	/**< Player is dead */

    bool wizard;	/**< Player is in wizard mode */
    byte map;           /**< Which stage_map to use */
    bool game_mode[GAME_MODE_MAX];  /**< Game mode (thrall, ironman, etc) */

  /*** Temporary fields ***/

    bool playing;	/**< True if player is playing */

    bool leaving;	/**< True if player is leaving */

    s16b create_stair;	/**< Create stair/path on next level */
    s16b path_coord;	/**< Records where the exit path was */

    s16b total_weight;	/**< Total weight being carried */

    s16b inven_cnt;	/**< Number of items in inventory */
    s16b equip_cnt;	/**< Number of items in equipment */
    s16b pack_size_reduce;	/**< Amount of space the quiver uses up. */

    s16b health_who;	/**< Health bar trackee */

    s16b monster_race_idx;		/**< Monster race trackee */

    s16b object_idx;	/**< Object trackee */
    s16b object_kind_idx;/**< Object kind trackee */

    s16b energy_use;	/**< Energy use this turn */

    s16b resting;	/**< Resting counter */
    s16b running;	/**< Running counter */

    s16b run_cur_dir;	/**< Direction we are running */
    s16b run_old_dir;	/**< Direction we came from */
    bool running_withpathfind;	  /**< Are we using the pathfinder ? */
    bool run_open_area;	/**< Looking for an open area */
    bool run_break_right;	/**< Looking for a break (right) */
    bool run_break_left;	/**< Looking for a break (left) */

    s16b command_arg;	/**< Gives argument of current command */
    s16b command_wrk;	/**< See "cmd1.c" */

    s16b new_spells;	/**< Number of spells available */

    s16b old_light;	/**< Old radius of light (if any) */
    s16b old_view;	/**< Old radius of view (if any) */

    s16b old_food_aux;	/**< Old value of food */

    bool cumber_armor;	/**< Mana draining armor */
    bool cumber_glove;	/**< Mana draining gloves */

    s16b cur_light;	/**< Radius of light (if any) */

    u32b notice;	/**< Special Updates (bit flags) */
    u32b update;	/**< Pending Updates (bit flags) */
    u32b redraw;	/**< Normal Redraws (bit flags) */
    u32b window;	/**< Window Redraws (bit flags) */

  /*** Extracted fields ***/

    byte vulnerability;	/**< Used to make animal packs charge and retreat */

    bitflag pflags[PF_SIZE];  /**< Chosen specialties */
    byte specialty_order[CLASS_SPECIALTIES];/**< Order of specialty abilities */
    s16b new_specialties;		/**< Number of specialties available */
    s16b old_specialties;
    s16b specialties_allowed;	/**< Number we can use right now */

  /**< Helper variables for some specialty abilities */
    s16b speed_boost;
			/**< Short term speed boost (Fury) */
    s16b heighten_power;
			/**< Magic Intensity boost from casting (Hghtn Magic) */
    s16b mana_gain;	/**< Mana gained by special means this turn */
    /* Generation fields (for quick start) */
    s32b au_birth;		/* Birth gold when option birth_money is false */
    s16b stat_birth[A_MAX];	/* Birth "natural" stat values */
    s16b ht_birth;		/* Birth Height */
    s16b wt_birth;		/* Birth Weight */
    s16b sc_birth;		/* Birth social class */

    /* Variable and calculatable player state */
    player_state state;

    /* "cached" quiver statistics*/
    u16b quiver_size;
    u16b quiver_slots;
    u16b quiver_remainder;

    struct object *inventory;
} player_type;

/**
 * Player sex info
 */
typedef struct player_sex {
    const char *title;		/**< Type of sex */

    const char *winner;	/**< Name of winner */
} player_sex;


/**
 * Player racial info
 */
struct player_race {

    struct player_race *next;
    const char *name;

    unsigned int ridx;

    s16b r_adj[A_MAX];	/**< Racial stat modifiers */

    s16b r_skills[SKILL_MAX];	/* racial skills */
    s16b rx_skills[SKILL_MAX];	/* racial extra skills */

    s16b re_id;		/**< ego-item index */
    s16b re_mint;	/**< min tval */
    s16b re_maxt;	/**< max tval */
    s16b re_skde;	/**< bonus to skill & deadliness */
    s16b re_ac;		/**< bonus to armour class */
    s16b re_bonus;	/**< other bonus */

    byte r_mhp;		/**< Race hit-dice modifier */
    byte difficulty;	/**< Race difficulty factor */
    byte start_lev;	/**< Race starting level */
    byte hometown;	/**< Race starting town */

    u16b b_age;		/**< base age */
    u16b m_age;		/**< mod age */

    byte m_b_ht;	/**< base height (males) */
    byte m_m_ht;	/**< mod height (males) */
    u16b m_b_wt;	/**< base weight (males) */
    u16b m_m_wt;	/**< mod weight (males) */

    byte f_b_ht;	/**< base height (females) */
    byte f_m_ht;	/**< mod height (females)	  */
    u16b f_b_wt;	/**< base weight (females) */
    u16b f_m_wt;	/**< mod weight (females) */

    byte infra;		/**< Infra-vision range */

    u16b choice;	/**< Legal class choices */

    s16b hist;		/**< Starting history index */

    bitflag flags_obj[OF_MAX];		/**< New object flags -NRM-*/
    bitflag flags_curse[CF_MAX];	/**< New curse flags  -NRM- */

    int percent_res[MAX_P_RES];	   /**< Percentage resists -NRM- */

    bitflag pflags[PF_SIZE];	/* Race (player) flags */

};


/**
 * Starting equipment entry
 */
struct start_item {
    struct object_kind *kind;
    byte min;	/**< Minimum starting amount */
    byte max;	/**< Maximum starting amount */
};


/**
 * Spell information.  Index only controls effects; position in spellbook 
 * is controlled by values in the array "book_start_index".
 */
typedef struct{
    byte index;		/**< The internal spell index. */
    byte slevel;	/**< Required level (to learn) */
    byte smana;		/**< Required mana (to cast) */
    byte sfail;		/**< Base chance of failure */
    byte sexp;		/**< Encoded experience bonus */
} magic_type;


/**
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */
typedef struct {
    byte spell_book;		/**< Tval of spell books (if any) */
    byte spell_stat;		/**< Stat for spells (if any) */
    byte spell_realm;		/**< Spell Realm (if any)  */
    byte spell_first;		/**< Level of first spell */
    s16b spell_weight1;		/**< Max armour weight to avoid mana penalties */
    s16b spell_weight2;		/**< Additional armour weight to lose all mana */
    byte spell_number;		/**< Total available spells for that class. */
    byte book_start_index[PY_MAX_BOOKS];
					/**< Index of 1st spell for all books. */
    magic_type info[PY_MAX_SPELLS];		/**< The available spells */
} player_magic;



/**
 * Player class info
 */
typedef struct player_class {
    struct player_class *next;
    const char *name;
    unsigned int cidx;

    const char *title[10];	/* Titles - offset */

    s16b c_adj[A_MAX];
			/**< Class stat modifier */

    s16b c_skills[SKILL_MAX];	/* class skills */
    s16b cx_skills[SKILL_MAX];	/* extra skills */

    s16b c_mhp;		/**< Class hit-dice adjustment */

    u32b sense_base;	/**< Base psuedo-id time */

    bitflag pflags[PF_SIZE];	/* Class (player) flags */
    bitflag specialties[PF_SIZE];
				  /**< Available Specialty Abilities */

    /**< Weapon Info */
    u16b max_1;		/**< Max Weight (decipounds) at level 1 */
    u16b max_50;	/**< Max Weight (decipounds) at level 50 */
    byte penalty;	/**< Penalty per 10 pounds over */
    byte max_penalty;
			/**< Max Penalty */
    byte bonus;		/**< Bonus per 10 pounds under */
    byte max_bonus;	/**< Max Bonus */

    struct start_item start_items[MAX_START_ITEMS];	/**< The starting inventory */
    player_magic magic;	/**< Class magic details */

} player_class;


/**
 * Player background information
 */
typedef struct history {
    struct history *nextp;
    unsigned int hidx;
    char *text;

    byte roll;		/**< Frequency of this entry */
    byte chart;		/**< Chart index */
    byte next;		/**< Next chart index */
    byte bonus;		/**< Social Class Bonus + 50 */
} hist_type;



/**
 * Some more player information
 *
 * This information is retained across player lives
 */
typedef struct {
    char full_name[32];	/**< Full name */

    bool opt[OPT_MAX];	/**< Options */

    u32b window_flag[ANGBAND_TERM_MAX];/**< Window flags */

    s16b hitpoint_warn;	/**< Hitpoint warning (0 to 9) */

    s16b delay_factor;	/**< Delay factor (0 to 9) */

    s16b panel_change;	/**< Panel change factor (0 to 4) */
	
    byte name_suffix;	/* numeric suffix for player name */
} player_other;


/** Druid blows. -LM- */
struct druid_blows {

    const char *description;
			/**< Name of the blow. */

    s16b dd;		/**< Number of damage dice. */
    s16b ds;		/**< Number of dice sides. */
};

/* play-calcs.c */
extern const byte adj_dex_dis[STAT_RANGE];
extern const byte adj_dex_ta[STAT_RANGE];
extern const byte adj_str_td[STAT_RANGE];
extern const byte adj_dex_th[STAT_RANGE];
extern const byte adj_str_wgt[STAT_RANGE];
extern const byte adj_str_hold[STAT_RANGE];
extern const byte adj_str_blow[STAT_RANGE];

extern void apply_resist(int *player_resist, int item_resist);
void calc_bonuses(object_type inventory[], player_state *state, bool id_only);
int calc_blows(const object_type *o_ptr, player_state *state, int extra_blows);
void notice_stuff(struct player *p);
void update_stuff(struct player *p);
void redraw_stuff(struct player *p);
void handle_stuff(struct player *p);
int weight_remaining(void);


/* play-spell.c */
int spell_collect_from_book(const object_type *o_ptr, int spells[PY_MAX_SPELLS]);
int spell_book_count_spells(const object_type *o_ptr, bool (*tester)(int spell));
bool spell_okay_list(bool (*spell_test)(int spell), const int spells[], int n_spells);
bool spell_okay_to_cast(int spell);
bool spell_okay_to_study(int spell);
bool spell_okay_to_browse(int spell);
bool spell_in_book(int spell, int book);
s16b spell_chance(int spell);
void spell_learn(int spell);
bool spell_cast(int spell, int dir);

/* play-timed.c */
bool set_timed(int idx, int v, bool notify);
bool inc_timed(int idx, int v, bool notify);
bool dec_timed(int idx, int v, bool notify);
bool clear_timed(int idx, bool notify);
bool set_food(int v);

/* play-util.c */
s16b modify_stat_value(int value, int amount);
bool player_can_cast(void);
bool player_can_study(void);
bool player_can_read(void);
bool player_can_fire(void);
extern const char *player_safe_name(struct player *p, bool strip_suffix);

#endif /* !PLAYER_PLAYER_H */
