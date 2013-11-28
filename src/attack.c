/** \file attack.c 

    \brief The non-magical attack code.
 
 * Hit chance, critical hits in melee and when shooting/throwing, calculate 
 * ego multiplier, calculate Deadliness adjustment.  Melee attacks (and shield 
 * bashes).  Chance of object breakage, the shooting code, the throwing code.
 *
 * Copyright (c) 2009 
 * Nick McConnell, Leon Marrick, Ben Harrison, James E. Wilson, 
 * Robert A. Koeneke
 *
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
#include "cmds.h"
#include "game-cmd.h"
#include "monster.h"
#include "object.h"
#include "tvalsval.h"
#include "spells.h"
#include "target.h"


/** Druid blows. -LM- */
struct druid_blows {

    const char *description; /**< Name of the blow. */

    s16b dd;		/**< Number of damage dice. */
    s16b ds;		/**< Number of dice sides. */
};

/**
 * Table of Druid blows. -LM-
 */
struct druid_blows d_blow[NUM_D_BLOWS] =
  {
    { "punch",		  1, 5 },
    { "kick",		  2, 4 },
    { "knee",		  1,12 },
    { "chop",		  2, 7 },
    { "uppercut",	  3, 6 },
    { "boot",		  3, 9 },
    { "bang on",	  6, 4 },
    { "slam",		  4, 9 },
    { "grapple with",	 13, 3 },
    { "hammer",		  9, 6 },
    { "head butt",	  3,24 },
    { "strangle",	  8,10 },
    { "roundhouse kick",  5,19 },
    { "assault",	 10,11 },
    { "crush",		 11,11 },
    { "double-kick",	 21, 6 },
    { "thunderclap belt", 8,19 },
    { "blizzard gouge",	 14,11 },
    { "tsunami whirl",	  7,26 },
    { "stormwind chop",	 10,22 }
  };

/**
 * Determine if the player hits a monster (non-magical combat).
 *
 * 5% of all attacks are guaranteed to hit, and another 5% to miss.  
 * The remaining 90% compare attack chance to armour class.
 */
static bool test_hit_combat(int chance, int ac, int visible, int item1,
							int item2)
{
	int k, power;

	/* Percentile dice */
	k = randint0(100);

	/* Instant 5% miss or hit chance */
	if (k < 10)
		return (k < 5);

	/* Invisible monsters are harder to hit */
	if (!visible)
		chance = chance / 2;

	/* Can't be negative */
	if (chance < 0)
		chance = 0;

	/* Get power */
	power = randint0(chance);

	/* Just hit tells to_h bonus */
	if (power == ac) {
		notice_other(IF_TO_H, item1);
		if (item2)
			notice_other(IF_TO_H, item2);
	}

	/* Power competes against armor */
	if ((chance > 0) && (power >= ac))
		return (TRUE);

	/* Assume miss */
	return (FALSE);
}


/**
 * Calculation of critical hits by the player in hand-to-hand combat.
 * -LM-
 *
 * Critical hits represent the ability of a skilled fighter to make his 
 * weapon go where he wants it to.  This increased control is represented 
 * by adding damage dice; this makes the attack both more powerful and 
 * more reliable.
 *
 * Weapons with few damage dice are less reliable in combat (because the 
 * damage they do is more variable).  Their great saving grace is precisely 
 * this variablity; criticals benefit them more.
 *
 * This function is responsible for the basic melee combat messages, which 
 * vary according to the quality of the hit.  A distinction is made between 
 * visible and invisible monsters.
 */
static int critical_melee(int chance, int sleeping_bonus, bool visible,
						  char m_name[], const object_type * o_ptr)
{
	bool armsman = FALSE;

	/* Extract melee power. */
	int power = (chance + sleeping_bonus);

	/* Assume no added dice */
	int add_dice = 0;

	/* Armsman Ability - 1/6 critical chance */
	if ((visible) && (player_has(PF_ARMSMAN)) && (randint0(6) == 0)) {
		armsman = TRUE;
	}

	/* Test for critical hit - chance power/(power + 240) */
	if ((armsman) || (randint1(power + 240) <= power)) {
		/* Determine level of critical hit. */
		/* 1/40 add 5 dice */
		if (randint0(40) == 0)
			add_dice = 5;
		/* Failing that, 1/12 add 4 dice */
		else if (randint0(12) == 0)
			add_dice = 4;
		/* Failing that, 1/3 add 3 dice */
		else if (randint0(3) == 0)
			add_dice = 3;
		/* Otherwise add 2 dice */
		else
			add_dice = 2;

		/* Encourage the player to beat on sleeping monsters. */
		if ((sleeping_bonus) && (visible)) {
			/* More "interesting" messages if we get a seriously good hit. */
			if ((add_dice >= 4) && (player_has(PF_BACKSTAB))) {
				msgt(MSG_HIT, "You ruthlessly sneak attack!");
			}

			/* Standard "wakeup call". */
			else {
				msgt(MSG_HIT, "You rudely awaken the monster.");
			}
		}

		/* Credit where credit is due - not if already a special message */
		else if (armsman) {
			msgt(MSG_HIT, "Armsman hit!");
		}

		/* Print special messages if monster is visible. */
		if (visible) {
			/* 
			 * Messages depend on quality of critical hit.  A distinction 
			 * is often made between edged and blunt weapons.  Unfortu-
			 * nately, whips sometimes display rather odd messages... 
			 */
			if (add_dice <= 2) {
				sound(MSG_HIT_GOOD);
				msgt(MSG_HIT, "You strike %s.", m_name);
			}

			else if (add_dice == 3) {
				sound(MSG_HIT_GREAT);
				if ((o_ptr->tval == TV_SWORD)
					|| (o_ptr->tval == TV_POLEARM)) {
					msgt(MSG_HIT, "You hack at %s.", m_name);
				} else {
					msgt(MSG_HIT, "You pound %s.", m_name);
				}
			}

			else if (add_dice == 4) {
				sound(MSG_HIT_SUPERB);
				if ((o_ptr->tval == TV_SWORD)
					|| (o_ptr->tval == TV_POLEARM)) {
					msgt(MSG_HIT, "You slice into %s.", m_name);

				} else {
					msgt(MSG_HIT, "You bludgeon %s.", m_name);
				}
			}

			else if (add_dice >= 5) {
				sound(MSG_HIT_HI_GREAT);
				msgt(MSG_HIT, "You *smite* %s!", m_name);
			}
		}
	}

	/* If the blow is not a critical hit, then the default message is shown. */
	else if (visible) {
		sound(MSG_HIT);
		msgt(MSG_HIT, "You hit %s.", m_name);
	}

	/* Hits on non-visible monsters always generate the same message. */
	if (!visible) {
		sound(MSG_HIT);
		msgt(MSG_HIT, "You hit something.");
	}

	/* Return the number of damage dice to add. */
	return (add_dice);
}



/**
 * Calculation of critical hits for objects fired or thrown by the player.
 * -LM-
 *
 * Critical shots represent the ability of a skilled fighter to make his 
 * missiles go where he wants them to.  This increased control is 
 * represented by adding damage dice; this makes the attack both more 
 * powerful and more reliable.  Because ammo normally rolls one die, 
 * critical hits are very powerful in archery.
 *
 * This function is responsible for the basic archery and throwing combat 
 * messages, which vary according to the quality of the hit.  A distinction 
 * is made between visible and invisible monsters.
 */
static int critical_shot(int chance, int sleeping_bonus,
						 bool thrown_weapon, bool visible, char m_name[],
						 object_type * o_ptr)
{
	char o_name[80];

	bool marksman = FALSE;

	/* Extract missile power. */
	int power = (chance + sleeping_bonus);

	/* Assume no added dice */
	int add_dice = 0;

	/* Throwing weapons get lots of critical hits. */
	if (thrown_weapon) {
		power = power * 3 / 2;
	}

	/* Marksman Ability - 1/6 chance of critical */
	if ((visible) && (player_has(PF_MARKSMAN)) && (randint0(6) == 0)) {
		marksman = TRUE;
	}

	/* Obtain an item description */
	object_desc(o_name, sizeof(o_name), o_ptr,
				ODESC_FULL | ODESC_SINGULAR);

	/* Test for critical hit. */
	if (marksman || (randint1(power + 360) <= power)) {
		/* Determine level of critical hit. */
		/* 1/50 chance of 3 dice */
		if (randint0(50) == 0)
			add_dice = 3;
		/* Failing that, 1/10 chance of 2 dice */
		else if (randint0(10) == 0)
			add_dice = 2;
		/* Otherwis 1 */
		else
			add_dice = 1;

		/* Encourage the player to throw and shoot things at sleeping monsters */
		if ((sleeping_bonus) && (visible)) {
			if ((thrown_weapon) && (add_dice >= 2)) {
				msgt(MSG_HIT, "Assassin strike!");
			}

			else {
				msgt(MSG_HIT, "You rudely awaken the monster.");
			}
		}

		/* Credit where credit is due - but not if already a special message */
		else if (marksman)
			msgt(MSG_HIT, "Marksmanship!");

		/* Print special messages if monster is visible. */
		if (visible) {
			/* Messages depend on quality of critical hit. */
			if (add_dice == 1) {
				msgt(MSG_HIT, "The %s penetrates %s.", o_name, m_name);
			}

			else if (add_dice == 2) {
				msgt(MSG_HIT, "The %s drives into %s.", o_name, m_name);
			}

			else if (add_dice >= 3) {
				msgt(MSG_HIT, "The %s transpierces %s!", o_name, m_name);
			}
		}
	}

	/* If the shot is not a critical hit, then the default message is shown. */
	else if (visible) {
		msgt(MSG_HIT, "The %s hits %s.", o_name, m_name);
	}

	/* Hits on non-visible monsters always generate the same message. */
	if (!visible) {
		msgt(MSG_HIT, "The %s finds a mark.", o_name);
	}

	/* Return the number of damage dice to add. */
	return (add_dice);
}

