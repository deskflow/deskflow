# This is a platform definition file for platforms without
# operating system, typically embedded platforms.
# It is used when CMAKE_SYSTEM_NAME is set to "Generic"
#
# It is intentionally empty, since nothing is known 
# about the platform. So everything has to be specified
# in the system/compiler files ${CMAKE_SYSTEM_NAME}-<compiler_basename>.cmake
# and/or ${CMAKE_SYSTEM_NAME}-<compiler_basename>-${CMAKE_SYSTEM_PROCESSOR}.cmake

# (embedded) targets without operating system usually don't support shared libraries
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
