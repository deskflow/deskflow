
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

file(GLOB _files ${ECOS_DIR}/*)

# remove all directories, which consist of lower-case letters only
# this skips e.g. CVS/ and .subversion/
foreach(_entry ${_files})
   if(IS_DIRECTORY ${_entry})
      get_filename_component(dir ${_entry} NAME)
      if(${dir} MATCHES "^[a-z]+$")
         file(REMOVE_RECURSE ${_entry})
      endif(${dir} MATCHES "^[a-z]+$")
   endif(IS_DIRECTORY ${_entry})
endforeach(_entry)
