# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

include(FetchContent)
set(INSTALL_GTEST OFF)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        b514bdc898e2951020cbdca1304b75f5950d1f59 # release-1.15.2
    GIT_SHALLOW 1
    FIND_PACKAGE_ARGS NAMES GTest
)
FetchContent_MakeAvailable(googletest)

macro(config_all_tests)

  # Required to load images from .qrc file.
  set(CMAKE_AUTORCC ON)

  set(base_dir ${PROJECT_SOURCE_DIR})
  set(src_dir ${base_dir}/src)
  set(test_base_dir ${src_dir}/test)
  set(gui_dir ${src_dir}/gui/src)

  config_test_deps()

  add_subdirectory(integtests)
  add_subdirectory(unittests)

endmacro()

macro(config_test)

  include_directories(
    ${test_base_dir}
    ${src_dir}
    ${src_dir}/lib
    ${gui_dir}
    ${ext_dir})

  set_sources()

endmacro()

macro(set_sources)

  file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
  file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

  file(GLOB_RECURSE shared_headers ${test_base_dir}/shared/*.h)
  file(GLOB_RECURSE shared_sources ${test_base_dir}/shared/*.cpp)

  list(APPEND headers ${shared_headers})
  list(APPEND sources ${shared_sources})

  file(GLOB_RECURSE mock_headers ${test_base_dir}/mock/*.h)
  file(GLOB_RECURSE mock_sources ${test_base_dir}/mock/*.cpp)

  list(APPEND headers ${mock_headers})
  list(APPEND sources ${mock_sources})

  if(ADD_HEADERS_TO_SOURCES)
    list(APPEND sources ${headers})
  endif()

  list(APPEND sources ${PROJECT_SOURCE_DIR}/src/apps/res/deskflow.qrc)

  replace_platform_sources()
  replace_arch_sources()

endmacro()

macro(replace_platform_sources)

  set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/platform)

  # Remove platform files so that specific platform files can be added later.
  # This is a bit weird, but it's simpler to include everything, remove all
  # platform files, then only include the platforms we need.
  file(GLOB_RECURSE all_platform_files ${platform_dir}/*)
  list(REMOVE_ITEM headers ${all_platform_files})
  list(REMOVE_ITEM sources ${all_platform_files})

  if(WIN32)
    file(GLOB platform_sources ${platform_dir}/MSWindows*.cpp)
    file(GLOB platform_headers ${platform_dir}/MSWindows*.h)
  elseif(APPLE)
    file(GLOB platform_sources ${platform_dir}/OSX*.cpp)
    file(GLOB platform_headers ${platform_dir}/OSX*.h)
  elseif(UNIX)
    file(GLOB platform_sources ${platform_dir}/XWindows*.cpp)
    file(GLOB platform_headers ${platform_dir}/XWindows*.h)
  endif()

  list(APPEND sources ${platform_sources})
  list(APPEND headers ${platform_headers})

endmacro()

macro(replace_arch_sources)

  set(arch_dir ${CMAKE_CURRENT_SOURCE_DIR}/arch)

  # Remove arch files so that specific arch files can be added later.
  # This is a bit weird, but it's simpler to include everything, remove all
  # arch files, then only include the archs we need.
  file(GLOB_RECURSE all_arch_files ${arch_dir}/*)
  list(REMOVE_ITEM headers ${all_arch_files})
  list(REMOVE_ITEM sources ${all_arch_files})

  if(WIN32)
    file(GLOB arch_sources ${arch_dir}/win32/*.cpp)
    file(GLOB arch_headers ${arch_dir}/win32/*.h)
  elseif(UNIX)
    file(GLOB arch_sources ${arch_dir}/unix/*.cpp)
    file(GLOB arch_headers ${arch_dir}/unix/*.h)
  endif()

  list(APPEND sources ${arch_sources})
  list(APPEND headers ${arch_headers})

endmacro()

macro(config_test_deps)

  # gui library autogen headers:
  # qt doesn't seem to auto include the autogen headers for libraries.
  include_directories(${PROJECT_BINARY_DIR}/src/lib/gui/gui_autogen/include)

  set(test_libs
      arch
      base
      client
      server
      io
      net
      platform
      server
      app
      mt
      gui
      GTest::gtest
      GTest::gmock
      ${libs})

endmacro()

config_all_tests()
