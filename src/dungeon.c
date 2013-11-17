/** \file dungeon.c 
    \brief Turn-by-turn processing

    * Char & monster regeneration, town and dungeon management,
    * all timed character, monster, and object states, entry into Wizard, 
    * debug, and borg mode, definitions of user commands, process player, 
    * the basic function for interacting with the dungeon (including what 
    * happens when a wizard cheats death).
    *
    * Copyright (c) 1997-2009 Nick McConnell, Ben Harrison, James E. Wilson, 
    * Robert A. Koeneke
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
#include "button.h"
#include "cave.h"
#include "cmds.h"
#include "files.h"
#include "game-event.h"
#include "generate.h"
#include "init.h"
#include "monster.h"
#include "tvalsval.h"
#include "prefs.h"
#include "savefile.h"
#include "spells.h"
#include "target.h"



/**
 * This routine separates all the normal and special artifacts into
 * separate lists for easy allocation later.
 */
void init_artifacts(void)
{
    int loop;
  
    /* First: count. Second: build lists */
    for (loop = 0; loop <= 1; loop++)
    {
	int a_idx;
      
	artifact_normal_cnt = 0;
	artifact_special_cnt = 0;
      
	/* Check every artifact (including randoms) */
	for (a_idx = 1; a_idx < z_info->a_max; a_idx++)
        {
	    /* Access this artifact */
	    artifact_type *a_ptr = &a_info[a_idx];
          
	    /* Require "real" artifacts */
	    if (!a_ptr->name) continue;
          
	    /* This is a "special" artifact */
	    if ((a_idx < ART_MIN_NORMAL) ||
		(a_ptr->tval == TV_AMULET) ||
		(a_ptr->tval == TV_RING) ||
		(a_ptr->tval == TV_ROD) ||
		(a_ptr->tval == TV_STAFF) ||
		(a_ptr->tval == TV_WAND))
            {
		if (loop == 1)
                {
		    artifact_special[artifact_special_cnt] = a_idx;
                }
              
		/* Count the special artifacts */
		artifact_special_cnt++;
            }
          
	    /*
	     * This is a "normal" artifact. Notice we must skip
	     * Morgoth's Crown and Hammer.
	     */
	    else if (!kf_has(a_ptr->flags_kind, KF_INSTA_ART))
            {
		if (loop == 1)
                {
		    artifact_normal[artifact_normal_cnt] = a_idx;
                }
              
		/* Count the normal artifacts */
		artifact_normal_cnt++;
            }
        }
      
	/* Allocate the lists the first time through */
	if (loop == 0)
        {
	    artifact_normal = C_ZNEW(artifact_normal_cnt, int);
	    artifact_special = C_ZNEW(artifact_special_cnt, int);
        }
    }
}


/**
 * Regenerate hit points
 */
static void regenhp(int percent)
{
    s32b new_chp, new_chp_frac;
    int old_chp;

    /* Save the old hitpoints */
    old_chp = p_ptr->chp;

    /* Extract the new hitpoints */
    new_chp = ((long) p_ptr->mhp) * percent + PY_REGEN_HPBASE;
    p_ptr->chp += (s16b) (new_chp >> 16);	/* div 65536 */

    /* check for overflow */
    if ((p_ptr->chp < 0) && (old_chp > 0))
	p_ptr->chp = MAX_SHORT;
    new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
    if (new_chp_frac >= 0x10000L) {
	p_ptr->chp_frac = (u16b) (new_chp_frac - 0x10000L);
	p_ptr->chp++;
    } else {
	p_ptr->chp_frac = (u16b) (new_chp_frac);
    }

    /* Fully healed */
    if (p_ptr->chp >= p_ptr->mhp) {
	p_ptr->chp = p_ptr->mhp;
	p_ptr->chp_frac = 0;
    }

    /* Notice changes */
    if (old_chp != p_ptr->chp) {
	/* Redraw */
	p_ptr->redraw |= (PR_HP);
    }
}


/**
 * Regenerate mana points
 */
static void regenmana(int percent)
{
    s32b new_mana, new_mana_frac;
    int old_csp;

    old_csp = p_ptr->csp;
    new_mana = ((long) p_ptr->msp) * percent + PY_REGEN_MNBASE;
    p_ptr->csp += (s16b) (new_mana >> 16);	/* div 65536 */

    /* check for overflow */
    if ((p_ptr->csp < 0) && (old_csp > 0)) {
	p_ptr->csp = MAX_SHORT;
    }
    new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
    if (new_mana_frac >= 0x10000L) {
	p_ptr->csp_frac = (u16b) (new_mana_frac - 0x10000L);
	p_ptr->csp++;
    } else {
	p_ptr->csp_frac = (u16b) new_mana_frac;
    }

    /* Must set frac to zero even if equal */
    if (p_ptr->csp >= p_ptr->msp) {
	p_ptr->csp = p_ptr->msp;
	p_ptr->csp_frac = 0;
    }

    /* Redraw mana */
    if (old_csp != p_ptr->csp) {
	/* Redraw */
	p_ptr->redraw |= (PR_MANA);
    }
}

/**
 * If player has inscribed the object with "!!", let him know when it's 
 * recharged. -LM-
 */
static void recharged_notice(const object_type * o_ptr, bool all)
{
    char o_name[120];

    const char *s;

    bool notify = FALSE;

    if (OPT(notify_recharge))
    {
	notify = TRUE;
    }
    else if (o_ptr->note)
    {
	/* Find a '!' */
	s = strchr(quark_str(o_ptr->note), '!');

	/* Process notification request */
	while (s)
	{
	    /* Find another '!' */
	    if (s[1] == '!')
	    {
		notify = TRUE;
		break;
	    }

	    /* Keep looking for '!'s */
	    s = strchr(s + 1, '!');
	}
    }

    if (!notify) return;

    /* Describe (briefly) */
    object_desc(o_name, sizeof(o_name), o_ptr, ODESC_BASE);

    /* Disturb the player */
    disturb(0, 0);

    /* Notify the player */
    if (o_ptr->number > 1)
    {
	if (all) msg("Your %s have recharged.", o_name);
	else msg("One of your %s has recharged.", o_name);
    }

    /* Artifacts */
    else if (o_ptr->name1)
    {
	msg("The %s has recharged.", o_name);
    }

    /* Single, non-artifact items */
    else msg("Your %s has recharged.", o_name);
}

/*
 * Recharge activatable objects in the player's equipment
 * and rods in the inventory and on the ground.
 */
static void recharge_objects(void)
{
    int i;

    bool charged = FALSE, discharged_stack;

    object_type *o_ptr;

    /*** Recharge equipment ***/
    for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
    {
	/* Get the object */
	o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx) continue;

	/* Recharge activatable objects */
	if (recharge_timeout(o_ptr))
	{
	    charged = TRUE;

	    /* Message if an item recharged */
	    recharged_notice(o_ptr, TRUE);
	}
    }

    /* Notice changes */
    if (charged)
    {
	/* Window stuff */
	p_ptr->redraw |= (PR_EQUIP);
    }

    charged = FALSE;

    /*** Recharge the inventory ***/
    for (i = 0; i < INVEN_PACK; i++)
    {
	o_ptr = &p_ptr->inventory[i];

	/* Skip non-objects */
	if (!o_ptr->k_idx) continue;

	discharged_stack = (number_charging(o_ptr) == o_ptr->number) 
	    ? TRUE : FALSE;

	/* Recharge rods, and update if any rods are recharged */
	if (o_ptr->tval == TV_ROD && recharge_timeout(o_ptr))
	{
	    charged = TRUE;

	    /* Entire stack is recharged */
	    if (o_ptr->timeout == 0)
	    {
		recharged_notice(o_ptr, TRUE);
	    }

	    /* Previously exhausted stack has acquired a charge */
	    else if (discharged_stack)
	    {
		recharged_notice(o_ptr, FALSE);
	    }
	}
    }

    /* Notice changes */
    if (charged)
    {
	/* Combine pack */
	p_ptr->notice |= (PN_COMBINE);

	/* Redraw stuff */
	p_ptr->redraw |= (PR_INVEN);
    }

    /*** Recharge the ground ***/
    for (i = 1; i < o_max; i++)
    {
	/* Get the object */
	o_ptr = &o_list[i];

	/* Skip dead objects */
	if (!o_ptr->k_idx) continue;

	/* Recharge rods on the ground */
	if (o_ptr->tval == TV_ROD)
	    recharge_timeout(o_ptr);
    }
}

/**
 * Remove light-sensitive monsters from sunlt areas
 */
void sun_banish(void)
{
    bool some_gone = FALSE;

    monster_type *m_ptr;
    monster_race *r_ptr;

    int i;

    /* Process all monsters */
    for (i = 1; i < m_max - 1; i++) {
	/* Access the monster */
	m_ptr = &m_list[i];


	/* Check if it hates light */
	r_ptr = &r_info[m_ptr->r_idx];

	/* Paranoia -- Skip dead monsters */
	if (!m_ptr->r_idx)
	    continue;

	/* Hack -- Skip Unique Monsters */
	if (rf_has(r_ptr->flags, RF_UNIQUE))
	    continue;

	/* Skip monsters not hurt by light */
	if (!(rf_has(r_ptr->flags, RF_HURT_LIGHT)))
	    continue;

	/* Banish */
	delete_monster_idx(i);
	some_gone = TRUE;
    }

    process_monsters(0);
    if (some_gone)
	msg("Creatures of the darkness flee from the sunlight!");
}


