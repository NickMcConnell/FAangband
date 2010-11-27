#include <nds.h>
#include <angband.h>

 //#include "nds/ds_errfont.h" /* contains the subpixel font date in bgr */
 //#include "nds/ds_main.h"

extern u16* subfont_rgb_bin;
extern u16* subfont_bgr_bin;
extern u16* top_font_bin;
extern u16* btm_font_bin;
extern u16* tiles_bin;

void nds_init_font_subpix();
void swap_font(bool bottom);
void draw_char_subpix(byte x, byte y, char c);
void draw_color_char_subpix(byte x, byte y, char c, byte clr);

void draw_text_subpix(byte x, byte y, char *s);
void draw_color_text_subpix(byte x, byte y, char *s, byte clr);
