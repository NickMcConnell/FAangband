#ifndef INCLUDED_OBJECT_H
#define INCLUDED_OBJECT_H

#include "angband.h"
#include "z-textblock.h"

/** Maximum number of scroll titles generated */
#define MAX_TITLES     60

/* Power calculation */
#define INHIBIT_POWER       20000

struct player;

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
