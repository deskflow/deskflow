# GetPrerequisites.cmake
#
# This script provides functions to list the .dll, .dylib or .so files that an
# executable or shared library file depends on. (Its prerequisites.)
#
# It uses various tools to obtain the list of required shared library files:
#   dumpbin (Windows)
#   ldd (Linux/Unix)
#   otool (Mac OSX)
#
# The following functions are provided by this script:
#   gp_append_unique
#   is_file_executable
#   gp_item_default_embedded_path
#     (projects can override with gp_item_default_embedded_path_override)
#   gp_resolve_item
#     (projects can override with gp_resolve_item_override)
#   gp_resolved_file_type
#   gp_file_type
#   get_prerequisites
#   list_prerequisites
#   list_prerequisites_by_glob
#
# Requires CMake 2.6 or greater because it uses function, break, return and
# PARENT_SCOPE.

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

# gp_append_unique list_var value
#
# Append value to the list variable ${list_var} only if the value is not
# already in the list.
#
function(gp_append_unique list_var value)
  set(contains 0)

  foreach(item ${${list_var}})
    if("${item}" STREQUAL "${value}")
      set(contains 1)
      break()
    endif("${item}" STREQUAL "${value}")
  endforeach(item)

  if(NOT contains)
    set(${list_var} ${${list_var}} "${value}" PARENT_SCOPE)
  endif(NOT contains)
endfunction(gp_append_unique)


