# SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_subdirectory(lib)
add_subdirectory(apps)

option(SKIP_BUILD_TESTS "Skip building and running unit tests" OFF)
option(BUILD_TESTS "Build tests" ON)
if(SKIP_BUILD_TESTS)
  set(BUILD_TESTS OFF)
endif()
if(BUILD_TESTS)
  add_subdirectory(unittests)
endif()

