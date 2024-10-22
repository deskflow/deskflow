# Deskflow -- mouse and keyboard sharing utility
# Copyright (C) 2024 Symless Ltd.
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

set(LIBEI_MIN_VERSION 1.3)
set(LIBPORTAL_MIN_VERSION 0.8)

macro(configure_libs)

  set(libs)
  if(UNIX)
    configure_unix_libs()
  elseif(WIN32)
    configure_windows_libs()
  endif()

  configure_python()
  configure_qt()
  configure_openssl()
  configure_coverage()

endmacro()

#
# Unix (Mac, Linux, BSD, etc)
#
macro(configure_unix_libs)

  if(NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()

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
  check_function_exists(sigwait HAVE_POSIX_SIGWAIT)
  check_function_exists(strftime HAVE_STRFTIME)
  check_function_exists(vsnprintf HAVE_VSNPRINTF)
  check_function_exists(inet_aton HAVE_INET_ATON)

  # For some reason, the check_function_exists macro doesn't detect the
  # inet_aton on some pure Unix platforms (e.g. sunos5). So we need to do a more
  # detailed check and also include some extra libs.
  if(NOT HAVE_INET_ATON)
    set(CMAKE_REQUIRED_LIBRARIES nsl)

    check_c_source_compiles(
      "#include <arpa/inet.h>\n int main() { inet_aton (0, 0); }"
      HAVE_INET_ATON_ADV)

    set(CMAKE_REQUIRED_LIBRARIES)

    if(HAVE_INET_ATON_ADV)
      # Override the previous fail.
      set(HAVE_INET_ATON 1)

      # Assume that both nsl and socket will be needed, it seems safe to add
      # socket on the back of nsl, since socket only ever needed when nsl is
      # needed.
      list(APPEND libs nsl socket)
    endif()

  endif()

  check_type_size(char SIZEOF_CHAR)
  check_type_size(int SIZEOF_INT)
  check_type_size(long SIZEOF_LONG)
  check_type_size(short SIZEOF_SHORT)

  # pthread is used on both Linux and Mac
  check_library_exists("pthread" pthread_create "" HAVE_PTHREAD)
  if(HAVE_PTHREAD)
    list(APPEND libs pthread)
  else()
    message(FATAL_ERROR "Missing library: pthread")
  endif()

  if(APPLE)
    configure_mac_libs()
  else()

    configure_xorg_libs()
    configure_wayland_libs()

    find_package(pugixml REQUIRED)

    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(lib_glib REQUIRED IMPORTED_TARGET glib-2.0)
      pkg_search_module(PC_GDKPIXBUF gdk-pixbuf-2.0)

      include_directories(${PC_GDKPIXBUF_INCLUDE_DIRS})

      pkg_check_modules(lib_gdkpixbuf REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)
      pkg_check_modules(lib_notify REQUIRED IMPORTED_TARGET libnotify)

      add_definitions(-DHAVE_GDK_PIXBUF=1 -DHAVE_LIBNOTIFY=1)
    else()
      message(WARNING "pkg-config not found, skipping libnotify and gdk-pixbuf")
    endif()
  endif()

  # For config.h, set some static values; it may be a good idea to make these
  # values dynamic for non-standard UNIX compilers.
  set(ACCEPT_TYPE_ARG3 socklen_t)
  set(HAVE_CXX_BOOL 1)
  set(HAVE_CXX_CASTS 1)
  set(HAVE_CXX_EXCEPTIONS 1)
  set(HAVE_CXX_MUTABLE 1)
  set(HAVE_CXX_STDLIB 1)
  set(HAVE_PTHREAD_SIGNAL 1)
  set(SELECT_TYPE_ARG1 int)
  set(SELECT_TYPE_ARG234 " (fd_set *)")
  set(SELECT_TYPE_ARG5 " (struct timeval *)")
  set(STDC_HEADERS 1)
  set(TIME_WITH_SYS_TIME 1)
  set(HAVE_SOCKLEN_T 1)

  # Unix only: For config.h, save the results based on a template (config.h.in).
  # Note that this won't work on Windows because filenames are not case sensitive,
  # and we have header files named "Config.h" (upper case 'C').
  configure_file(${CMAKE_SOURCE_DIR}/src/lib/config.h.in
                 ${CMAKE_BINARY_DIR}/src/lib/config.h @ONLY)

  add_definitions(-DSYSAPI_UNIX=1 -DHAVE_CONFIG_H)

endmacro()

#
# Apple macOS
#
macro(configure_mac_libs)

  set(CMAKE_CXX_FLAGS
      "--sysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS} -DGTEST_USE_OWN_TR1_TUPLE=1"
  )

  find_library(lib_ScreenSaver ScreenSaver)
  find_library(lib_IOKit IOKit)
  find_library(lib_ApplicationServices ApplicationServices)
  find_library(lib_Foundation Foundation)
  find_library(lib_Carbon Carbon)

  list(
    APPEND
    libs
    ${lib_ScreenSaver}
    ${lib_IOKit}
    ${lib_ApplicationServices}
    ${lib_Foundation}
    ${lib_Carbon})

  find_library(lib_UserNotifications UserNotifications)
  list(APPEND libs ${lib_UserNotifications})

  add_definitions(-DWINAPI_CARBON=1 -D_THREAD_SAFE)

endmacro()

macro(configure_wayland_libs)

  include(FindPkgConfig)

  if(PKG_CONFIG_FOUND)
    configure_libei()
    configure_libportal()

    pkg_check_modules(LIBXKBCOMMON REQUIRED xkbcommon)
    pkg_check_modules(GLIB2 REQUIRED glib-2.0 gio-2.0)
    find_library(LIBM m)
    include_directories(${LIBXKBCOMMON_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS}
                        ${LIBM_INCLUDE_DIRS})
  else()
    message(WARNING "pkg-config not found, skipping wayland libraries")
  endif()

endmacro()

macro(configure_libei)
  option(SYSTEM_LIBEI "Use system libei" ON)
  if(SYSTEM_LIBEI)
    pkg_check_modules(LIBEI QUIET "libei-1.0 >= ${LIBEI_MIN_VERSION}")
    if(LIBEI_FOUND)
      message(STATUS "libei version: ${LIBEI_VERSION}")
      add_definitions(-DWINAPI_LIBEI=1)
      include_directories(${LIBEI_INCLUDE_DIRS})
    else()
      message(WARNING "libei >= ${LIBEI_MIN_VERSION} not found")
    endif()
  else()
    set(libei_bin_dir ${PROJECT_BINARY_DIR}/meson/subprojects/libei/src)
    set(libei_src_dir ${PROJECT_SOURCE_DIR}/subprojects/libei)
    find_library(
      LIBEI_LINK_LIBRARIES
      NAMES ei
      PATHS ${libei_bin_dir}
      NO_DEFAULT_PATH)
    if(LIBEI_LINK_LIBRARIES)
      message(STATUS "Using local subproject libei")
      set(LIBEI_FOUND true)
      add_definitions(-DWINAPI_LIBEI=1)
      set(LIBEI_INCLUDE_DIRS ${libei_src_dir}/src)
      include_directories(${LIBEI_INCLUDE_DIRS})
    else()
      message(WARNING "Local libei not found")
    endif()
  endif()
endmacro()

macro(configure_libportal)
  option(SYSTEM_LIBPORTAL "Use system libportal" ON)
  if(SYSTEM_LIBPORTAL)
    pkg_check_modules(LIBPORTAL QUIET "libportal >= ${LIBPORTAL_MIN_VERSION}")
    if(LIBPORTAL_FOUND)
      message(STATUS "libportal version: ${LIBPORTAL_VERSION}")
      check_libportal()
    else()
      message(WARNING "libportal >= ${LIBPORTAL_MIN_VERSION} not found")
    endif()
  else()
    set(libportal_bin_dir
        ${PROJECT_BINARY_DIR}/meson/subprojects/libportal/libportal)
    set(libportal_src_dir ${PROJECT_SOURCE_DIR}/subprojects/libportal)

    option(LIBPORTAL_STATIC "Use the static libportal binary" OFF)
    if(LIBPORTAL_STATIC)
      set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()

    find_library(
      LIBPORTAL_LINK_LIBRARIES
      NAMES portal
      PATHS ${libportal_bin_dir}
      NO_DEFAULT_PATH)

    if(LIBPORTAL_LINK_LIBRARIES)
      message(STATUS "Using local subproject libportal")
      set(LIBPORTAL_FOUND true)
      set(LIBPORTAL_INCLUDE_DIRS ${libportal_src_dir})

      message(STATUS "libportal library file: ${LIBPORTAL_LINK_LIBRARIES}")

      # HACK: Somehow `check_symbol_exists` doesn't pick up on the symbols even though
      # they are actually there. Since we use master branch of libportal, for now we'll
      # assume that the symbols are there.
      set(HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS true)
      set(HAVE_LIBPORTAL_CREATE_REMOTE_DESKTOP_SESSION_FULL true)
      set(HAVE_LIBPORTAL_INPUTCAPTURE true)
      set(HAVE_LIBPORTAL_OUTPUT_NONE true)
    else()
      message(WARNING "Local libportal not found")
    endif()
  endif()

  if(LIBPORTAL_FOUND)
    add_definitions(-DWINAPI_LIBPORTAL=1)
    include_directories(${LIBPORTAL_INCLUDE_DIRS})
  endif()

endmacro()

# libportal 0.7 has xdp_session_connect_to_eis but it doesn't have remote desktop session restore or
# the inputcapture code, so let's check for explicit functions that bits depending on what we have
macro(check_libportal)
  include(CMakePushCheckState)
  include(CheckCXXSourceCompiles)

  cmake_push_check_state(RESET)

  set(CMAKE_REQUIRED_INCLUDES
      "${CMAKE_REQUIRED_INCLUDES};${LIBPORTAL_INCLUDE_DIRS};${GLIB2_INCLUDE_DIRS}"
  )
  set(CMAKE_REQUIRED_LIBRARIES
      "${CMAKE_REQUIRED_LIBRARIES};${LIBPORTAL_LINK_LIBRARIES};${GLIB2_LINK_LIBRARIES}"
  )

  check_symbol_exists(xdp_session_connect_to_eis "libportal/portal.h"
                      HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS)

  check_symbol_exists(
    xdp_portal_create_remote_desktop_session_full "libportal/portal.h"
    HAVE_LIBPORTAL_CREATE_REMOTE_DESKTOP_SESSION_FULL)

  check_symbol_exists(xdp_input_capture_session_connect_to_eis
                      "libportal/inputcapture.h" HAVE_LIBPORTAL_INPUTCAPTURE)

  # check_symbol_exists can’t check for enum values
  check_cxx_source_compiles(
    "#include <libportal/portal.h>
        int main() { XdpOutputType out = XDP_OUTPUT_NONE; }
    " HAVE_LIBPORTAL_OUTPUT_NONE)

  cmake_pop_check_state()

  if(NOT HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS)
    message(WARNING "xdp_session_connect_to_eis not found")
  endif()

  if(NOT HAVE_LIBPORTAL_CREATE_REMOTE_DESKTOP_SESSION_FULL)
    message(WARNING "xdp_portal_create_remote_desktop_session_full not found")
  endif()

  if(NOT HAVE_LIBPORTAL_INPUTCAPTURE)
    message(WARNING "xdp_input_capture_session_connect_to_eis not found")
  endif()

  if(NOT HAVE_LIBPORTAL_OUTPUT_NONE)
    message(WARNING "XDP_OUTPUT_NONE not found")
  endif()

endmacro()

#
# X.org/X11 for Linux, BSD, etc
#
macro(configure_xorg_libs)

  # Set include dir for BSD-derived systems
  set(CMAKE_REQUIRED_INCLUDES "/usr/local/include")

  set(XKBlib "X11/Xlib.h;X11/XKBlib.h")
  set(CMAKE_EXTRA_INCLUDE_FILES "${XKBlib};X11/extensions/Xrandr.h")
  check_type_size("XRRNotifyEvent" X11_EXTENSIONS_XRANDR_H)
  set(HAVE_X11_EXTENSIONS_XRANDR_H "${X11_EXTENSIONS_XRANDR_H}")
  set(CMAKE_EXTRA_INCLUDE_FILES)

  check_include_files("${XKBlib};X11/extensions/dpms.h"
                      HAVE_X11_EXTENSIONS_DPMS_H)
  check_include_files("X11/extensions/Xinerama.h"
                      HAVE_X11_EXTENSIONS_XINERAMA_H)
  check_include_files("${XKBlib};X11/extensions/XKBstr.h"
                      HAVE_X11_EXTENSIONS_XKBSTR_H)
  check_include_files("X11/extensions/XKB.h" HAVE_XKB_EXTENSION)
  check_include_files("X11/extensions/XTest.h" HAVE_X11_EXTENSIONS_XTEST_H)
  check_include_files("${XKBlib}" HAVE_X11_XKBLIB_H)
  check_include_files("X11/extensions/XInput2.h" HAVE_XI2)

  if(HAVE_X11_EXTENSIONS_DPMS_H)
    # Assume that function prototypes declared, when include exists.
    set(HAVE_DPMS_PROTOTYPES 1)
  endif()

  if(NOT HAVE_X11_XKBLIB_H)
    message(FATAL_ERROR "Missing header: " ${XKBlib})
  endif()

  # Set library path and -L flag for BSD-derived systems.
  # On our FreeBSD CI, `link_directories` is also needed for some reason.
  set(CMAKE_LIBRARY_PATH "/usr/local/lib")
  set(CMAKE_REQUIRED_FLAGS "-L${CMAKE_LIBRARY_PATH}")
  link_directories(${CMAKE_LIBRARY_PATH})

  check_library_exists("SM;ICE" IceConnectionNumber "" HAVE_ICE)
  check_library_exists("Xext;X11" DPMSQueryExtension "" HAVE_Xext)
  check_library_exists("Xtst;Xext;X11" XTestQueryExtension "" HAVE_Xtst)
  check_library_exists("Xinerama" XineramaQueryExtension "" HAVE_Xinerama)
  check_library_exists("Xi" XISelectEvents "" HAVE_Xi)
  check_library_exists("Xrandr" XRRQueryExtension "" HAVE_Xrandr)

  if(HAVE_ICE)

    # Assume we have SM if we have ICE.
    set(HAVE_SM 1)
    list(APPEND libs SM ICE)

  endif()

  if(!X11_xkbfile_FOUND)
    message(FATAL_ERROR "Missing library: xkbfile")
  endif()

  if(HAVE_Xtst)

    # Xtxt depends on X11.
    set(HAVE_X11)
    list(
      APPEND
      libs
      Xtst
      X11
      xkbfile)

  else()

    message(FATAL_ERROR "Missing library: Xtst")

  endif()

  if(HAVE_Xext)
    list(APPEND libs Xext)
  endif()

  if(HAVE_Xinerama)
    list(APPEND libs Xinerama)
  else(HAVE_Xinerama)
    if(HAVE_X11_EXTENSIONS_XINERAMA_H)
      set(HAVE_X11_EXTENSIONS_XINERAMA_H 0)
      message(WARNING "Old Xinerama implementation detected, disabled")
    endif()
  endif()

  if(HAVE_Xrandr)
    list(APPEND libs Xrandr)
  endif()

  # this was outside of the linux scope, not sure why, moving it back inside.
  if(HAVE_Xi)
    list(APPEND libs Xi)
  endif()

  add_definitions(-DWINAPI_XWINDOWS=1)

endmacro()

#
# Windows
#
macro(configure_windows_libs)

  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} /MP /D _BIND_TO_CURRENT_VCLIBS_VERSION=1")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2")

  list(
    APPEND
    libs
    Wtsapi32
    Userenv
    Wininet
    comsuppw
    Shlwapi)

  add_definitions(
    /DWIN32
    /D_WINDOWS
    /D_CRT_SECURE_NO_WARNINGS
    /DDESKFLOW_VERSION=\"${DESKFLOW_VERSION}\"
    /D_XKEYCHECK_H)

  configure_file(${PROJECT_SOURCE_DIR}/res/win/version.rc.in
                 ${PROJECT_BINARY_DIR}/src/version.rc @ONLY)

  configure_openssl()

