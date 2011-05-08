/* squelch.h - squelch interface */

#ifndef SQUELCH_H
#define SQUELCH_H

#define TYPE_MAX 19

/**
 * The different kinds of quality squelch
 */
enum
{
    SQUELCH_NONE,	
    SQUELCH_CURSED,
    SQUELCH_DUBIOUS,   
    SQUELCH_DUBIOUS_NON,   
    SQUELCH_NON_EGO,
    SQUELCH_AVERAGE,    
    SQUELCH_GOOD_STRONG,    
    SQUELCH_GOOD_WEAK,
    SQUELCH_ALL,	

    SQUELCH_MAX
};


/**
 * Structure to describe tval/description pairings. 
 */
typedef struct
{
  int tval;
  const char *desc;
} tval_desc;

/* squelch.c  */
extern byte squelch_level[TYPE_MAX];
void squelch_birth_init(void);
int get_autoinscription_index(s16b k_idx);
const char *get_autoinscription(s16b kind_idx);
int apply_autoinscription(object_type *o_ptr);
int remove_autoinscription(s16b kind);
int add_autoinscription(s16b kind, cptr inscription);
void autoinscribe_ground(void);
void autoinscribe_pack(void);
bool squelch_tval(int tval);
extern bool squelch_item_ok(const object_type *o_ptr);
bool squelch_hide_item(object_type *o_ptr);
extern void squelch_drop(void);
void squelch_items(void);
extern bool seen_tval(int tval);
void do_cmd_options_item(const char *name, int row);



#endif /* !SQUELCH_H */
