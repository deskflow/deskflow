# - The builtin (binary) CPack RPM generator (Unix only)
# CPackRPM may be used to create RPM package using CPack.
# CPackRPM is a CPack generator thus it uses the CPACK_XXX variables
# used by CPack : http://www.cmake.org/Wiki/CMake:CPackConfiguration
#
# However CPackRPM has specific features which are controlled by
# the specifics CPACK_RPM_XXX variables.You'll find a detailed usage on 
# the wiki: 
#  http://www.cmake.org/Wiki/CMake:CPackPackageGenerators#RPM_.28Unix_Only.29
# However as a handy reminder here comes the list of specific variables:
#
#  CPACK_RPM_PACKAGE_SUMMARY 
#     Mandatory : YES
#     Default   : CPACK_PACKAGE_DESCRIPTION
#     The RPM package summary
#  CPACK_RPM_PACKAGE_NAME
#     Mandatory : YES
#     Default   : CPACK_PACKAGE_NAME
#     The RPM package name
#  CPACK_RPM_PACKAGE_VERSION
#     Mandatory : YES
#     Default   : CPACK_PACKAGE_VERSION
#     The RPM package version
#  CPACK_RPM_PACKAGE_ARCHITECTURE
#     Mandatory : NO
#     Default   : -
#     The RPM package architecture. This may be set to "noarch" if you 
#     know you are building a noarch package.
#  CPACK_RPM_PACKAGE_RELEASE
#     Mandatory : YES
#     Default   : 1
#     The RPM package release. This is the numbering of the RPM package 
#     itself, i.e. the version of the packaging and not the version of the 
#     content (see CPACK_RPM_PACKAGE_VERSION). One may change the default 
#     value if the previous packaging was buggy and/or you want to put here
#     a fancy Linux distro specific numbering.
#  CPACK_RPM_PACKAGE_LICENSE
#     Mandatory : YES
#     Default   : "unknown"
#     The RPM package license policy.
#  CPACK_RPM_PACKAGE_GROUP
#     Mandatory : YES
#     Default   : "unknown"
#     The RPM package group.
#  CPACK_RPM_PACKAGE_VENDOR 
#     Mandatory : YES
#     Default   : CPACK_PACKAGE_VENDOR if set or"unknown"
#     The RPM package group.
#  CPACK_RPM_PACKAGE_DESCRIPTION
#     Mandatory : YES
#     Default   : CPACK_PACKAGE_DESCRIPTION_FILE if set or "no package description available"
#  CPACK_RPM_PACKAGE_REQUIRES
#     Mandatory : NO
#     Default   : -
#     May be used to set RPM dependencies. 
#  CPACK_RPM_SPEC_INSTALL_POST
#     Mandatory : NO
#     Default   : -
#     May be used to set an RPM post-install command inside the spec file. 
#     For example setting it to "/bin/true" may be used to prevent 
#     rpmbuild to strip binaries.
#  CPACK_RPM_SPEC_MORE_DEFINE
#     Mandatory : NO
#     Default   : -
#     May be used to add any %define lines to the generated spec file.
#  CPACK_RPM_PACKAGE_DEBUG
#     Mandatory : NO
#     Default   : -
#     May be set when invoking cpack in order to trace debug informations 
#     during CPack RPM run. For example you may launch CPack like this 
#     cpack -D CPACK_RPM_PACKAGE_DEBUG=1 -G RPM

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

# Author: Eric Noulard with the help of Alexander Neundorf.

IF(CMAKE_BINARY_DIR)
  MESSAGE(FATAL_ERROR "CPackRPM.cmake may only be used by CPack internally.")
ENDIF(CMAKE_BINARY_DIR)

IF(NOT UNIX)
  MESSAGE(FATAL_ERROR "CPackRPM.cmake may only be used under UNIX.")
ENDIF(NOT UNIX)

# rpmbuild is the basic command for building RPM package
# it may be a simple (symbolic) link to rpmb command.
FIND_PROGRAM(RPMBUILD_EXECUTABLE rpmbuild)

