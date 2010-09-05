# Microsoft Developer Studio Project File - Name="arch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=arch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "arch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "arch.mak" CFG="arch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "arch - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "arch - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "arch - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\gen\build"
# PROP Intermediate_Dir "..\..\gen\build"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GR /GX /O2 /I "..\common" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\build\arch.pdb" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "arch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\gen\debug"
# PROP Intermediate_Dir "..\..\gen\debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GR /GX /ZI /Od /I "..\base" /I "..\mt" /I "..\common" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\debug\arch.pdb" /FD /GZ /c
# SUBTRACT CPP /YX
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

# Name "arch - Win32 Release"
# Name "arch - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CArch.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchConsoleWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchDaemonWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchFileWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchLogWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchMiscWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchMultithreadWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchNetworkWinsock.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchSleepWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchStringWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchSystemWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchTaskBarWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CArchTimeWindows.cpp
# End Source File
# Begin Source File

SOURCE=.\CMultibyte.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\vsnprintf.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\XArch.cpp
# End Source File
# Begin Source File

SOURCE=.\XArchWindows.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CArch.h
# End Source File
# Begin Source File

SOURCE=.\CArchConsoleWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchDaemonWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchFileWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchLogWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchMiscWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchMultithreadWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchNetworkWinsock.h
# End Source File
# Begin Source File

SOURCE=.\CArchSleepWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchStringWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchSystemWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchTaskBarWindows.h
# End Source File
# Begin Source File

SOURCE=.\CArchTimeWindows.h
# End Source File
# Begin Source File

SOURCE=.\IArchConsole.h
# End Source File
# Begin Source File

SOURCE=.\IArchDaemon.h
# End Source File
# Begin Source File

SOURCE=.\IArchFile.h
# End Source File
# Begin Source File

SOURCE=.\IArchLog.h
# End Source File
# Begin Source File

SOURCE=.\IArchMultithread.h
# End Source File
# Begin Source File

SOURCE=.\IArchNetwork.h
# End Source File
# Begin Source File

SOURCE=.\IArchSleep.h
# End Source File
# Begin Source File

SOURCE=.\IArchString.h
# End Source File
# Begin Source File

SOURCE=.\IArchSystem.h
# End Source File
# Begin Source File

SOURCE=.\IArchTaskBar.h
# End Source File
# Begin Source File

SOURCE=.\IArchTaskBarReceiver.h
# End Source File
# Begin Source File

SOURCE=.\IArchTime.h
# End Source File
# Begin Source File

SOURCE=.\XArch.h
# End Source File
# Begin Source File

SOURCE=.\XArchWindows.h
# End Source File
# End Group
# End Target
# End Project
