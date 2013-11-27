/*
 * File: cmd-obj.c
 * Purpose: Handle objects in various ways
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2007-9 Andi Sidwell, Chris Carr, Ed Graham, Erik Osheim
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
#include "effects.h"
#include "game-cmd.h"
#include "game-event.h"
#include "monster.h"
#include "object.h"
#include "target.h"
#include "tvalsval.h"
#include "spells.h"
#include "squelch.h"

/*** Utility bits and bobs ***/

/*
 * Check to see if the player can use a rod/wand/staff/activatable object.
 */
static int check_devices(object_type * o_ptr)
{
	int fail;
	const char *action;
	const char *what = NULL;

	/* Get the right string */
	switch (o_ptr->tval) {
	case TV_ROD:
		action = "zap the rod";
		break;
	case TV_WAND:
		action = "use the wand";
		what = "wand";
		break;
	case TV_STAFF:
		action = "use the staff";
		what = "staff";
		break;
	default:
		action = "activate it";
		break;
	}

	/* Figure out how hard the item is to use */
	fail = get_use_device_chance(o_ptr);

	/* Roll for usage */
	if (randint1(1000) < fail) {
		flush();
		msg("You failed to %s properly.", action);
		return FALSE;
	}

	/* Notice empty staffs */
	if (what && o_ptr->pval <= 0) {
		flush();
		msg("The %s has no charges left.", what);
		o_ptr->ident |= (IDENT_EMPTY);
		return FALSE;
	}

	return TRUE;
}



/*** Inscriptions ***/



/**
 * Remove the inscription from an object
 * XXX Mention item (when done)?
 */
void do_cmd_uninscribe(cmd_code code, cmd_arg args[])
{
	object_type *o_ptr = object_from_item_idx(args[0].item);

	if (obj_has_inscrip(o_ptr))
		msg("Inscription removed.");

	/* Remove the incription */
	o_ptr->note = 0;

	p_ptr->notice |= (PN_COMBINE | PN_SQUELCH | PN_SORT_QUIVER);
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
}


/**
 * Inscribe an object with a comment
 */
void do_cmd_inscribe(cmd_code code, cmd_arg args[])
{
	object_type *o_ptr = object_from_item_idx(args[0].item);

	o_ptr->note = quark_add(args[1].string);

	p_ptr->notice |= (PN_COMBINE | PN_SQUELCH | PN_SORT_QUIVER);
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP);
}

/*** Examination ***/
void textui_obj_examine(void)
{
	char header[120];

	textblock *tb;
	region area = { 0, 0, 0, 0 };

	object_type *o_ptr;
	int item;

	/* Select item */
	if (!get_item
		(&item, "Examine which item?", "You have nothing to examine.",
		 CMD_NULL, (USE_EQUIP | USE_INVEN | USE_FLOOR | IS_HARMLESS)))
		return;

	/* Track object for object recall */
	track_object(item);

	/* Display info */
	o_ptr = object_from_item_idx(item);
	tb = object_info(o_ptr, OINFO_NONE);
	object_desc(header, sizeof(header), o_ptr,
				ODESC_PREFIX | ODESC_FULL | ODESC_CAPITAL);

	textui_textblock_show(tb, area, format("%s", header));
	textblock_free(tb);
}




/*** Taking off/putting on ***/

/**
 * Take off an item, if not shapechanged.
 */
void do_cmd_takeoff(cmd_code code, cmd_arg args[])
{
	int item = args[0].item;

	if (SCHANGE) {
		msg("You cannot take off equipment while shapechanged.");
		msg("Use the ']' command to return to your normal form.");
		return;
	}

	if (!item_is_available(item, NULL, USE_EQUIP)) {
		msg("You are not wielding that item.");
		return;
	}

	if (!obj_can_takeoff(object_from_item_idx(item))) {
		msg("You cannot take off that item.");
		return;
	}

	/* Ensure that the shield hand is used, if a shield is available. */
	if (item == INVEN_WIELD)
		p_ptr->state.shield_on_back = FALSE;

	(void) inven_takeoff(item, 255);
	pack_overflow();
	p_ptr->energy_use = 50;
}

