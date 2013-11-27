#ifndef INCLUDED_OBJECT_TVALSVAL_H
#define INCLUDED_OBJECT_TVALSVAL_H


/*** Object "tval" and "sval" codes ***/

/*
 * PS: to regenerate the inside of an sval enum, do this:
 * $ grep --context 2 'I\:TVAL\:' object.txt | grep ^[NI] |
 *   perl -pe 'y/[a-z]\- /[A-Z]__/' | perl -pe 's/(&_|~|)//g' | cut -d: -f3 |
 *   perl -00 -pe 's/([^\n]*)\n([^\n]*)\n/\tSV_\1\ =\ \2,\n/g'
 */

/*
 * The values for the "tval" field of various objects.
 *
 * This value is the primary means by which items are sorted in the
 * player inventory, followed by "sval" and "cost".
 *
 * Note that a "BOW" with tval = 19 and sval S = 10*N+P takes a missile
 * weapon with tval = 16+N, and does (xP) damage when so combined.  This
 * fact is not actually used in the source, but it kind of interesting.
 *
 * Note that as of 2.7.8, the "item flags" apply to all items, though
 * only armor and weapons and a few other items use any of these flags.
 */

#define TV_SKELETON      1	/* Skeletons ('`') */
#define TV_BOTTLE	 2	/* Empty bottles ('!') */
#define TV_JUNK          3	/* Sticks, Pottery, etc ('`'). */
#define TV_SPIKE         5	/* Spikes ('`') */
#define TV_CHEST         7	/* Chests ('~') */

#define TV_SHOT		16	/* Ammo for slings */
#define TV_ARROW        17	/* Ammo for bows */
#define TV_BOLT         18	/* Ammo for x-bows */
#define TV_BOW          19	/* Slings/Bows/Xbows */
#define TV_DIGGING      20	/* Shovels/Picks */
#define TV_HAFTED       21	/* Priest Weapons */
#define TV_POLEARM      22	/* Axes and Pikes */
#define TV_SWORD        23	/* Edged Weapons */
#define TV_BOOTS        30	/* Boots */
#define TV_GLOVES       31	/* Gloves */
#define TV_HELM         32	/* Helms */
#define TV_CROWN        33	/* Crowns */
#define TV_SHIELD       34	/* Shields */
#define TV_CLOAK        35	/* Cloaks */
#define TV_SOFT_ARMOR   36	/* Soft Armor */
#define TV_HARD_ARMOR   37	/* Hard Armor */
#define TV_DRAG_ARMOR	38	/* Dragon Scale Mail */

#define TV_LIGHT         39	/* Lights (including Specials) */
#define TV_AMULET       40	/* Amulets (including Specials) */
#define TV_RING         45	/* Rings (including Specials) */
#define TV_STAFF        55	/* Staffs */
#define TV_WAND         65	/* Wands */
#define TV_ROD          66	/* Rods */
#define TV_SCROLL       70	/* Scrolls */
#define TV_POTION       75	/* Potions */
#define TV_FLASK        77	/* Flasks */
#define TV_FOOD         80	/* Food and mushrooms */
#define TV_MAGIC_BOOK   90	/* Mage and Rogue books */
#define TV_PRAYER_BOOK  91	/* Priest and Paladin books */
#define TV_DRUID_BOOK	92	/* Druid and Ranger books */
#define TV_NECRO_BOOK	93	/* Necromancer and Assassin books */
#define TV_GOLD         100	/* Gold can only be picked up by players */

/* Special note:  the constant "TV_MAGIC_BOOK" is used as a base index in
 * all tables concerning player spells, so make certain all the tvals for
 * spell books follow each other in sequential order.  At some point, I
 * may replace it with the more flexible MAX_REALM. -LM-
 */