typedef struct {
	int notice_slay;
	int slain;
} slay_info;

static const slay_info slays[] = {
	{IF_SLAY_ANIMAL, RF_ANIMAL},
	{IF_SLAY_EVIL, RF_EVIL},
	{IF_SLAY_UNDEAD, RF_UNDEAD},
	{IF_SLAY_DEMON, RF_DEMON},
	{IF_SLAY_ORC, RF_ORC},
	{IF_SLAY_TROLL, RF_TROLL},
	{IF_SLAY_GIANT, RF_GIANT},
	{IF_SLAY_DRAGON, RF_DRAGON}
};

typedef struct {
	int notice_brand;
	int resistant;
} brand_info;

static const brand_info brands[] = {
	{IF_BRAND_ACID, RF_IM_ACID},
	{IF_BRAND_ELEC, RF_IM_ELEC},
	{IF_BRAND_FIRE, RF_IM_FIRE},
	{IF_BRAND_COLD, RF_IM_COLD},
	{IF_BRAND_POIS, RF_IM_POIS}
};

/**
 * Adjust for and notice slays
 */
void get_slay_info(monster_type * m_ptr, int *slay, int *mul, int item,
				   bool notice_launcher, bool notice_ring)
{
	int i;
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	for (i = 0; i < MAX_P_SLAY; i++) {
		/* Are we slaying this monster race? */
		if ((slay[i] > MULTIPLE_BASE)
			&& (rf_has(r_ptr->flags, slays[i].slain))) {
			if (m_ptr->ml) {
				rf_on(l_ptr->flags, slays[i].slain);
			}

			/* Increase multiple if it's better */
			if (*mul < slay[i])
				*mul = slay[i];

			/* Special case for holy attacks */
			if ((i == P_SLAY_EVIL)
				&& (p_ptr->special_attack & (ATTACK_HOLY))
				&& (*mul < 15))
				*mul = 15;

			/* Notice slay */
			notice_other(slays[i].notice_slay, item + 1);
			if (notice_launcher)
				notice_other(slays[i].notice_slay, INVEN_BOW + 1);
			if (notice_ring) {
				notice_other(slays[i].notice_slay, INVEN_RIGHT + 1);
				notice_other(slays[i].notice_slay, INVEN_LEFT + 1);
			}
		}
	}
}

/**
 * Adjust for and notice brands
 */
void get_brand_info(monster_type * m_ptr, int *brand, int *mul, int item,
					bool notice_launcher, bool notice_ring)
{
	int i;
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	for (i = 0; i < MAX_P_BRAND; i++) {
		/* Is this brand effective? */
		if (brand[i] > MULTIPLE_BASE) {
			/* Notice immunity */
			if (rf_has(r_ptr->flags, brands[i].resistant)) {
				if (m_ptr->ml) {
					rf_on(l_ptr->flags, brands[i].resistant);
				}
			}

			/* Otherwise, take extra damage */
			else if (*mul < brand[i]) {
				/* Increase multiple if it's better */
				*mul = brand[i];

				/* Notice brand */
				notice_other(brands[i].notice_brand, item + 1);
				if (notice_launcher)
					notice_other(brands[i].notice_brand, INVEN_BOW + 1);
				if (notice_ring) {
					notice_other(brands[i].notice_brand, INVEN_RIGHT + 1);
					notice_other(brands[i].notice_brand, INVEN_LEFT + 1);
				}
			}
		}
	}
}

/**
 * Handle all special adjustments to the damage done by a non-magical attack.
 *
 * At present, only weapons (including digging tools) and ammo have their 
 * damage adjusted.  Flasks of oil could do fire damage, but they currently 
 * don't.
 *
 * Most slays are x2, except Slay Animal (x1.7), and Slay Evil (x1.5).
 * Weapons of *slaying* now get a larger bonus.  All brands are x1.7.  
 * All slays and brands also add to the base damage.
 *
 * Examples:  (assuming monster is an orc)
 * Dagger of Slay Orc:    1d4 * 2.0 + 10:   Average: 15 damage.
 * Dagger of *Slay Orc*:  1d4 * 2.5 + 15:   Average: just over 21 damage.
 *
 * Players may have temporary magic branding.  Paladins do not get to apply 
 * temporary brands to missiles.  A nasty hack, but necessary. -LM-
 */
static int adjust_dam(long *die_average, object_type * o_ptr,
					  monster_type * m_ptr, int item)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	object_type *i_ptr;
	int i, j, slay[MAX_P_SLAY], brand[MAX_P_BRAND];

	bool notice_launcher = FALSE, notice_ring = FALSE;

	/* 
	 * Assume no special adjustments to damage.  We normally multiply damage 
	 * by 10 for accuracy.
	 */
	int mul = 10;
	int add = 0;

	/* Extract the slays and brands */
	for (i = 0; i < MAX_P_SLAY; i++)
		slay[i] = o_ptr->multiple_slay[i];
	for (i = 0; i < MAX_P_BRAND; i++)
		brand[i] = o_ptr->multiple_brand[i];

	switch (o_ptr->tval) {
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		{
			/* Check launcher for additional brands (slays) */
			i_ptr = &p_ptr->inventory[INVEN_BOW];

			/* If wielding a launcher - sanity check */
			if (i_ptr->k_idx) {
				/* Take the maximum value */
				for (i = 0; i < MAX_P_SLAY; i++)
					slay[i] = MAX(i_ptr->multiple_slay[i], slay[i]);
				for (i = 0; i < MAX_P_BRAND; i++)
					brand[i] = MAX(i_ptr->multiple_brand[i], brand[i]);

				/* Prepare to notice */
				notice_launcher = TRUE;
			}
			break;
		}
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
		{
			for (j = 0; j < 2; j++) {
				/* Check rings for additional brands (slays) */
				i_ptr = &p_ptr->inventory[INVEN_LEFT + j];

				/* If wearing a ring */
				if (i_ptr->k_idx) {
					/* Take the maximum value */
					for (i = 0; i < MAX_P_SLAY; i++)
						slay[i] = MAX(i_ptr->multiple_slay[i], slay[i]);
					for (i = 0; i < MAX_P_BRAND; i++)
						brand[i] = MAX(i_ptr->multiple_brand[i], brand[i]);

					/* Prepare to notice */
					notice_ring = TRUE;
				}
			}
			/* temporary elemental brands */
			if (p_ptr->special_attack & (ATTACK_ACID))
				brand[P_BRAND_ACID] =
					MAX(brand[P_BRAND_ACID], BRAND_BOOST_NORMAL);
			if (p_ptr->special_attack & (ATTACK_ELEC))
				brand[P_BRAND_ELEC] =
					MAX(brand[P_BRAND_ELEC], BRAND_BOOST_NORMAL);
			if (p_ptr->special_attack & (ATTACK_FIRE))
				brand[P_BRAND_FIRE] =
					MAX(brand[P_BRAND_FIRE], BRAND_BOOST_NORMAL);
			if (p_ptr->special_attack & (ATTACK_COLD))
				brand[P_BRAND_COLD] =
					MAX(brand[P_BRAND_COLD], BRAND_BOOST_NORMAL);
			if (p_ptr->special_attack & (ATTACK_POIS))
				brand[P_BRAND_POIS] =
					MAX(brand[P_BRAND_POIS], BRAND_BOOST_NORMAL);
			break;
		}
	}


	/* Wielded weapons and diggers and fired missiles may do extra damage. */
	switch (o_ptr->tval) {
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
		{
			/* Hack -- paladins (and priests) cannot take advantage of
			 * temporary elemental brands to rescue their lousy shooting skill.
			 * Missle weapons are "kind of" edged, right? */
			if (!player_has(PF_BLESS_WEAPON)) {
				if (p_ptr->special_attack & (ATTACK_ACID))
					brand[P_BRAND_ACID] =
						MAX(brand[P_BRAND_ACID], BRAND_BOOST_NORMAL);
				if (p_ptr->special_attack & (ATTACK_ELEC))
					brand[P_BRAND_ELEC] =
						MAX(brand[P_BRAND_ELEC], BRAND_BOOST_NORMAL);
				if (p_ptr->special_attack & (ATTACK_FIRE))
					brand[P_BRAND_FIRE] =
						MAX(brand[P_BRAND_FIRE], BRAND_BOOST_NORMAL);
				if (p_ptr->special_attack & (ATTACK_COLD))
					brand[P_BRAND_COLD] =
						MAX(brand[P_BRAND_COLD], BRAND_BOOST_NORMAL);
				if (p_ptr->special_attack & (ATTACK_POIS))
					brand[P_BRAND_POIS] =
						MAX(brand[P_BRAND_POIS], BRAND_BOOST_NORMAL);

			}

			/* Fall through. */
		}
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_DIGGING:
		{
			get_slay_info(m_ptr, slay, &mul, item, notice_launcher,
						  notice_ring);
			get_brand_info(m_ptr, brand, &mul, item, notice_launcher,
						   notice_ring);

			/* Additional bonus for Holy Light */
			if (player_has(PF_HOLY_LIGHT)) {
				/* +2 or +3 versus Undead and light-sensitive creatures */
				if ((rf_has(r_ptr->flags, RF_UNDEAD))
					|| (rf_has(r_ptr->flags, RF_HURT_LIGHT))) {
					mul += (mul + 10) / 10;
				}

				/* +1 or +2 versus other Evil creatures */
				else if (rf_has(r_ptr->flags, RF_EVIL)) {
					mul += mul / 10;
				}
			}

			break;
		}
	}

	/* Hack - Sometimes, a temporary Holy Attack becomes exhusted. */
	if ((p_ptr->special_attack & (ATTACK_HOLY)) && (randint1(20) == 1)) {
		p_ptr->special_attack &= ~(ATTACK_HOLY);
		msg("Your temporary Holy attack has dissipated.");

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATUS);
	}


	/* 
	 * In addition to multiplying the base damage, slays and brands also 
	 * add to it.  This means that a dagger of Slay Orc (1d4) is a lot 
	 * better against orcs than is a dagger (1d9).
	 * SJGU tone down the affect of slays and brands for launchers
	 */
	if (mul > 10) {
		switch (o_ptr->tval) {
		case TV_SHOT:
		case TV_ARROW:
		case TV_BOLT:
			{
				mul -= (mul - 9) / 3;
			}
		}
		add = (mul - 10);
	}

	/* Apply multiplier to the die average now. */
	*die_average *= mul;

	/* Return the addend for later handling. */
	return (add);
}



