//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// registry.cpp


#include "registry.h"
#include "stringtool.h"
#include "array.h"
#include <malloc.h>


// remove
bool Registry::remove(HKEY i_root, const tstring &i_path,
		      const tstring &i_name)
{
  if (i_name.empty())
    return RegDeleteKey(i_root, i_path.c_str()) == ERROR_SUCCESS;
  HKEY hkey;
  if (ERROR_SUCCESS !=
      RegOpenKeyEx(i_root, i_path.c_str(), 0, KEY_SET_VALUE, &hkey))
    return false;
  LONG r = RegDeleteValue(hkey, i_name.c_str());
  RegCloseKey(hkey);
  return r == ERROR_SUCCESS;
}


// does exist the key ?
bool Registry::doesExist(HKEY i_root, const tstring &i_path)
{
  HKEY hkey;
  if (ERROR_SUCCESS !=
      RegOpenKeyEx(i_root, i_path.c_str(), 0, KEY_READ, &hkey))
    return false;
  RegCloseKey(hkey);
  return true;
}


// read DWORD
bool Registry::read(HKEY i_root, const tstring &i_path,
		    const tstring &i_name, int *o_value, int i_defaultValue)
{
  HKEY hkey;
  if (ERROR_SUCCESS ==
      RegOpenKeyEx(i_root, i_path.c_str(), 0, KEY_READ, &hkey))
  {
    DWORD type = REG_DWORD;
    DWORD size = sizeof(*o_value);
    LONG r = RegQueryValueEx(hkey, i_name.c_str(), NULL,
			     &type, (BYTE *)o_value, &size);
    RegCloseKey(hkey);
    if (r == ERROR_SUCCESS)
      return true;
  }
  *o_value = i_defaultValue;
  return false;
}


// write DWORD
bool Registry::write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		     int i_value)
{
  HKEY hkey;
  DWORD disposition;
  if (ERROR_SUCCESS !=
      RegCreateKeyEx(i_root, i_path.c_str(), 0, _T(""),
		     REG_OPTION_NON_VOLATILE,
		     KEY_ALL_ACCESS, NULL, &hkey, &disposition))
    return false;
  LONG r = RegSetValueEx(hkey, i_name.c_str(), NULL, REG_DWORD,
			 (BYTE *)&i_value, sizeof(i_value));
  RegCloseKey(hkey);
  return r == ERROR_SUCCESS;
}


// read string
bool Registry::read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    tstring *o_value, const tstring &i_defaultValue)
{
  HKEY hkey;
  if (ERROR_SUCCESS ==
      RegOpenKeyEx(i_root, i_path.c_str(), 0, KEY_READ, &hkey))
  {
    DWORD type = REG_SZ;
    DWORD size = 0;
    BYTE dummy;
    if (ERROR_MORE_DATA ==
	RegQueryValueEx(hkey, i_name.c_str(), NULL, &type, &dummy, &size))
    {
      if (0 < size)
      {
	Array<BYTE> buf(size);
	if (ERROR_SUCCESS == RegQueryValueEx(hkey, i_name.c_str(),
					     NULL, &type, buf.get(), &size))
	{
	  buf.back() = 0;
	  *o_value = reinterpret_cast<_TCHAR *>(buf.get());
	  RegCloseKey(hkey);
	  return true;
	}
      }
    }
    RegCloseKey(hkey);
  }
  if (!i_defaultValue.empty())
    *o_value = i_defaultValue;
  return false;
}


// write string
bool Registry::write(HKEY i_root, const tstring &i_path,
		     const tstring &i_name, const tstring &i_value)
{
  HKEY hkey;
  DWORD disposition;
  if (ERROR_SUCCESS !=
      RegCreateKeyEx(i_root, i_path.c_str(), 0, _T(""),
		     REG_OPTION_NON_VOLATILE,
		     KEY_ALL_ACCESS, NULL, &hkey, &disposition))
    return false;
  RegSetValueEx(hkey, i_name.c_str(), NULL, REG_SZ,
		(BYTE *)i_value.c_str(),
		(i_value.size() + 1) * sizeof(tstring::value_type));
  RegCloseKey(hkey);
  return true;
}


// read list of string
bool Registry::read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    tstrings *o_value, const tstrings &i_defaultValue)
{
  HKEY hkey;
  if (ERROR_SUCCESS ==
      RegOpenKeyEx(i_root, i_path.c_str(), 0, KEY_READ, &hkey))
  {
    DWORD type = REG_MULTI_SZ;
    DWORD size = 0;
    BYTE dummy;
    if (ERROR_MORE_DATA ==
	RegQueryValueEx(hkey, i_name.c_str(), NULL, &type, &dummy, &size))
    {
      if (0 < size)
      {
	Array<BYTE> buf(size);
	if (ERROR_SUCCESS == RegQueryValueEx(hkey, i_name.c_str(),
					     NULL, &type, buf.get(), &size))
	{
	  buf.back() = 0;
	  o_value->clear();
	  const _TCHAR *p = reinterpret_cast<_TCHAR *>(buf.get());
	  const _TCHAR *end = reinterpret_cast<_TCHAR *>(buf.end());
	  while (p < end && *p)
	  {
	    o_value->push_back(p);
	    p += o_value->back().length() + 1;
	  }
	  RegCloseKey(hkey);
	  return true;
	}
      }
    }
    RegCloseKey(hkey);
  }
  if (!i_defaultValue.empty())
    *o_value = i_defaultValue;
  return false;
}


