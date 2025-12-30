#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/runtime/log.h"

// This POC uses low-level CSR access (CSR_READ/CSR_WRITE) to configure
// PMP registers directly. It avoids any DIF usage to keep full control over
// the register layout and values.

// Use PMP region 7 (upper byte of PMPCFG1). This follows the existing
// pmp_smoketest which typically uses region 7 for experiments.
#define PMP_REGION_ID 7

static volatile bool pmp_load_exception;

// Override the OTTF load/store exception handler to record if a load/store
// fault happened. The handler is called with an exception info pointer but
// we only need to set a flag for detection in the test.
void ottf_load_store_fault_handler(uint32_t *exc_info) {
  (void)exc_info;
  pmp_load_exception = true;
}

OTTF_DEFINE_TEST_CONFIG();

// Small aligned buffer that we will protect via PMP. We pick 4-byte size and
// align to 4 to allow using the NA4 (4-byte) PMP mode which has a simple
// PMPADDR encoding (addr >> 2).
__attribute__((aligned(4))) static volatile uint32_t pmp_protected_word = 0xA5A5A5A5u;

bool test_main(void) {
  LOG_INFO("ibex PMP MML bypass POC start");

  // Sanity: ensure no spurious fault before configuring PMP.
  pmp_load_exception = false;
  volatile uint32_t tmp = pmp_protected_word;  // should not fault before config
  (void)tmp;
  CHECK(!pmp_load_exception, "Unexpected load fault before PMP configuration");

  // Compute the PMPADDR value for NA4 mode: PMPADDR = addr >> 2.
  uintptr_t target_addr = (uintptr_t)&pmp_protected_word;
  uint32_t pmpaddr_val = (uint32_t)(target_addr >> 2);

  LOG_INFO("Configuring PMPADDR7 = 0x%08x (target addr 0x%08x)", pmpaddr_val,
           (uint32_t)target_addr);

  // Write PMPADDR7 directly.
  CSR_WRITE(CSR_REG_PMPADDR7, pmpaddr_val);

  // Prepare PMPCFG1 value to set region 7's cfg byte to: L=1, A=NA4 (A=2), R/W/X=0.
  // pmpcfg byte layout (per RISC-V privileged spec):
  // [7] L, [6:5] reserved, [4:3] A (00=OFF, 01=TOR, 10=NA4, 11=NAPOT), [2] X, [1] W, [0] R
  // For NA4, A = 2 -> bits [4:3] = 10 (0x10). L = 1 -> 0x80. Permissions R/W/X cleared.
  // Combined byte value = 0x80 | 0x10 = 0x90.
  const uint32_t region7_cfg_byte = 0x90u;

  // Read-modify-write PMPCFG1 (covers regions 4..7 in bytes [7:0] LSB->region4).
  uint32_t pmpcfg1_val = 0;
  CSR_READ(CSR_REG_PMPCFG1, &pmpcfg1_val);
  LOG_INFO("PMPCFG1 before = 0x%08x", pmpcfg1_val);

  // Region 7 is the MSByte of PMPCFG1 (byte index 3). Clear that byte and set new cfg.
  uint32_t mask = 0xFFu << 24;
  pmpcfg1_val = (pmpcfg1_val & ~mask) | (region7_cfg_byte << 24);
  CSR_WRITE(CSR_REG_PMPCFG1, pmpcfg1_val);

  // Optionally clear MSECCFG to set MML = 0. Writing zero clears all security mode
  // controls; this makes sure the condition described by the report (MML == 0)
  // is present. The CSR bit layout is implementation-defined; clearing the whole
  // register is sufficient to force MML==0 on implementations which follow the
  // vulnerability report's assumptions.
  CSR_WRITE(CSR_REG_MSECCFG, 0u);

  // Read back and log to help debugging.
  uint32_t pmpcfg1_after = 0;
  CSR_READ(CSR_REG_PMPCFG1, &pmpcfg1_after);
  LOG_INFO("PMPCFG1 after  = 0x%08x", pmpcfg1_after);

  uint32_t mseccfg_val = 0;
  CSR_READ(CSR_REG_MSECCFG, &mseccfg_val);
  LOG_INFO("MSECCFG        = 0x%08x", mseccfg_val);

  // Now attempt to read the protected word. According to the RISC-V PMP and
  // SMEPMP semantics, because region 7 is locked (L=1) and permissions are
  // cleared, any access in M-mode should trap with a load access fault. The
  // vulnerability report states that when MML == 0, M-mode is incorrectly
  // allowed to access PMP-protected regions. We therefore expect a load fault
  // here; if none occurs, the device is vulnerable.
  pmp_load_exception = false;
  volatile uint32_t observed = pmp_protected_word;  // This access SHOULD trap
  (void)observed;

  CHECK(pmp_load_exception,
        "PMP did NOT raise a load access fault in M-mode with pmpcfg.L=1 and MML=0");

  LOG_INFO("PMP correctly trapped the M-mode access (unexpected for vulnerable HW)");

  return true;
}
