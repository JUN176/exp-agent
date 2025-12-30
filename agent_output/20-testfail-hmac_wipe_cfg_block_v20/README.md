Vulnerability POC: HMAC WIPE_SECRET triggers during cfg_block (Vuln ID 20)

Affected module: hmac
Vulnerability ID: 20

Summary
-------
This proof-of-concept demonstrates that writing to the WIPE_SECRET CSR triggers
secret wiping even when the configuration is supposed to be blocked (cfg_block
asserted). The RTL contains the assignment:

  assign wipe_secret = reg2hw.wipe_secret.qe;
  assign wipe_v      = reg2hw.wipe_secret.q;

and in update_secret_key the wipe branch is evaluated before the cfg_block
qualification. As a result, the write-event pulse (qe) alone causes the secret
to be overwritten (replicated {32{wipe_v}}), breaking the intended protection.

Impact
------
High. An attacker can force key zeroization or deterministic reprogramming of
the HMAC secret while the engine is in a protected phase. This enables DoS by
repeated wipes, corrupts integrity of HMAC calculations, and violates
assumptions about configuration immutability during processing.

POC Overview
------------
The test performs the following steps using only base library (no DIF APIs):

1. Writes a known non-zero key into the HMAC KEY CSRs.
2. Configures HMAC and starts a hash operation to assert cfg_block.
3. While the engine is running (cfg_block expected asserted), writes to the
   WIPE_SECRET CSR with an all-ones value (0xFFFFFFFF).
4. Reads back the DIGEST CSRs which the HW is expected to populate with the
   wipe value when wipe occurs.
5. Asserts that the DIGEST words equal the wipe value, demonstrating the wipe
   happened despite cfg_block.

Files in package
----------------
- hmac_wipe_cfg_block_poc.c : C test implementing the POC (base library APIs)
- BUILD                    : Bazel rule to build the test
- README.md                : This document

Build & Run
-----------
Use bazel to build and run the test in the OpenTitan environment. Example:

  bazel test //path/to/hmac_wipe_cfg_block_v20:hmac_v20_poc_test

Expected result
---------------
The test should pass, and the log should include messages indicating the wipe
was observed while cfg_block was asserted. If the DIGEST registers equal the
wipe value after the write, the vulnerability is reproduced.

Notes & Technical Details
-------------------------
- The POC uses mmio_region_* and bitfield_* base APIs. No DIF APIs are used to
  ensure precise control of register writes and events.
- Timing is controlled by starting the HMAC operation and polling the HMAC
  status idle bit to ensure the engine has started (cfg_block should be set
  within the HW state machine). The WIPE_SECRET write is performed while the
  engine is active.
- The RTL bug gives wipe priority over cfg_block in update_secret_key and uses
  the 1-bit wipe field replicated across the entire key word width via
  {32{wipe_v}}, enabling deterministic reprogramming.

References
----------
- RTL excerpt (hmac.sv) lines around update_secret_key: assign wipe_secret = reg2hw.wipe_secret.qe;

