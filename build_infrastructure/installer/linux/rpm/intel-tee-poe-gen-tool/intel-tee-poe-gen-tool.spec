#
#  Copyright(c) 2025-2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

%define _install_path @install_path@
%define _tool_name poe-gen-tool

Name:           intel-tee-poe-gen-tool
Provides:       poe-gen-tool
Version:        @version@
Release:        1%{?dist}
Summary:        Intel(R) Platform Ownership Endorsement(POE) Tool 
Group:          Development/System
#Recommends:    

License:        BSD License
URL:            https://github.com/intel/confidential-computing.tee.dcap.poe
Source0:        %{name}-%{version}.tar.gz


%description
This tool is used to collect the platform instance ID, then to generate the platform ownership endorsement

%prep
%setup -qc

%install
make DESTDIR=%{?buildroot} install
echo "%{_install_path}" > %{_specdir}/list-%{name}
find %{?buildroot} | sort | \
awk '$0 !~ last "/" {print last} {last=$0} END {print last}' | \
sed -e "s#^%{?buildroot}##" | \
grep -v "^%{_install_path}" >> %{_specdir}/list-%{name} || :

%files -f %{_specdir}/list-%{name}

%define debug_package %{nil}

%posttrans
################################################################################
# Set up platform ownership endorsment tool                                         #
################################################################################

# Install the PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL 
ln -s -f %{_install_path}/%{_tool_name} /usr/local/bin/%{_tool_name}
retval=$?

if test $retval -ne 0; then
    echo "failed to install $PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_NAME."
    exit 6
fi

printf 'Installation succeed!\n'

%postun

# Only perform cleanup on package erase (arg 0), not on upgrade
if [ "$1" -eq 0 ]; then

# Removing PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL soft link file
rm -f /usr/local/bin/%{_tool_name}

    printf 'Uninstallation succeed!\n'
fi

%changelog
*  @date@ Intel Confidential Computing Team <confidential.computing@intel.com> - @version@-1
- Initial Release
