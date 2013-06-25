/* player/player.h - player interface */

#ifndef PLAYER_PLAYER_H
#define PLAYER_PLAYER_H

/* play-calcs.c */
extern const byte adj_dex_dis[STAT_RANGE];
extern const byte adj_dex_ta[STAT_RANGE];
extern const byte adj_str_td[STAT_RANGE];
extern const byte adj_dex_th[STAT_RANGE];
extern const byte adj_str_wgt[STAT_RANGE];
extern const byte adj_str_hold[STAT_RANGE];
extern const byte adj_str_blow[STAT_RANGE];

extern void apply_resist(int *player_resist, int item_resist);
void calc_bonuses(object_type inventory[], player_state *state, bool id_only);
int calc_blows(const object_type *o_ptr, player_state *state, int extra_blows);
void notice_stuff(struct player *p);
void update_stuff(struct player *p);
void redraw_stuff(struct player *p);
void handle_stuff(struct player *p);
int weight_remaining(void);


/* play-spell.c */
int spell_collect_from_book(const object_type *o_ptr, int spells[PY_MAX_SPELLS]);
int spell_book_count_spells(const object_type *o_ptr, bool (*tester)(int spell));
bool spell_okay_list(bool (*spell_test)(int spell), const int spells[], int n_spells);
bool spell_okay_to_cast(int spell);
bool spell_okay_to_study(int spell);
bool spell_okay_to_browse(int spell);
bool spell_in_book(int spell, int book);
s16b spell_chance(int spell);
void spell_learn(int spell);
bool spell_cast(int spell, int dir);

/* play-timed.c */
bool set_timed(int idx, int v, bool notify);
bool inc_timed(int idx, int v, bool notify);
bool dec_timed(int idx, int v, bool notify);
bool clear_timed(int idx, bool notify);
bool set_food(int v);

/* play-util.c */
s16b modify_stat_value(int value, int amount);
bool player_can_cast(void);
bool player_can_study(void);
bool player_can_read(void);
bool player_can_fire(void);
extern const char *player_safe_name(struct player *p);

#endif /* !PLAYER_PLAYER_H */
