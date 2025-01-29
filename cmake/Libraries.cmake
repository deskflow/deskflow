# SPDX-FileCopyrightText: 2024 - 2025 Deskflow Developers
# SPDX-FileCopyrightText: 2024 Symless Ltd
# SPDX-License-Identifier: MIT

macro(configure_libs)

  set(libs)
  if(UNIX)
    configure_unix_libs()
  elseif(WIN32)
    find_package(Python REQUIRED QUIET)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D _BIND_TO_CURRENT_VCLIBS_VERSION=1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2")
    list(APPEND libs Wtsapi32 Userenv Wininet comsuppw Shlwapi)
    add_definitions(
      /DWIN32
      /D_WINDOWS
      /D_CRT_SECURE_NO_WARNINGS
      /D_XKEYCHECK_H
    )
  endif()

  find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Widgets Network)

  # Define the location of Qt deployment tool
  if(WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT VCPKG_INSTALL_DIR STREQUAL "")
      find_program(DEPLOYQT windeployqt.debug.bat)
    else()
      find_program(DEPLOYQT windeployqt)
    endif()
  elseif(APPLE)
      find_program(DEPLOYQT macdeployqt)
  endif()

  message(STATUS "Qt version: ${Qt6_VERSION}")

  # TODO SSL check can happen in lib/net when don't have to deploy it any longer on windows

  # Apple has to use static libraries because "Use of the Apple-provided OpenSSL
  # libraries by apps is strongly discouraged."
  # https://developer.apple.com/library/archive/documentation/Security/Conceptual/cryptoservices/SecureNetworkCommunicationAPIs/SecureNetworkCommunicationAPIs.html
  if(APPLE)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
  endif()

  find_package(OpenSSL ${REQUIRED_OPENSSL_VERSION} REQUIRED COMPONENTS SSL Crypto)

  option(ENABLE_COVERAGE "Enable test coverage" OFF)
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
      NAME coverage-integtests
      EXECUTABLE integtests
      BASE_DIRECTORY ${test_src}
      EXCLUDE ${test_exclude}
    )

    setup_target_for_coverage_gcovr_xml(
      NAME coverage-unittests
      EXECUTABLE unittests
      BASE_DIRECTORY ${test_src}
      EXCLUDE ${test_exclude}
    )
  endif()

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

  check_include_files(locale.h HAVE_LOCALE_H)
  check_include_files(sys/select.h HAVE_SYS_SELECT_H)
  check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
  check_include_files(sys/time.h HAVE_SYS_TIME_H)
  check_include_files(sys/utsname.h HAVE_SYS_UTSNAME_H)
  check_include_files(unistd.h HAVE_UNISTD_H)
  check_include_files(wchar.h HAVE_WCHAR_H)

  check_function_exists(getpwuid_r HAVE_GETPWUID_R)
  check_function_exists(gmtime_r HAVE_GMTIME_R)
  check_function_exists(nanosleep HAVE_NANOSLEEP)
  check_function_exists(sigwait HAVE_POSIX_SIGWAIT)
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

  # pthread is used on both Linux and Mac
  check_library_exists("pthread" pthread_create "" HAVE_PTHREAD)
  if(HAVE_PTHREAD)
    list(APPEND libs pthread)
  else()
    message(FATAL_ERROR "Missing library: pthread")
  endif()

  if(APPLE)
    set(CMAKE_CXX_FLAGS "--sysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS} -DGTEST_USE_OWN_TR1_TUPLE=1")
    find_library(lib_ScreenSaver ScreenSaver)
    find_library(lib_IOKit IOKit)
    find_library(lib_ApplicationServices ApplicationServices)
    find_library(lib_Foundation Foundation)
    find_library(lib_Carbon Carbon)
    find_library(lib_UserNotifications UserNotifications)
    list(APPEND libs
      ${lib_ScreenSaver} ${lib_IOKit} ${lib_ApplicationServices}
      ${lib_Foundation} ${lib_Carbon} ${lib_UserNotifications}
    )

    add_definitions(-DWINAPI_CARBON=1 -D_THREAD_SAFE)
  else()

    configure_xorg_libs()

    include(FindPkgConfig)

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(LIBXKBCOMMON REQUIRED xkbcommon)
      pkg_check_modules(GLIB2 REQUIRED glib-2.0 gio-2.0)
      find_library(LIBM m)
      include_directories(${LIBXKBCOMMON_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS}
                          ${LIBM_INCLUDE_DIRS})
    else()
      message(WARNING "pkg-config not found, skipping wayland libraries")
    endif()


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
  set(HAVE_PTHREAD_SIGNAL 1)
  set(SELECT_TYPE_ARG1 int)
  set(SELECT_TYPE_ARG234 " (fd_set *)")
  set(SELECT_TYPE_ARG5 " (struct timeval *)")
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
