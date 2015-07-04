# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2011 Nick Bolton
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

include_directories(
	../../ext/gtest-1.6.0
	../../ext/gtest-1.6.0/include
	../../ext/gmock-1.6.0
	../../ext/gmock-1.6.0/include)
	
add_library(gtest STATIC ../../ext/gtest-1.6.0/src/gtest-all.cc)
add_library(gmock STATIC ../../ext/gmock-1.6.0/src/gmock-all.cc)

if (UNIX)
	# ignore warnings in gtest and gmock
	set_target_properties(gtest PROPERTIES COMPILE_FLAGS "-w")
	set_target_properties(gmock PROPERTIES COMPILE_FLAGS "-w")
endif()

add_subdirectory(integtests)
add_subdirectory(unittests)
