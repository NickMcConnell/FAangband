/*
 * File: x-spell.c
 * Purpose: Spell effect definitions and information about them
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
#include "button.h"
#include "cave.h"
#include "cmds.h"
#include "effects.h"
#include "target.h"
#include "tvalsval.h"
#include "spells.h"

/*
 * The defines below must match the spell numbers in spell.txt
 * if they don't, "interesting" things will probably happen.
 *
 * It would be nice if we could get rid of this dependency.
 */
#define SPELL_FIRE_BOLT                           0
#define SPELL_DETECT_MONSTERS                     1
#define SPELL_PHASE_DOOR                          2
#define SPELL_DETECT_TRAPS_DOORS_STAIRS           3
#define SPELL_LIGHT_AREA                          4
#define SPELL_STINKING_CLOUD                      5
#define SPELL_REDUCE_CUTS_AND_POISON              6
#define SPELL_RESIST_MAGIC                        7
#define SPELL_IDENTIFY                            8
#define SPELL_LIGHTNING_BOLT                      9
#define SPELL_CONFUSE_MONSTER                    10
#define SPELL_TELEKINESIS                        11
#define SPELL_SLEEP_MONSTER                      12
#define SPELL_TELEPORT_SELF                      13
#define SPELL_SPEAR_OF_LIGHT                     14
#define SPELL_FROST_BEAM                         15
#define SPELL_MAGICAL_THROW                      16
#define SPELL_SATISFY_HUNGER                     17
#define SPELL_DETECT_INVISIBLE                   18
#define SPELL_MAGIC_DISARM                       19
#define SPELL_BLINK_MONSTER                      20
#define SPELL_CURE                               21
#define SPELL_DETECT_ENCHANTMENT                 22
#define SPELL_STONE_TO_MUD                       23
#define SPELL_MINOR_RECHARGE                     24
#define SPELL_SLEEP_MONSTERS                     25
#define SPELL_THRUST_AWAY                        26
#define SPELL_FIRE_BALL                          27
#define SPELL_TAP_MAGICAL_ENERGY                 28
#define SPELL_SLOW_MONSTER                       29
#define SPELL_TELEPORT_OTHER                     30
#define SPELL_HASTE_SELF                         31
#define SPELL_HOLD_MONSTERS                      32
#define SPELL_CLEAR_MIND                         33
#define SPELL_RESIST_ELEMENT                     34
#define SPELL_SHIELD                             35
#define SPELL_RESISTANCE                         36
#define SPELL_ESSENCE_OF_SPEED                   37
#define SPELL_STRENGTHEN_DEFENCES                38
#define SPELL_DOOR_CREATION                      39
#define SPELL_STAIR_CREATION                     40
#define SPELL_TELEPORT_LEVEL                     41
#define SPELL_WORD_OF_RECALL                     42
#define SPELL_WORD_OF_DESTRUCTION                43
#define SPELL_DIMENSION_DOOR                     44
#define SPELL_ACID_BOLT                          45
#define SPELL_POLYMORPH_OTHER                    46
#define SPELL_EARTHQUAKE                         47
#define SPELL_BEGUILING                          48
#define SPELL_STARBURST                          49
#define SPELL_MAJOR_RECHARGE                     50
#define SPELL_CLOUD_KILL                         51
#define SPELL_ICE_STORM                          52
#define SPELL_METEOR_SWARM                       53
#define SPELL_CACOPHONY                          54
#define SPELL_UNLEASH_CHAOS                      55
#define SPELL_WALL_OF_FORCE                      56
#define SPELL_RUNE_OF_THE_ELEMENTS               57
#define SPELL_RUNE_OF_MAGIC_DEFENCE              58
#define SPELL_RUNE_OF_INSTABILITY                59
#define SPELL_RUNE_OF_MANA                       60
#define SPELL_RUNE_OF_PROTECTION                 61
#define SPELL_RUNE_OF_POWER                      62
#define SPELL_RUNE_OF_SPEED                      63
#define PRAYER_DETECT_EVIL                       64
#define PRAYER_CURE_LIGHT_WOUNDS                 65
#define PRAYER_BLESS                             66
#define PRAYER_REMOVE_FEAR                       67
#define PRAYER_CALL_LIGHT                        68
#define PRAYER_FIND_TRAPS                        69
#define PRAYER_DETECT_DOORS_STAIRS               70
#define PRAYER_SLOW_POISON                       71
#define PRAYER_CURE_SERIOUS_WOUNDS               72
#define PRAYER_SCARE_MONSTER                     73
#define PRAYER_PORTAL                            74
#define PRAYER_CHANT                             75
#define PRAYER_SANCTUARY                         76
#define PRAYER_SATISFY_HUNGER                    77
#define PRAYER_REMOVE_CURSE                      78
#define PRAYER_RESIST_HEAT_AND_COLD              79
#define PRAYER_NEUTRALIZE_POISON                 80
#define PRAYER_ORB_OF_DRAINING                   81
#define PRAYER_SENSE_INVISIBLE                   82
#define PRAYER_PROTECTION_FROM_EVIL              83
#define PRAYER_CURE_MORTAL_WOUNDS                84
#define PRAYER_EARTHQUAKE                        85
#define PRAYER_SENSE_SURROUNDINGS                86
#define PRAYER_TURN_UNDEAD                       87
#define PRAYER_PRAYER                            88
#define PRAYER_DISPEL_UNDEAD                     89
#define PRAYER_HEAL                              90
#define PRAYER_DISPEL_EVIL                       91
#define PRAYER_SACRED_SHIELD                     92
#define PRAYER_GLYPH_OF_WARDING                  93
#define PRAYER_HOLY_WORD                         94
#define PRAYER_BLINK                             95
#define PRAYER_TELEPORT_SELF                     96
#define PRAYER_TELEPORT_OTHER                    97
#define PRAYER_TELEPORT_LEVEL                    98
#define PRAYER_WORD_OF_RECALL                    99
#define PRAYER_ALTER_REALITY                    100
#define PRAYER_DETECT_MONSTERS                  101
#define PRAYER_DETECTION                        102
#define PRAYER_PROBING                          103
#define PRAYER_PERCEPTION                       104
#define PRAYER_CLAIRVOYANCE                     105
#define PRAYER_BANISHMENT                       106
#define PRAYER_HEALING                          107
#define PRAYER_SACRED_KNOWLEDGE                 108
#define PRAYER_RESTORATION                      109
#define PRAYER_REMEMBRANCE                      110
#define PRAYER_UNBARRING_WAYS                   111
#define PRAYER_RECHARGING                       112
#define PRAYER_DISPEL_CURSE                     113
#define PRAYER_DISARM_TRAP                      114
#define PRAYER_HOLDING                          115
#define PRAYER_ENCHANT_WEAPON_OR_ARMOUR         116
#define PRAYER_LIGHT_OF_MANWE                   117
#define PRAYER_LANCE_OF_OROME                   118
#define PRAYER_HAMMER_OF_AULE                   119
#define PRAYER_STRIKE_OF_MANDOS                 120
#define PRAYER_CALL_ON_VARDA                    121
#define PRAYER_ELEMENTAL_INFUSION               122
#define PRAYER_SANCTIFY_FOR_BATTLE              123
#define PRAYER_HORN_OF_WRATH                    124
#define SPELL_HIT_AND_RUN                       125
#define SPELL_DAY_OF_MISRULE                    126
#define SPELL_DETECT_TREASURE                   127
#define LORE_DETECT_LIFE                        128
#define LORE_CALL_LIGHT                         129
#define LORE_FORAGING                           130
#define LORE_BLINK                              131
#define LORE_COMBAT_POISON                      132
#define LORE_LIGHTNING_SPARK                    133
#define LORE_DOOR_DESTRUCION                    134
#define LORE_TURN_STONE_TO_MUD                  135
#define LORE_RAY_OF_SUNLIGHT                    136
#define LORE_CURE_POISON                        137
#define LORE_FROST_BOLT                         138
#define LORE_SLEEP_CREATURE                     139
#define LORE_FRIGHTEN_CREATURE                  140
#define LORE_DETECT_TRAPS_DOORS                 141
#define LORE_SNUFF_SMALL_LIFE                   142
#define LORE_FIRE_BOLT                          143
#define LORE_HEROISM                            144
#define LORE_REMOVE_CURSE                       145
#define LORE_ACID_BOLT                          146
#define LORE_TELEPORT_MONSTER                   147
#define LORE_GRAVITY_BOLT                       148
#define LORE_RESIST_POISON                      149
#define LORE_EARTHQUAKE                         150
#define LORE_RESIST_FIRE_AND_COLD               151
#define LORE_DETECT_ALL                         152
#define LORE_NATURAL_VITALITY                   153
#define LORE_RESIST_ACID_AND_LIGHTNING          154
#define LORE_WITHER_FOE                         155
#define LORE_DISARM_TRAP                        156
#define LORE_IDENTIFY                           157
#define LORE_CREATE_ATHELAS                     158
#define LORE_RAGING_STORM                       159
#define LORE_THUNDERCLAP                        160
#define LORE_BECOME_MOUSE                       161
#define LORE_BECOME_FERRET                      162
#define LORE_BECOME_HOUND                       163
#define LORE_BECOME_GAZELLE                     164
#define LORE_BECOME_LION                        165
#define LORE_DETECT_EVIL                        166
#define LORE_SONG_OF_FRIGHTENING                167
#define LORE_SENSE_SURROUNDINGS                 168
#define LORE_SIGHT_BEYOND_SIGHT                 169
#define LORE_HERBAL_HEALING                     170
#define LORE_BLIZZARD                           171
#define LORE_TRIGGER_TSUNAMI                    172
#define LORE_VOLCANIC_ERUPTION                  173
#define LORE_MOLTEN_LIGHTNING                   174
#define LORE_STARBURST                          175
#define LORE_SONG_OF_LULLING                    176
#define LORE_SONG_OF_PROTECTION                 177
#define LORE_SONG_OF_DISPELLING                 178
#define LORE_SONG_OF_WARDING                    179
#define LORE_SONG_OF_RENEWAL                    180
#define LORE_WEB_OF_VAIRE                       181
#define LORE_SPEED_OF_NESSA                     182
#define LORE_RENEWAL_OF_VANA                    183
#define LORE_SERVANT_OF_YAVANNA                 184
#define LORE_TEARS_OF_NIENNA                    185
#define LORE_HEALING_OF_ESTE                    186
#define LORE_CREATURE_KNOWLEDGE                 187
#define LORE_NATURES_VENGEANCE                  188
#define LORE_SONG_OF_GROWTH                     189
#define LORE_SONG_OF_PRESERVATION               190
#define LORE_TREMOR                             191
#define RITUAL_NETHER_BOLT                      192
#define RITUAL_DETECT_EVIL                      193
#define RITUAL_ENHANCED_INFRAVISION             194
#define RITUAL_BREAK_CURSE                      195
#define RITUAL_SLOW_MONSTER                     196
#define RITUAL_SLEEP_MONSTER                    197
#define RITUAL_HORRIFY                          198
#define RITUAL_BECOME_BAT                       199
#define RITUAL_DOOR_DESTRUCTION                 200
#define RITUAL_DARK_BOLT                        201
#define RITUAL_NOXIOUS_FUMES                    202
#define RITUAL_TURN_UNDEAD                      203
#define RITUAL_TURN_EVIL                        204
#define RITUAL_CURE_POISON                      205
#define RITUAL_DISPEL_UNDEAD                    206
#define RITUAL_DISPEL_EVIL                      207
#define RITUAL_SEE_INVISIBLE                    208
#define RITUAL_SHADOW_SHIFTING                  209
#define RITUAL_DETECT_TRAPS                     210
#define RITUAL_DETECT_DOORS_STAIRS              211
#define RITUAL_SLEEP_MONSTERS                   212
#define RITUAL_SLOW_MONSTERS                    213
#define RITUAL_DETECT_MAGIC                     214
#define RITUAL_DEATH_BOLT                       215
#define RITUAL_RESIST_POISON                    216
#define RITUAL_EXORCISE_DEMONS                  217
#define RITUAL_DARK_SPEAR                       218
#define RITUAL_CHAOS_STRIKE                     219
#define RITUAL_GENOCIDE                         220
#define RITUAL_DARK_BALL                        221
#define RITUAL_STENCH_OF_DEATH                  222
#define RITUAL_PROBING                          223
#define RITUAL_SHADOW_MAPPING                   224
#define RITUAL_IDENTIFY                         225
#define RITUAL_SHADOW_WARPING                   226
#define RITUAL_POISON_AMMO                      227
#define RITUAL_RESIST_ACID_AND_COLD             228
#define RITUAL_HEAL_ANY_WOUND                   229
#define RITUAL_PROTECTION_FROM_EVIL             230
#define RITUAL_BLACK_BLESSING                   231
#define RITUAL_BANISH_EVIL                      232
#define RITUAL_SHADOW_BARRIER                   233
#define RITUAL_DETECT_ALL_MONSTERS              234
#define RITUAL_STRIKE_AT_LIFE                   235
#define RITUAL_ORB_OF_DEATH                     236
#define RITUAL_DISPEL_LIFE                      237
#define RITUAL_VAMPIRIC_DRAIN                   238
#define RITUAL_RECHARGING                       239
#define RITUAL_BECOME_WEREWOLF                  240
#define RITUAL_DISPEL_CURSE                     241
#define RITUAL_BECOME_VAMPIRE                   242
#define RITUAL_HASTE_SELF                       243
#define RITUAL_PREPARE_BLACK_BREATH             244
#define RITUAL_WORD_OF_DESTRUCTION              245
#define RITUAL_TELEPORT_AWAY                    246
#define RITUAL_SMASH_UNDEAD                     247
#define RITUAL_BIND_UNDEAD                      248
#define RITUAL_DARKNESS_STORM                   249
#define RITUAL_MENTAL_AWARENESS                 250
#define RITUAL_SLIP_INTO_THE_SHADOWS            251
#define RITUAL_BLOODWRATH                       252
#define RITUAL_REBALANCE_WEAPON                 253
#define RITUAL_DARK_POWER                       254



