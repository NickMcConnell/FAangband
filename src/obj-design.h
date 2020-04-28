/**
 * \file obj-design.h 
 * \brief Random artifact and jewellery design
 *
 * Random artifacts, rings and amulets.  Selling and providing qualities.
 * Choosing object type and kind, determining the potential.
 * Depth and rarity for artifacts.
 * Selecting a theme, and the properties of all possible themes.  
 * Adding semi-random qualities until potential is all used up.
 * Cursing an artifact.
 * Removing contradictory flags.
 * Naming an artifact, using either of two methods.
 * Initializing random artifacts.
 *
 * Copyright (c) 1998 Leon Marrick
 *
 * Thanks to Greg Wooledge for his support and string-handling code 
 * and to W. Sheldon Simms for his Tolkienesque random name generator.
 *
 * Copyright (c) Si Griffin
 * Copyright (c) 2020 Nick McConnell
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#ifndef INCLUDED_OBJDESIGN_H
#define INCLUDED_OBJDESIGN_H

/**
 * Percentage chance for an object to be terrible.
 */
#define TERRIBLE_CHANCE		5

/**
 * Multiplier to get activation cost from effect power
 */
#define EFFECT_MULT         80

/**
 * Number of random artifacts
 */
#define ART_NUM_RANDOM      40

/**
 * Constants used by the artifact naming code.
 */
#define MIN_NAME_LEN 5
#define MAX_NAME_LEN 9

void initialize_random_artifacts(u32b randart_seed);

#endif /* !INCLUDED_OBJDESIGN_H */
