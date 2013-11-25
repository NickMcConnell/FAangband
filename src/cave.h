/* cave.h - cave interface */

#ifndef CAVE_H
#define CAVE_H

#include "defines.h"
#include "z-type.h"

/**
 * Maximum size of the "temp" array (see "cave.c")
 * Note that we must be as large as "VIEW_MAX" for proper functioning
 * of the "update_view()" function, and we must also be as large as the
 * largest illuminatable room, but no room is larger than 800 grids.  We
 * must also be large enough to allow "good enough" use as a circular queue,
 * to calculate monster flow, but note that the flow code is "paranoid".
 */
#define TEMP_MAX 1536

/**
 * Maximum distance from the character to store flow (noise) information
 */
#define NOISE_STRENGTH 45

/**
 * Character turns it takes for smell to totally dissipate
 */
#define SMELL_STRENGTH 60

/*** Feature Indexes (see "lib/edit/terrain.txt") ***/

/** Nothing */
#define FEAT_NONE	0x00

/* Various */
#define FEAT_FLOOR	0x01
#define FEAT_INVIS	0x02
#define FEAT_OPEN	0x03
#define FEAT_BROKEN	0x04
#define FEAT_MORE	0x05
#define FEAT_LESS	0x06
#define FEAT_MORE_SHAFT	0x07
#define FEAT_LESS_SHAFT	0x08

/* Doors */
#define FEAT_DOOR_HEAD	0x20
#define FEAT_DOOR_TAIL	0x2F

/* Extra */
#define FEAT_SECRET	0x30
#define FEAT_RUBBLE	0x31

/* Seams */
#define FEAT_MAGMA	0x32
#define FEAT_QUARTZ	0x33
#define FEAT_MAGMA_H	0x34
#define FEAT_QUARTZ_H	0x35
#define FEAT_MAGMA_K	0x36
#define FEAT_QUARTZ_K	0x37

/* Walls */
#define FEAT_WALL_EXTRA	0x38
#define FEAT_WALL_INNER	0x39
#define FEAT_WALL_OUTER	0x3A
#define FEAT_WALL_SOLID	0x3B
#define FEAT_PERM_EXTRA	0x3C
#define FEAT_PERM_INNER	0x3D
#define FEAT_PERM_OUTER	0x3E
#define FEAT_PERM_SOLID	0x3F

/*
 * Oangband shops are moved to 0x40 to 0x48 to make room for
 * the extra bookstore.  Method borrowed from Zangband. -LM-
 * Nother one. -NRM-
 */
#define FEAT_SHOP_HEAD 0x40
#define FEAT_SHOP_HOME 0x47
#define FEAT_SHOP_TAIL 0x49

/* "stairs" in wilderness -NRM- */
#define FEAT_LESS_NORTH		0x60
#define FEAT_MORE_NORTH		0x61
#define FEAT_LESS_EAST		0x62
#define FEAT_MORE_EAST		0x63
#define FEAT_LESS_SOUTH		0x64
#define FEAT_MORE_SOUTH		0x65
#define FEAT_LESS_WEST		0x66
#define FEAT_MORE_WEST		0x67

/* Special dungeon/wilderness features. -LM- Expanded for FAangband 0.3.4 */
#define FEAT_MIN_SPECIAL        0x70
#define FEAT_LAVA		0x70
#define FEAT_WATER      	0x71
#define FEAT_TREE               0x72
#define FEAT_TREE2		0x73
#define FEAT_GRASS              0x74  
#define FEAT_ROAD               0x75
#define FEAT_VOID               0x77  
#define FEAT_PIT                0x78  
#define FEAT_DUNE               0x7a

/*** Terrain flags ***/

enum
{
	#define TF(a,b) TF_##a,
	#include "list-terrain-flags.h"
	#undef TF
	TF_MAX
};

#define TF_SIZE                FLAG_SIZE(TF_MAX)

#define tf_has(f, flag)        flag_has_dbg(f, TF_SIZE, flag, #f, #flag)


/**
 * Information about terrain "features"
 */
typedef struct feature {
    char *name;		/**< Name  */
    char *text;		/**< Text  */
    int fidx;

    struct feature *next;

    byte mimic;		/**< Feature to mimic */
    byte priority; /**< Display priority */
    
    byte locked;   /**< How locked is it? */
    byte jammed;   /**< How jammed is it? */
    byte shopnum;  /**< Which shop does it take you to? */
    random_value dig;      /**< How hard is it to dig through? */

    bitflag flags[TF_SIZE];		/**< Bitflags */


    byte d_attr;	/**< Default feature attribute */
    wchar_t d_char;	/**< Default feature character */


    byte x_attr[3];   /**< Desired feature attribute (set by user/pref file) */
    wchar_t x_char[3];   /**< Desired feature character (set by user/pref file) */
} feature_type;


enum grid_light_level
{
	FEAT_LIGHTING_BRIGHT = 0,
	FEAT_LIGHTING_LIT,
	FEAT_LIGHTING_DARK,
	FEAT_LIGHTING_MAX
};

typedef struct
{
    u32b m_idx;		/* Monster index */
    u32b f_idx;		/* Feature index */
    u32b first_k_idx;	/* The "Kind" of the first item on the grid */
    u32b trap;          /* Trap index */
    bool multiple_objects;	/* Is there more than one item there? */
    
    enum grid_light_level lighting; /* Light level */
    bool in_view; /* TRUE when the player can currently see the grid. */
    bool is_player;
    bool hallucinate;
    bool trapborder;
} grid_data;


/**** Available Types ****/


/**
 * An array of 256 cave bitflag arrays
 */
typedef bitflag grid_256[256][SQUARE_SIZE];

/**
 * An array of DUNGEON_WID byte's
 */
typedef byte byte_wid[DUNGEON_WID];

/**
 * An array of DUNGEON_WID s16b's
 */
typedef s16b s16b_wid[DUNGEON_WID];

/**
 * An array of NUM_STAGES u16b's
 */
typedef u16b u16b_stage[NUM_STAGES];



extern int distance(int y1, int x1, int y2, int x2);
extern bool los(int y1, int x1, int y2, int x2);
extern bool no_light(void);
extern bool cave_valid_bold(int y, int x);
extern byte get_color(byte a, int attr, int n);
extern void grid_data_as_text(grid_data *g, int *ap, wchar_t *cp, byte *tap, wchar_t *tcp);
extern void map_info(unsigned x, unsigned y, grid_data *g);
extern void move_cursor_relative(int y, int x);
extern void big_putch(int x, int y, byte a, char c);
extern void print_rel(wchar_t c, byte a, int y, int x);
extern void note_spot(int y, int x);
extern void light_spot(int y, int x);
extern void prt_map(void);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);
extern void forget_view(void);
extern void update_view(void);
extern void update_noise(void);
extern void update_smell(void);
extern void map_area(int y, int x, bool extended);
extern void wiz_light(bool wizard);
extern void wiz_dark(void);
extern void illuminate(void);
extern void cave_set_feat(int y, int x, int feat);
extern int project_path(u16b *gp, int range, \
                         int y1, int x1, int y2, int x2, int flg);
extern byte projectable(int y1, int x1, int y2, int x2, int flg);
extern void scatter(int *yp, int *xp, int y, int x, int d, int m);
extern void health_track(int m_idx);
extern void monster_race_track(int r_idx);
extern void track_object(int item);
extern void track_object_kind(int k_idx);
extern void disturb(int stop_search, int unused_flag);
extern bool dtrap_edge(int y, int x);

#define CAVE_INFO_Y	DUNGEON_HGT
#define CAVE_INFO_X	256


#endif /* !CAVE_H */
