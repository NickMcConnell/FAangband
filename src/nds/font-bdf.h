/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2 -*-
   font-bdf.h --- simple, self-contained code for manipulating BDF fonts.

   Copyright © 2001 Jamie Zawinski <jwz@jwz.org>

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  No representations are made about the suitability of this
   software for any purpose.  It is provided "as is" without express or 
   implied warranty.
 */

#ifndef __FONT_BDF__
#define __FONT_BDF__

#define TEXT_COLOUR_BASE 64

struct font_char {
  int lbearing;  /* origin to left edge of raster */
  int width;     /* advance to next char's origin */
  int descent;   /* baseline to bottom edge of raster */
  struct ppm *ppm;
};

struct font {
  char *name;
  int ascent;
  int descent;
  int monochrome_p;
  int height;
  struct font_char chars[256];
};

extern int font_bdf_init();

/* Filename below may be "-" for stdin. */
extern struct font *read_bdf (const char *filename);

/* returns dimensions of font */
extern void bdf_dims (struct font *font, int *minwd, int *avgwd,
                      int *maxwd, int *ht);

/* Copies the font and the PPMs in it */
struct font * copy_font (struct font *font);
extern void free_font (struct font *font);

/*
   Compute the length of the given string when drawn.
 */

void text_dims(struct font *fnt, char *str, int *width, int *height);


/* draw a string with origin XY.
   Alignment 0 means center on the Y axis;
   -1 means flushright; 1 means flushleft.
   Alpha ranges from 0-255.
   Newlines are allowed; tabs are not handled specially.
 */
extern void draw_string (struct font *font, char *string,
                         struct ppm *into, int x, int y,
                         int fg, int bg);

extern int
bdf_draw_char (struct font *font, const unsigned char c,
           struct ppm *into, int x, int y,
           unsigned char fg, unsigned char bg,
               int uline);


#endif /* __FONT_BDF__ */
