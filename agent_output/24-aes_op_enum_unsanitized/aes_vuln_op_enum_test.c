#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
// NOTE: The generated register header is provided by the build dependency
// "//hw/top:aes_c_regs" and is expected to define register offsets for the
// AES peripheral (e.g. AES_CTRL_*). If that header provides a specific
// offset macro for the operation field, the test can be adapted to use it.
// As a conservative fallback we use offset 0x0 for the control register.

OTTF_DEFINE_TEST_CONFIG();

// Fallback offset for the AES operation/control register. Replace with the
// generated offset from aes_regs.h if available (e.g. AES_CTRL_REG_OFFSET).
#define AES_CTRL_OFFSET 0x0

bool test_main(void) {
  LOG_INFO("AES unsanitized enum POC: starting");

  // Compose the absolute address of the AES control register.
  const uintptr_t aes_ctrl_addr = TOP_EARLGREY_AES_BASE_ADDR + AES_CTRL_OFFSET;

  // Choose an out-of-range / invalid enumeration value for the operation
  // field. Valid operation values are typically small (e.g. 1=Encrypt, 2=Decrypt).
  // We deliberately write a large value to demonstrate lack of input
  // validation/sanitization at the MMIO level.
  const uint32_t invalid_op_value = 0xDEADBEEF;

  LOG_INFO("Writing invalid op value 0x%08x to AES control register at 0x%08x",
           invalid_op_value, (uint32_t)aes_ctrl_addr);

  // Perform the raw MMIO write using the base library (abs_mmio) to ensure
  // we bypass any high-level checks that a DIF might implement.
  abs_mmio_write32((uint32_t)aes_ctrl_addr, invalid_op_value);

  // Read back the register to see whether the value was accepted by the
  // hardware register file. If the design had sanitized the input (cap the
  // enum to valid values), the read-back would differ from the written value.
  uint32_t read_back = abs_mmio_read32((uint32_t)aes_ctrl_addr);

  LOG_INFO("Read back AES control register value: 0x%08x", read_back);

  // If the hardware accepts the invalid enumeration value unchanged, this
  // demonstrates that the control signal can be driven to an invalid value
  // that may then be fanned out to submodules (as shown in the RTL).
  CHECK(read_back == invalid_op_value,
        "AES control register accepted invalid enum value - potential op_i propagation without validation");

  LOG_INFO("Test PASSED: invalid op was accepted (indicates lack of sanitization)");

  return true;
}
