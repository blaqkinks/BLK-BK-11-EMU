
// BK Keyboard keycodes
enum KEY_enum {
		/* low keycodes */
		KEY_KT				= 003,

		KEY_LEFT			= 010,		/* (Left Arrow) */
		KEY_ENTER			= 012,		/* ���� */
		KEY_CLEAR			= 014,		/* ��� */
		KEY_SETTAB		= 015,		/* ���.��� */
		KEY_RUS				= 016,		/* ��� */
		KEY_LAT				= 017,		/* ��� */
		KEY_CLRTAB		= 020,		/* ���.��� */
		KEY_HOME			= 022,		/* (Home) */
		KEY_VS				= 023,		/* �� */
		KEY_HT				= 024,		/* �� */
		KEY_DEL				= 026,		/* ��� */
		KEY_INS				= 027,		/* ����� */
		KEY_ERASE			= 030,		/* <= */
		KEY_RIGHT			= 031,		/* (Right Arrow) */
		KEY_UP				= 032,		/* (Up Arrow) */
		KEY_DOWN			= 033,		/* (Down Arrow) */
		KEY_UPLT			= 034,		/* (Up-Left Arrow) */
		KEY_UPRT			= 035,		/* (Up-Right Arrow) */
		KEY_DNRT			= 036,		/* (Down-Right Arrow) */
		KEY_DNLT			= 037,		/* (Down-Left Arrow) */

		/* high keycodes */
		KEY_REPEAT		= 0201,		/* ���� */
		KEY_ICTL			= 0202,		/* ��� �� */
		KEY_BLCK			= 0204,		/* ���� ��� */
		KEY_TAB				= 0211,		/* ��� */
		KEY_EXTMEM		= 0214,		/* �� */
		KEY_STEP			= 0220,		/* ��� */
		KEY_COLOR0		= 0221,		/* ��. 0 */
		KEY_COLOR1		= 0222,		/* ��. 1 */
		KEY_COLOR2		= 0223,		/* ��. 2 */
		KEY_COLOR3		= 0224,		/* ��. 3 */
		KEY_GRAF			= 0225,		/* ���� */
		KEY_ZAP				= 0226,		/* ��� */
		KEY_STIR			= 0227,		/* ���� */
		KEY_RED				= 0230,		/* ��� */
		KEY_DELEND		= 0231,		/* Delete to end */
		KEY_CURSOR		= 0232,		/* ������ */
		KEY_VMODE			= 0233,		/* 32/64 */
		KEY_INVS			= 0234,		/* ���. � */
		KEY_INVE			= 0235,		/* ���. � */
		KEY_SETIND		= 0236,		/* ���. ��� */
		KEY_UNDER			= 0237,		/* ����. */

		/* magic numbers for special keys */
		KEY_SWITCH		= 0373,		/* video mode switch */
		KEY_AR2				= 0374,		/* AP2 */
		KEY_RESET			= 0375,		/* "reset buton", only with AR2 */
		KEY_STOP			= 0376,		/* STOP */
		KEY_NOTHING		= 0377,
		};

// Keyboard shift state
enum KS_enum {
		KS_CAPS,
		KS_SHIFT,
		KS_CTRL
		};

