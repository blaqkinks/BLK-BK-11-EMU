
/*
 *
 *	"cpu.c":
 *	Emulation of K1801VM1 (LSI-11 analog) CPU.
 *
 *	by Allynd Dudnikov (blaqk).
 *
 *	Based on PDP-11 emulator by Eric A. Edwards, but heavily changed.
 *
 *	Includes instruction timing values by       .
 *
 */

#include "defines.h"

// Instruction types
enum IT_enum {
	IT__ILL,

	IT_ADC,
	IT_ADCB,
	IT_ADD,
	IT_ASH,
	IT_ASHC,
	IT_ASL,
	IT_ASLB,
	IT_ASR,
	IT_ASRB,
	IT_BCC,
	IT_BCS,
	IT_BEQ,
	IT_BGE,
	IT_BGT,
	IT_BHI,
	IT_BIC,
	IT_BICB,
	IT_BIS,
	IT_BISB,
	IT_BIT,
	IT_BITB,
	IT_BLE,
	IT_BLOS,
	IT_BLT,
	IT_BMI,
	IT_BNE,
	IT_BPL,
	IT_BPT,
	IT_BR,
	IT_BVC,
	IT_BVS,
	IT_CLR,
	IT_CLRB,
	IT_CCC,
	IT_CMP,
	IT_CMPB,
	IT_COM,
	IT_COMB,
	IT_DEC,
	IT_DECB,
	IT_DIV,
	IT_EMT,
	IT_HALT,
	IT_INC,
	IT_INCB,
	IT_IOT,
	IT_JMP,
	IT_JSR,
	IT_MARK,
	IT_MFPD,
	IT_MFPI,
	IT_MFPS,
	IT_MOV,
	IT_MOVB,
	IT_MTPD,
	IT_MTPI,
	IT_MTPS,
	IT_MUL,
	IT_NEG,
	IT_NEGB,
	IT_RESET,
	IT_ROL,
	IT_ROLB,
	IT_ROR,
	IT_RORB,
	IT_RTI,
	IT_RTS,
	IT_RTT,
	IT_SBC,
	IT_SBCB,
	IT_SCC,
	IT_SOB,
	IT_SUB,
	IT_SWAB,
	IT_SXT,
	IT_TRAP,
	IT_TST,
	IT_TSTB,
	IT_WAIT,
	IT_XOR,

	IT__TOTAL
	};

static char *_inames [IT__TOTAL] = {
	"<illegal>",

	"ADC",
	"ADCB",
	"ADD",
	"ASH",
	"ASHC",
	"ASL",
	"ASLB",
	"ASR",
	"ASRB",
	"BCC",
	"BCS",
	"BEQ",
	"BGE",
	"BGT",
	"BHI",
	"BIC",
	"BICB",
	"BIS",
	"BISB",
	"BIT",
	"BITB",
	"BLE",
	"BLOS",
	"BLT",
	"BMI",
	"BNE",
	"BPL",
	"BPT",
	"BR",
	"BVC",
	"BVS",
	"CLR",
	"CLRB",
	"CCC",
	"CMP",
	"CMPB",
	"COM",
	"COMB",
	"DEC",
	"DECB",
	"DIV",
	"EMT",
	"HALT",
	"INC",
	"INCB",
	"IOT",
	"JMP",
	"JSR",
	"MARK",
	"MFPD",
	"MFPI",
	"MFPS",
	"MOV",
	"MOVB",
	"MTPD",
	"MTPI",
	"MTPS",
	"MUL",
	"NEG",
	"NEGB",
	"RESET",
	"ROL",
	"ROLB",
	"ROR",
	"RORB",
	"RTI",
	"RTS",
	"RTT",
	"SBC",
	"SBCB",
	"SCC",
	"SOB",
	"SUB",
	"SWAB",
	"SXT",
	"TRAP",
	"TST",
	"TSTB",
	"WAIT",
	"XOR"
};	// _inames

static unsigned IT_count [IT__TOTAL];

#define	UNDEF	0

#define	IID(IT)		IT_cp(IT)

void IT_cp (unsigned IT) {
if (IT < IT__TOTAL)
		++ IT_count [IT];
}	// IT_cp

void IT_dump () {
unsigned i;
logF (0, "Instructions statistics\n");
for (i = 0; i != IT__TOTAL; ++ i)
	logF (0, "%s: %u\n", _inames[i], IT_count[i]);
}	// IT_dump

// Current processor
static pdp_regs *_proc;

#define	REG(n)	(_proc->regs[n])

#define	R_SP		REG(SP)
#define	R_PC		REG(PC)

#define	R_PSW		(_proc->psw)

#define Timing(ticks)		(_proc->clock += (ticks))

CPU_RES lc_byte(d_word baddr, d_byte *byte) {
return ll_byte (_proc, baddr, byte);
}	// lc_byte

CPU_RES sc_byte(d_word laddr, d_byte byte) {
return sl_byte (_proc, laddr, byte);
}	// sc_byte

/*
 * Instruction Table for Fast Decode.
 */

#define SRC_MODE(IR)	((IR & 07000) >> 9)
#define SRC_REG(IR)		((IR & 0700) >> 6)
#define DST_MODE(IR)	((IR & 070) >> 3)
#define DST_REG(IR)		((IR & 07))

//
// Instruction timings
//

#define REGREG 12

static int a_time[8]  = {0, 12, 12, 20, 12, 20, 20, 28};
static int b_time[8]  = {0, 20, 20, 32, 20, 32, 32, 40};
static int ab_time[8] = {0, 16, 16, 24, 16, 24, 24, 32};
static int a2_time[8] = {0, 20, 20, 28, 20, 28, 28, 36};
static int ds_time[8] = {0, 32, 32, 40, 32, 40, 40, 48};

#define a1_time a_time
#define dj_time a2_time

#define	time_RTI		40
#define	time_INT		68
#define	time_WAIT		1167

//
//	Condition codes
//

#define CLR_CC(f)			(R_PSW &= ~(f))
#define SET_CC(f)			(R_PSW |= (f))
#define	TST_CC(f)			(R_PSW & (f))

void calc_NZ (d_word val) {
if (val)
	CLR_CC(CC_Z);
else
	SET_CC(CC_Z);

if (val & 0x8000)
	SET_CC(CC_N);
else
	CLR_CC(CC_N);
}	// calc_NZ

void calcB_NZ (d_byte val) {
if (val)
	CLR_CC(CC_Z);
else
	SET_CC(CC_Z);

if (val & 0x80)
	SET_CC(CC_N);
else
	CLR_CC(CC_N);
}	// calcB_NZ

void calc_VC (d_word src_word, d_word dst_word) {
if (src_word < dst_word)
			SET_CC(CC_C);
else	CLR_CC(CC_C);

if ((~((src_word - dst_word) ^ dst_word) &
	  (dst_word ^ src_word)) & 0x8000)
			SET_CC(CC_V);
else	CLR_CC(CC_V);
}	// calc_VC

void calcB_VC (d_byte src_byte, d_byte dst_byte) {
if (src_byte < dst_byte)
			SET_CC(CC_C);
else	CLR_CC(CC_C);

if ((~((src_byte - dst_byte) ^ dst_byte) &
	  (dst_byte ^ src_byte)) & 0x80)
			SET_CC(CC_V);
else	CLR_CC(CC_V);
}	// calcB_VC

void calc_V (flag_t val) {
if (val)
	SET_CC(CC_V);
else
	CLR_CC(CC_V);
}	// calc_V

void calc_C (flag_t val) {
if (val)
	SET_CC(CC_C);
else
	CLR_CC(CC_C);
}	// calc_C

void calc_V_CxN () {
d_word flags = R_PSW & (CC_C|CC_N);
if (flags == CC_C || flags == CC_N)
	SET_CC(CC_V);
else
	CLR_CC(CC_V);
}	// calc_V_CxN

