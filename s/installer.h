///////////////////////////////////////////////////////////////////////////////
// installer.h


#ifndef _INSTALLER_H
#  define _INSTALLER_H


namespace Installer
{
  /////////////////////////////////////////////////////////////////////////////
  // SetupFile
  
  // files to copy
  namespace SetupFile
  {
    enum Kind
    {
      File,
      Dir,
      Dll,
    };
    
    enum OS
    {
      ALL,
      W9x, // W95, W98,
      NT,  NT4, W2k,				// W2k includes XP
    };
    
    enum Destination
    {
      ToDest,
      ToDriver,
    };

    enum Flag
    {
      Normal = 1,
    };
    
    struct Data
    {
      Kind m_kind;
      OS m_os;
      u_int32 m_flags;				/// user defined flags
      const _TCHAR *m_from;
      Destination m_destination;
      const _TCHAR *m_to;
    };
  }

  
  /////////////////////////////////////////////////////////////////////////////
  // Locale / StringResource
  
  enum Locale
  {
    LOCALE_Japanese_Japan_932 = 0,
    LOCALE_C = 1,
  };

  struct StringResource
  {
    UINT m_id;
    _TCHAR *m_str;
  };

  class Resource
  {
    const StringResource *m_stringResources;
    
    Locale m_locale;
    
  public:
    // constructor
    Resource(const StringResource *i_stringResources);
    Resource(const StringResource *i_stringResources, Locale i_locale)
      : m_stringResources(i_stringResources), m_locale(i_locale) { }
    
    // get resource string
    const _TCHAR *loadString(UINT i_id);

    // locale
    Locale getLocale() const { return m_locale; }
  };

  
  /////////////////////////////////////////////////////////////////////////////
  // Utility Functions

  // createLink - uses the shell's IShellLink and IPersistFile interfaces 
  //   to create and store a shortcut to the specified object. 
  HRESULT createLink(LPCTSTR i_pathObj, LPCTSTR i_pathLink, LPCTSTR i_desc,
		     LPCTSTR i_workingDirectory = NULL);
  
  // create file extension information
  void createFileExtension(const tstringi &i_ext, const tstring &i_contentType,
			   const tstringi &i_fileType,
			   const tstring &i_fileTypeName,
			   const tstringi &i_iconPath,
			   const tstring &i_command);
  
  // remove file extension information
  void removeFileExtension(const tstringi &i_ext, const tstringi &i_fileType);
  
  // create uninstallation information
  void createUninstallInformation(const tstringi &i_name,
				  const tstring &i_displayName,
				  const tstring &i_commandLine);
  
  // remove uninstallation information
  void removeUninstallInformation(const tstringi &i_name);

  // normalize path
  tstringi normalizePath(tstringi i_path);
  
  // create deep directory
  bool createDirectories(const _TCHAR *i_folder);

  // get driver directory
  tstringi getDriverDirectory();

  // get current directory
  tstringi getModuleDirectory();

  // get start menu name
  tstringi getStartMenuName(const tstringi &i_shortcutName);

  // get start up name
  tstringi getStartUpName(const tstringi &i_shortcutName);

  // create driver service
  DWORD createDriverService(const tstringi &i_serviceName,
			    const tstring &i_serviceDescription,
			    const tstringi &i_driverPath,
			    const _TCHAR *i_preloadedGroups,
			    bool forUsb);

  // remove driver service
  DWORD removeDriverService(const tstringi &i_serviceName);

  // check operating system
  bool checkOs(SetupFile::OS i_os);
  
  // install files
  bool installFiles(const SetupFile::Data *i_setupFiles,
		    size_t i_setupFilesSize, u_int32 i_flags,
		    const tstringi &i_srcDir, const tstringi &i_destDir);
  
  // remove files from src
  bool removeSrcFiles(const SetupFile::Data *i_setupFiles,
		      size_t i_setupFilesSize, u_int32 i_flags,
		      const tstringi &i_srcDir);
  
  // remove files
  void removeFiles(const SetupFile::Data *i_setupFiles,
		   size_t i_setupFilesSize, u_int32 i_flags,
		   const tstringi &i_destDir);
  
  // uninstall step1
  int uninstallStep1(const _TCHAR *i_uninstallOption);
  
  // uninstall step2
  // (after this function, we cannot use any resource)
  void uninstallStep2(const _TCHAR *i_argByStep1);
}


#endif // _INSTALLER_H
