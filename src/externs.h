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

extern int max_macrotrigger;
extern char *macro_template;
extern char *macro_modifier_chr;
extern char *macro_modifier_name[MAX_MACRO_MOD];
extern char *macro_trigger_name[MAX_MACRO_TRIGGER];
extern char *macro_trigger_keycode[2][MAX_MACRO_TRIGGER];

/* tables.c */
extern s16b ddd[9];
extern s16b ddx[10];
extern s16b ddy[10];
extern s16b ddx_ddd[9];
extern s16b ddy_ddd[9];
extern char hexsym[16];
extern byte adj_mag_study[];
extern byte adj_mag_mana[];
extern byte adj_mag_fail[];
extern byte adj_mag_stat[];
extern byte adj_chr_gold[];
extern byte adj_int_dev[];
extern byte adj_wis_sav[];
extern byte adj_dex_dis[];
extern byte adj_int_dis[];
extern byte adj_dex_ta[];
extern byte adj_str_td[];
extern byte adj_dex_th[];
extern byte adj_str_th[];
extern byte adj_str_wgt[];
extern byte adj_str_hold[];
extern byte adj_str_dig[];
extern byte adj_str_blow[];
extern byte adj_dex_shots[];
extern byte adj_dex_blow[];
extern byte adj_dex_safe[];
extern byte adj_con_fix[];
extern byte adj_con_mhp[];
extern byte adj_dex_evas[];
extern byte blows_table[12][12];
extern byte extract_energy[200];
extern u32b resist_to_flag[14];
extern s32b player_exp[PY_MAX_LEVEL];
extern player_sex sex_info[MAX_SEXES];
extern player_magic magic_info[MAX_CLASS];
extern cptr spell_names[255];
extern byte deadliness_conversion[151];
extern int chest_traps[100];
extern cptr color_names[16];
extern int new_group_index[];
extern grouper group_item[];
extern cptr stat_names[A_MAX];
extern cptr stat_names_reduced[A_MAX];
extern cptr window_flag_desc[32];
extern cptr option_text[OPT_MAX];
extern cptr option_desc[OPT_MAX];
extern bool option_norm[OPT_MAX];
extern byte option_page[OPT_PAGE_MAX][OPT_PAGE_PER];
extern druid_blows d_blow[NUM_D_BLOWS];
extern cptr feel_text[FEEL_MAX];
extern const grouper object_text_order[];
extern byte mana_cost_RF4[32];
extern byte mana_cost_RF5[32];
extern byte mana_cost_RF6[32];
extern byte mana_cost_RF7[32];
extern byte spell_desire_RF4[32][8];
extern byte spell_desire_RF5[32][8];
extern byte spell_desire_RF6[32][8];
extern byte spell_desire_RF7[32][8];
extern byte spell_range_RF4[32];
extern byte spell_range_RF5[32];
extern byte spell_range_RF6[32];
extern byte spell_range_RF7[32];
extern cptr specialty_names[TOTAL_SPECIALTIES];
extern int stage_map[NUM_STAGES][9];
extern int dungeon_map[NUM_STAGES][9];
extern cptr locality_name[MAX_LOCALITIES];
extern cptr short_locality_name[MAX_LOCALITIES];
extern int towns[10];
extern int race_town_prob[10][14];
byte type_of_store[MAX_STORES];

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
extern u32b sf_xtra;
extern u32b sf_when;
extern u16b sf_lives;
extern u16b sf_saves;
extern bool arg_fiddle;
extern bool arg_wizard;
extern bool arg_sound;
extern bool arg_graphics;
extern bool arg_graphics_nice;
extern bool arg_monochrome;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool game_start;
extern bool character_quickstart;
extern bool character_generated;
extern bool character_dungeon;
extern bool character_loaded;
extern bool character_saved;
extern s16b character_icky;
extern s16b character_xtra;
extern u32b seed_flavor;
extern u32b seed_town[10];
extern s16b min_hgt;
extern s16b max_hgt;
extern s16b min_wid;
extern s16b max_wid;
extern s16b num_repro;
extern s16b object_level;
extern s16b monster_level;
extern char summon_kin_type;
extern s32b turn;
extern bool is_daylight;
extern s32b old_turn;
extern int use_graphics;
extern bool use_transparency;
extern bool use_graphics_nice;
extern bool use_bigtile;
extern bool use_dbltile;
extern bool use_trptile;
extern bool small_screen;
extern char notes_start[80];
extern int image_count;
extern s16b signal_count;
extern bool msg_flag;
extern bool inkey_base;
extern bool inkey_xtra;
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
extern u16b group_id;
extern s16b screen_x;
extern s16b screen_y;
extern s16b feeling;
extern s32b do_feeling;
extern s16b rating;
extern bool empty_level;
extern bool good_item_flag;
extern bool closing_flag;
extern bool fake_monochrome;
extern s16b max_panel_rows, max_panel_cols;
extern s16b panel_row_min, panel_row_max;
extern s16b panel_col_min, panel_col_max;
extern s16b panel_col_prt, panel_row_prt;
extern bool panel_extra_rows;
extern int player_uid;
extern int player_euid;
extern int player_egid;
extern char savefile[1024];
extern s16b macro__num;
extern char **macro__pat;
extern char **macro__act;
extern term *angband_term[TERM_WIN_MAX];
extern char angband_term_name[TERM_WIN_MAX][16];
extern byte angband_color_table[256][4];
extern char angband_sound_name[SOUND_MAX][16];
extern sint view_n;
extern u16b *view_g;
extern int  vinfo_grids;
extern int  vinfo_slopes;
extern u32b vinfo_bits_3;
extern u32b vinfo_bits_2;
extern u32b vinfo_bits_1;
extern u32b vinfo_bits_0;
extern sint temp_n;
extern u16b *temp_g;
extern byte *temp_y;
extern byte *temp_x;
extern u16b (*adjacency)[NUM_STAGES];
extern u16b (*stage_path)[NUM_STAGES];
extern u16b (*temp_path)[NUM_STAGES];
extern u16b (*race_prob)[32];
extern byte (*cave_info)[256];
extern byte (*cave_info2)[256];
extern byte (*cave_feat)[DUNGEON_WID];
extern s16b (*cave_o_idx)[DUNGEON_WID];
extern s16b (*cave_m_idx)[DUNGEON_WID];

