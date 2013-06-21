/* squelch.h - squelch interface */

#ifndef SQUELCH_H
#define SQUELCH_H

#define  Q_TV_MAX  19
#define  S_TV_MAX  16


/**
 * The different kinds of quality squelch
 */
enum
{
    SQUELCH_NONE,	
    SQUELCH_KNOWN_PERILOUS,   
    SQUELCH_KNOWN_DUBIOUS,   
    SQUELCH_AVERAGE,    
    SQUELCH_KNOWN_GOOD,    
    SQUELCH_KNOWN_EXCELLENT,    
    SQUELCH_FELT_DUBIOUS,   
    SQUELCH_FELT_GOOD,   

    SQUELCH_MAX
};

#define SQUELCH_KNOWN_MAX SQUELCH_FELT_DUBIOUS

enum
{
    DESTROY_THIS_ITEM,
    DESTROY_CURSED,
    DESTROY_DUBIOUS,   
    DESTROY_DUBIOUS_NON,   
    DESTROY_NON_EGO,
    DESTROY_AVERAGE,    
    DESTROY_GOOD_STRONG,    
    DESTROY_GOOD_WEAK,
    DESTROY_ALL,	
    DESTROY_THIS_FLAVOR,
    DESTROY_THIS_EGO
};

/**
 * Structure to describe tval/description pairings. 
 */
typedef struct
{
  int tval;
  const char *desc;
} tval_desc;

/**
 * Structure to describe ego item short name. 
 */
typedef struct ego_desc
{
  s16b e_idx;
  const char *short_name;
} ego_desc;

/**
 * Structure to describe squelch levels. 
 */
typedef struct
{
    int enum_val;
    const char *heavy_desc;
    const char *light_desc;
    const char *adjective;
} quality_desc_struct;


/* squelch.c  */
tval_desc quality_choices[Q_TV_MAX];
quality_desc_struct quality_strings[SQUELCH_MAX];
extern bool squelch_profile[Q_TV_MAX][SQUELCH_MAX];
tval_desc sval_dependent[S_TV_MAX];
void squelch_birth_init(void);
int get_autoinscription_index(s16b k_idx);
const char *get_autoinscription(s16b kind_idx);
int apply_autoinscription(object_type *o_ptr);
int remove_autoinscription(s16b kind);
int add_autoinscription(s16b kind, const char *inscription);
void autoinscribe_ground(void);
void autoinscribe_pack(void);
int squelch_type_of(const object_type *o_ptr);
bool squelch_tval(int tval);
int feel_to_squelch_level(int feel);
extern bool squelch_item_ok(const object_type *o_ptr);
bool squelch_hide_item(object_type *o_ptr);
extern void squelch_drop(void);
void squelch_items(void);

/* ui-options.c */
int ego_item_name(char *buf, size_t buf_size, ego_desc *d_ptr);
extern bool seen_tval(int tval);
void do_cmd_options_item(const char *name, int row);


#endif /* !SQUELCH_H */
