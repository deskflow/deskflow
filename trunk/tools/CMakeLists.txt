# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2013 Bolton Software Ltd.
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

set(cpp_dir cryptopp562)

file(GLOB cpp_src ${cpp_dir}/*.cpp)

if (WIN32)
  file(GLOB cpp_hdr ${cpp_dir}/*.h)
	list(APPEND cpp_src ${cpp_hdr})
endif()

if (WIN32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-bit
	# do not compile crypto++ ASM on 64-bit windows, we just get
	# bunch of linker errors.
	set (CMAKE_CXX_FLAGS "/DCRYPTOPP_DISABLE_ASM /EHsc")
endif()
endif()

if (APPLE)
	# osx goes bat-shit crazy if we dont use these magical arguments.
	set (CMAKE_CXX_FLAGS "-O2 -DCRYPTOPP_DISABLE_ASM -pipe -Wno-tautological-compare")
endif()

add_library(cryptopp STATIC ${cpp_src})
