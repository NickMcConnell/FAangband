/** \file monster1.c 
    \brief Monster descriptions and info

 * how the char gains new monster info and the monster recall, including 
 * all descriptions.  The player ghost code.
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


/**
 * Returns the r_idx of the monster with the given name. If no monster has
 * the exact name given, returns the r_idx of the first monster having the
 * given name as a (case-insensitive) substring.
 *
 * Returns -1 if no match is found.
 */
int lookup_monster(const char *name)
{
	int i;
	int r_idx = -1;

	/* Look for it */
	for (i = 1; i < z_info->r_max; i++) {
		monster_race *r_ptr = &r_info[i];

		/* Test for equality */
		if (r_ptr->name && streq(name, r_ptr->name))
			return i;

		/* Test for close matches */
		if (r_ptr->name && my_stristr(r_ptr->name, name) && r_idx == -1)
			r_idx = i;
	}

	/* Return our best match */
	return r_idx;
}

/**
 * Return the monster base matching the given name.
 */
monster_base *lookup_monster_base(const char *name)
{
	monster_base *base;

	/* Look for it */
	for (base = rb_info; base; base = base->next) {
		if (streq(name, base->name))
			return base;
	}

	return NULL;
}

/**
 * Return whether the given base matches any of the names given.
 *
 * Accepts a variable-length list of name strings. The list must end with NULL.
 */
bool match_monster_bases(const monster_base * base, ...)
{
	bool ok = FALSE;
	va_list vp;
	char *name;

	va_start(vp, base);
	while (!ok && ((name = va_arg(vp, char *)) != NULL))
		 ok = base == lookup_monster_base(name);
	va_end(vp);

	return ok;
}

/**
 * Pronoun arrays, by gender.
 */
static const char *wd_he[3] = { "It", "He", "She" };

/**
 * Pronoun arrays, by gender.
 */
static const char *wd_his[3] = { "its", "his", "her" };

/**
 * Descriptions of monster rarity.
 */
static char *wd_rarity(byte rarity, bool unique)
{
	static char rarity_desc[40];

	if (unique) {
		if (rarity == 1)
			strcpy(rarity_desc, "very often encountered");
		else if (rarity == 2)
			strcpy(rarity_desc, "often encountered");
		else if (rarity == 3)
			strcpy(rarity_desc, "fairly often encountered");
		else if (rarity == 4)
			strcpy(rarity_desc, "infrequently encountered");
		else if ((rarity == 5) || (rarity == 6))
			strcpy(rarity_desc, "seldom encountered");
		else if (rarity < 10)
			strcpy(rarity_desc, "very seldom encountered");
		else
			strcpy(rarity_desc, "almost never encountered");
	} else {
		if (rarity == 1)
			strcpy(rarity_desc, "very common");
		else if (rarity == 2)
			strcpy(rarity_desc, "common");
		else if (rarity == 3)
			strcpy(rarity_desc, "not very common");
		else if ((rarity == 4) || (rarity == 5))
			strcpy(rarity_desc, "fairly rare");
		else if (rarity < 9)
			strcpy(rarity_desc, "rare");
		else
			strcpy(rarity_desc, "extremely rare");
	}

	return rarity_desc;
}

/**
 * Pluralizer.  Args(count, singular, plural)
 */
#define plural(c,s,p) \
    (((c) == 1) ? (s) : (p))

/**
 * Determine if the "armor" is known
 * The higher the level, the fewer kills needed.
 */
static bool know_armour(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	s32b level = r_ptr->level;

	s32b kills = l_list[r_idx].tkills;

	/* Rangers learn quickly. */
	if (player_has(PF_LORE))
		kills *= 2;

	/* Normal monsters */
	if (kills > 304 / (4 + level))
		return (TRUE);

	/* Skip non-uniques */
	if (!(rf_has(r_ptr->flags, RF_UNIQUE)))
		return (FALSE);

	/* Unique monsters */
	if (kills > 304 / (38 + (5 * level) / 4))
		return (TRUE);

	/* Assume false */
	return (FALSE);
}


/**
 * Determine if the "mana" is known
 * The higher the level, the fewer kills needed.
 */
static bool know_mana(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	s32b level = r_ptr->level;

	s32b kills = l_list[r_idx].tkills;

	/* Stong sorcery users (i.e. Mages) learn quickly. */
	if ((mp_ptr->spell_book == TV_MAGIC_BOOK)
		&& (player_has(PF_STRONG_MAGIC)))
		kills *= 2;

	/* Normal monsters */
	if (kills > 304 / (4 + level))
		return (TRUE);

	/* Skip non-uniques */
	if (!(rf_has(r_ptr->flags, RF_UNIQUE)))
		return (FALSE);

	/* Unique monsters */
	if (kills > 304 / (38 + (5 * level) / 4))
		return (TRUE);

	/* Assume false */
	return (FALSE);
}


/**
 * Determine if the "damage" of the given attack is known
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 */
static bool know_damage(int r_idx, int i)
{
	monster_race *r_ptr = &r_info[r_idx];

	s32b level = r_ptr->level;

	s32b a = l_list[r_idx].blows[i];

	s32b d1 = r_ptr->blow[i].d_dice;
	s32b d2 = r_ptr->blow[i].d_side;

	s32b d = d1 * d2;

	/* Hack - keep the target number reasonable */
	if (d > 100)
		d = 100;

	/* Hack -- Rangers learn quickly. */
	if (player_has(PF_LORE))
		level = 10 + 3 * level / 2;

	/* Normal monsters */
	if ((4 + level) * a > 80 * d)
		return (TRUE);

	/* Skip non-uniques */
	if (!(rf_has(r_ptr->flags, RF_UNIQUE)))
		return (FALSE);

	/* Unique monsters */
	if ((4 + level) * (2 * a) > 80 * d)
		return (TRUE);

	/* Assume false */
	return (FALSE);
}


