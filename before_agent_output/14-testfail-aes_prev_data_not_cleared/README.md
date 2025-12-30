Vulnerability: AES previous data register zeroization bypass (DATA_REG.SEC_WIPE)

Module: aes
RTL: hw/ip/aes/rtl/aes_core.sv (lines 367-373)

Summary
-------
The AES core's data_in_prev_mux erroneously maps DIP_CLEAR to data_in instead of the pseudo-random
clearing value (prd_clearing_data). When the control path selects DIP_CLEAR and the previous-data
write-enable is asserted, data_in_prev_q is reloaded from the live data_in (software-provided
plaintext) rather than being wiped with PRD. This violates the DATA_REG.SEC_WIPE requirement and
can lead to sensitive data remaining visible after clear operations (CWE-226: Incomplete Clearing).

POC Description
---------------
This proof-of-concept demonstrates a practical manifestation of the bug by:
1) Writing a recognisable secret into AES.DATA_IN registers.
2) Triggering the KEY_IV_DATA_IN_CLEAR operation (assumed to be trigger bit 2 in the AES TRIGGER
   register).
3) Reading back AES.DATA_IN registers. If the secret remains intact, the clear operation failed to
   wipe the input-related registers, indicating the vulnerability.

Notes and Assumptions
---------------------
- This test uses low-level base library MMIO operations (mmio_region_* and abs_mmio_* patterns) as
  recommended for POC development.
- Register offsets are taken from the RTL package definitions (aes_reg_pkg) and hardcoded here as
  constants. Specifically:
    AES_DATA_IN_0_OFFSET = 0x54
    AES_DATA_IN_1_OFFSET = 0x58
    AES_DATA_IN_2_OFFSET = 0x5c
    AES_DATA_IN_3_OFFSET = 0x60
    AES_TRIGGER_OFFSET    = 0x80
- The mapping of trigger fields into bits is assumed (based on the field order in the RTL):
  prng_reseed = bit 0, data_out_clear = bit 1, key_iv_data_in_clear = bit 2, start = bit 3.
  Therefore the POC writes 0x4 to the TRIGGER register to request a Key/IV/DataIn clear.
- If the target design maps trigger bits differently, the concrete trigger write value may need to
  be adjusted (consult the auto-generated aes_regs.h for the exact field positions).

How to build and run
--------------------
- The Bazel BUILD rule is provided. Example:
  bazel build //path/to:aes_prev_data_wipe_vuln_test
  bazel test //path/to:aes_prev_data_wipe_vuln_test --test_output=all

Expected Results
----------------
- On a vulnerable hardware/simulation image (matching the RTL at the top), the test will detect that
  the DATA_IN registers still contain the secret after the KEY_IV_DATA_IN_CLEAR trigger and will
  pass (proving the vulnerability).
- On a patched or non-vulnerable implementation the DATA_IN registers will not retain the secret
  (they will be cleared to PRD/zeros) and the test will fail (indicating the vulnerability is not
  present in that build).

APIs used
---------
- mmio_region_from_addr / mmio_region_write32 / mmio_region_read32 (sw/device/lib/base/mmio.h)
- LOG_INFO (sw/device/lib/runtime/log.h)
- CHECK (sw/device/lib/testing/test_framework/check.h)

Caveats
-------
Because some control behavior and trigger bit positions are inferred from the RTL rather than
queried from the auto-generated register header, small adjustments to the trigger value or offsets
may be necessary to run the POC against a specific software environment. The BUILD file depends on
//hw/top:aes_c_regs which provides the canonical register header; consult that header if the POC
must be adapted to a particular tree layout.
