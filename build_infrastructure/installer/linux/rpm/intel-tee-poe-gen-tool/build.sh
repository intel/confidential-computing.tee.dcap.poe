#!/usr/bin/env bash
#
#  Copyright(c) 2025-2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -euo pipefail

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR="${SCRIPT_DIR}/../.."
LINUX_INSTALLER_COMMON_DIR="${ROOT_DIR}/common"
LINUX_INSTALLER_COMMON_PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_DIR="${LINUX_INSTALLER_COMMON_DIR}/intel-tee-poe-gen-tool"

POE_GEN_TOOL_VERSION=${1:-"9.9.9.9"}
export PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_VERSION=${POE_GEN_TOOL_VERSION}

# RPM Version: field forbids hyphens; replace with tildes (pre-release convention).
RPM_VERSION=$(echo "${POE_GEN_TOOL_VERSION}" | tr '-' '~')

source ${LINUX_INSTALLER_COMMON_PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_DIR}/installConfig
RPM_BUILD_FOLDER=${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}-${POE_GEN_TOOL_VERSION}
# Source tarball name must match Source0: %{name}-%{version}.tar.gz in the spec.
RPM_SOURCE_NAME="${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}-${RPM_VERSION}"

main() {
    pre_build
    update_spec
    create_upstream_tarball
    build_rpm_package
    post_build
}

pre_build() {
    rm -fR ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}
    mkdir -p ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
    cp -f ${SCRIPT_DIR}/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}.spec ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}/SPECS
}

post_build() {
        RPMS=$(find ${SCRIPT_DIR}/${RPM_BUILD_FOLDER} -name "*.rpm" 2> /dev/null)
        [ -z "${RPMS}" ] || cp ${RPMS} ${SCRIPT_DIR}
        rm -fR ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}
}

update_spec() {
    min_version="4.12"
    rpm_version=$(rpmbuild --version 2> /dev/null | awk '{print $NF}')
    cur_version=$(echo -e "${rpm_version}\n${min_version}" | sort -V | head -n 1)
    
	pushd ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}
    sed -i "s#@version@#${RPM_VERSION}#" SPECS/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}.spec
    sed -i "s#@install_path@#${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_PATH}/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}#" SPECS/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}.spec
    sed -i "s#@date@#$(date +'%a %b %d %Y')#" SPECS/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}.spec
    if [ "${min_version}" != "${cur_version}" ]; then
        sed -i "s/^Recommends:/Requires:  /" SPECS/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}.spec
    fi
    popd
}

create_upstream_tarball() {
    ${LINUX_INSTALLER_COMMON_PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_DIR}/createTarball.sh

    tar -xvf ${LINUX_INSTALLER_COMMON_PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_DIR}/output/${TARBALL_NAME} -C ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}/SOURCES
    pushd ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}/SOURCES
    # change the install path to /usr/share instead of /opt/intel if the OS is clear linux
    sed -i "s#\(PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_PATH=\).*#\1${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_PATH}#" installConfig
    tar -zcvf ${RPM_SOURCE_NAME}$(echo ${TARBALL_NAME}|awk -F'.' '{print "."$(NF-1)"."$(NF)}') *
    popd
}

build_rpm_package() {
    pushd ${SCRIPT_DIR}/${RPM_BUILD_FOLDER}
    rpmbuild --define="_topdir `pwd`" -bb SPECS/${PLATFORM_OWNERSHIP_ENDORSEMENT_TOOL_PACKAGE_NAME}.spec
    popd
}

main $@