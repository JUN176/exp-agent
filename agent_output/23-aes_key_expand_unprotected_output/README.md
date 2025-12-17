Vulnerability: AES KeyExpand Unprotected Output (ID 23)

Affected Module: hw/ip/aes (aes_key_expand.sv)
Vulnerable RTL Location: hw/ip/aes/rtl/aes_key_expand.sv line 441

Summary
-------
The aes_key_expand module assigns internal round-key material to its output port
`key_o` via a direct combinational assignment `assign key_o = regular;`.
There is no gating by validity signals (e.g., OUTPUT_VALID) nor zeroization of
these outputs after reset. This allows internal round-key values (or deterministic
masks/PRD values) to be exposed throughout AES operation, enabling PRD reuse,
deterministic zero windows, and making side-channel attacks (trace averaging,
DPA/CPA) significantly easier.

Exploit Type: Proof-of-Concept (POC)
This test demonstrates that the key expansion outputs are observable via MMIO
register reads and are not zeroized/gated as would be expected for sensitive
internal state.

Files in this package
---------------------
- aes_key_expand_vuln_test.c : C test that reads AES key-related registers using
  low-level base library MMIO APIs and reports non-zero values indicating
  leakage.
- BUILD                     : Bazel rule to build the test (opentitan_test)
- README.md                 : This file

Design of the POC
-----------------
Test Plan:
1. Use mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR) to create an MMIO
   region for the AES peripheral.
2. Trigger AES clearing of key/IV/data_out registers using AES_TRIGGER.
   (This mirrors the normal clearing sequence but does not request PRD updates
   or reseeding.)
3. Without triggering PRD writes (i.e., before reseed/prd_we), read the
   registers that correspond to the AES KeyExpand outputs (key_o) using low-level
   mmio reads.
4. If the registers contain non-zero (or otherwise sensitive) words, this
   indicates internal key schedule material is exposed without validity gating
   or zeroization.

Base library APIs used
----------------------
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()
  (sw/device/lib/base/mmio.h)
- abs_mmio_* fallback (sw/device/lib/base/abs_mmio.h)
- LOG_INFO() (sw/device/lib/runtime/log.h)
- CHECK() (sw/device/lib/testing/test_framework/check.h)

Why base APIs
-------------
The POC intentionally uses the low-level base APIs for precise control over
MMIO reads/writes and to avoid DIF-level abstractions which may hide timing or
sequence details relevant to the vulnerability trigger.

How to build and run
--------------------
From the repository root:

  bazel build //path/to/package:aes_key_expand_poc_test
  bazel test  //path/to/package:aes_key_expand_poc_test --test_output=all

Note: The BUILD rule depends on the generated register header target
`//hw/top:aes_c_regs` which provides aes_regs.h.

Expected Results
----------------
- The test will log the 8 words of key-related outputs for both shares (or the
  input key shares if the exact expanded-output registers are not exposed).
- A successful demonstration is when these values are non-zero and readable
  immediately after reset/clear without any PRD reseed. This indicates the
  internal round-key material is observable and not gated/zeroized as required.

Technical details
-----------------
- Vulnerable line: assign key_o     = regular; (aes_key_expand.sv:441)
- The absence of a clocked register or validity gating means `key_o` changes
  combinationally with inputs (key_i) and internal irregular values (rcon,
  sub_word_out). There is no explicit zeroization on reset nor masking with a
  validity signal.

Mitigation
----------
- Gate the `key_o` outputs behind a validity signal (e.g., only drive key_o
  when OUTPUT_VALID / KEY_READY is asserted), and ensure zeroization of outputs
  on reset and on state transitions where the expanded key must not be
  observable.
- Consider adding a register stage or secure buffer that only latches
  key_o when the core indicates the key schedule output is intended to be
  used internally, not exposed externally.

References
----------
- hw/ip/aes/rtl/aes_key_expand.sv

