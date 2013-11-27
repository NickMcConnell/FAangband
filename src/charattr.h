#ifndef INCLUDED_CHARATTR_H
#define INCLUDED_CHARATTR_H

/* 
 * Hard line maxima for char_attr lines 
 */
#define MAX_C_A_LEN 80
#define MAX_C_A_SML 48

typedef struct char_attr char_attr;

/**
 * Char, attr pair for dumping info and maybe other things
 */
struct char_attr {
    wchar_t pchar;
    byte pattr;
};

/**
 * An array of MAX_C_A_LEN char_attr's
 */
typedef char_attr char_attr_line[MAX_C_A_LEN];

extern void x_fprintf(ang_file *f, int encoding, const char *fmt, ...);
extern void dump_put_str(byte attr, const char *str, int col);
extern void dump_lnum(char *header, s32b num, int col, byte color);
extern void dump_num(char *header, int num, int col, byte color);
extern void dump_deci(char *header, int num, int deci, int col, byte color);
extern void dump_line_file(char_attr *this_line);
extern void dump_line_screen(char_attr *this_line);
extern void dump_line_mem(char_attr *this_line);
extern void dump_line(char_attr *this_line);
extern int handle_item(void);
extern int make_metric(int wgt);


#endif /* !INCLUDED_CHARATTR_H */
