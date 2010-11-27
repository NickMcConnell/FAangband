#include <main-nds.h>
#include <nds/overlay.h>
#include <nds/text.h>
#include <nds/kbd.h>
#include <misc.h>

#define BUTTON_WD 24
#define BUTTON_HT 15

#define IN_EDIT_MODE() (transient_visible==9)
int transient_visible = 0;
int show_keys = 0;
int info_butt = -1; // # of info panel
int edit_selected = -1;
int entry_butt = -1; // HACK


void redraw_all_buttons ();
void save_user_buttons ();
void draw_button (int n);

#define RECTANGLE 1
#define CIRCLE    2

#define RADIUS(b) ((b.wd+b.ht)/2/2)

static struct {
  char name[256]; // name to display (keep long for dialogs)
  char keys[16];  // keys to send when pressed
  int  active;    // whether in use or not
  int  transient; // hidden until activated
  int  type;      // button type
  int  shape;     // rect/circle
  int  backup;    // flag indicating buttons is backed up for restoring later
  int  x0, y0, wd, ht; // position and size
  int  bg;        // palette number for background
} butt[MAX_OVERLAY_BUTTONS];

// stacking order: redraws are done by buttons stacking[0] first etc
// when a button is selected it's index is moved to stacking[0] and the
// whole array is shuffled down....
int stacking[MAX_OVERLAY_BUTTONS];

void init_overlay() {
  int i,n;

  // clear active flags and set default stacking order
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    butt[n].active = butt[n].backup = 0;
    stacking[n] = n;
  }

  // set up palettes
  BG_PALETTE[START_PAL]   = OVERLAY_WHITE;
  BG_PALETTE[START_PAL+1] = OVERLAY_KEYS;
  BG_PALETTE[START_PAL+2] = OVERLAY_GAME_FG;
  BG_PALETTE[START_PAL+3] = OVERLAY_USER_FG;
  BG_PALETTE[START_PAL+4] = OVERLAY_INFO_FG;
  BG_PALETTE[START_PAL+5] = OVERLAY_EDIT_FG;
  
  for (i=0; i<MAX_OVERLAY_BUTTONS; i++)
    BG_PALETTE[START_BG_PAL+i] = OVERLAY_GAME_BG_DIM; // override later
}

void set_transient(int vis) {
  static int last_vis = -1;
  transient_visible = vis;
  int n;
  // any buttons that would be drawn/erased with this change?
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    if (!butt[n].active) continue;
    if (butt[n].transient > last_vis &&
	butt[n].transient <= vis) break;
    if (butt[n].transient <= last_vis &&
	butt[n].transient > vis) break;
  }
  last_vis = vis;
  if (n<MAX_OVERLAY_BUTTONS) // yes!
    redraw_all_buttons();
}

// TODO: replace with ascii_to_text/text_to_ascii
char * trans(char *k) {
  static char s[20];
  char *t=s;
  while (*k) {
    char a = *k++;
    if (a=='\\') {
      if (*k) {
	char b = *k++;
	*t++ = ( b=='r' ? '\r'   :
		 b=='e' ? '\x1b' :  b );
      }
    } else
      *t++ = a;
  }
  *t = 0;
  printf("trans = %s\n", s);
  return s;
}

char * find_key_button(char *name) {
  int n;
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    if (!butt[n].active) continue;
    if (butt[n].type!=B_KEY) continue;
    if (strcmp(butt[n].name,name)==0)
      return trans(butt[n].keys);
  }
  return NULL;
}