/* Wield or wear an item */
void do_cmd_wield(cmd_code code, cmd_arg args[])
{
	object_type *equip_o_ptr;
	char o_name[80];

	unsigned n;

	int item = args[0].item;
	int slot = args[1].number;
	object_type *o_ptr = object_from_item_idx(item);

	if (SCHANGE) {
		msg("You cannot wield equipment while shapechanged.");
		msg("Use the ']' command to return to your normal form.");
		return;
	}

	if (!item_is_available(item, NULL, USE_INVEN | USE_FLOOR)) {
		msg("You do not have that item to wield.");
		return;
	}

	/* Check the slot */
	if (!slot_can_wield_item(slot, o_ptr)) {
		msg("You cannot wield that item there.");
		return;
	}

	equip_o_ptr = &p_ptr->inventory[slot];

	/* If the slot is open, wield and be done */
	if (!equip_o_ptr->k_idx) {
		wield_item(o_ptr, item, slot);
		return;
	}

	/* If the slot is in the quiver and objects can be combined */
	if (obj_is_quiver_obj(equip_o_ptr) &&
		object_similar(equip_o_ptr, o_ptr, OSTACK_QUIVER)) {
		wield_item(o_ptr, item, slot);
		return;
	}

	/* Prevent wielding into a cursed slot */
	if (cf_has(equip_o_ptr->flags_curse, CF_STICKY_WIELD)) {
		object_desc(o_name, sizeof(o_name), equip_o_ptr, ODESC_BASE);
		msg("The %s you are %s appears to be cursed.", o_name,
			describe_use(slot));
		notice_curse(CF_STICKY_WIELD, slot + 1);
		return;
	}

	/* "!t" checks for taking off */
	n = check_for_inscrip(equip_o_ptr, "!t");
	while (n--) {
		/* Prompt */
		object_desc(o_name, sizeof(o_name), equip_o_ptr,
					ODESC_PREFIX | ODESC_FULL);

		/* Forget it */
		if (!get_check(format("Really take off %s? ", o_name)))
			return;
	}

	wield_item(o_ptr, item, slot);
}


/**
 * Drop an item
 */
void do_cmd_drop(cmd_code code, cmd_arg args[])
{
	int item = args[0].item;
	object_type *o_ptr = object_from_item_idx(item);
	int amt = args[1].number;


	if (!item_is_available(item, NULL, USE_INVEN | USE_EQUIP)) {
		msg("You do not have that item to drop it.");
		return;
	}

	/* Hack -- Cannot drop some cursed items */
	if (cf_has(o_ptr->flags_curse, CF_STICKY_CARRY)) {
		/* Oops */
		msg("Hmmm, it seems to be cursed.");

		/* Notice */
		notice_curse(CF_STICKY_CARRY, item + 1);

		/* Nope */
		return;
	}

	if ((item >= INVEN_WIELD)
		&& cf_has(o_ptr->flags_curse, CF_STICKY_WIELD)) {
		/* Oops */
		msg("Hmmm, it seems to be cursed.");

		/* Notice */
		notice_curse(CF_STICKY_WIELD, item + 1);

		/* Nope */
		return;
	}

	/* Ensure that the shield hand is used, if a shield is available. */
	if (item == INVEN_WIELD)
		p_ptr->state.shield_on_back = FALSE;

	/* Take a partial turn */
	p_ptr->energy_use = 50;

	/* Drop (some of) the item */
	inven_drop(item, amt);
}


/*** Using items the traditional way ***/

/*
 * Use an object the right way.
 *
 * There may be a BIG problem with any "effect" that can cause "changes"
 * to the inventory.  For example, a "scroll of recharging" can cause
 * a wand/staff to "disappear", moving the inventory up.  Luckily, the
 * scrolls all appear BEFORE the staffs/wands, so this is not a problem.
 * But, for example, a "staff of recharging" could cause MAJOR problems.
 * In such a case, it will be best to either (1) "postpone" the effect
 * until the end of the function, or (2) "change" the effect, say, into
 * giving a staff "negative" charges, or "turning a staff into a stick".
 * It seems as though a "rod of recharging" might in fact cause problems.
 * The basic problem is that the act of recharging (and destroying) an
 * item causes the inducer of that action to "move", causing "o_ptr" to
 * no longer point at the correct item, with horrifying results.
 */
