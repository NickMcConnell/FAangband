/** \file generate.c 
    \brief Dungeon generation
 
 * Code for making, stocking, and populating levels when generated.  
 * Includes rooms of every kind, pits, vaults (inc. interpretation of 
 * v_info.txt), streamers, tunnelling, etc.  Level feelings and other 
 * messages, autoscummer behavior.  Creation of the town.  Creation of
 * wilderness.
 *
 * Copyright (c) 2005 
 * Nick McConnell, Leon Marrick, Ben Harrison, James E. Wilson, 
 * Robert A. Koeneke
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



/*
 * Level generation is not an important bottleneck, though it can be 
 * annoyingly slow on older machines...  Thus we emphasize simplicity 
 * and correctness over speed.  See individual functions for notes.
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * The "v_info.txt" file is used to store vault generation info.
 */


/*
 * Dungeon generation values
 */
/** Number of rooms to attempt */
#define DUN_ROOMS	     30	  
/** 1/chance of being a destroyed level */
#define DEST_LEVEL_CHANCE    25
/** 1/chance of being a moria-style level */
#define MORIA_LEVEL_CHANCE   40	  

/** 
 * 1/chance of being a themed level - higher in wilderness -NRM-
 */
#define THEMED_LEVEL_CHANCE  (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE \
			      ? 180 : 70)


/*
 * Dungeon tunnel generation values
 */
/** 1 in # chance of random direction */
#define DUN_TUN_RND  	30	
/**1 in # chance of adjusting direction */
#define DUN_TUN_ADJ  	10
/** Chance of doors at room entrances */
#define DUN_TUN_PEN	35	
/** Chance of doors at tunnel junctions */
#define DUN_TUN_JCT	70      

/*
 * Dungeon streamer generation values
 */
/** Width of streamers (can sometimes be higher) */
#define DUN_STR_WID	2    
/** Number of magma streamers */
#define DUN_STR_MAG	3    
/** 1/chance of treasure per magma */
#define DUN_STR_MC	70   
/** Number of quartz streamers */
#define DUN_STR_QUA	2    
/** 1/chance of treasure per quartz */
#define DUN_STR_QC	35   
/** 1/(4 + chance) of altering direction */
#define DUN_STR_CHG	16   

/*
 * Dungeon treasure allocation values
 */
/** Amount of objects for rooms */
#define DUN_AMT_ROOM	9	
/** Amount of objects for rooms/corridors */
#define DUN_AMT_ITEM	2	
/** Amount of treasure for rooms/corridors */
#define DUN_AMT_GOLD	2	

/*
 * Hack -- Dungeon allocation "places"
 */
/** Hallway */
#define ALLOC_SET_CORR		1	
/** Room */
#define ALLOC_SET_ROOM		2	
/** Anywhere */
#define ALLOC_SET_BOTH		3	

/*
 * Hack -- Dungeon allocation "types"
 */
/** Rubble */
#define ALLOC_TYP_RUBBLE	1	
/** Trap */
#define ALLOC_TYP_TRAP		3	
/** Gold */
#define ALLOC_TYP_GOLD		4	
/** Object */
#define ALLOC_TYP_OBJECT	5	


/**
 * Maximum numbers of rooms along each axis (currently 6x18)
 */
#define MAX_ROOMS_ROW	(DUNGEON_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL	(DUNGEON_WID / BLOCK_WID)

/*
 * Maximal number of room types
 */
#define ROOM_MAX	11

/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	DUN_ROOMS
#define DOOR_MAX	100
#define WALL_MAX	40
#define TUNN_MAX	300
#define STAIR_MAX	30

/**
 * Tree type chances 
 */
#define HIGHLAND_TREE_CHANCE 30

/**
 * Simple structure to hold a map location
 */

typedef struct coord coord;

struct coord
{
  byte y;
  byte x;
};

/**
 * Structure to hold all dungeon generation data
 */

typedef struct dun_data dun_data;

struct dun_data
{
  /* Array of centers of rooms */
  int cent_n;
  coord cent[CENT_MAX];
  
  /* Array to store whether rooms are connected or not. */
  bool connected[CENT_MAX];
  
  /* Array of possible door locations */
  int door_n;
  coord door[DOOR_MAX];
  
  /* Array of wall piercing locations */
  int wall_n;
  coord wall[WALL_MAX];
  
  /* Array of tunnel grids */
  int tunn_n;
  coord tunn[TUNN_MAX];
  
  /* Array of good potential stair grids */
  int stair_n;
  coord stair[STAIR_MAX];
  
  /* Number of blocks along each axis */
  int row_rooms;
  int col_rooms;
  
  /* Array to store block usage */
  int room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];
};

/**
 * Dungeon generation data -- see  cave_gen()
 */
static dun_data *dun;


/**
 * Is the level moria-style?
 */
static bool moria_level;

/**
 * Is the level underworld?
 */
static bool underworld;


/**
 * Room type information
 */
typedef struct room_data room_data;

struct room_data
{
  /* Allocation information. */
  s16b room_gen_num[11];
  
  /* Minimum level on which room can appear. */
  byte min_level;
};

/**
 * Table of values that control how many times each type of room will, 
 * on average, appear on 100 levels at various depths.  Each type of room 
 * has its own row, and each column corresponds to dungeon levels 0, 10, 
 * 20, and so on.  The final value is the minimum depth the room can appear 
 * at.  -LM-
 *
 * Level 101 and below use the values for level 100.
 *
 * Rooms with lots of monsters or loot may not be generated if the object or 
 * monster lists are already nearly full.  Rooms will not appear above their 
 * minimum depth.  No type of room (other than type 1) can appear more than 
 * DUN_ROOMS/2 times in any level.
 *
 * The entries for room type 1 are blank because these rooms are built once 
 * all other rooms are finished -- until the level fills up, or the room 
 * count reaches the limit (DUN_ROOMS).
 */
static room_data room[ROOM_MAX] = 
{
  /* Depth:        0   10   20   30   40   50   60   70   80   90  100   min */

  /* Nothing */{{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },  0 },
  /* Simple */ {{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },  0 },
  /*Overlap */ {{ 60,  80, 100, 120, 140, 165, 180, 200, 220, 240, 260 },  1 },
  /* Cross  */ {{  0,  25,  50,  70,  85, 100, 110, 120, 130, 140, 150 },  3 },
  /* Large  */ {{  0,  25,  50,  70,  85, 100, 110, 120, 130, 140, 150 },  3 },
  /* Pit    */ {{  0,   5,  12,  25,  30,  35,  38,  40,  40,  40,  40 },  5 },
  /*Chambers*/ {{  0,   2,   6,  12,  15,  18,  19,  20,  20,  20,  20 },  5 },
  /* I.Room */ {{ 50,  60,  70,  80,  80,  80,  80,  80,  80,  80,  80 },  0 },
  /* L.Vault*/ {{  0,   1,   4,   9,  16,  27,  40,  55,  70,  80,  90 },  5 },
  /* G.Vault*/ {{  0,   0,   1,   2,   3,   4,   6,   7,   8,  10,  12 }, 20 },
  /* Huge   */ {{  0,   0,   0,   0,   4,   4,   4,   4,   4,   4,   4 }, 41 }
};



/**
 * This table takes a depth, and returns a suitable monster symbol.  Depth 
 * input is assumed to be player depth.  It is also assumed that monsters 
 * can be generated slightly out of depth.  -LM-
 *
 * Depths greater than 60 should map to the row for level 60.
 *
 * Monster pits use the first seven columns, and rooms of chambers use all  
 * thirteen.  Because rooms of chambers are about half as common as monster 
 * pits, and have fewer monsters, creatures in the first seven columns will 
 * be much more common than those in the last six.  On the other hand, 
 * monsters in rooms of chambers have a lot more maneuver room...
 *
 * - Symbol '*' is any creature of a randomly-chosen racial char.
 * - Symbol 'A' is any animal.
 *
 * - Symbol '1' is any insect ('a', 'c', 'l', 'F', 'I', 'K').
 * - Symbol '2' is any naga, snake, or other reptile.
 * - Symbol '3' is any jelly, mold, icky thing ('i', 'j', or 'm').
 *
 * - Symbol '%' is any orc, ogre, troll, or giant.
 * - Symbol 'N' is any undead.  Upon occasion, deep in the dungeon, the 
 *   racial type may be forced to 'G', 'L', 'V', or 'W'.
 * - Symbols 'p' and 'h' may sometimes be found in combination.  If they 
 *   are, they are usually all of a given class (magical, pious, natural, 
 *   assassination/thievery, or warrior)
 * - Symbols 'E' and 'v' may be found in combination.  If they are, they 
 *   will always be of a given elemental type.
 * - Symbols 'd' and 'D' both mean dragons of either char.  Dragons are 
 *   often of a particular type (blue, red, gold, shadow/ethereal etc.).
 * - Symbols 'u' and 'U' may mean lesser demons, greater demons, or both, 
 *   depending on depth. 
 * 
 * - Other symbols usually represent specific racial characters.
 */
static char mon_symbol_at_depth[12][13] = 
{
  /* Levels 5, 10, 15, and 20 */
  {'A', 'A', '3', '3', '3', 'k', 'y',   '*', '*', '1', 'a', '2', 'S' },
  {'A', '3', '3', 'o', 'o', 'N', '1',   '*', 'C', 'f', 'a', '2', 'S' },
  {'A', '3', 'o', 'o', 'o', 'u', '*',   '#', '#', 'S', 'E', '2', 'Z' },
  {'A', '3', '3', 'o', 'T', 'T', '#',   'p', 'h', 'f', '1', '*', 'Z' },
  
  /* Levels 25, 30, 35, and 40 */
  {'A', '3', 'T', 'T', 'u', 'P', 'P',   'p', 'v', 'd', '1', 'S', '2' },
  {'A', 'A', 'T', 'P', 'P', 'N', 'd',   'p', 'h', 'f', 'v', 'g', 'Z' },
  {'A', '3', 'T', 'P', 'N', 'u', 'd',   'p', 'H', 'E', '1', '*', '2' },
  {'A', 'A', 'T', 'P', 'N', 'u', 'd',   'p', 'h', 'g', 'E', '*', 'Z' },
  
  /* Levels 45, 50, 55, and 60 */
  {'A', 'P', 'N', 'u', 'd', 'd', '*',   'p', 'h', 'v', 'E', '*', 'Z' },
  {'N', 'N', 'U', 'U', 'D', 'D', '*',   'p', 'h', 'v', 'T', 'B', 'Z' },
  {'A', 'N', 'U', 'U', 'D', 'D', '*',   'p', 'h', 'W', 'G', '*', 'Z' },
  {'N', 'N', 'U', 'U', 'D', 'D', '*',   'p', 'h', 'v', '*', 'D', 'Z' } 
};

/**
 * Restrictions on monsters, used in pits, vaults, and chambers.
 */
static bool allow_unique;
static char d_char_req[15];
static byte d_attr_req[4];
static u32b racial_flag_mask;
static u32b breath_flag_mask;



/**
 * Table of monster descriptions.  Used to make descriptions for kinds 
 * of pits and rooms of chambers that have no special names.
 */
cptr d_char_req_desc[] =
{
  "B:bird",
  "C:canine",
  "D:dragon",
  "E:elemental",
  "F:dragon fly",
  "G:ghost",
  "H:hybrid",
  "I:insect",
  "J:snake",
  "K:killer beetle",
  "L:lich",
  "M:mummy",
  "O:ogre",
  "P:giant",
  "Q:quylthulg",
  "R:reptile",
  "S:spider",
  "T:troll",
  "U:demon",
  "V:vampire",
  "W:wraith",
  "Y:yeti",
  "Z:zephyr hound",
  "a:ant",
  "b:bat",
  "c:centipede",
  "d:dragon",
  "e:floating eye",
  "f:feline",
  "g:golem",
  "h:humanoid",
  "i:icky thing",
  "j:jelly",
  "k:kobold",
  "l:louse",
  "m:mold",
  "n:naga",
  "o:orc",
  "p:human",
  "q:quadruped",
  "r:rodent",
  "s:skeleton",
  "t:townsperson",
  "u:demon",
  "v:vortex",
  "w:worm",
  "y:yeek",
  "z:zombie",
  ",:mushroom patch",
  NULL
};

/** 
 * Number and type of "vaults" in wilderness levels 
 * These need to be set at the start of each wilderness generation routine.
 */
int wild_vaults = 0;
int wild_type = 0;


/**
 * Specific levels on which there should never be a vault
 */
bool no_vault(void)
{
  /* No vaults on mountaintops */
  if (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAINTOP) return (TRUE);

  /* No vaults on dungeon entrances */
  if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE) &&
      (stage_map[p_ptr->stage][DOWN]))
    return (TRUE);

  /* Anywhere else is OK */
  return (FALSE);
}


/**************************************************************/
/*                                                            */
/*                 The monster-selection code                 */
/*                                                            */
/**************************************************************/


/**
 * Hack - select beings of the four basic elements.  Used in the elemental 
 * war themed level.
 */
static bool vault_aux_elemental(int r_idx)
{
  u32b breaths_allowed;
  monster_race *r_ptr = &r_info[r_idx];
  
  
  /* Demons are always welcome. */
  if (r_ptr->d_char == 'U' || r_ptr->d_char == 'u') return (TRUE);
  
  /* Certain names are a givaway. */
  if (strstr(r_name + r_ptr->name, "Fire") || 
      strstr(r_name + r_ptr->name, "Hell") || 
      strstr(r_name + r_ptr->name, "Frost") || 
      strstr(r_name + r_ptr->name, "Cold") || 
      strstr(r_name + r_ptr->name, "Acid") || 
      strstr(r_name + r_ptr->name, "Water") || 
      strstr(r_name + r_ptr->name, "Energy")) return (TRUE);
  
  /* Otherwise, try selecting by breath attacks. */
  breaths_allowed = (RF4_BRTH_ACID | RF4_BRTH_ELEC |
		     RF4_BRTH_FIRE | RF4_BRTH_COLD);
  
  /* Must have at least one of the required breath attacks */
  if (!(r_ptr->flags4 & breaths_allowed)) return (FALSE);
  
  /* Okay */
  return (TRUE);
}


/**
 * Use various selection criteria (set elsewhere) to restrict monster 
 * generation.
 *
 * This function is capable of selecting monsters by:
 *   - racial symbol (may be any of the characters allowed)
 *   - symbol color (may be any of up to four colors).
 *   - racial flag(s) (monster may have any allowed flag)
 *   - breath flag(s) (monster must have exactly the flags specified)
 *
 * Uniques may be forbidden, or allowed on rare occasions.
 *
 * Some situations (like the elemental war themed level) require special 
 * processing; this is done in helper functions called from this one.
 */
static bool mon_select(int r_idx)
{
  monster_race *r_ptr = &r_info[r_idx];
  bool ok = FALSE;
  
  
  /* Special case:  Elemental war themed level. */
  if (p_ptr->themed_level == THEME_ELEMENTAL)
    {
      return (vault_aux_elemental(r_idx));
    }
  
  /* Special case:  Estolad themed level. */
  if (p_ptr->themed_level == THEME_ESTOLAD)
    {
      if (!(r_ptr->flags3 & RF3_RACIAL)) return (FALSE);
    }
  
  
  /* Require that the monster symbol be correct. */
  if (d_char_req[0] != '\0')
    {
      if (strchr(d_char_req, r_ptr->d_char) == 0) return (FALSE);
    }
  
  /* Require correct racial type. */
  if (racial_flag_mask)
    {
      if (!(r_ptr->flags3 & (racial_flag_mask))) return (FALSE);
      
      /* Hack -- no invisible undead until deep. */
      if ((p_ptr->depth < 40) && (r_ptr->flags3 & (RF3_UNDEAD)) && 
	  (r_ptr->flags2 & (RF2_INVISIBLE))) return (FALSE);
    }
  
  /* Require that monster breaths be exactly those specified. */
  /* Exception for winged dragons */
  if (breath_flag_mask)
    {
      /* 'd'ragons */
      if ((r_ptr->flags4 != breath_flag_mask) && (r_ptr->d_char != 'D')) 
	return (FALSE);

      /* Winged 'D'ragons */
      if (r_ptr->d_char == 'D')
	{
	  if ((r_ptr->flags4 & breath_flag_mask) != breath_flag_mask) 
	    return (FALSE);

	  /* Hack - this deals with all current Ds that need excluding */
	  if (r_ptr->flags4 & RF4_BRTH_SOUND)
	    return (FALSE);
	}
    }
  
  /* Require that the monster color be correct. */
  if (d_attr_req[0])
    {
      /* Check all allowed colors, if given. */
      if ((d_attr_req[0]) && (r_ptr->d_attr == d_attr_req[0])) ok = TRUE;
      if ((d_attr_req[1]) && (r_ptr->d_attr == d_attr_req[1])) ok = TRUE;
      if ((d_attr_req[2]) && (r_ptr->d_attr == d_attr_req[2])) ok = TRUE;
      if ((d_attr_req[3]) && (r_ptr->d_attr == d_attr_req[3])) ok = TRUE;
      
      /* Hack -- No multihued dragons allowed in the arcane dragon pit. */
      if ((strchr(d_char_req, 'd') || strchr(d_char_req, 'D')) && 
	  (d_attr_req[0] == TERM_VIOLET) && 
	  (r_ptr->flags4 == (RF4_BRTH_ACID | 
			     RF4_BRTH_ELEC | RF4_BRTH_FIRE | 
			     RF4_BRTH_COLD | RF4_BRTH_POIS)))
	{
	  return (FALSE);
	}
      
      /* Doesn't match any of the given colors?  Not good. */
      if (!ok) return (FALSE);
    }
  
  /* Usually decline unique monsters. */
  if (r_ptr->flags1 & (RF1_UNIQUE))
    {
      if (!allow_unique) return (FALSE);
      else if (rand_int(5) != 0) return (FALSE);
    }
  
  /* Okay */
  return (TRUE);
}

/**
 * Accept characters representing a race or group of monsters and 
 * an (adjusted) depth, and use these to set values for required racial 
 * type, monster symbol, monster symbol color, and breath type.  -LM-
 *
 * This function is called to set restrictions, point the monster 
 * allocation function to mon_select(), and remake monster allocation.  
 * It undoes all of this things when called with the symbol '\0'.
 * 
 * Describe the monsters (used by cheat_room) and determine if they 
 * should be neatly ordered or randomly placed (used in monster pits).
 */
static char *mon_restrict(char symbol, byte depth, bool *ordered, 
			  bool unique_ok)
{
  int i, j;
  
  /* Assume no definite name */
  char name[80] = "misc";
  
  /* Clear global monster restriction variables. */
  allow_unique = unique_ok;
  for (i = 0; i < 10; i++) d_char_req[i] = '\0';
  for (i = 0; i < 4; i++) d_attr_req[i] = 0;
  racial_flag_mask = 0; breath_flag_mask = 0;
  
  
  /* No symbol, no restrictions. */
  if (symbol == '\0')
    {
      get_mon_num_hook = NULL;
      get_mon_num_prep();
      return ("misc");
    }
  
  /* Handle the "wild card" symbol '*'  */
  if (symbol == '*')
    {
      for (i = 0; i < 2500; i++)
	{
	  /* Get a random monster. */
	  j = randint(z_info->r_max - 1);
	  
	  /* Must be a real monster */
	  if (!r_info[j].rarity) continue;
	  
	  /* Try for close to depth, accept in-depth if necessary */
	  if (i < 200)
	    {
	      if ((!(r_info[j].flags1 & RF1_UNIQUE)) && 
		  (r_info[j].level != 0) && 
		  (r_info[j].level <= depth) && 
		  (ABS(r_info[j].level - p_ptr->depth) < 
		   1 + (p_ptr->depth / 4))) break;
	    }
	  else
	    {
	      if ((!(r_info[j].flags1 & RF1_UNIQUE)) && 
		  (r_info[j].level != 0) && 
		  (r_info[j].level <= depth)) break;
	    }
	}
      
      /* We've found a monster. */
      if (i < 2499)
	{
	  /* ...use that monster's symbol for all monsters. */
	  symbol = r_info[j].d_char;
	}
      else
	{
	  /* Paranoia - pit stays empty if no monster is found */
	  return (NULL);
	}
    }
  
  /* Apply monster restrictions according to symbol. */
  switch (symbol)
    {
      /* All animals */
    case 'A':
      {
	strcpy(name, "animal");
	racial_flag_mask = RF3_ANIMAL;
	*ordered = FALSE;
	break;
      }
      
      /* Insects */
    case '1':
      {
	strcpy(name, "insect");
	strcpy(d_char_req, "aclFIK");
	*ordered = FALSE;
	break;
      }
      
      /* Reptiles */
    case '2':
      {
	strcpy(name, "reptile");
	strcpy(d_char_req, "nJR");
	*ordered = FALSE;
	break;
      }
      
      /* Jellies, etc. */
    case '3':
      {
	strcpy(name, "jelly");
	strcpy(d_char_req, "ijm,");
	*ordered = FALSE;
	break;
      }
      
      /* Humans and humaniods */
    case 'p':
    case 'h':
      {
	/* 'p's and 'h's can coexist. */
	if (rand_int(3) == 0) 
	  {
	    strcpy(d_char_req, "ph");
	    
	    /* If so, they will usually all be of similar classes. */
	    if (rand_int(4) != 0)
	      {
		/* Randomizer. */
		i = rand_int(5);
		
		/* Magicians and necromancers */
		if (i == 0)
		  {
		    d_attr_req[0] = TERM_RED;
		    d_attr_req[1] = TERM_L_RED;
		    d_attr_req[2] = TERM_VIOLET;
		    strcpy(name, "school of sorcery");
		  }
		/* Priests and paladins */
		else if (i == 1)
		  {
		    d_attr_req[0] = TERM_GREEN;
		    d_attr_req[1] = TERM_L_GREEN;
		    d_attr_req[2] = TERM_WHITE;
		    d_attr_req[3] = TERM_L_WHITE;
		    strcpy(name, "temple of piety");
		  }
		/* Druids and ninjas */
		else if (i == 2)
		  {
		    d_attr_req[0] = TERM_ORANGE;
		    d_attr_req[1] = TERM_YELLOW;
		    strcpy(name, "gathering of nature");
		  }
		/* Thieves and assassins */
		else if (i == 3)
		  {
		    d_attr_req[0] = TERM_BLUE;
		    d_attr_req[1] = TERM_L_BLUE;
		    d_attr_req[2] = TERM_SLATE;
		    d_attr_req[3] = TERM_L_DARK;
		    strcpy(name, "den of thieves");
		  }
		/* Warriors and rangers */
		else 
		  {
		    d_attr_req[0] = TERM_UMBER;
		    d_attr_req[1] = TERM_L_UMBER;
		    strcpy(name, "fighter's hall");
		  }
	      }
	    else
	      {
		strcpy(name, "humans and humanoids");
	      }
	  }
	
	/* Usually, just accept the symbol. */
	else 
	  {
	    d_char_req[0] = symbol;
	    
	    if (symbol == 'p') strcpy(name, "human");
	    else if (symbol == 'h') strcpy(name, "humanoid");
	  }
	
	*ordered = FALSE;
	break;
      }
      
      /* Orcs */
    case 'o':
      {
	strcpy(name, "orc");
	strcpy(d_char_req, "o");
	*ordered = TRUE;
	break;
      }
      
      /* Trolls */
    case 'T':
      {
	strcpy(name, "troll");
	strcpy(d_char_req, "T");
	*ordered = TRUE;
	break;
      }
      
      /* Giants (sometimes ogres at low levels) */
    case 'P':
      {
	strcpy(name, "giant");
	if ((p_ptr->depth < 30) && (rand_int(3) == 0)) 
	  strcpy(d_char_req, "O");
	else strcpy(d_char_req, "P");
	*ordered = TRUE;
	break;
      }
      
      /* Orcs, ogres, trolls, or giants */
    case '%':
      {
	strcpy(name, "moria");
	strcpy(d_char_req, "oOPT");
	*ordered = FALSE;
	break;
      }
      
      /* Monsters found in caves */
    case '0':
      {
	strcpy(name, "dungeon monsters");
	strcpy(d_char_req, "ykoOT");
	*ordered = FALSE;
	break;
      }
      
      /* Monsters found in wilderness caves */
    case 'x':
      {
	strcpy(name, "underworld monsters");
	strcpy(d_char_req, "bgkosuyOTUVXW");
	*ordered = FALSE;
	break;
      }
      
      
      /* Undead */
    case 'N':
      {
	/* Sometimes, restrict by symbol. */
	if ((depth > 40) && (rand_int(3) == 0))
	  {
	    for (i = 0; i < 500; i++)
	      {
		/* Find a suitable monster near depth. */
		j = randint(z_info->r_max - 1);
		
		/* Require a non-unique undead. */
		if ((r_info[j].flags3 & RF3_UNDEAD) && 
		    (!(r_info[j].flags1 & RF1_UNIQUE)) && 
		    (strchr("GLWV", r_info[j].d_char)) && 
		    (ABS(r_info[j].level - p_ptr->depth) < 
		     1 + (p_ptr->depth / 4)))
		  {
		    break;
		  }
	      }
	    
	    /* If we find a monster, */
	    if (i < 499)
	      {
		/* Use that monster's symbol for all monsters */
		d_char_req[0] = r_info[j].d_char;
		
		/* No pit name (yet) */
		
		/* In this case, we do order the monsters */
		*ordered = TRUE;
	      }
	    else
	      {
		/* Accept any undead. */
		strcpy(name, "undead");
		racial_flag_mask = RF3_UNDEAD;
		*ordered = FALSE;
	      }
	  }
	else
	  {
	    /* No restrictions on symbol. */
	    strcpy(name, "undead");
	    racial_flag_mask = RF3_UNDEAD;
	    *ordered = FALSE;
	  }
	break;
      }
      
      /* Demons */
    case 'u':
    case 'U':
      {
	strcpy(name, "demon");
	if (depth > 55)      strcpy(d_char_req, "U");
	else if (depth < 40) strcpy(d_char_req, "u");
	else                 strcpy(d_char_req, "uU");
	*ordered = TRUE;
	break;
      }
      
      /* Dragons */
    case 'd':
    case 'D':
      {
	strcpy(d_char_req, "dD");
	
	/* Dragons usually associate with others of their kind. */
	if (rand_int(6) != 0)
	  {
	    /* Dragons of a single kind are ordered. */
	    *ordered = TRUE;
	    
	    /* Some dragon types are not found everywhere */
	    if (depth > 70) i = rand_int(35);
	    else if (depth > 45) i = rand_int(32);
	    else if (depth > 32) i = rand_int(30);
	    else if (depth > 23) i = rand_int(28);
	    else i = rand_int(24);
	    
	    if (i < 4)
	      {
		breath_flag_mask = (RF4_BRTH_ACID);
		strcpy(name, "dragon - acid");
	      }
	    else if (i < 8)
	      {
		breath_flag_mask = (RF4_BRTH_ELEC);
		strcpy(name, "dragon - electricity");
	      }
	    else if (i < 12)
	      {
		breath_flag_mask = (RF4_BRTH_FIRE);
		strcpy(name, "dragon - fire");
	      }
	    else if (i < 16)
	      {
		breath_flag_mask = (RF4_BRTH_COLD);
		strcpy(name, "dragon - cold");
	      }
	    else if (i < 20)
	      {
		breath_flag_mask = (RF4_BRTH_POIS);
		strcpy(name, "dragon - poison");
	      }
	    else if (i < 24)
	      {
		breath_flag_mask = (RF4_BRTH_ACID | 
				    RF4_BRTH_ELEC | RF4_BRTH_FIRE | 
				    RF4_BRTH_COLD | RF4_BRTH_POIS);
		strcpy(name, "dragon - multihued");
	      }
	    else if (i < 26)
	      {
		breath_flag_mask = (RF4_BRTH_CONFU);
		strcpy(name, "dragon - confusion");
	      }
	    else if (i < 28)
	      {
		breath_flag_mask = (RF4_BRTH_SOUND);
		strcpy(name, "dragon - sound");
	      }
	    else if (i < 30)
	      {
		breath_flag_mask = (RF4_BRTH_LITE | 
				    RF4_BRTH_DARK);
		strcpy(name, "dragon - ethereal");
	      }
	    
	    /* Chaos, Law, Balance, Power, etc.) */
	    else
	      {
		d_attr_req[0] = TERM_VIOLET;
		d_attr_req[1] = TERM_L_BLUE;
		d_attr_req[2] = TERM_L_GREEN;
		strcpy(name, "dragon - arcane");
	      }
	  }
	else
	  {
	    strcpy(name, "dragon - mixed");
	    
	    /* Dragons of all kinds are not ordered. */
	    *ordered = FALSE;
	  }
	break;
      }
      
      /* Vortexes and elementals */
    case 'v':
    case 'E':
      {
	/* Usually, just have any kind of 'v' or 'E' */
	if (rand_int(3) != 0)
	  {
	    d_char_req[0] = symbol;
	    
	    if (symbol == 'v') strcpy(name, "vortex");
	    if (symbol == 'E') strcpy(name, "elemental");
	  }
	
	/* Sometimes, choose both 'v' and 'E's of one element */
	else
	  {
	    strcpy(d_char_req, "vE");
	    
	    i = rand_int(4);
	    
	    /* Fire */
	    if (i == 0)
	      {
		d_attr_req[0] = TERM_RED;
		strcpy(name, "fire");
	      }
	    /* Frost */
	    if (i == 1)
	      {
		d_attr_req[0] = TERM_L_WHITE;
		d_attr_req[1] = TERM_WHITE;
		strcpy(name, "frost");
	      }
	    /* Air/electricity */
	    if (i == 2)
	      {
		d_attr_req[0] = TERM_L_BLUE;
		d_attr_req[1] = TERM_BLUE;
		strcpy(name, "air");
	      }
	    /* Acid/water/earth */
	    if (i == 3)
	      {
		d_attr_req[0] = TERM_GREEN;
		d_attr_req[1] = TERM_L_UMBER;
		d_attr_req[2] = TERM_UMBER;
		d_attr_req[3] = TERM_SLATE;
		strcpy(name, "earth & water");
	      }
	  }
	
	*ordered = FALSE;
	break;
      }
      
      /* Special case:  mimics and treasure */
    case '!':
    case '?':
    case '=':
    case '~':
    case '|':
    case '.':
    case '$':
      {
	if (symbol == '$')
	  {
	    strcpy(name, "treasure");
	    
	    /* Nothing but loot! */
	    if (rand_int(3) == 0) strcpy(d_char_req, "$");
	    
	    /* Guard the money well. */
	    else strcpy(d_char_req, "$!?=~|.");
	  }
	else
	  {
	    /* No treasure. */
	    strcpy(d_char_req, "!?=~|.");
	    strcpy(name, "mimic");
	  }
	
	*ordered = FALSE;
	break;
      }
      
      /* Special case:  creatures of earth. */
    case 'X':
    case '#':
      {
	strcpy(d_char_req, "X#");
	strcpy(name, "creatures of earth");
	*ordered = FALSE;
	break;
      }
      
      /* Water creatures. */
    case '6':
      {
	allow_unique = TRUE;
	strcpy(d_char_req, "vEZ");
	d_attr_req[0] = TERM_SLATE;
	break;
      }
      
      /* Beings of fire or ice. */
    case '7':
      {
	allow_unique = TRUE;
	strcpy(d_char_req, "vE");
	if (rand_int(2) == 0) d_attr_req[0] = TERM_RED;
	else
	  {
	    d_attr_req[0] = TERM_L_WHITE;
	    d_attr_req[1] = TERM_WHITE;
	  }
	
	break;
      }
      
      /* Space for more monster types here. */
      
      
      /* Any symbol not handled elsewhere. */
    default:
      {
	/* Accept the character. */
	d_char_req[0] = symbol;
	
	/* Some monsters should logically be ordered. */
	if (strchr("knosuyzGLMOPTUVW", symbol)) *ordered = TRUE;
	
	/* Most should not */
	else *ordered = FALSE;
	
	break;
      }
    }
  
  /* If monster pit hasn't been named already, get a name. */
  if (streq(name, "misc"))
    {
      /* Search a table for a description of the symbol */
      for (i = 0; d_char_req_desc[i]; ++i)
	{
	  if (symbol == d_char_req_desc[i][0]) 
	    {
	      /* Get all but the 1st 2 characters of the text. */
	      sprintf(name, "%s", d_char_req_desc[i] + 2);
	      break;
	    }
	}
    }
  
  /* Apply our restrictions */
  get_mon_num_hook = mon_select;
  
  /* Prepare allocation table */
  get_mon_num_prep();
  
  /* Return the name. */
  return (format("%s", name));
}





/**************************************************************/
/*                                                            */
/*            General dungeon-generation functions            */
/*                                                            */
/**************************************************************/


/**
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y, x)"
 */
static int next_to_walls(int y, int x)
{
  int k = 0;
  
  if ((cave_feat[y+1][x] >= FEAT_MAGMA) && 
      (cave_feat[y+1][x] <= FEAT_SHOP_HEAD)) 
    k++;
  if ((cave_feat[y-1][x] >= FEAT_MAGMA) && 
      (cave_feat[y-1][x] <= FEAT_SHOP_HEAD))
    k++;
  if ((cave_feat[y][x+1] >= FEAT_MAGMA) && 
      (cave_feat[y][x+1] <= FEAT_SHOP_HEAD))
    k++;
  if ((cave_feat[y][x-1] >= FEAT_MAGMA) && 
      (cave_feat[y][x-1] <= FEAT_SHOP_HEAD))
    k++;
  
  return (k);
}


/**
 * Returns co-ordinates for the player.  Player prefers to be near 
 * walls, because large open spaces are dangerous.
 */
