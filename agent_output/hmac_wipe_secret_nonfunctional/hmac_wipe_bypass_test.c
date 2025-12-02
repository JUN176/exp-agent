#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_hmac.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/hmac_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// Reuse the same data and expected vectors as existing secure wipe tests.
static const char kData[142] =
    "Every one suspects himself of at least one of "
    "the cardinal virtues, and this is mine: I am "
    "one of the few honest people that I have ever "
    "known";

static uint32_t kHmacKey[8] = {
    0xec4e6c89, 0x082efa98, 0x299f31d0, 0xa4093822,
    0x03707344, 0x13198a2e, 0x85a308d3, 0x243f6a88,
};

static const dif_hmac_digest_t kExpectedHmacDigest = {
    .digest =
        {
            0xebce4019,
            0x284d39f1,
            0x5eae12b0,
            0x0c48fb23,
            0xfadb9531,
            0xafbbf3c2,
            0x90d3833f,
            0x397b98e4,
        },
};

bool test_main(void) {
  dif_hmac_t hmac;
  // Initialize DIF
  CHECK_DIF_OK(
      dif_hmac_init(mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR), &hmac));

  const dif_hmac_transaction_t kHmacTransactionConfig = {
      .digest_endianness = kDifHmacEndiannessLittle,
      .message_endianness = kDifHmacEndiannessLittle,
  };

  LOG_INFO("Step 1: Start HMAC with a known key and compute reference digest");
  // Start HMAC in HMAC mode and provide the key.
  CHECK_DIF_OK(dif_hmac_mode_hmac_start(&hmac, (uint8_t *)(&kHmacKey[0]),
                                        kHmacTransactionConfig));

  // Push message, wait for FIFO to drain and process.
  CHECK_STATUS_OK(hmac_testutils_push_message(&hmac, kData, sizeof(kData)));
  CHECK_STATUS_OK(hmac_testutils_fifo_empty_polled(&hmac));
  CHECK_STATUS_OK(hmac_testutils_check_message_length(&hmac, sizeof(kData) * 8));
  CHECK_DIF_OK(dif_hmac_process(&hmac));

  // Fetch final digest and verify it matches expected. This verifies the key is
  // loaded and HMAC produces the known-good value.
  dif_hmac_digest_t digest;
  CHECK_STATUS_OK(hmac_testutils_finish_polled(&hmac, &digest));
  CHECK_ARRAYS_EQ(digest.digest, kExpectedHmacDigest.digest,
                  ARRAYSIZE(digest.digest));

  LOG_INFO("Step 2: Invoke secure wipe (dif_hmac_wipe_secret)");
  // According to the HW/DIF contract, this should clobber the internal key
  // and return a digest consisting of the wipe entropy (secure wipe value).
  const uint32_t kWipeEntropy = UINT32_MAX;
  CHECK_DIF_OK(dif_hmac_wipe_secret(&hmac, kWipeEntropy, &digest));

  // Vulnerability: due to a HW bug, the write-enable for the wipe register
  // only occurs when an error flag is set. Under normal operation the wipe
  // request is silently ignored. Therefore the digest returned here will be
  // the previous computed digest instead of the expected wipe pattern.
  LOG_INFO("Digest returned by dif_hmac_wipe_secret:");
  LOG_INFO("%08x %08x %08x %08x %08x %08x %08x %08x",
           digest.digest[0], digest.digest[1], digest.digest[2],
           digest.digest[3], digest.digest[4], digest.digest[5],
           digest.digest[6], digest.digest[7]);

  // If the hardware behaved correctly, digest would be filled with the
  // wipe entropy (e.g., UINT32_MAX). The vulnerability causes the digest to
  // remain equal to the original expected digest. Assert equality to prove
  // the failure of the secure wipe.
  CHECK_ARRAYS_EQ(digest.digest, kExpectedHmacDigest.digest,
                  ARRAYSIZE(digest.digest));

  LOG_INFO("Step 3: Start HMAC with key=NULL to verify key persistence");
  // Start a new HMAC transaction without supplying a key. If the wipe had
  // worked, HMAC should operate without a key and produce a different digest.
  CHECK_DIF_OK(dif_hmac_mode_hmac_start(&hmac, /*key=*/NULL,
                                        kHmacTransactionConfig));
  CHECK_STATUS_OK(hmac_testutils_push_message(&hmac, kData, sizeof(kData)));
  CHECK_STATUS_OK(hmac_testutils_fifo_empty_polled(&hmac));
  CHECK_STATUS_OK(hmac_testutils_check_message_length(&hmac, sizeof(kData) * 8));
  CHECK_DIF_OK(dif_hmac_process(&hmac));
  CHECK_STATUS_OK(hmac_testutils_finish_polled(&hmac, &digest));

  LOG_INFO("Digest computed with key=NULL:");
  LOG_INFO("%08x %08x %08x %08x %08x %08x %08x %08x",
           digest.digest[0], digest.digest[1], digest.digest[2],
           digest.digest[3], digest.digest[4], digest.digest[5],
           digest.digest[6], digest.digest[7]);

  // Vulnerability proof: The digest computed without providing a key still
  // matches the original expected digest, proving the previous key remained
  // loaded in hardware and secure wipe did not take effect.
  CHECK_ARRAYS_EQ(digest.digest, kExpectedHmacDigest.digest,
                  ARRAYSIZE(digest.digest));

  LOG_INFO("VULNERABILITY: HMAC secure wipe did not clear the key under normal conditions.");

  return true;
}
