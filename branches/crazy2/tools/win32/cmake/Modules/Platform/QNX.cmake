SET(QNXNTO 1)

# The QNX GCC does not seem to have -isystem so remove the flag.
SET(CMAKE_INCLUDE_SYSTEM_FLAG_C)
SET(CMAKE_INCLUDE_SYSTEM_FLAG_CXX)

SET(CMAKE_DL_LIBS "")
SET(CMAKE_SHARED_LIBRARY_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")
SET(CMAKE_SHARED_LIBRARY_RPATH_LINK_C_FLAG "-Wl,-rpath-link,")
SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-Wl,-soname,")
SET(CMAKE_EXE_EXPORTS_C_FLAG "-Wl,--export-dynamic")

# Shared libraries with no builtin soname may not be linked safely by
# specifying the file path.
SET(CMAKE_PLATFORM_USES_PATH_WHEN_NO_SONAME 1)

# Initialize C link type selection flags.  These flags are used when
# building a shared library, shared module, or executable that links
# to other libraries to select whether to use the static or shared
# versions of the libraries.
FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
  SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-Wl,-Bstatic")
  SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-Wl,-Bdynamic")
ENDFOREACH(type)
# force the language to be c++ since qnx only has gcc and not g++ and c++?
SET(CMAKE_CXX_COMPILE_OBJECT
  "<CMAKE_CXX_COMPILER> -x c++ <DEFINES> <FLAGS> -o <OBJECT> -c <SOURCE>")

INCLUDE(Platform/UnixPaths)
