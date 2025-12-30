#include "sw/device/lib/arch/device.h"
#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_base.h"
#include "sw/device/lib/dif/dif_pinmux.h"
#include "sw/device/lib/dif/dif_rv_plic.h"
#include "sw/device/lib/dif/dif_spi_device.h"
#include "sw/device/lib/runtime/hart.h"
#include "sw/device/lib/runtime/irq.h"
#include "sw/device/lib/runtime/log.h"
#include "sw/device/lib/testing/spi_device_testutils.h"
#include "sw/device/lib/testing/test_framework/check.h"
#include "sw/device/lib/testing/test_framework/ottf_main.h"

#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

OTTF_DEFINE_TEST_CONFIG();

// This test coordinates with the simulation testbench to demonstrate the
// locality TOCTOU vulnerability in hw/ip/spi_device/rtl/spi_tpm.sv.
//
// High-level plan (see README for details):
// 1) The test instructs the TB (via LOG_INFO sync markers) to send a TPM
//    WRITE transaction whose addr[15:12] encodes a VALID locality (e.g. 0)
//    so the DUT latches locality to a valid value.
// 2) Immediately after (without gaps), the TB sends a TPM READ transaction
//    where addr[15:12] encodes an INVALID locality (e.g. >= NumLocality).
//    Because of the non-blocking assignments in the RTL, invalid_locality
//    is computed from the PRIOR cycle's locality (stale) and will still be
//    0 (valid). The HW return/read path will therefore allow a HW-managed
//    TPM register value to be returned instead of the expected 0xFF value.
//
// In this test we perform the device-side software handling and observe
// the sequence of commands/addresses the DUT sees. The actual confirmation
// that the host observed a leaked HW value is performed by the simulation
// testbench. This test logs the observed addresses and asserts the command
// sequence to ensure the TB followed the exploit waveform.

static dif_spi_device_handle_t spi_device;
static dif_pinmux_t pinmux;
static dif_rv_plic_t plic;

static volatile bool header_interrupt_received = false;

void ottf_external_isr(uint32_t *exc_info) {
  // Minimal ISR copied from existing TPM tests. We only care about the
  // TPM header-not-empty interrupt which signals that a command header
  // (write or read) has arrived from the host.
  #define EXPECTED_IRQ kTopEarlgreyPlicIrqIdSpiDeviceTpmHeaderNotEmpty

  plic_isr_ctx_t plic_ctx = {.rv_plic = &plic,
                             .hart_id = kTopEarlgreyPlicTargetIbex0};

  spi_device_isr_ctx_t spi_device_ctx = {
      .spi_device = &spi_device.dev,
      .plic_spi_device_start_irq_id =
          kTopEarlgreyPlicIrqIdSpiDeviceUploadCmdfifoNotEmpty,
      .expected_irq = kDifSpiDeviceIrqTpmHeaderNotEmpty,
      .is_only_irq = false};

  top_earlgrey_plic_peripheral_t peripheral;
  dif_spi_device_irq_t spi_device_irq;

  isr_testutils_spi_device_isr(plic_ctx, spi_device_ctx, false, &peripheral,
                               &spi_device_irq);

  if (spi_device_irq == kDifSpiDeviceIrqTpmHeaderNotEmpty) {
    header_interrupt_received = true;
    // Disable until handled
    CHECK_DIF_OK(dif_spi_device_irq_set_enabled(&spi_device.dev,
        kDifSpiDeviceIrqTpmHeaderNotEmpty, kDifToggleDisabled));
  }
}

static void ack_spi_tpm_header_irq(dif_spi_device_handle_t *spi_device_h) {
  header_interrupt_received = false;
  CHECK_DIF_OK(dif_spi_device_irq_acknowledge(
      &spi_device_h->dev, kDifSpiDeviceIrqTpmHeaderNotEmpty));
  CHECK_DIF_OK(dif_spi_device_irq_set_enabled(
      &spi_device_h->dev, kDifSpiDeviceIrqTpmHeaderNotEmpty, kDifToggleEnabled));
}

