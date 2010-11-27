/** \file z-form.c
    \brief Low-level text formatting (snprintf() replacement)
 
 * Copyright (c) 1997-2009 Ben Harrison, Jeff Greene
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
 *
 *
 * Here is some information about the routines in this file.
 *
 * In general, the following routines take a "buffer", a "max length",
 * a "format string", and some "arguments", and use the format string
 * and the arguments to create a (terminated) string in the buffer
 * (using only the first "max length" bytes), and return the "length"
 * of the resulting string, not including the (mandatory) terminator.
 *
 * The format strings allow the basic "sprintf()" format sequences, though
 * some of them are processed slightly more carefully or portably, as well
 * as a few "special" sequences, including the "capilitization" sequences of
 * "%C" and "%S".
 *
 * Note that some "limitations" are enforced by the current implementation,
 * for example, no "format sequence" can exceed 100 characters, including any
 * "length" restrictions, and the result of combining and "format sequence"
 * with the relevent "arguments" must not exceed 1000 characters.
 *
 * These limitations could be fixed by stealing some of the code from,
 * say, "vsprintf()" and placing it into my "vstrnfmt()" function.
 *
 * Note that a "^" inside a "format sequence" causes the first non-space
 * character in the string resulting from the combination of the format
 * sequence and the argument(s) to be "capitalized" if possible.  Note
 * that the "^" character is removed before the "standard" formatting
 * routines are called.  Likewise, a "*" inside a "format sequence" is
 * removed from the "format sequence", and replaced by the textual form
 * of the next argument in the argument list.  See examples below.
 *
 * Legal format characters: %,b,n,p,c,s,d,i,o,u,X,x,E,e,F,f,G,g,r,v.
 *
 * Format("%%")
 *   Append the literal "%".
 *   No legal modifiers.
 *
 * Format("%n", size_t *np)
 *   Save the current length into (*np).
 *   No legal modifiers.
 *
 * Format("%p", void *v)
 *   Append the pointer "v" (implementation varies).
 *   No legal modifiers.
 *
 * format("%b", int b)
 *   Append the integer formatted as binary.
 *   If a modifier of 1, 2, 3 or 4 is provided, then only append 2**n bits, not
 *   all 32.
 *
 * Format("%E", double r)
 * Format("%F", double r)
 * Format("%G", double r)
 * Format("%e", double r)
 * Format("%f", double r)
 * Format("%g", double r)
 *   Append the double "r", in various formats.
 *
 * Format("%ld", long int i)
 *   Append the long integer "i".
 *
 * Format("%d", int i)
 *   Append the integer "i".
 *
 * Format("%lu", unsigned long int i)
 *   Append the unsigned long integer "i".
 *
 * Format("%u", unsigned int i)
 *   Append the unsigned integer "i".
 *
 * Format("%lo", unsigned long int i)
 *   Append the unsigned long integer "i", in octal.
 *
 * Format("%o", unsigned int i)
 *   Append the unsigned integer "i", in octal.
 *
 * Format("%lX", unsigned long int i)
 *   Note -- use all capital letters
 * Format("%lx", unsigned long int i)
 *   Append the unsigned long integer "i", in hexidecimal.
 *
 * Format("%X", unsigned int i)
 *   Note -- use all capital letters
 * Format("%x", unsigned int i)
 *   Append the unsigned integer "i", in hexidecimal.
 *
 * Format("%c", char c)
 *   Append the character "c".
 *   Do not use the "+" or "0" flags.
 *
 * Format("%s", cptr s)
 *   Append the string "s".
 *   Do not use the "+" or "0" flags.
 *   Note that a "NULL" value of "s" is converted to the empty string.
 *
 * Format("%y", type_union *y). Use any of the above patterns; 
 * z is interpreted as one of c, d, f, or s in the patterns above,
 * as appropriate for the type of the corresponding argument.
 * (There is currently no way to render a typeunion in octal or hex.)
 * 
 * For examples below, assume "int n = 0; int m = 100; char buf[100];",
 * plus "char *s = NULL;", and unknown values "char *txt; int i;".
 *
 * For example: "n = strnfmt(buf, -1, "(Max %d)", i);" will have a
 * similar effect as "sprintf(buf, "(Max %d)", i); n = strlen(buf);".
 *
 * For example: "(void)strnfmt(buf, 16, "%s", txt);" will have a similar
 * effect as "strncpy(buf, txt, 16); buf[15] = '\0';".
 *
 * For example: "if (strnfmt(buf, 16, "%s", txt) < 16) ..." will have
 * a similar effect as "strcpy(buf, txt)" but with bounds checking.
 *
 * For example: "s = buf; s += vstrnfmt(s, -1, ...); ..." will allow
 * multiple "appends" to "buf" (at the cost of losing the max-length info).
 *
 * For example: "s = buf; n = vstrnfmt(s+n, 100-n, ...); ..." will allow
 * multiple bounded "appends" to "buf", with constant access to "strlen(buf)".
 *
 * For example: "format("%^-.*s", i, txt)" will produce a string containing
 * the first "i" characters of "txt", left justified, with the first non-space
 * character capitilized, if reasonable.
 */


#include "z-form.h"
#include "z-type.h"

#include "z-util.h"
#include "z-virt.h"
#include "z-term.h"


/**
 * Character translations and definitions.  -JG-
 *
 * Upper case and lower case equivalents of a given ISO Latin-1 character.
 * A character's general "type"; types may be combined.
 *
 * Note that this table assumes use of the standard Angband extended fonts.
 *
 * Notice the accented characters in positions 191+.  If they don't appear
 * correct to you, then you are viewing this table in a non-Latin-1 font.
 * Individual ports are responsible for translations from the game's
 * internal Latin-1 to their system's character encoding(s) (unless ASCII).
 */
