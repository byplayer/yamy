//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// stringtool.cpp


#include "stringtool.h"
#include "array.h"
#include <locale>
#include <malloc.h>
#include <mbstring.h>


/* ************************************************************************** *
   
STRLCPY(3)                OpenBSD Programmer's Manual               STRLCPY(3)

NAME
     strlcpy, strlcat - size-bounded string copying and concatenation



SYNOPSIS
     #include <string.h>

     size_t
     strlcpy(char *dst, const char *src, size_t size);

     size_t
     strlcat(char *dst, const char *src, size_t size);

DESCRIPTION
     The strlcpy() and strlcat() functions copy and concatenate strings re-
     spectively.  They are designed to be safer, more consistent, and less er-
     ror prone replacements for strncpy(3) and strncat(3).  Unlike those func-
     tions, strlcpy() and strlcat() take the full size of the buffer (not just
     the length) and guarantee to NUL-terminate the result (as long as size is
     larger than 0).  Note that you should include a byte for the NUL in size.

     The strlcpy() function copies up to size - 1 characters from the NUL-ter-
     minated string src to dst, NUL-terminating the result.

     The strlcat() function appends the NUL-terminated string src to the end
     of dst. It will append at most size - strlen(dst) - 1 bytes, NUL-termi-
     nating the result.

RETURN VALUES
     The strlcpy() and strlcat() functions return the total length of the
     string they tried to create.  For strlcpy() that means the length of src.
     For strlcat() that means the initial length of dst plus the length of
     src. While this may seem somewhat confusing it was done to make trunca-
     tion detection simple.

EXAMPLES
     The following code fragment illustrates the simple case:

           char *s, *p, buf[BUFSIZ];

           ...

           (void)strlcpy(buf, s, sizeof(buf));
           (void)strlcat(buf, p, sizeof(buf));

     To detect truncation, perhaps while building a pathname, something like
     the following might be used:

           char *dir, *file, pname[MAXPATHNAMELEN];

           ...

           if (strlcpy(pname, dir, sizeof(pname)) >= sizeof(pname))
                   goto toolong;
           if (strlcat(pname, file, sizeof(pname)) >= sizeof(pname))
                   goto toolong;

     Since we know how many characters we copied the first time, we can speed
     things up a bit by using a copy instead on an append:

           char *dir, *file, pname[MAXPATHNAMELEN];
           size_t n;

           ...

           n = strlcpy(pname, dir, sizeof(pname));
           if (n >= sizeof(pname))
                   goto toolong;
           if (strlcpy(pname + n, file, sizeof(pname) - n) >= sizeof(pname)-n)
                   goto toolong;

     However, one may question the validity of such optimizations, as they de-
     feat the whole purpose of strlcpy() and strlcat().  As a matter of fact,
     the first version of this manual page got it wrong.

SEE ALSO
     snprintf(3),  strncat(3),  strncpy(3)

OpenBSD 2.6                      June 22, 1998                               2


-------------------------------------------------------------------------------

Source: OpenBSD 2.6 man pages. Copyright: Portions are copyrighted by BERKELEY
SOFTWARE DESIGN, INC., The Regents of the University of California,
Massachusetts Institute of Technology, Free Software Foundation, FreeBSD Inc.,
and others.

* ************************************************************************** */


// copy
template <class T>
static inline size_t xstrlcpy(T *o_dest, const T *i_src, size_t i_destSize)
{
  T *d = o_dest;
  const T *s = i_src;
  size_t n = i_destSize;

  ASSERT( o_dest != NULL );
  ASSERT( i_src != NULL );

  // Copy as many bytes as will fit
  if (n != 0 && --n != 0)
  {
    do
    {
      if ((*d++ = *s++) == 0)
	break;
    } while (--n != 0);
  }

  // Not enough room in o_dest, add NUL and traverse rest of i_src
  if (n == 0)
  {
    if (i_destSize != 0)
      *d = T();					// NUL-terminate o_dest
    while (*s++)
      ;
  }

  return (s - i_src - 1);			// count does not include NUL
}


// copy
size_t strlcpy(char *o_dest, const char *i_src, size_t i_destSize)
{
  return xstrlcpy(o_dest, i_src, i_destSize);
}


// copy
size_t wcslcpy(wchar_t *o_dest, const wchar_t *i_src, size_t i_destSize)
{
  return xstrlcpy(o_dest, i_src, i_destSize);
}


// copy 
size_t mbslcpy(unsigned char *o_dest, const unsigned char *i_src,
	       size_t i_destSize)
{
  unsigned char *d = o_dest;
  const unsigned char *s = i_src;
  size_t n = i_destSize;

  ASSERT( o_dest != NULL );
  ASSERT( i_src != NULL );

  if (n == 0)
    return strlen(reinterpret_cast<const char *>(i_src));
  
  // Copy as many bytes as will fit
  for (-- n; *s && 0 < n; -- n)
  {
    if (_ismbblead(*s))
    {
      if (!(s[1] && 2 <= n))
	break;
      *d++ = *s++;
      -- n;
    }
    *d++ = *s++;
  }
  *d = '\0';

  for (; *s; ++ s)
    ;
  
  return s - i_src;
}


