/** \file play-calcs.c 
    \brief Display of character data

 * Purpose: Player status calculation, signalling ui events based on status
 *          changes.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick & Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
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

#include "angband.h"
#include "cave.h"
#include "game-event.h"
#include "monster.h"
#include "player.h"
#include "spells.h"
#include "squelch.h"
#include "trap.h"


/**
 * Stat Table (INT/WIS) -- Number of half-spells per level
 */
byte adj_mag_study[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	1 /* 7 */ ,
	1 /* 8 */ ,
	1 /* 9 */ ,
	1 /* 10 */ ,
	1 /* 11 */ ,
	2 /* 12 */ ,
	2 /* 13 */ ,
	2 /* 14 */ ,
	2 /* 15 */ ,
	2 /* 16 */ ,
	2 /* 17 */ ,
	2 /* 18/00-18/09 */ ,
	2 /* 18/10-18/19 */ ,
	2 /* 18/20-18/29 */ ,
	2 /* 18/30-18/39 */ ,
	2 /* 18/40-18/49 */ ,
	3 /* 18/50-18/59 */ ,
	3 /* 18/60-18/69 */ ,
	3 /* 18/70-18/79 */ ,
	3 /* 18/80-18/89 */ ,
	3 /* 18/90-18/99 */ ,
	3 /* 18/100-18/109 */ ,
	3 /* 18/110-18/119 */ ,
	3 /* 18/120-18/129 */ ,
	3 /* 18/130-18/139 */ ,
	3 /* 18/140-18/149 */ ,
	3 /* 18/150-18/159 */ ,
	3 /* 18/160-18/169 */ ,
	3 /* 18/170-18/179 */ ,
	3 /* 18/180-18/189 */ ,
	3 /* 18/190-18/199 */ ,
	3 /* 18/200-18/209 */ ,
	3 /* 18/210-18/219 */ ,
	3 /* 18/220+ */
};


/**
 * Stat Table (INT/WIS) -- extra tenth-mana-points per level.
 */
byte adj_mag_mana[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	5 /* 7 */ ,
	6 /* 8 */ ,
	7 /* 9 */ ,
	8 /* 10 */ ,
	9 /* 11 */ ,
	9 /* 12 */ ,
	10 /* 13 */ ,
	10 /* 14 */ ,
	10 /* 15 */ ,
	11 /* 16 */ ,
	11 /* 17 */ ,
	12 /* 18/00-18/09 */ ,
	13 /* 18/10-18/19 */ ,
	14 /* 18/20-18/29 */ ,
	15 /* 18/30-18/39 */ ,
	17 /* 18/40-18/49 */ ,
	20 /* 18/50-18/59 */ ,
	22 /* 18/60-18/69 */ ,
	25 /* 18/70-18/79 */ ,
	30 /* 18/80-18/89 */ ,
	35 /* 18/90-18/99 */ ,
	40 /* 18/100-18/109 */ ,
	45 /* 18/110-18/119 */ ,
	50 /* 18/120-18/129 */ ,
	55 /* 18/130-18/139 */ ,
	60 /* 18/140-18/149 */ ,
	65 /* 18/150-18/159 */ ,
	70 /* 18/160-18/169 */ ,
	74 /* 18/170-18/179 */ ,
	77 /* 18/180-18/189 */ ,
	79 /* 18/190-18/199 */ ,
	81 /* 18/200-18/209 */ ,
	83 /* 18/210-18/219 */ ,
	85 /* 18/220+ */
};


/**
 * Stat Table (INT) -- Magic devices
 */
byte adj_int_dev[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	0 /* 7 */ ,
	1 /* 8 */ ,
	1 /* 9 */ ,
	1 /* 10 */ ,
	1 /* 11 */ ,
	1 /* 12 */ ,
	1 /* 13 */ ,
	1 /* 14 */ ,
	2 /* 15 */ ,
	2 /* 16 */ ,
	2 /* 17 */ ,
	3 /* 18/00-18/09 */ ,
	3 /* 18/10-18/19 */ ,
	4 /* 18/20-18/29 */ ,
	4 /* 18/30-18/39 */ ,
	5 /* 18/40-18/49 */ ,
	5 /* 18/50-18/59 */ ,
	6 /* 18/60-18/69 */ ,
	6 /* 18/70-18/79 */ ,
	7 /* 18/80-18/89 */ ,
	7 /* 18/90-18/99 */ ,
	8 /* 18/100-18/109 */ ,
	9 /* 18/110-18/119 */ ,
	10 /* 18/120-18/129 */ ,
	11 /* 18/130-18/139 */ ,
	12 /* 18/140-18/149 */ ,
	13 /* 18/150-18/159 */ ,
	14 /* 18/160-18/169 */ ,
	15 /* 18/170-18/179 */ ,
	16 /* 18/180-18/189 */ ,
	17 /* 18/190-18/199 */ ,
	18 /* 18/200-18/209 */ ,
	19 /* 18/210-18/219 */ ,
	20 /* 18/220+ */
};


/**
 * Stat Table (WIS) -- Saving throw
 */
byte adj_wis_sav[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	0 /* 7 */ ,
	1 /* 8 */ ,
	1 /* 9 */ ,
	1 /* 10 */ ,
	1 /* 11 */ ,
	1 /* 12 */ ,
	1 /* 13 */ ,
	1 /* 14 */ ,
	2 /* 15 */ ,
	2 /* 16 */ ,
	2 /* 17 */ ,
	3 /* 18/00-18/09 */ ,
	3 /* 18/10-18/19 */ ,
	3 /* 18/20-18/29 */ ,
	3 /* 18/30-18/39 */ ,
	3 /* 18/40-18/49 */ ,
	4 /* 18/50-18/59 */ ,
	4 /* 18/60-18/69 */ ,
	5 /* 18/70-18/79 */ ,
	5 /* 18/80-18/89 */ ,
	6 /* 18/90-18/99 */ ,
	7 /* 18/100-18/109 */ ,
	8 /* 18/110-18/119 */ ,
	9 /* 18/120-18/129 */ ,
	10 /* 18/130-18/139 */ ,
	11 /* 18/140-18/149 */ ,
	12 /* 18/150-18/159 */ ,
	13 /* 18/160-18/169 */ ,
	14 /* 18/170-18/179 */ ,
	15 /* 18/180-18/189 */ ,
	16 /* 18/190-18/199 */ ,
	17 /* 18/200-18/209 */ ,
	18 /* 18/210-18/219 */ ,
	19 /* 18/220+ */
};


/**
 * Stat Table (DEX) -- disarming (also getting out of pits)
 */
const byte adj_dex_dis[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	0 /* 7 */ ,
	0 /* 8 */ ,
	0 /* 9 */ ,
	0 /* 10 */ ,
	0 /* 11 */ ,
	0 /* 12 */ ,
	1 /* 13 */ ,
	1 /* 14 */ ,
	1 /* 15 */ ,
	2 /* 16 */ ,
	2 /* 17 */ ,
	4 /* 18/00-18/09 */ ,
	4 /* 18/10-18/19 */ ,
	4 /* 18/20-18/29 */ ,
	4 /* 18/30-18/39 */ ,
	5 /* 18/40-18/49 */ ,
	5 /* 18/50-18/59 */ ,
	5 /* 18/60-18/69 */ ,
	6 /* 18/70-18/79 */ ,
	6 /* 18/80-18/89 */ ,
	7 /* 18/90-18/99 */ ,
	8 /* 18/100-18/109 */ ,
	8 /* 18/110-18/119 */ ,
	8 /* 18/120-18/129 */ ,
	8 /* 18/130-18/139 */ ,
	8 /* 18/140-18/149 */ ,
	9 /* 18/150-18/159 */ ,
	9 /* 18/160-18/169 */ ,
	9 /* 18/170-18/179 */ ,
	9 /* 18/180-18/189 */ ,
	9 /* 18/190-18/199 */ ,
	10 /* 18/200-18/209 */ ,
	10 /* 18/210-18/219 */ ,
	10 /* 18/220+ */
};


/**
 * Stat Table (INT) -- disarming
 */
byte adj_int_dis[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	0 /* 7 */ ,
	1 /* 8 */ ,
	1 /* 9 */ ,
	1 /* 10 */ ,
	1 /* 11 */ ,
	1 /* 12 */ ,
	1 /* 13 */ ,
	1 /* 14 */ ,
	2 /* 15 */ ,
	2 /* 16 */ ,
	2 /* 17 */ ,
	3 /* 18/00-18/09 */ ,
	3 /* 18/10-18/19 */ ,
	3 /* 18/20-18/29 */ ,
	4 /* 18/30-18/39 */ ,
	4 /* 18/40-18/49 */ ,
	5 /* 18/50-18/59 */ ,
	6 /* 18/60-18/69 */ ,
	7 /* 18/70-18/79 */ ,
	8 /* 18/80-18/89 */ ,
	9 /* 18/90-18/99 */ ,
	10 /* 18/100-18/109 */ ,
	10 /* 18/110-18/119 */ ,
	11 /* 18/120-18/129 */ ,
	12 /* 18/130-18/139 */ ,
	13 /* 18/140-18/149 */ ,
	14 /* 18/150-18/159 */ ,
	15 /* 18/160-18/169 */ ,
	16 /* 18/170-18/179 */ ,
	17 /* 18/180-18/189 */ ,
	18 /* 18/190-18/199 */ ,
	19 /* 18/200-18/209 */ ,
	19 /* 18/210-18/219 */ ,
	19 /* 18/220+ */
};


/**
 * Stat Table (DEX) -- bonus to ac (plus 128)
 */
const byte adj_dex_ta[STAT_RANGE] = {
	128 + -4 /* 3 */ ,
	128 + -3 /* 4 */ ,
	128 + -2 /* 5 */ ,
	128 + -1 /* 6 */ ,
	128 + 0 /* 7 */ ,
	128 + 0 /* 8 */ ,
	128 + 0 /* 9 */ ,
	128 + 0 /* 10 */ ,
	128 + 0 /* 11 */ ,
	128 + 0 /* 12 */ ,
	128 + 0 /* 13 */ ,
	128 + 0 /* 14 */ ,
	128 + 1 /* 15 */ ,
	128 + 1 /* 16 */ ,
	128 + 1 /* 17 */ ,
	128 + 2 /* 18/00-18/09 */ ,
	128 + 2 /* 18/10-18/19 */ ,
	128 + 2 /* 18/20-18/29 */ ,
	128 + 2 /* 18/30-18/39 */ ,
	128 + 2 /* 18/40-18/49 */ ,
	128 + 3 /* 18/50-18/59 */ ,
	128 + 3 /* 18/60-18/69 */ ,
	128 + 3 /* 18/70-18/79 */ ,
	128 + 4 /* 18/80-18/89 */ ,
	128 + 5 /* 18/90-18/99 */ ,
	128 + 6 /* 18/100-18/109 */ ,
	128 + 7 /* 18/110-18/119 */ ,
	128 + 8 /* 18/120-18/129 */ ,
	128 + 9 /* 18/130-18/139 */ ,
	128 + 9 /* 18/140-18/149 */ ,
	128 + 10 /* 18/150-18/159 */ ,
	128 + 11 /* 18/160-18/169 */ ,
	128 + 12 /* 18/170-18/179 */ ,
	128 + 13 /* 18/180-18/189 */ ,
	128 + 14 /* 18/190-18/199 */ ,
	128 + 15 /* 18/200-18/209 */ ,
	128 + 15 /* 18/210-18/219 */ ,
	128 + 15 /* 18/220+ */
};


/**
 * Stat Table (STR) -- bonus to Deadliness (plus 128).  To compensate
 * for changes elsewhere, STR now has a larger effect. -LM-
 */
const byte adj_str_td[STAT_RANGE] = {
	128 + -2 /* 3 */ ,
	128 + -2 /* 4 */ ,
	128 + -1 /* 5 */ ,
	128 + -1 /* 6 */ ,
	128 + 0 /* 7 */ ,
	128 + 0 /* 8 */ ,
	128 + 0 /* 9 */ ,
	128 + 0 /* 10 */ ,
	128 + 0 /* 11 */ ,
	128 + 0 /* 12 */ ,
	128 + 0 /* 13 */ ,
	128 + 0 /* 14 */ ,
	128 + 0 /* 15 */ ,
	128 + 1 /* 16 */ ,
	128 + 2 /* 17 */ ,
	128 + 3 /* 18/00-18/09 */ ,
	128 + 4 /* 18/10-18/19 */ ,
	128 + 4 /* 18/20-18/29 */ ,
	128 + 5 /* 18/30-18/39 */ ,
	128 + 6 /* 18/40-18/49 */ ,
	128 + 7 /* 18/50-18/59 */ ,
	128 + 8 /* 18/60-18/69 */ ,
	128 + 9 /* 18/70-18/79 */ ,
	128 + 10 /* 18/80-18/89 */ ,
	128 + 11 /* 18/90-18/99 */ ,
	128 + 12 /* 18/100-18/109 */ ,
	128 + 13 /* 18/110-18/119 */ ,
	128 + 14 /* 18/120-18/129 */ ,
	128 + 15 /* 18/130-18/139 */ ,
	128 + 16 /* 18/140-18/149 */ ,
	128 + 17 /* 18/150-18/159 */ ,
	128 + 18 /* 18/160-18/169 */ ,
	128 + 19 /* 18/170-18/179 */ ,
	128 + 20 /* 18/180-18/189 */ ,
	128 + 21 /* 18/190-18/199 */ ,
	128 + 22 /* 18/200-18/209 */ ,
	128 + 23 /* 18/210-18/219 */ ,
	128 + 25 /* 18/220+ */
};


/**
 * Stat Table (DEX) -- bonus to Skill (plus 128.  To compensate for
 * changes elsewhere, DEX now has a larger effect. -LM-
 */
const byte adj_dex_th[STAT_RANGE] = {
	128 + -4 /* 3 */ ,
	128 + -3 /* 4 */ ,
	128 + -2 /* 5 */ ,
	128 + -1 /* 6 */ ,
	128 + -1 /* 7 */ ,
	128 + 0 /* 8 */ ,
	128 + 0 /* 9 */ ,
	128 + 0 /* 10 */ ,
	128 + 0 /* 11 */ ,
	128 + 0 /* 12 */ ,
	128 + 0 /* 13 */ ,
	128 + 0 /* 14 */ ,
	128 + 1 /* 15 */ ,
	128 + 2 /* 16 */ ,
	128 + 3 /* 17 */ ,
	128 + 3 /* 18/00-18/09 */ ,
	128 + 3 /* 18/10-18/19 */ ,
	128 + 4 /* 18/20-18/29 */ ,
	128 + 4 /* 18/30-18/39 */ ,
	128 + 4 /* 18/40-18/49 */ ,
	128 + 5 /* 18/50-18/59 */ ,
	128 + 5 /* 18/60-18/69 */ ,
	128 + 6 /* 18/70-18/79 */ ,
	128 + 6 /* 18/80-18/89 */ ,
	128 + 7 /* 18/90-18/99 */ ,
	128 + 8 /* 18/100-18/109 */ ,
	128 + 9 /* 18/110-18/119 */ ,
	128 + 10 /* 18/120-18/129 */ ,
	128 + 11 /* 18/130-18/139 */ ,
	128 + 12 /* 18/140-18/149 */ ,
	128 + 13 /* 18/150-18/159 */ ,
	128 + 14 /* 18/160-18/169 */ ,
	128 + 15 /* 18/170-18/179 */ ,
	128 + 16 /* 18/180-18/189 */ ,
	128 + 17 /* 18/190-18/199 */ ,
	128 + 18 /* 18/200-18/209 */ ,
	128 + 19 /* 18/210-18/219 */ ,
	128 + 20 /* 18/220+ */
};


