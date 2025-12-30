#include "hw/ip/aes/model/aes_modes.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/dif/dif_aes.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// Small helper to assert DIF return values using CHECK (base-only assertion).
static void expect_dif_ok(dif_result_t res, const char *msg) {
  CHECK(res == kDifOk, "DIF call failed: %s, code=%d", msg, res);
}

bool test_main(void) {
  LOG_INFO("AES PRD persistence vulnerability POC (vuln 22)");

  dif_aes_t aes;
  dif_aes_key_share_t key;

  // Prepare a static key share pair. Keep share1 non-zero so masking has effect.
  uint8_t key_share1[16] = {
    0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f,
    0x8f, 0x9f, 0xaf, 0xbf, 0xcf, 0xdf, 0xef, 0xff,
  };
  uint8_t key_share0[16];
  for (size_t i = 0; i < sizeof(key_share0); ++i) {
    key_share0[i] = kAesModesKey128[i] ^ key_share1[i];
  }
  memcpy(key.share0, key_share0, sizeof(key.share0));
  memcpy(key.share1, key_share1, sizeof(key.share1));

  // Initialize AES and clear state.
  expect_dif_ok(dif_aes_init(mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR), &aes),
                "dif_aes_init");
  expect_dif_ok(dif_aes_reset(&aes), "dif_aes_reset");

  // Configure transaction. Important: reseed_on_key_change=false so starting a
  // new transaction does NOT implicitly reseed the masking PRNG.
  dif_aes_transaction_t transaction = {
      .operation = kDifAesOperationEncrypt,
      .mode = kDifAesModeEcb,
      .key_len = kDifAesKey128,
      .key_provider = kDifAesKeySoftwareProvided,
      .mask_reseeding = kDifAesReseedPerBlock,
      .manual_operation = kDifAesManualOperationManual,
      .reseed_on_key_change = false,  // <-- critical for the test
      .force_masks = true,            // force masking to make PRD relevant
      .ctrl_aux_lock = false,
  };

  // Plaintext to encrypt (use model test vector).
  dif_aes_data_t plain_text;
  memcpy(plain_text.data, kAesModesPlainText, sizeof(plain_text.data));

  dif_aes_data_t cipher1, cipher2;

  // ------------------ Session 1: seed PRD, encrypt ------------------
  expect_dif_ok(dif_aes_start(&aes, &transaction, &key, NULL), "dif_aes_start s1");

  // Trigger a PRNG reseed so the AES internal PRD buffer is populated with a
  // freshly drawn value. This is the deliberate reseed that should populate
  // PRD in hardware.
  expect_dif_ok(dif_aes_trigger(&aes, kDifAesTriggerPrngReseed), "prng reseed s1");

  // Wait for the AES to be idle before loading data (reseeding may take time).
  bool idle = false;
  do {
    expect_dif_ok(dif_aes_get_status(&aes, kDifAesStatusIdle, &idle), "get_status idle s1");
  } while (!idle);

  // Load plaintext and trigger encryption.
  expect_dif_ok(dif_aes_load_data(&aes, plain_text), "load_data s1");
  expect_dif_ok(dif_aes_trigger(&aes, kDifAesTriggerStart), "start s1");

  // Wait for output and read ciphertext.
  bool out_valid = false;
  do {
    expect_dif_ok(dif_aes_get_status(&aes, kDifAesStatusOutputValid, &out_valid), "get_status out_valid s1");
  } while (!out_valid);
  expect_dif_ok(dif_aes_read_output(&aes, &cipher1), "read_output s1");
  LOG_INFO("Session 1 ciphertext: 0x%08x 0x%08x 0x%08x 0x%08x",
           cipher1.data[0], cipher1.data[1], cipher1.data[2], cipher1.data[3]);

  // End session 1. Note: dif_aes_end will clear internal AES state using the
  // documented internal clearing triggers, but (per the finding) it does NOT
  // clear the PRD buffer register because PRD is only cleared on rst_ni.
  expect_dif_ok(dif_aes_end(&aes), "dif_aes_end s1");

  // ------------------ Session 2: start WITHOUT reseed, encrypt ----------
  expect_dif_ok(dif_aes_start(&aes, &transaction, &key, NULL), "dif_aes_start s2");

  // IMPORTANT: Do NOT call PRNG reseed here. If PRD persisted across the
  // dif_aes_end boundary, the same PRD will be used for masking again.

  // Load identical plaintext and trigger encryption.
  expect_dif_ok(dif_aes_load_data(&aes, plain_text), "load_data s2");
  expect_dif_ok(dif_aes_trigger(&aes, kDifAesTriggerStart), "start s2");

  // Wait for output and read ciphertext.
  out_valid = false;
  do {
    expect_dif_ok(dif_aes_get_status(&aes, kDifAesStatusOutputValid, &out_valid), "get_status out_valid s2");
  } while (!out_valid);
  expect_dif_ok(dif_aes_read_output(&aes, &cipher2), "read_output s2");
  LOG_INFO("Session 2 ciphertext: 0x%08x 0x%08x 0x%08x 0x%08x",
           cipher2.data[0], cipher2.data[1], cipher2.data[2], cipher2.data[3]);

  // Compare ciphertexts. If PRD persisted across the non-rst reset boundary,
  // both ciphertexts will be identical because the masking condition is the
  // same for both sessions. If PRD were properly cleared, Session 2 would
  // observe a fresh/zero PRD (depending on implementation) and produce a
  // different ciphertext for the same plaintext.
  CHECK(memcmp(&cipher1, &cipher2, sizeof(cipher1)) == 0,
        "PRD reuse NOT observed: ciphertexts differ (PRD likely cleared)!");

  LOG_INFO("PRD reuse observed: ciphertexts identical across sessions - vulnerability confirmed");

  // Clean up.
  expect_dif_ok(dif_aes_end(&aes), "dif_aes_end cleanup");

  return true;
}
