/*
 * File: player/spell.c
 * Purpose: Spell and prayer casting/praying
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

#include "angband.h"
#include "tvalsval.h"
#include "game-cmd.h"
#include "spells.h"


/**
 * Collect spells from a book into the spells[] array.
 */
int spell_collect_from_book(const object_type * o_ptr,
			    int spells[PY_MAX_SPELLS])
{
    int i;
    int start = mp_ptr->book_start_index[o_ptr->sval];
    int end = mp_ptr->book_start_index[o_ptr->sval + 1];
    int n_spells = 0;

    for (i = start; i < end; i++) {
	int spell = mp_ptr->info[i].index;
	if (spell >= 0)
	    spells[n_spells++] = spell;
    }

    return n_spells;
}


/**
 * Return the number of castable spells in the spellbook 'o_ptr'.
 */
int spell_book_count_spells(const object_type * o_ptr,
			    bool(*tester) (int spell))
{
    int i;
    int start = mp_ptr->book_start_index[o_ptr->sval];
    int end = mp_ptr->book_start_index[o_ptr->sval + 1];
    int n_spells = 0;

    for (i = start; i < end; i++) {
	int spell = mp_ptr->info[i].index;
	if (spell >= 0 && tester(spell))
	    n_spells++;
    }

    return n_spells;
}


/**
 * True if at least one spell in spells[] is OK according to spell_test.
 */
bool spell_okay_list(bool(*spell_test) (int spell), const int spells[],
		     int n_spells)
{
    int i;
    bool okay = FALSE;

    for (i = 0; i < n_spells; i++) {
	if (spell_test(spells[i]))
	    okay = TRUE;
    }

    return okay;
}

/**
 * True if the spell is castable.
 */
bool spell_okay_to_cast(int spell)
{
    if (spell < 32)
	return (p_ptr->spell_learned1 & (1L << spell));
    else
	return (p_ptr->spell_learned2 & (1L << (spell - 32)));
}

/**
 * True if the spell can be studied.
 */
bool spell_okay_to_study(int spell)
{
    const magic_type *s_ptr = &mp_ptr->info[spell];
    bool learned = FALSE;

    if (spell < 32)
	learned = p_ptr->spell_learned1 & (1L << spell);
    else
	learned = p_ptr->spell_learned2 & (1L << (spell - 32));

    return (s_ptr->slevel <= p_ptr->lev) && !learned;
}

/**
 * True if the spell is browsable.
 */
bool spell_okay_to_browse(int spell)
{
    const magic_type *s_ptr = &mp_ptr->info[spell];
    return (s_ptr->slevel < 99);
}


/**
 * Returns chance of failure for a spell
 */
s16b spell_chance(int spell)
{
    int chance, minfail;

    magic_type *s_ptr;


    /* Paranoia -- must be literate */
    if (!mp_ptr->spell_book)
	return (100);

    /* Access the spell */
    s_ptr = &mp_ptr->info[spell];

    /* Extract the base spell failure rate */
    chance = s_ptr->sfail;

    /* Reduce failure rate by "effective" level adjustment */
    chance -= 4 * (p_ptr->lev - s_ptr->slevel);

    /* Reduce failure rate by INT/WIS adjustment */
    chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

    /* Not enough mana to cast */
    if (s_ptr->smana > p_ptr->csp) {
	chance += 5 * (s_ptr->smana - p_ptr->csp);
    }

    /* Extract the minimum failure rate */
    minfail = adj_mag_fail[p_ptr->stat_ind[mp_ptr->spell_stat]];

    /* Minimum failure rate */
    if (chance < minfail)
	chance = minfail;

    /* Stunning makes spells harder (after minfail) */
    if (p_ptr->timed[TMD_STUN] > 50)
	chance += 20;
    else if (p_ptr->timed[TMD_STUN])
	chance += 10;

    /* Always a 5 percent chance of working */
    if (chance > 95)
	chance = 95;

    /* Return the chance */
    return (chance);
}



/* Check if the given spell is in the given book. */
bool spell_in_book(int spell, int book)
{
    int i;
    object_type *o_ptr = object_from_item_idx(book);
    int start = mp_ptr->book_start_index[o_ptr->sval];
    int end = mp_ptr->book_start_index[o_ptr->sval + 1];

    for (i = start; i < end; i++) {
	if (spell == mp_ptr->info[i].index)
	    return TRUE;
    }

    return FALSE;
}


/*
 * Learn the specified spell.
 */
void spell_learn(int spell)
{
    int i;
    cptr p;

    magic_type *s_ptr;

    /* Determine magic description. */
    if (mp_ptr->spell_book == TV_MAGIC_BOOK)
	p = "spell";
    if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	p = "prayer";
    if (mp_ptr->spell_book == TV_DRUID_BOOK)
	p = "druidic lore";
    if (mp_ptr->spell_book == TV_NECRO_BOOK)
	p = "ritual";

    /* Learn the spell */
    if (spell < 32) {
	p_ptr->spell_learned1 |= (1L << spell);
    } else {
	p_ptr->spell_learned2 |= (1L << (spell - 32));
    }

    /* Find the next open entry in "spell_order[]" */
    for (i = 0; i < 64; i++) {
	/* Stop at the first empty space */
	if (p_ptr->spell_order[i] == 99)
	    break;
    }

    /* Add the spell to the known list */
    p_ptr->spell_order[i++] = spell;

    /* Access the spell */
    s_ptr = &mp_ptr->info[spell];

    /* Mention the result */
    message_format(MSG_STUDY, 0, "You have learned the %s of %s.", p,
		   spell_names[s_ptr->index]);

    /* Sound */
    sound(MSG_STUDY);

    /* One less spell available */
    p_ptr->new_spells--;

    /* Message if needed */
    if (p_ptr->new_spells) {
	/* Message */
	msg_format("You can learn %d more %s%s.", p_ptr->new_spells, p,
		   ((p_ptr->new_spells != 1)
		    && (!mp_ptr->spell_book != TV_DRUID_BOOK)) ? "s" : "");
    }

    /* Save the new_spells value */
    p_ptr->old_spells = p_ptr->new_spells;

    /* Redraw Study Status */
    p_ptr->redraw |= (PR_STUDY);
}