/** 
 * Calculate the damage done by a druid fighting barehanded, display an
 * appropriate message, and determine if the blow is capable of causing
 * monster confusion. -LM-
 *
 * Added minimum attack quality and a bonus for deadliness.  Also changed
 * the chance of confusion to depend on skill rather than plev.
 */
static int get_druid_damage(int plev, char m_name[], monster_race * r_ptr,
							int power, int deadliness)
{
	const char *description;
	int dd = 0, ds = 0;
	int chance, n, n_chances, i;
	int damage;
	int b_select;
	bool power_strike = FALSE;

	/* Martial Arts Ability gives a 1/6 powerful blow, Power Strike 1/8 */
	if ((player_has(PF_MARTIAL_ARTS) && (randint0(6) == 0))
		|| (player_has(PF_POWER_STRIKE) && (randint0(8) == 0))) {
		power_strike = TRUE;
	}

	/* 
	 * Minimum attack quality.
	 * That this is not zero makes very little difference in
	 * terms of mean damage.  It just makes players feel better
	 * to not get the low end attacks at high plev. -BR-
	 */
	b_select = 1 + (plev / 10);

	/* 
	 * How many chances we get to choose an attack is 
	 * based on deadliness.  For each chance we will
	 * choose an attack, and and keep the best one -BR-
	 */

	/* Minimum deadliness (No negative deadliness for druid blows) */
	if (deadliness < 1)
		i = 0;

	/* Otherwise */
	else
		i = deadliness_conversion[deadliness];

	/* Get between 2 and 5 chances */
	n_chances = 2 + (i / 100);

	/* Deal with fractional chances */
	if (randint0(100) < (i % 100))
		n_chances++;

	/* Loop over number of number of chances */
	for (n = 0; n < n_chances; n++) {
		/* Choose a (level restricted) attack */
		chance = randint1(2 * plev / 5);

		/* Keep the best attack */
		if (chance > b_select)
			b_select = chance;
	}

	/* Better Hits on a Power Strike - sometimes better than normally allowed */
	if (power_strike && (b_select < NUM_D_BLOWS))
		b_select++;

	/* Just to make sure... */
	if (b_select > NUM_D_BLOWS)
		b_select = NUM_D_BLOWS;

	/* 
	 * Now, get the description and calculate the damage.
	 * Maximize for power stikes.
	 */
	description = d_blow[b_select - 1].description;
	dd = d_blow[b_select - 1].dd;
	ds = d_blow[b_select - 1].ds;

	/* Additional bonus for Holy Light */
	if (player_has(PF_HOLY_LIGHT)) {
		/* +2 sides versus Undead and light-sensitive creatures */
		if ((rf_has(r_ptr->flags, RF_UNDEAD))
			|| (rf_has(r_ptr->flags, RF_HURT_LIGHT))) {
			ds += 2;
		}

		/* +1 side versus other Evil creatures */
		else if (rf_has(r_ptr->flags, RF_EVIL)) {
			ds++;;
		}
	}

	if (power_strike)
		damage = (dd * ds);
	else
		damage = damroll(dd, ds);

	/* Record the damage for display */
	for (i = 0; i < 11; i++)
		p_ptr->barehand_dam[i] = p_ptr->barehand_dam[i + 1];
	p_ptr->barehand_dam[11] = (u16b) damage;

	/* Druids can also confuse monsters. */
	if ((power_strike && (randint0(3) != 0))
		|| (power > randint0(500) + 25)) {
		/* Use the special druid confusion attack. */
		p_ptr->special_attack |= (ATTACK_DRUID_CONFU);

		/* And display the attack message. Feedback for relevant specialties */
		if (power_strike)
			msgt(MSG_HIT, "Power Strike! You attempt to confuse %s.",
				 m_name);
		else
			msgt(MSG_HIT, "You %s and attempt to confuse %s.",
				 description, m_name);
	} else {
		/* Basic attack message. */
		if (power_strike)
			msgt(MSG_HIT, "Power Strike! You %s %s.", description, m_name);
		else
			msgt(MSG_HIT, "You %s %s.", description, m_name);
	}
	return (damage);
}


/**
 * Deadliness multiplies the damage done by a percentage, which varies 
 * from 0% (no damage done at all) to at most 355% (damage is multiplied 
 * by more than three and a half times!).
 *
 * We use the table "deadliness_conversion" to translate internal plusses 
 * to deadliness to percentage values.
 *
 * This function multiplies damage by 100.
 */
static void apply_deadliness(long *die_average, int deadliness)
{
	int i;

	/* Paranoia - ensure legal table access. */
	if (deadliness > 150)
		deadliness = 150;
	if (deadliness < -150)
		deadliness = -150;

	/* Deadliness is positive - damage is increased */
	if (deadliness >= 0) {
		i = deadliness_conversion[deadliness];

		*die_average *= (100 + i);
	}

	/* Deadliness is negative - damage is decreased */
	else {
		i = deadliness_conversion[ABS(deadliness)];

		if (i >= 100)
			*die_average = 0;
		else
			*die_average *= (100 - i);
	}
}


/**
 * Attempt a shield bash; return true if the monster dies
 */
bool attempt_shield_bash(int y, int x, bool * fear, int *blows,
						 char *m_name)
{
	monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int bash_chance, bash_quality, bash_dam;

	/* Bashing chance depends on melee Skill, Dex, and a class level bonus. */
	bash_chance =
		p_ptr->state.skills[SKILL_TO_HIT_MELEE] +
		(adj_dex_th[p_ptr->state.stat_ind[A_DEX]]) - 128 +
		((player_has(PF_STRONG_BASHES)) ? p_ptr->lev : 0);

	/* Spell casters don't bash much - except for those who like the "blunt"
	 * nature of bashes. */

	if ((player_has(PF_STRONG_MAGIC)) && (!(player_has(PF_BLESS_WEAPON)))) {
		bash_chance /= 3;
	}

	/* Players bash more often when they see a real need. */
	/* Unarmed and unskilled */
	if ((!(p_ptr->inventory[INVEN_WIELD].k_idx)) && (bash_chance)
		&& (!(player_has(PF_UNARMED_COMBAT)))
		&& (!(player_has(PF_MARTIAL_ARTS)))) {
		bash_chance *= 4;
	}

	/* ... or armed with a puny weapon */
	if ((p_ptr->inventory[INVEN_WIELD].k_idx) && (bash_chance) &&
		((p_ptr->inventory[INVEN_WIELD].dd *
		  p_ptr->inventory[INVEN_WIELD].ds * (*blows))
		 < (p_ptr->inventory[INVEN_ARM].dd *
			p_ptr->inventory[INVEN_ARM].ds * 3))) {
		bash_chance *= 2;
	}

	/* Try to get in a shield bash. */
	if (bash_chance > randint0(240 + r_ptr->level * 9)) {
		msgt(MSG_HIT, "You get in a shield bash!");

		/* Calculate attack quality, a mix of momentum and accuracy. */
		bash_quality =
			p_ptr->state.skills[SKILL_TO_HIT_MELEE] + (p_ptr->wt / 8) +
			(p_ptr->total_weight / 80) +
			(p_ptr->inventory[INVEN_ARM].weight / 3);

		/* Enhanced for shield masters */
		if (player_has(PF_SHIELD_MAST))
			bash_quality += p_ptr->inventory[INVEN_ARM].weight / 5;

		/* Calculate damage.  Big shields are deadly. */
		bash_dam =
			damroll(p_ptr->inventory[INVEN_ARM].dd,
					p_ptr->inventory[INVEN_ARM].ds);

		/* Notice dice */
		notice_other(IF_DD_DS, INVEN_ARM + 1);

		/* Multiply by quality and experience factors */
		bash_dam *= bash_quality / 20 + p_ptr->lev / 7;

		/* Strength bonus. */
		bash_dam += (adj_str_td[p_ptr->state.stat_ind[A_STR]] - 128);

		/* Paranoia. */
		if (bash_dam > 125)
			bash_dam = 125;

		/* Encourage the player to keep wearing that heavy shield. */
		if (randint1(bash_dam) > 30 + randint1(bash_dam / 2)) {
			sound(MSG_HIT_HI_SUPERB);
			msgt(MSG_HIT, "WHAMM!");
		}

		/* Damage, check for fear and death. */
		if (mon_take_hit(cave_m_idx[y][x], bash_dam, fear, NULL)) {
			/* 
			 * Hack -- High-level warriors can spread their attacks out 
			 * among weaker foes.
			 * In this case a shield bash kill takes the same time one
			 * attack.
			 */
			if ((player_has(PF_SPREAD_ATTACKS)) && (p_ptr->energy_use)
				&& (p_ptr->lev > 39)) {
				p_ptr->energy_use = p_ptr->energy_use / (*blows);
			}

			/* Specialty Ability Fury */
			if (player_has(PF_FURY)) {
				int boost_value;

				/* Mega-Hack - base bonus value on energy used this round */
				/* value = 15 * (energy_use/100) * (10/energy per turn) */
				boost_value =
					(3 * p_ptr->energy_use) /
					(extract_energy[p_ptr->state.pspeed] * 2);

				/* Minimum boost */
				boost_value = ((boost_value > 0) ? boost_value : 1);

				add_speed_boost(boost_value);
			}

			/* Fight's over. */
			return (TRUE);
		}

		/* Stunning. */
		if (bash_quality + p_ptr->lev > randint1(200 + r_ptr->level * 4)) {
			if (rf_has(r_ptr->flags, RF_NO_STUN)) {
				if (m_ptr->ml)
					rf_on(l_ptr->flags, RF_NO_STUN);
			} else {
				msgt(MSG_HIT, "%s is stunned.", m_name);
				m_ptr->stunned += randint0(p_ptr->lev / 5) + 4;
				if (m_ptr->stunned > 24)
					m_ptr->stunned = 24;
			}
		}

		/* Confusion. */
		if (bash_quality + p_ptr->lev > randint1(300 + r_ptr->level * 6)) {
			if (rf_has(r_ptr->flags, RF_NO_CONF)) {
				if (m_ptr->ml)
					rf_on(l_ptr->flags, RF_NO_CONF);
			} else {
				msgt(MSG_HIT, "%s appears confused.", m_name);
				m_ptr->confused += randint0(p_ptr->lev / 5) + 4;
			}
		}

		/* The player will sometimes stumble. */
		if ((30 + adj_dex_th[p_ptr->state.stat_ind[A_DEX]] - 128)
			< randint1(60)) {
			*blows -= randint1(*blows);

			msgt(MSG_GENERIC, "You stumble!");
		}
	}

	return (FALSE);
}

