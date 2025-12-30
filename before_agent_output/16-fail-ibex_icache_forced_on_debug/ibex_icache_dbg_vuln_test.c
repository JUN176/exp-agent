#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/base/csr.h"
#include "sw/device/lib/base/csr_registers.h"
#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/memory.h"

// NOTE: The RTL file specified in the report could not be read by the agent.
// This POC is therefore written against the published CSR interfaces and the
// vulnerability description. Some field bit positions (below) are assumptions
// based on common Ibex/top-level encodings. If these masks do not match the
// DUT, adjust them to the correct bit positions from the RTL.

// Assumed mask for the CPUCTRL CSR icache enable bit. Adjust if needed.
#define CPUCTRL_ICACHE_ENABLE_MASK (1u << 0)

// Assumed mask for the DCSR bit that indicates the core is in debug mode.
// Many RISC-V implementations use the Debug Mode/Debug cause bits; if the
// target uses a different mechanism to indicate debug mode, adapt accordingly.
#define DCSR_DEBUG_MASK (1u << 0)

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  uint32_t val = 0;

  LOG_INFO("ibex icache debug-mode forcing POC start");

  // 1) Read current CPUCTRL CSR value and log it.
  CSR_READ(CSR_REG_CPUCTRL, &val);
  LOG_INFO("Initial CPUCTRL = 0x%08x", val);

  // 2) Try to disable the icache via the software-controllable bit.
  // This uses CSR_CLEAR_BITS which compiles to a csrc instruction.
  LOG_INFO("Clearing icache enable bit (mask 0x%08x)", CPUCTRL_ICACHE_ENABLE_MASK);
  CSR_CLEAR_BITS(CSR_REG_CPUCTRL, CPUCTRL_ICACHE_ENABLE_MASK);

  // Give the write a moment (barrier) then read back.
  uint32_t cpuctrl_after_clear = 0;
  CSR_READ(CSR_REG_CPUCTRL, &cpuctrl_after_clear);
  LOG_INFO("CPUCTRL after clearing icache bit = 0x%08x", cpuctrl_after_clear);

  // If clearing failed immediately, record it (POC still continues).
  if (cpuctrl_after_clear & CPUCTRL_ICACHE_ENABLE_MASK) {
    LOG_INFO("Warning: icache enable bit still set after software clear. Continuing POC");
  } else {
    LOG_INFO("icache appears disabled by software (good). Now entering debug mode to test forcing behavior.");
  }

  // 3) Enter debug mode. The method of entering debug mode from software on the
  // same hart varies across platforms. The vulnerability description states the
  // hardware forces the icache on "whenever the processor is in debug mode".
  // Many implementations expose DCSR (CSR_REG_DCSR) where writing a particular
  // bit (or the debugger externally) will put the core into debug state. For
  // the purposes of this POC we set a presumed debug-indicating bit in DCSR.
  // NOTE: On real hardware this may not cause the hart to halt immediately when
  // set from software; in many designs debug mode is entered from an external
  // debugger. If that is the case, adapt the test to use an external debug
  // agent or simulation waveform.
  LOG_INFO("Setting DCSR debug bit (mask 0x%08x) to simulate debugger entering debug mode", DCSR_DEBUG_MASK);
  CSR_SET_BITS(CSR_REG_DCSR, DCSR_DEBUG_MASK);

  // 4) Read back CPUCTRL and check whether the icache enable bit has been
  // forced on by the hardware despite our software clearing it above.
  uint32_t cpuctrl_in_debug = 0;
  CSR_READ(CSR_REG_CPUCTRL, &cpuctrl_in_debug);
  LOG_INFO("CPUCTRL while in debug (after setting DCSR) = 0x%08x", cpuctrl_in_debug);

  // According to the reported vulnerability, the icache bit should be forced to 1
  // when the processor is in debug mode. We assert that behavior here: the test
  // passes if the bit is set while in debug mode even if we cleared it earlier.
  if (cpuctrl_in_debug & CPUCTRL_ICACHE_ENABLE_MASK) {
    LOG_INFO("VULN TRIGGERED: icache enable bit is set while in debug mode (forced on)");
  } else {
    LOG_INFO("No forced icache observed: icache enable bit remains clear while in debug mode");
    // Fail the test to indicate the DUT does not exhibit the documented behavior
    CHECK(false, "icache was not forced on in debug mode");
  }

  // 5) Cleanup: clear the DCSR debug bit and restore original CPUCTRL state if needed.
  CSR_CLEAR_BITS(CSR_REG_DCSR, DCSR_DEBUG_MASK);

  // Optionally restore the icache enable state to the original value by writing
  // the original bits back. For safety we just log the final state.
  uint32_t cpuctrl_final = 0;
  CSR_READ(CSR_REG_CPUCTRL, &cpuctrl_final);
  LOG_INFO("Final CPUCTRL = 0x%08x", cpuctrl_final);

  LOG_INFO("ibex icache debug-mode forcing POC finished");
  return true;
}