# Check version of the rpmbuild tool this would be easier to 
# track bugs with users and CPackRPM debug mode.
# We may use RPM version in order to check for available version dependent features 
IF(RPMBUILD_EXECUTABLE)
  execute_process(COMMAND ${RPMBUILD_EXECUTABLE} --version
                  OUTPUT_VARIABLE _TMP_VERSION
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE "^.*\ " ""   
         RPMBUILD_EXECUTABLE_VERSION
         ${_TMP_VERSION})     
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: rpmbuild version is <${RPMBUILD_EXECUTABLE_VERSION}>")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)  
ENDIF(RPMBUILD_EXECUTABLE)

IF(NOT RPMBUILD_EXECUTABLE)
  MESSAGE(FATAL_ERROR "RPM package requires rpmbuild executable")
ENDIF(NOT RPMBUILD_EXECUTABLE)

# We may use RPM version in the future in order
# to shut down warning about space in buildtree 
# some recent RPM version should support space in different places.
# not checked [yet].
IF(CPACK_TOPLEVEL_DIRECTORY MATCHES ".* .*")
  MESSAGE(FATAL_ERROR "${RPMBUILD_EXECUTABLE} can't handle paths with spaces, use a build directory without spaces for building RPMs.")
ENDIF(CPACK_TOPLEVEL_DIRECTORY MATCHES ".* .*")

# If rpmbuild is found 
# we try to discover alien since we may be on non RPM distro like Debian.
# In this case we may try to to use more advanced features
# like generating RPM directly from DEB using alien.
# FIXME feature not finished (yet)
FIND_PROGRAM(ALIEN_EXECUTABLE alien)
IF(ALIEN_EXECUTABLE)
  MESSAGE(STATUS "alien found, we may be on a Debian based distro.")
ENDIF(ALIEN_EXECUTABLE)

# 
# Use user-defined RPM specific variables value
# or generate reasonable default value from
# CPACK_xxx generic values.
# The variables comes from the needed (mandatory or not)
# values found in the RPM specification file aka ".spec" file.
# The variables which may/should be defined are:
#

# CPACK_RPM_PACKAGE_SUMMARY (mandatory)
IF(NOT CPACK_RPM_PACKAGE_SUMMARY)
  # if neither var is defined lets use the name as summary
  IF(NOT CPACK_PACKAGE_DESCRIPTION_SUMMARY)
    STRING(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_RPM_PACKAGE_SUMMARY)
  ELSE(NOT CPACK_PACKAGE_DESCRIPTION_SUMMARY)
    SET(CPACK_RPM_PACKAGE_SUMMARY ${CPACK_PACKAGE_DESCRIPTION_SUMMARY})    
  ENDIF(NOT CPACK_PACKAGE_DESCRIPTION_SUMMARY)
ENDIF(NOT CPACK_RPM_PACKAGE_SUMMARY)
 
# CPACK_RPM_PACKAGE_NAME (mandatory)
IF(NOT CPACK_RPM_PACKAGE_NAME)
  STRING(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_RPM_PACKAGE_NAME)
ENDIF(NOT CPACK_RPM_PACKAGE_NAME)

