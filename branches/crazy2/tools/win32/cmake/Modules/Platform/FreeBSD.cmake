IF(EXISTS /usr/include/dlfcn.h)
  SET(CMAKE_DL_LIBS "")
  SET(CMAKE_SHARED_LIBRARY_C_FLAGS "-fPIC")            # -pic 
  SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-shared")       # -shared
  SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")         # +s, flag for exe link to use shared lib
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")       # -rpath
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")   # : or empty
  SET(CMAKE_SHARED_LIBRARY_RPATH_LINK_C_FLAG "-Wl,-rpath-link,")
  SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-Wl,-soname,")
  SET(CMAKE_EXE_EXPORTS_C_FLAG "-Wl,--export-dynamic")
ENDIF(EXISTS /usr/include/dlfcn.h)

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

INCLUDE(Platform/UnixPaths)
