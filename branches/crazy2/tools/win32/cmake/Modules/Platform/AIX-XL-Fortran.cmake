SET(CMAKE_SHARED_LIBRARY_CREATE_Fortran_FLAGS "-G -Wl,-brtl,-bnoipath")  # -shared
SET(CMAKE_SHARED_LIBRARY_LINK_Fortran_FLAGS "-Wl,-brtl,-bnoipath,-bexpall")  # +s, flag for exe link to use shared lib
SET(CMAKE_SHARED_LIBRARY_Fortran_FLAGS " ")
SET(CMAKE_SHARED_MODULE_Fortran_FLAGS  " ")