/**
 * Stat Table (STR) -- weight limit in deca-pounds
 */
const byte adj_str_wgt[STAT_RANGE] = {
	6 /* 3 */ ,
	7 /* 4 */ ,
	8 /* 5 */ ,
	9 /* 6 */ ,
	10 /* 7 */ ,
	11 /* 8 */ ,
	12 /* 9 */ ,
	13 /* 10 */ ,
	14 /* 11 */ ,
	15 /* 12 */ ,
	15 /* 13 */ ,
	16 /* 14 */ ,
	17 /* 15 */ ,
	18 /* 16 */ ,
	19 /* 17 */ ,
	20 /* 18/00-18/09 */ ,
	22 /* 18/10-18/19 */ ,
	24 /* 18/20-18/29 */ ,
	26 /* 18/30-18/39 */ ,
	28 /* 18/40-18/49 */ ,
	30 /* 18/50-18/59 */ ,
	30 /* 18/60-18/69 */ ,
	30 /* 18/70-18/79 */ ,
	30 /* 18/80-18/89 */ ,
	30 /* 18/90-18/99 */ ,
	30 /* 18/100-18/109 */ ,
	30 /* 18/110-18/119 */ ,
	30 /* 18/120-18/129 */ ,
	30 /* 18/130-18/139 */ ,
	30 /* 18/140-18/149 */ ,
	30 /* 18/150-18/159 */ ,
	30 /* 18/160-18/169 */ ,
	30 /* 18/170-18/179 */ ,
	30 /* 18/180-18/189 */ ,
	30 /* 18/190-18/199 */ ,
	30 /* 18/200-18/209 */ ,
	30 /* 18/210-18/219 */ ,
	30 /* 18/220+ */
};


/**
 * Stat Table (STR) -- weapon weight limit in pounds
 */
const byte adj_str_hold[STAT_RANGE] = {
	4 /* 3 */ ,
	5 /* 4 */ ,
	6 /* 5 */ ,
	7 /* 6 */ ,
	8 /* 7 */ ,
	10 /* 8 */ ,
	12 /* 9 */ ,
	14 /* 10 */ ,
	16 /* 11 */ ,
	18 /* 12 */ ,
	20 /* 13 */ ,
	22 /* 14 */ ,
	24 /* 15 */ ,
	26 /* 16 */ ,
	28 /* 17 */ ,
	30 /* 18/00-18/09 */ ,
	30 /* 18/10-18/19 */ ,
	35 /* 18/20-18/29 */ ,
	40 /* 18/30-18/39 */ ,
	45 /* 18/40-18/49 */ ,
	50 /* 18/50-18/59 */ ,
	55 /* 18/60-18/69 */ ,
	60 /* 18/70-18/79 */ ,
	65 /* 18/80-18/89 */ ,
	70 /* 18/90-18/99 */ ,
	80 /* 18/100-18/109 */ ,
	80 /* 18/110-18/119 */ ,
	80 /* 18/120-18/129 */ ,
	80 /* 18/130-18/139 */ ,
	80 /* 18/140-18/149 */ ,
	90 /* 18/150-18/159 */ ,
	90 /* 18/160-18/169 */ ,
	90 /* 18/170-18/179 */ ,
	90 /* 18/180-18/189 */ ,
	90 /* 18/190-18/199 */ ,
	100 /* 18/200-18/209 */ ,
	100 /* 18/210-18/219 */ ,
	100	/* 18/220+ */
};


/**
 * Stat Table (STR) -- digging value
 */
byte adj_str_dig[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	1 /* 5 */ ,
	2 /* 6 */ ,
	3 /* 7 */ ,
	4 /* 8 */ ,
	4 /* 9 */ ,
	5 /* 10 */ ,
	5 /* 11 */ ,
	6 /* 12 */ ,
	6 /* 13 */ ,
	7 /* 14 */ ,
	7 /* 15 */ ,
	8 /* 16 */ ,
	8 /* 17 */ ,
	9 /* 18/00-18/09 */ ,
	10 /* 18/10-18/19 */ ,
	12 /* 18/20-18/29 */ ,
	15 /* 18/30-18/39 */ ,
	20 /* 18/40-18/49 */ ,
	25 /* 18/50-18/59 */ ,
	30 /* 18/60-18/69 */ ,
	35 /* 18/70-18/79 */ ,
	40 /* 18/80-18/89 */ ,
	45 /* 18/90-18/99 */ ,
	50 /* 18/100-18/109 */ ,
	55 /* 18/110-18/119 */ ,
	60 /* 18/120-18/129 */ ,
	65 /* 18/130-18/139 */ ,
	70 /* 18/140-18/149 */ ,
	75 /* 18/150-18/159 */ ,
	80 /* 18/160-18/169 */ ,
	85 /* 18/170-18/179 */ ,
	90 /* 18/180-18/189 */ ,
	95 /* 18/190-18/199 */ ,
	100 /* 18/200-18/209 */ ,
	100 /* 18/210-18/219 */ ,
	100	/* 18/220+ */
};


/**
 * Stat Table (STR) -- help index into the "blow" table
 */
const byte adj_str_blow[STAT_RANGE] = {
	3 /* 3 */ ,
	4 /* 4 */ ,
	5 /* 5 */ ,
	6 /* 6 */ ,
	7 /* 7 */ ,
	8 /* 8 */ ,
	9 /* 9 */ ,
	10 /* 10 */ ,
	11 /* 11 */ ,
	12 /* 12 */ ,
	13 /* 13 */ ,
	14 /* 14 */ ,
	15 /* 15 */ ,
	16 /* 16 */ ,
	17 /* 17 */ ,
	20 /* 18/00-18/09 */ ,
	30 /* 18/10-18/19 */ ,
	40 /* 18/20-18/29 */ ,
	50 /* 18/30-18/39 */ ,
	60 /* 18/40-18/49 */ ,
	70 /* 18/50-18/59 */ ,
	80 /* 18/60-18/69 */ ,
	90 /* 18/70-18/79 */ ,
	100 /* 18/80-18/89 */ ,
	110 /* 18/90-18/99 */ ,
	120 /* 18/100-18/109 */ ,
	130 /* 18/110-18/119 */ ,
	140 /* 18/120-18/129 */ ,
	150 /* 18/130-18/139 */ ,
	160 /* 18/140-18/149 */ ,
	170 /* 18/150-18/159 */ ,
	180 /* 18/160-18/169 */ ,
	190 /* 18/170-18/179 */ ,
	200 /* 18/180-18/189 */ ,
	210 /* 18/190-18/199 */ ,
	220 /* 18/200-18/209 */ ,
	230 /* 18/210-18/219 */ ,
	240	/* 18/220+ */
};

/**
 * Stat Table (DEX) -- index into the "blow" table
 */
byte adj_dex_blow[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	0 /* 7 */ ,
	0 /* 8 */ ,
	0 /* 9 */ ,
	1 /* 10 */ ,
	1 /* 11 */ ,
	1 /* 12 */ ,
	1 /* 13 */ ,
	1 /* 14 */ ,
	1 /* 15 */ ,
	1 /* 16 */ ,
	1 /* 17 */ ,
	1 /* 18/00-18/09 */ ,
	2 /* 18/10-18/19 */ ,
	2 /* 18/20-18/29 */ ,
	2 /* 18/30-18/39 */ ,
	3 /* 18/40-18/49 */ ,
	3 /* 18/50-18/59 */ ,
	4 /* 18/60-18/69 */ ,
	4 /* 18/70-18/79 */ ,
	5 /* 18/80-18/89 */ ,
	5 /* 18/90-18/99 */ ,
	6 /* 18/100-18/109 */ ,
	7 /* 18/110-18/119 */ ,
	8 /* 18/120-18/129 */ ,
	9 /* 18/130-18/139 */ ,
	9 /* 18/140-18/149 */ ,
	10 /* 18/150-18/159 */ ,
	10 /* 18/160-18/169 */ ,
	11 /* 18/170-18/179 */ ,
	11 /* 18/180-18/189 */ ,
	11 /* 18/190-18/199 */ ,
	11 /* 18/200-18/209 */ ,
	11 /* 18/210-18/219 */ ,
	11 /* 18/220+ */
};


/**
 * Stat Table (DEX) -- Used for number of shots per round
 */
byte adj_dex_shots[STAT_RANGE] = {
	0 /* 3 */ ,
	0 /* 4 */ ,
	0 /* 5 */ ,
	0 /* 6 */ ,
	0 /* 7 */ ,
	0 /* 8 */ ,
	0 /* 9 */ ,
	0 /* 10 */ ,
	0 /* 11 */ ,
	0 /* 12 */ ,
	0 /* 13 */ ,
	0 /* 14 */ ,
	0 /* 15 */ ,
	0 /* 16 */ ,
	0 /* 17 */ ,
	1 /* 18/00-18/09 */ ,
	1 /* 18/10-18/19 */ ,
	2 /* 18/20-18/29 */ ,
	3 /* 18/30-18/39 */ ,
	4 /* 18/40-18/49 */ ,
	5 /* 18/50-18/59 */ ,
	6 /* 18/60-18/69 */ ,
	7 /* 18/70-18/79 */ ,
	8 /* 18/80-18/89 */ ,
	9 /* 18/90-18/99 */ ,
	10 /* 18/100-18/109 */ ,
	11 /* 18/110-18/119 */ ,
	12 /* 18/120-18/129 */ ,
	13 /* 18/130-18/139 */ ,
	14 /* 18/140-18/149 */ ,
	15 /* 18/150-18/159 */ ,
	16 /* 18/160-18/169 */ ,
	17 /* 18/170-18/179 */ ,
	18 /* 18/180-18/189 */ ,
	19 /* 18/190-18/199 */ ,
	20 /* 18/200-18/209 */ ,
	20 /* 18/210-18/219 */ ,
	20 /* 18/220+ */
};


/**
 * Stat Table (CON) -- extra half-hitpoints per level (plus 128).
 * Because monsters don't breath as powerfully now, I have reduced the
 * effect of this stat. -LM-
 */
byte adj_con_mhp[] = {
	128 + -5 /* 3 */ ,
	128 + -3 /* 4 */ ,
	128 + -2 /* 5 */ ,
	128 + -1 /* 6 */ ,
	128 + 0 /* 7 */ ,
	128 + 0 /* 8 */ ,
	128 + 0 /* 9 */ ,
	128 + 0 /* 10 */ ,
	128 + 0 /* 11 */ ,
	128 + 0 /* 12 */ ,
	128 + 0 /* 13 */ ,
	128 + 0 /* 14 */ ,
	128 + 1 /* 15 */ ,
	128 + 1 /* 16 */ ,
	128 + 2 /* 17 */ ,
	128 + 3 /* 18/00-18/09 */ ,
	128 + 4 /* 18/10-18/19 */ ,
	128 + 4 /* 18/20-18/29 */ ,
	128 + 4 /* 18/30-18/39 */ ,
	128 + 4 /* 18/40-18/49 */ ,
	128 + 5 /* 18/50-18/59 */ ,
	128 + 5 /* 18/60-18/69 */ ,
	128 + 6 /* 18/70-18/79 */ ,
	128 + 6 /* 18/80-18/89 */ ,
	128 + 7 /* 18/90-18/99 */ ,
	128 + 8 /* 18/100-18/109 */ ,
	128 + 9 /* 18/110-18/119 */ ,
	128 + 9 /* 18/120-18/129 */ ,
	128 + 10 /* 18/130-18/139 */ ,
	128 + 11 /* 18/140-18/149 */ ,
	128 + 12 /* 18/150-18/159 */ ,
	128 + 12 /* 18/160-18/169 */ ,
	128 + 13 /* 18/170-18/179 */ ,
	128 + 14 /* 18/180-18/189 */ ,
	128 + 15 /* 18/190-18/199 */ ,
	128 + 17 /* 18/200-18/209 */ ,
	128 + 18 /* 18/210-18/219 */ ,
	128 + 20 /* 18/220+ */
};


/**
 * Stat Table (DEX) evasion max bonus from DEX.
 */
byte adj_dex_evas[] = {
	25 /* 3 */ ,
	25 /* 4 */ ,
	25 /* 5 */ ,
	25 /* 6 */ ,
	25 /* 7 */ ,
	25 /* 8 */ ,
	25 /* 9 */ ,
	25 /* 10 */ ,
	26 /* 11 */ ,
	27 /* 12 */ ,
	28 /* 13 */ ,
	29 /* 14 */ ,
	30 /* 15 */ ,
	31 /* 16 */ ,
	32 /* 17 */ ,
	33 /* 18/00-18/09 */ ,
	33 /* 18/10-18/19 */ ,
	34 /* 18/20-18/29 */ ,
	34 /* 18/30-18/39 */ ,
	35 /* 18/40-18/49 */ ,
	35 /* 18/50-18/59 */ ,
	35 /* 18/60-18/69 */ ,
	36 /* 18/70-18/79 */ ,
	36 /* 18/80-18/89 */ ,
	36 /* 18/90-18/99 */ ,
	37 /* 18/100-18/109 */ ,
	37 /* 18/110-18/119 */ ,
	37 /* 18/120-18/129 */ ,
	38 /* 18/130-18/139 */ ,
	38 /* 18/140-18/149 */ ,
	38 /* 18/150-18/159 */ ,
	39 /* 18/160-18/169 */ ,
	39 /* 18/170-18/179 */ ,
	39 /* 18/180-18/189 */ ,
	40 /* 18/190-18/199 */ ,
	40 /* 18/200-18/209 */ ,
	40 /* 18/210-18/219 */ ,
	40 /* 18/220+ */
};


/**
 * This table is used to help calculate the number of blows the player 
 * can make in a single round of attacks (one player turn) with a 
 * weapon that is not too heavy to wield effectively.
 *
 * The player gets "blows_table[P][D]" blows/round, as shown below.
 *
 * To get "P", we look up the relevant "adj_str_blow[]" (see above),
 * multiply it by 6, and then divide it by the effective weapon 
 * weight (in deci-pounds), rounding down.
 *
 * To get "D", we look up the relevant "adj_dex_blow[]" (see above).
 *
 * (Some interesting calculations)
 * The character cannot get five blows with any weapon greater than 24
 * lb, and cannot get six with any weapon greater than 14.4 lb.
 */
byte blows_table[12][12] = {
	/*  <- Dexterity factor -> */
	/* 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11+ */

	{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3},	/*  0         */
	{2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3},	/*  1    ^    */
	{2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4},	/*  2    |    */
	{2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4},	/*  3         */
	{2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4},	/*  4  Ratio  */
	{2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},	/*  5  of STR */
	{2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5},	/*  6  over   */
	{2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5},	/*  7  weight */
	{2, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5},	/*  8         */
	{2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5},	/*  9    |    */
	{2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6},	/* 10    V    */
	{2, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6}	/* 11+        */
};

