# - FindGTK2.cmake
# This module can find the GTK2 widget libraries and several of its other
# optional components like gtkmm, glade, and glademm.
#
# NOTE: If you intend to use version checking, CMake 2.6.2 or later is
#       required.
#
# Specify one or more of the following components
# as you call this find module. See example below.
#
#   gtk
#   gtkmm
#   glade
#   glademm
#
# The following variables will be defined for your use
#
#   GTK2_FOUND - Were all of your specified components found?
#   GTK2_INCLUDE_DIRS - All include directories
#   GTK2_LIBRARIES - All libraries
#
#   GTK2_VERSION - The version of GTK2 found (x.y.z)
#   GTK2_MAJOR_VERSION - The major version of GTK2
#   GTK2_MINOR_VERSION - The minor version of GTK2
#   GTK2_PATCH_VERSION - The patch version of GTK2
#
# Optional variables you can define prior to calling this module:
#
#   GTK2_DEBUG - Enables verbose debugging of the module
#   GTK2_SKIP_MARK_AS_ADVANCED - Disable marking cache variables as advanced
#
#=================
# Example Usage:
#
#   Call find_package() once, here are some examples to pick from:
#
#   Require GTK 2.6 or later
#       find_package(GTK2 2.6 REQUIRED gtk)
#
#   Require GTK 2.10 or later and Glade
#       find_package(GTK2 2.10 REQUIRED gtk glade)
#
#   Search for GTK/GTKMM 2.8 or later
#       find_package(GTK2 2.8 COMPONENTS gtk gtkmm)
#
#   if(GTK2_FOUND)
#      include_directories(${GTK2_INCLUDE_DIRS})
#      add_executable(mygui mygui.cc)
#      target_link_libraries(mygui ${GTK2_LIBRARIES})
#   endif()
#

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2008-2009 Philip Lowman <philip@yhbt.com>
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

# Version 0.7 (3/22/09)
#   * Checked into CMake CVS
#   * Added versioning support
#   * Module now defaults to searching for GTK if COMPONENTS not specified.
#   * Added HKCU prior to HKLM registry key and GTKMM specific environment
#      variable as per mailing list discussion.
#   * Added lib64 to include search path and a few other search paths where GTK
#      may be installed on Unix systems.
#   * Switched to lowercase CMake commands
#   * Prefaced internal variables with _GTK2 to prevent collision
#   * Changed internal macros to functions
#   * Enhanced documentation
# Version 0.6 (1/8/08)
#   Added GTK2_SKIP_MARK_AS_ADVANCED option
# Version 0.5 (12/19/08)
#   Second release to cmake mailing list