static int find_button (int x, int y) {
  int s, n;
  // NB: scan in reverse, since draw order to forward
  // to ensure we get the top-most button at x,y
  for (s=MAX_OVERLAY_BUTTONS-1; s>=0; s--) {
    n = stacking[s];
    if (!butt[n].active) continue;
    if (butt[n].transient > transient_visible) continue;
    if (IN_EDIT_MODE() && !(butt[n].type & B_MAPPABLE)) continue;
    if (butt[n].shape == RECTANGLE && butt[n].type != B_INFO &&
	x >= butt[n].x0 && x < butt[n].x0 + butt[n].wd &&
	y >= butt[n].y0 && y < butt[n].y0 + butt[n].ht)
      return n;
    if (butt[n].shape == CIRCLE &&
	(x - butt[n].x0)*(x - butt[n].x0) +
	(y - butt[n].y0)*(y - butt[n].y0) < RADIUS(butt[n])*RADIUS(butt[n]))
      return n;
  }
  return -1;
}

static void reset_colors() {
  int i;
  for (i=0; i<MAX_OVERLAY_BUTTONS; i++)
    BG_PALETTE[START_BG_PAL+i] = B_COLOR_DIM(butt[i]);
}

static int held_last = -1; // button that is being held
static int held_pos = 0;
static int held_left = 0;
static int held_right = 0;

char * check_overlay_release(int x, int y) {
  int n = find_button(x,y);
  reset_colors();

  if (n >= 0 && 
      butt[n].type & B_MAPPABLE && 
      held_left > 1 && held_right > 1) {
    bool ret;
    char tmp1[20], tmp2[20];
    BG_PALETTE[butt[n].bg] = OVERLAY_USER_BG_HIGH;
    held_left = held_right = 0;
    strncpy(tmp1, butt[n].keys, sizeof(tmp1));
    msg_print("[use '\\e' for escape, '\\r' for return]");
    ret = get_string("New keys: ", tmp1, sizeof(tmp1));
    if (!ret) {
      reset_colors();
      msg_print("Key redefinition aborted.");
      return "";
    }
    if (butt[n].type==B_USER){
      strncpy(tmp2, butt[n].name, sizeof(tmp2));
      ret = get_string("New name: ", tmp2, sizeof(tmp2));
      if (!ret) {
	reset_colors();
	msg_print("Key redefinition aborted.");
	return "";
      }
    }
    reset_colors();
    strncpy(butt[n].keys, tmp1, sizeof(butt[n].keys));
    if (butt[n].type==B_USER)
      strncpy(butt[n].name, tmp2, sizeof(butt[n].name));
    save_user_buttons();
    redraw_all_buttons();
    held_last = -1;
    held_left = held_right = 0;
    msg_print("Key re-defined. (saved)");
    return "";
  }

  held_last = -1;
  held_left = held_right = 0;

  if (n<0) return NULL;
  return trans(butt[n].keys);
}

char * check_overlay_hold(int x, int y) {
  int n = find_button(x,y);
  if (n>=0) {
    // draw this button
    draw_button(n);
    // update stacking (TODO: will this remain consistent?)
    if (n != stacking[MAX_OVERLAY_BUTTONS-1]) {
      int i;
      for (i=0; i<MAX_OVERLAY_BUTTONS-1; i++)
	if (stacking[i] == n) break;
      for (; i<MAX_OVERLAY_BUTTONS; i++)
	stacking[i] = stacking[i+1];
      stacking[MAX_OVERLAY_BUTTONS-1] = n;
    }
  }
  reset_colors();
  if (n<0) {
    held_last = n; // HACK
    return NULL;
  }

  if (n != held_last) {
    held_last = n;
    held_pos  = 0; // middle, we'll get side next frame
    held_left = held_right = 0;
  }
  else {
    if (x < butt[n].x0 + butt[n].wd/2 - 1) {
      if (held_pos != -1)
	++held_left;
      held_pos = -1;
    } else if (x > butt[n].x0 + butt[n].wd/2 + 1) {
      if (held_pos != 1)
	++held_right;
      held_pos = 1;
    } else
      held_pos = 0;
  }

  if (butt[n].type != B_INFO)
    BG_PALETTE[butt[n].bg] = B_COLOR_HIGH(butt[n]);

  return butt[n].keys;
}