/**
 * Player attacks a (poor, defenseless) creature in melee. 
 * -RAK-, -BEN-, -LM-
 *
 * Handle various bonuses, and try for a shield bash.
 *
 * (What happens when the character hits a monster)
 *      Critical hits may increase the number of dice rolled.  Slays, brands, 
 * monster resistances, and the Deadliness percentage all affect the average 
 * value of each die (this produces the same effect as simply multiplying 
 * the damage, but produces a smoother graph of possible damages).
 *      Once both the number of dice and the dice sides are adjusted, they 
 * are rolled.  Any special adjustments to damage (like those from slays) 
 * are then applied.
 *
 * Example:
 * 1) Dagger (1d4):
 *    1 die rolling four
 * 2) Gets a small critical hit for +2 damage dice:
 *    3 dice rolling four.  Average of each die is 2.5
 * 3) Dagger is a weapon of Slay Orc, and monster is an orc:
 *    average of each die is 2.5 * 2.0 = 5.0
 * 4) Player has a Deadliness value of +72, which gives a bonus of 200%:
 *    5.0 * (100% + 200%) -> average of each die is now 15
 * 5) Roll three dice, each with an average of 15, to get an average damage 
 *    of 45.
 * 6) Finally, the special bonus for orc slaying weapons against orcs is 
 *    added:
 *    45 + 10 bonus = average damage of 55.
 * 
 * If a weapon is not wielded, characters get fist strikes.
 *
 * Successful hits may induce various special effects, including earthquakes, 
 * confusion blows, monsters panicking, and so on.
 */
