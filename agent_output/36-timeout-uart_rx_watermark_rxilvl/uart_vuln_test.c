#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
// 引入顶层地址定义（关键：获取正确的 UART 基地址）
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
// Generated register header for UART
#include "uart_regs.h"

// 修正1：定义 UART 中断状态全掩码（覆盖 bit 0~8 所有中断位）
#define UART_INTR_STATE_MASK 0x1FFu

OTTF_DEFINE_TEST_CONFIG(.enable_concurrency = false,
                        .console.test_may_clobber = true, );

bool test_main(void)
{
  mmio_region_t uart = mmio_region_from_addr(TOP_EARLGREY_UART0_BASE_ADDR);

  LOG_INFO("UART RX watermark (RXILVL) vulnerability POC start");

  // Perform a soft reset of the UART peripheral similar to dif_uart_reset.
  mmio_region_write32(uart, UART_CTRL_REG_OFFSET, 0u);
  uint32_t tmp = 0;
  tmp = bitfield_bit32_write(tmp, UART_FIFO_CTRL_RXRST_BIT, true);
  tmp = bitfield_bit32_write(tmp, UART_FIFO_CTRL_TXRST_BIT, true);
  mmio_region_write32(uart, UART_FIFO_CTRL_REG_OFFSET, tmp);

  mmio_region_write32(uart, UART_OVRD_REG_OFFSET, 0u);
  mmio_region_write32(uart, UART_TIMEOUT_CTRL_REG_OFFSET, 0u);

  // Disable all interrupts and clear sticky states.
  mmio_region_write32(uart, UART_INTR_ENABLE_REG_OFFSET, 0u);
  // 使用修正后的掩码清空所有中断状态
  mmio_region_write32(uart, UART_INTR_STATE_REG_OFFSET, UART_INTR_STATE_MASK);

  // --- Case 1: Collapsed shift (RXILVL in [1..5]) ---
  // The buggy RTL computes rx_thresh_val = 1'b1 << uart_fifo_rxilvl using a 1-bit
  // literal. For shifts > 0 this collapses to 0, making the watermark condition
  // (rx_fifo_depth >= rx_thresh_val) always true. We set RXILVL=2 and expect
  // the RX watermark event to be asserted even when the FIFO is empty.
  LOG_INFO("Testing collapsed-shift case (RXILVL=2)");

  uint32_t fifo_ctrl = mmio_region_read32(uart, UART_FIFO_CTRL_REG_OFFSET);
  // 使用寄存器定义中的合法值：RXILVL=2 对应 RXLVL4
  fifo_ctrl = bitfield_field32_write(fifo_ctrl, UART_FIFO_CTRL_RXILVL_FIELD, 2u);
  mmio_region_write32(uart, UART_FIFO_CTRL_REG_OFFSET, fifo_ctrl);

  // Clear sticky intr state and read back immediately.
  mmio_region_write32(uart, UART_INTR_STATE_REG_OFFSET, UART_INTR_STATE_MASK);
  uint32_t intr_state = mmio_region_read32(uart, UART_INTR_STATE_REG_OFFSET);

  LOG_INFO("INTR_STATE after setting RXILVL=2: 0x%08x", intr_state);

  // We expect intr_state to be non-zero because the watermark becomes true with
  // threshold == 0 (rx_fifo_depth >= 0 is always true). If it is zero, the
  // implementation did not assert the event as expected (test fails).
  CHECK(intr_state != 0u, "Collapsed-shift RXILVL did NOT assert watermark as expected");

  // --- Case 2: Reserved encoding (RXILVL=7) ---
  // The buggy RTL handles out-of-range encodings by setting rx_thresh_val to
  // all-ones ({RxFifoDepthW{1'b1}}). For the implemented FIFO depth width this
  // yields an unreachable threshold that effectively disables the watermark.
  // We set RXILVL=7 (reserved) and then fill the FIFO via loopback; the RX
  // watermark should NOT assert even when the FIFO is filled to capacity.
  LOG_INFO("Testing reserved-encoding case (RXILVL=7)");

  // Re-reset FIFOs/states to ensure a clean start.
  mmio_region_write32(uart, UART_CTRL_REG_OFFSET, 0u);
  tmp = 0;
  tmp = bitfield_bit32_write(tmp, UART_FIFO_CTRL_RXRST_BIT, true);
  tmp = bitfield_bit32_write(tmp, UART_FIFO_CTRL_TXRST_BIT, true);
  mmio_region_write32(uart, UART_FIFO_CTRL_REG_OFFSET, tmp);
  mmio_region_write32(uart, UART_INTR_STATE_REG_OFFSET, UART_INTR_STATE_MASK);

  // Set RXILVL to reserved value 7 (raw encoding write).
  fifo_ctrl = mmio_region_read32(uart, UART_FIFO_CTRL_REG_OFFSET);
  fifo_ctrl = bitfield_field32_write(fifo_ctrl, UART_FIFO_CTRL_RXILVL_FIELD, 7u);
  mmio_region_write32(uart, UART_FIFO_CTRL_REG_OFFSET, fifo_ctrl);

  // Enable system loopback so TX bytes are received back into RX FIFO.
  uint32_t ctrl = mmio_region_read32(uart, UART_CTRL_REG_OFFSET);
  ctrl = bitfield_bit32_write(ctrl, UART_CTRL_LLPBK_BIT, true);
  // Also enable TX so we can send bytes.
  ctrl = bitfield_bit32_write(ctrl, UART_CTRL_TX_BIT, true);
  // Enable RX so received bytes go into RX FIFO.
  ctrl = bitfield_bit32_write(ctrl, UART_CTRL_RX_BIT, true);
  mmio_region_write32(uart, UART_CTRL_REG_OFFSET, ctrl);

  // Clear intr state before sending bytes.
  mmio_region_write32(uart, UART_INTR_STATE_REG_OFFSET, UART_INTR_STATE_MASK);

  // Try to send up to 32 bytes (TX FIFO size, RX FIFO is 64). Stop if TX becomes full.
  // 修正：根据寄存器定义，TX FIFO 深度是 32，RX FIFO 深度是 64
  const uint32_t kTxFifoSize = 32u;
  uint32_t bytes_sent = 0u;
  while (bytes_sent < kTxFifoSize)
  {
    uint32_t status = mmio_region_read32(uart, UART_STATUS_REG_OFFSET);
    bool tx_full = bitfield_bit32_read(status, UART_STATUS_TXFULL_BIT);
    if (tx_full)
    {
      break;
    }

    uint32_t w = bitfield_field32_write(0u, UART_WDATA_WDATA_FIELD, (uint8_t)bytes_sent);
    mmio_region_write32(uart, UART_WDATA_REG_OFFSET, w);
    ++bytes_sent;
  }

  // Give hardware a moment (combinational logic should already reflect) and
  // sample INTR_STATE.
  intr_state = mmio_region_read32(uart, UART_INTR_STATE_REG_OFFSET);
  LOG_INFO("INTR_STATE after sending %u bytes with RXILVL=7: 0x%08x", bytes_sent, intr_state);

  // With RXILVL=7 the threshold is effectively unreachable; therefore the
  // watermark must NOT be asserted even when the FIFO contains data. We expect
  // intr_state to be zero here.
  CHECK(intr_state == 0u, "Reserved RXILVL=7 unexpectedly asserted watermark");

  LOG_INFO("UART RX watermark (RXILVL) vulnerability POC completed successfully");
  return true;
}