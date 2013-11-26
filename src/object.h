#ifndef INCLUDED_OBJECT_H
#define INCLUDED_OBJECT_H

#include "charattr.h"
#include "game-cmd.h"
#include "z-textblock.h"

/** Maximum number of scroll titles generated */
#define MAX_TITLES     60

/* Power calculation */
#define INHIBIT_POWER       20000

struct player;
typedef struct autoinscription autoinscription;

/*** Constants ***/

/*** Object origin kinds ***/

enum
{
	ORIGIN_NONE = 0,
	ORIGIN_MIXED,			/* stack with mixed origins */
	ORIGIN_BIRTH,			/* objects created at character birth */
	ORIGIN_STORE,			/* something you bought */
	ORIGIN_FLOOR,			/* found on the dungeon floor */
	ORIGIN_DROP,			/* normal monster drops */
	ORIGIN_DROP_UNKNOWN,	        /* drops from unseen foes */
	ORIGIN_ACQUIRE,			/* called forth by scroll */
	ORIGIN_CHEAT,			/* created by wizard mode */
	ORIGIN_CHEST,			/* found in a chest */
	ORIGIN_RUBBLE,			/* found under rubble */
	ORIGIN_VAULT,			/* on the floor of a vault */
	ORIGIN_CHAOS			/* created chaotically */
};


/*
 * Special Object Flags
 */
#define IDENT_SENSE	  0x01	/* Item has been "sensed" */
#define IDENT_STORE	  0x02	/* Item is in the inventory of a store */
#define IDENT_EMPTY	  0x04	/* Item charges are known */
#define IDENT_KNOWN	  0x08	/* Item abilities are known */
#define IDENT_CURSED      0x10  /* Item is known to be cursed */
#define IDENT_UNCURSED    0x20  /* Item is known not to be cursed */
#define IDENT_KNOW_CURSES 0x40  /* Item curses are all known */
#define IDENT_WORN	  0x80	/* Item has been wielded or worn */

/**
 * Modes for object_desc().
 */
typedef enum
{
	ODESC_BASE   = 0x00,   /*!< Only describe the base name */
	ODESC_COMBAT = 0x01,   /*!< Also show combat bonuses */
	ODESC_EXTRA  = 0x02,   /*!< Show charges/inscriptions/pvals */

	ODESC_FULL   = ODESC_COMBAT | ODESC_EXTRA,
	                       /*!< Show entire description */

	ODESC_STORE  = 0x04,   /*!< This is an in-store description */
	ODESC_PLURAL = 0x08,   /*!< Always pluralise */
	ODESC_SINGULAR    = 0x10,    /*!< Always singular */
	ODESC_SPOIL  = 0x20,    /*!< Display regardless of player knowledge */
	ODESC_PREFIX = 0x40,   /* */
	ODESC_CAPITAL = 0x80	/*!< Capitalise object name */
} odesc_detail_t;


/**
 * Modes for item lists in "show_inven()"  "show_equip()" and "show_floor()"
 */
typedef enum
{
	OLIST_NONE   = 0x00,   /* No options */
   	OLIST_WINDOW = 0x01,   /* Display list in a sub-term (left-align) */
   	OLIST_QUIVER = 0x02,   /* Display quiver lines */
   	OLIST_GOLD   = 0x04,   /* Include gold in the list */
	OLIST_WEIGHT = 0x08,   /* Show item weight */
	OLIST_PRICE  = 0x10,   /* Show item price */
	OLIST_FAIL   = 0x20    /* Show device failure */

} olist_detail_t;


/**
 * Modes for object_info()
 */
typedef enum
{
	OINFO_NONE   = 0x00, /* No options */
	OINFO_TERSE  = 0x01, /* Keep descriptions brief, e.g. for dumps */
	OINFO_SUBJ   = 0x02, /* Describe object from the character's POV */
	OINFO_FULL   = 0x04, /* Treat object as if fully IDd */
	OINFO_DUMMY  = 0x08, /* Object does not exist (e.g. knowledge menu) */
	OINFO_EGO    = 0x10, /* Describe ego random powers */
} oinfo_detail_t;


/**
 * Modes for stacking by object_similar()
 */
