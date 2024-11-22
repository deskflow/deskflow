# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-License-Identifier: MIT

# Copy License with txt ext for picky package creation tools
file(COPY_FILE
  ${CMAKE_SOURCE_DIR}/LICENSE
  ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt
  ONLY_IF_DIFFERENT
)

# Handle Pre install Items
if(WIN32)

  # Configure the windows version rc file
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/version.rc.in
    ${PROJECT_BINARY_DIR}/src/version.rc @ONLY
  )

elseif(NOT APPLE)

  # Install our desktop file
  install(
    FILES ${CMAKE_CURRENT_LIST_DIR}/org.deskflow.deskflow.desktop
    DESTINATION share/applications
  )

  # Install our icon
  install(
    FILES ${CMAKE_CURRENT_LIST_DIR}/deskflow.png
    DESTINATION share/icons/hicolor/512x512/apps/
    RENAME org.deskflow.deskflow.png
  )

  # Install our metainfo
  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.deskflow.deskflow.metainfo.xml
    DESTINATION share/metainfo/
  )

  # Prepare PKGBUILD for Arch Linux
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/dist/arch/PKGBUILD.in
    ${CMAKE_BINARY_DIR}/PKGBUILD
    @ONLY
  )
endif()

if(WIN32)
  set(CPACK_PACKAGE_NAME "Deskflow")
else()
  set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
endif()

# Generic Package Items
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_CONTACT "Deskflow <maintainers@deskflow.org>")
set(CPACK_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Deskflow")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt)

set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

#Prevent this override from being written in the package
if(NOT PACKAGE_VERSION_LABEL)
    set (PACKAGE_VERSION_LABEL "${CPACK_PACKAGE_VERSION}")
endif()

set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

  set(CPACK_WIX_UPGRADE_GUID "027D1C8A-E7A5-4754-BB93-B2D45BFDBDC8")

  set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/wix-banner.png")
  set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/wix-dialog.png")
  set(CPACK_WIX_MSM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Microsoft_VC142_CRT_x64.msm")


  # Required Extra Extenstions
  list(APPEND CPACK_WIX_EXTENSIONS "WixToolset.Util.wixext" "WixToolset.Firewall.wixext")

  # Make sure to also put the xmlns for the ext into the wix block on generated files
  list(APPEND CPACK_WIX_CUSTOM_XMLNS "util=http://wixtoolset.org/schemas/v4/wxs/util" "firewall=http://wixtoolset.org/schemas/v4/wxs/firewall")

  # Inject Service and Firewall changes into geneated Wix File.
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wix-patch.xml.in "${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml" @ONLY)
  set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml")

  # Menu Entry
  set(CPACK_WIX_PROGRAM_MENU_FOLDER "Deskflow")
  set(CPACK_PACKAGE_EXECUTABLES "deskflow" "Deskflow")

  # Default Install Path
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "Deskflow")

  # If Wix4+ is installed make a package
  find_program(WIX_APP wix)
   if (NOT "${WIX_APP}" STREQUAL "")
     set(CPACK_WIX_VERSION 4)
     list(APPEND CPACK_GENERATOR "WIX")
   endif()

  if(CMAKE_SYSTEM_PROCESSOR MATCHES AMD64)
    set(OS_STRING "win-x64")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ARM64)
    set(OS_STRING "win-arm64")
  else()
    set(OS_STRING "win-${CMAKE_SYSTEM_PROCESSOR}")
  endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(OS_STRING "macos-${CMAKE_SYSTEM_PROCESSOR}")
  set(CMAKE_INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks")
  set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/deploy/dmg-volume.icns")
  set(CPACK_DMG_BACKGROUND_IMAGE "${PROJECT_SOURCE_DIR}/deploy/dmg-background.tiff")
  set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${PROJECT_SOURCE_DIR}/deploy/generate_ds_store.applescript")
  set(CPACK_DMG_VOLUME_NAME "Deskflow")
  set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE ON)
  set(CPACK_GENERATOR "DragNDrop")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # Gather distro info
  # This is used in package names
  configure_linux_package_name()
  # HACK: The GUI depends on the Qt6 QPA plugins package, but that's not picked
  # up by shlibdeps on Ubuntu 22 (though not a problem on Ubuntu 24 and Debian
  # 12), so we must add it manually.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "|.*BSD")
  message(STATUS "BSD packaging not yet supported")
  set(OS_STRING ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
endif()

# Always use "deskflow" for start of name
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION_LABEL}-${OS_STRING}")
message(STATUS "Package Basename: ${CPACK_PACKAGE_FILE_NAME}")

include(CPack)
