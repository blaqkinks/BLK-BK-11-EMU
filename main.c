/*
 * This file is part of 'pdp', a PDP-11 simulator.
 *
 * For information contact:
 *
 *   Computer Science House
 *   Attn: Eric Edwards
 *   Box 861
 *   25 Andrews Memorial Drive
 *   Rochester, NY 14623
 *
 * Email:  mag@potter.csh.rit.edu
 * FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
 * 
 * Copyright 1994, Eric A. Edwards
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Eric A. Edwards makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * main.c -  Main routine and setup.
 */


#include "defines.h"
#include "scr.h"
#include <SDL/SDL.h>
#include <SDL/SDL_main.h>
#include <libintl.h>
#include <locale.h>
#include <sys/time.h>

#include <stdarg.h>

#define _(String) gettext (String)

/*
 *	Message logging
 */

void logF (flag_t msg_class, char *format, ...) {
if (! msg_class) {
va_list ptr;

va_start (ptr, format);
vfprintf (stderr, _(format), ptr);
va_end (ptr);
}
}	/* logF */

flag_t log_tape;
flag_t log_timer;
flag_t log_tape;
flag_t log_line;
flag_t log_secret;
flag_t log_checkpt;

flag_t log_EMT;
flag_t log_TRAP;

/*
 * Globals.
 */

char * printer_file = 0;
char init_path[BUFSIZ];

/*
 * At start-up, bkmodel == 0, 1, or 2 means BK-0010, 3 means BK-0011M.
 * During simulation, bkmodel == 0 is BK-0010, 1 is BK-0011M.
 */
flag_t bkmodel = 3; /* default BK model */
flag_t terak = 0; /* by default we emulate BK */
flag_t fake_disk = 1; /* true for BK-0011M and bkmodel == 2 */

/* Standard path and ROMs for basic hardware configurations */

char * romdir = "/usr/share/bk"; /* default ROM path */
char * monitor10rom = "MONIT10.ROM";
char * focal10rom = "FOCAL10.ROM";
char * basic10rom = "BASIC10.ROM"; 
char * diskrom = "DISK_327.ROM";
char * bos11rom = "B11M_BOS.ROM";
char * bos11extrom = "B11M_EXT.ROM";
char * basic11arom = "BAS11M_0.ROM";
char * basic11brom = "BAS11M_1.ROM";

char * rompath10 = 0;
char * rompath12 = 0;
char * rompath16 = 0;

int TICK_RATE;	/* cpu clock speed */

char * floppyA = "A.img";
char * floppyB = "B.img";
char * floppyC = "C.img";
char * floppyD = "D.img";

unsigned short last_branch;
/*
 * Command line flags and variables.
 */

flag_t aflag;		/* autoboot flag */
flag_t nflag;		/* audio flag */
flag_t mouseflag;	/* mouse flag */
flag_t covoxflag;	/* covox flag */
flag_t synthflag;	/* AY-3-8910 flag */
flag_t plipflag;	/* PLIP flag */
flag_t fullspeed;	/* do not slow down to real BK speed */
flag_t traceflag;	/* print all instruction addresses */
FILE * tracefile;	/* trace goes here */
flag_t tapeflag;	/* Disable reading from tape */
flag_t turboflag;	/* "Turbo" mode with doubled clock speed */
flag_t hflag;			/* Tape files with header */
double frame_delay;	/* Delay in ticks between video frames */
double half_frame_delay;

void launch (char *file);

// Checkpoint description
typedef struct _CheckP {
	d_word addr;				// location
	char *mesg;					// message to display
	void (* rout)();		// routine to call

	struct _CheckP *next;
	} CheckP;

static CheckP *cp_list = 0;

void install_cp (d_word addr, char *mesg, void (* rout) ()) {
CheckP *cp = malloc (sizeof (CheckP));
if (! cp) return;

cp->addr = addr;
cp->mesg = mesg;
cp->rout = rout;

cp->next = cp_list;
cp_list = cp;
}	// install_cp

int checkpoint(d_word pc) {
    extern int breakpoint;

	CheckP *cp;
	for (cp = cp_list; cp; cp = cp->next)
		if (pc == cp->addr) {
			if (cp->mesg) logF(log_checkpt, cp->mesg, pc);
			(cp->rout) ();

			break;
			}

    return (pc == breakpoint);
}		// checkpoint

// KBscript stuff

flag_t kbscript_on = 0;
FILE *kbscript_in = 0;
char *kbscript_ptr = 0;

