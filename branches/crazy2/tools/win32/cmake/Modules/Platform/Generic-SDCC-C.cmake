
# This file implements basic support for sdcc (http://sdcc.sourceforge.net/)
# a free C compiler for 8 and 16 bit microcontrollers.
# To use it either a toolchain file is required or cmake has to be run like this:
# cmake -DCMAKE_C_COMPILER=sdcc -DCMAKE_SYSTEM_NAME=Generic <dir...>
# Since sdcc doesn't support C++, C++ support should be disabled in the
# CMakeLists.txt using the PROJECT() command:
# PROJECT(my_project C)

SET(CMAKE_STATIC_LIBRARY_PREFIX "")
SET(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
SET(CMAKE_SHARED_LIBRARY_PREFIX "")          # lib
SET(CMAKE_SHARED_LIBRARY_SUFFIX ".lib")          # .so
SET(CMAKE_IMPORT_LIBRARY_PREFIX )
SET(CMAKE_IMPORT_LIBRARY_SUFFIX )
SET(CMAKE_EXECUTABLE_SUFFIX ".ihx")          # intel hex file
SET(CMAKE_LINK_LIBRARY_SUFFIX ".lib")
SET(CMAKE_DL_LIBS "")

SET(CMAKE_C_OUTPUT_EXTENSION ".rel")

# find sdcclib as CMAKE_AR
# since cmake may already have searched for "ar", sdcclib has to 
# be searched with a different variable name (SDCCLIB_EXECUTABLE) 
# and must then be forced into the cache
GET_FILENAME_COMPONENT(SDCC_LOCATION "${CMAKE_C_COMPILER}" PATH)
FIND_PROGRAM(SDCCLIB_EXECUTABLE sdcclib PATHS "${SDCC_LOCATION}" NO_DEFAULT_PATH)
FIND_PROGRAM(SDCCLIB_EXECUTABLE sdcclib)
SET(CMAKE_AR "${SDCCLIB_EXECUTABLE}" CACHE FILEPATH "The sdcc librarian" FORCE)

# CMAKE_C_FLAGS_INIT and CMAKE_EXE_LINKER_FLAGS_INIT should be set in a CMAKE_SYSTEM_PROCESSOR file
IF(NOT DEFINED CMAKE_C_FLAGS_INIT)
  SET(CMAKE_C_FLAGS_INIT "-mmcs51 --model-small")
ENDIF(NOT DEFINED CMAKE_C_FLAGS_INIT)

IF(NOT DEFINED CMAKE_EXE_LINKER_FLAGS_INIT)
  SET (CMAKE_EXE_LINKER_FLAGS_INIT --model-small)
ENDIF(NOT DEFINED CMAKE_EXE_LINKER_FLAGS_INIT)

# compile a C file into an object file
SET(CMAKE_C_COMPILE_OBJECT  "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -o <OBJECT> -c <SOURCE>")

# link object files to an executable
SET(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <OBJECTS> --out-fmt-ihx -o  <TARGET> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")

# needs sdcc 2.7.0 + sddclib from cvs
SET(CMAKE_C_CREATE_STATIC_LIBRARY
      "\"${CMAKE_COMMAND}\" -E remove <TARGET>"
      "<CMAKE_AR> -a <TARGET> <LINK_FLAGS> <OBJECTS> ")

# not supported by sdcc
SET(CMAKE_C_CREATE_SHARED_LIBRARY "")
SET(CMAKE_C_CREATE_MODULE_LIBRARY "")

