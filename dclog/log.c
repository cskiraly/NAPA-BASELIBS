/*
 * This file implements logging functionality for the GRAPES framework
 */

#define _GNU_SOURCE

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<grapes_log.h>
#include	"dclog.h"

DCLog *dclog = NULL;
static char initialized = 0;
FILE *logstream = NULL;
char *logbuffer = NULL;
size_t logbuffer_size = 0;

void grapesInitLog(int log_level, const char *filename, const char *mode) {
        if (initialized) return;

        /* Initialize the logger facility */
        if ((dclog = NewDCLog()) == NULL) {
                fprintf( stderr, "FATAL ERROR: failed to initialize logger!\n" );
                exit(-1);
        }
	if (!filename) filename = "stderr";
	if (!mode || strcmp(mode, "a") != 0 || strcmp(mode, "w") != 0) mode = "a";

        if (DCLogOpen( dclog, filename, mode) != 1) {
                fprintf( stderr, "FATAL ERROR: failed to initialize logger, logfile: %s, mode: %s!\n", filename, mode );
                exit(-1);
	};
        if(log_level < 0)
            DCLogSetLevel( dclog, UCHAR_MAX );
        else
            DCLogSetLevel( dclog, log_level );
        DCLogSetHeader( dclog, 1 );
        DCLogSetPrintLevel( dclog, 1 );

	logstream = open_memstream(&logbuffer, &logbuffer_size);
	if (!logstream) {
		fprintf(stderr, "Unable to initialize logger, exiting");
		exit(-1);
	}
        initialized = 1;
}

void grapesCloseLog() {
	if (!initialized) return;

	DCLogClose(dclog);
	if (logstream) fclose(logstream);
	if (logbuffer) free(logbuffer);
	initialized = 0;
}

void grapesWriteLog(const unsigned char lev, const char *fmt, ... ) {
	va_list str_args;

	if (!initialized) return;

	rewind(logstream);

  	va_start( str_args, fmt );
  	if (vfprintf( logstream, fmt, str_args ) < 0) return;
  	va_end( str_args );

	char zero = 0;
	if (fwrite(&zero, 1, 1, logstream) != 1) return;
	fflush(logstream);

	DCLogWrite(dclog, lev , logbuffer);
}

