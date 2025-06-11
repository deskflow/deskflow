# SPDX-FileCopyrightText: 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)
set(DOXYGEN_EXCLUDE_PATTERNS "*unittests/*")

# Files used to make our documents
doxygen_add_docs(dev-docs
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  COMMENT "Generating developer documentation" ALL)

# HACK Only these will show in your IDE
target_sources(dev-docs PRIVATE
  mainpage.md
  contributing.md
  build.md
  protocol_reference.md
)

# missing install target is intended generate a local copy
