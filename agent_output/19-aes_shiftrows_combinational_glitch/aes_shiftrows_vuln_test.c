// POC: AES ShiftRows combinational glitch demonstration
// Vulnerability ID: 19

#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "aes_regs.h"  // Generated register offsets and bitfield defs

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TIMEOUT_US (1000000)

OTTF_DEFINE_TEST_CONFIG();

static void aes_set_multireg(mmio_region_t base, const uint32_t *data,
                            size_t regs_num, ptrdiff_t reg0_offset) {
  for (size_t i = 0; i < regs_num; ++i) {
    ptrdiff_t offset = reg0_offset + (ptrdiff_t)i * (ptrdiff_t)sizeof(uint32_t);
    mmio_region_write32(base, offset, data[i]);
  }
}

static void aes_read_multireg(mmio_region_t base, uint32_t *data,
                              size_t regs_num, ptrdiff_t reg0_offset) {
  for (size_t i = 0; i < regs_num; ++i) {
    ptrdiff_t offset = reg0_offset + (ptrdiff_t)i * (ptrdiff_t)sizeof(uint32_t);
    data[i] = mmio_region_read32(base, offset);
  }
}

static bool wait_for_status_bit(mmio_region_t base, ptrdiff_t status_offset,
                                bitfield_bit32_index_t bit, bool desired,
                                uint32_t timeout_us) {
  uint32_t i = 0;
  while (mmio_region_get_bit32(base, status_offset, (bit)) != desired) {
    if (i++ > timeout_us) {
      return false;
    }
  }
  return true;
}

