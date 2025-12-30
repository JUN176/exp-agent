#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/base/csr_registers.h"
#include "sw/device/lib/runtime/log.h"

#include <stdint.h>

OTTF_DEFINE_TEST_CONFIG();

/*
 * PoC test for vulnerability: illegal_umode_insn tied to constant 0 in ibex ID
 * stage. The test constructs a small user-mode code sequence containing a
 * privileged instruction (mret) and then uses M-mode to set mepc to that
 * address, set mstatus.MPP to User, and execute an mret to enter U-mode.
 * In a correct implementation, attempting to execute mret in U-mode raises
 * an illegal instruction exception (mcause == 2). If the extra "illegal
 * in user mode" signal is permanently disabled, this redundant check would
 * not catch unexpected future privileged instructions; the test detects if
 * the illegal-in-U-mode trap does NOT occur.
 */

/* Keep the instruction buffer in writable memory so we can set MEPC to it. */
volatile uint32_t user_code[] __attribute__((aligned(4))) = {
    /* mret encoding */ 0x30200073u,
    /* ebreak (safe rendezvous if mret somehow succeeds) */ 0x00100073u,
};

bool test_main(void) {
  uint32_t orig_mstatus = 0;
  uint32_t mstatus = 0;
  uint32_t cause = 0;

  LOG_INFO("ibex illegal_umode_insn POC start");

  /* Save original mstatus for best-effort restore (informational). */
  CSR_READ(CSR_REG_MSTATUS, &orig_mstatus);

  /*
   * Prepare mstatus with MPP=User (00). MPP is bits [12:11] in mstatus.
   * Clear those bits to set MPP=0 (User). We do not set MPIE/FS etc.
   */
  CSR_READ(CSR_REG_MSTATUS, &mstatus);
  mstatus &= ~(0x3u << 11);  // Clear MPP[12:11]

  /* Set MEPC to the address of our user_code buffer. */
  uintptr_t entry = (uintptr_t)&user_code[0];
  LOG_INFO("Setting MEPC to user code at 0x%08x", (unsigned)entry);
  CSR_WRITE(CSR_REG_MEPC, (uint32_t)entry);

  /* Write the modified mstatus (with MPP=U) back. */
  CSR_WRITE(CSR_REG_MSTATUS, mstatus);

  LOG_INFO("Performing M-mode mret to transfer control to U-mode (user_code)");

  /* Execute mret: this will jump to user_code in U-mode and attempt to run mret. */
  asm volatile("mret");

  /*
   * If execution reaches here, one of two things happened:
   * 1) The core trapped while in U-mode executing the privileged instruction
   *    and the trap handler returned execution to this test harness in M-mode.
   * 2) The privileged instruction executed in U-mode without trapping (vulnerable)
   *    and control somehow returned here. Either way, check mcause.
   */
  CSR_READ(CSR_REG_MCAUSE, &cause);

  LOG_INFO("Returned to M-mode, mcause = %u", cause);

  /* RISC-V illegal instruction exception code is 2. */
  if (cause == 2) {
    LOG_INFO("Illegal instruction trap occurred as expected (mcause=2)");
  } else {
    LOG_INFO("No illegal-in-U-mode trap detected! Potential vulnerability.");
  }

  /* Assert that the illegal instruction trap happened. If it didn't, fail. */
  CHECK(cause == 2, "Expected illegal instruction trap when executing privileged instruction in U-mode. Got mcause=%u", cause);

  /* Best-effort restore of mstatus (not strictly necessary for test teardown). */
  CSR_WRITE(CSR_REG_MSTATUS, orig_mstatus);

  return true;
}