static void play_ambient_sound(void)
{
    /* Town sound */
    if (p_ptr->depth == 0) {
	/* Hack - is it daytime or nighttime? */
	if (turn % (10L * TOWN_DAWN) < TOWN_DAWN / 2) {
	    /* It's day. */
	    sound(MSG_AMBIENT_DAY);
	} else {
	    /* It's night. */
	    sound(MSG_AMBIENT_NITE);
	}

    }

    /* Dungeon level 1-20 */
    else if (p_ptr->depth <= 20) {
	sound(MSG_AMBIENT_DNG1);
    }

    /* Dungeon level 21-40 */
    else if (p_ptr->depth <= 40) {
	sound(MSG_AMBIENT_DNG2);
    }

    /* Dungeon level 41-60 */
    else if (p_ptr->depth <= 60) {
	sound(MSG_AMBIENT_DNG3);
    }

    /* Dungeon level 61-80 */
    else if (p_ptr->depth <= 80) {
	sound(MSG_AMBIENT_DNG4);
    }

    /* Dungeon level 80- */
    else {
	sound(MSG_AMBIENT_DNG5);
    }
}

/**
 * Handle certain things once every 10 game turns.
 */
static void process_world(void)
{
    int i, j;

    int regen_amount, mana_regen_amount, chance;
    int plev = p_ptr->lev;
    object_type *o_ptr;

    bool was_ghost = FALSE;

    bool extend_magic = FALSE;

    bool divine = (player_has(PF_DIVINE));
    bool hardy = (player_has(PF_HARDY));

    bool dawn;
    
    /*** Check the Time ***/

    /* Every 10 game turns */
    if (turn % 10)
	return;

    /* Play an ambient sound at regular intervals. */
    if (!(turn % ((10L * TOWN_DAWN) / 4))) {
	play_ambient_sound();
    }

    /* Hack - beneficial effects timeout at 2/3 speed with ENHANCE_MAGIC */
    if ((player_has(PF_ENHANCE_MAGIC))
	&& ((turn / 10) % EXTEND_MAGIC_FRACTION))
	extend_magic = TRUE;

    /*** Attempt timed autosave.  From Zangband. ***/
    if (autosave && autosave_freq) {
	if (!(turn % ((s32b) autosave_freq * 10))) {
	    is_autosave = TRUE;
	    save_game();
	    is_autosave = FALSE;
	    message_flush();
	}
    }

    /*** Handle the "outside" (stores and sunshine) ***/

    /* While not in cave, including cave towns */
    if (outside) {
	/* Hack -- Daybreak/Nightfall outside */
	if (!(turn % ((10L * TOWN_DAWN) / 2))) {
	    /* Check for dawn */
	    dawn = (!(turn % (10L * TOWN_DAWN)));

	    /* Illuminate */
	    illuminate();

	    /* Day breaks */
	    if (dawn) {
		/* Message */
		msg("The sun has risen.");
		sun_banish();
		if (p_ptr->schange == SHAPE_VAMPIRE)
		    shapechange(SHAPE_NORMAL);
		update_view();
	    }

	    /* Night falls */
	    else {
		/* Message */
		msg("The sun has set.");
		update_view();
	    }
	}
    }


    /* Update the stores once a day (while out of town) */
    if (p_ptr->depth)
	if (!(turn % (10L * STORE_TURNS)))
	    stores_maint(1);


    /*** Process the monsters ***/

    /* Hack - see if there is already a player ghost on the level */
    if (bones_selector)
	was_ghost = TRUE;

    /* Check for creature generation, except on themed levels */
    if ((randint0(MAX_M_ALLOC_CHANCE) == 0) && (!p_ptr->themed_level)) {
	int n;

	/* Hack for small towns */
	if (!p_ptr->depth)
	    n = MAX_SIGHT - 5;
	else
	    n = MAX_SIGHT + 5;

	/* Make a new monster */
	(void) alloc_monster(n, FALSE, FALSE);
    }

    /* Hack - if there is a ghost now, and there was not before, give a
     * challenge */
    if ((bones_selector) && (!(was_ghost)))
	ghost_challenge();

    /*** Damage over Time ***/

    /* Take damage from poison */
    if (p_ptr->timed[TMD_POISONED]) {
	/* Take damage */
	take_hit(randint1
		 (p_ptr->timed[TMD_POISONED] >
		  300 ? 20 : (p_ptr->timed[TMD_POISONED] + 14) / 15), "poison");
    }

    /* Take damage from cuts */
    if (p_ptr->timed[TMD_CUT]) {
	/* Mortal wound or Deep Gash */
	if (p_ptr->timed[TMD_CUT] > 200) {
	    i = 3;
	}

	/* Severe cut */
	else if (p_ptr->timed[TMD_CUT] > 100) {
	    i = 2;
	}

	/* Other cuts */
	else {
	    i = 1;
	}

	/* Take damage */
	take_hit(i, "a fatal wound");
    }


    /*** Check the Food, and Regenerate ***/

    /* Digest normally */
    if (p_ptr->food < PY_FOOD_MAX) {
	/* Every 100 game turns */
	if (!(turn % 100)) {
	    /* Basic digestion rate based on speed */
	    i = extract_energy[p_ptr->state.pspeed] * 2;

	    /* Regeneration takes more food */
	    if (p_ptr->state.regenerate)
		i += 30;

	    /* Fast digestion takes more food */
	    if (p_ptr->state.fast_digest) {
		notice_curse(CF_HUNGRY, 0);
		i += 20;
	    }

	    /* Slow digestion takes less food */
	    if (p_ptr->state.slow_digest) {
		notice_obj(OF_SLOW_DIGEST, 0);
		i -= 10;
	    }

	    /* Minimal digestion */
	    if (i < 1)
		i = 1;

	    /* Digest some food */
	    (void) set_food(p_ptr->food - i);
	}
    }

    /* Digest quickly when gorged */
    else {
	/* Digest a lot of food */
	(void) set_food(p_ptr->food - 100);
    }

    /* Starve to death (slowly) */
    if (p_ptr->food < PY_FOOD_STARVE) {
	/* Calculate damage */
	i = (PY_FOOD_STARVE - p_ptr->food) / 10;

	/* Take damage */
	take_hit(i, "starvation");
    }

    /* Default regeneration */
    regen_amount = PY_REGEN_NORMAL;

    /* Getting Weak */
    if (p_ptr->food < PY_FOOD_WEAK) {
	/* Lower regeneration */
	if (p_ptr->food < PY_FOOD_STARVE) {
	    regen_amount = 0;
	} else if (p_ptr->food < PY_FOOD_FAINT) {
	    regen_amount = PY_REGEN_FAINT;
	} else {
	    regen_amount = PY_REGEN_WEAK;
	}

	/* Getting Faint */
	if (p_ptr->food < PY_FOOD_FAINT) {
	    /* Faint occasionally */
	    if (!p_ptr->timed[TMD_PARALYZED] && (randint0(100) < 10)) {
		/* Message */
		msg("You faint from the lack of food.");
		disturb(1, 0);

		/* Hack -- faint (bypass free action) */
		(void) inc_timed(TMD_PARALYZED, 1 + randint0(5), TRUE);
	    }
	}
    }


    /* Searching or Resting */
    if (p_ptr->searching || p_ptr->resting) {
	regen_amount = regen_amount * 2;
    }

    /* Regeneration ability.  A greater effect on mana in FAangband. */
    if (p_ptr->state.regenerate) {
	notice_obj(OF_REGEN, 0);
	regen_amount = regen_amount * 2;
	mana_regen_amount = 3 * regen_amount / 2;
    }

    /* Otherwise, the basic mana regen is the same as that of HPs. */
    else
	mana_regen_amount = regen_amount;

    /* Consider specialty abilities */
    if (player_has(PF_REGENERATION))
	regen_amount *= 2;
    if (player_has(PF_MEDITATION))
	mana_regen_amount *= 2;

    /* Regenerate the mana */
    if (p_ptr->csp < p_ptr->msp) {
	regenmana(mana_regen_amount);
    }

    /* Various things interfere with healing */
    if (p_ptr->timed[TMD_PARALYZED])
	regen_amount = 0;
    if (p_ptr->timed[TMD_POISONED])
	regen_amount = 0;
    if (p_ptr->timed[TMD_STUN])
	regen_amount = 0;
    if (p_ptr->timed[TMD_CUT])
	regen_amount = 0;
    if (p_ptr->state.slow_regen) {
	regen_amount /= 2;
	notice_curse(CF_SLOW_REGEN, 0);
    }

    /* Regenerate Hit Points if needed */
    if (p_ptr->chp < p_ptr->mhp) {
	regenhp(regen_amount);
    }


    /*** Timeout Various Things ***/

    /* Hack -- Hallucinating */
    if (p_ptr->timed[TMD_IMAGE]) {
	/* Maiar recover quickly from anything. */
	if (divine)
	    (void) dec_timed(TMD_IMAGE, 2, FALSE);
	else
	    (void) dec_timed(TMD_IMAGE, 1, FALSE);
    }

    /* Blindness */
    if (p_ptr->timed[TMD_BLIND]) {
	/* Maiar recover quickly from anything. */
	if (divine)
	    (void) dec_timed(TMD_BLIND, 2, FALSE);

	else
	    (void) dec_timed(TMD_BLIND, 1, FALSE);
    }

    /* Timed see-invisible */
    if ((p_ptr->timed[TMD_SINVIS]) && (!extend_magic)) {
	(void) dec_timed(TMD_SINVIS, 1, FALSE);
    }

    /* Timed Telepathy */
    if ((p_ptr->timed[TMD_TELEPATHY]) && (!extend_magic)) {
	(void) dec_timed(TMD_TELEPATHY, 1, FALSE);
    }

    /* Timed near-complete stealth -LM- */
    if ((p_ptr->timed[TMD_SSTEALTH]) && (!extend_magic)) {
	if (!(player_has(PF_WOODEN)
	      &&
	      (tf_has(f_info[cave_feat[p_ptr->py][p_ptr->px]].flags, TF_TREE))))
	{
	    (void) dec_timed(TMD_SSTEALTH, 1, FALSE);

	    /* Warn the player that he's going to be revealed soon. */
	    if (p_ptr->timed[TMD_SSTEALTH] == 5)
		msg("You sense your mantle of shadow fading...");
	}
    }

    /* Timed temporary elemental brands. -LM- */
    if ((p_ptr->timed[TMD_ATT_ACID]) && (!extend_magic)) {
	(void) dec_timed(TMD_ATT_ACID, 1, FALSE);
    }
    if ((p_ptr->timed[TMD_ATT_ELEC]) && (!extend_magic)) {
	(void) dec_timed(TMD_ATT_ELEC, 1, FALSE);
    }
    if ((p_ptr->timed[TMD_ATT_FIRE]) && (!extend_magic)) {
	(void) dec_timed(TMD_ATT_FIRE, 1, FALSE);
    }
    if ((p_ptr->timed[TMD_ATT_COLD]) && (!extend_magic)) {
	(void) dec_timed(TMD_ATT_COLD, 1, FALSE);
    }
    if ((p_ptr->timed[TMD_ATT_COLD]) && (!extend_magic)) {
	(void) dec_timed(TMD_ATT_COLD, 1, FALSE);
    }

    /* Timed infra-vision */
    if ((p_ptr->timed[TMD_SINFRA]) && (!extend_magic)) {
	(void) dec_timed(TMD_SINFRA, 1, FALSE);
    }

    /* Paralysis */
    if (p_ptr->timed[TMD_PARALYZED]) {
	/* Maiar recover quickly from anything. */
	if (divine)
	    (void) dec_timed(TMD_PARALYZED, 2, FALSE);

	else
	    (void) dec_timed(TMD_PARALYZED, 1, FALSE);
    }

    /* Confusion */
    if (p_ptr->timed[TMD_CONFUSED]) {
	/* Maiar recover quickly from anything. */
	if (divine)
	    (void) dec_timed(TMD_CONFUSED, 2, FALSE);

	else
	    (void) dec_timed(TMD_CONFUSED, 1, FALSE);
    }

    /* Afraid */
    if (p_ptr->timed[TMD_AFRAID] && !(p_ptr->state.fear)) {
	/* Maiar recover quickly from anything. */
	if (divine)
	    (void) dec_timed(TMD_AFRAID, 2, FALSE);

	else
	    (void) dec_timed(TMD_AFRAID, 1, FALSE);
    }

    /* Permanent fear */
    else if (p_ptr->state.fear) {
	p_ptr->timed[TMD_AFRAID] = 1;
	notice_curse(CF_AFRAID, 0);
    }

    /* Fast */
    if ((p_ptr->timed[TMD_FAST]) && (!extend_magic)) {
	(void) dec_timed(TMD_FAST, 1, FALSE);
    }

    /* Slow */
    if (p_ptr->timed[TMD_SLOW]) {
	/* Maiar recover quickly from anything. */
	if (divine)
	    (void) dec_timed(TMD_SLOW, 2, FALSE);

	else
	    (void) dec_timed(TMD_SLOW, 1, FALSE);
    }

    /* Protection from evil. */
    if ((p_ptr->timed[TMD_PROTEVIL]) && (!extend_magic)) {
	(void) dec_timed(TMD_PROTEVIL, 1, FALSE);
    }

    /* Increased Magical Defences. -LM- */
    if (p_ptr->timed[TMD_INVULN] && (!extend_magic)) {
	(void) dec_timed(TMD_INVULN, 1, FALSE);
    }

    /* Heroism. */
    if ((p_ptr->timed[TMD_HERO]) && (!extend_magic)) {
	(void) dec_timed(TMD_HERO, 1, FALSE);
    }

    /* Berserk. */
    if ((p_ptr->timed[TMD_SHERO]) && (!extend_magic)) {
	(void) dec_timed(TMD_SHERO, 1, FALSE);
    }

    /* Blessed */
    if ((p_ptr->timed[TMD_BLESSED]) && (!extend_magic)) {
	(void) dec_timed(TMD_BLESSED, 1, FALSE);
    }

    /* Shield */
    if ((p_ptr->timed[TMD_SHIELD]) && (!extend_magic)) {
	(void) dec_timed(TMD_SHIELD, 1, FALSE);
    }

    /* Oppose Cold. */
    if ((p_ptr->timed[TMD_OPP_COLD]) && (!extend_magic)) {
	(void) dec_timed(TMD_OPP_COLD, 1, FALSE);
    }

    /* Oppose Acid. */
    if ((p_ptr->timed[TMD_OPP_ACID]) && (!extend_magic)) {
	(void) dec_timed(TMD_OPP_ACID, 1, FALSE);
    }

    /* Oppose Lightning */
    if ((p_ptr->timed[TMD_OPP_ELEC]) && (!extend_magic)) {
	(void) dec_timed(TMD_OPP_ELEC, 1, FALSE);
    }

    /* Oppose Fire */
    if ((p_ptr->timed[TMD_OPP_FIRE]) && (!extend_magic)) {
	(void) dec_timed(TMD_OPP_FIRE, 1, FALSE);
    }

    /* Oppose Poison */
    if ((p_ptr->timed[TMD_OPP_POIS]) && (!extend_magic)) {
	(void) dec_timed(TMD_OPP_POIS, 1, FALSE);
    }


    /*** Poison and Stun and Cut ***/

    /* Poison */
    if (p_ptr->timed[TMD_POISONED]) {
	int adjust = (adj_con_fix[p_ptr->state.stat_ind[A_CON]] + 1);

	/* Hobbits are sturdy. */
	if (hardy)
	    adjust++;

	/* Maiar recover quickly from anything. */
	if (divine)
	    adjust = 3 * adjust / 2;

	/* Apply some healing */
	(void) dec_timed(TMD_POISONED, adjust, FALSE);
    }

    /* Stun */
    if (p_ptr->timed[TMD_STUN]) {
	int adjust = (adj_con_fix[p_ptr->state.stat_ind[A_CON]] + 1);

	/* Maiar recover quickly from anything. */
	if (divine)
	    adjust = 3 * adjust / 2;

	/* Apply some healing */
	(void) dec_timed(TMD_STUN, adjust, FALSE);
    }

    /* Cut */
    if (p_ptr->timed[TMD_CUT]) {
	int adjust = (adj_con_fix[p_ptr->state.stat_ind[A_CON]] + 1);

	/* Hobbits are sturdy. */
	if (hardy)
	    adjust++;

	/* Maiar recover quickly from anything. */
	if (divine)
	    adjust = 3 * adjust / 2;

	/* Hack -- Truly "mortal" wound */
	if (p_ptr->timed[TMD_CUT] > 1000)
	    adjust = 0;

	/* Apply some healing */
	(void) dec_timed(TMD_CUT, adjust, FALSE);
    }

    /* Every 500 turns, warn about any Black Breath not gotten from an equipped 
     * object, and stop any resting. -LM-
     */
    if (!(turn % 5000) && (p_ptr->black_breath)) {
	bool be_silent = FALSE;

	/* check all equipment for the Black Breath flag. */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {
	    o_ptr = &p_ptr->inventory[i];

	    /* Skip non-objects */
	    if (!o_ptr->k_idx)
		continue;

	    /* No messages if object has the flag, to avoid annoyance. */
	    if (cf_has(o_ptr->flags_curse, CF_DRAIN_EXP))
		be_silent = TRUE;

	}
	/* If we are allowed to speak, warn and disturb. */

	if (be_silent == FALSE) {
	    msg("The Black Breath saps your soul!");
	    disturb(0, 0);
	}
    }

    /* Decay special heighten power */
    if (p_ptr->heighten_power) {
	int decrement;

	/* 
	 * Mega-Hack - To keep it from being a free ride for high speed 
	 * characters, Heighten Power decays quickly when highly charged
	 */
	decrement = 10 + (p_ptr->heighten_power / 55);

	if (p_ptr->heighten_power > decrement)
	    p_ptr->heighten_power -= decrement;
	else
	    p_ptr->heighten_power = 0;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
    }

    /* Decay special speed boost */
    if (p_ptr->speed_boost) {
	if (p_ptr->speed_boost > 10)
	    p_ptr->speed_boost -= 10;
	else
	    p_ptr->speed_boost = 0;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
    }

    /*** Process Light ***/

    /* Check for light being wielded */
    o_ptr = &p_ptr->inventory[INVEN_LIGHT];

    /* Burn some fuel in the current light */
    if (o_ptr->tval == TV_LIGHT) {
	/* Hack -- Use some fuel (except on artifacts) */
	if (!artifact_p(o_ptr) && (o_ptr->pval > 0) && (!is_daylight)) {
	    /* Decrease life-span */
	    o_ptr->pval--;

	    /* Hack -- notice interesting fuel steps */
	    if ((o_ptr->pval < 100) || (!(o_ptr->pval % 100))) {
		p_ptr->redraw |= (PR_EQUIP);
	    }

	    /* Hack -- Special treatment when blind */
	    if (p_ptr->timed[TMD_BLIND]) {
		/* Hack -- save some light for later */
		if (o_ptr->pval == 0)
		    o_ptr->pval++;
	    }

	    /* The light is now out */
	    else if (o_ptr->pval == 0) {
		disturb(0, 0);
		msg("Your light has gone out!");
	    }

	    /* The light is getting dim */
	    else if ((o_ptr->pval < 100) && (!(o_ptr->pval % 10))) {
		disturb(0, 0);
		msg("Your light is growing faint.");
	    }
	}
    }

    /* Calculate torch radius */
    p_ptr->update |= (PU_TORCH);


    /*** Process Inventory ***/

    /* Handle experience draining.  In Oangband, the effect is worse,
     * especially for high-level characters.  As per Tolkien, hobbits are
     * resistant. */
    if (p_ptr->black_breath) {
	if (hardy)
	    chance = 2;
	else
	    chance = 5;

	if ((randint0(100) < chance) && (p_ptr->exp > 0)) {
	    p_ptr->exp -= 1 + plev / 5;
	    p_ptr->max_exp -= 1 + plev / 5;
	    check_experience();
	}
    }

    /* Recharge activatable objects and rods */
    recharge_objects();



    /*** Random curse effects ***/

    /* Random teleportation */
    if ((p_ptr->state.teleport) && (randint0(100) < 1)) {
	/* Teleport player */
	teleport_player(40, FALSE);

	/* Notice */
	notice_curse(CF_TELEPORT, 0);
    }

    /* Random aggravation (with hasting of awake monsters */
    if ((p_ptr->state.rand_aggro) && (randint0(500) < 1)) {
	/* Aggravate, and notice if seen */
	if (aggravate_monsters(1, FALSE))
	    notice_curse(CF_AGGRO_RAND, 0);
    }

    /* Random poisoning */
    if ((p_ptr->state.rand_pois) && (randint0(300) < 1)) {
	/* Damage player */
	pois_hit(5 + randint1(5));

	/* Notice */
	notice_curse(CF_POIS_RAND, 0);
    }

    /* Random poison cloud */
    if ((p_ptr->state.rand_pois_bad) && (randint0(300) < 1)) {
	/* Weak poison cloud, enough to wake nearby monsters */
	fire_sphere(GF_POIS, 0, 5, 3, 40);

	/* Hurt the player */
	pois_hit(10 + randint1(10));

	/* Notice */
	notice_curse(CF_POIS_RAND_BAD, 0);
    }

    /* Random cuts */
    if ((p_ptr->state.rand_cuts) && (randint0(200) < 1)) {
	/* Wound the player, usually notice */
	if (inc_timed(TMD_CUT, 5 + randint0(10), TRUE))
	    notice_curse(CF_CUT_RAND, 0);
    }

    /* Random bad cuts */
    if ((p_ptr->state.rand_cuts_bad) && (randint0(200) < 1)) {
	/* Wound the player, usually notice */
	if (inc_timed(TMD_CUT, 20 + randint0(30), TRUE))
	    notice_curse(CF_CUT_RAND_BAD, 0);
    }

    /* Random hallucination */
    if ((p_ptr->state.rand_hallu) && (randint0(300) < 1)) {
	/* Start hallucinating */
	(void) inc_timed(TMD_IMAGE, 10 + randint1(10), TRUE);

	/* Notice */
	notice_curse(CF_HALLU_RAND, 0);
    }

    /* Droppable weapons */
    if (p_ptr->state.drop_weapon) {
	/* Check the whole inventory */
	for (i = 0; i < INVEN_TOTAL; i++) {
	    o_ptr = &p_ptr->inventory[i];

	    /* Chance for each item */
	    if ((cf_has(o_ptr->flags_curse, CF_DROP_WEAPON)) && 
		(randint0(200) < 1)) 
	    {
		/* Notice */
		notice_curse(CF_DROP_WEAPON, i + 1);

		/* Dropped it */
		inven_drop(i, randint1(o_ptr->number));
	    }
	}
    }

    /* Summon demons */
    if ((p_ptr->state.attract_demon) && (randint0(500) < 1)) {
	/* Message */
	msg("You have attracted a demon.");

	/* Here it comes */
	summon_specific(p_ptr->py, p_ptr->px, FALSE, p_ptr->depth,
			SUMMON_DEMON);

	/* Notice */
	notice_curse(CF_ATTRACT_DEMON, 0);
    }

    /* Summon undead */
    if ((p_ptr->state.attract_undead) && (randint0(500) < 1)) {
	/* Message */
	msg("A call goes out beyond the grave.");

	/* Here it comes */
	summon_specific(p_ptr->py, p_ptr->px, FALSE, p_ptr->depth,
			SUMMON_UNDEAD);

	/* Notice */
	notice_curse(CF_ATTRACT_UNDEAD, 0);
    }

    /* Random paralysis - nasty! */
    if (((p_ptr->state.rand_paral) || (p_ptr->state.rand_paral_all))
	&& (randint0(600) < 1)) {
	/* Paralyzed for 1 turn */
	(void) inc_timed(TMD_PARALYZED, 1, TRUE);

	/* Notice */
	if (p_ptr->state.rand_paral)
	    notice_curse(CF_PARALYZE, 0);
	else
	    notice_curse(CF_PARALYZE_ALL, 0);
    }

    /* Experience drain */
    if ((p_ptr->state.drain_exp) && (randint0(100) < 1) && (p_ptr->exp > 0)) {
	/* Drain */
	p_ptr->exp -= MIN(1 + plev / 5, p_ptr->exp);
	p_ptr->max_exp -= MIN(1 + plev / 5, p_ptr->exp);
	check_experience();

	/* Notice */
	notice_curse(CF_DRAIN_EXP, 0);
    }

    /* Mana drain */
    if ((p_ptr->state.drain_mana) && (randint0(100) < 1) && (p_ptr->csp > 0)) {
	/* Drain */
	p_ptr->csp -= MIN(1 + plev / 5, p_ptr->csp);

	/* Notice */
	notice_curse(CF_DRAIN_MANA, 0);

	/* Redraw mana */
	p_ptr->redraw |= (PR_MANA);
    }

    /* Drain a random stat */
    if ((p_ptr->state.drain_stat) && (randint0(1000) < 1)) {
	/* Drain */
	if (do_dec_stat(randint0(A_MAX)))
	    notice_curse(CF_DRAIN_STAT, 0);
    }

    /* Drain charges - code from monster attack */
    if ((p_ptr->state.drain_charge) && (randint0(200) < 1)) {
	/* Blindly hunt five times for an item. */
	for (j = 0; j < 5; j++) {
	    bool has_charges = FALSE;

	    /* Pick an item */
	    i = randint0(INVEN_PACK);

	    /* Obtain the item */
	    o_ptr = &p_ptr->inventory[i];

	    /* Skip non-objects */
	    if (!o_ptr->k_idx)
		continue;

	    /* Drain charged wands/staffs/rods */
	    if ((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND)
		|| (o_ptr->tval == TV_ROD)) {
		/* case of charged wands/staffs. */
		if (((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND))
		    && (o_ptr->pval))
		    has_charges = TRUE;

		/* case of (at least partially) charged rods. */
		if ((o_ptr->tval == TV_ROD) && 
		    (o_ptr->timeout < randcalc(o_ptr->time, 0, MINIMISE)))
		    has_charges = TRUE;

		if (has_charges) {
		    /* Message */
		    msg("Energy drains from your pack!");

		    /* Obvious */
		    notice_curse(CF_DRAIN_CHARGE, 0);

		    /* Uncharge */
		    if ((o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND))
			o_ptr->pval = 0;

		    /* New-style rods. */
		    if (o_ptr->tval == TV_ROD)
			o_ptr->timeout = randcalc(o_ptr->time, 0, RANDOMISE);


		    /* Combine / Reorder the pack */
		    p_ptr->notice |= (PN_COMBINE | PN_REORDER);

		    /* Not more than one inventory slot affected. */
		    break;
		}
	    }
	}
    }

    /* Delayed Word-of-Recall */
    if (p_ptr->word_recall) {
	/* Count down towards recall */
	p_ptr->word_recall--;

	/* Activate the recall */
	if (!p_ptr->word_recall) {
	    /* Disturbing! */
	    disturb(0, 0);

	    /* Sound */
	    sound(MSG_TPLEVEL);

	    /* Determine the level */
	    if (p_ptr->stage != p_ptr->home) {
		msg("You feel yourself yanked homewards!");

		/* Homeward bound */
		p_ptr->last_stage = NOWHERE;
		p_ptr->stage = p_ptr->home;

		/* Reset depth */
		p_ptr->depth = 0;

		/* Leaving */
		p_ptr->leaving = TRUE;
	    } else {
		msg("You feel yourself yanked away!");

		/* New stage */
		p_ptr->last_stage = NOWHERE;
		p_ptr->stage = p_ptr->recall_pt;

		/* Reset depth */
		p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

		/* Leaving */
		p_ptr->leaving = TRUE;
	    }

	    /* Sound */
	    sound(MSG_TPLEVEL);

	    p_ptr->redraw |= PR_STATUS;
	}
    }

    /* Delayed level feelings */
    if ((p_ptr->depth) && (!p_ptr->leaving) && (!do_feeling) && (!(turn % 100))
	&& (!p_ptr->themed_level)) {

	int chance = 40;

	/* After sufficient time, can learn about the level */
	if ((randint0(80) < p_ptr->state.skills[SKILL_SEARCH])
	    && (randint0(80) < chance)) {
	    /* Now have a feeling */
	    do_feeling = TRUE;

	    /* Announce feeling */
	    do_cmd_feeling();

	    /* Update the level indicator */
	    p_ptr->redraw |= (PR_DEPTH);

	    /* Disturb */
	    disturb(0, 0);
	}
    }
}


