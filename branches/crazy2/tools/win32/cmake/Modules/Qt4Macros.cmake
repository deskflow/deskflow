# This file is included by FindQt4.cmake, don't include it directly.

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
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


######################################
#
#       Macros for building Qt files
#
######################################


MACRO (QT4_EXTRACT_OPTIONS _qt4_files _qt4_options)
  SET(${_qt4_files})
  SET(${_qt4_options})
  SET(_QT4_DOING_OPTIONS FALSE)
  FOREACH(_currentArg ${ARGN})
    IF ("${_currentArg}" STREQUAL "OPTIONS")
      SET(_QT4_DOING_OPTIONS TRUE)
    ELSE ("${_currentArg}" STREQUAL "OPTIONS")
      IF(_QT4_DOING_OPTIONS) 
        LIST(APPEND ${_qt4_options} "${_currentArg}")
      ELSE(_QT4_DOING_OPTIONS)
        LIST(APPEND ${_qt4_files} "${_currentArg}")
      ENDIF(_QT4_DOING_OPTIONS)
    ENDIF ("${_currentArg}" STREQUAL "OPTIONS")
  ENDFOREACH(_currentArg) 
ENDMACRO (QT4_EXTRACT_OPTIONS)


# macro used to create the names of output files preserving relative dirs
MACRO (QT4_MAKE_OUTPUT_FILE infile prefix ext outfile )
  STRING(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
  STRING(LENGTH ${infile} _infileLength)
  SET(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})
  IF(_infileLength GREATER _binlength)
    STRING(SUBSTRING "${infile}" 0 ${_binlength} _checkinfile)
    IF(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
      FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_BINARY_DIR} ${infile})
    ELSE(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
      FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
    ENDIF(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  ELSE(_infileLength GREATER _binlength)
    FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
  ENDIF(_infileLength GREATER _binlength)
  IF(WIN32 AND rel MATCHES "^[a-zA-Z]:") # absolute path 
    STRING(REGEX REPLACE "^([a-zA-Z]):(.*)$" "\\1_\\2" rel "${rel}")
  ENDIF(WIN32 AND rel MATCHES "^[a-zA-Z]:") 
  SET(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${rel}")
  STRING(REPLACE ".." "__" _outfile ${_outfile})
  GET_FILENAME_COMPONENT(outpath ${_outfile} PATH)
  GET_FILENAME_COMPONENT(_outfile ${_outfile} NAME_WE)
  FILE(MAKE_DIRECTORY ${outpath})
  SET(${outfile} ${outpath}/${prefix}${_outfile}.${ext})
ENDMACRO (QT4_MAKE_OUTPUT_FILE )


MACRO (QT4_GET_MOC_FLAGS _moc_flags)
  SET(${_moc_flags})
  GET_DIRECTORY_PROPERTY(_inc_DIRS INCLUDE_DIRECTORIES)

  FOREACH(_current ${_inc_DIRS})
    IF("${_current}" MATCHES ".framework/?$")
      STRING(REGEX REPLACE "/[^/]+.framework" "" framework_path "${_current}")
      SET(${_moc_flags} ${${_moc_flags}} "-F${framework_path}")
    ELSE("${_current}" MATCHES ".framework/?$")
      SET(${_moc_flags} ${${_moc_flags}} "-I${_current}")
    ENDIF("${_current}" MATCHES ".framework/?$")
  ENDFOREACH(_current ${_inc_DIRS})

  GET_DIRECTORY_PROPERTY(_defines COMPILE_DEFINITIONS)
  FOREACH(_current ${_defines})
    SET(${_moc_flags} ${${_moc_flags}} "-D${_current}")
  ENDFOREACH(_current ${_defines})

  IF(Q_WS_WIN)
    SET(${_moc_flags} ${${_moc_flags}} -DWIN32)
  ENDIF(Q_WS_WIN)

ENDMACRO(QT4_GET_MOC_FLAGS)


# helper macro to set up a moc rule
MACRO (QT4_CREATE_MOC_COMMAND infile outfile moc_flags moc_options)
  # For Windows, create a parameters file to work around command line length limit
  IF (WIN32)
    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    GET_FILENAME_COMPONENT(_moc_outfile_name "${outfile}" NAME)
    GET_FILENAME_COMPONENT(_moc_outfile_dir "${outfile}" PATH)
    IF(_moc_outfile_dir)
      SET(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    ENDIF(_moc_outfile_dir)
    SET (_moc_parameters_file ${outfile}_parameters)
    SET (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    FILE (REMOVE ${_moc_parameters_file})
    FOREACH(arg ${_moc_parameters})
      FILE (APPEND ${_moc_parameters_file} "${arg}\n")
    ENDFOREACH(arg)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE} @${_moc_outfile_name}_parameters
                       DEPENDS ${infile}
                       ${_moc_working_dir}
                       VERBATIM)
  ELSE (WIN32)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE}
                       ARGS ${moc_flags} ${moc_options} -o ${outfile} ${infile}
                       DEPENDS ${infile})
  ENDIF (WIN32)
ENDMACRO (QT4_CREATE_MOC_COMMAND)


MACRO (QT4_GENERATE_MOC infile outfile )
# get include dirs and flags
   QT4_GET_MOC_FLAGS(moc_flags)
   GET_FILENAME_COMPONENT(abs_infile ${infile} ABSOLUTE)
   QT4_CREATE_MOC_COMMAND(${abs_infile} ${outfile} "${moc_flags}" "")
   SET_SOURCE_FILES_PROPERTIES(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
ENDMACRO (QT4_GENERATE_MOC)


# QT4_WRAP_CPP(outfiles inputfile ... )

MACRO (QT4_WRAP_CPP outfiles )
  # get include dirs
  QT4_GET_MOC_FLAGS(moc_flags)
  QT4_EXTRACT_OPTIONS(moc_files moc_options ${ARGN})

  FOREACH (it ${moc_files})
    GET_FILENAME_COMPONENT(it ${it} ABSOLUTE)
    QT4_MAKE_OUTPUT_FILE(${it} moc_ cxx outfile)
    QT4_CREATE_MOC_COMMAND(${it} ${outfile} "${moc_flags}" "${moc_options}")
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH(it)

ENDMACRO (QT4_WRAP_CPP)


# QT4_WRAP_UI(outfiles inputfile ... )

MACRO (QT4_WRAP_UI outfiles )
  QT4_EXTRACT_OPTIONS(ui_files ui_options ${ARGN})

  FOREACH (it ${ui_files})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_UIC_EXECUTABLE}
      ARGS ${ui_options} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile})
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)

