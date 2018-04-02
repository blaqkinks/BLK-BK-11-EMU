/*
 * Used to be
 * Copyright 1994, Eric A. Edwards
 * After substantial changes
 * Public Domain - Allynd Dudnikov (balqk))
 */

/*
 * service.c - Event handler system and interrupt service.
 */

#include "defines.h"
#include <libintl.h>
#define _(String) gettext (String)

/* 1 if interrupt requested while WAIT was in effect */

unsigned char in_wait_instr = 0;

/* Allows up to 32 different HW interrupts,
 * there can be no more than 1 pending interrupt of each kind.
 */
unsigned long pending_interrupts = 0;
double earliest = 0.0;
event events[NUM_PRI];

/*
 * ev_init() - Initialize the event system.
 */

void ev_init()
{
	pending_interrupts = 0;
}

/*
 * ev_register() - Register an event.
 */

void
ev_register( priority, handler, delay, info )
unsigned priority;
int (*handler)(); 
unsigned long delay;	/* in clock ticks */
d_word info;
{
	unsigned ticks = pdp.clock;


	if (pending_interrupts & (1 << priority)) {
		/* There is one pending already */
		return;
	}
	/*
	 * Fill in the info.
	 */

	events[priority].handler = handler;
	events[priority].info = info;
	events[priority].when = ticks + delay;
	if (!pending_interrupts || ticks + delay < earliest) {
		earliest = ticks + delay;
	}
	pending_interrupts |= 1 << priority;
	/* fprintf(stderr, "Registering pri %d @ %g\n", priority, earliest); */
}


/*
 * ev_fire() - Fire off any pending event by the mask
 * priority list.
 */

void ev_fire( int priority )
{
	unsigned ticks = pdp.clock;

	int x;
	unsigned long mask = 0;
	switch (priority) {
		case 0: mask = ~0; break;
		case 1: mask = ~0; break;
		case 2: mask = ~0; break;
		case 3: mask = ~0; break;
		/* BK-0010 uses MTPS #200 to block keyboard interrupts */
		case 4: mask = 1; break;
		case 5: mask = 0; break;
		case 6: mask = 0; break;
		case 7: mask = 0; break;
	}
	if (!(mask & pending_interrupts) || ticks < earliest) {
		return;
	}

	earliest = 10.0e38;

	for( x = 0; x < NUM_PRI && (1<<x) <= pending_interrupts; ++x ) {
		if (mask & pending_interrupts & (1<<x) &&
			ticks >= events[x].when) {
			events[x].handler(events[x].info);
			/* fprintf(stderr, "Firing pri %d\n", x); */
			pending_interrupts &= ~(1 << x);
			mask = 0;
		} else if (pending_interrupts & (1<<x) &&
			events[x].when < earliest) {
			earliest = events[x].when;
		}
	}
	/* fprintf(stderr, "Earliest is %g\n", earliest); */
}


