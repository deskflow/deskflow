# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2024 Symless Ltd
# SPDX-License-Identifier: MIT

if(APPLE)
  set(target Deskflow)
else()
  set(target deskflow)
endif()


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}\\n Control and configure deskflow")
  set(EXE_ICON "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\" ")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in deskflow.rc)
  set(platform_extra deskflow.rc)
elseif(APPLE)
  #setup our bundle plist file
  set(BUNDLE_EXECUTABLE_NAME "${target}")
  set(BUNDLE_BUNDLE_NAME "${target}")
  set(BUNDLE_DISPLAY_NAME "${target}")
  set(BUNDLE_GUI_IDENTIFIER "org.deskflow.deskflow")
  set(BUNDLE_ICON_FILE ${target}.icns)
  set(BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}")
  set(BUNDLE_COPYRIGHT "${CMAKE_PROJECT_COPYRIGHT}")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../res/deskflow.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist
    @ONLY
  )

  file(COPY_FILE
    ${PROJECT_SOURCE_DIR}/LICENSES/LicenseRef-OpenSSL-Exception.txt
    ${CMAKE_CURRENT_BINARY_DIR}/LICENSE_EXCEPTION
    ONLY_IF_DIFFERENT
  )

  set(platform_extra ../res/Deskflow.icns ${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_CURRENT_BINARY_DIR}/LICENSE_EXCEPTION)
  set_source_files_properties(${platform_extra} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${target} WIN32 MACOSX_BUNDLE
  ${platform_extra}
  ../res/deskflow.qrc
  Action.cpp
  Action.h
  DataDownloader.cpp
  DataDownloader.h
  Hotkey.cpp
  Hotkey.h
  KeySequence.cpp
  KeySequence.h
  main.cpp
  MainWindow.cpp
  MainWindow.h
  MainWindow.ui
  ProcessorArch.h
  QUtility.cpp
  QUtility.h
  ScreenSetupModel.cpp
  ScreenSetupModel.h
  ServerConfig.cpp
  ServerConfig.h
  VersionChecker.cpp
  VersionChecker.h
  dialogs/AboutDialog.cpp
  dialogs/AboutDialog.h
  dialogs/AboutDialog.ui
  dialogs/ActionDialog.cpp
  dialogs/ActionDialog.h
  dialogs/ActionDialog.ui
  dialogs/AddClientDialog.cpp
  dialogs/AddClientDialog.h
  dialogs/AddClientDialog.ui
  dialogs/FingerprintDialog.h
  dialogs/FingerprintDialog.cpp
  dialogs/HotkeyDialog.cpp
  dialogs/HotkeyDialog.h
  dialogs/HotkeyDialog.ui
  dialogs/ScreenSettingsDialog.cpp
  dialogs/ScreenSettingsDialog.h
  dialogs/ScreenSettingsDialog.ui
  dialogs/ServerConfigDialog.cpp
  dialogs/ServerConfigDialog.h
  dialogs/ServerConfigDialog.ui
  dialogs/SettingsDialog.cpp
  dialogs/SettingsDialog.h
  dialogs/SettingsDialog.ui
  widgets/FingerprintPreview.h
  widgets/FingerprintPreview.cpp
  widgets/KeySequenceWidget.cpp
  widgets/KeySequenceWidget.h
  widgets/NewScreenWidget.h
  widgets/NewScreenWidget.cpp
  widgets/ScreenSetupView.cpp
  widgets/ScreenSetupView.h
  widgets/TrashScreenWidget.cpp
  widgets/TrashScreenWidget.h
)

target_link_libraries(
  ${target}
  gui
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

if(WIN32)
  set_target_properties(${target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
  install(
    TARGETS ${target}
    RUNTIME_DEPENDENCY_SET guiDeps
    DESTINATION .
  )
  install(RUNTIME_DEPENDENCY_SET guiDeps
    PRE_EXCLUDE_REGEXES
      "api-ms-win-.*"
      "ext-ms-.*"
      "^hvsifiletrust\\.dll$"
    POST_EXCLUDE_REGEXES
      ".*system32.*"
    RUNTIME DESTINATION .
  )

  add_custom_command(
    TARGET ${target} POST_BUILD
      COMMAND ${DEPLOYQT} --no-compiler-runtime --no-system-d3d-compiler --no-quick-import -network $<TARGET_FILE:${target}>
  )
elseif(APPLE)
  set_target_properties(${target} PROPERTIES
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist"
  )
  install(TARGETS ${target} BUNDLE DESTINATION .)
else()
  install(TARGETS ${target} DESTINATION bin)
endif()
