SET(CMAKE_SHARED_LIBRARY_LINK_Fortran_FLAGS "")

SET(CMAKE_Fortran_MODDIR_FLAG "-module ")
SET(CMAKE_Fortran_FLAGS_INIT "-Mpreprocess -Kieee -fpic")
SET(CMAKE_Fortran_FLAGS_DEBUG_INIT "-g -O0 -Mbounds")
SET(CMAKE_Fortran_FLAGS_MINSIZEREL_INIT "-O2 -s")
SET(CMAKE_Fortran_FLAGS_RELEASE_INIT "-fast -O3 -Mipa=fast")
SET(CMAKE_Fortran_FLAGS_RELWITHDEBINFO_INIT "-O2 -gopt")

