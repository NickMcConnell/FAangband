/*
 * File: trap.c
 * Purpose: Trap triggering, selection, and placement
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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

/**
 * Hack -- instantiate a trap
 *
 * This function has been altered in Oangband to (in a hard-coded fashion) 
 * control trap level. -LM-
 */
void pick_trap(int y, int x)
{
    int feat = 0;
    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    bool trap_is_okay = FALSE;

    /* Paranoia */
    if ((cave_feat[y][x] != FEAT_INVIS) && (cave_feat[y][x] != FEAT_GRASS_INVIS)
	&& (cave_feat[y][x] != FEAT_TREE_INVIS)
	&& (cave_feat[y][x] != FEAT_TREE2_INVIS))
	return;

    /* Try to create a trap appropriate to the level.  Make certain that at
     * least one trap type can be made on any possible level. -LM- */
    while (!(trap_is_okay)) {
	/* Pick at random. */
	feat = FEAT_TRAP_HEAD + randint0(16);

	/* Assume legal until proven otherwise. */
	trap_is_okay = TRUE;

	/* Check legality. */
	switch (feat) {
	    /* trap doors */
	case FEAT_TRAP_HEAD + 0x00:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* Hack -- no trap doors on quest levels */
		if (is_quest(p_ptr->stage))
		    trap_is_okay = FALSE;

		/* Hack -- no trap doors at the bottom of dungeons */
		if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
		    && (!stage_map[p_ptr->stage][DOWN]))
		    trap_is_okay = FALSE;

		/* No trap doors at level 1 (instadeath risk). */
		if (p_ptr->depth < 2)
		    trap_is_okay = FALSE;

		/* Trap doors only in dungeons or normal wilderness */
		if (stage_map[p_ptr->stage][STAGE_TYPE] > CAVE)
		    trap_is_okay = FALSE;

		/* Trap doors only in dungeons in ironman */
		if (OPT(adult_ironman)
		    && (stage_map[p_ptr->stage][STAGE_TYPE] < CAVE))
		    trap_is_okay = FALSE;

		break;
	    }

	    /* pits */
	case FEAT_TRAP_HEAD + 0x01:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No pits at level 1 (instadeath risk). */
		if (p_ptr->depth < 2)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* stat-reducing darts. */
	case FEAT_TRAP_HEAD + 0x02:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No darts above level 8. */
		if (p_ptr->depth < 8)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* discolored spots */
	case FEAT_TRAP_HEAD + 0x03:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No discolored spots above level 5. */
		if (p_ptr->depth < 5)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* gas trap */
	case FEAT_TRAP_HEAD + 0x04:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* Gas traps can exist anywhere. */

		break;
	    }

	    /* summoning rune */
	case FEAT_TRAP_HEAD + 0x05:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No summoning runes above level 3. */
		if (p_ptr->depth < 3)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* dungeon alteration */
	case FEAT_TRAP_HEAD + 0x06:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No dungeon alteration above level 20. */
		if (p_ptr->depth < 20)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* alter char and reality */
	case FEAT_TRAP_HEAD + 0x07:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No alter char and reality above level 60. */
		if (p_ptr->depth < 60)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* move player */
	case FEAT_TRAP_HEAD + 0x08:
	    {

		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		break;
	    }

	    /* arrow, bolt, or shot */
	case FEAT_TRAP_HEAD + 0x09:
	    {
		/* No trees */
		if (tf_has(f_ptr->flags, TF_TREE))
		    trap_is_okay = FALSE;

		/* No arrow, bolt, or shots above level 4. */
		if (p_ptr->depth < 4)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* falling tree branch */
	case FEAT_TRAP_HEAD + 0x0A:
	    {
		/* Need the right trees */
		if (cave_feat[y][x] != FEAT_TREE_INVIS)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* falling tree branch */
	case FEAT_TRAP_HEAD + 0x0B:
	    {
		/* Need the right trees */
		if (cave_feat[y][x] != FEAT_TREE2_INVIS)
		    trap_is_okay = FALSE;

		break;
	    }

	    /* Any other selection is not defined. */
	default:
	    {
		trap_is_okay = FALSE;

		break;
	    }
	}
    }

    /* Activate the trap */
    cave_set_feat(y, x, feat);
}



