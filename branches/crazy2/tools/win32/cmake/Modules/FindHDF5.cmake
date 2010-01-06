# - Find HDF5, a library for reading and writing self describing array data.
#
# This module invokes the HDF5 wrapper compiler that should be installed
# alongside HDF5.  Depending upon the HDF5 Configuration, the wrapper compiler
# is called either h5cc or h5pcc.  If this succeeds, the module will then call
# the compiler with the -show argument to see what flags are used when compiling
# an HDF5 client application.
#
# The module will optionally accept the COMPONENTS argument.  If no COMPONENTS
# are specified, then the find module will default to finding only the HDF5 C
# library.  If one or more COMPONENTS are specified, the module will attempt to
# find the language bindings for the specified components.  Currently, the only
# valid components are C and CXX.  The module does not yet support finding the
# Fortran bindings.  If the COMPONENTS argument is not given, the module will
# attempt to find only the C bindings.
#
# On UNIX systems, this module will read the variable HDF5_USE_STATIC_LIBRARIES
# to determine whether or not to prefer a static link to a dynamic link for HDF5
# and all of it's dependencies.  To use this feature, make sure that the
# HDF5_USE_STATIC_LIBRARIES variable is set before the call to find_package.
#
# To provide the module with a hint about where to find your HDF5 installation,
# you can set the environment variable HDF5_ROOT.  The Find module will then
# look in this path when searching for HDF5 executables, paths, and libraries.
#
# In addition to finding the includes and libraries required to compile an HDF5
# client application, this module also makes an effort to find tools that come
# with the HDF5 distribution that may be useful for regression testing.
# 
# This module will define the following variables:
#  HDF5_INCLUDE_DIRS - Location of the hdf5 includes
#  HDF5_INCLUDE_DIR - Location of the hdf5 includes (deprecated)
#  HDF5_DEFINITIONS - Required compiler definitions for HDF5
#  HDF5_C_LIBRARIES - Required libraries for the HDF5 C bindings.
#  HDF5_CXX_LIBRARIES - Required libraries for the HDF5 C++ bindings
#  HDF5_LIBRARIES - Required libraries for all requested bindings
#  HDF5_FOUND - true if HDF5 was found on the system
#  HDF5_LIBRARY_DIRS - the full set of library directories
#  HDF5_IS_PARALLEL - Whether or not HDF5 was found with parallel IO support
#  HDF5_C_COMPILER_EXECUTABLE - the path to the HDF5 C wrapper compiler
#  HDF5_CXX_COMPILER_EXECUTABLE - the path to the HDF5 C++ wrapper compiler
#  HDF5_DIFF_EXECUTABLE - the path to the HDF5 dataset comparison tool

#=============================================================================
# Copyright 2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# This module is maintained by Will Dicharry <wdicharry@stellarscience.com>.

include(SelectLibraryConfigurations)
include(FindPackageHandleStandardArgs)

# List of the valid HDF5 components
set( HDF5_VALID_COMPONENTS 
    C
    CXX
)

# try to find the HDF5 wrapper compilers
find_program( HDF5_C_COMPILER_EXECUTABLE
    NAMES h5cc h5pcc
    HINTS ENV HDF5_ROOT
    PATH_SUFFIXES bin Bin
    DOC "HDF5 Wrapper compiler.  Used only to detect HDF5 compile flags." )
mark_as_advanced( HDF5_C_COMPILER_EXECUTABLE )

find_program( HDF5_CXX_COMPILER_EXECUTABLE
    NAMES h5c++ h5pc++
    HINTS ENV HDF5_ROOT
    PATH_SUFFIXES bin Bin
    DOC "HDF5 C++ Wrapper compiler.  Used only to detect HDF5 compile flags." )
mark_as_advanced( HDF5_CXX_COMPILER_EXECUTABLE )

find_program( HDF5_DIFF_EXECUTABLE 
    NAMES h5diff
    HINTS ENV HDF5_ROOT
    PATH_SUFFIXES bin Bin 
    DOC "HDF5 file differencing tool." )
