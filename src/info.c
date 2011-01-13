/** \file info.c 
    \brief Object information


 * Tables containing object kind descriptions, extra numerical info.  
 * Detailed info on spells.  Text for extended object descriptions of 
 * various kinds, artifact and DSM activation and flag descriptions.  Self 
 * Knowledge.  Spell failure chance, if is OK to cast, extra info shown in 
 * books, and print spells of a given spellbook.
 *
 * Copyright (c) 1999-2009 Nick McConnell, Leon Marrick, Ben Harrison, 
 * James E. Wilson, Robert A. Koeneke
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



/**
 * General information about classes of objects. -LM-
 * 
 * Index is tval.
 */
char *obj_class_info[101] = 
{
  "",	"",	"",	"",	"",
  "",	"",	"Chests may have some really good treasures hidden inside, but can be perilous to open...",	"",	"",

  "",	"",	"",	"",	"",
  "",	"Sling ammo.",	"Bow ammo.",	"Crossbow ammo.",	"Missile launchers allow you to inflict damage from a distance without using magic.",
  
  "Diggers, especially heavy ones, are invaluable for forced entry and escape and can make a lucky miner rich.",	"Hafted weapons rely on blunt force to inflict damage.  Since they spill relatively little blood, priests much prefer to carry one.",	"Pole-mounted weapons are often cumbersome and may require two hands to wield, but some offer both a high degree of protection and powerful attacks.",	"The effectiveness of edged weapons depends on keen edges and sharp points.  They tend to be quite light and are easy to use, but some may not deal enough damage for your liking.",	"",
  "",	"",	"",	"",	"",
  
  "Footwear protects the feet only, but some rare items of this type have magics to render them fleet, light, or steady.",	"Your hands would benefit from protection too, but most magic users need to keep their fingers unencumbered or magically supple.",	"Many a blow will be struck upon your head, and protection here will certainly be helpful.  Some rare items may protect and enhance your mind.",	"Many a blow will be struck upon your head, and protection here will certainly be helpful.  Some rare items may protect and enhance your mind.",	"Shields can be worn on your arm, or on your back if you need both hands to use your weapon.  So protective can a shield be that it can reduce damage as much or more than body armour, and you can perhaps deflect physical missiles (even shards) or take advantage of opportunities to bash your foe if you have one on your arm.",
  "Experienced adventurers wrap a cloak around their body.  Some rare items of this type may allow you to slip silently around less alert enemies.",	"Some kind of body protection will become a necessity as you face ever more dangerous opponents; rare items of this type may hold many and varied protective magics.",	"Some kind of body protection will become a necessity as you face ever more dangerous opponents; rare items of this type may hold many and varied protective magics.",	"Armour made of dragon scales is rare indeed, and powerful dragon magics allow you to sometimes breathe even as great dragons do.",	"An adventurer who cannot see is jackal food.  The further away your illumination ends, the greater your chance to ready yourself for desperate combat.",
  
  "Amulets slip around your neck, and almost all have magics wondrous or perilous bound inside.",	"",	"",	"",	"",
  "You may wear a ring upon each of your two ring fingers, and benefit or suffer from the magics it contains.",	"",	"",	"",	"",
  
  "",	"",	"",	"",	"",
  "Staffs are heavy, and take up plenty of space in your backpack, but can hold a lot of sometimes powerful spells that affect large areas.  Staffs recharge easily and well.",	"",	"",	"",	"",
  
  "",	"",	"",	"",	"",
  "Wands hold a variety of spells, useful both in combat and for exploration.  Bolt spells in wands often beam, and ball spells affect large areas.  Once its charges are exhausted, a wand is useless until recharged.",	"The magics stored in rods never run out, given enough time between uses to recover.  Rods can do a lot of damage, but they affect only small areas.  Bolt spells in rods seldom or never beam.",	"",	"",	"",
  
  "One will often find sheets of parchment with magic spells.  They are easy to read, and are a warrior or paladin's best chance at making use of magic.",	"",	"",	"",	"",
  "Healers, alchemists, and sages create potions in vast quantities, and store them in small flasks of clear glass or gleaming crystal.  Once quaffed, a potion is guaranteed to work, but not every strange liquid was mixed for your benefit...",	"",	"",	"",	"",
  
  "Deep in the murky dungeons strange mushrooms grow, and monsters rout about sealed packets of food that their owners will never need again.",	"",	"",	"",	"",
  "",	"",	"",	"",	"",
  
  "A manual of sorcerous magics, bound in fine linen dyed deep red.",	"A shining gospel of piety in flawless white and gold.",	"A runed stone with earthly and natural magics chiseled in brown and rich greens.",	"A black tome of necromantic rituals, with shadows lying deep around it.",	"",
  "",	"",	"",	"",	"",
  
  "Small valuables and coins."
};


/**
 * Extra numerical information about magical devices. -LM-
 * 
 * Y-coordinate:  Dragon Scale Mails = 0,  
 *                Staffs = 1, Wands = 2, Rods = 3.
 * X-coordinate:  object's sval (can currently accept an sval of up to 49)
 */
char *obj_special_info[4][50] =
{
  /* Dragon Scale Mails. */
  {
    "",								/*  */
    "(It can breathe acid for 45-270 damage)",			/* Black */
    "(It can breathe lightning for 40-260 damage)",		/* Blue */
    "(It can breathe frost for 45-270 damage)",			/* White */
    "(It can breathe fire for 50-300 damage)",			/* Red */
    "(It can breathe poison for 45-270 damage)",			/* Green */
    "(It can breathe any of various elements for 60-360 damage)",	/* Multihued */
    "",	"",	"",						/*  */
    "(It can breathe light or darkness for 50-300 damage)",	/* Shining */
    "",								/*  */
    "(It can breathe sound or shards for 60-360 damage)",		/* Law */
    "",								/*  */
    "(It can breathe confusion for 40-260 damage)",		/* Bronze */
    "",								/*  */
    "(It can breathe sound for 40-260 damage)",			/* Gold */
    "",								/*  */
    "(It can breathe chaos or disenchantment for 55-330 damage)",	/* Chaos */
    "",								/*  */
    "(It can breathe sound, shards, chaos, or disenchantment for 60-360 damage)",	/* Balance */
    "",	"",	"",	"",	"",	"",	"",	"",	"",
    "(It can breathe just about anything for 75-450 damage)",	/* Power */
    "",	"",	"",	"",	"",	"",	"",	"",	"",	"",
    "",	"",	"",	"",	"",	"",	"",	"",	""
  },
  
  /* Staffs. */
  {
    "",					/* Darkness */
    "",					/* Slowness */
    "",					/* Haste Monsters */
    "",					/* Summoning */
    "",					/* Teleportation */
    "",					/* Perception */
    "",					/* Remove Curse */
    "(damage: 20 - 60+)",		/* Starlight */
    "",					/* Light */
    "",					/* Magic Mapping */
    "",					/* Treasure Location */
    "",					/* Object Location */
    "",					/* Trap Location */
    "",					/* Door/Stair Location */
    "",					/* Detect Invisible */
    "",					/* Detect Evil */
    "(heal: 10 - 30)",		/* Cure Medium Wounds */
    "",					/* Curing */
    "(heal: 300)",			/* Healing */
    "",					/* Banish Evil */
    "",					/* Sleep Monsters */
    "",					/* Slow Monsters */
    "",					/* Speed */
    "",					/* Probing */
    "(damage: 60)",			/* Dispel Evil */
    "(damage: 100)",			/* Power */
    "(damage: 120)",			/* Holiness */
    "",					/* Genocide */
    "",					/* Earthquakes */
    "",					/* *Destruction* */
    "",					/* Detection */
    "(damage: 125 - 200)",		/* Mana Storm */
    "(damage: 100 - 167)",		/* Starburst */
    "",					/* Mass Confusion */
    "",		/*  */
    "",		/*  */
    "",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    "(damage: 30 - 90+)",		/* Gandalf */
    "(damage: 100 - 200)",		/* Winds */
    "",					/* Meneltarma */
    "",					/* Radagast */
    "",		/*  */	"",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    ""		/*  */
  },
  
  /* Wands. */
  {
    "",		/* Heal Monster */
    "",		/* Haste Monster */
    "",		/* Clone Monster */
    "(distance: 55-80)",		/* Teleport Away */
    "(chance: 95%)",		/* Disarming */
    "",		/* Door Destruction */
    "",		/* Stone to Mud */
    "(damage: 4 - 20)",		/* Light */
    "",		/* Sleep Monster */
    "",		/* Slow Monster */
    "",		/* Confuse Monster */
    "",		/* Scare Monster */
    "(damage: 50 - 100)",		/* Drain Life */
    "",		/* Polymorph */
    "(damage: 12)",		/* Stinking Cloud */
    "(damage: 2 - 12)",		/* Magic Missile */
    "(damage: 5 - 80)",		/* Acid Bolts */
    "(damage: 3 - 48)",		/* Lightning Bolts */
    "(damage: 6 - 98)",		/* Fire Bolts */
    "(damage: 4 - 64)",		/* Cold Bolts */
    "(damage: 60 - 90)",		/* Acid Balls */
    "(damage: 40 - 70)",		/* Lightning Balls */
    "(damage: 70 - 100)",		/* Fire Balls */
    "(damage: 50 - 80)",		/* Cold Balls */
    "",					/* Wonder */
    "(damage: 100 - 300)",		/* Annihilation */
    "(damage: 160)",			/* Dragon's Flame */
    "(damage: 160)",			/* Dragon's Frost */
    "(damage: 180 - 210)",		/* Dragon's Breath */
    "(damage: 10 - 234)",		/* Striking */
    "(damage: 35 - 140)",		/* Storms */
    "(damage: 4 - 56)",		/* Shrapnel */
    "",		/*  */
    "",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    "(damage ~30 - 310.  hurt 30.)",/* Ilkorin */
    "",	/* Saruman */
    "(various chaotic effects)",	/* Unmaking */
    "((damage: 3 * plev + d100)",	/* Ulpion */
    "",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    ""		/*  */
  },
  
  /* Rods. */
  {
    "",				/* Trap Detection */
    "",				/* Door Detection */
    "",				/* Identify */
    "",				/* Recall */
    "",				/* Illumination */
    "",				/* Magic Mapping */
    "",				/*  */
    "",				/* Probing */
    "",				/* Curing */
    "(Heal: 500)",		/* Healing */
    "",				/* Restoration */
    "",				/* Speed */
    "",				/*  */
    "(distance: 45-60+)",				/* Teleport Away */
    "(chance: 95%)",		/* Disarming */
    "(damage: 4 - 20)",	/* Light */
    "",				/* Sleep Monster */
    "",				/* Slow Monster */
    "(damage: 45 - 120)",	/* Drain Life */
    "",				/* Polymorph */
    "(damage: 6 - 88)",	/* Acid Bolts */
    "(damage: 4 - 56)",	/* Lightning Bolts */
    "(damage: 7 - 104)",	/* Fire Bolts */
    "(damage: 5 - 72)",	/* Frost Bolts */
    "(damage: 60 - 100)",	/* Acid Balls */
    "(damage: 40 - 80)",	/* Lightning Balls */
    "(damage: 70 - 110)",	/* Fire Balls */
    "(damage: 50 - 90)",	/* Cold Balls */
    "(damage: 18 - 272)",	/* Lightning Strike */
    "(damage: 21 - 296)",	/* Northwinds */
    "(damage: 24 - 320)",	/* Dragonfire */
    "(damage: 27 - 344)",	/* Glaurung's Blood */
    "",				/* Aggravate Level */
    "",		/*  */	"",		/*  */	"",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    "",		/*  */
    "",				/* Delving */
    "",				/* Shadow */
    "",				/* Air */
    "",				/* Portals */
    "",		/*  */
    "",		/*  */	"",		/*  */	"",		/*  */
    "",		/*  */
    ""		/*  */
  }
};


/**
 * Prints "User's tips" for various spells.  User's tips only appear after 
 * a learnt spell is browsed. -LM-
 */
char *spell_tips[255] = 
{
  "Fires a bolt of fire.",	/* 0 - Fire Bolt */
  "Detects nearby monsters that are not invisible.",
  "Random minor displacement.",
  "Detects nearby traps, doors, and stairs.",
  "Permanently lights up the area lit by your light source or a room.",
  "Fires a ball of poison.",
  "Reduces cuts and poisoning.",
  "Short term increase in saving throw and armour class, and protection from blindness and confusion.",
  "Standard identification.",
  "Fires a bolt or beam of lightning.",
  "Attempts to confuse one monster.",	/* 10 - Confuse Monster */
  "Fetches an item from a distance.",
  "Attempts to put a monster to sleep.",
  "Random major displacement.",
  "Fires a line of weak light.  Affects light-hating creatures.",
  "Fires a beam of cold.",
  "Throws an item using mental strength.",
  "Fully feeds you.",
  "Detects nearby invisible monsters.",
  "Attempts to disarm a trap adjacent to you.",
  "Teleports a monster a short distance.",	/* 20 - Blink Monster */
  "Removes all poison, cuts and stunning.",
  "Detects nearby enchanted objects.",
  "Melts a wall square to floor.",
  "Minor recharging.",
  "Attempts to put all monster in line of sight to sleep.",
  "Fires a short-range beam of force.",
  "Fires a ball of fire.",
  "Turns rod, wand, or staff energy into mana.  Rods have little energy.",
  "Attempts to slow a monster down.",
  "Teleports an opponent away.",	/* 30 - Teleport Other */
  "Temporarily hasten yourself.",
  "Puts target creatures in Stasis.  They are completely paralyzed and invulnerable.",
  "Recovers mana much more rapidly than normal resting.  Put this spell in a keymap for best results.",
  "Temporary opposition to an element.  Cumulative with equipment resistances.",
  "Temporarily increases armour class by 50.",
  "Temporary opposition to the elements and poison.  Cumulative with equipment resistances.",
  "Long-duration haste spell.",
  "Greatly increases saving throw and armour class.  Perfect protection from blindness and confusion.",
  "Creates a barrier of doors around you.",
  "Creates a staircase nearby.  Random choice between up or down, except on quest levels.",   /* 40 - Stair Creation */
  "Immediately takes you to an adjacent level.",
  "Recalls you to the town, or to one of four recall points.",
  "Destroys almost all objects nearby, deletes ordinary monsters, and banishes uniques from the level.",
  "Controlled minor displacement.  Hint:  use the 'p' option when you enter targeting mode.",
  "Fires a bolt or beam of acid.",
  "Attempts to change a monster.",
  "Shakes the nearby area, randomly scrambling terrain.",
  "Very powerful attempt to confuse, slow, and then sleep all monsters.",
  "Fires a ball of light centered on you.",
  "Powerful recharging spell.",		/* 50 - Major Recharge */
  "Fires a large poison and acid ball.",
  "Fires a large ice ball.",
  "Fires a small but unresistible magic ball.",
  "Hits all monsters in line of sight with a blast of sound.",
  "Fires a large chaos ball.",
  "Fires an arc of force.",
  "Lays a rune which increases (player) or decreases (monster) elemental spells cast from it.  Maximum of four runes.",
  "Lays a rune which increases (player) or decreases (monster) the saving throw of one standing on it.  Maximum of four runes.",
  "Lays a rune which creates an earthquake when walked on.  Maximum of four runes.",
  "Lays a rune which contains a reserve of mana for the player.  Casting on an existing rune will increase the reserve.  Drains mana from monsters walking over it.  One rune only.",	/* 60 - Rune of Mana */
  "Lays a rune which monsters cannot pass over or be summoned on until broken.  Maximum of four runes.",
  "Lays a rune which increases (player) or decreases (monster) the spell power of one standing on it.  One rune only.",
  "Lays a rune which increases (player) or decreases (monster) the speed of one standing on it.  One rune only.",
  "Detects all nearby evil monsters, even invisible ones.",
  "Reduces cuts and heals you a little.",
  "Short-duration bonus to fighting ability and armour class.",
  "Removes any fear you currently feel.",
  "Permanently lights up the area lit by your light source or a room.",
  "Detects all nearby traps.",
  "Detects all nearby doors and stairs.", /* 70 - Detect Doors/Stairs */
  "Reduces the amount of poison in your system.",
  "Reduces cuts and heals you a moderate amount.",
  "Attempts to frighten one monster.",
  "Random medium-range displacement.",
  "Medium-duration bonus to fighting ability and armour class.",
  "Attempts to put all monsters next to you to sleep.",
  "Fully feeds you.",
  "Removes standard curses.",
  "Temporary opposition to fire and frost.  Cumulative with equipment resistances.",
  "Removes all poison from your body.",	/* 80 - Neutralize Poison */
  "Fires an orb of holy force that does extra damage to evil creatures.",
  "Temporary see invisible.",
  "Temporary protection from lesser evil creatures.",
  "Reduces cuts and heals you a large amount.",
  "Shakes the nearby area, randomly changing terrain.",
  "Maps the local area.",
  "Attempts to make all undead monsters in line of sight turn and run.",
  "Long-duration bonus to fighting ability and armour class.",
  "Dispels all undead in line of sight.",
  "A large amount of healing, eliminates cuts and stunning.",	/* 90 - Heal */
  "Dispels all evil monsters in line of sight.",
  "Temporarily increases armour class by 50.",
  "Places a glyph on the floor that monsters cannot pass over or be summoned on until broken.  Max of four glyphs on a level; disarm extras.",
  "Strong dispel evil, healing, and remove poisoning, fear, stunning, and cuts.",
  "Minor random displacement.",
  "Long-range random displacement.",
  "Teleports an opponent away.",
  "Immediately takes you to the next level up or down.",
  "Recalls you to the town, or to one of your recall points.",
  "Regenerates the level.",	/* 100 - Alter Reality */
  "Detects nearby monsters that are not invisible.",
  "Detects all nearby monsters, traps, doors, stairs, gold seams, and objects.",
  "Learns about a monster's attributes and resistances.",
  "Standard identification of an object.",
  "Permanently light up the entire level, except for vaults, and detect everything nearby.",
  "Teleports away all evil monsters in line of sight.",
  "An extremely strong healing spell.  Removes cuts and stuns.",
  "Full *identification* of any object.",
  "Restores all stats.",
  "Restores experience level.",	/* 110 - Remembrance */
  "Destroys all doors next to you.",
  "Medium strength recharging spell.",
  "Removes both normal and heavy curses.",
  "Attempts to disarm an adjacent trap.",
  "Attempts to put a monster in stasis or fully bar a door.",
  "Adds plusses to Skill and Deadliness to weapons or plusses to armour class to armour.",
  "Fires a large ball of light.",
  "Fires a beam of holy magic that does extra damage to evil creatures.",
  "Destroys almost all objects nearby, deletes ordinary monsters, and banishes uniques from the level.",
  "Strikes at the soul of a living monster.",	/* 120 - Annihilation */
  "Large ball of light centered on you, healing, and a powerful frighten monsters spell.",
  "Select a temporary elemental brand.  More brands become available as you gain levels.  Only one brand is available at a time.  For Paladins, only works on melee weapons.",
  "Temporary Slay Evil.  Acts like (and can replace) a standard Slay evil weapon.",
  "Heroism, frightens monsters, also a little healing and recovery of courage.",
  "The next time you hit or steal from a monster, you will teleport away.",
  "Renders you hasted and berserk.",
  "Detects nearby objects, treasure and gold seams.",
  "Detects all nearby living monsters, even invisible ones.",
  "Permanently lights up the area lit by your light source or a room.",
  "Fully feeds you.",	/* 130 - foraging */
  "Minor random displacement.",
  "Reduces the amount of poison in your system.",
  "Short-range beam of lightning.",
  "Destroys all doors next to you.",
  "Melts a wall square to floor.",
  "Fires a line of weak light.  Affects light-hating creatures.",
  "Removes all poison from your body.",
  "Fires a bolt or beam of frost.",
  "Attempts to put a monster to sleep.",
  "Attempts to frighten a monster.",	/* 140 - frighten creature */
  "Detects nearby traps, doors, and stairs.",
  "Kills all weak monsters in line of sight; spell has no effect on stronger creatures.",
  "Fires a bolt or beam of fire.",
  "Temporary heroism.",
  "Breaks ordinary curses.",
  "Fires a bolt or beam of acid.",
  "Teleports a monster away.",
  "Fires a bolt or beam of gravity.",
  "Temporary opposition to poison.  Cumulative with equipment resistances.",
  "Shakes the nearby area, randomly changing terrain.",	/* 150 - earthquake */
  "Temporary opposition to fire and cold.  Cumulative with equipment resistances.",
  "Detects all nearby monsters, traps, doors, stairs, gold seams, and objects.",
  "Recovers from wounds, poison and physical damage far more rapidly than normal resting.  This is a good spell to assign to a keymap.",
  "Temporary opposition to acid and electricity.  Cumulative with equipment resistances.",
  "Hurts a monster, then attempts to slow and confuse it.",
  "Attempts to disarm an adjacent trap.",
  "Standard identification of an object.",
  "Creates Athelas.",
  "Fires a large lightning ball.",
  "Fires a very large sound ball.",	/* 160 - thunderclap */
  "Turns you into a silent mouse.",
  "Turns you into a ferret, most crafty of animals.",
  "Turns you into a hound always aware of its surroundings.",
  "Turns you into a speedy gazelle.",
  "Turns you into a dauntless lion.",
  "Detects all nearby evil monsters, even invisible ones.",
  "Attempts (strongly) to frighten all monsters in line of sight.",
  "Maps the local area.",
  "Detects all nearby monsters, traps, doors, and stairs, and grants temporary see invisible.",
  "Healing, plus removal of all cuts and poison.",	/* 170 - herbal healing */
  "Fires a very large cold ball.",
  "Hurls water outwards from yourself in all directions.",
  "Fires a very large fire ball and trigger an earthquake.",
  "Fires a small, intense plasma ball.",
  "Fires a large ball of light centered on yourself.",
  "Attempts to slow and sleep all monsters in line of sight.",
  "Temporarily increases armour class by 50.",
  "Dispels all monsters in line of sight.  Extra damage to evil creatures.",
  "Places a glyph on the floor that monsters cannot pass over or be summoned on until broken.",
  "Restores all your stats.",	/* 180 - song of renewal */
  "Fires a bolt or beam of time.",
  "Temporarily hastens you.",
  "Recharges a magical item.",
  "Turns you into a wise and powerful Elder Ent.   Use potions or spells to protect yourself from fire!",
  "Restores your experience.",
  "Dispels evil monsters, heals you, blesses you, and removes fear, poisoning, stunning, and cuts.",
  "Low-level probing spell that lets you learn about racial type and basic resistances.",
  "Hurts any monster on grass or near trees.  Animals take less damage.",
  "Encourages trees and grass grow in your line of sight.",
  "Makes one object proof against acid and fire attacks.",	/* 190 - song of preservation */
  "Creates a small, targetable earthquake.",
  "Fires a bolt of nether force.",
  "Detects all nearby evil monsters, even invisible ones.",
  "Increases the range at which you can see warm-blooded creatures by 3.",
  "Removes ordinary curses.",
  "Attempts to slow a monster.",
  "Attempts to put a monster to sleep.",
  "Attempts to frighten a monster.",
  "Turns you into a fast-fluttering bat.",
  "Destroys all doors next to you.",	/* 200 - door destruction */
  "Fires a bolt or beam of darkness.",
  "Covers the immediate area with a cloud of poison.  Does not quickly lose strength with distance from the center.",
  "Attempts to make all undead monsters in line of sight flee.",
  "Attempts to make all evil monsters in line of sight flee.",
  "Removes all poison from your system.",
  "Attacks all undead creatures in line of sight.",
  "Attacks all evil creatures in line of sight.",
  "Grants temporary see invisible.",
  "Random minor displacement.",
  "Detects all nearby traps.",	/* 210 - detect traps */
  "Detects all nearby doors.",
  "Attempts to put all monsters in line of sight to sleep.",
  "Attempts to slow all monsters in line of sight.",
  "Detects nearby magical objects.",
  "Fires a bolt or beam of life-draining magic.",
  "Temporary opposition to poison.  Cumulative with equipment resistances.",
  "Attacks all demons in line of sight.",
  "Fires a beam of darkness.",
  "Fires a heavy bolt of chaos.",
  "Deletes all monsters of the symbol you choose from the level.",     /* 220 - genocide */
  "Fires a large ball of darkness.",
  "Dispels living creatures, attempts to confuse all survivors, and then blankets everything in a cloud of poison.",
  "Learns most attributes of all monsters in line of sight.",
  "Maps the immediate area.",
  "Normal identify spell.",
  "Random large-range displacement.",
  "Coats any non-ego missile or stack of missiles in deadly poison.",
  "Temporary opposition to cold and acid.  Cumulative with equipment resistances.",
  "Removes all cuts and stunning.",
  "Grants temporary protection from evil.",    /* 230 - protection from evil */
  "Temporary increase to armour class and fighting ability.",
  "Teleports away all evil monsters in line of sight.",
  "Increases armour class by 35 temporarily.",
  "Detects all nearby monsters.",
  "Fires a nether bolt.",
  "Fires a life-draining orb.",
  "Attacks all living creatures in line of sight.",
  "Fires a life-draining bolt, heals you, and increases your food by a set amount.",
  "Medium-strength recharging spell.",
  "Turns you into a raging werewolf.",	/* 240 - become werewolf */
  "Breaks both normal and heavy curses.",
  "Turns you into a fell vampire.",
  "Hastens you temporarily.",
  "Allows you to infect a monster with the Black Breath, which prevents regeneration.",
  "Destroys almost all objects nearby, deletes ordinary monsters, and banishes uniques from the level.",
  "Teleports a monster away.",
  "Attacks all undead in line of sight.",
  "Places all undead in a powerful Statis.",
  "Fires a large darkness ball.",
  "Grants temporary telepathy.",	/* 250 - timed ESP */
  "Renders you almost imperceptible to sleeping monsters and those not normally found deeper than twice your depth.  You become fully visible as soon as you inflict any damage on or steal from a monster.",
  "Hastens you and drives you into a berserk rage.",
  "Grants any non-artifact throwing weapon perfect balance.  Be careful about using this spell on weapons that have a hidden quality (use the 'I'nspect command).",
  "Turns 75 hitpoints into mana."
};

