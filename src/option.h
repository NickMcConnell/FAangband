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
#define OPT_PAGE_PER				16

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
#define OPT_use_sound   		0
#define OPT_rogue_like_commands		1
#define OPT_use_old_target		2
#define OPT_pickup_always		3
#define OPT_pickup_inven		4
#define OPT_pickup_detail		5
#define OPT_pickup_single		6
#define OPT_hide_squelchable		7
#define OPT_squelch_worthless		8
#define OPT_easy_open			9
#define OPT_easy_alter  		10
#define OPT_show_lists                  11
#define OPT_show_menus                  12
#define OPT_mouse_movement              13
#define OPT_mouse_buttons               14
#define OPT_xchars_to_file              15

#define OPT_hp_changes_colour           16
#define OPT_highlight_player		17
#define OPT_center_player		18
#define OPT_show_piles                  19
#define OPT_show_flavors		20
#define OPT_show_labels			21
#define OPT_show_weights		22
#define OPT_show_detect 		23
#define OPT_view_yellow_light		24
#define OPT_view_bright_light		25
#define OPT_view_granite_light		26
#define OPT_view_special_light		27
#define OPT_view_perma_grids	 	28
#define OPT_view_torch_grids		29
#define OPT_animate_flicker             30


#define OPT_run_ignore_stairs		32
#define OPT_run_ignore_doors		33
#define OPT_run_cut_corners		34
#define OPT_run_use_corners		35
#define OPT_disturb_move		36
#define OPT_disturb_near		37
#define OPT_disturb_panel		38
#define OPT_disturb_detect              39 
#define OPT_disturb_state		40
#define OPT_quick_messages		41
#define OPT_verify_destroy		42
#define OPT_ring_bell			43
#define OPT_auto_more			44
#define OPT_flush_failure		45
#define OPT_flush_disturb		46
#define OPT_notify_recharge		47

#define OPT_auto_scum   		48
#define OPT_hard_mode   		49
#define OPT_solid_walls   		50
#define OPT_hybrid_walls   		51


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
#define OPT_birth_no_stairs             (OPT_BIRTH+10)
#define OPT_birth_ai_cheat              (OPT_BIRTH+11)
#define OPT_birth_auto_scum             (OPT_BIRTH+12)
#define OPT_birth_compressed            (OPT_BIRTH+13)

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
#define OPT_adult_no_stairs             (OPT_ADULT+10)
#define OPT_adult_ai_cheat              (OPT_ADULT+11)
#define OPT_adult_auto_scum             (OPT_ADULT+12)
#define OPT_adult_compressed            (OPT_ADULT+13)

#define OPT_score_peek                  (OPT_SCORE+0)
#define OPT_score_hear                  (OPT_SCORE+1)
#define OPT_score_room                  (OPT_SCORE+2)
#define OPT_score_xtra                  (OPT_SCORE+3)
#define OPT_score_know                  (OPT_SCORE+4)
#define OPT_score_live                  (OPT_SCORE+5)



#define OPT(opt_name)	op_ptr->opt[OPT_##opt_name]

#endif /* !INCLUDED_OPTIONS_H */
