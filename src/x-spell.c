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
#include "effects.h"
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
#define LORE_SONG_OF_PRESEERVATION              190
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



cptr get_spell_name(int tval, int spell)
{
		return spell_names[spell];
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
void get_spell_info(int tval, int spell, char *p, size_t len)
{
  int plev = p_ptr->lev;
  
  int beam, beam_low;
  
  /* Specialty Ability */
  if (check_ability(SP_HEIGHTEN_MAGIC)) 
    plev += 1 + ((p_ptr->heighten_power + 5)/ 10);
  if (check_ability(SP_CHANNELING)) plev += get_channeling_boost();
  
  /* Beaming chance */
  beam = (((check_ability(SP_BEAM)))  ? plev : (plev / 2));
  beam_low = (beam - 10 > 0 ? beam - 10 : 0);
  
  /* Default */
  strcpy(p, "");
  
  /* Analyze the spell */
  switch (spell)
    {
      /* Sorcery */
      
    case 0: strnfmt(p, len, " dam %dd3", 4 + plev / 10); break;
    case 2: strnfmt(p, len, " range 10"); break;
    case 4: strnfmt(p, len, " dam 2d%d, rad %d", 
		    1 + (plev / 5), ( plev / 10) + 1); break;
    case 5: strnfmt(p, len, " dam %d, rad 2", 5 + (plev / 3)); break;
    case 7: strnfmt(p, len, " dur 10+d5"); break; 
    case 9: strnfmt(p, len, " dam %dd8, beam %d%%", (2+((plev-5)/5)), beam); 
      break;
    case 13: strnfmt(p, len, " range %d", 50 + plev * 2); break;
    case 15: strnfmt(p, len, " dam %d", 5 + plev); break;
    case 20: strnfmt(p, len, " range %d", 5 + plev/10); break;
    case 26: strnfmt(p, len, " dam %dd%d, length %d", 6 + (plev / 10), 8, 1 + (plev / 10)); 
      break;
    case 27: strnfmt(p, len, " dam %d, rad 2", 55 + plev); break;
    case 30: strnfmt(p, len, " dist %d", 45 + (plev/2)); break;
    case 34: strnfmt(p, len, " dur %d+d%d", plev, plev); break;
    case 35: strnfmt(p, len, " dur 30+d20"); break;
    case 36: strnfmt(p, len, " dur 20+d20"); break;
    case 37: strnfmt(p, len, " dur %d+d30", 10+plev); break;
    case 38: strnfmt(p, len, " dur 40"); break;
    case 45: strnfmt(p, len, " dam %dd8, beam %d%%", (3 + plev / 3), beam); break;
    case 49: strnfmt(p, len, " dam %d, rad %d", 5 * plev / 2, plev / 12); break;
    case 51: strnfmt(p, len, " dam %d,%d, rad 3,2", 10 + plev, 2 * plev); break;
    case 52: strnfmt(p, len, " dam %d, rad 3", 3 * plev); break;
    case 53: strnfmt(p, len, " dam %d, rad 1", 80 + plev * 2); break;
    case 54: strnfmt(p, len, " dam %d+3d%d", plev, plev); break;
    case 55: strnfmt(p, len, " dam %d, rad 1", 80 + plev * 2); break;
    case 56: strnfmt(p, len, " dam %d, rad %d", 4 * plev, 3 + plev / 15); break;

      
      /* Piety */
    case 65: strnfmt(p, len, " heal 2d%d", plev / 4 + 5); break;
    case 66: strnfmt(p, len, " dur 12+d12"); break;
    case 68: strnfmt(p, len, " dam 2d%d, rad %d", 1 + plev/3, plev/10+1); break;
    case 71: strnfmt(p, len, " halve poison"); break;
    case 72: strnfmt(p, len, " heal 4d%d", plev / 4 + 6); break;
    case 74: strnfmt(p, len, " range %d", 2 * plev); break;
    case 75: strnfmt(p, len, " dur 24+d24"); break;
    case 79: strnfmt(p, len, " dur %d+d10", plev / 2); break;
    case 81: strnfmt(p, len, " dam %d+3d6, rad %d", plev / 4 + 
		     (plev / ((check_ability(SP_STRONG_MAGIC)) ? 2 : 4)), 
		     (plev < 30) ? 1 : 2); break;
    case 82: strnfmt(p, len, " dur %d+d24", plev); break;
    case 83: strnfmt(p, len, " dur %d+d24", 3 * plev / 2); break;
    case 84: strnfmt(p, len, " heal 9d%d, any cut", plev / 3 + 12); break;
    case 85: strnfmt(p, len, " radius 10"); break;
    case 88: strnfmt(p, len, " dur 48+d48"); break;
    case 89: strnfmt(p, len, " dam d%d", 3 * plev); break;
    case 90: strnfmt(p, len, " heal 300, any cut"); break;
    case 91: strnfmt(p, len, " dam d%d", 3 * plev); break;
    case 92: strnfmt(p, len, " dur %d+d20", plev / 2); break;
    case 94: strnfmt(p, len, " dam d%d, heal 300", plev * 4); break;
    case 95: strnfmt(p, len, " range 10"); break;
    case 96: strnfmt(p, len, " range %d", 4 * plev); break;
    case 97: strnfmt(p, len, " dist %d", 45 + (plev/3)); break;
    case 107: strnfmt(p, len, " heal 700, any cut"); break;
    case 117: strnfmt(p, len, " dam %d, rad 3", 2 * plev); break;
    case 118: strnfmt(p, len, " dam %d", 3 * plev / 2); break;
    case 120: strnfmt(p, len, " dam %d+d100", plev * 3); break;
    case 121: strnfmt(p, len, " dam %d, heal 500", plev * 5); break;
      
      
      /* Nature Magics */
      
    case 129: strnfmt(p, len, " dam 2d%d, rad %d", 1 + plev/4, plev/10+1); break;
    case 131: strnfmt(p, len, " range 10"); break;
    case 132: strnfmt(p, len, " halve poison"); break;
    case 133: strnfmt(p, len, " dam %dd6, length %d", 2+plev/8, 1+plev/5); break;
    case 135: strnfmt(p, len, " dam 20+d30"); break;
    case 136: strnfmt(p, len, " dam 4d5"); break;
    case 138: strnfmt(p, len, " dam %dd8, beam %d%%", 2+plev/5, beam_low); break;
    case 142: strnfmt(p, len, " dam %d", 2 + plev / 5); break;
    case 143: strnfmt(p, len, " dam %dd8, beam %d%%", 3 + plev / 5, beam_low); 
      break;
    case 144: strnfmt(p, len, " dur 20+d20"); break;
    case 146: strnfmt(p, len, " dam %dd8, beam %d%%", 5+plev/5, beam_low); break;
    case 147: strnfmt(p, len, " dist %d", 45 + (plev/3)); break;
    case 148: strnfmt(p, len, " dam %dd8, beam %d%%", 5+plev/4, beam_low); break;
    case 149: strnfmt(p, len, " dur 20+d20"); break;
    case 150: strnfmt(p, len, " radius 10"); break;
    case 151: strnfmt(p, len, " dur 20+d20"); break;
    case 153: strnfmt(p, len, " heal 2d%d, pois/cut", plev / 5); break;
    case 154: strnfmt(p, len, " dur 20+d20"); break;
    case 155: strnfmt(p, len, " dam %dd8, conf/slow", plev / 7); break;
    case 159: strnfmt(p, len, " dam %d+d%d, rad %d", plev, 60 + plev * 2, 
		      1 + plev / 15); break;
    case 160: strnfmt(p, len, " dam %d+d%d, rad %d", plev, 40 + plev * 2, 
		      plev / 8); break;
    case 169: strnfmt(p, len, " dur 24+d24"); break;
    case 170: strnfmt(p, len, " heal %dd12, any cut", 25 + plev / 2); break;
    case 171: strnfmt(p, len, " dam %d+d%d, rad %d", plev, 50 + plev * 2, 
		      1 + plev / 12); break;
    case 172: strnfmt(p, len, " dam %d+d%d, rad %d", 30 + ((4 * plev) / 5), 
		      30 + plev * 2, plev / 7); break;
    case 173: strnfmt(p, len, " dam %d+d%d, rad %d", 3 * plev / 2, 50 + plev * 3, 
		      1 + plev / 15); break;
    case 174: strnfmt(p, len, " dam %d+d%d, rad 1", 35+(2*plev), 90+plev*4); break;
    case 175: strnfmt(p, len, " dam %d+d%d, rad %d", 40 + (3 * plev / 2), plev * 3, 
		      plev / 10); break;
    case 177: strnfmt(p, len, " dur %d+d30", plev / 2); break;
    case 178: strnfmt(p, len, " dam d%d", plev * 2); break;
    case 181: strnfmt(p, len, " dam %dd8, beam %d%%", plev / 6, plev * 2); break;
    case 182: strnfmt(p, len, " dur %d+d10", plev / 2); break;
    case 186: strnfmt(p, len, " heal 500, dam 100"); break;
    case 188: strnfmt(p, len, " dam 6d6+%d", plev/3); break;

      /* Necromantic Spells */
      
    case 192: strnfmt(p, len, " dam 2d%d", 5 + plev / 7); break;
    case 194: strnfmt(p, len, " dur 70+d70"); break;
    case 199: strnfmt(p, len, " hurt 2d4"); break;
    case 201: strnfmt(p, len, " dam %dd8, beam %d%%", 3+plev/7, beam_low); break;
    case 202: strnfmt(p, len, " dam %d, poison", 10 + plev); break;
    case 206: strnfmt(p, len, " dam %d+d%d", plev + 15, 3 * plev / 2); break;
    case 207: strnfmt(p, len, " dam %d+d%d", plev, plev); break;
    case 208: strnfmt(p, len, " dur 20+d%d", plev / 2); break;
    case 209: strnfmt(p, len, " range 16, hurt 1d4"); break;
    case 215: strnfmt(p, len, " dam %dd8, hurt 1d6", 2 + plev / 3); break;
    case 216: strnfmt(p, len, " dur %d+d20", plev / 2); break;
    case 217: strnfmt(p, len, " dam %d+d%d", 2 * plev, 2 * plev); break;
    case 218: strnfmt(p, len, " dam %d", 15 + 7 * plev / 4); break;
    case 219: strnfmt(p, len, " dam %dd8, beam %d%%", 1 + plev / 2, beam); break;
    case 221: strnfmt(p, len, " dam %d, rad 2", 50 + plev * 2); break;
    case 222: strnfmt(p, len, " dam %d+d%d, hit ~9", 50 + plev * 2, plev); break;
    case 226: strnfmt(p, len, " range %d, hurt 2d6", plev * 3); break;
    case 228: strnfmt(p, len, " dur 20+d20"); break;
    case 230: strnfmt(p, len, " %d+d%d", plev / 2, plev); break;
    case 231: strnfmt(p, len, " dur d66"); break;
    case 233: strnfmt(p, len, " dur 10+d20"); break;
    case 235: strnfmt(p, len, " dam %dd13", 3 * plev / 5); break;
    case 236: strnfmt(p, len, " dam %d, hurt 2d8", 20 + (4 * plev)); break;
    case 237: strnfmt(p, len, " dam %d+4d%d", 60, plev); break;
    case 238: strnfmt(p, len, " dam %dd11, heal %d", plev / 3, 3 * plev); break;
    case 240: strnfmt(p, len, " hurt 2d7"); break;
    case 242: strnfmt(p, len, " hurt 3d6"); break;
    case 243: strnfmt(p, len, " dur 10+d20"); break;
    case 245: strnfmt(p, len, " radius 15"); break;
    case 246: strnfmt(p, len, " dist %d", 45 + (plev/3)); break;
    case 247: strnfmt(p, len, " dam %d+d50", plev * 3); break;
    case 249: strnfmt(p, len, " dam %d, rad %d", 11 * plev / 2, plev/7); break;
    case 250: strnfmt(p, len, " dur 30+d40"); break;
    case 251: strnfmt(p, len, " dur 40"); break;
    case 252: strnfmt(p, len, " dur 40"); break;
    case 254: strnfmt(p, len, " mana %d", 3 * plev / 2); break;
    }
}



bool spell_needs_aim(int tval, int spell)
{
  switch (spell)
    {
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


bool cast_spell(int tval, int index, int dir)
{
	int py = p_ptr->py;
	int px = p_ptr->px;

	int plev = p_ptr->lev;
	int shape = SHAPE_NORMAL;
	bool beam = FALSE;

      /* Hack -- higher chance of "beam" instead of "bolt" for mages 
       * and necros.
       */
      beam = ((check_ability(SP_BEAM)) ? plev : (plev / 2));
      
      /* Spell Effects.  Spells are mostly listed by realm, each using a 
       * block of 64 spell slots, but there are a certain number of spells 
       * that are used by more than one realm (this allows more neat class-
       * specific magics)
       */
      switch (index)
	{
	  /* Sorcerous Spells */
	  
	case 0:	/* Fire Bolt */
	  {
	    fire_bolt(GF_FIRE, dir,
		      damroll(4 + plev / 10, 3));
	    break;
	  }
	case 1:	/* Detect Monsters */
	  {
	    (void)detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 2:	/* Phase Door */
	  {
	    teleport_player(10, TRUE);
	    break;
	  }
	case 3: /* Detect Traps/Doors/Stairs */
	  {
	    /* Hack - 'show' effected region only with
	     * the first detect */
	    (void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	    (void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	    break;
	  }
	case 4:	/* Light Area */
	  {
	    (void)lite_area(damroll(2, 1 + (plev / 5)), (plev / 10) + 1);
	    break;
	  }
	case 5:	/* Stinking Cloud */
	  {
	    fire_ball(GF_POIS, dir, 5 + plev / 3, 2, FALSE);
	    break;
	  }
	case 6:	/* Reduce Cuts and Poison */
	  {
	    (void)set_poisoned(p_ptr->poisoned / 2);
	    (void)set_cut(p_ptr->cut / 2);
	    break;
	  }
	case 7:	/* Resist Magic */
	  {
	    if (!p_ptr->magicdef)
	      {
		(void)set_extra_defences(10 + randint1(5));
	      }
	    else
	      {
		(void)set_extra_defences(p_ptr->magicdef + randint1(5));
	      }
	    
	    break;
	  }
	case 8:	/* Identify */
	  {
	    if (!ident_spell()) return FALSE;
	    break;
	  }
	case 9:	/* Lightning Bolt */
	  {
	    fire_bolt_or_beam(beam, GF_ELEC, dir, damroll(2+((plev-5)/5), 8));
	    break;
	  }
	case 10:	/* Confuse Monster */
	  {
	    (void)confuse_monster(dir, plev + 10);
	    break;
	  }
	case 11:        /* Telekinesis */
	  {
	    if (!target_set_interactive(TARGET_OBJ)) return FALSE;
	    if (!py_pickup(2, p_ptr->target_row, p_ptr->target_col)) return FALSE;
	    break;
	  }
	case 12:	/* Sleep Monster */
	  {
	    (void)sleep_monster(dir, plev + 10);
	    break;
	  }
	case 13:	/* Teleport Self */
	  {
	    teleport_player(50 + plev * 2, TRUE);
	    break;
	  }
	case 14:	/* Spear of Light */
	  {
	    msg_print("A line of blue shimmering light appears.");
	    lite_line(dir);
	    break;
	  }
	case 15:	/* Frost Beam */
	  {
	    fire_beam(GF_COLD, dir, 5 + plev);
	    break;
	  }
	case 16:        /* Magical Throw */
	  {
	    magic_throw = TRUE;
	    textui_cmd_throw();
	    magic_throw = FALSE;
	    break;
	  }
	case 17:	/* Satisfy Hunger */
	  {
	    (void)set_food(PY_FOOD_MAX - 1);
	    break;
	  }
	case 18:	/* Detect Invisible */
	  {
	    (void)detect_monsters_invis(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 19:	/* Magic Disarm */
	  {
	    (void)disarm_trap(dir);
	    break;
	  }
	case 20:        /* Blink Monster */
	  {
	    (void)teleport_monster(dir, 5 + (plev/10));
	    break;
	  }
	case 21:	/* Cure */
	  {
	    (void)set_poisoned(0);
	    (void)set_cut(0);
	    (void)set_stun(0);
	    break;
	  }
	case 22:	/* Detect Enchantment */
	  {
	    (void)detect_objects_magic(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 23:  /* Stone to Mud */
	  {
	    (void)wall_to_mud(dir);
	    break;
	  }
	case 24:	/* Minor Recharge */
	  {
	    if (!recharge(120)) return FALSE;
	    break;
	  }
	case 25:	/* Sleep Monsters */
	  {
	    (void)sleep_monsters(plev + 10);
	    break;
	  }
	case 26:	/* Thrust Away */
	  {
	    fire_arc(GF_FORCE, dir, damroll(6 + (plev / 10), 8), (1 + plev / 10), 0);
	    break;
	  }
	case 27:	/* Fire Ball */
	  {
	    fire_ball(GF_FIRE, dir, 55 + plev, 2, FALSE);
	    break;
	  }
	case 28:	/* Tap magical energy */
	  {
	    tap_magical_energy();
	    break;
	  }
	case 29:	/* Slow Monster */
	  {
	    (void)slow_monster(dir, plev + 10);
	    break;
	  }
	case 30:	/* Teleport Other */
	  {
	    (void)teleport_monster(dir, 45 + (plev/2));
	    break;
	  }
	case 31:	/* Haste Self */
	  {
	    if (!p_ptr->fast)
	      {
		(void)set_fast(randint1(20) + plev);
	      }
	    else
	      {
		(void)set_fast(p_ptr->fast + randint1(5));
	      }
	    break;
	  }
	case 32:	/* Hold Monsters */
	  {
	    fire_ball(GF_HOLD, dir, 0, 2, FALSE);
	    break;
	  }
	case 33:	/* Clear Mind */
	  {
	    if (p_ptr->csp < p_ptr->msp)
	      {
		p_ptr->csp += 1 + plev / 12;
		p_ptr->csp_frac = 0;
		if (p_ptr->csp > p_ptr->msp) (p_ptr->csp = p_ptr->msp);
		msg_print("You feel your head clear a little.");
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	      }
	    break;
	  }
	case 34:	/* Resist Element */
	  {
	    if (!choose_ele_resist()) return FALSE;
	    break;
	  }
	case 35:	/* Shield */
	  {
	    if (!p_ptr->shield)
	      {
		(void)set_shield(p_ptr->shield + randint1(20) + 30);
	      }
	    else
	      {
		(void)set_shield(p_ptr->shield + randint1(10) + 15);
	      }
	    break;
	  }
	case 36:	/* Resistance */
	  {
	    (void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	    (void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	    (void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	    (void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	    (void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + 20);
	    break;
	  }
	case 37:	/* Essence of Speed */
	  {
	    if (!p_ptr->fast)
	      {
		(void)set_fast(randint1(30) + 10 + plev);
	      }
	    else
	      {
		(void)set_fast(p_ptr->fast + randint1(10));
	      }
	    break;
	  }
	case 38:	/* Strengthen Defences */
	  {
	    if (!p_ptr->magicdef)
	      {
		(void)set_extra_defences(40);
	      }
	    else
	      {
		(void)set_extra_defences(p_ptr->magicdef + randint1(20));
	      }
	    
	    break;
	  }
	case 39:	/* Door Creation */
	  {
	    (void)door_creation();
	    break;
	  }
	case 40:	/* Stair Creation */
	  {
	    (void)stair_creation();
	    break;
	  }
	case 41:	/* Teleport Level */
	  {
	    (void)teleport_player_level(TRUE);
	    break;
	  }
	case 42:	/* Word of Recall */
	  {
	    if (!word_recall(randint0(20) + 15))
	      break;
	    break;
	  }
	case 43:	/* Word of Destruction */
	  {
	    destroy_area(py, px, 15, TRUE);
	    break;
	  }
	case 44: /* Dimension Door. */
	  {
	    msg_print("Choose a location to teleport to.");
	    msg_print(NULL);
	    dimen_door();
	    break;
	  }
	case 45:	/* Acid Bolt */
	  {
	    fire_bolt_or_beam(beam, GF_ACID, dir, damroll(3 + plev / 3, 8));
	    break;
	  }
	case 46:	/* Polymorph Other */
	  {
	    (void)poly_monster(dir);
	    break;
	  }
	case 47:	/* Earthquake */
	  {
	    earthquake(py, px, 10, FALSE);
	    break;
	  }
	case 48:	/* Beguiling */
	  {
	    (void)slow_monsters(5 * plev / 3);
	    (void)confu_monsters(5 * plev / 3);
	    (void)sleep_monsters(5 * plev / 3);
	    break;
	  }
	case 49:	/* Starburst */
	  {
	    fire_sphere(GF_LITE, 0,
			5 * plev / 2, plev / 12, 20);
	    break;
	  }
	case 50:	/* Major Recharge */
	  {
	    recharge(220);
	    break;
	  }
	case 51:	/* Cloud Kill */
	  {
	    fire_ball(GF_POIS, dir, 10 + plev, 3, FALSE);
	    fire_ball(GF_ACID, dir, 2 * plev, 2, FALSE);
	    break;
	  }
	case 52:	/* Ice Storm */
	  {
	    fire_ball(GF_ICE, dir, 3 * plev, 3, FALSE);
	    break;
	  }
	case 53:	/* Meteor Swarm */
	  {
	    fire_ball(GF_METEOR, dir, 80 + (plev * 2), 1, FALSE);
	    break;
	  }  
	case 54:	/* Cacophony */
	  {
	    (void)cacophony(plev + damroll(3, plev));
	    break;
	  }
	case 55:	/* Unleash Chaos */
	  {
	    fire_ball(GF_CHAOS, dir, 80 + (plev * 2), 3, FALSE);
	    break;
	  }
	case 56:	/* Wall of Force */
	  {
	    fire_arc(GF_FORCE, dir, 4 * plev, 3 + plev / 15, 60);
	    break;
	  }
	case 57: /* Rune of the Elements */
	  {
	    lay_rune(RUNE_ELEMENTS);
	    break;
	  }
	case 58: /* Rune of Magic Defence */
	  {
	    lay_rune(RUNE_MAGDEF);
	    break;
	  }
	case 59: /* Rune of Instability */
	  {
	    lay_rune(RUNE_QUAKE);
	    break;
	  }
	case 60: /* Rune of Mana */
	  {
	    lay_rune(RUNE_MANA);
	    break;
	  }
	case 61: /* Rune of Protection */
	  {
	    lay_rune(RUNE_PROTECT);
	    break;
	  }
	case 62: /* Rune of Power */
	  {
	    lay_rune(RUNE_POWER);
	    break;
	  }
	case 63: /* Rune of Speed */
	  {
	    lay_rune(RUNE_SPEED);
	    break;
	  }
	
	/* Holy Prayers */
	
	case 64: /* Detect Evil */
	  {
	    (void)detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 65: /* Cure Light Wounds */
	  {
	    (void)hp_player(damroll(2, plev / 4 + 5));
	    (void)set_cut(p_ptr->cut - 10);
	    break;
	  }
	case 66: /* Bless */
	  {
	    if (!p_ptr->blessed)
	      {
		(void)set_blessed(p_ptr->blessed + randint1(12) + 12);
	      }
	    else
	      {
		(void)set_blessed(p_ptr->blessed + randint1(4) + 4);
	      }
	    break;
	  }
	case 67: /* Remove Fear */
	  {
	    (void)set_afraid(0);
	    break;
	  }
	case 68: /* Call Light */
	  {
	    (void)lite_area(damroll(2, 1 + (plev / 3)), (plev / 10) + 1);
	    break;
	  }
	case 69: /* Find Traps */
	  {
	    (void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 70: /* Detect Doors/Stairs */
	  {
	    /* Hack - 'show' effected region only with
	     * the first detect */
	    (void)detect_doors(DETECT_RAD_DEFAULT, TRUE);
	    (void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	    break;
	  }
	case 71: /* Slow Poison */
	  {
	    (void)set_poisoned(p_ptr->poisoned / 2);
	    break;
	  }
	case 72: /* Cure Serious Wounds */
	  {
	    (void)hp_player(damroll(4, plev / 4 + 6));
	    (void)set_cut((p_ptr->cut / 2) - 5);
	    break;
	  }
	case 73: /* Scare Monster */
	  {
	    (void)fear_monster(dir, (3 * plev / 2) + 10);
	    break;
	  }
	case 74: /* Portal */
	  {
	    teleport_player(2 * plev, TRUE);
	    break;
	  }
	case 75: /* Chant */
	  {
	    if (!p_ptr->blessed)
	      {
		(void)set_blessed(p_ptr->blessed + randint1(24) + 24);
	      }
	    else
	      {
		(void)set_blessed(p_ptr->blessed + randint1(8) + 8);
	      }
	    break;
	  }
	case 76: /* Sanctuary */
	  {
	    (void)sleep_monsters_touch(plev + 25);
	    break;
	  }
	case 77: /* Satisfy Hunger */
	  {
	    (void)set_food(PY_FOOD_MAX - 1);
	    break;
	  }
	case 78: /* Remove Curse */
	  {
	    if (remove_curse()) 
	      msg_print ("You feel kindly hands aiding you.");
	    break;
	  }
	case 79: /* Resist Heat and Cold */
	  {
	    (void)set_oppose_fire(p_ptr->oppose_fire + randint1(10) + plev / 2);
	    (void)set_oppose_cold(p_ptr->oppose_cold + randint1(10) + plev / 2);
	    break;
	  }
	case 80: /* Neutralize Poison */
	  {
	    (void)set_poisoned(0);
	    break;
	  }
	case 81: /* Orb of Draining */
	  {
	    fire_sphere(GF_HOLY_ORB, dir, (damroll(3, 6) + plev / 4 +
			 (plev / ((check_ability(SP_STRONG_MAGIC)) ? 2 : 4))),
			((plev < 30) ? 1 : 2), 30);
	    break;
	  }
	case 82: /* Sense Invisible */
	  {
	    (void)set_tim_invis(p_ptr->tim_invis + randint1(24) + plev);
	    break;
	  }
	case 83: /* Protection from Evil */
	  {
	    if (!p_ptr->protevil)
	      {
		(void)set_protevil(p_ptr->protevil + 
				   randint1(24) + 3 * plev / 2);
	      }
	    else
	      {
		(void)set_protevil(p_ptr->protevil + randint1(30));
	      }
	    break;
	  }
	case 84: /* Cure Mortal Wounds */
	  {
	    (void)hp_player(damroll(9, plev / 3 + 12));
	    (void)set_stun(0);
	    (void)set_cut(0);
	    break;
	  }
	case 85: /* Earthquake */
	  {
	    earthquake(py, px, 10, FALSE);
	    break;
	  }
	case 86: /* Sense Surroundings. */
	  {
	    /* Extended area for high-level Rangers. */
	    if ((check_ability(SP_LORE)) && (plev > 34))
	      map_area(0, 0, TRUE);
	    else map_area(0, 0, FALSE);
	    break;
	  }
	case 87: /* Turn Undead */
	  {
	    (void)turn_undead((3 * plev / 2) + 10);
	    break;
	  }
	case 88: /* Prayer */
	  {
	    if (!p_ptr->blessed)
	      {
		(void)set_blessed(p_ptr->blessed + randint1(48) + 48);
	      }
	    else
	      {
		(void)set_blessed(p_ptr->blessed + randint1(12) + 12);
	      }
	    break;
	  }
	case 89: /* Dispel Undead */
	  {
	    (void)dispel_undead(randint1(plev * 3));
	    break;
	  }
	case 90: /* Heal */
	  {
	    (void)hp_player(300);
	    (void)set_stun(0);
	    (void)set_cut(0);
	    break;
	  }
	case 91: /* Dispel Evil */
	  {
	    (void)dispel_evil(randint1(plev * 3));
	    break;
	  }
	case 92: /* Sacred Shield */
	  {
	    if (!p_ptr->shield)
	      {
		(void)set_shield(p_ptr->shield + randint1(20) + plev / 2);
	      }
	    else
	      {
		(void)set_shield(p_ptr->shield + randint1(10) + plev / 4);
	      }
	    break;
	  }
	case 93: /* Glyph of Warding */
	  {
	    (void)lay_rune(RUNE_PROTECT);
	    break;
	  }
	case 94: /* Holy Word */
	  {
	    (void)dispel_evil(randint1(plev * 4));
	    (void)hp_player(300);
	    (void)set_afraid(0);
	    (void)set_poisoned(p_ptr->poisoned - 200);
	    (void)set_stun(0);
	    (void)set_cut(0);
	    break;
	  }
	case 95: /* Blink */
	  {
	    teleport_player(10, TRUE);
	    break;
	  }
	case 96: /* Teleport Self */
	  {
	    teleport_player(plev * 4, TRUE);
	    break;
	  }
	case 97: /* Teleport Other */
	  {
	    (void)teleport_monster(dir, 45 + (plev/3));
	    break;
	  }
	case 98: /* Teleport Level */
	  {
	    (void)teleport_player_level(TRUE);
	    break;
	  }
	case 99: /* Word of Recall */
	  {
	    if (!word_recall(randint0(20) + 15))
	      break;
	    break;
	  }
	case 100: /* Alter Reality */
	  {
	    if (OPT(adult_ironman)) msg_print("Nothing happens.");
	    else
	      {
	        msg_print("The world changes!");
	    
	        /* Leaving */
	        p_ptr->leaving = TRUE;
	      }
	    
	    break;
	  }
	case 101: /* Detect Monsters */
	  {
	    (void)detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 102: /* Detection */
	  {
	    (void)detect_all(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 103: /* Probing */
	  {
	    (void)probing();
	    break;
	  }
	case 104: /* Perception */
	  {
	    if (!ident_spell()) return FALSE;
	    break;
	  }
	case 105: /* Clairvoyance */
	  {
	    wiz_lite(FALSE);
	    break;
	  }
	case 106: /* Banishment */
	  {
	    if (banish_evil(80))
	      {
		msg_print("The power of the Valar banishes evil!");
	      }
	    break;
	  } 
	case 107: /* Healing */
	  {
	    (void)hp_player(700);
	    (void)set_stun(0);
	    (void)set_cut(0);
	    break;
	  }
	case 108: /* Sacred Knowledge */
	  {
	    (void)identify_fully();
	    break;
	  }
	case 109: /* Restoration */
	  {
	    (void)do_res_stat(A_STR);
	    (void)do_res_stat(A_INT);
	    (void)do_res_stat(A_WIS);
	    (void)do_res_stat(A_DEX);
	    (void)do_res_stat(A_CON);
	    (void)do_res_stat(A_CHR);
	    break;
	  }
	case 110: /* Remembrance */
	  {
	    (void)restore_level();
	    break;
	  }
	case 111: /* Unbarring Ways */
	  {
	    (void)destroy_doors_touch();
	    break;
	  }
	case 112: /* Recharging */
	  {
	    if (!recharge(140)) return FALSE;
	    break;
	  }
	case 113: /* Dispel Curse */
	  {
	    if (remove_curse_good()) 
	      msg_print("A beneficent force surrounds you for a moment.");
	    break;
	  }
	case 114: /* Disarm Trap */
	  {
	    (void)disarm_trap(dir);
	    break;
	  }
	case 115: /* Holding */
	  {
	    /* Spell will hold any monster or door in one square. */
	    fire_ball(GF_HOLD, dir, 0, 0, FALSE);
	    
	    break;
	  }
	case 116: /* Enchant Weapon or Armour */
	  {
	    char answer;
	    
	    /* Query */
	    if (small_screen) 
	      msg_print("Enchant a 'W'eapon or 'A'rmour");
	    else 
	      msg_print("Would you like to enchant a 'W'eapon or 'A'rmour");
	    
	    /* Buttons */
	    add_button("a", 'a');
	    add_button("w", 'w');

	    /* Interact and enchant. */
	    while(1)
	      {
		ui_event_data ke;

		ke = inkey_ex();
		answer = ke.key;

		if ((answer == 'W') || (answer == 'w'))
		  {
		    (void)enchant_spell(randint0(4) + 1, 
					randint0(4) + 1, 0);
		    break;
		  }
		else if ((answer == 'A') || (answer == 'a'))
		  {
		    (void)enchant_spell(0, 0, randint0(3) + 2);
		    break;
		  }
		else if (answer == ESCAPE) 
		  {
		    kill_button('w');
		    kill_button('a');
		    return FALSE;
		  }
	      }
	    kill_button('w');
	    kill_button('a');

	    break;
	  }
	case 117: /* Light of Manwe */
	  {
	    fire_ball(GF_LITE, dir, plev * 2, 3, FALSE);
	    break;
	  }
	case 118: /* Lance of Orome */
	  {
	    fire_beam(GF_HOLY_ORB, dir, 3 * plev / 2);
	    break;
	  }
	case 119: /* Hammer of Aule */
	  {
	    destroy_area(py, px, 15, TRUE);
	    break;
	  }
	  
	case 120: /* Strike of Mandos */
	  {
	    drain_life(dir, plev * 3 + randint1(100));
	    break;
	  }
	case 121: /* Call on Varda */
	  {
	    msg_print("Gilthoniel A Elbereth!");
	    fire_sphere(GF_LITE, 0, plev * 5, plev / 7 + 2, 20);
	    (void)fear_monsters(plev * 2);
	    (void)hp_player(500);
	    break;
	  }
	  
	case 122: /* Paladin Prayer: Elemental Infusion */
	  {
	    if (!choose_ele_attack()) return FALSE;
	    break;
	  }
	case 123: /* Paladin Prayer: Sanctify for Battle */
	  {
	    /* Can't have black breath and holy attack 
	     * (doesn't happen anyway) */
	    if (p_ptr->special_attack & ATTACK_BLKBRTH) 
	      p_ptr->special_attack &= ~ ATTACK_BLKBRTH;
	    
	    if (!(p_ptr->special_attack & ATTACK_HOLY))
	      msg_print("Your blows will strike with Holy might!");
	    p_ptr->special_attack |= (ATTACK_HOLY);
	    
	    /* Redraw the state */
	    p_ptr->redraw |= (PR_STATUS);
	    
	    break;
	  }
	case 124: /* Paladin Prayer: Horn of Wrath */
	  {
	    (void)hp_player(20);
	    if (!p_ptr->hero)
	      {
		(void)set_hero(p_ptr->hero + randint1(20) + 20);
	      }
	    else
	      {
		(void)set_hero(p_ptr->hero + randint1(10) + 10);
	      }
	    (void)set_afraid(0);
	    
	    (void)fear_monsters(plev);
	    break;
	  }
	case 125:	/* Rogue Spell: Hit and Run */
	  {
	    p_ptr->special_attack |= (ATTACK_FLEE);
	    
	    /* Redraw the state */
	    p_ptr->redraw |= (PR_STATUS);
	    
	    break;
	  }
	case 126:	/* Rogue Spell: Day of Misrule */
	  {
	    cptr p = (p_ptr->psex == SEX_FEMALE ? "Daughters" : "Sons");
	    msg_format("%s of Night rejoice!  It's the Day of Misrule!", p);
	    (void)set_fast(randint1(30) + 30);
	    (void)set_shero(p_ptr->shero + randint1(30) + 30);
	    break;
	  }
	case 127:	/* Rogue Spell: Detect Treasure */
	  {
	    /* Hack - 'show' affected region only with
	     * the first detect */
	    (void)detect_treasure(DETECT_RAD_DEFAULT, TRUE);
	    (void)detect_objects_gold(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_objects_normal(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	
	
	/* Nature Magics */
	
	case 128: /* detect life */
	  {
	    (void)detect_monsters_living(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 129:  /* call light */
	  {
	    (void)lite_area(damroll(2, 1 + (plev / 4)), (plev / 10) + 1);
	    break;
	  }
	case 130: /* foraging */
	  {
	    (void)set_food(PY_FOOD_MAX - 1);
	    break;
	  }
	case 131:  /* blink */
	  {
	    teleport_player(10, TRUE);
	    break;
	  }
	case 132:  /* combat poison */
	  {
	    (void)set_poisoned(p_ptr->poisoned / 2);
	    break;
	  }
	case 133:  /* lightning spark */
	  {
	    fire_arc(GF_ELEC, dir, damroll(2 + (plev/8), 6),(1 + plev / 5), 0);
	    break;
	  }
	case 134:  /* door destruction */
	  {
	    (void)destroy_doors_touch();
	    break;
	  }
	case 135:  /* turn stone to mud */
	  {
	    (void)wall_to_mud(dir);
	    break;
	  }
	case 136:  /* ray of sunlight */
	  {
	    msg_print("A ray of golden yellow light appears.");
	    lite_line(dir);
	    break;
	  }
	case 137:  /* Cure poison */
	  {
	    (void)set_poisoned(0);
	    break;
	  }
	case 138:  /* frost bolt */
	  {
	    fire_bolt_or_beam(beam - 10, GF_COLD,dir,damroll(2 + (plev/5), 8));
	    break;
	  }
	case 139:  /* sleep creature */
	  {
	    (void)sleep_monster(dir, plev + 10);
	    break;
	  }
	case 140:  /* frighten creature */
	  {
	    (void)fear_monster(dir, plev + 10);
	    break;
	  }
	case 141:  /* detect trap/doors */
	  {
	    (void)detect_traps(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_stairs(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 142:  /* snuff small life */
	  {
	    (void)dispel_small_monsters(2 + plev / 5);
	    break;
	  }
	case 143:  /* fire bolt */
	  {
	    fire_bolt_or_beam(beam - 10, GF_FIRE, dir,
			      damroll(3 + (plev/5), 8));
	    break;
	  }
	case 144:  /* heroism */
	  {
	    (void)hp_player(20);
	    if (!p_ptr->hero)
	      {
		(void)set_hero(p_ptr->hero + randint1(20) + 20);
	      }
	    else
	      {
		(void)set_hero(p_ptr->hero + randint1(10) + 10);
	      }
	    (void)set_afraid(0);
	    break;
	  }
	case 145:  /* remove curse */
	  {
	    if (remove_curse()) msg_print("You feel tender hands aiding you.");
	    break;
	  }
	case 146:  /* acid bolt */
	  {
	    fire_bolt_or_beam(beam - 10, GF_ACID, dir,
			      damroll(5 + (plev/5), 8));
	    break;
	  }
	case 147:  /* teleport monster */
	  {
	    (void)teleport_monster(dir, 45 + (plev/3));
	    break;
	  }
	case 148:  /* gravity bolt */
	  {
	    fire_bolt_or_beam(beam - 10, GF_GRAVITY, dir,
			      damroll(5 + (plev/4), 8));
	    break;
	  }
	case 149:  /* resist poison */
	  {
	    (void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + 20);
	    break;
	  }
	case 150:  /* earthquake */
	  {
	    earthquake(py, px, 10, FALSE);
	    break;
	  }
	case 151:  /* resist fire & cold */
	  {
	    (void)set_oppose_fire(p_ptr->oppose_fire + randint1(20) + 20);
	    (void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	    break;
	  }
	case 152:  /* detect all */
	  {
	    (void)detect_all(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 153:  /* natural vitality */
	  {
	    (void)set_poisoned((3 * p_ptr->poisoned / 4) - 5);
	    (void)hp_player(damroll(2, plev / 5));
	    (void)set_cut(p_ptr->cut - plev / 2);
	    break;
	  }
	case 154:  /* resist acid & lightning */
	  {
	    (void)set_oppose_acid(p_ptr->oppose_acid + randint1(20) + 20);
	    (void)set_oppose_elec(p_ptr->oppose_elec + randint1(20) + 20);
	    break;
	  }
	case 155:  /* wither foe */
	  {
	    fire_bolt(GF_HOLY_ORB, dir, damroll(plev / 7, 8));		
	    (void)confuse_monster(dir, plev + 10);
	    (void)slow_monster(dir, plev + 10);
	    break;
	  }
	case 156:  /* disarm trap */
	  {
	    (void)disarm_trap(dir);
	    break;
	  }
	case 157:  /* identify */
	  {
	    if (!ident_spell()) return FALSE;
	    break;
	  }
	case 158:  /* create athelas */
	  {
	    (void)create_athelas();
	    break;
	  }
	case 159:  /* raging storm */
	  {
	    fire_ball(GF_ELEC, dir, plev + randint1(60 + plev * 2), 
		      (1 + plev / 15), FALSE);
	    break;
	  }
	case 160:  /* thunderclap */
	  {
	    msg_print("Boom!");
	    fire_sphere(GF_SOUND, 0,
			plev + randint1(40 + plev * 2), plev / 8, 20);
	    break;
	  }
	case 161:  /* become mouse */
	  {
	    shape = SHAPE_MOUSE;
	    break;
	  }
	case 162:  /* become ferret */
	  {
	    shape = SHAPE_FERRET;
	    break;
	  }
	case 163:  /* become hound */
	  {
	    shape = SHAPE_HOUND;
	    break;
	  }
	case 164:  /* become gazelle */
	  {
	    shape = SHAPE_GAZELLE;
	    break;
	  }
	case 165:  /* become lion */
	  {
	    shape = SHAPE_LION;
	    break;
	  }
	case 166:  /* detect evil */
	  {
	    (void)detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 167:  /* song of frightening */
	  {
	    (void)fear_monsters(3 * plev / 2 + 10);
	    break;
	  }
	case 168:  /* sense surroundings */
	  {
	    map_area(0, 0, FALSE);
	    break;
	  }
	case 169:  /* sight beyond sight */
	  {
	    /* Hack - 'show' effected region only with
	     * the first detect */
	    (void)detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
	    (void)detect_monsters_invis(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_traps(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_doors(DETECT_RAD_DEFAULT, FALSE);
	    (void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	    if (!p_ptr->tim_invis)
	      {
		(void)set_tim_invis(p_ptr->tim_invis + 
				    randint1(24) + 24);
	      }
	    else
	      {
		(void)set_tim_invis(p_ptr->tim_invis + 
				    randint1(12) + 12);
	      }
	    break;
	  }
	case 170:  /* herbal healing */
	  {
	    (void)hp_player(damroll(25 + plev / 2, 12));
	    (void)set_cut(0);
	    (void)set_poisoned(p_ptr->poisoned - 200);
	    break;
	  }
	case 171:  /* blizzard */
	  {
	    fire_ball(GF_COLD, dir, plev + randint1(50 + plev * 2), 
		      1 + plev / 12, FALSE);
	    break;
	  }
	case 172:  /* trigger tsunami */
	  {
	    msg_print("You hurl mighty waves at your foes!");
	    fire_sphere(GF_WATER, 0, 30 + ((4 * plev) / 5) + 
			randint1(30 + plev * 2), plev / 7, 20);
	    break;
	  }
	case 173:  /* volcanic eruption */
	  {
	    msg_print("The earth convulses and erupts in fire!");
	    fire_sphere(GF_FIRE, 0, 3 * plev / 2 + randint1(50 + plev * 3), 
			1 + plev / 15, 20);
	    earthquake(py, px, plev / 5, TRUE);
	    break;
	  }
	case 174:  /* molten lightning */
	  {
	    fire_ball(GF_PLASMA, dir, 
		      35 + (2 * plev) + randint1(90 + plev * 4), 1, FALSE);
	    break;
	  }
	case 175:  /* starburst. */
	  {
	    msg_print("Light bright beyond enduring dazzles your foes!");
	    fire_sphere(GF_LITE, 0, 40 + (3 * plev / 2) + randint1(plev * 3), 
			plev / 10, 20);
	    break;
	  }
	case 176:  /* song of lulling */
	  {
	    msg_print("Your tranquil music enchants those nearby.");
	    
	    (void)slow_monsters(5 * plev / 3);
	    (void)sleep_monsters(5 * plev / 3);
	    break;
	  }
	case 177:  /* song of protection */
	  {
	    msg_print("Your song creates a mystic shield.");
	    if (!p_ptr->shield)
	      {
		(void)set_shield(p_ptr->shield + randint1(30) + plev / 2);
	      }
	    else
	      {
		(void)set_shield(p_ptr->shield + randint1(15) + plev / 4);
	      }
	    break;
	  }
	case 178:  /* song of dispelling */
	  {
	    msg_print("An unbearable discord tortures your foes!");
	    
	    (void)dispel_monsters(randint1(plev * 2));
	    (void)dispel_evil(randint1(plev * 2));
	    break;
	  }
	case 179:  /* song of warding */
	  {
	    msg_print("Your song creates a place of sanctuary.");
	    
	    (void)lay_rune(RUNE_PROTECT);
	    break;
	  }
	case 180:  /* song of renewal */
	  {
	    msg_print("Amidst the gloom, you invoke light and beauty; your body regains its natural vitality.");
	    
	    (void)do_res_stat(A_STR);
	    (void)do_res_stat(A_INT);
	    (void)do_res_stat(A_WIS);
	    (void)do_res_stat(A_DEX);
	    (void)do_res_stat(A_CON);
	    (void)do_res_stat(A_CHR);
	    (void)restore_level();
	    break;
	  }
	case 181:  /* Web of Vaire */
	  {
	    fire_bolt_or_beam(plev * 2, GF_TIME, dir,
			      damroll(plev / 6, 8));
	    break;
	  }
	case 182:  /* Speed of Nessa */
	  {
	    if (!p_ptr->fast)
	      {
		(void)set_fast(randint1(10) + plev / 2);
	      }
	    else
	      {
		(void)set_fast(p_ptr->fast + randint1(5));
	      }
	    break;
	  }
	case 183:  /* Renewal of Vana */
	  {
	    if (!recharge(125)) return FALSE;
	    break;
	  }
	case 184:  /* Servant of Yavanna */
	  {
	    shape = SHAPE_ENT;
	    break;
	  }
	case 185:  /* Tears of Nienna */
	  {
	    (void)restore_level();
	    break;
	  }
	case 186:  /* Healing of Este */
	  {
	    (void)dispel_evil(100);
	    (void)hp_player(500);
	    (void)set_blessed(p_ptr->blessed + randint1(100) + 100);
	    (void)set_afraid(0);
	    (void)set_poisoned(0);
	    (void)set_stun(0);
	    (void)set_cut(0);
	    break;
	  }
	case 187:  /* Ranger Spell:  Creature Knowledge */
	  {
	    msg_print("Target the monster you wish to learn about.");
	    if (!get_aim_dir(&dir)) return FALSE;
	    pseudo_probe();
	    break;
	  }
	case 188: /* Nature's vengeance */
	  {
	    nature_strike(damroll(6, 6) + plev / 3);
	    break;
	  }
	case 189: /* Song of growth */
	  {
	    grow_trees_and_grass(FALSE);
	    break;
	  }
	case 190: /* Song of preservation */
	  {
	    u32b flags = (OF_ACID_PROOF|OF_FIRE_PROOF);

	    if (!el_proof(flags)) return FALSE;
	    break;
	  }
	case 191: /* Tremor */
	  {
	    if (!tremor()) return FALSE;
	    break;
	  }
	
	/* Necromantic Spells */
	
	case 192: /* nether bolt */
	  {
	    fire_bolt(GF_NETHER, dir, damroll(2, 5 + plev / 7));
	    break;
	  }
	case 193: /* detect evil */
	  {
	    (void)detect_monsters_evil(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 194: /* enhanced infravision */
	  {
	    set_tim_infra(p_ptr->tim_infra + 70 + randint1(70));
	    break;
	  }
	case 195: /* break curse */
	  {
	    if (remove_curse()) 
	      msg_print("You feel mighty hands aiding you.");
	    break;
	  }
	case 196: /* slow monster */
	  {
	    (void)slow_monster(dir, plev + 5);
	    break;
	  }
	case 197: /* sleep monster */
	  {
	    (void)sleep_monster(dir, plev + 5);
	    break;
	  }
	case 198: /* horrify */
	  {
	    (void)fear_monster(dir, plev + 15);
	    break;
	  }
	case 199: /* become bat */
	  {
	    take_hit(damroll(2, 4), "shapeshifting stress");
	    shape = SHAPE_BAT;
	    break;
	  }
	case 200: /* door destruction */
	  {
	    (void)destroy_doors_touch();
	    break;
	  }
	case 201: /* dark bolt */
	  {
	    fire_bolt_or_beam(beam - 10, GF_DARK, dir, 
			      damroll((3 + plev / 7), 8));
	    break;
	  }
	case 202: /* noxious fumes */
	  {
	    fire_sphere(GF_POIS, 0, 10 + plev, 2 + plev / 12, 40);
	    
	    pois_hit(5);
	    break;
	  }
	case 203: /* turn undead */
	  {
	    (void)turn_undead(3 * plev / 2);
	    break;
	  }
	case 204: /* turn evil */
	  {
	    (void)turn_evil(5 * plev / 4);
	    break;
	  }
	case 205: /* cure poison */
	  {
	    (void)set_poisoned(0);
	    break;
	  }
	case 206: /* dispel undead */
	  {
	    (void)dispel_undead(plev + 15 + randint1(3 * plev / 2));
	    break;
	  }
	case 207: /* dispel evil */
	  {
	    (void)dispel_evil(plev + randint1(plev));
	    break;
	  }
	case 208: /* see invisible */
	  {
	    if (!p_ptr->tim_invis)
	      {
		set_tim_invis(p_ptr->tim_invis + 20 + 
			      randint1(plev / 2));
	      }
	    else
	      {
		set_tim_invis(p_ptr->tim_invis + 10 + 
			      randint1(plev / 4));
	      }
	    break;
	  }
	case 209: /* shadow shifting */
	  {
	    take_hit(damroll(1, 4), "shadow dislocation");
	    teleport_player(16, TRUE);
	    break;
	  }
	case 210: /* detect traps */
	  {
	    (void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 211: /* detect doors/stairs */
	  {
	    /* Hack - 'show' effected region only with
	     * the first detect */
	    (void)detect_doors(DETECT_RAD_DEFAULT, TRUE);
	    (void)detect_stairs(DETECT_RAD_DEFAULT, FALSE);
	    break;
	  }
	case 212: /* sleep monsters */
	  {
	    (void)sleep_monsters(plev + 5);
	    break;
	  }
	case 213: /* slow monsters */
	  {
	    (void)slow_monsters(plev + 5);
	    break;
	  }
	case 214: /* detect magic */
	  {
	    (void)detect_objects_magic(DETECT_RAD_DEFAULT, TRUE);
	    break;
	  }
	case 215: /* death bolt */
	  {
	    take_hit(damroll(1, 6), "the dark arts");
	    fire_bolt_or_beam(beam, GF_SPIRIT, dir,
			      damroll(2 + plev / 3, 8));
	    break;
	  }
	case 216: /* resist poison */
	  {
	    (void)set_oppose_pois(p_ptr->oppose_pois + randint1(20) + plev / 2);
	    break;
	  }
	case 217: /* Exorcise Demons */
	  {
	    (void)dispel_demons(2 * plev + randint1(2 * plev));
	    break;
	  }
	case 218: /* dark spear */
	  {
	    fire_beam(GF_DARK, dir, 15 + 7 * plev / 4);
	    break;
	  }
	case 219: /* chaos strike */
	  {
	    fire_bolt_or_beam(beam, GF_CHAOS, dir, damroll(1 + plev / 2, 8));
	    break;
	  }
	case 220: /* genocide */
	  {
	    (void)genocide();
	    break;
	  }
	case 221: /* dark ball */
	  {
	    fire_ball(GF_DARK, dir, 50 + plev * 2, 2, FALSE);
	    break;
	  }
	case 222: /* stench of death */
	  {
	    take_hit(damroll(2, 8), "the stench of Death");
	    (void)dispel_living(50 + randint1(plev));
	    confu_monsters(plev + 10);
	    fire_sphere(GF_POIS, dir, plev * 2, 5 + plev / 11, 40);
	    break;
	  }
	case 223: /* probing */
	  {
	    (void)probing();
	    break;
	  }
	case 224: /* shadow mapping */
	  {
	    map_area(0, 0, FALSE);
	    break;
	  }
	case 225: /* identify */
	  {
	    if (!ident_spell()) return FALSE;
	    break;
	  }
	case 226: /* shadow warping */
	  {
	    take_hit(damroll(2, 6), "shadow dislocation");
	    teleport_player(plev * 3, TRUE);
	    break;
	  }
	case 227: /* poison ammo - for assassins only */
	  {
	    if (!brand_missile(0, EGO_POISON)) return FALSE;
	    break;
	  }
	case 228: /* resist acid and cold */
	  {
	    (void)set_oppose_acid(p_ptr->oppose_pois + randint1(20) + 20);
	    (void)set_oppose_cold(p_ptr->oppose_cold + randint1(20) + 20);
	    break;
	  }
	case 229: /* heal any wound */
	  {
	    (void)set_cut(0);
	    (void)set_stun(0);
	    break;
	  }
	case 230: /* protection from evil */
	  {
	    (void)set_protevil(p_ptr->protevil + plev / 2 + randint1(plev));
	    break;
	  }
	case 231: /* black blessing */
	  {
	    (void)set_blessed(p_ptr->blessed + randint1(66));
	    break;
	  }
	case 232: /* banish evil */
	  {
	    (void)banish_evil(80);
	    break;
	  }
	case 233: /* shadow barrier */
	  {
	    if (!p_ptr->shield)
	      {
		(void)set_shield(p_ptr->shield + randint1(20) + 10);
	      }
	    else
	      {
		(void)set_shield(p_ptr->shield + randint1(10) + 5);
	      }
	    break;
	  }
	case 234: /* detect all monsters */
	  {
	    /* Hack - 'show' affected region only with
	     * the first detect */
	    (void)detect_monsters_normal(DETECT_RAD_DEFAULT, TRUE);
	    (void)detect_monsters_invis(DETECT_RAD_DEFAULT, FALSE);
	    break;
	  }
	case 235: /* strike at life */
	  {
	    fire_bolt(GF_NETHER, dir,
		      damroll(3 * plev / 5, 13));
	    break;
	  }
	case 236: /* orb of death */
	  {
	    take_hit(damroll(2, 8), "Death claiming his wages");
	    fire_sphere(GF_SPIRIT, dir, 20 + (4 * plev), 1, 20);
	    break;
	  }
	case 237: /* dispel life */
	  {
	    (void)dispel_living(60 + damroll(4, plev));
	    break;
	  }
	case 238: /* vampiric drain */
	  {
	    fire_bolt(GF_SPIRIT, dir,
		      damroll(plev / 3, 11));
	    (void)hp_player(3 * plev);
	    (void)set_food(p_ptr->food + 1000);
	    break;
	  }
	case 239: /* recharging */
	  {
	    if (!recharge(140)) return FALSE;
	    break;
	  }
	case 240: /* become werewolf */
	  {
	    take_hit(damroll(2, 7), "shapeshifting stress");
	    shape = SHAPE_WEREWOLF;
	    break;
	  }
	case 241: /* dispel curse */
	  {
	    if (remove_curse_good()) 
	      msg_print ("You feel mighty hands aiding you.");
	    break;
	  }
	case 242: /* become vampire */
	  {
	    take_hit(damroll(3, 6), "shapeshifting stress");
	    shape = SHAPE_VAMPIRE;
	    break;
	  }
	case 243: /* haste self */
	  {
	    if (!p_ptr->fast)
	      {
		(void)set_fast(10 + randint1(20));
	      }
	    else
	      {
		(void)set_fast(p_ptr->fast + randint1(5));
	      }
	    break;
	  }
	case 244: /* Assassin spell - prepare black breath */
	  {
	    /* Can't have black breath and holy attack 
	     * (doesn't happen anyway) */
	    if (p_ptr->special_attack & ATTACK_HOLY) 
	      p_ptr->special_attack &= ~ ATTACK_HOLY;
	    
	    if (!(p_ptr->special_attack & ATTACK_BLKBRTH))
	      msg_print("Your hands start to radiate Night.");	
	    p_ptr->special_attack |= (ATTACK_BLKBRTH);
	    
	    /* Redraw the state */
	    p_ptr->redraw |= (PR_STATUS);
	    
	    break;
	  }
	case 245: /* word of destruction */
	  {
	    destroy_area(py, px, 15, TRUE);
	    break;
	  }
	case 246: /* teleport away */
	  {
	    (void)teleport_monster(dir, 45 + (plev/3));
	    break;
	  }
	case 247: /* smash undead */
	  {
	    (void)dispel_undead(plev * 3 + randint1(50));
	    break;
	  }
	case 248: /* bind undead */
	  {
	    (void)hold_undead();
	    break;
	  }
	case 249: /* darkness storm */
	  {
	    fire_ball(GF_DARK, dir, 11 * plev / 2, plev / 7, FALSE);
	    break;
	  }
	case 250: /* Necro spell - timed ESP */
	  {
	    if (!p_ptr->tim_esp)
	      {
		(void)set_tim_esp(30 + randint1(40));
	      }
	    else
	      {
		(void)set_tim_esp(p_ptr->tim_esp + randint1(30));
	      }
	    break;
	  }
	case 251:	/* Rogue and Assassin Spell - Slip into the Shadows */
	  {
	    if (!p_ptr->superstealth)
	      {
		(void)set_superstealth(40,TRUE);
	      }
	    else
	      {
		(void)set_superstealth(p_ptr->superstealth + randint1(20),TRUE);
	      }
	    break;
	  }
	case 252:	/* Assassin spell: Bloodwrath */
	  {
	    if (!p_ptr->shero)
	      {
		(void)set_shero(40);
	      }
	    else
	      {
		(void)set_shero(p_ptr->shero + randint1(20));
	      }
	    
	    (void)set_fast(40);
	    
	    break;
	  }
	case 253:	/* Assassin Spell - Rebalance Weapon */
	  {
	    rebalance_weapon();
	    break;
	  }
	case 254:	/* Necro spell - Dark Power */
	  {
	    if (p_ptr->csp < p_ptr->msp)
	      {
		take_hit(75, "dark power");
		p_ptr->csp += 3 * plev / 2;
		p_ptr->csp_frac = 0;
		if (p_ptr->csp > p_ptr->msp) (p_ptr->csp = p_ptr->msp);
		msg_print("You feel flow of power.");
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER_0 | PW_PLAYER_1);
	      }
	    break;
	  }
	  
	default:	/* No Spell */
	  {
	    msg_print("Undefined Spell");
	    break;
	  }
	}
 
 /* Alter shape, if necessary. */
  if (shape) shapechange(shape);
  
	/* Success */
	return (TRUE);
}
