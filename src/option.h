#ifndef INCLUDED_OPTIONS_H
#define INCLUDED_OPTIONS_H


/*
 * Option types 
 */
enum
{
	OP_SPECIAL = 0,
	OP_INTERFACE,
	OP_GAMEPLAY,
	OP_CHEAT,
	OP_SCORE,

        OP_MAX
};

/*
 * Information for "do_cmd_options()".
 */
#define OPT_PAGE_MAX				OP_CHEAT
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


/*** Option definitions (hack waiting for game modes)  ***/

#define OPT_ADULT					32
#define OPT_adult_no_sell               (OPT_ADULT+4)
#define OPT_adult_ironman               (OPT_ADULT+5)
#define OPT_adult_thrall                (OPT_ADULT+6)
#define OPT_adult_small_device          (OPT_ADULT+7)
#define OPT_adult_dungeon               (OPT_ADULT+8)
#define OPT_adult_no_artifacts          (OPT_ADULT+9)
#define OPT_adult_no_stairs             (OPT_ADULT+10)
#define OPT_adult_ai_cheat              (OPT_ADULT+11)
#define OPT_adult_auto_scum             (OPT_ADULT+12)
#define OPT_adult_compressed            (OPT_ADULT+13)

#endif /* !INCLUDED_OPTIONS_H */
