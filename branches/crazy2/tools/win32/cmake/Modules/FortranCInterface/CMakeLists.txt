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

cmake_minimum_required(VERSION 2.6.3)
project(FortranCInterface C Fortran)
include(${FortranCInterface_BINARY_DIR}/Input.cmake OPTIONAL)

# Check if the C compiler supports '$' in identifiers.
include(CheckCSourceCompiles)
check_c_source_compiles("
extern int dollar$(void);
int main() { return 0; }
" C_SUPPORTS_DOLLAR)

# List manglings of global symbol names to try.
set(global_symbols
  my_sub    # VisualAge
  my_sub_   # GNU, Intel, HP, SunPro, MIPSpro
  my_sub__  # GNU g77
  MY_SUB    # Intel on Windows
  mysub     # VisualAge
  mysub_    # GNU, Intel, HP, SunPro, MIPSpro
  MYSUB     # Intel on Windows
  ${FortranCInterface_GLOBAL_SYMBOLS}
  )
list(REMOVE_DUPLICATES global_symbols)

# List manglings of module symbol names to try.
set(module_symbols
  __my_module_MOD_my_sub  # GNU 4.3
  __my_module_NMOD_my_sub # VisualAge
  __my_module__my_sub     # GNU 4.2
  __mymodule_MOD_mysub    # GNU 4.3
  __mymodule_NMOD_mysub   # VisualAge
  __mymodule__mysub       # GNU 4.2
  my_module$my_sub        # HP
  my_module_mp_my_sub_    # Intel
  MY_MODULE_mp_MY_SUB     # Intel on Windows
  my_module_my_sub_       # PGI
  mymodule$mysub          # HP
  mymodule_mp_mysub_      # Intel
  MYMODULE_mp_MYSUB       # Intel on Windows
  mymodule_mysub_         # PGI
  ${FortranCInterface_MODULE_SYMBOLS}
  )
list(REMOVE_DUPLICATES module_symbols)

# Note that some compiler manglings cannot be invoked from C:
#   MIPSpro uses "MY_SUB.in.MY_MODULE"
#   SunPro uses "my_module.my_sub_"

# Add module symbols only with Fortran90.
if(CMAKE_Fortran_COMPILER_SUPPORTS_F90)
  set(myfort_modules mymodule.f90 my_module.f90)
  set(call_mod call_mod.f90)
  set_property(SOURCE main.F PROPERTY COMPILE_DEFINITIONS CALL_MOD)
else()
  set(module_symbols)
endif()

# Generate C symbol sources.
foreach(symbol IN LISTS global_symbols module_symbols)
  # Skip symbols with '$' if C cannot handle them.
  if(C_SUPPORTS_DOLLAR OR NOT "${symbol}" MATCHES "\\$")
    if("${symbol}" MATCHES "SUB")
      set(upper "-UPPER")
    else()
      set(upper)
    endif()
    string(REPLACE "$" "S" name "${symbol}")
    set(source ${CMAKE_CURRENT_BINARY_DIR}/symbols/${name}${upper}.c)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/symbol.c.in ${source} @ONLY)
    list(APPEND symbol_sources ${source})
  endif()
endforeach()

# Provide symbols through Fortran.
add_library(myfort STATIC mysub.f my_sub.f ${myfort_modules})

# Provide symbols through C but fall back to Fortran.
add_library(symbols STATIC mymodule_.c my_module_.c ${symbol_sources})
target_link_libraries(symbols myfort)

# Require symbols through Fortran.
add_executable(FortranCInterface main.F call_sub.f ${call_mod})
target_link_libraries(FortranCInterface symbols)
