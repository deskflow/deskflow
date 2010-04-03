macro(cmake_gnu_has_isysroot lang)
  if("x${CMAKE_${lang}_HAS_ISYSROOT}" STREQUAL "x")
    set(_doc "${lang} compiler has -isysroot")
    message(STATUS "Checking whether ${_doc}")
    execute_process(
      COMMAND ${CMAKE_${lang}_COMPILER} "-v" "--help"
      OUTPUT_VARIABLE _gcc_help
      ERROR_VARIABLE _gcc_help
      )
    if("${_gcc_help}" MATCHES "isysroot")
      message(STATUS "Checking whether ${_doc} - yes")
      set(CMAKE_${lang}_HAS_ISYSROOT 1)
    else()
      message(STATUS "Checking whether ${_doc} - no")
      set(CMAKE_${lang}_HAS_ISYSROOT 0)
    endif()
  endif()
endmacro()
