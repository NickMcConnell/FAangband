/** \file z-virt.h 
    \brief Memery management include
 *
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 *

 
 * Memory management routines.
 *
 * Set ralloc_aux to modify the memory allocation routine.
 * Set rnfree_aux to modify the memory de-allocation routine.
 * Set rpanic_aux to let the program react to memory failures.
 *
 * These routines work best as a *replacement* for malloc/free.
 *
 * The string_make() and string_free() routines handle dynamic strings.
 * A dynamic string is a string allocated at run-time, which should not
 * be modified once it has been created.
 *
 * Note the macros below which simplify the details of allocation,
 * deallocation, setting, clearing, casting, size extraction, etc.
 *
 * The macros MAKE/C_MAKE and KILL have a "procedural" metaphor,
 * and they actually modify their arguments.
 *
 * Note that, for some reason, some allocation macros may disallow
 * "stars" in type names, but you can use typedefs to circumvent
 * this.  For example, instead of "type **p; MAKE(p,type*);" you
 * can use "typedef type *type_ptr; type_ptr *p; MAKE(p,type_ptr)".
 *
 * Note that it is assumed that "memset()" will function correctly,
 * in particular, that it returns its first argument.
 */


#ifndef INCLUDED_Z_VIRT_H
#define INCLUDED_Z_VIRT_H

#include "h-basic.h"


/**** Available macros ****/


/* Set every byte in an array of type T[N], at location P, to V, and return P */
#define C_BSET(P, V, N, T) \
	(memset((P), (V), (N) * sizeof(T)))

/* Set every byte in a thing of type T, at location P, to V, and return P */
#define BSET(P, V, T) \
	(memset((P), (V), sizeof(T)))


/* Wipe an array of type T[N], at location P, and return P */
#define C_WIPE(P, N, T) \
	(memset((P), 0, (N) * sizeof(T)))

/* Wipe a thing of type T, at location P, and return P */
#define WIPE(P, T) \
	(memset((P), 0, sizeof(T)))


/* Load an array of type T[N], at location P1, from another, at location P2 */
#define C_COPY(P1, P2, N, T) \
	(memcpy((P1), (P2), (N) * sizeof(T)))

/* Load a thing of type T, at location P1, from another, at location P2 */
#define COPY(P1, P2, T) \
	(memcpy((P1), (P2), sizeof(T)))


/* Allocate, and return, an array of type T[N] */
#define C_RNEW(N, T) \
	(ralloc((N) * sizeof(T)))

/* Allocate, and return, a thing of type T */
#define RNEW(T) \
	(ralloc(sizeof(T)))


/* Allocate, wipe, and return an array of type T[N] */
#define C_ZNEW(N, T) \
	(C_WIPE(C_RNEW(N, T), N, T))

/* Allocate, wipe, and return a thing of type T */
#define ZNEW(T) \
	(WIPE(RNEW(T), T))


/* Allocate a wiped array of type T[N], assign to pointer P */
#define C_MAKE(P, N, T) \
	((P) = C_ZNEW(N, T))

/* Allocate a wiped thing of type T, assign to pointer P */
#define MAKE(P, T) \
	((P) = ZNEW(T))


/* Free one thing at P, return NULL */
#define FREE(P) \
	(rnfree(P))

/* Free a thing at location P and set P to NULL */
#define KILL(P) \
	((P)=FREE(P))



/**** Available variables ****/

/* Replacement hook for "rnfree()" */
extern void* (*rnfree_aux)(void*);

/* Replacement hook for "rpanic()" */
extern void* (*rpanic_aux)(size_t);

/* Replacement hook for "ralloc()" */
extern void* (*ralloc_aux)(size_t);


/**** Available functions ****/

/* De-allocate memory */
extern void* rnfree(void *p);

/* Panic, attempt to allocate 'len' bytes */
extern void* rpanic(size_t len);

/* Allocate (and return) 'len', or quit */
extern void* ralloc(size_t len);

/*** Initialisation bits ***/

/* Hook types for memory allocation */
typedef void *(*mem_alloc_hook)(size_t);
typedef void *(*mem_free_hook)(void *);
typedef void *(*mem_realloc_hook)(void *, size_t);

/* Set up memory allocation hooks */
bool mem_set_hooks(mem_alloc_hook alloc, mem_free_hook free, mem_realloc_hook realloc);


/**** Normal bits ***/

/* Allocate (and return) 'len', or quit */
void *mem_alloc(size_t len);

/* De-allocate memory */
void *mem_free(void *p);

/* Reallocate memory */
void *mem_realloc(void *p, size_t len);

/* Create a "dynamic string" */
extern char *string_make(const char *str);

/* Free a string allocated with "string_make()" */
extern char *string_free(char *str);

#endif /* INCLUDED_Z_VIRT_H */
