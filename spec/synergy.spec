Summary: Share mouse and keyboard between multiple computers over the network
Name: synergy
Version: 1.7.4
Release: 1%{?dist}
License: GPLv2
Group: System Environment/Daemons
URL: http://synergy-project.org/
Source: https://github.com/synergy/synergy/archive/v1.7.4-stable.tar.gz
Provides: synergy = %{version}-%{release}
Obsoletes: synergy-plus < 1.4.15
BuildRequires: cmake
BuildRequires: libX11-devel
BuildRequires: libXext-devel
BuildRequires: libXtst-devel
BuildRequires: libXt-devel
BuildRequires: libXinerama-devel
BuildRequires: libcurl-devel
BuildRequires: qt-devel
BuildRequires: openssl-devel
BuildRequires: avahi-compat-libdns_sd-devel

%description
Synergy lets you easily share your mouse and keyboard between multiple
computers, where each computer has its own display. No special hardware is
required, all you need is a local area network. Synergy is supported on
Windows, Mac OS X and Linux. Redirecting the mouse and keyboard is as simple
as moving the mouse off the edge of your screen.


%prep
%setup -q -n synergy-%{version}-stable


%build
mkdir ext/gmock-1.6.0
unzip ext/gmock-1.6.0.zip -d ext/gmock-1.6.0/
mkdir ext/gtest-1.6.0
unzip ext/gtest-1.6.0.zip -d ext/gtest-1.6.0/
./configure 
make


%install
# No install target (yet? as of 1.3.7)
install -D -p -m 0755 bin/synergyc %{buildroot}%{_bindir}/synergyc
install -D -p -m 0755 bin/synergys %{buildroot}%{_bindir}/synergyd
install -D -p -m 0755 bin/synergys %{buildroot}%{_bindir}/synergys
install -D -p -m 0644 doc/synergyc.man %{buildroot}%{_mandir}/man8/synergyc.8
install -D -p -m 0644 doc/synergys.man %{buildroot}%{_mandir}/man8/synergys.8


%files
# None of the documentation files are actually useful here, they all point to
# the online website, so include just one, the README
%doc README doc/synergy.conf.example*
%{_bindir}/synergyc
%{_bindir}/synergyd
%{_bindir}/synergys
%{_mandir}/man8/synergyc.8*
%{_mandir}/man8/synergys.8*

%changelog
* Thu Oct 29 2015 Rohan Ferris <r.ferris@library.uq.edu.au> - 1.7.4
- Reflected changes to project hosting structure
- Include sample configuration files

* Sun Aug 04 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.4.10-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Mon Feb 18 2013 Christian Krause <chkr@fedoraproject.org> - 1.4.10-1
- Update to 1.4.10 (#843971).
- Cleanup spec file.

* Fri Feb 15 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.7-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sat Jul 21 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.7-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Tue Feb 28 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.7-5
- Rebuilt for c++ ABI breakage

* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.7-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Mon Jul 18 2011 Matthias Saou <matthias@saou.eu> 1.3.7-3
- Add missing Provides for synergy-plus (#722843 re-review).

* Mon Jul 18 2011 Matthias Saou <matthias@saou.eu> 1.3.7-2
- Update summary.

* Tue Jul 12 2011 Matthias Saou <matthias@saou.eu> 1.3.7-1
- Update to 1.3.7.
- Drop patch disabling XInitThreads, see upstream #610.
- Update %%description and %%doc.
- Replace cmake patch with our own install lines : Less rebasing.

* Mon Jul 11 2011 Matthias Saou <matthias@saou.eu> 1.3.6-2
- Update Obsoletes for the latest version + fix (release + 1 because of dist).
- Add missing cmake BuildRequires.
- Update cmake patch to also install man pages.

* Fri Feb 18 2011 quiffman GMail 1.3.6-1
- Update to reflect the synergy/synergy+ merge to synergy-foss.org (#678427).
- Build 1.3.5 and newer use CMake.
- Patch CMakeLists.txt to install the binaries.

* Thu Jul  8 2010 Matthias Saou <matthias@saou.eu> 1.3.4-6
- Don't apply the RHEL patch on RHEL6, only 4 and 5.

* Mon Dec  7 2009 Matthias Saou <matthias@saou.eu> 1.3.4-5
- Obsolete synergy (last upstream released version is from 2006) since synergy+
  is a drop-in replacement (#538179).

* Tue Nov 24 2009 Matthias Saou <matthias@saou.eu> 1.3.4-4
- Disable XInitThreads() on RHEL to fix hang (upstream #194).

* Tue Aug 18 2009 Matthias Saou <matthias@saou.eu> 1.3.4-3
- Don't use the -executable find option, it doesn't work with older versions.

* Tue Aug 18 2009 Matthias Saou <matthias@saou.eu> 1.3.4-2
- Initial RPM release, based on the spec from the original synergy.
- Remove spurious executable bit from sources files.
