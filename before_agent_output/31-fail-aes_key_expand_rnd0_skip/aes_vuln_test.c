#include "sw/device/lib/base/bitfield.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/base/memory.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "aes_regs.h"
#include "sw/device/tests/crypto/aes_testvectors.h"

// Simple POC to demonstrate AES-256 key-schedule skipping the first
// scheduling step when rnd == 0 in the hardware implementation. The
// test configures the AES peripheral in CTR mode with a 256-bit key,
// provides the known test vector key/IV and a single plaintext block,
// then compares the hardware ciphertext against the expected software
// test-vector. A mismatch indicates the key schedule is incorrect.

OTTF_DEFINE_TEST_CONFIG();

bool test_main(void) {
  // Create MMIO region for AES peripheral.
  mmio_region_t aes = mmio_region_from_addr(TOP_EARLGREY_AES_BASE_ADDR);

  // Wait for AES to be idle.
  int wait = 1000000;
  while (!mmio_region_get_bit32(aes, AES_STATUS_REG_OFFSET,
                                AES_STATUS_IDLE_BIT) && --wait) {
  }
  CHECK(wait > 0, "AES did not become idle");
  LOG_INFO("AES is idle, configuring peripheral...");

  // Construct control register value using the same field helpers as DIF.
  // Operation: Encrypt (value 1)
  // Mode: CTR (value 16 as used by DIF enum kDifAesModeCtr)
  // Key length: 256-bit (value 4 as used by DIF enum kDifAesKey256)
  uint32_t ctrl_reg =
      bitfield_field32_write(0, AES_CTRL_SHADOWED_OPERATION_FIELD, 1);
  ctrl_reg = bitfield_field32_write(ctrl_reg, AES_CTRL_SHADOWED_MODE_FIELD,
                                    16);
  ctrl_reg = bitfield_field32_write(ctrl_reg, AES_CTRL_SHADOWED_KEY_LEN_FIELD,
                                    4);
  // Keep PRNG reseed rate = 0 and manual operation = 0 (automatic)
  ctrl_reg = bitfield_field32_write(
      ctrl_reg, AES_CTRL_SHADOWED_PRNG_RESEED_RATE_FIELD, 0);
  ctrl_reg = bitfield_bit32_write(ctrl_reg,
                                  AES_CTRL_SHADOWED_MANUAL_OPERATION_BIT,
                                  false);
  // Use software-provided key (side load = 0)
  ctrl_reg = bitfield_bit32_write(ctrl_reg, AES_CTRL_SHADOWED_SIDELOAD_BIT,
                                  false);

  // Shadowed control register requires a double-write in hardware.
  mmio_region_write32(aes, AES_CTRL_SHADOWED_REG_OFFSET, ctrl_reg);
  mmio_region_write32(aes, AES_CTRL_SHADOWED_REG_OFFSET, ctrl_reg);

  LOG_INFO("Control register configured (0x%08x)", ctrl_reg);

  // Provide AES key shares. We use share0 = test key, share1 = 0 (no masking).
  for (int i = 0; i < AES_KEY_SHARE0_MULTIREG_COUNT; ++i) {
    // Offsets are in bytes; each subregister is 32 bits.
    ptrdiff_t off = AES_KEY_SHARE0_0_REG_OFFSET + i * sizeof(uint32_t);
    uint32_t w = (i < 8) ? kKey256[i] : 0;
    mmio_region_write32(aes, off, w);
  }
  for (int i = 0; i < AES_KEY_SHARE1_MULTIREG_COUNT; ++i) {
    ptrdiff_t off = AES_KEY_SHARE1_0_REG_OFFSET + i * sizeof(uint32_t);
    mmio_region_write32(aes, off, 0);
  }
  LOG_INFO("Key shares written (share0 = kKey256, share1 = zeros)");

  // Provide IV (CTR mode requires IV). Use the test vector IV kIv.
  for (int i = 0; i < AES_IV_MULTIREG_COUNT; ++i) {
    ptrdiff_t off = AES_IV_0_REG_OFFSET + i * sizeof(uint32_t);
    uint32_t v = (i < 4) ? kIv[i] : 0;
    mmio_region_write32(aes, off, v);
  }
  LOG_INFO("IV written");

  // Write a single plaintext block (first block of kPlaintext)
  for (int i = 0; i < AES_DATA_IN_MULTIREG_COUNT; ++i) {
    ptrdiff_t off = AES_DATA_IN_0_REG_OFFSET + i * sizeof(uint32_t);
    uint32_t p = (i < 4) ? kPlaintext[i] : 0;
    mmio_region_write32(aes, off, p);
  }
  LOG_INFO("Plaintext block written, waiting for output...");

  // Wait for OUTPUT_VALID status.
  wait = 1000000;
  while (!mmio_region_get_bit32(aes, AES_STATUS_REG_OFFSET,
                                AES_STATUS_OUTPUT_VALID_BIT) && --wait) {
  }
  CHECK(wait > 0, "AES did not produce output in time");

  // Read ciphertext block from hardware
  uint32_t hw_out[4];
  for (int i = 0; i < AES_DATA_OUT_MULTIREG_COUNT; ++i) {
    ptrdiff_t off = AES_DATA_OUT_0_REG_OFFSET + i * sizeof(uint32_t);
    hw_out[i] = mmio_region_read32(aes, off);
  }

  LOG_INFO("Hardware ciphertext: 0x%08x 0x%08x 0x%08x 0x%08x", hw_out[0],
           hw_out[1], hw_out[2], hw_out[3]);

  // Expected ciphertext from the test vector for CTR, 256-bit key.
  uint32_t expected[4] = {kCtr256Iso9797M2[0], kCtr256Iso9797M2[1],
                          kCtr256Iso9797M2[2], kCtr256Iso9797M2[3]};

  LOG_INFO("Expected ciphertext: 0x%08x 0x%08x 0x%08x 0x%08x", expected[0],
           expected[1], expected[2], expected[3]);

  // If the hardware is vulnerable, the ciphertext will differ from the
  // expected reference: the AES-256 key schedule bug causes incorrect
  // subkeys to be used and hence a wrong ciphertext.
  bool mismatch = (memcmp(hw_out, expected, sizeof(hw_out)) != 0);

  CHECK(mismatch, "AES hardware output matches expected value: vulnerability not observed");

  LOG_INFO("AES-256 key-schedule bug observed: hardware ciphertext differs from expected reference.");

  return true;
}
