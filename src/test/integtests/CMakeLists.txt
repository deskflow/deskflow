# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

config_test()
set(target integtests)
add_executable(${target} ${sources} ${headers})
target_link_libraries(${target} ${test_libs})
