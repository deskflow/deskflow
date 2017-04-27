cmake_minimum_required (VERSION 3.4)

find_package (Qt5 COMPONENTS Core Widgets Network)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

file (GLOB LEGACY_GUI_SOURCE_FILES src/*.cpp src/*.h)
file (GLOB LEGACY_GUI_UI_FILES src/*.ui)

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

if (WIN32)
    include_directories ($ENV{BONJOUR_SDK_HOME}/Include)
    find_library (DNSSD_LIB dnssd.lib 
                  HINTS ENV BONJOUR_SDK_HOME
                  PATH_SUFFIXES "Lib/x64")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries (synergy dns_sd)
endif()

qt5_use_modules (synergy Core Widgets Network)
target_compile_definitions (synergy PRIVATE -DSYNERGY_VERSION_STAGE="${SYNERGY_VERSION_STAGE}")
target_compile_definitions (synergy PRIVATE -DSYNERGY_REVISION="${SYNERGY_REVISION}")

if (WIN32)
    target_link_libraries (synergy ${DNSSD_LIB})
    set_target_properties (synergy PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install (TARGETS synergy DESTINATION ${SYNERGY_BUNDLE_BINARY_DIR})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install (TARGETS synergy DESTINATION bin)
endif()
