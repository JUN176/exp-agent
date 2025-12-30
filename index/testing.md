# OpenTitan Testing Library Index Guide

## Overview

The `sw/device/lib/testing` directory contains the complete testing infrastructure for OpenTitan chip-level tests. The library consists of two main components: testutils libraries (which wrap multiple DIF invocations commonly used across tests) and the on-device test framework (OTTF).

---

## 1. Test Framework Core (`test_framework/`)

The on-device test framework provides a uniform execution environment for chip-level tests across all verification targets (DV simulation, Verilator, FPGA, and silicon). 

### Core Test Framework Files

**`ottf_main.h` / `ottf_main.c`**
- Purpose: Main entry point for on-device tests
- Key Features: Defines `test_main()` function signature, FreeRTOS task creation/management, test execution macros
- Usage: All chip-level tests must implement `test_main()` and include this header
- Provides: `EXECUTE_TEST()` macro for profiling test execution, `OTTF_DEFINE_TEST_CONFIG()` macro, task management APIs

**`check.h`**
- Purpose: Runtime assertion macros with logging integration
- Key Features: `CHECK()`, `TRY_CHECK()`, `CHECK_ARRAYS_EQ()`, `CHECK_DIF_OK()`, `CHECK_STATUS_OK()`, `UNWRAP()`
- Important: `CHECK()` aborts on failure, while `TRY_CHECK()` returns status without aborting
- Array checking: Compares buffers and prints mismatches with detailed formatting

**`status.h` / `status.c`**
- Purpose: Test status tracking and signaling
- Key Features: Defines test status values (`kTestStatusPassed`, `kTestStatusFailed`, etc.), `test_status_set()` function
- Lifecycle: Tracks device state from boot ROM through test execution to final pass/fail 
- Critical: All tests must call `test_status_set()` to signal completion 

