Vulnerability: AES key_share0 register readback (Vuln ID 18)

Module: aes
Affected RTL: hw/ip/aes/rtl/aes_reg_top.sv
Lines: 1678-1680

Summary:
Write-only AES key registers (KEY_SHARE0) are incorrectly routed to the
software-visible read bus. The RTL selects reg2hw.key_share0[*].q in the read
mux when the corresponding addr_hit for KEY_SHARE0_* is asserted, allowing
software to read back the written masking share. The prim_subreg_ext instance
has .re tied low (disabling internal read-event signalling) but this does not
prevent the storage signal from being routed to the external read data path.

Impact:
A single 32-bit read returns one share of the masked AES key. Reading all
8 words of KEY_SHARE0 discloses an entire masking share, collapsing the
masking protection and enabling key recovery via software or side-channel
techniques.

Exploit Type:
POC - demonstrates direct leakage of KEY_SHARE0 through MMIO reads.

What this package does:
- Writes known 32-bit patterns into the AES KEY_SHARE0 registers using the
  normal MMIO write path.
- Reads back from the same KEY_SHARE0 addresses using MMIO read.
- Compares the read values to the written patterns. A match proves the
  vulnerability.

Base library APIs used:
- mmio_region_from_addr / mmio_region_write32 / mmio_region_read32
- LOG_INFO for logging
- OTTF test harness (ottf_main)

Files:
- aes_vuln_readback_test.c : C test that writes and reads back KEY_SHARE0
- BUILD : Bazel target to build the test

Build & Run:
Use the standard OpenTitan bazel test flow:

  bazel build //path/to:aes_vuln_readback_test
  bazel test //path/to:aes_vuln_readback_test --test_output=all

Expected Result:
- On a vulnerable design the test will pass: each read equals the value
  previously written, and the test logs will indicate the leaked key share.
- On a corrected design the reads will not match the written values (e.g.,
  return zeros or indeterminate data) and the CHECK assertions will fail.

Technical details:
The RTL at lines 1678-1680 assigns reg_rdata_next[31:0] = reg2hw.key_share0[0].q
when addr_hit[1] is asserted. Similar assignments exist for key_share0 indices
1..7. Because the internal prim_subreg_ext's .re only controls internal
read-event signalling and not the external data output, tying .re low does
not prevent the storage element from being muxed to the software read path.

Notes:
- This POC only uses base library MMIO functions and does not depend on DIF
  APIs.
- No assembly is required.