byte char_tables[256][CHAR_TABLE_SLOTS] =
{
/* TO_UPPER, TO_LOWER, CHAR TYPE */
    {   0,     0,           0L },               /*        Empty      */
    {   1,     1,  CHAR_SYMBOL },               /*        Solid      */
    {   2,     2,  CHAR_SYMBOL },               /* Mostly solid      */
    {   3,     3,  CHAR_SYMBOL },               /* Wall pattern      */
    {   4,     4,  CHAR_SYMBOL },               /*    Many dots      */
    {   5,     5,  CHAR_SYMBOL },               /*  Medium dots      */
    {   6,     6,  CHAR_SYMBOL },               /*     Few dots      */
    {   7,     7,  CHAR_SYMBOL },               /*     Tiny dot      */
    {   8,     8,  CHAR_SYMBOL },               /*    Small dot      */
    {   9,     9,  CHAR_SYMBOL },               /*   Medium dot      */
    {  10,    10,  CHAR_SYMBOL },               /*    Large dot      */
    {  11,    11,  CHAR_SYMBOL },               /*       Rubble      */
    {  12,    12,  CHAR_SYMBOL },               /*     Treasure      */
    {  13,    13,  CHAR_SYMBOL },               /*  Closed door      */
    {  14,    14,  CHAR_SYMBOL },               /*    Open Door      */
    {  15,    15,  CHAR_SYMBOL },               /*  Broken door      */
    {  16,    16,  CHAR_SYMBOL },               /*       Pillar      */
    {  17,    17,  CHAR_SYMBOL },               /*        Water      */
    {  18,    18,  CHAR_SYMBOL },               /*         Tree      */
    {  19,    19,  CHAR_SYMBOL },               /*    Fire/lava      */
    {  20,    20,  CHAR_SYMBOL },               /*   Pit/portal      */
    {  22,    22,           0L },               /*       Unused      */
    {  22,    22,           0L },               /*       Unused      */
    {  23,    23,           0L },               /*       Unused      */
    {  24,    24,           0L },               /*       Unused      */
    {  25,    25,           0L },               /*       Unused      */
    {  26,    26,           0L },               /*       Unused      */
    {  27,    27,           0L },               /*       Unused      */
    {  28,    28,           0L },               /*       Unused      */
    {  29,    29,           0L },               /*       Unused      */
    {  30,    30,           0L },               /*       Unused      */
    {  31,    31,           0L },               /*       Unused      */

    {  32,    32,   CHAR_BLANK },               /*        Space      */
    {  33,    33,   CHAR_PUNCT },               /*            !      */
    {  34,    34,   CHAR_PUNCT },               /*            "      */
    {  35,    35,   CHAR_PUNCT },               /*            #      */
    {  36,    36,   CHAR_PUNCT },               /*            $      */
    {  37,    37,   CHAR_PUNCT },               /*            %      */
    {  38,    38,   CHAR_PUNCT },               /*            &      */
    {  39,    39,   CHAR_PUNCT },               /*            '      */
    {  40,    40,   CHAR_PUNCT },               /*            (      */
    {  41,    41,   CHAR_PUNCT },               /*            )      */
    {  42,    42,   CHAR_PUNCT },               /*            *      */
    {  43,    43,   CHAR_PUNCT },               /*            +      */
    {  44,    44,   CHAR_PUNCT },               /*            ,      */
    {  45,    45,   CHAR_PUNCT },               /*            -      */
    {  46,    46,   CHAR_PUNCT },               /*            .      */
    {  47,    47,   CHAR_PUNCT },               /*            /      */

    {  48,    48,   CHAR_DIGIT },               /*            0      */
    {  49,    49,   CHAR_DIGIT },               /*            1      */
    {  50,    50,   CHAR_DIGIT },               /*            2      */
    {  51,    51,   CHAR_DIGIT },               /*            3      */
    {  52,    52,   CHAR_DIGIT },               /*            4      */
    {  53,    53,   CHAR_DIGIT },               /*            5      */
    {  54,    54,   CHAR_DIGIT },               /*            6      */
    {  55,    55,   CHAR_DIGIT },               /*            7      */
    {  56,    56,   CHAR_DIGIT },               /*            8      */
    {  57,    57,   CHAR_DIGIT },               /*            9      */
    {  58,    58,   CHAR_DIGIT },               /*            :      */
    {  59,    59,   CHAR_DIGIT },               /*            ;      */

    {  60,    60,   CHAR_PUNCT },               /*            <      */
    {  61,    61,   CHAR_PUNCT },               /*            =      */
    {  62,    62,   CHAR_PUNCT },               /*            >      */
    {  63,    63,   CHAR_PUNCT },               /*            ?      */
    {  64,    64,   CHAR_PUNCT },               /*            @      */
    {  65,    97,   CHAR_UPPER | CHAR_VOWEL },  /*            A      */
    {  66,    98,   CHAR_UPPER },               /*            B      */
    {  67,    99,   CHAR_UPPER },               /*            C      */
    {  68,   100,   CHAR_UPPER },               /*            D      */
    {  69,   101,   CHAR_UPPER | CHAR_VOWEL },  /*            E      */
    {  70,   102,   CHAR_UPPER },               /*            F      */
    {  71,   103,   CHAR_UPPER },               /*            G      */
    {  72,   104,   CHAR_UPPER },               /*            H      */
    {  73,   105,   CHAR_UPPER | CHAR_VOWEL },  /*            I      */
    {  74,   106,   CHAR_UPPER },               /*            J      */
    {  75,   107,   CHAR_UPPER },               /*            K      */
    {  76,   108,   CHAR_UPPER },               /*            L      */
    {  77,   109,   CHAR_UPPER },               /*            M      */
    {  78,   110,   CHAR_UPPER },               /*            N      */
    {  79,   111,   CHAR_UPPER | CHAR_VOWEL },  /*            O      */
    {  80,   112,   CHAR_UPPER },               /*            P      */
    {  81,   113,   CHAR_UPPER },               /*            Q      */
    {  82,   114,   CHAR_UPPER },               /*            R      */
    {  83,   115,   CHAR_UPPER },               /*            S      */
    {  84,   116,   CHAR_UPPER },               /*            T      */
    {  85,   117,   CHAR_UPPER | CHAR_VOWEL },  /*            U      */
    {  86,   118,   CHAR_UPPER },               /*            V      */
    {  87,   119,   CHAR_UPPER },               /*            W      */
    {  88,   120,   CHAR_UPPER },               /*            X      */
    {  89,   121,   CHAR_UPPER },               /*            Y      */
    {  90,   122,   CHAR_UPPER },               /*            Z      */

    {  91,    91,   CHAR_PUNCT },               /*            [      */
    {  92,    92,   CHAR_PUNCT },               /*            \      */
    {  93,    93,   CHAR_PUNCT },               /*            ]      */
    {  94,    94,   CHAR_PUNCT },               /*            ^      */
    {  95,    95,   CHAR_PUNCT },               /*            _      */
    {  96,    96,   CHAR_PUNCT },               /*            `      */
    {  65,    97,   CHAR_LOWER | CHAR_VOWEL },  /*            a      */
    {  66,    98,   CHAR_LOWER },               /*            b      */
    {  67,    99,   CHAR_LOWER },               /*            c      */
    {  68,   100,   CHAR_LOWER },               /*            d      */
    {  69,   101,   CHAR_LOWER | CHAR_VOWEL },  /*            e      */
    {  70,   102,   CHAR_LOWER },               /*            f      */
    {  71,   103,   CHAR_LOWER },               /*            g      */
    {  72,   104,   CHAR_LOWER },               /*            h      */
    {  73,   105,   CHAR_LOWER | CHAR_VOWEL },  /*            i      */
    {  74,   106,   CHAR_LOWER },               /*            j      */
    {  75,   107,   CHAR_LOWER },               /*            k      */
    {  76,   108,   CHAR_LOWER },               /*            l      */
    {  77,   109,   CHAR_LOWER },               /*            m      */
    {  78,   110,   CHAR_LOWER },               /*            n      */
    {  79,   111,   CHAR_LOWER | CHAR_VOWEL },  /*            o      */
    {  80,   112,   CHAR_LOWER },               /*            p      */
    {  81,   113,   CHAR_LOWER },               /*            q      */
    {  82,   114,   CHAR_LOWER },               /*            r      */
    {  83,   115,   CHAR_LOWER },               /*            s      */
    {  84,   116,   CHAR_LOWER },               /*            t      */
    {  85,   117,   CHAR_LOWER | CHAR_VOWEL },  /*            u      */
    {  86,   118,   CHAR_LOWER },               /*            v      */
    {  87,   119,   CHAR_LOWER },               /*            w      */
    {  88,   120,   CHAR_LOWER },               /*            x      */
    {  89,   121,   CHAR_LOWER },               /*            y      */
    {  90,   122,   CHAR_LOWER },               /*            z      */
    { 123,   123,   CHAR_PUNCT },               /*            {    */
    { 124,   124,   CHAR_PUNCT },               /*            |      */
    { 125,   125,   CHAR_PUNCT },               /*            }      */
    { 126,   126,   CHAR_PUNCT },               /*            ~      */
    { 127,   127,  CHAR_SYMBOL },               /* Wall pattern      */

    { 128,   128,           0L },               /*       Unused      */
    { 129,   129,           0L },               /*       Unused      */
    { 130,   130,           0L },               /*       Unused      */
    { 131,   131,           0L },               /*       Unused      */
    { 132,   132,           0L },               /*       Unused      */
    { 133,   133,           0L },               /*       Unused      */
    { 134,   134,           0L },               /*       Unused      */
    { 135,   135,           0L },               /*       Unused      */
    { 136,   136,           0L },               /*       Unused      */
    { 137,   137,           0L },               /*       Unused      */
    { 138,   138,           0L },               /*       Unused      */
    { 139,   139,           0L },               /*       Unused      */
    { 140,   140,           0L },               /*       Unused      */
    { 141,   141,           0L },               /*       Unused      */
    { 142,   142,           0L },               /*       Unused      */
    { 143,   143,           0L },               /*       Unused      */
    { 144,   144,           0L },               /*       Unused      */
    { 145,   145,           0L },               /*       Unused      */
    { 146,   146,           0L },               /*       Unused      */
    { 147,   147,           0L },               /*       Unused      */
    { 148,   148,           0L },               /*       Unused      */
    { 149,   149,           0L },               /*       Unused      */
    { 150,   150,           0L },               /*       Unused      */
    { 151,   151,           0L },               /*       Unused      */
    { 152,   152,           0L },               /*       Unused      */
    { 153,   153,           0L },               /*       Unused      */
    { 154,   154,           0L },               /*       Unused      */
    { 155,   155,           0L },               /*       Unused      */
    { 156,   156,           0L },               /*       Unused      */
    { 157,   157,           0L },               /*       Unused      */
    { 158,   158,           0L },               /*       Unused      */
    { 159,   159,           0L },               /*       Unused      */
    { 160,   160,           0L },               /*       Unused      */

    { 161,   161,   CHAR_PUNCT },               /*       iexcl   ¡   */
    { 162,   162,   CHAR_PUNCT },               /*        euro   ¢   */
    { 163,   163,   CHAR_PUNCT },               /*       pound   £   */
    { 164,   164,   CHAR_PUNCT },               /*      curren   ¤   */
    { 165,   165,   CHAR_PUNCT },               /*         yen   ¥   */
    { 166,   166,   CHAR_PUNCT },               /*      brvbar   ¦   */
    { 167,   167,   CHAR_PUNCT },               /*        sect   §   */
    { 168,   168,  CHAR_SYMBOL },               /*  Bolt - vert      */
    { 169,   169,  CHAR_SYMBOL },               /*  Bolt - horz      */
    { 170,   170,  CHAR_SYMBOL },               /*  Bolt -rdiag      */
    { 171,   171,  CHAR_SYMBOL },               /*  Bolt -ldiag      */
    { 172,   172,  CHAR_SYMBOL },               /*  Spell-cloud      */
    { 173,   173,  CHAR_SYMBOL },               /*   Spell-elec      */
    { 174,   174,  CHAR_SYMBOL },               /*  Spell-explo      */

    { 175,   175,           0L },               /*       Unused      */
    { 176,   176,           0L },               /*       Unused      */
    { 177,   177,           0L },               /*       Unused      */
    { 178,   178,           0L },               /*       Unused      */
    { 179,   179,           0L },               /*       Unused      */
    { 180,   180,           0L },               /*       Unused      */
    { 181,   181,           0L },               /*       Unused      */
    { 182,   182,           0L },               /*       Unused      */
    { 183,   183,           0L },               /*       Unused      */
    { 184,   184,           0L },               /*       Unused      */
    { 185,   185,           0L },               /*       Unused      */
    { 186,   186,           0L },               /*       Unused      */
    { 187,   187,           0L },               /*       Unused      */
    { 188,   188,           0L },               /*       Unused      */
    { 189,   189,           0L },               /*       Unused      */
    { 190,   190,           0L },               /*       Unused      */

    { 191,   191,   CHAR_PUNCT },               /*      iquest   ¿   */
    { 192,   224,   CHAR_UPPER | CHAR_VOWEL },  /*      Agrave   À   */
    { 193,   225,   CHAR_UPPER | CHAR_VOWEL },  /*      Aacute   Á   */
    { 194,   226,   CHAR_UPPER | CHAR_VOWEL },  /*       Acirc   Â   */
    { 195,   227,   CHAR_UPPER | CHAR_VOWEL },  /*      Atilde   Ã   */
    { 196,   228,   CHAR_UPPER | CHAR_VOWEL },  /*        Auml   Ä   */
    { 197,   229,   CHAR_UPPER | CHAR_VOWEL },  /*       Aring   Å   */
    { 198,   230,   CHAR_UPPER | CHAR_VOWEL },  /*       Aelig   Æ   */
    { 199,   231,   CHAR_UPPER },               /*      Ccedil   Ç   */
    { 200,   232,   CHAR_UPPER | CHAR_VOWEL },  /*      Egrave   È   */
    { 201,   233,   CHAR_UPPER | CHAR_VOWEL },  /*      Eacute   É   */
    { 202,   234,   CHAR_UPPER | CHAR_VOWEL },  /*       Ecirc   Ê   */
    { 203,   235,   CHAR_UPPER | CHAR_VOWEL },  /*        Euml   Ë   */
    { 204,   236,   CHAR_UPPER | CHAR_VOWEL },  /*      Igrave   Ì   */
    { 205,   237,   CHAR_UPPER | CHAR_VOWEL },  /*      Iacute   Í   */
    { 206,   238,   CHAR_UPPER | CHAR_VOWEL },  /*       Icirc   Î   */
    { 207,   239,   CHAR_UPPER | CHAR_VOWEL },  /*        Iuml   Ï   */
    { 208,   240,   CHAR_UPPER },               /*         ETH   Ð   */
    { 209,   241,   CHAR_UPPER },               /*      Ntilde   Ñ   */
    { 210,   242,   CHAR_UPPER | CHAR_VOWEL },  /*      Ograve   Ò   */
    { 211,   243,   CHAR_UPPER | CHAR_VOWEL },  /*      Oacute   Ó   */
    { 212,   244,   CHAR_UPPER | CHAR_VOWEL },  /*       Ocirc   Ô   */
    { 213,   245,   CHAR_UPPER | CHAR_VOWEL },  /*      Otilde   Õ   */
    { 214,   246,   CHAR_UPPER | CHAR_VOWEL },  /*        Ouml   Ö   */
    { 215,   215,           0L },               /*       Unused      */
    { 216,   248,   CHAR_UPPER | CHAR_VOWEL },  /*      Oslash   Ø   */
    { 217,   249,   CHAR_UPPER | CHAR_VOWEL },  /*      Ugrave   Ù   */
    { 218,   250,   CHAR_UPPER | CHAR_VOWEL },  /*      Uacute   Ú   */
    { 219,   251,   CHAR_UPPER | CHAR_VOWEL },  /*       Ucirc   Û   */
    { 220,   252,   CHAR_UPPER | CHAR_VOWEL },  /*        Uuml   Ü   */
    { 221,   253,   CHAR_UPPER },               /*      Yacute   Ý   */
    { 222,   254,   CHAR_UPPER },               /*       THORN   Þ   */
    { 223,   223,   CHAR_LOWER },               /*       szlig   ß   */

    { 192,   224,   CHAR_LOWER | CHAR_VOWEL },  /*      agrave   à   */
    { 193,   225,   CHAR_LOWER | CHAR_VOWEL },  /*      aacute   á   */
    { 194,   226,   CHAR_LOWER | CHAR_VOWEL },  /*       acirc   â   */
    { 195,   227,   CHAR_LOWER | CHAR_VOWEL },  /*      atilde   ã   */
    { 196,   228,   CHAR_LOWER | CHAR_VOWEL },  /*        auml   ä   */
    { 197,   229,   CHAR_LOWER | CHAR_VOWEL },  /*       aring   å   */
    { 198,   230,   CHAR_LOWER | CHAR_VOWEL },  /*       aelig   æ   */
    { 199,   231,   CHAR_LOWER },               /*      ccedil   ç   */
    { 200,   232,   CHAR_LOWER | CHAR_VOWEL },  /*      egrave   è   */
    { 201,   233,   CHAR_LOWER | CHAR_VOWEL },  /*      eacute   é   */
    { 202,   234,   CHAR_LOWER | CHAR_VOWEL },  /*       ecirc   ê   */
    { 203,   235,   CHAR_LOWER | CHAR_VOWEL },  /*        euml   ë   */
    { 204,   236,   CHAR_LOWER | CHAR_VOWEL },  /*      igrave   ì   */
    { 205,   237,   CHAR_LOWER | CHAR_VOWEL },  /*      iacute   í   */
    { 206,   238,   CHAR_LOWER | CHAR_VOWEL },  /*       icirc   î   */
    { 207,   239,   CHAR_LOWER | CHAR_VOWEL },  /*        iuml   ï   */
    { 208,   240,   CHAR_LOWER },               /*         eth   ð   */
    { 209,   241,   CHAR_LOWER },               /*      ntilde   ñ   */
    { 210,   242,   CHAR_LOWER | CHAR_VOWEL },  /*      ograve   ò   */
    { 211,   243,   CHAR_LOWER | CHAR_VOWEL },  /*      oacute   ó   */
    { 212,   244,   CHAR_LOWER | CHAR_VOWEL },  /*       ocirc   ô   */
    { 213,   245,   CHAR_LOWER | CHAR_VOWEL },  /*      otilde   õ   */
    { 214,   246,   CHAR_LOWER | CHAR_VOWEL },  /*        ouml   ö   */
    { 247,   247,           0L },               /*       Unused      */
    { 216,   248,   CHAR_LOWER | CHAR_VOWEL },  /*      oslash   ø   */
    { 217,   249,   CHAR_LOWER | CHAR_VOWEL },  /*      ugrave   ù   */
    { 218,   250,   CHAR_LOWER | CHAR_VOWEL },  /*      uacute   ú   */
    { 219,   251,   CHAR_LOWER | CHAR_VOWEL },  /*       ucirc   û   */
    { 220,   252,   CHAR_LOWER | CHAR_VOWEL },  /*        uuml   ü   */
    { 221,   253,   CHAR_LOWER },               /*      yacute   ý   */
    { 222,   254,   CHAR_LOWER },               /*       thorn   þ   */
    { 121,   255,   CHAR_LOWER },               /*        yuml   ÿ   */
};