#ifdef MONSTER_FLOW

extern byte (*cave_cost)[DUNGEON_WID];
extern byte (*cave_when)[DUNGEON_WID];
extern int scent_when;
extern int flow_center_y;
extern int flow_center_x;
extern int update_center_y;
extern int update_center_x;
extern int cost_at_center;

#endif

extern maxima *z_info;
extern object_type *o_list;
extern monster_type *m_list;
extern monster_lore *l_list;
extern quest *q_list;
extern note_info *notes;
extern store_type *store;
extern object_type *inventory;
extern object_type *quiver;
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_ego_size;
extern alloc_entry *alloc_ego_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern u32b alloc_race_total;
extern byte misc_to_attr[256];
extern char misc_to_char[256];
extern byte tval_to_attr[128];
extern char macro_buffer[1024];
extern char *keymap_act[KEYMAP_MODES][256];
extern player_sex *sp_ptr;
extern player_race *rp_ptr;
extern player_class *cp_ptr;
extern player_magic *mp_ptr;
extern player_other *op_ptr;
extern player_type *p_ptr;
extern int add_wakeup_chance;
extern u32b total_wakeup_chance;
extern vault_type *v_info;
extern char *v_name;
extern char *v_text;
extern vault_type *t_info;
extern char *t_name;
extern char *t_text;
extern feature_type *f_info;
extern char *f_name;
extern char *f_text;
extern object_kind *k_info;
extern char *k_name;
extern char *k_text;
extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;
extern set_type *s_info;
extern char *s_name;
extern char *s_text;
extern ego_item_type *e_info;
extern char *e_name;
extern char *e_text;
extern monster_race *r_info;
extern hist_type *h_info;
extern char *h_text;
extern player_race *p_info;
extern char *p_name;
extern char *p_text;
extern player_class *c_info;
extern char *c_name;
extern char *c_text;
extern char *r_name;
extern char *r_text;
extern owner_type *b_info;
extern char *b_name;
extern byte *g_info;
extern flavor_type *flavor_info;
extern char *flavor_name;
extern char *flavor_text;

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

extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(object_type*);
extern bool (*ang_sort_comp)(vptr u, vptr v, int a, int b);
extern void (*ang_sort_swap)(vptr u, vptr v, int a, int b);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);
extern ang_file *text_out_file;
extern void (*text_out_hook)(byte a, char *str);
extern int text_out_wrap;
extern int text_out_indent;
extern int dump_row;
extern ang_file *dump_out_file;
extern char_attr *dump_ptr;
extern void (*dump_line_hook)(char_attr *this_line);
extern char_attr_line *dumpline;
extern char_attr_line *pline0;
extern char_attr_line *pline1;
extern ang_file *highscore_fd;
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
extern byte player_is_crossing;
extern byte number_of_thefts_on_level;
extern byte num_trap_on_level;
extern byte num_runes_on_level[MAX_RUNE];
extern int mana_reserve;
extern int *artifact_normal, *artifact_special;
extern int artifact_normal_cnt, artifact_special_cnt;
extern bool angband_keymap_flag;
extern char pf_result[MAX_PF_LENGTH];
extern int pf_result_index;
extern void (*sound_hook)(int);
extern autoinscription *inscriptions;
extern u16b inscriptions_count;
extern mouse_button *mse_button;
extern mouse_button *backup_button;
extern int status_end;
extern int depth_start;
extern int button_length;
extern int num_buttons;
extern int prompt_end;
extern bool normal_screen;
extern int (*add_button_hook)(char *label, unsigned char keypress);
extern int (*kill_button_hook)(unsigned char keypress);
extern void (*kill_all_buttons_hook)(void);
extern void (*backup_buttons_hook)(void);
extern void (*restore_buttons_hook)(void);

/*
 * Automatically generated "function declarations"
 */

/* attack.c */
extern bool py_attack(int y, int x, bool can_push);
extern void do_cmd_fire(void);
extern void do_cmd_throw(void);

/* birth.c */
extern void player_birth(void);

/* cave.c */
extern sint distance(int y1, int x1, int y2, int x2);
extern bool los(int y1, int x1, int y2, int x2);
extern bool no_lite(void);
extern bool cave_valid_bold(int y, int x);
extern void map_info(int y, int x, byte *ap, char *cp, byte *tap, char *tcp);
extern void map_info_default(int y, int x, byte *ap, char *cp);
extern void move_cursor_relative(int y, int x);
extern void big_putch(int x, int y, byte a, char c);
extern void print_rel(char c, byte a, int y, int x);
extern void note_spot(int y, int x);
extern void lite_spot(int y, int x);
extern void prt_map(void);
extern void display_map(int *cy, int *cx, bool smll);
extern void do_cmd_view_map(void);
extern errr vinfo_init(void);
extern void forget_view(void);
extern void update_view(void);
extern void update_noise(void);
extern void update_smell(void);
extern void map_area(int y, int x, bool extended);
extern void wiz_lite(bool wizard);
extern void wiz_dark(void);
extern void town_illuminate(bool daytime, bool cave);
extern void stage_illuminate(bool daytime);
extern void cave_set_feat(int y, int x, int feat);
extern sint project_path(u16b *gp, int range, \
                         int y1, int x1, int y2, int x2, int flg);
