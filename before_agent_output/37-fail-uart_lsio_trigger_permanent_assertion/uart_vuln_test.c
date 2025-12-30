#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"

// Include generated register offsets for UART.
#include "uart_regs.h"

// Base address for UART0 provided by top-level autogen target.
// This macro is provided by the BUILD dependency "//hw/top_earlgrey/sw/autogen:top_earlgrey".
#ifndef TOP_EARLGREY_UART0_BASE_ADDR
#warning "TOP_EARLGREY_UART0_BASE_ADDR not defined; set to 0x41100000 as fallback"
#define TOP_EARLGREY_UART0_BASE_ADDR 0x41100000u
#endif

// NOTE: The RTL exposes an output port `lsio_trigger_o` which is a hardware
// handshake to LSIO DMA. In some top-level integrations this signal is
// reflected to a dedicated status register or to a bit in an existing status
// register. The vulnerable RTL drives `lsio_trigger_o` high after reset
// unconditionally (see hw/ip/uart/rtl/uart_core.sv L356-L364), which collapses
// the DMA request into a permanent "not-in-reset" indicator.
//
// The POC below demonstrates how firmware can detect the persistent assertion
// by reading a memory-mapped status register assumed to reflect the LSIO DMA
// handshake. The exact register/bit is platform dependent. For the purposes of
// a reproducible, base-library-only POC we attempt to read a likely candidate
// (an offset within the UART register region) and log the result.

// Hypothetical offset for a HW->REG status that contains the LSIO trigger
// bit. On some integrations this may be provided explicitly; otherwise a
// platform integrator must map `lsio_trigger_o` into the system register
// space. We select an offset that is safe within the UART register region
// but adjustable by the reviewer.
#define UART_HW2REG_LSIO_TRIGGER_REG_OFFSET (UART_STATUS_REG_OFFSET + 0x10)
// Hypothetical bit index within that status register that reflects lsio_trigger_o
#define UART_HW2REG_LSIO_TRIGGER_BIT 0

OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = false);

bool test_main(void) {
  uintptr_t uart0_base = (uintptr_t)TOP_EARLGREY_UART0_BASE_ADDR;

  LOG_INFO("UART vulnerability POC: checking LSIO trigger behavior");

  // Read a handful of UART registers to record baseline state.
  uint32_t status = mmio_region_read32(mmio_region_from_addr(uart0_base),
                                       UART_STATUS_REG_OFFSET);
  uint32_t ctrl = mmio_region_read32(mmio_region_from_addr(uart0_base),
                                     UART_CTRL_REG_OFFSET);
  uint32_t fifo_ctrl = mmio_region_read32(mmio_region_from_addr(uart0_base),
                                          UART_FIFO_CTRL_REG_OFFSET);

  LOG_INFO("UART STATUS = 0x%08x", status);
  LOG_INFO("UART CTRL   = 0x%08x", ctrl);
  LOG_INFO("UART FIFO_CTRL = 0x%08x", fifo_ctrl);

  // Read the assumed LSIO trigger status register multiple times to detect
  // persistence. On correct hardware, the LSIO request should follow
  // watermark/timeout/dma handshake and not be permanently asserted.
  mmio_region_t uart_base = mmio_region_from_addr(uart0_base);
  bool seen_deassert = false;
  bool seen_assert = false;
  for (int i = 0; i < 10; ++i) {
    uint32_t lsio_reg = mmio_region_read32(uart_base, UART_HW2REG_LSIO_TRIGGER_REG_OFFSET);
    bool lsio_bit = !!(lsio_reg & (1u << UART_HW2REG_LSIO_TRIGGER_BIT));
    LOG_INFO("Read LSIO_STATUS[0x%03x] = 0x%08x -> bit=%d",
             UART_HW2REG_LSIO_TRIGGER_REG_OFFSET, lsio_reg, lsio_bit);
    if (lsio_bit) seen_assert = true;
    else seen_deassert = true;

    // Small software delay between reads to allow for downstream state changes.
    for (volatile int d = 0; d < 1000; ++d) {}
  }

  // Vulnerability: if the RTL drives lsio_trigger_o unconditionally high after
  // reset then firmware will only see asserted state and never see a
  // deassertion that corresponds to proper watermark/handshake conditions.
  CHECK(seen_assert, "LSIO trigger was never observed asserted (unexpected)");
  // If we never observe a deassertion, report the problematic behavior.
  if (!seen_deassert) {
    LOG_INFO("Detected persistent LSIO trigger assertion: potential DoS / DMA starve");
    // We intentionally fail the test to flag the security issue to DV/CI.
    CHECK(false, "LSIO trigger remained asserted across observations -> VULN-37");
  }

  // If both states were observed, behavior appears correct (non-vulnerable on
  // this platform mapping).
  LOG_INFO("LSIO trigger observed to toggle; no persistent assertion detected");

  return true;
}
