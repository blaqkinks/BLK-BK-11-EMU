
/*
 *
 *	"sdl_sound.c":
 *	SDL-based sound engine.
 *
 *	by Allynd Dudnikov (blaqk).
 *
 */

#include "defines.h"
#include <stdio.h>
#include <SDL/SDL.h>

unsigned io_sound_bufsize = 512, io_sound_freq = 44100;
extern flag_t nflag, covoxflag, synthflag;

static unsigned sound_timing = 0;

// Get current sound clock value
static unsigned sound_clock () {
return ((double)pdp.clock * io_sound_freq) / TICK_RATE;
}	// sound_clock

//
// Speaker sound queue
//

enum { spk_total = 1024 };

static unsigned spk_timing[spk_total];
static unsigned spk_head = 0, spk_tail = 0, spk_count = 0;

static unsigned spk_max = 0;

flag_t log_spk = 1, log_cvx = 1;

/* Put next speaker pulse in queue */
bool spk_pulse () {
bool OK = true;

SDL_LockAudio ();

if (spk_count == spk_total) { OK = false; }
else {
unsigned time = spk_timing[spk_tail ++] = sound_clock();
logF (log_spk, "+spk %u\n", time);

if (spk_tail == spk_total) spk_tail = 0;
spk_count ++;

if (spk_count > spk_max) spk_max = spk_count;
}

SDL_UnlockAudio();

return OK;
}	// spk_pulse

void spk_decode (unsigned count, Sint16 * to) {
Sint16 val = spk_head & 1 ? 0x7fff : -0x7fff;

logF (log_spk, "-spk (%u->%u)\n", sound_timing, sound_timing + count);

while (count --) {
		while (spk_count && sound_timing >= spk_timing[spk_head]) {
		val = -val;
		if (++ spk_head == spk_total) spk_head = 0;
		spk_count --;
		}

		*to ++ = val;
		++ sound_timing;
		}

}	// spk_decode

//
// Covox sound queue
//

enum { cvx_total = 2048 };

static unsigned cvx_timing[cvx_total];
static d_byte cvx_samples[cvx_total];
static unsigned cvx_head = 0, cvx_tail = 0, cvx_count = 0;

bool covox_sample (d_byte val) {
bool OK = true;
SDL_LockAudio ();

if (cvx_count == cvx_total) { OK = false; }
else {
cvx_samples[cvx_tail] = val;
cvx_timing[cvx_tail ++] = sound_clock();
if (cvx_tail == cvx_total) cvx_tail = 0;
cvx_count ++;
}

SDL_UnlockAudio();

return OK;
}	// covox_sample

void covox_decode (unsigned count, Sint16 * to) {
Sint16 val = cvx_samples[cvx_head];

while (count --) {
		while (cvx_count && sound_timing >= cvx_timing[cvx_head]) {
		if (++ cvx_head == cvx_total) cvx_head = 0;
		val = cvx_samples[cvx_head];
		cvx_count --;
		}

		*to ++ = ((int) val - 128) << 8;
		++ sound_timing;
		}
}	// covox_decode

void synth_decode (unsigned count, Sint16 * to) {
while (count --)
	* to ++ = synth_next();
}	// synth_decode

static void callback(void *dummy, Uint8 *outbuf, int len)
{
	// TODO:
	// implement proper sound mixing from different sources

	unsigned count = len / 2;
	Sint16 *to = (Sint16 *) outbuf;

	if (synthflag)
		synth_decode (count, to);
	else if (covoxflag)
		covox_decode (count, to);
	else
		spk_decode (count, to);
}	// callback

void sound_finish() {
	/* release the write thread so it can terminate */
	SDL_PauseAudio(1); 

	logF (log_spk, "speaker: max %u/%u\n", spk_max, spk_total);
}

void sound_init() {
	SDL_AudioSpec desired, actual;

	static int init_done = 0;
	if (!nflag)
		return;

	if (init_done) return;

	desired.format = AUDIO_S16MSB;
	desired.channels = 1;
	desired.freq = io_sound_freq;
	desired.samples = io_sound_bufsize;
	desired.userdata = NULL;
	desired.callback = callback;

	if (SDL_OpenAudio(&desired, &actual) < 0) {
		logF(0, "Failed to initialize sound (freq %d, %d samples): %s\n",
			io_sound_freq, io_sound_bufsize, SDL_GetError());
	}

	logF(0, "Sound initialised: freq %d, samples %d, format %04X\n",
			actual.freq, actual.samples, actual.format);
	io_sound_freq = actual.freq;
	io_sound_bufsize = actual.samples;

	atexit(sound_finish);
	SDL_PauseAudio(0);
	init_done = 1;
}
