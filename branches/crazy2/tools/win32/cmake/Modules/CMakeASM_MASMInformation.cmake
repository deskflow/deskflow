
#=============================================================================
# Copyright 2008-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# support for the MS assembler, masm and masm64

SET(ASM_DIALECT "_MASM")

SET(CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS asm)

SET(CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "<CMAKE_ASM${ASM_DIALECT}_COMPILER> <FLAGS> /c  /Fo <OBJECT> <SOURCE>")

INCLUDE(CMakeASMInformation)
SET(ASM_DIALECT)
