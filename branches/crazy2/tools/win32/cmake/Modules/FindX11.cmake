# - Find X11 installation
# Try to find X11 on UNIX systems. The following values are defined
#  X11_FOUND        - True if X11 is available
#  X11_INCLUDE_DIR  - include directories to use X11
#  X11_LIBRARIES    - link against these to use X11
#
# and also the following more fine grained variables:
# Include paths: X11_ICE_INCLUDE_PATH,          X11_ICE_LIB,        X11_ICE_FOUND
#                X11_X11_INCLUDE_PATH,          X11_X11_LIB
#                X11_Xaccessrules_INCLUDE_PATH,                     X11_Xaccess_FOUND
#                X11_Xaccessstr_INCLUDE_PATH,                       X11_Xaccess_FOUND
#                X11_Xau_INCLUDE_PATH,          X11_Xau_LIB,        X11_Xau_FOUND
#                X11_Xcomposite_INCLUDE_PATH,   X11_Xcomposite_LIB, X11_Xcomposite_FOUND
#                X11_Xcursor_INCLUDE_PATH,      X11_Xcursor_LIB,    X11_Xcursor_FOUND
#                X11_Xdamage_INCLUDE_PATH,      X11_Xdamage_LIB,    X11_Xdamage_FOUND
#                X11_Xdmcp_INCLUDE_PATH,        X11_Xdmcp_LIB,      X11_Xdmcp_FOUND
#                                               X11_Xext_LIB,       X11_Xext_FOUND
#                X11_dpms_INCLUDE_PATH,         (in X11_Xext_LIB),  X11_dpms_FOUND
#                X11_XShm_INCLUDE_PATH,         (in X11_Xext_LIB),  X11_XShm_FOUND
#                X11_Xshape_INCLUDE_PATH,       (in X11_Xext_LIB),  X11_Xshape_FOUND
#                X11_xf86misc_INCLUDE_PATH,     X11_Xxf86misc_LIB,  X11_xf86misc_FOUND
#                X11_xf86vmode_INCLUDE_PATH,                        X11_xf86vmode_FOUND
#                X11_Xfixes_INCLUDE_PATH,       X11_Xfixes_LIB,     X11_Xfixes_FOUND
#                X11_Xft_INCLUDE_PATH,          X11_Xft_LIB,        X11_Xft_FOUND
#                X11_Xinerama_INCLUDE_PATH,     X11_Xinerama_LIB,   X11_Xinerama_FOUND
#                X11_Xinput_INCLUDE_PATH,       X11_Xinput_LIB,     X11_Xinput_FOUND
#                X11_Xkb_INCLUDE_PATH,                              X11_Xkb_FOUND
#                X11_Xkblib_INCLUDE_PATH,                           X11_Xkb_FOUND
#                X11_Xpm_INCLUDE_PATH,          X11_Xpm_LIB,        X11_Xpm_FOUND
#                X11_XTest_INCLUDE_PATH,        X11_XTest_LIB,      X11_XTest_FOUND
#                X11_Xrandr_INCLUDE_PATH,       X11_Xrandr_LIB,     X11_Xrandr_FOUND
#                X11_Xrender_INCLUDE_PATH,      X11_Xrender_LIB,    X11_Xrender_FOUND
#                X11_Xscreensaver_INCLUDE_PATH, X11_Xscreensaver_LIB, X11_Xscreensaver_FOUND
#                X11_Xt_INCLUDE_PATH,           X11_Xt_LIB,         X11_Xt_FOUND
#                X11_Xutil_INCLUDE_PATH,                            X11_Xutil_FOUND
#                X11_Xv_INCLUDE_PATH,           X11_Xv_LIB,         X11_Xv_FOUND

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
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

