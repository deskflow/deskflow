# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Nick Bolton
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

set(inc
	wm_hooks.h
)

set(src
	wm_hooks.cxx
	wm_hooks.rc
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	..
)

include_directories(${inc})
add_library(vnc_wm_hooks SHARED ${src})

add_custom_command(
	TARGET vnc_wm_hooks 
	POST_BUILD
	COMMAND xcopy /Y /Q
		..\\..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\vnc_wm_hooks.*
		..\\..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\
)
