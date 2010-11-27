// max overlay buttons
#define MAX_OVERLAY_BUTTONS 48

// button types
#define B_USER 1  // user defined button
#define B_GAME 2  // game automatic button
#define B_KEY  4  // buttons corresponding to NDS keys
#define B_DIR  8  // dirpad key
#define B_INFO 16 // dialog box
#define B_EDIT 32 // editing control key

#define B_MAPPABLE  (B_USER|B_KEY)
#define B_RENAMABLE (B_USER)

// offset for overlay display/highlighting palette entriess
#define START_PAL    150
#define START_BG_PAL 156

#define OVERLAY_WHITE         RGB15(31,31,31)
#define OVERLAY_KEYS          RGB15(31,31,0)
#define OVERLAY_GAME_FG       RGB15(20,10,20)
#define OVERLAY_USER_FG       RGB15(10,10,20)
#define OVERLAY_INFO_FG       RGB15(10,20,10)
#define OVERLAY_EDIT_FG       RGB15(10,20,10)

#define OVERLAY_GAME_BG_DIM   RGB15(15,4,4)
#define OVERLAY_GAME_BG_HIGH  RGB15(31,8,8)

#define OVERLAY_USER_BG_DIM   RGB15(4,4,15)
#define OVERLAY_USER_BG_HIGH  RGB15(8,8,31)

#define OVERLAY_INFO_BG_DIM   RGB15(4,8,4)
#define OVERLAY_INFO_BG_HIGH  RGB15(8,16,8)

#define OVERLAY_EDIT_BG_DIM   RGB15(4,15,4)
#define OVERLAY_EDIT_BG_HIGH  RGB15(8,31,8)

#define B_COLOR_DIM(b)			    \
  ( b.type & (B_USER|B_KEY) ? OVERLAY_USER_BG_DIM :	\
    b.type & B_INFO         ? OVERLAY_INFO_BG_DIM : \
    b.type & B_EDIT         ? OVERLAY_EDIT_BG_DIM : \
                              OVERLAY_GAME_BG_DIM )

#define B_COLOR_HIGH(b)				 \
  (  b.type & (B_USER|B_KEY) ? OVERLAY_USER_BG_HIGH :	 \
     b.type & B_INFO         ? OVERLAY_INFO_BG_HIGH :	 \
     b.type & B_EDIT         ? OVERLAY_EDIT_BG_HIGH :	 \
                               OVERLAY_GAME_BG_HIGH )

extern int transient_visible;

void set_transient(int vis);
void init_overlay();
char * find_key_button(char *name);
char * user_button_keys (int n);
char * check_overlay_release(int x, int y);
char * check_overlay_hold(int x, int y);
int add_overlay_button(char *name, char *keys,
		       int x, int y, int width, int height,
		       int game, int transient);

void add_user_button(char *name, char *keys);
int add_button_nds(char *name, unsigned char keypress);
int kill_button_nds(unsigned char keypress);
void kill_all_buttons_nds(void);
void restore_buttons_nds ();
void backup_buttons_nds ();
void add_user_buttons ();
void overlay_toggle_showkeys();

int edit_mode_click(int x, int y);
int edit_drag(int x, int y, int x0, int y0);
