/* list-identify-flags.h - object kind flags
 *
 * Changing flag order will break savefiles. There is a hard-coded limit of
 * 32 flags, due to 4 bytes of storage for item flags in the savefile. Flags
 * below start from 1 on line 11, so a flag's sequence number is its line
 * number minus 10.
 */

/* symbol       message */
IF(NONE,"")
IF(RES_ACID,"")
IF(RES_ELEC,"")
IF(RES_FIRE,"")
IF(RES_COLD,"")
IF(RES_POIS,"")
IF(RES_LIGHT,"")
IF(RES_DARK,"")
IF(RES_CONFU,"")
IF(RES_SOUND,"")
IF(RES_SHARD,"")
IF(RES_NEXUS,"")
IF(RES_NETHR,"")
IF(RES_CHAOS,"")
IF(RES_DISEN,"")
IF(SLAY_ANIMAL,"")
IF(SLAY_EVIL,"")
IF(SLAY_UNDEAD,"")
IF(SLAY_DEMON,"")
IF(SLAY_ORC,"")
IF(SLAY_TROLL,"")
IF(SLAY_GIANT,"")
IF(SLAY_DRAGON,"")
IF(BRAND_ACID,"")
IF(BRAND_ELEC,"")
IF(BRAND_FIRE,"")
IF(BRAND_COLD,"")
IF(BRAND_POIS,"")
IF(TO_H,"")
IF(TO_D,"")
IF(TO_A,"")
IF(AC,"")
IF(DD_DS,"")