/**
 * Places a random trap at the given location.
 *
 * The location must be a legal, naked, floor grid.
 *
 * Note that all traps start out as "invisible" and "untyped", and then
 * when they are "discovered" (by detecting them or setting them off),
 * the trap is "instantiated" as a visible, "typed", trap.
 */
void place_trap(int y, int x)
{
    int d, grass = 0, floor = 0;

    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    /* Paranoia */
    if (!in_bounds(y, x))
	return;

    /* Hack - handle trees */
    if ((tf_has(f_ptr->flags, TF_TREE)) && (cave_o_idx[y][x] == 0)
	&& (cave_m_idx[y][x] >= 0)) {
	if (cave_feat[y][x] == FEAT_TREE)
	    cave_set_feat(y, x, FEAT_TREE_INVIS);
	else if (cave_feat[y][x] == FEAT_TREE2)
	    cave_set_feat(y, x, FEAT_TREE2_INVIS);
	return;
    }

    /* Require empty, clean, floor grid */
    if (!cave_naked_bold(y, x))
	return;

    /* Adjacent grids vote for grass or floor */
    for (d = 0; d < 8; d++) {
	if (cave_feat[y + ddy_ddd[d]][x + ddx_ddd[d]] == FEAT_FLOOR)
	    floor++;
	else if (cave_feat[y + ddy_ddd[d]][x + ddx_ddd[d]] == FEAT_GRASS)
	    grass++;
    }

    /* Place an invisible trap */
    if (grass > floor)
	cave_set_feat(y, x, FEAT_GRASS_INVIS);
    else
	cave_set_feat(y, x, FEAT_INVIS);

}

/**
 * Determine if a trap affects the player.
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_trap_hit(int power)
{
    int k, ac;
    bool hit = FALSE;

    /* This function is called from the trap attack code, which generally uses
     * the simple RNG.  We temporarily switch over to the complex RNG for true
     * randomness. - LM- */
    Rand_quick = FALSE;

    /* Percentile dice */
    k = randint0(100);

    /* 5% minimum chance to hit, 5% minimum chance to miss */
    if (k < 10)
	hit = (k < 5);

    /* Total armor */
    ac = p_ptr->ac + p_ptr->to_a;

    /* Power competes against Armor */
    if ((power > 0) && (randint1(power) >= (ac * 3 / 4)))
	hit = TRUE;

    /* Resume use of the simple RNG. */
    Rand_quick = TRUE;

    /* Return hit or miss. */
    return (hit);

}



/**
 * Handle player hitting a real trap.  Rewritten in Oangband to allow a 
 * greater variety of traps, with effects controlled by dungeon level.  
 * To allow a trap to choose one of a variety of effects consistantly, 
 * the quick RNG is often used, and xy coordinates input as a seed value. 
 */
