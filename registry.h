//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// registry.h


#ifndef _REGISTRY_H
#  define _REGISTRY_H

#  include "stringtool.h"
#  include <list>


/// registry access class
class Registry
{
  HKEY m_root;					/// registry root
  tstring m_path;				/// path from registry root

public:
  typedef std::list<tstring> tstrings;
  
public:
  ///
  Registry() : m_root(NULL) { }
  ///
  Registry(HKEY i_root, const tstring &i_path)
    : m_root(i_root), m_path(i_path) { }
  
  /// set registry root and path
  void setRoot(HKEY i_root, const tstring &i_path)
  { m_root = i_root; m_path = i_path; }
  
  /// remvoe
  bool remove(const tstring &i_name = _T("")) const
  { return remove(m_root, m_path, i_name); }
  
  /// does exist the key ?
  bool doesExist() const { return doesExist(m_root, m_path); }

  /// read DWORD
  bool read(const tstring &i_name, int *o_value, int i_defaultValue = 0)
    const
  { return read(m_root, m_path, i_name, o_value, i_defaultValue); }
  /// write DWORD
  bool write(const tstring &i_name, int i_value) const
  { return write(m_root, m_path, i_name, i_value); }
 
  /// read tstring
  bool read(const tstring &i_name, tstring *o_value, 
	    const tstring &i_defaultValue = _T("")) const
  { return read(m_root, m_path, i_name, o_value, i_defaultValue); }
  /// write tstring
  bool write(const tstring &i_name, const tstring &i_value) const
  { return write(m_root, m_path, i_name, i_value); }

  /// read list of tstring
  bool read(const tstring &i_name, tstrings *o_value, 
	    const tstrings &i_defaultValue = tstrings()) const
  { return read(m_root, m_path, i_name, o_value, i_defaultValue); }
  /// write list of tstring
  bool write(const tstring &i_name, const tstrings &i_value) const
  { return write(m_root, m_path, i_name, i_value); }

  /// read binary data
  bool read(const tstring &i_name, BYTE *o_value, DWORD i_valueSize,
	    const BYTE *i_defaultValue = NULL, DWORD i_defaultValueSize = 0)
    const
  { return read(m_root, m_path, i_name, o_value, i_valueSize, i_defaultValue,
		i_defaultValueSize); }
  /// write binary data
  bool write(const tstring &i_name, const BYTE *i_value,
	     DWORD i_valueSize) const
  { return write(m_root, m_path, i_name, i_value, i_valueSize); }

public:
  /// remove
  static bool remove(HKEY i_root, const tstring &i_path,
		     const tstring &i_name = _T(""));
  
  /// does exist the key ?
  static bool doesExist(HKEY i_root, const tstring &i_path);
  
  /// read DWORD
  static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		   int *o_value, int i_defaultValue = 0);
  /// write DWORD
  static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    int i_value);

  /// read tstring
  static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		   tstring *o_value, const tstring &i_defaultValue = _T(""));
  /// write tstring
  static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    const tstring &i_value);
  
  /// read list of tstring
  static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		   tstrings *o_value, const tstrings &i_defaultValue = tstrings());
  /// write list of tstring
  static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    const tstrings &i_value);
  
  /// read binary data
  static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		   BYTE *o_value, DWORD i_valueSize,
		   const BYTE *i_defaultValue = NULL,
		   DWORD i_defaultValueSize = 0);
  /// write binary data
  static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    const BYTE *i_value, DWORD i_valueSize);
  /// read LOGFONT
  static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
		   LOGFONT *o_value, const tstring &i_defaultStringValue);
  /// write LOGFONT
  static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
		    const LOGFONT &i_value);
};


#endif // !_REGISTRY_H
