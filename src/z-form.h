/** \file z-form.h 
    \brief Low-level formatting include

 *
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 *

 *
 * This file provides functions very similar to "sprintf()", but which
 * not only parse some additional "format sequences", but also enforce
 * bounds checking, and allow repeated "appends" to the same buffer.
 *
 * See z-form.c for more detailed information about the routines,
 * including a list of the legal "format sequences".
 *
 * This file makes use of both z-util.c and z-virt.c
 */

#ifndef INCLUDED_Z_FORM_H
#define INCLUDED_Z_FORM_H

#include "h-basic.h"

/*
 * Modes of operation for the "xstr_trans()" function.
 */
#define LATIN1  0
#define SYSTEM_SPECIFIC 1
#define ASCII 2


/**
 * An extended character translation.  Using a tag,
 * get a 8-bit character.  Or, using an 8-bit character,
 * get a tag.
 */
typedef struct xchar_type xchar_type;
struct xchar_type
{
	cptr tag;
	byte c;
};


/**** Available Functions ****/

extern void xchar_trans_hook(char *s, int encoding);

/** Get encodes in a string */
extern bool get_encode(char *str, char *c);

/** Translate an ASCII string into Latin-1, etc., using encodes */
extern void xstr_trans(char *str, int encoding);

/** Translate a Latin-1 string into escaped ASCII */
extern void escape_latin1(char *dest, size_t max, cptr src);

/** Format arguments into given bounded-length buffer */
extern size_t vstrnfmt(char *buf, size_t max, cptr fmt, va_list vp);

/** Simple interface to "vstrnfmt()" */
extern size_t strnfmt(char *buf, size_t max, cptr fmt, ...);

/** Format arguments into a static resizing buffer */
extern char *vformat(cptr fmt, va_list vp);

/** Free the memory allocated for the format buffer */
extern void vformat_kill(void);

/** Append a formatted string to another string */
extern void strnfcat(char *str, size_t max, size_t *end, cptr fmt, ...);

/** Simple interface to "vformat()" */
extern char *format(cptr fmt, ...);

/** Vararg interface to "plog()", using "format()" */
extern void plog_fmt(cptr fmt, ...);

/** Vararg interface to "quit()", using "format()" */
extern void quit_fmt(cptr fmt, ...);

/** Vararg interface to "core()", using "format()" */
extern void core_fmt(cptr fmt, ...);


/* Column titles for character information table */
#define COLUMN_TO_UPPER 	0
#define COLUMN_TO_LOWER 	1
#define COLUMN_CHAR_TYPE	2

#define CHAR_TABLE_SLOTS	3

/* Bit flags for COLUMN_CHAR_TYPE */
#define CHAR_BLANK  0x01
#define CHAR_UPPER  0x02
#define CHAR_LOWER  0x04
#define CHAR_PUNCT  0x08
#define CHAR_SYMBOL 0x10
#define CHAR_DIGIT  0x20
#define CHAR_VOWEL  0x40
#define CHAR_XXXX3  0x80

/** Character information table */
extern byte char_tables[256][CHAR_TABLE_SLOTS];


/*
 * Set of customized macros for use with 256 character set
 */
#define	my_isupper(Y) \
		(char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_UPPER))

#define	my_islower(Y) \
		(char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_LOWER))

#define	my_isalpha(Y) \
		(char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_UPPER | CHAR_LOWER))

#define	my_isspace(Y) \
		(char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_BLANK))

#define my_is_vowel(Y) \
        (char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_VOWEL))

#define	my_ispunct(Y) \
        (char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_PUNCT))

#define	my_isdigit(Y) \
         (char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_DIGIT))

#define	my_isalnum(Y) \
        (char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_UPPER | CHAR_LOWER | CHAR_DIGIT))

#define	my_isprint(Y) \
 		(char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_BLANK | CHAR_UPPER | CHAR_LOWER | \
 	 									           CHAR_PUNCT | CHAR_DIGIT))

/* Note: the regular is_graph does not have the CHAR_SYMBOL check) */
#define	my_isgraph(Y) \
		(char_tables[(byte)(Y)][COLUMN_CHAR_TYPE] & (CHAR_UPPER | CHAR_LOWER | \
	 									           CHAR_PUNCT | CHAR_DIGIT | CHAR_SYMBOL))

#define my_toupper(Y) \
		(char_tables[(byte)(Y)][COLUMN_TO_UPPER])

#define my_tolower(Y) \
		(char_tables[(byte)(Y)][COLUMN_TO_LOWER])



#endif /* INCLUDED_Z_FORM_H */
