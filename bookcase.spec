%define name    bookcase
%define version 0.2
%define release 1mdk

Summary: Personal catalog for books, music, and movies
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Graphical Desktop/KDE
Source: %{name}-%{version}.tar.gz
URL: http://radiojodi.com/bookcase/
Requires: kdebase qt >= 2.3
BuildRequires: kdelibs-devel gcc-c++ libqt2 libqt2-devel
BuildRoot: %{_tmppath}/%{name}-buildroot

%description
Bookcase is a personal catalog application for books, music, and movies.

%prep
rm -rf $RPM_BUILD_ROOT

%setup  -q
find . -name "*.h" -exec perl -pi -e '$_.="\n" if eof' {} \;
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS"

%build
%configure --enable-final $LOCALFLAGS
make

%install
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applnk/Office
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/gnome/apps/Office
make DESTDIR=$RPM_BUILD_ROOT install-strip

(cd $RPM_BUILD_ROOT
mkdir -p ./usr/lib/menu
cat > ./usr/lib/menu/%{name} <<EOF
?package(%{name}):\
command="%_bindir/bookcase"\
icon="%{name}.png"\
kde_filename="bookcase"\
kde_command="bookcase -caption \"%c\" %i %m"\
title="Bookcase"\
longtitle="Personal Catalog"\
needs="kde"\
section="Office/Accessories"
EOF
)  


mkdir -p $RPM_BUILD_ROOT%{_miconsdir}
mkdir -p $RPM_BUILD_ROOT%{_liconsdir}


%post
%{update_menus}

%postun
%{clean_menus}

%files
%defattr (-,root,root)
%doc doc/ AUTHORS COPYING ChangeLog INSTALL NEWS README TODO bookcase.dtd
%{_bindir}/*
%{_mandir}/man1/*
%{_datadir}/applnk/Office/bookcase.desktop
%{_datadir}/gnome/apps/Office/bookcase.desktop
%{_libdir}/menu/*
%{_iconsdir}/*.png
%{_miconsdir}/*.png
%{_liconsdir}/*.png

%clean 
rm -rf $RPM_BUILD_ROOT

%changelog
* Sat Feb  9 2002  <robby@radiojodi.com> 0.2-1mdk
- Changed xpm to png
- Added desktop files to Office category
- Added bookcase.dtd to doc files
- Added --enable-final to configure
- BuildRequires: libqt2 kdelibs

* Wed Oct 17 2001  <robby@radiojodi.com> 0.1-2mdk
- BuildRequires: gcc-c++ libqt

* Thu Oct 11 2001  <robby@radiojodi.com> 0.1-1mdk
- initial spec.in file
