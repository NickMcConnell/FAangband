/** \file info.c 
    \brief Object information


 * Tables containing object kind descriptions, extra numerical info.  
 * Detailed info on spells.  Text for extended object descriptions of 
 * various kinds, artifact and DSM activation and flag descriptions.  Self 
 * Knowledge.  Spell failure chance, if is OK to cast, extra info shown in 
 * books, and print spells of a given spellbook.
 *
 * Copyright (c) 1999-2009 Nick McConnell, Leon Marrick, Ben Harrison, 
 * James E. Wilson, Robert A. Koeneke
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
#include "ui-menu.h"



/**
 * General information about classes of objects. -LM-
 * 
 * Index is tval.
 */
char *obj_class_info[101] = {
    "", "", "", "", "",
    "", "",
    "Chests may have some really good treasures hidden inside, but can be perilous to open...",
    "", "",

    "", "", "", "", "",
    "", "Sling ammo.", "Bow ammo.", "Crossbow ammo.",
    "Missile launchers allow you to inflict damage from a distance without using magic.",

    "Diggers, especially heavy ones, are invaluable for forced entry and escape and can make a lucky miner rich.",
    "Hafted weapons rely on blunt force to inflict damage.  Since they spill relatively little blood, priests much prefer to carry one.",
    "Pole-mounted weapons are often cumbersome and may require two hands to wield, but some offer both a high degree of protection and powerful attacks.",
    "The effectiveness of edged weapons depends on keen edges and sharp points.  They tend to be quite light and are easy to use, but some may not deal enough damage for your liking.",
    "",
    "", "", "", "", "",

    "Footwear protects the feet only, but some rare items of this type have magics to render them fleet, light, or steady.",
    "Your hands would benefit from protection too, but most magic users need to keep their fingers unencumbered or magically supple.",
    "Many a blow will be struck upon your head, and protection here will certainly be helpful.  Some rare items may protect and enhance your mind.",
    "Many a blow will be struck upon your head, and protection here will certainly be helpful.  Some rare items may protect and enhance your mind.",
    "Shields can be worn on your arm, or on your back if you need both hands to use your weapon.  So protective can a shield be that it can reduce damage as much or more than body armour, and you can perhaps deflect physical missiles (even shards) or take advantage of opportunities to bash your foe if you have one on your arm.",
    "Experienced adventurers wrap a cloak around their body.  Some rare items of this type may allow you to slip silently around less alert enemies.",
    "Some kind of body protection will become a necessity as you face ever more dangerous opponents; rare items of this type may hold many and varied protective magics.",
    "Some kind of body protection will become a necessity as you face ever more dangerous opponents; rare items of this type may hold many and varied protective magics.",
    "Armour made of dragon scales is rare indeed, and powerful dragon magics allow you to sometimes breathe even as great dragons do.",
    "An adventurer who cannot see is jackal food.  The further away your illumination ends, the greater your chance to ready yourself for desperate combat.",

    "Amulets slip around your neck, and almost all have magics wondrous or perilous bound inside.",
    "", "", "", "",
    "You may wear a ring upon each of your two ring fingers, and benefit or suffer from the magics it contains.",
    "", "", "", "",

    "", "", "", "", "",
    "Staffs are heavy, and take up plenty of space in your backpack, but can hold a lot of sometimes powerful spells that affect large areas.  Staffs recharge easily and well.",
    "", "", "", "",

    "", "", "", "", "",
    "Wands hold a variety of spells, useful both in combat and for exploration.  Bolt spells in wands often beam, and ball spells affect large areas.  Once its charges are exhausted, a wand is useless until recharged.",
    "The magics stored in rods never run out, given enough time between uses to recover.  Rods can do a lot of damage, but they affect only small areas.  Bolt spells in rods seldom or never beam.",
    "", "", "",

    "One will often find sheets of parchment with magic spells.  They are easy to read, and are a warrior or paladin's best chance at making use of magic.",
    "", "", "", "",
    "Healers, alchemists, and sages create potions in vast quantities, and store them in small flasks of clear glass or gleaming crystal.  Once quaffed, a potion is guaranteed to work, but not every strange liquid was mixed for your benefit...",
    "", "", "", "",

    "Deep in the murky dungeons strange mushrooms grow, and monsters rout about sealed packets of food that their owners will never need again.",
    "", "", "", "",
    "", "", "", "", "",

    "A manual of sorcerous magics, bound in fine linen dyed deep red.",
    "A shining gospel of piety in flawless white and gold.",
    "A runed stone with earthly and natural magics chiseled in brown and rich greens.",
    "A black tome of necromantic rituals, with shadows lying deep around it.",
    "",
    "", "", "", "", "",

    "Small valuables and coins."
};


/**
 * Output numerical values for magical device damages, healing, etc., for 
 * an item belonging to an object class whose effects are fully known.  The 
 * only way this information can appear is through *identifying*, or by 
 * eventual learning through use. -LM-
 */
static char *extra_data(object_type * o_ptr)
{
    byte tval_to_index;

    /* Boundary control. */
    if (o_ptr->sval > 49)
	return (NULL);

    /* Table can handle Dragon Scale Mails, */
    if (o_ptr->tval == TV_DRAG_ARMOR)
	tval_to_index = 0;

    /* Staffs, */
    else if (o_ptr->tval == TV_STAFF)
	tval_to_index = 1;

    /* Wands, */
    else if (o_ptr->tval == TV_WAND)
	tval_to_index = 2;

    /* and Rods... */
    else if (o_ptr->tval == TV_ROD)
	tval_to_index = 3;

    /* ...But nothing else. */
    else
	return (NULL);


    /* Locate the object in the table, and return the description. */
    return (format("%s", obj_special_info[tval_to_index][o_ptr->sval]));
}


#define CHECK_FIRST(txt, first) \
  if ((first)) { (first) = FALSE; text_out_to_screen(TERM_WHITE, (txt)); } \
  else text_out_to_screen(TERM_WHITE, ", ");

/**
 * Display the damage done with a multiplier
 */
extern void output_dam(object_type * o_ptr, int mult, const char *against,
		       bool * first)
{
    int dam, die_average, add = 0, i, deadliness, crit, chance;

    /* Average damage for one standard side (x10) */
    die_average = (10 * (o_ptr->ds + 1)) / 2;

    /* Multiply by slay or brand (x10) */
    die_average *= mult;

    /* Record the addend */
    if (mult > MULTIPLE_BASE)
	add = (mult - MULTIPLE_BASE);

    /* Multiply by deadliness (x100) */
    deadliness = p_ptr->dis_to_d;
    if (object_known_p(o_ptr))
	deadliness += o_ptr->to_d;
    if (deadliness > 150)
	deadliness = 150;
    if (deadliness < -150)
	deadliness = -150;
    if (deadliness >= 0)
	die_average *= (100 + deadliness_conversion[deadliness]);
    else {
	i = deadliness_conversion[ABS(deadliness)];
	if (i >= 100)
	    die_average = 0;
	else
	    die_average *= (100 - i);
    }

    /* Factor in criticals (x 10) */
    chance = p_ptr->state.skills[SKILL_TO_HIT_MELEE] + p_ptr->dis_to_h;
    if (object_known_p(o_ptr))
	chance += o_ptr->to_h;
    chance = (100 * chance) / (chance + 240);
    if (player_has(PF_ARMSMAN))
	chance = 100 - (83 * (100 - chance)) / 100;
    crit = 259 * chance;
    crit /= 1000;

    /* Multiply by number of sides */
    dam = die_average * (o_ptr->dd * 10 + crit);

    CHECK_FIRST("", *first);
    if ((dam > 50000) || (add > 0))
	text_out_to_screen(TERM_L_GREEN, format("%d", add + dam / 100000));
    else
	text_out_to_screen(TERM_L_RED, "0");
    text_out_to_screen(TERM_WHITE, format(" against %s", against));
}

/**
 * Outputs the damage we do/would do with the weapon
 */
