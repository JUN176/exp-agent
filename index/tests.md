# OpenTitan Device Tests - Comprehensive Directory Documentation

## Overview

The `/sw/device/tests/` directory contains chip-level tests that run on OpenTitan hardware across all verification targets including DV simulation, Verilator simulation, FPGA, and silicon.  

## Test Types

The directory contains three main categories of tests:

1. **Smoke Tests** - Minimal operations on IP blocks to verify basic functionality (naming: `{IP name}_smoketest.c`)
2. **IP Integration Tests** - Tests exercising specific IP functionality (naming: `{IP name}[_{feature}]_test.c`)
3. **System-level Tests** - Complex scenarios encompassing multiple functionalities (naming: `{use case}_systemtest.c`) 

---

## Directory Structure

### Root Directory (`/sw/device/tests/`)

Contains the primary test suite with 150+ individual test files covering all major IP blocks and system-level scenarios.

### Subdirectories

#### 1. **`crypto/`** - Cryptographic Function Tests
**Purpose**: Comprehensive cryptographic testing including functional tests, test vectors, and the cryptotest framework.

**Subdirectories**:
- `crypto/cryptotest/` - Unified framework for cryptographic API testing and compliance
  - `firmware/` - On-device command listener firmware
  - `json/` - uJSON command definitions
- `crypto/testvectors/` - Test vectors for cryptographic operations
  - `sphincsplus_kat/` - SPHINCS+ known answer tests
  - `wycheproof/` - Wycheproof test vectors

**Crypto Test Files**:
- **AES Tests**:
  - `aes_functest.c` - AES functional testing
  - `aes_gcm_functest.c` - AES-GCM mode testing
  - `aes_gcm_timing_test.c` - AES-GCM timing analysis
  - `aes_kwp_functest.c` - AES key wrap functionality
  - `aes_kwp_kat_functest.c` - AES key wrap known answer tests
  - `aes_kwp_sideload_functest.c` - AES key wrap with sideload
  - `aes_sideload_functest.c` - AES sideload key testing

- **DRBG Tests**:
  - `drbg_functest.c` - Deterministic Random Bit Generator testing

- **ECDH Tests**:
  - `ecdh_p256_functest.c` - ECDH P-256 curve operations
  - `ecdh_p256_sideload_functest.c` - ECDH P-256 with sideload
  - `ecdh_p384_functest.c` - ECDH P-384 curve operations
  - `ecdh_p384_sideload_functest.c` - ECDH P-384 with sideload

- **ECDSA Tests**:
  - `ecdsa_p256_functest.c` - ECDSA P-256 signing
  - `ecdsa_p256_sideload_functest.c` - ECDSA P-256 with sideload
  - `ecdsa_p256_verify_functest.c` - ECDSA P-256 verification
  - `ecdsa_p384_functest.c` - ECDSA P-384 signing
  - `ecdsa_p384_sideload_functest.c` - ECDSA P-384 with sideload

- **HKDF & HMAC Tests**:
  - `hkdf_functest.c` - HMAC-based Key Derivation Function
  - `hmac_functest.c` - HMAC functionality
  - `hmac_multistream_functest.c` - HMAC streaming operations
  - `hmac_sha256_functest.c` - HMAC-SHA256
  - `hmac_sha384_functest.c` - HMAC-SHA384
  - `hmac_sha512_functest.c` - HMAC-SHA512

- **KDF Tests**:
  - `kdf_hmac_ctr_functest.c` - KDF with HMAC counter mode
  - `kdf_kmac_functest.c` - KDF with KMAC
  - `kdf_kmac_sideload_functest.c` - KDF with KMAC sideload

- **KMAC Tests**:
  - `kmac_functest.c` - KMAC functionality
  - `kmac_sideload_functest.c` - KMAC with sideload keys

- **RSA Tests**:
  - `rsa_2048_encryption_functest.c` - RSA-2048 encryption
  - `rsa_2048_key_from_cofactor_functest.c` - RSA-2048 key generation from cofactor
  - `rsa_2048_keygen_functest.c` - RSA-2048 key generation
  - `rsa_2048_signature_functest.c` - RSA-2048 signatures
  - `rsa_3072_encryption_functest.c` - RSA-3072 encryption
  - `rsa_3072_keygen_functest.c` - RSA-3072 key generation
  - `rsa_3072_signature_functest.c` - RSA-3072 signatures
  - `rsa_3072_verify_functest.c` - RSA-3072 verification
  - `rsa_4096_encryption_functest.c` - RSA-4096 encryption
  - `rsa_4096_keygen_functest.c` - RSA-4096 key generation
  - `rsa_4096_signature_functest.c` - RSA-4096 signatures