bool py_attack(int y, int x, bool can_push)
{
	/* Damage */
	long damage;
	bool bonus_attack = FALSE;

	/* blow count */
	int num = 0;
	int blows = p_ptr->state.num_blow;

	/* Bonus to attack if monster is sleeping. */
	int sleeping_bonus = 0;

	/* AC Bonus (penalty) for terrain */
	int terrain_bonus = 0;

	/* Skill and Deadliness */
	int bonus, chance, total_deadliness;

	bool first_hit = TRUE;

	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_lore *l_ptr;

	object_type *o_ptr;

	feature_type *f_ptr = &f_info[cave_feat[y][x]];

	char m_name[80];

	bool fear = FALSE;

	bool do_quake = FALSE;

	bool did_burn = FALSE;

	bool chaotic = FALSE;

	/* Get the monster */
	m_ptr = &m_list[cave_m_idx[y][x]];
	r_ptr = &r_info[m_ptr->r_idx];
	l_ptr = &l_list[m_ptr->r_idx];

	/* Just push past neutral monsters */
	if ((m_ptr->hostile != -1) && can_push) {
		/* Get monster name (or "something") */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0);

		/* Message */
		msg("You push past %s.", m_name);

		return (FALSE);
	}


	/* 
	 * If the monster is sleeping and visible, it can be hit more easily.
	 */
	if ((m_ptr->csleep) && (m_ptr->ml)) {
		sleeping_bonus = 5 + 1 * p_ptr->lev / 5;
		if (player_has(PF_BACKSTAB))
			sleeping_bonus *= 2;
		else if (player_has(PF_ASSASSINATE))
			sleeping_bonus = (3 * sleeping_bonus / 2);

	}

	/* Disturb the monster */
	m_ptr->csleep = 0;

	/* Disturb the player */
	disturb(0, 0);

	/* Auto-Recall if possible and visible */
	if (m_ptr->ml)
		monster_race_track(m_ptr->r_idx);

	/* Track a new monster */
	if (m_ptr->ml)
		health_track(cave_m_idx[y][x]);

	/* Extract monster name (or "it") */
	monster_desc(m_name, sizeof(m_name), m_ptr, 0);

	/* Handle player fear */
	if (p_ptr->timed[TMD_AFRAID]) {
		if (m_ptr->ml) {
			/* Message */
			msg("You are too afraid to attack %s!", m_name);
		} else {
			/* Special Message */
			msg("Something scary is in your way!");
		}

		/* Done */
		return (TRUE);
	}

	/* Hack - Monsters in stasis are invulnerable. */
	if (m_ptr->stasis) {
		if (m_ptr->ml) {
			/* Message */
			msg("Stasis protects %s!", m_name);
		} else {
			/* Special Message */
			msg("Something immovable is in your way!");
		}

		/* Done */
		return (TRUE);
	}

	/* Become hostile */
	m_ptr->hostile = -1;

	/* Monsters in rubble can take advantage of cover. 
	 * Monsters in trees can take advantage of cover, except from players who
	 * know nature lore. */
	if (tf_has(f_ptr->flags, TF_PROTECT) &&
		!(tf_has(f_ptr->flags, TF_ORGANIC) &&
		  (player_has(PF_WOODSMAN) || player_has(PF_ELVEN)))) {
		terrain_bonus = r_ptr->ac / 7 + 5;
	}

	/* Monsters in water are vulnerable.  */
	if (tf_has(f_ptr->flags, TF_EXPOSE)) {
		terrain_bonus -= r_ptr->ac / 5;
	}


	/**** The monster bashing code. -LM-  ****/

	/* No shield on arm, no bash.  */
	if ((!p_ptr->inventory[INVEN_ARM].k_idx)
		|| (p_ptr->state.shield_on_back));

	/* 
	 * Players do not bash if they could otherwise take advantage of special 
	 * bonuses against sleeping monsters, or if the monster is low-level or 
	 * not visible.
	 */
	else if ((sleeping_bonus) || (!m_ptr->ml)
			 || (r_ptr->level < p_ptr->lev / 2));

	else if (attempt_shield_bash(y, x, &fear, &blows, m_name))
		return (TRUE);

	/* Get the weapon */
	o_ptr = &p_ptr->inventory[INVEN_WIELD];

	/* Calculate the attack quality */
	bonus = p_ptr->state.to_h + o_ptr->to_h;
	chance =
		p_ptr->state.skills[SKILL_TO_HIT_MELEE] + (BTH_PLUS_ADJ * bonus);

	/* Calculate deadliness */
	total_deadliness = p_ptr->state.to_d + o_ptr->to_d;

	/* Paranoia.  Ensure legal table access. */
	if (total_deadliness > 150)
		total_deadliness = 150;

	/* Specialty Ability */
	if ((player_has(PF_FAST_ATTACK))
		&& (randint1(8) <= p_ptr->state.num_blow)) {
		blows++;
		bonus_attack = TRUE;
	}

	/* Check for chaotic */
	if (of_has(o_ptr->flags_obj, OF_CHAOTIC))
		chaotic = TRUE;

	/* Attack once for each legal blow */
	while (num++ < blows) {
		int crit_dice = 0;

		/* Credit where credit is due */
		if (bonus_attack && (num == blows))
			msg("Extra Attack!");

		/* Test for hit */
		if (test_hit_combat
			(chance + sleeping_bonus, r_ptr->ac + terrain_bonus, m_ptr->ml,
			 INVEN_WIELD + 1, 0)) {
			/* Sound */
			sound(MSG_HIT);

			/* If this is the first hit, make some noise. */
			if (first_hit) {
				/* Only 1 first hit per round */
				first_hit = FALSE;

				/* Hack -- Rogues and Assassins are silent melee killers. */
				if ((player_has(PF_BACKSTAB))
					|| (player_has(PF_ASSASSINATE)))
					add_wakeup_chance =
						p_ptr->state.base_wakeup_chance / 4 + 500;

				/* Otherwise, make the standard amount of noise. */
				else
					add_wakeup_chance =
						p_ptr->state.base_wakeup_chance / 2 + 1000;
			}


			/* Character is wielding a weapon. */
			if (o_ptr->k_idx) {
				int dice, add;
				long die_average, temp, sides;

				/* Base damage dice */
				dice = o_ptr->dd;

				/* Calculate criticals. */
				crit_dice =
					critical_melee(chance, sleeping_bonus, m_ptr->ml,
								   m_name, o_ptr);

				/* Critical hits may add damage dice. */
				dice += crit_dice;

				/* Mana Burn Specialty */
				if ((crit_dice > 0) && (!did_burn)
					&& player_has(PF_MANA_BURN) && (m_ptr->mana)) {
					/* Base burn amount */
					int burn = BASE_MANA_BURN;

					/* Only one per round! */
					did_burn = TRUE;

					/* Can only burn what we have */
					if (burn > m_ptr->mana)
						burn = m_ptr->mana;

					/* Hit mana and add to damage */
					m_ptr->mana -= burn;
					dice++;

					msgt(MSG_HIT, "Mana Burn!");
				}

				/* Get the average value of a single damage die. (x10) */
				die_average = (10 * (o_ptr->ds + 1)) / 2;

				/* Adjust the average for slays and brands. (10x inflation) */
				add = adjust_dam(&die_average, o_ptr, m_ptr, INVEN_WIELD);

				/* Apply deadliness to average. (100x inflation) */
				apply_deadliness(&die_average, total_deadliness);


				/* Convert die average to die sides. */
				temp = (2 * die_average) - 10000;

				/* Calculate the actual number of sides to each die. */
				sides =
					(temp / 10000) + (randint0(10000) <
									  (temp % 10000) ? 1 : 0);


				/* Roll out the damage. */
				damage = damroll(dice, (s16b) sides);

				/* Maxroll tells to_d bonus */
				if (damage == dice * sides)
					notice_other(IF_TO_D, INVEN_WIELD + 1);

				/* Apply any special additions to damage. */
				damage += add;

			}

			else {
				/* Hack - If no weapon is wielded, Druids are able to fight
				 * effectively.  All other classes only get 1 damage, plus
				 * whatever bonus their strength gives them. -LM- */
				if ((player_has(PF_UNARMED_COMBAT))
					|| player_has(PF_MARTIAL_ARTS)) {
					damage =
						get_druid_damage(p_ptr->lev, m_name, r_ptr,
										 chance + sleeping_bonus,
										 total_deadliness);
				} else {
					damage =
						1 +
						((int) (adj_str_td[p_ptr->state.stat_ind[A_STR]])
						 - 128);
					msgt(MSG_HIT, "You punch %s.", m_name);
				}
			}

			/* Paranoia -- No negative damage */
			if (damage < 0)
				damage = 0;

			/* Hack -- check for earthquake. */
			if (p_ptr->state.impact && (damage > 49)) {
				notice_obj(OF_IMPACT, INVEN_WIELD + 1);
				do_quake = TRUE;
			}

			/* The verbose wizard message has been moved to mon_take_hit. */

			/* Damage, check for fear and death. */
			if (mon_take_hit(cave_m_idx[y][x], (s16b) damage, &fear, NULL)) {
				/* 
				 * Hack -- High-level warriors can spread their attacks out 
				 * among weaker foes.
				 */
				if ((player_has(PF_SPREAD_ATTACKS)) && (p_ptr->energy_use)
					&& (p_ptr->lev > 39) && (num < blows)) {
					p_ptr->energy_use = p_ptr->energy_use * num / blows;
				}

				/* No "holding over" druid confusion attacks */
				p_ptr->special_attack &= ~(ATTACK_DRUID_CONFU);

				/* Fight's over. */
				break;
			}

			/* Confusion attack */
			if ((p_ptr->special_attack & (ATTACK_CONFUSE))
				|| (p_ptr->special_attack & (ATTACK_DRUID_CONFU))) {
				/* Message */
				if (!(p_ptr->special_attack & (ATTACK_DRUID_CONFU)))
					msgt(MSG_HIT, "Your hands stop glowing.");

				/* Cancel special confusion attack */
				p_ptr->special_attack &= ~(ATTACK_CONFUSE);
				p_ptr->special_attack &= ~(ATTACK_DRUID_CONFU);
				p_ptr->redraw |= PR_STATUS;

				/* Confuse the monster */
				if (rf_has(r_ptr->flags, RF_NO_CONF)) {
					if (m_ptr->ml) {
						rf_on(l_ptr->flags, RF_NO_CONF);
					}

					msg("%s is unaffected.", m_name);
				} else if (randint0(110) < r_ptr->level + randint1(10)) {
					msg("%s is unaffected.", m_name);
				} else if (m_ptr->confused > 0) {
					if (m_ptr->ml)
						msgt(MSG_HIT, "%s appears more confused.", m_name);
					else
						msgt(MSG_HIT, "%s sounds more confused.", m_name);
					m_ptr->confused += 4 + randint0(p_ptr->lev) / 12;
				} else {
					if (m_ptr->ml)
						msgt(MSG_HIT, "%s appears confused.", m_name);
					else
						msgt(MSG_HIT, "%s sounds confused.", m_name);
					m_ptr->confused += 10 + randint0(p_ptr->lev) / 5;
				}
			}

			/* Black Breath attack. -LM- */
			if (p_ptr->special_attack & (ATTACK_BLKBRTH)) {
				/* Cancel black breath */
				p_ptr->special_attack &= ~(ATTACK_BLKBRTH);

				/* Message */
				msg("Your hands stop radiating Night.");

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATUS);

				/* The undead are immune */
				if (rf_has(r_ptr->flags, RF_UNDEAD)) {
					if (m_ptr->ml) {	/* control for visibility */
						rf_on(l_ptr->flags, RF_UNDEAD);
					}

					msg("%s is immune!", m_name);
				}
				/* All other monsters get a saving throw. */
				else if ((randint0(160)) < (r_ptr->level + randint0(60))) {
					msg("%s wards off your deadly blow.", m_name);
				}
				/* Tasting some of their own medicine... */
				else {
					m_ptr->black_breath = TRUE;
					msgt(MSG_HIT,
						 "%s is stricken with the Black Breath!", m_name);
				}
			}

			/* Rogue "Hit and Run" attack. -LM- */
			if (p_ptr->special_attack & (ATTACK_FLEE)) {
				/* Cancel the fleeing spell */
				p_ptr->special_attack &= ~(ATTACK_FLEE);

				/* Message */
				msg("You escape into the shadows!");

				/* Teleport. */
				teleport_player(6 + p_ptr->lev / 5, TRUE);

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATUS);

				/* Specialty Ability Fury */
				if (player_has(PF_FURY)) {
					int boost_value;

					/* Mega-Hack - base bonus value on energy used this round */
					/* value = 15 * (energy_use/100) * (10/energy per turn) */
					boost_value =
						(3 * p_ptr->energy_use) /
						(extract_energy[p_ptr->state.pspeed] * 2);

					/* Minimum boost */
					boost_value = ((boost_value > 0) ? boost_value : 1);

					add_speed_boost(boost_value);
				}

				/* Fight's over. */
				return (TRUE);
			}

			/* Chaotic attack. */
			if (chaotic) {
				/* Sometimes do chaotic stuff */
				if (randint0(10) == 0) {
					/* May not still be something there to hit */
					notice_obj(OF_CHAOTIC, 0);
					if (!chaotic_effects(m_ptr))
						return (TRUE);
				}
			}

			/* Monster is no longer asleep */
			sleeping_bonus = 0;
		}

		/* Player misses */
		else {
			/* Sound */
			sound(MSG_MISS);

			/* Message */
			msgt(MSG_MISS, "You miss %s.", m_name);
		}
	}

	/* Specialty Ability Fury */
	if (player_has(PF_FURY)) {
		int boost_value;

		/* Mega-Hack - base bonus value on energy used this round */
		/* value = 15 * (energy_use/100) * (10/energy per turn) */
		boost_value =
			(3 * p_ptr->energy_use) /
			(extract_energy[p_ptr->state.pspeed] * 2);

		/* Minimum boost */
		boost_value = ((boost_value > 0) ? boost_value : 1);

		add_speed_boost(boost_value);
	}

	/* Hack -- delayed fear messages */
	if (fear && m_ptr->ml) {
		/* Sound */
		sound(MSG_FLEE);

		/* Message */
		msgt(MSG_FLEE, "%s flees in terror!", m_name);
	}


	/* Mega-Hack -- apply earthquake brand.  Radius reduced in Oangband. */
	if (do_quake) {
		int py = p_ptr->py;
		int px = p_ptr->px;

		earthquake(py, px, 4, FALSE);
	}

	return (TRUE);
}



/**
 * Determine the odds of an object breaking when thrown at a monster.
 *
 * Note that artifacts never break; see the "drop_near()" function.
 */
static int breakage_chance(object_type * o_ptr)
{
	/* Examine the item type */
	switch (o_ptr->tval) {
		/* Always break */
	case TV_FLASK:
	case TV_POTION:
	case TV_BOTTLE:
	case TV_FOOD:
	case TV_JUNK:
		{
			return (100);
		}

		/* Often break */
	case TV_LIGHT:
	case TV_SCROLL:
	case TV_SKELETON:
		{
			return (40);
		}
		/* Frequently break */
	case TV_ARROW:
		{
			return (30);
		}

		/* Sometimes break */
	case TV_WAND:
	case TV_SHOT:
	case TV_BOLT:
	case TV_SPIKE:
		{
			return (20);
		}
	}

	/* Rarely break */
	return (10);
}


/**
 * Fire an object from the pack or floor.
 *
 * You may only fire items that match your missile launcher.  
 *
 * Project the missile along the path from player to target.
 * If it enters a grid occupied by a monster, check for a hit.
 *
 * The basic damage calculations in archery are the same as in melee, 
 * except that a launcher multiplier is also applied to the dice sides.
 *
 * Apply any special attack or class bonuses, and check for death.
 * Drop the missile near the target (or end of path), sometimes breaking it.
 */
