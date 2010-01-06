
#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
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

# This module is shared by multiple languages; use include blocker.
if(__WINDOWS_BORLAND)
  return()
endif()
set(__WINDOWS_BORLAND 1)

SET(BORLAND 1)

# Borland target type flags (bcc32 -h -t):
#  -tW     GUI App         (implies -U__CONSOLE__)
#  -tWC    Console App     (implies -D__CONSOLE__=1)
#  -tWD    Build a DLL     (implies -D__DLL__=1 -D_DLL=1)
#  -tWM    Enable threads  (implies -D__MT__=1 -D_MT=1)
#  -tWR    Use DLL runtime (implies -D_RTLDLL, and '-tW' too!!)
#
# Notes:
#  - The flags affect linking so we pass them to the linker.
#  - The flags affect preprocessing so we pass them to the compiler.
#  - Since '-tWR' implies '-tW' we use '-tWR -tW-' instead.
#  - Since '-tW-' disables '-tWD' we use '-tWR -tW- -tWD' for DLLs.
set(_RTLDLL "-tWR -tW-")
set(_COMPILE_C "-c")
set(_COMPILE_CXX "-P -c")

SET(CMAKE_LIBRARY_PATH_FLAG "-L")
SET(CMAKE_LINK_LIBRARY_FLAG "")

SET(CMAKE_FIND_LIBRARY_SUFFIXES "-bcc.lib" ".lib")

# uncomment these out to debug makefiles
#SET(CMAKE_START_TEMP_FILE "")
#SET(CMAKE_END_TEMP_FILE "")
#SET(CMAKE_VERBOSE_MAKEFILE 1)

# Borland cannot handle + in the file name, so mangle object file name
SET (CMAKE_MANGLE_OBJECT_FILE_NAMES "ON")

# extra flags for a win32 exe
SET(CMAKE_CREATE_WIN32_EXE "-tW" )
# extra flags for a console app
SET(CMAKE_CREATE_CONSOLE_EXE "-tWC" )

SET (CMAKE_BUILD_TYPE Debug CACHE STRING
     "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel.")

SET (CMAKE_EXE_LINKER_FLAGS_INIT "-tWM -lS:10000000 -lSc:10000000 ")
SET (CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "-v")
SET (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT "-v")
SET (CMAKE_SHARED_LINKER_FLAGS_INIT ${CMAKE_EXE_LINKER_FLAGS_INIT})
SET (CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT ${CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT})
SET (CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT})
SET (CMAKE_MODULE_LINKER_FLAGS_INIT ${CMAKE_SHARED_LINKER_FLAGS_INIT})
SET (CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT ${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT})
SET (CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT})

macro(__borland_language lang)
  set(CMAKE_SHARED_LIBRARY_${lang}_FLAGS "-tWD")

  # compile a source file into an object file
  # place <DEFINES> outside the response file because Borland refuses
  # to parse quotes from the response file.
  set(CMAKE_${lang}_COMPILE_OBJECT
    "<CMAKE_${lang}_COMPILER> ${_RTLDLL} <DEFINES> ${CMAKE_START_TEMP_FILE}-DWIN32 -o<OBJECT> <FLAGS> ${_COMPILE_${lang}} <SOURCE>${CMAKE_END_TEMP_FILE}"
    )

  set(CMAKE_${lang}_LINK_EXECUTABLE
    "<CMAKE_${lang}_COMPILER> ${_RTLDLL} -e<TARGET> ${CMAKE_START_TEMP_FILE}<LINK_FLAGS> <FLAGS> <LINK_LIBRARIES> <OBJECTS>${CMAKE_END_TEMP_FILE}"
    # "implib -c -w <TARGET_IMPLIB> <TARGET>"
    )

  # place <DEFINES> outside the response file because Borland refuses
  # to parse quotes from the response file.
  set(CMAKE_${lang}_CREATE_PREPROCESSED_SOURCE
    "cpp32 <DEFINES> ${CMAKE_START_TEMP_FILE}-DWIN32 <FLAGS> -o<PREPROCESSED_SOURCE> ${_COMPILE_${lang}} <SOURCE>${CMAKE_END_TEMP_FILE}"
    )
  # Borland >= 5.6 allows -P option for cpp32, <= 5.5 does not

  # Create a module library.
  set(CMAKE_${lang}_CREATE_SHARED_MODULE
    "<CMAKE_${lang}_COMPILER> ${_RTLDLL} -tWD ${CMAKE_START_TEMP_FILE}-e<TARGET> <LINK_FLAGS> <LINK_LIBRARIES> <OBJECTS>${CMAKE_END_TEMP_FILE}"
    )

  # Create an import library for another target.
  set(CMAKE_${lang}_CREATE_IMPORT_LIBRARY
    "implib -c -w <TARGET_IMPLIB> <TARGET>"
    )

  # Create a shared library.
  # First create a module and then its import library.
  set(CMAKE_${lang}_CREATE_SHARED_LIBRARY
    ${CMAKE_${lang}_CREATE_SHARED_MODULE}
    ${CMAKE_${lang}_CREATE_IMPORT_LIBRARY}
    )

  # create a static library
  set(CMAKE_${lang}_CREATE_STATIC_LIBRARY
    "tlib ${CMAKE_START_TEMP_FILE}/p512 <LINK_FLAGS> /a <TARGET_QUOTED> <OBJECTS>${CMAKE_END_TEMP_FILE}"
    )

  # Initial configuration flags.
  set(CMAKE_${lang}_FLAGS_INIT "-tWM")
  set(CMAKE_${lang}_FLAGS_DEBUG_INIT "-Od -v")
  set(CMAKE_${lang}_FLAGS_MINSIZEREL_INIT "-O1 -DNDEBUG")
  set(CMAKE_${lang}_FLAGS_RELEASE_INIT "-O2 -DNDEBUG")
  set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT "-Od")
  set(CMAKE_${lang}_STANDARD_LIBRARIES_INIT "import32.lib")
endmacro()
