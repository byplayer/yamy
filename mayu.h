//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// mayu.h


#ifndef _MAYU_H
#  define _MAYU_H


///
#ifdef USE_INI
#  define MAYU_REGISTRY_ROOT 0, _T("yamy")
#else // !USE_INI
#  define MAYU_REGISTRY_ROOT HKEY_CURRENT_USER, _T("Software\\gimy.net\\yamy")
#endif // !USE_INI

///
#  define MUTEX_MAYU_EXCLUSIVE_RUNNING		\
_T("{46269F4D-D560-40f9-B38B-DB5E280FEF47}")

///
#  define MUTEX_YAMYD_BLOCKER		\
_T("{267C9CA1-C4DC-4011-B78A-745781FD60F4}")

///
#  define MAX_MAYU_REGISTRY_ENTRIES 256


#endif // _MAYU_H
