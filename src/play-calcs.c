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
#include "squelch.h"


/**
 * Stat Table (INT/WIS) -- Number of half-spells per level
 */
byte adj_mag_study[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    1	/* 7 */,
    1	/* 8 */,
    1	/* 9 */,
    1	/* 10 */,
    1	/* 11 */,
    2	/* 12 */,
    2	/* 13 */,
    2	/* 14 */,
    2	/* 15 */,
    2	/* 16 */,
    2	/* 17 */,
    2	/* 18/00-18/09 */,
    2	/* 18/10-18/19 */,
    2	/* 18/20-18/29 */,
    2	/* 18/30-18/39 */,
    2	/* 18/40-18/49 */,
    3	/* 18/50-18/59 */,
    3	/* 18/60-18/69 */,
    3	/* 18/70-18/79 */,
    3	/* 18/80-18/89 */,
    3	/* 18/90-18/99 */,
    3	/* 18/100-18/109 */,
    3	/* 18/110-18/119 */,
    3	/* 18/120-18/129 */,
    3	/* 18/130-18/139 */,
    3	/* 18/140-18/149 */,
    3	/* 18/150-18/159 */,
    3	/* 18/160-18/169 */,
    3	/* 18/170-18/179 */,
    3	/* 18/180-18/189 */,
    3	/* 18/190-18/199 */,
    3	/* 18/200-18/209 */,
    3	/* 18/210-18/219 */,
    3	/* 18/220+ */
  };


/**
 * Stat Table (INT/WIS) -- extra tenth-mana-points per level.
 */
byte adj_mag_mana[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    5	/* 7 */,
    6	/* 8 */,
    7	/* 9 */,
    8	/* 10 */,
    9	/* 11 */,
    9	/* 12 */,
    10	/* 13 */,
    10	/* 14 */,
    10	/* 15 */,
    11	/* 16 */,
    11	/* 17 */,
    12	/* 18/00-18/09 */,
    13	/* 18/10-18/19 */,
    14	/* 18/20-18/29 */,
    15	/* 18/30-18/39 */,
    17	/* 18/40-18/49 */,
    20	/* 18/50-18/59 */,
    22	/* 18/60-18/69 */,
    25	/* 18/70-18/79 */,
    30	/* 18/80-18/89 */,
    35	/* 18/90-18/99 */,
    40	/* 18/100-18/109 */,
    45	/* 18/110-18/119 */,
    50	/* 18/120-18/129 */,
    55	/* 18/130-18/139 */,
    60	/* 18/140-18/149 */,
    65	/* 18/150-18/159 */,
    70	/* 18/160-18/169 */,
    74	/* 18/170-18/179 */,
    77	/* 18/180-18/189 */,
    79	/* 18/190-18/199 */,
    81	/* 18/200-18/209 */,
    83	/* 18/210-18/219 */,
    85	/* 18/220+ */
  };


/**
 * Stat Table (INT) -- Magic devices
 */
byte adj_int_dev[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    1	/* 8 */,
    1	/* 9 */,
    1	/* 10 */,
    1	/* 11 */,
    1	/* 12 */,
    1	/* 13 */,
    1	/* 14 */,
    2	/* 15 */,
    2	/* 16 */,
    2	/* 17 */,
    3	/* 18/00-18/09 */,
    3	/* 18/10-18/19 */,
    4	/* 18/20-18/29 */,
    4	/* 18/30-18/39 */,
    5	/* 18/40-18/49 */,
    5	/* 18/50-18/59 */,
    6	/* 18/60-18/69 */,
    6	/* 18/70-18/79 */,
    7	/* 18/80-18/89 */,
    7	/* 18/90-18/99 */,
    8	/* 18/100-18/109 */,
    9	/* 18/110-18/119 */,
    10	/* 18/120-18/129 */,
    11	/* 18/130-18/139 */,
    12	/* 18/140-18/149 */,
    13	/* 18/150-18/159 */,
    14	/* 18/160-18/169 */,
    15	/* 18/170-18/179 */,
    16	/* 18/180-18/189 */,
    17	/* 18/190-18/199 */,
    18	/* 18/200-18/209 */,
    19	/* 18/210-18/219 */,
    20	/* 18/220+ */
  };


/**
 * Stat Table (WIS) -- Saving throw
 */
byte adj_wis_sav[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    1	/* 8 */,
    1	/* 9 */,
    1	/* 10 */,
    1	/* 11 */,
    1	/* 12 */,
    1	/* 13 */,
    1	/* 14 */,
    2	/* 15 */,
    2	/* 16 */,
    2	/* 17 */,
    3	/* 18/00-18/09 */,
    3	/* 18/10-18/19 */,
    3	/* 18/20-18/29 */,
    3	/* 18/30-18/39 */,
    3	/* 18/40-18/49 */,
    4	/* 18/50-18/59 */,
    4	/* 18/60-18/69 */,
    5	/* 18/70-18/79 */,
    5	/* 18/80-18/89 */,
    6	/* 18/90-18/99 */,
    7	/* 18/100-18/109 */,
    8	/* 18/110-18/119 */,
    9	/* 18/120-18/129 */,
    10	/* 18/130-18/139 */,
    11	/* 18/140-18/149 */,
    12	/* 18/150-18/159 */,
    13	/* 18/160-18/169 */,
    14	/* 18/170-18/179 */,
    15	/* 18/180-18/189 */,
    16	/* 18/190-18/199 */,
    17	/* 18/200-18/209 */,
    18	/* 18/210-18/219 */,
    19	/* 18/220+ */
  };


/**
 * Stat Table (DEX) -- disarming (also getting out of pits)
 */
byte adj_dex_dis[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    0	/* 8 */,
    0	/* 9 */,
    0	/* 10 */,
    0	/* 11 */,
    0	/* 12 */,
    1	/* 13 */,
    1	/* 14 */,
    1	/* 15 */,
    2	/* 16 */,
    2	/* 17 */,
    4	/* 18/00-18/09 */,
    4	/* 18/10-18/19 */,
    4	/* 18/20-18/29 */,
    4	/* 18/30-18/39 */,
    5	/* 18/40-18/49 */,
    5	/* 18/50-18/59 */,
    5	/* 18/60-18/69 */,
    6	/* 18/70-18/79 */,
    6	/* 18/80-18/89 */,
    7	/* 18/90-18/99 */,
    8	/* 18/100-18/109 */,
    8	/* 18/110-18/119 */,
    8	/* 18/120-18/129 */,
    8	/* 18/130-18/139 */,
    8	/* 18/140-18/149 */,
    9	/* 18/150-18/159 */,
    9	/* 18/160-18/169 */,
    9	/* 18/170-18/179 */,
    9	/* 18/180-18/189 */,
    9	/* 18/190-18/199 */,
    10	/* 18/200-18/209 */,
    10	/* 18/210-18/219 */,
    10	/* 18/220+ */
  };


/**
 * Stat Table (INT) -- disarming
 */
byte adj_int_dis[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    1	/* 8 */,
    1	/* 9 */,
    1	/* 10 */,
    1	/* 11 */,
    1	/* 12 */,
    1	/* 13 */,
    1	/* 14 */,
    2	/* 15 */,
    2	/* 16 */,
    2	/* 17 */,
    3	/* 18/00-18/09 */,
    3	/* 18/10-18/19 */,
    3	/* 18/20-18/29 */,
    4	/* 18/30-18/39 */,
    4	/* 18/40-18/49 */,
    5	/* 18/50-18/59 */,
    6	/* 18/60-18/69 */,
    7	/* 18/70-18/79 */,
    8	/* 18/80-18/89 */,
    9	/* 18/90-18/99 */,
    10	/* 18/100-18/109 */,
    10	/* 18/110-18/119 */,
    11	/* 18/120-18/129 */,
    12	/* 18/130-18/139 */,
    13	/* 18/140-18/149 */,
    14	/* 18/150-18/159 */,
    15	/* 18/160-18/169 */,
    16	/* 18/170-18/179 */,
    17	/* 18/180-18/189 */,
    18	/* 18/190-18/199 */,
    19	/* 18/200-18/209 */,
    19	/* 18/210-18/219 */,
    19	/* 18/220+ */
  };


/**
 * Stat Table (DEX) -- bonus to ac (plus 128)
 */
byte adj_dex_ta[STAT_RANGE] =
  {
    128 + -4	/* 3 */,
    128 + -3	/* 4 */,
    128 + -2	/* 5 */,
    128 + -1	/* 6 */,
    128 + 0	/* 7 */,
    128 + 0	/* 8 */,
    128 + 0	/* 9 */,
    128 + 0	/* 10 */,
    128 + 0	/* 11 */,
    128 + 0	/* 12 */,
    128 + 0	/* 13 */,
    128 + 0	/* 14 */,
    128 + 1	/* 15 */,
    128 + 1	/* 16 */,
    128 + 1	/* 17 */,
    128 + 2	/* 18/00-18/09 */,
    128 + 2	/* 18/10-18/19 */,
    128 + 2	/* 18/20-18/29 */,
    128 + 2	/* 18/30-18/39 */,
    128 + 2	/* 18/40-18/49 */,
    128 + 3	/* 18/50-18/59 */,
    128 + 3	/* 18/60-18/69 */,
    128 + 3	/* 18/70-18/79 */,
    128 + 4	/* 18/80-18/89 */,
    128 + 5	/* 18/90-18/99 */,
    128 + 6	/* 18/100-18/109 */,
    128 + 7	/* 18/110-18/119 */,
    128 + 8	/* 18/120-18/129 */,
    128 + 9	/* 18/130-18/139 */,
    128 + 9	/* 18/140-18/149 */,
    128 + 10	/* 18/150-18/159 */,
    128 + 11	/* 18/160-18/169 */,
    128 + 12	/* 18/170-18/179 */,
    128 + 13	/* 18/180-18/189 */,
    128 + 14	/* 18/190-18/199 */,
    128 + 15	/* 18/200-18/209 */,
    128 + 15	/* 18/210-18/219 */,
    128 + 15	/* 18/220+ */
  };


/**
 * Stat Table (STR) -- bonus to Deadliness (plus 128).  To compensate
 * for changes elsewhere, STR now has a larger effect. -LM-
 */
byte adj_str_td[STAT_RANGE] =
  {
    128 + -2	/* 3 */,
    128 + -2	/* 4 */,
    128 + -1	/* 5 */,
    128 + -1	/* 6 */,
    128 + 0	/* 7 */,
    128 + 0	/* 8 */,
    128 + 0	/* 9 */,
    128 + 0	/* 10 */,
    128 + 0	/* 11 */,
    128 + 0	/* 12 */,
    128 + 0	/* 13 */,
    128 + 0	/* 14 */,
    128 + 0	/* 15 */,
    128 + 1	/* 16 */,
    128 + 2	/* 17 */,
    128 + 3	/* 18/00-18/09 */,
    128 + 4	/* 18/10-18/19 */,
    128 + 4	/* 18/20-18/29 */,
    128 + 5	/* 18/30-18/39 */,
    128 + 6	/* 18/40-18/49 */,
    128 + 7	/* 18/50-18/59 */,
    128 + 8	/* 18/60-18/69 */,
    128 + 9	/* 18/70-18/79 */,
    128 + 10	/* 18/80-18/89 */,
    128 + 11	/* 18/90-18/99 */,
    128 + 12	/* 18/100-18/109 */,
    128 + 13	/* 18/110-18/119 */,
    128 + 14	/* 18/120-18/129 */,
    128 + 15	/* 18/130-18/139 */,
    128 + 16	/* 18/140-18/149 */,
    128 + 17	/* 18/150-18/159 */,
    128 + 18	/* 18/160-18/169 */,
    128 + 19	/* 18/170-18/179 */,
    128 + 20	/* 18/180-18/189 */,
    128 + 21	/* 18/190-18/199 */,
    128 + 22	/* 18/200-18/209 */,
    128 + 23	/* 18/210-18/219 */,
    128 + 25	/* 18/220+ */
  };


/**
 * Stat Table (DEX) -- bonus to Skill (plus 128.  To compensate for
 * changes elsewhere, DEX now has a larger effect. -LM-
 */
byte adj_dex_th[STAT_RANGE] =
  {
    128 + -4	/* 3 */,
    128 + -3	/* 4 */,
    128 + -2	/* 5 */,
    128 + -1	/* 6 */,
    128 + -1	/* 7 */,
    128 + 0	/* 8 */,
    128 + 0	/* 9 */,
    128 + 0	/* 10 */,
    128 + 0	/* 11 */,
    128 + 0	/* 12 */,
    128 + 0	/* 13 */,
    128 + 0	/* 14 */,
    128 + 1	/* 15 */,
    128 + 2	/* 16 */,
    128 + 3	/* 17 */,
    128 + 3	/* 18/00-18/09 */,
    128 + 3	/* 18/10-18/19 */,
    128 + 4	/* 18/20-18/29 */,
    128 + 4	/* 18/30-18/39 */,
    128 + 4	/* 18/40-18/49 */,
    128 + 5	/* 18/50-18/59 */,
    128 + 5	/* 18/60-18/69 */,
    128 + 6	/* 18/70-18/79 */,
    128 + 6	/* 18/80-18/89 */,
    128 + 7	/* 18/90-18/99 */,
    128 + 8	/* 18/100-18/109 */,
    128 + 9	/* 18/110-18/119 */,
    128 + 10	/* 18/120-18/129 */,
    128 + 11	/* 18/130-18/139 */,
    128 + 12	/* 18/140-18/149 */,
    128 + 13	/* 18/150-18/159 */,
    128 + 14	/* 18/160-18/169 */,
    128 + 15	/* 18/170-18/179 */,
    128 + 16	/* 18/180-18/189 */,
    128 + 17	/* 18/190-18/199 */,
    128 + 18	/* 18/200-18/209 */,
    128 + 19	/* 18/210-18/219 */,
    128 + 20	/* 18/220+ */
  };