/**
 * Prints information about specialty abilties
 */
char *specialty_tips[TOTAL_SPECIALTIES]=
{
  "Increases the effects of body armor.",
  "Improves shield AC, chance of shield deflection, and damage from shield bashes.",
  "Reduces mana penalties for armor use.",
  "Gives a good chance to avoid melee attacks and a great chance to avoid missile weapons when lightly armored.",
  "Improves your saving throws.",
  "Causes you to speed up after teleporting.  Gives resistance to hostile teleportation.",
  "Increases AC and gives damage resistance in the dark.  Gives stealth and resist darkness.  Allows you to take actions (like reading) without light.  Reduces light radius.",
  "","","","","","","","","","","","","",
  "Grants extra critical hits with melee weapons.",
  "Gives you extra melee attacks; better when you have more attacks to begin with.  Works armed and unarmed.",
  "Grants extra critical hits in ranged combat.",
  "Gives your successful bow and crossbow attacks a chance of continuing to additional targets.",
  "Allows you to hurl throwing weapons and other objects great distances.",
  "Gives you a stronger barehanded attack.",
  "Gives you a powerful unarmed attack.",
  "Allows you to burn monster mana in melee.  Mana burned adds to melee damage.",
  "Trades some shooting accuracy for faster shooting with a short or long bow, even faster with increasing dexterity.",
  "","","","","","","","","","","",
  "Enhances your power to slow, sleep, confuse, or turn your enemies.",
  "Increases the effects and durations of beneficial magical effects.",
  "Increases spell casting speed, especially for low level spells from high level casters.",
  "Gives you mana when monsters cast spells.",
  "Causes spells cast in rapid succession to become increasingly powerful.",
  "Gives you mana when you slay monsters.  May cause damage when overloaded.",
  "Causes you to gain hit points when casting spells.",
  "",
  "Makes your spells stronger when your mana level is high.",
  "","","","","","","","","","","",
  "Increases and partially sustains Dexterity and Constitution.",
  "Increases and partially sustains Intelligence and Wisdom.",
  "",
  "Allows you to move faster after attacking or being damaged.",
  "Improves mana regeneration; cumulative with other forms of regeneration.",
  "Improves hit point regeneration; cumulative with other forms of regeneration.",
  "Allows you to set an additional monster trap, and to learn to set more advanced types of traps.",
  "Increases light radius.  Causes undead, evil, and light-sensitive creatures to take extra melee damage and makes them fear your light-based spells.  Grants resistance to light.",
  "","","","","","","","","","","","", 
  "","","","","","","","","","","","","","","","","","","","", /* 80 - 99 */
  "","","","","","","","","","","","","","","","","","","","", /* 100 - 119 */
  "","","","","","","","",                   /* 120 - 127 */
  "You are slightly more skilled with swords than other melee weapons.",
  "You are slightly more skilled with axes and polearms than other melee weapons.",
  "You are slightly more skilled with blunt weapons than other melee weapons.",
  "You are slightly more skilled with slings than other ranged weapons.",
  "You are slightly more skilled with bows than other ranged weapons.",
  "You are slightly more skilled with crossbows than other ranged weapons.",
  "","","",
  "You are slightly less skilled with swords than other melee weapons.",
  "You are slightly less skilled with axes and polearms than other melee weapons.",
  "You are slightly less skilled with blunt weapons than other melee weapons.",
  "You are slightly less skilled with slings than other ranged weapons.",
  "You are slightly less skilled with bows than other ranged weapons.",
  "You are slightly less skilled with crossbows than other ranged weapons.",
  "","","",
  "You recover quickly from poison and cuts, and succumb slowly to the Black Breath",
  "You eat.  A lot.",
  "You recover very quickly from negative effects, and can move through water without being slowed.",
  "You are incredibly stealthy.",
  "Your bare hands tear through stone and you are almost impossible to detect in forests, but you are vulnerable to fire and falling.",
  "You can take the form of a bear (use the ']' command).",
  "You are more able to hit monsters while in plain regions.",
  "You have an inborn ability to move freely through forests.",
  "You are able to move freely through rubble, and to evade attacks while in mountain regions.",
  "You are more able to hit monsters while in forests.","","","","",
  "","","","","","","","","","","","","","","","","","","","", /* 160 - 179 */
  "","","","","","","","","","","","",
  "Your shooting speed with a bow increases quickly with Dexterity.",
  "Your shooting speed with a bow increases very quickly with Dexterity.",
  "Your shooting speed with a sling increases quickly with Dexterity.",
  "Your shooting speed with a sling increases very quickly with Dexterity.",
  "Your shooting speed with a crossbow increases quickly with Dexterity.",
  "Your shooting speed with a crossbow increases very quickly with Dexterity.",
  "","","",
  "You effectively assassinate unsuspecting creatures in melee and with ranged attacks.",
  "You do extra damage when shooting.",
  "You effectively backstab unsuspecting creatures in melee.",
  "You can efficiently spread your attacks over multiple weak opponents at level 40.",
  "You are adept at fighting with your shield as well as your weapon.",
  "You are skilled in hand to hand combat, and are somewhat talented with slings.",
  "You fight better with blessed weapons (Paladin). You are not comfortable with edged melee weapons, unless they have been Blessed (Priest).",
  "",
  "You have a bonus to cause sleep, confusion, and slow.",
  "You have a great understanding of magical devices and can recharge them very reliably.",
  "You are an expert spellcaster and have a large reserve of mana.",
  "Your spells are more likely to 'beam', striking multiple targets.",
  "",
  "You learn quickly about monster abilities.",
  "You carry a pious authority.  You glow with an inner light at level 35 (Priest) or level 45 (Paladin).",
  "You resist fear intrinsically at level 30.  You regenerate hitpoints intrinsically at level 40.",
  "You can learn details of monster attributes at level 35.",
  "You deal with dark forces.  Beware!",
  "You can rob gold from living monsters.",
  "You obtain relatively specific pseudoidentify information.",
  "You can set monster traps.",
  "You are attuned to nature.  You easily move and hide in trees.",
  "You can learn an extra specialty abilty at level 1."
  "","","","","","","","","","","","","","","","",
  "","","","","","","","","","","","","","",""                 /* 240 - 255 */
};

/**
 * Output numerical values for magical device damages, healing, etc., for 
 * an item belonging to an object class whose effects are fully known.  The 
 * only way this information can appear is through *identifying*, or by 
 * eventual learning through use. -LM-
 */
static char *extra_data(object_type *o_ptr)
{
  byte tval_to_index;
  
  /* Boundary control. */
  if (o_ptr->sval > 49) return(NULL);
  
  /* Table can handle Dragon Scale Mails, */
  if (o_ptr->tval == TV_DRAG_ARMOR) tval_to_index = 0;
  
  /* Staffs, */
  else if (o_ptr->tval == TV_STAFF) tval_to_index = 1;
  
  /* Wands, */
  else if (o_ptr->tval == TV_WAND) tval_to_index = 2;
  
  /* and Rods... */
  else if (o_ptr->tval == TV_ROD) tval_to_index = 3;
  
  /* ...But nothing else. */
  else return(NULL);
  
  
  /* Locate the object in the table, and return the description. */
  return(format("%s", obj_special_info[tval_to_index][o_ptr->sval]));
}


#define CHECK_FIRST(txt, first) \
  if ((first)) { (first) = FALSE; text_out_to_screen(TERM_WHITE, (txt)); } \
  else text_out_to_screen(TERM_WHITE, ", ");

/**
 * Display the damage done with a multiplier
 */
extern void output_dam(object_type *o_ptr, int mult, const char *against, bool *first)
{
  int dam, die_average, add = 0, i, deadliness, crit, chance;

  /* Average damage for one standard side (x10) */
  die_average = (10 * (o_ptr->ds + 1)) / 2;

  /* Multiply by slay or brand (x10) */
  die_average *= mult;

  /* Record the addend */
  if (mult > MULTIPLE_BASE) add = (mult - MULTIPLE_BASE);

  /* Multiply by deadliness (x100) */
  deadliness = p_ptr->dis_to_d;
  if (object_known_p(o_ptr)) deadliness += o_ptr->to_d;
  if (deadliness >  150) deadliness =  150;
  if (deadliness < -150) deadliness = -150;
  if (deadliness >= 0)
    die_average *= (100 + deadliness_conversion[deadliness]);
  else
    {
      i = deadliness_conversion[ABS(deadliness)];
      if (i >= 100) die_average = 0;
      else die_average *= (100 - i);
    }

  /* Factor in criticals (x 10) */
  chance = p_ptr->skill_thn + p_ptr->dis_to_h;
  if (object_known_p(o_ptr)) chance += o_ptr->to_h;
  chance = (100 * chance) / (chance + 240);
  if (check_ability(SP_ARMSMAN)) chance = 100 - (83 * (100 - chance)) / 100;
  crit = 259 * chance;
  crit /= 1000;
  
  /* Multiply by number of sides */
  dam = die_average * (o_ptr->dd * 10 + crit);

  CHECK_FIRST("", *first);
  if ((dam > 50000) || (add > 0))
    text_out_to_screen(TERM_L_GREEN, format("%d", add + dam / 100000));
  else
    text_out_to_screen(TERM_L_RED, "0");
  text_out_to_screen(TERM_WHITE, format(" against %s", against));
}

/**
 * Outputs the damage we do/would do with the weapon
 */
extern void display_weapon_damage(object_type *o_ptr)
{
  object_type forge, *old_ptr = &forge;
  object_type *i_ptr;
  int i, j;
  
  bool first = TRUE;
  int show_m_tohit;
  int brand[MAX_P_BRAND], slay[MAX_P_SLAY];
  
  const char *slayee[] = { "animals", "evil creatures", "undead", "demons", 
			   "orcs", "trolls", "giants", "dragons" };
  const char *brandee[] = { "acid", "lightning", "fire", "cold", "poison"  };
  
  /* Extract the slays and brands */
  for (j = 0; j < MAX_P_SLAY; j++)
    slay[j] = (o_ptr->id_other & (OBJECT_ID_BASE_SLAY << j)) 
      ? o_ptr->multiple_slay[j] : MULTIPLE_BASE;
  for (j = 0; j < MAX_P_BRAND; j++)
    brand[j] = (o_ptr->id_other & (OBJECT_ID_BASE_BRAND << j)) 
      ? o_ptr->multiple_brand[j] : MULTIPLE_BASE;

  /* Check rings for additional brands (slays) */
  for (i = 0; i < 2; i++)
    {
      i_ptr = &inventory[INVEN_LEFT + i];
      
      /* If wearing a ring */
      if (i_ptr->k_idx)
	{
	  /* Pick up any brands (and slays!) */
	  for (j = 0; j < MAX_P_SLAY; j++)
	    slay[j] = MAX(slay[j],
			  (i_ptr->id_other & (OBJECT_ID_BASE_SLAY << j)) 
			  ? i_ptr->multiple_slay[j] : MULTIPLE_BASE);
	  for (j = 0; j < MAX_P_BRAND; j++)
	    brand[j] = MAX(brand[j], 
			   (i_ptr->id_other & (OBJECT_ID_BASE_BRAND << j)) 
			   ? i_ptr->multiple_brand[j] : MULTIPLE_BASE);
	}
    }

  /* temporary elemental brands */
  if (p_ptr->special_attack & (ATTACK_ACID)) 
    brand[P_BRAND_ACID] = MAX(brand[P_BRAND_ACID],BRAND_BOOST_NORMAL);
  if (p_ptr->special_attack & (ATTACK_ELEC)) 
    brand[P_BRAND_ELEC] = MAX(brand[P_BRAND_ELEC],BRAND_BOOST_NORMAL);
  if (p_ptr->special_attack & (ATTACK_FIRE)) 
    brand[P_BRAND_FIRE] = MAX(brand[P_BRAND_FIRE],BRAND_BOOST_NORMAL);
  if (p_ptr->special_attack & (ATTACK_COLD)) 
    brand[P_BRAND_COLD] = MAX(brand[P_BRAND_COLD],BRAND_BOOST_NORMAL);
  if (p_ptr->special_attack & (ATTACK_POIS)) 
    brand[P_BRAND_POIS] = MAX(brand[P_BRAND_POIS],BRAND_BOOST_NORMAL);
  if (p_ptr->special_attack & (ATTACK_HOLY)) 
    slay[P_SLAY_EVIL] = MAX(slay[P_SLAY_EVIL],15);

  /* Ok now the hackish stuff, we replace the current weapon with this one */
  object_copy(old_ptr, &inventory[INVEN_WIELD]);
  object_copy(&inventory[INVEN_WIELD], o_ptr);
  calc_bonuses(TRUE);
  show_m_tohit = p_ptr->dis_to_h;
  if (o_ptr->id_other & IF_TO_H) show_m_tohit += o_ptr->to_h;
  
  text_out_to_screen(TERM_WHITE, "\nWielding it you would have ");
  text_out_to_screen(TERM_L_GREEN, format("%d ", p_ptr->num_blow));
  text_out_to_screen(TERM_WHITE, 
		     format("blow%s and do an average damage per blow of ", 
			    (p_ptr->num_blow) ? "s" : ""));
  
  for (i = 0; i < MAX_P_SLAY; i++)
    {
      if (slay[i] > MULTIPLE_BASE)
	output_dam(o_ptr, slay[i], slayee[i], &first);
    }
  
  for (i = 0; i < MAX_P_BRAND; i++)
    {
      if (brand[i] > MULTIPLE_BASE)
	{
	  char buf[40];

	  strnfmt(buf, sizeof(buf), "non %s resistant creatures", brandee[i]);
	  output_dam(o_ptr, brand[i], buf, &first);
	}
    }

  output_dam(o_ptr, MULTIPLE_BASE, (first) ? "all monsters" : "other monsters",
	     &first);
  
  text_out_to_screen(TERM_WHITE, ".  Your + to Skill would be ");
  text_out_to_screen(TERM_L_GREEN, format("%d", show_m_tohit));
  text_out_to_screen(TERM_WHITE, ". ");
  
  /* get our weapon back */
  object_copy(&inventory[INVEN_WIELD], old_ptr);
  calc_bonuses(TRUE);
}

/**
 * Display the ammo damage done with a multiplier
 */
extern void output_ammo_dam(object_type *o_ptr, int mult, const char *against, 
			    bool *first, bool perfect)
{
  object_type *b_ptr = &inventory[INVEN_BOW];
  
  int dam, die_average, add = 0, i, deadliness, crit, chance, dice = o_ptr->dd;

  bool thrown;

  /* Throwing weapon, or launched missile? */
  thrown = !is_missile(o_ptr);

  /* Average damage for one standard side (x10) */
  die_average = (10 * (o_ptr->ds + 1)) / 2;

  /* Apply the launcher multiplier. */
  if (!thrown)
    die_average *= p_ptr->ammo_mult;
	  
  /* Multiply by slay or brand (x10) */
  die_average *= mult;

  /* Record the addend */
  if (mult > 10) add = (mult - 10);

  /* Multiply by deadliness (x100) */
  deadliness = p_ptr->dis_to_d;
  if (object_known_p(o_ptr)) deadliness += o_ptr->to_d;
  if (object_known_p(b_ptr)) deadliness += b_ptr->to_d;
  if (deadliness >  150) deadliness =  150;
  if (deadliness < -150) deadliness = -150;
  if (deadliness >= 0)
    die_average *= (100 + deadliness_conversion[deadliness]);
  else
    {
      i = deadliness_conversion[ABS(deadliness)];
      if (i >= 100) die_average = 0;
      else die_average *= (100 - i);
    }

  /* Get critical chance (x 10) */
  chance = p_ptr->skill_thb + p_ptr->dis_to_h;
  if (object_known_p(o_ptr)) chance += o_ptr->to_h;
  if ((!thrown) && (object_known_p(b_ptr))) chance += b_ptr->to_h;
  if (thrown) chance = chance * 3 / 2;
  chance = (100 * chance) / (chance + 360);
  if (check_ability(SP_MARKSMAN)) chance = 100 - (83 * (100 - chance)) / 100;
  crit = 116 * chance;
  crit /= 1000; 

  /* Increase dice */
  if (thrown && perfect) dice *= 2;
  dice = dice * 10 + crit;
  if (thrown) dice *= 2 + p_ptr->lev/12;

  /* Multiply by number of sides */
  dam = die_average * dice;

  CHECK_FIRST("", *first);
  if ((dam > 50000) || (add > 0))
    text_out_to_screen(TERM_L_GREEN, format("%d", add + dam / 100000));
  else
    text_out_to_screen(TERM_L_RED, "0");
  text_out_to_screen(TERM_WHITE, format(" against %s", against));
}

/**
 * Outputs the damage we do/would do with the current bow and this ammo
 */
