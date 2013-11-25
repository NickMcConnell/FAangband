/* birth.h */

#ifndef BIRTH_H
#define BIRTH_H

extern void player_birth(bool quickstart_allowed);
extern void player_generate(struct player *p, player_sex *s,
                            struct player_race *r, player_class *c);

char *find_roman_suffix_start(const char *buf);

#endif /* !BIRTH_H */
