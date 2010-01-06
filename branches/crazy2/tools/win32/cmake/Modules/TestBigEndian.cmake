# - Define macro to determine endian type
# Check if the system is big endian or little endian
#  TEST_BIG_ENDIAN(VARIABLE)
#  VARIABLE - variable to store the result to
#

#=============================================================================
# Copyright 2002-2009 Kitware, Inc.
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

MACRO(TEST_BIG_ENDIAN VARIABLE)
  IF("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
    MESSAGE(STATUS "Check if the system is big endian")
    MESSAGE(STATUS "Searching 16 bit integer")
  
    INCLUDE(CheckTypeSize)

    CHECK_TYPE_SIZE("unsigned short" CMAKE_SIZEOF_UNSIGNED_SHORT)
    IF(CMAKE_SIZEOF_UNSIGNED_SHORT EQUAL 2)
      MESSAGE(STATUS "Using unsigned short")
      SET(CMAKE_16BIT_TYPE "unsigned short")
    ELSE(CMAKE_SIZEOF_UNSIGNED_SHORT EQUAL 2)
      CHECK_TYPE_SIZE("unsigned int"   CMAKE_SIZEOF_UNSIGNED_INT)
      IF(CMAKE_SIZEOF_UNSIGNED_INT)
        MESSAGE(STATUS "Using unsigned int")
        SET(CMAKE_16BIT_TYPE "unsigned int")

      ELSE(CMAKE_SIZEOF_UNSIGNED_INT)
  
        CHECK_TYPE_SIZE("unsigned long"  CMAKE_SIZEOF_UNSIGNED_LONG)
        IF(CMAKE_SIZEOF_UNSIGNED_LONG)
          MESSAGE(STATUS "Using unsigned long")
          SET(CMAKE_16BIT_TYPE "unsigned long")
        ELSE(CMAKE_SIZEOF_UNSIGNED_LONG)
          MESSAGE(FATAL_ERROR "no suitable type found")
        ENDIF(CMAKE_SIZEOF_UNSIGNED_LONG)
    
      ENDIF(CMAKE_SIZEOF_UNSIGNED_INT)
    
    ENDIF(CMAKE_SIZEOF_UNSIGNED_SHORT EQUAL 2)

  
    CONFIGURE_FILE("${CMAKE_ROOT}/Modules/TestEndianess.c.in" 
                   "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestEndianess.c"
                    IMMEDIATE @ONLY)
  
     FILE(READ "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestEndianess.c"
          TEST_ENDIANESS_FILE_CONTENT)

     TRY_COMPILE(HAVE_${VARIABLE}
      "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/TestEndianess.c"
      OUTPUT_VARIABLE OUTPUT
      COPY_FILE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TestEndianess.bin" )

      IF(HAVE_${VARIABLE})

        FILE(STRINGS "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TestEndianess.bin"
            CMAKE_TEST_ENDIANESS_STRINGS_LE LIMIT_COUNT 1 REGEX "THIS IS LITTLE ENDIAN")

        FILE(STRINGS "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TestEndianess.bin"
            CMAKE_TEST_ENDIANESS_STRINGS_BE LIMIT_COUNT 1 REGEX "THIS IS BIG ENDIAN")

        # on mac, if there are universal binaries built both will be true
        # return the result depending on the machine on which cmake runs
        IF(CMAKE_TEST_ENDIANESS_STRINGS_BE  AND  CMAKE_TEST_ENDIANESS_STRINGS_LE)
          IF(CMAKE_SYSTEM_PROCESSOR MATCHES powerpc)
            SET(CMAKE_TEST_ENDIANESS_STRINGS_BE TRUE)
            SET(CMAKE_TEST_ENDIANESS_STRINGS_LE FALSE)
          ELSE(CMAKE_SYSTEM_PROCESSOR MATCHES powerpc)
            SET(CMAKE_TEST_ENDIANESS_STRINGS_BE FALSE)
            SET(CMAKE_TEST_ENDIANESS_STRINGS_LE TRUE)
          ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES powerpc)
          MESSAGE(STATUS "TEST_BIG_ENDIAN found different results, consider setting CMAKE_OSX_ARCHITECTURES or CMAKE_TRY_COMPILE_OSX_ARCHITECTURES to one or no architecture !")
        ENDIF(CMAKE_TEST_ENDIANESS_STRINGS_BE  AND  CMAKE_TEST_ENDIANESS_STRINGS_LE)

        IF(CMAKE_TEST_ENDIANESS_STRINGS_LE)
          SET(${VARIABLE} 0 CACHE INTERNAL "Result of TEST_BIG_ENDIAN" FORCE)
          MESSAGE(STATUS "Check if the system is big endian - little endian")
        ENDIF(CMAKE_TEST_ENDIANESS_STRINGS_LE)

        IF(CMAKE_TEST_ENDIANESS_STRINGS_BE)
          SET(${VARIABLE} 1 CACHE INTERNAL "Result of TEST_BIG_ENDIAN" FORCE)
          MESSAGE(STATUS "Check if the system is big endian - big endian")
        ENDIF(CMAKE_TEST_ENDIANESS_STRINGS_BE)

        IF(NOT CMAKE_TEST_ENDIANESS_STRINGS_BE  AND  NOT CMAKE_TEST_ENDIANESS_STRINGS_LE)
          MESSAGE(SEND_ERROR "TEST_BIG_ENDIAN found no result!")
        ENDIF(NOT CMAKE_TEST_ENDIANESS_STRINGS_BE  AND  NOT CMAKE_TEST_ENDIANESS_STRINGS_LE)
 
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the system is big endian passed with the following output:\n${OUTPUT}\nTestEndianess.c:\n${TEST_ENDIANESS_FILE_CONTENT}\n\n")
 
      ELSE(HAVE_${VARIABLE})
        MESSAGE(STATUS "Check if the system is big endian - failed")
        FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the system is big endian failed with the following output:\n${OUTPUT}\nTestEndianess.c:\n${TEST_ENDIANESS_FILE_CONTENT}\n\n")
        SET(${VARIABLE})
      ENDIF(HAVE_${VARIABLE})
  ENDIF("HAVE_${VARIABLE}" MATCHES "^HAVE_${VARIABLE}$")
ENDMACRO(TEST_BIG_ENDIAN)


