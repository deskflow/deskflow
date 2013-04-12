# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Bolton Software Ltd.
# Copyright (C) 2009 Nick Bolton
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

# Version number for Synergy
set(VERSION_MAJOR 1)
set(VERSION_MINOR 4)
set(VERSION_REV 12)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}")

# The check for 2.6 may be too strict (consider lowering).
cmake_minimum_required(VERSION 2.4.7)

# CMake complains if we don't have this.
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

# We're escaping quotes in the Windows version number, because
# for some reason CMake won't do it at config version 2.4.7
# It seems that this restores the newer behaviour where define
# args are not auto-escaped.
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0005 NEW)
endif()

# First, declare project (important for prerequisite checks).
project(synergy C CXX)

# put binaries in a different dir to make them easier to find.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# for unix, put debug files in a separate bin "debug" dir.
# release bin files should stay in the root of the bin dir.
if (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/debug)
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/debug)
  endif()
endif()

# Set some easy to type variables.
set(root_dir ${CMAKE_SOURCE_DIR})
set(cmake_dir ${root_dir}/res)
set(bin_dir ${root_dir}/bin)
set(doc_dir ${root_dir}/doc)
set(doc_dir ${root_dir}/doc)

# Declare libs, so we can use list in linker later. There's probably 
# a more elegant way of doing this; with SCons, when you check for the
# lib, it is automatically passed to the linker.
set(libs)

