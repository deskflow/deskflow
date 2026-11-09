# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2024 Symless Ltd
# SPDX-License-Identifier: MIT

if(DESKFLOW_UI_REVIEW_BUILD)
  set(target DeskflowUIReview)
elseif(DESKFLOW_CANDIDATE_BUILD)
  set(target DeskflowCandidate)
elseif(DESKFLOW_LAB_BUILD)
  set(target DeskflowLab)
elseif(APPLE)
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
elseif(BUILD_OSX_BUNDLE)
  #setup our bundle plist file
  set(BUNDLE_EXECUTABLE_NAME "${target}")
  set(BUNDLE_BUNDLE_NAME "${target}")
  set(BUNDLE_DISPLAY_NAME "${target}")
  set(BUNDLE_GUI_IDENTIFIER "${CMAKE_PROJECT_REV_FQDN}")
  set(BUNDLE_ICON_FILE Deskflow.icns)
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

set(guiEntry deskflow-gui.cpp)
if(DESKFLOW_UI_REVIEW_BUILD)
  set(guiEntry "${CMAKE_SOURCE_DIR}/native-tests/apple-ui/FullReview.cpp")
endif()
add_executable(${target} WIN32 MACOSX_BUNDLE
  ${platform_extra}
  ../res/deskflow.qrc
  ${guiEntry}
)