/**
 * Calculate number of spells player should have, and forget,
 * or remember, spells until that number is properly reflected.
 *
 * Note that this function induces various "status" messages,
 * which must be bypassed until the character is created.
 */
static void calc_spells(void)
{
	int i, j, k, levels;
	int num_allowed, num_known;

	magic_type *mt_ptr;

	s16b old_spells;

	/* Hack -- must be literate */
	if (!mp_ptr->spell_book)
		return;

	/* Hack -- wait for creation */
	if (!character_generated)
		return;

	/* Hack -- handle "xtra" mode */
	if (character_xtra)
		return;

	/* Save the new_spells value */
	old_spells = p_ptr->new_spells;

	/* Determine the number of spells allowed */
	levels = p_ptr->lev - mp_ptr->spell_first + 1;

	/* Hack -- no negative spells */
	if (levels < 0)
		levels = 0;


	/* Extract total allowed spells */
	num_allowed =
		(adj_mag_study[p_ptr->state.stat_ind[mp_ptr->spell_stat]] *
		 levels / 2);

	/* Boundary control. */
	if (num_allowed > mp_ptr->spell_number)
		num_allowed = mp_ptr->spell_number;


	/* Assume none known */
	num_known = 0;

	/* Count the number of spells we know */
	for (j = 0; j < mp_ptr->spell_number; j++) {
		/* Count known spells */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED) {
			num_known++;
		}
	}

	/* See how many spells we must forget or may learn */
	p_ptr->new_spells = num_allowed - num_known;



	/* Forget spells which are too hard */
	for (i = PY_MAX_SPELLS; i >= 0; i--) {
		/* Access the spell */
		j = p_ptr->spell_order[i];

		/* Skip non-spells */
		if (j >= 99)
			continue;

		/* Get the spell */
		mt_ptr = &mp_ptr->info[j];

		/* Skip spells we are allowed to know */
		if (mt_ptr->slevel <= p_ptr->lev)
			continue;

		/* Is it known? */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED) {
			/* Mark as forgotten */
			p_ptr->spell_flags[j] |= PY_SPELL_FORGOTTEN;

			/* No longer known */
			p_ptr->spell_flags[j] &= ~PY_SPELL_LEARNED;

			/* Message */
			msg("You have forgotten the %s of %s.",
				magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
				get_spell_name(mt_ptr->index));

			/* One more can be learned */
			p_ptr->new_spells++;
		}
	}


	/* Forget spells if we know too many spells */
	for (i = PY_MAX_SPELLS; i >= 0; i--) {
		/* Stop when possible */
		if (p_ptr->new_spells >= 0)
			break;

		/* Get the (i+1)th spell learned */
		j = p_ptr->spell_order[i];

		/* Skip unknown spells */
		if (j >= 99)
			continue;

		/* Get the spell */
		mt_ptr = &mp_ptr->info[j];

		/* Forget it (if learned) */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED) {
			/* Mark as forgotten */
			p_ptr->spell_flags[j] |= PY_SPELL_FORGOTTEN;

			/* No longer known */
			p_ptr->spell_flags[j] &= ~PY_SPELL_LEARNED;

			/* Message */
			msg("You have forgotten the %s of %s.",
				magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
				get_spell_name(mt_ptr->index));

			/* One more can be learned */
			p_ptr->new_spells++;
		}
	}


	/* Check for spells to remember */
	for (i = 0; i < PY_MAX_SPELLS; i++) {
		/* None left to remember */
		if (p_ptr->new_spells <= 0)
			break;

		/* Get the next spell we learned */
		j = p_ptr->spell_order[i];

		/* Skip unknown spells */
		if (j >= 99)
			break;

		/* Access the spell */
		mt_ptr = &mp_ptr->info[j];

		/* Skip spells we cannot remember */
		if (mt_ptr->slevel > p_ptr->lev)
			continue;

		/* First set of spells */
		if (p_ptr->spell_flags[j] & PY_SPELL_FORGOTTEN) {
			/* No longer forgotten */
			p_ptr->spell_flags[j] &= ~PY_SPELL_FORGOTTEN;

			/* Known once more */
			p_ptr->spell_flags[j] |= PY_SPELL_LEARNED;

			/* Message */
			msg("You have remembered the %s of %s.",
				magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
				get_spell_name(mt_ptr->index));

			/* One less can be learned */
			p_ptr->new_spells--;
		}
	}


	/* Assume no spells available */
	k = 0;

	/* Count spells that can be learned */
	for (j = 0; j < mp_ptr->spell_number; j++) {
		/* Access the spell */
		mt_ptr = &mp_ptr->info[j];

		/* Skip spells we cannot remember */
		if (mt_ptr->slevel > p_ptr->lev)
			continue;

		/* Skip spells we already know */
		if (p_ptr->spell_flags[j] & PY_SPELL_LEARNED) {
			continue;
		}

		/* Count it */
		k++;
	}

	/* Cannot learn more spells than exist */
	if (p_ptr->new_spells > k)
		p_ptr->new_spells = k;

	/* Spell count changed */
	if (old_spells != p_ptr->new_spells) {
		/* Message if needed */
		if (p_ptr->new_spells) {
			/* Message */
			msg("You can learn %d more %s%s.", p_ptr->new_spells,
				magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
				((p_ptr->new_spells != 1)
				 && (mp_ptr->spell_book != TV_DRUID_BOOK)) ? "s" : "");
		}

		/* Redraw Study Status */
		p_ptr->redraw |= (PR_STUDY | PR_OBJECT);
	}
}


/**
 * Calculate number of specialties player should have. -BR-
 */
static void calc_specialty(void)
{
	int i;
	int num_known, questortwo = 2;

	/* Calculate number allowed */
	if (p_ptr->quests < 2)		/* -NRM- */
		questortwo = p_ptr->quests;
	p_ptr->specialties_allowed = 1 + questortwo;
	if (player_has(PF_XTRA_SPECIALTY))
		p_ptr->specialties_allowed++;
	if (p_ptr->specialties_allowed > MAX_SPECIALTIES)
		p_ptr->specialties_allowed = MAX_SPECIALTIES;

	/* Assume none known */
	num_known = 0;

	/* Count the number of specialties we know */
	for (i = 0; i < MAX_SPECIALTIES; i++) {
		if (p_ptr->specialty_order[i] != PF_NO_SPECIALTY)
			num_known++;
	}

	/* See how many specialties we must forget or may learn */
	p_ptr->new_specialties = p_ptr->specialties_allowed - num_known;

	/* Check if specialty array is full */
	if ((num_known == MAX_SPECIALTIES) && (p_ptr->new_specialties > 0))
		p_ptr->new_specialties = 0;

	/* More specialties are available (or fewer forgotten) */
	if (p_ptr->old_specialties < p_ptr->new_specialties) {
		if (p_ptr->old_specialties < 0) {
			msg("You have regained specialist abilities.");

			/* Put forgotten specialties back on the flags */
			for (i = MIN(0, p_ptr->new_specialties);
				 i > p_ptr->old_specialties; i--)
				pf_on(p_ptr->pflags,
					  p_ptr->specialty_order[num_known + i]);
		}

		if (p_ptr->new_specialties > 0)
			msg("You may learn a specialist ability using the 'O' key.");

		/* Redraw Study Status */
		p_ptr->redraw |= (PR_STUDY);
	}

	/* Fewer specialties are available (or more forgotten) */
	if (p_ptr->old_specialties > p_ptr->new_specialties) {
		if (p_ptr->new_specialties < 0) {
			msg("You have lost specialist abilities.");

			/* Remove forgotten specialties from the flags */
			for (i = 0; i > p_ptr->new_specialties; i--)
				pf_off(p_ptr->pflags,
					   p_ptr->specialty_order[num_known - i]);
		}

		/* Redraw Study Status */
		p_ptr->redraw |= (PR_STUDY);
	}

	/* Save the new_specialties value */
	p_ptr->old_specialties = p_ptr->new_specialties;

}


/**
 * Calculate maximum mana.  You do not need to know any spells.
 * Note that mana is lowered by heavy (or inappropriate) armor, and
 * by a shapeshift.
 *
 * This function induces status messages.
 *
 * New treatment of encumberance by LM
 */
static void calc_mana(void)
{
	int msp, levels, cur_wgt, max_wgt, penalty_wgt, armor_penalty;

	object_type *o_ptr;

	bool old_cumber_glove = p_ptr->cumber_glove;
	bool old_cumber_armor = p_ptr->cumber_armor;

	/* Hack -- Must possess some magical realm. */
	if (!mp_ptr->spell_realm) {
		p_ptr->msp = 0;
		p_ptr->csp = 0;
		return;
	}

	/* Hack -- handle "xtra" mode */
	if (character_xtra)
		return;

	/* Extract "effective" player level */
	levels = (p_ptr->lev - mp_ptr->spell_first) + 1;

	/* Hack -- no negative mana */
	if (levels < 0)
		levels = 0;

	/* Extract total mana, using standard rounding. */
	msp =
		(adj_mag_mana[p_ptr->state.stat_ind[mp_ptr->spell_stat]] * levels +
		 5) / 10;

	/* The weak spellcasters get half as much mana (rounded up) in Oangband. */
	if (!(player_has(PF_STRONG_MAGIC)))
		msp = (msp + 1) / 2;

	/* Hack -- usually add one mana */
	if (msp)
		msp++;

	/* Modest boost for Clarity ability */
	if (player_has(PF_CLARITY))
		msp += msp / 20;

	/* Only mage and Necromancer-type spellcasters are affected by gloves. */
	if ((mp_ptr->spell_book == TV_MAGIC_BOOK)
		|| (mp_ptr->spell_book == TV_NECRO_BOOK)) {
		/* Assume player is not encumbered by gloves */
		p_ptr->cumber_glove = FALSE;

		/* Get the gloves */
		o_ptr = &p_ptr->inventory[INVEN_HANDS];

		/* Normal gloves hurt mage or necro-type spells.  Now, only Free Action 
		 * or magic mastery stops this effect. */
		if (o_ptr->k_idx && !of_has(o_ptr->flags_obj, OF_FREE_ACT)
			&& (o_ptr->bonus_other[P_BONUS_M_MASTERY] <= 0)) {
			/* Encumbered */
			p_ptr->cumber_glove = TRUE;

			/* Reduce mana */
			msp = 3 * msp / 4;
		}
	}


	/* Assume player not encumbered by armor */
	p_ptr->cumber_armor = FALSE;

	/* Weigh the armor */
	cur_wgt = 0;
	cur_wgt += p_ptr->inventory[INVEN_BODY].weight;
	cur_wgt += p_ptr->inventory[INVEN_HEAD].weight;
	cur_wgt += p_ptr->inventory[INVEN_ARM].weight;
	cur_wgt += p_ptr->inventory[INVEN_OUTER].weight;
	cur_wgt += p_ptr->inventory[INVEN_HANDS].weight;
	cur_wgt += p_ptr->inventory[INVEN_FEET].weight;

	/* Determine the weight allowance */
	max_wgt = mp_ptr->spell_weight1;
	penalty_wgt = mp_ptr->spell_weight2;

	/* Specialist Ability */
	if (player_has(PF_ARMOR_PROFICIENCY)) {
		max_wgt += 50;
		penalty_wgt += 150;
	}

	/* Heavy armor penalizes mana by a percentage. */
	if (cur_wgt > max_wgt) {
		/* Calculate Penalty */
		armor_penalty = (msp * (cur_wgt - max_wgt)) / penalty_wgt;

		/* If non-zero */
		if (armor_penalty) {
			/* Encumbered */
			p_ptr->cumber_armor = TRUE;
			msp -= armor_penalty;
		}
	}

	/* Any non-humaniod shape penalizes mana, unless prevented by specialty
	 * ability */
	if (p_ptr->schange) {
		/* Chop mana to 2/3. */
		msp = 2 * msp / 3;
	}


	/* Mana can never be negative */
	if (msp < 0)
		msp = 0;


	/* Maximum mana has changed */
	if (p_ptr->msp != msp) {
		/* Save new limit */
		p_ptr->msp = msp;

		/* Enforce new limit */
		if (p_ptr->csp >= msp) {
			p_ptr->csp = msp;
			p_ptr->csp_frac = 0;
		}

		/* Display mana later */
		p_ptr->redraw |= (PR_MANA);
	}


	/* Take note when "glove state" changes */
	if (old_cumber_glove != p_ptr->cumber_glove) {
		/* Message */
		if (p_ptr->cumber_glove) {
			msg("Your covered hands feel unsuitable for spellcasting.");
		} else {
			msg("Your hands feel more suitable for spellcasting.");
		}
	}


	/* Take note when "armor state" changes */
	if (old_cumber_armor != p_ptr->cumber_armor) {
		/* Message */
		if (p_ptr->cumber_armor) {
			msg("The weight of your armor encumbers your movement.");
		} else {
			msg("You feel able to move more freely.");
		}
	}
}



/**
 * Calculate the players (maximal) hit points
 *
 * Adjust current hitpoints if necessary
 */
static void calc_hitpoints(void)
{
	int bonus, mhp;

	/* Hack -- handle "xtra" mode */
	if (character_xtra)
		return;

	/* Un-inflate "half-hitpoint bonus per level" value */
	bonus = ((int) (adj_con_mhp[p_ptr->state.stat_ind[A_CON]]) - 128);

	/* Calculate hitpoints */
	mhp = p_ptr->player_hp[p_ptr->lev - 1] + (bonus * p_ptr->lev / 2);

	/* Always have at least one hitpoint per level */
	if (mhp < p_ptr->lev + 1)
		mhp = p_ptr->lev + 1;

	/* Modest boost for Athletics ability */
	if (player_has(PF_ATHLETICS))
		mhp += mhp / 20;

	/* New maximum hitpoints */
	if (p_ptr->mhp != mhp) {
		/* Save new limit */
		p_ptr->mhp = mhp;

		/* Enforce new limit */
		if (p_ptr->chp >= mhp) {
			p_ptr->chp = mhp;
			p_ptr->chp_frac = 0;
		}

		/* Display hitpoints (later) */
		p_ptr->redraw |= (PR_HP);
	}
}



/**
 * Extract and set the current "light radius"
 */
