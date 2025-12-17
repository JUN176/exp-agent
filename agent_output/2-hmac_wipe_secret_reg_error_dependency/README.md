Vulnerability: HMAC secret wipe only enabled when reg_error is set

Affected module: hmac
Vulnerability ID: 2

Summary
- The HMAC IP exposes a WIPE_SECRET register that software writes to with
  an entropy value to randomize internal secret state (key, hash state).
- Due to a hardware gating condition (wipe_secret_we tied to reg_error), the
  wipe has effect only when the HMAC error status is set. Under normal
  operation this means writes to the wipe register are ignored and the
  secret remains intact. An attacker can thus reuse a previous user's key.

Exploit Type
- Proof-of-Concept (POC): Demonstrates that wiping the HMAC secret without
  an active error does not clear the key, while performing the same wipe
  during an error does.

What the test does
1. Loads a known HMAC key into the HMAC secret registers and computes a
   baseline HMAC over a fixed message.
2. Writes to the WIPE_SECRET register with a chosen entropy value while the
   device is in a normal (non-error) state, then reads the digest registers
   that the software API expects to be overwritten by the wipe operation.
3. Starts a new HMAC operation WITHOUT reloading the key. If the wipe had
   worked, the digest would differ. If it didn\'t, the digest will match the
   baseline (demonstrating the secret was not wiped).
4. Induces a software-visible error (disable SHA and write HASH_START), then
   writes the WIPE_SECRET register again with a different entropy value and
   verifies the digest registers are overwritten.

Base library APIs used
- mmio_region_from_addr / mmio_region_read32 / mmio_region_write32
- mmio_region_write8
- mmio_region_nonatomic_set_bit32
- abs_mmio_write32
- bitfield_field32_read / bitfield_bit32_write
- read_32 (memory.h)

Why this proves the vulnerability
- The test requires the WIPE_SECRET write to actually clobber HMAC internal
  state and digest registers. The POC shows that the clobber only occurs when
  an error is present. In normal conditions a subsequent HMAC run without
  reloading the key produces the same digest as before the wipe — showing
  the key was not cleared and can be reused.

How to build & run
- Ensure the repo is configured for simulation (or appropriate target).
- From the directory containing the generated BUILD file run:
  bazel test //path/to:hmac_wipe_vuln_test

Expected results
- The test asserts that:
  * Wipe in normal condition does NOT overwrite digest registers.
  * Wipe while an error is present DOES overwrite digest registers.
  * A subsequent HMAC without reloading the key matches the baseline when
    the wipe failed.

Notes
- This POC uses only base library APIs (mmio/abs_mmio/bitfield) to ensure
  precise control over register accesses and avoid DIF abstractions.
- The test relies on generated hmac_regs.h (provided by the target
  //hw/top:hmac_c_regs). Ensure that target is available in the Bazel
  invocation.

References
- Reported vulnerability description: wipe_secret_we only true when reg_error
  is true, preventing normal secret wipe.