/// stream output
tostream &operator<<(tostream &i_ost, const tstringq &i_data)
{
  i_ost << _T("\"");
  for (const _TCHAR *s = i_data.c_str(); *s; ++ s)
  {
    switch (*s)
    {
      case _T('\a'): i_ost << _T("\\a"); break;
      case _T('\f'): i_ost << _T("\\f"); break;
      case _T('\n'): i_ost << _T("\\n"); break;
      case _T('\r'): i_ost << _T("\\r"); break;
      case _T('\t'): i_ost << _T("\\t"); break;
      case _T('\v'): i_ost << _T("\\v"); break;
      case _T('"'): i_ost << _T("\\\""); break;
      default:
	if (_istlead(*s))
	{
	  _TCHAR buf[3] = { s[0], s[1], 0 };
	  i_ost << buf;
	  ++ s;
	}
	else if (_istprint(*s))
	{
	  _TCHAR buf[2] = { *s, 0 };
	  i_ost << buf;
	}
	else
	{
	  i_ost << _T("\\x");
	  _TCHAR buf[5];
#ifdef _UNICODE
	  _sntprintf(buf, NUMBER_OF(buf), _T("%04x"), *s);
#else
	  _sntprintf(buf, NUMBER_OF(buf), _T("%02x"), *s);
#endif
	  i_ost << buf;
	}
	break;
    }
  }
  i_ost << _T("\"");
  return i_ost;
}


// interpret meta characters such as \n
tstring interpretMetaCharacters(const _TCHAR *i_str, size_t i_len,
				const _TCHAR *i_quote,
				bool i_doesUseRegexpBackReference)
{
  // interpreted string is always less than i_len
  Array<_TCHAR> result(i_len + 1);
  // destination
  _TCHAR *d = result.get();
  // end pointer
  const _TCHAR *end = i_str + i_len;
  
  while (i_str < end && *i_str)
  {
    if (*i_str != _T('\\'))
    {
      if (_istlead(*i_str) && *(i_str + 1) && i_str + 1 < end)
	*d++ = *i_str++;
      *d++ = *i_str++;
    }
    else if (*(i_str + 1) != _T('\0'))
    {
      i_str ++;
      if (i_quote && _tcschr(i_quote, *i_str))
	*d++ = *i_str++;
      else
	switch (*i_str)
	{
	  case _T('a'): *d++ = _T('\x07'); i_str ++; break;
	    //case _T('b'): *d++ = _T('\b'); i_str ++; break;
	  case _T('e'): *d++ = _T('\x1b'); i_str ++; break;
	  case _T('f'): *d++ = _T('\f'); i_str ++; break;
	  case _T('n'): *d++ = _T('\n'); i_str ++; break;
	  case _T('r'): *d++ = _T('\r'); i_str ++; break;
	  case _T('t'): *d++ = _T('\t'); i_str ++; break;
	  case _T('v'): *d++ = _T('\v'); i_str ++; break;
	    //case _T('?'): *d++ = _T('\x7f'); i_str ++; break;
	    //case _T('_'): *d++ = _T(' '); i_str ++; break;
	    //case _T('\\'): *d++ = _T('\\'); i_str ++; break;
	  case _T('\''): *d++ = _T('\''); i_str ++; break;
	  case _T('"'): *d++ = _T('"'); i_str ++; break;
	  case _T('\\'): *d++ = _T('\\'); i_str ++; break;
	  case _T('c'): // control code, for example '\c[' is escape: '\x1b'
	    i_str ++;
	    if (i_str < end && *i_str)
	    {
	      static const _TCHAR *ctrlchar =
		_T("@ABCDEFGHIJKLMNO")
		_T("PQRSTUVWXYZ[\\]^_")
		_T("@abcdefghijklmno")
		_T("pqrstuvwxyz@@@@?");
	      static const _TCHAR *ctrlcode =
		_T("\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17")
		_T("\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37")
		_T("\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17")
		_T("\20\21\22\23\24\25\26\27\30\31\32\00\00\00\00\177");
	      if (const _TCHAR *c = _tcschr(ctrlchar, *i_str))
		*d++ = ctrlcode[c - ctrlchar], i_str ++;
	    }
	    break;
	  case _T('x'): case _T('X'):
	  {
	    i_str ++;
	    static const _TCHAR *hexchar = _T("0123456789ABCDEFabcdef");
	    static int hexvalue[] = { 0, 1, 2, 3, 4, 5 ,6, 7, 8, 9,
				      10, 11, 12, 13, 14, 15,
				      10, 11, 12, 13, 14, 15, };
	    bool brace = false;
	    if (i_str < end && *i_str == _T('{'))
	    {
	      i_str ++;
	      brace = true;
	    }
	    int n = 0;
	    for (; i_str < end && *i_str; i_str ++)
	      if (const _TCHAR *c = _tcschr(hexchar, *i_str))
		n = n * 16 + hexvalue[c - hexchar];
	      else
		break;
	    if (i_str < end && *i_str == _T('}') && brace)
	      i_str ++;
	    if (0 < n)
	      *d++ = static_cast<_TCHAR>(n);
	    break;
	  }
	  case _T('1'): case _T('2'): case _T('3'):
	  case _T('4'): case _T('5'): case _T('6'): case _T('7'):
	    if (i_doesUseRegexpBackReference)
	      goto case_default;
	    // fall through
	  case _T('0'):
	  {
	    static const _TCHAR *octalchar = _T("01234567");
	    static int octalvalue[] = { 0, 1, 2, 3, 4, 5 ,6, 7, };
	    int n = 0;
	    for (; i_str < end && *i_str; i_str ++)
	      if (const _TCHAR *c = _tcschr(octalchar, *i_str))
		n = n * 8 + octalvalue[c - octalchar];
	      else
		break;
	    if (0 < n)
	      *d++ = static_cast<_TCHAR>(n);
	    break;
	  }
	  default:
	  case_default:
	    *d++ = _T('\\');
	    if (_istlead(*i_str) && *(i_str + 1) && i_str + 1 < end)
	      *d++ = *i_str++;
	    *d++ = *i_str++;
	    break;
	}
    }
  }
  *d =_T('\0');
  return result.get();
}


