/*
 * Copyright (C) 2026 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <variant>
#include <vector>

// ── Include the source under test ─────────────────────────────────────────────
// Rename 'main' before pulling in the TU so the linker sees only one main().
// The renamed symbol is never called; the pragma suppresses the resulting
// unused-function diagnostic.
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define main poe_tool_original_main_unused
#include "../src/POETool.cpp"  // NOLINT(build/include)
#undef main
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif


// Run parse_arg() while suppressing any stdout/stderr output (usage messages,
// error strings) so that GTest output stays clean.
static std::variant<ParsedOptions, int>
runParse(std::initializer_list<const char*> args)
{
    std::vector<const char*> argv(args);
    std::ostringstream sink;
    std::streambuf* oldCout = std::cout.rdbuf(sink.rdbuf());
    std::streambuf* oldCerr = std::cerr.rdbuf(sink.rdbuf());
    auto result = parse_arg(static_cast<int>(argv.size()), argv.data());
    std::cout.rdbuf(oldCout);
    std::cerr.rdbuf(oldCerr);
    return result;
}

TEST(ParseArgTest, NoArgs_PrintsHelpReturnsZero)
{
    auto r = runParse({"poe-gen-tool"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 0);
}

TEST(ParseArgTest, LongHelpFlag_ReturnsZero)
{
    auto r = runParse({"poe-gen-tool", "--help"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 0);
}

TEST(ParseArgTest, ShortHelpFlag_ReturnsZero)
{
    auto r = runParse({"poe-gen-tool", "-h"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 0);
}

TEST(ParseArgTest, VerboseWithNoCommand_ReturnsUsageError)
{
    auto r = runParse({"poe-gen-tool", "--verbose"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 2);
}

TEST(ParseArgTest, VersionCommand_ReturnsParsedOptions)
{
    auto r = runParse({"poe-gen-tool", "version"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    EXPECT_EQ(std::get<ParsedOptions>(r).command, Command::kVersion);
    EXPECT_FALSE(std::get<ParsedOptions>(r).verbFlag);
}

TEST(ParseArgTest, VersionWithExtraArg_ReturnsUsageError)
{
    auto r = runParse({"poe-gen-tool", "version", "unexpected"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 2);
}

TEST(ParseArgTest, VerboseFlagBeforeVersion_PropagatesVerbose)
{
    auto r = runParse({"poe-gen-tool", "--verbose", "version"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    const auto& opts = std::get<ParsedOptions>(r);
    EXPECT_EQ(opts.command, Command::kVersion);
    EXPECT_TRUE(opts.verbFlag);
}

TEST(ParseArgTest, ShortVerboseFlagBeforeVersion_PropagatesVerbose)
{
    auto r = runParse({"poe-gen-tool", "-v", "version"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    EXPECT_TRUE(std::get<ParsedOptions>(r).verbFlag);
}

TEST(ParseArgTest, ExtractHelp_ReturnsZero)
{
    auto r = runParse({"poe-gen-tool", "extract", "--help"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 0);
}

TEST(ParseArgTest, ExtractNoType_ReturnsUsageError)
{
    auto r = runParse({"poe-gen-tool", "extract", "file.hex"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 2);
}

TEST(ParseArgTest, ExtractTypeWithoutFile_ReturnsUsageError)
{
    auto r = runParse({"poe-gen-tool", "extract", "--type", "pm"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 2);
}

TEST(ParseArgTest, ExtractUnknownType_ReturnsUsageError)
{
    auto r = runParse({"poe-gen-tool", "extract", "--type", "invalid", "file"});
    ASSERT_TRUE(std::holds_alternative<int>(r));
    EXPECT_EQ(std::get<int>(r), 2);
}

TEST(ParseArgTest, ExtractTypePm_ReturnsParsedOptions)
{
    auto r = runParse({"poe-gen-tool", "extract", "--type", "pm", "manifest.hex"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    const auto& opts = std::get<ParsedOptions>(r);
    EXPECT_EQ(opts.command,   Command::kExtract);
    EXPECT_EQ(opts.inputType, InputType::kPM);
    EXPECT_EQ(opts.fileName,  "manifest.hex");
    EXPECT_FALSE(opts.verbFlag);
}

TEST(ParseArgTest, ExtractShortTypePm_ReturnsParsedOptions)
{
    auto r = runParse({"poe-gen-tool", "extract", "-t", "pm", "manifest.hex"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    const auto& opts = std::get<ParsedOptions>(r);
    EXPECT_EQ(opts.inputType, InputType::kPM);
    EXPECT_EQ(opts.fileName,  "manifest.hex");
}

TEST(ParseArgTest, ExtractTypePckCert_ReturnsParsedOptions)
{
    auto r = runParse({"poe-gen-tool", "extract", "--type", "pck_cert", "cert.pem"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    const auto& opts = std::get<ParsedOptions>(r);
    EXPECT_EQ(opts.command,   Command::kExtract);
    EXPECT_EQ(opts.inputType, InputType::kPckCert);
    EXPECT_EQ(opts.fileName,  "cert.pem");
}

TEST(ParseArgTest, ExtractTypeQuote_ReturnsParsedOptions)
{
    auto r = runParse({"poe-gen-tool", "extract", "--type", "quote", "quote.dat"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    const auto& opts = std::get<ParsedOptions>(r);
    EXPECT_EQ(opts.command,   Command::kExtract);
    EXPECT_EQ(opts.inputType, InputType::kQuote);
    EXPECT_EQ(opts.fileName,  "quote.dat");
}

TEST(ParseArgTest, ExtractWithVerboseFlag_PropagatesVerbose)
{
    auto r = runParse({"poe-gen-tool", "--verbose", "extract", "--type", "pm", "f.hex"});
    ASSERT_TRUE(std::holds_alternative<ParsedOptions>(r));
    EXPECT_TRUE(std::get<ParsedOptions>(r).verbFlag);
}
