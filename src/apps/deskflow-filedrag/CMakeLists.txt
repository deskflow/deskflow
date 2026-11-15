# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

# deskflow-filedrag: the helper that does the OLE side of dragging files between
# computers. It runs as the signed-in user (OLE drag and drop cannot cross integrity
# levels and deskflow-core is often elevated). Windows only.

set(target ${CMAKE_PROJECT_NAME}-filedrag)

set(FILEDRAG_CORE_SOURCES
  ${CMAKE_SOURCE_DIR}/src/lib/deskflow/FileDragCodec.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/deskflow/FileDragSender.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/deskflow/FileDragStaging.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/platform/MSWindowsFileDragPipe.cpp
)

add_executable(${target} WIN32
  main.cpp
  SourceSide.cpp
  DestSide.cpp
  FileDragHelper.h
  ${FILEDRAG_CORE_SOURCES}
)

set(EXE_DESCRIPTION "${CMAKE_PROJECT_PROPER_NAME} file drag helper")
set(EXE_ICON "IDI_DESKFLOW ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/src/apps/res/deskflow.ico\"")
configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in ${target}.rc)
target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc)

target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src/lib)
target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(${target} PRIVATE ole32 oleaut32 shell32 shlwapi uuid user32 gdi32 userenv)

install(
  TARGETS ${target}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# end-to-end check without a mouse: runs two helpers on a hidden desktop (not installed)
if(NOT DEFINED BUILD_TESTS OR BUILD_TESTS)
  add_executable(${target}-e2e
    test/FileDragE2E.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/deskflow/FileDragCodec.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/platform/MSWindowsFileDragPipe.cpp
  )
  target_include_directories(${target}-e2e PRIVATE ${CMAKE_SOURCE_DIR}/src/lib)
  target_compile_definitions(${target}-e2e PRIVATE UNICODE _UNICODE NOMINMAX)
  target_link_libraries(${target}-e2e PRIVATE bcrypt user32 userenv)
  add_dependencies(${target}-e2e ${target})
endif()
