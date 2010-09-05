# Microsoft Developer Studio Project File - Name="platform" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=platform - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "platform.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "platform.mak" CFG="platform - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "platform - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "platform - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "platform - Win32 Release"

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
# ADD CPP /nologo /MT /W4 /GX /O2 /I "..\common" /I "..\arch" /I "..\base" /I "..\mt" /I "..\io" /I "..\synergy" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\build\platform.pdb" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "platform - Win32 Debug"

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
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "..\common" /I "..\arch" /I "..\base" /I "..\mt" /I "..\io" /I "..\synergy" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fd"..\..\gen\debug\platform.pdb" /FD /GZ /c
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

# Name "platform - Win32 Release"
# Name "platform - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CMSWindowsClipboard.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardAnyTextConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardBitmapConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardHTMLConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardTextConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardUTF16Converter.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsDesks.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsEventQueueBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsKeyState.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsScreen.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsScreenSaver.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsUtil.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CMSWindowsClipboard.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardAnyTextConverter.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardBitmapConverter.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardHTMLConverter.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardTextConverter.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsClipboardUTF16Converter.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsDesks.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsEventQueueBuffer.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsKeyState.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsScreen.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsScreenSaver.h
# End Source File
# Begin Source File

SOURCE=.\CMSWindowsUtil.h
# End Source File
# End Group
# End Target
# End Project