# Depending on the platform, pass in the required defines.
if (UNIX)

	# For config.h, detect the libraries, functions, etc.
	include(CheckIncludeFiles)
	include(CheckLibraryExists)
	include(CheckFunctionExists)
	include(CheckTypeSize)
	include(CheckIncludeFileCXX)
	include(CheckSymbolExists)
	include(CheckCSourceCompiles)

	check_include_file_cxx(istream HAVE_ISTREAM)
	check_include_file_cxx(ostream HAVE_OSTREAM)
	check_include_file_cxx(sstream HAVE_SSTREAM)

	check_include_files(inttypes.h HAVE_INTTYPES_H)
	check_include_files(locale.h HAVE_LOCALE_H)
	check_include_files(memory.h HAVE_MEMORY_H)
	check_include_files(stdlib.h HAVE_STDLIB_H)
	check_include_files(strings.h HAVE_STRINGS_H)
	check_include_files(string.h HAVE_STRING_H)
	check_include_files(sys/select.h HAVE_SYS_SELECT_H)
	check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
	check_include_files(sys/stat.h HAVE_SYS_STAT_H)
	check_include_files(sys/time.h HAVE_SYS_TIME_H)
	check_include_files(sys/utsname.h HAVE_SYS_UTSNAME_H)
	check_include_files(unistd.h HAVE_UNISTD_H)
	check_include_files(wchar.h HAVE_WCHAR_H)

	check_function_exists(getpwuid_r HAVE_GETPWUID_R)
	check_function_exists(gmtime_r HAVE_GMTIME_R)
	check_function_exists(nanosleep HAVE_NANOSLEEP)
	check_function_exists(poll HAVE_POLL)
	check_function_exists(sigwait HAVE_POSIX_SIGWAIT)
	check_function_exists(strftime HAVE_STRFTIME)
	check_function_exists(vsnprintf HAVE_VSNPRINTF)
	check_function_exists(inet_aton HAVE_INET_ATON)
	
	# For some reason, the check_function_exists macro doesn't detect
	# the inet_aton on some pure Unix platforms (e.g. sunos5). So we
	# need to do a more detailed check and also include some extra libs.
	if (NOT HAVE_INET_ATON)

		set(CMAKE_REQUIRED_LIBRARIES nsl)
		check_c_source_compiles(
			"#include <arpa/inet.h>\n int main() { inet_aton(0, 0); }" 
			HAVE_INET_ATON_ADV)
		set(CMAKE_REQUIRED_LIBRARIES)

		if (HAVE_INET_ATON_ADV)
			
			# Override the previous fail.
			set(HAVE_INET_ATON 1)

			# Assume that both nsl and socket will be needed,
			# it seems safe to add socket on the back of nsl,
			# since socket only ever needed when nsl is needed.
			list(APPEND libs nsl socket)

		endif()

	endif()

	check_type_size(char SIZEOF_CHAR)
	check_type_size(int SIZEOF_INT)
	check_type_size(long SIZEOF_LONG)
	check_type_size(short SIZEOF_SHORT)

	# pthread is used on both Linux and Mac
	check_library_exists("pthread" pthread_create "" HAVE_PTHREAD)
	if (HAVE_PTHREAD)
		list(APPEND libs pthread)
	else (HAVE_PTHREAD)
		message(FATAL_ERROR "Missing library: pthread")
	endif()
	
	if (APPLE)
		exec_program(uname ARGS -v OUTPUT_VARIABLE DARWIN_VERSION)
		string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
		message(STATUS "DARWIN_VERSION=${DARWIN_VERSION}")
		if (DARWIN_VERSION LESS 9)
			# 10.4: universal (32-bit intel and power pc) 
			set(CMAKE_OSX_ARCHITECTURES "ppc;i386"
				CACHE STRING "" FORCE)
		else()
			# 10.5+: 32-bit only -- missing funcs in 64-bit os libs
			# such as GetGlobalMouse.
			set(CMAKE_OSX_ARCHITECTURES "i386"
				CACHE STRING "" FORCE)
		endif()
		
		set(CMAKE_CXX_FLAGS "--sysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS}")

		find_library(lib_ScreenSaver ScreenSaver)
		find_library(lib_IOKit IOKit)
		find_library(lib_ApplicationServices ApplicationServices)
		find_library(lib_Foundation Foundation)
		find_library(lib_Carbon Carbon)

		list(APPEND libs
			${lib_ScreenSaver}
			${lib_IOKit}
			${lib_ApplicationServices}
			${lib_Foundation}
			${lib_Carbon}
		)

	else()

		# add include dir for bsd (posix uses /usr/include/)
		set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}:/usr/local/include")

		set(XKBlib "X11/Xlib.h;X11/XKBlib.h")
		check_symbol_exists("XRRNotifyEvent" "${XKBlib};X11/extensions/Xrandr.h" HAVE_X11_EXTENSIONS_XRANDR_H)

		check_include_files("${XKBlib};X11/extensions/dpms.h" HAVE_X11_EXTENSIONS_DPMS_H)
		check_include_files("X11/extensions/Xinerama.h" HAVE_X11_EXTENSIONS_XINERAMA_H)
		check_include_files("${XKBlib};X11/extensions/XKBstr.h" HAVE_X11_EXTENSIONS_XKBSTR_H)
		check_include_files("X11/extensions/XKB.h" HAVE_XKB_EXTENSION)
		check_include_files("X11/extensions/XTest.h" HAVE_X11_EXTENSIONS_XTEST_H)
		check_include_files("${XKBlib}" HAVE_X11_XKBLIB_H)
		check_include_files("X11/extensions/XInput2.h" HAVE_XI2)

		if (HAVE_X11_EXTENSIONS_DPMS_H)
			# Assume that function prototypes declared, when include exists.
			set(HAVE_DPMS_PROTOTYPES 1)
		endif()

		if (NOT HAVE_X11_XKBLIB_H)
			message(FATAL_ERROR "Missing header: " ${XKBlib})
		endif()

		check_library_exists("SM;ICE" IceConnectionNumber "" HAVE_ICE)
		check_library_exists("Xext;X11" DPMSQueryExtension "" HAVE_Xext)
		check_library_exists("Xtst;Xext;X11" XTestQueryExtension "" HAVE_Xtst)
		check_library_exists("Xinerama" XineramaQueryExtension "" HAVE_Xinerama)
		check_library_exists("Xi" XISelectEvents "" HAVE_Xi)
		check_library_exists("Xrandr" XRRQueryExtension "" HAVE_Xrandr)

		if (HAVE_ICE)

			# Assume we have SM if we have ICE.
			set(HAVE_SM 1)
			list(APPEND libs SM ICE)

		endif()

		if (HAVE_Xtst)

			# Xtxt depends on X11.
			set(HAVE_X11)
			list(APPEND libs Xtst X11)

		else()

			message(FATAL_ERROR "Missing library: Xtst")

		endif()

		if (HAVE_Xext)
			list(APPEND libs Xext)
		endif()

		if (HAVE_Xinerama)
			list(APPEND libs Xinerama)
		else (HAVE_Xinerama)
			if (HAVE_X11_EXTENSIONS_XINERAMA_H)
				set(HAVE_X11_EXTENSIONS_XINERAMA_H 0)
				message(WARNING "Old Xinerama implementation detected, disabled")
			endif()
		endif()

		if (HAVE_Xrandr)
			list(APPEND libs Xrandr)
		endif()

	endif()

        IF(HAVE_Xi)
                LIST(APPEND libs Xi)
        ENDIF()

	# For config.h, set some static values; it may be a good idea to make
	# these values dynamic for non-standard UNIX compilers.
	set(ACCEPT_TYPE_ARG3 socklen_t)
	set(HAVE_CXX_BOOL 1)
	set(HAVE_CXX_CASTS 1)
	set(HAVE_CXX_EXCEPTIONS 1)
	set(HAVE_CXX_MUTABLE 1)
	set(HAVE_CXX_STDLIB 1)
	set(HAVE_PTHREAD_SIGNAL 1)
	set(SELECT_TYPE_ARG1 int)
	set(SELECT_TYPE_ARG234 "(fd_set *)")
	set(SELECT_TYPE_ARG5 "(struct timeval *)")
	set(STDC_HEADERS 1)
	set(TIME_WITH_SYS_TIME 1)
	set(HAVE_SOCKLEN_T 1)

	# For config.h, save the results based on a template (config.h.in).
	configure_file(res/config.h.in ${root_dir}/config.h)

	add_definitions(-DSYSAPI_UNIX=1 -DHAVE_CONFIG_H)

	if (APPLE)
		add_definitions(-DWINAPI_CARBON=1 -D_THREAD_SAFE)
	else (APPLE)
		add_definitions(-DWINAPI_XWINDOWS=1)
	endif()