extern void display_ammo_damage(object_type *o_ptr)
{
  int i;
  object_type *i_ptr;

  bool first = TRUE;
  bool perfect;
  
  int brand[MAX_P_BRAND], slay[MAX_P_SLAY];
  
  const char *slayee[] = { "animals", "evil creatures", "undead", "demons", 
			   "orcs", "trolls", "giants", "dragons" };
  const char *brandee[] = { "acid", "lightning", "fire", "cold", "poison"  };
  
  /* Extract the slays and brands */
  for (i = 0; i < MAX_P_SLAY; i++)
    slay[i] = (o_ptr->id_other & (OBJECT_ID_BASE_SLAY << i)) 
      ? o_ptr->multiple_slay[i] : MULTIPLE_BASE;
  for (i = 0; i < MAX_P_BRAND; i++)
    brand[i] = (o_ptr->id_other & (OBJECT_ID_BASE_BRAND << i)) 
      ? o_ptr->multiple_brand[i] : MULTIPLE_BASE;

  /* Hack -- paladins (and priests) cannot take advantage of 
  * temporary elemental brands to rescue their
  * lousy shooting skill.
  *
  * Missle weapons are "kind of" edged, right?
  */
  if (!check_ability(SP_BLESS_WEAPON) || (o_ptr->tval > TV_BOLT)) 
    {
      if (p_ptr->special_attack & (ATTACK_ACID)) 
        brand[P_BRAND_ACID] = MAX(brand[P_BRAND_ACID],BRAND_BOOST_NORMAL);
      if (p_ptr->special_attack & (ATTACK_ELEC)) 
        brand[P_BRAND_ELEC] = MAX(brand[P_BRAND_ELEC],BRAND_BOOST_NORMAL);
      if (p_ptr->special_attack & (ATTACK_FIRE)) 
        brand[P_BRAND_FIRE] = MAX(brand[P_BRAND_FIRE],BRAND_BOOST_NORMAL);
      if (p_ptr->special_attack & (ATTACK_COLD)) 
        brand[P_BRAND_COLD] = MAX(brand[P_BRAND_COLD],BRAND_BOOST_NORMAL);
      if (p_ptr->special_attack & (ATTACK_POIS)) 
        brand[P_BRAND_POIS] = MAX(brand[P_BRAND_POIS],BRAND_BOOST_NORMAL);
    }  
  if (p_ptr->special_attack & (ATTACK_HOLY)) 
    slay[P_SLAY_EVIL] = MAX(slay[P_SLAY_EVIL],15);

  /* Check for well-balanced throwing weapons */
  perfect = (o_ptr->id_obj & OF_PERFECT_BALANCE);
  
  if (o_ptr->tval > TV_BOLT)
    {
      text_out_to_screen(TERM_WHITE, 
			 "\nThrowing it you would do an average damage of ");
      for (i = 0; i < MAX_P_SLAY; i++)
	if (slay[i] > MULTIPLE_BASE) 
	  output_ammo_dam(o_ptr, slay[i], slayee[i], &first, perfect);
      
      for (i = 0; i < MAX_P_BRAND; i++)  
	if (brand[i] > MULTIPLE_BASE) 
	  {
	    char buf[40];

	    strnfmt(buf, sizeof(buf), "non %s resistant creatures", brandee[i]);
	    output_ammo_dam(o_ptr, brand[i], buf, &first, perfect);
	  }
      
      output_ammo_dam(o_ptr, MULTIPLE_BASE, 
		      (first) ? "all monsters" : "other monsters", 
		      &first, perfect);
      text_out_to_screen(TERM_WHITE, ". ");
    }
  else if (p_ptr->ammo_tval == o_ptr->tval)
    {
      /* Check launcher for additional brands (slays) */
      i_ptr = &inventory[INVEN_BOW];
      
      /* If wielding a launcher - sanity check */
      if (i_ptr->k_idx)
	{
	  /* Pick up any brands (and slays!) */
	  for (i = 0; i < MAX_P_SLAY; i++)
	    slay[i] = MAX(slay[i],
			  (i_ptr->id_other & (OBJECT_ID_BASE_SLAY << i)) 
			  ? i_ptr->multiple_slay[i] : MULTIPLE_BASE);
	  for (i = 0; i < MAX_P_BRAND; i++)
	    brand[i] = MAX(brand[i], 
			  (i_ptr->id_other & (OBJECT_ID_BASE_BRAND << i)) 
			  ? i_ptr->multiple_brand[i] : MULTIPLE_BASE);
	}
      
      text_out_to_screen(TERM_WHITE, "\nUsing it with your current launcher you would do an average damage per shot of ");
      
      for (i = 0; i < MAX_P_SLAY; i++)      
	if (slay[i] > MULTIPLE_BASE)
	  output_ammo_dam(o_ptr, slay[i], slayee[i], &first, perfect);

      for (i = 0; i < MAX_P_BRAND; i++)
	if (brand[i] > MULTIPLE_BASE) 
	  {
	    char buf[40];

	    strnfmt(buf, sizeof(buf), "non %s resistant creatures", brandee[i]);
	    output_ammo_dam(o_ptr, brand[i], buf, &first, perfect);
	  }
      
      output_ammo_dam(o_ptr, MULTIPLE_BASE, (first) ? "all monsters" : 
		      "other monsters", 
		      &first, perfect);
      text_out_to_screen(TERM_WHITE, ". ");
    }
  else text_out_to_screen(TERM_WHITE, "\nYou cannot use this missile with your current launcher. ");
  
}

void display_device_chance(object_type *o_ptr)
{
  /* Get the object kind */
  object_kind *k_ptr = &k_info[o_ptr->k_idx];

  /* Extract the item level */
  int lev = k_ptr->level;
  
  /* Base chance of success */
  int chance = p_ptr->skill_dev;

  /* Final probability */
  int prob = 10000;
  
  /* Confusion hurts skill */
  if (p_ptr->confused) chance = chance / 2;
  
  /* High level objects are harder */
  chance = chance - ((lev > 50) ? 50 : lev);

  /* Calculate the chance */
  if (chance < USE_DEVICE)
    {
      prob *= 2; 
      prob /= (USE_DEVICE * (USE_DEVICE + 1 - chance));
    }
  else 
    {
      prob -= prob * (USE_DEVICE - 1) / chance;
    }
  prob /= 100;

  /* Display */
  text_out_to_screen(TERM_WHITE, format("\nYou currently have a %d%% chance of activating this device.", prob));

  return;
}


/**
 * Extract and return extended information about an object, including 
 * (usually) flavor, (sometimes) damage information for magical items, 
 * and (if appropriate) ego and artifact lore. -LM-
 *
 * Code mostly from object_desc and roff_aux.
 */
void object_info(char buf[2048], object_type *o_ptr, bool in_store)
{
  object_kind *k_ptr = &k_info[o_ptr->k_idx];
  
  char *t;
  char *s;
  char *u;
  char *v;
  char *w;
  char *x;
  
  /* Assume no flavor string, no ego info, and no base info. */
  char *modstr = "";
  char *egoinfo = "";
  char *baseinfo = "";
  
  
  
  /* Standard artifacts have unique descriptions. */
  if ((artifact_p(o_ptr)) && (o_ptr->name1 < ART_MIN_RANDOM) && 
      (object_known_p(o_ptr)))
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];

      /* If already in memory, simple to access */
      strcpy(buf, a_text + a_ptr->text);
      
      /* Hack - translate if we do that */
      if (Term->xchar_hook)
	xstr_trans(buf, (Term->xchar_hook(128) == 128));

      /* Return the description, if any. */
      return;
    }
  
  /* All non-artifact or random artifact objects. */
  else
    {
      /* If already in memory, simple to access */
      strcpy(buf, k_text + k_ptr->text);
      
      /* No object description, so return failure. */
      if (!buf[0]) return;
      
      
      /* Various object types have different kinds of information. */
      switch(o_ptr->tval)
	{
	  /* Dragon Scale Mails */
	case TV_DRAG_ARMOR:
	  {
	    /* Allow processing of activation information. */
	    baseinfo = format("%s", buf);
	    break;
	  }
	  
	  /* Staffs */
	case TV_STAFF:
	  {
	    /* Color the object, unless in store. */
	    if (in_store)
	      {
		modstr = "";
		baseinfo = format("A staff %s", buf);
	      }
	    else
	      {
		modstr = object_adj(o_ptr->tval, o_ptr->sval);
		baseinfo = format("& # staff %s", buf);
	      }
	    
	    break;
	  }
	  
	  /* Wands */
	case TV_WAND:
	  {
	    /* Color the object, unless in store. */
	    if (in_store)
	      {
		modstr = "";
		baseinfo = format("A wand %s", buf);
	      }
	    else
	      {
		modstr = object_adj(o_ptr->tval, o_ptr->sval);
		baseinfo = format("& # wand %s", buf);
	      }
	    
	    break;
	  }
	  
	  /* Rods */
	case TV_ROD:
	  {
	    /* Color the object, unless in store. */
	    if (in_store)
	      {
		modstr = "";
		baseinfo = format("A rod %s", buf);
	      }
	    else
	      {
		modstr = object_adj(o_ptr->tval, o_ptr->sval);
		baseinfo = format("& # rod %s", buf);
	      }
	    
	    break;
	  }
	  
	  /* Scrolls */
	case TV_SCROLL:
	  {
	    /* Color the object, unless in store. */
	    if (in_store)
	      {
		modstr = "";
		baseinfo = format("A parchment scroll %s", buf);
	      }
	    else
	      {
		modstr = object_adj(o_ptr->tval, o_ptr->sval);
		baseinfo = format("A parchment scroll %s It is titled \"#\".", 
				  buf);
	      }
	    
	    break;
	  }
	  
	  /* Potions */
	case TV_POTION:
	  {
	    /* Color the object, unless in store. */
	    if (in_store)
	      {
		modstr = "";
		baseinfo = format("A potion %s", buf);
	      }
	    else
	      {
		modstr = object_adj(o_ptr->tval, o_ptr->sval);
		baseinfo = format("& # potion %s", buf);
	      }
	    
	    break;
	  }
	  
	  /* All other objects can just display the info text. */
	default:
	  {
	    /* Store the basic info text. */
	    baseinfo = format("%s", buf);
	  }
	}
      
      
      /* Ego-object descriptions are added to any base description. */
      if ((o_ptr->name2) && (object_known_p(o_ptr)))
	{
	  ego_item_type *e_ptr = &e_info[o_ptr->name2];
	  char ebuf[2048];
	  
	  /* First, find the information in memory, or get it from 
	   * the binary file.
	   */
	  /* If already in memory, simple to access */
	  strcpy(ebuf, e_text + e_ptr->text);
	  
	  /* Point to the ego-item information. */
	  egoinfo = ebuf;
	}
      
      /* Point to "buf", and start dumping the result */
      t = buf;
      
      
      /*** Assemble the object information. ***/
      
      /* The object needs an article */
      if (baseinfo[0] == '&')
	{
	  /* Skip ampersand and space. */
	  s = baseinfo + 2;
	  
	  /* Flavor starts with a vowel */
	  if (is_a_vowel(modstr[0])) w = "An ";
	  
	  /* Flavor starts with a non-vowel */
	  else w = "A ";
	}
      else 
	{
	  w = "";
	  
	  /* Start at beginning of base info. */
	  s = baseinfo;
	}
      
      /* Copy the base info, inserting flavor and info text. */
      for (; *s; s++)
	{
	  /* Insert article */
	  if (s != baseinfo)
	    {
	      for (; *w; w++) *t++ = *w;
	    }
	  
	  /* Insert flavor, make it lowercase */
	  if (*s == '#')
	    {
	      char make_lower;
	      
	      if (strlen(modstr))
		{
		  for (u = modstr; *u; u++)
		    {
		      make_lower = *u;
		      *t++ = tolower(make_lower);
		    }
		}
	    }
	  
	  /* Insert numerical info. */
	  else if (*s == '~')
	    {
	      /* Extra info if object is fully known. */
	      if (o_ptr->ident & (IDENT_KNOWN) || k_ptr->known_effect)
		{
		  /* Grab the numerical info. */
		  char *moddata = extra_data(o_ptr);
		  
		  /* If there is any numerical data,  */
		  if (strlen(moddata) > 0)
		    {
		      /* ...insert a space, and */
		      *t++ = ' ';
		      
		      /* insert the mumerical data into the string. */
		      for (v = moddata; *v; v++) *t++ = *v;
		    }
		}
	      
	      /* Otherwise, nothing. */
	    }
	  
	  /* Normally copy. */
	  else *t++ = *s;
	}
      
      /* Extra info for ego items. */
      if ((o_ptr->name2) && (object_known_p(o_ptr)))
	{
	  char *divider = (small_screen ? "                    ---" :
			  "                                   ---");


	  /* Insert a return, a divider, and another return. */
	  *t++ = '\n';
	  for (x = divider; *x; x++) *t++ = *x;
	  *t++ = '\n';
	  
	  /* Copy the ego info to the information string. */
	  for (x = egoinfo; *x; x++) *t++ = *x;
	}
      
      /* End the string. */
      *t = '\0';
      
      /* Hack - translate if we do that */
      if (Term->xchar_hook)
	xstr_trans(buf, (Term->xchar_hook(128) == 128));

      /* Return the string. */
      return;
    }
}

/**
 * Describe an item.  Rewritten for FAangband 1.1 to fit ID by use.
 */
