/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *			     Mud constants module			    *
 ****************************************************************************/

#include <stdio.h>
#include "mud.h"

const char *const npc_race[MAX_NPC_RACE] = {
   "human", "dwarf", "elf", "half-elf", "gnome", "catfolk", "wolffolk",
   "cyclops", "firebird", "benn", "merfolk", "naga", "dryad",
   "naiad", "lizardfolk", "dollfolk", "fairy", "satyr", "minotaur", "centaur",
   "sphinx", "salamander", "gryphon", "unicorn", "slime", 		/* Line added by me -Danny */

   "ant", "ape", "baboon", "bat", "bear", "bee",
   "beetle", "boar", "bugbear", "construct", "dog", "dragon", "ferret", "fly",
   "gargoyle", "gelatin", "ghoul", "gnoll", "gnome", "goblin", "golem",
   "gorgon", "harpy", "hobgoblin", "kobold", "lizardman", "locust",
   "lycanthrope", "minotaur", "mold", "mule", "neanderthal", "ooze", "orc",
   "rat", "rustmonster", "shadow", "shapeshifter", "shrew", "shrieker",
   "skeleton", "slime", "snake", "spider", "stirge", "thoul", "troglodyte",
   "undead", "wight", "wolf", "worm", "zombie", "bovine", "canine", "feline",
   "porcine", "mammal", "rodent", "avis", "reptile", "amphibian", "fish",
   "crustacean", "insect", "spirit", "magical", "horse", "animal", "humanoid",
   "monster", "god"
};


const char *const npc_class[MAX_NPC_CLASS] = {
   "bard", "chevalier", "cleric", "druid", "elementalist", "enchanter", "illusionist",
   "lancer", "monk", "paladin", "ranger", "rogue", "scholar", "warrior",
   "pc14", "pc15", "pc16", "pc17", "pc18", "pc19",
   "baker", "butcher", "blacksmith", "mayor", "king", "queen"
};

const char *const body_shape[15] = {
   "regular", "lithe", "thin", "rotund", "curvaceous",
   "voluptuous", "pear-shaped", "fat", "corpulent", "starved",
   "lanky", "muscular", "stocky", "toned", "wiry"
};


/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
   {-5, -4, 0, 0},   /* 0  */
   {-5, -4, 3, 1},   /* 1  */
   {-3, -2, 3, 2},
   {-3, -1, 10, 3},  /* 3  */
   {-2, -1, 25, 4},
   {-2, -1, 55, 5},  /* 5  */
   {-1, 0, 80, 6},
   {-1, 0, 90, 7},
   {0, 0, 100, 8},
   {0, 0, 100, 9},
   {0, 0, 115, 10},  /* 10  */
   {0, 0, 115, 11},
   {0, 0, 140, 12},
   {0, 0, 140, 13},  /* 13  */
   {0, 1, 170, 14},
   {1, 1, 170, 15},  /* 15  */
   {1, 2, 195, 16},
   {2, 3, 220, 22},
   {2, 4, 250, 25},  /* 18  */
   {3, 5, 400, 30},
   {3, 6, 500, 35},  /* 20  */
   {4, 7, 600, 40},
   {5, 7, 700, 45},
   {6, 8, 800, 50},
   {8, 10, 900, 55},
   {10, 12, 999, 60} /* 25   */
};



const struct int_app_type int_app[26] = {
   {3},  /*  0 */
   {5},  /*  1 */
   {7},
   {8},  /*  3 */
   {9},
   {10}, /*  5 */
   {11},
   {12},
   {13},
   {15},
   {17}, /* 10 */
   {19},
   {22},
   {25},
   {28},
   {31}, /* 15 */
   {34},
   {37},
   {40}, /* 18 */
   {44},
   {49}, /* 20 */
   {55},
   {60},
   {70},
   {85},
   {99}  /* 25 */
};



const struct wis_app_type wis_app[26] = {
   {-5},  /*  0 */
   {-5},  /*  1 */
   {-4},
   {-4},  /*  3 */
   {-3},
   {-3},  /*  5 */
   {-2},
   {-2},
   {-1},
   {-1},
   {0},  /* 10 */
   {0},
   {1},
   {1},
   {2},
   {2},  /* 15 */
   {3},
   {3},
   {4},  /* 18 */
   {4},
   {5},  /* 20 */
   {5},
   {6},
   {6},
   {7},
   {7}   /* 25 */
};