void kbscript_file (char *file) {
if (file) {
if ((kbscript_in = fopen (file, "rt"))) kbscript_on = 1;
}
else logF (0, "Can't read %s\n", file);
}		// kbscript_file

void kbscript_buffer (char *buf) {
kbscript_ptr = buf;
kbscript_on = 1;
}	// kbscript_buffer

char kbscript_getch () {
int ch = 0;
if (kbscript_in) {

if ((ch = fgetc (kbscript_in)) == EOF) {
	fclose (kbscript_in);
	kbscript_in = 0;
	kbscript_on = 0;
	return 0;
	}
}

else if (kbscript_ptr) {
	ch = *kbscript_ptr ++;
	if (! ch) { kbscript_on = 0; kbscript_ptr = 0; }

}
return ch;
}		// kbscript_getch

// DPscript stuff
flag_t dpscript_on = 0;
FILE *dpscript_out;

void dpscript_file (char *file) {
if (file) {
if ((dpscript_out = fopen (file, "wt"))) dpscript_on = 1;
}
else logF (0, "Can't write %s\n", file);
}		// dpscript_file

void dpscript_putch (char ch) {
fputc (ch, dpscript_out);
}	// dpscript_putch

void dpscript_putstr (pdp_regs *p, d_word addr, unsigned count, unsigned char sentinel) {
if (! count) count = 0200000;

while (count --) {
	unsigned char ch;
	ll_byte (p, addr ++, &ch);
	if (ch == sentinel) break;

	fputc (ch, dpscript_out);
	}
}	// dpscript_putstr

/*
 * main()
 */

extern unsigned char fake_tape;
void fake_tuneup_sequence();
void fake_array_with_tuneup();
void fake_read_strobe();
void fake_write_file();

void fake_disk_io();
void fake_sector_io();

pdp_regs pdp;		/* internal processor registers */
volatile int stop_it;	/* set when a SIGINT happens during execution */

// In 'cpu.c':

// Initialise CPU
void cpu_init (pdp_regs *p);

// Execute CPU instruction
CPU_RES cpu_instruction ();

// Service interrupt
CPU_RES cpu_service (d_word vector);

	/*
	 * Compute execution statistics and print it.
	 */

unsigned start_ticks, stop_ticks;

void IT_dump();				// ("cpu.c")

void final_statistics () {
	double expired, speed;		/* for statistics information */
	stop_ticks = get_ticks();			// (TMP)

	logF (0, "\n");

	IT_dump();

	expired = (double)(stop_ticks - start_ticks) / 1000;

	if ( expired != 0.0 )
		speed = (((double)pdp.total) / expired );
	else
		speed = 0.0;
	logF (0, "Expired time: %.5g sec\n", (double) expired);
	logF (0, "Instructions executed: %d\n", pdp.total);
	logF (0, "Simulation rate: %.5g instructions per second\n", speed);
	logF (0, "BK speed: %.5g instructions per second\n",
		(double) pdp.total * TICK_RATE / pdp.clock);
	pdp.total = 0;
}	// final_statistics



/*
 * args()
 */

int args(int argc, char **argv ) {
	char *farg;
	char **narg;

	narg = argv;
	while ( --argc ) {
		narg++;
		farg = *narg;
		if ( *farg++ == '-' ) {
			switch( *farg ) {
			 case '0': case '1': case '2': case '3': case '4':
				bkmodel = *farg - '0';
				break;
			 case 'K':
				bkmodel = 9;
				// Terak has no sound yet, turn sound off
				nflag = 0;
				break;
			 case 'A':
				floppyA = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'B':
				floppyB = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'C':
				floppyC = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'D':
				floppyD = *++farg ? farg : (argc--,*++narg);
				break;
			case 'a':
				aflag = 0;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'n':
				nflag = 0;
				break;
			case 'h':
				hflag = 1;
				break;
			case 'v':
				covoxflag = 1;
				break;
			case 'y':
				synthflag = 1;
				break;
			case 'm':
				mouseflag = *(farg+1) ? *(farg+1)-'0' : 1;
				break;
			case 'p':
				plipflag = 1;
				break;
			case 'S':
				fullspeed = 1;
				break;
			case 's':
				turboflag = 1;
				break;
			case 'R':
				rompath12 = *++farg ? farg : (argc--,*++narg);
				break;
			case 'r':
				rompath16 = *++farg ? farg : (argc--,*++narg);
				break;

			case 'k':
				kbscript_file (*++farg ? farg : (argc--,*++narg));
				break;

			case 'd':
				dpscript_file (*++farg ? farg : (argc--,*++narg));
				break;

			case 'T':
				tapeflag = 1;
				break;

			case 't':
				traceflag = 1;
				if (*++farg)
					strcpy(init_path, farg);
				break;

			case 'l':
				printer_file = *++farg ? farg : (argc--,*++narg);
				break;

			case 'x':
				launch (*++farg ? farg : (argc--,*++narg));
				break;

			default:
				return -1;
				/*NOTREACHED*/
				break;
			}
		} else {
			return -1;
		}
	}
	return 0;
}

