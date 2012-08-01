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
 *   Implements the DCLog class.
 *
 * USAGE:
 *
 *   #include <dclog.h>
 *
 * EXAMPLES:
 *
 *   See dclog.h
 *
 * TODO:
 *
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

// Standard headers
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
//#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>

#else
#include <winsock2.h>

static struct tm *localtime_r(const time_t *clock, struct tm *result) {
    struct tm *res = localtime(clock);
    *result = *res;
  return result;
}
#endif

// dclog headers
#include <dclog.h>

// Features
#define DCLOG_FEAT_HEADER 1 << 0 //!< see DCLogSetHeader()
#define DCLOG_FEAT_UNIQUE 1 << 1 //!< see DCLogSetUniqueByDay()
#define DCLOG_FEAT_PLEVEL 1 << 2 //!< see DCLogSetPrintLevel()
#define DCLOG_FEAT_NEWLINE 1 << 3 //!< see DCLogSetNewLine()

//! String representations of log levels
static char *levels[] = {
  "ALARM",   // 0
  "ERROR",   // 1
  "WARNING", // 2
  "INFO",    // 3
  "DEBUG",   // 4
  "PROFILE", // 5
  NULL       // 6
}; // levels[]


// Private method prototypes
// -------------------------------------------------------------------------


static UCHAR _truncate( DCLog *dclog );


// -------------------------------------------------------------------------


// Private methods
// -------------------------------------------------------------------------


/** Truncates the currently open logfile.
 * 
 * @param *dclog  pointer to a DCLog object
 *
 * @return  1 on success, 0 on failure
 */

static UCHAR _truncate( DCLog *dclog ) {

  if (dclog == NULL || dclog->fn == NULL || dclog->fp == NULL)
    return 0;

  if ((dclog->fp = freopen( dclog->fn, "w", dclog->fp )) == NULL)
    return 0;
  
  return 1;

} // _truncate()


// -------------------------------------------------------------------------


// Constructors and destructors
// -------------------------------------------------------------------------


/** Constructer; creates a new DCLog object.
 *
 * @return  pointer to the newly created DCLog object
 */

DCLog *NewDCLog( void ) {

  DCLog *dclog;
  int    i;

  if ((dclog = malloc( sizeof( DCLog ) )) == NULL) {

    fprintf( stderr, "NewDCLog() could not malloc() %d bytes, is the system "
             "low on memory?\n", sizeof( DCLog ) );

    return (DCLog *)NULL;
    
  } // if (malloc(3) failed)
  
  // Zero out the memory
  memset( dclog, 0, sizeof( DCLog ) );

  // Allocate enough memory for the alarming stuff and set the defaults
  dclog->mailhost = malloc( (strlen( DCLOG_MAILHOST ) + 1) * sizeof( char ) );
  strcpy( dclog->mailhost, DCLOG_MAILHOST );
  dclog->to = malloc( (strlen( DCLOG_TO ) + 1) * sizeof( char ) );
  strcpy( dclog->to, DCLOG_TO );
  strcpy( dclog->spool, DCLOG_SPOOL );
  
  /*** Why are sizes hard-coded? We need a comment here! ***/
  for (i = 0; i < 128; i++) {

    dclog->oldtime[i] = (time_t) 0;
    dclog->count[i]   = 1;

  } // for (***)

  // Turn size limiting off
  dclog->limit_size    = 0;
  dclog->max_file_size = 0;
  
  return dclog;
  
} // NewLog()


/** Destroys a log object that is no longer needed.
 *
 * @param *dclog  pointer to a DCLog object
 *
 * @return  1 on success, 0 on failure
 */

UCHAR DestroyDCLog( DCLog *dclog ) {

  if (dclog == NULL)
    return 0;

  DCLogClose( dclog );
  
  free( dclog->fn );
  dclog->fn = NULL;
  free( dclog->mailhost );
  dclog->mailhost = NULL;
  free( dclog->to );
  dclog->to = NULL;
  free( dclog->from );
  dclog->from = NULL;

  if (dclog->prev_date != NULL)
    free( dclog->prev_date );
  
  free( dclog );
  dclog = NULL;
  
  return 1;

} // DestroyDCLog()


// -------------------------------------------------------------------------


// Stringifying methods
// -------------------------------------------------------------------------


/** Returns the numeric log level for the specified string representation.
 *
 * @param *str  string representation of a log level
 *
 * @return  numeric log level on success, -1 on error
 */