extern void display_weapon_damage(object_type * o_ptr)
{
    object_type forge, *old_ptr = &forge;
    object_type *i_ptr;
    int i, j;

    bool first = TRUE;
    int show_m_tohit;
    int brand[MAX_P_BRAND], slay[MAX_P_SLAY];

    const char *slayee[] = { "animals", "evil creatures", "undead", "demons",
	"orcs", "trolls", "giants", "dragons"
    };
    const char *brandee[] = { "acid", "lightning", "fire", "cold", "poison" };

    /* Extract the slays and brands */
    for (j = 0; j < MAX_P_SLAY; j++)
	slay[j] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j))
	    ? o_ptr->multiple_slay[j] : MULTIPLE_BASE;
    for (j = 0; j < MAX_P_BRAND; j++)
	brand[j] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + j))
	    ? o_ptr->multiple_brand[j] : MULTIPLE_BASE;

    /* Check rings for additional brands (slays) */
    for (i = 0; i < 2; i++) {
	i_ptr = &p_ptr->inventory[INVEN_LEFT + i];

	/* If wearing a ring */
	if (i_ptr->k_idx) {
	    /* Pick up any brands (and slays!) */
	    for (j = 0; j < MAX_P_SLAY; j++)
		slay[j] =
		    MAX(slay[j], (if_has(i_ptr->id_other, OBJECT_ID_BASE_SLAY + j))
			? i_ptr->multiple_slay[j] : MULTIPLE_BASE);
	    for (j = 0; j < MAX_P_BRAND; j++)
		brand[j] =
		    MAX(brand[j],
			(if_has(i_ptr->id_other, OBJECT_ID_BASE_BRAND + j))
			? i_ptr->multiple_brand[j] : MULTIPLE_BASE);
	}
    }

    /* temporary elemental brands */
    if (p_ptr->special_attack & (ATTACK_ACID))
	brand[P_BRAND_ACID] = MAX(brand[P_BRAND_ACID], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_ELEC))
	brand[P_BRAND_ELEC] = MAX(brand[P_BRAND_ELEC], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_FIRE))
	brand[P_BRAND_FIRE] = MAX(brand[P_BRAND_FIRE], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_COLD))
	brand[P_BRAND_COLD] = MAX(brand[P_BRAND_COLD], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_POIS))
	brand[P_BRAND_POIS] = MAX(brand[P_BRAND_POIS], BRAND_BOOST_NORMAL);
    if (p_ptr->special_attack & (ATTACK_HOLY))
	slay[P_SLAY_EVIL] = MAX(slay[P_SLAY_EVIL], 15);

    /* Ok now the hackish stuff, we replace the current weapon with this one */
    object_copy(old_ptr, &p_ptr->inventory[INVEN_WIELD]);
    object_copy(&p_ptr->inventory[INVEN_WIELD], o_ptr);
    calc_bonuses(TRUE);
    show_m_tohit = p_ptr->dis_to_h;
    if (if_has(o_ptr->id_other, IF_TO_H))
	show_m_tohit += o_ptr->to_h;

    text_out_to_screen(TERM_WHITE, "\nWielding it you would have ");
    text_out_to_screen(TERM_L_GREEN, format("%d ", p_ptr->num_blow));
    text_out_to_screen(TERM_WHITE,
		       format("blow%s and do an average damage per blow of ",
			      (p_ptr->num_blow) ? "s" : ""));

    for (i = 0; i < MAX_P_SLAY; i++) {
	if (slay[i] > MULTIPLE_BASE)
	    output_dam(o_ptr, slay[i], slayee[i], &first);
    }

    for (i = 0; i < MAX_P_BRAND; i++) {
	if (brand[i] > MULTIPLE_BASE) {
	    char buf[40];

	    strnfmt(buf, sizeof(buf), "non %s resistant creatures", brandee[i]);
	    output_dam(o_ptr, brand[i], buf, &first);
	}
    }

    output_dam(o_ptr, MULTIPLE_BASE,
	       (first) ? "all monsters" : "other monsters", &first);

    text_out_to_screen(TERM_WHITE, ".  Your + to Skill would be ");
    text_out_to_screen(TERM_L_GREEN, format("%d", show_m_tohit));
    text_out_to_screen(TERM_WHITE, ". ");

    /* get our weapon back */
    object_copy(&p_ptr->inventory[INVEN_WIELD], old_ptr);
    calc_bonuses(TRUE);
}

/**
 * Display the ammo damage done with a multiplier
 */
extern void output_ammo_dam(object_type * o_ptr, int mult, const char *against,
			    bool * first, bool perfect)
{
    object_type *b_ptr = &p_ptr->inventory[INVEN_BOW];

    int dam, die_average, add = 0, i, deadliness, crit, chance, dice =
	o_ptr->dd;

    bool thrown;

    /* Throwing weapon, or launched missile? */
    thrown = !is_missile(o_ptr);

    /* Average damage for one standard side (x10) */
    die_average = (10 * (o_ptr->ds + 1)) / 2;

    /* Apply the launcher multiplier. */
    if (!thrown)
	die_average *= p_ptr->ammo_mult;

    /* Multiply by slay or brand (x10) */
    die_average *= mult;

    /* Record the addend */
    if (mult > 10)
	add = (mult - 10);

    /* Multiply by deadliness (x100) */
    deadliness = p_ptr->dis_to_d;
    if (object_known_p(o_ptr))
	deadliness += o_ptr->to_d;
    if (object_known_p(b_ptr))
	deadliness += b_ptr->to_d;
    if (deadliness > 150)
	deadliness = 150;
    if (deadliness < -150)
	deadliness = -150;
    if (deadliness >= 0)
	die_average *= (100 + deadliness_conversion[deadliness]);
    else {
	i = deadliness_conversion[ABS(deadliness)];
	if (i >= 100)
	    die_average = 0;
	else
	    die_average *= (100 - i);
    }

    /* Get critical chance (x 10) */
    chance = p_ptr->state.skills[SKILL_TO_HIT_BOW] + p_ptr->dis_to_h;
    if (object_known_p(o_ptr))
	chance += o_ptr->to_h;
    if ((!thrown) && (object_known_p(b_ptr)))
	chance += b_ptr->to_h;
    if (thrown)
	chance = chance * 3 / 2;
    chance = (100 * chance) / (chance + 360);
    if (player_has(PF_MARKSMAN))
	chance = 100 - (83 * (100 - chance)) / 100;
    crit = 116 * chance;
    crit /= 1000;

    /* Increase dice */
    if (thrown && perfect)
	dice *= 2;
    dice = dice * 10 + crit;
    if (thrown)
	dice *= 2 + p_ptr->lev / 12;

    /* Multiply by number of sides */
    dam = die_average * dice;

    CHECK_FIRST("", *first);
    if ((dam > 50000) || (add > 0))
	text_out_to_screen(TERM_L_GREEN, format("%d", add + dam / 100000));
    else
	text_out_to_screen(TERM_L_RED, "0");
    text_out_to_screen(TERM_WHITE, format(" against %s", against));
}

/**
 * Outputs the damage we do/would do with the current bow and this ammo
 */
extern void display_ammo_damage(object_type * o_ptr)
{
    int i;
    object_type *i_ptr;

    bool first = TRUE;
    bool perfect;

    int brand[MAX_P_BRAND], slay[MAX_P_SLAY];

    const char *slayee[] = { "animals", "evil creatures", "undead", "demons",
	"orcs", "trolls", "giants", "dragons"
    };
    const char *brandee[] = { "acid", "lightning", "fire", "cold", "poison" };

    /* Extract the slays and brands */
    for (i = 0; i < MAX_P_SLAY; i++)
	slay[i] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + i))
	    ? o_ptr->multiple_slay[i] : MULTIPLE_BASE;
    for (i = 0; i < MAX_P_BRAND; i++)
	brand[i] = (if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + i))
	    ? o_ptr->multiple_brand[i] : MULTIPLE_BASE;

    /* Hack -- paladins (and priests) cannot take advantage of * temporary
     * elemental brands to rescue their * lousy shooting skill. * * Missle
     * weapons are "kind of" edged, right? */
    if (!player_has(PF_BLESS_WEAPON) || (o_ptr->tval > TV_BOLT)) {
	if (p_ptr->special_attack & (ATTACK_ACID))
	    brand[P_BRAND_ACID] = MAX(brand[P_BRAND_ACID], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_ELEC))
	    brand[P_BRAND_ELEC] = MAX(brand[P_BRAND_ELEC], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_FIRE))
	    brand[P_BRAND_FIRE] = MAX(brand[P_BRAND_FIRE], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_COLD))
	    brand[P_BRAND_COLD] = MAX(brand[P_BRAND_COLD], BRAND_BOOST_NORMAL);
	if (p_ptr->special_attack & (ATTACK_POIS))
	    brand[P_BRAND_POIS] = MAX(brand[P_BRAND_POIS], BRAND_BOOST_NORMAL);
    }
    if (p_ptr->special_attack & (ATTACK_HOLY))
	slay[P_SLAY_EVIL] = MAX(slay[P_SLAY_EVIL], 15);

    /* Check for well-balanced throwing weapons */
    perfect = (of_has(o_ptr->id_obj, OF_PERFECT_BALANCE));

    if (o_ptr->tval > TV_BOLT) {
	text_out_to_screen(TERM_WHITE,
			   "\nThrowing it you would do an average damage of ");
	for (i = 0; i < MAX_P_SLAY; i++)
	    if (slay[i] > MULTIPLE_BASE)
		output_ammo_dam(o_ptr, slay[i], slayee[i], &first, perfect);

	for (i = 0; i < MAX_P_BRAND; i++)
	    if (brand[i] > MULTIPLE_BASE) {
		char buf[40];

		strnfmt(buf, sizeof(buf), "non %s resistant creatures",
			brandee[i]);
		output_ammo_dam(o_ptr, brand[i], buf, &first, perfect);
	    }

	output_ammo_dam(o_ptr, MULTIPLE_BASE,
			(first) ? "all monsters" : "other monsters", &first,
			perfect);
	text_out_to_screen(TERM_WHITE, ". ");
    } else if (p_ptr->ammo_tval == o_ptr->tval) {
	/* Check launcher for additional brands (slays) */
	i_ptr = &p_ptr->inventory[INVEN_BOW];

	/* If wielding a launcher - sanity check */
	if (i_ptr->k_idx) {
	    /* Pick up any brands (and slays!) */
	    for (i = 0; i < MAX_P_SLAY; i++)
		slay[i] =
		    MAX(slay[i], (if_has(i_ptr->id_other, OBJECT_ID_BASE_SLAY + i))
			? i_ptr->multiple_slay[i] : MULTIPLE_BASE);
	    for (i = 0; i < MAX_P_BRAND; i++)
		brand[i] =
		    MAX(brand[i],
			(if_has(i_ptr->id_other, OBJECT_ID_BASE_BRAND + i))
			? i_ptr->multiple_brand[i] : MULTIPLE_BASE);
	}

	text_out_to_screen(TERM_WHITE,
			   "\nUsing it with your current launcher you would do an average damage per shot of ");

	for (i = 0; i < MAX_P_SLAY; i++)
	    if (slay[i] > MULTIPLE_BASE)
		output_ammo_dam(o_ptr, slay[i], slayee[i], &first, perfect);

	for (i = 0; i < MAX_P_BRAND; i++)
	    if (brand[i] > MULTIPLE_BASE) {
		char buf[40];

		strnfmt(buf, sizeof(buf), "non %s resistant creatures",
			brandee[i]);
		output_ammo_dam(o_ptr, brand[i], buf, &first, perfect);
	    }

	output_ammo_dam(o_ptr, MULTIPLE_BASE,
			(first) ? "all monsters" : "other monsters", &first,
			perfect);
	text_out_to_screen(TERM_WHITE, ". ");
    } else
	text_out_to_screen(TERM_WHITE,
			   "\nYou cannot use this missile with your current launcher. ");

}

