#ifndef INCLUDED_STORE_H
#define INCLUDED_STORE_H

/**
 * Total number of stores (see "store.c", etc)
 */
#define MAX_STORES       60

/**
 * Number of stores in a large town (see "store.c", etc)
 */
#define MAX_STORES_BIG    9

/**
 * Number of stores in a small town (see "store.c", etc)
 */
#define MAX_STORES_SMALL  4

/**
 * Number of store types
 */
#define MAX_STORE_TYPES  10

/*
 * Store numbers
 */
#define STORE_GEN 		0
#define STORE_ARMORY 	1
#define STORE_WEAPON 	2
#define STORE_TEMPLE 	3
#define STORE_ALCH 		4
#define STORE_MAGIC 	5
#define STORE_BLACKM 	6
#define STORE_HOME 		7
#define STORE_BOOK 		8
#define STORE_MERCH     9

/**<*
 * A store owner
 */
struct owner_type {
    struct owner_type *next;
    int bidx;
    char *owner_name;	/**< Name */
    u32b unused;	/**< Currently unused */

    s16b max_cost;	/**< Purse limit */

    int inflate;	/**< Inflation */
    byte owner_race;	/**< Owner race */

};




/**
 * A store, with an owner, various state flags, a current stock
 * of items, and a table of items that are often purchased.
 */
struct store_type {
    byte owner;		/**< Owner index */
    byte type;		/**< What type of store it is -NRM- */

    s16b insult_cur;	/**< Insult counter */

    s16b good_buy;	/**< Number of "good" buys */
    s16b bad_buy;	/**< Number of "bad" buys */

    s32b store_open;	/**< Closed until this turn */

    s32b store_wrap;	/**< Unused for now */

    s16b table_num;	/**< Table -- Number of entries */
    s16b table_size;	/**< Table -- Total Size of Array */
    s16b *table;	/**< Table -- Legal item kinds */

    s16b stock_num;	/**< Stock -- Number of entries */
    s16b stock_size;	/**< Stock -- Total Size of Array */
    object_type *stock;
			/**< Stock -- Actual stock items */
};

extern byte type_of_store[MAX_STORES];


s32b price_item(object_type * o_ptr, int greed, bool flip);
extern void store_shuffle(int which);
extern void store_maint(int which);
extern void stores_maint(int times);
extern void store_init(void);

#endif /* INCLUDED_STORE_H */
