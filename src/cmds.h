/** \file cmds.h 
    \brief Commands include file.
*/
 
#ifndef __angband_cmds_h__
#define __angband_cmds_h__

void textui_obj_inscribe(object_type *o_ptr, int item);
void textui_obj_examine(object_type *o_ptr, int item);
void textui_obj_wield(object_type *o_ptr, int item);
void textui_cmd_rest(void);
void textui_cmd_suicide(void);
void textui_cmd_destroy(void);
extern void textui_cmd_fire_at_nearest(void);
extern void textui_cmd_throw(void);

/* cmd-misc.c */
extern void cmd_init(void);
extern int click_area(ui_event_data ke);
extern void do_cmd_show_obj(void);
extern void do_cmd_handle(void);
extern void handle_mousepress(int y, int x);
extern void do_cmd_wizard(void);
extern void do_cmd_try_debug(void);
extern bool do_cmd_try_borg(void);
extern void do_cmd_quit(void);
extern void do_cmd_xxx_options(void);
extern void do_cmd_reshape(void);
extern void do_cmd_monlist(void);
extern void do_cmd_itemlist(void);
extern void do_cmd_unknown(void);

/* cmd2.c */
extern void do_cmd_move_house(void);
extern void do_cmd_go_up(void);
extern void do_cmd_go_down(void);
extern void do_cmd_search(void);
extern void do_cmd_toggle_search(void);
extern int count_chests(int *y, int *x, bool trapped);
extern void do_cmd_open(void);
extern void do_cmd_close(void);
extern void do_cmd_tunnel(void);
extern void do_cmd_disarm(void);
extern void do_cmd_bash(void);
extern void do_cmd_alter(void);
extern bool do_cmd_spike_test(int y, int x);
extern void do_cmd_spike(void);
extern void do_cmd_walk(void);
extern void do_cmd_jump(void);
extern void do_cmd_pathfind(int y, int x);
extern void do_cmd_run(void);
extern void do_cmd_hold(void);
extern void do_cmd_pickup(void);
extern void do_cmd_rest(void);
extern bool easy_open_door(int y, int x);

/* cmd3.c */
extern void do_cmd_floor(void);
extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
extern void do_cmd_wield(void);
extern void do_cmd_takeoff(void);
extern void do_cmd_drop(void);
extern void do_cmd_destroy(void);
extern void do_cmd_observe(void);
extern void do_cmd_uninscribe(void);
extern void do_cmd_inscribe(void);
extern void do_cmd_refill(void);
extern void do_cmd_target(void);
extern void do_cmd_target_closest(void);
extern void do_cmd_look(void);
extern void do_cmd_locate(void);
extern void do_cmd_query_symbol(void);
extern bool ang_sort_comp_hook(void *u, void *v, int a, int b);
extern void ang_sort_swap_hook(void *u, void *v, int a, int b);
extern void py_steal(int y, int x);
extern bool py_set_trap(int y, int x);
extern bool py_modify_trap(int y, int x);

/* cmd4.c */
extern void do_cmd_redraw(void);
extern void resize_map(void);
extern void redraw_window(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(void);
extern void ghost_challenge(void);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void make_note(char *note, int what_stage, byte note_type, s16b lev);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen(void);
extern void do_cmd_knowledge(void);
extern void init_cmd4_c(void);
extern void do_cmd_time(void);
extern void do_cmd_knowledge_objects(void *obj, const char *name);

/* cmd5.c */
extern void shapechange(s16b shape);
extern void create_athelas(void);
extern void dimen_door(void);
extern int get_channeling_boost(void);
extern void do_cmd_browse(void);
extern void do_cmd_study(void);
extern void do_cmd_cast_or_pray(void);
extern bool check_ability(int ability);
extern bool check_ability_specialty(int ability);
extern bool check_specialty_gain(int specialty);
extern void do_cmd_specialty(void);

/* cmd6.c */
extern void do_cmd_eat_food(void);
extern void do_cmd_quaff_potion(void);
extern void do_cmd_read_scroll(void);
extern void do_cmd_use_staff(void);
extern void do_cmd_aim_wand(void);
extern void do_cmd_zap_rod(void);
extern void do_cmd_activate(void);
extern void do_cmd_bear_shape(void);
extern void do_cmd_unchange(void);

/* ui-spell.c -- just for now */
void textui_book_browse(const object_type *o_ptr);
void textui_spell_browse(void);
void textui_obj_study(void);
void textui_obj_cast(void);

/* ui-knowledge.c */
extern void big_pad(int col, int row, byte a, byte c);
extern void textui_browse_object_knowledge(void *obj, const char *name);
extern void textui_knowledge_init(void);
extern void textui_browse_knowledge(void);

/* Types of item use */
typedef enum
{
	USE_TIMEOUT,
	USE_CHARGE,
	USE_SINGLE
} use_type;

#endif

