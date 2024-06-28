# Synergy -- mouse and keyboard sharing utility Copyright (C) 2012-2024 Symless
# Ltd. Copyright (C) 2009-2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License found in the file LICENSE that
# should have accompanied this file.
#
# This package is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

#
# If enabled, configure packaging based on OS.
#
macro(configure_packaging)

  if(${SYNERGY_BUILD_INSTALLER})
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
      configure_windows_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      configure_macos_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux|.*BSD|DragonFly")
      configure_linux_packaging()
    endif()
  else()
    message(STATUS "Not configuring installer")
  endif()
endmacro()

#
# Windows installer
#
macro(configure_windows_packaging)
  find_openssl_dir_win32(OPENSSL_PATH)
  set(QT_PATH $ENV{CMAKE_PREFIX_PATH})
  configure_files(${CMAKE_CURRENT_SOURCE_DIR}/res/dist/wix
                  ${CMAKE_BINARY_DIR}/installer)
endmacro()

#
# macOS app bundle
#
macro(configure_macos_packaging)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_INSTALL_RPATH
        "@loader_path/../Libraries;@loader_path/../Frameworks")
    set(SYNERGY_BUNDLE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/res/dist/macos/bundle)
    set(SYNERGY_BUNDLE_DIR ${CMAKE_BINARY_DIR}/bundle)
    set(SYNERGY_BUNDLE_APP_DIR ${SYNERGY_BUNDLE_DIR}/Synergy.app)
    set(SYNERGY_BUNDLE_BINARY_DIR ${SYNERGY_BUNDLE_APP_DIR}/Contents/MacOS)
    configure_files(${SYNERGY_BUNDLE_SOURCE_DIR} ${SYNERGY_BUNDLE_DIR})
  endif()
endmacro()

#
# Linux packages (including BSD and DragonFly)
#
macro(configure_linux_packaging)
  configure_files(${CMAKE_CURRENT_SOURCE_DIR}/res/dist/rpm
                  ${CMAKE_BINARY_DIR}/rpm)
  install(FILES res/synergy.svg DESTINATION share/icons/hicolor/scalable/apps)
  install(FILES res/synergy.desktop DESTINATION share/applications)
endmacro()

#
# Same as the `configure_file` command but for directories recursively.
#
macro(configure_files srcDir destDir)
  message(STATUS "Configuring directory ${destDir}")
  make_directory(${destDir})

  file(
    GLOB_RECURSE sourceFiles
    RELATIVE ${srcDir}
    ${srcDir}/*)
  file(
    GLOB_RECURSE templateFiles
    LIST_DIRECTORIES false
    RELATIVE ${srcDir}
    ${srcDir}/*.in)
  list(REMOVE_ITEM sourceFiles ${templateFiles})

  foreach(sourceFile ${sourceFiles})
    set(sourceFilePath ${srcDir}/${sourceFile})
    if(IS_DIRECTORY ${sourceFilePath})
      message(STATUS "Copying directory ${sourceFile}")
      make_directory(${destDir}/${sourceFile})
    else()
      message(STATUS "Copying file ${sourceFile}")
      configure_file(${sourceFilePath} ${destDir}/${sourceFile} COPYONLY)
    endif()
  endforeach(sourceFile)

  foreach(templateFile ${templateFiles})
    set(sourceTemplateFilePath ${srcDir}/${templateFile})
    string(REGEX REPLACE "\.in$" "" templateFile ${templateFile})
    message(STATUS "Configuring file ${templateFile}")
    configure_file(${sourceTemplateFilePath} ${destDir}/${templateFile} @ONLY)
  endforeach(templateFile)
endmacro(configure_files)

function(find_openssl_dir_win32 result)
  execute_process(
    COMMAND where openssl
    OUTPUT_VARIABLE OPENSSL_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # It's possible that there are multiple OpenSSL installations on the system,
  # which is the case on GitHub runners. For now we'll pick the first one, but
  # that's probably not very robust. Maybe our choco config could install to a
  # specific location?
  string(REGEX REPLACE "\r?\n" ";" OPENSSL_PATH_LIST ${OPENSSL_PATH})
  message(STATUS "Found OpenSSL binaries at: ${OPENSSL_PATH_LIST}")

  list(GET OPENSSL_PATH_LIST 0 OPENSSL_FIRST_PATH)
  message(STATUS "First OpenSSL binary: ${OPENSSL_FIRST_PATH}")

  get_filename_component(OPENSSL_BIN_DIR ${OPENSSL_FIRST_PATH} DIRECTORY)
  message(STATUS "OpenSSL bin dir: ${OPENSSL_BIN_DIR}")

  get_filename_component(OPENSSL_DIR ${OPENSSL_BIN_DIR} DIRECTORY)
  message(STATUS "OpenSSL install root dir: ${OPENSSL_DIR}")

  set(${result}
      ${OPENSSL_DIR}
      PARENT_SCOPE)
endfunction()