# CPACK_RPM_PACKAGE_VERSION (mandatory)
IF(NOT CPACK_RPM_PACKAGE_VERSION)
  IF(NOT CPACK_PACKAGE_VERSION)
    MESSAGE(FATAL_ERROR "RPM package requires a package version")
  ENDIF(NOT CPACK_PACKAGE_VERSION)
  SET(CPACK_RPM_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
ENDIF(NOT CPACK_RPM_PACKAGE_VERSION)

# CPACK_RPM_PACKAGE_ARCHITECTURE (optional)
IF(CPACK_RPM_PACKAGE_ARCHITECTURE)
  SET(TMP_RPM_BUILDARCH "Buildarch: ${CPACK_RPM_PACKAGE_ARCHITECTURE}")
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: using user-specified build arch = ${CPACK_RPM_PACKAGE_ARCHITECTURE}")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)
ELSE(CPACK_RPM_PACKAGE_ARCHITECTURE)
  SET(TMP_RPM_BUILDARCH "")
ENDIF(CPACK_RPM_PACKAGE_ARCHITECTURE)

# CPACK_RPM_PACKAGE_RELEASE
# The RPM release is the numbering of the RPM package ITSELF
# this is the version of the PACKAGING and NOT the version
# of the CONTENT of the package.
# You may well need to generate a new RPM package release
# without changing the version of the packaged software.
# This is the case when the packaging is buggy (not) the software :=)
# If not set, 1 is a good candidate
IF(NOT CPACK_RPM_PACKAGE_RELEASE)
  SET(CPACK_RPM_PACKAGE_RELEASE 1)
ENDIF(NOT CPACK_RPM_PACKAGE_RELEASE)

# CPACK_RPM_PACKAGE_LICENSE
IF(NOT CPACK_RPM_PACKAGE_LICENSE)
  SET(CPACK_RPM_PACKAGE_LICENSE "unknown")
ENDIF(NOT CPACK_RPM_PACKAGE_LICENSE)

# CPACK_RPM_PACKAGE_GROUP
IF(NOT CPACK_RPM_PACKAGE_GROUP)
  SET(CPACK_RPM_PACKAGE_GROUP "unknown")
ENDIF(NOT CPACK_RPM_PACKAGE_GROUP)

# CPACK_RPM_PACKAGE_VENDOR
IF(NOT CPACK_RPM_PACKAGE_VENDOR)
  IF(CPACK_PACKAGE_VENDOR)
    SET(CPACK_RPM_PACKAGE_VENDOR "${CPACK_PACKAGE_VENDOR}")
  ELSE(CPACK_PACKAGE_VENDOR)
    SET(CPACK_RPM_PACKAGE_VENDOR "unknown")
  ENDIF(CPACK_PACKAGE_VENDOR)
ENDIF(NOT CPACK_RPM_PACKAGE_VENDOR)

# CPACK_RPM_PACKAGE_SOURCE
# The name of the source tarball in case we generate a source RPM

# CPACK_RPM_PACKAGE_DESCRIPTION
# The variable content may be either
#   - explicitely given by tthe user or
#   - filled with the content of CPACK_PACKAGE_DESCRIPTION_FILE
#     if it is defined
#   - set to a default value
#
IF (NOT CPACK_RPM_PACKAGE_DESCRIPTION)
        IF (CPACK_PACKAGE_DESCRIPTION_FILE)
                FILE(READ ${CPACK_PACKAGE_DESCRIPTION_FILE} CPACK_RPM_PACKAGE_DESCRIPTION)
        ELSE (CPACK_PACKAGE_DESCRIPTION_FILE)
                SET(CPACK_RPM_PACKAGE_DESCRIPTION "no package description available")
        ENDIF (CPACK_PACKAGE_DESCRIPTION_FILE)
ENDIF (NOT CPACK_RPM_PACKAGE_DESCRIPTION)

# CPACK_RPM_PACKAGE_REQUIRES
# Placeholder used to specify binary RPM dependencies (if any)
# see http://www.rpm.org/max-rpm/s1-rpm-depend-manual-dependencies.html
IF(CPACK_RPM_PACKAGE_REQUIRES)
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: User defined Requires:\n ${CPACK_RPM_PACKAGE_REQUIRES}")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)
  SET(TMP_RPM_REQUIRES "Requires: ${CPACK_RPM_PACKAGE_REQUIRES}")
ENDIF(CPACK_RPM_PACKAGE_REQUIRES)

# CPACK_RPM_SPEC_INSTALL_POST
# May be used to define a RPM post intallation script
# for example setting it to "/bin/true" may prevent
# rpmbuild from stripping binaries.
IF(CPACK_RPM_SPEC_INSTALL_POST)
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: User defined CPACK_RPM_SPEC_INSTALL_POST = ${CPACK_RPM_SPEC_INSTALL_POST}")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)
  SET(TMP_RPM_SPEC_INSTALL_POST "%define __spec_install_post ${CPACK_RPM_SPEC_INSTALL_POST}")
ENDIF(CPACK_RPM_SPEC_INSTALL_POST)

# CPACK_RPM_SPEC_MORE_DEFINE
# This is a generated spec rpm file spaceholder
IF(CPACK_RPM_SPEC_MORE_DEFINE)
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: User defined more define spec line specified:\n ${CPACK_RPM_SPEC_MORE_DEFINE}")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)
ENDIF(CPACK_RPM_SPEC_MORE_DEFINE)

# CPACK_RPM_USER_BINARY_SPECFILE 
# FIXME when this is set then CPack should us the 
# user provided file.

# Now we may create the RPM build tree structure
SET(CPACK_RPM_ROOTDIR "${CPACK_TOPLEVEL_DIRECTORY}")
MESSAGE(STATUS "CPackRPM:Debug: Using CPACK_RPM_ROOTDIR=${CPACK_RPM_ROOTDIR}")
# Prepare RPM build tree
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR})
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR}/tmp)
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR}/BUILD)
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR}/RPMS)
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR}/SOURCES)
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR}/SPECS)
FILE(MAKE_DIRECTORY ${CPACK_RPM_ROOTDIR}/SRPMS)

