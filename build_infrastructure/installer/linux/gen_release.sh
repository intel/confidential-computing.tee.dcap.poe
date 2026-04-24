#!/usr/bin/env bash
#
# Copyright(c) 2025-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

set -euo pipefail

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR=$(cd "${SCRIPT_DIR}/../../.." && pwd)

POE_GEN_TOOL_VERSION=${1:-"9.9.9.9"}

OUTPUT_DIR=${ROOT_DIR}/output
mkdir -p "${OUTPUT_DIR}"
cd "${OUTPUT_DIR}"

rel_dir_base=intel_tee_poe-gen-tool
rel_dir_name=${rel_dir_base}_v${POE_GEN_TOOL_VERSION}

rm -rf "${rel_dir_base}"*
pushd "${ROOT_DIR}"
make clean
make 
popd

mkdir "${rel_dir_name}"
if [ ! -f "${ROOT_DIR}/poe-gen-tool/build/bin/poe-gen-tool" ]; then
    echo "ERROR: poe-gen-tool not found"
    exit 1
fi
cp "${ROOT_DIR}/poe-gen-tool/build/bin/poe-gen-tool" "${rel_dir_name}"
cp "${ROOT_DIR}/poe-gen-tool/README.md" "${rel_dir_name}"
cp "${ROOT_DIR}/License.txt" "${rel_dir_name}"

# Set restrictive permissions: executable 0755, docs 0644
chmod 0755 "${rel_dir_name}/poe-gen-tool"
chmod 0644 "${rel_dir_name}/README.md" "${rel_dir_name}/License.txt"

tar cvpzf "${rel_dir_name}.tar.gz" "${rel_dir_name}" --remove-files
rmdir "${rel_dir_name}" 2>/dev/null || true

exit 0

