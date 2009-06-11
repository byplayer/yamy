//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// msgstream.h


#ifndef _MSGSTREAM_H
#  define _MSGSTREAM_H

#  include "misc.h"
#  include "stringtool.h"
#  include "multithread.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// msgstream

/** msgstream.

    <p>Before writing to omsgstream, you must acquire lock by calling
    <code>acquire()</code>.  Then after completion of writing, you
    must call <code>release()</code>.</p>
    
    <p>Omsgbuf calls <code>PostMessage(hwnd, messageid, 0,
    (LPARAM)omsgbuf)</code> to notify that string is ready to get.
    When the window (<code>hwnd</code>) get the message, you can get
    the string containd in the omsgbuf by calling
    <code>acquireString()</code>.  After calling
    <code>acquireString()</code>, you must / call releaseString().</p>

*/

template<class T, size_t SIZE = 1024,
  class TR = std::char_traits<T>, class A = std::allocator<T> >
class basic_msgbuf : public std::basic_streambuf<T, TR>, public SyncObject
{
public:
  typedef std::basic_string<T, TR, A> String;	/// 
  typedef std::basic_streambuf<T, TR> Super;	///
  
private:
  HWND m_hwnd;					/** window handle for
						    notification */
  UINT m_messageId;				/// messageid for notification
  T *m_buf;					/// for streambuf
  String m_str;					/// for notification
  CriticalSection m_cs;				/// lock
  A m_allocator;				/// allocator
  
  /** debug level.
      if ( m_msgDebugLevel &lt;= m_debugLevel ), message is displayed
  */
  int m_debugLevel;
  int m_msgDebugLevel;				///

private:
  basic_msgbuf(const basic_msgbuf &);		/// disable copy constructor

public:
  ///
  basic_msgbuf(UINT i_messageId, HWND i_hwnd = 0)
    : m_hwnd(i_hwnd),
      m_messageId(i_messageId),
      m_buf(m_allocator.allocate(SIZE, 0)),
      m_debugLevel(0),
      m_msgDebugLevel(0)
  {
    ASSERT(m_buf);
    setp(m_buf, m_buf + SIZE);
  }
  
  ///
  ~basic_msgbuf()
  {
    sync();
    m_allocator.deallocate(m_buf, SIZE);
  }
  
  /// attach/detach a window
  basic_msgbuf* attach(HWND i_hwnd)
  {
    Acquire a(&m_cs);
    ASSERT( !m_hwnd && i_hwnd );
    m_hwnd = i_hwnd;
    if (!m_str.empty())
      PostMessage(m_hwnd, m_messageId, 0, (LPARAM)this);
    return this;
  }
  
  ///
  basic_msgbuf* detach()
  {
    Acquire a(&m_cs);
    sync();
    m_hwnd = 0;
    return this;
  }
  
  /// get window handle
  HWND getHwnd() const { return m_hwnd; }
  
  /// is a window attached ?
  int is_open() const { return !!m_hwnd; }
  
  /// acquire string and release the string
  const String &acquireString()
  {
    m_cs.acquire();
    return m_str;
  }
  
  ///
  void releaseString()
  {
    m_str.resize(0);
    m_cs.release();
  }

  /// set debug level
  void setDebugLevel(int i_debugLevel)
  {
    m_debugLevel = i_debugLevel;
  }
  
  ///
  int getDebugLevel() const { return m_debugLevel; }
  
  // for stream
  typename Super::int_type overflow(typename Super::int_type i_c = TR::eof())
  {
    if (sync() == TR::eof()) // sync before new buffer created below
      return TR::eof();
    
    if (i_c != TR::eof())
    {
      *pptr() = TR::to_char_type(i_c);
      pbump(1);
      sync();
    }
    return TR::not_eof(i_c); // return something other than EOF if successful
  }
  
  // for stream
  int sync()
  {
    T *begin = pbase();
    T *end = pptr();
    T *i;
    for (i = begin; i < end; ++ i)
      if (_istlead(*i))
	++ i;
    if (i == end)
    {
      if (m_msgDebugLevel <= m_debugLevel)
	m_str += String(begin, end - begin);
      setp(m_buf, m_buf + SIZE);
    }
    else // end < i
    {
      if (m_msgDebugLevel <= m_debugLevel)
	m_str += String(begin, end - begin - 1);
      m_buf[0] = end[-1];
      setp(m_buf, m_buf + SIZE);
      pbump(1);
    }
    return TR::not_eof(0);
  }

  // sync object
  
  /// begin writing
  virtual void acquire()
  {
    m_cs.acquire();
  }

  /// begin writing
  virtual void acquire(int i_msgDebugLevel)
  {
    m_cs.acquire();
    m_msgDebugLevel = i_msgDebugLevel;
  }
  
  /// end writing
  virtual void release()
  {
    if (!m_str.empty())
      PostMessage(m_hwnd, m_messageId, 0, reinterpret_cast<LPARAM>(this));
    m_msgDebugLevel = m_debugLevel;
    m_cs.release();
  }
};


///
template<class T, size_t SIZE = 1024,
  class TR = std::char_traits<T>, class A = std::allocator<T> >
class basic_omsgstream : public std::basic_ostream<T, TR>, public SyncObject
{
public:
  typedef std::basic_ostream<T, TR> Super;	/// 
  typedef basic_msgbuf<T, SIZE, TR, A> StreamBuf; /// 
  typedef std::basic_string<T, TR, A> String;	/// 
  
private:
  StreamBuf m_streamBuf;			/// 

public:
  ///
  explicit basic_omsgstream(UINT i_messageId, HWND i_hwnd = 0)
    : Super(&m_streamBuf), m_streamBuf(i_messageId, i_hwnd)
  {
  }
  
  ///
  virtual ~basic_omsgstream()
  {
  }
  
  ///
  StreamBuf *rdbuf() const
  {
    return const_cast<StreamBuf *>(&m_streamBuf);
  }

  /// attach a msg control
  void attach(HWND i_hwnd)
  {
    m_streamBuf.attach(i_hwnd);
  }

  /// detach a msg control
  void detach()
  {
    m_streamBuf.detach();
  }

  /// get window handle of the msg control
  HWND getHwnd() const
  {
    return m_streamBuf.getHwnd();
  }

  /// is the msg control attached ?
  int is_open() const
  {
    return m_streamBuf.is_open();
  }

  /// set debug level
  void setDebugLevel(int i_debugLevel)
  {
    m_streamBuf.setDebugLevel(i_debugLevel);
  }
  
  ///
  int getDebugLevel() const
  {
    return m_streamBuf.getDebugLevel();
  }

  /// acquire string and release the string
  const String &acquireString()
  {
    return m_streamBuf.acquireString();
  }
  
  ///
  void releaseString()
  {
    m_streamBuf->releaseString();
  }

  // sync object
  
  /// begin writing
  virtual void acquire()
  {
    m_streamBuf.acquire();
  }
  
  /// begin writing
  virtual void acquire(int i_msgDebugLevel)
  {
    m_streamBuf.acquire(i_msgDebugLevel);
  }
  
  /// end writing
  virtual void release()
  {
    m_streamBuf.release();
  }
};

///
typedef basic_omsgstream<_TCHAR> tomsgstream;


#endif // !_MSGSTREAM_H
