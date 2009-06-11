///////////////////////////////////////////////////////////////////////////////
// setup.cpp


#include "../misc.h"
#include "../registry.h"
#include "../stringtool.h"
#include "../windowstool.h"
#include "installer.h"

#include <shlobj.h>
#include <sys/types.h>
#include <sys/stat.h>


namespace Installer
{
  using namespace std;
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Utility Functions

  /** createLink
		uses the shell's IShellLink and IPersistFile interfaces to
		create and store a shortcut to the specified object.
      @return
		the result of calling the member functions of the interfaces.
      @param i_pathObj
		address of a buffer containing the path of the object.
      @param i_pathLink
      		address of a buffer containing the path where the
		shell link is to be stored.
      @param i_desc
		address of a buffer containing the description of the
		shell link.
  */
  HRESULT createLink(LPCTSTR i_pathObj, LPCTSTR i_pathLink, LPCTSTR i_desc,
		     LPCTSTR i_workingDirectory)
  { 
    // Get a pointer to the IShellLink interface. 
    IShellLink* psl;
    HRESULT hres =
      CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		       IID_IShellLink, (void **)&psl);
    if (SUCCEEDED(hres))
    { 
      // Set the path to the shortcut target and add the description. 
      psl->SetPath(i_pathObj);
      psl->SetDescription(i_desc);
      if (i_workingDirectory)
	psl->SetWorkingDirectory(i_workingDirectory);
 
      // Query IShellLink for the IPersistFile interface for saving the 
      // shortcut in persistent storage. 
      IPersistFile* ppf; 
      hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
 
      if (SUCCEEDED(hres))
      {
#ifdef UNICODE
	// Save the link by calling IPersistFile::Save. 
	hres = ppf->Save(i_pathLink, TRUE); 
#else
	wchar_t wsz[MAX_PATH];
	// Ensure that the string is ANSI. 
	MultiByteToWideChar(CP_ACP, 0, i_pathLink, -1, wsz, MAX_PATH);
	// Save the link by calling IPersistFile::Save. 
	hres = ppf->Save(wsz, TRUE); 
#endif
	ppf->Release();
      }
      psl->Release();
    } 
    return hres; 
  }


  // create file extension information
  void createFileExtension(const tstringi &i_ext, const tstring &i_contentType,
			   const tstringi &i_fileType,
			   const tstring &i_fileTypeName,
			   const tstringi &i_iconPath,
			   const tstring &i_command)
  {
    tstring dummy;

    Registry regExt(HKEY_CLASSES_ROOT, i_ext);
    if (!         regExt.read (_T(""), &dummy))
      CHECK_TRUE( regExt.write(_T(""), i_fileType) );
    if (!         regExt.read (_T("Content Type"), &dummy))
      CHECK_TRUE( regExt.write(_T("Content Type"), i_contentType) );

    Registry      regFileType(HKEY_CLASSES_ROOT, i_fileType);
    if (!         regFileType.read (_T(""), &dummy))
      CHECK_TRUE( regFileType.write(_T(""), i_fileTypeName) );

    Registry      regFileTypeIcon(HKEY_CLASSES_ROOT,
				  i_fileType + _T("\\DefaultIcon"));
    if (!         regFileTypeIcon.read (_T(""), &dummy))
      CHECK_TRUE( regFileTypeIcon.write(_T(""), i_iconPath) );

    Registry      regFileTypeComand(HKEY_CLASSES_ROOT,
				    i_fileType + _T("\\shell\\open\\command"));
    if (!         regFileTypeComand.read (_T(""), &dummy))
      CHECK_TRUE( regFileTypeComand.write(_T(""), i_command) );
  }


  // remove file extension information
  void removeFileExtension(const tstringi &i_ext, const tstringi &i_fileType)
  {
    Registry::remove(HKEY_CLASSES_ROOT, i_ext);
    Registry::remove(HKEY_CLASSES_ROOT,
		     i_fileType + _T("\\shell\\open\\command"));
    Registry::remove(HKEY_CLASSES_ROOT, i_fileType + _T("\\shell\\open"));
    Registry::remove(HKEY_CLASSES_ROOT, i_fileType + _T("\\shell"));
    Registry::remove(HKEY_CLASSES_ROOT, i_fileType);
  }

  
  // create uninstallation information
  void createUninstallInformation(const tstringi &i_name,
				  const tstring &i_displayName,
				  const tstring &i_commandLine)
  {
    Registry reg(
      HKEY_LOCAL_MACHINE,
      _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\")
      + i_name);

    CHECK_TRUE( reg.write(_T("DisplayName"), i_displayName) );
    CHECK_TRUE( reg.write(_T("UninstallString"), i_commandLine) );
  }

  
  // remove uninstallation information
  void removeUninstallInformation(const tstringi &i_name)
  {
    Registry::
      remove(HKEY_LOCAL_MACHINE,
	     _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\")
	     + i_name);
  }
  
  
  // normalize path
  tstringi normalizePath(tstringi i_path)
  {
    tregex regSlash(_T("^(.*)/(.*)$"));
    tsmatch what;
    while (boost::regex_search(i_path, what, regSlash))
      i_path = what.str(1) + _T("\\") + what.str(2);

    tregex regTailBackSlash(_T("^(.*)\\\\$"));
    while (boost::regex_search(i_path, what, regTailBackSlash))
      i_path = what.str(1);
    
    return i_path;
  }
  
  
  // create deep directory
  bool createDirectories(const _TCHAR *i_folder)
  {
    const _TCHAR *s = _tcschr(i_folder, _T('\\')); // TODO: '/'
    if (s && s - i_folder == 2 && i_folder[1] == _T(':'))
      s = _tcschr(s + 1, _T('\\'));
    
    struct _stat sbuf;
    while (s)
    {
      tstringi f(i_folder, 0, s - i_folder);
      if (_tstat(f.c_str(), &sbuf) < 0)
	if (!CreateDirectory(f.c_str(), NULL))
	  return false;
      s = _tcschr(s + 1, _T('\\'));
    }
    if (_tstat(i_folder, &sbuf) < 0)
      if (!CreateDirectory(i_folder, NULL))
	return false;
    return true;
  }


  // get driver directory
  tstringi getDriverDirectory()
  {
    _TCHAR buf[GANA_MAX_PATH];
    CHECK_TRUE( GetSystemDirectory(buf, NUMBER_OF(buf)) );
    return tstringi(buf) + _T("\\drivers");
  }

  
  // get current directory
  tstringi getModuleDirectory()
  {
    _TCHAR buf[GANA_MAX_PATH];
    CHECK_TRUE( GetModuleFileName(g_hInst, buf, NUMBER_OF(buf)) );
    tregex reg(_T("^(.*)\\\\[^\\\\]*$"));
    tsmatch what;
    tstringi path(buf);
    if (boost::regex_search(path, what, reg))
      return what.str(1);
    else
      return path;
  }


  // get start menu name
  tstringi getStartMenuName(const tstringi &i_shortcutName)
  {
#if 0
    char programDir[GANA_MAX_PATH];
    if (SUCCEEDED(SHGetSpecialFolderPath(NULL, programDir,
					 CSIDL_COMMON_PROGRAMS, FALSE)))
      return tstringi(programDir) + "\\" + shortcutName + ".lnk";
#else
    tstringi programDir;
    if (Registry::read(HKEY_LOCAL_MACHINE,
		       _T("Software\\Microsoft\\Windows\\CurrentVersion\\")
		       _T("Explorer\\Shell Folders"), _T("Common Programs"),
		       &programDir))
      return programDir + _T("\\") + i_shortcutName + _T(".lnk");
#endif
    return _T("");
  }


  // get start up name
  tstringi getStartUpName(const tstringi &i_shortcutName)
  {
    tstringi startupDir;
    if (Registry::read(HKEY_CURRENT_USER,
		       _T("Software\\Microsoft\\Windows\\CurrentVersion\\")
		       _T("Explorer\\Shell Folders"), _T("Startup"),
		       &startupDir))
      return startupDir + _T("\\") + i_shortcutName + _T(".lnk");
    return _T("");
  }


