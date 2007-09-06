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

LIB_IO_SRC = lib\io
LIB_IO_DST = $(BUILD_DST)\$(LIB_IO_SRC)
LIB_IO_LIB = "$(LIB_IO_DST)\io.lib"
LIB_IO_CPP =						\
	"CStreamBuffer.cpp"				\
	"CStreamFilter.cpp"				\
	"IStream.cpp"					\
	"XIO.cpp"						\
	$(NULL)
LIB_IO_OBJ =							\
	"$(LIB_IO_DST)\CStreamBuffer.obj"	\
	"$(LIB_IO_DST)\CStreamFilter.obj"	\
	"$(LIB_IO_DST)\IStream.obj"			\
	"$(LIB_IO_DST)\XIO.obj"				\
	$(NULL)
LIB_IO_INC =						\
	/I"lib\common"					\
	/I"lib\arch"					\
	/I"lib\base"					\
	/I"lib\mt"						\
	$(NULL)

CPP_FILES = $(CPP_FILES) $(LIB_IO_CPP)
OBJ_FILES = $(OBJ_FILES) $(LIB_IO_OBJ)
LIB_FILES = $(LIB_FILES) $(LIB_IO_LIB)

# Dependency rules
$(LIB_IO_OBJ): $(AUTODEP)
!if EXIST($(LIB_IO_DST)\deps.mak)
!include $(LIB_IO_DST)\deps.mak
!endif

# Build rules.  Use batch-mode rules if possible.
!if DEFINED(_NMAKE_VER)
{$(LIB_IO_SRC)\}.cpp{$(LIB_IO_DST)\}.obj::
!else
{$(LIB_IO_SRC)\}.cpp{$(LIB_IO_DST)\}.obj:
!endif
	@$(ECHO) Compile in $(LIB_IO_SRC)
	-@$(MKDIR) $(LIB_IO_DST) 2>NUL:
	$(cpp) $(cppdebug) $(cppflags) $(cppvarsmt) /showIncludes \
		$(LIB_IO_INC) \
		/Fo$(LIB_IO_DST)\ \
		/Fd$(LIB_IO_LIB:.lib=.pdb) \
		$< | $(AUTODEP) $(LIB_IO_SRC) $(LIB_IO_DST)
$(LIB_IO_LIB): $(LIB_IO_OBJ)
	@$(ECHO) Link $(@F)
	$(implib) $(ildebug) $(ilflags) \
		/out:$@ \
		$**
	$(AUTODEP) $(LIB_IO_SRC) $(LIB_IO_DST) $(**:.obj=.d)
