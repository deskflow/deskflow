# Synergy Core

[![CI](https://github.com/symless/synergy-core/actions/workflows/ci.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/ci.yml)
[![CodeQL Analysis](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml)
[![Build containers](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=symless_synergy-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=symless_synergy-core)

The Synergy Core project is the open-source core component of [Synergy](https://symless.com/synergy), a keyboard and mouse sharing tool. 

This project is intended for developers, and when built produces the community edition of Synergy. To use it, install `synergy` with your favorite package manager.

* [Download Synergy](https://symless.com/synergy/download)
* [Contact support](https://symless.com/synergy/contact)
* [Help articles](https://symless.com/synergy/help)
* [Project Wiki](https://github.com/symless/synergy-core/wiki)

## Developer quick start

Simplified instructions for those who want to contribute to the development of Synergy Core.

Having problems? Check the [Quick Start FAQ](https://github.com/symless/synergy-core/wiki/Quick-Start-FAQ) wiki page.

**Dependencies:**
```
python scripts/install_deps.py
```

**Configure:**

*Windows:*
```
cmake -B build --preset=windows-release
```

*macOS:*
```
cmake -B build --preset=macos-release
```

*Linux:*
```
cmake -B build --preset=linux-release
```

**Build:**
```
cmake --build build -j8
```

**Test:**
```
./build/bin/unittests
```
