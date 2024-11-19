# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd.
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

macro(configure_linux_package_name)
  # Get Distro name information
  execute_process(
    COMMAND bash "-c" "cat /etc/os-release | grep ^ID= | sed 's/ID=//g'"
    OUTPUT_VARIABLE _DISTRO_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\"" "" DISTRO_NAME "${_DISTRO_NAME}")
  message(STATUS "Distro Name: ${DISTRO_NAME}")

  execute_process(
    COMMAND bash "-c"
            "cat /etc/os-release | grep ^ID_LIKE= | sed 's/ID_LIKE=//g'"
    OUTPUT_VARIABLE _DISTRO_LIKE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\"" "" DISTRO_LIKE "${_DISTRO_LIKE}")
  message(STATUS "Distro Like: ${DISTRO_LIKE}")

  execute_process(
    COMMAND
      bash "-c"
      "cat /etc/os-release | grep ^VERSION_CODENAME= | sed 's/VERSION_CODENAME=//g'"
    OUTPUT_VARIABLE _DISTRO_CODENAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\"" "" DISTRO_CODENAME "${_DISTRO_CODENAME}")
  message(STATUS "Distro Codename: ${DISTRO_CODENAME}")

  execute_process(
    COMMAND bash "-c"
            "cat /etc/os-release | grep ^VERSION_ID= | sed 's/VERSION_ID=//g'"
    OUTPUT_VARIABLE _DISTRO_VERSION_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\"" "" DISTRO_VERSION_ID "${_DISTRO_VERSION_ID}")
  message(STATUS "Distro ID: ${DISTRO_VERSION_ID}")

  # Check if Debian-link
  string(REGEX MATCH debian|buntu DEBTYPE "${DISTRO_LIKE}")
  if((NOT ("${DEBTYPE}" STREQUAL "")) OR ("${DISTRO_NAME}" STREQUAL "debian"))
    set(CPACK_GENERATOR "DEB")
  endif()

  # Check if Rpm-like
  string(REGEX MATCH suse|fedora|rhel RPMTYPE "${DISTRO_LIKE}")
  string(REGEX MATCH fedora|suse|rhel RPMNAME "${DISTRO_NAME}")
  if((NOT ("${RPMTYPE}" STREQUAL "")) OR (NOT ("${RPMNAME}" STREQUAL "")))
      set(CPACK_GENERATOR "RPM")
  endif()

  # Disto specific name adjustments
  if("${DISTRO_NAME}" STREQUAL "opensuse-tumbleweed")
    set(DISTRO_NAME "opensuse")
    set(DISTRO_CODENAME "tumbleweed")
  elseif("${DISTRO_NAME}" STREQUAL "arch")
    # Arch linux is rolling the version id reported is the date of last iso.
    set(DISTRO_VERSION_ID "")
  endif()

  # Determain the code name to be used if any
  if(NOT "${DISTRO_VERSION_ID}" STREQUAL "")
    set(CN_STRING "${DISTRO_VERSION_ID}-")
  endif()

  if(NOT "${DISTRO_CODENAME}" STREQUAL "")
    set(CN_STRING "${DISTRO_CODENAME}-")
  endif()

  set(OS_STRING "${DISTRO_NAME}-${CN_STRING}${CMAKE_SYSTEM_PROCESSOR}")

endmacro()

#
# Same as the `configure_file` command but for directories recursively.
#
macro(configure_files srcDir destDir)

  message(VERBOSE "Configuring directory ${destDir}")
  make_directory(${destDir})

  file(
    GLOB_RECURSE sourceFiles
    RELATIVE ${srcDir}
    ${srcDir}/*)
  file(
    GLOB_RECURSE templateFiles
    LIST_DIRECTORIES false
    RELATIVE ${srcDir}
    ${srcDir}/*.in)
  list(REMOVE_ITEM sourceFiles ${templateFiles})

  foreach(sourceFile ${sourceFiles})
    set(sourceFilePath ${srcDir}/${sourceFile})
    if(IS_DIRECTORY ${sourceFilePath})
      message(VERBOSE "Copying directory ${sourceFile}")
      make_directory(${destDir}/${sourceFile})
    else()
      message(VERBOSE "Copying file ${sourceFile}")
      configure_file(${sourceFilePath} ${destDir}/${sourceFile} COPYONLY)
    endif()

  endforeach(sourceFile)

  foreach(templateFile ${templateFiles})

    set(sourceTemplateFilePath ${srcDir}/${templateFile})
    string(REGEX REPLACE "\.in$" "" templateFile ${templateFile})
    message(VERBOSE "Configuring file ${templateFile}")
    configure_file(${sourceTemplateFilePath} ${destDir}/${templateFile} @ONLY)

  endforeach(templateFile)

endmacro(configure_files)
