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

INCLUDE(Platform/UnixPaths)
