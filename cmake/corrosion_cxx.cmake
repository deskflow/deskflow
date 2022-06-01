
# Creates a target including rust lib and cxxbridge named ${NAMESPACE}::${LAST STEM OF PATH}
# <LAST STEM OF PATH> must match the crate name ie "some/path/to/myrustcrate" -> "libmyrustcrate.a"
function(add_library_rust)
    # set(OPTIONS)
    set(ONE_VALUE_KEYWORDS PATH NAMESPACE CXX_BRIDGE_SOURCE_FILE)
    # set(MULTI_VALUE_KEYWORDS)
    cmake_parse_arguments(_RUST_LIB "${OPTIONS}" "${ONE_VALUE_KEYWORDS}" "${MULTI_VALUE_KEYWORDS}" ${ARGN})


    ### Check inputs
    if("${_RUST_LIB_PATH}" STREQUAL "")
        message(FATAL_ERROR "add_library_rust called without a given path to root of a rust crate, fix by adding 'PATH <pathToRustlibRoot>'")
    endif()

    if("${_RUST_LIB_NAMESPACE}" STREQUAL "")
        message(FATAL_ERROR "Must supply a namespace given by keyvalue NAMESPACE <value>")
    endif()

    if("${_RUST_LIB_CXX_BRIDGE_SOURCE_FILE}" STREQUAL "")
        set(_RUST_LIB_CXX_BRIDGE_SOURCE_FILE "src/lib.rs")
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_RUST_LIB_PATH}/Cargo.toml")
        message(FATAL_ERROR "The path ${CMAKE_CURRENT_LIST_DIR}/${_RUST_LIB_PATH} does not contain a Cargo.toml")
    endif()

    ## Simplyfy inputs
    set(_LIB_PATH ${_RUST_LIB_PATH})
    set(_NAMESPACE ${_RUST_LIB_NAMESPACE})
    set(_CXX_BRIDGE_SOURCE_FILE ${_RUST_LIB_CXX_BRIDGE_SOURCE_FILE})

    ## Import Rust target
    corrosion_import_crate(MANIFEST_PATH "${_LIB_PATH}/Cargo.toml")
    
    ## Set cxxbridge values

    # parse stem
    _get_stem_name_of_path(PATH ${_LIB_PATH})
    set(_LIB_PATH_STEM ${STEM_OF_PATH})

    if (CMAKE_VS_PLATFORM_NAME)
        set(CXXBRIDGE_BINARY_FOLDER ${CMAKE_BINARY_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/cargo/build/${_CORROSION_RUST_CARGO_TARGET}/cxxbridge) 
    elseif(CMAKE_CONFIGURATION_TYPES)
        message(FATAL_ERROR "Config types no platform")
        # set(CXXBRIDGE_BINARY_FOLDER ${CMAKE_BINARY_DIR}/cargo/build/${_CORROSION_RUST_CARGO_TARGET}/cxxbridge) 
    else()
        set(CXXBRIDGE_BINARY_FOLDER ${CMAKE_BINARY_DIR}/cargo/build/${_CORROSION_RUST_CARGO_TARGET}/cxxbridge)
    endif()
    
    set(COMMON_HEADER ${CXXBRIDGE_BINARY_FOLDER}/rust/cxx.h)
    set(BINDING_HEADER ${CXXBRIDGE_BINARY_FOLDER}/${_LIB_PATH_STEM}/${_CXX_BRIDGE_SOURCE_FILE}.h)
    set(BINDING_SOURCE ${CXXBRIDGE_BINARY_FOLDER}/${_LIB_PATH_STEM}/${_CXX_BRIDGE_SOURCE_FILE}.cc)
    set(CXX_BINDING_INCLUDE_DIR ${CXXBRIDGE_BINARY_FOLDER})
    

    ## Create cxxbridge target
    add_custom_command(
        DEPENDS ${_LIB_PATH_STEM}-static
        OUTPUT
            ${COMMON_HEADER}
            ${BINDING_HEADER}
            ${BINDING_SOURCE} 
    )

    add_library(${_LIB_PATH_STEM}_cxxbridge)
    target_sources(${_LIB_PATH_STEM}_cxxbridge
        PUBLIC
            ${COMMON_HEADER}
            ${BINDING_HEADER}
            ${BINDING_SOURCE}
    )
    target_include_directories(${_LIB_PATH_STEM}_cxxbridge
        PUBLIC
            ${CXX_BINDING_INCLUDE_DIR}
    )

    ## Create total target with alias with given namespace
    add_library(${_LIB_PATH_STEM}-total INTERFACE)
    target_link_libraries(${_LIB_PATH_STEM}-total
        INTERFACE
            ${_LIB_PATH_STEM}_cxxbridge
            ${_LIB_PATH_STEM}
    )
    # for end-user to link into project
    add_library(${_NAMESPACE}::${_LIB_PATH_STEM} ALIAS ${_LIB_PATH_STEM}-total)
    
endfunction(add_library_rust)


function(_get_stem_name_of_path)
    # set(OPTIONS)
    set(ONE_VALUE_KEYWORDS PATH)
    # set(MULTI_VALUE_KEYWORDS)
    cmake_parse_arguments(_PATH_STEM "${OPTIONS}" "${ONE_VALUE_KEYWORDS}" "${MULTI_VALUE_KEYWORDS}" ${ARGN})

    ### Check inputs
    if("${_PATH_STEM_PATH}" STREQUAL "")
        message(FATAL_ERROR "Path to get stem for is empty!")
    endif()

    # Convert all slashes to forward slash
    set(_PATH ${_PATH_STEM_PATH})
    ## Replace all Windows double slashes
    string(REPLACE "\\\\" "/" _PATH_OUTPUT ${_PATH})
    set(_PATH ${_PATH_OUTPUT})
    ## Replace all Windows single slashes
    string(REPLACE "\\" "/" _PATH_OUTPUT ${_PATH}) 
    set(_PATH ${_PATH_OUTPUT})

    # Convert to list
    string(REPLACE "/" ";" _PATH_AS_LIST ${_PATH})
    list(LENGTH _PATH_AS_LIST LIST_LENGTH)
    math(EXPR INDEX "${LIST_LENGTH} - 1" OUTPUT_FORMAT DECIMAL)

    list(GET _PATH_AS_LIST "${INDEX}" _STEM)

    set(STEM_OF_PATH ${_STEM} PARENT_SCOPE)
endfunction(_get_stem_name_of_path)
