%define name    bookcase
%define version 0.7.1
%define release 1rls
# get around stupid Mandrake libtool tag CXX bug
%define __libtoolize true

Summary: A book collection manager
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Office
Source: %{name}-%{version}.tar.gz
URL: http://www.periapsis.org/bookcase/
Requires: kdebase libxslt1 >= 1.0.19
BuildRequires: kdelibs-devel libxslt-devel >= 1.0.19
BuildRoot: %{_tmppath}/%{name}-buildroot

%description
Bookcase is a KDE application for keeping track of your book collection.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q

%build
%configure2_5x --disable-debug --enable-final --disable-rpath
make

%install
rm -Rf %{buildroot}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/apps/%{name}/pics
%makeinstall_std

mkdir -p %{buildroot}/%{_menudir}
cat > %{buildroot}/%{_menudir}/%{name} <<EOF
?package(%{name}):\
command="%_bindir/bookcase" \
icon="%{name}.png" \
kde_filename="bookcase" \
kde_command="bookcase -caption \"%c\" %i %m" \
title="Bookcase" \
longtitle="Book Collection Manager" \
needs="kde" \
section="Office/Accessories" \
mimetypes="application/x-bookcase"
EOF

#icons too
mkdir -p %{buildroot}/{%{_miconsdir},%{_liconsdir}}
install -m644 icons/hi32-app-bookcase.png %{buildroot}/%{_iconsdir}/%{name}.png
install -m644 icons/hi32-app-bookcase.png %{buildroot}/%{_liconsdir}/%{name}.png
install -m644 icons/hi16-app-bookcase.png %{buildroot}/%{_miconsdir}/%{name}.png

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
%{_bindir}/*
#%{_mandir}/man1/*
%{_datadir}/applnk/Applications/bookcase.desktop
%{_datadir}/mimelnk/application/x-bookcase.desktop
%{_libdir}/menu/*
%{_datadir}/apps/%{name}/
%{_iconsdir}/*/*/*/*.png
%{_iconsdir}/%{name}.png
%{_miconsdir}/%{name}.png
%{_liconsdir}/%{name}.png
%{_datadir}/doc/HTML/*/bookcase/*

%changelog
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