void object_info_detail(object_type *o_ptr)
{
  int j;
  
  int attr_listed = 0;
  int attr_num = 0;
  int kill_num = 0;
  int max = 0, min = 0;

  bool res = FALSE, vul = FALSE;

  char buf[10] = "";
  char *statname[] = {"strength", "intelligence", "wisdom", 
			    "dexterity", "constitution", "charisma"};
  char *othername[] = {"magic mastery", "stealth", "searching", 
			     "infravision", "tunnelling", "speed", 
			     "shooting speed", "shooting power"};
  char *slayee[] = { "animals", "evil creatures", "undead", "demons", 
			   "orcs", "trolls", "giants", "dragons" };
  char *brand[] = { "acid", "lightning", "fire", "cold", "poison"  };

  
  /* Describe activation, if present and fully known. */
  if ((o_ptr->activation) && (o_ptr->ident & IDENT_WORN))
    {
      text_out_to_screen(TERM_YELLOW, 
			 format("Activation: %s\n", item_activation(o_ptr)));
    }
  
  
  /* Hack -- describe lite's */
  if (o_ptr->tval == TV_LITE)
    {
      if (artifact_p(o_ptr) && (o_ptr->k_idx != 477))
	{
	  text_out_to_screen(TERM_WHITE, 
			     "It provides light (radius 3) forever.  ");
	}
      else if (o_ptr->sval == SV_LITE_LANTERN)
	{
	  text_out_to_screen(TERM_WHITE, 
			     "It provides light (radius 2) when fueled.  ");
	}
      else if (o_ptr->sval == SV_LITE_TORCH)
	{
	  text_out_to_screen(TERM_WHITE, 
			     "It provides light (radius 1) when fueled.  ");
	}
    }

  /* Hack - artifact magic devices */
  if ((o_ptr->name1) && (extra_data(o_ptr)))
    text_out_to_screen(TERM_WHITE, extra_data(o_ptr));

  text_out_to_screen(TERM_WHITE, "\n");
  
  
  /* And then describe it fully */

  /* Single skill and deadliness bonuses for jewellery */
  if (is_jewellery(o_ptr))
    {
      if (o_ptr->to_d == 0)
	{
	  if (o_ptr->to_h < 0)
	    {
	      text_out_to_screen(TERM_WHITE, "It subtracts ");
	      text_out_to_screen(TERM_RED, format("%d ", -o_ptr->to_h));
	      text_out_to_screen(TERM_WHITE, "from your skill.  ");
	    }
	  else if (o_ptr->to_h > 0)
	    {
	      text_out_to_screen(TERM_WHITE, "It adds ");
	      text_out_to_screen(TERM_L_GREEN, format("%d ", o_ptr->to_h));
	      text_out_to_screen(TERM_WHITE, "to your skill.  ");
	    }
	}
      if (o_ptr->to_h == 0)
	{
	  if (o_ptr->to_d < 0)
	    {
	      text_out_to_screen(TERM_WHITE, "It subtracts ");
	      text_out_to_screen(TERM_RED, format("%d ", -o_ptr->to_d));
	      text_out_to_screen(TERM_WHITE, "from your deadliness.  ");
	    }
	  else if (o_ptr->to_d > 0)
	    {
	      text_out_to_screen(TERM_WHITE, "It adds ");
	      text_out_to_screen(TERM_GREEN, format("%d ", o_ptr->to_d));
	      text_out_to_screen(TERM_WHITE, "to your deadliness.  ");
	    }
	}
    }
  
  /* Affects stats. */
  if ((o_ptr->ident) & IDENT_WORN)
    {
      for (j = 0; j < A_MAX; j++)
	{
	  if (o_ptr->bonus_stat[j] != 0) attr_num++;
	  if (o_ptr->bonus_stat[j] < min) min = o_ptr->bonus_stat[j];
	  if (o_ptr->bonus_stat[j] > max) max = o_ptr->bonus_stat[j];
	}
      
      if (attr_num > 0)
	{
	  byte attr = (o_ptr->bonus_stat[A_STR] > 0 ? 
		       TERM_L_GREEN : TERM_ORANGE); 
	  
	  text_out_to_screen(TERM_WHITE, "It gives ");
	  
	  /* Special case:  all stats */
	  if (min == max)
	    {
	      text_out_to_screen(attr, format("%d ", min));
	      text_out_to_screen(attr, "to all your stats");
	    }
	  
	  /* Some stats */
	  else
	    {
	      for (j = 0; j < A_MAX; j++)
		{
		  if (o_ptr->bonus_stat[j] == 0) continue;
		  attr =  (o_ptr->bonus_stat[j] > 0 ? 
			   TERM_L_GREEN : TERM_ORANGE);
		  text_out_to_screen(attr, format("%d ", o_ptr->bonus_stat[j]));
		  text_out_to_screen(TERM_WHITE, "to your ");
		  text_out_to_screen(attr, statname[j]);
		  if (attr_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
		  if (attr_num == 2) text_out_to_screen(TERM_WHITE, " and ");
		  if (attr_num == 1) text_out_to_screen(TERM_WHITE, ". ");
		  attr_num--;
		}
	    }
	}
    }
      
  
  /* Sustain stats. */
  if ((o_ptr->id_obj & (OF_SUSTAIN_STR)) || 
      (o_ptr->id_obj & (OF_SUSTAIN_INT)) || 
      (o_ptr->id_obj & (OF_SUSTAIN_WIS)) || 
      (o_ptr->id_obj & (OF_SUSTAIN_DEX)) || 
      (o_ptr->id_obj & (OF_SUSTAIN_CON)) || 
      (o_ptr->id_obj & (OF_SUSTAIN_CHR)))
    {
      /* Clear number of items to list. */
      attr_num = 0;
      
      /* How many attributes need to be listed? */
      if (o_ptr->id_obj & (OF_SUSTAIN_STR)) attr_num++;
      if (o_ptr->id_obj & (OF_SUSTAIN_INT)) attr_num++;
      if (o_ptr->id_obj & (OF_SUSTAIN_WIS)) attr_num++;
      if (o_ptr->id_obj & (OF_SUSTAIN_DEX)) attr_num++;
      if (o_ptr->id_obj & (OF_SUSTAIN_CON)) attr_num++;
      if (o_ptr->id_obj & (OF_SUSTAIN_CHR)) attr_num++;
      
      /* Special case:  sustain all stats */
      if (attr_num == A_MAX)
	{
	  text_out_to_screen(TERM_WHITE, "It sustains ");
	  text_out_to_screen(TERM_L_GREEN, "all your stats. ");
	}
      else
	{
	  text_out_to_screen(TERM_WHITE, "It sustains your ");
	  
	  /* Loop for number of attributes in this group. */
	  for (j = 0; j < A_MAX; j++)
	    {
	      if (!(o_ptr->id_obj & (OF_SUSTAIN_STR << j))) continue;
	      text_out_to_screen(TERM_L_GREEN, statname[j]);
	      if (attr_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
	      if (attr_num == 2) text_out_to_screen(TERM_WHITE, " and ");
	      if (attr_num == 1) text_out_to_screen(TERM_WHITE, ". ");
	      attr_num--;
	    }
	  
	}
    }
      
  
  /* Clear number of items to list. */
  attr_num = 0;

  /* Affects other bonuses. */
  if ((o_ptr->ident) & IDENT_WORN)
    {
      for (j = 0; j < MAX_P_BONUS; j++)
	if (o_ptr->bonus_other[j] != 0) attr_num++;
      
      if (attr_num > 0)
	{
	  byte attr = TERM_WHITE; 
	  
	  text_out_to_screen(TERM_WHITE, "It gives ");
	  
	  for (j = 0; j < MAX_P_BONUS; j++)
	    {
	      if (o_ptr->bonus_other[j] == 0) continue;
	      attr =  (o_ptr->bonus_other[j] > 0 ? TERM_L_GREEN : TERM_ORANGE);
	      text_out_to_screen(attr, format("%d ", o_ptr->bonus_other[j]));
	      text_out_to_screen(TERM_WHITE, "to your ");
	      text_out_to_screen(attr, othername[j]);
	      if (attr_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
	      if (attr_num == 2) text_out_to_screen(TERM_WHITE, " and ");
	      if (attr_num == 1) text_out_to_screen(TERM_WHITE, ". ");
	      attr_num--;
	    }
	}
    }
  
  /* Clear number of items to list. */
  attr_num = 0;

  /* Slays. */
  for (j = 0; j < MAX_P_SLAY; j++)
    if (o_ptr->id_other & (OBJECT_ID_BASE_SLAY << j)) 
      {
	attr_num++;
	
	/* Hack for great banes */
	if ((j == P_SLAY_ANIMAL) && 
	    (o_ptr->multiple_slay[j] > SLAY_BOOST_MINOR))
	  {
	    attr_num--;
	    kill_num++;
	  }
	if ((j == P_SLAY_EVIL) && (o_ptr->multiple_slay[j] > SLAY_BOOST_SMALL))
	  {
	    attr_num--;
	    kill_num++;
	  }
	if ((j >= P_SLAY_UNDEAD) && 
	    (o_ptr->multiple_slay[j] > SLAY_BOOST_NORMAL))
	  {
	    attr_num--;
	    kill_num++;
	  }
      }
    
  if (attr_num > 0)
    {
      byte attr = TERM_RED; 

      text_out_to_screen(TERM_WHITE, "It slays ");
      
      for (j = 0; j < MAX_P_SLAY; j++)
	{
	  if (!(o_ptr->id_other & (OBJECT_ID_BASE_SLAY << j))) continue;
	  if ((j == 0) && (o_ptr->multiple_slay[j] > SLAY_BOOST_MINOR)) 
	    continue;
	  if ((j == 1) && (o_ptr->multiple_slay[j] > SLAY_BOOST_SMALL)) 
	    continue;
	  if ((j > 1) && (o_ptr->multiple_slay[j] > SLAY_BOOST_NORMAL))
	    continue;
	  text_out_to_screen(attr, slayee[j]);
	  if (attr_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
	  if (attr_num == 2) text_out_to_screen(TERM_WHITE, " and ");
	  if (attr_num == 1) text_out_to_screen(TERM_WHITE, ". ");
	  attr_num--;
	}
    }
  
  if (kill_num > 0)
    {
      byte attr = TERM_RED; 

      text_out_to_screen(TERM_WHITE, "It is a great bane of ");
      
      for (j = 0; j < MAX_P_SLAY; j++)
	{
	  if (!(o_ptr->id_other & (OBJECT_ID_BASE_SLAY << j))) continue;
	  if ((j == 0) && (o_ptr->multiple_slay[j] <= SLAY_BOOST_MINOR)) 
	    continue;
	  if ((j == 1) && (o_ptr->multiple_slay[j] <= SLAY_BOOST_SMALL)) 
	    continue;
	  if ((j > 1) && (o_ptr->multiple_slay[j] <= SLAY_BOOST_NORMAL))
	    continue;
	  text_out_to_screen(attr, slayee[j]);
	  if (kill_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
	  if (kill_num == 2) text_out_to_screen(TERM_WHITE, " and ");
	  if (kill_num == 1) text_out_to_screen(TERM_WHITE, ". ");
	  kill_num--;
	}
    }
  
  /* Clear number of items to list. */
  attr_num = 0;
  
  /* Elemental and poison brands. */
  for (j = 0; j < MAX_P_BRAND; j++)
    if (o_ptr->id_other & (OBJECT_ID_BASE_BRAND << j)) attr_num++;
  
  if (attr_num > 0)
    {
      byte attr = TERM_L_UMBER; 
      
      text_out_to_screen(TERM_WHITE, "It does extra damage from ");
      
      for (j = 0; j < MAX_P_BRAND; j++)
	{
	  if (!(o_ptr->id_other & (OBJECT_ID_BASE_BRAND << j))) continue;
	  text_out_to_screen(attr, brand[j]);
	  if (attr_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
	  if (attr_num == 2) text_out_to_screen(TERM_WHITE, " and ");
	  if (attr_num == 1) text_out_to_screen(TERM_WHITE, ". ");
	  attr_num--;
	}
    }
  
  /* Throwing weapons. */
  if (o_ptr->id_obj & (OF_THROWING))
    {
      if (o_ptr->id_obj & (OF_PERFECT_BALANCE))
	{
	  text_out_to_screen(TERM_WHITE, "It can be thrown hard and fast. ");
	}
      else text_out_to_screen(TERM_WHITE, "It can be thrown effectively. ");
    }
  
  /* Clear number of items to list. */
  attr_num = 0;
  
  /* Elemental immunities. */
  for (j = 0; j < 4; j++)
    if ((o_ptr->percent_res[j] == RES_BOOST_IMMUNE) && 
	(o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j))) attr_num++;
        
  if (attr_num > 0)
    {
      text_out_to_screen(TERM_WHITE, "It provides ");
      text_out_to_screen(TERM_BLUE, "immunity ");
      text_out_to_screen(TERM_WHITE, "to ");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < 4; j++)
	{
	  if (o_ptr->percent_res[j] > RES_BOOST_IMMUNE) continue;
	  if (!(o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j))) continue;
	  
	  /* List the attribute description, in its proper place. */
	  if (j == 0) text_out_to_screen(TERM_SLATE, "acid");
	  if (j == 1) text_out_to_screen(TERM_BLUE, "electricity");
	  if (j == 2) text_out_to_screen(TERM_RED, "fire");
	  if (j == 3) text_out_to_screen(TERM_L_WHITE, "frost");
	  if (attr_num >= 3) text_out_to_screen(TERM_WHITE, ", ");
	  if (attr_num == 2) text_out_to_screen(TERM_WHITE, " and ");
	  if (attr_num == 1) text_out_to_screen(TERM_WHITE, ". ");
	  attr_num--;
	}
      
      /* End sentence.  Go to next line. */
      text_out_to_screen(TERM_WHITE, ". ");
    }
  
  /* Check for resists and vulnerabilities */
  for (j = 0; j < MAX_P_RES; j++)
    {
      if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] < 100) && 
	  (o_ptr->percent_res[j] > 0) && 
	  (o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j)))
	res = TRUE;
      else if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] > 100) &&
	       (o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j)))
	vul = TRUE;
    }

  
  /* Resistances. */
  if (res)
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      for (j = 0; j < MAX_P_RES; j++)
	{
	  if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] < 100) && 
	      (o_ptr->percent_res[j] > 0) && 
	      (o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j)))
	    attr_num++;
	}

      /* How many attributes need to be listed? */
      text_out_to_screen(TERM_WHITE, "It provides ");
      text_out_to_screen(TERM_L_BLUE, "resistance ");
      text_out_to_screen(TERM_WHITE, "to");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < MAX_P_RES; j++)
	{
	  bool list_ok = FALSE;
	  
	  if (j == P_RES_CONFU) continue;
	  if (!(o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j))) continue;

	  if ((o_ptr->percent_res[j] < 100) && (o_ptr->percent_res[j] > 0))
	    list_ok = TRUE;
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    text_out_to_screen(TERM_WHITE, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		text_out_to_screen(TERM_WHITE, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == P_RES_ACID) text_out_to_screen(TERM_SLATE, " acid");
	  if (j == P_RES_ELEC) text_out_to_screen(TERM_BLUE, " electricity");
	  if (j == P_RES_FIRE) text_out_to_screen(TERM_RED, " fire");
	  if (j == P_RES_COLD) text_out_to_screen(TERM_L_WHITE, " frost");
	  if (j == P_RES_POIS) text_out_to_screen(TERM_GREEN, " poison");
	  if (j == P_RES_LITE) text_out_to_screen(TERM_ORANGE, " light");
	  if (j == P_RES_DARK) text_out_to_screen(TERM_L_DARK, " darkness");
	  if (j == P_RES_SOUND) text_out_to_screen(TERM_YELLOW, " sound");
	  if (j == P_RES_SHARD) text_out_to_screen(TERM_UMBER, " shards");
	  if (j == P_RES_NEXUS) text_out_to_screen(TERM_L_RED, " nexus");
	  if (j == P_RES_NETHR) text_out_to_screen(TERM_L_GREEN, " nether");
	  if (j == P_RES_CHAOS) text_out_to_screen(TERM_VIOLET, " chaos");
	  if (j == P_RES_DISEN) text_out_to_screen(TERM_VIOLET, " disenchantment");

	  sprintf(buf, "(%d%%)", 100 - o_ptr->percent_res[j]);
	  text_out_to_screen(TERM_WHITE, buf);
	}
      
      /* End sentence.  Go to next line. */
      text_out_to_screen(TERM_WHITE, ". ");
    }
  
  
  /* Vulnerabilities. */
  if (vul)
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      for (j = 0; j < MAX_P_RES; j++)
	{
	  if ((j != P_RES_CONFU) && (o_ptr->percent_res[j] > 100) && 
	      (o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j)))
	    attr_num++;
	}

      text_out_to_screen(TERM_WHITE, "It makes you ");
      text_out_to_screen(TERM_ORANGE, "vulnerable ");
      text_out_to_screen(TERM_WHITE, "to");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < MAX_P_RES; j++)
	{
	  bool list_ok = FALSE;
	  
	  if (j == P_RES_CONFU) continue;
	  if (!(o_ptr->id_other & (OBJECT_ID_BASE_RESIST << j))) continue;

	  if (o_ptr->percent_res[j] > 100)
	    list_ok = TRUE;
	  
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    text_out_to_screen(TERM_WHITE, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		text_out_to_screen(TERM_WHITE, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == P_RES_ACID) text_out_to_screen(TERM_SLATE, " acid");
	  if (j == P_RES_ELEC) text_out_to_screen(TERM_BLUE, " electricity");
	  if (j == P_RES_FIRE) text_out_to_screen(TERM_RED, " fire");
	  if (j == P_RES_COLD) text_out_to_screen(TERM_L_WHITE, " frost");
	  if (j == P_RES_POIS) text_out_to_screen(TERM_GREEN, " poison");
	  if (j == P_RES_LITE) text_out_to_screen(TERM_ORANGE, " light");
	  if (j == P_RES_DARK) text_out_to_screen(TERM_L_DARK, " darkness");
	  if (j == P_RES_SOUND) text_out_to_screen(TERM_YELLOW, " sound");
	  if (j == P_RES_SHARD) text_out_to_screen(TERM_UMBER, " shards");
	  if (j == P_RES_NEXUS) text_out_to_screen(TERM_L_RED, " nexus");
	  if (j == P_RES_NETHR) text_out_to_screen(TERM_L_GREEN, " nether");
	  if (j == P_RES_CHAOS) text_out_to_screen(TERM_VIOLET, " chaos");
	  if (j == P_RES_DISEN) 
	    text_out_to_screen(TERM_VIOLET, " disenchantment");

	  sprintf(buf, "(%d%%)", o_ptr->percent_res[j] - 100);
	  text_out_to_screen(TERM_WHITE, buf);
	}
      
      /* End sentence.  Go to next line. */
      text_out_to_screen(TERM_WHITE, ". ");
    }
  
  
  /* Clear a listing variable. */
	attr_num = 0;
  
  /* Special processing for the three "survival resists" */
  if (o_ptr->id_obj & (OF_FEARLESS)) attr_num++;
  if (o_ptr->id_obj & (OF_SEEING)) attr_num++;
  if (o_ptr->id_other & IF_RES_CONFU) attr_num++;
  
  if (o_ptr->id_obj & (OF_FEARLESS))
    {
      text_out_to_screen(TERM_WHITE, "It renders you fearless");
      if (attr_num == 1) text_out_to_screen(TERM_WHITE, ".  ");
      else text_out_to_screen(TERM_WHITE, ", and");
    }
  
  if (o_ptr->id_obj & (OF_SEEING))
    {
      if ((attr_num > 1) && (o_ptr->id_obj & (OF_FEARLESS))) 
	text_out_to_screen(TERM_WHITE, " provides resistance to blindness");
      else text_out_to_screen(TERM_WHITE, "It provides resistance to blindness");
      
      if (o_ptr->id_other & IF_RES_CONFU) 
	text_out_to_screen(TERM_WHITE, " and");
      else text_out_to_screen(TERM_WHITE, ".  ");
    }
  
  if (o_ptr->id_other & IF_RES_CONFU)
    {
      if ((o_ptr->percent_res[P_RES_CONFU] < 100) && 
      (o_ptr->percent_res[P_RES_CONFU] > 0))
	{
	  if ((attr_num > 1) && (!(o_ptr->id_obj & (OF_SEEING))))
	    text_out_to_screen(TERM_WHITE, " provides resistance to");
	  else if (attr_num == 1)
	    text_out_to_screen(TERM_WHITE, "It provides resistance to");
	  text_out_to_screen(TERM_L_UMBER, " confusion");
	  sprintf(buf, "(%d%%)", 100 - o_ptr->percent_res[P_RES_CONFU]);
	  text_out_to_screen(TERM_WHITE, buf);
	  text_out_to_screen(TERM_WHITE, ".  ");
	}
      else if (o_ptr->percent_res[P_RES_CONFU] > 100)
	{
	  if (attr_num > 1) 
	    text_out_to_screen(TERM_WHITE, " makes you vulnerable to ");
	  else 
	    text_out_to_screen(TERM_WHITE, "It makes you vulnerable to ");
	  text_out_to_screen(TERM_L_UMBER, "confusion");
	  sprintf(buf, "(%d%%)", o_ptr->percent_res[P_RES_CONFU] - 100);
	  text_out_to_screen(TERM_WHITE, buf);
	  text_out_to_screen(TERM_WHITE, ". ");
	}
    }
  
  
  /* Miscellaneous abilities. */
  if ((o_ptr->id_obj & (OF_SLOW_DIGEST)) || 
      (o_ptr->id_obj & (OF_FEATHER)) || 
      (o_ptr->id_obj & (OF_LITE)) || 
      (o_ptr->id_obj & (OF_REGEN)) || 
      (o_ptr->id_obj & (OF_TELEPATHY)) || 
      (o_ptr->id_obj & (OF_SEE_INVIS)) || 
      (o_ptr->id_obj & (OF_FREE_ACT)) || 
      (o_ptr->id_obj & (OF_HOLD_LIFE)) || 
      (o_ptr->id_obj & (OF_IMPACT)) || 
      (o_ptr->id_obj & (OF_BLESSED)) || 
      (o_ptr->id_obj & (OF_CHAOTIC) || 
      (o_ptr->id_obj & (OF_DARKNESS))))
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      /* How many attributes need to be listed? */
      if (o_ptr->id_obj & (OF_SLOW_DIGEST)) attr_num++;
      if (o_ptr->id_obj & (OF_FEATHER)) attr_num++;
      if (o_ptr->id_obj & (OF_LITE)) attr_num++;
      if (o_ptr->id_obj & (OF_REGEN)) attr_num++;
      if (o_ptr->id_obj & (OF_TELEPATHY)) attr_num++;
      if (o_ptr->id_obj & (OF_SEE_INVIS)) attr_num++;
      if (o_ptr->id_obj & (OF_FREE_ACT)) attr_num++;
      if (o_ptr->id_obj & (OF_HOLD_LIFE)) attr_num++;
      if (o_ptr->id_obj & (OF_IMPACT)) attr_num++;
      if (o_ptr->id_obj & (OF_BLESSED)) attr_num++;
      if (o_ptr->id_obj & (OF_CHAOTIC)) attr_num++; 
      if (o_ptr->id_obj & (OF_DARKNESS)) attr_num++;
      
      text_out_to_screen(TERM_WHITE, "It");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < 12; j++)
	{
	  bool list_ok = FALSE;
	  
	  if ((j == 0) && (o_ptr->id_obj & (OF_SLOW_DIGEST))) list_ok = TRUE;
	  if ((j == 1) && (o_ptr->id_obj & (OF_FEATHER))) list_ok = TRUE;
	  if ((j == 2) && (o_ptr->id_obj & (OF_LITE))) list_ok = TRUE;
	  if ((j == 3) && (o_ptr->id_obj & (OF_REGEN))) list_ok = TRUE;
	  if ((j == 4) && (o_ptr->id_obj & (OF_TELEPATHY))) list_ok = TRUE;
	  if ((j == 5) && (o_ptr->id_obj & (OF_SEE_INVIS))) list_ok = TRUE;
	  if ((j == 6) && (o_ptr->id_obj & (OF_FREE_ACT))) list_ok = TRUE;
	  if ((j == 7) && (o_ptr->id_obj & (OF_HOLD_LIFE)))list_ok = TRUE;
	  if ((j == 8) && (o_ptr->id_obj & (OF_IMPACT))) list_ok = TRUE;
	  if ((j == 9) && (o_ptr->id_obj & (OF_BLESSED))) list_ok = TRUE;
	  if ((j == 10) && (o_ptr->id_obj & (OF_CHAOTIC))) list_ok = TRUE;
	  if ((j == 11) && (o_ptr->id_obj & (OF_DARKNESS))) list_ok = TRUE;
	  
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    text_out_to_screen(TERM_WHITE, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		text_out_to_screen(TERM_WHITE, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == 0) 
	    text_out_to_screen(TERM_WHITE, " slows your metabolism");
	  if (j == 1) 
	    text_out_to_screen(TERM_WHITE, " induces feather falling");
	  if (j == 2) 
	    text_out_to_screen(TERM_WHITE, " provides permanent light");
	  if (j == 3) 
	    text_out_to_screen(TERM_WHITE, " speeds your regenerative powers");
	  if (j == 4) 
	    text_out_to_screen(TERM_WHITE, " gives telepathic powers");
	  if (j == 5) 
	    text_out_to_screen(TERM_WHITE, 
			       " allows you to see invisible monsters");
	  if (j == 6) 
	    text_out_to_screen(TERM_WHITE, " provides immunity to paralysis");
	  if (j == 7) 
	    text_out_to_screen(TERM_WHITE, 
			       " provides resistance to life draining");
	  if (j == 8) 
	    text_out_to_screen(TERM_WHITE, " induces earthquakes");
	  if (j == 9) 
	    text_out_to_screen(TERM_WHITE, " has been blessed by the gods");
	  if (j == 10) 
	    text_out_to_screen(TERM_WHITE, " causes chaotic effects");
	  if (j == 11) 
	    text_out_to_screen(TERM_WHITE, " allows you to see in the dark");
	}
      
      /* End sentence.  Go to next line. */
      text_out_to_screen(TERM_WHITE, ". ");
    }
  
  
  /* Nastyness. */
  if (known_cursed_p(o_ptr) || 
      ((o_ptr->ident & IDENT_STORE) && (o_ptr->flags_obj & OF_SHOW_CURSE)))
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      /* How many attributes need to be listed? */
      if (o_ptr->id_curse & CF_TELEPORT) attr_num++;
      if (o_ptr->id_curse & CF_NO_TELEPORT) attr_num++;
      if (o_ptr->id_curse & CF_AGGRO_PERM) attr_num++;
      if (o_ptr->id_curse & CF_AGGRO_RAND) attr_num++;
      if (o_ptr->id_curse & CF_SLOW_REGEN) attr_num++;
      if (o_ptr->id_curse & CF_AFRAID) attr_num++;
      if (o_ptr->id_curse & CF_HUNGRY) attr_num++;
      if (o_ptr->id_curse & CF_POIS_RAND) attr_num++;
      if (o_ptr->id_curse & CF_POIS_RAND_BAD) attr_num++;
      if (o_ptr->id_curse & CF_CUT_RAND) attr_num++;
      if (o_ptr->id_curse & CF_CUT_RAND_BAD) attr_num++;
      if (o_ptr->id_curse & CF_HALLU_RAND) attr_num++;
      if (o_ptr->id_curse & CF_DROP_WEAPON) attr_num++;
      if (o_ptr->id_curse & CF_ATTRACT_DEMON) attr_num++;
      if (o_ptr->id_curse & CF_ATTRACT_UNDEAD) attr_num++;
      if (o_ptr->id_curse & CF_STICKY_CARRY) attr_num++;
      if (o_ptr->id_curse & CF_STICKY_WIELD) attr_num++;
      if (o_ptr->id_curse & CF_PARALYZE) attr_num++;
      if (o_ptr->id_curse & CF_PARALYZE_ALL) attr_num++;
      if (o_ptr->id_curse & CF_DRAIN_EXP) attr_num++;
      if (o_ptr->id_curse & CF_DRAIN_MANA) attr_num++;
      if (o_ptr->id_curse & CF_DRAIN_STAT) attr_num++;
      if (o_ptr->id_curse & CF_DRAIN_CHARGE) attr_num++;
      
      text_out_to_screen(TERM_WHITE, "It is cursed");
      if (attr_num) text_out_to_screen(TERM_WHITE, "; it");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < 23; j++)
	{
	  bool list_ok = FALSE;
	  
	  if ((j == 0) && (o_ptr->id_curse & CF_TELEPORT)) list_ok = TRUE;
	  if ((j == 1) && (o_ptr->id_curse & CF_NO_TELEPORT)) list_ok = TRUE;
	  if ((j == 2) && (o_ptr->id_curse & CF_AGGRO_PERM)) list_ok = TRUE;
	  if ((j == 3) && (o_ptr->id_curse & CF_AGGRO_RAND)) list_ok = TRUE;
	  if ((j == 4) && (o_ptr->id_curse & CF_SLOW_REGEN)) list_ok = TRUE;
	  if ((j == 5) && (o_ptr->id_curse & CF_AFRAID)) list_ok = TRUE;
	  if ((j == 6) && (o_ptr->id_curse & CF_HUNGRY)) list_ok = TRUE;
	  if ((j == 7) && (o_ptr->id_curse & CF_POIS_RAND)) list_ok = TRUE;
	  if ((j == 8) && (o_ptr->id_curse & CF_POIS_RAND_BAD)) list_ok = TRUE;
	  if ((j == 9) && (o_ptr->id_curse & CF_CUT_RAND)) list_ok = TRUE;
	  if ((j == 10) && (o_ptr->id_curse & CF_CUT_RAND_BAD)) list_ok = TRUE;
	  if ((j == 11) && (o_ptr->id_curse & CF_HALLU_RAND)) list_ok = TRUE;
	  if ((j == 12) && (o_ptr->id_curse & CF_DROP_WEAPON)) list_ok = TRUE;
	  if ((j == 13) && (o_ptr->id_curse & CF_ATTRACT_DEMON)) 
	    list_ok = TRUE;
	  if ((j == 14) && (o_ptr->id_curse & CF_ATTRACT_UNDEAD)) 
	    list_ok = TRUE;
	  if ((j == 15) && (o_ptr->id_curse & CF_STICKY_CARRY)) list_ok = TRUE;
	  if ((j == 16) && (o_ptr->id_curse & CF_STICKY_WIELD)) list_ok = TRUE;
	  if ((j == 17) && (o_ptr->id_curse & CF_PARALYZE)) list_ok = TRUE;
	  if ((j == 18) && (o_ptr->id_curse & CF_PARALYZE_ALL)) list_ok = TRUE;
	  if ((j == 19) && (o_ptr->id_curse & CF_DRAIN_EXP)) list_ok = TRUE;
	  if ((j == 20) && (o_ptr->id_curse & CF_DRAIN_MANA)) list_ok = TRUE;
	  if ((j == 21) && (o_ptr->id_curse & CF_DRAIN_STAT)) list_ok = TRUE;
	  if ((j == 22) && (o_ptr->id_curse & CF_DRAIN_CHARGE)) list_ok = TRUE;
	  
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    text_out_to_screen(TERM_WHITE, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		text_out_to_screen(TERM_WHITE, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == 0) 
	    text_out_to_screen(TERM_WHITE, " induces random teleportation");
	  if (j == 1) 
	    text_out_to_screen(TERM_WHITE, " prevents teleportation");
	  if (j == 2) 
	    text_out_to_screen(TERM_WHITE, " aggravates nearby creatures");
	  if (j == 3) 
	    text_out_to_screen(TERM_WHITE, " occasionally greatly aggravates nearby creatures");
	  if (j == 4) text_out_to_screen(TERM_WHITE, " slows your regeneration");
	  if (j == 5) text_out_to_screen(TERM_WHITE, " makes you afraid");
	  if (j == 6) text_out_to_screen(TERM_WHITE, " speeds your digestion");
	  if (j == 7) text_out_to_screen(TERM_WHITE, " occasionally poisons you");
	  if (j == 8) text_out_to_screen(TERM_WHITE, " randomly envelops you in poison");
	  if (j == 9) text_out_to_screen(TERM_WHITE, " occasionally cuts you");
	  if (j == 10) text_out_to_screen(TERM_WHITE, " will suddenly cause deep wounds");
	  if (j == 11) text_out_to_screen(TERM_WHITE, " sometimes makes you hallucinate");
	  if (j == 12) text_out_to_screen(TERM_WHITE, " will suddenly leap from your grasp");
	  if (j == 13) text_out_to_screen(TERM_WHITE, " makes demons suddenly appear nearby");
	  if (j == 14) text_out_to_screen(TERM_WHITE, " calls the undead from their slumber");
	  if (j == 15) text_out_to_screen(TERM_WHITE, " cannot be dropped from your pack");
	  if (j == 16) text_out_to_screen(TERM_WHITE, " sticks to you if wielded");
	  if (j == 17) text_out_to_screen(TERM_WHITE, " will briefly paralyze you");
	  if (j == 18) text_out_to_screen(TERM_WHITE, " can paralyze you even if you feel immune");
	  if (j == 19) text_out_to_screen(TERM_WHITE, " drains experience");
	  if (j == 20) text_out_to_screen(TERM_WHITE, " drains mana");
	  if (j == 21) text_out_to_screen(TERM_WHITE, " will sometimes lower a stat");
	  if (j == 22) text_out_to_screen(TERM_WHITE, " will take energy from your magic devices");
	}
      
      /* End sentence.  Go to next line. */
      text_out_to_screen(TERM_WHITE, ".  ");

      /* Say if the curse is permanent */
      if (o_ptr->id_obj & OF_PERMA_CURSE) 
	text_out_to_screen(TERM_WHITE, "It cannot be uncursed. ");

      /* Say if curse removal has been tried */
      if (o_ptr->id_obj & OF_FRAGILE) 
	text_out_to_screen(TERM_WHITE, 
			   "Attempting to uncurse it may destroy it. ");
    }

  /* Ignore various elements. */
  if ((o_ptr->id_obj & OF_ACID_PROOF) ||
      (o_ptr->id_obj & OF_ELEC_PROOF) ||
      (o_ptr->id_obj & OF_FIRE_PROOF) ||
      (o_ptr->id_obj & OF_COLD_PROOF))
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      /* How many attributes need to be listed? */
      if (o_ptr->id_obj & (OF_ACID_PROOF)) attr_num++;
      if (o_ptr->id_obj & (OF_ELEC_PROOF)) attr_num++;
      if (o_ptr->id_obj & (OF_FIRE_PROOF)) attr_num++;
      if (o_ptr->id_obj & (OF_COLD_PROOF)) attr_num++;
      
      text_out_to_screen(TERM_WHITE, "It cannot be damaged by");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < 4; j++)
	{
	  bool list_ok = FALSE;
	  
	  if ((j == 0) && (o_ptr->id_obj & (OF_ACID_PROOF))) list_ok = TRUE;
	  if ((j == 1) && (o_ptr->id_obj & (OF_ELEC_PROOF))) list_ok = TRUE;
	  if ((j == 2) && (o_ptr->id_obj & (OF_FIRE_PROOF))) list_ok = TRUE;
	  if ((j == 3) && (o_ptr->id_obj & (OF_COLD_PROOF))) list_ok = TRUE;
	  
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    text_out_to_screen(TERM_WHITE, ",");
	  
	  /* "or" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		text_out_to_screen(TERM_WHITE, " or");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == 0) text_out_to_screen(TERM_WHITE, " acid");
	  if (j == 1) text_out_to_screen(TERM_WHITE, " electricity");
	  if (j == 2) text_out_to_screen(TERM_WHITE, " fire");
	  if (j == 3) text_out_to_screen(TERM_WHITE, " frost");
	}
      
      /* End sentence.  Go to next line. */
      text_out_to_screen(TERM_WHITE, ". ");
    }

  /* Only look at wieldables */
  if (wield_slot(o_ptr) >= INVEN_WIELD)
    {
      /* All normal properties known */
      if (o_ptr->ident & IDENT_KNOWN)
	{
	  /* Everything known */
	  if (o_ptr->ident & (IDENT_KNOW_CURSES | IDENT_UNCURSED))
	    text_out_to_screen(TERM_WHITE, "You know all about this object.");
	  
	  /* Some unknown curses */
	  else 
	    text_out_to_screen(TERM_WHITE, 
			       "You know all the enchantments on this object. ");
	}
      
      /* Curses known */
      else
	{
	  /* Known uncursed */
	  if (o_ptr->ident & IDENT_UNCURSED)
	    text_out_to_screen(TERM_WHITE, "It is not cursed. ");
	  
	  /* Say if all curses are known */
	  else if (o_ptr->ident & IDENT_KNOW_CURSES) 
	    text_out_to_screen(TERM_WHITE, 
			       "You know all the curses on this object. ");
	}
    }
  
}