typedef enum
{
	OSTACK_NONE    = 0x00, /* No options (this does NOT mean no stacking) */
	OSTACK_STORE   = 0x01, /* Store stacking */
	OSTACK_PACK    = 0x02, /* Inventory and home */
	OSTACK_LIST    = 0x04, /* Object list */
	OSTACK_MONSTER = 0x08, /* Monster carrying objects */
	OSTACK_FLOOR   = 0x10, /* Floor stacking */
	OSTACK_QUIVER  = 0x20  /* Quiver */
} object_stack_t;


/**
 * Pseudo-ID markers.
 */
typedef enum
{
	INSCRIP_NULL = 0,            /*!< No pseudo-ID status */
	INSCRIP_STRANGE = 1,         /*!< Item that has mixed combat bonuses */
	INSCRIP_AVERAGE = 2,         /*!< Item with no interesting features */
	INSCRIP_MAGICAL = 3,         /*!< Item with combat bonuses */
	INSCRIP_SPLENDID = 4,        /*!< Obviously good item */
	INSCRIP_EXCELLENT = 5,       /*!< Ego-item */
	INSCRIP_SPECIAL = 6,         /*!< Artifact */
	INSCRIP_UNKNOWN = 7,

	INSCRIP_MAX                  /*!< Maximum number of pseudo-ID markers */
} obj_pseudo_t;

/**
 * Determine if a given inventory item is "aware"
 */
#define object_aware_p(T) \
	(k_info[(T)->k_idx].aware)

/**
 * Determine if a given inventory item is "tried"
 */
#define object_tried_p(T) \
	(k_info[(T)->k_idx].tried)


/**
 * Determine if a given inventory item is "known"
 * Test One -- Check for special "known" tag
 * Test Two -- Check for "Easy Know" + "Aware"
 */
#define object_known_p(T) \
	(((T)->ident & (IDENT_KNOWN)) || \
	 (kf_has(k_info[(T)->k_idx].flags_kind, KF_EASY_KNOW) \
	  && k_info[(T)->k_idx].aware))

#define object_is_known object_known_p

/**
 * Object is a missile
 */
#define is_missile(T) \
	(((T)->tval == TV_SHOT)   || \
	 ((T)->tval == TV_ARROW)  || \
	 ((T)->tval == TV_BOLT))

/**
 * Object is armour
 */
#define is_weapon(T) \
  (((T)->tval >= TV_SHOT) && ((T)->tval <= TV_SWORD))

/**
 * Object is armour
 */
#define is_armour(T) \
  (((T)->tval >= TV_BOOTS) && ((T)->tval <= TV_DRAG_ARMOR))

/**
 * Object is a ring or amulet
 */
#define is_jewellery(T) \
  (((T)->tval >= TV_AMULET) && ((T)->tval <= TV_RING))

/**
 * Determine if the attr and char should consider the item's flavor
 *
 * Identified scrolls should use their own tile.
 */
#define use_flavor_glyph(K) \
	((k_info[(K)].flavor) && \
	 !((k_info[(K)].tval == TV_SCROLL) && k_info[(K)].aware))


/**
 * Return the "attr" for a given item kind.
 * Use "flavor" if available.
 * Default to user definitions.
 */
#define object_kind_attr(K) \
	(use_flavor_glyph(K) ? \
	 (flavor_info[k_info[(K)].flavor].x_attr) : \
	 (k_info[(K)].x_attr))

/**
 * Return the "char" for a given item kind.
 * Use "flavor" if available.
 * Default to user definitions.
 */
#define object_kind_char(K) \
	(use_flavor_glyph(K) ? \
	 (flavor_info[k_info[(K)].flavor].x_char) : \
	 (k_info[(K)].x_char))

/**
 * Return the "attr" for a given item.
 * Use "flavor" if available.
 * Default to user definitions.
 */
#define object_attr(T) \
	(object_kind_attr((T)->k_idx))

/**
 * Return the "char" for a given item.
 * Use "flavor" if available.
 * Default to user definitions.
 */
#define object_char(T) \
	(object_kind_char((T)->k_idx))


/**
 * Return the "attr" for a given item.
 * Use "flavor" if available.
 * Use default definitions.
 */
