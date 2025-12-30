# Comprehensive Index: OpenTitan DIF Library (`sw/device/lib/dif`)

## Overview

The DIF (Device Interface Function) library provides low-level hardware access routines for OpenTitan peripherals. DIFs are designed for design verification and early silicon verification, not production firmware. 

Each DIF library contains both auto-generated DIFs (under `autogen/` subtree) and manually-implemented DIFs for actuating hardware functionality. 

---

## Core/Base Files

### `README.md`
Overview documentation for the DIF library, including development guidelines, style guide, and API conventions.

### `dif_base.h` / `dif_base.c`
Shared base types and macros used across all DIFs, including `dif_result_t` (global DIF return codes) and `dif_toggle_t` (enable/disable states). 

### `dif_test_base.h`
Testing utilities and base classes for DIF unit tests. 

---

## 1. Cryptography and Security

### AES (Advanced Encryption Standard)
- **`dif_aes.h`** - Public API for AES encryption/decryption operations
- **`dif_aes.c`** - AES DIF implementation supporting ECB, CBC, and other cipher modes 
- **`dif_aes.md`** - Documentation and checklist
- **`dif_aes_unittest.cc`** - Unit tests

### HMAC (Hash-based Message Authentication Code)
- **`dif_hmac.h`** - Public API for HMAC operations
- **`dif_hmac.c`** - HMAC DIF implementation
- **`dif_hmac.md`** - Documentation
- **`dif_hmac_unittest.cc`** - Unit tests

### KMAC (Keccak Message Authentication Code)
- **`dif_kmac.h`** - Public API for KMAC operations
- **`dif_kmac.c`** - KMAC DIF implementation
- **`dif_kmac.md`** - Documentation
- **`dif_kmac_unittest.cc`** - Unit tests

### OTBN (OpenTitan Big Number Accelerator)
- **`dif_otbn.h`** - Public API for the cryptographic coprocessor 
- **`dif_otbn.c`** - OTBN DIF implementation for executing cryptographic operations
- **`dif_otbn.md`** - Documentation
- **`dif_otbn_unittest.cc`** - Unit tests

### CSRNG (Cryptographically Secure Random Number Generator)
- **`dif_csrng.h`** - Public API following NIST SP 800-90Ar1 conventions 
- **`dif_csrng.c`** - CSRNG DIF implementation
- **`dif_csrng_shared.h`** / **`dif_csrng_shared.c`** - Shared CSRNG functionality
- **`dif_csrng.md`** - Documentation
- **`dif_csrng_unittest.cc`** - Unit tests

### EDN (Entropy Distribution Network)
- **`dif_edn.h`** - Public API for entropy distribution
- **`dif_edn.c`** - EDN DIF implementation
- **`dif_edn.md`** - Documentation
- **`dif_edn_unittest.cc`** - Unit tests

### Entropy Source
- **`dif_entropy_src.h`** - Public API for hardware entropy source
- **`dif_entropy_src.c`** - Entropy source DIF implementation
- **`dif_entropy_src.md`** - Documentation
- **`dif_entropy_src_unittest.cc`** - Unit tests

---

## 2. Key and Lifecycle Management

### Key Manager
- **`dif_keymgr.h`** - Public API for key management across boot stages 
- **`dif_keymgr.c`** - Key manager DIF implementation
- **`dif_keymgr.md`** - Documentation
- **`dif_keymgr_unittest.cc`** - Unit tests

### Key Manager DPE (DICE Protection Environment)
- **`dif_keymgr_dpe.h`** - Public API for DPE-specific key management
- **`dif_keymgr_dpe.c`** - Key manager DPE implementation
- **`dif_keymgr_dpe.md`** - Documentation

### Lifecycle Controller
- **`dif_lc_ctrl.h`** - Public API for managing chip lifecycle states 
- **`dif_lc_ctrl.c`** - Lifecycle controller DIF implementation
- **`dif_lc_ctrl.md`** - Documentation
- **`dif_lc_ctrl_unittest.cc`** - Unit tests

### OTP Controller
- **`dif_otp_ctrl.h`** - Public API for one-time programmable memory
- **`dif_otp_ctrl.c`** - OTP controller DIF implementation
- **`dif_otp_ctrl.md`** - Documentation
- **`dif_otp_ctrl_unittest.cc`** - Unit tests

---

## 3. Communication Peripherals

