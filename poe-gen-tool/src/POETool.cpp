/*
 * Copyright(c) 2025-2026 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * File: POETool.cpp
 * generate the raw data for PIID/PRID retrieval
 */

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "utility.h"

using namespace intel::tools::poe;

constexpr std::string_view VER_FILE_DESCRIPTION_STR =
    "Intel(R): this tool is used to collect the platform instance ID, then to generate the "
    "platform ownership endorsement";
constexpr std::string_view VER_PRODUCTNAME_STR = "poe-gen-tool";

#ifndef POE_VERSION
#define POE_VERSION "9.9.9.9-dev"
#endif
constexpr std::string_view STRPRODUCTVER = POE_VERSION;

enum class Command {
    kNone,
    kExtract,
    kVersion,
};

enum class InputType {
    kNone,
    kPM,
    kPckCert,
    kQuote,
};

struct ParsedOptions {
    InputType inputType = InputType::kNone;
    Command command = Command::kNone;
    std::string fileName;
    bool verbFlag = false;
};

// Allows cxxopts to parse the --type parameter as enum
std::istream& operator>>(std::istream& in, InputType& type) {
    std::string token;
    in >> token;

    if (token == "pm") {
        type = InputType::kPM;
    } else if (token == "pck_cert") {
        type = InputType::kPckCert;
    } else if (token == "quote") {
        type = InputType::kQuote;
    } else {
        in.setstate(std::ios::failbit);
    }

    return in;
}

// Allows cxxopts to parse the <command> param as enum
std::istream& operator>>(std::istream& in, Command& command) {
    std::string token;
    in >> token;

    if (token == "extract") {
        command = Command::kExtract;
    } else if (token == "version") {
        command = Command::kVersion;
    } else {
        in.setstate(std::ios::failbit);
    }

    return in;
}

namespace {
cxxopts::Options create_root_options() {
    cxxopts::Options options(std::string(VER_PRODUCTNAME_STR),
                             "Intel(R) Platform Ownership Endorsement Generator");

    options.custom_help("[--verbose] <command> [<args>]");
    options.add_options()
        ("v,verbose", "Show what is being done")
        ("h,help", "Show command help")
        ("command", "Subcommand to run: extract | version",
         cxxopts::value<Command>());

    options.parse_positional({"command"});
    options.positional_help("");  // suppress "positional parameters" suffix in help
    options.allow_unrecognised_options();  // command-specific options will go here
    return options;
}

// Subparser for the "extract" command (delta on top of global params).
// Once usage grows more complex, a full subcommand-like parser can be used.
cxxopts::Options create_extract_options() {
    cxxopts::Options options(std::string(VER_PRODUCTNAME_STR),
                             "Intel(R) Platform Ownership Endorsement Generator");

    options.custom_help("extract --type <pm|pck_cert|quote> <input_file>");
    options.add_options()
        ("h,help", "Show this help message")
        ("t,type",
         "Type of the input_file (required). One of:\n"
         "  pm       : for platform manifest (Base16 format)\n"
         "  pck_cert : for PCK certificate (PEM format)\n"
         "  quote    : for quote",
         cxxopts::value<InputType>())
        ("input_file", "Input file path", cxxopts::value<std::string>());

    options.parse_positional({"input_file"});
    options.positional_help("");  // suppress "positional parameters" suffix in help
    return options;
}

// Stock help with extra examples
void show_root_usage(const cxxopts::Options& options) {
    std::cout << options.help() << "\n";
    std::cout << "Commands:\n";
    std::cout << "  extract   Extract PIID/PRID from pm, pck_cert, or quote\n\n";
    std::cout << "  version   Show tool version\n\n";
    std::cout << "Global options:\n";
    std::cout << "  --verbose can be placed before or after <command>\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --help\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type pm        platform_manifest.bin\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type pck_cert  pck_cert.pem\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type quote     quote.dat\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " version\n\n";
}

// Extract subcommand help with detailed examples
void show_extract_usage(const cxxopts::Options& options) {
    std::cout << options.help() << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type pm platform_manifest.bin\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type pm platform_manifest.bin --verbose\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type pck_cert pck_cert.pem\n";
    std::cout << "  " << VER_PRODUCTNAME_STR << " extract --type quote quote.dat\n\n";
    std::cout << "Global options:\n"
              << "  --verbose, -v   Show what is being done (can be placed before or after 'extract')\n\n";
    std::cout << "Notes:\n"
              << "  At the same time, you can only provide one and only one type for these "
                 "three types: pm, pck_cert, quote.\n";
}

}  // namespace

