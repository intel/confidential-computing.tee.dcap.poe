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
    inline static std::string fileName = "";
    inline static std::string type = "";
    inline static bool verbFlag = false;
    inline static bool isPMFileProvided = false;
    inline static bool isPCKCertProvided = false;
    inline static bool isQuoteProvided = false;
};

void show_usage() {
    std::cout << "Usage: " << VER_PRODUCTNAME_STR
              << " extract --type <pm|pck_cert|quote>  <input_file> [-v] [--version]\n";
    std::cout << "Example: " << VER_PRODUCTNAME_STR
              << " extract --type pm platform_manifest.bin\n ";
    std::cout << "    Or: " << VER_PRODUCTNAME_STR << " extract --type pck_cert pck_cert.pem\n ";
    std::cout << "    Or: " << VER_PRODUCTNAME_STR << " extract --type quote  quote.dat\n";
    std::cout << "\nOptions:\n";
    std::cout
        << " extract                           - extract the piid (and prid) from the input file.\n"
           " --type, -t <pm|pck_cert|quote>    - support three types:\n "
           "------------------------------------------------------------------------------------\n"
           "|   pm   | the input file includes the platform manifest, and the platform manifest |\n"
           "|        | should be in hex string format                                           |\n"
           "-------------------------------------------------------------------------------------\n"
           "|pck_cert| the input file includes the provisioning certification key's certificate,|\n"
           "|        | and the certificate should be in PEM format.                             |\n"
           "-------------------------------------------------------------------------------------\n"
           "|  quote | the input file includes the quote                                        |\n"
           "-------------------------------------------------------------------------------------"
           "\n";
    std::cout
        << " input_file       - the file that includes platform manifest, or pck cert, or quote \n";
    std::cout << " -v, --verbose    - explain what is being done \n";
    std::cout << " --version        - output version information and exit \n";
    std::cout << " -h, --help       - show command help\n\n";
    std::cout << "Notes: \n";
    std::cout
        << "At the same time, you can only provide one and only one type for these three types: "
           "pm, pck_cert, quote.\n";
}

int parse_arg(int argc, const char* argv[]) {
    if (argc == 1) {
        show_usage();
        return -1;
    }
    int type_option_count = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--type" || arg == "-t" || arg.substr(0, 7) == "--type=" ||
            arg.substr(0, 3) == "-t=") {
            type_option_count++;
        }
    }
    if (type_option_count > 1) {
        std::cerr << "Error: Only one type option is allowed (--type or -t). Do not mix them.\n";
        // show_usage();
        return -1;
    }
    try {
        cxxopts::Options options("poe-gen-tool", "Platform Ownership Endorsement Generation Tool");

        // sub-command：current we only support "extract"
        options.custom_help("[OPTIONS] <input_file>");
        options.add_options()("command", "Command to run", cxxopts::value<std::string>())(
            "t,type", "Type of data: pm, pck_cert, or quote", cxxopts::value<std::string>())(
            "input_file", "Input file path", cxxopts::value<std::string>())(
            "h,help", "Print usage")("v,verbose", "Verbose output",
                                     cxxopts::value<bool>()->default_value("false"))(
            "version", "this tool's version information");

        options.parse_positional({"command", "input_file"});

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            show_usage();
            return 1;
        }

        if (result.count("version")) {
            std::cout << STRPRODUCTVER << std::endl;
            return 1;
        }

        if (!result.count("command")) {
            std::cerr << "Error: Missing command (e.g., 'extract')\n";
            show_usage();
            return -1;
        }

        std::string command = result["command"].as<std::string>();
        if (command != "extract") {
            std::cerr << "Error: Unknown command '" << command
                      << "'. Only 'extract' is supported.\n";
            show_usage();
            return -1;
        }

        if (!result.count("type")) {
            std::cerr << "Error: --type is required for 'extract'\n";
            show_usage();
            return -1;
        }

        ProgramOptions::type = result["type"].as<std::string>();
        if (ProgramOptions::type == "pm") {
            ProgramOptions::isPMFileProvided = true;
        } else if (ProgramOptions::type == "pck_cert") {
            ProgramOptions::isPCKCertProvided = true;
        } else if (ProgramOptions::type == "quote") {
            ProgramOptions::isQuoteProvided = true;
        } else {
            std::cerr << "Error: --type must be one of: pm, pck_cert, quote\n";
            return -1;
        }

        if (!result.count("input_file")) {
            std::cerr << "Error: Missing input file\n";
            show_usage();
            return -1;
        }

        ProgramOptions::fileName = result["input_file"].as<std::string>();

        if (result.count("verbose") && result["verbose"].as<bool>() != false) {
            ProgramOptions::verbFlag = true;
        }

    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        show_usage();
        return -1;
    }

    return 0;
}
// return value:
// -1: when some error happens
//  0: success
//  1: ./poe-gen-tool -h or ./poe-gen-tool --version
int main(int argc, const char* argv[]) {
    // parse the command options
    int ret = parse_arg(argc, argv);
    if (ret != 0) {
        return ret;
    }

    ret = -1;
    if (ProgramOptions::verbFlag) {
        std::cout << "\n" << VER_FILE_DESCRIPTION_STR << ". Version " << STRPRODUCTVER << "\n\n";

        std::cout << "[INFO] Parameters:\n";
        if (ProgramOptions::isPMFileProvided) {
            std::cout << "  platform manifest file:   " << ProgramOptions::fileName << "\n";
        } else {
            std::cout << "  platform manifest file:   (not provided)\n";
        }
        if (ProgramOptions::isPCKCertProvided) {
            std::cout << "  PCK Certificate:   " << ProgramOptions::fileName << "\n";
        } else {
            std::cout << "  PCK Certificate:   (not provided)\n";
        }
        if (ProgramOptions::isQuoteProvided) {
            std::cout << "  quote file:   " << ProgramOptions::fileName << "\n\n";
        } else {
            std::cout << "  quote file:   (not provided)\n\n";
        }
    }

    // 1. optional:  read platform manifest to retrieve the PIID and PRIDs
    if (ProgramOptions::isPMFileProvided) {
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
    } else if (ProgramOptions::isPCKCertProvided) {
        // 2. optional: retrieve the PIID from PCK certificate
        std::optional<std::string> piidStr = retrievePIIDFromCert(ProgramOptions::fileName);
        if (piidStr.has_value()) {
            std::cout << piidStr.value() << std::endl;
            ret = 0;
            if (ProgramOptions::verbFlag) {
                std::cout << "[OK] Retrieved the PIID from PCK certificate successfully. \n";
            }
        } else {
            std::cerr << "[ERROR]: Can NOT retrieve piid from PCK certificate.\n";
        }
    } else if (ProgramOptions::isQuoteProvided) {
        // 3. optional: retrieve the PIID from the quote
        std::optional<std::string> piidStr = retrievePIIDFromQuote(ProgramOptions::fileName);
        if (piidStr.has_value()) {
            std::cout << piidStr.value() << std::endl;
            ret = 0;
            if (ProgramOptions::verbFlag) {
                std::cout << "[OK] Retrieved the PIID from Quote successfully. \n";
            }
        } else {
            std::cerr << "[ERROR]: Can NOT retrieve piid from Quote.\n";
        }
    } else {
        std::cerr << "[ERROR]: Input file was NOT provided.\n";
    }
    return ret;
}
