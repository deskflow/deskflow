# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME SecureUtilsTests
  DEPENDS net
  LIBS base arch mt io ${extra_libs}
  SOURCE SecureUtilsTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/net"
)

create_test(
  NAME FingerprintDatabaseTests
  DEPENDS net
  LIBS base arch mt io ${extra_libs}
  SOURCE FingerprintDatabaseTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/net"
)


