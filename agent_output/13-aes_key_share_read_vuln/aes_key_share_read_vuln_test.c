#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "aes_regs.h"

OTTF_DEFINE_TEST_CONFIG();

// This test demonstrates that the AES KEY_SHARE0 registers, which are intended
// to be write-only, can be read back by software due to a faulty read-data
// multiplexer in the register file (see HW RTL aes_reg_top.sv).

bool test_main(void) {
  // Create an MMIO region for the AES peripheral using the autogen base addr.
  mmio_region_t aes_base = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Distinct test pattern words for the 8 key share words.
  const uint32_t test_share0[8] = {
      0xA5A5A5A5u,
      0x5A5A5A5Au,
      0xDEADBEEFu,
      0xFEEDC0DEu,
      0x01234567u,
      0x89ABCDEFu,
      0x0F1E2D3Cu,
      0xC3D2E1F0u,
  };

  LOG_INFO("AES KEY_SHARE0 readback vulnerability POC starting");

  // For each KEY_SHARE0 word: write a known pattern then read it back.
  for (int i = 0; i < 8; ++i) {
    ptrdiff_t offset = 0;
    switch (i) {
      case 0: offset = AES_KEY_SHARE0_0_OFFSET; break;
      case 1: offset = AES_KEY_SHARE0_1_OFFSET; break;
      case 2: offset = AES_KEY_SHARE0_2_OFFSET; break;
      case 3: offset = AES_KEY_SHARE0_3_OFFSET; break;
      case 4: offset = AES_KEY_SHARE0_4_OFFSET; break;
      case 5: offset = AES_KEY_SHARE0_5_OFFSET; break;
      case 6: offset = AES_KEY_SHARE0_6_OFFSET; break;
      case 7: offset = AES_KEY_SHARE0_7_OFFSET; break;
      default: offset = 0; break;
    }

    uint32_t w = test_share0[i];

    // Write the key share word. In normal operation software writes these
    // registers to provide the masked key share to AES. They are intended to
    // be write-only to avoid leaking the secret share back to software.
    mmio_region_write32(aes_base, offset, w);

    // Read back the same register. Due to the RTL bug the read-data mux routes
    // the stored key share value to the read bus, allowing software to read
    // back the secret material it should not be able to observe.
    uint32_t r = mmio_region_read32(aes_base, offset);

    LOG_INFO("KEY_SHARE0[%d] write=0x%08x read=0x%08x", i, w, r);

    // If the read returns the written value, the register is readable and the
    // vulnerability is present. Use CHECK to fail the test if we don't observe
    // the leak (i.e., read != written).
    CHECK(r == w, "AES KEY_SHARE0[%d] did not read back expected value", i);
  }

  LOG_INFO("AES KEY_SHARE0 readback observed on all words - vulnerability confirmed");

  return true;
}