void display_device_chance(object_type * o_ptr)
{
    /* Get the object kind */
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    /* Extract the item level */
    int lev = k_ptr->level;

    /* Base chance of success */
    int chance = p_ptr->state.skills[SKILL_DEVICE];

    /* Final probability */
    int prob = 10000;

    /* Confusion hurts skill */
    if (p_ptr->timed[TMD_CONFUSED])
	chance = chance / 2;

    /* High level objects are harder */
    chance = chance - ((lev > 50) ? 50 : lev);

    /* Calculate the chance */
    if (chance < USE_DEVICE) {
	prob *= 2;
	prob /= (USE_DEVICE * (USE_DEVICE + 1 - chance));
    } else {
	prob -= prob * (USE_DEVICE - 1) / chance;
    }
    prob /= 100;

    /* Display */
    text_out_to_screen(TERM_WHITE,
		       format
		       ("\nYou currently have a %d%% chance of activating this device.",
			prob));

    return;
}


/**
 * Extract and return extended information about an object, including 
 * (usually) flavor, (sometimes) damage information for magical items, 
 * and (if appropriate) ego and artifact lore. -LM-
 *
 * Code mostly from object_desc and roff_aux.
 */
void object_info(char buf[2048], object_type * o_ptr, bool in_store)
{
    object_kind *k_ptr = &k_info[o_ptr->k_idx];

    char *t;
    char *s;
    char *u;
    char *v;
    char *w;
    char *x;

    /* Assume no flavor string, no ego info, and no base info. */
    char *modstr = "";
    char *egoinfo = "";
    char *baseinfo = "";



    /* Standard artifacts have unique descriptions. */
    if ((artifact_p(o_ptr)) && (o_ptr->name1 < ART_MIN_RANDOM)
	&& (object_known_p(o_ptr))) {
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	/* If already in memory, simple to access */
	strcpy(buf, a_text + a_ptr->text);

	/* Hack - translate if we do that */
	if (Term->xchar_hook)
	    xstr_trans(buf, (Term->xchar_hook(128) == 128));

	/* Return the description, if any. */
	return;
    }

    /* All non-artifact or random artifact objects. */
    else {
	/* If already in memory, simple to access */
	strcpy(buf, k_text + k_ptr->text);

	/* No object description, so return failure. */
	if (!buf[0])
	    return;


	/* Various object types have different kinds of information. */
	switch (o_ptr->tval) {
	    /* Dragon Scale Mails */
	case TV_DRAG_ARMOR:
	    {
		/* Allow processing of activation information. */
		baseinfo = format("%s", buf);
		break;
	    }

	    /* Staffs */
	case TV_STAFF:
	    {
		/* Color the object, unless in store. */
		if (in_store) {
		    modstr = "";
		    baseinfo = format("A staff %s", buf);
		} else {
		    modstr = object_adj(o_ptr->tval, o_ptr->sval);
		    baseinfo = format("& # staff %s", buf);
		}

		break;
	    }

	    /* Wands */
	case TV_WAND:
	    {
		/* Color the object, unless in store. */
		if (in_store) {
		    modstr = "";
		    baseinfo = format("A wand %s", buf);
		} else {
		    modstr = object_adj(o_ptr->tval, o_ptr->sval);
		    baseinfo = format("& # wand %s", buf);
		}

		break;
	    }

	    /* Rods */
	case TV_ROD:
	    {
		/* Color the object, unless in store. */
		if (in_store) {
		    modstr = "";
		    baseinfo = format("A rod %s", buf);
		} else {
		    modstr = object_adj(o_ptr->tval, o_ptr->sval);
		    baseinfo = format("& # rod %s", buf);
		}

		break;
	    }

	    /* Scrolls */
	case TV_SCROLL:
	    {
		/* Color the object, unless in store. */
		if (in_store) {
		    modstr = "";
		    baseinfo = format("A parchment scroll %s", buf);
		} else {
		    modstr = object_adj(o_ptr->tval, o_ptr->sval);
		    baseinfo =
			format("A parchment scroll %s It is titled \"#\".",
			       buf);
		}

		break;
	    }

	    /* Potions */
	case TV_POTION:
	    {
		/* Color the object, unless in store. */
		if (in_store) {
		    modstr = "";
		    baseinfo = format("A potion %s", buf);
		} else {
		    modstr = object_adj(o_ptr->tval, o_ptr->sval);
		    baseinfo = format("& # potion %s", buf);
		}

		break;
	    }

	    /* All other objects can just display the info text. */
	default:
	    {
		/* Store the basic info text. */
		baseinfo = format("%s", buf);
	    }
	}


	/* Ego-object descriptions are added to any base description. */
	if ((o_ptr->name2) && (object_known_p(o_ptr))) {
	    ego_item_type *e_ptr = &e_info[o_ptr->name2];
	    char ebuf[2048];

	    /* First, find the information in memory, or get it from the binary 
	     * file. */
	    /* If already in memory, simple to access */
	    strcpy(ebuf, e_text + e_ptr->text);

	    /* Point to the ego-item information. */
	    egoinfo = ebuf;
	}

	/* Point to "buf", and start dumping the result */
	t = buf;


      /*** Assemble the object information. ***/

	/* The object needs an article */
	if (baseinfo[0] == '&') {
	    /* Skip ampersand and space. */
	    s = baseinfo + 2;

	    /* Flavor starts with a vowel */
	    if (is_a_vowel(modstr[0]))
		w = "An ";

	    /* Flavor starts with a non-vowel */
	    else
		w = "A ";
	} else {
	    w = "";

	    /* Start at beginning of base info. */
	    s = baseinfo;
	}

	/* Copy the base info, inserting flavor and info text. */
	for (; *s; s++) {
	    /* Insert article */
	    if (s != baseinfo) {
		for (; *w; w++)
		    *t++ = *w;
	    }

	    /* Insert flavor, make it lowercase */
	    if (*s == '#') {
		char make_lower;

		if (strlen(modstr)) {
		    for (u = modstr; *u; u++) {
			make_lower = *u;
			*t++ = tolower(make_lower);
		    }
		}
	    }

	    /* Insert numerical info. */
	    else if (*s == '~') {
		/* Extra info if object is fully known. */
		if (o_ptr->ident & (IDENT_KNOWN) || k_ptr->known_effect) {
		    /* Grab the numerical info. */
		    char *moddata = extra_data(o_ptr);

		    /* If there is any numerical data, */
		    if (strlen(moddata) > 0) {
			/* ...insert a space, and */
			*t++ = ' ';

			/* insert the mumerical data into the string. */
			for (v = moddata; *v; v++)
			    *t++ = *v;
		    }
		}

		/* Otherwise, nothing. */
	    }

	    /* Normally copy. */
	    else
		*t++ = *s;
	}

	/* Extra info for ego items. */
	if ((o_ptr->name2) && (object_known_p(o_ptr))) {
	    char *divider =
		(small_screen ? "                    ---" :
		 "                                   ---");


	    /* Insert a return, a divider, and another return. */
	    *t++ = '\n';
	    for (x = divider; *x; x++)
		*t++ = *x;
	    *t++ = '\n';

	    /* Copy the ego info to the information string. */
	    for (x = egoinfo; *x; x++)
		*t++ = *x;
	}

	/* End the string. */
	*t = '\0';

	/* Hack - translate if we do that */
	if (Term->xchar_hook)
	    xstr_trans(buf, (Term->xchar_hook(128) == 128));

	/* Return the string. */
	return;
    }
}

