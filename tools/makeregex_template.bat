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

pushd %BOOST_DIR%\libs\regex\build

nmake -f %REGEX_VC%.mak libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%_clean
nmake -f %REGEX_VC%.mak XCFLAGS=-D_WCTYPE_INLINE_DEFINED main_dir libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%_dir ./%REGEX_VC%0/libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%.lib

popd

copy /Y %BOOST_DIR%\libs\regex\build\%REGEX_VC%0\libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%.lib ..\proj\ext_lib%CPUBIT%\%CONFIG%\libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%.lib
copy /Y %BOOST_DIR%\libs\regex\build\%REGEX_VC%0\libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%.lib ..\proj\ext_lib%CPUBIT%\%CONFIG%\libboost_regex-mt-s%GD%-%BOOST_MAJOR%.lib

if "%CONFIG%" == "Debug" copy /Y %BOOST_DIR%\libs\regex\build\%REGEX_VC%0\libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%.pdb ..\proj\ext_lib%CPUBIT%\%CONFIG%\libboost_regex-%REGEX_VC%0-mt-s%GD%-%BOOST_MAJOR%.pdb
