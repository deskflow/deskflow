# BundleUtilities.cmake
#
# A collection of CMake utility functions useful for dealing with .app bundles
# on the Mac and bundle-like directories on any OS.
#
# The following functions are provided by this script:
#   get_bundle_main_executable
#   get_dotapp_dir
#   get_bundle_and_executable
#   get_bundle_all_executables
#   get_item_key
#   clear_bundle_keys
#   set_bundle_key_values
#   get_bundle_keys
#   copy_resolved_item_into_bundle
#   fixup_bundle_item
#   fixup_bundle
#   copy_and_fixup_bundle
#   verify_bundle_prerequisites
#   verify_bundle_symlinks
#   verify_app
#
# Requires CMake 2.6 or greater because it uses function, break and
# PARENT_SCOPE. Also depends on GetPrerequisites.cmake.

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

# The functions defined in this file depend on the get_prerequisites function
# (and possibly others) found in:
#
get_filename_component(BundleUtilities_cmake_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
include("${BundleUtilities_cmake_dir}/GetPrerequisites.cmake")


# get_bundle_main_executable
#
# The result will be the full path name of the bundle's main executable file
# or an "error:" prefixed string if it could not be determined.
#
function(get_bundle_main_executable bundle result_var)
  set(result "error: '${bundle}/Contents/Info.plist' file does not exist")

  if(EXISTS "${bundle}/Contents/Info.plist")
    set(result "error: no CFBundleExecutable in '${bundle}/Contents/Info.plist' file")
    set(line_is_main_executable 0)
    set(bundle_executable "")

    # Read Info.plist as a list of lines:
    #
    set(eol_char "E")
    file(READ "${bundle}/Contents/Info.plist" info_plist)
    string(REGEX REPLACE ";" "\\\\;" info_plist "${info_plist}")
    string(REGEX REPLACE "\n" "${eol_char};" info_plist "${info_plist}")

    # Scan the lines for "<key>CFBundleExecutable</key>" - the line after that
    # is the name of the main executable.
    #
    foreach(line ${info_plist})
      if(line_is_main_executable)
        string(REGEX REPLACE "^.*<string>(.*)</string>.*$" "\\1" bundle_executable "${line}")
        break()
      endif(line_is_main_executable)

      if(line MATCHES "^.*<key>CFBundleExecutable</key>.*$")
        set(line_is_main_executable 1)
      endif(line MATCHES "^.*<key>CFBundleExecutable</key>.*$")
    endforeach(line)

    if(NOT "${bundle_executable}" STREQUAL "")
      if(EXISTS "${bundle}/Contents/MacOS/${bundle_executable}")
        set(result "${bundle}/Contents/MacOS/${bundle_executable}")
      else(EXISTS "${bundle}/Contents/MacOS/${bundle_executable}")

        # Ultimate goal:
        # If not in "Contents/MacOS" then scan the bundle for matching files. If
        # there is only one executable file that matches, then use it, otherwise
        # it's an error...
        #
        #file(GLOB_RECURSE file_list "${bundle}/${bundle_executable}")

        # But for now, pragmatically, it's an error. Expect the main executable
        # for the bundle to be in Contents/MacOS, it's an error if it's not:
        #
        set(result "error: '${bundle}/Contents/MacOS/${bundle_executable}' does not exist")
      endif(EXISTS "${bundle}/Contents/MacOS/${bundle_executable}")
    endif(NOT "${bundle_executable}" STREQUAL "")
  else(EXISTS "${bundle}/Contents/Info.plist")
    #
    # More inclusive technique... (This one would work on Windows and Linux
    # too, if a developer followed the typical Mac bundle naming convention...)
    #
    # If there is no Info.plist file, try to find an executable with the same
    # base name as the .app directory:
    #
  endif(EXISTS "${bundle}/Contents/Info.plist")

  set(${result_var} "${result}" PARENT_SCOPE)
endfunction(get_bundle_main_executable)


# get_dotapp_dir
#
# Returns the nearest parent dir whose name ends with ".app" given the full path
# to an executable. If there is no such parent dir, then return a dir at the same
# level as the executable, named with the executable's base name and ending with
# ".app"
#
# The returned directory may or may not exist.
#
function(get_dotapp_dir exe dotapp_dir_var)
  set(s "${exe}")

  set(has_dotapp_parent 0)
  if(s MATCHES "^.*/.*\\.app/.*$")
    set(has_dotapp_parent 1)
  endif(s MATCHES "^.*/.*\\.app/.*$")

  set(done 0)
  while(NOT ${done})
    get_filename_component(snamewe "${s}" NAME_WE)
    get_filename_component(sname "${s}" NAME)
    get_filename_component(sdir "${s}" PATH)
    if(has_dotapp_parent)
      # If there is a ".app" parent directory,
      # ascend until we hit it:
      #   (typical of a Mac bundle executable)
      #
      set(s "${sdir}")
      if(sname MATCHES "\\.app$")
        set(done 1)
        set(dotapp_dir "${sdir}/${sname}")
      endif(sname MATCHES "\\.app$")
    else(has_dotapp_parent)
      # Otherwise use a directory named the same
      # as the exe, but with a ".app" extension:
      #   (typical of a non-bundle executable on Mac, Windows or Linux)
      #
      set(done 1)
      set(dotapp_dir "${sdir}/${snamewe}.app")
    endif(has_dotapp_parent)
  endwhile(NOT ${done})

  set(${dotapp_dir_var} "${dotapp_dir}" PARENT_SCOPE)
endfunction(get_dotapp_dir)


# get_bundle_and_executable
#
# Takes either a ".app" directory name or the name of an executable
# nested inside a ".app" directory and returns the path to the ".app"
# directory in ${bundle_var} and the path to its main executable in
# ${executable_var}
#
function(get_bundle_and_executable app bundle_var executable_var valid_var)
  set(valid 0)

  if(EXISTS "${app}")
    # Is it a directory ending in .app?
    if(IS_DIRECTORY "${app}")
      if(app MATCHES "\\.app$")
        get_bundle_main_executable("${app}" executable)
        if(EXISTS "${app}" AND EXISTS "${executable}")
          set(${bundle_var} "${app}" PARENT_SCOPE)
          set(${executable_var} "${executable}" PARENT_SCOPE)
          set(valid 1)
          #message(STATUS "info: handled .app directory case...")
        else(EXISTS "${app}" AND EXISTS "${executable}")
          message(STATUS "warning: *NOT* handled - .app directory case...")
        endif(EXISTS "${app}" AND EXISTS "${executable}")
      else(app MATCHES "\\.app$")
        message(STATUS "warning: *NOT* handled - directory but not .app case...")
      endif(app MATCHES "\\.app$")
    else(IS_DIRECTORY "${app}")
      # Is it an executable file?
      is_file_executable("${app}" is_executable)
      if(is_executable)
        get_dotapp_dir("${app}" dotapp_dir)
        if(EXISTS "${dotapp_dir}")
          set(${bundle_var} "${dotapp_dir}" PARENT_SCOPE)
          set(${executable_var} "${app}" PARENT_SCOPE)
          set(valid 1)
          #message(STATUS "info: handled executable file in .app dir case...")
        else()
          get_filename_component(app_dir "${app}" PATH)
          set(${bundle_var} "${app_dir}" PARENT_SCOPE)
          set(${executable_var} "${app}" PARENT_SCOPE)
          set(valid 1)
          #message(STATUS "info: handled executable file in any dir case...")
        endif()
      else(is_executable)
        message(STATUS "warning: *NOT* handled - not .app dir, not executable file...")
      endif(is_executable)
    endif(IS_DIRECTORY "${app}")
  else(EXISTS "${app}")
    message(STATUS "warning: *NOT* handled - directory/file does not exist...")
  endif(EXISTS "${app}")

  if(NOT valid)
    set(${bundle_var} "error: not a bundle" PARENT_SCOPE)
    set(${executable_var} "error: not a bundle" PARENT_SCOPE)
  endif(NOT valid)

  set(${valid_var} ${valid} PARENT_SCOPE)
endfunction(get_bundle_and_executable)


# get_bundle_all_executables
#
# Scans the given bundle recursively for all executable files and accumulates
# them into a variable.
#
function(get_bundle_all_executables bundle exes_var)
  set(exes "")

  file(GLOB_RECURSE file_list "${bundle}/*")
  foreach(f ${file_list})
    is_file_executable("${f}" is_executable)
    if(is_executable)
      set(exes ${exes} "${f}")
    endif(is_executable)
  endforeach(f)

  set(${exes_var} "${exes}" PARENT_SCOPE)
endfunction(get_bundle_all_executables)


# get_item_key
#
# Given a file (item) name, generate a key that should be unique considering the set of
# libraries that need copying or fixing up to make a bundle standalone. This is
# essentially the file name including extension with "." replaced by "_"
#
# This key is used as a prefix for CMake variables so that we can associate a set
# of variables with a given item based on its key.
#
function(get_item_key item key_var)
  get_filename_component(item_name "${item}" NAME)
  if(WIN32)
    string(TOLOWER "${item_name}" item_name)
  endif()
  string(REGEX REPLACE "\\." "_" ${key_var} "${item_name}")
  set(${key_var} ${${key_var}} PARENT_SCOPE)
endfunction(get_item_key)


# clear_bundle_keys
#
# Loop over the list of keys, clearing all the variables associated with each
# key. After the loop, clear the list of keys itself.
#
# Caller of get_bundle_keys should call clear_bundle_keys when done with list
# of keys.
#
function(clear_bundle_keys keys_var)
  foreach(key ${${keys_var}})
    set(${key}_ITEM PARENT_SCOPE)
    set(${key}_RESOLVED_ITEM PARENT_SCOPE)
    set(${key}_DEFAULT_EMBEDDED_PATH PARENT_SCOPE)
    set(${key}_EMBEDDED_ITEM PARENT_SCOPE)
    set(${key}_RESOLVED_EMBEDDED_ITEM PARENT_SCOPE)
    set(${key}_COPYFLAG PARENT_SCOPE)
  endforeach(key)
  set(${keys_var} PARENT_SCOPE)
endfunction(clear_bundle_keys)


# set_bundle_key_values
#
# Add a key to the list (if necessary) for the given item. If added,
# also set all the variables associated with that key.
#
function(set_bundle_key_values keys_var context item exepath dirs copyflag)
  get_filename_component(item_name "${item}" NAME)

  get_item_key("${item}" key)

  list(LENGTH ${keys_var} length_before)
  gp_append_unique(${keys_var} "${key}")
  list(LENGTH ${keys_var} length_after)

  if(NOT length_before EQUAL length_after)
    gp_resolve_item("${context}" "${item}" "${exepath}" "${dirs}" resolved_item)

    gp_item_default_embedded_path("${item}" default_embedded_path)

    if(item MATCHES "[^/]+\\.framework/")
      # For frameworks, construct the name under the embedded path from the
      # opening "${item_name}.framework/" to the closing "/${item_name}":
      #
      string(REGEX REPLACE "^.*(${item_name}.framework/.*/${item_name}).*$" "${default_embedded_path}/\\1" embedded_item "${item}")
    else(item MATCHES "[^/]+\\.framework/")
      # For other items, just use the same name as the original, but in the
      # embedded path:
      #
      set(embedded_item "${default_embedded_path}/${item_name}")
    endif(item MATCHES "[^/]+\\.framework/")

    # Replace @executable_path and resolve ".." references:
    #
    string(REPLACE "@executable_path" "${exepath}" resolved_embedded_item "${embedded_item}")
    get_filename_component(resolved_embedded_item "${resolved_embedded_item}" ABSOLUTE)

    # *But* -- if we are not copying, then force resolved_embedded_item to be
    # the same as resolved_item. In the case of multiple executables in the
    # original bundle, using the default_embedded_path results in looking for
    # the resolved executable next to the main bundle executable. This is here
    # so that exes in the other sibling directories (like "bin") get fixed up
    # properly...
    #
    if(NOT copyflag)
      set(resolved_embedded_item "${resolved_item}")
    endif(NOT copyflag)

    set(${keys_var} ${${keys_var}} PARENT_SCOPE)
    set(${key}_ITEM "${item}" PARENT_SCOPE)
    set(${key}_RESOLVED_ITEM "${resolved_item}" PARENT_SCOPE)
    set(${key}_DEFAULT_EMBEDDED_PATH "${default_embedded_path}" PARENT_SCOPE)
    set(${key}_EMBEDDED_ITEM "${embedded_item}" PARENT_SCOPE)
    set(${key}_RESOLVED_EMBEDDED_ITEM "${resolved_embedded_item}" PARENT_SCOPE)
    set(${key}_COPYFLAG "${copyflag}" PARENT_SCOPE)
  else(NOT length_before EQUAL length_after)
    #message("warning: item key '${key}' already in the list, subsequent references assumed identical to first")
  endif(NOT length_before EQUAL length_after)
endfunction(set_bundle_key_values)


# get_bundle_keys
#
# Loop over all the executable and library files within the bundle (and given as
# extra "${libs}") and accumulate a list of keys representing them. Set values
# associated with each key such that we can loop over all of them and copy
# prerequisite libs into the bundle and then do appropriate install_name_tool
# fixups.
#
function(get_bundle_keys app libs dirs keys_var)
  set(${keys_var} PARENT_SCOPE)

  get_bundle_and_executable("${app}" bundle executable valid)
  if(valid)
    # Always use the exepath of the main bundle executable for @executable_path
    # replacements:
    #
    get_filename_component(exepath "${executable}" PATH)

    # But do fixups on all executables in the bundle:
    #
    get_bundle_all_executables("${bundle}" exes)

    # For each extra lib, accumulate a key as well and then also accumulate
    # any of its prerequisites. (Extra libs are typically dynamically loaded
    # plugins: libraries that are prerequisites for full runtime functionality
    # but that do not show up in otool -L output...)
    #
    foreach(lib ${libs})
      set_bundle_key_values(${keys_var} "${lib}" "${lib}" "${exepath}" "${dirs}" 1)

      set(prereqs "")
      get_prerequisites("${lib}" prereqs 1 1 "${exepath}" "${dirs}")
      foreach(pr ${prereqs})
        set_bundle_key_values(${keys_var} "${lib}" "${pr}" "${exepath}" "${dirs}" 1)
      endforeach(pr)
    endforeach(lib)

    # For each executable found in the bundle, accumulate keys as we go.
    # The list of keys should be complete when all prerequisites of all
    # binaries in the bundle have been analyzed.
    #
    foreach(exe ${exes})
      # Add the exe itself to the keys:
      #
      set_bundle_key_values(${keys_var} "${exe}" "${exe}" "${exepath}" "${dirs}" 0)

      # Add each prerequisite to the keys:
      #
      set(prereqs "")
      get_prerequisites("${exe}" prereqs 1 1 "${exepath}" "${dirs}")
      foreach(pr ${prereqs})
        set_bundle_key_values(${keys_var} "${exe}" "${pr}" "${exepath}" "${dirs}" 1)
      endforeach(pr)
    endforeach(exe)

    # Propagate values to caller's scope:
    #
    set(${keys_var} ${${keys_var}} PARENT_SCOPE)
    foreach(key ${${keys_var}})
      set(${key}_ITEM "${${key}_ITEM}" PARENT_SCOPE)
      set(${key}_RESOLVED_ITEM "${${key}_RESOLVED_ITEM}" PARENT_SCOPE)
      set(${key}_DEFAULT_EMBEDDED_PATH "${${key}_DEFAULT_EMBEDDED_PATH}" PARENT_SCOPE)
      set(${key}_EMBEDDED_ITEM "${${key}_EMBEDDED_ITEM}" PARENT_SCOPE)
      set(${key}_RESOLVED_EMBEDDED_ITEM "${${key}_RESOLVED_EMBEDDED_ITEM}" PARENT_SCOPE)
      set(${key}_COPYFLAG "${${key}_COPYFLAG}" PARENT_SCOPE)
    endforeach(key)
  endif(valid)
endfunction(get_bundle_keys)


# copy_resolved_item_into_bundle
#
# Copy a resolved item into the bundle if necessary. Copy is not necessary if
# the resolved_item is "the same as" the resolved_embedded_item.
#
function(copy_resolved_item_into_bundle resolved_item resolved_embedded_item)
  if(WIN32)
    # ignore case on Windows
    string(TOLOWER "${resolved_item}" resolved_item_compare)
    string(TOLOWER "${resolved_embedded_item}" resolved_embedded_item_compare)
  else()
    set(resolved_item_compare "${resolved_item}")
    set(resolved_embedded_item_compare "${resolved_embedded_item}")
  endif()

  if("${resolved_item_compare}" STREQUAL "${resolved_embedded_item_compare}")
    message(STATUS "warning: resolved_item == resolved_embedded_item - not copying...")
  else()
    #message(STATUS "copying COMMAND ${CMAKE_COMMAND} -E copy ${resolved_item} ${resolved_embedded_item}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${resolved_item}" "${resolved_embedded_item}")
  endif()

  if(UNIX AND NOT APPLE)
    file(RPATH_REMOVE FILE "${resolved_embedded_item}")
  endif(UNIX AND NOT APPLE)
endfunction(copy_resolved_item_into_bundle)


# fixup_bundle_item
#
# Get the direct/non-system prerequisites of the resolved embedded item. For each
# prerequisite, change the way it is referenced to the value of the _EMBEDDED_ITEM
# keyed variable for that prerequisite. (Most likely changing to an "@executable_path"
# style reference.)
#
# Also, change the id of the item being fixed up to its own _EMBEDDED_ITEM value.
#
# Accumulate changes in a local variable and make *one* call to install_name_tool
# at the end of the function with all the changes at once.
#
function(fixup_bundle_item resolved_embedded_item exepath dirs)
  # This item's key is "ikey":
  #
  get_item_key("${resolved_embedded_item}" ikey)

  set(prereqs "")
  get_prerequisites("${resolved_embedded_item}" prereqs 1 0 "${exepath}" "${dirs}")

  set(changes "")

  foreach(pr ${prereqs})
    # Each referenced item's key is "rkey" in the loop:
    #
    get_item_key("${pr}" rkey)

    if(NOT "${${rkey}_EMBEDDED_ITEM}" STREQUAL "")
      set(changes ${changes} "-change" "${pr}" "${${rkey}_EMBEDDED_ITEM}")
    else(NOT "${${rkey}_EMBEDDED_ITEM}" STREQUAL "")
      message("warning: unexpected reference to '${pr}'")
    endif(NOT "${${rkey}_EMBEDDED_ITEM}" STREQUAL "")
  endforeach(pr)

  # Change this item's id and all of its references in one call
  # to install_name_tool:
  #
  execute_process(COMMAND install_name_tool
    ${changes} -id "${${ikey}_EMBEDDED_ITEM}" "${resolved_embedded_item}"
  )
endfunction(fixup_bundle_item)


# fixup_bundle
#
# Fix up a bundle in-place and make it standalone, such that it can be drag-n-drop
# copied to another machine and run on that machine as long as all of the system
# libraries are compatible.
#
# Gather all the keys for all the executables and libraries in a bundle, and then,
# for each key, copy each prerequisite into the bundle. Then fix each one up according
# to its own list of prerequisites.
#
# Then clear all the keys and call verify_app on the final bundle to ensure that
# it is truly standalone.
#
function(fixup_bundle app libs dirs)
  message(STATUS "fixup_bundle")
  message(STATUS "  app='${app}'")
  message(STATUS "  libs='${libs}'")
  message(STATUS "  dirs='${dirs}'")

  get_bundle_and_executable("${app}" bundle executable valid)
  if(valid)
    get_filename_component(exepath "${executable}" PATH)

    message(STATUS "fixup_bundle: preparing...")
    get_bundle_keys("${app}" "${libs}" "${dirs}" keys)

    message(STATUS "fixup_bundle: copying...")
    list(LENGTH keys n)
    math(EXPR n ${n}*2)

    set(i 0)
    foreach(key ${keys})
      math(EXPR i ${i}+1)
      if(${${key}_COPYFLAG})
        message(STATUS "${i}/${n}: copying '${${key}_RESOLVED_ITEM}'")
      else(${${key}_COPYFLAG})
        message(STATUS "${i}/${n}: *NOT* copying '${${key}_RESOLVED_ITEM}'")
      endif(${${key}_COPYFLAG})

      set(show_status 0)
      if(show_status)
        message(STATUS "key='${key}'")
        message(STATUS "item='${${key}_ITEM}'")
        message(STATUS "resolved_item='${${key}_RESOLVED_ITEM}'")
        message(STATUS "default_embedded_path='${${key}_DEFAULT_EMBEDDED_PATH}'")
        message(STATUS "embedded_item='${${key}_EMBEDDED_ITEM}'")
        message(STATUS "resolved_embedded_item='${${key}_RESOLVED_EMBEDDED_ITEM}'")
        message(STATUS "copyflag='${${key}_COPYFLAG}'")
        message(STATUS "")
      endif(show_status)

      if(${${key}_COPYFLAG})
        copy_resolved_item_into_bundle("${${key}_RESOLVED_ITEM}"
          "${${key}_RESOLVED_EMBEDDED_ITEM}")
      endif(${${key}_COPYFLAG})
    endforeach(key)

    message(STATUS "fixup_bundle: fixing...")
    foreach(key ${keys})
      math(EXPR i ${i}+1)
      if(APPLE)
        message(STATUS "${i}/${n}: fixing up '${${key}_RESOLVED_EMBEDDED_ITEM}'")
        fixup_bundle_item("${${key}_RESOLVED_EMBEDDED_ITEM}" "${exepath}" "${dirs}")
      else(APPLE)
        message(STATUS "${i}/${n}: fix-up not required on this platform '${${key}_RESOLVED_EMBEDDED_ITEM}'")
      endif(APPLE)
    endforeach(key)

    message(STATUS "fixup_bundle: cleaning up...")
    clear_bundle_keys(keys)

    message(STATUS "fixup_bundle: verifying...")
    verify_app("${app}")
  else(valid)
    message(SEND_ERROR "error: fixup_bundle: not a valid bundle")
  endif(valid)

  message(STATUS "fixup_bundle: done")
endfunction(fixup_bundle)


# copy_and_fixup_bundle
#
# Makes a copy of the bundle "src" at location "dst" and then fixes up the
# new copied bundle in-place at "dst"...
#
function(copy_and_fixup_bundle src dst libs dirs)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory "${src}" "${dst}")
  fixup_bundle("${dst}" "${libs}" "${dirs}")
