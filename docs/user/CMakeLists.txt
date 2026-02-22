# SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)

# Files used to make our documents
# User facing documents will not include doxy comments in source code
doxygen_add_docs(user-docs ${CMAKE_CURRENT_SOURCE_DIR} COMMENT "Generating user documentation" ALL)

# HACK Only these will show in your IDE
target_sources(user-docs PRIVATE
  mainpage.md
  configuration.md
)

install(
  DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
  COMPONENT deskflow_user_docs
)