/* The "sval" codes for TV_CHEST.  -LM- */
#define SV_SW_CHEST		1	/*  Small wooden chest. */
#define SV_SI_CHEST		2	/*  Small iron chest. */
#define SV_SS_CHEST		3	/*  Small steel chest. */
#define SV_SJ_CHEST		4	/*  Small jeweled chest. */
#define SV_LW_CHEST		5	/*  Large wooden chest. */
#define SV_LI_CHEST		6	/*  Large iron chest. */
#define SV_LS_CHEST		7	/*  Large steel chest. */
#define SV_LJ_CHEST		8	/*  Large jeweled chest. */


/* The "sval" codes for TV_SHOT/TV_ARROW/TV_BOLT */
#define SV_AMMO_NORMAL		1	/* shots, arrows, bolts */
#define SV_AMMO_HEAVY		2	/* seeker shots, arrows, bolts */

/* The "sval" codes for TV_BOW (note information in "sval") */
#define SV_SLING		2	/* (x2) */
#define SV_SHORT_BOW		12	/* (x2) */
#define SV_LONG_BOW		13	/* (x3) */
#define SV_LIGHT_XBOW		23	/* (x3) */
#define SV_HEAVY_XBOW		24	/* (x4) */

enum sval_skeleton /* tval 1 */
{
        SV_BROKEN_SKULL = 1,
        SV_BROKEN_BONE = 2,
        SV_RODENT_SKELETON = 3,
        SV_CANINE_SKELETON = 4,
        SV_PETTY_DWARF_SKELETON = 5,
        SV_ELF_SKELETON = 6,
        SV_DWARF_SKELETON = 7,
        SV_HUMAN_SKELETON = 8,
};

enum sval_junk /* tval 3 */
{
        SV_SHARD_OF_POTTERY = 3,
        SV_BROKEN_STICK = 6,
};

enum sval_digging /* tval 20 */
{
        SV_SHOVEL = 1,
        SV_GNOMISH_SHOVEL = 2,
        SV_DWARVEN_SHOVEL = 3,
        SV_PICK = 4,
        SV_ORCISH_PICK = 5,
        SV_DWARVEN_PICK = 6
};

enum sval_hafted /* tval 21 */
{
        SV_MAGESTAFF = 1,
        SV_WHIP = 2,
        SV_QUARTERSTAFF = 3,
        SV_THROWING_HAMMER = 4,
        SV_MACE = 5,
        SV_BALL_AND_CHAIN = 6,
        SV_WAR_HAMMER = 8,
        SV_SPIKED_CLUB = 10,
        SV_MORNING_STAR = 12,
        SV_FLAIL = 13,
        SV_LEAD_FILLED_MACE = 15,
        SV_STORM_HAMMER = 17,
        SV_TWO_HANDED_FLAIL = 18,
        SV_MACE_OF_DISRUPTION = 20,
        SV_GROND = 50
};

enum sval_polearm /* tval 22 */
{
        SV_SPEAR = 2,
        SV_THRUSTING_SPEAR = 3,
        SV_TRIDENT = 5,
        SV_PIKE = 8,
        SV_BEAKED_AXE = 10,
        SV_BROAD_AXE = 11,
        SV_GLAIVE = 13,
        SV_THROWING_AXE = 14,
        SV_HALBERD = 15,
        SV_SCYTHE = 17,
        SV_LANCE = 20,
        SV_BATTLE_AXE = 22,
        SV_GREAT_AXE = 25,
        SV_DART = 28,
        SV_SCYTHE_OF_SLICING = 30
};

enum sval_sword /* tval 23 */
{
        SV_BROKEN_SWORD = 2,
        SV_DAGGER = 4,
        SV_MAIN_GAUCHE = 5,
        SV_RAPIER = 7,
        SV_SMALL_SWORD = 8,
        SV_SHORT_SWORD = 10,
        SV_SABRE = 11,
        SV_CUTLASS = 12,
        SV_FALCHION = 15,
        SV_BROAD_SWORD = 16,
        SV_LONG_SWORD = 17,
        SV_SCIMITAR = 18,
        SV_KATANA = 20,
        SV_BASTARD_SWORD = 21,
        SV_TWO_HANDED_SWORD = 25,
        SV_EXECUTIONERS_SWORD = 28,
        SV_VALINOREAN_SWORD = 30
};