/**
 * Hack -- helper function for "process_player()"
 *
 * Check for changes in the "monster memory"
 */
static void process_player_aux(void)
{
    static int old_monster_race_idx = 0;

    static bitflag old_r_flags[RF_SIZE] = { 0, 0, 0 };
    static bitflag old_rs_flags[RSF_SIZE] = { 0, 0, 0, 0 };

    static byte old_r_blows0 = 0;
    static byte old_r_blows1 = 0;
    static byte old_r_blows2 = 0;
    static byte old_r_blows3 = 0;

    static byte old_r_cast_spell = 0;


    /* Tracking a monster */
    if (p_ptr->monster_race_idx) {
	/* Get the monster lore */
	monster_lore *l_ptr = &l_list[p_ptr->monster_race_idx];

	/* Check for change of any kind */
	if ((old_monster_race_idx != p_ptr->monster_race_idx)
	    || (!rf_is_equal(old_r_flags, l_ptr->flags))
	    || (!rsf_is_equal(old_rs_flags, l_ptr->spell_flags))
	    || (old_r_blows0 != l_ptr->blows[0])
	    || (old_r_blows1 != l_ptr->blows[1])
	    || (old_r_blows2 != l_ptr->blows[2])
	    || (old_r_blows3 != l_ptr->blows[3])
	    || (old_r_cast_spell != l_ptr->cast_spell)) {
	    /* Memorize old race */
	    old_monster_race_idx = p_ptr->monster_race_idx;

	    /* Memorize flags */
	    rf_copy(old_r_flags, l_ptr->flags);
	    rsf_copy(old_rs_flags, l_ptr->spell_flags);

	    /* Memorize blows */
	    old_r_blows0 = l_ptr->blows[0];
	    old_r_blows1 = l_ptr->blows[1];
	    old_r_blows2 = l_ptr->blows[2];
	    old_r_blows3 = l_ptr->blows[3];

	    /* Memorize castings */
	    old_r_cast_spell = l_ptr->cast_spell;

	    /* Redraw stuff */
	    p_ptr->redraw |= (PR_MONSTER);

	    /* Window stuff */
	    redraw_stuff(p_ptr);
	}
    }
}

