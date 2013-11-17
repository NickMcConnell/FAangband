/** \file gen-monster.c 
    \brief Dungeon monster generation
 
 * Code for selecting appropriate monsters for levels when generated.  
 *
 * Copyright (c) 2011
 * Nick McConnell, Leon Marrick, Ben Harrison, James E. Wilson, 
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
#include "generate.h"
#include "monster.h"

/**
 * This table takes a depth, and returns a suitable monster symbol.  Depth 
 * input is assumed to be player depth.  It is also assumed that monsters 
 * can be generated slightly out of depth.  -LM-
 *
 * Depths greater than 60 should map to the row for level 60.
 *
 * Monster pits use the first seven columns, and rooms of chambers use all  
 * thirteen.  Because rooms of chambers are about half as common as monster 
 * pits, and have fewer monsters, creatures in the first seven columns will 
 * be much more common than those in the last six.  On the other hand, 
 * monsters in rooms of chambers have a lot more maneuver room...
 *
 * - Symbol '*' is any creature of a randomly-chosen racial char.
 * - Symbol 'A' is any animal.
 *
 * - Symbol '1' is any insect ('a', 'c', 'l', 'F', 'I', 'K').
 * - Symbol '2' is any naga, snake, or other reptile.
 * - Symbol '3' is any jelly, mold, icky thing ('i', 'j', or 'm').
 *
 * - Symbol '%' is any orc, ogre, troll, or giant.
 * - Symbol 'N' is any undead.  Upon occasion, deep in the dungeon, the 
 *   racial type may be forced to 'G', 'L', 'V', or 'W'.
 * - Symbols 'p' and 'h' may sometimes be found in combination.  If they 
 *   are, they are usually all of a given class (magical, pious, natural, 
 *   assassination/thievery, or warrior)
 * - Symbols 'E' and 'v' may be found in combination.  If they are, they 
 *   will always be of a given elemental type.
 * - Symbols 'd' and 'D' both mean dragons of either char.  Dragons are 
 *   often of a particular type (blue, red, gold, shadow/ethereal etc.).
 * - Symbols 'u' and 'U' may mean lesser demons, greater demons, or both, 
 *   depending on depth. 
 * 
 * - Other symbols usually represent specific racial characters.
 */
char mon_symbol_at_depth[12][13] = {
	/* Levels 5, 10, 15, and 20 */
	{'A', 'A', '3', '3', '3', 'k', 'y', '*', '*', '1', 'a', '2', 'S'},
	{'A', '3', '3', 'o', 'o', 'N', '1', '*', 'C', 'f', 'a', '2', 'S'},
	{'A', '3', 'o', 'o', 'o', 'u', '*', '#', '#', 'S', 'E', '2', 'Z'},
	{'A', '3', '3', 'o', 'T', 'T', '#', 'p', 'h', 'f', '1', '*', 'Z'},

	/* Levels 25, 30, 35, and 40 */
	{'A', '3', 'T', 'T', 'u', 'P', 'P', 'p', 'v', 'd', '1', 'S', '2'},
	{'A', 'A', 'T', 'P', 'P', 'N', 'd', 'p', 'h', 'f', 'v', 'g', 'Z'},
	{'A', '3', 'T', 'P', 'N', 'u', 'd', 'p', 'H', 'E', '1', '*', '2'},
	{'A', 'A', 'T', 'P', 'N', 'u', 'd', 'p', 'h', 'g', 'E', '*', 'Z'},

	/* Levels 45, 50, 55, and 60 */
	{'A', 'P', 'N', 'u', 'd', 'd', '*', 'p', 'h', 'v', 'E', '*', 'Z'},
	{'N', 'N', 'U', 'U', 'D', 'D', '*', 'p', 'h', 'v', 'T', 'B', 'Z'},
	{'A', 'N', 'U', 'U', 'D', 'D', '*', 'p', 'h', 'W', 'G', '*', 'Z'},
	{'N', 'N', 'U', 'U', 'D', 'D', '*', 'p', 'h', 'v', '*', 'D', 'Z'}
};

/**
 * Restrictions on monsters, used in pits, vaults, and chambers.
 */
