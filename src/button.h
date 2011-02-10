/* button.h - button interface */

#ifndef BUTTON_H
#define BUTTON_H

/*** Constants ***/

/**
 * Maximum number of mouse buttons
 */
#define MAX_MOUSE_BUTTONS  20

/**
 * Maximum length of a mouse button label
 */
#define MAX_MOUSE_LABEL 10


/*** Types ***/

/**
 * Mouse button structure
 */
typedef struct
{
	char label[MAX_MOUSE_LABEL]; /*!< Label on the button */
	int left;                    /*!< Column containing the left edge of the button */
	int right;                   /*!< Column containing the right edge of the button */
	unsigned char key;           /*!< Keypress corresponding to the button */
} button_mouse;



int button_add_text(const char *label, unsigned char keypress);
int button_add(const char *label, unsigned char keypress);
void button_backup_all(void);
void button_restore(void);
int button_kill_text(unsigned char keypress);
int button_kill(unsigned char keypress);
void button_kill_all(void);
void button_init(button_add_f add, button_kill_f kill);
char button_get_key(int x, int y);
size_t button_print(int row, int col);

#endif /* !BUTTON_H */