bool test_main(void) {
  // Initialize pinmux, spi device and plic using DIFs (allowed for init).
  CHECK_DIF_OK(dif_pinmux_init(mmio_region_from_addr(TOP_EARLGREY_PINMUX_AON_BASE_ADDR), &pinmux));
  CHECK_DIF_OK(dif_spi_device_init_handle(mmio_region_from_addr(TOP_EARLGREY_SPI_DEVICE_BASE_ADDR), &spi_device));
  CHECK_DIF_OK(dif_rv_plic_init(mmio_region_from_addr(TOP_EARLGREY_RV_PLIC_BASE_ADDR), &plic));

  // Configure TPM interface: ensure locality checking and return-by-hardware
  // are ENABLED so the vulnerable RTL path is exercised.
  const dif_spi_device_tpm_config_t kTpmConfig = {
      .interface = kDifSpiDeviceTpmInterfaceCrb,
      .disable_return_by_hardware = false,
      .disable_address_prefix_check = false,
      .disable_locality_check = false};

  CHECK_DIF_OK(dif_spi_device_tpm_configure(&spi_device, kDifToggleEnabled, kTpmConfig));

  // Enable the TPM header interrupt used to detect incoming commands.
  CHECK_DIF_OK(dif_spi_device_irq_set_enabled(&spi_device.dev, kDifSpiDeviceIrqTpmHeaderNotEmpty, kDifToggleEnabled));

  // Sync with TB so it can start the crafted host waveform.
  LOG_INFO("SYNC: Begin TPM locality TOCTOU exploit test");
  LOG_INFO("SYNC: TB must send two back-to-back TPM transactions:\n"
           "  1) WRITE with addr[15:12] = VALID_LOCALITY (e.g. 0)\n"
           "  2) immediately after, READ with addr[15:12] = INVALID_LOCALITY (e.g. 7)");

  // Wait for first (WRITE) header from host
  ATOMIC_WAIT_FOR_INTERRUPT(header_interrupt_received);
  uint8_t write_cmd = 0;
  uint32_t write_addr = 0;
  CHECK_DIF_OK(dif_spi_device_tpm_get_command(&spi_device, &write_cmd, &write_addr));
  LOG_INFO("Observed first command: 0x%x addr=0x%08x", write_cmd, write_addr);

  // Read the written data (software path for write). Block until data available.
  uint8_t buf[64] = {0};
  dif_result_t r = kDifOutOfRange;
  uint32_t num_bytes = (write_cmd & 0x3f) + 1;
  while (r == kDifOutOfRange) {
    r = dif_spi_device_tpm_read_data(&spi_device, num_bytes, buf);
  }
  CHECK_DIF_OK(r);
  CHECK_DIF_OK(dif_spi_device_tpm_free_write_fifo(&spi_device));
  ack_spi_tpm_header_irq(&spi_device);

  // Instruct TB to immediately perform the READ with an INVALID locality.
  LOG_INFO("SYNC: Now TB should send the immediate READ with INVALID locality");

  // Wait for second (READ) header from host
  ATOMIC_WAIT_FOR_INTERRUPT(header_interrupt_received);
  uint8_t read_cmd = 0;
  uint32_t read_addr = 0;
  CHECK_DIF_OK(dif_spi_device_tpm_get_command(&spi_device, &read_cmd, &read_addr));
  LOG_INFO("Observed second command: 0x%x addr=0x%08x", read_cmd, read_addr);

  // The simulation TB is expected to capture the host-observed return bytes and
  // assert a failure if the HW returned a non-0xFF value in violation of the
  // locality policy. Here we simply validate we observed a READ and that the
  // locality nibble changed between the transactions.
  CHECK((read_cmd & 0x80) == 0x80, "Second command is not a READ as expected");

  uint32_t write_locality = (write_addr >> 12) & 0xF;
  uint32_t read_locality = (read_addr >> 12) & 0xF;
  LOG_INFO("Write locality = %u, Read locality = %u", write_locality, read_locality);
  CHECK(write_locality != read_locality, "Locality nibble did not change between transactions");

  // Tell TB we are done; TB will evaluate whether the HW returned data
  // incorrectly (i.e., returned real TPM status instead of 0xFF for the
  // invalid locality). If the TB observed leakage it should fail the test in
  // simulation; otherwise this test will pass.
  LOG_INFO("SYNC: Done - TB should report whether leakage occurred");

  return true;
}
