# SPDX-FileCopyrightText: (C) 2025-2026 Deskflow Contributors
# SPDX-License-Identifier: MIT

# Warning: Do not use for CI/production, as the `entitlements-dev.plist` file adds special
# entitlements that are only appropriate for local development.
#
# macOS made TCC stricter so that if you don't sign your local dev builds properly, macOS will
# nag you to remove and re-approve the app every time you make a change to the binary which is
# extremely annoying during development.
#
# If you were to use ad-hoc signing (i.e. not specify a certificate), TCC would still nag you
# because the binary identity is anchored not on the app ID, but on the CD hash (which changes
# based on the binary contents).
#
# To use, simply generate a personal certificate for free with Xcode and pass the ID to CMake.
# Full instructions are in the docs.

function(configure_mac_codesign target)
  set_property(GLOBAL APPEND PROPERTY _MAC_CODESIGN_DEPENDS $<TARGET_FILE:${target}>)

  get_property(deferred GLOBAL PROPERTY _MAC_CODESIGN_DEFERRED)

  if(NOT deferred)
    set_property(GLOBAL PROPERTY _MAC_CODESIGN_DEFERRED TRUE)
    message(STATUS "Apple codesign ID for development only: ${APPLE_CODESIGN_DEV}")
    cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _finalize_mac_codesign)
  endif()
endfunction()

function(_finalize_mac_codesign)
  get_property(depends GLOBAL PROPERTY _MAC_CODESIGN_DEPENDS)

  set(stamp_file "${CMAKE_BINARY_DIR}/CMakeFiles/codesign-dev.stamp")

  # Use a stamp file because codesign modifies the binaries it signs.
  add_custom_command(
    OUTPUT ${stamp_file}
    COMMAND /usr/bin/codesign
            --force
            --options runtime
            --entitlements "${CMAKE_SOURCE_DIR}/src/apps/res/entitlements-dev.plist"
            --sign "${APPLE_CODESIGN_DEV}"
            "$<TARGET_BUNDLE_DIR:${CMAKE_PROJECT_PROPER_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E touch ${stamp_file}
    DEPENDS ${depends}
    COMMENT "Codesigning ${CMAKE_PROJECT_PROPER_NAME}"
    VERBATIM
  )

  add_custom_target(codesign-dev ALL DEPENDS ${stamp_file})
endfunction()