//
//
// Addressing modes
//
//

// Address or register operand
typedef unsigned AOR_t;

// Mask to distinguish between address/register
enum { AOR_REG = 1 << 16 };

// Calculate AOR from modreg mask
static CPU_RES calc_AOR(unsigned modreg, int is_byte, AOR_t *AOR_p) {
	CPU_RES result;
	int is_indir = 0;
	unsigned reg = modreg & 07;
	d_word addr;

	if (reg >= 6)			// (SP or PC)
		is_byte = 0;

	switch (modreg & 070) {
	case 000:			// Rn: register
		*AOR_p = reg | AOR_REG;
		return CPU_OK;

	case 010:			// (Rn): register indirect
		addr = REG(reg);
		break;

	case 030:			// @(Rn)+: autoincrement indirect
		++ is_indir;
		// ...and fall through

	case 020:			// (Rn)+: autoincrement
		addr = ((is_byte && !is_indir)? (REG(reg) ++) : (REG(reg) += 2) - 2);
		break;

	case 050:			// @-(Rn): autodecrement indirect
		++ is_indir;
		// ...and fall through

	case 040:			// -(Rn): autodecrement
		addr = ((is_byte && !is_indir)? (-- REG(reg)) : (REG(reg) -= 2));
		break;

	case 070:			// @off(Rn): offset indirect
		++ is_indir;
		// ...and fall through

	case 060:			// off(Rn): offset
		if ((result = lc_word ((R_PC += 2) - 2, &addr)) != CPU_OK)
			return result;

		addr += REG(reg);
		break;
	}	// switch

if (is_indir)
	if ((result = lc_word (addr, &addr)) != CPU_OK)
		return result;

*AOR_p = addr;
return CPU_OK;
}		// calc_AOR

// Load byte from AOR to 'addr'
static CPU_RES ldbyte_AOR (AOR_t AOR, d_byte *data_p) {
if (AOR & AOR_REG)
	{ *data_p = REG(AOR & 07); return CPU_OK; }
else
	{ return lc_byte (AOR, data_p); }
}		// ldbyte_AOR

// Store byte from 'data' to AOR
static CPU_RES stbyte_AOR (AOR_t AOR, d_byte data) {
if (AOR & AOR_REG) {
  d_word *reg = &REG(AOR & 07);
	*reg &= ~0xff; *reg |= data;
	return CPU_OK;
	}
else
	{ return sc_byte (AOR, data); }
}		// stbyte_AOR

// Load word from AOR to 'addr'
static CPU_RES ldword_AOR (AOR_t AOR, d_word *data_p) {
if (AOR & AOR_REG)
	{ *data_p = REG(AOR & 07); return CPU_OK; }
else
	{ return lc_word(AOR, data_p); }
}		// ldword_AOR

// Store word from 'data' to AOR
static CPU_RES stword_AOR (AOR_t AOR, d_word data) {
if (AOR & AOR_REG)
	{ REG(AOR & 07) = data; return CPU_OK; }
else
	{ return sc_word(AOR, data); }
}		// stword_AOR

/*
 * Common system stack operations
 */

static CPU_RES push(d_word data) {
	return sc_word (R_SP -= 2, data);
}	// push

static CPU_RES pop(d_word *data) {
	return lc_word ((R_SP += 2) - 2, data);
}	// pop

// Perform RTS PC
CPU_RES rts_PC (pdp_regs *p) {
	pop(&R_PC);
	return CPU_OK;
}	// rts_PC

/*
 * Branch Instructions
 */

bool BR_A(d_word psw)
	{ IID(IT_BR); return true; }

bool BR_NE(d_word psw)
	{ IID(IT_BNE); return (psw & CC_Z) == 0; }
bool BR_EQ(d_word psw)
	{ IID(IT_BEQ); return (psw & CC_Z) == CC_Z; }

bool BR_PL(d_word psw)
	{ IID(IT_BPL); return (psw & CC_N) == 0; }
bool BR_MI(d_word psw)
	{ IID(IT_BMI); return (psw & CC_N) == CC_N; }

bool BR_VC(d_word psw)
	{ IID(IT_BVC); return (psw & CC_V) == 0; }
bool BR_VS(d_word psw)
	{ IID(IT_BVS); return (psw & CC_V) == CC_V; }

bool BR_CC(d_word psw)
	{ IID(IT_BCC); return (psw & CC_C) == 0; }
bool BR_CS(d_word psw)
	{ IID(IT_BCS); return (psw & CC_C) == CC_C; }

bool BR_HI(d_word psw)
	{ IID(IT_BHI); return (psw & (CC_C|CC_Z)) == 0; }

/*
 * BLOS: Branch Lower or Same Instruction.
 */

bool BR_LOS (d_word psw) {
IID(IT_BLOS);
return (psw & CC_C) || (psw & CC_Z);
}	// BR_LOS

/*
 * BGE: Branch Greater Than or Equal Instruction.
 */

bool BR_GE (d_word psw) {
IID(IT_BGE);
psw &= CC_N|CC_V;
return !psw || psw == (CC_N|CC_V);
}	// BR_GE

/*
 * BLT: Branch Less Than Instruction.
 */

bool BR_LT (d_word psw) {
IID(IT_BLT);
psw &= CC_N|CC_V;
return psw == CC_N || psw == CC_V;
}	// BR_LT

/*
 * BLE: Branch Less Than Or Equal Instruction.
 */

bool BR_LE (d_word psw) {
IID(IT_BLE);
if (psw & CC_Z) return true;
psw &= CC_N|CC_V;
return psw == CC_N || psw == CC_V;
}	// BR_LE

/*
 * BGT: Branch Greater Than Instruction.
 */

bool BR_GT (d_word psw) {
IID(IT_BGT);
if (psw & CC_Z) return false;
psw &= CC_N|CC_V;
return !psw || psw == (CC_N|CC_V);
}	// BR_GT

/*
 *	Condition codes operations (SCC, CCC)
 */

static CPU_RES OP_SCC (d_word IR) {
	IID(IT_SCC);
	Timing (REGREG);
	R_PSW |= (IR & 017);
	return CPU_OK;
}	// OP_SCC

static CPU_RES OP_CCC (d_word IR) {
	IID(IT_CCC);
	Timing (REGREG);
	R_PSW &= ~(IR & 017);
	return CPU_OK;
}	// OP_CCC

/*
 * JMP: Jump Instruction.
 */

static CPU_RES OP_JMP (d_word IR) {
	CPU_RES result;
	AOR_t jmp_AOR;

	IID(IT_JMP);
	Timing (dj_time[DST_MODE(IR)]);

	if ((result = calc_AOR (IR, 0, &jmp_AOR)))
		return result;

	if (jmp_AOR & AOR_REG)
		return CPU_ILLEGAL;				// (can't transfer control to register)

	last_branch = R_PC;
	R_PC = (d_word) jmp_AOR;

	return CPU_OK;
}	// OP_JMP

/*
 * JSR: Jump To Subroutine Instruction.
 */

static CPU_RES OP_JSR (d_word IR) {
	AOR_t jmp_AOR;
	unsigned link_reg = SRC_REG(IR);
	CPU_RES result;

	IID(IT_JSR);
	last_branch = R_PC;

	Timing (ds_time[DST_MODE(IR)]);

	if ((result = calc_AOR (IR, 0, &jmp_AOR)) != CPU_OK)
		return result;

	if (jmp_AOR & AOR_REG)
		return CPU_ILLEGAL;				// (can't transfer control to register)

	if ((result = push(REG(link_reg))) != CPU_OK)
		return result;

	REG(link_reg) = R_PC;
	R_PC = (d_word) jmp_AOR;

	return CPU_OK;
}	// OP_JSR

/*
 * RTS: Return From Subroutine Instruction.
 */

