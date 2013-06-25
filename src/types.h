/** \file types.h 
    \brief Type definitions

 * Definitions for a large number of types and arrays.  This file controls
 * what sort of data can go where.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *


 *
 * Note that "char" may or may not be signed, and that "signed char"
 * may or may not work on all machines.  So always use "s16b" or "s32b"
 * for signed values.  Also, note that unsigned values cause math problems
 * in many cases, so try to only use "u16b" and "u32b" for "bit flags",
 * unless you really need the extra bit of information, or you really
 * need to restrict yourself to a single byte for storage reasons.
 *
 * Also, if possible, attempt to restrict yourself to sub-fields of
 * known size (use "s16b" or "s32b" instead of "int", and "byte" instead
 * of "bool"), and attempt to align all fields along four-byte words, to
 * optimize storage issues on 32-bit machines.  Also, avoid "bit flags"
 * since these increase the code size and slow down execution.  When
 * you need to store bit flags, use one byte per flag, or, where space
 * is an issue, use a "byte" or "u16b" or "u32b", and add special code
 * to access the various bit flags.
 *
 * Many of these structures were developed to reduce the number of global
 * variables, facilitate structured program design, allow the use of ascii
 * template files, simplify access to indexed data, or facilitate efficient
 * clearing of many variables at once.
 *
 * Note that certain data is saved in multiple places for efficient access,
 * and when modifying the data in one place it must also be modified in the
 * other places, to prevent the creation of inconsistant data.
 */

#include "option.h"



/**** Available Types ****/


/**
 * An array of 256 cave bitflag arrays
 */
typedef bitflag grid_256[256][CAVE_SIZE];

/**
 * An array of DUNGEON_WID byte's
 */
typedef byte byte_wid[DUNGEON_WID];

/**
 * An array of DUNGEON_WID s16b's
 */
typedef s16b s16b_wid[DUNGEON_WID];

/**
 * An array of NUM_STAGES u16b's
 */
typedef u16b u16b_stage[NUM_STAGES];



/** Function hook types **/

/** Function prototype for the UI to provide to create native buttons */
typedef int (*button_add_f) (const char *, keycode_t);

/** Function prototype for the UI to provide to remove native buttons */
typedef int (*button_kill_f) (keycode_t);

/**** Available Structs ****/

typedef struct char_attr char_attr;
typedef struct alloc_entry alloc_entry;
typedef struct quest quest;
typedef struct spell spell_type;
typedef struct owner_type owner_type;
typedef struct store_type store_type;
typedef struct druid_blows druid_blows;
typedef struct autoinscription autoinscription;
typedef struct history_info history_info;
typedef struct color_type color_type;


/**** Available structs ****/

/**
 * Char, attr pair for dumping info and maybe other things
 */
struct char_attr {
    wchar_t pchar;
    byte pattr;
};

/**
 * An array of MAX_C_A_LEN char_attr's
 */
typedef char_attr char_attr_line[MAX_C_A_LEN];

/**
 * Information about maximal indices of certain arrays
 * Actually, these are not the maxima, but the maxima plus one
 */
typedef struct maxima {
    u16b f_max;		/**< Max size for "f_info[]" */
    u16b trap_max;	/**< Max size for "trap_info[]" */
    u16b k_max;		/**< Max size for "k_info[]" */
    u16b a_max;		/**< Max size for "a_info[]" */
    u16b e_max;		/**< Max size for "e_info[]" */
    u16b r_max;		/**< Max size for "r_info[]" */
    u16b mp_max;        /**< Maximum number of monster pain message sets */
    u16b v_max;		/**< Max size for "v_info[]" */
    u16b t_max;		/**< Max size for "t_info[]" */
    u16b p_max;		/**< Max size for "p_info[]" */
    u16b h_max;		/**< Max size for "h_info[]" */
    u16b b_max;		/**< Max size per element of "b_info[]" */
    u16b c_max;		/**< Max size for "c_info[]" */
    u16b flavor_max;	/**< Max size for "flavor_info[]" */
    u16b s_max;		/**< Max size for "s_info[]" */
    u16b set_max;	/**< Max size for "set_info[]" */
    u16b pit_max;	/**< Maximum number of monster pit types */

    u16b o_max;		/**< Max size for "o_list[]" */
    u16b m_max;		/**< Max size for "mon_list[]" */
    u16b l_max;		/**< Max size for "trap_list[]" */
} maxima;


