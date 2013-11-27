/** \file config.h 
    \brief Compilation options.
 
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */


/*
 * Look through the following lines, and where a comment includes the
 * tag "OPTION:", examine the associated "#define" statements, and decide
 * whether you wish to keep, comment, or uncomment them.  You should not
 * have to modify any lines not indicated by "OPTION".
 *
 * Note: Also examine the "system" configuration file "h-config.h"
 * and the variable initialization file "variable.c".  If you change
 * anything in "variable.c", you only need to recompile that file.
 *
 * And finally, remember that the "Makefile" will specify some rather
 * important compile time options, like what visual module to use.
 */


/*
 * OPTION: See the Makefile(s), where several options may be declared.
 *
 * Some popular options include "USE_GCU" (allow use with Unix "curses"),
 * "USE_X11" (allow basic use with Unix X11), "USE_XAW" (allow use with
 * Unix X11 plus the Athena Widget set), and "USE_CAP" (allow use with
 * the "termcap" library, or with hard-coded vt100 terminals).
 *
 * The old "USE_NCU" option has been replaced with "USE_GCU".
 *
 * Several other such options are available for non-unix machines,
 * such as "MACINTOSH", "WINDOWS", "USE_IBM", "USE_EMX".
 *
 * You may also need to specify the "system", using defines such as
 * "SOLARIS" (for Solaris), etc, see "h-config.h" for more info.
 */


/*
 * OPTION: Include "ncurses.h" instead of "curses.h" in "main-gcu.c"
 */
/* #define USE_NCURSES */



/*
 * OPTION: Hack -- Compile in support for "Borg mode"
 */
/* #define ALLOW_BORG */

/*
 * OPTION: Hack -- Compile in support for "Debug Commands"
 */
#define ALLOW_DEBUG

/*
 * OPTION: Hack -- Compile in support for "Spoiler Generation"
 */
#define ALLOW_SPOILERS


/*
 * OPTION: Allow "do_cmd_colors" at run-time
 */
#define ALLOW_COLORS

/*
 * OPTION: Allow "do_cmd_visuals" at run-time
 */
#define ALLOW_VISUALS

/*
 * Allow "Wizards" to yield "high scores"
 */
/* #define SCORE_WIZARDS */

/*
 * Allow "Borgs" to yield "high scores"
 */
/* #define SCORE_BORGS */

/*
 * Allow "Cheaters" to yield "high scores"
 */
/* #define SCORE_CHEATERS */

/*
 * OPTION: Allow the use of "sound" in various places.
 */
#define USE_SOUND

/*
 * OPTION: Allow the use of "graphics" in various places
 */
#define USE_GRAPHICS /* TNB */


/*** Some really important things you ought to change ***/

/*
 * Defines the default paths to the Angband directories, for ports that use
 * the main.c file.
 *
 * "config path" is for per-installation configurable data, like the game's
 * edit files and system-wide preferences.
 *
 * "lib path" is for static data, like sounds, graphics and fonts.
 *
 * "data path" is for variable data, like save files and scores. On single-
 * user systems, this also includes user preferences and dumps (on multi-
 * user systems these go under the user's home directory).
 *
 * The configure script overrides these values. Check the "--prefix=<dir>"
 * option of the configure script.
 *
 * These values will be over-ridden by the "ANGBAND_PATH" environment
 * variable, if that variable is defined and accessible.  The final
 * "slash" is required if the value supplied is in fact a directory.
 *
 * Using the value "./lib/" below tells Angband that, by default,
 * the user will run "angband" from the same directory that contains
 * the "lib" directory.  This is a reasonable (but imperfect) default.
 *
 * If at all possible, you should change this value to refer to the
 * actual location of the folders, for example, "/etc/angband/"
 * or "/usr/share/angband/", or "/var/games/angband/". In fact, if at all
 * possible you should use a packaging system which does this for you.
 *
 * N.B. The data path is only used if USE_PRIVATE_PATHS is not defined.
 * The other two are always used. 
 */
#ifndef DEFAULT_CONFIG_PATH
# define DEFAULT_CONFIG_PATH "." PATH_SEP "lib" PATH_SEP
#endif 

#ifndef DEFAULT_LIB_PATH
# define DEFAULT_LIB_PATH "." PATH_SEP "lib" PATH_SEP
#endif 

#ifndef DEFAULT_DATA_PATH
# define DEFAULT_DATA_PATH "." PATH_SEP "lib" PATH_SEP
#endif 


/*
 * OPTION: Create and use a hidden directory in the users home directory
 * for storing pref-files and character-dumps.
 */
#if defined(UNIX) && !defined(MACH_O_CARBON) && !defined(PRIVATE_USER_PATH)
#  define PRIVATE_USER_PATH "~/.angband"
#endif


/*
 * OPTION: Create and use hidden directories in the users home directory
 * for storing save files, data files, and high-scores
 */
#ifdef PRIVATE_USER_PATH
/* # define USE_PRIVATE_PATHS */
#endif /* PRIVATE_USER_PATH */


/*
 * OPTION: Person to bother if something goes wrong.
 */
#define MAINTAINER	"nckmccnnll@yahoo.com.au"

/*
 * OPTION: Default font (when using X11).
 */
#define DEFAULT_X11_FONT		"9x15"


/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_0		"10x20"
#define DEFAULT_X11_FONT_1		"9x15"
#define DEFAULT_X11_FONT_2		"9x15"
#define DEFAULT_X11_FONT_3		"5x8"
#define DEFAULT_X11_FONT_4		"5x8"
#define DEFAULT_X11_FONT_5		"5x8"
#define DEFAULT_X11_FONT_6		"5x8"
#define DEFAULT_X11_FONT_7		"5x8"



/*
 * OPTION: Gamma correct X11 colours.
 */

#define SUPPORT_GAMMA


