#include <stdint.h>
#include <stddef.h>

#include "sw/device/lib/runtime/ibex.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/memory.h"

#include "sw/device/lib/dif/dif_entropy_src.h"

#include "entropy_src_regs.h"  // Generated register offsets
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"  // Base addrs

OTTF_DEFINE_TEST_CONFIG();

// This test attempts to provoke Markov health-test failures by feeding
// deterministic / highly correlated data into the entropy source using the
// firmware-override (FW_OV) interface. Because the RTL ties the Markov
// "fail pulse" outputs to 0 and does not perform threshold checks at
// window boundaries, the Markov health-test counters will remain zero even
// when presented with bad input. The test asserts that Markov failures are
// observed; if they are not, we treat that as the vulnerability being
// present and fail the test (demonstrating the issue).

enum {
  // Number of 32-bit words to write into the FW override FIFO to provoke
  // failures. Each word contains multiple entropy samples depending on
  // configuration; write a generous amount to cover several windows.
  kFwOvWordsToWrite = 4096,
  // Words written in each fifo write call.
  kFwOvWriteChunkWords = 16,
};

bool test_main(void) {
  mmio_region_t entropy_base = mmio_region_from_addr(
      TOP_EARLGREY_ENTROPY_SRC_BASE_ADDR);

  dif_entropy_src_t entropy;
  CHECK(dif_entropy_src_init(entropy_base, &entropy) == kDifOk,
        "dif_entropy_src_init failed");

  // 1) Configure the Markov health-test to be extremely sensitive so that
  // deterministic / highly correlated data should trigger failures quickly.
  dif_entropy_src_health_test_config_t markov_cfg = {};
  markov_cfg.test_type = kDifEntropySrcTestMarkov;
  // Very low high threshold to force failure on correlated data.
  markov_cfg.high_threshold = 1;
  // No low threshold for Markov in many implementations; set to 0.
  markov_cfg.low_threshold = 0;

  CHECK(dif_entropy_src_health_test_configure(&entropy, markov_cfg) ==
            kDifOk,
        "dif_entropy_src_health_test_configure(Markov) failed");

  // 2) Enable the firmware-override feature and make sure the FIFO threshold
  // is small so we can stream data.
  dif_entropy_src_fw_override_config_t fwov_cfg = {};
  fwov_cfg.entropy_insert_enable = true;
  fwov_cfg.buffer_threshold = 8;  // small threshold

  CHECK(dif_entropy_src_fw_override_configure(&entropy, fwov_cfg,
                                              kDifToggleEnabled) ==
            kDifOk,
        "dif_entropy_src_fw_override_configure failed");

  // 3) Enable ENTROPY_SRC in a mode that routes override data through the
  // health-test machinery. We enable the block using the DIF configure
  // helper so default safe settings are applied.
  dif_entropy_src_config_t config = {};
  config.fips_enable = false;
  config.fips_flag = false;
  config.rng_fips = false;
  config.route_to_firmware = false;
  config.bypass_conditioner = true;  // bypass conditioner to make input visible
  config.single_bit_mode = kDifEntropySrcSingleBitModeDisabled;
  config.health_test_threshold_scope = false;
  config.health_test_window_size = 512;  // reasonable window size in bits
  config.alert_threshold = UINT16_MAX;

  CHECK(dif_entropy_src_configure(&entropy, config, kDifToggleEnabled) ==
            kDifOk,
        "dif_entropy_src_configure failed");

  // 4) Prepare a deterministic pattern that should strongly violate the
  // Markov test (e.g., repeating 0xAAAAAAAA or 0x00000000 to force
  // correlations/toggling). We'll write chunks into the FW OV FIFO.
  uint32_t pattern_chunk[kFwOvWriteChunkWords];
  for (size_t i = 0; i < kFwOvWriteChunkWords; ++i) {
    // Alternate pattern to trigger first-order correlations.
    pattern_chunk[i] = (i % 2) ? 0xAAAAAAAAu : 0x55555555u;
  }

  size_t words_remaining = kFwOvWordsToWrite;
  while (words_remaining > 0) {
    size_t to_write = words_remaining;
    if (to_write > kFwOvWriteChunkWords) {
      to_write = kFwOvWriteChunkWords;
    }

    size_t words_written = 0;
    // Start SHA3 insert so the override FIFO will be consumed.
    CHECK(dif_entropy_src_fw_override_sha3_start_insert(&entropy,
                                                         kDifToggleEnabled) ==
              kDifOk,
          "dif_entropy_src_fw_override_sha3_start_insert(enable) failed");

    CHECK(dif_entropy_src_fw_ov_data_write(&entropy, pattern_chunk, to_write,
                                           &words_written) == kDifOk,
          "dif_entropy_src_fw_ov_data_write failed");

    CHECK(dif_entropy_src_fw_override_sha3_start_insert(&entropy,
                                                         kDifToggleDisabled) ==
              kDifOk,
          "dif_entropy_src_fw_override_sha3_start_insert(disable) failed");

    // If the FIFO wasn't able to accept all words (unlikely in test), back off
    // and retry until it's drained.
    if (words_written == 0) {
      // Small delay
      busy_spin_micros(10);
      continue;
    }

    words_remaining -= words_written;
  }

  // 5) Give the hardware some time to process the data and update health-test
  // counters. In simulation this may require a few milliseconds.
  busy_spin_micros(1000);

  // 6) Read back the health-test statistics via DIF and inspect the Markov
  // failure counters. According to the design, these counters should be
  // incremented when the Markov test exceeds thresholds. We expect >0 fails
  // for our strongly correlated patterns. If the RTL has the bug (pulses
  // tied to 0 / comparisons not performed), the counters will remain 0.
  dif_entropy_src_health_test_stats_t stats;
  CHECK(dif_entropy_src_get_health_test_stats(&entropy, &stats) == kDifOk,
        "dif_entropy_src_get_health_test_stats failed");

  uint32_t markov_high_fails = stats.high_fails[kDifEntropySrcTestMarkov];

  LOG_INFO("Markov high_fails = %u", markov_high_fails);

  // We expect at least one failure. If none are observed, that's evidence of
  // the vulnerability described.
  CHECK(markov_high_fails > 0,
        "Markov health-test did not record any high-threshold failures. "
        "This indicates the Markov health-test logic may be disabled (" 
        "e.g., test_fail_{hi,lo}_pulse_o tied low or comparisons skipped).");

  return true;
}
