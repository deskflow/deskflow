# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME ArchTests
  DEPENDS arch
  LIBS base ${extra_libs}
  SOURCE ArchTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/arch"
)

# Resource snapshots use Linux procfs or macOS Mach and /dev/fd.
if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_executable(ArchPosixLifecycleTests ArchPosixLifecycleTests.cpp)
  target_link_libraries(ArchPosixLifecycleTests PRIVATE arch base Qt::Core)
  set_target_properties(ArchPosixLifecycleTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  set_property(GLOBAL APPEND PROPERTY ${CMAKE_PROJECT_NAME}_tests ArchPosixLifecycleTests)

  # A fresh process owns each Arch lifecycle, including its normal destructor.
  foreach(case IN ITEMS
      main-only main-poll managed-repeat first-unblock concurrent-wait signal-stop wait-tls cross-worker-tls-wait
      cooperative-cancel concurrent-first-unblock tls-first-poll
      foreign-std foreign-qt foreign-outlives-arch
      pipe-create-failure pipe-read-flags-failure pipe-write-flags-failure
      full-wake-pipe startup-1 startup-2)
    add_test(NAME ArchPosixLifecycleTests_${case}
      COMMAND $<TARGET_FILE:ArchPosixLifecycleTests> ${case}
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/arch")
    set_tests_properties(ArchPosixLifecycleTests_${case} PROPERTIES TIMEOUT 60)
  endforeach()
endif()
