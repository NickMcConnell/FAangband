/** \file main.h 
    \brief Include for main

 *
 * Copyright (c) 2002 Robert Ruehlmann
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include "angband.h"

extern errr init_sound_sdl(int argc, char **argv);


extern errr init_gtk(int argc, char **argv);
extern errr init_x11(int argc, char **argv);
extern errr init_gcu(int argc, char **argv);
extern errr init_sdl(int argc, char **argv);


extern const char help_x11[];
extern const char help_gtk[];
extern const char help_gcu[];
extern const char help_sdl[];


struct module
{
	cptr name;
	cptr help;
	errr (*init)(int argc, char **argv);
};

#endif /* INCLUDED_MAIN_H */
