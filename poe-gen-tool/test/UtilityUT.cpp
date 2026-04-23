/*
 * Copyright (C) 2026 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "utility.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace poe = intel::tools::poe;

class FileIOTest : public ::testing::Test
{
protected:
    std::filesystem::path tmpDir;

    void SetUp() override
    {
        const auto baseTmpDir = std::filesystem::temp_directory_path();
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<unsigned long long> dist;
        bool created = false;
        for (int attempt = 0; attempt < 16 && !created; ++attempt)
        {
            tmpDir = baseTmpDir / ("poe_gen_tool_ut_" + std::to_string(dist(gen)));
            created = std::filesystem::create_directories(tmpDir);
        }
        ASSERT_TRUE(created) << "Failed to create unique temp directory under: " << baseTmpDir;
    }

    void TearDown() override
    {
        std::filesystem::remove_all(tmpDir);
    }

    [[nodiscard]] std::filesystem::path filePath(const std::string& name) const
    {
        return tmpDir / name;
    }

    // Write binary (or text) content to a temp file.
    void writeFile(const std::filesystem::path& path, const std::string& data) const
    {
        std::ofstream f(path, std::ios::binary);
        ASSERT_TRUE(f.is_open()) << "Cannot create temp file: " << path;
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
};

static std::string buildMinimalValidPM(
    const std::string& piid = "AABBCCDDEEFF00112233445566778899",
    const std::string& prid = "11223344556677889900AABBCCDDEEFF")
{
    std::string pm;
    pm.reserve(1092);

    pm += "178E874B49E44AA599BB3057170925B40000010000"
          "84947AC684404189902A7E76CD6589260000010000";
    pm += std::string(22, '0');
    pm += piid;
    pm += std::string(848, '0');
    pm += "01000000B40BC4671AB54066B7F960B6504BC18B";
    pm += std::string(34, '0');
    pm += prid;

    return pm;
}

TEST_F(FileIOTest, PM_NonExistentFile_ReturnsNullopt)
{
    EXPECT_FALSE(
        poe::retrievePIIDAndPRIDsFromPM(filePath("no_such_file.hex").string())
            .has_value());
}

TEST_F(FileIOTest, PM_EmptyFile_ReturnsNullopt)
{
    const auto path = filePath("empty.hex");
    writeFile(path, "");
    EXPECT_FALSE(poe::retrievePIIDAndPRIDsFromPM(path.string()).has_value());
}

TEST_F(FileIOTest, PM_NoPMGuid_ReturnsNullopt)
{
    const auto path = filePath("no_guid.hex");
    writeFile(path, std::string(200, 'A'));  // 200 hex chars, no GUID present
    EXPECT_FALSE(poe::retrievePIIDAndPRIDsFromPM(path.string()).has_value());
}

TEST_F(FileIOTest, PM_WrongVersion_ReturnsNullopt)
{
    auto pm = buildMinimalValidPM();
    pm.replace(36, 4, "0200");
    const auto path = filePath("bad_version.hex");
    writeFile(path, pm);
    EXPECT_FALSE(poe::retrievePIIDAndPRIDsFromPM(path.string()).has_value());
}

TEST_F(FileIOTest, PM_WrongType_ReturnsNullopt)
{
    auto pm = buildMinimalValidPM();
    pm.replace(40, 2, "01");
    const auto path = filePath("bad_type.hex");
    writeFile(path, pm);
    EXPECT_FALSE(poe::retrievePIIDAndPRIDsFromPM(path.string()).has_value());
}

TEST_F(FileIOTest, PM_ValidSinglePackage_ReturnsExpectedJSON)
{
    const std::string piid = "AABBCCDDEEFF00112233445566778899";
    const std::string prid = "11223344556677889900AABBCCDDEEFF";
    const auto path = filePath("valid.hex");
    writeFile(path, buildMinimalValidPM(piid, prid));

    const auto result = poe::retrievePIIDAndPRIDsFromPM(path.string());
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->find("PlatformInstanceID"), std::string::npos);
    EXPECT_NE(result->find("DeviceIDs"), std::string::npos);
    EXPECT_NE(result->find(piid), std::string::npos);
    EXPECT_NE(result->find(prid), std::string::npos);
}

TEST_F(FileIOTest, PM_NonHexVersionField_ReturnsNullopt)
{
    auto pm = buildMinimalValidPM();
    pm.replace(36, 4, "GGGG");
    const auto path = filePath("bad_hex_version.hex");
    writeFile(path, pm);
    EXPECT_FALSE(poe::retrievePIIDAndPRIDsFromPM(path.string()).has_value());
}

TEST_F(FileIOTest, PM_NonHexPackageCount_ReturnsNullopt)
{
    auto pm = buildMinimalValidPM();
    pm.replace(986, 8, "GGGGGGGG");
    const auto path = filePath("bad_hex_pkg_count.hex");
    writeFile(path, pm);
    EXPECT_FALSE(poe::retrievePIIDAndPRIDsFromPM(path.string()).has_value());
}


TEST_F(FileIOTest, Cert_NonExistentFile_ReturnsNullopt)
{
    EXPECT_FALSE(
        poe::retrievePIIDFromCert(filePath("no_such.pem").string()).has_value());
}

TEST_F(FileIOTest, Cert_EmptyFile_ReturnsNullopt)
{
    const auto path = filePath("empty.pem");
    writeFile(path, "");
    EXPECT_FALSE(poe::retrievePIIDFromCert(path.string()).has_value());
}

TEST_F(FileIOTest, Cert_NoPemMarkers_ReturnsNullopt)
{
    const auto path = filePath("no_markers.pem");
    writeFile(path, "this is not a PEM file at all, just plain text\n");
    EXPECT_FALSE(poe::retrievePIIDFromCert(path.string()).has_value());
}

TEST_F(FileIOTest, Cert_InvalidPemContent_ReturnsNullopt)
{
    // PEM markers present but base64 payload is not valid DER — OpenSSL will
    // reject it during PEM_read_bio_X509().
    const auto path = filePath("bad_content.pem");
    writeFile(path,
        "-----BEGIN CERTIFICATE-----\n"
        "dGhpcyBpcyBub3QgYSByZWFsIGNlcnQ=\n"  // "this is not a real cert"
        "-----END CERTIFICATE-----\n");
    EXPECT_FALSE(poe::retrievePIIDFromCert(path.string()).has_value());
}

// Intel SGX QE vendor ID (16 bytes).  Placed at byte offset 12 in the quote.
static const std::string kQeVendorId(
    "\x93\x9A\x72\x33\xF7\x9C\x4C\xA9"
    "\x94\x0A\x0D\xB3\x95\x7F\x06\x07", 16);

TEST_F(FileIOTest, Quote_NonExistentFile_ReturnsNullopt)
{
    EXPECT_FALSE(
        poe::retrievePIIDFromQuote(filePath("no_such.dat").string()).has_value());
}

TEST_F(FileIOTest, Quote_EmptyFile_ReturnsNullopt)
{
    const auto path = filePath("empty.dat");
    writeFile(path, "");
    EXPECT_FALSE(poe::retrievePIIDFromQuote(path.string()).has_value());
}

TEST_F(FileIOTest, Quote_TooSmall_ReturnsNullopt)
{
    // Minimum required: QE_VENDOR_ID_OFFSET(12) + QE_VENDOR_ID_LENGTH(16) = 28 bytes
    const auto path = filePath("small.dat");
    writeFile(path, std::string(10, '\x00'));
    EXPECT_FALSE(poe::retrievePIIDFromQuote(path.string()).has_value());
}

TEST_F(FileIOTest, Quote_WrongQeVendorId_ReturnsNullopt)
{
    // 28 zero bytes — vendor ID at bytes 12-27 is all zeros, not Intel's ID
    const auto path = filePath("bad_vendor.dat");
    writeFile(path, std::string(28, '\x00'));
    EXPECT_FALSE(poe::retrievePIIDFromQuote(path.string()).has_value());
}

TEST_F(FileIOTest, Quote_CorrectQeVendorIdNoCert_ReturnsNullopt)
{
    // Vendor ID matches but there is no embedded PEM certificate in the payload
    std::string quote(28, '\x00');
    quote.replace(12, 16, kQeVendorId);
    const auto path = filePath("no_cert.dat");
    writeFile(path, quote);
    EXPECT_FALSE(poe::retrievePIIDFromQuote(path.string()).has_value());
}
