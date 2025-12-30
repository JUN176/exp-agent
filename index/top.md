# Files in `hw/top` Directory

The `opentitan/bazel-bin/hw/top` directory (specifically within `bazel-bin`) contains auto-generated register definition headers for the various hardware IP blocks in the OpenTitan top-level design. These files define register offsets, bit fields, and other hardware-specific constants.

## Cryptography & Security

**`aes_regs.h`**: Register definitions for the AES (Advanced Encryption Standard) peripheral.

**`csrng_regs.h`**: Register definitions for the CSRNG (Cryptographically Secure Random Number Generator).

**`edn_regs.h`**: Register definitions for the EDN (Entropy Distribution Network).

**`entropy_src_regs.h`**: Register definitions for the Entropy Source.

**`hmac_regs.h`**: Register definitions for the HMAC (Hash-based Message Authentication Code) peripheral.

**`kmac_regs.h`**: Register definitions for the KMAC (Keccak Message Authentication Code) peripheral.

**`otbn_regs.h`**: Register definitions for the OTBN (OpenTitan Big Number Accelerator).

**`otp_ctrl_regs.h`**: Register definitions for the OTP (One-Time Programmable) Controller.

## System Control, Memory & Power

**`ast_regs.h`**: Register definitions for the AST (Analog Sensor Top).

**`clkmgr_regs.h`**: Register definitions for the Clock Manager.

**`flash_ctrl_regs.h`**: Register definitions for the Flash Controller.

**`lc_ctrl_regs.h`**: Register definitions for the Life Cycle Controller.

**`rstmgr_regs.h`**: Register definitions for the Reset Manager.

**`sensor_ctrl_regs.h`**: Register definitions for the Sensor Controller.

**`sram_ctrl_regs.h`**: Register definitions for the SRAM Controller.

## Peripherals / IO

**`gpio_regs.h`**: Register definitions for the GPIO (General Purpose Input/Output) peripheral.

**`pinmux_regs.h`**: Register definitions for the Pin Multiplexer.

**`spi_device_regs.h`**: Register definitions for the SPI Device peripheral.

**`uart_regs.h`**: Register definitions for the UART (Universal Asynchronous Receiver-Transmitter) peripheral.

## Processor & Core

**`rv_core_ibex_regs.h`**: Register definitions for the Ibex RISC-V Core configuration.

**`rv_plic_regs.h`**: Register definitions for the RISC-V PLIC (Platform-Level Interrupt Controller).

**`rv_timer_regs.h`**: Register definitions for the RISC-V Timer.

---

# Files in `opentitan/bazel-bin/hw/top/dt` Directory

The `opentitan/bazel-bin/hw/top/dt` directory contains Device Table (DT) related files, which likely provide hardware configuration and address information for various IP blocks in the OpenTitan design. These files are typically used to look up hardware parameters and base addresses.

## Core / API

**`dt_api.h` / `dt_api.c`**: Core Device Table API functions and definitions used by other DT modules.

## Cryptography & Security

**`dt_aes.h` / `dt_aes.c`**: Device Table definitions for the AES (Advanced Encryption Standard) peripheral.

**`dt_csrng.h` / `dt_csrng.c`**: Device Table definitions for the CSRNG (Cryptographically Secure Random Number Generator).

**`dt_edn.h` / `dt_edn.c`**: Device Table definitions for the EDN (Entropy Distribution Network).

**`dt_entropy_src.h` / `dt_entropy_src.c`**: Device Table definitions for the Entropy Source.

**`dt_hmac.h` / `dt_hmac.c`**: Device Table definitions for the HMAC (Hash-based Message Authentication Code) peripheral.

**`dt_keymgr.h` / `dt_keymgr.c`**: Device Table definitions for the Key Manager.

**`dt_kmac.h` / `dt_kmac.c`**: Device Table definitions for the KMAC (Keccak Message Authentication Code) peripheral.

**`dt_otbn.h` / `dt_otbn.c`**: Device Table definitions for the OTBN (OpenTitan Big Number Accelerator).

**`dt_otp_ctrl.h` / `dt_otp_ctrl.c`**: Device Table definitions for the OTP (One-Time Programmable) Controller.

**`dt_otp_macro.h` / `dt_otp_macro.c`**: Device Table definitions for the OTP Macro.

## System Control, Memory & Power

**`dt_alert_handler.h` / `dt_alert_handler.c`**: Device Table definitions for the Alert Handler.

**`dt_aon_timer.h` / `dt_aon_timer.c`**: Device Table definitions for the Always-On Timer.

**`dt_ast.h` / `dt_ast.c`**: Device Table definitions for the AST (Analog Sensor Top).

**`dt_clkmgr.h` / `dt_clkmgr.c`**: Device Table definitions for the Clock Manager.

**`dt_flash_ctrl.h` / `dt_flash_ctrl.c`**: Device Table definitions for the Flash Controller.

**`dt_lc_ctrl.h` / `dt_lc_ctrl.c`**: Device Table definitions for the Life Cycle Controller.

**`dt_pwrmgr.h` / `dt_pwrmgr.c`**: Device Table definitions for the Power Manager.

**`dt_rom_ctrl.h` / `dt_rom_ctrl.c`**: Device Table definitions for the ROM Controller.

**`dt_rstmgr.h` / `dt_rstmgr.c`**: Device Table definitions for the Reset Manager.

**`dt_sensor_ctrl.h` / `dt_sensor_ctrl.c`**: Device Table definitions for the Sensor Controller.

**`dt_sram_ctrl.h` / `dt_sram_ctrl.c`**: Device Table definitions for the SRAM Controller.

**`dt_sysrst_ctrl.h` / `dt_sysrst_ctrl.c`**: Device Table definitions for the System Reset Controller.

## Peripherals / IO

**`dt_adc_ctrl.h` / `dt_adc_ctrl.c`**: Device Table definitions for the ADC (Analog-to-Digital Converter) Controller.

**`dt_gpio.h` / `dt_gpio.c`**: Device Table definitions for the GPIO (General Purpose Input/Output) peripheral.

**`dt_i2c.h` / `dt_i2c.c`**: Device Table definitions for the I2C peripheral.

**`dt_pattgen.h` / `dt_pattgen.c`**: Device Table definitions for the Pattern Generator.

**`dt_pinmux.h` / `dt_pinmux.c`**: Device Table definitions for the Pin Multiplexer.

**`dt_pwm.h` / `dt_pwm.c`**: Device Table definitions for the PWM (Pulse Width Modulation) peripheral.

**`dt_spi_device.h` / `dt_spi_device.c`**: Device Table definitions for the SPI Device peripheral.

**`dt_spi_host.h` / `dt_spi_host.c`**: Device Table definitions for the SPI Host peripheral.

**`dt_uart.h` / `dt_uart.c`**: Device Table definitions for the UART (Universal Asynchronous Receiver-Transmitter) peripheral.

**`dt_usbdev.h` / `dt_usbdev.c`**: Device Table definitions for the USB Device peripheral.

## Processor & Core

**`dt_rv_core_ibex.h` / `dt_rv_core_ibex.c`**: Device Table definitions for the Ibex RISC-V Core.

**`dt_rv_dm.h` / `dt_rv_dm.c`**: Device Table definitions for the RISC-V Debug Module.

**`dt_rv_plic.h` / `dt_rv_plic.c`**: Device Table definitions for the RISC-V PLIC (Platform-Level Interrupt Controller).

**`dt_rv_timer.h` / `dt_rv_timer.c`**: Device Table definitions for the RISC-V Timer.