# is_file_executable file result_var
#
# Return 1 in ${result_var} if ${file} is a binary executable.
#
# Return 0 in ${result_var} otherwise.
#
function(is_file_executable file result_var)
  #
  # A file is not executable until proven otherwise:
  #
  set(${result_var} 0 PARENT_SCOPE)

  get_filename_component(file_full "${file}" ABSOLUTE)
  string(TOLOWER "${file_full}" file_full_lower)

  # If file name ends in .exe on Windows, *assume* executable:
  #
  if(WIN32)
    if("${file_full_lower}" MATCHES "\\.exe$")
      set(${result_var} 1 PARENT_SCOPE)
      return()
    endif("${file_full_lower}" MATCHES "\\.exe$")

    # A clause could be added here that uses output or return value of dumpbin
    # to determine ${result_var}. In 99%+? practical cases, the exe name
    # match will be sufficient...
    #
  endif(WIN32)

  # Use the information returned from the Unix shell command "file" to
  # determine if ${file_full} should be considered an executable file...
  #
  # If the file command's output contains "executable" and does *not* contain
  # "text" then it is likely an executable suitable for prerequisite analysis
  # via the get_prerequisites macro.
  #
  if(UNIX)
    if(NOT file_cmd)
      find_program(file_cmd "file")
    endif(NOT file_cmd)

    if(file_cmd)
      execute_process(COMMAND "${file_cmd}" "${file_full}"
        OUTPUT_VARIABLE file_ov
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

      # Replace the name of the file in the output with a placeholder token
      # (the string " _file_full_ ") so that just in case the path name of
      # the file contains the word "text" or "executable" we are not fooled
      # into thinking "the wrong thing" because the file name matches the
      # other 'file' command output we are looking for...
      #
      string(REPLACE "${file_full}" " _file_full_ " file_ov "${file_ov}")
      string(TOLOWER "${file_ov}" file_ov)

      #message(STATUS "file_ov='${file_ov}'")
      if("${file_ov}" MATCHES "executable")
        #message(STATUS "executable!")
        if("${file_ov}" MATCHES "text")
          #message(STATUS "but text, so *not* a binary executable!")
        else("${file_ov}" MATCHES "text")
          set(${result_var} 1 PARENT_SCOPE)
          return()
        endif("${file_ov}" MATCHES "text")
      endif("${file_ov}" MATCHES "executable")
    else(file_cmd)
      message(STATUS "warning: No 'file' command, skipping execute_process...")
    endif(file_cmd)
  endif(UNIX)
endfunction(is_file_executable)


# gp_item_default_embedded_path item default_embedded_path_var
#
# Return the path that others should refer to the item by when the item
# is embedded inside a bundle.
#
# Override on a per-project basis by providing a project-specific
# gp_item_default_embedded_path_override function.
#
function(gp_item_default_embedded_path item default_embedded_path_var)

  # On Windows and Linux, "embed" prerequisites in the same directory
  # as the executable by default:
  #
  set(path "@executable_path")
  set(overridden 0)

  # On the Mac, relative to the executable depending on the type
  # of the thing we are embedding:
  #
  if(APPLE)
    #
    # The assumption here is that all executables in the bundle will be
    # in same-level-directories inside the bundle. The parent directory
    # of an executable inside the bundle should be MacOS or a sibling of
    # MacOS and all embedded paths returned from here will begin with
    # "@executable_path/../" and will work from all executables in all
    # such same-level-directories inside the bundle.
    #

    # By default, embed things right next to the main bundle executable:
    #
    set(path "@executable_path/../../Contents/MacOS")

    # Embed .dylibs right next to the main bundle executable:
    #
    if(item MATCHES "\\.dylib$")
      set(path "@executable_path/../MacOS")
      set(overridden 1)
    endif(item MATCHES "\\.dylib$")

    # Embed frameworks in the embedded "Frameworks" directory (sibling of MacOS):
    #
    if(NOT overridden)
      if(item MATCHES "[^/]+\\.framework/")
        set(path "@executable_path/../Frameworks")
        set(overridden 1)
      endif(item MATCHES "[^/]+\\.framework/")
    endif(NOT overridden)
  endif()

  # Provide a hook so that projects can override the default embedded location
  # of any given library by whatever logic they choose:
  #
  if(COMMAND gp_item_default_embedded_path_override)
    gp_item_default_embedded_path_override("${item}" path)
  endif(COMMAND gp_item_default_embedded_path_override)

  set(${default_embedded_path_var} "${path}" PARENT_SCOPE)
endfunction(gp_item_default_embedded_path)


# gp_resolve_item context item exepath dirs resolved_item_var
#
# Resolve an item into an existing full path file.
#
# Override on a per-project basis by providing a project-specific
# gp_resolve_item_override function.
#
function(gp_resolve_item context item exepath dirs resolved_item_var)
  set(resolved 0)
  set(resolved_item "${item}")

  # Is it already resolved?
  #
  if(EXISTS "${resolved_item}")
    set(resolved 1)
  endif(EXISTS "${resolved_item}")

  if(NOT resolved)
    if(item MATCHES "@executable_path")
      #
      # @executable_path references are assumed relative to exepath
      #
      string(REPLACE "@executable_path" "${exepath}" ri "${item}")
      get_filename_component(ri "${ri}" ABSOLUTE)

      if(EXISTS "${ri}")
        #message(STATUS "info: embedded item exists (${ri})")
        set(resolved 1)
        set(resolved_item "${ri}")
      else(EXISTS "${ri}")
        message(STATUS "warning: embedded item does not exist '${ri}'")
      endif(EXISTS "${ri}")
    endif(item MATCHES "@executable_path")
  endif(NOT resolved)

  if(NOT resolved)
    if(item MATCHES "@loader_path")
      #
      # @loader_path references are assumed relative to the
      # PATH of the given "context" (presumably another library)
      #
      get_filename_component(contextpath "${context}" PATH)
      string(REPLACE "@loader_path" "${contextpath}" ri "${item}")
      get_filename_component(ri "${ri}" ABSOLUTE)

      if(EXISTS "${ri}")
        #message(STATUS "info: embedded item exists (${ri})")
        set(resolved 1)
        set(resolved_item "${ri}")
      else(EXISTS "${ri}")
        message(STATUS "warning: embedded item does not exist '${ri}'")
      endif(EXISTS "${ri}")
    endif(item MATCHES "@loader_path")
  endif(NOT resolved)

  if(NOT resolved)
    set(ri "ri-NOTFOUND")
    find_file(ri "${item}" ${exepath} ${dirs} NO_DEFAULT_PATH)
    find_file(ri "${item}" ${exepath} ${dirs} /usr/lib)
    if(ri)
      #message(STATUS "info: 'find_file' in exepath/dirs (${ri})")
      set(resolved 1)
      set(resolved_item "${ri}")
      set(ri "ri-NOTFOUND")
    endif(ri)
  endif(NOT resolved)

  if(NOT resolved)
    if(item MATCHES "[^/]+\\.framework/")
      set(fw "fw-NOTFOUND")
      find_file(fw "${item}"
        "~/Library/Frameworks"
        "/Library/Frameworks"
        "/System/Library/Frameworks"
      )
      if(fw)
        #message(STATUS "info: 'find_file' found framework (${fw})")
        set(resolved 1)
        set(resolved_item "${fw}")
        set(fw "fw-NOTFOUND")
      endif(fw)
    endif(item MATCHES "[^/]+\\.framework/")
  endif(NOT resolved)

  # Using find_program on Windows will find dll files that are in the PATH.
  # (Converting simple file names into full path names if found.)
  #
  if(WIN32)
  if(NOT resolved)
    set(ri "ri-NOTFOUND")
    find_program(ri "${item}" PATHS "${exepath};${dirs}" NO_DEFAULT_PATH)
    find_program(ri "${item}" PATHS "${exepath};${dirs}")
    if(ri)
      #message(STATUS "info: 'find_program' in exepath/dirs (${ri})")
      set(resolved 1)
      set(resolved_item "${ri}")
      set(ri "ri-NOTFOUND")
    endif(ri)
  endif(NOT resolved)
  endif(WIN32)

  # Provide a hook so that projects can override item resolution
  # by whatever logic they choose:
  #
  if(COMMAND gp_resolve_item_override)
    gp_resolve_item_override("${context}" "${item}" "${exepath}" "${dirs}" resolved_item resolved)
  endif(COMMAND gp_resolve_item_override)

  if(NOT resolved)
    message(STATUS "
warning: cannot resolve item '${item}'

  possible problems:
    need more directories?
    need to use InstallRequiredSystemLibraries?
    run in install tree instead of build tree?
")
#    message(STATUS "
#******************************************************************************
#warning: cannot resolve item '${item}'
#
#  possible problems:
#    need more directories?
#    need to use InstallRequiredSystemLibraries?
#    run in install tree instead of build tree?
#
#    context='${context}'
#    item='${item}'
#    exepath='${exepath}'
#    dirs='${dirs}'
#    resolved_item_var='${resolved_item_var}'
#******************************************************************************
#")
  endif(NOT resolved)

  set(${resolved_item_var} "${resolved_item}" PARENT_SCOPE)
endfunction(gp_resolve_item)


# gp_resolved_file_type original_file file exepath dirs type_var
#
# Return the type of ${file} with respect to ${original_file}. String
# describing type of prerequisite is returned in variable named ${type_var}.
#
# Use ${exepath} and ${dirs} if necessary to resolve non-absolute ${file}
# values -- but only for non-embedded items.
#
# Possible types are:
#   system
#   local
#   embedded
#   other
#
function(gp_resolved_file_type original_file file exepath dirs type_var)
  #message(STATUS "**")

  if(NOT IS_ABSOLUTE "${original_file}")
    message(STATUS "warning: gp_resolved_file_type expects absolute full path for first arg original_file")
  endif()

  set(is_embedded 0)
  set(is_local 0)
  set(is_system 0)

  set(resolved_file "${file}")

  if("${file}" MATCHES "^@(executable|loader)_path")
    set(is_embedded 1)
  endif()

  if(NOT is_embedded)
    if(NOT IS_ABSOLUTE "${file}")
      gp_resolve_item("${original_file}" "${file}" "${exepath}" "${dirs}" resolved_file)
    endif()

    string(TOLOWER "${original_file}" original_lower)
    string(TOLOWER "${resolved_file}" lower)

    if(UNIX)
      if(resolved_file MATCHES "^(/lib/|/lib32/|/lib64/|/usr/lib/|/usr/lib32/|/usr/lib64/|/usr/X11R6/)")
        set(is_system 1)
      endif()
    endif()

    if(APPLE)
      if(resolved_file MATCHES "^(/System/Library/|/usr/lib/)")
        set(is_system 1)
      endif()
    endif()

    if(WIN32)
      string(TOLOWER "$ENV{SystemRoot}" sysroot)
      string(REGEX REPLACE "\\\\" "/" sysroot "${sysroot}")

      string(TOLOWER "$ENV{windir}" windir)
      string(REGEX REPLACE "\\\\" "/" windir "${windir}")

      if(lower MATCHES "^(${sysroot}/sys(tem|wow)|${windir}/sys(tem|wow)|(.*/)*msvc[^/]+dll)")
        set(is_system 1)
      endif()
    endif()

    if(NOT is_system)
      get_filename_component(original_path "${original_lower}" PATH)
      get_filename_component(path "${lower}" PATH)
      if("${original_path}" STREQUAL "${path}")
        set(is_local 1)
      endif()
    endif()
  endif()

  # Return type string based on computed booleans:
  #
  set(type "other")

  if(is_system)
    set(type "system")
  elseif(is_embedded)
    set(type "embedded")
  elseif(is_local)
    set(type "local")
  endif()

  #message(STATUS "gp_resolved_file_type: '${file}' '${resolved_file}'")
  #message(STATUS "                type: '${type}'")

  if(NOT is_embedded)
    if(NOT IS_ABSOLUTE "${resolved_file}")
      if(lower MATCHES "^msvc[^/]+dll" AND is_system)
        message(STATUS "info: non-absolute msvc file '${file}' returning type '${type}'")
      else()
        message(STATUS "warning: gp_resolved_file_type non-absolute file '${file}' returning type '${type}' -- possibly incorrect")
      endif()
    endif()
  endif()

  set(${type_var} "${type}" PARENT_SCOPE)

  #message(STATUS "**")
endfunction()


# gp_file_type original_file file type_var
#
# Return the type of ${file} with respect to ${original_file}. String
# describing type of prerequisite is returned in variable named ${type_var}.
#
# Possible types are:
#   system
#   local
#   embedded
#   other
#
function(gp_file_type original_file file type_var)
  if(NOT IS_ABSOLUTE "${original_file}")
    message(STATUS "warning: gp_file_type expects absolute full path for first arg original_file")
  endif()

  get_filename_component(exepath "${original_file}" PATH)

  set(type "")
  gp_resolved_file_type("${original_file}" "${file}" "${exepath}" "" type)

  set(${type_var} "${type}" PARENT_SCOPE)
endfunction(gp_file_type)


# get_prerequisites target prerequisites_var exclude_system recurse dirs
#
# Get the list of shared library files required by ${target}. The list in
# the variable named ${prerequisites_var} should be empty on first entry to
# this function. On exit, ${prerequisites_var} will contain the list of
# required shared library files.
#
#  target is the full path to an executable file
#
#  prerequisites_var is the name of a CMake variable to contain the results
#
#  exclude_system is 0 or 1: 0 to include "system" prerequisites , 1 to
#   exclude them
#
#  recurse is 0 or 1: 0 for direct prerequisites only, 1 for all prerequisites
#   recursively
#
#  exepath is the path to the top level executable used for @executable_path
#   replacment on the Mac
#
#  dirs is a list of paths where libraries might be found: these paths are
#   searched first when a target without any path info is given. Then standard
#   system locations are also searched: PATH, Framework locations, /usr/lib...
#
function(get_prerequisites target prerequisites_var exclude_system recurse exepath dirs)
  set(verbose 0)
  set(eol_char "E")

  if(NOT IS_ABSOLUTE "${target}")
    message("warning: target '${target}' is not absolute...")
  endif(NOT IS_ABSOLUTE "${target}")

  if(NOT EXISTS "${target}")
    message("warning: target '${target}' does not exist...")
  endif(NOT EXISTS "${target}")

  # <setup-gp_tool-vars>
  #
  # Try to choose the right tool by default. Caller can set gp_tool prior to
  # calling this function to force using a different tool.
  #
  if("${gp_tool}" STREQUAL "")
    set(gp_tool "ldd")
    if(APPLE)
      set(gp_tool "otool")
    endif(APPLE)
    if(WIN32)
      set(gp_tool "dumpbin")
    endif(WIN32)
  endif("${gp_tool}" STREQUAL "")

  set(gp_tool_known 0)

  if("${gp_tool}" STREQUAL "ldd")
    set(gp_cmd_args "")
    set(gp_regex "^[\t ]*[^\t ]+ => ([^\t ]+).*${eol_char}$")
    set(gp_regex_cmp_count 1)
    set(gp_tool_known 1)
  endif("${gp_tool}" STREQUAL "ldd")

  if("${gp_tool}" STREQUAL "otool")
    set(gp_cmd_args "-L")
    set(gp_regex "^\t([^\t]+) \\(compatibility version ([0-9]+.[0-9]+.[0-9]+), current version ([0-9]+.[0-9]+.[0-9]+)\\)${eol_char}$")
    set(gp_regex_cmp_count 3)
    set(gp_tool_known 1)
  endif("${gp_tool}" STREQUAL "otool")

  if("${gp_tool}" STREQUAL "dumpbin")
    set(gp_cmd_args "/dependents")
    set(gp_regex "^    ([^ ].*[Dd][Ll][Ll])${eol_char}$")
    set(gp_regex_cmp_count 1)
    set(gp_tool_known 1)
    set(ENV{VS_UNICODE_OUTPUT} "") # Block extra output from inside VS IDE.
  endif("${gp_tool}" STREQUAL "dumpbin")

  if(NOT gp_tool_known)
    message(STATUS "warning: gp_tool='${gp_tool}' is an unknown tool...")
    message(STATUS "CMake function get_prerequisites needs more code to handle '${gp_tool}'")
    message(STATUS "Valid gp_tool values are dumpbin, ldd and otool.")
    return()
  endif(NOT gp_tool_known)

  set(gp_cmd_paths ${gp_cmd_paths}
    "C:/Program Files/Microsoft Visual Studio 9.0/VC/bin"
    "C:/Program Files (x86)/Microsoft Visual Studio 9.0/VC/bin"
    "C:/Program Files/Microsoft Visual Studio 8/VC/BIN"
    "C:/Program Files (x86)/Microsoft Visual Studio 8/VC/BIN"
    "C:/Program Files/Microsoft Visual Studio .NET 2003/VC7/BIN"
    "C:/Program Files (x86)/Microsoft Visual Studio .NET 2003/VC7/BIN"
    "/usr/local/bin"
    "/usr/bin"
    )

  find_program(gp_cmd ${gp_tool} PATHS ${gp_cmd_paths})

  if(NOT gp_cmd)
    message(STATUS "warning: could not find '${gp_tool}' - cannot analyze prerequisites...")
    return()
  endif(NOT gp_cmd)

  if("${gp_tool}" STREQUAL "dumpbin")
    # When running dumpbin, it also needs the "Common7/IDE" directory in the
    # PATH. It will already be in the PATH if being run from a Visual Studio
    # command prompt. Add it to the PATH here in case we are running from a
    # different command prompt.
    #
    get_filename_component(gp_cmd_dir "${gp_cmd}" PATH)
    get_filename_component(gp_cmd_dlls_dir "${gp_cmd_dir}/../../Common7/IDE" ABSOLUTE)
    if(EXISTS "${gp_cmd_dlls_dir}")
      # only add to the path if it is not already in the path
      if(NOT "$ENV{PATH}" MATCHES "${gp_cmd_dlls_dir}")
        set(ENV{PATH} "$ENV{PATH};${gp_cmd_dlls_dir}")
      endif(NOT "$ENV{PATH}" MATCHES "${gp_cmd_dlls_dir}")
    endif(EXISTS "${gp_cmd_dlls_dir}")
  endif("${gp_tool}" STREQUAL "dumpbin")
  #
  # </setup-gp_tool-vars>

  if("${gp_tool}" STREQUAL "ldd")
    set(old_ld_env "$ENV{LD_LIBRARY_PATH}")
    foreach(dir ${exepath} ${dirs})
      set(ENV{LD_LIBRARY_PATH} "${dir}:$ENV{LD_LIBRARY_PATH}")
    endforeach(dir)
  endif("${gp_tool}" STREQUAL "ldd")


  # Track new prerequisites at each new level of recursion. Start with an
  # empty list at each level:
  #
  set(unseen_prereqs)

  # Run gp_cmd on the target:
  #
  execute_process(
    COMMAND ${gp_cmd} ${gp_cmd_args} ${target}
    OUTPUT_VARIABLE gp_cmd_ov
    )

  if("${gp_tool}" STREQUAL "ldd")
    set(ENV{LD_LIBRARY_PATH} "${old_ld_env}")
  endif("${gp_tool}" STREQUAL "ldd")

  if(verbose)
    message(STATUS "<RawOutput cmd='${gp_cmd} ${gp_cmd_args} ${target}'>")
    message(STATUS "gp_cmd_ov='${gp_cmd_ov}'")
    message(STATUS "</RawOutput>")
  endif(verbose)

  get_filename_component(target_dir "${target}" PATH)

  # Convert to a list of lines:
  #
  string(REGEX REPLACE ";" "\\\\;" candidates "${gp_cmd_ov}")
  string(REGEX REPLACE "\n" "${eol_char};" candidates "${candidates}")

  # Analyze each line for file names that match the regular expression:
  #
  foreach(candidate ${candidates})
  if("${candidate}" MATCHES "${gp_regex}")
    # Extract information from each candidate:
    string(REGEX REPLACE "${gp_regex}" "\\1" raw_item "${candidate}")

    if(gp_regex_cmp_count GREATER 1)
      string(REGEX REPLACE "${gp_regex}" "\\2" raw_compat_version "${candidate}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\1" compat_major_version "${raw_compat_version}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\2" compat_minor_version "${raw_compat_version}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\3" compat_patch_version "${raw_compat_version}")
    endif(gp_regex_cmp_count GREATER 1)

    if(gp_regex_cmp_count GREATER 2)
      string(REGEX REPLACE "${gp_regex}" "\\3" raw_current_version "${candidate}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\1" current_major_version "${raw_current_version}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\2" current_minor_version "${raw_current_version}")
      string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" "\\3" current_patch_version "${raw_current_version}")
    endif(gp_regex_cmp_count GREATER 2)

    # Use the raw_item as the list entries returned by this function. Use the
    # gp_resolve_item function to resolve it to an actual full path file if
    # necessary.
    #
    set(item "${raw_item}")

    # Add each item unless it is excluded:
    #
    set(add_item 1)

    if(${exclude_system})
      set(type "")
      gp_resolved_file_type("${target}" "${item}" "${exepath}" "${dirs}" type)

      if("${type}" STREQUAL "system")
        set(add_item 0)
      endif("${type}" STREQUAL "system")
    endif(${exclude_system})

    if(add_item)
      list(LENGTH ${prerequisites_var} list_length_before_append)
      gp_append_unique(${prerequisites_var} "${item}")
      list(LENGTH ${prerequisites_var} list_length_after_append)

      if(${recurse})
        # If item was really added, this is the first time we have seen it.
        # Add it to unseen_prereqs so that we can recursively add *its*
        # prerequisites...
        #
        # But first: resolve its name to an absolute full path name such
        # that the analysis tools can simply accept it as input.
        #
        if(NOT list_length_before_append EQUAL list_length_after_append)
          gp_resolve_item("${target}" "${item}" "${exepath}" "${dirs}" resolved_item)
          set(unseen_prereqs ${unseen_prereqs} "${resolved_item}")
        endif(NOT list_length_before_append EQUAL list_length_after_append)
      endif(${recurse})
    endif(add_item)
  else("${candidate}" MATCHES "${gp_regex}")
    if(verbose)
      message(STATUS "ignoring non-matching line: '${candidate}'")
    endif(verbose)
  endif("${candidate}" MATCHES "${gp_regex}")
  endforeach(candidate)

  list(LENGTH ${prerequisites_var} prerequisites_var_length)
  if(prerequisites_var_length GREATER 0)
    list(SORT ${prerequisites_var})
  endif(prerequisites_var_length GREATER 0)
  if(${recurse})
    set(more_inputs ${unseen_prereqs})
    foreach(input ${more_inputs})
      get_prerequisites("${input}" ${prerequisites_var} ${exclude_system} ${recurse} "${exepath}" "${dirs}")
    endforeach(input)
  endif(${recurse})

  set(${prerequisites_var} ${${prerequisites_var}} PARENT_SCOPE)
endfunction(get_prerequisites)


# list_prerequisites target all exclude_system verbose
#
#  ARGV0 (target) is the full path to an executable file
#
#  optional ARGV1 (all) is 0 or 1: 0 for direct prerequisites only,
#   1 for all prerequisites recursively
#
#  optional ARGV2 (exclude_system) is 0 or 1: 0 to include "system"
#   prerequisites , 1 to exclude them
#
#  optional ARGV3 (verbose) is 0 or 1: 0 to print only full path
#   names of prerequisites, 1 to print extra information
#
function(list_prerequisites target)
  if("${ARGV1}" STREQUAL "")
    set(all 1)
  else("${ARGV1}" STREQUAL "")
    set(all "${ARGV1}")
  endif("${ARGV1}" STREQUAL "")

  if("${ARGV2}" STREQUAL "")
    set(exclude_system 0)
  else("${ARGV2}" STREQUAL "")
    set(exclude_system "${ARGV2}")
  endif("${ARGV2}" STREQUAL "")

  if("${ARGV3}" STREQUAL "")
    set(verbose 0)
  else("${ARGV3}" STREQUAL "")
    set(verbose "${ARGV3}")
  endif("${ARGV3}" STREQUAL "")

  set(count 0)
  set(count_str "")
  set(print_count "${verbose}")
  set(print_prerequisite_type "${verbose}")
  set(print_target "${verbose}")
  set(type_str "")

  get_filename_component(exepath "${target}" PATH)

  set(prereqs "")
  get_prerequisites("${target}" prereqs ${exclude_system} ${all} "${exepath}" "")

  if(print_target)
    message(STATUS "File '${target}' depends on:")
  endif(print_target)

  foreach(d ${prereqs})
    math(EXPR count "${count} + 1")

    if(print_count)
      set(count_str "${count}. ")
    endif(print_count)

    if(print_prerequisite_type)
      gp_file_type("${target}" "${d}" type)
      set(type_str " (${type})")
    endif(print_prerequisite_type)

    message(STATUS "${count_str}${d}${type_str}")
  endforeach(d)
endfunction(list_prerequisites)


# list_prerequisites_by_glob glob_arg glob_exp
#
#  glob_arg is GLOB or GLOB_RECURSE
#
#  glob_exp is a globbing expression used with "file(GLOB" to retrieve a list
#   of matching files. If a matching file is executable, its prerequisites are
#   listed.
#
# Any additional (optional) arguments provided are passed along as the
# optional arguments to the list_prerequisites calls.
#
function(list_prerequisites_by_glob glob_arg glob_exp)
  message(STATUS "=============================================================================")
  message(STATUS "List prerequisites of executables matching ${glob_arg} '${glob_exp}'")
  message(STATUS "")
  file(${glob_arg} file_list ${glob_exp})
  foreach(f ${file_list})
    is_file_executable("${f}" is_f_executable)
    if(is_f_executable)
      message(STATUS "=============================================================================")
      list_prerequisites("${f}" ${ARGN})
      message(STATUS "")
    endif(is_f_executable)
  endforeach(f)
endfunction(list_prerequisites_by_glob)