char DCLogLevelToNum( const char *str ) {

  char i;

  // Step through our array of names. If the string matches, return the
  // array index.
  for (i = 0; levels[(int)i] != NULL; i++)
    if (strncasecmp( str, levels[(int)i], 10 ) == 0)
      return i;

  // Nope, found nothing
  return -1;
  
} // DCLogLevelToNum()


/** Returns the string representation of the specified numeric log level.
 *
 * @param lev  numeric log level  
 *
 * @return  string representation of a log level on success, NULL on error
 */

const char *DCLogLevelToString( const char lev ) {

  char i;
  
  // Loop through the array of strings, returning the string when the
  // index matches lev (we do it this way to make sure lev is legal)
  for (i = 0; levels[(int)i] != NULL; i++)
    if (i == lev)
      return levels[(int)i];

  // We did not find a match, return error
  return (char *)NULL;
  
} // DCLogLevelToString()


// -------------------------------------------------------------------------


// Setup methods
// -------------------------------------------------------------------------


/** Turns the log header on or off.
 *
 * If on, each log message will be appended to the timestamp and process
 * ID (e.g. 15:27:13.05 [2727]: test for echo).
 *
 * @param *dclog  pointer to a DCLog object
 * @param  val    1 for on, 0 for off
 *
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetHeader( DCLog *dclog, const UCHAR val ) {
  
  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  if (val == 0)
    dclog->features ^= DCLOG_FEAT_HEADER;
  else if (val == 1)
    dclog->features |= DCLOG_FEAT_HEADER;
  else
    return 0;
  
  return 1;
  
} // DCLogSetHeader()


/** Sets the logging level.
 *
 * @param *dclog  pointer to a DCLog object
 * @param  lev    logging level (messages with a higher level will not be
 *                logged)
 *
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetLevel( DCLog *dclog, const UCHAR lev ) {

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  // Set the logging level
  dclog->lev = lev;
  
  return 1;
  
} // DCLogSetLevel()


/** Sets the hostname of the SMTP mail server.
 *
 * Alarm emails will be sent through this mail server.
 *
 * @param *dclog     pointer to a DCLog object
 * @param  mailhost  email address of the server
 *
 * @return  1 on success, 2 on failure.
 */

UCHAR DCLogSetMailServer( DCLog *dclog, const char *mailhost ) {

  // Make sure that we have a valid DCLog object and a string
  if (dclog == NULL || mailhost == NULL)
    return 0;
  
  // If dclog->to is already set...
  if (dclog->mailhost != NULL) {
    
    // ...and it is the same, return success
    if (strcmp( mailhost, dclog->mailhost ) == 0)
      return 1;
    
    // ...and it is not the same, destroy it
    free( dclog->mailhost );
    dclog->mailhost = NULL;
    
  } // if (dclog->to != NULL)
  
  // Allocate enough memory for dclog->to and set it
  if ((dclog->mailhost = malloc( (strlen( mailhost ) + 1) * sizeof( char ) ))
      == NULL)
    return 0;
  if (strcpy( dclog->mailhost, mailhost ) == NULL)
    return 0;
  
  return 1;
    
} // DCLogSetMailServer()


/** Sets maximum file size and turns on size limits.
 *
 * @param *dclog  pointer to a DCLog object
 * @param  size   maximum file size, in bytes
 * 
 * @return  1 on success, 0 on error
 */

UCHAR DCLogSetMaxFileSize( DCLog *dclog, const int size ) {

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  dclog->limit_size    = 1;
  dclog->max_file_size = size;

  return 1;

} // DCLogSetMaxFileSize()


/** Sets the page server's email address.
 *
 * Alarm emails will be sent to this address.
 *
 * @param *dclog   pointer to a DCLog object
 * @param  server  email address of the server
 *
 * @return  1 on success, 0 on failure.
 */

UCHAR DCLogSetPageServer( DCLog *dclog, const char *server ) {

  // Make sure that we have a valid DCLog object and a string
  if (dclog == NULL || server == NULL)
    return 0;
  
  // If dclog->to is already set...
  if (dclog->to != NULL) {
    
    // ...and it is the same, return success
    if (strcmp( server, dclog->to ) == 0)
      return 1;
    
    // ...and it is not the same, destroy it
    free( dclog->to );
    dclog->to = NULL;
    
  } // if (dclog->to != NULL)
  
    // Allocate enough memory for dclog->to and set it
  if ((dclog->to = malloc( (strlen( server ) + 1) * sizeof( char ) ))
      == NULL)
    return 0;
  if (strcpy( dclog->to, server ) == NULL)
    return 0;
  
  return 1;
    
} // DCLogSetPageServer()


