# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012-2016 Symless Ltd.
# Copyright (C) 2009 Nick Bolton
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
	${OPENSSL_INCLUDE}
)

if (UNIX)
	include_directories(
		../../..
	)
endif()

add_library(net STATIC ${sources})

if (WIN32)
	add_custom_command(
		TARGET net
		POST_BUILD
		COMMAND xcopy /Y /Q
			..\\..\\..\\..\\ext\\${OPENSSL_PLAT_DIR}\\out32dll\\libeay32.*
			..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}
	)
	add_custom_command(
		TARGET net 
		POST_BUILD
		COMMAND xcopy /Y /Q
			..\\..\\..\\..\\ext\\${OPENSSL_PLAT_DIR}\\out32dll\\ssleay32.*
			..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}
	)
endif()

if (UNIX)
	target_link_libraries(net mt io ${OPENSSL_LIBS})
endif()
