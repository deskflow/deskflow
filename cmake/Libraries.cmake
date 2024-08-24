set(LIBEI_MIN_VERSION 0.99.1)
set(LIBEI_TAG 1.2.1)
set(LIBPORTAL_TAG a1530a9)

macro(configure_libs)

  update_submodules()

  set(libs)
  if(UNIX)
    configure_unix_libs()
  elseif(WIN32)
    configure_windows_libs()
  endif()

  config_qt()
  configure_openssl()
  configure_test_libs()

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

  # For config.h, save the results based on a template (config.h.in).
  configure_file(res/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/lib/config.h)

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

# TODO: the output is a bit noisy, maybe make it quieter?
macro(configure_wayland_libs)

  include(FindPkgConfig)

  # Until libei is generally available in package repos, build it by default.
  option(SYSTEM_LIBEI "Use system libei" OFF)
  if(SYSTEM_LIBEI)
    pkg_check_modules(LIBEI REQUIRED "libei-1.0 >= ${LIBEI_MIN_VERSION}")
  else()
    set(libei_source_dir "${CMAKE_BINARY_DIR}/libei")
    set(libei_build_dir "${libei_source_dir}/build")
    build_libei()
    find_library(LIBEI_LIBRARIES libei PATH ${libei_build_dir})
    set(LIBEI_INCLUDE_DIRS ${libei_source_dir}/src ${libei_build_dir}/libei)
  endif()

  # Until libportal is generally available in package repos, build it by default.
  option(SYSTEM_LIBPORTAL "Use system libportal" OFF)
  if(SYSTEM_LIBPORTAL)
    pkg_check_modules(LIBPORTAL REQUIRED libportal)
  else()
    build_libportal()
  endif()

  pkg_check_modules(LIBXKBCOMMON REQUIRED xkbcommon)
  pkg_check_modules(GLIB2 REQUIRED glib-2.0 gio-2.0)
  find_library(LIBM m)

  include_directories(
    ${LIBXKBCOMMON_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    ${LIBM_INCLUDE_DIRS}
    ${LIBEI_INCLUDE_DIRS}
    ${LIBPORTAL_INCLUDE_DIRS})

  list(
    APPEND
    libs
    ${LIBXKBCOMMON_LINK_LIBRARIES}
    ${GLIB2_LINK_LIBRARIES}
    ${LIBM_LIBRARIES}
    ${LIBEI_LINK_LIBRARIES}
    ${LIBPORTAL_LINK_LIBRARIES})

  add_definitions(-DWINAPI_LIBEI=1)
  add_definitions(-DWINAPI_LIBPORTAL=1)

endmacro()

macro(check_meson)
  find_program(MESON_EXECUTABLE meson)

  if(NOT MESON_EXECUTABLE)
    message(FATAL_ERROR "Meson is not installed")
  endif()
endmacro()

macro(check_git)
  find_package(Git QUIET)

  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Git was not found")
  endif()
endmacro()

# TODO: re-implement in install_deps.py.
# that way we're not tied to a particular version of libei.
macro(build_libei)
  include(FetchContent)

  check_meson()
  check_git()

  message(STATUS "Building libei ${LIBEI_TAG}")

  FetchContent_Declare(
    libei
    GIT_REPOSITORY https://gitlab.freedesktop.org/libinput/libei.git
    GIT_TAG ${LIBEI_TAG}
    SOURCE_DIR ${libei_source_dir})

  FetchContent_MakeAvailable(libei)

  execute_process(
    COMMAND meson setup ${libei_build_dir} ${libei_source_dir}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE libei_setup_result
    OUTPUT_QUIET)

  if(NOT libei_setup_result EQUAL "0")
    message(FATAL_ERROR "Meson setup failed: ${libei_setup_result}")
  endif()

  execute_process(
    COMMAND meson compile -C ${libei_build_dir}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE libei_compile_result
    OUTPUT_QUIET)

  if(NOT libei_compile_result EQUAL "0")
    message(FATAL_ERROR "Meson compile failed: ${libei_compile_result}")
  endif()

endmacro()

# TODO: move to a separate .cmake file and call it from install_deps.py,
# that way we're not tied to a particular version of libportal, and
# we can reintroduce the symbol checks to support multiple libportal versions.
macro(build_libportal)
  include(FetchContent)

  set(libportal_source_dir "${CMAKE_BINARY_DIR}/libportal")
  set(libportal_build_dir "${libportal_source_dir}/build")

  check_meson()
  check_git()

  message(STATUS "Building libportal ${LIBPORTAL_TAG}")

  FetchContent_Declare(
    libportal
    GIT_REPOSITORY https://github.com/flatpak/libportal.git
    GIT_TAG ${LIBPORTAL_TAG}
    SOURCE_DIR ${libportal_source_dir})

  FetchContent_MakeAvailable(libportal)

  execute_process(
    COMMAND meson setup ${libportal_build_dir} ${libportal_source_dir}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE libportal_setup_result
    OUTPUT_QUIET)

  if(NOT libportal_setup_result EQUAL "0")
    message(FATAL_ERROR "Meson setup failed: ${libportal_setup_result}")
  endif()

  execute_process(
    COMMAND meson compile -C ${libportal_build_dir}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE libportal_compile_result
    OUTPUT_QUIET)

  if(NOT libportal_compile_result EQUAL "0")
    message(FATAL_ERROR "Meson compile failed: ${libportal_compile_result}")
  endif()

  # HACK: copy generated enums header (portal-enums.h) to the include dir
  # so that we can include it without including the build dir (which causes
  # type declaration conflicts).
  file(COPY ${libportal_build_dir}/libportal/portal-enums.h
       DESTINATION ${libportal_source_dir}/libportal)

  find_library(LIBPORTAL_LIBRARIES libportal PATH ${libportal_build_dir})
  set(LIBPORTAL_INCLUDE_DIRS ${libportal_source_dir})
  include_directories(${LIBPORTAL_INCLUDE_DIRS})

endmacro()

#
# X.org/X11 for Linux, BSD, etc
#
macro(configure_xorg_libs)

  # Add include dir for BSD (posix uses /usr/include/)
  set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}:/usr/local/include")

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
    /DSYNERGY_VERSION=\"${SYNERGY_VERSION}\"
    /D_XKEYCHECK_H)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/win/version.rc.in
                 ${CMAKE_BINARY_DIR}/src/version.rc @ONLY)

  find_openssl_dir_win32(OPENSSL_PATH)
  add_definitions(-DOPENSSL_PATH="${OPENSSL_PATH}")