enum sval_boots /* tval 30 */
{
        SV_PAIR_OF_LEATHER_SANDALS = 1,
        SV_PAIR_OF_SOFT_LEATHER_BOOTS = 2,
        SV_PAIR_OF_HARD_LEATHER_BOOTS = 3,
        SV_PAIR_OF_METAL_SHOD_BOOTS = 4,
        SV_PAIR_OF_DWARVEN_BOOTS = 5
};

enum sval_gloves /* tval 31 */
{
        SV_SET_OF_LEATHER_GLOVES = 1,
        SV_SET_OF_MAIL_GAUNTLETS = 2,
        SV_SET_OF_STEEL_GAUNTLETS = 3,
        SV_SET_OF_CESTI = 4
};

enum sval_helm /* tval 32 */
{
        SV_HARD_LEATHER_CAP = 2,
        SV_METAL_CAP = 3,
        SV_BARBUT = 4,
        SV_IRON_HELM = 5,
        SV_STEEL_HELM = 6
};

enum sval_crown /* tval 33 */
{
        SV_IRON_CROWN = 10,
        SV_SILVER_CROWN = 11,
        SV_JEWEL_ENCRUSTED_CROWN = 12,
        SV_MORGOTH = 50
};

enum sval_shield /* tval 34 */
{
        SV_WICKER_SHIELD = 1,
        SV_SMALL_LEATHER_SHIELD = 2,
        SV_SMALL_METAL_SHIELD = 3,
        SV_LARGE_LEATHER_SHIELD = 4,
        SV_LARGE_METAL_SHIELD = 5,
        SV_KNIGHTS_SHIELD = 6,
        SV_BODY_SHIELD = 7,
        SV_SHIELD_OF_DEFLECTION = 10
};

enum sval_cloak /* tval 35 */
{
        SV_CLOAK = 1,
        SV_ELVEN_CLOAK = 2,
        SV_SHADOW_CLOAK = 4,
        SV_ETHEREAL_CLOAK = 6,
        SV_UNLIGHT_CLOAK = 10
};

enum sval_soft_armour /* tval 36 */
{
        SV_FILTHY_RAG = 1,
        SV_ROBE = 2,
        SV_SOFT_LEATHER_ARMOR = 4,
        SV_SOFT_STUDDED_LEATHER = 5,
        SV_HARD_LEATHER_ARMOR = 6,
        SV_HARD_STUDDED_LEATHER = 7,
        SV_LEATHER_SCALE_MAIL = 11
};

enum sval_hard_armour /* tval 37 */
{
        SV_RUSTY_CHAIN_MAIL = 1,
        SV_METAL_SCALE_MAIL = 3,
        SV_CHAIN_MAIL = 4,
        SV_AUGMENTED_CHAIN_MAIL = 6,
        SV_DOUBLE_CHAIN_MAIL = 7,
        SV_BAR_CHAIN_MAIL = 8,
        SV_METAL_BRIGANDINE_ARMOUR = 9,
        SV_PARTIAL_PLATE_ARMOUR = 12,
        SV_METAL_LAMELLAR_ARMOUR = 13,
        SV_FULL_PLATE_ARMOUR = 15,
        SV_RIBBED_PLATE_ARMOUR = 18,
        SV_MITHRIL_CHAIN_MAIL = 20,
        SV_MITHRIL_PLATE_MAIL = 25,
        SV_ADAMANTITE_PLATE_MAIL = 30
};

