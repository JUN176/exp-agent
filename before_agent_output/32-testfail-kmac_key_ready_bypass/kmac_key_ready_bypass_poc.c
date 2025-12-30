#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/bitfield.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "kmac_regs.h"

// Timeout for polling loops (in arbitrary iterations).
#define TIMEOUT 1000000

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void)
{
  LOG_INFO("KMAC key-ready bypass POC (vuln 32) start");

  mmio_region_t kmac_base = mmio_region_from_addr(TOP_EARLGREY_KMAC_BASE_ADDR);

  // 1) Sanity: ensure KMAC reports idle.
  uint32_t status = mmio_region_read32(kmac_base, KMAC_STATUS_REG_OFFSET);
  bool idle = bitfield_bit32_read(status, KMAC_STATUS_SHA3_IDLE_BIT);
  LOG_INFO("Initial KMAC STATUS = 0x%08x (idle=%d)", status, idle);

  CHECK(idle, "KMAC must be idle to run this POC");

  // 2) Configure KMAC mode/strength via shadowed CFG register.
  // We set the mode to SHA3 (基础模式) + 启用KMAC_EN，选择128位强度
  uint32_t cfg_reg = mmio_region_read32(kmac_base, KMAC_CFG_SHADOWED_REG_OFFSET);

  // 核心修正1：启用KMAC功能（KMAC_EN_BIT置1）
  cfg_reg = bitfield_bit32_write(cfg_reg, KMAC_CFG_SHADOWED_KMAC_EN_BIT, 1);
  // 核心修正2：设置K强度为128（使用正确的常量）
  cfg_reg = bitfield_field32_write(cfg_reg, KMAC_CFG_SHADOWED_KSTRENGTH_FIELD,
                                   KMAC_CFG_SHADOWED_KSTRENGTH_VALUE_L128);
  // 核心修正3：设置基础模式为SHA3（KMAC依赖SHA3基础模式）
  cfg_reg = bitfield_field32_write(cfg_reg, KMAC_CFG_SHADOWED_MODE_FIELD,
                                   KMAC_CFG_SHADOWED_MODE_VALUE_SHA3);

  // Note: Do NOT program any key shares. The vulnerability claim is that the
  // hardware will proceed even when the key_valid input is not asserted.
  mmio_region_write32(kmac_base, KMAC_CFG_SHADOWED_REG_OFFSET, cfg_reg);
  // Write twice as the DIF does for shadowed registers.
  mmio_region_write32(kmac_base, KMAC_CFG_SHADOWED_REG_OFFSET, cfg_reg);

  LOG_INFO("Wrote KMAC CFG (KMAC_EN=1, mode=SHA3, kstrength=128). Not programming key shares.");

  // 3) Issue START command (this pulses the start_i input into the core).
  uint32_t cmd_reg = bitfield_field32_write(0u, KMAC_CMD_CMD_FIELD, KMAC_CMD_CMD_VALUE_START);
  mmio_region_write32(kmac_base, KMAC_CMD_REG_OFFSET, cmd_reg);
  LOG_INFO("Issued START command to KMAC (without provisioning a key)");

  // 4) Poll for the hardware entering the absorb state (which indicates the
  // datapath/key write were enabled and the core proceeded). If the core
  // instead raises a key-not-valid error we consider the hardware to be
  // protected.
  bool entered_absorb = false;
  for (int i = 0; i < TIMEOUT; ++i)
  {
    uint32_t st = mmio_region_read32(kmac_base, KMAC_STATUS_REG_OFFSET);
    if (bitfield_bit32_read(st, KMAC_STATUS_SHA3_ABSORB_BIT))
    {
      entered_absorb = true;
      break;
    }
    // Check for KMAC error (e.g. key not valid). If set, stop early.
    uint32_t intr = mmio_region_read32(kmac_base, KMAC_INTR_STATE_REG_OFFSET);
    if (bitfield_bit32_read(intr, KMAC_INTR_STATE_KMAC_ERR_BIT))
    {
      LOG_INFO("KMAC reported error while starting (intr=0x%08x)", intr);
      break;
    }
  }

  if (entered_absorb)
  {
    LOG_INFO("VULNERABLE: KMAC entered ABSORB without a programmed/validated key.");
    // Success: we can proceed to use the datapath under potentially stale key.
    return true;
  }

  // If we didn't enter absorb, fail the test to indicate the hardware rejects
  // start when key is not ready.
  CHECK(false, "KMAC did not enter absorb without a valid key (no vuln detected)");
  return false;
}