#define object_attr_default(T) \
	((k_info[(T)->k_idx].flavor) ? \
	 (flavor_info[k_info[(T)->k_idx].flavor].d_attr) : \
	 (k_info[(T)->k_idx].d_attr))

/**
 * Return the "char" for a given item.
 * Use "flavor" if available.
 * Use default definitions.
 */
#define object_char_default(T) \
	((k_info[(T)->k_idx].flavor) ? \
	 (flavor_info[k_info[(T)->k_idx].flavor].d_char) : \
	 (k_info[(T)->k_idx].d_char))

/**
 * Artifacts use the "name1" field
 */
#define artifact_p(T) \
	((T)->name1 ? TRUE : FALSE)

/**
 * Ego-Items use the "name2" field
 */
#define ego_item_p(T) \
	((T)->name2 ? TRUE : FALSE)

/**
 * Cursed items.
 */
#define cursed_p(T) \
    (!cf_is_empty((T)->flags_curse) ? TRUE : FALSE)

/**
 * Known cursed items.
 */
#define known_cursed_p(T) \
    (!(cf_is_empty((T)->id_curse)) || ((T)->ident & IDENT_CURSED) ? TRUE : FALSE)


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
 * And here's the structure for the "fixed" spell information
 */
typedef struct spell {
	struct spell *next;
	unsigned int sidx;
	char *name;
	char *text;
} spell_type;


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

/*** Functions ***/

/* identify.c */
bool item_dubious(const object_type *o_ptr, bool unknown);
int value_check_aux1(object_type *o_ptr);
/* SJGU */
int value_check_aux2(object_type *o_ptr);
bool has_ego_properties(const object_type *o_ptr);
void notice_curse(int curse_flag, int item);
void notice_obj(int obj_flag, int item);
void notice_other(int other_flag, int item);
void object_known(object_type *o_ptr);
void object_aware(object_type *o_ptr);
void object_tried(object_type *o_ptr);

/* jewel.c */
bool design_ring_or_amulet(object_type *o_ptr, int lev);

/* obj-desc.c */
void object_kind_name(char *buf, size_t max, int k_idx, bool easy_know);
size_t object_desc(char *buf, size_t max, const object_type *o_ptr, odesc_detail_t mode);

/* obj-info.c */
textblock *object_info(const object_type *o_ptr, oinfo_detail_t mode);
textblock *object_info_ego(struct ego_item *ego);
void object_info_spoil(ang_file *f, const object_type *o_ptr, int wrap);
void object_info_chardump(const object_type *o_ptr, char_attr_line **line, 
			  int *current_line, int indent, int wrap);

/* obj-make.c */
s16b get_obj_num(int level);
void object_prep(object_type *o_ptr, int k_idx, aspect rand_aspect);
void apply_magic(object_type *o_ptr, int lev, bool okay, bool good, bool great);
bool make_object(object_type *j_ptr, bool good, bool great, bool exact_kind);
bool make_gold(object_type *j_ptr);
void place_object(int y, int x, bool good, bool great, bool exact_kind, byte origin);
void place_gold(int y, int x);