/**
 * Information about terrain "features"
 */
typedef struct feature {
    char *name;		/**< Name  */
    char *text;		/**< Text  */
    int fidx;

    struct feature *next;

    byte mimic;		/**< Feature to mimic */
    byte priority; /**< Display priority */
    
    byte locked;   /**< How locked is it? */
    byte jammed;   /**< How jammed is it? */
    byte shopnum;  /**< Which shop does it take you to? */
    random_value dig;      /**< How hard is it to dig through? */

    bitflag flags[TF_SIZE];		/**< Bitflags */


    byte d_attr;	/**< Default feature attribute */
    wchar_t d_char;	/**< Default feature character */


    byte x_attr[3];   /**< Desired feature attribute (set by user/pref file) */
    wchar_t x_char[3];   /**< Desired feature character (set by user/pref file) */
} feature_type;


/*
 * A trap template.
 */
typedef struct trap
{
    char *name;		      /**< Name  */
    char *text;		      /**< Text  */

    struct trap *next;
    u32b tidx;

    byte d_attr;              /**< Default trap attribute */
    wchar_t d_char;              /**< Default trap character */

    byte x_attr;              /**< Desired trap attribute */
    wchar_t x_char;              /**< Desired trap character */

    byte rarity;              /**< Rarity */
    byte min_depth;           /**< Minimum depth */
    byte max_num;             /**< Unused */

    bitflag flags[TRF_SIZE]; /**< Trap flags (all traps of this kind) */
} trap_kind;

/*
 * An actual trap.
 */
typedef struct trap_type
{
    byte t_idx;               /**< Trap kind index */
    struct trap *kind;

    byte fy;                  /**< Location of trap */
    byte fx;

    byte xtra;

    bitflag flags[TRF_SIZE]; /**< Trap flags (only this particular trap) */
} trap_type;


/**
 * Information about object "kinds", including player knowledge.
 *
 * Only "aware", "tried", and "known" are saved in the savefile.
 */
typedef struct object_kind {
    char *name;		/**< Name (offset) */
    char *text;		/**< Text (offset) */

    struct object_kind *next;
    u32b kidx;

    byte tval;		/**< Object type */
    byte sval;		/**< Object sub type */
    random_value pval;   /**< Power for any flags which need it */

    random_value to_h;     /**< Bonus to hit */
    random_value to_d;     /**< Bonus to damage */
    random_value to_a;   /**< Bonus to armor */
    s16b ac;		/**< Base armor */

    byte dd, ds;	/**< Damage dice/sides */

    s16b weight;	/**< Weight */

    s32b cost;		/**< Object "base cost" */

    bitflag flags_obj[OF_SIZE];	/**< New object flags -NRM-*/
    bitflag flags_curse[CF_SIZE]; /**< New curse flags  -NRM- */
    bitflag flags_kind[KF_SIZE];	   /**< New object_kind flags -NRM- */

    int percent_res[MAX_P_RES];	   /**< Percentage resists -NRM- */
    int bonus_stat[A_MAX];	   /**< Stat bonuses       -NRM- */
    int bonus_other[MAX_P_BONUS];  /**< Other bonuses      -NRM- */
    int multiple_slay[MAX_P_SLAY];  /**< Slay multiples     -NRM- */
    int multiple_brand[MAX_P_BRAND];  /**< Brand multiples    -NRM- */

    byte d_attr;	/**< Default object attribute */
    wchar_t d_char;	/**< Default object character */

    byte locale[4];	/**< Allocation level(s) */
    byte chance[4];	/**< Allocation chance(s) */
    byte level;		/**< Level */

    u16b effect;       /**< Effect this item produces (effects.c) */
    random_value time;	       /**< Recharge time (rods/activation) */
    random_value charge;	       /**< Number of charges (staves/wands) */

    byte gen_mult_prob;	   /**< Probability of generating more than one */
    random_value stack_size;  /**< Number to generate */

    u16b flavor;	/**< Special object flavor (or zero) */

    byte x_attr;	/**< Desired object attribute */
    wchar_t x_char;	/**< Desired object character */

    u16b note;     /**< Autoinscription quark number */

    bool aware;		/**< The player is "aware" of the item's effects */
    bool tried;		/**< The player has "tried" one of the items */
    bool known_effect;
			/**< Item's effects when used are known. -LM- */
    bool squelch;
    bool everseen;		/**< Used to despoilify squelch menus */
} object_kind;



