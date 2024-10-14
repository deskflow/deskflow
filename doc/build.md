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
| DOCS                     | Build Documentation                     | ON                 | `doxygen` |
| BUILD_GUI                | Build GUI                               | ON                 | |
| BUILD_INSTALLER          | Build installers/packages               | ON                 | |
| BUILD_TESTS              | Build unit tests and integration tests  | ON                 | |
| BUILD_UNIFIED            | Build unified binary (client+server)    | OFF                | |
| ENABLE_COVERAGE          | Enable test coverage                    | OFF                | `gcov` |
| SYSTEM_LIBEI             | Use system libei (use local dep)        | ON                 | |
| SYSTEM_LIBPORTAL         | Use system libportal (or local dep)     | ON                 | |
| SYSTEM_GTEST             | Use system GoogleTest (or local dep)    | ON                 | |
| SYSTEM_TOMLPLUSPLUS      | Use system tomlplusplus (or local dep)  | ON                 | |
| SYSTEM_CLI11             | Use system CLI11                        | ON                 | |
| LIBPORTAL_STATIC         | Use static libportal (hacky)            | OFF                | `subprojects/packagefiles/libportal/static-lib.diff` |

Example cmake configuration.
`cmake -S. -Bbuild -DSYSTEM_LIBEI=OFF`

## Developer Quick Start

Deskflow is free and open source software, and anyone is welcome to build it,
run it, tinker with it, redistribute it as part of their own app, etc.

These instructions will build Deskflow, which doesn't require a license
or serial key. Check the [Build Guide](https://github.com/deskflow/deskflow/wiki/Build-Guide)
wiki page if you have problems.

**1. Dependencies:**

You can either copy/paste the commands for your OS from [`config.yaml`](../config.yaml) or use the deps script.

*Linux, macOS, or BSD-derived:*
```
./scripts/install_deps.sh
```

*Windows:*
```
python scripts/install_deps.py
```

**2. Configure:**

*Linux, macOS, or BSD-derived:*
```
cmake -B build
```

*Windows:*
```
cmake -B build --preset=windows-release
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
