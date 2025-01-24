# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

include_directories(./lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)

add_subdirectory(lib)
add_subdirectory(apps)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
