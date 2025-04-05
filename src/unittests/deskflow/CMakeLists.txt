# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME test_ArgParser
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE test_ArgParser.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

create_test(
  NAME test_Config
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE test_Config.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

create_test(
  NAME test_LanguageManager
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE test_LanguageManager.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

if(UNIX AND NOT APPLE)
  create_test(
    NAME test_X11LayoutParser
    DEPENDS app
    LIBS arch base ${extra_libs}
    SOURCE test_X11LayoutParser.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
  )
endif()

