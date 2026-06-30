# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
#
# Windows secure-desktop bridge: submits HID reports to deskflow-vhid.sys.

if(NOT WIN32)
  return()
endif()

set(target ${CMAKE_PROJECT_NAME}-vhid-bridge)

add_executable(${target} deskflow-vhid-bridge-win.cpp)

target_include_directories(${target} PRIVATE
  ${CMAKE_SOURCE_DIR}/src/driver/deskflow-vhid/public
)

target_link_libraries(${target} PRIVATE setupapi)

install(
  TARGETS ${target}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
