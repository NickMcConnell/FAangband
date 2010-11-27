/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2 -*-
   font-bdf.c --- simple, self-contained code for manipulating BDF fonts.

   Copyright © 2001, 2002 Jamie Zawinski <jwz@jwz.org>

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  No representations are made about the suitability of this
   software for any purpose.  It is provided "as is" without express or 
   implied warranty.
 */

#include <main-nds.h>
#include <nds/font-bdf.h>
#include <nds/ppm-lite.h>
#include <nds/kbd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))

#define TEXT_PALETTE_NAME "text.pal"

typedef enum {
  STATE_SEARCHING,
  STATE_HAVE_ESCAPE,
  STATE_HAVE_CSI
} ansi_state;

/* for parsing hex numbers fast */
static const char hex[128] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
                              16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
                              16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
                               0, 1, 2, 3, 4, 5, 6, 7, 8, 9,16,16,16,16,16,16,
                              16,10,11,12,13,14,15,16,16,16,16,16,16,16,16,16,
                              16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
                              16,10,11,12,13,14,15,16,16,16,16,16,16,16,16,16,
                              16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16};

// MJS: what should I do for non-fixed width?
void bdf_dims (struct font *font, int *minwd, int *avgwd,
               int *maxwd, int *ht) {
  int i, min=999, max=0, avg=0;
  for (i = 0; i < countof(font->chars); i++) {
    avg += font->chars[i].width;
    min = MIN(min, font->chars[i].width);
    max = MAX(max, font->chars[i].width);
  }
  *minwd = min;
  *avgwd = (10 * avg / countof(font->chars) + 5) / 10; // Hokey!
  *maxwd = max;
  *ht    = font->height;
}

