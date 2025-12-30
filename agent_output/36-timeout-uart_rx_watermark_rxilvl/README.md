Vulnerability: UART RX FIFO watermark threshold mis-calculation (Vuln ID 36)

Module: uart

Summary:
The UART RX watermark threshold logic in hw/ip/uart/rtl/uart_core.sv mishandles two cases:

A) Reserved encoding (RXILVL=7): The RTL sets rx_thresh_val to all-ones for out-of-range encodings, producing an effectively unreachable threshold and disabling the RX watermark event.

B) Shift sizing bug: rx_thresh_val = 1'b1 << uart_fifo_rxilvl uses a 1-bit left operand. Shifting a 1-bit value by any positive amount collapses to 0; the watermark condition (rx_fifo_depth >= rx_thresh_val) then evaluates true for all depths when rx_thresh_val == 0. Encodings 1..5 therefore cause a permanent watermark assertion.

Security impact:
An attacker with ability to configure RXILVL via software can induce denial-of-service or data loss:
- Using the reserved value (7) disables the RX watermark interrupt, allowing RX overflow and inbound data loss.
- Using RXILVL in 1..5 can generate a perpetual watermark interrupt, flooding the interrupt subsystem and potentially livelocking the CPU.

Exploit type: Proof-of-Concept (demonstrates both failure modes)

What this package contains:
- uart_vuln_test.c : C test that demonstrates both problematic behaviors using low-level base library MMIOs.
- BUILD : Bazel rule to compile the test with appropriate dependencies.

Key base APIs used:
- mmio_region_from_addr(), mmio_region_read32(), mmio_region_write32()  (sw/device/lib/base/mmio)
- bitfield_field32_write(), bitfield_bit32_write(), bitfield_bit32_read() (sw/device/lib/base/bitfield)
- LOG_INFO (sw/device/lib/runtime/log)
- OTTF test framework (sw/device/lib/testing/test_framework:ottf_main)

Test strategy:
1) Reset UART and clear interrupts/states.

2) Collapsed-shift case (RXILVL=2):
   - Write raw RXILVL encoding value 2 directly into UART_FIFO_CTRL.RXILVL.
   - Clear INTR_STATE and immediately read it back.
   - Because rx_thresh_val computes to 0, the watermark condition should be true (rx_fifo_depth >= 0), and the RX watermark event should be asserted even when FIFO is empty.

3) Reserved encoding case (RXILVL=7):
   - Reset FIFOs and set RXILVL field to raw value 7.
   - Enable loopback and transmit up to 32 bytes to populate RX FIFO.
   - Read INTR_STATE: with the unreachable threshold (all-ones), the watermark must NOT assert even when FIFO is filled.

Expected results:
- For RXILVL=2 (collapsed shift) INTR_STATE should be non-zero indicating watermark asserted.
- For RXILVL=7 (reserved) INTR_STATE should remain zero even after populating RX FIFO.

Building and running:
- Build with Bazel via the provided opentitan_test rule.
- Run under simulation (sim_verilator/sim_dv) or on hardware with appropriate mapping.

Notes and caveats:
- This POC uses the generated register definitions from //hw/top:uart_c_regs (uart_regs.h). The exact field/bit names are taken from the autogen header.
- The test uses raw writes of the RXILVL field; in production, high-level DIF APIs may prevent writing reserved encodings, but hardware-side logic still misbehaves if software can write these values.

Remediation:
- Fix the RTL to compute rx_thresh_val as a correctly sized power-of-two value, e.g. using an appropriately sized literal (e.g., `{{RxFifoDepthW{1'b1}} & (1 << uart_fifo_rxilvl)}`) or use 1 <<: uart_fifo_rxilvl with sizing, and treat reserved encodings explicitly (saturate to RxFifoDepth-2 or clamp to max supported value).

References:
- RTL snippet: hw/ip/uart/rtl/uart_core.sv lines 336-355 (rx_thresh_val calculation)

