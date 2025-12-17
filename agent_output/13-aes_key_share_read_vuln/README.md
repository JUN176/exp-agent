Vulnerability: AES KEY_SHARE0 Readback (Vuln ID 13)

Affected module: aes
RTL file: hw/ip/aes/rtl/aes_reg_top.sv
Vulnerable lines: 1678-1708

Summary:
The AES register file incorrectly routes the internal storage of the KEY_SHARE0
registers to the read-data bus. These registers are intended to be write-only
to prevent software from reading secret key share material, but the read
multiplexer returns reg2hw.key_share0[*].q when their addresses are accessed,
allowing software to read back values written into KEY_SHARE0.

Security impact:
Critical - this allows arbitrary software with AES MMIO access to read one
masking share of the AES key. An attacker can recover the full key by
combining this with the other share or other weaknesses, breaking the masking
countermeasure and leading to key disclosure.

Exploit type:
POC (information disclosure) - demonstrates that KEY_SHARE0 registers are
readable by software.

What this package does:
- Writes distinct test patterns into AES.KEY_SHARE0[0..7].
- Reads back each register and verifies the read value equals the written value.
- If the readback matches, the test asserts and reports success indicating
  the vulnerability is present.

Base library APIs used:
- mmio_region_from_addr, mmio_region_write32, mmio_region_read32 (sw/device/lib/base/mmio.h)
- LOG_INFO (sw/device/lib/runtime/log.h)
- CHECK (sw/device/lib/testing/test_framework/check.h)

Assembly components:
None.

How to build and run:
- Add this test to the OpenTitan workspace and run:
  bazel test //opentitan/sw/device/tests:aes_vuln13_test

- Or build for simulation:
  bazel build //opentitan/sw/device/tests:aes_vuln13_test

Expected result:
- The test logs the written and read values for each KEY_SHARE0 word and
  completes successfully when the read values equal the written patterns.
  Success indicates the AES KEY_SHARE0 registers are readable and the
  vulnerability is confirmed.

Notes and mitigation:
- As a mitigation, the hardware should be updated so that KEY_SHARE0 registers
  are truly write-only: their read mux should not expose reg2hw.key_share0.q.
  Alternatively, additional logic can force reads of these addresses to return
  zeros or undefined values and clear any internal storage from the readable
  domain.
