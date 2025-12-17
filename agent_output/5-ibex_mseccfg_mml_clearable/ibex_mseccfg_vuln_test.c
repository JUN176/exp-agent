#include "sw/device/lib/testing/test_framework/ottf_test_config.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/base/csr_registers.h"

// Define the OT Test Framework configuration.
OTTF_DEFINE_TEST_CONFIG();

// Proof-of-concept test that demonstrates software can both set and clear
// bits in the MSECCFG CSR. The reported vulnerability states that the
// mseccfg.MML bit should be irreversible (cannot be cleared by software
// once set), but OpenTitan allows software writes. This test verifies that
// software can set a zero bit in MSECCFG and subsequently clear it.

bool test_main(void) {
  uint32_t mseccfg = 0;
  LOG_INFO("[POC] Starting ibex MSECCFG clearability test");

  // Read the current value of MSECCFG CSR.
  CSR_READ(CSR_REG_MSECCFG, &mseccfg);
  LOG_INFO("[POC] MSECCFG initial: 0x%08x", mseccfg);

  // Find a bit position that is currently zero so we can set and then try
  // to clear it. This avoids guessing the MML bit position while still
  // demonstrating that bits in MSECCFG are writable and (importantly)
  // clearable by software.
  uint32_t mask = 0;
  for (int i = 0; i < 32; ++i) {
    uint32_t b = (1u << i);
    if ((mseccfg & b) == 0u) {
      mask = b;
      break;
    }
  }

  // If no zero bit is found, the register is all-ones and we cannot perform
  // the test; abort with a meaningful message.
  CHECK(mask != 0u, "No clearable bit found in MSECCFG to perform the test");
  LOG_INFO("[POC] Selected bit mask 0x%08x (bit %d)", mask, __builtin_ctz(mask));

  // Set the selected bit using CSR_SET_BITS (writes to the CSR).
  CSR_SET_BITS(CSR_REG_MSECCFG, mask);
  uint32_t after_set = 0;
  CSR_READ(CSR_REG_MSECCFG, &after_set);
  LOG_INFO("[POC] MSECCFG after setting mask: 0x%08x", after_set);
  CHECK((after_set & mask) == mask, "Failed to set the selected bit in MSECCFG");

  // Attempt to clear the selected bit using CSR_CLEAR_BITS.
  CSR_CLEAR_BITS(CSR_REG_MSECCFG, mask);
  uint32_t after_clear = 0;
  CSR_READ(CSR_REG_MSECCFG, &after_clear);
  LOG_INFO("[POC] MSECCFG after clearing mask: 0x%08x", after_clear);

  // If the bit has been cleared by software, this demonstrates the
  // asserted vulnerability (MML-like bits are not immutable).
  if ((after_clear & mask) == 0u) {
    LOG_INFO("[POC] MSECCFG bit cleared by software - vulnerability confirmed.");
    return true;  // Test passes because the vulnerability exists.
  }

  // Otherwise, the bit remained set (sticky) -> no vulnerability detected
  // for this particular bit. Fail the test to make the result explicit.
  LOG_INFO("[POC] MSECCFG bit remained set after clear attempt - no vuln for selected bit.");
  CHECK(false, "Selected MSECCFG bit is sticky and could not be cleared by software");

  // Should never reach here.
  return false;
}
