SET(CMAKE_SHARED_LIBRARY_PREFIX "lib")          # lib
SET(CMAKE_SHARED_LIBRARY_SUFFIX ".so")          # .so
SET(CMAKE_DL_LIBS "-lld")

# RPATH support on AIX is called libpath.  By default the runtime
# libpath is paths specified by -L followed by /usr/lib and /lib.  In
# order to prevent the -L paths from being used we must force use of
# -Wl,-blibpath:/usr/lib:/lib whether RPATH support is on or not.
# When our own RPATH is to be added it may be inserted before the
# "always" paths.
SET(CMAKE_PLATFORM_REQUIRED_RUNTIME_PATH /usr/lib /lib)
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-blibpath:")
SET(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")

# Files named "libfoo.a" may actually be shared libraries.
SET_PROPERTY(GLOBAL PROPERTY TARGET_ARCHIVES_MAY_BE_SHARED_LIBS 1)

# since .a can be a static or shared library on AIX, we can not do this.
# at some point if we wanted it, we would have to figure out if a .a is
# static or shared, then we could add this back:

# Initialize C link type selection flags.  These flags are used when
# building a shared library, shared module, or executable that links
# to other libraries to select whether to use the static or shared
# versions of the libraries.
#FOREACH(type SHARED_LIBRARY SHARED_MODULE EXE)
#  SET(CMAKE_${type}_LINK_STATIC_C_FLAGS "-bstatic")
#  SET(CMAKE_${type}_LINK_DYNAMIC_C_FLAGS "-bdynamic")
#ENDFOREACH(type)

INCLUDE(Platform/UnixPaths)
