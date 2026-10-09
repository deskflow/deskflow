# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

add_executable(deskflow-handoff deskflow-handoff.mm ../../lib/gui/handoff/Native.mm)
target_link_libraries(deskflow-handoff PRIVATE handoff "-framework AppKit" "-framework ApplicationServices")
if(BUILD_OSX_BUNDLE)
  set_target_properties(deskflow-handoff PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_BUNDLE_CONTENT_DIR:Deskflow>/MacOS
  )
  if(APPLE_CODESIGN_DEV)
    configure_mac_codesign(deskflow-handoff)
  endif()
endif()
install(TARGETS deskflow-handoff RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