void erase_button (int n) {
  int i,j;
  if (!butt[n].active) return;
  if (butt[n].transient > transient_visible) return;
  int x0 = butt[n].x0;
  int y0 = butt[n].y0;
  int wd = butt[n].wd;
  int ht = butt[n].ht;
  u16b *fb = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN_OVER);
  u16 clr  = IN_EDIT_MODE() ? 1 : 0;
  if (butt[n].shape == RECTANGLE) {
    // --- rectangular button ---
    for (i=0; i<wd; i++)
      for (j=0; j<ht; j++)
	draw_pix8(fb, x0+i, y0+j, clr);
  }
}


void draw_button (int n) {
  
  if (!butt[n].active) return;
  if (butt[n].transient > transient_visible) return;

  int i, j;
  int x0 = butt[n].x0;
  int y0 = butt[n].y0;
  int wd = butt[n].wd;
  int ht = butt[n].ht;
  int type = butt[n].type;
  int bg = butt[n].bg;
  int fg = ( type&(B_GAME|B_DIR) ? START_PAL+2 :
	     type&(B_USER|B_KEY) ? START_PAL+3 :
	     type==B_INFO        ? START_PAL+4 :
	     type==B_EDIT        ? START_PAL+5 : START_PAL );

  // draw the button!
  u16b *fb = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN_OVER);

  int w, h;
  int sk = show_keys && butt[n].type & B_MAPPABLE;
  char *s = sk ? butt[n].keys : butt[n].name;
  int fgt = sk ? START_PAL+1 : START_PAL;

  text_dims(normal_font, s, &w, &h);

  if (wd == -1)
    butt[n].wd = wd = w + 8; // HACK: padding
  if (ht == -1)
    butt[n].ht = ht = h + 6; // HACK: padding

  int dx, dy;
  if (butt[n].type & B_KEY) {
    // --- circular button ---

    butt[n].shape = CIRCLE;

    int r = RADIUS(butt[n]); // why can't I set a constant here????
    int f = 1-r;
    int ddfx = 1;
    int ddfy = -2*r;
    int x = 0;
    int y = r;

    draw_line8(fb,x0-y,y0+x, x0+y,y0+x,bg);

    draw_pix8(fb,x0+x,y0+y,fg);
    draw_pix8(fb,x0-x,y0+y,fg);
    draw_pix8(fb,x0+y,y0-x,fg);
    draw_pix8(fb,x0-y,y0-x,fg);

    int yy = -1, xx = -1;
    while (x < y) {
      if (f >= 0) {
	y--;
	ddfy += 2;
	f += ddfy;
      }
      x++;
      ddfx += 2;
      f += ddfx;

      if (y != yy) {
	draw_line8(fb,x0-x,y0+y, x0+x,y0+y,bg);
	draw_line8(fb,x0-x,y0-y, x0+x,y0-y,bg);
	yy = y;
      }
      if (x != xx) {
	draw_line8(fb,x0-y,y0+x, x0+y,y0+x,bg);
	draw_line8(fb,x0-y,y0-x, x0+y,y0-x,bg);
	xx = x;
      }

      draw_pix8(fb,x0+x,y0+y,fg);
      draw_pix8(fb,x0-x,y0+y,fg);
      draw_pix8(fb,x0+x,y0-y,fg);
      draw_pix8(fb,x0-x,y0-y,fg);
      draw_pix8(fb,x0+y,y0+x,fg);
      draw_pix8(fb,x0-y,y0+x,fg);
      draw_pix8(fb,x0+y,y0-x,fg);
      draw_pix8(fb,x0-y,y0-x,fg);
    }

    dx = -w/2;
    dy = -h/2;
  } else {
    
    butt[n].shape = RECTANGLE;

    // --- rectangular button ---
    for (i=0; i<wd; i++)
      for (j=0; j<ht; j++) {
	int c = ((i==0)||(i==wd-1)||(j==0)||(j==ht-1)) ? fg : bg;
	draw_pix8(fb, x0+i, y0+j, c);
      }
    dx = (wd - w)/2;
    dy = (ht - h)/2;
  }

  draw_color_text_raw(normal_font, s, x0+dx,y0+dy, fb, fgt, bg);
}

