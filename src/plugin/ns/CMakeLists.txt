# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2015 Synergy Si Ltd.
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
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

add_library(ns SHARED ${sources})

if (WIN32)
	add_custom_command(
		TARGET ns 
		POST_BUILD
		COMMAND xcopy /Y /Q
			..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\ns.dll
			..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\plugins\\
	)
else()
	add_custom_command(
		TARGET ns
		POST_BUILD
		COMMAND cp
			..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\ns.so
			..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\plugins\\
	)
endif()