- **SHA Tests**:
  - `sha256_functest.c` - SHA-256 hashing
  - `sha384_functest.c` - SHA-384 hashing
  - `sha512_functest.c` - SHA-512 hashing

- **Other Crypto Tests**:
  - `otcrypto_export_size.c` - Crypto export size validation
  - `otcrypto_export_test.c` - Crypto export functionality
  - `otcrypto_hash_test.c` - Hash function testing
  - `otcrypto_interface.c/h` - Crypto interface utilities
  - `symmetric_keygen_functest.c` - Symmetric key generation

---

#### 2. **`penetrationtests/`** - Security Penetration Testing
**Purpose**: Side-channel analysis (SCA) and fault injection (FI) attack testing framework for FPGA and silicon targets. 

**Subdirectories**:
- `firmware/` - Penetration test firmware
  - `fi/` - Fault injection test implementations
    - `otbn/` - OTBN-specific FI tests
  - `sca/` - Side-channel analysis test implementations
    - `otbn/` - OTBN-specific SCA tests
  - `lib/` - Shared penetration test libraries
- `json/` - uJSON command definitions
- `doc/` - Documentation and setup guides

**Firmware Test Files**:
- `firmware_cryptolib_fi_asym.c` - Cryptolib asymmetric FI tests
- `firmware_cryptolib_fi_sym.c` - Cryptolib symmetric FI tests
- `firmware_cryptolib_sca_asym.c` - Cryptolib asymmetric SCA tests
- `firmware_cryptolib_sca_sym.c` - Cryptolib symmetric SCA tests
- `firmware_fi.c` - General fault injection tests
- `firmware_fi_ibex.c` - Ibex core fault injection tests
- `firmware_fi_otbn.c` - OTBN fault injection tests
- `firmware_sca.c` - General side-channel analysis tests

**FI Subdirectory Tests** (`firmware/fi/`):
- `alert_fi.c/h` - Alert handler FI testing
- `crypto_fi.c/h` - Crypto block FI testing
- `cryptolib_fi_asym.c/h` - Cryptolib asymmetric FI
- `cryptolib_fi_sym.c/h` - Cryptolib symmetric FI
- `ibex_fi.S/.c/.h` - Ibex processor FI testing
- `lc_ctrl_fi.c/h` - Lifecycle controller FI
- `otbn_fi.c/h` - OTBN FI testing
- `otp_fi.c/h` - OTP controller FI
- `rng_fi.c/h` - RNG FI testing
- `rom_fi.c/h` - ROM FI testing

**SCA Subdirectory Tests** (`firmware/sca/`):
- `aes_sca.c/h` - AES side-channel analysis
- `cryptolib_sca_asym.c/h` - Cryptolib asymmetric SCA
- `cryptolib_sca_sym.c/h` - Cryptolib symmetric SCA
- `ecc256_keygen_sca.c/h` - ECC-256 keygen SCA
- `edn_sca.c/h` - EDN side-channel analysis
- `hmac_sca.c/h` - HMAC SCA
- `ibex_sca.c/h` - Ibex processor SCA
- `kmac_sca.c/h` - KMAC SCA
- `otbn_sca.c/h` - OTBN SCA
- `prng_sca.c/h` - PRNG SCA
- `sha3_sca.c/h` - SHA3 SCA
- `trigger_sca.c/h` - SCA trigger utilities

---

#### 3. **`pmod/`** - PMOD Peripheral Tests
**Purpose**: Tests for external peripherals connected via PMOD interface (I2C and SPI devices).

**I2C PMOD Tests**:
- `i2c_host_accelerometer_test.c` - Accelerometer sensor testing
- `i2c_host_ambient_light_detector_test.c` - Light sensor testing
- `i2c_host_clock_stretching_test.c` - I2C clock stretching
- `i2c_host_compass_test.c` - Compass sensor testing
- `i2c_host_eeprom_test.c` - EEPROM testing
- `i2c_host_fram_test.c` - FRAM memory testing
- `i2c_host_gas_sensor_test.c` - Gas sensor testing
- `i2c_host_hdc1080_humidity_temp_test.c` - Humidity/temperature sensor
- `i2c_host_irq_test.c` - I2C interrupt testing
- `i2c_host_power_monitor_test.c` - Power monitor testing

