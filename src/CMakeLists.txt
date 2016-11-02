# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012-2016 Symless Ltd.
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

if (WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(OPENSSL_PLAT_DIR openssl-win64)
	else()
		set(OPENSSL_PLAT_DIR openssl-win32)
	endif()
	set(OPENSSL_INCLUDE ${CMAKE_SOURCE_DIR}/ext/${OPENSSL_PLAT_DIR}/inc32)
endif()

if (APPLE)
	set(OPENSSL_PLAT_DIR openssl-osx)
	set(OPENSSL_INCLUDE ${CMAKE_SOURCE_DIR}/ext/${OPENSSL_PLAT_DIR}/include)
endif()

if (WIN32)
	set(OPENSSL_LIBS
		${CMAKE_SOURCE_DIR}/ext/${OPENSSL_PLAT_DIR}/out32dll/libeay32.lib
		${CMAKE_SOURCE_DIR}/ext/${OPENSSL_PLAT_DIR}/out32dll/ssleay32.lib
	)
endif()

if (UNIX)
	if (APPLE)
		set(OPENSSL_LIBS
		${CMAKE_SOURCE_DIR}/ext/${OPENSSL_PLAT_DIR}/libssl.a
		${CMAKE_SOURCE_DIR}/ext/${OPENSSL_PLAT_DIR}/libcrypto.a
		)
	else()
		set(OPENSSL_LIBS ssl crypto)
	endif()
endif()

add_subdirectory(lib)
add_subdirectory(cmd)
add_subdirectory(micro)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "IRIX")
	add_subdirectory(test)
endif()
