#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

OTTF_DEFINE_TEST_CONFIG();

// Assumed register offsets (taken from aes_reg_pkg in RTL)
#define AES_IV_0_OFFSET         0x44
#define AES_IV_1_OFFSET         0x48
#define AES_IV_2_OFFSET         0x4c
#define AES_IV_3_OFFSET         0x50

#define AES_DATA_IN_0_OFFSET    0x54
#define AES_DATA_IN_1_OFFSET    0x58
#define AES_DATA_IN_2_OFFSET    0x5c
#define AES_DATA_IN_3_OFFSET    0x60

#define AES_TRIGGER_OFFSET      0x80

// Assumed trigger bit positions (from RTL trigger field order):
// prng_reseed (bit 0), data_out_clear (bit 1), key_iv_data_in_clear (bit 2), start (bit 3)
#define AES_TRIGGER_KEY_IV_DATA_IN_CLEAR (1u << 2)

bool test_main(void) {
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Craft a recognisable "secret" written to DATA_IN registers.
  const uint32_t secret[4] = { 0xDEADBEEF, 0xCAFEBABE, 0x01234567, 0x89ABCDEF };

  LOG_INFO("AES prev-data wipe POC start");

  // 1) Write secret into DATA_IN registers
  for (int i = 0; i < 4; ++i) {
    mmio_region_write32(aes, AES_DATA_IN_0_OFFSET + 4 * i, secret[i]);
  }

  // Read back to ensure write succeeded
  uint32_t readback[4];
  for (int i = 0; i < 4; ++i) {
    readback[i] = mmio_region_read32(aes, AES_DATA_IN_0_OFFSET + 4 * i);
    LOG_INFO("Wrote DATA_IN[%d] = 0x%08x", i, readback[i]);
    CHECK(readback[i] == secret[i], "Write/readback mismatch for DATA_IN[%d]", i);
  }

  // 2) Trigger Key/IV/DataIn clear operation that *should* zeroize previous data.
  LOG_INFO("Triggering KEY_IV_DATA_IN_CLEAR (assumed bit 2)");
  mmio_region_write32(aes, AES_TRIGGER_OFFSET, AES_TRIGGER_KEY_IV_DATA_IN_CLEAR);

  // Small delay to allow clearing to take effect (registers are synchronous in hardware).
  for (volatile int i = 0; i < 1000; ++i) {
    // busy wait
  }

  // 3) Read back DATA_IN registers. Expected behavior: cleared to pseudo-random or zeros.
  uint32_t after[4];
  for (int i = 0; i < 4; ++i) {
    after[i] = mmio_region_read32(aes, AES_DATA_IN_0_OFFSET + 4 * i);
    LOG_INFO("After clear DATA_IN[%d] = 0x%08x", i, after[i]);
  }

  // 4) Check whether the DATA_IN registers still contain the secret.
  // If they do, this indicates the clear operation didn't wipe software-visible data as expected
  // (one manifestation of the reported bug: previous data register zeroization not performed).
  bool vuln_detected = true;
  for (int i = 0; i < 4; ++i) {
    if (after[i] != secret[i]) {
      vuln_detected = false; // at least one word was changed, so clear may have happened
    }
  }

  if (vuln_detected) {
    LOG_INFO("VULNERABLE: DATA_IN registers retained secret after KEY_IV_DATA_IN_CLEAR trigger");
  } else {
    LOG_INFO("Not vulnerable (DATA_IN changed after clear)");
  }

  // We want the test to succeed only when the vulnerability is observed so that the POC proves it.
  CHECK(vuln_detected, "AES previous-data clear vulnerability not observed on this hardware/simulation");

  return true;
}
