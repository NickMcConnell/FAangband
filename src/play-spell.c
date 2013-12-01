/*
 * File: play-spell.c
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
#include "cmds.h"
#include "game-cmd.h"
#include "player.h"
#include "spells.h"
#include "trap.h"
#include "tvalsval.h"


/**
 * Stat Table (INT/WIS) -- Minimum failure rate (percentage)
 */
const byte adj_mag_fail[STAT_RANGE] =
  {
    99	/* 3 */,
    99	/* 4 */,
    99	/* 5 */,
    99	/* 6 */,
    50	/* 7 */,
    40	/* 8 */,
    30	/* 9 */,
    20	/* 10 */,
    15	/* 11 */,
    12	/* 12 */,
    11	/* 13 */,
    10	/* 14 */,
    9	/* 15 */,
    8	/* 16 */,
    7	/* 17 */,
    6	/* 18/00-18/09 */,
    6	/* 18/10-18/19 */,
    5	/* 18/20-18/29 */,
    5	/* 18/30-18/39 */,
    5	/* 18/40-18/49 */,
    4	/* 18/50-18/59 */,
    4	/* 18/60-18/69 */,
    4	/* 18/70-18/79 */,
    4	/* 18/80-18/89 */,
    3	/* 18/90-18/99 */,
    3	/* 18/100-18/109 */,
    2	/* 18/110-18/119 */,
    2	/* 18/120-18/129 */,
    2	/* 18/130-18/139 */,
    2	/* 18/140-18/149 */,
    1	/* 18/150-18/159 */,
    1	/* 18/160-18/169 */,
    1	/* 18/170-18/179 */,
    1	/* 18/180-18/189 */,
    1	/* 18/190-18/199 */,
    0	/* 18/200-18/209 */,
    0	/* 18/210-18/219 */,
    0	/* 18/220+ */
  };


/**
 * Stat Table (INT/WIS) -- Reduction of failure rate
 */
const byte adj_mag_stat[STAT_RANGE] =
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
    6	/* 18/80-18/89 */,
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
		int spell = i;
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
	int spell;
	int start = mp_ptr->book_start_index[o_ptr->sval];
	int end = mp_ptr->book_start_index[o_ptr->sval + 1];
	int n_spells = 0;

	for (spell = start; spell < end; spell++) {
		if (tester(spell))
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
	return (p_ptr->spell_flags[spell] & PY_SPELL_LEARNED);
}

/**
 * True if the spell can be studied.
 */
bool spell_okay_to_study(int spell)
{
	const magic_type *mt_ptr = &mp_ptr->info[spell];
	return (mt_ptr->slevel <= p_ptr->lev) &&
		!(p_ptr->spell_flags[spell] & PY_SPELL_LEARNED);
}

/**
 * True if the spell is browsable.
 */
bool spell_okay_to_browse(int spell)
{
	const magic_type *mt_ptr = &mp_ptr->info[spell];
	return (mt_ptr->slevel < 99);
}


/**
 * Returns chance of failure for a spell
 */
s16b spell_chance(int spell)
{
	int chance, minfail;

	magic_type *mt_ptr;


	/* Paranoia -- must be literate */
	if (!mp_ptr->spell_book)
		return (100);

	/* Access the spell */
	mt_ptr = &mp_ptr->info[spell];

	/* Extract the base spell failure rate */
	chance = mt_ptr->sfail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 4 * (p_ptr->lev - mt_ptr->slevel);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -=
		3 * (adj_mag_stat[p_ptr->state.stat_ind[mp_ptr->spell_stat]] - 1);

	/* Not enough mana to cast */
	if (mt_ptr->smana > p_ptr->csp) {
		chance += 5 * (mt_ptr->smana - p_ptr->csp);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->state.stat_ind[mp_ptr->spell_stat]];

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
	object_type *o_ptr = object_from_item_idx(book);
	int start = mp_ptr->book_start_index[o_ptr->sval];
	int end = mp_ptr->book_start_index[o_ptr->sval + 1];

	if ((spell >= start) && (spell < end))
		return TRUE;

	return FALSE;
}


/*
 * Learn the specified spell.
 */
