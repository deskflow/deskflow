# - This module defines variables and macros required to build eCos application.
# This file contains the following macros:
# ECOS_ADD_INCLUDE_DIRECTORIES() - add the eCos include dirs
# ECOS_ADD_EXECUTABLE(name source1 ... sourceN ) - create an eCos executable
# ECOS_ADJUST_DIRECTORY(VAR source1 ... sourceN ) - adjusts the path of the source files and puts the result into VAR
#
# Macros for selecting the toolchain:
# ECOS_USE_ARM_ELF_TOOLS()       - enable the ARM ELF toolchain for the directory where it is called
# ECOS_USE_I386_ELF_TOOLS()      - enable the i386 ELF toolchain for the directory where it is called
# ECOS_USE_PPC_EABI_TOOLS()      - enable the PowerPC toolchain for the directory where it is called
#
# It contains the following variables:
# ECOS_DEFINITIONS
# ECOSCONFIG_EXECUTABLE
# ECOS_CONFIG_FILE               - defaults to ecos.ecc, if your eCos configuration file has a different name, adjust this variable
# for internal use only:
#  ECOS_ADD_TARGET_LIB

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

# first check that ecosconfig is available
FIND_PROGRAM(ECOSCONFIG_EXECUTABLE NAMES ecosconfig)
IF(NOT ECOSCONFIG_EXECUTABLE)
   MESSAGE(SEND_ERROR "ecosconfig was not found. Either include it in the system path or set it manually using ccmake.")
ELSE(NOT ECOSCONFIG_EXECUTABLE)
   MESSAGE(STATUS "Found ecosconfig: ${ECOSCONFIG_EXECUTABLE}")
ENDIF(NOT ECOSCONFIG_EXECUTABLE)

# check that ECOS_REPOSITORY is set correctly
IF (NOT EXISTS $ENV{ECOS_REPOSITORY}/ecos.db)
   MESSAGE(SEND_ERROR "The environment variable ECOS_REPOSITORY is not set correctly. Set it to the directory which contains the file ecos.db")
ELSE (NOT EXISTS $ENV{ECOS_REPOSITORY}/ecos.db)
   MESSAGE(STATUS "ECOS_REPOSITORY is set to $ENV{ECOS_REPOSITORY}")
ENDIF (NOT EXISTS $ENV{ECOS_REPOSITORY}/ecos.db)

# check that tclsh (coming with TCL) is available, otherwise ecosconfig doesn't work
FIND_PACKAGE(Tclsh)
IF (NOT TCL_TCLSH)
   MESSAGE(SEND_ERROR "The TCL tclsh was not found. Please install TCL, it is required for building eCos applications.")
ELSE (NOT TCL_TCLSH)
   MESSAGE(STATUS "tlcsh found: ${TCL_TCLSH}")
ENDIF (NOT TCL_TCLSH)

#add the globale include-diretories
#usage: ECOS_ADD_INCLUDE_DIRECTORIES()
MACRO(ECOS_ADD_INCLUDE_DIRECTORIES)
#check for ProjectSources.txt one level higher
   IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../ProjectSources.txt)
      INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../)
   ELSE (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../ProjectSources.txt)
      INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)
   ENDIF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../ProjectSources.txt)

#the ecos include directory
   INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/ecos/install/include/)

ENDMACRO(ECOS_ADD_INCLUDE_DIRECTORIES)


#we want to compile for the xscale processor, in this case the following macro has to be called
#usage: ECOS_USE_ARM_ELF_TOOLS()
MACRO (ECOS_USE_ARM_ELF_TOOLS)
   SET(CMAKE_CXX_COMPILER "arm-elf-c++")
   SET(CMAKE_COMPILER_IS_GNUCXX 1)
   SET(CMAKE_C_COMPILER "arm-elf-gcc")
   SET(CMAKE_AR "arm-elf-ar")
   SET(CMAKE_RANLIB "arm-elf-ranlib")
#for linking
   SET(ECOS_LD_MCPU "-mcpu=xscale")
#for compiling
   ADD_DEFINITIONS(-mcpu=xscale -mapcs-frame)
#for the obj-tools
   SET(ECOS_ARCH_PREFIX "arm-elf-")
ENDMACRO (ECOS_USE_ARM_ELF_TOOLS)

