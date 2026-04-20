Intel(R) Software Guard Extensions Data Center Attestation Primitives (Intel(R) SGX DCAP): Platform Ownership Endorsement Tool
===============================================

## Prerequisites
- Ensure that you have one of the following input files:
    * file that includes the platform manifest, and the platform manifest should be in hex string 
      format. For example, the output file from PCKIDRetrievalTool
    * Or the PEM format certificate(or the certificate chain that including the platform's 
      certificate) for the platform
    * Or the quote that was generated on the target platform


## Usage
Usage: poe-gen-tool [--verbose] <command> [<args>]
Example: poe-gen-tool extract --type pm platform_manifest.bin
      Or: poe-gen-tool extract --type pck_cert pck_cert.pem
      Or: poe-gen-tool extract --type quote quote.dat
      Or: poe-gen-tool version

Global options:
 -v, --verbose    - explain what is being done
 -h, --help       - show command help

Commands:
 extract           - extract the piid (and prid) from the input file.
 version           - output version information and exit

extract options:
 --type, -t <pm|pck_cert|quote>        - support three types:
 ------------------------------------------------------------------------------------
|   pm   | the input file includes the platform manifest, and the platform manifest |
|        | should be in hex string format                                           |
-------------------------------------------------------------------------------------
|pck_cert| the input file includes the provisioning certification key's certificate,|
|        | and the certificate should be in PEM format.                             |
-------------------------------------------------------------------------------------
|  quote | the input file includes the quote                                        |
-------------------------------------------------------------------------------------
 input_file       - the file that includes platform manifest, or pck cert, or quote

Notes: 
1. At the same time, you can only provide one and only one type for these three types: pm, pck_cert, quote.
2. For more background information, you can refer to: 
     https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/technical-documentation/platform-ownership-endorsements.html 



