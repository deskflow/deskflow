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

LIB_COMMON_SRC = lib\common
LIB_COMMON_DST = $(BUILD_DST)\$(LIB_COMMON_SRC)
LIB_COMMON_LIB = "$(LIB_COMMON_DST)\common.lib"
LIB_COMMON_CPP =					\
	Version.cpp						\
	$(NULL)
LIB_COMMON_OBJ =					\
	"$(LIB_COMMON_DST)\Version.obj"	\
	$(NULL)
LIB_COMMON_INC =					\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_COMMON_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_COMMON_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_COMMON_LIB)

# Dependency rules
$(LIB_COMMON_OBJ): $(AUTODEP)
!if EXIST($(LIB_COMMON_DST)\deps.mak)
!include $(LIB_COMMON_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_COMMON_SRC)\}.cpp{$(LIB_COMMON_DST)\}.obj::
!else
{$(LIB_COMMON_SRC)\}.cpp{$(LIB_COMMON_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_COMMON_SRC)
	-@$(MKDIR) $(LIB_COMMON_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_COMMON_INC) \
		/Fo$(LIB_COMMON_DST)\ \
		/Fd$(LIB_COMMON_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_COMMON_SRC) $(LIB_COMMON_DST)
$(LIB_COMMON_LIB): $(LIB_COMMON_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_COMMON_SRC) $(LIB_COMMON_DST) $(**:.obj=.d)