#if defined(_WINNT)

#  define MAYUD_FILTER_KEY _T("System\\CurrentControlSet\\Control\\Class\\{4D36E96B-E325-11CE-BFC1-08002BE10318}")

  // create driver service
  DWORD createDriverService(const tstringi &i_serviceName,
			    const tstring &i_serviceDescription,
			    const tstringi &i_driverPath,
			    const _TCHAR *i_preloadedGroups,
			    bool forUsb)
  {
    SC_HANDLE hscm =
      OpenSCManager(NULL, NULL,
		    SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT);
    if (!hscm)
      return false;

    SC_HANDLE hs =
      CreateService(hscm, i_serviceName.c_str(), i_serviceDescription.c_str(),
		    SERVICE_START | SERVICE_STOP, SERVICE_KERNEL_DRIVER,
		    forUsb == true ? SERVICE_DEMAND_START : SERVICE_AUTO_START,
		    SERVICE_ERROR_IGNORE,
		    i_driverPath.c_str(), NULL, NULL,
		    i_preloadedGroups, NULL, NULL);
    DWORD err = GetLastError();
    if (hs == NULL)
    {
      switch (err)
      {
	case ERROR_SERVICE_EXISTS:
	{
#if 0
	  hs = OpenService(hscm, i_serviceName.c_str(), SERVICE_CHANGE_CONFIG);
	  if (hs == NULL) {
	    CloseServiceHandle(hscm);
	    return GetLastError();
	  }
	  if (!ChangeServiceConfig(
		hscm, SERVICE_KERNEL_DRIVER,
		forUsb == true ? SERVICE_DEMAND_START : SERVICE_AUTO_START,
		SERVICE_ERROR_IGNORE,
		i_driverPath.c_str(), NULL, NULL,
		i_preloadedGroups, NULL, NULL,
		i_serviceDescription.c_str())) {
	    CloseServiceHandle(hs);
	    CloseServiceHandle(hscm);
	    return GetLastError();		// ERROR_IO_PENDING!
	    // this code always reaches here. why?
	  }
#else
	  Registry reg(HKEY_LOCAL_MACHINE,
		       _T("SYSTEM\\CurrentControlSet\\Services\\mayud"));
	  reg.write(_T("Start"),
		    forUsb ? SERVICE_DEMAND_START : SERVICE_AUTO_START);
#endif
	  break;
	}
	default:
	{
	  CloseServiceHandle(hscm);
	  return err;
	}
      }
    }
    CloseServiceHandle(hs);
    CloseServiceHandle(hscm);

    if (forUsb == true) {
      Registry reg(HKEY_LOCAL_MACHINE, MAYUD_FILTER_KEY);
      typedef std::list<tstring> Filters;
      Filters filters;
      if (!reg.read(_T("UpperFilters"), &filters))
	return false;
      for (Filters::iterator i = filters.begin(); i != filters.end(); ) {
	  Filters::iterator next = i;
	  ++ next;
	  if (*i == _T("mayud")) {
	      filters.erase(i);
	  }
	  i = next;
      }
      filters.push_back(_T("mayud"));
      if (!reg.write(_T("UpperFilters"), filters))
	return false;
    }

    return ERROR_SUCCESS;
  }