enum sval_dragon_armour /* tval 38 */
{
        SV_DRAGON_BLACK = 1,
        SV_DRAGON_BLUE = 2,
        SV_DRAGON_WHITE = 3,
        SV_DRAGON_RED = 4,
        SV_DRAGON_GREEN = 5,
        SV_DRAGON_MULTIHUED = 6,
        SV_DRAGON_SHINING = 10,
        SV_DRAGON_LAW = 12,
        SV_DRAGON_BRONZE = 14,
        SV_DRAGON_GOLD = 16,
        SV_DRAGON_CHAOS = 18,
        SV_DRAGON_BALANCE = 20,
        SV_DRAGON_POWER = 30
};

enum sval_light /* tval 39 */
{
        SV_LIGHT_TORCH = 0,
        SV_LIGHT_LANTERN = 1,
        SV_LAMP = 4,
        SV_PEARL = 5,
        SV_PALANTIR = 6,
        SV_STONE_LORE = 15
};

enum sval_amulet /* tval 40 */
{
        SV_AMULET_CHALCEDONY = 0,
        SV_AMULET_MALACHITE = 1,
        SV_AMULET_OPAL = 2,
        SV_AMULET_GARNET = 3,
        SV_AMULET_PEARL = 4,
        SV_AMULET_AMETHYST = 5,
        SV_AMULET_SAPPHIRE = 6,
        SV_AMULET_RUBY = 7,
        SV_AMULET_EMERALD = 8,
        SV_AMULET_DIAMOND = 9,
        SV_AMULET_NECKLACE = 12,
        SV_AMULET_ELFSTONE = 14
};

enum sval_ring /* tval 45 */
{
        SV_RING_STEEL = 0,
        SV_RING_PEWTER = 1,
        SV_RING_GALVORN = 2,
        SV_RING_BRASS = 3,
        SV_RING_BRONZE = 4,
        SV_RING_COPPER = 5,
        SV_RING_SILVER = 6,
        SV_RING_GOLD = 7,
        SV_RING_ADAMANTITE = 8,
        SV_RING_MITHRIL = 9
};

enum sval_food /* tval 80 */
{
        SV_FOOD_BLINDNESS = 1,
        SV_FOOD_PARANOIA = 2,
        SV_FOOD_CONFUSION = 3,
        SV_FOOD_HALLUCINATION = 4,
        SV_FOOD_CURE_POISON = 12,
        SV_FOOD_CURE_BLINDNESS = 13,
        SV_FOOD_CURE_PARANOIA = 14,
        SV_FOOD_CURE_CONFUSION = 15,
        SV_FOOD_WEAKNESS = 6,
        SV_FOOD_UNHEALTH = 10,
        SV_FOOD_RESTORE_CON = 18,
        SV_FOOD_RESTORING = 19,
        SV_FOOD_STUPIDITY = 8,
        SV_FOOD_NAIVETY = 9,
        SV_FOOD_POISON = 0,
        SV_FOOD_SICKNESS = 7,
        SV_FOOD_PARALYSIS = 5,
        SV_FOOD_RESTORE_STR = 17,
        SV_FOOD_DISEASE = 11,
        SV_FOOD_CURE_SERIOUS = 16,
        SV_FOOD_RATION = 35,
        SV_FOOD_BISCUIT = 32,
        SV_FOOD_JERKY = 33,
        SV_FOOD_SLIME_MOLD = 36,
        SV_FOOD_WAYBREAD = 37,
        SV_FOOD_PINT_OF_ALE = 38,
        SV_FOOD_PINT_OF_WINE = 39,
        SV_FOOD_ATHELAS = 40,
        SV_FOOD_BEORNING = 41
};

