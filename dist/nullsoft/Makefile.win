# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2007 Chris Schoeneman
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

NSIS = "$(PROGRAMFILES)\NSIS\makensis.exe"
NSIS_FLAGS = /NOCD /V1

BIN_INSTALLER_SRC = dist\nullsoft
BIN_INSTALLER_DST = $(BUILD_DST)\$(BIN_INSTALLER_SRC)
BIN_DOSIFY_EXE = "$(BIN_INSTALLER_DST)\dosify.exe"
BIN_DOSIFY_C =							\
	"$(BIN_INSTALLER_SRC)\dosify.c"		\
	$(NULL)
BIN_DOSIFY_OBJ =						\
	"$(BIN_INSTALLER_DST)\dosify.obj"	\
	$(NULL)

BIN_INSTALLER_NSI = "$(BIN_INSTALLER_SRC)\synergy.nsi"
BIN_INSTALLER_EXE = "$(BUILD_DST)\SynergyInstaller.exe"
BIN_INSTALLER_DOCS =		\
	COPYING					\
	ChangeLog				\
	$(NULL)
BIN_INSTALLER_DOS_DOCS =			\
	$(BUILD_DST)\COPYING.txt		\
	$(BUILD_DST)\ChangeLog.txt		\
	$(NULL)

C_FILES     = $(C_FILES)     $(BIN_DOSIFY_C)
OBJ_FILES   = $(OBJ_FILES)   $(BIN_DOSIFY_OBJ)
OPTPROGRAMS = $(OPTPROGRAMS) $(BIN_DOSIFY_EXE)

# Build rules.
$(BIN_DOSIFY_OBJ): $(BIN_DOSIFY_C)
	@$(ECHO) Compile $(BIN_DOSIFY_C)
	-@$(MKDIR) $(BIN_INSTALLER_DST) 2>NUL:
	$(cc) $(cdebug) $(cflags) $(cvars) /Fo$@ /Fd$(@:.obj=.pdb) $**
$(BIN_DOSIFY_EXE): $(BIN_DOSIFY_OBJ)
	@$(ECHO) Link $(@F)
	$(link) $(ldebug) $(conlflags) $(conlibsmt) /out:$@ $**

# Convert text files from Unix to DOS format.
$(BIN_INSTALLER_DOS_DOCS): $(BIN_DOSIFY_EXE) $(BIN_INSTALLER_DOCS)
	@$(ECHO) Convert text files to DOS format
	$(BIN_DOSIFY_EXE) "." "$(BUILD_DST)" $(BIN_INSTALLER_DOCS)

# Allow installers for both debug and release.
$(BIN_INSTALLER_EXE): $(BIN_INSTALLER_NSI) all $(BIN_INSTALLER_DOS_DOCS)
	@$(ECHO) Build $(@F)
	$(NSIS) $(NSIS_FLAGS) /DOUTPUTDIR=$(@D) /DOUTPUTFILE=$@ $(BIN_INSTALLER_NSI)
installer: $(BIN_INSTALLER_EXE)
debug-installer:
	@$(MAKE) /nologo /f $(MAKEFILE) DEBUG=1 installer
release-installer:
	@$(MAKE) /nologo /f $(MAKEFILE) NODEBUG=1 installer