const struct dex_app_type dex_app[26] = {
   {60}, /* 0 */
   {50}, /* 1 */
   {50},
   {40},
   {30},
   {20}, /* 5 */
   {10},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {-10},   /* 15 */
   {-15},
   {-20},
   {-30},
   {-40},
   {-50},   /* 20 */
   {-60},
   {-75},
   {-90},
   {-105},
   {-120}   /* 25 */
};



const struct con_app_type con_app[26] = {
   {-6, 20},   /*  0 */
   {-6, 25},   /*  1 */
   {-6, 30},
   {-6, 35},   /*  3 */
   {-6, 40},
   {-5, 45},   /*  5 */
   {-4, 50},
   {-3, 55},
   {-2, 60},
   {-1, 65},
   {0, 70}, /* 10 */
   {1, 75},
   {2, 80},
   {3, 85},
   {4, 88},
   {5, 90}, /* 15 */
   {6, 95},
   {7, 97},
   {8, 99}, /* 18 */
   {9, 99},
   {10, 99}, /* 20 */
   {11, 99},
   {12, 99},
   {13, 99},
   {14, 99},
   {15, 99}  /* 25 */
};


const struct cha_app_type cha_app[26] = {
   {-60},   /* 0 */
   {-50},   /* 1 */
   {-50},
   {-40},
   {-30},
   {-20},   /* 5 */
   {-10},
   {-5},
   {-1},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {1},
   {5},  /* 15 */
   {10},
   {20},
   {30},
   {40},
   {50}, /* 20 */
   {60},
   {70},
   {80},
   {90},
   {99}  /* 25 */
};

/* Have to fix this up - not exactly sure how it works (Scryn) */
const struct lck_app_type lck_app[26] = {
   {60}, /* 0 */
   {50}, /* 1 */
   {50},
   {40},
   {30},
   {20}, /* 5 */
   {10},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {-10},   /* 15 */
   {-15},
   {-20},
   {-30},
   {-40},
   {-50},   /* 20 */
   {-60},
   {-75},
   {-90},
   {-105},
   {-120}   /* 25 */
};


/*
 * Liquid properties.
 * Used in #OBJECT section of area file.
 */
const struct liq_type liq_table[LIQ_MAX] = {
   {"water", "clear", {0, 1, 10}},  /*  0 */
   {"beer", "amber", {3, 2, 5}},
   {"wine", "rose", {5, 2, 5}},
   {"ale", "brown", {2, 2, 5}},
   {"dark ale", "dark", {1, 2, 5}},

   {"whisky", "golden", {6, 1, 4}}, /*  5 */
   {"lemonade", "pink", {0, 1, 8}},
   {"firebreather", "boiling", {10, 0, 0}},
   {"local specialty", "everclear", {3, 3, 3}},
   {"slime mold juice", "green", {0, 4, -8}},

   {"milk", "white", {0, 3, 6}}, /* 10 */
   {"tea", "tan", {0, 1, 6}},
   {"coffee", "black", {0, 1, 6}},
   {"blood", "red", {0, 2, -1}},
   {"salt water", "clear", {0, 1, -2}},

   {"cola", "cherry", {0, 1, 5}},   /* 15 */
   {"mead", "honey color", {4, 2, 5}}, /* 16 */
   {"grog", "thick brown", {3, 2, 5}}  /* 17 */
};

/* removed "pea" and added chop, spear, smash - Grimm */
/* Removed duplication in damage types - Samson 1-9-00 */
const char *attack_table[DAM_MAX_TYPE] =
{
   "hit", "slash", "stab", "hack", "crush", "lash", "pierce", "thrust"
};

const char *attack_table_plural[DAM_MAX_TYPE] =
{
   "hits", "slashes", "stabs",  "hacks", "crushes", "lashes", "pierces", "thrusts"
};

const char *weapon_skills[WEP_MAX] =
{
  "Barehand", "Sword", "Dagger", "Whip", "Talon",
  "Mace", "Archery", "Blowgun", "Sling", "Axe", "Spear", "Staff"
};

