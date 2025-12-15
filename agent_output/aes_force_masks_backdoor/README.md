AES FORCE_MASKS Backdoor - Exploit POC

Module: aes
RTL location: hw/ip/aes/rtl/aes.sv (SecAllowForcingMasks hard-wired to 1 at line ~180)

Vulnerability summary:
The AES core instantiation hard-wires SecAllowForcingMasks to 1, overriding
secure-top-level defaults that should disable software control of the
FORCE_MASKS CSR. This enables software to set CTRL_AUX_SHADOWED.FORCE_MASKS
and thereby force the internal masking PRNG output to a constant (via a
reseed), effectively disabling DOM masking. An attacker able to configure the
AES auxiliary control (via dif_aes_start with force_masks=true) can perform
standard first-order CPA/DPA and recover keys with modest trace counts.

Exploit type: POC (demonstrates practical disabling of masking at runtime)

What the test does (high level):
1) Uses CSRNG+EDN test helpers to prepare a deterministic seed that causes
   the AES masking PRNG to output an all-zero vector after reseed.
2) Starts an AES transaction with transaction.force_masks = true,
   which should only be allowed if the hardware permits it.
3) Triggers a PRNG reseed and DataOutClear to load the prepared zero-seed
   into the AES masking PRNG.
4) Performs an AES-ECB encryption and compares the produced ciphertext with
   the known-good unmasked KAT value. A match indicates masking was
   effectively disabled at runtime.

Expected result:
- The test should pass when the AES RTL allows forcing masks via software
  (i.e., SecAllowForcingMasks = 1). The AES will produce the known KAT
  ciphertext even though masking should have been active, proving the
  backdoor.

How to build:
  bazel build //sw/device/tests:aes_force_masks_exploit_test

How to run:
  bazel test //sw/device/tests:aes_force_masks_exploit_test

Notes / Technical details:
- The vulnerability is in the parameter passed to aes_core instantiation:
  .SecAllowForcingMasks (1). This permanently exposes the CTRL_AUX_FORCE_MASKS
  control to software even when the secure top-level default intends to keep
  it disabled.
- This test does not perform side-channel analysis. Instead it demonstrates
  the functional adverse effect: software can force masks, reseed PRNG to a
  constant, and obtain deterministic unmasked behaviour observable in
  ciphertext KATs. An attacker would follow similar steps and then perform
  CPA/DPA to extract keys.
- No assembly payload is required for this exploit; all interactions are via
  DIFs and testutils.
