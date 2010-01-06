# - Macro to provide an option dependent on other options.
# This macro presents an option to the user only if a set of other
# conditions are true.  When the option is not presented a default
# value is used, but any value set by the user is preserved for when
# the option is presented again.
# Example invocation:
#  CMAKE_DEPENDENT_OPTION(USE_FOO "Use Foo" ON
#                         "USE_BAR;NOT USE_ZOT" OFF)
# If USE_BAR is true and USE_ZOT is false, this provides an option called
# USE_FOO that defaults to ON.  Otherwise, it sets USE_FOO to OFF.  If
# the status of USE_BAR or USE_ZOT ever changes, any value for the
# USE_FOO option is saved so that when the option is re-enabled it
# retains its old value.

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

MACRO(CMAKE_DEPENDENT_OPTION option doc default depends force)
  IF(${option}_ISSET MATCHES "^${option}_ISSET$")
    SET(${option}_AVAILABLE 1)
    FOREACH(d ${depends})
      STRING(REGEX REPLACE " +" ";" CMAKE_DEPENDENT_OPTION_DEP "${d}")
      IF(${CMAKE_DEPENDENT_OPTION_DEP})
      ELSE(${CMAKE_DEPENDENT_OPTION_DEP})
        SET(${option}_AVAILABLE 0)
      ENDIF(${CMAKE_DEPENDENT_OPTION_DEP})
    ENDFOREACH(d)
    IF(${option}_AVAILABLE)
      OPTION(${option} "${doc}" "${default}")
      SET(${option} "${${option}}" CACHE BOOL "${doc}" FORCE)
    ELSE(${option}_AVAILABLE)
      IF(${option} MATCHES "^${option}$")
      ELSE(${option} MATCHES "^${option}$")
        SET(${option} "${${option}}" CACHE INTERNAL "${doc}")
      ENDIF(${option} MATCHES "^${option}$")
      SET(${option} ${force})
    ENDIF(${option}_AVAILABLE)
  ELSE(${option}_ISSET MATCHES "^${option}_ISSET$")
    SET(${option} "${${option}_ISSET}")
  ENDIF(${option}_ISSET MATCHES "^${option}_ISSET$")
ENDMACRO(CMAKE_DEPENDENT_OPTION)
