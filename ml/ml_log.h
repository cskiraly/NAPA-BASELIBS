/*
 * Copyright (c) 2009 NEC Europe Ltd. All Rights Reserved.
 * Authors: Kristian Beckers  <beckers@nw.neclab.eu>
 *          Sebastian Kiesel  <kiesel@nw.neclab.eu>
 *
 * This file is part of the Messaging Library.
 *
 * The Messaging Library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The Messaging Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Messaging Library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _ML_LOG_H
#define _ML_LOG_H

#include <stdlib.h>
#include <stdio.h>

extern int ml_log_level;

extern void setLogLevel(int ll);

#define DPRINT(ll, format, ... )  {struct timeval tnow; if(ll <= ml_log_level) {gettimeofday(&tnow,NULL); fprintf(stderr, "%ld.%03ld "format, tnow.tv_sec, tnow.tv_usec/1000, ##__VA_ARGS__ );fprintf(stderr,format[strlen(format)-1] == '\n'?"":"\n"); fflush(stderr);}}

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