// Returns ParsedOptions on success, or the exit code on error/early-exit.
std::variant<ParsedOptions, int> parse_arg(int argc, const char* argv[]) {
    auto rootOptions = create_root_options();

    if (argc == 1) {
        show_root_usage(rootOptions);
        return 0;
    }

    cxxopts::ParseResult rootResult;
    try {
        // Pass 1: parse only global options + subcommand. Unknown tokens are kept in
        // rootResult.unmatched() for subcommand-specific parsing in pass 2.
        rootResult = rootOptions.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << '\n';
        show_root_usage(rootOptions);
        return 2;
    }

    ParsedOptions opts;
    opts.verbFlag = rootResult.count("verbose") > 0;

    if (rootResult.count("help") && !rootResult.count("command")) {
        show_root_usage(rootOptions);
        return 0;
    }

    if (!rootResult.count("command")) {
        std::cerr << "Error: Missing command. Supported commands: extract, version.\n";
        show_root_usage(rootOptions);
        return 2;
    }

    opts.command = rootResult["command"].as<Command>();

    switch (opts.command) {
        case Command::kVersion:
            if (rootResult.count("help")) {
                show_root_usage(rootOptions);
                return 0;
            }
            if (!rootResult.unmatched().empty()) {
                std::cerr << "Error: 'version' does not take additional arguments.\n";
                show_root_usage(rootOptions);
                return 2;
            }
            return opts;

        case Command::kExtract: {
            auto extractOptions = create_extract_options();

            // Short-circuit: show extract help before any pass-2 parsing so that
            // help is always shown cleanly, even when other args are invalid.
            if (rootResult.count("help")) {
                show_extract_usage(extractOptions);
                return 0;
            }

            const auto& unmatched = rootResult.unmatched();

            std::vector<const char*> extractArgv;
            extractArgv.reserve(unmatched.size() + 1);
            extractArgv.push_back(argv[0]);  // program name for help/version display
            for (const auto& arg : unmatched) {
                extractArgv.push_back(arg.c_str());
            }

            try {
                // Pass 2: re-parse only subcommand tail with extract-specific schema.
                auto extractResult =
                    extractOptions.parse(static_cast<int>(extractArgv.size()), extractArgv.data());

                if (extractResult.count("type") > 1) {
                    std::cerr << "Error: --type/-t specified multiple times.\n";
                    return 2;
                }

                if (!extractResult.count("type")) {
                    std::cerr << "Error: --type is required for 'extract'\n";
                    show_extract_usage(extractOptions);
                    return 2;
                }

                opts.inputType = extractResult["type"].as<InputType>();

                if (!extractResult.count("input_file")) {
                    std::cerr << "Error: Missing input file\n";
                    show_extract_usage(extractOptions);
                    return 2;
                }

                opts.fileName = extractResult["input_file"].as<std::string>();
            } catch (const cxxopts::exceptions::exception& e) {
                std::cerr << "Error parsing options: " << e.what() << '\n';
                show_extract_usage(extractOptions);
                return 2;
            }
            return opts;
        }

        default:
            std::cerr << "Error: Unknown command. Supported commands: extract, version.\n";
            show_root_usage(rootOptions);
            return 2;
    }
}

int main(int argc, const char* argv[]) {
    try {
        int ret = 1;  // 1 = operational/runtime failure; 0 = success; 2 = usage error

        // parse the command options
        auto parseResult = parse_arg(argc, argv);
        if (auto* exitCode = std::get_if<int>(&parseResult)) {
            return *exitCode;
        }
        const auto& opts = std::get<ParsedOptions>(parseResult);

        if (opts.command == Command::kVersion) {
            std::cout << STRPRODUCTVER << '\n';
            return 0;
        }

        if (opts.command != Command::kExtract) {
            std::cerr << "Error: Unsupported command. Supported commands: extract, version.\n";
            return 2;
        }

        if (opts.verbFlag) {
            std::cout << "\n" << VER_FILE_DESCRIPTION_STR << ". Version " << STRPRODUCTVER << "\n\n";
            std::cout << "[INFO] Parameters:\n";
            switch (opts.inputType) {
                case InputType::kPM:
                    std::cout << "  platform manifest file:   " << opts.fileName << "\n\n";
                    break;
                case InputType::kPckCert:
                    std::cout << "  PCK Certificate:          " << opts.fileName << "\n\n";
                    break;
                case InputType::kQuote:
                    std::cout << "  quote file:               " << opts.fileName << "\n\n";
                    break;
                default:
                    break;
            }
        }

        switch (opts.inputType) {
            case InputType::kPM: {
                std::optional<std::string> piidAndPridsStr =
                    retrievePIIDAndPRIDsFromPM(opts.fileName);
                if (piidAndPridsStr) {
                    std::cout << *piidAndPridsStr << '\n';
                    ret = 0;
                    if (opts.verbFlag) {
                        std::cout << "[OK] Retrieved PIID and PRIDs from platform manifest successfully.\n";
                    }
                } else {
                    std::cerr << "[ERROR]: Can NOT retrieve PIID and PRIDs from platform manifest.\n";
                }
                break;
            }
            case InputType::kPckCert: {
                std::optional<std::string> piidStr = retrievePIIDFromCert(opts.fileName);
                if (piidStr) {
                    std::cout << *piidStr << '\n';
                    ret = 0;
                    if (opts.verbFlag) {
                        std::cout << "[OK] Retrieved the PIID from PCK certificate successfully.\n";
                    }
                } else {
                    std::cerr << "[ERROR]: Can NOT retrieve piid from PCK certificate.\n";
                }
                break;
            }
            case InputType::kQuote: {
                std::optional<std::string> piidStr = retrievePIIDFromQuote(opts.fileName);
                if (piidStr) {
                    std::cout << *piidStr << '\n';
                    ret = 0;
                    if (opts.verbFlag) {
                        std::cout << "[OK] Retrieved the PIID from Quote successfully.\n";
                    }
                } else {
                    std::cerr << "[ERROR]: Can NOT retrieve piid from Quote.\n";
                }
                break;
            }
            default:
                std::cerr << "[ERROR]: Input file was NOT provided.\n";
                break;
        }
        return ret;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR]: Unexpected exception: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "[ERROR]: Unknown unexpected exception.\n";
        return 1;
    }
}
