<!--
    SPDX-FileCopyrightText: 2024 Symless Ltd.
    SPDX-License-Identifier: CC0-1.0
-->

# Building Deskflow

## Configuration

CMake options:

|         Option           |            Description                  |   Default Value    | Additional requirements |
:-------------------------:|:---------------------------------------:|:------------------:|:-----------------------:|
| CMAKE_BUILD_TYPE         | Type of Build that is produced          | ReleaseWithDebInfo | |
| DOCS                     | Build Documentation                     | ON                 | [doxygen] |
| BUILD_GUI                | Build GUI                               | ON                 | `Qt`|
| BUILD_INSTALLER          | Build installers/packages               | ON                 | |
| BUILD_TESTS              | Build unit tests and integration tests  | ON                 | `gtest`|
| BUILD_UNIFIED            | Build unified binary (client+server)    | OFF                | |
| ENABLE_COVERAGE          | Enable test coverage                    | OFF                | `gcov` |
| SYSTEM_LIBEI             | Use system libei (use local dep)        | ON                 | |
| SYSTEM_LIBPORTAL         | Use system libportal (or local dep)     | ON                 | |
| LIBPORTAL_STATIC         | Use static libportal (hacky)            | OFF                | `subprojects/packagefiles/libportal/static-lib.diff` |

Example cmake configuration.
`cmake -S. -Bbuild -DSYSTEM_LIBEI=OFF`

### CMake preset examples

 You may set user cmake preset in your local copy of the repo see the [CMakeUserPresets.json] for an example. 


## Developer Quick Start

Deskflow is free and open source software, and anyone is welcome to build it,
run it, tinker with it, redistribute it as part of their own app, etc.

These instructions will build Deskflow, which doesn't require a license
or serial key. Check the [Build Guide](https://github.com/deskflow/deskflow/wiki/Build-Guide)
wiki page if you have problems.

**1. Dependencies:**

 git
 Qt 6.2
 cmake 3.24+
 openssl 3.0+
 
 gtest, tomlplusplus and cli11 will be fetched at build time if not installed on the system. 

 
*Linux, macOS, or BSD-derived:*
```
./scripts/install_deps.sh
```

*Windows:*
 


**2. Configure:**

*Linux, macOS, or BSD-derived:*
```
cmake -B build
```

*Windows:*
```
cmake -B build
```

**3. Build:**
```
cmake --build build -j8
```

**4. Test:**
```
./build/bin/unittests
./build/bin/integtests
```

**5. Run:**
```
./build/bin/deskflow
```



### VCPKG

  Deskflow is able to use vcpkg to manage the Dependencies. It is recommened for windows users.
  
  Follow the set up instructions provided by microsoft.
  
  https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell
   
 
  To use vcpkg make sure you include a toolchain option when you run the cmake configuration setp
 
  ```
    -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
  ```

  The first time you use vcpkg it will download and build all dependencies, this will take sometime.
  
  You should install vcpkg to C:\ or any drive root as on windows there is a limit to how long a path can be, having this setup will also prevent vcpkg to building all its cache inside the projects build dir.
  

  [doxygen]:https://doxygen.nl
