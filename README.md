# Synergy

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=symless_synergy-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=symless_synergy-core)
[![CI](https://github.com/symless/synergy/actions/workflows/ci.yml/badge.svg)](https://github.com/symless/synergy/actions/workflows/ci.yml)
[![CodeQL Analysis](https://github.com/symless/synergy/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/symless/synergy/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/symless/synergy/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/symless/synergy/actions/workflows/sonarcloud-analysis.yml)
[![Build containers](https://github.com/symless/synergy/actions/workflows/build-containers.yml/badge.svg)](https://github.com/symless/synergy/actions/workflows/build-containers.yml)

[Synergy](https://symless.com/synergy) is a keyboard and mouse sharing app. 
Use the keyboard, mouse, or trackpad of one computer to control nearby computers, 
and work seamlessly between them.

This project contains the source code for _Synergy 1 Community Edition_ which is actively 
maintained.
Synergy 1 Community Edition is free and open source software, and anyone is welcome to build it,
run it, tinker with it, redistribute it as part of their own app, etc.

**Wayland support:** Wayland now has experimental support (GNOME 46 is required, KDE TBD).

![Synergy 1 Community Edition](https://github.com/user-attachments/assets/faf5bd69-336c-4bd0-ace3-e911f199d961)

To use the community edition, install the `synergy` package with your favorite package manager
or build it yourself using the Developer Quick Start instructions below.

## Goals and Philosophy

Version 1.15 brings a new philosophy of being more approachable to the open-source community 
instead of wholly focusing on commercial interests.
We still have customers to support the development of the code, but we are committed to maintaining
and improving Synergy 1 Community Edition for years to come and we're excited to work with our 
community of open source users to improve the code for the benefit of everyone.

## Operating Systems

We support all major operating systems, including Windows, macOS, Linux, and Unix-like BSD-derived.

All Linux distributions are supported, primarily focusing on: 
Debian, Ubuntu, Linux Mint, Fedora, RHEL, AlmaLinux, Rocky Linux, Arch Linux, openSUSE, Gentoo.

We officially support FreeBSD, and would also like to support: OpenBSD, NetBSD, DragonFly, Solaris.

## Ways to get involved

Here are a few ways to join in with the project and get involved:
* Compile the latest `master` version (see below) and [report a bug](https://github.com/symless/synergy/issues)
* [Submit a PR](https://github.com/symless/synergy/wiki/Contributing) (pull request) with a bug fix or improvement
* [Let us know](https://github.com/symless/synergy/issues) if you have an idea for an improvement

## Where to get help

* Join us on [Discord](https://discord.com/invite/xBFv6j7) or [Slack](https://synergy-app.slack.com/join/shared_invite/zt-d8if26fr-6x~TSTz4skGmTnFP5IPaww#/shared-invite/email) (`#open-source` channel)
* [Start a discussion](https://github.com/symless/synergy/discussions) on our GitHub project
* [Read the wiki](https://github.com/symless/synergy/wiki) for guides and info

## Developer Quick Start

Synergy 1 Community Edition is free and open source software, and anyone is welcome to build and 
run it. These instructions will build Synergy 1 Community Edition, which doesn't require a license
or serial key. Check the [Developer Guide](https://github.com/symless/synergy/wiki/Developer-Guide)
wiki page if you have problems.

**1. Dependencies:**

*Linux, macOS, or BSD-derived:*
```
./scripts/install_deps.sh
```

*Windows:*
```
python scripts/install_deps.py
```

**2. Configure:**

*Linux:*
```
cmake -B build --preset=linux-release
```

*macOS:*
```
cmake -B build --preset=macos-release
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
./build/bin/synergy
```

## How to install (packages)

Synergy 1 Community Edition is packaged by the community (status shown below).

Synergy 2 is no longer in development and we recommend that package maintainers do not provide it.

Synergy 3 and Synergy 1 (licensed) are available to download from the 
[official packages](https://symless.com/synergy/download).

**Community edition:**

*Fedora, Red Hat, etc:*
```
sudo dnf install synergy
```

*macOS:*
```
brew install synergy
```

*Debian, Ubuntu, etc:*
*(temporarily broken)*
```
sudo apt install synergy
```

*Arch, Manjaro, etc:*
*(temporarily broken)*
```
sudo pacman -S synergy
```

*Windows:*
*(temporarily broken)*
```
choco install synergy
```

*Note:* Some packages are temporarily broken. We have reached out to the package maintainers to 
resolve this. If you're a package maintainer and have a question for us, 
[please let us know](ttps://github.com/symless/synergy/wiki/Contact-the-team).

## Package Maintainers

Synergy is already available through most package managers as `synergy`, and we would love to see
the latest version of Synergy 1 Community Edition on every package manager for every OS we support
(Windows, macOS, Linux, Unix-like BSD-derived, etc).

Package maintainers can use `scripts/package.py` to see how we build packages,
but most package maintainers will create a packaging script rather than use our scripts 
(which is fine by us).
If you're a package maintainer and have a question, please 
[get in touch](https://github.com/symless/synergy/wiki/Contact-the-team).

Good packages:
- [Fedora](https://packages.fedoraproject.org/pkgs/synergy/synergy/)
- [Arch Linux](https://aur.archlinux.org/packages/synergy)
- [FreeBSD](https://www.freshports.org/sysutils/synergy)
- [Homebrew](https://formulae.brew.sh/formula/synergy-core#default)
- [Gentoo](https://packages.gentoo.org/packages/x11-misc/synergy)
- [openSUSE](https://build.opensuse.org/package/show/openSUSE:Factory/synergy)

Broken packages:
- [Ubuntu](https://launchpad.net/ubuntu/+source/synergy)
- [Debian](https://tracker.debian.org/pkg/synergy)
- [Chocolatey](https://community.chocolatey.org/packages/synergy)

It appears that the `synergy` package has been removed or discontinued from some package 
repositories. There are many reasons why this may happen, but sometimes if a package maintainer
steps down or leaves the project without finding a replacement, the package might lose support,
leading to its removal.
This can also happen when there are difficulties updating the package to the latest version,
and communication has broken down between the package maintainer and the upstream developers.
If you're a package maintainer and would like to bring the `synergy` package back to life, please 
[get in touch](https://github.com/symless/synergy/wiki/Contact-the-team) if you need our help.

## FAQ

### Has Synergy moved beyond its goals from the 1.x era?

Our goal for Synergy 1 (including the community edition) has always been and will always be to 
make a simple, reliable, and feature-rich mouse and keyboard-sharing tool. We do maintain another
product called Synergy 3, but as this uses Synergy 1 Core (the server and client part of Synergy),
we depend on Synergy 1 to remain stable and modern which is why we continue to develop and improve
the product.

### If I want to solve issues in Synergy do I need to contribute to a fork?

We welcome PRs (pull requests) from the community. If you'd like to make a change, please feel
free to [start a discussion](https://github.com/symless/synergy/discussions) or 
[open a PR](https://github.com/symless/synergy/wiki/Contributing).
It's great that people spin up re-branded forks of Synergy, power to them. However, it's not 
necessary if you want to make changes.
If you're thinking of starting your own re-branded fork of Synergy, it might be because we're
doing something wrong so please 
[let us know](https://github.com/symless/synergy/wiki/Contact-the-team) what we can do to let
you feel welcome in our community.

### Is clipboard sharing supported?

Absolutely. The clipboard-sharing feature is a cornerstone feature of the product and we are 
committed to maintaining and improving that feature.

### Is Wayland for Linux supported?

Yes! Wayland (the Linux display server protocol aimed to become the successor of the X Window 
System) is an important platform for us.
The [`libei`](https://gitlab.freedesktop.org/libinput/libei) and 
[`libportal`](https://github.com/flatpak/libportal) libraries enable 
Wayland support for Synergy. We would like to give special thanks to Peter Hutterer (@whot),
who is the author of `libei`, a major contributor to `libportal`, and the author of the Wayland
implementation in Synergy. Others such as Olivier Fourdan helped with the Wayland implementation,
and we rely on the work of our community of developers to continue the development of Wayland 
support.

### Where did it all start?

Synergy was first created in 2001 by Chris Schoeneman.
Read about the [history of the project](https://github.com/symless/synergy/wiki/History) on our
wiki.

## Repology

Repology monitors a huge number of package repositories and other sources comparing package
versions across them and gathering other information.

[![Repology](https://repology.org/badge/vertical-allrepos/synergy.svg?exclude_unsupported=1)](https://repology.org/project/synergy/versions)
