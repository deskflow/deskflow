# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2013 Synergy Si Ltd.
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

set(cryptopp_dir cryptopp562)

# only compile the crypto++ files we need.
set(cryptopp_src
	${cryptopp_dir}/3way.cpp
	${cryptopp_dir}/algparam.cpp
	${cryptopp_dir}/asn.cpp
	${cryptopp_dir}/authenc.cpp
	${cryptopp_dir}/basecode.cpp
	${cryptopp_dir}/cpu.cpp
	${cryptopp_dir}/cryptlib.cpp
	${cryptopp_dir}/des.cpp
	${cryptopp_dir}/dessp.cpp
	${cryptopp_dir}/dll.cpp
	${cryptopp_dir}/ec2n.cpp
	${cryptopp_dir}/ecp.cpp
	${cryptopp_dir}/filters.cpp
	${cryptopp_dir}/fips140.cpp
	${cryptopp_dir}/gcm.cpp
	${cryptopp_dir}/gf2n.cpp
	${cryptopp_dir}/gfpcrypt.cpp
	${cryptopp_dir}/hex.cpp
	${cryptopp_dir}/hmac.cpp
	${cryptopp_dir}/hrtimer.cpp
	${cryptopp_dir}/integer.cpp
	${cryptopp_dir}/iterhash.cpp
	${cryptopp_dir}/misc.cpp
	${cryptopp_dir}/modes.cpp
	${cryptopp_dir}/mqueue.cpp
	${cryptopp_dir}/nbtheory.cpp
	${cryptopp_dir}/oaep.cpp
	${cryptopp_dir}/osrng.cpp
	${cryptopp_dir}/pubkey.cpp
	${cryptopp_dir}/queue.cpp
	${cryptopp_dir}/randpool.cpp
	${cryptopp_dir}/rdtables.cpp
	${cryptopp_dir}/rijndael.cpp
	${cryptopp_dir}/rng.cpp
	${cryptopp_dir}/sha.cpp
)

# if 64-bit windows, compile asm file.
if (CMAKE_CL_64)
	list(APPEND cryptopp_src $(IntDir)x64dll.obj $(IntDir)x64masm.obj)

	add_custom_command(OUTPUT $(IntDir)x64dll.obj
		COMMAND ml64.exe /c /nologo /Fo$(IntDir)x64dll.obj /Zi
		"${CMAKE_CURRENT_SOURCE_DIR}/${cryptopp_dir}/x64dll.asm"
		MAIN_DEPENDENCY ${cryptopp_dir}/x64dll.asm
		VERBATIM)

	add_custom_command(OUTPUT $(IntDir)x64masm.obj
		COMMAND ml64.exe /c /nologo /Fo$(IntDir)x64masm.obj /Zi
		"${CMAKE_CURRENT_SOURCE_DIR}/${cryptopp_dir}/x64masm.asm"
		MAIN_DEPENDENCY ${cryptopp_dir}/x64masm.asm
		VERBATIM)
endif()

if (UNIX)
	add_definitions(-DCRYPTOPP_DISABLE_ASM)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -pipe")

	if (APPLE)
		if (DARWIN_VERSION GREATER 10)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-tautological-compare")
		endif()
	else()
		set(CRYPTOPP_ARCH "native")
		if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^arm.*")
			set(CRYPTOPP_ARCH "armv6zk")
		endif()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${CRYPTOPP_ARCH}")
	endif()
endif()

add_library(cryptopp STATIC ${cryptopp_src})

if (UNIX)
	# ignore warnings in crypto++
	set_target_properties(cryptopp PROPERTIES COMPILE_FLAGS "-w")
endif()
