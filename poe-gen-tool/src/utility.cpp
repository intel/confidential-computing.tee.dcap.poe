/*
 * Copyright(c) 2025-2026 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * File: utility.cpp
 *
 * Description: utility functions
 *
 */
#include "utility.h"

#include <openssl/asn1.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>  //for std::exchange
#include <vector>
namespace intel {
namespace tools {
namespace poe {
// const value in hex string encoding
inline constexpr std::size_t PRID_NUMBER_LENGTH = 4;
inline constexpr std::size_t PLATFORM_INFO_LENGTH = 476;
inline constexpr std::size_t HEADER_LENGTH = 32;
inline constexpr std::size_t PIID_LENGTH = 16;
inline constexpr std::size_t PRID_LENGTH = 16;
inline constexpr std::size_t VERSION_OFFSET_IN_PM = 18;
inline constexpr std::size_t TYPE_OFFSET_IN_PM = 20;
inline constexpr std::size_t VERSION_LENGTH = 2;
inline constexpr std::size_t TYPE_LENGTH = 1;
inline constexpr std::size_t MAX_PACKAGE_COUNT = 1024;
inline constexpr std::size_t FIRST_PRID_OFFSET_IN_PM = 33;
inline constexpr std::size_t PRID_OFFSET_IN_PM = 81;
// const value in binary encoding
inline constexpr std::size_t QE_VENDOR_ID_OFFSET = 12;
inline constexpr std::size_t QE_VENDOR_ID_LENGTH = 16;

constexpr size_t MAX_ALLOWED_FILE_SIZE = 20 * 1024 * 1024;  // 20 MB

constexpr std::string_view platform_manifest_guid = "178E874B49E44AA599BB3057170925B4";
constexpr std::string_view platform_info_guid = "84947ac684404189902a7e76cd658926";
constexpr std::string_view pairing_receipt_guid = "b40bc4671ab54066b7f960b6504bc18b";
constexpr std::string_view qe_vendor_id = "939A7233F79C4CA9940A0DB3957F0607";

// OID for SGX Extensions
constexpr std::string_view OID_SGX_EXTENSIONS = "1.2.840.113741.1.13.1";
// OID for Platform Instance ID
constexpr std::string_view OID_PLATFORM_INSTANCE_ID = "1.2.840.113741.1.13.1.6";

constexpr std::string_view begCert = "-----BEGIN CERTIFICATE-----";
constexpr std::string_view endCert = "-----END CERTIFICATE-----";

[[nodiscard]] std::optional<std::string> reverse_hex_string(std::string_view hex_string) {
    // the string's length must be even
    if (hex_string.length() % 2 != 0 || hex_string.length() == 0) {
        std::cerr << "ERROR: invalid argument,Hex string length must be even and be greater than "
                     "zero. \n";
        return std::nullopt;
    }

    std::string result;
    result.reserve(hex_string.length());
    for (auto it = hex_string.rbegin(); it != hex_string.rend(); it += 2) {
        result += *(it + 1);
        result += *(it);
    }

    return result;
}

bool caseInsensitiveCompare(char a, char b) {
    return tolower(static_cast<unsigned char>(a)) == tolower(static_cast<unsigned char>(b));
}

size_t findIgnoreCase(std::string_view str, std::string_view sub) {
    if (sub.empty()) return 0;
    auto it = std::search(str.begin(), str.end(), sub.begin(), sub.end(), caseInsensitiveCompare);

    if (it != str.end()) {
        return it - str.begin();
    }
    return std::string::npos;
}

// Deleters
auto bio_deleter = [](BIO* ptr) {
    if (ptr) BIO_free(ptr);
};
auto x509_deleter = [](X509* ptr) {
    if (ptr) X509_free(ptr);
};
auto asn1_obj_deleter = [](ASN1_OBJECT* ptr) {
    if (ptr) ASN1_OBJECT_free(ptr);
};
auto asn1_type_stack_deleter = [](STACK_OF(ASN1_TYPE) * ptr) {
    if (ptr) sk_ASN1_TYPE_pop_free(ptr, ASN1_TYPE_free);
};

// Smart pointer aliases
using BIOPtr = std::unique_ptr<BIO, decltype(bio_deleter)>;
using X509Ptr = std::unique_ptr<X509, decltype(x509_deleter)>;
using ASN1ObjectPtr = std::unique_ptr<ASN1_OBJECT, decltype(asn1_obj_deleter)>;
using ASN1TypeStackPtr = std::unique_ptr<STACK_OF(ASN1_TYPE), decltype(asn1_type_stack_deleter)>;

// Helper to convert ASN1_OCTET_STRING to hex string
std::string octet_string_to_hex(const ASN1_OCTET_STRING& oct) {
    if (!oct.data || oct.length <= 0) return "";
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < oct.length; ++i) {
        oss << std::setw(2) << static_cast<int>(oct.data[i]);
    }
    return oss.str();
}

bool hex_strings_equals_by_byte(std::string_view hex, const char* bin, size_t bin_size) {
    if (hex.size() != bin_size * 2) return false;
    if (hex.empty()) return false;

    auto hex_digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    for (size_t i = 0; i < bin_size; ++i) {
        int hi = hex_digit(hex[2 * i]);
        int lo = hex_digit(hex[2 * i + 1]);
        if (hi == -1 || lo == -1) return false;
        if (static_cast<char>((hi << 4) | lo) != bin[i]) {
            return false;
        }
    }
    return true;
}

std::optional<std::string> extract_x509_name_field(const X509_NAME* name, int nid) {
    if (!name) return std::nullopt;

    int index = X509_NAME_get_index_by_NID(name, nid, -1);
    if (index == -1) return std::nullopt;

    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, index);
    if (!entry) return std::nullopt;

