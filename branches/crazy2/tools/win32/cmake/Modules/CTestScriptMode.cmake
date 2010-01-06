# This file is read by ctest in script mode (-S)

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Alexander Neundorf <neundorf@kde.org>
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

# Determine the current system, so this information can be used 
# in ctest scripts
include(CMakeDetermineSystem)

# Also load the system specific file, which sets up e.g. the search paths.
# This makes the FIND_XXX() calls work much better
include(CMakeSystemSpecificInformation)