#endif // _WINNT


#if defined(_WINNT)
  // remove driver service
  DWORD removeDriverService(const tstringi &i_serviceName)
  {
    DWORD err = ERROR_SUCCESS;
    
    Registry reg(HKEY_LOCAL_MACHINE, MAYUD_FILTER_KEY);
    std::list<tstring> filters;
    if (reg.read(_T("UpperFilters"), &filters))
    {
      filters.remove(_T("mayud"));
      reg.write(_T("UpperFilters"), filters);
    }

    SC_HANDLE hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE hs =
      OpenService(hscm, i_serviceName.c_str(),
		  SERVICE_START | SERVICE_STOP | DELETE);
    if (!hs)
    {
      err = GetLastError();
      goto error;
    }

    SERVICE_STATUS ss;
    ControlService(hs, SERVICE_CONTROL_STOP, &ss);
  
    if (!DeleteService(hs))
    {
      err = GetLastError();
      goto error;
    }
    error:
    CloseServiceHandle(hs);
    CloseServiceHandle(hscm);
    return err;
  }
#endif // _WINNT


  // check operating system
  bool checkOs(SetupFile::OS os)
  {
    OSVERSIONINFO ver;
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    
    switch (os)
    {
      default:
      case SetupFile::ALL:
	return true;
      case SetupFile::W9x:
	return (ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
		4 <= ver.dwMajorVersion);
      case SetupFile::NT :
	return (ver.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		4 <= ver.dwMajorVersion);
      case SetupFile::NT4:
	return (ver.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		ver.dwMajorVersion == 4);
      case SetupFile::W2k:			// W2k, XP, ...
	return (ver.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		5 <= ver.dwMajorVersion);
    }
  }

  
  // install files
  bool installFiles(const SetupFile::Data *i_setupFiles,
		    size_t i_setupFilesSize, u_int32 i_flags,
		    const tstringi &i_srcDir, const tstringi &i_destDir)
  {
    tstringi to, from;
    tstringi destDriverDir = getDriverDirectory();

    for (size_t i = 0; i < i_setupFilesSize; ++ i)
    {
      const SetupFile::Data &s = i_setupFiles[i];
      const tstringi &fromDir = i_srcDir;
      const tstringi &toDir =
	(s.m_destination == SetupFile::ToDriver) ? destDriverDir : i_destDir;

      if (!s.m_from)
	continue;				// remove only

      if (fromDir == toDir)
	continue;				// same directory

      if (!checkOs(s.m_os))			// check operating system
	continue;

      if ((s.m_flags & i_flags) != i_flags)	// check flags
	continue;
      
      // type
      switch (s.m_kind)
      {
	case SetupFile::Dll:
	{
	  // rename driver
	  tstringi from_ = toDir + _T("\\") + s.m_to;
	  tstringi to_ = toDir + _T("\\deleted.") + s.m_to;
	  DeleteFile(to_.c_str());
	  MoveFile(from_.c_str(), to_.c_str());
	  DeleteFile(to_.c_str());
	}
	// fall through
	default:
	case SetupFile::File:
	{
	  from += fromDir + _T('\\') + s.m_from + _T('\0');
	  to   += toDir   + _T('\\') + s.m_to   + _T('\0');
	  break;
	}
	case SetupFile::Dir:
	{
	  createDirectories((toDir + _T('\\') + s.m_to).c_str());
	  break;
	}
      }
    }
#if 0
    {
      tstringi to_(to), from_(from);
      for (size_t i = 0; i < to_.size(); ++ i)
	if (!to_[i])
	  to_[i] = ' ';
      for (size_t i = 0; i < from_.size(); ++ i)
	if (!from_[i])
	  from_[i] = ' ';
      MessageBox(NULL, to_.c_str(), from_.c_str(), MB_OK);
    }
#endif

    SHFILEOPSTRUCT fo;
    ::ZeroMemory(&fo, sizeof(fo));
    fo.wFunc = FO_COPY;
    fo.fFlags = FOF_MULTIDESTFILES;
    fo.pFrom = from.c_str();
    fo.pTo   = to.c_str();
    if (SHFileOperation(&fo) || fo.fAnyOperationsAborted)
      return false;
    return true;
  }
  

  // remove files from src
  bool removeSrcFiles(const SetupFile::Data *i_setupFiles, 
		      size_t i_setupFilesSize, u_int32 i_flags,
		      const tstringi &i_srcDir)
  {
    tstringi destDriverDir = getDriverDirectory();

    for (size_t i = 0; i < i_setupFilesSize; ++ i)
    {
      const SetupFile::Data &s = i_setupFiles[i_setupFilesSize - i - 1];
      const tstringi &fromDir = i_srcDir;
      
      if (!s.m_from)
	continue;	// remove only

      if (!checkOs(s.m_os))	// check operating system
	continue;
      
      if ((s.m_flags & i_flags) != i_flags)	// check flags
	continue;

      // type
      switch (s.m_kind)
      {
	default:
	case SetupFile::Dll:
	case SetupFile::File:
	  DeleteFile((fromDir + _T('\\') + s.m_from).c_str());
	  break;
	case SetupFile::Dir:
	  RemoveDirectory((fromDir + _T('\\') + s.m_from).c_str());
	  break;
      }
    }
    RemoveDirectory(i_srcDir.c_str());
    return true;
  }

  
  // remove files
  void removeFiles(const SetupFile::Data *i_setupFiles,
		   size_t i_setupFilesSize, u_int32 i_flags,
		   const tstringi &i_destDir)
  {
    tstringi destDriverDir = getDriverDirectory();

    for (size_t i = 0; i < i_setupFilesSize; ++ i)
    {
      const SetupFile::Data &s = i_setupFiles[i_setupFilesSize - i - 1];
      const tstringi &toDir =
	(s.m_destination == SetupFile::ToDriver) ? destDriverDir : i_destDir;

      if (!checkOs(s.m_os))	// check operating system
	continue;

      if ((s.m_flags & i_flags) != i_flags)	// check flags
	continue;
      
      // type
      switch (s.m_kind)
      {
	case SetupFile::Dll:
	  DeleteFile((toDir + _T("\\deleted.") + s.m_to).c_str());
	  // fall through
	default:
	case SetupFile::File:
	  DeleteFile((toDir + _T('\\') + s.m_to).c_str());
	  break;
	case SetupFile::Dir:
	  RemoveDirectory((toDir + _T('\\') + s.m_to).c_str());
	  break;
      }
    }
    RemoveDirectory(i_destDir.c_str());
  }
  
  
  // uninstall step1
  int uninstallStep1(const _TCHAR *i_uninstallOption)
  {
    // copy this EXEcutable image into the user's temp directory
    _TCHAR setup_exe[GANA_MAX_PATH], tmp_setup_exe[GANA_MAX_PATH];
    GetModuleFileName(NULL, setup_exe, NUMBER_OF(setup_exe));
    GetTempPath(NUMBER_OF(tmp_setup_exe), tmp_setup_exe);
    GetTempFileName(tmp_setup_exe, _T("del"), 0, tmp_setup_exe);
    CopyFile(setup_exe, tmp_setup_exe, FALSE);
    
    // open the clone EXE using FILE_FLAG_DELETE_ON_CLOSE
    HANDLE hfile = CreateFile(tmp_setup_exe, 0, FILE_SHARE_READ, NULL,
			      OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    
    // spawn the clone EXE passing it our EXE's process handle
    // and the full path name to the original EXE file.
    _TCHAR commandLine[512];
    HANDLE hProcessOrig =
      OpenProcess(SYNCHRONIZE, TRUE, GetCurrentProcessId());
    _sntprintf(commandLine, NUMBER_OF(commandLine), _T("%s %s %d"),
	       tmp_setup_exe, i_uninstallOption, hProcessOrig);
    STARTUPINFO si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    CreateProcess(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &si,&pi);
    Sleep(2000); // important
    CloseHandle(hProcessOrig);
    CloseHandle(hfile);
    
    return 0;
  }

  // uninstall step2
  // (after this function, we cannot use any resource)
  void uninstallStep2(const _TCHAR *argByStep1)
  {
    // clone EXE: When original EXE terminates, delete it
    HANDLE hProcessOrig = (HANDLE)_ttoi(argByStep1);
    WaitForSingleObject(hProcessOrig, INFINITE);
    CloseHandle(hProcessOrig);
  }
  
  
  /////////////////////////////////////////////////////////////////////////////
  // Locale / StringResource


  // constructor
  Resource::Resource(const StringResource *i_stringResources)
    : m_stringResources(i_stringResources),
      m_locale(LOCALE_C)
  {
    struct LocaleInformaton
    {
      const _TCHAR *m_localeString;
      Locale m_locale;
    };

    // set locale information
    const _TCHAR *localeString = ::_tsetlocale(LC_ALL, _T(""));
    
    static const LocaleInformaton locales[] =
    {
      { _T("Japanese_Japan.932"), LOCALE_Japanese_Japan_932 },
    };

    for (size_t i = 0; i < NUMBER_OF(locales); ++ i)
      if (_tcsicmp(localeString, locales[i].m_localeString) == 0)
      {
	m_locale = locales[i].m_locale;
	break;
      }
  }
  
  
  // get resource string
  const _TCHAR *Resource::loadString(UINT i_id)
  {
    int n = static_cast<int>(m_locale);
    int index = -1;
    for (int i = 0; m_stringResources[i].m_str; ++ i)
      if (m_stringResources[i].m_id == i_id)
      {
	if (n == 0)
	  return m_stringResources[i].m_str;
	index = i;
	n --;
      }
    if (0 <= index)
      return m_stringResources[index].m_str;
    else
      return _T("");
  }
}
