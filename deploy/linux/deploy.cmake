# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# HACK This is set when the files is included so its the real path
# calling CMAKE_CURRENT_LIST_DIR after include would return the wrong scope var
set(MY_DIR ${CMAKE_CURRENT_LIST_DIR})

# Install our desktop file
install(
  FILES ${MY_DIR}/org.deskflow.deskflow.desktop
  DESTINATION share/applications
)

# Install our icon
install(
  FILES ${MY_DIR}/deskflow.png
  DESTINATION share/icons/hicolor/512x512/apps/
  RENAME org.deskflow.deskflow.png
)

# Install our metainfo
install(
  FILES ${MY_DIR}/org.deskflow.deskflow.metainfo.xml
  DESTINATION share/metainfo/
)

# Prepare PKGBUILD for Arch Linux
configure_file(
  ${MY_DIR}/arch/PKGBUILD.in
  ${CMAKE_BINARY_DIR}/PKGBUILD
  @ONLY
)


set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
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
  string(REGEX MATCH debian|deepin|uos DEBNAME "${DISTRO_NAME}")
  if((NOT ("${DEBTYPE}" STREQUAL "")) OR (NOT ("${DEBNAME}" STREQUAL "")))
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

elseif(${CMAKE_SYSTEM_NAME} MATCHES "|.*BSD")
  message(STATUS "BSD packaging not yet supported")
  set(OS_STRING ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
endif()
