#=============================================================================
# Copyright 2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

configure_file(${FortranCInterface_SOURCE_DIR}/Input.cmake.in
               ${FortranCInterface_BINARY_DIR}/Input.cmake @ONLY)

# Detect the Fortran/C interface on the first run or when the
# configuration changes.
if(${FortranCInterface_BINARY_DIR}/Input.cmake
    IS_NEWER_THAN ${FortranCInterface_BINARY_DIR}/Output.cmake
    OR ${FortranCInterface_SOURCE_DIR}/Output.cmake.in
    IS_NEWER_THAN ${FortranCInterface_BINARY_DIR}/Output.cmake
    OR ${FortranCInterface_SOURCE_DIR}/CMakeLists.txt
    IS_NEWER_THAN ${FortranCInterface_BINARY_DIR}/Output.cmake
    OR ${CMAKE_CURRENT_LIST_FILE}
    IS_NEWER_THAN ${FortranCInterface_BINARY_DIR}/Output.cmake
    )
  message(STATUS "Detecting Fortran/C Interface")
else()
  return()
endif()

# Invalidate verification results.
unset(FortranCInterface_VERIFIED_C CACHE)
unset(FortranCInterface_VERIFIED_CXX CACHE)

set(_result)

# Build a sample project which reports symbols.
try_compile(FortranCInterface_COMPILED
  ${FortranCInterface_BINARY_DIR}
  ${FortranCInterface_SOURCE_DIR}
  FortranCInterface
  CMAKE_FLAGS
    "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
    "-DCMAKE_Fortran_FLAGS:STRING=${CMAKE_Fortran_FLAGS}"
  OUTPUT_VARIABLE FortranCInterface_OUTPUT)
set(FortranCInterface_COMPILED ${FortranCInterface_COMPILED})
unset(FortranCInterface_COMPILED CACHE)

# Locate the sample project executable.
if(FortranCInterface_COMPILED)
  find_program(FortranCInterface_EXE
    NAMES FortranCInterface
    PATHS ${FortranCInterface_BINARY_DIR} ${FortranCInterface_BINARY_DIR}/Debug
    NO_DEFAULT_PATH
    )
  set(FortranCInterface_EXE ${FortranCInterface_EXE})
  unset(FortranCInterface_EXE CACHE)
else()
  set(_result "Failed to compile")
  set(FortranCInterface_EXE)
  file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Fortran/C interface test project failed with the following output:\n"
    "${FortranCInterface_OUTPUT}\n")
endif()

# Load symbols from INFO:symbol[] strings in the executable.
set(FortranCInterface_SYMBOLS)
if(FortranCInterface_EXE)
  file(STRINGS "${FortranCInterface_EXE}" _info_strings
    LIMIT_COUNT 8 REGEX "INFO:[^[]*\\[")
  foreach(info ${_info_strings})
    if("${info}" MATCHES ".*INFO:symbol\\[([^]]*)\\].*")
      string(REGEX REPLACE ".*INFO:symbol\\[([^]]*)\\].*" "\\1" symbol "${info}")
      list(APPEND FortranCInterface_SYMBOLS ${symbol})
    endif()
  endforeach()
elseif(NOT _result)
  set(_result "Failed to load sample executable")
endif()

set(_case_mysub "LOWER")
set(_case_my_sub "LOWER")
set(_case_MYSUB "UPPER")
set(_case_MY_SUB "UPPER")
set(_global_regex  "^(_*)(mysub|MYSUB)([_$]*)$")
set(_global__regex "^(_*)(my_sub|MY_SUB)([_$]*)$")
set(_module_regex  "^(_*)(mymodule|MYMODULE)([A-Za-z_$]*)(mysub|MYSUB)([_$]*)$")
set(_module__regex "^(_*)(my_module|MY_MODULE)([A-Za-z_$]*)(my_sub|MY_SUB)([_$]*)$")