static bool allow_unique;
static char d_char_req[15];
static byte d_attr_req[4];
static u32b racial_flag_mask;
static bitflag breath_flag_mask[RSF_SIZE];


/**
 * Table of monster descriptions.  Used to make descriptions for kinds 
 * of pits and rooms of chambers that have no special names.
 */
const char *d_char_req_desc[] = {
	"B:bird",
	"C:canine",
	"D:dragon",
	"E:elemental",
	"F:dragon fly",
	"G:ghost",
	"H:hybrid",
	"I:insect",
	"J:snake",
	"K:killer beetle",
	"L:lich",
	"M:mummy",
	"O:ogre",
	"P:giant",
	"Q:quylthulg",
	"R:reptile",
	"S:spider",
	"T:troll",
	"U:demon",
	"V:vampire",
	"W:wraith",
	"Y:yeti",
	"Z:zephyr hound",
	"a:ant",
	"b:bat",
	"c:centipede",
	"d:dragon",
	"e:floating eye",
	"f:feline",
	"g:golem",
	"h:humanoid",
	"i:icky thing",
	"j:jelly",
	"k:kobold",
	"l:louse",
	"m:mold",
	"n:naga",
	"o:orc",
	"p:human",
	"q:quadruped",
	"r:rodent",
	"s:skeleton",
	"t:townsperson",
	"u:demon",
	"v:vortex",
	"w:worm",
	"y:yeek",
	"z:zombie",
	",:mushroom patch",
	NULL
};

/**************************************************************/
/*                                                            */
/*                 The monster-selection code                 */
/*                                                            */
/**************************************************************/


/**
 * Hack - select beings of the four basic elements.  Used in the elemental 
 * war themed level.
 */
static bool vault_aux_elemental(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];


	/* Demons are always welcome. */
	if (r_ptr->d_char == 'U' || r_ptr->d_char == 'u')
		return (TRUE);

	/* Certain names are a givaway. */
	if (strstr(r_ptr->name, "Fire")
		|| strstr(r_ptr->name, "Hell")
		|| strstr(r_ptr->name, "Frost")
		|| strstr(r_ptr->name, "Cold")
		|| strstr(r_ptr->name, "Acid")
		|| strstr(r_ptr->name, "Water")
		|| strstr(r_ptr->name, "Energy"))
		return (TRUE);

	/* Otherwise, try selecting by breath attacks. */
	if (rf_has(r_ptr->spell_flags, RSF_BRTH_ACID)
		|| rf_has(r_ptr->spell_flags, RSF_BRTH_ELEC)
		|| rf_has(r_ptr->spell_flags, RSF_BRTH_FIRE)
		|| rf_has(r_ptr->spell_flags, RSF_BRTH_COLD))
		return (TRUE);

	/* Nope */
	return (FALSE);
}


/**
 * Use various selection criteria (set elsewhere) to restrict monster 
 * generation.
 *
 * This function is capable of selecting monsters by:
 *   - racial symbol (may be any of the characters allowed)
 *   - symbol color (may be any of up to four colors).
 *   - racial flag(s) (monster may have any allowed flag)
 *   - breath flag(s) (monster must have exactly the flags specified)
 *
 * Uniques may be forbidden, or allowed on rare occasions.
 *
 * Some situations (like the elemental war themed level) require special 
 * processing; this is done in helper functions called from this one.
 */
