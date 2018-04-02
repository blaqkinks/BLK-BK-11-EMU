#include "defines.h"
#include "emu2149.h"
#include <fcntl.h>
#include <stdio.h>

#ifndef __MINGW32__

#include <sys/ioctl.h>

#endif

unsigned char synth_reg;

extern int io_sound_freq;

PSG * psg;

static void synth_init() {
	synth_reg = ~0;
	if (!psg) {
		PSG_init(3579545, io_sound_freq);
		psg=PSG_new();
	}
	PSG_reset(psg);
	// PSG_setVolumeMode(psg,2);
}	// synth_init

static CPU_RES synth_read(c_addr addr, d_word *word) {
	// *word = PSG_readReg(psg, synth_reg) ^ 0xFF;
	*word = 0; // BK does not read from AY
	return CPU_OK;
}	// synth_read

static CPU_RES synth_write(c_addr addr, d_word word) {
	// Writing register address
	synth_reg = (word & 0xF) ^ 0xF;
	return CPU_OK;
}	// synth_write

static CPU_RES synth_bwrite(c_addr addr, d_byte byte) {
	// Writing data; what happens if the address is odd?
	PSG_writeReg(psg, synth_reg, byte ^ 0xFF);
	return CPU_OK;
}	// synth_bwrite

int synth_next() {
	int a = psg ? PSG_calc(psg) : 0; 
	return a;
}

pdp_qmap q_synth = {
	"synth", "PSG audio",
	PORT_REG, PORT_SIZE, synth_init, synth_read, synth_write, synth_bwrite
};

