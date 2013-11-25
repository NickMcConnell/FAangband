/* File: init.h */

/*
 * Copyright (c) 2000 Robert Ruehlmann
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#ifndef INCLUDED_INIT_H
#define INCLUDED_INIT_H

#include "h-basic.h"
#include "z-bitflag.h"
#include "z-file.h"
#include "z-rand.h"
#include "parser.h"

/**
 * Information about maximal indices of certain arrays
 * Actually, these are not the maxima, but the maxima plus one
 */
typedef struct maxima {
    u16b f_max;		/**< Max size for "f_info[]" */
    u16b trap_max;	/**< Max size for "trap_info[]" */
    u16b k_max;		/**< Max size for "k_info[]" */
    u16b a_max;		/**< Max size for "a_info[]" */
    u16b e_max;		/**< Max size for "e_info[]" */
    u16b r_max;		/**< Max size for "r_info[]" */
    u16b mp_max;        /**< Maximum number of monster pain message sets */
    u16b v_max;		/**< Max size for "v_info[]" */
    u16b t_max;		/**< Max size for "t_info[]" */
    u16b p_max;		/**< Max size for "p_info[]" */
    u16b h_max;		/**< Max size for "h_info[]" */
    u16b b_max;		/**< Max size per element of "b_info[]" */
    u16b c_max;		/**< Max size for "c_info[]" */
    u16b flavor_max;	/**< Max size for "flavor_info[]" */
    u16b s_max;		/**< Max size for "s_info[]" */
    u16b set_max;	/**< Max size for "set_info[]" */
    u16b pit_max;	/**< Maximum number of monster pit types */

    u16b o_max;		/**< Max size for "o_list[]" */
    u16b m_max;		/**< Max size for "mon_list[]" */
    u16b l_max;		/**< Max size for "trap_list[]" */
} maxima;



extern maxima *z_info;

//#ifdef TEST
extern struct parser *init_parse_a(void);
extern struct parser *init_parse_c(void);
extern struct parser *init_parse_e(void);
extern struct parser *init_parse_f(void);
extern struct parser *init_parse_h(void);
extern struct parser *init_parse_k(void);
extern struct parser *init_parse_p(void);
extern struct parser *init_parse_r(void);
extern struct parser *init_parse_s(void);
extern struct parser *init_parse_v(void);
extern struct parser *init_parse_z(void);
extern struct parser *init_parse_flavor(void);
extern struct parser *init_parse_names(void);
extern struct parser *init_parse_hints(void);
//#endif

extern errr parse_file(struct parser *p, const char *filename);

extern void init_file_paths(const char *config, const char *lib, const char *data);
extern errr init_race_probs(void);
extern void create_needed_dirs(void);
extern bool init_angband(void);
extern void cleanup_angband(void);

#endif /* INCLUDED_INIT_H */
