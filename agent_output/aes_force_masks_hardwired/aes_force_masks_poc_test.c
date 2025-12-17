#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "aes_regs.h"  // Generated register offsets and bit indices

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  // Create an MMIO region for the AES peripheral using generated top-level
  // base address. This keeps the test independent of DIFs and uses base APIs
  // to perform low-level register accesses.
  mmio_region_t aes_base = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Read the current AUX shadowed register value and log it.
  uint32_t aux_before = mmio_region_read32(aes_base, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("AES CTRL_AUX_SHADOWED before: 0x%08x", aux_before);

  // Construct a value with FORCE_MASKS bit set. Use bitfield helpers so the
  // test does not depend on numeric bit positions being memorized.
  uint32_t aux_with_force = bitfield_bit32_write(aux_before, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT, true);

  // Write the shadowed register. The AES shadowed registers use a double-write
  // policy in DIF helpers; mimic that here to ensure the shadowed bank latches
  // correctly in hardware.
  mmio_region_write32(aes_base, AES_CTRL_AUX_SHADOWED_REG_OFFSET, aux_with_force);
  mmio_region_write32(aes_base, AES_CTRL_AUX_SHADOWED_REG_OFFSET, aux_with_force);

  // Write the REGWEN register to leave the AUX register writable. The DIF
  // implementation writes a boolean indicating whether the register should
  // remain unlocked. Here we set it to 1 (unlocked) following the same logic.
  mmio_region_write32(aes_base, AES_CTRL_AUX_REGWEN_REG_OFFSET, 1u);

  // Read back the AUX shadowed register to verify the FORCE_MASKS bit is set.
  uint32_t aux_after = mmio_region_read32(aes_base, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("AES CTRL_AUX_SHADOWED after : 0x%08x", aux_after);

  // Validate that the FORCE_MASKS bit is writable and retains the value.
  bool force_masks_set = bitfield_bit32_read(aux_after, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT);
  CHECK(force_masks_set, "Failed to set FORCE_MASKS in AES CTRL_AUX_SHADOWED");

  // The presence of a writable FORCE_MASKS bit despite the top-level secure
  // default means the core allowed forcing masks. This demonstrates the
  // vulnerability: software can disable masking randomness at runtime.
  LOG_INFO("SUCCESS: FORCE_MASKS bit is writable and set. AES masking can be forced.");

  return true;
}