/**
 * Information about "artifacts".
 */
typedef struct artifact {
    char *name;		/**< Name (offset) */
    char *text;		/**< Text (offset) */

    u32b aidx;

    struct artifact *next;

    byte tval;		/**< Artifact type */
    byte sval;		/**< Artifact sub type */
    s16b pval;		/**< Artifact extra info */

    s16b to_h;		/**< Bonus to hit */
    s16b to_d;		/**< Bonus to damage */
    s16b to_a;		/**< Bonus to armor */
    s16b ac;		/**< Base armor */

    byte dd, ds;	/**< Damage when hits */

    s16b weight;	/**< Weight */
    s32b cost;		/**< Artifact "cost" */

    bitflag flags_obj[OF_SIZE];		/**< New object flags -NRM-*/
    bitflag flags_curse[CF_SIZE];	/**< New curse flags  -NRM- */
    bitflag flags_kind[KF_SIZE];    /**< New object kind flags -NRM- */

    int percent_res[MAX_P_RES];	   /**< Percentage resists -NRM- */
    int bonus_stat[A_MAX];	   /**< Stat bonuses       -NRM- */
    int bonus_other[MAX_P_BONUS];	   /**< Other bonuses      -NRM- */
    int multiple_slay[MAX_P_SLAY];	   /**< Slay multiples     -NRM- */
    int multiple_brand[MAX_P_BRAND];	   /**< Brand multiples    -NRM- */

    byte level;		/**< Artifact level */
    byte rarity;	/**< Artifact rarity */

    bool created;	/**< Whether this artifact has been created */
    bool seen;	/**< Whether this artifact has been seen as an artifact */
    u16b effect;     /**< Artifact activation (see effects.c) */
    char *effect_msg;

    random_value time;  /**< Recharge time (if appropriate) */
    random_value charge;	       /**< Number of charges (staves/wands) */

    byte set_no;	/**< Stores the set number of the artifact. 
			 * 0 if not part of a set -GS- */
    bool set_bonus;	/**< Is the item set, is the bonus currently applied? */
} artifact_type;


/**
 *
 */
typedef struct {
    byte tval;
    const char * name;
} grouper;

/* Item sets */

/** Information about an item in a set -GS- */
typedef struct set_element {
    byte a_idx;		/**< the artifact ID */
    bitflag flags_obj[OF_SIZE];	/**< New object flags -NRM-*/
    bitflag flags_curse[CF_SIZE];/**< New curse flags  -NRM- */

    int percent_res[MAX_P_RES];	   /**< Percentage resists -NRM- */
    int bonus_stat[A_MAX];	   /**< Stat bonuses       -NRM- */
    int bonus_other[MAX_P_BONUS];  /**< Other bonuses      -NRM- */
    int multiple_slay[MAX_P_SLAY];  /**< Slay multiples     -NRM- */
    int multiple_brand[MAX_P_BRAND];  /**< Brand multiples    -NRM- */
} set_element;

/** Information about items sets -GS- */
typedef struct set_type {
    struct set_type *next;
    char *name;			/**< Name */
    char *text;			/**< Text */

    byte setidx;	        /**< the set ID */
    byte no_of_items;		/**< The number of items in the set */
    set_element set_items[6];	/**< the artifact no and extra powers. */
} set_type;


/**
 * Information about "ego-items".
 */