    ASN1_STRING* asn1_str = X509_NAME_ENTRY_get_data(entry);
    if (!asn1_str) return std::nullopt;

    auto utf8_deleter = [](unsigned char* ptr) {
        if (ptr) OPENSSL_free(ptr);
    };

    unsigned char* utf8_raw = nullptr;
    int len = ASN1_STRING_to_UTF8(&utf8_raw, asn1_str);

    std::unique_ptr<unsigned char, decltype(utf8_deleter)> utf8(std::exchange(utf8_raw, nullptr),
                                                                utf8_deleter);

    if (len <= 0) {
        return std::nullopt;
    }

    std::string result(reinterpret_cast<char*>(utf8.get()), static_cast<size_t>(len));
    return result;
}

std::optional<std::string> get_issuer_cn(const X509* cert) {
    if (!cert) return std::nullopt;
    X509_NAME* issuer = X509_get_issuer_name(cert);
    return extract_x509_name_field(issuer, NID_commonName);
}

// get the piid through the piid's oid
std::optional<std::string> getExtensionByOIDString(std::string_view pckCert,
                                                   std::string_view oid_str) {
    BIOPtr bio(BIO_new_mem_buf(pckCert.data(), static_cast<int>(pckCert.size())));
    if (!bio) {
        std::cerr << "ERROR: Failed to create the BIO object from the certificate.\n";
        return std::nullopt;
    }
    X509Ptr cert((PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr)));
    if (!cert) {
        std::cerr << "ERROR: Failed to parse the PEM certificate.\n";
        return std::nullopt;
    }

    std::optional<std::string> issuer = get_issuer_cn(cert.get());
    if (!issuer.has_value() || issuer->empty()) {
        std::cerr << "ERROR: Failed to extract issuer CN from certificate.\n";
        return std::nullopt;
    }
    if (std::string::npos == findIgnoreCase(*issuer, "Platform CA")) {
        std::cerr << "ERROR: This certificate is NOT issued by platform CA.\n";
        return std::nullopt;
    }

    const int extsCount = X509_get_ext_count(cert.get());
    if (extsCount < 0) {
        std::cerr << "ERROR: Failed to retrieve extensions from certificate" << std::endl;
        return std::nullopt;
    }

    ASN1ObjectPtr target_obj(OBJ_txt2obj(oid_str.data(), 1));  // no name lookup, only numeric
    if (!target_obj) {
        std::cerr << "ERROR: Failed to parse OID: " << oid_str << std::endl;
        return std::nullopt;
    }

    int loc = -1;
    X509_EXTENSION* ext = nullptr;
    while ((loc = X509_get_ext_by_OBJ(cert.get(), target_obj.get(), loc)) >= 0) {
        ext = X509_get_ext(cert.get(), loc);
        break;
    }

    if (!ext) {
        std::cerr << "ERROR: Extension with OID " << oid_str << " not found.\n";
        return std::nullopt;
    }

    // get the SGX Extensions's original value（OCTET STRING context）
    ASN1_OCTET_STRING* ext_value = X509_EXTENSION_get_data(ext);
    if (!ext_value || !ext_value->data || ext_value->length <= 0) {
        std::cerr << "ERROR: Extension value is empty.\n";
        return std::nullopt;
    }