endfunction(copy_and_fixup_bundle)


# verify_bundle_prerequisites
#
# Verifies that the sum of all prerequisites of all files inside the bundle
# are contained within the bundle or are "system" libraries, presumed to exist
# everywhere.
#
function(verify_bundle_prerequisites bundle result_var info_var)
  set(result 1)
  set(info "")
  set(count 0)

  get_bundle_main_executable("${bundle}" main_bundle_exe)

  file(GLOB_RECURSE file_list "${bundle}/*")
  foreach(f ${file_list})
    is_file_executable("${f}" is_executable)
    if(is_executable)
      get_filename_component(exepath "${f}" PATH)
      math(EXPR count "${count} + 1")

      message(STATUS "executable file ${count}: ${f}")

      set(prereqs "")
      get_prerequisites("${f}" prereqs 1 1 "${exepath}" "")

      # On the Mac,
      # "embedded" and "system" prerequisites are fine... anything else means
      # the bundle's prerequisites are not verified (i.e., the bundle is not
      # really "standalone")
      #
      # On Windows (and others? Linux/Unix/...?)
      # "local" and "system" prereqs are fine...
      #
      set(external_prereqs "")

      foreach(p ${prereqs})
        set(p_type "")
        gp_file_type("${f}" "${p}" p_type)

        if(APPLE)
          if(NOT "${p_type}" STREQUAL "embedded" AND NOT "${p_type}" STREQUAL "system")
            set(external_prereqs ${external_prereqs} "${p}")
          endif()
        else()
          if(NOT "${p_type}" STREQUAL "local" AND NOT "${p_type}" STREQUAL "system")
            set(external_prereqs ${external_prereqs} "${p}")
          endif()
        endif()
      endforeach(p)

      if(external_prereqs)
        # Found non-system/somehow-unacceptable prerequisites:
        set(result 0)
        set(info ${info} "external prerequisites found:\nf='${f}'\nexternal_prereqs='${external_prereqs}'\n")
      endif(external_prereqs)
    endif(is_executable)
  endforeach(f)

  if(result)
    set(info "Verified ${count} executable files in '${bundle}'")
  endif(result)

  set(${result_var} "${result}" PARENT_SCOPE)
  set(${info_var} "${info}" PARENT_SCOPE)