double ticks_timer = 0.0;
double ticks_screen = 0.0;
extern unsigned long pending_interrupts;

// Show display or control character
static char *code_char (unsigned char ch) {
static char buffer[16];

#if 0
static char cyr_KOI8 [64] =
	"þàáöäåôãõèéêëìíîïÿðñòóæâüûçøýù÷ú"
	"ÞÀÁÖÄÅÔÃÕÈÉÊËÌÍÎÏßÐÑÒÓÆÂÜÛÇØÝÙ×Ú";
#endif

static char *low_names [] = {
	0,						// 0
	0,						// 1
	0,						// 2
	0,						// 3
	0,						// 4
	0,						// 5
	0,						// 6
	0,						// 7
	"ÂËÅÂÎ",			// 8
	0,						// 9
	"ÂÂÎÄ",				// 10
	0,						// 11
	"ÑÁÐ",				// 12
	"ÓÑÒ_ÒÀÁ",		// 13
	"ÐÓÑ",				// 14
	"ËÀÒ",				// 15
	"ÑÁÐ_ÒÀÁ",		// 16
	0,						// 17
	"HOME",				// 18
	"ÂÑ",					// 19
	"ÃÒ",					// 20
	0,						// 21
	"ÑÄÂ",				// 22
	"ÐÀÇÄÂ",			// 23
	"ÓÄ",					// 24
	"ÂÏÐÀÂÎ",			// 25
	"ÂÂÅÐÕ",			// 26
	"ÂÍÈÇ",				// 27
	"ÂÂ+ÂË",			// 28
	"ÂÂ+ÂÏ",			// 29
	"ÂÍ+ÂÏ",			// 30
	"ÂÍ+ÂË"				// 31
	};

static char *high_names [] = {
	"ÇÁ",					// 127
	0,						// 128
	"ÏÎÂÒ",				// 129
	"ÈÍÄ ÑÓ",			// 130
	0,						// 131
	"ÁËÎÊ ÐÅÄ",		// 132
	0,						// 133
	0,						// 134
	0,						// 135
	0,						// 136
	"ÒÀÁ",				// 137
	0,						// 138
	0,						// 139
	"ÐÏ",					// 140
	0,						// 141
	0,						// 142
	0,						// 143
	"ØÀÃ",				// 144
	"ÖÂ 0",				// 145
	"ÖÂ 1",				// 146
	"ÖÂ 2",				// 147
	"ÖÂ 3",				// 148
	"ÃÐÀÔ",				// 149
	"ÇÀÏ",				// 150
	"ÑÒÈÐ",				// 151
	"ÐÅÄ",				// 152
	"ÑÁÐ ->",			// 153
	"ÊÓÐÑÎÐ",			// 154
	"32/64",			// 155
	"ÈÍÂ.Ñ",			// 156
	"ÈÍÂ.Ý",			// 157
	"ÓÑÒ.ÈÍÄ",		// 158
	"ÏÎÄ×",				// 159
	};

if (ch < ' ') {
	if (low_names[ch])
		sprintf (buffer, "[%s]", low_names[ch]);
	else
		sprintf (buffer, "<%u>", ch);
	return buffer;
	}

else if (ch < 127)
	return 0;

else if (ch < 160) {
	if (high_names[ch - 127])
		sprintf (buffer, "[%s]", high_names[ch - 127]);
	else
		sprintf (buffer, "<%u>", ch);
	return buffer;
	}

else if (ch < 192) {
	sprintf (buffer, "[ÏÑÃ %u]", ch - 60);
	return buffer;
	}

else return 0;
}	// code_char

void echo_char (FILE *out, unsigned char ch) {
char *str = code_char(ch);
if (str)
	while (*str) putc (*str ++, out);
else { putc ('\'', out); putc (ch, out); putc ('\'', out); }
}	// echo_char

void echo_string (FILE *out, pdp_regs *p,
	d_word addr, unsigned count, unsigned char sentinel) {
if (! count) count = 0200000;
unsigned mode = 0;

while (count --) {
	unsigned char ch;
	ll_byte (p, addr ++, &ch);
	if (ch == sentinel) break;

	char *str = code_char(ch);
	if (str) {
		if (mode) { putc ('"', out); mode = 0; }
		while (*str) putc (*str ++, out);
		}
	else {
		if (! mode) { putc ('"', out); mode = 1; }
		putc (ch, out);
		}
	}

if (mode) putc ('"', out);
}	// echo_string