bool test_main(void) {
  LOG_INFO("AES ShiftRows combinational glitch POC start");

  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Simple all-zero key shares for deterministic operation.
  uint32_t key0[8] = {0};
  uint32_t key1[8] = {0};

  // Example plaintext block (4 words)
  uint32_t plain[4] = {0x00112233, 0x44556677, 0x8899aabb, 0xccddeeff};
  uint32_t cipher_clean[4];
  uint32_t cipher_toggled[4];

  // 1) Configure AES control register for manual operation and ENCRYPT
  uint32_t ctrl_reg = 0;
  ctrl_reg = bitfield_field32_write(ctrl_reg, AES_CTRL_SHADOWED_OPERATION_FIELD,
                                    AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENCRYPT);
  // Set manual operation so that loading data does not auto-start.
  ctrl_reg = bitfield_bit32_write(ctrl_reg, AES_CTRL_SHADOWED_MANUAL_OPERATION_BIT, true);

  mmio_region_write32(aes, AES_CTRL_SHADOWED_REG_OFFSET, ctrl_reg);

  // 2) Load key shares (software-provided key)
  aes_set_multireg(aes, key0, AES_KEY_SHARE0_MULTIREG_COUNT, AES_KEY_SHARE0_0_REG_OFFSET);
  aes_set_multireg(aes, key1, AES_KEY_SHARE1_MULTIREG_COUNT, AES_KEY_SHARE1_0_REG_OFFSET);

  // 3) Baseline: perform clean encrypt run (no mid-cycle toggles)
  aes_set_multireg(aes, plain, AES_DATA_IN_MULTIREG_COUNT, AES_DATA_IN_0_REG_OFFSET);

  // Trigger start
  uint32_t trig = 0;
  trig = bitfield_bit32_write(trig, AES_TRIGGER_START_BIT, true);
  mmio_region_write32(aes, AES_TRIGGER_REG_OFFSET, trig);

  // Wait for output valid
  CHECK(wait_for_status_bit(aes, AES_STATUS_REG_OFFSET, AES_STATUS_OUTPUT_VALID_BIT, true, TIMEOUT_US), "Timeout waiting for OUTPUT_VALID (clean run)");
  aes_read_multireg(aes, cipher_clean, AES_DATA_OUT_MULTIREG_COUNT, AES_DATA_OUT_0_REG_OFFSET);

  LOG_INFO("Clean run output: 0x%08x 0x%08x 0x%08x 0x%08x",
           cipher_clean[0], cipher_clean[1], cipher_clean[2], cipher_clean[3]);

  // 4) Clear AES internal state and output registers (use trigger flags)
  uint32_t clear_trig = 0;
  clear_trig = bitfield_bit32_write(clear_trig, AES_TRIGGER_KEY_IV_DATA_IN_CLEAR_BIT, true);
  clear_trig = bitfield_bit32_write(clear_trig, AES_TRIGGER_DATA_OUT_CLEAR_BIT, true);
  mmio_region_write32(aes, AES_TRIGGER_REG_OFFSET, clear_trig);

  // Wait for AES to be idle before next run
  CHECK(wait_for_status_bit(aes, AES_STATUS_REG_OFFSET, AES_STATUS_IDLE_BIT, true, TIMEOUT_US), "Timeout waiting for IDLE after clear");

  // 5) Prepare for toggled run: write same configuration but we will toggle operation
  ctrl_reg = 0;
  ctrl_reg = bitfield_field32_write(ctrl_reg, AES_CTRL_SHADOWED_OPERATION_FIELD,
                                    AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENCRYPT);
  ctrl_reg = bitfield_bit32_write(ctrl_reg, AES_CTRL_SHADOWED_MANUAL_OPERATION_BIT, true);
  mmio_region_write32(aes, AES_CTRL_SHADOWED_REG_OFFSET, ctrl_reg);

  aes_set_multireg(aes, key0, AES_KEY_SHARE0_MULTIREG_COUNT, AES_KEY_SHARE0_0_REG_OFFSET);
  aes_set_multireg(aes, key1, AES_KEY_SHARE1_MULTIREG_COUNT, AES_KEY_SHARE1_0_REG_OFFSET);

  // 6) Write data_in but rapidly toggle op_i by switching the operation field
  aes_set_multireg(aes, plain, AES_DATA_IN_MULTIREG_COUNT, AES_DATA_IN_0_REG_OFFSET);

  // Perform rapid toggles between ENCRYPT and DECRYPT. These are combinational
  // writes to the shadowed control register that feed op_i. If op_i and
  // data_i are not synchronous, this may cause combinational muxes in
  // shift_rows to transiently select incorrect shifts.
  for (int i = 0; i < 16; ++i) {
    uint32_t toggled = 0;
    // Alternate between decrypt and encrypt values
    if (i & 1) {
      toggled = bitfield_field32_write(0, AES_CTRL_SHADOWED_OPERATION_FIELD,
                                       AES_CTRL_SHADOWED_OPERATION_VALUE_AES_DECRYPT);
    } else {
      toggled = bitfield_field32_write(0, AES_CTRL_SHADOWED_OPERATION_FIELD,
                                       AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENCRYPT);
    }
    toggled = bitfield_bit32_write(toggled, AES_CTRL_SHADOWED_MANUAL_OPERATION_BIT, true);
    mmio_region_write32(aes, AES_CTRL_SHADOWED_REG_OFFSET, toggled);
    // No deliberate delay - the goal is to make back-to-back writes happen
    // without waiting for AES internal state transitions.
  }

  // Finally, trigger start after toggles
  trig = 0;
  trig = bitfield_bit32_write(trig, AES_TRIGGER_START_BIT, true);
  mmio_region_write32(aes, AES_TRIGGER_REG_OFFSET, trig);

  // Wait for output valid
  CHECK(wait_for_status_bit(aes, AES_STATUS_REG_OFFSET, AES_STATUS_OUTPUT_VALID_BIT, true, TIMEOUT_US), "Timeout waiting for OUTPUT_VALID (toggled run)");
  aes_read_multireg(aes, cipher_toggled, AES_DATA_OUT_MULTIREG_COUNT, AES_DATA_OUT_0_REG_OFFSET);

  LOG_INFO("Toggled run output: 0x%08x 0x%08x 0x%08x 0x%08x",
           cipher_toggled[0], cipher_toggled[1], cipher_toggled[2], cipher_toggled[3]);

  // If toggles cause combinational hazards, the outputs of the two runs may differ.
  bool equal = true;
  for (int i = 0; i < 4; ++i) {
    if (cipher_clean[i] != cipher_toggled[i]) {
      equal = false;
      break;
    }
  }

  if (!equal) {
    LOG_INFO("VULN: Outputs differ after rapid op toggling - combinational glitch observed");
    return true; // Success: vulnerability demonstrated
  }

  // If outputs are the same, the POC did not observe a glitch.
  LOG_INFO("No observable difference between clean and toggled runs");
  CHECK(false, "POC did not trigger observable glitch");

  return false;
}