/**
 * Place an item description on the screen.
 */
void object_info_screen(object_type *o_ptr, bool fake)
{
  int y, x;
  int y0, x0;
  int i;
  
  bool aware, known, known_effects;
  bool in_store = o_ptr->ident & IDENT_STORE;

  object_kind *k_ptr;
  char o_name[120];
  
  char info_text[2048];
  char *object_kind_info;
  
  /* Initialize object description. */
  strcpy(info_text, "");
  
  /* Get the object kind. */
  k_ptr = &k_info[o_ptr->k_idx];

  /* Hack - make sure the object is really in the store */
  for (i = 0; i < INVEN_TOTAL; i++)
    if (o_ptr == &inventory[i])
      in_store = FALSE;
  
  /* Create and output a status message (hack - not in stores). */
  if (in_store)
    object_desc_store(o_name, o_ptr, TRUE, 3);
  else
    object_desc(o_name, o_ptr, TRUE, 3);
      
  
  /* What is our level of knowledge about the object? */
  aware = object_aware_p(o_ptr);
  known = (object_known_p(o_ptr) || in_store);
  known_effects = k_ptr->known_effect;
  
  /* Object is fully known - give maximal information. */
  if (known || aware)
    {
      /* Fake artifacts in knowledge screens need special treatment */
      if (fake)
	object_desc_spoil(o_name, o_ptr, TRUE, 0);
      else
	/* Get the specific object type's information. */
	object_info(info_text, o_ptr, in_store);
      
      /* No object kind info. */
      object_kind_info = "";
    }
  
  /* Hack - translate if we do that */
  if (Term->xchar_hook)
    xstr_trans(o_name, (Term->xchar_hook(128) == 128));

  /* Save the location */
  Term_locate(&x0, &y0);
   
  /* Save screen */
  screen_save();
  
  /* Label the information. */
  Term_gotoxy(0, 0);
  text_out_indent = 0;
  text_out_to_screen(TERM_WHITE, o_name);
  text_out_to_screen(TERM_WHITE, "\n");
  
  /* Object type or artifact information. */
  text_out_to_screen(TERM_L_BLUE, info_text);
  
  /* 
   * If it is a set item, describe it as such.
   */
  if ((known) && (o_ptr->name1))
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      
      /* Is it a set item? */
      if (a_ptr->set_no)
	{
	  set_type *s_ptr = &s_info[a_ptr->set_no];
	  
	  /* Fully ID description? */
	  text_out_to_screen(TERM_WHITE,"  ");
	  strcpy(info_text, s_text + s_ptr->text);
	  text_out_to_screen(TERM_BLUE, info_text);
	  
	  /* End sentence */
	  text_out_to_screen(TERM_BLUE,".");
	}
    }
  
  /* Describe the object. */
  text_out_to_screen(TERM_WHITE, "\n");
  
  /* Fully describe the object flags and attributes. */
  object_info_detail(o_ptr);
  
  /* Print melee damage info */
  if ((k_ptr->tval == TV_SWORD) || (k_ptr->tval == TV_POLEARM) ||
      (k_ptr->tval == TV_HAFTED) || (k_ptr->tval == TV_DIGGING))
    display_weapon_damage(o_ptr);
  
  /* Print range damage info */
  if ((is_missile(o_ptr)) || (o_ptr->flags_obj & OF_THROWING))
    display_ammo_damage(o_ptr);
  
  /* Print device chance */
  if ((k_ptr->tval == TV_STAFF) || (k_ptr->tval == TV_WAND) ||
      (k_ptr->tval == TV_ROD))
    display_device_chance(o_ptr);
  
  /* Obtain the cursor location */
  (void)Term_locate(&x, &y);
  
  /* Get information about the general object kind. */
  object_kind_info = format("%s", obj_class_info[o_ptr->tval]);
  
  /* Spacing */
  text_out_to_screen(TERM_WHITE, "\n");
  
  /* Object kind information. */
  text_out_to_screen(TERM_WHITE, object_kind_info);
  
  /* Spacing */
  text_out_to_screen(TERM_WHITE, "\n");
  
  /* Hack -- attempt to stay on screen. */
  for (i = y; i < Term->hgt; i++)
    {
      /* No more space! */
      if (i > Term->hgt - 2) break;
      
      /* Advance one line. */
      text_out_to_screen(TERM_WHITE, "\n");
      
      /* Enough clear space.  Done. */
      if (i == (y + 2)) break;
    }
  
  /* The exit sign. */
  text_out_to_screen(TERM_L_BLUE, "(Press any key to continue.)");
  Term_locate(&x, &y);
  text_out_to_screen(TERM_WHITE, "\n");
  Term_gotoxy(x, y);
  (void)inkey_ex();
  
  /* Load screen */
  screen_load();
  Term_gotoxy(x0, y0);
  
}

/**
 * Item description for character sheet/dump.
 * Not used yet - needs new function for displaying this stuff.
 * I have a nasty feeling this whole file needs rewriting...
 */
void object_info_dump(object_type *o_ptr)
{
  object_kind *k_ptr = &k_info[o_ptr->k_idx];
  char info_text[2048];

  /* 
   * If it is a set item, describe it as such.
   */
  if ((o_ptr->ident & IDENT_KNOWN) && (o_ptr->name1))
    {
      artifact_type *a_ptr = &a_info[o_ptr->name1];
      
      /* Is it a set item? */
      if (a_ptr->set_no)
	{
	  set_type *s_ptr = &s_info[a_ptr->set_no];
	  
	  /* Fully ID description? */
	  strcpy(info_text, s_text + s_ptr->text);
	  text_out_to_screen(TERM_BLUE, info_text);
	  
	  /* End sentence */
	  text_out_to_screen(TERM_BLUE,".");
	}
    }
  
  /* Describe the object. */
  text_out_to_screen(TERM_WHITE, "\n");
  
  /* Fully describe the object flags and attributes. */
  object_info_detail(o_ptr);
  
  /* Print melee damage info */
  if ((k_ptr->tval == TV_SWORD) || (k_ptr->tval == TV_POLEARM) ||
      (k_ptr->tval == TV_HAFTED) || (k_ptr->tval == TV_DIGGING))
    display_weapon_damage(o_ptr);
  
  /* Print range damage info */
  if ((is_missile(o_ptr)) || (o_ptr->flags_obj & OF_THROWING))
    display_ammo_damage(o_ptr);
  
  /* Print device chance */
  if ((k_ptr->tval == TV_STAFF) || (k_ptr->tval == TV_WAND) ||
      (k_ptr->tval == TV_ROD))
    display_device_chance(o_ptr);
  
}

/**
 * Describe the "Activation" (if any) for an object or artifact.
 * Return a string, or NULL for "no activation"
 */
