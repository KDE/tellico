%define name    bookcase
%define version 0.9.1
%define release 1rls
%define iconname %{name}.png
%define __libtoolize /bin/true

Summary: A collection manager
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Office
Source: %{name}-%{version}.tar.gz
URL: http://www.periapsis.org/bookcase/
Requires: kdebase libxslt1 >= 1.0.19
BuildRequires: kdelibs-devel >= 3.1 libxslt-devel >= 1.0.19
BuildRequires: ImageMagick libart_lgpl-devel
BuildRoot: %{_tmppath}/%{name}-buildroot

%description
Bookcase is a KDE application for keeping track of your collection of books, bibliographies, music, movies, coins, stamps, trading cards, comic books, or wines.

Features:

o Supports any number of user-defined fields, of ten different types:
   o text, paragraph, list, checkbox, year, URL
   o tables of one or two columns.
   o images
   o combinations of other fields
o Handles books with multiple authors, genres, keywords, etc.
o Automatically formats titles and names
o Supports collection searching and view filtering
o Sorts and groups collection by various properties
o Automatically validates ISBN
o Allows customizable entry templates through XSLT
o Imports Bibtex, Bibtexml, CSV, and XSLT-filtered data
o Exports to Bibtex, Bibtexml, CSV, HTML, PilotDB, and XSLT-filtered data


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

kdedesktop2mdkmenu.pl bookcase "Office/Accessories"    %buildroot/%_datadir/applnk/Applications/bookcase.desktop                             %buildroot/%_menudir/bookcase 

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
%doc %dir %{_datadir}/doc/HTML/*/bookcase/*
%{_bindir}/*
%{_datadir}/applnk/Applications/bookcase.desktop
%{_datadir}/mimelnk/application/x-bookcase.desktop
%{_datadir}/apps/%{name}/
%{_menudir}/%{name}
%{_miconsdir}/%{iconname}
%{_iconsdir}/%{iconname}
%{_liconsdir}/%{iconname}
%{_iconsdir}/*/*/*  

%changelog
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
