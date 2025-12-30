# OpenTitan `/sw/device/lib` Directory Structure

The `/sw/device/lib` directory contains 10 subdirectories that provide foundational libraries for device software. Here's a comprehensive breakdown:

---

## 1. **arch/** - Architecture and Device-Specific Implementations

**Purpose**: Contains device-specific and boot stage-specific information for different platforms (FPGA, simulation, silicon). [1](Lib索引.md#5-0) 

### Files:
- **boot_stage.h** - Defines boot stage types (ROM, ROM_EXT, Owner) to identify which stage in the boot sequence a binary is executing from [2](Lib索引.md#5-1) 
- **boot_stage_owner.c** - Boot stage implementation for Owner stage
- **boot_stage_rom.c** - Boot stage implementation for ROM stage
- **boot_stage_rom_ext.c** - Boot stage implementation for ROM_EXT stage
- **device.h** - Device-specific declarations including device types, clock frequencies, UART configuration, and platform detection [3](Lib索引.md#5-2) 
- **device_fpga_cw305.c** - Device constants for ChipWhisperer CW305 FPGA
- **device_fpga_cw310.c** - Device constants for ChipWhisperer CW310 FPGA
- **device_fpga_cw340.c** - Device constants for ChipWhisperer CW340 FPGA
- **device_silicon.c** - Device constants for silicon instantiation
- **device_sim_dv.c** - Device constants for DV simulation testbench
- **device_sim_qemu.c** - Device constants for QEMU emulation
- **device_sim_verilator.c** - Device constants for Verilator simulation

---

## 2. **base/** - Base Utility Libraries

**Purpose**: Provides fundamental utility libraries including memory-mapped I/O, bit manipulation, status codes, and freestanding C library implementations.

### Main Directory Files:
- **abs_mmio.c/h** - Abstract MMIO operations
- **adv_macros.h** - Advanced preprocessor macros
- **bitfield.c/h** - Bit field manipulation utilities
- **crc32.c/h** - CRC32 checksum implementation
- **crc32_functest.c** - Functional test for CRC32
- **crc32_perftest.c** - Performance test for CRC32
- **crc32_unittest.cc** - Unit tests for CRC32
- **csr.h** - Control and Status Register (CSR) access utilities
- **csr_registers.h** - CSR register definitions
- **global_mock.h** - Global mock utilities for testing
- **global_mock_unittest.cc** - Unit tests for global mocks
- **hardened.c/h** - Hardened boolean types with higher Hamming distance for security-critical code [4](Lib索引.md#5-3) 
- **hardened_asm.h** - Assembly definitions for hardened types
- **hardened_functest.c** - Functional tests for hardened types
- **hardened_memory.c/h** - Hardened memory operations
- **hardened_memory_unittest.cc** - Unit tests for hardened memory
- **hardened_unittest.cc** - Unit tests for hardened types
- **macros.h** - General utility macros
- **math.c/h** - Mathematical utility functions
- **math_builtins.c** - Math builtin implementations
- **math_builtins_unittest.cc** - Unit tests for math builtins
- **math_unittest.cc** - Unit tests for math functions
- **memory.c/h** - Memory manipulation functions (memcpy, memset, etc.)
- **memory_perftest.c** - Performance tests for memory operations
- **memory_unittest.cc** - Unit tests for memory operations
- **mmio.c/h** - Memory-mapped I/O functions for volatile hardware access [5](Lib索引.md#5-4) 
- **mock_abs_mmio.cc/h** - Mock implementations for abstract MMIO testing
- **mock_crc32.cc/h** - Mock implementations for CRC32 testing
- **mock_mmio.cc/h** - Mock implementations for MMIO testing
- **mock_mmio_test.cc** - Tests for MMIO mocks
- **mock_mmio_test_utils.h** - Utility functions for MMIO mock testing
- **multibits.h** - Multi-bit encoded values for fault tolerance
- **multibits_asm.h** - Assembly definitions for multi-bit values
- **random_order.c/h** - Random ordering utilities
- **random_order_unittest.cc** - Unit tests for random ordering
- **status.c/h** - Status code handling with module identification and line numbers [6](Lib索引.md#5-5) 
- **status_report_unittest.cc** - Unit tests for status reporting
- **status_report_unittest_c.c/h** - C-based status reporting tests
- **status_unittest.cc** - Unit tests for status codes
- **stdasm.h** - Standard assembly macros

### Subdirectory: **freestanding/**
Freestanding C library headers (minimal standard library for embedded systems):
- **assert.h** - Assert macro implementation
- **float.h** - Floating-point constants
- **iso646.h** - Alternative operator spellings
- **limits.h** - Integer limits
- **stdalign.h** - Alignment support
- **stdarg.h** - Variable arguments
- **stdbool.h** - Boolean type
- **stddef.h** - Standard definitions
- **stdint.h** - Integer types
- **stdnoreturn.h** - Non-returning function attribute
- **string.h** - String and memory operations [7](Lib索引.md#5-6) 

### Subdirectory: **internal/**
Internal implementation details:
- **absl_status.h** - Abseil status code compatibility
- **status.h** - Internal status implementation details

---

## 3. **boards/** - Board-Specific Support

**Purpose**: Contains board-specific hardware support code.

### Subdirectory: **teacup_v1_3_0/**
Support for the Teacup development board:
- **leds.c/h** - LED controller driver for Teacup board's I2C RGB LEDs [8](Lib索引.md#5-7) 

---

## 4. **crt/** - C Runtime

**Purpose**: C runtime library providing initialization functions usable before full C runtime setup. [9](Lib索引.md#5-8) 

### Files:
- **crt.S** - Assembly functions for early initialization including section clearing routines [10](Lib索引.md#5-9) 

---

## 5. **crypto/** - Cryptographic Library

**Purpose**: Comprehensive cryptographic library providing high-level cryptographic operations and hardware acceleration drivers. [11](Lib索引.md#5-10) 

### Subdirectory: **data/**
- **crypto_testplan.hjson** - Test plan configuration for crypto tests

### Subdirectory: **drivers/**
Hardware acceleration drivers:
- **aes.c/h** - AES hardware driver
- **aes_test.c** - AES driver tests
- **entropy.c/h** - Entropy source driver
- **entropy_kat.c/h** - Known Answer Tests for entropy
- **entropy_test.c** - Entropy driver tests
- **hmac.c/h** - HMAC hardware driver
- **keymgr.c/h** - Key manager hardware driver
- **keymgr_test.c** - Key manager tests
- **kmac.c/h** - KMAC hardware driver
- **mock_entropy.cc** - Mock entropy for testing
- **mock_rv_core_ibex.cc** - Mock Ibex core for testing
- **otbn.c/h** - OpenTitan Big Number accelerator driver
- **rv_core_ibex.c/h** - RISC-V Ibex core interface
- **rv_core_ibex_test.c** - Ibex core tests

### Subdirectory: **impl/**
Cryptographic algorithm implementations:
- **aes.c** - AES software implementation
- **aes_gcm.c** - AES-GCM implementation
- **drbg.c** - Deterministic Random Bit Generator
- **ecc_p256.c** - P-256 elliptic curve operations
- **ecc_p384.c** - P-384 elliptic curve operations
- **ed25519.c** - Ed25519 signature algorithm
- **hkdf.c** - HMAC-based Key Derivation Function
- **hmac.c** - HMAC implementation
- **integrity.c/h** - Integrity checking
- **kdf_ctr.c** - Counter-mode KDF
- **key_transport.c** - Key transport mechanisms
- **key_transport_unittest.cc** - Key transport tests
- **keyblob.c/h** - Key blob handling
- **keyblob_unittest.cc** - Key blob tests
- **kmac.c** - KMAC implementation
- **kmac_kdf.c** - KMAC-based KDF
- **rsa.c** - RSA operations
- **sha2.c** - SHA-2 hash functions
- **sha3.c** - SHA-3 hash functions
- **status.h** - Crypto status codes
- **status_debug_unittest.cc** - Status debugging tests
- **status_unittest.cc** - Status tests
- **x25519.c** - X25519 key exchange

#### Subdirectory: **aes_gcm/**
- **aes_gcm.c/h** - AES-GCM mode implementation
- **ghash.c/h** - GHASH function for GCM
- **ghash_unittest.cc** - GHASH tests

#### Subdirectory: **aes_kwp/**
- **aes_kwp.c/h** - AES Key Wrap with Padding

#### Subdirectory: **ecc/**
- **p256.c/h** - P-256 curve implementation
- **p384.c/h** - P-384 curve implementation

#### Subdirectory: **rsa/**
- **rsa_3072_verify.c/h** - RSA-3072 verification
- **rsa_datatypes.h** - RSA data types
- **rsa_encryption.c/h** - RSA encryption
- **rsa_keygen.c/h** - RSA key generation
- **rsa_modexp.c/h** - Modular exponentiation
- **rsa_padding.c/h** - RSA padding schemes
- **rsa_signature.c/h** - RSA signature operations

#### Subdirectory: **sha2/**
- **sha256.c/h** - SHA-256 implementation
- **sha512.c/h** - SHA-512 implementation

### Subdirectory: **include/**
Public cryptographic API headers:
- **aes.h** - AES API
- **aes_gcm.h** - AES-GCM API
- **datatypes.h** - Cryptographic data types
- **drbg.h** - DRBG API
- **ecc_p256.h** - P-256 ECC API
- **ecc_p384.h** - P-384 ECC API
- **ed25519.h** - Ed25519 API
- **hkdf.h** - HKDF API
- **hmac.h** - HMAC API
- **kdf_ctr.h** - Counter KDF API
- **key_transport.h** - Key transport API
- **kmac.h** - KMAC API
- **kmac_kdf.h** - KMAC KDF API
- **otcrypto.h** - Unified crypto library header
- **rsa.h** - RSA API
- **sha2.h** - SHA-2 API
- **sha3.h** - SHA-3 API
- **x25519.h** - X25519 API

#### Subdirectory: **freestanding/**
- **absl_status.h** - Abseil status compatibility
- **defs.h** - Common definitions
- **hardened.h** - Hardened types for crypto

---

## 6. **dif/** - Device Interface Functions

**Purpose**: Low-level hardware access routines for all OpenTitan peripherals. DIFs provide direct hardware functionality access for use in verification and testing. [12](Lib索引.md#5-11) 

### Files:
- **README.md** - DIF library documentation and style guide
- **dif_adc_ctrl.c/h/md** - ADC Controller DIF
- **dif_adc_ctrl_unittest.cc** - ADC Controller tests
- **dif_aes.c/h/md** - AES DIF
- **dif_aes_unittest.cc** - AES tests
- **dif_alert_handler.c/h/md** - Alert Handler DIF
- **dif_alert_handler_unittest.cc** - Alert Handler tests
- **dif_aon_timer.c/h/md** - Always-On Timer DIF
- **dif_aon_timer_unittest.cc** - AON Timer tests
- **dif_base.c/h** - Base DIF types and utilities
- **dif_clkmgr.md** - Clock Manager documentation
- **dif_clkmgr_unittest.cc** - Clock Manager tests
- **dif_csrng.c/h/md** - CSRNG (Cryptographically Secure RNG) DIF
- **dif_csrng_shared.c/h** - Shared CSRNG utilities
- **dif_csrng_unittest.cc** - CSRNG tests
- **dif_dma.c/h/md** - DMA Controller DIF
- **dif_dma_unittest.cc** - DMA tests
- **dif_edn.c/h/md** - Entropy Distribution Network DIF
- **dif_edn_unittest.cc** - EDN tests
- **dif_entropy_src.c/h/md** - Entropy Source DIF
- **dif_entropy_src_unittest.cc** - Entropy Source tests
- **dif_flash_ctrl.c/h/md** - Flash Controller DIF
- **dif_flash_ctrl_unittest.cc** - Flash Controller tests
- **dif_gpio.c/h** - GPIO DIF
- **dif_gpio_unittest.cc** - GPIO tests
- **dif_hmac.c/h/md** - HMAC DIF
- **dif_hmac_unittest.cc** - HMAC tests
- **dif_i2c.c/h/md** - I2C DIF
- **dif_i2c_unittest.cc** - I2C tests
- **dif_keymgr.c/h/md** - Key Manager DIF
- **dif_keymgr_dpe.c/h/md** - Key Manager DPE variant DIF
- **dif_keymgr_unittest.cc** - Key Manager tests
- **dif_kmac.c/h/md** - KMAC DIF
- **dif_kmac_unittest.cc** - KMAC tests
- **dif_lc_ctrl.c/h/md** - Lifecycle Controller DIF
- **dif_lc_ctrl_unittest.cc** - Lifecycle Controller tests
- **dif_mbx.c/h/md** - Mailbox DIF
- **dif_mbx_unittest.cc** - Mailbox tests
- **dif_otbn.c/h/md** - OTBN (OpenTitan Big Number) DIF
- **dif_otbn_unittest.cc** - OTBN tests
- **dif_otp_ctrl.c/h/md** - OTP Controller DIF
- **dif_otp_ctrl_unittest.cc** - OTP Controller tests
- **dif_pattgen.c/h/md** - Pattern Generator DIF
- **dif_pattgen_unittest.cc** - Pattern Generator tests
- **dif_pinmux.c/md** - Pin Multiplexer DIF
- **dif_pinmux_unittest.cc** - Pin Multiplexer tests
- **dif_pwm.c/h** - PWM DIF
- **dif_pwm_unittest.cc** - PWM tests
- **dif_pwrmgr.c/h/md** - Power Manager DIF
- **dif_pwrmgr_unittest.cc** - Power Manager tests
- **dif_rom_ctrl.c/h/md** - ROM Controller DIF
- **dif_rom_ctrl_unittest.cc** - ROM Controller tests
- **dif_rstmgr.md** - Reset Manager documentation
- **dif_rstmgr_unittest.cc** - Reset Manager tests
- **dif_rv_core_ibex.c/h** - RISC-V Ibex Core DIF
- **dif_rv_core_ibex_unittest.cc** - Ibex Core tests
- **dif_rv_dm.c/h/md** - RISC-V Debug Module DIF
- **dif_rv_dm_unittest.cc** - Debug Module tests
- **dif_rv_plic.c/h/md** - RISC-V PLIC (Platform-Level Interrupt Controller) DIF
- **dif_rv_plic_unittest.cc** - PLIC tests
- **dif_rv_timer.c/h/md** - RISC-V Timer DIF
- **dif_rv_timer_unittest.cc** - Timer tests
- **dif_sensor_ctrl.c/h/md** - Sensor Controller DIF
- **dif_sensor_ctrl_unittest.cc** - Sensor Controller tests
- **dif_soc_dbg_ctrl.c/h** - SoC Debug Controller DIF
- **dif_spi_device.c/h/md** - SPI Device DIF
- **dif_spi_device_unittest.cc** - SPI Device tests
- **dif_spi_host.c/h/md** - SPI Host DIF
- **dif_spi_host_unittest.cc** - SPI Host tests
- **dif_sram_ctrl.c/h/md** - SRAM Controller DIF
- **dif_sram_ctrl_unittest.cc** - SRAM Controller tests
- **dif_sysrst_ctrl.c/h/md** - System Reset Controller DIF
- **dif_sysrst_ctrl_unittest.cc** - System Reset Controller tests
- **dif_test_base.h** - Base utilities for DIF testing
- **dif_uart.c/h/md** - UART DIF
- **dif_uart_unittest.cc** - UART tests
- **dif_usbdev.c/h/md** - USB Device DIF
- **dif_usbdev_unittest.cc** - USB Device tests

---

## 7. **peripherals/** - External Peripheral Drivers

**Purpose**: Drivers for external peripheral devices not part of the OpenTitan chip itself.

### Files:
- **ssd1131_screen.c/h** - Driver for SSD1131 OLED screen controller [13](Lib索引.md#5-12) 

---

## 8. **runtime/** - Runtime Support Libraries

**Purpose**: Runtime support including logging, interrupt handling, processor control, and memory protection.

### Files:
- **epmp.c/h** - Enhanced Physical Memory Protection (ePMP) configuration
- **hart.c/h** - Hardware thread (hart) control including halt and wait functions [14](Lib索引.md#5-13) 
- **hart_polyfills.c** - Hart compatibility functions
- **ibex.c/h** - Ibex processor-specific functions including cycle-accurate busy loops [15](Lib索引.md#5-14) 
- **irq.c/h** - Interrupt handling utilities
- **log.c/h** - Generic logging APIs with severity levels and formatting [16](Lib索引.md#5-15) 
- **pmp.c/h** - Physical Memory Protection (PMP) configuration
- **print.c/h** - Basic printing functions
- **print_unittest.cc** - Print function tests

---

## 9. **testing/** - Test Utilities and Framework

**Purpose**: Test utility libraries and on-device test framework for chip-level testing across all verification targets. [17](Lib索引.md#5-16) 

### Main Directory Files:
- **README.md** - Testing library documentation
- **alert_handler_testutils.c/h** - Alert Handler test utilities
- **aon_timer_testutils.h** - AON Timer test utilities
- **binary_blob.h** - Binary blob handling for tests
- **clkmgr_testutils.c** - Clock Manager test utilities
- **csrng_testutils.c/h** - CSRNG test utilities
- **dma_testutils.c/h** - DMA test utilities
- **edn_testutils.c/h** - EDN test utilities
- **entropy_src_testutils.c/h** - Entropy Source test utilities
- **entropy_testutils.c/h** - General entropy test utilities
- **hexstr.c/h** - Hexadecimal string conversion
- **hexstr_unittest.cc** - Hexadecimal string tests
- **hmac_testutils.c/h** - HMAC test utilities
- **i2c_testutils.c/h** - I2C test utilities
- **keymgr_dpe_testutils.c/h** - Key Manager DPE test utilities
- **keymgr_testutils.c/h** - Key Manager test utilities
- **kmac_testutils.c/h** - KMAC test utilities
- **lc_ctrl_testutils.c/h** - Lifecycle Controller test utilities
- **nv_counter_testutils.c/h** - Non-volatile counter test utilities
- **otbn_testutils.c/h** - OTBN test utilities
- **otbn_testutils_rsa.c/h** - OTBN RSA test utilities
- **otp_ctrl_testutils.c/h** - OTP Controller test utilities
- **profile.c/h** - Performance profiling utilities
- **pwrmgr_testutils.c/h** - Power Manager test utilities
- **rand_testutils.c/h** - Random number test utilities
- **randomness_quality.c/h** - Randomness quality testing
- **rstmgr_testutils.c/h** - Reset Manager test utilities
- **rv_core_ibex_testutils.c/h** - Ibex Core test utilities
- **rv_plic_testutils.c/h** - PLIC test utilities
- **sensor_ctrl_testutils.c/h** - Sensor Controller test utilities
- **spi_device_testutils.c/h** - SPI Device test utilities
- **spi_flash_emulator.c/h** - SPI Flash emulator for testing
- **spi_flash_testutils.c/h** - SPI Flash test utilities
- **spi_host_testutils.c/h** - SPI Host test utilities
- **sram_ctrl_testutils.c/h** - SRAM Controller test utilities
- **sysrst_ctrl_testutils.c/h** - System Reset Controller test utilities
- **uart_testutils.c/h** - UART test utilities
- **usb_logging.c/h** - USB logging utilities
- **usb_testutils.c/h** - USB test utilities
- **usb_testutils_controlep.c/h** - USB control endpoint utilities
- **usb_testutils_diags.h** - USB diagnostics
- **usb_testutils_simpleserial.c/h** - USB simple serial utilities
- **usb_testutils_streams.c/h** - USB streaming utilities

### Subdirectory: **json/**
JSON command handling for tests:
- **chip_specific_startup.c/h** - Chip-specific startup for JSON tests
- **command.c/h** - Command parsing
- **gpio.c/h** - GPIO JSON commands
- **i2c_target.c/h** - I2C target JSON commands
- **mem.c/h** - Memory JSON commands
- **ottf.c/h** - OTTF JSON integration
- **pinmux.c/h** - Pin multiplexer JSON commands
- **pinmux_config.c/h** - Pin multiplexer configuration
- **provisioning_data.c/h** - Provisioning data handling
- **spi_passthru.c/h** - SPI passthrough JSON commands

### Subdirectory: **test_framework/**
On-device test framework (OTTF): [18](Lib索引.md#5-17) 

- **FreeRTOSConfig.h** - FreeRTOS configuration
- **README.md** - Test framework documentation
- **check.h** - Test assertion macros
- **chip_level_test_infra.svg** - Test infrastructure diagram
- **coverage.h** - Code coverage support
- **coverage_llvm.c** - LLVM coverage implementation
- **coverage_none.c** - No-coverage stub
- **freertos_hooks.c** - FreeRTOS hook functions
- **freertos_port.S** - FreeRTOS port assembly
- **freertos_port.c** - FreeRTOS port C code
- **ottf_console.c/h** - Console I/O for OTTF
- **ottf_console_internal.h** - Internal console definitions
- **ottf_console_spi.c** - SPI console implementation
- **ottf_console_uart.c** - UART console implementation
- **ottf_flow_control_functest.c** - Flow control functional test
- **ottf_isrs.S** - Interrupt service routines
- **ottf_macros.h** - OTTF utility macros
- **ottf_main.h** - OTTF main entry point
- **ottf_start.S** - OTTF startup assembly
- **ottf_test_config.h** - Test configuration
- **ottf_utils.h** - OTTF utilities
- **status.c/h** - Test status reporting
- **ujson_ottf.c/h** - uJSON integration with OTTF
- **ujson_ottf_commands.c/h** - uJSON command handlers

#### Subdirectory: **data/**
- **ottf_testplan.hjson** - OTTF test plan configuration

### Subdirectory: **test_rom/**
Test ROM implementations:
- **darjeeling_fake_driver_funcs.c** - Fake driver functions for Darjeeling
- **english_breakfast_fake_driver_funcs.c** - Fake driver functions for English Breakfast
- **test_rom_start.S** - Test ROM startup assembly
- **test_rom_test.c** - Test ROM tests

---

## 10. **ujson/** - Micro JSON Library

**Purpose**: Lightweight JSON parser/serializer for device-host communication in testing environments. [19](Lib索引.md#5-18) 

### Files:
- **example.c/h** - Example usage of ujson
- **example_roundtrip.c** - Round-trip example
- **example_test.cc** - Example tests
- **private_status.c/h** - Private status handling
- **test_helpers.h** - Test helper functions
- **ujson.c/h** - Core ujson implementation
- **ujson_derive.h** - Derivation macros for ujson
- **ujson_rust.h** - Rust interface for ujson

### Subdirectory: **rust/**
Rust language bindings for ujson

---

## Notes

Each subdirectory serves a specific purpose in the OpenTitan device software stack:
- **arch/** and **crt/** provide the lowest-level platform-specific code
- **base/** provides fundamental utilities used throughout the codebase
- **dif/** provides hardware access layer (but not HAL or drivers for production)
- **runtime/** provides higher-level runtime services
- **crypto/** provides cryptographic operations
- **testing/** provides comprehensive testing infrastructure
- **ujson/** facilitates device-host communication for testing

The organization follows a layered architecture where lower-level libraries (base, arch, crt) are used by higher-level ones (dif, runtime, crypto), with testing utilities building on all of these to enable comprehensive verification across simulation, FPGA, and silicon targets.