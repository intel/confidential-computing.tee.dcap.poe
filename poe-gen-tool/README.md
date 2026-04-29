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


## Output Format

The `extract` command outputs JSON to stdout.
The output includes a `syntaxVersion` field (currently `1`) that identifies the format version.
Consumers should check this value before parsing to ensure compatibility with the expected format.

A formal JSON Schema is provided at [`schemas/extract-output.schema.json`](schemas/extract-output.schema.json).
You can use it to validate output programmatically, for example with any JSON Schema 2020-12 compliant validator.

### Parsing guidance

- **Always check `syntaxVersion` first.**
  If the value is higher than what your code supports, handle it gracefully (e.g., warn and skip, or fail with a clear message).

- **Ignore unknown fields.**
  The schema permits additional properties.
  Future tool releases may add new optional fields without bumping the syntax version.
  Consumers must accept and ignore fields they do not recognize — use field-presence checks (e.g., `json.contains("fieldName")`) rather than version comparisons to discover optional data.

- **When `syntaxVersion` increments:**
  Only breaking changes cause a version bump — for example, removing a field, renaming a field, or changing a field's type or semantics.
  As long as `syntaxVersion` remains at the value your code was written for, your parser will continue to work even if new fields appear in the output.

- **Required fields by extraction type:**

  | Field               | `--type pm` | `--type pck_cert` | `--type quote` |
  |---------------------|:-----------:|:-----------------:|:--------------:|
  | `syntaxVersion`     | present     | present           | present        |
  | `platformInstanceId`| present     | present           | present        |
  | `deviceIds`         | present     | absent            | absent         |

### Example outputs

Result for extraction from PCK Certificate or SGX/TD Quote:

```json
{
  "syntaxVersion": 1,
  "platformInstanceId": "0a0b0c0d0e0f101112131415161718ff"
}
```

Result for extraction from Platform Manifest:

```json
{
  "syntaxVersion": 1,
  "platformInstanceId": "0a0b0c0d0e0f101112131415161718ff",
  "deviceIds": [
    "a1b2c3d4e5f60718293a4b5c6d7e8f90",
    "b2c3d4e5f6071829304a5b6c7d8e9fa0"
  ]
}
```