static void new_player_spot(void)
{
  int i = 0;
  int y, x;
  
  /*
   * Check stored stair locations, then search at random.
   */
  while (TRUE)
    {
      i++;
      
      /* Scan stored locations first. */
      if (i < dun->stair_n)
	{
	  /* Get location */
	  y = dun->stair[i].y;
	  x = dun->stair[i].x;
	  
	  /* Require exactly three adjacent walls */
	  if (next_to_walls(y, x) != 3) continue;
	  
	  /* If character starts on stairs, ... */
	  if (dungeon_stair)
	    {
	      /* Accept stairs going the right way or floors. */
	      if (p_ptr->create_stair)
		{
		  /* Accept correct stairs */
		  if (cave_feat[y][x] == p_ptr->create_stair) break;
		  
		  /* Accept floors, build correct stairs. */
		  if (cave_naked_bold(y, x))
		    {
		      cave_set_feat(y, x, p_ptr->create_stair);
		      break;
		    }
		}
	    }
	  
	  /* If character doesn't start on stairs, ... */
	  else
	    {
	      /* Accept only "naked" floor grids */
	      if (cave_naked_bold(y, x)) break;
	    }
	}
      
      /* Then, search at random */
      else
	{
	  /* Pick a random grid */
	  y = rand_int(DUNGEON_HGT);
	  x = rand_int(DUNGEON_WID);
	  
	  /* Refuse to start on anti-teleport (vault) grids */
	  if (cave_info[y][x] & (CAVE_ICKY)) continue;
	  
	  /* Must be a "naked" floor grid */
	  if (!cave_naked_bold(y, x)) continue;
	  
	  /* Player prefers to be near walls. */
	  if (i < 300 && (next_to_walls(y, x) < 2)) continue;
	  else if (i < 600 && (next_to_walls(y, x) < 1)) continue;
	  
	  /* Success */
	  break;
	}
    }
  
  /* Place the player */
  player_place(y, x);
}


/**
 * Convert existing terrain type to rubble
 */
static void place_rubble(int y, int x)
{
  /* Create rubble */
  cave_set_feat(y, x, FEAT_RUBBLE);
}


/**
 * Convert existing terrain type to "up stairs"
 */
static void place_up_stairs(int y, int x)
{
  /* Create up stairs */
  cave_set_feat(y, x, FEAT_LESS);
}


/**
 * Convert existing terrain type to "down stairs"
 */
static void place_down_stairs(int y, int x)
{
  /* Create down stairs */
  cave_set_feat(y, x, FEAT_MORE);
}


/**
 * Place an up/down staircase at given location
 */
static void place_random_stairs(int y, int x)
{
  /* Paranoia */
  if (!cave_clean_bold(y, x)) return;
  
  /* Choose a staircase */
  if (!p_ptr->depth)
    {
      place_down_stairs(y, x);
    }
  else if (adult_dungeon && !stage_map[p_ptr->stage][DOWN])
    {
      place_up_stairs(y, x);
    }
  else if (rand_int(100) < 50)
    {
      place_down_stairs(y, x);
    }
  else
    {
      place_up_stairs(y, x);
    }
}


/**
 * Places some staircases near walls
 */
static void alloc_stairs(int feat, int num, int walls)
{
  int y, x, i, j;
  bool no_down_shaft = (!stage_map[stage_map[p_ptr->stage][DOWN]][DOWN] ||
			is_quest(stage_map[p_ptr->stage][DOWN]) ||
			is_quest(p_ptr->stage));
  bool no_up_shaft = (!stage_map[stage_map[p_ptr->stage][UP]][UP]);
  bool morgy = is_quest(p_ptr->stage) && stage_map[p_ptr->stage][DEPTH] == 100;
  
  
  /* Place "num" stairs */
  for (i = 0; i < num; i++)
    {
      /* Try hard to place the stair */
      for (j = 0; j < 3000; j++)
	{
	  /* Cut some slack if necessary. */
	  if ((j > dun->stair_n) && (walls > 2)) walls = 2;
	  if ((j > 1000) && (walls > 1)) walls = 1;
	  if (j > 2000) walls = 0;
	  
	  /* Use the stored stair locations first. */
	  if (j < dun->stair_n)
	    {
	      y = dun->stair[j].y;
	      x = dun->stair[j].x;
	    }
	  
	  /* Then, search at random. */
	  else
	    {
	      /* Pick a random grid */
	      y = rand_int(DUNGEON_HGT);
	      x = rand_int(DUNGEON_WID);
	    }
	  
	  /* Require "naked" floor grid */
	  if (!cave_naked_bold(y, x)) continue;
	  
	  /* Require a certain number of adjacent walls */
	  if (next_to_walls(y, x) < walls) continue;
	  
	  /* If we've asked for a shaft and they're forbidden, fail */
	  if (no_down_shaft && (feat == FEAT_MORE_SHAFT))
	    return;
	  if (no_up_shaft && (feat == FEAT_LESS_SHAFT))
	    return;
	  
	  /* Town or no way up -- must go down */
	  if ((!p_ptr->depth) || (!stage_map[p_ptr->stage][UP]))
	    {
	      /* Clear previous contents, add down stairs */
	      if (feat != FEAT_MORE_SHAFT) cave_set_feat(y, x, FEAT_MORE);
	    }
	  
	  /* Bottom of dungeon, Morgoth or underworld -- must go up */
	  else if ((!stage_map[p_ptr->stage][DOWN]) || underworld || morgy)
	    {
	      /* Clear previous contents, add up stairs */
	      if (feat != FEAT_LESS_SHAFT) cave_set_feat(y, x, FEAT_LESS);
	    }
	  
	  /* Requested type */
	  else
	    {
	      /* Clear previous contents, add stairs */
	      cave_set_feat(y, x, feat);
	    }
	  
	  /* Finished with this staircase. */
	  break;
	}
    }
}


/**
 * Allocates some objects (using "place" and "type")
 */
static void alloc_object(int set, int typ, int num)
{
  int y, x, k;
  
  /* Place some objects */
  for (k = 0; k < num; k++)
    {
      /* Pick a "legal" spot */
      while (TRUE)
	{
	  bool room;
	  
	  /* Location */
	  y = rand_int(DUNGEON_HGT);
	  x = rand_int(DUNGEON_WID);
	  
	  /* Paranoia - keep objects out of the outer walls */
	  if (!in_bounds_fully(y, x)) continue;

	  /* Require "naked" floor grid */
	  if (!cave_naked_bold(y, x)) continue;
	  
	  /* Check for "room" */
	  room = (cave_info[y][x] & (CAVE_ROOM)) ? TRUE : FALSE;
	  
	  /* Require corridor? */
	  if ((set == ALLOC_SET_CORR) && room) continue;
	  
	  /* Require room? */
	  if ((set == ALLOC_SET_ROOM) && !room) continue;
	  
	  /* Accept it */
	  break;
	}
      
      /* Place something */
      switch (typ)
	{
	case ALLOC_TYP_RUBBLE:
	  {
	    place_rubble(y, x);
	    break;
	  }
	  
	case ALLOC_TYP_TRAP:
	  {
	    place_trap(y, x);
	    break;
	  }
	  
	case ALLOC_TYP_GOLD:
	  {
	    place_gold(y, x);
	    break;
	  }
	  
	case ALLOC_TYP_OBJECT:
	  {
	    place_object(y, x, FALSE, FALSE, FALSE);
	    break;
	  }
	}
    }
}


/**
 * Value "1" means the grid will be changed, value "0" means it won't.
 *
 * We have 47 entries because 47 is not divisible by any reasonable 
 * figure for streamer width.
 */
static bool streamer_change_grid[47] = 
{
  0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 
  1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0
};


/**
 * Places "streamers" of rock through dungeon.
 *
 * Note that there are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 */
static void build_streamer(int feat, int chance)
{
  int table_start;
  int i;
  int y, x, dy, dx;
  int start_dir, dir;
  int out1, out2;
  bool change;
  
  /* Initialize time until next turn, and time until next treasure */
  int time_to_treas = randint(chance * 2);
  int time_to_turn = randint(DUN_STR_CHG * 2);
  
  
  /* Set standard width.  Vary width sometimes. */
  int width = 2 * DUN_STR_WID + 1;
  if (rand_int(6) == 0) width += randint(3);
  else if (rand_int(6) == 0) width -= randint(3);
  if (width < 1) width = 1;
  
  /* Set expansion outward from centerline. */
  out1 = width / 2;
  out2 = (width + 1) / 2;
  
  
  /* Hack -- Choose starting point */
  y = rand_spread(DUNGEON_HGT / 2, DUNGEON_HGT / 4);
  x = rand_spread(DUNGEON_WID / 2, DUNGEON_WID / 4);
  
  /* Choose a random compass direction */
  dir = start_dir = ddd[rand_int(8)];
  
  /* Get an initial start position on the grid alteration table. */
  table_start = rand_int(47);
  
  /* Place streamer into dungeon */
  while (TRUE)
    {
      /* Advance streamer width steps on the table. */
      table_start += width;
      
      /*
       * Change grids outwards along sides.  If moving diagonally, 
       * change a cross-shaped area.
       */
      if (ddy[dir]) 
	{
	  for (dx = x - out1; dx <= x + out2; dx++)
	    {
	      /* Stay within dungeon. */
	      if (!in_bounds(y, dx)) continue;
	      
	      /* Only convert "granite" walls */
	      if (cave_feat[y][dx] < FEAT_WALL_EXTRA) continue;
	      if (cave_feat[y][dx] > FEAT_WALL_SOLID) continue;
	      
	      i = table_start + dx - x;
	      
	      if ((i < 47) && (i >= 0)) change = streamer_change_grid[i];
	      else change = streamer_change_grid[i % 47];
	      
	      /* No change to be made. */
	      if (!change) continue;
	      
	      /* Clear previous contents, add proper vein type */
	      cave_set_feat(y, dx, feat);
	      
	      /* Count down time to next treasure. */
	      time_to_treas--;
	      
	      /* Hack -- Add some (known) treasure */
	      if (time_to_treas == 0)
		{
		  time_to_treas = randint(chance * 2);
		  cave_feat[y][dx] += 0x04;
		}
	    }
	}
      
      if (ddx[dir]) 
	{
	  for (dy = y - out1; dy <= y + out2; dy++)
	    {
				/* Stay within dungeon. */
	      if (!in_bounds(dy, x)) continue;
	      
	      /* Only convert "granite" walls */
	      if (cave_feat[dy][x] < FEAT_WALL_EXTRA) continue;
	      if (cave_feat[dy][x] > FEAT_WALL_SOLID) continue;
	      
	      i = table_start + dy - y;
	      
	      if ((i < 47) && (i >= 0)) change = streamer_change_grid[i];
	      else change = streamer_change_grid[i % 47];
	      
	      /* No change to be made. */
	      if (!change) continue;
	      
	      /* Clear previous contents, add proper vein type */
	      cave_set_feat(dy, x, feat);

	      /* Count down time to next treasure. */
	      time_to_treas--;
	      
	      /* Hack -- Add some (known) treasure */
	      if (time_to_treas == 0)
		{
		  time_to_treas = randint(chance * 2);
		  cave_feat[dy][x] += 0x04;
		}
	    }
	}
      
      /* Count down to next direction change. */
      time_to_turn--;
      
      /* Sometimes, vary direction slightly. */
      if (time_to_turn == 0)
	{
	  /* Get time until next turn. */
	  time_to_turn = randint(DUN_STR_CHG * 2);
	  
	  /* Randomizer. */
	  i = rand_int(3);
	  
	  /* New direction is always close to start direction. */
	  if (start_dir == 2) 
	    {
	      if (i == 0) dir = 2;
	      if (i == 1) dir = 1;
	      else        dir = 3;
	    }
	  else if (start_dir == 8)
	    {
	      if (i == 0) dir = 8;
	      if (i == 1) dir = 9;
	      else        dir = 7;
	    }
	  else if (start_dir == 6)
	    {
	      if (i == 0) dir = 6;
	      if (i == 1) dir = 3;
	      else        dir = 9;
	    }
	  else if (start_dir == 4)
	    {
	      if (i == 0) dir = 4;
	      if (i == 1) dir = 7;
	      else        dir = 1;
	    }
	  else if (start_dir == 3)
	    {
	      if (i == 0) dir = 3;
	      if (i == 1) dir = 2;
	      else        dir = 6;
	    }
	  else if (start_dir == 1)
	    {
	      if (i == 0) dir = 1;
	      if (i == 1) dir = 4;
	      else        dir = 2;
	    }
	  else if (start_dir == 9)
	    {
	      if (i == 0) dir = 9;
	      if (i == 1) dir = 6;
	      else        dir = 8;
	    }
	  else if (start_dir == 7)
	    {
	      if (i == 0) dir = 7;
	      if (i == 1) dir = 8;
	      else        dir = 4;
	    }
	}
      
      /* Advance the streamer */
      y += ddy[dir];
      x += ddx[dir];
      
      /* Stop at dungeon edge */
      if (!in_bounds(y, x)) break;
    }
}



/**
 * Build a destroyed level
 */
void destroy_level(bool new_level)
{
  int y1, x1, y, x, k, t, n, epicenter_max;
  
  
  /* Note destroyed levels. */
  if (cheat_room && new_level) msg_print("Destroyed Level");
  
  /* determine the maximum number of epicenters. */
  if (new_level) epicenter_max = randint(5) + 1;
  else epicenter_max = randint(5) + 5;
  
  /* Drop a few epi-centers */
  for (n = 0; n < epicenter_max; n++)
    {
      /* Pick an epi-center */
      x1 = rand_range(5, DUNGEON_WID-1 - 5);
      y1 = rand_range(5, DUNGEON_HGT-1 - 5);
      
      /* Big area of affect */
      for (y = (y1 - 15); y <= (y1 + 15); y++)
	{
	  for (x = (x1 - 15); x <= (x1 + 15); x++)
	    {
	      /* Skip illegal grids */
	      if (!in_bounds_fully(y, x)) continue;
	      
	      /* Extract the distance */
	      k = distance(y1, x1, y, x);
	      
	      /* Stay in the circle of death */
	      if (k >= 16) continue;
	      
	      /* Delete the monster (if any) */
	      delete_monster(y, x);
	      
	      /* Destroy valid grids */
	      if (cave_valid_bold(y, x))
		{
		  /* Delete objects */
		  delete_object(y, x);
		  
		  /* Wall (or floor) type */
		  t = rand_int(200);
		  
		  /* Granite */
		  if (t < 20)
		    {
		      /* Create granite wall */
		      cave_set_feat(y, x, FEAT_WALL_EXTRA);
		    }
		  
		  /* Quartz */
		  else if (t < 60)
		    {
		      /* Create quartz vein */
		      cave_set_feat(y, x, FEAT_QUARTZ);
		    }
		  
		  /* Magma */
		  else if (t < 80)
		    {
		      /* Create magma vein */
		      cave_set_feat(y, x, FEAT_MAGMA);
		    }
		  
		  /* Rubble. */
		  else if (t < 130)
		    {
		      /* Create rubble */
		      cave_set_feat(y, x, FEAT_RUBBLE);
		    }
		  
		  /* Floor */
		  else
		    {
		      /* Create floor */
		      cave_set_feat(y, x, FEAT_FLOOR);
		    }
		  
		  /* No longer part of a room or vault */
		  cave_info[y][x] &= ~(CAVE_ROOM | CAVE_ICKY);
		  
		  /* No longer illuminated */
		  cave_info[y][x] &= ~(CAVE_GLOW);
		}
	    }
	}
    }
}




/**************************************************************/
/*                                                            */
/*                   The room-building code                   */
/*                                                            */
/**************************************************************/


/**
 * Place objects, up to the number asked for, in a rectangle centered on 
 * y0, x0.  Accept values for maximum vertical and horizontal displacement.
 *
 * Return prematurely if the code starts looping too much (this may happen 
 * if y0 or x0 are out of bounds, or the floor is already filled).
 */
static void spread_objects(int depth, int num, int y0, int x0, int dy, int dx)
{
  int i, j;	/* Limits on loops */
  int count;
  int y = y0, x = x0;
  
  
  /* Try to place objects within our rectangle of effect. */
  for (count = 0, i = 0; ((count < num) && (i < 50)); i++)
    {
      /* Get a location */
      if ((dy == 0) && (dx == 0))
	{
	  y = y0; x = x0;
	  if (!in_bounds(y, x)) return;
	}
      else
	{
	  for (j = 0; j < 10; j++)
	    {
	      y = rand_spread(y0, dy);
	      x = rand_spread(x0, dx);
	      if (!in_bounds(y, x))
		{
		  if (j < 9) continue;
		  else return;
		}
	      break;
	    }
	}
      
      /* Require "clean" floor space */
      if (!cave_clean_bold(y, x)) continue;
      
      /* Place an item */
      if (rand_int(100) < 67)
	{
	  place_object(y, x, FALSE, FALSE, FALSE);
	}
      
      /* Place gold */
      else
	{
	  place_gold(y, x);
	}
      
      /* Count the object, reset the loop count */
      count++;
      i = 0;
    }
}


/**
 * Place traps, up to the number asked for, in a rectangle centered on 
 * y0, x0.  Accept values for maximum vertical and horizontal displacement.
 *
 * Return prematurely if the code starts looping too much (this may happen 
 * if y0 or x0 are out of bounds, or the floor is already filled).
 */
static void spread_traps(int num, int y0, int x0, int dy, int dx)
{
  int i, j;	/* Limits on loops */
  int count;
  int y = y0, x = x0;
  
  /* Try to create traps within our rectangle of effect. */
  for (count = 0, i = 0; ((count < num) && (i < 50)); i++)
    {
      /* Get a location */
      if ((dy == 0) && (dx == 0))
	{
	  y = y0; x = x0;
	  if (!in_bounds(y, x)) return;
	}
      else
	{
	  for (j = 0; j < 10; j++)
	    {
	      y = rand_spread(y0, dy);
	      x = rand_spread(x0, dx);
	      if (!in_bounds(y, x))
		{
		  if (j < 9) continue;
		  else return;
		}
	      break;
	    }
	}
      
      /* Require "naked" floor grids */
      if (!cave_naked_bold(y, x)) continue;
      
      /* Place the trap */
      place_trap(y, x);
      
      /* Count the trap, reset the loop count */
      count++;
      i = 0;
    }
}


/**
 * Place monsters, up to the number asked for, in a rectangle centered on 
 * y0, x0.  Accept values for monster depth, symbol, and maximum vertical 
 * and horizontal displacement.  Call monster restriction functions if 
 * needed.
 *
 * Return prematurely if the code starts looping too much (this may happen 
 * if y0 or x0 are out of bounds, or the area is already occupied).
 */
static void spread_monsters(char symbol, int depth, int num, 
			    int y0, int x0, int dy, int dx)
{
  int i, j;	/* Limits on loops */
  int count;
  int y = y0, x = x0;
  int start_mon_num = m_max;
  bool dummy;
  
  /* Restrict monsters.  Allow uniques. */
  (void)mon_restrict(symbol, (byte)depth, &dummy, TRUE);
  
  /* Set generation level */
  monster_level = depth;
  
  /* Build the monster probability table. */
  if (!get_mon_num(depth)) return;
  
  
  /* Try to summon monsters within our rectangle of effect. */
  for (count = 0, i = 0; ((count < num) && (i < 50)); i++)
    {
      /* Get a location */
      if ((dy == 0) && (dx == 0))
	{
	  y = y0; x = x0;
	  if (!in_bounds(y, x)) return;
	}
      else
	{
	  for (j = 0; j < 10; j++)
	    {
	      y = rand_spread(y0, dy);
	      x = rand_spread(x0, dx);
	      if (!in_bounds(y, x))
		{
		  if (j < 9) continue;
		  else return;
		}
	      break;
	    }
	}
      
      /* Require "empty" floor grids */
      if (!cave_empty_bold(y, x)) continue;
      
      /* Place the monster (sleeping, allow groups, quickly) */
      (void)place_monster(y, x, TRUE, TRUE, TRUE);
      
      /* Rein in monster groups and escorts a little. */
      if (m_max - start_mon_num > num * 2) break;
      
      /* Count the monster(s), reset the loop count */
      count++;
      i = 0;
    }
  
  /* Remove monster restrictions. */
  (void)mon_restrict('\0', (byte)depth, &dummy, TRUE);
  
  /* Reset monster generation level. */
  monster_level = p_ptr->depth;
}



/**
 * Generate helper -- create a new room with optional light
 * 
 * Return FALSE if the room is not fully within the dungeon.
 */
static bool generate_room(int y1, int x1, int y2, int x2, int light)
{
  int y, x;
  
  /* Confirm that room is in bounds. */
  if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2))) return (FALSE);
  
  for (y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++)
	{
	  cave_info[y][x] |= (CAVE_ROOM);
	  if (light) cave_info[y][x] |= (CAVE_GLOW);
	}
    }
  
  /* Success. */
  return (TRUE);
}


/**
 * Generate helper -- fill a rectangle with a feature
 */
static void generate_fill(int y1, int x1, int y2, int x2, int feat)
{
  int y, x;
  
  for (y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++)
	{
	  cave_set_feat(y, x, feat);
	}
    }
}


/**
 * Generate helper -- mark a rectangle with a set of cave_info flags
 */
static void generate_mark(int y1, int x1, int y2, int x2, int flg)
{
  int y, x;
  
  for (y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++)
	{
	  cave_info[y][x] |= (flg);
	}
    }
}


/**
 * Generate helper -- draw a rectangle with a feature
 */
static void generate_draw(int y1, int x1, int y2, int x2, int feat)
{
  int y, x;
  
  for (y = y1; y <= y2; y++)
    {
      cave_set_feat(y, x1, feat);
      cave_set_feat(y, x2, feat);
    }
  
  for (x = x1; x <= x2; x++)
    {
      cave_set_feat(y1, x, feat);
      cave_set_feat(y2, x, feat);
    }
}


/**
 * Generate helper -- split a rectangle with a feature
 */
static void generate_plus(int y1, int x1, int y2, int x2, int feat)
{
  int y, x;
  int y0, x0;
  
  /* Center */
  y0 = (y1 + y2) / 2;
  x0 = (x1 + x2) / 2;
  
  for (y = y1; y <= y2; y++)
    {
      cave_set_feat(y, x0, feat);
    }
  
  for (x = x1; x <= x2; x++)
    {
      cave_set_feat(y0, x, feat);
    }
}


/**
 * Generate helper -- open all sides of a rectangle with a feature
 */
static void generate_open(int y1, int x1, int y2, int x2, int feat)
{
  int y0, x0;
  
  /* Center */
  y0 = (y1 + y2) / 2;
  x0 = (x1 + x2) / 2;
  
  /* Open all sides */
  cave_set_feat(y1, x0, feat);
  cave_set_feat(y0, x1, feat);
  cave_set_feat(y2, x0, feat);
  cave_set_feat(y0, x2, feat);
}


/**
 * Generate helper -- open one side of a rectangle with a feature
 */
static void generate_hole(int y1, int x1, int y2, int x2, int feat)
{
  int y0, x0;
  
  /* Center */
  y0 = (y1 + y2) / 2;
  x0 = (x1 + x2) / 2;
  
  /* Open random side */
  switch (rand_int(4))
    {
    case 0:
      {
	cave_set_feat(y1, x0, feat);
	break;
      }
    case 1:
      {
	cave_set_feat(y0, x1, feat);
	break;
      }
    case 2:
      {
	cave_set_feat(y2, x0, feat);
	break;
      }
    case 3:
      {
	cave_set_feat(y0, x2, feat);
	break;
      }
    }
}


/**
 * Find a good spot for the next room.  
 *
 * Find and allocate a free space in the dungeon large enough to hold 
 * the room calling this function.
 *
 * We allocate space in 11x11 blocks, but want to make sure that rooms 
 * align neatly on the standard screen.  Therefore, we make them use 
 * blocks in few 11x33 rectangles as possible.
 *
 * Be careful to include the edges of the room in height and width!
 *
 * Return TRUE and values for the center of the room if all went well.  
 * Otherwise, return FALSE.
 */
static bool find_space(int *y, int *x, int height, int width)
{
  int i;
  int by, bx, by1, bx1, by2, bx2;
  int block_y, block_x;
  
  bool filled;
  
  
  /* Find out how many blocks we need. */
  int blocks_high = 1 + ((height - 1) / BLOCK_HGT);
  int blocks_wide = 1 + ((width - 1) / BLOCK_WID);
  
  /* Sometimes, little rooms like to have more space. */
  if ((blocks_wide == 2) && (rand_int(3) == 0)) blocks_wide = 3;
  else if ((blocks_wide == 1) && (rand_int(2) == 0)) 
    blocks_wide = 1 + randint(2);
  
  
  /* We'll allow twenty-five guesses. */
  for (i = 0; i < 25; i++)
    {
      filled = FALSE;
      
      /* Pick a top left block at random */
      block_y = rand_int(dun->row_rooms + blocks_high);
      block_x = rand_int(dun->col_rooms + blocks_wide);
      
      
      /* Itty-bitty rooms can shift about within their rectangle */
      if (blocks_wide < 3)
	{
	  /* Rooms that straddle a border must shift. */
	  if ((blocks_wide == 2) && ((block_x % 3) == 2))
	    {
	      if (rand_int(2) == 0) block_x--;
	      else block_x++;
	    }
	}
      
      /* Rooms with width divisible by 3 get fitted to a rectangle. */
      else if ((blocks_wide % 3) == 0)
	{
	  /* Align to the left edge of a 11x33 rectangle. */
	  if ((block_x % 3) == 2) block_x++;
	  if ((block_x % 3) == 1) block_x--;
	}
      
      /*
       * Big rooms that do not have a width divisible by 3 get 
       * aligned towards the edge of the dungeon closest to them.
       */
      else
	{
	  /* Shift towards left edge of dungeon. */
	  if (block_x + (blocks_wide / 2) <= dun->col_rooms / 2)
	    {
	      if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2)) 
		block_x--;
	      if ((block_x % 3) == 1) block_x--;
	    }
	  
	  /* Shift toward right edge of dungeon. */
	  else
	    {
	      if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2)) 
		block_x++;
	      if ((block_x % 3) == 1) block_x++;
	    }
	}
      
      /* Extract blocks */
      by1 = block_y + 0;
      bx1 = block_x + 0;
      by2 = block_y + blocks_high;
      bx2 = block_x + blocks_wide;
      
      /* Never run off the screen */
      if ((by1 < 0) || (by2 > dun->row_rooms)) continue;
      if ((bx1 < 0) || (bx2 > dun->col_rooms)) continue;
      
      /* Verify available space */
      for (by = by1; by < by2; by++)
	{
	  for (bx = bx1; bx < bx2; bx++)
	    {
	      if (dun->room_map[by][bx])
		{
		  filled = TRUE;
		}
	    }
	}
      
      /* If space filled, try again. */
      if (filled) continue;
      
      
      /* It is *extremely* important that the following calculation */
      /* be *exactly* correct to prevent memory errors XXX XXX XXX */
      
      /* Acquire the location of the room */
      (*y) = ((by1 + by2) * BLOCK_HGT) / 2;
      (*x) = ((bx1 + bx2) * BLOCK_WID) / 2;
      
      
      /* Save the room location */
      if (dun->cent_n < CENT_MAX)
	{
	  dun->cent[dun->cent_n].y = *y;
	  dun->cent[dun->cent_n].x = *x;
	  dun->cent_n++;
	}
      
      /* Reserve some blocks.  Mark each with the room index. */
      for (by = by1; by < by2; by++)
	{
	  for (bx = bx1; bx < bx2; bx++)
	    {
	      dun->room_map[by][bx] = dun->cent_n;
	    }
	}
      
      /* Success. */
      return (TRUE);
    }
  
  /* Failure. */
  return (FALSE);
}



/**
 * Is a feature passable (at least in theory) by the character?
 */
static bool passable(int feat)
{
  /* Some kinds of terrain are passable. */
  if ((feat == FEAT_FLOOR) || 
      (feat == FEAT_SECRET) || 
      (feat == FEAT_RUBBLE) || 
      (feat == FEAT_WATER ) || 
      (feat == FEAT_TREE  ) || 
      (feat == FEAT_TREE2 ) || 
      (feat == FEAT_INVIS ) || 
      (feat == FEAT_GRASS_INVIS ) || 
      (feat == FEAT_LAVA  ) || 
      (feat == FEAT_OPEN  ) || 
      (feat == FEAT_BROKEN) || 
      (feat == FEAT_LESS  ) || 
      (feat == FEAT_MORE  )) return (TRUE);
  
  /* Doors are passable. */
  if ((feat >= FEAT_DOOR_HEAD) && 
      (feat <= FEAT_DOOR_TAIL)) return (TRUE);
  
  /* Everything else is not passable. */
  return (FALSE);
}



/**
 * Make a starburst room. -LM-
 *
 * Starburst rooms are made in three steps:
 * 1: Choose a room size-dependant number of arcs.  Large rooms need to 
 *    look less granular and alter their shape more often, so they need 
 *    more arcs.
 * 2: For each of the arcs, calculate the portion of the full circle it 
 *    includes, and its maximum effect range (how far in that direction 
 *    we can change features in).  This depends on room size, shape, and 
 *    the maximum effect range of the previous arc.
 * 3: Use the table "get_angle_to_grid" to supply angles to each grid in 
 *    the room.  If the distance to that grid is not greater than the 
 *    maximum effect range that applies at that angle, change the feature 
 *    if appropriate (this depends on feature type).
 *
 * Usage notes:
 * - This function uses a table that cannot handle distances larger than 
 *   20, so it calculates a distance conversion factor for larger rooms.
 * - This function is not good at handling rooms much longer along one axis 
 *   than the other, so it divides such rooms up, and calls itself to handle
 *   each section.  
 * - It is safe to call this function on areas that might contain vaults or 
 *   pits, because "icky" and occupied grids are left untouched.
 *
 * - Mixing these rooms (using normal floor) with rectangular ones on a 
 *   regular basis produces a somewhat chaotic looking dungeon.  However, 
 *   this code does works well for lakes, etc.
 *
 */
