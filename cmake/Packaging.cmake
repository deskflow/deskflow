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

  if(${BUILD_INSTALLER})
    set(CPACK_PACKAGE_NAME "synergy")
    set(CPACK_PACKAGE_CONTACT "Synergy <support@symless.com>")
    set(CPACK_PACKAGE_DESCRIPTION "Mouse and keyboard sharing utility")
    set(CPACK_PACKAGE_VENDOR "Symless")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
      configure_windows_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      configure_macos_packaging()
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux|.*BSD|DragonFly")
      configure_linux_packaging()
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

  message(STATUS "Configuring Windows installer")

  set(CPACK_PACKAGE_VERSION ${SYNERGY_VERSION_MS})
  set(QT_PATH $ENV{CMAKE_PREFIX_PATH})

  configure_files(${CMAKE_CURRENT_SOURCE_DIR}/res/dist/wix
                  ${CMAKE_BINARY_DIR}/installer)

endmacro()

#
# macOS app bundle
#
macro(configure_macos_packaging)

  message(STATUS "Configuring macOS app bundle")

  set(CPACK_PACKAGE_VERSION ${SYNERGY_VERSION})

  set(CMAKE_INSTALL_RPATH
      "@loader_path/../Libraries;@loader_path/../Frameworks")
  set(SYNERGY_BUNDLE_SOURCE_DIR
      ${CMAKE_CURRENT_SOURCE_DIR}/res/dist/macos/bundle)
  set(SYNERGY_BUNDLE_DIR ${CMAKE_BINARY_DIR}/bundle)
  set(SYNERGY_BUNDLE_APP_DIR ${SYNERGY_BUNDLE_DIR}/Synergy.app)
  set(SYNERGY_BUNDLE_BINARY_DIR ${SYNERGY_BUNDLE_APP_DIR}/Contents/MacOS)

  configure_files(${SYNERGY_BUNDLE_SOURCE_DIR} ${SYNERGY_BUNDLE_DIR})

endmacro()

#
# Linux packages (including BSD and DragonFly)
#
macro(configure_linux_packaging)

  message(STATUS "Configuring Linux packaging")

  set(CPACK_PACKAGE_VERSION ${SYNERGY_VERSION_LINUX})
  set(CPACK_GENERATOR "DEB;RPM;TGZ")

  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Synergy <developers@symless.com>")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  set(CPACK_RPM_PACKAGE_LICENSE "GPLv2")
  set(CPACK_RPM_PACKAGE_GROUP "Applications/System")

  configure_libei_package_dep()

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

  install(FILES res/dist/linux/synergy.desktop DESTINATION share/applications)
  install(FILES res/synergy.png DESTINATION share/pixmaps)

  # Prepare PKGBUILD for Arch Linux
  configure_file(res/dist/arch/PKGBUILD.in ${CMAKE_BINARY_DIR}/PKGBUILD @ONLY)

endmacro()

macro(configure_libei_package_dep)

  # By default, bundle libei if the project was fetched, otherwise system libei is used.
  # The project is only fetched and built if Meson wasn't able to find it on the system.
  file(GLOB libei_project_exists ${CMAKE_SOURCE_DIR}/subprojects/libei)
  if(libei_project_exists)
    set(DEFAULT_BUNDLE_LIBEI ON)
  else()
    set(DEFAULT_BUNDLE_LIBEI OFF)
  endif()

  option(BUNDLE_LIBEI "Bundle libei" ${DEFAULT_BUNDLE_LIBEI})
  if(BUNDLE_LIBEI)
    find_library(libei_lib_file NAMES ei)
    get_filename_component(libei_lib_file_real ${libei_lib_file} REALPATH)
    message(STATUS "Package will bundle libei: ${libei_lib_file_real}")
    install(
      FILES ${libei_lib_file_real}
      DESTINATION lib
      COMPONENT libraries)

    # Disable RPM dependency on libei, as it's bundled.
    set(CPACK_RPM_SPEC_MORE_DEFINE "%define __requires_exclude libei")
  endif()

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
