# Version number for Synergy+
SET(VERSION_MAJOR 1)
SET(VERSION_MINOR 3)
SET(VERSION_REV 5)
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}")


# The check for 2.6 may be too strict (consider lowering).
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)

# CMake complains if we don't have this.
IF(COMMAND cmake_policy)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

# We're escaping quotes in the Windows version number, because
# for some reason CMake won't do it at config version 2.4.7
# It seems that this restores the newer behaviour where define
# args are not auto-escaped.
IF(COMMAND cmake_policy)
	CMAKE_POLICY(SET CMP0005 NEW)
ENDIF(COMMAND cmake_policy)

# First, declare project (important for prerequisite checks).
PROJECT(synergy-plus C CXX)

# Set some easy to type variables.
SET(root_dir ${CMAKE_SOURCE_DIR})
SET(cmake_dir ${root_dir}/cmake)
SET(bin_dir ${root_dir}/bin)

# Now for the stuff to generate config.h (and setup defines).
INCLUDE(${cmake_dir}/CMakeLists_config.txt)

# Now for all the executables and libraries.
INCLUDE(${cmake_dir}/CMakeLists_lib.txt)
INCLUDE(${cmake_dir}/CMakeLists_synergyc.txt)
INCLUDE(${cmake_dir}/CMakeLists_synergys.txt)
INCLUDE(${cmake_dir}/CMakeLists_launcher.txt)

# Setup the CPack config.
INCLUDE(${cmake_dir}/CMakeLists_cpack.txt)

IF(WIN32)
	# add /analyze in order to unconver potential bugs in the source code
	# Details: http://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /analyze")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /analyze")

	# this line removes "/D NDEBUG" from release, we want them in order to find bugs even on release builds.
	SET(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2")
ENDIF(WIN32)


