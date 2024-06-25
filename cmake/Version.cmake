#
# Gets the version number either from an env var or from the VERSION file.
#

set (VERSION $ENV{SYNERGY_VERSION})
string(STRIP "${VERSION}" VERSION)

if (NOT VERSION)
    file(READ "${CMAKE_SOURCE_DIR}/VERSION" VERSION)
endif()

message (STATUS "Version number: " ${VERSION})
add_definitions (-DSYNERGY_VERSION="${VERSION}")