/** Turns printing the message level on or off.
 *
 * If on, each log message will be appended to the mnemonic level at
 * which it was logged (e.g. messages logged at DCLOG_DEBUG would look like:
 * "DEBUG some message or other").
 *
 * @param *dclog  pointer to a DCLog object
 * @param  val    1 for on, 0 for off
 *
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetPrintLevel( DCLog *dclog, const UCHAR val ) {

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  if (val == 0)
    dclog->features ^= DCLOG_FEAT_PLEVEL;
  else if (val == 1)
    dclog->features |= DCLOG_FEAT_PLEVEL;
  else
    return 0;
  
  return 1;
    
} // DCLogSetPrintLevel()

/** Turn automatic appending of a \n to log messages on or off.
 *
 * If on, a newline will be automatically appended to the message
 *
 * @param *dclog  pointer to a DCLog object
 * @param  val    1 for on, 0 for off
 *
 * @return  1 on success, 0 on failure
 */
UCHAR DCLogSetNewLine( DCLog *dclog, const UCHAR val ) {

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  if (val == 0)
    dclog->features ^= DCLOG_FEAT_NEWLINE;
  else if (val == 1)
    dclog->features |= DCLOG_FEAT_NEWLINE;
  else
    return 0;
  
  return 1;

} // DCLogSetNewLine

