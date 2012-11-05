
%define name    libgig
%define version 3.3.0.svn3
%define release 11
%define prefix  /usr/local

Summary:	C++ library for loading and modifying Gigasampler and DLS files.
Name:		%{name}
Version:	%{version}
Release:	%{release}
Prefix:		%{prefix}
License:	GPL
Group:		Sound
Source0:	%{name}-%{version}.tar.bz2
URL:		http://www.linuxsampler.org/libgig/
BuildRoot:	/var/tmp/%{name}-%{version}-buildroot

%description
C++ library for loading and modifying Gigasampler and DLS files.

%package devel
Summary:	C++ library for loading and modifying Gigasampler and DLS files.
Group:		Development/Libraries
Requires:	%{name} = %{version}

%description devel
C++ library for loading and modifying Gigasampler and DLS files.

%prep

%setup
if [ -f Makefile.cvs ]; then make -f Makefile.cvs; fi

%build
./configure --prefix=%{prefix}
make
make docs

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
mkdir -p $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{prefix} install

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%clean
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%{prefix}/bin/*
%{prefix}/lib/libgig.so*
%{prefix}/share/man/man1/*

%files devel
%defattr(-,root,root)
%doc doc/html/*
%{prefix}/lib/libgig.a
%{prefix}/lib/libgig.la
%{prefix}/lib/pkgconfig/gig.pc
%{prefix}/include/*

%changelog
* Thu Jul 30 2009 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for release 3.3.0
* Wed Dec 03 2008 Christian Schoenebeck <cuse@users.sourceforge.net>
- fixed rpmbuild error on Fedora Core 8
  (fixes #86, patch by Devin Anderson)
* Wed Dec 05 2007 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for release 3.2.1
* Sun Oct 14 2007 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for release 3.2.0
- libgig's home has moved to http://www.linuxsampler.org/libgig/
* Sat Mar 24 2007 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for 3.1.1
* Fri Nov 24 2006 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for 3.1.0
* Thu Jun 01 2006 Rui Nuno Capela <rncbc@users.sourceforge.net>
- changed deprecated copyright attribute to license
- added ldconfig to post-(un)install steps
* Sun May 07 2006 Christian Schoenebeck <cuse@users.sourceforge.net>
- libgig's home has been slightly changed from stud.fh-heilbronn.de
  to stud.hs-heilbronn.de
* Fri Apr 28 2006 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for 3.0.0
* Mon Aug 15 2005 Christian Schoenebeck <cuse@users.sourceforge.net>
- prepared for 2.0.2
* Mon Jun 13 2005 Rui Nuno Capela <rncbc@users.sourceforge.net>
- prepared for 2.0.1
* Mon May  9 2005 Rui Nuno Capela <rncbc@users.sourceforge.net>
- prepared for 2.0.0
* Wed Nov 24 2004 Rui Nuno Capela <rncbc@users.sourceforge.net>
- prepared for 1.0.0
* Sat Jul 10 2004 Christian Schoenebeck <cuse@users.sourceforge.net>
- renamed 'libgig.pc' to 'gig.pc' as well as the pkg-config lib name
* Wed Jul 02 2004 Rui Nuno Capela <rncbc@users.sourceforge.net>
- Created and corrected initial libgig.spec
