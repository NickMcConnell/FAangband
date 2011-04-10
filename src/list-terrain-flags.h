/* list-terrain-flags.h - monster race blow effects
 *
 * Adjusting these flags does not break savefiles. Flags below start from 1
 * on line 11, so a flag's sequence number is its line number minus 10.
 *
 *
 */

/*  symbol     descr */
TF(NONE,        "")
TF(LOS,        "LOS passes through this")
TF(PROJECT,    "Projections can pass through this")
TF(PASSABLE,   "Can be passed through by all creatures")
TF(INTERESTING,"Looking around notices this")
TF(PERMANENT,  "Feature is permanent")
TF(NO_SCENT,   "Cannot store scent")
TF(OBJECT,     "Can hold objects")
TF(TORCH_ONLY, "Becomes bright only when torch-lit")
TF(FLOOR,      "A clear floor")
TF(WALL,       "Is a solid wall")
TF(ROCK,       "Is rocky")
TF(GRANITE,    "Is a granite rock wall")
TF(DOOR_ANY,   "Is any door")
TF(DOOR_CLOSED,"Is a closed door")
TF(SHOP,       "Is a shop")
TF(TREE,       "Is a tree")
TF(TRAP,       "Is a trap")
TF(TRAP_INVIS, "Is an invisible trap ")
TF(M_TRAP,     "Is a monster trap")
TF(STAIR,      "Is a stair or path")
TF(RUNE,       "Is a rune")
TF(DOOR_JAMMED,"Is a jammed door")