IF (UNIX)
  SET(X11_FOUND 0)
  # X11 is never a framework and some header files may be
  # found in tcl on the mac
  SET(CMAKE_FIND_FRAMEWORK_SAVE ${CMAKE_FIND_FRAMEWORK})
  SET(CMAKE_FIND_FRAMEWORK NEVER)
  SET(X11_INC_SEARCH_PATH
    /usr/pkg/xorg/include
    /usr/X11R6/include 
    /usr/X11R7/include 
    /usr/include/X11
    /usr/openwin/include 
    /usr/openwin/share/include 
    /opt/graphics/OpenGL/include
  )

  SET(X11_LIB_SEARCH_PATH
    /usr/pkg/xorg/lib
    /usr/X11R6/lib
    /usr/X11R7/lib
    /usr/openwin/lib 
  )

  FIND_PATH(X11_X11_INCLUDE_PATH X11/X.h                             ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xlib_INCLUDE_PATH X11/Xlib.h                         ${X11_INC_SEARCH_PATH})

  # Look for includes; keep the list sorted by name of the cmake *_INCLUDE_PATH
  # variable (which doesn't need to match the include file name).
  
  # Solaris lacks XKBrules.h, so we should skip kxkbd there.
  FIND_PATH(X11_ICE_INCLUDE_PATH X11/ICE/ICE.h                       ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xaccessrules_INCLUDE_PATH X11/extensions/XKBrules.h  ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xaccessstr_INCLUDE_PATH X11/extensions/XKBstr.h      ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xau_INCLUDE_PATH X11/Xauth.h                         ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xcomposite_INCLUDE_PATH X11/extensions/Xcomposite.h  ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xcursor_INCLUDE_PATH X11/Xcursor/Xcursor.h           ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xdamage_INCLUDE_PATH X11/extensions/Xdamage.h        ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xdmcp_INCLUDE_PATH X11/Xdmcp.h                       ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_dpms_INCLUDE_PATH X11/extensions/dpms.h              ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_xf86misc_INCLUDE_PATH X11/extensions/xf86misc.h      ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_xf86vmode_INCLUDE_PATH X11/extensions/xf86vmode.h    ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xfixes_INCLUDE_PATH X11/extensions/Xfixes.h          ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xft_INCLUDE_PATH X11/Xft/Xft.h                       ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xinerama_INCLUDE_PATH X11/extensions/Xinerama.h      ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xinput_INCLUDE_PATH X11/extensions/XInput.h          ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xkb_INCLUDE_PATH X11/extensions/XKB.h                ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xkblib_INCLUDE_PATH X11/XKBlib.h                     ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xpm_INCLUDE_PATH X11/xpm.h                           ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_XTest_INCLUDE_PATH X11/extensions/XTest.h            ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_XShm_INCLUDE_PATH X11/extensions/XShm.h              ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xrandr_INCLUDE_PATH X11/extensions/Xrandr.h          ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xrender_INCLUDE_PATH X11/extensions/Xrender.h        ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xscreensaver_INCLUDE_PATH X11/extensions/scrnsaver.h ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xshape_INCLUDE_PATH X11/extensions/shape.h           ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xutil_INCLUDE_PATH X11/Xutil.h                       ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xt_INCLUDE_PATH X11/Intrinsic.h                      ${X11_INC_SEARCH_PATH})
  FIND_PATH(X11_Xv_INCLUDE_PATH X11/extensions/Xvlib.h               ${X11_INC_SEARCH_PATH})


  FIND_LIBRARY(X11_X11_LIB X11               ${X11_LIB_SEARCH_PATH})

  # Find additional X libraries. Keep list sorted by library name.
  FIND_LIBRARY(X11_ICE_LIB ICE               ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_SM_LIB SM                 ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xau_LIB Xau               ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xcomposite_LIB Xcomposite ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xcursor_LIB Xcursor       ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xdamage_LIB Xdamage       ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xdmcp_LIB Xdmcp           ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xext_LIB Xext             ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xfixes_LIB Xfixes         ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xft_LIB Xft               ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xinerama_LIB Xinerama     ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xinput_LIB Xi             ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xpm_LIB Xpm               ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xrandr_LIB Xrandr         ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xrender_LIB Xrender       ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xscreensaver_LIB Xss      ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xt_LIB Xt                 ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_XTest_LIB Xtst            ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xv_LIB Xv                 ${X11_LIB_SEARCH_PATH})
  FIND_LIBRARY(X11_Xxf86misc_LIB Xxf86misc   ${X11_LIB_SEARCH_PATH})

  SET(X11_LIBRARY_DIR "")
  IF(X11_X11_LIB)
    GET_FILENAME_COMPONENT(X11_LIBRARY_DIR ${X11_X11_LIB} PATH)
  ENDIF(X11_X11_LIB)

  SET(X11_INCLUDE_DIR) # start with empty list
  IF(X11_X11_INCLUDE_PATH)
    SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_X11_INCLUDE_PATH})
  ENDIF(X11_X11_INCLUDE_PATH)

  IF(X11_Xlib_INCLUDE_PATH)
    SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xlib_INCLUDE_PATH})
  ENDIF(X11_Xlib_INCLUDE_PATH)

  IF(X11_Xutil_INCLUDE_PATH)
    SET(X11_Xutil_FOUND TRUE)
    SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xutil_INCLUDE_PATH})
  ENDIF(X11_Xutil_INCLUDE_PATH)

  IF(X11_Xshape_INCLUDE_PATH)
    SET(X11_Xshape_FOUND TRUE)
    SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xshape_INCLUDE_PATH})
  ENDIF(X11_Xshape_INCLUDE_PATH)

  SET(X11_LIBRARIES) # start with empty list
  IF(X11_X11_LIB)
    SET(X11_LIBRARIES ${X11_LIBRARIES} ${X11_X11_LIB})
  ENDIF(X11_X11_LIB)

  IF(X11_Xext_LIB)
    SET(X11_Xext_FOUND TRUE)
    SET(X11_LIBRARIES ${X11_LIBRARIES} ${X11_Xext_LIB})
  ENDIF(X11_Xext_LIB)

  IF(X11_Xt_LIB AND X11_Xt_INCLUDE_PATH)
    SET(X11_Xt_FOUND TRUE)
  ENDIF(X11_Xt_LIB AND X11_Xt_INCLUDE_PATH)

  IF(X11_Xft_LIB AND X11_Xft_INCLUDE_PATH)
    SET(X11_Xft_FOUND TRUE)
    SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xft_INCLUDE_PATH})
  ENDIF(X11_Xft_LIB AND X11_Xft_INCLUDE_PATH)

  IF(X11_Xv_LIB AND X11_Xv_INCLUDE_PATH)
    SET(X11_Xv_FOUND TRUE)
    SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xv_INCLUDE_PATH})
  ENDIF(X11_Xv_LIB AND X11_Xv_INCLUDE_PATH)

  IF (X11_Xau_LIB AND X11_Xau_INCLUDE_PATH)
    SET(X11_Xau_FOUND TRUE)
  ENDIF (X11_Xau_LIB AND X11_Xau_INCLUDE_PATH)

  IF (X11_Xdmcp_INCLUDE_PATH AND X11_Xdmcp_LIB)
      SET(X11_Xdmcp_FOUND TRUE)
      SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xdmcp_INCLUDE_PATH})
  ENDIF (X11_Xdmcp_INCLUDE_PATH AND X11_Xdmcp_LIB)

  IF (X11_Xaccessrules_INCLUDE_PATH AND X11_Xaccessstr_INCLUDE_PATH)
      SET(X11_Xaccess_FOUND TRUE)
      SET(X11_Xaccess_INCLUDE_PATH ${X11_Xaccessstr_INCLUDE_PATH})
      SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xaccess_INCLUDE_PATH})
  ENDIF (X11_Xaccessrules_INCLUDE_PATH AND X11_Xaccessstr_INCLUDE_PATH)

  IF (X11_Xpm_INCLUDE_PATH AND X11_Xpm_LIB)
      SET(X11_Xpm_FOUND TRUE)
      SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xpm_INCLUDE_PATH})
  ENDIF (X11_Xpm_INCLUDE_PATH AND X11_Xpm_LIB)

  IF (X11_Xcomposite_INCLUDE_PATH AND X11_Xcomposite_LIB)
     SET(X11_Xcomposite_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xcomposite_INCLUDE_PATH})
  ENDIF (X11_Xcomposite_INCLUDE_PATH AND X11_Xcomposite_LIB)

  IF (X11_Xdamage_INCLUDE_PATH AND X11_Xdamage_LIB)
     SET(X11_Xdamage_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xdamage_INCLUDE_PATH})
  ENDIF (X11_Xdamage_INCLUDE_PATH AND X11_Xdamage_LIB)

  IF (X11_XShm_INCLUDE_PATH)
     SET(X11_XShm_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_XShm_INCLUDE_PATH})
  ENDIF (X11_XShm_INCLUDE_PATH)

  IF (X11_XTest_INCLUDE_PATH AND X11_XTest_LIB)
      SET(X11_XTest_FOUND TRUE)
      SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_XTest_INCLUDE_PATH})
  ENDIF (X11_XTest_INCLUDE_PATH AND X11_XTest_LIB)

  IF (X11_Xinerama_INCLUDE_PATH AND X11_Xinerama_LIB)
     SET(X11_Xinerama_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xinerama_INCLUDE_PATH})
  ENDIF (X11_Xinerama_INCLUDE_PATH  AND X11_Xinerama_LIB)

  IF (X11_Xfixes_INCLUDE_PATH AND X11_Xfixes_LIB)
     SET(X11_Xfixes_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xfixes_INCLUDE_PATH})
  ENDIF (X11_Xfixes_INCLUDE_PATH AND X11_Xfixes_LIB)

  IF (X11_Xrender_INCLUDE_PATH AND X11_Xrender_LIB)
     SET(X11_Xrender_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xrender_INCLUDE_PATH})
  ENDIF (X11_Xrender_INCLUDE_PATH AND X11_Xrender_LIB)

  IF (X11_Xrandr_INCLUDE_PATH AND X11_Xrandr_LIB)
     SET(X11_Xrandr_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
  ENDIF (X11_Xrandr_INCLUDE_PATH AND X11_Xrandr_LIB)

  IF (X11_xf86misc_INCLUDE_PATH AND X11_Xxf86misc_LIB)
     SET(X11_xf86misc_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_xf86misc_INCLUDE_PATH})
  ENDIF (X11_xf86misc_INCLUDE_PATH  AND X11_Xxf86misc_LIB)

  IF (X11_xf86vmode_INCLUDE_PATH)
     SET(X11_xf86vmode_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_xf86vmode_INCLUDE_PATH})
  ENDIF (X11_xf86vmode_INCLUDE_PATH)

  IF (X11_Xcursor_INCLUDE_PATH AND X11_Xcursor_LIB)
     SET(X11_Xcursor_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xcursor_INCLUDE_PATH})
  ENDIF (X11_Xcursor_INCLUDE_PATH AND X11_Xcursor_LIB)

  IF (X11_Xscreensaver_INCLUDE_PATH AND X11_Xscreensaver_LIB)
     SET(X11_Xscreensaver_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xscreensaver_INCLUDE_PATH})
  ENDIF (X11_Xscreensaver_INCLUDE_PATH AND X11_Xscreensaver_LIB)

  IF (X11_dpms_INCLUDE_PATH)
     SET(X11_dpms_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_dpms_INCLUDE_PATH})
  ENDIF (X11_dpms_INCLUDE_PATH)

  IF (X11_Xkb_INCLUDE_PATH AND X11_Xkblib_INCLUDE_PATH AND X11_Xlib_INCLUDE_PATH)
     SET(X11_Xkb_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xkb_INCLUDE_PATH} )
  ENDIF (X11_Xkb_INCLUDE_PATH AND X11_Xkblib_INCLUDE_PATH AND X11_Xlib_INCLUDE_PATH)

  IF (X11_Xinput_INCLUDE_PATH AND X11_Xinput_LIB)
     SET(X11_Xinput_FOUND TRUE)
     SET(X11_INCLUDE_DIR ${X11_INCLUDE_DIR} ${X11_Xinput_INCLUDE_PATH})
  ENDIF (X11_Xinput_INCLUDE_PATH AND X11_Xinput_LIB)

  IF(X11_ICE_LIB AND X11_ICE_INCLUDE_PATH)
     SET(X11_ICE_FOUND TRUE)
  ENDIF(X11_ICE_LIB AND X11_ICE_INCLUDE_PATH)

  # Deprecated variable for backwards compatibility with CMake 1.4
  IF (X11_X11_INCLUDE_PATH AND X11_LIBRARIES)
    SET(X11_FOUND 1)
  ENDIF (X11_X11_INCLUDE_PATH AND X11_LIBRARIES)

  IF(X11_FOUND)
    INCLUDE(CheckFunctionExists)
    INCLUDE(CheckLibraryExists)

    # Translated from an autoconf-generated configure script.
    # See libs.m4 in autoconf's m4 directory.
    IF($ENV{ISC} MATCHES "^yes$")
      SET(X11_X_EXTRA_LIBS -lnsl_s -linet)
    ELSE($ENV{ISC} MATCHES "^yes$")
      SET(X11_X_EXTRA_LIBS "")

      # See if XOpenDisplay in X11 works by itself.
      CHECK_LIBRARY_EXISTS("${X11_LIBRARIES}" "XOpenDisplay" "${X11_LIBRARY_DIR}" X11_LIB_X11_SOLO)
      IF(NOT X11_LIB_X11_SOLO)
        # Find library needed for dnet_ntoa.
        CHECK_LIBRARY_EXISTS("dnet" "dnet_ntoa" "" X11_LIB_DNET_HAS_DNET_NTOA) 
        IF (X11_LIB_DNET_HAS_DNET_NTOA)
          SET (X11_X_EXTRA_LIBS ${X11_X_EXTRA_LIBS} -ldnet)
        ELSE (X11_LIB_DNET_HAS_DNET_NTOA)
          CHECK_LIBRARY_EXISTS("dnet_stub" "dnet_ntoa" "" X11_LIB_DNET_STUB_HAS_DNET_NTOA) 
          IF (X11_LIB_DNET_STUB_HAS_DNET_NTOA)
            SET (X11_X_EXTRA_LIBS ${X11_X_EXTRA_LIBS} -ldnet_stub)
          ENDIF (X11_LIB_DNET_STUB_HAS_DNET_NTOA)
        ENDIF (X11_LIB_DNET_HAS_DNET_NTOA)
      ENDIF(NOT X11_LIB_X11_SOLO)

      # Find library needed for gethostbyname.
      CHECK_FUNCTION_EXISTS("gethostbyname" CMAKE_HAVE_GETHOSTBYNAME)
      IF(NOT CMAKE_HAVE_GETHOSTBYNAME)
        CHECK_LIBRARY_EXISTS("nsl" "gethostbyname" "" CMAKE_LIB_NSL_HAS_GETHOSTBYNAME) 
        IF (CMAKE_LIB_NSL_HAS_GETHOSTBYNAME)
          SET (X11_X_EXTRA_LIBS ${X11_X_EXTRA_LIBS} -lnsl)
        ELSE (CMAKE_LIB_NSL_HAS_GETHOSTBYNAME)
          CHECK_LIBRARY_EXISTS("bsd" "gethostbyname" "" CMAKE_LIB_BSD_HAS_GETHOSTBYNAME) 
          IF (CMAKE_LIB_BSD_HAS_GETHOSTBYNAME)
            SET (X11_X_EXTRA_LIBS ${X11_X_EXTRA_LIBS} -lbsd)
          ENDIF (CMAKE_LIB_BSD_HAS_GETHOSTBYNAME)
        ENDIF (CMAKE_LIB_NSL_HAS_GETHOSTBYNAME)
      ENDIF(NOT CMAKE_HAVE_GETHOSTBYNAME)

      # Find library needed for connect.
      CHECK_FUNCTION_EXISTS("connect" CMAKE_HAVE_CONNECT)
      IF(NOT CMAKE_HAVE_CONNECT)
        CHECK_LIBRARY_EXISTS("socket" "connect" "" CMAKE_LIB_SOCKET_HAS_CONNECT) 
        IF (CMAKE_LIB_SOCKET_HAS_CONNECT)
          SET (X11_X_EXTRA_LIBS -lsocket ${X11_X_EXTRA_LIBS})
        ENDIF (CMAKE_LIB_SOCKET_HAS_CONNECT)
      ENDIF(NOT CMAKE_HAVE_CONNECT)

      # Find library needed for remove.
      CHECK_FUNCTION_EXISTS("remove" CMAKE_HAVE_REMOVE)
      IF(NOT CMAKE_HAVE_REMOVE)
        CHECK_LIBRARY_EXISTS("posix" "remove" "" CMAKE_LIB_POSIX_HAS_REMOVE) 
        IF (CMAKE_LIB_POSIX_HAS_REMOVE)
          SET (X11_X_EXTRA_LIBS ${X11_X_EXTRA_LIBS} -lposix)
        ENDIF (CMAKE_LIB_POSIX_HAS_REMOVE)
      ENDIF(NOT CMAKE_HAVE_REMOVE)

      # Find library needed for shmat.
      CHECK_FUNCTION_EXISTS("shmat" CMAKE_HAVE_SHMAT)
      IF(NOT CMAKE_HAVE_SHMAT)
        CHECK_LIBRARY_EXISTS("ipc" "shmat" "" CMAKE_LIB_IPS_HAS_SHMAT) 
        IF (CMAKE_LIB_IPS_HAS_SHMAT)
          SET (X11_X_EXTRA_LIBS ${X11_X_EXTRA_LIBS} -lipc)
        ENDIF (CMAKE_LIB_IPS_HAS_SHMAT)
      ENDIF(NOT CMAKE_HAVE_SHMAT)
    ENDIF($ENV{ISC} MATCHES "^yes$")

    IF (X11_ICE_FOUND)
      CHECK_LIBRARY_EXISTS("ICE" "IceConnectionNumber" "${X11_LIBRARY_DIR}"
                            CMAKE_LIB_ICE_HAS_ICECONNECTIONNUMBER)
      IF(CMAKE_LIB_ICE_HAS_ICECONNECTIONNUMBER)
        SET (X11_X_PRE_LIBS ${X11_ICE_LIB})
        IF(X11_SM_LIB)
          SET (X11_X_PRE_LIBS ${X11_SM_LIB} ${X11_X_PRE_LIBS})
        ENDIF(X11_SM_LIB)
      ENDIF(CMAKE_LIB_ICE_HAS_ICECONNECTIONNUMBER)
    ENDIF (X11_ICE_FOUND)

    # Build the final list of libraries.
    SET(X11_LIBRARIES ${X11_X_PRE_LIBS} ${X11_LIBRARIES} ${X11_X_EXTRA_LIBS})

    INCLUDE(FindPackageMessage)
    FIND_PACKAGE_MESSAGE(X11 "Found X11: ${X11_X11_LIB}"
      "[${X11_X11_LIB}][${X11_INCLUDE_DIR}]")
  ELSE (X11_FOUND)
    IF (X11_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find X11")
    ENDIF (X11_FIND_REQUIRED)
  ENDIF (X11_FOUND)

  MARK_AS_ADVANCED(
    X11_X11_INCLUDE_PATH
    X11_X11_LIB
    X11_Xext_LIB
    X11_Xau_LIB
    X11_Xau_INCLUDE_PATH
    X11_Xlib_INCLUDE_PATH
    X11_Xutil_INCLUDE_PATH
    X11_Xcomposite_INCLUDE_PATH
    X11_Xcomposite_LIB
    X11_Xaccess_INCLUDE_PATH
    X11_Xfixes_LIB
    X11_Xfixes_INCLUDE_PATH
    X11_Xrandr_LIB
    X11_Xrandr_INCLUDE_PATH
    X11_Xdamage_LIB
    X11_Xdamage_INCLUDE_PATH
    X11_Xrender_LIB
    X11_Xrender_INCLUDE_PATH
    X11_Xxf86misc_LIB
    X11_xf86misc_INCLUDE_PATH
    X11_xf86vmode_INCLUDE_PATH
    X11_Xinerama_LIB
    X11_Xinerama_INCLUDE_PATH
    X11_XTest_LIB
    X11_XTest_INCLUDE_PATH
    X11_Xcursor_LIB
    X11_Xcursor_INCLUDE_PATH
    X11_dpms_INCLUDE_PATH
    X11_Xt_LIB
    X11_Xt_INCLUDE_PATH
    X11_Xdmcp_LIB
    X11_LIBRARIES
    X11_Xaccessrules_INCLUDE_PATH
    X11_Xaccessstr_INCLUDE_PATH
    X11_Xdmcp_INCLUDE_PATH
    X11_Xkb_INCLUDE_PATH
    X11_Xkblib_INCLUDE_PATH
    X11_Xscreensaver_INCLUDE_PATH
    X11_Xscreensaver_LIB
    X11_Xpm_INCLUDE_PATH
    X11_Xpm_LIB
    X11_Xinput_LIB
    X11_Xinput_INCLUDE_PATH
    X11_Xft_LIB
    X11_Xft_INCLUDE_PATH
    X11_Xshape_INCLUDE_PATH
    X11_Xv_LIB
    X11_Xv_INCLUDE_PATH
    X11_XShm_INCLUDE_PATH
    X11_ICE_LIB
    X11_ICE_INCLUDE_PATH
    X11_SM_LIB
  )
  SET(CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK_SAVE})
ENDIF (UNIX)

# X11_FIND_REQUIRED_<component> could be checked too