/* Cast the specified spell */
bool spell_cast(int spell, int dir)
{
    int chance;
    int plev = p_ptr->lev;
    bool failed = FALSE;
    int py = p_ptr->py;
    int px = p_ptr->px;

    /* Get the spell */
    const magic_type *s_ptr = &mp_ptr->info[spell];

    /* Spell failure chance */
    chance = spell_chance(spell);

    /* Specialty Ability */
    if (player_has(PF_HEIGHTEN_MAGIC))
	plev += 1 + ((p_ptr->heighten_power + 5) / 10);
    if (player_has(PF_CHANNELING))
	plev += get_channeling_boost();

    /* Failed spell */
    if (randint0(100) < chance) {
	failed = TRUE;

	if (OPT(flush_failure))
	    flush();
	if (mp_ptr->spell_book == TV_MAGIC_BOOK)
	    msg_print("You failed to get the spell off!");
	if (mp_ptr->spell_book == TV_PRAYER_BOOK)
	    msg_print("You lost your concentration!");
	if (mp_ptr->spell_book == TV_DRUID_BOOK)
	    msg_print("You lost your concentration!");
	if (mp_ptr->spell_book == TV_NECRO_BOOK)
	    msg_print("You perform the ritual incorrectly!");
    }

    /* Process spell */
    else {
	/* Cast the spell */
	if (!cast_spell(mp_ptr->spell_book, spell, dir))
	    return FALSE;

	/* A spell was cast */
	sound(MSG_SPELL);




	/* A spell was cast or a prayer prayed */
	if (!
	    ((spell <
	      32) ? (p_ptr->spell_worked1 & (1L << spell)) : (p_ptr->
							      spell_worked2 &
							      (1L <<
							       (spell -
								32))))) {
	    int e = s_ptr->sexp;

	    /* The spell or prayer worked */
	    if (spell < 32) {
		p_ptr->spell_worked1 |= (1L << spell);
	    } else {
		p_ptr->spell_worked2 |= (1L << (spell - 32));
	    }

	    /* Gain experience */
	    gain_exp(e * s_ptr->slevel);
	}
    }

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

    /* Hack - simplify rune of mana calculations by fully draining the rune
     * first */
    if ((cave_feat[py][px] == FEAT_RUNE_MANA) && (mana_reserve <= s_ptr->smana)
	&& (s_ptr->index != 60)) {
	p_ptr->csp += mana_reserve;
	mana_reserve = 0;
    }

    /* Rune of mana can take less mana than specified */
    if (s_ptr->index == 60) {
	/* Standard mana amount */
	int mana = 40;

	/* Already full? */
	if (mana_reserve >= MAX_MANA_RESERVE) {
	    /* Use no mana */
	    mana = 0;
	}

	/* Don't put in more than we have */
	else if (p_ptr->csp < mana)
	    mana = p_ptr->csp;

	/* Don't put in more than it will hold */
	if (mana_reserve + mana > MAX_MANA_RESERVE)
	    mana = MAX_MANA_RESERVE - mana_reserve;

	/* Deduct */
	p_ptr->csp -= mana;
    }

    /* Use mana from a rune if possible */
    else if ((cave_feat[py][px] == FEAT_RUNE_MANA)
	     && (mana_reserve > s_ptr->smana)) {
	mana_reserve -= s_ptr->smana;
    }

    /* Sufficient mana */
    else if (s_ptr->smana <= p_ptr->csp) {
	/* Use some mana */
	p_ptr->csp -= s_ptr->smana;

	/* Specialty ability Harmony */
	if ((failed == FALSE) & (player_has(PF_HARMONY))) {
	    int frac, boost;

	    /* Percentage of max hp to be regained */
	    frac = 3 + (s_ptr->smana / 3);

	    /* Cap at 10 % */
	    if (frac > 10)
		frac = 10;

	    /* Calculate fractional bonus */
	    boost = (frac * p_ptr->mhp) / 100;

	    /* Apply bonus */
	    (void) hp_player(boost);
	}
    }

    /* Over-exert the player */
    else {
	int oops = s_ptr->smana - p_ptr->csp;

	/* No mana left */
	p_ptr->csp = 0;
	p_ptr->csp_frac = 0;

	/* Message */
	if (mp_ptr->spell_book == TV_NECRO_BOOK)
	    msg_print("You collapse after the ritual!");
	else
	    msg_print("You faint from the effort!");

	/* Hack -- Bypass free action */
	(void) inc_timed(TMD_PARALYZED, randint1(5 * oops + 1), TRUE);

	/* Damage CON (possibly permanently) */
	if (randint0(100) < 50) {
	    bool perm = (randint0(100) < 25);

	    /* Message */
	    msg_print("You have damaged your health!");

	    /* Reduce constitution */
	    (void) dec_stat(A_CON, 15 + randint1(10), perm);
	}
    }


    /* Redraw mana */
    p_ptr->redraw |= (PR_MANA);

    /* Window stuff */
    p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);

    return TRUE;
}
