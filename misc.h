//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc.h


#ifndef _MISC_H
#  define _MISC_H

#  include "compiler_specific.h"
#  include <windows.h>
#  include <cassert>


typedef unsigned char u_char;			/// unsigned char
typedef unsigned short u_short;			/// unsigned short
typedef unsigned long u_long;			/// unsigned long

typedef char int8;				/// signed 8bit
typedef short int16;				/// signed 16bit
typedef long int32;				/// signed 32bit
typedef unsigned char u_int8;			/// unsigned 8bit
typedef unsigned short u_int16;			/// unsigned 16bit
typedef unsigned long u_int32;			/// unsigned 32bit
#if defined(__BORLANDC__)
typedef unsigned __int64 u_int64;			/// unsigned 64bit
#elif _MSC_VER <= 1300
typedef unsigned _int64 u_int64;			/// unsigned 64bit
#else
typedef unsigned long long u_int64;			/// unsigned 64bit
#endif


#  ifdef NDEBUG
#    define ASSERT(i_exp)
#    define CHECK(i_cond, i_exp)	i_exp
#    define CHECK_TRUE(i_exp)		i_exp
#    define CHECK_FALSE(i_exp)		i_exp
#  else // NDEBUG
/// assertion. i_exp is evaluated only in debug build
#    define ASSERT(i_exp)		assert(i_exp)
/// assertion, but i_exp is always evaluated
#    define CHECK(i_cond, i_exp)	assert(i_cond (i_exp))
/// identical to CHECK(!!, i_exp)
#    define CHECK_TRUE(i_exp)		assert(!!(i_exp))
/// identical to CHECK(!, i_exp)
#    define CHECK_FALSE(i_exp)		assert(!(i_exp))
#  endif // NDEBUG


/// get number of array elements
#  define NUMBER_OF(i_array) (sizeof(i_array) / sizeof((i_array)[0]))

/// max path length
#  define GANA_MAX_PATH		(MAX_PATH * 4)

/// max length of global atom
#  define GANA_MAX_ATOM_LENGTH	256

#  undef MAX
/// redefine MAX macro
#  define MAX(a, b)	(((b) < (a)) ? (a) : (b))

#  undef MIN
/// redefine MIN macro
#  define MIN(a, b)	(((a) < (b)) ? (a) : (b))


#endif // !_MISC_H
