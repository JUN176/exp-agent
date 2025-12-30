Title: KMAC key-ready bypass POC (Vulnerability 32)

Affected module: kmac (hw/ip/kmac)

Vulnerability summary:
The KMAC core FSM transitions from StKmacIdle to StKey when (kmac_en_i && start_i) without checking key_valid_i. As a result, the core may enable the key write and datapath (en_key_write / en_kmac_datapath) even when the key material has not been properly provisioned or validated, allowing operations to proceed under stale or uninitialized keys (CWE-665).

POC goal:
Demonstrate that KMAC can be started (enter ABSORB state) without programming or validating a key, proving that the start path is not gated by key_valid and that the datapath/key write can be enabled under potentially stale key material.

Test strategy:
- Use low-level base library APIs (mmio_region_*) to configure KMAC and issue a START command.
- Intentionally do NOT write any key shares to the KMAC key registers.
- Configure the KMAC mode to KMAC and issue the START command.
- Poll the KMAC STATUS register for the ABSORB bit. If the core enters ABSORB, the vulnerability is confirmed.
- If the core raises a KMAC error (KMAC_INTR_STATE.KMAC_ERR) instead, the hardware rejected the start and the POC fails.

Why base APIs:
The POC uses mmio_region_* and bitfield_* functions from the base library per the POC development philosophy: low-level deterministic register access is required to precisely craft the start condition and observe raw status/intr registers. DIFs are intentionally not used.

Files in the package:
- kmac_key_ready_bypass_poc.c  : C test that implements the POC.
- BUILD                       : Bazel build file for running the test under OpenTitan test infra.
- README.md                   : This document (explanations + instructions).

How to build and run:
1) From the OpenTitan workspace root, build the test:
   bazel build //sw/device/tests:kmac_v32_poc_test

2) Run the test on simulator (example):
   bazel test //sw/device/tests:kmac_v32_poc_test --test_output=all

Expected result:
- Vulnerable HW: The test will detect that KMAC entered the ABSORB state without a valid key and will return PASS while logging "VULNERABLE: KMAC entered ABSORB without a programmed/validated key.".
- Patched/Protected HW: The test will observe a KMAC error or the core will not enter ABSORB; the test will FAIL asserting that the hardware did not allow start without a valid key.

Notes and limitations:
- The POC assumes register names and bitfield constants provided by the auto-generated kmac_regs.h (dependency //hw/top:kmac_c_regs). These are used directly in the test.
- The test does not attempt to read internal signals like en_key_write or en_kmac_datapath directly (these are internal to the IP). Instead it uses the observable STATUS and INTR_STATE registers to infer whether the datapath progressed.
- On FPGA targets timing may differ; TIMEOUT in the test may be adjusted if needed.

APIs used:
- mmio_region_from_addr, mmio_region_read32, mmio_region_write32
- bitfield_field32_write, bitfield_bit32_read
- LOG_INFO, CHECK

Reference RTL excerpt (opentitan/hw/ip/kmac/rtl/kmac_core.sv lines 153-188):
- assign unused_signals = ^{mode_i, key_valid_i};
- StKmacIdle transition to StKey on (kmac_en_i && start_i) without gating on key_valid_i
- StKey asserts en_kmac_datapath and en_key_write (enabling datapath/key writes)

Author: Automated POC generator
Vulnerability ID: 32
