# Microsoft Developer Studio Project File - Name="init" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=init - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "init.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "init.mak" CFG="init - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "init - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "init - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "init - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /I "../src/libsp/win32/include" /I "../src" /I "../src/contrib/libdvdnav/src" /I "../src/contrib/libdvdnav/src/dvdread" /I "../src/contrib/libdvdnav/msvc" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "DVDCSS_USE_EXTERNAL_CSS" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes /machine:I386 /out:"../init.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "init - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "../src/libsp/win32/include" /I "../src" /I "../src/contrib/libdvdnav/src" /I "../src/contrib/libdvdnav/src/dvdread" /I "../src/contrib/libdvdnav/msvc" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "DVDCSS_USE_EXTERNAL_CSS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../init.exe" /pdbtype:sept /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "init - Win32 Release"
# Name "init - Win32 Debug"
# Begin Group "Source files"

# PROP Default_Filter "*.cpp;*.c"
# Begin Source File

SOURCE=..\src\audio.cpp
# End Source File
# Begin Source File

SOURCE=..\src\avi.cpp
# End Source File
# Begin Source File

SOURCE=..\src\bitstream.cpp
# End Source File
# Begin Source File

SOURCE=..\src\cdda.cpp
# End Source File
# Begin Source File

SOURCE=..\src\divx.cpp
# End Source File
# Begin Source File

SOURCE=..\src\init.cpp
# End Source File
# Begin Source File

SOURCE=..\src\media.cpp
# ADD CPP /D "DVDNAV_COMPILE"
# End Source File
# Begin Source File

SOURCE="..\src\module-dvd.cpp"
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE="..\src\module-init.cpp"
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=..\src\module.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=..\src\mpg.cpp
# End Source File
# Begin Source File

SOURCE=..\src\player.cpp
# ADD CPP /D "PLAYER_INFO_EMBED"
# End Source File
# Begin Source File

SOURCE=..\src\info\player_info.cpp
# ADD CPP /D "PLAYER_INFO_EMBED"
# End Source File
# Begin Source File

SOURCE="..\src\script-explorer.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\script-objects.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\script-player.cpp"
# End Source File
# Begin Source File

SOURCE=..\src\script.cpp
# End Source File
# Begin Source File

SOURCE=..\src\settings.cpp
# End Source File
# Begin Source File

SOURCE=..\src\subtitle.cpp
# End Source File
# Begin Source File

SOURCE=..\src\video.cpp
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\src\audio.h
# End Source File
# Begin Source File

SOURCE=..\src\avi.h
# End Source File
# Begin Source File

SOURCE=..\src\bitstream.h
# End Source File
# Begin Source File

SOURCE=..\src\cdda.h
# End Source File
# Begin Source File

SOURCE="..\src\divx-tables.h"
# End Source File
# Begin Source File

SOURCE=..\src\divx.h
# End Source File
# Begin Source File

SOURCE=..\src\module.h
# End Source File
# Begin Source File

SOURCE=..\src\mpg.h
# End Source File
# Begin Source File

SOURCE=..\src\player.h
# End Source File
# Begin Source File

SOURCE=..\src\info\player_info.h
# End Source File
# Begin Source File

SOURCE="..\src\script-internal.h"
# End Source File
# Begin Source File

SOURCE=..\src\script.h
# End Source File
# Begin Source File

SOURCE=..\src\settings.h
# End Source File
# Begin Source File

SOURCE=..\src\subtitle.h
# End Source File
# Begin Source File

SOURCE=..\src\video.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\libsp\win32\fip.bmp
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\libsp.rc
# End Source File
# Begin Source File

SOURCE=..\src\libsp\win32\manifest.xml
# End Source File
# End Target
# End Project