ENDMACRO (QT4_WRAP_UI)


# QT4_ADD_RESOURCES(outfiles inputfile ... )

MACRO (QT4_ADD_RESOURCES outfiles )
  QT4_EXTRACT_OPTIONS(rcc_files rcc_options ${ARGN})

  FOREACH (it ${rcc_files})
    GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cxx)
    #  parse file for dependencies 
    #  all files are absolute paths or relative to the location of the qrc file
    FILE(READ "${infile}" _RC_FILE_CONTENTS)
    STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
    SET(_RC_DEPENDS)
    FOREACH(_RC_FILE ${_RC_FILES})
      STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
      STRING(REGEX MATCH "^/|([A-Za-z]:/)" _ABS_PATH_INDICATOR "${_RC_FILE}")
      IF(NOT _ABS_PATH_INDICATOR)
        SET(_RC_FILE "${rc_path}/${_RC_FILE}")
      ENDIF(NOT _ABS_PATH_INDICATOR)
      SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
    ENDFOREACH(_RC_FILE)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_RCC_EXECUTABLE}
      ARGS ${rcc_options} -name ${outfilename} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      DEPENDS ${_RC_DEPENDS})
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)

ENDMACRO (QT4_ADD_RESOURCES)


MACRO(QT4_ADD_DBUS_INTERFACE _sources _interface _basename)
  GET_FILENAME_COMPONENT(_infile ${_interface} ABSOLUTE)
  SET(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
  SET(_impl   ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp)
  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc)

  # handling more arguments (as in FindQt4.cmake from KDE4) will come soon, then
  # _params will be used for more than just -m
  SET(_params -m)

  ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
      COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} ${_params} -p ${_basename} ${_infile}
      DEPENDS ${_infile})

  SET_SOURCE_FILES_PROPERTIES(${_impl} PROPERTIES SKIP_AUTOMOC TRUE)

  QT4_GENERATE_MOC(${_header} ${_moc})

  SET(${_sources} ${${_sources}} ${_impl} ${_header} ${_moc})
  MACRO_ADD_FILE_DEPENDENCIES(${_impl} ${_moc})

ENDMACRO(QT4_ADD_DBUS_INTERFACE)


