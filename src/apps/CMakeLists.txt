# SPDX-FileCopyrightText: 2024 Deskflow Developers
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

if(UNIX AND NOT APPLE)
  find_program(HELP2MAN help2man)
  if(NOT HELP2MAN)
    message(STATUS "Man page tool (help2man) not found, man pages will not be generated")
  endif()
endif()

function(generate_app_man TARGET NAME)
  if(HELP2MAN)
    add_custom_command(
      TARGET ${target} POST_BUILD
        COMMAND QT_QPA_PLATFORM=minimal PATH=$<TARGET_FILE_DIR:${target}>:${PATH} ${HELP2MAN}
          --name ${NAME}
          --include ${CMAKE_SOURCE_DIR}/src/apps/res/manpage.txt
          --no-info
          ${target}
          -o $<TARGET_FILE_DIR:${target}>/${target}.1
    )
    install(
      FILES $<TARGET_FILE_DIR:${target}>/${target}.1
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )
  endif()
endfunction()

set(WIN32_PRE_EXCLUDE_REGEXES
    "api-ms-win-.*"
    "ext-ms-.*"
    "^hvsifiletrust\\.dll$"
    "^icu.*\\.dll$"
)

set(WIN32_POST_EXCLUDE_REGEXES ".*system32.*")

add_subdirectory(deskflow-core)
add_subdirectory(deskflow-daemon) #Only used on windows
add_subdirectory(deskflow-gui)
