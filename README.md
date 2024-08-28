# Synergy

[![Quality Gate](https://sonarcloud.io/api/project_badges/measure?project=symless_synergy-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=symless_synergy-core)
[![CI](https://github.com/symless/synergy-core/actions/workflows/ci.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/ci.yml)
[![CodeQL Analysis](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/sonarcloud-analysis.yml)
[![Build containers](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml/badge.svg)](https://github.com/symless/synergy-core/actions/workflows/build-containers.yml)

[Synergy](https://symless.com/synergy) is a keyboard and mouse sharing app. Use the keyboard, mouse, or trackpad of one computer to control nearby computers, and work seamlessly between them.

This project contains the source code for _Synergy 1 Community Edition_ which is actively maintained, free to use, and does not require a license or serial key.

![Synergy 1 Community Edition](https://github.com/user-attachments/assets/faf5bd69-336c-4bd0-ace3-e911f199d961)

To use the community edition, install the `synergy` package with your favorite package manager or build it yourself using the Developer Quick Start instructions below.

Join us on [Discord](https://discord.com/invite/xBFv6j7) or [Slack](https://synergy-app.slack.com/join/shared_invite/zt-d8if26fr-6x~TSTz4skGmTnFP5IPaww#/shared-invite/email) in the `#open-source` channel.

* [Read the wiki](https://github.com/symless/synergy/wiki)
* [Raise an issue](https://github.com/symless/synergy/issues)
* [Start a discussion](https://github.com/symless/synergy/discussions)

## Developer Quick Start

How to build Synergy 1 Community Edition. Check the [Developer Guide](https://github.com/symless/synergy-core/wiki/Developer-Guide) wiki page if you have problems.

**1. Dependencies:**

*Windows:*
```
python scripts/install_deps.py
```

*macOS/Linux:*
```
./scripts/install_deps.py
```

**2. Configure:**

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

**3. Build:**
```
cmake --build build -j8
```

**4. Test:**
```
./build/bin/unittests
```

## Packages

Synergy 1 Community Edition is packaged by the community (status shown below). Package maintainers can use `scripts/package.py` to build packages.

Synergy 2 is no longer in development and we recommend that package maintainers do not provide it.

Synergy 3 and Synergy 1 (licensed) are available to download from the [official packages](https://symless.com/synergy/download).

**Community edition:**

*Windows:*
```
choco install synergy
```

*macOS:*
```
brew install synergy
```

*Debian, Ubuntu, etc:*
```
sudo apt install synergy
```

*Fedora, Red Hat, etc:*
```
sudo dnf install synergy
```

*Arch, Manjaro, etc:*
```
sudo pacman -S synergy
```

*Repology:*

[![Repology](https://repology.org/badge/vertical-allrepos/synergy.svg?exclude_unsupported=1)](https://repology.org/project/synergy/versions)
