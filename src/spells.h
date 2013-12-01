/* spells.h - spell implementations and helpers */

#ifndef SPELLS_H
#define SPELLS_H

/** 1/x chance of reducing stats (for elemental attacks).  From Zangband
 * -LM-
 */
#define HURT_CHANCE	25

/** 
 * Maximum rune mana reserve 
 */
#define MAX_MANA_RESERVE 200


/*
 * Spell types used by project(), and related functions.
 */
enum
{
    #define GF(a) GF_##a,
    #include "list-gf-types.h"
    #undef GF
};

/**
 * Bolt motion (used in prefs.c, spells1.c)
 */
enum
{
    BOLT_NO_MOTION,
    BOLT_0,
    BOLT_45,
    BOLT_90,
    BOLT_135,
    BOLT_MAX
};


/*
 * Bit flags for the "project()" function
 *
 *   JUMP:  Jump directly to the target location (this is a hack)
 *   BEAM:  Work as a beam weapon (affect every grid passed through)
 *   ARC:   Act as an arc spell (a portion of a caster-centered ball)
 *   THRU:  Continue "through" the target (used for "bolts"/"beams")
 *   STOP:  Stop as soon as we hit a monster (used for "bolts")
 *   GRID:  Affect each grid in the "blast area" in some way
 *   ITEM:  Affect each object in the "blast area" in some way
 *   KILL:  Affect each monster in the "blast area" in some way
 *   PLAY:  Explicitly affect the player
 *   HIDE:  Hack -- disable "visual" feedback from projection
 *   CHCK:  Note occupied grids, but do not stop at them.
 */
#define PROJECT_NONE    0x0000
#define PROJECT_JUMP	0x0001
#define PROJECT_BEAM	0x0002
#define PROJECT_SAFE    0x0004
#define PROJECT_ARC	0x0008
#define PROJECT_THRU	0x0010
#define PROJECT_STOP	0x0020
#define PROJECT_GRID	0x0040
#define PROJECT_ITEM	0x0080
#define PROJECT_KILL	0x0100
#define PROJECT_PLAY	0x0200
#define PROJECT_HIDE	0x0400
#define PROJECT_CHCK	0x0800

/*
 * Bit flags for the "enchant()" function
 */
#define ENCH_TOHIT   0x01
#define ENCH_TODAM   0x02
#define ENCH_TOAC    0x04


/* spells1.c */
extern bool check_save(int roll);
extern s16b poly_r_idx(int r_idx, bool shapechange);
extern void teleport_away(int m_idx, int dis);
extern void teleport_player(int dis, bool safe);
extern void teleport_player_to(int ny, int nx, bool friendly);
extern void teleport_player_level(bool friendly);
int gf_name_to_idx(const char *name);
const char *gf_idx_to_name(int type);
extern bool chaotic_effects(monster_type *m_ptr);
extern void add_heighten_power(int value);
extern void add_speed_boost(int value);
extern int resist_damage(int dam, byte resist, byte rand_factor);
extern void take_hit(int dam, const char *kb_str);
extern void acid_dam(int dam, const char *kb_str);
extern void elec_dam(int dam, const char *kb_str);
extern void fire_dam(int dam, const char *kb_str);
extern void cold_dam(int dam, const char *kb_str);
extern void pois_dam(int dam, const char *kb_str);
extern bool pois_hit(int pois_inc);
extern bool inc_stat(int stat, bool star);
extern bool dec_stat(int stat, int amount, int permanent);
extern bool res_stat(int stat);
extern int remove_player_mana(int power);
extern int apply_dispel(int power);
extern bool apply_disenchant(int mode);
extern bool project(int who, int rad, int y, int x, int dam, int typ, 
                    int flg, int degrees_of_arc, byte diameter_of_source);
extern void teleport_towards(int oy, int ox, int ny, int nx);

