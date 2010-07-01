/* =========================================================================
 * File: test-dclog.c
 *
 * Copyright (c) 2004 and onwards, Twenty First Century Communications, Inc.
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
 *   Unit test for dclog.
 *
 * USAGE:
 *
 *   test-dclog
 *
 * EXAMPLES:
 *
 *   ***
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
 *   Josh Glover <josh.glover@tfcci.com> (2004/07/09): Initial revision
 * =========================================================================
 */

// Standard library headers
#include <stdio.h>
#include <stdlib.h>

// dclog headers
#include <dclog.h>


int main( void ) {

  DCLog *dclog = NULL;

  UCHAR lev;
  
  static const char log_simple[64] = "test-dclog.log.simple";
  static const char log_header[64] = "test-dclog.log.header";
  static const char log_level[64]  = "test-dclog.log.level";
  static const char log_limit[64]  = "test-dclog.log.limit";
  static const char log_unique[64] = "test-dclog.log.unique";
  static const char log_alarm[64]  = "test-dclog.log.alarm";

  static const char mail_server[64] = "pagedev.tfcc.com";
  static const char page_server[64] = "page@pagedev.tfcc.com";
  static const char spool_dir[64]   = "/tmp/ald-spool";
  
  // Simple log
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing simple DCLog object... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to UCHAR_MAX... " );
  if (!DCLogSetLevel( dclog, UCHAR_MAX )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_simple );
  if (!DCLogOpen( dclog, log_simple, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Logging to file '%s'... ", log_simple );
  if (!DCLogWrite( dclog, DCLOG_INFO, "Hi, I am logfile '%s'.\nI am a very "
                   "simple logfile, with no neat features turned on.\n",
                   log_simple )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogWrite() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Closing logfile '%s'... ", log_simple );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying simple DCLog object... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );

  
  // -------------------------------------------------------------------------


  // Log with header turned on
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing DCLog object with headers... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to UCHAR_MAX... " );
  if (!DCLogSetLevel( dclog, UCHAR_MAX )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Turning on log message headers... " );
  if (!DCLogSetHeader( dclog, 1 )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetHeader() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_header );
  if (!DCLogOpen( dclog, log_header, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Logging to file '%s'... ", log_header );
  if (!DCLogWrite( dclog, DCLOG_INFO, "Hi, I am logfile '%s'.\nI have log "
                   "message headers turned on.\n", log_header )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogWrite() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Closing logfile '%s'... ", log_header );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying DCLog object with headers... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );

  
  // -------------------------------------------------------------------------


  // Log with level printing turned on
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing DCLog object with levels... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to DCLOG_INFO... " );
  if (!DCLogSetLevel( dclog, DCLOG_INFO )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Turning on log message level printing... " );
  if (!DCLogSetPrintLevel( dclog, 1 )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetPrintLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_level );
  if (!DCLogOpen( dclog, log_level, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  for (lev = 0; lev < DCLOG_PROFILE + 1; lev++) {

    fprintf( stderr, "Logging to file '%s' at level %s... ", log_level,
             DCLogLevelToString( lev ) );
    if (!DCLogWrite( dclog, lev, "Hi, I am logfile '%s'.\nI have log "
                     "message levels turned on.\nAs you can see, this message "
                     "is being logged at level %s.\nIsn't log level printing "
                     "great?\n", log_level, DCLogLevelToString( lev ) )) {

      fprintf( stderr, "failed!\n" );
      return 1;

    } // if (DCLogWrite() failed)
    fprintf( stderr, "OK\n" );
    
  } // for (logging at different levels)

  fprintf( stderr, "Closing logfile '%s'... ", log_level );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying DCLog object with levels... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );
  

  // -------------------------------------------------------------------------


  // Log with size limiting turned on
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing DCLog object with size limiting... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to DCLOG_INFO... " );
  if (!DCLogSetLevel( dclog, DCLOG_INFO )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Turning on size limiting and setting size to 256 bytes... " );
  if (!DCLogSetMaxFileSize( dclog, 256 )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetMaxFileSize() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_limit );
  if (!DCLogOpen( dclog, log_limit, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  for (lev = 0; lev < UCHAR_MAX; lev++) {

    fprintf( stderr, "Logging to file '%s' at level %s... ", log_limit,
             DCLogLevelToString( DCLOG_INFO ) );
    if (!DCLogWrite( dclog, DCLOG_INFO, "Hi, I am logfile '%s'.\nI have file "
                     "size limiting turned on.\n", log_limit )) {

      fprintf( stderr, "failed!\n" );
      return 1;

    } // if (DCLogWrite() failed)
    fprintf( stderr, "OK\n" );
    
  } // for (logging many times)

  fprintf( stderr, "Closing logfile '%s'... ", log_limit );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying DCLog object with limits... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );
  

  // -------------------------------------------------------------------------


  // Log with unique turned on (default)
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing DCLog object with unique filenames... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to UCHAR_MAX... " );
  if (!DCLogSetLevel( dclog, UCHAR_MAX )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Turning on unique... " );
  if (!DCLogSetUniqueByDay( dclog, 1 )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetUniqueByDay() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_unique );
  if (!DCLogOpen( dclog, log_unique, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Logging to file '%s'... ", log_unique );
  if (!DCLogWrite( dclog, DCLOG_INFO, "Hi, I am logfile '%s'.\nI have unique "
                   "filenames turned on.\n", log_unique )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogWrite() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Closing logfile '%s'... ", log_unique );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying DCLog object with unique filenames... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );

  
  // -------------------------------------------------------------------------


  // Log with unique turned on (custom format string)
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing DCLog object with a custom format string... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to UCHAR_MAX... " );
  if (!DCLogSetLevel( dclog, UCHAR_MAX )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Turning on unique... " );
  if (!DCLogSetTimestampFormat( dclog, ".%Y-%m-%d" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetTimestampFormat() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_unique );
  if (!DCLogOpen( dclog, log_unique, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Logging to file '%s'... ", log_unique );
  if (!DCLogWrite( dclog, DCLOG_INFO, "Hi, I am logfile '%s'.\nI have unique "
                   "filenames turned on.\n", log_unique )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogWrite() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Closing logfile '%s'... ", log_unique );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying DCLog object with unique filenames... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );

  
  // -------------------------------------------------------------------------


  // Log with alarms
  // -------------------------------------------------------------------------


  fprintf( stderr, "Constructing DCLog object for alarming... " );
  if ((dclog = NewDCLog()) == NULL) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (NewDCLog() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting log level to UCHAR_MAX... " );
  if (!DCLogSetLevel( dclog, UCHAR_MAX )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetLevel() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting mail server to '%s'... ", mail_server );
  if (!DCLogSetMailServer( dclog, mail_server )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetMailServer() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting page server to '%s'... ", page_server );
  if (!DCLogSetPageServer( dclog, page_server )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetPageServer() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Setting spool directory to '%s'... ", spool_dir );
  if (!DCLogSetSpool( dclog, spool_dir )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogSetSpool() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Opening logfile '%s'... ", log_alarm );
  if (!DCLogOpen( dclog, log_alarm, "w" )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogOpen() failed)
  fprintf( stderr, "OK\n" );

  for (lev = 0; lev < 10; lev++) {

    char code[32];

    snprintf( code, 32, "test_dclog_%02hhu", lev );
    fprintf( stderr, "Alarming with code: %s... ", code );
    DCLogWrite( dclog, DCLOG_INFO, "Alarming with code: %s... ", code );
    if (!DCLogAlarm( dclog, code, "test alarm", "This is a test alarm with "
                     "an error code of %02hhu.", lev )) {
      
      fprintf( stderr, "failed!\n" );
      return 1;

    } // if (DCLogAlarm() failed)
    fprintf( stderr, "OK\n" );

  } // for (alarming)

  fprintf( stderr, "Closing logfile '%s'... ", log_alarm );
  if (!DCLogClose( dclog )) {

    fprintf( stderr, "failed!\n" );
    return 1;

  } // if (DCLogClose() failed)
  fprintf( stderr, "OK\n" );

  fprintf( stderr, "Destroying DCLog object with headers... " );
  if (!DestroyDCLog( dclog )) {

    fprintf( stderr, "failed\n" );
    return 1;

  } // if (DestroyDCLog() failed)
  fprintf( stderr, "OK\n" );

  
  // -------------------------------------------------------------------------


  return 0;

} // main()