/**
 * Describe an item.  Rewritten for FAangband 1.1 to fit ID by use.
 */
void object_info_detail(object_type * o_ptr)
{
    int j;

    int attr_listed = 0;
    int attr_num = 0;
    int kill_num = 0;
    int max = 0, min = 0;

    bool res = FALSE, vul = FALSE;

    char buf[10] = "";
    char *statname[] = { "strength", "intelligence", "wisdom",
	"dexterity", "constitution", "charisma"
    };
    char *othername[] = { "magic mastery", "stealth", "searching",
	"infravision", "tunnelling", "speed",
	"shooting speed", "shooting power"
    };
    char *slayee[] = { "animals", "evil creatures", "undead", "demons",
	"orcs", "trolls", "giants", "dragons"
    };
    char *brand[] = { "acid", "lightning", "fire", "cold", "poison" };


    /* Describe activation, if present and fully known. */
    if ((o_ptr->activation) && (o_ptr->ident & IDENT_WORN)) {
	text_out_to_screen(TERM_YELLOW,
			   format("Activation: %s\n", item_activation(o_ptr)));
    }


    /* Hack -- describe lite's */
    if (o_ptr->tval == TV_LITE) {
	if (artifact_p(o_ptr) && (o_ptr->k_idx != 477)) {
	    text_out_to_screen(TERM_WHITE,
			       "It provides light (radius 3) forever.  ");
	} else if (o_ptr->sval == SV_LITE_LANTERN) {
	    text_out_to_screen(TERM_WHITE,
			       "It provides light (radius 2) when fueled.  ");
	} else if (o_ptr->sval == SV_LITE_TORCH) {
	    text_out_to_screen(TERM_WHITE,
			       "It provides light (radius 1) when fueled.  ");
	}
    }

    /* Hack - artifact magic devices */
    if ((o_ptr->name1) && (extra_data(o_ptr)))
	text_out_to_screen(TERM_WHITE, extra_data(o_ptr));

    text_out_to_screen(TERM_WHITE, "\n");


    /* And then describe it fully */

    /* Single skill and deadliness bonuses for jewellery */
    if (is_jewellery(o_ptr)) {
	if (o_ptr->to_d == 0) {
	    if (o_ptr->to_h < 0) {
		text_out_to_screen(TERM_WHITE, "It subtracts ");
		text_out_to_screen(TERM_RED, format("%d ", -o_ptr->to_h));
		text_out_to_screen(TERM_WHITE, "from your skill.  ");
	    } else if (o_ptr->to_h > 0) {
		text_out_to_screen(TERM_WHITE, "It adds ");
		text_out_to_screen(TERM_L_GREEN, format("%d ", o_ptr->to_h));
		text_out_to_screen(TERM_WHITE, "to your skill.  ");
	    }
	}
	if (o_ptr->to_h == 0) {
	    if (o_ptr->to_d < 0) {
		text_out_to_screen(TERM_WHITE, "It subtracts ");
		text_out_to_screen(TERM_RED, format("%d ", -o_ptr->to_d));
		text_out_to_screen(TERM_WHITE, "from your deadliness.  ");
	    } else if (o_ptr->to_d > 0) {
		text_out_to_screen(TERM_WHITE, "It adds ");
		text_out_to_screen(TERM_GREEN, format("%d ", o_ptr->to_d));
		text_out_to_screen(TERM_WHITE, "to your deadliness.  ");
	    }
	}
    }

    /* Affects stats. */
    if ((o_ptr->ident) & IDENT_WORN) {
	for (j = 0; j < A_MAX; j++) {
	    if (o_ptr->bonus_stat[j] != 0)
		attr_num++;
	    if (o_ptr->bonus_stat[j] < min)
		min = o_ptr->bonus_stat[j];
	    if (o_ptr->bonus_stat[j] > max)
		max = o_ptr->bonus_stat[j];
	}

	if (attr_num > 0) {
	    byte attr =
		(o_ptr->bonus_stat[A_STR] > 0 ? TERM_L_GREEN : TERM_ORANGE);

	    text_out_to_screen(TERM_WHITE, "It gives ");

	    /* Special case: all stats */
	    if (min == max) {
		text_out_to_screen(attr, format("%d ", min));
		text_out_to_screen(attr, "to all your stats");
	    }

	    /* Some stats */
	    else {
		for (j = 0; j < A_MAX; j++) {
		    if (o_ptr->bonus_stat[j] == 0)
			continue;
		    attr =
			(o_ptr->bonus_stat[j] > 0 ? TERM_L_GREEN : TERM_ORANGE);
		    text_out_to_screen(attr,
				       format("%d ", o_ptr->bonus_stat[j]));
		    text_out_to_screen(TERM_WHITE, "to your ");
		    text_out_to_screen(attr, statname[j]);
		    if (attr_num >= 3)
			text_out_to_screen(TERM_WHITE, ", ");
		    if (attr_num == 2)
			text_out_to_screen(TERM_WHITE, " and ");
		    if (attr_num == 1)
			text_out_to_screen(TERM_WHITE, ". ");
		    attr_num--;
		}
	    }
	}
    }


    /* Sustain stats. */
    if ((of_has(o_ptr->id_obj, OF_SUSTAIN_STR)) || (of_has(o_ptr->id_obj, OF_SUSTAIN_INT))
	|| (of_has(o_ptr->id_obj, OF_SUSTAIN_WIS))
	|| (of_has(o_ptr->id_obj, OF_SUSTAIN_DEX))
	|| (of_has(o_ptr->id_obj, OF_SUSTAIN_CON))
	|| (of_has(o_ptr->id_obj, OF_SUSTAIN_CHR))) {
	/* Clear number of items to list. */
	attr_num = 0;

	/* How many attributes need to be listed? */
	if (of_has(o_ptr->id_obj, OF_SUSTAIN_STR))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_SUSTAIN_INT))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_SUSTAIN_WIS))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_SUSTAIN_DEX))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_SUSTAIN_CON))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_SUSTAIN_CHR))
	    attr_num++;

	/* Special case: sustain all stats */
	if (attr_num == A_MAX) {
	    text_out_to_screen(TERM_WHITE, "It sustains ");
	    text_out_to_screen(TERM_L_GREEN, "all your stats. ");
	} else {
	    text_out_to_screen(TERM_WHITE, "It sustains your ");

	    /* Loop for number of attributes in this group. */
	    for (j = 0; j < A_MAX; j++) {
		if (!of_has(o_ptr->id_obj, OF_SUSTAIN_STR + j))
		    continue;
		text_out_to_screen(TERM_L_GREEN, statname[j]);
		if (attr_num >= 3)
		    text_out_to_screen(TERM_WHITE, ", ");
		if (attr_num == 2)
		    text_out_to_screen(TERM_WHITE, " and ");
		if (attr_num == 1)
		    text_out_to_screen(TERM_WHITE, ". ");
		attr_num--;
	    }

	}
    }


    /* Clear number of items to list. */
    attr_num = 0;

    /* Affects other bonuses. */
    if ((o_ptr->ident) & IDENT_WORN) {
	for (j = 0; j < MAX_P_BONUS; j++)
	    if (o_ptr->bonus_other[j] != 0)
		attr_num++;

	if (attr_num > 0) {
	    byte attr = TERM_WHITE;

	    text_out_to_screen(TERM_WHITE, "It gives ");

	    for (j = 0; j < MAX_P_BONUS; j++) {
		if (o_ptr->bonus_other[j] == 0)
		    continue;
		attr = (o_ptr->bonus_other[j] > 0 ? TERM_L_GREEN : TERM_ORANGE);
		text_out_to_screen(attr, format("%d ", o_ptr->bonus_other[j]));
		text_out_to_screen(TERM_WHITE, "to your ");
		text_out_to_screen(attr, othername[j]);
		if (attr_num >= 3)
		    text_out_to_screen(TERM_WHITE, ", ");
		if (attr_num == 2)
		    text_out_to_screen(TERM_WHITE, " and ");
		if (attr_num == 1)
		    text_out_to_screen(TERM_WHITE, ". ");
		attr_num--;
	    }
	}
    }

    /* Clear number of items to list. */
    attr_num = 0;

    /* Slays. */
    for (j = 0; j < MAX_P_SLAY; j++)
	if (if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j)) {
	    attr_num++;

	    /* Hack for great banes */
	    if ((j == P_SLAY_ANIMAL)
		&& (o_ptr->multiple_slay[j] > SLAY_BOOST_MINOR)) {
		attr_num--;
		kill_num++;
	    }
	    if ((j == P_SLAY_EVIL)
		&& (o_ptr->multiple_slay[j] > SLAY_BOOST_SMALL)) {
		attr_num--;
		kill_num++;
	    }
	    if ((j >= P_SLAY_UNDEAD)
		&& (o_ptr->multiple_slay[j] > SLAY_BOOST_NORMAL)) {
		attr_num--;
		kill_num++;
	    }
	}

    if (attr_num > 0) {
	byte attr = TERM_RED;

	text_out_to_screen(TERM_WHITE, "It slays ");

	for (j = 0; j < MAX_P_SLAY; j++) {
	    if (!(if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j)))
		continue;
	    if ((j == 0) && (o_ptr->multiple_slay[j] > SLAY_BOOST_MINOR))
		continue;
	    if ((j == 1) && (o_ptr->multiple_slay[j] > SLAY_BOOST_SMALL))
		continue;
	    if ((j > 1) && (o_ptr->multiple_slay[j] > SLAY_BOOST_NORMAL))
		continue;
	    text_out_to_screen(attr, slayee[j]);
	    if (attr_num >= 3)
		text_out_to_screen(TERM_WHITE, ", ");
	    if (attr_num == 2)
		text_out_to_screen(TERM_WHITE, " and ");
	    if (attr_num == 1)
		text_out_to_screen(TERM_WHITE, ". ");
	    attr_num--;
	}
    }

    if (kill_num > 0) {
	byte attr = TERM_RED;

	text_out_to_screen(TERM_WHITE, "It is a great bane of ");

	for (j = 0; j < MAX_P_SLAY; j++) {
	    if (!(if_has(o_ptr->id_other, OBJECT_ID_BASE_SLAY + j)))
		continue;
	    if ((j == 0) && (o_ptr->multiple_slay[j] <= SLAY_BOOST_MINOR))
		continue;
	    if ((j == 1) && (o_ptr->multiple_slay[j] <= SLAY_BOOST_SMALL))
		continue;
	    if ((j > 1) && (o_ptr->multiple_slay[j] <= SLAY_BOOST_NORMAL))
		continue;
	    text_out_to_screen(attr, slayee[j]);
	    if (kill_num >= 3)
		text_out_to_screen(TERM_WHITE, ", ");
	    if (kill_num == 2)
		text_out_to_screen(TERM_WHITE, " and ");
	    if (kill_num == 1)
		text_out_to_screen(TERM_WHITE, ". ");
	    kill_num--;
	}
    }

    /* Clear number of items to list. */
    attr_num = 0;

    /* Elemental and poison brands. */
    for (j = 0; j < MAX_P_BRAND; j++)
	if (if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + j))
	    attr_num++;

    if (attr_num > 0) {
	byte attr = TERM_L_UMBER;

	text_out_to_screen(TERM_WHITE, "It does extra damage from ");

	for (j = 0; j < MAX_P_BRAND; j++) {
	    if (!(if_has(o_ptr->id_other, OBJECT_ID_BASE_BRAND + j)))
		continue;
	    text_out_to_screen(attr, brand[j]);
	    if (attr_num >= 3)
		text_out_to_screen(TERM_WHITE, ", ");
	    if (attr_num == 2)
		text_out_to_screen(TERM_WHITE, " and ");
	    if (attr_num == 1)
		text_out_to_screen(TERM_WHITE, ". ");
	    attr_num--;
	}
    }

    /* Throwing weapons. */
    if (of_has(o_ptr->id_obj, OF_THROWING)) {
	if (of_has(o_ptr->id_obj, OF_PERFECT_BALANCE)) {
	    text_out_to_screen(TERM_WHITE, "It can be thrown hard and fast. ");
	} else
	    text_out_to_screen(TERM_WHITE, "It can be thrown effectively. ");
    }

    /* Clear number of items to list. */
    attr_num = 0;

    /* Elemental immunities. */
    for (j = 0; j < 4; j++)
	if ((o_ptr->percent_res[j] == RES_BOOST_IMMUNE)
	    && (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
	    attr_num++;

    if (attr_num > 0) {
	text_out_to_screen(TERM_WHITE, "It provides ");
	text_out_to_screen(TERM_BLUE, "immunity ");
	text_out_to_screen(TERM_WHITE, "to ");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 4; j++) {
	    if (o_ptr->percent_res[j] > RES_BOOST_IMMUNE)
		continue;
	    if (!(if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
		continue;

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		text_out_to_screen(TERM_SLATE, "acid");
	    if (j == 1)
		text_out_to_screen(TERM_BLUE, "electricity");
	    if (j == 2)
		text_out_to_screen(TERM_RED, "fire");
	    if (j == 3)
		text_out_to_screen(TERM_L_WHITE, "frost");
	    if (attr_num >= 3)
		text_out_to_screen(TERM_WHITE, ", ");
	    if (attr_num == 2)
		text_out_to_screen(TERM_WHITE, " and ");
	    if (attr_num == 1)
		text_out_to_screen(TERM_WHITE, ". ");
	    attr_num--;
	}

	/* End sentence.  Go to next line. */
	text_out_to_screen(TERM_WHITE, ". ");
    }

    /* Check for resists and vulnerabilities */
    for (j = 0; j < MAX_P_RES; j++) {
	if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] < 100)
	    && (o_ptr->percent_res[j] > 0)
	    && (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
	    res = TRUE;
	else if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] > 100)
		 && (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
	    vul = TRUE;
    }


    /* Resistances. */
    if (res) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	for (j = 0; j < MAX_P_RES; j++) {
	    if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] < 100)
		&& (o_ptr->percent_res[j] > 0)
		&& (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
		attr_num++;
	}

	/* How many attributes need to be listed? */
	text_out_to_screen(TERM_WHITE, "It provides ");
	text_out_to_screen(TERM_L_BLUE, "resistance ");
	text_out_to_screen(TERM_WHITE, "to");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < MAX_P_RES; j++) {
	    bool list_ok = FALSE;

	    if (j == P_RES_CONFU)
		continue;
	    if (!(if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
		continue;

	    if ((o_ptr->percent_res[j] < 100) && (o_ptr->percent_res[j] > 0))
		list_ok = TRUE;
	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		text_out_to_screen(TERM_WHITE, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    text_out_to_screen(TERM_WHITE, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == P_RES_ACID)
		text_out_to_screen(TERM_SLATE, " acid");
	    if (j == P_RES_ELEC)
		text_out_to_screen(TERM_BLUE, " electricity");
	    if (j == P_RES_FIRE)
		text_out_to_screen(TERM_RED, " fire");
	    if (j == P_RES_COLD)
		text_out_to_screen(TERM_L_WHITE, " frost");
	    if (j == P_RES_POIS)
		text_out_to_screen(TERM_GREEN, " poison");
	    if (j == P_RES_LITE)
		text_out_to_screen(TERM_ORANGE, " light");
	    if (j == P_RES_DARK)
		text_out_to_screen(TERM_L_DARK, " darkness");
	    if (j == P_RES_SOUND)
		text_out_to_screen(TERM_YELLOW, " sound");
	    if (j == P_RES_SHARD)
		text_out_to_screen(TERM_UMBER, " shards");
	    if (j == P_RES_NEXUS)
		text_out_to_screen(TERM_L_RED, " nexus");
	    if (j == P_RES_NETHR)
		text_out_to_screen(TERM_L_GREEN, " nether");
	    if (j == P_RES_CHAOS)
		text_out_to_screen(TERM_VIOLET, " chaos");
	    if (j == P_RES_DISEN)
		text_out_to_screen(TERM_VIOLET, " disenchantment");

	    sprintf(buf, "(%d%%)", 100 - o_ptr->percent_res[j]);
	    text_out_to_screen(TERM_WHITE, buf);
	}

	/* End sentence.  Go to next line. */
	text_out_to_screen(TERM_WHITE, ". ");
    }


    /* Vulnerabilities. */
    if (vul) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	for (j = 0; j < MAX_P_RES; j++) {
	    if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] > 100)
		&& (if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
		attr_num++;
	}

	text_out_to_screen(TERM_WHITE, "It makes you ");
	text_out_to_screen(TERM_ORANGE, "vulnerable ");
	text_out_to_screen(TERM_WHITE, "to");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < MAX_P_RES; j++) {
	    bool list_ok = FALSE;

	    if (j == P_RES_CONFU)
		continue;
	    if (!(if_has(o_ptr->id_other, OBJECT_ID_BASE_RESIST + j)))
		continue;

	    if (o_ptr->percent_res[j] > 100)
		list_ok = TRUE;

	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		text_out_to_screen(TERM_WHITE, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    text_out_to_screen(TERM_WHITE, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == P_RES_ACID)
		text_out_to_screen(TERM_SLATE, " acid");
	    if (j == P_RES_ELEC)
		text_out_to_screen(TERM_BLUE, " electricity");
	    if (j == P_RES_FIRE)
		text_out_to_screen(TERM_RED, " fire");
	    if (j == P_RES_COLD)
		text_out_to_screen(TERM_L_WHITE, " frost");
	    if (j == P_RES_POIS)
		text_out_to_screen(TERM_GREEN, " poison");
	    if (j == P_RES_LITE)
		text_out_to_screen(TERM_ORANGE, " light");
	    if (j == P_RES_DARK)
		text_out_to_screen(TERM_L_DARK, " darkness");
	    if (j == P_RES_SOUND)
		text_out_to_screen(TERM_YELLOW, " sound");
	    if (j == P_RES_SHARD)
		text_out_to_screen(TERM_UMBER, " shards");
	    if (j == P_RES_NEXUS)
		text_out_to_screen(TERM_L_RED, " nexus");
	    if (j == P_RES_NETHR)
		text_out_to_screen(TERM_L_GREEN, " nether");
	    if (j == P_RES_CHAOS)
		text_out_to_screen(TERM_VIOLET, " chaos");
	    if (j == P_RES_DISEN)
		text_out_to_screen(TERM_VIOLET, " disenchantment");

	    sprintf(buf, "(%d%%)", o_ptr->percent_res[j] - 100);
	    text_out_to_screen(TERM_WHITE, buf);
	}

	/* End sentence.  Go to next line. */
	text_out_to_screen(TERM_WHITE, ". ");
    }


    /* Clear a listing variable. */
    attr_num = 0;

    /* Special processing for the three "survival resists" */
    if (of_has(o_ptr->id_obj, OF_FEARLESS))
	attr_num++;
    if (of_has(o_ptr->id_obj, OF_SEEING))
	attr_num++;
    if (if_has(o_ptr->id_other, IF_RES_CONFU))
	attr_num++;

    if (of_has(o_ptr->id_obj, OF_FEARLESS)) {
	text_out_to_screen(TERM_WHITE, "It renders you fearless");
	if (attr_num == 1)
	    text_out_to_screen(TERM_WHITE, ".  ");
	else
	    text_out_to_screen(TERM_WHITE, ", and");
    }

    if (of_has(o_ptr->id_obj, OF_SEEING)) {
	if ((attr_num > 1) && (of_has(o_ptr->id_obj, OF_FEARLESS)))
	    text_out_to_screen(TERM_WHITE, " provides resistance to blindness");
	else
	    text_out_to_screen(TERM_WHITE,
			       "It provides resistance to blindness");

	if (if_has(o_ptr->id_other, IF_RES_CONFU))
	    text_out_to_screen(TERM_WHITE, " and");
	else
	    text_out_to_screen(TERM_WHITE, ".  ");
    }

    if (if_has(o_ptr->id_other, IF_RES_CONFU)) {
	if ((o_ptr->percent_res[P_RES_CONFU] < 100)
	    && (o_ptr->percent_res[P_RES_CONFU] > 0)) {
	    if ((attr_num > 1) && (!(of_has(o_ptr->id_obj, OF_SEEING))))
		text_out_to_screen(TERM_WHITE, " provides resistance to");
	    else if (attr_num == 1)
		text_out_to_screen(TERM_WHITE, "It provides resistance to");
	    text_out_to_screen(TERM_L_UMBER, " confusion");
	    sprintf(buf, "(%d%%)", 100 - o_ptr->percent_res[P_RES_CONFU]);
	    text_out_to_screen(TERM_WHITE, buf);
	    text_out_to_screen(TERM_WHITE, ".  ");
	} else if (o_ptr->percent_res[P_RES_CONFU] > 100) {
	    if (attr_num > 1)
		text_out_to_screen(TERM_WHITE, " makes you vulnerable to ");
	    else
		text_out_to_screen(TERM_WHITE, "It makes you vulnerable to ");
	    text_out_to_screen(TERM_L_UMBER, "confusion");
	    sprintf(buf, "(%d%%)", o_ptr->percent_res[P_RES_CONFU] - 100);
	    text_out_to_screen(TERM_WHITE, buf);
	    text_out_to_screen(TERM_WHITE, ". ");
	}
    }


    /* Miscellaneous abilities. */
    if ((of_has(o_ptr->id_obj, OF_SLOW_DIGEST)) || (of_has(o_ptr->id_obj, OF_FEATHER))
	|| (of_has(o_ptr->id_obj, OF_LITE)) || (of_has(o_ptr->id_obj, OF_REGEN))
	|| (of_has(o_ptr->id_obj, OF_TELEPATHY)) || (of_has(o_ptr->id_obj, OF_SEE_INVIS))
	|| (of_has(o_ptr->id_obj, OF_FREE_ACT)) || (of_has(o_ptr->id_obj, OF_HOLD_LIFE))
	|| (of_has(o_ptr->id_obj, OF_IMPACT)) || (of_has(o_ptr->id_obj, OF_BLESSED))
	|| (of_has(o_ptr->id_obj, OF_CHAOTIC) || (of_has(o_ptr->id_obj, OF_DARKNESS)))) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	/* How many attributes need to be listed? */
	if (of_has(o_ptr->id_obj, OF_SLOW_DIGEST))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_FEATHER))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_LITE))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_REGEN))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_TELEPATHY))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_SEE_INVIS))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_FREE_ACT))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_HOLD_LIFE))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_IMPACT))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_BLESSED))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_CHAOTIC))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_DARKNESS))
	    attr_num++;

	text_out_to_screen(TERM_WHITE, "It");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 12; j++) {
	    bool list_ok = FALSE;

	    if ((j == 0) && (of_has(o_ptr->id_obj, OF_SLOW_DIGEST)))
		list_ok = TRUE;
	    if ((j == 1) && (of_has(o_ptr->id_obj, OF_FEATHER)))
		list_ok = TRUE;
	    if ((j == 2) && (of_has(o_ptr->id_obj, OF_LITE)))
		list_ok = TRUE;
	    if ((j == 3) && (of_has(o_ptr->id_obj, OF_REGEN)))
		list_ok = TRUE;
	    if ((j == 4) && (of_has(o_ptr->id_obj, OF_TELEPATHY)))
		list_ok = TRUE;
	    if ((j == 5) && (of_has(o_ptr->id_obj, OF_SEE_INVIS)))
		list_ok = TRUE;
	    if ((j == 6) && (of_has(o_ptr->id_obj, OF_FREE_ACT)))
		list_ok = TRUE;
	    if ((j == 7) && (of_has(o_ptr->id_obj, OF_HOLD_LIFE)))
		list_ok = TRUE;
	    if ((j == 8) && (of_has(o_ptr->id_obj, OF_IMPACT)))
		list_ok = TRUE;
	    if ((j == 9) && (of_has(o_ptr->id_obj, OF_BLESSED)))
		list_ok = TRUE;
	    if ((j == 10) && (of_has(o_ptr->id_obj, OF_CHAOTIC)))
		list_ok = TRUE;
	    if ((j == 11) && (of_has(o_ptr->id_obj, OF_DARKNESS)))
		list_ok = TRUE;

	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		text_out_to_screen(TERM_WHITE, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    text_out_to_screen(TERM_WHITE, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		text_out_to_screen(TERM_WHITE, " slows your metabolism");
	    if (j == 1)
		text_out_to_screen(TERM_WHITE, " induces feather falling");
	    if (j == 2)
		text_out_to_screen(TERM_WHITE, " provides permanent light");
	    if (j == 3)
		text_out_to_screen(TERM_WHITE,
				   " speeds your regenerative powers");
	    if (j == 4)
		text_out_to_screen(TERM_WHITE, " gives telepathic powers");
	    if (j == 5)
		text_out_to_screen(TERM_WHITE,
				   " allows you to see invisible monsters");
	    if (j == 6)
		text_out_to_screen(TERM_WHITE,
				   " provides immunity to paralysis");
	    if (j == 7)
		text_out_to_screen(TERM_WHITE,
				   " provides resistance to life draining");
	    if (j == 8)
		text_out_to_screen(TERM_WHITE, " induces earthquakes");
	    if (j == 9)
		text_out_to_screen(TERM_WHITE, " has been blessed by the gods");
	    if (j == 10)
		text_out_to_screen(TERM_WHITE, " causes chaotic effects");
	    if (j == 11)
		text_out_to_screen(TERM_WHITE,
				   " allows you to see in the dark");
	}

	/* End sentence.  Go to next line. */
	text_out_to_screen(TERM_WHITE, ". ");
    }


    /* Nastyness. */
    if (known_cursed_p(o_ptr)
	|| ((o_ptr->ident & IDENT_STORE)
	    && (of_has(o_ptr->flags_obj, OF_SHOW_CURSE)))) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	/* How many attributes need to be listed? */
	if (cf_has(o_ptr->flags_id, CF_TELEPORT))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_NO_TELEPORT))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_AGGRO_PERM))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_AGGRO_RAND))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_SLOW_REGEN))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_AFRAID))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_HUNGRY))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_POIS_RAND))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_POIS_RAND_BAD))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_CUT_RAND))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_CUT_RAND_BAD))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_HALLU_RAND))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_DROP_WEAPON))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_ATTRACT_DEMON))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_ATTRACT_UNDEAD))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_STICKY_CARRY))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_STICKY_WIELD))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_PARALYZE))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_PARALYZE_ALL))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_DRAIN_EXP))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_DRAIN_MANA))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_DRAIN_STAT))
	    attr_num++;
	if (cf_has(o_ptr->flags_id, CF_DRAIN_CHARGE))
	    attr_num++;

	text_out_to_screen(TERM_WHITE, "It is cursed");
	if (attr_num)
	    text_out_to_screen(TERM_WHITE, "; it");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 23; j++) {
	    bool list_ok = FALSE;

	    if ((j == 0) && (cf_has(o_ptr->flags_id, CF_TELEPORT)))
		list_ok = TRUE;
	    if ((j == 1) && (cf_has(o_ptr->flags_id, CF_NO_TELEPORT)))
		list_ok = TRUE;
	    if ((j == 2) && (cf_has(o_ptr->flags_id, CF_AGGRO_PERM)))
		list_ok = TRUE;
	    if ((j == 3) && (cf_has(o_ptr->flags_id, CF_AGGRO_RAND)))
		list_ok = TRUE;
	    if ((j == 4) && (cf_has(o_ptr->flags_id, CF_SLOW_REGEN)))
		list_ok = TRUE;
	    if ((j == 5) && (cf_has(o_ptr->flags_id, CF_AFRAID)))
		list_ok = TRUE;
	    if ((j == 6) && (cf_has(o_ptr->flags_id, CF_HUNGRY)))
		list_ok = TRUE;
	    if ((j == 7) && (cf_has(o_ptr->flags_id, CF_POIS_RAND)))
		list_ok = TRUE;
	    if ((j == 8) && (cf_has(o_ptr->flags_id, CF_POIS_RAND_BAD)))
		list_ok = TRUE;
	    if ((j == 9) && (cf_has(o_ptr->flags_id, CF_CUT_RAND)))
		list_ok = TRUE;
	    if ((j == 10) && (cf_has(o_ptr->flags_id, CF_CUT_RAND_BAD)))
		list_ok = TRUE;
	    if ((j == 11) && (cf_has(o_ptr->flags_id, CF_HALLU_RAND)))
		list_ok = TRUE;
	    if ((j == 12) && (cf_has(o_ptr->flags_id, CF_DROP_WEAPON)))
		list_ok = TRUE;
	    if ((j == 13) && (cf_has(o_ptr->flags_id, CF_ATTRACT_DEMON)))
		list_ok = TRUE;
	    if ((j == 14) && (cf_has(o_ptr->flags_id, CF_ATTRACT_UNDEAD)))
		list_ok = TRUE;
	    if ((j == 15) && (cf_has(o_ptr->flags_id, CF_STICKY_CARRY)))
		list_ok = TRUE;
	    if ((j == 16) && (cf_has(o_ptr->flags_id, CF_STICKY_WIELD)))
		list_ok = TRUE;
	    if ((j == 17) && (cf_has(o_ptr->flags_id, CF_PARALYZE)))
		list_ok = TRUE;
	    if ((j == 18) && (cf_has(o_ptr->flags_id, CF_PARALYZE_ALL)))
		list_ok = TRUE;
	    if ((j == 19) && (cf_has(o_ptr->flags_id, CF_DRAIN_EXP)))
		list_ok = TRUE;
	    if ((j == 20) && (cf_has(o_ptr->flags_id, CF_DRAIN_MANA)))
		list_ok = TRUE;
	    if ((j == 21) && (cf_has(o_ptr->flags_id, CF_DRAIN_STAT)))
		list_ok = TRUE;
	    if ((j == 22) && (cf_has(o_ptr->flags_id, CF_DRAIN_CHARGE)))
		list_ok = TRUE;

	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		text_out_to_screen(TERM_WHITE, ",");

	    /* "and" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    text_out_to_screen(TERM_WHITE, " and");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		text_out_to_screen(TERM_WHITE, " induces random teleportation");
	    if (j == 1)
		text_out_to_screen(TERM_WHITE, " prevents teleportation");
	    if (j == 2)
		text_out_to_screen(TERM_WHITE, " aggravates nearby creatures");
	    if (j == 3)
		text_out_to_screen(TERM_WHITE,
				   " occasionally greatly aggravates nearby creatures");
	    if (j == 4)
		text_out_to_screen(TERM_WHITE, " slows your regeneration");
	    if (j == 5)
		text_out_to_screen(TERM_WHITE, " makes you afraid");
	    if (j == 6)
		text_out_to_screen(TERM_WHITE, " speeds your digestion");
	    if (j == 7)
		text_out_to_screen(TERM_WHITE, " occasionally poisons you");
	    if (j == 8)
		text_out_to_screen(TERM_WHITE,
				   " randomly envelops you in poison");
	    if (j == 9)
		text_out_to_screen(TERM_WHITE, " occasionally cuts you");
	    if (j == 10)
		text_out_to_screen(TERM_WHITE,
				   " will suddenly cause deep wounds");
	    if (j == 11)
		text_out_to_screen(TERM_WHITE,
				   " sometimes makes you hallucinate");
	    if (j == 12)
		text_out_to_screen(TERM_WHITE,
				   " will suddenly leap from your grasp");
	    if (j == 13)
		text_out_to_screen(TERM_WHITE,
				   " makes demons suddenly appear nearby");
	    if (j == 14)
		text_out_to_screen(TERM_WHITE,
				   " calls the undead from their slumber");
	    if (j == 15)
		text_out_to_screen(TERM_WHITE,
				   " cannot be dropped from your pack");
	    if (j == 16)
		text_out_to_screen(TERM_WHITE, " sticks to you if wielded");
	    if (j == 17)
		text_out_to_screen(TERM_WHITE, " will briefly paralyze you");
	    if (j == 18)
		text_out_to_screen(TERM_WHITE,
				   " can paralyze you even if you feel immune");
	    if (j == 19)
		text_out_to_screen(TERM_WHITE, " drains experience");
	    if (j == 20)
		text_out_to_screen(TERM_WHITE, " drains mana");
	    if (j == 21)
		text_out_to_screen(TERM_WHITE, " will sometimes lower a stat");
	    if (j == 22)
		text_out_to_screen(TERM_WHITE,
				   " will take energy from your magic devices");
	}

	/* End sentence.  Go to next line. */
	text_out_to_screen(TERM_WHITE, ".  ");

	/* Say if the curse is permanent */
	if (of_has(o_ptr->id_obj, OF_PERMA_CURSE))
	    text_out_to_screen(TERM_WHITE, "It cannot be uncursed. ");

	/* Say if curse removal has been tried */
	if (of_has(o_ptr->id_obj, OF_FRAGILE))
	    text_out_to_screen(TERM_WHITE,
			       "Attempting to uncurse it may destroy it. ");
    }

    /* Ignore various elements. */
    if ((of_has(o_ptr->id_obj, OF_ACID_PROOF)) || (of_has(o_ptr->id_obj, OF_ELEC_PROOF))
	|| (of_has(o_ptr->id_obj, OF_FIRE_PROOF)) || (of_has(o_ptr->id_obj, OF_COLD_PROOF))) {
	/* Clear number of items to list, and items listed. */
	attr_num = 0;
	attr_listed = 0;

	/* How many attributes need to be listed? */
	if (of_has(o_ptr->id_obj, OF_ACID_PROOF))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_ELEC_PROOF))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_FIRE_PROOF))
	    attr_num++;
	if (of_has(o_ptr->id_obj, OF_COLD_PROOF))
	    attr_num++;

	text_out_to_screen(TERM_WHITE, "It cannot be damaged by");

	/* Loop for number of attributes in this group. */
	for (j = 0; j < 4; j++) {
	    bool list_ok = FALSE;

	    if ((j == 0) && (of_has(o_ptr->id_obj, OF_ACID_PROOF)))
		list_ok = TRUE;
	    if ((j == 1) && (of_has(o_ptr->id_obj, OF_ELEC_PROOF)))
		list_ok = TRUE;
	    if ((j == 2) && (of_has(o_ptr->id_obj, OF_FIRE_PROOF)))
		list_ok = TRUE;
	    if ((j == 3) && (of_has(o_ptr->id_obj, OF_COLD_PROOF)))
		list_ok = TRUE;

	    if (!list_ok)
		continue;

	    /* Listing another attribute. */
	    attr_listed++;

	    /* Commas separate members of a list of more than two. */
	    if ((attr_num > 2) && (attr_listed > 1))
		text_out_to_screen(TERM_WHITE, ",");

	    /* "or" before final member of a list of more than one. */
	    if ((attr_num > 1) && (j != 0)) {
		if (attr_num == attr_listed)
		    text_out_to_screen(TERM_WHITE, " or");
	    }

	    /* List the attribute description, in its proper place. */
	    if (j == 0)
		text_out_to_screen(TERM_WHITE, " acid");
	    if (j == 1)
		text_out_to_screen(TERM_WHITE, " electricity");
	    if (j == 2)
		text_out_to_screen(TERM_WHITE, " fire");
	    if (j == 3)
		text_out_to_screen(TERM_WHITE, " frost");
	}

	/* End sentence.  Go to next line. */
	text_out_to_screen(TERM_WHITE, ". ");
    }

    /* Only look at wieldables */
    if (wield_slot(o_ptr) >= INVEN_WIELD) {
	/* All normal properties known */
	if (o_ptr->ident & IDENT_KNOWN) {
	    /* Everything known */
	    if (o_ptr->ident & (IDENT_KNOW_CURSES | IDENT_UNCURSED))
		text_out_to_screen(TERM_WHITE,
				   "You know all about this object.");

	    /* Some unknown curses */
	    else
		text_out_to_screen(TERM_WHITE,
				   "You know all the enchantments on this object. ");
	}

	/* Curses known */
	else {
	    /* Known uncursed */
	    if (o_ptr->ident & IDENT_UNCURSED)
		text_out_to_screen(TERM_WHITE, "It is not cursed. ");

	    /* Say if all curses are known */
	    else if (o_ptr->ident & IDENT_KNOW_CURSES)
		text_out_to_screen(TERM_WHITE,
				   "You know all the curses on this object. ");
	}
    }

}


