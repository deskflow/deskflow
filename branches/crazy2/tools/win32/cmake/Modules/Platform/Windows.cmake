SET(WIN32 1)

SET(CMAKE_STATIC_LIBRARY_PREFIX "")
SET(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")
SET(CMAKE_SHARED_LIBRARY_PREFIX "")          # lib
SET(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")          # .so
SET(CMAKE_IMPORT_LIBRARY_PREFIX "")
SET(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
SET(CMAKE_EXECUTABLE_SUFFIX ".exe")          # .exe
SET(CMAKE_LINK_LIBRARY_SUFFIX ".lib")
SET(CMAKE_DL_LIBS "")

SET(CMAKE_FIND_LIBRARY_PREFIXES "")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

# for borland make long command lines are redirected to a file
# with the following syntax, see Windows-bcc32.cmake for use
IF(CMAKE_GENERATOR MATCHES "Borland")
  SET(CMAKE_START_TEMP_FILE "@&&|\n")
  SET(CMAKE_END_TEMP_FILE "\n|")
ENDIF(CMAKE_GENERATOR MATCHES "Borland")

# for nmake make long command lines are redirected to a file
# with the following syntax, see Windows-bcc32.cmake for use
IF(CMAKE_GENERATOR MATCHES "NMake")
  SET(CMAKE_START_TEMP_FILE "@<<\n")
  SET(CMAKE_END_TEMP_FILE "\n<<")
ENDIF(CMAKE_GENERATOR MATCHES "NMake")

INCLUDE(Platform/WindowsPaths)

# uncomment these out to debug nmake and borland makefiles
#SET(CMAKE_START_TEMP_FILE "")
#SET(CMAKE_END_TEMP_FILE "")
#SET(CMAKE_VERBOSE_MAKEFILE 1)

