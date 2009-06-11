//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// multithread.h


#ifndef _MULTITHREAD_H
#  define _MULTITHREAD_H

#  include <windows.h>


///
class SyncObject
{
public:
  ///
  virtual void acquire() = 0;
  ///
  virtual void acquire(int ) { acquire(); }
  ///
  virtual void release() = 0;
};


///
class CriticalSection : public SyncObject
{
  CRITICAL_SECTION m_cs;			///

public:
  ///
  CriticalSection() { InitializeCriticalSection(&m_cs); }
  ///
  ~CriticalSection() { DeleteCriticalSection(&m_cs); }
  ///
  void acquire() { EnterCriticalSection(&m_cs); }
  ///
  void release() { LeaveCriticalSection(&m_cs); }
};


///
class Acquire
{
  SyncObject *m_so;	///
  
public:
  ///
  Acquire(SyncObject *i_so) : m_so(i_so) { m_so->acquire(); }
  ///
  Acquire(SyncObject *i_so, int i_n) : m_so(i_so) { m_so->acquire(i_n); }
  ///
  ~Acquire() { m_so->release(); }
};


#endif // !_MULTITHREAD_H