mark_as_advanced( HDF5_DIFF_EXECUTABLE )

# Invoke the HDF5 wrapper compiler.  The compiler return value is stored to the
# return_value argument, the text output is stored to the output variable.
macro( _HDF5_invoke_compiler language output return_value )
    if( HDF5_${language}_COMPILER_EXECUTABLE )
        exec_program( ${HDF5_${language}_COMPILER_EXECUTABLE} 
            ARGS -show
            OUTPUT_VARIABLE ${output}
            RETURN_VALUE ${return_value}
        )
        if( ${${return_value}} EQUAL 0 )
            # do nothing
        else()
            message( STATUS 
              "Unable to determine HDF5 ${language} flags from HDF5 wrapper." )
        endif()
    endif()
endmacro()

# Parse a compile line for definitions, includes, library paths, and libraries.
macro( _HDF5_parse_compile_line 
    compile_line_var
    include_paths
    definitions
    library_paths
    libraries )

    # Match the include paths
    string( REGEX MATCHALL "-I([^\" ]+)" include_path_flags 
        "${${compile_line_var}}"
    )
    foreach( IPATH ${include_path_flags} )
        string( REGEX REPLACE "^-I" "" IPATH ${IPATH} )
        string( REGEX REPLACE "//" "/" IPATH ${IPATH} )
        list( APPEND ${include_paths} ${IPATH} )
    endforeach()

    # Match the definitions
    string( REGEX MATCHALL "-D[^ ]*" definition_flags "${${compile_line_var}}" )
    foreach( DEF ${definition_flags} )
        list( APPEND ${definitions} ${DEF} )
    endforeach()

    # Match the library paths
    string( REGEX MATCHALL "-L([^\" ]+|\"[^\"]+\")" library_path_flags
        "${${compile_line_var}}"
    )
    
    foreach( LPATH ${library_path_flags} )
        string( REGEX REPLACE "^-L" "" LPATH ${LPATH} )
        string( REGEX REPLACE "//" "/" LPATH ${LPATH} )
        list( APPEND ${library_paths} ${LPATH} )
    endforeach()

    # now search for the library names specified in the compile line (match -l...)
    # match only -l's preceded by a space or comma
    # this is to exclude directory names like xxx-linux/
    string( REGEX MATCHALL "[, ]-l([^\", ]+)" library_name_flags
        "${${compile_line_var}}" )
    # strip the -l from all of the library flags and add to the search list
    foreach( LIB ${library_name_flags} )
        string( REGEX REPLACE "^[, ]-l" "" LIB ${LIB} )
        list( APPEND ${libraries} ${LIB} )
    endforeach()
endmacro()

if( HDF5_INCLUDE_DIRS AND HDF5_LIBRARIES )
    # Do nothing: we already have HDF5_INCLUDE_PATH and HDF5_LIBRARIES in the
    # cache, it would be a shame to override them