void redraw_all_buttons () {
  int i;
  u16b *fb = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN_OVER);
  u16 clr2 = IN_EDIT_MODE() ? (1<<8) + 1 : 0;
  for (i=0; i<128*192; i++) fb[i] = clr2;
  for (i=0; i<MAX_OVERLAY_BUTTONS; i++) {
    int n = stacking[i];
    draw_button(n);
  }
}

void overlay_toggle_showkeys() {
  show_keys ^= 1;
  redraw_all_buttons();
}

int add_overlay_button(char *name, char *keys,
		       int x0, int y0, int wd, int ht,
		       int type, int transient) {
  int n;

  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    if (butt[n].active) continue;
    if (butt[n].backup) continue;
    break;
  }
 
  if (n == MAX_OVERLAY_BUTTONS)
    return -1; // ignore this request!

  int bg = START_BG_PAL + n;

  butt[n].active    = 1;
  butt[n].type      = type;
  butt[n].transient = transient;
  butt[n].backup    = 0;

  BG_PALETTE[bg] = B_COLOR_DIM(butt[n]);

  strncpy(butt[n].name, name, sizeof(butt[n].name));
  strncpy(butt[n].keys, keys, sizeof(butt[n].keys));

  int l = strlen(name);
  if (butt[n].name[l-1]=='\n')
    butt[n].name[l-1] = 0;

  butt[n].x0 = x0; butt[n].wd = wd;
  butt[n].y0 = y0; butt[n].ht = ht;
    
  butt[n].bg = bg;

  draw_button(n);

  return n; // button #
}

// vars for buttons

int user_button_y = 0;
int user_button_x = 256 - BUTTON_WD;

int game_button_y = 192 - BUTTON_HT;
int game_button_x = 256 - BUTTON_WD;


// these routines are for user-defined buttons

void add_user_button(char *name, char *keys) {
  add_overlay_button(name, keys, 
		     user_button_x, user_button_y,
		     BUTTON_WD, BUTTON_HT, B_USER, 0);
  user_button_y += (BUTTON_HT-1);
  if (0) if (user_button_y + BUTTON_HT >= game_button_y) {
    user_button_y = 0;
    user_button_x -= BUTTON_WD;
  }
}

// these are the routines for Nick's automatic button spawning/killing

int have_button (char *k) {
  int n;
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++)
    if (butt[n].active && strcmp(butt[n].keys,k)==0)
      return 1;
  return 0;
}

int find_game_button (unsigned char keypress) {
  int n;
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++)
    if (butt[n].active && butt[n].type==B_GAME && butt[n].keys[0]==keypress)
      return n;
  return -1;
}

int add_button_nds(char *name, unsigned char keypress) {
  char keys[2];
  keys[0] = keypress;
  keys[1] = 0;
  //if (have_button(keys))
  //  return 0;
  if (find_game_button(keypress)>=0)
    return 0;
  add_overlay_button(name, keys, 
		     game_button_x, game_button_y,
		     BUTTON_WD, BUTTON_HT, B_GAME, 0);
  game_button_y -= (BUTTON_HT-1);
  if (game_button_y <= 4*BUTTON_HT) { // HACK
    game_button_y = 192 - BUTTON_HT;
    game_button_x -= BUTTON_WD;
  }
  return 0;
}

void redo_game_buttons () {
  int n;

  // HACK: kill all game buttons, then add again to repack
  game_button_y = 192 - BUTTON_HT;
  game_button_x = 256 - BUTTON_WD;

  // chasing our tail here, but we know there's room
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    unless (butt[n].active) continue;
    unless (butt[n].type==B_GAME) continue;
    butt[n].active = 0;
    add_button_nds(butt[n].name, butt[n].keys[0]);
  }

  redraw_all_buttons(); // HACK
}