static bool generate_starburst_room(int y1, int x1, int y2, int x2,
				    bool light, int feat, bool special_ok)
{
  int y0, x0, y, x, ny, nx;
  int i, d;
  int dist, max_dist, dist_conv, dist_check;
  int height, width;
  int degree_first, center_of_arc, degree;
  
  /* Special variant room.  Discovered by accident. */
  bool make_cloverleaf = FALSE;
  
  /* Holds first degree of arc, maximum effect distance in arc. */
  int arc[45][2];
  
  /* Number (max 45) of arcs. */
  int arc_num;
  
  
  /* Make certain the room does not cross the dungeon edge. */
  if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2))) return (FALSE);
  
  /* Robustness -- test sanity of input coordinates. */
  if ((y1 + 2 >= y2) || (x1 + 2 >= x2)) return (FALSE);
  
  
  /* Get room height and width. */
  height = 1 + y2 - y1;
  width  = 1 + x2 - x1;
  

  /* Handle long, narrow rooms by dividing them up. */
  if ((height > 5 * width / 2) || (width > 5 * height / 2))
    {
      int tmp_ay, tmp_ax, tmp_by, tmp_bx;
      
      /* Get bottom-left borders of the first room. */
      tmp_ay = y2;
      tmp_ax = x2;
      if (height > width) tmp_ay = y1 + 2 * height / 3;
      else tmp_ax = x1 + 2 * width / 3;
      
      /* Make the first room. */
      (void)generate_starburst_room(y1, x1, tmp_ay, tmp_ax, 
				    light, feat, FALSE);
      
      
      /* Get top_right borders of the second room. */
      tmp_by = y1;
      tmp_bx = x1;
      if (height > width) tmp_by = y1 + 1 * height / 3;
		else tmp_bx = x1 + 1 * width / 3;

      /* Make the second room. */
      (void)generate_starburst_room(tmp_by, tmp_bx, y2, x2, 
				    light, feat, FALSE);
      
      
      /*
       * Hack -- extend a "corridor" between room centers, to ensure 
       * that the rooms are connected together.
       */
      if (feat == FEAT_FLOOR)
	{
	  for (y = (y1 + tmp_ay) / 2; y <= (tmp_by + y2) / 2; y++)
	    {
	      for (x = (x1 + tmp_ax) / 2; x <= (tmp_bx + x2) / 2; x++)
		{
		  cave_set_feat(y, x, feat);
		}
	    }
	}
      
      /*
       * Hack -- Because rubble and trees are added randomly, and because 
       * water should have smooth edges, we need to fill any gap between 
       * two starbursts of these kinds of terrain.
       */
      if ((feat == FEAT_TREE) || (feat == FEAT_TREE2) || 
	  (feat == FEAT_WATER) || (feat == FEAT_RUBBLE))
	{
	  int tmp_cy1, tmp_cx1, tmp_cy2, tmp_cx2;
	  
	  if (height > width)
	    {
	      tmp_cy1 = y1 + (height - width) / 2;
	      tmp_cx1 = x1;
	      tmp_cy2 = tmp_cy1 - (height - width) / 2;
	      tmp_cx2 = x2;
	    }
	  else
	    {
	      tmp_cy1 = y1;
	      tmp_cx1 = x1 + (width - height) / 2;
	      tmp_cy2 = y2;
	      tmp_cx2 = tmp_cx1 + (width - height) / 2;
	      
	      tmp_cy1 = y1;
	      tmp_cx1 = x1;
	    }
	  
	  /* Make the third room. */
	  (void)generate_starburst_room(tmp_cy1, tmp_cx1, tmp_cy2, 
					tmp_cx2, light, feat, FALSE);
	}
      
      /* Return. */
      return (TRUE);
    }
  
  
  /* Get a shrinkage ratio for large rooms, as table is limited. */
  if ((width > 44) || (height > 44))
    {
      if (width > height) dist_conv = 10 * width  / 44;
      else dist_conv = 10 * height / 44;
    }
  else dist_conv = 10;
  
  
  /* Make a cloverleaf room sometimes. */
  if ((special_ok) && (height > 10) && (rand_int(20) == 0))
    {
      arc_num = 12;
      make_cloverleaf = TRUE;
    }
  
  /* Usually, we make a normal starburst. */
  else
    {
      /* Ask for a reasonable number of arcs. */
      arc_num = 8 + (height * width / 80);
      arc_num = arc_num + 3 - rand_int(7);
      if (arc_num < 8) arc_num = 8;
      if (arc_num > 45) arc_num = 45;
    }
  
  
  /* Get the center of the starburst. */
  y0 = y1 + height / 2;
  x0 = x1 + width  / 2;
  
  /* Start out at zero degrees. */
  degree_first = 0;
  
  
  /* Determine the start degrees and expansion distance for each arc. */
  for (i = 0; i < arc_num; i++)
    {
      /* Get the first degree for this arc. */
      arc[i][0] = degree_first;
      
      /* Get a slightly randomized start degree for the next arc. */
      degree_first += (180 + rand_int(arc_num)) / arc_num;
      if (degree_first < 180 * (i+1) / arc_num) 
	degree_first = 180 * (i+1) / arc_num;
      if (degree_first > (180 + arc_num) * (i+1) / arc_num) 
	degree_first = (180 + arc_num) * (i+1) / arc_num;
      
      
      /* Get the center of the arc. */
      center_of_arc = degree_first + arc[i][0];
      
      
      /* Calculate a reasonable distance to expand vertically. */
      if (((center_of_arc > 45) && (center_of_arc < 135)) || 
	  ((center_of_arc > 225) && (center_of_arc < 315)))
	{
	  arc[i][1] = height / 4 + rand_int((height + 3) / 4);
	}
      
      /* Calculate a reasonable distance to expand horizontally. */
      else if (((center_of_arc < 45) || (center_of_arc > 315)) || 
	       ((center_of_arc < 225) && (center_of_arc > 135)))
	{
	  arc[i][1] = width / 4 + rand_int((width + 3) / 4);
	}
      
      /* Handle arcs that count as neither vertical nor horizontal */
      else if (i != 0)
	{
	  if (make_cloverleaf) arc[i][1] = 0;
	  else arc[i][1] = arc[i-1][1] + 3 - rand_int(7);
	}
      
      
      /* Keep variability under control. */
      if ((!make_cloverleaf) && (i != 0) && (i != arc_num - 1))
	{
	  /* Water edges must be quite smooth. */
	  if (feat == FEAT_WATER)
	    {
	      if (arc[i][1] > arc[i-1][1] + 2) 
		arc[i][1] = arc[i-1][1] + 2;
	      
	      if (arc[i][1] > arc[i-1][1] - 2) 
		arc[i][1] = arc[i-1][1] - 2;
	    }
	  else
	    {
	      if (arc[i][1] > 3 * (arc[i-1][1] + 1) / 2) 
		arc[i][1] = 3 * (arc[i-1][1] + 1) / 2;
	      
	      if (arc[i][1] < 2 * (arc[i-1][1] - 1) / 3) 
		arc[i][1] = 2 * (arc[i-1][1] - 1) / 3;
	    }
	}
      
      /* Neaten up final arc of circle by comparing it to the first. */
      if ((i == arc_num - 1) && (ABS(arc[i][1] - arc[0][1]) > 3))
	{
	  if (arc[i][1] > arc[0][1]) 
	    arc[i][1] -= rand_int(arc[i][1] - arc[0][1]);
	  else if (arc[i][1] < arc[0][1]) 
	    arc[i][1] += rand_int(arc[0][1] - arc[i][1]);
	}
    }
  
  
  /* Precalculate check distance. */
  dist_check = 21 * dist_conv / 10;
  
  /* Change grids between (and not including) the edges. */
  for (y = y1 + 1; y < y2; y++)
    {
      for (x = x1 + 1; x < x2; x++)
	{
	  /* Do not touch "icky" grids. */
	  if (cave_info[y][x] & (CAVE_ICKY)) continue;
	  
	  /* Do not touch occupied grids. */
	  if (cave_m_idx[y][x] != 0) continue;
	  if (cave_o_idx[y][x] != 0) continue;
	  
	  
	  /* Get distance to grid. */
	  dist = distance(y0, x0, y, x);
	  
	  /* Look at the grid if within check distance. */
	  if (dist < dist_check)
	    {
	      /* Convert and reorient grid for table access. */
	      ny = 20 + 10 * (y - y0) / dist_conv;
	      nx = 20 + 10 * (x - x0) / dist_conv;
	      
	      /* Illegal table access is bad. */
	      if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40))
		continue;
	      
	      /* Get angle to current grid. */
	      degree = get_angle_to_grid[ny][nx];
	      
	      /* Scan arcs to find the one that applies here. */
	      for (i = arc_num - 1; i >= 0; i--)
		{
		  if (arc[i][0] <= degree)
		    {
		      max_dist = arc[i][1];
		      
		      /* Must be within effect range. */
		      if (max_dist >= dist)
			{
			  /* If new feature is not passable, or floor, 
			   *  always place it. */
			  if ((feat == FEAT_FLOOR) || (!passable(feat)))
			    {
			      cave_set_feat(y, x, feat);
			      
			      if (feat == FEAT_FLOOR) 
				cave_info[y][x] |= (CAVE_ROOM);
			      else cave_info[y][x] &= ~(CAVE_ROOM);
			      
			      if (light) cave_info[y][x] |= (CAVE_GLOW);
			      else cave_info[y][x] &= ~(CAVE_GLOW);
			    }
			  
			  /* If new feature is non-floor passable terrain, 
			   *  place it only over floor. */
			  else
			    {
			      /* Replace old feature in some cases. */
			      if ((feat == FEAT_TREE) || (feat == FEAT_TREE2) 
				  || (feat == FEAT_RUBBLE))
				{
				  /* Make denser in the middle. */
				  if ((cave_feat[y][x] == FEAT_FLOOR) && 
				      (randint(max_dist + 5) >= dist + 5)) 
				    cave_set_feat(y, x, feat);
				}
			      if ((feat == FEAT_WATER) || (feat == FEAT_LAVA))
				{
				  if (cave_feat[y][x] == FEAT_FLOOR) 
				    cave_set_feat(y, x, feat);
				}
			      
			      /* Light grid. */
			      if (light) cave_info[y][x] |= (CAVE_GLOW);
			    }
			}
		      
		      /* Arc found.  End search */
		      break;
		    }
		}
	    }
	}
    }
  
  /*
   * If we placed floors or dungeon granite, all dungeon granite next 
   * to floors needs to become outer wall.
   */
  if ((feat == FEAT_FLOOR) || (feat == FEAT_WALL_EXTRA))
    {
      for (y = y1 + 1; y < y2; y++)
	{
	  for (x = x1 + 1; x < x2; x++)
	    {
	      /* Floor grids only */
	      if (cave_feat[y][x] == FEAT_FLOOR)
		{
		  /* Look in all directions. */
		  for (d = 0; d < 8; d++)
		    {
		      /* Extract adjacent location */
		      int yy = y + ddy_ddd[d];
		      int xx = x + ddx_ddd[d];
		      
		      /* Join to room */
		      cave_info[yy][xx] |= (CAVE_ROOM);
		      
		      /* Illuminate if requested. */
		      if (light) cave_info[yy][xx] |= (CAVE_GLOW);
		      
		      /* Look for dungeon granite. */
		      if (cave_feat[yy][xx] == FEAT_WALL_EXTRA)
			{
			  /* Turn into outer wall. */
			  cave_set_feat(yy, xx, FEAT_WALL_OUTER);
			}
		    }
		}
	    }
	}
    }
  
  /* Success */
  return (TRUE);
}






/**
 * Room building routines.
 *
 * Nine basic room types:
 *   1 -- normal
 *   2 -- overlapping
 *   3 -- cross shaped
 *   4 -- large room with features
 *   5 -- monster pits
 *   6 -- chambered rooms
 *   7 -- interesting rooms
 *   8 -- simple vaults
 *   9 -- greater vaults
 *
 */


/**
 * Special kind of room type 1, found only in moria-style dungeons.  Uses 
 * the "starburst room" code.
 */
static bool build_type1_moria(bool light)
{
  int y0, x0, y1, x1, y2, x2;
  int i;
  int height, width;
  int feat;
  
  int select = rand_int(120);
  
  /* Pick a room size */
  height = BLOCK_HGT;
  width = BLOCK_WID;
  
  
  /* Try twice to find space for a room. */
  for (i = 0; i < 2; i++)
    {
      /* Really large room - only on first try. */
      if ((i == 0) && (select % 15 == 0))
	{
	  height = (1 + randint(2)) * height;
	  width =  (2 + randint(3)) * width;
	}
      
      /* Long, narrow room.  Sometimes tall and thin. */
      else if (select % 4 != 0)
	{
	  if (select % 15 == 5) height = (2 + (select % 2)) * height;
	  else width = (2 + (select % 3)) * width;
	}
      
      /* Find and reserve some space in the dungeon.  Get center of room. */
      if (!find_space(&y0, &x0, height, width))
	{
	  if (i == 0) continue;
	  if (i == 1) return (FALSE);
	}
      else break;
    }
  
  /* Locate the room */
  y1 = y0 - height / 2;
  x1 = x0 - width / 2;
  y2 =  y1 + height - 1;
  x2 =  x1 + width - 1;
  
  
  /* Generate starburst room.  Return immediately if out of bounds. */
  if (!generate_starburst_room(y1, x1, y2, x2, light, FEAT_FLOOR, TRUE))
    {
      return (FALSE);
    }
  
  /* Sometimes, the room may have rubble, water, or trees in it. */
  select = rand_int(100);
  
  if (select >= 99) feat = FEAT_TREE;
  else if (select >= 98) feat = FEAT_TREE2;
  else if (select >= 92) feat = FEAT_WATER;
  else if (select >= 88) feat = FEAT_RUBBLE;
  else feat = 0;
  
  /* Generate special terrain if necessary. */
  if (feat)
    {
      (void)generate_starburst_room(y1 + rand_int(height / 4), 
				    x1 + rand_int(width / 4), 
				    y2 - rand_int(height / 4), 
				    x2 - rand_int(width / 4), 
				    FALSE, feat, FALSE);
    }
  
  /* Success */
  return (TRUE);
}


/**
 * Type 1 -- normal rectangular rooms
 *
 * These rooms have the lowest build priority (this means that they 
 * should not be very large), and are by far the most common type.
 */
static bool build_type1(void)
{
  int y, x, rand;
  int y0, x0;
  
  int y1, x1, y2, x2;
  
  bool light = FALSE;
  
  /* Occasional light */
  if ((p_ptr->depth <= randint(35)) && (!underworld)) light = TRUE;
  
  
  /* Use an alternative function if on a moria level. */
  if (moria_level) return (build_type1_moria(light));
  
  
  
  /* Pick a room size (less border walls) */
  x = 1 + randint(11) + randint(11);
  y = 1 + randint(4) + randint(4);
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y0, &x0, y+2, x+2)) return (FALSE);
  
  /* Locate the room */
  y1 = y0 - y / 2;
  x1 = x0 - x / 2;
  y2 =  y1 + y - 1;
  x2 =  x1 + x - 1;
  
  
  /* Generate new room.  Quit immediately if out of bounds. */
  if (!generate_room(y1-1, x1-1, y2+1, x2+1, light)) return (FALSE);
  
  
  /* Generate outer walls */
  generate_draw(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_OUTER);
  
  /* Make a standard room. */
  generate_fill(y1, x1, y2, x2, FEAT_FLOOR);
  
  /* Sometimes, we get creative. */
  if (rand_int(36) == 0)
    {
      /* Choose a room type.  Some types require odd dimensions. */
      if ((y % 2 == 0) || (x % 2 == 0)) rand = 60 + rand_int(40);
      else rand = rand_int(100);
      
      /* Pillar room (requires odd dimensions) */
      if (rand < 30)
	{
	  int offsety = 0;
	  int offsetx = 0;
	  if (rand_int(2) == 0) offsety = 1;
	  if (rand_int(2) == 0) offsetx = 1;
	  
	  for (y = y1 + offsety; y <= y2 - offsety; y += 2)
	    {
	      for (x = x1 + offsetx; x <= x2 - offsetx; x += 2)
		{
		  cave_set_feat(y, x, FEAT_WALL_INNER);
		}
	    }
	}
      
      /* Ragged-edge room (requires odd dimensions) */
      else if (rand < 60)
	{
	  int offset = 0;
	  if (rand_int(2) == 0) offset = 1;
	  
	  for (y = y1 + offset; y <= y2 - offset; y += 2)
	    {
	      cave_set_feat(y, x1, FEAT_WALL_INNER);
	      cave_set_feat(y, x2, FEAT_WALL_INNER);
	    }
	  
	  for (x = x1 + offset; x <= x2 - offset; x += 2)
	    {
	      cave_set_feat(y1, x, FEAT_WALL_INNER);
	      cave_set_feat(y2, x, FEAT_WALL_INNER);
	    }
	}
      
      /* The ceiling has collapsed. */
      else if (rand < 68)
	{
	  for (y = y1; y <= y2; y++)
	    {
	      for (x = x1; x <= x2; x++)
		{
		  /* Wall (or floor) type */
		  int t = rand_int(100);
		  
		  /* Granite */
		  if (t < 5)
		    {
		      /* Create granite wall */
		      cave_set_feat(y, x, FEAT_WALL_EXTRA);
		    }
		  
		  /* Quartz */
		  else if (t < 15)
		    {
		      /* Create quartz vein */
		      cave_set_feat(y, x, FEAT_QUARTZ);
		    }
		  
		  /* Magma */
		  else if (t < 25)
		    {
		      /* Create magma vein */
		      cave_set_feat(y, x, FEAT_MAGMA);
		    }
		  
		  /* Rubble. */
		  else if (t < 70)
		    {
		      /* Create rubble */
		      cave_set_feat(y, x, FEAT_RUBBLE);
		    }
		  
		  /* Floor */
		  else
		    {
		      /* Create floor */
		      cave_set_feat(y, x, FEAT_FLOOR);
		    }
		}
	    }
	  
	  /* Here, creatures of Earth dwell. */
	  if ((p_ptr->depth > 35) && (rand_int(3) == 0))
	    {
	      spread_monsters('X', p_ptr->depth, 2 + randint(3), 
			      y0, x0, 3, 9);
	      
	      /* No normal monsters. */
	      generate_mark(y1, x1, y2, x2, CAVE_TEMP);
	    }
	}
      
      /* A lake and/or a forest now fills the room. */
      else
	{
	  bool water_room = FALSE;
	  
	  /* Where there is water, ... */
	  if (generate_starburst_room(y1 + rand_int(3), x1 + rand_int(5), 
				      y2 - rand_int(3), x2 - rand_int(5), 
				      FALSE, FEAT_WATER, FALSE))
	    {
	      /* ... there may be water creatures, ... */
	      if ((p_ptr->depth > 15) && (rand_int(4) == 0))
		{
		  spread_monsters('6', p_ptr->depth, 2 + randint(4), 
				  y0, x0, 3, 7);
		  
		  water_room = TRUE;
		}
	    }
	  
	  /* ... or a little nature preserve. */
	  if ((!water_room) && (rand_int(2) == 0))
	    {
	      int feat = FEAT_TREE;

	      if (rand_int(2) == 0) feat = FEAT_TREE2;

	      /* From light and earth... */
	      /* Note that we also have to light the boundary walls. */
	      if (!light)
		{
		  for (y = y1 - 1; y <= y2 + 1; y++)
		    {
		      for (x = x1 - 1; x <= x2 + 1; x++)
			{
			  cave_info[y][x] |= (CAVE_GLOW);
			}
		    }
		}
	      
	      /* ... spring trees. */
	      (void)generate_starburst_room(y1, x1, y2, x2, FALSE, 
					    feat, FALSE);
	      
	      /* Animals love trees. */
	      if (rand_int(6) == 0)
		spread_monsters('3', p_ptr->depth, 2 + 
				rand_int(6), y0, x0, 4, 11);
	      else if (rand_int(3) == 0)
		spread_monsters('A', p_ptr->depth, 2 + 
				rand_int(5), y0, x0, 4, 11);
	    }
	}
    }
  
  /* Success */
  return (TRUE);
}


/**
 * Type 2 -- Overlapping rectangular rooms
 */
static bool build_type2(void)
{
  int y1a, x1a, y2a, x2a;
  int y1b, x1b, y2b, x2b;
  int y0, x0;
  int height, width;
  
  int light = FALSE;
  
  /* Occasional light */
  if (p_ptr->depth <= randint(35)) light = TRUE;
  
  
  /* Determine extents of room (a) */
  y1a = randint(4);
  x1a = randint(13);
  y2a = randint(3);
  x2a = randint(9);
  
  /* Determine extents of room (b) */
  y1b = randint(3);
  x1b = randint(9);
  y2b = randint(4);
  x2b = randint(13);
  
  
  /* Calculate height */
  height = 11;
  
  /* Calculate width */
  if ((x1a < 8) && (x2a < 9) && (x1b < 8) && (x2b < 9)) width = 22;
  else width = 33;
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y0, &x0, height, width)) return (FALSE);
  
  /* locate room (a) */
  y1a = y0 - y1a;
  x1a = x0 - x1a;
  y2a = y0 + y2a;
  x2a = x0 + x2a;
  
  /* locate room (b) */
  y1b = y0 - y1b;
  x1b = x0 - x1b;
  y2b = y0 + y2b;
  x2b = x0 + x2b;
  
  
  /* Generate new room (a).  Quit immediately if out of bounds. */
  if (!generate_room(y1a-1, x1a-1, y2a+1, x2a+1, light)) return (FALSE);
  
  /* Generate new room (b).  Quit immediately if out of bounds. */
  if (!generate_room(y1b-1, x1b-1, y2b+1, x2b+1, light)) return (FALSE);
  
  
  /* Generate outer walls (a) */
  generate_draw(y1a-1, x1a-1, y2a+1, x2a+1, FEAT_WALL_OUTER);
  
  /* Generate outer walls (b) */
  generate_draw(y1b-1, x1b-1, y2b+1, x2b+1, FEAT_WALL_OUTER);
  
  /* Generate inner floors (a) */
  generate_fill(y1a, x1a, y2a, x2a, FEAT_FLOOR);
  
  /* Generate inner floors (b) */
  generate_fill(y1b, x1b, y2b, x2b, FEAT_FLOOR);
  
  
  /* Sometimes, we get creative. */
  if ((p_ptr->depth >= 12) && (rand_int(40) == 0))
    {
      /* Demons have taken up residence. */
      if (rand_int(3) == 0)
	{
	  /* Pool of lava */
	  (void)generate_starburst_room(y0 - randint(2), x0 - randint(3), 
					y0 + randint(2), x0 + randint(3), 
					FALSE, FEAT_LAVA, FALSE);
	  
	  if (p_ptr->depth > 45) 
	    spread_monsters('U', p_ptr->depth, 3 + rand_int(5), y0, x0, 
			    height / 2, width / 2);
	  else spread_monsters('u', p_ptr->depth, 2 + rand_int(3), 
			       y0, x0, height / 2, width / 2);
	}
      
		/* Beings of frost or fire */
      else
	{
	  /* Get some monsters. */
	  spread_monsters('7', p_ptr->depth, 2 + rand_int(5), 
			  y0, x0, height / 2, width / 2);
	}
    }
  
  /* Success */
  return (TRUE);
}



/**
 * Type 3 -- Cross shaped rooms
 *
 * Room "a" runs north/south, and Room "b" runs east/east
 * So a "central pillar" would run from x1a,y1b to x2a,y2b.
 *
 * Note that currently, the "center" is always 3x3, but I think that
 * the code below will work for 5x5 (and perhaps even for asymmetric
 * values like 4x3 or 5x3 or 3x4 or 3x5).
 */
static bool build_type3(void)
{
  int y, x;
  int y0, x0;
  int height, width;
  
  int y1a, x1a, y2a, x2a;
  int y1b, x1b, y2b, x2b;
  
  int dy, dx, wy, wx;
  
  int light = FALSE;
  
  /* Occasional light */
  if (p_ptr->depth <= randint(35)) light = TRUE;
  
  
  /* Pick inner dimension */
  wy = 1;
  wx = 1;
  
  /* Pick outer dimension */
  dy = rand_range(3, 4);
  dx = rand_range(3, 11);
  
  /* Determine extents of room (a) */
  y1a = dy;
  x1a = wx;
  y2a = dy;
  x2a = wx;
  
  /* Determine extents of room (b) */
  y1b = wy;
  x1b = dx;
  y2b = wy;
  x2b = dx;
  
  /* Calculate height */
  if ((y1a + y2a + 1) > (y1b + y2b + 1)) height = y1a + y2a + 1;
  else height = y1b + y2b + 1;
  
  /* Calculate width */
  if ((x1a + x2a + 1) > (x1b + x2b + 1)) width = x1a + x2a + 1;
  else width = x1b + x2b + 1;
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y0, &x0, height, width)) return (FALSE);
  
  /* locate room (b) */
  y1a = y0 - dy;
  x1a = x0 - wx;
  y2a = y0 + dy;
  x2a = x0 + wx;
  
  /* locate room (b) */
  y1b = y0 - wy;
  x1b = x0 - dx;
  y2b = y0 + wy;
  x2b = x0 + dx;
  
  
  /* Generate new room (a).  Quit immediately if out of bounds. */
  if (!generate_room(y1a-1, x1a-1, y2a+1, x2a+1, light)) return (FALSE);
  
  /* Generate new room (b).  Quit immediately if out of bounds. */
  if (!generate_room(y1b-1, x1b-1, y2b+1, x2b+1, light)) return (FALSE);


  /* Generate outer walls (a) */
  generate_draw(y1a-1, x1a-1, y2a+1, x2a+1, FEAT_WALL_OUTER);
  
  /* Generate outer walls (b) */
  generate_draw(y1b-1, x1b-1, y2b+1, x2b+1, FEAT_WALL_OUTER);
  
  /* Generate inner floors (a) */
  generate_fill(y1a, x1a, y2a, x2a, FEAT_FLOOR);
  
  /* Generate inner floors (b) */
  generate_fill(y1b, x1b, y2b, x2b, FEAT_FLOOR);
  
  
  /* Special features */
  switch (randint(4))
    {
      /* Nothing */
    case 1:
      {
	break;
      }
      
      /* Large solid middle pillar */
    case 2:
      {
	/* Generate a small inner solid pillar */
	generate_fill(y1b, x1a, y2b, x2a, FEAT_WALL_INNER);
	
	break;
      }
      
      /* Inner treasure vault */
    case 3:
      {
	/* Generate a small inner vault */
	generate_draw(y1b, x1a, y2b, x2a, FEAT_WALL_INNER);
	
	/* Open the inner vault with a secret door */
	generate_hole(y1b, x1a, y2b, x2a, FEAT_SECRET);
	
	/* Place a treasure in the vault */
	object_level = p_ptr->depth + 2;
	place_object(y0, x0, FALSE, FALSE, FALSE);
	object_level = p_ptr->depth;
	
	/* Let's guard the treasure well */
	monster_level = p_ptr->depth + 4;
	(void)place_monster(y0, x0, TRUE, TRUE, FALSE);
	monster_level = p_ptr->depth;
	
	/* Traps, naturally. */
	spread_traps(randint(3), y0, x0, 4, 4);
	
	break;
      }
      
      /* Something else */
    case 4:
      {
	/* Occasionally pinch the center shut */
	if (rand_int(3) == 0)
	  {
	    /* Pinch the east/west sides */
	    for (y = y1b; y <= y2b; y++)
	      {
		if (y == y0) continue;
		cave_set_feat(y, x1a - 1, FEAT_WALL_INNER);
		cave_set_feat(y, x2a + 1, FEAT_WALL_INNER);
	      }
	    
	    /* Pinch the north/south sides */
	    for (x = x1a; x <= x2a; x++)
	      {
		if (x == x0) continue;
		cave_set_feat(y1b - 1, x, FEAT_WALL_INNER);
		cave_set_feat(y2b + 1, x, FEAT_WALL_INNER);
	      }
	    
	    /* Open sides with secret doors */
	    if (rand_int(3) == 0)
	      {
		generate_open(y1b-1, x1a-1, y2b+1, x2a+1, FEAT_SECRET);
	      }
	  }
	
	/* Occasionally put a "plus" in the center */
	else if (rand_int(3) == 0)
	  {
	    generate_plus(y1b, x1a, y2b, x2a, FEAT_WALL_INNER);
	  }
	
	/* Occasionally put a "pillar" in the center */
	else if (rand_int(3) == 0)
	  {
	    cave_set_feat(y0, x0, FEAT_WALL_INNER);
	  }
	
	break;
      }
    }
  
  /* Success */
  return (TRUE);
}


/**
 * Type 4 -- Large room with an inner room
 *
 * Possible sub-types:
 *	1 - An inner room with a small inner room
 *	2 - An inner room with a pillar or pillars
 *	3 - An inner room with a checkerboard
 *	4 - An inner room with four compartments
 */
static bool build_type4(void)
{
  int y, x, y1, x1, y2, x2;
  int y0, x0;
  
  int light = FALSE;
  
  /* Occasional light */
  if (p_ptr->depth <= randint(35)) light = TRUE;
  
  
  /* Pick a room size (less border walls) */
  y = 9;
  x = 23;
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y0, &x0, y+2, x+2)) return (FALSE);
  
  /* Locate the room */
  y1 = y0 - y / 2;
  x1 = x0 - x / 2;
  y2 =  y1 + y - 1;
  x2 =  x1 + x - 1;
  
  
  /* Generate new room.  Quit immediately if out of bounds. */
  if (!generate_room(y1-1, x1-1, y2+1, x2+1, light)) return (FALSE);
  
  
  /* Generate outer walls */
  generate_draw(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_OUTER);
  
  /* Generate inner floors */
  generate_fill(y1, x1, y2, x2, FEAT_FLOOR);
  
  
  /* The inner room */
  y1 = y1 + 2;
  y2 = y2 - 2;
  x1 = x1 + 2;
  x2 = x2 - 2;
  
  /* Generate inner walls */
  generate_draw(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_INNER);
  
  /* Inner room variations */
  switch (randint(4))
    {
      /* An inner room with a small inner room */
    case 1:
      {
	/* Open the inner room with a secret door */
	generate_hole(y1-1, x1-1, y2+1, x2+1, FEAT_SECRET);
	
	/* Place another inner room */
	generate_draw(y0-1, x0-1, y0+1, x0+1, FEAT_WALL_INNER);
	
	/* Open the inner room with a locked door */
	generate_hole(y0-1, x0-1, y0+1, x0+1, FEAT_DOOR_HEAD + randint(7));
	
	/* Monsters on guard */
	spread_monsters('\0', p_ptr->depth + 2, 4, y0, x0, 2, 6);
	
	/* Object (80%) */
	if (rand_int(100) < 80)
	  {
	    object_level = p_ptr->depth + 2;
	    place_object(y0, x0, FALSE, FALSE, FALSE);
	    object_level = p_ptr->depth;
	  }
	
	/* Stairs (20%) */
	else
	  {
	    place_random_stairs(y0, x0);
	  }
	
	/* Traps */
	spread_traps(rand_int(3) + 1, y0, x0, 2, 4);
	
	break;
      }
      
      
      /* An inner room with an inner pillar or pillars */
    case 2:
      {
	/* Open the inner room with a secret door */
	generate_hole(y1-1, x1-1, y2+1, x2+1, FEAT_SECRET);
	
	/* Inner pillar */
	generate_fill(y0-1, x0-1, y0+1, x0+1, FEAT_WALL_INNER);
	
	/* Occasionally, two more large inner pillars */
	if (rand_int(2) == 0)
	  {
	    /* Three spaces */
	    if (rand_int(100) < 50)
	      {
		/* Inner pillar */
		generate_fill(y0-1, x0-7, y0+1, x0-5, FEAT_WALL_INNER);
		
		/* Inner pillar */
		generate_fill(y0-1, x0+5, y0+1, x0+7, FEAT_WALL_INNER);
	      }
	    
	    /* Two spaces */
	    else
	      {
		/* Inner pillar */
		generate_fill(y0-1, x0-6, y0+1, x0-4, FEAT_WALL_INNER);
		
		/* Inner pillar */
		generate_fill(y0-1, x0+4, y0+1, x0+6, FEAT_WALL_INNER);
	      }
	  }
	
	/* Occasionally, some Inner rooms */
	if (rand_int(3) == 0)
	  {
	    /* Inner rectangle */
	    generate_draw(y0-1, x0-5, y0+1, x0+5, FEAT_WALL_INNER);
	    
	    /* Secret doors (random top/bottom) */
	    place_secret_door(y0 - 3 + (randint(2) * 2), x0 - 3);
	    place_secret_door(y0 - 3 + (randint(2) * 2), x0 + 3);
	    
	    /* Monsters */
	    spread_monsters('\0', p_ptr->depth, randint(4), y0, x0, 2, 7);
	    
	    /* Objects */
	    if (rand_int(3) == 0) 
	      place_object(y0, x0 - 2, FALSE, FALSE, FALSE);
	    if (rand_int(3) == 0) 
	      place_object(y0, x0 + 2, FALSE, FALSE, FALSE);
	  }
	
	break;
      }
      
      /* An inner room with a checkerboard */
    case 3:
      {
	/* Open the inner room with a secret door */
	generate_hole(y1-1, x1-1, y2+1, x2+1, FEAT_SECRET);
	
	/* Checkerboard */
	for (y = y1; y <= y2; y++)
	  {
	    for (x = x1; x <= x2; x++)
	      {
		if ((x + y) & 0x01)
		  {
		    cave_set_feat(y, x, FEAT_WALL_INNER);
		  }
	      }
	  }
	
	/* Monsters (especially undead) just love mazes. */
	if (rand_int(3) == 0)
	  spread_monsters('N', p_ptr->depth, randint(6), y0, x0, 2, 9);
	else if (rand_int(3) == 0)
	  spread_monsters('*', p_ptr->depth, randint(6), y0, x0, 2, 9);
	else
	  spread_monsters('\0', p_ptr->depth, randint(6), y0, x0, 2, 9);
	
	/* No random monsters. */
	generate_mark(y1, x1, y2, x2, CAVE_TEMP);
	
	/* Traps make them entertaining. */
	spread_traps(2 + rand_int(4), y0, x0, 2, 9);
	
	/* Mazes should have some pretty good treasure too. */
	spread_objects(p_ptr->depth, 2 + rand_int(4), y0, x0, 2, 9);
	
	break;
      }
      
      /* Four small rooms. */
    case 4:
      {
	/* Inner "cross" */
	generate_plus(y1, x1, y2, x2, FEAT_WALL_INNER);
	
	/* Doors into the rooms */
	if (rand_int(100) < 50)
	  {
	    int i = randint(10);
	    place_secret_door(y1 - 1, x0 - i);
	    place_secret_door(y1 - 1, x0 + i);
	    place_secret_door(y2 + 1, x0 - i);
	    place_secret_door(y2 + 1, x0 + i);
	  }
	else
	  {
	    int i = randint(3);
	    place_secret_door(y0 + i, x1 - 1);
	    place_secret_door(y0 - i, x1 - 1);
	    place_secret_door(y0 + i, x2 + 1);
	    place_secret_door(y0 - i, x2 + 1);
	  }
	
	/* Treasure, centered at the center of the cross */
	spread_objects(p_ptr->depth, 2 + randint(2), y0, x0, 1, 1);
	
	/* Gotta have some monsters */
	spread_monsters('\0', p_ptr->depth, 6 + rand_int(11), y0, x0, 2, 9);
	
	break;
      }
    }
  
  /* Success */
  return (TRUE);
}


/**
 * Type 5 -- Monster pits
 *
 * A monster pit is a 11x33 room, with an inner room filled with monsters.
 * 
 * The type of monsters is determined by inputing the current dungeon 
 * level into "mon_symbol_at_depth", and accepting the character returned.
 * After translating this into a set of selection criteria, monsters are 
 * chosen and arranged in the inner room.
 *
 * Monster pits will never contain unique monsters.
 *
 */
