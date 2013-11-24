#ifndef INCLUDED_EXTERNS_H
#define INCLUDED_EXTERNS_H

#include "spells.h"
#include "z-type.h"

/** \file externs.h 
    \brief Variable and function definitions.
 
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 *


 *
 * Note that some files have their own header files
 */


/*
 * Automatically generated "variable" declarations
 */

/* tables.c */
extern s16b ddd[9];
extern s16b ddx[10];
extern s16b ddy[10];
extern s16b ddx_ddd[9];
extern s16b ddy_ddd[9];
extern char hexsym[16];
extern byte adj_mag_fail[STAT_RANGE];
extern byte adj_mag_stat[STAT_RANGE];
extern byte adj_chr_gold[STAT_RANGE];
extern byte adj_dex_safe[STAT_RANGE];
extern byte adj_con_fix[STAT_RANGE];
extern byte extract_energy[200];
extern u32b resist_to_flag[14];
extern s32b player_exp[PY_MAX_LEVEL];
extern player_sex sex_info[MAX_SEXES];
extern byte deadliness_conversion[151];
extern int chest_traps[100];
extern const char *color_names[16];
extern int new_group_index[];
extern grouper group_item[];
extern const char *stat_names[A_MAX];
extern const char *stat_names_reduced[A_MAX];
extern const char *window_flag_desc[32];
extern druid_blows d_blow[NUM_D_BLOWS];
extern const char *magic_desc[REALM_MAX][MD_MAX];
extern const char *feel_text[FEEL_MAX];
extern const grouper object_text_order[];
extern byte mana_cost[RSF_MAX];
extern byte spell_desire[RSF_MAX][D_MAX];
extern byte spell_range[RSF_MAX];
extern int stage_map[NUM_STAGES][9];
extern int compressed_map[NUM_STAGES][9];
extern int dungeon_map[NUM_STAGES][9];
extern int extended_map[NUM_STAGES][9];
extern int fanilla_map[NUM_STAGES][9];
extern const char *locality_name[MAX_LOCALITIES];
extern const char *short_locality_name[MAX_LOCALITIES];
extern int towns[10];
extern int extended_towns[10];
extern int compressed_towns[10];
extern int race_town_prob[10][14];
extern byte type_of_store[MAX_STORES];
extern byte get_angle_to_grid[41][41];
const byte g_info[196];

/* variable.c */
extern const char *copyright;
extern byte version_major;
extern byte version_minor;
extern byte version_patch;
extern byte version_extra;
extern byte sf_major;
extern byte sf_minor;
extern byte sf_patch;
extern byte sf_extra;
extern bool arg_rebalance;
extern bool arg_wizard;
extern int arg_graphics;
extern bool arg_graphics_nice;
extern bool game_start;
extern bool character_generated;
extern bool character_dungeon;
extern bool character_loaded;
extern bool character_saved;
extern s16b character_icky;
extern s16b character_xtra;
extern u32b seed_flavor;
extern u32b seed_town[10];
extern s16b num_repro;
extern s16b object_level;
extern s16b monster_level;
extern wchar_t summon_kin_type;
extern s32b turn;
extern s32b old_turn;
extern int use_graphics;
extern bool use_transparency;
extern bool use_graphics_nice;
extern char notes_start[80];
extern s16b signal_count;
extern bool msg_flag;
extern bool inkey_base;
extern u32b inkey_scan;
extern bool inkey_flag;
extern s16b coin_type;
extern bool magic_throw;
extern bool opening_chest;
extern u16b j_level;
extern bool shimmer_monsters;
extern bool shimmer_objects;
extern bool repair_mflag_show;
extern bool repair_mflag_mark;
extern s16b o_max;
extern s16b o_cnt;
extern s16b m_max;
extern s16b m_cnt;
extern s16b trap_max;
extern s16b trap_cnt;
extern u16b group_id;
extern u16b feeling;
extern bool do_feeling;
extern s16b rating;
extern int player_uid;
extern int player_euid;
extern int player_egid;
extern char savefile[1024];
extern term *angband_term[TERM_WIN_MAX];
extern char angband_term_name[TERM_WIN_MAX][16];
extern byte angband_color_table[256][4];
extern color_type color_table[MAX_COLORS];
extern const const char *angband_sound_name[MSG_MAX];
extern int temp_n;
extern u16b *temp_g;
extern byte *temp_y;
extern byte *temp_x;
extern u16b (*adjacency)[NUM_STAGES];
extern u16b (*stage_path)[NUM_STAGES];
extern u16b (*temp_path)[NUM_STAGES];
extern u16b (*race_prob)[NUM_STAGES];
extern byte *dummy;
extern bitflag (*cave_info)[256][SQUARE_SIZE];
extern byte (*cave_feat)[DUNGEON_WID];
extern s16b (*cave_o_idx)[DUNGEON_WID];
extern s16b (*cave_m_idx)[DUNGEON_WID];

