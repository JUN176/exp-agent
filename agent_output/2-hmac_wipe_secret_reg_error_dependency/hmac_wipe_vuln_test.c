#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "hmac_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// Test data and key reused from existing HMAC tests to allow digest comparisons.
static const char kData[142] =
    "Every one suspects himself of at least one of "
    "the cardinal virtues, and this is mine: I am "
    "one of the few honest people that I have ever "
    "known";

static uint32_t kHmacKey[8] = {
    0xec4e6c89, 0x082efa98, 0x299f31d0, 0xa4093822,
    0x03707344, 0x13198a2e, 0x85a308d3, 0x243f6a88,
};

// Small spin-wait helper.
static void spin_sleep(uint32_t loops) {
  for (volatile uint32_t i = 0; i < loops; ++i) {
    asm volatile("nop");
  }
}

// Read the number of FIFO entries using the STATUS register field.
static uint32_t get_fifo_entry_count(mmio_region_t base) {
  uint32_t status = mmio_region_read32(base, HMAC_STATUS_REG_OFFSET);
  return bitfield_field32_read(status, HMAC_STATUS_FIFO_DEPTH_FIELD);
}

static uint32_t get_fifo_available_space(mmio_region_t base) {
  // HMAC_MSG_FIFO_SIZE_WORDS is defined in hmac_regs.h
  return HMAC_MSG_FIFO_SIZE_WORDS - get_fifo_entry_count(base);
}

// Push message into HMAC MSG FIFO. This mirrors the behavior of the DIF but
// uses only base APIs so we can precisely control the sequence.
static void push_message(mmio_region_t base, const char *data, size_t len) {
  const uint8_t *data_sent = (const uint8_t *)data;
  size_t bytes_remaining = len;

  while (bytes_remaining > 0) {
    // Wait for at least one word of space in the FIFO.
    while (get_fifo_available_space(base) == 0) {
      // allow HW to make progress
      spin_sleep(10);
    }

    bool word_aligned = ((uintptr_t)data_sent % sizeof(uint32_t)) == 0;
    size_t bytes_written = 0;

    if (bytes_remaining < sizeof(uint32_t) || !word_aligned) {
      // write single byte
      mmio_region_write8(base, HMAC_MSG_FIFO_REG_OFFSET, *data_sent);
      bytes_written = 1;
    } else {
      uint32_t word = read_32((char *)data_sent);
      mmio_region_write32(base, HMAC_MSG_FIFO_REG_OFFSET, word);
      bytes_written = sizeof(uint32_t);
    }

    bytes_remaining -= bytes_written;
    data_sent += bytes_written;
  }
}

// Read digest from digest registers into dest (8 words).
static void read_digest(mmio_region_t base, uint32_t dest[8]) {
  for (size_t i = 0; i < 8; ++i) {
    dest[i] = mmio_region_read32(base, HMAC_DIGEST_7_REG_OFFSET -
                                         (ptrdiff_t)(i * sizeof(uint32_t)));
  }
}

// Configure HMAC to HMAC mode with SHA256 and 256-bit key fields set.
static void configure_hmac_for_hmac(mmio_region_t base) {
  uint32_t cfg = mmio_region_read32(base, HMAC_CFG_REG_OFFSET);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_SHA_EN_BIT, true);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_HMAC_EN_BIT, true);
  cfg = bitfield_field32_write(cfg, HMAC_CFG_DIGEST_SIZE_FIELD,
                               HMAC_CFG_DIGEST_SIZE_VALUE_SHA2_256);
  cfg = bitfield_field32_write(cfg, HMAC_CFG_KEY_LENGTH_FIELD,
                               HMAC_CFG_KEY_LENGTH_VALUE_KEY_256);
  mmio_region_write32(base, HMAC_CFG_REG_OFFSET, cfg);
}

// Start the hash operation (sets HASH_START). Use nonatomic set bit to
// mirror DIF behavior.
static void start_hash(mmio_region_t base) {
  mmio_region_nonatomic_set_bit32(base, HMAC_CMD_REG_OFFSET,
                                  HMAC_CMD_HASH_START_BIT);
}

// Trigger HMAC process (PROCESS bit)
static void process_hash(mmio_region_t base) {
  mmio_region_nonatomic_set_bit32(base, HMAC_CMD_REG_OFFSET,
                                  HMAC_CMD_HASH_PROCESS_BIT);
}

// Wait for HMAC done interrupt state (HMAC_DONE) with a simple timeout.
static bool wait_hmac_done(mmio_region_t base, uint32_t timeout_loops) {
  while (timeout_loops--) {
    bool done = mmio_region_get_bit32(base, HMAC_INTR_STATE_REG_OFFSET,
                                      HMAC_INTR_STATE_HMAC_DONE_BIT);
    if (done) {
      // clear the HMAC_DONE state like DIF does
      mmio_region_nonatomic_set_bit32(base, HMAC_INTR_STATE_REG_OFFSET,
                                      HMAC_INTR_STATE_HMAC_DONE_BIT);
      return true;
    }
    spin_sleep(50);
  }
  return false;
}

// Load key into HMAC key registers. The HW expects least significant word at
// HMAC_KEY_7_REG_OFFSET and so on (as in DIF implementation).
static void load_key(mmio_region_t base, const uint32_t key[8]) {
  for (size_t i = 0; i < 8; ++i) {
    const ptrdiff_t word_offset = (ptrdiff_t)(i * sizeof(uint32_t));
    mmio_region_write32(base, HMAC_KEY_7_REG_OFFSET - word_offset, key[i]);
  }
}

