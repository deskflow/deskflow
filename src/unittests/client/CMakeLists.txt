# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version app mt net)
endif()

create_test(
  NAME HidConsumerTests
  DEPENDS client
  LIBS base arch ${extra_libs}
  SOURCE HidConsumerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/client"
)
