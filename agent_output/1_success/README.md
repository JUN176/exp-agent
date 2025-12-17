AES FORCE_MASKS Hard-wired Parameter Vulnerability

Module: aes
Affected RTL: hw/ip/aes/rtl/aes.sv
Vulnerable line: 180 (instantiation of aes_core with SecAllowForcingMasks set to 1)

Vulnerability summary:
The top-level AES wrapper hard-wires the aes_core parameter SecAllowForcingMasks to 1, overriding the secure default of 0. This exposes a CSR (CTRL_AUX.SHADOWED.FORCE_MASKS) that allows software to force internal masking values to constants at runtime. When FORCE_MASKS is set, the DOM masking randomization can be disabled, reintroducing first-order side-channel leakage (power/EM). Any software able to write the AES auxiliary control register can enable this behaviour silently.

Exploit type:
Proof-of-Concept (demonstrates that software can set the FORCE_MASKS CSR at runtime).

What this POC does:
- Uses low-level base library MMIO APIs to directly access AES registers.
- Enables AUX register writes (AES_CTRL_AUX_REGWEN = 1).
- Performs a shadowed write to AES_CTRL_AUX_SHADOWED setting FORCE_MASKS = 1 (two writes as required for shadowed registers).
- Locks the AUX register (AES_CTRL_AUX_REGWEN = 0) to emulate normal software behaviour.
- Reads back the AUX shadowed register to verify that the FORCE_MASKS bit is set.

Why this proves the vulnerability:
If the AES core had the secure default (SecAllowForcingMasks=0) propagated, the FORCE_MASKS CSR path would not exist or would be inert. The ability to set and read back the FORCE_MASKS bit via software demonstrates that the CSR path is exposed and controllable at runtime. With this control, an attacker can force constant masks and perform standard first-order CPA/DPA attacks to recover keys with modest trace counts.

Base library APIs used:
- mmio_region_from_addr, mmio_region_read32, mmio_region_write32 (sw/device/lib/base/mmio.h)
- bitfield_bit32_read, bitfield_bit32_write (sw/device/lib/base/bitfield.h)
- abs_mmio (included for reference; primary code uses mmio_region_*)
- LOG_INFO (sw/device/lib/runtime/log.h)
- CHECK (sw/device/lib/testing/test_framework/check.h)

Assembly components:
None.

Build and run:
- Build:
  bazel build //path/to/this: aes_force_masks_poc_test

- Run (on simulation or FPGA as supported):
  bazel test //path/to/this: aes_force_masks_poc_test --test_output=all

Expected result:
The test should pass and log that FORCE_MASKS was set via the CSR path. This indicates that software has runtime control to force masking constants, demonstrating the vulnerability.

Technical notes:
- The POC intentionally uses low-level MMIO accesses rather than DIFs to precisely control the register write sequence (shadowed double-write, explicit REGWEN toggling).
- To fully exploit this vulnerability for key recovery, an attacker would additionally: seed the masking PRNG (via CSRNG/EDN) to produce known masks (e.g., all zeros), enable FORCE_MASKS, perform chosen AES encryptions, and run standard first-order CPA/DPA analysis on collected traces.
- The RTL fix should avoid hard-wiring SecAllowForcingMasks=1 at top-level and ensure the secure default (0) is enforced, or otherwise gate the CSR path behind higher-level lifecycle or root-of-trust checks.
