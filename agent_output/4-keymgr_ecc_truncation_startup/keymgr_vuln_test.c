#include <stdbool.h>
#include <stdint.h>

#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "keymgr_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// This test attempts to reproduce an ECC / invalid-state failure during
// keymgr startup described in the vulnerability report. The RTL bug causes
// an assignment to be truncated which leads to an ECC error and the keymgr
// entering an invalid state. We exercise the "advance state" operation
// using only base library MMIO primitives to trigger the faulty path.

bool test_main(void) {
  mmio_region_t keymgr = mmio_region_from_addr(TOP_EARLGREY_KEYMGR_BASE_ADDR);

  LOG_INFO("[POC] Triggering keymgr advance operation to provoke ECC error");

  // Construct CONTROL_SHADOWED value: DEST = NONE, OP = ADVANCE, CDI_SEL = 0
  uint32_t reg_control = bitfield_field32_write(
      0u, KEYMGR_CONTROL_SHADOWED_DEST_SEL_FIELD,
      KEYMGR_CONTROL_SHADOWED_DEST_SEL_VALUE_NONE);

  reg_control = bitfield_field32_write(
      reg_control, KEYMGR_CONTROL_SHADOWED_OPERATION_FIELD,
      KEYMGR_CONTROL_SHADOWED_OPERATION_VALUE_ADVANCE);

  // Set CDI_SEL bit to 0 (sealing CDI) - using a manual field descriptor as
  // the auto-generated macros provide the bit index for this single-bit field.
  reg_control = bitfield_field32_write(
      reg_control,
      (bitfield_field32_t){.mask = 0x1u, .index = KEYMGR_CONTROL_SHADOWED_CDI_SEL_BIT},
      0u);

  // Write CONTROL_SHADOWED (shadowed register write) and then START (non-shadowed)
  mmio_region_write32_shadowed(keymgr, KEYMGR_CONTROL_SHADOWED_REG_OFFSET,
                               reg_control);
  mmio_region_write32(keymgr, KEYMGR_START_REG_OFFSET,
                      (1u << KEYMGR_START_EN_BIT));

  // Poll OP_STATUS register until DONE_SUCCESS or DONE_ERROR (or timeout)
  const int kTimeout = 100000;
  int i = 0;
  uint32_t op_status = 0;
  for (; i < kTimeout; ++i) {
    op_status = mmio_region_read32(keymgr, KEYMGR_OP_STATUS_REG_OFFSET);
    if (op_status == KEYMGR_OP_STATUS_STATUS_VALUE_DONE_SUCCESS ||
        op_status == KEYMGR_OP_STATUS_STATUS_VALUE_DONE_ERROR) {
      break;
    }
  }

  CHECK(i < kTimeout, "Keymgr operation timed out");

  if (op_status == KEYMGR_OP_STATUS_STATUS_VALUE_DONE_ERROR) {
    // Read and report the error code.
    uint32_t err_code = mmio_region_read32(keymgr, KEYMGR_ERR_CODE_REG_OFFSET);
    LOG_INFO("Keymgr reported DONE_ERROR. ERR_CODE=0x%08x", err_code);

    // Clear OP_STATUS and ERR_CODE (rw1c behavior) to read state cleanly.
    mmio_region_write32(keymgr, KEYMGR_OP_STATUS_REG_OFFSET, op_status);
    mmio_region_write32(keymgr, KEYMGR_ERR_CODE_REG_OFFSET, err_code);

    // Read working state and extract STATE field.
    uint32_t reg_state = mmio_region_read32(keymgr, KEYMGR_WORKING_STATE_REG_OFFSET);
    uint32_t state_field = bitfield_field32_read(reg_state, KEYMGR_WORKING_STATE_STATE_FIELD);
    LOG_INFO("Working state field after error = 0x%08x", state_field);

    // Check for INVALID state which is the expected failure mode for this bug.
    CHECK(state_field == KEYMGR_WORKING_STATE_STATE_VALUE_INVALID,
          "Expected keymgr to enter INVALID state due to truncated assignment (bug)");

    LOG_INFO("POC detected ECC/truncation failure leading to INVALID state - test PASS");
    return true;
  }

  // If the operation completed successfully, the bug was not triggered in this run.
  LOG_INFO("Keymgr operation completed successfully (OP_STATUS=0x%08x).", op_status);
  CHECK(false, "Bug not triggered: keymgr did not report ECC error/INVALID state");
  return false;
}
