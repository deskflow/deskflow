# Microsoft Developer Studio Project File - Name="installer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=installer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "installer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "installer.mak" CFG="installer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "installer - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "installer - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "installer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "installer___Win32_Release"
# PROP BASE Intermediate_Dir "installer___Win32_Release"
# PROP BASE Cmd_Line "NMAKE /f installer.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "installer.exe"
# PROP BASE Bsc_Name "installer.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "."
# PROP Cmd_Line "nmake /nologo /f "installer.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "..\..\build\SynergyInstaller.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""
NSIS=makensis

!ELSEIF  "$(CFG)" == "installer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "installer___Win32_Debug"
# PROP BASE Intermediate_Dir "installer___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f installer.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "installer.exe"
# PROP BASE Bsc_Name "installer.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "."
# PROP Cmd_Line "echo "Can only build installer for Release configuration""
# PROP Rebuild_Opt ""
# PROP Target_File "dummy.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""
NSIS=makensis

!ENDIF 

# Begin Target

# Name "installer - Win32 Release"
# Name "installer - Win32 Debug"

!IF  "$(CFG)" == "installer - Win32 Release"

!ELSEIF  "$(CFG)" == "installer - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dosify.c
# PROP Intermediate_Dir "."
# End Source File
# Begin Source File

SOURCE=.\installer.mak
# PROP Intermediate_Dir "."
# End Source File
# Begin Source File

SOURCE=.\synergy.nsi
# PROP Intermediate_Dir "."
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
