# SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
# SPDX-License-Identifier: MIT
#
# macOS login-window input bridge: replays the elected server's input stream
# through a Karabiner DriverKit virtual HID device so the cluster can drive
# this machine at the login window, where CGEventPost is blocked.
#
# Header-only SDK: pqrs-org Karabiner-DriverKit-VirtualHIDDevice. Pinned via
# FetchContent; set VHID_SDK_DIR to a local checkout to build offline (the
# version must match the installed driver/daemon's protocol).

if(NOT APPLE)
  return()
endif()

set(target ${CMAKE_PROJECT_NAME}-vhid-bridge)

set(VHID_SDK_DIR "" CACHE PATH "Local Karabiner-DriverKit-VirtualHIDDevice checkout (skips FetchContent)")
set(VHID_SDK_TAG "v6.14.0" CACHE STRING "Karabiner-DriverKit-VirtualHIDDevice tag to fetch")

if(VHID_SDK_DIR)
  set(vhid_sdk_root "${VHID_SDK_DIR}")
else()
  include(FetchContent)
  FetchContent_Declare(karabiner_vhid_sdk
    GIT_REPOSITORY https://github.com/pqrs-org/Karabiner-DriverKit-VirtualHIDDevice.git
    GIT_TAG ${VHID_SDK_TAG}
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(karabiner_vhid_sdk)
  set(vhid_sdk_root "${karabiner_vhid_sdk_SOURCE_DIR}")
endif()

foreach(dir IN ITEMS "${vhid_sdk_root}/include" "${vhid_sdk_root}/vendor/vendor/include")
  if(NOT EXISTS "${dir}")
    message(FATAL_ERROR "Karabiner VHID SDK include dir missing: ${dir}")
  endif()
endforeach()

add_executable(${target} deskflow-vhid-bridge.cpp)

# The pqrs headers want C++2b; keep this target self-contained rather than
# raising the project-wide standard.
set_target_properties(${target} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

target_include_directories(${target} SYSTEM PRIVATE
  "${vhid_sdk_root}/include"
  "${vhid_sdk_root}/vendor/vendor/include"
)

target_link_libraries(${target} PRIVATE
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework Foundation"
  "-framework IOKit"
)

install(
  TARGETS ${target}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_OSX_BUNDLE)
  set_target_properties(${target} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_BUNDLE_CONTENT_DIR:${CMAKE_PROJECT_PROPER_NAME}>/MacOS
  )
  if(APPLE_CODESIGN_DEV)
    configure_mac_codesign(${target})
  endif()
endif()
