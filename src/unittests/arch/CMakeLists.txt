# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME IArchStringTests
  DEPENDS arch
  LIBS base ${extra_libs}
  SOURCE IArchStringTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/arch"
)
