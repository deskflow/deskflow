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

LIB_ARCH_SRC = lib\arch
LIB_ARCH_DST = $(BUILD_DST)\$(LIB_ARCH_SRC)
LIB_ARCH_LIB = "$(LIB_ARCH_DST)\arch.lib"
LIB_ARCH_CPP =						\
	"CArch.cpp"						\
	"CArchDaemonNone.cpp"			\
	"XArch.cpp"						\
	"CArchConsoleWindows.cpp"		\
	"CArchDaemonWindows.cpp"		\
	"CArchFileWindows.cpp"			\
	"CArchLogWindows.cpp"			\
	"CArchMiscWindows.cpp"			\
	"CArchMultithreadWindows.cpp"	\
	"CArchNetworkWinsock.cpp"		\
	"CArchSleepWindows.cpp"			\
	"CArchStringWindows.cpp"		\
	"CArchSystemWindows.cpp"		\
	"CArchTaskBarWindows.cpp"		\
	"CArchTimeWindows.cpp"			\
	"XArchWindows.cpp"				\
	$(NULL)
LIB_ARCH_OBJ =										\
	"$(LIB_ARCH_DST)\CArch.obj"						\
	"$(LIB_ARCH_DST)\CArchDaemonNone.obj"			\
	"$(LIB_ARCH_DST)\XArch.obj"						\
	"$(LIB_ARCH_DST)\CArchConsoleWindows.obj"		\
	"$(LIB_ARCH_DST)\CArchDaemonWindows.obj"		\
	"$(LIB_ARCH_DST)\CArchFileWindows.obj"			\
	"$(LIB_ARCH_DST)\CArchLogWindows.obj"			\
	"$(LIB_ARCH_DST)\CArchMiscWindows.obj"			\
	"$(LIB_ARCH_DST)\CArchMultithreadWindows.obj"	\
	"$(LIB_ARCH_DST)\CArchNetworkWinsock.obj"		\
	"$(LIB_ARCH_DST)\CArchSleepWindows.obj"			\
	"$(LIB_ARCH_DST)\CArchStringWindows.obj"		\
	"$(LIB_ARCH_DST)\CArchSystemWindows.obj"		\
	"$(LIB_ARCH_DST)\CArchTaskBarWindows.obj"		\
	"$(LIB_ARCH_DST)\CArchTimeWindows.obj"			\
	"$(LIB_ARCH_DST)\XArchWindows.obj"				\
	$(NULL)
LIB_ARCH_INC =						\
	/I"lib\common"					\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_ARCH_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_ARCH_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_ARCH_LIB)

# Dependency rules
$(LIB_ARCH_OBJ): $(AUTODEP)
!if EXIST($(LIB_ARCH_DST)\deps.mak)
!include $(LIB_ARCH_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_ARCH_SRC)\}.cpp{$(LIB_ARCH_DST)\}.obj::
!else
{$(LIB_ARCH_SRC)\}.cpp{$(LIB_ARCH_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_ARCH_SRC)
	-@$(MKDIR) $(LIB_ARCH_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_ARCH_INC) \
		/Fo$(LIB_ARCH_DST)\ \
		/Fd$(LIB_ARCH_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_ARCH_SRC) $(LIB_ARCH_DST)
$(LIB_ARCH_LIB): $(LIB_ARCH_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_ARCH_SRC) $(LIB_ARCH_DST) $(**:.obj=.d)
