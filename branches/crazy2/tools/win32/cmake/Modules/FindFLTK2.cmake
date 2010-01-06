# - Find the native FLTK2 includes and library
# The following settings are defined
#  FLTK2_FLUID_EXECUTABLE, where to find the Fluid tool
#  FLTK2_WRAP_UI, This enables the FLTK2_WRAP_UI command
#  FLTK2_INCLUDE_DIR, where to find include files
#  FLTK2_LIBRARIES, list of fltk2 libraries
#  FLTK2_FOUND, Don't use FLTK2 if false.
# The following settings should not be used in general.
#  FLTK2_BASE_LIBRARY   = the full path to fltk2.lib
#  FLTK2_GL_LIBRARY     = the full path to fltk2_gl.lib
#  FLTK2_IMAGES_LIBRARY = the full path to fltk2_images.lib

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

SET (FLTK2_DIR $ENV{FLTK2_DIR} )

#  Platform dependent libraries required by FLTK2
IF(WIN32)
  IF(NOT CYGWIN)
    IF(BORLAND)
      SET( FLTK2_PLATFORM_DEPENDENT_LIBS import32 )
    ELSE(BORLAND)
      SET( FLTK2_PLATFORM_DEPENDENT_LIBS wsock32 comctl32 )
    ENDIF(BORLAND)
  ENDIF(NOT CYGWIN)
ENDIF(WIN32)

IF(UNIX)
  INCLUDE(${CMAKE_ROOT}/Modules/FindX11.cmake)
  SET( FLTK2_PLATFORM_DEPENDENT_LIBS ${X11_LIBRARIES} -lm)
ENDIF(UNIX)

IF(APPLE)
  SET( FLTK2_PLATFORM_DEPENDENT_LIBS  "-framework Carbon -framework Cocoa -framework ApplicationServices -lz")
ENDIF(APPLE)

IF(CYGWIN)
  SET( FLTK2_PLATFORM_DEPENDENT_LIBS ole32 uuid comctl32 wsock32 supc++ -lm -lgdi32)
ENDIF(CYGWIN)

# If FLTK2_INCLUDE_DIR is already defined we assigne its value to FLTK2_DIR
IF(FLTK2_INCLUDE_DIR)
  SET(FLTK2_DIR ${FLTK2_INCLUDE_DIR})
ELSE(FLTK2_INCLUDE_DIR)
  SET(FLTK2_INCLUDE_DIR ${FLTK2_DIR})
ENDIF(FLTK2_INCLUDE_DIR)


# If FLTK2 has been built using CMake we try to find everything directly
SET(FLTK2_DIR_STRING "directory containing FLTK2Config.cmake.  This is either the root of the build tree, or PREFIX/lib/fltk for an installation.")

# Search only if the location is not already known.
IF(NOT FLTK2_DIR)
  # Get the system search path as a list.
  IF(UNIX)
    STRING(REGEX MATCHALL "[^:]+" FLTK2_DIR_SEARCH1 "$ENV{PATH}")
  ELSE(UNIX)
    STRING(REGEX REPLACE "\\\\" "/" FLTK2_DIR_SEARCH1 "$ENV{PATH}")
  ENDIF(UNIX)
  STRING(REGEX REPLACE "/;" ";" FLTK2_DIR_SEARCH2 ${FLTK2_DIR_SEARCH1})

  # Construct a set of paths relative to the system search path.
  SET(FLTK2_DIR_SEARCH "")
  FOREACH(dir ${FLTK2_DIR_SEARCH2})
    SET(FLTK2_DIR_SEARCH ${FLTK2_DIR_SEARCH} "${dir}/../lib/fltk")
  ENDFOREACH(dir)

  #
  # Look for an installation or build tree.
  #
  FIND_PATH(FLTK2_DIR FLTK2Config.cmake
    # Look for an environment variable FLTK2_DIR.
    $ENV{FLTK2_DIR}

    # Look in places relative to the system executable search path.
    ${FLTK2_DIR_SEARCH}

    # Look in standard UNIX install locations.
    /usr/local/lib/fltk2
    /usr/lib/fltk2
    /usr/local/include
    /usr/include
    /usr/local/fltk2
    /usr/X11R6/include

    # Read from the CMakeSetup registry entries.  It is likely that
    # FLTK2 will have been recently built.
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild1]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild2]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild3]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild4]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild5]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild6]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild7]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild8]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild9]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild10]

    # Help the user find it if we cannot.
    DOC "The ${FLTK2_DIR_STRING}"
    )

  IF(NOT FLTK2_DIR)
    FIND_PATH(FLTK2_DIR fltk/run.h ${FLTK2_INCLUDE_SEARCH_PATH})
  ENDIF(NOT FLTK2_DIR)

ENDIF(NOT FLTK2_DIR)


