/* squelch.h - squelch interface */

#ifndef SQUELCH_H
#define SQUELCH_H

#define TYPE_MAX 19

/**
 * Names of categories.
 */
static const char *type_names[TYPE_MAX] =
{
  "Swords",
  "Polearms",
  "Blunt weapons",
  "Missile weapons",
  "Sling ammunition",
  "Bow ammunition",
  "Crossbow ammunition",
  "Soft armor",
  "Hard armor",
  "Dragon Scale Mail",
  "Cloaks",
  "Shields",
  "Helms",
  "Crowns",
  "Gloves",
  "Boots",
  "Diggers",
  "Rings",
  "Amulets",
};

/**
 * Mapping of tval -> type 
 */
static int tvals[TYPE_MAX] =
{
  TV_SWORD, 
  TV_POLEARM,
  TV_HAFTED,
  TV_BOW,
  TV_SHOT,
  TV_ARROW,
  TV_BOLT,
  TV_SOFT_ARMOR,
  TV_HARD_ARMOR,
  TV_DRAG_ARMOR,
  TV_CLOAK,
  TV_SHIELD,
  TV_HELM,
  TV_CROWN,
  TV_GLOVES,
  TV_BOOTS,
  TV_DIGGING,
  TV_RING,
  TV_AMULET
};

byte squelch_level[TYPE_MAX];


/**
 * The different kinds of quality squelch
 */
enum
{
  SQUELCH_NONE,
  SQUELCH_CURSED,
  SQUELCH_DUBIOUS,
  SQUELCH_DUBIOUS_NON,
  SQUELCH_NON_EGO,
  SQUELCH_AVERAGE,
  SQUELCH_GOOD_STRONG,
  SQUELCH_GOOD_WEAK,
  SQUELCH_ALL,
  
  SQUELCH_MAX
};

/**
 * The names for the various kinds of quality
 */
static const char *quality_names[SQUELCH_MAX] =
{
  "none",				             /* SQUELCH_NONE */
  "cursed (objects known to have a curse)",    	     /* SQUELCH_CURSED */
  "dubious (all dubious items)",                     /* SQUELCH_DUBIOUS */
  "dubious non-ego (strong pseudo-ID)",              /* SQUELCH_DUBIOUS_NON */
  "non-ego (all but ego-items - strong pseudo-ID)",  /* SQUELCH_NON_EGO */
  "average (everything not good or better)",         /* SQUELCH_AVERAGE */
  "good (strong pseudo-ID or identify)",             /* SQUELCH_GOOD_STRONG */
  "good (weak pseudo-ID)",		             /* SQUELCH_GOOD_WEAK */
  "everything except artifacts",	             /* SQUELCH_ALL */
};


/**
 * Structure to describe tval/description pairings. 
 */
typedef struct
{
  int tval;
  const char *desc;
} tval_desc;

/**
 * Categories for sval-dependent squelch. 
 */
static tval_desc sval_dependent[] =
{
  { TV_STAFF,		"Staffs" },
  { TV_WAND,		"Wands" },
  { TV_ROD,		"Rods" },
  { TV_SCROLL,		"Scrolls" },
  { TV_POTION,		"Potions" },
  { TV_FOOD,		"Food" },
  { TV_MAGIC_BOOK,	"Magic books" },
  { TV_PRAYER_BOOK,	"Prayer books" },
  { TV_DRUID_BOOK,	"Stones of Lore" },
  { TV_NECRO_BOOK,	"Necromantic tomes" },
  { TV_SPIKE,		"Spikes" },
  { TV_LITE,		"Lights" },
  { TV_FLASK,           "Oil" },
  { TV_SKELETON,        "Skeletons" },
  { TV_BOTTLE,          "Bottles" },
  { TV_JUNK,            "Junk" }
};


/* squelch.c  */
extern byte squelch_level[TYPE_MAX];
int get_autoinscription_index(s16b k_idx);
const char *get_autoinscription(s16b kind_idx);
int apply_autoinscription(object_type *o_ptr);
int remove_autoinscription(s16b kind);
int add_autoinscription(s16b kind, cptr inscription);
void autoinscribe_ground(void);
void autoinscribe_pack(void);
bool squelch_tval(int tval);
extern bool squelch_item_ok(object_type *o_ptr);
bool squelch_hide_item(object_type *o_ptr);
extern void squelch_drop(void);
void squelch_items(void);
extern bool seen_tval(int tval);
void do_cmd_options_item(const char *name, int row);



#endif /* !SQUELCH_H */
