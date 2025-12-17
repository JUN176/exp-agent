Vulnerability: LC_CTRL Hash Truncation (Vuln ID 3)

Affected module: lc_ctrl (Lifecycle Controller)

Summary
-------
The lifecycle controller FSM in hardware compares only the lower 32 bits of two 128-bit hashed tokens (hashed_token_i and hashed_token_mux) instead of the full 128-bit cSHAKE output. This reduces the effective security from 2^128 to 2^32, enabling an attacker to craft tokens that differ in the high 96 bits but share the lower 32 bits and successfully perform sensitive lifecycle transitions (e.g., to DEV or RMA).

Exploit Type
------------
Proof-of-Concept (POC) demonstrating that a forged token that preserves only the lowest 32 bits of the genuine token can trigger a lifecycle transition.

Files in this package
---------------------
- lc_ctrl_vuln_test.c : C test that programs a forged token (high 12 bytes arbitrary, low 4 bytes matching expected token) and attempts an LC transition.
- BUILD               : Bazel test rule to build the test.
- README.md           : This file.

POC Strategy
------------
1. The DV environment typically backdoors the OTP so the expected token is known to the testbench. We declare a `kExpectedLcToken` symbol (volatile const) which the DV can override; it is initialized with a deterministic value so the test also runs in standalone simulation.
2. Acquire the LC mutex via MMIO so software can program transition-related registers.
3. Construct a forged 128-bit token where bytes [0..11] are arbitrary and bytes [12..15] (the lowest 32 bits) are copied from the expected token.
4. Program the token into LC_CTRL_TRANSITION_TOKEN_{0..3} registers and set the transition target (DEV in this POC).
5. Issue the TRANSITION command and poll LC state. If the state changes to DEV, the exploit succeeded.

Base library APIs used
----------------------
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32() (mmio)
- bitfield_bit32_write(), bitfield_field32_read() (bitfield)
- memcpy() (memory)
- LOG_INFO(), CHECK() (runtime/test framework)

Why this demonstrates the vulnerability
---------------------------------------
The RTL bug restricts the comparison to the lower 32 bits of the hash, which means an attacker only needs to match 32 bits of the hash to pass the check. By ensuring the forged token's lower 32 bits equal the genuine token's lower 32 bits (which in DV is a known/testbench-controlled value), the test shows the device performs a sensitive lifecycle transition despite the token being incorrect in the upper 96 bits.

How to build and run
--------------------
From the OpenTitan repository root (example):

  bazel test //opentitan/sw/device/tests:lc_ctrl_vuln_test

Notes
-----
- This POC assumes a simulation/DV environment where the expected token can be backdoored or is known to the testbench (this mirrors existing LC_CTRL tests). In real silicon, an attacker would need to brute-force 32 bits rather than 128, which is practically feasible with sufficient resources.
- No DIF APIs are used for the exploit logic; only base library MMIO and bitfield helpers are used as recommended.

