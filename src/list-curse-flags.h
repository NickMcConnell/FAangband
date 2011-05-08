/* list-curse-flags.h - object curse flags
 *
 * Changing flag order will break savefiles. There is a hard-coded limit of
 * 32 flags, due to 4 bytes of storage for item flags in the savefile. Flags
 * below start from 1 on line 11, so a flag's sequence number is its line
 * number minus 10.
 */

/* symbol       message */
CF(NONE,"")
CF(TELEPORT,"")
CF(NO_TELEPORT,"")
CF(AGGRO_PERM,"")
CF(AGGRO_RAND,"")
CF(SLOW_REGEN,"")
CF(AFRAID,"")
CF(HUNGRY,"")
CF(POIS_RAND,"")
CF(POIS_RAND_BAD,"")
CF(CUT_RAND,"")
CF(CUT_RAND_BAD,"")
CF(HALLU_RAND,"")
CF(DROP_WEAPON,"")
CF(ATTRACT_DEMON,"")
CF(ATTRACT_UNDEAD,"")
CF(STICKY_CARRY,"")
CF(STICKY_WIELD,"")
CF(PARALYZE,"")
CF(PARALYZE_ALL,"")
CF(DRAIN_EXP,"")
CF(DRAIN_MANA,"")
CF(DRAIN_STAT,"")
CF(DRAIN_CHARGE,"")
