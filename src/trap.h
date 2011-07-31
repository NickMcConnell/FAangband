/* trap.h - trap interface */

#ifndef TRAP_H
#define TRAP_H

bool cave_trap_specific(int y, int x, int t_idx);
bool cave_basic_monster_trap(int y, int x);
bool cave_advanced_monster_trap(int y, int x);
bool cave_monster_trap(int y, int x);
int monster_trap_idx(int y, int x);
extern void hit_trap(int y, int x);
extern void pick_trap(int y, int x);
bool place_trap(int y, int x, int t_idx, int trap_level);
extern void py_steal(int y, int x);
extern bool py_set_trap(int y, int x);
extern bool py_modify_trap(int y, int x);
bool remove_trap(int y, int x, int t_idx);

#endif /* !TRAP_H */
