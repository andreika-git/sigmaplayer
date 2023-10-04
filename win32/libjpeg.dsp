# Microsoft Developer Studio Project File - Name="libjpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libjpeg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libjpeg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libjpeg.mak" CFG="libjpeg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libjpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libjpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libjpeg - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX  /Fo"Release/libjpeg/" /Fd"Release/libjpeg/" /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /Fo"Debug/libjpeg/" /Fd"Debug/libjpeg/" /FD /GZ /c
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

# Name "libjpeg - Win32 Release"
# Name "libjpeg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jcomapi.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdapimin.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdapistd.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdatadst.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdinput.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdmainct.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdmarker.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdmerge.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdphuff.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdpostct.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdtrans.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jerror.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jidctflt.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jidctfst.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jidctint.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jidctred.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jmemansi.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdct.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jerror.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\src\contrib\libjpeg\jversion.h
# End Source File
# End Group
# End Target
# End Project
