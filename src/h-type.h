/** \file h-type.h 
    \brief Basic "types".
 
 * Note the attempt to make all basic types have 4 letters.
 * This improves readibility and standardizes the code.
 *
 * Likewise, all complex types are at least 4 letters.
 * Thus, almost every three letter word is a legal variable.
 * But beware of certain reserved words ('for' and 'if' and 'do').
 *
 * Note that the type used in structures for bit flags should be uint.
 * As long as these bit flags are sequential, they will be space smart.
 *
 * It must be true that char/byte takes exactly 1 byte
 * It must be true that s16b/u16b takes exactly 2 bytes
 * It must be true that s32b/u32b takes exactly 4 bytes
 *
 * Also, see <limits.h> for min/max values for sind, uind, long, huge
 * (SHRT_MIN, SHRT_MAX, USHRT_MAX, LONG_MIN, LONG_MAX, ULONG_MAX)
 * These limits should be verified and coded into "h-constant.h".
 */


#ifndef INCLUDED_H_TYPE_H
#define INCLUDED_H_TYPE_H


/*** Special 4 letter names for some standard types ***/


/* A standard pointer (to "void" because ANSI C says so) */
typedef void *vptr;

/* A simple pointer (to unmodifiable strings) */
typedef const char *cptr;


/* Error codes for function return values */
/* Success = 0, Failure = -N, Problem = +N */
typedef int errr;


/*
 * Hack -- prevent problems with non-MACINTOSH
 */
#undef uint
#define uint uint_hack

/*
 * Hack -- prevent problems with MSDOS and WINDOWS
 */
#undef huge
#define huge huge_hack

/*
 * Hack -- prevent problems with AMIGA
 */
#undef byte
#define byte byte_hack

/*
 * Hack -- prevent problems with C++
 */
#undef bool
#define bool bool_hack


/* Note that unsigned values can cause math problems */
/* An unsigned byte of memory */
typedef unsigned char byte;


/* Simple True/False type */
typedef char bool;


/* A signed, standard integer (at least 2 bytes) */
typedef int sint;

/* An unsigned, "standard" integer (often pre-defined) */
typedef unsigned int uint;


/* The largest possible unsigned integer */
typedef unsigned long huge;


/* Signed/Unsigned 16 bit value */
typedef signed short s16b;
typedef unsigned short u16b;

/* Signed/Unsigned 32 bit value */
#ifdef L64	/* 64 bit longs */
typedef signed int s32b;
typedef unsigned int u32b;
#else
typedef signed long s32b;
typedef unsigned long u32b;
#endif




/*** Pointers to Functions with simple return types and any args ***/

/* Replace and remove or move to main-acn.c */
typedef errr	(*func_errr)();
typedef bool	(*func_bool)();



#endif /* INCLUDED_H_TYPE_H */