/**
 * Place an item description on the screen.
 */
void object_info_screen(object_type * o_ptr, bool fake)
{
    int y, x;
    int y0, x0;
    int i;

    bool aware, known, known_effects;
    bool in_store = o_ptr->ident & IDENT_STORE;

    object_kind *k_ptr;
    char o_name[120];

    char info_text[2048];
    char *object_kind_info;

    /* Initialize object description. */
    strcpy(info_text, "");

    /* Get the object kind. */
    k_ptr = &k_info[o_ptr->k_idx];

    /* Hack - make sure the object is really in the store */
    for (i = 0; i < INVEN_TOTAL; i++)
	if (o_ptr == &p_ptr->inventory[i])
	    in_store = FALSE;

    /* Create and output a status message (hack - not in stores). */
    if (in_store)
	object_desc_store(o_name, o_ptr, TRUE, 3);
    else
	object_desc(o_name, o_ptr, TRUE, 3);


    /* What is our level of knowledge about the object? */
    aware = object_aware_p(o_ptr);
    known = (object_known_p(o_ptr) || in_store);
    known_effects = k_ptr->known_effect;

    /* Object is fully known - give maximal information. */
    if (known || aware) {
	/* Fake artifacts in knowledge screens need special treatment */
	if (fake)
	    object_desc_spoil(o_name, o_ptr, TRUE, 0);
	else
	    /* Get the specific object type's information. */
	    object_info(info_text, o_ptr, in_store);

	/* No object kind info. */
	object_kind_info = "";
    }

    /* Hack - translate if we do that */
    if (Term->xchar_hook)
	xstr_trans(o_name, (Term->xchar_hook(128) == 128));

    /* Save the location */
    Term_locate(&x0, &y0);

    /* Save screen */
    screen_save();

    /* Label the information. */
    Term_gotoxy(0, 0);
    text_out_indent = 0;
    text_out_to_screen(TERM_WHITE, o_name);
    text_out_to_screen(TERM_WHITE, "\n");

    /* Object type or artifact information. */
    text_out_to_screen(TERM_L_BLUE, info_text);

    /* 
     * If it is a set item, describe it as such.
     */
    if ((known) && (o_ptr->name1)) {
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	/* Is it a set item? */
	if (a_ptr->set_no) {
	    set_type *s_ptr = &s_info[a_ptr->set_no];

	    /* Fully ID description? */
	    text_out_to_screen(TERM_WHITE, "  ");
	    strcpy(info_text, s_text + s_ptr->text);
	    text_out_to_screen(TERM_BLUE, info_text);

	    /* End sentence */
	    text_out_to_screen(TERM_BLUE, ".");
	}
    }

    /* Describe the object. */
    text_out_to_screen(TERM_WHITE, "\n");

    /* Fully describe the object flags and attributes. */
    object_info_detail(o_ptr);

    /* Print melee damage info */
    if ((k_ptr->tval == TV_SWORD) || (k_ptr->tval == TV_POLEARM)
	|| (k_ptr->tval == TV_HAFTED) || (k_ptr->tval == TV_DIGGING))
	display_weapon_damage(o_ptr);

    /* Print range damage info */
    if ((is_missile(o_ptr)) || (of_has(o_ptr->flags_obj, OF_THROWING)))
	display_ammo_damage(o_ptr);

    /* Print device chance */
    if ((k_ptr->tval == TV_STAFF) || (k_ptr->tval == TV_WAND)
	|| (k_ptr->tval == TV_ROD))
	display_device_chance(o_ptr);

    /* Obtain the cursor location */
    (void) Term_locate(&x, &y);

    /* Get information about the general object kind. */
    object_kind_info = format("%s", obj_class_info[o_ptr->tval]);

    /* Spacing */
    text_out_to_screen(TERM_WHITE, "\n");

    /* Object kind information. */
    text_out_to_screen(TERM_WHITE, object_kind_info);

    /* Spacing */
    text_out_to_screen(TERM_WHITE, "\n");

    /* Hack -- attempt to stay on screen. */
    for (i = y; i < Term->hgt; i++) {
	/* No more space! */
	if (i > Term->hgt - 2)
	    break;

	/* Advance one line. */
	text_out_to_screen(TERM_WHITE, "\n");

	/* Enough clear space.  Done. */
	if (i == (y + 2))
	    break;
    }

    /* The exit sign. */
    text_out_to_screen(TERM_L_BLUE, "(Press any key to continue.)");
    Term_locate(&x, &y);
    text_out_to_screen(TERM_WHITE, "\n");
    Term_gotoxy(x, y);
    (void) inkey_ex();

    /* Load screen */
    screen_load();
    Term_gotoxy(x0, y0);

}