**SPI PMOD Tests**:
- `spi_host_gigadevice1Gb_flash_test.c` - GigaDevice 1Gb flash
- `spi_host_gigadevice256Mb_flash_test.c` - GigaDevice 256Mb flash
- `spi_host_issi256Mb_flash_test.c` - ISSI 256Mb flash
- `spi_host_macronix128Mb_flash_test.c` - Macronix 128Mb flash
- `spi_host_macronix1Gb_flash_test.c` - Macronix 1Gb flash
- `spi_host_micron512Mb_flash_test.c` - Micron 512Mb flash
- `spi_host_winbond1Gb_flash_test.c` - Winbond 1Gb flash

---

#### 4. **`sim_dv/`** - DV Simulation Specific Tests
**Purpose**: Tests requiring Design Verification (DV) simulation environment with specific testbench capabilities. 

**Test Files**:
- **ADC Tests**:
  - `adc_ctrl_sleep_debug_cable_wakeup_test.c` - ADC wakeup from sleep

- **Alert Handler Tests**:
  - `alert_handler_entropy_test.c` - Alert handler entropy testing
  - `alert_handler_lpg_sleep_mode_alerts.c` - Low power group alerts

- **AST Tests**:
  - `ast_clk_rst_inputs.c` - AST clock/reset input testing
  - `ast_usb_clk_calib.c` - USB clock calibration

- **Clock Manager Tests**:
  - `clkmgr_external_clk_src_for_lc_test.c` - External clock for lifecycle

- **CSRNG Tests**:
  - `csrng_fuse_en_sw_app_read.c` - CSRNG fuse enable test
  - `csrng_lc_hw_debug_en_test.c` - CSRNG lifecycle hardware debug

- **Flash Controller Tests**:
  - `exit_test_unlocked_bootstrap.c` - Unlocked bootstrap exit
  - `flash_ctrl_lc_rw_en_test.c` - Flash lifecycle read/write enable
  - `flash_init_test.c` - Flash initialization
  - `flash_rma_unlocked_test.c` - Flash RMA unlocked mode

- **I2C Tests**:
  - `i2c_device_tx_rx_test.c` - I2C device TX/RX
  - `i2c_host_tx_rx_test.c` - I2C host TX/RX

- **Lifecycle Tests**:
  - `lc_ctrl_scrap_test.c` - Lifecycle scrap state
  - `lc_ctrl_transition_test.c` - Lifecycle transitions
  - `lc_ctrl_volatile_raw_unlock_test.c` - Volatile raw unlock
  - `lc_walkthrough_test.c` - Lifecycle walkthrough
  - `lc_walkthrough_testunlocks_test.c` - Test unlock walkthrough

- **OTP Tests**:
  - `inject_scramble_seed.c` - Scramble seed injection
  - `otp_ctrl_lc_signals_test.c` - OTP lifecycle signals
  - `otp_ctrl_vendor_test_csr_access_test.c` - Vendor test CSR access
  - `otp_ctrl_vendor_test_ecc_error_test.c` - Vendor test ECC errors

- **Power Manager Tests**:
  - `pwrmgr_b2b_sleep_reset_test.c` - Back-to-back sleep/reset
  - `pwrmgr_deep_sleep_power_glitch_test.c` - Deep sleep power glitch
  - `pwrmgr_main_power_glitch_test.c` - Main power glitch
  - `pwrmgr_random_sleep_power_glitch_reset_test.c` - Random sleep power glitch
  - `pwrmgr_sleep_power_glitch_test.c` - Sleep power glitch
  - `pwrmgr_sysrst_ctrl_test.c` - System reset control

- **ROM & Sensor Tests**:
  - `rom_ctrl_integrity_check_test.c` - ROM integrity checking
  - `sensor_ctrl_status.c` - Sensor controller status

- **SPI Tests**:
  - `spi_host_tx_rx_test.c` - SPI host TX/RX
  - `spi_passthrough_test.c` - SPI passthrough mode

- **SRAM Tests**:
  - `sram_ctrl_execution_main_test.c` - SRAM execution from main