extern byte projectable(int y1, int x1, int y2, int x2, int flg);
extern void scatter(int *yp, int *xp, int y, int x, int d, int m);
extern void health_track(int m_idx);
extern void monster_race_track(int r_idx);
extern void object_kind_track(int k_idx);
extern void disturb(int stop_search, int unused_flag);
extern bool is_quest(int stage);

/* cmd1.c */
extern void search(void);
extern void py_pickup_aux(int o_idx);
extern byte py_pickup(int pickup, int y, int x);
extern void hit_trap(int y, int x);
extern void fall_off_cliff(void);
extern void move_player(int dir, int do_pickup);
extern void run_step(int dir);


/* dungeon.c */
extern void play_game(bool new_game);

/* files.c */
extern void html_screenshot(cptr name);
extern void safe_setuid_drop(void);
extern void safe_setuid_grab(void);
extern s16b tokenize(char *buf, s16b num, char **tokens);
extern errr check_time(void);
extern errr check_time_init(void);
extern void display_player(int mode);
extern void display_player_sml(void);
extern int make_dump(char_attr_line *line, int mode);
extern void display_dump(char_attr_line *line, int top_line, int bottom_line, 
			 int col);
extern errr file_character(cptr name, char_attr_line *line, int last_line);
extern bool show_file(cptr name, cptr what, int line, int mode);
extern void do_cmd_help(void);
extern void process_player_name(bool sf);
extern bool get_name(bool sf);
extern void do_cmd_suicide(void);
extern void do_cmd_save_game(void);
extern void display_scores(int from, int to);
extern void close_game(void);
extern void exit_game_panic(void);
extern errr get_rnd_line(char *file_name, char *output);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);

/* generate.c */
extern void destroy_level(bool new_level);
extern void generate_cave(void);

/* identify.c */
extern bool item_dubious(const object_type *o_ptr, bool unknown);
extern int value_check_aux1(object_type *o_ptr);
/* SJGU */
extern int value_check_aux2(object_type *o_ptr);
extern bool has_ego_properties(object_type *o_ptr);
extern u32b flags_other(object_type *o_ptr);
extern void notice_curse(u32b curse_flag, int item);
extern void notice_obj(u32b obj_flag, int item);
extern void notice_other(u32b other_flag, int item);

/* info.c */
extern char *obj_class_info[101];
extern char *obj_special_info[4][50];
extern char *spell_tips[];
extern char *specialty_tips[TOTAL_SPECIALTIES];
extern void output_dam(object_type *o_ptr, int mult, const char *against, 
                       bool *first);
extern void display_weapon_damage(object_type *o_ptr);
extern void output_ammo_dam(object_type *o_ptr, int mult, const char *against, 
                            bool *first, bool perfect);
extern void display_ammo_damage(object_type *o_ptr);
extern void object_info(char buf[2048], object_type *o_ptr, bool in_store);
extern void object_info_screen(object_type *o_ptr, bool fake);
extern char *item_activation(object_type *o_ptr);
extern void identify_fully_aux(object_type *o_ptr);
extern void self_knowledge(bool spoil);
extern s16b spell_chance(int spell);
extern bool spell_okay(int spell, bool known);
extern void spell_info(char *p, int spell);
extern void print_spells(int tval, int sval, int y, int x);
extern void do_cmd_view_abilities(void);

/* init2.c */
extern void init_artifacts(void);
extern errr init_t_info(byte chosen_level);
extern void kill_t_info(void);
extern void init_file_paths(const char *path);
extern void create_user_dirs(void);
extern void init_angband(void);
extern void cleanup_angband(void);

/* jewel.c */
extern bool design_ring_or_amulet(object_type *o_ptr, int lev);

/* load.c */
extern errr rd_savefile_new(void);
extern errr rd_version_info(void);

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

/* monster1.c */
extern void roff_top(int r_idx);
extern void screen_roff(int r_idx);
extern void display_roff(int r_idx);
extern bool prepare_ghost(int r_idx, monster_type *m_ptr, bool from_savefile);

