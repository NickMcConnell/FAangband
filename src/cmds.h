#ifndef INCLUDED_CMDS_H
#define INCLUDED_CMDS_H

/** \file cmds.h 
    \brief Commands include file.
*/
 
#include "game-cmd.h"

/* 
 * Command handlers will take a pointer to the command structure
 * so that they can access any arguments supplied.
 */
typedef void (*cmd_handler_fn)(cmd_code code, cmd_arg args[]);


/* cmd0.c */
extern void cmd_init(void);
unsigned char cmd_lookup_key(cmd_code cmd, int mode);
cmd_code cmd_lookup(unsigned char key, int mode);

/* cmd-obj.c */
void pseudo_probe(void);
void do_cmd_uninscribe(cmd_code code, cmd_arg args[]);
void do_cmd_inscribe(cmd_code code, cmd_arg args[]);
void do_cmd_takeoff(cmd_code code, cmd_arg args[]);
void do_cmd_wield(cmd_code code, cmd_arg args[]);
void do_cmd_drop(cmd_code code, cmd_arg args[]);
void do_cmd_use(cmd_code code, cmd_arg args[]);
void do_cmd_refill(cmd_code code, cmd_arg args[]);
void do_cmd_study_spell(cmd_code code, cmd_arg args[]);
extern int get_channeling_boost(void);
void do_cmd_cast(cmd_code code, cmd_arg args[]);
void do_cmd_study_book(cmd_code code, cmd_arg args[]);

void textui_obj_examine(void);

/* cmd-misc.c */
extern void cmd_init(void);
extern int click_area(ui_event ke);
extern void do_cmd_show_obj(void);
extern void do_cmd_handle(void);
extern void handle_mousepress(int y, int x);
extern void do_cmd_wizard(void);
extern void do_cmd_try_debug(void);
extern bool do_cmd_try_borg(void);
extern void do_cmd_quit(cmd_code code, cmd_arg args[]);
extern void do_cmd_xxx_options(void);
extern void do_cmd_reshape(void);
extern void do_cmd_monlist(void);
extern void do_cmd_itemlist(void);
extern void do_cmd_unknown(void);

/* cmd1.c */
bool search(bool verbose);
extern byte py_pickup(int pickup, int y, int x);
extern void fall_off_cliff(void);
extern void move_player(int dir);

/* cmd2.c */
int count_feats(int *y, int *x, int flag, bool under);
int count_traps(int *y, int *x);
int count_chests(int *y, int *x, bool trapped);
int coords_to_dir(int y, int x);
extern void do_cmd_move_house(void);
extern void do_cmd_go_up(cmd_code code, cmd_arg args[]);
extern void do_cmd_go_down(cmd_code code, cmd_arg args[]);
extern void do_cmd_search(cmd_code code, cmd_arg args[]);
extern void do_cmd_toggle_search(cmd_code code, cmd_arg args[]);
extern void do_cmd_open(cmd_code code, cmd_arg args[]);
extern void do_cmd_close(cmd_code code, cmd_arg args[]);
extern void do_cmd_tunnel(cmd_code code, cmd_arg args[]);
extern bool do_cmd_open_aux(int y, int x);
extern bool do_cmd_disarm_aux(int y, int x);
extern void do_cmd_disarm(cmd_code code, cmd_arg args[]);
extern void do_cmd_bash(cmd_code code, cmd_arg args[]);
extern void do_cmd_alter(cmd_code code, cmd_arg args[]);
extern bool do_cmd_spike_test(int y, int x);
extern void do_cmd_spike(cmd_code code, cmd_arg args[]);
extern void do_cmd_walk(cmd_code code, cmd_arg args[]);
extern void do_cmd_jump(cmd_code code, cmd_arg args[]);
extern void do_cmd_run(cmd_code code, cmd_arg args[]);
extern void do_cmd_pathfind(cmd_code code, cmd_arg args[]);
extern void do_cmd_hold(cmd_code code, cmd_arg args[]);
extern void do_cmd_pickup(cmd_code code, cmd_arg args[]);
extern void do_cmd_rest(cmd_code code, cmd_arg args[]);
extern bool easy_open_door(int y, int x);

void textui_cmd_rest(void);
void textui_cmd_suicide(void);

/* cmd3.c */
extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
void wield_item(object_type *o_ptr, int item, int slot);
extern void do_cmd_destroy(cmd_code code, cmd_arg args[]);
void textui_cmd_destroy(void);
extern void do_cmd_target(void);
extern void do_cmd_target_closest(void);
extern void do_cmd_look(void);
extern void do_cmd_locate(void);
extern void do_cmd_query_symbol(void);
extern int cmp_monsters(const void *a, const void *b);

/* cmd4.c */
extern void do_cmd_redraw(void);
extern void redraw_window(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(void);
extern void ghost_challenge(void);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
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

/* specialty.c */
extern void do_cmd_specialty(void);

/* ui-spell.c -- just for now */
void textui_book_browse(const object_type *o_ptr);
void textui_spell_browse(void);
void textui_obj_study(void);
void textui_obj_cast(void);

/* ui-knowledge.c */
extern int big_pad(int col, int row, byte a, wchar_t c);
extern void textui_browse_object_knowledge(const char *name, int row);
extern void textui_knowledge_init(void);
extern void textui_browse_knowledge(void);

/* attack.c */
extern void do_cmd_fire(cmd_code code, cmd_arg args[]);
extern void textui_cmd_fire_at_nearest(void);
extern void do_cmd_throw(cmd_code code, cmd_arg args[]);
extern void textui_cmd_throw(void);

extern void do_cmd_store(cmd_code code, cmd_arg args[]);
extern void do_cmd_suicide(cmd_code code, cmd_arg args[]);
extern void do_cmd_save_game(cmd_code code, cmd_arg args[]);

/* Types of item use */
typedef enum
{
	USE_TIMEOUT,
	USE_CHARGE,
	USE_SINGLE
} use_type;

#endif

