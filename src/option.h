#ifndef INCLUDED_OPTIONS_H
#define INCLUDED_OPTIONS_H


/*
 * Option types 
 */
enum
{
	OP_INTERFACE = 0,
	OP_GAMEPLAY,
	OP_CHEAT,
	OP_SCORE,
	OP_SPECIAL,

        OP_MAX
};

/*
 * Information for "do_cmd_options()".
 */
#define OPT_PAGE_MAX				OP_SCORE
#define OPT_PAGE_PER				16

/* The option data structures */
extern int option_page[OPT_PAGE_MAX][OPT_PAGE_PER];

const char *option_name(int opt);
const char *option_desc(int opt);
int option_type(int opt);

bool option_set(const char *opt, bool on);
void init_options(void);


/*
 * Indexes
 */
enum
{
        #define OP(a, b, c, d) OPT_##a,
	#include "list-options.h"
	#undef OP
	OPT_MAX
};

#define OPT(opt_name)	op_ptr->opt[OPT_##opt_name]

#endif /* !INCLUDED_OPTIONS_H */
