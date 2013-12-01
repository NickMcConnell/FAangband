#ifndef MONSTER_MONSTER_H
#define MONSTER_MONSTER_H

#include "defines.h"
#include "h-basic.h"
#include "z-bitflag.h"
#include "z-rand.h"
#include "cave.h"
#include "object.h"

/**
 * There is a 1/25 (4%) chance of inflating the requested monster_level
 * during the creation of a monsters (see "get_mon_num()" in "monster2.c").
 * Lower values yield harder monsters more often.  Value raised in FAangband.
 */
#define NASTY_MON	25	/** 1/chance of inflated monster level */

/**
 * There is a 1/160 chance per round of creating a new monster
 */
#define MAX_M_ALLOC_CHANCE	160

/**
 * A monster can only "multiply" (reproduce) if there are fewer than 100
 * monsters on the level capable of such spontaneous reproduction.  This
 * is a hack which prevents the "m_list[]" array from exploding due to
 * reproducing monsters.  Messy, but necessary.
 */
#define MAX_REPRO	100

/** 
 * Special player ghost slot in lib/edit/monster.txt 
 */
#define PLAYER_GHOST_RACE       799


/**
 * Sign of a non-racial monster
 */
#define NON_RACIAL 255

/*** Monster flags ***/


/*
 * Special Monster Flags (all temporary)
 */
#define MFLAG_VIEW	0x01	/* Monster is in line of sight */
#define MFLAG_XXX1	0x02	/*  */
#define MFLAG_XXX2	0x04	/*  */
#define MFLAG_ACTV	0x08	/* Monster is in active mode */
#define MFLAG_WARY	0x10	/* Monster is wary of traps */
#define MFLAG_XXX5	0x20	/*  */
#define MFLAG_SHOW	0x40	/* Monster is recently memorized */
#define MFLAG_MARK	0x80	/* Monster is currently memorized */

enum
{
	#define RF(a,b) RF_##a,
	#include "list-mon-flags.h"
	#undef RF
	RF_MAX
};

#define RF_SIZE                FLAG_SIZE(RF_MAX)

#define rf_has(f, flag)        flag_has_dbg(f, RF_SIZE, flag, #f, #flag)
#define rf_next(f, flag)       flag_next(f, RF_SIZE, flag)
#define rf_is_empty(f)         flag_is_empty(f, RF_SIZE)
#define rf_is_full(f)          flag_is_full(f, RF_SIZE)
#define rf_is_inter(f1, f2)    flag_is_inter(f1, f2, RF_SIZE)
#define rf_is_subset(f1, f2)   flag_is_subset(f1, f2, RF_SIZE)
#define rf_is_equal(f1, f2)    flag_is_equal(f1, f2, RF_SIZE)
#define rf_on(f, flag)         flag_on_dbg(f, RF_SIZE, flag, #f, #flag)
#define rf_off(f, flag)        flag_off(f, RF_SIZE, flag)
#define rf_wipe(f)             flag_wipe(f, RF_SIZE)
#define rf_setall(f)           flag_setall(f, RF_SIZE)
#define rf_negate(f)           flag_negate(f, RF_SIZE)
#define rf_copy(f1, f2)        flag_copy(f1, f2, RF_SIZE)
#define rf_union(f1, f2)       flag_union(f1, f2, RF_SIZE)
#define rf_comp_union(f1, f2)  flag_comp_union(f1, f2, RF_SIZE)
#define rf_inter(f1, f2)       flag_inter(f1, f2, RF_SIZE)
#define rf_diff(f1, f2)        flag_diff(f1, f2, RF_SIZE)

/* Some flags are obvious */
#define RF_OBVIOUS_MASK \
	RF_UNIQUE, RF_QUESTOR, RF_MALE, RF_FEMALE, \
	RF_FRIEND, RF_FRIENDS, RF_ESCORT, RF_ESCORTS

