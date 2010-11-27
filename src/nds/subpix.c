
#include <main-nds.h>
#include "nds/ds_errfont.h" /* contains the subpixel font data in bgr */

extern u16b color_data[];

u16* subfont_rgb_bin = (u16*)BG_BMP_RAM(ANG_SUBPIX_BASE);// (0x06018400);
u16* subfont_bgr_bin = (u16*)BG_BMP_RAM(ANG_SUBPIX_BASE+1);// (0x0601C400);
u16* top_font_bin;
u16* btm_font_bin;

void nds_init_font_subpix() {
  // the font is now compiled in as ds_subfont for error reporting purposes
  // ds_subfont contains the bgr version
  //subfont_bgr_bin = &ds_subfont[0];
  u16b i,t,t2;
  for (i=0;i<8*3*256;i++) {
    t = ds_subfont[i];
    t2 = t & 0x8000;
    t2 |= (t & 0x001f)<<10;
    t2 |= (t & 0x03e0);
    t2 |= (t & 0x7c00)>>10;
    subfont_bgr_bin[i] = t;
    subfont_rgb_bin[i] = t2;
  }
  top_font_bin = subfont_bgr_bin;
  btm_font_bin = subfont_rgb_bin;
}

void swap_font(bool bottom) {
  if (!bottom) {
    if (top_font_bin == subfont_rgb_bin) top_font_bin = subfont_bgr_bin;
    else top_font_bin = subfont_rgb_bin;
  } else {
    if (btm_font_bin == subfont_rgb_bin) btm_font_bin = subfont_bgr_bin;
    else btm_font_bin = subfont_rgb_bin;
  }
}

void draw_char_subpix(byte x, byte y, char c) {
  u32b vram_offset = y * 8 * 256 + x * 3, tile_offset = c * 24;
  u16b* fb = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN_OVER);
  const u16b* chardata = top_font_bin;
  byte xx, yy;
  if (x > 84) return;
  for (yy = 0; yy < 8; yy++)
    for (xx = 0; xx < 3; xx++) 
      fb[yy * 256 + xx + vram_offset] 
	= chardata[yy * 3 + xx + tile_offset] | BIT(15);
}

void draw_text_subpix(byte x, byte y, char *s) {
  int i;
  for (i=0; i<strlen(s); i++)
    draw_char_subpix(x+i,y,s[i]);
}

void draw_color_char_subpix(byte x, byte y, char c, byte clr) {
  u32b vram_offset = (y & 0x1F) * 8 * 256 + x * 3, tile_offset = c * 24;
  u16b* fb = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN_OVER);
  const u16b* chardata = top_font_bin;
  byte xx, yy;
  u16b val;
  u16b fgc = color_data[clr & 0xF];
  if (x > 84) return;
  for (yy = 0; yy < 8; yy++) {
    for (xx = 0;xx < 3; xx++) {
      val = (chardata[yy * 3 + xx + tile_offset]);
      fb[yy * 256 + xx + vram_offset] = (val  & fgc) | BIT(15);
    }
  }
}

void draw_color_text_subpix(byte x, byte y, char *s, byte clr) {
  int i;
  for (i=0; i<strlen(s); i++)
    draw_color_char_subpix(x+i,y,s[i],clr);
}