- **System Reset Tests**:
  - `sysrst_ctrl_ec_rst_l_test.c` - EC reset line control

---

#### 5. **`otbn_data/`** - OTBN Test Data
**Purpose**: Test data and parameters for OTBN (OpenTitan Big Number accelerator) tests.

**Files**:
- `otbn_rsa_test_private_key_3072.pem` - RSA-3072 private key for testing
- `otbn_rsa_test_private_key_4096.pem` - RSA-4096 private key for testing
- `otbn_test_params.py` - Python test parameter generation

---

#### 6. **`doc/`** - Documentation
**Purpose**: Additional test documentation and guides.

---

#### 7. **`closed_source/`** - Proprietary Tests
**Purpose**: Closed-source test implementations (content not publicly visible).

---

## Root Directory Test Files (Alphabetical by IP Block)

### AES (Advanced Encryption Standard) Tests
- `aes_entropy_test.c` - AES entropy source testing
- `aes_force_prng_reseed_test.c` - Force PRNG reseed operations
- `aes_idle_test.c` - AES idle state behavior
- `aes_interrupt_encryption_test.c` - Interrupt-driven encryption
- `aes_prng_reseed_test.c` - PRNG reseeding during operations
- `aes_smoketest.c` - AES basic functionality smoke test
- `aes_stall_test.c` - AES stall condition handling

### Alert Handler Tests
- `alert_handler_escalation_test.c` - Alert escalation mechanisms
- `alert_handler_lpg_sleep_mode_pings.c` - Low power group sleep mode pings
- `alert_handler_ping_ok_test.c` - Alert ping success scenarios
- `alert_handler_ping_timeout_test.c` - Alert ping timeout handling

### AON Timer Tests
- `aon_timer_irq_test.c` - Always-on timer interrupt testing
- `aon_timer_sleep_wdog_sleep_pause_test.c` - Watchdog sleep pause
- `aon_timer_wdog_bite_reset_test.c` - Watchdog bite reset
- `aon_timer_wdog_lc_escalate_test.c` - Watchdog lifecycle escalation

### AST (Analog Sensor Top) Tests
- `ast_clk_outs_test.c` - AST clock output testing

### Chip Power Tests
- `chip_power_idle_load_test.c` - Chip power during idle with load
- `chip_power_sleep_load_test.c` - Chip power during sleep with load

### Clock Manager Tests
- `clkmgr_external_clk_src_for_sw_fast_test.c` - External clock fast switching
- `clkmgr_external_clk_src_for_sw_slow_test.c` - External clock slow switching
- `clkmgr_jitter_frequency_test.c` - Clock jitter frequency analysis
- `clkmgr_jitter_test.c` - Clock jitter testing
- `clkmgr_off_aes_trans_test.c` - AES transactional clock gating
- `clkmgr_off_hmac_trans_test.c` - HMAC transactional clock gating
- `clkmgr_off_kmac_trans_test.c` - KMAC transactional clock gating
- `clkmgr_off_otbn_trans_test.c` - OTBN transactional clock gating
- `clkmgr_off_peri_test.c` - Peripheral clock gating
- `clkmgr_reset_frequency_test.c` - Clock frequency after reset
- `clkmgr_sleep_frequency_test.c` - Clock frequency during sleep

### Coverage & CRT Tests
- `coverage_test.c` - Code coverage testing
- `crt_test.c` - C runtime startup testing

### CSRNG (Cryptographically Secure Random Number Generator) Tests
- `csrng_edn_concurrency_test.c` - CSRNG/EDN concurrent operations
- `csrng_kat_test.c` - CSRNG known answer tests

### DMA (Direct Memory Access) Tests
- `dma_abort.c` - DMA abort operations
- `dma_inline_hashing.c` - DMA with inline hashing

### EDN (Entropy Distribution Network) Tests
- `edn_auto_mode.c` - EDN automatic mode
- `edn_boot_mode.c` - EDN boot mode operation
- `edn_kat.c` - EDN known answer tests
- `edn_sw_mode.c` - EDN software mode