/* monster2.c */
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
extern void monster_desc(char *desc, monster_type *m_ptr, int mode);
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

/* object1.c */
extern void flavor_init(void);
byte proc_list_color_hack(object_type *o_ptr);
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
extern void display_inven(void);
extern void display_equip(void);
extern void show_inven(void);
extern void show_equip(void);
extern void toggle_inven_equip(void);
extern bool verify_item(cptr prompt, int item);
extern bool get_item_allow(int item);
extern bool get_item(int *cp, cptr pmt, cptr str, int mode);
extern bool get_item_tk(int *cp, cptr pmt, cptr str, int y, int x);
extern bool scan_floor(int *items, int *item_num, int y, int x, int mode);
extern void show_floor(int *floor_list, int floor_num);
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

/* prefs.c */
void autoinsc_dump(ang_file *fff);
void squelch_dump(ang_file *fff);
void option_dump(ang_file *fff);
void macro_dump(ang_file *fff);
void keymap_dump(ang_file *fff);
void dump_monsters(ang_file *fff);
void dump_objects(ang_file *fff);
void dump_features(ang_file *fff);
void dump_flavors(ang_file *fff);
void dump_colors(ang_file *fff);
bool prefs_save(const char *path, void (*dump)(ang_file *), const char *title);
s16b tokenize(char *buf, s16b num, char **tokens);
errr process_pref_file_command(char *buf);
errr process_pref_file(cptr name);

/* randart.c */
extern void initialize_random_artifacts(void);

/* save.c */
extern bool save_player(void);
extern bool load_player(void);

/* score.c */
extern void enter_score(time_t *death_time);
extern void show_scores(void);
extern void predict_score(void);

/* spells1.c */
extern bool check_save(int roll);
extern s16b poly_r_idx(int r_idx, bool shapechange);
extern void teleport_away(int m_idx, int dis);
extern void teleport_player(int dis, bool safe);
extern void teleport_player_to(int ny, int nx, bool friendly);
extern void teleport_player_level(bool friendly);
extern bool chaotic_effects(monster_type *m_ptr);
extern void add_heighten_power(int value);
extern void add_speed_boost(int value);
extern int resist_damage(int dam, byte resist, byte rand_factor);
extern void take_hit(int dam, cptr kb_str);
extern void acid_dam(int dam, cptr kb_str);
extern void elec_dam(int dam, cptr kb_str);
extern void fire_dam(int dam, cptr kb_str);
extern void cold_dam(int dam, cptr kb_str);
extern void pois_dam(int dam, cptr kb_str);
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
extern void set_ele_attack(u32b attack_type, int duration);
extern bool el_proof(u32b flag);
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
extern void lite_room(int y1, int x1);
extern void unlite_room(int y1, int x1);
extern bool lite_area(int dam, int rad);
extern bool unlite_area(int dam, int rad);
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
extern bool lite_line(int dir);
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

/* store.c */
extern void do_cmd_store(void);
extern void store_shuffle(int which);
extern void store_maint(int which);
extern void stores_maint(int times);
extern void store_init(void);

