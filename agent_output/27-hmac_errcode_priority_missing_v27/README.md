HMAC ERR_CODE Priority Bug (Vulnerability ID 27)

Summary
-------
This package contains a proof-of-concept test that demonstrates a bug in the
HMAC IP where an invalid configuration detected at the time of HASH_START is
not included in the err_code priority encoding. As a result, err_valid (the
HMAC error interrupt) can be asserted while the ERR_CODE register contains
NoError (0).

Vulnerability Details
---------------------
- Module: hmac
- Symptom: HMAC sets hmac_err interrupt (intr_state.hmac_err) but ERR_CODE == 0
- Root cause (RTL): The err_code priority encoder omits the invalid_config_atstart
  condition (and an always_comb has a duplicated case item), causing a
  mismatch between err_valid and the written err_code value.
- Security Impact: CWE-755 (Improper Handling of Exceptional Conditions).
  Firmware may treat the interrupt as spurious or mis-handle recovery steps.

Exploit Type
------------
- POC: Demonstrates the inconsistent state where hmac_err is pending but
  ERR_CODE == NoError. Does not attempt to bypass cryptographic protections.

What the Test Does
------------------
1. Clears any pending HMAC interrupts.
2. Enables the HMAC_ERR interrupt.
3. Writes an intentionally invalid configuration (HMAC enabled but key length
   set to KEY_NONE), then issues HASH_START.
4. Reads INTR_STATE and ERR_CODE.
5. The POC considers the vulnerability present if INTR_STATE shows HMAC_ERR
   pending and ERR_CODE == 0.

Base library APIs used
---------------------
- abs_mmio_write32 / abs_mmio_read32: Direct, absolute MMIO access for control
  registers (interrupt enable/state and configuration).
- mmio_region_from_addr / mmio_region_read32: Structured MMIO reads for data
  registers.
- bitfield_bit32_write / bitfield_field32_write / bitfield_bit32_read: Setting
  and reading individual bits and fields.
- LOG_INFO / CHECK: Test logging and assertions via the OTTF framework.

Building and Running
--------------------
- Build:
  bazel build //path/to:hmac_v27_vuln_test

- Run (simulation):
  bazel test //path/to:hmac_v27_vuln_test --test_output=all

Expected Results
----------------
- On vulnerable hardware (as described): the test returns PASS and prints:
  "VULNERABLE: HMAC_ERR pending while ERR_CODE == NoError".
- On corrected hardware: the test will FAIL with message:
  "Vulnerability not observed: either no hmac_err pending or ERR_CODE != NoError".

Notes
-----
- This POC deliberately uses only the OpenTitan base libraries (abs_mmio,
  mmio, bitfield, etc.) to manipulate registers at low-level for precise control.
- The test is constructed to succeed when the hardware exhibits the incorrect
  behavior so it can be used in automated regression to detect presence of the
  bug.
