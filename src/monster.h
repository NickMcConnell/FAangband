#ifndef MONSTER_MONSTER_H
#define MONSTER_MONSTER_H

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

#endif /* !MONSTER_MONSTER_H */
