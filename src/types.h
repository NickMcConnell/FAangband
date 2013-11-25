#ifndef INCLUDED_TYPES_H
#define INCLUDED_TYPES_H

/** \file types.h 
    \brief Miscellaneous type definitions
*/

typedef struct alloc_entry alloc_entry;


/**
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */
struct alloc_entry {
    s16b index;		/**< The actual index */

    byte level;		/**< Base dungeon level */
    byte prob1;		/**< Probability, pass 1 */
    byte prob2;		/**< Probability, pass 2 */
    byte prob3;		/**< Probability, pass 3 */

    u16b total;		/**< Unused for now */
};



/**
 * A hint.
 */
struct hint
{
        char *hint;
        struct hint *next;
};

#endif /* INCLUDED_TYPES_H */