/**
 * Helper function for mana gained through special means
 * (Power Siphon and Soul Siphon).
 */
static void special_mana_gain(void)
{
    if (p_ptr->mana_gain) {
	/* Message */
	if (p_ptr->csp < p_ptr->msp)
	    msg("You gain mana.");

	/* Partial fill */
	if (p_ptr->mana_gain < p_ptr->msp - p_ptr->csp) {
	    p_ptr->csp += p_ptr->mana_gain;
	    p_ptr->mana_gain = 0;
	}
	/* Complete Fill */
	else {
	    p_ptr->mana_gain -= p_ptr->msp - p_ptr->csp;
	    p_ptr->csp = p_ptr->msp;
	}

	/* 
	 * Hack - If there is a lot of mana left, it can do damage
	 * Mega-Hack - Restrict to Necromancers to make it affect Soul Siphon
	 * and not Power Siphon.
	 */
	if ((p_ptr->mana_gain > p_ptr->lev) && (player_has(PF_EVIL))) {
	    msg("You absorb too much mana!");
	    take_hit(damroll(2, 8), "mana burn");
	}

	/* Paranioa */
	if (p_ptr->csp > p_ptr->msp)
	    p_ptr->csp = p_ptr->msp;

	/* Clear mana gain */
	p_ptr->mana_gain = 0;

	/* Redraw */
	p_ptr->redraw |= (PR_MANA);
    }
}