static bool build_type5(void)
{
  int y, x, y0, x0, y1, x1, y2, x2;
  int i, j;
  int depth;
  
  char *name;
  char symbol;
  
  bool ordered = FALSE;
  bool dummy;
  int light = FALSE;
  
  
  /* Pick a room size (less border walls) */
  y = 9;
  x = 23;
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y0, &x0, y+2, x+2)) return (FALSE);
  
  /* Locate the room */
  y1 = y0 - y / 2;
  x1 = x0 - x / 2;
  y2 =  y1 + y - 1;
  x2 =  x1 + x - 1;
  
  
  /* Generate new room.  Quit immediately if out of bounds. */
  if (!generate_room(y1-1, x1-1, y2+1, x2+1, light)) return (FALSE);
  
  
  /* Generate outer walls */
  generate_draw(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_OUTER);
  
  /* Generate inner floors */
  generate_fill(y1, x1, y2, x2, FEAT_FLOOR);
  
  /* Advance to the center room */
  y1 = y1 + 2;
  y2 = y2 - 2;
  x1 = x1 + 2;
  x2 = x2 - 2;
  
  /* Generate inner walls */
  generate_draw(y1-1, x1-1, y2+1, x2+1, FEAT_WALL_INNER);
  
  /* Open the inner room with a secret door */
  generate_hole(y1-1, x1-1, y2+1, x2+1, FEAT_SECRET);
  
  
  /* Get a legal depth. */
  depth = p_ptr->depth + rand_int(11) - 5;
  if (depth > 60) depth = 60;
  if (depth <  5) depth =  5;
  
  /* Choose a monster type, using that depth. */
  symbol = mon_symbol_at_depth[depth / 5 - 1][rand_int(7)];
  
  /* Allow tougher monsters. */
  depth = p_ptr->depth + 3 + (p_ptr->depth < 70 ? p_ptr->depth/7 : 10);
  
  
  /* 
   * Set monster generation restrictions.  Decide how to order 
   * monsters.  Get a description of the monsters.
   */
  name = mon_restrict(symbol, (byte)depth, &ordered, FALSE);
  
  /* A default description probably means trouble, so stop. */
  if (streq(name, "misc") || !name[0]) return (TRUE);
  
  /* Build the monster probability table.  Leave the room empty on failure. */
  if (!get_mon_num(depth)) return (TRUE);
  
  
  /* Arrange the monsters in the room randomly. */
  if (!ordered)
    {
      int r_idx = 0;
      
      /* Place some monsters */
      for (y = y0 - 2; y <= y0 + 2; y++)
	{
	  for (x = x0 - 9; x <= x0 + 9; x++)
	    {
	      /* Get a monster index */
	      r_idx = get_mon_num_quick(depth);
	      
	      /* Place a single monster */
	      (void)place_monster_aux(y, x, r_idx, FALSE, FALSE);
	    }
	}
    }
  
  /* Arrange the monsters in the room in an orderly fashion. */
  else
    {
      s16b what[16];
      
      /* Pick some monster types */
      for (i = 0; i < 16; i++)
	{
	  /* Get a monster index */
	  what[i] = get_mon_num_quick(depth);
	}
      
      /* Sort the monsters */
      for (i = 0; i < 16 - 1; i++)
	{
	  for (j = 0; j < 16 - 1; j++)
	    {
	      int i1 = j;
	      int i2 = j + 1;
	      
	      int p1 = r_info[what[i1]].level;
	      int p2 = r_info[what[i2]].level;

	      /* Bubble sort */
	      if (p1 > p2)
		{
		  int tmp = what[i1];
		  what[i1] = what[i2];
		  what[i2] = tmp;
		}
	    }
	}
      
      
      /* Top and bottom rows (outer) */
      for (x = x0 - 9; x <= x0 - 4; x++)
	{
	  place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE);
	  place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE);
	}
      for (x = x0 + 4; x <= x0 + 9; x++)
	{
	  place_monster_aux(y0 - 2, x, what[2], FALSE, FALSE);
	  place_monster_aux(y0 + 2, x, what[2], FALSE, FALSE);
	}
      
      /* Top and bottom rows (inner) */
      for (x = x0 - 3; x <= x0 + 3; x++)
	{
	  place_monster_aux(y0 - 2, x, what[3], FALSE, FALSE);
	  place_monster_aux(y0 + 2, x, what[3], FALSE, FALSE);
	}
      
      /* Middle columns */
      for (y = y0 - 1; y <= y0 + 1; y++)
	{
	  place_monster_aux(y, x0 - 9, what[2], FALSE, FALSE);
	  place_monster_aux(y, x0 + 9, what[2], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 8, what[4], FALSE, FALSE);
	  place_monster_aux(y, x0 + 8, what[4], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 7, what[5], FALSE, FALSE);
	  place_monster_aux(y, x0 + 7, what[5], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 6, what[6], FALSE, FALSE);
	  place_monster_aux(y, x0 + 6, what[6], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 5, what[7], FALSE, FALSE);
	  place_monster_aux(y, x0 + 5, what[7], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 4, what[8], FALSE, FALSE);
	  place_monster_aux(y, x0 + 4, what[8], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 3, what[9], FALSE, FALSE);
	  place_monster_aux(y, x0 + 3, what[9], FALSE, FALSE);
	  
	  place_monster_aux(y, x0 - 2, what[11], FALSE, FALSE);
	  place_monster_aux(y, x0 + 2, what[11], FALSE, FALSE);
	}
      
      /* Above/Below the center monster */
      for (x = x0 - 1; x <= x0 + 1; x++)
	{
	  place_monster_aux(y0 + 1, x, what[12], FALSE, FALSE);
	  place_monster_aux(y0 - 1, x, what[12], FALSE, FALSE);
	}
      
      /* Next to the center monster */
      place_monster_aux(y0, x0 + 1, what[14], FALSE, FALSE);
      place_monster_aux(y0, x0 - 1, what[14], FALSE, FALSE);
      
      /* Center monster */
      place_monster_aux(y0, x0, what[15], FALSE, FALSE);
    }
  
  /* Remove restrictions */
  (void)mon_restrict('\0', (byte)depth, &dummy, FALSE);
  
  
  /* Describe */
  if (cheat_room)
    {
      /* Room type */
      msg_format("Monster pit (%s)", name);
    }
  
  /* Increase the level rating */
  rating += 10;
  
  /* Sometimes cause a special feeling - removed in FA0.2.2 -NRM-
  if ((randint(50) >= p_ptr->depth) && (rand_int(2) == 0))
    {
      good_item_flag = TRUE;
    }
  */
  
  /* Success */
  return (TRUE);
}



/**
 * Helper function to "build_type6".  Fill a room matching 
 * the rectangle input with magma, and surround it with inner wall.  
 * Create a door in a random inner wall grid along the border of the 
 * rectangle.
 */
static void make_chamber(int c_y1, int c_x1, int c_y2, int c_x2)
{
  int i, d, y, x;
  int count;
  
  /* Fill with soft granite (will later be replaced with floor). */
  generate_fill(c_y1+1, c_x1+1, c_y2-1, c_x2-1, FEAT_MAGMA);
  
  /* Generate inner walls over dungeon granite and magma. */
  for (y = c_y1; y <= c_y2; y++)
    {
      /* left wall */
      x = c_x1;
      
      if ((cave_feat[y][x] == FEAT_WALL_EXTRA) ||  
	  (cave_feat[y][x] == FEAT_MAGMA)) 
	cave_set_feat(y, x, FEAT_WALL_INNER);
    }
  
  for (y = c_y1; y <= c_y2; y++)
    {
      /* right wall */
      x = c_x2;
      
      if ((cave_feat[y][x] == FEAT_WALL_EXTRA) || 
	  (cave_feat[y][x] == FEAT_MAGMA)) 
	cave_set_feat(y, x, FEAT_WALL_INNER);
    }
  
  for (x = c_x1; x <= c_x2; x++)
    {
      /* top wall */
      y = c_y1;
      
      if ((cave_feat[y][x] == FEAT_WALL_EXTRA) || 
	  (cave_feat[y][x] == FEAT_MAGMA)) 
	cave_set_feat(y, x, FEAT_WALL_INNER);
    }
  
  for (x = c_x1; x <= c_x2; x++)
    {
      /* bottom wall */
      y = c_y2;
      
      if ((cave_feat[y][x] == FEAT_WALL_EXTRA) || 
	  (cave_feat[y][x] == FEAT_MAGMA)) 
	cave_set_feat(y, x, FEAT_WALL_INNER);
    }
  
  /* Try a few times to place a door. */
  for (i = 0; i < 20; i++)
    {
      /* Pick a square along the edge, not a corner. */
      if (rand_int(2) == 0)
	{
	  /* Somewhere along the (interior) side walls. */
	  if (rand_int(2)) x = c_x1;
	  else x = c_x2;
	  y = c_y1 + rand_int(1 + ABS(c_y2 - c_y1));
	}
      else
	{
	  /* Somewhere along the (interior) top and bottom walls. */
	  if (rand_int(2)) y = c_y1;
	  else y = c_y2;
	  x = c_x1 + rand_int(1 + ABS(c_x2 - c_x1));
	}
      
      /* If not an inner wall square, try again. */
      if (cave_feat[y][x] != FEAT_WALL_INNER) continue;
      
      /* Paranoia */
      if (!in_bounds_fully(y, x)) continue;
      
      /* Reset wall count */
      count = 0;
      
      /* If square has not more than two adjacent walls, and no adjacent 
       * doors, place door. */
      for (d = 0; d < 9; d++)
	{
	  /* Extract adjacent (legal) location */
	  int yy = y + ddy_ddd[d];
	  int xx = x + ddx_ddd[d];
	  
	  /* No doors beside doors. */
	  if (cave_feat[yy][xx] == FEAT_OPEN) break;
	  
	  /* Count the inner walls. */
	  if (cave_feat[yy][xx] == FEAT_WALL_INNER) count++;
	  
	  /* No more than two walls adjacent (plus the one we're on). */
	  if (count > 3) break;
	  
	  /* Checked every direction? */
	  if (d == 8)
	    {
	      /* Place an open door. */
	      cave_set_feat(y, x, FEAT_OPEN);
	      
	      /* Success. */
	      return;
	    }
	}
    }
}



/**
 * Expand in every direction from a start point, turning magma into rooms.  
 * Stop only when the magma and the open doors totally run out.
 */
static void hollow_out_room(int y, int x)
{
  int d, yy, xx;
  
  for (d = 0; d < 9; d++)
    {
      /* Extract adjacent location */
      yy = y + ddy_ddd[d];
      xx = x + ddx_ddd[d];
      
      /* Change magma to floor. */
      if (cave_feat[yy][xx] == FEAT_MAGMA) 
	{
	  cave_set_feat(yy, xx, FEAT_FLOOR);
	  
	  /* Hollow out the room. */
	  hollow_out_room(yy, xx);
	}
      /* Change open door to broken door. */
      else if (cave_feat[yy][xx] == FEAT_OPEN) 
	{
	  cave_set_feat(yy, xx, FEAT_BROKEN);
	  
	  /* Hollow out the (new) room. */
	  hollow_out_room(yy, xx);
	}
    }
}



/**
 * Type 6 -- Rooms of chambers
 *
 * Build a room, varying in size between 22x22 and 44x66, consisting of 
 * many smaller, irregularly placed, chambers all connected by doors or 
 * short tunnels. -LM-
 *
 * Plop down an area-dependent number of magma-filled chambers, and remove 
 * blind doors and tiny rooms.
 *
 * Hollow out a chamber near the center, connect it to new chambers, and 
 * hollow them out in turn.  Continue in this fashion until there are no 
 * remaining chambers within two squares of any cleared chamber.
 *
 * Clean up doors.  Neaten up the wall types.  Turn floor grids into rooms, 
 * illuminate if requested.
 *
 * Fill the room with up to 35 (sometimes up to 50) monsters of a creature 
 * race or type that offers a challenge at the character's depth.  This is 
 * similar to monster pits, except that we choose among a wider range of 
 * monsters.
 *
 */
static bool build_type6(void)
{
  int i, d;
  int area, num_chambers;
  int y, x, yy, xx;
  int yy1, xx1, yy2, xx2, yy3, xx3;
  
  int height, width, count;
  
  int y0, x0, y1, x1, y2, x2;
  
  bool dummy;
  
  /* Monster generation variables. */
  s16b monsters_left, depth;
  char symbol;
  
  /* Description of monsters in room */
  char *name;
  
  /* Deeper in the dungeon, chambers are less likely to be lit. */
  bool light = (rand_int(45) > p_ptr->depth) ? TRUE : FALSE;
  
  
  /* Calculate a level-dependent room size modifier. */
  if (p_ptr->depth > rand_int(160)) i = 4;
  else if (p_ptr->depth > rand_int(100)) i = 3;
  else i = 2;
  
  /* Calculate the room size. */
  height = BLOCK_HGT * i;
  width = BLOCK_WID * (i + rand_int(3));
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y0, &x0, height, width)) return (FALSE);
  
  /* Calculate the borders of the room. */
  y1 = y0 - (height / 2);
  x1 = x0 - (width / 2);
  y2 = y0 + (height - 1) / 2;
  x2 = x0 + (width - 1) / 2;
  
  /* Make certain the room does not cross the dungeon edge. */
  if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2))) return (FALSE);
  
  
  /* Determine how much space we have. */
  area = ABS(y2 - y1) * ABS(x2 - x1);
  
  /* Calculate the number of smaller chambers to make. */
  num_chambers = 10 + area / 80;
  
  /* Build the chambers. */
  for (i = 0; i < num_chambers; i++)
    {
      int c_y1, c_x1, c_y2, c_x2;
      int size, width, height;
      
      /* Determine size of chamber. */
      size = 3 + rand_int(4);
      width = size + rand_int(10);
      height = size + rand_int(4);
      
      /* Pick an upper-left corner at random. */
      c_y1 = y1 + rand_int(1 + y2 - y1 - height);
      c_x1 = x1 + rand_int(1 + x2 - x1 - width);
      
      /* Determine lower-right corner of chamber. */
      c_y2 = c_y1 + height;
      if (c_y2 > y2) c_y2 = y2;
      
      c_x2 = c_x1 + width;
      if (c_x2 > x2) c_x2 = x2;
      
      /* Make me a (magma filled) chamber. */
      make_chamber(c_y1, c_x1, c_y2, c_x2);
    }
  
  /* Remove useless doors, fill in tiny, narrow rooms. */
  for (y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++)
	{
	  count = 0;
	  
	  /* Stay legal. */
	  if (!in_bounds_fully(y, x)) continue;
	  
	  /* Check all adjacent grids. */
	  for (d = 0; d < 8; d++)
	    {
	      /* Extract adjacent location */
	      yy = y + ddy_ddd[d];
	      xx = x + ddx_ddd[d];
	      
	      /* Count the walls and dungeon granite. */
	      if ((cave_feat[yy][xx] == FEAT_WALL_INNER) || 
		  (cave_feat[yy][xx] == FEAT_WALL_EXTRA)) count++;
	    }
	  
	  /* Five adjacent walls:  Change non-chamber to wall. */
	  if ((count == 5) && (cave_feat[y][x] != FEAT_MAGMA)) 
	    cave_set_feat(y, x, FEAT_WALL_INNER);
	  
	  /* More than five adjacent walls:  Change anything to wall. */
	  else if (count > 5) cave_set_feat(y, x, FEAT_WALL_INNER);
	}
    }
  
  /* Pick a random magma spot near the center of the room. */
  for (i = 0; i < 50; i++)
    {
      y = y1 + ABS(y2 - y1) / 4 + rand_int(ABS(y2 - y1) / 2);
      x = x1 + ABS(x2 - x1) / 4 + rand_int(ABS(x2 - x1) / 2);
      if (cave_feat[y][x] == FEAT_MAGMA) break;
    }
  
  
  /* Hollow out the first room. */
  cave_set_feat(y, x, FEAT_FLOOR);
  hollow_out_room(y, x);
  
  
  /* Attempt to change every in-room magma grid to open floor. */
  for (i = 0; i < 100; i++)
    {
      /* Assume this run will do no useful work. */
      bool joy = FALSE;
      
      /* Make new doors and tunnels between magma and open floor. */
      for (y = y1; y < y2; y++)
	{
	  for (x = x1; x < x2; x++)
	    {
	      /* Current grid must be magma. */
	      if (cave_feat[y][x] != FEAT_MAGMA) continue;
	      
	      /* Stay legal. */
	      if (!in_bounds_fully(y, x)) continue;
	      
	      /* Check only horizontal and vertical directions. */
	      for (d = 0; d < 4; d++)
		{
		  /* Extract adjacent location */
		  yy1 = y + ddy_ddd[d];
		  xx1 = x + ddx_ddd[d];
		  
		  /* Find inner wall. */
		  if (cave_feat[yy1][xx1] == FEAT_WALL_INNER) 
		    {
		      /* Keep going in the same direction. */
		      yy2 = yy1 + ddy_ddd[d];
		      xx2 = xx1 + ddx_ddd[d];
		      
		      /* If we find open floor, place a door. */
		      if ((in_bounds(yy2, xx2)) && 
			  (cave_feat[yy2][xx2] == FEAT_FLOOR))
			{
			  joy = TRUE;
			  
			  /* Make a broken door in the wall grid. */
			  cave_set_feat(yy1, xx1, FEAT_BROKEN);
			  
			  /* Hollow out the new room. */
			  cave_set_feat(y, x, FEAT_FLOOR);
			  hollow_out_room(y, x);
			  
			  break;
			}
		      
		      /* If we find more inner wall... */
		      if ((in_bounds(yy2, xx2)) && 
			  (cave_feat[yy2][xx2] == FEAT_WALL_INNER))
			{
			  /* ...Keep going in the same direction. */
			  yy3 = yy2 + ddy_ddd[d];
			  xx3 = xx2 + ddx_ddd[d];
			  
			  /* If we /now/ find floor, make a tunnel. */
			  if ((in_bounds(yy3, xx3)) && 
			      (cave_feat[yy3][xx3] == FEAT_FLOOR))
			    {
			      joy = TRUE;
			      
			      /* Turn both wall grids into floor. */
			      cave_set_feat(yy1, xx1, FEAT_FLOOR);
			      cave_set_feat(yy2, xx2, FEAT_FLOOR);
			      
			      /* Hollow out the new room. */
			      cave_set_feat(y, x, FEAT_FLOOR);
			      hollow_out_room(y, x);
			      
			      break;
			    }
			}
		    }
		}
	    }
	}
      
      /* If we could find no work to do, stop. */
      if (!joy) break;
    }
  
  
  /* Turn broken doors into a random kind of door, remove open doors. */
  for (y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++)
	{
	  if (cave_feat[y][x] == FEAT_OPEN) 
	    cave_set_feat(y, x, FEAT_WALL_INNER);
	  else if (cave_feat[y][x] == FEAT_BROKEN) 
	    place_random_door(y, x);
	}
    }
  
  
  /* Turn all walls and magma not adjacent to floor into dungeon granite. */
  /* Turn all floors and adjacent grids into rooms, sometimes lighting them. */
  for (y = (y1-1 > 0 ? y1-1 : 0) ; 
       y < (y2+2 < DUNGEON_HGT ? y2+2 : DUNGEON_HGT) ; y++)
    {
      for (x = (x1-1 > 0 ? x1-1 : 0) ; 
	   x < (x2+2 < DUNGEON_WID ? x2+2 : DUNGEON_WID) ; x++)
	{
	  if ((cave_feat[y][x] == FEAT_WALL_INNER) || 
	      (cave_feat[y][x] == FEAT_MAGMA))
	    {
	      for (d = 0; d < 9; d++)
		{
		  /* Extract adjacent location */
		  int yy = y + ddy_ddd[d];
		  int xx = x + ddx_ddd[d];
		  
		  /* Stay legal */
		  if (!in_bounds(yy, xx)) continue;
		  
		  /* No floors allowed */
		  if (cave_feat[yy][xx] == FEAT_FLOOR) break;
		  
		  /* Turn me into dungeon granite. */
		  if (d == 8)
		    {
		      cave_set_feat(y, x, FEAT_WALL_EXTRA);
		    }
		}
	    }
	  if (cave_feat[y][x] == FEAT_FLOOR)
	    {
	      for (d = 0; d < 9; d++)
		{
		  /* Extract adjacent location */
		  int yy = y + ddy_ddd[d];
		  int xx = x + ddx_ddd[d];
		  
		  /* Stay legal */
		  if (!in_bounds(yy, xx)) continue;
		  
		  /* Turn into room. */
		  cave_info[yy][xx] |= (CAVE_ROOM);
		  
		  /* Illuminate if requested. */
		  if (light) cave_info[yy][xx] |= (CAVE_GLOW);
		}
	    }
	}
    }
  
  
  /* Turn all inner wall grids adjacent to dungeon granite into outer walls. */
  for (y = (y1-1 > 0 ? y1-1 : 0) ; 
       y < (y2+2 < DUNGEON_HGT ? y2+2 : DUNGEON_HGT) ; y++)
    {
      for (x = (x1-1 > 0 ? x1-1 : 0) ; 
	   x < (x2+2 < DUNGEON_WID ? x2+2 : DUNGEON_WID) ; x++)
	{
	  /* Stay legal. */
	  if (!in_bounds_fully(y, x)) continue;
	  
	  if (cave_feat[y][x] == FEAT_WALL_INNER)
	    {
	      for (d = 0; d < 9; d++)
		{
		  /* Extract adjacent location */
		  int yy = y + ddy_ddd[d];
		  int xx = x + ddx_ddd[d];
		  
		  /* Look for dungeon granite */
		  if (cave_feat[yy][xx] == FEAT_WALL_EXTRA)
		    {
		      /* Turn me into outer wall. */
		      cave_set_feat(y, x, FEAT_WALL_OUTER);
		      
		      /* Done; */
		      break;
		    }
		}
	    }
	}
    }
  
  
  /*** Now we get to place the monsters. ***/
  
  /* Get a legal depth. */
  depth = p_ptr->depth + rand_int(11) - 5;
  if (depth > 60) depth = 60;
  if (depth <  5) depth =  5;
  
  /* Choose a monster type, using that depth. */
  symbol = mon_symbol_at_depth[depth / 5 - 1][rand_int(13)];
  
  /* Allow (slightly) tougher monsters. */
  depth = p_ptr->depth + (p_ptr->depth < 60 ? p_ptr->depth / 12 : 5);
  
  
  /* Set monster generation restrictions.  Describe the monsters. */
  name = mon_restrict(symbol, (byte)depth, &dummy, TRUE);
  
  /* A default description probably means trouble, so stop. */
  if (streq(name, "misc") || !name[0]) return (TRUE);
  
  /* Build the monster probability table. */
  if (!get_mon_num(depth)) return (TRUE);
  
  
  /* No normal monsters. */
  generate_mark(y1, x1, y2, x2, CAVE_TEMP);
  
  
  /* Usually, we want 35 monsters. */
  monsters_left = 35;
  
  /* Fewer monsters near the surface. */
  if (p_ptr->depth < 45) monsters_left = 5 + 2 * p_ptr->depth / 3;
  
  /* More monsters of kinds that tend to be weak. */
  if (strstr("abciBCFKRS", d_char_req)) monsters_left += 15;
  
  /* Place the monsters. */
  for(i = 0; i < 300; i++)
    {
      /* Check for early completion. */
      if (!monsters_left) break;
      
      /* Pick a random in-room square. */
      y = y1 + rand_int(1 + ABS(y2 - y1));
      x = x1 + rand_int(1 + ABS(x2 - x1));
      
      /* Require a floor square with no monster in it already. */
      if (!cave_naked_bold(y, x)) continue;
      
      /* Place a single monster.  Sleeping 2/3rds of the time. */
      place_monster_aux(y, x, get_mon_num_quick(depth), 
			(rand_int(3) != 0), FALSE);
      
      /* One less monster to place. */
      monsters_left--;
    }
  
  /* Remove our restrictions. */
  (void)mon_restrict('\0', (byte)depth, &dummy, FALSE);
  
  
  /* Describe */
  if (cheat_room)
    {
      /* Room type */
      msg_format("Room of chambers (%s)", name);
    }
  
  /* Increase the level rating */
  rating += 10;
  
  /* (Sometimes) Cause a "special feeling". removed in FA0.2.2 -NRM-
  if ((randint(50) >= p_ptr->depth) && (rand_int(2) == 0))
    {
      good_item_flag = TRUE;
    }
  */
  
  /* Success. */
  return (TRUE);
}



/**
 * Apply any general restrictions on monsters in vaults and themed levels.
 */
static void general_monster_restrictions(void)
{
  int i;
  
  /* Clear global monster restriction variables. */
  allow_unique = TRUE;
  for (i = 0; i < 10; i++) d_char_req[i] = '\0';
  for (i = 0; i < 4; i++) d_attr_req[i] = 0;
  racial_flag_mask = 0, breath_flag_mask = 0;
  
  /* Assume no restrictions. */
  get_mon_num_hook = NULL;
  
  
  /* Most themed levels have monster restrictions. */
  switch (p_ptr->themed_level)
    {
    case THEME_ELEMENTAL:
      {
	/* Handled with a call from mon_select() */
	get_mon_num_hook = mon_select;
	
	break;
      }
    case THEME_DRAGON:
      {
	strcpy(d_char_req, "dD");
	get_mon_num_hook = mon_select;
	break;
      }
    case THEME_WILDERNESS:
      {
	racial_flag_mask = RF3_ANIMAL;
	get_mon_num_hook = mon_select;
	break;
      }
    case THEME_DEMON:
      {
	racial_flag_mask = RF3_DEMON;
	get_mon_num_hook = mon_select;
	break;
      }
    case THEME_MINE:
      {
	strcpy(d_char_req, "oOPT");
	get_mon_num_hook = mon_select;
	break;
      }
    case THEME_ESTOLAD:
      {
	racial_flag_mask = RF3_RACIAL;
	get_mon_num_hook = mon_select;
	break;
      }
      
      /* All non-themed levels, and any themed levels not defined */
    default:
      {
	break;
      }
    }
  
  /* Prepare allocation table. */
  get_mon_num_prep();
}


/**
 * Hack -- fill in "vault" rooms and themed levels
 */