/**
 * Translate from encodes to extended 8-bit characters and back again.
 */
static const xchar_type latin1_encode[] =
{
    { "`A", 192 },  { "'A", 193 },  { "^A", 194 },  { "~A", 195 },
    { "\"A", 196 },  { "*A", 197 },  { ",C", 199 },  { "`E", 200 },
    { "'E", 201 },  { "^E", 202 }, { "\"E", 203 },  { "`I", 204 },
    { "'I", 205 },  { "^I", 206 }, { "\"I", 207 },  { "~N", 209 },
    { "`O", 210 },  { "'O", 211 },  { "^O", 212 },  { "~O", 213 },
	{ "\"O", 214 },  { "/O", 216 },  { "`U", 217 },  { "'U", 218 },
    { "^U", 219 }, { "\"U", 220 },  { "'Y", 221 },  { "`a", 224 },
    { "'a", 225 },  { "^a", 226 },  { "~a", 227 }, { "\"a", 228 },
    { "*a", 229 },  { ",c", 231 },  { "`e", 232 },  { "'e", 233 },
    { "^e", 234 }, { "\"e", 235 },  { "`i", 236 },  { "'i", 237 },
    { "^i", 238 }, { "\"i", 239 },  { "~n", 241 },  { "`o", 242 },
    { "'o", 243 },  { "^o", 244 },  { "~o", 245 }, { "\"o", 246 },
    { "/o", 248 },  { "`u", 249 },  { "'u", 250 },  { "^u", 251 },
    { "\"u", 252 },  { "'y", 253 }, { "\"y", 255 },

    { "iexcl", 161 }, { "euro", 162 }, { "pound", 163 }, { "curren", 164 },
    { "yen", 165 },   { "brvbar", 166 }, { "sect", 167 }, { "Agrave", 192 },
    { "Aacute", 193 }, { "Acirc", 194 }, { "Atilde", 195 }, { "Auml", 196 },
    { "Aring", 197 }, { "Aelig", 198 }, { "Ccedil", 199 }, { "Egrave", 200 },
    { "Eacute", 201 }, { "Ecirc", 202 }, { "Euml", 203 }, { "Igrave", 204 },
    { "Iacute", 205 }, { "Icirc", 206 }, { "Iuml", 207 }, { "ETH", 208 },
    { "Ntilde", 209 }, { "Ograve", 210 }, { "Oacute", 211 }, { "Ocirc", 212 },
    { "Otilde", 213 }, { "Ouml", 214 }, { "Oslash", 216 }, { "Ugrave", 217 },
    { "Uacute", 218 }, { "Ucirc", 219 }, { "Uuml", 220 }, { "Yacute", 221 },
    { "THORN", 222 }, { "szlig", 223 }, { "agrave", 224 }, { "aacute", 225 },
    { "acirc", 226 }, { "atilde", 227 }, { "auml", 228 }, { "aring", 229 },
    { "aelig", 230 }, { "ccedil", 231 }, { "egrave", 232 }, { "eacute", 233 },
    { "ecirc", 234 }, { "euml", 235 }, { "igrave", 236 }, { "iacute", 237 },
    { "icirc", 238 }, { "iuml", 239 }, { "eth", 240 },   { "ntilde", 241 },
    { "ograve", 242 }, { "oacute", 243 }, { "ocirc", 244 }, { "otilde", 245 },
    { "ouml", 246 }, { "oslash", 248 }, { "ugrave", 249 }, { "uacute", 250 },
    { "ucirc", 251 }, { "uuml", 252 }, { "yacute", 253 }, { "thorn", 254 },
    { "yuml", 255 },   { "\0", 0 }
};