/* "race" flags */
#define RF_RACE_MASK \
	RF_ORC, RF_TROLL, RF_GIANT, RF_DRAGON, \
	RF_DEMON, RF_UNDEAD, RF_EVIL, RF_ANIMAL, RF_METAL



/*
 * Monster spell flags
 */

enum
{
	#define RSF(a,b,c,d,e,f,g,h,i,j,k) RSF_##a,
	#include "list-mon-spells.h"
	#undef RSF
	RSF_MAX
};

#define RSF_SIZE               FLAG_SIZE(RSF_MAX)

#define rsf_has(f, flag)       flag_has_dbg(f, RSF_SIZE, flag, #f, #flag)
#define rsf_next(f, flag)      flag_next(f, RSF_SIZE, flag)
#define rsf_is_empty(f)        flag_is_empty(f, RSF_SIZE)
#define rsf_is_full(f)         flag_is_full(f, RSF_SIZE)
#define rsf_is_inter(f1, f2)   flag_is_inter(f1, f2, RSF_SIZE)
#define rsf_is_subset(f1, f2)  flag_is_subset(f1, f2, RSF_SIZE)
#define rsf_is_equal(f1, f2)   flag_is_equal(f1, f2, RSF_SIZE)
#define rsf_on(f, flag)        flag_on_dbg(f, RSF_SIZE, flag, #f, #flag)
#define rsf_off(f, flag)       flag_off(f, RSF_SIZE, flag)
#define rsf_wipe(f)            flag_wipe(f, RSF_SIZE)
#define rsf_setall(f)          flag_setall(f, RSF_SIZE)
#define rsf_negate(f)          flag_negate(f, RSF_SIZE)
#define rsf_copy(f1, f2)       flag_copy(f1, f2, RSF_SIZE)
#define rsf_union(f1, f2)      flag_union(f1, f2, RSF_SIZE)
#define rsf_comp_union(f1, f2) flag_comp_union(f1, f2, RSF_SIZE)
#define rsf_inter(f1, f2)      flag_inter(f1, f2, RSF_SIZE)
#define rsf_diff(f1, f2)       flag_diff(f1, f2, RSF_SIZE)

/**
 * Summons
 * Needed for shapechanging
 */
#define RSF_SUMMON_MASK \
    RSF_S_KIN, RSF_S_MONSTER, RSF_S_MONSTERS, RSF_S_ANT, RSF_S_SPIDER, \
	RSF_S_HOUND, RSF_S_ANIMAL, RSF_S_THIEF, RSF_S_SWAMP, RSF_S_DRAGON, \
	RSF_S_HI_DRAGON, RSF_S_DEMON, RSF_S_HI_DEMON, RSF_S_APPROP, \
	RSF_S_UNDEAD, RSF_S_HI_UNDEAD, RSF_S_QUEST, RSF_S_UNIQUE

/**
 * Breath attacks.
 * Need special treatment in movement AI.
 */
#define RSF_BREATH_MASK \
        RSF_BRTH_ACID, RSF_BRTH_ELEC, RSF_BRTH_FIRE, RSF_BRTH_COLD, \
         RSF_BRTH_POIS, RSF_BRTH_PLAS, RSF_BRTH_LIGHT, RSF_BRTH_DARK, \
         RSF_BRTH_CONFU, RSF_BRTH_SOUND, RSF_BRTH_SHARD, RSF_BRTH_INER, \
         RSF_BRTH_GRAV, RSF_BRTH_FORCE, RSF_BRTH_NEXUS, RSF_BRTH_NETHR, \
         RSF_BRTH_CHAOS, RSF_BRTH_DISEN, RSF_BRTH_TIME, RSF_BRTH_STORM, \
	 RSF_BRTH_DFIRE, RSF_BRTH_ICE, RSF_BRTH_ALL 

/**
 * Harassment (not direct damage) attacks.
 * Need special treatment in AI.
 */
#define RSF_HARASS_MASK \
	RSF_DARKNESS, RSF_TRAPS, RSF_FORGET, RSF_HUNGER, RSF_DRAIN_MANA, \
	  RSF_SCARE, RSF_BLIND, RSF_CONF, RSF_SLOW, RSF_HOLD, RSF_DISPEL