static bool build_vault(int y0, int x0, int ymax, int xmax, cptr data, 
			bool light, bool icky, byte vault_type)
{
  int x, y, i;
  int y1, x1, y2, x2, panic_y = 0, panic_x = 0;
  int temp, races = 0;
  
  bool placed = FALSE;
  
  cptr t;
  char racial_symbol[30] = "";
  
  /* Bail if no vaults allowed on this stage */
  if (no_vault()) return (FALSE);
  
  /* Calculate the borders of the vault. */
  if (p_ptr->themed_level)
    {
      y1 = y0;
      x1 = x0;
      y2 = x0 + ymax - 1;
      x2 = x0 + xmax - 1;
    }
  else
    {
      y1 = y0 - (ymax / 2);
      x1 = x0 - (xmax / 2);
      y2 = y1 + ymax - 1;
      x2 = x1 + xmax - 1;
    }
  
  /* Make certain that the vault does not cross the dungeon edge. */
  if ((!in_bounds(y1, x1)) || (!in_bounds(y2, x2))) return (FALSE);
  
  
  /* No random monsters in vaults and interesting rooms. */
  if (!p_ptr->themed_level) generate_mark(y1, x1, y2, x2, CAVE_TEMP);
  
  
  /* 
   * Themed levels usually have monster restrictions that take effect 
   * if no other restrictions are currently in force.  This can be ex-
   * panded to vaults too - imagine a "sorcerer's castle"...
   */
  if (p_ptr->themed_level) general_monster_restrictions();
  
  
  /* Place dungeon features and objects */
  for (t = data, y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++, t++)
	{
	  /* Hack -- skip "non-grids" */
	  if (*t == ' ') 
	    {
	      continue;
	    }
	  
	  /* Lay down a floor or grass */
	  if (((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) ||
	       (stage_map[p_ptr->stage][STAGE_TYPE] == DESERT) ||
	       (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAIN)) &&
	      (p_ptr->themed_level != THEME_SLAIN))
	    cave_set_feat(y, x, FEAT_FLOOR);
	  else
	    cave_set_feat(y, x, FEAT_GRASS);
	  
	  /* Part of a vault.  Can be lit.  May be "icky". */
	  if (icky) cave_info[y][x] |= (CAVE_ROOM | CAVE_ICKY);
	  else if (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) 
	    cave_info[y][x] |= (CAVE_ROOM);
	  if (light) cave_info[y][x] |= (CAVE_GLOW);
	  
	  /* Analyze the grid */
	  switch (*t)
	    {
	      /* Granite wall (outer) or outer edge of dungeon level or web. */
	    case '%':
	      {
		if (p_ptr->themed_level)
		  cave_set_feat(y, x, FEAT_PERM_SOLID);
		else if (stage_map[p_ptr->stage][STAGE_TYPE] == VALLEY)
		  cave_set_feat(y, x, FEAT_WEB);
		else
		  cave_set_feat(y, x, FEAT_WALL_OUTER);
		break;
	      }
	      /* Granite wall (inner) */
	    case '#':
	      {
		cave_set_feat(y, x, FEAT_WALL_INNER);
		break;
	      }
	      /* Permanent wall (inner) */
	    case 'X':
	      {
		cave_set_feat(y, x, FEAT_PERM_INNER);
		break;
	      }
	      /* Treasure seam, in either magma or quartz. */
	    case '*':
	      {
		if (randint(2) == 1) 
		  cave_set_feat(y, x, FEAT_MAGMA_K);
		else cave_set_feat(y, x, FEAT_QUARTZ_K);
		break;
	      }
	      /* Lava. */
	    case '@':
	      {
		cave_set_feat(y, x, FEAT_LAVA);
		break;
	      }
	      /* Water. */
	    case 'x':
	      {
		cave_set_feat(y, x, FEAT_WATER);
		break;
	      }
	      /* Tree. */
	    case ';':
	      {
		if (randint(p_ptr->depth + HIGHLAND_TREE_CHANCE) 
		    > HIGHLAND_TREE_CHANCE)
		  cave_set_feat(y, x, FEAT_TREE2);
		else
		  cave_set_feat(y, x, FEAT_TREE);		  
		break;
	      }
	      /* Rubble. */
	    case ':':
	      {
		cave_set_feat(y, x, FEAT_RUBBLE);
		break;
	      }
	      /* Sand dune */
	    case '/':
	      {
		cave_set_feat(y, x, FEAT_DUNE);
		break;
	      }
	      /* Treasure/trap */
	    case '&':
	      {
		if (rand_int(100) < 50)
		  {
		    place_object(y, x, FALSE, FALSE, FALSE);
		  }
		else
		  {
		    place_trap(y, x);
		  }
		break;
	      }
	      /* Secret doors */
	    case '+':
	      {
		place_secret_door(y, x);
		break;
	      }
	      /* Trap */
	    case '^':
	      {
		place_trap(y, x);
		break;
	      }
	      /* Up stairs (and player location in themed level).  */
	    case '<':
	      {
		if (stage_map[p_ptr->stage][UP]) cave_set_feat(y, x, FEAT_LESS);
		
		/* Place player only in themed level, and only once. */
		if ((p_ptr->themed_level) && (!placed))
		  {
		    player_place(y, x);
		    placed = TRUE;
		  }
		
		break;
	      }
	      /* Down stairs. */
	    case '>':
	      {
		/* No down stairs at bottom or on quests */
		if (is_quest(p_ptr->stage) || 
		    (!stage_map[p_ptr->stage][DOWN])) break;
		
		cave_set_feat(y, x, FEAT_MORE);
		break;
	      }
	      /* Wilderness paths. */
	    case '\\':
	      {
		int adj;
		byte dir;
		bool more;

		/* Work out which direction */
		if (y == 1) dir = NORTH;
		else if (x == 1) dir = WEST;
		else if (y == DUNGEON_HGT - 2) dir = SOUTH;
		else if (x == DUNGEON_WID - 2) dir = EAST;
		else break;
		adj = stage_map[p_ptr->stage][dir];
		
		/* Cancel the path if nowhere to go */
		if (!adj) break;
		
		/* Set the feature */
		more = (stage_map[adj][DEPTH] > p_ptr->depth);
		switch (dir)
		  {		
		  case NORTH:
		    {
		      if (more) cave_set_feat(y, x, FEAT_MORE_NORTH);
		      else cave_set_feat(y, x, FEAT_LESS_NORTH);
		      break;
		    }
		  case EAST:
		    {
		      if (more) cave_set_feat(y, x, FEAT_MORE_EAST);
		      else cave_set_feat(y, x, FEAT_LESS_EAST);
		      break;
		    }
		  case SOUTH:
		    {
		      if (more) cave_set_feat(y, x, FEAT_MORE_SOUTH);
		      else cave_set_feat(y, x, FEAT_LESS_SOUTH);
		      break;
		    }
		  case WEST:
		    {
		      if (more) cave_set_feat(y, x, FEAT_MORE_WEST);
		      else cave_set_feat(y, x, FEAT_LESS_WEST);
		      break;
		    }
		  }

		/* Place the player? */
		if ((adj == p_ptr->last_stage) && (p_ptr->themed_level) && 
		    (!placed))
		  {
		    player_place(y, x);
		    placed = TRUE;
		  }
		else
		  {
		    panic_y = y;
		    panic_x = x;
		  }
		break;
	      }
	    }
	}
    }
  
  /* Place dungeon monsters and objects */
  for (t = data, y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++, t++)
	{
	  /* Hack -- skip "non-grids" */
	  if (*t == ' ') continue;
	  
	  /* Most alphabetic characters signify monster races. */
	  if (isalpha(*t) && (*t != 'x') && (*t != 'X'))
	    {
	      /* If the symbol is not yet stored, ... */
	      if (!strchr(racial_symbol, *t))
		{
		  /* ... store it for later processing. */
		  if (races < 30) racial_symbol[races++] = *t;
		}
	    }
	  
	  /* Otherwise, analyze the symbol */
	  else switch (*t)
	    {
	      /* An ordinary monster, object (sometimes good), or trap. */
	    case '1':
	      {
		int rand = rand_int(4);
		
		if (rand < 2)
		  {
		    place_monster(y, x, TRUE, TRUE, FALSE);
		  }
		
		/* I had not intended this function to create 
		 * guaranteed "good" quality objects, but perhaps 
		 * it's better that it does at least sometimes.
		 */
		else if (rand == 2)
		  {
		    if (rand_int(8) == 0) 
		      place_object(y, x, TRUE, FALSE, FALSE);
		    else place_object(y, x, FALSE, FALSE, FALSE);
		    
		  }
		else
		  {
		    place_trap(y, x);
		  }
		break;
	      }
	      /* Slightly out of depth monster. */
	    case '2':
	      {
		monster_level = p_ptr->depth + 3;
		place_monster(y, x, TRUE, TRUE, FALSE);
		monster_level = p_ptr->depth;
		break;
	      }
	      /* Slightly out of depth object. */
	    case '3':
	      {
		object_level = p_ptr->depth + 3;
		place_object(y, x, FALSE, FALSE, FALSE);
		object_level = p_ptr->depth;
		break;
	      }
	      /* Monster and/or object */
	    case '4':
	      {
		if (rand_int(100) < 50)
		  {
		    monster_level = p_ptr->depth + 4;
		    place_monster(y, x, TRUE, TRUE, FALSE);
		    monster_level = p_ptr->depth;
		  }
		if (rand_int(100) < 50)
		  {
		    object_level = p_ptr->depth + 4;
		    place_object(y, x, FALSE, FALSE, FALSE);
		    object_level = p_ptr->depth;
		  }
		break;
	      }
	      /* Out of depth object. */
	    case '5':
	      {
		object_level = p_ptr->depth + 7;
		place_object(y, x, FALSE, FALSE, FALSE);
		object_level = p_ptr->depth;
		break;
	      }
	      /* Out of depth monster. */
	    case '6':
	      {
		monster_level = p_ptr->depth + 7;
		place_monster(y, x, TRUE, TRUE, FALSE);
		monster_level = p_ptr->depth;
		break;
	      }
	      /* Very out of depth object. */
	    case '7':
	      {
		object_level = p_ptr->depth + 15;
		place_object(y, x, FALSE, FALSE, FALSE);
		object_level = p_ptr->depth;
		break;
	      }
	      /* Very out of depth monster. */
	    case '8':
	      {
		monster_level = p_ptr->depth + 20;
		place_monster(y, x, TRUE, TRUE, FALSE);
		monster_level = p_ptr->depth;
		break;
	      }
	      /* Meaner monster, plus "good" (or better) object */
	    case '9':
	      {
		monster_level = p_ptr->depth + 15;
		place_monster(y, x, TRUE, TRUE, FALSE);
		monster_level = p_ptr->depth;
		object_level = p_ptr->depth + 5;
		place_object(y, x, TRUE, FALSE, FALSE);
		object_level = p_ptr->depth;
		break;
	      }
	      
	      /* Nasty monster and "great" (or better) object */
	    case '0':
	      {
		monster_level = p_ptr->depth + 30;
		place_monster(y, x, TRUE, TRUE, FALSE);
		monster_level = p_ptr->depth;
		object_level = p_ptr->depth + 15;
		place_object(y, x, TRUE, TRUE, FALSE);
		object_level = p_ptr->depth;
		break;
	      }
	      
	      /* A chest. */
	    case '~':
	      {
		required_tval = TV_CHEST;
		
		object_level = p_ptr->depth + 5;
		place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Treasure. */
	    case '$':
	      {
		place_gold(y, x);
		break;
	      }
	      /* Armour. */
	    case ']':
	      {
		object_level = p_ptr->depth + 3;
		
		if (randint(3) == 1) temp = randint(9);
		else temp = randint(8);
		
		if (temp == 1) required_tval = TV_BOOTS;
		else if (temp == 2) required_tval = TV_GLOVES;
		else if (temp == 3) required_tval = TV_HELM;
		else if (temp == 4) required_tval = TV_CROWN;
		else if (temp == 5) required_tval = TV_SHIELD;
		else if (temp == 6) required_tval = TV_CLOAK;
		else if (temp == 7) required_tval = TV_SOFT_ARMOR;
		else if (temp == 8) required_tval = TV_HARD_ARMOR;
		else required_tval = TV_DRAG_ARMOR;
		
		place_object(y, x, TRUE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Weapon. */
	    case '|':
	      {
		object_level = p_ptr->depth + 3;
		
		temp = randint(4);
		
		if (temp == 1) required_tval = TV_SWORD;
		else if (temp == 2) required_tval = TV_POLEARM;
		else if (temp == 3) required_tval = TV_HAFTED;
		else if (temp == 4) required_tval = TV_BOW;
		
		place_object(y, x, TRUE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Ring. */
	    case '=':
	      {
		required_tval = TV_RING;
		
		object_level = p_ptr->depth + 3;
		if (randint(4) == 1) 
		  place_object(y, x, TRUE, FALSE, TRUE);
		else place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Amulet. */
	    case '"':
	      {
		required_tval = TV_AMULET;
		
		object_level = p_ptr->depth + 3;
		if (randint(4) == 1) 
		  place_object(y, x, TRUE, FALSE, TRUE);
		else place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Potion. */
	    case '!':
	      {
		required_tval = TV_POTION;
		
		object_level = p_ptr->depth + 3;
		if (randint(4) == 1) 
		  place_object(y, x, TRUE, FALSE, TRUE);
		else place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Scroll. */
	    case '?':
	      {
		required_tval = TV_SCROLL;
		
		object_level = p_ptr->depth + 3;
		if (randint(4) == 1) 
		  place_object(y, x, TRUE, FALSE, TRUE);
		else place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Staff. */
	    case '_':
	      {
		required_tval = TV_STAFF;
		
		object_level = p_ptr->depth + 3;
		if (randint(4) == 1) 
		  place_object(y, x, TRUE, FALSE, TRUE);
		else place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Wand or rod. */
	    case '-':
	      {
		if (rand_int(100) < 50) required_tval = TV_WAND;
		else required_tval = TV_ROD;
		
		object_level = p_ptr->depth + 3;
		if (randint(4) == 1) 
		  place_object(y, x, TRUE, FALSE, TRUE);
		else place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	      /* Food or mushroom. */
	    case ',':
	      {
		required_tval = TV_FOOD;
		
		object_level = p_ptr->depth + 3;
		place_object(y, x, FALSE, FALSE, TRUE);
		object_level = p_ptr->depth;
		
		required_tval = 0;
		
		break;
	      }
	    }
	}
    }
  
  /* 
   * To avoid rebuilding the monster list too often (which can quickly 
   * get expensive), we handle monsters of a specified race separately.
   */
  for (i = 0; racial_symbol[i] != '\0'; i++)
    {
      /* Require correct race, allow uniques. */
      allow_unique = TRUE;
      sprintf(d_char_req, "%c", racial_symbol[i]);
      d_attr_req[0] = 0; 
      d_attr_req[1] = 0; 
      d_attr_req[2] = 0; 
      d_attr_req[3] = 0;
      racial_flag_mask = 0, breath_flag_mask = 0;
      
      /* Determine level of monster */
      if      (vault_type == 0)  temp = p_ptr->depth + 3;
      else if (vault_type == 7)  temp = p_ptr->depth;
      else if (vault_type == 8)  temp = p_ptr->depth + 3;
      else if (vault_type == 9)  temp = p_ptr->depth + 6;
      else if (vault_type == 12) temp = p_ptr->depth + 3;
      else if (vault_type == 13) temp = p_ptr->depth + 6;
      else if ((vault_type > 13) && (vault_type % 2))
	temp = p_ptr->depth + 4;
      else                       temp = p_ptr->depth;
      
      /* Apply our restrictions */
      get_mon_num_hook = mon_select;
      
      /* Prepare allocation table */
      get_mon_num_prep();
      
      /* Build the monster probability table. */
      if (!get_mon_num(temp)) continue;
      
      
      /* Place the monsters */
      for (t = data, y = y1; y <= y2; y++)
	{
	  for (x = x1; x <= x2; x++, t++)
	    {
	      if (*t == racial_symbol[i])
		{
		  /* Place a monster */
		  place_monster_aux(y, x, get_mon_num_quick(temp),
				    FALSE, FALSE);
		}
	    }
	}
    }
  
  /* Clear any current monster restrictions. */
  if (get_mon_num_hook)
    {
      get_mon_num_hook = NULL;
      get_mon_num_prep();
    }
  
  /* Ensure that the player is always placed in a themed level. */
  if ((p_ptr->themed_level) && (!placed))
    {
      if (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
	new_player_spot();
      else
	player_place(panic_y, panic_x);
    }
  
  /* Success. */
  return (TRUE);
}


/**
 * Type 7 -- interesting rooms. -LM-
 */
static bool build_type7(void)
{
  vault_type *v_ptr;
  int i, y, x;
  int *v_idx = malloc(z_info->v_max * sizeof(*v_idx));
  int v_cnt = 0;
  
  /* Examine each vault */
  for (i = 0; i < z_info->v_max; i++)
    {
      /* Access the vault */
      v_ptr = &v_info[i];
      
      /* Accept each interesting room that is acceptable for this depth. */
      if ((v_ptr->typ == 7) && (v_ptr->min_lev <= p_ptr->depth) && 
	  (v_ptr->max_lev >= p_ptr->depth))
	{
	  v_idx[v_cnt++] = i;
	}
    }
  
  /* Access a random vault record */
  v_ptr = &v_info[v_idx[rand_int(v_cnt)]];
  
  if (!find_space(&y, &x, v_ptr->hgt, v_ptr->wid)) 
    {
      free(v_idx);
      return (FALSE);
    }
  
  /* Boost the rating */
  rating += v_ptr->rat;
  
  
  /* Build the vault (sometimes lit, not icky, type 7) */
  if (!build_vault(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text, 
		   (p_ptr->depth < rand_int(37)), FALSE, 7)) 
    {
      free(v_idx);
      return (FALSE);
    }
  
  free(v_idx);
  
  return (TRUE);
}

/**
 * Type 8 -- lesser vaults.
 */
static bool build_type8(void)
{
  vault_type *v_ptr;
  int i, y, x;
  int *v_idx = malloc(z_info->v_max * sizeof(*v_idx));
  int v_cnt = 0;
  
  /* Examine each vault */
  for (i = 0; i < z_info->v_max; i++)
    {
      /* Access the vault */
      v_ptr = &v_info[i];
      
      /* Accept each lesser vault that is acceptable for this depth. */
      if ((v_ptr->typ == 8) && (v_ptr->min_lev <= p_ptr->depth) && 
	  (v_ptr->max_lev >= p_ptr->depth))
	{
	  v_idx[v_cnt++] = i;
	}
    }
  
  /* Access a random vault record */
  v_ptr = &v_info[v_idx[rand_int(v_cnt)]];
  
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y, &x, v_ptr->hgt, v_ptr->wid)) 
    {
      free(v_idx);
      return (FALSE);
    }
  
  
  /* Message */
  if (cheat_room) msg_print("Lesser Vault");
  
  /* Boost the rating */
  rating += v_ptr->rat;
  
  /* Build the vault (never lit, icky, type 8) */
  if (!build_vault(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text, 
		   FALSE, TRUE, 8)) 
    {
      free(v_idx);
      return (FALSE);
    }
  
  free(v_idx);
  
  return (TRUE);
}



/**
 * Type 9 -- greater vaults.
 */
static bool build_type9(void)
{
  vault_type *v_ptr;
  int i, y, x;
  int *v_idx = malloc(z_info->v_max * sizeof(int));
  int v_cnt = 0;
  
  /* Examine each vault */
  for (i = 0; i < z_info->v_max; i++)
    {
      /* Access the vault */
      v_ptr = &v_info[i];
      
      /* Accept each greater vault that is acceptable for this depth. */
      if ((v_ptr->typ == 9) && (v_ptr->min_lev <= p_ptr->depth) && 
	  (v_ptr->max_lev >= p_ptr->depth))
	{
	  v_idx[v_cnt++] = i;
	}
    }

  /* Access a random vault record */
  v_ptr = &v_info[v_idx[rand_int(v_cnt)]];
  
  
  /* Find and reserve some space in the dungeon.  Get center of room. */
  if (!find_space(&y, &x, v_ptr->hgt, v_ptr->wid)) 
    {
      free(v_idx);
      return (FALSE);
    }
  
  
  /* Message */
  if (cheat_room) msg_print("Greater Vault");
  
  /* Boost the rating */
  rating += v_ptr->rat;
  
  /* Build the vault (never lit, icky, type 9) */
  if (!build_vault(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text, 
		   FALSE, TRUE, 9)) 
    {
      free(v_idx);
      return (FALSE);
    }
  
  free(v_idx);
  
  return (TRUE);
}





/**
 * Type 10 -- Extremely large rooms.
 * 
 * These are the largest, most difficult to position, and thus highest-
 * priority rooms in the dungeon.  They should be rare, so as not to 
 * interfere with greater vaults.
 * 
 *                     (huge chamber)
 * - A single starburst-shaped room of extreme size, usually dotted or 
 * even divided with irregularly-shaped fields of rubble. No special 
 * monsters.  Appears deeper than level 40.
 *
 */
static bool build_type10(void)
{
  bool light;
  
  int i, count;
  
  int y0, x0, y1, x1, y2, x2;
  int y1_tmp, x1_tmp, y2_tmp, x2_tmp;
  int width, height;
  int width_tmp, height_tmp;
  
  
  /* Huge cave room */
  if (p_ptr->depth > 40)
    {
      /* This room is usually lit. */
      if (rand_int(3) != 0) light = TRUE;
      else light = FALSE;
      
      /* Get a size */
      height = (2 + randint(2)) * BLOCK_HGT;
      width  = (3 + randint(6)) * BLOCK_WID;
      
      /* Find and reserve some space.  Get center of room. */
      if (!find_space(&y0, &x0, height, width)) return (FALSE);
      
      /* Locate the room */
      y1 = y0 - height / 2;
      x1 = x0 - width / 2;
      y2 =  y1 + height - 1;
      x2 =  x1 + width - 1;
      
      /* Make a huge starburst room with optional light. */
      if (!generate_starburst_room(y1, x1, y2, x2, light, 
				   FEAT_FLOOR, FALSE)) return (FALSE);
      
      
      /* Often, add rubble to break things up a bit. */
      if (randint(5) > 2)
	{
	  /* Determine how many rubble fields to add (between 1 and 6). */
	  count = height * width * randint(2) / 1100;
	  
	  /* Make the rubble fields. */
	  for (i = 0; i < count; i++)
	    {
	      height_tmp = 8 + rand_int(16);
	      width_tmp = 10 + rand_int(24);
	      
	      /* Semi-random location. */
	      y1_tmp = y1 + rand_int(height - height_tmp);
	      x1_tmp = x1 + rand_int(width - width_tmp);
	      y2_tmp = y1_tmp + height_tmp;
	      x2_tmp = x1_tmp + width_tmp;
	      
	      /* Make the rubble field. */
	      generate_starburst_room(y1_tmp, x1_tmp, y2_tmp, 
				      x2_tmp, FALSE, FEAT_RUBBLE, FALSE);
	    }
	}
      
      /* Success. */
      return (TRUE);
    }
  
  
  /* No room was selected, so none was built. */
  return (FALSE);
}



/**
 * Helper function that reads the room data table and returns the number 
 * of rooms, of a given type, we should build on this level.
 */
static int num_rooms_allowed(int room_type)
{
  int allowed = 0;
  int base_num, num_tries, mod, i;
  
  /* Point to the room information. */
  room_data *rm_ptr = &room[room_type];
  
  
  /* No rooms allowed above their minimum depth. */
  if (p_ptr->depth < rm_ptr->min_level) return (0);
  
  /* No "nothing" rooms. */
  if (room_type == 0) return (0);
  
  /* No special limit on ordinary rooms. */
  if (room_type == 1) return (DUN_ROOMS);
  
  
  /* If below level 100, use the rarity value for level 100. */
  if (p_ptr->depth > 100)
    {
      base_num = rm_ptr->room_gen_num[10];
    }
  else
    {
      mod = p_ptr->depth % 10;
      
      /* If depth is divisable by 10, use the appropriate table value. */
      if (mod == 0) 
	{
	  base_num = rm_ptr->room_gen_num[p_ptr->depth / 10];
	}
      /* Otherwise, use a weighted average of the nearest values. */
      else
	{
	  base_num = ((mod * rm_ptr->room_gen_num[(p_ptr->depth + 9) / 10]) 
		      + ((10 - mod) * rm_ptr->room_gen_num[p_ptr->depth / 10]
			 )) / 10;
	}
    }
  
  /* Find out how many times we'll try to boost the room count. */
  num_tries = 3 * base_num / 100;
  if (num_tries < 2) num_tries = (base_num < 12 ? 1 : 2);
  if (num_tries > DUN_ROOMS / 2) num_tries = DUN_ROOMS / 2;
  
  /* No multiple huge rooms on any level. */
  if (room_type == 10) num_tries = 1;
  
  /* Try several times to increase the number of rooms to build. */
  for (i = 0; i < num_tries; i++)
    {
      if (rand_int(1000) < 10 * base_num / num_tries)
	{
	  allowed++;
	}
    }
  
  /* Return the number of rooms of that type we should build. */
  return (allowed);
}


/**
 * Build a room of the given type.
 * 
 * Check to see if there will probably be enough space in the monster 
 * and object arrays.
 */
static bool room_build(int room_type)
{
  /* If trying to build a special room, check some limits first. */
  if (room_type > 4)
    {
      /* Help prevent object over-flow */
      if (o_max > 3 * z_info->o_max / 4)
	{
	  return (FALSE);
	}
      
      /* Help prevent monster over-flow */
      if (m_max > 3 * z_info->m_max / 4)
	{
	  return (FALSE);
	}
    }
  
  
  /* Build a room */
  switch (room_type)
    {
      /* Find space for, position, and build the room asked for */
    case 10: if (!build_type10()) return (FALSE); break;
    case  9: if (!build_type9())  return (FALSE); break;
    case  8: if (!build_type8())  return (FALSE); break;
    case  7: if (!build_type7())  return (FALSE); break;
    case  6: if (!build_type6())  return (FALSE); break;
    case  5: if (!build_type5())  return (FALSE); break;
    case  4: if (!build_type4())  return (FALSE); break;
    case  3: if (!build_type3())  return (FALSE); break;
    case  2: if (!build_type2())  return (FALSE); break;
    case  1: if (!build_type1())  return (FALSE); break;
      
      /* Paranoia */
    default: return (FALSE);
    }
  
  /* Success */
  return (TRUE);
}



/**************************************************************/
/*                                                            */
/*                     The tunnelling code                    */
/*                                                            */
/**************************************************************/


/**
 * Given a current position (y1, x1), move towards the target grid 
 * (y2, x2) either vertically or horizontally.
 *
 * If both vertical and horizontal directions seem equally good, 
 * prefer to move horizontally.
 */
static void correct_dir(int *row_dir, int *col_dir, int y1, int x1, int y2, 
			int x2)
{
  /* Move vertically if vertical distance to target is greater. */
  if (ABS(y1 - y2) > ABS(x1 - x2))
    {
      *row_dir = ((y1 < y2) ? 1 : -1);
      *col_dir = 0;
    }
  
  /* Prefer to move horizontally. */
  else
    {
      *row_dir = 0;
      *col_dir = ((x1 < x2) ? 1 : -1);
    }
}


/**
 * Go in a semi-random direction from current location to target location.  
 * Do not actually head away from the target grid.  Always make a turn.
 */
static void adjust_dir(int *row_dir, int *col_dir, int y1, int x1, int y2, 
		       int x2)
{
  /* Always turn 90 degrees. */
  if (*row_dir == 0)
    {
      *col_dir = 0;
      
      /* On the y-axis of target - freely choose a side to turn to. */
      if (y1 == y2) *row_dir = ((rand_int(2) == 0) ? - 1 : 1);
      
      /* Never turn away from target. */
      else *row_dir = ((y1 < y2) ? 1 : -1);
    }
  else
    {
      *row_dir = 0;
      
      /* On the x-axis of target - freely choose a side to turn to. */
      if (x1 == x2) *col_dir = ((rand_int(2) == 0) ? - 1 : 1);
      
      /* Never turn away from target. */
      else *col_dir = ((x1 < x2) ? 1 : -1);
    }
}


/**
 * Go in a completely random orthongonal direction.  If we turn around 
 * 180 degrees, save the grid; it may be a good place to place stairs 
 * and/or the player.
 */
static void rand_dir(int *row_dir, int *col_dir, int y, int x)
{
  /* Pick a random direction */
  int i = rand_int(4);
  
  /* Extract the dy/dx components */
  int row_dir_tmp = ddy_ddd[i];
  int col_dir_tmp = ddx_ddd[i];
  
  /* Save useful grids. */
  if ((-(*row_dir) == row_dir_tmp) && (-(*col_dir) == col_dir_tmp))
    {
      /* Save the current tunnel location if surrounded by walls. */
      if ((in_bounds_fully(y, x)) && (dun->stair_n < STAIR_MAX) && 
	  (next_to_walls(y, x) == 4))
	{
	  dun->stair[dun->stair_n].y = y;
	  dun->stair[dun->stair_n].x = x;
	  dun->stair_n++;
	}
    }
  
  /* Save the new direction. */
  *row_dir = row_dir_tmp;
  *col_dir = col_dir_tmp;
}


/** Terrain type is unalterable and impassable. */
static bool unalterable(byte feat)
{
  /* A few features are unalterable. */
  if ((feat == FEAT_PERM_EXTRA) ||
      (feat == FEAT_PERM_INNER) ||
      (feat == FEAT_PERM_OUTER) ||
      (feat == FEAT_PERM_SOLID))
    {
      return (TRUE);
    }
  
  /* Assume alterable */
  return (FALSE);
}

/**
 * Given a set of coordinates, return the index number of the room occupying 
 * the dungeon block this location is in.
 */
static int get_room_index(int y, int x)
{
  /* Which block are we in? */
  int by = y / BLOCK_HGT;
  int bx = x / BLOCK_WID;
  
  /* Paranoia -- confirm that block is in the dungeon. */
  if ((by > MAX_ROOMS_ROW) || (by > MAX_ROOMS_ROW)) return (-1);
  
  /* Get the room index. */
  return (dun->room_map[by][bx] - 1);
}



/**
 * Search for a vault entrance.
 *
 * Notes:
 * - This function looks in both directions, and chooses the nearest 
 *   entrance (if it has a choice).
 * - We assume rooms will have outer walls surrounding them.
 * - We assume the vault designer hasn't designed false entrances, or
 *   done something else really sneaky.
 */
static bool find_entrance(int row_dir, int col_dir, int *row1, int *col1)
{
  int i, j;
  int y;
  int x;
  int dy, dx;
  
  /*
   * Initialize entrances found while looking in both directions, and 
   * the distances to them.
   */
  int target_y[2] = {0, 0};
  int target_x[2] = {0, 0};
  int grids[2]    = {0, 0};
  
  
  /* Search in both directions. */
  for (i = 0; i < 2; i++)
    {
      bool stop_loop = FALSE;
      
      y = *row1;
      x = *col1;
      
      dy = row_dir;
      dx = col_dir;
      
      /* Keep running through the steps. */
      while (TRUE)
	{
	  int dy_tmp = dy;
	  
	  /* Search grids on both sides for more impassable walls. */
	  for (j = i; j < 2 + i; j++)
	    {
	      if (dy_tmp == 0)
		{
		  dy = ((j == 1) ? - 1 : 1);
		  dx = 0;
		}
	      else
		{
		  dy = 0;
		  dx = ((j == 1) ? - 1 : 1);
		}
	      
	      /* Look in chosen direction. */
	      if ((!unalterable(cave_feat[y + dy][x + dx])) && 
		  (cave_feat[y + dy][x + dx] != FEAT_WALL_OUTER))
		{
		  /*
		   * Check the grid after this one.  If it belongs 
		   * to the same room, we've found an entrance.
		   */
		  if (get_room_index(y + dy, x + dx) == 
		      get_room_index(y + dy + dy, x + dx + dx))
		    {
		      target_y[i] = y + dy;
		      target_x[i] = x + dx;
		      break;
		    }
		}
	      
	      /* Look again. */
	      else if (unalterable(cave_feat[y + dy][x + dx]))
		{
		  break;
		}
	      
	      /* We're out on some kind of weird spur. */
	      else if (j == (1 + i))
		{
		  /* Stop travelling in this direction. */
		  stop_loop = TRUE;
		  break;
		}
	    }
	  
	  /* Success or (known) failure. */
	  if (target_y[i] && target_x[i]) break;
	  if (stop_loop) break;
	  
	  /* Keep heading in the same direction. */
	  while (TRUE)
	    {
	      /* Advance to new grid in our direction of travel. */
	      y += dy;
	      x += dx;
	      
	      /* Count the number of grids we've travelled */
	      grids[i]++;
	      
	      /*
	       * We're back where we started.  Room either has no 
	       * entrances, or we can't find them.
	       */
	      if ((y == *row1) && (x == *col1))
		{
		  stop_loop = TRUE;
		  break;
		}
	      
	      /* We have hit the dungeon edge. */
	      if (!in_bounds_fully(y + dy, x + dx))
		{
		  stop_loop = TRUE;
		  break;
		}
	      
	      /* Next grid is outer wall. */
	      if (cave_feat[y + dy][x + dx] == FEAT_WALL_OUTER)
		{
		  /* We need to make another turn. */
		  break;
		}
	      
	      /* Next grid is alterable, and not outer wall. */
	      else if (!unalterable(cave_feat[y + dy][x + dx]))
		{
		  /*
		   * Check the grid after this one.  If it belongs 
		   * to the same room, we've found an entrance.
		   */
		  if (get_room_index(y + dy, x + dx) == 
		      get_room_index(y + dy + dy, x + dx + dx))
		    {
		      target_y[i] = y + dy;
		      target_x[i] = x + dx;
		      break;
		    }
		  
		  /*
		   * If we're in the same room, our likely best move 
		   * is to keep moving along the permanent walls.
		   */
		  else
		    {
		      break;
		    }
		}
	    }
	  
	  /* Success. */
	  if (target_y[i] && target_x[i]) break;
	  
	  /* Failure. */
	  if (stop_loop) break;
	}
    }
  
  /*
   * Compare reports.  Pick the only target available, or choose 
   * the target that took less travelling to get to.
   */
  if ((target_y[0] && target_x[0]) && (target_y[1] && target_x[1]))
    {
      if (grids[0] < grids[1])
	{
	  *row1 = target_y[0];
	  *col1 = target_x[0];
	}
      else
	{
	  *row1 = target_y[1];
	  *col1 = target_x[1];
	}
      
      return (TRUE);
    }
  
  else if (target_y[0] && target_x[0])
    {
      *row1 = target_y[0];
      *col1 = target_x[0];
      return (TRUE);
    }
  else if (target_y[1] && target_x[1])
    {
      *row1 = target_y[1];
      *col1 = target_x[1];
      return (TRUE);
    }
  
  /* No entrances found. */
  else return (FALSE);
}

/**
 * Tests suitability of potential entranceways, and places doors if 
 * appropriate.
 */
static void try_entrance(int y0, int x0)
{
  int i, k;
  
  /* Require walls on at least two sides. */
  for (k = 0, i = 0; i < 4; i++)
    {
      /* Extract the location */
      int y = y0 + ddy_ddd[i];
      int x = x0 + ddx_ddd[i];
      
      /* Ignore non-walls. */
      if ((cave_feat[y][x] < FEAT_MAGMA) || 
	  (cave_feat[y][x] > FEAT_MTRAP_HEAD)) continue;
      
      /* We require at least two walls. */
      if ((k++) == 2) place_random_door(y0, x0);
    }
}

/**
 * Places door at y, x position if at least 2 walls and two corridor spaces 
 * found
 */
static void try_door(int y0, int x0)
{
  int i, y, x;
  int k = 0;
  
  
  /* Ignore walls */
  if (cave_info[y0][x0] & (CAVE_WALL)) return;
  
  /* Ignore room grids */
  if (cave_info[y0][x0] & (CAVE_ROOM)) return;
  
  /* Occasional door (if allowed) */
  if (rand_int(100) < DUN_TUN_JCT)
    {
      /* Count the adjacent non-wall grids */
      for (i = 0; i < 4; i++)
	{
	  /* Extract the location */
	  y = y0 + ddy_ddd[i];
	  x = x0 + ddx_ddd[i];
	  
	  /* Skip impassable grids (or trees) */
	  if (cave_info[y][x] & (CAVE_WALL)) continue;
	  
	  /* Skip grids inside rooms */
	  if (cave_info[y][x] & (CAVE_ROOM)) continue;
	  
	  /* We require at least two walls outside of rooms. */
	  if ((k++) == 2) break;
	}
      
      if (k == 2)
	{
	  /* Check Vertical */
	  if ((cave_feat[y0-1][x0] >= FEAT_MAGMA) &&
	      (cave_feat[y0-1][x0] <= FEAT_SHOP_HEAD) &&
	      (cave_feat[y0+1][x0] >= FEAT_MAGMA) &&
	      (cave_feat[y0+1][x0] <= FEAT_SHOP_HEAD))
	    {
	      place_random_door(y0, x0);
	    }
	  
	  /* Check Horizontal */
	  else if ((cave_feat[y0][x0-1] >= FEAT_MAGMA) &&
		   (cave_feat[y0][x0-1] <= FEAT_SHOP_HEAD) &&
		   (cave_feat[y0][x0+1] >= FEAT_MAGMA) &&
		   (cave_feat[y0][x0+1] <= FEAT_SHOP_HEAD))
	    {
	      place_random_door(y0, x0);
	    }
	}
    }
}




/**
 * Constructs a tunnel between two points. 
 *
 * The tunnelling code connects room centers together.  It is the respon-
 * sibility of rooms to ensure all grids in them are accessable from the 
 * center, or from a passable grid nearby if the center is a wall.
 *
 * (warnings)
 * This code is still beta-quality.  Use with care.  Known areas of 
 * weakness include: 
 * - A group of rooms may be connected to each other, and not to the rest 
 *   of the dungeon.  This problem is rare.
 * - While the entrance-finding code is very useful, sometimes the tunnel 
 *   gets lost on the way.
 * - On occasion, a tunnel will travel far too long.  It can even (rarely) 
 *   happen that it would lock up the game if not artifically stopped.
 * - There are number of minor but annoying problems, both old and new, 
 *   like excessive usage of tunnel grids, tunnels turning areas of the
 *   dungeon into Swiss cheese, and so on.
 * - This code is awfully, awfully long.
 *
 * (Handling the outer walls of rooms)
 * In order to place doors correctly, know when a room is connected, and 
 * keep entances and exits to rooms neat, we set and use several different 
 * kinds of granite.  Because of this, we must call this function before 
 * making streamers.
 * - "Outer" walls must surround rooms.  The code can handle outer walls 
 * up to two grids thick (which is common in non-rectangular rooms).
 * - When outer wall is pierced, "solid" walls are created along the axis 
 * perpendicular to the direction of movement for three grids in each 
 * direction.  This makes entrances tidy.
 * 
 * (Handling difficult terrain)
 * When an unalterable (permanent) wall is encountered, this code is 
 * capable of finding entrances and of using waypoints.  It is anticipated 
 * that this will make vaults behave better than they did.
 *
 * Useful terrain values:
 *   FEAT_WALL_EXTRA -- granite walls
 *   FEAT_WALL_INNER -- inner room walls
 *   FEAT_WALL_OUTER -- outer room walls
 *   FEAT_WALL_SOLID -- solid room walls
 *   FEAT_PERM_INNER -- inner room walls (perma)
 *   FEAT_PERM_OUTER -- outer room walls (perma)
 *   FEAT_PERM_SOLID -- dungeon border (perma)
 */
void build_tunnel(int start_room, int end_room)
{
  int i = 0, j = 0, tmp, y, x;
  int y0, x0, y1, x1;
  int dy, dx;
  
  int row_dir, col_dir;
  
  
  /* Get start and target grids. */
  int row1 = dun->cent[start_room].y;
  int col1 = dun->cent[start_room].x;
  int row2 = dun->cent[end_room].y;
  int col2 = dun->cent[end_room].x;
  int tmp_row = row1, tmp_col = col1;
  
  
  /* Store initial target, because we may have to use waypoints. */
  int initial_row2 = row2;
  int initial_col2 = col2;
  
  /* Not yet worried about our progress */
  int desperation = 0;
  
  /* Start out not allowing the placement of doors */
  bool door_flag = FALSE;
  
  /* Don't leave just yet */
  bool leave = FALSE;
  
  /* Not heading for a known entrance. */
  bool head_for_entrance = FALSE;
  
  /* Initialize some movement counters */
  int adjust_dir_timer = randint(DUN_TUN_ADJ * 2);
  int rand_dir_timer   = randint(DUN_TUN_RND * 2);
  int correct_dir_timer = 0;
  
  
  /* Set number of tunnel grids and room entrances to zero. */
  dun->tunn_n = 0;
  dun->wall_n = 0;
  
  /* Start out heading in the correct direction */
  correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
  
  /* Keep going until done (or look like we're getting nowhere). */
  while ((row1 != initial_row2) || (col1 != initial_col2))
    {
      /* Stop when tunnel is too long, or we want to stop. */
      if ((leave) || (dun->tunn_n == TUNN_MAX) || (j++ == 400)) break;
      
      /* 
       * If we've reached a waypoint, the source and destination rooms 
       * should be connected to each other now, but they may not be to 
       * the rest of the network.  Get another room center at random, 
       * and move towards it.
       */
      if ((row1 == row2) && (col1 == col2))
	{
	  while (TRUE)
	    {
	      i = rand_int(dun->cent_n);
	      if ((i != start_room) && (i != end_room)) break;
	    }
	  
	  row2 = initial_row2 = dun->cent[i].y;
	  col2 = initial_col2 = dun->cent[i].x;
	  
	  head_for_entrance = FALSE;
	}
      
      /* Try moving randomly if we seem stuck. */
      else if ((row1 != tmp_row) && (col1 != tmp_col))
	{
	  desperation++;
	  
	  /* Try a 90 degree turn. */
	  if (desperation == 1)
	    {
	      adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
	      adjust_dir_timer = 3;
	    }
	  
	  /* Try turning randomly. */
	  else if (desperation < 4)
	    {
	      rand_dir(&row_dir, &col_dir, row1, col1);
	      correct_dir_timer = 2;
	    }
	  else
	    {
	      /* We've run out of ideas.  Stop wasting time. */
	      break;
	    }
	}
      
      /* We're making progress. */
      else
	{
	  /* No worries. */
	  desperation = 0;
	  
	  /* Check room. */
	  tmp = get_room_index(row1, col1);
	  
	  /* We're in our destination room - head straight for target. */
	  if ((tmp == end_room) && (cave_info[row1][col1] & (CAVE_ROOM)))
	    {
	      correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
	    }
	  
	  else
	    {
	      /* Count down times until next movement changes. */
	      if (adjust_dir_timer > 0) adjust_dir_timer--;
	      if (rand_dir_timer > 0) rand_dir_timer--;
	      if (correct_dir_timer > 0) correct_dir_timer--;
	      
	      /* Make a random turn, set timer. */
	      if (rand_dir_timer == 0)
		{
		  rand_dir(&row_dir, &col_dir, row1, col1);
		  
		  rand_dir_timer = randint(DUN_TUN_RND * 2);
		  correct_dir_timer = randint(4);
		}
	      
	      /* Adjust direction, set timer. */
	      else if (adjust_dir_timer == 0)
		{
		  adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
		  
		  adjust_dir_timer = randint(DUN_TUN_ADJ * 2);
		}
	      
	      
	      /* Go in correct direction. */
	      else if (correct_dir_timer == 0)
		{
		  correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
		  
		  /* Don't use again unless needed. */
		  correct_dir_timer = -1;
		}
	    }
	}
      
      
      /* Get the next location */
      tmp_row = row1 + row_dir;
      tmp_col = col1 + col_dir;
      
      /* Do not leave the dungeon */
      if (!in_bounds_fully(tmp_row, tmp_col))
	{
	  /* Adjust direction */
	  adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
	  
	  /* Get the next location */
	  tmp_row = row1 + row_dir;
	  tmp_col = col1 + col_dir;
	  
	  /* Our destination is illegal - stop. */
	  if (!in_bounds_fully(tmp_row, tmp_col)) break;
	}
      
      /* Tunnel through dungeon granite. */
      if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_EXTRA)
	{
	  /* Accept this location */
	  row1 = tmp_row;
	  col1 = tmp_col;
	  
	  /* Save the current tunnel location */
	  if (dun->tunn_n < TUNN_MAX)
	    {
	      dun->tunn[dun->tunn_n].y = row1;
	      dun->tunn[dun->tunn_n].x = col1;
	      dun->tunn_n++;
	    }
	  
	  /* Allow door in next grid */
	  door_flag = TRUE;
	  
	  continue;
	}
      
      
      /* Pierce outer walls of rooms. */
      else if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_OUTER)
	{
	  /* Look ahead */
	  y0 = tmp_row + row_dir;
	  x0 = tmp_col + col_dir;

	  /* No annoying little alcoves near edge. */
	  if (!in_bounds_fully(y0, x0)) continue;
	  
	  
	  /* Disallow door in next grid */
	  door_flag = FALSE;
	  
	  /* Hack -- delay turns */
	  adjust_dir_timer++;  rand_dir_timer++;
	  
	  /* Navigate around various kinds of walls */
	  if ((cave_feat[y0][x0] == FEAT_WALL_SOLID) || 
	      (cave_feat[y0][x0] == FEAT_WALL_INNER) || 
	      (unalterable(cave_feat[y0][x0])))
	    {
	      for (i = 0; i < 2; i++)
		{
		  if (i == 0)
		    {
		      /* Check the more direct route first. */
		      adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
		      
		      /* Verify that we haven't just been here. */
		      if ((dun->tunn_n == 0) || 
			  !(dun->tunn[dun->tunn_n - 1].y == row1 + row_dir) || 
			  !(dun->tunn[dun->tunn_n - 1].x == col1 + col_dir))
			{
			  tmp_row = row1 + row_dir;
			  tmp_col = col1 + col_dir;
			}
		      
		      else continue;
		    }
		  
		  else
		    {
		      /* If that didn't work, try the other side. */
		      tmp_row = row1 - row_dir;
		      tmp_col = col1 - col_dir;
		    }
		  
		  if ((!unalterable(cave_feat[tmp_row][tmp_col])) && 
		      (cave_feat[tmp_row][tmp_col] != FEAT_WALL_SOLID) && 
		      (cave_feat[tmp_row][tmp_col] != FEAT_WALL_OUTER) && 
		      (cave_feat[tmp_row][tmp_col] != FEAT_WALL_INNER))
		    {
		      /* Accept the location */
		      row1 = tmp_row;
		      col1 = tmp_col;
		      
		      /* Save the current tunnel location */
		      if (dun->tunn_n < TUNN_MAX)
			{
			  dun->tunn[dun->tunn_n].y = row1;
			  dun->tunn[dun->tunn_n].x = col1;
			  dun->tunn_n++;
			}
		      
		      /* Continue */
		      break;
		    }
		  
		  /* No luck. */
		  if (i == 1) continue;
		}
	    }
	  
	  /* Handle a double line of outer walls robustly. */
	  else if (cave_feat[y0][x0] == FEAT_WALL_OUTER)
	    {
	      /* Look ahead (again). */
	      y1 = y0 + row_dir;
	      x1 = x0 + col_dir;
	      
	      /* We've found something passable. */
	      if (passable(cave_feat[y1][x1]))
		{
		  /* Turn both outer wall grids into floor. */
		  cave_set_feat(tmp_row, tmp_col, FEAT_FLOOR);
		  cave_set_feat(y0, x0, FEAT_FLOOR);
		  
		  /* Save the wall location */
		  if (dun->wall_n < WALL_MAX)
		    {
		      dun->wall[dun->wall_n].y = tmp_row;
		      dun->wall[dun->wall_n].x = tmp_col;
		      dun->wall_n++;
		    }
		  
		  /* Accept this location */
		  row1 = tmp_row = y0;
		  col1 = tmp_col = x0;
		}
	      
	      /* No luck - look at the sides. */
	      else
		{
		  for (i = 0; i < 2; i++)
		    {
		      if (i == 0)
			{
			  /* Check the more direct route first. */
			  adjust_dir(&row_dir, &col_dir, row1, col1, 
				     row2, col2);
			  
			  tmp_row = row1 + row_dir;
			  tmp_col = col1 + col_dir;
			}
		      else
			{
			  /* If that didn't work, try the other side. */
			  tmp_row = row1 - row_dir;
			  tmp_col = col1 - col_dir;
			}
		      
		      if ((!unalterable(cave_feat[tmp_row][tmp_col])) && 
			  (cave_feat[tmp_row][tmp_col] != FEAT_WALL_SOLID) && 
			  (cave_feat[tmp_row][tmp_col] != FEAT_WALL_OUTER) && 
			  (cave_feat[tmp_row][tmp_col] != FEAT_WALL_INNER))
			{
			  /* Accept the location */
			  row1 = tmp_row;
			  col1 = tmp_col;
			  
			  /* Save the current tunnel location */
			  if (dun->tunn_n < TUNN_MAX)
			    {
			      dun->tunn[dun->tunn_n].y = row1;
			      dun->tunn[dun->tunn_n].x = col1;
			      dun->tunn_n++;
			    }
			  
			  /* Continue */
			  break;
			}
		    }
		}
	    }
	  
	  /* Second grid contains any other kind of terrain. */
	  else
	    {
	      /* Accept this location */
	      row1 = tmp_row;
	      col1 = tmp_col;
	      
	      /* Convert to floor grid */
	      cave_set_feat(row1, col1, FEAT_FLOOR);
	      
	      /* Save the wall location */
	      if (dun->wall_n < WALL_MAX)
		{
		  dun->wall[dun->wall_n].y = row1;
		  dun->wall[dun->wall_n].x = col1;
		  dun->wall_n++;
		}
	    }
	  
	  /* Forbid re-entry near this piercing. */
	  if ((!unalterable(cave_feat[row1 + row_dir][col1 + col_dir])) && 
	      (cave_info[row1][col1] & (CAVE_ROOM)))
	    {
	      if (row_dir)
		{
		  for (x = col1 - 3; x <= col1 + 3; x++)
		    {
		      /* Convert adjacent "outer" walls */
		      if ((in_bounds(row1, x)) && 
			  (cave_feat[row1][x] == FEAT_WALL_OUTER))
			{
			  /* Change the wall to a "solid" wall */
			  cave_set_feat(row1, x, FEAT_WALL_SOLID);
			}
		    }
		}
	      else
		{
		  for (y = row1 - 3; y <= row1 + 3; y++)
		    {
		      /* Convert adjacent "outer" walls */
		      if ((in_bounds(y, col1)) && 
			  (cave_feat[y][col1] == FEAT_WALL_OUTER))
			{
			  /* Change the wall to a "solid" wall */
			  cave_set_feat(y, col1, FEAT_WALL_SOLID);
			}
		    }
		}
	      
	      /* Get current room. */
	      tmp = get_room_index(row1, col1);
	      
	      /* Record our success. */
	      if ((tmp != start_room) && (tmp != -1))
		{
		  /* If this room is connected, now our start room is too. */
		  if (dun->connected[tmp])
		    {
		      dun->connected[start_room] = TRUE;
		      
		      /* If our destination room is connected, we're done. */
		      if (dun->connected[end_room]) leave = TRUE;
		    }
		  
		  /* If our start room was connected, this one is too. */
		  else if (dun->connected[start_room]) 
		    dun->connected[tmp] = TRUE;
		}
	      
	      continue;
	    }
	}
      
      
      /*
       * We've hit a feature that can't be altered.
       */
      else if (unalterable(cave_feat[tmp_row][tmp_col]))
	{
	  /* We don't know what to do. */
	  if (!head_for_entrance)
	    {
	      /* Get the room that occupies this block. */
	      tmp = get_room_index(tmp_row, tmp_col);
	      
	      /* We're in our starting room. */
	      if (tmp == start_room)
		{
		  /* Look at next grid. */
		  y = tmp_row + row_dir;
		  x = tmp_col + col_dir;
		  
		  /* If the next grid is outer wall, we know we need 
		   * to find an entrance.  Otherwise, travel through 
		   * the wall.
		   */
		  if (cave_feat[y][x] != FEAT_WALL_OUTER)
		    {
		      row1 = tmp_row;
		      col1 = tmp_col;
		      continue;
		    }
		}
	      
	      y = tmp_row;
	      x = tmp_col;
	      
	      /* We need to find an entrance to this room. */
	      if (!find_entrance(row_dir, col_dir, &y, &x))
		{
		  /* No entrance means insoluable trouble. */
		  leave = TRUE;
		  continue;
		}
	      
	      /* We're in our starting room. */
	      if (tmp == start_room)
		{
		  /* Jump immediately to entrance. */
		  row1 = tmp_row = y;
		  col1 = tmp_col = x;
		  
		  /* Look for outer wall to head for. */
		  for (i = 0; i < 4; i++)
		    {
		      y = row1 + ddy_ddd[i];
		      x = col1 + ddx_ddd[i];
		      
		      if (cave_feat[y][x] == FEAT_WALL_OUTER)
			{
			  /* Aim for outer wall. */
			  row_dir = ddy_ddd[i];
			  col_dir = ddx_ddd[i];
			  
			  adjust_dir_timer = 2;
			}
		    }
		}
	      
	      /* We're anywhere else. */
	      else
		{
		  /* Aim for given waypoint. */
		  row2 = y;
		  col2 = x;
		  
		  /* Reset the final target. */
		  initial_row2 = y;
		  initial_col2 = x;
		  
		  /* Enter "head for entrance" mode. */
		  head_for_entrance = TRUE;
		}
	    }
	  
	  /* We're heading for an entrance to a vault. */
	  if (head_for_entrance)
	    {
	      /* Check both sides. */
	      for (i = 0; i < 2; i++)
		{
		  /*
		   * Try going in the direction that best approches 
		   * the target first.  On the 2nd try, check the 
		   * opposite side.
		   */
		  if (col_dir == 0)
		    {
		      dy = 0;
		      if (i == 0) dx = ((col1 < col2) ?  1 : -1);
		      else        dx = ((col1 < col2) ? -1 :  1);
		    }
		  else
		    {
		      dx = 0;
		      if (i == 0) dy = ((row1 < row2) ?  1 : -1);
		      else        dy = ((row1 < row2) ? -1 :  1);
		    }
		  
		  /* Do not accept floor unless necessary. */
		  /* if ((cave_feat[row1 + dy][col1 + dx] == FEAT_FLOOR)
		     && (i == 0)) continue; */
		  
		  
		  /* Check to see if grid to this side is alterable. */
		  if (!unalterable(cave_feat[row1 + dy][col1 + dx]))
		    {
		      /* Change direction. */
		      row_dir = dy;
		      col_dir = dx;
		      
		      /* Accept this location */
		      row1 += row_dir;
		      col1 += col_dir;
		      
		      /* Clear previous contents, add a floor */
		      cave_set_feat(row1, col1, FEAT_FLOOR);
		      
		      /* Return to main loop. */
		      break;
		    }
		  
		  /* We seem to be in trouble. */
		  else if (i == 1)
		    {
		      /* If we previously found floor, accept the floor. */
		      if (cave_feat[row1 -(dy)][col1 -(dx)] == FEAT_FLOOR)
			{
			  /* Change direction. */
			  row_dir = -(dy);
			  col_dir = -(dx);
			  
			  /* Accept this location */
			  row1 += row_dir;
			  col1 += col_dir;
			  
			  break;
			}
		      
		      /* Otherwise, go backwards. */
		      {
			/* Change direction. */
			row_dir = -(row_dir);
			col_dir = -(col_dir);
			
			/* Accept this location */
			row1 += row_dir;
			col1 += col_dir;
			
			break;
		      }
		    }
		}
	    }
	}
      
      /* We've hit a solid wall. */
      else if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_SOLID)
	{
	  /* check both sides, most direct route first. */
	  for (i = 0; i < 2; i++)
	    {
	      if (i == 0)
		{
		  /* Check the more direct route first. */
		  adjust_dir(&row_dir, &col_dir, row1, col1, row2, col2);
		  
		  tmp_row = row1 + row_dir;
		  tmp_col = col1 + col_dir;
		}
	      else
		{
		  /* If that didn't work, try the other side. */
		  tmp_row = row1 - row_dir;
		  tmp_col = col1 - col_dir;
		}
	      
	      if ((!unalterable(cave_feat[tmp_row][tmp_col])) && 
		  (cave_feat[tmp_row][tmp_col] != FEAT_WALL_SOLID))
		{
		  /* Accept the location */
		  row1 = tmp_row;
		  col1 = tmp_col;
		  
		  /* Save the current tunnel location */
		  if (dun->tunn_n < TUNN_MAX)
		    {
		      dun->tunn[dun->tunn_n].y = row1;
		      dun->tunn[dun->tunn_n].x = col1;
		      dun->tunn_n++;
		    }
		  
		  /* Move on. */
		  i = 2;
		}
	    }
	  
	  continue;
	}
      
      /* Travel quickly through rooms. */
      else if (cave_info[tmp_row][tmp_col] & (CAVE_ROOM))
	{
	  /* Accept the location */
	  row1 = tmp_row;
	  col1 = tmp_col;
	  
	  continue;
	}
      
      /*
       * Handle all passable terrain outside of rooms (this is 
       * usually another corridor).
       */
      else if (passable(cave_feat[tmp_row][tmp_col]))
	{
	  /* We've hit another tunnel. */
	  if (cave_feat[tmp_row][tmp_col] == FEAT_FLOOR)
	    {
	      /* Collect legal door locations */
	      if (door_flag)
		{
		  /* Save the door location */
		  if (dun->door_n < DOOR_MAX)
		    {
		      dun->door[dun->door_n].y = tmp_row;
		      dun->door[dun->door_n].x = tmp_col;
		      dun->door_n++;
		    }
		  
		  /* No door in next grid */
		  door_flag = FALSE;
		}
	      
	      /* Mark start room connected. */
	      dun->connected[start_room] = TRUE;
	      
	      /* 
	       * If our destination room isn't connected, jump to 
	       * its center, and head towards the start room.
	       */
	      if (dun->connected[end_room] == FALSE)
		{
		  /* Swap rooms. */
		  tmp = end_room;
		  end_room = start_room;
		  start_room = tmp;
		  
		  /* Re-initialize */
		  row1 = dun->cent[start_room].y;
		  col1 = dun->cent[start_room].x;
		  row2 = dun->cent[end_room].y;
		  col2 = dun->cent[end_room].x;
		  initial_row2 = row2;
		  initial_col2 = col2;
		  tmp_row = row1, tmp_col = col1;
		}
	      else
		{
		  /* All done. */
		  leave = TRUE;
		}
	      
	      continue;
	    }
	  
	  /* Grid is not another tunnel.  Advance, make no changes. */
	  row1 = tmp_row;
	  col1 = tmp_col;
	  
	  continue;
	}
    }
  
  /* Turn the tunnel into corridor */
  for (i = 0; i < dun->tunn_n; i++)
    {
      /* Access the grid */
      y = dun->tunn[i].y;
      x = dun->tunn[i].x;
      
      /* Clear previous contents, add a floor */
      cave_set_feat(y, x, FEAT_FLOOR);
    }
  
  /* Make doors in entranceways. */
  for (i = 0; i < dun->wall_n; i++)
    {
      /* Access the grid */
      y = dun->wall[i].y;
      x = dun->wall[i].x;
      
      /* Sometimes, make a door in the entranceway */
      if (rand_int(100) < DUN_TUN_PEN) try_entrance(y, x);
    }
  
  
  /* We've reached the target.  If one room was connected, now both are. */
  if ((row1 == initial_row2) && (col1 == initial_col2))
    {
      if (dun->connected[start_room])
	dun->connected[end_room] = TRUE;
      else if (dun->connected[end_room])
	dun->connected[start_room] = TRUE;
    }
}


/**
 * Creation of themed levels.  Use a set of flags to ensure that no level 
 * is built more than once.  Store the current themed level number for later 
 * reference.  -LM-
 *
 * Checking for appropriateness of a themed level to a given stage is now
 * (FAangband 0.4.0) handled by a separate function. -NRM-
 *
 * see "lib/edit/t_info.txt".  
 */
bool themed_level_ok(byte choice)
{
  /* Already appeared */
  if (p_ptr->themed_level_appeared & (1L << (choice - 1))) 
    return (FALSE); 

  /* Check the location */
  switch (choice)
    {
    case THEME_ELEMENTAL:
      {
	if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) &&
	    (p_ptr->depth >= 35) && (p_ptr->depth <= 70))
	  break;
	return (FALSE);
      }
    case THEME_DRAGON:
      {
	if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) &&
	    (p_ptr->depth >= 40) && (p_ptr->depth <= 80))
	  break;
	return (FALSE);
      }
    case THEME_WILDERNESS:
      {
	if (stage_map[p_ptr->stage][STAGE_TYPE] == DESERT)
	  break;
	return (FALSE);
      }
    case THEME_DEMON:
      {
	if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) &&
	    (p_ptr->depth >= 60) && (p_ptr->depth <= 255))
	  break;
	return (FALSE);
      }
    case THEME_MINE:
      {
	if ((stage_map[p_ptr->stage][STAGE_TYPE] == CAVE) &&
	    (p_ptr->depth >= 20) && (p_ptr->depth <= 45))
	  break;
	return (FALSE);
      }
    case THEME_WARLORDS:
      {
	if (((stage_map[p_ptr->stage][STAGE_TYPE] == FOREST) ||
	     (stage_map[p_ptr->stage][STAGE_TYPE] == PLAIN)) &&
	    (p_ptr->depth >= 20))
	  break;
	return (FALSE);
      }
    case THEME_AELUIN:
      {
	if (stage_map[p_ptr->stage][LOCALITY] == DORTHONION) 
	  break;
	return (FALSE);
      }
    case THEME_ESTOLAD:
      {
	if ((stage_map[p_ptr->stage][LOCALITY] == EAST_BELERIAND) &&
	    (p_ptr->depth >= 10))
	  break;
	return (FALSE);
      }
    case THEME_SLAIN:
      {
	if (stage_map[p_ptr->stage][LOCALITY] == ANFAUGLITH) 
	  break;
	return (FALSE);
      }
    default: return (FALSE);
    }
  
  /* Must be OK */
  return (TRUE);
}



