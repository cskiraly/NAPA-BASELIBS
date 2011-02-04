#ifndef _NAPA_LOG_H
#define _NAPA_LOG_H

/** Logging facilities for NAPA 

 * @file napa_log.h
 * @brief This module provides convenient logging functions for other NAPA modules
 *
 */

#include	<stdlib.h>

/* These log levels should match dclog's internal definition */
#define DCLOG_ALARM   0 //!< alarms
#define DCLOG_ERROR   1 //!< error messages
#define DCLOG_WARNING 2 //!< warnings
#define DCLOG_INFO    3 //!< information messages
#define DCLOG_DEBUG   4 //!< debugging messages
#define DCLOG_PROFILE 5 //!< profiling messages

/** log level for CRITICAL messages */
#define LOG_CRITICAL    DCLOG_ALARM
/** log level for ERROR messages */
#define LOG_ERROR       DCLOG_ERROR
/** log level for WARNING messages */
#define LOG_WARN        DCLOG_WARNING
/** log level for INFO messages */
#define LOG_INFO        DCLOG_INFO
/** log level for DEBUG messages */
#define LOG_DEBUG       DCLOG_DEBUG
/** log level for PROFILE messages */
#define LOG_PROFILE     DCLOG_PROFILE

/** general-purpose, module-aware log facility, to be used with a log priority */
#define napa_log(priority, format, ... ) napaWriteLog(priority,  format " [%s,%d]\n",  ##__VA_ARGS__ , __FILE__, __LINE__ )
/** Convenience macro to log TODOs */
#define todo(format, ...) napa_log(LOG_WARN, "[TODO] " format " file: %s, line %d",  ##__VA_ARGS__ )

/** Convenience macro to log LOG_DEBUG messages */
#define debug(format, ... ) napa_log(LOG_DEBUG, format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_INFO messages */
#define info(format, ... )  napa_log(LOG_INFO, format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_WARN messages */
#define warn(format, ... )  napa_log(LOG_WARN, format, ##__VA_ARGS__ )
/** Convenience macro to log LOG_ERROR messages */
#define error(format, ... )  napa_log(LOG_ERROR, format, ##__VA_ARGS__ )
/**  Convenience macro to log LOG_CRITICAL messages and crash the program */
#define fatal(format, ... ) {  napa_log(LOG_CRITICAL, format, ##__VA_ARGS__ ); exit(-1); }

#ifdef __cplusplus
extern "C" {
#endif

/** 
  Initializes the NAPA logging facility.

  @param[in] loglevel Only log messages on or above this level
  @param[in] filename Write log messages to this file (might be "stderr" or "stdout")
  @param[in] mode file open mode ("a" for append or "w" for truncate)
*/
void napaInitLog(int loglevel, const char *filename, const char *mode);

/** Closes down NAPA logging facility. Log messages are discarded after this. */
void napaCloseLog();

/** Low-level interface to the NAPA logging system. Use the above defined convenience macros instead */
void napaWriteLog(const unsigned char lev, const char *fmt, ... );
#ifdef __cplusplus
}
#endif

#endif


