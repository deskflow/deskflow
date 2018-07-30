cmake_minimum_required (VERSION 3.4)

find_package (Qt5 COMPONENTS Core Widgets Network)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

file (GLOB LEGACY_GUI_SOURCE_FILES src/*.cpp src/*.h)
file (GLOB LEGACY_GUI_UI_FILES src/*.ui)
file (GLOB LEGACY_ACTIVATION_FILES src/*Activation* src/*License*)
file (GLOB LEGACY_ZEROCONF_FILES src/Zeroconf*)

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
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install (TARGETS synergy DESTINATION bin)
endif()
