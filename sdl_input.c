
/*
 *
 *	"sdl_input.c":
 *	SDL-based input engine.
 *
 *	by Allynd Dudnikov (blaqk).
 *
 */

#include "SDL/SDL.h"
#include "SDL/SDL_keysym.h"
#include "SDL/SDL_events.h"

#include "defines.h"
#include "kbd.h"

static int special_keys[SDLK_LAST];

static unsigned key_mapping [] = {
    SDLK_BACKSPACE,		KEY_ERASE,
    SDLK_TAB,					KEY_HT,
    SDLK_RETURN,			KEY_ENTER,
    SDLK_CLEAR,				KEY_CLEAR,

    SDLK_SCROLLOCK,		KEY_SWITCH,
    SDLK_LSUPER,			KEY_AR2,
    SDLK_LALT,				KEY_AR2,
    SDLK_ESCAPE,			KEY_STOP,

    SDLK_DELETE,			KEY_DEL,
    SDLK_LEFT,				KEY_LEFT,
    SDLK_UP,					KEY_UP,
    SDLK_RIGHT,				KEY_RIGHT,
    SDLK_DOWN,				KEY_DOWN,
    SDLK_HOME,				KEY_VS,
    SDLK_PAGEUP,			KEY_CLRTAB,
    SDLK_PAGEDOWN,		KEY_SETTAB,

    SDLK_END,					KEY_HOME,
    SDLK_INSERT,			KEY_INS,
    SDLK_BREAK,				KEY_STOP,
    SDLK_F1,					KEY_REPEAT,
    SDLK_F2,					003,           /* kt */
    SDLK_F3,					0213,          /* -|--> */
    SDLK_F4,					KEY_DEL,       /* |<--- */
    SDLK_F5,					KEY_INS,       /* |---> */
    SDLK_F6,					KEY_ICTL,      /* ind su */
    SDLK_F7,					KEY_BLCK,      /* blk red */
    SDLK_F8,					0200,          /* shag */
    SDLK_F9,					KEY_CLEAR,     /* sbr */
    SDLK_F10,					KEY_STOP,
    SDLK_F11,					KEY_RESET,
		};

void kbd_init () {
		int i;

    for (i = 0; i < SDLK_LAST; i++)
			special_keys[i] = -1;

    for (i = SDLK_NUMLOCK; i <= SDLK_COMPOSE; i++)
			special_keys[i] = KEY_NOTHING;

		for (i = 0; i < sizeof(key_mapping) / sizeof(*key_mapping); i += 2)
			special_keys[key_mapping[i]] = key_mapping[i + 1];
}	// kbd_init

/*
 * input_processing()
 *
 * Handle input events.
 *
 */

static int grab_mode;			// (mouse grab mode)

void input_processing() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
	    switch (ev.type) {
	    case SDL_KEYDOWN:
	    case SDL_KEYUP: {
				unsigned ksym = ev.key.keysym.sym, kmod = ev.key.keysym.mod;
				if (ksym != SDLK_UNKNOWN) {
					tty_keyevent (special_keys[ksym] != -1 ? special_keys[ksym] : ksym,
						ev.type == SDL_KEYDOWN,
						(kmod & KMOD_CAPS ? KS_CAPS : 0) |
						(kmod & KMOD_SHIFT ? KS_SHIFT : 0) |
						(kmod & KMOD_CTRL ? KS_CTRL : 0));
					}
				}
				break;

	    case SDL_VIDEOEXPOSE:
	    case SDL_ACTIVEEVENT:
				/* the visibility changed */
				scr_dirty  = 256;
				break;

	    case SDL_MOUSEBUTTONDOWN:
				if (ev.button.button == SDL_BUTTON_MIDDLE) {
					/* middle button switches grab mode */
					SDL_WM_GrabInput((grab_mode ^= 1) ? SDL_GRAB_ON : SDL_GRAB_OFF);
					return;
					}
				// (... fall through ...)

	    case SDL_MOUSEBUTTONUP:
				mouse_buttonevent (ev.type == SDL_MOUSEBUTTONDOWN, ev.button.button);
				break;

	    case SDL_MOUSEMOTION:
				mouse_moveevent (ev.motion.xrel, ev.motion.yrel);
				break;

	    case SDL_VIDEORESIZE:
				scr_switch(ev.resize.w, ev.resize.h);
				break;

	    case SDL_QUIT:
				exit(0);

	    default:
				logF(0, "Unexpected SDL event: %d\n", ev.type);
	    }
    }
}	// input_processing

/* Timing & delays */

/* Get ticks elapsed (ms.) */
unsigned get_ticks() {
return SDL_GetTicks();
}	// get_ticks

unsigned idle_ticks = 0;

/* Wait for the ticks (ms.) */
void wait_ticks(unsigned ticks) {
idle_ticks += ticks;
SDL_Delay (ticks);
}	// wait_ticks