#=============================================================
# _GTK2_GET_VERSION
# Internal function to parse the version number in gtkversion.h
#   _OUT_major = Major version number
#   _OUT_minor = Minor version number
#   _OUT_micro = Micro version number
#   _gtkversion_hdr = Header file to parse
#=============================================================
function(_GTK2_GET_VERSION _OUT_major _OUT_minor _OUT_micro _gtkversion_hdr)
    file(READ ${_gtkversion_hdr} _contents)
    if(_contents)
        string(REGEX REPLACE ".*#define GTK_MAJOR_VERSION[ \t]+\\(([0-9]+)\\).*" "\\1" ${_OUT_major} "${_contents}")
        string(REGEX REPLACE ".*#define GTK_MINOR_VERSION[ \t]+\\(([0-9]+)\\).*" "\\1" ${_OUT_minor} "${_contents}")
        string(REGEX REPLACE ".*#define GTK_MICRO_VERSION[ \t]+\\(([0-9]+)\\).*" "\\1" ${_OUT_micro} "${_contents}")
        
        if(NOT ${_OUT_major} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for GTK2_MAJOR_VERSION!")
        endif()
        if(NOT ${_OUT_minor} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for GTK2_MINOR_VERSION!")
        endif()
        if(NOT ${_OUT_micro} MATCHES "[0-9]+")
            message(FATAL_ERROR "Version parsing failed for GTK2_MICRO_VERSION!")
        endif()

        set(${_OUT_major} ${${_OUT_major}} PARENT_SCOPE)
        set(${_OUT_minor} ${${_OUT_minor}} PARENT_SCOPE)
        set(${_OUT_micro} ${${_OUT_micro}} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Include file ${_gtkversion_hdr} does not exist")
    endif()
endfunction()

#=============================================================
# _GTK2_FIND_INCLUDE_DIR
# Internal function to find the GTK include directories
#   _var = variable to set
#   _hdr = header file to look for
#=============================================================
function(_GTK2_FIND_INCLUDE_DIR _var _hdr)

    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK2_FIND_INCLUDE_DIR( ${_var} ${_hdr} )")
    endif()

    set(_relatives
        # FIXME
        glibmm-2.4
        glib-2.0
        atk-1.0
        atkmm-1.6
        cairo
        cairomm-1.0
        gdkmm-2.4
        giomm-2.4
        gtk-2.0
        gtkmm-2.4
        libglade-2.0
        libglademm-2.4
        pango-1.0
        pangomm-1.4
        sigc++-2.0
    )

    set(_suffixes)
    foreach(_d ${_relatives})
        list(APPEND _suffixes ${_d})
        list(APPEND _suffixes ${_d}/include) # for /usr/lib/gtk-2.0/include
    endforeach()

    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "include suffixes = ${_suffixes}")
    endif()

    find_path(${_var} ${_hdr}
        PATHS
            /usr/local/lib64
            /usr/local/lib
            /usr/lib64
            /usr/lib
            /opt/gnome/include
            /opt/gnome/lib
            /opt/openwin/include
            /usr/openwin/lib
            $ENV{GTKMM_BASEPATH}/include
            $ENV{GTKMM_BASEPATH}/lib
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\2.4;Path]/include
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\2.4;Path]/lib
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\2.4;Path]/include
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\2.4;Path]/lib
        PATH_SUFFIXES
            ${_suffixes}
    )

    if(${_var})
        set(GTK2_INCLUDE_DIRS ${GTK2_INCLUDE_DIRS} ${${_var}} PARENT_SCOPE)
        if(NOT GTK2_SKIP_MARK_AS_ADVANCED)
            mark_as_advanced(${_var})
        endif()
    endif()

endfunction(_GTK2_FIND_INCLUDE_DIR)

#=============================================================
# _GTK2_FIND_LIBRARY
# Internal function to find libraries packaged with GTK2
#   _var = library variable to create
#=============================================================
function(_GTK2_FIND_LIBRARY _var _lib _expand_vc _append_version)

    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "_GTK2_FIND_LIBRARY( ${_var} ${_lib} ${_expand_vc} ${_append_version} )")
    endif()

    # Not GTK versions per se but the versions encoded into Windows
    # import libraries (GtkMM 2.14.1 has a gtkmm-vc80-2_4.lib for example)
    # Also the MSVC libraries use _ for . (this is handled below)
    set(_versions 2.20 2.18 2.16 2.14 2.12
                  2.10  2.8  2.6  2.4  2.2 2.0
                  1.20 1.18 1.16 1.14 1.12
                  1.10  1.8  1.6  1.4  1.2 1.0)

    set(_library)
    set(_library_d)

    set(_library ${_lib})

    if(_expand_vc)
        # Add vc80/vc90 midfixes
        if(MSVC80)
            set(_library   ${_library}-vc80)
            set(_library_d ${_library}-d)
        elseif(MSVC90)
            set(_library   ${_library}-vc90)
            set(_library_d ${_library}-d)
        endif()
    endif()

    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "After midfix addition = ${_library} and ${_library_d}")
    endif()

    set(_lib_list)
    set(_libd_list)
    if(_append_version)
        foreach(_ver ${_versions})
            list(APPEND _lib_list  "${_library}-${_ver}")
            list(APPEND _libd_list "${_library_d}-${_ver}")
        endforeach()
    else()
        set(_lib_list ${_library})
        set(_libd_list ${_library_d})
    endif()
    
    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "library list = ${_lib_list} and library debug list = ${_libd_list}")
    endif()

    # For some silly reason the MSVC libraries use _ instead of .
    # in the version fields
    if(_expand_vc AND MSVC)
        set(_no_dots_lib_list)
        set(_no_dots_libd_list)
        foreach(_l ${_lib_list})
            string(REPLACE "." "_" _no_dots_library ${_l})
            list(APPEND _no_dots_lib_list ${_no_dots_library})
        endforeach()
        # And for debug
        set(_no_dots_libsd_list)
        foreach(_l ${_libd_list})
            string(REPLACE "." "_" _no_dots_libraryd ${_l})
            list(APPEND _no_dots_libd_list ${_no_dots_libraryd})
        endforeach()

        # Copy list back to original names
        set(_lib_list ${_no_dots_lib_list})
        set(_libd_list ${_no_dots_libd_list})
    endif()

    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                       "While searching for ${_var}, our proposed library list is ${_lib_list}")
    endif()

    find_library(${_var} 
        NAMES ${_lib_list}
        PATHS
            /opt/gnome/lib
            /opt/gnome/lib64
            /usr/openwin/lib
            /usr/openwin/lib64
            $ENV{GTKMM_BASEPATH}/lib
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\2.4;Path]/lib
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\2.4;Path]/lib
        )

    if(_expand_vc AND MSVC)
        if(GTK2_DEBUG)
            message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}]     "
                           "While searching for ${_var}_DEBUG our proposed library list is ${_libd_list}")
        endif()

        find_library(${_var}_DEBUG
            NAMES ${_libd_list}
            PATHS
            $ENV{GTKMM_BASEPATH}/lib
            [HKEY_CURRENT_USER\\SOFTWARE\\gtkmm\\2.4;Path]/lib
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\gtkmm\\2.4;Path]/lib
        )

        if(${_var} AND ${_var}_DEBUG)
            if(NOT GTK2_SKIP_MARK_AS_ADVANCED)
                mark_as_advanced(${_var}_DEBUG)
            endif()
            set(GTK2_LIBRARIES ${GTK2_LIBRARIES} optimized ${${_var}} debug ${${_var}_DEBUG})
            set(GTK2_LIBRARIES ${GTK2_LIBRARIES} PARENT_SCOPE)
        endif()
    else()
        if(NOT GTK2_SKIP_MARK_AS_ADVANCED)
            mark_as_advanced(${_var})
        endif()
        set(GTK2_LIBRARIES ${GTK2_LIBRARIES} ${${_var}})
        set(GTK2_LIBRARIES ${GTK2_LIBRARIES} PARENT_SCOPE)
        # Set debug to release
        set(${_var}_DEBUG ${${_var}})
        set(${_var}_DEBUG ${${_var}} PARENT_SCOPE)
    endif()
