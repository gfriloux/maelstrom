Name         : maelstrom
Version      : 0.0.99
Release      : 1%{?dist}
License      : LGPLv2.1 BSD
Group        : System Environment/Libraries
Provides     : maelstrom = %{version}, libmaelstrom0 = %{version}
URL          : https://github.com/gfriloux/maelstrom
Packager     : Guillaume Friloux <guillaume@friloux.me>
Summary      : A whirling maelstrom of network libraries
Source       : ~/rpmbuild/SOURCES/maelstrom-0.0.99.tar.gz

BuildRequires: pkgconfig subversion automake doxygen m4 autoconf gzip bzip2 tar
BuildRequires: eina ecore

%description
A whirling maelstrom of network libraries

%package devel
Provides     : libmaelstrom-dev libmaelstrom-devel
Summary      : Maelstrom headers, static libraries, documentation and test programs
Group        : Development/Libraries
Requires     : %{name} = %{version}

%description devel
Headers, static libraries, test programs and documentation for maelstrom

%prep
rm -rf "%{buildroot}"
%setup -q

%build
touch config.rpath
./autogen.sh --prefix=/usr                                                     \
             --libdir=/usr/lib64

%install
make %{?_smp_mflags}
%makeinstall

%clean
rm -rf "%{buildroot}"

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_libdir}/lib*.so.*
%{_bindir}/*

%files devel
%defattr(-, root, root)
%{_libdir}/pkgconfig/*
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/*.la
#%{_libdir}/*.a
