/*
 * Copyright(c) 2025-2026 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** File: utility.h
 *
 * Description: Definitions of some utility functions
 *
 */

#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace intel {
namespace tools {
namespace poe {

[[nodiscard]] std::optional<std::string> retrievePIIDAndPRIDsFromPM(std::string_view filename);

[[nodiscard]] std::optional<std::string> retrievePIIDFromCert(std::string_view filename);

[[nodiscard]] std::optional<std::string> retrievePIIDFromQuote(std::string_view filename);

}  // namespace poe
}  // namespace tools
}  // namespace intel