static void calc_torch(void)
{
	object_type *o_ptr = &p_ptr->inventory[INVEN_LIGHT];

	s16b old_light = p_ptr->cur_light;

	/* Assume no light */
	p_ptr->cur_light = 0;

	/* Player is glowing */
	if (p_ptr->state.light) {
		notice_obj(OF_LIGHT, 0);
		p_ptr->cur_light += 1;
	}

	/* Examine actual lights */
	if (o_ptr->tval == TV_LIGHT) {
		/* Torches (with fuel) provide some light */
		if ((o_ptr->sval == SV_LIGHT_TORCH) && (o_ptr->pval > 0)) {
			p_ptr->cur_light += 1;
		}

		/* Lanterns (with fuel) provide more light */
		if ((o_ptr->sval == SV_LIGHT_LANTERN) && (o_ptr->pval > 0)) {
			p_ptr->cur_light += 2;
		}

		/* Artifact Lights provide permanent, bright, light */
		if (artifact_p(o_ptr))
			p_ptr->cur_light += 3;
	}

	/* Priests and Paladins get a bonus to light radius at level 35 and 45,
	 * respectively. */
	if (player_has(PF_HOLY)) {
		/* Hack -- the "strong caster" check here is a hack. What is the better 
		 * option? */
		if ((p_ptr->lev > 44)
			|| ((p_ptr->lev > 34) && (player_has(PF_STRONG_MAGIC)))) {
			p_ptr->cur_light += 1;
		}
	}

	/* Special ability Holy Light */
	if (player_has(PF_HOLY_LIGHT))
		p_ptr->cur_light++;

	/* Special ability Unlight */
	if (player_has(PF_UNLIGHT) || p_ptr->state.darkness) {
		notice_obj(OF_DARKNESS, 0);
		p_ptr->cur_light--;
	}

	/* Vampire shape */
	if ((p_ptr->schange == SHAPE_VAMPIRE) && (p_ptr->cur_light >= 3))
		p_ptr->cur_light = 2;

	/* Notice changes in the "light radius" */
	if (old_light != p_ptr->cur_light) {
		/* Update the visuals */
		p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);
	}
}



/**
 * Computes current weight limit.
 */
static int weight_limit(player_state * state)
{
	int i;

	/* Weight limit based only on strength */
	i = adj_str_wgt[state->stat_ind[A_STR]] * 100;

	/* Return the result */
	return (i);
}

/*
 * Computes weight remaining before burdened.
 */
int weight_remaining()
{
	int i;

	/* Weight limit based only on strength */
	i = 60 * adj_str_wgt[p_ptr->state.stat_ind[A_STR]] -
		p_ptr->total_weight - 1;

	/* Return the result */
	return (i);
}

/** Calculate all class-based bonuses and penalties to melee Skill.  Oangband
 * recognizes that it takes a great deal of training to get critical hits with
 * a large, heavy weapon - training that many classes simply do not have the
 * time or inclination for.  -LM- 
 */
int add_special_melee_skill(player_state * state, s16b weight,
							const object_type * o_ptr)
{
	int add_skill = 0;
	int max_weight = 0;

	/* Druids and Martial Artists love to fight barehanded */
	if (!o_ptr->k_idx) {
		if (player_has(PF_UNARMED_COMBAT))
			add_skill = 14 + (p_ptr->lev);
		else if (player_has(PF_MARTIAL_ARTS))
			add_skill = p_ptr->lev / 2;
	}



	/* Otherwise, check weapon weight */
	else {
		/* Maximum compfortable weight depends on class and level */
		max_weight =
			cp_ptr->max_1 +
			((p_ptr->lev * (cp_ptr->max_50 - cp_ptr->max_1)) / 50);

		/* Too heavy */
		if (weight > max_weight) {
			/* Penalize */
			add_skill = -((weight - max_weight) * cp_ptr->penalty) / 100;
			if (add_skill < -(cp_ptr->max_penalty))
				add_skill = -(cp_ptr->max_penalty);
		}

		/* Some classes (Rogues and Assasins) benefit from extra light weapons */
		else if (cp_ptr->bonus) {
			/* Apply bonus */
			add_skill = ((max_weight - weight) * cp_ptr->bonus) / 100;
			if (add_skill > cp_ptr->max_bonus)
				add_skill = cp_ptr->max_bonus;
		}
	}

	/* Priest penalty for non-blessed edged weapons. */
	if ((player_has(PF_BLESS_WEAPON)) && (player_has(PF_STRONG_MAGIC))
		&& ((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM))
		&& (!state->bless_blade)) {
		add_skill -= 10 + p_ptr->lev / 2;

		/* Icky weapon */
		state->icky_wield = TRUE;
	}

	/* Paladin bonus for blessed weapons. */
	if ((player_has(PF_BLESS_WEAPON)) && (!player_has(PF_STRONG_MAGIC))
		&& (state->bless_blade))
		add_skill += 10 + p_ptr->lev / 4;

	/* Now, special racial abilities and limitations are considered.  Most
	 * modifiers are relatively small, to keep options open to the player. */
	if (o_ptr->tval == TV_SWORD) {
		if (player_has(PF_SWORD_SKILL))
			add_skill += 3 + p_ptr->lev / 7;
		else if (player_has(PF_SWORD_UNSKILL))
			add_skill -= 3 + p_ptr->lev / 7;
	}

	else if (o_ptr->tval == TV_POLEARM) {
		if (player_has(PF_POLEARM_SKILL))
			add_skill += 3 + p_ptr->lev / 7;
		else if (player_has(PF_POLEARM_UNSKILL))
			add_skill -= 3 + p_ptr->lev / 7;
	}

	else if (o_ptr->tval == TV_HAFTED) {
		if (player_has(PF_HAFTED_SKILL))
			add_skill += 3 + p_ptr->lev / 7;
		else if (player_has(PF_HAFTED_UNSKILL))
			add_skill -= 3 + p_ptr->lev / 7;
	}

	return (add_skill);
}

/** Calculate all class and race-based bonuses and 
 * penalties to missile Skill 
 */
int add_special_missile_skill(player_state * state, s16b weight,
							  const object_type * o_ptr)
{
	int add_skill = 0;

	/* Nice bonus for most favored weapons - if no tradeoff */
	if ((((player_has(PF_BOW_SPEED_GREAT))
		  && (state->ammo_tval == TV_ARROW))
		 || ((player_has(PF_SLING_SPEED_GREAT))
			 && (state->ammo_tval == TV_SHOT))
		 || ((player_has(PF_XBOW_SPEED_GREAT))
			 && (state->ammo_tval == TV_BOLT)))
		&& (!player_has(PF_RAPID_FIRE))) {
		/* Big bonus */
		add_skill = 3 + p_ptr->lev / 4;
	}

	/* Hack - Unarmed fighters (i.e. Druids) do a bit better with slings */
	if ((player_has(PF_UNARMED_COMBAT)) & (state->ammo_tval == TV_SHOT)) {
		add_skill = p_ptr->lev / 7;
	}

	/* Now, special racial abilities and limitations are considered.  The
	 * choice of race can be of some significance. */

	if (state->ammo_tval == TV_BOLT) {
		if (player_has(PF_XBOW_SKILL))
			add_skill += 3 + p_ptr->lev / 7;
		else if (player_has(PF_XBOW_UNSKILL))
			add_skill -= 3 + p_ptr->lev / 7;
	} else if (state->ammo_tval == TV_ARROW) {
		if (player_has(PF_BOW_SKILL))
			add_skill += 3 + p_ptr->lev / 7;
		else if (player_has(PF_BOW_UNSKILL))
			add_skill -= 3 + p_ptr->lev / 7;
	} else if (state->ammo_tval == TV_SHOT) {
		if (player_has(PF_SLING_SKILL))
			add_skill += 3 + p_ptr->lev / 7;
		else if (player_has(PF_SLING_UNSKILL))
			add_skill -= 3 + p_ptr->lev / 7;
	}
	return (add_skill);
}


/**
 * Paranoid bounds checking on player resistance arrays.
 *
 * These now represent what percentage of damage the player takes -NRM-
 */
static void resistance_limits(player_state * state)
{
	int i;

	/* Check all extremes */
	for (i = 0; i < MAX_P_RES; i++) {
		if (state->res_list[i] > RES_LEVEL_MAX)
			state->res_list[i] = RES_LEVEL_MAX;
		if (state->res_list[i] < RES_LEVEL_MIN)
			state->res_list[i] = RES_LEVEL_MIN;
		if (state->dis_res_list[i] > RES_LEVEL_MAX)
			state->dis_res_list[i] = RES_LEVEL_MAX;
		if (state->dis_res_list[i] < RES_LEVEL_MIN)
			state->dis_res_list[i] = RES_LEVEL_MIN;
	}
}

/**
 * Apply a percentage resistance to the existing player resistance level.
 */
extern void apply_resist(int *player_resist, int item_resist)
{
	*player_resist = (int) (*player_resist * item_resist) / 100;
}



/** Applies vital statistic changes from a shapeshift 
 * to the player.
*/
static void shape_change_stat(player_state * state)
{
	switch (p_ptr->schange) {
	case SHAPE_NORMAL:
		break;
	case SHAPE_MOUSE:
		{
			state->stat_add[A_STR] -= 2;
			state->stat_add[A_INT] -= 7;
			state->stat_add[A_CON] -= 1;
			state->stat_add[A_CHR] -= 5;
			break;
		}
	case SHAPE_FERRET:
		{
			state->stat_add[A_DEX] += 4;
			state->stat_add[A_CHR] -= 2;
			break;
		}
	case SHAPE_HOUND:
		{
			state->stat_add[A_CON] += 2;
			state->stat_add[A_INT] -= 2;
			state->stat_add[A_CHR] -= 2;
			break;
		}
	case SHAPE_GAZELLE:
		{
			state->stat_add[A_STR] -= 2;
			state->stat_add[A_DEX] += 2;
			state->stat_add[A_CON] -= 1;
			state->stat_add[A_WIS] -= 2;
			break;
		}
	case SHAPE_LION:
		{
			state->stat_add[A_STR] += 3;
			state->stat_add[A_CHR] -= 4;
			state->stat_add[A_WIS] -= 2;
			state->stat_add[A_INT] -= 2;
			break;
		}
	case SHAPE_ENT:
		{
			state->stat_add[A_STR] += 4;
			state->stat_add[A_WIS] += 1;
			state->stat_add[A_DEX] -= 5;
			state->stat_add[A_CON] += 4;
			state->stat_add[A_CHR] -= 1;
			break;
		}
	case SHAPE_BAT:
		{
			state->stat_add[A_STR] -= 1;
			state->stat_add[A_WIS] -= 2;
			state->stat_add[A_INT] -= 2;
			state->stat_add[A_CHR] -= 2;
			break;
		}
	case SHAPE_WEREWOLF:
		{
			state->stat_add[A_STR] += 2;
			state->stat_add[A_CHR] -= 5;
			state->stat_add[A_INT] -= 2;
			break;
		}
	case SHAPE_VAMPIRE:
		{
			state->stat_add[A_STR] += 2;
			state->stat_add[A_CON] += 1;
			state->stat_add[A_INT] += 2;
			state->stat_add[A_CHR] -= 3;
			break;
		}
	case SHAPE_WYRM:
		{
			state->stat_add[A_STR] += 2;
			state->stat_add[A_CON] += 1;
			state->stat_add[A_WIS] += 1;
			state->stat_add[A_INT] += 1;
			state->stat_add[A_DEX] -= 1;
			state->stat_add[A_CHR] -= 1;
			break;
		}
	case SHAPE_BEAR:
		{
			state->stat_add[A_STR] += 1;
			if (p_ptr->lev >= 10)
				state->stat_add[A_STR] += 1;
			if (p_ptr->lev >= 20)
				state->stat_add[A_CON] += 1;
			if (p_ptr->lev >= 30)
				state->stat_add[A_CON] += 1;
			if (p_ptr->lev >= 40)
				state->stat_add[A_STR] += 1;
			state->stat_add[A_INT] -= 1;
			state->stat_add[A_CHR] -= 1;
			break;
		}
	}
}

/** A Sangband-derived function to apply all non-stat changes from a shapeshift 
 * to the player.  Any alterations also need to be added to the character 
 * screen (files.c, function "player_flags"), and all timed states 
 * (opposition to the elements for example) must be hacked into the timing of 
 * player states in  dungeon.c.  -LM-
 */
