
#include <main-nds.h>
#include <nds/kbd.h>
#include <string.h>

#define ISWHITESPACE(c) ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
#define BUFSZ 1024

const nds_kbd_key row0[] = {
  {16,(u16)'`'}, {16,(u16)'1'}, {16,(u16)'2'}, {16,(u16)'3'}, {16,(u16)'4'}, 
  {16,(u16)'5'}, {16,(u16)'6'}, {16,(u16)'7'}, {16,(u16)'8'}, {16,(u16)'9'}, 
  {16,(u16)'0'}, {16,(u16)'-'}, {16,(u16)'='}, {32,(u16)'\b'}, {0,0}};
const nds_kbd_key row1[] = {
  {24,(u16)'\t'}, {16,(u16)'q'}, {16,(u16)'w'}, {16,(u16)'e'}, {16,(u16)'r'}, 
  {16,(u16)'t'}, {16,(u16)'y'}, {16,(u16)'u'}, {16,(u16)'i'}, {16,(u16)'o'}, 
  {16,(u16)'p'}, {16,(u16)'['}, {16,(u16)']'}, {24,(u16)'\\'}, {0,0}};
const nds_kbd_key row2[] = {
  {32,K_CAPS}, {16,(u16)'a'}, {16,(u16)'s'}, {16,(u16)'d'}, {16,(u16)'f'}, 
  {16,(u16)'g'}, {16,(u16)'h'}, {16,(u16)'j'}, {16,(u16)'k'}, {16,(u16)'l'}, 
  {16,(u16)';'}, {16,(u16)'\''}, {32,(u16)'\n'}, {0,0}};
const nds_kbd_key row3[] = {
  {40,K_SHIFT}, {16,(u16)'z'}, {16,(u16)'x'}, {16,(u16)'c'}, {16,(u16)'v'}, 
  {16,(u16)'b'}, {16,(u16)'n'}, {16,(u16)'m'}, {16,(u16)','}, {16,(u16)'.'}, 
  {16,(u16)'/'}, {40,K_SHIFT}, {0,0}};
const nds_kbd_key row4[] = {
  {32,K_CTRL}, {24,K_ALT}, {128,(u16)' '}, {24,K_ALT}, {32,K_CTRL}, {0,0}};
char shifts[] = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:\'\",<.>/?";
const nds_kbd_key *kbdrows[] = {row0, row1, row2, row3, row4};

static bool shift = false, ctrl = false, alt = false, caps = false;

/*
 * Copy a block of memory 2 bytes at a time.  This is needed for
 * things like VRAM.
 *
 * TODO: Test if we can do this in 32-bit copies, instead.
 */
void memcpy16(void *dest, void *src, int count)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  int i;

  for (i = 0; i < count / 2; i++) {
    d[i] = s[i];
  }

  /* If it's an odd count, copy the last byte, if needed.
     This relies on the control variable retaining it's value after
     the loop is complete.  But I can never remember if that's well
     defined behaviour... */
     
  if (count & 0x01) {
    d[i] = (d[i] & 0x00FF) | (s[i] << 8);
  }
}

/*
 * Returns the string with the trailing whitespace stripped off, and the
 * pointer advanced to point past the leading whitespace.
 */
char *nds_strip(char *str, int front, int back)
{
  char *end = str + strlen(str) - 1;

  while (front && *str && ISWHITESPACE(*str)) {
    str++;
  }

  while (back && (end >= str) && ISWHITESPACE(*end)) {
    *end = '\0';
    end--;
  }

  return str;
}

int xxnds_load_file(char *fname, void *dest)
{
  u8 *d = (u8 *)dest; /* Makes our pointer arithmetic cleaner... */

  FILE *file = fopen(fname, "r");
  u8 buf[1024];
  int pos, read;

  if (file == NULL) {
    return -1;
  }

  pos = 0;

  while ((read = fread(buf, 1, sizeof(buf), file)) > 0) {
    memcpy16(d + pos, buf, read);
    pos += read;
  }

  fclose(file);

  return 0;
}

int nds_load_palette(char *fname, nds_palette palette) {
  FILE *pfile;
  int palidx = 0;

  if ((pfile = fopen(fname, "r")) == NULL) {
    iprintf("Unable to open '%s'\n", fname);

    return -1;
  }

  while (1) {
    char buf[BUFSZ];
    char *tmp;
    int r, g, b;

    if (fgets(buf, BUFSZ, pfile) == NULL) {
      break;
    }

    tmp = nds_strip(buf, 1, 1);

    if ((*tmp == '#') || (*tmp == '\0')) {
      continue;
    }

    if (sscanf(tmp, "%2x%2x%2x", &r, &g, &b) < 3) {
      iprintf("Malformed line: '%s'\n", tmp);

      return -1;
    }

    palette[palidx++] = RGB15(r >> 3, g >> 3, b >> 3);
  }

  return palidx;
}

