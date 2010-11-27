#include <main-nds.h>
#include <nds/font-bdf.h>
#include <nds/ppm-lite.h>

#define USE_PPM 1

void draw_color_text(struct font *fnt, 
		      char *str,
		      int x, int y,
		      u16 *dest,
		      int fg, int bg) {
  int w, h;
  if (fg < 0) fg = 1;
  if (bg < 0) bg = 0;

  fg &= 15;
  bg &= 15;

  fg += TEXT_COLOUR_BASE;
  bg += TEXT_COLOUR_BASE;
  struct ppm *img;
  text_dims(fnt, str, &w, &h);
  img = alloc_ppm(w, h);
  clear_ppm(img, bg);
  draw_string(fnt, str, img, 0, 0, fg, bg);
  draw_ppm(img, dest, x, y, 256);
  free_ppm(img);
}

void draw_color_text_raw(struct font *fnt, 
			 char *str,
			 int x, int y,
			 u16 *dest,
			 int fg, int bg) {
  int w, h;
  if (fg < 0) fg = 1;
  if (bg < 0) bg = 0;
  struct ppm *img;
  text_dims(fnt, str, &w, &h);
  img = alloc_ppm(w, h);
  clear_ppm(img, bg);
  draw_string(fnt, str, img, 0, 0, fg, bg);
  draw_ppm(img, dest, x, y, 256);
  free_ppm(img);
}

void draw_text(struct font *fnt, 
	       char *str,
	       int x, int y,
	       u16 *dest) {
  draw_color_text(fnt, str, x, y, dest, 1, 0);
}

void draw_char(int x, int y, char c, u16b *dest, struct font *font) {
  char str[2] = { 0, 0 };
  str[0] = c;
  draw_text(font, str, x, y, dest);
}

void draw_color_char(int x, int y, char c, int fg, int bg, u16b *dest,
		     struct font *font) {
  char str[2] = { 0, 0 };
  str[0] = c;
  draw_color_text(font, str, x, y, dest, fg, bg);
}

void draw_color_char_raw(int x, int y, char c, int fg, int bg, u16b *dest,
		     struct font *font) {
  char str[2] = { 0, 0 };
  str[0] = c;
  int w, h;
  struct ppm *img;
  text_dims(font, str, &w, &h);
  img = alloc_ppm(w, h);
  bdf_draw_char (font, str[0],
		 img, 0, 0,
		 fg, bg, 0);
  draw_ppm(img, dest, x, y, 256);
  free_ppm(img);
}
