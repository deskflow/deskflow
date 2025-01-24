# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# Generic Package Items
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT ${CMAKE_PROJECT_CONTACT})
set(CPACK_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR ${CMAKE_PROJECT_VENDOR})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSES/GPL-2.0-only.txt)

set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

#Prevent this override from being written in the package
if(NOT PACKAGE_VERSION_LABEL)
    set (PACKAGE_VERSION_LABEL "${CPACK_PACKAGE_VERSION}")
endif()

if(WIN32)
  include(windows/deploy.cmake)
elseif(UNIX AND NOT APPLE)
  include(linux/deploy.cmake)
elseif(APPLE)
  include(mac/deploy.cmake)
else()
  message(STATUS "UNKNOWN System: ${CMAKE_SYSTEM_NAME}")
endif()

# Always use "deskflow" for start of name
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION_LABEL}-${OS_STRING}")
message(STATUS "Package Basename: ${CPACK_PACKAGE_FILE_NAME}")

include(CPack)