/**
 * Stat Table (STR) -- weight limit in deca-pounds
 */
byte adj_str_wgt[STAT_RANGE] =
  {
    6	/* 3 */,
    7	/* 4 */,
    8	/* 5 */,
    9	/* 6 */,
    10	/* 7 */,
    11	/* 8 */,
    12	/* 9 */,
    13	/* 10 */,
    14	/* 11 */,
    15	/* 12 */,
    15	/* 13 */,
    16	/* 14 */,
    17	/* 15 */,
    18	/* 16 */,
    19	/* 17 */,
    20	/* 18/00-18/09 */,
    22	/* 18/10-18/19 */,
    24	/* 18/20-18/29 */,
    26	/* 18/30-18/39 */,
    28	/* 18/40-18/49 */,
    30	/* 18/50-18/59 */,
    30	/* 18/60-18/69 */,
    30	/* 18/70-18/79 */,
    30	/* 18/80-18/89 */,
    30	/* 18/90-18/99 */,
    30	/* 18/100-18/109 */,
    30	/* 18/110-18/119 */,
    30	/* 18/120-18/129 */,
    30	/* 18/130-18/139 */,
    30	/* 18/140-18/149 */,
    30	/* 18/150-18/159 */,
    30	/* 18/160-18/169 */,
    30	/* 18/170-18/179 */,
    30	/* 18/180-18/189 */,
    30	/* 18/190-18/199 */,
    30	/* 18/200-18/209 */,
    30	/* 18/210-18/219 */,
    30	/* 18/220+ */
  };


/**
 * Stat Table (STR) -- weapon weight limit in pounds
 */
byte adj_str_hold[STAT_RANGE] =
  {
    4	/* 3 */,
    5	/* 4 */,
    6	/* 5 */,
    7	/* 6 */,
    8	/* 7 */,
    10	/* 8 */,
    12	/* 9 */,
    14	/* 10 */,
    16	/* 11 */,
    18	/* 12 */,
    20	/* 13 */,
    22	/* 14 */,
    24	/* 15 */,
    26	/* 16 */,
    28	/* 17 */,
    30	/* 18/00-18/09 */,
    30	/* 18/10-18/19 */,
    35	/* 18/20-18/29 */,
    40	/* 18/30-18/39 */,
    45	/* 18/40-18/49 */,
    50	/* 18/50-18/59 */,
    55	/* 18/60-18/69 */,
    60	/* 18/70-18/79 */,
    65	/* 18/80-18/89 */,
    70	/* 18/90-18/99 */,
    80	/* 18/100-18/109 */,
    80	/* 18/110-18/119 */,
    80	/* 18/120-18/129 */,
    80	/* 18/130-18/139 */,
    80	/* 18/140-18/149 */,
    90	/* 18/150-18/159 */,
    90	/* 18/160-18/169 */,
    90	/* 18/170-18/179 */,
    90	/* 18/180-18/189 */,
    90	/* 18/190-18/199 */,
    100	/* 18/200-18/209 */,
    100	/* 18/210-18/219 */,
    100	/* 18/220+ */
  };


/**
 * Stat Table (STR) -- digging value
 */
byte adj_str_dig[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    1	/* 5 */,
    2	/* 6 */,
    3	/* 7 */,
    4	/* 8 */,
    4	/* 9 */,
    5	/* 10 */,
    5	/* 11 */,
    6	/* 12 */,
    6	/* 13 */,
    7	/* 14 */,
    7	/* 15 */,
    8	/* 16 */,
    8	/* 17 */,
    9	/* 18/00-18/09 */,
    10	/* 18/10-18/19 */,
    12	/* 18/20-18/29 */,
    15	/* 18/30-18/39 */,
    20	/* 18/40-18/49 */,
    25	/* 18/50-18/59 */,
    30	/* 18/60-18/69 */,
    35	/* 18/70-18/79 */,
    40	/* 18/80-18/89 */,
    45	/* 18/90-18/99 */,
    50	/* 18/100-18/109 */,
    55	/* 18/110-18/119 */,
    60	/* 18/120-18/129 */,
    65	/* 18/130-18/139 */,
    70	/* 18/140-18/149 */,
    75	/* 18/150-18/159 */,
    80	/* 18/160-18/169 */,
    85	/* 18/170-18/179 */,
    90	/* 18/180-18/189 */,
    95	/* 18/190-18/199 */,
    100	/* 18/200-18/209 */,
    100	/* 18/210-18/219 */,
    100	/* 18/220+ */
  };


/**
 * Stat Table (STR) -- help index into the "blow" table
 */
byte adj_str_blow[STAT_RANGE] =
  {
    3	/* 3 */,
    4	/* 4 */,
    5	/* 5 */,
    6	/* 6 */,
    7	/* 7 */,
    8	/* 8 */,
    9	/* 9 */,
    10	/* 10 */,
    11	/* 11 */,
    12	/* 12 */,
    13	/* 13 */,
    14	/* 14 */,
    15	/* 15 */,
    16	/* 16 */,
    17	/* 17 */,
    20 /* 18/00-18/09 */,
    30 /* 18/10-18/19 */,
    40 /* 18/20-18/29 */,
    50 /* 18/30-18/39 */,
    60 /* 18/40-18/49 */,
    70 /* 18/50-18/59 */,
    80 /* 18/60-18/69 */,
    90 /* 18/70-18/79 */,
    100 /* 18/80-18/89 */,
    110 /* 18/90-18/99 */,
    120 /* 18/100-18/109 */,
    130 /* 18/110-18/119 */,
    140 /* 18/120-18/129 */,
    150 /* 18/130-18/139 */,
    160 /* 18/140-18/149 */,
    170 /* 18/150-18/159 */,
    180 /* 18/160-18/169 */,
    190 /* 18/170-18/179 */,
    200 /* 18/180-18/189 */,
    210 /* 18/190-18/199 */,
    220 /* 18/200-18/209 */,
    230 /* 18/210-18/219 */,
    240 /* 18/220+ */
  };

/**
 * Stat Table (DEX) -- index into the "blow" table
 */
byte adj_dex_blow[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    0	/* 8 */,
    0	/* 9 */,
    1	/* 10 */,
    1	/* 11 */,
    1	/* 12 */,
    1	/* 13 */,
    1	/* 14 */,
    1	/* 15 */,
    1	/* 16 */,
    1	/* 17 */,
    1	/* 18/00-18/09 */,
    2	/* 18/10-18/19 */,
    2	/* 18/20-18/29 */,
    2	/* 18/30-18/39 */,
    3	/* 18/40-18/49 */,
    3	/* 18/50-18/59 */,
    4	/* 18/60-18/69 */,
    4	/* 18/70-18/79 */,
    5	/* 18/80-18/89 */,
    5	/* 18/90-18/99 */,
    6	/* 18/100-18/109 */,
    7	/* 18/110-18/119 */,
    8	/* 18/120-18/129 */,
    9	/* 18/130-18/139 */,
    9	/* 18/140-18/149 */,
    10	/* 18/150-18/159 */,
    10	/* 18/160-18/169 */,
    11	/* 18/170-18/179 */,
    11	/* 18/180-18/189 */,
    11	/* 18/190-18/199 */,
    11	/* 18/200-18/209 */,
    11	/* 18/210-18/219 */,
    11	/* 18/220+ */
  };


/**
 * Stat Table (DEX) -- Used for number of shots per round
 */
