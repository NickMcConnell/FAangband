/* trap.h - trap interface */

#ifndef TRAP_H
#define TRAP_H

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