/*
 * Place cursor on a monster or the player.
 */
static void place_cursor(void) {
	if (OPT(show_target) && target_sighted()) {
		s16b col, row;
		target_get(&col, &row);
		move_cursor_relative(row, col);
	}
}


/**
 * Process the player
 *
 * Notice the annoying code to handle "pack overflow", which
 * must come first just in case somebody manages to corrupt
 * the savefiles by clever use of menu commands or something.
 *
 * Notice the annoying code to handle "monster memory" changes,
 * which allows us to avoid having to update the window flags
 * every time we change any internal monster memory field, and
 * also reduces the number of times that the recall window must
 * be redrawn.
 *
 * Note that the code to check for user abort during repeated commands
 * and running and resting can be disabled entirely with an option, and
 * even if not disabled, it will never check during "special" resting
 * (codes -1 and -2), and it will only check during every 16th player
 * turn of "normal" resting.
 *
 * This function is no longer responsible for adding player energy.
 */
static void process_player(void)
{
    int i;

    u32b temp_wakeup_chance;

    byte feat = cave_feat[p_ptr->py][p_ptr->px];
    feature_type *f_ptr = &f_info[feat];

    /*** Check for interupts ***/

    /* Complete resting */
    if (p_ptr->resting < 0) {
	/* Basic resting */
	if (p_ptr->resting == -1) {
	    /* Stop resting */
	    if ((p_ptr->chp == p_ptr->mhp) && (p_ptr->csp == p_ptr->msp)) {
		disturb(0, 0);
	    }
	}

	/* Complete resting */
	else if (p_ptr->resting == -2) {
	    /* Stop resting */
	    if ((p_ptr->chp == p_ptr->mhp) && (p_ptr->csp == p_ptr->msp)
		&& !p_ptr->timed[TMD_BLIND] && !p_ptr->timed[TMD_CONFUSED]
		&& !p_ptr->timed[TMD_POISONED] && !p_ptr->timed[TMD_AFRAID]
		&& !p_ptr->timed[TMD_STUN] && !p_ptr->timed[TMD_CUT]
		&& !p_ptr->timed[TMD_SLOW] && !p_ptr->timed[TMD_PARALYZED]
		&& !p_ptr->timed[TMD_IMAGE] && !p_ptr->word_recall) {
		disturb(0, 0);
	    }
	}

	/* Resting until sunrise/set */
	else if (p_ptr->resting == -3) {
	    /* Stop resting */
	    if (!(((turn / 10) * 10) % (10L * TOWN_DAWN))) {
		if (!outside) msg("It is daybreak.");
		disturb(0, 0);
	    } else if (!(((turn / 10) * 10) % ((10L * TOWN_DAWN) / 2))) {
		if (!outside) msg("It is nightfall.");
		disturb(0, 0);
	    }
	}
    }

    /* Check for "player abort" */
    if (p_ptr->running || cmd_get_nrepeats() > 0
	|| (p_ptr->resting && !((turn * 10) % 0x0F))) {
		ui_event e;

		/* Do not wait */
		inkey_scan = SCAN_INSTANT;

		/* Check for a key */
		e = inkey_ex();
		if (e.type != EVT_NONE) {
			/* Flush and disturb */
			flush();
			disturb(0, 0);
			msg("Cancelled.");
		}
    }

    /* Add context-sensitive mouse buttons */

    if (tf_has(f_ptr->flags, TF_UPSTAIR))
	button_add("[<]", '<');
    else
	button_kill('<');

    if (tf_has(f_ptr->flags, TF_DOWNSTAIR))
	button_add("[>]", '>');
    else
	button_kill('>');

    if (cave_o_idx[p_ptr->py][p_ptr->px] > 0)
	button_add("[g]", 'g');
    else
	button_kill('g');


    /*** Hack - handle special mana gain ***/
    special_mana_gain();

    /*** Handle actual user input ***/

    /* Repeat until energy is reduced */
    do {
	/* Notice stuff (if needed) */
	if (p_ptr->notice)
	    notice_stuff(p_ptr);

	/* Update stuff (if needed) */
	if (p_ptr->update)
	    update_stuff(p_ptr);

	/* Redraw stuff (if needed) */
	if (p_ptr->redraw)
	    redraw_stuff(p_ptr);

	/* Place the cursor on the player */
	place_cursor();

	/* Refresh (optional) */
	Term_fresh();


	/* Hack -- Pack Overflow */
	pack_overflow();

	/* Assume free turn */
	p_ptr->energy_use = 0;


	/* Paralyzed or Knocked Out */
	if ((p_ptr->timed[TMD_PARALYZED]) || (p_ptr->timed[TMD_STUN] >= 100)) {
	    /* Take a turn */
	    p_ptr->energy_use = 100;
	}

	/* Picking up objects */
	else if (p_ptr->notice & PN_PICKUP)
	{
	    /* Recursively call the pickup function, use energy */
	    p_ptr->energy_use = py_pickup(0, p_ptr->py, p_ptr->px) * 10;
	    if (p_ptr->energy_use > 100)
		p_ptr->energy_use = 100;
	    p_ptr->notice &= ~(PN_PICKUP);
	}

	/* Resting */
	else if (p_ptr->resting) {
	    /* Timed rest */
	    if (p_ptr->resting > 0) {
		/* Reduce rest count */
		p_ptr->resting--;

		/* Redraw the state */
		p_ptr->redraw |= (PR_STATE);
	    }

	    /* Take a turn */
	    p_ptr->energy_use = 100;
	}

	/* Running */
	else if (p_ptr->running) {
	    /* Take a step */
	    run_step(0);
	}

	/* Repeated command */
	else if (cmd_get_nrepeats() > 0)
	{
	    /* Hack -- Assume messages were seen */
	    msg_flag = FALSE;

	    /* Clear the top line */
	    prt("", 0, 0);

	    /* Process the command */
	    process_command(CMD_GAME, TRUE);
	}

	/* Normal command */
	else {
	    /* Check monster recall */
	    process_player_aux();

	    /* Place the cursor on the player */
	    place_cursor();

	    /* Process the command */
	    process_command(CMD_GAME, FALSE);

	    /* Mega hack - complete redraw if big graphics */
	    if ((tile_width > 1) || (tile_height > 1))
		p_ptr->redraw |= (PR_MAP);

	}

	/*** Hack - handle special mana gain ***/
	special_mana_gain();

	/*** Clean up ***/

	/* Significant */
	if (p_ptr->energy_use) {
	    /* Use some energy */
	    p_ptr->energy -= p_ptr->energy_use;


	    /* Hack -- constant hallucination */
	    if (p_ptr->timed[TMD_IMAGE])
		p_ptr->redraw |= (PR_MAP);


	    /* Shimmer monsters if needed */
	    if (shimmer_monsters) {
		/* Clear the flag */
		shimmer_monsters = FALSE;

		/* Shimmer multi-hued monsters */
		for (i = 1; i < m_max; i++) {
		    monster_type *m_ptr;
		    monster_race *r_ptr;

		    /* Access monster */
		    m_ptr = &m_list[i];

		    /* Skip dead monsters */
		    if (!m_ptr->r_idx)
			continue;

		    /* Access the monster race */
		    r_ptr = &r_info[m_ptr->r_idx];

		    /* Skip non-multi-hued monsters */
		    if (!(rf_has(r_ptr->flags, RF_ATTR_MULTI)))
			continue;

		    /* Reset the flag */
		    shimmer_monsters = TRUE;

		    /* Redraw regardless */
		    light_spot(m_ptr->fy, m_ptr->fx);
		}
	    }

	    /* Repair "mark" flags */
	    if (repair_mflag_mark) {
		/* Reset the flag */
		repair_mflag_mark = FALSE;

		/* Process the monsters */
		for (i = 1; i < m_max; i++) {
		    monster_type *m_ptr;

		    /* Access monster */
		    m_ptr = &m_list[i];

		    /* Skip dead monsters */
		    /* if (!m_ptr->r_idx) continue; */

		    /* Repair "mark" flag */
		    if (m_ptr->mflag & (MFLAG_MARK)) {
			/* Skip "show" monsters */
			if (m_ptr->mflag & (MFLAG_SHOW)) {
			    /* Repair "mark" flag */
			    repair_mflag_mark = TRUE;

			    /* Skip */
			    continue;
			}

			/* Forget flag */
			m_ptr->mflag &= ~(MFLAG_MARK);

			/* Update the monster */
			update_mon(i, FALSE);
		    }
		}
	    }
	}

	/* Repair "show" flags */
	if (repair_mflag_show) {
	    /* Reset the flag */
	    repair_mflag_show = FALSE;

	    /* Process the monsters */
	    for (i = 1; i < m_max; i++) {
		monster_type *m_ptr;

		/* Access monster */
		m_ptr = &m_list[i];

		/* Skip dead monsters */
		/* if (!m_ptr->r_idx) continue; */

		/* Clear "show" flag */
		m_ptr->mflag &= ~(MFLAG_SHOW);
	    }
	}
	/* HACK: This will redraw the itemlist too frequently, but I'm don't
	   know all the individual places it should go. */
	p_ptr->redraw |= PR_ITEMLIST;
    }
    while (!p_ptr->energy_use && !p_ptr->leaving);

    /* 
     * Characters are noisy.
     * Use the amount of extra (directed) noise the player has made 
     * this turn to calculate the total amount of ambiant noise.
     */
    temp_wakeup_chance = p_ptr->state.base_wakeup_chance + add_wakeup_chance;

    /* People don't make much noise when resting. */
    if (p_ptr->resting)
	temp_wakeup_chance /= 2;

    /* Characters hidden in shadow have almost perfect stealth. */
    if ((p_ptr->timed[TMD_SSTEALTH]) && (!p_ptr->state.aggravate)) {
	if (temp_wakeup_chance > 200)
	    temp_wakeup_chance = 200;
    }


    /* Increase noise level if necessary. */
    if (temp_wakeup_chance > total_wakeup_chance)
	total_wakeup_chance = temp_wakeup_chance;


    /* Update noise flow information */
    update_noise();

    /* Update scent trail */
    update_smell();


    /* 
     * Reset character vulnerability.  Will be calculated by 
     * the first member of an animal pack that has a use for it.
     */
    p_ptr->vulnerability = 0;

}