/* util.c */
extern void text_to_ascii(char *buf, size_t len, cptr str);
extern void ascii_to_text(char *buf, size_t len, cptr str);
extern sint macro_find_exact(cptr pat);
extern errr macro_add(cptr pat, cptr act);
extern errr macro_init(void);
extern errr macro_free(void);
extern errr macro_trigger_free(void);
extern void flush(void);
extern int add_button_text(char *label, unsigned char keypress);
extern int kill_button_text(unsigned char keypress);
extern void kill_all_buttons_text(void);
extern void backup_buttons_text(void);
extern void restore_buttons_text(void);
extern int add_button(char *label, unsigned char keypress);
extern void backup_buttons(void);
extern void restore_buttons(void);
extern int kill_button(unsigned char keypress);
extern void kill_all_buttons(void);
extern char anykey(void);
extern char inkey(void);
extern event_type inkey_ex(void);
extern void bell(cptr reason);
extern void sound(int val);
extern s16b quark_add(cptr str);
extern char *quark_str(s16b i);
extern errr quarks_init(void);
extern errr quarks_free(void);
extern bool check_for_inscrip(const object_type *o_ptr, const char *inscrip);
extern s16b message_num(void);
extern cptr message_str(s16b age);
extern u16b message_type(s16b age);
extern byte message_color(s16b age);
extern errr message_color_define(u16b type, byte color);
extern void message_add(cptr str, u16b type);
extern void messages_easy(bool command);
extern errr messages_init(void);
extern void messages_free(void);
extern void move_cursor(int row, int col);
extern void msg_print(cptr msg);
extern void msg_format(cptr fmt, ...);
extern void message(u16b message_type, s16b extra, cptr message);
extern void message_format(u16b message_type, s16b extra, cptr fmt, ...);
extern void message_flush(void);
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern void put_str_center(cptr str, int row);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern void prt_center(cptr str, int row);
extern void c_roff(byte a, cptr str, byte l_margin, byte r_margin);
extern void roff(cptr str, byte l_margin, byte r_margin);
extern void text_out_to_file(byte attr, char *str);
extern void text_out_to_screen(byte a, char *str);
extern void text_out(char *str);
extern void text_out_c(byte a, char *str);
extern void clear_from(int row);
extern void dump_put_str(byte attr, const char *str, int col);
extern void dump_lnum(char *header, s32b num, int col, byte color);
extern void dump_num(char *header, int num, int col, byte color);
extern void dump_deci(char *header, int num, int deci, int col, byte color);
extern void dump_line_file(char_attr *this_line);
extern void dump_line_screen(char_attr *this_line);
extern void dump_line_mem(char_attr *this_line);
extern void dump_line(char_attr *this_line);
extern bool askfor_aux(char *buf, size_t len, bool keypress_h(char *, size_t, size_t *, size_t *, char, bool));
bool get_name_keypress(char *buf, size_t buflen, size_t *curs, size_t *len, 
		       char keypress, bool firsttime);

extern bool get_string(cptr prompt, char *buf, size_t len);
extern s16b get_quantity(cptr prompt, int max);
extern bool get_check(cptr prompt);
extern int get_check_other(cptr prompt, char other);
extern bool get_com(cptr prompt, char *command);
extern bool get_com_ex(cptr prompt, event_type *command);
extern void pause_line(int row);
extern void request_command(void);
extern int damroll(int num, int sides);
extern int maxroll(int num, int sides);
extern bool is_a_vowel(int ch);
extern int color_char_to_attr(char c);
extern void repeat_push(int what);
extern bool repeat_pull(int *what);
extern void repeat_clear(void);
extern void repeat_check(void);
extern int handle_item(void);
extern int make_metric(int wgt);
extern byte get_angle_to_grid[41][41];
extern cptr get_ext_color_name(byte ext_color);
extern int color_text_to_attr(cptr name);
extern cptr attr_to_text(byte a);
extern bool is_valid_pf(int y, int x);
extern bool findpath(int y, int x);

#ifdef SUPPORT_GAMMA
extern byte gamma_table[256];
extern void build_gamma_table(int gamma);
#endif /* SUPPORT_GAMMA */

/* xtra1.c */
extern void cnv_stat(int val, char *out_val);
extern s16b modify_stat_value(int value, int amount);
extern void update_statusline(void);
extern void apply_resist(int *player_resist, int item_resist);
extern void calc_bonuses(bool inspect);
extern void notice_stuff(void);
extern void update_stuff(void);
extern void redraw_stuff(void);
extern void window_stuff(void);
extern void handle_stuff(void);
extern int add_special_missile_skill(byte pclass, s16b weight, 
                                     object_type *o_ptr);
extern int add_special_melee_skill(byte pclass, s16b weight, 
                                   object_type *o_ptr);