typedef struct ego_item {
    struct ego_item *next;

    char *name;			/**< Name */
    char *text;			/**< Text */

    u32b eidx;

    s32b cost;			/**< Ego-item "cost" */

    bitflag flags_obj[OF_SIZE];	 /**< New object flags -NRM-*/
    bitflag flags_curse[CF_SIZE]; 	/**< New curse flags  -NRM- */
    bitflag flags_kind[KF_SIZE];	/**< New object kind flags -NRM- */

    bitflag id_curse[CF_SIZE];	/**< Curse ID flags  */
    bitflag id_obj[OF_SIZE];	/**< Object ID flags  */
    bitflag id_other[IF_SIZE]; /**< Miscellaneous ID flags  */

    int percent_res[MAX_P_RES];	   /**< Percentage resists -NRM- */
    int bonus_stat[A_MAX];	   /**< Stat bonuses       -NRM- */
    int bonus_other[MAX_P_BONUS];   /**< Other bonuses      -NRM- */
    int multiple_slay[MAX_P_SLAY];   /**< Slay multiples     -NRM- */
    int multiple_brand[MAX_P_BRAND];  /**< Brand multiples    -NRM- */

    byte level;			/**< Minimum level */
    byte rarity;		/**< Object rarity */
    byte rating;		/**< Rating boost */

    byte tval[EGO_TVALS_MAX];	/**< Legal tval */
    byte min_sval[EGO_TVALS_MAX];/**< Minimum legal sval */
    byte max_sval[EGO_TVALS_MAX];/**< Maximum legal sval */

    byte max_to_h;		/**< Maximum to-hit bonus */
    byte max_to_d;		/**< Maximum to-dam bonus */
    byte max_to_a;		/**< Maximum to-ac bonus */

    random_value time;	       /**< Recharge time (rods/activation) */

    u16b effect;		/**< Activation index */
    bool everseen;		/**< Do not spoil squelch menus */
    bool squelch;		/**< Squelch this ego-item */
} ego_item_type;




/**
 * Monster blow structure
 *
 *	- Method (RBM_*)
 *	- Effect (RBE_*)
 *	- Damage Dice
 *	- Damage Sides
 */
typedef struct {
    byte method;
    byte effect;
    byte d_dice;
    byte d_side;
} monster_blow;



/*
 * Monster pain messages.
 */
typedef struct monster_pain
{
	const char *messages[7];
	int pain_idx;
	
	struct monster_pain *next;
} monster_pain;
 
/*
 * Information about "base" monster type.
 */
typedef struct monster_base
{
	struct monster_base *next;

	char *name;
	char *text;

	bitflag flags[RF_SIZE];         /* Flags */
	bitflag spell_flags[RSF_SIZE];  /* Spell flags */
	
	wchar_t d_char;			/* Default monster character */

	monster_pain *pain;		/* Pain messages */
} monster_base;

/**
 * Monster "race" information, including racial memories
 *
 * Note that "d_attr" and "d_char" are used for MORE than "visual" stuff.
 *
 * Note that "x_attr" and "x_char" are used ONLY for "visual" stuff.
 *
 * Note that "cur_num" (and "max_num") represent the number of monsters
 * of the given race currently on (and allowed on) the current level.
 * This information yields the "dead" flag for Unique monsters.
 *
 * Note that "max_num" is reset when a new player is created.
 * Note that "cur_num" is reset when a new level is created.
 *
 * Note that several of these fields, related to "recall", can be
 * scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc).  All of the "recall"
 * fields have a special prefix to aid in searching for them.
 */
typedef struct monster_race {
    struct monster_race *next;
    unsigned int ridx;
    char *name;		/**< Name (offset) */
    char *text;		/**< Text (offset) */

    struct monster_base *base;
	
    byte hdice;		/**< Creatures hit dice count */
    byte hside;		/**< Creatures hit dice sides */

    s16b ac;		/**< Armour Class */

    s16b sleep;		/**< Inactive counter (base) */
    byte aaf;		/**< Area affect radius (1-100) */
    byte speed;		/**< Speed (normally 110) */

    s32b mexp;		/**< Exp value for kill */

    byte mana;		/**< max mana */

    byte freq_ranged;	/**< Ranged attack frequency */

    bitflag flags[RF_SIZE];	/* Flags */
    bitflag spell_flags[RSF_SIZE];	/* Spell flags */

    monster_blow blow[MONSTER_BLOW_MAX];/**< Up to four blows per round */


    byte level;		/**< Level of creature */
    byte rarity;	/**< Rarity of creature */


    byte d_attr;	/**< Default monster attribute */
    wchar_t d_char;	/**< Default monster character */


    byte x_attr;	/**< Desired monster attribute */
    wchar_t x_char;	/**< Desired monster character */


    byte max_num;	/**< Maximum population allowed per level */

    byte cur_num;	/**< Monster population on current level */

    byte spell_power;

} monster_race;

