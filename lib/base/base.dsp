# Microsoft Developer Studio Project File - Name="base" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=base - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "base.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "base.mak" CFG="base - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "base - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "base - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "base - Win32 Release"

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
# ADD CPP /nologo /MT /W4 /GR /GX /O2 /I "..\common" /I "..\arch" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\build\base.pdb" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "base - Win32 Debug"

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
# ADD CPP /nologo /MTd /W4 /Gm /GR /GX /ZI /Od /I "..\common" /I "..\arch" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\debug\base.pdb" /FD /GZ /c
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

# Name "base - Win32 Release"
# Name "base - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\CEventQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\CFunctionEventJob.cpp
# End Source File
# Begin Source File

SOURCE=.\CFunctionJob.cpp
# End Source File
# Begin Source File

SOURCE=.\CLog.cpp
# End Source File
# Begin Source File

SOURCE=.\CSimpleEventQueueBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\CStopwatch.cpp
# End Source File
# Begin Source File

SOURCE=.\CStringUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\CUnicode.cpp
# End Source File
# Begin Source File

SOURCE=.\IEventQueue.cpp
# End Source File
# Begin Source File

SOURCE=.\LogOutputters.cpp
# End Source File
# Begin Source File

SOURCE=.\XBase.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CEvent.h
# End Source File
# Begin Source File

SOURCE=.\CEventQueue.h
# End Source File
# Begin Source File

SOURCE=.\CFunctionEventJob.h
# End Source File
# Begin Source File

SOURCE=.\CFunctionJob.h
# End Source File
# Begin Source File

SOURCE=.\CLog.h
# End Source File
# Begin Source File

SOURCE=.\CPriorityQueue.h
# End Source File
# Begin Source File

SOURCE=.\CSimpleEventQueueBuffer.h
# End Source File
# Begin Source File

SOURCE=.\CStopwatch.h
# End Source File
# Begin Source File

SOURCE=.\CString.h
# End Source File
# Begin Source File

SOURCE=.\CStringUtil.h
# End Source File
# Begin Source File

SOURCE=.\CUnicode.h
# End Source File
# Begin Source File

SOURCE=.\IEventJob.h
# End Source File
# Begin Source File

SOURCE=.\IEventQueue.h
# End Source File
# Begin Source File

SOURCE=.\IEventQueueBuffer.h
# End Source File
# Begin Source File

SOURCE=.\IJob.h
# End Source File
# Begin Source File

SOURCE=.\ILogOutputter.h
# End Source File
# Begin Source File

SOURCE=.\LogOutputters.h
# End Source File
# Begin Source File

SOURCE=.\TMethodEventJob.h
# End Source File
# Begin Source File

SOURCE=.\TMethodJob.h
# End Source File
# Begin Source File

SOURCE=.\XBase.h
# End Source File
# End Group
# End Target
# End Project
