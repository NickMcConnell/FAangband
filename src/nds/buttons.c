
#include <main-nds.h>
#include <nds/buttons.h>

#include <unistd.h>

//[mappable]*2^[mods] things to map commands to, [cmd_length] chars per command
byte nds_btn_cmds[NDS_NUM_MAPPABLE << NDS_NUM_MODIFIER][NDS_CMD_LENGTH];

byte btn_defaults[] = 
  {// A    B     X      Y   SEL  ST
    '\r', 'b', '\x1b', 'y', 'i', ' ',  // 
    ' ',  ' ', ' ',    ' ', ' ', ' ',  // L
    ' ',  ' ', ' ',    ' ', ' ', ' ',  // R
    ' ',  ' ', ' ',    ' ', ' ', ' '}; // L+R

const s16 mappables[] = { KEY_A, KEY_B, KEY_X, KEY_Y, KEY_SELECT, KEY_START };
const s16 modifiers[] = { KEY_L, KEY_R };

s16 nds_buttons_to_btnid(u16 kd, u16 kh) {

  if (!(kd & NDS_MAPPABLE_MASK)) return -1;
  u16 i, mods = 0;
  
  for (i=0;i<NDS_NUM_MODIFIER;i++) {
    if (kh & modifiers[i]) mods |= (1 << i);
  }
  for (i=0;i<NDS_NUM_MAPPABLE;i++) {
    if (kd & mappables[i]) return i + NDS_NUM_MAPPABLE * (mods);
  }
  return -1;
}


void nds_init_buttons() {
  u16b i, j;
  for (i = 0; i < (NDS_NUM_MAPPABLE << NDS_NUM_MODIFIER); i++)
    for (j = 0; j < NDS_CMD_LENGTH; j++) 
      nds_btn_cmds[i][j] = 0;

  if (access(NDS_BUTTON_FILE,0444) == -1) {
    /* Set defaults */
    for (i = 0; i < (NDS_NUM_MAPPABLE << NDS_NUM_MODIFIER); i++) 
      nds_btn_cmds[i][0] = btn_defaults[i];
  
    return;
  }
  
  FILE* f = fopen(NDS_BUTTON_FILE, "r");
  fread(&nds_btn_cmds[0], NDS_CMD_LENGTH,
	(NDS_NUM_MAPPABLE << NDS_NUM_MODIFIER), f);
  fclose(f);
}

void nds_check_buttons(u16b kd, u16b kh) 
{
  s16b btn = nds_buttons_to_btnid(kd,kh);
  if (btn == -1) return;
  byte *cmd = &nds_btn_cmds[btn][0];
  while (*cmd != 0) {
    put_key_event(*(cmd++)); // in main_nds
  }
}

