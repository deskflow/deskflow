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

LIB_MT_SRC = lib\mt
LIB_MT_DST = $(BUILD_DST)\$(LIB_MT_SRC)
LIB_MT_LIB = "$(LIB_MT_DST)\mt.lib"
LIB_MT_CPP =						\
	"CCondVar.cpp"					\
	"CLock.cpp"						\
	"CMutex.cpp"					\
	"CThread.cpp"					\
	"XMT.cpp"						\
	$(NULL)
LIB_MT_OBJ =						\
	"$(LIB_MT_DST)\CCondVar.obj"	\
	"$(LIB_MT_DST)\CLock.obj"		\
	"$(LIB_MT_DST)\CMutex.obj"		\
	"$(LIB_MT_DST)\CThread.obj"		\
	"$(LIB_MT_DST)\XMT.obj"			\
	$(NULL)
LIB_MT_INC =						\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_MT_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_MT_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_MT_LIB)

# Dependency rules
$(LIB_MT_OBJ): $(AUTODEP)
!if EXIST($(LIB_MT_DST)\deps.mak)
!include $(LIB_MT_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_MT_SRC)\}.cpp{$(LIB_MT_DST)\}.obj::
!else
{$(LIB_MT_SRC)\}.cpp{$(LIB_MT_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_MT_SRC)
	-@$(MKDIR) $(LIB_MT_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_MT_INC) \
		/Fo$(LIB_MT_DST)\ \
		/Fd$(LIB_MT_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_MT_SRC) $(LIB_MT_DST)
$(LIB_MT_LIB): $(LIB_MT_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_MT_SRC) $(LIB_MT_DST) $(**:.obj=.d)
