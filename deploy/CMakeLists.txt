# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# Handle Pre install Items
if(WIN32)

  # Configure the windows version rc file
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/version.rc.in
    ${PROJECT_BINARY_DIR}/src/version.rc @ONLY
  )

endif()
