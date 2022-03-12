%{!?buildtype: %global buildtype debug}
%{!?warning_level: %global warning_level 1}
%{!?ndebug: %global ndebug if-release}
%{!?gcov: %global gcov 1}

Name: beyond
Summary: BeyonD
Version: 0.0.1
Release: 0
License: Apache-2.0
Source0: %{name}-%{version}.tar.gz
Source1001: %{name}.manifest
Source1002: %{name}-plugins.manifest
Source1003: %{name}-samples.manifest
Source1004: %{name}-tizen-capi.manifest
Source1005: %{name}-mock.manifest
Source1006: %{name}-tools.manifest
Source2002: %{name}_unittest.sh
Requires: gstreamer >= 1.8.0
Requires: nnstreamer >= 1.6.0
BuildRequires: bc
BuildRequires: curl
BuildRequires: unzip
BuildRequires: glib2-devel
BuildRequires: gstreamer-devel
BuildRequires: gst-plugins-base-devel
BuildRequires: pkgconfig(dlog)
BuildRequires: libopenssl1.1-devel
BuildRequires: grpc-devel
BuildRequires: protobuf-compiler
BuildRequires: cmake
BuildRequires: gtest-devel
# for tools
BuildRequires: opencv-devel
# for st_tizen
BuildRequires: jsoncpp-devel
# for st_tizen unittest
BuildRequires: bundle-devel
BuildRequires: rpc-port-devel
BuildRequires: capi-appfw-package-manager-devel
BuildRequires: capi-base-common-devel
BuildRequires: jsoncpp-devel
%if 0%{?gcov}
BuildRequires: lcov
%endif

# for libbeyond-runtime_tflite plugin
BuildRequires: tensorflow-lite-devel

%description
 BeyonD - AI based on Distributed Computing System.
 BeyonD Artificial Intelligence System, "BeyonD", provides you the smartest AI system that includes inference and training, working on the loosely coupled IoT devices. The BeyonD is going to get the low-end device to be a component of a network-based AI system. Hence the BeyonD makes a various number of devices that could work together in order to use their remaining computing resources for the AI.
 We and You want to get dummy IoT to be a machine that is able to talk to us.
 Here, BeyonD will make your dream come true.

%package devel
Summary: BeyonD development supporting package
Group: Development/Libraries
Requires: beyond-common-devel
Requires: %{name} = %{version}-%{release}

%description devel
 This is a development package for BeyonD framework.
 You are able to integrate your Idea or new machine learning(+inference) system to BeyonD.
 You can imagine a more smart computer system that is based on a network "BeyonD" the AI.
 Grab our hands, you will be there. The future.

# NOTE:
# Common package does not have dependencies to a library package libbeyond or libbeyond-tizen-capi
# It only provides common headers files, log.h and common.h
%package common-devel
Summary: BeyonD development common headers
Group: Development/Libraries

%description common-devel
 This package provides common headers for BeyonD C and CPP implementations

%package tizen-capi-devel
Summary: BeyonD development Tizen C API headers
Group: Development/Libraries
Requires: beyond-common-devel
Requires: beyond-tizen-capi

%description tizen-capi-devel
 This package provides Tizen C API headers

%package tizen-capi
Summary: BeyonD Tizen C API library
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description tizen-capi
 This package provides a Tizen C API library

%package plugins
Summary: BeyonD Plugin Collection
Requires: %{name} = %{version}-%{release}
Requires: libgrpc
BuildRequires: pkgconfig(dns_sd)

%description plugins
BeyonD Peer, Runtime and Discovery Module Collection that includes nnstreamer peer and various sample plugins

%package plugins-devel
Summary: BeyonD Plugin header files

%description plugins-devel
Header files for BeyonD Peer, Runtime and Discovery Module collection
Includes custom configuration structures

%package samples
Summary: BeyonD Sample Application Collection
Requires: %{name} = %{version}-%{release}

%description samples
BeyonD Sample Applications

%package mock
Summary: BeyonD mockup
Requires: %{name} = %{version}-%{release}

%description mock
BeyonD mockup

%package mock-devel
Summary: BeyonD mockup development
Requires: %{name} = %{version}-%{release}

%description mock-devel
BeyonD mockup development

%package tools
Summary: BeyonD Tool Collection
Requires: %{name} = %{version}-%{release}

%description tools
BeyonD Tools Collection

%prep
%setup -q
cp %{SOURCE1001} .
cp %{SOURCE1002} .
cp %{SOURCE1003} .
cp %{SOURCE1004} .
cp %{SOURCE1005} .
cp %{SOURCE1006} .

%build
%if 0%{?gcov}
export LDFLAGS+=" -lgcov -Wl,--dynamic-list-data"
%endif

