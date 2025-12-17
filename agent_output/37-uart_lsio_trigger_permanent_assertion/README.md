Vulnerability: UART LSIO Trigger Permanently Asserted (Vuln ID: 37)

Module: uart
Affected RTL: hw/ip/uart/rtl/uart_core.sv (L356-L364)

Summary
-------
The uart_core module contains a registered output `lsio_trigger_o` which is
intended to be asserted when FIFO watermark or timeout conditions indicate
that a downstream LSIO DMA transfer should occur. The RTL implements this
signal as a constant-1 after reset (i.e., it is set to 0 on reset and then
immediately driven to 1 on the next clock), effectively collapsing the DMA
request into a permanent "not-in-reset" indicator. This bypasses the intended
watermark/timeout/enable logic and can keep a DMA channel continuously active,
leading to denial-of-service, data integrity issues, or memory corruption if
DMA addressability is unconstrained.

Why this POC
------------
Firmware cannot directly modify RTL behavior, but it can observe the effect of
hardware outputs that have been mapped into the system register space. Some
platform integrations expose the LSIO DMA handshake (or a related status bit)
via a memory-mapped status register in the UART register window. This POC
reads a candidate status register within the UART region and logs whether the
LSIO trigger is persistently asserted after reset.

Contents
--------
- uart_vuln_test.c : C test which reads UART registers and an assumed HW->REG
  status reflecting lsio_trigger_o.
- BUILD            : Bazel test rule for building the POC.

How it works
-----------
1. The test reads baseline UART registers (STATUS, CTRL, FIFO_CTRL) and logs
   their values.
2. It then repeatedly reads a hypothetical status register at
   UART_STATUS_REG_OFFSET + 0x10 where platform integrators may map the
   `lsio_trigger_o` bit (this offset is a reviewer-adjustable value).
3. If the test observes the bit asserted on every read (never deasserted) the
   test fails with an assertion pointing to VULN-37.

Notes for reviewers / integrators
--------------------------------
- The exact register/bit used by a platform to expose `lsio_trigger_o` is
  platform dependent. If your platform exposes the handshake in a different
  register offset, edit UART_HW2REG_LSIO_TRIGGER_REG_OFFSET and
  UART_HW2REG_LSIO_TRIGGER_BIT in uart_vuln_test.c accordingly.
- The test uses only base library APIs (mmio_region_read32, mmio_region_from_addr,
  LOG_INFO, CHECK) to adhere to the POC development philosophy.

Expected Results
----------------
- Vulnerable integration (lsio_trigger_o tied high after reset): the test will
  repeatedly observe the LSIO status bit asserted and fail with
  "LSIO trigger remained asserted across observations -> VULN-37".
- Fixed integration: the test will observe both asserted and deasserted
  states (or only deasserted) and will pass, indicating the handshake is
  governed by watermark/timeout/enable logic as intended.

Building & Running
------------------
- bazel build //path/to:uart_vuln_test
- bazel test //path/to:uart_vuln_test --test_output=all

Base library APIs used
----------------------
- mmio_region_from_addr(), mmio_region_read32()  (//sw/device/lib/base:mmio)
- abs_mmio_read32/abs_mmio_write32 are available but not required here
- LOG_INFO (//sw/device/lib/runtime:log)
- OTTF test framework (//sw/device/lib/testing/test_framework:ottf_main)

References
---------
- RTL excerpt (opentitan/hw/ip/uart/rtl/uart_core.sv L356-L364):
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      lsio_trigger_o <= 1'b0;
    end else begin
      // Simplified assertion for static data path enablement
      // TX/RX watermark logic handled within downstream DMA FSM
      lsio_trigger_o <= 1'b1;
    end
  end

Acknowledgements
----------------
Report and analysis by hardware security team. Please adjust the register
mapping in the POC to match your platform's integration of the LSIO handshake
signal.
