
// BK Keyboard keycodes
enum KEY_enum {
		/* low keycodes */
		KEY_KT				= 003,

		KEY_LEFT			= 010,		/* (Left Arrow) */
		KEY_ENTER			= 012,		/* ббнд */
		KEY_CLEAR			= 014,		/* яап */
		KEY_SETTAB		= 015,		/* сяр.рюа */
		KEY_RUS				= 016,		/* пся */
		KEY_LAT				= 017,		/* кюр */
		KEY_CLRTAB		= 020,		/* яап.рюа */
		KEY_HOME			= 022,		/* (Home) */
		KEY_VS				= 023,		/* бя */
		KEY_HT				= 024,		/* цр */
		KEY_DEL				= 026,		/* ядб */
		KEY_INS				= 027,		/* пюгдб */
		KEY_ERASE			= 030,		/* <= */
		KEY_RIGHT			= 031,		/* (Right Arrow) */
		KEY_UP				= 032,		/* (Up Arrow) */
		KEY_DOWN			= 033,		/* (Down Arrow) */
		KEY_UPLT			= 034,		/* (Up-Left Arrow) */
		KEY_UPRT			= 035,		/* (Up-Right Arrow) */
		KEY_DNRT			= 036,		/* (Down-Right Arrow) */
		KEY_DNLT			= 037,		/* (Down-Left Arrow) */

		/* high keycodes */
		KEY_REPEAT		= 0201,		/* онбр */
		KEY_ICTL			= 0202,		/* хмд яс */
		KEY_BLCK			= 0204,		/* акнй пед */
		KEY_TAB				= 0211,		/* рюа */
		KEY_EXTMEM		= 0214,		/* по */
		KEY_STEP			= 0220,		/* ьюц */
		KEY_COLOR0		= 0221,		/* жб. 0 */
		KEY_COLOR1		= 0222,		/* жб. 1 */
		KEY_COLOR2		= 0223,		/* жб. 2 */
		KEY_COLOR3		= 0224,		/* жб. 3 */
		KEY_GRAF			= 0225,		/* цпют */
		KEY_ZAP				= 0226,		/* гюо */
		KEY_STIR			= 0227,		/* ярхп */
		KEY_RED				= 0230,		/* пед */
		KEY_DELEND		= 0231,		/* Delete to end */
		KEY_CURSOR		= 0232,		/* йспянп */
		KEY_VMODE			= 0233,		/* 32/64 */
		KEY_INVS			= 0234,		/* хмб. я */
		KEY_INVE			= 0235,		/* хмб. щ */
		KEY_SETIND		= 0236,		/* сяр. хмд */
		KEY_UNDER			= 0237,		/* ондв. */

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

