# Synergy Core

[![Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=symless_synergy-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=symless_synergy-core)
[![CI](https://github.com/symless/synergy-core/actions/workflows/ci.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/ci.yml)
[![CodeQL Analysis](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml)
[![Build containers](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml)

[Synergy](https://symless.com/synergy) is a keyboard and mouse sharing tool. 

Use the keyboard, mouse, or trackpad of one computer to control nearby computers, and work seamlessly between them.

This project contains the source code for Synergy 1 Community Edition which is actively maintained and free to use (no serial key is required for the community edition).

![Synergy 1 Community Edition](https://github.com/user-attachments/assets/faf5bd69-336c-4bd0-ace3-e911f199d961)

To use the community edition, install the `synergy` package with your favorite package manager or build it yourself using the Developer Quick Start instructions below.

Join us on [Discord](https://discord.com/invite/xBFv6j7) or [Slack](https://synergy-app.slack.com/join/shared_invite/zt-d8if26fr-6x~TSTz4skGmTnFP5IPaww#/shared-invite/email) in the `#open-source` channel.

* [Download Synergy](https://symless.com/synergy/download)
* [Contact support](https://symless.com/synergy/contact)
* [Help articles](https://symless.com/synergy/help)
* [Project Wiki](https://github.com/symless/synergy-core/wiki)

## Developer Quick Start

How to build Synergy 1 Community Edition. Check the [Developer Guide](https://github.com/symless/synergy-core/wiki/Developer-Guide) wiki page if you have problems.

**Dependencies:**

*Windows:*
```
python scripts/install_deps.py
```

*macOS/Linux:*
```
./scripts/install_deps.py
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

Synergy 1 Community Edition is packaged by the community (status shown below).

Synergy 2 is no longer in development and should not be provided.

For the licensed builds of Synergy 1 and Synergy 3, please use the [official packages](https://symless.com/synergy/download).

**Windows:**
```
choco install synergy
```

**macOS:**
```
brew install synergy
```

**Debian, Ubuntu, etc:**
```
sudo apt install synergy
```

**Fedora, Red Hat, etc:**
```
sudo dnf install synergy
```

**Arch, Manjaro, etc:**
```
sudo pacman -Syu synergy
```

[![Repology](https://repology.org/badge/vertical-allrepos/synergy.svg?exclude_unsupported=1)](https://repology.org/project/synergy/versions)
