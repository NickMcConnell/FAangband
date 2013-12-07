/* list-cave-flags.h - special grid flags
 *
 * Adjusting these flags does not break savefiles. Flags below start from 1
 * on line 11, so a flag's sequence number is its line number minus 10.
 *
 *
 */

/*  symbol     descr */
SQUARE(NONE,     "")
SQUARE(MARK,     "memorized feature")
SQUARE(GLOW,     "self-illuminating")
SQUARE(ICKY,     "part of a vault")
SQUARE(ROOM,     "part of a room")
SQUARE(SEEN,     "seen flag")
SQUARE(VIEW,     "view flag")
SQUARE(TEMP,     "temp flag")
SQUARE(WALL,     "wall flag")
SQUARE(DTRAP,    "trap detected grid")
SQUARE(TRAP,     "grid containing a known trap")
SQUARE(INVIS,    "grid containing an unknown trap")
SQUARE(DEDGE,    "trap detection edge grid")
