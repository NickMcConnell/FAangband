#define NDS_BUTTON_FILE		"buttons.dat"

#define NDS_MAPPABLE_MASK	(KEY_A | KEY_B | KEY_X | KEY_Y | KEY_START | KEY_SELECT)
#define NDS_MODIFIER_MASK	(KEY_L | KEY_R)
#define NDS_BUTTON_MASK		(NDS_MAPPABLE_MASK | NDS_MODIFIER_MASK)
#define NDS_NUM_MAPPABLE	6      // A, B, X, Y, Select, Start
#define NDS_NUM_MODIFIER	2      // R, L
#define NDS_CMD_LENGTH		16     // max. 15 keys/button + null terminator

void nds_init_buttons();
void nds_check_buttons(u16b kd, u16b kh);
