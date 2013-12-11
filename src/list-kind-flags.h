/* list-kind-flags.h - object kind flags
 *
 * Changing flag order will break savefiles. There is a hard-coded limit of
 * 32 flags, due to 4 bytes of storage for item flags in the savefile. Flags
 * below start from 1 on line 11, so a flag's sequence number is its line
 * number minus 10.
 */

/* symbol       message */
KF(NONE,"")
KF(RAND_RES_NEG,"")
KF(RAND_RES_SML,"")
KF(RAND_RES,"")
KF(RAND_RES_XTRA,"")
KF(RAND_SUSTAIN,"")
KF(RAND_POWER,"")
KF(RAND_CURSE,"")
KF(RAND_XXX,"")
KF(INSTA_ART,"")
KF(NO_ORDER,"")
KF(EASY_KNOW,"")
KF(XTRA_DICE,"")
KF(XTRA_AC,"")
KF(XTRA_TO_H,"")
KF(XTRA_TO_D,"")
KF(LIGHTWEIGHT,"")
KF(HEAVY,"")
KF(XTRA_SIDES,"")
KF(XTRA_TO_A,"")
KF(POWERFUL,"")
KF(XXX12,"")
KF(XXX11,"")
KF(XXX10,"")
KF(XXX9,"")
KF(XXX8,"")
KF(XXX7,"")
KF(XXX6,"")
KF(XXX5,"")
KF(XXX4,"")
KF(XXX3,"")
KF(XXX2,"")
KF(XXX1,"")