else()
    _HDF5_invoke_compiler( C HDF5_C_COMPILE_LINE HDF5_C_RETURN_VALUE )
    _HDF5_invoke_compiler( CXX HDF5_CXX_COMPILE_LINE HDF5_CXX_RETURN_VALUE )

    if( NOT HDF5_FIND_COMPONENTS )
        set( HDF5_LANGUAGE_BINDINGS "C" )
    else()
        # add the extra specified components, ensuring that they are valid.
        foreach( component ${HDF5_FIND_COMPONENTS} )
            list( FIND HDF5_VALID_COMPONENTS ${component} component_location )
            if( ${component_location} EQUAL -1 )
                message( FATAL_ERROR  
                    "\"${component}\" is not a valid HDF5 component." )
            else()
                list( APPEND HDF5_LANGUAGE_BINDINGS ${component} )
            endif()
        endforeach()
    endif()
    
    # seed the initial lists of libraries to find with items we know we need
    set( HDF5_C_LIBRARY_NAMES_INIT hdf5_hl hdf5 )
    set( HDF5_CXX_LIBRARY_NAMES_INIT hdf5_cpp ${HDF5_C_LIBRARY_NAMES_INIT} )
    
    foreach( LANGUAGE ${HDF5_LANGUAGE_BINDINGS} )
        if( HDF5_${LANGUAGE}_COMPILE_LINE )
            _HDF5_parse_compile_line( HDF5_${LANGUAGE}_COMPILE_LINE
                HDF5_${LANGUAGE}_INCLUDE_FLAGS
                HDF5_${LANGUAGE}_DEFINITIONS
                HDF5_${LANGUAGE}_LIBRARY_DIRS
                HDF5_${LANGUAGE}_LIBRARY_NAMES
            )
        
            # take a guess that the includes may be in the 'include' sibling directory
            # of a library directory.
            foreach( dir ${HDF5_${LANGUAGE}_LIBRARY_DIRS} )
                list( APPEND HDF5_${LANGUAGE}_INCLUDE_FLAGS ${dir}/../include )
            endforeach()
        endif()

        # set the definitions for the language bindings.
        list( APPEND HDF5_DEFINITIONS ${HDF5_${LANGUAGE}_DEFINITIONS} )
    
        # find the HDF5 include directories
        find_path( HDF5_${LANGUAGE}_INCLUDE_DIR hdf5.h
            HINTS
                ${HDF5_${LANGUAGE}_INCLUDE_FLAGS}
                ENV
                    HDF5_ROOT
            PATHS 
                $ENV{HOME}/.local/include
            PATH_SUFFIXES
                include
                Include
        )
        mark_as_advanced( HDF5_${LANGUAGE}_INCLUDE_DIR )
        list( APPEND HDF5_INCLUDE_DIRS ${HDF5_${LANGUAGE}_INCLUDE_DIR} )
        
        set( HDF5_${LANGUAGE}_LIBRARY_NAMES 
            ${HDF5_${LANGUAGE}_LIBRARY_NAMES_INIT} 
            ${HDF5_${LANGUAGE}_LIBRARY_NAMES} )
        
        # find the HDF5 libraries
        foreach( LIB ${HDF5_${LANGUAGE}_LIBRARY_NAMES} )
            if( UNIX AND HDF5_USE_STATIC_LIBRARIES )
                # According to bug 1643 on the CMake bug tracker, this is the
                # preferred method for searching for a static library.
                # See http://www.cmake.org/Bug/view.php?id=1643.  We search
                # first for the full static library name, but fall back to a
                # generic search on the name if the static search fails.
                set( THIS_LIBRARY_SEARCH_DEBUG lib${LIB}d.a ${LIB}d )
                set( THIS_LIBRARY_SEARCH_RELEASE lib${LIB}.a ${LIB} )
            else()
                set( THIS_LIBRARY_SEARCH_DEBUG ${LIB}d )
                set( THIS_LIBRARY_SEARCH_RELEASE ${LIB} )
            endif()
            find_library( HDF5_${LIB}_LIBRARY_DEBUG 
                NAMES ${THIS_LIBRARY_SEARCH_DEBUG} 
                HINTS ${HDF5_${LANGUAGE}_LIBRARY_DIRS} 
                ENV HDF5_ROOT 
                PATH_SUFFIXES lib Lib )
            find_library( HDF5_${LIB}_LIBRARY_RELEASE
                NAMES ${THIS_LIBRARY_SEARCH_RELEASE} 
                HINTS ${HDF5_${LANGUAGE}_LIBRARY_DIRS} 
                ENV HDF5_ROOT 
                PATH_SUFFIXES lib Lib )
            select_library_configurations( HDF5_${LIB} )
            # even though we adjusted the individual library names in
            # select_library_configurations, we still need to distinguish
            # between debug and release variants because HDF5_LIBRARIES will
            # need to specify different lists for debug and optimized builds.
            # We can't just use the HDF5_${LIB}_LIBRARY variable (which was set
            # up by the selection macro above) because it may specify debug and
            # optimized variants for a particular library, but a list of
            # libraries is allowed to specify debug and optimized only once.
            list( APPEND HDF5_${LANGUAGE}_LIBRARIES_DEBUG 
                ${HDF5_${LIB}_LIBRARY_DEBUG} )
            list( APPEND HDF5_${LANGUAGE}_LIBRARIES_RELEASE 
                ${HDF5_${LIB}_LIBRARY_RELEASE} )
        endforeach()
        list( APPEND HDF5_LIBRARY_DIRS ${HDF5_${LANGUAGE}_LIBRARY_DIRS} )
        
        # Append the libraries for this language binding to the list of all
        # required libraries.
        list( APPEND HDF5_LIBRARIES_DEBUG 
            ${HDF5_${LANGUAGE}_LIBRARIES_DEBUG} )
        list( APPEND HDF5_LIBRARIES_RELEASE
            ${HDF5_${LANGUAGE}_LIBRARIES_RELEASE} )
    endforeach()

    # We may have picked up some duplicates in various lists during the above
    # process for the language bindings (both the C and C++ bindings depend on
    # libz for example).  Remove the duplicates.
    if( HDF5_INCLUDE_DIRS )
        list( REMOVE_DUPLICATES HDF5_INCLUDE_DIRS )
    endif()
    if( HDF5_LIBRARIES_DEBUG )
        list( REMOVE_DUPLICATES HDF5_LIBRARIES_DEBUG )
    endif()
    if( HDF5_LIBRARIES_RELEASE )
        list( REMOVE_DUPLICATES HDF5_LIBRARIES_RELEASE )
    endif()
    if( HDF5_LIBRARY_DIRS )
        list( REMOVE_DUPLICATES HDF5_LIBRARY_DIRS )
    endif()

    # Construct the complete list of HDF5 libraries with debug and optimized
    # variants when the generator supports them.
    if( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
        set( HDF5_LIBRARIES
            debug ${HDF5_LIBRARIES_DEBUG}
            optimized ${HDF5_LIBRARIES_RELEASE} )
    else()
        set( HDF5_LIBRARIES ${HDF5_LIBRARIES_RELEASE} )
    endif()

    # If the HDF5 include directory was found, open H5pubconf.h to determine if
    # HDF5 was compiled with parallel IO support
    set( HDF5_IS_PARALLEL FALSE )
    foreach( _dir HDF5_INCLUDE_DIRS )
        if( EXISTS "${_dir}/H5pubconf.h" )
            file( STRINGS "${_dir}/H5pubconf.h" 
                HDF5_HAVE_PARALLEL_DEFINE
                REGEX "HAVE_PARALLEL 1" )
            if( HDF5_HAVE_PARALLEL_DEFINE )
                set( HDF5_IS_PARALLEL TRUE )
            endif()
        endif()
    endforeach()
    set( HDF5_IS_PARALLEL ${HDF5_IS_PARALLEL} CACHE BOOL
        "HDF5 library compiled with parallel IO support" )
    mark_as_advanced( HDF5_IS_PARALLEL )

endif()

find_package_handle_standard_args( HDF5 DEFAULT_MSG 
    HDF5_LIBRARIES 
    HDF5_INCLUDE_DIRS
)

mark_as_advanced( 
    HDF5_INCLUDE_DIRS 
    HDF5_LIBRARIES 
    HDF5_DEFINTIONS
    HDF5_LIBRARY_DIRS
    HDF5_C_COMPILER_EXECUTABLE
    HDF5_CXX_COMPILER_EXECUTABLE )

# For backwards compatibility we set HDF5_INCLUDE_DIR to the value of
# HDF5_INCLUDE_DIRS
set( HDF5_INCLUDE_DIR "${HDF5_INCLUDE_DIRS}" )

