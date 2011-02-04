/*
 * This file implements logging functionality for the NAPA framework
 */

#define _GNU_SOURCE

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<napa_log.h>
#include	"dclog.h"

DCLog *dclog = NULL;
static char initialized = 0;
FILE *logstream = NULL;
char *logbuffer = NULL;
size_t logbuffer_size = 0;

void napaInitLog(int log_level, const char *filename, const char *mode) {
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

#if !WIN32 && !MAC_OS
	logstream = open_memstream(&logbuffer, &logbuffer_size);
	if (!logstream) {
		fprintf(stderr, "Unable to initialize logger, exiting");
		exit(-1);
	}
#else
        logbuffer_size=1000;
	logbuffer = (char *) malloc(logbuffer_size);
#endif
        initialized = 1;
}

void napaCloseLog() {
	if (!initialized) return;

	DCLogClose(dclog);
	if (logstream) fclose(logstream);
	if (logbuffer) free(logbuffer);
	initialized = 0;
}

void napaWriteLog(const unsigned char lev, const char *fmt, ... ) {
	va_list str_args;

	if (!initialized) return;


  	va_start( str_args, fmt );
#if !WIN32 && !MAC_OS
	rewind(logstream);
  	if (vfprintf( logstream, fmt, str_args ) < 0) return;
	char zero = 0;
	if (fwrite(&zero, 1, 1, logstream) != 1) return;
	fflush(logstream);
#else
        if (vsnprintf( logbuffer, logbuffer_size, fmt, str_args) < 0) return;
        logbuffer[logbuffer_size - 1] = '\0';
#endif
  	va_end( str_args );

//fprintf(stderr, "X.do logger lev:%d, msg %s\n", lev, logbuffer);
	DCLogWrite(dclog, lev , logbuffer);
        fflush(dclog->fp);
}

