# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd.
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

macro(configure_linux_package_name)
  # Get Distro name information
  if(EXISTS "/etc/os-release")
    FILE(STRINGS "/etc/os-release" RELEASE_FILE_CONTENTS)
  else()
    message(FATAL_ERROR "Unable to read file /etc/os-release")
  endif()

  foreach(LINE IN LISTS RELEASE_FILE_CONTENTS)
    if( "${LINE}" MATCHES "^ID=")
      string(REGEX REPLACE "^ID=" "" DISTRO_NAME ${LINE})
      string(REGEX REPLACE "\"" "" DISTRO_NAME ${DISTRO_NAME})
      message(DEBUG "Distro Name :${DISTRO_NAME}")
    elseif( "${LINE}" MATCHES "^ID_LIKE=")
      string(REGEX REPLACE "^ID_LIKE=" "" DISTRO_LIKE "${LINE}")
      string(REGEX REPLACE "\"" "" DISTRO_LIKE ${DISTRO_LIKE})
      message(DEBUG "Distro Like :${DISTRO_LIKE}")
    elseif( "${LINE}" MATCHES "^VERSION_CODENAME=")
      string(REGEX REPLACE "^VERSION_CODENAME=" "" DISTRO_CODENAME "${LINE}")
      string(REGEX REPLACE "\"" "" DISTRO_CODENAME "${DISTRO_CODENAME}")
      message(DEBUG "Distro Codename:${DISTRO_CODENAME}")
    elseif( "${LINE}" MATCHES "^VERSION_ID=")
      string(REGEX REPLACE "^VERSION_ID=" "" DISTRO_VERSION_ID "${LINE}")
      string(REGEX REPLACE "\"" "" DISTRO_VERSION_ID "${DISTRO_VERSION_ID}")
      message(DEBUG "Distro VersionID:${DISTRO_VERSION_ID}")
    endif()
  endforeach()

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

  if("${DISTRO_NAME}" STREQUAL "")
    set(DISTRO_NAME "linux")
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