### Entropy Source Tests
- `entropy_src_ast_rng_req_test.c` - AST RNG request handling
- `entropy_src_bypass_mode_health_test.c` - Bypass mode health checks
- `entropy_src_csrng_test.c` - Entropy source to CSRNG interface
- `entropy_src_edn_reqs_test.c` - EDN request handling
- `entropy_src_fw_observe_many_contiguous.c` - Firmware observation mode
- `entropy_src_fw_override_test.c` - Firmware override capabilities
- `entropy_src_fw_ovr_test.c` - Firmware override testing
- `entropy_src_kat_test.c` - Entropy source known answer tests
- `entropy_src_smoketest.c` - Entropy source smoke test

### Example Tests
- `example_concurrency_test.c` - Example concurrency patterns
- `example_mem_ujcmd.c` - Example memory uJSON commands
- `example_sival.c` - Example silicon validation test
- `example_test_from_flash.c` - Example test executed from flash
- `example_test_from_rom.c` - Example test executed from ROM

### Flash Controller Tests
- `flash_ctrl_clock_freqs_test.c` - Flash clock frequency variations
- `flash_ctrl_idle_low_power_test.c` - Flash idle low power mode
- `flash_ctrl_info_access_lc.c` - Flash info partition lifecycle access
- `flash_ctrl_mem_protection_test.c` - Flash memory protection
- `flash_ctrl_rma_test.c` - Flash RMA (Return Merchandise Authorization)
- `flash_ctrl_write_clear_test.c` - Flash write and clear operations

### GPIO Tests
- `gpio_intr_test.c` - GPIO interrupt handling
- `gpio_pinmux_test.c` - GPIO pin multiplexing

### HMAC Tests
- `hmac_endianness_test.c` - HMAC endianness handling
- `hmac_error_conditions_test.c` - HMAC error scenarios
- `hmac_secure_wipe_test.c` - HMAC secure memory wiping
- `hmac_smoketest.c` - HMAC smoke test

### I2C Tests
- `i2c_host_override_test.c` - I2C host override functionality
- `i2c_target_smbus_arp_test.c` - I2C target SMBus ARP
- `i2c_target_test.c` - I2C target mode operation

### Key Manager Tests
- `keymgr_dpe_key_derivation_test.c` - DPE key derivation
- `keymgr_key_derivation_test.c` - Standard key derivation
- `keymgr_sideload_aes_test.c` - AES sideload key testing
- `keymgr_sideload_kmac_test.c` - KMAC sideload key testing
- `keymgr_sideload_otbn_test.c` - OTBN sideload key testing

### KMAC Tests
- `kmac_app_rom_test.c` - KMAC ROM application interface
- `kmac_endianess_test.c` - KMAC endianness handling
- `kmac_error_conditions_test.c` - KMAC error scenarios
- `kmac_mode_cshake_test.c` - KMAC cSHAKE mode
- `kmac_mode_kmac_test.c` - KMAC mode operations

### Lifecycle Controller Tests
- `lc_ctrl_otp_hw_cfg0_test.c` - Lifecycle OTP hardware config

### OTBN (OpenTitan Big Number) Tests
- `otbn_ecdsa_op_irq_test.c` - OTBN ECDSA interrupt operations
- `otbn_irq_test.c` - OTBN interrupt handling
- `otbn_isa_test.c` - OTBN instruction set architecture
- `otbn_randomness_test.c` - OTBN randomness generation
- `otbn_rsa_test.c` - OTBN RSA operations

### OTP Controller Tests
- `otp_ctrl_descrambling_test.c` - OTP descrambling operations
- `otp_ctrl_mem_access_test.c` - OTP memory access patterns

### OTTF (On-device Test Framework) Tests
- `ottf_console_with_gpio_tx_indicator_test.c` - Console with GPIO TX indicator

### PLIC (Platform-Level Interrupt Controller) Tests
- `plic_sw_irq_test.c` - Software interrupt testing

### PMP (Physical Memory Protection) Tests
- `pmp_smoketest_napot.c` - PMP naturally aligned power-of-two
- `pmp_smoketest_tor.c` - PMP top-of-range

### Power Virus Test
- `power_virus_systemtest.c` - Maximum power consumption stress test

### PWM (Pulse Width Modulation) Tests
- `pwm_smoketest.c` - PWM smoke test

