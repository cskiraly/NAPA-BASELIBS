/*
 * Common functions (mostly init stuff) for GRAPES
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>

#include	<event2/event.h>


#include	"grapes.h"
#include	"grapes_log.h"

struct event_base *eventbase = NULL;

#if 0
	/* Won't need this for normal operation */
void never_callback(evutil_socket_t fd, short what, void *arg) {
	fatal("It's the end of the world!");
}
#endif

/** Initialize the software. This should be called FIRST */
void grapesInit(void *event2_base) {
	eventbase = event2_base;
#if 0
	/* Won't need this for normal operation */
        struct timeval t = { 1000*1000*1000, 0 };
        event_base_once(eventbase, -1, EV_TIMEOUT, never_callback, NULL, &t);
#endif
}

void grapesYield() {
	event_base_loop(eventbase, EVLOOP_ONCE);
}

struct periodic {
	void(*cb)(HANDLE handle, void *arg);
	void *cbarg;
	double frequency;
};

void schedule_periodic_callback(evutil_socket_t fd, short what, void *arg) {
	struct periodic *p = arg;
	/* This indicates STOP */
	if (p == NULL || p->frequency == 0.0) {
		if (arg) free(arg);
		return;
	}
	
	double next = 1.0 / p->frequency;
        struct timeval t = { floor(next), fmod(next,1.0) * 1000000.0 };
        event_base_once(eventbase, -1, EV_TIMEOUT, schedule_periodic_callback, arg, &t);
	if (p->cb) p->cb(arg, p->cbarg);
}


HANDLE grapesSchedulePeriodic(const struct timeval *start, double frequency, void(*cb)(HANDLE handle, void *arg), void *cbarg) {
	struct timeval t = { 0,0 };
	if (start) t = *start;
	struct periodic *h = malloc(sizeof(struct periodic));
	if (!h) return NULL;

	h->cb = cb;
	h->cbarg = cbarg;
	h->frequency = frequency;

	event_base_once(eventbase, -1, EV_TIMEOUT, schedule_periodic_callback, h, &t);
	return h;
}

void grapesStopPeriodic(HANDLE h) {
	struct periodic *p = h;
	p->frequency = 0.0;
}


const char *timeval2str(const struct timeval *ts) {
	if (ts->tv_sec == 0 && ts->tv_usec == 0) return "0";
 
	static char buf[100];
	sprintf(buf, "%ld%03ld", ts->tv_sec, ts->tv_usec/1000);
 
	return buf;
}


