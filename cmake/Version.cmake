# Synergy -- mouse and keyboard sharing utility
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

    set (SYNERGY_VERSION $ENV{SYNERGY_VERSION})
    string(STRIP "${SYNERGY_VERSION}" SYNERGY_VERSION)

    if (NOT SYNERGY_VERSION)
        file(READ "${CMAKE_SOURCE_DIR}/VERSION" SYNERGY_VERSION)
        string(STRIP "${SYNERGY_VERSION}" SYNERGY_VERSION)
    endif()

    message (STATUS "Version number: " ${SYNERGY_VERSION})
    add_definitions (-DSYNERGY_VERSION="${SYNERGY_VERSION}")
    
    # Useful for copyright (e.g. in macOS bundle .plist.in and Windows version .rc file)
    string(TIMESTAMP SYNERGY_BUILD_YEAR "%Y")

    # MSI requires a 4-digit number and doesn't accept semver.
    if (WIN32)
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${SYNERGY_VERSION}")
        set(VERSION_MAJOR "${CMAKE_MATCH_1}")
        set(VERSION_MINOR "${CMAKE_MATCH_2}")
        set(VERSION_PATCH "${CMAKE_MATCH_3}")

        # Find the revision number, which is the number after the 'r'.
        string(REGEX MATCH "r([0-9]+)$" _ "${SYNERGY_VERSION}")
        set(VERSION_REVISION "${CMAKE_MATCH_1}")
        if (NOT VERSION_REVISION)
            set(VERSION_REVISION "0")
        endif()

        # Dot-separated version number for MSI and Windows version .rc file.
        set(SYNERGY_VERSION_MS "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_REVISION}")
        message (STATUS "Version number for Microsoft (dots): " ${SYNERGY_VERSION_MS})

        # CSV version number for Windows version .rc file.
        set(SYNERGY_VERSION_MS_CSV "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_REVISION}")
        message (STATUS "Version number for Microsoft (CSV): " ${SYNERGY_VERSION_MS_CSV})
    endif()

endmacro()