/**
 * Monster "lore" information
 *
 * Note that these fields are related to the "monster recall" and can
 * be scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc). XXX XXX XXX
 *
 */
typedef struct {
    s16b sights;	/**< Count sightings of this monster */
    s16b deaths;	/**< Count deaths from this monster */

    s16b pkills;	/**< Count monsters killed in this life */
    s16b tkills;	/**< Count monsters killed in all lives */

    byte wake;		/**< Number of times woken up (?) */
    byte ignore;	/**< Number of times ignored (?) */

    byte xtra1;		/**< Something (unused) */
    byte xtra2;		/**< Something (unused) */

    byte drop_gold;	/**< Max number of gold dropped at once */
    byte drop_item;	/**< Max number of item dropped at once */

    byte cast_spell;	/**< Max number of other spells seen */

    byte blows[MONSTER_BLOW_MAX];/**< Number of times each blow type was seen */

    bitflag flags[RF_SIZE];	/* Observed racial flags - a 1 indicates the
				 * flag (or lack thereof) is known to the
				 * player */
    bitflag spell_flags[RSF_SIZE];	/* Observed racial spell flags */
} monster_lore;


/**
 * Information about "vault generation"
 */
typedef struct vault {
    struct vault *next;
    unsigned int vidx;
    char *name;
    char *message;
    char *text;

    byte typ;		/**< Vault type */

    byte rat;		/**< Vault rating */

    byte hgt;		/**< Vault height */
    byte wid;		/**< Vault width */

    byte min_lev;	/**< Minimum allowable level, if specified. */
    byte max_lev;	/**< Maximum allowable level, if specified. */
} vault_type;



/**
 * Object information, for a specific object.
 *
 * Note that a "discount" on an item is permanent and never goes away.
 *
 * Note that inscriptions are now handled via the "quark_str()" function
 * applied to the "note" field, which will return NULL if "note" is zero.
 *
 * Note that "object" records are "copied" on a fairly regular basis,
 * and care must be taken when handling such objects.
 *
 * Note that "object flags" must now be derived from the object kind,
 * the artifact and ego-item indexes, and the two "xtra" fields.
 *
 * Note that the amount of resistance is now held in the nibble flag
 * fields perc_res1 and perc_res2. -NRM-
 *
 * Each cave grid points to one (or zero) objects via the "o_idx"
 * field (above).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a "stack" of objects in the same grid.
 *
 * Each monster points to one (or zero) objects via the "hold_o_idx"
 * field (below).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a pile of objects held by the monster.
 *
 * The "held_m_idx" field is used to indicate which monster, if any,
 * is holding the object.  Objects being held have "ix=0" and "iy=0".
 */
typedef struct object {
    s16b k_idx;		/**< Kind index (zero if "dead") */
    struct object_kind *kind;

    byte iy;		/**< Y-position on map, or zero */
    byte ix;		/**< X-position on map, or zero */

    byte tval;		/**< Item type (from kind) */
    byte sval;		/**< Item sub-type (from kind) */

    s16b pval;		/**< Item extra-parameter */

    byte discount;	/**< Discount (if any) */

    s16b weight;	/**< Item weight */

    byte name1;		/**< Artifact type, if any */
    byte name2;		/**< Ego-Item type, if any */

    bitflag flags_obj[OF_SIZE];		/**< New object flags -NRM-*/
    bitflag flags_curse[CF_SIZE];	/**< New curse flags  -NRM- */

    bitflag id_curse[CF_SIZE];	/**< Curse ID flags  */
    bitflag id_obj[OF_SIZE];	/**< Object ID flags  */
    bitflag id_other[IF_SIZE];	/**< Miscellaneous ID flags  */

    int percent_res[MAX_P_RES];	   /**< Percentage resists -NRM- */
    int bonus_stat[A_MAX];	   /**< Stat bonuses       -NRM- */
    int bonus_other[MAX_P_BONUS];   /**< Other bonuses      -NRM- */
    int multiple_slay[MAX_P_SLAY];  /**< Slay multiples     -NRM- */
    int multiple_brand[MAX_P_BRAND];  /**< Brand multiples    -NRM- */

    byte ident;		/**< ID flags  */
    u16b effect;	/**< Activation indicator */

    s16b ac;		/**< Normal AC */
    s16b to_h;		/**< Plusses to hit */
    s16b to_d;		/**< Plusses to damage */
    s16b to_a;		/**< Plusses to AC */

    byte dd, ds;	/**< Damage dice/sides */

    s16b timeout;	/**< Timeout Counter */
    random_value time;	       /**< Recharge time (rods/activation) */


    byte number;	/**< Number of items */
    byte marked;	/**< Object is marked */

    byte feel;		/**< Feeling index */

    s16b next_o_idx;	/**< Next object in stack (if any) */
    s16b held_m_idx;	/**< Monster holding us (if any) */

    byte origin;        /* How this item was found */
    byte origin_stage;  /* Where the item was found */
    u16b origin_xtra;   /* Extra information about origin */
    
    quark_t note;		/**< Inscription index */
} object_type;



