/* list-options.h - options
 *
 * Currently, if there are more than 16 of any option type, the later ones
 * will be ignored
 * Cheat options need to be followed by corresponding score options
 */

/* name                   description
type     normal */
OP(none,                  "",
SPECIAL, FALSE)
OP(rogue_like_commands,   "Rogue-like commands",
INTERFACE, FALSE)
OP(center_player,         "Keep the player centered (slow)",
INTERFACE, FALSE)
OP(show_lists,            "Automatically show lists for commands",
INTERFACE, TRUE)
OP(show_menus,            "Enter key brings up command menu",
INTERFACE, TRUE)
OP(mouse_movement,        "Allow mouse clicks to move the player",
INTERFACE, FALSE)
OP(show_flavors,          "Show flavors in object descriptions",
INTERFACE, TRUE)
OP(show_detect,           "Show detection region",
INTERFACE, TRUE)
OP(view_yellow_light,     "Use special colors for torch light",
INTERFACE, FALSE)
OP(animate_flicker,       "Animate multi-colored monsters and items",
INTERFACE, FALSE)
OP(solid_walls,           "Show walls as solid blocks",
INTERFACE, FALSE)
OP(hybrid_walls,          "Show walls with shaded background",
INTERFACE, FALSE)
OP(ring_bell,             "Audible bell (on errors, etc)",
INTERFACE, TRUE)
OP(use_sound,             "Play sounds in game",
INTERFACE, FALSE)
OP(hp_changes_colour,     "Player colour indicates low hit points",
INTERFACE, TRUE)
OP(auto_more,             "Automatically clear '-more-' prompts",
INTERFACE, FALSE)
OP(auto_scum,             "Auto-scum for good levels",
GAMEPLAY, FALSE)
OP(night_mare,            "Generate more pits and vaults",
GAMEPLAY, FALSE)
OP(use_old_target,        "Use old target by default",
GAMEPLAY, FALSE)
OP(pickup_always,         "Pick things up by default",
GAMEPLAY, TRUE)
OP(pickup_inven,          "Always pickup items matching inventory",
GAMEPLAY, TRUE)
OP(easy_open,             "Open/close/disarm without direction",
GAMEPLAY, TRUE)
OP(easy_alter,            "Open/close/disarm on movement",
GAMEPLAY, FALSE)
OP(hide_squelchable,      "Hide items set as squelchable",
GAMEPLAY, TRUE)
OP(run_ignore_stairs,     "When running, ignore stairs",
GAMEPLAY, TRUE)
OP(run_ignore_doors,      "When running, ignore doors",
GAMEPLAY, TRUE)
OP(disturb_near,          "Disturb whenever viewable monster moves",
GAMEPLAY, TRUE)
OP(disturb_detect,        "Disturb when leaving trap detect area",
GAMEPLAY, TRUE)
OP(notify_recharge,       "Notify on object recharge",
GAMEPLAY, FALSE)
OP(show_target,           "Highlight target with cursor",
GAMEPLAY, TRUE)
OP(cheat_peek,            "Cheat: Peek into object creation",
CHEAT, FALSE)
OP(score_peek,            "Score: Peek into object creation",
SCORE, FALSE )
OP(cheat_hear,            "Cheat: Peek into monster creation",
CHEAT, FALSE )
OP(score_hear,            "Score: Peek into monster creation",
SCORE, FALSE )
OP(cheat_room,            "Cheat: Peek into dungeon creation",
CHEAT, FALSE )
OP(score_room,            "Score: Peek into dungeon creation",
SCORE, FALSE )
OP(cheat_xtra,            "Cheat: Peek into something else",
CHEAT, FALSE )
OP(score_xtra,            "Score: Peek into something else",
SCORE, FALSE )
OP(cheat_know,            "Cheat: Know complete monster info",
CHEAT, FALSE )
OP(score_know,            "Score: Know complete monster info",
SCORE, FALSE )
OP(cheat_live,            "Cheat: Allow player to avoid death",
CHEAT, FALSE )
OP(score_live,            "Score: Allow player to avoid death",
SCORE, FALSE )
