# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2012-2024 Symless Ltd.
# Copyright (C) 2009-2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

macro(configure_definitions)

  configure_meta()
  configure_ninja()
  configure_options()

  set(INTEG_TESTS_BIN integtests)
  set(UNIT_TESTS_BIN unittests)

  if(NOT "$ENV{GIT_SHA}" STREQUAL "")
    # Shorten the Git SHA to 8 chars for readability
    string(SUBSTRING "$ENV{GIT_SHA}" 0 8 GIT_SHA_SHORT)
    message(STATUS "Short Git SHA: ${GIT_SHA_SHORT}")
    add_definitions(-DGIT_SHA_SHORT="${GIT_SHA_SHORT}")
  endif()

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Disabling debug build")
    add_definitions(-DNDEBUG)
  endif()

  # TODO: find out why we need these, and remove them if we don't.
  if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
  endif()

  # TODO: explain why we're adding headers to sources.
  if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    set(ADD_HEADERS_TO_SOURCES FALSE)
  else()
    set(ADD_HEADERS_TO_SOURCES TRUE)
  endif()

  set(BIN_TEMP_DIR ${PROJECT_BINARY_DIR}/temp/bin)

endmacro()

macro(configure_meta)

  set(DESKFLOW_APP_ID
      "deskflow"
      CACHE STRING "ID of the app for filenames, etc")

  set(DESKFLOW_DOMAIN
      "deskflow.org"
      CACHE STRING "Domain of the app maintainer (not a URL)")

  set(DESKFLOW_APP_NAME
      "Deskflow"
      CACHE STRING "App name (used in GUI title bar, etc)")

  set(DESKFLOW_AUTHOR_NAME
      "Deskflow"
      CACHE STRING "Author name (also used as organization name)")

  set(DESKFLOW_MAINTAINER
      "Deskflow <maintainers@deskflow.org>"
      CACHE STRING "Maintainer email address in RFC 5322 mailbox format")

  set(DESKFLOW_WEBSITE_URL
      "https://deskflow.org"
      CACHE STRING "URL of the app website")

  set(DESKFLOW_VERSION_URL
      "https://api.deskflow.org/version"
      CACHE STRING "URL to get the latest version")

  set(DESKFLOW_HELP_TEXT
      "Report a bug"
      CACHE STRING "Text label for the help menu item")

  set(DESKFLOW_RES_DIR
      "${PROJECT_SOURCE_DIR}/res"
      CACHE STRING "Resource directory for images, etc")

  set(DESKFLOW_MAC_BUNDLE_CODE
      "DFLW"
      CACHE STRING "Mac bundle code (4 characters)")

  set(DESKFLOW_SHOW_DEV_THANKS
      true
      CACHE BOOL "Show developer thanks message")

  message(VERBOSE "App ID: ${DESKFLOW_APP_ID}")
  message(VERBOSE "App domain: ${DESKFLOW_DOMAIN}")
  message(VERBOSE "App name: ${DESKFLOW_APP_NAME}")
  message(VERBOSE "Author name: ${DESKFLOW_AUTHOR_NAME}")
  message(VERBOSE "Maintainer: ${DESKFLOW_MAINTAINER}")
  message(VERBOSE "Website URL: ${DESKFLOW_WEBSITE_URL}")
  message(VERBOSE "Version URL: ${DESKFLOW_VERSION_URL}")
  message(VERBOSE "Help text: ${DESKFLOW_HELP_TEXT}")
  message(VERBOSE "Res dir: ${DESKFLOW_RES_DIR}")
  message(VERBOSE "Mac bundle code: ${DESKFLOW_MAC_BUNDLE_CODE}")
  message(VERBOSE "Show dev thanks: ${DESKFLOW_SHOW_DEV_THANKS}")

  # TODO: We need to move this to configure_file() in the future, which is much cleaner.
  add_definitions(-DDESKFLOW_APP_ID="${DESKFLOW_APP_ID}")
  add_definitions(-DDESKFLOW_DOMAIN="${DESKFLOW_DOMAIN}")
  add_definitions(-DDESKFLOW_APP_NAME="${DESKFLOW_APP_NAME}")
  add_definitions(-DDESKFLOW_AUTHOR_NAME="${DESKFLOW_AUTHOR_NAME}")
  add_definitions(-DDESKFLOW_MAINTAINER="${DESKFLOW_MAINTAINER}")
  add_definitions(-DDESKFLOW_WEBSITE_URL="${DESKFLOW_WEBSITE_URL}")
  add_definitions(-DDESKFLOW_VERSION_URL="${DESKFLOW_VERSION_URL}")
  add_definitions(-DDESKFLOW_HELP_TEXT="${DESKFLOW_HELP_TEXT}")
  add_definitions(-DDESKFLOW_RES_DIR="${DESKFLOW_RES_DIR}")

  if(DESKFLOW_SHOW_DEV_THANKS)
    message(VERBOSE "Showing developer thanks message")
    add_definitions(-DDESKFLOW_SHOW_DEV_THANKS)
  else()
    message(VERBOSE "Not showing developer thanks message")
  endif()

  configure_bin_names()