void do_cmd_use(cmd_code code, cmd_arg args[])
{
	int item = args[0].item;
	object_type *o_ptr = object_from_item_idx(item);
	int effect;
	bool ident = FALSE, used = FALSE;
	bool was_aware = object_aware_p(o_ptr);
	int dir = 5;
	int snd;
	use_type use;
	int items_allowed = 0;

	/* Determine how this item is used. */
	if (obj_is_rod(o_ptr)) {
		if (!obj_can_zap(o_ptr)) {
			msg("That rod is still charging.");
			return;
		}

		use = USE_TIMEOUT;
		snd = MSG_ZAP_ROD;
		items_allowed = USE_INVEN | USE_FLOOR;
	} else if (obj_is_wand(o_ptr)) {
		if (!obj_has_charges(o_ptr)) {
			msg("That wand has no charges.");
			return;
		}

		use = USE_CHARGE;
		snd = MSG_ZAP_ROD;
		items_allowed = USE_INVEN | USE_FLOOR;
	} else if (obj_is_staff(o_ptr)) {
		if (!obj_has_charges(o_ptr)) {
			msg("That staff has no charges.");
			return;
		}

		use = USE_CHARGE;
		snd = MSG_USE_STAFF;
		items_allowed = USE_INVEN | USE_FLOOR;
	} else if (obj_is_food(o_ptr)) {
		use = USE_SINGLE;
		snd = MSG_EAT;
		items_allowed = USE_INVEN | USE_FLOOR;
	} else if (obj_is_potion(o_ptr)) {
		use = USE_SINGLE;
		snd = MSG_QUAFF;
		items_allowed = USE_INVEN | USE_FLOOR;
	} else if (obj_is_scroll(o_ptr)) {
		/* Check player can use scroll */
		if (!player_can_read())
			return;

		use = USE_SINGLE;
		snd = MSG_GENERIC;
		items_allowed = USE_INVEN | USE_FLOOR;
	} else if (obj_is_activatable(o_ptr)) {
		if (!obj_can_activate(o_ptr)) {
			msg("That item is still charging.");
			return;
		}

		use = USE_TIMEOUT;
		snd = MSG_ACT_ARTIFACT;
		items_allowed = USE_EQUIP;
	} else {
		msg("The item cannot be used at the moment");
	}

	/* Check if item is within player's reach. */
	if (items_allowed == 0
		|| !item_is_available(item, NULL, items_allowed)) {
		msg("You cannot use that item from its current location.");
		return;
	}

	/* track the object used */
	track_object(item);

	/* Figure out effect to use */
	effect = object_effect(o_ptr);

	/* If the item requires a direction, get one (allow cancelling) */
	if (obj_needs_aim(o_ptr))
		dir = args[1].direction;

	/* Hack for dragonform */
	if ((effect >= EF_DRAGON_BLACK) && (effect <= EF_DRAGON_POWER)
		&& (p_ptr->schange == SHAPE_WYRM))
		dir = TRUE;

	/* Check for use if necessary, and execute the effect */
	if ((use != USE_CHARGE && use != USE_TIMEOUT) || check_devices(o_ptr)) {
		/* Special message for artifacts */
		if (artifact_p(o_ptr) && wearable_p(o_ptr)) {
			msgt(snd, "You activate it.");
			if (a_info[o_ptr->name1].effect_msg)
				msg(a_info[o_ptr->name1].effect_msg);
		} else {
			/* Make a noise! */
			sound(snd);
		}

		/* Do effect */
		used = effect_do(effect, &ident, was_aware, dir);

		/* Quit if the item wasn't used and no knowledge was gained */
		if (!used && (was_aware || !ident))
			return;
	}

	/* If the item is a null pointer or has been wiped, be done now */
	if (!o_ptr || !o_ptr->kind)
		return;

	/* Food feeds the player */
	if (o_ptr->tval == TV_FOOD || o_ptr->tval == TV_POTION)
		(void) set_food(p_ptr->food + o_ptr->pval);

	/* Use the turn */
	p_ptr->energy_use = 100;

	/* Mark as tried and redisplay */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	p_ptr->redraw |= (PR_INVEN | PR_EQUIP | PR_OBJECT);

	/* 
	 * If the player becomes aware of the item's function, then mark it as
	 * aware and reward the player with some experience.  Otherwise, mark
	 * it as "tried".
	 */
	if (ident && !was_aware) {
		/* Object level */
		int lev = k_info[o_ptr->k_idx].level;

		object_aware(o_ptr);
		gain_exp((lev + (p_ptr->lev / 2)) / p_ptr->lev);
		p_ptr->notice |= PN_SQUELCH;
	} else if (used) {
		object_tried(o_ptr);
	}

	/* If there are no more of the item left, then we're done. */
	if (!o_ptr->number)
		return;

	/* Chargeables act differently to single-used items when not used up */
	if (used && use == USE_CHARGE) {
		/* Use a single charge */
		o_ptr->pval--;

		/* Describe charges */
		if (item >= 0)
			inven_item_charges(item);
		else
			floor_item_charges(0 - item);
	} else if (used && use == USE_TIMEOUT) {
		if (o_ptr->time.base)
			o_ptr->timeout += randcalc(o_ptr->time, 0, RANDOMISE);
	} else if (used && use == USE_SINGLE) {
		/* Destroy a potion in the pack */
		if (item >= 0) {
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}

		/* Destroy a potion on the floor */
		else {
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}
	}

}


