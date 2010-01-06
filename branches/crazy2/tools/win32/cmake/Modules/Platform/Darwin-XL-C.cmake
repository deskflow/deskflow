SET(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-qmkshrobj")
SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")

# Enable shared library versioning.
SET(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-Wl,-install_name")
