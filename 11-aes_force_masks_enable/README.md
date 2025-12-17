Vulnerability: AES SecAllowForcingMasks parameter hardwired to 1
Module: aes
Vulnerability ID: 11

Overview
--------
The AES core was instantiated with the SecAllowForcingMasks parameter tied to
1 in the top-level RTL. This enables the FORCE_MASKS CSR path inside the AES
peripheral, allowing software to force the internal masking PRNG to produce
constant masks. When software can set FORCE_MASKS and trigger reseeding to a
chosen seed, the design reintroduces first-order side-channel leakage.

Exploit Type
------------
POC — demonstrates that software can set the FORCE_MASKS control bit using
low-level MMIO, proving the CSR path is enabled in silicon/RTL.

What this package contains
--------------------------
- BUILD: Bazel target to build the test: aes_force_masks_poc_test
- aes_force_masks_poc_test.c: Test that uses base library APIs (mmio, bitfield,
  LOG) to set AES_CTRL_AUX_SHADOWED.FORCE_MASKS and trigger a PRNG reseed.

Base library APIs used
---------------------
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()
  (//sw/device/lib/base:mmio)
- bitfield_bit32_write(), bitfield_bit32_read() (//sw/device/lib/base:bitfield)
- LOG_INFO() (//sw/device/lib/runtime:log)
- OTTF test framework (//sw/device/lib/testing/test_framework:ottf_main)

Why this proves the vulnerability
---------------------------------
The test writes to AES_CTRL_AUX_SHADOWED (shadowed register, double-write)
and then reads it back. If the FORCE_MASKS bit can be set by software, it
confirms the CSR path exists and is writable at runtime. With SecAllowForcingMasks
hardwired to 1 in RTL, this path cannot be disabled by SoC integrators.

How to build and run
--------------------
bazel build //opentitan/sw/device/tests:crypto/aes_force_masks_poc_test
bazel test //opentitan/sw/device/tests:crypto/aes_force_masks_poc_test

Expected results
----------------
- The test will PASS if the FORCE_MASKS bit can be set and read back as 1.
- The test logs the register values before and after the write.

Notes and caveats
-----------------
- If the AES_CTRL_AUX_REGWEN register has been set to 0 by earlier boot code,
  the shadowed AUX register may be write-locked. In that case the test will
  fail because writes are prevented; this is still useful information (it
  indicates the CSR was locked by firmware) but does not refute the RTL
  issue.
- This POC intentionally uses only base library APIs (no DIF) to demonstrate
  low-level exploitability and to keep the test deterministic and minimal.

References
----------
- RTL line: hw/ip/aes/rtl/aes.sv L0180 shows .SecAllowForcingMasks set to 1,
  enabling the FORCE_MASKS CSR path.