### UART (Universal Asynchronous Receiver-Transmitter)
- **`dif_uart.h`** - Public API for serial communication 
- **`dif_uart.c`** - UART DIF implementation
- **`dif_uart.md`** - Documentation
- **`dif_uart_unittest.cc`** - Unit tests

### SPI Device
- **`dif_spi_device.h`** - Public API for SPI device mode (flash emulation, passthrough)
- **`dif_spi_device.c`** - SPI device DIF implementation
- **`dif_spi_device.md`** - Documentation
- **`dif_spi_device_unittest.cc`** - Unit tests

### SPI Host
- **`dif_spi_host.h`** - Public API for SPI host/controller mode
- **`dif_spi_host.c`** - SPI host DIF implementation
- **`dif_spi_host.md`** - Documentation
- **`dif_spi_host_unittest.cc`** - Unit tests

### I2C (Inter-Integrated Circuit)
- **`dif_i2c.h`** - Public API for I2C communication
- **`dif_i2c.c`** - I2C DIF implementation
- **`dif_i2c.md`** - Documentation
- **`dif_i2c_unittest.cc`** - Unit tests

### USB Device
- **`dif_usbdev.h`** - Public API for USB device functionality
- **`dif_usbdev.c`** - USB device DIF implementation
- **`dif_usbdev.md`** - Documentation
- **`dif_usbdev_unittest.cc`** - Unit tests

### Mailbox
- **`dif_mbx.h`** - Public API for mailbox communication
- **`dif_mbx.c`** - Mailbox DIF implementation
- **`dif_mbx.md`** - Documentation
- **`dif_mbx_unittest.cc`** - Unit tests

---

## 4. General Purpose I/O and Pin Control

### GPIO (General Purpose Input/Output)
- **`dif_gpio.h`** - Public API for GPIO pin control and interrupts 
- **`dif_gpio.c`** - GPIO DIF implementation
- **`dif_gpio.md`** - Documentation
- **`dif_gpio_unittest.cc`** - Unit tests

### Pin Multiplexer
- **`dif_pinmux.h`** - Public API for pin multiplexing configuration
- **`dif_pinmux.c`** - Pin multiplexer DIF implementation
- **`dif_pinmux.md`** - Documentation
- **`dif_pinmux_unittest.cc`** - Unit tests

### Pattern Generator
- **`dif_pattgen.h`** - Public API for pattern generation on GPIO
- **`dif_pattgen.c`** - Pattern generator DIF implementation
- **`dif_pattgen.md`** - Documentation
- **`dif_pattgen_unittest.cc`** - Unit tests

### PWM (Pulse Width Modulation)
- **`dif_pwm.h`** - Public API for PWM output control
- **`dif_pwm.c`** - PWM DIF implementation
- **`dif_pwm.md`** - Documentation
- **`dif_pwm_unittest.cc`** - Unit tests

---

## 5. Timers

### AON Timer (Always-On Timer)
- **`dif_aon_timer.h`** - Public API for always-on timer (watchdog and wakeup)
- **`dif_aon_timer.c`** - AON timer DIF implementation
- **`dif_aon_timer.md`** - Documentation
- **`dif_aon_timer_unittest.cc`** - Unit tests

### RV Timer (RISC-V Timer)
- **`dif_rv_timer.h`** - Public API for RISC-V compliant timer
- **`dif_rv_timer.c`** - RV timer DIF implementation
- **`dif_rv_timer.md`** - Documentation
- **`dif_rv_timer_unittest.cc`** - Unit tests

---

## 6. Power, Clock, and Reset Management