static bool mon_select(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	bool ok = FALSE;
	bitflag mon_breath[RSF_SIZE];


	/* Require correct breath attack */
	rsf_copy(mon_breath, r_ptr->spell_flags);
	flags_mask(mon_breath, RSF_SIZE, RSF_BREATH_MASK, FLAG_END);

	/* Special case: Elemental war themed level. */
	if (p_ptr->themed_level == THEME_ELEMENTAL) {
		return (vault_aux_elemental(r_idx));
	}

	/* Special case: Estolad themed level. */
	if (p_ptr->themed_level == THEME_ESTOLAD) {
		if (!(rf_has(r_ptr->flags, RF_RACIAL)))
			return (FALSE);
	}


	/* Require that the monster symbol be correct. */
	if (d_char_req[0] != '\0') {
		if (strchr(d_char_req, r_ptr->d_char) == 0)
			return (FALSE);
	}

	/* Require correct racial type. */
	if (racial_flag_mask) {
		if (!(rf_has(r_ptr->flags, racial_flag_mask)))
			return (FALSE);

		/* Hack -- no invisible undead until deep. */
		if ((p_ptr->depth < 40) && (rf_has(r_ptr->flags, RF_UNDEAD))
			&& (rf_has(r_ptr->flags, RF_INVISIBLE)))
			return (FALSE);
	}

	/* Require that monster breaths be exactly those specified. */
	/* Exception for winged dragons */
	if (!rsf_is_empty(breath_flag_mask)) {
		/* 'd'ragons */
		if (!rsf_is_equal(mon_breath, breath_flag_mask)
			&& (r_ptr->d_char != 'D'))
			return (FALSE);

		/* Winged 'D'ragons */
		if (r_ptr->d_char == 'D') {
			if (!rsf_is_subset(mon_breath, breath_flag_mask))
				return (FALSE);

			/* Hack - this deals with all current Ds that need excluding */
			if (rsf_has(r_ptr->flags, RSF_BRTH_SOUND))
				return (FALSE);
		}
	}

	/* Require that the monster color be correct. */
	if (d_attr_req[0]) {
		/* Check all allowed colors, if given. */
		if ((d_attr_req[0]) && (r_ptr->d_attr == d_attr_req[0]))
			ok = TRUE;
		if ((d_attr_req[1]) && (r_ptr->d_attr == d_attr_req[1]))
			ok = TRUE;
		if ((d_attr_req[2]) && (r_ptr->d_attr == d_attr_req[2]))
			ok = TRUE;
		if ((d_attr_req[3]) && (r_ptr->d_attr == d_attr_req[3]))
			ok = TRUE;

		/* Hack -- No multihued dragons allowed in the arcane dragon pit. */
		if ((strchr(d_char_req, 'd') || strchr(d_char_req, 'D'))
			&& (d_attr_req[0] == TERM_VIOLET)
			&& (rf_has(r_ptr->flags, RSF_BRTH_ACID)
				|| rf_has(r_ptr->flags, RSF_BRTH_ELEC)
				|| rf_has(r_ptr->flags, RSF_BRTH_FIRE)
				|| rf_has(r_ptr->flags, RSF_BRTH_COLD)
				|| rf_has(r_ptr->flags, RSF_BRTH_POIS))) {
			return (FALSE);
		}

		/* Doesn't match any of the given colors? Not good. */
		if (!ok)
			return (FALSE);
	}

	/* Usually decline unique monsters. */
	if (rf_has(r_ptr->flags, RF_UNIQUE)) {
		if (!allow_unique)
			return (FALSE);
		else if (randint0(5) != 0)
			return (FALSE);
	}

	/* Okay */
	return (TRUE);
}

/**
 * Accept characters representing a race or group of monsters and 
 * an (adjusted) depth, and use these to set values for required racial 
 * type, monster symbol, monster symbol color, and breath type.  -LM-
 *
 * This function is called to set restrictions, point the monster 
 * allocation function to mon_select(), and remake monster allocation.  
 * It undoes all of this things when called with the symbol '\0'.
 * 
 * Describe the monsters (used by cheat_room) and determine if they 
 * should be neatly ordered or randomly placed (used in monster pits).
 */
