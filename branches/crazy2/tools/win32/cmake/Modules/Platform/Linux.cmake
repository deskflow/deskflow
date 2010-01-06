SET(CMAKE_DL_LIBS "dl")
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

# Debian policy requires that shared libraries be installed without
# executable permission.  Fedora policy requires that shared libraries
# be installed with the executable permission.  Since the native tools
# create shared libraries with execute permission in the first place a
# reasonable policy seems to be to install with execute permission by
# default.  In order to support debian packages we provide an option
# here.  The option default is based on the current distribution, but
# packagers can set it explicitly on the command line.
IF(DEFINED CMAKE_INSTALL_SO_NO_EXE)
  # Store the decision variable in the cache.  This preserves any
  # setting the user provides on the command line.
  SET(CMAKE_INSTALL_SO_NO_EXE "${CMAKE_INSTALL_SO_NO_EXE}" CACHE INTERNAL
    "Install .so files without execute permission.")
ELSE(DEFINED CMAKE_INSTALL_SO_NO_EXE)
  # Store the decision variable as an internal cache entry to avoid
  # checking the platform every time.  This option is advanced enough
  # that only package maintainers should need to adjust it.  They are
  # capable of providing a setting on the command line.
  IF(EXISTS "/etc/debian_version")
    SET(CMAKE_INSTALL_SO_NO_EXE 1 CACHE INTERNAL
      "Install .so files without execute permission.")
  ELSE(EXISTS "/etc/debian_version")
    SET(CMAKE_INSTALL_SO_NO_EXE 0 CACHE INTERNAL
      "Install .so files without execute permission.")
  ENDIF(EXISTS "/etc/debian_version")
ENDIF(DEFINED CMAKE_INSTALL_SO_NO_EXE)

INCLUDE(Platform/UnixPaths)

# Debian has lib64 paths only for compatibility so they should not be
# searched.
IF(EXISTS "/etc/debian_version")
  SET_PROPERTY(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE)
ENDIF(EXISTS "/etc/debian_version")