    const unsigned char* p = ext_value->data;
    long len = ext_value->length;

    // Decode the SGX Extensions sequence
    ASN1TypeStackPtr seq(d2i_ASN1_SEQUENCE_ANY(nullptr, &p, len));
    if (!seq) {
        std::cerr << "ERROR: Failed to decode SGX Extensions.\n";
        return std::nullopt;
    }

    ASN1ObjectPtr piid_oid(OBJ_txt2obj(OID_PLATFORM_INSTANCE_ID.data(), 1));
    if (!piid_oid) {
        std::cerr << "ERROR: Failed to parse PIID OID.\n";
        return std::nullopt;
    }

    // Iterate through the SGX Extensions
    for (int field_index = 0; field_index < sk_ASN1_TYPE_num(seq.get()); field_index++) {
        ASN1_TYPE* field = sk_ASN1_TYPE_value(seq.get(), field_index);
        if (field->type != V_ASN1_SEQUENCE) continue;

        const unsigned char* q = field->value.sequence->data;
        long seq_len = field->value.sequence->length;

        ASN1TypeStackPtr sub_seq(d2i_ASN1_SEQUENCE_ANY(NULL, &q, seq_len));
        if (!sub_seq) {
            std::cerr << "ERROR: Failed to decode SGX Extensions SEQUENCE.\n";
            return std::nullopt;
        }

        // must have at least 2 elements in the sequence
        if (sk_ASN1_TYPE_num(sub_seq.get()) < 2) continue;

        // Get the first element (extension ID)
        ASN1_TYPE* ext_id_type = sk_ASN1_TYPE_value(sub_seq.get(), 0);
        // Get the second element (extension value)
        ASN1_TYPE* ext_value_type = sk_ASN1_TYPE_value(sub_seq.get(), 1);

        // Check if the first element is an OBJECT IDENTIFIER
        if (!ext_id_type || ext_id_type->type != V_ASN1_OBJECT) continue;

        ASN1_OBJECT* ext_id = ext_id_type->value.object;

        // Check if the extension ID matches Platform Instance ID
        if (OBJ_cmp(ext_id, piid_oid.get()) != 0) continue;
        // Check if the second element is an OCTET STRING
        if (ext_value_type && ext_value_type->type == V_ASN1_OCTET_STRING) {
            // Extract the Platform Instance ID
            ASN1_OCTET_STRING* platform_instance_id = ext_value_type->value.octet_string;
            if (!platform_instance_id) {
                return std::nullopt;
            }
            std::string hex_str = octet_string_to_hex(*platform_instance_id);
            if (hex_str.empty()) {
                std::cerr << "ERROR: PIID value is empty.\n";
                return std::nullopt;
            }

            std::ostringstream oss;
            oss << "{\"PlatformInstanceID\" : \"" << hex_str << "\"}";
            return oss.str();
        }
    }  // finished the iteration of sgx extensions

    // Not found
    std::cerr << "ERROR: Platform Instance ID is not found in extensions.\n";
    return std::nullopt;
}

std::unique_ptr<std::ifstream> open_file(std::string_view filename) {
    namespace fs = std::filesystem;
    const fs::path file_path{filename};
    // Check if file exists and get size
    std::error_code ec{};
    auto status = fs::status(file_path, ec);
    if (ec || !fs::is_regular_file(status)) {
        std::cerr << "ERROR: the file " << filename << " doesn't exist.\n";
        return nullptr;
    }

    auto file_size = fs::file_size(file_path, ec);
    if (ec) {
        std::cerr << "ERROR: Failed to get file " << filename << " size.\n ";
        return nullptr;
    }

    // Enforce size limit
    if (file_size > MAX_ALLOWED_FILE_SIZE) {
        std::cerr << "ERROR: File too large: " << filename << " (" << std::to_string(file_size)
                  << " bytes > " << std::to_string(MAX_ALLOWED_FILE_SIZE) << " bytes) \n";
        return nullptr;
    }
    if (0 == file_size) {
        std::cerr << "ERROR: this is one empty file. \n";
        return nullptr;
    }

    auto file_ptr = std::make_unique<std::ifstream>(file_path, std::ios::binary);
    if (!file_ptr->is_open()) {
        std::cerr << "ERROR: Failed to open file: " << filename << "\n";
        return nullptr;
    }
    return file_ptr;
}