int get_spell_index(const object_type * o_ptr, int index)
{
	int spell = mp_ptr->book_start_index[o_ptr->sval] + index;
	if (spell > mp_ptr->book_start_index[o_ptr->sval + 1])
		return -1;

	return spell;
}

const char *get_spell_name(int sindex)
{
	return s_info[sindex].name;
}

/**
 * Extra information on a spell		-DRS-
 *
 * We can use up to 20 characters of the buffer 'p'
 *
 * The strings in this function were extracted from the code in the
 * functions "do_cmd_cast()" and "do_cmd_pray()" and are up to date 
 * (as of 0.4.0). -LM-
 */
void get_spell_info(int tval, int sindex, char *p, size_t len)
{
	int plev = p_ptr->lev;

	int beam, beam_low;

	/* Specialty Ability */
	if (player_has(PF_HEIGHTEN_MAGIC))
		plev += 1 + ((p_ptr->heighten_power + 5) / 10);
	if (player_has(PF_CHANNELING))
		plev += get_channeling_boost();

	/* Beaming chance */
	beam = (((player_has(PF_BEAM))) ? plev : (plev / 2));
	beam_low = (beam - 10 > 0 ? beam - 10 : 0);

	/* Default */
	strcpy(p, "");

	/* Analyze the spell */
	switch (sindex) {
		/* Sorcery */

	case SPELL_FIRE_BOLT:
		strnfmt(p, len, " dam %dd3", 4 + plev / 10);
		break;
	case SPELL_PHASE_DOOR:
		strnfmt(p, len, " range 10");
		break;
	case SPELL_LIGHT_AREA:
		strnfmt(p, len, " dam 2d%d, rad %d", 1 + (plev / 5),
				(plev / 10) + 1);
		break;
	case SPELL_STINKING_CLOUD:
		strnfmt(p, len, " dam %d, rad 2", 5 + (plev / 3));
		break;
	case SPELL_RESIST_MAGIC:
		strnfmt(p, len, " dur 10+d5");
		break;
	case SPELL_LIGHTNING_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", (2 + ((plev - 5) / 5)),
				beam);
		break;
	case SPELL_TELEPORT_SELF:
		strnfmt(p, len, " range %d", 50 + plev * 2);
		break;
	case SPELL_FROST_BEAM:
		strnfmt(p, len, " dam %d", 5 + plev);
		break;
	case SPELL_BLINK_MONSTER:
		strnfmt(p, len, " range %d", 5 + plev / 10);
		break;
	case SPELL_THRUST_AWAY:
		strnfmt(p, len, " dam %dd%d, length %d", 6 + (plev / 10), 8,
				1 + (plev / 10));
		break;
	case SPELL_FIRE_BALL:
		strnfmt(p, len, " dam %d, rad 2", 55 + plev);
		break;
	case SPELL_TELEPORT_OTHER:
		strnfmt(p, len, " dist %d", 45 + (plev / 2));
		break;
	case SPELL_RESIST_ELEMENT:
		strnfmt(p, len, " dur %d+d%d", plev, plev);
		break;
	case SPELL_SHIELD:
		strnfmt(p, len, " dur 30+d20");
		break;
	case SPELL_RESISTANCE:
		strnfmt(p, len, " dur 20+d20");
		break;
	case SPELL_ESSENCE_OF_SPEED:
		strnfmt(p, len, " dur %d+d30", 10 + plev);
		break;
	case SPELL_STRENGTHEN_DEFENCES:
		strnfmt(p, len, " dur 40");
		break;
	case SPELL_ACID_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", (3 + plev / 3), beam);
		break;
	case SPELL_STARBURST:
		strnfmt(p, len, " dam %d, rad %d", 5 * plev / 2, plev / 12);
		break;
	case SPELL_CLOUD_KILL:
		strnfmt(p, len, " dam %d,%d, rad 3,2", 10 + plev, 2 * plev);
		break;
	case SPELL_ICE_STORM:
		strnfmt(p, len, " dam %d, rad 3", 3 * plev);
		break;
	case SPELL_METEOR_SWARM:
		strnfmt(p, len, " dam %d, rad 1", 80 + plev * 2);
		break;
	case SPELL_CACOPHONY:
		strnfmt(p, len, " dam %d+3d%d", plev, plev);
		break;
	case SPELL_UNLEASH_CHAOS:
		strnfmt(p, len, " dam %d, rad 1", 80 + plev * 2);
		break;
	case SPELL_WALL_OF_FORCE:
		strnfmt(p, len, " dam %d, rad %d", 4 * plev, 3 + plev / 15);
		break;


		/* Piety */
	case PRAYER_CURE_LIGHT_WOUNDS:
		strnfmt(p, len, " heal 2d%d", plev / 4 + 5);
		break;
	case PRAYER_BLESS:
		strnfmt(p, len, " dur 12+d12");
		break;
	case PRAYER_CALL_LIGHT:
		strnfmt(p, len, " dam 2d%d, rad %d", 1 + plev / 3, plev / 10 + 1);
		break;
	case PRAYER_SLOW_POISON:
		strnfmt(p, len, " halve poison");
		break;
	case PRAYER_CURE_SERIOUS_WOUNDS:
		strnfmt(p, len, " heal 4d%d", plev / 4 + 6);
		break;
	case PRAYER_PORTAL:
		strnfmt(p, len, " range %d", 2 * plev);
		break;
	case PRAYER_CHANT:
		strnfmt(p, len, " dur 24+d24");
		break;
	case PRAYER_RESIST_HEAT_AND_COLD:
		strnfmt(p, len, " dur %d+d10", plev / 2);
		break;
	case PRAYER_ORB_OF_DRAINING:
		strnfmt(p, len, " dam %d+3d6, rad %d",
				plev / 4 +
				(plev / ((player_has(PF_STRONG_MAGIC)) ? 2 : 4)),
				(plev < 30) ? 1 : 2);
		break;
	case PRAYER_SENSE_INVISIBLE:
		strnfmt(p, len, " dur %d+d24", plev);
		break;
	case PRAYER_PROTECTION_FROM_EVIL:
		strnfmt(p, len, " dur %d+d24", 3 * plev / 2);
		break;
	case PRAYER_CURE_MORTAL_WOUNDS:
		strnfmt(p, len, " heal 9d%d, any cut", plev / 3 + 12);
		break;
	case PRAYER_EARTHQUAKE:
		strnfmt(p, len, " radius 10");
		break;
	case PRAYER_PRAYER:
		strnfmt(p, len, " dur 48+d48");
		break;
	case PRAYER_DISPEL_UNDEAD:
		strnfmt(p, len, " dam d%d", 3 * plev);
		break;
	case PRAYER_HEAL:
		strnfmt(p, len, " heal 300, any cut");
		break;
	case PRAYER_DISPEL_EVIL:
		strnfmt(p, len, " dam d%d", 3 * plev);
		break;
	case PRAYER_SACRED_SHIELD:
		strnfmt(p, len, " dur %d+d20", plev / 2);
		break;
	case PRAYER_HOLY_WORD:
		strnfmt(p, len, " dam d%d, heal 300", plev * 4);
		break;
	case PRAYER_BLINK:
		strnfmt(p, len, " range 10");
		break;
	case PRAYER_TELEPORT_SELF:
		strnfmt(p, len, " range %d", 4 * plev);
		break;
	case PRAYER_TELEPORT_OTHER:
		strnfmt(p, len, " dist %d", 45 + (plev / 3));
		break;
	case PRAYER_HEALING:
		strnfmt(p, len, " heal 700, any cut");
		break;
	case PRAYER_LIGHT_OF_MANWE:
		strnfmt(p, len, " dam %d, rad 3", 2 * plev);
		break;
	case PRAYER_LANCE_OF_OROME:
		strnfmt(p, len, " dam %d", 3 * plev / 2);
		break;
	case PRAYER_STRIKE_OF_MANDOS:
		strnfmt(p, len, " dam %d+d100", plev * 3);
		break;
	case PRAYER_CALL_ON_VARDA:
		strnfmt(p, len, " dam %d, heal 500", plev * 5);
		break;


		/* Nature Magics */

	case LORE_CALL_LIGHT:
		strnfmt(p, len, " dam 2d%d, rad %d", 1 + plev / 4, plev / 10 + 1);
		break;
	case LORE_BLINK:
		strnfmt(p, len, " range 10");
		break;
	case LORE_COMBAT_POISON:
		strnfmt(p, len, " halve poison");
		break;
	case LORE_LIGHTNING_SPARK:
		strnfmt(p, len, " dam %dd6, length %d", 2 + plev / 8,
				1 + plev / 5);
		break;
	case LORE_TURN_STONE_TO_MUD:
		strnfmt(p, len, " dam 20+d30");
		break;
	case LORE_RAY_OF_SUNLIGHT:
		strnfmt(p, len, " dam 4d5");
		break;
	case LORE_FROST_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", 2 + plev / 5, beam_low);
		break;
	case LORE_SNUFF_SMALL_LIFE:
		strnfmt(p, len, " dam %d", 2 + plev / 5);
		break;
	case LORE_FIRE_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", 3 + plev / 5, beam_low);
		break;
	case LORE_HEROISM:
		strnfmt(p, len, " dur 20+d20");
		break;
	case LORE_ACID_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", 5 + plev / 5, beam_low);
		break;
	case LORE_TELEPORT_MONSTER:
		strnfmt(p, len, " dist %d", 45 + (plev / 3));
		break;
	case LORE_GRAVITY_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", 5 + plev / 4, beam_low);
		break;
	case LORE_RESIST_POISON:
		strnfmt(p, len, " dur 20+d20");
		break;
	case LORE_EARTHQUAKE:
		strnfmt(p, len, " radius 10");
		break;
	case LORE_RESIST_FIRE_AND_COLD:
		strnfmt(p, len, " dur 20+d20");
		break;
	case LORE_NATURAL_VITALITY:
		strnfmt(p, len, " heal 2d%d, pois/cut", plev / 5);
		break;
	case LORE_RESIST_ACID_AND_LIGHTNING:
		strnfmt(p, len, " dur 20+d20");
		break;
	case LORE_WITHER_FOE:
		strnfmt(p, len, " dam %dd8, conf/slow", plev / 7);
		break;
	case LORE_RAGING_STORM:
		strnfmt(p, len, " dam %d+d%d, rad %d", plev, 60 + plev * 2,
				1 + plev / 15);
		break;
	case LORE_THUNDERCLAP:
		strnfmt(p, len, " dam %d+d%d, rad %d", plev, 40 + plev * 2,
				plev / 8);
		break;
	case LORE_SIGHT_BEYOND_SIGHT:
		strnfmt(p, len, " dur 24+d24");
		break;
	case LORE_HERBAL_HEALING:
		strnfmt(p, len, " heal %dd12, any cut", 25 + plev / 2);
		break;
	case LORE_BLIZZARD:
		strnfmt(p, len, " dam %d+d%d, rad %d", plev, 50 + plev * 2,
				1 + plev / 12);
		break;
	case LORE_TRIGGER_TSUNAMI:
		strnfmt(p, len, " dam %d+d%d, rad %d", 30 + ((4 * plev) / 5),
				30 + plev * 2, plev / 7);
		break;
	case LORE_VOLCANIC_ERUPTION:
		strnfmt(p, len, " dam %d+d%d, rad %d", 3 * plev / 2, 50 + plev * 3,
				1 + plev / 15);
		break;
	case LORE_MOLTEN_LIGHTNING:
		strnfmt(p, len, " dam %d+d%d, rad 1", 35 + (2 * plev),
				90 + plev * 4);
		break;
	case LORE_STARBURST:
		strnfmt(p, len, " dam %d+d%d, rad %d", 40 + (3 * plev / 2),
				plev * 3, plev / 10);
		break;
	case LORE_SONG_OF_PROTECTION:
		strnfmt(p, len, " dur %d+d30", plev / 2);
		break;
	case LORE_SONG_OF_DISPELLING:
		strnfmt(p, len, " dam d%d", plev * 2);
		break;
	case LORE_WEB_OF_VAIRE:
		strnfmt(p, len, " dam %dd8, beam %d%%", plev / 6, plev * 2);
		break;
	case LORE_SPEED_OF_NESSA:
		strnfmt(p, len, " dur %d+d10", plev / 2);
		break;
	case LORE_HEALING_OF_ESTE:
		strnfmt(p, len, " heal 500, dam 100");
		break;
	case LORE_NATURES_VENGEANCE:
		strnfmt(p, len, " dam 6d6+%d", plev / 3);
		break;

		/* Necromantic Spells */

	case RITUAL_NETHER_BOLT:
		strnfmt(p, len, " dam 2d%d", 5 + plev / 7);
		break;
	case RITUAL_ENHANCED_INFRAVISION:
		strnfmt(p, len, " dur 70+d70");
		break;
	case RITUAL_BECOME_BAT:
		strnfmt(p, len, " hurt 2d4");
		break;
	case RITUAL_DARK_BOLT:
		strnfmt(p, len, " dam %dd8, beam %d%%", 3 + plev / 7, beam_low);
		break;
	case RITUAL_NOXIOUS_FUMES:
		strnfmt(p, len, " dam %d, poison", 10 + plev);
		break;
	case RITUAL_DISPEL_UNDEAD:
		strnfmt(p, len, " dam %d+d%d", plev + 15, 3 * plev / 2);
		break;
	case RITUAL_DISPEL_EVIL:
		strnfmt(p, len, " dam %d+d%d", plev, plev);
		break;
	case RITUAL_SEE_INVISIBLE:
		strnfmt(p, len, " dur 20+d%d", plev / 2);
		break;
	case RITUAL_SHADOW_SHIFTING:
		strnfmt(p, len, " range 16, hurt 1d4");
		break;
	case RITUAL_DEATH_BOLT:
		strnfmt(p, len, " dam %dd8, hurt 1d6", 2 + plev / 3);
		break;
	case RITUAL_RESIST_POISON:
		strnfmt(p, len, " dur %d+d20", plev / 2);
		break;
	case RITUAL_EXORCISE_DEMONS:
		strnfmt(p, len, " dam %d+d%d", 2 * plev, 2 * plev);
		break;
	case RITUAL_DARK_SPEAR:
		strnfmt(p, len, " dam %d", 15 + 7 * plev / 4);
		break;
	case RITUAL_CHAOS_STRIKE:
		strnfmt(p, len, " dam %dd8, beam %d%%", 1 + plev / 2, beam);
		break;
	case RITUAL_DARK_BALL:
		strnfmt(p, len, " dam %d, rad 2", 50 + plev * 2);
		break;
	case RITUAL_STENCH_OF_DEATH:
		strnfmt(p, len, " dam %d+d%d, hit ~9", 50 + plev * 2, plev);
		break;
	case RITUAL_SHADOW_WARPING:
		strnfmt(p, len, " range %d, hurt 2d6", plev * 3);
		break;
	case RITUAL_RESIST_ACID_AND_COLD:
		strnfmt(p, len, " dur 20+d20");
		break;
	case RITUAL_PROTECTION_FROM_EVIL:
		strnfmt(p, len, " %d+d%d", plev / 2, plev);
		break;
	case RITUAL_BLACK_BLESSING:
		strnfmt(p, len, " dur d66");
		break;
	case RITUAL_SHADOW_BARRIER:
		strnfmt(p, len, " dur 10+d20");
		break;
	case RITUAL_STRIKE_AT_LIFE:
		strnfmt(p, len, " dam %dd13", 3 * plev / 5);
		break;
	case RITUAL_ORB_OF_DEATH:
		strnfmt(p, len, " dam %d, hurt 2d8", 20 + (4 * plev));
		break;
	case RITUAL_DISPEL_LIFE:
		strnfmt(p, len, " dam %d+4d%d", 60, plev);
		break;
	case RITUAL_VAMPIRIC_DRAIN:
		strnfmt(p, len, " dam %dd11, heal %d", plev / 3, 3 * plev);
		break;
	case RITUAL_BECOME_WEREWOLF:
		strnfmt(p, len, " hurt 2d7");
		break;
	case RITUAL_BECOME_VAMPIRE:
		strnfmt(p, len, " hurt 3d6");
		break;
	case RITUAL_HASTE_SELF:
		strnfmt(p, len, " dur 10+d20");
		break;
	case RITUAL_WORD_OF_DESTRUCTION:
		strnfmt(p, len, " radius 15");
		break;
	case RITUAL_TELEPORT_AWAY:
		strnfmt(p, len, " dist %d", 45 + (plev / 3));
		break;
	case RITUAL_SMASH_UNDEAD:
		strnfmt(p, len, " dam %d+d50", plev * 3);
		break;
	case RITUAL_DARKNESS_STORM:
		strnfmt(p, len, " dam %d, rad %d", 11 * plev / 2, plev / 7);
		break;
	case RITUAL_MENTAL_AWARENESS:
		strnfmt(p, len, " dur 30+d40");
		break;
	case RITUAL_SLIP_INTO_THE_SHADOWS:
		strnfmt(p, len, " dur 40");
		break;
	case RITUAL_BLOODWRATH:
		strnfmt(p, len, " dur 40");
		break;
	case RITUAL_DARK_POWER:
		strnfmt(p, len, " mana %d", 3 * plev / 2);
		break;
	}
}