#SET(CPACK_RPM_FILE_NAME "${CPACK_RPM_PACKAGE_NAME}-${CPACK_RPM_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}-${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
SET(CPACK_RPM_FILE_NAME "${CPACK_OUTPUT_FILE_NAME}")
# it seems rpmbuild can't handle spaces in the path
# neither escaping (as below) nor putting quotes around the path seem to help
#STRING(REGEX REPLACE " " "\\\\ " CPACK_RPM_DIRECTORY "${CPACK_TOPLEVEL_DIRECTORY}")
SET(CPACK_RPM_DIRECTORY "${CPACK_TOPLEVEL_DIRECTORY}")

# Use files tree to construct files command (spec file)
EXECUTE_PROCESS(COMMAND find -type f
               COMMAND sed {s/\\.//}
               WORKING_DIRECTORY "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}"
               OUTPUT_VARIABLE CPACK_RPM_INSTALL_FILES)
 
SET(CPACK_RPM_BINARY_SPECFILE "${CPACK_RPM_ROOTDIR}/SPECS/${CPACK_RPM_PACKAGE_NAME}.spec")
IF(CPACK_RPM_USER_BINARY_SPECFILE)
  # User may have specified SPECFILE just use it
  MESSAGE("CPackRPM: Will use user specified spec file: ${CPACK_RPM_USER_BINARY_SPECFILE}")
  # Note that user provided file is processed for @var replacement
  CONFIGURE_FILE(${CPACK_RPM_USER_BINARY_SPECFILE} ${CPACK_RPM_BINARY_SPECFILE} @ONLY)
ELSE(CPACK_RPM_USER_BINARY_SPECFILE)
  # No User specified spec file generate a valid one using var values
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: CPACK_TOPLEVEL_DIRECTORY          = ${CPACK_TOPLEVEL_DIRECTORY}")
    MESSAGE("CPackRPM:Debug: CPACK_TOPLEVEL_TAG                = ${CPACK_TOPLEVEL_TAG}")
    MESSAGE("CPackRPM:Debug: CPACK_TEMPORARY_DIRECTORY         = ${CPACK_TEMPORARY_DIRECTORY}")
    MESSAGE("CPackRPM:Debug: CPACK_OUTPUT_FILE_NAME            = ${CPACK_OUTPUT_FILE_NAME}")
    MESSAGE("CPackRPM:Debug: CPACK_OUTPUT_FILE_PATH            = ${CPACK_OUTPUT_FILE_PATH}")
    MESSAGE("CPackRPM:Debug: CPACK_PACKAGE_FILE_NAME           = ${CPACK_PACKAGE_FILE_NAME}")
    MESSAGE("CPackRPM:Debug: CPACK_RPM_BINARY_SPECFILE         = ${CPACK_RPM_BINARY_SPECFILE}")
    MESSAGE("CPackRPM:Debug: CPACK_PACKAGE_INSTALL_DIRECTORY   = ${CPACK_PACKAGE_INSTALL_DIRECTORY}")
    MESSAGE("CPackRPM:Debug: CPACK_TEMPORARY_PACKAGE_FILE_NAME = ${CPACK_TEMPORARY_PACKAGE_FILE_NAME}")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)
  FILE(WRITE ${CPACK_RPM_BINARY_SPECFILE}
    "# -*- rpm-spec -*-
Buildroot:      ${CPACK_RPM_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}
Summary:        ${CPACK_RPM_PACKAGE_SUMMARY}
Name:           ${CPACK_RPM_PACKAGE_NAME}
Version:        ${CPACK_RPM_PACKAGE_VERSION}
Release:        ${CPACK_RPM_PACKAGE_RELEASE}
License:        ${CPACK_RPM_PACKAGE_LICENSE}
Group:          ${CPACK_RPM_PACKAGE_GROUP}
Vendor:         ${CPACK_RPM_PACKAGE_VENDOR}
${TMP_RPM_REQUIRES}
${TMP_RPM_BUILDARCH}

#p define prefix ${CMAKE_INSTALL_PREFIX}
%define _rpmdir ${CPACK_RPM_DIRECTORY}
%define _rpmfilename ${CPACK_RPM_FILE_NAME}
%define _unpackaged_files_terminate_build 0
%define _topdir ${CPACK_RPM_DIRECTORY}
${TMP_RPM_SPEC_INSTALL_POST}
${CPACK_RPM_SPEC_MORE_DEFINE}

%description
${CPACK_RPM_PACKAGE_DESCRIPTION}

# This is a shortcutted spec file
# generated by CMake RPM generator
# we skip the _prepn _build and _install
# steps because CPack does that for us
# WE MUST NOT DO ANYTHING in those steps because they
# may fail for non-privileged user
#p prep

#p build
  
#p install

%clean

%files
%defattr(-,root,root,-)
#p dir %{prefix}
#p {prefix}/*
${CPACK_RPM_INSTALL_FILES}

%changelog
* Sat Oct 03 2009 Kami <cmoidavid@gmail.com>
  Update to handle more precisely the files section
* Mon Oct 03 2008 Erk <eric.noulard@gmail.com>
  Update generator to handle optional dependencies using Requires
  Update DEBUG output typos. 
* Mon Aug 25 2008 Erk <eric.noulard@gmail.com>
  Update generator to handle optional post-install
* Tue Aug 16 2007 Erk <eric.noulard@gmail.com>
  Generated by CPack RPM Generator and associated macros
")
ENDIF(CPACK_RPM_USER_BINARY_SPECFILE)


IF(RPMBUILD_EXECUTABLE)
  # Now call rpmbuild using the SPECFILE
  EXECUTE_PROCESS(
    COMMAND "${RPMBUILD_EXECUTABLE}" -bb 
            --buildroot "${CPACK_RPM_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}" 
            "${CPACK_RPM_BINARY_SPECFILE}"
    WORKING_DIRECTORY "${CPACK_TOPLEVEL_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}"
    ERROR_FILE "${CPACK_TOPLEVEL_DIRECTORY}/rpmbuild.err"
    OUTPUT_FILE "${CPACK_TOPLEVEL_DIRECTORY}/rpmbuild.out")  
  IF(CPACK_RPM_PACKAGE_DEBUG)
    MESSAGE("CPackRPM:Debug: You may consult rpmbuild logs in: ")
    MESSAGE("CPackRPM:Debug:    - ${CPACK_TOPLEVEL_DIRECTORY}/rpmbuild.err")
    MESSAGE("CPackRPM:Debug:    - ${CPACK_TOPLEVEL_DIRECTORY}/rpmbuild.out")
  ENDIF(CPACK_RPM_PACKAGE_DEBUG)
ELSE(RPMBUILD_EXECUTABLE)
  IF(ALIEN_EXECUTABLE)
    MESSAGE(FATAL_ERROR "RPM packaging through alien not done (yet)")
  ENDIF(ALIEN_EXECUTABLE)
ENDIF(RPMBUILD_EXECUTABLE)
