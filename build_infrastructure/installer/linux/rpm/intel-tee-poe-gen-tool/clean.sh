#!/usr/bin/env bash
#
#  Copyright(c) 2025-2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


set -euo pipefail

SCRIPT_DIR=$(dirname "$0")
OUTPUT_DIR="${SCRIPT_DIR}/../../common/intel-tee-poe-gen-tool"

rm -f ${SCRIPT_DIR}/intel-tee-poe-gen-tool*.rpm
rm -rf ${OUTPUT_DIR}/output
rm -f ${OUTPUT_DIR}/gen_source.py
