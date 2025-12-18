Vulnerability PoC: LC_CTRL hash truncation (Vuln ID 3)

Affected module: lc_ctrl (Lifecycle Controller)
RTL location: hw/ip/lc_ctrl/rtl/lc_ctrl_fsm.sv (lines 456-459)

Summary:
The LC controller FSM compares only the lower 32 bits of two 128-bit hash
values (hashed_token_i and hashed_token_mux) before allowing a transition to
FlashRmaSt/RMA. The vulnerable RTL snippet is:

  if (hashed_token_i[31:0] == hashed_token_mux[31:0] &&
      !token_hash_err_i &&
      &hashed_token_valid_mux) begin
    fsm_state_d = FlashRmaSt;

This reduces the effective protection from 2^128 to 2^32 for token guessing.

PoC Description:
This package provides a simulation-only proof-of-concept test that demonstrates
how an attacker could exploit the truncation bug. The test writes a 128-bit
transition token to the LC_CTRL token registers with the lower 32-bit word set
to a known constant (EXPECTED_HASH_LOW32). It then requests a transition to
RMA. If the DUT only compares the lower 32 bits of the hash, the transition
will succeed and the test observes the LC state change.

Important Simulation Assumptions (required for this PoC):
- The simulation testbench must backdoor the internal expected hashed token
  lower-32 value (hashed_token_mux[31:0]) to the constant EXPECTED_HASH_LOW32
  used in the test (default 0xDEADBEEF).
- The testbench must ensure token_hash_err_i == 0 and hashed_token_valid_mux
  is asserted. Typically this means the testbench preloads OTP / KMAC state
  appropriately or forces these internal signals via backdoor.

Why these assumptions?
The internal hashed_token_mux / hashed_token_valid_mux signals are not
software-visible. A silicon exploit would attempt a brute-force search over
2^32 values to match the truncated 32-bit comparison. In a simulation
environment we instead set the expected value deterministically to show the
logic flaw and the fact that only 32 bits are compared.

Files in this package:
- lc_ctrl_hash_truncation_v3_test.c : The C PoC test (uses base mmio APIs).
- BUILD : Bazel build file for the ottf test target.

Base library APIs used:
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()
  (sw/device/lib/base/mmio.h)
- bitfield_field32_read(), bitfield_bit32_write() (sw/device/lib/base/bitfield.h)
- kMultiBitBool8True / False (sw/device/lib/base/multibits.h)
- OTTF test framework (sw/device/lib/testing/test_framework/ottf_main)
- LOG_INFO, CHECK macros (runtime and test framework)

How to run (simulation):
1. Ensure your simulation testbench/backdoor sets the expected hashed_token_mux
   lower-32 value to 0xDEADBEEF (or change EXPECTED_HASH_LOW32 in the C file).
2. Build and run the test in the simulation environment as an OTTF test.

Expected result:
- If the DUT only compares the lower 32 bits: the test will observe a state
  transition to RMA and return success (vulnerability confirmed).
- If the DUT compares the full 128-bit hash correctly: no transition will be
  observed and the test will fail (no vulnerability in that build).

Notes on real-world exploitation:
- On actual silicon, the attacker would need to brute-force the 32-bit space
  (2^32) to find a matching hash suffix, which is feasible compared to 2^128.
- Fixes should ensure the full 128-bit hash comparison is implemented, and
  security annotations / formal checks are updated to prevent regressions.

