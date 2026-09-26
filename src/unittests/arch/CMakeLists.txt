# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME ArchTests
  DEPENDS arch
  LIBS base ${extra_libs}
  SOURCE ArchTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/arch"
)
