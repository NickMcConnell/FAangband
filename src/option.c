/*
 * File: options.c
 * Purpose: Options table and definitions.
 *
 * Copyright (c) 1997 Ben Harrison
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
#include "angband.h"
#include "option.h"

typedef struct {
	const char *name;
	const char *description;
	int type;
	bool normal;
} option_entry;

int option_page[OPT_PAGE_MAX][OPT_PAGE_PER] = { {0} };

static option_entry options[OPT_MAX] = {
#define OP(a, b, c, d)    { #a, b, OP_##c, d },
#include "list-options.h"
#undef OP
};


/* Accessor functions */
const char *option_name(int opt)
{
	if (opt >= OPT_MAX)
		return NULL;
	return options[opt].name;
}

const char *option_desc(int opt)
{
	if (opt >= OPT_MAX)
		return NULL;
	return options[opt].description;
}

int option_type(int opt)
{
	if (opt >= OPT_MAX)
		return 0;
	return options[opt].type;
}

/* Setup functions */
bool option_set(const char *name, bool on)
{
	size_t opt;
	for (opt = 0; opt < OPT_MAX; opt++) {
		/* Look for matching names */
		if (!options[opt].name || !streq(options[opt].name, name))
			continue;

		/* Set the option */
		op_ptr->opt[opt] = on;

		/* If a cheat option is set on, set the corresponding score option */
		if (on && (option_type(opt) == OP_CHEAT))
			op_ptr->opt[opt + 1] = TRUE;

		return TRUE;
	}

	/* Didn't find it */
	return FALSE;
}

void init_options(void)
{
	int opt, page;

	/* Allocate options to pages */
	for (page = 0; page < OPT_PAGE_MAX; page++) {
		int page_opts = 0;
		for (opt = 0; opt < OPT_MAX; opt++) {
			if ((options[opt].type == page) && (page_opts < OPT_PAGE_PER))
				option_page[page][page_opts++] = opt;
		}
		while (page_opts < OPT_PAGE_PER)
			option_page[page][page_opts++] = OPT_none;
	}

	/* Set defaults */
	for (opt = 0; opt < OPT_MAX; opt++) {
		op_ptr->opt[opt] = options[opt].normal;
	}
}