enum sval_wand /* tval 65 */
{
        SV_WAND_HEAL_MONSTER = 0,
        SV_WAND_HASTE_MONSTER = 1,
        SV_WAND_CLONE_MONSTER = 2,
        SV_WAND_TELEPORT_AWAY = 3,
        SV_WAND_DISARMING = 4,
        SV_WAND_DOOR_DEST = 5,
        SV_WAND_STONE_TO_MUD = 6,
        SV_WAND_LIGHT = 7,
        SV_WAND_SLEEP_MONSTER = 8,
        SV_WAND_SLOW_MONSTER = 9,
        SV_WAND_CONFUSE_MONSTER = 10,
        SV_WAND_FEAR_MONSTER = 11,
        SV_WAND_DRAIN_LIFE = 12,
        SV_WAND_POLYMORPH = 13,
        SV_WAND_STINKING_CLOUD = 14,
        SV_WAND_MAGIC_MISSILE = 15,
        SV_WAND_ACID_BOLT = 16,
        SV_WAND_ELEC_BOLT = 17,
        SV_WAND_FIRE_BOLT = 18,
        SV_WAND_COLD_BOLT = 19,
        SV_WAND_ACID_BALL = 20,
        SV_WAND_ELEC_BALL = 21,
        SV_WAND_FIRE_BALL = 22,
        SV_WAND_COLD_BALL = 23,
        SV_WAND_WONDER = 24,
        SV_WAND_ANNIHILATION = 25,
        SV_WAND_DRAGON_FIRE = 26,
        SV_WAND_DRAGON_COLD = 27,
        SV_WAND_DRAGON_BREATH = 28,
        SV_WAND_STRIKING = 29,
        SV_WAND_STORMS = 30,
        SV_WAND_SHARD_BOLT = 31,
        SV_WAND_ILKORIN = 40,
        SV_WAND_SARUMAN = 41,
        SV_WAND_UNMAKING = 42,
        SV_WAND_ULPION = 43
};

enum sval_staff /* tval 55 */
{
        SV_STAFF_DARKNESS = 0,
        SV_STAFF_SLOWNESS = 1,
        SV_STAFF_HASTE_MONSTERS = 2,
        SV_STAFF_SUMMONING = 3,
        SV_STAFF_TELEPORTATION = 4,
        SV_STAFF_IDENTIFY = 5,
        SV_STAFF_REMOVE_CURSE = 6,
        SV_STAFF_STARLIGHT = 7,
        SV_STAFF_LIGHT = 8,
        SV_STAFF_MAPPING = 9,
        SV_STAFF_DETECT_GOLD = 10,
        SV_STAFF_DETECT_ITEM = 11,
        SV_STAFF_DETECT_TRAP = 12,
        SV_STAFF_DETECT_DOOR = 13,
        SV_STAFF_DETECT_INVIS = 14,
        SV_STAFF_DETECT_EVIL = 15,
        SV_STAFF_CURE_MEDIUM = 16,
        SV_STAFF_CURING = 17,
        SV_STAFF_HEALING = 18,
        SV_STAFF_BANISHMENT = 19,
        SV_STAFF_SLEEP_MONSTERS = 20,
        SV_STAFF_SLOW_MONSTERS = 21,
        SV_STAFF_SPEED = 22,
        SV_STAFF_PROBING = 23,
        SV_STAFF_DISPEL_EVIL = 24,
        SV_STAFF_POWER = 25,
        SV_STAFF_HOLINESS = 26,
        SV_STAFF_EARTHQUAKES = 28,
        SV_STAFF_DESTRUCTION = 29,
        SV_STAFF_DETECTION = 30,
        SV_STAFF_MSTORM = 31,
        SV_STAFF_STARBURST = 32,
        SV_STAFF_MASS_CONFU = 33,
        SV_STAFF_GANDALF = 40,
        SV_STAFF_WINDS = 41,
        SV_STAFF_MENELTARMA = 42,
        SV_STAFF_RADAGAST = 43
};

