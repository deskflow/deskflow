# OpenThreads is a C++ based threading library. Its largest userbase 
# seems to OpenSceneGraph so you might notice I accept OSGDIR as an
# environment path.
# I consider this part of the Findosg* suite used to find OpenSceneGraph 
# components.
# Each component is separate and you must opt in to each module.
# 
# Locate OpenThreads
# This module defines
# OPENTHREADS_LIBRARY
# OPENTHREADS_FOUND, if false, do not try to link to OpenThreads
# OPENTHREADS_INCLUDE_DIR, where to find the headers
#
# $OPENTHREADS_DIR is an environment variable that would
# correspond to the ./configure --prefix=$OPENTHREADS_DIR
# used in building osg.
#
# Created by Eric Wing.

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
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

# Header files are presumed to be included like
# #include <OpenThreads/Thread>

# To make it easier for one-step automated configuration/builds,
# we leverage environmental paths. This is preferable 
# to the -DVAR=value switches because it insulates the 
# users from changes we may make in this script.
# It also offers a little more flexibility than setting
# the CMAKE_*_PATH since we can target specific components.
# However, the default CMake behavior will search system paths
# before anything else. This is problematic in the cases
# where you have an older (stable) version installed, but
# are trying to build a newer version.
# CMake doesn't offer a nice way to globally control this behavior
# so we have to do a nasty "double FIND_" in this module.
# The first FIND disables the CMAKE_ search paths and only checks
# the environmental paths.
# If nothing is found, then the second find will search the
# standard install paths.
# Explicit -DVAR=value arguments should still be able to override everything.

find_path(OPENTHREADS_INCLUDE_DIR OpenThreads/Thread
    HINTS
        # enough environment variables?
        $ENV{OPENTHREADS_INCLUDE_DIR}
        $ENV{OPENTHREADS_DIR}
        $ENV{OSG_INCLUDE_DIR}
        $ENV{OSG_DIR}
        $ENV{OSGDIR}
        $ENV{OpenThreads_ROOT}
        $ENV{OSG_ROOT}
    PATHS
        /sw # Fink
        /opt/local # DarwinPorts
        /opt/csw # Blastwave
        /opt
        /usr/freeware
    PATH_SUFFIXES include
)


find_library(OPENTHREADS_LIBRARY 
    NAMES OpenThreads OpenThreadsWin32 
    HINTS
        $ENV{OPENTHREADS_LIBRARY_DIR}
        $ENV{OPENTHREADS_DIR}
        $ENV{OSG_LIBRARY_DIR}
        $ENV{OSG_DIR}
        $ENV{OSGDIR}
        $ENV{OpenThreads_ROOT}
        $ENV{OSG_ROOT}
    PATHS
        /sw
        /opt/local
        /opt/csw
        /opt
        /usr/freeware
    PATH_SUFFIXES lib64 lib
)

find_library(OPENTHREADS_LIBRARY_DEBUG 
    NAMES OpenThreadsd OpenThreadsWin32d
    HINTS
        $ENV{OPENTHREADS_DEBUG_LIBRARY_DIR}
        $ENV{OPENTHREADS_LIBRARY_DIR}
        $ENV{OPENTHREADS_DIR}
        $ENV{OSG_LIBRARY_DIR}
        $ENV{OSG_DIR}
        $ENV{OSGDIR}
        $ENV{OpenThreads_ROOT}
        $ENV{OSG_ROOT}
    PATHS
        /sw
        /opt/local
        /opt/csw
        /opt
        /usr/freeware
    PATH_SUFFIXES lib64 lib
)

if(OPENTHREADS_LIBRARY_DEBUG)
    set(OPENTHREADS_LIBRARIES
        optimized ${OPENTHREADS_LIBRARY}
        debug ${OPENTHREADS_LIBRARY_DEBUG})
else()
    set(OPENTHREADS_LIBRARIES ${OPENTHREADS_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenThreads DEFAULT_MSG
    OPENTHREADS_LIBRARY OPENTHREADS_INCLUDE_DIR)