MACRO(QT4_ADD_DBUS_INTERFACES _sources)
  FOREACH (_current_FILE ${ARGN})
    GET_FILENAME_COMPONENT(_infile ${_current_FILE} ABSOLUTE)
    # get the part before the ".xml" suffix
    STRING(REGEX REPLACE "(.*[/\\.])?([^\\.]+)\\.xml" "\\2" _basename ${_current_FILE})
    STRING(TOLOWER ${_basename} _basename)
    QT4_ADD_DBUS_INTERFACE(${_sources} ${_infile} ${_basename}interface)
  ENDFOREACH (_current_FILE)
ENDMACRO(QT4_ADD_DBUS_INTERFACES)


MACRO(QT4_GENERATE_DBUS_INTERFACE _header) # _customName OPTIONS -some -options )
  QT4_EXTRACT_OPTIONS(_customName _qt4_dbus_options ${ARGN})

  GET_FILENAME_COMPONENT(_in_file ${_header} ABSOLUTE)
  GET_FILENAME_COMPONENT(_basename ${_header} NAME_WE)

  IF (_customName)
    SET(_target ${CMAKE_CURRENT_BINARY_DIR}/${_customName})
  ELSE (_customName)
    SET(_target ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.xml)
  ENDIF (_customName)

  ADD_CUSTOM_COMMAND(OUTPUT ${_target}
      COMMAND ${QT_DBUSCPP2XML_EXECUTABLE} ${_qt4_dbus_options} ${_in_file} -o ${_target}
      DEPENDS ${_in_file}
  )
ENDMACRO(QT4_GENERATE_DBUS_INTERFACE)


MACRO(QT4_ADD_DBUS_ADAPTOR _sources _xml_file _include _parentClass) # _optionalBasename _optionalClassName)
  GET_FILENAME_COMPONENT(_infile ${_xml_file} ABSOLUTE)

  SET(_optionalBasename "${ARGV4}")
  IF (_optionalBasename)
    SET(_basename ${_optionalBasename} )
  ELSE (_optionalBasename)
    STRING(REGEX REPLACE "(.*[/\\.])?([^\\.]+)\\.xml" "\\2adaptor" _basename ${_infile})
    STRING(TOLOWER ${_basename} _basename)
  ENDIF (_optionalBasename)

  SET(_optionalClassName "${ARGV5}")
  SET(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
  SET(_impl   ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp)
  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc)

  IF(_optionalClassName)
    ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
       COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} -m -a ${_basename} -c ${_optionalClassName} -i ${_include} -l ${_parentClass} ${_infile}
       DEPENDS ${_infile}
    )
  ELSE(_optionalClassName)
    ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
       COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} -m -a ${_basename} -i ${_include} -l ${_parentClass} ${_infile}
       DEPENDS ${_infile}
     )
  ENDIF(_optionalClassName)

  QT4_GENERATE_MOC(${_header} ${_moc})
  SET_SOURCE_FILES_PROPERTIES(${_impl} PROPERTIES SKIP_AUTOMOC TRUE)
  MACRO_ADD_FILE_DEPENDENCIES(${_impl} ${_moc})

  SET(${_sources} ${${_sources}} ${_impl} ${_header} ${_moc})
ENDMACRO(QT4_ADD_DBUS_ADAPTOR)


MACRO(QT4_AUTOMOC)
  QT4_GET_MOC_FLAGS(_moc_INCS)

  SET(_matching_FILES )
  FOREACH (_current_FILE ${ARGN})

    GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
    # if "SKIP_AUTOMOC" is set to true, we will not handle this file here.
    # This is required to make uic work correctly:
    # we need to add generated .cpp files to the sources (to compile them),
    # but we cannot let automoc handle them, as the .cpp files don't exist yet when
    # cmake is run for the very first time on them -> however the .cpp files might
    # exist at a later run. at that time we need to skip them, so that we don't add two
    # different rules for the same moc file
    GET_SOURCE_FILE_PROPERTY(_skip ${_abs_FILE} SKIP_AUTOMOC)

    IF ( NOT _skip AND EXISTS ${_abs_FILE} )

      FILE(READ ${_abs_FILE} _contents)

      GET_FILENAME_COMPONENT(_abs_PATH ${_abs_FILE} PATH)

      STRING(REGEX MATCHALL "# *include +[^ ]+\\.moc[\">]" _match "${_contents}")
      IF(_match)
        FOREACH (_current_MOC_INC ${_match})
          STRING(REGEX MATCH "[^ <\"]+\\.moc" _current_MOC "${_current_MOC_INC}")

          GET_FILENAME_COMPONENT(_basename ${_current_MOC} NAME_WE)
          IF(EXISTS ${_abs_PATH}/${_basename}.hpp)
            SET(_header ${_abs_PATH}/${_basename}.hpp)
          ELSE(EXISTS ${_abs_PATH}/${_basename}.hpp)
            SET(_header ${_abs_PATH}/${_basename}.h)
          ENDIF(EXISTS ${_abs_PATH}/${_basename}.hpp)
          SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_current_MOC})
          QT4_CREATE_MOC_COMMAND(${_header} ${_moc} "${_moc_INCS}" "")
          MACRO_ADD_FILE_DEPENDENCIES(${_abs_FILE} ${_moc})
        ENDFOREACH (_current_MOC_INC)
      ENDIF(_match)
    ENDIF ( NOT _skip AND EXISTS ${_abs_FILE} )
  ENDFOREACH (_current_FILE)
