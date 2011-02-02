#ifndef _ML_LOG_H
#define _ML_LOG_H

#include <stdlib.h>
#include <stdio.h>

//#include "grapes_log.h"

#ifndef _GRAPES_LOG_H

#define DPRINT(format, ... )  {struct timeval tnow; gettimeofday(&tnow,NULL); fprintf(stderr, "%ld.%03ld "format, tnow.tv_sec, tnow.tv_usec/1000, ##__VA_ARGS__ );}

#define debug(format, ... ) //DPRINT(format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_INFO messages */
#define info(format, ... )  DPRINT(format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_WARN messages */
#define warn(format, ... )  DPRINT(format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_ERROR messages */
#define error(format, ... )  DPRINT(format, ##__VA_ARGS__ )
/**  Convenience macro to log LOG_CRITICAL messages and crash the program */
#define fatal(format, ... )  { DPRINT(format, ##__VA_ARGS__ ); exit(-1); }

#endif
#endif
