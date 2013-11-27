#ifndef INCLUDED_MAPMODE_H
#define INCLUDED_MAPMODE_H

/**
 * Maximum number of paths from a wilderness stage to adjoining stages.
 */
#define MAX_PATHS       13


/**
 * Number of recall points the player can set
 */
#define MAX_RECALL_PTS   4

/**
 * Number of stages.  The relationship between individual stages is defined
 * in the stage_map array in tables.c.     -NRM-
 */
#define NUM_STAGES      412

/** Number of localities */

#define MAX_LOCALITIES 37


/**
 * Total number of towns 
 */
#define NUM_TOWNS       10

/**
 * Number of small towns 
 */
#define NUM_TOWNS_SMALL  6

/**
 * Number of large towns 
 */
#define NUM_TOWNS_BIG    4

/**
 * Game maps
 */
#define MAP_COMPRESSED     0
#define MAP_EXTENDED       1
#define MAP_DUNGEON        2
#define MAP_FANILLA        3
#define MAP_MAX            4

#define MAP(gmap)	(p_ptr->map == MAP_##gmap)

/**
 * Game modes
 */
enum
{
    GAME_MODE_THRALL = 0,
    GAME_MODE_IRONMAN,
    GAME_MODE_NO_STAIRS,
    GAME_MODE_SMALL_DEVICE,
    GAME_MODE_NO_ARTIFACTS,
    GAME_MODE_NO_SELLING,
    GAME_MODE_AI_CHEAT,

    GAME_MODE_MAX
};

#define MODE(gmode)	p_ptr->game_mode[GAME_MODE_##gmode]

/* Localities -NRM- */

#define NOWHERE                0
#define HITHAEGLIR             1
#define ERIADOR                2
#define ERED_LUIN              3
#define ERED_LUIN_SOUTH        4
#define OSSIRIAND              5
#define TAUR_IM_DUINATH        6
#define EAST_BELERIAND         7
#define ANDRAM                 8
#define WEST_BELERIAND         9
#define THARGELION             10
#define DORIATH                11
#define HIMLAD                 12
#define DOR_DINEN              13
#define DORTHONION             14
#define TALATH_DIRNEN          15
#define BRETHIL                16
#define SIRION_VALE            17
#define FEN_OF_SERECH          18
#define ANFAUGLITH             19
#define LOTHLANN               20
#define AMON_RUDH              21
#define NAN_DUNGORTHEB         22
#define NARGOTHROND            23
#define TOL_IN_GAURHOTH        24
#define ANGBAND                25
#define ANDUIN_VALE            26
#define GLADDEN_FIELDS         27
#define KHAZAD_DUM             28
#define BELEGOST               29
#define MENEGROTH              30
#define EPHEL_BRANDIR          31
#define GONDOLIN               32
#define ENT_PATH               33
#define ERIADOR_SOUTH          34
#define UNDERWORLD             35
#define MOUNTAIN_TOP           36

/** Number of stage types */

#define NUM_STAGE_TYPES 10

/* types of stage -NRM- */

#define TOWN                   0
#define PLAIN                  1
#define FOREST                 2
#define MOUNTAIN               3  
#define SWAMP                  4
#define RIVER                  5
#define DESERT                 6
#define CAVE                   7
#define VALLEY                 8
#define MOUNTAINTOP            9

/* Fields for stage_map array */
#define LOCALITY               0
#define DEPTH                  1
#define NORTH                  2
#define EAST                   3
#define SOUTH                  4
#define WEST                   5
#define UP                     6
#define DOWN                   7
#define STAGE_TYPE             8

/* 
 * Special stage numbers 
 * 
 * These need to be changed any time the maps change
 */
#define ERIADOR_TOWN           (MAP(COMPRESSED) ? 5 : 6)
#define OSSIRIAND_TOWN         (MAP(COMPRESSED) ? 20 : 30)
#define ERED_LUIN_SOUTH_TOWN   (MAP(COMPRESSED) ? 24 : 37)
#define TAUR_IM_DUINATH_TOWN   (MAP(COMPRESSED) ? 27 : 44)
#define EPHEL_BRANDIR_TOWN     (MAP(COMPRESSED) ? 59 : 115)
#define GLADDEN_FIELDS_TOWN    (MAP(COMPRESSED) ? 79 : 150)
#define KHAZAD_DUM_TOWN        (MAP(COMPRESSED) ? 80 : 151)
#define BELEGOST_TOWN          (MAP(COMPRESSED) ? 81 : 152)
#define MENEGROTH_TOWN         (MAP(COMPRESSED) ? 82 : 153)
#define GONDOLIN_TOWN          (MAP(COMPRESSED) ? 83 : 154)
#define UNDERWORLD_STAGE       (MAP(COMPRESSED) ? 84 : 255)
#define MOUNTAINTOP_STAGE      (MAP(COMPRESSED) ? 85 : 256)
#define THRALL_START \
	(MAP(DUNGEON) ? 87 : (MAP(COMPRESSED) ? 70 : 135))


/**
 * Is the player outside?
 */
#define outside \
    ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)	\
     && (stage_map[p_ptr->stage][STAGE_TYPE] != VALLEY) \
       && ((p_ptr->stage < KHAZAD_DUM_TOWN) || (p_ptr->stage > MENEGROTH_TOWN)))

			 
/**
 * Is the player in daylight?
 */
#define is_daylight  (((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)) \
			&& outside) 


/**
 * An array of NUM_STAGES u16b's
 */
typedef u16b u16b_stage[NUM_STAGES];


extern int stage_map[NUM_STAGES][9];
extern int compressed_map[NUM_STAGES][9];
extern int dungeon_map[NUM_STAGES][9];
extern int extended_map[NUM_STAGES][9];
extern int fanilla_map[NUM_STAGES][9];
extern const char *locality_name[MAX_LOCALITIES];
extern const char *short_locality_name[MAX_LOCALITIES];
extern int towns[10];
extern int extended_towns[10];
extern int compressed_towns[10];
extern int race_town_prob[10][14];
const byte g_info[196];

#endif /* INCLUDED_MAPMODE_H */
