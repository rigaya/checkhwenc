Name:       $PACKAGE_NAME$
Version:    $PACKAGE_VERSION$
Release:    1
Group:      Applications
Summary:    $PACKAGE_NAME$
Packager:   $PACKAGE_MAINTAINER$
License:    $PACKAGE_LICENSE$
Source:     tmp.tar.gz
BuildRoot:  %{_tmppath}/%{name}-%{version}-buildroot
BuildArch:  $PACKAGE_ARCH$
Requires:   $PACKAGE_DEPENDS$

%description
$PACKAGE_DESCRIPTION$

%global debug_package %{nil}

%prep
rm -rf $RPM_BUILD_ROOT
 
%setup -n %{name}

%build

%install
mkdir -p $RPM_BUILD_ROOT/usr/bin/
install -m 755 $PACKAGE_BIN$ $RPM_BUILD_ROOT/usr/bin/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
/usr/bin/$PACKAGE_BIN$

%changelog
