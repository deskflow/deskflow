# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT


add_subdirectory(deskflow-core)

option(BUILD_LEGACY "Build legacy binaries" OFF)
if(BUILD_LEGACY)
  message(WARNING "Legacy binaries are deprecated will be removed")
  add_subdirectory(deskflow-client)
  add_subdirectory(deskflow-server)
endif(BUILD_LEGACY)

option(BUILD_GUI "Build GUI" ON)
if(BUILD_GUI)
  add_subdirectory(deskflow-gui)
endif(BUILD_GUI)