/**
 * Link to the xchar_trans function.
 */
void xchar_trans_hook(char *s, int encoding)
{
 	/* Option to translate into ASCII */
 	if (encoding == ASCII)
 	{
 		if (*s < 0) *s = seven_bit_translation[128 + *s];
 	}

 	/* Option to translate into system-specific character set */
 	else if (encoding == SYSTEM_SPECIFIC)
 	{
 		if (*s < 0) *s = xchar_trans(*s);
	}
}


/**
 * Given what we think is an encode, return a Latin-1 character position.
 */
static byte encode_to_xchar(char *encode)
{
 	int i;

 	/* Scan until we hit the end-of-table marker */
 	for (i = 0; latin1_encode[i].c; i++)
 	{
 		/* We found the encode; return the character */
 		if (streq(encode, latin1_encode[i].tag))
 			return (latin1_encode[i].c);
 	}

 	/* This encode is not recognized */
 	return (0);
}

/**
 * Read an encode.  Return the Latin-1 character position if successful.
 */
bool get_encode(char *str, char *c)
{
 	int n = 0;
 	char *s;
 	char encode[80];

 	/* Assume empty char */
 	*c = '\0';

 	/* An encode must start with a '[' */
 	if (str[0] != '[') return (FALSE);

 	/* Copy the encode (between brackets) */
 	for (s = str + 1; ((n < 80) && (*s) && (*s != ']')); n++)
 	{
 		encode[n] = *s++;
 	}

 	/* End the encode */
 	encode[n] = '\0';

 	/* We have a trailing bracket */
 	if (*s == ']')
 	{
 		/* Look up extended character */
 		*c = (char)encode_to_xchar(encode);

 		/* Encode is legal -- return the char */
 		if (*c) return (TRUE);
 	}

 	/* Illegal encode */
 	return (FALSE);
}