byte flicker = 0;
byte color_flicker[MAX_COLORS][3] = {
    {TERM_DARK, TERM_L_DARK, TERM_L_RED},
    {TERM_WHITE, TERM_L_WHITE, TERM_L_BLUE},
    {TERM_SLATE, TERM_WHITE, TERM_L_DARK},
    {TERM_ORANGE, TERM_YELLOW, TERM_L_RED},
    {TERM_RED, TERM_L_RED, TERM_L_PINK},
    {TERM_GREEN, TERM_L_GREEN, TERM_L_TEAL},
    {TERM_BLUE, TERM_L_BLUE, TERM_SLATE},
    {TERM_UMBER, TERM_L_UMBER, TERM_MUSTARD},
    {TERM_L_DARK, TERM_SLATE, TERM_L_VIOLET},
    {TERM_WHITE, TERM_SLATE, TERM_L_WHITE},
    {TERM_L_PURPLE, TERM_PURPLE, TERM_L_VIOLET},
    {TERM_YELLOW, TERM_L_YELLOW, TERM_MUSTARD},
    {TERM_L_RED, TERM_RED, TERM_L_PINK},
    {TERM_L_GREEN, TERM_L_TEAL, TERM_GREEN},
    {TERM_L_BLUE, TERM_DEEP_L_BLUE, TERM_BLUE_SLATE},
    {TERM_L_UMBER, TERM_UMBER, TERM_MUD},
    {TERM_PURPLE, TERM_VIOLET, TERM_MAGENTA},
    {TERM_VIOLET, TERM_L_VIOLET, TERM_MAGENTA},
    {TERM_TEAL, TERM_L_TEAL, TERM_L_GREEN},
    {TERM_MUD, TERM_YELLOW, TERM_UMBER},
    {TERM_L_YELLOW, TERM_WHITE, TERM_L_UMBER},
    {TERM_MAGENTA, TERM_L_PINK, TERM_L_RED},
    {TERM_L_TEAL, TERM_L_WHITE, TERM_TEAL},
    {TERM_L_VIOLET, TERM_L_PURPLE, TERM_VIOLET},
    {TERM_L_PINK, TERM_L_RED, TERM_L_WHITE},
    {TERM_MUSTARD, TERM_YELLOW, TERM_UMBER},
    {TERM_BLUE_SLATE, TERM_BLUE, TERM_SLATE},
    {TERM_DEEP_L_BLUE, TERM_L_BLUE, TERM_BLUE},
};

byte get_flicker(byte a)
{
    switch (flicker % 3) {
    case 1:
	return color_flicker[a][1];
    case 2:
	return color_flicker[a][2];
    }
    return a;
}

/**
 * This animates monsters and/or items as necessary.
 */
void do_animation(void)
{
    int i;

    for (i = 1; i < m_max; i++) {
	byte attr;
	monster_type *m_ptr = &m_list[i];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!m_ptr || !m_ptr->ml)
	    continue;
	else if (rf_has(r_ptr->flags, RF_ATTR_MULTI))
	    attr = randint1(BASIC_COLORS - 1);
	else if (rf_has(r_ptr->flags, RF_ATTR_FLICKER))
	    attr = get_flicker(r_ptr->x_attr);
	else
	    continue;

	m_ptr->attr = attr;
	p_ptr->redraw |= (PR_MAP | PR_MONLIST);
    }
    flicker++;
}