/* Number of times harassment spells get special treatment */
#define BASE_HARASS 5

/* Number of times harassment spells get special treatment from 
 * weaker creatures */
#define LOW_HARASS 2

/**
 * Hack -- "bolt" spells that may hurt fellow monsters
 * Need special treatment in AI.
 */
#define RSF_BOLT_MASK \
  RSF_ARROW, RSF_BOLT, RSF_SHOT, RSF_MISSL, RSF_PMISSL, RSF_BOULDER,	\
    RSF_BOLT_ACID, RSF_BOLT_ELEC, RSF_BOLT_FIRE, RSF_BOLT_COLD,		\
    RSF_BOLT_POIS, RSF_BOLT_NETHR, RSF_BOLT_WATER, RSF_BOLT_DARK,	\
    RSF_BOLT_PLAS, RSF_BOLT_ICE

/**
 * Archery attacks
 * Need special treatment in AI.
 */
#define RSF_ARCHERY_MASK \
  RSF_ARROW, RSF_BOLT, RSF_SHOT, RSF_MISSL, RSF_PMISSL, RSF_BOULDER

/**
 * Spells that can be can without a player in sight
 * Need special treatment in AI.
 */
#define RSF_NO_PLAYER_MASK \
  RSF_HEAL, RSF_ADD_MANA, RSF_TELE_SELF_TO, RSF_CURE


/*
 * Legal restrictions for "summon_specific()"
 */
#define SUMMON_KIN		 1
#define SUMMON_SAURON            2
#define SUMMON_ANT		11
#define SUMMON_SPIDER		12
#define SUMMON_HOUND		13
#define SUMMON_ANIMAL		14
#define SUMMON_SWAMP     	15
#define SUMMON_DEMON		16
#define SUMMON_UNDEAD		17
#define SUMMON_DRAGON		18
#define SUMMON_HI_DEMON		20
#define SUMMON_HI_UNDEAD	21
#define SUMMON_HI_DRAGON	22
#define SUMMON_WRAITH		31 /* now unused */
#define SUMMON_UNIQUE		32
#define SUMMON_ELEMENTAL	33
#define SUMMON_VORTEX		34
#define SUMMON_HYBRID		35
#define SUMMON_BIRD		36
#define SUMMON_GOLEM            37
#define SUMMON_THIEF		38


/*
 * Some bit-flags for the "smart" field
 *
 * Most of these relate to OF_* object flags or P_RES_* and percentage resists
 */
#define SM_RES_STRONG_ACID	0x00000001
#define SM_RES_STRONG_ELEC	0x00000002
#define SM_RES_STRONG_FIRE	0x00000004
#define SM_RES_STRONG_COLD	0x00000008
#define SM_RES_STRONG_POIS	0x00000010
#define SM_XXX1		        0x00000020
#define SM_XXX2		        0x00000040
#define SM_XXX3		        0x00000080
#define SM_GOOD_SAVE		0x00000100
#define SM_PERF_SAVE		0x00000200
#define SM_IMM_FREE		0x00000400
#define SM_IMM_MANA		0x00000800
#define SM_IMM_ACID		0x00001000
#define SM_IMM_ELEC		0x00002000
#define SM_IMM_FIRE		0x00004000
#define SM_IMM_COLD		0x00008000
#define SM_RES_ACID		0x00010000
#define SM_RES_ELEC		0x00020000
#define SM_RES_FIRE		0x00040000
#define SM_RES_COLD		0x00080000
#define SM_RES_POIS		0x00100000
#define SM_RES_FEAR		0x00200000
#define SM_RES_LIGHT		0x00400000
#define SM_RES_DARK		0x00800000
#define SM_RES_BLIND	        0x01000000
#define SM_RES_CONFU	        0x02000000
#define SM_RES_SOUND	        0x04000000
#define SM_RES_SHARD	        0x08000000
#define SM_RES_NEXUS	        0x10000000
#define SM_RES_NETHR	        0x20000000
#define SM_RES_CHAOS	        0x40000000
#define SM_RES_DISEN	        0x80000000