int kill_button_nds(unsigned char keypress) {
  int n = find_game_button(keypress);
  if (n < 0)
    return 0;
  butt[n].active = 0; // kill this button!
  redo_game_buttons(); // repack
  return 0;
}

void kill_all_buttons_nds() {
  int n;
  // chasing our tail here, but we know there's room
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    unless (butt[n].active) continue;
    unless (butt[n].type==B_GAME) continue;
    butt[n].active = 0;
  }
  game_button_y = 192 - BUTTON_HT;
  game_button_x = 256 - BUTTON_WD;
  redraw_all_buttons(); // HACK
}

void backup_buttons_nds () {
  int n;
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++)
    if (butt[n].active && butt[n].type==B_GAME && butt[n].backup)
      return; // already backed up
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++)
    if (butt[n].active && butt[n].type==B_GAME)
      butt[n].backup = 1;
}

void restore_buttons_nds () {
  int n;
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++)
    if (butt[n].type==B_GAME) {
      butt[n].active = butt[n].backup;
      butt[n].backup = 0;
    }
  redo_game_buttons();
}

void add_user_buttons () {
  FILE *fp;
  char name[20], keys[20], stype[20], geom[20];
  int i = 0;
  fp = fopen("nds/buttons.def","r");
  if (fp != NULL) {
    while (fgets(stype,20,fp) && fgets(geom,20,fp) && 
	   fgets(name,20,fp) && fgets(keys,20,fp)) {
      int x0, y0, wd, ht, trans, n;
      int type = 0;
      if (strncmp(stype,"KEY",3)==0) type = B_KEY;
      if (strncmp(stype,"USER",4)==0) type = B_USER;
      if (strncmp(stype,"DIR",3)==0) type = B_DIR;
      if (type == 0) continue;
      if (sscanf(geom,"%d %d %d %d %d",&x0,&y0,&wd,&ht,&trans)!=5) {
	printf("error parsing geom: %s\n", geom);
	continue;
      }
      if (name[strlen(name)-1] == '\n') name[strlen(name)-1] = 0;
      if (keys[strlen(keys)-1] == '\n') keys[strlen(keys)-1] = 0;
      for (n=0; n<MAX_OVERLAY_BUTTONS; n++)
	if (butt[n].active && butt[n].x0==x0 && butt[n].y0==y0) break;
      if (n<MAX_OVERLAY_BUTTONS) continue;
      add_overlay_button(name,keys,x0,y0,wd,ht,type,trans);
      i++;
    }
    fclose(fp);
    if (i==0) {
      printf("Error parsing BUTTONS.DEF\n");
      printf("using defaults...\n");
    }
  }

  int trans = 3*3; // transient level of key buttons/help

  if (i==0) {
    int wd, ht, x0, y0;
    add_user_button("*","*");
    add_user_button("t","t");
    add_user_button("s","s");
    add_user_button(",",",");

    wd = ht = 22;
    int dx = 17, dy = 17;

    x0 = 128 - 2*dx - dx/2;
    y0 = 192 - 2*dy;
    add_overlay_button("X","\\e",  x0+0*dx,y0-1*dy,wd,ht,B_KEY,trans);
    add_overlay_button("Y","y",    x0-1*dx,y0+0*dy,wd,ht,B_KEY,trans);
    add_overlay_button("A","\\r",  x0+1*dx,y0+0*dy,wd,ht,B_KEY,trans);
    add_overlay_button("B","Mars", x0+0*dx,y0+1*dy,wd,ht,B_KEY,trans);
    
    x0 = 128 + 2*dx - dx/2;
    y0 = 192 - 2*dy;
    add_overlay_button("L+X","",   x0+0*dx,y0-1*dy,wd,ht,B_KEY,trans);
    add_overlay_button("L+Y","",   x0-1*dx,y0+0*dy,wd,ht,B_KEY,trans);
    add_overlay_button("L+A","",   x0+1*dx,y0+0*dy,wd,ht,B_KEY,trans);
    add_overlay_button("L+B","f0.",x0+0*dx,y0+1*dy,wd,ht,B_KEY,trans);
  }
    
  { // draw dirpad
    int ht = 18, wd = 18;
    int x0 = 256 - 5 - 4*wd;
    int y0 = 0;
    
    add_overlay_button("1","1",x0+0*wd-0,y0+2*ht-2,wd,ht,B_DIR,1);
    add_overlay_button("2","2",x0+1*wd-1,y0+2*ht-2,wd,ht,B_DIR,1);
    add_overlay_button("3","3",x0+2*wd-2,y0+2*ht-2,wd,ht,B_DIR,1);
    
    add_overlay_button("4","4",x0+0*wd-0,y0+1*ht-1,wd,ht,B_DIR,1);
    add_overlay_button("5","5",x0+1*wd-1,y0+1*ht-1,wd,ht,B_DIR,1);
    add_overlay_button("6","6",x0+2*wd-2,y0+1*ht-1,wd,ht,B_DIR,1);
    
    add_overlay_button("7","7",x0+0*wd-0,y0+0*ht-0,wd,ht,B_DIR,1);
    add_overlay_button("8","8",x0+1*wd-1,y0+0*ht-0,wd,ht,B_DIR,1);
    add_overlay_button("9","9",x0+2*wd-2,y0+0*ht-0,wd,ht,B_DIR,1);
  }
  { // draw help
    char *help =
      "Touch a blue button to select\n"
      "it, and touch again to edit.\n"
      "R to switch between names/keys.\n"
      "All changes will be saved.\n";
    info_butt = add_overlay_button(help,"",8,8,-1,-1,B_INFO,trans);
    if (0) { // editing buttons
      int i=0, wd = 32, ht=BUTTON_HT-1;
      add_overlay_button("add","",      0,i++*(ht-1),wd,ht,B_EDIT,trans);
      add_overlay_button("del","",      0,i++*(ht-1),wd,ht,B_EDIT,trans);
    }
  }
  if (0) {
    int wd=60, ht=16;
    int x0=16, y0=16, dx=3, dy=3;
    add_overlay_button("light bolt","1",x0+0*dx,y0+0*dy,wd,ht,B_DIR,1);
    add_overlay_button("dark bolt","2",x0+1*dx,y0+1*dy,wd,ht,B_DIR,1);
    add_overlay_button("nox fumes","3",x0+2*dx,y0+2*dy,wd,ht,B_DIR,1);
    add_overlay_button("shadow shft","4",x0+3*dx,y0+3*dy,wd,ht,B_DIR,1);
    add_overlay_button("identify","5",x0+4*dx,y0+4*dy,wd,ht,B_DIR,1);
  }
}

