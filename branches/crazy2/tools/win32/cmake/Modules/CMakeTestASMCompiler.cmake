
#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
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

# This file is used by EnableLanguage in cmGlobalGenerator to
# determine that that selected ASM compiler can actually compile
# and link the most basic of programs.   If not, a fatal error
# is set and cmake stops processing commands and will not generate
# any makefiles or projects.
IF(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_WORKS 1 CACHE INTERNAL "")
ELSE(CMAKE_ASM${ASM_DIALECT}_COMPILER)
  SET(CMAKE_ASM${ASM_DIALECT}_COMPILER_WORKS 0 CACHE INTERNAL "")
ENDIF(CMAKE_ASM${ASM_DIALECT}_COMPILER)
