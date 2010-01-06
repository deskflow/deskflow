# - Find LAPACK library
# This module finds an installed fortran library that implements the LAPACK
# linear-algebra interface (see http://www.netlib.org/lapack/).
#
# The approach follows that taken for the autoconf macro file, acx_lapack.m4
# (distributed at http://ac-archive.sourceforge.net/ac-archive/acx_lapack.html).
#
# This module sets the following variables:
#  LAPACK_FOUND - set to true if a library implementing the LAPACK interface
#    is found
#  LAPACK_LINKER_FLAGS - uncached list of required linker flags (excluding -l
#    and -L).
#  LAPACK_LIBRARIES - uncached list of libraries (using full path name) to
#    link against to use LAPACK
#  LAPACK95_LIBRARIES - uncached list of libraries (using full path name) to
#    link against to use LAPACK95
#  LAPACK95_FOUND - set to true if a library implementing the LAPACK f95
#    interface is found
#  BLA_STATIC  if set on this determines what kind of linkage we do (static)
#  BLA_VENDOR  if set checks only the specified vendor, if not set checks
#     all the possibilities
#  BLA_F95     if set on tries to find the f95 interfaces for BLAS/LAPACK
### List of vendors (BLA_VENDOR) valid in this module
##  Intel(mkl), ACML,Apple, NAS, Generic

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

get_property(_LANGUAGES_ GLOBAL PROPERTY ENABLED_LANGUAGES)
if(NOT _LANGUAGES_ MATCHES Fortran)
  if(LAPACK_FIND_REQUIRED)
    message(FATAL_ERROR
      "FindLAPACK is Fortran-only so Fortran must be enabled.")
  else(LAPACK_FIND_REQUIRED)
    message(STATUS "Looking for LAPACK... - NOT found (Fortran not enabled)")
    return()
  endif(LAPACK_FIND_REQUIRED)
endif(NOT _LANGUAGES_ MATCHES Fortran)

include(CheckFortranFunctionExists)
set(LAPACK_FOUND FALSE)
set(LAPACK95_FOUND FALSE)

macro(Check_Lapack_Libraries LIBRARIES _prefix _name _flags _list _blas _threads)
# This macro checks for the existence of the combination of fortran libraries
# given by _list.  If the combination is found, this macro checks (using the
# Check_Fortran_Function_Exists macro) whether can link against that library
# combination using the name of a routine given by _name using the linker
# flags given by _flags.  If the combination of libraries is found and passes
# the link test, LIBRARIES is set to the list of complete library paths that
# have been found.  Otherwise, LIBRARIES is set to FALSE.

# N.B. _prefix is the prefix applied to the names of all cached variables that
# are generated internally and marked advanced by this macro.