extern byte (*cave_cost)[DUNGEON_WID];
extern byte (*cave_when)[DUNGEON_WID];
extern int scent_when;
extern int flow_center_y;
extern int flow_center_x;
extern int update_center_y;
extern int update_center_x;
extern int cost_at_center;

extern maxima *z_info;
extern trap_type *trap_list;
extern object_type *o_list;
extern monster_type *m_list;
extern monster_lore *l_list;
extern quest *q_list;
extern store_type *store;
extern const char *** name_sections;
extern object_type *inventory;
extern object_type *quiver;
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_ego_size;
extern alloc_entry *alloc_ego_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern u32b alloc_race_total;
extern byte gf_to_attr[GF_MAX][BOLT_MAX];
extern wchar_t gf_to_char[GF_MAX][BOLT_MAX];
extern byte misc_to_attr[256];
extern char misc_to_char[256];
extern byte tval_to_attr[128];
extern player_sex *sp_ptr;
extern player_race *rp_ptr;
extern player_class *cp_ptr;
extern player_magic *mp_ptr;
extern player_other *op_ptr;
extern player_type *p_ptr;
extern int add_wakeup_chance;
extern u32b total_wakeup_chance;
extern vault_type *v_info;
extern vault_type *t_info;
extern feature_type *f_info;
extern trap_kind *trap_info;
extern object_kind *k_info;
extern artifact_type *a_info;
extern set_type *set_info;
extern ego_item_type *e_info;
extern monster_base *rb_info;
extern monster_race *r_info;
extern monster_pain *pain_messages;
extern hist_type *h_info;
extern player_race *p_info;
extern player_class *c_info;
extern owner_type *b_info;
extern spell_type *s_info;
extern flavor_type *flavor_info;
extern struct hint *hints;
extern struct pit_profile *pit_info;

extern const char *ANGBAND_SYS;
extern const char *ANGBAND_GRAF;

extern char *ANGBAND_DIR;
extern char *ANGBAND_DIR_APEX;
extern char *ANGBAND_DIR_BONE;
extern char *ANGBAND_DIR_DATA;
extern char *ANGBAND_DIR_EDIT;
extern char *ANGBAND_DIR_FILE;
extern char *ANGBAND_DIR_HELP;
extern char *ANGBAND_DIR_INFO;
extern char *ANGBAND_DIR_SAVE;
extern char *ANGBAND_DIR_PREF;
extern char *ANGBAND_DIR_USER;
extern char *ANGBAND_DIR_XTRA;

extern char *ANGBAND_DIR_XTRA_FONT;
extern char *ANGBAND_DIR_XTRA_GRAF;
extern char *ANGBAND_DIR_XTRA_SOUND;
extern char *ANGBAND_DIR_XTRA_HELP;
extern char *ANGBAND_DIR_XTRA_ICON;

extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(const object_type*);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);
extern ang_file *text_out_file;
extern void (*text_out_hook)(byte a, const char *str);
extern int text_out_wrap;
extern int text_out_indent;
extern int text_out_pad;
extern int dump_row;
extern ang_file *dump_out_file;
extern char_attr *dump_ptr;
extern void (*dump_line_hook)(char_attr *this_line);
extern char_attr_line *dumpline;
extern char_attr_line *pline0;
extern char_attr_line *pline1;
void text_out_dump(byte a, char *str, char_attr_line **line, 
		   int *current_line, int indent, int wrap);
void textblock_dump(textblock *tb, char_attr_line **line, int *current_line, 
		    int indent, int wrap);
extern char themed_feeling[80];
extern byte required_tval;
extern byte bones_selector;
extern int r_ghost;
extern char ghost_name[80];
extern int ghost_string_type;
extern char ghost_string[80];
extern bool ghost_has_spoken;
extern bool is_autosave;
extern bool autosave;
extern s16b autosave_freq;
extern byte number_of_thefts_on_level;
extern byte num_trap_on_level;
extern byte num_runes_on_level[RUNE_TAIL];
extern int mana_reserve;
extern int *artifact_normal, *artifact_special;
extern int artifact_normal_cnt, artifact_special_cnt;
extern bool angband_keymap_flag;
extern void (*sound_hook)(int);
extern autoinscription *inscriptions;
extern u16b inscriptions_count;

/* util.c */
extern struct keypress *inkey_next;

/*
 * Automatically generated "function declarations"
 */

/* attack.c */
extern bool py_attack(int y, int x, bool can_push);

/* birth.c */
extern void player_birth(bool quickstart_allowed);

/* charattr.c */
extern void x_fprintf(ang_file *f, int encoding, const char *fmt, ...);
extern void dump_put_str(byte attr, const char *str, int col);
extern void dump_lnum(char *header, s32b num, int col, byte color);
extern void dump_num(char *header, int num, int col, byte color);
extern void dump_deci(char *header, int num, int deci, int col, byte color);
extern void dump_line_file(char_attr *this_line);
extern void dump_line_screen(char_attr *this_line);
extern void dump_line_mem(char_attr *this_line);
extern void dump_line(char_attr *this_line);
extern int handle_item(void);
extern int make_metric(int wgt);


/* cmd1.c */
bool search(bool verbose);
extern byte py_pickup(int pickup, int y, int x);
extern void fall_off_cliff(void);
extern void move_player(int dir);

/* cmd2.c */
/* XXX should probably be moved to cave.c? */
bool is_open(int feat);
bool is_closed(int feat);
bool is_trap(int feat);
int count_feats(int *y, int *x, int flag, bool under);
int count_traps(int *y, int *x);
int count_chests(int *y, int *x, bool trapped);
int coords_to_dir(int y, int x);

/* death.c */
void death_screen(void);

/* monattk.c */
extern bool make_attack_normal(monster_type *m_ptr, int y, int x);
extern bool make_attack_ranged(monster_type *m_ptr, int attack);

/* monmove.c */
extern int get_scent(int y, int x);
extern int choose_ranged_attack(int m_idx, bool archery_only, int shape_rate);
extern bool cave_exist_mon(monster_race *r_ptr, int y, int x, 
                           bool occupied_ok);
extern void process_monsters(byte minimum_energy);
extern void reset_monsters(void);

/* pathfind.c */
extern bool findpath(int y, int x);
extern int get_angle_to_target(int y0, int x0, int y1, int x1, int dir);
extern void get_grid_using_angle(int angle, int y0, int x0,
	int *ty, int *tx);
extern void run_step(int dir);

/* randart.c */
extern void initialize_random_artifacts(void);

/* score.c */
extern void enter_score(time_t *death_time);
extern void show_scores(void);
extern void predict_score(void);

/* store.c */
s32b price_item(object_type * o_ptr, int greed, bool flip);
extern void store_shuffle(int which);
extern void store_maint(int which);
extern void stores_maint(int times);
extern void store_init(void);