endmacro()

macro(configure_python)
  if(WIN32)
    find_package(Python REQUIRED QUIET)
  else()
    find_package(Python3 REQUIRED QUIET)
  endif()
endmacro()

macro(configure_qt)

  find_package(
    Qt6
    COMPONENTS Core Widgets Network
    REQUIRED)

  message(STATUS "Qt version: ${Qt6_VERSION}")

  set(GUI_RES_DIR ${DESKFLOW_RES_DIR}/gui)
  set(GUI_QRC_FILE ${GUI_RES_DIR}/app.qrc)

endmacro()

macro(configure_openssl)
  # Apple has to use static libraries because "Use of the Apple-provided OpenSSL
  # libraries by apps is strongly discouraged."
  # https://developer.apple.com/library/archive/documentation/Security/Conceptual/cryptoservices/SecureNetworkCommunicationAPIs/SecureNetworkCommunicationAPIs.html
  # TODO: How about bundling the OpenSSL .dylib files with the app so they can be updated?
  if(APPLE)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
  endif()

  find_package(OpenSSL 3.0 REQUIRED COMPONENTS SSL Crypto)
  if(WIN32) #Used for dev in TLS and WIX
    cmake_path(SET OPENSSL_ROOT_DIR NORMALIZE "${OPENSSL_INCLUDE_DIR}/..")
    message(VERBOSE "Set OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}")
    set(OPENSSL_EXE_DIR "${OPENSSL_ROOT_DIR}/tools/openssl")
    add_definitions(-DOPENSSL_EXE_DIR="${OPENSSL_EXE_DIR}")
  endif()

endmacro()

macro(configure_coverage)

  if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage")
    include(cmake/CodeCoverage.cmake)
    append_coverage_compiler_flags()
    set(test_exclude subprojects/* build/* src/test/*)
    set(test_src ${PROJECT_SOURCE_DIR}/src)

    # Apparently solves the bug in gcov where it returns negative counts and confuses gcovr.
    # > Got negative hit value in gcov line 'branch  2 taken -1' caused by a bug in gcov tool
    # Bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68080
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-update=atomic")

    setup_target_for_coverage_gcovr_xml(
      NAME
      coverage-${INTEG_TESTS_BIN}
      EXECUTABLE
      ${INTEG_TESTS_BIN}
      BASE_DIRECTORY
      ${test_src}
      EXCLUDE
      ${test_exclude})

    setup_target_for_coverage_gcovr_xml(
      NAME
      coverage-${UNIT_TESTS_BIN}
      EXECUTABLE
      ${UNIT_TESTS_BIN}
      BASE_DIRECTORY
      ${test_src}
      EXCLUDE
      ${test_exclude})

  else()
    message(STATUS "Code coverage is disabled")
  endif()
endmacro()