bool test_main(void) {
  LOG_INFO("HMAC wipe-secret reg_error dependency POC starting...");

  mmio_region_t base = mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR);

  // 1) Compute a baseline HMAC with a known key.
  LOG_INFO("Loading key and computing baseline HMAC...");
  load_key(base, kHmacKey);
  configure_hmac_for_hmac(base);
  start_hash(base);
  push_message(base, kData, sizeof(kData));
  process_hash(base);

  CHECK(wait_hmac_done(base, 10000), "HMAC did not finish (baseline)");

  uint32_t baseline_digest[8];
  read_digest(base, baseline_digest);
  LOG_INFO("Baseline digest[0]=0x%08x", baseline_digest[0]);

  // 2) Attempt to wipe secret without any error condition and verify effect.
  const uint32_t kEntropy = 0xDEADBEEF;
  LOG_INFO("Attempting wipe (normal condition) with entropy=0x%08x", kEntropy);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_WIPE_SECRET_REG_OFFSET,
                   kEntropy);

  // Read digest immediately after wipe (DIF does the same).
  uint32_t wipe_digest_normal[8];
  read_digest(base, wipe_digest_normal);

  // If the wipe worked as intended, the digest registers would be overwritten
  // with the entropy value. We check whether they equal the entropy value.
  bool wiped_normal = true;
  for (size_t i = 0; i < 8; ++i) {
    if (wipe_digest_normal[i] != kEntropy) {
      wiped_normal = false;
      break;
    }
  }

  if (wiped_normal) {
    LOG_INFO("Wipe succeeded in normal condition (unexpected)");
  } else {
    LOG_INFO("Wipe did NOT succeed in normal condition (vulnerable behavior)");
  }

  // 3) Start a new HMAC operation WITHOUT reloading the key. On a correct
  // implementation the previous key should have been wiped; therefore the
  // resulting digest should differ from baseline. If wipe failed, digest will
  // match baseline.
  LOG_INFO("Starting HMAC without reloading key (expect different digest if wipe worked)...");
  // Note: do NOT reload key here. Reconfigure and run with same message.
  configure_hmac_for_hmac(base);
  start_hash(base);
  push_message(base, kData, sizeof(kData));
  process_hash(base);
  CHECK(wait_hmac_done(base, 10000), "HMAC did not finish (post-wipe)");

  uint32_t post_wipe_digest[8];
  read_digest(base, post_wipe_digest);

  bool digest_same = true;
  for (size_t i = 0; i < 8; ++i) {
    if (post_wipe_digest[i] != baseline_digest[i]) {
      digest_same = false;
      break;
    }
  }

  if (digest_same) {
    LOG_INFO("Post-wipe digest == baseline digest: wipe did not remove key (vulnerable)");
  } else {
    LOG_INFO("Post-wipe digest != baseline digest: wipe removed key (expected)");
  }

  // 4) Now induce an error condition that sets reg_error. We follow a
  // behavior known to set an error: disable SHA_EN and then write HASH_START.
  // This mirrors tests in hmac_error_conditions_test.c.
  LOG_INFO("Inducing an error condition (disable SHA then HASH_START)...");
  uint32_t cfg = mmio_region_read32(base, HMAC_CFG_REG_OFFSET);
  cfg = bitfield_bit32_write(cfg, HMAC_CFG_SHA_EN_BIT, false);
  mmio_region_write32(base, HMAC_CFG_REG_OFFSET, cfg);

  // Write HASH_START to trigger the error (SHA disabled + start -> error).
  mmio_region_nonatomic_set_bit32(base, HMAC_CMD_REG_OFFSET,
                                  HMAC_CMD_HASH_START_BIT);

  // Small delay to let HW set error state.
  spin_sleep(1000);

  uint32_t err_code = mmio_region_read32(base, HMAC_ERR_CODE_REG_OFFSET);
  LOG_INFO("HMAC error register after induced error: 0x%08x", err_code);

  // 5) Attempt wipe while in error condition.
  const uint32_t kEntropy2 = 0xFEEDC0DE;
  LOG_INFO("Attempting wipe while error present with entropy=0x%08x", kEntropy2);
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_WIPE_SECRET_REG_OFFSET,
                   kEntropy2);

  uint32_t wipe_digest_error[8];
  read_digest(base, wipe_digest_error);

  bool wiped_on_error = true;
  for (size_t i = 0; i < 8; ++i) {
    if (wipe_digest_error[i] != kEntropy2) {
      wiped_on_error = false;
      break;
    }
  }

  if (wiped_on_error) {
    LOG_INFO("Wipe succeeded while error present (expected behavior if HW gate requires error)");
  } else {
    LOG_INFO("Wipe did NOT succeed even while error present (unexpected)");
  }

  // Final assertions to prove vulnerability: wipe failing in normal conditions
  // but succeeding when error is present demonstrates the dependency.
  CHECK(!wiped_normal && wiped_on_error,
        "Vulnerability not reproduced: expected wipe to require error to succeed");

  // Additionally check that post-wipe digest (normal path) equals baseline,
  // which would mean the secret remained and can be reused.
  CHECK(digest_same, "Expected post-wipe digest to match baseline when wipe fails");

  LOG_INFO("POC completed: vulnerability reproduced");
  return true;
}
