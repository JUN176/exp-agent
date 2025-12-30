#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/dif/dif_otbn.h"
#include "sw/device/lib/testing/otbn_testutils.h"
#include "sw/device/lib/runtime/log.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

#include <string.h>

OTTF_DEFINE_TEST_CONFIG();

// This test demonstrates that IMEM and DMEM can be read while OTBN is busy
// executing when the blanker enable signals are incorrectly tied high.
//
// Strategy:
// 1. Initialize OTBN via the DIF.
// 2. Write a known pattern into IMEM and DMEM.
// 3. Start OTBN execution (Execute command).
// 4. While OTBN reports BUSY_EXECUTE, repeatedly attempt to read IMEM and
//    DMEM using the DIF read helpers. If the reads return the previously
//    written pattern while OTBN is busy, the blanking feature is disabled
//    and the vulnerability is present.
//
// Note: We use DIF helpers for convenience to access IMEM/DMEM registers.
// The core vulnerability is in hardware (otbn.sv) where the blanker's
// enable input is tied to constant '1', causing the blanker to pass data
// through during execution.

bool test_main(void) {
  dif_otbn_t otbn;

  // Initialize OTBN DIF using the MMIO base address provided by the top
  CHECK_DIF_OK(dif_otbn_init(mmio_region_from_addr(TOP_EARLGREY_OTBN_BASE_ADDR), &otbn));

  // Prepare a small pattern to write into IMEM and DMEM.
  const uint32_t kImemPattern[4] = {0xDEADBEEF, 0xFEEDFACE, 0x11223344, 0xA5A5A5A5};
  const uint32_t kDmemPattern[4] = {0xCAFEBABE, 0x0BADC0DE, 0x5555AAAA, 0x99996666};

  // Write pattern to IMEM offset 0 and DMEM offset 0.
  CHECK_DIF_OK(dif_otbn_imem_write(&otbn, /*offset_bytes=*/0, kImemPattern, sizeof(kImemPattern)));
  CHECK_DIF_OK(dif_otbn_dmem_write(&otbn, /*offset_bytes=*/0, kDmemPattern, sizeof(kDmemPattern)));

  LOG_INFO("Patterns written to IMEM and DMEM. Starting OTBN execute...");

  // Start OTBN execution. The application at IMEM offset 0 may finish quickly,
  // but we poll the status and attempt reads while status reports BUSY_EXECUTE.
  CHECK_DIF_OK(dif_otbn_write_cmd(&otbn, kDifOtbnCmdExecute));

  bool imem_leaked = false;
  bool dmem_leaked = false;

  // Buffers for reads.
  uint32_t imem_read[ARRAYSIZE(kImemPattern)];
  uint32_t dmem_read[ARRAYSIZE(kDmemPattern)];

  // Poll for busy status and attempt reads while busy. Use a timeout to avoid
  // infinite loops on simulators or if something goes wrong.
  const int kMaxPolls = 1000;
  for (int attempt = 0; attempt < kMaxPolls; ++attempt) {
    dif_otbn_status_t status;
    CHECK_DIF_OK(dif_otbn_get_status(&otbn, &status));

    if (status == kDifOtbnStatusBusyExecute) {
      // Attempt to read IMEM and DMEM while OTBN is executing. If the blanker
      // is disabled, these reads should return the patterns we wrote above.
      CHECK_DIF_OK(dif_otbn_imem_read(&otbn, /*offset_bytes=*/0, imem_read, sizeof(imem_read)));
      CHECK_DIF_OK(dif_otbn_dmem_read(&otbn, /*offset_bytes=*/0, dmem_read, sizeof(dmem_read)));

      if (memcmp(imem_read, kImemPattern, sizeof(kImemPattern)) == 0) {
        imem_leaked = true;
        LOG_INFO("IMEM data readable while OTBN BUSY: leak confirmed");
      } else {
        LOG_INFO("IMEM read while BUSY returned non-matching data (attempt %d)", attempt);
      }

      if (memcmp(dmem_read, kDmemPattern, sizeof(kDmemPattern)) == 0) {
        dmem_leaked = true;
        LOG_INFO("DMEM data readable while OTBN BUSY: leak confirmed");
      } else {
        LOG_INFO("DMEM read while BUSY returned non-matching data (attempt %d)", attempt);
      }

      // If both leaked, we can stop early.
      if (imem_leaked && dmem_leaked) {
        break;
      }
    }
  }

  // Final check: both IMEM and DMEM should be readable while BUSY for the
  // vulnerability to be present as described.
  CHECK(imem_leaked && dmem_leaked,
        "Blanker did not prevent reads during execution: IMEM leaked=%d DMEM leaked=%d",
        imem_leaked, dmem_leaked);

  // Cleanup: wait for OTBN to finish if it hasn't already. This is good test
  // hygiene but not required for the exploit proof.
  CHECK_STATUS_OK(otbn_testutils_wait_for_done(&otbn, kDifOtbnErrBitsNoError));

  return true;
}
