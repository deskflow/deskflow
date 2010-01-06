#=============================================================================
# Copyright 2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

cmake_minimum_required(VERSION 2.7)
project(VerifyFortranC C Fortran)

option(VERIFY_CXX "Whether to verify C++ and Fortran" OFF)
if(VERIFY_CXX)
  enable_language(CXX)
  set(VerifyCXX VerifyCXX.cxx)
  add_definitions(-DVERIFY_CXX)
endif()

include(FortranCInterface)

FortranCInterface_HEADER(VerifyFortran.h SYMBOLS VerifyFortran)
include_directories(${VerifyFortranC_BINARY_DIR})

add_executable(VerifyFortranC main.c VerifyC.c VerifyFortran.f ${VerifyCXX})

if(NOT VERIFY_CXX)
  # The entry point (main) is defined in C; link with the C compiler.
  set_property(TARGET VerifyFortranC PROPERTY LINKER_LANGUAGE C)
endif()