endmacro()

macro(configure_bin_names)

  set(GUI_BINARY_NAME
      "deskflow"
      CACHE STRING "Filename of the GUI binary")

  set(SERVER_BINARY_NAME
      "deskflow-server"
      CACHE STRING "Filename of the server binary")

  set(CLIENT_BINARY_NAME
      "deskflow-client"
      CACHE STRING "Filename of the client binary")

  set(CORE_BINARY_NAME
      "deskflow-core"
      CACHE STRING "Filename of the core binary")

  set(DAEMON_BINARY_NAME
      "deskflow-daemon"
      CACHE STRING "Filename of the daemon binary")

  set(LEGACY_BINARY_NAME
      "deskflow-legacy"
      CACHE STRING "Filename of the legacy binary")

  message(VERBOSE "GUI binary: ${GUI_BINARY_NAME}")
  message(VERBOSE "Server binary: ${SERVER_BINARY_NAME}")
  message(VERBOSE "Client binary: ${CLIENT_BINARY_NAME}")
  message(VERBOSE "Core binary: ${CORE_BINARY_NAME}")
  message(VERBOSE "Daemon binary: ${DAEMON_BINARY_NAME}")
  message(VERBOSE "Legacy binary: ${LEGACY_BINARY_NAME}")

  add_definitions(-DGUI_BINARY_NAME="${GUI_BINARY_NAME}")
  add_definitions(-DSERVER_BINARY_NAME="${SERVER_BINARY_NAME}")
  add_definitions(-DCLIENT_BINARY_NAME="${CLIENT_BINARY_NAME}")
  add_definitions(-DCORE_BINARY_NAME="${CORE_BINARY_NAME}")
  add_definitions(-DDAEMON_BINARY_NAME="${DAEMON_BINARY_NAME}")
  add_definitions(-DLEGACY_BINARY_NAME="${LEGACY_BINARY_NAME}")

endmacro()

macro(configure_ninja)
  # use response files so that ninja can compile on windows, otherwise you get
  # an error when linking qt: "The input line is too long."
  set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
  set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
  set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_NINJA_FORCE_RESPONSE_FILE
      1
      CACHE INTERNAL "")
endmacro()

macro(configure_options)

  set(DEFAULT_BUILD_GUI ON)
  set(DEFAULT_BUILD_INSTALLER ON)
  set(DEFAULT_BUILD_TESTS ON)

  # unified binary is off by default for now, for backwards compatibility.
  set(DEFAULT_BUILD_UNIFIED OFF)

  # coverage is off by default because it's GCC only and a developer preference.
  set(DEFAULT_ENABLE_COVERAGE OFF)

  if("$ENV{DESKFLOW_BUILD_MINIMAL}" STREQUAL "true")
    set(DEFAULT_BUILD_GUI OFF)
    set(DEFAULT_BUILD_INSTALLER OFF)
  endif()

  if("$ENV{DESKFLOW_BUILD_TESTS}" STREQUAL "false")
    set(DEFAULT_BUILD_TESTS OFF)
  endif()

  if("$ENV{DESKFLOW_BUILD_UNIFIED}" STREQUAL "true")
    set(DEFAULT_BUILD_UNIFIED ON)
  endif()

  if("$ENV{DESKFLOW_ENABLE_COVERAGE}" STREQUAL "true")
    set(DEFAULT_ENABLE_COVERAGE ON)
  endif()

  option(BUILD_GUI "Build GUI" ${DEFAULT_BUILD_GUI})
  option(BUILD_INSTALLER "Build installer" ${DEFAULT_BUILD_INSTALLER})
  option(BUILD_TESTS "Build tests" ${DEFAULT_BUILD_TESTS})
  option(BUILD_UNIFIED "Build unified binary" ${DEFAULT_BUILD_UNIFIED})
  option(ENABLE_COVERAGE "Enable test coverage" ${DEFAULT_ENABLE_COVERAGE})

endmacro()
