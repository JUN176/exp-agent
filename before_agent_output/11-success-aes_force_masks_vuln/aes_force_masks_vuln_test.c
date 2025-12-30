#include <stdint.h>
#include <stdbool.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "aes_regs.h" // Generated register offsets and bit indices.

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void)
{
  // Create an MMIO region for the AES instance using generated base address.
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  LOG_INFO("AES force-masks vulnerability check starting...");

  // Read the shadowed auxiliary control register which contains the
  // FORCE_MASKS bit.
  uint32_t aux = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  bool force_masks_set = bitfield_bit32_read(aux,
                                             AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT);

  if (force_masks_set)
  {
    LOG_INFO("FORCE_MASKS bit is already set (vulnerable: software can force masks at runtime)");
    // We still assert that software can read the bit and observe it set.
    CHECK(force_masks_set, "Expected FORCE_MASKS to be set");
    return true;
  }

  LOG_INFO("FORCE_MASKS bit is not set. Attempting to enable it via software...");

  // Prepare a shadowed write value that sets only the FORCE_MASKS bit.
  uint32_t new_aux = bitfield_bit32_write(0,
                                          AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT,
                                          true);

  // Shadowed registers require writing the value twice. Use structured
  // mmio_region_write32 to perform the double-write analogous to
  // aes_shadowed_write in DIF implementation.
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, new_aux);
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, new_aux);

  // Also ensure the AUX register write-enable (REGWEN) is enabled so that
  // the write takes effect like the DIF would do when not locked. The DIF
  // expects a 1 when the aux control is not locked (ctrl_aux_lock == false).
  mmio_region_write32(aes, AES_CTRL_AUX_REGWEN_REG_OFFSET, 1u);

  // Read back the shadowed aux register to verify FORCE_MASKS took effect.
  uint32_t aux_after = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  bool force_masks_after = bitfield_bit32_read(aux_after,
                                               AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT);

  LOG_INFO("Read back AUX_SHADOWED=0x%08x", aux_after);

  // If the hardware was instantiated with SecAllowForcingMasks hard-wired to
  // 1, the FORCE_MASKS path is available to software and setting the bit will
  // succeed. This proves the vulnerability described in the report.
  CHECK(force_masks_after, "Failed to enable FORCE_MASKS via CSR path - target may not be vulnerable or register is locked");

  LOG_INFO("Successfully set FORCE_MASKS via CSR path. AES masking can be forced to constant values by software (vulnerable)");

  return true;
}
