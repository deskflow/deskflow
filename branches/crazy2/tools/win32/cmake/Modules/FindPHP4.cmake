# - Find PHP4
# This module finds if PHP4 is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PHP4_INCLUDE_PATH       = path to where php.h can be found
#  PHP4_EXECUTABLE         = full path to the php4 binary
#

#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
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

SET(PHP4_POSSIBLE_INCLUDE_PATHS
  /usr/include/php4
  /usr/local/include/php4
  /usr/include/php
  /usr/local/include/php
  /usr/local/apache/php
  )

SET(PHP4_POSSIBLE_LIB_PATHS
  /usr/lib
  )

FIND_PATH(PHP4_FOUND_INCLUDE_PATH main/php.h
  ${PHP4_POSSIBLE_INCLUDE_PATHS})

IF(PHP4_FOUND_INCLUDE_PATH)
  SET(php4_paths "${PHP4_POSSIBLE_INCLUDE_PATHS}")
  FOREACH(php4_path Zend main TSRM)
    SET(php4_paths ${php4_paths} "${PHP4_FOUND_INCLUDE_PATH}/${php4_path}")
  ENDFOREACH(php4_path Zend main TSRM)
  SET(PHP4_INCLUDE_PATH "${php4_paths}" INTERNAL "PHP4 include paths")
ENDIF(PHP4_FOUND_INCLUDE_PATH)

FIND_PROGRAM(PHP4_EXECUTABLE NAMES php4 php )

MARK_AS_ADVANCED(
  PHP4_EXECUTABLE
  PHP4_FOUND_INCLUDE_PATH
  )

IF(APPLE)
# this is a hack for now
  SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS 
   "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS} -Wl,-flat_namespace")
  FOREACH(symbol
    __efree
    __emalloc
    __estrdup
    __object_init_ex
    __zend_get_parameters_array_ex
    __zend_list_find
    __zval_copy_ctor
    _add_property_zval_ex
    _alloc_globals
    _compiler_globals
    _convert_to_double
    _convert_to_long
    _zend_error
    _zend_hash_find
    _zend_register_internal_class_ex
    _zend_register_list_destructors_ex
    _zend_register_resource
    _zend_rsrc_list_get_rsrc_type
    _zend_wrong_param_count
    _zval_used_for_init
    )
    SET(CMAKE_SHARED_MODULE_CREATE_C_FLAGS 
      "${CMAKE_SHARED_MODULE_CREATE_C_FLAGS},-U,${symbol}")
  ENDFOREACH(symbol)
ENDIF(APPLE)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PHP4 DEFAULT_MSG PHP4_EXECUTABLE PHP4_INCLUDE_PATH)