endfunction(_GTK2_FIND_LIBRARY)

#=============================================================

#
# main()
#

set(GTK2_FOUND)
set(GTK2_INCLUDE_DIRS)
set(GTK2_LIBRARIES)

if(NOT GTK2_FIND_COMPONENTS)
    # Assume they only want GTK
    set(GTK2_FIND_COMPONENTS gtk)
endif()

#
# If specified, enforce version number
#
if(GTK2_FIND_VERSION)
    cmake_minimum_required(VERSION 2.6.2)
    set(GTK2_FAILED_VERSION_CHECK true)
    if(GTK2_DEBUG)
        message(STATUS "[FindGTK2.cmake:${CMAKE_CURRENT_LIST_LINE}] "
                       "Searching for version ${GTK2_FIND_VERSION}")
    endif()
    _GTK2_FIND_INCLUDE_DIR(GTK2_GTK_INCLUDE_DIR gtk/gtk.h)
    if(GTK2_GTK_INCLUDE_DIR)
        _GTK2_GET_VERSION(GTK2_MAJOR_VERSION
                          GTK2_MINOR_VERSION
                          GTK2_PATCH_VERSION
                          ${GTK2_GTK_INCLUDE_DIR}/gtk/gtkversion.h)
        set(GTK2_VERSION
            ${GTK2_MAJOR_VERSION}.${GTK2_MINOR_VERSION}.${GTK2_PATCH_VERSION})
        if(GTK2_FIND_VERSION_EXACT)
            if(GTK2_VERSION VERSION_EQUAL GTK2_FIND_VERSION)
                set(GTK2_FAILED_VERSION_CHECK false)
            endif()
        else()
            if(GTK2_VERSION VERSION_EQUAL   GTK2_FIND_VERSION OR
               GTK2_VERSION VERSION_GREATER GTK2_FIND_VERSION)
                set(GTK2_FAILED_VERSION_CHECK false)
            endif()
        endif()
    else()
        # If we can't find the GTK include dir, we can't do version checking
        if(GTK2_FIND_REQUIRED AND NOT GTK2_FIND_QUIETLY)
            message(FATAL_ERROR "Could not find GTK2 include directory")
        endif()
        return()
    endif()

    if(GTK2_FAILED_VERSION_CHECK)
        if(GTK2_FIND_REQUIRED AND NOT GTK2_FIND_QUIETLY)
            if(GTK2_FIND_VERSION_EXACT)
                message(FATAL_ERROR "GTK2 version check failed.  Version ${GTK2_VERSION} was found, version ${GTK2_FIND_VERSION} is needed exactly.")
            else()
                message(FATAL_ERROR "GTK2 version check failed.  Version ${GTK2_VERSION} was found, at least version ${GTK2_FIND_VERSION} is required")
            endif()
        endif()    
        
        # If the version check fails, exit out of the module here
        return()
    endif()
