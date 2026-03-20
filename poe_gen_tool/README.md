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
Usage: poe_gen_tool extract --type <pm|pck_cert|quote>  <input_file> [-v] [--version]
Example: poe_gen_tool extract --type pm platform_manifest.bin
      Or: poe_gen_tool extract --type pck_cert pck_cert.pem
      Or: poe_gen_tool extract --type quote quote.dat

Options:
 extract           - extract the piid (and prid) from the input file.
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
 -v, --verbose    - explain what is being done 
 --version        - output version information and exit 
 -h, --help       - show command help

Notes: 
At the same time, you can only provide one and only one type for these three types: pm, pck_cert, quote.



