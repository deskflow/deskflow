# Deskflow -- mouse and keyboard sharing utility
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

  set(DESKFLOW_PROJECT_RES_DIR ${PROJECT_SOURCE_DIR}/res)

  if(${BUILD_INSTALLER})
    set(CPACK_PACKAGE_NAME ${DESKFLOW_APP_ID})
    set(CPACK_PACKAGE_CONTACT ${DESKFLOW_MAINTAINER})
    set(CPACK_PACKAGE_DESCRIPTION "Mouse and keyboard sharing utility")
    set(CPACK_PACKAGE_VENDOR ${DESKFLOW_AUTHOR_NAME})
    set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)

    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
      configure_windows_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      configure_mac_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
      configure_linux_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "|.*BSD")
      message(STATUS "BSD packaging not yet supported")
    endif()

    include(CPack)
  else()
    message(STATUS "Not configuring installer")
  endif()

endmacro()

#
# Windows installer
#
macro(configure_windows_packaging)

  message(VERBOSE "Configuring Windows installer")

  set(CPACK_PACKAGE_VERSION ${DESKFLOW_VERSION_MS})
  set(QT_PATH $ENV{CMAKE_PREFIX_PATH})

  set(DESKFLOW_MSI_64_GUID
      "027D1C8A-E7A5-4754-BB93-B2D45BFDBDC8"
      CACHE STRING "GUID for 64-bit MSI installer")

  set(DESKFLOW_MSI_32_GUID
      "8F57C657-BC87-45E6-840E-41242A93511C"
      CACHE STRING "GUID for 32-bit MSI installer")

  configure_files(${PROJECT_SOURCE_DIR}/res/dist/wix
                  ${PROJECT_BINARY_DIR}/installer)

endmacro()

#
# macOS app bundle
#
macro(configure_mac_packaging)

  message(VERBOSE "Configuring macOS app bundle")

  set(CPACK_PACKAGE_VERSION ${DESKFLOW_VERSION})

  set(CMAKE_INSTALL_RPATH
      "@loader_path/../Libraries;@loader_path/../Frameworks")
  set(DESKFLOW_BUNDLE_SOURCE_DIR
      ${PROJECT_SOURCE_DIR}/res/dist/mac/bundle
      CACHE PATH "Path to the macOS app bundle")
  set(DESKFLOW_BUNDLE_DIR ${PROJECT_BINARY_DIR}/bundle/${DESKFLOW_APP_NAME}.app)
  set(DESKFLOW_BUNDLE_BINARY_DIR ${DESKFLOW_BUNDLE_DIR}/Contents/MacOS)

  configure_files(${DESKFLOW_BUNDLE_SOURCE_DIR} ${DESKFLOW_BUNDLE_DIR})

  file(RENAME ${DESKFLOW_BUNDLE_DIR}/Contents/Resources/App.icns
       ${DESKFLOW_BUNDLE_DIR}/Contents/Resources/${DESKFLOW_APP_NAME}.icns)

endmacro()

#
# Linux packages
#
macro(configure_linux_packaging)

  message(VERBOSE "Configuring Linux packaging")

  set(CPACK_PACKAGE_VERSION ${DESKFLOW_VERSION_LINUX})
  set(CPACK_GENERATOR "DEB;RPM;TGZ")

  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${DESKFLOW_MAINTAINER})
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  set(CPACK_RPM_PACKAGE_LICENSE "GPLv2")
  set(CPACK_RPM_PACKAGE_GROUP "Applications/System")

  # HACK: The GUI depends on the Qt6 QPA plugins package, but that's not picked
  # up by shlibdeps on Ubuntu 22 (though not a problem on Ubuntu 24 and Debian
  # 12), so we must add it manually.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins")

  # The default for CMake seems to be /usr/local, which seems uncommon. While
  # the default /usr/local prefix causes the app to appear on Debian and Fedora,
  # it doesn't seem to appear on Arch Linux. Setting the prefix to /usr seems to
  # work on a wider variety of distros, and that also seems to be where most
  # apps install to.
  set(CMAKE_INSTALL_PREFIX /usr)

  set(source_desktop_file ${DESKFLOW_PROJECT_RES_DIR}/dist/linux/app.desktop.in)
  set(configured_desktop_file ${PROJECT_BINARY_DIR}/app.desktop)
  set(install_desktop_file ${DESKFLOW_APP_ID}.desktop)

  configure_file(${source_desktop_file} ${configured_desktop_file} @ONLY)

  install(
    FILES ${configured_desktop_file}
    DESTINATION share/applications
    RENAME ${install_desktop_file})

  install(
    FILES ${DESKFLOW_RES_DIR}/app.png
    DESTINATION share/pixmaps
    RENAME ${DESKFLOW_APP_ID}.png)

  # Prepare PKGBUILD for Arch Linux
  configure_file(${DESKFLOW_PROJECT_RES_DIR}/dist/arch/PKGBUILD.in
                 ${CMAKE_BINARY_DIR}/PKGBUILD @ONLY)

endmacro()

#
# Same as the `configure_file` command but for directories recursively.
#
macro(configure_files srcDir destDir)

  message(VERBOSE "Configuring directory ${destDir}")
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
      message(VERBOSE "Copying directory ${sourceFile}")
      make_directory(${destDir}/${sourceFile})
    else()
      message(VERBOSE "Copying file ${sourceFile}")
      configure_file(${sourceFilePath} ${destDir}/${sourceFile} COPYONLY)
    endif()

  endforeach(sourceFile)

  foreach(templateFile ${templateFiles})

    set(sourceTemplateFilePath ${srcDir}/${templateFile})
    string(REGEX REPLACE "\.in$" "" templateFile ${templateFile})
    message(VERBOSE "Configuring file ${templateFile}")
    configure_file(${sourceTemplateFilePath} ${destDir}/${templateFile} @ONLY)

  endforeach(templateFile)

endmacro(configure_files)