extern char *mon_restrict(char symbol, byte depth, bool * ordered,
						  bool unique_ok)
{
	int i, j;

	/* Assume no definite name */
	char name[80] = "misc";

	/* Clear global monster restriction variables. */
	allow_unique = unique_ok;
	for (i = 0; i < 10; i++)
		d_char_req[i] = '\0';
	for (i = 0; i < 4; i++)
		d_attr_req[i] = 0;
	racial_flag_mask = 0;
	rsf_wipe(breath_flag_mask);


	/* No symbol, no restrictions. */
	if (symbol == '\0') {
		get_mon_num_hook = NULL;
		get_mon_num_prep();
		return ("misc");
	}

	/* Handle the "wild card" symbol '*' */
	if (symbol == '*') {
		for (i = 0; i < 2500; i++) {
			/* Get a random monster. */
			j = randint1(z_info->r_max - 1);

			/* Must be a real monster */
			if (!r_info[j].rarity)
				continue;

			/* Try for close to depth, accept in-depth if necessary */
			if (i < 200) {
				if ((!rf_has(r_info[j].flags, RF_UNIQUE))
					&& (r_info[j].level != 0) && (r_info[j].level <= depth)
					&& (ABS(r_info[j].level - p_ptr->depth) <
						1 + (p_ptr->depth / 4)))
					break;
			} else {
				if ((!rf_has(r_info[j].flags, RF_UNIQUE))
					&& (r_info[j].level != 0)
					&& (r_info[j].level <= depth))
					break;
			}
		}

		/* We've found a monster. */
		if (i < 2499) {
			/* ...use that monster's symbol for all monsters. */
			symbol = r_info[j].d_char;
			strcpy(name, r_info[j].base->name);
		} else {
			/* Paranoia - pit stays empty if no monster is found */
			return (NULL);
		}
	}

	/* Apply monster restrictions according to symbol. */
	switch (symbol) {
		/* All animals */
	case 'A':
		{
			strcpy(name, "animal");
			racial_flag_mask = RF_ANIMAL;
			*ordered = FALSE;
			break;
		}

		/* Insects */
	case '1':
		{
			strcpy(name, "insect");
			strcpy(d_char_req, "aclFIK");
			*ordered = FALSE;
			break;
		}

		/* Reptiles */
	case '2':
		{
			strcpy(name, "reptile");
			strcpy(d_char_req, "nJR");
			*ordered = FALSE;
			break;
		}

		/* Jellies, etc. */
	case '3':
		{
			strcpy(name, "jelly");
			strcpy(d_char_req, "ijm,");
			*ordered = FALSE;
			break;
		}

		/* Humans and humaniods */
	case 'p':
	case 'h':
		{
			/* 'p's and 'h's can coexist. */
			if (randint0(3) == 0) {
				strcpy(d_char_req, "ph");

				/* If so, they will usually all be of similar classes. */
				if (randint0(4) != 0) {
					/* Randomizer. */
					i = randint0(5);

					/* Magicians and necromancers */
					if (i == 0) {
						d_attr_req[0] = TERM_RED;
						d_attr_req[1] = TERM_L_RED;
						d_attr_req[2] = TERM_VIOLET;
						strcpy(name, "school of sorcery");
					}
					/* Priests and paladins */
					else if (i == 1) {
						d_attr_req[0] = TERM_GREEN;
						d_attr_req[1] = TERM_L_GREEN;
						d_attr_req[2] = TERM_WHITE;
						d_attr_req[3] = TERM_L_WHITE;
						strcpy(name, "temple of piety");
					}
					/* Druids and ninjas */
					else if (i == 2) {
						d_attr_req[0] = TERM_ORANGE;
						d_attr_req[1] = TERM_YELLOW;
						strcpy(name, "gathering of nature");
					}
					/* Thieves and assassins */
					else if (i == 3) {
						d_attr_req[0] = TERM_BLUE;
						d_attr_req[1] = TERM_L_BLUE;
						d_attr_req[2] = TERM_SLATE;
						d_attr_req[3] = TERM_L_DARK;
						strcpy(name, "den of thieves");
					}
					/* Warriors and rangers */
					else {
						d_attr_req[0] = TERM_UMBER;
						d_attr_req[1] = TERM_L_UMBER;
						strcpy(name, "fighter's hall");
					}
				} else {
					strcpy(name, "humans and humanoids");
				}
			}

			/* Usually, just accept the symbol. */
			else {
				d_char_req[0] = symbol;

				if (symbol == 'p')
					strcpy(name, "human");
				else if (symbol == 'h')
					strcpy(name, "humanoid");
			}

			*ordered = FALSE;
			break;
		}

		/* Orcs */
	case 'o':
		{
			strcpy(name, "orc");
			strcpy(d_char_req, "o");
			*ordered = TRUE;
			break;
		}

		/* Trolls */
	case 'T':
		{
			strcpy(name, "troll");
			strcpy(d_char_req, "T");
			*ordered = TRUE;
			break;
		}

		/* Giants (sometimes ogres at low levels) */
	case 'P':
		{
			strcpy(name, "giant");
			if ((p_ptr->depth < 30) && (randint0(3) == 0))
				strcpy(d_char_req, "O");
			else
				strcpy(d_char_req, "P");
			*ordered = TRUE;
			break;
		}

		/* Orcs, ogres, trolls, or giants */
	case '%':
		{
			strcpy(name, "moria");
			strcpy(d_char_req, "oOPT");
			*ordered = FALSE;
			break;
		}

		/* Monsters found in caves */
	case '0':
		{
			strcpy(name, "dungeon monsters");
			strcpy(d_char_req, "ykoOT");
			*ordered = FALSE;
			break;
		}

		/* Monsters found in wilderness caves */
	case 'x':
		{
			strcpy(name, "underworld monsters");
			strcpy(d_char_req, "bgkosuyOTUVXW");
			*ordered = FALSE;
			break;
		}


		/* Undead */
	case 'N':
		{
			/* Sometimes, restrict by symbol. */
			if ((depth > 40) && (randint0(3) == 0)) {
				for (i = 0; i < 500; i++) {
					/* Find a suitable monster near depth. */
					j = randint1(z_info->r_max - 1);

					/* Require a non-unique undead. */
					if (rf_has(r_info[j].flags, RF_UNDEAD)
						&& (!rf_has(r_info[j].flags, RF_UNIQUE))
						&& (strchr("GLWV", r_info[j].d_char))
						&& (ABS(r_info[j].level - p_ptr->depth) <
							1 + (p_ptr->depth / 4))) {
						break;
					}
				}

				/* If we find a monster, */
				if (i < 499) {
					/* Use that monster's symbol for all monsters */
					d_char_req[0] = r_info[j].d_char;

					/* No pit name (yet) */

					/* In this case, we do order the monsters */
					*ordered = TRUE;
				} else {
					/* Accept any undead. */
					strcpy(name, "undead");
					racial_flag_mask = RF_UNDEAD;
					*ordered = FALSE;
				}
			} else {
				/* No restrictions on symbol. */
				strcpy(name, "undead");
				racial_flag_mask = RF_UNDEAD;
				*ordered = FALSE;
			}
			break;
		}

		/* Demons */
	case 'u':
	case 'U':
		{
			strcpy(name, "demon");
			if (depth > 55)
				strcpy(d_char_req, "U");
			else if (depth < 40)
				strcpy(d_char_req, "u");
			else
				strcpy(d_char_req, "uU");
			*ordered = TRUE;
			break;
		}

		/* Dragons */
	case 'd':
	case 'D':
		{
			strcpy(d_char_req, "dD");

			/* Dragons usually associate with others of their kind. */
			if (randint0(6) != 0) {
				/* Dragons of a single kind are ordered. */
				*ordered = TRUE;

				/* Some dragon types are not found everywhere */
				if (depth > 70)
					i = randint0(35);
				else if (depth > 45)
					i = randint0(32);
				else if (depth > 32)
					i = randint0(30);
				else if (depth > 23)
					i = randint0(28);
				else
					i = randint0(24);

				if (i < 4) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_ACID,
							   FLAG_END);
					strcpy(name, "dragon - acid");
				} else if (i < 8) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_ELEC,
							   FLAG_END);
					strcpy(name, "dragon - electricity");
				} else if (i < 12) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_FIRE,
							   FLAG_END);
					strcpy(name, "dragon - fire");
				} else if (i < 16) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_COLD,
							   FLAG_END);
					strcpy(name, "dragon - cold");
				} else if (i < 20) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_POIS,
							   FLAG_END);
					strcpy(name, "dragon - poison");
				} else if (i < 24) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_ACID,
							   RSF_BRTH_ELEC, RSF_BRTH_FIRE, RSF_BRTH_COLD,
							   RSF_BRTH_POIS, FLAG_END);
					strcpy(name, "dragon - multihued");
				} else if (i < 26) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_CONFU,
							   FLAG_END);
					strcpy(name, "dragon - confusion");
				} else if (i < 28) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_SOUND,
							   FLAG_END);
					strcpy(name, "dragon - sound");
				} else if (i < 30) {
					flags_init(breath_flag_mask, RSF_SIZE, RSF_BRTH_LIGHT,
							   RSF_BRTH_DARK, FLAG_END);
					strcpy(name, "dragon - ethereal");
				}

				/* Chaos, Law, Balance, Power, etc.) */
				else {
					d_attr_req[0] = TERM_VIOLET;
					d_attr_req[1] = TERM_L_BLUE;
					d_attr_req[2] = TERM_L_GREEN;
					strcpy(name, "dragon - arcane");
				}
			} else {
				strcpy(name, "dragon - mixed");

				/* Dragons of all kinds are not ordered. */
				*ordered = FALSE;
			}
			break;
		}

		/* Vortexes and elementals */
	case 'v':
	case 'E':
		{
			/* Usually, just have any kind of 'v' or 'E' */
			if (randint0(3) != 0) {
				d_char_req[0] = symbol;

				if (symbol == 'v')
					strcpy(name, "vortex");
				if (symbol == 'E')
					strcpy(name, "elemental");
			}

			/* Sometimes, choose both 'v' and 'E's of one element */
			else {
				strcpy(d_char_req, "vE");

				i = randint0(4);

				/* Fire */
				if (i == 0) {
					d_attr_req[0] = TERM_RED;
					strcpy(name, "fire");
				}
				/* Frost */
				if (i == 1) {
					d_attr_req[0] = TERM_L_WHITE;
					d_attr_req[1] = TERM_WHITE;
					strcpy(name, "frost");
				}
				/* Air/electricity */
				if (i == 2) {
					d_attr_req[0] = TERM_L_BLUE;
					d_attr_req[1] = TERM_BLUE;
					strcpy(name, "air");
				}
				/* Acid/water/earth */
				if (i == 3) {
					d_attr_req[0] = TERM_GREEN;
					d_attr_req[1] = TERM_L_UMBER;
					d_attr_req[2] = TERM_UMBER;
					d_attr_req[3] = TERM_SLATE;
					strcpy(name, "earth & water");
				}
			}

			*ordered = FALSE;
			break;
		}

		/* Special case: mimics and treasure */
	case '!':
	case '?':
	case '=':
	case '~':
	case '|':
	case '.':
	case '$':
		{
			if (symbol == '$') {
				strcpy(name, "treasure");

				/* Nothing but loot! */
				if (randint0(3) == 0)
					strcpy(d_char_req, "$");

				/* Guard the money well. */
				else
					strcpy(d_char_req, "$!?=~|.");
			} else {
				/* No treasure. */
				strcpy(d_char_req, "!?=~|.");
				strcpy(name, "mimic");
			}

			*ordered = FALSE;
			break;
		}

		/* Special case: creatures of earth. */
	case 'X':
	case '#':
		{
			strcpy(d_char_req, "X#");
			strcpy(name, "creatures of earth");
			*ordered = FALSE;
			break;
		}

		/* Water creatures. */
	case '6':
		{
			allow_unique = TRUE;
			strcpy(d_char_req, "vEZ");
			d_attr_req[0] = TERM_SLATE;
			break;
		}

		/* Beings of fire or ice. */
	case '7':
		{
			allow_unique = TRUE;
			strcpy(d_char_req, "vE");
			if (randint0(2) == 0)
				d_attr_req[0] = TERM_RED;
			else {
				d_attr_req[0] = TERM_L_WHITE;
				d_attr_req[1] = TERM_WHITE;
			}

			break;
		}

		/* Space for more monster types here. */


		/* Any symbol not handled elsewhere. */
	default:
		{
			/* Accept the character. */
			d_char_req[0] = symbol;

			/* Some monsters should logically be ordered. */
			if (strchr("knosuyzGLMOPTUVW", symbol))
				*ordered = TRUE;

			/* Most should not */
			else
				*ordered = FALSE;

			break;
		}
	}

	/* Apply our restrictions */
	get_mon_num_hook = mon_select;

	/* Prepare allocation table */
	get_mon_num_prep();

	/* Return the name. */
	return (format("%s", name));
}