/* spells2.c */
extern void shapechange(s16b shape);
extern bool choose_ele_resist(void);
extern void create_athelas(void);
extern void dimen_door(void);
void rebalance_weapon(void);
extern bool set_recall(int v);
extern bool hp_player(int num);
extern void magic_spiking(void);
extern bool lay_rune(int type);
extern bool do_dec_stat(int stat);
extern bool do_res_stat(int stat);
extern bool do_inc_stat(int stat, bool star);
extern void identify_object(object_type *o_ptr);
extern void identify_pack(void);
extern bool remove_curse(void);
extern bool remove_curse_good(void);
extern bool restore_level(void);
extern bool lose_all_info(void);
extern bool detect_traps(int range, bool show);
extern bool detect_doors(int range, bool show);
extern bool detect_stairs(int range, bool show);
extern bool detect_treasure(int range, bool show);
extern bool detect_objects_gold(int range, bool show);
extern bool detect_objects_normal(int range, bool show);
extern bool detect_objects_magic(int range, bool show);
extern bool detect_monsters_normal(int range, bool show);
extern bool detect_monsters_invis(int range, bool show);
extern bool detect_monsters_evil(int range, bool show);
extern bool detect_monsters_living(int range, bool show);
extern bool detect_all(int range, bool show);
extern void stair_creation(void);
extern bool enchant(object_type *o_ptr, int n, int eflag);
extern bool enchant_spell(int num_hit, int num_dam, int num_ac);
extern bool brand_missile(int ammo_type, int brand_type);
bool choose_ele_attack(void);
extern void set_ele_attack(u32b attack_type, int duration);
extern bool el_proof(bitflag *flag);
extern bool curse_armor(void);
extern bool curse_weapon(void);
extern bool ident_spell(void);
extern bool identify_fully(void);
extern bool recharge(int num);
extern void tap_magical_energy(void);
extern void do_starlight(int burst_number, int dam, bool strong);
extern bool listen_to_natural_creatures(void);
extern void grow_trees_and_grass(bool powerful);
extern void unmake(int dir);
extern void ele_air_smite(void);
extern bool project_los_not_player(int y1, int x1, int dam, int typ);
extern bool speed_monsters(void);
extern bool slow_monsters(int dam);
extern bool sleep_monsters(int dam);
extern bool fear_monsters(int dam);
extern bool confu_monsters(int dam);
extern bool banish_evil(int dist);
extern bool turn_undead(int dam);
extern bool turn_evil(int dam);
extern bool dispel_undead(int dam);
extern bool nature_strike(int dam);
extern bool dispel_evil(int dam);
extern bool dispel_demons(int dam);
extern bool dispel_not_evil(int dam);
extern bool dispel_monsters(int dam);
extern bool dispel_small_monsters(int dam);
extern bool dispel_living(int dam);
extern bool dispel_light_hating(int dam);
extern bool hold_undead(void);
extern bool hold_all(void);
extern bool poly_all(int dam);
extern bool teleport_all(int dam);
extern bool cacophony(int dam);
extern bool aggravate_monsters(int who, bool entire_level);
extern bool genocide(void);
extern bool mass_genocide(void);
extern bool probing(void);
extern void destroy_area(int y1, int x1, int r, bool full);
extern void earthquake(int cy, int cx, int r, bool volcano);
extern bool tremor(void);
extern void light_room(int y1, int x1);
extern void unlight_room(int y1, int x1);
extern bool light_area(int dam, int rad);
extern bool unlight_area(int dam, int rad);
extern bool fire_ball(int typ, int dir, int dam, int rad, bool jump);
extern bool fire_sphere(int typ, int dir, int dam, int rad, 
                        byte diameter_of_source);
extern bool fire_cloud(int typ, int dir, int dam, int rad);
extern bool fire_meteor(int who, int typ, int y, int x, int dam, int rad, 
                        bool hurt_player);
extern bool fire_arc(int typ, int dir, int dam, int rad, int degrees_of_arc);
extern bool fire_bolt(int typ, int dir, int dam);
extern bool fire_beam(int typ, int dir, int dam);
extern bool fire_bolt_or_beam(int prob, int typ, int dir, int dam);
extern bool light_line(int dir);
extern bool drain_life(int dir, int dam);
extern bool wall_to_mud(int dir);
extern bool destroy_door(int dir);
extern bool disarm_trap(int dir);
extern bool heal_monster(int dir);
extern bool speed_monster(int dir);
extern bool slow_monster(int dir, int dam);
extern bool sleep_monster(int dir, int dam);
extern bool confuse_monster(int dir, int dam);
extern bool poly_monster(int dir);
extern bool clone_monster(int dir);
extern bool fear_monster(int dir, int dam);
extern bool dispel_an_undead(int dir, int dam);
extern bool dispel_a_demon(int dir, int dam);
extern bool dispel_a_dragon(int dir, int dam);
extern bool teleport_monster(int dir, int dist);
extern bool door_creation(void);
extern bool trap_creation(void);
extern bool destroy_doors_touch(void);
extern bool sleep_monsters_touch(int dam);

/* x-spell.c */
extern int get_spell_index(const object_type *o_ptr, int index);
extern const char *get_spell_name(int index);
extern void get_spell_info(int tval, int index, char *buf, size_t len);
extern bool cast_spell(int tval, int index, int dir, int plev);
extern bool spell_needs_aim(int tval, int spell);

#endif /* !SPELLS_H */