bool nds_load_kbd() {
  u16 palette[4];
  int ret;

  ret = xxnds_load_file("nds/kbd.bin", (void *)BG_TILE_RAM_SUB(ANG_KBD_TILE_BASE));
  if (ret < 0) return FALSE;

  ret = xxnds_load_file("nds/kbd.map", (void *)BG_MAP_RAM_SUB(ANG_KBD_MAP_BASE));
  if (ret < 0) return FALSE;

  ret = nds_load_palette("nds/kbd.pal", palette);
  if (ret != 4) {
    palette[0] = RGB15(0,0,0);
    palette[1] = RGB15(0,0,16);
    palette[2] = RGB15(31,31,31);
    palette[3] = RGB15(31,31,0);
  }

  BG_PALETTE_SUB[0] = RGB15(0, 0, 0);   /* Regular background */
  BG_PALETTE_SUB[16] = RGB15(0, 0, 0);
  BG_PALETTE_SUB[1] = palette[0];       /* Key background, normal   */
  BG_PALETTE_SUB[17] = palette[1];      /* Key background, selected */
  BG_PALETTE_SUB[2] = palette[2];       /* Key text, normal */
  BG_PALETTE_SUB[18] = palette[3];      /* Key text, selected */

  return TRUE;
}

void kbd_init() {
  u16b i;
  for (i = 0; i < 16; i++) {
    BG_PALETTE_SUB[i+16] = BG_PALETTE_SUB[i] ^ 0x7FFF;
  }
}

u16b kbd_mod_code(u16 ret) {
  if (ret & K_MODIFIER) return ret;
  if (caps && !shift) 
    if (ret >= 'a' && ret <= 'z') ret -= 0x20;
  if (shift) {
    char* temp;
    if (!caps && ret >= 'a' && ret <= 'z') ret -= 0x20;
    if ((temp = strchr(shifts,ret)) != NULL) ret = *(temp + 1);
  }
  if (alt)
    ret |= 0x80;
  if (ctrl/* && ret >= 'a' && ret < 'a'+32*/) 
    ret = ret & 0x1f;
  return ret;
}

void kbd_set_color_from_pos(u16b r, u16b k, byte color) 
{
  u16b ii, xx = 0, jj;
  u16b *map[] = { 
    (u16b*)(BG_MAP_RAM(ANG_KBD_MAP_BASE+0)+3*32*2), 
    (u16b*)(BG_MAP_RAM(ANG_KBD_MAP_BASE+1)+3*32*2), 
    (u16b*)(BG_MAP_RAM(ANG_KBD_MAP_BASE+2)+3*32*2),
    (u16b*)(BG_MAP_RAM(ANG_KBD_MAP_BASE+3)+3*32*2) 
  };
  for (ii = 0; ii < k; ii++) 
    xx += kbdrows[r][ii].width >> 3;
  for (ii = 0; ii < (kbdrows[r][k].width>>3); ii++) 
    for (jj = 0; jj < 4; jj++) {
      map[jj][(10 + r * 2) * 32 + ii + xx + 1] 
	= (map[jj][(10+r*2)*32+ii+xx+1] & 0x0FFF) | (color << 12);
      map[jj][(10 + r * 2 + 1) * 32 + ii + xx + 1] 
	= (map[jj][(10+r*2+1)*32+ii+xx+1] & 0x0FFF) | (color << 12);
    }
}

void kbd_set_color_from_code(u16b code, byte color) {
  u16b r,k;
  for (r = 0; r < 5; r++)
    for (k = 0; kbdrows[r][k].width != 0; k++)
      if (kbd_mod_code(kbdrows[r][k].code) == code)  
	// do not break!! there may be >1 key with this code (modifier keys)
	kbd_set_color_from_pos(r,k,color);
}

void kbd_set_map() {
  SUB_BG0_CR = BG_TILE_BASE(ANG_KBD_TILE_BASE) |
    BG_MAP_BASE(ANG_KBD_MAP_BASE + (caps | (shift<<1))) |
    BG_PRIORITY(0) | BG_16_COLOR;
  
}

u16b kbd_xy2key(byte x, byte y) {
  if (x >= 104 && x < 152 && y >=24 && y < 72) {
    // on arrow-pad
    byte kx = (x-104)/16, ky = (y-24)/16;
    return (kx + (2 - ky) * 3 + 1 + '0')/* | (shift ? K_SHIFTED_MOVE : 0)*/;
  }
  if (y >=80 && y < 96) {
    if (x >= 8 && x < 24) return '\033';
    if (x >= 40 && x < 248) {	// F-key
      x -= 40;
      y = x/72;	// which section
      x -= y*72;	// offset in section
	// section*4 + offset/16 + 1
      return (x < 64) ? K_F(y*4+(x>>4)+1) : 0;
    }
  }
  s16b ox = x - 8, oy = y-104;
  if (ox < 0 || ox >= 240) return 0;
  if (oy < 0 || oy >= 80) return 0;
  u16b row = oy / 16;
  int i;
  for (i = 0; ox > 0; ox -= kbdrows[row][i++].width);
  u16b ret = kbdrows[row][i-1].code;
  return kbd_mod_code(ret);
}

void kbd_dotoggle(bool *flag, int how) {
  switch (how) {
  case 0: *flag = false; return;
  case 1: *flag = true; return;
  default:
  case -1: *flag = !*flag; return;
  }
}