bool spell_needs_aim(int tval, int spell)
{
	const magic_type *mt_ptr = &mp_ptr->info[spell];

	switch (mt_ptr->index) {
	case SPELL_FIRE_BOLT:
	case SPELL_STINKING_CLOUD:
	case SPELL_LIGHTNING_BOLT:
	case SPELL_CONFUSE_MONSTER:
	case SPELL_SLEEP_MONSTER:
	case SPELL_SPEAR_OF_LIGHT:
	case SPELL_FROST_BEAM:
	case SPELL_MAGIC_DISARM:
	case SPELL_BLINK_MONSTER:
	case SPELL_STONE_TO_MUD:
	case SPELL_THRUST_AWAY:
	case SPELL_FIRE_BALL:
	case SPELL_SLOW_MONSTER:
	case SPELL_TELEPORT_OTHER:
	case SPELL_HOLD_MONSTERS:
	case SPELL_ACID_BOLT:
	case SPELL_POLYMORPH_OTHER:
	case SPELL_CLOUD_KILL:
	case SPELL_ICE_STORM:
	case SPELL_METEOR_SWARM:
	case SPELL_UNLEASH_CHAOS:
	case SPELL_WALL_OF_FORCE:
	case PRAYER_SCARE_MONSTER:
	case PRAYER_ORB_OF_DRAINING:
	case PRAYER_TELEPORT_OTHER:
	case PRAYER_DISARM_TRAP:
	case PRAYER_HOLDING:
	case PRAYER_LIGHT_OF_MANWE:
	case PRAYER_LANCE_OF_OROME:
	case PRAYER_STRIKE_OF_MANDOS:
	case LORE_LIGHTNING_SPARK:
	case LORE_TURN_STONE_TO_MUD:
	case LORE_RAY_OF_SUNLIGHT:
	case LORE_FROST_BOLT:
	case LORE_SLEEP_CREATURE:
	case LORE_FRIGHTEN_CREATURE:
	case LORE_FIRE_BOLT:
	case LORE_ACID_BOLT:
	case LORE_TELEPORT_MONSTER:
	case LORE_GRAVITY_BOLT:
	case LORE_WITHER_FOE:
	case LORE_DISARM_TRAP:
	case LORE_RAGING_STORM:
	case LORE_BLIZZARD:
	case LORE_MOLTEN_LIGHTNING:
	case LORE_WEB_OF_VAIRE:
	case RITUAL_NETHER_BOLT:
	case RITUAL_SLOW_MONSTER:
	case RITUAL_SLEEP_MONSTER:
	case RITUAL_HORRIFY:
	case RITUAL_DARK_BOLT:
	case RITUAL_DEATH_BOLT:
	case RITUAL_DARK_SPEAR:
	case RITUAL_CHAOS_STRIKE:
	case RITUAL_DARK_BALL:
	case RITUAL_STENCH_OF_DEATH:
	case RITUAL_STRIKE_AT_LIFE:
	case RITUAL_ORB_OF_DEATH:
	case RITUAL_VAMPIRIC_DRAIN:
	case RITUAL_TELEPORT_AWAY:
	case RITUAL_DARKNESS_STORM:
		return TRUE;

	default:
		return FALSE;
	}
}