void do_cmd_fire(cmd_code code, cmd_arg args[])
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int dir, item;
	int i, y, x, ty, tx;
	int tdis;

	int break_chance;

	int armour, bonus, chance, total_deadliness;

	int sleeping_bonus = 0;
	int terrain_bonus = 0;

	int damage = 0;

	/* Assume no weapon of velocity or accuracy bonus. */
	bool special_dam = FALSE;
	bool special_hit = FALSE;

	object_type *o_ptr;
	object_type *j_ptr;

	object_type *i_ptr;
	object_type object_type_body;

	bool hit_body = FALSE;
	bool did_pierce = FALSE;
	bool did_miss = FALSE;

	byte missile_attr;
	wchar_t missile_char;

	char o_name[120];
	char m_name[80];

	int path_n = 0;
	u16b path_g[256];

	int msec = op_ptr->delay_factor * op_ptr->delay_factor;

	(void) code;

	/* Get the "bow" (if any) */
	o_ptr = &p_ptr->inventory[INVEN_BOW];

	/* Require a usable launcher */
	if (!o_ptr->tval || !p_ptr->state.ammo_tval) {
		msg("You have nothing to fire with.");
		return;
	}

	/* Get item to fire and direction to fire in. */
	item = args[0].item;
	dir = args[1].direction;

	/* Check the item being fired is usable by the player. */
	if (!item_is_available
		(item, NULL, (USE_EQUIP | USE_INVEN | USE_FLOOR))) {
		msg("That item is not within your reach.");
		return;
	}

	/* Get the object for the ammo */
	j_ptr = object_from_item_idx(item);

	/* Check the ammo can be used with the launcher */
	if (j_ptr->tval != p_ptr->state.ammo_tval) {
		msg("That ammo cannot be fired by your current weapon.");
		return;
	}

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain a local object */
	object_copy(i_ptr, j_ptr);

	/* Single object */
	i_ptr->number = 1;

	/* Take a (partial) turn */
	p_ptr->energy_use = (1000 / p_ptr->state.num_fire);

	/* Sound */
	sound(MSG_SHOOT);

	/* Missile launchers of Velocity sometimes "supercharge" */
	if ((o_ptr->name2 == EGO_VELOCITY) && (randint0(5) == 0)) {
		/* Learn the to-dam (and maybe ego) */
		notice_other(IF_TO_D, INVEN_BOW + 1);

		object_desc(o_name, sizeof(o_name), o_ptr,
					ODESC_FULL | ODESC_SINGULAR);

		/* Set special damage */
		special_dam = TRUE;

		/* Give a hint to the player. */
		if (!has_ego_properties(o_ptr))
			msg("You feel a strange aura of power around your %s.",
				o_name);
		else
			msg("Your %s feels very powerful.", o_name);
	}

	/* Missile launchers of Accuracy sometimes "supercharge" */
	if ((o_ptr->name2 == EGO_ACCURACY) && (randint0(5) == 0)) {
		/* Learn the to-hit (and maybe ego) */
		notice_other(IF_TO_H, INVEN_BOW + 1);

		object_desc(o_name, sizeof(o_name), o_ptr,
					ODESC_FULL | ODESC_SINGULAR);

		/* Set special accuracy */
		special_hit = TRUE;

		/* Give a hint to the player. */
		if (!has_ego_properties(o_ptr))
			msg("You feel a strange aura of power around your %s.",
				o_name);
		else
			msg("Your %s feels very accurate.", o_name);
	}

	/* Fire ammo of backbiting, and it will turn on you.  -LM- */
	if (i_ptr->name2 == EGO_BACKBITING) {
		/* Learn to-hit (!) */
		if (item >= 0)
			notice_other(IF_TO_H, item + 1);
		else
			notice_other(IF_TO_H, item);

		/* Message. */
		msg("Your missile turns in midair and strikes you!");

		/* Calculate damage. */
		damage = damroll(p_ptr->state.ammo_mult * i_ptr->dd * randint1(2),
						 i_ptr->ds * 4);
		if (special_dam) {
			damage += 15;
		}

		/* Inflict both normal and wound damage. */
		take_hit(damage, "ammo of backbiting");
		inc_timed(TMD_CUT, randint1(damage * 3), TRUE);

		/* That ends that shot! */
		return;
	}

	/* Describe the object */
	object_desc(o_name, sizeof(o_name), i_ptr,
				ODESC_FULL | ODESC_SINGULAR);

	/* Find the color and symbol for the object for throwing */
	missile_attr = object_attr(i_ptr);
	missile_char = object_char(i_ptr);


	/* Base range (XXX - this formula is a little weird) */
	tdis = 10 + 5 * p_ptr->state.ammo_mult;

	/* Calculate the quality of the shot */
	bonus = (p_ptr->state.to_h + o_ptr->to_h + i_ptr->to_h);
	chance =
		p_ptr->state.skills[SKILL_TO_HIT_BOW] + (BTH_PLUS_ADJ * bonus);

	/* Sum all the applicable additions to Deadliness. */
	total_deadliness = p_ptr->state.to_d + o_ptr->to_d + i_ptr->to_d;

	/* Start at the player */
	y = py;
	x = px;

	/* Predict the "target" location */
	ty = py + 99 * ddy[dir];
	tx = px + 99 * ddx[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay())
		target_get((s16b *) & tx, (s16b *) & ty);

	/* Calculate the path */
	path_n = project_path(path_g, tdis, py, px, ty, tx, PROJECT_THRU);

	/* Hack -- Handle stuff */
	handle_stuff(p_ptr);

	/* Project along the path */
	for (i = 0; i < path_n; ++i) {
		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);
		feature_type *f_ptr = &f_info[cave_feat[ny][nx]];

		/* Hack -- Stop before hitting walls */
		if (!tf_has(f_ptr->flags, TF_PASSABLE))
			break;

		/* Advance */
		x = nx;
		y = ny;

		/* Only do visuals if the player can "see" the missile */
		if (panel_contains(y, x) && player_can_see_bold(y, x)) {
			/* Visual effects */
			print_rel(missile_char, missile_attr, y, x);
			move_cursor_relative(y, x);
			Term_fresh();
			if (p_ptr->redraw)
				redraw_stuff(p_ptr);
			Term_xtra(TERM_XTRA_DELAY, msec);
			light_spot(y, x);
			Term_fresh();
			if (p_ptr->redraw)
				redraw_stuff(p_ptr);
		}

		/* Delay anyway for consistency */
		else {
			/* Pause anyway, for consistancy */
			Term_xtra(TERM_XTRA_DELAY, msec);
		}

		/* Handle monster */
		if (cave_m_idx[y][x] > 0) {
			monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			feature_type *f_ptr = &f_info[cave_feat[y][x]];

			bool fear = FALSE;

			int dice, add;
			long die_average, temp, sides;
			int cur_range, base_range, chance2;

			/* Assume a default death */
			const char *note_dies = " dies.";

			/* Find Range */
			cur_range = distance(py, px, y, x);

			/* Most Accurate Range */
			base_range = tdis / 2;
			if (base_range > 10)
				base_range = 10;

			/* Penalize Range */
			if (cur_range > base_range)
				chance2 = chance - cur_range + base_range;
			else
				chance2 = chance;

			/* Note the (attempted) collision */
			hit_body = TRUE;

			/* Get "the monster" or "it" */
			monster_desc(m_name, sizeof(m_name), m_ptr, 0);

			/* 2nd hit on a pierce is easier */
			if (did_pierce)
				chance2 += chance2 / 2;

			/* Hard to hit after a miss */
			else if (did_miss)
				chance2 -= chance2 / 3;

			/* Sleeping, visible monsters are easier to hit. */
			if ((m_ptr->csleep) && (m_ptr->ml)) {
				sleeping_bonus = 5 + p_ptr->lev / 5;
			}

			/* Monsters in rubble can take advantage of cover. 
			 * Monsters in trees can take advantage of cover, except from 
			 * players who know nature lore. */
			if (tf_has(f_ptr->flags, TF_PROTECT) &&
				!(tf_has(f_ptr->flags, TF_ORGANIC) &&
				  (player_has(PF_WOODSMAN) || player_has(PF_ELVEN)))) {
				terrain_bonus = r_ptr->ac / 5 + 5;
			}
			/* Monsters in water are vulnerable.  */
			if (tf_has(f_ptr->flags, TF_EXPOSE)) {
				terrain_bonus -= r_ptr->ac / 4;
			}

			/* Get effective armour class of monster. */
			armour = r_ptr->ac + terrain_bonus;

			/* Weapons of velocity sometimes almost negate monster armour. */
			if (special_hit)
				armour /= 3;

			/* Did we miss it (penalize distance travelled) */
			if (!
				(test_hit_combat
				 (chance2 + sleeping_bonus, armour, m_ptr->ml,
				  INVEN_BOW + 1, ((item < 0) ? item : item + 1)))) {
				did_miss = TRUE;

				/* Keep Going */
				continue;
			}

			/* Some monsters get "destroyed" */
			if ((rf_has(r_ptr->flags, RF_DEMON))
				|| (rf_has(r_ptr->flags, RF_UNDEAD))
				|| (rf_has(r_ptr->flags, RF_STUPID))
				|| (strchr("Evg", r_ptr->d_char))) {
				/* Special note at death */
				note_dies = " is destroyed.";
			}


			/* Make some noise.  Hack -- Assassins are silent missile weapon
			 * killers. */
			if (player_has(PF_ASSASSINATE))
				add_wakeup_chance =
					p_ptr->state.base_wakeup_chance / 4 + 600;
			else
				add_wakeup_chance =
					p_ptr->state.base_wakeup_chance / 3 + 1200;


			/* Hack -- Track this monster race, if monster is visible */
			if (m_ptr->ml)
				monster_race_track(m_ptr->r_idx);

			/* Hack -- Track this monster, if visible */
			if (m_ptr->ml)
				health_track(cave_m_idx[y][x]);


			/* 
			 * The basic damage-determination formula is the same in
			 * archery as it is in melee (apart from the launcher mul-
			 * tiplier).
			 */

			/* base damage dice */
			dice = i_ptr->dd;

			/* Critical hits may add damage dice. */
			dice +=
				critical_shot(chance2, sleeping_bonus, FALSE, m_ptr->ml,
							  m_name, i_ptr);


			/* Get the average value of a single damage die. (x10) */
			die_average = (10 * (i_ptr->ds + 1)) / 2;

			/* Apply the launcher multiplier. */
			die_average *= p_ptr->state.ammo_mult;

			/* Adjust the average for slays and brands. (10x inflation) */
			add = adjust_dam(&die_average, i_ptr, m_ptr, item);

			/* Apply deadliness to average. (100x inflation) */
			apply_deadliness(&die_average, total_deadliness);


			/* Convert die average to die sides. */
			temp = (2 * die_average) - 10000;

			/* Calculate the actual number of sides to each die. */
			sides =
				(temp / 10000) + (randint0(10000) <
								  (temp % 10000) ? 1 : 0);


			/* Roll out the damage. */
			damage = damroll(dice, (s16b) sides);

			/* Max roll tells to_d bonus (launcher and missile) */
			if (damage == dice * sides) {
				notice_other(IF_TO_D, INVEN_BOW + 1);
				if (item >= 0)
					notice_other(IF_TO_D, item + 1);
				else
					notice_other(IF_TO_D, item);
			}

			/* Apply any special additions to damage. */
			damage += add;


			/* If a weapon of velocity activates, increase damage. */
			if (special_dam) {
				damage += 15;
			}

			/* If an "enhance missile" spell has been cast, increase the
			 * damage, and cancel the spell. */
			if (p_ptr->special_attack & (ATTACK_SUPERSHOT)) {
				damage = 5 * damage / 4 + 35;
				p_ptr->special_attack &= ~(ATTACK_SUPERSHOT);

				/* Redraw the state */
				p_ptr->redraw |= (PR_STATUS);
			}

			/* Hack - Assassins are deadly... */
			if (player_has(PF_STRONG_SHOOT)) {
				/* Increase damage directly (to avoid excessive total damage by 
				 * granting too high a Deadliness). */
				if (p_ptr->state.ammo_tval == TV_SHOT) {

					damage += p_ptr->lev / 2;
				}
				if (p_ptr->state.ammo_tval == TV_ARROW) {
					damage += 2 * p_ptr->lev / 5;
				}
				if (p_ptr->state.ammo_tval == TV_BOLT) {
					damage += p_ptr->lev / 3;
				}
			}

			/* No negative damage */
			if (damage < 0)
				damage = 0;

			/* Hit the monster, check for death. */
			if (mon_take_hit(cave_m_idx[y][x], damage, &fear, note_dies)) {
				/* Dead monster */
			}

			/* No death */
			else {
				/* Become hostile */
				m_ptr->hostile = -1;

				/* Message */
				message_pain(cave_m_idx[y][x], damage);

				/* Take note */
				if (fear && m_ptr->ml) {
					/* Sound */
					sound(MSG_FLEE);

					/* Message */
					msgt(MSG_FLEE, "%s flees in terror!", m_name);
				}
			}

			/* Check for piercing */
			if ((player_has(PF_PIERCE_SHOT)) && (randint0(2) == 0)
				&& ((p_ptr->state.ammo_tval == TV_ARROW)
					|| (p_ptr->state.ammo_tval == TV_BOLT))) {
				msg("Pierce!");
				did_pierce = TRUE;
				continue;
			}

			/* Otherwise Stop looking */
			else
				break;
		}

		/* Stop if it's a tree or rubble */
		if (!cave_project(ny, nx))
			break;

	}

	/* Copy ID flags */
	if_copy(i_ptr->id_other, j_ptr->id_other);

	/* Reduce and describe inventory */
	if (item >= 0) {
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Reduce and describe floor item */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}

	/* Chance of breakage (during attacks) */
	break_chance = (hit_body ? breakage_chance(i_ptr) : 0);

	/* Drop (or break) near that location */
	drop_near(i_ptr, break_chance, y, x, TRUE);
}



