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

# Either get the version number from the environment or from the VERSION file.
# On Windows, we also set a special 4-digit MSI version number.
macro(set_version)

  set(DESKFLOW_VERSION $ENV{DESKFLOW_VERSION})
  string(STRIP "${DESKFLOW_VERSION}" DESKFLOW_VERSION)

  if(NOT DESKFLOW_VERSION)
    file(READ "${PROJECT_SOURCE_DIR}/VERSION" DESKFLOW_VERSION)
    string(STRIP "${DESKFLOW_VERSION}" DESKFLOW_VERSION)
  endif()

  message(STATUS "Version number (semver): " ${DESKFLOW_VERSION})
  add_definitions(-DDESKFLOW_VERSION="${DESKFLOW_VERSION}")

  # Useful for copyright (e.g. in macOS bundle .plist.in and Windows version .rc
  # file)
  string(TIMESTAMP DESKFLOW_BUILD_YEAR "%Y")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_windows_version()
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set_linux_version()
  endif()

endmacro()

macro(set_four_part_version)

  string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${DESKFLOW_VERSION}")
  set(VERSION_MAJOR "${CMAKE_MATCH_1}")
  set(VERSION_MINOR "${CMAKE_MATCH_2}")
  set(VERSION_PATCH "${CMAKE_MATCH_3}")

  # Find the revision number, which is the number after the 'r'.
  string(REGEX MATCH "r([0-9]+)$" _ "${DESKFLOW_VERSION}")
  set(VERSION_REVISION "${CMAKE_MATCH_1}")
  if(NOT VERSION_REVISION)
    set(VERSION_REVISION "0")
  endif()

  set(DESKFLOW_VERSION_FOUR_PART
      "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_REVISION}")

endmacro()

# MSI requires a 4-digit number and doesn't accept semver.
macro(set_windows_version)

  set_four_part_version()

  # Dot-separated version number for MSI and Windows version .rc file.
  set(DESKFLOW_VERSION_MS ${DESKFLOW_VERSION_FOUR_PART})
  message(VERBOSE "Version number for (Microsoft 4-part): "
          ${DESKFLOW_VERSION_MS})

  # CSV version number for Windows version .rc file.
  set(DESKFLOW_VERSION_MS_CSV
      "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_REVISION}")
  message(VERBOSE "Version number for (Microsoft CSV): "
          ${DESKFLOW_VERSION_MS_CSV})
endmacro()

macro(set_linux_version)

  # Replace the first occurrence of '-' with '~' for Linux versioning; the '-'
  # char is reserved for use at at the end of the version string to indicate a
  # package revision. Debian has always used this convention, but support for
  # this was also introduced in RPM 4.10.0.
  string(REGEX REPLACE "-" "~" DESKFLOW_VERSION_LINUX "${DESKFLOW_VERSION}")
  message(STATUS "Version number (DEB/RPM): ${DESKFLOW_VERSION_LINUX}")

  # Arch does not support SemVer or DEB/RPM version format, so use the four-part
  # version format which funnily enough is what Microsoft requires for MSI.
  set_four_part_version()
  message(STATUS "Version number (4-part): ${DESKFLOW_VERSION_FOUR_PART}")

endmacro()