extern void get_chamber_monsters(int y1, int x1, int y2, int x2)
{
	bool dummy;
	int i, y, x;
	s16b monsters_left, depth;
	char symbol;

	/* Description of monsters in room */
	char *name;

	/* Get a legal depth. */
	depth = p_ptr->depth + randint0(11) - 5;
	if (depth > 60)
		depth = 60;
	if (depth < 5)
		depth = 5;

	/* Choose a monster type, using that depth. */
	symbol = mon_symbol_at_depth[depth / 5 - 1][randint0(13)];

	/* Allow (slightly) tougher monsters. */
	depth = p_ptr->depth + (p_ptr->depth < 60 ? p_ptr->depth / 12 : 5);

	/* Set monster generation restrictions.  Describe the monsters. */
	name = mon_restrict(symbol, (byte) depth, &dummy, TRUE);

	/* A default description probably means trouble, so stop. */
	if (streq(name, "misc") || !name[0])
		return;

	/* Build the monster probability table. */
	if (!get_mon_num(depth))
		return;

	/* No normal monsters. */
	generate_mark(y1, x1, y2, x2, SQUARE_TEMP);


	/* Usually, we want 35 monsters. */
	monsters_left = 35;

	/* Fewer monsters near the surface. */
	if (p_ptr->depth < 45)
		monsters_left = 5 + 2 * p_ptr->depth / 3;

	/* More monsters of kinds that tend to be weak. */
	if (strstr("abciBCFKRS", d_char_req))
		monsters_left += 15;

	/* Place the monsters. */
	for (i = 0; i < 300; i++) {
		/* Check for early completion. */
		if (!monsters_left)
			break;

		/* Pick a random in-room square. */
		y = y1 + randint0(1 + ABS(y2 - y1));
		x = x1 + randint0(1 + ABS(x2 - x1));

		/* Require a passable square with no monster in it already. */
		if (!cave_empty_bold(y, x))
			continue;

		/* Place a single monster.  Sleeping 2/3rds of the time. */
		place_monster_aux(y, x, get_mon_num_quick(depth),
						  (randint0(3) != 0), FALSE);

		/* One less monster to place. */
		monsters_left--;
	}

	/* Remove our restrictions. */
	(void) mon_restrict('\0', (byte) depth, &dummy, FALSE);

	/* Describe */
	if (OPT(cheat_room)) {
		/* Room type */
		msg("Room of chambers (%s)", name);
	}
}