/** Sets the program name that will be used when alarming.
 *
 * The from address in the alarm email will be program_name@hostname,
 * where program_name is set by this function, and hostname is the
 * hostname of the machine on which your code is running.
 * 
 * @param *dclog  pointer to a DCLog object
 * @param *prog   program string
 * 
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetProgram( DCLog *dclog, const char *prog ) {

  char  hostname[256];
  char *from;
  
  // Make sure that we have a valid DCLog object and a string
  if (dclog == NULL || prog == NULL)
    return 0;
  
  // Grab the hostname or use a default
  if (gethostname( hostname, 256 ) == -1)
    strcpy( hostname, DCLOG_FROM );
  
  // The from line should be program@hostname
  from = malloc( (strlen( prog ) + strlen( hostname ) + 2)
                 * sizeof( char ) );
  sprintf( from, "%s@%s", prog, hostname );
  
  // If dclog->from is already set...
  if (dclog->from != NULL) {
    
    // ...and it is the same, return success
    if (strcmp( from, dclog->from ) == 0)
      return 1;
    
    // ...and it is not the same, destroy it
    free( dclog->from );
    dclog->from = NULL;
    
  } // if (dclog->from != NULL)
  
  // Allocate enough memory for dclog->from and set it
  dclog->from = from;
  
  return 1;
    
} // DCLogSetProgram()


/** Sets the alarm spool directory.
 * 
 * @param *dclog  pointer to a DCLog object
 * @param *spool  spool directory
 * 
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetSpool( DCLog *dclog, const char *spool ) {

  // Make sure that we have a valid DCLog object and a string
  if (dclog == NULL || spool == NULL)
    return 0;

  if (strncpy( dclog->spool, spool, 256 ) == NULL)
    return 0;
  
  return 1;
  
} // DCLogSetSpool()


/** Turns unique logfiles on or off.
 *
 * If on, each logfile will get the day of the month appended to it
 * (e.g. my_logfile.27 versus my_logfile). Setting this after the logfile
 * is opened does not make a whole lot of sense.
 *
 * @param *dclog  pointer to a DCLog object
 * @param  val    1 for on, 0 for off
 *
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetUniqueByDay( DCLog *dclog, const UCHAR val ) {

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;

  // Update the features bitmask
  if (val == 0)
    dclog->features ^= DCLOG_FEAT_UNIQUE;
  else if (val == 1)
    dclog->features |= DCLOG_FEAT_UNIQUE;
  else
    return 0;

  // If ts_fmt is not set, apply the default
  if (dclog->ts_fmt[0] == '\0' &&
      strncpy( dclog->ts_fmt, DCLOG_TS_FMT_DEFAULT, DCLOG_TS_FMT_LEN )
      == NULL)
    return 0;

  return 1;
    
} // DCLogSetUniqueByDay()


/** Sets the timestamp format string and turns the unique feature on
 *
 * @param *dclog   pointer to a DCLog object
 * @param  ts_fmt  strftime(3) time format string
 *
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogSetTimestampFormat( DCLog *dclog, char *const ts_fmt ) {

  // Validate args
  if (dclog == NULL || ts_fmt == NULL)
    return 0;

  if (strncpy( dclog->ts_fmt, ts_fmt, DCLOG_TS_FMT_LEN ) == NULL)
    return 0;
  
  return DCLogSetUniqueByDay( dclog, 1 );
  
} // DCLogSetTimestampFormat()


// -------------------------------------------------------------------------


// Logging methods
// -------------------------------------------------------------------------


/** Opens a logfile.
 *
 * If the unique_by_day member of the DCLog object is set to true, the day
 * of the month will be appended to the filename of the logfile (see
 * DCLogSetUniqueByDay, above).
 *
 * @param *dclog  pointer to a DCLog object
 * @param  fn     fully qualified filename
 * @param  mode   mode for opening file (w truncates, a appends)
 * 
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogOpen( DCLog *dclog, const char *fn, const char *mode ) {

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  // Allocate enough memory to store the filename string, unless we already
  // have a filename, in which case we will assume it is correct.
  if (dclog->fn == NULL &&
      (dclog->fn = malloc( sizeof( char ) * DCLOG_FN_LEN )) == NULL)
    return 0;

  // Copy the filename string
  strncpy( dclog->fn, fn, DCLOG_FN_LEN );

  // If the filename string is "stderr", log to stderr
  if (strcasecmp( fn, "stderr" ) == 0) {
    
    dclog->fp = stderr;
    return 1;

  } // if (strcasecmp( fn, "stderr" ) == 0)

  // If the filename string is "stdout", log to stdout
  else if (strcasecmp( fn, "stdout" ) == 0) {
    
    dclog->fp = stdout;
    return 1;

  } // else if (strcasecmp( fn, "stdout" ) == 0)

  // If control reaches here, we are logging to a file
  
  // If unique feature is on, append the timestamp string
  if (dclog->features & DCLOG_FEAT_UNIQUE) {
    
    time_t     tv  = time( NULL );
    struct tm *tm = localtime( &tv );

    strftime( &dclog->fn[strlen( dclog->fn )],
              DCLOG_FN_LEN - 1 - strlen( dclog->fn ),
              dclog->ts_fmt, tm );

  } // if (dclog->features & DCLOG_FEAT_UNIQUE)
  
  // Open the file
  if ((dclog->fp = fopen( dclog->fn, mode )) == NULL)
    return 0;

  // Set the stream to unbuffered
  if (setvbuf( dclog->fp, (char *)NULL, _IONBF, 0 ) != 0)
    fprintf( stderr,
             "WARNING: DCLogOpen() could not set log file '%s' unbuffered!\n",
	     dclog->fn );

  // Return success.
  return 1;
    
} // DCLogOpen()


/** Closes an open logfile.
 *
 * @param *dclog  pointer to a DCLog object
 * 
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogClose( DCLog *dclog ) {

  int retval;
  
  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  // If the file is not open, return failure
  if (dclog->fp == NULL)
    return 0;
  
  // Attempt to close the file (unless we are logging to stderr or
  // stdout). If the close fails, we will return an error later.
  if (strcasecmp( dclog->fn, "stderr" ) == 0 ||
      strcasecmp( dclog->fn, "stdout" ) == 0)
    retval = 0;
  else
    retval = fclose( dclog->fp );
  
  // Set the file pointer to NULL (so that this DCLog object cannot be
  // written to again)
  dclog->fp = NULL;
  
  // If fclose() returned EOF, we need to return failure (success
  // otherwise)
  return retval == EOF ? 0 : 1;
  
} // DCLogClose()


/** Writes a message to an open logfile at the specified level.
 *
 * If the global logging level for this object is lower than the specified
 * level, the mesage will not be logged, but success will be returned. If
 * the ts_header member of the DCLog object is set to true, a timestamp /
 * PID header will be written before every log message (see DCLogSetHeader(),
 * above). No newline is appended to your message, so if you want one, put
 * it in your format string. If the level is zero, "ERROR: " will be
 * written before the message.
 *
 * @param *dclog  pointer to a DCLog object
 * @param  lev    level of this log message
 * @param  fmt    printf-style format string
 * @param  ...    arguments to the format string
 * 
 * @return  1 on success, 0 on failure
 */

