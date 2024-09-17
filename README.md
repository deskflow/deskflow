# Deskflow

> [!TIP]
> We're on Libera Chat IRC network (`#deskflow` or `#deskflow-dev`).
> 
> Or if you want to stay on GitHub, you can [start a new discussion](https://github.com/deskflow/deskflow/discussions).

> [!IMPORTANT]
> 🚀 Deskflow is now the upstream project for Synergy.
>
> The project has been renamed to draw a line between the community-run project (now Deskflow),
> and the commercially licensed product, Synergy.
>
> We are currently in the process of renaming the project to Deskflow and moving all licensing code 
> downstream.
> 
> Hold on to your butts, this is going to be a hell of a ride.
> 
> [Join the discussion](https://github.com/deskflow/deskflow/discussions/7517)

[![GitHub Release](https://img.shields.io/github/v/release/deskflow/deskflow?display_name=release&label=latest%20version)](https://github.com/deskflow/deskflow/releases)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/deskflow/deskflow)](https://github.com/deskflow/deskflow/commits/master/)
[![GitHub License](https://img.shields.io/github/license/deskflow/deskflow)](LICENSE)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)

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

![Deskflow](https://github.com/user-attachments/assets/faf5bd69-336c-4bd0-ace3-e911f199d961)

To use Deskflow you can follow the [Build Quick Start](#build-quick-start),
use one of our packages, or if it's available by your favorite package repository,
install `deskflow` (see: [installing packages](#how-to-install-packages)).

## Project values

- Motivated by the community interests (not business-driven)
- Privacy by default (no update check or phone home on by default)
- Nothing customer-related (this is all moved downstream to Synergy)
- Have fun; we don't need to worry about impressing anyone

## Ways to get involved

Here are a few ways to join in with the project and get involved:
* Build the latest `master` version (see below) and [report a bug](https://github.com/deskflow/deskflow/issues)
* [Submit a PR](https://github.com/deskflow/deskflow/wiki/Contributing) (pull request) with a bug fix or improvement
* [Let us know](https://github.com/deskflow/deskflow/issues) if you have an idea for an improvement

## Build Quick Start

> [!TIP]
> Check the [Build Guide](https://github.com/deskflow/deskflow/wiki/Build-Guide)
> wiki page if you have problems.

**1. Dependencies:**

You can either copy/paste the command from [config.yaml] or use the script.

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

**5. Run**
```
./build/bin/deskflow
```

## How to install (packages)

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

We support all major operating systems, including Windows, macOS, Linux, and Unix-like BSD-derived.

All Linux distributions are supported, primarily focusing on: 
Debian, Ubuntu, Linux Mint, Fedora, RHEL, AlmaLinux, Rocky Linux, Arch Linux, openSUSE, Gentoo.

We officially support FreeBSD, and would also like to support: OpenBSD, NetBSD, DragonFly, Solaris.

## Similar Projects

* [Lan Mouse](https://github.com/feschber/lan-mouse) - A Rust implementation with the goal of
  having native front-ends and interoperability with Deskflow.
* [Input Leap](https://github.com/input-leap/input-leap) - A Deskflow-derivative with the goal
  of continuing what Barrier started (a now dead Deskflow fork).

## FAQ

### Is Deskflow compatible with Lan Mouse or Input Leap?

We would love to see compatibility between our fellow open source projects, Lan Mouse and 
Input Leap. This idea is discussed from time in the communities for all of our projects,
so it may happen in the not-too-distant future.

### If I want to solve issues in Deskflow do I need to contribute to a fork?

We welcome PRs (pull requests) from the community. If you'd like to make a change, please feel
free to [start a discussion](https://github.com/deskflow/deskflow/discussions) or 
[open a PR](https://github.com/deskflow/deskflow/wiki/Contributing).

### Is clipboard sharing supported?

Absolutely. The clipboard-sharing feature is a cornerstone feature of the product and we are 
committed to maintaining and improving that feature.

### Is Wayland for Linux supported?

Yes! Wayland (the Linux display server protocol aimed to become the successor of the X Window 
System) is an important platform for us.
The [`libei`](https://gitlab.freedesktop.org/libinput/libei) and 
[`libportal`](https://github.com/flatpak/libportal) libraries enable 
Wayland support for Deskflow. We would like to give special thanks to Peter Hutterer,
who is the author of `libei`, a major contributor to `libportal`, and the author of the Wayland
implementation in Deskflow. Others such as Olivier Fourdan and Povilas Kanapickas helped with the
Wayland implementation.

### Where did it all start?

Deskflow was first created as Synergy in 2001 by Chris Schoeneman.
Read about the [history of the project](https://github.com/deskflow/deskflow/wiki/History) on our
wiki.

## Repology

Repology monitors a huge number of package repositories and other sources comparing package
versions across them and gathering other information.

[![Repology](https://repology.org/badge/vertical-allrepos/deskflow.svg?exclude_unsupported=1)](https://repology.org/project/deskflow/versions)

## License

This project is licensed under [GPL-2.0](LICENSE) with an [OpenSSL exception](LICENSE_EXCEPTION).
