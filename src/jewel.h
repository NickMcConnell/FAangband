/** \file jewel.h
    \brief Jewellery include file
*/

#ifndef __jewel_h__
#define __jewel_h__

/* Definitions of most property flags. */

/* Stat bonuses */
#define ADD_STR			1
#define ADD_INT			2
#define ADD_WIS			3
#define ADD_DEX			4
#define ADD_CON			5
#define ADD_CHR			6

/* Other bonuses */
#define MAGIC_MASTERY		10
#define STEALTH			11
#define SEARCH			12
#define INFRA			13
#define TUNNEL			14
#define SPEED			15
#define MIGHT			16
#define SHOTS			17

/* Slays */
#define SLAY_ANIMAL		20
#define SLAY_EVIL		21
#define SLAY_UNDEAD		22
#define SLAY_DEMON		23
#define SLAY_ORC		24
#define SLAY_TROLL		25
#define SLAY_GIANT		26
#define SLAY_DRAGON		27

/* Brands */
#define BRAND_ACID		30
#define BRAND_ELEC		31
#define BRAND_FIRE		32
#define BRAND_COLD		33
#define BRAND_POIS		34

/* Object flags */
#define SUST_STR		40
#define SUST_INT		41
#define SUST_WIS		42
#define SUST_DEX		43
#define SUST_CON		44
#define SUST_CHR		45
#define SLOW_DIGEST		46
#define FEATHER	 		47
#define LITE			48
#define REGEN			49
#define TELEPATHY		50
#define SEE_INVIS		51
#define FREE_ACT		52
#define HOLD_LIFE		53
#define SEEING                  54
#define FEARLESS                55
#define DARKNESS                56
#define ELEC_PROOF              57
#define CHAOTIC                 58

/* Resists */
#define IM_ACID			70
#define IM_ELEC			71
#define IM_FIRE			72
#define IM_COLD			73
#define RES_ACID		74
#define RES_ELEC		75
#define RES_FIRE		76
#define RES_COLD		77
#define RES_POIS		78
#define RES_LITE		79
#define RES_DARK		80
#define RES_CONFU		81
#define RES_SOUND		82
#define RES_SHARD		83
#define RES_NEXUS		84
#define RES_NETHR		85
#define RES_CHAOS		86
#define RES_DISEN		87

/* Vulnerabilities */
#define VUL_BASE		90
#define VUL_ACID		90
#define VUL_ELEC		91
#define VUL_FIRE		92
#define VUL_COLD		93
#define VUL_POIS		94
#define VUL_LITE		95
#define VUL_DARK		96
#define VUL_CONFU		97
#define VUL_SOUND		98
#define VUL_SHARD		99
#define VUL_NEXUS		100
#define VUL_NETHR		101
#define VUL_CHAOS		102
#define VUL_DISEN		103
#define VUL_MAX  		103

/* Combat and armour bonuses */
#define ADD_AC			105
#define ADD_SKILL		106
#define ADD_DEADLINESS		107

/* Curses */
#define TELEPORT                110
#define CURSE_BASE              110
#define NO_TELEPORT             111
#define AGGRO_PERM              112
#define AGGRO_RAND              113
#define SLOW_REGEN              114
#define AFRAID                  115
#define HUNGRY                  116
#define POIS_RAND               117
#define POIS_RAND_BAD           118
#define CUT_RAND                119
#define CUT_RAND_BAD            120
#define HALLU_RAND              121
#define DROP_WEAPON             122
#define ATTRACT_DEMON           123
#define ATTRACT_UNDEAD          124
#define STICKY_CARRY            125
#define STICKY_WIELD            126
#define PARALYZE                127
#define PARALYZE_ALL            128
#define DRAIN_EXP               129
#define DRAIN_MANA              130
#define DRAIN_STAT              131
#define DRAIN_CHARGE            132
#define CURSE_MAX               132

#endif
