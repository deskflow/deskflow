# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

option(BUILD_UNIFIED "Build unified binary" OFF)
if(BUILD_UNIFIED)
  add_subdirectory(deskflow-core)
else()
  add_subdirectory(deskflow-client)
  add_subdirectory(deskflow-server)
endif(BUILD_UNIFIED)

## Only used on windows
add_subdirectory(deskflow-daemon)

option(BUILD_GUI "Build GUI" ON)
if(BUILD_GUI)
  add_subdirectory(deskflow-gui)
endif(BUILD_GUI)