else (UNIX)

	list(APPEND libs Wtsapi32 Userenv)

	add_definitions(
		/DWIN32
		/D_WINDOWS
		/D_CRT_SECURE_NO_WARNINGS
		/DVERSION=\"${VERSION}\"
	)
	
	if (MSVC_VERSION EQUAL 1600)
		set(SLN_FILENAME "${CMAKE_CURRENT_BINARY_DIR}/synergy.sln")
		if (EXISTS "${SLN_FILENAME}" )
			file(APPEND "${SLN_FILENAME}" "\n# This should be regenerated!\n")
		endif()
	endif()

endif()

if (GAME_DEVICE_SUPPORT)
	add_definitions(-DGAME_DEVICE_SUPPORT)
endif()

if (VNC_SUPPORT)
	add_definitions(-DVNC_SUPPORT)
endif()

add_subdirectory(src)
add_subdirectory(tools)

if (WIN32)
	# add /analyze in order to unconver potential bugs in the source code
	# Details: http://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx
	# add /FR to generate browse information (ncb files) usefull for using IDE

	#define _BIND_TO_CURRENT_CRT_VERSION 1
	#define _BIND_TO_CURRENT_ATL_VERSION 1
	#define _BIND_TO_CURRENT_MFC_VERSION 1
	#define _BIND_TO_CURRENT_OPENMP_VERSION 1
	# next line replaced the previous 4 ones:
	#define _BIND_TO_CURRENT_VCLIBS_VERSION 1;

	# compiler: /MP - use multi cores to compile
	# added _SECURE_SCL=1 for finding bugs with iterators - http://msdn.microsoft.com/en-us/library/aa985965.aspx
	
	# common args between all vs builds
	set(VS_ARGS "/FR /MP /D _BIND_TO_CURRENT_VCLIBS_VERSION=1 /D _SECURE_SCL=1 ${VS_ARGS_EXTRA}")
	
	# we may use `cmake -D VS_ARGS_EXTRA="/analyze"` for example to specify
	# analyze mode (since we don't always want to use it; e.g. on non-team 
	# or non-x86 compiler editions where there's no support)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${VS_ARGS}")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${VS_ARGS}")

	# this line removes "/D NDEBUG" from release, we want them in order to
	# find bugs even on release builds.
	set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2")

endif()

if (CONF_CPACK)

	if (WIN32)
	   message(FATAL_ERROR "CPack support for Windows has been removed.")
	endif()

	if (UNIX)
		if (APPLE)
			message(FATAL_ERROR "CPack support for Apple has been removed.")
		else ()
			install(FILES bin/synergy
				DESTINATION bin
				PERMISSIONS
				OWNER_READ OWNER_WRITE OWNER_EXECUTE
				GROUP_READ GROUP_EXECUTE
				WORLD_READ WORLD_EXECUTE)

			# install gnome menu item
			install(FILES res/synergy.desktop
				DESTINATION share/applications)
			install(FILES res/synergy.ico
				DESTINATION share/icons)
		endif()
	endif()

	# The default CPack behaviour is not to append the system processor
	# type, which is undesirable in our case, since we want to support 
	# both 32-bit and 64-bit processors.
	set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

	set(CPACK_PACKAGE_NAME "synergy")
	set(CPACK_PACKAGE_VENDOR "The Synergy Project")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Synergy server and client")
	set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
	set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
	set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_REV})
	set(CPACK_PACKAGE_VERSION ${VERSION})
	set(CPACK_PACKAGE_CONTACT http://synergy-foss.org/)
	set(CPACK_RESOURCE_FILE_LICENSE "${cmake_dir}/License.rtf")
	set(CPACK_RESOURCE_FILE_README "${cmake_dir}/Readme.txt")

	# Must be last (since it relies of CPACK_ vars).
	include(CPack)

endif()

if (CONF_DOXYGEN)

	set(VERSION, "${VERSION}")

	# For doxygen.cfg, save the results based on a template (doxygen.cfg.in).
	configure_file(${cmake_dir}/doxygen.cfg.in ${doc_dir}/doxygen.cfg)

endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "IRIX")
	set_target_properties(synergys PROPERTIES LINK_FLAGS "-all -woff 33 -woff 84 -woff 15")
	set_target_properties(synergyc PROPERTIES LINK_FLAGS "-all -woff 33 -woff 84 -woff 15")
	set_target_properties(synergyd PROPERTIES LINK_FLAGS "-all -woff 33 -woff 84 -woff 15")
endif()