/**
 * Take a 7-bit ASCII string and use any encodes in it to insert 8-bit
 * characters.  Use the Latin-1 (ISO) standard by default.  -LM-
 *
 * Optionally, translate into 7-bit ASCII or a system-specific character set.
 *
 * The input string must be '\0'-terminated, and should not be greater than
 * 1024 character in length (we check this).
 */
void xstr_trans(char *str, int encoding)
{
 	/* Assume no encodes in this string */
 	bool flag = FALSE;

 	int n, c;

 	char *s, *b;
 	char buf[1024];
 	char encode[80];


 	/* Require a string */
 	if (!str) return;

 	/* Start at the beginning */
 	s = str;
 	b = buf;

 	/* Scan the string */
 	for (; *s;)
 	{
 		/* Paranoia -- check bounds */
 		if (b - buf > 1022) break;

 		/* Character is a [ */
 		if (*s == '[')
 		{
 			/* Remember where we are */
 			char *s_old = s;

 			/* Assume illegal */
 			c = 0;

 			/* Skip past the open bracket */
 			s += 1;

 			/* Copy the encode (between brackets) */
 			for (n = 0; ((n < 79) && (*s) && (*s != ']')); n++)
 			{
 				encode[n] = *s++;
 			}

 			/* End the encode */
 			encode[n] = '\0';

 			/* We have a trailing bracket */
 			if (*s == ']')
 			{
 				/* Go to next character */
 				s++;

 				/* Look up extended character */
 				c = (char)encode_to_xchar(encode);
 			}

 			/* Encode is legal */
 			if (c)
 			{
 				/* Note the encode */
 				flag = TRUE;

				/* Save it */
 				*b++ = c;
 			}

 			/* Encode is illegal */
 			else
 			{
 				/* Return to start of encode */
 				s = s_old;

 				/* Copy the '[', go to the next character */
 				*b++ = *s++;
 			}
 		}

 		/* No encoding recognized */
 		else
 		{
 			/* Copy the character */
 			*b++ = *s++;
 		}
	}

 	/* End the string */
 	*b = '\0';

 	/* Copy the edited string back, if we've found encodes */
 	if (flag) strcpy(str, buf);

	/* Translate the string if we don't want standard Latin-1 */
	if (encoding != LATIN1)
	{
		for (s = str; *s; s++) xchar_trans_hook(s, encoding);
	}
}

