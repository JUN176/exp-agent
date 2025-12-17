#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

// Generated register offsets and bit indexes for AES.
#include "aes_regs.h"
// Top-level base address constants (TOP_EARLGREY_AES_BASE_ADDR).
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  LOG_INFO("AES FORCE_MASKS POC: verifying software can set FORCE_MASKS");

  // Create an MMIO region for the AES instance using the top-level base.
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Read the current AUX shadowed register value.
  uint32_t aux_before = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("AES CTRL_AUX_SHADOWED before = 0x%08x", aux_before);

  // Read whether AUX regwren allows writes. If zero, the shadowed register is
  // write-locked and attempts to change the value are expected to fail.
  uint32_t aux_regwen = mmio_region_read32(aes, AES_CTRL_AUX_REGWEN_REG_OFFSET);
  LOG_INFO("AES CTRL_AUX_REGWEN = 0x%08x", aux_regwen);

  // Construct a write that sets FORCE_MASKS = 1.
  // Use bitfield helper to avoid guessing bit positions.
  uint32_t force_masks_set = bitfield_bit32_write(0, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT, true);

  // Shadowed registers require a double write in software (see AES DIF
  // implementation). Apply the double write here using low-level mmio.
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, force_masks_set);
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, force_masks_set);

  // Read back the shadowed register to observe whether the bit was set.
  uint32_t aux_after = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("AES CTRL_AUX_SHADOWED after  = 0x%08x", aux_after);

  // Verify the FORCE_MASKS bit is now set.
  bool force_masks_is_set = bitfield_bit32_read(aux_after, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT);

  // If SecAllowForcingMasks was hardwired to 1 in the RTL, software should be
  // able to set this bit at runtime (unless the register has been locked by
  // earlier boot code). This is the core of the vulnerability: enabling the
  // CSR path allows software to force internal masking to constants.
  CHECK(force_masks_is_set,
        "FORCE_MASKS bit not set - AES may be configured to disallow forcing masks or locked");

  LOG_INFO("SUCCESS: FORCE_MASKS bit is writable and set. This demonstrates the AES design allows software to force masks at runtime.");

  // For completeness, also demonstrate that software can trigger a PRNG reseed
  // which—combined with FORCE_MASKS=1—can be used to produce constant masks
  // for subsequent operations. This writes the PRNG reseed trigger bit.
  uint32_t trigger_reg = bitfield_bit32_write(0, AES_TRIGGER_PRNG_RESEED_BIT, true);
  mmio_region_write32(aes, AES_TRIGGER_REG_OFFSET, trigger_reg);
  LOG_INFO("Triggered PRNG reseed (write to AES_TRIGGER).");

  return true;
}
