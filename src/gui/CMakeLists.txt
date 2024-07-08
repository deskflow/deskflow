find_package(
  Qt6
  COMPONENTS Core Widgets Network Core5Compat
  REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB GUI_SOURCE_FILES src/*.cpp src/*.h src/validators/* src/widgets/*)
file(GLOB GUI_UI_FILES src/*.ui)
file(GLOB ACTIVATION_FILES src/*Activation* src/*License*)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  file(GLOB GUI_MAC_SOURCE_FILES src/*.mm)
  list(APPEND GUI_SOURCE_FILES ${GUI_MAC_SOURCE_FILES})
endif()

# Retrieve the absolute path to qmake and then use that path to find the
# binaries
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

if(NOT ENABLE_LICENSING)
  list(REMOVE_ITEM GUI_SOURCE_FILES ${ACTIVATION_FILES})
  list(REMOVE_ITEM GUI_UI_FILES ${ACTIVATION_FILES})
endif()

if(WIN32)
  set(GUI_RC_FILES res/win/Synergy.rc ${CMAKE_BINARY_DIR}/src/version.rc)
endif()

add_executable(synergy WIN32 ${GUI_SOURCE_FILES} ${GUI_UI_FILES}
                             ${GUI_RC_FILES} res/Synergy.qrc ${QM_FILES})

include_directories(./src)
target_link_libraries(synergy shared)

target_link_libraries(synergy Qt6::Core Qt6::Widgets Qt6::Network
                      Qt6::Core5Compat)
target_compile_definitions(
  synergy PRIVATE -DSYNERGY_VERSION_STAGE="${SYNERGY_VERSION_STAGE}")
target_compile_definitions(synergy
                           PRIVATE -DSYNERGY_REVISION="${SYNERGY_REVISION}")

if(WIN32)
  set_target_properties(synergy PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  install(TARGETS synergy DESTINATION ${SYNERGY_BUNDLE_BINARY_DIR})
  install(
    CODE "MESSAGE (\"Running macdeployqt to install frameworks in bundle\")")
  install(
    CODE "execute_process(COMMAND ${MACDEPLOYQT_EXECUTABLE} ${SYNERGY_BUNDLE_APP_DIR} -always-overwrite)"
  )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  install(TARGETS synergy DESTINATION bin)
endif()

if(WIN32)
  if(Qt6_FOUND
     AND WIN32
     AND TARGET Qt6::qmake
     AND NOT TARGET Qt6::windeployqt)
    get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)

    execute_process(
      COMMAND "${_qt6_qmake_location}" -query QT_INSTALL_PREFIX
      RESULT_VARIABLE return_code
      OUTPUT_VARIABLE qt6_install_prefix
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(imported_location "${qt6_install_prefix}/bin/windeployqt.exe")

    if(EXISTS ${imported_location})
      add_executable(Qt6::windeployqt IMPORTED)

      set_target_properties(Qt6::windeployqt PROPERTIES IMPORTED_LOCATION
                                                        ${imported_location})
    endif()
  endif()
  if(TARGET Qt6::windeployqt)
    # execute windeployqt in a tmp directory after build
    add_custom_command(
      TARGET synergy
      POST_BUILD
      COMMAND set PATH=%PATH%$<SEMICOLON>${qt6_install_prefix}/bin
      COMMAND Qt6::windeployqt
              "$<TARGET_FILE_DIR:synergy>/$<TARGET_FILE_NAME:synergy>")
  endif()
endif()
