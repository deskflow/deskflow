SET(CMAKE_SHARED_LIBRARY_SUFFIX ".sl")          # .so
SET(CMAKE_DL_LIBS "dld")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".sl" ".so" ".a")
SET(CMAKE_EXTRA_SHARED_LIBRARY_SUFFIXES ".so")

# The HP linker needs to find transitive shared library dependencies
# in the -L path.  Therefore the runtime path must be added to the
# link line with -L flags.
SET(CMAKE_SHARED_LIBRARY_LINK_C_WITH_RUNTIME_PATH 1)
SET(CMAKE_LINK_DEPENDENT_LIBRARY_DIRS 1)

# Shared libraries with no builtin soname may not be linked safely by
# specifying the file path.
SET(CMAKE_PLATFORM_USES_PATH_WHEN_NO_SONAME 1)

# fortran
IF(CMAKE_COMPILER_IS_GNUG77)
ELSE(CMAKE_COMPILER_IS_GNUG77)
  # use ld directly to create shared libraries for hp cc
  SET(CMAKE_Fortran_CREATE_SHARED_LIBRARY
      "ld <CMAKE_SHARED_LIBRARY_CREATE_Fortran_FLAGS> <CMAKE_SHARED_LIBRARY_SONAME_Fortran_FLAG><TARGET_SONAME> <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
  SET(CMAKE_SHARED_LIBRARY_Fortran_FLAGS "+Z")            # -pic 
  SET(CMAKE_SHARED_LIBRARY_CREATE_Fortran_FLAGS "-E -b -L/usr/lib")       # -shared
  SET(CMAKE_SHARED_LIBRARY_LINK_Fortran_FLAGS "-Wl,+s,-E,+nodefaultrpath")  # +s, flag for exe link to use shared lib
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_Fortran_FLAG "+b")       # -rpath
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_Fortran_FLAG_SEP ":")   # : or empty
  SET(CMAKE_SHARED_LIBRARY_SONAME_Fortran_FLAG "+h")
  SET(CMAKE_EXECUTABLE_RUNTIME_Fortran_FLAG "-Wl,+b")       # -rpath
ENDIF(CMAKE_COMPILER_IS_GNUG77)

# C compiler
IF(CMAKE_COMPILER_IS_GNUCC)
ELSE(CMAKE_COMPILER_IS_GNUCC)
  # hp cc
  # use ld directly to create shared libraries for hp cc
  SET(CMAKE_C_CREATE_SHARED_LIBRARY
      "ld <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> <CMAKE_SHARED_LIBRARY_SONAME_C_FLAG><TARGET_SONAME> <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
  SET(CMAKE_SHARED_LIBRARY_C_FLAGS "+Z")            # -pic 
  SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-E -b +nodefaultrpath -L/usr/lib")       # -shared
  SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-Wl,+s,-E,+nodefaultrpath")  # +s, flag for exe link to use shared lib
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "+b")       # -rpath
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")   # : or empty
  SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "+h")
  SET(CMAKE_EXECUTABLE_RUNTIME_C_FLAG "-Wl,+b")       # -rpath
  SET(CMAKE_C_FLAGS_INIT "")
  SET(CMAKE_C_COMPILE_OBJECT
    "<CMAKE_C_COMPILER> <DEFINES> -Aa -Ae <FLAGS> -o <OBJECT>   -c <SOURCE>")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# CXX compiler
IF(CMAKE_COMPILER_IS_GNUCXX) 
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  # for hp aCC
  SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "+Z")            # -pic 
  SET(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "+Z -Wl,-E,+nodefaultrpath -b -L/usr/lib")       # -shared
  SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-Wl,+s,-E,+nodefaultrpath")  # +s, flag for exe link to use shared lib
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG "-Wl,+b")       # -rpath
  SET(CMAKE_SHARED_LIBRARY_RUNTIME_CXX_FLAG_SEP ":")   # : or empty
  SET(CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG "-Wl,+h")
  SET(CMAKE_EXECUTABLE_RUNTIME_CXX_FLAG "-Wl,+b")       # -rpath
  SET (CMAKE_CXX_FLAGS_INIT "")
  SET (CMAKE_CXX_FLAGS_DEBUG_INIT "-g")
  SET (CMAKE_CXX_FLAGS_MINSIZEREL_INIT "+O3 -DNDEBUG")
  SET (CMAKE_CXX_FLAGS_RELEASE_INIT "+O2 -DNDEBUG")
  SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-g")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
# set flags for gcc support
INCLUDE(Platform/UnixPaths)

# Look in both 32-bit and 64-bit implict link directories, but tell
# CMake not to pass the paths to the linker.  The linker will find the
# library for the proper architecture.  In the future we should detect
# which path will be used by the linker.  Since the pointer type size
# CMAKE_SIZEOF_VOID_P is not set until after this file executes, we
# would need to append to CMAKE_SYSTEM_LIBRARY_PATH at a later point
# (after CMakeTest(LANG)Compiler.cmake runs for at least one language).
LIST(APPEND CMAKE_SYSTEM_LIBRARY_PATH /usr/lib/hpux32)
LIST(APPEND CMAKE_SYSTEM_LIBRARY_PATH /usr/lib/hpux64)
LIST(APPEND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
  /usr/lib/hpux32 /usr/lib/hpux64)

IF(NOT CMAKE_COMPILER_IS_GNUCC)
  SET (CMAKE_C_CREATE_PREPROCESSED_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
  SET (CMAKE_C_CREATE_ASSEMBLY_SOURCE "<CMAKE_C_COMPILER> <DEFINES> <FLAGS> -S <SOURCE> -o <ASSEMBLY_SOURCE>")
ENDIF(NOT CMAKE_COMPILER_IS_GNUCC)

IF(NOT CMAKE_COMPILER_IS_GNUCXX)
  SET (CMAKE_CXX_CREATE_PREPROCESSED_SOURCE "<CMAKE_CXX_COMPILER> <DEFINES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
  SET (CMAKE_CXX_CREATE_ASSEMBLY_SOURCE
    "<CMAKE_CXX_COMPILER> <DEFINES> <FLAGS> -S <SOURCE>"
    "mv `basename \"<SOURCE>\" | sed 's/\\.[^./]*$$//'`.s <ASSEMBLY_SOURCE>"
    "rm -f `basename \"<SOURCE>\" | sed 's/\\.[^./]*$$//'`.o"
    )
ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)

# Initialize C and CXX link type selection flags.  These flags are
# used when building a shared library, shared module, or executable
# that links to other libraries to select whether to use the static or
# shared versions of the libraries.  Note that C modules and shared
# libs are built using ld directly so we leave off the "-Wl," portion.
FOREACH(type SHARED_LIBRARY SHARED_MODULE)
  SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-a archive")
  SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-a default")
ENDFOREACH(type)
FOREACH(type EXE)
  SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-Wl,-a,archive")
  SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-Wl,-a,default")
ENDFOREACH(type)
FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
  SET(CMAKE_${type}_LINK_STATIC_CXX_FLAGS "-Wl,-a,archive")
  SET(CMAKE_${type}_LINK_DYNAMIC_CXX_FLAGS "-Wl,-a,default")
ENDFOREACH(type)