char *item_activation(object_type *o_ptr)
{
  /* Require activation ability */
  if (!o_ptr->activation) return (NULL);
  
  /* Some objects and artifacts can be activated */
  switch (o_ptr->activation)
    {
    case ACT_GWINDOR:
      {
	return "illumination (2d15 damage) every 10+d10 turns";
      }
    case ACT_NIMPHELOS:
      {
	return "magic mapping every 40+d40 turns";
      }
    case ACT_AVALLONE:
      {
	return "clairvoyance every 120+d120 turns";
      }
    case ACT_DWARVES:
      {
	return "level clairvoyance every 200+d200 turns";
      }
    case ACT_CARLAMMAS:
      {
	return "protection from evil every 225+d225 turns";
      }
    case ACT_INGWE:
      {
	return "dispel evil (x4) every 300+d300 turns";
      }
    case ACT_BOROMIR:
      {
	return "frighten monsters every 40+d40 turns";
      }
    case ACT_ELESSAR:
      {
	return "heal (500) and restore life levels every 500 turns";
      }
    case ACT_TULKAS:
      {
	return "haste self (75+d75 turns) every 150+d150 turns";
      }
    case ACT_NARYA:
      {
	return "large fire ball (225) every 275+d275 turns";
      }
    case ACT_NENYA:
      {
	return "large frost ball (250) every 325+d325 turns";
      }
    case ACT_VILYA:
      {
	return "large lightning ball (275) every 375+d375 turns";
      }
    case ACT_STONE_LORE:
      {
	return "perilous identify every turn";
      }
    case ACT_RAZORBACK:
      {
	return "Assume Dragonform; Activation in Dragonform: star ball (150) every 1000 turns";
      }
    case ACT_BLADETURNER:
      {
	return "Assume Dragonform; Activation in Dragonform: heroism, bless, and resistance every 400 turns";
      }
    case ACT_SOULKEEPER:
      {
	return "heal (1000) every 888 turns";
      }
    case ACT_BELEGENNON:
      {
	return "phase door every 2 turns";
      }
    case ACT_CELEBORN:
      {
	return "genocide every 500 turns";
      }
    case ACT_CASPANION:
      {
	return "door destruction every turn";
      }
    case ACT_HIMRING:
      {
	return "protection from evil every 200 + d200 turns";
      }
    case ACT_ELEMENTS:
      {
	return "protection from the elements every 160 turns";
      }
    case ACT_EARENDIL:
      {
	return "curing every 100 turns";
      }
    case ACT_GIL_GALAD:
      {
	return "blinding light (75) every 250 turns";
      }
    case ACT_HOLHENNETH:
      {
	return "detection every 55+d55 turns";
      }
    case ACT_VINYAMAR:
      {
	return "heroism every 200 turns";
      }
    case ACT_GONDOR:
      {
	return "heal (500) every 500 turns";
      }
    case ACT_VALINOR:
      {
	return "resistance (20+d20 turns) every 200 turns";
      }
    case ACT_HOLCOLLETH:
      {
	return "Sleep II every 55 turns";
      }
    case ACT_THINGOL:
      {
	return "recharge magical device every 90 turns";
      }
    case ACT_COLANNON:
      {
	return "teleport (100) every 45 turns";
      }
    case ACT_LUTHIEN:
      {
	return "restore life levels every 450 turns";
      }
    case ACT_CAMMITHRIM:
      {
	return "spear of light (4d5) every 5 turns";
      }
    case ACT_EOL:
      {
	return "mana bolt (9d8) every 7+d7 turns";
      }
    case ACT_PAURNIMMEN:
      {
	return "add cold damage to your melee attacks (50 turns) every 100+d50 turns";
      }
    case ACT_PAURNEN:
      {
	return "add acid damage to your melee attacks (30 turns) every 100+d50 turns";
      }
    case ACT_FINGOLFIN:
      {
	return "instant tunnelling every 3+d3 turns";
      }
    case ACT_FEANOR:
      {
	return "haste self (20+d20 turns) every 200 turns";
      }
    case ACT_DAL:
      {
	return "remove fear and cure poison every 5 turns";
      }
    case ACT_GIMLI:
      {
	return "magic mapping every 35+d35 turns";
      }
    case ACT_NARTHANC:
      {
	return "fire bolt (6d8) every 7+d7 turns";
      }
    case ACT_NIMTHANC:
      {
	return "frost bolt (5d8) every 6+d6 turns";
      }
    case ACT_DETHANC:
      {
	return "lightning bolt (4d8) every 5+d5 turns";
      }
    case ACT_RILIA:
      {
	return "stinking cloud (12) every 4+d4 turns";
      }
    case ACT_BELANGIL:
      {
	return "frost ball (3 * level / 2) every 5+d5 turns";
      }
    case ACT_ARUNRUTH:
      {
	return "frost bolt (12d8) every 300 turns";
      }
    case ACT_RINGIL:
      {
	return "frost cone (250) every 200 turns";
      }
    case ACT_ANDURIL:
      {
	return "fire ball (150) every 200 turns";
      }
    case ACT_MANWE:
      {
	return "wide arc of force (300) every 1000 turns";
      }
    case ACT_THEODEN:
      {
	return "drain life (120) every 400 turns";
      }
    case ACT_AEGLOS:
      {
	return "frost ball (100) every 500 turns";
      }
    case ACT_OROME:
      {
	return "stone to mud every 5 turns";
      }
    case ACT_EONWE:
      {
	return "mass genocide every 1000 turns";
      }
    case ACT_LOTHARANG:
      {
	return "cure wounds (4d12) every 3+d3 turns";
      }
    case ACT_ULMO:
      {
	return "teleport away every 75 turns (distance: 45-60+)";
      }
    case ACT_AVAVIR:
      {
	return "word of recall every 200 turns";
      }
    case ACT_TOTILA:
      {
	return "confuse monster every 15 turns";
      }
    case ACT_FIRESTAR:
      {
	return "large fire ball (125) every 150 turns";
      }
    case ACT_TARATOL:
      {
	return "haste self (20+d20 turns) every 100+d100 turns";
      }
    case ACT_ERIRIL:
      {
	return "identify every 10 turns";
      }
    case ACT_OLORIN:
      {
	return "probing every 20 turns";
      }
    case ACT_TURMIL:
      {
	return "drain life (90) every 70 turns";
      }
    case ACT_HARAD:
      {
	return "a deadly shot every 200+d200 turns";
      }
    case ACT_CUBRAGOL:
      {
	return "fire branding of bolts every 999 turns";
      }
    case ACT_BUCKLAND:
      {
	return "elemental branding of shots every 1500 turns";
      }
      
    case ACT_DRAGON_BLUE:
      {
	return "assume dragon form; Activation in dragon form: breathe lightning very 50+d50 turns";
      }
    case ACT_DRAGON_WHITE:
      {
	return "assume dragon form; Activation in dragon form: breathe frost every 50+d50 turns";
      }
    case ACT_DRAGON_BLACK:
      {
	return "assume dragon form; Activation in dragon form: breathe acid every 50+d50 turns";
      }
    case ACT_DRAGON_GREEN:
      {
	return "assume dragon form; Activation in dragon form: breathe poison gas every 50+d50 turns";
      }
    case ACT_DRAGON_RED:
      {
	return "assume dragon form; Activation in dragon form: breathe fire every 50+d50 turns";
      }
    case ACT_DRAGON_MULTIHUED:
      {
	return "assume dragon form; Activation in dragon form: breathe an element or poison every 50+d50 turns";
      }
    case ACT_DRAGON_BRONZE:
      {
	return "assume dragon form; Activation in dragon form: breathe confusion every 50+d50 turns";
      }
    case ACT_DRAGON_GOLD:
      {
	return "assume dragon form; Activation in dragon form: breathe sound every 50+d50 turns";
      }
    case ACT_DRAGON_CHAOS:
      {
	return "assume dragon form; Activation in dragon form: breathe chaos/disenchant every 50+d50 turns";
      }
    case ACT_DRAGON_LAW:
      {
	return "assume dragon form; Activation in dragon form: breathe sound/shards every 50+d50 turns";
      }
    case ACT_DRAGON_BALANCE:
      {
	return "assume dragon form; Activation in dragon form: breathe balance every 50+d50 turns";
      }
    case ACT_DRAGON_SHINING:
      {
	return "assume dragon form; Activation in dragon form: breathe light/darkness every 50+d50 turns";
      }
    case ACT_DRAGON_POWER:
      {
	return "assume dragon form; Activation in dragon form: breathe the elements every 50+d50 turns";
      }
      
    case ACT_RING_ACID:
      {
	return "cast a acid ball(80) and oppose acid every 50+d100 turns";
      }
    case ACT_RING_COLD:
      {
	return "cast a cold ball(80) and oppose cold every 50+d100 turns";
      }
    case ACT_RING_FIRE:
      {
	return "cast a fire ball(80) and oppose fire every 50+d100 turns";
      }
    case ACT_RING_ELEC:
      {
	return "cast a electricity ball(80) and oppose electricity every 50+d100 turns";
      }
    case ACT_RING_POIS:
      {
	return "cast a poison ball(80) and oppose poison every 50+d100 turns";
      }
    case ACT_AMULET_ESCAPING:
      {
	return "teleport(40) every 40+d40 turns";
      }
    case ACT_AMULET_LION:
      {
	return "become lion every 200 turns";
      }
    case ACT_AMULET_METAMORPH:
      {
	return "Make a random shapechange every 300 turns";
      }
      
    case ACT_RANDOM_FIRE1:
      {
	return "fire bolt (3 + level / 8)d8 every 7+d7 turns";
      }
    case ACT_RANDOM_FIRE2:
      {
	return "sphere of fire (90) every 250 turns";
      }
    case ACT_RANDOM_FIRE3:
      {
	return "fire storm (150) every 600 turns";
      }
    case ACT_RANDOM_COLD1:
      {
	return "frost bolt (3 + level / 8)d8 every 7+d7 turns";
      }
    case ACT_RANDOM_COLD2:
      {
	return "sphere of frost (90) every 250 turns";
      }
    case ACT_RANDOM_COLD3:
      {
	return "frost storm (150) every 600 turns";
      }
    case ACT_RANDOM_ACID1:
      {
	return "acid bolt (3 + level / 8)d8 every 7+d7 turns";
      }
    case ACT_RANDOM_ACID2:
      {
	return "sphere of acid (90) every 300 turns";
      }
    case ACT_RANDOM_ACID3:
      {
	return "acid storm (160) every 800 turns";
      }
    case ACT_RANDOM_ELEC1:
      {
	return "electricity bolt (3 + level / 8)d8 every 7+d7 turns";
      }
    case ACT_RANDOM_ELEC2:
      {
	return "ball lightning (100) every 300 turns";
      }
    case ACT_RANDOM_ELEC3:
      {
	return "lightning strike (130+25) every 800 turns";
      }
    case ACT_RANDOM_POIS1:
      {
	return "poison dart (3 + level / 10)d8 every 22+d22 turns";
      }
    case ACT_RANDOM_POIS2:
      {
	return "poison cloud (110) every 300 turns";
      }
    case ACT_RANDOM_LIGHT1:
      {
	return "blinding ball of light (50+10) every 250 turns";
      }
    case ACT_RANDOM_LIGHT2:
      {
	return "dispel light-hating (175) every 600 turns";
      }
      
    case ACT_RANDOM_DISPEL_UNDEAD:
      {
	return "dispel undead (100) every 300 turns";
      }
    case ACT_RANDOM_DISPEL_EVIL:
      {
	return "dispel evil (100) every 400 turns";
      }
    case ACT_RANDOM_SMITE_UNDEAD:
      {
	return "dispel an undead (level / 4)d33 every 200 turns";
      }
    case ACT_RANDOM_SMITE_DEMON:
      {
	return "dispel a demon (level / 4)d33 every 200 turns";
      }
    case ACT_RANDOM_SMITE_DRAGON:
      {
	return "dispel a dragon (level / 4)d33 every 200 turns";
      }
    case ACT_RANDOM_HOLY_ORB:
      {
	return "holy orb (60) every 175 turns";
      }
    case ACT_RANDOM_BLESS:
      {
	return "blessing (24+d24) every 200 turns";
      }
    case ACT_RANDOM_FRIGHTEN_ALL:
      {
	return "frighten adversaries every 120+d120 turns";
      }
    case ACT_RANDOM_HEAL1:
      {
	return "heal (5d20) every 85 turns";
      }
    case ACT_RANDOM_HEAL2:
      {
	return "heal (7d40) every 225 turns";
      }
    case ACT_RANDOM_HEAL3:
      {
	return "heal (10d60) every 500 turns";
      }
    case ACT_RANDOM_CURE:
      {
	return "cure ailments every 500 turns";
      }
    case ACT_RANDOM_PROT_FROM_EVIL:
      {
	return "protection from evil (24+d24) every 250 turns";
      }
    case ACT_RANDOM_CHAOS:
      {
	return "chaos ball (d300) every 600 turns";
      }
    case ACT_RANDOM_SHARD_SOUND:
      {
	return "shard or sound ball (150) every 600 turns";
      }
    case ACT_RANDOM_NETHR:
      {
	return "nether orb (100) every 400 turns";
      }
    case ACT_RANDOM_LINE_LIGHT:
      {
	return "ray of light (4d5) every 6+d6 turns";
      }
    case ACT_RANDOM_STARLIGHT:
      {
	return "starlight (4d5) every 8+d8 turns";
      }
    case ACT_RANDOM_EARTHQUAKE:
      {
	return "earthquake (radius 10) every 40+d40 turns";
      }
    case ACT_RANDOM_IDENTIFY:
      {
	return "identify every 30 turns";
      }
    case ACT_RANDOM_SPEED:
      {
	return "haste self (20+d20) every 120+d120 turns";
      }
    case ACT_RANDOM_TELEPORT_AWAY:
      {
	return "teleport away every 110 turns (distance: 45-60+)";
      }
    case ACT_RANDOM_HEROISM:
      {
	return "heroism every 200 turns";
      }
    case ACT_RANDOM_STORM_DANCE:
      {
	return "storm dance every 300 turns";
      }
    case ACT_RANDOM_RESIST_ELEMENTS:
      {
	return "resistance to the elements every 400 turns";
      }
    case ACT_RANDOM_RESIST_ALL:
      {
	return "resistance every 400 turns";
      }
    case ACT_RANDOM_TELEPORT1:
      {
	return "teleport self (30) every 10+d10 turns";
      }
    case ACT_RANDOM_TELEPORT2:
      {
	return "major displacement (200) every 80 turns";
      }
    case ACT_RANDOM_RECALL:
      {
	return "recall every 350 turns";
      }
    case ACT_RANDOM_REGAIN:
      {
	return "restore level every 800 turns";
      }
    case ACT_RANDOM_RESTORE:
      {
	return "restore stats every 800 turns";
      }
    case ACT_RANDOM_SHIELD:
      {
	return "magic shield every 400 turns";
      }
    case ACT_RANDOM_BRAND_MISSILE:
      {
	return "brand missiles every 1750 turns";
      }
    case ACT_RANDOM_SUPER_SHOOTING:
      {
	return "an especially deadly shot every 200+d200 turns";
      }
    case ACT_RANDOM_DETECT_MONSTERS:
      {
	return "detect monsters every 4+d4 turns";
      }
    case ACT_RANDOM_DETECT_EVIL:
      {
	return "detect evil every 4+d4 turns";
      }
    case ACT_RANDOM_DETECT_ALL:
      {
	return "detection every 30+d30 turns";
      }
    case ACT_RANDOM_MAGIC_MAP:
      {
	return "sense surroundings every 30+d30 turns";
      }
    case ACT_RANDOM_DETECT_D_S_T:
      {
	return "detect traps, doors, and stairs every 10+d10 turns";
      }
    case ACT_RANDOM_CONFU_FOE:
      {
	return "strong confuse monster every 250 turns";
      }
    case ACT_RANDOM_SLEEP_FOE:
      {
	return "strong sleep monster every 250 turns";
      }
    case ACT_RANDOM_TURN_FOE:
      {
	return "strong frighten monster every 250 turns";
      }
    case ACT_RANDOM_SLOW_FOE:
      {
	return "strong slow monster every 250 turns";
      }
    case ACT_RANDOM_BANISH_EVIL:
      {
	return "banish evil every 400 turns";
      }
    case ACT_RANDOM_DISARM:
      {
	return "disarming every 7+d7 turns";
      }
    case ACT_RANDOM_CONFU_FOES:
      {
	return "confuse monsters every 300 turns";
      }
    case ACT_RANDOM_SLEEP_FOES:
      {
	return "sleep monsters every 300 turns";
      }
    case ACT_RANDOM_TURN_FOES:
      {
	return "frighten monsters every 300 turns";
      }
    case ACT_RANDOM_SLOW_FOES:
      {
	return "slow monsters every 300 turns";
      }
     case ACT_POWER_ACID_BLAST:
      {
	return "acid blast every 1500 turns";
      }
    case ACT_POWER_CHAIN_LIGHTNING:
      {
	return "chain lightning every 2000 turns";
      }
    case ACT_POWER_LAVA_POOL:
      {
	return "lava pool every 2000 turns";
      }
    case ACT_POWER_ICE_WHIRLPOOL:
      {
	return "whirlpool of ice every 1000 turns";
      }
    case ACT_POWER_GROW_FOREST:
      {
	return "grow forest every 2000 turns";
      }
    case ACT_POWER_RESTORE_AND_ENHANCE:
      {
	return "restore and enhance you every 3000 turns";
      }
    case ACT_POWER_ZONE_OF_CHAOS:
      {
	return "create a zone of chaos every 3000 turns";
      }
    case ACT_POWER_PRESSURE_WAVE:
      {
	return "pressure wave every 1500 turns";
      }
    case ACT_POWER_ENERGY_DRAIN:
      {
	return "drain energy every 3000 turns";
      }
    case ACT_POWER_MASS_STASIS:
      {
	return "mass stasis every 1000 turns";
      }
    case ACT_POWER_LIGHT_FROM_ABOVE:
      {
	return "light from above every 1500 turns";
      }
    case ACT_POWER_MASS_POLYMORPH:
      {
	return "mass polymorph every 2500 turns";
      }
    case ACT_POWER_GRAVITY_WAVE:
      {
	return "gravity wave every 2000 turns";
      }
    case ACT_POWER_ENLIST_EARTH:
      {
	return "summon allies from the Earth every 3000 turns";
      }
    case ACT_POWER_TELEPORT_ALL:
      {
	return "mass teleport every 2000 turns";
      }
     
    case ACT_BALROG_WHIP:
      {
	return "lash out at at range 2 for base weapon damage (unless target resists fire)";
      }
    case ACT_MAGESTAFF:
      {
	return "restore 10 spell points every 15+d10 turns";
      }
    default:
      {
	return "an undocumented activation";
      }
    }
  
  
  /* Nothing else activates. */
  return NULL;
}



/**
 * Hack -- acquire self knowledge.  Idea originally from Nethack.
 *
 * List various information about the player and/or his current equipment.
 *
 * See also identify_fully().
 *
 * Use the roff() routines, perhaps.  XXX XXX XXX
 *
 * Use the show_file() method, perhaps.  XXX XXX XXX
 */
void self_knowledge(bool spoil)
{
  int i = 0, j, k;
  
  object_type *o_ptr;
  
  u32b flags_obj = 0L, flags_curse = 0L;

  char *info[128];
  
  bool stat[A_MAX], other[MAX_P_BONUS], slay[MAX_P_SLAY], brand[MAX_P_BRAND];
  
  /* Initialise */
  for (j = 0; j < A_MAX; j++) stat[j] = FALSE;
  for (j = 0; j < MAX_P_BONUS; j++) other[j] = FALSE;
  for (j = 0; j < MAX_P_SLAY; j++) slay[j] = FALSE;
  for (j = 0; j < MAX_P_BRAND; j++) brand[j] = FALSE;
  
  /* Acquire item flags from equipment */
  for (k = INVEN_WIELD; k < INVEN_SUBTOTAL; k++)
    {
      o_ptr = &inventory[k];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* Extract the flags */
      if (spoil)
	{
	  flags_obj |= o_ptr->flags_obj;
	  flags_curse |= o_ptr->flags_curse;
	}

      /* Bonuses, multiples */
      for (j = 0; j < A_MAX; j++)
	if (o_ptr->bonus_stat[j] != 0) stat[j] = TRUE;
      for (j = 0; j < MAX_P_BONUS; j++)
	if (o_ptr->bonus_other[j] != 0) other[j] = TRUE;
      for (j = 0; j < MAX_P_SLAY; j++)
	if (o_ptr->multiple_slay[j] > MULTIPLE_BASE) slay[j] = TRUE;
      for (j = 0; j < MAX_P_BRAND; j++)
	if (o_ptr->multiple_brand[j] > MULTIPLE_BASE) brand[j] = TRUE;
    }
  
  switch (p_ptr->schange)
    {
    case SHAPE_MOUSE:
      info[i++] = "You are wearing the body of a mouse.";
      break;
    case SHAPE_FERRET:
      info[i++] = "You are wearing the body of a ferret.";
      break;
    case SHAPE_HOUND:
      info[i++] = "You are wearing the body of a hound.";
      break;
    case SHAPE_GAZELLE:
      info[i++] = "You are wearing the body of a gazelle.";
      break;
    case SHAPE_LION:
      info[i++] = "You are wearing the body of a lion.";
      break;
    case SHAPE_ENT:
      info[i++] = "You are wearing the body of a ent.";
      break;
    case SHAPE_BAT:
      info[i++] = "You are wearing the body of a bat.";
      break;
    case SHAPE_WEREWOLF:
      info[i++] = "You are wearing the body of a werewolf.";
      break;	
    case SHAPE_VAMPIRE:
      info[i++] = "You are wearing the body of a vampire.";
      break;
    case SHAPE_WYRM:
      info[i++] = "You are wearing the body of a dragon.";
      break;
    case SHAPE_BEAR:
      info[i++] = "You are wearing the body of a bear.";
      break;
    }
  
  if (p_ptr->blind)
    {
      info[i++] = "You cannot see.";
    }
  if (p_ptr->confused)
    {
      info[i++] = "You are confused.";
    }
  if (p_ptr->afraid)
    {
      info[i++] = "You are terrified.";
    }
  if (p_ptr->cut)
    {
      info[i++] = "You are bleeding.";
    }
  if (p_ptr->stun)
    {
      info[i++] = "You are stunned.";
    }
  if (p_ptr->poisoned)
    {
      info[i++] = "You are poisoned.";
    }
  if (p_ptr->image)
    {
      info[i++] = "You are hallucinating.";
    }
  
  if (p_ptr->aggravate)
    {
      info[i++] = "You aggravate monsters.";
    }
  if (p_ptr->teleport)
    {
      info[i++] = "Your position is very uncertain.";
    }
  
  if (p_ptr->blessed)
    {
      info[i++] = "You feel righteous.";
    }
  if (p_ptr->hero)
    {
      info[i++] = "You feel heroic.";
    }
  if (p_ptr->shero)
    {
      info[i++] = "You are in a battle rage.";
    }
  if (p_ptr->protevil)
    {
      info[i++] = "You are protected from evil.";
    }
  if (p_ptr->shield)
    {
      info[i++] = "You are protected by a mystic shield.";
    }
  if (p_ptr->magicdef)
    {
      info[i++] = "You have enhanced protection from the spells of others.";
    }
  if (p_ptr->searching)
    {
      info[i++] = "You are looking around very carefully.";
    }
  if (p_ptr->new_spells)
    {
      info[i++] = "You can learn some spells/prayers.";
    }
  if (p_ptr->new_specialties)
    {
      info[i++] = "You can learn some specialty abilities.";
    }
  if (p_ptr->word_recall)
    {
      info[i++] = "You will soon be recalled.";
    }
  if (p_ptr->see_infra)
    {
      info[i++] = "Your eyes are sensitive to infrared light.";
    }
  
  if (p_ptr->slow_digest)
    {
      info[i++] = "Your appetite is small.";
    }
  if (p_ptr->ffall)
    {
      info[i++] = "You land gently.";
    }
  if (p_ptr->lite)
    {
      info[i++] = "You are glowing with light.";
    }
  if (p_ptr->regenerate)
    {
      info[i++] = "You regenerate quickly.";
    }
  if (p_ptr->telepathy)
    {
      info[i++] = "You have ESP.";
    }
  if (p_ptr->see_inv)
    {
      info[i++] = "You can see invisible creatures.";
    }
  if (p_ptr->free_act)
    {
      info[i++] = "You have free action.";
    }
  if (p_ptr->hold_life)
    {
      info[i++] = "You have a firm hold on your life force.";
    }

  if (p_immune(P_RES_ACID)) info[i++] = "You are immune to acid.";
  else if (p_resist_strong(P_RES_ACID)) 
    info[i++] = "You resist acid exceptionally well.";
  else if (p_resist_good(P_RES_ACID)) info[i++] = "You resist acid.";
  else if (p_resist_pos(P_RES_ACID)) info[i++] = "You resist acid slightly.";
  else if (p_vulnerable(P_RES_ACID)) info[i++] = "You are vulnerable to acid.";
  
  if (p_immune(P_RES_ELEC)) info[i++] = "You are immune to lightning.";
  else if (p_resist_strong(P_RES_ELEC)) 
    info[i++] = "You resist lightning exceptionally well.";
  else if (p_resist_good(P_RES_ELEC)) info[i++] = "You resist lightning.";
  else if (p_resist_pos(P_RES_ELEC)) 
    info[i++] = "You resist lightning slightly.";
  else if (p_vulnerable(P_RES_ELEC)) 
    info[i++] = "You are vulnerable to lightning.";
  
  if (p_immune(P_RES_FIRE)) info[i++] = "You are immune to fire.";
  else if (p_resist_strong(P_RES_FIRE)) 
    info[i++] = "You resist fire exceptionally well.";
  else if (p_resist_good(P_RES_FIRE)) info[i++] = "You resist fire.";
  else if (p_resist_pos(P_RES_FIRE)) info[i++] = "You resist fire slightly.";
  else if (p_vulnerable(P_RES_FIRE)) info[i++] = "You are vulnerable to fire.";
  
  if (p_immune(P_RES_COLD)) info[i++] = "You are immune to cold.";
  else if (p_resist_strong(P_RES_COLD)) 
    info[i++] = "You resist cold exceptionally well.";
  else if (p_resist_good(P_RES_COLD)) info[i++] = "You resist cold.";
  else if (p_resist_pos(P_RES_COLD)) info[i++] = "You resist cold slightly.";
  else if (p_vulnerable(P_RES_COLD)) info[i++] = "You are vulnerable to cold.";
  
  if (p_immune(P_RES_POIS)) info[i++] = "You are immune to poison.";
  else if (p_resist_strong(P_RES_POIS)) 
    info[i++] = "You resist poison exceptionally well.";
  else if (p_resist_good(P_RES_POIS)) info[i++] = "You resist poison.";
  else if (p_resist_pos(P_RES_POIS)) info[i++] = "You resist poison slightly.";
  else if (p_vulnerable(P_RES_POIS)) 
    info[i++] = "You are vulnerable to poison.";
  
  if (p_immune(P_RES_LITE)) info[i++] = "You are immune to light.";
  else if (p_resist_strong(P_RES_LITE)) 
    info[i++] = "You resist light exceptionally well.";
  else if (p_resist_good(P_RES_LITE)) info[i++] = "You resist light.";
  else if (p_resist_pos(P_RES_LITE)) info[i++] = "You resist light slightly.";
  else if (p_vulnerable(P_RES_LITE)) 
    info[i++] = "You are vulnerable to light.";
  
  if (p_immune(P_RES_DARK)) info[i++] = "You are immune to darkness.";
  else if (p_resist_strong(P_RES_DARK)) 
    info[i++] = "You resist darkness exceptionally well.";
  else if (p_resist_good(P_RES_DARK)) info[i++] = "You resist darkness.";
  else if (p_resist_pos(P_RES_DARK)) 
    info[i++] = "You resist darkness slightly.";
  else if (p_vulnerable(P_RES_DARK)) 
    info[i++] = "You are vulnerable to darkness.";
  
  if (p_immune(P_RES_CONFU)) info[i++] = "You are immune to confusion.";
  else if (p_resist_strong(P_RES_CONFU)) 
    info[i++] = "You resist confusion exceptionally well.";
  else if (p_resist_good(P_RES_CONFU)) info[i++] = "You resist confusion.";
  else if (p_resist_pos(P_RES_CONFU)) 
    info[i++] = "You resist confusion slightly.";
  else if (p_vulnerable(P_RES_CONFU)) 
    info[i++] = "You are vulnerable to confusion.";
  
  if (p_immune(P_RES_SOUND)) info[i++] = "You are immune to sound.";
  else if (p_resist_strong(P_RES_SOUND)) 
    info[i++] = "You resist sound exceptionally well.";
  else if (p_resist_good(P_RES_SOUND)) info[i++] = "You resist sound.";
  else if (p_resist_pos(P_RES_SOUND)) info[i++] = "You resist sound slightly.";
  else if (p_vulnerable(P_RES_SOUND)) 
    info[i++] = "You are vulnerable to sound.";
  
  if (p_immune(P_RES_SHARD)) info[i++] = "You are immune to blasts of shards.";
  else if (p_resist_strong(P_RES_SHARD)) 
    info[i++] = "You resist blasts of shards exceptionally well.";
  else if (p_resist_pos(P_RES_SHARD)) 
    info[i++] = "You resist blasts of shards.";
  else if (p_resist_good(P_RES_SHARD)) 
    info[i++] = "You slightly resist blasts of shards.";
  else if (p_vulnerable(P_RES_SHARD)) 
    info[i++] = "You are vulnerable to blasts of shards.";
  
  if (p_immune(P_RES_NEXUS)) info[i++] = "You are immune to nexus attacks.";
  else if (p_resist_strong(P_RES_NEXUS)) 
    info[i++] = "You resist nexus attacks exceptionally well.";
  else if (p_resist_good(P_RES_NEXUS)) info[i++] = "You resist nexus attacks.";
  else if (p_resist_pos(P_RES_NEXUS)) 
    info[i++] = "You resist nexus attacks slightly.";
  else if (p_vulnerable(P_RES_NEXUS)) 
    info[i++] = "You are vulnerable to nexus attacks.";
  
  if (p_immune(P_RES_NETHR)) info[i++] = "You are immune to nether forces.";
  else if (p_resist_strong(P_RES_NETHR)) 
    info[i++] = "You resist nether forces exceptionally well.";
  else if (p_resist_good(P_RES_NETHR)) info[i++] = "You resist nether forces.";
  else if (p_resist_pos(P_RES_NETHR)) 
    info[i++] = "You resist nether forces slightly.";
  else if (p_vulnerable(P_RES_NETHR)) 
    info[i++] = "You are vulnerable to nether forces.";
  
  if (p_immune(P_RES_CHAOS)) info[i++] = "You are immune to chaos.";
  else if (p_resist_strong(P_RES_CHAOS)) 
    info[i++] = "You resist chaos exceptionally well.";
  else if (p_resist_good(P_RES_CHAOS)) info[i++] = "You resist chaos.";
  else if (p_resist_pos(P_RES_CHAOS)) info[i++] = "You resist chaos slightly.";
  else if (p_vulnerable(P_RES_CHAOS)) 
    info[i++] = "You are vulnerable to chaos.";
  
  if (p_immune(P_RES_DISEN)) info[i++] = "You are immune to disenchantment.";
  else if (p_resist_strong(P_RES_DISEN)) 
    info[i++] = "You resist disenchantment exceptionally well.";
  else if (p_resist_good(P_RES_DISEN)) 
    info[i++] = "You resist disenchantment.";
  else if (p_resist_pos(P_RES_DISEN)) 
    info[i++] = "You resist disenchantment slightly.";
  else if (p_vulnerable(P_RES_DISEN)) 
    info[i++] = "You are vulnerable to disenchantment.";

  if (p_ptr->no_fear)
    {
      info[i++] = "You are completely fearless.";
    }
  if (p_ptr->no_blind)
    {
      info[i++] = "Your eyes are resistant to blindness.";
    }
  
  if (p_ptr->sustain_str)
    {
      info[i++] = "Your strength is sustained.";
    }
  if (p_ptr->sustain_int)
    {
      info[i++] = "Your intelligence is sustained.";
    }
  if (p_ptr->sustain_wis)
    {
      info[i++] = "Your wisdom is sustained.";
    }
  if (p_ptr->sustain_con)
    {
      info[i++] = "Your constitution is sustained.";
    }
  if (p_ptr->sustain_dex)
    {
      info[i++] = "Your dexterity is sustained.";
    }
  if (p_ptr->sustain_chr)
    {
      info[i++] = "Your charisma is sustained.";
    }
  
  if (stat[A_STR])
    {
      info[i++] = "Your strength is affected by your equipment.";
    }
  if (stat[A_INT])
    {
      info[i++] = "Your intelligence is affected by your equipment.";
    }
  if (stat[A_WIS])
    {
      info[i++] = "Your wisdom is affected by your equipment.";
    }
  if (stat[A_DEX])
    {
      info[i++] = "Your dexterity is affected by your equipment.";
    }
  if (stat[A_CON])
    {
      info[i++] = "Your constitution is affected by your equipment.";
    }
  if (stat[A_CHR])
    {
      info[i++] = "Your charisma is affected by your equipment.";
    }
  
  if (other[P_BONUS_M_MASTERY])
    {
      info[i++] = "Your device skill is affected by your equipment.";
    }
  if (other[P_BONUS_STEALTH])
    {
      info[i++] = "Your stealth is affected by your equipment.";
    }
  if (other[P_BONUS_SEARCH])
    {
      info[i++] = "Your searching ability is affected by your equipment.";
    }
  if (other[P_BONUS_INFRA])
    {
      info[i++] = "Your infravision is affected by your equipment.";
    }
  if (other[P_BONUS_TUNNEL])
    {
      info[i++] = "Your digging ability is affected by your equipment.";
    }
  if (other[P_BONUS_SPEED])
    {
      info[i++] = "Your speed is affected by your equipment.";
    }
  if (other[P_BONUS_SHOTS])
    {
      info[i++] = "Your shooting speed is affected by your equipment.";
    }
  if (other[P_BONUS_MIGHT])
    {
      info[i++] = "Your shooting might is affected by your equipment.";
    }
  
  
  /* Special "Attack Bonuses" */
  if (brand[P_BRAND_ACID])
    {
      info[i++] = "You melt your foes.";
    }
  if (brand[P_BRAND_ELEC])
    {
      info[i++] = "You shock your foes.";
    }
  if (brand[P_BRAND_FIRE])
    {
	  info[i++] = "You burn your foes.";
    }
  if (brand[P_BRAND_COLD])
    {
      info[i++] = "You freeze your foes.";
    }
  if (brand[P_BRAND_POIS])
    {
      info[i++] = "You poison your foes.";
    }
  
  /* Special "slay" flags */
  if (slay[P_SLAY_ANIMAL])
    {
      info[i++] = "You strike at animals with extra force.";
    }
  if (slay[P_SLAY_EVIL])
    {
      info[i++] = "You strike at evil with extra force.";
    }
  if (slay[P_SLAY_UNDEAD])
    {
      info[i++] = "You strike at undead with holy wrath.";
    }
  if (slay[P_SLAY_DEMON])
    {
      info[i++] = "You strike at demons with holy wrath.";
    }
  if (slay[P_SLAY_ORC])
    {
      info[i++] = "You are especially deadly against orcs.";
    }
  if (slay[P_SLAY_TROLL])
    {
      info[i++] = "You are especially deadly against trolls.";
    }
  if (slay[P_SLAY_GIANT])
    {
      info[i++] = "You are especially deadly against giants.";
    }
  if (slay[P_SLAY_DRAGON])
    {
      info[i++] = "You are especially deadly against dragons.";
    }
  
  /* Indicate Blessing */
  if (flags_obj & (OF_BLESSED))
    {
      info[i++] = "Your weapon has been blessed by the gods.";
    }
  
  /* Hack */
  if (flags_obj & (OF_IMPACT))
    {
      info[i++] = "Your weapon can induce earthquakes.";
    }

  
  
  /* Save screen */
  screen_save();
  

  /* Clear the screen */
  Term_clear();
  
  /* Label the information */
  prt("     Your Attributes:", 1, 0);
  
  /* Dump the info */
  for (k = 2, j = 0; j < i; j++)
    {
      /* Show the info */
      prt(info[j], k++, 0);
      
      /* Page wrap */
      if ((k == Term->hgt - 2) && (j+1 < i))
	{
	  prt("-- more --", k, 0);
	  inkey_ex();
	  
	  /* Clear the screen */
	  Term_clear();
	  
	  /* Label the information */
	  prt("     Your Attributes:", 1, 0);
	  
	  /* Reset */
	  k = 2;
	}
    }
  
  /* Pause */
  prt("[Press any key to continue]", k, 0);
  (void)inkey_ex();
  
  
  /* Load screen */
  screen_load();
}