/**
 * Monster information, for a specific monster.
 *
 * Note: fy, fx constrain dungeon size to 256x256
 *
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
typedef struct {
    s16b r_idx;		/**< Monster race index */
    s16b orig_idx;	 /**< Original race of shapechanged monster */

    byte fy;		/**< Y location on map */
    byte fx;		/**< X location on map */

    s16b hp;		/**< Current Hit points */
    s16b maxhp;		/**< Max Hit points */

    s16b csleep;	/**< Inactive counter */

    byte mspeed;	/**< Monster "speed" */
    byte energy;	/**< Monster "energy" */

    byte stunned;	/**< Monster is stunned */
    byte confused;	/**< Monster is confused */
    byte monfear;	/**< Monster is afraid */
    byte schange;	/**< Monster is shapechanged */
    byte stasis;	/**< Monster is held in stasis. -LM- */

    bool black_breath;
			/**< Monster suffers from the Black Breath -LM-  BR */

    byte cdis;		/**< Current dis from player */

    byte mflag;		/**< Extra monster flags */

    bool ml;		/**< Monster is "visible" */

    s16b hold_o_idx;	/**< Object being held (if any) */

    byte attr;  /* attr last used for drawing monster */

    u32b smart;	      /**< Field for "smart_learn" *//**< Now saved */

    byte ty;		/**< Monster target */
    byte tx;

    byte harass;	/**< 
			 * Mega Hack
			 * An AI variable making harassment spells 
			 * more likely early in a battle 
			 */

    byte min_range;	/**< What is the closest we want to be?  Not saved */
    byte best_range;	/**< How close do we want to be? Not saved */

    byte mana;		/**< Current mana level */

    bool moved;		/**< Monster has moved this turn */

    byte p_race;	/**< Player-type race for race-based monsters */
    byte old_p_race;	/**< Old player-type race for shapechanged monsters */
    s16b hostile;	/**< Who the monster is hostile to (group id) */
    u16b group;		/**< Group identifier (may be a group of one) */
    u16b group_leader;
			/**< Leader of the monster's group */

    u16b y_terr;	/**< Home for territorial monsters */
    u16b x_terr;

} monster_type;




/**
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */
struct alloc_entry {
    s16b index;		/**< The actual index */

    byte level;		/**< Base dungeon level */
    byte prob1;		/**< Probability, pass 1 */
    byte prob2;		/**< Probability, pass 2 */
    byte prob3;		/**< Probability, pass 3 */

    u16b total;		/**< Unused for now */
};



/**
 * Structure for the "quests"
 *
 * Hack -- currently, only the "level" parameter is set, with the
 * semantics that "one (QUEST) monster of that level" must be killed,
 * and then the "level" is reset to zero, meaning "all done".  Later,
 * we should allow quests like "kill 100 fire hounds", and note that
 * the "quest level" is then the level past which progress is forbidden
 * until the quest is complete.  Note that the "QUESTOR" flag then could
 * become a more general "never out of depth" flag for monsters.
 *
 * Actually, in Angband 2.8.0 it will probably prove easier to restrict
 * the concept of quest monsters to specific unique monsters, and to
 * actually scan the dead unique list to see what quests are left.
 */
