Name:     libQuotient-qt5
Summary:  A Qt5 library to write cross-platform clients for Matrix

%define version_major 0
%define version_minor 6
%define version_patch 0

Version:  %{version_major}.%{version_minor}.beta1
Release:  2
Group:    Development/Libraries
License:  LGPL2.1
URL:      https://github.com/quotient-im/libQuotient
Source0:  %{name}-%{version}.tar.xz
Patch0:   mer-qt5.6-hacks.patch
Requires: qt5-qtcore
Requires: qt5-qtnetwork
Requires: qt5-qtgui
Requires: qt5-qtmultimedia
# Requires: libqtolm-qt5 # FUTURE WORK
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Test)
# BuildRequires: pkgconfig(QtOlm) # FUTURE WORK
BuildRequires: cmake >= 3.1
BuildRequires: opt-gcc

%description
%{summary}.

%package devel
Summary:    Development files for Matrix Qt library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   qt5-qtmultimedia-devel
# Requires:   libqtolm-qt5-devel # FUTURE WORK

Requires: qt5-qtmultimedia-plugin-mediaservice-gstcamerabin
Requires: qt5-qtmultimedia-plugin-mediaservice-gstmediacapture
Requires: qt5-qtmultimedia-plugin-audio-alsa
Requires: qt5-qtmultimedia-plugin-mediaservice-gstaudiodecoder
Requires: qt5-qtmultimedia-plugin-mediaservice-gstmediaplayer
Requires: qt5-qtmultimedia-plugin-playlistformats-m3u
Requires: qt5-qtmultimedia-plugin-audio-pulseaudio
Requires: qt5-qtmultimedia-plugin-resourcepolicy-resourceqt

BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstcamerabin
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstmediacapture
BuildRequires: qt5-qtmultimedia-plugin-audio-alsa
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstaudiodecoder
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstmediaplayer
BuildRequires: qt5-qtmultimedia-plugin-playlistformats-m3u
BuildRequires: qt5-qtmultimedia-plugin-audio-pulseaudio
BuildRequires: qt5-qtmultimedia-plugin-resourcepolicy-resourceqt

%description devel
%{summary}.

%prep
%setup -q
%patch0 -p1

%build
%define cmake_build %__cmake --build .
%define cmake_install DESTDIR=%{?buildroot} %__cmake --build . --target install


%cmake \
    -DCMAKE_CXX_COMPILER=/opt/gcc/bin/g++ \
    -DCMAKE_SHARED_LINKER_FLAGS="-L/opt/gcc/lib -static-libstdc++" \
    -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir}/%{name} \
    -DBUILD_SHARED_LIBS=ON \
    -DUSE_INTREE_LIBQOLM=OFF

%cmake_build

%install
rm -rf %{buildroot}
%cmake_install
rm %{buildroot}/usr/share/ndk-modules/Android.mk
mkdir -p %{buildroot}%{_bindir}
install -m 755 quotest %{buildroot}%{_bindir}/quotest

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libQuotient.so.%{version_major}.%{version_minor}.%{version_patch}
%{_libdir}/libQuotient.so.%{version_major}.%{version_minor}
%{_libdir}/libQuotient.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}
%{_libdir}/libQuotient.so
%{_libdir}/pkgconfig/Quotient.pc
%{_libdir}/cmake/Quotient
%{_bindir}/quotest