static bool build_themed_level(void)
{
  byte i, choice;
  
  vault_type *t_ptr;
  
  /*
   * Down stairs aren't allowed on quest levels, and we must give
   * a chance for the quest monsters to appear.
   */
  if (is_quest(p_ptr->stage) || no_vault()) return (FALSE);
  
  /* Pick a themed level at random.  Our patience gives out after 40 tries. */
  for(i = 0; i < 40; i++)
    {
      /* Select a random themed level record. */
      choice = randint(THEME_MAX);
      
      /* Accept the first themed level, among those not already generated,
       * appropriate to be generated at this depth.
       */
      if (themed_level_ok(choice)) 
	break;      
      
      /* Admit failure. */
      if (i == 39) return (FALSE);
    }
  
  /*
   * Initialize the info arrays for the chosen themed level only.  Do 
   * not build the themed level if forbidden, or if an error occurs.
   */
  if (init_t_info(choice)) return (FALSE);
  
  /* Access the chosen themed level */
  t_ptr = &t_info[choice];
  
  
  /* Give the player something to read. */
  msg_print("Please wait.  This may take a little while.");
  
  /* Display the message */
  if (!fresh_after) Term_fresh();
  
  
  /* Indicate that the player is on the selected themed level. */
  p_ptr->themed_level = choice;
  
  /* Build the themed level. */
  if (!build_vault(0, 0, 66, 198, t_text + t_ptr->text, FALSE, FALSE, 0))
    {
      /* Oops.  We're /not/ on a themed level. */
      p_ptr->themed_level = 0;
      
      return (FALSE);
    }
  
  /* Indicate that this theme is built, and should not appear again. */
  p_ptr->themed_level_appeared |= (1L << (choice - 1));
  
  /* Now have a feeling */
  do_feeling = TRUE;
          
  /* Update the level indicator */
  p_ptr->redraw |= (PR_DEPTH);
    
  /* Kill the themed level arrays */
  kill_t_info();
  
  /* Success. */
  return (TRUE);
}


/**
 * Generate a new dungeon level.  Determine if the level is destroyed, 
 * empty, or themed.  If themed, create a themed level.  Otherwise, build 
 * up to DUN_ROOMS rooms, type by type, in descending order of size and 
 * difficulty.
 *
 * Build the dungeon borders, scramble and connect the rooms.  Place stairs, 
 * doors, and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "icky" to indicate the presence of a vault.
 * We mark grids "temp" to prevent random monsters being placed there.
 *
 * Note that "dun_body" adds about 1100 bytes of memory to the stack.
 */
static void cave_gen(void)
{
  int i, j, k, y, x, y1, x1;
  int by, bx;
  int num_to_build;
  int room_type;
  int rooms_built = 0;
  
  /* Build rooms in descending order of difficulty. */
  byte room_build_order[ROOM_MAX] = {10, 9, 6, 8, 5, 7, 4, 3, 2, 1, 0};
  
  bool destroyed = FALSE;
  bool dummy;
  
  dun_data dun_body;
  
  /* Global data */
  dun = &dun_body;
  
  moria_level = FALSE;
  underworld = FALSE;
  
  /* Teleport level from wilderness */
  if (stage_map[p_ptr->stage][LOCALITY] == UNDERWORLD) underworld = TRUE;

  /* It is possible for levels to be moria-style. */
  if (((p_ptr->depth >= 10) && (p_ptr->depth < 40) && 
       (rand_int(MORIA_LEVEL_CHANCE) == 0)) || (underworld))
    {
      moria_level = TRUE;
      if (cheat_room) msg_print("Moria level");
      
      /* Moria levels do not have certain kinds of rooms. */
      for (i = 0; i < ROOM_MAX; i++) 
	{
	  if ((room_build_order[i] >= 2) && (room_build_order[i] <= 4))
	    room_build_order[i] = 0;
	}
    }
  
  /* It is possible for levels to be destroyed */
  if ((p_ptr->depth > 10) && (!is_quest(p_ptr->stage)) && 
      (rand_int(DEST_LEVEL_CHANCE) == 0))
    {
      destroyed = TRUE;
      
      /* Destroyed levels do not have certain kinds of rooms. */
      for (i = 0; i < ROOM_MAX; i++) 
	{
	  if (room_build_order[i] > 5) room_build_order[i] = 0;
	}
    }
  
  
  /* Hack -- Start with basic granite (or floor, if empty) */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{

/* Empty levels are useful for testing rooms. */
#if 0
	  /* Create bare floors */
	  cave_info[y][x] &= ~(CAVE_WALL);
	  cave_feat[y][x] = FEAT_FLOOR;
	  
	  break;
	  
#endif /* End of empty level testing code */
	  
	  /* Create granite wall */
	  cave_info[y][x] |= (CAVE_WALL);
	  cave_feat[y][x] = FEAT_WALL_EXTRA;
	}
    }
  
  /* Actual maximum number of rooms on this level */
  dun->row_rooms = DUNGEON_HGT / BLOCK_HGT;
  dun->col_rooms = DUNGEON_WID / BLOCK_WID;
  
  /* No stair locations yet */
  dun->stair_n = 0;
  
  /* Initialize the room table */
  for (by = 0; by < dun->row_rooms; by++)
    {
      for (bx = 0; bx < dun->col_rooms; bx++)
	{
	  dun->room_map[by][bx] = 0;
	}
    }
  
  /* No rooms are connected yet */
  for (i = 0; i < CENT_MAX; i++)
    {
      dun->connected[i] = FALSE;
    }
  
  /* No rooms yet */
  dun->cent_n = 0;
  
  
  /* 
   * Build each type of room in turn until we cannot build any more.
   */
  for (i = 0; i < ROOM_MAX; i++)
    {
      /* What type of room are we building now? */
      room_type = room_build_order[i];
      
      /* Find out how many rooms of this type we can build. */
      num_to_build = num_rooms_allowed(room_type);
      
      /* No vaults on Quest levels (for now) -BR- */
      if (is_quest(p_ptr->stage) && ((room_type == 8) || (room_type ==9))) 
	num_to_build = 0;
      
      /* Try to build all we are allowed. */
      for (j = 0; j < num_to_build; j++)
	{
	  /* Stop building rooms when we hit the maximum. */
	  if (rooms_built >= DUN_ROOMS) break;
	  
	  /* Build the room. */
	  if (room_build(room_type))
	    {
	      /* Increase the room built count. */
	      if (room_type == 10) rooms_built += 5;
	      else if ((room_type == 6) || (room_type == 9)) 
		rooms_built += 3;
	      else if (room_type == 8) rooms_built += 2;
	      else rooms_built++;
	    }
	  
	  /* Go to next type of room on failure. */
	  else break;
	}
    }
  
  /* Special boundary walls -- Top */
  for (x = 0; x < DUNGEON_WID; x++)
    {
      y = 0;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
    }
  
  /* Special boundary walls -- Bottom */
  for (x = 0; x < DUNGEON_WID; x++)
    {
      y = DUNGEON_HGT - 1;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
    }
  
  /* Special boundary walls -- Left */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      x = 0;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
    }
  
  /* Special boundary walls -- Right */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      x = DUNGEON_WID - 1;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
    }
  
  /* Hack -- Scramble the room order */
  for (i = 0; i < dun->cent_n; i++)
    {
      int pick1 = i;
      int pick2 = rand_int(dun->cent_n);
      y1 = dun->cent[pick1].y;
      x1 = dun->cent[pick1].x;
      dun->cent[pick1].y = dun->cent[pick2].y;
      dun->cent[pick1].x = dun->cent[pick2].x;
      dun->cent[pick2].y = y1;
      dun->cent[pick2].x = x1;
      
      /* XXX XXX - swap around room index numbers. */
      for (by = 0; by < 6; by++)
	{
	  for (bx = 0; bx < 18; bx++)
	    {
	      if      (dun->room_map[by][bx] == pick2 + 1) 
		dun->room_map[by][bx] =  pick1 + 1;
	      else if (dun->room_map[by][bx] == pick1 + 1) 
		dun->room_map[by][bx] =  pick2 + 1;
	    }
	}
    }
  
  /* Start with no tunnel doors */
  dun->door_n = 0;
  
  /* Mark the first room as being connected. */
  dun->connected[0] = TRUE;
  
  /* Connect all the rooms together (and locate grids for tunnel doors) */
  for (i = 0; i < dun->cent_n; i++)
    {
      /* Connect the room to the next room. */
      if (i == dun->cent_n - 1) build_tunnel(dun->cent_n - 1, 0);
      else build_tunnel(i, i + 1);
    }
  
  /* Place tunnel doors */
  for (i = 0; i < dun->door_n; i++)
    {
      /* Extract junction location */
      y = dun->door[i].y;
      x = dun->door[i].x;
      
      /* Try placing doors */
      try_door(y, x - 1);
      try_door(y, x + 1);
      try_door(y - 1, x);
      try_door(y + 1, x);
    }
  
  
  /* Add some magma streamers */
  for (i = 0; i < DUN_STR_MAG; i++)
    {
      build_streamer(FEAT_MAGMA, DUN_STR_MC);
    }
  
  /* Add some quartz streamers */
  for (i = 0; i < DUN_STR_QUA; i++)
    {
      build_streamer(FEAT_QUARTZ, DUN_STR_QC);
    }
  
  
  /* Destroy the level if necessary */
  if (destroyed) destroy_level(TRUE);
  
  
  /* Place 3 or 4 down stairs near some walls */
  alloc_stairs(FEAT_MORE, rand_range(3, 4), 3);
  
  /* Place 1 or 2 up stairs near some walls */
  alloc_stairs(FEAT_LESS, rand_range(1, 2), 3);

  /* Place 2 or 3 down shafts near some walls */
  alloc_stairs(FEAT_MORE_SHAFT, rand_range(2, 3), 3);
  
  /* Place 1 or 2 up stairs near some walls */
  alloc_stairs(FEAT_LESS_SHAFT, rand_range(1, 2), 3);
  
  /* Determine the character location */
  new_player_spot();
  
  
  /* Basic "amount" */
  k = (p_ptr->depth / 3);
  if (k > 10) k = 10;
  if (k < 2) k = 2;
  
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Moria levels have a lot more monsters. */
  if (moria_level) i *= 2;
  
  /* Moria levels have a high proportion of cave dwellers. */
  if (moria_level)
    {
      /* Set global monster restriction variables. */
      if (underworld)
	mon_restrict('x', (byte)p_ptr->depth, &dummy, TRUE);
      else
	mon_restrict('0', (byte)p_ptr->depth, &dummy, TRUE);
    }
  else 
    {
      /* Remove all monster restrictions. */
      mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
    }
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(0, TRUE, TRUE);
    }
  
  /* Ensure quest monsters */
  if (is_quest(p_ptr->stage))
    {
      /* Ensure quest monsters */
      for (i = 1; i < z_info->r_max; i++)
	{
	  monster_race *r_ptr = &r_info[i];
	  /* Ensure quest monsters */
	  if ((r_ptr->flags1 & (RF1_QUESTOR)) && 
	      (r_ptr->level == p_ptr->depth) && (r_ptr->cur_num < 1))
	    {
	      int y, x;
	      
	      /* Pick a location */
	      while (1)
		{
		  y = rand_int(DUNGEON_HGT);
		  x = rand_int(DUNGEON_WID);
		  
		  if (cave_exist_mon(r_ptr, y, x, FALSE))
		    break;
		  
		  
		}
	      
	      /* Place the questor */
	      place_monster_aux(y, x, i, TRUE, TRUE);
	      
	      
	    }
	}
    }
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some rubble in corridors. */
  alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	}
    }
}

/*** Various wilderness helper routines  ***/


/**
 * Makes "paths to nowhere" from interstage paths toward the middle of the
 * current stage.  Adapted from tunnelling code.
 */

static coord path_start(int sy, int sx, int ty, int tx)
{
  
  int fy, fx, y, x, i, row_dir, col_dir;
  coord pathend; 
  
  /* make sure targets are in bounds, reflect back in if not */
  ty += ABS(ty) - ty - ABS(DUNGEON_HGT - 1 - ty) + (DUNGEON_HGT - 1 - ty);
  tx += ABS(tx) - tx - ABS(DUNGEON_WID - 1 - tx) + (DUNGEON_WID - 1 - tx);
  
  /* Set last point in case of out of bounds */
  fy = sy;
  fx = sx;
  
  /* start */
  correct_dir(&row_dir, &col_dir, sy, sx, ty, tx);
  y = sy + row_dir;
  x = sx + col_dir;
  if (in_bounds_fully(y, x))
    {
      cave_set_feat(y, x, FEAT_FLOOR);
      fy = y;
      fx = x;
    }
  else
    {
      pathend.x = fx;
      pathend.y = fy;
      return (pathend);
    }

  /* 100 steps should be enough */
  for (i = 0; i < 50; i++)
    {
      /* one randomish step... */
      adjust_dir(&row_dir, &col_dir, y, x, ty, tx);
      y += row_dir;
      x += col_dir;
      if (in_bounds_fully(y, x))
	{
	  cave_set_feat(y, x, FEAT_FLOOR);
	  fy = y;
	  fx = x;
	}
      else
	{
	  pathend.x = fx;
	  pathend.y = fy;
	  return (pathend);
	}
      
      /* ...and one good one */
      correct_dir(&row_dir, &col_dir, y, x, ty, tx);
      y += row_dir;
      x += col_dir;
      if (in_bounds_fully(y, x))
	{
	  cave_set_feat(y, x, FEAT_FLOOR);
	  fy = y;
	  fx = x;
	}
      else
	{
	  pathend.x = fx;
	  pathend.y = fy;
	  return (pathend);
	}
      
      /* near enough is good enough */
      if ((ABS(x - tx) < 3) && (ABS(y - ty) < 3))
	break;
    }

  /* return where we have finished */
  pathend.x = x;
  pathend.y = y;
  return (pathend);
}

/** Move the path if it might land in a river */
void river_move(int *xp)
{
  int x = (*xp), diff;

  diff = x - DUNGEON_WID / 2;
  if (ABS(diff) < 10)
    x = (diff < 0) ? (x - 10) : (x + 10);

  (*xp) = x;
  return;
}

/**
 * Places paths to adjacent surface stages, and joins them.  Does each 
 * direction separately, which is a bit repetitive -NRM-
 */
