#!/usr/bin/env bash
#
#  Copyright(c) 2025-2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
#


set -euo pipefail

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR="${SCRIPT_DIR}/.."
LINUX_INSTALLER_COMMON_DIR="${ROOT_DIR}"

INSTALL_PATH=${SCRIPT_DIR}/output

# Cleanup
rm -fr ${INSTALL_PATH}

# Get the configuration for this package
source ${SCRIPT_DIR}/installConfig
# Fetch the gen_source script
cp ${LINUX_INSTALLER_COMMON_DIR}/gen_source/gen_source.py ${SCRIPT_DIR}
# Copy the files according to the BOM
python ${SCRIPT_DIR}/gen_source.py --bom=BOMs/platform-ownership-endorsement-tool.txt --installdir=pkgroot/intel-tee-poe-gen-tool
python ${SCRIPT_DIR}/gen_source.py --bom=BOMs/platform-ownership-endorsement-tool-package.txt --cleanup=false


# Create the tarball
pushd ${INSTALL_PATH} &> /dev/null
tar -zcvf ${TARBALL_NAME} *
popd &> /dev/null