if(DESKFLOW_UI_REVIEW_BUILD)
  find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
  qt_add_lrelease(TS_FILES "${CMAKE_SOURCE_DIR}/translations/deskflow_zh_CN.ts" "${CMAKE_SOURCE_DIR}/translations/deskflow_en.ts"
    LRELEASE_TARGET FullReviewTranslations QM_FILES_OUTPUT_VARIABLE fullReviewQmFiles)
  add_dependencies(${target} FullReviewTranslations)
  qt_add_resources(${target} full_review_translations PREFIX "/translations"
    BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${fullReviewQmFiles})
  set_target_properties(${target} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.deskflow.uireview"
    MACOSX_BUNDLE_BUNDLE_NAME "Deskflow UI Review"
    MACOSX_BUNDLE_BUNDLE_VERSION "20.8"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.20.8")
endif()

target_link_libraries(
  ${target}
  gui
  common
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

if(DESKFLOW_LAB_BUILD)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  add_executable(DesktopProductFlowTests "${CMAKE_SOURCE_DIR}/native-tests/workspace/DesktopProductFlowTests.cpp" ../res/deskflow.qrc)
  target_link_libraries(DesktopProductFlowTests PRIVATE gui Qt6::Test)
  if(DESKFLOW_UI_REVIEW_BUILD)
    add_dependencies(DesktopProductFlowTests FullReviewTranslations)
    qt_add_resources(DesktopProductFlowTests desktop_product_translations PREFIX "/translations"
      BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${fullReviewQmFiles})
  endif()
  add_executable(LabStartupEntryTests "${CMAKE_SOURCE_DIR}/native-tests/startup/SettingsEntryTests.cpp" ../res/deskflow.qrc)
  target_link_libraries(LabStartupEntryTests PRIVATE gui Qt6::Test)
  add_executable(LabFirstSetupFlowTests "${CMAKE_SOURCE_DIR}/native-tests/first-use/FirstSetupFlowTests.cpp" ../res/deskflow.qrc)
  target_link_libraries(LabFirstSetupFlowTests PRIVATE gui Qt6::Test)
  add_executable(LabHomeFlowTests "${CMAKE_SOURCE_DIR}/native-tests/apple-ui/FullHomeTests.cpp" ../res/deskflow.qrc)
  target_link_libraries(LabHomeFlowTests PRIVATE gui Qt6::Test)
  add_executable(LabCoreSessionTests ../../unittests/gui/core/CoreProcessSessionTests.cpp)
  target_link_libraries(LabCoreSessionTests PRIVATE gui Qt6::Test)
  add_executable(LabDiscoveryTests ../../unittests/gui/core/DiscoveryIntegrationTests.cpp)
  target_link_libraries(LabDiscoveryTests PRIVATE gui Qt6::Test)
  add_executable(LabFullWindowTests ../../unittests/gui/core/FullWindowTests.cpp ../res/deskflow.qrc)
  target_link_libraries(LabFullWindowTests PRIVATE gui Qt6::Test)
  add_executable(LabIpcLifecycleTests ../../unittests/gui/core/IpcLifecycleTests.cpp)
  target_link_libraries(LabIpcLifecycleTests PRIVATE gui app Qt6::Test)
  add_executable(LabClientRecoveryTests ../../unittests/gui/core/ClientAppRecoveryTests.cpp)
  target_link_libraries(LabClientRecoveryTests PRIVATE app client server platform Qt6::Test Qt6::Widgets)
  add_executable(LabAuthenticationChainTests ../../unittests/gui/core/AuthenticationChainTests.cpp)
  target_link_libraries(LabAuthenticationChainTests PRIVATE gui app client server platform Qt6::Test Qt6::Widgets)
  if(APPLE)
    foreach(platform Mac Windows)
      add_executable(LabScopedAddress${platform}Tests
        "${CMAKE_SOURCE_DIR}/native-tests/ScopedAddress${platform}Tests.cpp"
        ../../lib/gui/core/ScopedAddressResolver.cpp)
      target_compile_definitions(LabScopedAddress${platform}Tests PRIVATE DESKFLOW_SCOPED_RESOLVER_TEST=1)
      target_include_directories(LabScopedAddress${platform}Tests PRIVATE "${CMAKE_SOURCE_DIR}/native-tests")
      target_link_libraries(LabScopedAddress${platform}Tests PRIVATE gui app client server platform Qt6::Test Qt6::Widgets)
    endforeach()
    target_compile_definitions(LabScopedAddressWindowsTests PRIVATE DESKFLOW_SCOPED_RESOLVER_WINDOWS_TEST=1)
  endif()
  if(APPLE)
    set_source_files_properties(../../lib/gui/core/RecoveryProbe.cpp PROPERTIES SKIP_AUTOMOC ON)
    add_executable(LabProbeProcessTests "${CMAKE_SOURCE_DIR}/native-tests/ProbeProcessTests.cpp" ../../lib/gui/core/RecoveryProbe.cpp)
    target_compile_definitions(LabProbeProcessTests PRIVATE DESKFLOW_PROBE_PROCESS_TEST=1)
    target_link_libraries(LabProbeProcessTests PRIVATE gui app client server platform Qt6::Test Qt6::Widgets)
  endif()
  add_library(LabDaemonSourceCompile OBJECT ../deskflow-daemon/DaemonApp.cpp)
  target_link_libraries(LabDaemonSourceCompile PRIVATE app platform Qt6::Network)
  add_executable(LabCandidatePolicyTests ../../unittests/gui/core/CandidatePolicyTests.cpp)
  target_link_libraries(LabCandidatePolicyTests PRIVATE common Qt6::Test)
  add_executable(LabRuntimeProfileTests ../../unittests/gui/core/RuntimeProfileTests.cpp)
  target_link_libraries(LabRuntimeProfileTests PRIVATE common Qt6::Test)
  add_executable(LabAutomaticRecoveryTests ../../unittests/gui/core/AutomaticRecoveryTests.cpp)
  add_executable(LabWindowsStartupProviderTests "${CMAKE_SOURCE_DIR}/native-tests/WindowsStartupProviderTests.cpp")
  target_link_libraries(LabWindowsStartupProviderTests PRIVATE gui Qt6::Test)
  add_executable(LabWindowsStartupDialogTests "${CMAKE_SOURCE_DIR}/native-tests/WindowsStartupDialogTests.cpp" ../res/deskflow.qrc)
  target_link_libraries(LabWindowsStartupDialogTests PRIVATE gui Qt6::Test)
  qt_add_resources(LabWindowsStartupDialogTests "windows_startup_translations" PREFIX "/translations" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${CMAKE_CURRENT_BINARY_DIR}/deskflow_zh_CN.qm")
  target_link_libraries(LabAutomaticRecoveryTests PRIVATE gui app client server platform Qt6::Test Qt6::Widgets)
  add_executable(LabRecoveryProbeTests ../../unittests/gui/core/RecoveryProbeTests.cpp)
  target_link_libraries(LabRecoveryProbeTests PRIVATE gui app client server platform Qt6::Test Qt6::Widgets)
  add_executable(LabStage12Tests ../../unittests/gui/core/Stage12IntegrationTests.cpp)
  target_link_libraries(LabStage12Tests PRIVATE gui app client server platform Qt6::Test Qt6::Widgets)
endif()

install(
  TARGETS ${target}
  RUNTIME_DEPENDENCY_SET guiDeps
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  BUNDLE DESTINATION .
)

if(WIN32)
  set_target_properties(${target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")

  install(RUNTIME_DEPENDENCY_SET guiDeps
    ${WIN32_RUNTIME_DEPENDENCY_SET_ARGS}
    PRE_EXCLUDE_REGEXES ${WIN32_PRE_EXCLUDE_REGEXES}
    POST_EXCLUDE_REGEXES ${WIN32_POST_EXCLUDE_REGEXES}
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
    PATTERN "icu*.dll" EXCLUDE
  )

elseif(APPLE)
  if (BUILD_OSX_BUNDLE)
    set_target_properties(${target} PROPERTIES
      INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist"
      XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    )
    if (APPLE_CODESIGN_DEV)
      configure_mac_codesign(${target})
    endif()
    if(DESKFLOW_UI_REVIEW_BUILD)
      # Use the review identity above, rather than the production plist template.
      set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_ROOT}/Modules/MacOSXBundleInfo.plist.in")
    endif()
  else()
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE FALSE)
  endif()
else()
  generate_app_man(${target} "${CMAKE_PROJECT_DESCRIPTION} \\(GUI\\)")
endif()
