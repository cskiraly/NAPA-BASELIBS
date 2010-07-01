/* =========================================================================
 * File: common-types.h
 *
 * Copyright (c) 2005 and onwards Josh Glover <fasthash@jmglov.net>
 *
 * LICENCE:
 *
 *   This file is distributed under the terms of the BSD-2 License.
 *   See the COPYING file, which should have been distributed with
 *   this file, for details. If you did not receive the COPYING file,
 *   see:
 *
 *   http://www.jmglov.net/opensource/licenses/bsd.txt
 *
 * DESCRIPTION:
 *
 *   Defines common type aliases and NULL values for each type.
 *
 * USAGE:
 *
 *   #include <common-types.h>
 *     
 * EXAMPLES:
 *
 *   UCHAR my_unsigned_char = UCHAR_NULL;
 *   int   my_int           = INT_NULL;
 *
 * DEPENDENCIES:
 *
 *   None
 *
 * TODO:
 *
 *   - Nothing, this code is perfect
 *
 * MODIFICATIONS:
 *
 *   Josh Glover <fasthash@jmglov.net> (2003/12/04): Initial revision
 * ========================================================================
 */

#include <limits.h>

#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

// Type aliases
// -------------------------------------------------------------------------

#ifndef UCHAR
#define UCHAR unsigned char
#endif // #ifndef UCHAR

#ifndef USHORT
#define USHORT unsigned short
#endif // #ifndef USHORT

#ifndef UINT
#define UINT unsigned int
#endif // #ifndef UINT

#ifndef ULONG
#define ULONG unsigned long
#endif // #ifndef ULONG

#ifndef ULONGLONG
#define ULONGLONG unsigned long long
#endif // #ifndef ULONGLONG

// -------------------------------------------------------------------------

// NULL values (use the highest possible value)
// -------------------------------------------------------------------------

#ifndef CHAR_NULL
#define CHAR_NULL  0xff
#endif // #ifndef CHAR_NULL

#ifndef UCHAR_NULL
#define UCHAR_NULL  0xff
#endif // #ifndef UCHAR_NULL

#ifndef SHORT_NULL
#define SHORT_NULL 0xffff
#endif // #ifndef SHORT_NULL

#ifndef USHORT_NULL
#define USHORT_NULL 0xffff
#endif // #ifndef USHORT_NULL

#ifndef UINT_NULL
#define UINT_NULL   0xffffffff
#endif // #ifndef UINT_NULL

#ifndef UINT_NULL
#define UINT_NULL   0xffffffff
#endif // #ifndef UINT_NULL

#ifndef LONG_NULL
#define LONG_NULL  0xffffffff
#endif // #ifndef LONG_NULL

#ifndef ULONG_NULL
#define ULONG_NULL  0xffffffff
#endif // #ifndef ULONG_NULL

#ifndef LONGLONG_NULL
#define LONGLONG_NULL  0xffffffffffff
#endif // #ifndef LONGLONG_NULL

#ifndef ULONGLONG_NULL
#define ULONGLONG_NULL  0xffffffffffff
#endif // #ifndef ULONGLONG_NULL

// -------------------------------------------------------------------------

#endif // #ifndef __COMMON_TYPES_H__