std::optional<std::string> retrievePIIDFromCert(std::string_view pemFile) {
    auto file_ptr = open_file(pemFile);
    if (!file_ptr) {
        std::cerr << "ERROR: Could NOT open file " << pemFile << std::endl;
        return std::nullopt;
    }
    std::string certData =
        std::string((std::istreambuf_iterator<char>(*file_ptr)), std::istreambuf_iterator<char>());

    std::string pckData = "";
    size_t pos_begin = findIgnoreCase(certData, begCert);
    size_t pos_end = findIgnoreCase(certData, endCert);
    if (pos_begin != std::string::npos && pos_end != std::string::npos && pos_end > pos_begin) {
        pckData = certData.substr(pos_begin, pos_end - pos_begin + endCert.size());
    } else {
        std::cerr << "ERROR: Failed to find the PCK cert in the cert file. \n";
        return std::nullopt;
    }

    return getExtensionByOIDString(pckData, OID_SGX_EXTENSIONS);
}

std::optional<std::string> retrievePIIDFromQuote(std::string_view quoteFile) {
    auto file_ptr = open_file(quoteFile);
    if (!file_ptr) {
        std::cerr << "ERROR: Could NOT open file " << quoteFile << std::endl;
        return std::nullopt;
    }
    std::string quoteData =
        std::string((std::istreambuf_iterator<char>(*file_ptr)), std::istreambuf_iterator<char>());

    if (quoteData.length() < QE_VENDOR_ID_OFFSET + QE_VENDOR_ID_LENGTH) {
        std::cerr << "ERROR: this is not a valid quote, and the quote's length is too small.\n";
        return std::nullopt;
    }
    if (hex_strings_equals_by_byte(qe_vendor_id, quoteData.data() + QE_VENDOR_ID_OFFSET,
                                   QE_VENDOR_ID_LENGTH) == false) {
        std::cerr << "ERROR: this is not a valid quote. \n";
        return std::nullopt;
    }

    size_t pos_begin = findIgnoreCase(quoteData, begCert);
    size_t pos_end = findIgnoreCase(quoteData, endCert);
    if (pos_begin != std::string::npos && pos_end != std::string::npos && pos_end > pos_begin) {
        std::string pckData = quoteData.substr(pos_begin, pos_end - pos_begin + endCert.size());
        return getExtensionByOIDString(pckData, OID_SGX_EXTENSIONS);
    } else {
        std::cerr << "ERROR: Failed to find the PCK cert in the quote. \n";
        return std::nullopt;
    }
}

