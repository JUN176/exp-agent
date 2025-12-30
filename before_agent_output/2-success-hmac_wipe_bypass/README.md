Vulnerability: HMAC secure wipe gated on reg_error (hmac_reg_top.sv L2128)

Affected module: hmac
Vulnerability ID: 2

Summary
-------
The HMAC register write enable for the secure wipe register (WIPE_SECRET) is
only asserted when reg_error is true in the RTL (see hmac_reg_top.sv L2128):

  assign wipe_secret_we = (addr_hit[8] && reg_we && reg_error);

As a result, normal writes to the WIPE_SECRET register are ignored unless the
controller is already in an error state. This prevents the intended secret
wiping behavior under normal operation, allowing the previous key to persist
and be reused by subsequent transactions.

Exploit Type
------------
Proof-of-Concept (POC) demonstrating that a call to the WIPE_SECRET register
does not clear the key. The POC computes an HMAC digest with a known key,
invokes the wipe register write, then starts a new HMAC transaction without
reloading the key. If the second digest matches the first, the wipe was
ineffective and the vulnerability is confirmed.

APIs and primitives used
------------------------
- mmio_region_from_addr / mmio_region_read32 / mmio_region_write32
- mmio_region_write8
- mmio_region_nonatomic_set_bit32
- mmio_region_get_bit32
- bitfield_bit32_write, bitfield_field32_write
- read_32 (memory.h)
- Test framework: OTTF (check/log macros)

Why assembly is not needed
-------------------------
All required actions are achievable using the base MMIO and bitfield helpers.
No privilege changes or special CSR manipulation are necessary.

How to build and run
--------------------
1. Place the package under the workspace. The Bazel target is defined in the
   provided BUILD file as :hmac_poc_test.

2. Build and run (simulator):
   bazel test //path/to:hmac_poc_test --test_output=all

Expected Results
----------------
- If the vulnerability exists (which this POC demonstrates), the test will
  PASS because the second digest equals the baseline digest (wipe was
  ineffective). The test logs will contain "VULN: Secure wipe was bypassed - key
  persisted across wipe.".

- If the hardware is fixed such that WIPE_SECRET writes always clear the key,
  the second digest will differ from the baseline and the test will FAIL.

Notes
-----
- This POC intentionally avoids using DIF APIs to ensure precise control over
  register accesses and to closely match the RTL behavior.
- The test asserts success when the vulnerability is present to make it easy
  to detect regressions (i.e., a fixed design will cause the test to fail).

References
----------
- RTL: hw/ip/hmac/rtl/hmac_reg_top.sv (L2128)
- DIF implementation for reference: sw/device/lib/dif/dif_hmac.c
