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

struct ProgramOptions {
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

    inline static std::string fileName = "";
    inline static Command command = Command::kNone;
    inline static InputType inputType = InputType::kNone;
    inline static bool verbFlag = false;
};

// Allows cxxopts to parse the --type parameter as enum
std::istream& operator>>(std::istream& in, ProgramOptions::InputType& type) {
    std::string token;
    in >> token;

    if (token == "pm") {
        type = ProgramOptions::InputType::kPM;
    } else if (token == "pck_cert") {
        type = ProgramOptions::InputType::kPckCert;
    } else if (token == "quote") {
        type = ProgramOptions::InputType::kQuote;
    } else {
        in.setstate(std::ios::failbit);
    }

    return in;
}

// Allows cxxopts to parse the <command> param as enum
std::istream& operator>>(std::istream& in, ProgramOptions::Command& command) {
    std::string token;
    in >> token;

    if (token == "extract") {
        command = ProgramOptions::Command::kExtract;
    } else if (token == "version") {
        command = ProgramOptions::Command::kVersion;
    } else {
        in.setstate(std::ios::failbit);
    }

    return in;
}

namespace {

// Global CLI parser
cxxopts::Options create_root_options() {
    cxxopts::Options options(std::string(VER_PRODUCTNAME_STR),
                             "Platform Ownership Endorsement Generation Tool");

    options.custom_help("[--verbose] <command> [<args>]");
    options.add_options()
        ("v,verbose", "Show what is being done")
        ("h,help", "Show command help")
        ("command", "Subcommand to run: extract | version",
         cxxopts::value<ProgramOptions::Command>());

    options.parse_positional({"command"});
    options.positional_help("");  // suppress "positional parameters" suffix in help
    options.allow_unrecognised_options();  // command-specific options will go here
    return options;
}

// Subparser for the "extract" command (delta on top of global params).
// Once usage grows more complex, a full subcommand-like parser can be used.
cxxopts::Options create_extract_options() {
    cxxopts::Options options(std::string(VER_PRODUCTNAME_STR),
                             "Platform Ownership Endorsement Generation Tool");

    options.custom_help("extract --type <pm|pck_cert|quote> <input_file>");
    options.add_options()
        ("h,help", "Show this help message")
        ("t,type",
         "Type of the input_file (required). One of:\n"
         "  pm       : for platform manifest (Base16 format)\n"
         "  pck_cert : for PCK certificate (PEM format)\n"
         "  quote    : for quote",
         cxxopts::value<ProgramOptions::InputType>())
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

int parse_arg(int argc, const char* argv[]) {
    ProgramOptions::fileName.clear();
    ProgramOptions::command = ProgramOptions::Command::kNone;
    ProgramOptions::inputType = ProgramOptions::InputType::kNone;
    ProgramOptions::verbFlag = false;

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
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        show_root_usage(rootOptions);
        return 2;
    }

    if (rootResult.count("verbose")) {
        ProgramOptions::verbFlag = true;
    }

    if (rootResult.count("help") && !rootResult.count("command")) {
        show_root_usage(rootOptions);
        ProgramOptions::command = ProgramOptions::Command::kNone;
        return 0;
    }

    if (!rootResult.count("command")) {
        std::cerr << "Error: Missing command. Supported commands: extract, version.\n";
        show_root_usage(rootOptions);
        return 2;
    }

    ProgramOptions::command = rootResult["command"].as<ProgramOptions::Command>();

    switch (ProgramOptions::command) {
        case ProgramOptions::Command::kVersion:
            if (rootResult.count("help")) {
                show_root_usage(rootOptions);
                ProgramOptions::command = ProgramOptions::Command::kNone;
                return 0;
            }
            if (!rootResult.unmatched().empty()) {
                std::cerr << "Error: 'version' does not take additional arguments.\n";
                show_root_usage(rootOptions);
                return 2;
            }
            return 0;

        case ProgramOptions::Command::kExtract: {
            auto extractOptions = create_extract_options();

            // Short-circuit: show extract help before any pass-2 parsing so that
            // help is always shown cleanly, even when other args are invalid.
            if (rootResult.count("help")) {
                show_extract_usage(extractOptions);
                ProgramOptions::command = ProgramOptions::Command::kNone;
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

                ProgramOptions::inputType = extractResult["type"].as<ProgramOptions::InputType>();

                if (!extractResult.count("input_file")) {
                    std::cerr << "Error: Missing input file\n";
                    show_extract_usage(extractOptions);
                    return 2;
                }

                ProgramOptions::fileName = extractResult["input_file"].as<std::string>();
            } catch (const cxxopts::exceptions::exception& e) {
                std::cerr << "Error parsing options: " << e.what() << std::endl;
                show_extract_usage(extractOptions);
                return 2;
            }
            return 0;
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
        int parseStatus = parse_arg(argc, argv);
        if (parseStatus != 0) {
            return parseStatus;
        }

        if (ProgramOptions::command == ProgramOptions::Command::kNone) {
            return 0;
        }

        if (ProgramOptions::command == ProgramOptions::Command::kVersion) {
            std::cout << STRPRODUCTVER << std::endl;
            return 0;
        }

        if (ProgramOptions::command != ProgramOptions::Command::kExtract) {
            std::cerr << "Error: Unsupported command. Supported commands: extract, version.\n";
            return 2;
        }

        if (ProgramOptions::verbFlag) {
            std::cout << "\n" << VER_FILE_DESCRIPTION_STR << ". Version " << STRPRODUCTVER << "\n\n";
            std::cout << "[INFO] Parameters:\n";
            switch (ProgramOptions::inputType) {
                case ProgramOptions::InputType::kPM:
                    std::cout << "  platform manifest file:   " << ProgramOptions::fileName << "\n\n";
                    break;
                case ProgramOptions::InputType::kPckCert:
                    std::cout << "  PCK Certificate:          " << ProgramOptions::fileName << "\n\n";
                    break;
                case ProgramOptions::InputType::kQuote:
                    std::cout << "  quote file:               " << ProgramOptions::fileName << "\n\n";
                    break;
                default:
                    break;
            }
        }

        switch (ProgramOptions::inputType) {
            case ProgramOptions::InputType::kPM: {
                std::optional<std::string> piidAndPridsStr =
                    retrievePIIDAndPRIDsFromPM(ProgramOptions::fileName);
                if (piidAndPridsStr.has_value()) {
                    std::cout << piidAndPridsStr.value() << std::endl;
                    ret = 0;
                    if (ProgramOptions::verbFlag) {
                        std::cout << "[OK] Retrieved PIID and PRIDs from platform manifest successfully.\n";
                    }
                } else {
                    std::cerr << "[ERROR]: Can NOT retrieve PIID and PRIDs from platform manifest.\n";
                }
                break;
            }
            case ProgramOptions::InputType::kPckCert: {
                std::optional<std::string> piidStr = retrievePIIDFromCert(ProgramOptions::fileName);
                if (piidStr.has_value()) {
                    std::cout << piidStr.value() << std::endl;
                    ret = 0;
                    if (ProgramOptions::verbFlag) {
                        std::cout << "[OK] Retrieved the PIID from PCK certificate successfully.\n";
                    }
                } else {
                    std::cerr << "[ERROR]: Can NOT retrieve piid from PCK certificate.\n";
                }
                break;
            }
            case ProgramOptions::InputType::kQuote: {
                std::optional<std::string> piidStr = retrievePIIDFromQuote(ProgramOptions::fileName);
                if (piidStr.has_value()) {
                    std::cout << piidStr.value() << std::endl;
                    ret = 0;
                    if (ProgramOptions::verbFlag) {
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
        std::cerr << "[ERROR]: Unexpected exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[ERROR]: Unknown unexpected exception." << std::endl;
        return 1;
    }
}