# Parse the symbol names.
foreach(symbol ${FortranCInterface_SYMBOLS})
  foreach(form "" "_")
    # Look for global symbols.
    string(REGEX REPLACE "${_global_${form}regex}"
                         "\\1;\\2;\\3" pieces "${symbol}")
    list(LENGTH pieces len)
    if(len EQUAL 3)
      set(FortranCInterface_GLOBAL_${form}SYMBOL "${symbol}")
      list(GET pieces 0 FortranCInterface_GLOBAL_${form}PREFIX)
      list(GET pieces 1 name)
      list(GET pieces 2 FortranCInterface_GLOBAL_${form}SUFFIX)
      set(FortranCInterface_GLOBAL_${form}CASE "${_case_${name}}")
    endif()

    # Look for module symbols.
    string(REGEX REPLACE "${_module_${form}regex}"
                         "\\1;\\2;\\3;\\4;\\5" pieces "${symbol}")
    list(LENGTH pieces len)
    if(len EQUAL 5)
      set(FortranCInterface_MODULE_${form}SYMBOL "${symbol}")
      list(GET pieces 0 FortranCInterface_MODULE_${form}PREFIX)
      list(GET pieces 1 module)
      list(GET pieces 2 FortranCInterface_MODULE_${form}MIDDLE)
      list(GET pieces 3 name)
      list(GET pieces 4 FortranCInterface_MODULE_${form}SUFFIX)
      set(FortranCInterface_MODULE_${form}CASE "${_case_${name}}")
    endif()
  endforeach()
endforeach()

# Construct mangling macro definitions.
set(_name_LOWER "name")
set(_name_UPPER "NAME")
foreach(form "" "_")
  if(FortranCInterface_GLOBAL_${form}SYMBOL)
    if(FortranCInterface_GLOBAL_${form}PREFIX)
      set(_prefix "${FortranCInterface_GLOBAL_${form}PREFIX}##")
    else()
      set(_prefix "")
    endif()
    if(FortranCInterface_GLOBAL_${form}SUFFIX)
      set(_suffix "##${FortranCInterface_GLOBAL_${form}SUFFIX}")
    else()
      set(_suffix "")
    endif()
    set(_name "${_name_${FortranCInterface_GLOBAL_${form}CASE}}")
    set(FortranCInterface_GLOBAL${form}_MACRO
      "(name,NAME) ${_prefix}${_name}${_suffix}")
  endif()
  if(FortranCInterface_MODULE_${form}SYMBOL)
    if(FortranCInterface_MODULE_${form}PREFIX)
      set(_prefix "${FortranCInterface_MODULE_${form}PREFIX}##")
    else()
      set(_prefix "")
    endif()
    if(FortranCInterface_MODULE_${form}SUFFIX)
      set(_suffix "##${FortranCInterface_MODULE_${form}SUFFIX}")
    else()
      set(_suffix "")
    endif()
    set(_name "${_name_${FortranCInterface_MODULE_${form}CASE}}")
    set(_middle "##${FortranCInterface_MODULE_${form}MIDDLE}##")
    set(FortranCInterface_MODULE${form}_MACRO
      "(mod_name,name, mod_NAME,NAME) ${_prefix}mod_${_name}${_middle}${_name}${_suffix}")
  endif()
endforeach()

# Summarize what is available.
foreach(scope GLOBAL MODULE)
  if(FortranCInterface_${scope}_SYMBOL AND
      FortranCInterface_${scope}__SYMBOL)
    set(FortranCInterface_${scope}_FOUND 1)
  else()
    set(FortranCInterface_${scope}_FOUND 0)
  endif()
endforeach()

# Record the detection results.
configure_file(${FortranCInterface_SOURCE_DIR}/Output.cmake.in
               ${FortranCInterface_BINARY_DIR}/Output.cmake @ONLY)
file(APPEND ${FortranCInterface_BINARY_DIR}/Output.cmake "\n")

# Report the results.
if(FortranCInterface_GLOBAL_FOUND)
  if(FortranCInterface_MODULE_FOUND)
    set(_result "Found GLOBAL and MODULE mangling")
  else(FortranCInterface_MODULE_FOUND)
    set(_result "Found GLOBAL but not MODULE mangling")
  endif()
elseif(NOT _result)
  set(_result "Failed to recognize symbols")
endif()
message(STATUS "Detecting Fortran/C Interface - ${_result}")
