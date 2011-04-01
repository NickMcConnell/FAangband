#ifndef INCLUDED_OBJECT_H
#define INCLUDED_OBJECT_H

#include "angband.h"
#include "z-textblock.h"

/** Maximum number of scroll titles generated */
#define MAX_TITLES     60

struct player;

/*** Constants ***/

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
	ODESC_PREFIX = 0x40   /* */
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

/* obj-util.c */
/* Basic tval testers */
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

int get_use_device_chance(const object_type *o_ptr);
bool obj_has_charges(const object_type *o_ptr);
bool obj_can_zap(const object_type *o_ptr);
bool obj_is_activatable(const object_type *o_ptr);
bool obj_can_activate(const object_type *o_ptr);
bool obj_can_refill(const object_type *o_ptr);
bool obj_can_browse(const object_type *o_ptr);
bool obj_can_cast_from(const object_type *o_ptr);
bool obj_can_study(const object_type *o_ptr);
bool obj_can_takeoff(const object_type *o_ptr);
bool obj_can_wear(const object_type *o_ptr);
bool obj_can_fire(const object_type *o_ptr);
bool obj_has_inscrip(const object_type *o_ptr);
u16b object_effect(const object_type *o_ptr);
object_type *object_from_item_idx(int item);
bool obj_needs_aim(object_type *o_ptr);
bool item_is_available(int item, bool (*tester)(const object_type *), int mode);
extern unsigned check_for_inscrip(const object_type *o_ptr, const char *inscrip);

/* obj-ui.c */
extern void show_inven(olist_detail_t mode);
extern void show_equip(olist_detail_t mode);
extern void show_floor(int *floor_list, int floor_num, olist_detail_t mode);
extern bool verify_item(cptr prompt, int item);
extern bool get_item(int *cp, cptr pmt, cptr str, int mode);

/* object1.c */
extern void flavor_init(void);
extern byte proc_list_color_hack(object_type *o_ptr);
extern void reset_visuals(bool prefs);
extern void object_kind_name(char *buf, size_t max, int k_idx, bool easy_know);
extern void object_desc(char *buf, object_type *o_ptr, int pref, int mode);
extern void object_desc_spoil(char *buf, object_type *o_ptr, int pref, 
                              int mode);
extern void object_desc_store(char *buf, object_type *o_ptr, int pref, 
                              int mode);
extern void strip_name(char *buf, int k_idx, bool easy_know);
extern char index_to_label(int i);
extern s16b label_to_inven(int c);
extern s16b label_to_equip(int c);
extern s16b wield_slot(object_type *o_ptr);
extern cptr mention_use(int i);
extern cptr describe_use(int i);
extern bool item_tester_okay(object_type *o_ptr);
extern void toggle_inven_equip(void);
extern bool verify_item(cptr prompt, int item);
extern bool get_item_allow(int item);
extern bool scan_floor(int *items, int *item_num, int y, int x, int mode);
extern bool get_item_floor(int *cp, cptr pmt, cptr str, int mode);
extern char *object_adj(int tval, int sval);
extern bool check_set(byte s_idx);
extern void apply_set(int s_idx);
extern void remove_set(int s_idx);

/* object2.c */
extern void excise_object_idx(int o_idx);
extern void delete_object_idx(int o_idx);
extern void delete_object(int y, int x);
extern void compact_objects(int size);
extern void wipe_o_list(void);
extern s16b o_pop(void);
extern errr get_obj_num_prep(void);
extern s16b get_obj_num(int level);
extern void object_known(object_type *o_ptr);
extern void object_aware(object_type *o_ptr);
extern void object_tried(object_type *o_ptr);
extern s32b object_value(object_type *o_ptr);
extern void distribute_charges(object_type *o_ptr, object_type *q_ptr, 
                               int amt);
extern void reduce_charges(object_type *o_ptr, int amt);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern s16b lookup_kind(int tval, int sval);
extern void object_wipe(object_type *o_ptr);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern void object_prep(object_type *o_ptr, int k_idx);
extern s16b m_bonus(int max, int level);
extern void apply_magic(object_type *o_ptr, int lev, bool okay, bool good, 
                        bool great);
extern bool make_object(object_type *j_ptr, bool good, bool great, 
                        bool exact_kind);
extern bool make_gold(object_type *j_ptr);
extern s16b floor_carry(int y, int x, object_type *j_ptr);
extern void drop_near(object_type *j_ptr, int chance, int y, int x);
extern void acquirement(int y1, int x1, int num, bool great);
extern void place_object(int y, int x, bool good, bool great, bool exact_kind);
extern void place_gold(int y, int x);
extern void pick_trap(int y, int x);
extern void place_trap(int y, int x);
extern void inven_item_charges(int item);
extern void inven_item_describe(int item);
extern void inven_item_increase(int item, int num);
extern void inven_item_optimize(int item);
extern void floor_item_charges(int item);
extern void floor_item_describe(int item);
extern void floor_item_increase(int item, int num);
extern void floor_item_optimize(int item);
extern bool inven_carry_okay(object_type *o_ptr);
extern s16b inven_carry(object_type *o_ptr);
extern s16b inven_takeoff(int item, int amt);
extern void inven_drop(int item, int amt);
extern void combine_pack(void);
extern void reorder_pack(void);
extern int quiver_count(void);
extern void find_quiver_size(void);
extern int process_quiver(int num_new, object_type *o_ptr);
extern void display_koff(int k_idx);
extern void place_secret_door(int y, int x);
extern void place_unlocked_door(int y, int x);
extern void place_closed_door(int y, int x);
extern void place_random_door(int y, int x);
extern bool lookup_reverse(s16b k_idx, int *tval, int *sval);
extern int lookup_name(int tval, const char *name);
extern int lookup_artifact_name(const char *name);
extern int lookup_sval(int tval, const char *name);
extern int tval_find_idx(const char *name);
extern const char *tval_find_name(int tval);

#endif /* !INCLUDED_OBJECT_H */