/* util.c */
extern void text_to_ascii(char *buf, size_t len, const char *str);
extern void ascii_to_text(char *buf, size_t len, const char *str);
extern void flush(void);
extern void anykey(void);
extern struct keypress inkey(void);
extern ui_event inkey_ex(void);
extern void bell(const char *reason);
extern void sound(int val);
extern void msg(const char *fmt, ...);
extern void msgt(unsigned int type, const char *fmt, ...);
extern void message_flush(void);
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(byte attr, const char *str, int row, int col);
extern void put_str(const char *str, int row, int col);
extern void c_prt(byte attr, const char *str, int row, int col);
extern void prt(const char *str, int row, int col);
extern void text_out_to_file(byte attr, const char *str);
extern void text_out_to_screen(byte a, const char *str);
extern void text_out(const char *fmt, ...);
extern void text_out_c(byte a, const char *fmt, ...);
extern void text_out_e(const char *fmt, ...);
extern void clear_from(int row);
extern bool askfor_aux_keypress(char *buf, size_t buflen, size_t *curs, size_t *len, struct keypress keypress, bool firsttime);
extern bool askfor_aux(char *buf, size_t len, bool keypress_h(char *, size_t, size_t *, size_t *, struct keypress, bool));
extern bool get_string(const char *prompt, char *buf, size_t len);
extern s16b get_quantity(const char *prompt, int max);
extern char get_char(const char *prompt, const char *options, size_t len, char fallback);
extern bool get_check(const char *prompt);
extern bool (*get_file)(const char *suggested_name, char *path, size_t len);
extern bool get_com(const char *prompt, struct keypress *command);
extern bool get_com_ex(const char *prompt, ui_event *command);
extern void pause_line(struct term *term);
extern bool is_a_vowel(int ch);
extern int color_char_to_attr(char c);
extern int color_text_to_attr(const char *name);
extern const char *attr_to_text(byte a);
extern bool char_matches_key(wchar_t c, keycode_t key);

#ifdef SUPPORT_GAMMA
extern byte gamma_table[256];
extern void build_gamma_table(int gamma);
#endif /* SUPPORT_GAMMA */

/* x-char.c */
extern void xchar_trans_hook(char *s, int encoding);
extern void xstr_trans(char *str, int encoding);
extern void escape_latin1(char *dest, size_t max, const char *src);
extern const char seven_bit_translation[128];
extern char xchar_trans(byte c);

/* xtra2.c */
extern void check_experience(void);
extern void gain_exp(s32b amount);
extern void lose_exp(s32b amount);
bool word_recall(int v);
int motion_dir(int y1, int x1, int y2, int x2);
extern int target_dir(struct keypress ch);
extern bool get_rep_dir(int *dp);
extern bool confuse_dir(int *dp);
extern void center_panel(void);
extern bool change_panel(int dir);
extern void verify_panel(void);
bool modify_panel(term *t, int wy, int wx);
void town_adjust(int *dungeon_hgt, int *dungeon_wid);

/* xtra3.c */
void cnv_stat(int val, char *out_val, size_t out_len);
byte player_hp_attr(void);
byte player_sp_attr(void);
byte monster_health_attr(void);
void toggle_inven_equip(void);
void subwindows_set_flags(u32b *new_flags, size_t n_subwindows);
char* random_hint(void);
void show_news(void);

/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef NDS
extern void debug_a(void);
#endif


#ifdef ALLOW_DEBUG
/* wizard2.c */
extern void do_cmd_debug(void);
#endif /* ALLOW_DEBUG */
bool jump_menu(int level, int *location);



/* wizard1.c */
#ifdef ALLOW_SPOILERS
extern void do_cmd_spoilers(void);
#endif /* ALLOW_SPOILERS */

bool make_fake_artifact(object_type *o_ptr, int name1);

extern u16b lazymove_delay;


#endif /* !INCLUDED_EXTERNS_H */