endfunction(verify_bundle_prerequisites)


# verify_bundle_symlinks
#
# Verifies that any symlinks found in the bundle point to other files that are
# already also in the bundle... Anything that points to an external file causes
# this function to fail the verification.
#
function(verify_bundle_symlinks bundle result_var info_var)
  set(result 1)
  set(info "")
  set(count 0)

  # TODO: implement this function for real...
  # Right now, it is just a stub that verifies unconditionally...

  set(${result_var} "${result}" PARENT_SCOPE)
  set(${info_var} "${info}" PARENT_SCOPE)
endfunction(verify_bundle_symlinks)


# verify_app
#
# Verifies that an application appears valid based on running analysis tools on it.
# Calls message/FATAL_ERROR if the application is not verified.
#
function(verify_app app)
  set(verified 0)
  set(info "")

  get_bundle_and_executable("${app}" bundle executable valid)

  message(STATUS "===========================================================================")
  message(STATUS "Analyzing app='${app}'")
  message(STATUS "bundle='${bundle}'")
  message(STATUS "executable='${executable}'")
  message(STATUS "valid='${valid}'")

  # Verify that the bundle does not have any "external" prerequisites:
  #
  verify_bundle_prerequisites("${bundle}" verified info)
  message(STATUS "verified='${verified}'")
  message(STATUS "info='${info}'")
  message(STATUS "")

  if(verified)
    # Verify that the bundle does not have any symlinks to external files:
    #
    verify_bundle_symlinks("${bundle}" verified info)
    message(STATUS "verified='${verified}'")
    message(STATUS "info='${info}'")
    message(STATUS "")
  endif(verified)

  if(NOT verified)
    message(FATAL_ERROR "error: verify_app failed")
  endif(NOT verified)
endfunction(verify_app)