endmacro()

macro(config_qt)

  find_package(
    Qt6
    COMPONENTS Core Widgets Network
    REQUIRED)

  message(STATUS "Qt version: ${Qt6_VERSION}")

endmacro()

macro(configure_openssl)
  # Apple has to use static libraries because "Use of the Apple-provided OpenSSL
  # libraries by apps is strongly discouraged."
  # https://developer.apple.com/library/archive/documentation/Security/Conceptual/cryptoservices/SecureNetworkCommunicationAPIs/SecureNetworkCommunicationAPIs.html
  if(APPLE OR DEFINED ENV{SYNERGY_STATIC_OPENSSL})
    set(OPENSSL_USE_STATIC_LIBS TRUE)
  endif()

  find_package(OpenSSL REQUIRED)
endmacro()

macro(configure_test_libs)

  if(BUILD_TESTS AND NOT EXISTS
                     "${PROJECT_SOURCE_DIR}/ext/googletest/CMakeLists.txt")
    message(FATAL_ERROR "Git submodule for Google Test is missing")
  endif()

  if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage")
    include(cmake/CodeCoverage.cmake)
    append_coverage_compiler_flags()
    set(test_exclude ext/* build/* src/test/*)
    set(test_src ${PROJECT_SOURCE_DIR}/src)

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

  include_directories(BEFORE SYSTEM
                      ${PROJECT_SOURCE_DIR}/ext/googletest/googletest/include)
endmacro()

macro(update_submodules)

  option(GIT_SUBMODULE "Check submodules during build" ON)
  if(GIT_SUBMODULE)

    check_git()

    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/.git")
      message(FATAL_ERROR "This is not a git repository")
    endif()

    message(STATUS "Submodule update")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMODULE_RESULT)

    if(NOT GIT_SUBMODULE_RESULT EQUAL "0")
      message(
        FATAL_ERROR "Git submodule update failed: ${GIT_SUBMODULE_RESULT}")
    endif()

  endif()

endmacro()

macro(configure_python)
  if(WIN32)
    set(PYTHON_BIN "${CMAKE_BINARY_DIR}/python/Scripts/python.exe")
  else()
    set(PYTHON_BIN "${CMAKE_BINARY_DIR}/python/bin/python")
  endif()
endmacro()

#
# Find the OpenSSL directory on Windows based on the location of the first
# `openssl` binary found.
#
function(find_openssl_dir_win32 result)

  execute_process(
    COMMAND where openssl
    OUTPUT_VARIABLE OPENSSL_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # It's possible that there are multiple OpenSSL installations on the system,
  # which is the case on GitHub runners. For now we'll pick the first one, but
  # that's probably not very robust. Maybe our choco config could install to a
  # specific location?
  string(REGEX REPLACE "\r?\n" ";" OPENSSL_PATH_LIST ${OPENSSL_PATH})
  message(STATUS "Found OpenSSL binaries at: ${OPENSSL_PATH_LIST}")

  list(GET OPENSSL_PATH_LIST 0 OPENSSL_FIRST_PATH)
  message(VERBOSE "First OpenSSL binary: ${OPENSSL_FIRST_PATH}")

  get_filename_component(OPENSSL_BIN_DIR ${OPENSSL_FIRST_PATH} DIRECTORY)
  message(VERBOSE "OpenSSL bin dir: ${OPENSSL_BIN_DIR}")

  get_filename_component(OPENSSL_DIR ${OPENSSL_BIN_DIR} DIRECTORY)
  message(VERBOSE "OpenSSL install root dir: ${OPENSSL_DIR}")

  set(${result}
      ${OPENSSL_DIR}
      PARENT_SCOPE)

endfunction()
