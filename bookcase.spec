%define name    bookcase
%define version 0.4.1
%define release 1rls
# get around stupid Mandrake libtool tag CXX bug
%define __libtoolize true

Summary: A book collection manager
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Graphical Desktop/KDE
Source: %{name}-%{version}.tar.gz
URL: http://periapsis.org/bookcase/
Requires: kdebase libxslt1 >= 1.0.19
BuildRequires: gcc-c++ libqt3-devel kdelibs-devel libxslt1-devel >= 1.0.19
BuildRoot: %{_tmppath}/%{name}-buildroot

%description
Bookcase is a personal catalog application for your book collection.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q

%build
%configure --disable-debug --enable-final
make

%install
#mkdir -p $RPM_BUILD_ROOT%{_bindir}
#mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
#mkdir -p $RPM_BUILD_ROOT%{_datadir}/applnk/Applications
mkdir -p $RPM_BUILD_ROOT%{_datadir}/apps/%{name}/pics
%makeinstall_std
# why does this happen?
mv $RPM_BUILD_ROOT/usr/bin/i586-mandrake-linux-gnu-bookcase $RPM_BUILD_ROOT/usr/bin/bookcase

(cd $RPM_BUILD_ROOT
mkdir -p ./usr/lib/menu
cat > ./usr/lib/menu/%{name} <<EOF
?package(%{name}):\
command="%_bindir/bookcase"\
icon="%{name}.png"\
kde_filename="bookcase"\
kde_command="bookcase -caption \"%c\" %i %m"\
title="Bookcase"\
longtitle="Book Collection Manager"\
needs="kde"\
section="Office/Accessories"
EOF
)  

%post
%{update_menus}

%postun
%{clean_menus}

%files
%defattr (-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README TODO
%{_bindir}/*
#%{_mandir}/man1/*
#%{_datadir}/applnk/Applications/bookcase.desktop
%{_libdir}/menu/*
%{_datadir}/apps/%{name}/
%{_iconsdir}/*/*/*/*.png
%{_datadir}/locale/*/*/*
%{_datadir}/doc/HTML/*/*/*

%clean 
rm -rf $RPM_BUILD_ROOT

%changelog
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