// add session id to i_str
tstring addSessionId(const _TCHAR *i_str)
{
    DWORD sessionId;
    tstring s(i_str);
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId)) {
	_TCHAR buf[20];
	_sntprintf(buf, NUMBER_OF(buf), _T("%u"), sessionId);
	s += buf;
    }
    return s;
}


#ifdef _MBCS
// escape regexp special characters in MBCS trail bytes
std::string guardRegexpFromMbcs(const char *i_str)
{
  size_t len = strlen(i_str);
  Array<char> buf(len * 2 + 1);
  char *p = buf.get();
  while (*i_str)
  {
    if (_ismbblead(static_cast<u_char>(*i_str)) && i_str[1])
    {
      *p ++ = *i_str ++;
      if (strchr(".*?+(){}[]^$", *i_str))
	*p ++ = '\\';
    }
    *p ++ = *i_str ++;
  }
  return std::string(buf.get(), p);
}
#endif // !_MBCS


// converter
std::wstring to_wstring(const std::string &i_str)
{
  size_t size = mbstowcs(NULL, i_str.c_str(), i_str.size() + 1);
  if (size == (size_t)-1)
    return std::wstring();
  Array<wchar_t> result(size + 1);
  mbstowcs(result.get(), i_str.c_str(), i_str.size() + 1);
  return std::wstring(result.get());
}


// converter
std::string to_string(const std::wstring &i_str)
{
  size_t size = wcstombs(NULL, i_str.c_str(), i_str.size() + 1);
  if (size == (size_t)-1)
    return std::string();
  Array<char> result(size + 1);
  wcstombs(result.get(), i_str.c_str(), i_str.size() + 1);
  return std::string(result.get());
}


/// stream output
tostream &operator<<(tostream &i_ost, const tregex &i_data)
{
  return i_ost << i_data.str();
}


/// get lower string
tstring toLower(const tstring &i_str)
{
  tstring str(i_str);
  for (size_t i = 0; i < str.size(); ++ i)
  {
    if (_ismbblead(str[i]))
      ++ i;
    else
      str[i] = tolower(str[i]);
  }
  return str;
}


// convert wstring to UTF-8
std::string to_UTF_8(const std::wstring &i_str)
{
  // 0xxxxxxx: 00-7F
  // 110xxxxx 10xxxxxx: 0080-07FF
  // 1110xxxx 10xxxxxx 10xxxxxx: 0800 - FFFF

  int size = 0;
  
  // count needed buffer size
  for (std::wstring::const_iterator i = i_str.begin(); i != i_str.end(); ++ i)
  {
    if (0x0000 <= *i && *i <= 0x007f)
      size += 1;
    else if (0x0080 <= *i && *i <= 0x07ff)
      size += 2;
    else if (0x0800 <= *i && *i <= 0xffff)
      size += 3;
  }

  Array<char> result(size);
  int ri = 0;
  
  // make UTF-8
  for (std::wstring::const_iterator i = i_str.begin(); i != i_str.end(); ++ i)
  {
    if (0x0000 <= *i && *i <= 0x007f)
      result[ri ++] = static_cast<char>(*i);
    else if (0x0080 <= *i && *i <= 0x07ff)
    {
      result[ri ++] = static_cast<char>(((*i & 0x0fc0) >>  6) | 0xc0);
      result[ri ++] = static_cast<char>(( *i & 0x003f       ) | 0x80);
    }
    else if (0x0800 <= *i && *i <= 0xffff)
    {
      result[ri ++] = static_cast<char>(((*i & 0xf000) >> 12) | 0xe0);
      result[ri ++] = static_cast<char>(((*i & 0x0fc0) >>  6) | 0x80);
      result[ri ++] = static_cast<char>(( *i & 0x003f       ) | 0x80);
    }
  }

  return std::string(result.begin(), result.end());
}
