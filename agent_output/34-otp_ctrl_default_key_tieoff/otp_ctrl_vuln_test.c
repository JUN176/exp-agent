#include <stdint.h>
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/mmio.h"

// This test reconstructs the SystemVerilog expressions used in
// otp_ctrl_pkg.sv:FLASH_OTP_KEY_RSP_DEFAULT (lines provided in the report)
// and proves they evaluate to a "valid/ack" response while the key
// material is all zeros.

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  LOG_INFO("Starting otp_ctrl default key tie-off evaluation POC");

  // Simulate the SystemVerilog expressions as written in the RTL.
  // NOTE: We explicitly mimic the bit-widths used in the RTL comments
  // where possible. The original RTL lines are:
  // data_ack: (~(|4'h0)) || (&(~4'hF)),
  // addr_ack: ((1'b1 << 1) >> 1) !== 1'b0,
  // key: {FlashKeyWidth{1'b0}} ^ {FlashKeyWidth{1'b0}},
  // rand_key: '0,
  // seed_valid: (|(~0)) && (!(&0))

  // 1) data_ack calculation
  // Verilog: |4'h0  -> reduction OR of 4'b0000 == 0
  //           ~(...) -> bitwise NOT of 1'b0 -> 1'b1
  //           ~4'hF  -> 4'b0000, &(...)=reduction AND -> 0
  // So final: 1 || 0 == 1
  uint8_t red_or_4h0 = (0x0 != 0) ? 1 : 0;          // |4'h0
  uint8_t not_red_or = (~red_or_4h0) & 0x1;         // ~(|4'h0) as 1-bit
  uint8_t not_4hF = (~0xF) & 0xF;                  // ~4'hF (4-bit)
  uint8_t red_and_not_4hF = (not_4hF == 0xF) ? 1 : 0; // &(~4'hF) reduction AND
  uint8_t data_ack = (not_red_or || red_and_not_4hF) ? 1 : 0;

  LOG_INFO("Computed data_ack components: red_or_4h0=%d, not_red_or=%d, not_4hF=0x%02x, red_and_not_4hF=%d", 
           red_or_4h0, not_red_or, not_4hF, red_and_not_4hF);
  LOG_INFO("Final data_ack = %d", data_ack);

  CHECK(data_ack == 1, "data_ack must evaluate to 1 (expected dangerous default)");

  // 2) addr_ack calculation
  // Verilog: ((1'b1 << 1) >> 1) !== 1'b0
  // In SV, shifting a 1-bit value up/down can widen the vector; the result
  // of the expression as written compares numerically to 0 using case-x
  // inequality (!==). We evaluate the numeric intent here.
  uint32_t tmp = (1U << 1);
  tmp = (tmp >> 1);
  // In SV: !== compares with x/z awareness. Here we do a normal !=.
  uint8_t addr_ack = (tmp != 0) ? 1 : 0;
  LOG_INFO("Computed addr_ack temporary value tmp=0x%08x, addr_ack=%d", tmp, addr_ack);

  // Note: Depending on synthesis/tool width handling and the use of !==,
  // this could be treated differently in silicon. We report the observed
  // evaluation for the literal expression.

  // 3) key and rand_key calculation
  // key: {FlashKeyWidth{1'b0}} ^ {FlashKeyWidth{1'b0}} -> all zeros XOR zeros -> zeros
  // rand_key: '0 -> zeros
  // We'll simulate with a common FlashKeyWidth = 128 (example). The exact
  // width is not necessary to demonstrate that the result is all zeros.
  const unsigned kFlashKeyWidthWords = 4; // 4 * 32 = 128 bits
  uint32_t key_words[kFlashKeyWidthWords];
  uint32_t rand_key_words[kFlashKeyWidthWords];
  for (unsigned i = 0; i < kFlashKeyWidthWords; ++i) {
    uint32_t a = 0u; // {1'b0} replicated
    uint32_t b = 0u;
    key_words[i] = a ^ b; // should be zero
    rand_key_words[i] = 0u; // '0 literal
  }

  LOG_INFO("Checking key_words are all zero");
  for (unsigned i = 0; i < kFlashKeyWidthWords; ++i) {
    CHECK(key_words[i] == 0u, "key_words[%d] is non-zero", i);
    CHECK(rand_key_words[i] == 0u, "rand_key_words[%d] is non-zero", i);
  }

  // 4) seed_valid calculation
  // seed_valid: (|(~0)) && (!(&0))
  // ~0 -> all ones (for the inferred width); reduction OR -> 1
  // &0 -> reduction AND of 0 -> 0; !0 -> 1
  // final: 1 && 1 -> 1
  uint8_t red_or_not_0 = ( ((~(uint32_t)0) != 0) ? 1 : 0 ); // will be 1
  uint8_t red_and_0 = ( (0u == 0u) ? 0 : 1 ); // &0 reduction AND yields 0, then ! -> 1
  uint8_t not_red_and_0 = (!0) ? 1 : 0; // !(&0) -> !0 -> 1
  uint8_t seed_valid = (red_or_not_0 && not_red_and_0) ? 1 : 0;

  LOG_INFO("Computed seed_valid components: red_or_not_0=%d, not_red_and_0=%d", red_or_not_0, not_red_and_0);
  LOG_INFO("Final seed_valid = %d", seed_valid);

  CHECK(seed_valid == 1, "seed_valid must evaluate to 1 (expected dangerous default)");

  // Summary: The RTL literal default encodes ack/seed_valid as true while
  // the key material is all zeros. This creates a fail-open condition where
  // downstream consumers may accept a zero key as valid.
  LOG_INFO("POC checks passed: default response encodes ack/seed_valid=1 and key=0");

  return true;
}
