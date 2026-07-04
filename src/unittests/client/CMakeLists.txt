# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs platform version)
endif()

create_test(
  NAME ServerProxyTests
  DEPENDS client
  LIBS arch base app io ${extra_libs}
  SOURCE ServerProxyTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/client"
)
