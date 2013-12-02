/* trap.h - trap interface */

#ifndef TRAP_H
#define TRAP_H

/*** Trap Indexes (see "lib/edit/trap.txt") ***/

/** Nothing */
#define TRAP_NONE     0x00

/* Runes  */
#define RUNE_HEAD     0x01
#define RUNE_TAIL     0x07
#define RUNE_ELEMENTS 0x01
#define RUNE_MAGDEF   0x02
#define RUNE_QUAKE    0x03
#define RUNE_MANA     0x04
#define RUNE_PROTECT  0x05
#define RUNE_POWER    0x06
#define RUNE_SPEED    0x07

/* Other obstructions */
#define OBST_WEB      0x0A

/* Traps */
#define TRAP_HEAD     0x10
#define TRAP_TAIL     0x1F
#define TRAP_TRAPDOOR 0x10
#define TRAP_PIT      0x11
#define TRAP_DART     0x12
#define TRAP_SPOT     0x13
#define TRAP_GAS      0x14
#define TRAP_SUMMON   0x15
#define TRAP_ALTER    0x16
#define TRAP_HEX      0x17
#define TRAP_PORTAL   0x18
#define TRAP_MURDER   0x19
#define TRAP_BRANCH   0x1A

/* Specials trap that only effects monsters.  Created only by rogues. -LM- */
#define MTRAP_HEAD       0x40
#define MTRAP_TAIL       0x4F
#define MTRAP_BASE       0x40 /* Level 1 */
#define MTRAP_STURDY     0x41 /* Level 1 */
#define MTRAP_NET        0x42 /* Level 6 */
#define MTRAP_CONF       0x43 /* Level 12 */
#define MTRAP_POISON     0x44 /* Level 18 */
#define MTRAP_SPIRIT     0x45 /* Level 24 */
#define MTRAP_ELEC       0x46 /* Level 30 */
#define MTRAP_EXPLOSIVE  0x47 /* Level 36 */
#define MTRAP_PORTAL     0x48 /* Level 42 */
#define MTRAP_STASIS     0x49 /* Level 48 */
#define MTRAP_DRAIN_LIFE 0x4A /* Level * */
#define MTRAP_UNMAGIC    0x4B /* Level * */
#define MTRAP_DISPEL_M   0x4C /* Level * */
#define MTRAP_GENOCIDE   0x4D /* Level * */


/*** Trap flags ***/

enum
{
	#define TRF(a,b) TRF_##a,
	#include "list-trap-flags.h"
	#undef TRF
	TRF_MAX
};

#define TRF_SIZE                FLAG_SIZE(TRF_MAX)

#define trf_has(f, flag)        flag_has_dbg(f, TRF_SIZE, flag, #f, #flag)
#define trf_next(f, flag)       flag_next(f, TRF_SIZE, flag)
#define trf_is_empty(f)         flag_is_empty(f, TRF_SIZE)
#define trf_is_full(f)          flag_is_full(f, TRF_SIZE)
#define trf_is_inter(f1, f2)    flag_is_inter(f1, f2, TRF_SIZE)
#define trf_is_subset(f1, f2)   flag_is_subset(f1, f2, TRF_SIZE)
#define trf_is_equal(f1, f2)    flag_is_equal(f1, f2, TRF_SIZE)
#define trf_on(f, flag)         flag_on_dbg(f, TRF_SIZE, flag, #f, #flag)
#define trf_off(f, flag)        flag_off(f, TRF_SIZE, flag)
#define trf_wipe(f)             flag_wipe(f, TRF_SIZE)
#define trf_setall(f)           flag_setall(f, TRF_SIZE)
#define trf_negate(f)           flag_negate(f, TRF_SIZE)
#define trf_copy(f1, f2)        flag_copy(f1, f2, TRF_SIZE)
#define trf_union(f1, f2)       flag_union(f1, f2, TRF_SIZE)
#define trf_comp_union(f1, f2)  flag_comp_union(f1, f2, TRF_SIZE)
#define trf_inter(f1, f2)       flag_inter(f1, f2, TRF_SIZE)
#define trf_diff(f1, f2)        flag_diff(f1, f2, TRF_SIZE)


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


bool cave_trap_specific(int y, int x, int t_idx);
bool cave_basic_monster_trap(int y, int x);
bool cave_advanced_monster_trap(int y, int x);
bool cave_monster_trap(int y, int x);
int monster_trap_idx(int y, int x);
bool cave_visible_trap(int y, int x);
bool cave_invisible_trap(int y, int x);
bool cave_player_trap(int y, int x);
int visible_trap_idx(int y, int x);
bool cave_web(int y, int x);
bool get_trap_graphics(int t_idx, int *a, wchar_t *c, bool require_visible);
extern bool reveal_trap(int y, int x, int chance, bool msg);
extern void hide_trap_idx(int idx);
extern int num_traps(int y, int x, int vis);
extern void hit_trap(int y, int x);
bool place_trap(int y, int x, int t_idx, int trap_level);
extern void py_steal(int y, int x);
extern bool py_set_trap(int y, int x);
extern bool py_modify_trap(int y, int x);
bool get_trap(int y, int x, int *idx);
bool remove_trap(int y, int x, bool msg, int t_idx);
void remove_trap_kind(int y, int x, bool msg, int t_idx);
void wipe_trap_list(void);

#endif /* !TRAP_H */