static void shape_change_main(player_state * state)
{
	object_type *o_ptr;
	switch (p_ptr->schange) {
	case SHAPE_NORMAL:
		break;
	case SHAPE_MOUSE:
		{
			state->skills[SKILL_STEALTH] =
				(30 + state->skills[SKILL_STEALTH]) / 2;
			state->see_infra += 2;
			state->aggravate = FALSE;
			state->to_a -= 5;
			state->dis_to_a -= 5;
			state->to_h -= 15;
			state->dis_to_h -= 15;
			state->to_d -= 25;
			state->dis_to_d -= 25;
			state->skills[SKILL_DEVICE] /= 4;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_FERRET:
		{
			state->see_infra += 2;
			state->regenerate = TRUE;
			state->to_d -= 10;
			state->dis_to_d -= 10;
			state->pspeed += 2;
			state->skills[SKILL_SEARCH_FREQUENCY] += 10;
			state->skills[SKILL_SEARCH] += 10;
			state->skills[SKILL_DEVICE] /= 2;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_HOUND:
		{
			state->see_infra += 3;
			state->telepathy = TRUE;
			state->skills[SKILL_DEVICE] /= 2;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_GAZELLE:
		{
			state->to_a += 5;
			state->dis_to_a += 5;
			state->to_d -= 5;
			state->dis_to_d -= 5;
			state->pspeed += 6;
			state->skills[SKILL_DEVICE] /= 2;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_LION:
		{
			state->no_fear = TRUE;
			state->regenerate = TRUE;
			state->to_a += 5;
			state->dis_to_a += 5;
			state->to_h += 10;
			state->dis_to_h += 10;
			state->to_d += 15;
			state->dis_to_d += 15;
			state->pspeed += 1;
			state->skills[SKILL_DEVICE] /= 2;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_ENT:
		{
			apply_resist(&state->res_list[P_RES_COLD], RES_BOOST_NORMAL);
			apply_resist(&state->dis_res_list[P_RES_COLD],
						 RES_BOOST_NORMAL);
			apply_resist(&state->res_list[P_RES_POIS], RES_BOOST_NORMAL);
			apply_resist(&state->dis_res_list[P_RES_POIS],
						 RES_BOOST_NORMAL);
			apply_resist(&state->res_list[P_RES_FIRE], RES_CUT_MINOR);
			apply_resist(&state->dis_res_list[P_RES_FIRE], RES_CUT_MINOR);
			if (state->res_list[P_RES_FIRE] < RES_CAP_MODERATE)
				state->res_list[P_RES_FIRE] = RES_CAP_MODERATE;
			if (state->dis_res_list[P_RES_FIRE] < RES_CAP_MODERATE)
				state->dis_res_list[P_RES_FIRE] = RES_CAP_MODERATE;
			state->no_fear = TRUE;
			state->see_inv = TRUE;
			state->free_act = TRUE;
			state->ffall = FALSE;
			state->to_d += 10;
			state->dis_to_d += 10;
			state->skills[SKILL_DIGGING] += 150;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_BAT:
		{
			state->see_infra += 6;
			state->no_blind = TRUE;
			state->ffall = TRUE;
			state->to_h -= 5;
			state->dis_to_h -= 5;
			state->to_d -= 15;
			state->dis_to_d -= 15;
			state->pspeed += 5;
			state->skills[SKILL_DEVICE] /= 4;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_WEREWOLF:
		{
			state->see_infra += 3;
			state->regenerate = TRUE;
			state->aggravate = TRUE;
			state->to_a += 5;
			state->dis_to_a += 5;
			state->to_h += 20;
			state->dis_to_h += 20;
			state->to_d += 20;
			state->dis_to_d += 20;
			state->skills[SKILL_DEVICE] /= 2;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	case SHAPE_VAMPIRE:
		{
			state->see_infra += 3;
			state->see_inv = TRUE;
			state->hold_life = TRUE;
			apply_resist(&state->res_list[P_RES_COLD], RES_BOOST_NORMAL);
			apply_resist(&state->dis_res_list[P_RES_COLD],
						 RES_BOOST_NORMAL);
			apply_resist(&state->res_list[P_RES_LIGHT], RES_CUT_MINOR);
			apply_resist(&state->dis_res_list[P_RES_LIGHT], RES_CUT_MINOR);
			if (state->res_list[P_RES_LIGHT] < RES_CAP_EXTREME)
				state->res_list[P_RES_LIGHT] = RES_CAP_EXTREME;
			if (state->dis_res_list[P_RES_LIGHT] < RES_CAP_EXTREME)
				state->dis_res_list[P_RES_LIGHT] = RES_CAP_EXTREME;
			state->regenerate = TRUE;
			state->to_a += 5;
			state->dis_to_a += 5;
			state->to_h += 5;
			state->dis_to_h += 5;
			state->to_d += 5;
			state->dis_to_d += 5;
			state->skills[SKILL_STEALTH] += 1;
			state->skills[SKILL_DEVICE] += 30;
			state->skills[SKILL_TO_HIT_BOW] -= 10;
			state->skills[SKILL_TO_HIT_THROW] -= 10;
			break;
		}
	case SHAPE_WYRM:
		{
			o_ptr = &p_ptr->inventory[INVEN_BODY];
			state->to_a += 10;
			state->dis_to_a += 10;
			state->to_d += 5;
			state->dis_to_d += 5;
			state->skills[SKILL_STEALTH] -= 3;
			state->skills[SKILL_DEVICE] += 10;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;

			/* 
			 * Apply an extra bonus power depending on the type
			 * of DSM when in WYRM form 
			 */
			if (o_ptr->tval == TV_DRAG_ARMOR) {
				/* Elemental DSM -> immunity */
				if (o_ptr->sval == SV_DRAGON_BLACK) {
					apply_resist(&state->res_list[P_RES_ACID],
								 RES_BOOST_IMMUNE);
					apply_resist(&state->dis_res_list[P_RES_ACID],
								 RES_BOOST_IMMUNE);
				} else if (o_ptr->sval == SV_DRAGON_BLUE) {
					apply_resist(&state->res_list[P_RES_ELEC],
								 RES_BOOST_IMMUNE);
					apply_resist(&state->dis_res_list[P_RES_ELEC],
								 RES_BOOST_IMMUNE);
				} else if (o_ptr->sval == SV_DRAGON_WHITE) {
					apply_resist(&state->res_list[P_RES_COLD],
								 RES_BOOST_IMMUNE);
					apply_resist(&state->dis_res_list[P_RES_COLD],
								 RES_BOOST_IMMUNE);
				} else if (o_ptr->sval == SV_DRAGON_RED) {
					apply_resist(&state->res_list[P_RES_FIRE],
								 RES_BOOST_IMMUNE);
					apply_resist(&state->dis_res_list[P_RES_FIRE],
								 RES_BOOST_IMMUNE);
				} else if (o_ptr->sval == SV_DRAGON_GREEN) {
					apply_resist(&state->res_list[P_RES_POIS],
								 RES_BOOST_IMMUNE);
					apply_resist(&state->dis_res_list[P_RES_POIS],
								 RES_BOOST_IMMUNE);
				}

				/* Shining DSM -> SI */
				else if (o_ptr->sval == SV_DRAGON_SHINING)
					state->see_inv = TRUE;

				/* Law/Chaos DSM -> hold life */
				else if (o_ptr->sval == SV_DRAGON_LAW)
					state->hold_life = TRUE;
				else if (o_ptr->sval == SV_DRAGON_CHAOS)
					state->hold_life = TRUE;

				/* Bronze/Gold DSM -> FA */
				else if (o_ptr->sval == SV_DRAGON_BRONZE)
					state->free_act = TRUE;
				else if (o_ptr->sval == SV_DRAGON_GOLD)
					state->free_act = TRUE;

				/* Multihued, Balance and Power don't need any help */
			}
			break;
		}
	case SHAPE_BEAR:
		{
			state->to_a += 5;
			state->dis_to_a += 5;
			state->to_h += 5;
			state->dis_to_h += 5;
			if (p_ptr->lev >= 10) {
				state->to_d += 5;
				state->dis_to_d += 5;
			}
			if (p_ptr->lev >= 20) {
				state->to_d += 5;
				state->dis_to_d += 5;
			}
			if (p_ptr->lev >= 30) {
				state->to_a += 10;
				state->dis_to_a += 10;
			}
			if (p_ptr->lev >= 40) {
				state->to_h += 5;
				state->dis_to_h += 5;
			}
			state->skills[SKILL_DEVICE] /= 2;
			state->skills[SKILL_TO_HIT_BOW] -= 30;
			state->skills[SKILL_TO_HIT_THROW] -= 30;
			break;
		}
	}

	/* End shape bonuses; do bounds check on resistance levels */
	resistance_limits(state);
}



/**
 * Calculate the players current "state", taking into account
 * not only race/class intrinsics, but also objects being worn
 * and temporary spell effects.
 *
 * This is the "kitchen sink" function!	 I may get around to 
 * segmenting it, simply to make it more readable...  -LM-
 *
 * I have added class-specific modifiers to Skill and enforced a max
 * of one blow/rnd for weapons too heavy to wield effectively. -LM-
 *
 * This function calls itself if the player's STR stat changes. -LM-
 *
 * See also calc_mana() and calc_hitpoints().
 *
 * Take note of the new "speed code", in particular, a very strong
 * player will start slowing down as soon as he reaches 150 pounds,
 * but not until he reaches 450 pounds will he be half as fast as
 * a normal kobold.  This both hurts and helps the player, hurts
 * because in the old days a player could just avoid 300 pounds,
 * and helps because now carrying 300 pounds is not very painful.
 *
 * The "weapon" and "bow" do *not* add to the bonuses to hit or to
 * damage, since that would affect non-combat things.  These values
 * are actually added in later, at the appropriate place.
 *
 * This function induces various "status" messages.
 */
extern void calc_bonuses(object_type inventory[], player_state * state,
						 bool inspect)
{
	int i, j, hold;

	int temp_armour;

	int extra_shots = 0;
	int extra_might = 0;

	bool enhance = FALSE;

	object_type *o_ptr;

	/*** Reset ***/

	/* Reset player speed */
	state->pspeed = 110;

	/* Reset "blow" info */
	state->num_blow = 1;

	/* Reset "fire" info */
	state->num_fire = 0;
	state->ammo_mult = 0;
	state->ammo_tval = 0;

	/* Clear the stat modifiers */
	for (i = 0; i < A_MAX; i++)
		state->stat_add[i] = 0;

	/* Clear the Displayed/Real armor class */
	state->dis_ac = state->ac = 0;

	/* Clear the Displayed/Real Bonuses */
	state->dis_to_h = state->to_h = 0;
	state->dis_to_d = state->to_d = 0;
	state->dis_to_a = state->to_a = 0;

	/* Clear all the flags */
	state->teleport = FALSE;
	state->no_teleport = FALSE;
	state->aggravate = FALSE;
	state->rand_aggro = FALSE;
	state->slow_regen = FALSE;
	state->fear = FALSE;
	state->fast_digest = FALSE;
	state->rand_pois = FALSE;
	state->rand_pois_bad = FALSE;
	state->rand_cuts = FALSE;
	state->rand_cuts_bad = FALSE;
	state->rand_hallu = FALSE;
	state->drop_weapon = FALSE;
	state->attract_demon = FALSE;
	state->attract_undead = FALSE;
	state->rand_paral = FALSE;
	state->rand_paral_all = FALSE;
	state->drain_exp = FALSE;
	state->drain_mana = FALSE;
	state->drain_stat = FALSE;
	state->drain_charge = FALSE;
	state->bless_blade = FALSE;
	state->impact = FALSE;
	state->see_inv = FALSE;
	state->free_act = FALSE;
	state->slow_digest = FALSE;
	state->regenerate = FALSE;
	state->ffall = FALSE;
	state->hold_life = FALSE;
	state->telepathy = FALSE;
	state->light = FALSE;
	state->sustain_str = FALSE;
	state->sustain_int = FALSE;
	state->sustain_wis = FALSE;
	state->sustain_con = FALSE;
	state->sustain_dex = FALSE;
	state->sustain_chr = FALSE;
	state->no_fear = FALSE;
	state->no_blind = FALSE;
	state->darkness = FALSE;

	for (i = 0; i < MAX_P_RES; i++) {
		state->res_list[i] = RES_LEVEL_BASE;
		state->dis_res_list[i] = RES_LEVEL_BASE;
	}


	/*** Extract race/class info ***/

	/* Base infravision (purely racial) */
	state->see_infra = rp_ptr->infra;

	/* Base skill -- disarming */
	state->skills[SKILL_DISARM] = rp_ptr->r_skills[SKILL_DISARM] +
		cp_ptr->c_skills[SKILL_DISARM];

	/* Base skill -- magic devices */
	state->skills[SKILL_DEVICE] = rp_ptr->r_skills[SKILL_DEVICE] +
		cp_ptr->c_skills[SKILL_DEVICE];

	/* Base skill -- saving throw */
	state->skills[SKILL_SAVE] = rp_ptr->r_skills[SKILL_SAVE] +
		cp_ptr->c_skills[SKILL_SAVE];

	/* Base skill -- stealth */
	state->skills[SKILL_STEALTH] = rp_ptr->r_skills[SKILL_STEALTH] +
		cp_ptr->c_skills[SKILL_STEALTH];

	/* Base skill -- searching ability */
	state->skills[SKILL_SEARCH] = rp_ptr->r_skills[SKILL_SEARCH] +
		cp_ptr->c_skills[SKILL_SEARCH];

	/* Base skill -- searching frequency */
	state->skills[SKILL_SEARCH_FREQUENCY] =
		rp_ptr->r_skills[SKILL_SEARCH_FREQUENCY] +
		cp_ptr->c_skills[SKILL_SEARCH_FREQUENCY];

	/* Base skill -- combat (melee) */
	state->skills[SKILL_TO_HIT_MELEE] =
		rp_ptr->r_skills[SKILL_TO_HIT_MELEE] +
		cp_ptr->c_skills[SKILL_TO_HIT_MELEE];

	/* Base skill -- combat (shooting) */
	state->skills[SKILL_TO_HIT_BOW] = rp_ptr->r_skills[SKILL_TO_HIT_BOW] +
		cp_ptr->c_skills[SKILL_TO_HIT_BOW];

	/* Base skill -- combat (throwing) */
	state->skills[SKILL_TO_HIT_THROW] =
		rp_ptr->r_skills[SKILL_TO_HIT_THROW] +
		cp_ptr->c_skills[SKILL_TO_HIT_THROW];

	/* Base skill -- digging */
	state->skills[SKILL_DIGGING] = 0;

	/*** Analyze player ***/

	/* Object flags */
	if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_STR))
		state->sustain_str = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_INT))
		state->sustain_int = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_WIS))
		state->sustain_wis = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_DEX))
		state->sustain_dex = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_CON))
		state->sustain_con = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_CHR))
		state->sustain_chr = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SLOW_DIGEST))
		state->slow_digest = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_FEATHER))
		state->ffall = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_LIGHT))
		state->light = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_REGEN))
		state->regenerate = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_TELEPATHY))
		state->telepathy = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SEE_INVIS))
		state->see_inv = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_FREE_ACT))
		state->free_act = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_HOLD_LIFE))
		state->hold_life = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_BLESSED))
		state->bless_blade = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_IMPACT))
		state->impact = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_FEARLESS))
		state->no_fear = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_SEEING))
		state->no_blind = TRUE;
	if (of_has(rp_ptr->flags_obj, OF_DARKNESS))
		state->darkness = TRUE;


	/* Curse flags */
	if (cf_has(rp_ptr->flags_curse, CF_TELEPORT))
		state->teleport = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_NO_TELEPORT))
		state->no_teleport = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_AGGRO_PERM))
		state->aggravate = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_AGGRO_RAND))
		state->rand_aggro = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_SLOW_REGEN))
		state->slow_regen = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_AFRAID))
		state->fear = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_HUNGRY))
		state->fast_digest = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_POIS_RAND))
		state->rand_pois = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_POIS_RAND_BAD))
		state->rand_pois_bad = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_CUT_RAND))
		state->rand_cuts = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_CUT_RAND_BAD))
		state->rand_cuts_bad = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_HALLU_RAND))
		state->rand_hallu = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_DROP_WEAPON))
		state->drop_weapon = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_ATTRACT_DEMON))
		state->attract_demon = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_ATTRACT_UNDEAD))
		state->attract_undead = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_PARALYZE))
		state->rand_paral = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_PARALYZE_ALL))
		state->rand_paral_all = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_DRAIN_EXP))
		state->drain_exp = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_DRAIN_MANA))
		state->drain_mana = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_DRAIN_STAT))
		state->drain_stat = TRUE;
	if (cf_has(rp_ptr->flags_curse, CF_DRAIN_CHARGE))
		state->drain_charge = TRUE;

	/* Resistances */
	for (i = 0; i < MAX_P_RES; i++) {
		state->res_list[i] = rp_ptr->percent_res[i];
		state->dis_res_list[i] = rp_ptr->percent_res[i];
	}

	/* Ent */
	if (player_has(PF_WOODEN)) {
		/* Ents dig like maniacs, but only with their hands. */
		if (!p_ptr->inventory[INVEN_WIELD].k_idx)
			state->skills[SKILL_DIGGING] += p_ptr->lev * 10;

		/* Ents get tougher and stronger as they age, but lose dexterity. */
		if (p_ptr->lev > 25)
			state->stat_add[A_STR]++;
		if (p_ptr->lev > 40)
			state->stat_add[A_STR]++;
		if (p_ptr->lev > 45)
			state->stat_add[A_STR]++;

		if (p_ptr->lev > 25)
			state->stat_add[A_DEX]--;
		if (p_ptr->lev > 40)
			state->stat_add[A_DEX]--;
		if (p_ptr->lev > 45)
			state->stat_add[A_DEX]--;

		if (p_ptr->lev > 25)
			state->stat_add[A_CON]++;
		if (p_ptr->lev > 40)
			state->stat_add[A_CON]++;
		if (p_ptr->lev > 45)
			state->stat_add[A_CON]++;
	}

	/* Warrior. */
	if (player_has(PF_RELENTLESS)) {
		if (p_ptr->lev >= 30)
			state->no_fear = TRUE;
		if (p_ptr->lev >= 40)
			state->regenerate = TRUE;
	}

	/* Specialty ability Holy Light */
	if (player_has(PF_HOLY_LIGHT)) {
		apply_resist(&state->res_list[P_RES_LIGHT], RES_BOOST_NORMAL);
		apply_resist(&state->dis_res_list[P_RES_LIGHT], RES_BOOST_NORMAL);
	}

	/* Specialty ability Unlight */
	if (player_has(PF_UNLIGHT)) {
		apply_resist(&state->res_list[P_RES_DARK], RES_BOOST_NORMAL);
		apply_resist(&state->dis_res_list[P_RES_DARK], RES_BOOST_NORMAL);
	}

	/* End inherent resistances; do bounds check on resistance levels */
	resistance_limits(state);

  /*** Analyze equipment ***/

	/* Scan the equipment */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
		//  o_ptr = &p_ptr->inventory[i];
		o_ptr = &(inventory[i]);
		/* Skip non-objects */
		if (!o_ptr->k_idx)
			continue;

		/* Affect stats */
		for (j = 0; j < A_MAX; j++)
			state->stat_add[j] += o_ptr->bonus_stat[j];

		/* Affect stealth */
		state->skills[SKILL_STEALTH] +=
			o_ptr->bonus_other[P_BONUS_STEALTH];

		/* Affect searching ability (factor of five) */
		state->skills[SKILL_SEARCH] +=
			(o_ptr->bonus_other[P_BONUS_SEARCH] * 5);

		/* Affect searching frequency (factor of five) */
		state->skills[SKILL_SEARCH_FREQUENCY] +=
			(o_ptr->bonus_other[P_BONUS_SEARCH] * 5);

		/* Affect infravision */
		state->see_infra += o_ptr->bonus_other[P_BONUS_INFRA];

		/* Affect digging (factor of 20) */
		state->skills[SKILL_DIGGING] +=
			(o_ptr->bonus_other[P_BONUS_TUNNEL] * 20);

		/* Affect speed */
		state->pspeed += o_ptr->bonus_other[P_BONUS_SPEED];

		state->skills[SKILL_DEVICE] +=
			10 * o_ptr->bonus_other[P_BONUS_M_MASTERY];

		/* Affect shots.  Altered in Oangband. */
		extra_shots += o_ptr->bonus_other[P_BONUS_SHOTS];

		/* Affect might.  Altered in Oangband. */
		extra_might += o_ptr->bonus_other[P_BONUS_MIGHT];

		/* Object flags */
		if (of_has(o_ptr->flags_obj, OF_SUSTAIN_STR))
			state->sustain_str = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SUSTAIN_INT))
			state->sustain_int = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SUSTAIN_WIS))
			state->sustain_wis = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SUSTAIN_DEX))
			state->sustain_dex = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SUSTAIN_CON))
			state->sustain_con = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SUSTAIN_CHR))
			state->sustain_chr = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SLOW_DIGEST))
			state->slow_digest = TRUE;
		if (of_has(o_ptr->flags_obj, OF_FEATHER))
			state->ffall = TRUE;
		if (of_has(o_ptr->flags_obj, OF_LIGHT))
			state->light = TRUE;
		if (of_has(o_ptr->flags_obj, OF_REGEN))
			state->regenerate = TRUE;
		if (of_has(o_ptr->flags_obj, OF_TELEPATHY))
			state->telepathy = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SEE_INVIS))
			state->see_inv = TRUE;
		if (of_has(o_ptr->flags_obj, OF_FREE_ACT))
			state->free_act = TRUE;
		if (of_has(o_ptr->flags_obj, OF_HOLD_LIFE))
			state->hold_life = TRUE;
		if (of_has(o_ptr->flags_obj, OF_FEARLESS))
			state->no_fear = TRUE;
		if (of_has(o_ptr->flags_obj, OF_SEEING))
			state->no_blind = TRUE;
		if (of_has(o_ptr->flags_obj, OF_IMPACT))
			state->impact = TRUE;
		if (of_has(o_ptr->flags_obj, OF_BLESSED))
			state->bless_blade = TRUE;
		if (of_has(o_ptr->flags_obj, OF_DARKNESS))
			state->darkness = TRUE;

		/* Bad flags */
		if (cf_has(o_ptr->flags_curse, CF_TELEPORT))
			state->teleport = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_NO_TELEPORT))
			state->no_teleport = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_AGGRO_PERM))
			state->aggravate = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_AGGRO_RAND))
			state->rand_aggro = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_SLOW_REGEN))
			state->slow_regen = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_AFRAID))
			state->fear = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_HUNGRY))
			state->fast_digest = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_POIS_RAND))
			state->rand_pois = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_POIS_RAND_BAD))
			state->rand_pois_bad = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_CUT_RAND))
			state->rand_cuts = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_CUT_RAND_BAD))
			state->rand_cuts_bad = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_HALLU_RAND))
			state->rand_hallu = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_DROP_WEAPON))
			state->drop_weapon = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_ATTRACT_DEMON))
			state->attract_demon = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_ATTRACT_UNDEAD))
			state->attract_undead = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_PARALYZE))
			state->rand_paral = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_PARALYZE_ALL))
			state->rand_paral_all = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_DRAIN_EXP))
			state->drain_exp = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_DRAIN_MANA))
			state->drain_mana = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_DRAIN_STAT))
			state->drain_stat = TRUE;
		if (cf_has(o_ptr->flags_curse, CF_DRAIN_CHARGE))
			state->drain_charge = TRUE;


		for (j = 0; j < MAX_P_RES; j++)
			apply_resist(&state->res_list[j], o_ptr->percent_res[j]);

		/* Known resistance and immunity flags */
		for (j = 0; j < MAX_P_RES; j++)
			if (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j))
				apply_resist(&state->dis_res_list[j],
							 o_ptr->percent_res[j]);
		/* End item resistances; do bounds check on resistance levels */
		resistance_limits(state);

		/* 
		 * Modify the base armor class.   Shields worn on back are penalized. 
		 * Shield and Armor masters benefit.
		 */
		if ((i == INVEN_ARM) && (state->shield_on_back))
			temp_armour = o_ptr->ac / 3;
		else if ((i == INVEN_ARM) && (player_has(PF_SHIELD_MAST)))
			temp_armour = o_ptr->ac * 2;
		else if ((i == INVEN_BODY) && (player_has(PF_ARMOR_MAST)))
			temp_armour = (o_ptr->ac * 5) / 3;
		else
			temp_armour = o_ptr->ac;

		state->ac += temp_armour;

		/* The base armor class is always known */
		state->dis_ac += temp_armour;

		/* Apply the bonuses to armor class.  Shields worn on back are
		 * penalized. */
		if ((state->shield_on_back) && (i == INVEN_ARM))
			temp_armour = o_ptr->to_a / 2;
		else
			temp_armour = o_ptr->to_a;

		state->to_a += temp_armour;

		/* Apply the mental bonuses to armor class, if known */
		if (if_has(o_ptr->id_other, IF_TO_A))
			state->dis_to_a += temp_armour;

		/* Hack -- do not apply "weapon" bonuses */
		if (i == INVEN_WIELD)
			continue;

		/* Hack -- do not apply "bow" bonuses */
		if (i == INVEN_BOW)
			continue;

		/* Apply the bonuses to hit/damage */
		state->to_h += o_ptr->to_h;
		state->to_d += o_ptr->to_d;

		/* Apply the mental bonuses tp hit/damage, if known */
		if (if_has(o_ptr->id_other, IF_TO_H))
			state->dis_to_h += o_ptr->to_h;
		if (if_has(o_ptr->id_other, IF_TO_D))
			state->dis_to_d += o_ptr->to_d;
	}

	/* Hack -- clear a few flags for certain races. */

	/* The dark elf's saving grace */
	if ((player_has(PF_SHADOW)) && (state->aggravate)) {
		state->skills[SKILL_STEALTH] -= 3;
		state->aggravate = FALSE;
	}

	/* Nothing, but nothing, can make an Ent lightfooted. */
	if (player_has(PF_WOODEN))
		state->ffall = FALSE;


	/*** Analyze shapechanges - statistics only ***/
	shape_change_stat(state);

	/*** (Most) Specialty Abilities ***/

	/* Physical stat boost */
	if (player_has(PF_ATHLETICS)) {
		state->stat_add[A_DEX] += 2;
		state->stat_add[A_CON] += 2;
	}

	/* Mental stat boost */
	if (player_has(PF_CLARITY)) {
		state->stat_add[A_INT] += 2;
		state->stat_add[A_WIS] += 2;
	}

	/* Unlight stealth boost */
	if (player_has(PF_UNLIGHT)) {
		if ((p_ptr->cur_light <= 0) && (!is_daylight)
			&& !sqinfo_has(cave_info[p_ptr->py][p_ptr->px], SQUARE_GLOW))
			state->skills[SKILL_STEALTH] += 6;
		else
			state->skills[SKILL_STEALTH] += 3;
	}

	/* Speed Boost (Fury, Phasewalk) */
	if (p_ptr->speed_boost) {
		state->pspeed += (p_ptr->speed_boost + 5) / 10;
	}

	/* Speed boost in trees for elven druids and rangers */
	if ((player_has(PF_WOODSMAN)) && (player_has(PF_ELVEN))
		&& tf_has(f_info[cave_feat[p_ptr->py][p_ptr->px]].flags, TF_TREE))
		state->pspeed += 3;

	/* Speed boost for rune of speed */
	if (cave_trap_specific(p_ptr->py, p_ptr->px, RUNE_SPEED))
		state->pspeed += 10;

	/* Dwarves are good miners */
	if (player_has(PF_DWARVEN))
		state->skills[SKILL_DIGGING] += 40;

	/*** Handle stats ***/

	/* Calculate stats */
	for (i = 0; i < A_MAX; i++) {
		int add, top, use, ind;

		/* Extract modifier */
		add = state->stat_add[i];

		/* Modify the stats for race/class */
		add += (rp_ptr->r_adj[i] + cp_ptr->c_adj[i]);

		/* Extract the new "stat_top" value for the stat */
		top = modify_stat_value(p_ptr->stat_max[i], add);

		/* Save the new value */
		state->stat_top[i] = top;

		/* Extract the new "stat_use" value for the stat */
		use = modify_stat_value(p_ptr->stat_cur[i], add);

		/* Save the new value */
		state->stat_use[i] = use;

		/* Values: 3, 4, ..., 17 */
		if (use <= 18)
			ind = (use - 3);

		/* Ranges: 18/00-18/09, ..., 18/210-18/219 */
		else if (use <= 18 + 219)
			ind = (15 + (use - 18) / 10);

		/* Range: 18/220+ */
		else
			ind = (37);

		/* Save the new index */
		state->stat_ind[i] = ind;
	}

	/* Assume no evasion */
	state->evasion_chance = 0;

	/* Evasion AC boost */
	if (player_has(PF_EVASION)
		|| ((player_has(PF_DWARVEN))
			&& (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAIN))
		|| ((player_has(PF_PLAINSMAN))
			&& (stage_map[p_ptr->stage][STAGE_TYPE] == PLAIN))
		|| ((player_has(PF_EDAIN))
			&& (stage_map[p_ptr->stage][STAGE_TYPE] == FOREST))) {
		int cur_wgt = 0;
		int evasion_wgt;
		int max_bonus;

		/* Weigh the armor */
		cur_wgt += p_ptr->inventory[INVEN_BODY].weight;
		cur_wgt += p_ptr->inventory[INVEN_HEAD].weight;
		cur_wgt += p_ptr->inventory[INVEN_ARM].weight;
		cur_wgt += p_ptr->inventory[INVEN_OUTER].weight;
		cur_wgt += p_ptr->inventory[INVEN_HANDS].weight;
		cur_wgt += p_ptr->inventory[INVEN_FEET].weight;

		/* Highest weight to get any bonus */
		evasion_wgt = 150 + (3 * p_ptr->lev);

		/* Highest bonus we can get at this level */
		max_bonus = adj_dex_evas[state->stat_ind[A_DEX]];

		/* Do we get the max bonus? */
		if (cur_wgt <= ((6 * evasion_wgt) / 10)) {
			state->evasion_chance = max_bonus;
		}

		/* Do we get any bonus? */
		else if (cur_wgt <= evasion_wgt) {
			state->evasion_chance = max_bonus / 2;
		}
	}


  /*** Temporary flags ***/

	/* Hack - Temporary bonuses are stronger with Enhance Magic */
	if (player_has(PF_ENHANCE_MAGIC))
		enhance = TRUE;

	/* Temporary resists */
	if (p_ptr->timed[TMD_OPP_ACID]) {
		int bonus = RES_BOOST_GREAT;
		if (enhance)
			apply_resist(&bonus, RES_BOOST_MINOR);
		apply_resist(&state->res_list[P_RES_ACID], bonus);
		apply_resist(&state->dis_res_list[P_RES_ACID], bonus);
	}
	if (p_ptr->timed[TMD_OPP_FIRE]) {
		int bonus = RES_BOOST_GREAT;
		if (enhance)
			apply_resist(&bonus, RES_BOOST_MINOR);
		apply_resist(&state->res_list[P_RES_FIRE], bonus);
		apply_resist(&state->dis_res_list[P_RES_FIRE], bonus);
	}
	if (p_ptr->timed[TMD_OPP_COLD]) {
		int bonus = RES_BOOST_GREAT;
		if (enhance)
			apply_resist(&bonus, RES_BOOST_MINOR);
		apply_resist(&state->res_list[P_RES_COLD], bonus);
		apply_resist(&state->dis_res_list[P_RES_COLD], bonus);
	}
	if (p_ptr->timed[TMD_OPP_ELEC]) {
		int bonus = RES_BOOST_GREAT;
		if (enhance)
			apply_resist(&bonus, RES_BOOST_MINOR);
		apply_resist(&state->res_list[P_RES_ELEC], bonus);
		apply_resist(&state->dis_res_list[P_RES_ELEC], bonus);
	}
	if (p_ptr->timed[TMD_OPP_POIS]) {
		int bonus = RES_BOOST_GREAT;
		if (enhance)
			apply_resist(&bonus, RES_BOOST_MINOR);
		apply_resist(&state->res_list[P_RES_POIS], bonus);
		apply_resist(&state->dis_res_list[P_RES_POIS], bonus);
	}

	/* Apply temporary "stun".  */
	if (p_ptr->timed[TMD_STUN] > 50) {
		state->to_h -= 20;
		state->dis_to_h -= 20;
		state->to_d -= 20;
		state->dis_to_d -= 20;
	} else if (p_ptr->timed[TMD_STUN]) {
		state->to_h -= 5;
		state->dis_to_h -= 5;
		state->to_d -= 5;
		state->dis_to_d -= 5;
	}

	/* Heightened magical defenses.  Saving Throw effect added later */
	if (p_ptr->timed[TMD_INVULN]) {
		int bonus = ((enhance == TRUE) ? 35 : 25);

		state->to_a += bonus;
		state->dis_to_a += bonus;

		apply_resist(&state->res_list[P_RES_CONFU], RES_BOOST_NORMAL);
		apply_resist(&state->dis_res_list[P_RES_CONFU], RES_BOOST_NORMAL);
		state->no_blind = TRUE;
	}

	/* Temporary blessing */
	if (p_ptr->timed[TMD_BLESSED]) {
		int bonus1 = ((enhance == TRUE) ? 10 : 5);
		int bonus2 = ((enhance == TRUE) ? 15 : 10);

		state->to_a += bonus1;
		state->dis_to_a += bonus1;
		state->to_h += bonus2;
		state->dis_to_h += bonus2;
	}

	/* Temporary shield.  Added an exception for Necromancers to keep them in
	 * line. */
	if ((p_ptr->timed[TMD_SHIELD]) && (player_has(PF_EVIL))) {
		int bonus = ((enhance == TRUE) ? 50 : 35);

		state->to_a += bonus;
		state->dis_to_a += bonus;
	} else if (p_ptr->timed[TMD_SHIELD]) {
		int bonus = ((enhance == TRUE) ? 65 : 50);

		state->to_a += bonus;
		state->dis_to_a += bonus;
	}

	/* Temporary "Hero".  Now also increases Deadliness. */
	if (p_ptr->timed[TMD_HERO]) {
		int bonus1 = ((enhance == TRUE) ? 15 : 10);
		int bonus2 = ((enhance == TRUE) ? 10 : 5);

		state->to_h += bonus1;
		state->dis_to_h += bonus1;
		state->to_d += bonus2;
		state->dis_to_d += bonus2;
	}

	/* Temporary "Berserk".  Now also increases Deadliness. */
	if (p_ptr->timed[TMD_SHERO]) {
		int bonus1 = ((enhance == TRUE) ? 10 : 5);
		int bonus2 = ((enhance == TRUE) ? 20 : 15);
		int bonus3 = ((enhance == TRUE) ? 15 : 10);

		state->to_h += bonus1;
		state->dis_to_h += bonus1;
		state->to_d += bonus2;
		state->dis_to_d += bonus2;
		state->to_a -= bonus3;
		state->dis_to_a -= bonus3;

		/* but berserkers make *lousy* archers. */
		state->skills[SKILL_TO_HIT_BOW] -= 20;
		state->skills[SKILL_TO_HIT_THROW] -= 20;
	}

	/* Temporary "fast" */
	if (p_ptr->timed[TMD_FAST]) {
		int bonus = ((enhance == TRUE) ? 13 : 10);

		state->pspeed += bonus;
	}

	/* Temporary "slow" */
	if (p_ptr->timed[TMD_SLOW]) {
		state->pspeed -= 10;
	}

	/* Temporary see invisible */
	if (p_ptr->timed[TMD_SINVIS]) {
		state->see_inv = TRUE;
	}

	/* Temporary infravision boost.  More useful now. */
	if (p_ptr->timed[TMD_SINFRA]) {
		int bonus = ((enhance == TRUE) ? 5 : 3);

		state->see_infra = state->see_infra + bonus;
	}


	/*** Special flags ***/

	/* Hack -- Hero/Shero -> Res fear */
	if (p_ptr->timed[TMD_HERO] || p_ptr->timed[TMD_SHERO]) {
		state->no_fear = TRUE;
	}

	/* Clear contradictory flags */
	if (state->no_fear)
		state->fear = FALSE;
	if (state->fast_digest && state->slow_digest) {
		state->fast_digest = FALSE;
		state->slow_digest = FALSE;
	}

	if (p_resist_strong(P_RES_POIS)) {
		state->rand_pois = FALSE;
		state->rand_pois_bad = FALSE;
	}

	if (p_resist_good(P_RES_CHAOS))
		state->rand_hallu = FALSE;

	if (state->free_act)
		state->rand_paral = FALSE;

	/* End normal temporary bonuses; do bounds check on resistance levels */
	resistance_limits(state);

	/*** Analyze weight ***/

	/* Extract the current weight (in tenth pounds) */
	j = p_ptr->total_weight;

	/* Extract the "weight limit" (in tenth pounds) */
	i = weight_limit(state);

	/* Apply "encumbrance" from weight - more in water, except Maiar, flyers */
	if (j > i / 2) {
		feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];
		if (tf_has(f_ptr->flags, TF_WATERY)
			&& (!player_has(PF_DIVINE)) && !(p_ptr->schange == SHAPE_BAT)
			&& !(p_ptr->schange == SHAPE_WYRM))
			state->pspeed -= 3 * ((j - (i / 2)) / (i / 10));
		else
			state->pspeed -= ((j - (i / 2)) / (i / 10));
	}

	/* Bloating slows the player down (a little) */
	if (p_ptr->food >= PY_FOOD_MAX)
		state->pspeed -= 10;

	/* Searching slows the player down */
	if (p_ptr->searching)
		state->pspeed -= 10;

	/* Sanity check on extreme speeds */
	if (state->pspeed < 0)
		state->pspeed = 0;
	if (state->pspeed > 199)
		state->pspeed = 199;

  /*** Apply modifier bonuses ***/

	/* Actual Modifier Bonuses (Un-inflate stat bonuses) */
	state->to_a += ((int) (adj_dex_ta[state->stat_ind[A_DEX]]) - 128);
	state->to_d += ((int) (adj_str_td[state->stat_ind[A_STR]]) - 128);
	state->to_h += ((int) (adj_dex_th[state->stat_ind[A_DEX]]) - 128);

	/* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
	state->dis_to_a += ((int) (adj_dex_ta[state->stat_ind[A_DEX]]) - 128);
	state->dis_to_d += ((int) (adj_str_td[state->stat_ind[A_STR]]) - 128);
	state->dis_to_h += ((int) (adj_dex_th[state->stat_ind[A_DEX]]) - 128);


  /*** Modify skills ***/

	/* Affect Skill -- stealth (bonus one) */
	state->skills[SKILL_STEALTH] += 1;

	/* Affect Skill -- disarming (DEX and INT) */
	state->skills[SKILL_DISARM] += adj_dex_dis[state->stat_ind[A_DEX]];
	state->skills[SKILL_DISARM] += adj_int_dis[state->stat_ind[A_INT]];

	/* Affect Skill -- magic devices (INT) */
	state->skills[SKILL_DEVICE] += adj_int_dev[state->stat_ind[A_INT]];

	/* Affect Skill -- saving throw (WIS) */
	state->skills[SKILL_SAVE] += adj_wis_sav[state->stat_ind[A_WIS]];

	/* Affect Skill -- digging (STR) */
	state->skills[SKILL_DIGGING] += adj_str_dig[state->stat_ind[A_STR]];

	/* Affect Skill -- disarming (Level, by Class and Race) */
	state->skills[SKILL_DISARM] +=
		(cp_ptr->cx_skills[SKILL_DISARM] * p_ptr->lev / 50);
	state->skills[SKILL_DISARM] +=
		(rp_ptr->rx_skills[SKILL_DISARM] * p_ptr->lev / 50);

	/* Affect Skill -- magic devices (Level, by Class and Race) */
	state->skills[SKILL_DEVICE] +=
		(cp_ptr->cx_skills[SKILL_DEVICE] * p_ptr->lev / 50);
	state->skills[SKILL_DEVICE] +=
		(rp_ptr->rx_skills[SKILL_DEVICE] * p_ptr->lev / 50);

	/* Affect Skill -- saving throw (Level, by Class and Race) */
	state->skills[SKILL_SAVE] +=
		(cp_ptr->cx_skills[SKILL_SAVE] * p_ptr->lev / 50);
	state->skills[SKILL_SAVE] +=
		(rp_ptr->rx_skills[SKILL_SAVE] * p_ptr->lev / 50);

	/* Affect Skill -- stealth (Level, by Class and Race) */
	state->skills[SKILL_STEALTH] +=
		(cp_ptr->cx_skills[SKILL_STEALTH] * p_ptr->lev / 50);
	state->skills[SKILL_STEALTH] +=
		(rp_ptr->rx_skills[SKILL_STEALTH] * p_ptr->lev / 50);

	/* Affect Skill -- search ability (Level, by Class and Race) */
	state->skills[SKILL_SEARCH] +=
		(cp_ptr->cx_skills[SKILL_SEARCH] * p_ptr->lev / 50);
	state->skills[SKILL_SEARCH] +=
		(rp_ptr->rx_skills[SKILL_SEARCH] * p_ptr->lev / 50);

	/* Affect Skill -- search frequency (Level, by Class and Race) */
	state->skills[SKILL_SEARCH_FREQUENCY] +=
		(cp_ptr->cx_skills[SKILL_SEARCH_FREQUENCY] * p_ptr->lev / 50);
	state->skills[SKILL_SEARCH_FREQUENCY] +=
		(rp_ptr->rx_skills[SKILL_SEARCH_FREQUENCY] * p_ptr->lev / 50);

	/* Affect Skill -- combat (melee) (Level, by Class and Race) */
	state->skills[SKILL_TO_HIT_MELEE] +=
		(cp_ptr->cx_skills[SKILL_TO_HIT_MELEE] * p_ptr->lev / 50);
	state->skills[SKILL_TO_HIT_MELEE] +=
		(rp_ptr->rx_skills[SKILL_TO_HIT_MELEE] * p_ptr->lev / 50);

	/* Affect Skill -- combat (shooting) (Level, by Class and Race) */
	state->skills[SKILL_TO_HIT_BOW] +=
		(cp_ptr->cx_skills[SKILL_TO_HIT_BOW] * p_ptr->lev / 50);
	state->skills[SKILL_TO_HIT_BOW] +=
		(rp_ptr->rx_skills[SKILL_TO_HIT_BOW] * p_ptr->lev / 50);

	/* Affect Skill -- combat (throwing) (Level, by Class and Race) */
	state->skills[SKILL_TO_HIT_THROW] +=
		(cp_ptr->cx_skills[SKILL_TO_HIT_THROW] * p_ptr->lev / 50);
	state->skills[SKILL_TO_HIT_THROW] +=
		(rp_ptr->rx_skills[SKILL_TO_HIT_THROW] * p_ptr->lev / 50);

	/* Limit Skill -- digging from 1 up */
	if (state->skills[SKILL_DIGGING] < 1)
		state->skills[SKILL_DIGGING] = 1;

	/* Limit Skill -- stealth from 0 to 30 */
	if (state->skills[SKILL_STEALTH] > 30)
		state->skills[SKILL_STEALTH] = 30;
	if (state->skills[SKILL_STEALTH] < 0)
		state->skills[SKILL_STEALTH] = 0;

	/* No negative infravision */
	if (state->see_infra < 0)
		state->see_infra = 0;

  /*** Special Saving Throw boosts are calculated after other bonuses ***/

	/* Specialty magic resistance; gives great saving throws even above 100 */
	if (player_has(PF_MAGIC_RESIST)) {
		if (state->skills[SKILL_SAVE] <= 80)
			state->skills[SKILL_SAVE] +=
				(100 - state->skills[SKILL_SAVE]) / 2;
		else
			state->skills[SKILL_SAVE] += 10;
	}

	/* Heightened magical defenses.  Halves the difference between saving throw 
	 * and 100.  */
	if (p_ptr->timed[TMD_INVULN]) {
		if (state->skills[SKILL_SAVE] <= 100)
			state->skills[SKILL_SAVE] +=
				(100 - state->skills[SKILL_SAVE]) / 2;
	}

	/* Rune of Magical Defence */
	if (cave_trap_specific(p_ptr->py, p_ptr->px, RUNE_MAGDEF)) {
		if (state->skills[SKILL_SAVE] <= 100)
			state->skills[SKILL_SAVE] +=
				(100 - state->skills[SKILL_SAVE]) / 2;
	}


  /*** Analyze shapechanges - everything but statistics ***/
	shape_change_main(state);


	/* Obtain the "hold" value */
	hold = adj_str_hold[state->stat_ind[A_STR]];

  /*** Analyze current bow ***/

	/* Examine the "current bow" */
	//o_ptr = &p_ptr->inventory[INVEN_BOW];
	o_ptr = &(inventory[INVEN_BOW]);
	/* Assume not heavy */
	state->heavy_shoot = FALSE;

	/* It is hard to carry a heavy bow */
	if (hold < o_ptr->weight / 10) {
		/* Hard to wield a heavy bow */
		state->to_h += 2 * (hold - o_ptr->weight / 10);
		state->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		/* Heavy Bow */
		state->heavy_shoot = TRUE;
	}

	/* Analyze launcher */
	if (o_ptr->k_idx) {
		/* Get to shoot */
		state->num_fire = 10;

		/* Launcher multiplier is now simply their damage dice. */
		state->ammo_mult = o_ptr->dd;


		/* Analyze the launcher */
		switch (o_ptr->sval) {
			/* Sling and ammo */
		case SV_SLING:
			{
				state->ammo_tval = TV_SHOT;
				break;
			}

			/* Short Bow and Arrow */
		case SV_SHORT_BOW:
			{
				state->ammo_tval = TV_ARROW;
				break;
			}

			/* Long Bow and Arrow */
		case SV_LONG_BOW:
			{
				state->ammo_tval = TV_ARROW;
				break;
			}

			/* Light Crossbow and Bolt */
		case SV_LIGHT_XBOW:
			{
				state->ammo_tval = TV_BOLT;
				break;
			}

			/* Heavy Crossbow and Bolt */
		case SV_HEAVY_XBOW:
			{
				state->ammo_tval = TV_BOLT;
				break;
			}
		}

		/* Apply special flags */
		if (o_ptr->k_idx && !state->heavy_shoot) {
			/* Dex factor for shot speed */
			int dex_factor = (adj_dex_shots[state->stat_ind[A_DEX]]);

			/* Extra shots */
			state->num_fire += extra_shots * 10;

			/* Extra might */
			state->ammo_mult += extra_might;

			/* Love your launcher SJGU bonuses reduced */
			if (((player_has(PF_BOW_SPEED_GREAT))
				 && (state->ammo_tval == TV_ARROW))
				|| ((player_has(PF_SLING_SPEED_GREAT))
					&& (state->ammo_tval == TV_SHOT))
				|| ((player_has(PF_XBOW_SPEED_GREAT))
					&& (state->ammo_tval == TV_BOLT))) {
				/* Big bonus... */
				state->num_fire += 3 * dex_factor / 4;

				/* ...and sometimes even more */
				if (player_has(PF_RAPID_FIRE))
					state->num_fire += dex_factor / 4;
			}

			/* Like your launcher */
			else if (((player_has(PF_BOW_SPEED_GOOD))
					  && (state->ammo_tval == TV_ARROW))
					 || ((player_has(PF_SLING_SPEED_GOOD))
						 && (state->ammo_tval == TV_SHOT))
					 || ((player_has(PF_XBOW_SPEED_GOOD))
						 && (state->ammo_tval == TV_BOLT))) {
				/* Medium bonus */
				state->num_fire += dex_factor / 2;
			}

			/* Minimal bonus */
			else {
				/* Small bonus */
				state->num_fire += dex_factor / 4;
			}


			/* See formula "do_cmd_fire" in "attack.c" for Assassin bonus to
			 * Deadliness. */
		}

		/* Require at least one shot per round in real terms */
		if (state->num_fire < 10)
			state->num_fire = 10;
	}

	/* Add all class and race-specific adjustments to missile Skill. */
	state->skills[SKILL_TO_HIT_BOW] +=
		add_special_missile_skill(state, o_ptr->weight, o_ptr);


  /*** Analyze weapon ***/

	/* Examine the "current weapon" */
	//o_ptr = &p_ptr->inventory[INVEN_WIELD];
	o_ptr = &(inventory[INVEN_WIELD]);

	/* Assume that the player is not a Priest wielding an edged weapon. */
	state->icky_wield = FALSE;

	/* Assume the weapon is not too heavy */
	state->heavy_wield = FALSE;

	/* Inflict heavy weapon penalties. */
	if (hold < o_ptr->weight / 10) {
		/* Hard to wield a heavy weapon */
		state->to_h += 2 * (hold - o_ptr->weight / 10);
		state->dis_to_h += 2 * (hold - o_ptr->weight / 10);

		/* Heavy weapon */
		state->heavy_wield = TRUE;

		/* The player gets to swing a heavy weapon only once. */
		state->num_blow = 1;
	}

	/* Normal weapons */
	if (o_ptr->k_idx && !state->heavy_wield) {
		int str_index, dex_index;

		int effective_weight = 0, mul = 6;

		/* Enforce a minimum weight of three pounds. */
		effective_weight = (o_ptr->weight < 30 ? 30 : o_ptr->weight);


		/* Compare strength and weapon weight. */
		str_index =
			mul * adj_str_blow[state->stat_ind[A_STR]] / effective_weight;

		/* Maximal value */
		if (str_index > 11)
			str_index = 11;


		/* Index by dexterity */
		dex_index = (adj_dex_blow[state->stat_ind[A_DEX]]);

		/* Maximal value */
		if (dex_index > 11)
			dex_index = 11;


		/* Use the blows table */
		state->num_blow = blows_table[str_index][dex_index];

		/* Paranoia - require at least one blow */
		if (state->num_blow < 1)
			state->num_blow = 1;


		/* Boost digging skill by weapon weight */
		state->skills[SKILL_DIGGING] += (o_ptr->weight / 10);
	}

	/* Everyone gets 2 to 4 blows if not wielding a weapon. */
	else if (!o_ptr->k_idx)
		state->num_blow = 2 + (p_ptr->lev / 20);


	/* Add all other class and race-specific adjustments to melee Skill. */
	state->skills[SKILL_TO_HIT_MELEE] +=
		add_special_melee_skill(state, o_ptr->weight, o_ptr);

}