void hit_trap(int y, int x)
{
    int i, j, k, num;
    int dam = 0;

    int nastyness, selection;

    feature_type *f_ptr = &f_info[cave_feat[y][x]];

    cptr name = f_name + f_ptr->name;

    /* Use the "simple" RNG to insure that traps are consistant. */
    Rand_quick = TRUE;

    /* Use the coordinates of the trap to seed the RNG. */
    Rand_value = y * x;

    /* Disturb the player */
    disturb(0, 0);

    /* Analyze XXX XXX XXX */
    switch (cave_feat[y][x]) {
	/* trap door. */
    case FEAT_TRAP_HEAD + 0x00:
	{
	    Rand_quick = FALSE;

	    /* Paranoia -NRM- */
	    if (((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
		 || (stage_map[p_ptr->stage][STAGE_TYPE] == VALLEY))
		&& (!stage_map[p_ptr->stage][DOWN])) {
		cave_info[y][x] &= ~(CAVE_MARK);
		cave_set_feat(y, x, FEAT_FLOOR);
		msg_print("The trap fails!");
		break;
	    }


	    msg_print("You fall through a trap door!");
	    if (p_ptr->ffall) {
		notice_obj(OF_FEATHER, 0);
		msg_print("You float gently down to the next level.");
	    } else {
		dam = damroll(2, 8);
		take_hit(dam, name);
	    }
	    /* Remember where we came from */
	    p_ptr->last_stage = p_ptr->stage;

	    if (!stage_map[p_ptr->stage][DOWN]) {
		/* Set the ways forward and back */
		stage_map[255][UP] = p_ptr->stage;
		stage_map[p_ptr->stage][DOWN] = 255;
		stage_map[255][DEPTH] = p_ptr->depth + 1;
	    }

	    /* New stage */
	    p_ptr->stage = stage_map[p_ptr->stage][DOWN];

	    /* New depth */
	    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	    /* Leaving */
	    p_ptr->leaving = TRUE;

	    Rand_quick = TRUE;

	    break;
	}

	/* pits. */
    case FEAT_TRAP_HEAD + 0x01:
	{
	    /* determine how dangerous the trap is allowed to be. */
	    nastyness = randint1(p_ptr->depth);
	    if (randint1(20) == 1)
		nastyness += 20;
	    else if (randint1(5) == 1)
		nastyness += 10;

	    /* Player is now in pit. */
	    monster_swap(p_ptr->py, p_ptr->px, y, x);

	    /* Center on player. */
	    y = p_ptr->py;
	    x = p_ptr->px;

	    /* pit of daggers. */
	    if ((nastyness > 80) && (randint1(3) != 3)) {
		msg_print("You fall into a pit of daggers!");

		if (p_ptr->ffall) {
		    notice_obj(OF_FEATHER, 0);
		    msg_print("You float gently to the floor of the pit.");
		    msg_print("You carefully avoid setting off the daggers.");
		}

		else {
		    /* a trap of morgul. */
		    if (randint1(6) == 1) {
			Rand_quick = FALSE;


			msg_print
			    ("A single coldly gleaming dagger pierces you deeply!");
			msg_print
			    ("You feel a deadly chill slowly withering your soul.");

			/* activate the Black Breath. */
			p_ptr->black_breath = TRUE;

			/* lots of damage. */
			dam = damroll(20, 15);

			/* undead may be attracted. */
			if (randint1(2) == 1) {
			    msg_print
				("Undead suddenly appear and call you to them!");

			    k = randint1(3) + 2;
			    for (i = 0; i < k; i++) {
				summon_specific(y, x, FALSE, p_ptr->depth,
						SUMMON_UNDEAD);
			    }
			}

			/* morgul-traps are one-time only. */
			cave_info[y][x] &= ~(CAVE_MARK);
			cave_set_feat(y, x, FEAT_FLOOR);

			Rand_quick = TRUE;
		    }

		    else {
			Rand_quick = FALSE;

			/* activate the ordinary daggers. */
			msg_print("Daggers pierce you everywhere!");

			k = randint1(10) + 5;
			for (i = 0; i < k; i++) {
			    dam += damroll(3, 4);
			}

			Rand_quick = TRUE;
		    }

		    /* cut the player. */
		    (void) inc_timed(TMD_CUT, randint1(dam), TRUE);

		    /* Take the damage. */
		    take_hit(dam, name);
		}
	    }

	    /* poisoned spiked pit. */
	    else if ((nastyness > 55) && (randint1(3) != 3)) {
		msg_print("You fall into a spiked pit!");

		if (p_ptr->ffall) {
		    notice_obj(OF_FEATHER, 0);
		    msg_print("You float gently to the floor of the pit.");
		    msg_print("You carefully avoid touching the spikes.");
		}

		else {
		    Rand_quick = FALSE;

		    /* Base damage */
		    dam = damroll(2, 6);

		    /* Extra spike damage */
		    if (randint0(100) < 85) {
			bool was_poisoned;

			msg_print("You are impaled on poisonous spikes!");

			dam = dam * (randint1(6) + 3);
			(void) inc_timed(TMD_CUT, randint1(dam), TRUE);

			was_poisoned = pois_hit(dam);

			if (!was_poisoned)
			    msg_print("The poison does not affect you!");
		    }

		    /* Take the damage */
		    take_hit(dam, name);

		    Rand_quick = TRUE;
		}
	    }

	    /* spiked pit. */
	    else if ((nastyness > 30) && (randint1(3) != 3)) {
		msg_print("You fall into a spiked pit!");

		if (p_ptr->ffall) {
		    notice_obj(OF_FEATHER, 0);
		    msg_print("You float gently to the floor of the pit.");
		    msg_print("You carefully avoid touching the spikes.");
		}

		else {
		    Rand_quick = FALSE;

		    /* Base damage */
		    dam = damroll(2, 6);

		    /* Extra spike damage */
		    if (randint0(100) < 85) {
			msg_print("You are impaled!");

			dam = dam * (2 + randint1(4));
			(void) inc_timed(TMD_CUT, randint1(dam), TRUE);
		    }

		    /* Take the damage */
		    take_hit(dam, name);

		    Rand_quick = TRUE;
		}
	    }

	    /* ordinary pit in all other cases. */
	    else {
		msg_print("You fall into a pit!");
		if (p_ptr->ffall) {
		    notice_obj(OF_FEATHER, 0);
		    msg_print("You float gently to the bottom of the pit.");
		} else {
		    Rand_quick = FALSE;

		    dam = damroll(2, 6);
		    take_hit(dam, name);

		    Rand_quick = TRUE;
		}
	    }

	    break;
	}

	/* stat-reducing dart traps. */
    case FEAT_TRAP_HEAD + 0x02:
	{
	    /* decide if the dart hits. */
	    if (check_trap_hit(50 + p_ptr->depth)) {
		/* select a stat to drain. */
		selection = randint0(6);

		Rand_quick = FALSE;

		msg_print("A small dart hits you!");
		dam = damroll(1, 4);
		take_hit(dam, name);

		/* Determine how dangerous the trap is allowed to be. */
		nastyness = randint1(p_ptr->depth);

		/* decide how much to drain the stat by. */
		if ((nastyness > 50) && (randint1(3) == 1)) {
		    num = randint1(4);
		} else
		    num = 1;

		/* drain the stat. */
		for (i = 0; i < num; i++) {
		    (void) do_dec_stat(selection);
		}

		Rand_quick = TRUE;
	    } else {
		msg_print("A small dart barely misses you.");
	    }
	    break;
	}

	/* discolored spots. */
    case FEAT_TRAP_HEAD + 0x03:
	{
	    /* determine how dangerous the trap is allowed to be. */
	    nastyness = randint1(p_ptr->depth);
	    if (randint1(5) == 1)
		nastyness += 10;

	    /* pick a elemental attack type. */
	    selection = randint1(4);


	    /* electicity trap. */
	    if (selection == 1) {
		if ((nastyness >= 50) && (randint1(2) == 1)) {
		    Rand_quick = FALSE;

		    msg_print("You are struck by lightning!");
		    dam = damroll(6, 30);

		    Rand_quick = TRUE;
		} else {
		    Rand_quick = FALSE;

		    msg_print("You get zapped!");
		    dam = damroll(4, 8);

		    Rand_quick = TRUE;
		}
		Rand_quick = FALSE;
		elec_dam(dam, "an electricity trap");
		Rand_quick = TRUE;

	    }

	    /* frost trap. */
	    if (selection == 2) {
		if ((nastyness >= 50) && (randint1(2) == 1)) {
		    Rand_quick = FALSE;

		    msg_print("You are lost within a blizzard!");
		    dam = damroll(6, 30);

		    Rand_quick = TRUE;
		} else {
		    Rand_quick = FALSE;

		    msg_print("You are coated in frost!");
		    dam = damroll(4, 8);

		    Rand_quick = TRUE;
		}
		Rand_quick = FALSE;
		cold_dam(dam, "a frost trap");
		Rand_quick = TRUE;
	    }

	    /* fire trap. */
	    if (selection == 3) {
		if ((nastyness >= 50) && (randint1(2) == 1)) {
		    Rand_quick = FALSE;

		    msg_print("You are enveloped in a column of fire!");
		    dam = damroll(6, 30);

		    Rand_quick = TRUE;
		} else {
		    Rand_quick = FALSE;

		    msg_print("You are surrounded by flames!");
		    dam = damroll(4, 8);

		    Rand_quick = TRUE;
		}
		Rand_quick = FALSE;
		fire_dam(dam, "a fire trap");
		Rand_quick = TRUE;
	    }

	    /* acid trap. */
	    if (selection == 4) {
		if ((nastyness >= 50) && (randint1(2) == 1)) {
		    Rand_quick = FALSE;

		    msg_print("A cauldron of acid is tipped over your head!");
		    dam = damroll(6, 30);

		    Rand_quick = TRUE;
		} else {
		    Rand_quick = FALSE;

		    msg_print("You are splashed with acid!");
		    dam = damroll(4, 8);

		    Rand_quick = TRUE;
		}
		Rand_quick = FALSE;
		acid_dam(dam, "an acid trap");
		Rand_quick = TRUE;
	    }

	    break;
	}

	/* gas traps. */
    case FEAT_TRAP_HEAD + 0x04:
	{
	    selection = randint1(4);

	    /* blinding trap. */
	    if (selection == 1) {
		msg_print("You are surrounded by a black gas!");
		if (!p_ptr->state.no_blind) {
		    Rand_quick = FALSE;

		    (void) inc_timed(TMD_BLIND, randint0(30) + 15, TRUE);

		    Rand_quick = TRUE;
		}
	    } else
		notice_obj(OF_SEEING, 0);

	    /* confusing trap. */
	    if (selection == 2) {
		msg_print
		    ("You are surrounded by a gas of scintillating colors!");
		if (!p_resist_good(P_RES_CONFU)) {
		    Rand_quick = FALSE;

		    (void) inc_timed(TMD_CONFUSED, randint0(20) + 10, TRUE);

		    Rand_quick = TRUE;
		} else
		    notice_other(IF_RES_CONFU, 0);
	    }

	    /* poisoning trap. */
	    if (selection == 3) {
		msg_print("You are surrounded by a pungent green gas!");

		Rand_quick = FALSE;

		pois_hit(25);

		Rand_quick = TRUE;
	    }

	    /* sleeping trap. */
	    if (selection == 4) {
		msg_print("You are surrounded by a strange white mist!");
		if (!p_ptr->free_act) {
		    (void) inc_timed(TMD_PARALYZED, randint0(10) + 5, TRUE);
		} else
		    notice_obj(OF_FREE_ACT, 0);
	    }

	    break;
	}

	/* summoning traps. */
    case FEAT_TRAP_HEAD + 0x05:
	{
	    sound(MSG_SUM_MONSTER);
	    /* sometimes summon thieves. */
	    if ((p_ptr->depth > 8) && (randint1(5) == 1)) {
		msg_print("You have aroused a den of thieves!");

		Rand_quick = FALSE;

		num = 2 + randint1(3);
		for (i = 0; i < num; i++) {
		    (void) summon_specific(y, x, FALSE, p_ptr->depth,
					   SUMMON_THIEF);
		}

		Rand_quick = TRUE;
	    }

	    /* sometimes summon a nasty unique. */
	    else if (randint1(8) == 1) {
		msg_print("You are enveloped in a cloud of smoke!");

		Rand_quick = FALSE;

		(void) summon_specific(y, x, FALSE, p_ptr->depth + 5,
				       SUMMON_UNIQUE);

		Rand_quick = TRUE;
	    }

	    /* otherwise, the ordinary summon monsters. */
	    else {
		msg_print("You are enveloped in a cloud of smoke!");

		Rand_quick = FALSE;

		num = 2 + randint1(3);
		for (i = 0; i < num; i++) {
		    (void) summon_specific(y, x, FALSE, p_ptr->depth, 0);
		}

		Rand_quick = TRUE;
	    }

	    /* these are all one-time traps. */
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_FLOOR);

	    break;
	}

	/* dungeon alteration traps. */
    case FEAT_TRAP_HEAD + 0x06:
	{
	    /* determine how dangerous the trap is allowed to be. */
	    nastyness = randint1(p_ptr->depth);
	    if (randint1(5) == 1)
		nastyness += 10;

	    /* make room for alterations. */
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_FLOOR);

	    /* Everything truely random from here on. */
	    Rand_quick = FALSE;

	    /* dungeon destruction trap. */
	    if ((nastyness > 60) && (randint1(12) == 1)) {
		msg_print
		    ("A ear-splitting howl shatters your mind as the dungeon is smashed by hammer blows!");

		(void) destroy_level(FALSE);

		/* the player is hard-hit. */
		(void) inc_timed(TMD_CONFUSED, randint0(20) + 10, TRUE);
		(void) inc_timed(TMD_BLIND, randint0(30) + 15, TRUE);
		(void) inc_timed(TMD_STUN, randint1(50) + 50, TRUE);
		dam = damroll(15, 15);
		take_hit(dam, name);
	    }

	    /* earthquake trap. */
	    else if ((nastyness > 20) && (randint1(4) == 1)) {
		msg_print("A tremor shakes the earth around you");
		earthquake(y, x, 10, FALSE);
	    }

	    /* falling rock trap. */
	    else if ((nastyness > 4) && (randint1(2) == 1)) {
		msg_print("A rock falls on your head.");
		dam = damroll(2, 10);
		take_hit(dam, name);

		(void) inc_timed(TMD_STUN, randint1(10) + 10, TRUE);
	    }

	    /* a few pebbles. */
	    else {
		msg_print("A bunch of pebbles rain down on you.");
		dam = damroll(1, 8);
		take_hit(dam, name);
	    }

	    Rand_quick = TRUE;

	    break;
	}

	/* various char and equipment-alteration traps, lumped together to
	 * avoid any one effect being too common (some of them can be rather
	 * nasty). */
    case FEAT_TRAP_HEAD + 0x07:
	{
	    /* determine how dangerous the trap is allowed to be. */
	    nastyness = randint0(100);

	    /* these are all one-time traps. */
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_FLOOR);

	    /* Everything truely random from here on. */
	    Rand_quick = FALSE;

	    /* trap of drain wands. */
	    if (nastyness < 15) {
		/* Hold the object information. */
		object_type *o_ptr;

		/* Find an item */
		for (i = 0; i < 20; i++) {
		    /* Pick an item */
		    i = randint0(INVEN_PACK - p_ptr->pack_size_reduce);

		    /* Obtain the item */
		    o_ptr = &p_ptr->inventory[i];

		    /* use "num" to decide if a item can be uncharged.  By
		     * default, assume it can't. */
		    num = 0;

		    /* Skip non-objects */
		    if (!o_ptr->k_idx)
			continue;

		    /* Drain charged wands/staffs/rods */
		    if ((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND)
			|| (o_ptr->tval == TV_ROD)) {
			/* case of charged wands/staffs. */
			if (((o_ptr->tval == TV_STAFF)
			     || (o_ptr->tval == TV_WAND)) && (o_ptr->pval))
			    num = 1;

			/* case of charged rods. */
			if ((o_ptr->tval == TV_ROD)
			    && (o_ptr->timeout < (o_ptr->pval * o_ptr->number)))
			    num = 1;


			if (num == 1) {
			    /* Message */
			    msg_print("Energy drains from your pack!");

			    /* Uncharge */
			    if ((o_ptr->tval == TV_STAFF)
				|| (o_ptr->tval == TV_WAND))
				o_ptr->pval = 0;

			    if (o_ptr->tval == TV_ROD)
				o_ptr->timeout =
				    o_ptr->pval * o_ptr->number * 2;


			    /* Combine / Reorder the pack */
			    p_ptr->notice |= (PN_COMBINE | PN_REORDER);

			    /* not more than one inventory slot effected. */
			    break;
			} else
			    continue;
		    }
		}
	    }

	    /* trap of forgetting. */
	    else if (nastyness < 35) {
		if (check_save(100)) {
		    msg_print("You hang on to your memories!");
		} else if (lose_all_info()) {
		    msg_print("Your memories fade away.");
		}
	    }

	    /* trap of alter reality. */
	    else if (nastyness < 50) {
		if (OPT(adult_ironman))
		    msg_print("Nothing happens.");
		else {
		    msg_print("The world changes!");

		    /* Leaving */
		    p_ptr->leaving = TRUE;
		}
	    }

	    /* trap of remold player. */
	    else if (nastyness < 75) {
		int max1, cur1, max2, cur2, ii, jj;

		msg_print("You feel yourself being twisted by wild magic!");

		if (check_save(100)) {
		    msg_print("You resist the effects!");
		} else {
		    msg_print("Your body starts to scramble...");

		    /* Pick a pair of stats */
		    ii = randint0(6);
		    for (jj = ii; jj == ii; jj = randint0(6))	/* loop */
			;

		    max1 = p_ptr->stat_max[ii];
		    cur1 = p_ptr->stat_cur[ii];
		    max2 = p_ptr->stat_max[jj];
		    cur2 = p_ptr->stat_cur[jj];

		    p_ptr->stat_max[ii] = max2;
		    p_ptr->stat_cur[ii] = cur2;
		    p_ptr->stat_max[jj] = max1;
		    p_ptr->stat_cur[jj] = cur1;

		    p_ptr->update |= (PU_BONUS);
		}
	    }

	    /* time ball trap. */
	    else if (nastyness < 90) {
		msg_print("You feel time itself assault you!");

		/* Target the player with a radius 0 ball attack. */
		fire_meteor(0, GF_TIME, p_ptr->py, p_ptr->px, 75, 0, TRUE);
	    }

	    /* trap of bugs gone berserk. */
	    else {
		/* explain what the dickens is going on. */
		msg_print("GRUESOME Gnawing Bugs leap out at you!");

		if (!p_resist_good(P_RES_CONFU)) {
		    (void) inc_timed(TMD_CONFUSED, randint0(20) + 10, TRUE);
		} else
		    notice_other(IF_RES_CONFU, 0);

		if (!p_resist_good(P_RES_CHAOS)) {
		    (void) inc_timed(TMD_IMAGE, randint1(40), TRUE);
		} else
		    notice_other(IF_RES_CHAOS, 0);

		/* XXX (hard coded) summon 3-6 bugs. */
		k = randint1(4) + 2;
		for (i = 0; i < k; ++i) {
		    /* Look for a location */
		    for (j = 0; j < 20; ++j) {
			/* Pick a (scattered) distance. */
			int d = (j / 10) + randint1(3);

			/* Pick a location */
			scatter(&y, &x, y, x, d, 0);

			/* Require passable terrain */
			if (!cave_passable_bold(y, x))
			    continue;

			/* Hack -- no summon on glyph of warding */
			if (cave_feat[y][x] == FEAT_RUNE_PROTECT)
			    continue;

			/* Okay */
			break;
		    }

		    /* Attempt to place the awake bug */
		    place_monster_aux(y, x, 453, FALSE, TRUE);
		}

		/* herald the arrival of bugs. */
		msg_print("AAAAAAAHHHH! THEY'RE EVERYWHERE!");
	    }

	    Rand_quick = TRUE;

	    break;
	}

	/* teleport trap */
    case FEAT_TRAP_HEAD + 0x08:
	{
	    if (stage_map[p_ptr->stage][STAGE_TYPE] >= CAVE)
		msg_print("You teleport across the dungeon.");
	    else
		msg_print("You teleport across the wilderness.");

	    Rand_quick = FALSE;

	    teleport_player(250, FALSE);

	    Rand_quick = TRUE;

	    break;
	}

	/* murder holes. */
    case FEAT_TRAP_HEAD + 0x09:
	{
	    /* hold the object info. */
	    object_type *o_ptr;
	    object_type object_type_body;

	    /* hold the missile type and name. */
	    int sval = 0;
	    int tval = 0;
	    cptr missile_name = "";



	    /* Determine the missile type and base damage. */
	    if (randint1(3) == 1) {
		if (p_ptr->depth < 40) {
		    missile_name = "shot";
		    dam = damroll(2, 3);
		    tval = TV_SHOT;
		    sval = SV_AMMO_NORMAL;
		} else {
		    missile_name = "seeker shot";
		    dam = damroll(3, 7);
		    tval = TV_SHOT;
		    sval = SV_AMMO_HEAVY;
		}
	    }

	    else if (randint1(2) == 1) {
		if (p_ptr->depth < 55) {
		    missile_name = "arrow";
		    dam = damroll(2, 4);
		    tval = TV_ARROW;
		    sval = SV_AMMO_NORMAL;
		} else {
		    missile_name = "seeker arrow";
		    dam = damroll(3, 9);
		    tval = TV_ARROW;
		    sval = SV_AMMO_HEAVY;
		}
	    }

	    else {
		if (p_ptr->depth < 65) {
		    missile_name = "bolt";
		    dam = damroll(2, 5);
		    tval = TV_BOLT;
		    sval = SV_AMMO_NORMAL;
		} else {
		    missile_name = "seeker bolt";
		    dam = damroll(3, 11);
		    tval = TV_BOLT;
		    sval = SV_AMMO_HEAVY;
		}
	    }

	    /* determine if the missile hits. */
	    if (check_trap_hit(75 + p_ptr->depth)) {
		msg_format("A %s hits you from above.", missile_name);

		Rand_quick = FALSE;

		/* critical hits. */
		if (randint1(2) == 1) {
		    msg_print("It was well-aimed!");
		    dam *= 1 + randint1(2);
		}
		if (randint1(2) == 1) {
		    msg_print("It gouges you!");
		    dam = 3 * dam / 2;

		    /* cut the player. */
		    (void) inc_timed(TMD_CUT, randint1(dam), TRUE);
		}

		Rand_quick = TRUE;

		take_hit(dam, name);
	    }

	    /* Explain what just happened. */
	    else
		msg_format("A %s wizzes by your head.", missile_name);

	    /* these will eventually run out of ammo. */

	    Rand_quick = FALSE;

	    if (randint0(8) == 0) {
		cave_info[y][x] &= ~(CAVE_MARK);
		cave_set_feat(y, x, FEAT_FLOOR);
	    }

	    Rand_quick = TRUE;

	    /* Get local object */
	    o_ptr = &object_type_body;

	    /* Make a missile, identify it, and drop it near the player. */
	    object_prep(o_ptr, lookup_kind(tval, sval));
	    object_aware(o_ptr);
	    object_known(o_ptr);
	    drop_near(o_ptr, -1, y, x);

	    break;
	}

	/* falling tree branch */
    case FEAT_TRAP_HEAD + 0x0A:
	{
	    /* determine if the missile hits. */
	    if (check_trap_hit(75 + p_ptr->depth)) {
		/* Take damage */
		dam = damroll(3, 5);
		msg_print("A branch hits you from above.");

		Rand_quick = FALSE;

		/* critical hits. */
		if (randint1(2) == 1) {
		    msg_print("It was heavy!");
		    dam = 3 * dam / 2;

		    /* stun the player. */
		    (void) inc_timed(TMD_STUN, randint1(dam), TRUE);
		}

		Rand_quick = TRUE;

		take_hit(dam, name);
	    }

	    /* Explain what just happened. */
	    else
		msg_print("A falling branch just misses you.");

	    /* No more */
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_TREE);

	    break;
	}

	/* falling tree branch */
    case FEAT_TRAP_HEAD + 0x0B:
	{
	    /* determine if the missile hits. */
	    if (check_trap_hit(75 + p_ptr->depth)) {
		/* Take damage */
		dam = damroll(3, 5);
		msg_print("A branch hits you from above.");

		Rand_quick = FALSE;

		/* critical hits. */
		if (randint1(2) == 1) {
		    msg_print("It was heavy!");
		    dam = 3 * dam / 2;

		    /* stun the player. */
		    (void) inc_timed(TMD_STUN, randint1(dam), TRUE);
		}

		Rand_quick = TRUE;

		take_hit(dam, name);
	    }

	    /* Explain what just happened. */
	    else
		msg_print("A falling branch just misses you.");

	    /* No more */
	    cave_info[y][x] &= ~(CAVE_MARK);
	    cave_set_feat(y, x, FEAT_TREE2);

	    break;
	}

	/* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0C:
	{
	    msg_print("A dagger is thrown at you from the shadows!");
	    dam = damroll(3, 4);
	    take_hit(dam, name);

	    break;
	}

	/* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0D:
	{
	    msg_print("A dagger is thrown at you from the shadows!");
	    dam = damroll(3, 4);
	    take_hit(dam, name);

	    break;
	}

	/* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0E:
	{
	    msg_print("A dagger is thrown at you from the shadows!");
	    dam = damroll(3, 4);
	    take_hit(dam, name);

	    break;
	}

	/* undefined trap. */
    case FEAT_TRAP_HEAD + 0x0F:
	{
	    msg_print("A dagger is thrown at you from the shadows!");
	    dam = damroll(3, 4);
	    take_hit(dam, name);

	    break;
	}

    }

    /* Revert to usage of the complex RNG. */
    Rand_quick = FALSE;
}

