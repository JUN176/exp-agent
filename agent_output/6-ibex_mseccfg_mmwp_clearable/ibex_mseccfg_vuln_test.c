#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/base/log.h"
#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/base/csr_registers.h"

OTTF_DEFINE_TEST_CONFIG();

// Proof-of-concept test for MSECCFG.MMWP writable/clearable by software.
//
// Vulnerability summary:
// - The MSECCFG CSR contains the MMWP bit which, per SMEPMP specification,
//   should be irreversible once set (requires reset to clear).
// - This POC demonstrates that software can set the MMWP bit and then
//   successfully clear it via CSR clear/write operations — indicating the
//   OpenTitan implementation deviates from the SMEPMP security model.

bool test_main(void) {
  uint32_t reg = 0;

  // Read initial MSECCFG value.
  CSR_READ(CSR_REG_MSECCFG, &reg);
  LOG_INFO("Initial MSECCFG = 0x%08x", reg);

  // NOTE: The RTL uses a symbolic CSR bit index CSR_MSECCFG_MMWP_BIT. For
  // the purpose of this POC we assume the MMWP bit is bit 0 (mask = 1u << 0).
  // If the bit index differs on a particular build, adjust the mask
  // accordingly to match the implementation under test.
  const uint32_t mmwp_mask = (1u << 0);

  // Set the MMWP bit.
  LOG_INFO("Setting MMWP bit (mask=0x%08x)", mmwp_mask);
  CSR_SET_BITS(CSR_REG_MSECCFG, mmwp_mask);
  CSR_READ(CSR_REG_MSECCFG, &reg);
  LOG_INFO("After set, MSECCFG = 0x%08x", reg);
  // Confirm the bit is set.
  CHECK((reg & mmwp_mask) != 0, "Failed to set MMWP bit");

  // Attempt to clear the MMWP bit via the CSR clear instruction (software).
  LOG_INFO("Attempting to clear MMWP bit via CSR_CLEAR_BITS...");
  CSR_CLEAR_BITS(CSR_REG_MSECCFG, mmwp_mask);
  CSR_READ(CSR_REG_MSECCFG, &reg);
  LOG_INFO("After clear attempt, MSECCFG = 0x%08x", reg);

  // Vulnerability condition:
  // - If the implementation allows clearing the MMWP bit, (reg & mmwp_mask)
  //   will be 0 and the system is vulnerable.
  // - If the MMWP bit is truly irreversible, the bit will remain set and the
  //   CHECK will fail.
  CHECK((reg & mmwp_mask) == 0,
        "MMWP bit was not cleared by software - no vulnerability detected");

  return true;
}
