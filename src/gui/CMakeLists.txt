cmake_minimum_required (VERSION 3.4)

find_package (Qt5 COMPONENTS Core Widgets Network LinguistTools)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

file (GLOB LEGACY_GUI_SOURCE_FILES src/*.cpp src/*.h)
file (GLOB LEGACY_GUI_UI_FILES src/*.ui)
file (GLOB LEGACY_ACTIVATION_FILES src/*Activation* src/*License*)
file (GLOB LEGACY_ZEROCONF_FILES src/Zeroconf*)
file (GLOB LEGACY_LANGUAGE_FILES res/lang/*.ts)

configure_file(res/Languages.qrc res/Languages.qrc)

set_source_files_properties(${LEGACY_LANGUAGE_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/res/lang")

qt5_add_translation(QM_FILES ${LEGACY_LANGUAGE_FILES})

add_custom_target(translations ALL DEPENDS ${QM_FILES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    file (GLOB LEGACY_GUI_MAC_SOURCE_FILES src/*.mm)
    list (APPEND LEGACY_GUI_SOURCE_FILES ${LEGACY_GUI_MAC_SOURCE_FILES})
endif()

# Retrieve the absolute path to qmake and then use that path to find
# the binaries
get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

if (SYNERGY_ENTERPRISE)
    list (REMOVE_ITEM LEGACY_GUI_SOURCE_FILES ${LEGACY_ACTIVATION_FILES})
    list (REMOVE_ITEM LEGACY_GUI_UI_FILES ${LEGACY_ACTIVATION_FILES})
    list (REMOVE_ITEM LEGACY_GUI_SOURCE_FILES ${LEGACY_ZEROCONF_FILES})
endif ()

if (WIN32)
    set (LEGACY_GUI_RC_FILES res/win/Synergy.rc)
endif()

add_executable (synergy WIN32
    ${LEGACY_GUI_SOURCE_FILES}
    ${LEGACY_GUI_UI_FILES}
    ${LEGACY_GUI_RC_FILES}
    res/Synergy.qrc
    ${CMAKE_CURRENT_BINARY_DIR}/res/Languages.qrc
    ${QM_FILES}
)

include_directories (./src)
target_link_libraries (synergy shared)

if (NOT SYNERGY_ENTERPRISE)
if (WIN32)
    include_directories ($ENV{BONJOUR_SDK_HOME}/Include)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library (DNSSD_LIB dnssd.lib
                  HINTS ENV BONJOUR_SDK_HOME
                  PATH_SUFFIXES "Lib/x64")
    else()
        find_library (DNSSD_LIB dnssd.lib
                  HINTS ENV BONJOUR_SDK_HOME
                  PATH_SUFFIXES "Lib/Win32")
    endif()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries (synergy dns_sd)
endif()
endif()

qt5_use_modules (synergy Core Widgets Network)
target_compile_definitions (synergy PRIVATE -DSYNERGY_VERSION_STAGE="${SYNERGY_VERSION_STAGE}")
target_compile_definitions (synergy PRIVATE -DSYNERGY_REVISION="${SYNERGY_REVISION}")

if (WIN32)
if (NOT SYNERGY_ENTERPRISE)
    target_link_libraries (synergy ${DNSSD_LIB})
endif ()
    set_target_properties (synergy PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install (TARGETS synergy DESTINATION ${SYNERGY_BUNDLE_BINARY_DIR})
    install (CODE "MESSAGE (\"Running macdeployqt to install frameworks in bundle\")")
    install (CODE "execute_process(COMMAND ${MACDEPLOYQT_EXECUTABLE} 
             ${SYNERGY_BUNDLE_APP_DIR} -always-overwrite)")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install (TARGETS synergy DESTINATION bin)
endif()

if (WIN32)
    if(Qt5_FOUND AND WIN32 AND TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
        get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

        execute_process(
                COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
                RESULT_VARIABLE return_code
                OUTPUT_VARIABLE qt5_install_prefix
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

        if(EXISTS ${imported_location})
            add_executable(Qt5::windeployqt IMPORTED)

            set_target_properties(Qt5::windeployqt PROPERTIES
                    IMPORTED_LOCATION ${imported_location}
                    )
        endif()
    endif()
    if(TARGET Qt5::windeployqt)
        # execute windeployqt in a tmp directory after build
        add_custom_command(TARGET synergy
                POST_BUILD
                COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
                COMMAND Qt5::windeployqt "$<TARGET_FILE_DIR:synergy>/$<TARGET_FILE_NAME:synergy>"
                )
    endif()
endif()