# If FLTK2 was found, load the configuration file to get the rest of the
# settings.
IF(FLTK2_DIR)

  # Check if FLTK2 was built using CMake
  IF(EXISTS ${FLTK2_DIR}/FLTK2Config.cmake)
    SET(FLTK2_BUILT_WITH_CMAKE 1)
  ENDIF(EXISTS ${FLTK2_DIR}/FLTK2Config.cmake)

  IF(FLTK2_BUILT_WITH_CMAKE)
    SET(FLTK2_FOUND 1)
    INCLUDE(${FLTK2_DIR}/FLTK2Config.cmake)

    # Fluid
    IF(FLUID_COMMAND) 
      SET(FLTK2_FLUID_EXECUTABLE ${FLUID_COMMAND} CACHE FILEPATH "Fluid executable")
    ELSE(FLUID_COMMAND) 
      FIND_PROGRAM(FLTK2_FLUID_EXECUTABLE fluid2 PATHS 
        ${FLTK2_EXECUTABLE_DIRS}
        ${FLTK2_EXECUTABLE_DIRS}/RelWithDebInfo
        ${FLTK2_EXECUTABLE_DIRS}/Debug
        ${FLTK2_EXECUTABLE_DIRS}/Release
        NO_SYSTEM_PATH)
    ENDIF(FLUID_COMMAND)

    MARK_AS_ADVANCED(FLTK2_FLUID_EXECUTABLE)
    SET( FLTK_FLUID_EXECUTABLE ${FLTK2_FLUID_EXECUTABLE} )

    


    SET(FLTK2_INCLUDE_DIR ${FLTK2_DIR})
    LINK_DIRECTORIES(${FLTK2_LIBRARY_DIRS})

    SET(FLTK2_BASE_LIBRARY fltk2)
    SET(FLTK2_GL_LIBRARY fltk2_gl)
    SET(FLTK2_IMAGES_LIBRARY fltk2_images)

    # Add the extra libraries
    LOAD_CACHE(${FLTK2_DIR}
      READ_WITH_PREFIX
      FL FLTK2_USE_SYSTEM_JPEG
      FL FLTK2_USE_SYSTEM_PNG
      FL FLTK2_USE_SYSTEM_ZLIB
      )

    SET(FLTK2_IMAGES_LIBS "")
    IF(FLFLTK2_USE_SYSTEM_JPEG)
      SET(FLTK2_IMAGES_LIBS ${FLTK2_IMAGES_LIBS} fltk2_jpeg)
    ENDIF(FLFLTK2_USE_SYSTEM_JPEG)
    IF(FLFLTK2_USE_SYSTEM_PNG)
      SET(FLTK2_IMAGES_LIBS ${FLTK2_IMAGES_LIBS} fltk2_png)
    ENDIF(FLFLTK2_USE_SYSTEM_PNG)
    IF(FLFLTK2_USE_SYSTEM_ZLIB)
      SET(FLTK2_IMAGES_LIBS ${FLTK2_IMAGES_LIBS} fltk2_zlib)
    ENDIF(FLFLTK2_USE_SYSTEM_ZLIB)
    SET(FLTK2_IMAGES_LIBS "${FLTK2_IMAGES_LIBS}" CACHE INTERNAL
      "Extra libraries for fltk2_images library.")

  ELSE(FLTK2_BUILT_WITH_CMAKE)

    # if FLTK2 was not built using CMake
    # Find fluid executable.
    FIND_PROGRAM(FLTK2_FLUID_EXECUTABLE fluid2 ${FLTK2_INCLUDE_DIR}/fluid)

    # Use location of fluid to help find everything else.
    SET(FLTK2_INCLUDE_SEARCH_PATH "")
    SET(FLTK2_LIBRARY_SEARCH_PATH "")
    IF(FLTK2_FLUID_EXECUTABLE)
      SET( FLTK_FLUID_EXECUTABLE ${FLTK2_FLUID_EXECUTABLE} )
      GET_FILENAME_COMPONENT(FLTK2_BIN_DIR "${FLTK2_FLUID_EXECUTABLE}" PATH)
      SET(FLTK2_INCLUDE_SEARCH_PATH ${FLTK2_INCLUDE_SEARCH_PATH}
        ${FLTK2_BIN_DIR}/../include ${FLTK2_BIN_DIR}/..)
      SET(FLTK2_LIBRARY_SEARCH_PATH ${FLTK2_LIBRARY_SEARCH_PATH}
        ${FLTK2_BIN_DIR}/../lib)
      SET(FLTK2_WRAP_UI 1)
    ENDIF(FLTK2_FLUID_EXECUTABLE)

    SET(FLTK2_INCLUDE_SEARCH_PATH ${FLTK2_INCLUDE_SEARCH_PATH}
      /usr/local/include
      /usr/include
      /usr/local/fltk2
      /usr/X11R6/include
      )

    FIND_PATH(FLTK2_INCLUDE_DIR fltk/run.h ${FLTK2_INCLUDE_SEARCH_PATH})

    SET(FLTK2_LIBRARY_SEARCH_PATH ${FLTK2_LIBRARY_SEARCH_PATH}
      /usr/lib
      /usr/local/lib
      /usr/local/fltk2/lib
      /usr/X11R6/lib
      ${FLTK2_INCLUDE_DIR}/lib
      )

    FIND_LIBRARY(FLTK2_BASE_LIBRARY NAMES fltk2
      PATHS ${FLTK2_LIBRARY_SEARCH_PATH})
    FIND_LIBRARY(FLTK2_GL_LIBRARY NAMES fltk2_gl 
      PATHS ${FLTK2_LIBRARY_SEARCH_PATH})
    FIND_LIBRARY(FLTK2_IMAGES_LIBRARY NAMES fltk2_images
      PATHS ${FLTK2_LIBRARY_SEARCH_PATH})

    # Find the extra libraries needed for the fltk_images library.
    IF(UNIX)
      FIND_PROGRAM(FLTK2_CONFIG_SCRIPT fltk2-config PATHS ${FLTK2_BIN_DIR})
      IF(FLTK2_CONFIG_SCRIPT)
        EXEC_PROGRAM(${FLTK2_CONFIG_SCRIPT} ARGS --use-images --ldflags
          OUTPUT_VARIABLE FLTK2_IMAGES_LDFLAGS)
        SET(FLTK2_LIBS_EXTRACT_REGEX ".*-lfltk2_images (.*) -lfltk2.*")
        IF("${FLTK2_IMAGES_LDFLAGS}" MATCHES "${FLTK2_LIBS_EXTRACT_REGEX}")
          STRING(REGEX REPLACE "${FLTK2_LIBS_EXTRACT_REGEX}" "\\1"
            FLTK2_IMAGES_LIBS "${FLTK2_IMAGES_LDFLAGS}")
          STRING(REGEX REPLACE " +" ";" FLTK2_IMAGES_LIBS "${FLTK2_IMAGES_LIBS}")
          # The EXEC_PROGRAM will not be inherited into subdirectories from
          # the file that originally included this module.  Save the answer.
          SET(FLTK2_IMAGES_LIBS "${FLTK2_IMAGES_LIBS}" CACHE INTERNAL
            "Extra libraries for fltk_images library.")
        ENDIF("${FLTK2_IMAGES_LDFLAGS}" MATCHES "${FLTK2_LIBS_EXTRACT_REGEX}")
      ENDIF(FLTK2_CONFIG_SCRIPT)
    ENDIF(UNIX)

  ENDIF(FLTK2_BUILT_WITH_CMAKE)
