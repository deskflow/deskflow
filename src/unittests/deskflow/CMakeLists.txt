# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

# Deliberately independent of Qt and platform input APIs; also runnable with a
# C++20 compiler before installing the GUI build dependencies.
add_executable(SharedInputLockTests SharedInputLockTests.cpp)
target_include_directories(SharedInputLockTests PRIVATE ${PROJECT_SOURCE_DIR}/src/lib)
add_test(NAME SharedInputLockTests COMMAND SharedInputLockTests)
set_property(GLOBAL APPEND PROPERTY ${CMAKE_PROJECT_NAME}_tests SharedInputLockTests)

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME ClipboardTests
  DEPENDS app
  LIBS arch base io ${extra_libs}
  SOURCE ClipboardTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

create_test(
  NAME ClipboardChunksTests
  DEPENDS app
  LIBS arch base io ${extra_libs}
  SOURCE ClipboardChunksTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

create_test(
  NAME IKeyStateTests
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE IKeyStateTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

create_test(
  NAME KeyMapTests
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE KeyMapTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

create_test(
  NAME LanguageManagerTests
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE LanguageManagerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

if(BUILD_X11_SUPPORT)
  create_test(
    NAME X11LayoutParserTests
    DEPENDS app
    LIBS arch base ${extra_libs}
    SOURCE X11LayoutParserTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
  )
endif()