static int OP_RTS(d_word IR) {
	d_word data;
	CPU_RES result;
	int link_reg = DST_REG(IR);

	IID(IT_RTS);
	Timing (32);

	last_branch = R_PC;
	R_PC = REG(link_reg);

	if ((result = pop(&data)) != CPU_OK)
		return result;

	REG(link_reg) = data;

	return CPU_OK;
}	// OP_RTS

/*
 * Single operand instructions.
 */

/*
 * CLR: Clear Instruction.
 */

static d_word UN_CLR (d_word opd_word) {
	IID(IT_CLR);

	CLR_CC(CC_N|CC_C|CC_V);
	SET_CC(CC_Z);

	return 0;
}	// UN_CLR

/*
 * COM: Complement Instruction.
 */

static d_word UN_COM (d_word opd_word) {
	IID(IT_COM);

	opd_word = ~opd_word;

	calc_NZ (opd_word);
	CLR_CC(CC_V);
	SET_CC(CC_C);

	return opd_word;
}	// UN_COM

/*
 * INC: Increment Instruction.
 */

static d_word UN_INC (d_word opd_word) {
	IID(IT_INC);

	calc_V (opd_word == MPI);
	calc_NZ (++ opd_word);

	return opd_word;
}	// UN_INC

/*
 * DEC: Decrement Instruction.
 */

static d_word UN_DEC (d_word opd_word) {
	IID(IT_DEC);

	calc_V (opd_word == MNI);
	calc_NZ (-- opd_word);

	return opd_word;
}	// UN_DEC

/*
 * NEG: Negate Instruction.
 */

static d_word UN_NEG (d_word opd_word) {
	IID(IT_NEG);

	opd_word = (NEG_1 - opd_word) + 1;

	calc_NZ (opd_word);
	calc_V (opd_word == MNI);
	calc_C (opd_word != 0);

	return opd_word;
}	// UN_NEG

/*
 * ADC: Add Carry Instruction.
 */

static d_word UN_ADC(d_word opd_word) {
	IID(IT_ADC);

	if (TST_CC(CC_C)) {	/* do if carry is set */
		calc_V (opd_word == MPI);
		calc_C (opd_word == NEG_1);
		opd_word ++;			/* add carry */
		} else {
		CLR_CC (CC_V|CC_C);
		}

	calc_NZ (opd_word);
	return opd_word;
}	// UN_ADC

/*
 * SBC: Subtract Carry Instruction.
 */

static d_word UN_SBC(d_word opd_word) {
	IID(IT_SBC);

	calc_V (opd_word == MNI);

	if (TST_CC(CC_C)) {	/* do if carry is set */
		calc_C (opd_word == 0);
		opd_word --;			/* subtract carry */
		} else {
		CLR_CC(CC_C);
		}

	calc_NZ (opd_word);
	return opd_word;
}	// UN_SBC

/*
 * ASL: Arithmetic Shift Left Instruction.
 */

static d_word UN_ASL(d_word opd_word) {
	IID(IT_ASL);

	calc_C ((opd_word & SIGN) != 0);
	opd_word <<= 1;

	calc_NZ(opd_word);
	calc_V_CxN();

	return opd_word;
}	// UN_ASL

/*
 * ASR: Arithmetic Shift Right Instruction.
 */

static d_word UN_ASR(d_word opd_word) {
	IID(IT_ASR);

	calc_C ((opd_word & LSBIT) != 0);
	opd_word = (opd_word >> 1) | (opd_word & SIGN);	/* shift and replicate */

	calc_NZ(opd_word);
	calc_V_CxN();

	return opd_word;
}	// UN_ASR

/*
 * ROL: Rotate Left Instruction.
 */

static d_word UN_ROL(d_word opd_word) {
	IID(IT_ROL);

	d_word newC = opd_word & SIGN;	/* sign bit */
	opd_word <<= 1;			/* shift */

	if (TST_CC(CC_C))		/* roll in carry */
		opd_word |= LSBIT;

	calc_C (newC != 0);
	calc_NZ(opd_word);
	calc_V_CxN();

	return opd_word;
}	// UN_ROL

/*
 * ROR: Rotate Right Instruction.
 */

static d_word UN_ROR(d_word opd_word) {
	IID(IT_ROR);

	d_word newC = opd_word & LSBIT;		/* low bit */
	opd_word >>= 1;			/* shift */

	if (TST_CC(CC_C))		/* roll in carry */
		opd_word |= SIGN;

	calc_C (newC != 0);
	calc_NZ(opd_word);
	calc_V_CxN();

	return opd_word;
}	// UN_ROR

/*
 * SXT: Sign Extend Instruction.
 */

static d_word UN_SXT(d_word opd_word) {
	IID(IT_SXT);

	if (TST_CC(CC_N)) {
		opd_word = NEG_1;
		CLR_CC(CC_Z);
		} else {
		opd_word = 0;
		SET_CC(CC_Z);
		}

	CLR_CC(CC_V);

	return opd_word;
}	// UN_SXT

/*
 * TST: Test Instruction.
 */

static d_word UN_TST(d_word opd_word) {
	IID(IT_TST);

	CLR_CC(CC_C|CC_V);
	calc_NZ (opd_word);

	return UNDEF;
}	// UN_TST

/*
 * TSTB: Test Byte Instruction.
 */

static d_byte UN_TSTB (d_byte opd_byte) {
	IID(IT_TSTB);

	CLR_CC(CC_C|CC_V);
	calcB_NZ (opd_byte);

	return UNDEF;
}	// UN_TSTB

/*
 * CLRB: Clear Byte Instruction.
 */

static d_byte UN_CLRB (d_byte opd_byte) {
	IID(IT_CLRB);

	CLR_CC(CC_N|CC_C|CC_V);
	SET_CC(CC_Z);

  return 0;
}	// UN_CLRB

/*
 * COMB: Complement Byte Instrcution.
 */

static d_byte UN_COMB (d_byte opd_byte) {
	IID(IT_COMB);

	opd_byte = ~opd_byte;

	calcB_NZ (opd_byte);
	CLR_CC(CC_V);
	SET_CC(CC_C);

  return opd_byte;
}	// UN_COMB

/*
 * INCB: Increment Byte Instruction.
 */

static d_byte UN_INCB (d_byte opd_byte) {
	IID(IT_INCB);

	calc_V (opd_byte == MPI_B);

	calcB_NZ (++ opd_byte);
  return opd_byte;
}	// UN_INCB

/*
 * DECB: Decrement Byte Instrcution.
 */

static d_byte UN_DECB (d_byte opd_byte) {
	IID(IT_DECB);

	calc_V (opd_byte == MNI_B);

	calcB_NZ (-- opd_byte);
  return opd_byte;
}	// UN_DECB

/*
 * NEGB: Negate Byte Instruction.
 */

static d_byte UN_NEGB (d_byte opd_byte) {
	IID(IT_NEGB);

	opd_byte = (NEG_1_B - opd_byte) + 1;	/* hope this is right */

	calcB_NZ (opd_byte);

	calc_V (opd_byte == MNI_B);
	calc_C (opd_byte != 0);

	return opd_byte;
}	// UN_NEGB

/*
 * ASLB: Arithmetic Shift Left Byte Instruction.
 */

static d_byte UN_ASLB (d_byte opd_byte) {
	IID(IT_ASLB);

	calc_C ((opd_byte & SIGN_B) != 0);

	opd_byte <<= 1;

	calcB_NZ (opd_byte);
	calc_V_CxN();

	return opd_byte;
}	// UN_ASLB

/*
 * ASRB: Arithmetic Shift Right Byte Instruction.
 */

static d_byte UN_ASRB (d_byte opd_byte) {
	IID(IT_ASRB);

	calc_C ((opd_byte & LSBIT) != 0);

	opd_byte = (opd_byte >> 1) | (opd_byte & SIGN_B);	/* shift and replicate */

	calcB_NZ (opd_byte);
	calc_V_CxN();

	return opd_byte;
}	// UN_ASRB