/* xtra2.c */
extern bool set_blind(int v);
extern bool set_confused(int v);
extern bool set_poisoned(int v);
extern bool set_afraid(int v);
extern bool set_paralyzed(int v);
extern bool set_image(int v);
extern bool set_fast(int v);
extern bool set_slow(int v);
extern bool set_shield(int v);
extern bool set_blessed(int v);
extern bool set_hero(int v);
extern bool set_shero(int v);
extern bool set_protevil(int v);
extern bool set_extra_defences(int v);
extern bool set_tim_invis(int v);
extern bool set_tim_esp(int v);
extern bool set_superstealth(int v, bool message);
extern bool set_tim_infra(int v);
extern bool set_oppose_acid(int v);
extern bool set_oppose_elec(int v);
extern bool set_oppose_fire(int v);
extern bool set_oppose_cold(int v);
extern bool set_oppose_pois(int v);
extern bool set_stun(int v);
extern bool set_cut(int v);
extern bool set_food(int v);
extern bool set_recall(int v);
extern bool word_recall(int v);
extern void check_experience(void);
extern void gain_exp(s32b amount);
extern void lose_exp(s32b amount);
extern void monster_death(int m_idx);
extern bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note);
extern void look_mon_desc(int m_idx, char *buf, size_t max);
extern void ang_sort_aux(vptr u, vptr v, int p, int q);
extern void ang_sort(vptr u, vptr v, int n);
extern sint target_dir(char ch);
extern sint mouse_dir(event_type ke, bool locating);
extern bool target_able(int m_idx);
extern bool target_able_obj(int o_idx);
extern bool target_okay(void);
extern void target_set_monster(int m_idx);
extern void target_set_object(int o_idx);
extern void target_set_location(int y, int x);
extern bool target_set_interactive(int mode);
extern bool get_aim_dir(int *dp);
extern bool get_rep_dir(int *dp);
extern bool confuse_dir(int *dp);
extern bool tgt_pt (int *x, int *y);
extern void panel_center(void);
extern bool change_panel(int dy, int dx);
extern void verify_panel(void);

/* squelch.c */
extern byte squelch_level[TYPE_MAX];
int get_autoinscription_index(s16b k_idx);
const char *get_autoinscription(s16b kind_idx);
int apply_autoinscription(object_type *o_ptr);
int remove_autoinscription(s16b kind);
int add_autoinscription(s16b kind, cptr inscription);
void autoinscribe_ground(void);
void autoinscribe_pack(void);
bool squelch_tval(int tval);
extern bool squelch_item_ok(object_type *o_ptr);
bool squelch_hide_item(object_type *o_ptr);
extern void squelch_drop(void);
void squelch_items(void);
extern bool seen_tval(int tval);
void do_cmd_options_item(void *, cptr);

/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef MACH_O_CARBON
extern u32b _fcreator;
extern u32b _ftype;
extern void fsetfileinfo(cptr path, u32b fcreator, u32b ftype);
#endif

#ifdef MACINTOSH
/* main-mac.c */
/* extern void main(void); */
#endif

#ifdef WINDOWS
/* main-win.c */
/* extern int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, ...); */
#endif

#ifdef NDS
extern void debug_a(void);
#endif


#ifdef ALLOW_DEBUG
/* wizard2.c */
extern void do_cmd_debug(void);
#endif /* ALLOW_DEBUG */
bool jump_menu(int level, int *location);



/* main.c */
/* extern int main(int argc, char *argv[]); */

/* wizard1.c */
#ifdef ALLOW_SPOILERS
extern void do_cmd_spoilers(void);
#endif /* ALLOW_SPOILERS */

bool make_fake_artifact(object_type *o_ptr, int name1);

/* These should be defined in port-specific main- files - is this needed? */
extern int add_button_gui(char *label, unsigned char keypress);
extern int kill_button_gui(unsigned char keypress);
