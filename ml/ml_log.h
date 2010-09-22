#ifndef _ML_LOG_H
#define _ML_LOG_H

#include <stdlib.h>
#include <stdio.h>

extern int ml_log_level;

extern void setLogLevel(int ll);

#define DPRINT(ll, format, ... )  {struct timeval tnow; if(ll <= ml_log_level) {gettimeofday(&tnow,NULL); fprintf(stderr, "%ld.%03ld "format, tnow.tv_sec, tnow.tv_usec/1000, ##__VA_ARGS__ );}}

#define debug(format, ... ) DPRINT(4 ,format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_INFO messages */
#define info(format, ... )  DPRINT(3, format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_WARN messages */
#define warn(format, ... )  DPRINT(2, format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_ERROR messages */
#define error(format, ... )  DPRINT(1, format, ##__VA_ARGS__ )
/**  Convenience macro to log LOG_CRITICAL messages and crash the program */
#define fatal(format, ... )  { DPRINT(0, format, ##__VA_ARGS__ ); exit(-1); }

#endif
