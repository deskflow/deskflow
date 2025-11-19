# SPDX-FileCopyrightText: Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)
option(CLEAN_TRS "Clean obsolete translations from tr files" OFF)
find_package(Qt6 ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE COMPONENTS LinguistTools)

# To add a new language Add 639 shortname
set (${CMAKE_PROJECT_NAME}_TRS
  ${CMAKE_PROJECT_NAME}_es.ts
  ${CMAKE_PROJECT_NAME}_it.ts
  ${CMAKE_PROJECT_NAME}_ja.ts
  ${CMAKE_PROJECT_NAME}_zh_CN.ts
  ${CMAKE_PROJECT_NAME}_ru.ts
)

set(TR_OPTIONS -no-ui-lines -locations none -silent)
if(CLEAN_TRS)
  list(APPEND TR_OPTIONS -no-obsolete)
endif()

# English will have only plurals
qt_create_translation(TRS ${CMAKE_SOURCE_DIR}/src ${CMAKE_PROJECT_NAME}_en.ts OPTIONS -pluralonly ${TR_OPTIONS})

# Other languages contain the full set of strings.
qt_create_translation(TRS ${CMAKE_SOURCE_DIR}/src ${${CMAKE_PROJECT_NAME}_TRS} OPTIONS ${TR_OPTIONS})

#ensure that the targets are built always
add_custom_target(app_translations ALL DEPENDS ${TRS})

# install our translations
install(FILES ${TRS} DESTINATION ${CMAKE_INSTALL_I18N_DIR})

if(APPLE)
  # find the qt translation files not deployed by macdeployqt
  get_target_property(lupdate_executable Qt6::lupdate IMPORTED_LOCATION)
  get_filename_component(_qt_bin_dir "${lupdate_executable}" DIRECTORY)
  file(REAL_PATH "${_qt_bin_dir}/../" QT_ROOT_DIR)
  find_file(_QT_QM_FILE NAMES qtbase_en.qm PATHS ${QT_ROOT_DIR} PATH_SUFFIXES "translations" "share/qt/translations" REQUIRED)
  get_filename_component(MAC_QT_LANG_PATH "${_QT_QM_FILE}" DIRECTORY)

  # install each lang file and rename to the same name
  # rename does not work with a list so foreach is used instead
  install(FILES ${MAC_QT_LANG_PATH}/qtbase_en.qm DESTINATION ${CMAKE_INSTALL_I18N_DIR} RENAME qt_en.qm)
  foreach(LANG ${${CMAKE_PROJECT_NAME}_TRS})
    string(REPLACE "${CMAKE_PROJECT_NAME}_" "" LANG ${LANG})
    string(REPLACE ".ts" "" LANG ${LANG})
    install(FILES ${MAC_QT_LANG_PATH}/qtbase_${LANG}.qm DESTINATION ${CMAKE_INSTALL_I18N_DIR} RENAME qt_${LANG}.qm)
  endforeach()
endif()
