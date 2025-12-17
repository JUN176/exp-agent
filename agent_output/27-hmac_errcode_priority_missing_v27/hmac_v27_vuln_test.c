#include "sw/device/lib/arch/device.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "hmac_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// This test demonstrates vulnerability ID 27: when an invalid configuration at
// start causes err_valid to be asserted, the err_code priority encoder omits
// the invalid_config_atstart condition. The hardware therefore may assert the
// hmac_err interrupt while ERR_CODE == NoError (0).
//
// The test intentionally passes when the faulty behavior is observed (err
// interrupt pending AND ERR_CODE == 0). On a correct implementation the test
// will fail.

bool test_main(void) {
  LOG_INFO("HMAC ERR_CODE priority test (vuln id 27) starting...");

  mmio_region_t base = mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR);

  // Clear any pending interrupts first.
  uint32_t clear_reg = 0;
  clear_reg = bitfield_bit32_write(clear_reg, HMAC_INTR_STATE_HMAC_DONE_BIT, true);
  clear_reg = bitfield_bit32_write(clear_reg, HMAC_INTR_STATE_FIFO_EMPTY_BIT, true);
  clear_reg = bitfield_bit32_write(clear_reg, HMAC_INTR_STATE_HMAC_ERR_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_INTR_STATE_REG_OFFSET,
                   clear_reg);

  // Enable only the HMAC_ERR interrupt to observe the condition.
  uint32_t intr_enable = 0;
  intr_enable = bitfield_bit32_write(intr_enable, HMAC_INTR_ENABLE_HMAC_ERR_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_INTR_ENABLE_REG_OFFSET,
                   intr_enable);

  // Configure an intentionally invalid configuration: HMAC enabled but key
  // length set to KEY_NONE. This mirrors the sequence used in the existing
  // regression tests for invalid config, and should trigger the invalid
  // configuration error at start in the HMAC IP.
  uint32_t cfg = 0;
  cfg = bitfield_field32_write(cfg, HMAC_CFG_KEY_LENGTH_FIELD,
                               HMAC_CFG_KEY_LENGTH_VALUE_KEY_NONE);
  cfg = bitfield_field32_write(cfg, HMAC_CFG_DIGEST_SIZE_FIELD,
                               HMAC_CFG_DIGEST_SIZE_VALUE_SHA2_256);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_SHA_EN_BIT, true);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_HMAC_EN_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_CFG_REG_OFFSET, cfg);

  // Issue HASH_START command which should cause the IP to sample the config
  // and detect the invalid configuration at start.
  uint32_t cmd = mmio_region_read32(base, HMAC_CMD_REG_OFFSET);
  cmd = bitfield_bit32_write(cmd, HMAC_CMD_HASH_START_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_CMD_REG_OFFSET, cmd);

  // Short delay to allow the HW to respond and set interrupt/err fields. This
  // is intentionally small; the vulnerability manifests at start and should be
  // visible immediately.
  for (volatile int i = 0; i < 1000; ++i) {
    __asm volatile("nop");
  }

  // Read interrupt state and error code.
  uint32_t intr_state = mmio_region_read32(base, HMAC_INTR_STATE_REG_OFFSET);
  uint32_t err_code = mmio_region_read32(base, HMAC_ERR_CODE_REG_OFFSET);

  LOG_INFO("HMAC_INTR_STATE = 0x%08x", intr_state);
  LOG_INFO("HMAC_ERR_CODE  = 0x%08x", err_code);

  // If the hardware is vulnerable, we will observe the hmac_err interrupt
  // pending while ERR_CODE == 0 (NoError). The test intentionally succeeds
  // in that case to demonstrate the existence of the bug. On a correct
  // implementation the intr_state will either not have HMAC_ERR set yet, or
  // ERR_CODE will reflect the specific error (non-zero), causing the test to
  // fail.
  bool err_pending = bitfield_bit32_read(intr_state, HMAC_INTR_STATE_HMAC_ERR_BIT);

  if (err_pending && err_code == 0) {
    LOG_INFO("VULNERABLE: HMAC_ERR pending while ERR_CODE == NoError");
    return true; // Report success for POC (vulnerability observed).
  }

  CHECK(false, "Vulnerability not observed: either no hmac_err pending or ERR_CODE != NoError");
  return false;
}
