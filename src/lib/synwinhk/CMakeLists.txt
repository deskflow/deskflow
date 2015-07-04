# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2013 Synergy Si Ltd.
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

if (SYNERGY_ADD_HEADERS)
	list(APPEND sources ${headers})
endif()

include_directories(
	../
)

add_library(synwinhk SHARED ${sources})

# copy the dlls (and supporting files) from the lib dir to 
# the bin dir, so that synergyc and synergys can easily find them.
# we should leave the other libraries compiling to the lib dir,
# so that the bin dir remains tidy. the path is relative to the
# build dir (in this case, that's: build\src\lib\platform).
add_custom_command(
	TARGET synwinhk 
	POST_BUILD
	COMMAND xcopy /Y /Q
		..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\synwinhk.*
		..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\
)
