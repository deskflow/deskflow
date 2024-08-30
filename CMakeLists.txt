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

cmake_minimum_required(VERSION 3.5)
project(synergy C CXX)

include(cmake/Version.cmake)
include(cmake/Definitions.cmake)
include(cmake/Build.cmake)
include(cmake/Libraries.cmake)
include(cmake/Packaging.cmake)

set_version()
configure_definitions()
configure_libs()
configure_packaging()

add_subdirectory(src)

post_config_all()
