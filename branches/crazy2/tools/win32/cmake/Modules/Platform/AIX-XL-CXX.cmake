SET(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-G -Wl,-brtl,-bnoipath")       # -shared
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-Wl,-brtl,-bnoipath,-bexpall")  # +s, flag for exe link to use shared lib
SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS " ")
SET(CMAKE_SHARED_MODULE_CXX_FLAGS  " ")
