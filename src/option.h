#ifndef INCLUDED_OPTIONS_H
#define INCLUDED_OPTIONS_H


const char *option_name(int opt);
const char *option_desc(int opt);

bool option_set(const char *opt, bool on);
void option_set_defaults(void);


/*** Option display definitions ***/

/*
 * Information for "do_cmd_options()".
 */
#define OPT_PAGE_MAX				6
#define OPT_PAGE_PER				20

/* The option data structures */
extern const int option_page[OPT_PAGE_MAX][OPT_PAGE_PER];



/*** Option definitions ***/

/*
 * Option indexes (offsets)
 *
 * These values are hard-coded by savefiles (and various pieces of code).  Ick.
 */
#define OPT_BIRTH					128
#define OPT_CHEAT					160
#define OPT_ADULT					192
#define OPT_SCORE					224
#define OPT_NONE					255
#define OPT_MAX						256


/*
 * Indexes
 */
#define OPT_rogue_like_commands		0
#define OPT_quick_messages		1
#define OPT_floor_query_flag		2
#define OPT_carry_query_flag		3
#define OPT_use_old_target		4
#define OPT_always_pickup		5
#define OPT_always_repeat		6
#define OPT_squelch_worthless		7
#define OPT_stack_force_notes		8
#define OPT_stack_force_costs		9
#define OPT_show_labels			10
#define OPT_show_weights		11
#define OPT_show_choices		12
#define OPT_show_details		13
#define OPT_use_metric			14
#define OPT_show_flavors		15

#define OPT_run_ignore_stairs		16
#define OPT_run_ignore_doors		17
#define OPT_run_cut_corners		18
#define OPT_run_use_corners		19
#define OPT_disturb_move		20
#define OPT_disturb_near		21
#define OPT_disturb_panel		22
#define OPT_disturb_state		23
#define OPT_disturb_minor		24
#define OPT_disturb_other		25
#define OPT_alert_hitpoint		26
#define OPT_alert_failure		27
#define OPT_verify_destroy		28
#define OPT_verify_special		29
#define OPT_ring_bell			30
#define OPT_verify_destroy_junk		31
#define OPT_pickup_inven		32
#define OPT_auto_scum			33
#define OPT_easy_open			34
#define OPT_easy_disarm 		35
#define OPT_expand_look			36
#define OPT_expand_list			37
#define OPT_view_perma_grids	 	38
#define OPT_view_torch_grids		39
#define OPT_auto_more			40
#define OPT_dungeon_stair		41
#define OPT_strong_squelch              42
#define OPT_bottom_status               43
#define OPT_mouse_buttons               44
#define OPT_show_menus                  45
#define OPT_xchars_to_file              46
#define OPT_smart_cheat			47

#define OPT_view_reduce_lite		48
#define OPT_hidden_player		49
#define OPT_avoid_abort			50
#define OPT_avoid_other			51
#define OPT_flush_failure		52
#define OPT_flush_disturb		53
#define OPT_center_player		54
#define OPT_fresh_before		55
#define OPT_fresh_after			56

#define OPT_center_running		57

#define OPT_compress_savefile		58
#define OPT_hilite_player		59
#define OPT_view_yellow_lite		60
#define OPT_view_bright_lite		61
#define OPT_view_granite_lite		62
#define OPT_view_special_lite		63
#define OPT_easy_more                   65
#define OPT_animate_flicker             66
#define OPT_show_piles                  67
#define OPT_hp_changes_colour           68
#define OPT_show_detect                 69 
#define OPT_disturb_trap_detect         70 
#define OPT_show_lists                  71
#define OPT_hide_squelchable		72
#define OPT_auto_squelch		73
#define OPT_use_sound   		74

#define OPT_birth_point_based           (OPT_BIRTH+0)
#define OPT_birth_auto_roller           (OPT_BIRTH+1)
#define OPT_birth_take_notes            (OPT_BIRTH+2)
#define OPT_birth_preserve              (OPT_BIRTH+3)
#define OPT_birth_no_sell               (OPT_BIRTH+4)
#define OPT_birth_ironman               (OPT_BIRTH+5)
#define OPT_birth_thrall                (OPT_BIRTH+6)
#define OPT_birth_small_device          (OPT_BIRTH+7)
#define OPT_birth_dungeon               (OPT_BIRTH+8)
#define OPT_birth_no_artifacts          (OPT_BIRTH+9)

#define OPT_cheat_peek                  (OPT_CHEAT+0)
#define OPT_cheat_hear                  (OPT_CHEAT+1)
#define OPT_cheat_room                  (OPT_CHEAT+2)
#define OPT_cheat_xtra                  (OPT_CHEAT+3)
#define OPT_cheat_know                  (OPT_CHEAT+4)
#define OPT_cheat_live                  (OPT_CHEAT+5)

#define OPT_adult_point_based           (OPT_ADULT+0)
#define OPT_adult_auto_roller           (OPT_ADULT+1)
#define OPT_adult_take_notes            (OPT_ADULT+2)
#define OPT_adult_preserve              (OPT_ADULT+3)
#define OPT_adult_no_sell               (OPT_ADULT+4)
#define OPT_adult_ironman               (OPT_ADULT+5)
#define OPT_adult_thrall                (OPT_ADULT+6)
#define OPT_adult_small_device          (OPT_ADULT+7)
#define OPT_adult_dungeon               (OPT_ADULT+8)
#define OPT_adult_no_artifacts          (OPT_ADULT+9)

#define OPT_score_peek                  (OPT_SCORE+0)
#define OPT_score_hear                  (OPT_SCORE+1)
#define OPT_score_room                  (OPT_SCORE+2)
#define OPT_score_xtra                  (OPT_SCORE+3)
#define OPT_score_know                  (OPT_SCORE+4)
#define OPT_score_live                  (OPT_SCORE+5)



#define OPT(opt_name)	op_ptr->opt[OPT_##opt_name]

#endif /* !INCLUDED_OPTIONS_H */
