void draw_text(struct font *fnt, 
	       const char *str,
	       int x, int y,
	       u16 *dest);
void draw_color_text(struct font *fnt, 
		      const char *str,
		      int x, int y,
		      u16 *dest,
		      int fg, int bg);
void draw_color_text_raw(struct font *fnt, 
			 char *str,
			 int x, int y,
			 u16 *dest,
			 int fg, int bg);
void draw_char(int x, int y, char c, u16b *dest, struct font *font);
void draw_color_char(int x, int y, char c, int fg, int bg, u16b *dest,
		     struct font *font);

void draw_color_char_raw(int x, int y, char c, int fg, int bg, u16b *dest,
			 struct font *font);
