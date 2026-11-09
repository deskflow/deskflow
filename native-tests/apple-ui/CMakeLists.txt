# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
add_library(DeskflowHomeView STATIC
  "${deskflowSource}/src/lib/gui/widgets/ConnectionHome.cpp"
  "${deskflowSource}/src/lib/gui/widgets/ConnectionHome.h"
  "${deskflowSource}/src/lib/gui/IdentityBindings.cpp"
  "${deskflowSource}/src/lib/gui/IdentityBindings.h"
  "${deskflowSource}/src/lib/gui/dialogs/TrustedDevicesDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/TrustedDevicesDialog.h"
)
target_include_directories(DeskflowHomeView PUBLIC "${deskflowSource}/src/lib")
target_link_libraries(DeskflowHomeView PUBLIC Qt6::Widgets Qt6::Network)
add_executable(DeskflowHomeTests HomeTests.cpp)
target_link_libraries(DeskflowHomeTests PRIVATE DeskflowHomeView Qt6::Test)
qt_add_resources(DeskflowHomeTests home_test_icons PREFIX "/"
  BASE "${deskflowSource}/src/apps/res"
  FILES
    "${deskflowSource}/src/apps/res/icons/deskflow-light/apps/64/org.deskflow.deskflow-symbolic.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/network-connect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/network-disconnect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/configure.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/help-about.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/computer-add.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/computers.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/connection-link.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/laptop-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/monitor-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/keyboard-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/mouse-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/apps/64/org.deskflow.deskflow-symbolic.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/network-connect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/network-disconnect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/configure.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/help-about.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/computer-add.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/computers.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/connection-link.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/laptop-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/monitor-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/keyboard-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/mouse-duotone.svg"
)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
qt_add_lrelease(TS_FILES "${deskflowSource}/translations/deskflow_zh_CN.ts"
  LRELEASE_TARGET DeskflowHomeTestTranslations QM_FILES_OUTPUT_VARIABLE homeQmFiles)
add_dependencies(DeskflowHomeTests DeskflowHomeTestTranslations)
qt_add_resources(DeskflowHomeTests home_test_translations PREFIX "/translations"
  BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${homeQmFiles})
add_test(NAME DeskflowHomeTests COMMAND DeskflowHomeTests)
set_tests_properties(DeskflowHomeTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
# Keep the native review executable limited to the presentation widget. The
# broader test library also contains device/identity components it does not use.
add_executable(DeskflowDesignReview MACOSX_BUNDLE HomePreview.cpp
  "${deskflowSource}/src/lib/gui/widgets/ConnectionHome.cpp"
  "${deskflowSource}/src/lib/gui/widgets/ConnectionHome.h")
target_include_directories(DeskflowDesignReview PRIVATE "${deskflowSource}/src/lib")
target_link_libraries(DeskflowDesignReview PRIVATE Qt6::Widgets)
qt_add_resources(DeskflowDesignReview review_icons PREFIX "/"
  BASE "${deskflowSource}/src/apps/res"
  FILES
    "${deskflowSource}/src/apps/res/icons/deskflow-light/apps/64/org.deskflow.deskflow-symbolic.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/network-connect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/network-disconnect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/configure.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/help-about.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/computer-add.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/computers.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/16/connection-link.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/laptop-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/monitor-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/keyboard-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-light/actions/22/mouse-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/apps/64/org.deskflow.deskflow-symbolic.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/network-connect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/network-disconnect.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/configure.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/help-about.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/computer-add.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/computers.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/16/connection-link.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/laptop-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/monitor-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/keyboard-duotone.svg"
    "${deskflowSource}/src/apps/res/icons/deskflow-dark/actions/22/mouse-duotone.svg"
)
qt_add_lrelease(TS_FILES "${deskflowSource}/translations/deskflow_zh_CN.ts"
  LRELEASE_TARGET DeskflowReviewTranslations QM_FILES_OUTPUT_VARIABLE reviewQmFiles)
add_dependencies(DeskflowDesignReview DeskflowReviewTranslations)
qt_add_resources(DeskflowDesignReview review_translations PREFIX "/translations"
  BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${reviewQmFiles})
set_target_properties(DeskflowDesignReview PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.deskflow.designreview"
  MACOSX_BUNDLE_BUNDLE_NAME "Deskflow Design Review"
  MACOSX_BUNDLE_BUNDLE_VERSION "20.5"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "0.20.5")
add_library(DeskflowSettingsCompile OBJECT EXCLUDE_FROM_ALL
  "${deskflowSource}/src/lib/gui/dialogs/SettingsDialog.cpp"
  "${deskflowSource}/src/lib/gui/dialogs/SettingsDialog.h"
  "${deskflowSource}/src/lib/gui/dialogs/SettingsDialog.ui"
  "${deskflowSource}/src/lib/gui/widgets/SettingsDialogButtonBox.cpp"
  "${deskflowSource}/src/lib/gui/widgets/SettingsDialogButtonBox.h"
  "${deskflowSource}/src/lib/gui/widgets/LogDock.cpp"
  "${deskflowSource}/src/lib/gui/widgets/LogDock.h"
)
set_target_properties(DeskflowSettingsCompile PROPERTIES AUTOMOC ON AUTOUIC ON)
target_include_directories(DeskflowSettingsCompile PRIVATE "${CMAKE_BINARY_DIR}/generated" "${deskflowSource}/src/lib")
target_compile_definitions(DeskflowSettingsCompile PRIVATE QT_NO_KEYWORDS)
target_link_libraries(DeskflowSettingsCompile PRIVATE Qt6::Widgets Qt6::Network)