static void alloc_paths(int stage, int last_stage)
{
  int y, x, i, j, num, pathnum = 0, path, ty, tx;
  int py = DUNGEON_HGT/2, px = DUNGEON_WID/2, pcoord = p_ptr->path_coord;
  
  int north = stage_map[stage][NORTH];
  int east  = stage_map[stage][EAST];
  int south = stage_map[stage][SOUTH];
  int west  = stage_map[stage][WEST];
  
  coord temp;
  coord pathend[MAX_PATHS];
  
  u16b gp[512];
  int path_grids, pspot;
  
  bool jumped = TRUE;
  bool river;

  /* River levels need special treatment */
  river = (stage_map[p_ptr->stage][STAGE_TYPE] == RIVER);
  
  for (i = 0; i < MAX_PATHS; i++)
    {
      pathend[i].y = 0;
      pathend[i].x = 0;
    }

  /* Hack for finishing Nan Dungortheb */
  if ((last_stage == q_list[2].stage) && (last_stage != 0)) north = last_stage;
  
  /* North */
  if (north)
    {
      /* Harder or easier?  */
      if (stage_map[north][DEPTH] > p_ptr->depth)
	path = FEAT_MORE_NORTH;
      else
	path = FEAT_LESS_NORTH;
      
      /* Way back */
      if ((north == last_stage) && (p_ptr->create_stair))
	{
	  /* Hack - no paths in river */
	  if (river) river_move(&pcoord);
	  
	  cave_set_feat(1, pcoord, path);
	  py = 1;
	  px = pcoord;
	  jumped = FALSE;

	  /* make paths to nowhere */
	  ty = 1 + DUNGEON_HGT/3 + randint(20) - 10;
	  tx = px + randint(40) - 20;
	  temp = path_start(1, px, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
      
      /* Decide number of paths */
      num = rand_range(2, 3);
      
      /* Place "num" paths */
      for (i = 0; i < num; i++)
	{
	  x = 1 + rand_int(DUNGEON_WID/num - 2) + i * DUNGEON_WID/num;

	  /* Hack - no paths in river */
	  if (river) river_move(&x);
	  
	  cave_set_feat(1, x, path);
	  
	  /* make paths to nowhere */
	  ty = 1 + DUNGEON_HGT/3 + randint(20) - 10;
	  tx = x + randint(40) - 20;
	  temp = path_start(1, x, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
    } 
  
  /* East */
  if (east)
    {
      /* Harder or easier?  */
      if (stage_map[east][DEPTH] > p_ptr->depth)
	path = FEAT_MORE_EAST;
      else
	path = FEAT_LESS_EAST;
      
      /* Way back */
      if ((east == last_stage) && (p_ptr->create_stair))
	{
	  cave_set_feat(pcoord, DUNGEON_WID - 2, path);
	  py = pcoord;
	  px = DUNGEON_WID - 2;
	  jumped = FALSE;
	  
	  /* make paths to nowhere */
	  ty = py + randint (40) - 20;
	  tx = DUNGEON_WID - DUNGEON_HGT/3 - randint(20) + 8;
	  temp = path_start(py, DUNGEON_WID - 2, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
      
      /* Decide number of paths */
      num = rand_range(2, 3);

      /* Place "num" paths */
      for (i = 0; i < num; i++)
	{
	  y = 1 + rand_int(DUNGEON_HGT/num - 2) + i * DUNGEON_HGT/num;
	  cave_set_feat(y, DUNGEON_WID - 2, path);
	  
	  /* make paths to nowhere */
	  ty = y + randint (40) - 20;
	  tx = DUNGEON_WID - DUNGEON_HGT/3 - randint(20) + 8;
	  temp = path_start(y, DUNGEON_WID - 2, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
    } 
  
  /* South */
  if (south)
    {
      /* Harder or easier?  */
      if (stage_map[south][DEPTH] > p_ptr->depth)
	path = FEAT_MORE_SOUTH;
      else
	path = FEAT_LESS_SOUTH;
      
      /* Way back */
      if ((south == last_stage) && (p_ptr->create_stair))
	{
	  /* Hack - no paths in river */
	  if (river) river_move(&pcoord);
	  
	  cave_set_feat(DUNGEON_HGT - 2, pcoord, path);
	  py = DUNGEON_HGT - 2;
	  px = pcoord;
	  jumped = FALSE;
	  
	  /* make paths to nowhere */
	  ty = DUNGEON_HGT - DUNGEON_HGT/3 - randint(20) + 8;
	  tx = px + randint(40) - 20;
	  temp = path_start(DUNGEON_HGT - 2, px, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
      
      /* Decide number of paths */
      num = rand_range(2, 3);
      
      /* Place "num" paths */
      for (i = 0; i < num; i++)
	{
	  x = 1 + rand_int(DUNGEON_WID/num - 2) + i * DUNGEON_WID/num;

	  /* Hack - no paths in river */
	  if (river) river_move(&x);
	  
	  cave_set_feat(DUNGEON_HGT - 2, x, path);
	  
	  /* make paths to nowhere */
	  ty = DUNGEON_HGT - DUNGEON_HGT/3 - randint(20) + 8;
	  tx = x + randint(40) - 20;
	  temp = path_start(DUNGEON_HGT - 2, x, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
    } 
  
  /* West */
  if (west)
    {
      /* Harder or easier?  */
      if (stage_map[west][DEPTH] > p_ptr->depth)
	path = FEAT_MORE_WEST;
      else
	path = FEAT_LESS_WEST;
      
      /* Way back */
      if ((west == last_stage) && (p_ptr->create_stair))
	{
	  cave_set_feat(pcoord, 1, path);
	  py = pcoord;
	  px = 1;
	  jumped = FALSE;
	  
	  /* make paths to nowhere */
	  ty = py + randint (40) - 20;
	  tx = 1 + DUNGEON_HGT/3 + randint(20) - 10;
	  temp = path_start(py, 1, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
      
      /* Decide number of paths */
      num = rand_range(2, 3);
      
      /* Place "num" paths */
      for (i = 0; i < num; i++)
	{
	  y = 1 + rand_int(DUNGEON_HGT/num - 2) + i * DUNGEON_HGT/num;
	  cave_set_feat(y, 1, path);
	  
	  /* make paths to nowhere */
	  ty = y + randint (40) - 20;
	  tx = 1 + DUNGEON_HGT/3 + randint(20) - 10;
	  temp = path_start(y, 1, ty, tx);
	  for (j = MAX_PATHS - 1; j >=0; j--)
	    {
	      if (j == 0) pathend[j] = temp;
	      if (pathend[j].x == 0) continue;
	      if (pathend[j].x < temp.x) 
		{
		  pathend[j + 1] = pathend[j];
		  continue;
		}
	      pathend[j + 1] = temp;
	      pathnum++;
	      break;
	    }
	}
    } 
  
  /* Find the middle of the paths */
  pathnum /= 2;

  /* Make them paths to somewhere */
  
  for (i = 0; i < MAX_PATHS - 1; i++)
    {
      /* All joined? */
      if (!pathend[i + 1].x) 
	break;
      
      /* Find the shortest path */
      path_grids = project_path(gp, 512, pathend[i].y, pathend[i].x, 
				pathend[i + 1].y, pathend[i + 1].x, TRUE);

      /* Get the jumped player spot */
      if ((i == pathnum) && (jumped))
	{
	  pspot = path_grids/2;
	  py = GRID_Y(gp[pspot]);
	  px = GRID_X(gp[pspot]);
	}
      
      /* Make the path, adding an adjacent grid 8/9 of the time */
      for (j = 0; j < path_grids; j++)
	{
	  cave_set_feat(GRID_Y(gp[j]), GRID_X(gp[j]), FEAT_FLOOR);
	  cave_set_feat(GRID_Y(gp[j]) + rand_int(3) - 1, 
			GRID_X(gp[j]) + rand_int(3) - 1, FEAT_FLOOR);
	}
    }
  /* Place the player, unless we've just come upstairs */
  if (((stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE) &&
       (stage_map[p_ptr->last_stage][LOCALITY] != UNDERWORLD)) ||
      ((turn <= 10) && (adult_thrall)))
    return;
  
  player_place(py, px);
}


/**
 * Make a formation - a randomish group of terrain squares. -NRM-
 * Care probably needed with declaring feat[].
 *
 * As of FAangband 0.2.2, wilderness "vaults" are now made here.  These
 * are less structured than cave vaults or webs; in particular other
 * formations or even "vaults" can bleed into them.
 *
 */

int make_formation(int y, int x, int base_feat1, int base_feat2, int *feat, 
		   int prob)
{
  int terrain, j, jj, i = 0, total = 0;
  int *all_feat = malloc( prob * sizeof (*all_feat) );
  int ty = y;
  int tx = x;
  feature_type *f_ptr;
  
  /* Need to make some "wilderness vaults" */
  if (wild_vaults)
    {
      vault_type *v_ptr;
      int n, yy, xx;
      int v_cnt = 0;
      int *v_idx = malloc(z_info->v_max * sizeof(*v_idx));
      
      bool good_place = TRUE;

      /* Greater "vault" ? */
      if (rand_int(100 - p_ptr->depth) < 9) wild_type += 1;

      /* Examine each "vault" */
      for (n = 0; n < z_info->v_max; n++)
	{
	  /* Access the "vault" */
	  v_ptr = &v_info[n];
	  
	  /* Accept each "vault" that is acceptable for this location */
	  if ((v_ptr->typ == wild_type) && (v_ptr->min_lev <= p_ptr->depth) 
	      && (v_ptr->max_lev >= p_ptr->depth))
	    {
	      v_idx[v_cnt++] = n;
	    }
	}
      
      /* If none appropriate, cancel vaults for this level */
      if (!v_cnt)
	{
	  wild_vaults = 0;
	  free(all_feat);
	  free(v_idx);
	  return (0);
	}

      /* Access a random "vault" record */
      v_ptr = &v_info[v_idx[rand_int(v_cnt)]];
      
      /* Check to see if it will fit here (only avoid edges) */
      if ((in_bounds_fully(y - v_ptr->hgt/2, x - v_ptr->wid/2)) &&
	  (in_bounds_fully(y + v_ptr->hgt/2, x + v_ptr->wid/2)))
	{
	  for (yy = y - v_ptr->hgt/2; yy < y + v_ptr->hgt/2; yy++)
	    for (xx = x - v_ptr->wid/2; xx < x + v_ptr->wid/2; xx++)
	      {
		f_ptr = &f_info[cave_feat[yy][xx]];
		if ((f_ptr->flags & TF_PERMANENT) ||
		    (distance(yy, xx, p_ptr->py, p_ptr->px) < 20) ||
		    (cave_info[yy][xx] & CAVE_ICKY)) 
		  good_place = FALSE;
	      }
	}
      else
	good_place = FALSE;
      
      /* We've found a place */
      if (good_place)
	{ 
	  /* Build the "vault" (never lit, icky) */
	  if (!build_vault(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text, 
			   FALSE, TRUE, wild_type)) 
	    {
	      free(all_feat);
	      free(v_idx);
	      return (0);
	    }
	  
	  /* Boost the rating */
	  rating += v_ptr->rat;

	  /* Message */
	  if (cheat_room) msg_format("%s. ",v_name + v_ptr->name);
  
	  /* One less to make */
	  wild_vaults--;
  
	  /* Takes up some space */
	  free(all_feat);
	  free(v_idx);
	  return (v_ptr->hgt * v_ptr->wid);
	}
    }
  
  
  
  /* Extend the array of terrain types to length prob */
  jj = 0;
  for (j = 0; j < prob - 1; j++)
    {
      if (feat[jj] == FEAT_NONE)
	jj = 0;
      all_feat[j] = feat[jj];
      jj++;
    }
  
  /* Make a formation */  
  while (i != (prob - 1))
    {
      /* Avoid paths, stay in bounds */
      if (((cave_feat[ty][tx] != base_feat1) && 
	   (cave_feat[ty][tx] != base_feat2)) || 
	  !(in_bounds_fully(ty, tx)) || 
	  (cave_info[ty][tx] & CAVE_ICKY))
	{
	  free(all_feat);
	  return (total);
	}
      
      /* Check for treasure */
      if ((all_feat[i] == FEAT_MAGMA) && (rand_int(DUN_STR_MC) == 0))
	all_feat[i] = FEAT_MAGMA_K;
      else if ((all_feat[i] == FEAT_QUARTZ) && (rand_int(DUN_STR_QC) == 0))
	all_feat[i] = FEAT_QUARTZ_K;
      
      /* Set the feature */
      cave_set_feat(ty, tx, all_feat[i]);
      cave_info[ty][tx] |= (CAVE_ICKY);
      if ((all_feat[i] >= FEAT_MAGMA) && (all_feat[i] <= FEAT_PERM_SOLID))
	cave_info[ty][tx] |= (CAVE_WALL);
      
      /* Choose a random step for next feature, try to keep going */
      terrain = rand_int(8) + 1;
      if (terrain > 4) terrain++;
      for (j = 0; j < 100; j++)
	{
	  ty += ddy[terrain];
	  tx += ddx[terrain];
	  if (!(cave_info[ty][tx] & (CAVE_ICKY))) break;
	}
      
      /* Count */
      total++;
      
      /* Pick the next terrain, or finish */
      i = rand_int(prob);
    }
    
  free(all_feat);
  return (total);
}


/**
 * Generate a new plain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void plain_gen(void)
{
  int i, j, k, y, x;
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  int form_grids = 0;
  
  int form_feats[8] = {FEAT_TREE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID, 
		       FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE};
  int ponds[2] = {FEAT_WATER, FEAT_NONE};
  
  bool dummy;
  feature_type *f_ptr;
  
  /* Hack -- Start with basic grass  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create grass */
	  cave_feat[y][x] = FEAT_GRASS;
	}
    }
  
  
  /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
  alloc_paths(stage, last_stage);
  
  /* Special boundary walls -- Top */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = 0; y < i; y++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];

	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT))
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  /* Special boundary walls -- Bottom */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = DUNGEON_HGT - 1; y > DUNGEON_HGT - 1 - i; y--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  	  
  /* Special boundary walls -- Left */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = 0; x < i; x++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
	  
  /* Special boundary walls -- Right */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = DUNGEON_WID - 1; x > DUNGEON_WID - 1 - i; x--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  /* Place some formations */
  while (form_grids < (50 * p_ptr->depth + 1000))
    {
      /* Set the "vault" type */
      wild_type = ((rand_int(5) == 0) ? 26 : 14);
      
      /* Choose a place */
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_GRASS, FEAT_GRASS, form_feats, 
				   p_ptr->depth + 1);
    }
  
  /* And some water */
  form_grids = 0;
  while (form_grids < 300) 
    {
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_GRASS, FEAT_GRASS, ponds, 10);
    }
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);

  /* Gets hairy north of the mountains */
  if (p_ptr->depth > 40) k += 10;
  
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k/2));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	  
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    {
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
}

void mtn_connect(int y, int x, int y1, int x1)
{
  u16b gp[512];
  int path_grids, j;
  
  /* Find the shortest path */
  path_grids = project_path(gp, 512, y, x, y1, x1, TRUE);
  
  /* Make the path, adding an adjacent grid 8/9 of the time */
  for (j = 0; j < path_grids; j++)
    {
      if ((cave_feat[GRID_Y(gp[j])][GRID_X(gp[j])] == FEAT_FLOOR) ||
	  (!in_bounds_fully(GRID_Y(gp[j]), GRID_X(gp[j])))) break;
      cave_set_feat(GRID_Y(gp[j]), GRID_X(gp[j]), FEAT_FLOOR);
      cave_info[GRID_Y(gp[j])][GRID_X(gp[j])] |= (CAVE_ICKY);
      /* y2 = GRID_Y(gp[j]) + rand_int(3) - 1;
	 x2 =GRID_X(gp[j]) + rand_int(3) - 1;
	 if (in_bounds_fully(y2, x2))
	 cave_set_feat(y2, x2, FEAT_FLOOR);*/
    }
}

/**
 * Generate a new mountain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void mtn_gen(void)
{
  bool made_plat;
  
  int i, j, k, y, x;
  int plats, a, b;
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  int form_grids = 0;
  int min, dist, floors = 0;
  int randpoints[20];
  coord pathpoints[20];
  coord nearest_point = {DUNGEON_HGT/2, DUNGEON_WID/2};
  coord stairs[3];
  
  /* Amusing hack to make paths work */
  int form_feats[8] = {FEAT_TRAP_HEAD, FEAT_TRAP_HEAD + 1, FEAT_TRAP_HEAD + 2, 
		       FEAT_TRAP_HEAD + 3, FEAT_TRAP_HEAD + 4, FEAT_NONE};
	
  bool dummy;
  bool amon_rudh = FALSE;
  
  
  /* Hack -- Start with basic grass (lets paths work -NRM-) */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create grass */
	  cave_feat[y][x] = FEAT_GRASS;
	}
    }
  
  
  /* Special boundary walls -- Top */
  for (x = 0; x < DUNGEON_WID; x++)
    {
      y = 0;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Special boundary walls -- Bottom */
  for (x = 0; x < DUNGEON_WID; x++)
    {
      y = DUNGEON_HGT - 1;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Special boundary walls -- Left */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      x = 0;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Special boundary walls -- Right */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      x = DUNGEON_WID - 1;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Place 2 or 3 paths to neighbouring stages, make the paths 
     through the stage, place the player -NRM- */
  alloc_paths(stage, last_stage);
  
  /* Dungeon entrance */
  if ((stage_map[stage][DOWN]) &&
      (stage_map[stage_map[stage][DOWN]][LOCALITY] != UNDERWORLD))
    {
      /* Set the flag */
      amon_rudh = TRUE;

      /* Mim's cave on Amon Rudh */
      i = 3; 
      while (i)
	{
	  y = rand_int(DUNGEON_HGT - 2) + 1;
	  x = rand_int(DUNGEON_WID - 2) + 1;
	  if ((cave_feat[y][x] == FEAT_FLOOR) || 
	      (cave_feat[y][x] == FEAT_GRASS))
	    {
	      cave_set_feat(y, x, FEAT_MORE);
	      i--;
	      stairs[2 - i].y = y;
	      stairs[2 - i].x = x;
	      if ((i == 0) && 
		  (stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE))
		player_place(y, x);
	    }
	}
    }
  
  
  /* Make paths permanent */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	if (cave_feat[y][x] == FEAT_FLOOR)
	  {
	    /* Hack - prepare for plateaux, connecting */
	    cave_info[y][x] |= (CAVE_ICKY);
	    floors++;
	  }
    }
  
  /* Pick some joining points */
  for (j = 0; j < 20; j++)
    randpoints[j] = rand_int(floors);
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  if (cave_feat[y][x] == FEAT_FLOOR)
	    floors--;
	  else continue;
	  for (j = 0; j < 20; j++)
	    {
	      if (floors == randpoints[j])
		{
		  pathpoints[j].y = y;
		  pathpoints[j].x = x;
		}
	    }
	}
    }

  /* Find the staircases, if any */
  if (amon_rudh)
    {
      for (j = 0; j < 3; j++)
	{
	  y = stairs[j].y;
	  x = stairs[j].x;

	  /* Now join them up */
	  min = DUNGEON_WID + DUNGEON_HGT;
	  for (i = 0; i < 20; i++)
	    {
	      dist = distance(y, x, pathpoints[i].y, pathpoints[i].x);
	      if (dist < min)
		{
		  min = dist;
		  nearest_point = pathpoints[i];
		}
	    }
	  mtn_connect(y, x, nearest_point.y, nearest_point.x);
	}
    }
  
  /* Make a few "plateaux" */
  plats = rand_range(2, 4);
  
  /* Try fairly hard */
  for (j = 0; j < 50; j++)
    {
      /* Try for a plateau */
      a = rand_int(6) + 4;
      b = rand_int(5) + 4;
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      made_plat = generate_starburst_room(y - b, x - a, y + b, x + a, FALSE, 
					  FEAT_TRAP_HEAD + 2, TRUE);
      
      /* Success ? */
      if (made_plat) 
	{
	  plats--;
	  
	  /* Now join it up */
	  min = DUNGEON_WID + DUNGEON_HGT;
	  for (i = 0; i < 20; i++)
	    {
	      dist = distance(y, x, pathpoints[i].y, pathpoints[i].x);
	      if (dist < min)
		{
		  min = dist;
		  nearest_point = pathpoints[i];
		}
	    }
	  mtn_connect(y, x, nearest_point.y, nearest_point.x);
	}
      
      
      /* Done ? */
      if (!plats) break;
    }
  
  
  
  while  (form_grids < 50 * (p_ptr->depth)  ) 
    {
      /* Set the "vault" type */
      wild_type = ((rand_int(5) == 0) ? 26 : 16);
      
      /* Choose a place */
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_GRASS, FEAT_GRASS, form_feats, 
				   p_ptr->depth * 2);
      /* Now join it up */
      min = DUNGEON_WID + DUNGEON_HGT;
      for (i = 0; i < 20; i++)
	{
	  dist = distance(y, x, pathpoints[i].y, pathpoints[i].x);
	  if (dist < min)
	    {
	      min = dist;
	      nearest_point = pathpoints[i];
	    }
	}
      mtn_connect(y, x, nearest_point.y, nearest_point.x);
      
    }
  
  /* Now change all the terrain to what we really want */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create grass */
	  switch (cave_feat[y][x])
	    {
	    case FEAT_GRASS:
	      {
		cave_set_feat(y, x, FEAT_WALL_SOLID);
		cave_info[y][x] |= (CAVE_WALL);
		break;
	      }
	    case FEAT_TRAP_HEAD:
	      {
		cave_set_feat(y, x, FEAT_RUBBLE);
		break;
	      } 
	    case FEAT_TRAP_HEAD + 1:
	      {
		cave_set_feat(y, x, FEAT_MAGMA);
		cave_info[y][x] |= (CAVE_WALL);
		break;
	      } 
	    case FEAT_TRAP_HEAD + 2:
	      {
		cave_set_feat(y, x, FEAT_GRASS);
		break;
	      } 
	    case FEAT_TRAP_HEAD + 3:
	      {
		if (randint(p_ptr->depth + HIGHLAND_TREE_CHANCE) 
		    > HIGHLAND_TREE_CHANCE)
		  cave_set_feat(y, x, FEAT_TREE2);
		else
		  cave_set_feat(y, x, FEAT_TREE);		  
		break;
	      } 
	    case FEAT_TRAP_HEAD + 4:
	      {
		cave_set_feat(y, x, FEAT_FLOOR);
		break;
	      } 
	    }
	}
    }
  
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	  
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    {
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);

  /* Gets hairy north of the mountains */
  if (p_ptr->depth > 40) k += 10;
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	}
    }
}

/**
 * Generate a new mountaintop level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 */
static void mtntop_gen(void)
{
  bool made_plat;
  
  int i, j, k, y, x, y1, x1;
  int plats, a, b;
  int spot, floors = 0;
  bool placed = FALSE;
  
  /* Hack -- Start with void */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create void */
	  cave_feat[y][x] = FEAT_VOID;
	}
    }
  
  
  /* Special boundary walls -- Top */
  for (x = 0; x < DUNGEON_WID; x++)
    {
      y = 0;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Special boundary walls -- Bottom */
  for (x = 0; x < DUNGEON_WID; x++)
    {
      y = DUNGEON_HGT - 1;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Special boundary walls -- Left */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      x = 0;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Special boundary walls -- Right */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      x = DUNGEON_WID - 1;
      
      /* Clear previous contents, add "solid" perma-wall */
      cave_set_feat(y, x, FEAT_PERM_SOLID);
      cave_info[y][x] |= (CAVE_WALL);
    }
  
  /* Make the main mountaintop */
  while (!placed)
    {
      a = rand_int(6) + 4;
      b = rand_int(5) + 4;
      y = DUNGEON_HGT / 2;
      x = DUNGEON_WID / 2;
      placed = generate_starburst_room(y - b, x - a, y + b, x + a, FALSE, 
					  FEAT_FLOOR, FALSE);
    }

  /* Summit */
  for (i = -1; i <= 1; i++)
    {
      cave_feat[y + i][x] = FEAT_WALL_SOLID;
      cave_info[y + i][x] |= CAVE_WALL;
      cave_feat[y][x + i] = FEAT_WALL_SOLID;
      cave_info[y][x + i] |= CAVE_WALL;
    }

  /* Count the floors */
  for (y1 = y - b; y1 < y + b; y1++)
    for (x1 = x - a; x1 < x + a; x1++)
      if (cave_feat[y1][x1] == FEAT_FLOOR)
	floors++;

  /* Choose the player place */
  spot = rand_int(floors);

  /* Can we get down? */
  if (rand_int(2) == 0) 
    {
      y1 = rand_range(y - b, y + b);
      if (cave_feat[y1][x] != FEAT_VOID)
	{
	  i = rand_int(2);
	  if (i == 0) i = -1;
	  for (x1 = x; x1 != (x + i * (a + 1)); x1 += i)
	    if (cave_feat[y1][x1] == FEAT_VOID) break;
	  cave_set_feat(y1, x1, FEAT_MORE);
	}
    }
	

  /* Adjust the terrain, place the player */
  for (y1 = y - b; y1 < y + b; y1++)
    for (x1 = x - a; x1 < x + a; x1++)
      {
	/* Only change generated stuff */
	if (cave_feat[y1][x1] == FEAT_VOID) continue;

	/* Leave rock */
	if (cave_info[y1][x1] & CAVE_WALL) continue;

	/* Leave stair */
	if (cave_feat[y1][x1] == FEAT_MORE) continue;

	/* Place the player? */
	if (cave_feat[y1][x1] == FEAT_FLOOR)
	  {
	    floors--;
	    if (floors == spot) 
	      {
		player_place(y1, x1);
		cave_info[y1][x1] |= (CAVE_ICKY);
		continue;
	      }
	  }
	
	/* Place some rock */
	if (rand_int(10) < 2) 
	  {
	    cave_set_feat(y1, x1, FEAT_WALL_SOLID);
	    cave_info[y][x] |= (CAVE_WALL);
	    continue;
	  }
	
	/* rubble */
	if (rand_int(8) == 0) 
	  {
	    cave_set_feat(y1, x1, FEAT_RUBBLE);
	    continue;
	  }
	
	/* and the odd tree */
	if (rand_int(20) == 0) 
	  {
	    cave_set_feat(y1, x1, FEAT_TREE2);
	    continue;
	  }
      }
  
  /* Make a few "plateaux" */
  plats = rand_int(4);
  
  /* Try fairly hard */
  for (j = 0; j < 10; j++)
    {
      /* Try for a plateau */
      a = rand_int(6) + 4;
      b = rand_int(5) + 4;
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      made_plat = generate_starburst_room(y - b, x - a, y + b, x + a, FALSE, 
					  FEAT_FLOOR, FALSE);
      
      /* Success ? */
      if (made_plat) 
	{
	  plats--;
	  
	  /* Adjust the terrain a bit */
	  for (y1 = y - b; y1 < y + b; y1++)
	    for (x1 = x - a; x1 < x + a; x1++)
	      {
		/* Only change generated stuff */
		if (cave_feat[y1][x1] == FEAT_VOID) continue;

		/* Place some rock */
		if (rand_int(10) < 2) 
		  {
		    cave_set_feat(y1, x1, FEAT_WALL_SOLID);
		    cave_info[y][x] |= (CAVE_WALL);
		    continue;
		  }

		/* rubble */
		if (rand_int(8) == 0) 
		  {
		    cave_set_feat(y1, x1, FEAT_RUBBLE);
		    continue;
		  }
		
		/* and the odd tree */
		if (rand_int(20) == 0) 
		  {
		    cave_set_feat(y1, x1, FEAT_TREE2);
		    continue;
		  }
	      }

	}
	
      /* Done ? */
      if (!plats) break;
    }
  
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	  
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    {
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  
  
  /* Basic "amount" */
  k = p_ptr->depth;

  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = k; j > 0; j--)
    {
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Put some objects in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	}
    }
}

/**
 * Generate a new forest level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void forest_gen(void)
{
  bool made_plat;
  
  int i, j, k, y, x;
  int plats, a, b;
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  int form_grids = 0;
  
  int form_feats[8] = {FEAT_GRASS, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID, 
		       FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE};
  int ponds[2] = {FEAT_WATER, FEAT_NONE};
  
  bool dummy;
  feature_type *f_ptr;
  
  
  
  
  /* Hack -- Start with basic grass so paths work */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create grass */
	  cave_feat[y][x] = FEAT_GRASS;
	}
    }
  
  
  /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
  alloc_paths(stage, last_stage);
  
  /* Special boundary walls -- Top */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = 0; y < i; y++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  /* Special boundary walls -- Bottom */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = DUNGEON_HGT - 1; y > DUNGEON_HGT - 1 - i; y--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  	  
  /* Special boundary walls -- Left */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = 0; x < i; x++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
	  
  /* Special boundary walls -- Right */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = DUNGEON_WID - 1; x > DUNGEON_WID - 1 - i; x--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  /* Now place trees */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create trees */
	  if (cave_feat[y][x] == FEAT_GRASS)
	    {
	      if (randint(p_ptr->depth + HIGHLAND_TREE_CHANCE) 
		  > HIGHLAND_TREE_CHANCE)
		cave_set_feat(y, x, FEAT_TREE2);
	      else
		cave_set_feat(y, x, FEAT_TREE);
	    }		  
	  else
	    /* Hack - prepare for clearings */
	    cave_info[y][x] |= (CAVE_ICKY);

	  /* Mega hack - remove paths if emerging from Nan Dungortheb */
	  if ((last_stage == q_list[2].stage) && 
	      (cave_feat[y][x] == FEAT_MORE_NORTH))
	    cave_set_feat(y, x, FEAT_GRASS);
	}
    }
  
  /* Make a few clearings */
  plats = rand_range(2, 4);
  
  /* Try fairly hard */
  for (j = 0; j < 50; j++)
    {
      /* Try for a clearing */
      a = rand_int(6) + 4;
      b = rand_int(5) + 4;
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      made_plat = generate_starburst_room(y - b, x - a, y + b, x + a, FALSE, 
					  FEAT_GRASS, TRUE);
      
      /* Success ? */
      if (made_plat) plats--;
      
      /* Done ? */
      if (!plats) break;
    }
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  
  /* Place some formations */
  while (form_grids < (50 * p_ptr->depth + 1000))
    {
      /* Set the "vault" type */
      wild_type = ((rand_int(5) == 0) ? 26 : 18);
      
      /* Choose a place */
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_TREE, FEAT_TREE2, form_feats, 
				   p_ptr->depth + 1);
    }
  
  /* And some water */
  form_grids = 0;
  while (form_grids < 300) 
    {
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_TREE, FEAT_TREE2, ponds, 10);
    }
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);

  /* Gets hairy north of the mountains */
  if (p_ptr->depth > 40) k += 10;
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}

/**
 * Generate a new swamp level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void swamp_gen(void)
{
  int i, j, k, y, x;
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  int form_grids = 0;
  
  
  int form_feats[8] = {FEAT_TREE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID, 
		       FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE};
  
  bool dummy;
  feature_type *f_ptr;
  
  
  
  
  /* Hack -- Start with grass */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_feat[y][x] = FEAT_GRASS;
	}
    }
  
  
  /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
  alloc_paths(stage, last_stage);
  
  /* Special boundary walls -- Top */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = 0; y < i; y++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  /* Special boundary walls -- Bottom */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = DUNGEON_HGT - 1; y > DUNGEON_HGT - 1 - i; y--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  	  
  /* Special boundary walls -- Left */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = 0; x < i; x++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
	  
  /* Special boundary walls -- Right */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = DUNGEON_WID - 1; x > DUNGEON_WID - 1 - i; x--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  /* Hack -- add water  */
  for (y = 1; y < DUNGEON_HGT - 1; y++)
    {
      for (x = 1; x < DUNGEON_WID - 1; x++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];

	  if (f_ptr->flags & TF_PERMANENT)
	    continue;
	  if (((p_ptr->py == y) && (p_ptr->px == x)) || (rand_int(100) < 50))
	    cave_set_feat(y, x, FEAT_GRASS);
	  else
	    cave_set_feat(y, x, FEAT_WATER);
	}
    }
  
  
  /* Place some formations (but not many, and less for more danger) */
  while (form_grids < 20000 / p_ptr->depth) 
    {
      /* Set the "vault" type */
      wild_type = ((rand_int(5) == 0) ? 26 : 20);
      
      /* Choose a place */
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_GRASS, FEAT_WATER, form_feats, 
				   p_ptr->depth);
    }
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);

  /* Gets hairy north of the mountains */
  if (p_ptr->depth > 40) k += 10;
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}