const char *projectiles[PROJ_MAX] =
{
   "Bolt", "Arrow", "Dart", "Stone"
};


const char *s_blade_messages[24] = {
   "miss", "barely scratch", "scratch", "nick", "cut", "hit", "tear",
   "rip", "gash", "lacerate", "hack", "maul", "rend", "decimate",
   "_mangle_", "_devastate_", "_cleave_", "_butcher_", "DISEMBOWEL",
   "DISFIGURE", "GUT", "EVISCERATE", "* SLAUGHTER *", "*** ANNIHILATE ***"
};

const char *p_blade_messages[24] = {
   "misses", "barely scratches", "scratches", "nicks", "cuts", "hits",
   "tears", "rips", "gashes", "lacerates", "hacks", "mauls", "rends",
   "decimates", "_mangles_", "_devastates_", "_cleaves_", "_butchers_",
   "DISEMBOWELS", "DISFIGURES", "GUTS", "EVISCERATES", "* SLAUGHTERS *",
   "*** ANNIHILATES ***"
};

const char *s_blunt_messages[24] = {
   "miss", "barely scuff", "scuff", "pelt", "bruise", "strike", "thrash",
   "batter", "flog", "pummel", "smash", "maul", "bludgeon", "decimate",
   "_shatter_", "_devastate_", "_maim_", "_cripple_", "MUTILATE", "DISFIGURE",
   "MASSACRE", "PULVERIZE", "* OBLITERATE *", "*** ANNIHILATE ***"
};

const char *p_blunt_messages[24] = {
   "misses", "barely scuffs", "scuffs", "pelts", "bruises", "strikes",
   "thrashes", "batters", "flogs", "pummels", "smashes", "mauls",
   "bludgeons", "decimates", "_shatters_", "_devastates_", "_maims_",
   "_cripples_", "MUTILATES", "DISFIGURES", "MASSACRES", "PULVERIZES",
   "* OBLITERATES *", "*** ANNIHILATES ***"
};

const char *s_generic_messages[24] = {
   "miss", "brush", "scratch", "graze", "nick", "jolt", "wound",
   "injure", "hit", "jar", "thrash", "maul", "decimate", "_traumatize_",
   "_devastate_", "_maim_", "_demolish_", "MUTILATE", "MASSACRE",
   "PULVERIZE", "DESTROY", "* OBLITERATE *", "*** ANNIHILATE ***",
   "**** SMITE ****"
};

const char *p_generic_messages[24] = {
   "misses", "brushes", "scratches", "grazes", "nicks", "jolts", "wounds",
   "injures", "hits", "jars", "thrashes", "mauls", "decimates", "_traumatizes_",
   "_devastates_", "_maims_", "_demolishes_", "MUTILATES", "MASSACRES",
   "PULVERIZES", "DESTROYS", "* OBLITERATES *", "*** ANNIHILATES ***",
   "**** SMITES ****"
};

const char **const s_message_table[DAM_MAX_TYPE] =
{
   s_generic_messages,     /* hit */
   s_blade_messages,       /* slash */
   s_blade_messages,       /* stab */
   s_blade_messages,       /* hack */
   s_blunt_messages,       /* crush */
   s_blunt_messages,       /* lash */
   s_blade_messages,       /* pierce */
   s_blade_messages,       /* thrust */
};

const char **const p_message_table[DAM_MAX_TYPE] =
{
   p_generic_messages,     /* hit */
   p_blade_messages,       /* slash */
   p_blade_messages,       /* stab */
   p_blade_messages,       /* hack */
   p_blunt_messages,       /* crush */
   p_blunt_messages,       /* lash */
   p_blade_messages,       /* pierce */
   p_blade_messages,       /* thrust */
};

/* Weather constants - FB */
const char *const temp_settings[MAX_CLIMATE] = {
   "cold",
   "cool",
   "normal",
   "warm",
   "hot",
};

const char *const precip_settings[MAX_CLIMATE] = {
   "arid",
   "dry",
   "normal",
   "damp",
   "wet",
};

const char *const wind_settings[MAX_CLIMATE] = {
   "still",
   "calm",
   "normal",
   "breezy",
   "windy",
};

