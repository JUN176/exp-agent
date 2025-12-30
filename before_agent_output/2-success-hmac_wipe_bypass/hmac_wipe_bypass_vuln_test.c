#include <stdint.h>
#include <stddef.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "hmac_regs.h"

// Test demonstrating that HMAC secure wipe is gated on reg_error and thus
// ineffective under normal conditions. The test succeeds if the wipe is
// bypassable (i.e., the second HMAC with no key reuses the previous key).

OTTF_DEFINE_TEST_CONFIG();

static const char kData[] =
    "Every one suspects himself of at least one of "
    "the cardinal virtues, and this is mine: I am "
    "one of the few honest people that I have ever "
    "known";

static const uint32_t kHmacKey[8] = {
    0xec4e6c89, 0x082efa98, 0x299f31d0, 0xa4093822,
    0x03707344, 0x13198a2e, 0x85a308d3, 0x243f6a88,
};

// Helpers for register access
static inline mmio_region_t hmac_base(void) {
  return mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR);
}

static void write_key(const uint32_t key_words[8]) {
  mmio_region_t base = hmac_base();
  // The least significant word is at HMAC_KEY_7_REG_OFFSET
  for (size_t i = 0; i < 8; ++i) {
    ptrdiff_t off = HMAC_KEY_7_REG_OFFSET - (ptrdiff_t)(i * sizeof(uint32_t));
    mmio_region_write32(base, off, key_words[i]);
  }
}

static void cfg_enable_hmac(void) {
  mmio_region_t base = hmac_base();
  uint32_t reg = mmio_region_read32(base, HMAC_CFG_REG_OFFSET);
  // Enable SHA and HMAC mode.
  reg = bitfield_bit32_write(reg, HMAC_CFG_SHA_EN_BIT, true);
  reg = bitfield_bit32_write(reg, HMAC_CFG_HMAC_EN_BIT, true);
  // Set digest size to SHA2-256 and key length to 256.
  reg = bitfield_field32_write(reg, HMAC_CFG_DIGEST_SIZE_FIELD,
                               HMAC_CFG_DIGEST_SIZE_VALUE_SHA2_256);
  reg = bitfield_field32_write(reg, HMAC_CFG_KEY_LENGTH_FIELD,
                               HMAC_CFG_KEY_LENGTH_VALUE_KEY_256);
  mmio_region_write32(base, HMAC_CFG_REG_OFFSET, reg);
  // Kick the hardware to start transaction (non-atomic set bit as DIF does).
  mmio_region_nonatomic_set_bit32(base, HMAC_CMD_REG_OFFSET,
                                  HMAC_CMD_HASH_START_BIT);
}

static void push_message(const void *data, size_t len) {
  mmio_region_t base = hmac_base();
  const uint8_t *d = (const uint8_t *)data;
  size_t bytes_remaining = len;

  while (bytes_remaining > 0) {
    // If FIFO is full, wait a bit (simple spin). Use STATUS FIFO_DEPTH to
    // estimate availability.
    uint32_t status = mmio_region_read32(base, HMAC_STATUS_REG_OFFSET);
    uint32_t depth = bitfield_field32_read(status, HMAC_STATUS_FIFO_DEPTH_FIELD);
    // HMAC_MSG_FIFO_SIZE_WORDS is in HWIP; use a conservative check.
    if (depth >= HMAC_MSG_FIFO_SIZE_WORDS) {
      // busy wait
      continue;
    }

    bool word_aligned = ((uintptr_t)d % sizeof(uint32_t)) == 0 && bytes_remaining >= sizeof(uint32_t);
    if (!word_aligned) {
      mmio_region_write8(base, HMAC_MSG_FIFO_REG_OFFSET, *d);
      d += 1;
      bytes_remaining -= 1;
    } else {
      uint32_t w = read_32(d);
      mmio_region_write32(base, HMAC_MSG_FIFO_REG_OFFSET, w);
      d += sizeof(uint32_t);
      bytes_remaining -= sizeof(uint32_t);
    }
  }
}