/**
 * Returns chance of failure for a spell
 */
s16b spell_chance(int spell)
{
  int chance, minfail;
  
  magic_type *s_ptr;
  
  
  /* Paranoia -- must be literate */
  if (!mp_ptr->spell_book) return (100);
  
  /* Access the spell */
  s_ptr = &mp_ptr->info[spell];
  
  /* Extract the base spell failure rate */
  chance = s_ptr->sfail;
  
  /* Reduce failure rate by "effective" level adjustment */
  chance -= 4 * (p_ptr->lev - s_ptr->slevel);
  
  /* Reduce failure rate by INT/WIS adjustment */
  chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
  
  /* Not enough mana to cast */
  if (s_ptr->smana > p_ptr->csp)
    {
      chance += 5 * (s_ptr->smana - p_ptr->csp);
    }
  
  /* Extract the minimum failure rate */
  minfail = adj_mag_fail[p_ptr->stat_ind[mp_ptr->spell_stat]];
  
  /* Minimum failure rate */
  if (chance < minfail) chance = minfail;
  
  /* Stunning makes spells harder (after minfail) */
  if (p_ptr->stun > 50) chance += 20;
  else if (p_ptr->stun) chance += 10;
  
  /* Always a 5 percent chance of working */
  if (chance > 95) chance = 95;
  
  /* Return the chance */
  return (chance);
}



/**
 * Determine if a spell is "okay" for the player to cast or study
 * The spell must be legible, not forgotten, and also, to cast,
 * it must be known, and to study, it must not be known.
 */
bool spell_okay(int spell, bool known)
{
  magic_type *s_ptr;
  
  /* Access the spell */
  s_ptr = &mp_ptr->info[spell];
  
  /* Spell is illegal */
  if (s_ptr->slevel > p_ptr->lev) return (FALSE);
  
  /* Spell is forgotten */
  if ((spell < 32) ?
      (p_ptr->spell_forgotten1 & (1L << spell)) :
      (p_ptr->spell_forgotten2 & (1L << (spell - 32))))
    {
      /* Never okay */
      return (FALSE);
    }
  
  /* Spell is learned */
  if ((spell < 32) ?
      (p_ptr->spell_learned1 & (1L << spell)) :
      (p_ptr->spell_learned2 & (1L << (spell - 32))))
    {
      /* Okay to cast, not to study */
      
      return (known);
    }
  
  /* Okay to study, not to cast */
  return (!known);
}



/**
 * Extra information on a spell		-DRS-
 *
 * We can use up to 20 characters of the buffer 'p'
 *
 * The strings in this function were extracted from the code in the
 * functions "do_cmd_cast()" and "do_cmd_pray()" and are up to date 
 * (as of 0.4.0). -LM-
 */
void spell_info(char *p, int spell_index)
{
  int plev = p_ptr->lev;
  
  int beam, beam_low;
  
  /* Specialty Ability */
  if (check_ability(SP_HEIGHTEN_MAGIC)) 
    plev += 1 + ((p_ptr->heighten_power + 5)/ 10);
  if (check_ability(SP_CHANNELING)) plev += get_channeling_boost();
  
  /* Beaming chance */
  beam = (((check_ability(SP_BEAM)))  ? plev : (plev / 2));
  beam_low = (beam - 10 > 0 ? beam - 10 : 0);
  
  /* Default */
  strcpy(p, "");
  
  /* Analyze the spell */
  switch (spell_index)
    {
      /* Sorcery */
      
    case 0: sprintf(p, " dam %dd3", 4 + plev / 10); break;
    case 2: strcpy(p, " range 10"); break;
    case 4: sprintf(p, " dam 2d%d, rad %d", 
		    1 + (plev / 5), ( plev / 10) + 1); break;
    case 5: sprintf(p, " dam %d, rad 2", 5 + (plev / 3)); break;
    case 7: sprintf(p, " dur 10+d5"); break; 
    case 9: sprintf(p, " dam %dd8, beam %d%%", (2+((plev-5)/5)), beam); 
      break;
    case 13: sprintf(p, " range %d", 50 + plev * 2); break;
    case 15: sprintf(p, " dam %d", 5 + plev); break;
    case 20: sprintf(p, " range %d", 5 + plev/10); break;
    case 26: sprintf(p, " dam %dd%d, length %d", 6 + (plev / 10), 8, 1 + (plev / 10)); 
      break;
    case 27: sprintf(p, " dam %d, rad 2", 55 + plev); break;
    case 30: sprintf(p, " dist %d", 45 + (plev/2)); break;
    case 34: sprintf(p, " dur %d+d%d", plev, plev); break;
    case 35: strcpy(p, " dur 30+d20"); break;
    case 36: strcpy(p, " dur 20+d20"); break;
    case 37: sprintf(p, " dur %d+d30", 10+plev); break;
    case 38: strcpy(p, " dur 40"); break;
    case 45: sprintf(p, " dam %dd8, beam %d%%", (3 + plev / 3), beam); break;
    case 49: sprintf(p, " dam %d, rad %d", 5 * plev / 2, plev / 12); break;
    case 51: sprintf(p, " dam %d,%d, rad 3,2", 10 + plev, 2 * plev); break;
    case 52: sprintf(p, " dam %d, rad 3", 3 * plev); break;
    case 53: sprintf(p, " dam %d, rad 1", 80 + plev * 2); break;
    case 54: sprintf(p, " dam %d+3d%d", plev, plev); break;
    case 55: sprintf(p, " dam %d, rad 1", 80 + plev * 2); break;
    case 56: sprintf(p, " dam %d, rad %d", 4 * plev, 3 + plev / 15); break;

      
      /* Piety */
    case 65: sprintf(p, " heal 2d%d", plev / 4 + 5); break;
    case 66: strcpy(p, " dur 12+d12"); break;
    case 68: sprintf(p, " dam 2d%d, rad %d", 1 + plev/3, plev/10+1); break;
    case 71: strcpy(p, " halve poison"); break;
    case 72: sprintf(p, " heal 4d%d", plev / 4 + 6); break;
    case 74: sprintf(p, " range %d", 2 * plev); break;
    case 75: strcpy(p, " dur 24+d24"); break;
    case 79: sprintf(p, " dur %d+d10", plev / 2); break;
    case 81: sprintf(p, " dam %d+3d6, rad %d", plev / 4 + 
		     (plev / ((check_ability(SP_STRONG_MAGIC)) ? 2 : 4)), 
		     (plev < 30) ? 1 : 2); break;
    case 82: sprintf(p, " dur %d+d24", plev); break;
    case 83: sprintf(p, " dur %d+d24", 3 * plev / 2); break;
    case 84: sprintf(p, " heal 9d%d, any cut", plev / 3 + 12); break;
    case 85: strcpy(p, " radius 10"); break;
    case 88: strcpy(p, " dur 48+d48"); break;
    case 89: sprintf(p, " dam d%d", 3 * plev); break;
    case 90: strcpy(p, " heal 300, any cut"); break;
    case 91: sprintf(p, " dam d%d", 3 * plev); break;
    case 92: sprintf(p, " dur %d+d20", plev / 2); break;
    case 94: sprintf(p, " dam d%d, heal 300", plev * 4); break;
    case 95: strcpy(p, " range 10"); break;
    case 96: sprintf(p, " range %d", 4 * plev); break;
    case 97: sprintf(p, " dist %d", 45 + (plev/3)); break;
    case 107: strcpy(p, " heal 700, any cut"); break;
    case 117: sprintf(p, " dam %d, rad 3", 2 * plev); break;
    case 118: sprintf(p, " dam %d", 3 * plev / 2); break;
    case 120: sprintf(p, " dam %d+d100", plev * 3); break;
    case 121: sprintf(p, " dam %d, heal 500", plev * 5); break;
      
      
      /* Nature Magics */
      
    case 129: sprintf(p, " dam 2d%d, rad %d", 1 + plev/4, plev/10+1); break;
    case 131: strcpy(p, " range 10"); break;
    case 132: strcpy(p, " halve poison"); break;
    case 133: sprintf(p, " dam %dd6, length %d", 2+plev/8, 1+plev/5); break;
    case 135: strcpy(p, " dam 20+d30"); break;
    case 136: strcpy(p, " dam 4d5"); break;
    case 138: sprintf(p, " dam %dd8, beam %d%%", 2+plev/5, beam_low); break;
    case 142: sprintf(p, " dam %d", 2 + plev / 5); break;
    case 143: sprintf(p, " dam %dd8, beam %d%%", 3 + plev / 5, beam_low); 
      break;
    case 144: strcpy(p, " dur 20+d20"); break;
    case 146: sprintf(p, " dam %dd8, beam %d%%", 5+plev/5, beam_low); break;
    case 147: sprintf(p, " dist %d", 45 + (plev/3)); break;
    case 148: sprintf(p, " dam %dd8, beam %d%%", 5+plev/4, beam_low); break;
    case 149: strcpy(p, " dur 20+d20"); break;
    case 150: strcpy(p, " radius 10"); break;
    case 151: strcpy(p, " dur 20+d20"); break;
    case 153: sprintf(p, " heal 2d%d, pois/cut", plev / 5); break;
    case 154: strcpy(p, " dur 20+d20"); break;
    case 155: sprintf(p, " dam %dd8, conf/slow", plev / 7); break;
    case 159: sprintf(p, " dam %d+d%d, rad %d", plev, 60 + plev * 2, 
		      1 + plev / 15); break;
    case 160: sprintf(p, " dam %d+d%d, rad %d", plev, 40 + plev * 2, 
		      plev / 8); break;
    case 169: strcpy(p, " dur 24+d24"); break;
    case 170: sprintf(p, " heal %dd12, any cut", 25 + plev / 2); break;
    case 171: sprintf(p, " dam %d+d%d, rad %d", plev, 50 + plev * 2, 
		      1 + plev / 12); break;
    case 172: sprintf(p, " dam %d+d%d, rad %d", 30 + ((4 * plev) / 5), 
		      30 + plev * 2, plev / 7); break;
    case 173: sprintf(p, " dam %d+d%d, rad %d", 3 * plev / 2, 50 + plev * 3, 
		      1 + plev / 15); break;
    case 174: sprintf(p, " dam %d+d%d, rad 1", 35+(2*plev), 90+plev*4); break;
    case 175: sprintf(p, " dam %d+d%d, rad %d", 40 + (3 * plev / 2), plev * 3, 
		      plev / 10); break;
    case 177: sprintf(p, " dur %d+d30", plev / 2); break;
    case 178: sprintf(p, " dam d%d", plev * 2); break;
    case 181: sprintf(p, " dam %dd8, beam %d%%", plev / 6, plev * 2); break;
    case 182: sprintf(p, " dur %d+d10", plev / 2); break;
    case 186: strcpy(p, " heal 500, dam 100"); break;
    case 188: sprintf(p, " dam 6d6+%d", plev/3); break;

      /* Necromantic Spells */
      
    case 192: sprintf(p, " dam 2d%d", 5 + plev / 7); break;
    case 194: strcpy(p, " dur 70+d70"); break;
    case 199: strcpy(p, " hurt 2d4"); break;
    case 201: sprintf(p, " dam %dd8, beam %d%%", 3+plev/7, beam_low); break;
    case 202: sprintf(p, " dam %d, poison", 10 + plev); break;
    case 206: sprintf(p, " dam %d+d%d", plev + 15, 3 * plev / 2); break;
    case 207: sprintf(p, " dam %d+d%d", plev, plev); break;
    case 208: sprintf(p, " dur 20+d%d", plev / 2); break;
    case 209: strcpy(p, " range 16, hurt 1d4"); break;
    case 215: sprintf(p, " dam %dd8, hurt 1d6", 2 + plev / 3); break;
    case 216: sprintf(p, " dur %d+d20", plev / 2); break;
    case 217: sprintf(p, " dam %d+d%d", 2 * plev, 2 * plev); break;
    case 218: sprintf(p, " dam %d", 15 + 7 * plev / 4); break;
    case 219: sprintf(p, " dam %dd8, beam %d%%", 1 + plev / 2, beam); break;
    case 221: sprintf(p, " dam %d, rad 2", 50 + plev * 2); break;
    case 222: sprintf(p, " dam %d+d%d, hit ~9", 50 + plev * 2, plev); break;
    case 226: sprintf(p, " range %d, hurt 2d6", plev * 3); break;
    case 228: strcpy(p, " dur 20+d20"); break;
    case 230: sprintf(p, " %d+d%d", plev / 2, plev); break;
    case 231: strcpy(p, " dur d66"); break;
    case 233: strcpy(p, " dur 10+d20"); break;
    case 235: sprintf(p, " dam %dd13", 3 * plev / 5); break;
    case 236: sprintf(p, " dam %d, hurt 2d8", 20 + (4 * plev)); break;
    case 237: sprintf(p, " dam %d+4d%d", 60, plev); break;
    case 238: sprintf(p, " dam %dd11, heal %d", plev / 3, 3 * plev); break;
    case 240: strcpy(p, " hurt 2d7"); break;
    case 242: strcpy(p, " hurt 3d6"); break;
    case 243: strcpy(p, " dur 10+d20"); break;
    case 245: strcpy(p, " radius 15"); break;
    case 246: sprintf(p, " dist %d", 45 + (plev/3)); break;
    case 247: sprintf(p, " dam %d+d50", plev * 3); break;
    case 249: sprintf(p, " dam %d, rad %d", 11 * plev / 2, plev/7); break;
    case 250: sprintf(p, " dur 30+d40"); break;
    case 251: sprintf(p, " dur 40"); break;
    case 252: sprintf(p, " dur 40"); break;
    case 254: sprintf(p, " mana %d", 3 * plev / 2); break;
    }
}