/**
 * Generate a new desert level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void desert_gen(void)
{
  bool made_plat;
  
  int i, j, k, y, x, d;
  int plats, a, b;
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  int form_grids = 0;
  
  int form_feats[8] = {FEAT_GRASS, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID, 
		       FEAT_DUNE, FEAT_QUARTZ, FEAT_NONE};
  bool dummy;
  bool made_gate = FALSE;
  feature_type *f_ptr;
  
  
  
  /* Hack -- Start with basic grass so paths work */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create grass */
	  cave_feat[y][x] = FEAT_GRASS;
	}
    }
  
  
  /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
  alloc_paths(stage, last_stage);
  
  /* Special boundary walls -- Top */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = 0; y < i; y++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  /* Special boundary walls -- Bottom */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = DUNGEON_HGT - 1; y > DUNGEON_HGT - 1 - i; y--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  	  
  /* Special boundary walls -- Left */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = 0; x < i; x++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
	  
  /* Special boundary walls -- Right */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = DUNGEON_WID - 1; x > DUNGEON_WID - 1 - i; x--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  /* Dungeon entrance */
  if ((stage_map[stage][DOWN]) &&
      (stage_map[stage_map[stage][DOWN]][LOCALITY] != UNDERWORLD))
    {
      /* Hack - no vaults */
      wild_vaults = 0;

      /* Angband! */
      for (d = 0; d < DUNGEON_WID; d++)
	{
	  for (y = 0; y < d; y++)
	    {
	      x = d - y;
	      if (!in_bounds_fully(y, x))
		continue;
	      if (cave_feat[y][x] == FEAT_FLOOR)
		{
		  /* The gate of Angband */
		  cave_set_feat(y, x, FEAT_MORE);
		  made_gate = TRUE;
		  if ((stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE) ||
		      (turn < 10))
		    player_place(y, x);
		  break;
		}
	      else
		{
		  /* The walls of Thangorodrim */
		  cave_set_feat(y, x, FEAT_WALL_SOLID);
		  cave_info[y][x] |= CAVE_WALL;
		}
	    }
	  if (made_gate) break;
	}
    }
  
  /* Now place rubble, sand and magma */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create desert */
	  if (cave_feat[y][x] == FEAT_GRASS)
	    {
	      if (rand_int(100) < 50)
		cave_set_feat(y, x, FEAT_DUNE);
	      else if (rand_int(100) < 50)
		cave_set_feat(y, x, FEAT_RUBBLE);
	      else
		cave_set_feat(y, x, FEAT_MAGMA);
	    }
	  else
	    /* Hack - prepare for clearings */
	    cave_info[y][x] |= (CAVE_ICKY);
	}
    }
  
  /* Make a few clearings */
  plats = rand_range(2, 4);
  
  /* Try fairly hard */
  for (j = 0; j < 50; j++)
    {
      /* Try for a clearing */
      a = rand_int(6) + 4;
      b = rand_int(5) + 4;
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      made_plat = generate_starburst_room(y - b, x - a, y + b, x + a, FALSE, 
					  FEAT_GRASS, TRUE);
      
      /* Success ? */
      if (made_plat) plats--;
      
      /* Done ? */
      if (!plats) break;
    }
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  
  /* Place some formations */
  while (form_grids < 20 * p_ptr->depth) 
    {
      /* Set the "vault" type */
      wild_type = ((rand_int(5) == 0) ? 26 : 22);
      
      /* Choose a place */
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_RUBBLE, FEAT_MAGMA, form_feats, 
				   p_ptr->depth);
    }
  
  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);

  /* Gets hairy north of the mountains */
  if (p_ptr->depth > 40) k += 10;
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}


/**
 * Generate a new river level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void river_gen(void)
{
  int i, j, k, y, x, y1 = DUNGEON_HGT/2;
  int mid[DUNGEON_HGT];
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  int form_grids = 0;
  int path;
  
  int form_feats[8] = {FEAT_TREE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID, 
		       FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE};
  
  bool dummy;
  feature_type *f_ptr;
  
  
  
  
  /* Hack -- Start with basic grass  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create grass */
	  cave_feat[y][x] = FEAT_GRASS;
	}
    }
  
  
  /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
  alloc_paths(stage, last_stage);

  /* Hack - remember the path in case it has to move */
  path = cave_feat[p_ptr->py][p_ptr->px];
  
  /* Special boundary walls -- Top */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = 0; y < i; y++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  
  /* Special boundary walls -- Bottom */
  i = 4;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 7) i = 7;
      if (i < 0) i = 0;
      for (y = DUNGEON_HGT - 1; y > DUNGEON_HGT - 1 - i; y--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  	  
  /* Special boundary walls -- Left */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = 0; x < i; x++)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
		  
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
	  
  /* Special boundary walls -- Right */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = DUNGEON_WID - 1; x > DUNGEON_WID - 1 - i; x--)
	{
	  f_ptr = &f_info[cave_feat[y][x]];
	
	  /* Clear previous contents, add "solid" perma-wall */
	  if ((cave_feat[y][x] != FEAT_FLOOR) && 
	      !(f_ptr->flags & TF_PERMANENT)) 
	    { 
	      cave_set_feat(y, x, FEAT_PERM_SOLID);
	      cave_info[y][x] |= (CAVE_WALL);
	    }
	}
    }
  
  /* Place the river, start in the middle third */
  i = DUNGEON_WID/3 + rand_int(DUNGEON_WID/3);
  for (y = 1; y < DUNGEON_HGT - 1; y++)
    {      
      /* Remember the midpoint */
      mid[y] = i;

      for (x = i - rand_int(5) - 10; x < i + rand_int(5) + 10; x++)
	{
	  /* Make the river */
	  cave_set_feat(y, x, FEAT_WATER);
	  cave_info[y][x] |= (CAVE_ICKY);
	}
      /* Meander */
      i += rand_int(3) - 1;
    }
  
  /* Special dungeon entrances */
  if ((stage_map[stage][DOWN]) && 
      (stage_map[stage_map[stage][DOWN]][LOCALITY] != UNDERWORLD))
    {
      /* Hack - no "vaults" */
      wild_vaults = 0;

      /* If we're at Sauron's Isle... */
      if (stage_map[p_ptr->stage][LOCALITY] == SIRION_VALE)
	{
	  for (y = y1 - 10; y < y1 + 10; y++)
	    {
	      for (x = mid[y] - 10; x < mid[y] + 10; x++)
		{
		  if (distance(y1, mid[y1], y, x) < 6)
		    
		    /* ...make Tol-In-Gaurhoth... */
		    cave_set_feat(y, x, FEAT_GRASS);
		  else
		    
		    /* ...surround it by water... */
		    cave_set_feat(y, x, FEAT_WATER);
		  
		  /* ...and build the tower... */
		  if (distance(y1, mid[y1], y, x) < 2)
		    cave_set_feat(y, x, FEAT_WALL_SOLID);
		}
	    }
	  /* ...with door and stairs */
	  cave_set_feat(y1 + 1, mid[y1], FEAT_DOOR_HEAD);
	  cave_set_feat(y1, mid[y1], FEAT_MORE);
	  if (stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE)
	    player_place(y1, mid[y1]);
	}
      else
	/* Must be Nargothrond */
	{
	  /* This place is hard to get into... */
	  for (y = y1 - 1; y < y1 + 2; y++)
	    {
	      for (x = mid[y1] - 10; x < mid[y1] - 5; x++)
		cave_set_feat(y, x, FEAT_WALL_SOLID);
	      for (x = mid[y1] - 5; x < mid[y1] + 5; x++)
		{
		  if ((y == y1) && (x == mid[y1] - 5))
		    {
		      cave_set_feat(y, x, FEAT_MORE);
		      if (stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE)
			player_place(y, x);
		    }
		  else
		    cave_set_feat(y, x, FEAT_WATER);
		}
	    }
	}
    }
  
  /* Place some formations */
  while (form_grids < 50 * p_ptr->depth + 1000) 
    {
      /* Set the "vault" type */
      wild_type = ((rand_int(5) == 0) ? 26 : 24);
      
      /* Choose a place */
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;

      form_grids += make_formation(y, x, FEAT_GRASS, FEAT_GRASS, form_feats, 
				   p_ptr->depth/2);
    }

  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  /* Hack - move the player out of the river */
  y = p_ptr->py;
  x = p_ptr->px;
  while ((cave_feat[p_ptr->py][p_ptr->px] == FEAT_WATER) ||
	 (cave_feat[p_ptr->py][p_ptr->px] == FEAT_PERM_SOLID))
    p_ptr->px++;

  /* Place player if they had to move */
  if (x != p_ptr->px)
    {
      cave_m_idx[p_ptr->py][p_ptr->px] = -1;
      cave_m_idx[y][x] = 0;
      cave_set_feat(y, x, path);
      for (y = p_ptr->py; y > 0; y--)
	if ((cave_feat[y][p_ptr->px] > FEAT_RUBBLE) && 
	    (cave_feat[y][p_ptr->px] <= FEAT_PERM_SOLID))
	  cave_set_feat(y, p_ptr->px, FEAT_FLOOR);
    }
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);

  /* Gets hairy north of the mountains */
  if (p_ptr->depth > 40) k += 10;
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}

/**
 * Attempt to place a web of the required type
 */
bool place_web(int type)
{
  vault_type *v_ptr;
  int i, y, x = DUNGEON_WID/2, cy, cx;
  int *v_idx = malloc(z_info->v_max * sizeof(v_idx));
  int v_cnt = 0;

  bool no_good = FALSE;
  
  /* Examine each web */
  for (i = 0; i < z_info->v_max; i++)
    {
      /* Access the web */
      v_ptr = &v_info[i];
      
      /* Accept each web that is acceptable for this depth. */
      if ((v_ptr->typ == type) && (v_ptr->min_lev <= p_ptr->depth) && 
	  (v_ptr->max_lev >= p_ptr->depth))
	{
	  v_idx[v_cnt++] = i;
	}
    }

  /* None to be found */
  if (v_cnt == 0)
    {
      free(v_idx);
      return (FALSE);
    }
  
  /* Access a random vault record */
  v_ptr = &v_info[v_idx[rand_int(v_cnt)]];
  
  /* Look for somewhere to put it */
  for (i = 0; i < 25; i++)
    {
      /* Random top left corner */
      cy = randint(DUNGEON_HGT - 1 - v_ptr->hgt);
      cx = randint(DUNGEON_WID - 1 - v_ptr->wid);
      
      /* Check to see if it will fit (only avoid big webs and edges) */
      for (y = cy; y < cy + v_ptr->hgt; y++)
	for (x = cx; x < cx + v_ptr->wid; x++)
	  if ((cave_feat[y][x] == FEAT_VOID) || 
	      (cave_feat[y][x] == FEAT_PERM_SOLID) ||
	      (cave_feat[y][x] == FEAT_MORE_SOUTH) ||
	      ((y == p_ptr->py) && (x == p_ptr->px)) ||
	      (cave_info[y][x] & CAVE_ICKY)) 
	    no_good = TRUE;
      
      /* Try again, or stop if we've found a place */
      if (no_good) 
	{
	  no_good = FALSE;
	  continue;
	}
      else
	break;
    }

  /* Give up if we couldn't find anywhere */
  if (no_good) 
    {
      free(v_idx);
	return (FALSE);
    }
  
  /* Boost the rating */
  rating += v_ptr->rat;
  
  
  /* Build the vault (never lit, not icky unless full size) */
  if (!build_vault(y, x, v_ptr->hgt, v_ptr->wid, v_text + v_ptr->text, 
		   FALSE, (type == 13), type))
    {
      free(v_idx);
      return (FALSE);
    }
  
  free(v_idx);
  return (TRUE);
}





/**
 * Generate a new valley level. Place down slides, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
static void valley_gen(void)
{
  bool made_plat;
  
  int i, j, k, y, x;
  int plats, a, b, num = 2;
  int form_grids = 0;
  int path_x[3];
  int form_feats[8] = {FEAT_GRASS, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID, 
		       FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE};
  bool dummy;
  
  
  /* Hack -- Start with trees */ 
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create trees */
	  if (randint(p_ptr->depth + HIGHLAND_TREE_CHANCE) 
	      > HIGHLAND_TREE_CHANCE)
	    cave_set_feat(y, x, FEAT_TREE2);
	  else
	    cave_set_feat(y, x, FEAT_TREE);		  
	}
    }

  /* Prepare places for down slides */
  num += rand_int(2);
  for (i = 0; i < num; i++)
    path_x[i] = 1 + rand_int(DUNGEON_WID/num - 2) + i * DUNGEON_WID/num;
  
  /* Special boundary walls -- Top */
  i = 5;
  for (x = 0; x < DUNGEON_WID; x++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (y = 0; y < i; y++)
		
	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
      if ((x > 0) && (x == p_ptr->path_coord))
	{
	  if (y == 0) y++;
	  cave_set_feat(y, x, FEAT_RUBBLE);
	  player_place(y, x);
	}
    }
  
  
  /* Special boundary walls -- Bottom */
  i = 5;
  j = 0;
  if (p_ptr->depth != 70)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  i += 1 - rand_int(3);
	  if (i > 10) i = 10;
	  if (i < 0) i = 0;
	  for (y = DUNGEON_HGT - 1; y > DUNGEON_HGT - 1 - i; y--)
	    
	    /* Clear previous contents, add empty space */
	    cave_set_feat(y, x, FEAT_VOID);
	  
	  /* Down slides */
	  if (j < num)
	    if (x == path_x[j])
	      {
		cave_set_feat(y, x, FEAT_MORE_SOUTH);
		j++;
	      }
	}
    }  	 
 
  /* Special boundary walls -- Left */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = 0; x < i; x++)
		  
	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);

    }
	  
  /* Special boundary walls -- Right */
  i = 5;
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      i += 1 - rand_int(3);
      if (i > 10) i = 10;
      if (i < 0) i = 0;
      for (x = DUNGEON_WID - 1; x > DUNGEON_WID - 1 - i; x--)
	
	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }
  
  /* Make a few clearings */
  plats = rand_range(2, 4);
  
  /* Try fairly hard */
  for (j = 0; j < 50; j++)
    {
      /* Try for a clearing */
      a = rand_int(6) + 4;
      b = rand_int(5) + 4;
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      if (cave_feat[y][x] == FEAT_VOID)
	continue;
      made_plat = generate_starburst_room(y - b, x - a, y + b, x + a, FALSE, 
					  FEAT_GRASS, TRUE);
      
      /* Success ? */
      if (made_plat) plats--;
      
      /* Done ? */
      if (!plats) break;
    }
  
  /* Place some formations */
  while (form_grids < (40 * p_ptr->depth)) 
    {
      y = rand_int(DUNGEON_HGT - 1) + 1;
      x = rand_int(DUNGEON_WID - 1) + 1;
      form_grids += make_formation(y, x, FEAT_TREE, FEAT_TREE2, form_feats, 
				   p_ptr->depth + 1);
    }

  /* No longer "icky"  */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_ICKY);
	}
    }
  
  if (!p_ptr->path_coord)
    {
      y = DUNGEON_HGT/2 - 10 + rand_int(20);
      x = DUNGEON_WID/2 - 15 + rand_int(30);
      cave_set_feat(y, x, FEAT_GRASS);
      player_place(y, x);
      p_ptr->path_coord = 0;

      /* Make sure a web can't be placed on the player */
      cave_info[y][x] |= (CAVE_ICKY);
    }
  
  /* Basic "amount" */
  k = (p_ptr->depth / 2);
  if (k > 30) k = 30;
  
  /* Pick a base number of monsters */
  i = MIN_M_ALLOC_LEVEL + randint(8);
  
  /* Build the monster probability table. */
  monster_level = p_ptr->depth;
  (void)get_mon_num(monster_level);
  
  /* Put some monsters in the dungeon */
  for (j = i + k; j > 0; j--)
    {
      /* Always have some random monsters */
      if ((get_mon_num_hook) && (j < 5))
	{
	  /* Remove all monster restrictions. */
	  mon_restrict('\0', (byte)p_ptr->depth, &dummy, TRUE);
	  
	  /* Build the monster probability table. */
	  (void)get_mon_num(p_ptr->depth);
	}
      
      /* 
       * Place a random monster (quickly), but not in grids marked 
       * "CAVE_TEMP".
       */
      (void)alloc_monster(10, TRUE, TRUE);
    }
  
  
  /* Place some traps in the dungeon. */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint(k));
  
  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ROOM, 3));
  
  /* Put some objects/gold in the dungeon */
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, Rand_normal(DUN_AMT_ITEM, 3));
  alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));
  
  /* Place some webs */
  for (i = 0; i < damroll(k/20, 4); i++)
    place_web(11);

  if (rand_int(2) == 0)
    place_web(12);
  
  if (rand_int(10) == 0)
    place_web(13);
  
  /* Clear "temp" flags. */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  cave_info[y][x] &= ~(CAVE_TEMP);
	  /* Paranoia - remake the dungeon walls */
	  
	  if ((y == 0) || (x == 0) || (y == DUNGEON_HGT - 1)
	      || (x == DUNGEON_WID - 1))
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }

  /* Maybe place a few random portals. */
  if (adult_dungeon && stage_map[p_ptr->stage][DOWN])
    {
      feature_type *f_ptr = NULL;

      k = randint(3) + 1;
      while(k > 0)
	{
	  y = randint(DUNGEON_HGT -1);
	  x = randint(DUNGEON_WID - 1);
	  f_ptr = &f_info[cave_feat[y][x]];
	  if (f_ptr->flags & TF_TREE) 
	    {
	      cave_set_feat(y, x, FEAT_MORE);
	      k--;
	    }
	}
    }
}



/**
 * Builds a store at a given pseudo-location
 *
 * As of 2.8.1 (?) the town is actually centered in the middle of a
 * complete level, and thus the top left corner of the town itself
 * is no longer at (0,0), but rather, at (qy,qx), so the constants
 * in the comments below should be mentally modified accordingly.
 *
 * As of 2.7.4 (?) the stores are placed in a more "user friendly"
 * configuration, such that the four "center" buildings always
 * have at least four grids between them, to allow easy running,
 * and the store doors tend to face the middle of town.
 *
 * The stores now lie inside boxes from 3-9 and 12-18 vertically,
 * and from 7-17, 21-31, 35-45, 49-59.  Note that there are thus
 * always at least 2 open grids between any disconnected walls.
 * 
 * The home only appears if it is the player's home town.
 *
 * Note the use of town_illuminate() to handle all "illumination"
 * and "memorization" issues.
 */
static void build_store(int n, int yy, int xx, int stage)
{
  int y, x, y0, x0, y1, x1, y2, x2, tmp;
  
  int qy = DUNGEON_HGT / 3;
  int qx = DUNGEON_WID / 3;
  
  
  /* Find the "center" of the store */
  y0 = qy + yy * 9 + 6;
  x0 = qx + xx * 11 + 11;
  
  /* Determine the store boundaries */
  y1 = y0 - (1 + randint((yy == 0) ? 2 : 1));
  y2 = y0 + (1 + randint((yy == 1) ? 2 : 1));
  x1 = x0 - (1 + randint(3));
  x2 = x0 + (1 + randint(3));
  
  /* Build an invulnerable rectangular building */
  for (y = y1; y <= y2; y++)
    {
      for (x = x1; x <= x2; x++)
	{
	  /* Create the building (or not ... NRM) */
	  if ((n != 7) || (p_ptr->home == stage))
	    cave_set_feat(y, x, FEAT_PERM_EXTRA);
	  else
	    cave_set_feat(y, x, FEAT_FLOOR);
	}
    }
  
  /* Pick a door direction (S,N,E,W) */
  tmp = rand_int(4);
  
  /* Re-roll "annoying" doors */
  if (((tmp == 0) && (yy == 1)) ||
      ((tmp == 1) && (yy == 0)) ||
      ((tmp == 2) && (xx == 3)) ||
      ((tmp == 3) && (xx == 0)))
    {
      /* Pick a new direction */
      tmp = rand_int(4);
    }
  
  /* Extract a "door location" */
  switch (tmp)
    {
      /* Bottom side */
    case 0:
      {
	y = y2;
	x = rand_range(x1, x2);
	break;
      }
      
      /* Top side */
    case 1:
      {
	y = y1;
	x = rand_range(x1, x2);
	break;
      }
      
      /* Right side */
    case 2:
      {
	y = rand_range(y1, y2);
	x = x2;
	break;
      }
      
      /* Left side */
    default:
      {
	y = rand_range(y1, y2);
	x = x1;
	break;
      }
    }
  
  /* Clear previous contents, add a store door */
	if ((n != 7) || (p_ptr->home == stage))
	  cave_set_feat(y, x, FEAT_SHOP_HEAD + n);
	else
	  cave_set_feat(y, x, FEAT_FLOOR);
}




/**
 * Generate the "consistent" town features, and place the player
 *
 * Hack -- play with the R.N.G. to always yield the same town
 * layout, including the size and shape of the buildings, the
 * locations of the doorways, and the location of the stairs.
 */
static void town_gen_hack(void)
{
  int i, y, x, k, n, py = 1, px = 1;
  
  int qy = DUNGEON_HGT / 3;
  int qx = DUNGEON_WID / 3;
  int stage = p_ptr->stage;
  int last_stage = p_ptr->last_stage;
  
  int rooms[MAX_STORES_BIG + 1];
  
  bool place = FALSE;
  bool major = FALSE;
  
  /* Hack -- Use the "simple" RNG */
  Rand_quick = TRUE;
  
  /* Hack -- Induce consistent town layout */
  for (i = 0;i < 10; i++)
    if (stage == towns[i])
      Rand_value = seed_town[i];

  if (adult_dungeon) Rand_value = seed_town[0];
  
  /* Set major town flag if necessary */
  if ((stage > 150) || adult_dungeon)
    major = TRUE;
  
  /* Hack - reduce width for minor towns */
  if (!major)
    qx /= 2;
  
  /* Prepare an Array of "remaining stores", and count them */
  if (major)
    for (n = 0; n < MAX_STORES_BIG; n++) rooms[n] = n;
  else
    {
      rooms[0] = 9;
      rooms[1] = 3;
      rooms[2] = 4;
      rooms[3] = 7;
      n = 4;
    }

  if (adult_dungeon) rooms[n++] = 9;

  /* No stores for ironmen away from home */
  if ((!adult_ironman) || (p_ptr->stage == p_ptr->home))
    {  
      /* Place two rows of stores */
      for (y = 0; y < 2; y++)
	{
	  /* Place two, four or five stores per row */
	  for (x = 0; x < (adult_dungeon ? 5 : 4); x++)
	    {
	      /* Pick a random unplaced store */
	      k = ((n <= 1) ? 0 : rand_int(n));
	      
	      /* Build that store at the proper location */
	      build_store(rooms[k], y, x, stage);
	      
	      /* Shift the stores down, remove one store */
	      rooms[k] = rooms[--n];
	      
	      /* Cut short if a minor town */
	      if ((x > 0) && !major)
		break;
	    }
	}
      /* Hack -- Build the 9th store.  Taken from Zangband */
      if (major && !adult_dungeon)
	build_store(rooms[0], rand_int(2), 4, stage);
    }
      
  if (adult_dungeon)
    {
        /* Place the stairs */
        while (TRUE)
        {
                /* Pick a location at least "three" from the outer walls */
                y = qy + rand_range(3, DUNGEON_HGT / 3 - 4);
                x = qx + rand_range(3, DUNGEON_WID / 3 - 4);

                /* Require a "naked" floor grid */
                if (cave_naked_bold(y, x)) break;
        }

        /* Clear previous contents, add down stairs */
        cave_set_feat(y, x, FEAT_MORE);


        /* Place the player */
        player_place(y, x);
    }

  else
    {

      /* Place the paths */
      for (n = 2; n < 6; n++)
	{
	  /* Pick a path direction for the player if not obvious */
	  if (((!last_stage) || (last_stage == 255)) && (stage_map[stage][n]))
	    last_stage = stage_map[stage][n];
	  
	  /* Where did we come from? */
	  if ((last_stage) && (last_stage == stage_map[stage][n]))
	    place = TRUE;
	  
	  /* Pick a location at least "three" from the corners */
	  y = (DUNGEON_HGT / 3) + rand_range(3, qy - 4);
	  x = (DUNGEON_WID / 3) + rand_range(3, qx - 4);
	  
	  /* Shove it to the wall, place the path */
	  switch (n)
	    {
	    case NORTH: 
	      {
		y = (DUNGEON_HGT / 3) + 1;
		if (stage_map[stage][n])
		  cave_set_feat(y, x, FEAT_MORE_NORTH);
		break;
	      }
	    case EAST: 
	      {
		x = (DUNGEON_WID / 3) + qx - 2;
		if (stage_map[stage][n])
		  cave_set_feat(y, x, FEAT_MORE_EAST);
		break;
	      }
	    case SOUTH: 
	      {
		y = (DUNGEON_HGT / 3) + qy - 2;
		if (stage_map[stage][n])
		  cave_set_feat(y, x, FEAT_MORE_SOUTH);
		break;
	      }
	    case WEST: 
	      {
		x = (DUNGEON_WID / 3) + 1;
		if (stage_map[stage][n])
		  cave_set_feat(y, x, FEAT_MORE_WEST);
	      }
	    }
	  if (place)
	    {
	      py = y;
	      px = x;
	      place = FALSE;
	    }
	}
      
      /* Place the player */
      player_place(py, px);
    }
  
  /* Hack -- use the "complex" RNG */
  Rand_quick = FALSE;
}




/**
 * Town logic flow for generation of new town
 *
 * We start with a fully wiped cave of normal floors.
 *
 * Note that town_gen_hack() plays games with the R.N.G.
 *
 * This function does NOT do anything about the owners of the stores,
 * nor the contents thereof.  It only handles the physical layout.
 *
 * We place the player on the stairs at the same time we make them.
 *
 * Hack -- since the player always leaves the dungeon by the stairs,
 * he is always placed on the stairs, even if he left the dungeon via
 * word of recall or teleport level.
 */
static void town_gen(void)
{
  int i, y, x;
  
  int residents;
  int stage = p_ptr->stage;
  
  int qy = DUNGEON_HGT / 3;
  int qx = DUNGEON_WID / 3;
  int width = qx;
  
  bool daytime;
  bool dummy;
  bool cave = FALSE;
  
  /* Hack - some towns are underground */
  if (((stage > 150) && (stage < 154)) || adult_dungeon)
    cave = TRUE;
  
  /* Hack - smaller for minor towns */
  if ((stage < 151) && (!adult_dungeon))
    width = qx/2;
  
  
  /* Day time */
  if (((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)) && !cave)
    {
      /* Day time */
      daytime = TRUE;
      
      /* Number of residents */
      residents = MIN_M_ALLOC_TD;
    }
  
  /* Night time or cave */
  else
    {
      /* Night time */
      daytime = FALSE;
      
      /* Number of residents */
      residents = MIN_M_ALLOC_TN;
    }

  /* Start with solid walls */
  for (y = 0; y < DUNGEON_HGT; y++)
    {
      for (x = 0; x < DUNGEON_WID; x++)
	{
	  /* Create "solid" perma-wall */
	  cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
  
  /* Boundary walls (see town_illuminate() */
  for (x = qx; x < qx + width; x++)
    {
      cave_set_feat(qy, x, FEAT_PERM_INNER);
      cave_set_feat(qy + (DUNGEON_HGT / 3) - 1, x, FEAT_PERM_INNER);
    }
  
  /* Boundary walls (see town_illuminate() */
  for (y = qy; y < qy + (DUNGEON_HGT / 3); y++)
    {
      cave_set_feat(y, qx, FEAT_PERM_INNER);
      cave_set_feat(y, qx + width - 1, FEAT_PERM_INNER);
    }
  
  /* Then place some floors */
  for (y = qy + 1; y < qy + (DUNGEON_HGT / 3) - 1; y++)
    {
      for (x = qx + 1; x < qx + width - 1; x++)
	{
	  /* Create empty floor */
	  cave_set_feat(y, x, FEAT_FLOOR);
	}
    }
  
  /* Build stuff */
  town_gen_hack();
  
  /* Apply illumination */
  town_illuminate(daytime, cave);
  
  /* Remove restrictions */
  (void)mon_restrict('\0', 0, &dummy, FALSE);

  /* Make some residents */
  for (i = 0; i < residents; i++)
    {
      /* Make a resident */
      (void)alloc_monster(3, TRUE, FALSE);
    }
}





/**
 * Generate a random dungeon level
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 *
 * Note that this function resets flow data and grid flags directly.
 * Note that this function does not reset features, monsters, or objects.  
 * Features are left to the town and dungeon generation functions, and 
 * wipe_m_list() and wipe_o_list() handle monsters and objects.
 */
void generate_cave(void)
{
  int y, x, num;
  
  /* The dungeon is not ready */
  character_dungeon = FALSE;
  
  /* Don't know feeling yet */
  do_feeling = FALSE;
  
  /* Assume level is not themed. */
  p_ptr->themed_level = 0;
  
  /* Generate */
  for (num = 0; TRUE; num++)
    {
      bool okay = TRUE;
      cptr why = NULL;
      
      /* Reset monsters and objects */
      o_max = 1;
      m_max = 1;
      
      
      /* Clear flags and flow information. */
      for (y = 0; y < DUNGEON_HGT; y++)
	{
	  for (x = 0; x < DUNGEON_WID; x++)
	    {
	      /* No flags */
	      cave_info[y][x] = 0;
	      cave_info2[y][x] = 0;
	      
#ifdef MONSTER_FLOW
	      /* No flow */
	      cave_cost[y][x] = 0;
	      cave_when[y][x] = 0;
#endif /* MONSTER_FLOW */
	      
	    }
	}
      
      
      /* Mega-Hack -- no player in dungeon yet */
      cave_m_idx[p_ptr->py][p_ptr->px] = 0;
      p_ptr->px = p_ptr->py = 0;
      
      /* Reset the monster generation level */
      monster_level = p_ptr->depth;
      
      /* Reset the object generation level */
      object_level = p_ptr->depth;
      
      /* Nothing special here yet */
      good_item_flag = FALSE;
      
      /* Nothing good here yet */
      rating = 0;
      
      /* Only group is the player */
      group_id = 1;
      
      /* Set the number of wilderness "vaults" */
      wild_vaults = 0;
      if (p_ptr->depth > 10) wild_vaults += rand_int(2);
      if (p_ptr->depth > 20) wild_vaults += rand_int(2);
      if (p_ptr->depth > 30) wild_vaults += rand_int(2);
      if (p_ptr->depth > 40) wild_vaults += rand_int(2);
      if (no_vault()) wild_vaults = 0;

      /* Build the town */
      if (!p_ptr->depth)
	{
	  /* Make a town */
	  town_gen();
	}
      
      /* Not town */
      else
	{
	  /* It is possible for levels to be themed. */
	  if ((rand_int(THEMED_LEVEL_CHANCE) == 0) && build_themed_level())
	    {
	      /* Message. */
	      if (cheat_room) msg_print("Themed level");
	    }
	  
	  /* Build a real stage */
	  else
	    {
	      switch(stage_map[p_ptr->stage][STAGE_TYPE])
		{
		case CAVE:
		  {
		    cave_gen();
		    break;
		  }
		  
		case VALLEY:
		  {
		    valley_gen();
		    break;
		  }
		  
		case MOUNTAIN:
		  {
		    mtn_gen();
		    break;
		  }
		  
		case MOUNTAINTOP:
		  {
		    mtntop_gen();
		    break;
		  }
		  
		case FOREST:
		  {
		    forest_gen();
		    break;
		  }
		  
		case SWAMP:
		  {
		    swamp_gen();
		    break;
		  }
		  
		case RIVER:
		  {
		    river_gen();
		    break;
		  }
		  
		case DESERT:
		  {
		    desert_gen();
		    break;
		  }
		  
		case PLAIN:
		  {
		    plain_gen();
		  }
		}
	    }
	}
      
      okay = TRUE;
      
      
      /* Extract the feeling */
      if      (rating > 50 +     p_ptr->depth    ) feeling = 2;
      else if (rating > 40 + 4 * p_ptr->depth / 5) feeling = 3;
      else if (rating > 30 + 3 * p_ptr->depth / 5) feeling = 4;
      else if (rating > 20 + 2 * p_ptr->depth / 5) feeling = 5;
      else if (rating > 15 + 1 * p_ptr->depth / 3) feeling = 6;
      else if (rating > 10 + 1 * p_ptr->depth / 5) feeling = 7;
      else if (rating >  5 + 1 * p_ptr->depth /10) feeling = 8;
      else if (rating >  0) feeling = 9;
      else feeling = 10;
      
      /* Hack -- no feeling in the town */
      if (!p_ptr->depth) feeling = 0;
      
      
      /* Prevent object over-flow */
      if (o_max >= z_info->o_max)
	{
	  /* Message */
	  why = "too many objects";
	  
	  /* Message */
	  okay = FALSE;
	}
      
      /* Prevent monster over-flow */
      if (m_max >= z_info->m_max)
	{
	  /* Message */
	  why = "too many monsters";
	  
	  /* Message */
	  okay = FALSE;
	}
      
      /* Mega-Hack -- "auto-scum" */
      if (auto_scum && (num < 100) && !(p_ptr->themed_level))
	{
	  int fudge = (no_vault() ? 3 : 0);

	  /* Require "goodness" */
	  if ((feeling > fudge + 9) ||
	      ((p_ptr->depth >= 5) && (feeling > fudge + 8)) ||
	      ((p_ptr->depth >= 10) && (feeling > fudge + 7)) ||
	      ((p_ptr->depth >= 20) && (feeling > fudge + 6)))
	    {
	      /* Give message to cheaters */
	      if (cheat_room || cheat_hear ||
		  cheat_peek || cheat_xtra)
		{
		  /* Message */
		  why = "boring level";
		}
	      
	      /* Try again */
	      okay = FALSE;
	    }
	}
      
      /* Message */
      if ((cheat_room) && (why)) 
	msg_format("Generation restarted (%s)", why);
      
      /* Accept */
      if (okay) break;
      
      /* Wipe the objects */
      wipe_o_list();
      
      /* Wipe the monsters */
      wipe_m_list();
      
      /* A themed level was generated */
      if (p_ptr->themed_level)
	{
	  /* Allow the themed level to be generated again */
	  p_ptr->themed_level_appeared &=
	    ~(1L << (p_ptr->themed_level - 1));
	  
	  /* This is not a themed level */
	  p_ptr->themed_level = 0;
	}
    }
  
  
  /* The dungeon is ready */
  character_dungeon = TRUE;
  
  /* Reset path_coord */
  p_ptr->path_coord = 0;
  
  /* Verify the panel */
  verify_panel();
  
  
  /* Reset the number of traps, runes, and thefts on the level. */
  num_trap_on_level = 0;
  number_of_thefts_on_level = 0;
  for (num = 0; num < MAX_RUNE; num++)
    num_runes_on_level[num] = 0;
}





