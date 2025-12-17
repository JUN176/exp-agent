#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "hmac_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// Demonstrates that invalid_config_atstart triggers the hmac_err interrupt
// while ERR_CODE remains NoError (0). This is caused by the RTL including
// invalid_config_atstart in err_valid but not handling it in the priority
// case that assigns err_code.

bool test_main(void) {
  LOG_INFO("HMAC ERR_CODE mismatch POC: start");

  mmio_region_t hmac = mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR);

  // 1) Ensure interrupt state is clear
  uint32_t clear = 0;
  clear = bitfield_bit32_write(clear, HMAC_INTR_STATE_HMAC_ERR_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_INTR_STATE_REG_OFFSET,
                  clear);

  // 2) Enable HMAC_ERR interrupt so the hardware will set intr_state when
  //    err_valid is asserted.
  uint32_t intr_en = 0;
  intr_en = bitfield_bit32_write(intr_en, HMAC_INTR_ENABLE_HMAC_ERR_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_INTR_ENABLE_REG_OFFSET,
                  intr_en);

  LOG_INFO("Configured interrupts: HMAC_ERR enabled");

  // 3) Create an invalid configuration that satisfies `invalid_config` in
  //    the RTL. The RTL treats digest_size == SHA2_None as invalid.
  uint32_t cfg = 0;
  cfg = bitfield_field32_write(cfg, HMAC_CFG_DIGEST_SIZE_FIELD,
                               HMAC_CFG_DIGEST_SIZE_VALUE_SHA2_NONE);
  // Keep HMAC/ SHA enabled to make this a meaningful invalid config for the
  // controller. This mirrors patterns used in existing tests.
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_HMAC_EN_BIT, true);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_SHA_EN_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_CFG_REG_OFFSET, cfg);

  LOG_INFO("Wrote invalid HMAC config (digest = SHA2_None)");

  // 4) Trigger a hash start: this will set `invalid_config_atstart` because
  //    (reg_hash_start || reg_hash_continue) & invalid_config will be true.
  uint32_t cmd = mmio_region_read32(hmac, HMAC_CMD_REG_OFFSET);
  cmd = bitfield_bit32_write(cmd, HMAC_CMD_HASH_START_BIT, true);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_CMD_REG_OFFSET, cmd);

  LOG_INFO("Wrote HASH_START while config is invalid -> expect hmac_err");

  // 5) Read interrupt state and ERR_CODE.
  uint32_t intr_state = mmio_region_read32(hmac, HMAC_INTR_STATE_REG_OFFSET);
  uint32_t err_code = mmio_region_read32(hmac, HMAC_ERR_CODE_REG_OFFSET);

  LOG_INFO("INTR_STATE = 0x%08x, ERR_CODE = 0x%08x", intr_state, err_code);

  // HMAC_ERR bit should be set in intr_state (hardware should raise an interrupt)
  bool intr_set = bitfield_bit32_read(intr_state, HMAC_INTR_STATE_HMAC_ERR_BIT);

  // According to the bug, ERR_CODE will still be NoError (0) despite the
  // interrupt being asserted because invalid_config_atstart isn't handled in
  // the err_code priority case, leaving NoError as the default.
  CHECK(intr_set, "Expected HMAC_ERR interrupt to be set (intr_state)");
  CHECK(err_code == 0u,
        "Expected ERR_CODE to be NoError (0) when invalid_config_atstart was triggered due to RTL bug");

  LOG_INFO("POC successful: interrupt asserted but ERR_CODE == NoError");

  return true;
}