const char *const preciptemp_msg[6][6] = {
   /*
    * precip = 0
    */
   {
    "Frigid temperatures settle over the land",
    "It is bitterly cold",
    "The weather is crisp and dry",
    "A comfortable warmth sets in",
    "A dry heat warms the land",
    "Seething heat bakes the land"},
   /*
    * precip = 1
    */
   {
    "A few flurries drift from the high clouds",
    "Frozen drops of rain fall from the sky",
    "An occasional raindrop falls to the ground",
    "Mild drops of rain seep from the clouds",
    "It is very warm, and the sky is overcast",
    "High humidity intensifies the seering heat"},
   /*
    * precip = 2
    */
   {
    "A brief snow squall dusts the earth",
    "A light flurry dusts the ground",
    "Light snow drifts down from the heavens",
    "A light drizzle mars an otherwise perfect day",
    "A few drops of rain fall to the warm ground",
    "A light rain falls through the sweltering sky"},
   /*
    * precip = 3
    */
   {
    "Snowfall covers the frigid earth",
    "Light snow falls to the ground",
    "A brief shower moistens the crisp air",
    "A pleasant rain falls from the heavens",
    "The warm air is heavy with rain",
    "A refreshing shower eases the oppresive heat"},
   /*
    * precip = 4
    */
   {
    "Sleet falls in sheets through the frosty air",
    "Snow falls quickly, piling upon the cold earth",
    "Rain pelts the ground on this crisp day",
    "Rain drums the ground rythmically",
    "A warm rain drums the ground loudly",
    "Tropical rain showers pelt the seering ground"},
   /*
    * precip = 5
    */
   {
    "A downpour of frozen rain covers the land in ice",
    "A blizzard blankets everything in pristine white",
    "Torrents of rain fall from a cool sky",
    "A drenching downpour obscures the temperate day",
    "Warm rain pours from the sky",
    "A torrent of rain soaks the heated earth"}
};

const char *const windtemp_msg[6][6] = {
   /*
    * wind = 0
    */
   {
    "The frigid air is completely still",
    "A cold temperature hangs over the area",
    "The crisp air is eerily calm",
    "The warm air is still",
    "No wind makes the day uncomfortably warm",
    "The stagnant heat is sweltering"},
   /*
    * wind = 1
    */
   {
    "A light breeze makes the frigid air seem colder",
    "A stirring of the air intensifies the cold",
    "A touch of wind makes the day cool",
    "It is a temperate day, with a slight breeze",
    "It is very warm, the air stirs slightly",
    "A faint breeze stirs the feverish air"},
   /*
    * wind = 2
    */
   {
    "A breeze gives the frigid air bite",
    "A breeze swirls the cold air",
    "A lively breeze cools the area",
    "It is a temperate day, with a pleasant breeze",
    "Very warm breezes buffet the area",
    "A breeze ciculates the sweltering air"},
   /*
    * wind = 3
    */
   {
    "Stiff gusts add cold to the frigid air",
    "The cold air is agitated by gusts of wind",
    "Wind blows in from the north, cooling the area",
    "Gusty winds mix the temperate air",
    "Brief gusts of wind punctuate the warm day",
    "Wind attempts to cut the sweltering heat"},
   /*
    * wind = 4
    */
   {
    "The frigid air whirls in gusts of wind",
    "A strong, cold wind blows in from the north",
    "Strong wind makes the cool air nip",
    "It is a pleasant day, with gusty winds",
    "Warm, gusty winds move through the area",
    "Blustering winds punctuate the seering heat"},
   /*
    * wind = 5
    */
   {
    "A frigid gale sets bones shivering",
    "Howling gusts of wind cut the cold air",
    "An angry wind whips the air into a frenzy",
    "Fierce winds tear through the tepid air",
    "Gale-like winds whip up the warm air",
    "Monsoon winds tear the feverish air"}
};

const char *const precip_msg[3] = {
   "there is not a cloud in the sky",
   "pristine white clouds are in the sky",
   "thick, grey clouds mask the sun"
};

const char *const wind_msg[6] = {
   "there is not a breath of wind in the air",
   "a slight breeze stirs the air",
   "a breeze wafts through the area",
   "brief gusts of wind punctuate the air",
   "angry gusts of wind blow",
   "howling winds whip the air into a frenzy"
};
