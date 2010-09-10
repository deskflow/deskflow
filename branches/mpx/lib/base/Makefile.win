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

LIB_BASE_SRC = lib\base
LIB_BASE_DST = $(BUILD_DST)\$(LIB_BASE_SRC)
LIB_BASE_LIB = "$(LIB_BASE_DST)\base.lib"
LIB_BASE_CPP =						\
	"CEvent.cpp"					\
	"CEventQueue.cpp"				\
	"CFunctionEventJob.cpp"			\
	"CFunctionJob.cpp"				\
	"CLog.cpp"						\
	"CSimpleEventQueueBuffer.cpp"	\
	"CStopwatch.cpp"				\
	"CStringUtil.cpp"				\
	"CUnicode.cpp"					\
	"IEventQueue.cpp"				\
	"LogOutputters.cpp"				\
	"XBase.cpp"						\
	$(NULL)
LIB_BASE_OBJ =										\
	"$(LIB_BASE_DST)\CEvent.obj"					\
	"$(LIB_BASE_DST)\CEventQueue.obj"				\
	"$(LIB_BASE_DST)\CFunctionEventJob.obj"			\
	"$(LIB_BASE_DST)\CFunctionJob.obj"				\
	"$(LIB_BASE_DST)\CLog.obj"						\
	"$(LIB_BASE_DST)\CSimpleEventQueueBuffer.obj"	\
	"$(LIB_BASE_DST)\CStopwatch.obj"				\
	"$(LIB_BASE_DST)\CStringUtil.obj"				\
	"$(LIB_BASE_DST)\CUnicode.obj"					\
	"$(LIB_BASE_DST)\IEventQueue.obj"				\
	"$(LIB_BASE_DST)\LogOutputters.obj"				\
	"$(LIB_BASE_DST)\XBase.obj"						\
	$(NULL)
LIB_BASE_INC =						\
	/I"lib\common"					\
	/I"lib\arch"					\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_BASE_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_BASE_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_BASE_LIB)

# Dependency rules
$(LIB_BASE_OBJ): $(AUTODEP)
!if EXIST($(LIB_BASE_DST)\deps.mak)
!include $(LIB_BASE_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_BASE_SRC)\}.cpp{$(LIB_BASE_DST)\}.obj::
!else
{$(LIB_BASE_SRC)\}.cpp{$(LIB_BASE_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_BASE_SRC)
	-@$(MKDIR) $(LIB_BASE_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_BASE_INC) \
		/Fo$(LIB_BASE_DST)\ \
		/Fd$(LIB_BASE_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_BASE_SRC) $(LIB_BASE_DST)
$(LIB_BASE_LIB): $(LIB_BASE_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_BASE_SRC) $(LIB_BASE_DST) $(**:.obj=.d)
