# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

add_executable(CoreProcessTestChild CoreProcessTestChild.cpp)
target_link_libraries(CoreProcessTestChild PRIVATE Qt6::Core)
set_target_properties(CoreProcessTestChild PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

create_test(
  NAME CoreProcessTests
  DEPENDS gui
  SOURCE CoreProcessTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
add_dependencies(CoreProcessTests CoreProcessTestChild)

create_test(
  NAME NetworkMonitorTests
  DEPENDS gui
  SOURCE NetworkMonitorTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)
