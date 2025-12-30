Vulnerability: OTBN IMEM/DMEM Blanking Disabled (vuln id: 10)

Module: otbn
Affected file: hw/ip/otbn/rtl/otbn.sv
Affected line: 448

Summary
-------
The prim_blanker enable signals for IMEM and DMEM were hardcoded to '1' in the RTL
(assign imem_rdata_bus_en_d = 1'b1;), causing the blanker to always pass data
through. During OTBN execution this should be blocked to prevent leakage. With
this bug, a privileged bus master (like Ibex/host) can read IMEM and DMEM while
OTBN is busy, leaking sensitive instructions and data.

Exploit Type
------------
Proof-of-Concept (POC): Demonstrates that IMEM and DMEM can be read while the
OTBN core is reporting BUSY_EXECUTE, proving the blanker is ineffective.

Test Strategy
-------------
1. Initialize OTBN using DIF (dif_otbn_init).
2. Write known patterns into IMEM and DMEM using dif_otbn_imem_write and
   dif_otbn_dmem_write.
3. Start OTBN execution using dif_otbn_write_cmd(kDifOtbnCmdExecute).
4. While OTBN status reports BUSY_EXECUTE, repeatedly attempt to read IMEM and
   DMEM using dif_otbn_imem_read and dif_otbn_dmem_read.
5. If reads return the previously written patterns while OTBN is still busy,
   the blanker is disabled and leakage is confirmed.

Base library APIs used
----------------------
- dif_otbn_init, dif_otbn_write_cmd, dif_otbn_get_status,
  dif_otbn_imem_write/dif_otbn_imem_read, dif_otbn_dmem_write/dif_otbn_dmem_read
  (DIF used for device control and memory access for convenience)
- mmio_region_from_addr (indirectly via DIF initialization)
- LOG_INFO, CHECK macros from the OTTF test framework

Assembly
--------
No assembly is required for this POC.

How to build and run
--------------------
From the OpenTitan repo root run (example):
  bazel test //sw/device/tests:otbn_blanking_vuln_test

Expected Results
----------------
- The test should detect that IMEM and DMEM are readable while OTBN reports
  BUSY_EXECUTE and assert success, indicating the blanker is not preventing
  reads during execution.

Notes
-----
- This POC uses DIF functions for convenience to interact with OTBN memory and
  control registers. The vulnerability itself is in hardware (otbn.sv) where the
  blanker enable signals are incorrectly tied high. The POC demonstrates
  practical impact: leakage of instruction and data memory during execution.
- The test includes timeouts and logging to help debugging in simulation.