/*
 * ROLB: Rotate Left Byte Instruction.
 */

static d_byte UN_ROLB (d_byte opd_byte) {
	IID(IT_ROLB);

	d_byte newC = opd_byte & SIGN_B;		/* get top bit */
	opd_byte <<= 1;			/* shift */

	if (TST_CC (CC_C))	/* roll in carry */
		opd_byte |= LSBIT;

	calc_C (newC != 0);
	calcB_NZ (opd_byte);
	calc_V_CxN();

	return opd_byte;
}	// UN_ROLB

/*
 * RORB: Rotate Right Byte Instruction.
 */

static d_byte UN_RORB (d_byte opd_byte) {
	IID(IT_RORB);

	d_byte newC = opd_byte & LSBIT;		/* get low bit */
	opd_byte >>= 1;			/* shift */

	if (TST_CC (CC_C))	/* roll in carry */
		opd_byte |= SIGN_B;

	calc_C (newC != 0);
	calcB_NZ (opd_byte);
	calc_V_CxN();

	return opd_byte;
}	// UN_RORB

/*
 * ADCB: Add Carry Byte Instruction.
 */

static d_byte UN_ADCB (d_byte opd_byte) {
	IID(IT_ADCB);

	if (TST_CC (CC_C)) {		/* do if carry is set */
		calc_V (opd_byte == MPI_B);
		calc_C (opd_byte == NEG_1_B);
		++ opd_byte;			/* add the carry */
		} else {
		CLR_CC(CC_V|CC_C);
		}

	calcB_NZ (opd_byte);

	return opd_byte;
}	// UN_ADCB

/*
 * SBCB: Subtract Carry Byte Instruction.
 */

static d_byte UN_SBCB (d_byte opd_byte) {
	IID(IT_SBCB);

	if (TST_CC (CC_C)) {		/* do if carry is set */
		calc_C (opd_byte == 0);
		-- opd_byte;			/* subtract carry */
		} else {
		CLR_CC(CC_C);
		}

	calc_V (opd_byte == MNI_B);
	calcB_NZ (opd_byte);

	return opd_byte;
}	// UN_SBCB

/*
 * MFPS: Move from Processor Status Instruction.
 */

static d_byte UN_MFPS (d_byte opd_byte) {
	IID(IT_MFPS);

	opd_byte = LOW8(R_PSW);

	calcB_NZ (opd_byte);
	CLR_CC(CC_V);

	return opd_byte;
}	// UN_MFPS

/*
 * MTPS: Move to Processor Status Instruction.
 */

static d_byte UN_MTPS (d_byte opd_byte) {
	IID(IT_MTPS);

	R_PSW &= bkmodel ? ~0357 : ~0217;
	R_PSW += (opd_byte & (bkmodel ? 0357 : 0217));

	return 0;
}	// UN_MTPS

/*
 * SWAB: Swap Bytes Instruction.
 */

static d_word UN_SWAB(d_word opd_word) {
	IID(IT_SWAB);

	opd_word = ((opd_word << 8) & 0xff00) | ((opd_word >> 8) & 0x00ff);

	CLR_CC(CC_V|CC_C);
	calcB_NZ (opd_word);	/* checks done on low byte only */

	return opd_word;
}	// UN_SWAB

/*
 * Double operand instrcutions.
 */

/*
 * MOV: Move Instruction.
 */

static d_word BIN_MOV (d_word src_word, d_word dst_word) {
	IID(IT_MOV);

	CLR_CC(CC_V);
	calc_NZ (src_word);

	return src_word;
}	// BIN_MOV

/*
 * CMP: Compare Instruction.
 */

static d_word BIN_CMP (d_word src_word, d_word dst_word) {
	IID(IT_CMP);

	calc_VC (src_word, dst_word);
	calc_NZ (src_word - dst_word);
	return UNDEF;
}	// BIN_CMP

/*
 * ADD: Add Instruction.
 */

static d_word BIN_ADD (d_word src_word, d_word dst_word) {
	IID(IT_ADD);

	dst_word += src_word;
	calc_VC (dst_word, src_word);
	calc_NZ (dst_word);
	return dst_word;
}	// BIN_ADD

/*
 * SUB: Subtract Instruction.
 */

static d_word BIN_SUB (d_word src_word, d_word dst_word) {
	IID(IT_SUB);

	calc_VC (dst_word, src_word);
	dst_word -= src_word;
	calc_NZ (dst_word);
	return dst_word;
}	// BIN_SUB

/*
 * BIT: Bit Test Instruction.
 */

static d_word BIN_BIT (d_word src_word, d_word dst_word) {
	IID(IT_BIT);

	calc_NZ (src_word & dst_word);
	CLR_CC(CC_V);

	return UNDEF;
}	// BIN_BIT

/*
 * BIC: Bit Clear Instruction.
 */

static d_word BIN_BIC (d_word src_word, d_word dst_word) {
	IID(IT_BIC);

	dst_word &= ~src_word;
	calc_NZ (dst_word);
	CLR_CC(CC_V);

	return dst_word;
}	// BIN_BIC

/*
 * BIS: Bit Set Instruction.
 */

static d_word BIN_BIS (d_word src_word, d_word dst_word) {
	IID(IT_BIS);

	dst_word |= src_word;

	calc_NZ (dst_word);
	CLR_CC(CC_V);

	return dst_word;
}	// BIN_BIS

/*
 * MOVB: Move Byte Instruction.
 */

static d_byte BIN_MOVB (d_byte src_byte, d_byte dst_byte) {
	IID(IT_MOVB);

	calcB_NZ(src_byte);
	CLR_CC(CC_V);

	return src_byte;
}	// BIN_MOVB

/*
 * CMPB: Compare Byte Instruction.
 */

static d_byte BIN_CMPB (d_byte src_byte, d_byte dst_byte) {
	IID(IT_CMPB);

	calcB_VC (src_byte, dst_byte);
	calcB_NZ (src_byte - dst_byte);

	return UNDEF;
}	// BIN_CMPB

/*
 * BITB: Bit Test Byte Instruction.
 */

static d_byte BIN_BITB (d_byte src_byte, d_byte dst_byte) {
	IID(IT_BITB);

	calcB_NZ(src_byte & dst_byte);
	CLR_CC(CC_V);

	return UNDEF;
}	// BIN_BITB

/*
 * BICB: Bit Clear Byte Instruction.
 */

static d_byte BIN_BICB (d_byte src_byte, d_byte dst_byte) {
	IID(IT_BICB);

	dst_byte &= ~src_byte;

	calcB_NZ(dst_byte);
	CLR_CC(CC_V);

	return dst_byte;
}	// BIN_BICB

/*
 * BISB: Bit Set Byte Instruction.
 */

static d_byte BIN_BISB (d_byte src_byte, d_byte dst_byte) {
	IID(IT_BISB);

	dst_byte |= src_byte;

	calcB_NZ(dst_byte);
	CLR_CC(CC_V);

	return dst_byte;
}	// BIN_BISB

/*
 * Control instructions
 */

static CPU_RES OP_RESET (d_word IR) {
	IID(IT_RESET);

	q_reset();

	Timing (time_WAIT);
	R_PSW = 0200; 
	return CPU_OK;
}	// CPU_RESET

static CPU_RES OP_WAIT (d_word IR) {
	IID(IT_WAIT);

	Timing (time_WAIT);
	R_PC -= 2;		/* repeat instruction */
	return CPU_WAIT;
}	// OP_WAIT

static CPU_RES OP_HALT (d_word IR) {
	IID(IT_HALT);

	io_stop_happened = 4;
	return CPU_HALT;
}	// OP_HALT

static CPU_RES OP_FIS (d_word IR)
	{ IID(IT__ILL); Timing (144); return CPU_ILLEGAL; }		/* FIS would be fun! */