/**
 * Hack -- display monster information using "roff()"
 *
 * Note that there is now a compiler option to only read the monster
 * descriptions from the raw file when they are actually needed, which
 * saves about 60K of memory at the cost of disk access during monster
 * recall, which is optional to the user.
 *
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
extern void describe_monster(int r_idx, bool spoilers)
{
	const monster_race *r_ptr;
	monster_lore *l_ptr;

	bool old = FALSE;
	bool sin = FALSE;
	bool unique = FALSE;

	int m, n, r;

	char *p, *q;
	char name[80];

	int msex = 0;

	bool breath = FALSE;
	bool magic = FALSE;

	bitflag mon_flags[RF_SIZE];
	bitflag mon_spells[RSF_SIZE];

	int spower;

	int vn = 0;
	char *vp[64];
	char buf[2048];

	monster_lore save_mem;

	/* Access the race and lore */
	r_ptr = &r_info[r_idx];
	l_ptr = &l_list[r_idx];

	/* Get the monster name */
	my_strcpy(name, r_ptr->name, sizeof(name));

	/* Cheat -- know everything */
	if (OPT(cheat_know) || (rf_has(r_ptr->flags, RF_PLAYER_GHOST))) {
		/* XXX XXX XXX */

		/* Hack -- save memory */
		COPY(&save_mem, l_ptr, monster_lore);

		/* Hack -- Maximal kills */
		l_ptr->tkills = MAX_SHORT;

		/* Hack -- Maximal info */
		l_ptr->wake = l_ptr->ignore = MAX_UCHAR;

		/* Observe "maximal" attacks */
		for (m = 0; m < 4; m++) {
			/* Examine "actual" blows */
			if (r_ptr->blow[m].effect || r_ptr->blow[m].method) {
				/* Hack -- maximal observations */
				l_ptr->blows[m] = MAX_UCHAR;
			}
		}

		/* Hack -- maximal drops */
		l_ptr->drop_gold = l_ptr->drop_item =
			(((rf_has(r_ptr->flags, RF_DROP_4D2)) ? 8 : 0) +
			 ((rf_has(r_ptr->flags, RF_DROP_3D2)) ? 6 : 0) +
			 ((rf_has(r_ptr->flags, RF_DROP_2D2)) ? 4 : 0) +
			 ((rf_has(r_ptr->flags, RF_DROP_1D2)) ? 2 : 0) +
			 ((rf_has(r_ptr->flags, RF_DROP_90)) ? 1 : 0) +
			 ((rf_has(r_ptr->flags, RF_DROP_60)) ? 1 : 0));

		/* Hack -- but only "valid" drops */
		if (rf_has(r_ptr->flags, RF_ONLY_GOLD))
			l_ptr->drop_item = 0;
		if (rf_has(r_ptr->flags, RF_ONLY_ITEM))
			l_ptr->drop_gold = 0;

		/* Hack -- observe many spells */
		l_ptr->cast_spell = MAX_UCHAR;

		/* Hack -- know all the flags */
		rf_copy(l_ptr->flags, r_ptr->flags);
		rsf_copy(l_ptr->spell_flags, r_ptr->spell_flags);
	}


	/* Extract a gender (if applicable) */
	if (rf_has(r_ptr->flags, RF_FEMALE))
		msex = 2;
	else if (rf_has(r_ptr->flags, RF_MALE))
		msex = 1;


	/* Obtain a copy of the "known" flags */
	rf_copy(mon_flags, l_ptr->flags);
	rsf_copy(mon_spells, l_ptr->spell_flags);
	(void) rf_inter(mon_flags, r_ptr->flags);
	(void) rf_inter(mon_spells, r_ptr->spell_flags);

	/* Assume some "obvious" flags */
	if (rf_has(r_ptr->flags, RF_UNIQUE))
		rf_on(mon_flags, RF_UNIQUE);
	if (rf_has(r_ptr->flags, RF_QUESTOR))
		rf_on(mon_flags, RF_QUESTOR);
	if (rf_has(r_ptr->flags, RF_MALE))
		rf_on(mon_flags, RF_MALE);
	if (rf_has(r_ptr->flags, RF_FEMALE))
		rf_on(mon_flags, RF_FEMALE);

	/* Assume some "creation" flags */
	if (rf_has(r_ptr->flags, RF_FRIEND))
		rf_on(mon_flags, RF_FRIEND);
	if (rf_has(r_ptr->flags, RF_FRIENDS))
		rf_on(mon_flags, RF_FRIENDS);
	if (rf_has(r_ptr->flags, RF_ESCORT))
		rf_on(mon_flags, RF_ESCORT);
	if (rf_has(r_ptr->flags, RF_ESCORTS))
		rf_on(mon_flags, RF_ESCORTS);

	/* Assume some "location" flags */
	if (rf_has(r_ptr->flags, RF_ANGBAND))
		rf_on(mon_flags, RF_ANGBAND);
	if (rf_has(r_ptr->flags, RF_RUDH))
		rf_on(mon_flags, RF_RUDH);
	if (rf_has(r_ptr->flags, RF_NARGOTHROND))
		rf_on(mon_flags, RF_NARGOTHROND);
	if (rf_has(r_ptr->flags, RF_DUNGORTHEB))
		rf_on(mon_flags, RF_DUNGORTHEB);
	if (rf_has(r_ptr->flags, RF_GAURHOTH))
		rf_on(mon_flags, RF_GAURHOTH);
	if (rf_has(r_ptr->flags, RF_DUNGEON))
		rf_on(mon_flags, RF_DUNGEON);

	/* Killing a monster reveals some properties */
	if (l_ptr->tkills) {
		/* Know "race" flags */
		if (rf_has(r_ptr->flags, RF_ORC))
			rf_on(mon_flags, RF_ORC);
		if (rf_has(r_ptr->flags, RF_TROLL))
			rf_on(mon_flags, RF_TROLL);
		if (rf_has(r_ptr->flags, RF_GIANT))
			rf_on(mon_flags, RF_GIANT);
		if (rf_has(r_ptr->flags, RF_DRAGON))
			rf_on(mon_flags, RF_DRAGON);
		if (rf_has(r_ptr->flags, RF_DEMON))
			rf_on(mon_flags, RF_DEMON);
		if (rf_has(r_ptr->flags, RF_UNDEAD))
			rf_on(mon_flags, RF_UNDEAD);
		if (rf_has(r_ptr->flags, RF_EVIL))
			rf_on(mon_flags, RF_EVIL);
		if (rf_has(r_ptr->flags, RF_ANIMAL))
			rf_on(mon_flags, RF_ANIMAL);
		if (rf_has(r_ptr->flags, RF_TERRITORIAL))
			rf_on(mon_flags, RF_TERRITORIAL);

		/* Know "forced" flags */
		if (rf_has(r_ptr->flags, RF_FORCE_DEPTH))
			rf_on(mon_flags, RF_FORCE_DEPTH);
		if (rf_has(r_ptr->flags, RF_FORCE_MAXHP))
			rf_on(mon_flags, RF_FORCE_MAXHP);
	}

	/* Define a convenience variable */
	if (rf_has(r_ptr->flags, RF_UNIQUE))
		unique = TRUE;

	/* Descriptions */

	my_strcpy(buf, r_ptr->text, sizeof(buf));

	/* Dump it */
	text_out_indent = 0;
	text_out_to_screen(TERM_L_BLUE, buf);
	text_out_to_screen(TERM_WHITE, "  ");

	/* Player ghosts may have unique descriptions. */
	if ((rf_has(r_ptr->flags, RF_PLAYER_GHOST))
		&& (ghost_string_type == 2))
		text_out_to_screen(TERM_L_BLUE, format("%s  ", ghost_string));


	/* Notice "Quest" monsters */
	if (rf_has(mon_flags, RF_QUESTOR)) {
		text_out_to_screen(TERM_VIOLET,
						   "You feel an intense desire to kill this monster...  ");
	}

	/* New paragraph. */
	text_out_to_screen(TERM_WHITE, "\n");

	/* Treat uniques differently */
	if (unique) {
		/* Hack -- Determine if the unique is "dead" */
		bool dead = (r_ptr->max_num == 0) ? TRUE : FALSE;

		/* We've been killed... */
		if (l_ptr->deaths) {
			/* Killed ancestors */
			text_out_to_screen(TERM_WHITE,
							   format("%s has slain ", wd_he[msex]));

			text_out_to_screen(TERM_WHITE, format("%d ", l_ptr->deaths));

			text_out_to_screen(TERM_WHITE, "of your ancestors");

			/* But we've also killed it */
			if (dead) {
				text_out_to_screen(TERM_WHITE,
								   format(", but you have avenged %s!  ",
										  plural(l_ptr->deaths, "him",
												 "them")));
			}

			/* Unavenged (ever) */
			else {
				text_out_to_screen(TERM_WHITE,
								   format(", who %s unavenged.  ",
										  plural(l_ptr->deaths, "remains",
												 "remain")));
			}
		}

		/* Dead unique who never hurt us */
		else if (dead) {
			text_out_to_screen(TERM_WHITE, "You have slain this foe.  ");
		}

	}

	/* Not unique, but killed us */
	else if (l_ptr->deaths) {
		/* Dead ancestors */
		text_out_to_screen(TERM_WHITE, format("%d ", l_ptr->deaths));

		text_out_to_screen(TERM_WHITE,
						   format
						   ("of your ancestors %s been killed by this creature, ",
							plural(l_ptr->deaths, "has", "have")));

		/* Some kills this life */
		if (l_ptr->pkills) {
			text_out_to_screen(TERM_WHITE,
							   "and you have exterminated at least ");
			text_out_to_screen(TERM_WHITE, format("%d ", l_ptr->pkills));
			text_out_to_screen(TERM_WHITE, "of the creatures.  ");
		}

		/* Some kills past lives */
		else if (l_ptr->tkills) {
			text_out_to_screen(TERM_WHITE,
							   "and your ancestors have exterminated at least ");
			text_out_to_screen(TERM_WHITE, format("%d ", l_ptr->tkills));
			text_out_to_screen(TERM_WHITE, "of the creatures.  ");
		}

		/* No kills */
		else {
			text_out_to_screen(TERM_RED,
							   "which is not ever known to have been defeated.  ");
		}
	}

	/* Normal monsters */
	else {
		/* Killed some this life */
		if (l_ptr->pkills) {
			text_out_to_screen(TERM_WHITE, "You have killed at least ");
			text_out_to_screen(TERM_WHITE, format("%d ", l_ptr->pkills));
			text_out_to_screen(TERM_WHITE, "of these creatures.  ");
		}

		/* Killed some last life */
		else if (l_ptr->tkills) {
			text_out_to_screen(TERM_WHITE,
							   "Your ancestors have killed at least ");
			text_out_to_screen(TERM_WHITE, format("%d ", l_ptr->tkills));
			text_out_to_screen(TERM_WHITE, "of these creatures.  ");
		}

		/* Killed none */
		else {
			text_out_to_screen(TERM_RED,
							   "No battles to the death are recalled.  ");
		}
	}

	/* Nothing yet */
	old = FALSE;

	/* Describe location */
	if (r_ptr->level == 0) {
		text_out_to_screen(TERM_SLATE,
						   format("%s lives in the town", wd_he[msex]));
		old = TRUE;
	} else if (l_ptr->tkills) {
		char depth_desc[80];
		char location_desc[80];

		byte con_color = TERM_WHITE;

		strcpy(location_desc, "");

		if (rf_has(mon_flags, RF_ANGBAND))
			strcpy(location_desc, " in Angband");
		if (rf_has(mon_flags, RF_RUDH))
			strcpy(location_desc, " in Amon Rûdh");
		if (rf_has(mon_flags, RF_NARGOTHROND))
			strcpy(location_desc, " in Nargothrond");
		if (rf_has(mon_flags, RF_DUNGORTHEB))
			strcpy(location_desc, " in Nan Dungortheb");
		if (rf_has(mon_flags, RF_GAURHOTH))
			strcpy(location_desc, " in Tol-In-Gaurhoth");
		if (rf_has(mon_flags, RF_DUNGEON))
			strcpy(location_desc, " in dungeons");

		strcpy(depth_desc,
			   format("at a danger level of %d%s", r_ptr->level,
					  location_desc));


		/* Determine "con" of the monster (default is TERM_WHITE) */
		if (r_ptr->level < p_ptr->recall[0] - 5)
			con_color = TERM_SLATE;
		if (r_ptr->level > p_ptr->recall[0] + 1)
			con_color = TERM_RED;

		/* Build the description of rarity and location. */
		if (rf_has(r_ptr->flags, RF_QUESTOR)) {
			/* Questor monsters are fixed-depth, and always appear. */
			text_out_to_screen(TERM_WHITE,
							   format("%s is always found ", wd_he[msex]));
			text_out_to_screen(con_color, format("%s", depth_desc));
		}

		else if (rf_has(r_ptr->flags, RF_FORCE_DEPTH)) {
			text_out_to_screen(TERM_WHITE,
							   format("%s is %s, always found ",
									  wd_he[msex], wd_rarity(r_ptr->rarity,
															 unique)));
			text_out_to_screen(con_color, format("%s", depth_desc));
		} else {
			text_out_to_screen(TERM_WHITE,
							   format("%s is %s, normally found ",
									  wd_he[msex], wd_rarity(r_ptr->rarity,
															 unique)));
			text_out_to_screen(con_color, format("%s", depth_desc));
		}

		old = TRUE;
	}



	/* Describe movement */
	if (TRUE) {
		/* Introduction */
		if (old) {
			text_out_to_screen(TERM_WHITE, ", and ");
		} else {
			text_out_to_screen(TERM_WHITE, format("%s ", wd_he[msex]));
			old = TRUE;
		}
		if (rf_has(mon_flags, RF_NEVER_MOVE))
			text_out_to_screen(TERM_WHITE, "acts");
		else
			text_out_to_screen(TERM_WHITE, "moves");

		/* Random-ness */
		if ((rf_has(mon_flags, RF_RAND_50))
			|| (rf_has(mon_flags, RF_RAND_25))) {
			/* Adverb */
			if ((rf_has(mon_flags, RF_RAND_50))
				&& (rf_has(mon_flags, RF_RAND_25))) {
				text_out_to_screen(TERM_WHITE, " extremely");
			} else if (rf_has(mon_flags, RF_RAND_50)) {
				text_out_to_screen(TERM_WHITE, " somewhat");
			} else if (rf_has(mon_flags, RF_RAND_25)) {
				text_out_to_screen(TERM_WHITE, " a bit");
			}

			/* Adjective */
			text_out_to_screen(TERM_WHITE, " erratically");

			/* Hack -- Occasional conjunction */
			if (r_ptr->speed != 110)
				text_out_to_screen(TERM_WHITE, ", and");
		}

		/* Speed */
		if (r_ptr->speed > 110) {
			if (r_ptr->speed > 130)
				text_out_to_screen(TERM_UMBER, " incredibly");
			else if (r_ptr->speed > 120)
				text_out_to_screen(TERM_UMBER, " very");
			else if (r_ptr->speed < 116)
				text_out_to_screen(TERM_UMBER, " fairly");
			text_out_to_screen(TERM_UMBER, " quickly");
		} else if (r_ptr->speed < 110) {
			if (r_ptr->speed < 90)
				text_out_to_screen(TERM_UMBER, " extremely");
			else if (r_ptr->speed < 100)
				text_out_to_screen(TERM_UMBER, " very");
			text_out_to_screen(TERM_UMBER, " slowly");
		} else {
			text_out_to_screen(TERM_UMBER, " at normal speed");
		}
	}

	/* The code above includes "attack speed" */
	if (rf_has(mon_flags, RF_NEVER_MOVE)) {
		/* Introduce */
		if (old) {
			text_out_to_screen(TERM_WHITE, ", but ");
		} else {
			text_out_to_screen(TERM_WHITE, format("%s ", wd_he[msex]));
			old = TRUE;
		}

		/* Describe */
		text_out_to_screen(TERM_WHITE,
						   "does not deign to chase intruders");
	}

	/* End this sentence */
	if (old) {
		text_out_to_screen(TERM_WHITE, ".  ");
		old = FALSE;
	}


	/* Describe experience if known */
	if (l_ptr->tkills) {
		/* Introduction */
		if (rf_has(mon_flags, RF_UNIQUE)) {
			text_out_to_screen(TERM_WHITE, "Killing this");
		} else {
			text_out_to_screen(TERM_WHITE, "A kill of this");
		}

		/* Describe the "quality" */
		if (rf_has(mon_flags, RF_ANIMAL))
			text_out_to_screen(TERM_BLUE, " natural");
		if (rf_has(mon_flags, RF_TERRITORIAL))
			text_out_to_screen(TERM_BLUE, " territorial");
		if (rf_has(mon_flags, RF_EVIL))
			text_out_to_screen(TERM_BLUE, " evil");
		if (rf_has(mon_flags, RF_UNDEAD))
			text_out_to_screen(TERM_BLUE, " undead");

		/* Describe the "race" */
		if (rf_has(mon_flags, RF_DRAGON))
			text_out_to_screen(TERM_BLUE, " dragon");
		else if (rf_has(mon_flags, RF_DEMON))
			text_out_to_screen(TERM_BLUE, " demon");
		else if (rf_has(mon_flags, RF_GIANT))
			text_out_to_screen(TERM_BLUE, " giant");
		else if (rf_has(mon_flags, RF_TROLL))
			text_out_to_screen(TERM_BLUE, " troll");
		else if (rf_has(mon_flags, RF_ORC))
			text_out_to_screen(TERM_BLUE, " orc");
		else
			text_out_to_screen(TERM_WHITE, " creature");

		/* Group some variables */
		if (TRUE) {
			long i, j;

			/* calculate the integer exp part */
			i = (long) r_ptr->mexp * r_ptr->level / p_ptr->lev;

			/* calculate the fractional exp part scaled by 100, */
			/* must use long arithmetic to avoid overflow */
			j = ((((long) r_ptr->mexp * r_ptr->level % p_ptr->lev) *
				  (long) 1000 / p_ptr->lev + 5) / 10);

			/* Mention the experience */
			text_out_to_screen(TERM_WHITE, " is worth ");
			text_out_to_screen(TERM_VIOLET,
							   format("%ld.%02ld ", (long) i, (long) j));
			text_out_to_screen(TERM_WHITE,
							   format("point%s",
									  (((i == 1)
										&& (j == 0)) ? "" : "s")));

			/* Take account of annoying English */
			p = "th";
			i = p_ptr->lev % 10;
			if ((p_ptr->lev / 10) == 1)	/* nothing */
				;
			else if (i == 1)
				p = "st";
			else if (i == 2)
				p = "nd";
			else if (i == 3)
				p = "rd";

			/* Take account of "leading vowels" in numbers */
			q = "";
			i = p_ptr->lev;
			if ((i == 8) || (i == 11) || (i == 18))
				q = "n";

			/* Mention the dependance on the player's level */
			text_out_to_screen(TERM_WHITE,
							   format(" for a%s %lu%s level character.  ",
									  q, (long) i, p));
		}
	}
	/* If no kills, known racial information should still be displayed. */
	else if ((rf_has(mon_flags, RF_ANIMAL)) || (rf_has(mon_flags, RF_EVIL))
			 || (rf_has(mon_flags, RF_UNDEAD))
			 || (rf_has(mon_flags, RF_DRAGON))
			 || (rf_has(mon_flags, RF_DEMON))
			 || (rf_has(mon_flags, RF_GIANT))
			 || (rf_has(mon_flags, RF_TROLL))
			 || (rf_has(mon_flags, RF_ORC))
			 || (rf_has(mon_flags, RF_TERRITORIAL))) {
		bool add = FALSE;

		/* Pronoun. */
		if (rf_has(mon_flags, RF_UNIQUE))
			text_out_to_screen(TERM_WHITE, format("%s is a", wd_he[msex]));
		else
			text_out_to_screen(TERM_WHITE, "It is a");

		/* Describe the "quality" */
		if (rf_has(mon_flags, RF_ANIMAL)) {
			text_out_to_screen(TERM_WHITE, " natural");
			add = TRUE;
		}
		/* Describe the "quality" */
		if (rf_has(mon_flags, RF_TERRITORIAL)) {
			text_out_to_screen(TERM_WHITE, " territorial");
			add = TRUE;
		}
		if (rf_has(mon_flags, RF_EVIL)) {
			if (add)
				text_out_to_screen(TERM_WHITE, " evil");
			else
				text_out_to_screen(TERM_WHITE, "n evil");
			add = TRUE;
		}
		if (rf_has(mon_flags, RF_UNDEAD)) {
			if (add)
				text_out_to_screen(TERM_WHITE, " undead");
			else
				text_out_to_screen(TERM_WHITE, "n undead");
			add = TRUE;
		}

		/* Describe the "race" */
		if (rf_has(mon_flags, RF_DRAGON))
			text_out_to_screen(TERM_WHITE, " dragon");
		else if (rf_has(mon_flags, RF_DEMON))
			text_out_to_screen(TERM_WHITE, " demon");
		else if (rf_has(mon_flags, RF_GIANT))
			text_out_to_screen(TERM_WHITE, " giant");
		else if (rf_has(mon_flags, RF_TROLL))
			text_out_to_screen(TERM_WHITE, " troll");
		else if (rf_has(mon_flags, RF_ORC)) {
			if (add)
				text_out_to_screen(TERM_WHITE, " orc");
			else
				text_out_to_screen(TERM_WHITE, "n orc");
		} else
			text_out_to_screen(TERM_WHITE, " creature");

		/* End sentence. */
		text_out_to_screen(TERM_WHITE, ".  ");
	}

	/* Describe escorts */
	if ((rf_has(mon_flags, RF_ESCORT)) || (rf_has(mon_flags, RF_ESCORTS))) {
		text_out_to_screen(TERM_WHITE,
						   format("%s usually appears with escorts.  ",
								  wd_he[msex]));
	}

	/* Describe friends */
	else if ((rf_has(mon_flags, RF_FRIEND))
			 || (rf_has(mon_flags, RF_FRIENDS))) {
		text_out_to_screen(TERM_WHITE,
						   format("%s usually appears in groups.  ",
								  wd_he[msex]));
	}

	/* Get spell power */
	spower = r_ptr->spell_power;


	/* Collect inate attacks */
	vn = 0;
	if (rsf_has(mon_spells, RSF_SHRIEK))
		vp[vn++] = "shriek for help";
	if (rsf_has(mon_spells, RSF_LASH)) {
		if (rf_has(mon_flags, RF_ANIMAL)
			|| (r_ptr->blow[0].effect == RBE_ACID))
			vp[vn++] = "spit at you from a distance";
		else
			vp[vn++] = "lash you if nearby";
	}
	if (rsf_has(mon_spells, RSF_BOULDER)) {
		if (spower < 10)
			vp[vn++] = "throw rocks";
		else
			vp[vn++] = "throw boulders";
	}
	if (rsf_has(mon_spells, RSF_SHOT)) {
		if (spower < 5)
			vp[vn++] = "sling pebbles";
		else if (spower < 15)
			vp[vn++] = "sling leaden pellets";
		else
			vp[vn++] = "sling seeker shots";
	}
	if (rsf_has(mon_spells, RSF_ARROW)) {
		if (spower < 8)
			vp[vn++] = "shoot little arrows";
		else if (spower < 15)
			vp[vn++] = "shoot arrows";
		else
			vp[vn++] = "shoot seeker arrows";
	}
	if (rsf_has(mon_spells, RSF_BOLT)) {
		if (spower < 8)
			vp[vn++] = "fire bolts";
		else if (spower < 15)
			vp[vn++] = "fire crossbow quarrels";
		else
			vp[vn++] = "fire seeker bolts";
	}
	if (rsf_has(mon_spells, RSF_MISSL)) {
		if (spower < 8)
			vp[vn++] = "fire little missiles";
		else if (spower < 15)
			vp[vn++] = "fire missiles";
		else
			vp[vn++] = "fire heavy missiles";
	}
	if (rsf_has(mon_spells, RSF_PMISSL)) {
		if (rf_has(mon_flags, RF_MORGUL_MAGIC))
			vp[vn++] = "hurl black darts";
		else
			vp[vn++] = "whip poisoned darts";
	}

	/* Describe inate attacks */
	if (vn) {
		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0) {
				text_out_to_screen(TERM_WHITE, " may ");
				if (rf_has(mon_flags, RF_ARCHER))
					text_out_to_screen(TERM_WHITE, "frequently ");
			} else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " or ");
			else
				text_out_to_screen(TERM_WHITE, ", or ");

			/* Dump */
			text_out_to_screen(TERM_L_RED, vp[n]);
		}

		/* End */
		text_out_to_screen(TERM_WHITE, ".  ");
	}

	/* Collect breaths */
	vn = 0;
	if (rsf_has(mon_spells, RSF_BRTH_ACID))
		vp[vn++] = "acid";
	if (rsf_has(mon_spells, RSF_BRTH_ELEC))
		vp[vn++] = "lightning";
	if (rsf_has(mon_spells, RSF_BRTH_FIRE))
		vp[vn++] = "fire";
	if (rsf_has(mon_spells, RSF_BRTH_COLD))
		vp[vn++] = "frost";
	if (rsf_has(mon_spells, RSF_BRTH_POIS))
		vp[vn++] = "poison";
	if (rsf_has(mon_spells, RSF_BRTH_PLAS))
		vp[vn++] = "plasma";

	if (rsf_has(mon_spells, RSF_BRTH_LIGHT))
		vp[vn++] = "light";
	if (rsf_has(mon_spells, RSF_BRTH_DARK)) {
		if (rf_has(mon_flags, RF_MORGUL_MAGIC))
			vp[vn++] = "Night";
		else
			vp[vn++] = "darkness";
	}
	if (rsf_has(mon_spells, RSF_BRTH_CONFU))
		vp[vn++] = "confusion";
	if (rsf_has(mon_spells, RSF_BRTH_SOUND))
		vp[vn++] = "sound";
	if (rsf_has(mon_spells, RSF_BRTH_SHARD))
		vp[vn++] = "shards";
	if (rsf_has(mon_spells, RSF_BRTH_INER))
		vp[vn++] = "inertia";
	if (rsf_has(mon_spells, RSF_BRTH_GRAV))
		vp[vn++] = "gravity";
	if (rsf_has(mon_spells, RSF_BRTH_FORCE))
		vp[vn++] = "force";

	if (rsf_has(mon_spells, RSF_BRTH_NEXUS))
		vp[vn++] = "nexus";
	if (rsf_has(mon_spells, RSF_BRTH_NETHR))
		vp[vn++] = "nether";
	if (rsf_has(mon_spells, RSF_BRTH_CHAOS))
		vp[vn++] = "chaos";
	if (rsf_has(mon_spells, RSF_BRTH_DISEN))
		vp[vn++] = "disenchantment";
	if (rsf_has(mon_spells, RSF_BRTH_TIME))
		vp[vn++] = "time";

	if (rsf_has(mon_spells, RSF_BRTH_STORM))
		vp[vn++] = "storm";
	if (rsf_has(mon_spells, RSF_BRTH_DFIRE))
		vp[vn++] = "dragonfire";
	if (rsf_has(mon_spells, RSF_BRTH_ICE))
		vp[vn++] = "ice";
	if (rsf_has(mon_spells, RSF_BRTH_ALL))
		vp[vn++] = "the elements";
	if (rsf_has(mon_spells, RSF_XXX1))
		vp[vn++] = "something";

	/* Describe breaths */
	if (vn) {
		/* Note breath */
		breath = TRUE;

		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0)
				text_out_to_screen(TERM_WHITE, " may breathe ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " or ");
			else
				text_out_to_screen(TERM_WHITE, ", or ");

			/* Dump */
			text_out_to_screen(TERM_L_RED, vp[n]);
		}
		if (rf_has(mon_flags, RF_POWERFUL))
			text_out_to_screen(TERM_WHITE, " powerfully");
	}


	/* Collect spells */
	vn = 0;
	if (rsf_has(mon_spells, RSF_BALL_ACID)) {
		if (spower < 70)
			vp[vn++] = "produce acid balls";
		else
			vp[vn++] = "produce acid storms";
	}
	if (rsf_has(mon_spells, RSF_BALL_ELEC)) {
		if (spower < 70)
			vp[vn++] = "produce lightning balls";
		else
			vp[vn++] = "produce lightning storms";
	}
	if (rsf_has(mon_spells, RSF_BALL_FIRE)) {
		if (rf_has(mon_flags, RF_UDUN_MAGIC)) {
			if (spower < 70)
				vp[vn++] = "produce balls of hellfire";
			else if (spower < 110)
				vp[vn++] = "invoke storms of Udun-fire";
			else
				vp[vn++] = "call upon the fires of Udun";
		} else {
			if (spower < 70)
				vp[vn++] = "produce fire balls";
			else
				vp[vn++] = "produce fire storms";
		}
	}
	if (rsf_has(mon_spells, RSF_BALL_COLD)) {
		if (rf_has(mon_flags, RF_MORGUL_MAGIC)) {
			if (spower < 70)
				vp[vn++] = "produce spheres of deadly cold";
			else
				vp[vn++] = "invoke storms of deadly cold";
		} else {
			if (spower < 70)
				vp[vn++] = "produce frost balls";
			else
				vp[vn++] = "produce frost storms";
		}
	}
	if (rsf_has(mon_spells, RSF_BALL_POIS)) {
		if (rf_has(mon_flags, RF_MORGUL_MAGIC)) {
			if (spower < 15)
				vp[vn++] = "produce clouds of venom";
			else if (spower < 70)
				vp[vn++] = "produce venomous balls";
			else
				vp[vn++] = "raise storms of venom";
		} else {
			if (spower < 15)
				vp[vn++] = "produce stinking clouds";
			else if (spower < 70)
				vp[vn++] = "produce poison balls";
			else
				vp[vn++] = "produce storms of poison";
		}
	}
	if (rsf_has(mon_spells, RSF_BALL_LIGHT)) {
		if (spower < 15)
			vp[vn++] = "produce spheres of light";
		else if (spower < 70)
			vp[vn++] = "produce explosions of light";
		else
			vp[vn++] = "invoke starbursts";
	}
	if (rsf_has(mon_spells, RSF_BALL_DARK)) {
		if (rf_has(mon_flags, RF_MORGUL_MAGIC)) {
			if (spower < 70)
				vp[vn++] = "produce spheres of Night";
			else
				vp[vn++] = "conjure up storms of Night";
		} else {
			if (spower < 70)
				vp[vn++] = "produce balls of darkness";
			else
				vp[vn++] = "produce storms of darkness";
		}
	}
	if (rsf_has(mon_spells, RSF_BALL_CONFU)) {
		if (spower < 70)
			vp[vn++] = "produce confusion balls";
		else
			vp[vn++] = "produce storms of confusion";
	}
	if (rsf_has(mon_spells, RSF_BALL_SOUND)) {
		if (spower < 15)
			vp[vn++] = "produce explosions of sound";
		else if (spower < 70)
			vp[vn++] = "produce thunderclaps";
		else
			vp[vn++] = "unleash storms of sound";
	}
	if (rsf_has(mon_spells, RSF_BALL_SHARD)) {
		if (spower < 15)
			vp[vn++] = "produce blasts of shards";
		else if (spower < 90)
			vp[vn++] = "produce whirlwinds of shards";
		else
			vp[vn++] = "call up shardstorms";
	}
	if (rsf_has(mon_spells, RSF_BALL_STORM)) {
		if (spower < 30)
			vp[vn++] = "produce little storms";
		else if (spower < 70)
			vp[vn++] = "produce whirlpools";
		else
			vp[vn++] = "call up raging storms";
	}
	if (rsf_has(mon_spells, RSF_BALL_NETHR)) {
		if (spower < 30)
			vp[vn++] = "produce nether orbs";
		else if (spower < 70)
			vp[vn++] = "produce nether balls";
		else
			vp[vn++] = "invoke nether storms";
	}
	if (rsf_has(mon_spells, RSF_BALL_CHAOS)) {
		if (spower < 20)
			vp[vn++] = "produce spheres of chaos";
		else if (spower < 70)
			vp[vn++] = "produce explosions of chaos";
		else
			vp[vn++] = "call up maelstroms of raw chaos";
	}
	if (rsf_has(mon_spells, RSF_BALL_MANA)) {
		if (spower < 40)
			vp[vn++] = "produce manabursts";
		else if (spower < 90)
			vp[vn++] = "produce balls of mana";
		else
			vp[vn++] = "invoke mana storms";
	}
	if (rsf_has(mon_spells, RSF_BOLT_ACID))
		vp[vn++] = "produce acid bolts";
	if (rsf_has(mon_spells, RSF_BOLT_ELEC))
		vp[vn++] = "produce lightning bolts";
	if (rsf_has(mon_spells, RSF_BOLT_FIRE))
		vp[vn++] = "produce fire bolts";
	if (rsf_has(mon_spells, RSF_BOLT_COLD))
		vp[vn++] = "produce frost bolts";
	if (rsf_has(mon_spells, RSF_BOLT_POIS))
		vp[vn++] = "produce poison bolts";
	if (rsf_has(mon_spells, RSF_BOLT_PLAS))
		vp[vn++] = "produce plasma bolts";
	if (rsf_has(mon_spells, RSF_BOLT_ICE))
		vp[vn++] = "produce ice bolts";
	if (rsf_has(mon_spells, RSF_BOLT_WATER))
		vp[vn++] = "produce water bolts";
	if (rsf_has(mon_spells, RSF_BOLT_NETHR))
		vp[vn++] = "produce nether bolts";
	if (rsf_has(mon_spells, RSF_BOLT_DARK))
		vp[vn++] = "produce dark bolts";
	if (rsf_has(mon_spells, RSF_BEAM_ELEC))
		vp[vn++] = "shoot sparks of lightning";
	if (rsf_has(mon_spells, RSF_BEAM_ICE))
		vp[vn++] = "cast lances of ice";
	if (rsf_has(mon_spells, RSF_BEAM_NETHR)) {
		if (spower < 40)
			vp[vn++] = "cast beams of nether";
		else if (spower < 90)
			vp[vn++] = "hurl lances of nether";
		else
			vp[vn++] = "shoot rays of death";
	}
	if (rsf_has(mon_spells, RSF_ARC_HFIRE)) {
		if (rf_has(mon_flags, RF_UDUN_MAGIC)) {
			if (spower < 50)
				vp[vn++] = "produce columns of hellfire";
			else if (spower < 100)
				vp[vn++] = "envelop you in hellfire";
			else
				vp[vn++] = "breath like the Balrog";
		} else {
			if (spower < 50)
				vp[vn++] = "produce columns of fire";
			else if (spower < 100)
				vp[vn++] = "envelop you in fire";
			else
				vp[vn++] = "envelop you in flamestrikes";
		}
	}
	if (rsf_has(mon_spells, RSF_ARC_FORCE)) {
		if (spower < 50)
			vp[vn++] = "thrust you away";
		else if (spower < 100)
			vp[vn++] = "hurl you away";
		else
			vp[vn++] = "snatch you up, and throws you away";
	}
	if (rsf_has(mon_spells, RSF_HASTE))
		vp[vn++] = "haste-self";
	if (rsf_has(mon_spells, RSF_ADD_MANA))
		vp[vn++] = "restore mana";
	if (rsf_has(mon_spells, RSF_HEAL))
		vp[vn++] = "heal-self";
	if (rsf_has(mon_spells, RSF_CURE))
		vp[vn++] = "cure what ails it";
	if (rsf_has(mon_spells, RSF_BLINK))
		vp[vn++] = "blink-self";
	if (rsf_has(mon_spells, RSF_TPORT))
		vp[vn++] = "teleport-self";
	if (rsf_has(mon_spells, RSF_TELE_SELF_TO))
		vp[vn++] = "teleport toward you";
	if (rsf_has(mon_spells, RSF_TELE_TO))
		vp[vn++] = "teleport to";
	if (rsf_has(mon_spells, RSF_TELE_AWAY))
		vp[vn++] = "teleport away";
	if (rsf_has(mon_spells, RSF_TELE_LEVEL))
		vp[vn++] = "teleport level";
	if (rsf_has(mon_spells, RSF_DARKNESS))
		vp[vn++] = "create darkness";
	if (rsf_has(mon_spells, RSF_TRAPS))
		vp[vn++] = "create traps";
	if (rsf_has(mon_spells, RSF_FORGET))
		vp[vn++] = "cause amnesia";
	if (rsf_has(mon_spells, RSF_DRAIN_MANA))
		vp[vn++] = "drain mana";
	if (rsf_has(mon_spells, RSF_DISPEL))
		vp[vn++] = "dispel magic";
	if (rsf_has(mon_spells, RSF_MIND_BLAST))
		vp[vn++] = "cause mind blasting";
	if (rsf_has(mon_spells, RSF_BRAIN_SMASH))
		vp[vn++] = "cause brain smashing";
	if (rsf_has(mon_spells, RSF_WOUND)) {
		if (spower < 7)
			vp[vn++] = "cause light wounds";
		else if (spower < 15)
			vp[vn++] = "cause medium wounds";
		else if (spower < 30)
			vp[vn++] = "cause serious wounds";
		else if (spower < 50)
			vp[vn++] = "cause critical wounds";
		else
			vp[vn++] = "cause mortal wounds";
	}
	if (rsf_has(mon_spells, RSF_SHAPECHANGE))
		vp[vn++] = "change shape";
	if (rsf_has(mon_spells, RSF_SCARE))
		vp[vn++] = "terrify";
	if (rsf_has(mon_spells, RSF_BLIND))
		vp[vn++] = "blind";
	if (rsf_has(mon_spells, RSF_CONF))
		vp[vn++] = "confuse";
	if (rsf_has(mon_spells, RSF_SLOW))
		vp[vn++] = "slow";
	if (rsf_has(mon_spells, RSF_HOLD))
		vp[vn++] = "paralyze";

	/* Save current spell. */
	m = vn;

	/* Summons are described somewhat differently. */
	if (flags_test(mon_spells, RSF_SIZE, RSF_SUMMON_MASK, FLAG_END)) {
		/* Summons */
		if (rsf_has(mon_spells, RSF_S_KIN)) {
			if (rf_has(r_ptr->spell_flags, RF_UNIQUE))
				vp[vn++] = "its minions";
			else
				vp[vn++] = "similar monsters";
		}
		if (rsf_has(mon_spells, RSF_S_MONSTER))
			vp[vn++] = "a monster";
		if (rsf_has(mon_spells, RSF_S_MONSTERS))
			vp[vn++] = "monsters";
		if (rsf_has(mon_spells, RSF_S_ANT))
			vp[vn++] = "ants";
		if (rsf_has(mon_spells, RSF_S_SPIDER))
			vp[vn++] = "spiders";
		if (rsf_has(mon_spells, RSF_S_HOUND))
			vp[vn++] = "hounds";
		if (rsf_has(mon_spells, RSF_S_ANIMAL))
			vp[vn++] = "natural creatures";
		if (rsf_has(mon_spells, RSF_S_THIEF))
			vp[vn++] = "thieves";
		if (rsf_has(mon_spells, RSF_S_SWAMP))
			vp[vn++] = "swamp creatures";
		if (rsf_has(mon_spells, RSF_S_DRAGON))
			vp[vn++] = "a dragon";
		if (rsf_has(mon_spells, RSF_S_HI_DRAGON))
			vp[vn++] = "Ancient Dragons";
		if (rsf_has(mon_spells, RSF_S_DEMON))
			vp[vn++] = "a demon";
		if (rsf_has(mon_spells, RSF_S_HI_DEMON))
			vp[vn++] = "Greater Demons";
		if (rsf_has(mon_spells, RSF_S_UNDEAD))
			vp[vn++] = "an undead";
		if (rsf_has(mon_spells, RSF_S_HI_UNDEAD))
			vp[vn++] = "Greater Undead";
		if (rsf_has(mon_spells, RSF_S_QUEST))
			vp[vn++] = "Dungeon Guardians";
		if (rsf_has(mon_spells, RSF_S_UNIQUE))
			vp[vn++] = "Unique Monsters";
	}

	/* Describe spells */
	if (vn) {
		/* Note magic */
		magic = TRUE;

		/* Intro */
		if (breath) {
			text_out_to_screen(TERM_WHITE, ", and is also");
		} else {
			text_out_to_screen(TERM_WHITE, format("%s is", wd_he[msex]));
		}


		/* Verb Phrase */
		text_out_to_screen(TERM_WHITE, " magical, casting");

		/* Describe magic */
		if ((rf_has(mon_flags, RF_UDUN_MAGIC))
			&& (rf_has(mon_flags, RF_MORGUL_MAGIC))) {
			text_out_to_screen(TERM_RED,
							   " perilous spells of Udun and of Morgul");
		} else if (rf_has(mon_flags, RF_MORGUL_MAGIC))
			text_out_to_screen(TERM_RED, " Morgul-spells");
		else if (rf_has(mon_flags, RF_UDUN_MAGIC)) {
			text_out_to_screen(TERM_WHITE, " spells of");
			text_out_to_screen(TERM_RED, " Udun");
		}

		else
			text_out_to_screen(TERM_WHITE, " spells");

		/* Adverb */
		if (rf_has(mon_flags, RF_SMART)) {
			/* Suppress text if monster has both Udun and Morgul-magic. */
			if (!((rf_has(mon_flags, RF_UDUN_MAGIC))
				  && (rf_has(mon_flags, RF_MORGUL_MAGIC)))) {
				text_out_to_screen(TERM_WHITE, " intelligently");
			}
		}

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if ((vn > m) && (n == m)) {
				if (m > 1)
					text_out_to_screen(TERM_WHITE, ",");
				if (m > 0)
					text_out_to_screen(TERM_WHITE, " or");
				text_out_to_screen(TERM_L_RED, " summon ");
			} else if (n == 0)
				text_out_to_screen(TERM_WHITE, " which ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " or ");
			else {
				if ((vn > m) && (m == vn - 2))
					text_out_to_screen(TERM_WHITE, " or ");
				else
					text_out_to_screen(TERM_WHITE, ", or ");
			}

			/* Dump */
			text_out_to_screen(TERM_L_RED, vp[n]);
		}
	}

	/* End the sentence about inate/other spells */
	if (breath || magic) {
		/* Total casting */
		m = l_ptr->cast_spell;

		/* Average frequency */
		n = (r_ptr->freq_ranged);

		/* Guess at the frequency */
		if (m < 100) {
			n = ((n + 9) / 10) * 10;
		}

		/* Describe the spell frequency */
		else if (m) {
			text_out_to_screen(TERM_WHITE, "; ");
			text_out_to_screen(TERM_L_GREEN, "1 ");
			text_out_to_screen(TERM_WHITE, "time in ");
			text_out_to_screen(TERM_L_GREEN, format("%d", 100 / n));
		}

		/* Describe monster mana */
		if (r_ptr->mana && know_mana(r_idx)) {
			/* Mana */
			text_out_to_screen(TERM_WHITE, " with a mana rating of ");
			text_out_to_screen(TERM_L_GREEN, format("%d", r_ptr->mana));

		}

		/* End this sentence */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Describe monster "toughness" */
	if (know_armour(r_idx)) {
		/* Armor */
		text_out_to_screen(TERM_WHITE,
						   format("%s has an armour rating of ",
								  wd_he[msex]));
		text_out_to_screen(TERM_L_GREEN, format("%d", r_ptr->ac));

		/* Maximized hitpoints */
		if (rf_has(mon_flags, RF_FORCE_MAXHP)) {
			text_out_to_screen(TERM_WHITE, " and a life rating of ");
			text_out_to_screen(TERM_L_GREEN,
							   format("%d", r_ptr->hdice * r_ptr->hside));
			text_out_to_screen(TERM_WHITE, ".  ");
		}

		/* Variable hitpoints */
		else {
			text_out_to_screen(TERM_WHITE,
							   " and an average life rating of ");
			text_out_to_screen(TERM_L_GREEN,
							   format("%d",
									  (r_ptr->hdice * (r_ptr->hside + 1)) /
									  2));
			text_out_to_screen(TERM_WHITE, ".  ");
		}
	}


	/* Collect special abilities. */
	vn = 0;
	if (rf_has(mon_flags, RF_OPEN_DOOR))
		vp[vn++] = "open doors";
	if (rf_has(mon_flags, RF_BASH_DOOR))
		vp[vn++] = "bash down doors";
	if (rf_has(mon_flags, RF_PASS_WALL))
		vp[vn++] = "pass through walls";
	if (rf_has(mon_flags, RF_KILL_WALL))
		vp[vn++] = "bore through walls";
	if (rf_has(mon_flags, RF_MOVE_BODY))
		vp[vn++] = "push past other monsters";
	if (rf_has(mon_flags, RF_KILL_BODY))
		vp[vn++] = "destroy weaker monsters";
	if (rf_has(mon_flags, RF_TAKE_ITEM))
		vp[vn++] = "pick up objects";
	if (rf_has(mon_flags, RF_KILL_ITEM))
		vp[vn++] = "destroy objects";

	/* Describe special abilities. */
	if (vn) {
		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0)
				text_out_to_screen(TERM_WHITE, " can ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " and ");
			else
				text_out_to_screen(TERM_WHITE, ", and ");

			/* Dump */
			text_out_to_screen(TERM_L_UMBER, vp[n]);
		}

		/* End */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Describe special abilities. */
	if (rf_has(mon_flags, RF_INVISIBLE)) {
		text_out_to_screen(TERM_L_UMBER,
						   format("%s is invisible.  ", wd_he[msex]));
	}
	if (rf_has(mon_flags, RF_COLD_BLOOD)) {
		text_out_to_screen(TERM_L_UMBER,
						   format("%s is cold blooded.  ", wd_he[msex]));
	}
	if (rf_has(mon_flags, RF_EMPTY_MIND)) {
		text_out_to_screen(TERM_L_UMBER,
						   format("%s is not detected by telepathy.  ",
								  wd_he[msex]));
	}
	if (rf_has(mon_flags, RF_WEIRD_MIND)) {
		text_out_to_screen(TERM_L_UMBER,
						   format("%s is rarely detected by telepathy.  ",
								  wd_he[msex]));
	}
	if (rf_has(mon_flags, RF_MULTIPLY)) {
		text_out_to_screen(TERM_L_UMBER,
						   format("%s breeds explosively.  ",
								  wd_he[msex]));
	}
	if (rf_has(mon_flags, RF_REGENERATE)) {
		text_out_to_screen(TERM_L_UMBER,
						   format("%s regenerates quickly.  ",
								  wd_he[msex]));
	}


	/* Collect susceptibilities */
	vn = 0;
	if (rf_has(mon_flags, RF_HURT_ROCK))
		vp[vn++] = "rock remover";
	if (rf_has(mon_flags, RF_HURT_LIGHT))
		vp[vn++] = "bright light";
	if (rf_has(mon_flags, RF_HURT_FIRE))
		vp[vn++] = "fire";
	if (rf_has(mon_flags, RF_HURT_COLD))
		vp[vn++] = "cold";

	/* Describe susceptibilities */
	if (vn) {
		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0)
				text_out_to_screen(TERM_WHITE, " is hurt by ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " and ");
			else
				text_out_to_screen(TERM_WHITE, ", and ");

			/* Dump */
			text_out_to_screen(TERM_YELLOW, vp[n]);
		}

		/* End */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Collect immunities */
	vn = 0;
	if (rf_has(mon_flags, RF_IM_ACID))
		vp[vn++] = "acid";
	if (rf_has(mon_flags, RF_IM_ELEC))
		vp[vn++] = "lightning";
	if (rf_has(mon_flags, RF_IM_FIRE))
		vp[vn++] = "fire";
	if (rf_has(mon_flags, RF_IM_COLD))
		vp[vn++] = "cold";
	if (rf_has(mon_flags, RF_IM_POIS))
		vp[vn++] = "poison";

	/* Describe immunities */
	if (vn) {
		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0)
				text_out_to_screen(TERM_WHITE, " resists ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " and ");
			else
				text_out_to_screen(TERM_WHITE, ", and ");

			/* Dump */
			text_out_to_screen(TERM_ORANGE, vp[n]);
		}

		/* End */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Collect resistances */
	/* Neatness: mention some borderline high-level resistances only if others 
	 * have already been. */
	vn = 0;
	if (rsf_has(mon_spells, RSF_BRTH_LIGHT))
		vp[vn++] = "light";
	if ((rsf_has(mon_spells, RSF_BRTH_DARK))
		|| (rf_has(mon_flags, RF_MORGUL_MAGIC))
		|| (rf_has(mon_flags, RF_ORC)))
		vp[vn++] = "darkness";

	if (rsf_has(mon_spells, RSF_BRTH_CONFU))
		vp[vn++] = "confusion";
	if (rsf_has(mon_spells, RSF_BRTH_SOUND))
		vp[vn++] = "sound";
	if (rsf_has(mon_spells, RSF_BRTH_SHARD))
		vp[vn++] = "shards";
	if (rsf_has(mon_spells, RSF_BRTH_INER))
		vp[vn++] = "inertia";
	if (rsf_has(mon_spells, RSF_BRTH_GRAV))
		vp[vn++] = "gravity";
	if (rsf_has(mon_spells, RSF_BRTH_FORCE))
		vp[vn++] = "force";
	if ((rf_has(mon_flags, RF_RES_WATE)) || (prefix(name, "Water"))
		|| (rsf_has(mon_spells, RSF_BRTH_STORM)))
		vp[vn++] = "water";

	if ((rsf_has(mon_spells, RSF_BRTH_PLAS))
		|| (rf_has(mon_flags, RF_RES_PLAS))
		|| ((vn)
			&& ((rf_has(mon_flags, RF_IM_ELEC))
				|| (rf_has(mon_flags, RF_IM_FIRE))))
		|| prefix(name, "Plasma"))
		vp[vn++] = "plasma";

	if ((rf_has(mon_flags, RF_RES_NEXUS)) || prefix(name, "Nexus")
		|| (rsf_has(mon_spells, RSF_BRTH_NEXUS)))
		vp[vn++] = "nexus";
	if ((rf_has(mon_flags, RF_UNDEAD)) || (rf_has(mon_flags, RF_RES_NETH))
		|| (rsf_has(mon_spells, RSF_BRTH_NETHR)))
		vp[vn++] = "nether";
	if ((rf_has(mon_flags, RF_RES_DISE))
		|| (rsf_has(mon_spells, RSF_BRTH_DISEN)) || prefix(name, "Disen"))
		vp[vn++] = "disenchantment";
	if (rsf_has(mon_spells, RSF_BRTH_TIME))
		vp[vn++] = "time";


	/* Describe resistances */
	if (vn) {
		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0)
				text_out_to_screen(TERM_WHITE, " resists ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " and ");
			else
				text_out_to_screen(TERM_WHITE, ", and ");

			/* Dump */
			text_out_to_screen(TERM_ORANGE, vp[n]);
		}

		/* End */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Collect non-effects */
	vn = 0;
	if (rf_has(mon_flags, RF_NO_STUN))
		vp[vn++] = "stunned";
	if (rf_has(mon_flags, RF_NO_FEAR))
		vp[vn++] = "frightened";
	if (rf_has(mon_flags, RF_NO_CONF))
		vp[vn++] = "confused";
	if (rf_has(mon_flags, RF_NO_SLEEP))
		vp[vn++] = "slept";

	/* Describe non-effects */
	if (vn) {
		/* Intro */
		text_out_to_screen(TERM_WHITE, format("%s", wd_he[msex]));

		/* Scan */
		for (n = 0; n < vn; n++) {
			/* Intro */
			if (n == 0)
				text_out_to_screen(TERM_WHITE, " cannot be ");
			else if (n < vn - 1)
				text_out_to_screen(TERM_WHITE, ", ");
			else if (n == 1)
				text_out_to_screen(TERM_WHITE, " or ");
			else
				text_out_to_screen(TERM_WHITE, ", or ");

			/* Dump */
			text_out_to_screen(TERM_YELLOW, vp[n]);
		}

		/* End */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Do we know how aware it is? */
	if ((((int) l_ptr->wake * (int) l_ptr->wake) > r_ptr->sleep)
		|| (l_ptr->ignore == MAX_UCHAR) || ((r_ptr->sleep == 0)
											&& (l_ptr->tkills >= 10))) {
		const char *act;

		if (r_ptr->sleep > 150) {
			act = "is nearly oblivious of";
		} else if (r_ptr->sleep > 95) {
			act = "prefers to ignore";
		} else if (r_ptr->sleep > 75) {
			act = "pays little attention to";
		} else if (r_ptr->sleep > 45) {
			act = "tends to overlook";
		} else if (r_ptr->sleep > 25) {
			act = "takes quite a while to see";
		} else if (r_ptr->sleep > 10) {
			act = "takes a while to see";
		} else if (r_ptr->sleep > 5) {
			act = "is fairly observant of";
		} else if (r_ptr->sleep > 3) {
			act = "is observant of";
		} else if (r_ptr->sleep > 1) {
			act = "is very observant of";
		} else if (r_ptr->sleep > 0) {
			act = "is vigilant for";
		} else {
			act = "is ever vigilant for";
		}

		text_out_to_screen(TERM_WHITE,
						   format("%s %s intruders, noticing them from ",
								  wd_he[msex], act, wd_he[msex]));
		text_out_to_screen(TERM_WHITE,
						   format("%d feet.  ",
								  (MODE(SMALL_DEVICE) ? 5 : 10) *
								  r_ptr->aaf));
	}


	/* Drops gold and/or items */
	if (l_ptr->drop_gold || l_ptr->drop_item) {
		/* No "n" needed */
		sin = FALSE;

		/* Intro */
		text_out_to_screen(TERM_WHITE,
						   format("%s may carry", wd_he[msex]));

		/* Count maximum drop */
		n = MAX(l_ptr->drop_gold, l_ptr->drop_item);

		/* One drop (may need an "n") */
		if (n == 1) {
			text_out_to_screen(TERM_WHITE, " a");
			sin = TRUE;
		}

		/* Two drops */
		else if (n == 2) {
			text_out_to_screen(TERM_WHITE, " one or two");
		}

		/* Many drops */
		else {
			text_out_to_screen(TERM_WHITE, format(" up to %d", n));
		}


		/* Great */
		if (rf_has(mon_flags, RF_DROP_GREAT)) {
			p = " exceptional";
		}

		/* Good (no "n" needed) */
		else if (rf_has(mon_flags, RF_DROP_GOOD)) {
			p = " good";
			sin = FALSE;
		}

		/* Okay */
		else {
			p = NULL;
		}


		/* Objects */
		if (l_ptr->drop_item) {
			/* Handle singular "an" */
			if ((sin) && (!(rf_has(r_ptr->flags, RF_DROP_CHEST))))
				text_out_to_screen(TERM_WHITE, "n");
			sin = FALSE;

			/* Dump "object(s)" */
			if (p)
				text_out_to_screen(TERM_WHITE, p);
			if (rf_has(r_ptr->flags, RF_DROP_CHEST))
				text_out_to_screen(TERM_WHITE, " chest");
			else
				text_out_to_screen(TERM_WHITE, " object");
			if (n != 1)
				text_out_to_screen(TERM_WHITE, "s");

			/* Conjunction replaces variety, if needed for "gold" below */
			p = " or";
		}

		/* Treasures */
		if (l_ptr->drop_gold) {
			/* Cancel prefix */
			if (!p)
				sin = FALSE;

			/* Handle singular "an" */
			if (sin)
				text_out_to_screen(TERM_WHITE, "n");
			sin = FALSE;

			/* Dump "treasure(s)" */
			if (p)
				text_out_to_screen(TERM_WHITE, p);
			text_out_to_screen(TERM_WHITE, " treasure");
			if (n != 1)
				text_out_to_screen(TERM_WHITE, "s");
		}

		/* End this sentence */
		text_out_to_screen(TERM_WHITE, ".  ");
	}


	/* Count the number of "known" attacks */
	for (n = 0, m = 0; m < 4; m++) {
		/* Skip non-attacks */
		if (!r_ptr->blow[m].method)
			continue;

		/* Count known attacks */
		if (l_ptr->blows[m])
			n++;
	}

	/* Examine (and count) the actual attacks */
	for (r = 0, m = 0; m < 4; m++) {
		int method, effect, d1, d2;

		/* Skip non-attacks */
		if (!r_ptr->blow[m].method)
			continue;

		/* Skip unknown attacks */
		if (!l_ptr->blows[m])
			continue;


		/* Extract the attack info */
		method = r_ptr->blow[m].method;
		effect = r_ptr->blow[m].effect;
		d1 = r_ptr->blow[m].d_dice;
		d2 = r_ptr->blow[m].d_side;


		/* No method yet */
		p = NULL;

		/* Acquire the method */
		switch (method) {
		case RBM_HIT:
			p = "hit";
			break;
		case RBM_TOUCH:
			p = "touch";
			break;
		case RBM_PUNCH:
			p = "punch";
			break;
		case RBM_KICK:
			p = "kick";
			break;
		case RBM_CLAW:
			p = "claw";
			break;
		case RBM_BITE:
			p = "bite";
			break;
		case RBM_STING:
			p = "sting";
			break;
		case RBM_XXX1:
			break;
		case RBM_BUTT:
			p = "butt";
			break;
		case RBM_CRUSH:
			p = "crush";
			break;
		case RBM_ENGULF:
			p = "engulf";
			break;
		case RBM_XXX2:
			break;
		case RBM_CRAWL:
			p = "crawl on you";
			break;
		case RBM_DROOL:
			p = "drool on you";
			break;
		case RBM_SPIT:
			p = "spit";
			break;
		case RBM_XXX3:
			break;
		case RBM_GAZE:
			p = "gaze";
			break;
		case RBM_WAIL:
			p = "wail";
			break;
		case RBM_SPORE:
			p = "release spores";
			break;
		case RBM_XXX4:
			break;
		case RBM_BEG:
			p = "beg";
			break;
		case RBM_INSULT:
			p = "insult";
			break;
		case RBM_SNEER:
			p = "whinge";
			break;
		case RBM_REQUEST:
			p = "offer to trade";
			break;
		}


		/* Default effect */
		q = NULL;

		/* Acquire the effect */
		switch (effect) {
		case RBE_HURT:
			q = "attack";
			break;
		case RBE_POISON:
			q = "poison";
			break;
		case RBE_UN_BONUS:
			q = "disenchant";
			break;
		case RBE_UN_POWER:
			q = "drain charges";
			break;
		case RBE_EAT_GOLD:
			q = "steal gold";
			break;
		case RBE_EAT_ITEM:
			q = "steal items";
			break;
		case RBE_EAT_FOOD:
			q = "eat your food";
			break;
		case RBE_EAT_LIGHT:
			q = "absorb light";
			break;
		case RBE_ACID:
			q = "shoot acid";
			break;
		case RBE_ELEC:
			q = "electrify";
			break;
		case RBE_FIRE:
			q = "burn";
			break;
		case RBE_COLD:
			q = "freeze";
			break;
		case RBE_BLIND:
			q = "blind";
			break;
		case RBE_CONFUSE:
			q = "confuse";
			break;
		case RBE_TERRIFY:
			q = "terrify";
			break;
		case RBE_PARALYZE:
			q = "paralyze";
			break;
		case RBE_LOSE_STR:
			q = "reduce strength";
			break;
		case RBE_LOSE_INT:
			q = "reduce intelligence";
			break;
		case RBE_LOSE_WIS:
			q = "reduce wisdom";
			break;
		case RBE_LOSE_DEX:
			q = "reduce dexterity";
			break;
		case RBE_LOSE_CON:
			q = "reduce constitution";
			break;
		case RBE_LOSE_CHR:
			q = "reduce charisma";
			break;
		case RBE_LOSE_ALL:
			q = "reduce all stats";
			break;
		case RBE_SHATTER:
			q = "shatter";
			break;
		case RBE_EXP_10:
			q = "lower experience (by 10d6+)";
			break;
		case RBE_EXP_20:
			q = "lower experience (by 20d6+)";
			break;
		case RBE_EXP_40:
			q = "lower experience (by 40d6+)";
			break;
		case RBE_EXP_80:
			q = "lower experience (by 80d6+)";
			break;
		}


		/* Introduce the attack description */
		if (!r) {
			text_out_to_screen(TERM_WHITE, format("%s can ", wd_he[msex]));
		} else if (r < n - 1) {
			text_out_to_screen(TERM_WHITE, ", ");
		} else {
			text_out_to_screen(TERM_WHITE, ", and ");
		}


		/* Hack -- force a method */
		if (!p)
			p = "do something weird";

		/* Describe the method */
		text_out_to_screen(TERM_WHITE, p);


		/* Describe the effect (if any) */
		if (q) {
			/* Describe the attack type */
			text_out_to_screen(TERM_WHITE, " to ");
			text_out_to_screen(TERM_L_RED, q);

			/* Describe damage (if known) */
			if (d1 && d2 && know_damage(r_idx, m)) {
				/* Display the damage */
				text_out_to_screen(TERM_WHITE, " with damage");
				text_out_to_screen(TERM_GREEN, format(" %dd%d", d1, d2));
			}
		}


		/* Count the attacks as printed */
		r++;
	}

	/* Finish sentence above */
	if (r) {
		text_out_to_screen(TERM_WHITE, ".  ");
	}

	/* Notice lack of attacks */
	else if (rf_has(mon_flags, RF_NEVER_BLOW)) {
		text_out_to_screen(TERM_WHITE,
						   format("%s has no physical attacks.  ",
								  wd_he[msex]));
	}

	/* Or describe the lack of knowledge */
	else {
		text_out_to_screen(TERM_WHITE,
						   format("Nothing is known about %s attack.  ",
								  wd_his[msex]));
	}

	/* All done */
	text_out_to_screen(TERM_WHITE, "\n");

	/* Cheat -- know everything */
	if (OPT(cheat_know) || (rf_has(r_ptr->flags, RF_PLAYER_GHOST))) {
		/* Hack -- restore memory */
		COPY(l_ptr, &save_mem, monster_lore);
	}
}




/**
 * Hack -- Display the "name" and "attr/chars" of a monster race
 */
extern void roff_top(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	byte a1, a2;
	wchar_t c1, c2;

	char buf[100];

	/* Access the chars */
	c1 = r_ptr->d_char;
	c2 = r_ptr->x_char;

	/* Access the attrs */
	a1 = r_ptr->d_attr;
	a2 = r_ptr->x_attr;


	strcpy(buf, "");

	/* Clear the top line */
	Term_erase(0, 0, 255);

	/* Reset the cursor */
	Term_gotoxy(0, 0);

	/* A title (use "The" for non-uniques) */
	if (!(rf_has(r_ptr->flags, RF_UNIQUE))) {
		my_strcat(buf, "The ", sizeof(buf));
	}

	/* Special treatment for player ghosts. */
	if (rf_has(r_ptr->flags, RF_PLAYER_GHOST)) {
		my_strcat(buf, format("%s, the ", ghost_name), sizeof(buf));
	}

	/* For all other monsters, dump the racial name. */
	my_strcat(buf, r_ptr->name, sizeof(buf));

	/* Print it */
	Term_addstr(-1, TERM_WHITE, buf);

	if ((tile_width < 3) && (tile_height < 2)) {
		/* Append the "standard" attr/char info */
		Term_addstr(-1, TERM_WHITE, " ('");
		Term_addch(a1, c1);
		Term_addstr(-1, TERM_WHITE, "')");

		/* Append the "optional" attr/char info */
		Term_addstr(-1, TERM_WHITE, "/('");
		Term_addch(a2, c2);
		if ((tile_width == 2) && (a2 & 0x80))
			Term_addch(255, -1);
		Term_addstr(-1, TERM_WHITE, "'):");
	}
}



/**
 * Hack -- describe the given monster race at the top of the screen
 */
void screen_roff(int r_idx)
{
	/* Flush messages */
	message_flush();

	/* Begin recall */
	Term_erase(0, 1, 255);

	/* Recall monster */
	describe_monster(r_idx, FALSE);

	/* Describe monster */
	roff_top(r_idx);
}




/**
 * Hack -- describe the given monster race in the current "term" window
 */
void display_roff(int r_idx)
{
	int y;

	/* Erase the window */
	for (y = 0; y < Term->hgt; y++) {
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* Begin recall */
	Term_gotoxy(0, 1);

	/* Recall monster */
	describe_monster(r_idx, FALSE);

	/* Describe monster */
	roff_top(r_idx);
}


/** 
 * Add various player ghost attributes depending on race. -LM-
 */
static void process_ghost_race(int ghost_race, monster_type * m_ptr)
{
	monster_race *r_ptr = &r_info[PLAYER_GHOST_RACE];
	byte n;

	switch (ghost_race) {
		/* Human */
	case 0:
		{
			/* No differences */
			break;
		}
		/* Green Elf */
	case 1:
		{
			if (r_ptr->freq_ranged)
				r_ptr->freq_ranged += 3;
			r_ptr->aaf += (MODE(SMALL_DEVICE) ? 1 : 2);
			r_ptr->hdice = 6 * r_ptr->hdice / 7;
			break;
			if (rf_has(r_ptr->flags, RF_HURT_LIGHT))
				rf_off(r_ptr->flags, RF_HURT_LIGHT);
			break;
		}
		/* Grey Elf */
	case 2:
		{
			if (r_ptr->freq_ranged)
				r_ptr->freq_ranged += 5;
			r_ptr->aaf += (MODE(SMALL_DEVICE) ? 2 : 4);
			r_ptr->hdice = 4 * r_ptr->hdice / 5;
			if (rf_has(r_ptr->flags, RF_HURT_LIGHT))
				rf_off(r_ptr->flags, RF_HURT_LIGHT);
			break;
		}
		/* Hobbit */
	case 3:
		{
			r_ptr->hdice = 4 * r_ptr->hdice / 5;

			for (n = 1; n < 3; n++) {
				if (r_ptr->blow[n].effect == RBE_HURT) {
					if (n == 1)
						r_ptr->blow[n].effect = RBE_EAT_GOLD;

					if (n == 2)
						r_ptr->blow[n].effect = RBE_EAT_ITEM;

					break;
				}
			}

			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 4;

			rsf_on(r_ptr->spell_flags, RSF_SHOT);

			break;
		}
		/* Petty-Dwarf */
	case 4:
		{
			rsf_on(r_ptr->spell_flags, RSF_BLINK);
			rf_on(r_ptr->flags, RF_NO_SLEEP);
			r_ptr->hdice = 5 * r_ptr->hdice / 6;
			break;
		}
		/* Dwarf */
	case 5:
		{
			r_ptr->hdice = 6 * r_ptr->hdice / 5;
			break;
		}
		/* Druadan */
	case 6:
		{
			/* No difference */
			break;
		}
		/* Longbeard */
	case 7:
		{
			r_ptr->hdice = 6 * r_ptr->hdice / 5;
			break;
		}
		/* Adan */
	case 8:
		{
			r_ptr->ac += r_ptr->level / 10 + 5;

			for (n = 0; n < 4; n++) {
				if ((n == 1) || (n == 3))
					r_ptr->blow[n].d_side = 6 * r_ptr->blow[n].d_side / 5;
			}
			break;
		}
		/* High-Elf */
	case 9:
		{
			r_ptr->ac += r_ptr->level / 10 + 2;

			if (r_ptr->freq_ranged)
				r_ptr->freq_ranged += 8;
			r_ptr->aaf += (MODE(SMALL_DEVICE) ? 2 : 5);
			if (rf_has(r_ptr->flags, RF_HURT_LIGHT))
				rf_off(r_ptr->flags, RF_HURT_LIGHT);
			break;
		}

		/* Maia */
	case 10:
		{
			if (r_ptr->freq_ranged)
				r_ptr->freq_ranged += 5;
			r_ptr->ac += r_ptr->level / 10 + 5;
			r_ptr->aaf += (MODE(SMALL_DEVICE) ? 2 : 4);
			r_ptr->hdice = 5 * r_ptr->hdice / 4;
			break;
		}

		/* Dark Elf */
	case 11:
		{
			if (r_ptr->freq_ranged)
				r_ptr->freq_ranged += 3;
			r_ptr->aaf += (MODE(SMALL_DEVICE) ? 1 : 2);
			rf_on(r_ptr->flags, RF_INVISIBLE);
			r_ptr->hdice = 3 * r_ptr->hdice / 4;
			break;
		}

		/* Ent */
	case 12:
		{
			rf_on(r_ptr->flags, RF_ANIMAL);
			r_ptr->hdice = 4 * r_ptr->hdice / 3;
			if (rf_has(r_ptr->flags, RF_IM_FIRE))
				rf_off(r_ptr->flags, RF_IM_FIRE);
			break;
		}
	}
}

/** 
 * Add various player ghost attributes depending on class. -LM- 
 */
static void process_ghost_class(int ghost_class, monster_type * m_ptr)
{
	monster_race *r_ptr = &r_info[PLAYER_GHOST_RACE];
	int dun_level = r_ptr->level;
	byte n, k;

	/* Note the care taken to make sure that all monsters that get spells can
	 * also cast them, since not all racial templates include spells. */
	switch (ghost_class) {
		/* Warrior */
	case 0:
		{
			if (r_ptr->freq_ranged <= 10)
				r_ptr->freq_ranged = 5;
			else
				r_ptr->freq_ranged -= 5;

			if ((dun_level > 24) && (r_ptr->freq_ranged))
				rsf_on(r_ptr->spell_flags, RSF_BOLT);
			if ((dun_level > 54) && (r_ptr->freq_ranged))
				rf_on(r_ptr->flags, RF_ARCHER);

			r_ptr->spell_power = 2 * r_ptr->spell_power / 3;

			r_ptr->hdice = 4 * r_ptr->hdice / 3;
			r_ptr->ac += r_ptr->level / 10 + 5;

			for (n = 0; n < 4; n++) {
				r_ptr->blow[n].d_side = 3 * r_ptr->blow[n].d_side / 2;
			}

			break;
		}
		/* Mage */
	case 1:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 10;
			else
				r_ptr->freq_ranged += 10;

			if (rsf_has(r_ptr->flags, RSF_SHOT))
				rsf_off(r_ptr->flags, RSF_SHOT);
			if (rsf_has(r_ptr->flags, RSF_ARROW))
				rsf_off(r_ptr->flags, RSF_ARROW);
			if (rsf_has(r_ptr->flags, RSF_BOLT))
				rsf_off(r_ptr->flags, RSF_BOLT);

			rsf_on(r_ptr->spell_flags, RSF_BOLT_FIRE);

			if (dun_level > 11)
				rsf_on(r_ptr->spell_flags, RSF_BALL_POIS);
			if ((dun_level > 13) && (dun_level < 25))
				rsf_on(r_ptr->spell_flags, RSF_BOLT_ELEC);
			if ((dun_level > 16) && (dun_level < 35))
				rsf_on(r_ptr->spell_flags, RSF_BEAM_ICE);
			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_BALL_ELEC);
			if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_BALL_COLD);
			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_BALL_ACID);
			if (dun_level > 64)
				rsf_on(r_ptr->spell_flags, RSF_BALL_MANA);

			if (dun_level > 19)
				rsf_on(r_ptr->spell_flags, RSF_SLOW);
			if (dun_level > 39)
				rsf_on(r_ptr->spell_flags, RSF_HOLD);

			if (dun_level > 19)
				rsf_on(r_ptr->spell_flags, RSF_HASTE);
			if (dun_level > 39)
				m_ptr->mspeed += 5;
			if (m_ptr->mspeed > 130)
				m_ptr->mspeed = 130;

			rsf_on(r_ptr->spell_flags, RSF_BLINK);
			rsf_on(r_ptr->spell_flags, RSF_BLIND);
			if (dun_level > 26)
				rsf_on(r_ptr->spell_flags, RSF_TPORT);
			if (dun_level > 55)
				rsf_on(r_ptr->spell_flags, RSF_TELE_AWAY);

			r_ptr->hdice = 3 * r_ptr->hdice / 4;

			for (n = 0; n < 4; n++) {
				r_ptr->blow[n].d_side = 2 * r_ptr->blow[n].d_side / 3;
			}

			r_ptr->mana = 3 * (r_ptr->mana / 2);

			break;
		}
		/* Priest */
	case 2:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 10;
			else
				r_ptr->freq_ranged += 5;

			rsf_on(r_ptr->spell_flags, RSF_WOUND);
			if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_BALL_LIGHT);
			if (dun_level > 64)
				rsf_on(r_ptr->spell_flags, RSF_BEAM_NETHR);

			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_BLINK);

			rsf_on(r_ptr->spell_flags, RSF_SCARE);
			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_HOLD);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_FORGET);

			rsf_on(r_ptr->spell_flags, RSF_HEAL);
			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_CURE);

			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_S_MONSTER);
			if (dun_level > 49)
				rsf_on(r_ptr->spell_flags, RSF_S_MONSTERS);

			r_ptr->hdice = 4 * r_ptr->hdice / 5;

			r_ptr->mana = 4 * (r_ptr->mana / 3);

			break;
		}
		/* Rogue */
	case 3:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 8;

			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_HASTE);
			if (dun_level > 34)
				m_ptr->mspeed += 5;
			if (m_ptr->mspeed > 130)
				m_ptr->mspeed = 130;

			r_ptr->hdice = 4 * r_ptr->hdice / 5;

			if (dun_level > 11)
				rsf_on(r_ptr->spell_flags, RSF_SHOT);
			if ((dun_level > 44) && (r_ptr->freq_ranged))
				rf_on(r_ptr->flags, RF_ARCHER);

			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_BALL_POIS);

			rsf_on(r_ptr->spell_flags, RSF_DARKNESS);
			if (dun_level > 16)
				rsf_on(r_ptr->spell_flags, RSF_TRAPS);
			if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_TELE_TO);
			if (dun_level > 19)
				rsf_on(r_ptr->spell_flags, RSF_BLINK);
			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_BLIND);

			if (dun_level > 39)
				rf_on(r_ptr->flags, RF_EMPTY_MIND);
			else if (dun_level > 19)
				rf_on(r_ptr->flags, RF_WEIRD_MIND);

			if (dun_level > 39)
				rf_on(r_ptr->flags, RF_INVISIBLE);

			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_S_THIEF);

			if (r_ptr->blow[0].effect == RBE_HURT) {
				r_ptr->blow[0].effect = RBE_EAT_GOLD;
			}
			if (r_ptr->blow[1].effect == RBE_HURT) {
				r_ptr->blow[2].effect = RBE_EAT_ITEM;
			}

			r_ptr->mana = 5 * (r_ptr->mana / 4);

			break;
		}
		/* Ranger */
	case 4:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 8;
			r_ptr->aaf += (MODE(SMALL_DEVICE) ? 2 : 5);

			rsf_on(r_ptr->spell_flags, RSF_ARROW);
			if ((dun_level > 24) && (r_ptr->freq_ranged))
				rf_on(r_ptr->flags, RF_ARCHER);

			rsf_on(r_ptr->spell_flags, RSF_BLINK);
			if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_CURE);
			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_HOLD);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_HEAL);

			r_ptr->mana = 5 * (r_ptr->mana / 4);

			break;
		}
		/* Paladin */
	case 5:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 8;

			if (rsf_has(r_ptr->flags, RSF_SHOT))
				rsf_off(r_ptr->flags, RSF_SHOT);
			if (rsf_has(r_ptr->flags, RSF_ARROW))
				rsf_off(r_ptr->flags, RSF_ARROW);
			if (rsf_has(r_ptr->flags, RSF_BOLT))
				rsf_off(r_ptr->flags, RSF_BOLT);

			if (dun_level > 59)
				rsf_on(r_ptr->spell_flags, RSF_BALL_LIGHT);

			rsf_on(r_ptr->spell_flags, RSF_SHRIEK);
			rsf_on(r_ptr->spell_flags, RSF_SCARE);
			if (dun_level > 5)
				rsf_on(r_ptr->spell_flags, RSF_WOUND);
			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_CURE);
			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_HEAL);
			if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_BLINK);
			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_HOLD);

			rf_on(r_ptr->flags, RF_IM_FIRE);
			rf_on(r_ptr->flags, RF_IM_COLD);
			rf_on(r_ptr->flags, RF_IM_ELEC);
			rf_on(r_ptr->flags, RF_IM_ACID);

			r_ptr->mana = 5 * (r_ptr->mana / 4);

			break;
		}
		/* Druid */
	case 6:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 10;
			else
				r_ptr->freq_ranged += 5;

			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_BALL_ELEC);
			else
				rsf_on(r_ptr->spell_flags, RSF_BEAM_ELEC);

			if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_BALL_COLD);
			else if (dun_level > 9)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_COLD);

			if (dun_level > 39)
				rsf_on(r_ptr->spell_flags, RSF_BALL_FIRE);
			else if (dun_level > 14)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_FIRE);

			if (dun_level > 39)
				rsf_on(r_ptr->spell_flags, RSF_BALL_ACID);
			else if (dun_level > 19)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_ACID);

			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_BALL_POIS);
			else if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_POIS);

			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_PLAS);

			if (dun_level > 49)
				rsf_on(r_ptr->spell_flags, RSF_BALL_STORM);
			else if (dun_level > 34)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_WATER);

			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_BALL_SOUND);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_BALL_SHARD);
			if (dun_level > 59)
				rsf_on(r_ptr->spell_flags, RSF_ARC_FORCE);

			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_CURE);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_HEAL);

			if (dun_level < 40)
				rsf_on(r_ptr->spell_flags, RSF_S_ANT);
			if (dun_level < 40)
				rsf_on(r_ptr->spell_flags, RSF_S_SPIDER);
			if (dun_level > 20)
				rsf_on(r_ptr->spell_flags, RSF_S_ANIMAL);
			if (dun_level > 20)
				rsf_on(r_ptr->spell_flags, RSF_S_HOUND);

			r_ptr->hdice = 4 * r_ptr->hdice / 5;

			r_ptr->mana = 4 * (r_ptr->mana / 3);

			break;
		}
		/* Necromancer */
	case 7:
		{
			if (r_ptr->freq_ranged == 0)
				r_ptr->freq_ranged = 10;
			else
				r_ptr->freq_ranged += 10;

			if (dun_level > 49)
				rf_on(r_ptr->flags, RF_MORGUL_MAGIC);

			rsf_on(r_ptr->spell_flags, RSF_BOLT_DARK);
			rsf_on(r_ptr->spell_flags, RSF_WOUND);
			rsf_on(r_ptr->spell_flags, RSF_DARKNESS);

			if (dun_level > 4)
				rsf_on(r_ptr->spell_flags, RSF_BALL_POIS);
			if (dun_level > 14)
				rsf_on(r_ptr->spell_flags, RSF_BALL_DARK);
			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_BALL_NETHR);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_BALL_MANA);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_ARC_HFIRE);

			if (dun_level > 9)
				rsf_on(r_ptr->spell_flags, RSF_BLINK);
			if (dun_level > 9)
				rsf_on(r_ptr->spell_flags, RSF_BLIND);
			if (dun_level > 9)
				rsf_on(r_ptr->spell_flags, RSF_SCARE);
			if (dun_level > 19)
				rsf_on(r_ptr->spell_flags, RSF_CONF);
			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_TPORT);
			if (dun_level > 39)
				rsf_on(r_ptr->spell_flags, RSF_HOLD);

			if (dun_level > 49)
				rsf_on(r_ptr->spell_flags, RSF_BRAIN_SMASH);
			else if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_MIND_BLAST);

			if (dun_level > 19)
				rsf_on(r_ptr->spell_flags, RSF_S_DEMON);
			if (dun_level > 69)
				rsf_on(r_ptr->spell_flags, RSF_S_HI_DEMON);
			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_S_UNDEAD);
			if (dun_level > 79)
				rsf_on(r_ptr->spell_flags, RSF_S_HI_UNDEAD);

			r_ptr->hdice = 3 * r_ptr->hdice / 4;

			for (n = 0; n < 4; n++) {
				r_ptr->blow[n].d_side = 2 * r_ptr->blow[n].d_side / 3;
			}

			r_ptr->mana = 3 * (r_ptr->mana / 2);

			break;
		}
		/* Assassin */
	case 8:
		{
			if ((dun_level > 9) && (r_ptr->freq_ranged == 0))
				r_ptr->freq_ranged = 6;
			else
				r_ptr->freq_ranged = 3 * r_ptr->freq_ranged / 4;

			if ((dun_level > 11) && (dun_level < 45))
				rsf_on(r_ptr->spell_flags, RSF_MISSL);
			if (dun_level > 44)
				rsf_on(r_ptr->spell_flags, RSF_PMISSL);

			if (dun_level > 49)
				rf_on(r_ptr->flags, RF_ARCHER);
			if (dun_level > 54)
				rf_on(r_ptr->flags, RF_MORGUL_MAGIC);

			if (dun_level > 14)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_POIS);
			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_DARK);
			if (dun_level > 54)
				rsf_on(r_ptr->spell_flags, RSF_BOLT_NETHR);

			if (dun_level > 9)
				rsf_on(r_ptr->spell_flags, RSF_DARKNESS);
			if (dun_level > 14)
				rsf_on(r_ptr->spell_flags, RSF_SCARE);
			if (dun_level > 14)
				rsf_on(r_ptr->spell_flags, RSF_SLOW);
			if (dun_level > 24)
				rsf_on(r_ptr->spell_flags, RSF_BLINK);
			if (dun_level > 29)
				rsf_on(r_ptr->spell_flags, RSF_BLIND);
			if (dun_level > 39)
				rsf_on(r_ptr->spell_flags, RSF_TRAPS);
			if (dun_level > 49)
				rsf_on(r_ptr->spell_flags, RSF_TPORT);
			if (dun_level > 59)
				rsf_on(r_ptr->spell_flags, RSF_HOLD);

			if (dun_level > 34)
				rf_on(r_ptr->flags, RF_WEIRD_MIND);
			if (dun_level > 44)
				rf_on(r_ptr->flags, RF_INVISIBLE);

			if (dun_level > 29) {
				for (n = 0; n < 4; n++) {
					if (r_ptr->blow[n].effect == RBE_HURT) {
						k = n;

						if (k == 0)
							r_ptr->blow[n].effect = RBE_POISON;
						if (k == 1)
							r_ptr->blow[n].effect = RBE_BLIND;
						if (k == 2)
							r_ptr->blow[n].effect = RBE_CONFUSE;
						if (k == 3)
							r_ptr->blow[n].effect = RBE_EAT_ITEM;
					}
				}
			}

			r_ptr->mana = 5 * (r_ptr->mana / 4);

			break;
		}
	}
}

