# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

create_test(
  NAME ElectionStateTests
  DEPENDS coordination
  SOURCE ElectionStateTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

create_test(
  NAME CoordinationProtocolTests
  DEPENDS coordination
  SOURCE CoordinationProtocolTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

create_test(
  NAME FleetStateMergeTests
  DEPENDS coordination
  SOURCE FleetStateMergeTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

create_test(
  NAME CoordinatorFleetPublishTests
  DEPENDS coordination
  SOURCE CoordinatorFleetPublishTests.cpp
  LIBS base arch mt
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

create_test(
  NAME KeyboardRouterTests
  DEPENDS coordination
  SOURCE KeyboardRouterTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

create_test(
  NAME CoordinationLocalStatusTests
  DEPENDS common coordination
  SOURCE CoordinationLocalStatusTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
)

if(APPLE)
  create_test(
    NAME KeyboardRelayMapTests
    DEPENDS coordination
    SOURCE KeyboardRelayMapTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/coordination"
  )
endif()