inline UCHAR DCLogWrite( DCLog *dclog, const UCHAR lev,
                         const char *fmt, ... ) {

  va_list str_args;

  // Make sure that we have a valid DCLog object
  if (dclog == NULL)
    return 0;
  
  // If the FILE object does not exist, bail
  if (dclog->fp == NULL)
    return 0;
  
  // If the user-specified level is higher than the logging level for
  // this DCLog object, return success
  if (lev > dclog->lev) {
    return 1;
  }
  
  // If we are using the unique filename by day of the month feature,
  // DCLOG_FEAT_UNIQUE, see if the day of the month has changed
  if (dclog->features & DCLOG_FEAT_UNIQUE) {

    time_t    current_time;
    struct tm now;

    // What time is it?
    current_time = time( NULL );
    
    // Check for an unset prev_date and set it to the current time
    if (!dclog->prev_date) {
    
      dclog->prev_date = (struct tm *)malloc( sizeof ( struct tm ) );
      localtime_r( &current_time, dclog->prev_date );
      
    } // if (!prev_date)

    // Set the current time
    localtime_r( &current_time, &now );

    // Rotate logfile if the current day of the month is not the same as the
    // previous one
    if (dclog->prev_date->tm_mday != now.tm_mday) {

      char new_fn[256], old_fn[256];
      int  i;
      
      fprintf( dclog->fp,
               "\n\n*******************************************\n\n"
               "Warning: rotating log files due the day of the month changing "
               "from %02d to %02d"
               "\n\n*******************************************\n\n",
               dclog->prev_date->tm_mday, now.tm_mday );

      if (strncpy( new_fn, dclog->fn, 256 ) == NULL ||
          strncpy( old_fn, dclog->fn, 256 ) == NULL) {

        fprintf( stderr, "Failed to copy filename\n" );
        return 0;

      } // if (couldn't copy filename)

      // Snip off the rightmost '.' and everything to the right of it
      // in the filename
      for (i = strlen( new_fn ); i > 0; i--)
        if (new_fn[i] == '.') {
          
          new_fn[i] = '\0';
          break;

        } // if (found rightmost '.')

      if (DCLogClose( dclog ) == 0) {

        fprintf( stderr, "Failed to close file: %s\n", dclog->fn );
        return 0;

      } // if (couldn't close old logfile)
      
      if (DCLogOpen( dclog, new_fn, "w" ) == 0) {
          
        fprintf( stderr,"Failed to open file: %s\n", new_fn );
        return 0;

      } // if (couldn't open new logfile)
    
      fprintf( dclog->fp,
               "\n\n*******************************************\n\n"
               "Warning: rotated logfiles from '%s' to '%s' due the day of the "
               "month changing from %02d to %02d"
               "\n\n*******************************************\n\n",
               old_fn, dclog->fn, dclog->prev_date->tm_mday, now.tm_mday );

      *dclog->prev_date = now;
    
    } // if (rotating logfile)

  } // if (checking date)

  // Do we also need to check the current filesize?
  if (dclog->limit_size) {

    int         size;
    struct stat statbuf;
      
    // If over max file size, rotate the logfile
    if (stat( dclog->fn, &statbuf ) >= 0 &&
        (size = (int)statbuf.st_size) >= dclog->max_file_size) {

      time_t    current_time = time( NULL );
      struct tm now;
      char      old_fn[256];

      fprintf( dclog->fp, "Warning: rotating log files due to the size of the "
               "current logfile, %d bytes, being larger than the maxiumum "
               "allowable size, %d bytes\n", size, dclog->max_file_size );
      
      localtime_r( &current_time, &now );
      if (snprintf( old_fn, 256, "%s_%.2d%.2d%.2d",
                    dclog->fn, now.tm_hour, now.tm_min, now.tm_sec ) < 0) {

        fprintf( stderr, "Failed to write string for old filename\n" );
        return 0;

      } // if (failed to write string)
          
      rename( dclog->fn, old_fn );
      
      if (!_truncate( dclog )) {

        fprintf( stderr, "Failed to truncate logfile: %s\n", dclog->fn );
        return 0;

      } // if (truncating file failed)
        
      fprintf( dclog->fp, "Warning: rotated log files from '%s' to '%s' due to "
               "the size of the current logfile, %d bytes, being larger than the "
               "maxiumum allowable size, %d bytes\n",
               old_fn, dclog->fn, size, dclog->max_file_size );

    } // if (rotating logfiles)

  } // if (checking size)
  
  // If header is on, generate a message header and print it
  if (dclog->features & DCLOG_FEAT_HEADER) {
   
    struct timeval  tv;
    struct tm      *tm;
   
    // If gettimeofday() fails, don't write the header, but try to complain
    if (gettimeofday( &tv, NULL ) == -1)
      fprintf( dclog->fp, "gettimeofday() call failed!\n" );

    else {
      
      tm = localtime( &(tv.tv_sec) );
      
      fprintf( dclog->fp, "%02d:%02d:%02d.%02ld: ", tm->tm_hour,
               tm->tm_min, tm->tm_sec, tv.tv_usec );
      
    } // else (gettimeofday(3) succeeded)
    
  } // if (dclog->features & DCLOG_FEAT_HEADER)

  // If the DCLOG_FEAT_PLEVEL feature is turned on, we need to insert the level's
  // mnemonic
  if (dclog->features & DCLOG_FEAT_PLEVEL)
    fprintf( dclog->fp, "%s ", levels[lev] );

#if 0
  const char *f = fmt;
  static char nlfmt[4096];
  if (dclog->features & DCLOG_FEAT_NEWLINE) {
	strncpy(nlfmt, fmt, 4095);
	int len = strlen(nlfmt);
	nlfmt[len] = '\n';
	nlfmt[len+1] = 0;
	f = nlfmt;
  }
#endif

  // Initialise the variable str_args list
  va_start( str_args, fmt );
  
  // Print the log message
  if (vfprintf( dclog->fp, fmt, str_args ) < 0)
    return 0;
  
  // Finalise the variable str_args list
  va_end( str_args );
  
  return 1;
    
} // DCLogWrite()


