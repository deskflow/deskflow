SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-G -Wl,-brtl,-bnoipath")  # -shared
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-Wl,-brtl,-bnoipath,-bexpall")  # +s, flag for exe link to use shared lib
SET(CMAKE_SHARED_LIBRARY_C_FLAGS " ")
SET(CMAKE_SHARED_MODULE_C_FLAGS  " ")