void echo_kbkey (FILE *out, pdp_regs *p, d_word keyaddr) {
// TODO: if keyaddr is 0 ???

unsigned char length;
ll_byte (p, keyaddr ++, &length);
echo_string (out, p, keyaddr, length, 0x00);
}	// echo_kbkey

int tape_io_log (register pdp_regs *p, d_word control) {
int i;
d_byte command, reply;
d_word addr, length;
d_byte name[16+1];

ll_byte (p, control ++, &command);
ll_byte (p, control ++, &reply);
ll_word (p, control, &addr);
control += 2;
ll_word (p, control, &length);
control += 2;

for (i = 0; i != 16; ++ i) {
	ll_byte (p, control ++, &name[i]);
	}

printf ("Tape I/O attempt: cmd = %u\n", command);
printf ("Reply = %u\n", reply);
printf ("Addr = %o, Len = %o\n", addr, length);
printf ("Name = \"%.16s\"\n", name);

// TMP
#if 0

if (command == 2) {
	printf ("Write emulation... ");

	name[16] = '\0';
	FILE *out = fopen(name, "wb");
	if (out == NULL) {
		printf (" failed!\n");
		return 1;
		}

	d_byte byte;
	while (length --) {
		ll_byte (p, addr ++, &byte);
		fputc (byte, out);
		}

	fclose (out);
	printf ("done!\n");

	return 1;
	}

if (command == 3) {
	printf ("Read emulation... ");
	
	// TMP: convert name...
	FILE *in = fopen(name, "rb");
	if (in == NULL) {
		printf (" failed!\n");
		return 1;
		}

	d_byte byte;
	while (length --) {
		byte = getc (in);
		st_byte (p, addr ++, byte);
		}

	fclose (in);
	printf ("done!\n");

	return 1;
	}

#endif

return 0;
}		// tape_io_log

// TRAP's trace/intercept
int TRAP_intercept (flag_t flag, unsigned no, pdp_regs *p) {
// TODO: intercept and/or log TRAPs
return 0;
}		// TRAP_intercept

// EMT's trace/intercept
int EMT_intercept (flag_t flag, unsigned no, pdp_regs *p) {
if (flag) {
logF (log_EMT, "EMT %o: ", no);

switch (no) {
  case 04:			// Initialise keyboard driver
			logF (log_EMT, "KB_INIT");
			break;

	case 06:			// Read character from keyboard
			logF (log_EMT, "KB_GETC");

			if (kbscript_on) {
			char ch = kbscript_getch();
			if (ch) {
				logF (log_EMT, " << ");
				if (!log_EMT) echo_char (stderr, ch);
				logF (log_EMT, "\n");

				p->regs[0] &= 0xFF00; p->regs[0] |= ch;
				return 1;
				}
			}
			break;

	case 010:			// Read string from keyboard
			logF (log_EMT, "KB_GETS");
			// TODO: intercept & emulate
			break;

	case 012:
			logF (log_EMT, "KB_SETKEY #%u = ", p->regs[0]);
			if (!log_EMT) echo_kbkey (stderr, p, p->regs[2]);
			break;

	case 014:			// Initialise display driver
			logF (log_EMT, "DP_INIT");
			break;

	case 016:			// Write character to display
			logF (log_EMT, "DP_PUTC ");
			if (!log_EMT) echo_char (stderr, p->regs[0]);

			if (dpscript_on)
				dpscript_putch (p->regs[0]);
			break;

	case 020:			// Write string to display
			logF (log_EMT, "DP_PUTS ");
			if (!log_EMT)
				echo_string (stderr, p, p->regs[1], p->regs[2] & 0xFF, p->regs[2] >> 8);

			if (dpscript_on)
				dpscript_putstr (p, p->regs[1], p->regs[2] & 0xFF, p->regs[2] >> 8);
			break;

	case 022:			// Write character to status line
			logF (log_EMT, "DP_PUTSTAT [%u] ", p->regs[1]);
			if (!log_EMT) echo_char (stderr, p->regs[0]);
			break;

	case 024:			// Set cursor position
			logF (log_EMT, "DP_SETPOS (%u,%u)", p->regs[1], p->regs[2]);
			break;

	case 026:			// Get cursor position
			logF (log_EMT, "DP_GETLOC ");
			break;

	case 030:			// Draw point
			logF (log_EMT, "DP_PLOT %u, (%u,%u)", p->regs[0], p->regs[1], p->regs[2]);
			break;

	case 032:			// Draw vector
			logF (log_EMT, "DP_LINE %u, (%u,%u)", p->regs[0], p->regs[1], p->regs[2]);
			break;

	case 034:			// Get display status
			logF (log_EMT, "DP_GETSTAT");
			break;

	case 036:
			logF (log_EMT, "TAPE_IO\n");
			return tape_io_log (p, p->regs[1]);
			break;

	case 040:
			logF (log_EMT, "LINE_INIT");
			break;

	case 042:
			logF (log_EMT, "LINE_SNDC");
			break;

	case 044:
			logF (log_EMT, "LINE_RCVC");
			break;

	case 046:
			logF (log_EMT, "LINE_SNDBLK");
			break;

	case 050:
			logF (log_EMT, "LINE_RCVBLK");
			break;

	}	// switch


return 0;
}

else {
switch (no) {
	case 06:			// Read character from keyboard
		logF (log_EMT, " => ");
		if (!log_EMT) echo_char (stderr, p->regs[0]);
		break;

	case 010:			// Read string from keyboard
		// TODO: echo string on return...
		break;

	case 026:			// Get cursor position
		logF (log_EMT, " => (%u,%u)", p->regs[1], p->regs[2]);
		break;

	case 034:			// Get display status
		logF (log_EMT, " => %o", p->regs[0]);
		break;

	}
	logF (log_EMT, ".\n");
	}
return 0;
}	// EMT_intercept

