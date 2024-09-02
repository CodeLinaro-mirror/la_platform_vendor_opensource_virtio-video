# If kversion isn't defined on the rpmbuild line, define it here.
%{!?kversion: %define kversion %(uname -r)}

Name: virtio-video-headers
Version: 1.0
Release: r0
Summary: install virtio video uapi headers
BuildArch: noarch
License: GPL-2.0-only WITH Linux-syscall-note
Source0: %{name}-%{version}.tar.gz

BuildRequires: kernel-automotive-devel-uname-r = %{kversion}

%description
This contains virtio video headers used by the virtio backend

%prep
%setup -qn %{name}

KERNEL_SRC="/usr/src/kernels"
CURDIR=${PWD}
cd ${KERNEL_SRC}/%{kversion}/
scripts/headers_install.sh ${CURDIR}/virtio_video.h ${CURDIR}/virtio_video.h
scripts/headers_install.sh ${CURDIR}/virtio_video_msm_ext.h ${CURDIR}/virtio_video_msm_ext.h
scripts/headers_install.sh ${CURDIR}/virtio_video_hw_virt.h ${CURDIR}/virtio_video_hw_virt.h

%install
mkdir -p "$RPM_BUILD_ROOT/usr/include"
cp virtio_video.h $RPM_BUILD_ROOT/usr/include/
cp virtio_video_msm_ext.h $RPM_BUILD_ROOT/usr/include/
cp virtio_video_hw_virt.h $RPM_BUILD_ROOT/usr/include/

%files
%{_includedir}/virtio_video.h
%{_includedir}/virtio_video_msm_ext.h
%{_includedir}/virtio_video_hw_virt.h
