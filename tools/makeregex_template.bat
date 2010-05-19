#if _MSC_VER == 1500
#define VC_VERSION vc9
#elif _MSC_VER == 1400
#define VC_VERSION vc8
#endif

set CONFIG=%1
set BOOST_MAJOR=%2
set BOOST_MINOR=%3
set CPUBIT=%4
set BOOST_DIR=..\..\boost_%BOOST_MAJOR%_%BOOST_MINOR%
set REGEX_VC=VC_VERSION

if "%CONFIG%" == "Debug" set GD=gd

set REGEX=libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%

pushd %BOOST_DIR%\libs\regex\build

if exist %REGEX_VC%0\%REGEX%.lib nmake -f %REGEX_VC%.mak %REGEX%_clean
nmake -f %REGEX_VC%.mak XCFLAGS=-D_WCTYPE_INLINE_DEFINED main_dir %REGEX%_dir ./%REGEX_VC%0/%REGEX%.lib

popd

copy /Y %BOOST_DIR%\libs\regex\build\%REGEX_VC%0\%REGEX%.lib ..\proj\ext_lib%CPUBIT%\%CONFIG%\%REGEX%.lib
copy /Y %BOOST_DIR%\libs\regex\build\%REGEX_VC%0\%REGEX%.lib ..\proj\ext_lib%CPUBIT%\%CONFIG%\libboost_regex-mt-s%GD%-%BOOST_MAJOR%.lib

if "%CONFIG%" == "Debug" copy /Y %BOOST_DIR%\libs\regex\build\%REGEX_VC%0\%REGEX%.pdb ..\proj\ext_lib%CPUBIT%\%CONFIG%\%REGEX%.pdb