enum sval_rod /* tval 66 */
{
        SV_ROD_DETECT_TRAP = 0,
        SV_ROD_DETECT_DOOR = 1,
        SV_ROD_IDENTIFY = 2,
        SV_ROD_RECALL = 3,
        SV_ROD_ILLUMINATION = 4,
        SV_ROD_MAPPING = 5,
        SV_ROD_PROBING = 7,
        SV_ROD_CURING = 8,
        SV_ROD_HEALING = 9,
        SV_ROD_RESTORATION = 10,
        SV_ROD_SPEED = 11,
        SV_ROD_TELEPORT_AWAY = 13,
        SV_ROD_DISARMING = 14,
        SV_ROD_LIGHT = 15,
        SV_ROD_SLEEP_MONSTER = 16,
        SV_ROD_SLOW_MONSTER = 17,
        SV_ROD_DRAIN_LIFE = 18,
        SV_ROD_POLYMORPH = 19,
        SV_ROD_ACID_BOLT = 20,
        SV_ROD_ELEC_BOLT = 21,
        SV_ROD_FIRE_BOLT = 22,
        SV_ROD_COLD_BOLT = 23,
        SV_ROD_ACID_BALL = 24,
        SV_ROD_ELEC_BALL = 25,
        SV_ROD_FIRE_BALL = 26,
        SV_ROD_COLD_BALL = 27,
        SV_ROD_LIGHTINGSTRIKE = 28,
        SV_ROD_NORTHWINDS = 29,
        SV_ROD_DRAGONFIRE = 30,
        SV_ROD_GLAURUNGS = 31,
        SV_ROD_ROUSE_LEVEL = 32,
        SV_ROD_DELVING = 40,
        SV_ROD_SHADOW = 41,
        SV_ROD_AIR = 42,
        SV_ROD_PORTALS = 43
};

enum sval_gold /* tval 100 */
{
        SV_COPPER = 1,
        SV_SILVER = 2,
        SV_GARNETS = 3,
        SV_GOLD = 4,
        SV_OPALS = 5,
        SV_SAPPHIRES = 6,
        SV_RUBIES = 7,
        SV_DIAMONDS = 8,
        SV_EMERALDS = 9,
        SV_MITHRIL = 10,
        SV_ADAMANTITE = 11
};

enum sval_scroll /* tval 70 */
{
        SV_SCROLL_DARKNESS = 0,
        SV_SCROLL_AGGRAVATE_MONSTER = 1,
        SV_SCROLL_CURSE_ARMOR = 2,
        SV_SCROLL_CURSE_WEAPON = 3,
        SV_SCROLL_SUMMON_MONSTER = 4,
        SV_SCROLL_SUMMON_UNDEAD = 5,
        SV_SCROLL_TRAP_CREATION = 7,
        SV_SCROLL_PHASE_DOOR = 8,
        SV_SCROLL_TELEPORT = 9,
        SV_SCROLL_TELEPORT_LEVEL = 10,
        SV_SCROLL_WORD_OF_RECALL = 11,
        SV_SCROLL_IDENTIFY = 12,
        SV_SCROLL_REVEAL_CURSES = 13,
        SV_SCROLL_REMOVE_CURSE = 14,
        SV_SCROLL_STAR_REMOVE_CURSE = 15,
        SV_SCROLL_ENCHANT_ARMOR = 16,
        SV_SCROLL_ENCHANT_WEAPON_TO_HIT = 17,
        SV_SCROLL_ENCHANT_WEAPON_TO_DAM = 18,
        SV_SCROLL_STAR_RECHARGING = 19,
        SV_SCROLL_STAR_ENCHANT_ARMOR = 20,
        SV_SCROLL_STAR_ENCHANT_WEAPON = 21,
        SV_SCROLL_RECHARGING = 22,
        SV_SCROLL_BRANDING = 23,
        SV_SCROLL_LIGHT = 24,
        SV_SCROLL_MAPPING = 25,
        SV_SCROLL_DETECT_GOLD = 26,
        SV_SCROLL_DETECT_ITEM = 27,
        SV_SCROLL_DETECT_TRAP = 28,
        SV_SCROLL_DETECT_DOOR = 29,
        SV_SCROLL_DETECT_INVIS = 30,
        SV_SCROLL_SATISFY_HUNGER = 32,
        SV_SCROLL_BLESSING = 33,
        SV_SCROLL_HOLY_CHANT = 34,
        SV_SCROLL_HOLY_PRAYER = 35,
        SV_SCROLL_MONSTER_CONFUSION = 36,
        SV_SCROLL_PROTECTION_FROM_EVIL = 37,
        SV_SCROLL_RUNE_OF_PROTECTION = 38,
        SV_SCROLL_DOOR_DESTRUCTION = 39,
        SV_SCROLL_FRIGHTENING = 40,
        SV_SCROLL_STAR_DESTRUCTION = 41,
        SV_SCROLL_DISPEL_UNDEAD = 42,
        SV_SCROLL_GENOCIDE = 44,
        SV_SCROLL_MASS_GENOCIDE = 45,
        SV_SCROLL_ACQUIREMENT = 46,
        SV_SCROLL_STAR_ACQUIREMENT = 47,
        SV_SCROLL_ELE_ATTACKS = 48,
        SV_SCROLL_ACID_PROOF = 49,
        SV_SCROLL_ELEC_PROOF = 50,
        SV_SCROLL_FIRE_PROOF = 51,
        SV_SCROLL_COLD_PROOF = 52
};