std::optional<std::string> retrievePIIDAndPRIDsFromPM(std::string_view filename) {
    auto file_ptr = open_file(filename);
    if (!file_ptr) {
        std::cerr << "ERROR: Could NOT open file " << filename << std::endl;
        return std::nullopt;
    }

    file_ptr->seekg(0, std::ios::end);
    std::streamsize size = file_ptr->tellg();
    file_ptr->seekg(0, std::ios::beg);

    std::string fileBuffer(size, '\0');
    if (!file_ptr->read(fileBuffer.data(), size)) {
        std::cerr << "ERROR: Failed to read file :" << filename << std::endl;
        return std::nullopt;
    }

    // check the platform manifest's version and type
    size_t pos = findIgnoreCase(fileBuffer, platform_manifest_guid);
    if (pos == std::string::npos) {
        std::cerr << "ERROR: Could NOT find the platform manifest GUID." << std::endl;
        return std::nullopt;
    }

    if (fileBuffer.length() < pos + TYPE_OFFSET_IN_PM * 2 + TYPE_LENGTH * 2) {
        std::cerr << "ERROR: this is one invalid platform manifest, the size is too small. \n";
        return std::nullopt;
    }
    std::string version = fileBuffer.substr(pos + VERSION_OFFSET_IN_PM * 2, VERSION_LENGTH * 2);
    std::string type = fileBuffer.substr(pos + TYPE_OFFSET_IN_PM * 2, TYPE_LENGTH * 2);
    std::optional<std::string> result = reverse_hex_string(version);
    if (result.has_value()) {
        const int versionVal = std::stoi(*result, nullptr, 16);
        if (1 != versionVal) {
            std::cerr << "ERROR: the platform manifest's version is not correct" << std::endl;
            std::cerr << "the actual version is: " << versionVal
                      << "; the expected version is 1." << std::endl;
            return std::nullopt;
        }
    } else {
        std::cerr << "ERROR: Can NOT parse platform manifest header version.\n";
        return std::nullopt;
    }
    if (0 != std::stoi(type, nullptr, 16)) {
        std::cerr << "ERROR: the platform manifest's type is not correct" << std::endl;
        std::cerr << "the actual type is: " << std::stoi(type, nullptr, 16)
                  << "; the expected type is 0." << std::endl;
        return std::nullopt;
    }

    // check the platform info's version and type
    pos = findIgnoreCase(fileBuffer, platform_info_guid);
    if (pos == std::string::npos) {
        std::cerr << "ERROR: Failed to find the platform instance id." << std::endl;
        return std::nullopt;
    }

    if (fileBuffer.length() < pos + TYPE_OFFSET_IN_PM * 2 + TYPE_LENGTH * 2) {
        std::cerr << "ERROR: this is one invalid platform manifest, the size is too small. \n";
        return std::nullopt;
    }
    version = fileBuffer.substr(pos + VERSION_OFFSET_IN_PM * 2, VERSION_LENGTH * 2);
    type = fileBuffer.substr(pos + TYPE_OFFSET_IN_PM * 2, TYPE_LENGTH * 2);
    result = reverse_hex_string(version);
    if (result.has_value()) {
        if (1 != std::stoi(result.value(), nullptr, 16)) {
            std::cerr << "ERROR: the platform Info's version is not correct" << std::endl;
            std::cerr << "The actual version is:" << std::stoi(result.value(), nullptr, 16)
                      << "; the expected version is 1." << std::endl;
            return std::nullopt;
        }
    } else {
        std::cerr << "ERROR: Can NOT parse platform info version.\n";
        return std::nullopt;
    }
    if (0 != std::stoi(type, nullptr, 16)) {
        std::cerr << "ERROR: the platform Info's type is not correct" << std::endl;
        std::cerr << "The actual type is: " << std::stoi(type, nullptr, 16)
                  << "; the expected type is 00." << std::endl;
        return std::nullopt;
    }

    // Ensure the buffer is large enough for the PIID field and the full Platform Info
    if (fileBuffer.length() < pos + HEADER_LENGTH * 2 + PIID_LENGTH * 2 ||
        fileBuffer.length() < pos + PLATFORM_INFO_LENGTH * 2) {
        std::cerr << "ERROR: this is one invalid platform manifest, the size is too small. \n ";
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << "{\n  \"PlatformInstanceID\" : \"";
    oss << fileBuffer.substr(pos + HEADER_LENGTH * 2, PIID_LENGTH * 2) << "\"," << std::endl;

    std::string numberOfPackagesString = fileBuffer.substr(
        pos + PLATFORM_INFO_LENGTH * 2 - PRID_NUMBER_LENGTH * 2, PRID_NUMBER_LENGTH * 2);
    result = reverse_hex_string(numberOfPackagesString);
    if (result.has_value() == false) {
        std::cerr << "ERROR: Can NOT parse platform package number.\n";
        return std::nullopt;
    }
    std::size_t numberOfPackages =
        static_cast<std::size_t>(std::stoul(result.value(), nullptr, 16));
    if (numberOfPackages == 0 || numberOfPackages > MAX_PACKAGE_COUNT) {
        std::cerr << "ERROR: Invalid platform package number.\n";
        return std::nullopt;
    }

    pos = findIgnoreCase(fileBuffer, pairing_receipt_guid);
    if (pos != std::string::npos) {
        if (fileBuffer.length() < pos + FIRST_PRID_OFFSET_IN_PM * 2 + PRID_LENGTH * 2) {
            std::cerr << "ERROR: this is one invalid platform manifest, the size is too small. \n";
            return std::nullopt;
        }
        // the first PRID
        std::string prid = fileBuffer.substr(pos + FIRST_PRID_OFFSET_IN_PM * 2, PRID_LENGTH * 2);
        oss << "  \"DeviceIDs\" : [\"" << prid << "\"";
        for (std::size_t count = 1; count < numberOfPackages; ++count) {
            if (fileBuffer.length() < pos + PRID_OFFSET_IN_PM * 2 + PRID_LENGTH * 2 * count) {
                std::cerr << "ERROR: this is one invalid platform manifest, the size is too "
                             "small. \n";
                return std::nullopt;
            }
            prid = fileBuffer.substr(pos + PRID_OFFSET_IN_PM * 2 + PRID_LENGTH * 2 * (count - 1),
                                     PRID_LENGTH * 2);
            oss << ", \"" << prid << "\"";
        }
        oss << "]\n}";
    } else {
        std::cerr << "ERROR: Failed to find the processor registration ID." << std::endl;
        return std::nullopt;
    }
    return oss.str();
}

}  // namespace poe
}  // namespace tools
}  // namespace intel