struct font *
read_bdf (const char *file)
{
  int stdin_p = !strcmp(file, "-");
  FILE *in;
  char buf [1024];
  struct font *font = (struct font *) calloc (1, sizeof(*font));
  int line = 1;
  int current_char = -1;
  int current_char_height = -1;
  int current_char_descent = -1;

  int overall_bitmap_height = -1;
  int overall_bitmap_descent = -1;

  if (stdin_p)
    in = stdin;
  else
    {
      in = fopen (file, "r");

      if (!in)
        {
          iprintf("Unable to open %s\n", file);
          return NULL;
        }
    }

  if (!fgets (buf, sizeof(buf)-1, in))
    {
      iprintf ("%s: %d: premature EOF\n",
               file, line);
      return NULL;
    }

  if (!!strncmp (buf, "STARTFONT 2.", 12))
    {
      iprintf ("%s: %d: not a BDF 2 file\n",
               file, line);
      return NULL;
    }
  line++;

  while (fgets (buf, sizeof(buf)-1, in))
    {
      char dummy;
      int dummyn;
      if (!strncmp (buf, "FACE_NAME ", 10) && !font->name)
        {
          char *ss;
          font->name = strdup (buf + 10);
          for (ss = font->name; *ss; ss++)
            if (*ss == '\"' || *ss == '\r' || *ss == '\n')
              {
                strcpy (ss, ss+1);
                if (ss != font->name) ss--;
              }
        }
      else if (!strncmp (buf, "FONT_ASCENT ", 12))
        {
          if (1 != sscanf (buf+12, "%d %c", &font->ascent, &dummy))
            goto FAIL;
        }
      else if (!strncmp (buf, "FONT_DESCENT ", 12))
        {
          if (1 != sscanf (buf+12, "%d %c", &font->descent, &dummy))
            goto FAIL;
        }
      else if (!strncmp (buf, "FONTBOUNDINGBOX ", 16))
        {
          int w, h, x, y;
          if (4 != sscanf (buf+16, "%d %d %d %d %c", &w, &h, &x, &y, &dummy))
            goto FAIL;
          overall_bitmap_height  = h;
          overall_bitmap_descent = y;
        }
      else if (!strncmp (buf, "ENCODING ", 9))
        {
          if (1 != sscanf (buf+9, "%d %c", &current_char, &dummy) &&
              current_char <= 255 &&
              current_char >= -1)
            goto FAIL;
        }
      else if (!strncmp (buf, "STARTCHAR", 9) ||
               !strncmp (buf, "ENDCHAR", 7))
        {
          current_char = -1;
        }
      else if (!strncmp (buf, "DWIDTH ", 7))
        {
          int w;
          if (2 != sscanf (buf+7, "%d %d %c", &w, &dummyn, &dummy))
            goto FAIL;
          if (current_char != -1)
            font->chars[current_char].width = w;
        }
      else if (!strncmp (buf, "BBX ", 4))
        {
          int w, h, x, y;
          struct ppm *ppm;
          if (4 != sscanf (buf+4, "%d %d %d %d %c", &w, &h, &x, &y, &dummy))
            goto FAIL;

          /* Make all ppm data be the same height, so that when scaling,
             all characters experience proportional roundoff artifacts.
           */
          current_char_height  = h;
          current_char_descent = y;
          h = overall_bitmap_height;
          y = overall_bitmap_descent;

          if (h > font->height) {
            font->height = h;
          }

          if (current_char != -1)
            {
              font->chars[current_char].lbearing = x;
              font->chars[current_char].descent = y;
              ppm = (struct ppm *) calloc (1, sizeof(struct ppm));
              ppm->type = 4;
              ppm->width = ((w + 7) / 8) * 8;
              ppm->height = h;

              ppm->bitmap = (unsigned char *)
                calloc (1, (ppm->width * ppm->height));
              if (!ppm->bitmap)
                {
                  iprintf ("%s: out of memory (%d x %d)\n",
                           file, ppm->width, ppm->height);
                  return NULL;
                }

              font->chars[current_char].ppm = ppm;
            }
        }
      else if (!strncmp (buf, "BITMAP ", 4))
        {
          int y, yoff;

          if (font->chars[current_char].width == 0)
            {
              iprintf ("%s: %d: zero-width char ('%c') with bits?\n",
                       file, line, current_char);
              return NULL;
            }

          yoff = ((overall_bitmap_height - current_char_height) +
                  + (overall_bitmap_descent - current_char_descent));


          if (yoff < 0 ||
              yoff + current_char_height - 1 >= overall_bitmap_height)
            {
              iprintf ("%s: %d: char %d bbox is not contained in font bbox\n",
                       file, line, current_char);
              return NULL;
            }

          for (y = 0; y < current_char_height; y++)
            {
              if (!fgets (buf, sizeof(buf)-1, in))
                {
                  iprintf ("%s: %d: premature EOF\n",
                           file, line);
                  return NULL;
                }

              if (current_char > 0 && current_char <= 255)
                {
                  int yy = (y + yoff);
                  struct ppm *ppm = font->chars[current_char].ppm;
                  unsigned char *o = (ppm->bitmap + (yy * ppm->width));
                  char *s;

                  if (yy < 0 || yy >= ppm->height) abort();

                  for (s = buf; *s; s++)
                    {
                      int h = hex[(int) *s];
                      int i;
                      if (h != 16)
                        for (i = 3; i >= 0; i--)
                          {
                            /* ink bits are 0 (black) and opaque (255).
                               non-ink bits are 0 (black) and clear (0).
                             */
                            *o = ((h & (1<<i)) ? 255 : 0);
                            o++;
                          }
                    }
                }
                line++;
            }
        }
      else if (!strncmp (buf, "FONT ", 5) ||
               !strncmp (buf, "SIZE ", 5) ||
               !strncmp (buf, "COMMENT", 7) ||
               !strncmp (buf, "STARTPROPERTIES ", 7) ||
               !strncmp (buf, "FOUNDRY ", 8) ||
               !strncmp (buf, "FAMILY_NAME ", 12) ||
               !strncmp (buf, "WEIGHT_NAME ", 12) ||
               !strncmp (buf, "SLANT ", 6) ||
               !strncmp (buf, "SETWIDTH_NAME ", 14) ||
               !strncmp (buf, "ADD_STYLE_NAME ", 15) ||
               !strncmp (buf, "PIXEL_SIZE ", 11) ||
               !strncmp (buf, "POINT_SIZE ", 11) ||
               !strncmp (buf, "RESOLUTION_X ", 13) ||
               !strncmp (buf, "RESOLUTION_Y ", 13) ||
               !strncmp (buf, "SPACING ", 8) ||
               !strncmp (buf, "AVERAGE_WIDTH ", 14) ||
               !strncmp (buf, "CHARSET_REGISTRY ", 17) ||
               !strncmp (buf, "CHARSET_ENCODING ", 17) ||
               !strncmp (buf, "FONTNAME_REGISTRY ", 18) ||
               !strncmp (buf, "CAP_HEIGHT ", 11) ||
               !strncmp (buf, "X_HEIGHT ", 9) ||
               !strncmp (buf, "COPYRIGHT ", 10) ||
               !strncmp (buf, "NOTICE ", 7) ||
               !strncmp (buf, "_DEC_", 5) ||
               !strncmp (buf, "MULE_", 5) ||
               !strncmp (buf, "DEFAULT_CHAR ", 13) ||
               !strncmp (buf, "RELATIVE_SETWIDTH ", 18) ||
               !strncmp (buf, "RELATIVE_WEIGHT ", 16) ||
               !strncmp (buf, "CHARSET_COLLECTIONS ", 20) ||
               !strncmp (buf, "FULL_NAME ", 10) ||
               !strncmp (buf, "FONT_NAME ", 10) ||
               !strncmp (buf, "ENDPROPERTIES", 13) ||
               !strncmp (buf, "CHARS ", 5) ||
               !strncmp (buf, "SWIDTH ", 7) ||
               !strncmp (buf, "DWIDTH ", 7) ||
               !strncmp (buf, "RESOLUTION ", 11) ||
               !strncmp (buf, "UNDERLINE_POSITION ", 19) ||
               !strncmp (buf, "UNDERLINE_THICKNESS ", 20) ||
               !strncmp (buf, "_XMBDFED_INFO ", 14) ||
               !strncmp (buf, "WEIGHT ", 7) ||
               !strncmp (buf, "QUAD_WIDTH ", 11) ||
               !strncmp (buf, "ENDFONT", 7))
        {
          /* ignore */
        }
      else if (buf[0] == '\n')
        {
          /* Empty line, ignore */
        }
      else
        {
        FAIL:
          iprintf ("%s: %d: unparsable line: '%s'\n",
                   file, line, buf);
          return NULL;
        }

      line++;
    }

  if (!font->name)
    font->name = strdup ("<unknown>");

  if (!stdin_p)
    fclose (in);

  font->monochrome_p = 1;

  return font;
}


