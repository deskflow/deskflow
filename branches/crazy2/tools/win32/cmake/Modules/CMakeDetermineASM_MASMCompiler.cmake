
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

# Find the MS assembler (masm or masm64)

SET(ASM_DIALECT "_MASM")

# if we are using the 64bit cl compiler, assume we also want the 64bit assembler
IF(CMAKE_CL_64)
   SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT ml64)
ELSE(CMAKE_CL_64)
   SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT ml)
ENDIF(CMAKE_CL_64)

INCLUDE(CMakeDetermineASMCompiler)
SET(ASM_DIALECT)
