# Synergy Core

[![Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=symless_synergy-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=symless_synergy-core)
[![CI](https://github.com/symless/synergy-core/actions/workflows/ci.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/ci.yml)
[![CodeQL Analysis](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml)
[![Build containers](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml)

The Synergy Core project is the open-source core component of [Synergy](https://symless.com/synergy), a keyboard and mouse sharing tool.

Join us on [Discord](https://discord.com/invite/xBFv6j7) or [Slack](https://synergy-app.slack.com/join/shared_invite/zt-d8if26fr-6x~TSTz4skGmTnFP5IPaww#/shared-invite/email) in the `#open-source` channel.

This project is intended for advanced technical users, and when built produces Synergy 1 Community Edition. 
To use the community edition, either install the `synergy` package with your favorite package manager or build it yourself using the Developer Quick Start instructions below.

* [Download Synergy](https://symless.com/synergy/download)
* [Contact support](https://symless.com/synergy/contact)
* [Help articles](https://symless.com/synergy/help)
* [Project Wiki](https://github.com/symless/synergy-core/wiki)

## Developer Quick Start

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

## Packages

To get Synergy 1, use your favorite package manager to install `synergy` (this repo).

Synergy 2 is no longer in development and should not be provided.

For both Synergy 1 and Synergy 3 you can use the [official packages](https://symless.com/synergy/download).

[![Repology](https://repology.org/badge/vertical-allrepos/synergy.svg?exclude_unsupported=1)](https://repology.org/project/synergy/versions)