endif()

#
# Find all components
#

foreach(_GTK2_component ${GTK2_FIND_COMPONENTS})
    if(_GTK2_component STREQUAL "gtk")
        
        _GTK2_FIND_INCLUDE_DIR(GTK2_GLIB_INCLUDE_DIR glib.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GLIBCONFIG_INCLUDE_DIR glibconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GLIB_LIBRARY glib false true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_GDK_INCLUDE_DIR gdk/gdk.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GDKCONFIG_INCLUDE_DIR gdkconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GDK_LIBRARY gdk-x11 false true)
        _GTK2_FIND_LIBRARY    (GTK2_GDK_LIBRARY gdk-win32 false true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_GTK_INCLUDE_DIR gtk/gtk.h)
        _GTK2_FIND_LIBRARY    (GTK2_GTK_LIBRARY gtk-x11 false true)
        _GTK2_FIND_LIBRARY    (GTK2_GTK_LIBRARY gtk-win32 false true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_CAIRO_INCLUDE_DIR cairo.h)
        _GTK2_FIND_LIBRARY    (GTK2_CAIRO_LIBRARY cairo false false)

        _GTK2_FIND_INCLUDE_DIR(GTK2_PANGO_INCLUDE_DIR pango/pango.h)
        _GTK2_FIND_LIBRARY    (GTK2_PANGO_LIBRARY pango false true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_ATK_INCLUDE_DIR atk/atk.h)
        _GTK2_FIND_LIBRARY    (GTK2_ATK_LIBRARY atk false true)

        #elseif(_GTK2_component STREQUAL "gdk_pixbuf")
        #_GTK2_FIND_INCLUDE_DIR(GTK2_GDKPIXBUF_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h)
        #_GTK2_FIND_LIBRARY    (GTK2_GDKPIXBUF_LIBRARY gdk_pixbuf false true)

    elseif(_GTK2_component STREQUAL "gtkmm")

        _GTK2_FIND_INCLUDE_DIR(GTK2_GLIBMM_INCLUDE_DIR glibmm.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GLIBMMCONFIG_INCLUDE_DIR glibmmconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GLIBMM_LIBRARY glibmm true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_GDKMM_INCLUDE_DIR gdkmm.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GDKMMCONFIG_INCLUDE_DIR gdkmmconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GDKMM_LIBRARY gdkmm true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_GTKMM_INCLUDE_DIR gtkmm.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GTKMMCONFIG_INCLUDE_DIR gtkmmconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GTKMM_LIBRARY gtkmm true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_CAIROMM_INCLUDE_DIR cairomm/cairomm.h)
        _GTK2_FIND_LIBRARY    (GTK2_CAIROMM_LIBRARY cairomm true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_PANGOMM_INCLUDE_DIR pangomm.h)
        _GTK2_FIND_LIBRARY    (GTK2_PANGOMM_LIBRARY pangomm true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_SIGC++_INCLUDE_DIR sigc++/sigc++.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_SIGC++CONFIG_INCLUDE_DIR sigc++config.h)
        _GTK2_FIND_LIBRARY    (GTK2_SIGC++_LIBRARY sigc true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_GIOMM_INCLUDE_DIR giomm.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GIOMMCONFIG_INCLUDE_DIR giommconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GIOMM_LIBRARY giomm true true)

        _GTK2_FIND_INCLUDE_DIR(GTK2_ATKMM_INCLUDE_DIR atkmm.h)
        _GTK2_FIND_LIBRARY    (GTK2_ATKMM_LIBRARY atkmm true true)

    elseif(_GTK2_component STREQUAL "glade")

        _GTK2_FIND_INCLUDE_DIR(GTK2_GLADE_INCLUDE_DIR glade/glade.h)
        _GTK2_FIND_LIBRARY    (GTK2_GLADE_LIBRARY glade false true)
    
    elseif(_GTK2_component STREQUAL "glademm")

        _GTK2_FIND_INCLUDE_DIR(GTK2_GLADEMM_INCLUDE_DIR libglademm.h)
        _GTK2_FIND_INCLUDE_DIR(GTK2_GLADEMMCONFIG_INCLUDE_DIR libglademmconfig.h)
        _GTK2_FIND_LIBRARY    (GTK2_GLADEMM_LIBRARY glademm true true)

    else()
        message(FATAL_ERROR "Unknown GTK2 component ${_component}")
    endif()
