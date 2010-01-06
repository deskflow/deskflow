include(Compiler/GNU)
__compiler_gnu(Fortran)

# No -DNDEBUG for Fortran.
SET(CMAKE_Fortran_FLAGS_MINSIZEREL_INIT "-Os")
SET(CMAKE_Fortran_FLAGS_RELEASE_INIT "-O3")

# We require updates to CMake C++ code to support preprocessing rules
# for Fortran.
SET(CMAKE_Fortran_CREATE_PREPROCESSED_SOURCE)
SET(CMAKE_Fortran_CREATE_ASSEMBLY_SOURCE)

# Fortran-specific feature flags.
SET(CMAKE_Fortran_MODDIR_FLAG -J)
