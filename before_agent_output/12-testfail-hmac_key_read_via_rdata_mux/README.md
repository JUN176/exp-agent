Vulnerability: HMAC read-data multiplexer leaks secret key words (Vuln ID 12)

Affected module: hmac

Summary
-------
This proof-of-concept demonstrates a wiring bug in the HMAC register
read-data multiplexer: the RTL incorrectly sources internal secret key
storage onto the TL-UL read path. Specifically, reads at offsets 0x20 and
0x24 return internal key words (reg2hw.key[0].q and reg2hw.key[1].q) instead
of write-only or safe values. This allows any bus master with read access to
the HMAC register window to deterministically read the first 64 bits of the
HMAC key (CWE-200, CWE-320).

Exploit type: POC (deterministic disclosure of key material)

What this package contains
-------------------------
- BUILD: Bazel rule to compile the test (hmac_vuln_test)
- hmac_vuln_read_test.c: C test that reads from the HMAC base address using
  abs_mmio_read32 at offsets 0x20 and 0x24 and logs the returned values.

How it works (high level)
-------------------------
1. The RTL read-data multiplexer has case items that, for certain address
   hits, drive the read-data register with internal key state (reg2hw.key).
2. The PoC performs two 32-bit reads at the vulnerable offsets using
   abs_mmio_read32(TOP_EARLGREY_HMAC_BASE_ADDR + offset).
3. The values are logged via LOG_INFO; these should correspond to the first
   two 32-bit words of the secret HMAC key in a vulnerable design.

Base library APIs used
----------------------
- abs_mmio_read32() (sw/device/lib/base/abs_mmio.h)
- LOG_INFO() (sw/device/lib/runtime/log.h)
- OTTF test framework (sw/device/lib/testing/test_framework:ottf_main)

Notes on headers / build
------------------------
The BUILD rule depends on:
- //hw/top_earlgrey/sw/autogen:top_earlgrey (provides TOP_EARLGREY_HMAC_BASE_ADDR)
- //hw/top:hmac_c_regs (provides hmac_regs.h)
- standard base libraries (mmio, abs_mmio, bitfield, logging, ottf_main)

Expected results
----------------
When run on a device or simulation with the vulnerable RTL the test logs
two 32-bit values that correspond to secret key words. Manual inspection of
the log output is required to confirm that the values are indeed secret
material (this PoC intentionally does not hard-assert on specific values to
avoid false positives in different test environments).

Mitigation
----------
Fix the reg_top multiplexer so that read-data paths never source secret
state. Key registers must be implemented as write-only from the SW-visible
register architecture. Audit other reg_top implementations for similar
mistakes.

References
----------
- Vulnerability ID: 12
- File: hw/ip/hmac/rtl/hmac_reg_top.sv (reported lines ~2418-2424)
- CWE-200: Information Exposure
- CWE-320: Key Management Error