// write list of string
bool Registry::write(HKEY i_root, const tstring &i_path,
		     const tstring &i_name, const tstrings &i_value)
{
  HKEY hkey;
  DWORD disposition;
  if (ERROR_SUCCESS !=
      RegCreateKeyEx(i_root, i_path.c_str(), 0, _T(""),
		     REG_OPTION_NON_VOLATILE,
		     KEY_ALL_ACCESS, NULL, &hkey, &disposition))
    return false;
  tstring value;
  for (tstrings::const_iterator i = i_value.begin(); i != i_value.end(); ++ i)
  {
    value += *i;
    value += _T('\0');
  }
  RegSetValueEx(hkey, i_name.c_str(), NULL, REG_MULTI_SZ,
		(BYTE *)value.c_str(),
		(value.size() + 1) * sizeof(tstring::value_type));
  RegCloseKey(hkey);
  return true;
}


// read binary
bool Registry::read(HKEY i_root, const tstring &i_path,
		    const tstring &i_name, BYTE *o_value, DWORD i_valueSize,
		    const BYTE *i_defaultValue, DWORD i_defaultValueSize)
{
  if (o_value && 0 < i_valueSize)
  {
    HKEY hkey;
    if (ERROR_SUCCESS ==
	RegOpenKeyEx(i_root, i_path.c_str(), 0, KEY_READ, &hkey))
    {
      DWORD type = REG_BINARY;
      LONG r = RegQueryValueEx(hkey, i_name.c_str(), NULL, &type,
			       (BYTE *)o_value, &i_valueSize);
      RegCloseKey(hkey);
      if (r == ERROR_SUCCESS)
	return true;
    }
  }
  if (i_defaultValue)
    CopyMemory(o_value, i_defaultValue,
	       MIN(i_defaultValueSize, i_valueSize));
  return false;
}


// write binary
bool Registry::write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		     const BYTE *i_value, DWORD i_valueSize)
{
  if (!i_value)
    return false;
  HKEY hkey;
  DWORD disposition;
  if (ERROR_SUCCESS !=
      RegCreateKeyEx(i_root, i_path.c_str(), 0, _T(""),
		     REG_OPTION_NON_VOLATILE,
		     KEY_ALL_ACCESS, NULL, &hkey, &disposition))
    return false;
  RegSetValueEx(hkey, i_name.c_str(), NULL, REG_BINARY, i_value, i_valueSize);
  RegCloseKey(hkey);
  return true;
}


//
static bool string2logfont(LOGFONT *o_lf, const tstring &i_strlf)
{
  // -13,0,0,0,400,0,0,0,128,1,2,1,1,Terminal
  tregex lf(_T("^(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),")
	    _T("(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),(-?\\d+),")
	    _T("(-?\\d+),(-?\\d+),(-?\\d+),(.+)$"));
  tsmatch what;

  if (!boost::regex_match(i_strlf, what, lf))
    return false;
  o_lf->lfHeight         =       _ttoi(what.str(1).c_str());
  o_lf->lfWidth          =       _ttoi(what.str(2).c_str());
  o_lf->lfEscapement     =       _ttoi(what.str(3).c_str());
  o_lf->lfOrientation    =       _ttoi(what.str(4).c_str());
  o_lf->lfWeight         =       _ttoi(what.str(5).c_str());
  o_lf->lfItalic         = (BYTE)_ttoi(what.str(6).c_str());
  o_lf->lfUnderline      = (BYTE)_ttoi(what.str(7).c_str());
  o_lf->lfStrikeOut      = (BYTE)_ttoi(what.str(8).c_str());
  o_lf->lfCharSet        = (BYTE)_ttoi(what.str(9).c_str());
  o_lf->lfOutPrecision   = (BYTE)_ttoi(what.str(10).c_str());
  o_lf->lfClipPrecision  = (BYTE)_ttoi(what.str(11).c_str());
  o_lf->lfQuality        = (BYTE)_ttoi(what.str(12).c_str());
  o_lf->lfPitchAndFamily = (BYTE)_ttoi(what.str(13).c_str());
  tcslcpy(o_lf->lfFaceName, what.str(14).c_str(), NUMBER_OF(o_lf->lfFaceName));
  return true;
}


// read LOGFONT
bool Registry::read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    LOGFONT *o_value, const tstring &i_defaultStringValue)
{
  tstring buf;
  if (!read(i_root, i_path, i_name, &buf) || !string2logfont(o_value, buf))
  {
    if (!i_defaultStringValue.empty())
      string2logfont(o_value, i_defaultStringValue);
    return false;
  }
  return true;
}


// write LOGFONT
bool Registry::write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		     const LOGFONT &i_value)
{
  _TCHAR buf[1024];
  _sntprintf(buf, NUMBER_OF(buf),
	     _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s"),
	     i_value.lfHeight, i_value.lfWidth, i_value.lfEscapement,
	     i_value.lfOrientation, i_value.lfWeight, i_value.lfItalic,
	     i_value.lfUnderline, i_value.lfStrikeOut, i_value.lfCharSet,
	     i_value.lfOutPrecision, i_value.lfClipPrecision,
	     i_value.lfQuality,
	     i_value.lfPitchAndFamily, i_value.lfFaceName);
  return Registry::write(i_root, i_path, i_name, buf);
}
