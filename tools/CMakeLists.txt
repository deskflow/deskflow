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

file(GLOB cpp_ignore
	${cpp_dir}/simple.cpp
	${cpp_dir}/strciphr.cpp
	${cpp_dir}/polynomi.cpp
	${cpp_dir}/eprecomp.cpp
	${cpp_dir}/eccrypto.cpp
	${cpp_dir}/algebra.cpp)
list(REMOVE_ITEM cpp_src ${cpp_ignore})

# if 64-bit windows, compile asm file.
if (CMAKE_CL_64)
	list(APPEND cpp_src ${cpp_dir}/x64dll.asm ${cpp_dir}/x64masm.asm)
	
	add_custom_command(OUTPUT $(IntDir)x64dll.obj
		COMMAND ml64.exe /c /nologo /Fo$(IntDir)x64dll.obj /Zi 
		"${CMAKE_CURRENT_SOURCE_DIR}/${cpp_dir}/x64dll.asm"
		MAIN_DEPENDENCY ${cpp_dir}/x64dll.asm
		VERBATIM)
		
	add_custom_command(OUTPUT $(IntDir)x64masm.obj
		COMMAND ml64.exe /c /nologo /Fo$(IntDir)x64masm.obj /Zi 
		"${CMAKE_CURRENT_SOURCE_DIR}/${cpp_dir}/x64masm.asm"
		MAIN_DEPENDENCY ${cpp_dir}/x64masm.asm
		VERBATIM)
else()
	add_definitions(-DCRYPTOPP_DISABLE_ASM)
endif()

if (APPLE)
	# osx goes bat-shit crazy if we dont use these magical arguments.
	set(CMAKE_CXX_FLAGS "-O2 -DCRYPTOPP_DISABLE_ASM -pipe -Wno-tautological-compare")
endif()

add_library(cryptopp STATIC ${cpp_src})
