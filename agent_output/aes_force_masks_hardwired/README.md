Vulnerability: aes_force_masks_hardwired

Affected module: hw/ip/aes
RTL location: hw/ip/aes/rtl/aes.sv (aes core instantiation)
Problem summary:
- The AES top-level wrapper hard-wires the aes_core parameter SecAllowForcingMasks to 1
  (.SecAllowForcingMasks(1) at aes.sv:L180), overriding the secure top-level default of 0.
- This causes the FORCE_MASKS CSR (CTRL_AUX_SHADOWED.FORCE_MASKS) to be present and
  writable in silicon, allowing software to force internal masks to constant values.
- By forcing constant masks at runtime, the design removes the intended masking
  randomness and reintroduces first-order side-channel leakage for masked S-Box and
  key-addition operations. Any software that can write AES auxiliary control can enable
  FORCE_MASKS silently and perform first-order CPA/DPA to recover keys.

Exploit type: Proof-of-Concept (POC)
- Demonstrates that software can set the FORCE_MASKS bit via low-level MMIO writes.
- Does not perform side-channel measurement (out of scope for this test); it proves the
  attack surface exists and is exploitable by higher-level SCA tooling.

What this package contains:
- BUILD: Bazel test rule (opentitan_test) named aes_force_masks_poc_test.
- aes_force_masks_poc_test.c: Base-lib-only C test that uses mmio_region_* and bitfield
  helpers to set CTRL_AUX_SHADOWED.FORCE_MASKS and verifies the bit remains set.

Base library APIs used:
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()
- bitfield_bit32_write(), bitfield_bit32_read()
- LOG_INFO(), CHECK()

Why assembly/DIF are NOT used:
- The POC needs precise MMIO control; base lib APIs provide minimal, deterministic
  access. DIFs are intentionally avoided per the POC development policy.
- No privilege level switching, CSR manipulation, or timing-critical instruction
  sequences are required, so assembly is unnecessary.

How the test works (high level):
1. Create an mmio_region_t for the AES peripheral using TOP_EARLGREY_AES_BASE_ADDR.
2. Read current AES CTRL_AUX_SHADOWED register and construct a value with
   FORCE_MASKS bit set using bitfield_bit32_write().
3. Perform a shadowed write (write twice) to the CTRL_AUX_SHADOWED register and
   write AES_CTRL_AUX_REGWEN_REG_OFFSET = 1 to leave the AUX register unlocked.
   This follows the same sequence used by DIF helpers.
4. Read back the AUX shadowed register and verify the FORCE_MASKS bit is set.

Expected result:
- The test logs the before/after values of CTRL_AUX_SHADOWED and succeeds if
  the FORCE_MASKS bit can be set and read back. Success confirms the hardware
  exposes a writable FORCE_MASKS control despite the secure top-level default.

Build & run:
- From the OpenTitan workspace: bazel test //path/to/this/package:aes_force_masks_poc_test
- Use --test_output=all to see LOG_INFO outputs.

Security impact & mitigation notes:
- This is a high-severity SCA vulnerability: enabling FORCE_MASKS allows an attacker
  with write access to the AES auxiliary control to disable masking randomness and
  recover keys via standard first-order DPA/CPA.
- Root cause: parameter overriding in the AES top-level wrapper (.SecAllowForcingMasks(1)).
- Mitigation: Do not hard-wire SecAllowForcingMasks to 1 in the top-level wrapper.
  The parameter should be left to the SoC integrator or set to 0 for secure builds.
  Additionally, consider hardware mechanisms to tie FORCE_MASKS presence to secure
  lifecycle states or to alert/latch when such a dangerous option is enabled.

Technical references:
- RTL: hw/ip/aes/rtl/aes.sv lines ~160-190 (aes_core instantiation)

Notes:
- This POC intentionally performs only register writes/reads to prove the control
  is available. Performing side-channel analysis requires measurement hardware and
  is outside the scope of this software-only test.