/* obj-util.c */
object_kind *objkind_get(int tval, int sval);
void flavor_init(void);
byte proc_list_color_hack(object_type *o_ptr);
void reset_visuals(bool prefs);
char index_to_label(int i);
s16b label_to_inven(int c);
s16b label_to_equip(int c);
bool wearable_p(const object_type *o_ptr);
s16b wield_slot_ammo(const object_type *o_ptr);
s16b wield_slot(const object_type *o_ptr);
bool slot_can_wield_item(int slot, const object_type *o_ptr);
const char *mention_use(int slot);
const char *describe_use(int i);
bool item_tester_okay(const object_type *o_ptr);
int scan_floor(int *items, int max_size, int y, int x, int mode);
void excise_object_idx(int o_idx);
void delete_object_idx(int o_idx);
void delete_object(int y, int x);
void compact_objects(int size);
void wipe_o_list(void);
s16b o_pop(void);
object_type *get_first_object(int y, int x);
object_type *get_next_object(const object_type *o_ptr);
s32b object_value(const object_type *o_ptr);
bool object_similar(const object_type *o_ptr, const object_type *j_ptr, object_stack_t mode);
void object_absorb(object_type *o_ptr, const object_type *j_ptr);
void object_wipe(object_type *o_ptr);
void object_copy(object_type *o_ptr, const object_type *j_ptr);
void object_copy_amt(object_type *o_ptr, object_type *j_ptr, int amt);
s16b floor_carry(int y, int x, object_type *j_ptr);
void drop_near(object_type *j_ptr, int chance, int y, int x, bool verbose);
void acquirement(int y1, int x1, int num, bool great);
void inven_item_charges(int item);
void inven_item_describe(int item);
void inven_item_increase(int item, int num);
void save_quiver_size(struct player *p);
void inven_item_optimize(int item);
void floor_item_charges(int item);
void floor_item_describe(int item);
void floor_item_increase(int item, int num);
void floor_item_optimize(int item);
bool inven_carry_okay(const object_type *o_ptr);
s16b inven_carry(struct player *p, struct object *o);
bool inven_stack_okay(const object_type *o_ptr);
s16b inven_takeoff(int item, int amt);
void inven_drop(int item, int amt);
void combine_pack(void);
void reorder_pack(void);
void open_quiver_slot(int slot);
void sort_quiver(void);
int get_use_device_chance(const object_type *o_ptr);
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
void reduce_charges(object_type *o_ptr, int amt);
int number_charging(const object_type *o_ptr);
bool recharge_timeout(object_type *o_ptr);
unsigned check_for_inscrip(const object_type *o_ptr, const char *inscrip);
int lookup_kind(int tval, int sval);
bool lookup_reverse(s16b k_idx, int *tval, int *sval);
int lookup_name(int tval, const char *name);
int lookup_artifact_name(const char *name);
int lookup_sval(int tval, const char *name);
int tval_find_idx(const char *name);
const char *tval_find_name(int tval);
artifact_type *artifact_of(const object_type *o_ptr);
object_kind *object_kind_of(const object_type *o_ptr);
bool obj_is_staff(const object_type *o_ptr);
bool obj_is_wand(const object_type *o_ptr);
bool obj_is_rod(const object_type *o_ptr);
bool obj_is_potion(const object_type *o_ptr);
bool obj_is_scroll(const object_type *o_ptr);
bool obj_is_food(const object_type *o_ptr);
bool obj_is_light(const object_type *o_ptr);
bool obj_is_ring(const object_type *o_ptr);
bool obj_is_ammo(const object_type *o_ptr);
bool obj_is_quiver_obj(const object_type * o_ptr);
bool obj_has_charges(const object_type *o_ptr);
bool obj_can_zap(const object_type *o_ptr);
bool obj_is_activatable(const object_type *o_ptr);
bool obj_can_activate(const object_type *o_ptr);
bool obj_can_refill(const object_type *o_ptr);
bool obj_can_browse(const object_type *o_ptr);
bool obj_can_cast_from(const object_type *o_ptr);
bool obj_can_study(const object_type *o_ptr);
bool obj_can_takeoff(const object_type *o_ptr);
bool obj_think_can_takeoff(const object_type *o_ptr);
bool obj_can_wear(const object_type *o_ptr);
bool obj_can_fire(const object_type *o_ptr);
bool obj_can_throw(const object_type *o_ptr);
bool obj_has_inscrip(const object_type *o_ptr);
u16b object_effect(const object_type *o_ptr);
object_type *object_from_item_idx(int item);
bool obj_needs_aim(object_type *o_ptr);
bool get_item_okay(int item);
int scan_items(int *item_list, size_t item_list_max, int mode);
bool item_is_available(int item, bool (*tester)(const object_type *), int mode);
void display_itemlist(void);
void display_object_idx_recall(s16b o_idx);
void display_object_kind_recall(s16b k_idx);
bool check_set(byte s_idx);
void apply_set(int s_idx);
void remove_set(int s_idx);
bool pack_is_full(void);
void pack_overflow(void);

/* obj-ui.c */
void show_inven(olist_detail_t mode);
void show_equip(olist_detail_t mode);
void show_floor(const int *floor_list, int floor_num, olist_detail_t mode);
bool verify_item(const char *prompt, int item);
bool get_item(int *cp, const char *pmt, const char *str, cmd_code cmd, int mode);

#endif /* !INCLUDED_OBJECT_H */
