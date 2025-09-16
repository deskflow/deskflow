# SPDX-FileCopyrightText: 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME ArgParserTests
  DEPENDS app
  LIBS arch base ${extra_libs}
  SOURCE ArgParserTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
)

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


if(UNIX AND NOT APPLE)
  #this test does not work properly on windows / mac os
  create_test(
    NAME ServerAppTests
    DEPENDS app
    LIBS arch base
    SOURCE ServerAppTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
  )

  create_test(
    NAME X11LayoutParserTests
    DEPENDS app
    LIBS arch base ${extra_libs}
    SOURCE X11LayoutParserTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/deskflow"
  )
endif()

