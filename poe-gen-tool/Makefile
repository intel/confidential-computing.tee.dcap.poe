#
# Copyright(c) 2025-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Compiler detection
define check_compiler
    $(shell $(1) -dM -E - < /dev/null 2>/dev/null | grep -q $(2) && echo 1 || echo 0)
endef

IS_CLANG := $(call check_compiler,$(CXX),__clang__)
IS_GCC := $(call check_compiler,$(CXX),__GNUC__)

ifeq ($(IS_CLANG), 1)
    # For Clang: parse --version
    CXX_VERSION_FULL := $(shell $(CXX) --version | head -n1)
    CXX_VERSION := $(shell echo "$(CXX_VERSION_FULL)" | sed -n 's/.*clang version \([0-9][0-9]*\.[0-9][0-9]*\).*/\1/p')
else
    # For GCC: -dumpversion
    CXX_VERSION := $(shell $(CXX) -dumpversion)
endif

CXX_VERSION_MAJOR := $(shell echo $(CXX_VERSION) | cut -f1 -d.)
CXX_VERSION_MINOR := $(shell echo $(CXX_VERSION) | cut -f2 -d.)
CXX_NO_LESS_THAN_7 := $(shell [ $(CXX_VERSION_MAJOR) -ge 7 ] && echo 1 || echo 0)
CXX_NO_LESS_THAN_8 := $(shell [ $(CXX_VERSION_MAJOR) -ge 8 ] && echo 1 || echo 0)
CXX_NO_LESS_THAN_9 := $(shell [ $(CXX_VERSION_MAJOR) -ge 9 ] && echo 1 || echo 0)
CXX_NO_LESS_THAN_11 := $(shell [ $(CXX_VERSION_MAJOR) -ge 11 ] && echo 1 || echo 0)
CXX_NO_LESS_THAN_12 := $(shell [ $(CXX_VERSION_MAJOR) -ge 12 ] && echo 1 || echo 0)


ifeq ($(IS_GCC)$(CXX_NO_LESS_THAN_12), 11)
    FORTIFY_SOURCE_VAL:= 3
else ifeq ($(IS_CLANG)$(CXX_NO_LESS_THAN_9), 11)
    FORTIFY_SOURCE_VAL:= 3
else
    FORTIFY_SOURCE_VAL:= 2
endif


# turn on stack protector for SDK
CXX_BELOW_4_9 := $(shell expr "`$(CXX) -dumpversion`" \< "4.9")
ifeq ($(CXX_BELOW_4_9), 1)
    COMMON_FLAGS += -fstack-protector
else
    COMMON_FLAGS += -fstack-protector-strong
endif

ifdef DEBUG
    COMMON_FLAGS += -O0 -ggdb -DDEBUG -UNDEBUG
else
    COMMON_FLAGS += -O2 -D_FORTIFY_SOURCE=$(FORTIFY_SOURCE_VAL) -UDEBUG -DNDEBUG
endif


CET_FLAGS :=
ifeq ($(CXX_NO_LESS_THAN_8), 1)
    CET_FLAGS += -fcf-protection
endif
COMMON_FLAGS += $(CET_FLAGS)

COMMON_FLAGS += -ffunction-sections -fdata-sections

# turn on compiler warnings as much as possible
COMMON_FLAGS += -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type \
                -Waddress -Wsequence-point -Wformat-security \
                -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
                -Wcast-align -Wconversion -Wredundant-decls
                
# Third-party directory
THIRD_PARTY := third_party
CXXOPTS_DIR := $(THIRD_PARTY)/cxxopts
CXXOPTS_HPP := $(CXXOPTS_DIR)/include/cxxopts.hpp

# Default target
.PHONY: all
all: $(TARGET)

# Download cxxopts if not exists
$(CXXOPTS_HPP):
	@echo "Downloading cxxopts..."
	@mkdir -p $(dir $@)
	@cd $(THIRD_PARTY) && \
		rm -rf cxxopts && \
		git clone --depth=1 --branch=master https://github.com/jarro2783/cxxopts.git cxxopts && \
		cd cxxopts && \
		git checkout 11f26227ef8e951bdad48aeaa6537f1c5d8e9e0a && \
		echo "cxxopts downloaded at commit 11f26227ef8e951bdad48aeaa6537f1c5d8e9e0a successfully."
######## App Settings ########

App_Cpp_Files := $(wildcard  src/*.cpp)
App_Include_Paths :=  -I ./src/inc
App_Include_Paths +=  -I ./third_party/cxxopts/include

App_C_Flags := $(COMMON_FLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

App_Cpp_Flags := $(App_C_Flags) -std=c++20
App_Link_Flags := -Wl,-z,relro,-z,now,-z,noexecstack
App_Link_Flags +=  -lcrypto 


App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_Name := poe-gen-tool


.PHONY: all 

all: $(App_Name) 


######## App Objects ########

src/%.o: src/%.cpp $(CXXOPTS_HPP)
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(App_Name):  $(App_Cpp_Objects)
	$(CXX) $^ $(App_Link_Flags) -o $@
	echo "LINK =>  $@"


.PHONY: clean

clean:
	@rm -f $(App_Name)  $(App_Cpp_Objects) 


# Distclean (remove downloaded dependencies)
.PHONY: distclean
distclean: clean
	rm -rf $(THIRD_PARTY)

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all       - Build the executable"
	@echo "  clean     - Clean object files and executable"
	@echo "  distclean - Clean everything, including downloaded dependencies"
	@echo "  help      - Show this help"