/**
 * Place monsters, up to the number asked for, in a rectangle centered on 
 * y0, x0.  Accept values for monster depth, symbol, and maximum vertical 
 * and horizontal displacement.  Call monster restriction functions if 
 * needed.
 *
 * Return prematurely if the code starts looping too much (this may happen 
 * if y0 or x0 are out of bounds, or the area is already occupied).
 */
extern void spread_monsters(char symbol, int depth, int num, int y0,
							int x0, int dy, int dx)
{
	int i, j;					/* Limits on loops */
	int count;
	int y = y0, x = x0;
	int start_mon_num = m_max;
	bool dummy;

	/* Restrict monsters.  Allow uniques. */
	(void) mon_restrict(symbol, (byte) depth, &dummy, TRUE);

	/* Set generation level */
	monster_level = depth;

	/* Build the monster probability table. */
	if (!get_mon_num(depth))
		return;


	/* Try to summon monsters within our rectangle of effect. */
	for (count = 0, i = 0; ((count < num) && (i < 50)); i++) {
		/* Get a location */
		if ((dy == 0) && (dx == 0)) {
			y = y0;
			x = x0;
			if (!in_bounds(y, x))
				return;
		} else {
			for (j = 0; j < 10; j++) {
				y = rand_spread(y0, dy);
				x = rand_spread(x0, dx);
				if (!in_bounds(y, x)) {
					if (j < 9)
						continue;
					else
						return;
				}
				break;
			}
		}

		/* Require "empty" floor grids */
		if (!cave_empty_bold(y, x))
			continue;

		/* Place the monster (sleeping, allow groups, quickly) */
		(void) place_monster(y, x, TRUE, TRUE, TRUE);

		/* Rein in monster groups and escorts a little. */
		if (m_max - start_mon_num > num * 2)
			break;

		/* Count the monster(s), reset the loop count */
		count++;
		i = 0;
	}

	/* Remove monster restrictions. */
	(void) mon_restrict('\0', (byte) depth, &dummy, TRUE);

	/* Reset monster generation level. */
	monster_level = p_ptr->depth;
}


