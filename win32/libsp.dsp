# Microsoft Developer Studio Project File - Name="libsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libsp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsp.mak" CFG="libsp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsp - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libsp - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsp - Win32 Release"

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
# ADD CPP /nologo /MD /W4 /GX /O2 /I "../src/libsp" /I "../src/libsp/win32/include" /I "../src" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /Fo"Release/libsp/" /Fd"Release/libsp/" /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libsp - Win32 Debug"

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
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "../src/libsp" /I "../src/libsp/win32/include" /I "../src" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /Fo"Debug/libsp/" /Fd"Debug/libsp/" /FD /GZ /c
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

# Name "libsp - Win32 Release"
# Name "libsp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\libsp\win32\dirent.c
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\membin.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_cdrom.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_css.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_eeprom.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_fip.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_flash.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_khwl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_khwl_colors.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_memory.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_misc.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_module.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_mpeg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_msg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_video.cpp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\string.cpp
# End Source File
# Begin Source File

SOURCE="..\src\libsp\win32\win32-stuff.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\libsp\containers\clist.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\dirent.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\dllist.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\hashlist.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\list.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\membin.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\resource.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\slist.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_bswap.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_cdrom.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\sp_css.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_eeprom.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_fip.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_flash.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_io.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_khwl.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_memory.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_misc.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_module.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_mpeg.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_msg.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\sp_video.h
# End Source File
# Begin Source File

SOURCE=..\src\libsp\containers\string.h
# End Source File
# Begin Source File

SOURCE="..\src\libsp\win32\win32-stuff.h"
# End Source File
# End Group
# End Target
# End Project
