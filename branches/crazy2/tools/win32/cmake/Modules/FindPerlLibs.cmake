# - Find Perl libraries
# This module finds if PERL is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PERLLIBS_FOUND    = True if perl.h & libperl were found
#  PERL_INCLUDE_PATH = path to where perl.h is found
#  PERL_LIBRARY      = path to libperl
#  PERL_EXECUTABLE   = full path to the perl binary
#
#  The following variables are also available if needed
#  (introduced after CMake 2.6.4)
#
#  PERL_SITESEARCH    = path to the sitesearch install dir
#  PERL_SITELIB       = path to the sitelib install directory
#  PERL_VENDORARCH    = path to the vendor arch install directory
#  PERL_VENDORLIB     = path to the vendor lib install directory
#  PERL_ARCHLIB       = path to the arch lib install directory
#  PERL_PRIVLIB       = path to the priv lib install directory
#  PERL_EXTRA_C_FLAGS = Compilation flags used to build perl
#

#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
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

# find the perl executable
include(FindPerl)

if (PERL_EXECUTABLE)
  ### PERL_PREFIX
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:prefix
      OUTPUT_VARIABLE
        PERL_PREFIX_OUTPUT_VARIABLE
      RESULT_VARIABLE
        PERL_PREFIX_RESULT_VARIABLE
  )

  if (NOT PERL_PREFIX_RESULT_VARIABLE)
    string(REGEX REPLACE "prefix='([^']+)'.*" "\\1" PERL_PREFIX ${PERL_PREFIX_OUTPUT_VARIABLE})
  endif (NOT PERL_PREFIX_RESULT_VARIABLE)

  ### PERL_VERSION
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:version
      OUTPUT_VARIABLE
        PERL_VERSION_OUTPUT_VARIABLE
      RESULT_VARIABLE
        PERL_VERSION_RESULT_VARIABLE
  )
  if (NOT PERL_VERSION_RESULT_VARIABLE)
    string(REGEX REPLACE "version='([^']+)'.*" "\\1" PERL_VERSION ${PERL_VERSION_OUTPUT_VARIABLE})
  endif (NOT PERL_VERSION_RESULT_VARIABLE)

  ### PERL_ARCHNAME
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:archname
      OUTPUT_VARIABLE
        PERL_ARCHNAME_OUTPUT_VARIABLE
      RESULT_VARIABLE
        PERL_ARCHNAME_RESULT_VARIABLE
  )
  if (NOT PERL_ARCHNAME_RESULT_VARIABLE)
    string(REGEX REPLACE "archname='([^']+)'.*" "\\1" PERL_ARCHNAME ${PERL_ARCHNAME_OUTPUT_VARIABLE})
  endif (NOT PERL_ARCHNAME_RESULT_VARIABLE)



  ### PERL_EXTRA_C_FLAGS
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:cppflags
    OUTPUT_VARIABLE
      PERL_CPPFLAGS_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_CPPFLAGS_RESULT_VARIABLE
    )
  if (NOT PERL_CPPFLAGS_RESULT_VARIABLE)
    string(REGEX REPLACE "cppflags='([^']+)'.*" "\\1" PERL_EXTRA_C_FLAGS ${PERL_CPPFLAGS_OUTPUT_VARIABLE})
  endif (NOT PERL_CPPFLAGS_RESULT_VARIABLE)

  ### PERL_SITESEARCH
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:installsitesearch
    OUTPUT_VARIABLE
      PERL_SITESEARCH_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_SITESEARCH_RESULT_VARIABLE
  )
  if (NOT PERL_SITESEARCH_RESULT_VARIABLE)
    string(REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1" PERL_SITESEARCH ${PERL_SITESEARCH_OUTPUT_VARIABLE})
  endif (NOT PERL_SITESEARCH_RESULT_VARIABLE)

  ### PERL_SITELIB
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:installsitelib
    OUTPUT_VARIABLE
      PERL_SITELIB_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_SITELIB_RESULT_VARIABLE
  )
  if (NOT PERL_SITELIB_RESULT_VARIABLE)
    string(REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1" PERL_SITELIB ${PERL_SITELIB_OUTPUT_VARIABLE})
  endif (NOT PERL_SITELIB_RESULT_VARIABLE)

  ### PERL_VENDORARCH
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:installvendorarch
    OUTPUT_VARIABLE
      PERL_VENDORARCH_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_VENDORARCH_RESULT_VARIABLE
    )
  if (NOT PERL_VENDORARCH_RESULT_VARIABLE)
    string(REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1" PERL_VENDORARCH ${PERL_VENDORARCH_OUTPUT_VARIABLE})
  endif (NOT PERL_VENDORARCH_RESULT_VARIABLE)

  ### PERL_VENDORLIB
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:installvendorlib
    OUTPUT_VARIABLE
      PERL_VENDORLIB_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_VENDORLIB_RESULT_VARIABLE
  )
  if (NOT PERL_VENDORLIB_RESULT_VARIABLE)
    string(REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1" PERL_VENDORLIB ${PERL_VENDORLIB_OUTPUT_VARIABLE})
  endif (NOT PERL_VENDORLIB_RESULT_VARIABLE)

  ### PERL_ARCHLIB
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:installarchlib
      OUTPUT_VARIABLE
        PERL_ARCHLIB_OUTPUT_VARIABLE
      RESULT_VARIABLE
        PERL_ARCHLIB_RESULT_VARIABLE
  )
  if (NOT PERL_ARCHLIB_RESULT_VARIABLE)
    string(REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1" PERL_ARCHLIB ${PERL_ARCHLIB_OUTPUT_VARIABLE})
  endif (NOT PERL_ARCHLIB_RESULT_VARIABLE)

  ### PERL_PRIVLIB
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:installprivlib
    OUTPUT_VARIABLE
      PERL_PRIVLIB_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_PRIVLIB_RESULT_VARIABLE
  )
  if (NOT PERL_PRIVLIB_RESULT_VARIABLE)
    string(REGEX REPLACE "install[a-z]+='([^']+)'.*" "\\1" PERL_PRIVLIB ${PERL_PRIVLIB_OUTPUT_VARIABLE})
  endif (NOT PERL_PRIVLIB_RESULT_VARIABLE)


  ### PERL_POSSIBLE_INCLUDE_PATHS
  set(PERL_POSSIBLE_INCLUDE_PATHS
    ${PERL_ARCHLIB}/CORE
    /usr/lib/perl5/${PERL_VERSION}/${PERL_ARCHNAME}/CORE
    /usr/lib/perl/${PERL_VERSION}/${PERL_ARCHNAME}/CORE
    /usr/lib/perl5/${PERL_VERSION}/CORE
    /usr/lib/perl/${PERL_VERSION}/CORE
    )

  ### PERL_POSSIBLE_LIB_PATHS
  set(PERL_POSSIBLE_LIB_PATHS
    ${PERL_ARCHLIB}/CORE
    /usr/lib/perl5/${PERL_VERSION}/${PERL_ARCHNAME}/CORE
    /usr/lib/perl/${PERL_VERSION}/${PERL_ARCHNAME}/CORE
    /usr/lib/perl5/${PERL_VERSION}/CORE
    /usr/lib/perl/${PERL_VERSION}/CORE
  )

  ### PERL_POSSIBLE_LIBRARY_NAME
  execute_process(
    COMMAND
      ${PERL_EXECUTABLE} -V:libperl
    OUTPUT_VARIABLE
      PERL_LIBRARY_OUTPUT_VARIABLE
    RESULT_VARIABLE
      PERL_LIBRARY_RESULT_VARIABLE
  )
  if (NOT PERL_LIBRARY_RESULT_VARIABLE)
    foreach(_perl_lib_path ${PERL_POSSIBLE_LIB_PATHS})
      string(REGEX REPLACE "libperl='([^']+)'" "\\1" PERL_LIBRARY_OUTPUT_VARIABLE ${PERL_LIBRARY_OUTPUT_VARIABLE})
      set(PERL_POSSIBLE_LIBRARY_NAME ${PERL_POSSIBLE_LIBRARY_NAME} "${_perl_lib_path}/${PERL_LIBRARY_OUTPUT_VARIABLE}")
    endforeach(_perl_lib_path ${PERL_POSSIBLE_LIB_PATHS})
  endif (NOT PERL_LIBRARY_RESULT_VARIABLE)

  ### PERL_INCLUDE_PATH
  find_path(PERL_INCLUDE_PATH
    NAMES
      perl.h
    PATHS
      ${PERL_POSSIBLE_INCLUDE_PATHS}
  )
  
  ### PERL_LIBRARY
  find_library(PERL_LIBRARY
    NAMES
      ${PERL_POSSIBLE_LIBRARY_NAME}
      perl${PERL_VERSION}
      perl
    PATHS
      ${PERL_POSSIBLE_LIB_PATHS}
  )

endif (PERL_EXECUTABLE)

# handle the QUIETLY and REQUIRED arguments and set PERLLIBS_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PerlLibs DEFAULT_MSG PERL_LIBRARY PERL_INCLUDE_PATH)

# Introduced after CMake 2.6.4 to bring module into compliance
set(PERL_INCLUDE_DIR  ${PERL_INCLUDE_PATH})
set(PERL_INCLUDE_DIRS ${PERL_INCLUDE_PATH})
set(PERL_LIBRARIES    ${PERL_LIBRARY})

mark_as_advanced(
  PERL_INCLUDE_PATH
  PERL_EXECUTABLE
  PERL_LIBRARY
)
