AES FORCE_MASKS Backdoor PoC

Vulnerability name: aes_force_masks_backdoor
Affected module: aes (hw/ip/aes)

Summary
- The AES core instantiation in hw/ip/aes/rtl/aes.sv hard-wires the parameter
  SecAllowForcingMasks to 1, overriding the secure top-level default of 0.
- This enables the FORCE_MASKS CSR path at runtime. Software that can write the
  AES auxiliary control register can set FORCE_MASKS=1, forcing internal masks
  to constant values and disabling the masking randomization designed to
  mitigate first-order side-channel leakage.
- By setting FORCE_MASKS and locking the AUX register, software can ensure
  subsequent AES operations use constant masks, reintroducing first-order
  leakage and enabling powerful CPA/DPA attacks.

Exploit Type: Proof-of-Concept (demonstrates software control of FORCE_MASKS
and ability to lock the field; does not perform side-channel measurements)

What this PoC does
1) Uses base library MMIO to access AES registers directly (no DIF).
2) Performs a shadowed write to AES_CTRL_AUX_SHADOWED to set the
   AES_CTRL_AUX_SHADOWED_FORCE_MASKS bit to 1.
3) Writes AES_CTRL_AUX_REGWEN_REG_OFFSET = 0 to lock the AUX control register
   (preventing further software changes).
4) Attempts to clear FORCE_MASKS and verifies the bit remains set. This shows
   that software successfully enabled and froze the FORCE_MASKS behavior.

Key base library APIs used
- mmio_region_from_addr()
- mmio_region_read32()/mmio_region_write32()
- bitfield_bit32_write()/bitfield_bit32_read()
- LOG_INFO(), CHECK(), OTTF test framework

Files in this package
- BUILD: Bazel test rule (opentitan_test) named aes_force_masks_backdoor_test
- aes_force_masks_backdoor_test.c: The C PoC described above

Build
bazel build //path/to/this:ae s_force_masks_backdoor_test

Run
bazel test //path/to/this:aes_force_masks_backdoor_test --test_output=all

Expected result
- The test logs should show that the AES_CTRL_AUX_SHADOWED_FORCE_MASKS bit
  was written and that it remained set after locking the AUX register. The
  test will PASS if this behavior is observed, demonstrating the vulnerability.

Technical details
- RTL: hw/ip/aes/rtl/aes.sv sets .SecAllowForcingMasks(1) when instantiating
  aes_core (line ~180), enabling the FORCE_MASKS CSR irrespective of the top-
  level secure default. Since this is a hard override at instantiation time,
  SoC integrators cannot disable the CSR via parameters.
- The PoC uses the same register names/offsets as the driver/DIF (aes_regs.h)
  and performs the shadowed write sequence used by the legitimate driver, but
  directly via base MMIO to show software-level control.

Notes
- This PoC does not perform side-channel traces or key recovery. Its purpose
  is to demonstrate software control over FORCE_MASKS and the inability to
  remove that CSR path due to the hard-wired parameter.
- To perform an actual side-channel key recovery, an attacker would enable
  FORCE_MASKS, run chosen plaintext AES encryptions, and collect power/EM
  traces for CPA/DPA analysis.

References
- hw/ip/aes/rtl/aes.sv (SecAllowForcingMasks instantiation)

Contact / Attribution
- OpenTitan security research team