void spell_learn(int spell)
{
	int i;

	magic_type *mt_ptr;

	/* Learn the spell */
	p_ptr->spell_flags[spell] |= PY_SPELL_LEARNED;

	/* Find the next open entry in "spell_order[]" */
	for (i = 0; i < PY_MAX_SPELLS; i++) {
		/* Stop at the first empty space */
		if (p_ptr->spell_order[i] == 99)
			break;
	}

	/* Add the spell to the known list */
	p_ptr->spell_order[i] = spell;

	/* Access the spell */
	mt_ptr = &mp_ptr->info[spell];

	/* Mention the result */
	msgt(MSG_STUDY, "You have learned the %s of %s.",
		 magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
		 get_spell_name(mt_ptr->index));

	/* Sound */
	sound(MSG_STUDY);

	/* One less spell available */
	p_ptr->new_spells--;

	/* Message if needed */
	if (p_ptr->new_spells) {
		/* Message */
		msg("You can learn %d more %s%s.", p_ptr->new_spells,
			magic_desc[mp_ptr->spell_realm][SPELL_NOUN],
			((p_ptr->new_spells != 1)
			 && (!mp_ptr->spell_book != TV_DRUID_BOOK)) ? "s" : "");
	}

	/* Redraw Study Status */
	p_ptr->redraw |= (PR_STUDY | PR_OBJECT);
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
	const magic_type *mt_ptr = &mp_ptr->info[spell];

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

		flush();
		msg(magic_desc[mp_ptr->spell_realm][SPELL_FAIL]);
	}

	/* Process spell */
	else {
		/* Cast the spell */
		if (!cast_spell(mp_ptr->spell_book, mt_ptr->index, dir, plev))
			return FALSE;

		/* A spell was cast */
		sound(MSG_SPELL);


		/* A spell was cast or a prayer prayed */
		if (!(p_ptr->spell_flags[spell] & PY_SPELL_WORKED)) {
			int e = mt_ptr->sexp;

			/* The spell worked */
			p_ptr->spell_flags[spell] |= PY_SPELL_WORKED;

			/* Gain experience */
			gain_exp(e * mt_ptr->slevel);

			/* Redraw object recall */
			p_ptr->redraw |= (PR_OBJECT);
		}
	}

	/* Hack - simplify rune of mana calculations by fully draining the rune
	 * first */
	if (cave_trap_specific(py, px, RUNE_MANA) &&
		(mana_reserve <= mt_ptr->smana) && (mt_ptr->index != 60)) {
		p_ptr->csp += mana_reserve;
		mana_reserve = 0;
	}

	/* Rune of mana can take less mana than specified */
	if (mt_ptr->index == 60) {
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
	else if (cave_trap_specific(py, px, RUNE_MANA)
			 && (mana_reserve > mt_ptr->smana)) {
		mana_reserve -= mt_ptr->smana;
	}

	/* Sufficient mana */
	else if (mt_ptr->smana <= p_ptr->csp) {
		/* Use some mana */
		p_ptr->csp -= mt_ptr->smana;

		/* Specialty ability Harmony */
		if ((failed == FALSE) & (player_has(PF_HARMONY))) {
			int frac, boost;

			/* Percentage of max hp to be regained */
			frac = 3 + (mt_ptr->smana / 3);

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
		int oops = mt_ptr->smana - p_ptr->csp;

		/* No mana left */
		p_ptr->csp = 0;
		p_ptr->csp_frac = 0;

		/* Message */
		if (mp_ptr->spell_realm == REALM_NECROMANTIC)
			msg("You collapse after the ritual!");
		else
			msg("You faint from the effort!");

		/* Hack -- Bypass free action */
		(void) inc_timed(TMD_PARALYZED, randint1(5 * oops + 1), TRUE);

		/* Damage CON (possibly permanently) */
		if (randint0(100) < 50) {
			bool perm = (randint0(100) < 25);

			/* Message */
			msg("You have damaged your health!");

			/* Reduce constitution */
			(void) dec_stat(A_CON, 15 + randint1(10), perm);
		}
	}


	/* Redraw mana */
	p_ptr->redraw |= (PR_MANA);

	return TRUE;
}