void run_2(pdp_regs *p, int flag) {
	CPU_RES result;		/* result of execution */
	CPU_RES result2 = CPU_OK;	/* result of error handling */
	extern void intr_hand();	/* SIGINT handler */
	register unsigned priority;	/* current processor priority */
	int rtt = 0;			/* rtt don't trap yet flag */
	d_word oldpc;
	static char buf[80];

	/*
	 * Clear execution stop flag and install SIGINT handler.
	 */

	stop_it = 0;
	signal( SIGINT, intr_hand );

	Uint32 last_screen_update = get_ticks();
	double timing_delta = p->clock - last_screen_update * (TICK_RATE/1000.0);
	/*
	 * Run until told to stop.
	 */

	do {

		/*
		 * Fetch and execute the instruction.
		 */
	
		if (traceflag) {
			disas(p->regs[PC], buf);
			if (tracefile) fprintf(tracefile, "%s\t%s\n", buf, state(p));
			else printf("%s\n", buf);
		}

		oldpc = p->regs[PC];

		result = cpu_instruction (p);

		/*
		 * Mop up the mess.
		 */

		if (result != CPU_OK) {
			switch( result ) {
			case CPU_BUS_ERROR:			/* vector 4 */
				p->clock += 64;

			case CPU_ODD_ADDRESS:
				logF(0, " pc=%06o, last branch @ %06o\n", oldpc, last_branch );
				result2 = cpu_service (004);
				break;

			case CPU_ILLEGAL:		/* vector 10 */
#undef VERBOSE_ILLEGAL
#ifdef VERBOSE_ILLEGAL
				disas(oldpc, buf);
				logF(0, "Illegal inst. %s, last branch @ %06o\n", buf, last_branch);
#endif
				result2 = cpu_service (010);
				break;

			case CPU_BPT:			/* vector 14 */
				result2 = cpu_service (014);
				break;

			case CPU_IOT:			/* vector 20 */
				result2 = cpu_service (020);
				break;

			case CPU_EMT:			/* vector 30 */
				{
				d_word word;
				ll_word(p, p->regs[PC]-2, &word);
				if ((d_word) (word & ~0xFF) == (d_word) 0104000) {
					if (EMT_intercept (true, word & 0xFF, p)) break;
					}
				else logF (0, "Abnormal EMT @PC=%o?\n", p->regs[PC]-2);
				}

				result2 = cpu_service (030);
				break;

			case CPU_TRAP:			/* vector 34 */
				{
				d_word word;
				ll_word(p, p->regs[PC]-2, &word);
				if ((d_word) (word & ~0xFF) == (d_word) 0104400) {
					if (TRAP_intercept (true, word & 0xFF, p)) break;
					}
				else logF (0, "Abnormal TRAP @PC=%o?\n", p->regs[PC]-2);
				}

				result2 = cpu_service (034);
				break;

			case CPU_WAIT:
				in_wait_instr = 1;
				result2 = CPU_OK;
				break;

			case CPU_RTI:
				{
				d_word word;
				ll_word(p, p->regs[PC]-2, &word);

				if ((d_word) (word & ~0xFF) == (d_word) 0104000) {
					EMT_intercept (false, word & 0xFF, p);
					}

				else if ((d_word) (word & ~0xFF) == (d_word) 0104400) {
					TRAP_intercept (false, word & 0xFF, p);
					}
				}

				result = result2 = CPU_OK;
				break;

			case CPU_RTT:
				rtt = 1;
				result2 = CPU_OK;
				break;

			case CPU_HALT:
				io_stop_happened = 4;
				result2 = cpu_service (004);
				break;

			default:
				logF(0, "\nUnexpected return.\n");
				logF(0, "exec=%d pc=%06o ir=%06o\n", result, oldpc, p->ir);
				flag = 0;
				result2 = CPU_OK;
				break;
			}

			if (result2 != CPU_OK) {
				logF(0, "\nDouble trap @ %06o.\n", oldpc);
				lc_word(0177716, &p->regs[PC]);
				p->regs[PC] &= 0177400;
				p->regs[SP] = 01000;	/* whatever */
			}
		}

		if ((p->psw & CC_T) && (rtt == 0)) {
			/* trace bit */
			if (cpu_service (014) != CPU_OK) {
				logF(0, "\nDouble trap @ %06o.\n", p->regs[PC]);
				lc_word(0177716, &p->regs[PC]);
				p->regs[PC] &= 0177400;
				p->regs[SP] = 01000;	/* whatever */
			}
		}
		rtt = 0;
		p->total++;

		if (bkmodel && p->clock >= ticks_timer) {
			scr_sync();
			if (timer_intr_enabled) {
				ev_register(TIMER_PRI, cpu_service, 0, 0100);
			}
			ticks_timer += half_frame_delay;
		}

		if (p->clock >= ticks_screen) {
		    /* In full speed, update every 40 real ms */
		    if (fullspeed) {
			Uint32 cur_sdl_ticks = get_ticks();
		 	if (cur_sdl_ticks - last_screen_update >= 40) {
			    last_screen_update = cur_sdl_ticks;
			    scr_flush();
			}
		    } else {
			scr_flush();
		    }

		  input_processing();

		    ticks_screen += frame_delay;

		    /* In simulated speed, if we're more than 10 ms
		     * ahead, slow down. Avoid rounding the delay up
		     * by SDL.
		     */
		    if (!fullspeed) {
		    	double cur_delta =
				p->clock - get_ticks() * (TICK_RATE/1000.0);
			if (cur_delta - timing_delta > TICK_RATE/100) {
				int msec = (cur_delta - timing_delta) / (TICK_RATE/1000);
				wait_ticks(msec / 10 * 10);
			}
		    }
		}

		/*
		 * See if execution should be stopped.  If so
		 * stop running, otherwise look for events
		 * to fire.
		 */

		if ( stop_it ) {
			logF(0, "\nExecution interrupted.\n");
			flag = 0;
		} else {
			priority = ( p->psw >> 5) & 7;
			if ( pending_interrupts && priority != 7 ) {
				ev_fire( priority );
			}
		}

		if (checkpoint(p->regs[PC])) {
			flag = 0;
		}

	} while( flag );

	signal( SIGINT, SIG_DFL );
}

