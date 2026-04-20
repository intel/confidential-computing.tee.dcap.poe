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

# Single source of truth for the default version used by both CMake and
# packaging scripts. When POE_VERSION is provided by the caller (e.g. CI),
# that value is used instead.
POE_DEFAULT_VERSION := 9.9.9.9-dev
POE_GEN_TOOL_VERSION ?= $(if $(POE_VERSION),$(POE_VERSION),$(POE_DEFAULT_VERSION))

CMAKE_POE_VERSION_FLAG = -DPOE_VERSION=$(POE_GEN_TOOL_VERSION)

.PHONY: build poe-gen-tool
build: poe-gen-tool

poe-gen-tool:
	@cmake -S poe-gen-tool -B poe-gen-tool/build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		$(CMAKE_POE_VERSION_FLAG) || { echo "CMake configure failed!"; exit 1; }
	@cmake --build poe-gen-tool/build || { echo "Build failed!"; exit 1; }


.PHONY: deb
deb: deb_poe-gen-tool

.PHONY: rpm
rpm: rpm_poe-gen-tool

.PHONY: standalone_packages
standalone_packages: standalone_poe-gen-tool

.PHONY: deb_poe-gen-tool
deb_poe-gen-tool: poe-gen-tool
	./build_infrastructure/installer/linux/deb/intel-tee-poe-gen-tool/build.sh $(POE_GEN_TOOL_VERSION)

.PHONY: rpm_poe-gen-tool
rpm_poe-gen-tool: poe-gen-tool
	./build_infrastructure/installer/linux/rpm/intel-tee-poe-gen-tool/build.sh $(POE_GEN_TOOL_VERSION)

.PHONY: standalone_poe-gen-tool
standalone_poe-gen-tool: poe-gen-tool
	./build_infrastructure/installer/linux/gen_release.sh $(POE_GEN_TOOL_VERSION)


.PHONY: clean clean_poe-gen-tool
clean: clean_poe-gen-tool
	./build_infrastructure/installer/linux/rpm/intel-tee-poe-gen-tool/clean.sh
	./build_infrastructure/installer/linux/deb/intel-tee-poe-gen-tool/clean.sh
	@echo " Clean completed."

clean_poe-gen-tool:
	@echo " Cleaning poe-gen-tool build directory..."
	@if [ -d "poe-gen-tool/build" ]; then \
		cmake --build poe-gen-tool/build --target clean || { echo "Clean failed!"; exit 1; }; \
	else \
		echo " Build directory not found. Skipping clean."; \
	fi


.PHONY: distclean
distclean: clean
	@echo "🧹 Removing CMake cache and generated files..."
	@cd poe-gen-tool && \
	rm -rf build && \
	echo " Distclean completed!"|| \
	{ echo "Distclean failed!"; exit 1; }


	