enum sval_potion /* tval 75 */
{
        SV_POTION_WATER = 0,
        SV_POTION_APPLE_JUICE = 1,
        SV_POTION_SLIME_MOLD = 2,
        SV_POTION_SLOWNESS = 4,
        SV_POTION_SALT_WATER = 5,
        SV_POTION_POISON = 6,
        SV_POTION_BLINDNESS = 7,
        SV_POTION_CONFUSION = 9,
        SV_POTION_SLEEP = 11,
        SV_POTION_LOSE_MEMORIES = 13,
        SV_POTION_RUINATION = 15,
        SV_POTION_DEC_STR = 16,
        SV_POTION_DEC_INT = 17,
        SV_POTION_DEC_WIS = 18,
        SV_POTION_DEC_DEX = 19,
        SV_POTION_DEC_CON = 20,
        SV_POTION_DEC_CHR = 21,
        SV_POTION_DETONATIONS = 22,
        SV_POTION_DEATH = 23,
        SV_POTION_INFRAVISION = 24,
        SV_POTION_DETECT_INVIS = 25,
        SV_POTION_SLOW_POISON = 26,
        SV_POTION_CURE_POISON = 27,
        SV_POTION_BOLDNESS = 28,
        SV_POTION_SPEED = 29,
        SV_POTION_RESIST_HEAT_COLD = 30,
        SV_POTION_RESIST_ACID_ELEC = 31,
        SV_POTION_HEROISM = 32,
        SV_POTION_BERSERK_STR = 33,
        SV_POTION_CURE_LIGHT = 34,
        SV_POTION_CURE_SERIOUS = 35,
        SV_POTION_CURE_CRITICAL = 36,
        SV_POTION_HEALING = 37,
        SV_POTION_STAR_HEALING = 38,
        SV_POTION_LIFE = 39,
        SV_POTION_RESTORE_MANA = 40,
        SV_POTION_RESTORE_EXP = 41,
        SV_POTION_RES_STR = 42,
        SV_POTION_RES_INT = 43,
        SV_POTION_RES_WIS = 44,
        SV_POTION_RES_DEX = 45,
        SV_POTION_RES_CON = 46,
        SV_POTION_RES_CHR = 47,
        SV_POTION_INC_STR = 48,
        SV_POTION_INC_INT = 49,
        SV_POTION_INC_WIS = 50,
        SV_POTION_INC_DEX = 51,
        SV_POTION_INC_CON = 52,
        SV_POTION_INC_CHR = 53,
        SV_POTION_RESIST_ALL = 54,
        SV_POTION_AUGMENTATION = 55,
        SV_POTION_ENLIGHTENMENT = 56,
        SV_POTION_STAR_ENLIGHTENMENT = 57,
        SV_POTION_SELF_KNOWLEDGE = 58,
        SV_POTION_EXPERIENCE = 59,
        SV_POTION_VAMPIRE = 60,
        SV_POTION_STAR_INC_STR = 61,
        SV_POTION_STAR_INC_INT = 62,
        SV_POTION_STAR_INC_WIS = 63,
        SV_POTION_STAR_INC_DEX = 64,
        SV_POTION_STAR_INC_CON = 65,
        SV_POTION_STAR_INC_CHR = 66
};

