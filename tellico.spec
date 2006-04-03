%define name    tellico
%define version 1.1.4
%define release 1
%define iconname %{name}.png
%define __libtoolize /bin/true

Summary: A collection manager
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Databases
#Source: %{name}-%{version}.tar.gz
Source: %{name}-%{version}.tar.gz
URL: http://www.periapsis.org/tellico/
Requires: kdebase libxslt1 >= 1.0.19
# needed for kcddb
Requires: libkdemultimedia-common
# needed for audio file metadata import
Requires: taglib
BuildRequires: kdelibs-devel >= 3.1 libxslt-devel >= 1.0.19
BuildRequires: ImageMagick libart_lgpl-devel libtaglib-devel
BuildRequires: libkdemultimedia-common-devel libkdemultimedia1-kscd-devel
# specific for Mandrake versions < 10.2
BuildRequires: mandrakelinux-create-kde-mdk-menu
BuildRoot: %{_tmppath}/%{name}-buildroot
Obsoletes: bookcase

%description
Tellico is a KDE application for keeping track of your collection of books, bibliographies, music, movies, video games, coins, stamps, trading cards, comic books, or wines.

Features:

o Supports any number of user-defined fields, of several different types:
   o text, paragraph, list, checkbox, year, URL, date, ratings
   o tables
   o images
   o combinations of other fields
o Handles books with multiple authors, genres, keywords, etc.
o Automatically formats titles and names
o Supports collection searching and view filtering
o Sorts and groups collection by various properties
o Automatically validates ISBN
o Allows customizable entry templates through XSLT
o Imports Bibtex, Bibtexml, RIS, CSV, MODS, and XSLT-filtered data
o Exports to Bibtex, Bibtexml, CSV, HTML, Onix, PilotDB, and XSLT-filtered data
o Searches and adds items from Amazon.com, imdb.com, z39.50 servers, PubMed, and external applications

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q

%build
%configure2_5x --disable-debug --enable-final --disable-rpath
%make

%install
rm -Rf %{buildroot}
%makeinstall_std

mkdir -p %{buildroot}{%{_miconsdir},%{_iconsdir},%{_liconsdir},%{_menudir}}
convert icons/%{name}.png -geometry 48x48 %{buildroot}%{_liconsdir}/%{iconname}
convert icons/%{name}.png -geometry 32x32 %{buildroot}%{_iconsdir}/%{iconname} 
convert icons/%{name}.png -geometry 16x16 %{buildroot}%{_miconsdir}/%{iconname} 

kdedesktop2mdkmenu.pl Tellico "More applications/Databases"    %buildroot/%_datadir/applnk/Applications/tellico.desktop                             %buildroot/%_menudir/tellico 

%find_lang %{name}

%post
%{update_menus}

%postun
%{clean_menus}