struct quest 
{
    int stage;		/**< Stage quest monster will appear */
    int r_idx;		/**< Monster race */

    int cur_num;	/**< Number killed (unused) */
    int max_num;	/**< Number required (unused) */
};




/**<*
 * A store owner
 */
struct owner_type {
    struct owner_type *next;
    int bidx;
    char *owner_name;	/**< Name */
    u32b unused;	/**< Currently unused */

    s16b max_cost;	/**< Purse limit */

    int inflate;	/**< Inflation */
    byte owner_race;	/**< Owner race */

};




/**
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */
struct store_type {
    byte owner;		/**< Owner index */
    byte type;		/**< What type of store it is -NRM- */

    s16b insult_cur;	/**< Insult counter */

    s16b good_buy;	/**< Number of "good" buys */
    s16b bad_buy;	/**< Number of "bad" buys */

    s32b store_open;	/**< Closed until this turn */

    s32b store_wrap;	/**< Unused for now */

    s16b table_num;	/**< Table -- Number of entries */
    s16b table_size;	/**< Table -- Total Size of Array */
    s16b *table;	/**< Table -- Legal item kinds */

    s16b stock_num;	/**< Stock -- Number of entries */
    s16b stock_size;	/**< Stock -- Total Size of Array */
    object_type *stock;
			/**< Stock -- Actual stock items */
};





/**
 * And here's the structure for the "fixed" spell information
 */
struct spell {
	struct spell *next;
	unsigned int sidx;
	char *name;
	char *text;
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
 * Player sex info
 */
typedef struct player_sex {
    const char *title;		/**< Type of sex */

    const char *winner;	/**< Name of winner */
} player_sex;


/**
 * Player racial info
 */
typedef struct player_race {

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

} player_race;


/**
 * Starting equipment entry
 */
typedef struct start_item {
    object_kind *kind;
    byte min;	/**< Minimum starting amount */
    byte max;	/**< Maximum starting amount */
} start_item;


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

    start_item start_items[MAX_START_ITEMS];	/**< The starting inventory */
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

typedef struct {
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

/** From NPPAngband */

typedef struct flavor {
    char *text;
    struct flavor *next;
    unsigned int fidx;

    byte tval;	  /**< Associated object type */
    byte sval;	  /**< Associated object sub-type */

    byte d_attr;  /**< Default flavor attribute */
    wchar_t d_char;  /**< Default flavor character */

    byte x_attr;  /**< Desired flavor attribute */
    wchar_t x_char;  /**< Desired flavor character */
} flavor_type;

/** Information for object auto-inscribe */
struct autoinscription {
    s16b kind_idx;
    s16b inscription_idx;
};

struct history_info
{
    u16b type;		/**< Kind of history item */
    s16b place;		/**< Place when this item was recorded */
    s16b clev;		/**< Character level when this item was recorded */
    byte a_idx;		/**< Artifact this item relates to */
    s32b turn;		/**< Turn this item was recorded on */
    char event[80];	/**< The text of the item */
};


enum grid_light_level
{
	FEAT_LIGHTING_BRIGHT = 0,
	FEAT_LIGHTING_LIT,
	FEAT_LIGHTING_DARK,
	FEAT_LIGHTING_MAX
};

typedef struct
{
    u32b m_idx;		/* Monster index */
    u32b f_idx;		/* Feature index */
    u32b first_k_idx;	/* The "Kind" of the first item on the grid */
    u32b trap;          /* Trap index */
    bool multiple_objects;	/* Is there more than one item there? */
    
    enum grid_light_level lighting; /* Light level */
    bool in_view; /* TRUE when the player can currently see the grid. */
    bool is_player;
    bool hallucinate;
    bool trapborder;
} grid_data;


/**
 * A game color 
 */
struct color_type {
    char index_char;		    /**< Character index:  'r' = red, etc. */
    char name[32];		    /**< Color name */
    byte color_translate[MAX_ATTR]; /**< Index for various in-game translations */
};

/**
 * A hint.
 */
struct hint
{
        char *hint;
        struct hint *next;
};