/**
 * Once a monster with the flag "PLAYER_GHOST" is generated, it needs 
 * to have a little color added, if it hasn't been prepared before.  
 * This function uses a bones file to get a name, give the ghost a 
 * gender, and add flags depending on the race and class of the slain 
 * adventurer.  -LM-
 */
bool prepare_ghost(int r_idx, monster_type * m_ptr, bool from_savefile)
{
	int ghost_sex, ghost_race, ghost_class = 0;
	byte try, i, backup_file_selector;

	monster_race *r_ptr = &r_info[r_idx];
	monster_lore *l_ptr = &l_list[PLAYER_GHOST_RACE];

	ang_file *fp;
	char path[1024];
	char buf[80];

	/* Paranoia. */
	if (!(rf_has(r_ptr->flags, RF_PLAYER_GHOST)))
		return (TRUE);

	/* Hack -- No easy player ghosts, unless the ghost is from a savefile.
	 * This also makes player ghosts much rarer, and effectively (almost)
	 * prevents more than one from being on a level. From 0.5.1, other code
	 * makes it is formally impossible to have more than one ghost at a time.
	 * -BR- */

	if ((r_ptr->level < p_ptr->depth - 5) && (from_savefile == FALSE))
		return (FALSE);

	/* Store the index of the base race. */
	r_ghost = r_idx;

	/* Copy the info from the template to the special "ghost slot", and use
	 * that from here on */
	memcpy(&r_info[PLAYER_GHOST_RACE], r_ptr, sizeof(*r_ptr));
	r_ptr = &r_info[PLAYER_GHOST_RACE];

	/* Choose a bones file.  Use the variable bones_selector if it has any
	 * information in it (this allows saved ghosts to reacquire all special
	 * features), then use the current depth, and finally pick at random. */
	for (try = 0; try < 40; ++try) {
		/* Prepare a path, and store the file number for future reference. */
		if (try == 0) {
			if (bones_selector) {
				sprintf(path, "%s/bone.%03d", ANGBAND_DIR_BONE,
						bones_selector);
			} else {
				sprintf(path, "%s/bone.%03d", ANGBAND_DIR_BONE,
						p_ptr->depth);
				bones_selector = (byte) p_ptr->depth;
			}
		} else {
			backup_file_selector = randint1(MAX_DEPTH - 1);
			sprintf(path, "%s/bone.%03d", ANGBAND_DIR_BONE,
					backup_file_selector);
			bones_selector = backup_file_selector;
		}

		/* Attempt to open the bones file. */
		fp = file_open(path, MODE_READ, FTYPE_TEXT);

		/* No bones file with that number, try again. */
		if (!fp) {
			bones_selector = 0;
			continue;
		}

		/* Success. */
		if (fp)
			break;
	}

	/* No bones file found, so no Ghost is made. */
	if (!fp)
		return (FALSE);


	/* Scan the file (new style) */

	/* Name */
	if (!file_getl(fp, buf, sizeof(buf)))
		return FALSE;
	my_strcpy(ghost_name, buf, 15);

	/* Sex */
	if (!file_getl(fp, buf, sizeof(buf)))
		return FALSE;
	if (1 != sscanf(buf, "%d", &ghost_sex))
		return FALSE;

	/* Race */
	if (!file_getl(fp, buf, sizeof(buf)))
		return FALSE;
	if (1 != sscanf(buf, "%d", &ghost_race))
		return FALSE;

	/* Class */
	if (!file_getl(fp, buf, sizeof(buf)))
		return FALSE;
	if (1 != sscanf(buf, "%d", &ghost_class))
		return FALSE;

	/* String type (maybe) */
	if (file_getl(fp, buf, sizeof(buf))) {
		if (1 != sscanf(buf, "%d", &ghost_string_type))
			ghost_string_type = 0;
	}

	/* String (maybe) */
	if (file_getl(fp, buf, sizeof(buf))) {
		if (strlen(buf) > 0)
			my_strcpy(ghost_string, buf, strlen(buf));
		else
			ghost_string_type = 0;
	}

	/* Close the file */
	file_close(fp);

  /*** Process the ghost name and store it in a global variable. ***/

	/* XXX XXX XXX Find the first comma, or end of string */
	for (i = 0; (i < 16) && (ghost_name[i]) && (ghost_name[i] != ',');
		 i++);

	/* Terminate the name */
	ghost_name[i] = '\0';

	/* Force a name */
	if (!ghost_name[0])
		strcpy(ghost_name, "Nobody");

	/* Capitalize the name */
	if (islower(ghost_name[0]))
		ghost_name[0] = toupper(ghost_name[0]);


  /*** Process sex. ***/

	/* Sanity check. */
	if ((ghost_sex >= MAX_SEXES) || (ghost_class < 0))
		ghost_sex = randint0(MAX_SEXES);

	/* And use that number to toggle on either the male or the female flag. */
	if (ghost_sex == 0)
		rf_on(r_ptr->flags, RF_FEMALE);
	if (ghost_sex == 1)
		rf_on(r_ptr->flags, RF_MALE);


  /*** Process race. ***/

	/* Sanity check. */
	if (ghost_race >= z_info->p_max)
		ghost_race = randint0(z_info->p_max);

	/* And use the ghost race to gain some flags. */
	process_ghost_race(ghost_race, m_ptr);


  /*** Process class. ***/

	/* Sanity check. */
	if (ghost_class >= CLASS_MAX)
		ghost_class = randint0(CLASS_MAX);

	/* And use the ghost class to gain some flags. */
	process_ghost_class(ghost_class, m_ptr);

	/* Hack - a little extra help for the deepest ghosts */
	if (p_ptr->depth > 75)
		r_ptr->spell_power += 3 * (p_ptr->depth - 75) / 2;

	/* Hack -- increase the level feeling */
	rating += 10;

	/* Hack - Player ghosts are "seen" whenever generated, to conform with
	 * previous practice. */
	l_ptr->sights = 1;

	/* Success */
	return (TRUE);
}
