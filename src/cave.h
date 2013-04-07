/* cave.h - cave interface */

#ifndef CAVE_H
#define CAVE_H

#include "defines.h"
#include "z-type.h"

extern int distance(int y1, int x1, int y2, int x2);
extern bool los(int y1, int x1, int y2, int x2);
extern bool no_light(void);
extern bool cave_valid_bold(int y, int x);
extern byte get_color(byte a, int attr, int n);
extern void grid_data_as_text(grid_data *g, int *ap, wchar_t *cp, byte *tap, wchar_t *tcp);
extern void map_info(unsigned x, unsigned y, grid_data *g);
extern void move_cursor_relative(int y, int x);
extern void big_putch(int x, int y, byte a, char c);
extern void print_rel(wchar_t c, byte a, int y, int x);
extern void note_spot(int y, int x);
extern void light_spot(int y, int x);
extern void prt_map(void);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);
extern errr vinfo_init(void);
extern void forget_view(void);
extern void update_view(void);
extern void update_noise(void);
extern void update_smell(void);
extern void map_area(int y, int x, bool extended);
extern void wiz_light(bool wizard);
extern void wiz_dark(void);
extern void illuminate(void);
extern void cave_set_feat(int y, int x, int feat);
extern int project_path(u16b *gp, int range, \
                         int y1, int x1, int y2, int x2, int flg);
extern byte projectable(int y1, int x1, int y2, int x2, int flg);
extern void scatter(int *yp, int *xp, int y, int x, int d, int m);
extern void health_track(int m_idx);
extern void monster_race_track(int r_idx);
extern void track_object(int item);
extern void track_object_kind(int k_idx);
extern void disturb(int stop_search, int unused_flag);
extern bool is_quest(int stage);
extern bool dtrap_edge(int y, int x);


#endif /* !CAVE_H */
