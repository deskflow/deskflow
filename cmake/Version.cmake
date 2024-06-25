#
# Gets the version number either from an env var or from the VERSION file.
#

if (DEFINED ENV{SYNERGY_VERSION})
    set (VERSION $ENV{SYNERGY_VERSION})
else()
    file(READ "${CMAKE_SOURCE_DIR}/VERSION" VERSION)
endif()

string(STRIP "${VERSION}" VERSION)
message (STATUS "Version number: " ${VERSION})
add_definitions (-DSYNERGY_VERSION="${VERSION}")
