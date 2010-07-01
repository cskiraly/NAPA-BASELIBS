#ifndef _GRAPES_H
#define _GRAPES_H

/** @mainpage

   @section intro Introduction

   The GRAPES prototype code is the reference implementation for the evolving GRAPES (aka. PULSE+) API.
   The main point is to demonstrate the intended final <b>structure</b> of the implementation, with
   omitting as much details as possible. Thus, the pre-prototype contains C language bindings for
   most of the datatypes, API calls etc. without the actual implementation, or with a <i>stub</i>
   implementation only for internal testing and to demonstrate the intended structure.

   Thus, most of the information found in this documentation is intended for GRAPES developers.

   @section structure Code Structure

   Each module (listed below) is separated into a sub-directory within the source tree.
   The public interfaces (APIs) are located in the <i>include/</i> sub-directory in the 
   correspondingly named .h file (e.g. ml.h for the Messaging Layer which resides in sub-directory <i>ml/</i>
   Module subdirectories are as follows:
	- ml/ - <b>Messaging Layer</b> - ml.h
	- mon/ - <b>Monitoring Layer</b> - mon.h
	- rep/ - <b>Repository</b> - repoclient.h
	- som/ - <b>Scheduler and Overlay Manager</b> - som.h
	- ul/ - <b>User</b> (interface) <b>Layer</b> - ul.h

   @section deps Dependencies

   The pre-prototype code uses a number of external open-source libraries.
   Most importantly, the <A HREF="http://monkey.org/~provos/libevent/">libevent</A> framework 
   is used to implement the reactive architecture, which was chosen as the 
   model of computation for GRAPES.
   Additionally, further open-source libraries are used to perform common tasks
   such as config file parsing and logging.
   
   Included libraries are:
	- <A HREF="http://monkey.org/~provos/libevent/">libevent</A>: event loop library to implement 
	the reactive architecture
	- <A HREF="http://www.nongnu.org/confuse/">libconfuse</A>: configuration file parsing library
	- <A HREF="http://sourceforge.net/projects/dclog/">dclog</A>: log message writer library

   @section copyright Copyright and related issues

   This code is has been released to the public.
*/


/** @file grapes.h
 *
 * Common definitions for the GRAPES framework reference implementation, to be included by all 
 * applications using GRAPES.
 *
 * This header file contains basic, system-wide definitions (such as PeerID) which are necessary
 * for different GRAPES modules to interact.
 * 
 * Libraries and system-wide dependencies are included from this file, for example standard C
 * header files for common types (e.g. <i>bool</i> or <i>byte<i>
 *
 */

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef byte
/** 8-bit unsigned type */
typedef u_char byte;
#endif

/** General-purpose opaque handle type */
typedef void *HANDLE;

/** Opaque network-wide PeerID */
typedef char PeerID[64];

/** eventbase pointer for the libevent2 framework */
extern struct event_base *eventbase;

#ifdef __cplusplus
extern "C" {
#endif

/** Function to initialize the GRAPES framework.
 *
 * This function has to be called first, before any other GRAPES-provided function.
 
   @param event2_base libevent2 base structure, provided by the external application
*/
void grapesInit(void *event2_base);

/** Function to yield control (sleep) 
*/
void grapesYield();

/**
  Register a function to be called periodically.

  Use this function for scheduling periodic activity. 
  The callback function supports one extra argument besides its own handle.

  @param[in] start time of first exection, in timeval units from now. NULL (or timeval {0,0}) means "now" or "asap". 
  @param[in] frequency invocation frequency .
  @param[in] cb callback function to be called periodically.
  @param[in] cbarg argument for the callback function.
  @return handle to the function or NULL on error.
  @see grapesStopPeriodic
*/
HANDLE grapesSchedulePeriodic(const struct timeval *start, double frequency, void(*cb)(HANDLE handle, void *arg), void *cbarg);

/**
  Stop periodically calling the callback registered to the handle.

  @param h handle to the callback.
  @see grapesStartPeriodic
*/
void grapesStopPeriodic(HANDLE h);


/** Convenience function to print a timeval */
const char *timeval2str(const struct timeval *ts);

#ifdef __cplusplus
}
#endif

#endif