/*** Refuelling ***/
/**
 * Refill the players lamp (from the pack or floor)
 */
static void refill_lamp(object_type * j_ptr, object_type * o_ptr, int item)
{
	/* Refuel */
	j_ptr->pval += o_ptr->pval;

	/* Message */
	msg("You fuel your lamp.");

	/* Comment */
	if (j_ptr->pval >= FUEL_LAMP) {
		j_ptr->pval = FUEL_LAMP;
		msg("Your lamp is full.");
	}

	/* Use fuel from a lantern */
	if (o_ptr->sval == SV_LIGHT_LANTERN) {
		/* No more fuel */
		o_ptr->pval = 0;

		/* Combine / Reorder the pack (later) */
		p_ptr->notice |= (PN_COMBINE | PN_REORDER);
	}

	/* Decrease the item (from the pack) */
	else if (item >= 0) {
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Decrease the item (from the floor) */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_EQUIP);
}



/**
 * Refuel the players torch (from the pack or floor)
 */
static void refuel_torch(object_type * j_ptr, object_type * o_ptr,
						 int item)
{
	/* Access the primary torch */
	j_ptr = &p_ptr->inventory[INVEN_LIGHT];

	/* Refuel */
	j_ptr->pval += o_ptr->pval + 5;

	/* Message */
	msg("You combine the torches.");

	/* Over-fuel message */
	if (j_ptr->pval >= FUEL_TORCH) {
		j_ptr->pval = FUEL_TORCH;
		msg("Your torch is fully fueled.");
	}

	/* Refuel message */
	else {
		msg("Your torch glows more brightly.");
	}

	/* Decrease the item (from the pack) */
	if (item >= 0) {
		inven_item_increase(item, -1);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Decrease the item (from the floor) */
	else {
		floor_item_increase(0 - item, -1);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	/* Recalculate torch */
	p_ptr->update |= (PU_TORCH);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_EQUIP);
}



void do_cmd_refill(cmd_code code, cmd_arg args[])
{
	object_type *j_ptr = &p_ptr->inventory[INVEN_LIGHT];

	int item = args[0].item;
	object_type *o_ptr = object_from_item_idx(item);

	if (!item_is_available(item, NULL, USE_INVEN | USE_FLOOR)) {
		msg("You do not have that item to refill with it.");
		return;
	}

	if (j_ptr->tval != TV_LIGHT) {
		msg("You are not wielding a light.");
		return;
	}

	/* It's a lamp */
	else if (j_ptr->sval == SV_LIGHT_LANTERN)
		refill_lamp(j_ptr, o_ptr, item);

	/* It's a torch */
	else if (j_ptr->sval == SV_LIGHT_TORCH)
		refuel_torch(j_ptr, o_ptr, item);

	else {
		msg("Your light cannot be refilled.");
		return;
	}

	p_ptr->energy_use = 50;
}



/*** Spell casting ***/


/**
 * Study a book to gain a new spell/prayer
 */
void do_cmd_study_spell(cmd_code code, cmd_arg args[])
{
	int spell = args[0].choice;

	int item_list[INVEN_TOTAL + MAX_FLOOR_STACK];
	int item_num;
	int i;

	/* Check the player can study at all atm */
	if (!player_can_study())
		return;

	/* Check that the player can actually learn the nominated spell. */
	item_tester_hook = obj_can_browse;
	item_num =
		scan_items(item_list, N_ELEMENTS(item_list),
				   (USE_INVEN | USE_FLOOR));

	/* Check through all available books */
	for (i = 0; i < item_num; i++) {
		if (spell_in_book(spell, item_list[i])) {
			if (spell_okay_to_study(spell)) {
				/* Spell is in an available book, and player is capable. */
				spell_learn(spell);
				p_ptr->energy_use = 100;
			} else {
				/* Spell is present, but player incapable. */
				msg("You cannot learn that %s.",
					magic_desc[mp_ptr->spell_realm][SPELL_NOUN]);
			}

			return;
		}
	}

	/* Forget the item_tester_hook */
	item_tester_hook = NULL;

}

/**
 * Calculate level boost for Channeling ability.
 */
int get_channeling_boost(void)
{
	long max_channeling = 45 + (2 * p_ptr->lev);
	long channeling = 0L;
	int boost;

	if (p_ptr->msp > 0)
		channeling = (max_channeling * p_ptr->csp * p_ptr->csp)
			/ (p_ptr->msp * p_ptr->msp);
	boost = ((int) channeling + 5) / 10;

	return (boost);
}


/**
 * Warriors will eventually learn to pseudo-probe monsters.  This allows 
 * them to better choose between slays and brands.  They select a target, 
 * and receive (slightly incomplete) infomation about racial type, 
 * basic resistances, and HPs. -LM-
 */
void pseudo_probe(void)
{
	char m_name[80];

	/* Acquire the target monster */
	struct monster *m_ptr = target_get_monster();
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_lore *l_ptr = &l_list[m_ptr->r_idx];

	int approx_hp;
	int approx_mana = 0;


	/* If no target monster, fail. */
	if (!m_ptr) {
		msg("You must actually target a monster.");
		return;
	}

	else {
		/* Get "the monster" or "something" */
		monster_desc(m_name, sizeof(m_name), m_ptr, 0x104);

		/* Approximate monster HPs */
		approx_hp =
			m_ptr->hp - randint0(m_ptr->hp / 4) + randint0(m_ptr->hp / 4);

		/* Approximate monster HPs */
		if (r_ptr->mana)
			approx_mana =
				m_ptr->mana - randint0(m_ptr->mana / 4) +
				randint0(m_ptr->mana / 4);

		/* Describe the monster */
		if (!(r_ptr->mana))
			msg("%s has about %d hit points.", m_name, approx_hp);
		else
			msg("%s has about %d hit points and about %d mana.", m_name,
				approx_hp, approx_mana);

		/* Learn some flags.  Chance of omissions. */
		if ((rf_has(r_ptr->flags, RF_ANIMAL)) && (randint0(20) != 1))
			rf_on(l_ptr->flags, RF_ANIMAL);
		if ((rf_has(r_ptr->flags, RF_EVIL)) && (randint0(10) != 1))
			rf_on(l_ptr->flags, RF_EVIL);
		if ((rf_has(r_ptr->flags, RF_UNDEAD)) && (randint0(20) != 1))
			rf_on(l_ptr->flags, RF_UNDEAD);
		if ((rf_has(r_ptr->flags, RF_DEMON)) && (randint0(20) != 1))
			rf_on(l_ptr->flags, RF_DEMON);
		if ((rf_has(r_ptr->flags, RF_ORC)) && (randint0(20) != 1))
			rf_on(l_ptr->flags, RF_ORC);
		if ((rf_has(r_ptr->flags, RF_TROLL)) && (randint0(20) != 1))
			rf_on(l_ptr->flags, RF_TROLL);
		if ((rf_has(r_ptr->flags, RF_GIANT)) && (randint0(10) != 1))
			rf_on(l_ptr->flags, RF_GIANT);
		if ((rf_has(r_ptr->flags, RF_DRAGON)) && (randint0(20) != 1))
			rf_on(l_ptr->flags, RF_DRAGON);
		if ((rf_has(r_ptr->flags, RF_IM_ACID)) && (randint0(5) != 1))
			rf_on(l_ptr->flags, RF_IM_ACID);
		if ((rf_has(r_ptr->flags, RF_IM_ELEC)) && (randint0(5) != 1))
			rf_on(l_ptr->flags, RF_IM_ELEC);
		if ((rf_has(r_ptr->flags, RF_IM_FIRE)) && (randint0(5) != 1))
			rf_on(l_ptr->flags, RF_IM_FIRE);
		if ((rf_has(r_ptr->flags, RF_IM_COLD)) && (randint0(5) != 1))
			rf_on(l_ptr->flags, RF_IM_COLD);
		if ((rf_has(r_ptr->flags, RF_IM_POIS)) && (randint0(5) != 1))
			rf_on(l_ptr->flags, RF_IM_POIS);

		/* Confirm success. */
		msg("You feel you know more about this monster...");

		/* Update monster recall window */
		if (p_ptr->monster_race_idx == m_ptr->r_idx) {
			event_signal(EVENT_MONSTERTARGET);
		}
	}
}

/**
 * Cast a spell or pray a prayer.
 */
void do_cmd_cast(cmd_code code, cmd_arg args[])
{
	int spell = args[0].choice;
	int dir = args[1].direction;

	int item_list[INVEN_TOTAL + MAX_FLOOR_STACK];
	int item_num;
	int i;


	/* Require spell ability. */
	if (!player_can_cast())
		return;

	/* Warriors will eventually learn to pseudo-probe monsters. */
	if (player_has(PF_PROBE)) {
		/* Get a target. */
		msg("Target a monster to probe.");
		if (!get_aim_dir(&dir))
			return;

		/* Low-level probe spell. */
		pseudo_probe();

		/* Take a turn */
		p_ptr->energy_use = 100;
		return;
	}

	/* Restrict choices to spell books of the player's realm. */
	item_tester_hook = obj_can_browse;
	item_num =
		scan_items(item_list, N_ELEMENTS(item_list),
				   (USE_INVEN | USE_FLOOR));


	/* Check through all available books */
	for (i = 0; i < item_num; i++) {
		if (spell_in_book(spell, item_list[i])) {
			if (spell_okay_to_cast(spell)) {
				/* Get the spell */
				const magic_type *s_ptr = &mp_ptr->info[spell];

				/* Verify "dangerous" spells */
				if (s_ptr->smana > p_ptr->csp) {
					/* Warning */
					msg("You do not have enough mana to %s this %s.",
						magic_desc[mp_ptr->spell_realm][SPELL_VERB],
						magic_desc[mp_ptr->spell_realm][SPELL_NOUN]);

					/* Flush input */
					flush();

					/* Verify */
					if (!get_check("Attempt it anyway? "))
						return;
				}

				/* Cast a spell */
				if (spell_cast(spell, dir)) {
					/* Take a turn */
					p_ptr->energy_use = 100;

					/* Reduced for fast casters */
					if (player_has(PF_FAST_CAST)) {
						int level_difference = p_ptr->lev - s_ptr->slevel;

						p_ptr->energy_use -= 5 + (level_difference / 2);

						/* Increased bonus for really easy spells */
						if (level_difference > 25)
							p_ptr->energy_use -= (level_difference - 25);
					}

					/* Give Credit for Heighten Magic */
					if (player_has(PF_HEIGHTEN_MAGIC))
						add_heighten_power(20);

					/* Paranioa */
					if (p_ptr->energy_use < 50)
						p_ptr->energy_use = 50;
				} else {
					/* Spell is present, but player incapable. */
					msg("You cannot %s that %s.",
						magic_desc[mp_ptr->spell_realm][SPELL_VERB],
						magic_desc[mp_ptr->spell_realm][SPELL_NOUN]);
				}

				return;
			}
		}

	}
}

/**
 * Study a book to gain a new prayer
 */
void do_cmd_study_book(cmd_code code, cmd_arg args[])
{
	int book = args[0].item;
	object_type *o_ptr = object_from_item_idx(book);

	int total_spells, spell = -1;
	int i, k = 0;

	/* Check the player can study at all atm */
	if (!player_can_study())
		return;

	/* Check that the player has access to the nominated spell book. */
	if (!item_is_available(book, obj_can_browse, (USE_INVEN | USE_FLOOR))) {
		msg("That item is not within your reach.");
		return;
	}

	/* Extract spells */
	total_spells = spell_book_count_spells(o_ptr, spell_okay_to_browse);
	for (i = 0; i < total_spells; i++) {
		int s = get_spell_index(o_ptr, i);

		/* Skip non-OK spells */
		if (s == -1)
			continue;
		if (!spell_okay_to_study(s))
			continue;
		if (!spell_in_book(s, book))
			continue;

		/* Apply the randomizer */
		if ((++k > 1) && (randint0(k) != 0))
			continue;

		/* Track it */
		spell = s;
	}

	/* Nothing to study */
	if (spell < 0) {
		/* Message */
		msg("You cannot learn any %s%s that %s.",
			magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
			(mp_ptr->spell_realm == REALM_NATURE) ? " from" : "s in",
			magic_desc[mp_ptr->spell_realm][BOOK_NOUN]);

		/* Abort */
		return;
	}

	spell_learn(spell);


	/* Take a turn */
	p_ptr->energy_use = 100;

	/* Forget the item_tester_hook */
	item_tester_hook = NULL;

}