/* Spell Desire Table Columns */
#define D_BASE     0
#define D_SUMM     1
#define D_HURT     2
#define D_MANA     3
#define D_ESC      4
#define D_TACT     5
#define D_RES      6
#define D_RANGE    7
#define D_MAX      8

extern byte mana_cost[RSF_MAX];
extern byte spell_desire[RSF_MAX][D_MAX];
extern byte spell_range[RSF_MAX];


/*** Monster blow constants ***/


#define MONSTER_BLOW_MAX 4

/*
 * Monster blow methods
 */
enum
{
	#define RBM(x, y) RBM_##x,
	#include "list-blow-methods.h"
	#undef RBM
	RBM_MAX
};

/*
 * Monster blow effects
 */
enum
{
	#define RBE(x, y) RBE_##x,
	#include "list-blow-effects.h"
	#undef RBE
	RBE_MAX
};

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
 * Monster information, for a specific monster.
 *
 * Note: fy, fx constrain dungeon size to 256x256
 *
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
typedef struct monster {
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




/* monster1.c */
int lookup_monster(const char *name);
monster_base *lookup_monster_base(const char *name);
bool match_monster_bases(const monster_base *base, ...);
extern void describe_monster(int r_idx, bool spoilers);
extern void roff_top(int r_idx);
extern void screen_roff(int r_idx);
extern void display_roff(int r_idx);
extern bool prepare_ghost(int r_idx, monster_type *m_ptr, bool from_savefile);

/* monster2.c */
extern void monster_death(int m_idx);
extern bool mon_take_hit(int m_idx, int dam, bool *fear, const char *note);
extern void delete_monster_idx(int i);
extern void delete_monster(int y, int x);
extern void compact_monsters(int size);
extern void wipe_m_list(void);
extern s16b m_pop(void);
extern errr get_mon_num_prep(void);
extern s16b get_mon_num(int level);
extern s16b get_mon_num_quick(int level);
extern void display_monlist(void);
extern void display_itemlist(void);
extern void monster_desc(char *desc, size_t max, monster_type *m_ptr, int mode);
extern void monster_desc_race(char *desc, size_t max, int r_idx);
extern void lore_do_probe(int m_idx);
extern void lore_treasure(int m_idx, int num_item, int num_gold);
extern void update_mon(int m_idx, bool full);
extern void update_monsters(bool full);
extern s16b monster_carry(int m_idx, object_type *j_ptr);
extern void monster_swap(int y1, int x1, int y2, int x2);
extern s16b player_place(int y, int x);
extern s16b monster_place(int y, int x, monster_type *n_ptr);
extern bool place_monster_aux(int y, int x, int r_idx, bool slp, bool grp);
extern bool place_monster(int y, int x, bool slp, bool grp, bool quick);
extern bool alloc_monster(int dis, bool slp, bool quick);
extern bool summon_specific(int y1, int x1, bool scattered, int lev, int type);
extern bool summon_questor(int y1, int x1);
extern int assess_shapechange(int m_idx, monster_type *m_ptr);
extern bool multiply_monster(int m_idx);
extern void message_pain(int m_idx, int dam);
extern void update_smart_learn(int m_idx, int what);

/* monattk.c */
extern bool make_attack_normal(monster_type *m_ptr, int y, int x);
extern bool make_attack_ranged(monster_type *m_ptr, int attack);

/* monmove.c */
extern int get_scent(int y, int x);
extern int choose_ranged_attack(int m_idx, bool archery_only, int shape_rate);
extern bool cave_exist_mon(monster_race *r_ptr, int y, int x, 
                           bool occupied_ok);
extern void process_monsters(byte minimum_energy);
extern void reset_monsters(void);

#endif /* !MONSTER_MONSTER_H */