#usage: ECOS_USE_PPC_EABI_TOOLS()
MACRO (ECOS_USE_PPC_EABI_TOOLS)
   SET(CMAKE_CXX_COMPILER "powerpc-eabi-c++")
   SET(CMAKE_COMPILER_IS_GNUCXX 1)
   SET(CMAKE_C_COMPILER "powerpc-eabi-gcc")
   SET(CMAKE_AR "powerpc-eabi-ar")
   SET(CMAKE_RANLIB "powerpc-eabi-ranlib")
#for linking
   SET(ECOS_LD_MCPU "")
#for compiling
   ADD_DEFINITIONS()
#for the obj-tools
   SET(ECOS_ARCH_PREFIX "powerpc-eabi-")
ENDMACRO (ECOS_USE_PPC_EABI_TOOLS)

#usage: ECOS_USE_I386_ELF_TOOLS()
MACRO (ECOS_USE_I386_ELF_TOOLS)
   SET(CMAKE_CXX_COMPILER "i386-elf-c++")
   SET(CMAKE_COMPILER_IS_GNUCXX 1)
   SET(CMAKE_C_COMPILER "i386-elf-gcc")
   SET(CMAKE_AR "i386-elf-ar")
   SET(CMAKE_RANLIB "i386-elf-ranlib")
#for linking
   SET(ECOS_LD_MCPU "")
#for compiling
   ADD_DEFINITIONS()
#for the obj-tools
   SET(ECOS_ARCH_PREFIX "i386-elf-")
ENDMACRO (ECOS_USE_I386_ELF_TOOLS)


#since the actual sources are located one level upwards
#a "../" has to be prepended in front of every source file
#call the following macro to achieve this, the first parameter
#is the name of the new list of source files with adjusted paths,
#followed by all source files
#usage: ECOS_ADJUST_DIRECTORY(adjusted_SRCS ${my_srcs})
MACRO(ECOS_ADJUST_DIRECTORY _target_FILES )
   FOREACH (_current_FILE ${ARGN})
      GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
      IF (NOT ${_abs_FILE} STREQUAL ${_current_FILE})
         GET_FILENAME_COMPONENT(_abs_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../${_current_FILE} ABSOLUTE)
      ENDIF (NOT ${_abs_FILE} STREQUAL ${_current_FILE})
      LIST(APPEND ${_target_FILES} ${_abs_FILE})
   ENDFOREACH (_current_FILE)
ENDMACRO(ECOS_ADJUST_DIRECTORY)

# the default ecos config file name
# maybe in the future also out-of-source builds may be possible
SET(ECOS_CONFIG_FILE ecos.ecc)

#creates the dependancy from all source files on the ecos target.ld,
#adds the command for compiling ecos
MACRO(ECOS_ADD_TARGET_LIB)
# when building out-of-source, create the ecos/ subdir
    IF(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ecos)
        FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ecos)
    ENDIF(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ecos)

#sources depend on target.ld
   SET_SOURCE_FILES_PROPERTIES(
      ${ARGN}
      PROPERTIES
      OBJECT_DEPENDS
      ${CMAKE_CURRENT_BINARY_DIR}/ecos/install/lib/target.ld
   )

   ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ecos/install/lib/target.ld
      COMMAND sh -c \"make -C ${CMAKE_CURRENT_BINARY_DIR}/ecos || exit -1\; if [ -e ${CMAKE_CURRENT_BINARY_DIR}/ecos/install/lib/target.ld ] \; then touch ${CMAKE_CURRENT_BINARY_DIR}/ecos/install/lib/target.ld\; fi\"
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ecos/makefile
   )

   ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ecos/makefile
      COMMAND sh -c \" cd ${CMAKE_CURRENT_BINARY_DIR}/ecos\; ${ECOSCONFIG_EXECUTABLE} --config=${CMAKE_CURRENT_SOURCE_DIR}/ecos/${ECOS_CONFIG_FILE} tree || exit -1\;\"
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ecos/${ECOS_CONFIG_FILE}
   )

   ADD_CUSTOM_TARGET( ecos make -C ${CMAKE_CURRENT_BINARY_DIR}/ecos/ DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/ecos/makefile )
ENDMACRO(ECOS_ADD_TARGET_LIB)