/**
 *  Translate a Latin-1 string into escaped ASCII
 *  We assume that the contents of the source string use the Latin-1 encoding
 */
void escape_latin1(char *dest, size_t max, cptr src)
{
	size_t i = 0;

	/* Make space for the trailing null character */
	if (max > 0) --max;

	/* Copy the source string into the ouput string escaping the non-ascii characters */
	while (*src && (i < max))
	{
		/* Make a copy of the character */
		byte chr = (byte)*src++;

		/* Non-ascii characters get special treatment */
		if (chr > 127)
		{
			int j;
			const char *tag = NULL;

			/* Find the escape secuence of the character */
			for (j = 0; latin1_encode[j].c > 0; j++)
			{
				if (latin1_encode[j].c == chr)
				{
					tag = latin1_encode[j].tag;

					break;
				}
			}

			/* Found? */
			if (tag)
			{
				/* Append the opening delimiter */
				if (i < max) dest[i++] = '[';

				/* Append the escape secuence */
				for (j = 0; tag[j] && (i < max); j++)
				{
					dest[i++] = tag[j];
				}

				/* Append the closing delimiter */
				if (i < max) dest[i++] = ']';

				/* Done */
				continue;
			}
		}

		/* Common case. We just append the character */
		dest[i++] = (char)chr;
	}

	/* Trailing null character */
	dest[i] = '\0';
}

/**
 * Basic "vararg" format function.
 *
 * This function takes a buffer, a max byte count, a format string, and
 * a va_list of arguments to the format string, and uses the format string
 * and the arguments to create a string to the buffer.  The string is
 * derived from the format string and the arguments in the manner of the
 * "sprintf()" function, but with some extra "format" commands.  Note that
 * this function will never use more than the given number of bytes in the
 * buffer, preventing messy invalid memory references.  This function then
 * returns the total number of non-null bytes written into the buffer.
 *
 * Method: Let "str" be the (unlimited) created string, and let "len" be the
 * smaller of "max-1" and "strlen(str)".  We copy the first "len" chars of
 * "str" into "buf", place "\0" into buf[len], and return "len".
 *
 * In English, we do a sprintf() into "buf", a buffer with size "max",
 * and we return the resulting value of "strlen(buf)", but we allow some
 * special format commands, and we are more careful than "sprintf()".
 *
 * Typically, "max" is in fact the "size" of "buf", and thus represents
 * the "number" of chars in "buf" which are ALLOWED to be used.  An
 * alternative definition would have required "buf" to hold at least
 * "max+1" characters, and would have used that extra character only
 * in the case where "buf" was too short for the result.  This would
 * give an easy test for "overflow", but a less "obvious" semantics.
 *
 * Note that if the buffer was "too short" to hold the result, we will
 * always return "max-1", but we also return "max-1" if the buffer was
 * "just long enough".  We could have returned "max" if the buffer was
 * too short, not written a null, and forced the programmer to deal with
 * this special case, but I felt that it is better to at least give a
 * "usable" result when the buffer was too long instead of either giving
 * a memory overwrite like "sprintf()" or a non-terminted string like
 * "strncpy()".  Note that "strncpy()" also "null-pads" the result.
 *
 * Note that in most cases "just long enough" is probably "too short".
 *
 * We should also consider extracting and processing the "width" and other
 * "flags" by hand, it might be more "accurate", and it would allow us to
 * remove the limit (1000 chars) on the result of format sequences.
 *
 * Also, some sequences, such as "%+d" by hand, do not work on all machines,
 * and could thus be correctly handled here.
 *
 * Error detection in this routine is not very graceful, in particular,
 * if an error is detected in the format string, we simply "pre-terminate"
 * the given buffer to a length of zero, and return a "length" of zero.
 * The contents of "buf", except for "buf[0]", may then be undefined.
 */
