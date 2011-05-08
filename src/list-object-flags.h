/* list-object-flags.h - object flags
 *
 * Changing flag order will break savefiles. There is a hard-coded limit of
 * 32 flags, due to 4 bytes of storage for item flags in the savefile. Flags
 * below start from 1 on line 11, so a flag's sequence number is its line
 * number minus 10.
 */

/* symbol       message */
OF(NONE,        "")
OF(THROWING,     "")
OF(PERFECT_BALANCE,     "")
OF(SUSTAIN_STR,    "Your %s glows.")
OF(SUSTAIN_INT,    "Your %s glows.")
OF(SUSTAIN_WIS,    "Your %s glows.")
OF(SUSTAIN_DEX,    "Your %s glows.")
OF(SUSTAIN_CON,    "Your %s glows.")
OF(SUSTAIN_CHR,    "Your %s glows.")
OF(SLOW_DIGEST, "You feel your %s slow your metabolism.")
OF(FEATHER,     "Your %s slows your fall.")
OF(REGEN,       "You feel your %s speed up your recovery.")
OF(TELEPATHY,   "")
OF(SEE_INVIS,   "")
OF(FREE_ACT,    "Your %s glows.")
OF(HOLD_LIFE,   "Your %s glows.")
OF(SEEING,     "")
OF(FEARLESS,     "")
OF(LIGHT,     "")
OF(TWO_HANDED_REQ,     "")
OF(TWO_HANDED_DES,     "")
OF(BLESSED,     "")
OF(IMPACT,     "")
OF(ACID_PROOF,     "")
OF(ELEC_PROOF,     "")
OF(FIRE_PROOF,     "")
OF(COLD_PROOF,     "")
OF(SHOW_MODS,     "")
OF(SHOW_CURSE,     "")
OF(PERMA_CURSE,     "")
OF(FRAGILE,     "")
OF(DARKNESS,     "")
OF(CHAOTIC,     "")
