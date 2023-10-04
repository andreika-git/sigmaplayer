# Microsoft Developer Studio Project File - Name="libmad" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmad - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmad.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmad.mak" CFG="libmad - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmad - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmad - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmad - Win32 Release"

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
# ADD CPP /nologo /MD /GX /O2 /I "..\src\contrib\libmad\msvc++" /I "..\src\libsp\win32\include" /D "NDEBUG" /D "FPM_INTEL" /D "WIN32" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /D "ASO_ZEROCHECK" /Fo"Release/libmad/" /Fd"Release/libmad/" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libmad - Win32 Debug"

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
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "..\src\contrib\libmad\msvc++" /I "..\src\libsp\win32\include" /D "FPM_DEFAULT" /D "_LIB" /D "HAVE_CONFIG_H" /D "ASO_ZEROCHECK" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "DEBUG" /Fo"Debug/libmad/" /Fd"Debug/libmad/" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libmad - Win32 Release"
# Name "libmad - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\src\contrib\libmad\bit.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\decoder.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\fixed.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\frame.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\huffman.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\layer12.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\layer3.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\stream.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\synth.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\timer.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\version.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\src\contrib\libmad\bit.h
# End Source File
# Begin Source File

SOURCE=.\src\contrib\libmad\config.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\decoder.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\fixed.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\frame.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\global.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\huffman.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\layer12.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\layer3.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\stream.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\synth.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\timer.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\version.h
# End Source File
# End Group
# Begin Group "Data Files"

# PROP Default_Filter "dat"
# Begin Source File

SOURCE=..\src\contrib\libmad\D.dat
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\imdct_s.dat
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\qc_table.dat
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\rq_table.dat
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libmad\sf_table.dat
# End Source File
# End Group
# End Target
# End Project