struct font *
copy_font (struct font *font)
{
  int i;
  struct font *f2 = (struct font *) malloc (sizeof(*font));
  memcpy (f2, font, sizeof(*font));
  font = f2;
  font->name = strdup (font->name);
  for (i = 0; i < countof(font->chars); i++)
    if (font->chars[i].ppm)
      font->chars[i].ppm = copy_ppm (font->chars[i].ppm);
  return font;
}


void
free_font (struct font *font)
{
  int i;
  free (font->name);
  for (i = 0; i < countof(font->chars); i++)
    if (font->chars[i].ppm)
      free_ppm (font->chars[i].ppm);
  memset (font, 0xDE, sizeof(*font));
  free (font);
}

void 
text_dims(struct font *fnt, char *str, int *width, int *height)
{
  // MJS: modified to understand multi-line strings
  // (for splatting the motd at startup)
  ansi_state state = STATE_SEARCHING;
  int ht = fnt->height;
  int wd = 0, maxwd = 0;
  
  for (; *str != '\0'; str++) {
    switch (state) {
    case STATE_SEARCHING:
      if (*str == '\e') {
        state = STATE_HAVE_ESCAPE;
      } else {
        if (*str == '\n') {
          ht += fnt->height;
          maxwd = MAX(wd,maxwd);
          wd = 0;
         }
        else
          wd += fnt->chars[(int) *str].width;
      }

      break;

    case STATE_HAVE_ESCAPE:
      if (*str == '[') {
        state = STATE_HAVE_CSI;
      } else {
        state = STATE_SEARCHING;

        wd += fnt->chars[(int) *str].width;
      }

      break;

    case STATE_HAVE_CSI:
      if (((*str >= '0') && (*str <= '9')) || (*str == ' ') || (*str == ';')) {
        break;
      } else {
        state = STATE_SEARCHING;
      }

      break;
    }
  }

  wd = MAX(maxwd, wd);

  if (height != NULL) *height = ht;
  if (width != NULL)  *width = wd;
}


int
bdf_draw_char (struct font *font, const unsigned char c,
           struct ppm *into, int x, int y,
           unsigned char fg, unsigned char bg,
           int uline)
{
  int w = font->chars[(int) c].width;
  struct ppm *from = font->chars[(int) c].ppm;
  int i;

  if (from)
    {
      x += font->chars[(int) c].lbearing;
      y -= from->height + font->chars[(int) c].descent;
      y += font->ascent;

      paste_ppm (into, x, y,
                 from, 0, 0, from->width, from->height, fg, bg);
    }

  if (uline) {
    for (i = 0; i < w; i++) {
      put_pixel(into, x + i, y + font->height - 1, fg);
    }
  }

  return w;
}