### Power Manager Tests
- `pwrmgr_deep_sleep_por_reset_test.c` - Deep sleep power-on-reset
- `pwrmgr_lowpower_cancel_test.c` - Low power mode cancellation
- `pwrmgr_normal_sleep_por_reset_test.c` - Normal sleep power-on-reset
- `pwrmgr_sensor_ctrl_deep_sleep_wake_up.c` - Sensor wakeup from deep sleep
- `pwrmgr_smoketest.c` - Power manager smoke test
- `pwrmgr_usb_clk_disabled_when_active_test.c` - USB clock disable when active
- `pwrmgr_usbdev_smoketest.c` - Power manager USB device smoke test
- `pwrmgr_wdog_reset_reqs_test.c` - Watchdog reset requests

### Reset Manager Tests
- `rstmgr_alert_info_test.c` - Reset manager alert information
- `rstmgr_cpu_info_test.c` - Reset manager CPU information
- `rstmgr_sw_req_test.c` - Software reset requests

### RV Core Ibex Tests
- `rv_core_ibex_address_translation_test.S/.c` - Address translation
- `rv_core_ibex_epmp_test.S` - Enhanced physical memory protection
- `rv_core_ibex_icache_invalidate_test.c` - Instruction cache invalidation
- `rv_core_ibex_isa_test.S/.c` - Ibex ISA compliance
- `rv_core_ibex_mem_test.c` - Ibex memory operations
- `rv_core_ibex_nmi_irq_test.c` - Non-maskable interrupt/IRQ
- `rv_core_ibex_rnd_test.S/.c` - Ibex random number instruction

### RV Debug Module Tests
- `rv_dm_access_after_hw_reset.c` - Debug access after hardware reset
- `rv_dm_access_after_wakeup.c` - Debug access after wakeup
- `rv_dm_delayed_enable.c` - Delayed debug module enable
- `rv_dm_lc_disabled.c` - Debug module with lifecycle disabled
- `rv_dm_ndm_reset_req.c` - Non-debug module reset request
- `rv_dm_ndm_reset_req_when_cpu_halted.c` - NDM reset with halted CPU

### RV PLIC & Timer Tests
- `rv_plic_smoketest.c` - RISC-V PLIC smoke test
- `rv_timer_smoketest.c` - RISC-V timer smoke test
- `rv_timer_systick_test.c` - System tick timer

### Sensor Controller Tests
- `sensor_ctrl_wakeup_test.c` - Sensor controller wakeup

### Sleep Pin Tests
- `sleep_pin_mio_dio_val_test.c` - Sleep pin MIO/DIO value retention
- `sleep_pin_retention_test.c` - Pin state retention during sleep
- `sleep_pin_wake_test.c` - Wakeup via pin
- `sleep_pwm_pulses_test.c` - PWM pulses during sleep

### SOC Proxy Tests
- `soc_proxy_smoketest.c` - SOC proxy smoke test

### SPI Device Tests
- `spi_device_flash_smoketest.c` - SPI device flash mode smoke test
- `spi_device_ottf_console_test.c` - SPI device OTTF console
- `spi_device_sleep_test.c` - SPI device sleep mode
- `spi_device_tpm_tx_rx_test.c` - SPI device TPM TX/RX
- `spi_device_ujson_console_test.c` - SPI device uJSON console

### SPI Host Tests
- `spi_host_config_test.c` - SPI host configuration
- `spi_host_irq_test.c` - SPI host interrupt handling
- `spi_host_smoketest.c` - SPI host smoke test
- `spi_host_winbond_flash_test.c` - Winbond flash testing

### SPI Passthrough Tests
- `spi_passthru_test.c` - SPI passthrough functionality

### SRAM Controller Tests
- `sram_ctrl_execution_test.c` - Code execution from SRAM
- `sram_ctrl_lc_escalation_test.c` - SRAM lifecycle escalation
- `sram_ctrl_memset_test.c` - SRAM memset operations
- `sram_ctrl_readback_test.c` - SRAM readback verification
- `sram_ctrl_sleep_sram_ret_contents_no_scramble_test.c` - Sleep retention without scrambling
- `sram_ctrl_sleep_sram_ret_contents_scramble_test.c` - Sleep retention with scrambling
- `sram_ctrl_subword_access_test.c` - Sub-word memory access

### Status Report Tests
- `status_report_overflow_test.c` - Status report buffer overflow
- `status_report_test.c` - Status reporting functionality