/*
 * run() - Run instructions (either just one or until something bad
 * happens).  Lots of nasty stuff to set up the terminal and the
 * execution timing.
 */

void run( int flag )
{
	register pdp_regs *p = &pdp;	/* pointer to global struct */

	/*
	 * Set up the terminal cbreak i/o and start running.
	 */

	start_ticks = get_ticks();
	run_2( p, flag );
	stop_ticks = get_ticks();

	if (!flag)
		return;
}	// run


/* launch() - Load and launch .bin file */
void launch (char *file) {
static char buffer[64];
sprintf (buffer, "%s\nM\n%s\nS\n",
	bkmodel == HW_BK0010 ? "P M" : "MO", file);

kbscript_buffer (buffer);
}	// launch

/*
 * intr_hand() - Handle SIGINT during execution by breaking
 * back to user interface at the end of the current instruction.
 */

void intr_hand()
{
	stop_it = 1;
}

char *monitor10help = 
"BK0010 MONITOR (the OS) commands:\n\n\
 'A' to 'K'  - Quit MONITOR.\n\
 'M'         - Read from tape. Press 'Enter' and type in the filename of\n\
               the desired .bin snapshot. Wait until the data loads into\n\
               the memory or use F12 instead.\n\
 'S'         - Start execution. You can specify an address to start from.\n";

