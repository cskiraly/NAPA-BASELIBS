/*
 * Log test and demo application for the logging facilities of the NAPA framework.
 *
 * There are 2 ways of calling napa_log:
 * 	- napa_log(SEVERITY, format, args...)
 *	- or use one of the convenience macros like debug(format, args...) or info(format, args...) as shown below.
 *
 */

#include	<napa_log.h>		/* You must include napa_log.h in order to use napa logging */

#include	<math.h>		/* This one is just for fun */

int main(int argc, char *argv) {

	napa_log(LOG_INFO, "You won't see this as logging is not initialized");

	/* Initialize logging */
	napaInitLog(-1, NULL, NULL);

	napa_log(LOG_WARN,"Warning! Here I come!");

	napa_log(LOG_INFO,"Hey! This is my %dst %s message", 1, "INFO");
	info("And here comes the %dnd, just much simpler", 2);

	error("And this is an error message");	

	napaCloseLog();

	info("Invisible again, as logging is closed");

	napaInitLog(-1, NULL, NULL);

	debug("Next, I'm going to fatally die...");

	fatal("Unrecoverable error %lf, exiting...", M_PI);

	return 0;
}
