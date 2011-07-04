/* generate.h - dungeon generation interface */

#ifndef GENERATE_H
#define GENERATE_H

extern int level_hgt;
extern int level_wid;
extern char mon_symbol_at_depth[12][13];

extern char *mon_restrict(char symbol, byte depth, bool * ordered,
			  bool unique_ok);
extern void get_chamber_monsters(int y1, int x1, int y2, int x2);
extern void spread_monsters(char symbol, int depth, int num, int y0, int x0,
			    int dy, int dx);
extern void general_monster_restrictions(void);
extern void get_vault_monsters(char racial_symbol[], byte vault_type, cptr data,
			       int y1, int y2, int x1, int x2);


extern void generate_mark(int y1, int x1, int y2, int x2, int flg);
extern void place_unlocked_door(int y, int x);
extern void place_closed_door(int y, int x);
extern void destroy_level(bool new_level);
extern void generate_cave(void);

#endif /* !GENERATE_H */

