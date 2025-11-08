# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2024 Symless Ltd
# SPDX-License-Identifier: MIT

if(APPLE)
  set(target Deskflow)
else()
  set(target deskflow)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_PROPER_NAME} GUI configuration tool")
  set(EXE_ICON "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\" ")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in deskflow.rc)
  set(platform_extra deskflow.rc)
elseif(APPLE)
  #setup our bundle plist file
  set(BUNDLE_EXECUTABLE_NAME "${target}")
  set(BUNDLE_BUNDLE_NAME "${target}")
  set(BUNDLE_DISPLAY_NAME "${target}")
  set(BUNDLE_GUI_IDENTIFIER "${CMAKE_PROJECT_REV_FQDN}")
  set(BUNDLE_ICON_FILE ${target}.icns)
  set(BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}")
  set(BUNDLE_COPYRIGHT "${CMAKE_PROJECT_COPYRIGHT}")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../res/deskflow.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist
    @ONLY
  )

  set(platform_extra ../res/Deskflow.icns)
  set_source_files_properties(${platform_extra} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${target} WIN32 MACOSX_BUNDLE
  ${platform_extra}
  ../res/deskflow.qrc
  deskflow-gui.cpp
)

target_link_libraries(
  ${target}
  gui
  common
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

install(
  TARGETS ${target}
  RUNTIME_DEPENDENCY_SET guiDeps
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  BUNDLE DESTINATION .
)

if(WIN32)
  set_target_properties(${target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")

  install(RUNTIME_DEPENDENCY_SET guiDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  set(QT_DEPENDS_DIR ${CMAKE_BINARY_DIR}/qt-depends)

  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND ${DEPLOYQT}
      --no-compiler-runtime
      --no-system-d3d-compiler
      --no-opengl-sw
      --no-quick-import
      --dir "${QT_DEPENDS_DIR}"
      --plugindir "${QT_DEPENDS_DIR}/plugins"
      $<TARGET_FILE:${target}>
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
      ${QT_DEPENDS_DIR}
      $<TARGET_FILE_DIR:${target}>
  )

  install(
    DIRECTORY ${QT_DEPENDS_DIR}/
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PATTERN "dx*.dll" EXCLUDE
  )

elseif(APPLE)
  set_target_properties(${target} PROPERTIES
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist"
  )

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
  if (NOT "${APPLE_CODESIGN_DEV}" STREQUAL "")
    message(STATUS "Apple codesign ID for development only: ${APPLE_CODESIGN_DEV}")
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND /usr/bin/codesign
              --force
              --options runtime
              --entitlements "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/src/apps/res/entitlements-dev.plist>"
              --sign "${APPLE_CODESIGN_DEV}"
              "$<TARGET_BUNDLE_DIR:${target}>"
      VERBATIM
    )
  endif()
else()
  generate_app_man(${target} "${CMAKE_PROJECT_DESCRIPTION} \\(GUI\\)")
endif()
