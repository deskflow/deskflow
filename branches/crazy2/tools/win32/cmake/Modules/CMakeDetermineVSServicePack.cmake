# - Includes a public function for assisting users in trying to determine the
# Visual Studio service pack in use.
#
# Sets the passed in variable to one of the following values or an empty
# string if unknown.
#    vc80
#    vc80sp1
#    vc90
#    vc90sp1
#
# Usage:
# ===========================
#
#    if(MSVC)
#       include(CMakeDetermineVSServicePack)
#       DetermineVSServicePack( my_service_pack )
#
#       if( my_service_pack )
#           message(STATUS "Detected: ${my_service_pack}")
#       endif()
#    endif()
#
# ===========================

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Philip Lowman <philip@yhbt.com>
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

# [INTERNAL]
# Please do not call this function directly
function(_DetermineVSServicePackFromCompiler _OUT_VAR _cl_version)
   if    (${_cl_version} VERSION_EQUAL "14.00.50727.42")
       set(_version "vc80")
   elseif(${_cl_version} VERSION_EQUAL "14.00.50727.762")
       set(_version "vc80sp1")
   elseif(${_cl_version} VERSION_EQUAL "15.00.21022.08")
       set(_version "vc90")
   elseif(${_cl_version} VERSION_EQUAL "15.00.30729.01")
       set(_version "vc90sp1")
   else()
       set(_version "")
   endif()
   set(${_OUT_VAR} ${_version} PARENT_SCOPE)
endfunction()

#
# A function to call to determine the Visual Studio service pack
# in use.  See documentation above.
function(DetermineVSServicePack _pack)
    if(NOT DETERMINED_VS_SERVICE_PACK OR NOT ${_pack})
        file(WRITE "${CMAKE_BINARY_DIR}/return0.cc"
            "int main() { return 0; }\n")

        try_compile(DETERMINED_VS_SERVICE_PACK
            "${CMAKE_BINARY_DIR}"
            "${CMAKE_BINARY_DIR}/return0.cc"
            OUTPUT_VARIABLE _output
            COPY_FILE "${CMAKE_BINARY_DIR}/return0.cc")
        
        file(REMOVE "${CMAKE_BINARY_DIR}/return0.cc")
                
        if(DETERMINED_VS_SERVICE_PACK AND _output)
            string(REGEX MATCH "Compiler Version [0-9]+.[0-9]+.[0-9]+.[0-9]+"
                _cl_version "${_output}")
            if(_cl_version)
                string(REGEX MATCHALL "[0-9]+"
                    _cl_version_list "${_cl_version}")
                list(GET _cl_version_list 0 _major)
                list(GET _cl_version_list 1 _minor)
                list(GET _cl_version_list 2 _patch)
                list(GET _cl_version_list 3 _tweak)

                set(_cl_version_string ${_major}.${_minor}.${_patch}.${_tweak})
                
                # Call helper function to determine VS version
                _DetermineVSServicePackFromCompiler(_sp "${_cl_version_string}")
                if(_sp)
                    set(${_pack} ${_sp} CACHE INTERNAL
                        "The Visual Studio Release with Service Pack")
                endif()
            endif()
        endif()
    endif()
endfunction()