static CPU_RES OP_ILL (d_word IR)
	{ IID(IT__ILL); Timing (144); return CPU_ILLEGAL; }

/*
 * OP_MARK: Restore stack and jump.
 */

static int OP_MARK (d_word IR) {
	IID(IT_MARK); 

	d_word data;
	CPU_RES result;

	Timing (36);

	last_branch = R_PC;
	R_SP = R_PC + (IR & 077) * 2;
	R_PC = REG(R5);
  if ((result = pop(&data)) != CPU_OK)
		return result;

  REG(R5) = data;

  return CPU_OK;
}	// OP_MARK

/*
 * SOB - Subtract One and Branch Instruction.
 */

static CPU_RES OP_SOB (d_word IR) {
	IID(IT_SOB);

	last_branch = R_PC;

	Timing (20);

	if (-- REG(SRC_REG(IR))) {
		R_PC -= (IR & 077) * 2;
	}

	return CPU_OK;
}	// OP_SOB

/*
 * ASH: Arithmetic Shift Instruction.
 */

d_word RBIN_ASH (int opd_reg, d_word opd_word) {
	d_word temp;
	d_word old;
	d_word sign;
	d_word shift = opd_word;
	unsigned count;
	IID(IT_ASH);
	Timing (REGREG + a2_time[_proc->ir]);  // arbitrarily

	temp = REG(opd_reg);

	old = temp;

	if (( shift & 077 ) == 0 ) {	/* no shift */
		calc_NZ( temp );
		CLR_CC(CC_V);
		return old;
	}

	if ( shift & 040 ) {		/* right shift */
		count = 0100 - ( shift & 077 );
		sign = temp & SIGN;
		while ( count-- ) {
			if ( temp & LSBIT ) {
				SET_CC(CC_C);
			} else {
				CLR_CC(CC_C);
			}
			temp >>= 1;
			temp += sign;
		}
	} else {			/* left shift */
		count = shift & 037;
		while ( count-- ) {
			if ( temp & SIGN ) {
				SET_CC(CC_C);
			} else {
				CLR_CC(CC_C);
			}
			temp <<= 1;
		}
	}
		
	calc_NZ( temp );

	if (( old & SIGN ) == ( temp & SIGN )) {
		CLR_CC(CC_V);
	} else {
		SET_CC(CC_V);
	}

	REG(opd_reg) = temp;

	return UNDEF;
}		// RBIN_ASH

/*
 * ASHC: Arithmetic Shift Combined Instruction.
 */

d_word RBIN_ASHC (int opd_reg, d_word opd_word) {
	unsigned long temp;
	unsigned long old;
	unsigned long sign;
	unsigned count;
	d_word shift = opd_word;

	IID(IT_ASHC);
	Timing (2*REGREG + a2_time[_proc->ir]);  // arbitrarily

	temp = REG(opd_reg);
	temp <<= 16;
	temp += REG(opd_reg | 1);

	old = temp;

	if (( shift & 077 ) == 0 ) {	/* no shift */

		CLR_CC(CC_V);

		if ( temp & 0x80000000 ) {
			SET_CC(CC_N);
		} else {
			CLR_CC(CC_N);
		}
	
		if ( temp ) {
			CLR_CC(CC_Z);
		} else {
			SET_CC(CC_Z);
		}

		return UNDEF;
	}
	if ( shift & 040 ) {		/* right shift */
		count = 0100 - ( shift & 077 );
		sign = temp & 0x80000000;
		while ( count-- ) {
			if ( temp & LSBIT ) {
				SET_CC(CC_C);
			} else {
				CLR_CC(CC_C);
			}
			temp >>= 1;
			temp += sign;
		}
	} else {			/* left shift */
		count = shift & 037;
		while ( count-- ) {
			if ( temp & 0x80000000 ) {
				SET_CC(CC_C);
			} else {
				CLR_CC(CC_C);
			}
			temp <<= 1;
		}
	}
		
	if ( temp & 0x80000000 )
		SET_CC(CC_N);
	else
		CLR_CC(CC_N);

	if ( temp )
		CLR_CC(CC_Z);
	else
		SET_CC(CC_Z);

	if (( old & 0x80000000 ) == ( temp & 0x80000000 )) {
		CLR_CC(CC_V);
	} else {
		SET_CC(CC_V);
	}

	REG(opd_reg) = (temp >> 16);
	REG(opd_reg | 1) = LOW16( temp );

	return UNDEF;
}		// RBIN_ASHC

union s_u_word {
	d_word u_word;
	short s_word;
};

union s_u_long {
	unsigned long u_long;
	long s_long;
};

/*
 * MUL: Multiply Instruction.
 */

d_word RBIN_MUL (int opd_reg, d_word opd_word) {
	union s_u_word data1;
	union s_u_word data2;
	union s_u_long tmp;
 
	IID(IT_MUL);
	Timing (5*REGREG + a2_time[_proc->ir]);  // arbitrarily

	data1.u_word = REG(opd_reg);
	data2.u_word = opd_word;

	tmp.s_long = ((long) data1.s_word) * ((long) data2.s_word );

	REG(opd_reg) = (tmp.u_long >> 16);
	REG(opd_reg | 1) = (tmp.u_long & 0177777);

	CLR_CC(CC_V);
	CLR_CC(CC_C);

	if ( tmp.u_long == 0 )
		SET_CC(CC_Z);
	else
		CLR_CC(CC_Z);

	if ( tmp.u_long & 0x80000000 )
		SET_CC(CC_N);
	else
		CLR_CC(CC_N);

	return UNDEF;
}		// RBIN_MUL

/*
 * DIV: Divide Instruction.
 */

d_word RBIN_DIV (int opd_reg, d_word opd_word) {
	union s_u_long tmp;
	union s_u_long eql;
	union s_u_word data2;

	IID(IT_DIV);
	Timing (10*REGREG + a2_time[_proc->ir]);  // arbitrarily

	tmp.u_long = REG(opd_word);
	tmp.u_long = tmp.u_long << 16;
	tmp.u_long  += REG(opd_word | 1);

	data2.u_word = opd_word;

	if ( data2.u_word == 0 ) {
		SET_CC(CC_C);
		SET_CC(CC_V);
		return UNDEF;
	} else {
		CLR_CC(CC_C);
	}

	eql.s_long = tmp.s_long / data2.s_word;
	REG(opd_reg) = eql.u_long & 0177777;

	if ( eql.u_long == 0 )
		SET_CC(CC_Z);
	else
		CLR_CC(CC_Z);

	if (( eql.s_long > 077777) || ( eql.s_long < -0100000))
		SET_CC(CC_V);
	else
		CLR_CC(CC_V);

	if ( eql.s_long < 0 )
		SET_CC(CC_N);
	else
		CLR_CC(CC_N);

	eql.s_long = tmp.s_long % data2.s_word;
	REG(opd_reg | 1) = eql.u_long & 0177777;

	return UNDEF;
}		// RBIN_DIV

/*
 * XOR: Exclusive OR Instruction
 */

static d_word RBIN_XOR (int opd_reg, d_word opd_word) {
	IID(IT_XOR);

	Timing (REGREG + a2_time[_proc->ir]);

	opd_word ^= REG(opd_reg);

	calc_NZ (opd_word);
	CLR_CC(CC_V);

	return opd_word;
}		// RBIN_XOR


/*
 * cpu_service() - Service interrupt
 */