char *monitor11help = 
"BK0011M BOS commands:\n\n\
 'B'         - Boot from any bootable floppy.\n\
 'xxxB'      - Boot from the floppy drive xxx.\n\
 'L'         - Load file from tape\n\
 'xxxxxxL'   - Load file to address xxxxxx.\n\
 'M' or '0M' - Turn the tape-recoder on.\n\
 'xM'        - Turn the tape-recoder off.\n\
 'G'         - Run currently loaded program.\n\
 'xxxxxxG'   - Run from address xxxxxx.\n\
 'P'         - Continue after the STOP key press or HALT.\n\
 'Step'      - Execute a single instruction and return to MONITOR.\n\
 'Backspace' - Delete last digit (digits only).\n\
 'xxxxxx/'   - Open word xxxxxx (octal) in memory for editing.\n\
 'xxxxxx\\'   - Open byte xxxxxx (octal) in memory for editing.\n\
 'Rx'        - Open system register x for editing.\n\
 'Enter'     - Close opened memory cell and accept changes.\n\
 'Up'        - Move to the next memory cell and accept changes.\n\
 'Down'      - Move to the previous memory cell and accept changes\n\
 'Left'      - Jump to address <address>+<word>+2 (\"67\" addressing).\n\
 'Right'     - Jump to address <address>+<byte>*2+2 (assembler 'BR' jump)\n\
 '@'         - Close and jump to the address stored in the current memory cell.\n\
 'N;MC'      - Map memory page N (octal) to address range M (octal).\n";

void showemuhelp() {
    logF(0, "Emulator window hotkeys:\n\n");
    logF(0, " ScrollLock - Toggle video mode (B/W, Color)\n");
    logF(0, " Left Super+F11 - Reset emulated machine\n");
    logF(0, " F12 - Load a file into BK memory\n\n");
}

void showbkhelp() {
    switch (bkmodel) { /* Make the hints model-specific */
    case HW_BK0010:
				logF(0, monitor10help);
        logF(0, " 'T' - Run built-in tests.\n\n");
        logF(0, "Type 'P M' to quit FOCAL and get the MONITOR prompt.\n");
        logF(0, "Type 'P T' to enter the test mode. 1-5 selects a test.\n\n");
    break;

    case HW_BK0010_01:
				logF(0, monitor10help);
        logF(0, "\nType 'MO' to quit BASIC VILNIUS 1986 and get the MONITOR prompt.\n\n");
    break;

    case HW_BK0010_01F:
				logF(0, monitor10help);
        logF(0, "\nType 'S160000' to boot from floppy A:.\n");
        logF(0, "The BASIC ROM is disabled.\n\n");
    break;

    case HW_BK0011:
				logF(0, monitor11help);
        logF(0, "\nBK-0011M boots automatically from the first floppy drive available.\n\n");
    break;

    case HW_BK0011S:
				logF(0, monitor11help);
    break;
    }
}

