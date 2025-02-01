![Deskflow](https://github.com/user-attachments/assets/f005b958-24df-4f4a-9bfd-4f834dae59d6)

> [!TIP]
> [Synergy](https://symless.com/synergy) sponsors the Deskflow project by contributing code and providing financial support.
> 
> - [**Bounties**](https://github.com/deskflow/deskflow/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22%F0%9F%92%8E%20bounty%22) - Earn while contributing to open source
> - [**Rewarded**](https://github.com/deskflow/deskflow/issues?q=label%3A%22%F0%9F%92%B0%20rewarded%22%20) - Issues with a rewarded bounty
> 
> **Deskflow** is the official upstream project for Synergy.
> Purchasing a Synergy license is one way to support Deskflow’s growth and sustainability.
> Learn more: [Relationship with Synergy](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy)

**Deskflow** is a free and open source keyboard and mouse sharing app.
Use the keyboard, mouse, or trackpad of one computer to control nearby computers,
and work seamlessly between them.
It's like a software KVM (but without the video).
TLS encryption is enabled by default. Wayland is supported. Clipboard sharing is supported.

[![Downloads: Stable Release](https://img.shields.io/github/downloads/deskflow/deskflow/latest/total?style=for-the-badge&logo=github&label=Download%20Stable)](https://github.com/deskflow/deskflow/releases/latest)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[![Downloads: Continuous Build](https://img.shields.io/github/downloads/deskflow/deskflow/continuous/total?style=for-the-badge&logo=github&label=Download%20Continuous)](https://github.com/deskflow/deskflow/releases/continuous)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[![Download From Flathub](https://img.shields.io/flathub/downloads/org.deskflow.deskflow?style=for-the-badge&logo=flathub&label=Download%20from%20flathub)](https://flathub.org/apps/org.deskflow.deskflow)

To use Deskflow you can use one of our [packages](https://github.com/deskflow/deskflow/releases), install `deskflow` (if available in your package repository), or [build it](#build-quick-start) yourself from source.

> [!TIP]
> For macOS users, the easiest way to install and stay up to date is to use [Homebrew](https://brew.sh) with our [homebrew-tap](https://github.com/deskflow/homebrew-tap).

[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/deskflow/deskflow?logo=github)](https://github.com/deskflow/deskflow/commits/master/)
[![GitHub top language](https://img.shields.io/github/languages/top/deskflow/deskflow?logo=github)](https://github.com/deskflow/deskflow/commits/master/)
[![GitHub License](https://img.shields.io/github/license/deskflow/deskflow?logo=github)](LICENSE)
[![REUSE status](https://api.reuse.software/badge/github.com/deskflow/deskflow)](https://api.reuse.software/info/github.com/deskflow/deskflow)

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=coverage)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=code_smells)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=deskflow_deskflow&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=deskflow_deskflow)

[![CI](https://github.com/deskflow/deskflow/actions/workflows/continuous-integration.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/continuous-integration.yml)
[![CodeQL Analysis](https://github.com/deskflow/deskflow/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/codeql-analysis.yml)
[![SonarCloud Analysis](https://github.com/deskflow/deskflow/actions/workflows/sonarcloud-analysis.yml/badge.svg)](https://github.com/deskflow/deskflow/actions/workflows/sonarcloud-analysis.yml)
## Project Values

- Motivated by the community interests (not business-driven)
- Privacy by default (e.g. update check is off by default)
- Leading edge releases (we don't focus on supporting older systems)
- Decisions are discussed and documented publicly with majority rule
- Have fun; we don't need to worry about impressing anyone

## Ways to get involved

> [!TIP]
> Join us! Real-time discussion on Matrix: [`#deskflow:matrix.org`](https://matrix.to/#/#deskflow:matrix.org)
>
> Alternatively, we have [other ways](https://github.com/deskflow/deskflow/wiki/Chat-with-us) to communicate.
> 
Here are a few ways to join in with the project and get involved:
* Build the latest `master` version (see below) and [report a bug](https://github.com/deskflow/deskflow/issues)
* [Submit a PR](https://github.com/deskflow/deskflow/wiki/Contributing) (pull request) with a bug fix or improvement
* [Let us know](https://github.com/deskflow/deskflow/issues) if you have an idea for an improvement

## Build Quick Start

For instructions on building Deskflow, use the wiki page: [Building](https://github.com/deskflow/deskflow/wiki/Building)

## Operating Systems

We support all major operating systems, including Windows, macOS, Linux, and Unix-like BSD-derived.

Windows 10 or higher is required.

macOS 12 or higher is required.

Linux requires libei 1.3+ and libportal 0.8+ for the server/client. Additionally, Qt 6.7+ is required for the GUI.
Linux users with systems not meeting these requirements should use flatpak in place of a native package.

We officially support FreeBSD, and would also like to support: OpenBSD, NetBSD, DragonFly, Solaris.

## Repology

Repology monitors a huge number of package repositories and other sources comparing package
versions across them and gathering other information.

[![Repology](https://repology.org/badge/vertical-allrepos/deskflow.svg?exclude_unsupported=1)](https://repology.org/project/deskflow/versions)

**Note:** We are working with package maintainers to have our new package name adopted.

## Installing on macOS

When you install Deskflow on macOS, you need to allow accessibility access (Privacy & Security) to both the `Deskflow` app and the `deskflow` process.

If using Sequoia, you may also need to allow `Deskflow` under Local Network‍ settings (Privacy & Security).
When prompted by the OS, go to the settings and enable the access.

If you are upgrading and you already have `Deskflow` or `deskflow`
on the allowed list you will need to manually remove them before accessibility access can be granted to the new version.

macOS users who download directly from releases may need to run `xattr -c /Applications/Deskflow.app` after copying the app to the `Applications` dir.

It is recommend to install Deskflow using [Homebrew](https://brew.sh) from our [homebrew-tap](https://github.com/deskflow/homebrew-tap)

To add our tap, run:
```
brew tap deskflow/homebrew-tap
```
Then install either:
- Stable: `brew install deskflow`
- Continuous: `brew install deskflow-dev`



## Collaborative Projects

In the open source developer community, similar projects collaborate for the improvement of all
mouse and keyboard sharing tools. We aim for idea sharing and interoperability.

* [**Lan Mouse**](https://github.com/feschber/lan-mouse) -
  Rust implementation with the goal of having native front-ends and interoperability with
  Deskflow/Synergy.
* [**Input Leap**](https://github.com/input-leap/input-leap) -
  Deskflow/Synergy-derivative with the goal of continuing what Barrier started, after Barrier
  became a dead fork.
* [**Synergy**](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy) -
  Downstream commercial fork and Deskflow sponsor, geared toward adapting to customer
  needs, offering business and enterprise licensing.

## FAQ

### What is the relationship with Synergy?

[![Sponsored by: Synergy](https://raw.githubusercontent.com/deskflow/deskflow-artwork/b2c72a3e60a42dee793bd47efc275b5ee0bdaa5f/misc/synergy-sponsor.svg)](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy)

Synergy sponsors the Deskflow project by contributing code and providing financial support while maintaining its customer-oriented code downstream.

Learn more: [Relationship with Synergy](https://github.com/deskflow/deskflow/wiki/Relationship-with-Synergy)

### Is Deskflow compatible with Synergy, Input Leap, or Barrier?

Yes, Deskflow has network compatibility with all forks:
- Requires Deskflow >= v1.17.0.96
- Deskflow will *just work* with Input Leap and Barrier (server or client).
- Connecting a Deskflow client to a Synergy 1 server will also *just work*.
- To connect a Synergy 1 client, you need to select the Synergy protocol in the Deskflow server settings.

_Note:_ Only Synergy 1 is compatible with Deskflow (Synergy 3 is not yet compatible).

### Is Deskflow compatible with Lan Mouse?

We would love to see compatibility with Lan Mouse. This may be quite an effort as currently the way they handle the generated input is very different.

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

Some features _may_ be unavailable or broken on Wayland. Please see the [known Wayland issues](https://github.com/deskflow/deskflow/discussions/7499).

### Where did it all start?

Deskflow was first created as Synergy in 2001 by Chris Schoeneman.
Read about the [history of the project](https://github.com/deskflow/deskflow/wiki/History) on our
wiki.

## Meow'Dib (our mascot)

![Meow'Dib](https://github.com/user-attachments/assets/726f695c-3dfb-4abd-875d-ed658f6c610f)


## Deskflow Contributors

Deskflow is made by possible by these contributors.

 <a href = "https://github.com/deskflow/deskflow/graphs/contributors">
   <img src = "https://contrib.rocks/image?repo=deskflow/deskflow"/>
 </a>

## License

This project is licensed under [GPL-2.0](LICENSE) with an [OpenSSL exception](LICENSES/LicenseRef-OpenSSL-Exception.txt).
