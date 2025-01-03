# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# HACK This is set when the files is included so its the real path
# calling CMAKE_CURRENT_LIST_DIR after include would return the wrong scope var
set(MY_DIR ${CMAKE_CURRENT_LIST_DIR})

# Install depends into our staged copy
find_program(MACDEPLOYQT_BIN macdeployqt6)
install(CODE "execute_process(COMMAND ${MACDEPLOYQT_BIN} \"\${CMAKE_INSTALL_PREFIX}/Deskflow.app\")")

set(OS_STRING "macos-${CMAKE_SYSTEM_PROCESSOR}")
set(CMAKE_INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks")
set(CPACK_PACKAGE_ICON "${MY_DIR}/dmg-volume.icns")
set(CPACK_DMG_BACKGROUND_IMAGE "${MY_DIR}/dmg-background.tiff")
set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${MY_DIR}/generate_ds_store.applescript")
set(CPACK_DMG_VOLUME_NAME "Deskflow")
set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE ON)
set(CPACK_GENERATOR "DragNDrop")