**`ottf_console.h` / `ottf_console.c`**
- Purpose: Console abstraction for test output
- Key Features: Multi-backend console (UART/SPI), software buffering, flow control
- Structure: Supports both UART and SPI device backends with unified API [8](testing索引文档.md#5-7) 
- Functions: `ottf_console_init()`, `ottf_console_putbuf()`, `ottf_console_getc()`, flow control management

**`ottf_console_uart.h` / `ottf_console_uart.c`**
- Purpose: UART backend implementation for console
- Integration: Provides UART-specific console functionality

**`ottf_console_spi.h` / `ottf_console_spi.c`**
- Purpose: SPI device backend implementation for console
- Integration: Enables console output over SPI device interface

**`ottf_console_types.h`**
- Purpose: Common type definitions for console subsystem
- Contents: Console type enums, flow control types, function pointer types

**`ujson_ottf.h` / `ujson_ottf.c`**
- Purpose: JSON communication integration with OTTF console
- Key Features: CRC-protected JSON messaging, response macros (`RESP_OK`, `RESP_ERR`)
- Usage: Enables structured command/response communication with host 

**`ujson_ottf_commands.h` / `ujson_ottf_commands.c`**
- Purpose: Command dispatching for JSON-based test control
- Integration: Handles routing of JSON commands to appropriate handlers

**`coverage.h` / `coverage_llvm.c` / `coverage_none.c`**
- Purpose: Code coverage data collection and transmission
- Key Features: Sends LLVM profile buffer for coverage report generation 
- Variants: LLVM-based or no-op implementation depending on build configuration

**`ottf_macros.h`**
- Purpose: Common macros and constants for OTTF
- Contents: Word size definitions, context size, stack size constants 

**`ottf_test_config.h`**
- Purpose: Test configuration structure definition
- Usage: Tests use `OTTF_DEFINE_TEST_CONFIG()` to declare configuration

**`ottf_utils.h`**
- Purpose: Utility functions and helpers for OTTF

**`ottf_isrs.h` / `ottf_isrs.c` / `ottf_isrs.S`**
- Purpose: Interrupt service routine handlers for OTTF
- Contents: Default ISR implementations, interrupt vector setup

**`ottf_start.S`**
- Purpose: Assembly startup code for OTTF
- Function: Initializes CPU state before jumping to C code

**`ottf_flow_control_functest.c`**
- Purpose: Functional test for console flow control mechanisms

**`FreeRTOSConfig.h`**
- Purpose: FreeRTOS configuration for OTTF
- Contents: Task priorities, heap sizes, tick rates

**`freertos_hooks.c` / `freertos_port.c` / `freertos_port.S`**
- Purpose: FreeRTOS integration and porting layer
- Function: Adapts FreeRTOS to OpenTitan hardware

**`ottf_common.ld`**
- Purpose: Linker script for OTTF-based tests
- Function: Defines memory layout and section placement

---

## 2. Peripheral/IP-Specific Test Utilities

These files follow the naming convention `<IP>_testutils.h/c` and wrap multiple DIF calls for common testing patterns.  

### Cryptographic & Security IPs

**`aes_testutils.h` / `aes_testutils.c`**
- Purpose: AES cryptographic block testing utilities
- Key Features: Status polling, encryption/decryption setup, masking PRNG control
- Functions: `aes_testutils_setup_encryption()`, `aes_testutils_decrypt_ciphertext()`, masking configuration  

**`hmac_testutils.h` / `hmac_testutils.c`**
- Purpose: HMAC engine testing utilities
- Key Features: HMAC operation wrappers, digest comparison helpers

**`kmac_testutils.h` / `kmac_testutils.c`**
- Purpose: KMAC cryptographic accelerator testing utilities
- Key Features: KMAC operation setup, key sideloading helpers

**`keymgr_testutils.h` / `keymgr_testutils.c`**
- Purpose: Key manager testing utilities with flash initialization
- Key Features: State advancement, key derivation, identity generation
- Critical Functions: `keymgr_testutils_startup()` initializes keymgr from reset to CreatorRootKey state 
- Predefined Values: Creator/Owner secrets, binding values, key parameters 

**`keymgr_dpe_testutils.h` / `keymgr_dpe_testutils.c`**
- Purpose: Key manager DPE (DICE Protection Environment) specific testing utilities
- Integration: Extends keymgr testing for DPE functionality

**`otbn_testutils.h` / `otbn_testutils.c`**
- Purpose: OpenTitan Big Number accelerator testing utilities
- Key Features: Application loading, execution control, DMEM/IMEM access
- Macros: `OTBN_DECLARE_APP_SYMBOLS()`, `OTBN_APP_T_INIT()` for application management
- Functions: `otbn_testutils_load_app()`, `otbn_testutils_execute()`, `otbn_testutils_wait_for_done()`  

**`otbn_testutils_rsa.h` / `otbn_testutils_rsa.c`**
- Purpose: RSA-specific OTBN operations
- Key Features: RSA key generation, encryption/decryption helpers

### Entropy & Random Number Generation

**`entropy_src_testutils.h` / `entropy_src_testutils.c`**
- Purpose: Entropy source hardware testing utilities
- Key Features: Entropy source configuration, health test monitoring

**`csrng_testutils.h` / `csrng_testutils.c`**
- Purpose: Cryptographically Secure Random Number Generator testing utilities
- Key Features: CSRNG initialization, instantiation, generate commands

**`edn_testutils.h` / `edn_testutils.c`**
- Purpose: Entropy Distribution Network testing utilities
- Key Features: EDN configuration, automatic/manual mode setup

**`entropy_testutils.h` / `entropy_testutils.c`**
- Purpose: High-level entropy complex initialization
- Key Features: Complete entropy subsystem initialization (entropy_src, CSRNG, EDN0, EDN1)
- Functions: `entropy_testutils_auto_mode_init()`, `entropy_testutils_boot_mode_init()`, `entropy_testutils_stop_all()`  

**`rand_testutils.h` / `rand_testutils.c`**
- Purpose: Random number generation utilities for tests
- Key Features: Hardware/software hybrid RNG with LFSR-based PRNG
- Usage: Provides `rand_testutils_gen32()`, range generation, array shuffling 

**`randomness_quality.h` / `randomness_quality.c`**
- Purpose: Statistical quality testing for random data
- Key Features: Monobit test from Handbook of Applied Cryptography
- Significance Levels: 5% and 1% failure rate thresholds  

### Memory & Storage

**`flash_ctrl_testutils.h` / `flash_ctrl_testutils.c`**
- Purpose: Flash controller testing utilities
- Key Features: Region setup (data/info, scrambled/unscrambled), erase, program, read operations
- Functions: `flash_ctrl_testutils_wait_for_init()`, region configuration, bank erase 
- Operations: `flash_ctrl_testutils_erase_and_write_page()`, transaction waiting 

**`otp_ctrl_testutils.h` / `otp_ctrl_testutils.c`**
- Purpose: OTP controller testing utilities
- Key Features: OTP partition read/write, integrity checking

**`sram_ctrl_testutils.h` / `sram_ctrl_testutils.c`**
- Purpose: SRAM controller testing utilities
- Key Features: SRAM scrambling control, readback verification

**`ret_sram_testutils.h` / `ret_sram_testutils.c`**
- Purpose: Retention SRAM testing utilities
- Key Features: Retention memory access across power cycles

**`nv_counter_testutils.h` / `nv_counter_testutils.c`**
- Purpose: Non-volatile counter testing utilities
- Key Features: Counter read/write/increment operations

### Communication Interfaces

**`uart_testutils.h` / `uart_testutils.c`**
- Purpose: UART communication testing utilities
- Key Features: UART configuration, transmit/receive helpers

**`i2c_testutils.h` / `i2c_testutils.c`**
- Purpose: I2C interface testing utilities
- Key Features: I2C transaction helpers, timing configuration

**`spi_device_testutils.h` / `spi_device_testutils.c`**
- Purpose: SPI device mode testing utilities
- Key Features: SPI device configuration, FIFO management

**`spi_host_testutils.h` / `spi_host_testutils.c`**
- Purpose: SPI host mode testing utilities
- Key Features: SPI transaction initiation, chip select control

**`spi_flash_testutils.h` / `spi_flash_testutils.c`**
- Purpose: SPI flash operations testing utilities
- Key Features: Flash command wrappers, JEDEC ID reading

**`spi_flash_emulator.h` / `spi_flash_emulator.c`**
- Purpose: Software SPI flash emulator for testing
- Key Features: Emulates SPI EEPROM behavior for loopback testing 

**`usb_testutils.h` / `usb_testutils.c`**
- Purpose: USB device testing utilities
- Key Features: USB device configuration, endpoint management

**`usb_testutils_controlep.h` / `usb_testutils_controlep.c`**
- Purpose: USB control endpoint specific utilities
- Key Features: Control transfer handling

**`usb_testutils_diags.h`**
- Purpose: USB diagnostics and debugging utilities
- Key Features: USB state inspection, diagnostic output

**`usb_testutils_simpleserial.h` / `usb_testutils_simpleserial.c`**
- Purpose: Simple serial communication over USB
- Key Features: Basic UART-like interface over USB

**`usb_testutils_streams.h` / `usb_testutils_streams.c`**
- Purpose: Stream-based USB communication
- Key Features: Bidirectional data streams over USB

**`usb_logging.h` / `usb_logging.c`**
- Purpose: Logging output over USB
- Key Features: Multi-stream logging, reliable/unreliable delivery modes
- Functions: `usb_logging_enable()`, stream management, flushing 

### System Management & Control

**`pwrmgr_testutils.h` / `pwrmgr_testutils.c`**
- Purpose: Power manager testing utilities
- Key Features: Low power mode setup, wakeup reason checking
- Functions: `pwrmgr_testutils_enable_low_power()`, `pwrmgr_testutils_is_wakeup_reason()`  

**`rstmgr_testutils.h` / `rstmgr_testutils.c`**
- Purpose: Reset manager testing utilities
- Key Features: Reset reason checking, reset request helpers

**`clkmgr_testutils.h` / `clkmgr_testutils.c`**
- Purpose: Clock manager testing utilities
- Key Features: Clock gating control, frequency measurement

**`alert_handler_testutils.h` / `alert_handler_testutils.c`**
- Purpose: Alert handler testing utilities
- Key Features: Alert configuration, alert triggering, interrupt handling

**`aon_timer_testutils.h` / `aon_timer_testutils.c`**
- Purpose: Always-on timer testing utilities
- Key Features: Timer configuration, wakeup setup, interrupt handling

**`lc_ctrl_testutils.h` / `lc_ctrl_testutils.c`**
- Purpose: Lifecycle controller testing utilities
- Key Features: LC state checking, debug function detection, transition count verification
- Functions: `lc_ctrl_testutils_lc_state_log()`, `lc_ctrl_testutils_operational_state_check()` 

**`sensor_ctrl_testutils.h` / `sensor_ctrl_testutils.c`**
- Purpose: Sensor controller testing utilities
- Key Features: Alert sensor configuration and monitoring

**`sysrst_ctrl_testutils.h` / `sysrst_ctrl_testutils.c`**
- Purpose: System reset controller testing utilities
- Key Features: System reset configuration and triggering

**`rv_core_ibex_testutils.h` / `rv_core_ibex_testutils.c`**
- Purpose: RISC-V Ibex core testing utilities
- Key Features: Core configuration, performance counter access

**`rv_plic_testutils.h` / `rv_plic_testutils.c`**
- Purpose: Platform-Level Interrupt Controller testing utilities
- Key Features: Interrupt priority configuration, enabling/disabling

**`pinmux_testutils.h` / `pinmux_testutils.c`**
- Purpose: Pin multiplexer testing utilities
- Key Features: Pin configuration, MIO/DIO pad control

**`dma_testutils.h` / `dma_testutils.c`**
- Purpose: DMA controller testing utilities
- Key Features: DMA transfer setup and management

---

## 3. JSON Command Handlers (`json/`)

Provides host-to-device command interface using JSON over UART/console. 

**`command.h` / `command.c`**
- Purpose: Command enumeration and dispatching
- Commands: Defines all available test commands (ChipStartup, GpioSet, MemRead, SpiFlashWrite, etc.)

**`ottf.h` / `ottf.c`**
- Purpose: OTTF-specific JSON structures
- Contents: CRC structures, OTTF command responses

**`gpio.h` / `gpio.c`**
- Purpose: GPIO control via JSON commands
- Functions: GPIO read/write, interrupt configuration 

**`mem.h` / `mem.c`**
- Purpose: Memory access via JSON commands
- Functions: 32-bit and byte-array memory read/write 

**`i2c_target.h` / `i2c_target.c`**
- Purpose: I2C target mode JSON commands
- Functions: I2C address configuration, transfer initiation

**`pinmux.h` / `pinmux.c`**
- Purpose: Pinmux configuration via JSON
- Functions: Pin assignment and attribute configuration

**`pinmux_config.h` / `pinmux_config.c`**
- Purpose: Pinmux configuration data structures
- Contents: Predefined pinmux configurations

**`spi_passthru.h` / `spi_passthru.c`**
- Purpose: SPI passthrough mode JSON commands
- Functions: Address map configuration for passthrough

**`chip_specific_startup.h` / `chip_specific_startup.c`**
- Purpose: Chip-specific initialization via JSON
- Functions: Handles chip variant startup sequences

**`provisioning_data.h` / `provisioning_data.c`**
- Purpose: Device provisioning data structures
- Contents: Provisioning-related JSON message formats

---

## 4. General Testing Utilities

**`profile.h` / `profile.c`**
- Purpose: Cycle-accurate performance profiling
- Usage: `profile_start()` / `profile_end()` for measuring execution cycles
- Features: Nested profiling support, automatic logging 

**`hexstr.h` / `hexstr.c`**
- Purpose: Hexadecimal string encoding/decoding
- Functions: `hexstr_encode()`, `hexstr_decode()` for binary/hex conversion 

**`hexstr_unittest.cc`**
- Purpose: Unit tests for hexstr functionality
- Framework: C++ unit tests using GoogleTest

**`binary_blob.h`**
- Purpose: C++ template for binary data manipulation in tests
- Features: Seek, find, read/write operations on binary data
- Usage: Host-side testing with GoogleTest 

**`README.md`**
- Purpose: Documentation for the testing library
- Contents: Style guide, naming conventions, design principles

---

## 5. Test ROM (`test_rom/`)

**`test_rom.c`**
- Purpose: Test ROM implementation for chip-level testing
- Function: Minimal ROM for bootstrapping test execution

**`test_rom.ld`**
- Purpose: Linker script for test ROM
- Function: Memory layout for ROM code

**`test_rom_start.S`**
- Purpose: Assembly entry point for test ROM
- Function: CPU initialization and jump to C code

**`test_rom_test.c`**
- Purpose: Test cases for test ROM functionality

**`darjeeling_fake_driver_funcs.c`**
- Purpose: Fake driver functions for Darjeeling chip variant
- Usage: Provides stubs for platform-specific drivers

**`english_breakfast_fake_driver_funcs.c`**
- Purpose: Fake driver functions for English Breakfast chip variant
- Usage: Provides stubs for platform-specific drivers

---

## Key Design Principles

1. **Return Status Pattern**: All testutils functions return `status_t` and are marked with `OT_WARN_UNUSED_RESULT` 

2. **No Abort in Testutils**: Testutils use `TRY_CHECK*()` macros that return errors rather than aborting, allowing graceful error handling 

3. **Top-Level Agnostic**: Testutils avoid including chip-specific headers, making them portable across chip variants  

4. **DIF Integration**: Testutils accept initialized DIF handles as parameters rather than initializing peripherals themselves 

5. **Multi-DIF Operations**: Testutils only wrap operations requiring multiple DIF calls; single DIF calls should use DIFs directly  

---

## Notes

This testing infrastructure supports the complete test lifecycle from device initialization through test execution to result reporting. The modular design allows tests to mix testutils, DIFs, and custom code as needed. The JSON command interface enables sophisticated host-controlled testing scenarios, while the OTTF provides consistent test execution across all verification platforms (DV simulation, Verilator, FPGA, and silicon).