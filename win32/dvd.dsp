# Microsoft Developer Studio Project File - Name="dvd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dvd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dvd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dvd.mak" CFG="dvd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dvd - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dvd - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dvd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release/dvd"
# PROP BASE Intermediate_Dir "Release/dvd"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release/dvd"
# PROP Intermediate_Dir "Release/dvd"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../src/libsp/win32/include" /I "../src" /I "../src/contrib/libdvdnav/src" /I "../src/contrib/libdvdnav/src/dvdread" /I "../src/contrib/libdvdnav/msvc" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DVDCSS_USE_EXTERNAL_CSS" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dvd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug/dvd"
# PROP BASE Intermediate_Dir "Debug/dvd"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug/dvd"
# PROP Intermediate_Dir "Debug/dvd"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../src/libsp/win32/include" /I "../src" /I "../src/contrib/libdvdnav/src" /I "../src/contrib/libdvdnav/src/dvdread" /I "../src/contrib/libdvdnav/msvc" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DVDCSS_USE_EXTERNAL_CSS" /YX /FD /GZ /c
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

# Name "dvd - Win32 Release"
# Name "dvd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\dvd\dvd.cpp
# ADD CPP /I "../src/contrib/libdvdnav/src/vm" /D "DVDNAV_COMPILE"
# End Source File
# Begin Source File

SOURCE=..\src\dvd\dvd_player.cpp
# End Source File
# Begin Source File

SOURCE=..\src\dvd\dvd_css.c
# ADD CPP /I "../src/contrib/libdvdcss/src" /I "../src/contrib/libdvdcss/win32"
# End Source File
# Begin Source File

SOURCE=..\src\dvd\dvd_misc.c
# ADD CPP /I "../src/contrib/libdvdnav/src/vm" /D "HAVE_CONFIG_H" /D "DVDNAV_COMPILE"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\dvd.h
# End Source File
# Begin Source File

SOURCE=..\src\dvd\dvd-internal.h
# End Source File
# Begin Source File

SOURCE=..\src\dvd_misc.h
# End Source File
# End Group
# End Target
# End Project
