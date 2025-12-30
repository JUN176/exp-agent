AES Reset Retention Vulnerability (CWE-212) - POC

Vulnerability ID: 15
Module: aes
Affected RTL: hw/ip/aes/rtl/aes_core.sv (lines ~872-878)

Summary
-------
The AES core's output flip-flops (data_out_q) use an asynchronous reset that is incorrectly guarded by the sparse-encoded write-enable (data_out_we). The RTL pattern:

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni && data_out_we != SP2V_HIGH) begin
      data_out_q <= '0;
    end else if (data_out_we == SP2V_HIGH) begin
      data_out_q <= data_out_d;
    end
  end

means that when rst_ni == 0 and data_out_we == SP2V_HIGH the flop will load data_out_d instead of clearing. This ties reset semantics to a control signal and allows retention/propagation of sensitive AES output across resets.

Exploit Type
-----------
Proof-of-Concept (POC) demonstrating retention of AES output across peripheral reset.

Approach
-------
1. Start an AES encryption to produce a valid output (so internal data_out_we becomes asserted when output is ready).
2. As soon as OUTPUT_VALID is observed, request a peripheral software reset for the AES block via the Reset Manager.
3. Because the RTL gates the asynchronous reset with data_out_we, in simulation the reset may overlap the data_out_we == SP2V_HIGH condition and cause data_out_q to be loaded with data_out_d rather than cleared.
4. After releasing the peripheral reset, read the AES DATA_OUT registers. Any non-zero values indicate retention/leakage.

Files in this package
---------------------
- aes_vuln_test.c : C test implementing the POC using base mmio APIs and DIF helpers for device init + reset control.
- BUILD : Bazel target to build/run the test in OpenTitan environment.

Base Library & APIs used
------------------------
- mmio_region_from_addr / mmio_region_read32 / mmio_region_write32 (sw/device/lib/base/mmio)
- bitfield_field32_write (sw/device/lib/base/bitfield)
- LOG_INFO, CHECK, OTTF test harness (sw/device/lib/runtime, sw/device/lib/testing)
- DIF interfaces used for convenience:
  - dif_aes_* helpers to configure/load/read AES
  - dif_rstmgr_* to assert/release peripheral software reset

Why DIF is used
---------------
DIFs are used only for device initialization and to trigger peripheral software reset via the Reset Manager (dif_rstmgr). All low-level register accesses relevant to POC observation are done via base APIs or by the DIF implementations. Using DIF here simplifies the test code and closely mirrors realistic software that would interact with the hardware.

How to build & run
------------------
From the OpenTitan workspace root:

  bazel test //path/to/this/package:aes_vuln_test --test_output=all

(Replace //path/to/this/package with the actual location where the exploit package is placed under the repository.)

Expected result
---------------
- If the vulnerability is present in the DUT, the test will observe non-zero values in the AES DATA_OUT registers after a peripheral reset and will log "VULNERABILITY DETECTED: AES DATA_OUT retained non-zero data across reset" and pass the CHECK that demonstrates the issue.
- If the hardware clears outputs correctly on reset, DATA_OUT will be all zeros and the CHECK will fail, indicating the POC did not observe retention.

Technical notes
---------------
- The POC is timing sensitive; it races asserting peripheral reset against the AES output write-enable. In simulation environments (sim_dv / verilator) the timing window can be observed and controlled. In real silicon the window might require fault injection or glitching to reproduce reliably.
- The root cause is incorrect reset-dominance: the asynchronous reset is gated by a control signal which may be X or valid-high during reset, violating the expectation that reset always forces a known state.

References
---------
- AES RTL: hw/ip/aes/rtl/aes_core.sv L872-L878
- CWE-212: Improper Removal of Sensitive Information
