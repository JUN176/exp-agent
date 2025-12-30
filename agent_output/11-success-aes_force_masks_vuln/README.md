Vulnerability: AES SecAllowForcingMasks Hard-wired to 1 (Vuln ID 11)

Affected module: aes

Summary
- The AES IP was instantiated with the parameter SecAllowForcingMasks set to 1
  in RTL, which enables the CSR path that allows software to force internal
  masking values (CTRL_AUX_SHADOWED.FORCE_MASKS). This overrides the secure
  top-level default of 0 and permanently enables the FORCE_MASKS capability.

Impact
- By enabling constant masks through the FORCE_MASKS CSR, software can disable
  the intended DOM masking randomization used inside the AES S-Box and key
  addition. This allows standard first-order CPA/DPA attacks to recover keys
  with relatively few traces. The condition is silent (no alerts) and cannot
  be corrected by SoC integration because the parameter is hard-overridden
  in the AES instantiation.

Exploit type: POC (demonstrates that software can set the FORCE_MASKS bit via
the CSR path; this proves the vulnerability. An SCA attack would be required
as a follow-up to recover keys.)

What this test does
1. Reads the AES CTRL_AUX_SHADOWED register and checks the FORCE_MASKS bit.
   - If the bit is already set, this immediately indicates the CSR path is
     available and enabled by default (matching the reported RTL issue).
2. If the bit is not set, the test attempts to set it from software by
   performing a shadowed write (double write) to
   AES_CTRL_AUX_SHADOWED and enabling the AUX REGWEN register.
3. Reads back the register to verify the FORCE_MASKS bit is set.

Base library APIs used
- mmio_region_from_addr / mmio_region_read32 / mmio_region_write32
- bitfield_bit32_read / bitfield_bit32_write
- LOG_INFO, CHECK, and OTTF test framework helpers

Why this proves the vulnerability
- The RTL issue described (SecAllowForcingMasks forced to 1) means the CSR
  path allowing FORCE_MASKS is always present and usable by software. This
  test shows that software-level writes to the AES auxiliary control registers
  can enable FORCE_MASKS. Once enabled, AES masking can be made constant
  enabling first-order side-channel leakage (CPA/DPA).

How to build
- Use Bazel in the repository root:
  bazel test //path/to/this/test:aes_vuln_test

Expected result
- The test should pass and log that FORCE_MASKS was observed set or that it
  was successfully set by software. Either outcome proves software control
  over the FORCE_MASKS path (the vulnerability).

Notes and mitigations
- The correct fix is to instantiate the AES IP with SecAllowForcingMasks=0 at
  the top-level so that the CSR path is not present. Removing the CSR path or
  making it inaccessible to software prevents this class of SCA weakening.
- For immediate mitigation, restrict software privilege so untrusted code
  cannot write the AES auxiliary registers, but this is fragile compared to
  correcting the RTL parameter.
