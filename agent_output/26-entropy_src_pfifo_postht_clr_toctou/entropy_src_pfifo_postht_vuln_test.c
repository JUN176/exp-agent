#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "entropy_src_regs.h"  // Generated register offsets and field macros

OTTF_DEFINE_TEST_CONFIG();

// This POC demonstrates the TOCTOU race in pfifo_postht_clr described in
// vulnerability report #26. The core idea is to:
// 1) Enable firmware-override mode with entropy-insert enabled.
// 2) Push one word into the firmware-insert data port.
// 3) Immediately toggle the relevant enable (simulate a disable) and
//    re-enable to create the disable->enable boundary.
// 4) Observe whether the pushed word persists across the disable->enable
//    boundary (it should not). If it does, the pfifo_postht clear was
//    suppressed by a stale pfifo_postht_not_empty register and stale data
//    survived the intended flush-on-disable.

// Helper: compose FW_OV_CONTROL register value. We use the auto-generated
// field macros defined in entropy_src_regs.h.
static inline uint32_t fw_ov_control_val(bool fw_ov_mode_enable,
                                         bool entropy_insert_enable) {
  uint32_t reg = 0;
  reg = bitfield_field32_write(
      reg, ENTROPY_SRC_FW_OV_CONTROL_FW_OV_MODE_FIELD,
      fw_ov_mode_enable ? ENTROPY_SRC_FW_OV_CONTROL_FW_OV_MODE_MASK
                        : 0);
  reg = bitfield_field32_write(
      reg, ENTROPY_SRC_FW_OV_CONTROL_FW_OV_ENTROPY_INSERT_FIELD,
      entropy_insert_enable ? ENTROPY_SRC_FW_OV_CONTROL_FW_OV_ENTROPY_INSERT_MASK
                            : 0);
  return reg;
}

// Helper: compose FW_OV_SHA3_START register value (start/stop insert)
static inline uint32_t fw_ov_sha3_start_val(bool insert_start) {
  uint32_t reg = 0;
  reg = bitfield_field32_write(
      reg, ENTROPY_SRC_FW_OV_SHA3_START_FW_OV_INSERT_START_FIELD,
      insert_start ? ENTROPY_SRC_FW_OV_SHA3_START_FW_OV_INSERT_START_MASK
                   : 0);
  return reg;
}

bool test_main(void) {
  // Base MMIO region for entropy_src
  mmio_region_t es = mmio_region_from_addr(TOP_EARLGREY_ENTROPY_SRC_BASE_ADDR);

  LOG_INFO("Starting pfifo_postht_clr TOCTOU POC");

  // 1) Enable FW override mode with entropy insert enabled and set a small
  //    observation FIFO threshold so that FIFO activity is visible.
  mmio_region_write32(es, ENTROPY_SRC_OBSERVE_FIFO_THRESH_REG_OFFSET, 4);

  // Write FW_OV_CONTROL: enable fw override mode and enable entropy insert.
  uint32_t control = fw_ov_control_val(true /*fw_ov_mode_enable*/,
                                       true /*entropy_insert_enable*/);
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_CONTROL_REG_OFFSET, control);

  // Ensure SHA3 insert state is idle (not inserting) before pushing data.
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_SHA3_START_REG_OFFSET,
                      fw_ov_sha3_start_val(false));

  // 2) Push a identifiable word into the firmware insert data port.
  const uint32_t kMagic = 0xA5A5A5A5;
  LOG_INFO("Pushing magic word 0x%08x into FW_OV data port", kMagic);
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_DATA_REG_OFFSET, kMagic);

  // Start SHA3 insertion briefly so the hw accepts the word (simulate a
  // write acceptance). The sequence below tries to create a tight window in
  // which the hardware may accept the last push at the same time as the
  // module's es_enable_fo[7] is cleared by firmware.
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_SHA3_START_REG_OFFSET,
                      fw_ov_sha3_start_val(true));

  // 3) Immediately toggle the module-level/feature enable that should cause
  //    a flush. In the faulty RTL, the clear is gated by a registered
  //    pfifo_postht_not_empty which can lag the write acceptance. We simulate
  //    the disable+enable boundary by clearing and then restoring the
  //    entropy insert enable bit in FW_OV_CONTROL.
  // NOTE: The exact enable bit referenced in the RTL is `es_enable_fo[7]`.
  // In software-visible control we model this by toggling the FW_OV
  // entropy insert feature. This approximates the timing window that
  // exposes the TOCTOU.

  // Disable entropy insert (this should cause the post-HT FIFO to be
  // synchronously cleared by logic that implements flush-on-disable).
  uint32_t control_disabled = fw_ov_control_val(true /*fw_ov_mode_enable*/,
                                                 false /*entropy_insert*/);
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_CONTROL_REG_OFFSET,
                      control_disabled);

  // Very small delay barrier to try and increase the chance the hardware
  // takes the disable concurrently with the earlier write. We use a dummy
  // volatile read as a lightweight fence.
  volatile uint32_t tmp = mmio_region_read32(es, ENTROPY_SRC_FW_OV_CONTROL_REG_OFFSET);
  (void)tmp;

  // Re-enable entropy insert immediately.
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_CONTROL_REG_OFFSET, control);

  // Stop SHA3 insertion (allow hardware to process the inserted word).
  mmio_region_write32(es, ENTROPY_SRC_FW_OV_SHA3_START_REG_OFFSET,
                      fw_ov_sha3_start_val(false));

  // 4) Read the observation/output FIFO to detect whether the magic word
  // persists across the disable->enable boundary. If the post-HT PFIFO was
  // not properly flushed on disable, we will observe the magic word here.

  // Wait a little by polling the observe FIFO threshold level/status. For
  // portability we attempt a few reads of the observe FIFO register. The
  // expected behavior in a correct implementation: the FIFO should be empty
  // after the disable; hence no magic word should be observed. If we read
  // the magic word, the TOCTOU exists.
  const int kPollTries = 8;
  bool found = false;
  for (int i = 0; i < kPollTries; ++i) {
    uint32_t val = mmio_region_read32(es, ENTROPY_SRC_OBSERVE_FIFO_REG_OFFSET);
    if (val == kMagic) {
      found = true;
      LOG_INFO("Observed stale magic word at poll %d", i);
      break;
    }
  }

  // Report POC result: vulnerability existence is indicated by observing
  // the magic word after a disable->enable cycle.
  CHECK(!found, "pfifo_postht_clr TOCTOU: stale data survived disable-enable boundary");

  LOG_INFO("No stale data observed; either hardware fixed or timing did not hit the race");

  return true;
}
