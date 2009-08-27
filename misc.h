//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc.h


#ifndef _MISC_H
#  define _MISC_H

#  include "compiler_specific.h"
#  include <windows.h>
#  include <cassert>

#define YAMY_SUCCESS						0
#define YAMY_ERROR_ON_GET_USERNAME			1001
#define YAMY_ERROR_INSUFFICIENT_BUFFER		1002
#define YAMY_ERROR_NO_MEMORY				1003
#define YAMY_ERROR_ON_GET_LOGONUSERNAME		1004
#define YAMY_ERROR_ON_GET_SECURITYINFO		1005
#define YAMY_ERROR_ON_GET_DACL				1006
#define YAMY_ERROR_ON_INITIALIZE_ACL		1007
#define YAMY_ERROR_ON_GET_ACE				1008
#define YAMY_ERROR_ON_ADD_ACE				1009
#define YAMY_ERROR_ON_ADD_ALLOWED_ACE		1010
#define YAMY_ERROR_ON_SET_SECURITYINFO		1011
#define YAMY_ERROR_ON_OPEN_YAMY_PROCESS		1012
#define YAMY_ERROR_ON_OPEN_YAMY_TOKEN		1013
#define YAMY_ERROR_ON_IMPERSONATE			1014
#define YAMY_ERROR_ON_REVERT_TO_SELF		1015
#define YAMY_ERROR_ON_OPEN_CURRENT_PROCESS	1016
#define YAMY_ERROR_ON_LOOKUP_PRIVILEGE		1017
#define YAMY_ERROR_ON_ADJUST_PRIVILEGE		1018
#define YAMY_ERROR_ON_OPEN_WINLOGON_PROCESS	1019
#define YAMY_ERROR_ON_VIRTUALALLOCEX		1020
#define YAMY_ERROR_ON_WRITEPROCESSMEMORY	1021
#define YAMY_ERROR_ON_CREATEREMOTETHREAD	1022
#define YAMY_ERROR_TIMEOUT_INJECTION		1023
#define YAMY_ERROR_RETRY_INJECTION_SUCCESS	1024
#define YAMY_ERROR_ON_READ_SCANCODE_MAP		1025
#define YAMY_ERROR_ON_WRITE_SCANCODE_MAP	1026
#define YAMY_ERROR_ON_GET_WINLOGON_PID		1027

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
