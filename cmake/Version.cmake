#
# Parses the VERSION file, sets the version number define, and creates a .env.version file.
#

file(READ "${CMAKE_SOURCE_DIR}/VERSION" VERSION_CONTENTS)
string(STRIP "${VERSION_CONTENTS}" VERSION_CONTENTS)
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-*(.*)$" _ ${VERSION_CONTENTS})

set(VERSION_MAJOR "${CMAKE_MATCH_1}")
set(VERSION_MINOR "${CMAKE_MATCH_2}")
set(VERSION_PATCH "${CMAKE_MATCH_3}")
set(VERSION_STAGE "${CMAKE_MATCH_4}")

# CI should specify "snapshot" for the stage when not a release build.
if (DEFINED ENV{SYNERGY_VERSION_STAGE})
    set (VERSION_STAGE $ENV{SYNERGY_VERSION_STAGE})
endif()

# CI should provide the version number meta data.
if (DEFINED ENV{SYNERGY_VERSION_META})
    set (VERSION_META $ENV{SYNERGY_VERSION_META})
endif()

if (VERSION_META)
    set(SYNERGY_VERSION 
        "${VERSION_MAJOR}."
        "${VERSION_MINOR}."
        "${VERSION_PATCH}-"
        "${VERSION_STAGE}+"
        "${VERSION_META}"
    )
else()
    set(SYNERGY_VERSION 
        "${VERSION_MAJOR}."
        "${VERSION_MINOR}."
        "${VERSION_PATCH}-"
        "${VERSION_STAGE}"
    )
endif()


message (STATUS "Version number: " ${SYNERGY_VERSION})
message (STATUS "Build date: " ${SYNERGY_BUILD_DATE})

add_definitions (-DSYNERGY_VERSION="${SYNERGY_VERSION}")

file(WRITE ${CMAKE_BINARY_DIR}/.env.version
    "SYNERGY_VERSION_MAJOR=${VERSION_MAJOR}\n"
    "SYNERGY_VERSION_MINOR=${VERSION_MINOR}\n"
    "SYNERGY_VERSION_PATCH=${VERSION_PATCH}\n"
    "SYNERGY_VERSION_BUILD=${VERSION_BUILD}\n"
    "SYNERGY_VERSION_STAGE=${VERSION_STAGE}\n")
