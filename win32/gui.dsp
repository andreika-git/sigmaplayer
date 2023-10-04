# Microsoft Developer Studio Project File - Name="gui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=gui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gui.mak" CFG="gui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gui - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "gui - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release/gui"
# PROP BASE Intermediate_Dir "Release/gui"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release/gui"
# PROP Intermediate_Dir "Release/gui"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /I "../src/libsp/win32/include" /I "../src" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "gui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug/gui"
# PROP BASE Intermediate_Dir "Debug/gui"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug/gui"
# PROP Intermediate_Dir "Debug/gui"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "../src/libsp/win32/include" /I "../src" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

# Name "gui - Win32 Release"
# Name "gui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\gui\console.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\font.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\giflib.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\image.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\jpeg.cpp
# ADD CPP /I "../src/contrib/libjpeg"
# End Source File
# Begin Source File

SOURCE=..\src\gui\mmsl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\rect.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\res.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\text.cpp
# End Source File
# Begin Source File

SOURCE=..\src\gui\window.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\gui\console.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\font.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\giflib.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\image.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\jpeg.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\mmsl.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\rect.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\res.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\text.h
# End Source File
# Begin Source File

SOURCE=..\src\gui\window.h
# End Source File
# End Group
# End Target
# End Project
