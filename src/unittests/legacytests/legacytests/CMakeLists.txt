# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

config_test()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(target legacytests)
add_executable(${target} ${sources} ${headers})
target_link_libraries(${target} ${test_libs} common)

if(ENABLE_COVERAGE)
  set(test_exclude subprojects/* build/* src/unittests/*)
  set(test_src ${PROJECT_SOURCE_DIR}/src)
  setup_target_for_coverage_gcovr_xml(
    NAME coverage-${target}
    EXECUTABLE ${target}
    BASE_DIRECTORY ${test_src}
    EXCLUDE ${test_exclude}
  )
endif()