/*
 * Calculate bonuses, and print various things on changes.
 */
static void update_bonuses(void)
{
	int i;

	player_state *state = &p_ptr->state;
	player_state old = p_ptr->state;
	object_type *o_ptr;

	/*** Calculate bonuses ***/

	calc_bonuses(p_ptr->inventory, &p_ptr->state, FALSE);


	/*** Notice changes ***/

	/* Analyze stats */
	for (i = 0; i < A_MAX; i++) {
		/* Notice changes */
		if (state->stat_top[i] != old.stat_top[i]) {
			/* Redisplay the stats later */
			p_ptr->redraw |= (PR_STATS);
		}

		/* Notice changes */
		if (state->stat_use[i] != old.stat_use[i]) {
			/* Redisplay the stats later */
			p_ptr->redraw |= (PR_STATS);
		}

		/* Notice changes */
		if (state->stat_ind[i] != old.stat_ind[i]) {
			/* Change in STR may affect how shields are used. */
			if ((i == A_STR) && (p_ptr->inventory[INVEN_ARM].k_idx)) {
				/* Access the wield slot */
				o_ptr = &p_ptr->inventory[INVEN_WIELD];

				/* Analyze weapon for two-handed-use. */
				if (of_has(o_ptr->flags_obj, OF_TWO_HANDED_REQ)
					|| (of_has(o_ptr->flags_obj, OF_TWO_HANDED_DES)
						&& (state->stat_ind[A_STR] <
							29 + (o_ptr->weight / 50 >
								  8 ? 8 : o_ptr->weight / 50)))) {
					state->shield_on_back = TRUE;

				} else
					state->shield_on_back = FALSE;

				/* Hack - recalculate bonuses again. */
				if (old.shield_on_back != state->shield_on_back) {
					/* do not check strength again */
					old.stat_ind[i] = state->stat_ind[i];
					calc_bonuses(p_ptr->inventory, &p_ptr->state, FALSE);
				}
			}

			/* Change in CON affects Hitpoints */
			if (i == A_CON) {
				p_ptr->update |= (PU_HP);
			}

			/* Change in INT may affect Mana/Spells */
			else if (i == A_INT) {
				if (mp_ptr->spell_stat == A_INT) {
					p_ptr->update |= (PU_MANA | PU_SPELLS);
				}
			}

			/* Change in WIS may affect Mana/Spells */
			else if (i == A_WIS) {
				if (mp_ptr->spell_stat == A_WIS) {
					p_ptr->update |= (PU_MANA | PU_SPELLS);
				}
			}
		}
	}

	/* Hack -- Telepathy Change */
	if (state->telepathy != old.telepathy) {
		/* Update monster visibility */
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Hack -- See Invis Change */
	if (state->see_inv != old.see_inv) {
		/* Update monster visibility */
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Hack -- See Invis Change */
	if (state->see_infra != old.see_infra) {
		/* Update monster visibility */
		p_ptr->update |= (PU_MONSTERS);
	}

	/* Redraw speed (if needed) */
	if (state->pspeed != old.pspeed) {
		/* Redraw speed */
		p_ptr->redraw |= (PR_SPEED);
	}

	/* Recalculate stealth when needed */
	if (state->skills[SKILL_STEALTH] != old.skills[SKILL_STEALTH]) {
		/* Assume character is extremely noisy. */
		state->base_wakeup_chance = 100 * WAKEUP_ADJ;

		/* For every increase in stealth past 0, multiply wakeup chance by 0.8 */
		for (i = 0; i < state->skills[SKILL_STEALTH]; i++) {
			state->base_wakeup_chance = 4 * state->base_wakeup_chance / 5;

			/* Always make at least some innate noise */
			if (state->base_wakeup_chance < 100) {
				state->base_wakeup_chance = 100;
				break;
			}
		}
	}

	/* Big Hack - make sure base_wakeup_chance has ever been set */
	if (state->base_wakeup_chance == 0)
		state->base_wakeup_chance = 100 * WAKEUP_ADJ;

	/* Redraw armor (if needed) */
	if ((state->dis_ac != old.dis_ac)
		|| (state->dis_to_a != old.dis_to_a)) {
		/* Redraw */
		p_ptr->redraw |= (PR_ARMOR);
	}

	/* Hack -- handle "xtra" mode */
	if (character_xtra)
		return;

	/* Take note when player moves his shield on and off his back. */
	if (state->evasion_chance != old.evasion_chance) {
		/* Messages */
		if (!old.evasion_chance) {
			msg("You are able to Evade attacks.");
		} else if (!state->evasion_chance) {
			msg("You are no longer able to Evade attacks");
		}
		/* Mega-Hack - Mask out small changes */
		else if (state->evasion_chance > (old.evasion_chance + 5)) {
			msg("You are better able to Evade attacks.");
		} else if ((state->evasion_chance + 5) < old.evasion_chance) {
			msg("You are less able to Evade attacks.");
		}

		/* Save it */
		old.evasion_chance = state->evasion_chance;
	}

	/* Take note when "heavy bow" changes */
	if (old.heavy_shoot != state->heavy_shoot) {
		/* Message */
		if (state->heavy_shoot) {
			msg("You have trouble wielding such a heavy bow.");
		} else if (p_ptr->inventory[INVEN_BOW].k_idx) {
			msg("You have no trouble wielding your bow.");
		} else {
			msg("You feel relieved to put down your heavy bow.");
		}

		/* Save it */
		old.heavy_shoot = state->heavy_shoot;
	}

	/* Take note when "heavy weapon" changes */
	if (old.heavy_wield != state->heavy_wield) {
		/* Message */
		if (state->heavy_wield) {
			msg("You have trouble wielding such a heavy weapon.");
		} else if (p_ptr->inventory[INVEN_WIELD].k_idx) {
			msg("You have no trouble wielding your weapon.");
		} else {
			msg("You feel relieved to put down your heavy weapon.");
		}

		/* Save it */
		old.heavy_wield = state->heavy_wield;
	}

	/* Take note when "illegal weapon" changes */
	if (old.icky_wield != state->icky_wield) {
		/* Message */
		if (state->icky_wield) {
			msg("You do not feel comfortable with your weapon.");
		} else if (p_ptr->inventory[INVEN_WIELD].k_idx) {
			notice_obj(OF_BLESSED, INVEN_WIELD + 1);
			msg("You feel comfortable with your weapon.");
		} else {
			msg("You feel more comfortable after removing your weapon.");
		}

		/* Save it */
		old.icky_wield = state->icky_wield;
	}

	/* Take note when player moves his shield on and off his back. */
	if (old.shield_on_back != state->shield_on_back) {
		/* Messages */
		if (state->shield_on_back) {
			msg("You are carrying your shield on your back.");
		} else if (p_ptr->inventory[INVEN_ARM].k_idx) {
			msg("You are carrying your shield in your hand.");
		}

		/* No message for players no longer carrying a shield. */

		/* Save it */
		old.shield_on_back = state->shield_on_back;
	}

	/* Hack - force redraw if stuff has changed */
	if (p_ptr->redraw)
		p_ptr->update |= (PU_BONUS);
}



/**
 * Handle "p_ptr->notice"
 */
void notice_stuff(struct player *p)
{
	/* Notice stuff */
	if (!p_ptr->notice)
		return;


	/* Deal with autoinscribe stuff */
	if (p_ptr->notice & PN_AUTOINSCRIBE) {
		p_ptr->notice &= ~(PN_AUTOINSCRIBE);
		autoinscribe_pack();
		autoinscribe_ground();
	}

	/* Deal with squelch stuff */
	if (p_ptr->notice & PN_SQUELCH) {
		p_ptr->notice &= ~(PN_SQUELCH);
		if (OPT(hide_squelchable))
			squelch_drop();
	}

	/* Combine the pack */
	if (p_ptr->notice & (PN_COMBINE)) {
		p_ptr->notice &= ~(PN_COMBINE);
		combine_pack();
	}

	/* Reorder the pack */
	if (p_ptr->notice & (PN_REORDER)) {
		p_ptr->notice &= ~(PN_REORDER);
		reorder_pack();
	}

	/* Sort the quiver */
	if (p_ptr->notice & PN_SORT_QUIVER) {
		p_ptr->notice &= ~(PN_SORT_QUIVER);
		sort_quiver();
	}
}


/**
 * Handle "p_ptr->update"
 */
void update_stuff(struct player *p)
{
	/* Update stuff */
	if (!p_ptr->update)
		return;


	if (p_ptr->update & (PU_BONUS)) {
		p_ptr->update &= ~(PU_BONUS);
		update_bonuses();
	}

	if (p_ptr->update & (PU_TORCH)) {
		p_ptr->update &= ~(PU_TORCH);
		calc_torch();
	}

	if (p_ptr->update & (PU_HP)) {
		p_ptr->update &= ~(PU_HP);
		calc_hitpoints();
	}

	if (p_ptr->update & (PU_MANA)) {
		p_ptr->update &= ~(PU_MANA);
		calc_mana();
	}

	if (p_ptr->update & (PU_SPELLS)) {
		p_ptr->update &= ~(PU_SPELLS);
		calc_spells();
	}

	if (p_ptr->update & (PU_SPECIALTY)) {
		p_ptr->update &= ~(PU_SPECIALTY);
		calc_specialty();
	}


	/* Character is not ready yet, no screen updates */
	if (!character_generated)
		return;


	/* Character is in "icky" mode, no screen updates */
	if (character_icky)
		return;


	if (p_ptr->update & (PU_FORGET_VIEW)) {
		p_ptr->update &= ~(PU_FORGET_VIEW);
		forget_view();
	}

	if (p_ptr->update & (PU_UPDATE_VIEW)) {
		p_ptr->update &= ~(PU_UPDATE_VIEW);
		update_view();
	}

	if (p_ptr->update & (PU_DISTANCE)) {
		p_ptr->update &= ~(PU_DISTANCE);
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(TRUE);
	}

	if (p_ptr->update & (PU_MONSTERS)) {
		p_ptr->update &= ~(PU_MONSTERS);
		update_monsters(FALSE);
	}


	if (p_ptr->update & (PU_PANEL)) {
		p_ptr->update &= ~(PU_PANEL);
		event_signal(EVENT_PLAYERMOVED);
	}
}


struct flag_event_trigger {
	u32b flag;
	game_event_type event;
};


/*
 * Events triggered by the various flags.
 */
static const struct flag_event_trigger redraw_events[] = {
	{PR_MISC, EVENT_RACE_CLASS},
	{PR_TITLE, EVENT_PLAYERTITLE},
	{PR_LEV, EVENT_PLAYERLEVEL},
	{PR_EXP, EVENT_EXPERIENCE},
	{PR_STATS, EVENT_STATS},
	{PR_ARMOR, EVENT_AC},
	{PR_HP, EVENT_HP},
	{PR_MANA, EVENT_MANA},
	{PR_GOLD, EVENT_GOLD},
	{PR_HEALTH, EVENT_MONSTERHEALTH},
	{PR_MON_MANA, EVENT_MONSTERMANA},
	{PR_DEPTH, EVENT_DUNGEONLEVEL},
	{PR_SPEED, EVENT_PLAYERSPEED},
	{PR_SHAPE, EVENT_SHAPECHANGE},
	{PR_STATE, EVENT_STATE},
	{PR_STATUS, EVENT_STATUS},
	{PR_STUDY, EVENT_STUDYSTATUS},
	{PR_DTRAP, EVENT_DETECTIONSTATUS},

	{PR_INVEN, EVENT_INVENTORY},
	{PR_EQUIP, EVENT_EQUIPMENT},
	{PR_MONLIST, EVENT_MONSTERLIST},
	{PR_ITEMLIST, EVENT_ITEMLIST},
	{PR_MONSTER, EVENT_MONSTERTARGET},
	{PR_OBJECT, EVENT_OBJECTTARGET},
	{PR_MESSAGE, EVENT_MESSAGE},
};

/**
 * Handle "p_ptr->redraw"
 */
void redraw_stuff(struct player *p)
{
	size_t i;

	/* Redraw stuff */
	if (!p_ptr->redraw)
		return;

	/* Character is not ready yet, no screen updates */
	if (!character_generated)
		return;

	/* Character is in "icky" mode, no screen updates */
	if (character_icky)
		return;

	/* For each listed flag, send the appropriate signal to the UI */
	for (i = 0; i < N_ELEMENTS(redraw_events); i++) {
		const struct flag_event_trigger *hnd = &redraw_events[i];

		if (p_ptr->redraw & hnd->flag)
			event_signal(hnd->event);
	}

	/* Then the ones that require parameters to be supplied. */
	if (p_ptr->redraw & PR_MAP) {
		/* Mark the whole map to be redrawn */
		event_signal_point(EVENT_MAP, -1, -1);
	}

	p_ptr->redraw = 0;

	/*
	 * Do any plotting, etc. delayed from earlier - this set of updates
	 * is over.
	 */
	event_signal(EVENT_END);
}



/**
 * Handle "p_ptr->update" and "p_ptr->redraw"
 */
void handle_stuff(struct player *p)
{
	/* Update stuff */
	if (p_ptr->update)
		update_stuff(p);

	/* Redraw stuff */
	if (p_ptr->redraw)
		redraw_stuff(p);
}