void textui_cmd_fire_at_nearest(void)
{
	/* the direction '5' means 'use the target' */
	int i, dir = 5, item = -1;
	int bow_sv = p_ptr->inventory[INVEN_BOW].sval;
	int ammo_tv = 0;

	/* Require a usable launcher */
	if (!p_ptr->inventory[INVEN_BOW].tval) {
		msg("You have nothing to fire with.");
		return;
	}

	/* Get the right ammo type */
	switch (bow_sv) {
	case SV_SLING:
		{
			ammo_tv = TV_SHOT;
			break;
		}
	case SV_SHORT_BOW:
	case SV_LONG_BOW:
		{
			ammo_tv = TV_ARROW;
			break;
		}
	case SV_LIGHT_XBOW:
	case SV_HEAVY_XBOW:
		{
			ammo_tv = TV_BOLT;
			break;
		}
	}

	/* Find first eligible ammo in the quiver */
	for (i = QUIVER_START; i < QUIVER_END; i++) {
		if (p_ptr->inventory[i].tval != ammo_tv)
			continue;
		item = i;
		break;
	}

	/* Require usable ammo */
	if (item < 0) {
		msg("You have no ammunition in the quiver to fire");
		return;
	}

	/* Require foe */
	if (!target_set_closest(TARGET_KILL | TARGET_QUIET))
		return;

	/* Check for confusion */
	if (p_ptr->timed[TMD_CONFUSED]) {
		msg("You are confused.");
		dir = ddd[randint0(8)];
	}

	/* Fire! */
	cmd_insert(CMD_FIRE);
	cmd_set_arg_item(cmd_get_top(), 0, item);
	cmd_set_arg_target(cmd_get_top(), 1, dir);
}

/**
 * Throw an object from the pack or floor.
 *
 * Now allows for throwing weapons.  Unlike all other thrown objects, 
 * throwing weapons can get critical hits and take advantage of bonuses to 
 * Skill and Deadliness from other equipped items.
 *
 * Unlikely shooting, thrown weapons do not continue on after a miss.
 * It's too annoying to send your nice throwing weapons halfway across the
 * dungeon.
 */