void save_user_buttons () {
  int n;
  FILE *fp;
  fp = fopen("nds/buttons.def","w");
  for (n=0; n<MAX_OVERLAY_BUTTONS; n++) {
    unless (butt[n].type & B_MAPPABLE) continue;
    fprintf(fp, "%s\n", butt[n].type==B_KEY ? "KEY" : "USER");
    fprintf(fp, "%d %d %d %d %d\n", butt[n].x0, butt[n].y0, 
	    butt[n].wd, butt[n].ht, butt[n].transient);
    fprintf(fp, "%s\n", butt[n].name);
    fprintf(fp, "%s\n", butt[n].keys);
  }
  fclose(fp);
}

// HACK: no real error checking (length of buf etc)
int nds_get_string (char *prompt, 
		    char *def,
		    char *obuf) {
  int la, lb=0;
  char a[50]; // before cursor
  char b[50] = ""; // after cursor
  char buf[512], pbuf[512];
  int keys = 0; // keys entered
  int i;
  strcpy(a,def);
  la = strlen(a);
  sprintf(buf, "%s%s%c%s", prompt, a, 1, b);
  entry_butt = add_overlay_button(buf, "", 48, 90, -1, -1, B_INFO, 0);
  strcpy(pbuf, buf);
  
  int accept = 0;
  
  while (1) {

    sprintf(buf, "%s%s%c%s", prompt, a, 1, b);
    if (strcmp(buf,pbuf)!=0) {
      erase_button(entry_butt);
      butt[entry_butt].wd = butt[entry_butt].ht = -1;
      strcpy(butt[entry_butt].name, buf);
      draw_button(entry_butt);
      strcpy(pbuf, buf);
    }

    swiWaitForVBlank();
    scanKeys();

    // handle cursor movement first
    u32b kd = keysDown() | keysDownRepeat();
    if (kd & KEY_RIGHT) {
      ++keys;
      if (lb == 0) continue;
      a[la++] = b[0]; a[la] = 0;
      for (i=0; i<lb; i++) b[i] = b[i+1]; 
      b[--lb] = 0;
      continue;
    }

    if (kd & KEY_LEFT) {
      ++keys;
      if (la == 0) continue;
      for (i=lb; i>0; i--) b[i]=b[i-1];
      b[++lb] = 0;
      --la;
      b[0] = a[la];
      a[la] = 0;
      continue;
    }

    // now virtual kbd scan
    byte k = kbd_vblank(); // ret = 0x0a, esc = 0x1b, bs  = 0x08

    if (k==0) 
      continue; // next vblank/scankeys
    ++keys;

    if (k == 0x0a) { // RETURN
      accept = 1;
      break;
    }

    if (k == 0x1b)  // ESCAPE
      break;
      
    if (k == 0x08) { // BACKSPACE
      if (la == 0) 
	continue;
      if (keys == 1) { 
	// first key entered is backspace: delete default value (if any)
	a[la=0] = 0;
      } else 
	a[--la] = 0;
      continue;
    }

    if (k >= ' ' && k <= '~') {
      a[la++] = k; a[la] = 0;
      continue;
    }
  }

  butt[entry_butt].active = 0;
  redraw_all_buttons();

  if (!accept)
    return 0;
  
  strcpy(obuf, a);
  strcat(obuf, b);
  return 1;
}