/** Sends an alarm.
 *
 * Forks a child process, calls the function specified by the fp function
 * pointer argument (if fp is NULL, no function is called), connects to
 * the SMTP mail server (which should have been set by DCLogSetMailServer()),
 * and sends an alarm email of the following format:
 *
 * From: [program, set by DCLogSetProgram()]@[hostname].
 * To: [page server, set by DCLogSetPageServer()].
 * Subject: [code] : [syn].
 * Body: [controlled by format string fmt].
 * 
 * @param *dclog  pointer to a DCLog object
 * @param  code   alarm code
 * @param  syn    alarm synopsis
 * @param  fmt    detailed description (printf-style format string)
 * @param  ...    arguments to the format string
 * 
 * @return  1 on success, 0 on failure
 */

UCHAR DCLogAlarm( DCLog *dclog, const char *code, const char *syn,
                  const char *fmt, ... ) {

  char msg[DCLOG_MAX_ALARM_SIZE];
  char fn[256];
  
  va_list  str_args;
  FILE    *fp = NULL;
  
  // Make sure that we have valid arguments
  if (dclog == NULL || code == NULL || syn == NULL || fmt == NULL)
    return 0;
  
  // Make sure we know the program code
  if (dclog->from == NULL)
    DCLogSetProgram( dclog, DCLOG_PROG );

  // Write the user's message
  va_start( str_args, fmt );
  vsnprintf( msg, 480, fmt, str_args );
  va_end( str_args );

  if (snprintf( fn, 256, "%s/%s.%s.%ld", dclog->spool, dclog->from, code,
                random() ) < 0)
    return 0;

  if ((fp = fopen( fn, "w" )) == NULL)
    return 0;

  fprintf( fp,
           "To: %s\r\n"
           "From: %s\r\n"
           "Subject: %s: %s\r\n"
           "\r\n"
           "%s"
           "\r\n",
           dclog->to, dclog->from, code, syn, msg );

  fclose( fp );
  
  return 1;
  
} // DCLogAlarm()


// -------------------------------------------------------------------------


// Miscellaneous methods
// -------------------------------------------------------------------------


/** Makes sure the object is able to log.
 *
 * @param *dclog  pointer to a DCLog object
 *
 * @returns  1 on success, 0 on failure
 */

UCHAR DCLogCanLog( const DCLog *dclog ) {

  if (dclog == NULL || dclog->fp == NULL || dclog->fn == NULL)
    return 0;
  else
    return 1;
  
} // DCLogCanLog()


// -------------------------------------------------------------------------