/**
 * Item description for character sheet/dump.
 * Not used yet - needs new function for displaying this stuff.
 * I have a nasty feeling this whole file needs rewriting...
 */
void object_info_dump(object_type * o_ptr)
{
    object_kind *k_ptr = &k_info[o_ptr->k_idx];
    char info_text[2048];

    /* 
     * If it is a set item, describe it as such.
     */
    if ((o_ptr->ident & IDENT_KNOWN) && (o_ptr->name1)) {
	artifact_type *a_ptr = &a_info[o_ptr->name1];

	/* Is it a set item? */
	if (a_ptr->set_no) {
	    set_type *s_ptr = &s_info[a_ptr->set_no];

	    /* Fully ID description? */
	    strcpy(info_text, s_text + s_ptr->text);
	    text_out_to_screen(TERM_BLUE, info_text);

	    /* End sentence */
	    text_out_to_screen(TERM_BLUE, ".");
	}
    }

    /* Describe the object. */
    text_out_to_screen(TERM_WHITE, "\n");

    /* Fully describe the object flags and attributes. */
    object_info_detail(o_ptr);

    /* Print melee damage info */
    if ((k_ptr->tval == TV_SWORD) || (k_ptr->tval == TV_POLEARM)
	|| (k_ptr->tval == TV_HAFTED) || (k_ptr->tval == TV_DIGGING))
	display_weapon_damage(o_ptr);

    /* Print range damage info */
    if ((is_missile(o_ptr)) || (of_has(o_ptr->flags_obj, OF_THROWING)))
	display_ammo_damage(o_ptr);

    /* Print device chance */
    if ((k_ptr->tval == TV_STAFF) || (k_ptr->tval == TV_WAND)
	|| (k_ptr->tval == TV_ROD))
	display_device_chance(o_ptr);

}




/**
 * Determine if a spell is "okay" for the player to cast or study
 * The spell must be legible, not forgotten, and also, to cast,
 * it must be known, and to study, it must not be known.
 */
bool spell_okay(int spell, bool known)
{
    magic_type *s_ptr;
    /* Access the spell */
    s_ptr = &mp_ptr->info[spell];
    /* Spell is illegal */
    if (s_ptr->slevel > p_ptr->lev)
	return (FALSE);
    /* Spell is forgotten */
    if ((spell < 32) ? (p_ptr->spell_forgotten1 & (1L << spell)) : 
	(p_ptr->spell_forgotten2 & (1L << (spell - 32)))) {
	/* Never okay */
	return (FALSE);
    }

    /* Spell is learned */
    if ((spell < 32) ? (p_ptr->spell_learned1 & (1L << spell)) : 
	(p_ptr->spell_learned2 & (1L << (spell - 32))))
    {
	/* Okay to cast, not to study */

	return (known);
    }

    /* Okay to study, not to cast */
    return (!known);
}





