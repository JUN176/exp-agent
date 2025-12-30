Vulnerability POC: rng_bit_sel masked by health_test_window[0]

Vulnerability ID: 25
Module: entropy_src
RTL location: hw/ip/entropy_src/rtl/entropy_src_core.sv
Line(s): 2423-2424

Summary
-------
The RTL contains the assignment:
  assign rng_bit_sel = reg2hw.conf.rng_bit_sel.q & {1'b1, health_test_window[0]};

This AND-mask forces rng_bit_sel[0] to zero when health_test_window[0] == 0,
reducing the effective single-bit lane selector space and creating a hidden
functional coupling between the CONF.rng_bit_sel CSR and the
HEALTH_TEST_WINDOW CSR. Because health_test_window is not MUBI-hardened, it
can coerces the effective selector even if rng_bit_sel is write-locked.

Impact
------
- Deterministic reduction of selectable lanes lowers entropy if lane qualities
  differ (CWE-330).
- Violates CSR isolation and lock intent (REGWEN bypass via another CSR).
- Can cause noncompliance with entropy evaluation standards.

Exploit Type
-----------
POC: Demonstrates that toggling health_test_window[0] changes the effective
rng_bit_sel LSB and hence the selectable lane, enabling a software-driven
steering of the single-bit source. With additional sampling and statistics,
this can show a measurable reduction in entropy.

What the POC does
-----------------
- Uses base library mmio APIs to configure the entropy_src CONF register and
  the HEALTH_TEST_WINDOW register.
- Writes rng_bit_sel = 3 and shows how clearing health_test_window[0]
  forces the effective selection to 2 (LSB cleared) per the RTL mask.
- Configures route_to_firmware and single-bit-mode so firmware-visible
  outputs can be sampled to verify the behavioural change.

Files
-----
- entropy_src_vuln_test.c : C test implementing the POC (uses base lib APIs)
- BUILD : Bazel build rule to compile the test

Important notes
---------------
- The test uses the auto-generated register header provided by
  //hw/top:entropy_src_c_regs. The header included in the C code is
  "entropy_src_regs.h" and must match the generated file name for your top.
- Register/field macro names used in the POC are the common autogen names
  (e.g. ENTROPY_SRC_CONF_REG_OFFSET, ENTROPY_SRC_CONF_RNG_BIT_SEL_MASK,
  ENTROPY_SRC_CONF_RNG_BIT_SEL_OFFSET, ENTROPY_SRC_HEALTH_TEST_WINDOW_REG_OFFSET).
  If your environment has different names, update the C file accordingly.

How to verify
-------------
1. Build the test with Bazel:
   bazel build //path/to:entropy_src_vuln_test

2. Run the test on your simulation or hardware target.

3. Follow these steps inside the test or manually adapt the sequence:
   a) Set CONF.rng_bit_sel = 3 (attempt selecting lane 3).
   b) Ensure HEALTH_TEST_WINDOW[0] = 1. Sample observe FIFO outputs and
      record statistics.
   c) Set HEALTH_TEST_WINDOW[0] = 0. Sample observe FIFO outputs again.
   d) Compare samples; if lane 3 quality differs from lane 2, you will
      observe a change consistent with the LSB being forced to 0.

Expected result
---------------
- With HEALTH_TEST_WINDOW[0] == 1, rng_bit_sel writes to select lane 3 are
  honored (effective selection 3).
- With HEALTH_TEST_WINDOW[0] == 0, the hardware forces rng_bit_sel[0] = 0,
  making the effective selection 2 when software had written 3. This
  demonstrates the coupling.

Mitigation
----------
- Do not use health_test_window[0] as a mask for rng_bit_sel. Remove the
  coupling and ensure rng_bit_sel is only controlled by its CSR.
- Protect health_test_window via MUBI/hardening if it must interact with
  selection logic.
- Ensure REGWEN locks cannot be overridden by unrelated CSRs.

Base library APIs used
----------------------
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()
- bitfield_field32_write()
- LOG_INFO() for logging

Disclaimer
----------
- This POC focuses on demonstrating the register-level coupling; depending on
  your simulation/hardware environment you may need to adjust register/field
  macro names to match the generated headers.