endforeach()

#
# Solve for the GTK2 version if we haven't already
#
if(NOT GTK2_FIND_VERSION AND GTK2_GTK_INCLUDE_DIR)
    _GTK2_GET_VERSION(GTK2_MAJOR_VERSION
                      GTK2_MINOR_VERSION
                      GTK2_PATCH_VERSION
                      ${GTK2_GTK_INCLUDE_DIR}/gtk/gtkversion.h)
    set(GTK2_VERSION ${GTK2_MAJOR_VERSION}.${GTK2_MINOR_VERSION}.${GTK2_PATCH_VERSION})
endif()

#
# Try to enforce components
#

set(_GTK2_did_we_find_everything true)  # This gets set to GTK2_FOUND

include(FindPackageHandleStandardArgs)

foreach(_GTK2_component ${GTK2_FIND_COMPONENTS})
    string(TOUPPER ${_GTK2_component} _COMPONENT_UPPER)

    if(_GTK2_component STREQUAL "gtk")
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK2_${_COMPONENT_UPPER} "Some or all of the gtk libraries were not found."
            GTK2_GTK_LIBRARY
            GTK2_GTK_INCLUDE_DIR

            GTK2_GLIB_INCLUDE_DIR
            GTK2_GLIBCONFIG_INCLUDE_DIR
            GTK2_GLIB_LIBRARY

            GTK2_GDK_INCLUDE_DIR
            GTK2_GDKCONFIG_INCLUDE_DIR
            GTK2_GDK_LIBRARY
        )
    elseif(_GTK2_component STREQUAL "gtkmm")
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK2_${_COMPONENT_UPPER} "Some or all of the gtkmm libraries were not found."
            GTK2_GTKMM_LIBRARY
            GTK2_GTKMM_INCLUDE_DIR
            GTK2_GTKMMCONFIG_INCLUDE_DIR

            GTK2_GLIBMM_INCLUDE_DIR
            GTK2_GLIBMMCONFIG_INCLUDE_DIR
            GTK2_GLIBMM_LIBRARY

            GTK2_GDKMM_INCLUDE_DIR
            GTK2_GDKMMCONFIG_INCLUDE_DIR
            GTK2_GDKMM_LIBRARY
        )
    elseif(_GTK2_component STREQUAL "glade")
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK2_${_COMPONENT_UPPER} "The glade library was not found."
            GTK2_GLADE_LIBRARY
            GTK2_GLADE_INCLUDE_DIR
        )
    elseif(_GTK2_component STREQUAL "glademm")
        FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTK2_${_COMPONENT_UPPER} "The glademm library was not found."
            GTK2_GLADEMM_LIBRARY
            GTK2_GLADEMM_INCLUDE_DIR
            GTK2_GLADEMMCONFIG_INCLUDE_DIR
        )
    endif()

    if(NOT GTK2_${_COMPONENT_UPPER}_FOUND)
        set(_GTK2_did_we_find_everything false)
    endif()
endforeach()

if(_GTK2_did_we_find_everything AND NOT GTK2_VERSION_CHECK_FAILED)
    set(GTK2_FOUND true)
else()
    # Unset our variables.
    set(GTK2_FOUND false)
    set(GTK2_VERSION)
    set(GTK2_VERSION_MAJOR)
    set(GTK2_VERSION_MINOR)
    set(GTK2_VERSION_PATCH)
    set(GTK2_INCLUDE_DIRS)
    set(GTK2_LIBRARIES)
endif()

if(GTK2_INCLUDE_DIRS)
   list(REMOVE_DUPLICATES GTK2_INCLUDE_DIRS)
endif()