byte adj_dex_shots[STAT_RANGE] =
  {
    0	/* 3 */,
    0	/* 4 */,
    0	/* 5 */,
    0	/* 6 */,
    0	/* 7 */,
    0	/* 8 */,
    0	/* 9 */,
    0	/* 10 */,
    0	/* 11 */,
    0	/* 12 */,
    0	/* 13 */,
    0	/* 14 */,
    0	/* 15 */,
    0	/* 16 */,
    0	/* 17 */,
    1	/* 18/00-18/09 */,
    1	/* 18/10-18/19 */,
    2	/* 18/20-18/29 */,
    3	/* 18/30-18/39 */,
    4	/* 18/40-18/49 */,
    5	/* 18/50-18/59 */,
    6	/* 18/60-18/69 */,
    7	/* 18/70-18/79 */,
    8	/* 18/80-18/89 */,
    9	/* 18/90-18/99 */,
    10	/* 18/100-18/109 */,
    11	/* 18/110-18/119 */,
    12	/* 18/120-18/129 */,
    13	/* 18/130-18/139 */,
    14	/* 18/140-18/149 */,
    15	/* 18/150-18/159 */,
    16	/* 18/160-18/169 */,
    17	/* 18/170-18/179 */,
    18	/* 18/180-18/189 */,
    19	/* 18/190-18/199 */,
    20	/* 18/200-18/209 */,
    20	/* 18/210-18/219 */,
    20	/* 18/220+ */
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
byte blows_table[12][12] =
{
    /*  <- Dexterity factor -> */
    /* 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11+ */
  
    {  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3 }, /*  0         */
    {  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3 }, /*  1    ^    */
    {  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4 }, /*  2    |    */
    {  2,  2,  2,  2,  3,  3,  3,  3,  3,  4,  4,  4 }, /*  3         */
    {  2,  2,  2,  3,  3,  3,  3,  3,  4,  4,  4,  4 }, /*  4  Ratio  */
    {  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4 }, /*  5  of STR */
    {  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  5 }, /*  6  over   */
    {  2,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5 }, /*  7  weight */
    {  2,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5 }, /*  8         */
    {  2,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5 }, /*  9    |    */
    {  2,  3,  3,  3,  4,  4,  4,  5,  5,  5,  5,  6 }, /* 10    V    */
    {  2,  3,  3,  4,  4,  4,  5,  5,  5,  6,  6,  6 }  /* 11+        */
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

    magic_type *s_ptr;

    cptr p = "";


    /* Hack -- must be literate */
    if (!mp_ptr->spell_book)
	return;

    /* Hack -- wait for creation */
    if (!character_generated)
	return;

    /* Hack -- handle "xtra" mode */
    if (character_xtra)
	return;

    /* Determine magic description. */
    if (mp_ptr->spell_book == TV_MAGIC_BOOK)
	p = "spell";
    if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	p = "prayer";
    if (mp_ptr->spell_book == TV_DRUID_BOOK)
	p = "druidic lore";
    if (mp_ptr->spell_book == TV_NECRO_BOOK)
	p = "ritual";

    /* Determine the number of spells allowed */
    levels = p_ptr->lev - mp_ptr->spell_first + 1;

    /* Hack -- no negative spells */
    if (levels < 0)
	levels = 0;


    /* Extract total allowed spells */
    num_allowed =
	(adj_mag_study[p_ptr->stat_ind[mp_ptr->spell_stat]] * levels / 2);

    /* Boundary control. */
    if (num_allowed > mp_ptr->spell_number)
	num_allowed = mp_ptr->spell_number;


    /* Assume none known */
    num_known = 0;

    /* Count the number of spells we know */
    for (j = 0; j < mp_ptr->spell_number; j++) {
	/* Count known spells */
	if ((j <  32) ? (p_ptr->spell_learned1 & (1L << j)) : 
	    (p_ptr->spell_learned2 & (1L << (j - 32))))
	{
	    num_known++;
	}
    }

    /* See how many spells we must forget or may learn */
    p_ptr->new_spells = num_allowed - num_known;



    /* Forget spells which are too hard */
    for (i = 63; i >= 0; i--) {
	/* Efficiency -- all done */
	if (!p_ptr->spell_learned1 && !p_ptr->spell_learned2)
	    break;

	/* Access the spell */
	j = p_ptr->spell_order[i];

	/* Skip non-spells */
	if (j >= 99)
	    continue;

	/* Get the spell */
	s_ptr = &mp_ptr->info[j];

	/* Skip spells we are allowed to know */
	if (s_ptr->slevel <= p_ptr->lev)
	    continue;

	/* Is it known? */
	if ((j < 32) ? (p_ptr->spell_learned1 & (1L << j)) : 
	    (p_ptr->spell_learned2 & (1L << (j - 32))))
	{
	    /* Mark as forgotten */
	    if (j < 32) {
		p_ptr->spell_forgotten1 |= (1L << j);
	    } else {
		p_ptr->spell_forgotten2 |= (1L << (j - 32));
	    }

	    /* No longer known */
	    if (j < 32) {
		p_ptr->spell_learned1 &= ~(1L << j);
	    } else {
		p_ptr->spell_learned2 &= ~(1L << (j - 32));
	    }

	    /* Message */
	    msg_format("You have forgotten the %s of %s.", p,
		       spell_names[s_ptr->index]);

	    /* One more can be learned */
	    p_ptr->new_spells++;
	}
    }


    /* Forget spells if we know too many spells */
    for (i = 63; i >= 0; i--) {
	/* Stop when possible */
	if (p_ptr->new_spells >= 0)
	    break;

	/* Efficiency -- all done */
	if (!p_ptr->spell_learned1 && !p_ptr->spell_learned2)
	    break;

	/* Get the (i+1)th spell learned */
	j = p_ptr->spell_order[i];

	/* Skip unknown spells */
	if (j >= 99)
	    continue;

	/* Get the spell */
	s_ptr = &mp_ptr->info[j];

	/* Forget it (if learned) */
	if ((j <
	     32) ? (p_ptr->
		    spell_learned1 & (1L << j)) : (p_ptr->spell_learned2 & (1L
									    <<
									    (j -
									     32))))
	{
	    /* Mark as forgotten */
	    if (j < 32) {
		p_ptr->spell_forgotten1 |= (1L << j);
	    } else {
		p_ptr->spell_forgotten2 |= (1L << (j - 32));
	    }

	    /* No longer known */
	    if (j < 32) {
		p_ptr->spell_learned1 &= ~(1L << j);
	    } else {
		p_ptr->spell_learned2 &= ~(1L << (j - 32));
	    }

	    /* Message */
	    msg_format("You have forgotten the %s of %s.", p,
		       spell_names[s_ptr->index]);

	    /* One more can be learned */
	    p_ptr->new_spells++;
	}
    }


    /* Check for spells to remember */
    for (i = 0; i < 64; i++) {
	/* None left to remember */
	if (p_ptr->new_spells <= 0)
	    break;

	/* Efficiency -- all done */
	if (!p_ptr->spell_forgotten1 && !p_ptr->spell_forgotten2)
	    break;

	/* Get the next spell we learned */
	j = p_ptr->spell_order[i];

	/* Skip unknown spells */
	if (j >= 99)
	    break;

	/* Access the spell */
	s_ptr = &mp_ptr->info[j];

	/* Skip spells we cannot remember */
	if (s_ptr->slevel > p_ptr->lev)
	    continue;

	/* First set of spells */
	if ((j <
	     32) ? (p_ptr->
		    spell_forgotten1 & (1L << j)) : (p_ptr->spell_forgotten2 &
						     (1L << (j - 32)))) {
	    /* No longer forgotten */
	    if (j < 32) {
		p_ptr->spell_forgotten1 &= ~(1L << j);
	    } else {
		p_ptr->spell_forgotten2 &= ~(1L << (j - 32));
	    }

	    /* Known once more */
	    if (j < 32) {
		p_ptr->spell_learned1 |= (1L << j);
	    } else {
		p_ptr->spell_learned2 |= (1L << (j - 32));
	    }

	    /* Message */
	    msg_format("You have remembered the %s of %s.", p,
		       spell_names[s_ptr->index]);

	    /* One less can be learned */
	    p_ptr->new_spells--;
	}
    }


    /* Assume no spells available */
    k = 0;

    /* Count spells that can be learned */
    for (j = 0; j < mp_ptr->spell_number; j++) {
	/* Access the spell */
	s_ptr = &mp_ptr->info[j];

	/* Skip spells we cannot remember */
	if (s_ptr->slevel > p_ptr->lev)
	    continue;

	/* Skip spells we already know */
	if ((j <
	     32) ? (p_ptr->
		    spell_learned1 & (1L << j)) : (p_ptr->spell_learned2 & (1L
									    <<
									    (j -
									     32))))
	{
	    continue;
	}

	/* Count it */
	k++;
    }

    /* Cannot learn more spells than exist */
    if (p_ptr->new_spells > k)
	p_ptr->new_spells = k;

    /* Spell count changed */
    if (p_ptr->old_spells != p_ptr->new_spells) {
	/* Message if needed */
	if (p_ptr->new_spells) {
	    /* Message */
	    msg_format("You can learn %d more %s%s.", p_ptr->new_spells, p,
		       ((p_ptr->new_spells != 1)
			&& (mp_ptr->spell_book != TV_DRUID_BOOK)) ? "s" : "");
	}

	/* Save the new_spells value */
	p_ptr->old_spells = p_ptr->new_spells;

	/* Redraw Study Status */
	p_ptr->redraw |= (PR_STUDY);
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
    if (p_ptr->quests < 2)	/* -NRM- */
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
	    msg_print("You have regained specialist abilities.");

	    /* Put forgotten specialties back on the flags */
	    for (i = MIN(0, p_ptr->new_specialties); i > p_ptr->old_specialties;
		 i--)
		pf_on(p_ptr->pflags, p_ptr->specialty_order[num_known + i]);
	}

	if (p_ptr->new_specialties > 0)
	    msg_print("You may learn a specialist ability using the 'O' key.");

	/* Redraw Study Status */
	p_ptr->redraw |= (PR_STUDY);
    }

    /* Fewer specialties are available (or more forgotten) */
    if (p_ptr->old_specialties > p_ptr->new_specialties) {
	if (p_ptr->new_specialties < 0) {
	    msg_print("You have lost specialist abilities.");

	    /* Remove forgotten specialties from the flags */
	    for (i = 0; i > p_ptr->new_specialties; i--)
		pf_off(p_ptr->pflags, p_ptr->specialty_order[num_known - i]);
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


    /* Hack -- Must possess some magical realm. */
    if (!mp_ptr->spell_realm)
	return;

    /* Hack -- handle "xtra" mode */
    if (character_xtra)
	return;

    /* Extract "effective" player level */
    levels = (p_ptr->lev - mp_ptr->spell_first) + 1;

    /* Hack -- no negative mana */
    if (levels < 0)
	levels = 0;

    /* Extract total mana, using standard rounding. */
    msp = (adj_mag_mana[p_ptr->stat_ind[mp_ptr->spell_stat]] * levels + 5) / 10;

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
    if (p_ptr->old_cumber_glove != p_ptr->cumber_glove) {
	/* Message */
	if (p_ptr->cumber_glove) {
	    msg_print("Your covered hands feel unsuitable for spellcasting.");
	} else {
	    msg_print("Your hands feel more suitable for spellcasting.");
	}

	/* Save it */
	p_ptr->old_cumber_glove = p_ptr->cumber_glove;
    }


    /* Take note when "armor state" changes */
    if (p_ptr->old_cumber_armor != p_ptr->cumber_armor) {
	/* Message */
	if (p_ptr->cumber_armor) {
	    msg_print("The weight of your armor encumbers your movement.");
	} else {
	    msg_print("You feel able to move more freely.");
	}

	/* Save it */
	p_ptr->old_cumber_armor = p_ptr->cumber_armor;
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
 * Extract and set the current "lite radius"
 */
static void calc_torch(void)
{
    object_type *o_ptr = &p_ptr->inventory[INVEN_LITE];

    /* Assume no light */
    p_ptr->cur_lite = 0;

    /* Player is glowing */
    if (p_ptr->lite) {
	notice_obj(OF_LITE, 0);
	p_ptr->cur_lite += 1;
    }

    /* Examine actual lites */
    if (o_ptr->tval == TV_LITE) {
	/* Torches (with fuel) provide some light */
	if ((o_ptr->sval == SV_LITE_TORCH) && (o_ptr->pval > 0)) {
	    p_ptr->cur_lite += 1;
	}

	/* Lanterns (with fuel) provide more light */
	if ((o_ptr->sval == SV_LITE_LANTERN) && (o_ptr->pval > 0)) {
	    p_ptr->cur_lite += 2;
	}

	/* Artifact Lites provide permanent, bright, light */
	if (artifact_p(o_ptr))
	    p_ptr->cur_lite += 3;
    }

    /* Priests and Paladins get a bonus to light radius at level 35 and 45,
     * respectively. */
    if (player_has(PF_HOLY)) {
	/* Hack -- the "strong caster" check here is a hack. What is the better 
	 * option? */
	if ((p_ptr->lev > 44)
	    || ((p_ptr->lev > 34) && (player_has(PF_STRONG_MAGIC)))) {
	    p_ptr->cur_lite += 1;
	}
    }


    /* Special ability Holy Light */
    if (player_has(PF_HOLY_LIGHT))
	p_ptr->cur_lite++;

    /* Special ability Unlight */
    if (player_has(PF_UNLIGHT) || p_ptr->state.darkness) {
	notice_obj(OF_DARKNESS, 0);
	p_ptr->cur_lite--;
    }

    /* Reduce lite when running if requested */
    if (p_ptr->running && OPT(view_reduce_lite)) {
	/* Reduce the lite radius if needed */
	if (p_ptr->cur_lite > 1)
	    p_ptr->cur_lite = 1;
    }

    /* Notice changes in the "lite radius" */
    if (p_ptr->old_lite != p_ptr->cur_lite) {
	/* Update the visuals */
	p_ptr->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

	/* Remember the old lite */
	p_ptr->old_lite = p_ptr->cur_lite;
    }
}



/**
 * Computes current weight limit.
 */
static int weight_limit(void)
{
    int i;

    /* Weight limit based only on strength */
    i = adj_str_wgt[p_ptr->state.stat_ind[A_STR]] * 100;

    /* Return the result */
    return (i);
}

/** Calculate all class-based bonuses and penalties to melee Skill.  Oangband
 * recognizes that it takes a great deal of training to get critical hits with
 * a large, heavy weapon - training that many classes simply do not have the
 * time or inclination for.  -LM- 
 */
int add_special_melee_skill(byte pclass, s16b weight, object_type * o_ptr)
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
	&& (!p_ptr->state.bless_blade)) {
	add_skill -= 10 + p_ptr->lev / 2;

	/* Icky weapon */
	p_ptr->icky_wield = TRUE;
    }

    /* Paladin bonus for blessed weapons. */
    if ((player_has(PF_BLESS_WEAPON)) && (!player_has(PF_STRONG_MAGIC))
	&& (p_ptr->state.bless_blade))
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
int add_special_missile_skill(byte pclass, s16b weight, object_type * o_ptr)
{
    int add_skill = 0;

    /* Nice bonus for most favored weapons - if no tradeoff */
    if ((((player_has(PF_BOW_SPEED_GREAT))
	  && (p_ptr->state.ammo_tval == TV_ARROW))
	 || ((player_has(PF_SLING_SPEED_GREAT))
	     && (p_ptr->state.ammo_tval == TV_SHOT))
	 || ((player_has(PF_XBOW_SPEED_GREAT))
	     && (p_ptr->state.ammo_tval == TV_BOLT)))
	&& (!player_has(PF_RAPID_FIRE))) {
	/* Big bonus */
	add_skill = 3 + p_ptr->lev / 4;
    }

    /* Hack - Unarmed fighters (i.e. Druids) do a bit better with slings */
    if ((player_has(PF_UNARMED_COMBAT)) &
	(p_ptr->state.ammo_tval == TV_SHOT)) {
	add_skill = p_ptr->lev / 7;
    }

    /* Now, special racial abilities and limitations are considered.  The
     * choice of race can be of some significance. */

    if (p_ptr->state.ammo_tval == TV_BOLT) {
	if (player_has(PF_XBOW_SKILL))
	    add_skill += 3 + p_ptr->lev / 7;
	else if (player_has(PF_XBOW_UNSKILL))
	    add_skill -= 3 + p_ptr->lev / 7;
    } else if (p_ptr->state.ammo_tval == TV_ARROW) {
	if (player_has(PF_BOW_SKILL))
	    add_skill += 3 + p_ptr->lev / 7;
	else if (player_has(PF_BOW_UNSKILL))
	    add_skill -= 3 + p_ptr->lev / 7;
    } else if (p_ptr->state.ammo_tval == TV_SHOT) {
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
static void resistance_limits(void)
{
    int i;

    /* Check all extremes */
    for (i = 0; i < MAX_P_RES; i++) {
	if (p_ptr->state.res_list[i] > RES_LEVEL_MAX)
	    p_ptr->state.res_list[i] = RES_LEVEL_MAX;
	if (p_ptr->state.res_list[i] < RES_LEVEL_MIN)
	    p_ptr->state.res_list[i] = RES_LEVEL_MIN;
	if (p_ptr->state.dis_res_list[i] > RES_LEVEL_MAX)
	    p_ptr->state.dis_res_list[i] = RES_LEVEL_MAX;
	if (p_ptr->state.dis_res_list[i] < RES_LEVEL_MIN)
	    p_ptr->state.dis_res_list[i] = RES_LEVEL_MIN;
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
static void shape_change_stat(void)
{
    switch (p_ptr->schange) {
    case SHAPE_NORMAL:
	break;
    case SHAPE_MOUSE:
	{
	    p_ptr->state.stat_add[A_STR] -= 2;
	    p_ptr->state.stat_add[A_INT] -= 7;
	    p_ptr->state.stat_add[A_CON] -= 1;
	    p_ptr->state.stat_add[A_CHR] -= 5;
	    break;
	}
    case SHAPE_FERRET:
	{
	    p_ptr->state.stat_add[A_DEX] += 4;
	    p_ptr->state.stat_add[A_CHR] -= 2;
	    break;
	}
    case SHAPE_HOUND:
	{
	    p_ptr->state.stat_add[A_CON] += 2;
	    p_ptr->state.stat_add[A_INT] -= 2;
	    p_ptr->state.stat_add[A_CHR] -= 2;
	    break;
	}
    case SHAPE_GAZELLE:
	{
	    p_ptr->state.stat_add[A_STR] -= 2;
	    p_ptr->state.stat_add[A_DEX] += 2;
	    p_ptr->state.stat_add[A_CON] -= 1;
	    p_ptr->state.stat_add[A_WIS] -= 2;
	    break;
	}
    case SHAPE_LION:
	{
	    p_ptr->state.stat_add[A_STR] += 3;
	    p_ptr->state.stat_add[A_CHR] -= 4;
	    p_ptr->state.stat_add[A_WIS] -= 2;
	    p_ptr->state.stat_add[A_INT] -= 2;
	    break;
	}
    case SHAPE_ENT:
	{
	    p_ptr->state.stat_add[A_STR] += 4;
	    p_ptr->state.stat_add[A_WIS] += 1;
	    p_ptr->state.stat_add[A_DEX] -= 5;
	    p_ptr->state.stat_add[A_CON] += 4;
	    p_ptr->state.stat_add[A_CHR] -= 1;
	    break;
	}
    case SHAPE_BAT:
	{
	    p_ptr->state.stat_add[A_STR] -= 1;
	    p_ptr->state.stat_add[A_WIS] -= 2;
	    p_ptr->state.stat_add[A_INT] -= 2;
	    p_ptr->state.stat_add[A_CHR] -= 2;
	    break;
	}
    case SHAPE_WEREWOLF:
	{
	    p_ptr->state.stat_add[A_STR] += 2;
	    p_ptr->state.stat_add[A_CHR] -= 5;
	    p_ptr->state.stat_add[A_INT] -= 2;
	    break;
	}
    case SHAPE_VAMPIRE:
	{
	    p_ptr->state.stat_add[A_STR] += 2;
	    p_ptr->state.stat_add[A_CON] += 1;
	    p_ptr->state.stat_add[A_INT] += 2;
	    p_ptr->state.stat_add[A_CHR] -= 3;
	    break;
	}
    case SHAPE_WYRM:
	{
	    p_ptr->state.stat_add[A_STR] += 2;
	    p_ptr->state.stat_add[A_CON] += 1;
	    p_ptr->state.stat_add[A_WIS] += 1;
	    p_ptr->state.stat_add[A_INT] += 1;
	    p_ptr->state.stat_add[A_DEX] -= 1;
	    p_ptr->state.stat_add[A_CHR] -= 1;
	    break;
	}
    case SHAPE_BEAR:
	{
	    p_ptr->state.stat_add[A_STR] += 1;
	    if (p_ptr->lev >= 10)
		p_ptr->state.stat_add[A_STR] += 1;
	    if (p_ptr->lev >= 20)
		p_ptr->state.stat_add[A_CON] += 1;
	    if (p_ptr->lev >= 30)
		p_ptr->state.stat_add[A_CON] += 1;
	    if (p_ptr->lev >= 40)
		p_ptr->state.stat_add[A_STR] += 1;
	    p_ptr->state.stat_add[A_INT] -= 1;
	    p_ptr->state.stat_add[A_CHR] -= 1;
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
static void shape_change_main(void)
{
    object_type *o_ptr;
    switch (p_ptr->schange) {
    case SHAPE_NORMAL:
	break;
    case SHAPE_MOUSE:
	{
	    p_ptr->state.skills[SKILL_STEALTH] =
		(30 + p_ptr->state.skills[SKILL_STEALTH]) / 2;
	    p_ptr->state.see_infra += 2;
	    p_ptr->state.aggravate = FALSE;
	    p_ptr->state.to_a -= 5;
	    p_ptr->state.dis_to_a -= 5;
	    p_ptr->state.to_h -= 15;
	    p_ptr->state.dis_to_h -= 15;
	    p_ptr->state.to_d -= 25;
	    p_ptr->state.dis_to_d -= 25;
	    p_ptr->state.skills[SKILL_DEVICE] /= 4;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_FERRET:
	{
	    p_ptr->state.see_infra += 2;
	    p_ptr->state.regenerate = TRUE;
	    p_ptr->state.to_d -= 10;
	    p_ptr->state.dis_to_d -= 10;
	    p_ptr->pspeed += 2;
	    p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] += 10;
	    p_ptr->state.skills[SKILL_SEARCH] += 10;
	    p_ptr->state.skills[SKILL_DEVICE] /= 2;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_HOUND:
	{
	    p_ptr->state.see_infra += 3;
	    p_ptr->state.telepathy = TRUE;
	    p_ptr->state.skills[SKILL_DEVICE] /= 2;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_GAZELLE:
	{
	    p_ptr->state.to_a += 5;
	    p_ptr->state.dis_to_a += 5;
	    p_ptr->state.to_d -= 5;
	    p_ptr->state.dis_to_d -= 5;
	    p_ptr->pspeed += 6;
	    p_ptr->state.skills[SKILL_DEVICE] /= 2;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_LION:
	{
	    p_ptr->state.no_fear = TRUE;
	    p_ptr->state.regenerate = TRUE;
	    p_ptr->state.to_a += 5;
	    p_ptr->state.dis_to_a += 5;
	    p_ptr->state.to_h += 10;
	    p_ptr->state.dis_to_h += 10;
	    p_ptr->state.to_d += 15;
	    p_ptr->state.dis_to_d += 15;
	    p_ptr->pspeed += 1;
	    p_ptr->state.skills[SKILL_DEVICE] /= 2;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_ENT:
	{
	    apply_resist(&p_ptr->state.res_list[P_RES_COLD], RES_BOOST_NORMAL);
	    apply_resist(&p_ptr->state.dis_res_list[P_RES_COLD],
			 RES_BOOST_NORMAL);
	    apply_resist(&p_ptr->state.res_list[P_RES_POIS], RES_BOOST_NORMAL);
	    apply_resist(&p_ptr->state.dis_res_list[P_RES_POIS],
			 RES_BOOST_NORMAL);
	    apply_resist(&p_ptr->state.res_list[P_RES_FIRE], RES_CUT_MINOR);
	    apply_resist(&p_ptr->state.dis_res_list[P_RES_FIRE], RES_CUT_MINOR);
	    if (p_ptr->state.res_list[P_RES_FIRE] < RES_CAP_MODERATE)
		p_ptr->state.res_list[P_RES_FIRE] = RES_CAP_MODERATE;
	    if (p_ptr->state.dis_res_list[P_RES_FIRE] < RES_CAP_MODERATE)
		p_ptr->state.dis_res_list[P_RES_FIRE] = RES_CAP_MODERATE;
	    p_ptr->state.no_fear = TRUE;
	    p_ptr->state.see_inv = TRUE;
	    p_ptr->state.free_act = TRUE;
	    p_ptr->state.ffall = FALSE;
	    p_ptr->state.to_d += 10;
	    p_ptr->state.dis_to_d += 10;
	    p_ptr->state.skills[SKILL_DIGGING] += 150;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_BAT:
	{
	    p_ptr->state.see_infra += 6;
	    p_ptr->state.no_blind = TRUE;
	    p_ptr->state.ffall = TRUE;
	    p_ptr->state.to_h -= 5;
	    p_ptr->state.dis_to_h -= 5;
	    p_ptr->state.to_d -= 15;
	    p_ptr->state.dis_to_d -= 15;
	    p_ptr->pspeed += 5;
	    p_ptr->state.skills[SKILL_DEVICE] /= 4;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_WEREWOLF:
	{
	    p_ptr->state.see_infra += 3;
	    p_ptr->state.regenerate = TRUE;
	    p_ptr->state.aggravate = TRUE;
	    p_ptr->state.to_a += 5;
	    p_ptr->state.dis_to_a += 5;
	    p_ptr->state.to_h += 20;
	    p_ptr->state.dis_to_h += 20;
	    p_ptr->state.to_d += 20;
	    p_ptr->state.dis_to_d += 20;
	    p_ptr->state.skills[SKILL_DEVICE] /= 2;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    case SHAPE_VAMPIRE:
	{
	    p_ptr->state.see_infra += 3;
	    if (p_ptr->cur_lite >= 3)
		p_ptr->cur_lite = 2;
	    p_ptr->state.see_inv = TRUE;
	    p_ptr->state.hold_life = TRUE;
	    apply_resist(&p_ptr->state.res_list[P_RES_COLD], RES_BOOST_NORMAL);
	    apply_resist(&p_ptr->state.dis_res_list[P_RES_COLD],
			 RES_BOOST_NORMAL);
	    apply_resist(&p_ptr->state.res_list[P_RES_LITE], RES_CUT_MINOR);
	    apply_resist(&p_ptr->state.dis_res_list[P_RES_LITE], RES_CUT_MINOR);
	    if (p_ptr->state.res_list[P_RES_LITE] < RES_CAP_EXTREME)
		p_ptr->state.res_list[P_RES_LITE] = RES_CAP_EXTREME;
	    if (p_ptr->state.dis_res_list[P_RES_LITE] < RES_CAP_EXTREME)
		p_ptr->state.dis_res_list[P_RES_LITE] = RES_CAP_EXTREME;
	    p_ptr->state.regenerate = TRUE;
	    p_ptr->state.to_a += 5;
	    p_ptr->state.dis_to_a += 5;
	    p_ptr->state.to_h += 5;
	    p_ptr->state.dis_to_h += 5;
	    p_ptr->state.to_d += 5;
	    p_ptr->state.dis_to_d += 5;
	    p_ptr->state.skills[SKILL_STEALTH] += 1;
	    p_ptr->state.skills[SKILL_DEVICE] += 30;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 10;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 10;
	    break;
	}
    case SHAPE_WYRM:
	{
	    o_ptr = &p_ptr->inventory[INVEN_BODY];
	    p_ptr->state.to_a += 10;
	    p_ptr->state.dis_to_a += 10;
	    p_ptr->state.to_d += 5;
	    p_ptr->state.dis_to_d += 5;
	    p_ptr->state.skills[SKILL_STEALTH] -= 3;
	    p_ptr->state.skills[SKILL_DEVICE] += 10;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;

	    /* 
	     * Apply an extra bonus power depending on the type
	     * of DSM when in WYRM form 
	     */
	    if (o_ptr->tval == TV_DRAG_ARMOR) {
		/* Elemental DSM -> immunity */
		if (o_ptr->sval == SV_DRAGON_BLACK) {
		    apply_resist(&p_ptr->state.res_list[P_RES_ACID],
				 RES_BOOST_IMMUNE);
		    apply_resist(&p_ptr->state.dis_res_list[P_RES_ACID],
				 RES_BOOST_IMMUNE);
		} else if (o_ptr->sval == SV_DRAGON_BLUE) {
		    apply_resist(&p_ptr->state.res_list[P_RES_ELEC],
				 RES_BOOST_IMMUNE);
		    apply_resist(&p_ptr->state.dis_res_list[P_RES_ELEC],
				 RES_BOOST_IMMUNE);
		} else if (o_ptr->sval == SV_DRAGON_WHITE) {
		    apply_resist(&p_ptr->state.res_list[P_RES_COLD],
				 RES_BOOST_IMMUNE);
		    apply_resist(&p_ptr->state.dis_res_list[P_RES_COLD],
				 RES_BOOST_IMMUNE);
		} else if (o_ptr->sval == SV_DRAGON_RED) {
		    apply_resist(&p_ptr->state.res_list[P_RES_FIRE],
				 RES_BOOST_IMMUNE);
		    apply_resist(&p_ptr->state.dis_res_list[P_RES_FIRE],
				 RES_BOOST_IMMUNE);
		} else if (o_ptr->sval == SV_DRAGON_GREEN) {
		    apply_resist(&p_ptr->state.res_list[P_RES_POIS],
				 RES_BOOST_IMMUNE);
		    apply_resist(&p_ptr->state.dis_res_list[P_RES_POIS],
				 RES_BOOST_IMMUNE);
		}

		/* Shining DSM -> SI */
		else if (o_ptr->sval == SV_DRAGON_SHINING)
		    p_ptr->state.see_inv = TRUE;

		/* Law/Chaos DSM -> hold life */
		else if (o_ptr->sval == SV_DRAGON_LAW)
		    p_ptr->state.hold_life = TRUE;
		else if (o_ptr->sval == SV_DRAGON_CHAOS)
		    p_ptr->state.hold_life = TRUE;

		/* Bronze/Gold DSM -> FA */
		else if (o_ptr->sval == SV_DRAGON_BRONZE)
		    p_ptr->state.free_act = TRUE;
		else if (o_ptr->sval == SV_DRAGON_GOLD)
		    p_ptr->state.free_act = TRUE;

		/* Multihued, Balance and Power don't need any help */
	    }
	    break;
	}
    case SHAPE_BEAR:
	{
	    p_ptr->state.to_a += 5;
	    p_ptr->state.dis_to_a += 5;
	    p_ptr->state.to_h += 5;
	    p_ptr->state.dis_to_h += 5;
	    if (p_ptr->lev >= 10) {
		p_ptr->state.to_d += 5;
		p_ptr->state.dis_to_d += 5;
	    }
	    if (p_ptr->lev >= 20) {
		p_ptr->state.to_d += 5;
		p_ptr->state.dis_to_d += 5;
	    }
	    if (p_ptr->lev >= 30) {
		p_ptr->state.to_a += 10;
		p_ptr->state.dis_to_a += 10;
	    }
	    if (p_ptr->lev >= 40) {
		p_ptr->state.to_h += 5;
		p_ptr->state.dis_to_h += 5;
	    }
	    p_ptr->state.skills[SKILL_DEVICE] /= 2;
	    p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 30;
	    p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 30;
	    break;
	}
    }

    /* End shape bonuses; do bounds check on resistance levels */
    resistance_limits();
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
extern void calc_bonuses(bool inspect)
{
    int i, j, hold;

    int old_speed;

    int old_stealth;

    int old_see_infra;
    int old_telepathy;
    int old_see_inv;

    int temp_armour;

    int old_dis_ac;
    int old_dis_to_a;

    int extra_shots;
    int extra_might;

    int old_stat_top[A_MAX];
    int old_stat_use[A_MAX];
    int old_stat_ind[A_MAX];

    bool enhance = FALSE;

    object_type *o_ptr;

  /*** Memorize ***/


    /* Save the old speed */
    old_speed = p_ptr->pspeed;

    /* Save the old stealth */
    old_stealth = p_ptr->state.skills[SKILL_STEALTH];

    /* Save the old vision stuff */
    old_telepathy = p_ptr->state.telepathy;
    old_see_inv = p_ptr->state.see_inv;
    old_see_infra = p_ptr->state.see_infra;

    /* Save the old armor class */
    old_dis_ac = p_ptr->state.dis_ac;
    old_dis_to_a = p_ptr->state.dis_to_a;

    /* Save the old stats */
    for (i = 0; i < A_MAX; i++) {
	old_stat_top[i] = p_ptr->state.stat_top[i];
	old_stat_use[i] = p_ptr->state.stat_use[i];
	old_stat_ind[i] = p_ptr->state.stat_ind[i];
    }


    /* Hack - If the player's usage of his shield changes, we must recalculate
     * various things. */
  calc_again:


  /*** Reset ***/

    /* Reset player speed */
    p_ptr->pspeed = 110;

    /* Reset "blow" info */
    p_ptr->state.num_blow = 1;

    /* Reset "fire" info */
    p_ptr->state.num_fire = 0;
    p_ptr->state.ammo_mult = 0;
    p_ptr->state.ammo_tval = 0;
    extra_shots = 0;
    extra_might = 0;

    /* Clear the stat modifiers */
    for (i = 0; i < A_MAX; i++)
	p_ptr->state.stat_add[i] = 0;

    /* Clear the Displayed/Real armor class */
    p_ptr->state.dis_ac = p_ptr->state.ac = 0;

    /* Clear the Displayed/Real Bonuses */
    p_ptr->state.dis_to_h = p_ptr->state.to_h = 0;
    p_ptr->state.dis_to_d = p_ptr->state.to_d = 0;
    p_ptr->state.dis_to_a = p_ptr->state.to_a = 0;

    /* Clear all the flags */
    p_ptr->state.teleport = FALSE;
    p_ptr->state.no_teleport = FALSE;
    p_ptr->state.aggravate = FALSE;
    p_ptr->state.rand_aggro = FALSE;
    p_ptr->state.slow_regen = FALSE;
    p_ptr->state.fear = FALSE;
    p_ptr->state.fast_digest = FALSE;
    p_ptr->state.rand_pois = FALSE;
    p_ptr->state.rand_pois_bad = FALSE;
    p_ptr->state.rand_cuts = FALSE;
    p_ptr->state.rand_cuts_bad = FALSE;
    p_ptr->state.rand_hallu = FALSE;
    p_ptr->state.drop_weapon = FALSE;
    p_ptr->state.attract_demon = FALSE;
    p_ptr->state.attract_undead = FALSE;
    p_ptr->state.rand_paral = FALSE;
    p_ptr->state.rand_paral_all = FALSE;
    p_ptr->state.drain_exp = FALSE;
    p_ptr->state.drain_mana = FALSE;
    p_ptr->state.drain_stat = FALSE;
    p_ptr->state.drain_charge = FALSE;
    p_ptr->state.bless_blade = FALSE;
    p_ptr->state.impact = FALSE;
    p_ptr->state.see_inv = FALSE;
    p_ptr->state.free_act = FALSE;
    p_ptr->state.slow_digest = FALSE;
    p_ptr->state.regenerate = FALSE;
    p_ptr->state.ffall = FALSE;
    p_ptr->state.hold_life = FALSE;
    p_ptr->state.telepathy = FALSE;
    p_ptr->state.lite = FALSE;
    p_ptr->state.sustain_str = FALSE;
    p_ptr->state.sustain_int = FALSE;
    p_ptr->state.sustain_wis = FALSE;
    p_ptr->state.sustain_con = FALSE;
    p_ptr->state.sustain_dex = FALSE;
    p_ptr->state.sustain_chr = FALSE;
    p_ptr->state.no_fear = FALSE;
    p_ptr->state.no_blind = FALSE;
    p_ptr->state.darkness = FALSE;
    p_ptr->special_attack &= ~(ATTACK_CHAOTIC);

    for (i = 0; i < MAX_P_RES; i++) {
	p_ptr->state.res_list[i] = RES_LEVEL_BASE;
	p_ptr->state.dis_res_list[i] = RES_LEVEL_BASE;
    }


  /*** Extract race/class info ***/

    /* Base infravision (purely racial) */
    p_ptr->state.see_infra = rp_ptr->infra;

    /* Base skill -- disarming */
    p_ptr->state.skills[SKILL_DISARM] = rp_ptr->r_dis + cp_ptr->c_dis;

    /* Base skill -- magic devices */
    p_ptr->state.skills[SKILL_DEVICE] = rp_ptr->r_dev + cp_ptr->c_dev;

    /* Base skill -- saving throw */
    p_ptr->state.skills[SKILL_SAVE] = rp_ptr->r_sav + cp_ptr->c_sav;

    /* Base skill -- stealth */
    p_ptr->state.skills[SKILL_STEALTH] = rp_ptr->r_stl + cp_ptr->c_stl;

    /* Base skill -- searching ability */
    p_ptr->state.skills[SKILL_SEARCH] = rp_ptr->r_srh + cp_ptr->c_srh;

    /* Base skill -- searching frequency */
    p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] = rp_ptr->r_fos + cp_ptr->c_fos;

    /* Base skill -- combat (melee) */
    p_ptr->state.skills[SKILL_TO_HIT_MELEE] = rp_ptr->r_thn + cp_ptr->c_thn;

    /* Base skill -- combat (shooting) */
    p_ptr->state.skills[SKILL_TO_HIT_BOW] = rp_ptr->r_thb + cp_ptr->c_thb;

    /* Base skill -- combat (throwing) */
    p_ptr->state.skills[SKILL_TO_HIT_THROW] = rp_ptr->r_thb + cp_ptr->c_thb;

    /* Base skill -- digging */
    p_ptr->state.skills[SKILL_DIGGING] = 0;

  /*** Analyze player ***/

    /* Object flags */
    if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_STR))
	p_ptr->state.sustain_str = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_INT))
	p_ptr->state.sustain_int = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_WIS))
	p_ptr->state.sustain_wis = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_DEX))
	p_ptr->state.sustain_dex = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_CON))
	p_ptr->state.sustain_con = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SUSTAIN_CHR))
	p_ptr->state.sustain_chr = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SLOW_DIGEST))
	p_ptr->state.slow_digest = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_FEATHER))
	p_ptr->state.ffall = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_LITE))
	p_ptr->state.lite = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_REGEN))
	p_ptr->state.regenerate = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_TELEPATHY))
	p_ptr->state.telepathy = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SEE_INVIS))
	p_ptr->state.see_inv = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_FREE_ACT))
	p_ptr->state.free_act = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_HOLD_LIFE))
	p_ptr->state.hold_life = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_BLESSED))
	p_ptr->state.bless_blade = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_IMPACT))
	p_ptr->state.impact = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_FEARLESS))
	p_ptr->state.no_fear = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_SEEING))
	p_ptr->state.no_blind = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_DARKNESS))
	p_ptr->state.darkness = TRUE;
    if (of_has(rp_ptr->flags_obj, OF_CHAOTIC))
	p_ptr->special_attack |= ATTACK_CHAOTIC;


    /* Curse flags */
    if (cf_has(rp_ptr->flags_curse, OF_TELEPORT))
	p_ptr->state.teleport = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_NO_TELEPORT))
	p_ptr->state.no_teleport = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_AGGRO_PERM))
	p_ptr->state.aggravate = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_AGGRO_RAND))
	p_ptr->state.rand_aggro = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_SLOW_REGEN))
	p_ptr->state.slow_regen = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_AFRAID))
	p_ptr->state.fear = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_HUNGRY))
	p_ptr->state.fast_digest = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_POIS_RAND))
	p_ptr->state.rand_pois = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_POIS_RAND_BAD))
	p_ptr->state.rand_pois_bad = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_CUT_RAND))
	p_ptr->state.rand_cuts = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_CUT_RAND_BAD))
	p_ptr->state.rand_cuts_bad = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_HALLU_RAND))
	p_ptr->state.rand_hallu = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_DROP_WEAPON))
	p_ptr->state.drop_weapon = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_ATTRACT_DEMON))
	p_ptr->state.attract_demon = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_ATTRACT_UNDEAD))
	p_ptr->state.attract_undead = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_PARALYZE))
	p_ptr->state.rand_paral = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_PARALYZE_ALL))
	p_ptr->state.rand_paral_all = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_DRAIN_EXP))
	p_ptr->state.drain_exp = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_DRAIN_MANA))
	p_ptr->state.drain_mana = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_DRAIN_STAT))
	p_ptr->state.drain_stat = TRUE;
    if (cf_has(rp_ptr->flags_curse, OF_DRAIN_CHARGE))
	p_ptr->state.drain_charge = TRUE;

    /* Resistances */
    for (i = 0; i < MAX_P_RES; i++) {
	p_ptr->state.res_list[i] = rp_ptr->percent_res[i];
	p_ptr->state.dis_res_list[i] = rp_ptr->percent_res[i];
    }

    /* Ent */
    if (player_has(PF_WOODEN)) {
	/* Ents dig like maniacs, but only with their hands. */
	if (!p_ptr->inventory[INVEN_WIELD].k_idx)
	    p_ptr->state.skills[SKILL_DIGGING] += p_ptr->lev * 10;

	/* Ents get tougher and stronger as they age, but lose dexterity. */
	if (p_ptr->lev > 25)
	    p_ptr->state.stat_add[A_STR]++;
	if (p_ptr->lev > 40)
	    p_ptr->state.stat_add[A_STR]++;
	if (p_ptr->lev > 45)
	    p_ptr->state.stat_add[A_STR]++;

	if (p_ptr->lev > 25)
	    p_ptr->state.stat_add[A_DEX]--;
	if (p_ptr->lev > 40)
	    p_ptr->state.stat_add[A_DEX]--;
	if (p_ptr->lev > 45)
	    p_ptr->state.stat_add[A_DEX]--;

	if (p_ptr->lev > 25)
	    p_ptr->state.stat_add[A_CON]++;
	if (p_ptr->lev > 40)
	    p_ptr->state.stat_add[A_CON]++;
	if (p_ptr->lev > 45)
	    p_ptr->state.stat_add[A_CON]++;
    }

    /* Warrior. */
    if (player_has(PF_RELENTLESS)) {
	if (p_ptr->lev >= 30)
	    p_ptr->state.no_fear = TRUE;
	if (p_ptr->lev >= 40)
	    p_ptr->state.regenerate = TRUE;
    }

    /* Specialty ability Holy Light */
    if (player_has(PF_HOLY_LIGHT)) {
	apply_resist(&p_ptr->state.res_list[P_RES_LITE], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_LITE], RES_BOOST_NORMAL);
    }

    /* Specialty ability Unlight */
    if (player_has(PF_UNLIGHT)) {
	apply_resist(&p_ptr->state.res_list[P_RES_DARK], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_DARK], RES_BOOST_NORMAL);
    }

    /* End inherent resistances; do bounds check on resistance levels */
    resistance_limits();

  /*** Analyze equipment ***/

    /* Scan the equipment */
    for (i = INVEN_WIELD; i < INVEN_SUBTOTAL; i++) {
	o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx)
	    continue;

	/* Affect stats */
	for (j = 0; j < A_MAX; j++)
	    p_ptr->state.stat_add[j] += o_ptr->bonus_stat[j];

	/* Affect stealth */
	p_ptr->state.skills[SKILL_STEALTH] +=
	    o_ptr->bonus_other[P_BONUS_STEALTH];

	/* Affect searching ability (factor of five) */
	p_ptr->state.skills[SKILL_SEARCH] +=
	    (o_ptr->bonus_other[P_BONUS_SEARCH] * 5);

	/* Affect searching frequency (factor of five) */
	p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] +=
	    (o_ptr->bonus_other[P_BONUS_SEARCH] * 5);

	/* Affect infravision */
	p_ptr->state.see_infra += o_ptr->bonus_other[P_BONUS_INFRA];

	/* Affect digging (factor of 20) */
	p_ptr->state.skills[SKILL_DIGGING] +=
	    (o_ptr->bonus_other[P_BONUS_TUNNEL] * 20);

	/* Affect speed */
	p_ptr->pspeed += o_ptr->bonus_other[P_BONUS_SPEED];

	p_ptr->state.skills[SKILL_DEVICE] +=
	    10 * o_ptr->bonus_other[P_BONUS_M_MASTERY];

	/* Affect shots.  Altered in Oangband. */
	extra_shots += o_ptr->bonus_other[P_BONUS_SHOTS];

	/* Affect might.  Altered in Oangband. */
	extra_might += o_ptr->bonus_other[P_BONUS_MIGHT];

	/* Object flags */
	if (of_has(o_ptr->flags_obj, OF_SUSTAIN_STR))
	    p_ptr->state.sustain_str = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SUSTAIN_INT))
	    p_ptr->state.sustain_int = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SUSTAIN_WIS))
	    p_ptr->state.sustain_wis = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SUSTAIN_DEX))
	    p_ptr->state.sustain_dex = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SUSTAIN_CON))
	    p_ptr->state.sustain_con = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SUSTAIN_CHR))
	    p_ptr->state.sustain_chr = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SLOW_DIGEST))
	    p_ptr->state.slow_digest = TRUE;
	if (of_has(o_ptr->flags_obj, OF_FEATHER))
	    p_ptr->state.ffall = TRUE;
	if (of_has(o_ptr->flags_obj, OF_LITE))
	    p_ptr->state.lite = TRUE;
	if (of_has(o_ptr->flags_obj, OF_REGEN))
	    p_ptr->state.regenerate = TRUE;
	if (of_has(o_ptr->flags_obj, OF_TELEPATHY))
	    p_ptr->state.telepathy = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SEE_INVIS))
	    p_ptr->state.see_inv = TRUE;
	if (of_has(o_ptr->flags_obj, OF_FREE_ACT))
	    p_ptr->state.free_act = TRUE;
	if (of_has(o_ptr->flags_obj, OF_HOLD_LIFE))
	    p_ptr->state.hold_life = TRUE;
	if (of_has(o_ptr->flags_obj, OF_FEARLESS))
	    p_ptr->state.no_fear = TRUE;
	if (of_has(o_ptr->flags_obj, OF_SEEING))
	    p_ptr->state.no_blind = TRUE;
	if (of_has(o_ptr->flags_obj, OF_IMPACT))
	    p_ptr->state.impact = TRUE;
	if (of_has(o_ptr->flags_obj, OF_BLESSED))
	    p_ptr->state.bless_blade = TRUE;
	if (of_has(o_ptr->flags_obj, OF_DARKNESS))
	    p_ptr->state.darkness = TRUE;
	if (of_has(o_ptr->flags_obj, OF_CHAOTIC))
	    p_ptr->special_attack |= ATTACK_CHAOTIC;

	/* Bad flags */
	if (cf_has(o_ptr->flags_curse, CF_TELEPORT))
	    p_ptr->state.teleport = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_NO_TELEPORT))
	    p_ptr->state.no_teleport = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_AGGRO_PERM))
	    p_ptr->state.aggravate = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_AGGRO_RAND))
	    p_ptr->state.rand_aggro = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_SLOW_REGEN))
	    p_ptr->state.slow_regen = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_AFRAID))
	    p_ptr->state.fear = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_HUNGRY))
	    p_ptr->state.fast_digest = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_POIS_RAND))
	    p_ptr->state.rand_pois = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_POIS_RAND_BAD))
	    p_ptr->state.rand_pois_bad = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_CUT_RAND))
	    p_ptr->state.rand_cuts = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_CUT_RAND_BAD))
	    p_ptr->state.rand_cuts_bad = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_HALLU_RAND))
	    p_ptr->state.rand_hallu = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_DROP_WEAPON))
	    p_ptr->state.drop_weapon = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_ATTRACT_DEMON))
	    p_ptr->state.attract_demon = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_ATTRACT_UNDEAD))
	    p_ptr->state.attract_undead = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_PARALYZE))
	    p_ptr->state.rand_paral = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_PARALYZE_ALL))
	    p_ptr->state.rand_paral_all = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_DRAIN_EXP))
	    p_ptr->state.drain_exp = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_DRAIN_MANA))
	    p_ptr->state.drain_mana = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_DRAIN_STAT))
	    p_ptr->state.drain_stat = TRUE;
	if (cf_has(o_ptr->flags_curse, CF_DRAIN_CHARGE))
	    p_ptr->state.drain_charge = TRUE;


	for (j = 0; j < MAX_P_RES; j++)
	    apply_resist(&p_ptr->state.res_list[j], o_ptr->percent_res[j]);

	/* Known resistance and immunity flags */
	for (j = 0; j < MAX_P_RES; j++)
	    if (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j))
		apply_resist(&p_ptr->state.dis_res_list[j],
			     o_ptr->percent_res[j]);
	/* End item resistances; do bounds check on resistance levels */
	resistance_limits();

	/* 
	 * Modify the base armor class.   Shields worn on back are penalized. 
	 * Shield and Armor masters benefit.
	 */
	if ((i == INVEN_ARM) && (p_ptr->shield_on_back))
	    temp_armour = o_ptr->ac / 3;
	else if ((i == INVEN_ARM) && (player_has(PF_SHIELD_MAST)))
	    temp_armour = o_ptr->ac * 2;
	else if ((i == INVEN_BODY) && (player_has(PF_ARMOR_MAST)))
	    temp_armour = (o_ptr->ac * 5) / 3;
	else
	    temp_armour = o_ptr->ac;

	p_ptr->state.ac += temp_armour;

	/* The base armor class is always known */
	p_ptr->state.dis_ac += temp_armour;

	/* Apply the bonuses to armor class.  Shields worn on back are
	 * penalized. */
	if ((p_ptr->state.shield_on_back) && (i == INVEN_ARM))
	    temp_armour = o_ptr->to_a / 2;
	else
	    temp_armour = o_ptr->to_a;

	p_ptr->state.to_a += temp_armour;

	/* Apply the mental bonuses to armor class, if known */
	if (if_has(o_ptr->id_other, IF_TO_A))
	    p_ptr->state.dis_to_a += temp_armour;

	/* Hack -- do not apply "weapon" bonuses */
	if (i == INVEN_WIELD)
	    continue;

	/* Hack -- do not apply "bow" bonuses */
	if (i == INVEN_BOW)
	    continue;

	/* Apply the bonuses to hit/damage */
	p_ptr->state.to_h += o_ptr->to_h;
	p_ptr->state.to_d += o_ptr->to_d;

	/* Apply the mental bonuses tp hit/damage, if known */
	if (if_has(o_ptr->id_other, IF_TO_H))
	    p_ptr->state.dis_to_h += o_ptr->to_h;
	if (if_has(o_ptr->id_other, IF_TO_D))
	    p_ptr->state.dis_to_d += o_ptr->to_d;
    }

    /* Hack -- clear a few flags for certain races. */

    /* The dark elf's saving grace */
    if ((player_has(PF_SHADOW)) && (p_ptr->state.aggravate)) {
	p_ptr->state.skills[SKILL_STEALTH] -= 3;
	p_ptr->state.aggravate = FALSE;
    }

    /* Nothing, but nothing, can make an Ent lightfooted. */
    if (player_has(PF_WOODEN))
	p_ptr->state.ffall = FALSE;


  /*** Analyze shapechanges - statistics only ***/
    shape_change_stat();

  /*** (Most) Specialty Abilities ***/

    /* Physical stat boost */
    if (player_has(PF_ATHLETICS)) {
	p_ptr->state.stat_add[A_DEX] += 2;
	p_ptr->state.stat_add[A_CON] += 2;
    }

    /* Mental stat boost */
    if (player_has(PF_CLARITY)) {
	p_ptr->state.stat_add[A_INT] += 2;
	p_ptr->state.stat_add[A_WIS] += 2;
    }

    /* Unlight stealth boost */
    if (player_has(PF_UNLIGHT)) {
	if ((p_ptr->cur_lite <= 0) && (!is_daylight)
	    && !(cave_info[p_ptr->py][p_ptr->px] & CAVE_GLOW))
	    p_ptr->state.skills[SKILL_STEALTH] += 6;
	else
	    p_ptr->state.skills[SKILL_STEALTH] += 3;
    }

    /* Speed Boost (Fury, Phasewalk) */
    if (p_ptr->speed_boost) {
	p_ptr->pspeed += (p_ptr->speed_boost + 5) / 10;
    }

    /* Speed boost in trees for elven druids and rangers */
    if ((player_has(PF_WOODSMAN)) && (player_has(PF_ELVEN))
	&& tf_has(f_info[cave_feat[p_ptr->py][p_ptr->px]].flags, TF_TREE))
	p_ptr->pspeed += 3;

    /* Speed boost for rune of speed */
    if (cave_feat[p_ptr->py][p_ptr->px] == FEAT_RUNE_SPEED)
	p_ptr->pspeed += 10;

    /* Dwarves are good miners */
    if (player_has(PF_DWARVEN))
	p_ptr->state.skills[SKILL_DIGGING] += 40;

  /*** Handle stats ***/

    /* Calculate stats */
    for (i = 0; i < A_MAX; i++) {
	int add, top, use, ind;

	/* Extract modifier */
	add = p_ptr->state.stat_add[i];

	/* Modify the stats for race/class */
	add += (rp_ptr->r_adj[i] + cp_ptr->c_adj[i]);

	/* Extract the new "stat_top" value for the stat */
	top = modify_stat_value(p_ptr->stat_max[i], add);

	/* Save the new value */
	p_ptr->state.stat_top[i] = top;

	/* Extract the new "stat_use" value for the stat */
	use = modify_stat_value(p_ptr->stat_cur[i], add);

	/* Save the new value */
	p_ptr->state.stat_use[i] = use;

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
	p_ptr->state.stat_ind[i] = ind;
    }

    /* Assume no evasion */
    p_ptr->evasion_chance = 0;

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
	max_bonus = adj_dex_evas[p_ptr->state.stat_ind[A_DEX]];

	/* Do we get the max bonus? */
	if (cur_wgt <= ((6 * evasion_wgt) / 10)) {
	    p_ptr->evasion_chance = max_bonus;
	}

	/* Do we get any bonus? */
	else if (cur_wgt <= evasion_wgt) {
	    p_ptr->evasion_chance = max_bonus / 2;
	}
    }


  /*** Temporary flags ***/

    /* Hack - Temporary bonuses are stronger with Enhance Magic */
    if (player_has(PF_ENHANCE_MAGIC))
	enhance = TRUE;

    /* Temporary resists */
    if (p_ptr->state.oppose_acid) {
	int bonus = RES_BOOST_GREAT;
	if (enhance)
	    apply_resist(&bonus, RES_BOOST_MINOR);
	apply_resist(&p_ptr->state.res_list[P_RES_ACID], bonus);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_ACID], bonus);
    }
    if (p_ptr->state.oppose_fire) {
	int bonus = RES_BOOST_GREAT;
	if (enhance)
	    apply_resist(&bonus, RES_BOOST_MINOR);
	apply_resist(&p_ptr->state.res_list[P_RES_FIRE], bonus);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_FIRE], bonus);
    }
    if (p_ptr->state.oppose_cold) {
	int bonus = RES_BOOST_GREAT;
	if (enhance)
	    apply_resist(&bonus, RES_BOOST_MINOR);
	apply_resist(&p_ptr->state.res_list[P_RES_COLD], bonus);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_COLD], bonus);
    }
    if (p_ptr->state.oppose_elec) {
	int bonus = RES_BOOST_GREAT;
	if (enhance)
	    apply_resist(&bonus, RES_BOOST_MINOR);
	apply_resist(&p_ptr->state.res_list[P_RES_ELEC], bonus);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_ELEC], bonus);
    }
    if (p_ptr->state.oppose_pois) {
	int bonus = RES_BOOST_GREAT;
	if (enhance)
	    apply_resist(&bonus, RES_BOOST_MINOR);
	apply_resist(&p_ptr->state.res_list[P_RES_POIS], bonus);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_POIS], bonus);
    }

    /* Apply temporary "stun".  */
    if (p_ptr->timed[TMD_STUN] > 50) {
	p_ptr->state.to_h -= 20;
	p_ptr->state.dis_to_h -= 20;
	p_ptr->state.to_d -= 20;
	p_ptr->state.dis_to_d -= 20;
    } else if (p_ptr->timed[TMD_STUN]) {
	p_ptr->state.to_h -= 5;
	p_ptr->state.dis_to_h -= 5;
	p_ptr->state.to_d -= 5;
	p_ptr->state.dis_to_d -= 5;
    }

    /* Heightened magical defenses.  Saving Throw effect added later */
    if (p_ptr->timed[TMD_INVULN]) {
	int bonus = ((enhance == TRUE) ? 35 : 25);

	p_ptr->state.to_a += bonus;
	p_ptr->state.dis_to_a += bonus;

	apply_resist(&p_ptr->state.res_list[P_RES_CONFU], RES_BOOST_NORMAL);
	apply_resist(&p_ptr->state.dis_res_list[P_RES_CONFU], RES_BOOST_NORMAL);
	p_ptr->state.no_blind = TRUE;
    }

    /* Temporary blessing */
    if (p_ptr->timed[TMD_BLESSED]) {
	int bonus1 = ((enhance == TRUE) ? 10 : 5);
	int bonus2 = ((enhance == TRUE) ? 15 : 10);

	p_ptr->state.to_a += bonus1;
	p_ptr->state.dis_to_a += bonus1;
	p_ptr->state.to_h += bonus2;
	p_ptr->state.dis_to_h += bonus2;
    }

    /* Temporary shield.  Added an exception for Necromancers to keep them in
     * line. */
    if ((p_ptr->timed[TMD_SHIELD]) && (player_has(PF_EVIL))) {
	int bonus = ((enhance == TRUE) ? 50 : 35);

	p_ptr->state.to_a += bonus;
	p_ptr->state.dis_to_a += bonus;
    } else if (p_ptr->timed[TMD_SHIELD]) {
	int bonus = ((enhance == TRUE) ? 65 : 50);

	p_ptr->state.to_a += bonus;
	p_ptr->state.dis_to_a += bonus;
    }

    /* Temporary "Hero".  Now also increases Deadliness. */
    if (p_ptr->timed[TMD_HERO]) {
	int bonus1 = ((enhance == TRUE) ? 15 : 10);
	int bonus2 = ((enhance == TRUE) ? 10 : 5);

	p_ptr->state.to_h += bonus1;
	p_ptr->state.dis_to_h += bonus1;
	p_ptr->state.to_d += bonus2;
	p_ptr->state.dis_to_d += bonus2;
    }

    /* Temporary "Berserk".  Now also increases Deadliness. */
    if (p_ptr->timed[TMD_SHERO]) {
	int bonus1 = ((enhance == TRUE) ? 10 : 5);
	int bonus2 = ((enhance == TRUE) ? 20 : 15);
	int bonus3 = ((enhance == TRUE) ? 15 : 10);

	p_ptr->state.to_h += bonus1;
	p_ptr->state.dis_to_h += bonus1;
	p_ptr->state.to_d += bonus2;
	p_ptr->state.dis_to_d += bonus2;
	p_ptr->state.to_a -= bonus3;
	p_ptr->state.dis_to_a -= bonus3;

	/* but berserkers make *lousy* archers. */
	p_ptr->state.skills[SKILL_TO_HIT_BOW] -= 20;
	p_ptr->state.skills[SKILL_TO_HIT_THROW] -= 20;
    }

    /* Temporary "fast" */
    if (p_ptr->timed[TMD_FAST]) {
	int bonus = ((enhance == TRUE) ? 13 : 10);

	p_ptr->pspeed += bonus;
    }

    /* Temporary "slow" */
    if (p_ptr->timed[TMD_SLOW]) {
	p_ptr->pspeed -= 10;
    }

    /* Temporary see invisible */
    if (p_ptr->timed[TMD_SINVIS]) {
	p_ptr->state.see_inv = TRUE;
    }

    /* Temporary infravision boost.  More useful now. */
    if (p_ptr->timed[TMD_SINFRA]) {
	int bonus = ((enhance == TRUE) ? 5 : 3);

	p_ptr->state.see_infra = p_ptr->state.see_infra + bonus;
    }


  /*** Special flags ***/

    /* Hack -- Hero/Shero -> Res fear */
    if (p_ptr->timed[TMD_HERO] || p_ptr->timed[TMD_SGERO]) {
	p_ptr->state.no_fear = TRUE;
    }

    /* Clear contradictory flags */
    if (p_ptr->state.no_fear)
	p_ptr->state.fear = FALSE;
    if (p_ptr->state.fast_digest && p_ptr->state.slow_digest) {
	p_ptr->state.fast_digest = FALSE;
	p_ptr->state.slow_digest = FALSE;
    }

    if (p_resist_strong(P_RES_POIS)) {
	p_ptr->state.rand_pois = FALSE;
	p_ptr->state.rand_pois_bad = FALSE;
    }

    if (p_resist_good(P_RES_CHAOS))
	p_ptr->state.rand_hallu = FALSE;

    if (p_ptr->state.free_act)
	p_ptr->state.rand_paral = FALSE;

    /* End normal temporary bonuses; do bounds check on resistance levels */
    resistance_limits();

  /*** Analyze weight ***/

    /* Extract the current weight (in tenth pounds) */
    j = p_ptr->total_weight;

    /* Extract the "weight limit" (in tenth pounds) */
    i = weight_limit();

    /* Apply "encumbrance" from weight - more in water, except Maiar, flyers */
    if (j > i / 2) {
	if ((cave_feat[p_ptr->py][p_ptr->px] == FEAT_WATER)
	    && (!player_has(PF_DIVINE)) && !(p_ptr->schange == SHAPE_BAT)
	    && !(p_ptr->schange == SHAPE_WYRM))
	    p_ptr->pspeed -= 3 * ((j - (i / 2)) / (i / 10));
	else
	    p_ptr->pspeed -= ((j - (i / 2)) / (i / 10));
    }

    /* Bloating slows the player down (a little) */
    if (p_ptr->food >= PY_FOOD_MAX)
	p_ptr->pspeed -= 10;

    /* Searching slows the player down */
    if (p_ptr->searching)
	p_ptr->pspeed -= 10;

    /* Sanity check on extreme speeds */
    if (p_ptr->pspeed < 0)
	p_ptr->pspeed = 0;
    if (p_ptr->pspeed > 199)
	p_ptr->pspeed = 199;

  /*** Apply modifier bonuses ***/

    /* Actual Modifier Bonuses (Un-inflate stat bonuses) */
    p_ptr->state.to_a +=
	((int) (adj_dex_ta[p_ptr->state.stat_ind[A_DEX]]) - 128);
    p_ptr->state.to_d +=
	((int) (adj_str_td[p_ptr->state.stat_ind[A_STR]]) - 128);
    p_ptr->state.to_h +=
	((int) (adj_dex_th[p_ptr->state.stat_ind[A_DEX]]) - 128);

    /* Displayed Modifier Bonuses (Un-inflate stat bonuses) */
    p_ptr->state.dis_to_a +=
	((int) (adj_dex_ta[p_ptr->state.stat_ind[A_DEX]]) - 128);
    p_ptr->state.dis_to_d +=
	((int) (adj_str_td[p_ptr->state.stat_ind[A_STR]]) - 128);
    p_ptr->state.dis_to_h +=
	((int) (adj_dex_th[p_ptr->state.stat_ind[A_DEX]]) - 128);


  /*** Modify skills ***/

    /* Affect Skill -- stealth (bonus one) */
    p_ptr->state.skills[SKILL_STEALTH] += 1;

    /* Affect Skill -- disarming (DEX and INT) */
    p_ptr->state.skills[SKILL_DISARM] +=
	adj_dex_dis[p_ptr->state.stat_ind[A_DEX]];
    p_ptr->state.skills[SKILL_DISARM] +=
	adj_int_dis[p_ptr->state.stat_ind[A_INT]];

    /* Affect Skill -- magic devices (INT) */
    p_ptr->state.skills[SKILL_DEVICE] +=
	adj_int_dev[p_ptr->state.stat_ind[A_INT]];

    /* Affect Skill -- saving throw (WIS) */
    p_ptr->state.skills[SKILL_SAVE] +=
	adj_wis_sav[p_ptr->state.stat_ind[A_WIS]];

    /* Affect Skill -- digging (STR) */
    p_ptr->state.skills[SKILL_DIGGING] +=
	adj_str_dig[p_ptr->state.stat_ind[A_STR]];

    /* Affect Skill -- disarming (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_DISARM] += (cp_ptr->cx_dis * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_DISARM] += (rp_ptr->rx_dis * p_ptr->lev / 50);

    /* Affect Skill -- magic devices (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_DEVICE] += (cp_ptr->cx_dev * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_DEVICE] += (rp_ptr->rx_dev * p_ptr->lev / 50);

    /* Affect Skill -- saving throw (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_SAVE] += (cp_ptr->cx_sav * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_SAVE] += (rp_ptr->rx_sav * p_ptr->lev / 50);

    /* Affect Skill -- stealth (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_STEALTH] += (cp_ptr->cx_stl * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_STEALTH] += (rp_ptr->rx_stl * p_ptr->lev / 50);

    /* Affect Skill -- search ability (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_SEARCH] += (cp_ptr->cx_srh * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_SEARCH] += (rp_ptr->rx_srh * p_ptr->lev / 50);

    /* Affect Skill -- search frequency (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] +=
	(cp_ptr->cx_fos * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_SEARCH_FREQUENCY] +=
	(rp_ptr->rx_fos * p_ptr->lev / 50);

    /* Affect Skill -- combat (melee) (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_TO_HIT_MELEE] +=
	(cp_ptr->cx_thn * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_TO_HIT_MELEE] +=
	(rp_ptr->rx_thn * p_ptr->lev / 50);

    /* Affect Skill -- combat (shooting) (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_TO_HIT_BOW] += (cp_ptr->cx_thb * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_TO_HIT_BOW] += (rp_ptr->rx_thb * p_ptr->lev / 50);

    /* Affect Skill -- combat (throwing) (Level, by Class and Race) */
    p_ptr->state.skills[SKILL_TO_HIT_THROW] +=
	(cp_ptr->cx_thb * p_ptr->lev / 50);
    p_ptr->state.skills[SKILL_TO_HIT_THROW] +=
	(rp_ptr->rx_thb * p_ptr->lev / 50);

    /* Limit Skill -- digging from 1 up */
    if (p_ptr->state.skills[SKILL_DIGGING] < 1)
	p_ptr->state.skills[SKILL_DIGGING] = 1;

    /* Limit Skill -- stealth from 0 to 30 */
    if (p_ptr->state.skills[SKILL_STEALTH] > 30)
	p_ptr->state.skills[SKILL_STEALTH] = 30;
    if (p_ptr->state.skills[SKILL_STEALTH] < 0)
	p_ptr->state.skills[SKILL_STEALTH] = 0;

    /* No negative infravision */
    if (p_ptr->state.see_infra < 0)
	p_ptr->state.see_infra = 0;

  /*** Special Saving Throw boosts are calculated after other bonuses ***/

    /* Specialty magic resistance; gives great saving throws even above 100 */
    if (player_has(PF_MAGIC_RESIST)) {
	if (p_ptr->state.skills[SKILL_SAVE] <= 80)
	    p_ptr->state.skills[SKILL_SAVE] +=
		(100 - p_ptr->state.skills[SKILL_SAVE]) / 2;
	else
	    p_ptr->state.skills[SKILL_SAVE] += 10;
    }

    /* Heightened magical defenses.  Halves the difference between saving throw 
     * and 100.  */
    if (p_ptr->magicdef) {
	if (p_ptr->state.skills[SKILL_SAVE] <= 100)
	    p_ptr->state.skills[SKILL_SAVE] +=
		(100 - p_ptr->state.skills[SKILL_SAVE]) / 2;
    }

    /* Rune of Magical Defence */
    if (cave_feat[p_ptr->py][p_ptr->px] == FEAT_RUNE_MAGDEF) {
	if (p_ptr->state.skills[SKILL_SAVE] <= 100)
	    p_ptr->state.skills[SKILL_SAVE] +=
		(100 - p_ptr->state.skills[SKILL_SAVE]) / 2;
    }


  /*** Analyze shapechanges - everything but statistics ***/
    shape_change_main();


    /* Obtain the "hold" value */
    hold = adj_str_hold[p_ptr->state.stat_ind[A_STR]];

  /*** Analyze current bow ***/

    /* Examine the "current bow" */
    o_ptr = &p_ptr->inventory[INVEN_BOW];

    /* Assume not heavy */
    p_ptr->heavy_shoot = FALSE;

    /* It is hard to carry a heavy bow */
    if (hold < o_ptr->weight / 10) {
	/* Hard to wield a heavy bow */
	p_ptr->state.to_h += 2 * (hold - o_ptr->weight / 10);
	p_ptr->state.dis_to_h += 2 * (hold - o_ptr->weight / 10);

	/* Heavy Bow */
	p_ptr->heavy_shoot = TRUE;
    }

    /* Analyze launcher */
    if (o_ptr->k_idx) {
	/* Get to shoot */
	p_ptr->state.num_fire = 10;

	/* Launcher multiplier is now simply their damage dice. */
	p_ptr->state.ammo_mult = o_ptr->dd;


	/* Analyze the launcher */
	switch (o_ptr->sval) {
	    /* Sling and ammo */
	case SV_SLING:
	    {
		p_ptr->state.ammo_tval = TV_SHOT;
		break;
	    }

	    /* Short Bow and Arrow */
	case SV_SHORT_BOW:
	    {
		p_ptr->state.ammo_tval = TV_ARROW;
		break;
	    }

	    /* Long Bow and Arrow */
	case SV_LONG_BOW:
	    {
		p_ptr->state.ammo_tval = TV_ARROW;
		break;
	    }

	    /* Light Crossbow and Bolt */
	case SV_LIGHT_XBOW:
	    {
		p_ptr->state.ammo_tval = TV_BOLT;
		break;
	    }

	    /* Heavy Crossbow and Bolt */
	case SV_HEAVY_XBOW:
	    {
		p_ptr->state.ammo_tval = TV_BOLT;
		break;
	    }
	}

	/* Apply special flags */
	if (o_ptr->k_idx && !p_ptr->heavy_shoot) {
	    /* Dex factor for shot speed */
	    int dex_factor = (adj_dex_shots[p_ptr->state.stat_ind[A_DEX]]);

	    /* Extra shots */
	    p_ptr->state.num_fire += extra_shots * 10;

	    /* Extra might */
	    p_ptr->state.ammo_mult += extra_might;

	    /* Love your launcher SJGU bonuses reduced */
	    if (((player_has(PF_BOW_SPEED_GREAT))
		 && (p_ptr->state.ammo_tval == TV_ARROW))
		|| ((player_has(PF_SLING_SPEED_GREAT))
		    && (p_ptr->state.ammo_tval == TV_SHOT))
		|| ((player_has(PF_XBOW_SPEED_GREAT))
		    && (p_ptr->state.ammo_tval == TV_BOLT))) {
		/* Big bonus... */
		p_ptr->state.num_fire += 3 * dex_factor / 4;

		/* ...and sometimes even more */
		if (player_has(PF_RAPID_FIRE))
		    p_ptr->state.num_fire += dex_factor / 4;
	    }

	    /* Like your launcher */
	    else if (((player_has(PF_BOW_SPEED_GOOD))
		      && (p_ptr->state.ammo_tval == TV_ARROW))
		     || ((player_has(PF_SLING_SPEED_GOOD))
			 && (p_ptr->state.ammo_tval == TV_SHOT))
		     || ((player_has(PF_XBOW_SPEED_GOOD))
			 && (p_ptr->state.ammo_tval == TV_BOLT))) {
		/* Medium bonus */
		p_ptr->state.num_fire += dex_factor / 2;
	    }

	    /* Minimal bonus */
	    else {
		/* Small bonus */
		p_ptr->state.num_fire += dex_factor / 4;
	    }


	    /* See formula "do_cmd_fire" in "attack.c" for Assassin bonus to
	     * Deadliness. */
	}

	/* Require at least one shot per round in real terms */
	if (p_ptr->state.num_fire < 10)
	    p_ptr->state.num_fire = 10;
    }

    /* Add all class and race-specific adjustments to missile Skill. */
    p_ptr->state.skills[SKILL_TO_HIT_BOW] +=
	add_special_missile_skill(p_ptr->pclass, o_ptr->weight, o_ptr);


  /*** Analyze weapon ***/

    /* Examine the "current weapon" */
    o_ptr = &p_ptr->inventory[INVEN_WIELD];

    /* Assume that the player is not a Priest wielding an edged weapon. */
    p_ptr->icky_wield = FALSE;

    /* Assume the weapon is not too heavy */
    p_ptr->heavy_wield = FALSE;

    /* Inflict heavy weapon penalties. */
    if (hold < o_ptr->weight / 10) {
	/* Hard to wield a heavy weapon */
	p_ptr->state.to_h += 2 * (hold - o_ptr->weight / 10);
	p_ptr->state.dis_to_h += 2 * (hold - o_ptr->weight / 10);

	/* Heavy weapon */
	p_ptr->heavy_wield = TRUE;

	/* The player gets to swing a heavy weapon only once. */
	p_ptr->state.num_blow = 1;
    }

    /* Normal weapons */
    if (o_ptr->k_idx && !p_ptr->heavy_wield) {
	int str_index, dex_index;

	int effective_weight = 0, mul = 6;

	/* Enforce a minimum weight of three pounds. */
	effective_weight = (o_ptr->weight < 30 ? 30 : o_ptr->weight);


	/* Compare strength and weapon weight. */
	str_index =
	    mul * adj_str_blow[p_ptr->state.stat_ind[A_STR]] / effective_weight;

	/* Maximal value */
	if (str_index > 11)
	    str_index = 11;


	/* Index by dexterity */
	dex_index = (adj_dex_blow[p_ptr->state.stat_ind[A_DEX]]);

	/* Maximal value */
	if (dex_index > 11)
	    dex_index = 11;


	/* Use the blows table */
	p_ptr->state.num_blow = blows_table[str_index][dex_index];

	/* Paranoia - require at least one blow */
	if (p_ptr->state.num_blow < 1)
	    p_ptr->state.num_blow = 1;


	/* Boost digging skill by weapon weight */
	p_ptr->state.skills[SKILL_DIGGING] += (o_ptr->weight / 10);
    }

    /* Everyone gets 2 to 4 blows if not wielding a weapon. */
    else if (!o_ptr->k_idx)
	p_ptr->state.num_blow = 2 + (p_ptr->lev / 20);


    /* Add all other class and race-specific adjustments to melee Skill. */
    p_ptr->state.skills[SKILL_TO_HIT_MELEE] +=
	add_special_melee_skill(p_ptr->pclass, o_ptr->weight, o_ptr);


  /*** Notice changes ***/

    /* Analyze stats */
    for (i = 0; i < A_MAX; i++) {
	/* Notice changes */
	if (p_ptr->state.stat_top[i] != old_stat_top[i]) {
	    /* Redisplay the stats later */
	    p_ptr->redraw |= (PR_STATS);
	}

	/* Notice changes */
	if (p_ptr->state.stat_use[i] != old_stat_use[i]) {
	    /* Redisplay the stats later */
	    p_ptr->redraw |= (PR_STATS);
	}

	/* Notice changes */
	if (p_ptr->state.stat_ind[i] != old_stat_ind[i]) {
	    /* Change in STR may affect how shields are used. */
	    if ((i == A_STR) && (p_ptr->inventory[INVEN_ARM].k_idx)) {
		/* Access the wield slot */
		o_ptr = &p_ptr->inventory[INVEN_WIELD];

		/* Analyze weapon for two-handed-use. */
		if (of_has(o_ptr->flags_obj, OF_TWO_HANDED_REQ)
		    || (of_has(o_ptr->flags_obj, OF_TWO_HANDED_DES)
			&& (p_ptr->state.stat_ind[A_STR] <
			    29 + (o_ptr->weight / 50 >
				  8 ? 8 : o_ptr->weight / 50)))) {
		    p_ptr->shield_on_back = TRUE;

		} else
		    p_ptr->state.shield_on_back = FALSE;

		/* Hack - recalculate bonuses again. */
		if (p_ptr->state.old_shield_on_back !=
		    p_ptr->state.shield_on_back) {
		    /* do not check strength again */
		    old_stat_ind[i] = p_ptr->state.stat_ind[i];
		    goto calc_again;
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
    if (p_ptr->state.telepathy != old_telepathy) {
	/* Update monster visibility */
	p_ptr->update |= (PU_MONSTERS);
    }

    /* Hack -- See Invis Change */
    if (p_ptr->state.see_inv != old_see_inv) {
	/* Update monster visibility */
	p_ptr->update |= (PU_MONSTERS);
    }

    /* Hack -- See Invis Change */
    if (p_ptr->state.see_infra != old_see_infra) {
	/* Update monster visibility */
	p_ptr->update |= (PU_MONSTERS);
    }

    /* Redraw speed (if needed) */
    if (p_ptr->pspeed != old_speed) {
	/* Redraw speed */
	p_ptr->redraw |= (PR_SPEED);
    }

    /* Recalculate stealth when needed */
    if (p_ptr->state.skills[SKILL_STEALTH] != old_stealth) {
	/* Assume character is extremely noisy. */
	p_ptr->base_wakeup_chance = 100 * WAKEUP_ADJ;

	/* For every increase in stealth past 0, multiply wakeup chance by 0.8 */
	for (i = 0; i < p_ptr->state.skills[SKILL_STEALTH]; i++) {
	    p_ptr->base_wakeup_chance = 4 * p_ptr->base_wakeup_chance / 5;

	    /* Always make at least some innate noise */
	    if (p_ptr->base_wakeup_chance < 100) {
		p_ptr->base_wakeup_chance = 100;
		break;
	    }
	}
    }

    /* Big Hack - make sure base_wakeup_chance has ever been set */
    if (p_ptr->base_wakeup_chance == 0)
	p_ptr->base_wakeup_chance = 100 * WAKEUP_ADJ;

    /* Redraw armor (if needed) */
    if ((p_ptr->state.dis_ac != old_dis_ac)
	|| (p_ptr->state.dis_to_a != old_dis_to_a)) {
	/* Redraw */
	p_ptr->redraw |= (PR_ARMOR);
    }

    /* Hack -- handle "xtra" mode */
    if (character_xtra || inspect)
	return;

    /* Take note when player moves his shield on and off his back. */
    if (p_ptr->evasion_chance != p_ptr->old_evasion_chance) {
	/* Messages */
	if (!p_ptr->old_evasion_chance) {
	    msg_print("You are able to Evade attacks.");
	} else if (!p_ptr->evasion_chance) {
	    msg_print("You are no longer able to Evade attacks");
	}
	/* Mega-Hack - Mask out small changes */
	else if (p_ptr->evasion_chance > (p_ptr->old_evasion_chance + 5)) {
	    msg_print("You are better able to Evade attacks.");
	} else if ((p_ptr->evasion_chance + 5) < p_ptr->old_evasion_chance) {
	    msg_print("You are less able to Evade attacks.");
	}

	/* Save it */
	p_ptr->old_evasion_chance = p_ptr->evasion_chance;
    }

    /* Take note when "heavy bow" changes */
    if (p_ptr->old_heavy_shoot != p_ptr->heavy_shoot) {
	/* Message */
	if (p_ptr->heavy_shoot) {
	    msg_print("You have trouble wielding such a heavy bow.");
	} else if (p_ptr->inventory[INVEN_BOW].k_idx) {
	    msg_print("You have no trouble wielding your bow.");
	} else {
	    msg_print("You feel relieved to put down your heavy bow.");
	}

	/* Save it */
	p_ptr->old_heavy_shoot = p_ptr->heavy_shoot;
    }

    /* Take note when "heavy weapon" changes */
    if (p_ptr->old_heavy_wield != p_ptr->heavy_wield) {
	/* Message */
	if (p_ptr->heavy_wield) {
	    msg_print("You have trouble wielding such a heavy weapon.");
	} else if (p_ptr->inventory[INVEN_WIELD].k_idx) {
	    msg_print("You have no trouble wielding your weapon.");
	} else {
	    msg_print("You feel relieved to put down your heavy weapon.");
	}

	/* Save it */
	p_ptr->old_heavy_wield = p_ptr->heavy_wield;
    }

    /* Take note when "illegal weapon" changes */
    if (p_ptr->old_icky_wield != p_ptr->icky_wield) {
	/* Message */
	if (p_ptr->icky_wield) {
	    msg_print("You do not feel comfortable with your weapon.");
	} else if (p_ptr->inventory[INVEN_WIELD].k_idx) {
	    notice_obj(OF_BLESSED, INVEN_WIELD + 1);
	    msg_print("You feel comfortable with your weapon.");
	} else {
	    msg_print("You feel more comfortable after removing your weapon.");
	}

	/* Save it */
	p_ptr->old_icky_wield = p_ptr->icky_wield;
    }

    /* Take note when player moves his shield on and off his back. */
    if (p_ptr->old_shield_on_back != p_ptr->shield_on_back) {
	/* Messages */
	if (p_ptr->shield_on_back) {
	    msg_print("You are carrying your shield on your back.");
	} else if (p_ptr->inventory[INVEN_ARM].k_idx) {
	    msg_print("You are carrying your shield in your hand.");
	}

	/* No message for players no longer carrying a shield. */

	/* Save it */
	p_ptr->old_shield_on_back = p_ptr->shield_on_back;
    }

    /* Hack - force redraw if stuff has changed */
    if (p_ptr->redraw)
	p_ptr->update |= (PU_BONUS);
}



/**
 * Handle "p_ptr->notice"
 */
void notice_stuff(void)
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
	(void) process_quiver(0, NULL);
    }

    /* Reorder the pack */
    if (p_ptr->notice & (PN_REORDER)) {
	p_ptr->notice &= ~(PN_REORDER);
	reorder_pack();
    }
}


/**
 * Handle "p_ptr->update"
 */
void update_stuff(void)
{
    /* Update stuff */
    if (!p_ptr->update)
	return;


    if (p_ptr->update & (PU_BONUS)) {
	p_ptr->update &= ~(PU_BONUS);
	calc_bonuses(FALSE);
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
	verify_panel();
    }
}


/*
 * Events triggered by the various flags.
 */
static const struct flag_event_trigger redraw_events[] =
{
	{ PR_MISC,    EVENT_RACE_CLASS },
	{ PR_TITLE,   EVENT_PLAYERTITLE },
	{ PR_LEV,     EVENT_PLAYERLEVEL },
	{ PR_EXP,     EVENT_EXPERIENCE },
	{ PR_STATS,   EVENT_STATS },
	{ PR_ARMOR,   EVENT_AC },
	{ PR_HP,      EVENT_HP },
	{ PR_MANA,    EVENT_MANA },
	{ PR_GOLD,    EVENT_GOLD },
	{ PR_HEALTH,  EVENT_MONSTERHEALTH },
	{ PR_MON_MANA, EVENT_MONSTERMANA },
	{ PR_DEPTH,   EVENT_DUNGEONLEVEL },
	{ PR_SPEED,   EVENT_PLAYERSPEED },
	{ PR_SHAPE,   EVENT_SHAPECHANGE },
	{ PR_STATE,   EVENT_STATE },
	{ PR_STATUS,  EVENT_STATUS },
	{ PR_STUDY,   EVENT_STUDYSTATUS },
	{ PR_DTRAP,   EVENT_DETECTIONSTATUS },
	{ PR_BUTTONS, EVENT_MOUSEBUTTONS },

	{ PR_INVEN,   EVENT_INVENTORY },
	{ PR_EQUIP,   EVENT_EQUIPMENT },
	{ PR_MONLIST, EVENT_MONSTERLIST },
	{ PR_ITEMLIST, EVENT_ITEMLIST },
	{ PR_MONSTER, EVENT_MONSTERTARGET },
	{ PR_OBJECT, EVENT_OBJECTTARGET },
	{ PR_MESSAGE, EVENT_MESSAGE },
};

/**
 * Handle "p_ptr->redraw"
 */
void redraw_stuff(void)
{
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
    for (i = 0; i < N_ELEMENTS(redraw_events); i++)
    {
	const struct flag_event_trigger *hnd = &redraw_events[i];
	
	if (p_ptr->redraw & hnd->flag)
	    event_signal(hnd->event);
    }

    /* Then the ones that require parameters to be supplied. */
    if (p_ptr->redraw & PR_MAP) 
    {
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
void handle_stuff(void)
{
	/* Update stuff */
	if (p_ptr->update) update_stuff();

	/* Redraw stuff */
	if (p_ptr->redraw) redraw_stuff();
}