/**
 * Print out a list of available spells for any spellbook given.
 * Revised by -LM-
 *
 * Input y controls lines from top for list, and input x controls columns 
 * from left. 
 */
void print_spells(int tval, int sval, int y, int x)
{
  int i, left_justi;
  int j = 0;
  int first_spell, after_last_spell;
  
  magic_type *s_ptr;
  
  byte attr_book, attr_name, attr_extra;
  
  char *comment;
  char info[80];
  char out_val[160];
  
  object_kind *k_ptr = &k_info[lookup_kind(tval, sval)];
  char *basenm = (k_name + k_ptr->name);
  
  
  /* Currently must be a legal spellbook of the correct realm. */
  if ((tval != mp_ptr->spell_book) || (sval > SV_BOOK_MAX)) return;
  
  
  /* Choose appropriate spellbook color. */
  if (tval == TV_MAGIC_BOOK) 
    {
      if (sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_RED;
      else attr_book = TERM_RED;
    }
  else if (tval == TV_PRAYER_BOOK) 
    {
      if (sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_BLUE;
      else attr_book = TERM_BLUE;
    }
  else if (tval == TV_DRUID_BOOK) 
    {
      if (sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_GREEN;
      else attr_book = TERM_GREEN;
    }
  else if (tval == TV_NECRO_BOOK) 
    {
      if (sval < SV_BOOK_MIN_GOOD) attr_book = TERM_L_DARK;
      else attr_book = TERM_VIOLET;
    }
  else attr_book = TERM_WHITE;
  
  
  /* Find the array index of the spellbook's first spell. */
  first_spell = mp_ptr->book_start_index[sval];
  
  /* Find the first spell in the next book. */
  after_last_spell = mp_ptr->book_start_index[sval+1];
  
  /* Choose a left margin for the spellbook name. */
  left_justi = (small_screen ? 0 : ((80 - x) - strlen(basenm))) / 2;
  
  /* Center the spellbook name */
  prt("", y, x);
  c_put_str(attr_book, format("%s", basenm), y, x + left_justi);
  
  
  /* Title the list */
  prt("", y + 1, x);
  put_str("Name", y + 1, x + 5);
  put_str("Lv Mana Fail Info", y + 1, x + 35);
  
  /* Dump the spells in the book. */
  for (i = first_spell; i < after_last_spell; i++)
    {
      /* Access the spell */
      s_ptr = &mp_ptr->info[i];
      
      /* Increment the current line */
      j++;
      
      /* Skip illegible spells.  This should actually never appear. */
      if (s_ptr->slevel >= 99)
	{
	  sprintf(out_val, "  %c) %-30s", I2A(i - first_spell), 
		  "(illegible)");
	  c_prt(TERM_SLATE, out_val, y + j + 1, x);
	  continue;
	}
      
      /* Get extra info */
      spell_info(info, s_ptr->index);
      
      /* Use that info */
      comment = info;
      
      /* Analyze the spell */
      if ((i < 32) ?
	  ((p_ptr->spell_forgotten1 & (1L << i))) :
	  ((p_ptr->spell_forgotten2 & (1L << (i - 32)))))
	{
	  comment = " forgotten";
	  attr_name = TERM_L_WHITE;
	  attr_extra = TERM_L_WHITE;
	}
      else if (!((i < 32) ?
		 (p_ptr->spell_learned1 & (1L << i)) :
		 (p_ptr->spell_learned2 & (1L << (i - 32)))))
	{
	  comment = " unknown";
	  
	  /* Spells above the player's level are colored light gray. */
	  if (s_ptr->slevel > p_ptr->lev) attr_name = TERM_L_WHITE;
	  
	  /* If at or below the player's level, color in white. */
	  else attr_name = TERM_WHITE;
	  
	  attr_extra = TERM_L_WHITE;
	}
      else if (!((i < 32) ?
		 (p_ptr->spell_worked1 & (1L << i)) :
		 (p_ptr->spell_worked2 & (1L << (i - 32)))))
	{
	  comment = " untried";
	  attr_name = TERM_WHITE;
	  attr_extra = TERM_WHITE;
	}
      else 
	{
	  /* Vivid color for known, cast spells */
	  attr_name = attr_book;
	  attr_extra = TERM_L_BLUE;
	}
      
      /* Clear line */
      prt("", y + j + 1, x);
      
      /* Print out (colored) information about a single spell. */
      put_str(format("  %c) ", I2A(i - first_spell)), y + j + 1, x);
      c_put_str(attr_name, format("%-30s", spell_names[s_ptr->index]), 
		y + j + 1, x + 5);
      put_str(format("%2d %4d %3d%%", s_ptr->slevel, 
		     s_ptr->smana, spell_chance(i)), y + j + 1, x + 35);
      c_put_str(attr_extra, format("%s", comment), y + j + 1, x + 47);
    }
  }


/**
 * For a string describing the player's intrinsic racial flags.
 */
static char *view_abilities_aux(char *desc)
{
  u32b flags = rp_ptr->flags_obj;
  
  int attr_num, attr_listed, j;

  bool res = FALSE, vul = FALSE;
  
  char buf[10] = "";
  
  /* Sustain stats. */
  if ((flags & (OF_SUSTAIN_STR)) || (flags & (OF_SUSTAIN_INT)) || 
      (flags & (OF_SUSTAIN_WIS)) || (flags & (OF_SUSTAIN_DEX)) || 
      (flags & (OF_SUSTAIN_CON)) || (flags & (OF_SUSTAIN_CHR)))
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      /* How many attributes need to be listed? */
      if (flags & (OF_SUSTAIN_STR)) attr_num++;
      if (flags & (OF_SUSTAIN_INT)) attr_num++;
      if (flags & (OF_SUSTAIN_WIS)) attr_num++;
      if (flags & (OF_SUSTAIN_DEX)) attr_num++;
      if (flags & (OF_SUSTAIN_CON)) attr_num++;
      if (flags & (OF_SUSTAIN_CHR)) attr_num++;
      
      /* Special case:  sustain all stats */
      if (attr_num == 6)
	{
	  strcat(desc, "Your stats are all sustained.  ");
	}
      else
	{
	  strcat(desc, "Your");
	  
	  /* Loop for number of attributes in this group. */
	  for (j = 0; j < 6; j++)
	    {
	      bool list_ok = FALSE;
	      
	      if ((j == 0) && (flags & (OF_SUSTAIN_STR))) list_ok = TRUE;
	      if ((j == 1) && (flags & (OF_SUSTAIN_INT))) list_ok = TRUE;
	      if ((j == 2) && (flags & (OF_SUSTAIN_WIS))) list_ok = TRUE;
	      if ((j == 3) && (flags & (OF_SUSTAIN_DEX))) list_ok = TRUE;
	      if ((j == 4) && (flags & (OF_SUSTAIN_CON))) list_ok = TRUE;
	      if ((j == 5) && (flags & (OF_SUSTAIN_CHR))) list_ok = TRUE;
	      
	      if (!list_ok) continue;
	      
	      /* Listing another attribute. */
	      attr_listed++;
	      
	      /* Commas separate members of a list of more than two. */
	      if ((attr_num > 2) && (attr_listed > 1)) strcat(desc, ",");
	      
	      /* "and" before final member of a list of more than one. */
	      if ((attr_num > 1) && (j != 0))
		{
		  if (attr_num == attr_listed) strcat(desc, " and");
		}
	      
	      /* List the attribute description, in its proper place. */
	      if (j == 0) strcat(desc, " strength");
	      if (j == 1) strcat(desc, " intelligence");
	      if (j == 2) strcat(desc, " wisdom");
	      if (j == 3) strcat(desc, " dexterity");
	      if (j == 4) strcat(desc, " constitution");
	      if (j == 5) strcat(desc, " charisma");
	    }
	}
      
      /* Pluralize the verb. */
      if (attr_num > 1) strcat(desc, " are");
      else strcat(desc, " is");
      
      /* End sentence.  Go to next line. */
      strcat(desc, " sustained. ");
    }
  
  /* Clear number of items to list, and items listed. */
  attr_num = 0;
  attr_listed = 0;
  
  /* Elemental immunities. */
  for (j = 0; j < 4; j++)
    if (rp_ptr->percent_res[j] == RES_BOOST_IMMUNE) attr_num++;
  
  if (attr_num > 0)
    {
      strcat(desc, "You are immune to ");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < 4; j++)
	{
	  if (rp_ptr->percent_res[j] > RES_BOOST_IMMUNE) continue;
	  
	  /* List the attribute description, in its proper place. */
	  if (j == 0) strcat(desc, "acid");
	  if (j == 1) strcat(desc, "electricity");
	  if (j == 2) strcat(desc, "fire");
	  if (j == 3) strcat(desc, "frost");
	  if (attr_num >= 3) strcat(desc, ", ");
	  if (attr_num == 2) strcat(desc, " and ");
	  if (attr_num == 1) strcat(desc, ". ");
	  attr_num--;
	}
      
      /* End sentence.  Go to next line. */
      strcat(desc, ". ");
    }
  
  
  /* Check for resists and vulnerabilities */
  for (j = 0; j < MAX_P_RES; j++)
    {
      if ((rp_ptr->percent_res[j] < 100) && (rp_ptr->percent_res[j] > 0))
	res = TRUE;
      else if (rp_ptr->percent_res[j] > 100)
	vul = TRUE;
    }
  
  
  /* Resistances. */
  if (res)
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      for (j = 0; j < MAX_P_RES; j++)
	{
	  if ((rp_ptr->percent_res[j] < 100) && (rp_ptr->percent_res[j] > 0))
	    attr_num++;
	}
      
      /* How many attributes need to be listed? */
      strcat(desc, "You resist");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < MAX_P_RES; j++)
	{
	  bool list_ok = FALSE;
	  
	  if ((rp_ptr->percent_res[j] < 100) && 
	      (rp_ptr->percent_res[j] > 0))
	    list_ok = TRUE;
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    strcat(desc, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		strcat(desc, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == P_RES_ACID) strcat(desc, " acid");
	  if (j == P_RES_ELEC) strcat(desc, " electricity");
	  if (j == P_RES_FIRE) strcat(desc, " fire");
	  if (j == P_RES_COLD) strcat(desc, " frost");
	  if (j == P_RES_POIS) strcat(desc, " poison");
	  if (j == P_RES_LITE) strcat(desc, " light");
	  if (j == P_RES_DARK) strcat(desc, " darkness");
	  if (j == P_RES_CONFU) strcat(desc, " confusion");
	  if (j == P_RES_SOUND) strcat(desc, " sound");
	  if (j == P_RES_SHARD) strcat(desc, " shards");
	  if (j == P_RES_NEXUS) strcat(desc, " nexus");
	  if (j == P_RES_NETHR) strcat(desc, " nether");
	  if (j == P_RES_CHAOS) strcat(desc, " chaos");
	  if (j == P_RES_DISEN) strcat(desc, " disenchantment");
		
	  sprintf(buf, "(%d%%)", 100 - rp_ptr->percent_res[j]);
	  strcat(desc, buf);
	}
      
      /* End sentence.  Go to next line. */
      strcat(desc, ". ");
    }
  
  
  /* Vulnerabilities. */
  if (vul)
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      for (j = 0; j < MAX_P_RES; j++)
	{
	  if (rp_ptr->percent_res[j] > 100)
	    attr_num++;
	}
      
      strcat(desc, "You are vulnerable to");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < MAX_P_RES; j++)
	{
	  bool list_ok = FALSE;
	  
	  if (rp_ptr->percent_res[j] > 100)
	    list_ok = TRUE;
	  
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) 
	    strcat(desc, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) 
		strcat(desc, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == P_RES_ACID) strcat(desc, " acid");
	  if (j == P_RES_ELEC) strcat(desc, " electricity");
	  if (j == P_RES_FIRE) strcat(desc, " fire");
	  if (j == P_RES_COLD) strcat(desc, " frost");
	  if (j == P_RES_POIS) strcat(desc, " poison");
	  if (j == P_RES_LITE) strcat(desc, " light");
	  if (j == P_RES_DARK) strcat(desc, " darkness");
	  if (j == P_RES_SOUND) strcat(desc, " sound");
	  if (j == P_RES_SHARD) strcat(desc, " shards");
	  if (j == P_RES_NEXUS) strcat(desc, " nexus");
	  if (j == P_RES_NETHR) strcat(desc, " nether");
	  if (j == P_RES_CHAOS) strcat(desc, " chaos");
	  if (j == P_RES_DISEN) strcat(desc, " disenchantment");
	  
	  sprintf(buf, "(%d%%)", rp_ptr->percent_res[j] - 100);
	  strcat(desc, buf);
	}
      
      /* End sentence.  Go to next line. */
      strcat(desc, ". ");
    }
  
  
  
  
  /* Clear a listing variable. */
  attr_num = 0;
  
  /* Special processing for two "survival resists" */
  if (flags & (OF_FEARLESS)) attr_num++;
  if (flags & (OF_SEEING)) attr_num++;
  
  if (flags & (OF_FEARLESS))
    {
      strcat(desc, "You are fearless");
      if (attr_num == 1) strcat(desc, ".  ");
      else strcat(desc, ", and");
    }
  
  if (flags & (OF_SEEING))
    {
      if ((attr_num > 1) && (flags & (OF_FEARLESS))) 
	      strcat(desc, " can not be blinded.  ");
      else strcat(desc, "You can not be blinded.  ");
    }
  
  /* Miscellaneous abilities. */
  if ((flags & (OF_SLOW_DIGEST)) || (flags & (OF_FEATHER)) || 
      (flags & (OF_LITE)) || (flags & (OF_REGEN)) || 
      (flags & (OF_TELEPATHY)) || (flags & (OF_SEE_INVIS)) || 
      (flags & (OF_FREE_ACT)) || (flags & (OF_HOLD_LIFE)))
    {
      /* Clear number of items to list, and items listed. */
      attr_num = 0;
      attr_listed = 0;
      
      /* How many attributes need to be listed? */
      if (flags & (OF_SLOW_DIGEST)) attr_num++;
      if (flags & (OF_FEATHER)) attr_num++;
      if (flags & (OF_LITE)) attr_num++;
      if (flags & (OF_REGEN)) attr_num++;
      if (flags & (OF_TELEPATHY)) attr_num++;
      if (flags & (OF_SEE_INVIS)) attr_num++;
      if (flags & (OF_FREE_ACT)) attr_num++;
      if (flags & (OF_HOLD_LIFE)) attr_num++;
      
      strcat(desc, "You");
      
      /* Loop for number of attributes in this group. */
      for (j = 0; j < 8; j++)
	{
	  bool list_ok = FALSE;
	  
	  if ((j == 0) && (flags & (OF_SLOW_DIGEST))) list_ok = TRUE;
	  if ((j == 1) && (flags & (OF_FEATHER))) list_ok = TRUE;
	  if ((j == 2) && (flags & (OF_LITE))) list_ok = TRUE;
	  if ((j == 3) && (flags & (OF_REGEN))) list_ok = TRUE;
	  if ((j == 4) && (flags & (OF_TELEPATHY))) list_ok = TRUE;
	  if ((j == 5) && (flags & (OF_SEE_INVIS))) list_ok = TRUE;
	  if ((j == 6) && (flags & (OF_FREE_ACT))) list_ok = TRUE;
	  if ((j == 7) && (flags & (OF_HOLD_LIFE)))list_ok = TRUE;
	  
	  if (!list_ok) continue;
	  
	  /* Listing another attribute. */
	  attr_listed++;
	  
	  /* Commas separate members of a list of more than two. */
	  if ((attr_num > 2) && (attr_listed > 1)) strcat(desc, ",");
	  
	  /* "and" before final member of a list of more than one. */
	  if ((attr_num > 1) && (j != 0))
	    {
	      if (attr_num == attr_listed) strcat(desc, " and");
	    }
	  
	  /* List the attribute description, in its proper place. */
	  if (j == 0) strcat(desc, " digest slowly");
	  if (j == 1) strcat(desc, " falling gently");
	  if (j == 2) strcat(desc, " glow with permanent light");
	  if (j == 3) strcat(desc, " regenerative quickly");
	  if (j == 4) strcat(desc, " have telepathic powers");
	  if (j == 5) strcat(desc, " can see invisible monsters");
	  if (j == 6) strcat(desc, " are immune to paralysis");
	  if (j == 7) strcat(desc, " are resistant to life draining");
	}
      
      /* End sentence.  Go to next line. */
      strcat(desc, ".");
    }
  
  return(desc);
  
}



/*
 * View specialty code 
 */
int race_start, class_start, race_other_start;
int total_known, spec_known, racial_known, class_known; 
byte racial_list[32];
byte class_list[32];
char race_other_desc[1000] ="";

static char view_spec_tag(menu_type *menu, int oid)
{
  return I2A(oid);
}

/**
 * Display an entry on the gain specialty menu
 */
void view_spec_display(menu_type *menu, int oid, bool cursor, int row, 
			 int col, int width)
{
  int x, y;
  char buf[80];
  byte color;

  if (oid < class_start)
    {
      sprintf(buf, "Specialty Ability: %s", 
	      specialty_names[p_ptr->specialty_order[oid]]);
      color = cursor ? TERM_L_GREEN : TERM_GREEN;
    }
  else if (oid < race_start)
    {
      sprintf(buf, "Class: %s", 
	      specialty_names[SP_CLASS_START + class_list[oid - class_start]]);
      color = cursor ? TERM_L_UMBER : TERM_UMBER;
    }
  else if (oid < race_other_start)
    {
      sprintf(buf, "Racial: %s", 
	      specialty_names[SP_RACIAL_START + racial_list[oid - race_start]]);
      color = cursor ? TERM_YELLOW : TERM_ORANGE;
    }
  else
    {
      sprintf(buf, "Racial: Other");
      color = cursor ? TERM_YELLOW : TERM_ORANGE;
    }
  /* Print it */
  c_put_str(color, buf, row, col);

  /* Help text */
  if (cursor)
    {
      /* Locate the cursor */
      Term_locate(&x, &y);
  
      /* Move the cursor */
      Term_gotoxy(3, total_known + 2);
      text_out_indent = 3; 
      if (oid < class_start) 
	text_out_to_screen(TERM_L_BLUE, 
			   specialty_tips[p_ptr->specialty_order[oid]]);
      else if (oid < race_start) 
	text_out_to_screen(TERM_L_BLUE, 
			   specialty_tips[SP_CLASS_START + 
					  class_list[oid - class_start]]);
      else if (oid < race_other_start) 
	text_out_to_screen(TERM_L_BLUE, 
			   specialty_tips[SP_RACIAL_START + 
					  racial_list[oid - race_start]]);
      else text_out_to_screen(TERM_L_BLUE, race_other_desc);

      /* Restore */
      Term_gotoxy(x, y);
    }
}


/**
 * Display list available specialties.
 */
void view_spec_menu(void)
{
  menu_type menu;
  menu_iter menu_f = { 0, view_spec_tag, 0, view_spec_display, 0 };
  event_type evt = { EVT_NONE, 0, 0, 0, 0 };
  int cursor = 0;

  bool done = FALSE;
  
  char buf[80];

  /* Save the screen and clear it */
  screen_save();
  
  /* Prompt choices */
  sprintf(buf, "Race, class, and specialties abilities (%c-%c, ESC=exit): ",
	  I2A(0), I2A(total_known - 1));
  
  /* Set up the menu */
  WIPE(&menu, menu);
  menu.title = buf;
  menu.cmd_keys = " \n\r";
  menu.count = total_known;
  menu_init2(&menu, find_menu_skin(MN_SCROLL), &menu_f, &SCREEN_REGION);
  
  while (!done)
    {
      evt = menu_select(&menu, &cursor, 0);
      done = ((evt.type == EVT_ESCAPE) || (evt.type == EVT_BACK));
    }

  /* Load screen */
  screen_load();

  return;
}

/**
 * Browse known specialty abilities -BR-
 * Adapted from birth.c get_player_choice
 */
void do_cmd_view_abilities(void)
{
  int i;

  /* Count the number of specialties we know */
  for (i = 0, spec_known = 0; i < MAX_SPECIALTIES; i++)
    {
      if (p_ptr->specialty_order[i] != SP_NO_SPECIALTY) spec_known++;
    }
  
  total_known = spec_known;
  
  /* Count the number of class powers we have */
  for (i = 0, class_known = 0; i < 32; i++)
    {
      if (cp_ptr->flags_special & (0x00000001L << i))
	{
	  class_list[class_known++] = i;
	}
    }
  
  total_known += class_known;
  
  /* Count the number of racial powers we have */
  for (i = 0, racial_known = 0; i < 32; i++)
    {
      if (rp_ptr->flags_special & (0x00000001L << i))
	{
	  racial_list[racial_known++] = i;
	}
    }
  
  total_known += racial_known;
  
  /* Standard racial flags */
  if (rp_ptr->flags_obj | rp_ptr->flags_curse)
    {
      total_known++;
      view_abilities_aux(race_other_desc);
    }
  
  class_start = spec_known;
  race_start = spec_known + class_known;
  race_other_start = spec_known + class_known + racial_known;
  
  /* Buttons */
  normal_screen = FALSE;
  update_statusline();
  
  /* View choices until user exits */
  view_spec_menu();
  
  /* Buttons */
  normal_screen = TRUE;
  update_statusline();
  
  /* exit */
  return;
}


