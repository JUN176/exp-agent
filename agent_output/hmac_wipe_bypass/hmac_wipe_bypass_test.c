#include <stdbool.h>
#include <string.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_hmac.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/hmac_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// A reference key and message used to produce a known digest.
static uint32_t kHmacKey[8] = {
    0xec4e6c89, 0x082efa98, 0x299f31d0, 0xa4093822,
    0x03707344, 0x13198a2e, 0x85a308d3, 0x243f6a88,
};

static const char kData[] = "Every one suspects himself of at least one of the cardinal virtues";

bool test_main(void) {
  dif_hmac_t hmac;
  CHECK_DIF_OK(
      dif_hmac_init(mmio_region_from_addr(TOP_EARLGREY_HMAC_BASE_ADDR), &hmac));

  static const dif_hmac_transaction_t kHmacTransactionConfig = {
      .digest_endianness = kDifHmacEndiannessLittle,
      .message_endianness = kDifHmacEndiannessLittle,
      .digest_size = kDifSHA256,
      .key_length = kDifHMACKey256,
  };

  // Step 1: Compute a digest with an explicit key.
  LOG_INFO("Step 1: Start HMAC with explicit key and compute digest");
  CHECK_DIF_OK(dif_hmac_mode_hmac_start(&hmac, (uint8_t *)(&kHmacKey[0]),
                                        kHmacTransactionConfig));
  CHECK_STATUS_OK(hmac_testutils_push_message(&hmac, kData, sizeof(kData)));
  CHECK_STATUS_OK(hmac_testutils_fifo_empty_polled(&hmac));
  CHECK_STATUS_OK(hmac_testutils_check_message_length(&hmac, sizeof(kData) * 8));
  CHECK_DIF_OK(dif_hmac_process(&hmac));

  dif_hmac_digest_t digest_before;
  CHECK_STATUS_OK(hmac_testutils_finish_polled(&hmac, &digest_before));
  LOG_INFO("digest_before[0]=0x%08x", digest_before.digest[0]);

  // Step 2: Invoke the secure wipe primitive.
  // According to the RTL vulnerability, the wipe only takes effect when an
  // internal reg_error flag is set. Under normal conditions (no error), the
  // write to the wipe register will be ignored, leaving the previously loaded
  // key intact. We trigger the DIF API as a normal client would.
  LOG_INFO("Step 2: Call dif_hmac_wipe_secret() with non-error entropy");
  dif_hmac_digest_t digest_wipe;
  const uint32_t entropy = 0xA5A5A5A5;
  CHECK_DIF_OK(dif_hmac_wipe_secret(&hmac, entropy, &digest_wipe));
  LOG_INFO("digest_wipe[0]=0x%08x", digest_wipe.digest[0]);

  // Step 3: Start a new HMAC operation WITHOUT supplying a key. If the wipe
  // worked correctly, the HMAC engine should not have a key and the computed
  // digest should differ from the original. If the hardware is vulnerable,
  // the previous key remains and the digest will match digest_before.
  LOG_INFO("Step 3: Start HMAC with key=NULL and compute digest");
  CHECK_DIF_OK(dif_hmac_mode_hmac_start(&hmac, /*key=*/NULL, kHmacTransactionConfig));
  CHECK_STATUS_OK(hmac_testutils_push_message(&hmac, kData, sizeof(kData)));
  CHECK_STATUS_OK(hmac_testutils_fifo_empty_polled(&hmac));
  CHECK_STATUS_OK(hmac_testutils_check_message_length(&hmac, sizeof(kData) * 8));
  CHECK_DIF_OK(dif_hmac_process(&hmac));

  dif_hmac_digest_t digest_after;
  CHECK_STATUS_OK(hmac_testutils_finish_polled(&hmac, &digest_after));
  LOG_INFO("digest_after[0]=0x%08x", digest_after.digest[0]);

  // Compare the digests. If equal, the secure wipe did not remove the key and
  // the vulnerability is demonstrated.
  bool same = true;
  for (size_t i = 0; i < ARRAYSIZE(digest_before.digest); ++i) {
    if (digest_before.digest[i] != digest_after.digest[i]) {
      same = false;
      break;
    }
  }

  CHECK(same, "Secure wipe cleared the key (no vulnerability detected)");

  LOG_INFO("VULNERABILITY DETECTED: HMAC key persisted after secure wipe");
  return true;
}
