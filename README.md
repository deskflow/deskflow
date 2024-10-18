![Deskflow](https://github.com/user-attachments/assets/f005b958-24df-4f4a-9bfd-4f834dae59d6)

> [!TIP]
> Join us! Real-time discussion on Matrix: [`#deskflow:matrix.org`](https://matrix.to/#/#deskflow:matrix.org)
>
> Alternatively, we have [other ways](https://github.com/deskflow/deskflow/wiki/Chat-with-us) to communicate.

![GitHub top language](https://img.shields.io/github/languages/top/deskflow/deskflow)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/deskflow/deskflow)](https://github.com/deskflow/deskflow/commits/master/)
[![Sponsored by: Synergy](https://raw.githubusercontent.com/deskflow/deskflow-artwork/b2c72a3e60a42dee793bd47efc275b5ee0bdaa5f/misc/synergy-sponsor.svg)](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy)

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=coverage)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)

[![CI](https://github.com/deskflow/deskflow/actions/workflows/ci.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/ci.yml)
[![CodeQL Analysis](https://github.com/deskflow/deskflow/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/deskflow/deskflow/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/sonarcloud-analysis.yml)
[![Build containers](https://github.com/deskflow/deskflow/actions/workflows/build-containers.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/build-containers.yml)

**Deskflow** is a free and open source keyboard and mouse sharing app.
Use the keyboard, mouse, or trackpad of one computer to control nearby computers,
and work seamlessly between them.

**Wayland support:** Experimental support in 
[Deskflow v1.16](https://github.com/deskflow/deskflow/releases/tag/1.16.0-beta%2Br2)
(required >= GNOME 46 or KDE Plasma 6.1).

![Deskflow GUI](https://github.com/user-attachments/assets/883660dc-f3f5-4b69-8821-a079a58d3882)

To use Deskflow you can follow the [Build Quick Start](#build-quick-start),
use one of our packages, or if it's available by your favorite package repository,
install `deskflow` (see: [installing packages](#how-to-install-packages)).

> [!NOTE]
> 🚀 **Deskflow** is now the upstream project for Synergy.
>
> The Deskflow project was established to cultivate community-driven development where everyone can collaborate.
>
> Synergy supports and contributes to the Deskflow project while maintaining its customer-oriented code downstream.
>
> More info: [Relationship with Synergy](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy)

## Project Values

- Motivated by the community interests (not business-driven)
- Privacy by default (e.g. update check is off by default)
- Nothing customer-related (this is all moved downstream to Synergy)
- Leading edge releases (we don't focus on supporting older systems)
- Decisions are discussed and documented publicly with majority rule
- Have fun; we don't need to worry about impressing anyone

## Ways to get involved

Here are a few ways to join in with the project and get involved:
* Build the latest `master` version (see below) and [report a bug](https://github.com/deskflow/deskflow/issues)
* [Submit a PR](https://github.com/deskflow/deskflow/wiki/Contributing) (pull request) with a bug fix or improvement
* [Let us know](https://github.com/deskflow/deskflow/issues) if you have an idea for an improvement

## Building

To compile Deskflow, please read: [`doc/build.md`](doc/build.md)

## Installing

First, try: [Continuous Build](https://github.com/deskflow/deskflow/releases) release.

Soon, you will also be able to install Deskflow using your favorite package manager.

*macOS:*
*(coming soon)*
```
brew install deskflow
```

*Fedora, Red Hat, etc:*
*(coming soon)*
```
sudo dnf install deskflow
```

*Debian, Ubuntu, etc:*
*(coming soon)*
```
sudo apt install deskflow
```

*Arch, Manjaro, etc:*
*(coming soon)*
```
sudo pacman -S deskflow
```

*Windows:*
*(coming soon)*
```
choco install deskflow
```

**Note:** We are working with package maintainers to have our new package name adopted.

## Operating Systems

We support all major operating systems, including:
Windows, macOS, Linux, and Unix-like BSD-derived.

All Linux distributions are supported, focusing on: 
Debian, Ubuntu, Fedora, openSUSE, Rocky Linux,AlmaLinux, Arch Linux, Manjaro.

We officially support all BSD-derived Unix-like, focusing on FreeBSD.

## Collaborative Projects

In the open source developer community, similar projects collaborate for the benefit of all
mouse and keyboard sharing tools. We aim for idea sharing and interoperability.

* [**Lan Mouse**](https://github.com/feschber/lan-mouse) -
  Rust implementation with the goal of having native front-ends and interoperability with Deskflow/Synergy.
* [**Input Leap**](https://github.com/input-leap/input-leap) -
  Deskflow/Synergy-derivative with the goal of continuing what Barrier started, after Barrier became a dead fork.
* [**Synergy**](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy) -
  Downstream commercial fork and Deskflow sponsor, geared toward adapting to customer needs,
  offering business and enterprise licensing.

## FAQ

Please see our wiki [FAQ page](https://github.com/deskflow/deskflow/wiki/Project-FAQ) for answers to questions like:
- What's the difference between Deskflow and other forks?
- Is Deskflow compatible with Lan Mouse or Input Leap/Barrier?
- Is clipboard sharing supported?
- Is Wayland for Linux supported?

## Repology

Repology monitors a huge number of package repositories and other sources comparing package
versions across them and gathering other information.

[![Repology](https://repology.org/badge/vertical-allrepos/deskflow.svg?exclude_unsupported=1)](https://repology.org/project/deskflow/versions)

## Meow'Dib (our mascot)

![Meow'Dib](https://github.com/user-attachments/assets/726f695c-3dfb-4abd-875d-ed658f6c610f)

## License

This project is licensed under [GPL-2.0](LICENSE) with an [OpenSSL exception](LICENSE_EXCEPTION).
