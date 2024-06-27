# Synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012-2024 Symless Ltd.
# Copyright (C) 2009-2012 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#
# If enabled, configure packaging based on OS.
#
macro(configure_packaging)
    if (${SYNERGY_BUILD_INSTALLER})
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            configure_macos_packaging()
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            configure_windows_packaging()
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux|.*BSD|DragonFly")
            configure_linux_packaging()
        endif()
    else()
        message (STATUS "Not configuring installer")
    endif()
endmacro()

#
# macOS app bundle
#
macro(configure_macos_packaging)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (CMAKE_INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks")
    set (SYNERGY_BUNDLE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res/dist/macos/bundle)
    set (SYNERGY_BUNDLE_DIR ${CMAKE_BINARY_DIR}/bundle)
    set (SYNERGY_BUNDLE_APP_DIR ${SYNERGY_BUNDLE_DIR}/Synergy.app)
    set (SYNERGY_BUNDLE_BINARY_DIR ${SYNERGY_BUNDLE_APP_DIR}/Contents/MacOS)
    configure_files (${SYNERGY_BUNDLE_SOURCE_DIR} ${SYNERGY_BUNDLE_DIR})
    endif()
endmacro()

#
# Windows installer
#
macro(configure_windows_packaging)
    # Takes the `OPENSSL_CONF` env var which has the full path to `openssl.config` and find the
    # root path, e.g.:
    # OPENSSL_CONF: C:\Program Files\OpenSSL-Win64\bin\openssl.config
    # OPENSSL_PATH: C:\Program Files\OpenSSL-Win64

    string(REGEX MATCH "^(.*[/\\\\]OpenSSL-Win64)" OPENSSL_PATH "$ENV{OPENSSL_CONF}")
    message(STATUS "OpenSSL path: ${OPENSSL_PATH}")

    set(QT_PATH $ENV{CMAKE_PREFIX_PATH})
    configure_files (${CMAKE_CURRENT_SOURCE_DIR}/res/dist/wix ${CMAKE_BINARY_DIR}/installer)
endmacro()

#
# Linux packages (including BSD and DragonFly)
#
macro(configure_linux_packaging)
    configure_files (${CMAKE_CURRENT_SOURCE_DIR}/res/dist/rpm ${CMAKE_BINARY_DIR}/rpm)
    install(FILES res/synergy.svg DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES res/synergy.desktop DESTINATION share/applications)
endmacro()

#
# Same as the `configure_file` command but for directories recursively.
#
macro (configure_files srcDir destDir)
    message (STATUS "Configuring directory ${destDir}")
    make_directory (${destDir})

    file (GLOB_RECURSE sourceFiles RELATIVE ${srcDir} ${srcDir}/*)
    file (GLOB_RECURSE templateFiles LIST_DIRECTORIES false RELATIVE ${srcDir} ${srcDir}/*.in)
    list (REMOVE_ITEM sourceFiles ${templateFiles})

    foreach (sourceFile ${sourceFiles})
        set (sourceFilePath ${srcDir}/${sourceFile})
        if (IS_DIRECTORY ${sourceFilePath})
            message (STATUS "Copying directory ${sourceFile}")
            make_directory (${destDir}/${sourceFile})
        else()
            message (STATUS "Copying file ${sourceFile}")
            configure_file (${sourceFilePath} ${destDir}/${sourceFile} COPYONLY)
        endif()
    endforeach (sourceFile)

    foreach (templateFile ${templateFiles})
        set (sourceTemplateFilePath ${srcDir}/${templateFile})
                string (REGEX REPLACE "\.in$" "" templateFile ${templateFile})
        message (STATUS "Configuring file ${templateFile}")
        configure_file (${sourceTemplateFilePath} ${destDir}/${templateFile} @ONLY)
    endforeach (templateFile)
endmacro (configure_files)