size_t vstrnfmt(char *buf, size_t max, cptr fmt, va_list vp)
{
	cptr s;

	/* The argument is "long" */
	bool do_long;

	/* The argument needs "processing" */
	bool do_xtra;

	/* Bytes used in buffer */
	size_t n;

	/* Bytes used in format sequence */
	size_t q;

	/* Format sequence */
	char aux[128];

	/* Resulting string */
	char tmp[1024];


	/* Fatal error - no buffer length */
	if (!max) quit("Called vstrnfmt() with empty buffer!");


	/* Mega-Hack -- treat "no format" as "empty string" */
	if (!fmt) fmt = "";


	/* Begin the buffer */
	n = 0;

	/* Begin the format string */
	s = fmt;

	/* Scan the format string */
	while (TRUE)
	{
		type_union tval = END;

		/* All done */
		if (!*s) break;

		/* Normal character */
		if (*s != '%')
		{
			/* Check total length */
			if (n == max-1) break;

			/* Save the character */
			buf[n++] = *s++;

			/* Continue */
			continue;
		}

		/* Skip the "percent" */
		s++;

		/* Pre-process "%%" */
		if (*s == '%')
		{
			/* Check total length */
			if (n == max-1) break;

			/* Save the percent */
			buf[n++] = '%';

			/* Skip the "%" */
			s++;

			/* Continue */
			continue;
		}

		/* Pre-process "%n" */
		if (*s == 'n')
		{
			size_t *arg;

			/* Get the next argument */
			arg = va_arg(vp, size_t *);

			/* Save the current length */
			(*arg) = n;

			/* Skip the "n" */
			s++;

			/* Continue */
			continue;
		}


		/* Begin the "aux" string */
		q = 0;

		/* Save the "percent" */
		aux[q++] = '%';

		/* Assume no "long" argument */
		do_long = FALSE;

		/* Assume no "xtra" processing */
		do_xtra = FALSE;

		/* Build the "aux" string */
		while (TRUE)
		{
			/* Error -- format sequence is not terminated */
			if (!*s)
			{
				/* Terminate the buffer */
				buf[0] = '\0';

				/* Return "error" */
				return (0);
			}

			/* Error -- format sequence may be too long */
			if (q > 100)
			{
				/* Terminate the buffer */
				buf[0] = '\0';

				/* Return "error" */
				return (0);
			}

			/* Handle "alphabetic" chars */
			if (isalpha((unsigned char)*s))
			{
				/* Hack -- handle "long" request */
				if (*s == 'l')
				{
					/* Save the character */
					aux[q++] = *s++;

					/* Note the "long" flag */
					do_long = TRUE;
				}

				/* Mega-Hack -- handle "extra-long" request */
				else if (*s == 'L')
				{
					/* Error -- illegal format char */
					buf[0] = '\0';

					/* Return "error" */
					return (0);
				}

				/* Handle normal end of format sequence */
				else
				{
					/* Save the character */
					aux[q++] = *s++;

					/* Stop processing the format sequence */
					break;
				}
			}

			/* Handle "non-alphabetic" chars */
			else
			{
				/* Hack -- Handle 'star' (for "variable length" argument) */
				if (*s == '*')
				{
					int arg;

					/* Get the next argument */
					arg = va_arg(vp, int);

					/* Hack -- append the "length" */
					sprintf(aux + q, "%d", arg);

					/* Hack -- accept the "length" */
					while (aux[q]) q++;

					/* Skip the "*" */
					s++;
				}

				/* Mega-Hack -- Handle 'caret' (for "uppercase" request) */
				else if (*s == '^')
				{
					/* Note the "xtra" flag */
					do_xtra = TRUE;

					/* Skip the "^" */
					s++;
				}

				/* Collect "normal" characters (digits, "-", "+", ".", etc) */
				else
				{
					/* Save the character */
					aux[q++] = *s++;
				}
			}
		}


		/* Terminate "aux" */
		aux[q] = '\0';

		/* Clear "tmp" */
		tmp[0] = '\0';

		/* Parse a type_union */
		if (aux[q-1] == 'y')
		{
			tval = va_arg(vp, type_union);

			if (do_long)
			{
				/* Error -- illegal type_union argument */
				buf[0] = '\0';

				/* Return "error" */
				return (0);
			}

			/* Replace aux terminator with proper printf char */
			if (tval.t == T_CHAR) aux[q-1] = 'c';
			else if (tval.t == T_INT) aux[q-1] = 'd';
			else if (tval.t == T_FLOAT) aux[q-1] = 'f';
			else if (tval.t == T_STRING) aux[q-1] = 's';
			else
			{ 
				buf[0] = '\0';
				return (0);
			}
		}

		/* Process the "format" symbol */
		switch (aux[q-1])
		{
			/* Simple Character -- standard format */
			case 'c':
			{
				int arg;

				/* Get the next argument */
				arg = tval.t == T_END ? va_arg(vp, int) : tval.u.c;

				/* Format the argument */
				sprintf(tmp, aux, arg);

				/* Done */
				break;
			}

			/* Signed Integers -- standard format */
			case 'd': case 'i':
			{
				if (do_long)
				{
					long arg;

					/* Get the next argument */
					arg = va_arg(vp, long);

					/* Format the argument */
					sprintf(tmp, aux, arg);
				}
				else
				{
					int arg;

					/* Get the next argument */
					arg = tval.t == T_END ? va_arg(vp, int) : tval.u.i;

					/* Format the argument */
					sprintf(tmp, aux, arg);
				}

				/* Done */
				break;
			}

			/* Unsigned Integers -- various formats */
			case 'u': case 'o': case 'x': case 'X':
			{
				if (do_long)
				{
					unsigned long arg;

					/* Get the next argument */
					arg = va_arg(vp, unsigned long);

					/* Format the argument */
					sprintf(tmp, aux, arg);
				}
				else
				{
					unsigned int arg;

					/* Get the next argument */
					arg = va_arg(vp, unsigned int);

					/* Format the argument */
					sprintf(tmp, aux, arg);
				}

				/* Done */
				break;
			}

			/* Floating Point -- various formats */
			case 'f':
			case 'e': case 'E':
			case 'g': case 'G':
			{
				double arg;

				/* Get the next argument */
				arg = tval.t == T_END ? va_arg(vp, double) : tval.u.f;

				/* Format the argument */
				sprintf(tmp, aux, arg);

				/* Done */
				break;
			}

			/* Pointer -- implementation varies */
			case 'p':
			{
				void *arg;

				/* Get the next argument */
				arg = va_arg(vp, void*);

				/* Format the argument */
				sprintf(tmp, aux, arg);

				/* Done */
				break;
			}

			/* String */
			case 's':
			{
				cptr arg;
				char arg2[1024];

				/* Get the next argument */
				arg = tval.t == T_END ? va_arg(vp, cptr) : tval.u.s;

				/* Hack -- convert NULL to EMPTY */
				if (!arg) arg = "";

				/* Prevent buffer overflows */
				(void)my_strcpy(arg2, arg, sizeof(arg2));

				/* Translate it to 8-bit (Latin-1) */
 				xstr_trans(arg2, LATIN1);

				/* Format the argument */
				sprintf(tmp, aux, arg2);

				/* Done */
				break;
			}

#if 0 /* Later */
			/* Binary */
			case 'b':
			{
				int arg;
				size_t i, max = 32;
				u32b bitmask;
				char out[32 + 1];

				/* Get the next argument */
				arg = va_arg(vp, int);

				/* Check our aux string */
				switch (aux[0])
				{
					case '1': max = 2;  break;
					case '2': max = 4;  break;
					case '3': max = 8;  break;
					case '4': max = 16; break;
					default: 
					case '5': max = 32; break;
				}
				/* Format specially */
				for (i = 1; i <= max; i++, bitmask *= 2)
				{
					if (arg & bitmask) out[max - i] = '1';
					else out[max - i] = '0';
				}

				/* Terminate */
				out[max] = '\0';

				/* Append the argument */
				my_strcpy(tmp, out, sizeof tmp);

				/* Done */
				break;
			}
#endif

			/* Oops */
			default:
			{
				/* Error -- illegal format char */
				buf[0] = '\0';

				/* Return "error" */
				return (0);
			}
		}


		/* Mega-Hack -- handle "capitalization" */
		if (do_xtra)
		{
			for (q = 0; tmp[q]; q++)
			{
				/* Notice first non-space */
				if (!my_isspace((unsigned char)tmp[q]))
				{
					/* Capitalize if possible */
					if (my_islower((unsigned char)tmp[q]))
						tmp[q] = my_toupper((unsigned char)tmp[q]);

					/* Done */
					break;
				}
			}
		}

		/* Now append "tmp" to "buf" */
		for (q = 0; tmp[q]; q++)
		{
			/* Check total length */
			if (n == max-1) break;

			/* Save the character */
			buf[n++] = tmp[q];
		}
	}


	/* Terminate buffer */
	buf[n] = '\0';

	/* Return length */
	return (n);
}


