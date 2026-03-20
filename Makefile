#
# Copyright(c) 2025-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(DEBUG),1)
    CMAKE_BUILD_TYPE := Debug
else ifeq ($(RELEASE),1)
    CMAKE_BUILD_TYPE := Release
else
    CMAKE_BUILD_TYPE ?= Release
endif


.PHONY: build poe_gen_tool
build: poe_gen_tool

poe_gen_tool:
	@cmake -S poe_gen_tool -B poe_gen_tool/build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) || { echo "CMake configure failed!"; exit 1; }
	@cmake --build poe_gen_tool/build || { echo "Build failed!"; exit 1; }



.PHONY: clean
clean: clean_poe_gen_tool

clean_poe_gen_tool:
	@cmake --build poe_gen_tool/build --target clean || { echo "Clean failed!"; exit 1; }



.PHONY: distclean
distclean: clean
	@echo "🧹 Removing CMake cache and generated files..."
	@cd poe_gen_tool && \
	@rm -rf build && \
	echo " Distclean completed!"|| \
	{ echo "Distclean failed!"; exit 1; }


	