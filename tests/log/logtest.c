/*
 * Log test and demo application for the logging facilities of the GRAPES framework.
 *
 * There are 2 ways of calling grapes_log:
 * 	- grapes_log(SEVERITY, format, args...)
 *	- or use one of the convenience macros like debug(format, args...) or info(format, args...) as shown below.
 *
 */

#include	<grapes_log.h>		/* You must include grapes_log.h in order to use grapes logging */

#include	<math.h>		/* This one is just for fun */

int main(int argc, char *argv) {

	grapes_log(LOG_INFO, "You won't see this as logging is not initialized");

	/* Initialize logging */
	grapesInitLog(-1, NULL, NULL);

	grapes_log(LOG_WARN,"Warning! Here I come!");

	grapes_log(LOG_INFO,"Hey! This is my %dst %s message", 1, "INFO");
	info("And here comes the %dnd, just much simpler", 2);

	error("And this is an error message");	

	grapesCloseLog();

	info("Invisible again, as logging is closed");

	grapesInitLog(-1, NULL, NULL);

	debug("Next, I'm going to fatally die...");

	fatal("Unrecoverable error %lf, exiting...", M_PI);

	return 0;
}
