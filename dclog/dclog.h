/* =========================================================================
 * File: dclog.h
 *
 * Copyright (c) 2003 and onwards, Twenty First Century Communications, Inc.
 * <josh.glover@tfcci.com>
 *
 * LICENCE:
 *
 *   This file is distributed under the terms of the BSD License, v2. See
 *   the COPYING file, which should have been distributed with this file,
 *   for details. If you did not receive the COPYING file, see:
 *
 *   http://www.jmglov.net/open-source/licences/bsd.html
 *
 * DESCRIPTION:
 *
 *   Defines the DCLog class.
 *
 * USAGE:
 *
 *   #include <dclog.h>
 *
 * EXAMPLES:
 *
 *   See test-dclog.c
 *
 * TODO:
 *
 *   - Convert char pointers to arrays in class
 *   - Nothing, this code is perfect
 *  
 * DEPENDENCIES:
 *
 *   None
 *
 * MODIFICATIONS:
 *
 *   Josh Glover <josh.glover@tfcci.com> (2003/09/03): Initial revision
 * =========================================================================
 */

#ifndef __DCLOG_H__
#define __DCLOG_H__

// Standard headers
#include <stdio.h>
#include <time.h>

// Our headers
#include <common-types.h>

// Log levels
#define DCLOG_ALARM   0 //!< alarms
#define DCLOG_ERROR   1 //!< error messages
#define DCLOG_WARNING 2 //!< warnings
#define DCLOG_INFO    3 //!< information messages
#define DCLOG_DEBUG   4 //!< debugging messages
#define DCLOG_PROFILE 5 //!< profiling messages

//! Length of our filename string
#define DCLOG_FN_LEN 1024

//! Length of our timestamp strfime format string
#define DCLOG_TS_FMT_LEN 32

//! Default timestamp strfime format string
#define DCLOG_TS_FMT_DEFAULT ".%d"

//! Maximum size of an alarm email
#define DCLOG_MAX_ALARM_SIZE 4096


/** DCLog class
 *
 * Contains all of the state that the logger needs. The DCLog object should
 * be created with NewDCLog():
 *   DCLog *dclog = NewDCLog();
 * and then passed to all logger functions as the first argument:
 *   DCLogWrite( dclog, 5, "The foo has been barred %d times\n", num_foo );
 */

typedef struct {

  FILE *fp; //!< file pointer
  char *fn; //!< filename

  char *mailhost;   //!< hostname of the mail server
  char *to;         //!< email address to send alarms to
  char *from;       //!< from header for alarms
  char  spool[256]; //!< alarmd's spool directory
  
  UCHAR lev;      //!< logging level
  UCHAR features; //!< header, unique log filename, etc

  char ts_fmt[DCLOG_TS_FMT_LEN]; //!< strftime(3) format of timestamp
  
  struct tm *prev_date; //!< current file date

  time_t oldtime[1000];  //!< holds the time of the last alarm
  ULONG  count[1000];    //!< holds the amount of alarms between alarm events

  int limit_size;    //!< if true, use size limits
  int max_file_size; //!< maximum size, in bytes of a logfile

} DCLog;

static const char DCLOG_MAILHOST[] = "localhost";
static const char DCLOG_TO[]       = "root@localhost";
static const char DCLOG_FROM[]     = "localhost";
static const char DCLOG_PROG[]     = "dclog";
static const char DCLOG_SPOOL[]    = "/var/spool/alarmd";


// Constructors and destructors
// -------------------------------------------------------------------------


DCLog *NewDCLog( void );
UCHAR  DestroyDCLog( DCLog *dclog );


// -------------------------------------------------------------------------


// Stringifying methods
// -------------------------------------------------------------------------


char        DCLogLevelToNum( const char *str );
const char *DCLogLevelToString( const char lev );


// -------------------------------------------------------------------------


// Setup methods
// -------------------------------------------------------------------------


UCHAR DCLogSetHeader( DCLog *dclog, const UCHAR val );
UCHAR DCLogSetNewLine( DCLog *dclog, const UCHAR val );
UCHAR DCLogSetLevel( DCLog *dclog, const UCHAR lev );
UCHAR DCLogSetMailServer( DCLog *dclog, const char *mailhost );
UCHAR DCLogSetMaxFileSize( DCLog *dclog, const int size );
UCHAR DCLogSetPageServer( DCLog *dclog, const char *server );
UCHAR DCLogSetPrintLevel( DCLog *dclog, const UCHAR val );
UCHAR DCLogSetProgram( DCLog *dclog, const char *prog );
UCHAR DCLogSetSpool( DCLog *dclog, const char *spool );
UCHAR DCLogSetUniqueByDay( DCLog *dclog, const UCHAR val );
UCHAR DCLogSetTimestampFormat( DCLog *dclog, char *const ts_fmt );


// -------------------------------------------------------------------------


// Logging methods
// -------------------------------------------------------------------------


UCHAR DCLogOpen( DCLog *dclog, const char *fn, const char *mode );
UCHAR DCLogClose( DCLog *dclog );

UCHAR DCLogAlarm( DCLog *dclog, const char *code, const char *syn,
                  const char *fmt, ... );
UCHAR DCLogWrite( DCLog *dclog, const UCHAR lev, const char *fmt, ... );


// -------------------------------------------------------------------------


// Miscellaneous methods
// -------------------------------------------------------------------------


UCHAR DCLogCanLog( const DCLog *dclog );


// -------------------------------------------------------------------------


/*** Refactor for alarmd ***/
// SMTP-specific functions
// -------------------------------------------------------------------------


UCHAR DCLogSmtpSend( const char *mailhost, const char *to, const char *from,
                     const char *subject, const char *msg );
UCHAR DCLogSmtpShutdown( const int socket, const int retval );


// -------------------------------------------------------------------------
/*** Refactor for alarmd ***/


#endif // #ifndef __DCLOG_H__
