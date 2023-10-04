# Microsoft Developer Studio Project File - Name="dvdnav" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dvdnav - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dvdnav.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dvdnav.mak" CFG="dvdnav - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dvdnav - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dvdnav - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dvdnav - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../src/contrib/libdvdnav/src" /I "../src/contrib/libdvdnav/src/dvdread" /I "../src/contrib/libdvdnav/src/vm" /I "../src/libsp/win32/include" /I "../src/contrib/libdvdcss/src" /I "../src/contrib/libdvdnav/msvc" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /D "DVDNAV_COMPILE" /YX /Fo"Release/libdvdnav/" /Fd"Release/libdvdnav/" /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dvdnav - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../src/contrib/libdvdnav/src" /I "../src/contrib/libdvdnav/src/dvdread" /I "../src/contrib/libdvdnav/src/vm" /I "../src/libsp/win32/include" /I "../src/contrib/libdvdcss/src" /I "../src/contrib/libdvdnav/msvc" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /D "DVDNAV_COMPILE" /YX /Fo"Debug/libdvdnav/" /Fd"Debug/libdvdnav/" /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "dvdnav - Win32 Release"
# Name "dvdnav - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\vm\decoder.c
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvd_input.c"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvd_reader.c"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvd_udf.c"
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\dvdnav.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\highlight.c
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\ifo_print.c"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\ifo_read.c"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\md5.c"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\nav_print.c"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\nav_read.c"
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\navigation.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\read_cache.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\remap.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\searching.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\settings.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\vm\vm.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\vm\vmcmd.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\bswap.h"
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\vm\decoder.h
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvd_input.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvd_reader.h"
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\dvd_types.h
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvd_udf.h"
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\dvdnav.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\dvdnav_events.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\dvdnav_internal.h
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\dvdread_internal.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\ifo_print.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\ifo_read.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\ifo_types.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\md5.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\nav_print.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\nav_read.h"
# End Source File
# Begin Source File

SOURCE="..\src\contrib\libdvdnav\src\dvdread\nav_types.h"
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\read_cache.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\remap.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\vm\vm.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libdvdnav\src\vm\vmcmd.h
# End Source File
# End Group
# End Target
# End Project