/**
 * This is used when the user is idle to allow for simple animations.
 * Currently the only thing it really does is animate shimmering monsters.
 */
void idle_update(void)
{
    if (!character_dungeon)
	return;

    if (!OPT(animate_flicker) || (use_graphics != GRAPHICS_NONE))
	return;

    /* Animate and redraw if necessary */
    do_animation();
    redraw_stuff(p_ptr);

    /* Refresh the main screen */
    Term_fresh();
}

/**
 * Interact with the current dungeon level.
 *
 * This function will not exit until the level is completed,
 * the user dies, or the game is terminated.
 */ 
static void dungeon(void)
{
    int py = p_ptr->py;
    int px = p_ptr->px;

    bool just_arrived = TRUE;

    /* Hack -- enforce illegal panel */
    Term->offset_y = DUNGEON_HGT;
    Term->offset_x = DUNGEON_WID;

    /* Not leaving */
    p_ptr->leaving = FALSE;


    /* Reset the "command" vars */
    p_ptr->command_arg = 0;


    /* Cancel the target */
    target_set_monster(0);

    /* Cancel the health bar */
    health_track(0);


    /* Reset shimmer flags */
    shimmer_monsters = TRUE;
    shimmer_objects = TRUE;

    /* Reset repair flags */
    repair_mflag_show = TRUE;
    repair_mflag_mark = TRUE;


    /* Disturb */
    disturb(1, 0);


    /* Track maximum player level */
    if (p_ptr->max_lev < p_ptr->lev) {
	p_ptr->max_lev = p_ptr->lev;
    }

    /* Track depth */
    p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

    /* Autosave */
    is_autosave = TRUE;
    save_game();
    is_autosave = FALSE;
    message_flush();

    /* No stairs down from Quest */
    if ((is_quest(p_ptr->stage))
	&& ((p_ptr->create_stair == FEAT_MORE)
	    || (p_ptr->create_stair == FEAT_MORE_SHAFT)))
	p_ptr->create_stair = 0;


    /* No stairs from town or if not allowed */
    if (p_ptr->depth && MODE(NO_STAIRS)) {
	p_ptr->create_stair = 0;
    }

    /* Make a staircase */
    if (p_ptr->create_stair) {

	/* Place a staircase */
	if (cave_valid_bold(py, px)) {

	    /* XXX XXX XXX */
	    delete_object(py, px);

	    /* Make stairs */
	    cave_set_feat(py, px, p_ptr->create_stair);

	    /* Mark the stairs as known */
	    sqinfo_on(cave_info[py][px], SQUARE_MARK);
	}

	/* Cancel the stair request */
	p_ptr->create_stair = FALSE;
    }


    /* Choose panel */
    verify_panel();


    /* Flush messages */
    message_flush();


    /* Hack -- Increase "xtra" depth */
    character_xtra++;


    /* Clear */
    Term_clear();

    /* Check for vampires */
    if ((p_ptr->schange == SHAPE_VAMPIRE)
	&& ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)))
	shapechange(SHAPE_NORMAL);

    /* Update stuff */
    p_ptr->update |= (PU_BONUS | PU_HP | PU_SPELLS | PU_SPECIALTY);

    /* Calculate torch radius */
    p_ptr->update |= (PU_TORCH);

    /* Fully update the visuals (and monster distances) */
    p_ptr->update |= (PU_FORGET_VIEW | PU_UPDATE_VIEW | PU_DISTANCE);

    /* Redraw dungeon */
    p_ptr->redraw |= (PR_BASIC | PR_EXTRA | PR_MAP);

    /* Redraw stuff */
    p_ptr->redraw |= (PR_INVEN | PR_EQUIP);

    /* Redraw stuff */
    p_ptr->redraw |= (PR_MONSTER | PR_MONLIST | PR_ITEMLIST);

    /* Update stuff */
    update_stuff(p_ptr);

    /* Redraw stuff */
    redraw_stuff(p_ptr);


    /* Hack -- Decrease "xtra" depth */
    character_xtra--;


    /* Update stuff */
    p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);

    /* Combine / Reorder the pack */
    p_ptr->notice |= (PN_COMBINE | PN_REORDER | PN_SORT_QUIVER);

    /* Make basic mouse buttons */
    (void) button_add("[ESC]", ESCAPE);
    (void) button_add("[Ent]", KC_ENTER);
    (void) button_add("[Spc]", ' ');
    (void) button_add("[Rpt]", 'n');
    (void) button_add("[Std]", ',');

    /* Redraw buttons */
    p_ptr->redraw |= (PR_BUTTONS);

    /* Notice stuff */
    notice_stuff(p_ptr);

    /* Update stuff */
    update_stuff(p_ptr);

    /* Redraw stuff */
    redraw_stuff(p_ptr);

    /* Refresh */
    Term_fresh();


    /* Handle delayed death */
    if (p_ptr->is_dead)
	return;


    /* Announce (or repeat) the feeling */
    if ((p_ptr->depth) && (do_feeling))
	do_cmd_feeling();

    /* Announce a player ghost challenge. -LM- */
    if (bones_selector)
	ghost_challenge();

    /* Reset noise */
    total_wakeup_chance = 0;
    add_wakeup_chance = 0;

    /*** Process this dungeon level ***/

    /* Reset the monster generation level */
    monster_level = p_ptr->depth;

    /* Reset the object generation level */
    object_level = p_ptr->depth;

    /* Main loop */
    while (TRUE) {
	/* Hack -- Compact the monster list occasionally */
	if (m_cnt + 32 > z_info->m_max) compact_monsters(64);

	/* Hack -- Compress the monster list occasionally */
	if (m_cnt + 32 < m_max) compact_monsters(0);


	/* Hack -- Compact the object list occasionally */
	if (o_cnt + 32 > z_info->o_max) compact_objects(64);

	/* Hack -- Compress the object list occasionally */
	if (o_cnt + 32 < o_max) compact_objects(0);


	/*** Apply energy ***/

	/* Give the player some energy */
	p_ptr->energy += extract_energy[p_ptr->state.pspeed];

	/* Give the player the first move in a new level */
	if (just_arrived) {
	    just_arrived = FALSE;
	    if (!p_ptr->leaving) {
		int i;
		monster_type *m_ptr;

		/* Set the player energy to exactly 100 */
		p_ptr->energy = 100;

		/* Give the player more energy than any monster */
		for (i = m_max - 1; i >= 1; i--) {
		    /* Access the monster */
		    m_ptr = &m_list[i];

		    /* Ignore dead monsters */
		    if (!m_ptr->r_idx)
			continue;

		    /* Give the player at least as much energy */
		    if (m_ptr->energy > p_ptr->energy)
			p_ptr->energy = m_ptr->energy;
		}
	    }
	}

	/* Can the player move? */
	while (p_ptr->energy >= 100 && !p_ptr->leaving) {
	    /* Do any necessary animations */
	    do_animation(); 

	    /* Process monster with even more energy first */
	    process_monsters((byte) (p_ptr->energy + 1));

	    /* if still alive */
	    if (!p_ptr->leaving) {
		/* Mega hack -redraw big graphics - sorry NRM */
		if ((tile_width > 1) || (tile_height > 1)) 
		    p_ptr->redraw |= (PR_MAP);

		/* Process the player */
		process_player();
	    }
	}

	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(p_ptr);

	/* Update stuff */
	if (p_ptr->update) update_stuff(p_ptr);

	/* Redraw stuff */
	if (p_ptr->redraw) redraw_stuff(p_ptr);

	/* Hack -- Hilight the player */
	place_cursor();

	/* Refresh */
	Term_fresh();

	/* Handle "leaving" */
	if (p_ptr->leaving) break;

	/* Process monsters */
	process_monsters(0);

	/* Reset Monsters */
	reset_monsters();

	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(p_ptr);

	/* Update stuff */
	if (p_ptr->update) update_stuff(p_ptr);

	/* Redraw stuff */
	if (p_ptr->redraw) redraw_stuff(p_ptr);

	/* Hack -- Hilight the player */
	place_cursor();

	/* Refresh */
	Term_fresh();

	/* Handle "leaving" */
	if (p_ptr->leaving) break;


	/* Process the world */
	process_world();

	/* Notice stuff */
	if (p_ptr->notice) notice_stuff(p_ptr);

	/* Update stuff */
	if (p_ptr->update) update_stuff(p_ptr);

	/* Redraw stuff */
	if (p_ptr->redraw) redraw_stuff(p_ptr);

	/* Hack -- Hilight the player */
	place_cursor();

	/* Refresh */
	Term_fresh();

	/* Handle "leaving" */
	if (p_ptr->leaving) break;

	/* Count game turns */
	turn++;
    }

    /* Kill basic mouse buttons */
    (void) button_kill(ESCAPE);
    (void) button_kill(KC_ENTER);
    (void) button_kill(' ');
    (void) button_kill('n');
    (void) button_kill(',');
}



/**
 * Process some user pref files
 */