%clean 
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr (-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README TODO
%doc %dir %{_datadir}/doc/HTML/*/tellico/*
%{_bindir}/*
%{_datadir}/applnk/Applications/tellico.desktop
%{_datadir}/mimelnk/application/x-tellico.desktop
%{_datadir}/apps/kconf_update/tellico-rename.upd
%{_datadir}/apps/kconf_update/tellico.upd
%{_datadir}/apps/%{name}/
%{_menudir}/%{name}
%{_miconsdir}/%{iconname}
%{_iconsdir}/%{iconname}
%{_liconsdir}/%{iconname}
%{_iconsdir}/*/*/*  

%changelog
* Sun Apr  2 2006 Robby Stephenson <robby@periapsis.org> 1.1.4-1
- Version 1.1.4.

* Sun Mar 12 2006 Robby Stephenson <robby@periapsis.org> 1.1.3-1
- Version 1.1.3.

* Sun Mar  5 2006 Robby Stephenson <robby@periapsis.org> 1.1.2-1
- Version 1.1.2.

* Sat Feb 18 2006 Robby Stephenson <robby@periapsis.org> 1.1.1-1
- Version 1.1.1.

* Tue Feb  7 2006 Robby Stephenson <robby@periapsis.org> 1.1-1
- Version 1.1.

* Sun Jan 22 2006 Robby Stephenson <robby@periapsis.org> 1.1-0.pre2
- Version 1.1pre2.

* Thu Jan 12 2006 Robby Stephenson <robby@periapsis.org> 1.1-0.pre1
- Version 1.1pre1.

* Fri Sep  9 2005 Robby Stephenson <robby@periapsis.org> 1.0.1-1
- Version 1.0.1.

* Fri Aug 19 2005 Robby Stephenson <robby@periapsis.org> 1.0-1
- Version 1.0.

* Fri Aug 12 2005 Robby Stephenson <robby@periapsis.org> 1.0-0.pre2.1rls
- Version 1.0pre2.

* Wed Jul 27 2005 Robby Stephenson <robby@periapsis.org> 1.0-0.pre1.1rls
- Version 1.0pre1.

* Wed Jun 29 2005 Robby Stephenson <robby@periapsis.org> 0.13.8-1rls
- Version 0.13.8.

* Wed Apr 27 2005 Robby Stephenson <robby@periapsis.org> 0.13.7-1rls
- Version 0.13.7.

* Thu Mar 31 2005 Robby Stephenson <robby@periapsis.org> 0.13.6-1rls
- Version 0.13.6.

* Tue Mar  1 2005 Robby Stephenson <robby@periapsis.org> 0.13.5-1rls
- Version 0.13.5.

* Mon Feb 14 2005 Robby Stephenson <robby@periapsis.org> 0.13.4-1rls
- Version 0.13.4.

* Tue Feb  8 2005 Robby Stephenson <robby@periapsis.org> 0.13.3-1rls
- Version 0.13.3.

* Fri Dec 17 2004 Robby Stephenson <robby@periapsis.org> 0.13.2-1rls
- Version 0.13.2.

* Fri Dec  3 2004 Robby Stephenson <robby@periapsis.org> 0.13.1-1rls
- Version 0.13.1.

* Sat Nov 20 2004 Robby Stephenson <robby@periapsis.org> 0.13-1rls
- Version 0.13.
- Updated description.
- Updated requires for libkdemultimedia-common.

* Thu Nov 18 2004 Robby Stephenson <robby@periapsis.org> 0.13-0.pre3.1rls
- Version 0.13pre3.

* Wed Nov 10 2004 Robby Stephenson <robby@periapsis.org> 0.13.0.pre2.1rls
- Version 0.13pre2.

* Sun Nov  7 2004 Robby Stephenson <robby@periapsis.org> 0.13-0.pre1.1rls
- Version 0.13pre1
- Removed dependence on libcdda (cdparanoia)

* Tue Sep 14 2004 Robby Stephenson <robby@periapsis.org> 0.12-1rls
- Renamed to Tellico
- Version 0.12

* Thu Aug 26 2004 Robby Stephenson <robby@periapsis.org> 0.11-1rls
- Version 0.11
- Changed group to Databases
- Added requires for taglib, libkcddb, and libcdda

* Tue Aug 10 2004 Robby Stephenson <robby@periapsis.org> 0.10-1rls
- Version 0.10

* Mon Aug  9 2004 Robby Stephenson <robby@periapsis.org> 0.10pre2-1rls
- Version 0.10pre2

* Sun Aug  8 2004 Robby Stephenson <robby@periapsis.org> 0.10pre1-1rls
- Version 0.10pre1

* Wed Apr 21 2004 Robby Stephenson <robby@periapsis.org> 0.9.1-1rls
- Version 0.9.1

* Sat Apr 10 2004 Robby Stephenson <robby@periapsis.org> 0.9-1rls
- Version 0.9

* Tue Feb  3 2004 Robby Stephenson <robby@periapsis.org> 0.8.1-1rls
- Version 0.8.1

* Mon Jan 26 2004 Robby Stephenson <robby@periapsis.org> 0.8-2rls
- Rebuild

* Sun Jan 25 2004 Robby Stephenson <robby@periapsis.org> 0.8-1rls
- Version 0.8

* Thu Jan 22 2004 Robby Stephenson <robby@periapsis.org> 0.8-0.pre1.1rls
- Version 0.8pre1

* Sat Jan 17 2004 Robby Stephenson <robby@periapsis.org> 0.7.2-2rls
- Update to Charles Edwards' version in Mandrake contribs.

* Tue Nov 25 2003 Robby Stephenson <robby@periapsis.org> 0.7.2-1rls
- Version 0.7.2

* Sun Nov  9 2003 Robby Stephenson <robby@periapsis.org> 0.7.1-1rls
- Version 0.7.1

* Sat Nov  8 2003 Robby Stephenson <robby@periapsis.org> 0.7-1rls
- Version 0.7

* Wed Nov  5 2003 Robby Stephenson <robby@periapsis.org> 0.7pre2-1rls
- Version 0.7pre2

* Sun Nov  2 2003 Robby Stephenson <robby@periapsis.org> 0.7pre1-1rls
- Version 0.7pre1
- Add cleanups from Buchan Milne for Mandrake contribs.

* Tue Jul 22 2003 Robby Stephenson <robby@periapsis.org> 0.6.6-1rls
- Version 0.6.6

* Mon Jul 21 2003 Robby Stephenson <robby@periapsis.org> 0.6.5-1rls
- Version 0.6.5

* Wed Jun 25 2003 Robby Stephenson <robby@periapsis.org> 0.6.4-1rls
- Version 0.6.4

* Mon May 26 2003 Robby Stephenson <robby@periapsis.org> 0.6.3-1rls
- Version 0.6.3

* Sat May 10 2003 Robby Stephenson <robby@periapsis.org> 0.6.2-1rls
- Version 0.6.2

* Mon May  5 2003 Robby Stephenson <robby@periapsis.org> 0.6.1-1rls
- Version 0.6.1

* Thu Apr  3 2003 Robby Stephenson <robby@periapsis.org> 0.6-1rls
- Version 0.6
- Fix application and mime desktop files

* Sun Mar 23 2003 Robby Stephenson <robby@periapsis.org> 0.5.2a-1rls
- Version 0.5.2a

* Sat Mar 22 2003 Robby Stephenson <robby@periapsis.org> 0.5.2-2rls
- Fix doc files

* Fri Mar 21 2003 Robby Stephenson <robby@periapsis.org> 0.5.2-1rls
- Getting ready for version 0.5.2

* Sat Mar 15 2003 Robby Stephenson <robby@periapsis.org> 0.5.1-1rls
- Version 0.5.1

* Fri Mar 14 2003 Robby Stephenson <robby@periapsis.org> 0.5-1rls
- Version 0.5

* Sun Dec 4 2002 Robby Stephenson <robby@periapsis.org> 0.4.1-1rls
- Version 0.4.1

* Sun Nov 24 2002 Robby Stephenson <robby@periapsis.org> 0.4-1rls
- Fixed so it actually makes an RPM
- Workaround for libtool "unrecognized option --tag=CXX"
- Removed cruft
- Requires libxslt1 >= 1.0.19
- BuildRequires libxslt1-devel >= 1.0.19

* Wed Aug 21 2002  <robby@periapsis.org> 0.3-1rls
- Change to Qt3
- Use png
- Some wording changes

* Wed Oct 17 2001  <robby@radiojodi.com> 0.3-1mdk
- BuildRequires: gcc-c++ libqt

* Thu Oct 11 2001  <robby@radiojodi.com> 0.3-1mdk
- initial spec.in file