CPU_RES cpu_service (d_word vector) {
	CPU_RES result;
	d_word oldpsw;
	d_word oldpc;
	d_word oldmode;

	last_branch = R_PC;
	oldmode = ( R_PSW & 0140000 ) >> 14;

	oldpsw = R_PSW;
	oldpc = R_PC;

	/* If we're servicing an interrupt while a WAIT instruction
	 * was executing, we need to return after the WAIT.
	 */
	if (in_wait_instr) {
		oldpc += 2;
		in_wait_instr = 0;
	}

	if ((result = lc_word( vector, &(R_PC))) != CPU_OK)
		return result;
	if ((result = lc_word( vector + 2, &(R_PSW))) != CPU_OK)
		return result;

	if ((result = push(oldpsw)) != CPU_OK)
		return result;
	if ((result = push(oldpc)) != CPU_OK)
		return result;

	return CPU_OK;
}	// cpu_service

/*
 * RTI: Return from Interrupt Instruction.
 */

static CPU_RES OP_RTI (d_word IR) {
	IID(IT_RTI);

	d_word newpsw;
	d_word newpc;
	CPU_RES result;

	Timing (time_RTI);

	last_branch = R_PC;
	if ((result = pop(&newpc)) != CPU_OK)
		return result;
	if ((result = pop(&newpsw)) != CPU_OK)
		return result;

	R_PSW = newpsw;
	R_PC = newpc;

	return CPU_RTI;
}	// OP_RTI

/*
 * RTT: Return from Trap Instruction.
 */

static CPU_RES OP_RTT (d_word IR) {
	IID(IT_RTT);

	d_word newpsw;
	d_word newpc;
	CPU_RES result;

	Timing (time_RTI);

	last_branch = R_PC;
	if ((result = pop(&newpc)) != CPU_OK)
		return result;
	if ((result = pop(&newpsw)) != CPU_OK)
		return result;

	R_PSW = newpsw;
	R_PC = newpc;

	return CPU_RTT;
}	// OP_RTT

// TRAP or EMT Instructions.
static CPU_RES OP_TRAP (d_word IR) {
	Timing (time_INT);
	return IR & 0x100 ? (IID(IT_TRAP), CPU_TRAP) : (IID(IT_EMT), CPU_EMT);
}	// OP_TRAP

// Input-Output Trap Instruction
static CPU_RES OP_IOT (d_word IR) { IID(IT_IOT); Timing (time_INT); return CPU_IOT; }

// Break-Point Trap Instruction
static CPU_RES OP_BPT (d_word IR) { IID(IT_BPT); Timing (time_INT); return CPU_BPT; }

// Operand (AOR) access flags
enum {
	opR = 1,				// Read only
	opW = 2,				// Write only
	opRW = 3,				// Read/write

	opSxt = 1 << 5	// Sign extend, when destination is register
	};

// Flag code for binary operation
#define	opBin(src,dst,byte)			\
		(((byte) << 4) | ((src) << 2) | (dst))

// Get source bits of flag
#define	opSrc(flag)		((flag) >> 2)

// Get destination bits of flag
#define	opDst(flag)		(flag)

// Check 'byte' flag
#define	opByte(flag)	(flag & (1 << 4))

#define	opUn(opd,byte)			opBin(opd,opd,byte)

// Word unary operations
static d_word (* UN_list[]) (d_word) = {
	UN_CLR,				/* 0050: CLR */
	UN_COM,				/* 0051: COM */
	UN_INC,				/* 0052: INC */
	UN_DEC,				/* 0053: DEC */
	UN_NEG,				/* 0054: NEG */
	UN_ADC,				/* 0055: ADC */
	UN_SBC,				/* 0056: SBC */
	UN_TST,				/* 0057: TST */
	UN_ROR,				/* 0060: ROR */
	UN_ROL,				/* 0061: ROL */
	UN_ASR,				/* 0062: ASR */
	UN_ASL,				/* 0063: ASL */
	0,							/* 0064 */
	0,							/* 0065 */
	0,							/* 0066 */
	UN_SXT,				/* 0067: SXT */

	UN_SWAB				/* 0003: SWAB */
	};

static int UN_flag[] = {
	opUn(opW, 0),						/* 0050: CLR */
	opUn(opRW, 0),					/* 0051: COM */
	opUn(opRW, 0),					/* 0052: INC */
  opUn(opRW, 0),					/* 0053: DEC */
  opUn(opRW, 0),					/* 0054: NEG */
	opUn(opRW, 0),					/* 0055: ADC */
	opUn(opRW, 0),					/* 0056: SBC */
	opUn(opR, 0),						/* 0057: TST */
	opUn(opRW, 0),					/* 0060: ROR */
	opUn(opRW, 0),					/* 0061: ROL */
	opUn(opRW, 0),					/* 0062: ASR */
	opUn(opRW, 0),					/* 0063: ASL */
	0,											/* 0064 */
	0,											/* 0065 */
	0,											/* 0066 */
	opUn(opW, 0),						/* 0067: SXT */

	opUn(opRW, 0)						/* 0003: SWAB */
	};

// Byte unary operations
static d_byte (* UNB_list[]) (d_byte) = {
	UN_CLRB,				/* 1050: CLRB */
	UN_COMB,				/* 1051: COMB */
	UN_INCB,				/* 1052: INCB */
	UN_DECB,				/* 1053: DECB */
	UN_NEGB,				/* 1054: NEGB */
	UN_ADCB,				/* 1055: ADCB */
	UN_SBCB,				/* 1056: SBCB */
	UN_TSTB,				/* 1057: TSTB */
	UN_RORB,				/* 1060: RORB */
	UN_ROLB,				/* 1061: ROLB */
	UN_ASRB,				/* 1062: ASRB */
	UN_ASLB,				/* 1063: ASLB */
	UN_MTPS,				/* 1064: MTPS */
	0,
	0,
	UN_MFPS					/* 1067: MFPS */
	};

static int UNB_flag[] = {
	opUn(opW, 1),						/* 1050: CLRB */
	opUn(opRW, 1),					/* 1051: COMB */
	opUn(opRW, 1),					/* 1052: INCB */
  opUn(opRW, 1),					/* 1053: DECB */
  opUn(opRW, 1),					/* 1054: NEGB */
	opUn(opRW, 1),					/* 1055: ADCB */
	opUn(opRW, 1),					/* 1056: SBCB */
	opUn(opR, 1),						/* 1057: TSTB */
	opUn(opRW, 1),					/* 1060: RORB */
	opUn(opRW, 1),					/* 1061: ROLB */
	opUn(opRW, 1),					/* 1062: ASRB */
	opUn(opRW, 1),					/* 1063: ASLB */
	opUn(opR, 0),						/* 1064: MTPS */
	0,
	0,
	opUn(opW, 0)						/* 1067: MFPS */
	};

// General unary instruction
static CPU_RES OP_UN(d_word IR) {
	AOR_t opd_AOR;
	d_byte opd_byte;
	d_word opd_word;
	CPU_RES result;

	int is_byte = IR & 0100000;
	unsigned index = (IR - (is_byte ? 0105000 : 0005000)) >> 6;

	switch (IR & 0177700) {
	case 0006400:
			return OP_MARK(IR);

	case 0000300:			// (SWAB)
			index = 16;
			break;
	}

	int flags = (is_byte ? UNB_flag : UN_flag) [index];

	if (! flags) return CPU_ILLEGAL;

	if (opSrc(flags) & opW) {
		Timing (REGREG + ab_time[DST_MODE(IR)]);
		}
	else {
		Timing (REGREG + a1_time[DST_MODE(IR)]);
		}

	// Optimize: (R) operations
	if (! (IR & 00070)) {
		int opd_reg = IR & 7;

		if (is_byte) {
			d_byte opd_byte = UNB_list[index] ((d_byte) REG(opd_reg));

			if (opSrc(flags) & opW) {
				if (flags & opSxt)
				REG(opd_reg) = opd_byte & 0x80 ? ((d_word) opd_byte | 0xFF00) : opd_byte;
				else
				{ REG(opd_reg) &= 0xFF00; REG(opd_reg) |= opd_byte; }
			}
		}

		else {
			d_word opd_word = UN_list[index] (REG(opd_reg));

			if (opSrc(flags) & opW)
				REG(opd_reg) = opd_word;
		}

		return CPU_OK;
	}		// (R) operations

	if ((result = calc_AOR (IR, is_byte, &opd_AOR)))
		return result;

	if (opSrc(flags) & opR)
		if ((result = is_byte ?
			ldbyte_AOR(opd_AOR, &opd_byte) :
			ldword_AOR(opd_AOR, &opd_word) ) != CPU_OK)
				return result;

	if (is_byte)
			opd_byte = UNB_list[index] (opd_byte);
	else
			opd_word = UN_list[index] (opd_word);

	if (opSrc(flags) & opW)
		if ((result = is_byte ?
			stbyte_AOR(opd_AOR, opd_byte) :
			stword_AOR(opd_AOR, opd_word)) != CPU_OK)
				return result;

return CPU_OK;
}		// OP_UN

