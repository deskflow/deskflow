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

BIN_SYNERGYS_SRC = cmd\synergys
BIN_SYNERGYS_DST = $(BUILD_DST)\$(BIN_SYNERGYS_SRC)
BIN_SYNERGYS_EXE = "$(BUILD_DST)\synergys.exe"
BIN_SYNERGYS_CPP =							\
	"CServerTaskBarReceiver.cpp"			\
	"CMSWindowsServerTaskBarReceiver.cpp"	\
	"synergys.cpp"							\
	$(NULL)
BIN_SYNERGYS_OBJ =												\
	"$(BIN_SYNERGYS_DST)\CServerTaskBarReceiver.obj"			\
	"$(BIN_SYNERGYS_DST)\CMSWindowsServerTaskBarReceiver.obj"	\
	"$(BIN_SYNERGYS_DST)\synergys.obj"							\
	$(NULL)
BIN_SYNERGYS_RC = "$(BIN_SYNERGYS_SRC)\synergys.rc"
BIN_SYNERGYS_RES = "$(BIN_SYNERGYS_DST)\synergys.res"
BIN_SYNERGYS_INC =					\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	/I"lib\io"						\
	/I"lib\net"						\
	/I"lib\synergy"					\
	/I"lib\platform"				\
	/I"lib\server"					\
	$(NULL)
BIN_SYNERGYS_LIB =					\
	$(LIB_SERVER_LIB)				\
	$(LIB_PLATFORM_LIB)				\
	$(LIB_SYNERGY_LIB)				\
	$(LIB_NET_LIB)					\
	$(LIB_IO_LIB)					\
	$(LIB_MT_LIB)					\
	$(LIB_BASE_LIB)					\
	$(LIB_ARCH_LIB)					\
	$(LIB_COMMON_LIB)				\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(BIN_SYNERGYS_CPP)
OBJ_FILES = $(OBJ_FILES) $(BIN_SYNERGYS_OBJ)
PROGRAMS  = $(PROGRAMS)  $(BIN_SYNERGYS_EXE)

# Need shell functions.
guilibs = $(guilibs) shell32.lib

# Dependency rules
$(BIN_SYNERGYS_OBJ): $(AUTODEP)
!if EXIST($(BIN_SYNERGYS_DST)\deps.mak)
!include $(BIN_SYNERGYS_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(BIN_SYNERGYS_SRC)\}.cpp{$(BIN_SYNERGYS_DST)\}.obj::
!else
{$(BIN_SYNERGYS_SRC)\}.cpp{$(BIN_SYNERGYS_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(BIN_SYNERGYS_SRC)
	-@$(MKDIR) $(BIN_SYNERGYS_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(BIN_SYNERGYS_INC) \
		/Fo$(BIN_SYNERGYS_DST)\ \
		/Fd$(BIN_SYNERGYS_DST)\src.pdb \
		$< | $(AUTODEP) $(BIN_SYNERGYS_SRC) $(BIN_SYNERGYS_DST)
$(BIN_SYNERGYS_RES): $(BIN_SYNERGYS_RC)
	@$(ECHO) Compile $(**F)
	-@$(MKDIR) $(BIN_SYNERGYS_DST) 2>NUL:
	$(rc) $(rcflags) $(rcvars) \
		/fo$@ \
		$**
$(BIN_SYNERGYS_EXE): $(BIN_SYNERGYS_OBJ) $(BIN_SYNERGYS_RES) $(BIN_SYNERGYS_LIB)
	@$(ECHO) Link $(@F)
	$(link) $(ldebug) $(guilflags) $(guilibsmt) \
		/out:$@ \
		$**
	$(AUTODEP) $(BIN_SYNERGYS_SRC) $(BIN_SYNERGYS_DST) \
		$(BIN_SYNERGYS_OBJ:.obj=.d)