/**
 * Add a formatted string to the end of a string
 */
void strnfcat(char *str, size_t max, size_t *end, cptr fmt, ...)
{
	size_t len;

	va_list vp;

	/* Paranoia */
	if (*end >= max) return;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Build the string */
	len = vstrnfmt(&str[*end], max - *end, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Change the end value */
	*end += len;
}


static char *format_buf = NULL;
static size_t format_len = 0;


/**
 * Do a vstrnfmt (see above) into a (growable) static buffer.
 * This buffer is usable for very short term formatting of results.
 */
char *vformat(cptr fmt, va_list vp)
{
	/* Initial allocation */
	if (!format_buf)
	{
		format_len = 1024;
		C_MAKE(format_buf, format_len, char);
	}

	/* Null format yields last result */
	if (!fmt) return (format_buf);

	/* Keep going until successful */
	while (1)
	{
		size_t len;

		/* Build the string */
		len = vstrnfmt(format_buf, format_len, fmt, vp);

		/* Success */
		if (len < format_len-1) break;

		/* Grow the buffer */
		KILL(format_buf);
		format_len = format_len * 2;
		C_MAKE(format_buf, format_len, char);
	}

	/* Return the new buffer */
	return (format_buf);
}

void vformat_kill(void)
{
	KILL(format_buf);
}


/**
 * Do a vstrnfmt (see above) into a buffer of a given size.
 */
size_t strnfmt(char *buf, size_t max, cptr fmt, ...)
{
	size_t len;

	va_list vp;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Do the va_arg fmt to the buffer */
	len = vstrnfmt(buf, max, fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Return the number of bytes written */
	return (len);
}


/**
 * Do a vstrnfmt() into (see above) into a (growable) static buffer.
 * This buffer is usable for very short term formatting of results.
 * Note that the buffer is (technically) writable, but only up to
 * the length of the string contained inside it.
 */
char *format(cptr fmt, ...)
{
	char *res;
	va_list vp;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args */
	res = vformat(fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Return the result */
	return (res);
}




/**
 * Vararg interface to plog()
 */
void plog_fmt(cptr fmt, ...)
{
	char *res;
	va_list vp;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args */
	res = vformat(fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Call plog */
	plog(res);
}



/**
 * Vararg interface to quit()
 */
void quit_fmt(cptr fmt, ...)
{
	char *res;
	va_list vp;

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format */
	res = vformat(fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Call quit() */
	quit(res);
}