void do_cmd_throw(cmd_code code, cmd_arg args[])
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int dir, item;
	int i, y, x, ty, tx;
	int chance, tdis;
	int mul, div;

	int break_chance;

	int total_deadliness;
	int sleeping_bonus = 0;
	int terrain_bonus = 0;

	int damage;

	object_type *o_ptr;
	object_type *i_ptr;
	object_type object_type_body;

	bool hit_body = FALSE;

	byte stat;
	byte missile_attr;
	wchar_t missile_char;

	char o_name[120];
	char m_name[80];

	int path_n = 0;
	u16b path_g[256];

	int msec = op_ptr->delay_factor * op_ptr->delay_factor;

	(void) code;

	/* Get item to throw and direction in which to throw it. */
	item = args[0].item;
	dir = args[1].direction;

	/* Make sure the player isn't throwing wielded items */
	if (item >= INVEN_WIELD && item < QUIVER_START) {
		msg("You cannot throw wielded items.");
		return;
	}

	/* Check the item being thrown is usable by the player. */
	if (!item_is_available
		(item, NULL, (USE_EQUIP | USE_INVEN | USE_FLOOR))) {
		msg("That item is not within your reach.");
		return;
	}

	/* Get the object */
	o_ptr = object_from_item_idx(item);

	/* Can't unwield cursed items this way! */
	if ((item > INVEN_PACK) && (item < QUIVER_START)
		&& (cf_has(o_ptr->flags_curse, CF_STICKY_WIELD))) {
		/* Oops */
		msg("Hmmm, it seems to be cursed.");

		/* Notice */
		notice_curse(CF_STICKY_WIELD, item + 1);

		/* Nope */
		return;
	}

	if (cf_has(o_ptr->flags_curse, CF_STICKY_CARRY)) {
		/* Oops */
		msg("Hmmm, it seems to be cursed.");

		/* Notice */
		notice_curse(CF_STICKY_CARRY, item + 1);

		/* Nope */
		return;
	}

	/* Notice perfect balance */
	if (of_has(o_ptr->flags_obj, OF_PERFECT_BALANCE))
		notice_obj(OF_PERFECT_BALANCE, (item < 0) ? item : item + 1);

	/* Get local object */
	i_ptr = &object_type_body;

	/* Obtain a local object */
	object_copy(i_ptr, o_ptr);

	/* Distribute the charges of rods/wands between the stacks */
	distribute_charges(o_ptr, i_ptr, 1);

	/* Single object */
	i_ptr->number = 1;

	/* Description */
	object_desc(o_name, sizeof(o_name), i_ptr,
				ODESC_FULL | ODESC_SINGULAR);

	/* Get the color and symbol of the thrown object */
	missile_attr = object_attr(i_ptr);
	missile_char = object_char(i_ptr);


	/* Extract a "distance multiplier" */
	mul = 10;

	/* Enforce a minimum weight of one pound */
	div = ((i_ptr->weight > 10) ? i_ptr->weight : 10);

	/* Is it ordinary throw, or magical? */
	stat = (magic_throw ? A_INT : A_STR);

	/* Distance -- Reward strength or intelligence, penalize weight */
	tdis = (adj_str_blow[p_ptr->state.stat_ind[stat]] + 20) * mul / div;

	/* Max distance of 10 */
	if (tdis > 10)
		tdis = 10;

	/* Specialty Ability Mighty Throw */
	if (player_has(PF_MIGHTY_THROW))
		tdis *= 2;

	/* 
	 * Other thrown objects are easier to use, but only throwing weapons 
	 * take advantage of bonuses to Skill and Deadliness from other 
	 * equipped items.
	 */
	if (of_has(i_ptr->flags_obj, OF_THROWING)) {
		chance =
			p_ptr->state.skills[SKILL_TO_HIT_THROW] +
			BTH_PLUS_ADJ * (p_ptr->state.to_h + i_ptr->to_h);
		total_deadliness = p_ptr->state.to_d + i_ptr->to_d;
	} else {
		chance =
			(3 * p_ptr->state.skills[SKILL_TO_HIT_THROW] / 2) +
			(BTH_PLUS_ADJ * i_ptr->to_h);
		total_deadliness = i_ptr->to_d;
	}

	/* Mages become like rogues for this */
	if (magic_throw)
		chance += p_ptr->state.skills[SKILL_TO_HIT_THROW];


	/* Take a turn */
	p_ptr->energy_use = 100;


	/* Start at the player */
	y = py;
	x = px;

	/* Predict the "target" location */
	ty = py + 99 * ddy[dir];
	tx = px + 99 * ddx[dir];

	/* Check for "target request" */
	if ((dir == 5) && target_okay())
		target_get((s16b *) & tx, (s16b *) & ty);

	/* Calculate the path */
	path_n = project_path(path_g, tdis, py, px, ty, tx, 0);


	/* Hack -- Handle stuff */
	handle_stuff(p_ptr);

	/* Project along the path */
	for (i = 0; i < path_n; ++i) {
		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);
		feature_type *f_ptr = &f_info[cave_feat[ny][nx]];

		/* Hack -- Stop before hitting walls */
		if (!tf_has(f_ptr->flags, TF_PASSABLE))
			break;

		/* Advance */
		x = nx;
		y = ny;

		/* Only do visuals if the player can "see" the missile */
		if (panel_contains(y, x) && player_can_see_bold(y, x)) {
			/* Visual effects */
			print_rel(missile_char, missile_attr, y, x);
			move_cursor_relative(y, x);
			Term_fresh();
			if (p_ptr->redraw)
				redraw_stuff(p_ptr);
			Term_xtra(TERM_XTRA_DELAY, msec);
			light_spot(y, x);
			Term_fresh();
			if (p_ptr->redraw)
				redraw_stuff(p_ptr);
		}

		/* Delay anyway for consistency */
		else {
			/* Pause anyway, for consistancy */
			Term_xtra(TERM_XTRA_DELAY, msec);
		}

		/* Handle monster */
		if (cave_m_idx[y][x] > 0) {
			monster_type *m_ptr = &m_list[cave_m_idx[y][x]];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			feature_type *f_ptr = &f_info[cave_feat[y][x]];

			bool fear = FALSE;

			int dice, add;
			long die_average, temp, sides;
			int cur_range, base_range, chance2;

			/* Assume a default death */
			const char *note_dies = " dies.";

			/* Find Range */
			cur_range = distance(py, px, y, x);

			/* Most Accurate Range */
			base_range = tdis / 2;
			if (base_range > 10)
				base_range = 10;

			/* Penalize Range */
			if (cur_range > base_range)
				chance2 = chance - cur_range + base_range;
			else
				chance2 = chance;

			/* If the monster is sleeping, it'd better pray there are no
			 * Assassins with throwing weapons nearby. -LM- */
			if ((m_ptr->csleep) && (m_ptr->ml)
				&& (of_has(i_ptr->flags_obj, OF_THROWING))) {
				if (player_has(PF_ASSASSINATE))
					sleeping_bonus = 15 + p_ptr->lev / 2;
				else
					sleeping_bonus = 0;
			}

			/* Monsters in rubble can take advantage of cover. 
			 * Monsters in trees can take advantage of cover, except from 
			 * players who know nature lore. */
			if (tf_has(f_ptr->flags, TF_PROTECT) &&
				!(tf_has(f_ptr->flags, TF_ORGANIC) &&
				  (player_has(PF_WOODSMAN) || player_has(PF_ELVEN)))) {
				terrain_bonus = r_ptr->ac / 5 + 5;
			}
			/* Monsters in water are vulnerable.  */
			if (tf_has(f_ptr->flags, TF_EXPOSE)) {
				terrain_bonus -= r_ptr->ac / 4;
			}


			/* Get "the monster" or "it" */
			monster_desc(m_name, sizeof(m_name), m_ptr, 0);


			/* Note the collision */
			hit_body = TRUE;

			/* Did we miss it (penalize range)? */
			if (!test_hit_combat(chance2 + sleeping_bonus,
								 r_ptr->ac + terrain_bonus,
								 m_ptr->ml,
								 ((item < 0) ? item : item + 1), 0)) {
				/* Keep Going - magical throw always hits visible monsters */
				if (!(magic_throw) || !m_ptr->ml)
					break;
			}

			/* Some monsters get "destroyed" */
			if ((rf_has(r_ptr->flags, RF_DEMON))
				|| (rf_has(r_ptr->flags, RF_UNDEAD))
				|| (rf_has(r_ptr->flags, RF_STUPID))
				|| (strchr("Evg", r_ptr->d_char))) {
				/* Special note at death */
				note_dies = " is destroyed.";
			}


			/* Make some noise.  Hack -- Assassins are silent missile weapon
			 * killers. */
			if (player_has(PF_ASSASSINATE))
				add_wakeup_chance =
					p_ptr->state.base_wakeup_chance / 4 + 300;
			else
				add_wakeup_chance =
					p_ptr->state.base_wakeup_chance / 3 + 1200;


			/* Hack -- Track this monster race, if critter is visible */
			if (m_ptr->ml)
				monster_race_track(m_ptr->r_idx);

			/* Hack -- Track this monster, if visible */
			if (m_ptr->ml)
				health_track(cave_m_idx[y][x]);


			/* 
			 * The basic damage-determination formula is the same in
			 * throwing as it is in melee (apart from the thrown weapon 
			 * multiplier, and the ignoring of non-object bonuses to
			 * Deadliness for objects that are not thrown weapons). 
			 */

			/* Base damage dice */
			dice = i_ptr->dd;

			/* Object is a throwing weapon. */
			if (of_has(i_ptr->flags_obj, OF_THROWING)) {
				/* Perfectly balanced weapons do even more damage. */
				if (of_has(i_ptr->flags_obj, OF_PERFECT_BALANCE))
					dice *= 2;

				/* Critical hits may add damage dice. */
				dice +=
					critical_shot(chance2, sleeping_bonus, TRUE, m_ptr->ml,
								  m_name, i_ptr);

				/* 
				 * Multiply the number of damage dice by the throwing 
				 * weapon multiplier, if applicable.  This is not the 
				 * prettiest equation, but it does at least try to keep 
				 * throwing weapons competitive.
				 */
				dice *= 2 + p_ptr->lev / 12;
			}

			/* Ordinary thrown object */
			else {
				/* Display a default hit message. */
				if (m_ptr->ml) {
					msgt(MSG_HIT, "The %s hits %s.", o_name, m_name);
				} else {
					msgt(MSG_HIT, "The %s finds a mark.", o_name);
				}
			}


			/* Get the average value of a single damage die. (x10) */
			die_average = (10 * (i_ptr->ds + 1)) / 2;

			/* Adjust the average for slays and brands. (10x inflation) */
			add = adjust_dam(&die_average, i_ptr, m_ptr, item);

			/* Apply deadliness to average. (100x inflation) */
			apply_deadliness(&die_average, total_deadliness);


			/* Convert die average to die sides. */
			temp = (2 * die_average) - 10000;

			/* Calculate the actual number of sides to each die. */
			sides =
				(temp / 10000) + (randint0(10000) <
								  (temp % 10000) ? 1 : 0);


			/* Roll out the damage. */
			damage = damroll(dice, (s16b) sides);

			/* Max roll gives to_d bonus */
			if (damage == dice * sides)
				notice_other(IF_TO_D, ((item < 0) ? item : item + 1));

			/* Apply any special additions to damage. */
			damage += add;

			/* No negative damage */
			if (damage < 0)
				damage = 0;

			/* Hit the monster, check for death */
			if (mon_take_hit(cave_m_idx[y][x], damage, &fear, note_dies)) {
				/* Dead monster */
			}

			/* No death */
			else {
				/* Become hostile */
				m_ptr->hostile = -1;

				/* Message */
				message_pain(cave_m_idx[y][x], damage);

				/* Take note */
				if (fear && m_ptr->ml) {
					/* Sound */
					sound(MSG_FLEE);

					/* Message */
					msg("%s flees in terror!", m_name);
				}
			}


			/* Object falls to the floor */
			break;
		}

		/* Stop if it's trees or rubble */
		if (!cave_project(ny, nx))
			break;
	}

	/* Copy ID flags */
	if_copy(i_ptr->id_other, o_ptr->id_other);

	/* Reduce and describe inventory */
	if (item >= 0) {
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Reduce and describe floor item */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_optimize(0 - item);
	}


	/* Chance of breakage.  Throwing weapons are designed not to break. */
	if (of_has(i_ptr->flags_obj, OF_PERFECT_BALANCE))
		break_chance = 0;
	else if (of_has(i_ptr->flags_obj, OF_THROWING))
		break_chance = (hit_body ? 1 : 0);
	else
		break_chance = (hit_body ? breakage_chance(i_ptr) : 0);

	/* Drop (or break) near that location */
	drop_near(i_ptr, break_chance, y, x, TRUE);
}

void textui_cmd_throw(void)
{
	int item, dir;
	const char *q, *s;

	/* Get an item */
	item_tester_hook = obj_can_throw;
	q = "Throw which item? ";
	s = "You have nothing to throw.";
	if (!get_item
		(&item, q, s, CMD_THROW, (USE_INVEN | USE_FLOOR | QUIVER_TAGS))) {
		item_tester_hook = NULL;
		return;
	}
	item_tester_hook = NULL;

	/* Get a direction (or cancel) */
	if (!get_aim_dir(&dir))
		return;

	cmd_insert(CMD_THROW);
	cmd_set_arg_item(cmd_get_top(), 0, item);
	cmd_set_arg_target(cmd_get_top(), 1, dir);
}
