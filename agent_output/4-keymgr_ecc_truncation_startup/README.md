Vulnerability: keymgr ECC truncation on startup

Affected module: keymgr
Vulnerability ID: 4

Summary
-------
During key manager startup, an assignment to key_data is truncated in the RTL
(see keymgr_ctrl.sv lines around 312-313). The truncated assignment leads to
an ECC decoding/encoding failure which causes the key manager to report an
error and transition to an INVALID state, preventing normal operation.

PoC Description
---------------
This proof-of-concept triggers the startup advance operation using low-level
base library MMIO functions (mmio_region_*). It performs the following steps:

1. Constructs a CONTROL_SHADOWED register write with OPERATION=ADVANCE and
   DEST=NONE.
2. Writes CONTROL_SHADOWED (shadowed write) and then writes to START to
   trigger the operation.
3. Polls OP_STATUS for completion. If the operation reports DONE_ERROR, the
   test reads ERR_CODE and WORKING_STATE.
4. The PoC asserts that WORKING_STATE is INVALID after the error, which
   indicates the bug is present.

This test intentionally uses only base library primitives (mmio/bitfield/log)
for direct register interaction rather than DIF APIs to precisely control the
operation and observe hardware-visible registers.

Files
-----
- keymgr_vuln_test.c: C test that triggers the advance operation and checks
  for ECC error / INVALID state.
- BUILD: Bazel build rule to compile and run the test.

How to build and run
--------------------
# Build
bazel build //path/to/this:test

# Run
bazel test //path/to/this:test --test_output=all

Expected Result
---------------
The test should detect OP_STATUS == DONE_ERROR, ERR_CODE != 0, and the
WORKING_STATE field equal to KEYMGR_WORKING_STATE_STATE_VALUE_INVALID. On
successful detection the test returns PASS and logs the error code and state.

Notes
-----
- The PoC relies on register definitions provided by keymgr_regs.h (via
  the dependency //hw/top:keymgr_c_regs). It uses shadowed writes where the
  hardware requires them (CONTROL_SHADOWED and MAX_*_KEY_VER registers).
- If the DUT or testbench fixes the RTL truncation, the test will fail with
  "Bug not triggered" because the key manager will complete the operation
  successfully.
