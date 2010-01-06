
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
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# Function parse implicit linker options.
# This is used internally by CMake and should not be included by user
# code.

function(CMAKE_PARSE_IMPLICIT_LINK_INFO text lib_var dir_var log_var)
  set(implicit_libs_tmp "")
  set(implicit_dirs_tmp)
  set(log "")

  # Parse implicit linker arguments.
  set(linker "CMAKE_LINKER-NOTFOUND")
  if(CMAKE_LINKER)
    get_filename_component(linker ${CMAKE_LINKER} NAME)
  endif()
  # Construct a regex to match linker lines.  It must match both the
  # whole line and just the command (argv[0]).
  set(linker_regex "^( *|.*[/\\])(${linker}|ld|collect2)[^/\\]*( |$)")
  set(log "${log}  link line regex: [${linker_regex}]\n")
  string(REGEX REPLACE "\r?\n" ";" output_lines "${text}")
  foreach(line IN LISTS output_lines)
    set(cmd)
    if("${line}" MATCHES "${linker_regex}")
      if(UNIX)
        separate_arguments(args UNIX_COMMAND "${line}")
      else()
        separate_arguments(args WINDOWS_COMMAND "${line}")
      endif()
      list(GET args 0 cmd)
    endif()
    if("${cmd}" MATCHES "${linker_regex}")
      set(log "${log}  link line: [${line}]\n")
      string(REGEX REPLACE ";-([LYz]);" ";-\\1" args "${args}")
      foreach(arg IN LISTS args)
        if("${arg}" MATCHES "^-L(.:)?[/\\]")
          # Unix search path.
          string(REGEX REPLACE "^-L" "" dir "${arg}")
          list(APPEND implicit_dirs_tmp ${dir})
          set(log "${log}    arg [${arg}] ==> dir [${dir}]\n")
        elseif("${arg}" MATCHES "^-l[^:]")
          # Unix library.
          string(REGEX REPLACE "^-l" "" lib "${arg}")
          list(APPEND implicit_libs_tmp ${lib})
          set(log "${log}    arg [${arg}] ==> lib [${lib}]\n")
        elseif("${arg}" MATCHES "^(.:)?[/\\].*\\.a$")
          # Unix library full path.
          list(APPEND implicit_libs_tmp ${arg})
          set(log "${log}    arg [${arg}] ==> lib [${arg}]\n")
        elseif("${arg}" MATCHES "^-Y(P,)?")
          # Sun search path.
          string(REGEX REPLACE "^-Y(P,)?" "" dirs "${arg}")
          string(REPLACE ":" ";" dirs "${dirs}")
          list(APPEND implicit_dirs_tmp ${dirs})
          set(log "${log}    arg [${arg}] ==> dirs [${dirs}]\n")
        elseif("${arg}" MATCHES "^-l:")
          # HP named library.
          list(APPEND implicit_libs_tmp ${arg})
          set(log "${log}    arg [${arg}] ==> lib [${arg}]\n")
        elseif("${arg}" MATCHES "^-z(all|default|weak)extract")
          # Link editor option.
          list(APPEND implicit_libs_tmp ${arg})
          set(log "${log}    arg [${arg}] ==> opt [${arg}]\n")
        else()
          set(log "${log}    arg [${arg}] ==> ignore\n")
        endif()
      endforeach()
      break()
    elseif("${line}" MATCHES "LPATH(=| is:? )")
      set(log "${log}  LPATH line: [${line}]\n")
      # HP search path.
      string(REGEX REPLACE ".*LPATH(=| is:? *)" "" paths "${line}")
      string(REPLACE ":" ";" paths "${paths}")
      list(APPEND implicit_dirs_tmp ${paths})
      set(log "${log}    dirs [${paths}]\n")
    else()
      set(log "${log}  ignore line: [${line}]\n")
    endif()
  endforeach()

  # Cleanup list of libraries and flags.
  # We remove items that are not language-specific.
  set(implicit_libs "")
  foreach(lib IN LISTS implicit_libs_tmp)
    if("${lib}" MATCHES "^(crt.*\\.o|gcc.*|System.*)$")
      set(log "${log}  remove lib [${lib}]\n")
    else()
      list(APPEND implicit_libs "${lib}")
    endif()
  endforeach()

  # Cleanup list of directories.
  set(implicit_dirs "")
  foreach(d IN LISTS implicit_dirs_tmp)
    get_filename_component(dir "${d}" ABSOLUTE)
    list(APPEND implicit_dirs "${dir}")
    set(log "${log}  collapse dir [${d}] ==> [${dir}]\n")
  endforeach()
  list(REMOVE_DUPLICATES implicit_dirs)

  # Log results.
  set(log "${log}  implicit libs: [${implicit_libs}]\n")
  set(log "${log}  implicit dirs: [${implicit_dirs}]\n")

  # Return results.
  set(${lib_var} "${implicit_libs}" PARENT_SCOPE)
  set(${dir_var} "${implicit_dirs}" PARENT_SCOPE)
  set(${log_var} "${log}" PARENT_SCOPE)
endfunction()
