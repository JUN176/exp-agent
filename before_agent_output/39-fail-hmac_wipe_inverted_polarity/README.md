Vulnerability POC: HMAC WIPE_SECRET inverted reg_error polarity (Vuln ID 39)

Affected module: hmac
RTL location: hw/ip/hmac/rtl/hmac_reg_top.sv
Problematic lines: 2127-2128

Summary:
The WIPE_SECRET CSR write-enable signal is gated with "reg_error" instead of
"!reg_error". Because reg_error is normally 0 during valid register writes,
this inverted polarity prohibits normal full-word writes to WIPE_SECRET while
allowing writes when a bus error (wr_err/addrmiss/intg_err) is present.

Impact:
This prevents the standard software-triggered secure wipe from operating in
normal conditions and only allows wipe to occur during erroneous bus
transactions, undermining confidentiality guarantees (CWE-459) and creating
inconsistent software/hardware semantics.

POC approach:
1) Perform a normal 32-bit write to HMAC_WIPE_SECRET and observe that the
   digest registers are NOT updated (the wipe did not occur).
2) Perform byte-wise writes to the same WIPE_SECRET register. Sub-word writes
   intentionally trigger wr_err (write byte-enable mismatch), which sets
   reg_error=1 for those transactions. Due to the inverted gating, WIPE_SECRET
   becomes writable only in that errored transaction path; the byte writes
   therefore perform the wipe and update digest registers to the provided
   value.

Files in this package:
- hmac_v39_wipe_inverted_polarity_test.c : C test implementing the above POC.
- BUILD : Bazel target to build the test
- README.md : This file.

Base library APIs used:
- abs_mmio_write32 / abs_mmio_write8 : to perform full-word and byte writes to
  the WIPE_SECRET address and intentionally trigger sub-word writes.
- mmio_region_from_addr / mmio_region_read32 : to read digest registers.
- LOG_INFO, CHECK : logging and assertions from the test framework.

How to build and run (from the OpenTitan repo root):
  bazel test //path/to/package:hmac_v39_wipe_inverted_polarity_test

Expected result:
- The full 32-bit write does NOT change the digest registers (showing the
  intended wipe is blocked).
- The subsequent byte-wise writes cause the digest registers to be overwritten
  with the wipe value, demonstrating the inverted reg_error polarity bug.

Notes / Limitations:
- The POC relies on the register write-back and sub-word write detection
  semantics of the TLUL adapter and reggen-generated logic. In simulation or
  hardware, timings and bus behavior may differ; the test is designed to use
  standard mmio/abs_mmio helpers to produce representative transactions.
- If the DUT has been patched to correct the polarity, the test will fail
  the initial CHECK (the full-word write will succeed), indicating the
  vulnerability has been fixed.
