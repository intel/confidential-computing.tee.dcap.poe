# Intel® Platform Ownership Endorsement Generator (Intel® POE Generator)

A major step in Intel's [Platform Ownership Endorsement](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/platform-ownership-endorsements.html) flow is to extract platform identities and to generate signed POE structures.

For now, this tool is able to extract Platform Instance IDs (PIID) and Processor Registration IDs (PRID) from Platform Manifests, PCK Certificates, or SGX/TD Quotes.

## Prerequisites

- Ensure that you have one of the following input files:
    * a file that includes the Platform Manifest, and the Platform Manifest should be a hex string
      format. For example, the output file from PCKIDRetrievalTool
    * a PEM-formatted PCK Certificate (or a certificate chain that includes the PCK Certificate)
    * an SGX/TD Quote that was generated on the target platform


## Build

For build instructions, see [BUILD.md](BUILD.md).

## Usage

### Basic

```
Usage:
  poe-gen-tool [--verbose] <command> [<args>]

  -v, --verbose  Show what is being done
  -h, --help     Show command help

Commands:
  extract   Extract PIID/PRID from pm, pck_cert, or quote

  version   Show tool version

Global options:
  --verbose can be placed before or after <command>

Examples:
  poe-gen-tool extract --help
  poe-gen-tool extract --type pm        platform_manifest.bin
  poe-gen-tool extract --type pck_cert  pck_cert.pem
  poe-gen-tool extract --type quote     quote.dat
  poe-gen-tool version
```


### `extract` Command

```
Usage:
  poe-gen-tool extract --type <pm|pck_cert|quote> <input_file>

  -h, --help      Show this help message
  -t, --type arg  Type of the input_file (required). One of:
                    pm       : for platform manifest (Base16 format)
                    pck_cert : for PCK certificate (PEM format)
                    quote    : for quote

Examples:
  poe-gen-tool extract --type pm platform_manifest.bin
  poe-gen-tool extract --type pm platform_manifest.bin --verbose
  poe-gen-tool extract --type pck_cert pck_cert.pem
  poe-gen-tool extract --type quote quote.dat

Global options:
  --verbose, -v   Show what is being done (can be placed before or after 'extract')

Notes:
  At the same time, you can only provide one and only one type for these three types: pm, pck_cert, quote.
```
