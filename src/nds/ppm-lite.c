/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2 -*-
   ppm-lite.c --- simple, self-contained code for manipulating
   PBM, PGM, and PPM data.

   Copyright © 2001 Jamie Zawinski <jwz@jwz.org>

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  No representations are made about the suitability of this
   software for any purpose.  It is provided "as is" without express or 
   implied warranty.
 */

#include "config.h"
#include "ppm-lite.h"

#include "config.h"
#include "ppm-lite.h"
#include "font-bdf.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nds.h>

#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))

struct ppm *alloc_ppm(int width, int height)
{
  struct ppm *img;

  img = (struct ppm *)malloc(sizeof(struct ppm));

  img->width = width;
  img->height = height;
  img->bitmap = (unsigned char *)malloc(width * height);

  clear_ppm(img, 0);

  return img;
}

void clear_ppm(struct ppm *img, unsigned char bg)
{
  memset(img->bitmap, bg, img->width * img->height);
}


/* Manipulating image data
 */

struct ppm *
copy_ppm (struct ppm *ppm)
{
  struct ppm *ppm2 = (struct ppm *) calloc (1, sizeof(*ppm));
  int n;
  memcpy (ppm2, ppm, sizeof(*ppm2));
  n = ppm->width * ppm->height;
  ppm2->bitmap = (unsigned char *) calloc (1, n);
  if (!ppm2->bitmap)
    {
      iprintf ("out of memory (%d x %d)\n",
               ppm->width, ppm->height);
      return NULL;
    }
  memcpy (ppm2->bitmap, ppm->bitmap, n);
  return ppm2;
}


void
free_ppm (struct ppm *ppm)
{
  free (ppm->bitmap);
  free (ppm);
}


void
get_pixel (struct ppm *ppm,
           int x, int y,
           unsigned char *pixel)
{
  if (x < 0 || x >= ppm->width ||
      y < 0 || y >= ppm->height)
    {
      *pixel = 0;
    }
  else
    {
      unsigned char *p = ppm->bitmap + ((y * ppm->width) + x);
      *pixel = *p;
    }
}

void
put_pixel (struct ppm *ppm,
           int x, int y,
           unsigned char pixel)
{
  if (x < 0 || x >= ppm->width ||
      y < 0 || y >= ppm->height)
    {
      /* no-op */
    }
  else
    {
      unsigned char *p = ppm->bitmap + ((y * ppm->width) + x);

      *p = pixel;
    }
}


/* Paste part of one image into another, with all necessary clipping.
   Alpha controls the blending of the new image into the old image
    (and any alpha already in the new image is also taken into account.)
  If override_fg/bg is an RGB value (0xNNNNNN) then any dark/light values
    in the source image are assumed to be that, and only the source image's
    intensity and alpha are used.  (Note that 0 is a color: black.  To not
    specify override_color at all, pass in -1 (or ~0, which is the same
    thing.)
 */
void
paste_ppm (struct ppm *into, int to_x, int to_y,
           struct ppm *from, int from_x, int from_y, int w, int h,
           unsigned char fg, unsigned char bg)
{
  int i, j;

  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++)
      {
        unsigned char pixel;

        get_pixel (from, from_x + i, from_y + j, &pixel);
        put_pixel (into,   to_x + i,   to_y + j,  (pixel != 0) ? fg : bg);
      }
}

/*
 * Draws a PPM image direct to VRAM.  This produces a black-and-white image.
 */
void draw_ppm(struct ppm *ppm, unsigned short *vram, 
              int px, int py, int width)
{
  int y;

  for (y = py; y < py + ppm->height; y++) {
    int x = 0;
    u16 *target = vram + (y * width + px) / 2;
    unsigned char *bitmap = ppm->bitmap + ((y - py) * ppm->width);
    
    if (px & 1) {
      *target = (*target & 0x00FF) | (bitmap[0] << 8);
      target++;
      x++;
    }

    for (; (x < ppm->width) && ((x + px) < width); x += 2) {
      if (x == ppm->width - 1) {
        *target = (*target & 0xFF00) | bitmap[x];
        break;
      } else {
        *target++ = (bitmap[x + 1] << 8) | bitmap[x];
      }
    }
  }
}