ENDIF(FLTK2_DIR)


SET(FLTK2_FOUND 1)
FOREACH(var FLTK2_FLUID_EXECUTABLE FLTK2_INCLUDE_DIR
    FLTK2_BASE_LIBRARY FLTK2_GL_LIBRARY
    FLTK2_IMAGES_LIBRARY)
  IF(NOT ${var})
    MESSAGE( STATUS "${var} not found" )
    SET(FLTK2_FOUND 0)
  ENDIF(NOT ${var})
ENDFOREACH(var)


IF(FLTK2_FOUND)
  SET(FLTK2_LIBRARIES ${FLTK2_IMAGES_LIBRARY} ${FLTK2_IMAGES_LIBS} ${FLTK2_BASE_LIBRARY} ${FLTK2_GL_LIBRARY} )
  IF(APPLE)
    SET(FLTK2_LIBRARIES ${FLTK2_PLATFORM_DEPENDENT_LIBS} ${FLTK2_LIBRARIES})
  ELSE(APPLE)
    SET(FLTK2_LIBRARIES ${FLTK2_LIBRARIES} ${FLTK2_PLATFORM_DEPENDENT_LIBS})
  ENDIF(APPLE)

  # The following deprecated settings are for compatibility with CMake 1.4
  SET (HAS_FLTK2 ${FLTK2_FOUND})
  SET (FLTK2_INCLUDE_PATH ${FLTK2_INCLUDE_DIR})
  SET (FLTK2_FLUID_EXE ${FLTK2_FLUID_EXECUTABLE})
  SET (FLTK2_LIBRARY ${FLTK2_LIBRARIES})
ELSE(FLTK2_FOUND)
  # make FIND_PACKAGE friendly
  IF(NOT FLTK2_FIND_QUIETLY)
    IF(FLTK2_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR
              "FLTK2 required, please specify its location with FLTK2_DIR.")
    ELSE(FLTK2_FIND_REQUIRED)
      MESSAGE(STATUS "FLTK2 was not found.")
    ENDIF(FLTK2_FIND_REQUIRED)
  ENDIF(NOT FLTK2_FIND_QUIETLY)
ENDIF(FLTK2_FOUND)