int edit_mode_click(int x, int y) {

  int n = find_button(x,y);

  if (n < 0) {
    edit_selected = -1;
    reset_colors();
    return -1;
  }

  if (n == edit_selected) {
    int save = 0;
    char new_keys[128], new_name[128];
    window_swap(); // show kbd
    if (nds_get_string("Enter keys: ",butt[n].keys,new_keys)) {
      if (butt[n].type != B_USER) {
	// just keys accepted
	strncpy(butt[n].keys, new_keys, sizeof(butt[n].keys));
	save = 1;
      } else if (nds_get_string("Enter name: ",butt[n].name,new_name)) {
	// both have to be accepted
	strncpy(butt[n].keys, new_keys, sizeof(butt[n].keys));
	strncpy(butt[n].name, new_name, sizeof(butt[n].name));
	save = 1;
      }
    }
    window_swap(); // unshow kbd
    if (save) {
      save_user_buttons(); 
      redraw_all_buttons();
    }
  }

  edit_selected = n;
  reset_colors();
  if (butt[n].type != B_INFO)
    BG_PALETTE[butt[n].bg] = B_COLOR_HIGH(butt[n]);
  draw_button(n);
  return n;
}

int edit_drag(int x, int y, int x0, int y0) {
  int n = edit_selected;
  if (held_last >= 0)
    n = edit_selected = held_last;
  if (n < 0) 
    return -1;
  butt[n].x0 = x;
  butt[n].y0 = y;
  redraw_all_buttons();
  swiWaitForVBlank();
  return n;
}
