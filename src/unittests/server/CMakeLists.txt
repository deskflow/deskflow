# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version ${cli11_lib} ${tomlPP_lib} app mt net)
endif()

create_test(
  NAME test_ServerConfig
  DEPENDS server
  LIBS base arch ${extra_libs}
  SOURCE test_ServerConfig.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/server"
)

create_test(
  NAME test_Server
  DEPENDS server
  LIBS base arch ${extra_libs}
  SOURCE test_Server.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/server"
)

