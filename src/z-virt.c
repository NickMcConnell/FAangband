/** \file z-virt.c
    \brief Memory management routines
 
 * Copyright (c) 1997 Ben Harrison.
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
#include "z-virt.h"
#include "z-util.h"



/**
 * Optional auxiliary "rpanic" function
 */
void* (*rpanic_aux)(size_t) = NULL;

/**
 * The system is out of memory, so panic.  If "rpanic_aux" is set,
 * it can be used to free up some memory and do a new "ralloc()",
 * or if not, it can be used to save things, clean up, and exit.
 * By default, this function simply quits the computer.
 */
void* rpanic(size_t len)
{
	/* Hopefully, we have a real "panic" function */
	if (rpanic_aux) return ((*rpanic_aux)(len));

	/* Attempt to quit before icky things happen */
	quit("Out of Memory!");

	/* Paranoia */
	return (NULL);
}


/**
 * Optional auxiliary "ralloc" function
 */
void* (*ralloc_aux)(size_t) = NULL;


/**
 * Allocate some memory
 */
void* ralloc(size_t len)
{
	void *mem;

	/* Allow allocation of "zero bytes" */
	if (len == 0) return (NULL);

	/* Use the aux function if set */
	if (ralloc_aux) mem = (*ralloc_aux)(len);

	/* Use malloc() to allocate some memory */
	else mem = malloc(len);

	/* We were able to acquire memory */
	if (!mem) mem = rpanic(len);

	/* Return the memory, if any */
	return (mem);
}


/**
 * Optional auxiliary "rnfree" function
 */
void* (*rnfree_aux)(void*) = NULL;


/**
 * Free some memory (allocated by ralloc), return NULL
 */
void* rnfree(void *p)
{
	/* Easy to free nothing */
	if (!p) return (NULL);

	/* Use the "aux" function */
	if (rnfree_aux) return ((*rnfree_aux)(p));

	/* Use "free" */
	free(p);

	/* Done */
	return (NULL);
}

/**
 * Hooks for platform-specific memory allocation.
 */
static mem_realloc_hook realloc_aux;


/**
 * Set the hooks for the memory system.
 */
bool mem_set_hooks(mem_alloc_hook alloc, mem_free_hook free, mem_realloc_hook realloc)
{
	/* Error-check */
	if (!alloc || !free || !realloc) return FALSE;

	/* Set up hooks */
	ralloc_aux = alloc;
	rnfree_aux = free;
	realloc_aux = realloc;

	return TRUE;
}


/**
 * Allocate `len` bytes of memory.
 *
 * Returns:
 *  - NULL if `len` == 0; or
 *  - a pointer to a block of memory of at least `len` bytes
 *
 * Doesn't return on out of memory.
 */
void *mem_alloc(size_t len)
{
	void *mem;

	/* Allow allocation of "zero bytes" */
	if (len == 0) return (NULL);

	/* Allocate some memory */
	if (ralloc_aux) mem = (*ralloc_aux)(len);
	else            mem = malloc(len);

	/* Handle OOM */
	if (!mem) quit("Out of Memory!");

	return mem;
}


/**
 * Free the memory pointed to by `p`.
 *
 * Returns NULL.
 */
void *mem_free(void *p)
{
	/* Easy to free nothing */
	if (!p) return (NULL);

	/* Free memory */
	if (rnfree_aux) (*rnfree_aux)(p);
	else            free(p);

	/* Done */
	return (NULL);
}


/**
 * Allocate `len` bytes of memory, copying whatever is in `p` with it.
 *
 * Returns:
 *  - NULL if `len` == 0 or `p` is NULL; or
 *  - a pointer to a block of memory of at least `len` bytes
 *
 * Doesn't return on out of memory.
 */
void *mem_realloc(void *p, size_t len)
{
	void *mem;

	/* Fail gracefully */
	if (!p || len == 0) return (NULL);

	if (realloc_aux) mem = (*realloc_aux)(p, len);
	else             mem = realloc(p, len);

	/* Handle OOM */
	if (!mem) quit("Out of Memory!");

	return mem;
}


/**
 * Allocate a constant string, containing the same thing as 'str'
 */
char *string_make(const char *str)
{
	char *res;
	size_t siz;

	/* Simple sillyness */
	if (!str) return NULL;

	/* Allocate space for the string including terminator */
	siz = strlen(str) + 1;
	res = mem_alloc(siz);

	/* Copy the string (with terminator) */
	my_strcpy(res, str, siz);

	/* Return the allocated and initialized string */
	return (res);
}


/**
 * Un-allocate a string allocated above.
 */
#undef string_free
char *string_free(char *str)
{
	/* Kill the buffer of chars we must have allocated above */
	return mem_free(str);
}