bool cast_spell(int tval, int sindex, int dir, int plev)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int shape = SHAPE_NORMAL;
	bool beam = FALSE;

	/* Hack -- higher chance of "beam" instead of "bolt" for mages and necros. */
	beam = ((player_has(PF_BEAM)) ? plev : (plev / 2));

	/* Spell Effects.  Spells are mostly listed by realm, each using a block of 
	 * 64 spell slots, but there are a certain number of spells that are used
	 * by more than one realm (this allows more neat class- specific magics) */
	switch (sindex) {
		/* Sorcerous Spells */

	case SPELL_FIRE_BOLT:
		{
			fire_bolt(GF_FIRE, dir, damroll(4 + plev / 10, 3));
			break;
		}
	case SPELL_DETECT_MONSTERS:
		{
			(void) detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case SPELL_PHASE_DOOR:
		{
			teleport_player(10, TRUE);
			break;
		}
	case SPELL_DETECT_TRAPS_DOORS_STAIRS:
		{
			/* Hack - 'show' effected region only with the first detect */
			(void) detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_doors(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, FALSE);
			break;
		}
	case SPELL_LIGHT_AREA:
		{
			(void) light_area(damroll(2, 1 + (plev / 5)), (plev / 10) + 1);
			break;
		}
	case SPELL_STINKING_CLOUD:
		{
			fire_ball(GF_POIS, dir, 5 + plev / 3, 2, FALSE);
			break;
		}
	case SPELL_REDUCE_CUTS_AND_POISON:
		{
			(void) dec_timed(TMD_POISONED, p_ptr->timed[TMD_POISONED] / 2,
							 TRUE);
			(void) dec_timed(TMD_CUT, p_ptr->timed[TMD_CUT] / 2, TRUE);
			break;
		}
	case SPELL_RESIST_MAGIC:
		{
			if (!p_ptr->timed[TMD_INVULN]) {
				(void) inc_timed(TMD_INVULN, 10 + randint1(5), TRUE);
			} else {
				(void) inc_timed(TMD_INVULN, randint1(5), TRUE);
			}

			break;
		}
	case SPELL_IDENTIFY:
		{
			if (!ident_spell())
				return FALSE;
			break;
		}
	case SPELL_LIGHTNING_BOLT:
		{
			fire_bolt_or_beam(beam, GF_ELEC, dir,
							  damroll(2 + ((plev - 5) / 5), 8));
			break;
		}
	case SPELL_CONFUSE_MONSTER:
		{
			(void) confuse_monster(dir, plev + 10);
			break;
		}
	case SPELL_TELEKINESIS:
		{
			s16b ty, tx;
			if (!target_set_interactive(TARGET_OBJ, -1, -1))
				return FALSE;
			target_get(&tx, &ty);
			if (!py_pickup(1, ty, tx))
				return FALSE;
			break;
		}
	case SPELL_SLEEP_MONSTER:
		{
			(void) sleep_monster(dir, plev + 10);
			break;
		}
	case SPELL_TELEPORT_SELF:
		{
			teleport_player(50 + plev * 2, TRUE);
			break;
		}
	case SPELL_SPEAR_OF_LIGHT:
		{
			msg("A line of blue shimmering light appears.");
			light_line(dir);
			break;
		}
	case SPELL_FROST_BEAM:
		{
			fire_beam(GF_COLD, dir, 5 + plev);
			break;
		}
	case SPELL_MAGICAL_THROW:
		{
			magic_throw = TRUE;
			textui_cmd_throw();
			magic_throw = FALSE;
			break;
		}
	case SPELL_SATISFY_HUNGER:
		{
			(void) set_food(PY_FOOD_MAX - 1);
			break;
		}
	case SPELL_DETECT_INVISIBLE:
		{
			(void) detect_monsters_invis(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case SPELL_MAGIC_DISARM:
		{
			(void) disarm_trap(dir);
			break;
		}
	case SPELL_BLINK_MONSTER:
		{
			(void) teleport_monster(dir, 5 + (plev / 10));
			break;
		}
	case SPELL_CURE:
		{
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			break;
		}
	case SPELL_DETECT_ENCHANTMENT:
		{
			(void) detect_objects_magic(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case SPELL_STONE_TO_MUD:
		{
			(void) wall_to_mud(dir);
			break;
		}
	case SPELL_MINOR_RECHARGE:
		{
			if (!recharge(120))
				return FALSE;
			break;
		}
	case SPELL_SLEEP_MONSTERS:
		{
			(void) sleep_monsters(plev + 10);
			break;
		}
	case SPELL_THRUST_AWAY:
		{
			fire_arc(GF_FORCE, dir, damroll(6 + (plev / 10), 8),
					 (1 + plev / 10), 0);
			break;
		}
	case SPELL_FIRE_BALL:
		{
			fire_ball(GF_FIRE, dir, 55 + plev, 2, FALSE);
			break;
		}
	case SPELL_TAP_MAGICAL_ENERGY:
		{
			tap_magical_energy();
			break;
		}
	case SPELL_SLOW_MONSTER:
		{
			(void) slow_monster(dir, plev + 10);
			break;
		}
	case SPELL_TELEPORT_OTHER:
		{
			(void) teleport_monster(dir, 45 + (plev / 2));
			break;
		}
	case SPELL_HASTE_SELF:
		{
			if (!p_ptr->timed[TMD_FAST]) {
				(void) set_timed(TMD_FAST, randint1(20) + plev, TRUE);
			} else {
				(void) set_timed(TMD_FAST, randint1(5), TRUE);
			}
			break;
		}
	case SPELL_HOLD_MONSTERS:
		{
			fire_ball(GF_HOLD, dir, 0, 2, FALSE);
			break;
		}
	case SPELL_CLEAR_MIND:
		{
			if (p_ptr->csp < p_ptr->msp) {
				p_ptr->csp += 1 + plev / 12;
				p_ptr->csp_frac = 0;
				if (p_ptr->csp > p_ptr->msp)
					(p_ptr->csp = p_ptr->msp);
				msg("You feel your head clear a little.");
				p_ptr->redraw |= (PR_MANA);
			}
			break;
		}
	case SPELL_RESIST_ELEMENT:
		{
			if (!choose_ele_resist())
				return FALSE;
			break;
		}
	case SPELL_SHIELD:
		{
			if (!p_ptr->timed[TMD_SHIELD]) {
				(void) inc_timed(TMD_SHIELD, randint1(20) + 30, TRUE);
			} else {
				(void) inc_timed(TMD_SHIELD, randint1(10) + 15, TRUE);
			}
			break;
		}
	case SPELL_RESISTANCE:
		{
			int time = randint1(20) + 20;
			(void) inc_timed(TMD_OPP_ACID, time, TRUE);
			(void) inc_timed(TMD_OPP_ELEC, time, TRUE);
			(void) inc_timed(TMD_OPP_FIRE, time, TRUE);
			(void) inc_timed(TMD_OPP_COLD, time, TRUE);
			(void) inc_timed(TMD_OPP_POIS, time, TRUE);
			break;
		}
	case SPELL_ESSENCE_OF_SPEED:
		{
			if (!p_ptr->timed[TMD_FAST]) {
				(void) set_timed(TMD_FAST, randint1(30) + 10 + plev, TRUE);
			} else {
				(void) set_timed(TMD_FAST, randint1(10), TRUE);
			}
			break;
		}
	case SPELL_STRENGTHEN_DEFENCES:
		{
			if (!p_ptr->timed[TMD_INVULN]) {
				(void) inc_timed(TMD_INVULN, 40, TRUE);
			} else {
				(void) inc_timed(TMD_INVULN, randint1(20), TRUE);
			}

			break;
		}
	case SPELL_DOOR_CREATION:
		{
			(void) door_creation();
			break;
		}
	case SPELL_STAIR_CREATION:
		{
			(void) stair_creation();
			break;
		}
	case SPELL_TELEPORT_LEVEL:
		{
			(void) teleport_player_level(TRUE);
			break;
		}
	case SPELL_WORD_OF_RECALL:
		{
			if (!word_recall(randint0(20) + 15))
				break;
			break;
		}
	case SPELL_WORD_OF_DESTRUCTION:
		{
			destroy_area(py, px, 15, TRUE);
			break;
		}
	case SPELL_DIMENSION_DOOR:
		{
			msg("Choose a location to teleport to.");
			message_flush();
			dimen_door();
			break;
		}
	case SPELL_ACID_BOLT:
		{
			fire_bolt_or_beam(beam, GF_ACID, dir,
							  damroll(3 + plev / 3, 8));
			break;
		}
	case SPELL_POLYMORPH_OTHER:
		{
			(void) poly_monster(dir);
			break;
		}
	case SPELL_EARTHQUAKE:
		{
			earthquake(py, px, 10, FALSE);
			break;
		}
	case SPELL_BEGUILING:
		{
			(void) slow_monsters(5 * plev / 3);
			(void) confu_monsters(5 * plev / 3);
			(void) sleep_monsters(5 * plev / 3);
			break;
		}
	case SPELL_STARBURST:
		{
			fire_sphere(GF_LIGHT, 0, 5 * plev / 2, plev / 12, 20);
			break;
		}
	case SPELL_MAJOR_RECHARGE:
		{
			recharge(220);
			break;
		}
	case SPELL_CLOUD_KILL:
		{
			fire_ball(GF_POIS, dir, 10 + plev, 3, FALSE);
			fire_ball(GF_ACID, dir, 2 * plev, 2, FALSE);
			break;
		}
	case SPELL_ICE_STORM:
		{
			fire_ball(GF_ICE, dir, 3 * plev, 3, FALSE);
			break;
		}
	case SPELL_METEOR_SWARM:
		{
			fire_ball(GF_METEOR, dir, 80 + (plev * 2), 1, FALSE);
			break;
		}
	case SPELL_CACOPHONY:
		{
			(void) cacophony(plev + damroll(3, plev));
			break;
		}
	case SPELL_UNLEASH_CHAOS:
		{
			fire_ball(GF_CHAOS, dir, 80 + (plev * 2), 3, FALSE);
			break;
		}
	case SPELL_WALL_OF_FORCE:
		{
			fire_arc(GF_FORCE, dir, 4 * plev, 3 + plev / 15, 60);
			break;
		}
	case SPELL_RUNE_OF_THE_ELEMENTS:
		{
			lay_rune(RUNE_ELEMENTS);
			break;
		}
	case SPELL_RUNE_OF_MAGIC_DEFENCE:
		{
			lay_rune(RUNE_MAGDEF);
			break;
		}
	case SPELL_RUNE_OF_INSTABILITY:
		{
			lay_rune(RUNE_QUAKE);
			break;
		}
	case SPELL_RUNE_OF_MANA:
		{
			lay_rune(RUNE_MANA);
			break;
		}
	case SPELL_RUNE_OF_PROTECTION:
		{
			lay_rune(RUNE_PROTECT);
			break;
		}
	case SPELL_RUNE_OF_POWER:
		{
			lay_rune(RUNE_POWER);
			break;
		}
	case SPELL_RUNE_OF_SPEED:
		{
			lay_rune(RUNE_SPEED);
			break;
		}

		/* Holy Prayers */

	case PRAYER_DETECT_EVIL:
		{
			(void) detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case PRAYER_CURE_LIGHT_WOUNDS:
		{
			(void) hp_player(damroll(2, plev / 4 + 5));
			(void) dec_timed(TMD_CUT, 10, TRUE);
			break;
		}
	case PRAYER_BLESS:
		{
			if (!p_ptr->timed[TMD_BLESSED]) {
				(void) inc_timed(TMD_BLESSED, randint1(12) + 12, TRUE);
			} else {
				(void) inc_timed(TMD_BLESSED, randint1(4) + 4, TRUE);
			}
			break;
		}
	case PRAYER_REMOVE_FEAR:
		{
			(void) clear_timed(TMD_AFRAID, TRUE);
			break;
		}
	case PRAYER_CALL_LIGHT:
		{
			(void) light_area(damroll(2, 1 + (plev / 3)), (plev / 10) + 1);
			break;
		}
	case PRAYER_FIND_TRAPS:
		{
			(void) detect_traps(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case PRAYER_DETECT_DOORS_STAIRS:
		{
			/* Hack - 'show' effected region only with the first detect */
			(void) detect_doors(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, FALSE);
			break;
		}
	case PRAYER_SLOW_POISON:
		{
			(void) dec_timed(TMD_POISONED, p_ptr->timed[TMD_POISONED] / 2,
							 TRUE);
			break;
		}
	case PRAYER_CURE_SERIOUS_WOUNDS:
		{
			(void) hp_player(damroll(4, plev / 4 + 6));
			(void) dec_timed(TMD_CUT, p_ptr->timed[TMD_CUT] + 5, TRUE);
			break;
		}
	case PRAYER_SCARE_MONSTER:
		{
			(void) fear_monster(dir, (3 * plev / 2) + 10);
			break;
		}
	case PRAYER_PORTAL:
		{
			teleport_player(2 * plev, TRUE);
			break;
		}
	case PRAYER_CHANT:
		{
			if (!p_ptr->timed[TMD_BLESSED]) {
				(void) inc_timed(TMD_BLESSED, randint1(24) + 24, TRUE);
			} else {
				(void) inc_timed(TMD_BLESSED, randint1(8) + 8, TRUE);
			}
			break;
		}
	case PRAYER_SANCTUARY:
		{
			(void) sleep_monsters_touch(plev + 25);
			break;
		}
	case PRAYER_SATISFY_HUNGER:
		{
			(void) set_food(PY_FOOD_MAX - 1);
			break;
		}
	case PRAYER_REMOVE_CURSE:
		{
			if (remove_curse())
				msg("You feel kindly hands aiding you.");
			break;
		}
	case PRAYER_RESIST_HEAT_AND_COLD:
		{
			(void) inc_timed(TMD_OPP_FIRE, randint1(10) + plev / 2, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(10) + plev / 2, TRUE);
			break;
		}
	case PRAYER_NEUTRALIZE_POISON:
		{
			(void) clear_timed(TMD_POISONED, TRUE);
			break;
		}
	case PRAYER_ORB_OF_DRAINING:
		{
			fire_sphere(GF_HOLY_ORB, dir,
						(damroll(3, 6) + plev / 4 +
						 (plev / ((player_has(PF_STRONG_MAGIC)) ? 2 : 4))),
						((plev < 30) ? 1 : 2), 30);
			break;
		}
	case PRAYER_SENSE_INVISIBLE:
		{
			(void) inc_timed(TMD_SINVIS, randint1(24) + plev, TRUE);
			break;
		}
	case PRAYER_PROTECTION_FROM_EVIL:
		{
			if (!p_ptr->timed[TMD_PROTEVIL]) {
				(void) inc_timed(TMD_PROTEVIL, randint1(24) + 3 * plev / 2,
								 TRUE);
			} else {
				(void) inc_timed(TMD_PROTEVIL, randint1(30), TRUE);
			}
			break;
		}
	case PRAYER_CURE_MORTAL_WOUNDS:
		{
			(void) hp_player(damroll(9, plev / 3 + 12));
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			break;
		}
	case PRAYER_EARTHQUAKE:
		{
			earthquake(py, px, 10, FALSE);
			break;
		}
	case PRAYER_SENSE_SURROUNDINGS:
		{
			/* Extended area for high-level Rangers. */
			if ((player_has(PF_LORE)) && (plev > 34))
				map_area(0, 0, TRUE);
			else
				map_area(0, 0, FALSE);
			break;
		}
	case PRAYER_TURN_UNDEAD:
		{
			(void) turn_undead((3 * plev / 2) + 10);
			break;
		}
	case PRAYER_PRAYER:
		{
			if (!p_ptr->timed[TMD_BLESSED]) {
				(void) inc_timed(TMD_BLESSED, randint1(48) + 48, TRUE);
			} else {
				(void) inc_timed(TMD_BLESSED, randint1(12) + 12, TRUE);
			}
			break;
		}
	case PRAYER_DISPEL_UNDEAD:
		{
			(void) dispel_undead(randint1(plev * 3));
			break;
		}
	case PRAYER_HEAL:
		{
			(void) hp_player(300);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			break;
		}
	case PRAYER_DISPEL_EVIL:
		{
			(void) dispel_evil(randint1(plev * 3));
			break;
		}
	case PRAYER_SACRED_SHIELD:
		{
			if (!p_ptr->timed[TMD_SHIELD]) {
				(void) inc_timed(TMD_SHIELD, randint1(20) + plev / 2,
								 TRUE);
			} else {
				(void) inc_timed(TMD_SHIELD, randint1(10) + plev / 4,
								 TRUE);
			}
			break;
		}
	case PRAYER_GLYPH_OF_WARDING:
		{
			(void) lay_rune(RUNE_PROTECT);
			break;
		}
	case PRAYER_HOLY_WORD:
		{
			(void) dispel_evil(randint1(plev * 4));
			(void) hp_player(300);
			(void) clear_timed(TMD_AFRAID, TRUE);
			(void) dec_timed(TMD_POISONED, 200, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			break;
		}
	case PRAYER_BLINK:
		{
			teleport_player(10, TRUE);
			break;
		}
	case PRAYER_TELEPORT_SELF:
		{
			teleport_player(plev * 4, TRUE);
			break;
		}
	case PRAYER_TELEPORT_OTHER:
		{
			(void) teleport_monster(dir, 45 + (plev / 3));
			break;
		}
	case PRAYER_TELEPORT_LEVEL:
		{
			(void) teleport_player_level(TRUE);
			break;
		}
	case PRAYER_WORD_OF_RECALL:
		{
			if (!word_recall(randint0(20) + 15))
				break;
			break;
		}
	case PRAYER_ALTER_REALITY:
		{
			if (MODE(IRONMAN))
				msg("Nothing happens.");
			else {
				msg("The world changes!");

				/* Leaving */
				p_ptr->leaving = TRUE;
			}

			break;
		}
	case PRAYER_DETECT_MONSTERS:
		{
			(void) detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case PRAYER_DETECTION:
		{
			(void) detect_all(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case PRAYER_PROBING:
		{
			(void) probing();
			break;
		}
	case PRAYER_PERCEPTION:
		{
			if (!ident_spell())
				return FALSE;
			break;
		}
	case PRAYER_CLAIRVOYANCE:
		{
			wiz_light(FALSE);
			break;
		}
	case PRAYER_BANISHMENT:
		{
			if (banish_evil(80)) {
				msg("The power of the Valar banishes evil!");
			}
			break;
		}
	case PRAYER_HEALING:
		{
			(void) hp_player(700);
			(void) clear_timed(TMD_STUN, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			break;
		}
	case PRAYER_SACRED_KNOWLEDGE:
		{
			(void) identify_fully();
			break;
		}
	case PRAYER_RESTORATION:
		{
			(void) do_res_stat(A_STR);
			(void) do_res_stat(A_INT);
			(void) do_res_stat(A_WIS);
			(void) do_res_stat(A_DEX);
			(void) do_res_stat(A_CON);
			(void) do_res_stat(A_CHR);
			break;
		}
	case PRAYER_REMEMBRANCE:
		{
			(void) restore_level();
			break;
		}
	case PRAYER_UNBARRING_WAYS:
		{
			(void) destroy_doors_touch();
			break;
		}
	case PRAYER_RECHARGING:
		{
			if (!recharge(140))
				return FALSE;
			break;
		}
	case PRAYER_DISPEL_CURSE:
		{
			if (remove_curse_good())
				msg("A beneficent force surrounds you for a moment.");
			break;
		}
	case PRAYER_DISARM_TRAP:
		{
			(void) disarm_trap(dir);
			break;
		}
	case PRAYER_HOLDING:
		{
			/* Spell will hold any monster or door in one square. */
			fire_ball(GF_HOLD, dir, 0, 0, FALSE);

			break;
		}
	case PRAYER_ENCHANT_WEAPON_OR_ARMOUR:
		{
			keycode_t answer;

			/* Query */
			msg("Would you like to enchant a 'W'eapon or 'A'rmour");

			/* Buttons */
			button_add("a", 'a');
			button_add("w", 'w');

			/* Interact and enchant. */
			while (1) {
				ui_event ke;

				ke = inkey_ex();
				answer = ke.key.code;

				if ((answer == 'W') || (answer == 'w')) {
					(void) enchant_spell(randint0(4) + 1, randint0(4) + 1,
										 0);
					break;
				} else if ((answer == 'A') || (answer == 'a')) {
					(void) enchant_spell(0, 0, randint0(3) + 2);
					break;
				} else if (answer == ESCAPE) {
					button_kill('w');
					button_kill('a');
					return FALSE;
				}
			}
			button_kill('w');
			button_kill('a');

			break;
		}
	case PRAYER_LIGHT_OF_MANWE:
		{
			fire_ball(GF_LIGHT, dir, plev * 2, 3, FALSE);
			break;
		}
	case PRAYER_LANCE_OF_OROME:
		{
			fire_beam(GF_HOLY_ORB, dir, 3 * plev / 2);
			break;
		}
	case PRAYER_HAMMER_OF_AULE:
		{
			destroy_area(py, px, 15, TRUE);
			break;
		}

	case PRAYER_STRIKE_OF_MANDOS:
		{
			drain_life(dir, plev * 3 + randint1(100));
			break;
		}
	case PRAYER_CALL_ON_VARDA:
		{
			msg("Gilthoniel A Elbereth!");
			fire_sphere(GF_LIGHT, 0, plev * 5, plev / 7 + 2, 20);
			(void) fear_monsters(plev * 2);
			(void) hp_player(500);
			break;
		}

	case PRAYER_ELEMENTAL_INFUSION:
		{
			if (!choose_ele_attack())
				return FALSE;
			break;
		}
	case PRAYER_SANCTIFY_FOR_BATTLE:
		{
			/* Can't have black breath and holy attack (doesn't happen anyway) */
			if (p_ptr->special_attack & ATTACK_BLKBRTH)
				p_ptr->special_attack &= ~ATTACK_BLKBRTH;

			if (!(p_ptr->special_attack & ATTACK_HOLY))
				msg("Your blows will strike with Holy might!");
			p_ptr->special_attack |= (ATTACK_HOLY);

			/* Redraw the state */
			p_ptr->redraw |= (PR_STATUS);

			break;
		}
	case PRAYER_HORN_OF_WRATH:
		{
			(void) hp_player(20);
			if (!p_ptr->timed[TMD_HERO]) {
				(void) inc_timed(TMD_HERO, randint1(20) + 20, TRUE);
			} else {
				(void) inc_timed(TMD_HERO, randint1(10) + 10, TRUE);
			}
			(void) clear_timed(TMD_AFRAID, TRUE);

			(void) fear_monsters(plev);
			break;
		}
	case SPELL_HIT_AND_RUN:
		{
			p_ptr->special_attack |= (ATTACK_FLEE);

			/* Redraw the state */
			p_ptr->redraw |= (PR_STATUS);

			break;
		}
	case SPELL_DAY_OF_MISRULE:
		{
			const char *p =
				(p_ptr->psex == SEX_FEMALE ? "Daughters" : "Sons");
			msg(" of Night rejoice!  It's the Day of Misrule!", p);
			(void) inc_timed(TMD_FAST, randint1(30) + 30, TRUE);
			(void) inc_timed(TMD_SHERO, randint1(30) + 30, TRUE);
			break;
		}
	case SPELL_DETECT_TREASURE:
		{
			/* Hack - 'show' affected region only with the first detect */
			(void) detect_treasure(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_objects_gold(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_objects_normal(DETECT_RAD_DEFAULT, TRUE);
			break;
		}


		/* Nature Magics */

	case LORE_DETECT_LIFE:
		{
			(void) detect_monsters_living(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case LORE_CALL_LIGHT:
		{
			(void) light_area(damroll(2, 1 + (plev / 4)), (plev / 10) + 1);
			break;
		}
	case LORE_FORAGING:
		{
			(void) set_food(PY_FOOD_MAX - 1);
			break;
		}
	case LORE_BLINK:
		{
			teleport_player(10, TRUE);
			break;
		}
	case LORE_COMBAT_POISON:
		{
			(void) dec_timed(TMD_POISONED, p_ptr->timed[TMD_POISONED] / 2,
							 TRUE);
			break;
		}
	case LORE_LIGHTNING_SPARK:
		{
			fire_arc(GF_ELEC, dir, damroll(2 + (plev / 8), 6),
					 (1 + plev / 5), 0);
			break;
		}
	case LORE_DOOR_DESTRUCION:
		{
			(void) destroy_doors_touch();
			break;
		}
	case LORE_TURN_STONE_TO_MUD:
		{
			(void) wall_to_mud(dir);
			break;
		}
	case LORE_RAY_OF_SUNLIGHT:
		{
			msg("A ray of golden yellow light appears.");
			light_line(dir);
			break;
		}
	case LORE_CURE_POISON:
		{
			(void) clear_timed(TMD_POISONED, TRUE);
			break;
		}
	case LORE_FROST_BOLT:
		{
			fire_bolt_or_beam(beam - 10, GF_COLD, dir,
							  damroll(2 + (plev / 5), 8));
			break;
		}
	case LORE_SLEEP_CREATURE:
		{
			(void) sleep_monster(dir, plev + 10);
			break;
		}
	case LORE_FRIGHTEN_CREATURE:
		{
			(void) fear_monster(dir, plev + 10);
			break;
		}
	case LORE_DETECT_TRAPS_DOORS:
		{
			(void) detect_traps(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_doors(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case LORE_SNUFF_SMALL_LIFE:
		{
			(void) dispel_small_monsters(2 + plev / 5);
			break;
		}
	case LORE_FIRE_BOLT:
		{
			fire_bolt_or_beam(beam - 10, GF_FIRE, dir,
							  damroll(3 + (plev / 5), 8));
			break;
		}
	case LORE_HEROISM:
		{
			(void) hp_player(20);
			if (!p_ptr->timed[TMD_HERO]) {
				(void) inc_timed(TMD_HERO, randint1(20) + 20, TRUE);
			} else {
				(void) inc_timed(TMD_HERO, randint1(10) + 10, TRUE);
			}
			(void) clear_timed(TMD_AFRAID, TRUE);

			break;
		}
	case LORE_REMOVE_CURSE:
		{
			if (remove_curse())
				msg("You feel tender hands aiding you.");
			break;
		}
	case LORE_ACID_BOLT:
		{
			fire_bolt_or_beam(beam - 10, GF_ACID, dir,
							  damroll(5 + (plev / 5), 8));
			break;
		}
	case LORE_TELEPORT_MONSTER:
		{
			(void) teleport_monster(dir, 45 + (plev / 3));
			break;
		}
	case LORE_GRAVITY_BOLT:
		{
			fire_bolt_or_beam(beam - 10, GF_GRAVITY, dir,
							  damroll(5 + (plev / 4), 8));
			break;
		}
	case LORE_RESIST_POISON:
		{
			(void) inc_timed(TMD_OPP_POIS, randint1(20) + 20, TRUE);
			break;
		}
	case LORE_EARTHQUAKE:
		{
			earthquake(py, px, 10, FALSE);
			break;
		}
	case LORE_RESIST_FIRE_AND_COLD:
		{
			(void) inc_timed(TMD_OPP_FIRE, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			break;
		}
	case LORE_DETECT_ALL:
		{
			(void) detect_all(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case LORE_NATURAL_VITALITY:
		{
			(void) dec_timed(TMD_POISONED,
							 (3 * p_ptr->timed[TMD_POISONED] / 4) + 5,
							 TRUE);
			(void) hp_player(damroll(2, plev / 5));
			(void) dec_timed(TMD_CUT, p_ptr->timed[TMD_CUT] + plev / 2,
							 TRUE);
			break;
		}
	case LORE_RESIST_ACID_AND_LIGHTNING:
		{
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_ELEC, randint1(20) + 20, TRUE);
			break;
		}
	case LORE_WITHER_FOE:
		{
			fire_bolt(GF_HOLY_ORB, dir, damroll(plev / 7, 8));
			(void) confuse_monster(dir, plev + 10);
			(void) slow_monster(dir, plev + 10);
			break;
		}
	case LORE_DISARM_TRAP:
		{
			(void) disarm_trap(dir);
			break;
		}
	case LORE_IDENTIFY:
		{
			if (!ident_spell())
				return FALSE;
			break;
		}
	case LORE_CREATE_ATHELAS:
		{
			(void) create_athelas();
			break;
		}
	case LORE_RAGING_STORM:
		{
			fire_ball(GF_ELEC, dir, plev + randint1(60 + plev * 2),
					  (1 + plev / 15), FALSE);
			break;
		}
	case LORE_THUNDERCLAP:
		{
			msg("Boom!");
			fire_sphere(GF_SOUND, 0, plev + randint1(40 + plev * 2),
						plev / 8, 20);
			break;
		}
	case LORE_BECOME_MOUSE:
		{
			shape = SHAPE_MOUSE;
			break;
		}
	case LORE_BECOME_FERRET:
		{
			shape = SHAPE_FERRET;
			break;
		}
	case LORE_BECOME_HOUND:
		{
			shape = SHAPE_HOUND;
			break;
		}
	case LORE_BECOME_GAZELLE:
		{
			shape = SHAPE_GAZELLE;
			break;
		}
	case LORE_BECOME_LION:
		{
			shape = SHAPE_LION;
			break;
		}
	case LORE_DETECT_EVIL:
		{
			(void) detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case LORE_SONG_OF_FRIGHTENING:
		{
			(void) fear_monsters(3 * plev / 2 + 10);
			break;
		}
	case LORE_SENSE_SURROUNDINGS:
		{
			map_area(0, 0, FALSE);
			break;
		}
	case LORE_SIGHT_BEYOND_SIGHT:
		{
			/* Hack - 'show' effected region only with the first detect */
			(void) detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_monsters_invis(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_traps(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_doors(DETECT_RAD_DEFAULT, FALSE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, FALSE);
			if (!p_ptr->timed[TMD_SINVIS]) {
				(void) inc_timed(TMD_SINVIS, randint1(24) + 24, TRUE);
			} else {
				(void) inc_timed(TMD_SINVIS, randint1(12) + 12, TRUE);
			}
			break;
		}
	case LORE_HERBAL_HEALING:
		{
			(void) hp_player(damroll(25 + plev / 2, 12));
			(void) clear_timed(TMD_CUT, TRUE);
			(void) dec_timed(TMD_POISONED, 200, TRUE);
			break;
		}
	case LORE_BLIZZARD:
		{
			fire_ball(GF_COLD, dir, plev + randint1(50 + plev * 2),
					  1 + plev / 12, FALSE);
			break;
		}
	case LORE_TRIGGER_TSUNAMI:
		{
			msg("You hurl mighty waves at your foes!");
			fire_sphere(GF_WATER, 0,
						30 + ((4 * plev) / 5) + randint1(30 + plev * 2),
						plev / 7, 20);
			break;
		}
	case LORE_VOLCANIC_ERUPTION:
		{
			msg("The earth convulses and erupts in fire!");
			fire_sphere(GF_FIRE, 0, 3 * plev / 2 + randint1(50 + plev * 3),
						1 + plev / 15, 20);
			earthquake(py, px, plev / 5, TRUE);
			break;
		}
	case LORE_MOLTEN_LIGHTNING:
		{
			fire_ball(GF_PLASMA, dir,
					  35 + (2 * plev) + randint1(90 + plev * 4), 1, FALSE);
			break;
		}
	case LORE_STARBURST:
		{
			msg("Light bright beyond enduring dazzles your foes!");
			fire_sphere(GF_LIGHT, 0,
						40 + (3 * plev / 2) + randint1(plev * 3),
						plev / 10, 20);
			break;
		}
	case LORE_SONG_OF_LULLING:
		{
			msg("Your tranquil music enchants those nearby.");

			(void) slow_monsters(5 * plev / 3);
			(void) sleep_monsters(5 * plev / 3);
			break;
		}
	case LORE_SONG_OF_PROTECTION:
		{
			msg("Your song creates a mystic shield.");
			if (!p_ptr->timed[TMD_SHIELD]) {
				(void) inc_timed(TMD_SHIELD, randint1(30) + plev / 2,
								 TRUE);
			} else {
				(void) inc_timed(TMD_SHIELD, randint1(15) + plev / 4,
								 TRUE);
			}
			break;
		}
	case LORE_SONG_OF_DISPELLING:
		{
			msg("An unbearable discord tortures your foes!");

			(void) dispel_monsters(randint1(plev * 2));
			(void) dispel_evil(randint1(plev * 2));
			break;
		}
	case LORE_SONG_OF_WARDING:
		{
			msg("Your song creates a place of sanctuary.");

			(void) lay_rune(RUNE_PROTECT);
			break;
		}
	case LORE_SONG_OF_RENEWAL:
		{
			msg("Amidst the gloom, you invoke light and beauty; your body regains its natural vitality.");

			(void) do_res_stat(A_STR);
			(void) do_res_stat(A_INT);
			(void) do_res_stat(A_WIS);
			(void) do_res_stat(A_DEX);
			(void) do_res_stat(A_CON);
			(void) do_res_stat(A_CHR);
			(void) restore_level();
			break;
		}
	case LORE_WEB_OF_VAIRE:
		{
			fire_bolt_or_beam(plev * 2, GF_TIME, dir,
							  damroll(plev / 6, 8));
			break;
		}
	case LORE_SPEED_OF_NESSA:
		{
			if (!p_ptr->timed[TMD_FAST]) {
				(void) set_timed(TMD_FAST, randint1(10) + plev / 2, TRUE);
			} else {
				(void) set_timed(TMD_FAST, randint1(5), TRUE);
			}
			break;
		}
	case LORE_RENEWAL_OF_VANA:
		{
			if (!recharge(125))
				return FALSE;
			break;
		}
	case LORE_SERVANT_OF_YAVANNA:
		{
			shape = SHAPE_ENT;
			break;
		}
	case LORE_TEARS_OF_NIENNA:
		{
			(void) restore_level();
			break;
		}
	case LORE_HEALING_OF_ESTE:
		{
			(void) dispel_evil(100);
			(void) hp_player(500);
			(void) inc_timed(TMD_BLESSED, randint1(100) + 100, TRUE);
			(void) clear_timed(TMD_CUT, TRUE);
			(void) clear_timed(TMD_AFRAID, TRUE);
			(void) clear_timed(TMD_POISONED, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			break;
		}
	case LORE_CREATURE_KNOWLEDGE:
		{
			msg("Target the monster you wish to learn about.");
			if (!get_aim_dir(&dir))
				return FALSE;
			pseudo_probe();
			break;
		}
	case LORE_NATURES_VENGEANCE:
		{
			nature_strike(damroll(6, 6) + plev / 3);
			break;
		}
	case LORE_SONG_OF_GROWTH:
		{
			grow_trees_and_grass(FALSE);
			break;
		}
	case LORE_SONG_OF_PRESERVATION:
		{
			bitflag flags[OF_SIZE];
			of_on(flags, OF_ACID_PROOF);
			of_on(flags, OF_FIRE_PROOF);

			if (!el_proof(flags))
				return FALSE;
			break;
		}
	case LORE_TREMOR:
		{
			if (!tremor())
				return FALSE;
			break;
		}

		/* Necromantic Spells */

	case RITUAL_NETHER_BOLT:
		{
			fire_bolt(GF_NETHER, dir, damroll(2, 5 + plev / 7));
			break;
		}
	case RITUAL_DETECT_EVIL:
		{
			(void) detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case RITUAL_ENHANCED_INFRAVISION:
		{
			inc_timed(TMD_SINFRA, 70 + randint1(70), TRUE);
			break;
		}
	case RITUAL_BREAK_CURSE:
		{
			if (remove_curse())
				msg("You feel mighty hands aiding you.");
			break;
		}
	case RITUAL_SLOW_MONSTER:
		{
			(void) slow_monster(dir, plev + 5);
			break;
		}
	case RITUAL_SLEEP_MONSTER:
		{
			(void) sleep_monster(dir, plev + 5);
			break;
		}
	case RITUAL_HORRIFY:
		{
			(void) fear_monster(dir, plev + 15);
			break;
		}
	case RITUAL_BECOME_BAT:
		{
			take_hit(damroll(2, 4), "shapeshifting stress");
			shape = SHAPE_BAT;
			break;
		}
	case RITUAL_DOOR_DESTRUCTION:
		{
			(void) destroy_doors_touch();
			break;
		}
	case RITUAL_DARK_BOLT:
		{
			fire_bolt_or_beam(beam - 10, GF_DARK, dir,
							  damroll((3 + plev / 7), 8));
			break;
		}
	case RITUAL_NOXIOUS_FUMES:
		{
			fire_sphere(GF_POIS, 0, 10 + plev, 2 + plev / 12, 40);

			pois_hit(5);
			break;
		}
	case RITUAL_TURN_UNDEAD:
		{
			(void) turn_undead(3 * plev / 2);
			break;
		}
	case RITUAL_TURN_EVIL:
		{
			(void) turn_evil(5 * plev / 4);
			break;
		}
	case RITUAL_CURE_POISON:
		{
			(void) clear_timed(TMD_POISONED, TRUE);
			break;
		}
	case RITUAL_DISPEL_UNDEAD:
		{
			(void) dispel_undead(plev + 15 + randint1(3 * plev / 2));
			break;
		}
	case RITUAL_DISPEL_EVIL:
		{
			(void) dispel_evil(plev + randint1(plev));
			break;
		}
	case RITUAL_SEE_INVISIBLE:
		{
			if (!p_ptr->timed[TMD_SINVIS]) {
				inc_timed(TMD_SINVIS, 20 + randint1(plev / 2), TRUE);
			} else {
				inc_timed(TMD_SINVIS, 10 + randint1(plev / 4), TRUE);
			}
			break;
		}
	case RITUAL_SHADOW_SHIFTING:
		{
			take_hit(damroll(1, 4), "shadow dislocation");
			teleport_player(16, TRUE);
			break;
		}
	case RITUAL_DETECT_TRAPS:
		{
			(void) detect_traps(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case RITUAL_DETECT_DOORS_STAIRS:
		{
			/* Hack - 'show' effected region only with the first detect */
			(void) detect_doors(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_stairs(DETECT_RAD_DEFAULT, FALSE);
			break;
		}
	case RITUAL_SLEEP_MONSTERS:
		{
			(void) sleep_monsters(plev + 5);
			break;
		}
	case RITUAL_SLOW_MONSTERS:
		{
			(void) slow_monsters(plev + 5);
			break;
		}
	case RITUAL_DETECT_MAGIC:
		{
			(void) detect_objects_magic(DETECT_RAD_DEFAULT, TRUE);
			break;
		}
	case RITUAL_DEATH_BOLT:
		{
			take_hit(damroll(1, 6), "the dark arts");
			fire_bolt_or_beam(beam, GF_SPIRIT, dir,
							  damroll(2 + plev / 3, 8));
			break;
		}
	case RITUAL_RESIST_POISON:
		{
			(void) inc_timed(TMD_OPP_POIS, randint1(20) + plev / 2, TRUE);
			break;
		}
	case RITUAL_EXORCISE_DEMONS:
		{
			(void) dispel_demons(2 * plev + randint1(2 * plev));
			break;
		}
	case RITUAL_DARK_SPEAR:
		{
			fire_beam(GF_DARK, dir, 15 + 7 * plev / 4);
			break;
		}
	case RITUAL_CHAOS_STRIKE:
		{
			fire_bolt_or_beam(beam, GF_CHAOS, dir,
							  damroll(1 + plev / 2, 8));
			break;
		}
	case RITUAL_GENOCIDE:
		{
			(void) genocide();
			break;
		}
	case RITUAL_DARK_BALL:
		{
			fire_ball(GF_DARK, dir, 50 + plev * 2, 2, FALSE);
			break;
		}
	case RITUAL_STENCH_OF_DEATH:
		{
			take_hit(damroll(2, 8), "the stench of Death");
			(void) dispel_living(50 + randint1(plev));
			confu_monsters(plev + 10);
			fire_sphere(GF_POIS, dir, plev * 2, 5 + plev / 11, 40);
			break;
		}
	case RITUAL_PROBING:
		{
			(void) probing();
			break;
		}
	case RITUAL_SHADOW_MAPPING:
		{
			map_area(0, 0, FALSE);
			break;
		}
	case RITUAL_IDENTIFY:
		{
			if (!ident_spell())
				return FALSE;
			break;
		}
	case RITUAL_SHADOW_WARPING:
		{
			take_hit(damroll(2, 6), "shadow dislocation");
			teleport_player(plev * 3, TRUE);
			break;
		}
	case RITUAL_POISON_AMMO:
		{
			if (!brand_missile(0, EGO_POISON))
				return FALSE;
			break;
		}
	case RITUAL_RESIST_ACID_AND_COLD:
		{
			(void) inc_timed(TMD_OPP_ACID, randint1(20) + 20, TRUE);
			(void) inc_timed(TMD_OPP_COLD, randint1(20) + 20, TRUE);
			break;
		}
	case RITUAL_HEAL_ANY_WOUND:
		{
			(void) clear_timed(TMD_CUT, TRUE);
			(void) clear_timed(TMD_STUN, TRUE);
			break;
		}
	case RITUAL_PROTECTION_FROM_EVIL:
		{
			(void) inc_timed(TMD_PROTEVIL, plev / 2 + randint1(plev),
							 TRUE);
			break;
		}
	case RITUAL_BLACK_BLESSING:
		{
			(void) inc_timed(TMD_BLESSED, randint1(66), TRUE);
			break;
		}
	case RITUAL_BANISH_EVIL:
		{
			(void) banish_evil(80);
			break;
		}
	case RITUAL_SHADOW_BARRIER:
		{
			if (!p_ptr->timed[TMD_SHIELD]) {
				(void) inc_timed(TMD_SHIELD, randint1(20) + 10, TRUE);
			} else {
				(void) inc_timed(TMD_SHIELD, randint1(10) + 5, TRUE);
			}
			break;
		}
	case RITUAL_DETECT_ALL_MONSTERS:
		{
			/* Hack - 'show' affected region only with the first detect */
			(void) detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
			(void) detect_monsters_invis(DETECT_RAD_DEFAULT, FALSE);
			break;
		}
	case RITUAL_STRIKE_AT_LIFE:
		{
			fire_bolt(GF_NETHER, dir, damroll(3 * plev / 5, 13));
			break;
		}
	case RITUAL_ORB_OF_DEATH:
		{
			take_hit(damroll(2, 8), "Death claiming his wages");
			fire_sphere(GF_SPIRIT, dir, 20 + (4 * plev), 1, 20);
			break;
		}
	case RITUAL_DISPEL_LIFE:
		{
			(void) dispel_living(60 + damroll(4, plev));
			break;
		}
	case RITUAL_VAMPIRIC_DRAIN:
		{
			fire_bolt(GF_SPIRIT, dir, damroll(plev / 3, 11));
			(void) hp_player(3 * plev);
			(void) set_food(p_ptr->food + 1000);
			break;
		}
	case RITUAL_RECHARGING:
		{
			if (!recharge(140))
				return FALSE;
			break;
		}
	case RITUAL_BECOME_WEREWOLF:
		{
			take_hit(damroll(2, 7), "shapeshifting stress");
			shape = SHAPE_WEREWOLF;
			break;
		}
	case RITUAL_DISPEL_CURSE:
		{
			if (remove_curse_good())
				msg("You feel mighty hands aiding you.");
			break;
		}
	case RITUAL_BECOME_VAMPIRE:
		{
			take_hit(damroll(3, 6), "shapeshifting stress");
			shape = SHAPE_VAMPIRE;
			break;
		}
	case RITUAL_HASTE_SELF:
		{
			if (!p_ptr->timed[TMD_FAST]) {
				(void) set_timed(TMD_FAST, 10 + randint1(20), TRUE);
			} else {
				(void) set_timed(TMD_FAST, randint1(5), TRUE);
			}
			break;
		}
	case RITUAL_PREPARE_BLACK_BREATH:
		{
			/* Can't have black breath and holy attack (doesn't happen anyway) */
			if (p_ptr->special_attack & ATTACK_HOLY)
				p_ptr->special_attack &= ~ATTACK_HOLY;

			if (!(p_ptr->special_attack & ATTACK_BLKBRTH))
				msg("Your hands start to radiate Night.");
			p_ptr->special_attack |= (ATTACK_BLKBRTH);

			/* Redraw the state */
			p_ptr->redraw |= (PR_STATUS);

			break;
		}
	case RITUAL_WORD_OF_DESTRUCTION:
		{
			destroy_area(py, px, 15, TRUE);
			break;
		}
	case RITUAL_TELEPORT_AWAY:
		{
			(void) teleport_monster(dir, 45 + (plev / 3));
			break;
		}
	case RITUAL_SMASH_UNDEAD:
		{
			(void) dispel_undead(plev * 3 + randint1(50));
			break;
		}
	case RITUAL_BIND_UNDEAD:
		{
			(void) hold_undead();
			break;
		}
	case RITUAL_DARKNESS_STORM:
		{
			fire_ball(GF_DARK, dir, 11 * plev / 2, plev / 7, FALSE);
			break;
		}
	case RITUAL_MENTAL_AWARENESS:
		{
			if (!p_ptr->timed[TMD_TELEPATHY]) {
				(void) inc_timed(TMD_TELEPATHY, 30 + randint1(40), TRUE);
			} else {
				(void) inc_timed(TMD_TELEPATHY, randint1(30), TRUE);
			}
			break;
		}
	case RITUAL_SLIP_INTO_THE_SHADOWS:
		{
			if (!p_ptr->timed[TMD_SSTEALTH]) {
				(void) inc_timed(TMD_SSTEALTH, 40, TRUE);
			} else {
				(void) inc_timed(TMD_SSTEALTH, randint1(20), TRUE);
			}
			break;
		}
	case RITUAL_BLOODWRATH:
		{
			if (!p_ptr->timed[TMD_SHERO]) {
				(void) inc_timed(TMD_SHERO, 40, TRUE);
			} else {
				(void) inc_timed(TMD_SHERO, randint1(20), TRUE);
			}

			(void) set_timed(TMD_FAST, 40, TRUE);

			break;
		}
	case RITUAL_REBALANCE_WEAPON:
		{
			rebalance_weapon();
			break;
		}
	case RITUAL_DARK_POWER:
		{
			if (p_ptr->csp < p_ptr->msp) {
				take_hit(75, "dark power");
				p_ptr->csp += 3 * plev / 2;
				p_ptr->csp_frac = 0;
				if (p_ptr->csp > p_ptr->msp)
					(p_ptr->csp = p_ptr->msp);
				msg("You feel flow of power.");
				p_ptr->redraw |= (PR_MANA);
			}
			break;
		}

	default:					/* No Spell */
		{
			msg("Undefined Spell");
			break;
		}
	}

	/* Alter shape, if necessary. */
	if (shape)
		shapechange(shape);

	/* Success */
	return (TRUE);

}