// which: K_SHIFT, K_CTRL, K_ALT, K_MODIFIER=all keys
// how: -1 = toggle, 0 = off, 1 = on
void kbd_togglemod(int which, int how) 
{
  //boolean old_shift = shift, old_ctrl = ctrl, old_alt = alt, old_caps = caps;
  switch (which) {
  case K_CTRL: kbd_dotoggle(&ctrl,how); break;
  case K_SHIFT: kbd_dotoggle(&shift,how); break;
  case K_ALT: kbd_dotoggle(&alt,how); break;
  case K_CAPS: kbd_dotoggle(&caps,how); break;
  case K_MODIFIER:
    kbd_dotoggle(&ctrl,how);
    kbd_dotoggle(&shift,how);
    kbd_dotoggle(&alt,how);
    // NOT caps!!  This is called to un-set shift, ctrl, and alt after
    // a key is pressed.  Unsetting caps here would cause it to be the
    // same as shift.
    break;
  }
  
  /* if (old_shift != shift) */
  kbd_set_color_from_code(K_SHIFT,shift);
  
  /* if (old_ctrl != ctrl) */
  kbd_set_color_from_code(K_CTRL,ctrl);
  
  /* if (old_alt != alt) */
  kbd_set_color_from_code(K_ALT,alt);
  
  /* if (old_caps != caps) */
  kbd_set_color_from_code(K_CAPS,caps);
  
  kbd_set_map();
}


// clear this to prevent alt-b, f5, and f6 from having their special effects
// it's cleared during getlin, yn_function, etc
byte process_special_keystrokes = 1;

// run this every frame
// returns a key code if one has been typed, else returns 0
// assumes scankeys() was already called this frame (in real vblank handler)
byte kbd_vblank() {
  // frames the stylus has been held down for
  static u16b touched = 0;
  // coordinates from each frame, the median is used to get the keycode
  static s16b xarr[3],yarr[3];
  // the keycode of the last key pressed, so it can be un-hilited
  static u16b last_code;
  // the keycode of the currently pressed key, is usu. returned
  // u16b keycode;
  
  // if screen is being touched...
  if (keysHeld() & KEY_TOUCH) {
    if (touched < 3) {	// if counter < 3...
      touched++;				// add to counter
      xarr[touched-1] = IPC->touchXpx;	// add this to the array for
      yarr[touched-1] = IPC->touchYpx;	// finding the median
    }
  } 
  else           // not being touched
    touched = 0; // so reset the counter for next time
  
  // if the stylus was released
  if (keysUp() & KEY_TOUCH) {
    // if last_code is set and it wasn't a modifier
    if (last_code && !(last_code & K_MODIFIER)) {
      // clear the hiliting on this key
      kbd_set_color_from_code(last_code,0);
      // and also clear all modifiers (except caps)   
      kbd_togglemod(K_MODIFIER, 0);
    }
    last_code = 0;
  }
  
  // if the screen has been touched for 3 frames...
  if (touched == 3) {
    touched++;	// do not return the keycode again
    // also, not setting to zero prevents the keysHeld() thing
    //  from starting the process over and getting 3 more samples
      
    u16b i, tmp, the_x=0, the_y=0;
      
    // x/yarr now contains 3 values from each of the 3 frames
    // take the median of each array and put into the_x/y
      
    // sort the array
    // bubble sort, ugh
    for (i = 1; i < 3; i++) {
      if (xarr[i] < xarr[i-1]) {
	tmp = xarr[i];
	xarr[i] = xarr[i-1];
	xarr[i-1] = tmp;
      }
      if (yarr[i] < yarr[i-1]) {
	tmp = yarr[i];
	yarr[i] = yarr[i-1];
	yarr[i-1] = tmp;
      }
    }
    
    // get the middle value (median)
    // if it's -1, take the top value
    if (xarr[1] == -1) the_x = xarr[2];
    else the_x = xarr[1];
    if (yarr[1] == -1) the_y = yarr[2];
    else the_y = yarr[1];
      
    // get the keycode that corresponds to this key
    u16b keycode = kbd_xy2key(the_x, the_y);
      
    // if it's not a modifier, hilite it
    if (keycode && !(keycode & K_MODIFIER)) 
      kbd_set_color_from_code(keycode,1);
    // set last_code so it can be un-hilited later
    last_code = keycode;
      
#if 0
    // check for special keystrokes: alt-b, f5, f6
    if (process_special_keystrokes) {
      if (keycode & K_F(0)) {	// its an f-key
	if (keycode == K_F(6)) { // F6: toggle top font
	  swap_font(false);
	  Term_redraw();
	  keycode = last_code = 0;
	}
	kbd_togglemod(K_MODIFIER,0);
      }
    }
#endif
      
    // if it's a modifier, toggle it
    if (keycode & K_MODIFIER) kbd_togglemod(keycode,-1);
    else if ((keycode & 0x7F) != 0) {	// it's an actual keystroke, return it
      return (keycode & 0xFF);
    }
  }
  
  return 0;
}