/* draw a string with origin XY.
   Alignment 0 means center on the Y axis;
   -1 means flushright; 1 means flushleft.
   Alpha ranges from 0-255.
   Newlines are allowed; tabs are not handled specially.
 */
void
draw_string (struct font *font, char *string,
             struct ppm *into, int x, int y,
             int fg, int bg)
{
  int ox = x;
  int w;
  int override = ((fg >= 0) || (bg >= 0));
  int attr = 0;

  ansi_state state = STATE_SEARCHING;
  int code_params[2];
  int param_count = 0;
  char *start_ptr = NULL;

  if (fg < 0) {
    fg = CLR_WHITE;
  }

  //MJS: this code is causing me grief
#if 0
  if (bg < 0) {
    bg = CLR_BLACK;
  } else if (bg >= BRIGHT) {
    bg -= BRIGHT;
  }

  if (fg > CLR_GRAY) {
    attr |= TEXT_ATTR_BOLD;
    fg -= 8;
  }
#endif
  if (bg < 0) bg = CLR_BLACK;

 LINE:
  x = ox;
  w = 0;

  /* back up over the lbearing of the first character on the line */
  x -= 2 * font->chars[(int) *string].lbearing;

  while (*string)
    {
      if (*string == '\n')
        {
          y += font->ascent + font->descent;
          string++;
          goto LINE;
        }
      else
        {
          int w;

          switch (state) {
            case STATE_SEARCHING:
              if (*string == '\e') {
                state = STATE_HAVE_ESCAPE;
              } else {
                int tfg = fg + ((attr & TEXT_ATTR_BOLD) ? 8 : 0);
                int rfg, rbg;

                rfg = (attr & TEXT_ATTR_INVERSE) ? bg : tfg;
                rbg = (attr & TEXT_ATTR_INVERSE) ? tfg : bg;

                w = bdf_draw_char (font, *string, into, x, y, rfg, rbg, attr & TEXT_ATTR_ULINE);
                x += w;
              }

              break;

            case STATE_HAVE_ESCAPE:
              if (*string == '[') {
                state = STATE_HAVE_CSI;

                param_count = 0;
                memset(code_params, 0, sizeof(code_params));

                start_ptr = string + 1;
              } else {
                int tfg = fg + ((attr & TEXT_ATTR_BOLD) ? 8 : 0);
                int rfg, rbg;

                rfg = (attr & TEXT_ATTR_INVERSE) ? bg : tfg;
                rbg = (attr & TEXT_ATTR_INVERSE) ? tfg : bg;

                state = STATE_SEARCHING;

                w = bdf_draw_char (font, *string, into, x, y, rfg, rbg, attr & TEXT_ATTR_ULINE);
                x += w;
              }

              break;

            case STATE_HAVE_CSI:
              if (((*string >= '0') && (*string <= '9')) || (*string == ' ')) {
                break;
              } else if (*string == ';') {
                sscanf(start_ptr, "%d", &(code_params[param_count++]));
                start_ptr = string + 1;

                break;
              } else if (start_ptr != string) {
                sscanf(start_ptr, "%d", &(code_params[param_count++]));
              }

              switch (*string) {
                /* Select Graphic Rendition */
                case 'm':
                  state = STATE_SEARCHING;

                  if (override) {
                    break;
                  }

                  if (param_count == 0) {
                    fg = CLR_GRAY;
                    bg = CLR_BLACK;
                    attr = 0;
                  } else if (code_params[0] == 1) {
                    attr |= TEXT_ATTR_BOLD;
                  } else if (code_params[0] == 2) {
                    attr &= ~TEXT_ATTR_BOLD;
                  } else if (code_params[0] == 4) {
                    attr |= TEXT_ATTR_ULINE;
                  } else if (code_params[0] == 7) {
                    attr |= TEXT_ATTR_INVERSE;
                  } else if (code_params[0] == 24) {
                    attr &= ~TEXT_ATTR_ULINE;
                  } else if (code_params[0] == 27) {
                    attr &= ~TEXT_ATTR_INVERSE;
                  } else if ((code_params[0] >= 30) && (code_params[0] <= 39)) {
                    int c = code_params[0] - 30;

                    if (c == '9') {
                      fg = CLR_GRAY;
                      attr |= TEXT_ATTR_BOLD;
                    } else {
                      fg = c;
                    }
                  } else if ((code_params[0] >= 40) && (code_params[0] <= 49)) {
                    bg = code_params[0] - 40;
                  }

                  break;

                default:
                  state = STATE_SEARCHING;
                  break;
              }

              break;
          }
        }
      string++;
    }
}