### Power Manager
- **`dif_pwrmgr.h`** - Public API for power state management (wakeup/reset requests) [13](dif索引文档.md#3-12) 
- **`dif_pwrmgr.c`** - Power manager DIF implementation
- **`dif_pwrmgr.md`** - Documentation
- **`dif_pwrmgr_unittest.cc`** - Unit tests

### Clock Manager
- **`dif_clkmgr.h`** - Public API for clock control and gating
- **`dif_clkmgr.c`** - Clock manager DIF implementation
- **`dif_clkmgr.md`** - Documentation
- **`dif_clkmgr_unittest.cc`** - Unit tests

### Reset Manager
- **`dif_rstmgr.h`** - Public API for reset management
- **`dif_rstmgr.c`** - Reset manager DIF implementation
- **`dif_rstmgr.md`** - Documentation
- **`dif_rstmgr_unittest.cc`** - Unit tests

### System Reset Controller
- **`dif_sysrst_ctrl.h`** - Public API for system reset control
- **`dif_sysrst_ctrl.c`** - System reset controller DIF implementation
- **`dif_sysrst_ctrl.md`** - Documentation
- **`dif_sysrst_ctrl_unittest.cc`** - Unit tests

---

## 7. Memory and Storage Controllers

### Flash Controller
- **`dif_flash_ctrl.h`** - Public API for flash memory operations
- **`dif_flash_ctrl.c`** - Flash controller DIF implementation
- **`dif_flash_ctrl.md`** - Documentation
- **`dif_flash_ctrl_unittest.cc`** - Unit tests

### SRAM Controller
- **`dif_sram_ctrl.h`** - Public API for SRAM scrambling and initialization
- **`dif_sram_ctrl.c`** - SRAM controller DIF implementation
- **`dif_sram_ctrl.md`** - Documentation
- **`dif_sram_ctrl_unittest.cc`** - Unit tests

### ROM Controller
- **`dif_rom_ctrl.h`** - Public API for ROM integrity checking
- **`dif_rom_ctrl.c`** - ROM controller DIF implementation
- **`dif_rom_ctrl.md`** - Documentation
- **`dif_rom_ctrl_unittest.cc`** - Unit tests

### DMA (Direct Memory Access)
- **`dif_dma.h`** - Public API for DMA transfers
- **`dif_dma.c`** - DMA DIF implementation
- **`dif_dma.md`** - Documentation
- **`dif_dma_unittest.cc`** - Unit tests

---

## 8. Interrupt and Alert Handling

### RV PLIC (Platform-Level Interrupt Controller)
- **`dif_rv_plic.h`** - Public API for RISC-V PLIC interrupt routing
- **`dif_rv_plic.c`** - RV PLIC DIF implementation
- **`dif_rv_plic.md`** - Documentation
- **`dif_rv_plic_unittest.cc`** - Unit tests

### Alert Handler
- **`dif_alert_handler.h`** - Public API for alert classification and handling 
- **`dif_alert_handler.c`** - Alert handler DIF implementation
- **`dif_alert_handler.md`** - Documentation
- **`dif_alert_handler_unittest.cc`** - Unit tests

---

## 9. System Control and Debug

### RV Core Ibex
- **`dif_rv_core_ibex.h`** - Public API for Ibex RISC-V core control
- **`dif_rv_core_ibex.c`** - RV core Ibex DIF implementation
- **`dif_rv_core_ibex.md`** - Documentation
- **`dif_rv_core_ibex_unittest.cc`** - Unit tests

### RV Debug Module
- **`dif_rv_dm.h`** - Public API for RISC-V debug module
- **`dif_rv_dm.c`** - RV debug module DIF implementation
- **`dif_rv_dm.md`** - Documentation
- **`dif_rv_dm_unittest.cc`** - Unit tests

### Sensor Controller
- **`dif_sensor_ctrl.h`** - Public API for analog sensor monitoring
- **`dif_sensor_ctrl.c`** - Sensor controller DIF implementation
- **`dif_sensor_ctrl.md`** - Documentation
- **`dif_sensor_ctrl_unittest.cc`** - Unit tests

### SoC Debug Controller
- **`dif_soc_dbg_ctrl.h`** - Public API for SoC-level debug control
- **`dif_soc_dbg_ctrl.c`** - SoC debug controller DIF implementation

### SoC Proxy
- **`dif_soc_proxy.h`** - Public API for SoC proxy functionality
- **`dif_soc_proxy.c`** - SoC proxy DIF implementation

---

## 10. Analog and Mixed-Signal

### ADC Controller
- **`dif_adc_ctrl.h`** - Public API for analog-to-digital converter
- **`dif_adc_ctrl.c`** - ADC controller DIF implementation
- **`dif_adc_ctrl.md`** - Documentation
- **`dif_adc_ctrl_unittest.cc`** - Unit tests

---

## File Naming Conventions

Each DIF library follows a consistent pattern:

- **`.h` files**: Public API headers with function declarations and type definitions
- **`.c` files**: Implementation files containing the actual DIF logic
- **`.md` files**: Documentation and development checklists 
- **`_unittest.cc` files**: C++ unit tests for the DIF 
- **`autogen/` subdirectory**: Auto-generated DIFs (included via public headers)  

---

## Notes

- All DIFs must conform to strict coding standards defined in the DIF Style Guide 
- DIFs are transaction-oriented and support lifecycle functions (init, configure, start, end) 
- Interrupt management DIFs are auto-generated from IP HJSON configuration files 
- DIFs must not depend on other DIF libraries 