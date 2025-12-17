Vulnerability: Entropy ingestion gated by CONF.entropy_data_reg_enable (ID 21)

Affected module: entropy_src

Summary
-------
The RTL at entropy_src_core.sv (lines ~812-814) implements:
  assign es_rng_src_valid = entropy_src_rng_i.rng_valid & (reg2hw.conf.entropy_data_reg_enable.q != '0);

This uses the raw register Q value (a 4-bit MuBi field) in a simple non-zero check to gate RNG ingress. The check bypasses the synchronized, strictly decoded MuBi fanout and treats any non-zero pattern as true. As a result, firmware (or a single-bit upset) can write a zero pattern to the entropy_data_reg_enable field and suppress ingestion of raw entropy even while the RNG is enabled — effectively creating a denial-of-service for entropy consumers.

Exploit Type
------------
Proof-of-Concept (DoS): this test demonstrates that clearing the CONF.entropy_data_reg_enable field stops entropy samples from being updated, proving that software can starve the entropy pipeline.

What the POC does
-----------------
1. Observes that entropy samples are changing (precondition).
2. Ensures CONF.entropy_data_reg_enable is non-zero (set to 1 if needed).
3. Writes zero into CONF.entropy_data_reg_enable (the buggy gating control).
4. Verifies that entropy samples stop changing (DoS condition).
5. Restores the original field value.

Files in this package
---------------------
- BUILD: Bazel build rule (opentitan_test) named "entropy_src_vuln_test".
- entropy_vuln_test.c: C test implementing the POC using base library APIs.

Base library APIs used
---------------------
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()  (//sw/device/lib/base:mmio)
- bitfield_field32_t, bitfield_field32_read(), bitfield_field32_write()  (//sw/device/lib/base:bitfield)
- LOG_INFO() (//sw/device/lib/runtime:log)
- OTTF test framework for test entry (//sw/device/lib/testing/test_framework:ottf_main)

Why assembly is not needed
-------------------------
The exploit only requires MMIO writes/reads to the configuration register and polling the entropy data register. No privilege switching or precise instruction sequences are necessary.

How to build and run
--------------------
From the repository root (example):
  bazel test //sw/device/tests:entropy_src_vuln_test

Expected results
----------------
- The test should observe changing entropy samples initially.
- After writing zero to the CONF.entropy_data_reg_enable field, the test should observe that the entropy sample value stops changing (within the polling window), indicating ingestion has been gated.

Notes and caveats
-----------------
- The test assumes ENTROPY_SRC is enabled (ROM normally enables it). If ENTROPY_SRC is disabled in the test environment, the initial observation of changing samples may fail.
- The test uses reggen-provided macros from entropy_src_regs.h (included via //hw/top:entropy_src_c_regs) for register offsets and field masks/offsets.
- This POC demonstrates DoS; it does not recover or exploit secret leakage.

References
----------
- RTL snippet: entropy_src_core.sv lines 812-814
- Vulnerability ID: 21