// Byte binary operations
static d_byte (* BINB_list[]) (d_byte, d_byte) = {
	0,						/* 00 */
	0,						/* 01 */
	0,						/* 02 */
  0,						/* 03 */
  0,						/* 04 */
  0,						/* 05 */
  0,						/* 06 */
	0,						/* 07 */

	0,						/* 10 */
	BIN_MOVB,		/* 11: MOVB */
	BIN_CMPB,		/* 12: CMPB */
  BIN_BITB,		/* 13: BITB */
  BIN_BICB,		/* 14: BICB */
  BIN_BISB,		/* 15: BISB */
  0,						/* 16 */
	0,						/* 17 */
	};

// Word binary operations
static d_word (* BIN_list[]) (d_word, d_word) = {
	0,						/* 00 */
	BIN_MOV,			/* 01: MOV */
	BIN_CMP,			/* 02: CMP */
  BIN_BIT,			/* 03: BIT */
  BIN_BIC,			/* 04: BIC */
  BIN_BIS,			/* 05: BIS */
  BIN_ADD,			/* 06: ADD */
	0,						/* 07 */

	0,						/* 10 */
	0,						/* 11 */
	0,						/* 12 */
  0,						/* 13 */
  0,						/* 14 */
  0,						/* 15 */
  BIN_SUB,			/* 16: SUB */
	0,						/* 17 */
	};

static int BIN_flag[] = {
	0,													/* 00 */
	opBin(opR, opW,  0),				/* 01: MOV */
	opBin(opR, opR,  0),				/* 02: CMP */
  opBin(opR, opR,  0),				/* 03: BIT */
  opBin(opR, opRW, 0),				/* 04: BIC */
  opBin(opR, opRW, 0),				/* 05: BIS */
  opBin(opR, opRW, 0),				/* 06: ADD */
	0,													/* 07 */

	0,													/* 10 */
	opBin(opR, opW,  1)|opSxt,	/* 11: MOVB */
	opBin(opR, opR,  1),				/* 12: CMPB */
  opBin(opR, opR,  1),				/* 13: BITB */
  opBin(opR, opRW, 1),				/* 14: BICB */
  opBin(opR, opRW, 1),				/* 15: BISB */
  opBin(opR, opRW, 0),				/* 16: SUB */
	0,													/* 17 */
	};

// General binary instruction
static CPU_RES OP_BIN(d_word IR) {
	AOR_t src_AOR, dst_AOR;
	d_byte src_byte, dst_byte;
	d_word src_word, dst_word;
	CPU_RES result;

	unsigned index = IR >> 12;
	int flags = BIN_flag[index];
	int is_byte = opByte(flags);

	// Timing

	if (opDst(flags) & opW)
		Timing (REGREG + a_time[SRC_MODE(IR)] + (SRC_MODE(IR) ? ab_time : b_time)[DST_MODE(IR)]);
	else
		Timing (REGREG + a1_time[SRC_MODE(IR)] + (SRC_MODE(IR) ? a1_time : a2_time)[DST_MODE(IR)]);

	// Optimize: (R, R) operations
	if (! (IR & 07070)) {
		int src_reg = (IR >> 6) & 7, dst_reg = IR & 7;

		if (is_byte) {
			d_byte dst_byte = BINB_list[index] ((d_byte) REG(src_reg), (d_byte) REG(dst_reg));

			if (opDst(flags) & opW) {
				if (flags & opSxt)
					REG(dst_reg) = dst_byte & 0x80 ? ((d_word)dst_byte | 0xFF00) : dst_byte;
				else
				{ REG(dst_reg) &= 0xFF00; REG(dst_reg) |= dst_byte; }
			}
		}

		else {
			d_word dst_word = BIN_list[index] (REG(src_reg), REG(dst_reg));

			if (opDst(flags) & opW)
				{ REG(dst_reg) = dst_word; }
		}

		return CPU_OK;
	}		// (R, R) operations

	if ((result = calc_AOR (IR >> 6, is_byte, &src_AOR)) != CPU_OK
   || (result = calc_AOR (IR, is_byte, &dst_AOR)) != CPU_OK)
		return result;

	if (opSrc(flags) & opR)
		if ((result = is_byte ?
			ldbyte_AOR(src_AOR, &src_byte):
			ldword_AOR(src_AOR, &src_word)) != CPU_OK)
				return result;

	if (opDst(flags) & opR)
		if ((result = is_byte ?
			ldbyte_AOR(dst_AOR, &dst_byte):
			ldword_AOR(dst_AOR, &dst_word)) != CPU_OK)
				return result;

	if (is_byte)
			dst_byte = BINB_list[index] (src_byte, dst_byte);
	else
			dst_word = BIN_list[index] (src_word, dst_word);

	if (opDst(flags) & opW) {
		if (is_byte && (flags & opSxt) && !(IR & 0000070)) {
			dst_word = dst_byte & 0x80 ? ((d_word)dst_byte | 0xFF00) : dst_byte;
			REG(IR & 07) = dst_word;
			return CPU_OK;
			}

		if ((result = is_byte ?
			stbyte_AOR(dst_AOR, dst_byte):
			stword_AOR(dst_AOR, dst_word)) != CPU_OK)
				return result;
	}

return CPU_OK;
}		// OP_BIN

static d_word (* OPBINR_list []) (int opd_reg, d_word) = {
	RBIN_MUL,		/* 070000: MUL/illegal */
	RBIN_DIV,		/* 071000: DIV/illegal */
	RBIN_ASH,		/* 072000: ASH/illegal */
	RBIN_ASHC,		/* 073000: ASHC/illegal */
	RBIN_XOR		/* 074000: XOR */
	};

static int OPBINR_flag[] = {
#ifdef EIS_ALLOWED
	opR,				/* 070000: MUL (illegal) */
	opR,				/* 071000: DIV (illegal) */
	opR,				/* 072000: ASH (illegal) */
	opR,				/* 073000: ASHC (illegal) */
#else
	0,
	0,
#ifdef SHIFTS_ALLOWED
	opR,
	opR,
#else
	0,
	0,
#endif
#endif
	opRW				/* 074000: XOR */
	};

// Register binary
static CPU_RES OP_BINR (d_word IR) {
	AOR_t opd_AOR;
	d_word opd_data;
	CPU_RES result;

	unsigned index = (IR - 070000) >> 9;
	int flags = OPBINR_flag [index];

	if (! flags) return CPU_ILLEGAL;

	int opd_reg = (IR >> 6) & 7;

  if ((result = calc_AOR (IR, 0, &opd_AOR)) != CPU_OK) {
		return result;
  	}

	if (flags & opR)
		if ((result = ldword_AOR (opd_AOR, &opd_data)) != CPU_OK)
			return result;

	opd_data = OPBINR_list[index] (opd_reg, opd_data);

	if (flags & opW)
		if ((result = stword_AOR (opd_AOR, opd_data)) != CPU_OK)
			return result;

	return CPU_OK;
}		// OP_RBIN