ENDMACRO(QT4_AUTOMOC)


MACRO(QT4_CREATE_TRANSLATION _qm_files)
   QT4_EXTRACT_OPTIONS(_lupdate_files _lupdate_options ${ARGN})
   SET(_my_sources)
   SET(_my_dirs)
   SET(_my_tsfiles)
   SET(_ts_pro)
   FOREACH (_file ${_lupdate_files})
     GET_FILENAME_COMPONENT(_ext ${_file} EXT)
     GET_FILENAME_COMPONENT(_abs_FILE ${_file} ABSOLUTE)
     IF(_ext MATCHES "ts")
       LIST(APPEND _my_tsfiles ${_abs_FILE})
     ELSE(_ext MATCHES "ts")
       IF(NOT _ext)
         LIST(APPEND _my_dirs ${_abs_FILE})
       ELSE(NOT _ext)
         LIST(APPEND _my_sources ${_abs_FILE})
       ENDIF(NOT _ext)
     ENDIF(_ext MATCHES "ts")
   ENDFOREACH(_file)
   FOREACH(_ts_file ${_my_tsfiles})
     IF(_my_sources)
       # make a .pro file to call lupdate on, so we don't make our commands too
       # long for some systems
       GET_FILENAME_COMPONENT(_ts_name ${_ts_file} NAME_WE)
       SET(_ts_pro ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_ts_name}_lupdate.pro)
       SET(_pro_srcs)
       FOREACH(_pro_src ${_my_sources})
         SET(_pro_srcs "${_pro_srcs} \"${_pro_src}\"")
       ENDFOREACH(_pro_src ${_my_sources})
       FILE(WRITE ${_ts_pro} "SOURCES = ${_pro_srcs}")
     ENDIF(_my_sources)
     ADD_CUSTOM_COMMAND(OUTPUT ${_ts_file}
        COMMAND ${QT_LUPDATE_EXECUTABLE}
        ARGS ${_lupdate_options} ${_ts_pro} ${_my_dirs} -ts ${_ts_file}
        DEPENDS ${_my_sources} ${_ts_pro})
   ENDFOREACH(_ts_file)
   QT4_ADD_TRANSLATION(${_qm_files} ${_my_tsfiles})
ENDMACRO(QT4_CREATE_TRANSLATION)


MACRO(QT4_ADD_TRANSLATION _qm_files)
  FOREACH (_current_FILE ${ARGN})
    GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
    GET_FILENAME_COMPONENT(qm ${_abs_FILE} NAME_WE)
    GET_SOURCE_FILE_PROPERTY(output_location ${_abs_FILE} OUTPUT_LOCATION)
    IF(output_location)
      FILE(MAKE_DIRECTORY "${output_location}")
      SET(qm "${output_location}/${qm}.qm")
    ELSE(output_location)
      SET(qm "${CMAKE_CURRENT_BINARY_DIR}/${qm}.qm")
    ENDIF(output_location)

    ADD_CUSTOM_COMMAND(OUTPUT ${qm}
       COMMAND ${QT_LRELEASE_EXECUTABLE}
       ARGS ${_abs_FILE} -qm ${qm}
       DEPENDS ${_abs_FILE}
    )
    SET(${_qm_files} ${${_qm_files}} ${qm})
  ENDFOREACH (_current_FILE)
ENDMACRO(QT4_ADD_TRANSLATION)