### System Reset Controller Tests
- `sysrst_ctrl_ec_rst_l_test.c` - EC reset line control
- `sysrst_ctrl_in_irq_test.c` - Input interrupt testing
- `sysrst_ctrl_inputs_test.c` - Input signal testing
- `sysrst_ctrl_reset_test.c` - System reset operations
- `sysrst_ctrl_ulp_z3_wakeup_test.c` - Ultra-low power Z3 wakeup

### UART Tests
- `uart_baud_rate_test.c` - UART baud rate configuration
- `uart_loopback_test.c` - UART loopback mode
- `uart_parity_break_test.c` - Parity and break signal testing
- `uart_smoketest.c` - UART smoke test
- `uart_tx_rx_test.c` - UART transmit/receive

### USB Device Tests
- `usbdev_aon_pullup_test.c` - Always-on pullup resistor
- `usbdev_aon_wake_disconnect_test.c` - AON wake on disconnect
- `usbdev_aon_wake_reset_test.c` - AON wake on reset
- `usbdev_config_host_test.c` - USB device host configuration
- `usbdev_deep_disconnect_test.c` - Deep sleep disconnect
- `usbdev_deep_reset_test.c` - Deep sleep reset
- `usbdev_deep_resume_test.c` - Deep sleep resume
- `usbdev_iso_test.c` - Isochronous transfer testing
- `usbdev_logging_test.c` - USB device logging
- `usbdev_mem_test.c` - USB device memory operations
- `usbdev_mixed_test.c` - Mixed transfer types
- `usbdev_pincfg_test.c` - USB pin configuration
- `usbdev_pullup_test.c` - Pullup resistor control
- `usbdev_sleep_disconnect_test.c` - Sleep mode disconnect
- `usbdev_sleep_reset_test.c` - Sleep mode reset
- `usbdev_sleep_resume_test.c` - Sleep mode resume
- `usbdev_stream_test.c` - Streaming transfers
- `usbdev_suspend_full_test.c` - Full suspend testing
- `usbdev_suspend_resume_test.c` - Suspend/resume cycles
- `usbdev_test.c` - General USB device functionality
- `usbdev_toggle_restore_test.c` - Data toggle restoration
- `usbdev_vbus_test.c` - VBUS detection and control

---

## Support Files

### Implementation Files (Shared Code)
- `clkmgr_external_clk_src_for_sw_impl.c/h` - Clock manager external clock implementation
- `clkmgr_off_trans_impl.c/h` - Clock manager transactional gating implementation
- `entropy_src_kat_impl.c/h` - Entropy source KAT implementation
- `otbn_randomness_impl.c/h` - OTBN randomness implementation
- `pwrmgr_sleep_resets_lib.c/h` - Power manager sleep reset library
- `spi_host_flash_test_impl.c/h` - SPI host flash test implementation
- `sram_ctrl_sleep_sram_ret_contents_impl.c/h` - SRAM retention implementation
- `usbdev_suspend.c/h` - USB device suspend utilities

---

## Navigation Tips for Agents

### Finding Tests by IP Block
Tests are primarily named using the pattern `{ip_name}_*_test.c`. For example:
- All AES tests start with `aes_`
- All UART tests start with `uart_`
- All USB device tests start with `usbdev_`

### Finding Smoke Tests
Look for files ending in `_smoketest.c` - these provide minimal verification of IP functionality.

### Finding Advanced Tests
- **Cryptographic tests**: Look in `crypto/` subdirectory
- **Security tests**: Look in `penetrationtests/` subdirectory
- **External peripheral tests**: Look in `pmod/` subdirectory
- **Simulation-specific tests**: Look in `sim_dv/` subdirectory

### Test File Patterns
- `*_smoketest.c` - Basic functionality verification
- `*_test.c` - IP integration or feature-specific tests
- `*_systemtest.c` - Complex system-level scenarios
- `*_impl.c/h` - Shared implementation code used by multiple tests
- `*_functest.c` - Functional testing (primarily in crypto/)
- `*_kat*.c` - Known Answer Tests for cryptographic validation

---

## Notes

All tests in this directory use the On-device Test Framework (OTTF) and are designed to run across multiple verification targets including DV simulation, Verilator, FPGA, and silicon. 

The cryptotest framework provides a unified approach for testvector injection and cryptolib API verification through UART-based communication between host and device. 

Penetration tests are split into separate binaries (SCA, general FI, IBEX FI, OTBN FI) due to code size limitations. 
