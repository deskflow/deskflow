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

create_test(
  NAME ArchNetworkTests
  DEPENDS arch
  LIBS base Qt::Network ${extra_libs}
  SOURCE ArchNetworkTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/arch"
)

# A broken socket wakeup must fail the suite instead of hanging CI.
set_tests_properties(ArchNetworkTests PROPERTIES TIMEOUT 60)