# get the directory of the current file, used later on in the file
GET_FILENAME_COMPONENT( ECOS_CMAKE_MODULE_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

#macro for creating an executable ecos application
#the first parameter is the name of the executable,
#the second is the list of all source files (where the path
#has been adjusted beforehand by calling ECOS_ADJUST_DIRECTORY()
#usage: ECOS_ADD_EXECUTABLE(my_app ${adjusted_SRCS})
MACRO(ECOS_ADD_EXECUTABLE _exe_NAME )
   #definitions, valid for all ecos projects
   #the optimization and "-g" for debugging has to be enabled
   #in the project-specific CMakeLists.txt
   ADD_DEFINITIONS(-D__ECOS__=1 -D__ECOS=1)
   SET(ECOS_DEFINITIONS -Wall -Wno-long-long -pipe -fno-builtin)

#the executable depends on ecos target.ld
   ECOS_ADD_TARGET_LIB(${ARGN})

# when using nmake makefiles, the custom buildtype supresses the default cl.exe flags
# and the rules for creating objects are adjusted for gcc
   SET(CMAKE_BUILD_TYPE CUSTOM_ECOS_BUILD)
   SET(CMAKE_C_COMPILE_OBJECT     "<CMAKE_C_COMPILER>   <FLAGS> -o <OBJECT> -c <SOURCE>")
   SET(CMAKE_CXX_COMPILE_OBJECT   "<CMAKE_CXX_COMPILER> <FLAGS> -o <OBJECT> -c <SOURCE>")
# special link commands for ecos-executables
   SET(CMAKE_CXX_LINK_EXECUTABLE  "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <OBJECTS>  -o <TARGET> ${_ecos_EXTRA_LIBS} -nostdlib  -nostartfiles -L${CMAKE_CURRENT_BINARY_DIR}/ecos/install/lib -Ttarget.ld ${ECOS_LD_MCPU}")
   SET(CMAKE_C_LINK_EXECUTABLE    "<CMAKE_C_COMPILER>   <CMAKE_C_LINK_FLAGS>   <OBJECTS>  -o <TARGET> ${_ecos_EXTRA_LIBS} -nostdlib  -nostartfiles -L${CMAKE_CURRENT_BINARY_DIR}/ecos/install/lib -Ttarget.ld ${ECOS_LD_MCPU}")
# some strict compiler flags
   SET (CMAKE_C_FLAGS "-Wstrict-prototypes")
   SET (CMAKE_CXX_FLAGS "-Woverloaded-virtual -fno-rtti -Wctor-dtor-privacy -fno-strict-aliasing -fno-exceptions")

   ADD_EXECUTABLE(${_exe_NAME} ${ARGN})
   SET_TARGET_PROPERTIES(${_exe_NAME} PROPERTIES SUFFIX ".elf")

#create a binary file
   ADD_CUSTOM_COMMAND(
      TARGET ${_exe_NAME}
      POST_BUILD
      COMMAND ${ECOS_ARCH_PREFIX}objcopy
      ARGS -O binary ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.bin
   )

#and an srec file
   ADD_CUSTOM_COMMAND(
      TARGET ${_exe_NAME}
      POST_BUILD
      COMMAND ${ECOS_ARCH_PREFIX}objcopy
      ARGS -O srec ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.srec
   )

#add the created files to the clean-files
   SET_DIRECTORY_PROPERTIES(
      PROPERTIES
       ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.bin;${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.srec;${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst;"
   )

   ADD_CUSTOM_TARGET(ecosclean ${CMAKE_COMMAND} -DECOS_DIR=${CMAKE_CURRENT_BINARY_DIR}/ecos/ -P ${ECOS_CMAKE_MODULE_DIR}/ecos_clean.cmake  )
   ADD_CUSTOM_TARGET(normalclean ${CMAKE_MAKE_PROGRAM} clean WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
   ADD_DEPENDENCIES (ecosclean normalclean)


   ADD_CUSTOM_TARGET( listing
      COMMAND echo -e   \"\\n--- Symbols sorted by address ---\\n\" > ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst
      COMMAND ${ECOS_ARCH_PREFIX}nm -S -C -n ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.elf >> ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst
      COMMAND echo -e \"\\n--- Symbols sorted by size ---\\n\" >> ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst
      COMMAND ${ECOS_ARCH_PREFIX}nm -S -C -r --size-sort ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.elf >> ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst
      COMMAND echo -e \"\\n--- Full assembly listing ---\\n\" >> ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst
      COMMAND ${ECOS_ARCH_PREFIX}objdump -S -x -d -C ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.elf >> ${CMAKE_CURRENT_BINARY_DIR}/${_exe_NAME}.lst )

ENDMACRO(ECOS_ADD_EXECUTABLE)

