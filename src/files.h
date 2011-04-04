/* files.h - game file interface */

#ifndef FILES_H
#define FILES_H

extern void html_screenshot(cptr name, int mode);
extern void safe_setuid_drop(void);
extern void safe_setuid_grab(void);
extern s16b tokenize(char *buf, s16b num, char **tokens);
extern errr check_time(void);
extern errr check_time_init(void);
extern void display_player(int mode);
extern void display_player_sml(void);
extern int make_dump(char_attr_line *line, int mode);
extern void display_dump(char_attr_line *line, int top_line, int bottom_line, 
			 int col);
extern errr file_character(cptr name, char_attr_line *line, int last_line);
extern bool show_file(cptr name, cptr what, int line, int mode);
extern void do_cmd_help(void);
extern void process_player_name(bool sf);
extern bool get_name(char *buf, size_t buflen);
extern void display_scores(int from, int to);
extern void save_game(void);
extern void close_game(void);
extern void exit_game_panic(void);
extern errr get_rnd_line(char *file_name, char *output);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);


#endif /* !FILES_H */
