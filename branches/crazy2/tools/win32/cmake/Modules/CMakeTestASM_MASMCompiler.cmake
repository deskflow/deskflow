
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

# This file is used by EnableLanguage in cmGlobalGenerator to
# determine that the selected ASM_MASM "compiler" (should be masm or masm64) 
# can actually "compile" and link the most basic of programs.   If not, a 
# fatal error is set and cmake stops processing commands and will not generate
# any makefiles or projects.

SET(ASM_DIALECT "_MASM")
INCLUDE(CMakeTestASMCompiler)
SET(ASM_DIALECT)