/**
 * Special "sval" limit -- first "normal" food (no longer requires a flavor)
 */
#define SV_FOOD_MIN_FOOD        32

/**
 * Maximum base AC of current shield types (used in deflecting missiles and bolts)
 */
#define MAX_SHIELD_BASE_AC	12


/**
 * Special "sval" limit -- first "aimed" rod
 */
#define SV_ROD_MIN_DIRECTION	12

/**
 * Special "sval" limit -- first "large" chest
 */
#define SV_CHEST_MIN_LARGE	5

/**
 * Special "sval" limit -- first "good" magic/prayer book
 */
#define SV_BOOK_MIN_GOOD	4

/**
 * Special "sval" limit -- maximum allowed spellbook sval (used to prevent 
 * illegal access to tables)
 */
#define SV_BOOK_MAX		8

/**
 * Special "sval" limit -- max treasure sval (used in function "make_gold")
 */
#define SV_GOLD_MAX		11


/**
 * Special "sval" value -- unknown "sval"
 */
#define SV_UNKNOWN			255

/*** Artifact indexes (see "lib/edit/a_info.txt") ***/

#define ART_MORGOTH		51
#define ART_UNGOLIANT           74
#define ART_GROND		158




/*** Ego-Item indexes (see "lib/edit/e_info.txt") ***/


/* Weapons */
#define EGO_DORIATH		65
#define EGO_GONDOLIN            67
#define EGO_NOLDOR		68
#define EGO_NOGROD              69

/* Bows */
#define EGO_ACCURACY		104
#define EGO_VELOCITY		105

/* Ammo */
#define EGO_ACIDIC		119	/* Added in Oangband. */
#define EGO_ELECT		120	/* Added in Oangband. */
#define EGO_FLAME		121
#define EGO_FROST		122
#define EGO_POISON		123	/* Added in Oangband. */
#define EGO_BACKBITING		125

/* Rings */
#define EGO_RING_ELEMENTS       128
#define EGO_RING_PHYSICAL       129
#define EGO_RING_COMBAT         130
#define EGO_RING_MOBILITY       131
#define EGO_RING_ARCANE_RES     132
#define EGO_RING_UTILITY        133
#define EGO_RING_BASIC_RES      134
#define EGO_RING_HINDRANCE      135
#define EGO_RING_DAWN           136
#define EGO_RING_SPEED          137
#define EGO_RING_WOE            138
#define EGO_RING_FICKLENESS     139
#define EGO_RING_POWER          140

/* Amulets */

#define EGO_AMULET_MENTAL       160
#define EGO_AMULET_DOOM         161
#define EGO_AMULET_BASIC_RES    162
#define EGO_AMULET_MAGIC_MAST   163
#define EGO_AMULET_CLARITY      164
#define EGO_AMULET_SHADOWS      165
#define EGO_AMULET_METAMORPH    166
#define EGO_AMULET_SUSTENANCE   167
#define EGO_AMULET_TRICKERY     168
#define EGO_AMULET_WEAPONMAST   169
#define EGO_AMULET_VITALITY     170
#define EGO_AMULET_INSIGHT      171


#endif