static void trigger_process(void) {
  mmio_region_nonatomic_set_bit32(hmac_base(), HMAC_CMD_REG_OFFSET,
                                  HMAC_CMD_HASH_PROCESS_BIT);
}

static void read_digest(uint32_t out[8]) {
  mmio_region_t base = hmac_base();
  for (size_t i = 0; i < 8; ++i) {
    ptrdiff_t off = HMAC_DIGEST_7_REG_OFFSET - (ptrdiff_t)(i * sizeof(uint32_t));
    out[i] = mmio_region_read32(base, off);
  }
}

static void wait_for_done(void) {
  mmio_region_t base = hmac_base();
  // Wait for HMAC_DONE bit in INTR_STATE.
  for (;;) {
    bool done = mmio_region_get_bit32(base, HMAC_INTR_STATE_REG_OFFSET,
                                      HMAC_INTR_STATE_HMAC_DONE_BIT);
    if (done) {
      // Acknowledge the interrupt bit as DIF does.
      mmio_region_nonatomic_set_bit32(base, HMAC_INTR_STATE_REG_OFFSET,
                                      HMAC_INTR_STATE_HMAC_DONE_BIT);
      break;
    }
  }
}

bool test_main(void) {
  LOG_INFO("HMAC wipe bypass POC starting...");
  mmio_region_t base = hmac_base();

  // 1) Load key and start HMAC to compute baseline digest.
  LOG_INFO("Loading key and computing baseline HMAC...");
  write_key(kHmacKey);
  cfg_enable_hmac();
  push_message(kData, sizeof(kData));
  trigger_process();
  wait_for_done();

  uint32_t digest1[8];
  read_digest(digest1);

  LOG_INFO("Baseline digest: %08x %08x %08x %08x ...",
           digest1[0], digest1[1], digest1[2], digest1[3]);

  // 2) Attempt secure wipe by writing to the wipe register (entropy).
  // Under normal operation, the RTL only accepts this write when reg_error
  // is true (L2128 -> wipe_secret_we = addr_hit[8] && reg_we && reg_error),
  // therefore the write will be ignored and key will remain.
  LOG_INFO("Writing to wipe register (this should wipe the secret)...");
  mmio_region_write32(base, HMAC_WIPE_SECRET_REG_OFFSET, 0xFFFFFFFFu);

  // Read digest returned by the wipe (HW is supposed to output the wiped
  // digest). If wipe is ignored, this digest will be equal to previous.
  uint32_t wipe_digest[8];
  read_digest(wipe_digest);
  LOG_INFO("Wipe digest: %08x %08x %08x %08x ...",
           wipe_digest[0], wipe_digest[1], wipe_digest[2], wipe_digest[3]);

  // 3) Start a new HMAC transaction WITHOUT reloading the key (simulate
  // dif_hmac_mode_hmac_start(hmac, NULL,...)). If key was wiped the result
  // should differ. If key persisted, we'll get same digest as baseline.
  LOG_INFO("Starting new transaction WITHOUT loading key, computing HMAC...");
  // Do not write key registers.
  cfg_enable_hmac();
  push_message(kData, sizeof(kData));
  trigger_process();
  wait_for_done();

  uint32_t digest2[8];
  read_digest(digest2);
  LOG_INFO("Second digest: %08x %08x %08x %08x ...",
           digest2[0], digest2[1], digest2[2], digest2[3]);

  // Compare digests. If wipe was ineffective, digest2 == digest1.
  bool same = true;
  for (size_t i = 0; i < 8; ++i) {
    if (digest1[i] != digest2[i]) {
      same = false;
      break;
    }
  }

  if (same) {
    LOG_INFO("VULN: Secure wipe was bypassed - key persisted across wipe.");
  } else {
    LOG_INFO("Secure wipe seems effective (digests differ).");
  }

  // We want this test to PASS if the vulnerability is present, i.e., digests
  // are the same (wipe bypass). Assert that condition.
  CHECK(same, "Expected wipe to be bypassed (digests equal).\n");
  return true;
}
