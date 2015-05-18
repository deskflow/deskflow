# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2015 Alexander von Gluck IV
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

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

include_directories(../../micro)

# System headers
include_directories(/boot/system/develop/headers/private/storage/)
include_directories(/boot/system/develop/headers/private/input/)
include_directories(/boot/system/develop/headers/private/shared/)
include_directories(/boot/system/develop/headers/private/interface/)

add_library(bsynergy SHARED ${sources})
target_link_libraries(bsynergy micro be network shared translation)
