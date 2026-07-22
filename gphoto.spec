%define ver	0.5.0-pre
%define rel	1
%define prefix	/usr

Summary: GPhoto - GNU Digital Camera Software
Name: gphoto        
Version: %ver
Release: %rel
Copyright: GPL
Group: X11/Utilities
Source: ftp://ftp.gnome.org:/pub/GNOME/unstable/sources/gphoto/gphoto-%{ver}.tar.gz
BuildRoot: /var/tmp/gphoto-root
URL: http://www.gnu.org/software/gphoto/gphoto.html
Docdir: %{prefix}/doc
Requires: gtk+ >= 1.2.0
Requires: libusb >= 0.1
Requires: libgpio >= 0.0.2

%description
GPhoto is a collection of free programs and libraries for
integrating digital still photography devices with GNOME.

GNOME is the GNU Network Object Model Environment.
It makes your computer easy, powerful, and easy to
configure.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix
make

%install
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{prefix} install

%post
/sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc AUTHORS COPYING FAQ MANUAL NEWS README THANKS THEMES ChangeLog
%{prefix}/man/man1/gphoto.1
%{prefix}/man/man1/dc21x_cam.1
%{prefix}/bin/*
%{prefix}/lib/gphoto/*
%{prefix}/share/gphoto/doc/*
%{prefix}/share/gphoto/gallery/Default/*
%{prefix}/share/gphoto/gallery/RedNGray/*
%{prefix}/share/gphoto/gallery/CSStheme/*
%{prefix}/share/gnome/apps/Graphics/*