set(_libraries_work TRUE)
set(${LIBRARIES})
set(_combined_name)
foreach(_library ${_list})
  set(_combined_name ${_combined_name}_${_library})

  if(_libraries_work)
  IF (WIN32)
    if(BLA_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib;.dll")
    endif(BLA_STATIC)
    find_library(${_prefix}_${_library}_LIBRARY
    NAMES ${_library}
    PATHS ENV LIB
    )
  ENDIF (WIN32)

  if(APPLE)
    if(BLA_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so;.dylib")
    endif(BLA_STATIC)
    find_library(${_prefix}_${_library}_LIBRARY
    NAMES ${_library}
    PATHS /usr/local/lib /usr/lib /usr/local/lib64 /usr/lib64 ENV DYLD_LIBRARY_PATH
    )
    else(APPLE)
    if(BLA_STATIC)
     set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")
    endif(BLA_STATIC)
    find_library(${_prefix}_${_library}_LIBRARY
    NAMES ${_library}
    PATHS /usr/local/lib /usr/lib /usr/local/lib64 /usr/lib64 ENV LD_LIBRARY_PATH
    )
    endif(APPLE)

    mark_as_advanced(${_prefix}_${_library}_LIBRARY)
    set(${LIBRARIES} ${${LIBRARIES}} ${${_prefix}_${_library}_LIBRARY})
    set(_libraries_work ${${_prefix}_${_library}_LIBRARY})
  endif(_libraries_work)
endforeach(_library ${_list})

if(_libraries_work)
  # Test this combination of libraries.
  if(UNIX AND BLA_STATIC)
    set(CMAKE_REQUIRED_LIBRARIES ${_flags} "-Wl,--start-group ${${LIBRARIES}} ${_blas};-Wl,--end-group" ${_threads})
  else(UNIX AND BLA_STATIC)
    set(CMAKE_REQUIRED_LIBRARIES ${_flags} ${${LIBRARIES}} ${_blas} ${_threads})
  endif(UNIX AND BLA_STATIC)
#  message("DEBUG: CMAKE_REQUIRED_LIBRARIES = ${CMAKE_REQUIRED_LIBRARIES}")
  check_fortran_function_exists(${_name} ${_prefix}${_combined_name}_WORKS)
  set(CMAKE_REQUIRED_LIBRARIES)
  mark_as_advanced(${_prefix}${_combined_name}_WORKS)
  set(_libraries_work ${${_prefix}${_combined_name}_WORKS})
  #message("DEBUG: ${LIBRARIES} = ${${LIBRARIES}}")
endif(_libraries_work)

 if(_libraries_work)
   set(${LIBRARIES} ${${LIBRARIES}} ${_blas})
 else(_libraries_work)
    set(${LIBRARIES} FALSE)
 endif(_libraries_work)

endmacro(Check_Lapack_Libraries)


set(LAPACK_LINKER_FLAGS)
set(LAPACK_LIBRARIES)
set(LAPACK95_LIBRARIES)


if(LAPACK_FIND_QUIETLY OR NOT LAPACK_FIND_REQUIRED)
  find_package(BLAS)
else(LAPACK_FIND_QUIETLY OR NOT LAPACK_FIND_REQUIRED)
  find_package(BLAS REQUIRED)
endif(LAPACK_FIND_QUIETLY OR NOT LAPACK_FIND_REQUIRED)


if(BLAS_FOUND)
  set(LAPACK_LINKER_FLAGS ${BLAS_LINKER_FLAGS})
  if ($ENV{BLA_VENDOR} MATCHES ".+")
    set(BLA_VENDOR $ENV{BLA_VENDOR})
  else ($ENV{BLA_VENDOR} MATCHES ".+")
    if(NOT BLA_VENDOR)
      set(BLA_VENDOR "All")
    endif(NOT BLA_VENDOR)
  endif ($ENV{BLA_VENDOR} MATCHES ".+")
#acml lapack
 if (BLA_VENDOR STREQUAL "ACML" OR BLA_VENDOR STREQUAL "All")
  if(NOT LAPACK_LIBRARIES)
   check_lapack_libraries(
    LAPACK_LIBRARIES
    LAPACK
    cheev
    ""
    "acml"
    ""
    ""
    )
  endif(NOT LAPACK_LIBRARIES)
 endif (BLA_VENDOR STREQUAL "ACML" OR BLA_VENDOR STREQUAL "All")

# Apple LAPACK library?
if (BLA_VENDOR STREQUAL "Apple" OR BLA_VENDOR STREQUAL "All")
 if(NOT LAPACK_LIBRARIES)
  check_lapack_libraries(
  LAPACK_LIBRARIES
  LAPACK
  cheev
  ""
  "Accelerate"
  "${BLAS_LIBRARIES}"
  ""
  )
 endif(NOT LAPACK_LIBRARIES)
endif (BLA_VENDOR STREQUAL "Apple" OR BLA_VENDOR STREQUAL "All")
if (BLA_VENDOR STREQUAL "NAS" OR BLA_VENDOR STREQUAL "All")
  if ( NOT LAPACK_LIBRARIES )
    check_lapack_libraries(
    LAPACK_LIBRARIES
    LAPACK
    cheev
    ""
    "vecLib"
    "${BLAS_LIBRARIES}"
    ""
    )
  endif ( NOT LAPACK_LIBRARIES )
endif (BLA_VENDOR STREQUAL "NAS" OR BLA_VENDOR STREQUAL "All")
# Generic LAPACK library?
if (BLA_VENDOR STREQUAL "Generic" OR BLA_VENDOR STREQUAL "All")
  if ( NOT LAPACK_LIBRARIES )
    check_lapack_libraries(
    LAPACK_LIBRARIES
    LAPACK
    cheev
    ""
    "lapack"
    "${BLAS_LIBRARIES}"
    ""
    )
  endif ( NOT LAPACK_LIBRARIES )
endif (BLA_VENDOR STREQUAL "Generic" OR BLA_VENDOR STREQUAL "All")
#intel lapack
 if (BLA_VENDOR MATCHES "Intel*" OR BLA_VENDOR STREQUAL "All")
  if (_LANGUAGES_ MATCHES C OR _LANGUAGES_ MATCHES CXX)
   if(LAPACK_FIND_QUIETLY OR NOT LAPACK_FIND_REQUIRED)
      find_PACKAGE(Threads)
   else(LAPACK_FIND_QUIETLY OR NOT LAPACK_FIND_REQUIRED)
       find_package(Threads REQUIRED)
   endif(LAPACK_FIND_QUIETLY OR NOT LAPACK_FIND_REQUIRED)
   if (BLA_F95)
    if(NOT LAPACK95_LIBRARIES)
     check_lapack_libraries(
     LAPACK95_LIBRARIES
     LAPACK
     cheev
     ""
     "mkl_lapack95"
     "${BLAS95_LIBRARIES}"
     "${CMAKE_THREAD_LIBS_INIT}"
     )
    endif(NOT LAPACK95_LIBRARIES)
   else(BLA_F95)
    if(NOT LAPACK_LIBRARIES)
     check_lapack_libraries(
     LAPACK_LIBRARIES
     LAPACK
     cheev
     ""
     "mkl_lapack"
     "${BLAS_LIBRARIES}"
     "${CMAKE_THREAD_LIBS_INIT}"
     )
    endif(NOT LAPACK_LIBRARIES)
   endif(BLA_F95)
  endif (_LANGUAGES_ MATCHES C OR _LANGUAGES_ MATCHES CXX)
 endif(BLA_VENDOR MATCHES "Intel*" OR BLA_VENDOR STREQUAL "All")
else(BLAS_FOUND)
  message(STATUS "LAPACK requires BLAS")
endif(BLAS_FOUND)

if(BLA_F95)
 if(LAPACK95_LIBRARIES)
  set(LAPACK95_FOUND TRUE)
 else(LAPACK95_LIBRARIES)
  set(LAPACK95_FOUND FALSE)
 endif(LAPACK95_LIBRARIES)
 if(NOT LAPACK_FIND_QUIETLY)
  if(LAPACK95_FOUND)
    message(STATUS "A library with LAPACK95 API found.")
  else(LAPACK95_FOUND)
    if(LAPACK_FIND_REQUIRED)
      message(FATAL_ERROR
      "A required library with LAPACK95 API not found. Please specify library location."
      )
    else(LAPACK_FIND_REQUIRED)
      message(STATUS
      "A library with LAPACK95 API not found. Please specify library location."
      )
    endif(LAPACK_FIND_REQUIRED)
  endif(LAPACK95_FOUND)
 endif(NOT LAPACK_FIND_QUIETLY)
 set(LAPACK_FOUND "${LAPACK95_FOUND}")
 set(LAPACK_LIBRARIES "${LAPACK95_LIBRARIES}")
else(BLA_F95)
 if(LAPACK_LIBRARIES)
  set(LAPACK_FOUND TRUE)
 else(LAPACK_LIBRARIES)
  set(LAPACK_FOUND FALSE)
 endif(LAPACK_LIBRARIES)

 if(NOT LAPACK_FIND_QUIETLY)
  if(LAPACK_FOUND)
    message(STATUS "A library with LAPACK API found.")
  else(LAPACK_FOUND)
    if(LAPACK_FIND_REQUIRED)
      message(FATAL_ERROR
      "A required library with LAPACK API not found. Please specify library location."
      )
    else(LAPACK_FIND_REQUIRED)
      message(STATUS
      "A library with LAPACK API not found. Please specify library location."
      )
    endif(LAPACK_FIND_REQUIRED)
  endif(LAPACK_FOUND)
 endif(NOT LAPACK_FIND_QUIETLY)
endif(BLA_F95)
