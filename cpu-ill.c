#include "defines.h"

/*
 * TODO: CPU illegal instructions
 */

/*
 * ASH: Arithmetic Shift Instruction.
 */

d_word OPR_ASH (pdp_regs *p, int opd_reg, d_word opd_word) {
	d_word temp;
	d_word old;
	d_word sign;
	d_word shift = opd_word;
	unsigned count;

	temp = p->regs[opd_reg];

	old = temp;

	if (( shift & 077 ) == 0 ) {	/* no shift */
		CHG_CC_N( temp );
		CHG_CC_Z( temp );
		CLR_CC_V();
		return CPU_OK;
	}

	if ( shift & 040 ) {		/* right shift */
		count = 0100 - ( shift & 077 );
		sign = temp & SIGN;
		while ( count-- ) {
			if ( temp & LSBIT ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp >>= 1;
			temp += sign;
		}
	} else {			/* left shift */
		count = shift & 037;
		while ( count-- ) {
			if ( temp & SIGN ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp <<= 1;
		}
	}
		
	CHG_CC_N( temp );
	CHG_CC_Z( temp );

	if (( old & SIGN ) == ( temp & SIGN )) {
		CLR_CC_V();
	} else {
		SET_CC_V();
	}

	p->regs[opd_reg] = temp;

	return CPU_OK;
}		// OPR_ASH

/*
 * ASHC: Arithmetic Shift Combined Instruction.
 */

d_word OPR_ASHC (pdp_regs *p, int opd_reg, d_word opd_word) {
	unsigned long temp;
	unsigned long old;
	unsigned long sign;
	unsigned count;
	d_word shift = opd_word;

	temp = p->regs[opd_reg];
	temp <<= 16;
	temp += p->regs[opd_reg | 1 ];

	old = temp;

	if (( shift & 077 ) == 0 ) {	/* no shift */

		CLR_CC_V();

		if ( temp & 0x80000000 ) {
			SET_CC_N();
		} else {
			CLR_CC_N();
		}
	
		if ( temp ) {
			CLR_CC_Z();
		} else {
			SET_CC_Z();
		}

		return CPU_OK;
	}
	if ( shift & 040 ) {		/* right shift */
		count = 0100 - ( shift & 077 );
		sign = temp & 0x80000000;
		while ( count-- ) {
			if ( temp & LSBIT ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp >>= 1;
			temp += sign;
		}
	} else {			/* left shift */
		count = shift & 037;
		while ( count-- ) {
			if ( temp & 0x80000000 ) {
				SET_CC_C();
			} else {
				CLR_CC_C();
			}
			temp <<= 1;
		}
	}
		
	if ( temp & 0x80000000 )
		SET_CC_N();
	else
		CLR_CC_N();

	if ( temp )
		CLR_CC_Z();
	else
		SET_CC_Z();

	if (( old & 0x80000000 ) == ( temp & 0x80000000 )) {
		CLR_CC_V();
	} else {
		SET_CC_V();
	}

	p->regs[opd_reg] = (temp >> 16);
	p->regs[opd_reg | 1] = LOW16( temp );

	return CPU_OK;
}		// OPR_ASHC

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

d_word OPR_MUL (pdp_regs *p, int opd_reg, d_word opd_word) {
	union s_u_word data1;
	union s_u_word data2;
	union s_u_long tmp;

	data1.u_word = p->regs[opd_reg];
	data2.u_word = opd_word;

	tmp.s_long = ((long) data1.s_word) * ((long) data2.s_word );

	p->regs[opd_reg] = (tmp.u_long >> 16);
	p->regs[opd_reg | 1] = (tmp.u_long & 0177777);

	CLR_CC_ALL();

	if ( tmp.u_long == 0 )
		SET_CC_Z();
	else
		CLR_CC_Z();

	if ( tmp.u_long & 0x80000000 )
		SET_CC_N();
	else
		CLR_CC_N();

	return CPU_OK;
}		// OPR_MUL

/*
 * DIV: Divide Instruction.
 */

d_word OPR_DIV (pdp_regs *p, int opd_reg, d_word opd_word) {
	union s_u_long tmp;
	union s_u_long eql;
	union s_u_word data2;

	tmp.u_long = p->regs[opd_word];
	tmp.u_long = tmp.u_long << 16;
	tmp.u_long  += p->regs[opd_word | 1 ];

	data2.u_word = opd_word;

	if ( data2.u_word == 0 ) {
		SET_CC_C();
		SET_CC_V();
		return CPU_OK;
	} else {
		CLR_CC_C();
	}

	eql.s_long = tmp.s_long / data2.s_word;
	p->regs[opd_reg] = eql.u_long & 0177777;

	if ( eql.u_long == 0 )
		SET_CC_Z();
	else
		CLR_CC_Z();

	if (( eql.s_long > 077777) || ( eql.s_long < -0100000))
		SET_CC_V();
	else
		CLR_CC_V();

	if ( eql.s_long < 0 )
		SET_CC_N();
	else
		CLR_CC_N();

	eql.s_long = tmp.s_long % data2.s_word;
	p->regs[opd_reg | 1] = eql.u_long & 0177777;

	return CPU_OK;
}		// OPR_DIV

