/* history.h - player history tracking */

#ifndef HISTORY_H
#define HISTORY_H

/* History message types */
#define HISTORY_PLAYER_BIRTH     0x0001	/* Player was born */
#define HISTORY_ARTIFACT_UNKNOWN 0x0002	/* Player found unknown artifact */
#define HISTORY_ARTIFACT_KNOWN   0x0004	/* Player has IDed an artifact */
#define HISTORY_ARTIFACT_LOST    0x0008	/* Player had an artifact and lost it */
#define HISTORY_PLAYER_DEATH     0x0010	/* Player has been slain */
#define HISTORY_SLAY_UNIQUE      0x0020	/* Player has slain a unique monster */
#define HISTORY_USER_INPUT       0x0040	/* User-added note */
#define HISTORY_MOVE_HOUSE       0x0080	/* PLayer moved house */
#define HISTORY_GAIN_LEVEL       0x0100	/* Player gained a level */
#define HISTORY_GAIN_SPECIALTY   0x0200	/* Player gained a specialty */

typedef struct history_info history_info;
struct history_info
{
    u16b type;		/**< Kind of history item */
    s16b place;		/**< Place when this item was recorded */
    s16b clev;		/**< Character level when this item was recorded */
    byte a_idx;		/**< Artifact this item relates to */
    s32b turn;		/**< Turn this item was recorded on */
    char event[80];	/**< The text of the item */
};


void history_clear(void);
size_t history_get_num(void);
bool history_add_full(u16b type, byte a_idx, s16b dlev, s16b clev, s32b turn, const char *text);
bool history_add(const char *event, u16b type, byte a_idx);
bool history_add_artifact(byte a_idx, bool known, bool found);
void history_unmask_unknown(void);
bool history_lose_artifact(byte a_idx);
void history_display(void);
void dump_history(char_attr_line *line, int *curr_line, bool *dead);
bool history_is_artifact_known(byte a_idx);

extern history_info *history_list;

#endif /* !HISTORY_H */