/**
 * Apply any general restrictions on monsters in vaults and themed levels.
 */
extern void general_monster_restrictions(void)
{
	int i;

	/* Clear global monster restriction variables. */
	allow_unique = TRUE;
	for (i = 0; i < 10; i++)
		d_char_req[i] = '\0';
	for (i = 0; i < 4; i++)
		d_attr_req[i] = 0;
	racial_flag_mask = 0;
	rsf_wipe(breath_flag_mask);

	/* Assume no restrictions. */
	get_mon_num_hook = NULL;


	/* Most themed levels have monster restrictions. */
	switch (p_ptr->themed_level) {
	case THEME_ELEMENTAL:
		{
			/* Handled with a call from mon_select() */
			get_mon_num_hook = mon_select;

			break;
		}
	case THEME_DRAGON:
		{
			strcpy(d_char_req, "dD");
			get_mon_num_hook = mon_select;
			break;
		}
	case THEME_WILDERNESS:
		{
			racial_flag_mask = RF_ANIMAL;
			get_mon_num_hook = mon_select;
			break;
		}
	case THEME_DEMON:
		{
			racial_flag_mask = RF_DEMON;
			get_mon_num_hook = mon_select;
			break;
		}
	case THEME_MINE:
		{
			strcpy(d_char_req, "oOPT");
			get_mon_num_hook = mon_select;
			break;
		}
	case THEME_ESTOLAD:
		{
			racial_flag_mask = RF_RACIAL;
			get_mon_num_hook = mon_select;
			break;
		}

		/* All non-themed levels, and any themed levels not defined */
	default:
		{
			break;
		}
	}

	/* Prepare allocation table. */
	get_mon_num_prep();
}

