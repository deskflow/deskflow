# Building Deskflow

To build Deskflow you will a minimum of: 
    - [cmake] 3.24+
    - [Qt] 6.7.0+
    - [openssl] 3.0+
    - [libportal] 0.8+ (linux, bsd)
    - [libei] 1.3+ (linux, bsd)
    - [google_test] ^
    - [tomlplusplus] ^
    - [cli11] ^

> ^ Will be fetched if not found on the host system.

By default a build of Deskflow will: 
     - The GUI application deskflow
     - The Client application deskflow-client
     - The Server application deskflow-server
     - Documentation if [doxygen] was found on your system
     - Tests that will be run as part of the build process.

## Configuration
Deskflow supports the following build options

CMake options:
|         Option           |            Description                  |   Default Value    | Additional requirements |
:-------------------------:|:---------------------------------------:|:------------------:|:-----------------------:|
| BUILD_GUI                | Build GUI                               | ON                 | |
| BUILD_USER_DOCS          | Build user documentation                | DOXYGEN_FOUND      | `Doxygen` |
| BUILD_DEV_DOCS           | Build development documentation         | OFF                | `Doxygen` |
| BUILD_INSTALLER          | Build installers/packages               | ON                 | |
| BUILD_TESTS              | Build unit tests and legacy tests       | ON                 | `gtest`|
| BUILD_UNIFIED            | Build unified binary (client+server)    | OFF                | |
| ENABLE_COVERAGE          | Enable test coverage                    | OFF                | `gcov` |
| SKIP_BUILD_TESTS         | Skip running of tests at build time     | OFF                | |

Example cmake configuration.
`cmake -S. -Bbuild -DCMAKE_INSTALL_PREFIX=<INSTALLPREFIX>`

### Windows Configuration
 It is recommended to use vcpkg to get the dependencies to install. The first configuration will build all depends while configuing the project. If you do not want to instal qt via vcpkg you should remove the qt packages from vcpkg.json in the of the project BEFORE attempting to configure the project. 
 
## Build
After Configuring you should be able to run make to build all targets.

`cmake --build build`

## Install
 To test installation run `DESTDIR=<installDIR> cmake --install build` to install into `<installDir>/<CMAKE_INSTALL_PREFIX>` <br>
 Running `cmake --install build` will install to the `CMAKE_INSTALL_PREFIX`

## Making Deskflow packages
 Deskflow can generate several packages using cpack.
 To generate packages build the `package` or `package_source` target.
 Example: ` cmake --build build --target package package_source` would generate both package and package source packages.
 Deskflow can generate several package types depending on the system. Archive-based packages should work on all platforms. On Linux deb and rpm info is set up, flatpaks can be generated from the included file in deploy/linux and a PKGBUILD for Arch linux is generated in the build folder. On macos a dmg file will be created and signed. For windows wix can be used to create an installer.

 
[Qt]:https://www.qt.io
[doxygen]:http://www.stack.nl/~dimitri/doxygen/
[cmake]:https://cmake.org/
[openssl]:https://www.openssl.org/
[google_test]:https://github.com/google/googletest
[tomlplusplus]:https://github.com/marzer/tomlplusplus
[cli11]:https://github.com/CLIUtils/CLI11
[libei]:https://gitlab.freedesktop.org/libinput/libei
[libportal]:https://github.com/flatpak/libportal