mkdir -p build
cd build
%cmake .. \
    -DBUILD_GTEST=OFF \
    -DENABLE_GTEST=ON \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
    -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
    -DSYSCONF_INSTALL_DIR=%{_sysconfdir} \
    -DLIB_INSTALL_DIR=%{_libdir} \
    -DINCLUDE_INSTALL_DIR=%{_includedir} \
    -DDATA_INSTALL_DIR=%{_datadir} \
    -DPKGCONFIG_DIR=%{_libdir}/pkgconfig \
    -DPREFIX=%{_prefix} \
    -DNAME=%{name} \
    -DVERSION=%{version} \
    -DREVISION=${COMMIT_ID} \
    -DPLATFORM=tizen \
    -DCMAKE_BUILD_TYPE=%{buildtype} \
    -DTEST_TIMEOUT=20 \
    -DCOVERAGE=0%{?gcov} \
    -DSTDOUT_LOG=%{?stdlog:1}%{!?stdlog:0}

%__make %{?_smp_mflags}

%install
cd build
%make_install

%check
set +e
set +o pipefail
echo "================================ START"

# Run unittest
export CTEST_OUTPUT_ON_FAILURE=1
cd build
%__make test
if [ -f default.xml ]; then
    cat default.xml
fi
cd ..

%if 0%{?gcov}
# Extract coverage information
chmod 755 %{SOURCE2002}
#%{SOURCE2002} build
lcov -c --ignore-errors graph --no-external -b . -d . -o %{name}.info
genhtml %{name}.info -o out --legend --show-details
echo "================================"
%endif

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%manifest %{name}.manifest
%license LICENSE.APLv2
%defattr(-,root,root,-)
%{_libdir}/libbeyond.so*

%files devel
%defattr(-,root,root,-)
%{_includedir}/beyond/private/*.h
%{_includedir}/beyond/platform/beyond_platform.h
%{_libdir}/pkgconfig/beyond.pc

%files tizen-capi
%manifest %{name}-tizen-capi.manifest
%license LICENSE.APLv2
%defattr(-,root,root,-)
%{_libdir}/libbeyond-tizen-capi.so*

%files common-devel
%defattr(-,root,root,-)
%{_includedir}/beyond/common.h
%{_libdir}/pkgconfig/beyond-common.pc

%files tizen-capi-devel
%{_includedir}/beyond/beyond.h
%{_includedir}/beyond/discovery.h
%{_includedir}/beyond/inference.h
%{_includedir}/beyond/peer.h
%{_includedir}/beyond/runtime.h
%{_includedir}/beyond/authenticator.h
%{_libdir}/pkgconfig/beyond-tizen-capi.pc

%files plugins
%manifest %{name}-plugins.manifest
%defattr(-,root,root,-)
%license LICENSE.APLv2
%{_libdir}/libbeyond-peer_nn.so*
%{_libdir}/libbeyond-runtime_tflite.so*
%{_libdir}/libbeyond-discovery_dns_sd.so*
%{_libdir}/libbeyond-authenticator_ssl.so*

%files plugins-devel
%{_includedir}/beyond/plugin/peer_nn_plugin.h
%{_includedir}/beyond/plugin/authenticator_ssl_plugin.h
%{_includedir}/beyond/plugin/discovery_dns_sd_plugin.h
%{_includedir}/beyond/plugin/runtime_tflite_plugin.h
%{_libdir}/pkgconfig/beyond-peer_nn.pc
%{_libdir}/pkgconfig/beyond-authenticator_ssl.pc
%{_libdir}/pkgconfig/beyond-discovery_dns_sd.pc
%{_libdir}/pkgconfig/beyond-runtime_tflite.pc

%files samples
%manifest %{name}-samples.manifest
%defattr(-,root,root,-)
%license LICENSE.APLv2
%{_bindir}/sample-tizen_capi

%files mock-devel
%{_includedir}/beyond/mock/mock.h
%{_includedir}/beyond/mock/mock_ctrl.h
%{_libdir}/pkgconfig/beyond-mock.pc

%files mock
%manifest %{name}-mock.manifest
%defattr(-,root,root,-)
%license LICENSE.APLv2
%{_libdir}/libbeyond-mock.so*

%files tools
%manifest %{name}-tools.manifest
%defattr(-,root,root,-)
%license LICENSE.APLv2
%{_bindir}/beyond_eval
%{_bindir}/beyond_eval_diff
%{_bindir}/beyond-test
%{_bindir}/ut_discovery_dns_sd
%{_bindir}/beyond-peer_nn-test
%{_bindir}/beyond-tizen-capi-test
%{_bindir}/beyond-authenticator_ssl-test

%clean
rm -rf %{buildroot}