/**
 * To avoid rebuilding the monster list too often (which can quickly 
 * get expensive), we handle monsters of a specified race separately.
 */
extern void get_vault_monsters(char racial_symbol[], byte vault_type,
							   const char *data, int y1, int y2, int x1,
							   int x2)
{
	int i, y, x, temp;
	const char *t;

	for (i = 0; racial_symbol[i] != '\0'; i++) {
		/* Require correct race, allow uniques. */
		allow_unique = TRUE;
		sprintf(d_char_req, "%c", racial_symbol[i]);
		d_attr_req[0] = 0;
		d_attr_req[1] = 0;
		d_attr_req[2] = 0;
		d_attr_req[3] = 0;
		racial_flag_mask = 0;
		rsf_wipe(breath_flag_mask);

		/* Determine level of monster */
		if (vault_type == 0)
			temp = p_ptr->depth + 3;
		else if (vault_type == 7)
			temp = p_ptr->depth;
		else if (vault_type == 8)
			temp = p_ptr->depth + 3;
		else if (vault_type == 9)
			temp = p_ptr->depth + 6;
		else if (vault_type == 12)
			temp = p_ptr->depth + 3;
		else if (vault_type == 13)
			temp = p_ptr->depth + 6;
		else if ((vault_type > 13) && (vault_type % 2))
			temp = p_ptr->depth + 4;
		else
			temp = p_ptr->depth;

		/* Apply our restrictions */
		get_mon_num_hook = mon_select;

		/* Prepare allocation table */
		get_mon_num_prep();

		/* Build the monster probability table. */
		if (!get_mon_num(temp))
			continue;


		/* Place the monsters */
		for (t = data, y = y1; y <= y2; y++) {
			for (x = x1; x <= x2; x++, t++) {
				if (*t == racial_symbol[i]) {
					/* Place a monster */
					place_monster_aux(y, x, get_mon_num_quick(temp), FALSE,
									  FALSE);
				}
			}
		}
	}

	/* Clear any current monster restrictions. */
	if (get_mon_num_hook) {
		get_mon_num_hook = NULL;
		get_mon_num_prep();
	}
}
