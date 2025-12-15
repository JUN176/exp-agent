HMAC Secure Wipe Bypass (hmac_wipe_bypass)

Vulnerability Summary
- Module: hmac
- RTL file: hw/ip/hmac/rtl/hmac_reg_top.sv
- Vulnerability: The write-enable signal for the secure wipe path (wipe_secret_we)
  is only asserted when an internal error flag (reg_error) is set. Because
  reg_error is not set during normal operation, invoking the secure wipe
  register does not actually clear the loaded HMAC secret. This allows an
  attacker to reuse the previous user's key to compute subsequent HMAC digests.

Vulnerability Mechanism
- In the RTL: `assign wipe_secret_we = (addr_hit[8] && reg_we && reg_error);`
  This means a write to the wipe register only takes effect if `reg_error` is
  true. Typical clients calling the DIF API will not set reg_error; therefore
  the wipe request is ignored.

Exploit Type: Exploit (demonstrates unauthorized reuse of a previously loaded key)

Test Plan
1. Initialize the HMAC DIF and configure a known 256-bit key.
2. Compute a digest for a fixed message and record the digest (digest_before).
3. Call dif_hmac_wipe_secret() with a non-error entropy value.
4. Start a new HMAC operation with key=NULL and compute the digest for the same
   message (digest_after).
5. If digest_after == digest_before, the secure wipe did not remove the key
   and the vulnerability is confirmed.

Build & Run
- Build:
  bazel build //path/to/this: hmac_wipe_bypass_test

- Run (on supported targets):
  bazel test //path/to/this:hmac_wipe_bypass_test

Expected Results
- On vulnerable hardware, the test will pass and print "VULNERABILITY DETECTED:"
  because the digest computed after the wipe (with key=NULL) will match the
  digest computed before the wipe (with an explicit key).
- On fixed hardware, the test will fail the CHECK because the secure wipe will
  have cleared the key and the digests will differ.

Notes
- This test uses only DIF and testutils APIs and does not perform any
  privileged or assembly-level operations.
- The test intentionally considers a pass to be proof of the vulnerability.