static void process_some_user_pref_files(void)
{
    char buf[128];

    /* Process the "user.prf" file */
    (void) process_pref_file("user.prf", TRUE, TRUE);

    /* Access the "race" pref file */
    sprintf(buf, "%s.prf", rp_ptr->name);

    /* Process that file */
    process_pref_file(buf, TRUE, TRUE);

    /* Access the "class" pref file */
    sprintf(buf, "%s.prf", cp_ptr->name);

    /* Process that file */
    process_pref_file(buf, TRUE, TRUE);

    /* Get the "PLAYER.prf" filename */
    (void)strnfmt(buf, sizeof(buf), "%s.prf", player_safe_name(p_ptr), TRUE);

    /* Process the "PLAYER.prf" file */
    (void) process_pref_file(buf, TRUE, TRUE);
}

/**
 * Actually play a game
 *
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * player to start a new game.
 *
 * Note that we load the RNG state from savefiles (2.8.0 or later)
 * and so we only initialize it if we were unable to load it, and
 * we mark successful loading using the "Rand_quick" flag.  This
 * is a hack but it optimizes loading of savefiles.  XXX XXX
 */ 
void play_game(void)
{
    int i;
    u32b window_flag[ANGBAND_TERM_MAX];

    /* Initialize */
    bool new_game = init_angband();

    /* Hack -- Increase "icky" depth */
    character_icky++;


    /* Verify main term */
    if (!term_screen) {
	quit("main window does not exist");
    }

    /* Make sure main term is active */
    Term_activate(term_screen);

    /* Verify minimum size */
    if ((Term->hgt < 24) || (Term->wid < 80)) {
	quit("main window is too small");
    }


    /* set a default warning level that will be overridden by the savefile */
    op_ptr->hitpoint_warn = 3;
    
    /* initialize window options that will be overridden by the savefile */
    memset(window_flag, 0, sizeof(u32b)*ANGBAND_TERM_MAX);
    if (ANGBAND_TERM_MAX > 1) window_flag[1] = (PW_INVEN);
    if (ANGBAND_TERM_MAX > 2) window_flag[2] = (PW_PLAYER_0);
    if (ANGBAND_TERM_MAX > 3) window_flag[3] = (PW_MESSAGE);
    if (ANGBAND_TERM_MAX > 4) window_flag[4] = (PW_EQUIP);
    if (ANGBAND_TERM_MAX > 5) window_flag[5] = (PW_MONLIST);
    if (ANGBAND_TERM_MAX > 6) window_flag[6] = (PW_ITEMLIST);
    if (ANGBAND_TERM_MAX > 7) window_flag[7] = (PW_PLAYER_1);

    /* Set up the subwindows */
    subwindows_set_flags(window_flag, ANGBAND_TERM_MAX);

    /* Hack -- turn off the cursor */
    (void) Term_set_cursor(FALSE);

    /*** Try to load the savefile ***/

    p_ptr->is_dead = TRUE;

    if (savefile[0] && file_exists(savefile))
    {
	bool ok = savefile_load(savefile);
	if (!ok) quit("broken savefile");

	if (p_ptr->is_dead && arg_wizard)
	{
	    p_ptr->is_dead = FALSE;
	    p_ptr->noscore |= NOSCORE_WIZARD;
	}
    }

    /* No living character loaded */
    if (p_ptr->is_dead)
    {
	/* Make new player */
	new_game = TRUE;

	/* The dungeon is not ready */
	character_dungeon = FALSE;
    }

    /* Init RNG */
    if (Rand_quick) {
	u32b seed;

	/* Basic seed */
#ifdef _WIN32_WCE
	unsigned long fake_time(unsigned long *fake_time_t);
	seed = fake_time(NULL);
#else
	seed = (time(NULL));
#endif

#ifdef SET_UID

	/* Mutate the seed on Unix machines */
	seed = ((seed >> 3) * (getpid() << 1));

#endif

	/* Use the complex RNG */
	Rand_quick = FALSE;

	/* Seed the "complex" RNG */
	Rand_state_init(seed);
    }

    /* Roll new character */
    if (new_game) {
	/* The dungeon is not ready */
	character_dungeon = FALSE;

	/* Hack -- seed for flavors */
	seed_flavor = randint0(0x10000000);

	/* Hack -- seed for town layouts -NRM- */
	for (i = 0; i < 10; i++)
	    seed_town[i] = randint0(0x10000000);

	/* Roll up a new character */
	player_birth(p_ptr->ht_birth ? TRUE : FALSE);

	/* Start in home town - or on the stairs to Angband */
	if (MODE(THRALL))
	{
	    p_ptr->stage = THRALL_START;
	}
	else
	    p_ptr->stage = p_ptr->home;

	p_ptr->depth = stage_map[p_ptr->stage][DEPTH];

	/* Read the default options */
	process_pref_file("birth.prf", TRUE, TRUE);

    }

    /* Set the savefile name if it's not already set */
    if (!savefile[0])
	savefile_set_name(player_safe_name(p_ptr));

    /* Stop the player being quite so dead */
    p_ptr->is_dead = FALSE;

    /* Flash a message */
    prt("Please wait...", 0, 0);

    /* Flush the message */
    Term_fresh();

    /* Initialize race probabilities */
    if (init_race_probs()) quit("Cannot initialize race probs");

    /* Allow big cursor */
    smlcurs = FALSE;

    /* Flavor the objects */
    flavor_init();

    /* Reset visuals */
    reset_visuals(TRUE);

    /* Initialize the artifact allocation lists */
    init_artifacts();

    /* Tell the UI we've started. */
    event_signal(EVENT_ENTER_GAME);

    /* Redraw stuff */
    p_ptr->redraw |= (PR_INVEN | PR_EQUIP | PR_MESSAGE | PR_MONSTER);
    redraw_stuff(p_ptr);

    /* Process some user pref files */
    process_some_user_pref_files();

    /* React to changes */
    Term_xtra(TERM_XTRA_REACT, 0);


    /* Generate a stage if needed */
    if (!character_dungeon)
	generate_cave();


    /* Character is now "complete" */
    character_generated = TRUE;


    /* Hack -- Decrease "icky" depth */
    character_icky--;


    /* Start playing */
    p_ptr->playing = TRUE;

    /* Hack -- Enforce "delayed death" */
    if (p_ptr->chp < 0)
	p_ptr->is_dead = TRUE;

    /* Process */
    while (TRUE) {
	/* Play ambient sound on change of level. */
	play_ambient_sound();

	/* Process the level */
	dungeon();

	/* Notice stuff */
	if (p_ptr->notice)
	    notice_stuff(p_ptr);

	/* Update stuff */
	if (p_ptr->update)
	    update_stuff(p_ptr);

	/* Redraw stuff */
	if (p_ptr->redraw)
	    redraw_stuff(p_ptr);

	/* Cancel the target */
	target_set_monster(0);

	/* Cancel the health bar */
	health_track(0);


	/* Forget the view */
	forget_view();


	/* Handle "quit and save" */
	if (!p_ptr->playing && !p_ptr->is_dead)
	    break;

	/* Erase the old cave
	   wipe_o_list();
	   wipe_m_list(); */

	/* XXX XXX XXX */
	message_flush();

	/* Accidental Death */
	if (p_ptr->playing && p_ptr->is_dead) {
	    /* Mega-Hack -- Allow player to cheat death */
	    if ((p_ptr->wizard || OPT(cheat_live)) && !get_check("Die? ")) {
		/* Mark social class, reset age, if needed */
		if (p_ptr->sc)
		    p_ptr->sc = p_ptr->age = 0;

		/* Increase age */
		p_ptr->age++;

		/* Mark savefile */
		p_ptr->noscore |= NOSCORE_WIZARD;

		/* Message */
		msg("You invoke wizard mode and cheat death.");
		message_flush();

		/* Cheat death */
		p_ptr->is_dead = FALSE;

		/* Restore hit points */
		p_ptr->chp = p_ptr->mhp;
		p_ptr->chp_frac = 0;

		/* Restore spell points */
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;

		/* Hack -- Healing */
		(void) clear_timed(TMD_POISONED, TRUE);
		(void) clear_timed(TMD_BLIND, TRUE);
		(void) clear_timed(TMD_CONFUSED, TRUE);
		(void) clear_timed(TMD_IMAGE, TRUE);
		(void) clear_timed(TMD_STUN, TRUE);
		(void) clear_timed(TMD_CUT, TRUE);
		(void) clear_timed(TMD_AFRAID, TRUE);
		(void) clear_timed(TMD_PARALYZED, TRUE);
		p_ptr->black_breath = FALSE;

		/* Hack -- Prevent starvation */
		(void) set_food(PY_FOOD_MAX - 1);

		/* Hack -- cancel recall */
		if (p_ptr->word_recall) {
		    /* Message */
		    msg("A tension leaves the air around you...");
		    message_flush();

		    /* Hack -- Prevent recall */
		    p_ptr->word_recall = 0;
		}

		/* Note cause of death XXX XXX XXX */
		strcpy(p_ptr->died_from, "Cheating death");

		/* New stage */
		p_ptr->stage = p_ptr->home;

		/* New depth */
		p_ptr->depth = 0;

		/* Leaving */
		p_ptr->leaving = TRUE;
	    }
	}

	/* Handle "death" */
	if (p_ptr->is_dead)
	    break;

	/* Make a new level */
	generate_cave();
    }

    /* Disallow big cursor */
    smlcurs = TRUE;

    /* Tell the UI we're done with the game state */
    event_signal(EVENT_LEAVE_GAME);

    /* Close stuff */
    close_game();
}