static bool (* BR_list []) (d_word psw) = {
		0,					/* 0000000: (unused) */
		BR_A,				/* 0000400: BR */
		BR_NE,			/* 0001000: BNE */
		BR_EQ,			/* 0001400: BEQ */
		BR_GE,			/* 0002000: BGE */
		BR_LT,			/* 0002400: BLT */
		BR_GT,			/* 0003000: BGT */
		BR_LE,			/* 0003400: BLE */

		BR_PL,			/* 0100000: BPL */
		BR_MI,			/* 0100400: BMI */
		BR_HI,			/* 0101000: BHI */
		BR_LOS,			/* 0101400: BLOS */
		BR_VC,			/* 0102000: BVC */
		BR_VS,			/* 0102400: BVS */
		BR_CC,			/* 0103000: BCC */
		BR_CS				/* 0103400: BCS */
		};

static CPU_RES OP_BR (d_word IR) {
	d_word offset;

	last_branch = R_PC;

	Timing (16);

	if (BR_list [((IR >> 8) & 7) | (IR & 0x8000 ? 8 : 0)] (R_PSW)) {
		offset = LOW8(IR);
		if ( offset & SIGN_B )
			offset += 0177400 ;
		R_PC += offset * 2;
		}

	return CPU_OK;
}	// OP_BR

typedef CPU_RES (* _pfunc) (d_word IR);

static _pfunc sitab0[] = {
	OP_HALT, OP_WAIT, OP_RTI, OP_BPT, OP_IOT, OP_RESET, OP_RTT, OP_ILL
	};

static CPU_RES OP_000 (d_word IR) {
int index = (IR >> 6) & 7;

if (index >= 4) return OP_BR(IR);

switch (index) {
	case 0:
			switch (IR & 070) {
			case 000:		return (sitab0 [IR&07]) (IR);
			case 010:		return OP_HALT (IR);
			default:		return CPU_ILLEGAL;
			}

	case 1:					// (JMP)
			return OP_JMP(IR);

	case 2:
			switch (IR & 070) {
			case 000:			return OP_RTS(IR);
			case 040:
			case 050:			return OP_CCC(IR);
			case 060:
			case 070:			return OP_SCC(IR);

			default:			return CPU_ILLEGAL;
			}

	case 3:						// (SWAB)
			return OP_UN(IR);
	}
	return CPU_ILLEGAL;
}	// OP_000

/*
 * Instruction decode table.
 */

static _pfunc instr_table[128] = {
	/*000*/	OP_000,
	/*001*/	OP_BR,
	/*002*/	OP_BR,
	/*003*/	OP_BR,
	/*004*/	OP_JSR,
	/*005*/	OP_UN,
	/*006*/	OP_UN,
	/*007*/	OP_ILL,
	/*010*/	OP_BIN,
	/*011*/	OP_BIN,
	/*012*/	OP_BIN,
	/*013*/	OP_BIN,
	/*014*/	OP_BIN,
	/*015*/	OP_BIN,
	/*016*/	OP_BIN,
	/*017*/	OP_BIN,
	/*020*/	OP_BIN,
	/*021*/	OP_BIN,
	/*022*/	OP_BIN,
	/*023*/	OP_BIN,
	/*024*/	OP_BIN,
	/*025*/	OP_BIN,
	/*026*/	OP_BIN,
	/*027*/	OP_BIN,
	/*030*/	OP_BIN,
	/*031*/	OP_BIN,
	/*032*/	OP_BIN,
	/*033*/	OP_BIN,
	/*034*/	OP_BIN,
	/*035*/	OP_BIN,
	/*036*/	OP_BIN,
	/*037*/	OP_BIN,
	/*040*/	OP_BIN,
	/*041*/	OP_BIN,
	/*042*/	OP_BIN,
	/*043*/	OP_BIN,
	/*044*/	OP_BIN,
	/*045*/	OP_BIN,
	/*046*/	OP_BIN,
	/*047*/	OP_BIN,
	/*050*/	OP_BIN,
	/*051*/	OP_BIN,
	/*052*/	OP_BIN,
	/*053*/	OP_BIN,
	/*054*/	OP_BIN,
	/*055*/	OP_BIN,
	/*056*/	OP_BIN,
	/*057*/	OP_BIN,
	/*060*/	OP_BIN,
	/*061*/	OP_BIN,
	/*062*/	OP_BIN,
	/*063*/	OP_BIN,
	/*064*/	OP_BIN,
	/*065*/	OP_BIN,
	/*066*/	OP_BIN,
	/*067*/	OP_BIN,
	/*070*/	OP_BINR,
	/*071*/	OP_BINR,
	/*072*/	OP_BINR,
	/*073*/	OP_BINR,
	/*074*/	OP_BINR,
	/*075*/	OP_ILL,
	/*076*/	OP_ILL,
	/*077*/	OP_SOB,

	/*100*/	OP_BR,
	/*101*/	OP_BR,
	/*102*/	OP_BR,
	/*103*/	OP_BR,
	/*104*/	OP_TRAP,
	/*105*/	OP_UN,
	/*106*/	OP_UN,
	/*107*/	OP_ILL,
	/*110*/	OP_BIN,
	/*111*/	OP_BIN,
	/*112*/	OP_BIN,
	/*113*/	OP_BIN,
	/*114*/	OP_BIN,
	/*115*/	OP_BIN,
	/*116*/	OP_BIN,
	/*117*/	OP_BIN,
	/*120*/	OP_BIN,
	/*121*/	OP_BIN,
	/*122*/	OP_BIN,
	/*123*/	OP_BIN,
	/*124*/	OP_BIN,
	/*125*/	OP_BIN,
	/*126*/	OP_BIN,
	/*127*/	OP_BIN,
	/*130*/	OP_BIN,
	/*131*/	OP_BIN,
	/*132*/	OP_BIN,
	/*133*/	OP_BIN,
	/*134*/	OP_BIN,
	/*135*/	OP_BIN,
	/*136*/	OP_BIN,
	/*137*/	OP_BIN,
	/*140*/	OP_BIN,
	/*141*/	OP_BIN,
	/*142*/	OP_BIN,
	/*143*/	OP_BIN,
	/*144*/	OP_BIN,
	/*145*/	OP_BIN,
	/*146*/	OP_BIN,
	/*147*/	OP_BIN,
	/*150*/	OP_BIN,
	/*151*/	OP_BIN,
	/*152*/	OP_BIN,
	/*153*/	OP_BIN,
	/*154*/	OP_BIN,
	/*155*/	OP_BIN,
	/*156*/	OP_BIN,
	/*157*/	OP_BIN,
	/*160*/	OP_BIN,
	/*161*/	OP_BIN,
	/*162*/	OP_BIN,
	/*163*/	OP_BIN,
	/*164*/	OP_BIN,
	/*165*/	OP_BIN,
	/*166*/	OP_BIN,
	/*167*/	OP_BIN,
	/*170*/	OP_FIS,
	/*171*/	OP_FIS,
	/*172*/	OP_FIS,
	/*173*/	OP_FIS,
	/*174*/	OP_FIS,
	/*175*/	OP_FIS,
	/*176*/	OP_FIS,
	/*177*/	OP_FIS
	};

/* Execute instruction */
CPU_RES cpu_instruction () {
	d_word IR;

	CPU_RES	result = ll_word(_proc, (R_PC += 2) - 2, &IR);
	if (result == CPU_OK) {
			_proc->ir = IR;
			result = instr_table [IR >> 9] (IR);
			}

	return result;
}		// cpu_instruction

/*
 * cpu_init() - Initialize the cpu registers.
 */

void cpu_init(pdp_regs *p) {
	_proc = p;

	unsigned reg;
	for (reg = 0; reg != 8; ++ reg) {
		p->regs[reg] = 0;
	}

	p->ir = 0;
	p->psw = 0200;
}	// cpu_init

