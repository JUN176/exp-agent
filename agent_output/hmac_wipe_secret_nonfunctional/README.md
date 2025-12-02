HMAC Secure Wipe Nonfunctional (hmac_wipe_secret_nonfunctional)

Affected module: hmac

Vulnerability summary:
The HMAC IP contains a bug in the register write-enable logic for the "wipe_secret" control. The RTL implements the write-enable as:

  assign wipe_secret_we = (addr_hit[8] && reg_we && reg_error);

This causes writes to the wipe register to be effective only when the internal "reg_error" flag is set. Under normal (non-error) conditions reg_error is false, therefore attempts to request a secure wipe are ignored. As a consequence, the previous user's key remains loaded in the HMAC secret registers and can be reused by subsequent transactions.

Exploit type: Exploit (demonstrates unauthorized reuse of previous key / failure of secure wipe)

Test plan implemented in hmac_wipe_bypass_test.c:
1. Load a known key and compute an HMAC digest over a fixed message. Verify the digest matches the expected reference.
2. Call dif_hmac_wipe_secret() with an entropy value. A correct implementation would clobber the internal key and return a digest filled with the wipe entropy. Due to the bug the write is ignored and the digest returned remains the original digest.
3. Start a new HMAC transaction with key=NULL (i.e., no key supplied) and compute the HMAC over the same message. If the secure wipe had worked, the computed digest would differ. The test asserts the digest still matches the original reference, proving the key persisted.

How to build:
  bazel build //path/to/this:hmac_wipe_bypass_test

How to run:
  bazel test //path/to/this:hmac_wipe_bypass_test

Expected result:
- The test will log the digest values observed after the wipe and after a subsequent transaction with key=NULL.
- Both digests will match the original expected digest, demonstrating that dif_hmac_wipe_secret did not clear the key and that the HMAC secret was reused.

Notes / Technical details:
- The root cause is a faulty write-enable condition that ties the wipe register write to reg_error. The intended behavior is to allow wiping under normal conditions when a write to the wipe register occurs.
- This test will fail on hardware / designs where the RTL was fixed; it demonstrates the presence of the flaw on vulnerable designs.
