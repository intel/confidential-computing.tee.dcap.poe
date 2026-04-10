#!/usr/bin/env bash
#
#  Copyright(c) 2025-2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


set -euo pipefail

SCRIPT_DIR=$(dirname "$0")
COMMON_DIR="${SCRIPT_DIR}/../../common/intel-tee-poe-gen-tool"

rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.deb
rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*dbgsym*.ddeb
rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.tar.gz
rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.tar.xz
rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.dsc
rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.changes
rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.buildinfo
rm -f ${COMMON_DIR}/gen_source.py
rm -rf ${COMMON_DIR}/output
