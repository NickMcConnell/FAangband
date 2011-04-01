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
extern byte adj_mag_fail[STAT_RANGE];
extern byte adj_mag_stat[STAT_RANGE];
extern byte adj_chr_gold[STAT_RANGE];
extern byte adj_dex_safe[STAT_RANGE];
extern byte adj_con_fix[STAT_RANGE];
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
extern byte get_angle_to_grid[41][41];
extern int stage_map[NUM_STAGES][9];
extern int dungeon_map[NUM_STAGES][9];
extern cptr locality_name[MAX_LOCALITIES];
extern cptr short_locality_name[MAX_LOCALITIES];
extern int towns[10];
extern int race_town_prob[10][14];
byte type_of_store[MAX_STORES];
extern const byte char_tables[256][CHAR_TABLE_SLOTS];
extern const xchar_type latin1_encode[];

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
extern bool arg_rebalance;
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
extern cptr inkey_next;
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
extern color_type color_table[MAX_COLORS];
extern const cptr angband_sound_name[MSG_MAX];
extern int view_n;
extern u16b *view_g;
extern int  vinfo_grids;
extern int  vinfo_slopes;
extern u32b vinfo_bits_3;
extern u32b vinfo_bits_2;
extern u32b vinfo_bits_1;
extern u32b vinfo_bits_0;
extern int temp_n;
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
extern cptr** name_sections;
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
extern bool (*item_tester_hook)(const object_type*);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);
extern ang_file *text_out_file;
extern void (*text_out_hook)(byte a, char *str);
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

/* birth.c */
extern void player_birth(bool quickstart_allowed);

/* charattr.c */
extern void x_fprintf(ang_file *f, int encoding, cptr fmt, ...);
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
extern void search(void);
extern void py_pickup_aux(int o_idx);
extern byte py_pickup(int pickup, int y, int x);
extern void hit_trap(int y, int x);
extern void fall_off_cliff(void);
extern void move_player(int dir, int do_pickup);
extern void run_step(int dir);

/* cmd2.c */
/* XXX should probably be moved to cave.c? */
bool is_open(int feat);
bool is_closed(int feat);
bool is_trap(int feat);
int count_feats(int *y, int *x, bool (*test)(int feat), bool under);
int count_chests(int *y, int *x, bool trapped);
int coords_to_dir(int y, int x);

/* dungeon.c */
extern void play_game(void);

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
extern void init_file_paths(const char *configpath, const char *libpath, const char *datapath);
extern void create_user_dirs(void);
extern bool init_angband(void);
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
extern void describe_monster(int r_idx, bool spoilers);
extern void roff_top(int r_idx);
extern void screen_roff(int r_idx);
extern void display_roff(int r_idx);
extern bool prepare_ghost(int r_idx, monster_type *m_ptr, bool from_savefile);

/* monster2.c */
extern void monster_death(int m_idx);
extern bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note);
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

/* randart.c */
extern void initialize_random_artifacts(void);

/* randname.c */
extern size_t randname_make(randname_type name_type, size_t min, size_t max, char *word_buf, size_t buflen, const char ***wordlist);

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
extern void shapechange(s16b shape);
extern bool choose_ele_resist(void);
extern void create_athelas(void);
extern void dimen_door(void);
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

/* store.c */
extern void store_shuffle(int which);
extern void store_maint(int which);
extern void stores_maint(int times);
extern void store_init(void);

/* util.c */
extern void text_to_ascii(char *buf, size_t len, cptr str);
extern void ascii_to_text(char *buf, size_t len, cptr str);
extern char *find_roman_suffix_start(cptr buf);
extern int roman_to_int(const char *roman);
extern int int_to_roman(int n, char *roman, size_t bufsize);
extern void flush(void);
extern char anykey(void);
extern char inkey(void);
extern ui_event_data inkey_ex(void);
extern void bell(cptr reason);
extern void sound(int val);
extern void msg_print(cptr msg);
extern void msg_format(cptr fmt, ...);
extern void message(u16b message_type, s16b extra, cptr message);
extern void message_format(u16b message_type, s16b extra, cptr fmt, ...);
extern void message_flush(void);
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern void text_out_to_file(byte attr, cptr str);
extern void text_out_to_screen(byte a, cptr str);
extern void text_out(const char *fmt, ...);
extern void text_out_c(byte a, const char *fmt, ...);
extern void text_out_e(const char *fmt, ...);
extern void clear_from(int row);
extern bool askfor_aux_keypress(char *buf, size_t buflen, size_t *curs, size_t *len, char keypress, bool firsttime);
extern bool askfor_aux(char *buf, size_t len, bool keypress_h(char *, size_t, size_t *, size_t *, char, bool));
extern bool get_string(cptr prompt, char *buf, size_t len);
extern s16b get_quantity(cptr prompt, int max);
extern char get_char(cptr prompt, const char *options, size_t len, char fallback);
extern bool get_check(cptr prompt);
extern bool get_com(cptr prompt, char *command);
extern bool get_com_ex(cptr prompt, ui_event_data *command);
extern void pause_line(int row);
extern bool is_a_vowel(int ch);
extern int color_char_to_attr(char c);
extern int color_text_to_attr(cptr name);
extern cptr attr_to_text(byte a);
extern bool is_valid_pf(int y, int x);
extern bool findpath(int y, int x);

#ifdef SUPPORT_GAMMA
extern byte gamma_table[256];
extern void build_gamma_table(int gamma);
#endif /* SUPPORT_GAMMA */

/* x-char.c */
extern void xchar_trans_hook(char *s, int encoding);
extern void xstr_trans(char *str, int encoding);
extern void escape_latin1(char *dest, size_t max, cptr src);
extern const char seven_bit_translation[128];
extern char xchar_trans(byte c);

/* xtra2.c */
extern void check_experience(void);
extern void gain_exp(s32b amount);
extern void lose_exp(s32b amount);
int motion_dir(int y1, int x1, int y2, int x2);
extern int target_dir(char ch);
extern bool get_rep_dir(int *dp);
extern bool confuse_dir(int *dp);
extern void center_panel(void);
extern bool change_panel(int dy, int dx);
extern void verify_panel(void);

/* xtra3.c */
extern void cnv_stat(int val, char *out_val);
byte player_hp_attr(void);
byte player_sp_attr(void);
byte monster_health_attr(void);
void toggle_inven_equip(void);
void subwindows_set_flags(u32b *new_flags, size_t n_subwindows);
char* random_hint(void);

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



/* wizard1.c */
#ifdef ALLOW_SPOILERS
extern void do_cmd_spoilers(void);
#endif /* ALLOW_SPOILERS */

bool make_fake_artifact(object_type *o_ptr, int name1);

/* These should be defined in port-specific main- files - is this needed? */
extern int add_button_gui(char *label, unsigned char keypress);
extern int kill_button_gui(unsigned char keypress);