int main (int argc, char **argv) {

	/* Gettext stuff */
 	setlocale (LC_ALL, "");
/* GDG 	bindtextdomain ("bk", "/usr/share/locale");
 	textdomain ("bk"); */

	init_config();

	aflag = 1;		/* auto boot */
	nflag = 1;
	/* nothing is connected to the port by default, use ~/.bkrc */

	if ( args( argc, argv ) < 0 ) {
		logF (0, "Usage: %s [options]\n", argv[0]);
		logF (0, "   -0        BK-0010\n");
		logF (0, "   -1        BK-0010.01\n");
		logF (0, "   -2        BK-0010.01 + FDD\n");
		logF (0, "   -3        BK-0011M + FDD\n");
		logF (0, "   -K        Terak 8510/a\n");
		logF (0, "   -A<file>  A: floppy image file or device (instead of %s)\n", floppyA);
		logF (0, "   -B<file>  B: floppy image file or device (instead of %s)\n", floppyB);
		logF (0, "   -C<file>  C: floppy image file or device (instead of %s)\n", floppyC);
		logF (0, "   -D<file>  D: floppy image file or device (instead of %s)\n", floppyD);
		logF (0, "   -a        Do not boot automatically\n");
		logF (0, "   -c        Color mode\n");
		logF (0, "   -n        Disable sound \n");
		logF (0, "   -v        Enable Covox\n");
		logF (0, "   -y        Enable AY-3-8910\n");
		logF (0, "   -m        Enable mouse\n");
		logF (0, "   -S        Full speed mode\n");
		logF (0, "   -s        \"TURBO\" mode (Real overclocked BK)\n");
		logF (0, "   -R<file>  Specify an alternative ROM file @ 120000.\n");
		logF (0, "   -r<file>  Specify an alternative ROM file @ 160000.\n");
		logF (0, "   -T        Disable reading from tape\n");
		logF (0, "   -t        Trace mode, -t<file> - to file\n");
		logF (0, "   -l<path>  Enable printer and specify output pathname\n");
		logF (0, "\n\
The default ROM files are stored in\n\
%s or the directory specified\n\
by the environment variable BK_PATH.\n", romdir);
		logF (0, "\nExamples:.\n");
		logF (0, "   'bk -R./BK.ROM' - Use custom ROM\n");
		logF (0, "   'bk -a -n -f'   - Developer's mode\n");
		logF (0, "   'bk -c -d'      - Gaming mode\n\n");

		exit (-1);
	}

	atexit(SDL_Quit);
	atexit(disk_finish);
	atexit(final_statistics);			/* GDG */

	/* Set ROM configuration */

	if (getenv("BK_PATH"))
		romdir = getenv("BK_PATH");

	switch( bkmodel ) {
	case HW_BK0010:
		rompath10 = monitor10rom;
		rompath12 = focal10rom;
		rompath16 = 0;
		TICK_RATE = 3000000;
		break;

	case HW_BK0010_01:
		rompath10 = monitor10rom;
		rompath12 = basic10rom;
		rompath16 = 0;
		TICK_RATE = 3000000;
		break;

	case HW_BK0010_01F:
		rompath10 = monitor10rom;
		rompath12 = 0;
		rompath16 = diskrom;
		TICK_RATE = 3000000;
		break;

  case HW_BK0011:
	case HW_TERAK:
		rompath10 = rompath12 = rompath16 = 0;
		TICK_RATE = 4000000;
		break;

	case HW_BK0011S:
		rompath10 = rompath12 = rompath16 = 0;
		TICK_RATE = 3000000;
		break;

	default:
		logF(0, "Unknown BK model. Bailing out.\n", argv[0]);
		exit( -1 );
	}
	
	/* Turn on the "TURBO" mode */
	if ( turboflag ) {
	    TICK_RATE = (TICK_RATE * 2); 
	}

	logF(0, "Initializing SDL.\n");

	if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0)) {
		logF(0, "Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}

	logF(0, "Welcome to \"Elektronika BK\" emulator!\n\n");
	showemuhelp();		/* print a short emulator help message */
	showbkhelp();			/* print a short help message */

	logF(0, "SDL initialized.\n");

	/* Convert BK model to 0010/0011 flag */
	fake_disk = bkmodel >= 2;
	terak = bkmodel == 9;
	bkmodel = bkmodel >= 3;

	tty_open();             /* initialize the tty stuff */
	ev_init();		/* initialize the event system */
	cpu_init(&pdp);		/* ...the simulated cpu */
	mem_init();		/* ...main memory */
	scr_init();		/* video display */
	sound_init();
	boot_init();		/* ROM blocks */

	if (terak) {
		// setup_terak();
	} else {
	if (mouseflag)
		device_install(&q_mouse);

	if (printer_file)
		device_install(&q_printer);

	if (covoxflag)
		device_install(&q_covox);

	if (synthflag)
		device_install(&q_synth);

	if (plipflag)
		device_install(&q_bkplip);
	}

	q_reset();             /* ...any devices */

	/* Set checkpoints */
	if (fake_tape && !bkmodel) {
		install_cp (0116256, "Simulating write file to tape\n", fake_write_file);
		install_cp (0116712, "Simulating reading tune-up sequence\n", fake_tuneup_sequence);
		install_cp (0117260, "Simulating reading array with tune-up\n", fake_array_with_tuneup);
		install_cp (0117376, 0, fake_read_strobe);
		}

	if (fake_disk) {
		install_cp (0160250, 0, fake_disk_io);
		install_cp (0160372, 0, fake_sector_io);
	}
	/* Starting frame rate */ 
	frame_delay = TICK_RATE/25;
	half_frame_delay = TICK_RATE/50;

	if (terak) {
		pdp.regs[PC] = 0173000;
	} else {
		lc_word(0177716, &pdp.regs[PC]);
		pdp.regs[PC] &= 0177400;
	}
	if (init_path[0]) {
		tracefile = fopen(init_path, "w");
	}

	if ( aflag ) {
		run( 1 );			/* go for it */	
		ui();
	} else {
		ui();				/* run the user interface */
	}

	return 0;		/* get out of here */
}

