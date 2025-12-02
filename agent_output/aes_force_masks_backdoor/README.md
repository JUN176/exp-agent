AES FORCE_MASKS Backdoor Vulnerability

Module: aes
Vulnerability: SecAllowForcingMasks hardwired to 1 enabling FORCE_MASKS CSR path

Summary
- The AES core (hw/ip/aes/rtl/aes.sv) instantiates the AES core with
  SecAllowForcingMasks set to 1. This overrides the secure top-level default
  (0) and permanently enables the CSR control path that allows software to
  force internal masks to constant values (CTRL_AUX_SHADOWED.FORCE_MASKS).
- By setting transaction.force_masks = true in software and controlling the
  AES masking PRNG seed via CSRNG/EDN, an attacker can disable DOM masking
  randomization and perform standard first-order CPA/DPA on AES operations.
- The issue is silent (no alerts) and cannot be mitigated by SoC integrators
  because the parameter is hard-coded in the RTL.

Exploit Type: Exploit (enables first-order SCA by forcing constant masks)

Test Purpose
- This test demonstrates that software can enable constant masks via the
  FORCE_MASKS software control and that, when combined with a deterministic
  AES masking PRNG seed, the AES output matches the unmasked known-answer
  ciphertext. This proves that masking randomization can be disabled at
  runtime.

How it works (high-level)
1. Configure CSRNG and EDN so that the AES masking PRNG outputs an all-zero
   vector (deterministic masking state).
2. Initialize AES and start a transaction with `.force_masks = true`.
3. Trigger PRNG reseed and data-out-clear so the AES masking PRNG uses the
   deterministic seed.
4. Encrypt known plaintext blocks and compare produced ciphertext with the
   unmasked known-answer vector. A match indicates masking was effectively
   disabled.

Build Instructions
- From the OpenTitan workspace root run:
  bazel build //path/to/this:aes_force_masks_exploit_test

Run Instructions
- Run the test on a supported simulation or FPGA target:
  bazel test //path/to/this:aes_force_masks_exploit_test

Expected Result
- The test should pass: the produced ciphertext matches the known-answer
  unmasked vector, demonstrating that software-controlled FORCE_MASKS can
  disable masking and enable first-order SCA.

Notes
- The test re-uses existing testutils to program CSRNG/EDN and assumes access
  to AES test vectors defined in hw/ip/aes/model/aes_modes.h.
- This vulnerability allows local software with AES register write privileges
  to induce a weakened AES implementation that leaks via first-order power/
  EM measurements.
